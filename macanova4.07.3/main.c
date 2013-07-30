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
   Main program and driver

   961009 added scan of environmental variable $MACANOVA as a pseudo-
          command line prior to scan of command line
          Concentrated this in new module processArgs()

   970129 Modified handling of lines starting with '!' so it can be
          used in wx versions.
   970609 empty lines are no longer sent to the parser.
   970926 Added code to check environmental variables LINES and
		  COLUMNS
   980329 Added code to process command line -prompt and -bprompt
   980423 Stopped incrementing NERRORS after error return from yyparse()
          since it effectively counted most errors twice.
   980428 Added command line flags -e and -eq
   980512 Startup message now printed after setting of screen
          dimensions
   990215 changed myerrorout to putOutErrorMsg() and putOutMsg() and some
          uses of putOUTSTR() to putErrorOUTSTR()
*/
#define MAIN__
#include "globals.h"
#include "mainpars.h"
#ifdef BCPP
#include <io.h> /* for isatty */
#else /*BCPP*/
#ifndef EPX
#include <unistd.h> /* isatty for ultrix on DECStation */
#else /*EPX*/
int   isatty(int);
#endif /*EPX*/
#endif /*BCPP*/

#ifdef BCPP
#ifndef OVERLAYSIZE
#define OVERLAYSIZE 0x1000
#endif /*OVERLAYSIZE*/
#ifndef STACKSIZE
#define STACKSIZE 32000
#endif /*STACKSIZE*/
unsigned          _ovrbuffer = OVERLAYSIZE;
extern unsigned   _stklen = STACKSIZE;
#endif /* BCPP */

extern int      yydebug; /* set 1 for yacc debugging output */

extern char         *getenv();
extern void         *malloc();

#ifdef HASISATTY
int           isatty(int);
#endif /*HASISATTY*/


#ifdef COMMANDLINE
#ifdef MSDOS
/*  '/' equivalent to '\' on MSDOS*/
#define isSlash(C) ((C) == DIRSEPARATOR[0] || (C) == '/')
#else /*MSDOS*/
#define isSlash(C) ((C) == DIRSEPARATOR[0])
#endif /*MSDOS*/

#define SHIFT (++argv, --argc)

#define MOTDMARKER       "!!!!" /* Marker for Message of the day in Help file*/
#define MOTDMARKERLENGTH  4

#define MEMGUBED   (4096)

#ifdef MONITOR
#include <mon.h>
static short   MonitorStarted = 0;
static WORD   *MonitorBuffer = (WORD *) 0;
extern etext;
extern int     endofmv(void); /*loaded last*/

#define MONGUBED   (16384)
#define MONSTART (void(*)()) 2
#define MONEND ((void(*)()) endofmv) /* instead of (void(*)()) &etext*/
#define MONEND1 ((void(*)()) &etext) /* yields garbage */
#define MONEND2 ((void (*)()) 2000000) /* seems to work best */
#ifndef MONNFUNC
#define MONNFUNC   (GUBED - MONGUBED)
#endif /*MONNFUNC*/
#ifndef MONBUFSIZE
#define MONBUFSIZE  (1000000)
#endif /*MONBUFSIZE*/

static startMonitor(void)
{
	long        i;
	WHERE("startMonitor");
	
	MonitorBuffer = (MonitorBuffer == (WORD *) 0) ?
		(WORD *) malloc(MONBUFSIZE * sizeof(WORD)) : MonitorBuffer;
	
	if (MonitorBuffer == (WORD *) 0)
	{
		return;
	} /*if (MonitorBuffer == (WORD *) 0)*/
	for (i=0; i < MONBUFSIZE; i++)
	{
		MonitorBuffer[i] = (WORD) 0;
	}
	monitor(MONSTART, MONEND2, MonitorBuffer, MONBUFSIZE, MONNFUNC);
	MonitorStarted = 1;
} /*startMonitor()*/

static void stopMonitor(void)
{
	monitor((void (*)()) 0, (void (*)()) 0, (WORD *) 0, 0, 0);
	MonitorStarted = 0;
}

#endif /*MONITOR*/


static char      ProgName[64];
static char     *UsageMsg[] = 
{
	"Options are:",
	"  -q                   Suppress printing banner",
	"  -batch batchFile     Simulate batch(\"batchFile\")",
	"  -restore restoreFile Simulate restore(\"restoreFile\")",
	"  -f startupFile       Use startupFile instead of .macanova.ini",
	"  -prompt Prompt       Use Prompt as input prompt",
	"  -bprompt Prompt      Use Prompt as batch prompt (with -batch)",
	"  -e Expression        Evaluate Expression before doing anything else",
#ifndef SCROLLABLEWINDOW
	"  -eq Expression       Evaluate Expression and quit (not with -batch)",
#endif /*SCROLLABLEWINDOW*/
	"  -help helpFile       Use helpFile as default file for help information",
	"  -macro macroFile     Add \"macroFile\" to variable MACROFILES",
	"  -data dataFile       Set variable DATAFILE to \"dataFile\"",
	"  -dpath dataPath      Set variable DATAPATHS to \"dataPath\"",
	"  -mpath macroPath     Add \"macroPath\" to variable DATAPATHS",
	"  -home homePath       Set variable HOME to \"homePath\"",
	"  -l nlines            Set screen height to nlines (0 or >= 6)",
	"  -w ncols             Set screen width to ncols (>= 20)",
#ifdef SAVEHISTORY
	"  -history n           Set number of history lines to n (>= 0)",
#endif /*SAVEHISTORY*/
	"  -f and -r should not both be specified",
	(char *) 0	
};

static void usage(void)
{
	char    **msgsLine;

	sprintf(OUTSTR,	"usage: %s [options]", ProgName);
	putOUTSTR();

	for (msgsLine = UsageMsg;*msgsLine;msgsLine++)
	{
		putOutMsg(*msgsLine);
	}
} /*usage()*/

/*
   960502 changed fopen() to fmyopen()
   980329 added argument prompt
*/
static int doBatch(char * fileName, int echo, char * prompt)
{
	FILE           *batchFile;
	long            nargs;
	long            typeList[2];
	char           *valueList[2];
	char           *keyList[2];
	Symbolhandle    list;
	WHERE("doBatch");


	batchFile = fmyopen(fileName,TEXTREADMODE);
	if (batchFile == (FILE *) 0)
	{
		return (0);
	}
 /* dummy up MacAnova 'batch(filename,echo)' command */
	fclose(batchFile);
	nargs = 2;
	typeList[0] = CHAR;
	valueList[0] = fileName;
	keyList[0] = "";
	if (prompt[0] != '\0')
	{
		typeList[1] = CHAR;
		valueList[1] = prompt;
		keyList[1] = "prompt";
	} /*if (prompt[0] != '\0')*/
	else
	{
		typeList[1] = LOGIC;
		valueList[1] = (echo) ? "T" : "F";
		keyList[1] = "echo";
	} /*if (prompt[0] != '\0'){}else{}*/
	
	list = Buildlist(nargs,typeList,valueList,keyList);
	if (list != (Symbolhandle) 0)
	{
		strcpy(FUNCNAME, "batch");
		(void) batch(list);
		Removelist(list);
	}
	return (1);
} /*doBatch()*/

/*
	Dummy up restore(fileName) command and execute it.

	doRestore appears in main.c in non-mac versions; the ifdefs are included
	to make the code identical

	960502 changed fopen() to fmyopen()
	960503 changed macOpen() to macFindFile()
*/

#ifdef MACINTOSH
static int doRestore(char * fileName, Integer vRefNum)
#else /*MACINTOSH*/
static int     doRestore(char * fileName)
#endif /*MACINTOSH*/
{
	Symbolhandle list;
	long         typeList[1];
	char        *valueList[1];
	char        *keyList[1];
	FILE        *restoreFile;
	int          errorFlag = 0;
#ifdef MACINTOSH
	OSType       types[2];
	
	if (fileName == (char *) 0)
	{
		types[0] = KEEPFILETYPE;
		types[1] = ASCIIKEEPFILETYPE;
		fileName = macFindFile("", "\pMacAnova workspace file",
							   (unsigned char *) 0, READIT,	2, types,
							   (Integer *) 0);
		if (fileName == (char *) 0)
		{
			return (1);
		}
	}
	else
	{
		SetVol(0L, vRefNum);
	}
#endif /*MACINTOSH*/

	restoreFile = fmyopen(fileName,TEXTREADMODE);
	if (restoreFile == (FILE *) 0)
	{
		if (PRINTWARNINGS)
		{
			myprint("WARNING: unable to open file ");
			putOutErrorMsg(fileName);
		}
		errorFlag = 1;
	}
	else
	{
		fclose(restoreFile);

		/* dummy up restore(filename) */
		typeList[0] = CHAR;
		valueList[0] = fileName;
		keyList[0] = "";
		list = Buildlist(1,typeList,valueList,keyList);
		if (list == (Symbolhandle) 0)
		{
			errorFlag = 1;
		}
		else
		{
			if (restore(list) == (Symbolhandle) 0)
			{
				errorFlag = 1;
			}
#ifdef MACINTOSH
			UNLOADSEG(restore);
#endif /*MACINTOSH*/
			Removelist(list);
		}
		if (errorFlag && PRINTWARNINGS)
		{
			myprint("WARNING: unable to restore workspace from file ");
			putOutErrorMsg(fileName);
		}
	}
	return (errorFlag);
} /*doRestore()*/

#endif /*COMMANDLINE*/

#ifdef HASISATTY
#define TTYIN       ((isatty(0)) ? ITTYIN  : 0)
#define TTYOUT      ((isatty(1)) ? ITTYOUT : 0)
#else /*HASISATTY*/
#define TTYIN    ITTYIN    /* always true for non-Unix */
#define TTYOUT   ITTYOUT   /* always true for non-Unix */
#endif /*HASISATTY*/

static short    DoInit = 1;

#ifdef MSDOS
char	      **ArgV;   /* set equal to argv */
#endif /*MSDOS*/

#ifdef COMMANDLINE
#ifndef ENVBUFLENGTH
#define ENVBUFLENGTH  512
#endif /*ENVBUFLENGTH*/
#define FROMENV(N) ((N) >= buffer && (N) < buffer + ENVBUFLENGTH)

#ifndef ENVNAME
#define ENVNAME "MACANOVA"
#endif /*ENVNAME*/

typedef struct MacAnovaCommandLineArgs
{
	char            buffer[ENVBUFLENGTH];
	char            *initFileName;
	char            *batchFileName;
	char            *restoreFileName;
	char            *helpFileName;
	char            *dataFileName;
	char            *macroFileName;
	char            *dataPathName;
	char            *macroPathName;
	char            *homePathName;
	promptType       commandPrompt;
	promptType       batchPrompt;
	long             nlines;
	long             ncols;
	char            *expression;
#ifndef SCROLLABLEWINDOW
	char            *expressionQ;
#endif /*SCROLLABLEWINDOW*/
#ifdef SAVEHISTORY
	long             historyLength;
#endif /*SAVEHISTORY*/
	long             quiet;
} MacAnovaCommandLineArgs;  /*struct MacAnovaCommandLineArgs*/

static void badCommandLineFile(char * name, char * what1, char * what2,
							   int fromEnv)
{
	char          source[50];

	if (fromEnv)
	{
		sprintf(source," (from env. variable %s)", ENVNAME);
	}
	else
	{
		source[0] = '\0';
	}

	sprintf(OUTSTR, "ERROR: %s%s is not a legal %s name for %s",
			name, source, what1, what2);
	putErrorOUTSTR();
} /*badCommandLineFile()*/

static int processArgs(int argc, char ** argv,
					   MacAnovaCommandLineArgs * lineArgs)
{
	char         *cmd;
	char        **argvs[2];
	char         *argV[50];
	char         *envName = ENVNAME;
	char         *macEnv = getenv(envName);
	char         *nlinesStr = (char *) 0, *ncolsStr = (char *) 0;
	char         *pathName, *fileName;
	char         *cPrompt = (char *) 0, *bPrompt = (char *) 0;
	char         *buffer = lineArgs->buffer;
	int           argcs[2], argC = 0, i = -1;
	char          fromEnv[50];
	char         *progname;
	char         *tmp;
	long          screenHeight, screenWidth;
#ifdef SAVEHISTORY
	char         *historyLengthStr = (char *) 0;
	long          history;
#endif /*SAVEHISTORY*/
	WHERE("processArgs");
	
	sprintf(fromEnv," (from env. variable %s)", envName);
	
#ifdef MSDOS
	ArgV = argv;
#endif /*MSDOS*/

	lineArgs->initFileName = (char *) 0;
	lineArgs->batchFileName = (char *) 0;
	lineArgs->restoreFileName = (char *) 0;
	lineArgs->helpFileName = (char *) 0;
	lineArgs->dataFileName = (char *) 0;
	lineArgs->macroFileName = (char *) 0;
	lineArgs->dataPathName = (char *) 0;
	lineArgs->macroPathName = (char *) 0;
	lineArgs->commandPrompt[0] = '\0';
	lineArgs->batchPrompt[0] = '\0';
#ifdef UNIX
	lineArgs->homePathName = getenv("HOME");
#else /*UNIX*/
	lineArgs->homePathName = (char *) 0;
#endif /*UNIX*/
	lineArgs->nlines = -1;
	lineArgs->ncols = -1;
	lineArgs->expression = (char *) 0;
	lineArgs->expressionQ = (char *) 0;
#ifdef SAVEHISTORY
	lineArgs->historyLength = -1;
#endif /*SAVEHISTORY*/
	lineArgs->quiet = 0;
	
	/* find start of actual file name */
	for (progname = *argv;*progname;progname++)
	{
		;
	}
#ifdef MSDOS
	while (--progname >= *argv && !isSlash(*progname) && *progname != ':')
	{
		;
	}
#else /*MSDOS*/
	while (--progname >= *argv && !isSlash(*progname))
	{
		;
	}
#endif /*MSDOS*/
	progname++;
	strcpy(ProgName, progname);

#ifdef MSDOS
	progname = ProgName + strlen(ProgName);
	if (mystrncmp(progname - 4, ".exe", 4) == 0)
	{
		progname -= 4;
		*progname = '\0';
	}
#endif /*MSDOS*/

	if (macEnv != (char *) 0)
	{
		char      c, *dest = buffer, *src = macEnv;
		
		*dest = '\0';
		while (*src!= '\0')
		{
			argV[argC++] = dest;
			while (isspace(*src))
			{
				src++;
			}
			while (*src != '\0' && !isspace(*src))
			{
				*dest++ = *src++;
			}
			*dest++ = '\0';
		} /*while (*src!= '\0')*/
		if (argV[argC - 1] == '\0')
		{
			argC--;
		}
		argvs[0] = argV - 1;
		argcs[0] = argC + 1;
	} /*if (macEnv != (char *) 0)*/
	else
	{
		argcs[0] = 1;
	}
	argvs[1] = argv;
	argcs[1] = argc;
	
	for (i = 0; i < 2; i++)
	{
		argv = argvs[i];
		argc = argcs[i];
		
		while (SHIFT && **argv == '-')
		{
			cmd = *argv;
			if (strcmp(cmd,"-q") == 0)
			{
				lineArgs->quiet = 1;
			} /*if (strcmp(cmd,"-q") == 0)*/
			else
			{
				/* all options except -q require following field */
				if (SHIFT == 0)
				{
					goto parseError;
				} /*if (SHIFT == 0)*/

				if (strcmp(cmd,"-f") == 0)
				{
					lineArgs->initFileName = *argv;
				}
				else if (strcmp(cmd,"-r") == 0 || strcmp(cmd,"-restore") == 0)
				{
					lineArgs->restoreFileName = *argv;
				}
				else if (strcmp(cmd,"-b") == 0 || strcmp(cmd,"-batch") == 0)
				{
					lineArgs->batchFileName = *argv;
				}
				else if (strcmp(cmd,"-h") == 0 || strcmp(cmd,"-help") == 0)
				{
					lineArgs->helpFileName = *argv;
				}
				else if (strcmp(cmd,"-d") == 0 || strcmp(cmd,"-data") == 0)
				{
					lineArgs->dataFileName = *argv;
				}
				else if (strcmp(cmd,"-m") == 0 || strcmp(cmd,"-macro") == 0)
				{
					lineArgs->macroFileName = *argv;
				}
				else if (strcmp(cmd,"-dp") == 0 || strcmp(cmd,"-dpath") == 0)
				{
					lineArgs->dataPathName = *argv;
				}
				else if (strcmp(cmd,"-mp") == 0 || strcmp(cmd,"-mpath") == 0)
				{
					lineArgs->macroPathName = *argv;
				}
				else if (strcmp(cmd,"-home") == 0)
				{
					lineArgs->homePathName = *argv;
				}
				else if (strcmp(cmd,"-p") == 0 || strcmp(cmd,"-prompt") == 0)
				{
					cPrompt = *argv;
				}
				else if (strcmp(cmd,"-bp") == 0 || strcmp(cmd,"-bprompt") == 0)
				{
					bPrompt = *argv;
				}
				else if (strcmp(cmd,"-l") == 0)
				{
					nlinesStr = *argv;
				}
				else if (strcmp(cmd,"-w") == 0)
				{
					ncolsStr = *argv;
				}
				else if (strcmp(cmd,"-e") == 0)
				{
					lineArgs->expression = *argv;
				}
#ifndef SCROLLABLEWINDOW
				else if (strcmp(cmd,"-eq") == 0)
				{
					lineArgs->expressionQ = *argv;
				}
#endif /*SCROLLABLEWINDOW*/
#ifdef SAVEHISTORY
				else if (strcmp(cmd,"-hist") == 0 ||
						strcmp(cmd,"-history") == 0)
				{
					historyLengthStr = *argv;
				}
#endif /*SAVEHISTORY*/
				else
				{
					goto parseError;
				}
			} /*if (strcmp(cmd,"-q") == 0){}else{}*/
		} /*while (SHIFT && **argv == '-')*/

		if (argc > 0)
		{
			goto parseError;
		}
	} /*for (i = 0; i < 2; i++)*/
	
	/* Check file and path names for legality */
	fileName = lineArgs->helpFileName;
	if (fileName != (char *) 0 && !okfilename(fileName))
	{
		badCommandLineFile(fileName, "file", "help file", FROMENV(fileName));
		goto errorExit;
	}

	fileName = lineArgs->dataFileName;
	if (fileName != (char *) 0 && !okfilename(fileName))
	{
		badCommandLineFile(fileName, "file", "data file", FROMENV(fileName));
		goto errorExit;
	} /*if (fileName != (char *) 0 && !okfilename(fileName))*/

	fileName = lineArgs->macroFileName;
	if (fileName != (char *) 0 && !okfilename(fileName))
	{
		badCommandLineFile(fileName, "file", "macro file", FROMENV(fileName));
		goto errorExit;
	} /*if (fileName != (char *) 0 && !okfilename(fileName))*/

	fileName = lineArgs->restoreFileName;
	if (fileName != (char *) 0 && !okfilename(fileName))
	{
		badCommandLineFile(fileName, "file", "restore file",
						   FROMENV(fileName));
		goto errorExit;
	} /*if (fileName != (char *) 0 && !okfilename(fileName))*/

	fileName = lineArgs->initFileName;
	if (fileName != (char *) 0 && !okfilename(fileName))
	{
		badCommandLineFile(fileName, "file", "initialization file",
						   FROMENV(fileName));
		goto errorExit;
	} /*if (fileName != (char *) 0 && !okfilename(fileName))*/

	fileName = lineArgs->batchFileName;
	if (fileName != (char *) 0 && !okfilename(fileName))
	{
		badCommandLineFile(fileName, "file", "batch file", FROMENV(fileName));
		goto errorExit;
	} /*if (fileName != (char *) 0 && !okfilename(fileName))*/

	pathName = lineArgs->dataPathName;
	if (pathName != (char *) 0 && !okpathname(pathName))
	{
		badCommandLineFile(pathName, "path", "-dpath", FROMENV(pathName));
		goto errorExit;
	} /*if (pathName != (char *) 0 && !okpathname(pathName))*/

	pathName = lineArgs->macroPathName;
	if (pathName != (char *) 0 && !okpathname(pathName))
	{
		badCommandLineFile(pathName, "path", "-mpath", FROMENV(pathName));
		goto errorExit;
	} /*if (pathName != (char *) 0 && !okpathname(pathName))*/

	pathName = lineArgs->homePathName;
	if (pathName != (char *) 0 && !okpathname(pathName))
	{
		badCommandLineFile(pathName, "path", "-home", FROMENV(pathName));
		goto errorExit;
	} /*if (pathName != (char *) 0 && !okpathname(pathName))*/

	if (cPrompt != (char *) 0)
	{
		if (strlen(cPrompt) >= MAXPROMPT)
		{
			sprintf(OUTSTR,
					"WARNING: supplied prompt '%s' longer than %d characters; truncated",
					cPrompt, MAXPROMPT - 1);
			putErrorOUTSTR();
		}
		strncpy((char *) lineArgs->commandPrompt, cPrompt, MAXPROMPT);
		lineArgs->commandPrompt[MAXPROMPT - 1] = '\0';
	}
	else
	{
		lineArgs->commandPrompt[0] = '\0';
	}
	
	if (bPrompt != (char *) 0)
	{
		if (lineArgs->batchFileName == (char *) 0)
		{
			sprintf(OUTSTR,
					"WARNING: batch prompt ignored without '-batch'");
			putErrorOUTSTR();
			lineArgs->batchPrompt[0] = 0;
		}
		else if (strlen(bPrompt) >= MAXPROMPT)
		{
			sprintf(OUTSTR,
					"WARNING: batch prompt '%s' longer than %d characters; truncated",
					bPrompt, MAXPROMPT - 1);
			putErrorOUTSTR();
		}
		strncpy((char *) lineArgs->batchPrompt, bPrompt, MAXPROMPT);
		lineArgs->batchPrompt[MAXPROMPT - 1] = '\0';
	}
	else
	{
		lineArgs->batchPrompt[0] = '\0';
	}

	if (nlinesStr != (char *) 0)
	{
		screenHeight = 0;
		tmp = nlinesStr;
		while (isspace(*tmp))
		{
			tmp++;
		}
		while (*tmp && isdigit(*tmp))
		{
			screenHeight = 10*screenHeight + (*tmp++) - '0';
		}
		if (*tmp != '\0' || tmp == nlinesStr ||
		   (screenHeight < MINSCREENHEIGHT && screenHeight != 0))
		{
			sprintf(OUTSTR,
					"ERROR: value '%s'%s for -l not 0 or integer >= %d",
					nlinesStr, (FROMENV(pathName)) ? fromEnv : NullString,
					MINSCREENHEIGHT);
			goto errorExit;
		}
		lineArgs->nlines = screenHeight;
	} /*if (nlinesStr != (char *) 0)*/

	if (ncolsStr != (char *) 0)
	{
		screenWidth = 0;
		tmp = ncolsStr;
		while (*tmp == ' ')
		{
			tmp++;
		}
		while (*tmp && isdigit(*tmp))
		{
			screenWidth = 10*screenWidth + (*tmp++) - '0';
		}
		if (*tmp != '\0' || tmp == ncolsStr ||
		   screenWidth < MINSCREENWIDTH)
		{
			sprintf(OUTSTR,
					"ERROR: value '%s'%s for -w not integer >= %d",
					ncolsStr, (FROMENV(pathName)) ? fromEnv : NullString,
					MINSCREENWIDTH);
			goto errorExit;
		}
		lineArgs->ncols = screenWidth;
	} /*if (ncolsStr != (char *) 0)*/

#ifdef SAVEHISTORY
	if (historyLengthStr != (char *) 0)
	{
		history = 0;
		tmp = historyLengthStr;
		while (*tmp == ' ')
		{
			tmp++;
		}
		while (*tmp && isdigit(*tmp))
		{
			history = 10*history + (*tmp++) - '0';
		}
		if (*tmp != '\0' || tmp == historyLengthStr ||
		   history < 0)
		{
			sprintf(OUTSTR,
					"ERROR: value '%s' for -hist not integer >= 0",
					historyLengthStr);
			goto errorExit;
		} /*if (*tmp != '\0' || tmp == historyLengthStr || history < 0)*/
		lineArgs->historyLength = history;
	} /*if (historyLengthStr != (char *) 0)*/
#endif /*SAVEHISTORY*/

#ifndef SCROLLABLEWINDOW
	if (lineArgs->expressionQ != (char *) 0)
	{
		if (lineArgs->batchFileName != (char *) 0    ||
			lineArgs->expression != (char *) 0)
		{
			sprintf(OUTSTR, "ERROR: -eq is incompatible with -e and -batch");
			goto errorExit;
		}
		lineArgs->quiet = 1;
		if (lineArgs->nlines < 0)
		{
			lineArgs->nlines = 0;
		}
	} /*if (lineArgs->expressionQ != (char *) 0)*/
#endif /*SCROLLABLEWINDOW*/	
	return (1);

  parseError:
	if (i == 0)
	{
		sprintf(OUTSTR,
				"ERROR: problem with environmental variable %s = '%s'\n",
				envName, macEnv);
	}
	else
	{
		usage();
	}
	/*fall through*/
	
  errorExit:
	putErrorOUTSTR();
	return (0);
} /*processArgs()*/
#endif /*COMMANDLINE*/

static void doMacanova(void)
{
	int               done = 0;
	long              lastchar;
	long              inputStatus = 0;
	unsigned char     c;
	WHERE("doMacanova");

	while ( !done)
	{
		cleanitup();
		if (BDEPTH == 0)
		{
			NLINES = 0;
		}

		putprompt((char *) NULL);

		yydebug = (GUBED & 2048) ? 1 : 0;
#ifdef COUNTMISSES
/*
   code to allow run-time tuning of sizes of warehouses (handle caches)
   activated by setoptions(GUBED:4096+size)
*/
		if (GUBED & MEMGUBED && GUBED > MEMGUBED)
		{
			setWarehouseLimits(GUBED - MEMGUBED);
			GUBED = MEMGUBED;
		} /*if (GUBED & MEMGUBED && GUBED > MEMGUBED)*/
#endif /*COUNTMISSES*/

#ifdef MONITOR
		if (GUBED & MONGUBED && !MonitorStarted)
		{
			startMonitor();
			if (!MonitorStarted)
			{
				PRINT("Cannot allocate MonitorBuffer\n",0,0,0,0);
				GUBED = 0;
			}
			else
			{
				GUBED = MONGUBED;
			}
		}
		else if (MonitorStarted && !(GUBED & MONGUBED))
		{
			stopMonitor();
		}
#endif /*MONITOR*/
/*	try to get a line of input to *INPUTSTRING */
		if ((inputStatus = getinput()) <= 0 )
		{
			done = 1;
		} /*if ((inputStatus = getinput()) <= 0 )*/
		else if ((c = (*INPUTSTRING)[0], c) != '\0')
		{
#ifdef RECOGNIZEBANG
			if (c == BANG)
			{
				done = processBang();
			}
#endif /*RECOGNIZEBANG*/
#ifndef DJGPP
			else if ((done = yyparse()) != 0)
#else /*DJGPP*/
			else if ((_go32_was_ctrl_break_hit(),done = yyparse()) != 0)
#endif /*DJGPP*/
			{
#if (0) /*980423 this resulted in counting most errors twice*/
				if (MAXERRORS1)
				{
					NERRORS++;
				}
#endif /*0*/
				FatalError = (done == FATALERROR) ? 1 : FatalError;
				
				done = (done == END || FatalError != 0);
			}

			if (MAXERRORS1 && NERRORS >= MAXERRORS1)
			{
				if (BDEPTH > 0)
				{
					myprint("WARNING: too many errors on batch file ");
					putOutErrorMsg(*INPUTFILENAMES[BDEPTH]);
					closeBatch(1);
				}
				else if (!ISATTY && MAXERRORS && NERRORS >= MAXERRORS)
				{
					putOutErrorMsg("ERROR: too many errors");
					done = FATALERROR;
					FatalError = 1;
				}
				NERRORS = (BDEPTH <= 0 && ISATTY) ? 0 : NERRORS;
			} /*if (MAXERRORS1 && NERRORS >= MAXERRORS1)*/
		}
	} /* while (!done) */

#ifdef MONITOR
	if (MonitorStarted)
	{
		stopMonitor();
	} /*if (MonitorStarted)*/
#endif /*MONITOR*/

	if (FatalError)
	{
		putOutErrorMsg("FATAL ERROR: Execution terminated");
	}

} /*doMacanova() */

#ifndef COMMANDLINE
main()
#else /*COMMANDLINE*/
int main(int argc,char ** argv)
#endif /*COMMANDLINE*/
{
	long             screenheight  = -1, screenwidth = -1;
	char            *envValue;
#ifdef COMMANDLINE
	MacAnovaCommandLineArgs  lineArgs;
	char            *home, **tempHandle = (char **) 0, *cmd;
#ifdef MACANOVAINI
	char            *rcFileName  = MACANOVAINI;
#else /*MACANOVAINI*/
	char            *rcFileName = (char *) 0;
#endif /*MACANOVAINI*/
	char            *tmp;
#ifdef SAVEHISTORY
	int              history = -1;
#endif /*SAVEHISTORY*/
	long             i;
	int              retval = 1;
	FILE            *batchFile;
	char            *expression;
#endif /*COMMANDLINE*/
	WHERE("main");

#if defined(LINUX)
	ThisMachine = mvLinux;
#elif defined(MSDOS)
	ThisMachine = mvMsDos;
#elif defined(VMS)
	ThisMachine = mvVMS;
#elif defined(UNIX)
	ThisMachine = mvUnix;
#endif

	myInitHandles(); /* initialize handles */

	/* allocate buffer for building messages and other strings*/
	OUTSTR = mygetpointer(BUFFERLENGTH * sizeof(char));
	if (OUTSTR == (char *) 0)
	{
		goto notAllocated;
	}
	*OUTSTR = '\0';

#ifdef COMMANDLINE
	if (!processArgs(argc, argv, &lineArgs))
	{
		goto errorExit;
	}
	screenheight = lineArgs.nlines;
	screenwidth = lineArgs.ncols;
#ifdef SAVEHISTORY
	history = lineArgs.historyLength;
#endif /*SAVEHISTORY*/
#endif /*COMMANDLINE*/

	/* initialize certain globals */
	TERMINAL = getenv("TERM");

	ISATTY = TTYIN | TTYOUT;

#ifdef ACTIVEUPDATE
	if (ISATTY & ITTYOUT)
	{
		activeUpdateInit();
	}
#endif /*ACTIVEUPDATE*/

#if (0)
	startupMsg(lineArgs.quiet);
#endif /*0*/

	/* set interrupt and fp exception transfer location */
	RDEPTH = 0;
	if (setjmp(RestartBuf[RDEPTH]))
	{
		if (!ISATTY)
		{
			sprintf(OUTSTR,
					"WARNING: MacAnova terminated by interrupt in non-interactive mode");
			goto endit;
		}
	} /*if (setjmp(RestartBuf[RDEPTH]))*/

	INTERRUPT = INTNOTSET;

	signal(SIGFPE, fpeRoutine);
	signal(SIGINT, intRoutine);

	if (DoInit)
	{ /* make sure initialize is only called once */
		DoInit = 0;

#if (0) /* activate if macro FPRINT is to be used for debugging */
#ifndef DBUGFILENAME
#define DBUGFILENAME "dbug.out" /* or whatever ... */
#endif /*DBUGFILENAME*/
#ifdef MSDOS
#define DBUGMODE "wt"
#else /*MSDOS*/
#define DBUGMODE "w"
#endif /*MSDOS*/
		DBFILE = fopen(DBUGFILENAME,DBUGMODE);
#endif /*0*/

		/* Initialize handles, functions, predefined macros, options, etc */
#ifdef COMMANDLINE
		initialize(lineArgs.helpFileName,
				   lineArgs.dataFileName,
				   lineArgs.macroFileName,
				   lineArgs.dataPathName,
				   lineArgs.macroPathName,
				   lineArgs.homePathName,
				   lineArgs.commandPrompt);
#else /*COMMANDLINE*/
		initialize((char *) 0, (char *) 0, (char *) 0,
				   (char *) 0, (char *) 0, (char *) 0, (char *) 0);
#endif /*COMMANDLINE*/
		if (FatalError)
		{
			goto notAllocated;
		}

#ifdef UNIX
		if (screenheight < 0 && (envValue = getenv("LINES")) != (char *) 0)
		{
			char     *end;
			long      height = strtol(envValue, &end, 10);
				
			if (height > 0 && *end == '\0')
			{
				screenheight = height;
			}
		}
		if (screenwidth < 0 && (envValue = getenv("COLUMNS")) != (char *) 0)
		{
			char     *end;
			long      width = strtol(envValue, &end, 10);
				
			if (width > 0 && *end == '\0')
			{
				screenwidth = width;
			}
		}
#endif /*UNIX*/

		screenheight = (screenheight < 0) ? SCREENHEIGHT : screenheight;
		screenwidth = (screenwidth < 0) ? SCREENWIDTH : screenwidth;
		installScreenDims(screenwidth, screenheight);

		saveFormat();
#ifdef COMMANDLINE

#ifdef SAVEHISTORY
		history = (history < 0) ? DEFAULTHISTORYLENGTH : history;
#endif /*SAVEHISTORY*/

		startupMsg(lineArgs.quiet);

		 /* make sure help file is found if there */
		HELPFILE = fmyopen(*HELPFILENAME, TEXTREADMODE);

		if (!lineArgs.quiet && HELPFILE != (FILE *) 0 &&
			(LINE = mygethandle(MAXLINE)) != (char **) 0)
#else /*COMMANDLINE*/
		if (HELPFILE != (FILE *) 0 &&
			(LINE = mygethandle(MAXLINE)) != (char **) 0)
#endif /*COMMANDLINE*/
		{
			/*
			  Search help file for message of the day preceded and
			  followed by "!!!!"
			*/
			int       printit = 0;
			char     *msgEnd, *msgStart;
			char     *marker = MOTDMARKER;

			screenwidth = (SCREENWIDTH >= BUFFERLENGTH) ?
				BUFFERLENGTH - 1 : SCREENWIDTH;

			while (fillLINE(HELPFILE) != EOF)
			{
				msgStart = *LINE;
				if (strncmp(msgStart, marker, MOTDMARKERLENGTH) == 0)
				{
					if (!printit)
					{ /* 1st line starting "!!!!" */
						printit = 1;
					}
					else
					{ /* 2nd line starting "!!!!" */
						break;
					}
				} /*if (strncmp(msgStart, marker, MOTDMARKERLENGTH) == 0)*/
				else if (printit)
				{
					while (*msgStart == ' ')
					{ /* trim off starting blanks, if any */
						msgStart++;
					}
					msgEnd = msgStart + strlen(msgStart) - 1;
					while (*msgEnd == ' ' && msgEnd > msgStart)
					{ /* trim off trailing blanks, if any */
						msgEnd--;
					}
					if (msgEnd[0] != '\n')
					{
						msgEnd++;
					}
					*msgEnd = '\0';

					(void) centerBuffer(OUTSTR, msgStart, screenwidth);

					if (printit == 1)
					{ /* first line of message */
						myeol();
						printit = 2;
					}
					putOUTSTR();
				} /*else if (printit)*/
			} /*while (fillLINE(HELPFILE) != EOF)*/
			rewind(HELPFILE);
			mydisphandle(LINE);
			LINE = (char **) 0;
		}

#ifdef COMMANDLINE
		if (lineArgs.restoreFileName != (char *) 0)
		{
			if (doRestore(lineArgs.restoreFileName))
			{
				if (PRINTWARNINGS)
				{
					putPieces("WARNING: cannot restore workspace from file ",
							  lineArgs.restoreFileName, (char *) 0, (char *) 0);
				}				
				lineArgs.restoreFileName = (char *) 0;
			} /*if (doRestore(lineArgs.restoreFileName))*/
			else
			{
				if (lineArgs.initFileName != (char *) 0)
				{
					putOutErrorMsg("WARNING: cannot use -r and -f options together; -f ignored");
				}
				lineArgs.initFileName = rcFileName = (char *) 0;
			} /*if (doRestore(lineArgs.restoreFileName)){}else{}*/
		} /*if (lineArgs.restoreFileName != (char *) 0)*/


#ifdef COMMANDLINE
		expression = (lineArgs.expressionQ) ?
		  lineArgs.expressionQ : lineArgs.expression;
		if (expression != (char *) 0)
		{
			Symbolhandle     result;

			if (lineArgs.expression != (char *) 0)
			{
				myeol();
				sprintf(OUTSTR, "Command Line> %s", expression);
				putOUTSTR();
			}
			result = mvEval(&expression);

			if (lineArgs.expression != (char *) 0)
			{
				if (result == (Symbolhandle) 0)
				{
					sprintf(OUTSTR,
							"FATAL ERROR: \"-e %s\" resulted in error\"",
							lineArgs.expression);
					goto errorExit;
				}			
			}
			else if (result != (Symbolhandle) 0)
			{
				prexpr(result);
			}
		} /*if (expression != 0*/
#endif /*COMMANDLINE*/

/*
  If -f initFileName not specified use ~/rcFileName, if it exists
*/

		if (lineArgs.initFileName == (char *) 0 &&
		   rcFileName != (char *) 0)
		{
#ifdef UNIX
			if ((home = getenv("HOME")))
			{
				tempHandle = mygethandle(strlen(home) + strlen(rcFileName) + 2);
				if (tempHandle == (char **) 0)
				{
					goto notAllocated;
				}
				strcpy(*tempHandle,home);
				strcat(*tempHandle,"/");
				strcat(*tempHandle,rcFileName);
				lineArgs.initFileName = *tempHandle;
			} /*if ((home = getenv("HOME")))*/
			else
#endif /*UNIX*/
			{
				lineArgs.initFileName = rcFileName;
			}
		} /*if (lineArgs.initFileName == (char *) 0 && rcFileName != (char *) 0)*/

/*
  NOTE: if batchFileName has been specified, it is actually executed *after*
        initFileName, even though it appears to be invoked first.  It is as
		if batch(initFileName,echo:F) were the first line of batchFileName.
*/
		if (lineArgs.batchFileName != (char *) 0)
		{
			batchFile = fmyopen(lineArgs.batchFileName, TEXTREADMODE);
			if (batchFile != (FILE *) 0)
			{
				fclose(batchFile);
				(void) doBatch(lineArgs.batchFileName,1,
							   (char *) lineArgs.batchPrompt);
			}
			else if (PRINTWARNINGS)
			{
				myprint("WARNING: Unable to open batch file ");
				putOutErrorMsg(lineArgs.batchFileName);
			}
		} /*if (lineArgs.batchFileName != (char *) 0)*/

		if (lineArgs.initFileName != (char *) 0)
		{ /* no warning needed here since macanova.ini file is optional */
			(void) doBatch(lineArgs.initFileName, 0, NullString);
		}

		mydisphandle(tempHandle);
/*
	970111 changed to use initHistory()
*/
#ifdef SAVEHISTORY
		initHistory(history);
#endif /*SAVEHISTORY*/

#endif /*COMMANDLINE*/
	} /*if (DoInit)*/

#ifdef COMMANDLINE
	if (lineArgs.expressionQ == (char *) 0)
#endif /*COMMANDLINE*/
	{
		doMacanova();
	}
	
	if (GUBED & MEMGUBED)
	{
		memoryUsage(NAMEFORWHERE);
	}

	retval = 0;

  endit:
	
	if (HELPFILE != (FILE *) 0)
	{
		fclose(HELPFILE);
	}
	if (DBFILE != (FILE *) 0)
	{
		fclose(DBFILE);
	}
	if (BDEPTH > 0)
	{
		closeBatch(1);
	}
	return (retval);

  notAllocated:
	sprintf(OUTSTR,
			"FATAL ERROR: unable to allocate memory during inialization");
	PUTALERT(OUTSTR);
	*OUTSTR = '\0';
	retval = -1;

	/* fall through */

  errorExit:
	putErrorOUTSTR();
	return (retval);
} /*main()*/

void closeBatch(long all)
{

	while (BDEPTH > 0)
	{
		mydisphandle(INPUTFILENAMES[BDEPTH]);
		INPUTFILENAMES[BDEPTH] = (char **) 0;
		fclose(INPUTFILE[BDEPTH]);
		INPUTFILE[BDEPTH--] = (FILE *) 0;
		if (!all)
		{
			break;
		}
	} /*while (BDEPTH > 0)*/
	if (BDEPTH <= 0)
	{
		MAXERRORS1 = MAXERRORS;
	}
} /*closeBatch()*/

