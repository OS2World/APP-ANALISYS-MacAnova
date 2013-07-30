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
#pragma segment Macmain
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


/*
  Main driver for MacAnova, originally consisting primarily of modules from
  TransSkel
*/

/*
	This file was originally modified from the main module of a
	TransSkel multiple-window demonstration written by Paul DuBois, the
	author of TransSkel 24 June 1986.  Until April, 1994 it relied on
	TransSkel, version 1.1, as modified for the needs of MacAnova.

	940429 Made modifications necessary to run using TransSkel 3.11

	960502 changed all references to macOpen() to macFindFile()

	960719 added File menu items batchit and spoolit

	960812 starting up with Command Q held down suppresses banner.

	960820 fixed minor problem in main() which prevented asking for a
           batch file when Command pressed and a spool file cancelled

    970514 minor rearrangement in main(), should result in no functional
           difference
    980401 added additional argument to initialize().
    980825 added and modified code to handle System 7 and later Help Menu
    990215 Changed myerrorout() to putOutErrorMsg() and putOutMsg() and
           some uses of putOUTSTR() by putErrorOUTSTR()
*/


#define MAIN__

#ifdef MW_CW_PROFILER
#include <profiler.h>
#endif /*MW_CW_PROFILER*/

#ifdef PERFORMANCE
#include "profile.h"
#endif /*PERFORMANCE*/

#include "macIface.h" /* Note: macIface.h replaces MultiSkel.h */
#include "globals.h"
#include "mvsave.h"
#include "version.h"  /* just need NEEDS68881 */

#ifndef MASTERBLOCKS
#define MASTERBLOCKS 40 /*970514 increased from 20*/
#endif /*MASTERBLOCKS*/

#define MAXHANDLES   (MASTERBLOCKS * 64)
#ifndef EXTRABLOCKS
#define EXTRABLOCKS   4 /* handles for system use, e.g., windows, scrap */
#endif /*EXTRABLOCKS*/

#ifndef OUTLINESIZE
#define OUTLINESIZE   400  /*size of output line buffer*/
#endif /*OUTLINESIZE*/

#define MOTDMARKER       "!!!!" /* Marker for Message of the day in Help file*/
#define MOTDMARKERLENGTH  4

char        *OutLineBuf;

#undef UNDEFINED__


#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment Getyacc
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


/* resource numbers for parser arrays*/
#define YYEXCA 1001
#define YYACT  1002
#define YYPACT 1003
#define YYPGO  1004
#define YYR1   1005
#define YYR2   1006
#define YYCHK  1007
#define YYDEF  1008
/* Global pointers to parser arrays */
short *Pyyexca;
short *Pyyact;
short *Pyypact;
short *Pyypgo;
short *Pyyr1;
short *Pyyr2;
short *Pyychk;
short *Pyydef;

static short * getYaccTable(Integer id)
{
	Handle      yaccTable;
	short      *table;
	LongInt     rsrcSize;

	yaccTable = GetResource('YTAB', id);
	if (yaccTable != (Handle) 0)
	{
		HNoPurge(yaccTable);
		MaxMem(&rsrcSize);
		HLock(yaccTable);
		table = (short *) *yaccTable;
	}
	else
	{
		table = (short *) 0;
	}
	return (table);

} /*getYaccTable()*/

static int  setupYaccTables(void)
{
	if ((Pyyexca = getYaccTable(YYEXCA)) == (short *) 0)
	{
		goto resourceError;
	}

	if ((Pyyact = getYaccTable(YYACT)) == (short *) 0)
	{
		goto resourceError;
	}

	if ((Pyypact = getYaccTable(YYPACT)) == (short *) 0)
	{
		goto resourceError;
	}

	if ((Pyypgo = getYaccTable(YYPGO)) == (short *) 0)
	{
		goto resourceError;
	}

	if ((Pyyr1 = getYaccTable(YYR1)) == (short *) 0)
	{
		goto resourceError;
	}

	if ((Pyyr2 = getYaccTable(YYR2)) == (short *) 0)
	{
		goto resourceError;
	}

	if ((Pyychk = getYaccTable(YYCHK)) == (short *) 0)
	{
		goto resourceError;
	}

	if ((Pyydef = getYaccTable(YYDEF)) == (short *) 0)
	{
		goto resourceError;
	}

	return (1);

  resourceError:
	myAlert("FATAL ERROR reading resource");
	return (0);


} /*setupYaccTables()*/


#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment Macmain
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#ifndef NKEYCHECKS
#define NKEYCHECKS   5 /* number of times to check for events */
#endif /*NKEYCHECKS*/
#ifndef NTICKS
#define NTICKS   30 /* half a second */
#endif /*NTICKS*/

/*
  Initialize proc pointers

  ThisISA is a macro defined in macIface.h to have value
  kPowerPCISA or kM68kISA

*/

static void initProcPtrs(void)
{
#ifdef MW_CW
	AboutDialogFilterUPP = NewRoutineDescriptor( (ProcPtr) aboutDialogFilter,
				 	uppModalFilterProcInfo, ThisISA);
	AboutDialogFilterPtr = (ModalFilterUPP) AboutDialogFilterUPP;

	AboutUserItemUPP = NewRoutineDescriptor( (ProcPtr) aboutUserItem,
				 	uppUserItemProcInfo, ThisISA);
	AboutUserItemPtr = (UserItemProcPtr) AboutUserItemUPP;

	ConsoleUserItemUPP = NewRoutineDescriptor( (ProcPtr) consoleUserItem,
				 	uppUserItemProcInfo, ThisISA);
	ConsoleUserItemPtr = (UserItemProcPtr) ConsoleUserItemUPP;	

	EditCommandUserItemUPP = NewRoutineDescriptor( (ProcPtr) editCommandUserItem,
				 	uppUserItemProcInfo, ThisISA);
	EditCommandUserItemPtr = (UserItemProcPtr) EditCommandUserItemUPP;	

	OutlineOKUserItemUPP = NewRoutineDescriptor( (ProcPtr) outlineOK,
				 	uppUserItemProcInfo, ThisISA);
	OutlineOKUserItemPtr = (UserItemProcPtr) OutlineOKUserItemUPP;	

	MyDialogFilterUPP = NewRoutineDescriptor( (ProcPtr) myDialogFilter,
				 	uppModalFilterProcInfo, ThisISA);
	MyDialogFilterPtr = (ModalFilterUPP) MyDialogFilterUPP;

	NullControlActionUPP = NewRoutineDescriptor( (ProcPtr) NULL,
				 	uppControlActionProcInfo, ThisISA);
	NullControlActionPtr = (ControlActionUPP) NullControlActionUPP;
		
	MyControlActionUPP = NewRoutineDescriptor( (ProcPtr) TrackScroll,
				 	uppControlActionProcInfo, ThisISA);
	MyControlActionPtr = (ControlActionUPP) MyControlActionUPP;
		
	NullDialogHookUPP = NewRoutineDescriptor( (ProcPtr) NULL,
			 	uppDlgHookProcInfo, ThisISA);
	NullDialogHookPtr = (DlgHookUPP) NullDialogHookUPP;
		
	NullFileFilterUPP = NewRoutineDescriptor( (ProcPtr) NULL,
			 	uppFileFilterProcInfo, ThisISA);
	NullFileFilterPtr = (FileFilterUPP) NullFileFilterUPP;
	
	MyTEClickLoopUPP = NewRoutineDescriptor( (ProcPtr) doClikLoop,
			 	uppTEClickLoopProcInfo, ThisISA);
	MyTEClickLoopPtr = (TEClickLoopUPP) MyTEClickLoopUPP;
#else /* MW_CW */
	AboutDialogFilterPtr = (ModalFilterProcPtr) aboutDialogFilter;
	AboutUserItemPtr = (ProcPtr) aboutUserItem;
	ConsoleUserItemPtr = (ProcPtr) consoleUserItem;
	EditCommandUserItemPtr = (ProcPtr) editCommandUserItem;
	OutlineOKUserItemPtr = (ProcPtr) outlineOK;
	MyDialogFilterPtr = (ModalFilterProcPtr) myDialogFilter;
	NullControlActionPtr = (ProcPtr) NULL;
	MyControlActionPtr = (ProcPtr) TrackScroll;
	NullDialogHookPtr = (DlgHookProcPtr) NULL;
	NullFileFilterPtr = (FileFilterProcPtr) NULL;
	MyTEClickLoopPtr = (ClikLoopProcPtr) doClikLoop;
#endif /* MW_CW */
}

#if defined(MW_CW_New)
/*
	AppFile as defined in segLoad.h in CW11 had powerpc alignment, with fType
	4 bytes after vRefNum; hence the following definition taken from their
	but with more control over pragmas
*/
#pragma options align=mac68k

struct myAppFile {
	short 							vRefNum;
	OSType 							fType;
	short 							versNum; /*versNum in high byte*/
	Str255 							fName;
};
typedef struct myAppFile myAppFile;

#pragma options align=reset

#else /*MW_CW_New*/
#define myAppFile       AppFile
#endif /*MW_CW_New*/

#if defined(MW_CW)

#define thisAppFile(i) ((myAppFile *) (*appFileInfo + myAppFileOffset(i)))

static void myCountAppFiles(Integer *message, Integer *count)
{
	Handle    appFileInfo;
	
#if (!TARGET_CPU_68K) /* doesn't work with MW_CW 11 for 68K, LMGetAppParmHandle() returns 0 */
	appFileInfo = LMGetAppParmHandle();
#if (0)
	{ /*for debugging*/
		long         *stuff = (long *) *appFileInfo;
		Handle        apParam;
		Str31	      apName;
		Integer       apRefNum;
		WHERE("myCountAppFiles");

		ALERT("appFileInfo = %08x, *appFileInfo = %08x",appFileInfo,*appFileInfo,0,0);
		ALERT("appFileInfo[0-3] = %08x%08x%08x%08x",stuff[0],stuff[1],stuff[2],stuff[3]);
		ALERT("appFileInfo[4-7] = %08x%08x%08x%08x",stuff[4],stuff[5],stuff[6],stuff[7]);
		GetAppParms(apName, &apRefNum, &apParam);
		/*
			apParam is also set to 0 in MW_CW 11 for 68K; appears same as appFileInfo
			for MW_CW ppc; GetAppParms() is not available for PPC MW_CW 11
		*/
		stuff = (long *) apName;
		ALERT("apName = %08x%08x%08x%08x",stuff[0],stuff[1],stuff[2],stuff[3]);
		ALERT("apRefNum = %d, apParam = %08x, *apParam = %08x",apRefNum,apParam,*apParam,0);
		stuff = (long *) *apParam;
		ALERT("(*apParam)[] = %08x%08x%08x%08x",stuff[0],stuff[1],stuff[2],stuff[3]);
	}
#endif /*0*/
	*count = ((Integer *) *appFileInfo)[1];
#else /*!TARGET_CPU_68K*/
	*count = 0;
#endif /*!TARGET_CPU_68K*/
	*message = 0;
} /*myCountAppFiles()*/
/*
	Compute offset into low memory are returned by LMGetAppParmhandle
	associated with filenum-th file.  If filenum out of bounds
	return as if filenum == 1
*/
static Integer myAppFileOffset(Integer filenum)
{
	Integer      offset = 4;
	Handle       appFileInfo;
	Integer      count, i;

	appFileInfo = LMGetAppParmHandle();
	count = ((Integer *) *appFileInfo)[1];

	if (filenum > 1 && filenum <= count)
	{
		for (i = 1;i < filenum;i++)
		{
			offset += 8; /*vRevNum (2), fType (4), versNum (2)*/
			offset += (*appFileInfo + offset)[0] + 1;
			offset += (offset % 2);
		}
	}
	return(offset);
} /*myAppFileOffset*/

static void myGetAppFiles(short index, myAppFile *theFile)
{
	Handle    appFileInfo;
	short	  count;
	WHERE("myGetAppFiles");
	
	appFileInfo = LMGetAppParmHandle();
	count = ((Integer *) *appFileInfo)[1];

	if ( index >= 1 && index <= count)
	{
#if (1)
		BlockMove((Ptr) thisAppFile(index), (Ptr) theFile, sizeof(myAppFile));
#else /*1*/
		int       i;

		theFile->vRefNum = thisAppFile(index)->vRefNum;
		theFile->fType = thisAppFile(index)->fType;
		theFile->versNum = thisAppFile(index)->versNum;
		count = thisAppFile(index)->fName[0];
		theFile->fName[0] = count;
		for (i=1;i<=count;i++)
		{
			theFile->fName[i] = thisAppFile(index)->fName[i];
		}
#endif /*1*/
	}
	else
	{
		ALERT("bad index in myGetAppFiles(): count = %d index = %d",count,index,0,0);
		theFile->vRefNum = 0;
		theFile->fType = 0;
		theFile->versNum = 0;
		theFile->fName[0] = 0;
	}
} /*myGetAppFiles*/
	
		
static void myClrAppFiles(short index)
{
	Handle    appFileInfo;
	short	  count;
	
	appFileInfo = LMGetAppParmHandle();
	
	count = ((Integer *) *appFileInfo)[1];
	
	if (index >= 1 && index <= count)
	{
		thisAppFile(index)->fType = 0;
	}
} /*myClrAppFiles()*/
#else
#define myCountAppFiles CountAppFiles
#define myGetAppFiles   GetAppFiles
#define myClrAppFiles   ClrAppFiles
#endif /*MW_CW*/

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

/*	Note that byte order of integers is from most significant to least, that
	is, in the order of the usual hex representation.  This is relevent
	since a KeyMap variable (long [4]) represents a packed array of
	Boolean in Pascal.
*/

static unsigned long Tickcount;

/*
  970115 moved isKeyPressed() to macUtils() and changed direct call
  to GetKeys() to scanKeyboard() (also in macUtils.c)
*/

/*
	Check keys at start up
	960814 New behavior
	 If no keys are pressed, it returns 0; otherwise it returns the
	 number of keys (up to MAXNKEYS) and puts the virtual keycodes in
	 keys[].  If the Command Key is pressed, keys[0] is set to
	 COMMANDKEYCODE.
*/

#define MAXNKEYS  4

static int checkKeys(unsigned char keys [MAXNKEYS])
{
	Integer        j, keyCode, nkeys = 0;
	unsigned char  ch = NOTCOMMAND;
	WHERE("checkKeys");

	for (j = 0; j < MAXNKEYS; j++)
	{
		keys[j] = NOTCOMMAND;
	}

	Tickcount = TickCount();

	do
	{
		scanKeyboard(); /*indirect call to GetKeys()*/
		for (keyCode=0;keyCode<127;keyCode++)
		{
			if (isKeyPressed(keyCode))
			{
				for (j = 0; j < nkeys && keys[j] != keyCode; j++)
				{
					;
				}
				if (j == nkeys)
				{
					if (nkeys > 0 && keyCode == COMMANDKEYCODE)
					{
						int       k;

						for (k=nkeys; k > 0; k--)
						{
							keys[k] = keys[k-1];
						}
					}
					keys[nkeys++] = keyCode;				
				} /*if (j == nkeys)*/
			} /*if (isKeyPressed(keyCode))*/
		} /*for (keyCode=0;keyCode<127;keyCode++)*/
	} while (nkeys < MAXNKEYS && TickCount() - Tickcount < NTICKS);

	FlushEvents(everyEvent ^ keyUpMask, 0);

	return (nkeys);
} /*checkKeys()*/


void main(void)
{
	Integer         OporPr, nAppFiles;
	myAppFile       startup_info;
	FILE           *iniFile = (FILE *) 0;
	char           *iniFileName = MACANOVAINI;
	CursHandle      cursor;
	Integer         i, loadStuff = 0, restoreStuff = 0, batchStuff = 0;
	Integer         errorFlag = 0;
	Boolean         assumeBatch = false, quiet = false;
	Symbolhandle    list = (Symbolhandle) 0;
	char           *valueList[2];
	long            typeList[2];
	char           *keyList[2];
	unsigned char   keys[MAXNKEYS];
	int             nkeys;
	long            nargs;
	SkelInitParams  initParams;
	WHERE("main");

#ifdef MPW
	_DataInit();
	UNLOADSEG(_DataInit);
#endif /*MPW*/
#undef DEBUGTOFILE /*define if using macros FPRINT or FPRINT1 (see dbug.h)*/
#ifdef DEBUGTOFILE
	/* Open file to be written to by macro FPRINT */
	DBFILE = fmyopen("DEBUG.out","w");
#endif /*DEBUGTOFILE*/

	initParams.skelMoreMasters = MASTERBLOCKS+EXTRABLOCKS;
	initParams.skelGzProc = (GrowZoneProcPtr) nil;
	initParams.skelResumeProc = (ResumeProcPtr) nil;
	initParams.skelStackAdjust = (Size) 0;

/* Initialize TransSkel */
	SkelInit(&initParams);
	SkelSetWaitTimes(CmdSleep, BackgroundSleep);
	
/* Initialize proc pointers */
	initProcPtrs();

#ifdef MW_CW_PROFILER
	if (ProfilerInit(collectDetailed,bestTimeBase,1000,50))
	{
		myAlert("ProfilerInit error");
		goto errorExit;
	}
#ifndef powerc
	UNLOADSEGMENTS = 1;
#endif /*powerc*/
#endif /* MW_CW_PROFILER */

#ifdef PERFORMANCE
	if (!InitPerf(&ThePGlobals,TIMERCOUNT,CODEBUCKETSIZE,DOROM,DOAPPCODE,
				 "\pCODE",ROMID,ROMNAME,DONTDORAM,0,0,0))
	{
		myAlert("InitPerf failure");
		goto errorExit;
	}
#endif /*PERFORMANCE*/

	/* allocate buffer for building messages and other strings*/
	OUTSTR = mygetpointer(BUFFERLENGTH * sizeof(char));
	if (OUTSTR == (char *) 0)
	{
		goto notAllocated;
	}
	*OUTSTR = '\0';

	if (!setupYaccTables())
	{
		goto notAllocated;
	}
	UNLOADSEG(setupYaccTables);

/* Initialize MacAnova handles and output line buffer (see macIo.c) */
	if (!myInitHandles(MAXHANDLES) ||
	   (OutLineBuf = mygetpointer(OUTLINESIZE+1)) == (char *) 0)
	{
		goto notAllocated;
	}
	OutLineBuf[0] = '\0';

	{ /* get and save information about startup directory */
		LongInt   dirID;
		Integer   vRefNum;

		errorFlag = HGetVol((StringPtr) 0, &vRefNum, &dirID);

		if (errorFlag == noErr)
		{
			HomeVolume = vRefNum;
			HomeDirectory = dirID;
		}
		else
		{
			HomeVolume = HomeDirectory = 0;
		}
	}

	cursor = GetCursor(iBeamCursor);
	IBEAM = **cursor;
	cursor = GetCursor(crossCursor);
	CROSS = **cursor;
	cursor = GetCursor(watchCursor);
	WATCH = **cursor;

	ISATTY = ITTYIN | ITTYOUT; /* Both input and output just for safety */
	teCmd = (TEHandle) 0;

/* 970714 added initialization of new global Has68881*/
	{
		SysEnvRec        r;
		
		SysEnvirons(1, &r);
		Has68881 = r.hasFPU;
#ifdef NEEDS68881
		if (! Has68881)
		{
			strcpy(OUTSTR,
				   "FATAL ERROR: This version requires 68881 Math Co-Processor");
			goto errorExit;
		}
#endif /*NEEDS68881*/
	}

/* 970806 added initialization of ThisMachine */
#if defined(powerc)
	ThisMachine = mvMacintoshPPC;
#elif defined(NEEDS68881)
	ThisMachine = mvMacintosh68Kc;
#else
	ThisMachine = mvMacintosh68Kn;
#endif /*powerc*/

	nkeys = checkKeys(keys); /* what keys are held down */
#if (0)
	ALERT("nkeys = %d, keys = %08x",
		  nkeys, keys[3] | (keys[2]<<8) | (keys[1]<<16) | (keys[0]<<24),0,0);
#endif /*0*/

	for (i = 0; i < nkeys; i++)
	{
		if (keys[i] == BKEYCODE || keys[i] == COMMANDKEYCODE)
		{
			assumeBatch = true;
		}
		if (keys[i] == QKEYCODE || keys[i] == OPTIONKEYCODE)
		{
			quiet = true;
		}
	} /*for (i = 1; i < nkeys; i++)*/

	if (!SetUpMenus())		/* install menu handlers */
	{
		strcpy(OUTSTR, "FATAL ERROR: Cannot build menus");
		goto errorExit;
	}

	/*
		Initialize symbol table and various global variables, including
		those related to GLM's
		Note: UseWindows == 0 at this point, suppressing any attempt to
		use a window that has not yet been created
	*/
	{
		promptType     nullPrompt;
		
		nullPrompt[0] = '\0';
		initialize((char *) 0, (char *) 0, (char *) 0, (char *) 0,
				   (char *) 0, (char *) 0, nullPrompt);
				   
		if (FatalError)
		{
			goto notAllocated;
		}
	}
	UNLOADSEG(initialize);

	/* initialize default zoom rectangles and other graphic globals*/
	GraphRectInit(-1);

	UseWindows = (assumeBatch) ? -1 : 1;

/*	Initialize Font Information */

	GetFNum(CmdFontName, &CmdFont);
	if (CmdFont == 0)
	{
		GetFNum(REGULARFONTNAME, &CmdFont);
	}
	GetFNum(GraphFontName, &GraphFont);
	if (GraphFont == 0)
	{
		GetFNum(REGULARFONTNAME, &GraphFont);
	}
	GetFNum(PrintFontName, &PrintFont);
	if (PrintFont == 0)
	{
		GetFNum(REGULARFONTNAME, &PrintFont);
	}
	
/* try to open initialization and help files */
	iniFile = fmyopen(iniFileName, TEXTREADMODE);
	HELPFILE = fmyopen(*HELPFILENAME, TEXTREADMODE);
	
	if (assumeBatch)
	{
		/* execute spool("") to open Spool file for output */
		nargs = 1;
		typeList[0] = CHAR;
		valueList[0] = keyList[0] = "";
		list = Buildlist(nargs,typeList,valueList,keyList);

		/* since UseWindows == -1, spool() will not protest*/
		if (list == (Symbolhandle) 0 || spool(list) == (Symbolhandle) 0)
		{
			myAlert("WARNING: cannot use non-interactive mode without a spool file");
			UseWindows = 1;
		}
		UNLOADSEG(spool);
		Removelist(list);
	} /*if (assumeBatch)*/

	/* install window handlers and create windows  */

	if (UseWindows > 0)
	{
		CmdWindInit(quiet);
	}
	else
	{
		BatchModeInit(quiet);
		UseWindows = 0;
	}
	DEFAULTDUMBPLOT = !UseWindows; /*override default set in initialize()*/

	if (FatalError)
	{
		(void) quitIt();
		strcpy(OUTSTR, "INITIALIZATION ERROR: cannot continue");
		goto errorExit;
	} /*if (FatalError)*/

	SkelSetSuspendResume((SkelSuspendResumeProcPtr) DoSuspendResume);

/*

	Make sure help file is found if there; once found, it is never closed
	so that it won't be lost if the directory changes
*/
	if (!quiet && HELPFILE != (FILE *) 0 &&
	   (LINE = mygethandle(MAXLINE)) != (char **) 0)
	{
		/* look for message of the day preceded and followed by "!!!!"*/
		int       printit = 0, screenwidth;
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
	} /*if (help file exists) */

	myCountAppFiles(&OporPr, &nAppFiles);

	for (i = 1; i<= nAppFiles; i++)
	{
		myGetAppFiles(i, &startup_info);
		if (startup_info.fType == 'TEXT')
		{
	/* assume it is saved output window or batch file; only use first one */
			if (!assumeBatch)
			{ /* take it as window to be read in */
				if (!loadStuff)
				{
					loadStuff = i;
				}
			} /*if (!assumeBatch)*/
			else if (!batchStuff)
			{ /* assume it is batch file to execute */
				batchStuff = i;
			} /*if (!assumeBatch){}else{}*/
		} /*if (startup_info.fType == 'TEXT')*/
		else if (startup_info.fType == KEEPTYPE ||
				startup_info.fType == ASCIIKEEPTYPE)
		{
			if (!restoreStuff)
			{
				restoreStuff = i;
			}
		}
		else
		{
			PtoCstr(startup_info.fName);
			sprintf(OUTSTR,
					"WARNING: startup file %s is not text file or save file",
					(char *) startup_info.fName);
			myAlert(OUTSTR);
			if (UseWindows > 0)
			{
				putOutErrorMsg(OUTSTR);
			}
		}
		myClrAppFiles(i);
	} /*for (i = 1; i<= nAppFiles; i++)*/

	if (assumeBatch && batchStuff == 0)
	{ /* Non-interactive but no batch file found yet */
		batchStuff = -1;
	}

	if (restoreStuff)
	{ /* now do actual restoration */
		myGetAppFiles(restoreStuff, &startup_info);
		PtoCstr(startup_info.fName);
		if ((errorFlag = doRestore((char *) startup_info.fName,
								   startup_info.vRefNum)) != noErr)
		{
			restoreStuff = 0;
		}
	} /*if (restoreStuff)*/

	if (batchStuff)
	{ /* Note, batch file will be executed after MacAnova.ini */
		BDEPTH = 0;
		if (batchStuff > 0)
		{ /* file provided */
			myGetAppFiles(batchStuff, &startup_info);
			BatchVolume = startup_info.vRefNum;
			PtoCstr(startup_info.fName);
		}
		else
		{ /* ask for file */
			startup_info.fName[0] = '\0';
		}
		typeList[0] = CHAR;
		valueList[0] = (char *) startup_info.fName;
		keyList[0] = "";
		typeList[1] = LOGIC;
		valueList[1] = "T";
		keyList[1] = "echo";
		list = Buildlist(nargs, typeList, valueList, keyList);
		
		if (list == (Symbolhandle) 0 ||
			(strcpy(FUNCNAME, "batch"), batch(list) == (Symbolhandle) 0))
		{
			if (!UseWindows)
			{
				strcpy(OUTSTR, "No command file supplied. MacAnova terminating");
				goto errorExit;
			}
			BatchVolume = 0;
		}
		Removelist(list);

		if (BatchVolume != 0 && !UseWindows)
		{ /* make file the level zero command file */
			INPUTFILE[0] = INPUTFILE[1];
/*
	Note: If UseWindows == 0, then we must have found a command file and hence
	INPUTFILE[0] != STDIN
*/
			INPUTFILENAMES[0] = INPUTFILENAMES[1];
			BATCHECHO[0] = 1;
			INPUTFILE[1] = (FILE *) 0;
			INPUTFILENAMES[1] = (char **) 0;
			BATCHECHO[1] = 0;
			BDEPTH = 0;
			ISATTY = 0;
			myeol();
			sprintf(OUTSTR,"Input taken from file '%s'", *INPUTFILENAMES[0]);
			putOUTSTR();
		}
		UNLOADSEG(batch);
	} /*if (batchStuff)*/

	if (iniFile != (FILE *) 0 && restoreStuff)
	{ /* don't use initialization file if restoring workspace */
		fclose(iniFile);
		iniFile = (FILE *) 0;
	}
	if (iniFile != (FILE *) 0)
	{ /* We were able previously to open the file */
		/* set up execution of initialization file */
		TMPHANDLE = mygethandle(strlen(iniFileName) + 1);
		INPUTFILENAMES[BDEPTH+1] = TMPHANDLE;
		if (TMPHANDLE != (char **) 0)
		{
			strcpy(*TMPHANDLE, iniFileName);
			INPUTFILE[++BDEPTH] = iniFile;
		}
		else
		{
			fclose(iniFile);
			iniFile = (FILE *) 0;
		}
	} /*if (iniFile != (FILE *) 0)*/

	if (loadStuff)
	{
		myGetAppFiles(loadStuff, &startup_info);
		PtoCstr(startup_info.fName);
		errorFlag = loadWindow((char *) startup_info.fName,
							   startup_info.vRefNum, false);
		CtoPstr((char *) startup_info.fName);
		if (errorFlag != noErr)
		{
			myAlert("Unable to read saved command window");
			loadStuff = 0;
		}
	} /*if (loadStuff)*/

	if (restoreStuff)
	{ /* make sure message appears */
		myGetAppFiles(restoreStuff, &startup_info);
		PtoCstr(startup_info.fName);
		sprintf(OUTSTR,"Workspace restored from file %s",
				(char *) startup_info.fName);
		putOUTSTR();
	} /*if (restoreStuff)*/

	if (!UseWindows || iniFile == (FILE *) 0 && (restoreStuff || !loadStuff))
	{
		putprompt((char *) 0);
	}

	if (UseWindows > 0)
	{
		CmdDirty = CmdWindows[CurrentWindow].cmdDirty = false;
		EnableItem(HelpMenu, HelpItemNumber);
		EnableItem(FileMenu, (Integer) 0);
		EnableItem(EditMenu, (Integer) 0);
		EnableItem(WindowMenu,newwind);
		if (BDEPTH == 0)
		{
			EnableItem(WindowMenu, (Integer) 0);
			EnableItem(CommandMenu, (Integer) 0);
			EnableItem(OptionsMenu, (Integer) 0);
			EnableItem(FontMenu, (Integer) 0);
			EnableItem(FontSizeMenu, (Integer) 0);
			Running = 0;
		}
		setCmdEditItems();
	} /*if (UseWindows > 0)*/

	DrawMenuBar();

	SkelEventLoop();

	SkelCleanup();		/* throw away windows and menus */

#ifdef DEBUGTOFILE
	if (DBFILE != (FILE *) 0)
	{
		fclose(DBFILE);
	}
#endif /*DEBUGTOFILE*/
#ifdef MW_CW_PROFILER
	ProfilerDump("\pprofilerOutput");
	ProfilerTerm();
#endif /* MW_CW_PROFILER */
	return;

  notAllocated:
	strcpy(OUTSTR,
		   "FATAL ERROR: unable to allocate memory during inialization sequence");
/* fall through*/
  errorExit:
#ifdef DEBUGTOFILE
	if (DBFILE != (FILE *) 0)
	{
		fclose(DBFILE);
	}
#endif /*DEBUGTOFILE*/
	if (*OUTSTR)
	{
		myAlert(OUTSTR);
	}
} /*main()*/


/* 980825 added DoHelp() to handle Help Menu */

static pascal void DoHelp(Integer item)
{
	if (UseHelpMenu && item == HelpItemNumber)
	{
		DoApple(helpit);
	}
} /*DoHelp()*/

/*
	Initialize menus.  Tell Skel to process the Apple menu automatically,
	and associate the proper procedures with the File and Edit menus.
	
	980825 Modified to put help in system Help menu for System 7 or later
*/
#define MAXITEMLENGTH   32

static Integer  LegalFontSizes[] = {9, 10, 12, 14, 18, 20, 24, 36, 0};

#define SUBMENU     true
#define DRAWBAR     true

Boolean SetUpMenus(void)
{
	Integer       i, length, size;
	Integer       markChar;
	Str255        itemString;
	WHERE("SetUpMenus");
			
	SkelApple((StringPtr) "\pAbout MacAnovaÉ;Help/H",
			  (SkelMenuSelectProcPtr) DoApple);

	AppleMenu = SkelGetAppleMenu();
	FileMenu = GetMenu(FILEMENU);
	EditMenu = GetMenu(EDITMENU);
	WindowMenu = GetMenu(WINDOWMENU);
	CommandMenu = GetMenu(COMMANDMENU);
	OptionsMenu = GetMenu(OPTIONSMENU);
	FontMenu = GetMenu(FONTMENU);
	FontSizeMenu = GetMenu(FONTSIZEMENU);

	if (SkelQuery(skelQSysVersion) > 0x0700)
	{
		OSErr      code = HMGetHelpMenuHandle(&HelpMenu);
		
		UseHelpMenu = (code == noErr && HelpMenu != (MenuHandle) 0);
		if (UseHelpMenu)
		{
			DelMenuItem(AppleMenu, helpit);
		}
	}
	else
	{
		UseHelpMenu = 0;
	}
	HelpItemNumber = (UseHelpMenu) ? CountMItems(HelpMenu) + helpit1 : helpit;

	if (!UseHelpMenu)
	{
		HelpMenu = AppleMenu;
	}

	if (FileMenu == (MenuHandle) 0 || EditMenu == (MenuHandle) 0 ||
		WindowMenu == (MenuHandle) 0 || CommandMenu == (MenuHandle) 0 ||
		OptionsMenu == (MenuHandle) 0 || FontMenu == (MenuHandle) 0 ||
		FontSizeMenu == (MenuHandle) 0 ||
		!SkelMenu(FileMenu, (SkelMenuSelectProcPtr) DoFile,
				  (SkelMenuClobberProcPtr) nil, !SUBMENU, !DRAWBAR) ||
		!SkelMenu(EditMenu, (SkelMenuSelectProcPtr) DoEdit,
				  (SkelMenuClobberProcPtr) nil, !SUBMENU, !DRAWBAR) ||
		!SkelMenu(WindowMenu, (SkelMenuSelectProcPtr) DoWind,
				  (SkelMenuClobberProcPtr) nil, !SUBMENU, !DRAWBAR) ||
		!SkelMenu(CommandMenu, (SkelMenuSelectProcPtr) DoCommand,
				  (SkelMenuClobberProcPtr) nil, !SUBMENU,!DRAWBAR) ||
		!SkelMenu(OptionsMenu, (SkelMenuSelectProcPtr) DoOptions,
				  (SkelMenuClobberProcPtr) nil, !SUBMENU, !DRAWBAR) ||
		!SkelMenu(FontMenu, (SkelMenuSelectProcPtr) DoFont,
				  (SkelMenuClobberProcPtr) nil, !SUBMENU, !DRAWBAR) ||
		UseHelpMenu && !SkelMenu(HelpMenu, (SkelMenuSelectProcPtr) DoHelp,
						(SkelMenuClobberProcPtr) nil, SUBMENU, !DRAWBAR) ||
		!SkelMenu(FontSizeMenu, (SkelMenuSelectProcPtr) DoFontSize,
				  (SkelMenuClobberProcPtr) nil, SUBMENU, DRAWBAR))
	{
		return (false);
	}

	DisableItem(FileMenu, (Integer) 0);
	DisableItem(EditMenu, (Integer) 0);
	DisableItem(WindowMenu, (Integer) 0);
	DisableItem(CommandMenu, (Integer) 0);
	DisableItem(OptionsMenu, (Integer) 0);
	DisableItem(FontMenu, (Integer) 0);
	DisableItem(FontSizeMenu, (Integer) 0);
	DisableItem(HelpMenu, HelpItemNumber);

	/* Save pre-defined commands */
	for (i = 0;i < NCOMMANDS;i++)
	{
		GetItem(CommandMenu, command1 + i, itemString);
		length = itemString[0];
		PtoCstr(itemString);
		GetItemMark(CommandMenu, command1 + i, &markChar);
		if (markChar == diamondMark && itemString[length-1] != Newline)
		{
			length++;
		}
		CommandItems[i] = TMPHANDLE = mygethandle(length + 1);
		if (TMPHANDLE == (char **) 0)
		{
			return (false);
		}
		strcpy(*TMPHANDLE, (const char *) itemString);

		if (markChar == diamondMark)
		{
			(*TMPHANDLE)[length--] = '\0';
			(*TMPHANDLE)[length] = Newline;
		} /*if (markChar == diamondMark)*/

		if (length > MAXITEMLENGTH)
		{
			itemString[MAXITEMLENGTH] = 0xc9; /* '...' */

			itemString[MAXITEMLENGTH+1] = '\0';
		}
		CtoPstr((char *) itemString);
		SetItem(CommandMenu, command1+i, itemString);
	} /*for (i=0;i<NCOMMANDS,i++)*/

	AddResMenu(FontMenu, 'FONT');

	for (i = 0; size = LegalFontSizes[i]; i++)
	{
		Str27      pointSize;

		sprintf((char *) pointSize, (size < 10) ? "%3d Point" : "%2d Point", size);
		CtoPstr((char *) pointSize);
		InsMenuItem(FontSizeMenu, pointSize, 100);
	}
	decorateFontMenus(CmdFont, CmdFontSize);
	if (UseHelpMenu)
	{
		AppendMenu(HelpMenu, "\pHelp/H");
	}
	return (true);

} /*SetUpMenus()*/


/*
	Handle selection of About MacAnova item from Apple menu
*/

extern char       *Welcome[];

#define ABOUTTICKS         240L
#define AboutFont          systemFont
#define AboutFontSize      12
#define AboutFrameItem     ok+1
#define AboutMessageItem1  (AboutFrameItem+1)
#define AboutCurvature     32
#define NAboutLines        7

/* Returns ok for any key or time expired */

pascal Boolean aboutDialogFilter (DialogPtr theDialog, EventRecord *theEvent,
							   Integer *itemHit)
{
	Integer         evtMods = theEvent->modifiers;
	char            evtChar = theEvent->message & charCodeMask;
	WHERE("aboutDialogFilter");

	if (theEvent->what == keyDown || theEvent->what == autoKey)
	{
		*itemHit = ok;
		return (true);
	}

	if (Tickcount != 0 && TickCount() - Tickcount > ABOUTTICKS)
	{
		*itemHit = ok;
		return (true);
	}
	return (false);
} /*aboutDialogFilter*/

 pascal void aboutUserItem(WindowPtr theWindow, Integer itemNo)
{
	Rect            r;
	Integer         itemType, lineNo = itemNo - AboutMessageItem1;
	Handle          itemHandle;
	char           *start, *end;
	Integer         length, h, v;
	Str255          msg;
	FontInfo       info;
	WHERE("aboutUserItem");

	GetDItem((DialogPtr) theWindow, itemNo, &itemType, &itemHandle, &r);

	if (itemNo == AboutFrameItem)
	{
		PenSize(3,3);
		InsetRect(&r,2,2);
		EraseRect(&r);
		FrameRoundRect(&r, AboutCurvature, AboutCurvature);
		PenSize(1,1);
		InsetRect(&r,4,4);
		FrameRoundRect(&r, AboutCurvature-4, AboutCurvature-4);
	} /*if (itemNo == AboutFrameItem)*/
	else if (lineNo < NAboutLines)
	{
		start = Welcome[lineNo];
		while (*start == ' ')
		{
			start++;
		}
		if (start[0] == '\0')
		{
			return;
		}
		GetFontInfo(&info);
		end = start + strlen(start) - 1;
		while (end > start && *end == ' ')
		{
			end--;
		}
		length = end - start + 1;
		strncpy((char *) msg + 1,start,length);
		msg[0] = length;
		h = (r.left + r.right - StringWidth( msg))/2;
		v = r.bottom - info.descent;
		MoveTo(h, v);
		DrawString( msg);
	} /*if (itemNo == AboutFrameItem){}else{}*/
} /*aboutUserItem()*/

/*
	970222 help() command generated is now just put in command line, to be
	       executed later as a regular command, rather than creating a
	       list and executing help() directly
*/
pascal void DoApple(Integer item)
{
	GrafPtr            thePort;
	DialogPtr          theDialog;
	Integer            i;
	Rect               r;
	Handle             itemHandle;
	Integer            itemType, itemHit;
	Boolean            brief;
	LongInt            selStart = SelStart(teCmd), selEnd = SelEnd(teCmd);
	WHERE("DoApple");

	GetPort(&thePort);
	switch (abs(item))
	{
	  case aboutmacanova:

		theDialog = getDlog(ABOUTMACANOVA, false, 1, "y");
		SetPort((GrafPtr) theDialog);
		TextFont(AboutFont);
		TextSize(AboutFontSize);
		GetDItem(theDialog, AboutFrameItem, &itemType, &itemHandle, &r);
		r = theDialog->portRect;
		InsetRect(&r,2,2);
		SetDItem(theDialog, AboutFrameItem, userItem, 
		    	 (Handle) AboutUserItemPtr, &r);

		for (i=0; i < NAboutLines && Welcome[i] != (char *) 0;i++)
		{
			GetDItem(theDialog, AboutMessageItem1 + i, &itemType, &itemHandle,
					 &r);
			SetDItem(theDialog, AboutMessageItem1 + i, userItem,
					  (Handle) AboutUserItemPtr, &r);
		}

		ShowWindow((WindowPtr) theDialog);

		Tickcount = (item < 0) ? TickCount() : 0;
		ModalDialog(AboutDialogFilterPtr, &itemHit);
		DisposDialog(theDialog);
		
		break;

	  case helpit:
		brief = (FrontWindow() != CmdWind) || selEnd == selStart;
		MyShowWindow(CmdWind);

		{
			/* dummy up help("selection") */
			GetItem(HelpMenu, HelpItemNumber, (unsigned char *) OUTSTR);
			PtoCstr((unsigned char *) OUTSTR);
		}
		if (OUTSTR[0] == 'H')
		{
			strcpy(OUTSTR, "help()\n");
		}
		else
		{
			strcpy(OUTSTR + strlen(OUTSTR) - 1, ",scrollback:T)\n");
		}

		insertCmd(OUTSTR, true);
		
		OUTSTR[0] = '\0';
		break;
	} /*switch (item)*/
	
} /*DoApple()*/

/*
	Process selection from File menu.

	Help    List all help options
	Print {window,selection,graph}
	Keep    Equivalent to save("")
	Restore Equivalent to restore("")
	Save    Save command window as text file
	Quit	Request a halt by calling SkelStopEventLoop().  This makes
			SkelEventLoop() return.
	Interrupt   Set INTERRUPT non zero
	
	960719  Added items batchit and spoolit
	960722  Changed coding so that keepit, keepitas, and restoreit just insert the
	        appropriate command instead of dummying up an argument list and
	        calling save() or restore() directly
*/

pascal void DoFile(Integer item)
{
	char            command[20];
	LongInt         teLength = TextLength(teCmd);
	WindowPtr       theWindow = FrontWindow();
	Integer         answer;
	Integer         running = Running;
	Integer         windno;
	WHERE("DoFile");

	if (!UseWindows && item != interrupt && item != keepitas)
	{
		return;
	}
/*
	Running == 2 indicates not running but processing a menu command
	Recognized by CmdWindActivate
*/
	Running = (Running) ? 1 : 2;
	switch (item)
	{
	  case pagesetup:
		if (theWindow == CmdWind)
		{
			(void) doPageSetup(TEXTPRINT);
		}
		else
		{
		  	(void) doPageSetup(GRAPHPRINT);
		}
		break;

	  case printit:
		if ((windno = whichGraphWindow(theWindow)) >= 0)
		{
			GraphWindPrint(windno);
		}
		else if ((windno = whichCmdWindow(theWindow)) >= 0)
		{
			CmdWindPrint(windno);
		}
		break;

	  case openit:
		answer = loadWindow((char *) 0, (Integer) 0, true);
		if (answer == noErr && CmdEditable)
		{
			putprompt((char *) 0);
			CmdDirty = CmdWindows[CurrentWindow].cmdDirty = false;
		}
		break;

	  case saveit:
	  case saveitas:
		if ((windno = whichGraphWindow(theWindow)) >= 0)
		{
			doSaveGraph(windno, item);
		}
		else
		{
			doSave(CmdWindows + CurrentWindow, item);
		}
		break;

	  case keepit:
	  case keepitas:
	  	if (item == keepit && KEEPFILE != (FILE *) 0 &&
			KEEPFILENAME != (char **) 0)
	  	{ /* Save Workspace and there has been previous save() or asciisave()*/
	  		sprintf(command, "save(%s)\n",
	  				(ASCII > 0) ? ",ascii:T" : "");
	  	}
	  	else
	  	{
	  		sprintf(command, "%s(\"\")\n",
	  				(ASCII > 0) ? "asciisave" : "save");
	  	}
	  	insertCmd(command, true);
		break;

	  case restoreit:
	  	insertCmd("restore(\"\")\n", true);
		break;

	  case spoolit:
		sprintf(command, "spool(%s)\n",
				(SPOOLFILENAME == (char **) 0) ? "\"\"" : "");
		insertCmd(command, true);
		break;

	  case batchit:
		insertCmd("batch(\"\")\n", true);
		break;

	  case quit:
		(void) quitIt();			/* close files and request halt */
		break;

	  case interrupt:
		if (UseWindows > 0)
		{
			MyShowWindow(CmdWind);
			myeol();
			putOutMsg("*****  Interrupt  *****");
			myeol();

			if (!running)
			{
				clearUndoInfo();
			}
		}
		INTERRUPT = INTSET; /* set interrupt flag */
		GraphWindPaused = false;
		break;

	  case go_on:
		GraphWindPaused = false;
		break;

	} /*switch (item)*/

	Running = running;

} /*DoFile()*/

/*
	Process selection from Window menu

	Hide         Hide front window
	Command      Display command window
	Graph n      Display graph window n, where n = 1,2,3 or 4
	Close        Hide the frontmost window.  If it belongs to a desk accessory,
	             close the accessory.

*/
pascal void DoWind(Integer item)
{
	WindowPeek      wPeek;
	WindowPtr       theWindow = FrontWindow();
	LongInt         selStart = SelStart(teCmd), selEnd = SelEnd(teCmd);
	Integer         insertLine = CmdInsertLine;
	Integer         windno = whichGraphWindow(theWindow);
	Integer         lastWind = CurrentWindow;
	Integer         running = Running;
	Integer         mods = 0;
	Str255          wTitle;
	unsigned char   key = '\0';
	Boolean         fromMenu = (item > 0);
	WHERE("DoWind");

	item = abs(item);
	if (!UseWindows)
	{
		return;
	}
	/*
		Running == 2 indicates not running but processing a menu command
		Recognized by CmdWindActivate
	*/
	Running = (Running) ? 1 : 2;
	if (item > cmd1 + Nwindows-1)
	{
		item += MAXWINDOWS - Nwindows;
	}

	switch (item)
	{
	  case hideit:
		if (windno >= 0 || nVisible() > 1)
		{
			HideWindow(theWindow);
		}
		break;

	/*
	  Close the front window.  Take into account whether it belongs
	  to a desk accessory or not.
	  */
	  case closeit:
		wPeek = (WindowPeek) theWindow;
		if (wPeek->windowKind < 0)
		{
			CloseDeskAcc(wPeek->windowKind);
		}
		else if (windno >= 0)
		{ /* must be graph window */
			GraphWindClose();
		}
		else
		{
			CmdWindClose();
		}
		break;

	  case newwind:
		DisableItem(WindowMenu, newwind);
	    windno = createWindow((STR255)  0);
		if (windno < 0)
		{
			myAlert("Cannot create new window");
			FatalError = 1;
			(void) quitIt();
		}
		else if (windno >= MAXWINDOWS)
		{
			myAlert("Too many windows already");
		}
		else
		{
			saveWindowInfo(CurrentWindow);
			GetWTitle(CmdWindows[windno].cmdWind, wTitle);
			item = cmd1 + Nwindows - 1;
			InsMenuItem(WindowMenu,wTitle,item - 1);
			CmdWindows[windno].menuItem = item;
			setCommandM(CurrentWindow); /* set menu key to toggle to old window */
			restoreWindowInfo(windno);
			clearUndoInfo();
			if (fromMenu)
			{
				putprompt((char *) 0);
			}
			CmdDirty = CmdWindows[CurrentWindow].cmdDirty = false;
		}
		break;

	  case cmd1:
	  case cmd2:
	  case cmd3:
	  case cmd4:
	  case cmd5:
	  case cmd6:
	  case cmd7:
	  case cmd8:
	  case cmd9:
/*	  case cmd10:*/ /*there should be exactly MAXWINDOWS of these*/
		for (windno = 0;windno < MAXWINDOWS; windno++)
		{
			if (item == CmdWindows[windno].menuItem)
			{
				break;
			}
		}
		if (windno != CurrentWindow)
		{
			saveWindowInfo(CurrentWindow);
			setCommandM(CurrentWindow);
			restoreWindowInfo(windno);
		}
		MyShowWindow(CmdWind);
		break;
 /* There should be exactly NGRAPHS of these graph items */
	  case graph1:
	  case graph2:
	  case graph3:
	  case graph4:
#if (NGRAPHS > NPERPANEL)
	  case graph5:
	  case graph6:
	  case graph7:
	  case graph8:
#endif
#if (NGRAPHS > 2*NPERPANEL)
	  case graph9:
	  case graph10:
	  case graph11:
	  case graph12:
#endif
	  case panel1:
#if (NGRAPHS > NPERPANEL)
	  case panel2:
#endif
#if (NGRAPHS > 2*NPERPANEL)
	  case panel3:
#endif

		item -= graph1;
		theWindow = (item < NGRAPHS) ? GraphWind[item] : PanelWind[item-NGRAPHS];
		MyShowWindow(theWindow);
		break;

	  case gototop:
		key = HomeKey;
		break;

	  case gotobottom:
		key = EndKey;
		break;

	  case gotoprompt:
	  	key = 'A';
		mods = cmdKey;
		break;
		
	  case pageup:
		key = Pageup;
		break;

	  case pagedown:
		key = Pagedown;
		break;

	} /*switch (item)*/
	if (key != '\0')
	{
		(void) nonTextKey(key, '\0', mods);
	}
	Running = running;
} /*DoWind()*/

#define TITLEITEM  20  /* item number for user item drawing title */
#define NLITEM     21  /* item number for user item drawing "NL?" */

 pascal void editCommandUserItem(WindowPtr theWindow, Integer item)
{
	Rect           r;
	Integer        oldFont = theWindow->txFont;
	Integer        oldSize = theWindow->txSize;
	Integer        itemType;
	Handle         itemHandle;
	StringPtr      msg = (item == TITLEITEM) ?
						"\pMake Changes, Then Click OK" : "\pNL?";
	Integer        h, v;
	FontInfo       info;
	WHERE("editCommandUserItem");

	TextFont(systemFont);
	TextSize(12);
	GetFontInfo(&info);
	
	GetDItem((DialogPtr) theWindow, item, &itemType, &itemHandle, &r);

	h = (r.left + r.right - StringWidth(msg))/2;
	v = r.bottom - info.descent;
	MoveTo(h, v);
	DrawString(msg);

	TextFont(oldFont);
	TextSize(oldSize);
}

/* Handler for Command Menu */
pascal void DoCommand(Integer item)
{
	Str255          command;
	DialogPtr       theDialog;
	Integer         i, length;
	Integer         textItem1 = 4;
	Integer         checkBox1 = textItem1 + NCOMMANDS;
	Rect            r;
	Handle          itemHandle;
	Integer         itemType, itemHit, itemValue, checks = 0;
	WHERE("DoCommand");

	if (!UseWindows)
	{
		return;
	}

	switch (item)
	{
	  case editcommands:
		SetDAFont(CmdFont);

		theDialog = getDlog(EDITCOMMANDS, true, 2, "yn");

		for (i = 0;i < NCOMMANDS;i++)
		{
			strcpy((char *) command, *(CommandItems[i]));
			CtoPstr((char *) command);
			itemValue = (command[(int) command[0]] == Newline) ? 1 : 0;
			GetDItem(theDialog, textItem1 + i, &itemType, &itemHandle, &r);
			command[0] -= itemValue;
			SetIText(itemHandle,command);
			checks |= itemValue << i;
		} /*for (i = 0;i < NCOMMANDS;i++)*/
		setDlogItemValues(theDialog, checkBox1, NCOMMANDS, checks);

		GetDItem(theDialog, TITLEITEM, &itemType, &itemHandle, &r);
		if ((itemType & 0x7f) == userItem)
		{
			SetDItem(theDialog, TITLEITEM, userItem,
					 (Handle) EditCommandUserItemPtr, &r);
		}
		GetDItem(theDialog, NLITEM, &itemType, &itemHandle, &r);

		if ((itemType & 0x7f) == userItem)
		{
			SetDItem(theDialog, NLITEM, userItem,
					 (Handle) EditCommandUserItemPtr, &r);
		}
		ShowWindow((WindowPtr) theDialog);


		do /* while (itemHit > cancel)*/
		{
			ModalDialog(MyDialogFilterPtr, &itemHit);
			if (itemHit > cancel)
			{
				GetDItem(theDialog, itemHit, &itemType, &itemHandle, &r);
				itemValue = GetCtlValue((ControlHandle) itemHandle);
				SetCtlValue((ControlHandle) itemHandle,
					(itemValue == 0) ? 1 : 0);
			}
		} while (itemHit > cancel);

		if (itemHit == ok)
		{ /* check for zero length commands */
			for (i = 0;i < NCOMMANDS;i++)
			{
				GetDItem(theDialog, textItem1 + i, &itemType, &itemHandle,
						 &r);
				GetIText(itemHandle, command);
				if (command[0] == '\0')
				{
					myAlert("ERROR: You cannot leave any command blank!");
					break;
				}
			} /*for (i = 0;i < NCOMMANDS;i++)*/

			if (i == NCOMMANDS)
			{

				checks = getDlogItemValues(theDialog, checkBox1, NCOMMANDS);
				for (i = 0;i < NCOMMANDS;i++)
				{
					long       handleLength;

					GetDItem(theDialog, textItem1 + i, &itemType,
							 &itemHandle, &r);
					GetIText(itemHandle, command);
					itemValue = (checks >> i) & 1;
					if (itemValue)
					{
						command[0]++;
						command[(int) command[0]] = Newline;
					}
					length = command[0];
					handleLength = myhandlelength(CommandItems[i]);
					if (handleLength > 0)
					{
						if (handleLength != length + 1)
						{
							TMPHANDLE = mygethandle(length + 1);
							if (TMPHANDLE != (char **) 0)
							{
								mydisphandle(CommandItems[i]);
							}
						}
						else
						{
							TMPHANDLE = CommandItems[i];
						}

						if (TMPHANDLE != (char **) 0)
						{
							CommandItems[i] = TMPHANDLE;
							PtoCstr(command);
							strcpy(*TMPHANDLE, (char *) command);
							if (itemValue)
							{
								length--;
							}
							if (length > MAXITEMLENGTH)
							{
								length = MAXITEMLENGTH;
								command[length++] = 0xc9; /* '...' */
							}
							command[length] = '\0';
							CtoPstr((char *) command);
							SetItem(CommandMenu,command1+i,command);
							SetItemMark(CommandMenu, command1+i,
										(itemValue) ? diamondMark : noMark);
						} /*if (TMPHANDLE != (char **) 0)*/
					} /*if (handleLength > 0)*/
				} /*for (i = 0;i < NCOMMANDS;i++)*/
			} /*if (i == NCOMMANDS)*/
		} /*if (itemHit == ok)*/
		DisposDialog(theDialog);
		SetDAFont(systemFont);
		break;

	  case command1:
	  case command2:
	  case command3:
	  case command4:
	  case command5:
	  case command6:
	  case command7:
	  case command8:
		/* insert command in Command Window */
		insertCmd(*(CommandItems[item - command1]), false);
		break;

	} /*switch (item)*/

}/*DoCommand()*/


pascal void DoSuspendResume(Boolean inForeground)
{
	WHERE("DoSuspendResume");
	
	InterruptInterval =
	  (inForeground) ? FgInterruptInterval : BgInterruptInterval;

	ActivateWindow(inForeground);

} /*DoSuspendResume()*/

/*
	Process items selected from Options menu
*/


static void setNsig(char * command)
{
	DialogPtr       theDialog;
	Rect            r;
	Handle          itemHandle;
	long            origNsig;
	Str255          string;
	Integer         itemType, itemHit;
	Integer         textItem = 5;
	Boolean         badValue = false;
	WHERE("setNsig");

	theDialog = getDlog(NSIGDLOG, true, 2, "yn");
	
	origNsig = (PRINTFORMAT[2] == 'g') ? PRINTFORMAT[1] : DEFAULTPRINTDEC;

	setDlogNumber(theDialog, textItem, (double) origNsig, "%g", true);

	ShowWindow((WindowPtr) theDialog);

	do
	{
		ModalDialog(MyDialogFilterPtr, &itemHit);
	} while (itemHit > cancel);

	command[0] = '\0';
	if (itemHit == ok)
	{
		long       newNsig;
		char      *pstart = (char *) string, *pend;
		
		GetDItem(theDialog, textItem, &itemType, &itemHandle, &r);
		GetIText(itemHandle, string);
		PtoCstr(string);
		newNsig = strtol(pstart, &pend, 10);
		badValue = pend == pstart || *pend != '\0' || newNsig < 0;
		if (!badValue && newNsig != origNsig)
		{
			sprintf(command,"setoptions(nsig:%ld)\n", newNsig);
		}
	} /*if (itemHit == ok)*/

	DisposDialog(theDialog);
	if (badValue)
	{
		sprintf(DBOUT,
				"\322%s\323 is an improper value for default significant digits",
				string);
		myAlert(DBOUT);
	}
	InitCursor();
} /*setNsig()*/

#define WFORMAT    0
#define PFORMAT    1

#define FLOATFMTCHAR   'g'
#define FIXEDFMTCHAR   'f'

enum formatItems
{
	formatRadioButtonCount = 2,
	formatTextItemCount = 3,
	formatDefaultButton = 3,
	formatRadioButton1 = 6,
	formatTextItem1 = formatRadioButton1 + 3*formatRadioButtonCount
};

#define WidthStr         textStr[0]
#define DigitsStr        textStr[1]
#define MissingStr       textStr[2][0]
#define OrigWidthStr     origTextStr[0]
#define OrigDigitsStr    origTextStr[1]
#define OrigMissingStr   origTextStr[2][0]

static void setFormats(char * command)
{
	DialogPtr       theDialog;
	Rect            r;
	Handle          itemHandle;
	Integer         radioButtons[formatRadioButtonCount];
	Integer         textItems[formatTextItemCount];
	Integer         value, digits;
	Integer         i, j;
	Str63           textStr[formatTextItemCount][2];
	char            origTextStr[formatTextItemCount][2][32];
	Integer         itemType, itemHit, whichRadioButton;
	Integer         currentFormat, newFormat;
	Boolean         badMissing = false;
	char            modes[2], origModes[2];
	char            comma[2];
	unsigned char  *floatlabel = "\pSignificant Digits:";
	unsigned char  *fixedlabel = "\pNumber of Decimals:";
	char           *fmt = "%g";
	char           *pcommand = command;
	WHERE("setFormats");

	comma[0] = comma[1] = '\0';
	theDialog = getDlog(FORMATSDLOG, true, 3, "ynd");

	for (i = 0; i < formatRadioButtonCount; i++)
	{
		radioButtons[i] = formatRadioButton1 + 3*i;
	}
	for (i = 0; i < formatTextItemCount; i++)
	{
		textItems[i] = formatTextItem1 + 2*i;
	}

	for (i = 0; i < 2; i ++)
	{
		sprintf(origTextStr[i][PFORMAT],"%ld",PRINTFORMAT[i]);
		sprintf(origTextStr[i][WFORMAT],"%ld",WRITEFORMAT[i]);
		for (j = 0; j < 2; j++)
		{
			strcpy((char *) textStr[i][j], (char *) origTextStr[i][j]);
			CtoPstr((char *) textStr[i][j]);
		}
	};
	strcpy((char *) OrigMissingStr, NAMEFORMISSING);
	strcpy((char *) MissingStr, (char *) OrigMissingStr);
	CtoPstr((char *) MissingStr);

	modes[PFORMAT] = origModes[PFORMAT] = PRINTFORMAT[2];
	modes[WFORMAT] = origModes[WFORMAT] = WRITEFORMAT[2];

	
	for (i = 0; i < formatTextItemCount; i++)
	{
		j = (i < 2) ? PFORMAT : 0;
		GetDItem(theDialog, textItems[i], &itemType, &itemHandle, &r);
		SetIText(itemHandle, textStr[i][j]);
	}
	SelIText(theDialog, textItems[0], 0, 32767);

	value = (modes[PFORMAT] == FLOATFMTCHAR) ? 0x01 : 0x02;

	setDlogItemValues(theDialog, radioButtons[0], 2, 1 << PFORMAT);
	setDlogItemValues(theDialog, radioButtons[1], 2, value);

	GetDItem(theDialog, textItems[1]-1, &itemType, &itemHandle, &r);
	SetIText(itemHandle, (value & 1) ? floatlabel : fixedlabel);

	ShowWindow((WindowPtr) theDialog);

	do /* while (itemHit > cancel) */
	{
		ModalDialog(MyDialogFilterPtr, &itemHit);

		currentFormat = (getDlogItemValues(theDialog, radioButtons[0], 2) == 0x01) ?
			WFORMAT : PFORMAT;

		for (i = 0; i < formatTextItemCount; i++)
		{
			j = (i < 2) ? currentFormat : 0;
			GetDItem(theDialog, textItems[i], &itemType, &itemHandle, &r);
			GetIText(itemHandle, textStr[i][j]);
		}

		if (itemHit == formatDefaultButton)
		{ /* set default value */
			digits = (currentFormat == PFORMAT) ?
				DEFAULTPRINTDEC : DEFAULTWRITEDEC;
			sprintf((char *) WidthStr[currentFormat],"%ld",digits+7);
			sprintf((char *) DigitsStr[currentFormat],"%ld",digits);
			strcpy((char *) MissingStr, DEFAULTNAMEFORMISSING);

			for (i = 0; i < formatTextItemCount; i++)
			{
				j = (i < 2) ? currentFormat : 0;
				CtoPstr((char *) textStr[i][j]);
				GetDItem(theDialog, textItems[i], &itemType, &itemHandle, &r);
				SetIText(itemHandle, textStr[i][j]);
			}
			SelIText(theDialog, textItems[0], 0, 32767);

			modes[currentFormat] = 'g';
			setDlogItemValues(theDialog, radioButtons[1], 2, 0x01);
			GetDItem(theDialog, textItems[1]-1, &itemType, &itemHandle, &r);
			SetIText(itemHandle, floatlabel);
		}
		else if (itemHit > cancel)
		{ /* radio button hit */
			whichRadioButton = itemHit;
			if (whichRadioButton == radioButtons[0] ||
				whichRadioButton == radioButtons[0] + 1)
			{ /* format or wformat */
				whichRadioButton -= radioButtons[0];
				newFormat = (whichRadioButton) ? PFORMAT : WFORMAT;
				setDlogItemValues(theDialog, radioButtons[0], 2, 1 << whichRadioButton);
				if (newFormat != currentFormat)
				{ /* value has changed */
					value = (modes[newFormat] == FLOATFMTCHAR) ? 0x01 : 0x02;
					setDlogItemValues(theDialog, radioButtons[1], 2, value);

					for (i = 0; i < 2; i++)
					{
						GetDItem(theDialog, textItems[i], &itemType, &itemHandle, &r);
						SetIText(itemHandle, textStr[i][newFormat]);
					}
					SelIText(theDialog, textItems[0], 0, 32767);

					GetDItem(theDialog, textItems[1]-1, &itemType, &itemHandle, &r);
					SetIText(itemHandle, (modes[newFormat] == FLOATFMTCHAR) ?
							 floatlabel : fixedlabel);
				} /*if (newFormat != currentFormat)*/
			}
			else if (whichRadioButton == radioButtons[1] ||
					 whichRadioButton == radioButtons[1] + 1)
			{ /* fixed or floating */
				whichRadioButton -= radioButtons[1];
				setDlogItemValues(theDialog, radioButtons[1], 2,
								  1 << whichRadioButton);
				modes[currentFormat] = (whichRadioButton == 0) ?
										FLOATFMTCHAR : FIXEDFMTCHAR;
				GetDItem(theDialog, textItems[1]-1, &itemType, &itemHandle, &r);
				SetIText(itemHandle, (whichRadioButton == 0) ?
						 floatlabel : fixedlabel);
			}
		} /*if (itemHit > cancel)*/
	} while (itemHit > cancel);

	if (itemHit == ok)
	{
		
		for (i = 0; i < 2; i++)
		{
			for (j = 0; j < 2; j++)
			{
				PtoCstr(textStr[i][j]);
			}
		}
		PtoCstr(MissingStr);
		strcpy(pcommand, "setoptions(");
		pcommand += strlen(pcommand);
		if (strcmp((char *) WidthStr[PFORMAT], OrigWidthStr[PFORMAT]) != 0 ||
			strcmp((char *) DigitsStr[PFORMAT], OrigDigitsStr[PFORMAT]) != 0 ||
			modes[PFORMAT] != origModes[PFORMAT])
		{
			sprintf(pcommand,"format:\"%s.%s%c\"",
					WidthStr[PFORMAT], DigitsStr[PFORMAT], modes[PFORMAT]);
			pcommand += strlen(pcommand);
			comma[0] = ',';
		}
		if (strcmp((char *) WidthStr[WFORMAT], OrigWidthStr[WFORMAT]) != 0 ||
			strcmp((char *) DigitsStr[WFORMAT], OrigDigitsStr[WFORMAT]) != 0 ||
			modes[WFORMAT] != origModes[WFORMAT])
		{
			sprintf(pcommand,"%swformat:\"%s.%s%c\"",
					comma, WidthStr[WFORMAT], DigitsStr[WFORMAT], modes[WFORMAT]);
			pcommand += strlen(pcommand);
			comma[0] = ',';
		}
		if (strlen((char *) MissingStr) > LENGTHMISSING || strchr((char *) MissingStr, '"'))
		{
			badMissing = true;
		}
		else if (strcmp((char *) MissingStr, OrigMissingStr) != 0)
		{
			sprintf(pcommand, "%smissing:\"%s\"", comma, MissingStr);
			pcommand += strlen(pcommand);
			comma[0] = ',';
		}
	} /*if (itemHit == ok)*/

	if (comma[0])
	{
		strcpy(pcommand, ")\n");
	}
	else
	{
		command[0] = '\0';
	}

	DisposDialog(theDialog);
	if (badMissing)
	{
		sprintf(OUTSTR,
				"\322%s\323 is too long or contains \324\"\325\nValue for MISSING is not changed",
				MissingStr);
		myAlert(OUTSTR);
		*OUTSTR = '\0';
	}
	InitCursor();
} /*setFormats()*/

static void setSeeds(char * command)
{
	DialogPtr       theDialog;
	Rect            r;
	Handle          itemHandle;
	Str255          string1, string2;
	Integer         itemType, itemHit;
	Integer         textItem1 = 5, textItem2 = 7;
	int             badValue = 0;
	char           *fmt = "%.15g", *pend1, *pend2;
	long            rands1, rands2;
	WHERE("setSeeds");

	theDialog = getDlog(SEEDSDLOG, true, 2, "yn");
	
	setDlogNumber(theDialog, textItem2, (double) RANDS2, fmt, false);
	setDlogNumber(theDialog, textItem1, (double) RANDS1, fmt, true);
	ShowWindow((WindowPtr) theDialog);

	do
	{
		ModalDialog(MyDialogFilterPtr, &itemHit);
	} while (itemHit > cancel);

	command[0] = '\0';
	if (itemHit == ok)
	{
		GetDItem(theDialog, textItem1, &itemType, &itemHandle, &r);
		GetIText(itemHandle, string1);
		PtoCstr(string1);
		GetDItem(theDialog, textItem2, &itemType, &itemHandle, &r);
		GetIText(itemHandle, string2);
		PtoCstr(string2);
		rands1 = strtol((char *) string1, &pend1, 10);
		rands2 = strtol((char *) string2, &pend2, 10);
		if (pend1 == (char *) string1 || *pend1 != '\0' || rands1 < 0)
		{
			badValue = 1;
		}
		else if (pend2 == (char *) string2 || *pend2 != '\0' || rands2 < 0)
		{
			
			badValue = 2;
		}
		if (!badValue &&
			(RANDS1 != rands1 || RANDS2 != rands2))
		{
			sprintf(command,"setoptions(seeds:vector(%s,%s))\n",
					(char *) string1, (char *) string2);
		}
	}
	DisposDialog(theDialog);
	if (badValue)
	{
		sprintf(DBOUT, "\322%s\323 is an improper value for a seed",
				(badValue == 1) ? string1 : string2);
		myAlert(DBOUT);
	}
	InitCursor();
} /*setSeeds()*/

static void setAngles(char * command)
{
	DialogPtr       theDialog;
	Rect            r;
	Handle          itemHandle;
	Integer         radioButton1 = 5;
	Integer         value, origValue;
	Integer         itemType, itemHit;
	Integer         i;
	double          angles2cycles = 8.0*MV_PI_4;
	double          angles2degrees = MV_PI_4/45.0;
	Str255          string;
	WHERE("setAngles");

	theDialog = getDlog(ANGLESDLOG, false, 2, "yn");

	origValue = (ANGLES2RADIANS == angles2degrees) ? 0x01 :
		((ANGLES2RADIANS == angles2cycles) ? 0x02 : 0x04);

	setDlogItemValues(theDialog, radioButton1, 3, origValue);

	ShowWindow((WindowPtr) theDialog);

	do
	{
		ModalDialog(MyDialogFilterPtr, &itemHit);
		if (itemHit > cancel)
		{
			i = itemHit - radioButton1;
			setDlogItemValues(theDialog, radioButton1, 3, 1 << i);
		} /*if (itemHit > cancel)*/
	} while (itemHit > cancel);

	command[0] = '\0';
	if (itemHit == ok)
	{
		value = getDlogItemValues(theDialog, radioButton1, 3);
		itemHit = radioButton1 + ((value & 1) ? 0 : ((value & 2) ? 1 : 2));
		GetDItem(theDialog, itemHit, &itemType, &itemHandle, &r);
		GetCTitle((ControlHandle) itemHandle, string);
		PtoCstr(string);
		string[0] = tolower(string[0]);
		if (value != origValue)
		{
			sprintf(command,"setoptions(angles:\"%s\")\n", (char *) string);
		}
	} /*if (itemHit == ok)*/

	DisposDialog(theDialog);
	InitCursor();

} /*setAngles()*/

static void setGlmOptions(char * command)
{
	DialogPtr       theDialog;
	Integer         radioButton1 = 5, radioButton3 = 8;
	Integer         value1, value2;
	Integer         origValue1, origValue2;
	Integer         itemHit;
	Integer         i;
	char            sep[2];
	WHERE("setGlmOptions");

	sep[0] = sep[1] = '\0';
	
	theDialog = getDlog(GLMOPTIONSDLOG, true, 2, "yn");

	origValue1 = (PRINTPVALS) ? 0x01 :  0x02;
	origValue2 = (PRINTFSTATS) ? 0x01 :  0x02;
	setDlogItemValues(theDialog, radioButton1, 2, origValue1);

	setDlogItemValues(theDialog, radioButton3, 2, origValue2);

	ShowWindow((WindowPtr) theDialog);

	command[0] = '\0';
	do
	{
		ModalDialog(MyDialogFilterPtr, &itemHit);
		if (itemHit > cancel)
		{
			i = itemHit - radioButton1;
			if (0 <= i && i < 2)
			{
				setDlogItemValues(theDialog, radioButton1, 2, 1 << i);
			}
			i = itemHit - radioButton3;
			if (0 <= i && i < 2)
			{
				setDlogItemValues(theDialog, radioButton3, 2, 1 << i);
			}
		} /*if (itemHit > cancel)*/
	} while (itemHit > cancel);

	if (itemHit == ok)
	{
		char    *pcommand = command;
		
		value1 = getDlogItemValues(theDialog, radioButton1, 2);
		value2 = getDlogItemValues(theDialog, radioButton3, 2);
		
		strcpy(pcommand,"setoptions(");
		pcommand += strlen(pcommand);
		if (value1 != origValue1)
		{
			sprintf(pcommand, "%spvals:%c", sep,
					(value1 & 1) ? 'T' : 'F');
			pcommand += strlen(pcommand);
			sep[0] = ',';
		}
		if (value2 != origValue2)
		{
			sprintf(pcommand, "%sfstats:%c", sep,
					(value2 & 1) ? 'T' : 'F');
			pcommand += strlen(pcommand);
			sep[0] = ',';
		} 
 		if (sep[0] != '\0')
 		{
 			*pcommand++ = ')';
 			*pcommand++ = '\n';
 			*pcommand = '\0';
 		}
 		else
 		{
 			command[0] = '\0';
 		}
	} /*if (itemHit == ok)*/
	DisposDialog(theDialog);
	InitCursor();

} /*setGlmOptions()*/

static void setBatchOptions(char * command)
{
	DialogPtr       theDialog;
	Rect            r;
	Handle          itemHandle;
	Integer         radioButton1 = 4;
	Integer         textItem = 7;
	Integer         value;
	Integer         origValue = (DEFAULTBATCHECHO) ? 0x01 :  0x02;
	Str255          string;
	Integer         itemType, itemHit;
	Integer         i;
	Boolean         badValue = false;
	char            sep[2];
	WHERE("setBatchOptions");

	sep[0] = sep[1] = '\0';
	
	theDialog = getDlog(BOPTIONSDLOG, true, 2, "yn");

	setDlogNumber(theDialog, textItem, (double) MAXERRORS, "%g", true);

	setDlogItemValues(theDialog, radioButton1, 2, origValue);

	ShowWindow((WindowPtr) theDialog);

	do
	{
		ModalDialog(MyDialogFilterPtr, &itemHit);
		if (itemHit > cancel)
		{
			i = itemHit - radioButton1;
			setDlogItemValues(theDialog, radioButton1, 2, 1 << i);
		} /*if (itemHit > cancel)*/
	} while (itemHit > cancel);

	command[0] = '\0';
	if (itemHit == ok)
	{
		char       *pcommand = command, *pend;
		long        newMaxerrors;
		
		strcpy(pcommand, "setoptions(");
		pcommand += strlen(pcommand);
		value = getDlogItemValues(theDialog, radioButton1, 2);
		if (value != origValue)
		{
			sprintf(pcommand, "%sbatchecho:%c", sep,
					(value & 1) ? 'T' : 'F');
			pcommand += strlen(pcommand);
			sep[0] = ',';
		}
		GetDItem(theDialog, textItem, &itemType, &itemHandle, &r);
		GetIText(itemHandle, string);
		PtoCstr(string);
		newMaxerrors = strtol((const char *) string, (char **) &pend, 10);
		badValue = (pend == (char *) string || *pend != '\0' || newMaxerrors < 0);
		if (!badValue && newMaxerrors != MAXERRORS)
		{
			sprintf(pcommand, "%serrors:%s", sep, string);
			pcommand += strlen(pcommand);
			sep[0] = ',';
		}
		if (sep[0] != '\0')
		{
			*pcommand++ = ')';
			*pcommand++ = '\n';
			*pcommand = '\0';
		}
		else
		{
			command[0] = '\0';
		}
	} /*if (itemHit == ok)*/

	DisposDialog(theDialog);
	if (badValue)
	{
		sprintf(DBOUT,
				"\322%s\323 is an improper value for Error Limit",
				string);
		myAlert(DBOUT);
	}
	InitCursor();

} /*setBatchOptions()*/

enum otherItems
{
	otherRadioButtonCount = 4,
	otherTextItemCount = 3,
	otherRadioButton1 = 5,
	otherTextItem1 = otherRadioButton1 + 3*otherRadioButtonCount,
	otherQuotedCount = 1
};
static char         *OtherRadioOptionNames[] =
{
	"dumbplot", "warnings", "restoredel", "scrollback"
};
static char         *OtherTextOptionNames[] =
{
	"prompt", "history", "maxwhile"
};

#define TOOLONGITEMERR  1
#define EMPTYITEMERR    2
#define QUOTEINITEMERR  3

static void setOtherOptions(char * command)
{
	DialogPtr       theDialog;
	Rect            r;
	Handle          itemHandle;
	Integer         origValues[otherRadioButtonCount], newValue;
	Integer         radioButtons[otherRadioButtonCount];
	Integer         textItems[otherTextItemCount];
	Str255          textString;
	char            textItemLabels[otherTextItemCount][64];
	char            origTextStrings[otherTextItemCount][64];
	char           *pcommand = command;
	char            sep[2], *quote = "\"";
	Integer         itemType, itemHit;
	Integer         i;
	Integer         badTextItems[otherTextItemCount];
	Boolean         quoted[otherTextItemCount];
	WHERE("setOtherOptions");

	sep[0] = sep[1] = '\0';
	
	theDialog = getDlog(OTHEROPTIONSDLOG, true, 2, "yn");

	origValues[0] = (DEFAULTDUMBPLOT)   ? 0x01 :  0x02;
	origValues[1] = (PRINTWARNINGS)     ? 0x01 :  0x02;
	origValues[2] = (DEFAULTRESTOREDEL) ? 0x01 :  0x02;
	origValues[3] = (SCROLLBACK)        ? 0x01 :  0x02;
	
	for (i = 0; i < otherRadioButtonCount; i++)
	{
		radioButtons[i] = otherRadioButton1 + 3*i;
		setDlogItemValues(theDialog, radioButtons[i], 2, origValues[i]);
	} /*for (i = 0; i < otherRadioButtonCount; i++)*/
	
	strcpy(origTextStrings[0], PROMPT);
	sprintf(origTextStrings[1], "%ld", HISTORY);
	sprintf(origTextStrings[2], "%ld", MAXWHILE - 1);
	
	for (i = 0; i < otherTextItemCount; i++)
	{
		badTextItems[i] = 0;
		quoted[i] = (i < otherQuotedCount);
		textItems[i] = otherTextItem1 + 2*i;
		
		GetDItem(theDialog, textItems[i] - 1, &itemType, &itemHandle, &r);
		GetIText(itemHandle, textString);
		PtoCstr(textString);
		strcpy(textItemLabels[i], (char *) textString);
		
		strcpy((char *) textString, origTextStrings[i]);
		CtoPstr((char *) textString);
		GetDItem(theDialog, textItems[i], &itemType, &itemHandle, &r);
		SetIText(itemHandle, textString);
	} /*for (i = 0; i < otherTextItemCount; i++)*/
	SelIText(theDialog, textItems[0], 0, 32767);
	ShowWindow((WindowPtr) theDialog);

	do
	{
		ModalDialog(MyDialogFilterPtr, &itemHit);
		if (itemHit > cancel)
		{
			Integer     iHit;
			
			for (i = 0; i < otherRadioButtonCount; i++)
			{
				iHit = itemHit - radioButtons[i];
				if (0 <= iHit && iHit < 2)
				{
					setDlogItemValues(theDialog, radioButtons[i], 2, 1 << iHit);
					break;
				}
			} /*for (i = 0; i < otherRadioButtonCount; i++)*/
		} /*if (itemHit > cancel)*/
	} while (itemHit > cancel);

	if (itemHit == ok)
	{		
		strcpy(pcommand, "setoptions(");
		pcommand += strlen(pcommand);
		
		for (i = 0; i < otherRadioButtonCount; i++)
		{
			newValue = getDlogItemValues(theDialog, radioButtons[i], 2);
			if (newValue != origValues[i])
			{
				sprintf(pcommand, "%s%s:%c", sep,
						OtherRadioOptionNames[i],
						(newValue & 1) ? 'T' : 'F');
				pcommand += strlen(pcommand);
				sep[0] = ',';
			} /*if (newValue != origValues[i])*/
		} /*for (i = 0; i < otherRadioButtonCount; i++)*/
		
		for (i = 0; i < otherTextItemCount; i++)
		{
			GetDItem(theDialog, textItems[i], &itemType, &itemHandle, &r);
			GetIText(itemHandle, textString);
			PtoCstr(textString);
			if (strcmp((char *) textString, origTextStrings[i]) != 0)
			{
				if (i == 0 && strlen((char *) textString) > 20)
				{
					badTextItems[i] = TOOLONGITEMERR;
				}
				else if (textString[0] == '\0')
				{
					badTextItems[i] = EMPTYITEMERR;
				}
				else if (strchr((char *) textString, '"'))
				{
					badTextItems[i] = QUOTEINITEMERR;
				}
				else
				{
					sprintf(pcommand, "%s%s:%s%s%s", sep,
							OtherTextOptionNames[i],
							quoted[i] ? quote : NullString,
							textString,
							quoted[i] ? quote : NullString);
					pcommand += strlen(pcommand);
					sep[0] = ',';
				}
			} /*if (strcmp(textString, origTextStrings[i]) != 0)*/
		} /*for (i = 0; i < otherTextItemCount; i++)*/
		*pcommand++ = ')';
		*pcommand++ = '\n';
		*pcommand = '\0';
	} /*if (itemHit == ok)*/
	
	if (sep[0] == '\0')
	{
		command[0] = '\0';
	}
	DisposDialog(theDialog);
	for (i = 0; i < otherTextItemCount; i++)
	{
		char      *problem;
		
		problem = (char *) 0;
		switch (badTextItems[i])
		{
		  case TOOLONGITEMERR:
		  	problem = "is too long";
		  	break;
		  	
		  case EMPTYITEMERR:
		  	problem = "is null";
		  	break;
		  	
		  case QUOTEINITEMERR:
		  	problem = "contains \324\"\325";
		  	break;
		  	
		  default:
		  	break;
		} /*switch (badTextItems[i])*/
		  	
		if (problem)
		{
			sprintf(OUTSTR, "Value for %s %s; ignored",
					textItemLabels[i], problem);
			myAlert(OUTSTR);
		} /*if (problem)*/
	} /*for (i = 0; i < otherTextItemCount; i++)*/
	*OUTSTR = '\0';
	InitCursor();

} /*setOtherOptions()*/

pascal void DoOptions(Integer item)
{
	char       command[250];
	WHERE("DoOptions");
	
	switch (item)
	{
	  case insig:
		setNsig(command);
		break;

	  case iformats:
		setFormats(command);
		break;

	  case iseeds:
		setSeeds(command);
		break;

	  case iangles:
		setAngles(command);
		break;

	  case iglmoptions:
		setGlmOptions(command);
		break;

	  case iboptions:
		setBatchOptions(command);
		break;

	  case iotheroptions:
		setOtherOptions(command);
		break;

	} /*switch (item)*/
	if (command[0] != '\0')
	{
		insertCmd(command, true);
	}
}

pascal void DoFont(Integer item)
{
	Str255      fontName;
	Integer     fontNumber;
	WHERE("DoFont");
	
	GetItem(FontMenu, item, fontName);
	GetFNum(fontName, &fontNumber);
	setCmdWindFont(fontNumber, CmdFontSize);
} /*DoFont()*/

pascal void DoFontSize(Integer item)
{
	Str255      pointSize;
	int         fontSize;
	WHERE("DoFontSize");
	
	GetItem(FontSizeMenu, item, pointSize);
	PtoCstr(pointSize);
	sscanf((char *) pointSize, "%d", &fontSize);
	setCmdWindFont(CmdFont, (Integer) fontSize);
} /*DoFontSize()*/

/*
	Process item selected from Edit menu.  First check whether it should
	get routed to a desk accessory or not.  If not, then for route the
	selection to the text editing window, as that is the only one for
	this application to which edit commands are valid.

	(The test of FrontWindow is not strictly necessary, as the Edit
	menu is disabled when any of the other windows is frontmost, and so
	this Proc couldn't be called.)
*/

pascal void DoEdit(Integer item)
{
	WindowPtr          theWindow;
	WHERE("doEdit");

	if (!SystemEdit(item - 1))	/* check DA edit choice */
	{
		theWindow = FrontWindow();
		if (UseWindows > 0 && theWindow == CmdWind)
		{
			CmdWindEditMenu(item);
		}
		else if (whichGraphWindow(theWindow) >= 0 )
		{
			GraphWindEditMenu(item,theWindow);
		}
	} /*if (!SystemEdit(item - 1)) */
} /*DoEdit()*/


/*
	Miscellaneous routines
	These take care of drawing the grow box and the line along
	the right edge of the window, and of setting and resetting the clip
	region to disallow drawing in that right edge by the other drawing
	routines.
*/


void DrawGrowBox(wind)
WindowPtr       wind;
{
	Rect            r;
	RgnHandle       oldClip;

	r = wind->portRect;
	r.left = r.right - 15;	/* draw only along right edge */
	oldClip = NewRgn();
	GetClip(oldClip); /* save clipping region of current grafPort */
	ClipRect(&r); /* make r the clipping region */
	DrawGrowIcon(wind);
	SetClip(oldClip); /* restore previous clipping region */
	DisposeRgn(oldClip);
} /*DrawGrowBox()*/

#ifdef UNDEFINED__ /* following two functions not currently referenced */

/* shift right boundary of clipping region for window in by 15*/
void SetWindClip(wind)
WindowPtr       wind;
{
	Rect            r;

	r = wind->portRect;
	r.right -= 15;		/* don't draw along right edge */
	oldClip = NewRgn();
	GetClip(oldClip);  /* save clipping region of current grafPort in global */
	ClipRect(&r);
} /*SetWindClip()*/


void ResetWindClip()
{
	SetClip(oldClip);   /* restore clipping region previously saved in global */
	DisposeRgn(oldClip);
} /*ResetWindClip()*/
#endif /*UNDEFINED__*/

/*
	Function to terminate MacAnova
	Unless SaveOnQuit is false, the user is given the opportunity to save
	the workspace and any open text windows.
	Unless FatalError is non-zero, at any of the stages, quitting may be
	aborted by clicking Cancel in a dialog box.
	
	980808 Added SkelChkOneEvent() before checking option key in an attempt to
	       overcome erroneous reports the option key was down when it wasn't
*/

Integer quitIt(void)
{
	Integer          i;
	Integer          windno, button = ok;
	Str255           wTitle;
	Boolean          optionkeyPressed;
	cmdWindowInfoPtr wp;
	WHERE("quitIt");

	SkelChkOneEvent(); 
	optionkeyPressed = (SkelGetCurrentEvent()->modifiers & optionKey) != 0;
	GraphWindPaused = false;

/*
	The following is supposed to copy the last few lines of the command window
	to the Clipboard in case of a fatal error, but it does not seem always
	to work.  Unless a copy was done in MacAnova, on exit, the Clipboard
	holds what it did before invoking MacAnova
*/
	if (FatalError && UseWindows > 0)
	{ /* should copy last few lines of command window to Clip Board */
		onFatalError();
	}

	closeBatch(1); /* close all batch files */
	/* give use a chance to save the workspace */

	if (SaveOnQuit && !optionkeyPressed)
	{
		if ((button = saveAlert( 0, false)) == cancel && !FatalError &&
		    UseWindows > 0)
		{
			return (cancel);
		}
		if (button == ok)
		{
			Symbolhandle    list;
			long            typeList[1];
			char           *valueList[1], *keyList[1];
			
			typeList[0] = CHAR;
			keyList[0] = valueList[0] = "";
			list = Buildlist(1, typeList ,valueList, keyList);
	
			if (list != (Symbolhandle) 0)
			{
				strcpy(FUNCNAME, (ASCII > 0) ? "asciisave" : "save");
				save(list);
				UNLOADSEG(save);
				Removelist(list);
				if (UseWindows > 0 && !FatalError && !Reply.good)
				{
					putprompt((char *) 0);
					return (cancel);
				}
			} /*if (list != (Symbolhandle) 0)*/
		} /*if (button == ok)*/
	} /*if (SaveOnQuit && !optionkeyPressed)*/

	/* Give user a change to save dirty windows before quitting */
	if (UseWindows > 0  && SaveOnQuit && !optionkeyPressed)
	{
		for (windno = 0, wp = CmdWindows;windno < MAXWINDOWS; windno++, wp++)
		{
			if (wp->cmdWind != (WindowPtr) 0 && wp->cmdEditable && wp->cmdDirty)
			{
				GetWTitle(wp->cmdWind, wTitle);
				if ((button = saveAlert(wTitle, true)) == cancel && !FatalError)
				{
					return (button);
				}
				if (button == ok)
				{
					doSave(wp, quit);
					if (!FatalError && !Reply.good)
					{
						return (cancel);
					}
				} /*if (button == ok)*/
			} /*if (wp->cmdWind!=(WindowPtr) 0 && wp->cmdEditable && wp->cmdDirty)*/
		} /*for (windno = 0, wp = CmdWindows;windno < MAXWINDOWS; windno++, wp++)*/
	} /*if (UseWindows > 0  && SaveOnQuit && !optionkeyPressed)*/

	if (!UseWindows)
	{
		myAlert("End of non-interactive MacAnova run");
	}

#ifdef PERFORMANCE
	{
		OSErr    err;

		if (ProfileStatus > 0)
		{
			err = PerfDump(ThePGlobals,PERFORMOUT,DOHISTOGRAM,
						   RPTFILECOLUMNS);
			if (err)
			{
				ALERT("PerfDump reports err = %d",err,0,0,0);
			}
		}
		TermPerf(ThePGlobals);
	}
#endif /*PERFORMANCE*/

	SkelStopEventLoop();

/*	Close all files */
	if (INPUTFILE[0] != STDIN)
	{
		fclose(INPUTFILE[0]);
	}
	if (SPOOLFILE != (FILE *) 0)
	{
		fclose(SPOOLFILE);
	}
	if (HELPFILE != (FILE *) 0)
	{
		fclose(HELPFILE);
	}
	closeBatch(1);  /* shut down all batch files*/
	for (i=0;i<2;i++)
	{
		if (PrintHdl[i] != (THPrint) 0)
		{
			DisposHandle((Handle) PrintHdl[i]);
		}
	}
	myDispAll(); /* release all MacAnova handles */

	return (ok);

} /*quitIt()*/

/* Write length characters starting at linePtr to file */
static OSErr saveLine(Integer refNum, char *linePtr, LongInt length)
{
	return (FSWrite(refNum,&length,linePtr));
}
/*
	Save contents of command window in text file
*/

void doSave(cmdWindowInfoPtr wp,Integer item)
{
	OSErr          errorFlag;
	FInfo          fndrInfo;
	Integer        refNum, vRefNum;
	Integer        ask;
	Str255         wTitle;
	char          *fileName, *msgs;
	WHERE("doSave");

	if (!UseWindows)
	{
		return;
	}
	vRefNum = wp->cmdVRefNum;
	if (vRefNum == 0 && wp->cmdDirty)
	{
		item = saveitas;
	}
	if (item == saveit && !wp->cmdDirty)
	{
		Reply.good = true;
		return;
	}

	ask = (item == saveitas || item == quit);

	GetWTitle(wp->cmdWind, wTitle);

	if (!ask)
	{
		SetVol(0L, vRefNum);
	} /*if (!ask)*/
	else
	{
		fileName = macFindFile("", "\pFile for output window:", wTitle,
							   WRITEIT, 0, (OSType *) 0, &vRefNum);
		if (fileName == (char *) 0)
		{ /* cancelled */
			return;
		}
		
		strcpy((char *) wTitle, fileName);
		CtoPstr((char *) wTitle);
	} /*if (!ask){}else{}*/

	errorFlag = GetFInfo(wTitle, vRefNum, &fndrInfo);

	if (errorFlag == noErr)
	{
	 /* if file exists, delete it */
		errorFlag = FSDelete(wTitle, vRefNum);
		if (errorFlag != noErr)
		{
			goto errorExit;
		}
	} /*if (errorFlag == noErr)*/

 /* file not found or deleted, create it */
	errorFlag = Create(wTitle, vRefNum, CREATOR, 'TEXT');
	if (errorFlag != noErr)
	{
		goto errorExit;
	}

	errorFlag = FSOpen(wTitle, vRefNum,&refNum);
	if (errorFlag != noErr)
	{
		goto errorExit;
	}

	HLock(TextHandle(wp->teCmd));
	errorFlag = saveLine(refNum, *TextHandle(wp->teCmd),TextLength(wp->teCmd));
	HUnlock(TextHandle(wp->teCmd));
	if (errorFlag != noErr)
	{
		goto errorExit;
	}

/* Write a couple of lines at end with date */
	msgs = "\n#Output Window saved ";
	errorFlag = saveLine(refNum, msgs, strlen(msgs));
	if (errorFlag != noErr)
	{
		goto errorExit;
	}
	msgs = getTimeAndDate();
	errorFlag = saveLine(refNum, msgs, strlen(msgs));
	if (errorFlag != noErr)
	{
		goto errorExit;
	}
	msgs = "\n#--------------------------------------------\n";
	errorFlag = saveLine(refNum, msgs, strlen(msgs));
	if (errorFlag != noErr)
	{
		goto errorExit;
	}

	FSClose(refNum);
	FlushVol ((StringPtr) 0, vRefNum);

	wp->cmdVRefNum = vRefNum;
	wp->cmdDirty = false;
	if (wp - CmdWindows == CurrentWindow)
	{
		CmdDirty = false;
	}
	SetWTitle(wp->cmdWind, wTitle);
	SetItem(WindowMenu, wp->menuItem, wTitle);
	PtoCstr(wTitle);
	macSetInfo(vRefNum,(char *) wTitle,'TEXT',CREATOR);

	return;

  errorExit:
	PtoCstr(wTitle);
	fileErrorMsg(errorFlag, (char *) wTitle);
	Reply.good = false;
} /*doSave()*/

/*
	Replace contents of command window with contents of text file
*/
Integer loadWindow(char * fileName,Integer vRefNum,Boolean new)
{
	LongInt		logEOF;
	LongInt     length;
	Integer     refNum;
	OSErr       errorFlag = 0;
	Handle      hText = (Handle) 0;
	Integer     truncated = 0, windno, item;
	Str255      wTitle;
	WHERE("loadWindow");

	if (new && Nwindows == MAXWINDOWS)
	{
		myAlert("Too many windows already open");
		return (1);
	}
	if (fileName == (char *) 0)
	{
		 /* macFindFile default type is TEXT */
		fileName = macFindFile("", "\pFile for command window", 0, READIT,
							   0, (OSType *) 0, &vRefNum);
		if (fileName == (char *) 0)
		{ /* cancelled */
			return (1);
		}
	} /*if (fileName == (char *) 0)*/
	else
	{
		SetVol(0L, vRefNum);
	}
	
	strcpy((char *) wTitle, fileName);
	CtoPstr((char *) wTitle);
	errorFlag = FSOpen(wTitle,vRefNum, &refNum);

	if (errorFlag != noErr)
	{
		myprint("WARNING: unable to open file ");
		putOutErrorMsg(fileName);
	} /*if (errorFlag != noErr)*/
	else
	{
		errorFlag = GetEOF(refNum,&logEOF);
/*
   961114 changed limit from CMDWINDOWNEWSIZE to CMDWINDOWLIMIT
*/
		if (errorFlag == noErr)
		{
			long        limit = CMDWINDOWLIMIT;
			
			if (logEOF > limit)
			{
				errorFlag = SetFPos(refNum, fsFromStart, logEOF-limit);
				truncated = logEOF - limit;
				logEOF = limit;
			}
			if (errorFlag == noErr)
			{
				if ((hText = NewHandle (logEOF)) == (Handle) 0)
				{
					putOutErrorMsg("ERROR: insufficient free memory");
					return(2);
				}
				length = logEOF;
				HLock(hText);
				errorFlag = FSRead(refNum, &length, (Ptr) (*hText));
				HUnlock(hText);
				if (errorFlag == noErr && length != logEOF)
				{
					errorFlag = eofErr;
				}

				if (errorFlag != noErr)
				{
					DisposHandle(hText);
				}
			} /*if (errorFlag == noErr)*/

			if (errorFlag == noErr)
			{
				if (new)
				{
					windno = createWindow(wTitle);
					if (windno < 0)
					{
						myAlert("Cannot create new window");
						FatalError = 1;
						quitIt();
					}
					DisposHandle((Handle) TextHandle(CmdWindows[windno].teCmd));
					item = cmd1 + Nwindows - 1;
					InsMenuItem(WindowMenu,wTitle,item - 1);
					CmdWindows[windno].menuItem = item;
					setCommandM(CurrentWindow); /* set menu key to toggle back*/
					restoreWindowInfo(windno);
				} /*if (new)*/
				else
				{
					DisposHandle((Handle) TextHandle(teCmd));
					SetWTitle(CmdWind, wTitle);
					SetItem(WindowMenu,
							CmdWindows[CurrentWindow].menuItem,	wTitle);
				} /*if (new){}else{}*/
				TextHandle(teCmd) = hText;
				TextLength(teCmd) = length;
				TESetSelect(length, length, teCmd);
				TECalText(teCmd);

				CmdInsertLine = GetCmdIL();
				ScrollToInsertPt();
				CmdWindows[CurrentWindow].cmdVRefNum = vRefNum;
				CmdWindows[CurrentWindow].cmdDirty = CmdDirty = false;
				CmdEditable = true;
				CmdStrPos = length;
				saveWindowInfo(CurrentWindow);

				InvalRect(&(CmdWind->portRect));
				macUpdate(CmdWind);

				if (truncated)
				{
					sprintf(OUTSTR,
							"WARNING: file too long; only last %ld characters loaded",
							limit);
					myAlert(OUTSTR);
					putErrorOUTSTR();
				}
			} /*if (errorFlag == noErr)*/
			FSClose(refNum);
		} /*if (errorFlag == noErr)*/
		if (errorFlag != noErr)
		{
			myprint("WARNING: problems loading file ");
			putOutErrorMsg(fileName);
		}
	} /*if (errorFlag != noErr){}else{}*/
	fileErrorMsg(errorFlag, fileName);
	return (errorFlag);
} /*loadWindow()*/

#ifndef powerc
void myUnloadSeg(void *routineAddr)
{
	if (UNLOADSEGMENTS)
	{
		UnloadSeg(routineAddr);
	}
} /*myUnloadSeg()*/
#endif /*powerc*/
