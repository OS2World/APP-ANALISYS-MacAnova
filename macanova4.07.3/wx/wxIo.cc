/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 4.07 or later
*(C)*
*(C)* Copyright (c) 1999 by Gary Oehlert and Christopher Bingham
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

/*
  980119 added OverflowTextFrame()
  980328 added #include <math.h> before extern "C" {... #include "globals.h"}
         and removed undef and def of __cplusplus
  980401 moved #include <Xm/Text.h> to after #include "wx.h"
  980729 fixed wx_msw bug in fmyeol() which put two newlines on spool file
         for every one that was wanted.
  981002 fixed bug in get_dataPath that did not get the complete path
         when it containd a blank.  This affected only Windows version
  981028 Moved #include <pwd.h> to after #include "../globals.h"
  981110 Corrected Code for reading from console so it doesn't crash when
         cancel is clicked.
  990215 Replaced myerrorout() by putOutErrorMsg() and putOUTSTR() by
         putErrorOUTSTR()
*/

// For compilers that support precompilation, includes "wx.h".
#include "wx_prec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx.h"
#endif

#ifdef wx_motif
#include <Xm/Text.h>
#endif  // wx_motif

#include <math.h>

//980403 moved the following before include wxIface.h and wxmain.h
extern "C" {
#include "../globals.h"
#include "../matdat.h"
/*
  981028 moved the include <pwd.h> from outside the the extern to after
         globals.h
*/
#ifdef HASPWDH
#include <pwd.h>
#endif /*HASPWDH*/
} /*extern "C"*/

#include "wxIface.h"
#include "wxmain.h"

/*
  Functions in this file
  Various input/output related utilities
	void lineToWindow(char *msg, long msgLength)
	void fmyeol(FILE *fp)
	void putprompt(char *prompt)
	void myAlert(char *msgs)
	int mygetosversion(void)
	void fmyprint(char *msg,FILE *fp)

  Functions having to do with maintaining the command history
	static void   shiftSavedLines(void)
	static char ** getHistoryLine(int op, int * historyError)
	void initHistory(long historyLength)
	void saveInHistory(char ** lineH)
	static char **getCurrentCommand(void)
	char * recallHistory(int id)
	long getSomeHistory(long nlines, char *** historyHandle)
	void setSomeHistory(long nlines, char ** commands)

  Functions having to do with obtaining and checking file names
	char *expandFilename(char *fname)
	static int filenameFormOK(char *fname, int path)
	static int filenameCharsOK(char * fname)
	long okfilename(char *fname)
	long isfilename(char *fname)

  More output utilities
	void fmyflush(FILE *fp)
	void mybeep(long n)

  MacAnovaBaseFrame member functions and others
	void MacAnovaBaseFrame::OverflowTextFrame(int outOfMemory, long cutLength)
	void MacAnovaBaseFrame::lineToWindow(char * msg,  long msgLength)
	void MacAnovaBaseFrame::fmyeol(FILE * fp)
	int fillFromDialog(void)
	void MacAnovaBaseFrame::PutPrompt(char *prompt)
	void MacAnovaBaseFrame::PutPrompt(char *prompt, long frameIndex)

  More input/output utilities
	char *get_dataPath(void)
	void incrementNLINES(char *msg)
	FILE * fmyopen(char * fileName, char * mode)
	char * wxFindFile(char *fileName, char *message, char *defaultname)
*/
extern "C" {

void lineToWindow(char *msg, long msgLength)
{
	BaseFrame->lineToWindow(msg,msgLength);
} /*lineToWindow()*/

void fmyeol(FILE *fp)
{
	BaseFrame->fmyeol(fp);
} /*fmyeol()*/

void putprompt(char *prompt)
{
	BaseFrame->PutPrompt(prompt);
}

void myAlert(char *msgs)
{
	(void) wxMessageBox(msgs,"MacAnova Alert");
}

int mygetosversion(void)
{
	return (::wxGetOsVersion());
} /*mygetosversion*/

#define BEL '\007'
extern char       *TooManyErrors;

#define OUTLINESIZE   400  /*size of output line buffer*/

static char       wxOutLineBuf[OUTLINESIZE];
       char      *OutLineBuf = wxOutLineBuf; /* buffer for building output lines */
static long       OutLinePlace = 0;


/*
	If fp != STDOUT, put msg to file fp; otherwise add msg to OutLineBuf and
	flush *OutLineBuf if last character is CR (Mac end of line)
*/


void fmyprint(char *msg,FILE *fp)
{
	long            i, msgLength;
	char           *outline, c;
	WHERE("fmyprint");

	if(fp == STDOUT)
	{		/* collect line in OutLineBuf */
		msgLength = strlen(msg);
		if (OutLinePlace + msgLength > OUTLINESIZE)
		{
			lineToWindow(OutLineBuf, OutLinePlace);
			if(msgLength > OUTLINESIZE)
			{
				lineToWindow(msg, msgLength);
				msgLength = 0;
			}
		}
		outline = OutLineBuf + OutLinePlace;
		for (i = 0; i < msgLength; i++)
		{
			c = *msg++;
#ifndef wx_msw
			*outline++ = (c == CR) ? NL : c;
#else /*wx_msw*/
			if (c == NL)
			{
				*outline++ = CR;
			}
			*outline++ = c;
#endif /*wx_msw*/
		} /*for (i = 0; i < msgLength; i++)*/
		*outline = '\0';
		OutLinePlace += msgLength;
		if (OutLinePlace > 0 && OutLineBuf[OutLinePlace-1] == NL)
		{
			lineToWindow(OutLineBuf, OutLinePlace);
		}
	} /*if(fp == STDOUT)*/
	else
	{
		fputs(msg, fp);
	} /*if(fp == STDOUT){}else{}*/
} /*fmyprint()*/

#ifdef SAVEHISTORY

/* 970112 code to handle history*/

extern "C" {
enum historyCodes
{
	NOHISTORY = 0,
	UPHISTORY,
	DOWNHISTORY,
	TOPHISTORY,
	BOTTOMHISTORY
};

enum historyErrors
{
	NOHISTORYERROR = 0,
	NOHISTORYTOGET,
	OFFHISTORYBOTTOM,
	OFFHISTORYTOP,
	HISTORYALLOCATIONERROR
};

#if (0) /* for debugging */
void displayLine(char * line)
{
	char        *outstr = OUTSTR;

	while (*line)
	{
		sprintf(outstr,"%02x", (int) *line++);
		outstr += strlen(outstr);
	}
	myAlert(OUTSTR);
	*OUTSTR = '\0';
} /*displayLine()*/
#endif /*0*/

/* handle for vector of char handles for history of previous commands*/
static char      ****SavedLines = (char ****) 0;
/*
   Handle to contents of current command, created when accessing previous
   commands
*/
static char        **CurrentCommand = (char **) 0;
/* the current number of lines that are currently saved <= HISTORY */
static long          NSavedLines = 0;
/*
  index in history list of line currently being displayed if it came
  from the history list.  -1 means not a previous command
*/
static long          CurrentLine = -1;

/*
   Shift all saved lines up 1 in preparation for adding new line to list
   Dispose of the earliest one if limit has been reached
*/
static void   shiftSavedLines(void)
{
	short       i;
	char      **temp;
	
	if (NSavedLines > 0)
	{
		temp = (*SavedLines)[NSavedLines-1];
		for (i = NSavedLines - 1; i > 0; i--)
		{
			(*SavedLines)[i] = (*SavedLines)[i - 1];
		}
		(*SavedLines)[0] = (char **) 0;
		if (NSavedLines < HISTORY)
		{
			(*SavedLines)[NSavedLines] = temp;
		} /*if (NSavedLines < HISTORY)*/
		else
		{
			mydisphandle(temp);
		}
		CurrentLine = -1;
	} /*if (NSavedLines > 0)*/
} /*shiftSavedLines()*/

/*
  Retrieve a line from history
  Values of op
    UPHISTORY     get next earlier command than currently displayed
    DOWNHISTORY   get next later command than currently displayed
    TOPHISTORY    get earliest command saved
    BOTTOMHISTORY redisplay command that had been typed before recalling
                  earlier commands
*/
static char ** getHistoryLine(int op, int * historyError)
{
	char     **temp = (char **) 0;
	long       current = CurrentLine;
	int        incr = 0;
	WHERE("getHistoryLine");

	if (SavedLines != (char ****) 0)
	{
		if (op == UPHISTORY)
		{
			current++;
		}
		else if (op == DOWNHISTORY)
		{
			current--;
		}
		else if (op == TOPHISTORY)
		{
			current = NSavedLines - 1;
		}
		else if (op == BOTTOMHISTORY)
		{
			current = -1;
		}
		*historyError = NOHISTORYERROR;
		if (current >= 0 && current < NSavedLines)
		{
			temp = (*SavedLines)[current];
			CurrentLine = current;
		}
		else if (current < 0)
		{
			if (current < -1)
			{
				*historyError = OFFHISTORYBOTTOM;
			}
			temp = CurrentCommand;
			CurrentLine = -1;
		}
		else
		{
			*historyError = OFFHISTORYTOP;
			if (NSavedLines > 0)
			{
				temp = (*SavedLines)[NSavedLines-1];
				CurrentLine = NSavedLines - 1;
			}
			else
			{
				temp = CurrentCommand;
				CurrentLine = -1;
			}
		}
	} /*if (SavedLines != (char ****) 0)*/
	else
	{
		*historyError = NOHISTORYTOGET;
		temp = CurrentCommand;
		CurrentLine = -1;
	}
	return (temp);
} /*getHistoryLine()*/

/*
   Initialize or change the length of a vector of handles to hold recent
   history

   If there are memory allocation problems, mygethandle() or
   mygrowhandle() will print an error message, and history will be turned
   off because SavedLines will be NULL

   971023 Fixed apparent memory leak when history length is reduced
   971210 Negative historyLength disposes of lines but not SavedLines
*/

void initHistory(long historyLength)
{
	long           i;
	WHERE("initHistory");

	if (SavedLines != (char ****) 0 && NSavedLines > historyLength)
	{
		long        start = (historyLength >= 0) ? historyLength : 0;
		
		/* dispose of extras, if any */
		for (i = start; i < NSavedLines; i++)
		{
			mydisphandle((*SavedLines)[i]);
			(*SavedLines)[i] = (char **) 0;
		}
		NSavedLines = start;
	} /*if (SavedLines != (char ****) 0)*/
	
	if (historyLength < 0)
	{
		historyLength = HISTORY;
	}
	else if (historyLength > 0)
	{
		if (SavedLines == (char ****) 0)
		{
			SavedLines = (char ****) mygetsafehandle((historyLength) * sizeof(char **));
			if (SavedLines == (char ****) 0)
			{
				historyLength = 0;
			}
			else
			{
				for (i = 0; i < historyLength; i++)
				{
					(*SavedLines)[i] = (char **) 0;
				}
			}
			NSavedLines = 0;
		}
		else if (historyLength != HISTORY)
		{
			char       ****savedLines =
			  (char ****) mygrowsafehandle((char **) SavedLines,
										   historyLength * sizeof(char **));

			if (savedLines == (char ****) 0)
			{
				/* don't changed HISTORY */
				historyLength = HISTORY;
			}
			else
			{
				SavedLines = savedLines;
				/* clear new slots, if any */
				for (i = NSavedLines; i < historyLength; i++)
				{
					(*SavedLines)[i] = (char **) 0;
				}
				if (historyLength < NSavedLines)
				{
					NSavedLines = historyLength;
				}
			}
		}
	} /*if (historyLength > 0)*/
	else if (SavedLines != (char ****) 0)
	{
		mydisphandle((char **) SavedLines);
		SavedLines = (char ****) 0;
		NSavedLines = 0;
	}

	CurrentLine = -1;
	mydisphandle(CurrentCommand);
	CurrentCommand = (char **) 0;
	HISTORY = historyLength;
} /*initHistory()*/

/*
   Save *lineH in history list.

   All previous lines are shifted up 1, discarding the last if the
   limit has been reached.  The line being saved is put in slot 0
*/

void saveInHistory(char ** lineH)
{
	long        i;
	WHERE("saveInHistory");
	
	mydisphandle(CurrentCommand);
	CurrentCommand = (char **) 0;
	CurrentLine = -1;
	if (SavedLines != (char ****) 0 && (*lineH)[0] != '\0')
	{
		long        nDoubles = strlen(*lineH)/sizeof(double) + 1;
		char      **line = mygethandle(nDoubles*sizeof(double));
		/*
			allocate a multiple of sizeof(double) to make it more likely
			to use a handle in one of the caches
		*/

		if (line == (char **) 0)
		{
			goto errorExit;
			
		}
		shiftSavedLines();
		(*SavedLines)[0] = line;
		strcpy(*line, *lineH);
		if (NSavedLines < HISTORY)
		{
			NSavedLines++;
		}
	} /*if (SavedLines != (char ****) 0 && (*lineH)[0] != '\0')*/
	return;

  errorExit:
/* don't know anything better to do than clear history list */
	myAlert("History of past commands cleared");
	for (i = 0;i < NSavedLines;i++)
	{
		mydisphandle((*SavedLines)[i]);
		(*SavedLines)[i] = (char **) 0;
	} /*for(i=0;i<NSavedLines;i++)*/
	mydisphandle(CurrentCommand);
	CurrentCommand = (char **) 0;
	NSavedLines = 0;
	CurrentLine = -1;

} /*saveInHistory()*/


/*
  Create a handle containing everything from the start of the command
  line to the end of command window
*/
static char **getCurrentCommand(void)
{
	char               **lineH = (char **) 0, *line;
	MacAnovaTextWindow  *mtw = BaseFrame->RunningWindow;
	
	line = mtw->GetSelection(mtw->CmdStrPos, mtw->GetLastPosition());
	if (line != (char *) 0)
	{
		int     nDoubles;

#ifdef wx_msw
		stripCR(line);
#endif /*wx_msw*/
		nDoubles = strlen(line)/sizeof(double) + 1;

		lineH = mygethandle(nDoubles * sizeof(double));
		if (lineH != (char **) 0)
		{
			strcpy(*lineH, line);
		}
		delete [] line;
	} /*if (line != (char *) 0)*/
	return (lineH);
} /*getCurrentCommand()*/

/*
	970112 Handle Up History and Down History commands on Edit menu
	971210 Added getSomeHistory() and setSomeHistory()
*/

char * recallHistory(int id)
{
	char        **lineH;
	char         *line = (char *) 0;
	int           historyOp, historyError;
	WHERE("recallHistory");

	if (!Running && BDEPTH <= 0)
	{
		if (CurrentLine < 0)
		{
			mydisphandle(CurrentCommand);
			CurrentCommand = getCurrentCommand();
		} /*if (CurrentLine < 0)*/

		if (id == MACANOVA_EDIT_UPHISTORY)
		{
			historyOp = UPHISTORY;
		}
		else
		{
			historyOp = DOWNHISTORY;
		}
		lineH = getHistoryLine(historyOp, &historyError);
		line = (lineH != (char **) 0) ? *lineH : NullString;

		if (historyError != NOHISTORYERROR)
		{
			mybeep(1);
		}
	} /*if (!Running && BDEPTH <= 0)*/
	return (line);
} /*recallHistory()*/

enum historyScratch
{
	GLINES = 0,
	NTRASH
};

long getSomeHistory(long nlines, char *** historyHandle)
{
	char         **lines;
	long           i, needed;
	char          *place;
	TRASH(NTRASH, errorExit);
	
	if (NSavedLines <= 0)
	{
		nlines = 0;
	}
	else if (nlines == 0 || nlines >= NSavedLines)
	{
		nlines = NSavedLines - 1;
	}
	
	if (nlines == 0)
	{
		needed = 1;
	}
	else
	{
		needed = 0;
		for (i = 1; i <= nlines; i++)
		{
			needed += strlen(*(*SavedLines)[i]) + 1;
		}
	}
	
	if (!getScratch(lines, GLINES, needed, char))
	{
		goto errorExit;
	}
	
	place = *lines;

	if (nlines == 0)
	{
		nlines = 1;
		place[0] = '\0';
	}
	else
	{
		for (i = nlines; i > 0; i--)
		{
			place = copyStrings(*(*SavedLines)[i], place, 1);
		}
	}
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
	
	for (i = 0, place = *commands; i < nlines; i++)
	{
		saveInHistory(placeH);
		place = skipStrings(place,1);
	}
} /*setSomeHistory()*/
} /*extern "C" */

#endif /*SAVEHISTORY*/



#ifdef MSDOS
/*  '/' equivalent to '\' on MSDOS*/
#define isSlash(C) ((C) == DIRSEPARATOR[0] || (C) == '/')
#else /*MSDOS*/
#define isSlash(C) ((C) == DIRSEPARATOR[0])
#endif /*MSDOS*/

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

static char    FullPathName[PATHSIZE];

char *expandFilename(char *fname)
{
	Symbolhandle    home;
	char           *pc, c;
	char           *userdir = (char *) 0;
	long            dirNameLength;
#ifdef HASPWDH
	struct passwd  *passwd;
#endif /*HASPWDH*/
		
	if (fname[0] == EXPANSIONPREFIX)
	{
		if (isSlash(fname[1]))
		{						/* treat ~/... as $HOME/... */
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
#else  /*HASPWDH*/
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
	putErrorOUTSTR();
	
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

  970805 In Windows 95 and Windows NT long file names with multiple
  dots (a.b.c) are allowed
*/
#ifdef wx_msw
#define MAXPATHCOMPONENT  255 /*limit on component of Windows 95 path*/
#endif /*wx_msw*/

static int filenameFormOK(char *fname, int path)
{
	int        maxComponent, maxNamePart, maxNDots;
	char      *start = fname, *pc;
	int        namePartLength = 0, componentLength = 0, nDots = 0;
#ifdef wx_msw
	int        longFileNames =
    	(ThisMachine == mvWindows95 || ThisMachine == mvWindowsNT);
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

#ifdef wx_msw
	maxComponent = (!longFileNames) ? 12 : MAXPATHCOMPONENT;
	maxNamePart = (!longFileNames) ? 8 : MAXPATHCOMPONENT;
	maxNDots = (!longFileNames) ? 1 : MAXPATHCOMPONENT/2;
#else /*wx_msw*/
	maxComponent = 12;
	maxNamePart = 8;
	maxNDots = 1;
#endif /*wx_msw*/

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
#ifdef wx_msw
					if (!longFileNames)
					{ 	/* no Name; error */
						return (0);
					}
					nDots++;
#else /*wx_msw*/
					/* no Name; error */
					return (0);
#endif /*wx_msw*/
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
#ifdef wx_msw
				maxNamePart = (!longFileNames) ? 3 : maxNamePart;
#else /*wx_msw*/
				maxNamePart = 3;
#endif /*wx_msw*/
			}
		} /*if(isDot(*pc))*/
 		else if(isSlash(*pc))
		{/* end of pathElement */
			componentLength = namePartLength = nDots = 0;
#ifdef wx_msw
			maxNamePart = (!longFileNames) ? 8 : maxNamePart;
#else /*wx_msw*/
			maxNamePart = 8;
#endif /*wx_msw*/
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
	/*
	  Only allow standard DOS filename characters under Windows
	  under Windows 95 and NT allow also '[', /]' and ' '
	*/
	char      *okNonAlnum = (ThisMachine == mvWin32s) ?
		".\\/:_-`!@#$%^&(){}'" : ".\\/:_-`!@#$%^&(){}[]' ";
#elif defined(MSDOS)
	char      *okNonAlnum = ".\\/:_-`!@#$%^&(){}'";
#elif defined(VMS) /*MSDOS*/
	/* For VMS allow only : */
	char      *okNonAlnum = ".:[]<>_-+$";
#else  /*MSDOS*/
	/*don't allow (){}[]`'";|\~ even though that are technically legal */
	char      *okNonAlnum = "./_,-+!@#$%^&";
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

void fmyflush(FILE *fp)
{
	if(fp != STDOUT)
	{
		fflush(fp);
	}
	else
	{
		lineToWindow(OutLineBuf, OutLinePlace);

		if(SPOOLFILE != (FILE *) 0)
		{
			fflush(SPOOLFILE);
		}
	}
} /*fmyflush()*/

void mybeep(long n)
{
	if(BDEPTH == 0 && ISATTY)
	{
		while(n-- > 0)
		{
			wxBell();
		}
	}
} /*mybeep()*/


} /* extern "C" */

void MacAnovaBaseFrame::OverflowTextFrame(int outOfMemory, long cutLength)
{
	char            wTitle[PATHSIZE+1];
	char            outstr[400];
	char           *dashes = "----------------------------------";

#if (0)
	ALERT("XX Length + msgLength = %d, CMDWINDOWLIMIT = %d",
					Length+msgLength,CMDWINDOWLIMIT,0,0);
#endif /*0*/
	myAlert("Output window contents are too long; saving window");

	TextFrames[RunningFrame]->SaveWindowAs();

	if(numTextFrames < MAXWINDOWS)
	{				/* we have room for a new window */
		if(!outOfMemory)
		{
			RunningWindow->SetInsertionPointEnd();
			RunningWindow->WriteText(EOL_STRING);
			RunningWindow->SetInsertionPointEnd();
			RunningWindow->WriteText(dashes);
			RunningWindow->SetInsertionPointEnd();
			RunningWindow->WriteText(EOL_STRING);
			RunningWindow->SetInsertionPointEnd();
			RunningWindow->WriteText("Continued in new window");
			RunningWindow->SetInsertionPointEnd();
			RunningWindow->WriteText(EOL_STRING);
		}
		RunningWindow->CmdStrPos = RunningWindow->GetLastPosition();
		strncpy(wTitle,TextFrames[RunningFrame]->GetTitle(),PATHSIZE);
		wTitle[PATHSIZE] = '\0';

		RunningWindow->clearUndoInfo();

		NewTextFrame(TRUE,FALSE); /* create new window without prompt*/

		RunningFrame = numTextFrames-1;
		RunningWindow = TextFrames[RunningFrame]->text_window;

		sprintf(outstr,"Output continued from %s",wTitle);

		RunningWindow->WriteText(outstr);
		RunningWindow->WriteText(EOL_STRING);
		RunningWindow->WriteText(dashes);
 		RunningWindow->WriteText(EOL_STRING);
	} /*if(numTextFrames < MAXWINDOWS)*/
	else
	{				/* no room, trim off top */
		RunningWindow->CmdInsertLine = 1;
		RunningWindow->SetSelection(0L, 0L);
		RunningWindow->ShowPosition(0);

		sprintf(outstr,
			"Cannot open a new window%s%ld characters will be trimmed off the top",
			EOL_STRING,cutLength);
		myAlert(outstr);

		RunningWindow->SetSelection(0L, cutLength);
		RunningWindow->Cut();
		if(RunningWindow->UndoStatus == OUTPUTUNDO)
		{
			RunningWindow->LastLength -= cutLength;
			RunningWindow->LastCmdStrPos -= cutLength;
			if(RunningWindow->LastCmdStrPos < 0)
			{
				RunningWindow->clearUndoInfo();
			}
		}			/*if(UndoStatus == OUTPUTUNDO)*/
	} /*if(numTextFrames < MAXWINDOWS){}else{}*/

} /* MacAnovaBaseFrame::OverflowTextFrame() */


/*
  Write line of output to file fp which may specify command window
  If writing to console, interpret leading '\007's as BEL's
  If spooling is active, write line to spool file
  If line starts with "ERROR:", ring bell and count error
	970305 NLINES is now incremented here
*/

void MacAnovaBaseFrame::lineToWindow(char * msg,  long msgLength)
{
	long            nLines, cutLength;
	long            isOutLine = (msg == OutLineBuf);
	char            outstr[400];
	long            Length;
	WHERE("lineToWindow");

	Length = RunningWindow->GetLastPosition();

#ifdef wx_msw //980218
	if (msgLength == 1 && msg[0] == '\n')
	{
		msgLength++;
	}
#endif /*wx_msw*/

	while(*msg == BEL)
	{							/* turn leading BEL's into beeps */
		mybeep(1);
		msg++;
		msgLength--;
	}

	if (msgLength > 0)
	{
		if (SPOOLFILE != (FILE *) 0)
		{
			fputs(msg, SPOOLFILE);
		}

		if(UseWindows > 0)
		{
			if (RunningWindow->UndoStatus != CANTUNDO &&
				RunningWindow->UndoStatus != OUTPUTUNDO)
			{
				RunningWindow->clearUndoInfo();
			}

			if (msgLength > CMDWINDOWLIMIT)
			{
				sprintf(outstr,
						"WARNING: output line longer than %ld characters is truncated",
						(long) CMDWINDOWLIMIT);
				myAlert(outstr);
				msgLength = CMDWINDOWNEWSIZE;
			} /*if (msgLength > CMDWINDOWLIMIT)*/
			Length = RunningWindow->GetLastPosition();

			if (Length + msgLength > CMDWINDOWLIMIT)
			{
				OverflowTextFrame(0,msgLength + Length - CMDWINDOWNEWSIZE);
				Length = RunningWindow->GetLastPosition();
			} /*if (Length + msgLength > CMDWINDOWLIMIT)*/

			//RunningWindow->SetInsertionPoint(Length);
#ifdef wx_msw
			RunningWindow->Synch();
#if (0)
			HWND hWnd = RunningWindow->GetHWND();
			SendMessage(hWnd, EM_SETSEL, (WPARAM) Length, (LPARAM) Length);
			SendMessage(hWnd, EM_REPLACESEL, 0, (LPARAM) msg);
#else /*0*/
			if (TextFrames[RunningFrame]->JustCreated)
			{
				TextFrames[RunningFrame]->JustCreated = FALSE;
#define KLUDGELENGTH 29900
				// nothing yet in the window
				long        kludgeSize = KLUDGELENGTH - Length;
				char       *kludgeBuffer = new char[kludgeSize];
				int         i;

				for (i = 0; i < msgLength; i++)
				{
					kludgeBuffer[i] = msg[i];
				}
				for(;i < kludgeSize;i++)
				{
					kludgeBuffer[i] = ' ';
				}
				kludgeBuffer[kludgeSize-1] = '\0';
				RunningWindow->WriteText(kludgeBuffer);
				delete [] kludgeBuffer;
				RunningWindow->Remove(Length + msgLength+1,RunningWindow->GetLastPosition());
#if (0)
				ALERT("Length = %ld, msgLength = %ld, GetLastPosition() = %ld",
					  Length, msgLength,RunningWindow->GetLastPosition(),0);
#endif /*0*/
			}
			else
			{
				RunningWindow->WriteText(msg);
			}
#endif /*(0)*/
			long newLength = RunningWindow->GetLastPosition();
			if (newLength < Length + msgLength)
			{
		/* Entire message did not go to window; overflow and write again*/
#if (0)
				RunningWindow->CheckForError();
#endif /*0*/
				if(CMDWINDOWNEWSIZE < Length)
				{
					cutLength =  msgLength + Length - CMDWINDOWNEWSIZE;
				}
				else
				{
					cutLength = ((2*msgLength < 1000) ? 1000 : 2*msgLength);
				}
				OverflowTextFrame(1, cutLength);
                RunningWindow->WriteText(msg);
			} /*if (newLength < Length + msgLength)*/

#else //wx_msw
			//RunningWindow->Replace(Length,Length,msg);
			Widget textWidget = (Widget) RunningWindow->handle;
			XmTextInsert(textWidget,Length,msg);

#endif // wx_msw
			nLines = RunningWindow->GetNumberOfLines();
			RunningWindow->CmdInsertLine = nLines;
			RunningWindow->CmdStrPos = RunningWindow->GetLastPosition();
			incrementNLINES(msg);
		} /*if(UseWindows > 0)*/

		if(strncmp("ERROR:",msg, (int) 6) == 0)
		{
			if(MAXERRORS1)
			{
				NERRORS++;
			}

			mybeep(1);
		} /*if(strncmp("ERROR:",msg, (int) 6) == 0)*/
	} /*if (msgLength > 0)*/

	if (isOutLine)
	{
		OutLinePlace = 0;
		OutLineBuf[0] = '\0';
	}
} /*MacAnovaBaseFrame::lineToWindow()*/

/*
   terminate output line and pause if writing the console and screen is full
   (no pausing on Mac)
   */

void MacAnovaBaseFrame::fmyeol(FILE * fp)
{
	long            Length,nLines;
	char           *Newline = "\n";
    WHERE("MacAnovaBaseFrame::fmyeol");

	if(fp == STDOUT)
	{
		incrementNLINES(Newline);
		if (OutLinePlace > 0)
		{						/* flush OutLineBuf */
			lineToWindow(OutLineBuf, OutLinePlace);
		}
		if(UseWindows > 0)
		{
			if(RunningWindow->UndoStatus != CANTUNDO &&
			   RunningWindow->UndoStatus != OUTPUTUNDO)
			{
				RunningWindow->clearUndoInfo();
			}
#if (0)
			Length = RunningWindow->GetLastPosition();
			RunningWindow->SetInsertionPoint(Length);
#endif /*0*/
#ifdef wx_msw
#if (0)
			RunningWindow->WriteText(EOL_STRING);
#else /*0*/
 			lineToWindow(EOL_STRING,strlen(EOL_STRING));
#endif /*0*/

#else //wx_msw
			Length = RunningWindow->GetLastPosition();
			Widget textWidget = (Widget) RunningWindow->handle;
			XmTextInsert (textWidget, Length, EOL_STRING);
			RunningWindow->ShowPosition(Length); //XmTextScroll(textWidget,1);
#if (0)
			RunningWindow->WriteText(EOL_STRING);
			RunningWindow->Replace(Length,Length,EOL_STRING);
#endif /*0*/
#endif		// wx_msw
			nLines = RunningWindow->GetNumberOfLines();
			RunningWindow->CmdInsertLine = nLines;
			RunningWindow->CmdStrPos = RunningWindow->GetLastPosition();
		}						/*if(UseWindows > 0)*/
#ifndef wx_msw //980729 call to lineToWindow() above spools the new line
		if (SPOOLFILE != (FILE *) 0)
		{
			putc(Newline[0], SPOOLFILE);
		}
#endif /*wx_msw*/

		wxYield();

		if (INTERRUPT != INTNOTSET)
		{
			INTERRUPT = PRINTABORT;
		}
	}						/*if(fp == STDOUT)*/
	else
	{
		putc(Newline[0], fp);
	}
} /*MacAnovaBaseFrame::fmyeol()*/

/*
  Fill line from dialog window (CONSOLE).  This code was formerly part of
  fillLINE() which was moved to commonio.c along with supporting routines
  datagetc and fileToLine()

  981110 Fixed code so that it recognizes when the uses clicks cancel
         This now signals the end of the read.  We never see anything
         the user had already put in the dialog box so we can't use it;
         this contrasts with the Done button in the Macintosh version
*/

static Bool           HitDone = FALSE;
static unsigned char  LineBuffer[MAXLINE+1];
static unsigned char *Place = (unsigned char *) LineBuffer;

extern "C" {
	
int fillFromDialog(void)
{
	long               length;
	unsigned char     *pc;
	long               Length;
	char              *reply;
	WHERE("fillFromDialog");

	/*
	  Use dialog window or what was read previously
	  Always terminate (*LINE) with '\n'
	  Return only through first '\n'.
	  */
	if((*LINE)[0] == '\0')
	{					/* first time in this trip */
		Place = LineBuffer;
		HitDone = FALSE;
	}

	(*LINE)[0] = '\0';
	if(Place == LineBuffer)
	{
		if(HitDone)
		{
			HitDone = FALSE;
			return (EOF);
		}
		/* need to read from Dialog box */
		reply = wxGetTextFromUser("Enter Line","Console Input");

		HitDone = (reply == (char *) 0);

        if (HitDone)
        {
        	Length = 0;
        }
        else
        {
			Length = strlen(reply);
			if(Length > MAXLINE)
			{
				sprintf(OUTSTR,
						"WARNING: more than %ld characters entered; last %ld ignored",
						(long) MAXLINE, Length - MAXLINE);
				myAlert(OUTSTR);
				Length = MAXLINE;
			}
			strncpy((char *) LineBuffer,reply,Length);
        }
        LineBuffer[Length] = '\0';
		strcpy(*LINE, (char *) LineBuffer);

		if(LineBuffer[0] == '\0' && HitDone)
		{
			HitDone = FALSE;
			return (EOF);
		}
	} /*if(Place == LineBuffer)*/
	else
	{
		strcpy(*LINE, (char *) Place);
	}

	pc = (unsigned char *) strchr((char *) Place, '\n');
	if(pc != (unsigned char *) 0 && pc[1] != '\0')
	{/* still something not seen */
		pc++;
		length = pc - Place;
		Place = pc;
	} /*if(pc != (unsigned char *) 0)*/
	else
	{					/* this is the last of it */
		length = strlen((char *) Place);
		if(pc == (unsigned char *) 0)
		{
			(*LINE)[length++] = '\n';
		}
		Place = LineBuffer;
		HitDone = FALSE;
	}						/*if(Place == LineBuffer){}else{}*/
	(*LINE)[length] = '\0';
	return (HitDone ? EOF : 1);
} /*fillFromDialog()*/

} /* extern "C" */

void MacAnovaBaseFrame::PutPrompt(char *prompt)
{
	PutPrompt(prompt, RunningFrame);
}

void MacAnovaBaseFrame::PutPrompt(char *prompt, long frameIndex)
{
	TextFrames[frameIndex]->text_window->PutPrompt(prompt);
}

#ifdef wx_msw
extern "C" {
extern char      **ArgV; /*defined and set equal to argv in main()*/
static char        Path[PATHSIZE+1];
/* instead of string.h */
/*
  981002 Fixed bug so that it correctly handles path names containing
         spaces
*/
char *get_dataPath(void)
{
	char    *pathp;
	int      quoted = 0, i;
	char     c;

	pathp = GetCommandLine();

	if (pathp[0] == '"')
	{
		pathp++;
		quoted = 1;
	}
	for (i = 0; (c = pathp[i]) != '\0' && i < PATHSIZE ; i++)
	{
		if (quoted && c == '"' || !quoted && c == ' ')
		{
			break;
		}
		Path[i] = c;
	} /*for (i = 0; (c = pathp[i]) != '\0' && i < PATHSIZE ; i++)*/
	Path[i] = '\0';
	if (i == PATHSIZE)
	{
		putOutErrorMsg("ERROR: Path to MacAnova.exe too long");
		Path[0] = '\0';
		return(Path);
	}

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

}						/* extern "C" */
#endif /* wx_msw */

static char FileNameFromFileSelector[WXMAXPATHLENGTH];

extern "C" {

/*
  Increment global NLINES for every newline in msg
*/

void incrementNLINES(char *msg)
{
	WHERE("incrementNLINES");
	
	while (*msg != '\0')
	{
		if (*msg++ == '\n')
		{
			NLINES++;
		}
	}
} /*incrementNLINES()*/

/*
   Open file in appropriate mode; may do things differently on different
   platforms;
   */

#ifdef WXWINMSW
/*  '/' equivalent to '\' on MSDOS*/
#define isSlash(C) ((C) == DIRSEPARATOR[0] || (C) == '/')
#define isPath(N) (strchr(N, '/') || strchr(N, '\\') || strchr(N, ':'))
#else							/*WXWINMSW*/
#define isSlash(C) ((C) == DIRSEPARATOR[0])
#define isPath(N) strchr(N, DIRSEPARATOR[0])
#endif							/*WXWINMSW*/


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
	981004 fixed bug when fileName[0] == '\0' and defaultName != 0
    and defaultName did not already exist so wxPathOnly returned NULL
*/
char * wxFindFile(char *fileName, char *message, char *defaultname)
{
	char         tempPath[WXMAXPATHLENGTH], tempName[WXMAXPATHLENGTH];
	char        *argPath = (char *) 0, *argName = (char *) 0;
	WHERE("wxFindFile");

	if (fileName[0] == '\0')
	{
		if (defaultname && defaultname[0] != '\0')
		{
            argPath = wxPathOnly(defaultname);
            if (argPath != (char *) 0)
            {
 				strncpy(tempPath,wxPathOnly(defaultname),WXMAXPATHLENGTH-1);
            }
            else
            {
            	tempPath[0] = '\0';
            }
			if (tempPath[0])
			{
				argPath = tempPath;
			}
 			strncpy(tempName,wxFileNameFromPath(defaultname),
					WXMAXPATHLENGTH-1);
			if (tempName[0] != '\0')
			{
				argName = tempName;
			}
		} /*if (defaultname && defaultname[0] != '\0')*/
		argPath = wxFileSelector(message, argPath, argName, NULL, "*");

 		if(argPath == (char *) 0)
		{
			return(0);
		}
		strncpy(FileNameFromFileSelector, argPath, WXMAXPATHLENGTH-1);
		fileName = FileNameFromFileSelector;
	} /*if (fileName[0] == '\0')*/
	return(fileName);
} /*wxFindFile()*/


} /*extern "C" */
