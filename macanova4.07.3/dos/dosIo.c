/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 4.06 or later
*(C)*
*(C)* Copyright (c) 1998 by Gary Oehlert and Christopher Bingham
*(C)* unless indicated otherwise
*(C)*
*(C)* You may give out copies of this software;  for conditions see the
*(C)* file COPYING included with this distribution
*(C)*
*(C)* This file is distributed WITHOUT ANY WARANTEE; without even
*(C)* the implied warantee of MERCHANTABILITY or FITNESS FOR
*(C)* A PARTICULAR PURPOSE
*(C)*
*/

#include "globals.h"
#include "matdat.h"

#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>

#ifdef HASPWDH
#include <pwd.h>
#ifdef EPX
struct passwd *getpwnam(char *);
#endif
#endif /*HASPWDH*/

/*
   950704 Added interface with readline library.  Other changes in getinput()
   now in commonio.c

   970111 added initHistory()

   980508 Made modifications in fileToLine.  The most important change
   is that it returns ENDOFSTRING (-2) when hitting '\0' instead of '\n'
   (should happen only when reading from string)
*/

#ifdef BCPP
#include <conio.h> /* for kbhit()*/
#endif /*BCPP*/

#ifndef BS
#define BS     8
#endif /*BS*/
#ifndef DEL
#define DEL  127
#endif /*DEL*/

/* Macros for working with file and path names */
#ifdef MSDOS
/*  '/' equivalent to '\' on MSDOS*/
#define isSlash(C) ((C) == DIRSEPARATOR[0] || (C) == '/')
#define isPath(N) (strchr(N, '/') || strchr(N, '\\') || strchr(N, ':'))
#else /*MSDOS*/
#define isSlash(C) ((C) == DIRSEPARATOR[0])
#define isPath(N) strchr(N, DIRSEPARATOR[0])
#endif /*MSDOS*/

/*
   Because the code for saving lines to update the screen may need to allocate
   additional memory for a long line, it calls mygetsafehandle() and
   mygrowsafehandle() instead of mygethandle() and mygrowhandle().  In an
   implementation of memory management under which the storage pointed to
   indirectly by handles can move (as on a Macintosh), these entries should
   not do anything that might move the contents of allocated storage.  Hence
   I/O ought to be safe anywhere, without extra precautions, even when printing
   some or all of the contents a handle

   Under the current implementation on Unix and DOS which uses malloc() and its
   relatives, this precaution should be unnecessary.
*/
void fmyprint(char * /*msg*/, FILE * /*fp*/);
void fmyeol(FILE * /*fp*/);
void putprompt(char * /*prompt*/);
long isfilename(char * /*fname*/);

#ifdef READLINE
/*980511 changed Readline_prompt to global from static*/
char          *Readline_prompt = (char *) 0;  /* this is set by putprompt() */
static char   *Line_read = (char *) 0; /* pointer to last line read */
#endif /*READLINE*/

#define BEL '\007'

static int         UpdateActive = -1;
#ifdef ACTIVEUPDATE

#define MAXSAVEDLINES    60
#define DEFAULTLENGTH    80
#define SAVEDCHUNK       20
#define LineI(I) (*(SavedLines[i]))

static char      **SavedLines[MAXSAVEDLINES];
static long        NSavedLines = 0;
static long        LinePlace = 0;

static void   shiftSavedLines(void)
{
	short       i;
	char      **temp;
	
	if (NSavedLines > 0)
	{
		temp = SavedLines[NSavedLines-1];
		for (i = NSavedLines - 1; i > 0; i--)
		{
			SavedLines[i] = SavedLines[i - 1];
		}
		SavedLines[0] = temp;
		(*temp)[0] = '\0';
		LinePlace = 0;
	} /*if (NSavedLines > 0)*/
} /*shiftSaveLines()*/

/*
   save a line put to the screen for use in updating the screen after
   a high resolution plot

   971104 added check on value of myhandlelength()
*/

void     saveForUpdate(char *msg)
{
	char      **line;
	long        handleLength;
	short       i;
	short       msgLength;
	short       iseol;
	
	if(UpdateActive > 0 && *msg)
	{
		line = SavedLines[0];

		msgLength = strlen(msg);
		
		iseol = (msg[msgLength-1] == '\n');
		handleLength = myhandlelength(line);
		
		if (handleLength > 0 && LinePlace + msgLength + 1 > handleLength)
		{
			line = mygrowsafehandle(line, LinePlace + msgLength + SAVEDCHUNK);
			SavedLines[0] = line;
			if(line == (char **) 0)
			{
				goto errorExit;
			}
		}
		strcat(*line + LinePlace, msg);
		LinePlace += msgLength;
		(*line)[LinePlace] = '\0';
		if(iseol)
		{
			shiftSavedLines();
		}
	} /*if(UpdateActive > 0 && *msg)*/
	return;

  errorExit:
/* don't know anything better to do that stop doing updating */
	for(i=0;i<NSavedLines;i++)
	{
		mydisphandle(SavedLines[i]);
		SavedLines[i] = (char **) 0;
	} /*for(i=0;i<NSavedLines;i++)*/
	NSavedLines = 0;
} /*saveForUpdate()*/

/*
   Update screen with the most recent NSavedLines of output (after a 
   high resolution plot)
*/
void     updateConsole(void)
{
	long     i, j, nlines= NLINES;
	char    *line;
	
	if(UpdateActive > 0)
	{
		UpdateActive = 0; /* to suppress beeps on updated error messages*/
		
		for (i=NSavedLines - 1; i >= 0; i--)
		{
			line = LineI(i);			
			if(*line != '\0')
			{
				NLINES = 0; /* make sure it doesn't trigger pause */
				myprint(line);
			}
		} /*for (i=NSavedLines - 1; i >= 0; i--)*/
		NLINES = nlines;
		UpdateActive = 1;
	} /*if(UpdateActive > 0)*/
} /*updateConsole()*/
	
/*
   Initialize stuff relative to saving output lines to update the screen
   after a high resolution plot.  Static array SavedLines is created and
   it elements initialized to handles to null strings.
*/
void activeUpdateInit(void)
{
	long        nlines = (SCREENHEIGHT) ? SCREENHEIGHT : 24;
	long        i;
	
	nlines = (nlines > MAXSAVEDLINES) ? MAXSAVEDLINES : nlines;
	for (i = NSavedLines; i < nlines; i++)
	{
		SavedLines[i] = mygetsafehandle(DEFAULTLENGTH+1);
		if(SavedLines[i] == (char **) 0)
		{
			break;
		}
		(*SavedLines[i])[0] = '\0';
		NSavedLines++;
	} /*for (i = NSavedLines; i < nlines; i++)*/
	UpdateActive = (NSavedLines != 0) ? 1 : -1;

} /*activeUpdateInit*/
#else /*ACTIVEUPDATE*/
/*
   Null versions of routines related to updating the screen after
   a high resolution plot
*/
static void   shiftSavedLines(void)
{
} /*shiftSaveLines()*/

void     saveForUpdate(char *msg)
{
} /*saveForUpdate()*/

void     updateConsole(void)
{
} /*updateConsole()*/
	
void activeUpdateInit(void)
{
} /*activeUpdateInit*/

#endif /*ACTIVEUPDATE*/

/*
  980511 moved datagetc(), fileToLine() and fillLine() to commonio.c
*/

#ifndef SCROLLABLEWINDOW
static short    JustPaused = 0;

/*
   If READLINE is defined, MacAnova uses the readline library to allow
   intereactive editing of lines.  The code used is based on a first draft
   provided by Bernd Feige charly@headmod.uni-muenster.de
*/

#ifdef READLINE

#if defined (READLINE_LIBRARY)
#  include "readline.h"
#  include "history.h"
#else /*READLINE_LIBRARY*/
#  include <readline/readline.h>
#  include <readline/history.h>
#endif /*READLINE_LIBRARY*/

#ifdef DISABLECONTROLY  /* should be defined if ^Y suspends process*/
/*
   This may be unnecessary if readline library is properly compiled;
   see rltty.c
*/
/*
  Define IOCTLERRORMSG to get error message after ioctl error
  for use in debugging
*/
#undef IOCTLERRORMSG
#ifdef IOCTLERRORMSG
extern void perror(char *);
#endif /*IOCTLERRORMSG*/

#define CONTROLY    0x19
static int       IoctlReply__;

#ifdef HPUX
#define _INCLUDE_HPUX_SOURCE
#undef _HPUX_SOURCE
#define _HPUX_SOURCE
#include <sys/bsdtty.h>

#define TINFOTYPE struct ltchars
#define getDsuspc(TI,V)    (IoctlReply__ = ioctl(0, TIOCGLTC, (char *) TI),\
							*(V) = TI->t_dsuspc, IoctlReply__)
#define setDsuspc(TI,V)  (TI->t_dsuspc = V, ioctl(0, TIOCSLTC, (char *) TI))

#define NODSUSPC    0xff
#endif /*HPUX*/

#ifdef EPX
#if (0)
/*
  With DIABLECONTROLY defined, the following compiled.
  However, ioctl(0,TIOGCLTC,&ti) and ioctl(0, TIOCSLTC, &ti) returned -1 with
  errno set to ENOTTY (not a character device)
  The alternative code using TCGETA and TCSETA works correctly
*/
/* following corrected from what is in <bsd43/bsd43_.h> */
#   define BSD43_(x) BSD43_##x
#   define bsd43_(x) bsd43_##x

#include <bsd43/sys/ioctl.h>
/* following taken from part of <bsd43/sys/ioctl.h> that is skipped*/
#   define TIOCGLTC BSD43_(TIOCGLTC)
#   define TIOCSLTC BSD43_(TIOCSLTC)

#define TINFOTYPE struct bsd43_(ltchars)
#define getDsuspc(TI,V)    (IoctlReply__ = ioctl(0, TIOCGLTC, (char *) TI),\
							*(V) = TI->t_dsuspc, IoctlReply__)
#define setDsuspc(TI,V)  (TI->t_dsuspc = V, ioctl(0, TIOCSLTC, (char *) TI))

#else /*0*/
#include <sys/termio.h>
#define TINFOTYPE struct termio
#define getDsuspc(TI,V)    (IoctlReply__ = ioctl(0, TCGETA, (char *) TI),\
							*(V) = TI->c_cc[V_DSUSP], IoctlReply__)
#define setDsuspc(TI,V)  (TI->c_cc[V_DSUSP] = V, ioctl(0, TCSETA, (char *) TI))
#endif /*0*/

#define NODSUSPC    0xff
#endif /*EPX*/

static TINFOTYPE       TerminalInfo;
static TINFOTYPE      *Ti = (TINFOTYPE *) 0;
static unsigned char   Dsuspc = '\0';

static void disableControlY(void)
{
	if (Ti == (TINFOTYPE *) 0)
	{ 
		/* first time here*/
		Ti = &TerminalInfo;
		if (getDsuspc(Ti, &Dsuspc) == -1)
		{
#ifdef IOCTLERRORMSG
			perror("macanova");
#endif /*IOCTLERRORMSG*/
			Dsuspc = CONTROLY; /* for lack of anything better to do */
		}
		
	} /*if (Ti == (TINFOTYPE *) 0)*/

	if (setDsuspc(Ti, NODSUSPC) == -1)  /* allows ^Y to be used for yank*/
	{
#ifdef IOCTLERRORMSG
		perror("macanova");
#endif /*IOCTLERRORMSG*/
	}
} /*disableControlY()*/

static void enableControlY(void)
{
/* restore Dsuspc; I don't know if this is necessary */
	setDsuspc(Ti, Dsuspc);
} /*enableControlY()*/

#else
#define enableControlY()
#define disableControlY()
#endif /*DISABLECONTROLY*/

/*
   Initialize things
*/

void my_rl_init(void)
{
	static int             first = 1;

	if (first)
	{
		first = 0;
		rl_readline_name = "macanova";
		rl_initialize (); /* just in case not yet done */
#if (0)
	/* these don't seem to be needed, both DEL & BS already bound */
		rl_bind_key(BS, (Function *) rl_rubout);
		rl_bind_key(DEL, (Function *) rl_rubout);
#endif
	} /*if (first)*/

	disableControlY();

} /*my_rl_init()*/


extern int rl_done; /*defined in readline.c, not declared in readline.h*/

static Keymap RLKeymap = (Keymap) 0; /* copy of standard readline keymap */
static Keymap PauseKeymap = (Keymap) 0; /* single key scan key map */
static char  *PauseLine = (char *) 0;

/* rl_startup_hook is set to pauseMsg() when pausing on full screen*/
static void pauseMsg(void)
{
	rl_insert_text("Press 'q' to quit, 'j' or 'n' to see next line, any other key to continue");
} /*pauseMsg()*/

/* all readline keys are bound to rl_pause() when pausing*/
static int rl_pause(int count, int c)
{
	char      buffer[2];
	
	rl_end = rl_point = 0; /* readline buffer empty */
	rl_redisplay(); /* wipes out pause message */
	buffer[0] = c; /* return key hit */
	buffer[1] = '\0';
	rl_insert_text(buffer);   
	rl_done = 1; /*tell readline() to return*/
	return (0);
} /*rl_pause(*/

#ifdef READLINE11 
/*
   readline version 1.1 does not have rl_get_keymap() and rl_set_keymap()
*/

extern Keymap keymap;

Keymap rl_get_keymap(void)
{
	return( keymap );
} /*rl_get_keymap()*/

void rl_set_keymap(Keymap map)
{
	if(map)
	{
		keymap = map;
	}
} /*rl_set_keymap()*/
#endif /* READLINE11 */

static int rlpauseInit(void)
{
	int      i;

	my_rl_init();

	enableControlY(); /*undo disableControlY()*/

	if (PauseLine != (char *) 0)
	{
		free(PauseLine);
		PauseLine = (char *) 0;
	} /*if (PauseLine != (char *) 0)*/

	if (PauseKeymap == (Keymap) 0)
	{
		PauseKeymap = rl_make_keymap();

		if (PauseKeymap != (Keymap) 0)
		{
			RLKeymap = rl_get_keymap();
			for (i = 0; i < KEYMAP_SIZE; i++)
			{ /* every key binds to rl_pause()*/
				rl_bind_key_in_map(i, (Function *) rl_pause, PauseKeymap);
			} /*for (i = 0; i < KEYMAP_SIZE; i++)*/	
		} /*if (PauseKeymap != (Keymap) 0)*/
		else
		{
			RLKeymap = (Keymap) 0;
			return (0);
		} /*if (PauseKeymap != (Keymap) 0){}else{}*/
	} /*if (RLKeymap == (Keymap) 0)*/
	rl_set_keymap(PauseKeymap);
	rl_startup_hook = (Function *) pauseMsg;
	return (1);
} /*rlpauseInit()*/

/*
   ensure regular keymap is in place and rl-startup-hook is cleared
*/
void rlpauseCleanup(void)
{
	rl_startup_hook = (Function *) 0;
	if (RLKeymap != (Keymap) 0)
	{
		rl_set_keymap(RLKeymap);
	}
	if (PauseLine != (char *) 0)
	{
		free(PauseLine);
		PauseLine = (char *) 0;
	} /*if (PauseLine != (char *) 0)*/	
} /*rlpauseCleanup()*/

#define CONTROL_N    0x0e

#define JUSTONELINE(C) (C == 'j' || C == 'J' ||\
					C == 'n' || C == 'N' ||\
					C == CONTROL_N)
#endif /*READLINE*/

void incrementNLINES(char *msg)
{
	int           lines = 0;
	int           c, c1;
	char         *truncateMsg = "****Output truncated by user****\n";
	WHERE("incrementNLINES");
	
	while (*msg != '\0')
	{
		if (*msg++ == '\n')
		{
			lines++;
		}
	}
	NLINES += lines;

	if((ISATTY == (ITTYIN | ITTYOUT)) && SCREENHEIGHT > 0 &&
	   NLINES >= SCREENHEIGHT-1)
	{
		if (!JustPaused)
		{ /* don't pause twice in a row */
#ifdef READLINE
			long      nlines = NLINES;

			if (rlpauseInit())
			{
				char          nullPrompt[1];

				nullPrompt[0] = '\0';

				/* display pause message and get key pressed */
#if (0)
				PauseLine = readline((char *) 0);
#else /*0*/
				PauseLine = readline(nullPrompt);
#endif /*0*/
				c = (PauseLine != (char *) 0) ? PauseLine[0] : '\n';
				rlpauseCleanup();
			}
			else
#endif /*READLINE*/
			{
				fprintf(stderr,
						"Hit RETURN to continue or q RETURN to go to next command line: ");
				c = getchar();
				if(c != '\n' && c != '\r')
				{
					while((c1 = getchar()) != '\n' && c1 != '\r' && c1 != EOF)
					{
						;
					}
				} /*if(c != '\n' && c != '\r')*/
			}

			JustPaused = 1;
			NLINES = 0;
			if(c == 'q' || c == 'Q')
			{                               
				if (SPOOLFILE != (FILE *) 0)
				{
					fputs(truncateMsg, SPOOLFILE);
				}
				INTERRUPT = PRINTABORT;
#ifdef SIGNALARG
				intRoutine(0);
#else /*SIGNALARG*/
				intRoutine();
#endif /*SIGNALARG*/
				/* no return */
			} /*if(c == 'q' || c == 'Q')*/

#ifdef READLINE
			if (JUSTONELINE(c))
			{ /* advance 1 line on j, J, n, N, or ^n*/
				JustPaused = 0;
				NLINES = nlines-1;
			} /*if (JUSTONELINE(c))*/
#endif /*READLINE*/
		} /*if (!JustPaused)*/
	} /*if((ISATTY & ITTYIN) && SCREENHEIGHT > 0 && NLINES >= SCREENHEIGHT-1)*/
	else
	{
		JustPaused = 0;
	}
} /*incrementNLINES()*/
#else /*SCROLLABLEWINDOW*/

void incrementNLINES(char *msg)
{
	while (*msg != '\0')
	{
		if (*msg++ == '\n')
		{
			NLINES++;
		}
	}
} /*incrementNLINES()*/

#endif /*SCROLLABLEWINDOW*/

/*
   Open file in appropriate mode; may do things differently on different
   platforms;
*/

FILE * fmyopen(char * fileName, char * mode)
{
	Symbolhandle   dataPaths = (Symbolhandle) 0;
	FILE           *trialFile = fopen(fileName, mode);
	long            ipath, npaths, pathLength, nameLength;
	char           *pathName;
	char            path[PATHSIZE + 1];
	
	if (trialFile != (FILE *) 0 || strchr(mode, 'w') || isPath(fileName))
	{
		return (trialFile);
	}

	if (((dataPaths = Lookup("DATAPATHS")) != (Symbolhandle) 0 ||
		 (dataPaths = Lookup("DATAPATH")) != (Symbolhandle) 0) &&
		TYPE(dataPaths) == CHAR)
	{
		pathName = STRINGPTR(dataPaths);
		npaths = symbolSize(dataPaths);
		nameLength = strlen(fileName);
		for (ipath = 0; ipath < npaths;
			 ipath++, pathName = skipStrings(pathName, 1))
		{
			pathLength = strlen(pathName);
			if (okpathname(pathName) && pathLength + nameLength <= PATHSIZE)
			{
				strcpy(path, pathName);
				if (pathLength > 0 && !isSlash(pathName[pathLength-1]))
				{
					path[pathLength++] = DIRSEPARATOR[0];
				}
				if (pathLength + nameLength <= PATHSIZE)
				{
					strcpy(path + pathLength, fileName);
					if (okfilename(path) &&
						(trialFile = fopen(path,mode)) != (FILE *) 0)
					{
						break;
					}
				} /*if (pathLength + nameLength <= PATHSIZE)*/
			} /*if (okpathname(pathName) && pathLength+nameLength <= PATHSIZE)*/
		} /*for (ipath = 0; ipath < npaths; ...)*/
	}
	return (trialFile);
} /*fmyopen()*/

/*
  Write line of output to file fp
  If writing to console, interpret leading '\007's as BEL's
  If spooling is active, write line to spool file
  If line starts with "ERROR:", ring bell and count error
*/
  
void fmyprint(char *msg, FILE *fp)
{
	WHERE("fmyprint");

	if(fp == STDOUT)
	{
		if(UpdateActive == 0 ||
		   PRINTWARNINGS || strncmp("WARNING:",msg, (int) 8) != 0)
		{
			while(*msg == BEL)
			{ /* turn leading BEL's into beeps */
				mybeep(1);
				msg++;
			}

			if (UpdateActive != 0 && SPOOLFILE != (FILE *) 0)
			{
				fputs(msg, SPOOLFILE);
			}

			if(UpdateActive != 0 && strncmp("ERROR:",msg, (int) 6) == 0)
			{
				if(MAXERRORS1)
				{
					NERRORS++;
				}
				mybeep(1);
			}
			fputs(msg, stdout);
			incrementNLINES(msg);
		}
		saveForUpdate (msg);
	} /*if(fp == STDOUT)*/
	else
	{
		fputs(msg, fp);
	}
} /*fmyprint()*/

/*
  terminate output line and pause if writing the console and screen is full
  (no pausing on Mac)
*/

void fmyeol(FILE * fp)
{
	WHERE("fmyeol");
	
	fmyprint("\n",fp);
	
} /*fmyeol()*/

void fmyflush(FILE * fp)
{
	fflush((fp == STDOUT) ? stdout : fp);
}

/*
  sound n beeps, but only if not reading batch file and output is to console
*/

#define BELL "\007" /* this is char *, BEL is char */

void mybeep(long n)
{
	if(BDEPTH == 0 && (ISATTY & ITTYOUT))
	{
		while(n-- > 0)
		{
			fputs(BELL,stdout);
		}
	}
} /*mybeep()*/

/*
   Return filename part of path
*/

static char * pathTail(char *filename)
{
	char     *tail;

	for (tail = filename + strlen(filename) - 1;tail >= filename;tail--)
	{
#ifndef MSDOS
		if (isSlash(*tail))
		{
			break;
		}
#else /*MSDOS*/
		if (isSlash(*tail) || *tail == ':')
		{
			break;
		}
#endif /*MSDOS*/
	} /*for (tail = filename + strlen(filename) - 1;tail >= filename;tail--)*/
	tail++;
	return (tail);
} /*pathTail()*/

/* 
  If argument prompt is NULL, use global PROMPT (BDEPTH == 0)
  or batch file name (BDEPTH > 0), preceded by EOL.  This should only
  happen at first prompt for a command line.
  If prompt != NULL (probably "more> "), print it with no preceding EOL

  980415 modified to reflect changed prompt handling which allows a default
  prompt to be supplied instead of the file name when BDEPTH > 0
*/

void putprompt(char * prompt)
{
	char    *prmpt;
	WHERE("putprompt");

	/* write out the prompt */
	prmpt = (prompt == (char *) 0) ? PROMPT : prompt;

	if(BDEPTH == 0)
	{
		if(prompt == (char *) 0)
		{
			myeol();
			NLINES = 0;
		}
#ifdef READLINE
		Readline_prompt = (ISATTY == (ITTYIN | ITTYOUT)) ? prmpt : (char *) 0;
		if (Readline_prompt == (char *) 0)
#endif /*READLINE*/
		{			
			myprint(prmpt);
		}
#ifdef SCROLLABLEWINDOW
		CURRENTSCROLLBACK = SCROLLBACK; /* reset to default */
#endif /*SCROLLABLEWINDOW*/
	} /*if(BDEPTH == 0)*/
	else if(BATCHECHO[BDEPTH])
	{
		if(prompt == (char *) 0)
		{
			myeol();
		}
		if (prmpt[0] != '\0')
		{
			myprint(prmpt);
		}
		else
		{
			myprint(pathTail(*INPUTFILENAMES[BDEPTH]));
			myprint("> ");
		}
	} /*if(BDEPTH == 0){}else{}*/
} /*putprompt()*/


#ifdef READLINE
/*
   Read a string and return a pointer to it.  Returns NULL on EOF
*/

static char *rl_gets(void)
{
	WHERE("rl_gets");

	my_rl_init();

/*
   If the buffer has already been allocated, return the memory to the
   free pool.
*/
	if (Line_read != (char *) 0)
	{
		free(Line_read);
		Line_read = (char *) 0;
	} /*if (Line_read != (char *) 0)*/
	
	/* Get a line from the user. */

	if (Readline_prompt != (char *) 0)
	{
		saveForUpdate(Readline_prompt);
			
		if (SPOOLFILE != (FILE *) 0)
		{
			fputs(Readline_prompt, SPOOLFILE);
		}
	} /*if (Readline_prompt != (char *) 0)*/
	
	Line_read = readline (Readline_prompt);
	
	enableControlY();

	return (Line_read);
} /*rl_gets()*/

static char    *In_line = (char *) 0;

int my_rl_getc(FILE *f)
{
	if (f == stdin && ISATTY == (ITTYIN | ITTYOUT))
	{ /* don't use readline unless both input and output are tty's */
		if (In_line != (char *) 0 && *In_line == '\0')
		{
			In_line = (char *) 0;
			return ((int) '\n');
		} /*if (In_line != (char *) 0 && *In_line == '\0')*/

		if (In_line == (char *) 0)
		{
			In_line = rl_gets();
			if (In_line == (char *) 0)
			{
				return (EOF);
			}
		} /*if (In_line == (char *) 0)*/

		if (*In_line == '\0')
		{
			In_line = (char *) 0;
			return ((int) '\n');
		} /*if (*In_line == '\0')*/

		return ((int) *In_line++);
	} /*if (f == stdin)*/
	else
	{
		return (getc(f));
	} /*if (f == stdin){}else{}*/
} /*my_rl_getc()*/	

#endif /*READLINE*/

#ifdef SAVEHISTORY

void initHistory(long historyLength)
{
	historyLength = (historyLength < 0) ? 0 : historyLength;
	stifle_history(historyLength);
	HISTORY = historyLength;
} /*initHistory()*/

/*
   Save *lineH in history list.
   Note:  Conceivable future danger if add_history() caused any storage
   referenced by handles to move.
*/

void saveInHistory(char ** lineH)
{
	add_history(*lineH);
} /*saveInHistory()*/

enum historyScratch
{
	GLINES = 0,
	NTRASH
};
/*
	980108 added test for historyList == NULL
*/
long getSomeHistory(long nlines, char *** historyHandle)
{
	HIST_ENTRY   **historyList = history_list();
	long           start = 0, entries = 0;
	long           i, needed;
	char          *place, **lines;
	TRASH(NTRASH, errorExit);
	
	if (nlines == 0)
	{
		nlines = 1000000;
	}

	if (historyList == (HIST_ENTRY **) 0 ||
		historyList[0] == (HIST_ENTRY *) 0 ||
		historyList[1] == (HIST_ENTRY *) 0)
	{
		entries = 0;
	}
	else
	{
		/* don't count most recent entry */
		for (entries = 0; historyList[entries+1]; entries++)
		{
			if (entries >= nlines)
			{
				start++;
			}
		} /*for (entries = 0; historyList[entries+1]; entries++)*/
	}

	if (entries == 0)
	{
		needed = 1;
	}
	else
	{
		needed = 0;
		for (i = start; i < entries; i++)
		{
			needed += strlen(historyList[i]->line) + 1;
		}
	}
	
	/* fight memory leaks! */
	if (!getScratch(lines, GLINES, needed, char))
	{
		goto errorExit;
	}
	
	place = *lines;

	if (entries == 0)
	{
		nlines = 1;
		place[0] = '\0';
	} /*if (entries == 0)*/
	else
	{
		nlines = entries - start;
		for (i = start; i < entries; i++)
		{
			place = copyStrings(historyList[i]->line, place, 1);
		}
	} /*if (entries == 0){}else{}*/
	unTrash(GLINES);
	*historyHandle = lines;
	emptyTrash();

	return (nlines);

  errorExit:
	return (0);
} /*getSomeHistory()*/
	
void setSomeHistory(long nlines, char ** commands)
{
	long         i, history = HISTORY;
	char        *place, **placeH = &place;

	initHistory(-1); /* get rid of what we've got */
	initHistory(history);

	for (place = *commands; nlines > HISTORY; nlines--)
	{
		place = skipStrings(place, 1);
	} /*for (place = *commands; nlines > HISTORY; nlines--)*/
	
	for (i = 0; i < nlines; i++)
	{
		saveInHistory(placeH);
		place = skipStrings(place,1);
	}
} /*setSomeHistory()*/

#endif /*SAVEHISTORY*/


/* 
  Expand leading '~' in a filename.  A name of the form ~/path (or ~\path
  under MSDOS) is expanded to home/path or home\path, where home is the value
  of MacAnova CHARACTER variable HOME.  Under Unix, HOME is predefined to
  have value taken from environmental variable $HOME.  Under MSDOS, HOME is
  predefined to contain the name of the directory containing MACANOVA.EXE.
  Under Unix, a name of form ~username/path is expanded to homedir/path where
  getpwname() is used to get the home directory homedir for user username.
  Such a file name is not expanded under MSDOS and results in an error.
  On Macintosh, HOME is not predefined, but may be set in MacAnova.ini.  The
  usage ~username/path is not legal on a Mac

  950829 Fixed bug in DJGPP version
         Under MSDOS, any embedded '/' are changed to DIRSEPARATOR[0]
         (backslash))
*/

static char    FullPathName[PATHSIZE+1];

char *expandFilename(char *fname)
{
	Symbolhandle    home;
	char           *pc, c;
	char           *userdir = (char *) 0;
	long            dirNameLength;
#ifdef HASPWDH
	struct passwd  *passwd;
#endif /*HASPWDH*/
	WHERE("expandFilename");

	if (fname[0] == EXPANSIONPREFIX)
	{ /* fname starts with '~' */
		if (isSlash(fname[1]))
		{/* treat ~/... as $HOME/... */
			home = Lookup("HOME"); /* should be initialized at startup */
			if(home == (Symbolhandle) 0 || TYPE(home) != CHAR ||
			   !isScalar(home))
			{
				userdir = getenv("HOME");
				if (userdir == (char *) 0)
				{
					sprintf(OUTSTR,
							"ERROR: cannot expand %c%c;  no CHARACTER var. HOME or environment var. $HOME",
							fname[0], fname[1]);
					goto errorExit;
				}
			}
			else
			{
				userdir = STRINGPTR(home);
			}
			
			pc = fname+2;
		} /*if (isSlash(fname[1]))*/
		else
		{
#ifndef HASPWDH
			/* can't expand ~name on except on Unix */
			userdir = (char *) 0;
#else /*HASPWDH*/
			for(pc = fname+1;*pc && !isSlash(*pc);pc++)
			{
				;
			}
			c = *pc;
			*pc = '\0';
			passwd = getpwnam(fname+1);
			*pc = c;
			if(c != '\0')
			{
				pc++;
			}
				
			userdir = (passwd != (struct passwd *) 0) ?
				passwd->pw_dir :  (char *) 0;

#endif /*HASPWDH*/
		} /*if (isSlash(fname[1])){}else{}*/
		
		if(userdir == (char *) 0)
		{
			putPieces("ERROR: unable to expand ", fname,
					  (char *) 0, (char *) 0);
			goto errorExit;
		}
			
		dirNameLength = strlen(userdir);
/* allow for the possiblity that HOME ends in '/' */
		if (isSlash(userdir[dirNameLength-1]))
		{
			dirNameLength--;
		}
		
		if(dirNameLength + strlen(pc) + 1 >= PATHSIZE)
		{
			putPieces("ERROR: expanded file name ",userdir,pc," is too long");
			goto errorExit;
		}
		
		strcpy(FullPathName,userdir);
		FullPathName[dirNameLength++] = DIRSEPARATOR[0];
		FullPathName[dirNameLength] = '\0';
		strcat(FullPathName,pc);
	} /*if (fname[0] == EXPANSIONPREFIX)*/
	else
	{
		if(strlen(fname) >= PATHSIZE)
		{
			putPieces("ERROR: file name ", fname, " is too long", (char *) 0);
			goto errorExit;
		}
		
		strcpy(FullPathName, fname);
	} /*if (fname[0] == EXPANSIONPREFIX){}else{}*/
	
#ifdef MSDOS
	for (pc = FullPathName; *pc; pc++)
	{
		if (isSlash(*pc))
		{
			*pc = DIRSEPARATOR[0];
		}
	} /*for (pc = FullPathName; *pc; pc++)*/
#endif /*MSDOS*/

	return ((char *) FullPathName);
		
  errorExit:
	putOUTSTR();
	
	return ((char *) 0);

} /*expandFilename()*/
	
/*
  Check file names for legality on platform
*/

#define isDot(C) ((C) == '.')

#ifdef MSDOS

/*
  For MSDOS and Windows acceptable names are of the form
    pathElement/.../pathElement, or /pathElement/.../pathElement,
  possible prefixed by a drive designator of the form [a-zA-Z]:
  '/' may be replaced by '\'.

  For MSDOS pathElement is of form '.', '..', Name or Name.Ext, and
  Name has from 1 to 8 each characters and Ext has from 0 to three
  characters

  For Windows, when long file names are not permitted (Windows 3.1),
  acceptable names are the same as for MSDOS.
  When long file names are permitted (Windows 95 and Windows NT),
  a path element may contain up to 255 characters including any
  number of dots and the elements of a name (sequences of non-dots,
  non-slashes) preceding and/or following a dot can be of any length
  including 0.  Thus ..abc..d is a legal name.

  If path != 0, names ending in / or \ are acceptable.  Otherwise, they
  are not.
  
  In any case, if the total path length >= PATHSIZE, it won't fit in
  buffers and hence is rejected.
  
  970805 In Windows 95 and Windows NT long file names with multiple
  dots (a.b.c) are allowed
*/

#define MAXPATHCOMPONENT  255 /*limit on component of Windows 95 path*/

static int filenameFormOK(char *fname, int path)
{
	int        maxComponent, maxNamePart, maxNDots;
	char      *start = fname, *pc;
	int        namePartLength = 0, componentLength = 0, nDots = 0;
	int        longFileNames;

#ifdef wx_msw
	longFileNames = (ThisMachine != mvWin32s);
#else
	longFileNames = 0;
#endif /*wx_msw*/
	if (strlen(start) >= PATHSIZE)
	{
		/* length too long for buffers */
		return (0);
	}
	if (isalpha(start[0]) && start[1] == ':')
	{ /*skip volume id and ':'*/
		start += 2;
	}
	if (start[0] == '\0')
	{
		return (0);
	}

	maxComponent = (!longFileNames) ? 12 : MAXPATHCOMPONENT;
	maxNamePart = (!longFileNames) ? 8 : MAXPATHCOMPONENT;
	maxNDots = (!longFileNames) ? 1 : MAXPATHCOMPONENT;

	for(pc = start;*pc != '\0';pc++)
	{
		if(*pc == ':')
		{	/* have already skipped 'x:', ':' now illegal */
			return (0);
		}
		if(isDot(*pc))
		{
			if (nDots == 0 && namePartLength == 0)
			{
				if(isSlash(pc[1]))
				{/* pathElement is '.' */
					pc++;
				}
				else if(isDot(pc[1]) && isSlash(pc[2]))
				{/* pathElement is '..' */
					pc += 2;
				}
				else
				{
					if (!longFileNames)
					{ 	/* no Name; error */
						return (0);
					}
					nDots++;
				}
			} /*if (nDots == 0 && namePartLength == 0)*/
			else
			{/* Name completed */
				if (++nDots > maxNDots)
				{
					return (0);
				}
				namePartLength = 0;
				componentLength++;
				maxNamePart = (!longFileNames) ? 3 : maxNamePart;
			}
		} /*if(isDot(*pc))*/
 		else if(isSlash(*pc))
		{/* end of pathElement */
			componentLength = namePartLength = nDots = 0;
			maxNamePart = (!longFileNames) ? 8 : maxNamePart;
		}
		else
		{ /* name character */
			namePartLength++;
			componentLength++;
			if (namePartLength > maxNamePart ||
				componentLength > maxComponent)
			{
				return (0);
			}
		}
	} /*for(pc = start;*pc;pc++)*/
 
	return ( path || (--pc , !isSlash(*pc)));
} /* filenameFormOK()*/

#else /*MSDOS*/

/*
  On Unix accept non-empty names of the form
       pathElement or pathElement/pathElement or pathElement/.../pathElement
  where a pathElement does not contain '/' but can be empty/  Note that
  this includes paths with repeated '/' (e.g., "foo//bar") or
  of the form "../bar"
*/
static int filenameFormOK(char * fname, int path)
{
	/*
	   only current restriction is that it can't end in '/';
	   or start with '-'
	   repeated '/'s allowed
	*/
	return (fname[0] != '-' && (path || !isSlash(fname[strlen(fname)-1])));

} /* filenameFormOK()*/

#endif /*MSDOS*/

/*
   Check for illegal characters in fname
   All alphanumerics are legal plus the characters in okNonAlnum.
   Since fname has been checked for form before being checked
   here, special characters like '/' are included in okNonAlnum
*/
static int filenameCharsOK(char * fname)
{
#if defined(wx_msw)
	char      *okNonAlnum = (ThisMachine == mvWin32s) ?
		".\\/:_-`!@#$%^&(){}'" : ".\\/:_-`!@#$%^&(){}[]' ";
#elif defined(MSDOS)
	char      *okNonAlnum = ".\\/:_-`!@#$%^&(){}'";
#elif defined(VMS) /*MSDOS*/
	/* For VMS allow only : */
	char      *okNonAlnum = ".:[]<>_-+$";
#else /*MSDOS*/
	/*don't allow (){}[]`'";|\ even though that are technically legal */
	char      *okNonAlnum = "./_,-+!@#$%^&~";
#endif /*MSDOS*/
	char       c;
	
	while((c = *fname++) != '\0')
	{
		if (!isalnum(c) && !strchr(okNonAlnum, c))
		{
			return (0);
		}
	} /*while((c = *fname++) != '\0')*/
	return (1);
} /*filenameCharsOK()*/

/*
   silent check of appropriateness of file name
*/
long okfilename(char *fname)
{
	return (*fname != '\0' && filenameFormOK(fname, 0) &&
			filenameCharsOK(fname));
} /*okfilename()*/

/*
   silent check of appropriateness of path name
*/
long okpathname(char *fname)
{
	return (*fname != '\0' && filenameFormOK(fname, 1) &&
			filenameCharsOK(fname));
} /*okpathname()*/

/* Check filename with error messages */
long isfilename(char *fname)
{
	if (okfilename(fname))
	{
		return (1);
	}

	if(*fname == '\0')
	{
		putOutErrorMsg("ERROR: zero length file name");
	}
	else
	{
		putPieces("ERROR: improper file name ", fname, (char *) 0, (char *) 0);
	}
	return (0);
} /*isfilename()*/

#ifdef MSDOS

#ifdef BCPP
#include <dos.h>
#define ArgV _argv

void mypause(void)
{
	fprintf(stderr,"Hit any key to go on");
	while(!kbhit())
	{
		;
	}
} /*mypause()*/
#endif /* BCPP */

extern char      **ArgV; /*defined and set equal to argv in main()*/
static char        Path[PATHSIZE];
/* instead of string.h */
extern char *strrchr();

char *get_dataPath(void)
{
	char    *pathp;

	if( strlen( ArgV[0] ) > PATHSIZE )
	{
		putOutErrorMsg("ERROR: Path to MacAnova.exe too long");
		Path[0] = '\0';
		return(Path);
	}
	(void) strcpy(Path,ArgV[0]);
	if( (pathp = strrchr(Path,'\\')) == (char *) 0 )
	{
		pathp = strrchr(Path,'/');
	}
	if(pathp)
	{
		*(++pathp) = 0x00; 
	}
	else
	{
		Path[0] = '\0';
	}
	
	return(Path);
} /*get_dataPath()*/
#endif /*MSDOS*/
