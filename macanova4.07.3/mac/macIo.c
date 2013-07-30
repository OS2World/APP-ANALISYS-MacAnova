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
#include "macIface.h"

#define BEL '\007'

/* Macros for working with file and path names */
#ifdef MSDOS
/*  '/' equivalent to '\' on MSDOS*/
#define isSlash(C) ((C) == DIRSEPARATOR[0] || (C) == '/')
#define isPath(N) (strchr(N, '/') || strchr(N, '\\') || strchr(N, ':'))
#else /*MSDOS*/
#define isSlash(C) ((C) == DIRSEPARATOR[0])
#define isPath(N) strchr(N, DIRSEPARATOR[0])
#endif /*MSDOS*/

extern char       *TooManyErrors;


#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment Macio
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#ifndef OUTLINESIZE
#define OUTLINESIZE   400  /*size of output line buffer*/
#endif /*OUTLINESIZE*/

/* OutLineBuf is allocated by mygetpointer in main() in macMain.c */

extern char      *OutLineBuf; /* buffer for building output lines */
static long       OutLinePlace = 0;

/*
  Write line of output to file fp which may specify command window
  If writing to console, interpret leading '\007's as BEL's
  If spooling is active, write line to spool file
  If line starts with "ERROR:", ring bell and count error
*/

static void lineToWindow(char * msg,  long msgLength)
{
	long            nLines, cutLength;
	long            windno;
	long            isOutLine = (msg == OutLineBuf);
	Str255          wTitle;
	char           *outstr = (char *) wTitle;
	unsigned char  *dashes = "\p\n----------------------------------\n";
	long            teLength;
	WHERE("lineToWindow");

	if (UseWindows > 0)
	{
		if (teCmd == (TEHandle) 0)
		{
			return;
		}
		teLength = TextLength(teCmd);
	}
	while(*msg == BEL)
	{						/* turn leading BEL's into beeps */
		if(UseWindows > 0)
		{
			mybeep(1);
		}
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
			if(UndoStatus != CANTUNDO && UndoStatus != OUTPUTUNDO)
			{
				clearUndoInfo();
			}

			if(msgLength > CMDWINDOWLIMIT)
			{
				sprintf(outstr,
						"WARNING: output line longer than %ld characters is truncated",
						(long) CMDWINDOWLIMIT);
				myAlert(outstr);
				msgLength = CMDWINDOWNEWSIZE;
			}

			if (teLength + msgLength > CMDWINDOWLIMIT)
			{
				myAlert("Output window contents are too long; saving window");

				DoFile(saveitas);

				for(windno=0;windno<MAXWINDOWS;windno++)
				{
					if(CmdWindows[windno].cmdWind == (WindowPtr) 0)
					{
						break;
					}
				}

				if(windno < MAXWINDOWS)
				{				/* we have room for a new window */
					TESetSelect(teLength, teLength, teCmd);
					TEInsert(dashes+1, dashes[0], teCmd);
					outstr = (char *) "\pContinued in new window\n";
					TEInsert(outstr+1, outstr[0], teCmd);
					CmdStrPos = TextLength(teCmd);
					ScrollToInsertPt();
					GetWTitle(CmdWind, wTitle);

					clearUndoInfo();

					DoWind(-newwind); /* create new window without prompt*/

					TESetSelect(0, 0, teCmd);
					outstr = (char *) "\pOutput continued from ";
					TEInsert(outstr+1, outstr[0], teCmd);
					TEInsert(wTitle+1, wTitle[0], teCmd);
					outstr = (char *) dashes;
					TEInsert(outstr+1, outstr[0], teCmd);
				} /*if(windno < MAXWINDOWS)*/
				else
				{				/* no room, trim off top */
					CmdInsertLine = 1;
					TESetSelect(0L, 0L, teCmd);
					ScrollToInsertPt();
				
					cutLength = msgLength + teLength - CMDWINDOWNEWSIZE;
					sprintf(outstr,
							"Cannot open a new window\n%ld characters will be trimmed off the top",
							cutLength);
					myAlert(outstr);

					TESetSelect(0L, cutLength, teCmd);
					TECut(teCmd);
					macUpdate(CmdWind);
					if(UndoStatus == OUTPUTUNDO)
					{
						LastLength -= cutLength;
						LastCmdStrPos -= cutLength;
						if(LastCmdStrPos < 0)
						{
							clearUndoInfo();
						}
					} /*if(UndoStatus == OUTPUTUNDO)*/
				} /*if(windno < MAXWINDOWS){}else{}*/
			} /*if (TextLength(teCmd) > CMDWINDOWLIMIT)*/

			teLength = TextLength(teCmd);
			TESetSelect(teLength, teLength, teCmd);
			TEInsert(msg, msgLength, teCmd);
			nLines = NLines(teCmd);
			CmdInsertLine = nLines;
			CmdStrPos = TextLength(teCmd);
			CmdScrollLines = nLines + 1 - ViewLines;
			SetCtlMax(CmdScroll, (CmdScrollLines > 0 ? CmdScrollLines : 0));
			CmdWindows[CurrentWindow].cmdDirty = CmdDirty = true;
		} /*if(UseWindows > 0)*/

		if(strncmp("ERROR:",msg, (int) 6) == 0)
		{
			if(MAXERRORS1)
			{
				NERRORS++;
			}

			mybeep(1);
			if(!UseWindows && strcmp(msg, TooManyErrors) == 0)
			{
				if (!twoChoiceAlert("Go On", "Quit", "gq", msg))
				{
					if (isOutLine)
					{
						OutLinePlace = 0;
						OutLineBuf[0] = '\0';
					}
					if(!quitAlert('e'))
					{
						INTERRUPT = INTSET;
					}
				}
			}
		} /*if(strncmp("ERROR:",msg, (int) 6) == 0)*/
	} /*if (msgLength > 0)*/
	
	if (isOutLine)
	{
		OutLinePlace = 0;
		OutLineBuf[0] = '\0';
	}
} /*lineToWindow()*/

/* 970306 added */

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

/*
   Open file in appropriate mode; may do things differently on different
   platforms;
*/

#if (1)
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


#else
/* older code, pre search of PATHNAMES */
FILE * fmyopen(char * fileName, char * mode)
{
	FILE    *fp = fopen(fileName, mode);
	Integer  vRefNum, errorFlag;
	WHERE("fmyopen");
	
	if (fp == (FILE *) 0 && !isPath(fileName))
	{
		Str255     volName;
		
		/* Try again in Home directory */
		errorFlag = GetVol(volName, &vRefNum); /* save current directory */
		errorFlag = (errorFlag == noErr) ? SetVol(0L, HomeVolume) : errorFlag;
		if (errorFlag == noErr)
		{
			fp = fopen(fileName, mode);
		}
		SetVol(0L, vRefNum); /* restore current directory */
	} /*if (fp == (FILE *) 0)*/

	return (fp);
} /*fmyopen()*/
#endif

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
	{
		/* collect line in OutLineBuf */
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
			*outline++ = (c == NL) ? CR : c;
		}
		*outline = '\0';
		OutLinePlace += msgLength;
		if (OutLinePlace > 0 && OutLineBuf[OutLinePlace-1] == CR)
		{
			lineToWindow(OutLineBuf, OutLinePlace);
		}
	} /*if(fp == STDOUT)*/
	else
	{
		fputs(msg, fp);
	} /*if(fp == STDOUT){}else{}*/
} /*fmyprint()*/

/*
  terminate output line and pause if writing the console and screen is full
  (no pausing on Mac)
*/

void fmyeol(FILE * fp)
{
	if(fp == STDOUT)
	{
		if (OutLinePlace > 0)
		{ /* flush OutLineBuf */
			lineToWindow(OutLineBuf, OutLinePlace);
		}
		if(UseWindows > 0)
		{
			if(UndoStatus != CANTUNDO && UndoStatus != OUTPUTUNDO)
			{
				clearUndoInfo();
			}
			TESetSelect(TextLength(teCmd), TextLength(teCmd), teCmd);
			TEKey(Return, teCmd);
			CmdWindows[CurrentWindow].cmdDirty = CmdDirty = true;
			CmdInsertLine = NLines(teCmd);
			CmdStrPos = TextLength(teCmd);
			CmdScrollLines = NLines(teCmd) + 1 - ViewLines;
			SetCtlMax(CmdScroll, (CmdScrollLines > 0 ? CmdScrollLines : 0));
			if (CmdInsertLine - CmdLine > ViewLines - 1)
			{
				DoScroll(CmdInsertLine - ViewLines - CmdLine + 5);
			}
		} /*if(UseWindows > 0)*/
		if (SPOOLFILE != (FILE *) 0)
		{
			putc(Newline, SPOOLFILE);
		}
		if(timetocheck(DEFAULTTICKS))
		{
			SkelChkOneMaskedEvent (everyEvent ^ keyUpMask, Sleep);
		}
		if(INTERRUPT != INTNOTSET)
		{
			INTERRUPT = PRINTABORT;
		}
	} /*if(fp == STDOUT)*/
	else
	{
		putc(Newline, fp);
	}
} /*fmyeol()*/

void fmyflush(FILE * fp)
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

/*
  sound n beeps, but only if not reading batch file and output is to console
*/

void mybeep(long n)
{
	if(BDEPTH == 0 && ISATTY)
	{
		while(n-- > 0)
		{
			SysBeep(30);
		}
	}
} /*mybeep()*/

/*
	draw title in Console dialog box using system font
*/
pascal void consoleUserItem(WindowPtr theWindow, Integer item)
{
	Rect           r;
	Integer        oldFont = theWindow->txFont;
	Integer        oldSize = theWindow->txSize;
	Integer        itemType;
	Handle         itemHandle;
	unsigned char *msg = "\pEnter up to 500 characters.  Click Done to end";
	Integer        h, v;
	FontInfo       info;
	WHERE("editCommandUserItem");

	TextFont(systemFont);
	TextSize(12);
	GetFontInfo(&info);

	GetDItem((DialogPtr) theWindow, item, &itemType, &itemHandle, &r);

	h = (r.left + r.right - StringWidth( msg))/2;
	v = r.bottom - info.descent;
	MoveTo(h, v);
	DrawString( msg);

	TextFont(oldFont);
	TextSize(oldSize);
}

/*
  Fill global LINE with line from FILE *fn
  On Dos and Unix, fn should always be non-null.
  On Mac, STDIN is (FILE *) 0, and LINE is filled from dialog window when 
  fn == STDIN
*/

static Boolean        HitDone = false;
static unsigned char  LineBuffer[MAXLINE+1];
static unsigned char *Place = (unsigned char *) LineBuffer;

int fillFromDialog(void)
{
	GrafPtr               thePort;
	DialogPtr             theDialog;
	Integer               length;
	Rect                  r;
	Handle                itemHandle;
	TEHandle              textH;
	Integer               itemType, itemHit, nButtons = 2;
	Integer               doneItem = cancel;
	Integer               textItem = nButtons + 2, titleItem = textItem+1;
	unsigned char        *pc;
	Integer               teLength;
	SignedByte            flags;
	WHERE("fillLINE");

	/*
	  Use dialog window or what was read previously
	  Always terminate (*LINE) with '\n'
	  Return only through first '\n'.
	  */
	if((*LINE)[0] == '\0')
	{ /* first time in this trip */
		Place = LineBuffer;
		HitDone = false;
	}

	(*LINE)[0] = '\0';
	if(Place == LineBuffer)
	{
		if(HitDone)
		{
			HitDone = false;
			return (EOF);
		}
		/* need to read from Dialog box */
		GetPort(&thePort);
		SetDAFont(CmdFont);
		theDialog = getDlog(CONSOLEDLOG, true, nButtons, "nd");
		SetPort((GrafPtr) theDialog);
		TextFont(CmdFont);
		TextSize(CmdFontSize);
		GetDItem(theDialog, titleItem, &itemType, &itemHandle, &r);
		SetDItem(theDialog, titleItem, itemType,
				 (Handle) ConsoleUserItemPtr, &r);

		ShowWindow((WindowPtr) theDialog);

		do
		{
			ModalDialog(MyDialogFilterPtr,&itemHit);
		} while(itemHit > nButtons);

		HitDone = (itemHit == doneItem);

		/* directly access the text edit handle rather than use GetDItem() */
		textH = ((DialogPeek) theDialog)->textH;
		flags = HGetState((Handle) TextHandle(textH));
		HLock((Handle) TextHandle(textH));
		teLength = TextLength(textH);
		if(teLength > MAXLINE)
		{
			SetDAFont(systemFont);
			sprintf(OUTSTR,
					"WARNING: more than %ld characters entered; last %d ignored",
					MAXLINE, teLength - MAXLINE);
			myAlert(OUTSTR);
			teLength = MAXLINE;
		}
		if(teLength > 0)
		{
			BlockMove((Ptr) *TextHandle(textH), (Ptr) LineBuffer, teLength);
		}
		HSetState((Handle) TextHandle(textH), flags);

		LineBuffer[teLength] = '\0';
		strcpy(*LINE, (char *) LineBuffer);

		SetPort(thePort);
		SetCursor(&WATCH);
		DisposDialog(theDialog);
		SetDAFont(systemFont);
		if(LineBuffer[0] == '\0' && HitDone)
		{
			HitDone = false;
			return (EOF);
		}
	} /*if(Place == LineBuffer)*/
	else
	{
		itemHit = (HitDone) ? doneItem : ok;
		strcpy(*LINE, (char *) Place);
	}

	pc = (unsigned char *) strchr((char *) Place, '\n');
	if(pc != (unsigned char *) 0 && pc[1] != '\0')
	{ /* still something not seen */
		pc++;
		length = pc - Place;
		Place = pc;
		itemHit = ok;
	} /*if(pc != (unsigned char *) 0)*/
	else
	{ /* this is the last of it */
		length = strlen((char *) Place);
		if(pc == (unsigned char *) 0)
		{
			(*LINE)[length++] = '\n';
		}
		Place = LineBuffer;
		HitDone = false;
	} /*if(Place == LineBuffer){}else{}*/
	(*LINE)[length] = '\0';
	return ((itemHit == doneItem) ? EOF : 1);
} /*fillFromDialog()*/

#ifdef TIMEANDMEMORY
#include <time.h>
#endif /*TIMEANDMEMORY*/

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
  prompt to be supplied instead of the file name when BDEPTH > 0.
*/

void putprompt(char * prompt)
{
	char           *prmpt;
	WHERE("putprompt");

	/* write out the prompt */
	prmpt = (prompt == (char *) 0) ? PROMPT : prompt;

	if(BDEPTH == 0)
	{
		if(prompt == (char *) 0)
		{
			myeol();
		}
		myprint(prmpt);
		fmyflush(STDOUT);
		if (UseWindows > 0)
		{
			TEKey(Return, teCmd);
			TEKey(Backspace, teCmd);
			CmdStrPos = TextLength(teCmd);
			
			if (CURRENTSCROLLBACK)
			{
				Integer      linesBack =
					GetCmdChL(CmdStrPos) - GetCmdChL(LastCmdStrPos) - ViewLines + 2;

				if (linesBack > 0)
				{
					DoScroll(-linesBack);
				}
			} /*if (CURRENTSCROLLBACK)*/
			CURRENTSCROLLBACK = SCROLLBACK; /* reset to default */
		} /*if (UseWindows > 0)*/
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


#ifdef SAVEHISTORY
/* 970112 code to handle history*/

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

   If there are memory allocation problems, mygetsafehandle() or
   mygrowsafehandle() will print an error message, and history will be turned
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
   Save *lineH in history list

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
		char      **line = mygetsafehandle(nDoubles*sizeof(double));
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
static char ** getCurrentCommand(void)
{
			
	Integer      length = TextLength(teCmd) - CmdStrPos;
	char       **command;
	
	command = mygetsafehandle((length/sizeof(double) + 1)*sizeof(double));
	if (command != (char **) 0)
	{
		if (length > 0)
		{
			HLock(TextHandle(teCmd));
			BlockMove((Ptr) *TextHandle(teCmd) + CmdStrPos,
					  (Ptr) *command, (Size) length);
			HUnlock(TextHandle(teCmd));
		}
		(*command)[length] = '\0';
	}
	return (command);
} /*getCurrentCommand()*/

/*
	970112 Handle Up History and Down History commands on Edit menu
*/

char * recallHistory(unsigned char ch, Integer mods)
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

		if (ch == UpArrow)
		{
			historyOp = (mods & cmdKey) ? TOPHISTORY : UPHISTORY;
		}
		else
		{
			historyOp = (mods & cmdKey) ? BOTTOMHISTORY : DOWNHISTORY;
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
	WHERE("setSomeHistory");

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
  On Macintosh, HOME is predefined to be the full name of the MacIntosh folder.
  The usage ~username/path is not legal on a Mac
*/

static char    FullPathName[PATHSIZE];

char *expandFilename(char * fname)
{
	Symbolhandle    home;
	char           *pc;
	char           *userdir = (char *) 0;
	char           *sep1 = DIRSEPARATOR;
	char           *sep2 = "/";
	long            dirNameLength;

	*OUTSTR = '\0';
	if(fname[0] == EXPANSIONPREFIX)
	{ /* name starts with '~' */
		if(fname[1] == *sep1 || fname[1] == *sep2)
		{/* treat ~:... or ~/... specially */
			home = Lookup("HOME"); /* should be initialized at startup */
			if(home == (Symbolhandle) 0 || TYPE(home) != CHAR ||
			   !isScalar(home))
			{
				sprintf(OUTSTR,
						"ERROR: cannot expand %c%c unless CHARACTER variable HOME is defined",
						fname[0], fname[1]);
				goto errorExit;
			}
			userdir = STRINGPTR(home);
			pc = fname+2;
		}
		else
		{
/* can't expand ~name on a Macintosh */
			userdir = (char *) 0;
		}

		if(userdir == (char *) 0)
		{
			putPieces("ERROR: unable to expand ",fname,
					  (char *) 0, (char *) 0);
			goto errorExit;
		}

		dirNameLength = strlen(userdir);
/* allow for the possibility that the HOME ends in ":" */
		if(userdir[dirNameLength-1] == *sep1)
		{
			dirNameLength--;
		}
		if(dirNameLength + strlen(pc) + 1 >= PATHSIZE)
		{
			putPieces("ERROR: expanded file name ", userdir, pc,
					  " is too long");
			goto errorExit;
		}

		strcpy(FullPathName,userdir);
		FullPathName[dirNameLength++] = *sep1;
		FullPathName[dirNameLength] = '\0';
		strcat(FullPathName,pc);
	} /*if(fname[0] == EXPANSIONPREFIX)*/
	else
	{
		if(strlen(fname) > PATHSIZE)
		{
			putPieces("ERROR: filename ", fname, " is too long", (char *) 0);
			goto errorExit;
		}

		strcpy(FullPathName, fname);
	}

	return ((char *) FullPathName);

  errorExit:
	putOUTSTR();

	return ((char *) 0);

} /*expandFilename()*/

/*
  On Macintosh accept non-empty names of the form
       pathElement or pathElement:pathElement or pathElement:...:pathElement
  where a pathElement does not contain '/' but can be empty
  If path != 0, only restriction is starting with '.'
  If path == 0, it should be file name not ending in ':'
*/
static int filenameCharsOK(char * fname)
{
	return (1);
} /*filenameCharsOK()*/

static int filenameFormOK(char * fname, int path)
{
	int        ok = (fname[0] != '.' && strlen(fname) <= PATHSIZE &&
					 (path || !isSlash(fname[strlen(fname)-1])));
	char      *sepPlace;
	
	for (sepPlace = fname - 1; ok && sepPlace != (char *) 0;
		 sepPlace = strchr(sepPlace, DIRSEPARATOR[0]))
	{
		ok = *(++sepPlace) != '.';
	}
	return (ok);
} /* filenameFormOK()*/

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

/*
  Check file names for legality on platform
  On the Mac, essentially any name is legal as long as it is not too
  long and does not start with '.' (names starting with '.' may be interpreted
  as driver names by the OS)
*/

/* Check filename with error messages */
long isfilename(char *fname)
{
	if (okfilename(fname))
	{
		return (1);
	}

	*OUTSTR = '\0';
	if(*fname == '\0')
	{
		sprintf(OUTSTR,"ERROR: zero length file name");
	}
	else if(strlen(fname) >= PATHSIZE)
	{
		putPieces("ERROR: file name ", fname, " is too long", (char *) 0);
	}
	else if(*fname == '.')
	{
		sprintf(OUTSTR, "ERROR: file names must not start with '.'");
	}
	else
	{
		putPieces("ERROR: improper file name ", fname, (char *) 0, (char *) 0);
	}

	putOUTSTR();
	
	return (0);
} /*isfilename()*/

