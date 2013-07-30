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
  Most of the MacAnovaBaseFrame member functions are defined here
   MacAnovaBaseFrame::MacAnovaBaseFrame()
   MacAnovaBaseFrame::NewTextFrame()
   MacAnovaBaseFrame::CutTextFrame()
   MacAnovaBaseFrame::NewCanvasFrame()
   MacAnovaBaseFrame::CutCanvasFrame()
   MacAnovaBaseFrame::ResetAllWindowsMenus()
   MacAnovaBaseFrame::EnableAllTextWindowMenus()
   MacAnovaBaseFrame::DoQuit()
   MacAnovaBaseFrame::OnClose()
   MacAnovaBaseFrame::OnActivate()
   MacAnovaBaseFrame::CheckForInput()
   MacAnovaBaseFrame::CloseBatch()
   MacAnovaBaseFrame::NVisible()
   MacAnovaBaseFrame::GetRunningWindowParms()
   MacAnovaBaseFrame::GetC()
   MacAnovaBaseFrame::DoStats()
   MacAnovaBaseFrame::ProcessTypeAhead()
   MacAnovaBaseFrame::SaveWorkspace()
   MacAnovaBaseFrame::RestoreWorkspace()
   MacAnovaBaseFrame::FindGraphFrame()
   MacAnovaBaseFrame::FindTextFrame()
   MacAnovaBaseFrame::KeyInterrupt()

   The following are in wxIo.cc
   MacAnovaBaseFrame::lineToWindow()
   MacAnovaBaseFrame::fmyeol()
   MacAnovaBaseFrame::PutPrompt()

   980402 added code to process command line -prompt and -bprompt
   981218 added code to process command line -plotdelay (Motif only)
   990115 replaced myerrorout() by putOutErrorMsg()
*/
#ifdef __GNUG__
#pragma implementation
#endif

// For compilers that support precompilation, includes "wx.h".
#include "wx_prec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx.h"
#endif

#include <new.h> // for prototype for set_new_handler()

#include <math.h>
extern "C"
{
#define MAIN__
#include "wxIface.h"
#include "../globals.h"
#include "../mainpars.h"
#include "../mvsave.h"
} /*extern "C"*/

#if (0)
#undef ALERT
#define ALERT(F,A,B,C,D)
#undef FPRINT
#define FPRINT(F,A,B,C,D)
#endif /*0*/

#ifdef wx_motif
#include <Xm/Text.h>
#include <X11/keysym.h>
#endif

#include <ctype.h>
#include "wx_timer.h"
#include "wx_mf.h"
#include "wxmain.h"


#if defined(wx_msw) && !defined(__WATCOMC__)
#include "mmsystem.h"
#endif

extern int yydebug; /* set 1 for yacc debugging output */

// Declare two frames
MacAnovaBaseFrame   *BaseFrame = NULL;
#if (0)
wxBitmap            *test_bitmap = NULL;
wxIcon              *test_icon = NULL;
#endif /*0*/

#undef UNDEFINED

#ifdef USETICKTIMER
// Timer

class MacAnovaTickTimer: public wxTimer
{
  public:
	void Notify(void);
};

MacAnovaTickTimer *The_TickTimer;
#endif /*USETICKTIMER*/

MacAnovaMainTimer *The_timer;

#undef FREQUENTCURSORSET  //if defined reset cursor in check for input loop

wxCursor *The_arrow_cursor;
wxCursor *The_watch_cursor;
wxCursor *The_ibeam_cursor;

#ifndef MINSCREENHEIGHT
#define MINSCREENHEIGHT  12
#endif /*MINSCREENHEIGHT*/

#ifndef MINSCREENWIDTH
#define MINSCREENWIDTH   30
#endif /*MINSCREENWIDTH*/

#define MOTDMARKER       "!!!!" /* Marker for Message of the day in Help file*/
#define MOTDMARKERLENGTH  4

extern "C"
{
#ifdef wx_motif
extern int isatty(int fildes);
#endif
#ifdef wx_msw
extern int _isatty(int fildes);
#define isatty _isatty
#endif

void myAlert(char * msg);
} /*extern "C"*/

#ifdef wx_msw
static FirstPassShow = 0;
#endif // wx_msw

// This statement initialises the whole application
MacAnovaApp     myApp;

// A macro needed for some compilers (AIX) that need 'main' to be defined
// in the application itself.
IMPLEMENT_WXWIN_MAIN

// Testing of resources
MacAnovaApp::MacAnovaApp()
{
}

#ifdef MSDOS
#define SEP '\\'
#else /*MSDOS*/
#define SEP '/'
#endif /*MSDOS*/

#define SHIFT (++argv, --argc)

#define MEMGUBED   (4096)

static char    *ProgName;

static char     UsageMsg[] = 
"Usage: macanova [options]\n\n\
Options are:\n\
  -q                   Suppress printing banner\n\
  -batch batchFile     Simulate batch(\"batchFile\")\n\
  -restore restoreFile Simulate restore(\"restoreFile\")\n\
  -f startupFile       Use startupFile instead of .macanova.ini\n\
  -prompt Prompt       Use Prompt as input prompt\n\
  -bprompt Prompt      Use Prompt as batch prompt (with -batch)\n\
  -e Expression        Evaluate Expression before doing anything else\n"
#ifndef SCROLLABLEWINDOW
"  -eq Expression       Evaluate Expression and quit (not with -batch)\n"
#endif /*SCROLLABLEWINDOW*/
#ifdef wx_motif
"  -plotdelay m         Delay m milliseconds before opening graph window\n"
#endif /*wx_motif*/
"  -help helpFile       Use helpFile as default file for help information\n\
  -macro macroFile     Add \"macroFile\" to variable MACROFILES\n\
  -data dataFile       Set variable DATAFILE to \"dataFile\"\n\
  -dpath dataPath      Set variable DATAPATHS to \"dataPath\"\n\
  -mpath macroPath     Add \"macroPath\" to variable DATAPATHS\n\
  -home homePath       Set variable HOME to \"homePath\"\n\
  -f and -restore should not both be specified";


static void usage(void)
{
	(void) wxMessageBox(UsageMsg, "MacAnova Usage", wxOK|wxLeft);
} /*usage()*/


/*
   960502 changed fopen() to fmyopen()
   980329 added argument prompt

   980401 replaced with copy of version in main.c
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

	doRestore appears in macMain.c in mac version; the ifdefs are included
	to make the code identical
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

	restoreFile = fmyopen(fileName,"r");
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


#define FROMENV(N) ((N) >= buffer && (N) < buffer + ENVBUFLENGTH)

#ifndef ENVNAME
#define ENVNAME "MACANOVA"
#endif /*ENVNAME*/

MacAnovaCommandLineArgs::MacAnovaCommandLineArgs(void)
{
	initFileName = (char *) 0;
	batchFileName = (char *) 0;
	restoreFileName = (char *) 0;
	helpFileName = (char *) 0;
	dataFileName = (char *) 0;
	macroFileName = (char *) 0;
	dataPathName = (char *) 0;
	macroPathName = (char *) 0;
	commandPrompt[0] = '\0';
	batchPrompt[0] = '\0';
#ifdef UNIX
	homePathName = getenv("HOME");;
#else /*UNIX*/
	homePathName = (char *) 0;
#endif /*UNIX*/
	nlines = -1;
	ncols = -1;
	expression = (char *) 0;
#ifndef SCROLLABLEWINDOW
	expressionQ = (char *) 0;
#endif /*SCROLLABLEWINDOW*/
#ifdef wx_motif
	plotDelay = -1;
#endif /*wx_motif*/
#ifdef SAVEHISTORY
	historyLength = -1;
#endif /*SAVEHISTORY*/
	quiet = 0;
} /*MacAnovaCommandLineArgs::MacAnovaCommandLineArgs()*/

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
	myAlert(OUTSTR);
	*OUTSTR = '\0';
} /*badCommandLineFile()*/

MacAnovaCommandLineArgs * MacAnovaApp::ProcessArgs(void)
{
	MacAnovaCommandLineArgs *lineArgs  = new MacAnovaCommandLineArgs;
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
	char         *tmp;
	long          screenHeight, screenWidth;
#ifdef SAVEHISTORY
	char         *historyLengthStr = (char *) 0;
#endif /*SAVEHISTORY*/
#ifdef wx_motif
	char         *plotDelayStr = (char *) 0;
#endif /*wx_motif*/
	WHERE("ProcessArgs");
	
	OUTSTR[0] = '\0';
	sprintf(fromEnv," (from env. variable %s)", envName);
	
	for (ProgName = *argv;*ProgName;ProgName++)
	{
		;
	}
#ifdef MSDOS
	while (--ProgName >= *argv && *ProgName != SEP && *ProgName != ':')
	{
		;
	}
#else /*MSDOS*/
	while (--ProgName >= *argv && *ProgName != SEP)
	{
		;
	}
#endif /*MSDOS*/
	ProgName++;
	
	if (macEnv != (char *) 0)
	{
		char      *dest = buffer, *src = macEnv;
		
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
#ifdef wx_motif
				else if (strcmp(cmd,"-plotdelay") == 0)
				{
					plotDelayStr = *argv;
				}
#endif /*wx_motif*/
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
			if (i == 1 && (**argv == '<' || **argv == '>'))
			{
				sprintf(OUTSTR,
						"This version of MacAnova does not process redirected input or output;\nUse a non-windowed version");
				goto errorExit;
			} /*if (i == 1 && (**argv == '<' || **argv == '>'))*/
			goto parseError;
		} /*if (argc > 0)*/
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
			putOUTSTR();
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
			putOUTSTR();
			lineArgs->batchPrompt[0] = 0;
		}
		else if (strlen(bPrompt) >= MAXPROMPT)
		{
			sprintf(OUTSTR,
					"WARNING: batch prompt '%s' longer than %d characters; truncated",
					bPrompt, MAXPROMPT - 1);
			putOUTSTR();
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

#ifndef SCROLLABLEWINDOW
	if (expressionQ != (char *) 0)
	{
		if (batchFileName != (char *) 0    ||
			expression != (char *) 0)
		{
			sprintf(OUTSTR, "ERROR: -eq is incompatible with -e and -batch");
			goto errorExit;
		}
		quiet = 1;
		if (nlines < 0)
		{
			nlines = 0;
		}
	} /*if (expressionQ != (char *) 0)*/
#endif /*SCROLLABLEWINDOW*/

#ifdef wx_motif
	if (plotDelayStr != (char *) 0)
	{
		long          plotdelay = 0;

		tmp = plotDelayStr;
		while (*tmp == ' ')
		{
			tmp++;
		}
		while (*tmp && isdigit(*tmp))
		{
			plotdelay = 10*plotdelay + (*tmp++) - '0';
		}
		if (*tmp != '\0' || tmp == plotDelayStr ||
		   plotdelay < 0)
		{
			sprintf(OUTSTR,
					"ERROR: value '%s' for -plotdelay not integer >= 0",
					plotDelayStr);
			goto errorExit;
		} /*if (*tmp != '\0' || tmp == plotDelayStr || plotdelay < 0)*/
		lineArgs->plotDelay = plotdelay;
	}
#endif /*wx_motif*/

#ifdef SAVEHISTORY
	if (historyLengthStr != (char *) 0)
	{
		long          history = 0;

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

	return (lineArgs);

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
	if (*OUTSTR)
	{
		myAlert(OUTSTR);
		*OUTSTR = '\0';
	}
	return ((MacAnovaCommandLineArgs *) 0);
} /*MacAnovaApp::ProcessArgs()*/

#if (0)
MacAnovaCommandLineArgs * MacAnovaApp::ProcessArgs(void)
{
	char            *cmd;

	MacAnovaCommandLineArgs *lineArgs  = new MacAnovaCommandLineArgs;
	MacAnovaCommandLineArgs *retval = lineArgs;
	
	for (ProgName = *argv;*ProgName;ProgName++)
	{
		;
	}
#ifdef MSDOS
	while (--ProgName >= *argv && *ProgName != SEP && *ProgName != ':')
	{
		;
	}
#else /*MSDOS*/
	while (--ProgName >= *argv && *ProgName != SEP)
	{
		;
	}
#endif /*MSDOS*/
	ProgName++;
	while (SHIFT && **argv == '-')
	{
		cmd = *argv;
		if (strcmp(cmd,"-q") == 0)
		{
			lineArgs->quiet = 1;
		} /*if (strcmp(cmd,"-q") == 0)*/
		else
		{
			SHIFT;
			if (argc == 0)
			{
				usage();
				return NULL;
			}
			
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
			else
			{
				usage();
				return NULL;
			}
		} /*if (strcmp(cmd,"-q") == 0){}else{}*/
	} /*while (SHIFT && **argv == '-')*/

	if (argc > 0 && (**argv == '<' || **argv == '>'))
	{
		myAlert("This version of MacAnova does not process redirected input or output;\nUse a non-windowed version");
		return(0);
	}
	if (argc > 0)
	{
#if (1)
		usage();
#else /*1*/
		myAlert(UsageMsg);
#endif /*1*/
		return NULL;
	}
	
	/* Check file and path names for legality */
	if (lineArgs->helpFileName != (char *) 0 &&
		!okfilename(lineArgs->helpFileName))
	{
		badCommandLineFile(lineArgs->helpFileName, "file", "help file");
		retval = (MacAnovaCommandLineArgs *) 0;
	}

	if (lineArgs->dataFileName != (char *) 0 &&
		!okfilename(lineArgs->dataFileName))
	{
		badCommandLineFile(lineArgs->dataFileName, "file", "data file");
		retval = (MacAnovaCommandLineArgs *) 0;
	}
	
	if (lineArgs->macroFileName != (char *) 0 &&
		!okfilename(lineArgs->macroFileName))
	{
		badCommandLineFile(lineArgs->macroFileName, "file", "macro file");
		retval = (MacAnovaCommandLineArgs *) 0;
	} /*if (macroFileName != (char *) 0 && !okfilename(macroFileName))*/
	
	if (lineArgs->restoreFileName != (char *) 0 &&
		!okfilename(lineArgs->restoreFileName))
	{
		badCommandLineFile(lineArgs->restoreFileName, "file", "restore file");
		retval = (MacAnovaCommandLineArgs *) 0;
	} /*if (restoreFileName != (char *) 0 && !okfilename(restoreFileName))*/
	
	if (lineArgs->initFileName != (char *) 0 &&
		!okfilename(lineArgs->initFileName))
	{
		badCommandLineFile(lineArgs->initFileName, "file",
						   "initialization file");
		retval = (MacAnovaCommandLineArgs *) 0;
	} /*if (initFileName != (char *) 0 && !okfilename(initFileName))*/
	
	if (lineArgs->batchFileName != (char *) 0 &&
		!okfilename(lineArgs->batchFileName))
	{
		badCommandLineFile(lineArgs->batchFileName, "file", "batch file");
		retval = (MacAnovaCommandLineArgs *) 0;
	} /*if (batchFileName != (char *) 0 && !okfilename(batchFileName))*/
	
	if (lineArgs->dataPathName != (char *) 0 && !okpathname(lineArgs->dataPathName))
	{
		badCommandLineFile(lineArgs->dataPathName, "path", "-dpath");
		retval = (MacAnovaCommandLineArgs *) 0;
	} /*if (lineArgs->dataPathName != (char *) 0 && !okpathname(lineArgs->dataPathName))*/

	if (lineArgs->macroPathName != (char *) 0 && !okpathname(lineArgs->macroPathName))
	{
		badCommandLineFile(lineArgs->macroPathName, "path", "-mpath");
		retval = (MacAnovaCommandLineArgs *) 0;
	} /*if (lineArgs->macroPathName != (char *) 0 && !okpathname(lineArgs->macroPathName))*/

	if (lineArgs->homePathName != (char *) 0 &&
		!okpathname(lineArgs->homePathName))
	{
		badCommandLineFile(lineArgs->homePathName, "path", "-home");
		retval = (MacAnovaCommandLineArgs *) 0;
	} /*if (homePathName != (char *) 0 && !okpathname(homePathName))*/
	
	return(retval);
} /*MacAnovaApp::ProcessArgs()*/
#endif /*0*/


/*
   The `main program' equivalent, creating the windows and returning the
   main frame

   980401 added code to set default paper size.
*/
wxFrame *MacAnovaApp::OnInit(void)
{

#ifdef UNIX
	char            *home, **tempHandle = (char **) 0;
#endif /*UNIX*/
#ifdef MACANOVAINI
	char            *rcFileName  = MACANOVAINI;
#else /*MACANOVAINI*/
	char            *rcFileName = (char *) 0;
#endif /*MACANOVAINI*/
	long             screenwidth = -1, screenheight = -1;
#ifdef wx_motif
	long             plotdelay;
#endif /*wx_motif*/
#ifdef SAVEHISTORY
	int              history;
#endif /*SAVEHISTORY*/
	FILE            *batchFile;

	MacAnovaCommandLineArgs *lineArgs;

#if defined(wx_msw)
	if (IS_NT)
	{
		ThisMachine = mvWindowsNT;
	}
	else if (IS_WIN95)
	{
		ThisMachine = mvWindows95;
	}
	else
	{
		ThisMachine = mvWin32s;
	}
#elif defined(wx_motif)
#if defined(UNIX)
	ThisMachine = mvMotif;
#elif defined(VMS)
	ThisMachine = mvMotifVMS;
#endif /*UNIX*/
#endif /*wx_msw*/

	set_new_handler(0); // 960826 return NULL on "new" failure

	myInitHandles(); // initializes memory management

	// Create the main frame window

#if (0) /* activate if macro FPRINT is to be used for debugging */
#ifndef DBUGFILENAME
#define DBUGFILENAME "dbug.out" /* or whatever ... */
#endif /*DBUGFILENAME*/
#ifdef wx_msw
#define DBUGMODE "wt"
#else /*wx_msw*/
#define DBUGMODE "w"
#endif /*wx_msw*/
	DBFILE = fopen(DBUGFILENAME,DBUGMODE);
#endif /*0*/
#ifdef wx_msw
	int         screenHeight, screenWidth;

	// Tailor sizes to display size
	::wxDisplaySize(&screenWidth, &screenHeight);

	if (screenWidth < 800)
	{
		DefaultTextWindowWidth = (8*DEFAULTTEXTWINDOWWIDTH)/9;
		DefaultTextWindowHeight = (8*DEFAULTTEXTWINDOWHEIGHT)/9;
		DefaultTextFontSize = DEFAULTTEXTFONTSIZE-1;
	}
	else
	{
		DefaultTextWindowWidth = DEFAULTTEXTWINDOWWIDTH;
		DefaultTextWindowHeight = DEFAULTTEXTWINDOWHEIGHT;
		DefaultTextFontSize = DEFAULTTEXTFONTSIZE;
	}
	DefaultCanvasWindowWidth = (9*DefaultTextWindowWidth)/10;
	DefaultCanvasWindowHeight = (9*DefaultTextWindowHeight)/10;
	DefaultCanvasFontSize = DefaultTextFontSize;
#endif /*wx_msw*/

#define DEFAULTBASEFRAMEWIDTH  165
#define DEFAULTBASEFRAMEHEIGHT 150

	BaseFrame = new MacAnovaBaseFrame(NULL, "MacAnovaWX", 0, 0,
				DEFAULTBASEFRAMEWIDTH, DEFAULTBASEFRAMEHEIGHT);

	BaseFrame->OnSize(DEFAULTBASEFRAMEWIDTH, DEFAULTBASEFRAMEHEIGHT);

	// make the first text window (untitled)
	BaseFrame->NewTextFrame(TRUE,FALSE); // add prompt after banner
	BaseFrame->RunningFrame = 0;
	BaseFrame->RunningWindow = 
	  BaseFrame->TextFrames[BaseFrame->RunningFrame]->text_window;
	BaseFrame->GraphicsPause = 0;

	The_timer = new MacAnovaMainTimer;

#ifdef USETICKTIMER
	The_TickTimer = new MacAnovaTickTimer;
	The_TickTimer->Start(MilliSecondsPerTick);
#endif /*USETICKTIMER*/

#if defined(wx_motif) && !defined(USEA4PAPER)
	/* set Letter paper as default */
	wxThePrintSetupData->SetPaperName("Letter 8 1/2 x 11 in");
#endif /*defined(wx_motif) && !defined(USEA4PAPER)*/
	
	// set some cursors
	The_watch_cursor = new wxCursor(wxCURSOR_WATCH);
	The_arrow_cursor = new wxCursor(wxCURSOR_ARROW);
	The_ibeam_cursor = new wxCursor(wxCURSOR_IBEAM);
	
	// try to hide it
	BaseFrame->Show(FALSE);
	BaseFrame->TextFrames[0]->Show(TRUE);
	
	INTERRUPT = INTNOTSET;
	
	/* allocate buffer for building messages and other strings*/
	OUTSTR = mygetpointer(BUFFERLENGTH * sizeof(char));
	if (OUTSTR == (char *) 0)
	{
		goto notAllocated;
	}
	
	lineArgs = ProcessArgs();
	if (lineArgs == NULL)
	{
#ifdef wx_msw
		ExitProcess(0);
#endif //wx_msw
		return(0);
	}
	screenheight = lineArgs->nlines;
	screenwidth = lineArgs->ncols;

#ifdef wx_motif
	plotdelay = lineArgs->plotDelay;
	if (plotdelay >= 0)
	{
		DefaultFocusDelay = plotdelay;
	}
#endif /*wx_motif*/

#ifdef SAVEHISTORY
	history = lineArgs->historyLength;
#endif /*SAVEHISTORY*/

	ISATTY = ITTYIN | ITTYOUT; /* Both input and output just for safety */
	
#ifdef wx_motif
	if (!isatty(0) || !isatty(1))
	{
		myAlert("This version of MacAnova does not process redirected input or output;\nUse a non-windowed version");
		return(0);
	}
#endif // wx_motif
	UseWindows = 1;
	
	/*
	   Initialize symbol table
	   Note: UseWindows == 0 at this point suppressing any attempt to
	   use a window that has not yet been created
	   */
	initialize(lineArgs->helpFileName,
			   lineArgs->dataFileName,
			   lineArgs->macroFileName,
			   lineArgs->dataPathName,
			   lineArgs->macroPathName,
			   lineArgs->homePathName,
			   lineArgs->commandPrompt);

	if (FatalError)
	{
		goto notAllocated;
	}
	
	screenheight = (screenheight < 0) ? SCREENHEIGHT : screenheight;
	screenwidth = (screenwidth < 0) ? SCREENWIDTH : screenwidth;
	installScreenDims(screenwidth, screenheight);
	saveFormat();


#ifdef SAVEHISTORY
	history = (history < 0) ? DEFAULTHISTORYLENGTH : history;
#endif /*SAVEHISTORY*/

	startupMsg(lineArgs->quiet);

	/* try to open initialization and help files */

	/* make sure help file is found if there */
#ifndef BINARYHELPFILE
	HELPFILE = fopen(*HELPFILENAME,"r");
#else /*BINARYHELPFILE*/
	HELPFILE = fopen(*HELPFILENAME,"rb"); /* open in binary mode */
#endif /*BINARYHELPFILE*/
	
	if (!lineArgs->quiet && HELPFILE != (FILE *) 0 &&
	   (LINE = mygethandle(MAXLINE)) != (char **) 0)
	{
		/* look for message of the day preceded and followed by "!!!!"*/
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
				if (printit)
				{
					break;
				}
				else
				{
					printit = 1;
					continue;
				}
			} /*if (strncmp(msgStart, marker, MOTDMARKERLENGTH) == 0)*/
			
			if (printit)
			{
				while (*msgStart == ' ')
				{
					msgStart++;
				}
				msgEnd = msgStart + strlen(msgStart) - 1;
				while (*msgEnd == ' ' && msgEnd > msgStart)
				{
					msgEnd--;
				}
				if (msgEnd[0] != '\n')
				{
					msgEnd++;
				}
				*msgEnd = '\0';
				
				(void) centerBuffer(OUTSTR, msgStart, screenwidth);
				if (printit == 1)
				{
					myeol();
					printit = 2;
				}
				putOUTSTR();
			} /*if (printit)*/
		} /*while (fillLINE(HELPFILE) != EOF)*/
		rewind(HELPFILE);
		mydisphandle(LINE);
		LINE = (char **) 0;
	}
	
	if (lineArgs->restoreFileName != (char *) 0)
	{
		if (doRestore(lineArgs->restoreFileName))
		{
			if (PRINTWARNINGS)
			{
				putPieces("WARNING: cannot restore workspace from file ",
						  lineArgs->restoreFileName, (char *) 0, (char *) 0);
			}				
			lineArgs->restoreFileName = (char *) 0;
		} /*if (doRestore(lineArgs->restoreFileName))*/
		else
		{
			if (lineArgs->initFileName != (char *) 0)
			{
				putOutErrorMsg("WARNING: cannot use -r and -f options together; -f ignored");
			}
			lineArgs->initFileName = rcFileName = (char *) 0;
		} /*if (doRestore(lineArgs->restoreFileName)){}else{}*/
	} /*if (lineArgs->restoreFileName != (char *) 0)*/
	
	
	/*
	   If -f initFileName not specified use ~/rcFileName, if it exists
	   */
	
	if (lineArgs->initFileName == (char *) 0 && rcFileName != (char *) 0)
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
			lineArgs->initFileName = *tempHandle;
		} /*if ((home = getenv("HOME")))*/
		else
#endif /*UNIX*/
		{
			lineArgs->initFileName = rcFileName;
		}
	} /*if (lineArgs->initFileName == (char *) 0 && rcFileName != (char *) 0)*/
	
	if (lineArgs->expression != (char *) 0)
	{
		myeol();
		sprintf(OUTSTR, "Command Line> %s", lineArgs->expression);
		putOUTSTR();

		Symbolhandle  result = mvEval(&lineArgs->expression);
		if (result == (Symbolhandle) 0)
		{
			sprintf(OUTSTR,
					"FATAL ERROR: \"-e %s\" resulted in error\"",
					lineArgs->expression);
			goto errorExit;
		}
	} /*if (expression != 0*/

	/*
	   NOTE: if batchFileName has been specified, it is actually executed *after*
	   initFileName, even though it appears to be invoked first.  It is as
	   if batch(initFileName,echo:F) were the first line of batchFileName.
	   */
	if (lineArgs->batchFileName != (char *) 0)
	{
		batchFile = fmyopen(lineArgs->batchFileName, TEXTREADMODE);
		if (batchFile != (FILE *) 0)
		{
			fclose(batchFile);
			(void) doBatch(lineArgs->batchFileName,1,
						   (char *) lineArgs->batchPrompt);
		}
		else if (PRINTWARNINGS)
		{
			myprint("WARNING: Unable to open batch file ");
			putOutErrorMsg(lineArgs->batchFileName);
		}
	} /*if (lineArgs->batchFileName != (char *) 0)*/
	if (lineArgs->initFileName != (char *) 0)
	{ /* no warning needed here since macanova.ini file is optional */
		(void) doBatch(lineArgs->initFileName,0, NullString);
	}
#ifdef UNIX
	mydisphandle(tempHandle);
#endif /*UNIX*/
	
#ifdef SAVEHISTORY
	initHistory(history);
#endif /*SAVEHISTORY*/

	BaseFrame->PutPrompt((char *) 0,0);
	
	The_timer->Start(MACANOVATIMERINCREMENT); // timer to make us check for input
	wxStartTimer(); // misc timer
	
	// Return the main frame window
	return BaseFrame;
	
  notAllocated:
	strcpy(OUTSTR,
		   "FATAL ERROR: unable to allocate memory during inialization sequence");
	/* fall through*/

  errorExit:
	if (*OUTSTR)
	{
		myAlert(OUTSTR);
	}
	BaseFrame->DoQuit(MUSTQUIT);
	return(0);
	
} /*MacAnovaApp::OnInit()*/

// Define my frame constructor
MacAnovaBaseFrame::MacAnovaBaseFrame(wxFrame *frame, Const char *title,
									 int x, int y, int w, int h):
#ifdef wx_msw
wxFrame(frame, title, x, y, w, h, wxDEFAULT_FRAME | wxICONIZE)
#else // wx_msw
wxFrame(frame, title, x, y, w, h)
#endif //wx_msw
{
	// set up a standard frame and initialize the Text- and Canvas-
	// Frames to NULL.  
	int i;
	for (i=0;i<NUMTEXTFRAMES;i++)
	{
		TextFrames[i] = NULL;
	}
	for (i=0;i<NUMCANVASFRAMES;i++)
	{
		CanvasFrames[i] = NULL;
	}
	numTextFrames = 0;
	untitled_sequence = 1;
	numCanvasFrames = 0;

#ifdef wx_motif
	// initialize printer info
	TextPrintSetupData.orientation     = PS_PORTRAIT;               
	TextPrintSetupData.mode            = PS_PRINTER;                
	defaultTextPrintSetupData(PS_PORTRAIT);
	defaultTextPrintSetupData(PS_LANDSCAPE);

	GraphPrintSetupData.orientation    = PS_LANDSCAPE;               
	GraphPrintSetupData.mode           = PS_PRINTER;                
	defaultGraphPrintSetupData(PS_PORTRAIT);
	defaultGraphPrintSetupData(PS_LANDSCAPE);
#endif /*wx_motif*/
	Running = FALSE;
} /*MacAnovaBaseFrame::MacAnovaBaseFrame()*/

MacAnovaBaseFrame::~MacAnovaBaseFrame(void)
{
	long            i;
	
	for (i = numTextFrames-1;i >= 0;i--)
	{
		MacAnovaTextFrame     *tf = TextFrames[i];
		CutTextFrame(i);
		delete tf;
	} /*for (i=numTextFrames-1;i>=0;i--)*/
	
	for (i=0;i<NUMCANVASFRAMES;i++)
	 {
		if ( CanvasFrames[i] != NULL)
		{
			delete CanvasFrames[i];
		}
	} /*for (i=0;i<NUMCANVASFRAMES;i++)*/

} /*MacAnovaBaseFrame::~MacAnovaBaseFrame()*/

#ifdef wx_motif
void MacAnovaBaseFrame::defaultTextPrintSetupData(int orientation)
{
	if (orientation == PS_PORTRAIT)
	{
		TextPrintSetupData.scaleX[0]       = TEXTPORTRAITSCALING;       
		TextPrintSetupData.scaleY[0]       = TEXTPORTRAITSCALING;       
		TextPrintSetupData.translateX[0]   = TEXTPORTRAITTRANSLATIONX;  
		TextPrintSetupData.translateY[0]   = TEXTPORTRAITTRANSLATIONY;  
	}
	else
	{
		TextPrintSetupData.scaleX[1]       = TEXTLANDSCAPESCALING;       
		TextPrintSetupData.scaleY[1]       = TEXTLANDSCAPESCALING;       
		TextPrintSetupData.translateX[1]   = TEXTLANDSCAPETRANSLATIONX;  
		TextPrintSetupData.translateY[1]   = TEXTLANDSCAPETRANSLATIONY;
	}
} /*MacAnovaBaseFrame::defaultTextPrintSetupData()*/

void MacAnovaBaseFrame::defaultGraphPrintSetupData(int orientation)
{
	if (orientation == PS_PORTRAIT)
	{
		GraphPrintSetupData.scaleX[0]       = GRAPHPORTRAITSCALING;       
		GraphPrintSetupData.scaleY[0]       = GRAPHPORTRAITSCALING;       
		GraphPrintSetupData.translateX[0]   = GRAPHPORTRAITTRANSLATIONX;  
		GraphPrintSetupData.translateY[0]   = GRAPHPORTRAITTRANSLATIONY;  
	}
	else
	{
		GraphPrintSetupData.scaleX[1]       = GRAPHLANDSCAPESCALING;       
		GraphPrintSetupData.scaleY[1]       = GRAPHLANDSCAPESCALING;       
		GraphPrintSetupData.translateX[1]   = GRAPHLANDSCAPETRANSLATIONX;  
		GraphPrintSetupData.translateY[1]   = GRAPHLANDSCAPETRANSLATIONY;
	}
} /*MacAnovaBaseFrame::defaultGraphPrintSetupData()*/
#endif /*wx_motif*/


void MacAnovaBaseFrame::NewTextFrame(Bool untitled, Bool initialPrompt)
{
	char         buffer[50];
	int          xoffset = 0, yoffset = 0;
	int          windno = numTextFrames;
	
	// create a new text frame if there are any slots left
	
	if (numTextFrames == NUMTEXTFRAMES)
	{
		(void) wxMessageBox("Too many text windows open; close one",
						   "MacAnova -- Too many windows", wxOK|wxCENTRE);
		return;
	}

#if (0)
	if (GUBED & 8)
	{
		Symbolhandle    symh = Lookup("FONTSIZE");
		if (symh != (Symbolhandle) 0 && TYPE(symh) == REAL &&
			isScalar(symh))
		{
			DefaultTextFontSize = (int) DATAVALUE(symh, 0);
		}
		symh = Lookup("OFFSETS");
		if (symh != (Symbolhandle) 0 && TYPE(symh) == REAL &&
			symbolSize(symh) == 2)
		{
			xoffset = (int) DATAVALUE(symh, 0);
			yoffset = (int) DATAVALUE(symh, 1);
		}
		symh = Lookup("WINDOWSIZE");
		if (symh != (Symbolhandle) 0 && TYPE(symh) == REAL &&
			symbolSize(symh) == 2)
		{
			DefaultTextWindowWidth = (int) DATAVALUE(symh, 0);
			DefaultTextWindowHeight = (int) DATAVALUE(symh, 1);
		}
		symh = Lookup("GRAPHSIZE");
		if (symh != (Symbolhandle) 0 && TYPE(symh) == REAL &&
			symbolSize(symh) == 2)
		{
			DefaultCanvasWindowWidth = (int) DATAVALUE(symh, 0);
			DefaultCanvasWindowHeight = (int) DATAVALUE(symh, 1);
		}
	} /*if (GUBED & 8)*/
#endif /*0*/
	
#ifdef wx_msw
	xoffset += 5*(numTextFrames);
	yoffset += 15*(numTextFrames);
#endif /*wx_msw*/
	// if an untitled frame, give it the name untitled

	if (untitled)
	{
		sprintf(buffer,"Untitled %d",untitled_sequence);
		untitled_sequence++;
		TextFrames[windno] =
			new MacAnovaTextFrame(NULL, buffer, xoffset, yoffset,
								  DefaultTextWindowWidth,
								  DefaultTextWindowHeight);
		TextFrames[windno]->Untitled = TRUE;
	} /*if (untitled)*/
	else
	{
		// not untitled, so prompt for file name
		char     *s = wxFileSelector("Load text file", NULL, NULL, NULL, "*");
		if (s)
		{
			TextFrames[windno] =
				new MacAnovaTextFrame(NULL, s, xoffset, yoffset,
									  DefaultTextWindowWidth,
									  DefaultTextWindowHeight);
			MacAnovaTextWindow  *mtw = TextFrames[windno]->text_window;
			
			mtw->LoadFile(s);
			TextFrames[windno]->Untitled = FALSE;
#ifdef wx_motif

			long       pos = mtw->GetLastPosition();
			long       limit = CMDWINDOWLIMIT - 1000;
			long       truncated = (pos > limit) ? pos - limit : 0;

			if (truncated)
			{
				sprintf(OUTSTR,
						"WARNING: file too long; only last %ld characters loaded",
						pos - truncated);
				myAlert(OUTSTR);
				putOUTSTR();
				TextFrames[windno]->text_window->Remove(0,truncated);
			} /*if (truncated)*/
#endif /*wx_motif*/
		}
		else
		{
			return;
		}
	} /*if (untitled){}else{}*/

	numTextFrames++;
	TextFrames[windno]->TextFrameIndex = windno;
	RunningFrame = windno;
	RunningWindow = TextFrames[RunningFrame]->text_window;
	if (initialPrompt)
	{
		TextFrames[windno]->text_window->PutPrompt((char *) 0);
	}
	
	ResetAllWindowsMenus();
	BaseFrame->EnableAllTextWindowMenus(MACANOVA_WINDOWS_HIDE,
										BaseFrame->NVisible() > 1);
	
} /*MacAnovaBaseFrame::NewTextFrame()*/

void MacAnovaBaseFrame::CutTextFrame(int frame_index)
{
	int i;
	
	// If the frame index is in range, cut the text frame
	// from the list, shuffle any higher indexed frames down
	// decrement the frame counter and tell text frames to
	// update their window menus.
	if (frame_index >= 0 && frame_index < numTextFrames)
	{
		
		for (i=frame_index+1;i<numTextFrames;i++)
		{
			TextFrames[i-1] = TextFrames[i];
			TextFrames[i-1]->TextFrameIndex = i-1;
		}
		
		
		numTextFrames--;
		TextFrames[numTextFrames] = (MacAnovaTextFrame *) 0;
		ResetAllWindowsMenus();
	}

	if (RunningFrame >= frame_index && RunningFrame > 0)
	{
		RunningFrame--;
		RunningWindow = TextFrames[RunningFrame]->text_window;
	}
	
} /*MacAnovaBaseFrame::CutTextFrame()*/


int MacAnovaBaseFrame::NewCanvasFrame(void)
{
	// set up a new canvas frame, if needed
	
	char buffer[50];
	WHERE("NewCanvasFrame");
	// global ThisWindow (to conform with mac version) has
	// the index of the frame to use; if it is negative, find
	// a new frame
	
	if (numCanvasFrames == NUMCANVASFRAMES && ThisWindow < 0)
	{ // ThisWindow < 0 indicates new window
		(void) wxMessageBox("Too many graphics windows open; close one",
						   "MacAnova -- Too many windows", wxOK|wxCENTRE);
		return(-1);
	}
	
	if (ThisWindow > NUMCANVASFRAMES)
	{
		sprintf(buffer,
				"At most %d graphics windows are allowed",NUMCANVASFRAMES);
		(void) wxMessageBox(buffer, "MacAnova", wxOK|wxCENTRE);
		return(-1);
	}
	
	if (ThisWindow >= 0 && CanvasFrames[ThisWindow] != NULL)
	{ // the previous window still exists
		
		CanvasFrames[ThisWindow]->ResetCanvas();
		return(ThisWindow);
	}
	
	// if we get here we need a new frame
	if (ThisWindow < 0)
	{
		for (ThisWindow = 0;ThisWindow < NUMCANVASFRAMES;ThisWindow++)
		{
			if (CanvasFrames[ThisWindow] == NULL)
			{
				break;
			}
		}
	} /*if (ThisWindow < 0)*/
	
	
	// make the new frame
	sprintf(buffer,"Graph %ld",ThisWindow+1);
#if (0)
	// superceded kludge to try to fix problem
	// see MacAnovaCanvasFrame:MacAnovaCanvasFrame() in wxgraph.cc
	DefaultCanvasWindowWidth = -abs(DefaultCanvasWindowWidth);
	DefaultCanvasWindowHeight = -abs(DefaultCanvasWindowHeight);
#endif /*0*/
/* 970915 added xoffset and yoffset to position graph window under Windows*/
	int       xoffset = 20, yoffset = 20;

#ifdef wx_msw
	xoffset += ThisWindow*5;
	yoffset += ThisWindow*15;
#endif /*wx_motif*/
	CanvasFrames[ThisWindow] =
		new MacAnovaCanvasFrame(NULL, buffer,xoffset,yoffset,
								DefaultCanvasWindowWidth,
								DefaultCanvasWindowHeight);
	CanvasFrames[ThisWindow]->CanvasFrameIndex = ThisWindow;
	numCanvasFrames++;

	ResetAllWindowsMenus();

	CanvasFrames[ThisWindow]->ResetCanvas();

	return(ThisWindow);
} /*MacAnovaBaseFrame::NewCanvasFrame()*/

/*
	 If the frame index is in range, cut the canvas frame
	 from the list
	 decrement the frame counter and tell frames to
	 update their window menus.
*/
void MacAnovaBaseFrame::CutCanvasFrame(int frame_index)
{
	if (frame_index >= 0 && frame_index < NUMCANVASFRAMES)
	{
		CanvasFrames[frame_index] = NULL;
		numCanvasFrames--;
		ResetAllWindowsMenus();
	}
	
} /*MacAnovaBaseFrame::CutCanvasFrame()*/

/*
	  Loop through all text frames telling them to
	  reset their window menus
*/
void MacAnovaBaseFrame::ResetAllWindowsMenus(void)
{
	int          i;

	for (i=0;i<numTextFrames;i++)
	{
		TextFrames[i]->ResetWindowsMenu();
	}
	for (i=0;i<NUMCANVASFRAMES;i++)
	{
		if (CanvasFrames[i] != NULL)
		{
			CanvasFrames[i]->ResetWindowsMenu();
		}
	}
} /*MacAnovaBaseFrame::ResetAllWindowsMenus()*/

void MacAnovaBaseFrame::EnableAllTextWindowMenus(int id, Bool flag)
{
	int          i;
	
	for (i=0;i < numTextFrames;i++)
	{
		TextFrames[i]->GetMenuBar()->Enable(id, flag);
	}
} /*MacAnovaBaseFrame::EnableAllTextWindowMenus()*/


/*
	 Try to quit MacAnova; usually called from Quit
	 menu option, or possibly by the quit button on the
	 base frame window.
	 Check if we can close text frames; stop if any 
	 cannot be closed.

	 971117 deleting of stuff is done in destructors
*/

void MacAnovaBaseFrame::DoQuit(int mustQuit)
{
	int           i;
	int           decision;
	WHERE("MacAnovaBaseFrame::DoQuit");

	The_timer->Stop();
#ifdef wx_msw
	mustQuit = mustQuit || (GetKeyState(VK_MENU) & 0x80);
#endif /*wx_msw*/

	closeBatch(1); /* close all batch files */
	if (!mustQuit && SaveOnQuit)
    {
		decision = wxMessageBox("Do you wish to save your Workspace?",
								"Save Workspace?",
								wxYES_NO | wxCANCEL | wxCENTRE) ;
		if ( decision == wxCANCEL )
    	{
			The_timer->Start(MACANOVATIMERINCREMENT);
			return;
		}
    	else if ( decision == wxYES)
		{
			SaveWorkspace(TRUE);
		}

		for (i = numTextFrames-1;i >= 0;i--)
		{
			if (!TextFrames[i]->OKToClose())
			{
				The_timer->Start(MACANOVATIMERINCREMENT);
				return;
			}
		} /*for (i=numTextFrames-1;i>=0;i--)*/
	} /*if (!mustQuit && SaveOnQuit)*/

#if (0) /* now done by destructors */
	/*
	  If we got here, delete all text frames
	*/
	for (i = numTextFrames-1;i >= 0;i--)
	{
		MacAnovaTextFrame     *tf = TextFrames[i];

		delete [] tf->text_window->UndoBuffer;
		delete tf->text_window;
		tf->text_window = NULL;
		CutTextFrame(i);
		delete tf;
	} /*for (i=numTextFrames-1;i>=0;i--)*/
	
	for (i=0;i<NUMCANVASFRAMES;i++)
	 {
		if ( CanvasFrames[i] != NULL)
		{
			delete CanvasFrames[i];
		}
	} /*for (i=0;i<NUMCANVASFRAMES;i++)*/
#endif /*0*/

	if (DBFILE != (FILE *) 0)
	{
		fclose(DBFILE);
	}
	if (HELPFILE != (FILE *) 0)
	{
		fclose(HELPFILE);
	}
	closeBatch(1);

#if (0)
	myDispAll(); // release all MacAnova handles
#endif /*0*/

	delete The_timer;
	delete The_watch_cursor;
	delete The_arrow_cursor;
	delete The_ibeam_cursor;

#ifdef wx_msw
#if (0) /*deleting this seems to cause crashes*/
	delete this;
#endif /*0*/
	ExitProcess(0);
#endif //wx_msw

	delete this;
#if (0)
	return(TRUE);
#endif /*0*/
} /*MacAnovaBaseFrame::DoQuit()*/



// Define the behaviour for the frame closing
Bool MacAnovaBaseFrame::OnClose(void)
{
	// if we hit the close button on the main window,
	// just call DoQuit
	DoQuit(CANCANCEL); // if we return from DoQuit, we don't want to quit
	return(FALSE);
} /*MacAnovaBaseFrame::OnClose()*/

// When base fram activates, automatically switch to a text window
void MacAnovaBaseFrame::OnActivate(Bool active)
{
	WHERE("MacAnovaBaseFrame::OnActivate");
#if 1
	if (active && numTextFrames > 0)
	{
		int        id = numTextFrames-1;

		TextFrames[id]->Show(TRUE);
		TextFrames[id]->text_window->SetFocus();
	}
#endif //0
} /*MacAnovaBaseFrame::OnActivate()*/

int MacAnovaBaseFrame::CheckForInput(void)
{
	long         i,done = 0, inputStatus, inputFrame;
	WHERE("MacAnovaBaseFrame::CheckForInput");

	if (INTERRUPT != INTNOTSET)
	{
		NERRORS++;
		if (BDEPTH > 0)
		{
			putOutErrorMsg("ERROR: batch file(s) terminated");
			closeBatch(1); /* shut down all batch files*/
		}
		putprompt((char *) 0);
		wxSetCursor(The_ibeam_cursor);   //always set cursor after prompt
		INTERRUPT = INTNOTSET;
		Running = 0;
		return (0);
	} /*if (INTERRUPT != INTNOTSET)*/
	if (Running)
	{
#ifdef FREQUENTCURSORSET
		wxSetCursor(The_watch_cursor);
#endif /*FREQUENTCURSORSET*/
	}
	else
	{
#ifdef FREQUENTCURSORSET
		wxSetCursor(The_ibeam_cursor);
#endif /*FREQUENTCURSORSET*/
		ProcessTypeAhead();
	}

	if (!Running)
	{ // now check for input in the windows
		inputFrame = -1;
		for (i=0;i<numTextFrames;i++)
		{
			if (TextFrames[i]->text_window->inputReady())
			{
				inputFrame = i;
				RunningFrame = i;
				RunningWindow = TextFrames[RunningFrame]->text_window;
			}
		}
		// no input ready, so just return
		if (inputFrame < 0)
		{
			return (0);
		}
	} /*if (!Running)*/
	wxGetElapsedTime(TRUE);	// reset elapsed time to zero

	// we now work in the RunningFrame
	if ( (BDEPTH > 0 ||
		  RunningWindow->GetLastPosition() > RunningWindow->CmdStrPos )
		 && (inputStatus = getinput()) > 0)
	{ /* we have an input line, EOF on batch file, or out of memory*/
		unsigned char      c = ' '; /* not '\0'*/
#if (1)
		if (INPUTSTRING != (unsigned char **) 0)
#else
		if (INPUTSTRING != (unsigned char **) 0 &&
		   (c = (*INPUTSTRING[0], c) != '\0'))
#endif
		{/* not out of memory */
			if (BDEPTH <= 0)
			{
				if (!BaseFrame->Running)
				{
					NLINES = 0;
				}

				if (ISATTY && MAXERRORS == 0)
				{
					NERRORS = 0;
				}
				RunningWindow->clearUndoInfo();
				if (BDEPTH == 0)
				{
					/* save info in case needed for undoing output  */
					RunningWindow->LastLength =
						RunningWindow->GetLastPosition();
					RunningWindow->LastCmdStrPos = RunningWindow->CmdStrPos;
					RunningWindow->UndoStatus = OUTPUTUNDO;
					TextFrames[RunningFrame]->ResetEditMenu(OUTPUTUNDO);
				}
			} /*if (BDEPTH <= 0)*/

			if ((*INPUTSTRING)[0] != '\0')
			{
				if (!Running)
				{
					// could change the menus here
				} /*if (!Running)*/
				Running = 1;
				if (BDEPTH <= 0)
				{
					BDEPTH = 0;
					// reset menus for batch?
				}
				
				if (inputStatus > 1)
				{
					putOutErrorMsg("WARNING: extra characters after end of line");
				}
				
				wxSetCursor(The_watch_cursor);
#ifdef RECOGNIZEBANG
				if ((*INPUTSTRING)[0] == BANG)
				{
					done = processBang();
				} /*if ((*INPUTSTRING)[0] == BANG)*/
				else
#endif /*RECOGNIZEBANG*/
				{
					done = DoStats(); /* go process input line */
				}
				
				FatalError = (done == FATALERROR) ? 1 : FatalError;
				
				done = (done == END || FatalError != 0);
				
				
				if (INTERRUPT != INTNOTSET && BDEPTH > 0)
				{
					closeBatch(1); /* close all batch files on interrupt */
				}
			} /*if ((*INPUTSTRING)[0] != '\0')*/
			else if (BDEPTH <= 0)
			{
				Running = BDEPTH = 0;
			}
			cleanitup(); /* post command housekeeping*/
			if (!done)
			{
				if (BDEPTH == 0 && RunningWindow->UndoStatus == OUTPUTUNDO)
				{
#if (0)
					ResetAllEditMenus();
#endif /*0*/
				}
			}
		} /*if (INPUTSTRING != (unsigned char **) 0)*/
		else if (c != '\0')
		{ /* couldn't allocate INPUTSTRING */
			if (BDEPTH > 0)
			{
				putOutErrorMsg("ERROR: batch file(s) terminated");
				closeBatch(1); /* shut down all batch files*/
			}
			done = 0;
		} /*if (INPUTSTRING != (unsigned char **) 0){}else{}*/
		
		if (!done)
		{
			if (MAXERRORS1 && NERRORS >= MAXERRORS1)
			{
				if (BDEPTH > 0)
				{
					myprint("WARNING: too many errors on batch file ");
					putOutErrorMsg(*INPUTFILENAMES[BDEPTH]);
					closeBatch(1); /* shut down all batch files*/
				}
				if (!ISATTY && MAXERRORS && NERRORS >= MAXERRORS)
				{
					putOutErrorMsg(TooManyErrors);
					done = FATALERROR;
					FatalError = 1;
				}
			} /*if (MAXERRORS1 && NERRORS >= MAXERRORS1)*/
		} /*if (!done)*/
		
		if (done && !FatalError)
		{
			DoQuit(CANCANCEL); // we only get past here on a cancel
			done = 0;
		}
		if (!done)
		{
			if (BDEPTH <= 0)
			{
				Running = 0;
				// fix up menus here
			} /*if (BDEPTH == 0)*/
			
			if (INTERRUPT != INTNOTSET && BDEPTH > 0)
			{
				putOutErrorMsg("ERROR: batch file(s) terminated");
				closeBatch(1); /* shut down all batch files*/
				INTERRUPT = INTNOTSET;
			} /*if (INTERRUPT != INTNOTSET && BDEPTH > 0)*/
			
			putprompt((char *) 0); /* This is basic prompt */
			wxSetCursor(The_ibeam_cursor); //always set cursor after prompt

 /* conditionally set flag for dump of yacc parsing steps */
			yydebug = (GUBED & 2048) ? 1 : 0;
			
		} /*if (!done)*/
	} /* if (we have an input line or have hit EOF on batch file)*/
	FatalError = (inputStatus < 0) ? 1 : FatalError;
	if (FatalError != 0)
	{ /* panic quit, run out of space */
		DoQuit(MUSTQUIT);
	}
	return (!done && BDEPTH > 0);
} /*MacAnovaBaseFrame::CheckForInput()*/

extern "C"
{
void closeBatch(long all)
{
	BaseFrame->CloseBatch(all);
}
	
} /* extern "C" */


void MacAnovaBaseFrame::CloseBatch(int all)
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
		Running = 0;
		MAXERRORS1 = MAXERRORS;
	}
} /*closeBatch()*/

extern "C"
{
int GetC(long iPlace, int *done)
{
	return(BaseFrame->GetC(iPlace,done));
} /*GetC()*/

void GetRunningWindowParms(long *CmdStrPos, long *TextLength)
{
	BaseFrame->GetRunningWindowParms(CmdStrPos,TextLength);
} /*GetRunningWindowParms()*/
	
} /*extern "C"*/

int MacAnovaBaseFrame::NVisible(void)
{
	int          i, count = 0;
	
	for (i = 0; i < MAXWINDOWS; i++)
	{
		if (TextFrames[i])
		{
#ifdef wx_msw
			if (IsWindowVisible(TextFrames[i]->text_window->GetHWND()))
			{
				count++;
			}
#else /*wx_msw*/
			if (TextFrames[i]->visibleStatus)
			{
				count++;
			}
#endif /*wx_msw*/
		} /*if (TextFrames[i]->text_window)*/		
	} /*for (i = 0; i < MAXWINDOWS; i++)*/
	return (count);
} /*MacAnovaBaseFrame::NVisible()*/

static char *wxMacAnovaInputBuffer = 0;

void MacAnovaBaseFrame::GetRunningWindowParms(long *CmdStrPos,
											  long *TextLength)
{
	*CmdStrPos = RunningWindow->CmdStrPos;
	*TextLength = RunningWindow->GetLastPosition();
	
#ifdef wx_msw
	long fromLine, toLine, fromLineChar, toLineChar;
	
	RunningWindow->PositionToXY(*CmdStrPos,&fromLineChar, &fromLine);
	RunningWindow->PositionToXY(*TextLength,&toLineChar, &toLine);
	*TextLength -= (toLine - fromLine); // to take care of CR's expunged
#endif //wx_msw
} /*MacAnovaBaseFrame::GetRunningWindowParms()*/

int MacAnovaBaseFrame::GetC(long iPlace, int *done)
{
	long from, to;
	long          fromLine, toLine, fromLineChar, toLineChar;
#ifndef wx_motif
	long          fillingLine, fillingLineChar, fillingLineLength;
	long          spaceUsed = 0;
	char         *lineChars;
#endif /*wx_motif*/

	from = RunningWindow->CmdStrPos;
	to = RunningWindow->GetLastPosition();
	
	RunningWindow->PositionToXY(from,&fromLineChar, &fromLine);
	RunningWindow->PositionToXY(to,&toLineChar, &toLine);
	
#ifdef wx_msw
	to -= (toLine - fromLine); // to take care of CR's expunged
#endif							//wx_msw
	
	if (iPlace < 0)
	{
		// reset things if iPlace is negative
		// this includes getting rid of any old InputBuffer
		// and setting up the input buffer for this trip 
		if (wxMacAnovaInputBuffer)
		{
			delete [] wxMacAnovaInputBuffer;
		}
		
		wxMacAnovaInputBuffer = new char[to-from+3];
		
#ifdef wx_motif
		Widget textWidget = (Widget) RunningWindow->handle;
		XmTextGetSubstring(textWidget, from, to-from, to-from+3,
						   wxMacAnovaInputBuffer);
#else							// wx_motif
		for (fillingLine = fromLine; fillingLine <= toLine; fillingLine++)
		{
			fillingLineLength = RunningWindow->GetLineLength(fillingLine);
			lineChars = new char[fillingLineLength+4];
			RunningWindow->GetLineText(fillingLine,lineChars,fillingLineLength);
#if (0)
			lineChars[fillingLineLength] = '\r';
			lineChars[fillingLineLength+1] = '\n';
			lineChars[fillingLineLength+2] = 0;
			fillingLineLength += 2;
#endif /*0*/
			lineChars[fillingLineLength] = '\n';
			lineChars[fillingLineLength+1] = 0;
			fillingLineLength += 1;
			fillingLineChar = (fillingLine == fromLine ? fromLineChar : 0);
			while (fillingLineChar < fillingLineLength && spaceUsed < to-from)
			{
				wxMacAnovaInputBuffer[spaceUsed++] =
					lineChars[fillingLineChar++];
			}
			delete [] lineChars;
		} /*for (fillingLine = fromLine; fillingLine <= toLine; fillingLine++)*/
#endif //wx_motif
		
		wxMacAnovaInputBuffer[to-from]=0;
		
		return(0);
	} /*if (iPlace < 0)*/
	
	((iPlace + 1 >= to) ? *done = 1 : *done = 0);
	return(wxMacAnovaInputBuffer[iPlace-from]);
} /*MacAnovaBaseFrame::GetC()*/


int MacAnovaBaseFrame::DoStats(void)
{
	return ((INPUTSTRING != (unsigned char **) 0) ? yyparse() : 0);
} /*MacAnovaBaseFrame::DoStats()*/


void MacAnovaBaseFrame::ProcessTypeAhead(void)
{
	//  NEED TO FIX!!!
} /*MacAnovaBaseFrame::ProcessTypeAhead()*/

Bool MacAnovaBaseFrame::SaveWorkspace(Bool doSaveAs)
{
	char           *valueList[2];
	long            typeList[2];
	char           *keyList[2];
	long            nargs;
	Bool            errorFlag;
	Symbolhandle    list;
	
	nargs = 2;
	keyList[0] = "";
	if (!doSaveAs && KEEPFILE != (FILE *) 0 && KEEPFILENAME != (char **) 0)
	{ /* Save Workspace and there has been previous save() or asciisave()*/
		typeList[0] = 0; /* simulate 'save(, options:T)'*/
	}
	else
	{ /* Save Workspace As... or no previous save() or asciisave() */
		typeList[0] = CHAR; /* simulate 'save("", options:T)'*/
		valueList[0] = "";
	}
	typeList[1] = LOGIC;
	valueList[1] = "T";
	keyList[1] = "options"; /* always save options on menu save */
	
	list = Buildlist(nargs,typeList,valueList,keyList);
	
	errorFlag = (list == (Symbolhandle) 0);
	if (!errorFlag)
	{
		strcpy(FUNCNAME, (ASCII > 0) ? "asciisave" : "save");
		/* save() returns NULLSYMBOL or 0 so no memory leakage here*/
		errorFlag = (save(list) == (Symbolhandle) 0);
		Removelist(list);
	}
	if (UseWindows > 0)
	{
		putprompt((char *) 0);
	}
	*OUTSTR = '\0';
	return (errorFlag);
} /*MacAnovaBaseFrame::SaveWorkspace()*/

Bool MacAnovaBaseFrame::RestoreWorkspace(char * fileName)
{
	/* Dummy up restore(fileName) command and execute it.  */

	Symbolhandle list;
	long         typeList[1];
	char        *valueList[1];
	char        *keyList[1];
	FILE        *restoreFile;
	int          errorFlag = 0;

	if (fileName == (char *) 0)
	{
		fileName = wxFindFile("", "MacAnova workspace file",NULL);
		if (fileName == (char *) 0)
		{
			return (1);
		}
	} /*if (fileName == (char *) 0)*/

	restoreFile = fmyopen(fileName,"r");
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

		errorFlag = (list == (Symbolhandle) 0);
		if (!errorFlag);
		{
			errorFlag = (restore(list) == (Symbolhandle) 0);
			Removelist(list);
		}
		if (errorFlag && PRINTWARNINGS)
		{
			myprint("WARNING: unable to restore workspace from file ");
			putOutErrorMsg(fileName);
		}
	}
	if (UseWindows > 0)
	{
		putprompt((char *) 0);
	}
	*OUTSTR = '\0';
	return (errorFlag);
} /*MacAnovaBaseFrame::RestoreWorkspace()*/

void  MacAnovaBaseFrame::FindGraphFrame(int windno, Bool mustwait)
{
	if (windno >= 0 && windno < NGRAPHS && CanvasFrames[windno])
	{
#ifdef wx_motif
		mustwait = (mustwait || !CanvasFrames[windno]->visibleStatus);
#endif /*wx_motif*/

		CanvasFrames[windno]->Show(TRUE);

		if (mustwait && FocusDelay > 0)
		{
#if (0)
	/*
	  Debugging code to allow tuning of focus delay
	  981219 FocusDelay now tunable by command line flags -plotdelay n
             or setoptions(plotdelat:n)
	*/
			Symbolhandle    symhDelay = Lookup("FOCUSDELAY");

			if (symhDelay && isInteger(symhDelay, POSITIVEVALUE))
			{
				FocusDelay = (int) DATAVALUE(symhDelay, 0);
			}
#endif /*0*/
			
			wxYield();
			waitForTicks(FocusDelay);
		} /*if (mustwait && FocusDelay > 0)*/

#ifdef wx_motif
		CanvasFrames[windno]->SetFocus();
#endif /*wx_motif*/
		CanvasFrames[windno]->canvas->SetFocus();
	}
} /*MacAnovaBaseFrame:FindGraphFrame()*/


void MacAnovaBaseFrame::KeyInterrupt (void)
{
	myeol();
	myerrorout("*****  Interrupt  *****");
	myeol();

	if(!Running)
	{
		RunningWindow->clearUndoInfo();
	} 
	INTERRUPT = INTSET; /* set interrupt flag */
	GraphicsPause = 0;
} /*MacAnovaBaseFrame::KeyInterrupt ()*/

void  MacAnovaBaseFrame::FindTextFrame(int windno, Bool mustwait)
{
	if (windno >= 0 && windno < MAXWINDOWS && TextFrames[windno])
	{
#ifdef wx_motif
		mustwait = mustwait || !TextFrames[windno]->visibleStatus;
#endif /*wx_motif*/

		TextFrames[windno]->Show(TRUE);
		EnableAllTextWindowMenus(MACANOVA_WINDOWS_HIDE,
											NVisible() > 1);
/*
  970916 wxYield() and waitForTicks() put in to delay things enough
         in Motif so that the window has been put up before trying to set
         focus.  Without something, we often crashed with a "BadMatch"
         error message meaning focus window not viewable when
         XSetInputFocus() called.  It would be nice to be able to
         check for actual viewability
*/
		if (mustwait && FocusDelay > 0)
		{
			wxYield();
			waitForTicks(FocusDelay);
		}

		TextFrames[windno]->SetFocus();
		TextFrames[windno]->text_window->SetFocus();
		RunningFrame = windno;
		RunningWindow = TextFrames[windno]->text_window;
	} /*if (windno >= 0 && windno < MAXWINDOWS && TextFrames[windno])*/
} /*MacAnovaBaseFrame::FindTextFrame()*/

#ifdef USETICKTIMER
void MacAnovaTickTimer::Notify(void)
{
	WHERE("Notify");
	
	Ticks++;
} /*MacAnovaTickTimer::Notify()*/

extern "C"
{
void resetTickTimer(long msPerTick)
{
	The_TickTimer->Stop();
	if (msPerTick > 0)
	{
		The_TickTimer->Start(msPerTick);
	}
} /*resetTickTimer()*/

} /*extern "C"*/
#endif /*USETICKTIMER*/

void MacAnovaMainTimer::Notify(void)
{
#if (0)
	The_timer->Stop();
#endif /*0*/
	Stop();

#ifdef wx_msw
	if (! FirstPassShow)
	{
		BaseFrame->TextFrames[0]->Show(TRUE);
		BaseFrame->TextFrames[0]->text_window->SetFocus();
		FirstPassShow = 1;
	}
#endif //wx_msw

	if (BaseFrame->numTextFrames == 0)
	{
		BaseFrame->DoQuit(MUSTQUIT);
	}
	while (BaseFrame->CheckForInput())
	{
		wxYield();
	}
#if (0)
	The_timer->Start(MACANOVATIMERINCREMENT); //MACANOVATIMERINCREMENT ms between checks
#endif /*0*/
	Start(MACANOVATIMERINCREMENT);//MACANOVATIMERINCREMENT ms between checks
} /*MacAnovaMainTimer::Notify()*/

extern "C" {
/*
  970410 both help and about message routed through putMessageBox
  which uses temporary storage instead of OUTSTR (the default size of
  OUTSTR is too small)
*/
void putMessageBox(char * theMessage [], char * title)
{
	char      **msg;
	int         length = 0;
	int         i;
	
	for (i = 0; theMessage[i] != (char *) 0; i++)
	{
		length += strlen(theMessage[i]) + 1;
	}
	msg = mygethandle(length);
	if (msg != (char **) 0)
	{
		length = 0;
		
		for (i = 0; theMessage[i] != (char *) 0; i++)
		{
			strcpy(*msg + length, theMessage[i]);
			length += strlen(theMessage[i]) + 1;
			(*msg)[length-1] = '\n';
		} /*for (i = 0; theMessage[i] != (char *) 0; i++)*/
		(*msg)[length-1] = '\0';
		(void) wxMessageBox(*msg, title , wxOK|wxCENTRE);
		mydisphandle(msg);
	} /*if (msg != (char **) 0)*/
} /*putMessageBox()*/

#define NAboutLines       7  /*number of lines in about box*/
extern char              *Welcome[]; /*defined in startMsg.c*/

void putAboutBox(void)
{
	putMessageBox(Welcome, "About MacAnova");
} /*putAboutBox()*/

extern char      *HelpMsg[]; /* defined in help.c*/

void putHelpBox(void)
{
	putMessageBox(HelpMsg, "Help Summary");
} /*putHelpBox()*/

void putUnimplementedBox(void)
{
	(void) wxMessageBox("The menu item is not implemented.\n\
Sorry.",
					   "Unimplemented message", wxOK|wxCENTRE);
} /*putUnimplementedBox()*/

void myYield(void)
{
	wxYield();
} /*myYield()*/
	
/* C++ replacements for malloc() and free() using new and delete */
void * wxMalloc(size_t size)
{
	char     *ptr = new char[size];
	
	return ((void *) ptr);
} /*wxMalloc()*/

void wxFree(void * ptr)
{
	delete [] (char *) ptr;
} /*wxFree()*/


} /* extern "C" */

#ifdef NEEDSFTIME
extern "C" {
#if (0)
#include <time.h>
#include <sys/timeb.h>
#endif /*(0)*/
void ftime(struct timeb *tb)
{
	SYSTEMTIME st;
	GetSystemTime( &st );
	tb->time = st.wSecond;
	tb->millitm = st.wMilliseconds;
} /*ftime()*/
	
	
} /*extern "C" */
#endif // NEEDSFTIME
