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

#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment Shell
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

/*
  970127 made changes to implement keep:T on shell()
         Unable to get either Borland or djgpp versions of popen()
         pclose() to work in wx_msw version.  Borland versions might work
         with with Windows NT so code is compiled for wx_msw.
         Someday I'll try the Win32 way

  970131 added interact:T which gives the previous behavior, executing
         the child process with no pipe connection allowing interaction

  970516 In processBang(), a trailing '\n' removed.  Failure to do this
         led to problems in Windows version

  990215 changed putOUTSTR() to putErrorOUTSTR() and myerrorout() to
         putOutMsg()
*/

#include "globals.h"
#include "mainpars.h"

#ifndef BANG
#define BANG   '!'
#endif /*BANG*/

#ifdef BCPP
#include <stdlib.h>
#endif /* BCPP */

static char        Funcname[NAMELENGTH+3];

#ifdef HASPOPEN

#include <errno.h>

#define PIPECHUNK 256

#ifdef MSDOS
#ifdef DJGPP
#define READMODE "r"
#else /*DJGPP*/
#define READMODE "rt"
#endif /*DJGPP*/
#else /*MSDOS*/
#define READMODE "r"
#endif /*MSDOS*/

#define WRITEMODE TEXTWRITEMODE /*was "wb" in djgpp popen(); may make no difference*/

#if defined(DJGPP) /* || defined (wx_msw) doesn't work for wx_msw */

/* the following adapted from code in DJGPP file pipe.c */

#include <io.h>
#include <fcntl.h>

/*
  make sure storage allocation is compatible with MacAnova
*/
#define malloc(N) mygetpointer(N)
#define free(P)   myfreepointer(P)

#define strdup(S) mystrdup(S)
static char * mystrdup(const char *s)
{
	int         length = strlen(s);
	char       *s1 = mygetpointer(length + 1);

	if (s1 != (char *) 0)
	{
		strcpy(s1, s);
	}
	return (s1);
} /*mystrdup()*/

/* hold file pointer, descriptor, command, mode, temporary file name */
typedef struct pipe_list
{
    FILE         *fp;
    int           fd;
    char         *command;
	char          mode[3];
	char          temp_name[13];
    struct        pipe_list *next;
} pipe_list;

/* static, global list pointer */
static pipe_list *Pl = NULL;
static int        Place; /*for debugging*/

/*
  cm is command to be executed
  md is mode ("r") or ("w") for opening pipe
*/
FILE * popen (const char * cm, const char * md) /* program name, pipe mode */
{
	pipe_list    *l1, *l2;
	static char  *tn = (char *) 0;       /* temporary file basename */
	static char   tn1[13];
	WHERE("popen");
	
	Place = 0;
	if (tn == (char *) 0)
	{
		strcpy(tn1, "pXXXXXX");
		if ((tn = mktemp (tn1)) == (char *) 0)
		{
			return ((FILE *) 0);
		}
	} /*if (!tn)*/
  
	/* make new node */
	if ((l1 = (pipe_list *) malloc(sizeof(pipe_list))) == NULL)
	{
		return ((FILE *) 0);
	}

	/* zero out elements to we'll get here */
	l1->fd = 0;
	l1->fp = (FILE *) 0;
	l1->next = (pipe_list *) 0;

	/* if empty list - just grab new node */
	if (Pl == (pipe_list *) 0)
	{
		Pl = l1;
	}
	else
    {
		/* otherwise, find last node in list */
		++(l1->fd);
		l2 = Pl;
		while (l2->next)
        {
			++(l1->fd);
			l2 = l2->next;
        };
		/* add new node to list */
		l2->next = l1;
    }

	/* stick in elements we know already */
	strcpy (l1->mode, md);
	sprintf (l1->temp_name, "%s.%d", tn, l1->fd);

	/* if can save the program name, build temp file */
	if ((l1->command = strdup (cm)))
	 {
		/* if caller wants to read */
		if (l1->mode[0] == 'r')
        {          
			/* dup stdout */
			if ((l1->fd = dup (fileno (stdout))) == EOF)
			{
				l1->fp = (FILE *) 0;
				Place = 1;
			}
			else if (!(l1->fp = freopen (l1->temp_name, WRITEMODE, stdout)))
			{
				l1->fp = (FILE *) 0;
				Place = 2;
			}
            /* exec cmd */
			else if (system (cm) == EOF)
			{ 
				l1->fp = (FILE *) 0;
				Place = 3;
			}

			/* reopen real stdout */
			if (dup2 (l1->fd, fileno (stdout)) == EOF)
			{				l1->fp = (FILE *) 0;
				Place = 10*Place + 4;
			}
			else
			{
				/* open file for reader */
				l1->fp = fopen (l1->temp_name, "r");
			}
#ifdef DJGPP
			setmode(fileno (stdout), O_TEXT); /*KB*/
#endif /*DJGPP*/
		} /*if (l1->mode[0] == 'r')*/
		/* if caller wants to write */
		else if (l1->mode[0] == 'w')
		{
			/* open temp file */
			l1->fp = fopen (l1->temp_name, l1->mode);
		}
		else
		{
			/* unknown mode */
			l1->fp = (FILE *) 0;
			Place = 5;
		}
    } /*if (l1->command = strdup (cm))*/
	
	return (l1->fp);              /* return == NULL ? ERROR : OK */
} /*popen()*/

int pclose (FILE * pp)
{
	pipe_list    *l1, *l2;    /* list pointers */
	int           retval;     /* function return value */

	/* if pointer is first node */
	if (Pl->fp == pp)
    {
		/* save node and take it out the list */
		l1 = Pl;
		Pl = l1->next;
    }
	/* if more than one node in list */
	else if (Pl->next)
	{
		/* find right node */
		for (l2 = Pl, l1 = Pl->next; l1 != (pipe_list *) 0;
			 l2 = l1, l1 = l2->next)
		{
			if (l1->fp == pp)
			{
				break;
			}
		}		
		/* take node out of list */
		l2->next = l1->next;
	}
	else
	{
		return (-1);
	}
	
	/* if FILE not in list - return error */
	if (l1->fp == pp)
    {
		/* close the (hopefully) popen()ed file */
		fclose (l1->fp);

		/* if pipe was opened to write */
		if (l1->mode[0] == 'w')
        {
			/* dup stdin */
			if ((l1->fd = dup (fileno (stdin))) == EOF)
			{
				retval = -1;
			}
			/* open temp stdin */
			else if (!(l1->fp = freopen (l1->temp_name, "rb", stdin)))
			{
				retval = -1;
			}
			/* exec cmd */
			else if (system (l1->command) == EOF)
			{
				retval = -1;
			}
			/* reopen stdin */
			else if (dup2 (l1->fd, fileno (stdin)) == EOF)
			{
				retval = -1;
			}
        } /*if (l1->mode[0] == 'w')*/
        /* if pipe was opened to read */
		else if (l1->mode[0] == 'r')
		{
			retval = 0;
		}
        /* invalid mode */
		else
		{
			retval = -1;
		}
    } /*if (l1->fp == pp)*/
	unlink (l1->temp_name);       /* remove temporary file */
	free (l1->command);           /* dealloc memory */
	free ((char *) l1);                    /* dealloc memory */
	l1 = NULL;                    /* make pointer bogus */

	return (retval);              /* retval==0 ? OK : ERROR */
} /*pclose ()*/
#elif defined(wx_msw) /*DJGPP*/
#define popen   _popen
#define pclose  _pclose
#include "winerror.h"
#elif defined (HASPOPEN) /*DJGPP*/
extern FILE *popen(const char *, const char *);
#endif /*DJGPP*/

enum pipecodes
{
	POPENERROR,
	FREADERROR
};

static void pipeError(enum pipecodes code)
{
	char          buffer[201];
	int           length;

	strncpy(buffer, strerror(errno), 200);
	buffer[200] = '\0';
	length = strlen(buffer);
	while (length > 0 && buffer[length - 1] == '\n')
	{
		buffer[--length] = '\0';
	}
	sprintf(OUTSTR, "%s problem %s pipe: %s",
			  Funcname,
			  (code == POPENERROR) ? "opening" : "reading from",
			  buffer);
	putErrorOUTSTR();
	errno = 0;
} /*pipeError()*/

static Symbolhandle readPipe(Symbolhandle symhCommand,
							 unsigned int * errorCode)
{
	Symbolhandle      result = (Symbolhandle) 0;
	FILE             *fromPipe;
	long              length = 0, unused;
	long              nread, nlines = 0;
	char              c;
	char             *shellOutput;
	WHERE("readPipe");
	
	strncpy(OUTSTR, STRINGPTR(symhCommand), BUFFERLENGTH-3);
	OUTSTR[BUFFERLENGTH-2] = '\0';
	length = strlen(OUTSTR);

	fromPipe = popen(OUTSTR, READMODE);

	OUTSTR[0] = '\0';

	if (fromPipe == (FILE *) 0)
	{
#ifdef wx_msw
		if (_doserrno == ERROR_CALL_NOT_IMPLEMENTED)
		{
			sprintf(OUTSTR, "ERROR: %s not available with this operating system",
						Funcname);
			goto errorExit;
		}
#endif /*wx_msw*/
/*		fprintf(stderr,"Place = %d\n",Place);*/
		pipeError(POPENERROR);
		goto errorExit;
	}
	
	unused = PIPECHUNK;
	result = CInstall(SCRATCH, unused + 1);
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	TMPHANDLE = STRING(result);
	length = 0;

	while (1)
	{
		nread = fread(*TMPHANDLE + length, sizeof(char), unused, fromPipe);
		if (ferror(fromPipe))
		{
			pipeError(FREADERROR);
			goto errorExit;
		}

		length += nread;
		unused -= nread;
		(*TMPHANDLE)[length] = '\0';

		if (feof(fromPipe))
		{
			break;
		}

		if (unused < 20)
		{
			unused = PIPECHUNK;
			setSTRING(result, mygrowhandle(TMPHANDLE, length + unused + 1));
			TMPHANDLE = STRING(result);
			if (TMPHANDLE == (char **) 0)
			{
				goto errorExit;
			}
		} /*if (unused < 20)*/			
	} /*while (1)*/

	if (length > 0 && (*TMPHANDLE)[length - 1] == '\n')
	{ /* trim off final '\n' if any */
		length--;
		unused++;
		(*TMPHANDLE)[length] = '\0';
	} /*if ((*TMPHANDLE)[length - 1] == '\n')*/

	/* 
	   there should be no need to check myhandlelenth(TMPHANDLE)
	   for positivity since TMPHANDLE has just been created
	*/
	if (myhandlelength(TMPHANDLE) > length + 1)
	{ /*shrink to proper size */
		setSTRING(result, mygrowhandle(TMPHANDLE, length + 1));
		TMPHANDLE = STRING(result);
		if (TMPHANDLE == (char **) 0)
		{
			goto errorExit;
		}
	} /*if (unused > 0)*/

	/*replace any remaining new lines by '\0'*/
	for (shellOutput = *TMPHANDLE; c = *shellOutput; shellOutput++)
	{
		if (c == '\n')
		{
			*shellOutput = '\0';
			nlines++;
		}
	} /*for (shellOutput = *TMPHANDLE; c = *shellOutput; shellOutput++)*/
	nlines++;
	setDIM(result, 1, nlines);
	
	*errorCode = pclose(fromPipe);

	return (result);
	
  errorExit:
	putOUTSTR();
	Removesymbol(result);
	return (0);
		
} /*readPipe()*/
#endif /*HASPOPEN*/

/*
  MacAnova usage()
  shell("command",interact:T)
  shell("command",keep:T)
  shell("command") is equivalent to shell("command",keep:F)

  When neither HASPOPEN or HASSYSTEM is defined, shell() is not implemented
  When HASPOPEN is not defined, keep:T is illegal.
  HASSYSTEM is defined, interact:T and interact:F are identical

  970815 Improved handling of shell("")
*/

#if defined(HASSYSTEM) || defined(HASPOPEN)
Symbolhandle shell(Symbolhandle        list)
{
	Symbolhandle      symh, symhCommand, result = (Symbolhandle) 0;
	long              nargs = NARGS(list);
	unsigned int      errorCode;
	int               keep = 0, interact = -1;
	char             *keyword, *keepKey = "keep", *interactKey = "interact";
	WHERE("shell");

	*OUTSTR = '\0';

	strcpy(Funcname, FUNCNAME);
	if (Funcname[0] != BANG)
	{
		strcat(Funcname, "()");
	}
	if (nargs > 2)
	{
		badNargs(Funcname, -2);
		goto errorExit;
	}

	symhCommand = COMPVALUE(list,0);
	if (!argOK(symhCommand,0,0))
	{
		goto errorExit;
	}

	if (!isCharOrString(symhCommand))
	{
		sprintf(OUTSTR,
				"argument%s to %s()", (nargs == 1) ? "" : " 1", Funcname);
		notCharOrString(OUTSTR);
		*OUTSTR = '\0';
		goto errorExit;
	} /*if (!isCharOrString(symhCommand))*/

	if (nargs > 1)
	{
		symh = COMPVALUE(list, 1);
		if (!(keyword = isKeyword(symh)) ||
			(strcmp(keyword, keepKey) != 0 &&
			 strcmp(keyword, interactKey) != 0) ||
			!isTorF(symh))
		{
			sprintf(OUTSTR,
					"ERROR: argument 2 to %s must be '%s:T' or '%s:T",
					Funcname, keepKey, interactKey);
			goto errorExit;
		}
		if (strcmp(keyword, keepKey) == 0)
		{
			keep = (DATAVALUE(symh, 0) != 0.0);
		}
		else
		{
			interact = (DATAVALUE(symh, 0) != 0.0);
		}
	} /*if (nargs > 1)*/		

#ifdef HASPOPEN
	interact = (interact < 0) ? 0 : interact;
#else /*HASPOPEN*/
	if (keep)
	{
		sprintf(OUTSTR,
				"ERROR: %s(command,%s:T) is illegal in this version",
				FUNCNAME, keepKey);
		goto errorExit;
	}
	if (interact == 0 && Funcname[0] != BANG)
	{
		sprintf(OUTSTR,
				"WARNING: %s:F ignored by %s in this version",
				interactKey, Funcname);
		putOUTSTR();
	}
	interact = 1;
#endif /*!HASPOPEN*/

	if (STRINGVALUE(symhCommand, 0) == '\0')
	{
		sprintf(OUTSTR, "WARNING: null command \"\" to %s", Funcname);
		putOUTSTR();
		if (interact || !keep)
		{
			result = NULLSYMBOL;
		}
		else
		{
			result = CInstall(SCRATCH, 1);
			if (result == (Symbolhandle) 0)
			{
				goto errorExit;
			}
		}
	} /*if (STRINGVALUE(symhCommand, 0) == '\0')*/
	else
	{
		if (interact)
		{
			errorCode = system(STRINGPTR(symhCommand));
			result = NULLSYMBOL;
		}
#ifdef HASPOPEN
		else
		{
			result = readPipe(symhCommand, &errorCode);
			if (result == (Symbolhandle) 0)
			{
				goto errorExit;
			}
		}

		if (!keep && !interact)
		{
			char      *outstr = STRINGPTR(result);
			long       nlines = DIMVAL(result, 1);
		
			while (nlines > 0)
			{
				putOutMsg(outstr);
				if (--nlines > 0)
				{
					outstr = skipStrings(outstr, 1);
				} /*if (--nlines > 0)*/
			} /*while (1)*/
			Removesymbol(result);
			result = NULLSYMBOL;
		} /*if (!keep && !interact)*/
#endif /*HASPOPEN*/

		if (errorCode)
		{
#ifdef UNIX
			errorCode /= 256;
#endif /*UNIX*/

			strcpy(OUTSTR,"WARNING: ");
#ifdef BCPP
			strcat(OUTSTR, strerror(errno));
#else /*BCPP*/
			sprintf(OUTSTR+strlen(OUTSTR),"Operating system return code = %d",
					errorCode);
#endif /*BCPP*/
			goto errorExit;
		} /*if (errorCode)*/
	} /*if (STRINGVALUE(symhCommand, 0) == '\0'){}else{}*/
	
/* fall through */
  errorExit:
	putOUTSTR();

	return (result);
		
} /*shell()*/

#else /*HASSYSTEM || HASPOPEN*/

Symbolhandle shell(Symbolhandle        list)
{
	char       *notImplemented = " not implemented in this version";

	if (FUNCNAME[0] != BANG)
	{
		sprintf(OUTSTR, "ERROR: %s()%s", FUNCNAME, notImplemented);
	}
	else
	{
		sprintf(OUTSTR, "ERROR: Lines starting with '%s'%s",
				FUNCNAME, notImplemented);
	}
	
	putOUTSTR();

	return (0);
} /*shell()*/
#endif /*HASSYSTEM || HASOPEN*/

#ifdef RECOGNIZEBANG
#define Nargs  2

int processBang(void)
{
/*
   valueList[0] is set to a string whose size is the same as as double
   on many platforms so that its copy made in Buildlist() will come from a
   cache.  valueList[0] is not set to *INPUTSTRING + 1 because Buildlist()
   allocates storage.
*/
	Symbolhandle     list;
	long             typeList[Nargs];
	long             inputLength = strlen((char *) *INPUTSTRING);
	char            *valueList[Nargs], *keyList[Nargs];

	if (isNewline((*INPUTSTRING)[inputLength-1]))
	{
		(*INPUTSTRING)[--inputLength] = '\0';
	}
	if (inputLength > 1)
	{
		/* place holder for command */
		typeList[0] = CHAR;
		valueList[0] = "XXXXXXX";
		keyList[0] = "";

		/* interact:T */
		typeList[1] = LOGIC;
		valueList[1] = "T";
		keyList[1] = "interact";

		list = Buildlist(Nargs, typeList, valueList, keyList);
		if (list != (Symbolhandle) 0 &&
			(TMPHANDLE = mygethandle(inputLength)) != (char **) 0)
		{
			mydisphandle(STRING(COMPVALUE(list, 0)));
			setSTRING(COMPVALUE(list, 0), TMPHANDLE);
			strcpy(*TMPHANDLE, (char *) *INPUTSTRING + 1);
			FUNCNAME[0] = BANG;
			FUNCNAME[1] = '\0';
			(void) shell(list);
			CleanupList(list);
			Removesymbol(list);
		}
	} /*if (inputLength > 1)*/
	return ((FatalError) ? FATALERROR : 0);
} /*processBang()*/
#endif /*RECOGNIZEBANG*/

