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
#pragma segment Macinput
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


/*
  Version modified for use with MacAnova
  911222
  Version of 920129
  Version of 920810 -- getinput modified to match changes in yylex1()
  Version of 930812 for use with Version 3.3 of MacAnova
  940317 major changes made in undo
  940331 Started multi-window modifications

  940429 Modified to work with TransSkel version 3.12
  CmdWindMain is now a general idle procedure and is not tied to any particular
  window
  950702 Modified getinput to be same as Unix version (with ifdefs's) and
  moved it to macIo.c
  960718 Added copyandexecute to edit menu, made F5 and F6 equivalent to copytoend
         and copyandexecute, removed use of F5-F12 to invoke Command Menu items
  960719 Made copytoend undoable
  970113 Interfaced with history mechanism
  970425 Fixed bug in back spacing at start of window (CmdStrPos was updated erroneously)
         Made del key work like on other Mac applications, deleting forward
         No change in undo status on backspace or delete that does nothing
  971201 Made changes so that watch cursor appears more consistently when
         in running or batch mode.
  980516 When BDEPTH > 0, CmdWindMain() retains control until timetocheck()
         returns non-zero.  I similar change made dramatic speedup in
         BatchModeMain(); less is expected here unless echoing is suppressed
  980825 Modified setCmdHelpItems() and setCmdFileItems() to refer to HelpMenu
         instead of AppleMenu
*/

/*
	Evaolved from a TransSkel multiple-window demonstration: TextEdit module

	14 June 1986		Paul DuBois
*/
#ifdef PERFORMANCE
#include "profile.h"
#endif /*PERFORMANCE*/

#include "globals.h"
#include "mainpars.h"
#include "macIface.h" /* Note: macIface.h replaces MultiSkel.h */

extern int      yydebug; /* set 1 for yacc debugging output */

#ifndef CMDWINDOWLIMIT /* Should be defined in macIface.h; also used in macIo.c */
#define CMDWINDOWLIMIT   32000  /* threshhold for window to be full */
#endif /*CMDWINDOWLIMIT*/

#define NAHEAD    100 /* number of type ahead saved */
#define Text      (*TextHandle(teCmd))

/* Referenced in CmdWindMain and LineToWindow in macIo.c */
char         *TooManyErrors = "ERROR: too many errors; execution terminated";

static struct
{
	unsigned char      ch;
	unsigned char      code;
	Integer            mods;
} typeAhead[NAHEAD];

static Integer Nahead = 0; /* number of type ahead characters to process */

static Integer         PlaceInLine = 0;
                       /* most recent position in line of insertion point */
static unsigned char   LastChar = '\0';
                       /* last character processed by CmdWindKey */

/*
	Global UndoStatus contains information about the possibility of undoing
	or redoing.  Specific values are
		UndoStatus			Situation
		CANTUNDO            No undo possible
		TYPINGUNDO          Undo typing possible
		TYPINGREDO          Redo typing possible
		OUTPUTUNDO          Undo output after command possible
		OUTPUTREDO          Redo output after command possible
		DELETEUNDO          Undo deletion or replacement of selection possible
		PASTEUNDO           Undo paste possible
		CUTUNDO             Undo cut possible
*/

/*
	The following are defined in macIface.h to preserve information for
	undoing and redoing

Integer         UndoPlace = -1; [* start of change *]
Integer         PasteLength = -1; [* length of paste *]
Handle          CmdScrap = (Handle) 0; [* handle to save stuff*]

[* OldCmdStrPos is CmdStrPos at time of a deletion, replacement, cut, or paste*]
LongInt         OldCmdStrPos = -1;
LongInt         LastTypingPos = -1;

Integer         LastLength = -1;
Integer         LastCmdStrPos = -1;
	When UndoStatus == OUTPUTUNDO, LastLength is the length of the buffer,
	less 1 for the trailing newline, just before a command line is executed,
	and LastCmdStrPos is the value of CmdStrPos at that time.  
*/

/*
	window handler procedures
*/

/*
	960811 modified call to startupMsg() to have argument quiet
*/
void CmdWindInit(Boolean quiet)
{
	Integer      windno;
	WHERE("CmdWindInit");

	SkelSetIdle((SkelIdleProcPtr) CmdWindMain);
	windno = createWindow((STR255) 0);
	if (windno < 0)
	{
		myAlert("Cannot create command window");
		FatalError = 1;
		return;
	}
	CmdWindows[0].menuItem = cmd1;
	CmdWindows[0].cmdEditable = true;
	restoreWindowInfo(windno);
	setCommandM(windno);
	Running = 1;
	setCmdFileItems(false);

	BATCHECHO[0] = 0;

	startupMsg(quiet);
	UNLOADSEG(startupMsg);
	UndoStatus = !CANTUNDO;
	clearUndoInfo();

	UNLOADSEG(initialize);
} /*CmdWindInit()*/


pascal void CmdWindClobber(void)
{
	Integer          windno;
	GrafPtr          thePort;
	cmdWindowInfoPtr wp;
	Integer          currentWindow = CurrentWindow;

	GetPort(&thePort);
	windno = whichCmdWindow((WindowPtr) thePort);
	if (windno >= 0)
	{
		wp = CmdWindows + windno;
		if (windno == CurrentWindow)
		{
			clearUndoInfo();
		}
		else if (wp->cmdScrap != (Handle) 0)
		{
			DisposHandle(wp->cmdScrap);
		}
		TEDispose(wp->teCmd);
		DisposeWindow(wp->cmdWind);
		clearWindowInfo(windno);
	} /*if (windno >= 0)*/
} /*CmdWindClobber()*/

/* check whether input is complete */

/*
  970203 Modified so that it should work if anything equivalent to
         system() is available in the Macintosh version so that lines
         starting with ! are treated specially.
*/
Integer inputReady(void)
{
	Integer           ichar, inquotes, bracklvl, foundComment;
	Integer           teLength = TextLength(teCmd);
	Handle            hText = TextHandle(teCmd);
	char              c;
#ifdef RECOGNIZEBANG
	int               foundbang;
#endif /*RECOGNIZEBANG*/

	if (INPUTFILE[BDEPTH] != STDIN)
	{ /* always ready from not getting input from the command window */
		goto ready;
	}
	
#ifdef RECOGNIZEBANG
	foundbang = (*hText)[CmdStrPos] == BANG;
#endif /*RECOGNIZEBANG*/

	for (ichar = teLength-1;ichar >= CmdStrPos;ichar--)
	{ /* make sure line effectively terminates with '\n' */
		c = (*hText)[ichar];
		if (c == '\n')
		{
#ifdef RECOGNIZEBANG
			/* Just in case shell() is ever implemented on Mac */
			if (foundbang && ichar > 0 && (*hText)[ichar-1] == '\\')
			{
				goto notReady;
			}
#endif /*RECOGNIZEBANG*/
			break;
		}
		else if (c != ' ' && c != '\t')
		{ /* still waiting for '\n'; not ready yet */
			goto notReady;
		}
	} /*for (ichar = teLength-1;ichar >= CmdStrPos;ichar--)*/

	if (c != '\n')
	{
		goto notReady;
	}
#ifdef RECOGNIZEBANG
	else if (foundbang)
	{
		goto ready;
	}
#endif /*RECOGNIZEBANG*/

	inquotes = 0;
	bracklvl = 0;
	foundComment = 0;
	
	ichar = CmdStrPos;
	while (ichar < teLength && ichar >= CmdStrPos)
	{ /* now check balance of '{' and '}', keeping track of quotes */
		c = (*hText)[ichar++];

		if (c == '\\')
		{
			ichar++;
		}
		else if (!inquotes || c == '"')
		{
			switch((int) c)
			{
			  case '"':
				if (!foundComment)
				{
					inquotes = !inquotes;
				}
				break;
			  case '{':
				if (!foundComment)
				{
					bracklvl++;
				}
				break;
			  case '}':
				if (!foundComment)
				{
					bracklvl--;
				}
				break;
			  case '#':
				foundComment = 1;
				break;
		  	  case LF:
		  	  case CR:
				if ( bracklvl <= 0 )
				{
					goto ready;
				}
				foundComment = 0;
			  default:
				break;
			} /*switch((int) c)*/
		} /*if (c == '\\'){...}else if (!inquotes || c == '"'){...}*/
	} /*while (ichar < teLength && ichar >= CmdStrPos)*/
	/* if we get to here, not ready yet */
	/* fall through */
	
  notReady:
	return (0);

  ready:
	return (1);
} /*inputReady()*/

/*
	971201 Added following to make it easier to display watch consistently
*/
static void setCmdCursor(Point * thePt, TEHandle teCmd)
{
	if (Running || BDEPTH > 0)
	{
		SetCursor(&WATCH);
	}
	else
	{
		Point     mouseLoc;

		if (thePt)
		{
			mouseLoc = *thePt;
		}
		else
		{
			GetMouse(&mouseLoc);
		}	
		if (PtInRect(mouseLoc,&ViewRectangle(teCmd)))
		{
			SetCursor(&IBEAM);
		}
		else
		{
			InitCursor();
		}
	}
} /*setCmdCursor()*/

/*
	General control routine for non-batch mode MacAnova.  It is now installed
	as the TransSkel idle process and is not specifically associated with
	a window
*/

pascal void CmdWindMain(void)
{
	Integer         done = 0;
	Integer         i;
	Integer         inputStatus = 0;
	GrafPtr         thePort;
	Boolean         infront = (FrontWindow() == CmdWind);
	Boolean         inforeground = SkelQuery(skelQInForeground);
	WHERE("CmdWindMain");		
	
	if (!UseWindows)
	{
		return;
	}
	GetPort(&thePort);
	if (infront)
	{
		SetPort((GrafPtr) CmdWind);
	}
		
		
	if (INTERRUPT != INTNOTSET)
	{
		NERRORS++;
		if (BDEPTH > 0)
		{
			putOutErrorMsg("ERROR: batch file(s) terminated");
			closeBatch(1); /* shut down all batch files*/
		}
		putprompt((char *) 0);
		INTERRUPT = INTNOTSET;
	} /*if (INTERRUPT != INTNOTSET)*/

	if (infront && inforeground)
	{
		TEIdle(teCmd);		/* blink that cursor! */
		setCmdCursor((Point *) 0, teCmd);
		setCmdFileItems(infront);
		setCmdEditItems();

	/*	ViewLines = (r.bottom - r.top) / LineHeight(teCmd);*/
	
		CmdScrollLines = NLines(teCmd) + 1 - ViewLines;
	
		SetCtlMax(CmdScroll, (CmdScrollLines > 0 ? CmdScrollLines : 0));
		if (CmdScrollLines < 0 && CmdLine > 0)
		{
			DoScroll(-CmdLine);
		}
	} /*if (infront && inforeground)*/
	else
	{
		DisableItem(EditMenu,undoit);
	}

	while ((BDEPTH > 0 || inforeground && TextLength(teCmd) > CmdStrPos ) && inputReady() &&
		(inputStatus = getinput()) > 0)
	{ /* we have an input line, EOF on batch file, or out of memory*/
		if (INPUTSTRING != (unsigned char **) 0)
		{ /* not out of memory */
			if (BDEPTH <= 0)
			{
				NLINES = 0;
				if (ISATTY && MAXERRORS == 0)
				{
					NERRORS = 0;
				}
				clearUndoInfo();
				if (BDEPTH == 0)
				{
					/* save info in case needed for undoing output  */
					LastLength = TextLength(teCmd);
					LastCmdStrPos = CmdStrPos;
					UndoStatus = OUTPUTUNDO;
				}
			} /*if (BDEPTH <= 0)*/
			
			if ((*INPUTSTRING)[0] != '\0')
			{
				if (!Running)
				{
					DisableItem(FileMenu, restoreit);
					DisableItem(FileMenu, spoolit);
					DisableItem(FileMenu, batchit);
					DisableItem(EditMenu, copyandexecute);
#ifdef SAVEHISTORY
					DisableItem(EditMenu, forwardhistory);
					DisableItem(EditMenu, backwardhistory);
#endif /*SAVEHISTORY*/
					DisableItem(WindowMenu, hideit);
					DisableItem(CommandMenu, (Integer) 0);
					DisableItem(OptionsMenu, (Integer) 0);
					DisableItem(FontMenu, (Integer) 0);
					DisableItem(FontSizeMenu, (Integer) 0);
					if (inforeground)
					{
						DrawMenuBar();
					}
				} /*if (!Running)*/
				Running = 1;

				setCmdCursor((Point *) 0, teCmd);
				if (BDEPTH <= 0)
				{
					BDEPTH = 0;
					/* Note Undo on menu is not enabled at this time*/
					setCmdFileItems(false);
				}
				
				if (inputStatus > 1)
				{
					putOutErrorMsg("WARNING: extra characters after end of line");
				}
#ifdef PERFORMANCE
				if (GUBED & 32768 && (!ProfileStatus || ProfileStatus == PROFILEOFF))
				{
					(void) PerfControl(ThePGlobals, true);
					ProfileStatus = PROFILEON;
				}
				if (!(GUBED & 32768) && ProfileStatus == PROFILEON)
				{
					(void) PerfControl(ThePGlobals, false);
					ProfileStatus = PROFILEOFF;
				}
#endif /*PERFORMANCE*/
	
/*
  lines starting with '!' are now recognized on Macintosh
  but only result in an error message
*/
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
	
#ifdef PERFORMANCE
				if (ProfileStatus == PROFILEON)
				{
					(void) PerfControl(ThePGlobals, false);
					ProfileStatus = PROFILEOFF;
				}
#endif /*PERFORMANCE*/
	
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
			infront = (CmdWind == FrontWindow());
			if (!infront)
			{
				GetPort(&thePort);
			}
			if (!done)
			{
				if (infront && BDEPTH == 0 && UndoStatus == OUTPUTUNDO)
				{
					setCmdEditItems();
				}
			}
		} /*if (INPUTSTRING != (unsigned char **) 0)*/
		else
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
				else if (!ISATTY && MAXERRORS && NERRORS >= MAXERRORS)
				{
					putOutErrorMsg(TooManyErrors);
					done = FATALERROR;
					FatalError = 1;
				}
			} /*if (MAXERRORS1 && NERRORS >= MAXERRORS1)*/
			NERRORS = (BDEPTH <= 0 && ISATTY) ? 0 : NERRORS;
		} /*if (!done)*/

		if (done && !FatalError)
		{
			if (quitIt() == cancel)
			{
				done = 0;
			}
		}
		if (!done)
		{
			if (BDEPTH <= 0)
			{
				Running = 0;
				if (infront)
				{
					EnableItem(FileMenu, restoreit);
					if (Nwindows < MAXWINDOWS)
					{
						EnableItem(WindowMenu,newwind);
					}
					if (nVisible() > 1)
					{
						EnableItem(WindowMenu,hideit);
					}
				} /*if (infront)*/
				if (SPOOLFILENAME != (char **) 0)
				{
					unsigned char   *menuEntry;

					if (SPOOLFILE != (FILE *) 0)
					{
						menuEntry = "\pStop Spooling";
					}
					else
					{
						menuEntry = "\pResume Spooling";
					}
					SetItem(FileMenu,spoolit,menuEntry);
				} /*if (SPOOLFILENAME != (char **) 0)*/
				EnableItem(FileMenu, spoolit);
				EnableItem(FileMenu, batchit);
				EnableItem(CommandMenu, (Integer) 0);
				EnableItem(OptionsMenu, (Integer) 0);
				EnableItem(FontMenu, (Integer) 0);
				EnableItem(FontSizeMenu, (Integer) 0);
				if (inforeground)
				{
					DrawMenuBar();
				}
			} /*if (BDEPTH == 0)*/
			
			if (INTERRUPT != INTNOTSET && BDEPTH > 0)
			{
				putOutErrorMsg("ERROR: batch file(s) terminated");
				closeBatch(1); /* shut down all batch files*/
				INTERRUPT = INTNOTSET;
			} /*if (INTERRUPT != INTNOTSET && BDEPTH > 0)*/
			
			putprompt((char *) 0); /* This is basic prompt */

			yydebug = (GUBED & 2048) ? 1 : 0;

			if (!Running)
			{
				for (i=0;i<Nahead;i++)
				{ /* process type ahead */
					CmdWindKey(typeAhead[i].ch, typeAhead[i].code,
							   typeAhead[i].mods);
				} /*for (i=0;i<Nahead;i++)*/
				Nahead = 0;
			} /*if (!Running)*/
		} /*if (!done)*/
		if (BDEPTH <= 0 || timetocheck(DEFAULTTICKS))
		{
			break;
		}
	} /* while (we have an input line or have hit EOF on batch file)*/
	FatalError = (inputStatus < 0) ? 1 : FatalError;
	if (FatalError != 0)
	{ /* panic quit, run out of space */
		(void) quitIt();
	}
	if (!infront)
	{
		SetPort(thePort);
	}
} /*CmdWindMain()*/

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
	}
	if (BDEPTH <= 0)
	{
		Running = 0;
		MAXERRORS1 = MAXERRORS;
	}
} /*closeBatch()*/

/*
	Function to add command at end of command window
	If the last character is not Newline then:
		If the command ends '()' then
			the insertion point is left between ( and )
		Else if the commend ends '("")'then
			the insertion point is left between quotes
		Else
			the insertion point is left at the end of the command
	Else
		the insertion point is left after the newline
*/

void insertCmd(char *command, Boolean replace)
{
	LongInt       cmdLength = strlen(command);
	LongInt       place = TextLength(teCmd);
	Boolean       addNL = false;

	if (replace && place > CmdStrPos)
	{ /* clear out anything after CmdStrPos */
		TESetSelect(CmdStrPos, place, teCmd);
		TEDelete(teCmd);
		place = CmdStrPos;
		UndoStatus = CANTUNDO;
	}
	if (UndoStatus != TYPINGUNDO || LastTypingPos != place)
	{
		clearUndoInfo();
		LastTypingPos = UndoPlace = place;
		UndoStatus = TYPINGUNDO;
	}
	LastTypingPos += cmdLength;

	if (cmdLength > 0 && command[cmdLength-1] == Newline)
	{
		addNL = true;
		cmdLength--;
	}
	TESetSelect(place, place, teCmd);
	TEInsert((Ptr) command, cmdLength,teCmd);
	if (addNL)
	{
		TESetSelect(TextLength(teCmd), TextLength(teCmd), teCmd);
		TEKey(Return, teCmd);
	}
	place = TextLength(teCmd);
	/* put cursor between trailing '()' or inside trailing '("")' */
	if (!addNL && command[cmdLength-1] == ')')
	{
		if (command[cmdLength-2] == '(' || command[cmdLength-2] == ',')
		{
			place--;
		}
		else if (command[cmdLength-4] == '(' && command[cmdLength-3] == '"' &&
					command[cmdLength-2] == '"')
		{
			place -= 2;
		}
	}
	TESetSelect(place, place, teCmd);
	CmdInsertLine = GetCmdIL();
	ScrollToInsertPt();
	if (FrontWindow() == CmdWind)
	{
		setCmdEditItems();
	}
} /*insertCmd()*/


Integer DoStats(void)
{

	SkelSetWaitTimes(Sleep, BackgroundSleep);
	return ((INPUTSTRING != (unsigned char **) 0) ? yyparse() : 0);

} /*DoStats()*/

/*
	Process key stroke in Command Window
*/

#ifndef MATCHDELAY
#define MATCHDELAY     20
#endif /*MATCHDELAY*/
static char     *LParens = "({[", *RParens = ")}]";

static void matchParen(unsigned char cRight)
{
	unsigned char   cLeft, c;
	Integer         selStart = SelStart(teCmd), place = selStart - 1;
	Integer         limit = (selStart >= CmdStrPos) ? CmdStrPos : 0;
	Integer         count = 0;
	LongInt         tickCount = TickCount();
	
	cLeft = LParens[strchr(RParens, cRight) - RParens];

	do
	{
		c = Text[place--];

		if (strchr(RParens,c) != (char *) 0)
		{
			count++;
		}
		else if (strchr(LParens,c) != (char *) 0)
		{
			count--;
		}
	} while (place >= limit && count > 0);
	place++;
	if (count == 0 && c == cLeft)
	{
		TESetSelect(place, place+1, teCmd);
		while (TickCount() - tickCount < MATCHDELAY)
		{
			;
		}
	}
	else
	{
		mybeep(1);
	}
	TESetSelect(selStart, selStart, teCmd);
} /*matchParen()*/

/*
	970919 Enabled Shift Return and Cmd Return to act same as Enter
*/
pascal void CmdWindKey(Integer ch1, Integer code, Integer mods)
{
	unsigned char  ch = (unsigned char) ch1;
	Boolean        moveBack = false;
	LongInt        selStart = SelStart(teCmd), selEnd = SelEnd(teCmd);
	LongInt        teLength = TextLength(teCmd);
	WHERE("CmdWindKey");

	if (mods & cmdKey && ch == Period)
	{ /* treat like File menu Interrupt */
		DoFile(interrupt);
		return;
	}

	if (CmdEditable && Running)
	{ /* currently running a command */
		if (Nahead < NAHEAD)
		{ /* save info for later processing */
			typeAhead[Nahead].ch = ch;
			typeAhead[Nahead].code = (unsigned char) code;
			typeAhead[Nahead++].mods = mods;
		}
		return;
	} /*if (CmdEditable && Running)*/
	/*981113 Enter with any modifiers is still treated as Enter */
	if (ch == Return && mods & (cmdKey|shiftKey) || ch == Enter)
	{ /* Shift Return and Command Return is same as Enter*/
		ch = Enter;
		mods = 0;
	} /* (ch == Return && mods & (cmdKey|shiftKey))*/
/*
	Note: all the function keys on the extended key board have ch == 0x10
*/
	if (mods & cmdKey || (ch < Space && ch != Backspace && ch != Return &&
		ch != Enter && ch != TabKey && ch != DeleteKey))
	{ /* handle arrow keys, etc */
		if (ch == HelpKey || mods & cmdKey && ch == 'h')
		{ /* treat like File menu Help */
			if (!Running && BDEPTH <= 0)
			{
				clearUndoInfo(); /* no undo possible */
				DoApple(helpit);
			}
		}
		else
		{
			LastChar = ch = nonTextKey(ch, code, mods);
		}
		setCmdEditItems();
		return;
	}

	if (!CmdEditable)
	{ /* this window is not editable; do nothing */
		return;
	}

#if (0)  /* fossil debugging related code */
	LastLength = LastCmdStrPos = 0;
	if (ch == '~' && GUBED & 64)
	{
		ALERT("selStart = %d, selEnd = %d, CmdInsertLine = %d, CmdStrPos = %d\n",
		   selStart,selEnd,	CmdInsertLine,CmdStrPos); return;
	}
	{
		int          i;
		WindowPeek   theWindow;
		WStateData **WStateHandle;
		Rect         r;
		
		for (i=0;i<4;i++)
		{
			theWindow = (WindowPeek)GraphWind[i];
			if (theWindow != (WindowPeek) 0)
			{
				WStateHandle = (WStateData **) theWindow->dataHandle;
				r = (*WStateHandle)->userState;
				ALERT("userState.{top,left,bottom,right} = %d, %d, %d, %d",
					   r.top, r.left, r.bottom, r.right);
				r = (*WStateHandle)->stdState;
				ALERT("stdState.{top,left,bottom,right} = %d, %d, %d, %d",
					   r.top, r.left, r.bottom, r.right);
			}
		}
		return;
	}
#endif /*(0)*/

	if (ch == DeleteKey)
	{ /* Delete key deletes forward (970425) */
		if (selStart == selEnd)
		{
			if (selStart == teLength || selStart == CmdStrPos - 1)
			{
				return;
			}
			TESetSelect(++selStart, ++selEnd, teCmd);
		}
		ch = Backspace;
	} /*if (ch == DeleteKey)*/
	
	if (selStart != selEnd)
	{ /* whatever the key it will clear or replace something */
		clearUndoInfo();
	}
	else if (ch == Backspace)
	{
		if (selStart == 0 || selStart == CmdStrPos)
		{
			return;
		}
		moveBack = true;
	}

/*
	If we get here, then key will change command window
*/

	CmdWindows[CurrentWindow].cmdDirty = CmdDirty = true;

	if (UndoStatus != TYPINGUNDO && UndoStatus != DELETEUNDO)
	{
		clearUndoInfo(); /* forget undo information */
		UndoPlace = selStart;
		if (selStart == selEnd)
		{
			UndoStatus = TYPINGUNDO;
		}
	} /*if (UndoStatus != TYPINGUNDO && UndoStatus != DELETEUNDO)*/

	if (ch == Enter)
	{ /* treat as Return at end */
		ch = Return;
		if (selStart < CmdStrPos && selStart < selEnd)
		{ /* copy selection to end before adding Return; don't put on clipboard */
			TECopy(teCmd);
#if (0) /* don't put on Clip Board */
			(void) ZeroScrap();
			(void) TEToScrap();
			(void) TEFromScrap();
#endif /*0*/
			TESetSelect(teLength, teLength, teCmd);
			TEPaste(teCmd);
		}
		teLength = selStart = selEnd = TextLength(teCmd);
		TESetSelect(teLength, teLength, teCmd);
		CmdInsertLine = NLines(teCmd);
		clearUndoInfo(); 
	} /*if (ch == Enter)*/

	if (UndoStatus == TYPINGUNDO || UndoStatus == DELETEUNDO)
	{
		if (selStart != LastTypingPos)
		{ /* we've moved! */
			clearUndoInfo();
			UndoStatus = TYPINGUNDO;
			UndoPlace = selStart;
		}
		if (ch == Backspace && selStart <= UndoPlace)
		{
			clearUndoInfo();
		}
	} /*if (UndoStatus == TYPINGUNDO || UndoStatus == DELETEUNDO)*/

	OldCmdStrPos = CmdStrPos;

	/* first reset CmdStrPos */
	if (selStart < CmdStrPos)
	{			/*only adjust CmdStrPos if before it */
		if (selEnd <= CmdStrPos)
		{
			CmdStrPos -= selEnd - selStart;
			if (ch != Backspace)
			{
				CmdStrPos++;
			}
			else if (selStart == selEnd)
			{
				
				CmdStrPos--;
			}
		}
		else
		{ /* selstart < CmdStrPos && selEnd > CmdStrPos */
			CmdStrPos = selStart;
		}
	} /*if (selStart < CmdStrPos)*/

	if (selStart != selEnd)
	{
		toMyScrap(); /* save what is being replaced */
		TEDelete(teCmd);
		selEnd = selStart;
		UndoStatus = DELETEUNDO; /* deletion or replacement can be undone */
	}

	if (ch != Backspace || moveBack)
	{ /* don't allow backspacing over CmdStrPos */
		TEKey(ch, teCmd);
	}
	LastTypingPos = SelStart(teCmd);
	if (ch == Return)
	{
		CmdInsertLine++;
	}
	else if (ch == Backspace)
	{
		CmdInsertLine = GetCmdIL();
	}
	else if (selStart >= LineStarts(teCmd)[NLines(teCmd)-1])
	{
		CmdInsertLine = NLines(teCmd);
	}
	if (CmdInsertLine <= CmdLine || CmdInsertLine > CmdLine + ViewLines)
	{
		ScrollToInsertPt();
	}
	if (ch == Backspace && selStart != UndoPlace)
	{
		/*
			all that is really needed is a non-backspace
			printable character like 'a'
		*/
		ch = Text[selStart];
	}
	else if (strchr(RParens, ch))
	{
		matchParen(ch);
	}
	LastChar = ch;
	setCmdEditItems();
} /*CmdWindKey()*/

#define LastLineStart              starts[0]
#define ThisLineStart              starts[1]
#define NextLineStart              starts[2]
#define LineAfterNextLineStart     starts[3]

#define setCurrentCommand(L) insertCmd(L, true)

/*
	Handle arrow keys, home key, page up and down keys
	None of the keys handled changes the buffer in any way
	960718 disabled used of Function keys to run items from Command Menue
	       set F5 and F6 to select Copy To End and Execute
	970112 added support for history (option UpArrow and option DownArrow)
           With Command Key, these move to the earliest available saved
		   input line or to the most recently saved line.
	970919 Enabled Cmd Function keys to switch to graph windows (like Cmd 1, ... Cmd 8)
*/
unsigned char nonTextKey(unsigned char ch, unsigned char code, Integer mods)
{
	LongInt       selStart, selEnd, selStartOrig, selEndOrig;
	Integer       teLength, nLines;
	Integer       starts[4];
	Integer       insertLine, insertLineOrig;
	Integer       cmdNo;
	Integer       editItem = -1, commandItem = -1, windowItem = -1;
	unsigned char c;
	WHERE("nonTextKey");
/*
	Note LastChar and PlaceInLine are static variables
	CmdLine is the number of lines before the top line in the window
	CmdInsertLine is the line of the insertion point, starting with 1
*/

#ifdef SAVEHISTORY
	if (mods & optionKey && (ch == UpArrow || ch == DownArrow) ||
		 !(mods & cmdKey) && ch == FUNCTIONKEY && (code == F7 || code == F8))
	{
		char     *line;

		if (ch == FUNCTIONKEY)
		{
			ch = (code == F7) ? UpArrow : DownArrow;
			mods |= optionKey;
		}
		line = recallHistory(ch, mods);
		if (line != (char *) 0)
		{
			setCurrentCommand(line);
			teLength = TextLength(teCmd);
			TESetSelect(teLength, teLength, teCmd);
		}
		return ('\0');
	}
#endif /*SAVEHISTORY*/

	if (ch == FUNCTIONKEY)
	{
		cmdNo = functionKeyToNumber(code);
		if (!(mods & cmdKey))
		{
			switch (cmdNo)
			{
			  case 1: /*F1 = undo*/
				editItem = undoit;
				break;
		
			  case 2: /*F2 = cut*/
				editItem = cutit;
				break;
		
			  case 3: /*F3 = copy*/
				editItem = copyit;
				break;
		
			  case 4: /*F4 = paste*/
				editItem = pastit;
				break;
		
			  case 5: /*F5 = copytoend*/
				editItem = copytoend;
				break;
		
			  case 6: /*F6 = copyandexecute*/
				editItem = copyandexecute;
				break;
	
			  default:
				;
			} /*switch (cmdNo)*/
		} /*if (!(mods & cmdKey))*/
		else
		{
			switch (cmdNo)
			{
			  case 1:
				windowItem = graph1;
				break;
		
			  case 2:
				windowItem = graph2;
				break;
		
			  case 3:
				windowItem = graph3;
				break;
		
			  case 4:
				windowItem = graph4;
				break;
#if (NGRAPHS > 4)		
			  case 5:
				windowItem = graph5;
				break;
		
			  case 6:
				windowItem = graph6;
				break;
	
			  case 7:
				windowItem = graph7;
				break;
		
			  case 8:
				windowItem = graph8;
				break;
#endif /*NGRAPHS > 4*/
#if (NGRAPHS > 8)
			  case 9:
				windowItem = graph9;
				break;
		
			  case 10:
				windowItem = graph10;
				break;
	
			  case 11:
				windowItem = graph11;
				break;
		
			  case 12:
				windowItem = graph12;
				break;
#endif /*NGRAPHS > 8*/
	
			  default:
				;
			} /*switch (cmdNo)*/
		} /*if (!(mods & cmdKey)){}else{}*/

		if (editItem >= 0)
		{
			DoEdit(editItem);
		}
		else if (windowItem >= 0)
		{
			DoWind(windowItem);
		}
		return ('\0');
	} /*if (ch == FUNCTIONKEY)*/
/*
	LastLineStart = start of line before current line (or 0)
	ThisLineStart = start of current line (or just after last character)
	NextLineStart = start of next line (or just after last character)
	LineAfterNextLineStart = start of line after next line (or just after last character)
*/
	selStartOrig = selStart = SelStart(teCmd);
	selEndOrig = selEnd = SelEnd(teCmd);
	teLength = TextLength(teCmd);
	nLines = NLines(teCmd);
	insertLineOrig = insertLine = CmdInsertLine;

	LastLineStart = (insertLine > 1) ?
		LineStarts(teCmd)[insertLine-2] : 0;
	ThisLineStart = LineStarts(teCmd)[insertLine-1];
	NextLineStart = (insertLine  < nLines) ?
		LineStarts(teCmd)[insertLine] : teLength+1;
	LineAfterNextLineStart = (insertLine  < nLines - 1) ?
		LineStarts(teCmd)[insertLine+1] : teLength+1;
/*
	Some or all of these may be impossible because they are handled by menu
	handler and never passed on.  It is possible, however, that a menu key
	intended for a Desk Accessory might get here, since the result returned
	by MenuKey doesn't allow one to tell if the command key was on a DA
	menu.  This should not be possible under Multifinder or System 7 unless
	the user has selected the DA with the option key depressed.
*/

	if (mods & cmdKey)
	{
		if ((cmdNo = optionKeyToNumber(ch)) > 0)
		{
			if (!Running)
			{
				DoCommand(command1+cmdNo-1); /*simulate menu selection */
			}
			return (ch);
		}

		if (ch == OptionQ)
		{
			if (Running)
			{
				return (ch);
			}
			/* Quit without chance to save things */
			SaveOnQuit = 0;
			(void) quitIt();
			return ('\0');
		}
		
		if (ch == OptionW)
		{
			if (Running)
			{
				return (ch);
			}
			
			if (Nwindows > 1)
			{
				CmdWindClose();
			}
			return ('\0');
		} /*if (ch == OptionW)*/		

		/* Remaining possibilities have to do with motion */
		switch ((int) ch)
		{
		  case 't':
		  case 'T':
			ch = HomeKey;
			break;

		  case 'e':
		  case 'E':
			ch = EndKey;
			break;

		  case 'u':
		  case 'U':
			ch = Pageup;
			break;

		  case 'd':
		  case 'D':
			ch = Pagedown;
			break;

		/* Command B (back) is equivalent to LeftArrow */
		  case 'b':
		  case 'B':
		  case OptionB: 
			mods = (ch == OptionB) ? optionKey : 0;
			ch =  LeftArrow;
			break;

		/* Command F (forward) is equivalent to RightArrow */
		  case 'f':
		  case 'F':
		  case OptionF: 
			mods = (ch == OptionF) ? optionKey : 0;
			ch = RightArrow;
			break;

		  case 'a':
		  case 'A':
			ch = LineStart;
			break;

		  case LeftArrow:
		  case RightArrow:
		  case DownArrow:
		  case UpArrow:
			break;

		  default:
			ch = '\0';
		} /*switch ((int) ch)*/
	} /*if (mods & cmdKey)*/

	if (ch != LastChar)
	{
		PlaceInLine = selStart - ThisLineStart;
	}
	
	switch ((int) ch)
	{
	  case HomeKey:
		selStart = 0;
		insertLine = 1;
		break;

	  case EndKey:
		selStart = teLength;
		insertLine = nLines;
		break;

	  case Pageup:
		insertLine = CmdLine + 2 - ViewLines;
		if (insertLine < 1)
		{
			insertLine = 1;
		}
		selStart = LineStarts(teCmd)[insertLine-1];
		break;

	  case Pagedown:
		insertLine = CmdLine + 2*ViewLines - 4;
		if (insertLine >= nLines)
		{
			insertLine = nLines;
			selStart = teLength;
		}
		else
		{
			selStart = LineStarts(teCmd)[insertLine-1];
		}
		break;

	  case LeftArrow:
		if (selStart > 0)
		{
			if (mods & cmdKey)
			{
				selStart = ThisLineStart;
			}
			else if (mods & optionKey)
			{
				while (selStart > 0 && !isnamechar(c = Text[selStart-1]) &&
					   c != TEMPPREFIX)
				{
					selStart--;
				}
				while (selStart > 0 && (isnamechar(c = Text[selStart-1]) ||
					   					c == TEMPPREFIX))
				{
					selStart--;
				}
				insertLine = GetCmdChL(selStart);
			}
			else
			{
				selStart--;
				if (selStart < ThisLineStart && insertLine > 1)
				{
					insertLine--;
				}
			}
		}
		break;

	  case RightArrow:
		if (mods & cmdKey)
		{
			selEnd = NextLineStart - 1;
		}
		else if (mods & optionKey)
		{
			while (selEnd < teLength && !isnamechar(c = Text[selEnd]) &&
				   c != TEMPPREFIX)
			{
				selEnd++;
			}
			while (selEnd < teLength && (isnamechar(c = Text[selEnd]) ||
					   					 c == TEMPPREFIX))
			{
				selEnd++;
			}
			insertLine = GetCmdChL(selEnd);
		}
		else if (selEnd < teLength)
		{
			selEnd++;
			if (selEnd >= NextLineStart && insertLine < nLines)
			{
				insertLine++;
			}
		}
		selStart = selEnd;
		break;

	  case UpArrow:
		if (mods & cmdKey)
		{
			if (insertLine > CmdLine+1)
			{
				insertLine = CmdLine+1;
			}
			else
			{
				insertLine = CmdLine + 2 - ViewLines;
				if (insertLine < 1)
				{
					insertLine = 1;
				}
			}
			selStart = LineStarts(teCmd)[insertLine-1];
		}
		else if (insertLine > 1)
		{
			selStart = LastLineStart + PlaceInLine;
			if (selStart >= ThisLineStart)
			{
				selStart = ThisLineStart-1;
			}
			insertLine--;
		}
		break;

	  case DownArrow:
		if (mods & cmdKey)
		{
			if (insertLine < CmdLine+ViewLines)
			{
				insertLine = CmdLine+ViewLines;
			}
			else
			{
				insertLine = CmdLine + 2*ViewLines - 4;
			}
			if (insertLine >= nLines)
			{
				insertLine = nLines;
				selStart = teLength;
			}
			else
			{
				selStart = LineStarts(teCmd)[insertLine-1];
			}
		}
		else if (insertLine < nLines)
		{
			selStart = NextLineStart + PlaceInLine;
			if (selStart >= LineAfterNextLineStart)
			{
				selStart = LineAfterNextLineStart-1;
			}
			insertLine++;
		}
		break;

	  case LineStart:
		/* move to Command point */
		selStart = CmdStrPos;
		TESetSelect(selStart, selStart, teCmd);
		insertLine = GetCmdIL();
		break;

	  default:
		ch = '\0';
	} /*switch ((int) ch)*/

	if (ch != '\0')
	{
		TESetSelect(selStart, selStart, teCmd);

		if (ch == Pagedown || ch == DownArrow || ch == EndKey ||
		   ch == Pageup || ch == LineStart ||
		   insertLine != CmdInsertLine)
		{
			CmdInsertLine = insertLine;
			if (CmdInsertLine-1 < CmdLine || CmdInsertLine > CmdLine + ViewLines)
			{
				ScrollToInsertPt();
			}
		}
		if (ch == HomeKey || ch == Pageup || ch == Pagedown)
		{ /* these do not change insertion point or selection */
			TESetSelect(selStartOrig, selEndOrig, teCmd);
			CmdInsertLine = insertLineOrig;
		}
		else if (mods & shiftKey)
		{
			if (ch == UpArrow || ch == LeftArrow)
			{
				TESetSelect(selStart, selEndOrig, teCmd);
			}
			else if (ch == DownArrow || ch == RightArrow)
			{
				TESetSelect(selStartOrig, selStart, teCmd);
			}
		}
	} /*if (ch != '\0')*/
	return (ch);
} /*nonTextKey()*/

/* Scroll to CmdInsertLine */

void ScrollToInsertPt(void)
{
	Rect            r;
	Integer         linel;

	/* update maximum setting for scroll bar */
	CmdScrollLines = NLines(teCmd) - ViewLines;
	SetCtlMax(CmdScroll, (CmdScrollLines > 0 ? CmdScrollLines : 0));
	HiliteControl(CmdScroll, (CmdScrollLines > 0 ? 0 : 255));

	/* check if scrolling needed */
	r = ViewRectangle(teCmd);
	r.left += LEFTOFFSET;
	r.right -= RIGHTOFFSET;

	/* adjust for characters per line */
	linel = ((SelStart(teCmd) - (LineStarts(teCmd))[CmdInsertLine - 1])
		* CharWidth((Integer) Zero) )/ (r.right - r.left) + 1;

	if (CmdInsertLine <= CmdLine)
	{
		DoScroll(CmdInsertLine - CmdLine - 1);
	}
	else if (CmdInsertLine + linel - CmdLine > ViewLines - 2)
	{
		DoScroll(CmdInsertLine + linel - CmdLine - ViewLines + 2);
	}
} /*ScrollToInsertPt()*/

/* Find line number of character n */
Integer GetCmdChL(Integer n)
{
	Integer             lineNo;
	Integer             nLines = NLines(teCmd);
	
	if (TextLength(teCmd) == 0)
	{
		return (1);
	}
	for (lineNo = 0; LineStarts(teCmd)[lineNo] <= n && lineNo < nLines;
		lineNo++)
	{
		;
	}
	return (lineNo);
} /*GetCmdChL()*/
/* find line number of cursor */

Integer GetCmdIL(void)
{
	return (GetCmdChL(SelStart(teCmd)));
} /*GetCmdIL()*/

pascal void CmdWindClose(void)
{
	Integer          windno;
	Integer          item;
	Integer          i, button = cancel + 1;
	Str255           wTitle;
	cmdWindowInfoPtr wp;
	WindowPtr        theWindow = FrontWindow();
	Boolean          optionkey;
	WHERE("CmdWindClose");
	
	optionkey = (SkelGetCurrentEvent()->modifiers & optionKey) != 0;

	windno = whichCmdWindow(theWindow);
	if (windno >= 0)
	{
		wp = CmdWindows + windno;
		item  = CmdWindows[windno].menuItem;
		if (Nwindows == 1)
		{
			if (optionkey)
			{
				SaveOnQuit = 0;
			}
			DoFile(quit);
		} /*if (Nwindows == 1)*/
		else
		{
			if (CmdDirty && !optionkey)
			{
				GetWTitle(CmdWind, wTitle);
				button = saveAlert(wTitle, true);
			}
			else
			{
				button = cancel + 1;
			}
			if (button == ok)
			{
				DoFile(saveit);
				if (!Reply.good)
				{
					button = cancel;
				}
			}
			if (button == cancel)
			{
				return;
			}
			for (i=0;i<MAXWINDOWS;i++)
			{
				if (CmdWindows[i].menuItem > item)
				{
					CmdWindows[i].menuItem--;
				}
			} /*for (i=0;i<MAXWINDOWS;i++)*/
	
			DelMenuItem(WindowMenu, item);
	
			SkelRmveWind(theWindow);
			clearWindowInfo(windno);
			if (windno == CurrentWindow)
			{
				CurrentWindow = -1;
			}
			Nwindows--;
			if (nVisible() == 0)
			{ /* make one visible */
				for (windno=0;windno<MAXWINDOWS;windno++)
				{
					if (CmdWindows[windno].cmdWind != (WindowPtr) 0)
					{
						restoreWindowInfo(windno);
						setCommandM(windno);
						MyShowWindow(CmdWindows[windno].cmdWind);
						break;
					}
				} /*for (windno=0;windno<MAXWINDOWS;windno++)*/
			} /*if (nVisible() == 0)*/
		} /*if (Nwindows == 1){}else{}*/
	} /*if (windno >= 0)*/
} /*CmdWindClose()*/

pascal void CmdWindActivate(Boolean active)
{
	Integer           windno, offset = MAXWINDOWS - Nwindows;
	GrafPtr           thePort;
	cmdWindowInfoPtr  wp;
	WHERE("CmdWindActivate");

	GetPort(&thePort);
	windno = whichCmdWindow((WindowPtr) thePort);

	wp = CmdWindows + windno;
	if (active)
	{
		if (windno != CurrentWindow)
		{
			if (CurrentWindow >= 0)
			{ /* CurrentWindow still exists */
				saveWindowInfo(CurrentWindow);
				setCommandM(CurrentWindow);
			}
			else
			{
				setCommandM(windno);
			}
			restoreWindowInfo(windno);
		}
		ShowControl(wp->cmdScroll);
		DrawGrowBox(wp->cmdWind);
		TEActivate(wp->teCmd);

		setCmdCursor((Point *) 0, wp->teCmd);

		EnableItem(FileMenu, (Integer) 0);
		setCmdFileItems(true);
		EnableItem(EditMenu, (Integer) 0);
		setCmdEditItems();

		EnableItem(WindowMenu, (Integer) 0);
		if (Nwindows > 1)
		{
			EnableItem(WindowMenu, closeit);
		}
		else
		{
			DisableItem(WindowMenu, closeit);
		}
		if (nVisible() > 1)
		{
			EnableItem(WindowMenu, hideit);
		}
		else
		{
			DisableItem(WindowMenu, hideit);
		}
		/* Running is changed from 0 to 2 in processing some menu commands */
		if ((Running == 0 || Running == 2) && Nwindows < MAXWINDOWS &&
			whichCmdWindow(FrontWindow()) >= 0)
		{
			EnableItem(WindowMenu, newwind);
			EnableItem(CommandMenu, (Integer) 0);
			EnableItem(OptionsMenu, (Integer) 0);
			EnableItem(FontMenu, (Integer) 0);
			EnableItem(FontSizeMenu, (Integer) 0);
		}
		else
		{
			DisableItem(WindowMenu, newwind);
			DisableItem(CommandMenu, (Integer) 0);
			DisableItem(OptionsMenu, (Integer) 0);
			DisableItem(FontMenu, (Integer) 0);
			DisableItem(FontSizeMenu, (Integer) 0);
		}
		EnableItem(WindowMenu, gototop - offset);
		EnableItem(WindowMenu, gotobottom - offset);
		EnableItem(WindowMenu, gotoprompt - offset);
		EnableItem(WindowMenu, pageup - offset);
		EnableItem(WindowMenu, pagedown - offset);
		for (windno = 0;windno < Nwindows;windno++)
		{
			EnableItem(WindowMenu, cmd1 + windno);
		}

		HiliteControl(CmdScroll, (CmdScrollLines > 0 ? 0 : 255));
	} /*if (active)*/
	else
	{
		TEDeactivate(wp->teCmd);
		DisableItem(FileMenu, (Integer) 0);
		setCmdFileItems(false);

		EnableItem(EditMenu, undoit);
		EnableItem(EditMenu, cutit);
		EnableItem(EditMenu, copyit);
		EnableItem(EditMenu, pastit);
		DisableItem(EditMenu, copytoend);
		DisableItem(EditMenu, copyandexecute);
#ifdef SAVEHISTORY
		DisableItem(EditMenu, backwardhistory);
		DisableItem(EditMenu, forwardhistory);
#endif /*SAVEHISTORY*/
		DisableItem(WindowMenu, (Integer) 0);

		DisableItem(CommandMenu, (Integer) 0);
		DisableItem(OptionsMenu, (Integer) 0);
		DisableItem(FontMenu, (Integer) 0);
		DisableItem(FontSizeMenu, (Integer) 0);

		HideControl(wp->cmdScroll);
		DrawGrowBox(wp->cmdWind);
	} /*if (active){}else{}*/
	DrawMenuBar();
} /*CmdWindActivate()*/ 

/*
	Handle Edit menu items for text window.
	No longer gets items for graphs windows

	Undo should work in the following cases, all of which require that the
	contents of the window have not been modified since the action enabling
	undo.
	  Immediately after a command:
		Remove output and prompt and reset CmdStrPos to what it was before
		command
	  Immediately after such an undo:
		Restore output and prompt
	  Immediately after a deletion of a selection by backspace
	    Restore what was deleted and select it
	  Immediately after a deletion of a selection by replacing it with a
	  character
		Restore what was deleted and remove the character
	  Immediately after a paste
		Remove what was pasted and restore what was displaced, if anything.
	In each case CmdStrPos is restored to what it should be
	
*/

void CmdWindEditMenu(Integer item)
{
	Integer            location, cmdStrPos, length, change;
	Integer            selStart = SelStart(teCmd), selEnd = SelEnd(teCmd);
	Integer            teLength = TextLength(teCmd);
	Integer            displacedLength;
	LongInt            scrapLength;
	
	if (!CmdEditable &&
	   (item == undoit || item == cutit || item == pastit ||
	    item == copytoend || item == copyandexecute))
	{
		return;
	}

	switch ((int) item)
	{
	  case undoit:
		if (UndoStatus == CANTUNDO || Running)
		{
			return;
		}
		switch (UndoStatus)
		{
		  case PASTEUNDO:
			/* undo result of previous paste*/
			TESetSelect(UndoPlace,UndoPlace+PasteLength, teCmd);
			TECut(teCmd);
			/* Also put back what was pasted to clipboard*/
			(void) ZeroScrap();
			(void) TEToScrap();
			
			/* restore what it replaced */
			scrapLength = fromMyScrap();
			break;

		  case DELETEUNDO:
		  case DELETEREDO:
			/* Must have been deletion or replacement that enabled undo */
			location = UndoPlace;
			length = LastTypingPos - UndoPlace;
			cmdStrPos = CmdStrPos;
			scrapLength = fromMyScrap(); /* restore what was deleted*/
			selStart = SelStart(teCmd);
			selEnd = SelEnd(teCmd);
			if (selStart < cmdStrPos)
			{
				CmdStrPos = cmdStrPos + scrapLength - length;
			}
			if (LastChar != Backspace)
			{ /* was replacement of selection by typing, so remove it */
				TESetSelect(selEnd, selEnd+length, teCmd);
				toMyScrap();
				UndoPlace = location;
				LastTypingPos = location + scrapLength;
				TEDelete(teCmd);
				TESetSelect(selStart, selEnd, teCmd);
				UndoStatus = (item == DELETEREDO) ? DELETEUNDO : DELETEREDO;
			} /*if (LastChar != Backspace)*/
			break;

		  case CUTUNDO:
			/* undo previous cut */
			location = UndoPlace;
			change = TEGetScrapLen();
			TESetSelect(location, location, teCmd);
			TEPaste(teCmd); /* should still be on TE scrap */
			if (location < CmdStrPos)
			{
				CmdStrPos += change;
			}
			clearUndoInfo(); /* can't undo*/
			TESetSelect(location, location + change, teCmd);
			break;

		  case OUTPUTUNDO:
			/*
				Undo output from most recent command.  LastLength is length,
				including final newline, just before DoStats() and LastCmdStrPos
				is the value of CmdStrPos at that time.  We cut all the output,
				plus the final newline.
			*/
			TESetSelect(LastLength-1, teLength, teCmd);
			toMyScrap(); /* save stuff on local scrap */
			TEDelete(teCmd);
			CmdStrPos = LastCmdStrPos;
			LastLength = LastCmdStrPos = -1;
			LastChar = Backspace;
			UndoStatus = OUTPUTREDO; /* can redo */
			CmdInsertLine = GetCmdIL();
			break;

		  case OUTPUTREDO:
			/* redo output just undone */
			length = teLength;
			location = CmdStrPos;
			scrapLength = fromMyScrap(); /*get previous output from local scrap*/
			TESetSelect(TextLength(teCmd), TextLength(teCmd), teCmd);
			CmdStrPos = TextLength(teCmd);
			clearUndoInfo();
			LastCmdStrPos = location;
			LastLength = length + 1;
			UndoStatus = OUTPUTUNDO;
			CmdInsertLine = GetCmdIL();
			break;

		  case TYPINGUNDO:
			if (LastTypingPos > UndoPlace)
			{
				location = UndoPlace;
				length = LastTypingPos - location;
				UndoStatus = TYPINGREDO;
				OldCmdStrPos = CmdStrPos;
				TESetSelect(UndoPlace, LastTypingPos, teCmd);
				toMyScrap();
				if (location < CmdStrPos)
				{
					CmdStrPos -= length;
				}
				TEDelete(teCmd);
				TESetSelect(location, location, teCmd);
			}
			break;

		  case TYPINGREDO:
			selStart = UndoPlace;
			TESetSelect(selStart,selStart,teCmd);
			length = fromMyScrap();
			UndoPlace = selStart;
			LastTypingPos = UndoPlace + length;
			UndoStatus = TYPINGUNDO;
		} /*switch (UndoStatus)*/
		break;

		/*
		  cut selection, put in TE Scrap, clear clipboard and put
		  TE scrap in it
		  */
	  case cutit:
		/* first reset CmdStrPos */
		if (selStart != selEnd)
		{
			clearUndoInfo();
			OldCmdStrPos = CmdStrPos;
			UndoPlace = selStart;
			if (UndoPlace < CmdStrPos)
			{	/*only need change if before command line */
				if (selEnd <= CmdStrPos)
				{
					CmdStrPos -= selEnd - selStart;
				}
				else
				{ /* selStart < CmdStrPos && selEnd > CmdStrPos */
					CmdStrPos = selStart;
				}
			} /*if (UndoPlace < CmdStrPos)*/

			TECut(teCmd);
			(void) ZeroScrap();
			(void) TEToScrap();
			CmdInsertLine = GetCmdIL();
			UndoStatus = CUTUNDO; /*cut can be undone */
			LastChar = Backspace;
			CmdWindows[CurrentWindow].cmdDirty = CmdDirty = true;
		} /*if (selStart != selEnd)*/
		break;

		/*
		  copy selection to TE Scrap, clear clipboard and put
		  TE scrap in it
		  */
	  case copyit:
		if (selStart != selEnd)
		{
			TECopy(teCmd);
			(void) ZeroScrap();
			(void) TEToScrap();
			CmdInsertLine = GetCmdIL();
		}
		if (UndoStatus == CUTUNDO)
		{
			clearUndoInfo();
		}
		break;

		/*
		  get clipboard into TE scrap, put TE scrap into edit record
		  */
	  case pastit:
		(void) TEFromScrap();
		length = TEGetScrapLen();
		
		if (length > 0)
		{ /* there is something to paste */
			location = selStart;
			displacedLength = selEnd - selStart;
			change = length - displacedLength;
			if (teLength + change > CMDWINDOWLIMIT)
			{
				myAlert("You are trying to paste more than the window can hold");
			} /*if (teLength + change > CMDWINDOWLIMIT)*/
			else
			{
				clearUndoInfo();
				OldCmdStrPos = CmdStrPos;
				toMyScrap();/* save copy of displaced text before replacing it */
				PasteLength = length;
				UndoStatus = PASTEUNDO; /* paste can be undone */
				if (displacedLength > 0)
				{
					TEDelete(teCmd);
					TESetSelect(selStart,selStart,teCmd);
				}
				TEPaste(teCmd);

			/* only reset CmdStrPos if selection before or including CmdStrPos */
				if (selStart < CmdStrPos)
				{
					if (selEnd <= CmdStrPos)
					{ /* entire selection <= CmdStrPos */
						CmdStrPos += change;
					}
					else
					{ /* selStart < CmdStrPos && selEnd > CmdStrPos */
						CmdStrPos = selStart;
					}
				} /*if (selStart < CmdStrPos)*/
			} /*if (teLength + change > CMDWINDOWLIMIT){}else{}*/
			CmdWindows[CurrentWindow].cmdDirty = CmdDirty = true;
		} /*if (length > 0)*/
		break;

		/* copy selection to end of window */
	  case copytoend:
		/* first copy */
		/* Treated as typing for Undo purposes*/
		if (selStart != selEnd)
		{
			if (UndoStatus != TYPINGUNDO || LastTypingPos != TextLength(teCmd))
			{
				clearUndoInfo(); /* nothing can be redone*/
				UndoStatus = TYPINGUNDO;
				UndoPlace = TextLength(teCmd);
			}
			TECopy(teCmd);
#if (0) /*don't put on Clip Board*/
			(void) ZeroScrap();
			(void) TEToScrap();
			(void) TEFromScrap();
#endif /*0*/
			selStart = TextLength(teCmd);
			TESetSelect(selStart,selStart, teCmd);
			TEPaste(teCmd);
			LastTypingPos = TextLength(teCmd);
			CmdWindows[CurrentWindow].cmdDirty = CmdDirty = true;
		}
		break;

		/* copy selection to end of window */
	  case copyandexecute:
	  	/* simulate key press with Enter */
		CmdWindKey(Enter, 0, 0);
		break;

#ifdef SAVEHISTORY
	  case backwardhistory:
	  	/* simulate key press with option up arrow */
		scanKeyboard();
		CmdWindKey(UpArrow, 0,
				   optionKey | ((isKeyPressed(COMMANDKEYCODE)) ? cmdKey : 0));
		break;

	  case forwardhistory:
	  	/* simulate key press with option down arrow */
		scanKeyboard();
		CmdWindKey(DownArrow, 0,
				   optionKey | ((isKeyPressed(COMMANDKEYCODE)) ? cmdKey : 0));
		break;
#endif /*SAVEHISTORY*/

	} /*switch ((int) item)*/

	if (item != copyit)
	{
		CmdInsertLine = GetCmdIL();
		ScrollToInsertPt();
	}
	setCmdEditItems();
} /*CmdWindEditMenu()*/

/*
    Scroll to the correct position.  lDelta is the
    amount to CHANGE the current scroll setting by.
*/


/*
	970223set menu item or items related to help.
	Currently only one item and its on Apple menu.  Needs to be moved to Help menu
	under System 7
	Help menu item is set to "Help" if nothing selected and to help("topic")
	if topic is selected, trimming off leading and trailing blanks and non-name
	characters
	980825 modified code to use HelpMenu (same as AppleMenu for Systems < 7)
*/

#define TOPICNAMELENGTH     20  /* also defined in help.c*/
#define HEADLENGTH           6  /*strlen("help(\"")*/
#define TAILLENGTH           2  /*strlen("\")")*/

void setCmdHelpItems(void)
{
	Integer         istart = SelStart(teCmd);
	Integer         iend = SelEnd(teCmd);
	Integer         i, j;
	Integer         jstart, jend;
	char            c, *head = "help(\"", *tail = "\")";
	unsigned char   menuEntry[TOPICNAMELENGTH + HEADLENGTH + TAILLENGTH + 1];

	if (iend > istart)
	{		
		strcpy((char *) menuEntry, head);
		jstart = HEADLENGTH;
		jend = jstart + TOPICNAMELENGTH;
		
		for (i = istart, j = jstart; i < iend && j < jend; i++)
		{ /* trim off leading blanks && non alphas while copying */
			c = Text[i];
			if (j > jstart || isnamestart(c))
			{ /* stop when menuEntry is full or hit not alphanumeric*/
				if (!isnamechar(c))
				{
					break;
				}
				menuEntry[j++] = c;
			} /*if (j > jstart || isnamestart(c))*/
		} /*for (i = istart, j = jstart; i < iend && j < jend; i++)*/
		menuEntry[j] = '\0';

		if (j > jstart)
		{
			strcpy((char *) menuEntry + j, tail);
		}
		else
		{
			menuEntry[0] = '\0';
		}
	} /*if (selStart != selEnd)*/
	else
	{
		menuEntry[0] = '\0';
	}
	
	if (!menuEntry[0])
	{
		strcpy((char *) menuEntry, "Help");
	} /*if (selStart != selEnd){}else{}*/
	CtoPstr((char *) menuEntry);
	SetItem(HelpMenu, HelpItemNumber, menuEntry);
} /*setCmdHelpItems()*/

/*
	980825 modified code to use HelpMenu (same as AppleMenu for Systems < 7)
*/

void setCmdFileItems(Boolean infront)
{
	WHERE("setCmdFileItems");

	if (infront)
	{
		if (!Running && BDEPTH <= 0)
		{
			EnableItem(HelpMenu, HelpItemNumber);
			EnableItem(FileMenu, openit);
			EnableItem(FileMenu, saveit);
			EnableItem(FileMenu, saveitas);
			SetItem(FileMenu,saveitas, "\pSave Window As…");
			EnableItem(FileMenu, restoreit);
			EnableItem(FileMenu, keepit);
			EnableItem(FileMenu, keepitas);
			EnableItem(FileMenu, batchit);
			EnableItem(FileMenu, spoolit);
			if (TextLength(teCmd) > 0)
			{
				EnableItem(FileMenu, printit);
			}
			else
			{
				DisableItem(FileMenu, printit);
			}
			SetItem(FileMenu, printit,
					(SelEnd(teCmd) == SelStart(teCmd)) ?
					"\pPrint Window…" : "\pPrint Selection…");
			EnableItem(FileMenu, quit);
		} /*if (!Running && BDEPTH <= 0)*/
		DisableItem(FileMenu, go_on);
		EnableItem(FileMenu, interrupt);
	} /*if (infront)*/
	else
	{
		DisableItem(FileMenu, openit);
		DisableItem(FileMenu, saveit);
		DisableItem(FileMenu, saveitas);
		DisableItem(FileMenu, printit);
		DisableItem(FileMenu, restoreit);
		if (Running || BDEPTH > 0)
		{
			DisableItem(HelpMenu, HelpItemNumber);
			DisableItem(FileMenu, keepit);
			DisableItem(FileMenu, keepitas);
			DisableItem(FileMenu, spoolit);
			DisableItem(FileMenu, batchit);
			DisableItem(FileMenu, quit);
		}
	} /*if (infront){}else{}*/
	if (SPOOLFILENAME != (char **) 0)
	{
		unsigned char     *menuEntry = (SPOOLFILE != (FILE *) 0) ?
			"\pStop Spooling" : "\pResume Spooling";

		SetItem(FileMenu,spoolit,menuEntry);
	} /*if (SPOOLFILENAME != (char **) 0)*/
} /*setCmdFileItems)*/

void setCmdEditItems(void)
{
	Integer            selStart = SelStart(teCmd);
	Integer            selEnd = SelEnd(teCmd);
	unsigned char      *menuEntry;

	setCmdHelpItems();

	if (UndoStatus == CANTUNDO || !CmdEditable)
	{
		menuEntry = "\pCan't Undo";
	}
	else if (UndoStatus == OUTPUTUNDO)
	{
		menuEntry = "\pUndo Output";
	}
	else if (UndoStatus == CUTUNDO)
	{
		menuEntry = "\pUndo Cut";
	}
	else if (UndoStatus == PASTEUNDO)
	{
		menuEntry = "\pUndo Paste";
	}
	else if (UndoStatus == TYPINGUNDO || UndoStatus == DELETEUNDO)
	{
		menuEntry = "\pUndo Typing";
	}
	else if (UndoStatus == OUTPUTREDO)
	{
		menuEntry = "\pRedo Output";
	}
	else if (UndoStatus == TYPINGREDO || UndoStatus == DELETEREDO)
	{
		menuEntry = "\pRedo Typing";
	}
	SetItem(EditMenu,undoit,menuEntry);
	if (UndoStatus == CANTUNDO)
	{
		DisableItem(EditMenu, undoit);
	}
	else
	{
		EnableItem(EditMenu, undoit);
	}

	if (selStart != selEnd)
	{
		if (CmdEditable)
		{
			EnableItem(EditMenu,cutit);
			if (!Running && BDEPTH <= 0)
			{
				EnableItem(EditMenu,copytoend);
			}
		}
		EnableItem(EditMenu,copyit);
	}
	else
	{
		DisableItem(EditMenu,cutit);
		DisableItem(EditMenu,copyit);
		DisableItem(EditMenu,copytoend);
	}
	if (CmdEditable && !Running && BDEPTH <= 0)
	{
		if (selStart == selEnd || selStart >= CmdStrPos)
		{
			menuEntry = "\pExecute";
		}
		else
		{
			menuEntry = "\pCopy and Execute";
		}
		SetItem(EditMenu,copyandexecute,menuEntry);
		EnableItem(EditMenu, copyandexecute);
#ifdef SAVEHISTORY
		EnableItem(EditMenu, backwardhistory);
		EnableItem(EditMenu, forwardhistory);
#endif /*SAVEHISTORY*/
	}
	else
	{
		DisableItem(EditMenu, copyandexecute);
#ifdef SAVEHISTORY
		DisableItem(EditMenu, backwardhistory);
		DisableItem(EditMenu, forwardhistory);
#endif /*SAVEHISTORY*/
	}
	
	if (CmdEditable && scrapSize() > 0)
	{
		EnableItem(EditMenu,pastit);
	}
	else
	{
		DisableItem(EditMenu,pastit);
	}
} /*setCmdEditItems() */

void DoScroll(Integer lDelta)
{
	Integer         newLine;

	newLine = CmdLine + lDelta;
	if (newLine < 0)
	{
		newLine = 0;
	}
	if (newLine > GetCtlMax(CmdScroll))
	{
		newLine = GetCtlMax(CmdScroll) + 1;
	}

	SetCtlValue(CmdScroll, newLine);
	lDelta = (CmdLine - newLine) * LineHeight(teCmd);
	TEScroll(0, lDelta, teCmd);
	HiliteControl(CmdScroll, (CmdScrollLines > 0 ? 0 : 255));
	CmdLine = newLine;
} /*DoScroll()*/

/*
	Filter proc for tracking mousedown in scroll bar.  The part code
	of the part originally hit is stored as the control's reference
	value.

	The "void" had better be there!  Otherwise Lightspeed will treat
	it as an integer function, not a procedure.
*/

pascal void     TrackScroll(ControlHandle theScroll, Integer partCode)
{
	Integer         lDelta;

	if (partCode == GetCRefCon(theScroll)) /* still in same part? */
	{
		switch ((int) partCode)
		{
		  case inUpButton:
			lDelta = -1;
			break;
		  case inDownButton:
			lDelta = 1;
			break;
		  case inPageUp:
			lDelta = -HalfPage;
			break;
		  case inPageDown:
			lDelta = HalfPage;
			break;
		}
		DoScroll(lDelta);
	}
} /*TrackScroll()*/

pascal Boolean doClikLoop(void)
{
	Rect       r;
	Point      mouseLoc;
	
	GetMouse(&mouseLoc);
	r = ViewRectangle(teCmd);

	if (mouseLoc.v < r.top)
	{
		DoScroll(-1);
	}
	else if (mouseLoc.v > r.bottom)
	{
		DoScroll(1);
	}
	return (true);
} /*doClikLoop*/

#if (0)
pascal Boolean wordBreak(text, charPos)
Ptr         text;
Integer     charPos;
{
	unsigned char  *ch = (unsigned char *) text + charPos, c;
	
	c = *ch++;
	return ((isspace(c) || 
		!(isalnum(c) || c == '.' || c == '$' || (c == '<' && *ch == '-'))) ?
		true : false);
} /*wordBreak()*/
#endif /*0*/

/*
    Handle hits in scroll bar
*/

pascal void CmdWindMouse(Point thePt, LongInt when, Integer mods)
{
	Integer         thePart;
	LongInt         selStart, selEnd;
	Point           thePt2 = thePt;
	static LongInt  lastWhen = 0;
	static LongInt  doubleTime = -1;
	WHERE("CmdWindMouse");

	if (doubleTime < 0)
	{
		doubleTime = GetDblTime();
	} /*if (doubleTime < 0)*/
	
	thePart = TestControl(CmdScroll, thePt);
	setCmdCursor(&thePt, teCmd);

	if (thePart == inThumb)
	{
		(void) TrackControl(CmdScroll, thePt, NullControlActionPtr);
		DoScroll(GetCtlValue(CmdScroll) - CmdLine);
	}
	else if (thePart != 0)
	{
		SetCRefCon(CmdScroll, (LongInt) thePart);
		(void) TrackControl(CmdScroll, thePt, MyControlActionPtr);
	}
	else
	{
		TEClick(thePt2, (mods & shiftKey) != 0, teCmd);
#if (0)
		DEBUGGER1(64,"selStart = %d, selEnd = %d, nLines = %d,teLength = %d\n",
				  SelStart(teCmd),SelEnd(teCmd),NLines(teCmd),
				  TextLength(teCmd));
#endif /*0*/

		selStart = SelStart(teCmd);
		selEnd = SelEnd(teCmd);
		if (selStart == selEnd)
		{ /* kludge to avoid arrow key problem */
			TESetSelect(TextLength(teCmd), TextLength(teCmd), teCmd);
			TEKey(Return, teCmd);
			TEKey(Backspace, teCmd);
			TESetSelect(selStart, selStart, teCmd);
		}
		else if (selEnd == selStart + 1 && when - lastWhen < doubleTime)
		{
			char            *cp, c = Text[selStart], c1, target;
			Integer          incr = 0, here, start, last, count;
			
			if ((cp = strchr(LParens, c)))
			{
				incr = 1;
				last = TextLength(teCmd);
				target = RParens[cp - LParens];
			}
			else if (selStart > 0 && (cp = strchr(RParens, c)))
			{
				incr = -1;
				last = -1;
				target = LParens[cp - RParens];
			}
			if (incr != 0)
			{
				start = selStart + incr;
				for (here = start, count = 1; here != last; here += incr)
				{
					c1 = Text[here];
					if (c1 == target)
					{
						if (--count == 0)
						{
							break;
						}
					}
					else if (c1 == c)
					{
						count++;
					}
				} /*for (here = start, count = 1; here != last; here += incr)*/
				if (count == 0)
				{
					selStart = (incr > 0) ? start : here + 1;
					selEnd = (incr > 0) ? here : start + 1;
					TESetSelect(selStart,selEnd, teCmd);
				} /*if (count == 0)*/
			} /*if (incr != 0)*/
		}
		if (selStart >= LineStarts(teCmd)[NLines(teCmd)-1])
		{
			CmdInsertLine = NLines(teCmd);
		}
		else
		{
			CmdInsertLine = GetCmdIL();
		}
		if (GetCtlValue(CmdScroll) != CmdLine)
		{
			SetCtlValue(CmdScroll, CmdLine);
		}
		setCmdEditItems();
	}
	lastWhen = when;
} /*CmdWindMouse()*/

/*
	Update Cmd window.  The update event might be in response to a
	window resizing.  If so, resize the rects and recalc the linestarts
	of the text.  To resize the rects, only the right edge of the
	destRect need be changed (the bottom is not used, and the left and
	top should not be changed). The viewRect should be sized to the
	screen.

	This should be preceded by BeginUpdate(CmdWind) and followed by
	EndUpdate(CmdWind).
*/

pascal void CmdWindUpdate(Boolean resized)
{
	Rect               r;
	Integer            windno;
	cmdWindowInfoPtr   wp;
	GrafPtr            thePort;
	WHERE("CmdWindUpdate");

	GetPort(&thePort);
	windno = whichCmdWindow((WindowPtr) thePort);
	wp = CmdWindows + windno;
	r = (wp->cmdWind)->portRect;
	EraseRect(&r);
	if (resized)
	{
		r.left += LEFTOFFSET;
		r.bottom -= BOTTOMOFFSET;
		r.top += TOPOFFSET;
		r.right -= RIGHTOFFSET;

		DestRectangle(wp->teCmd).right = r.right;
		ViewRectangle(wp->teCmd) = r;
		TECalText(wp->teCmd);
		/*
			move and resize the scroll bar as well.  The ValidRect call is done
			because the HideControl adds the control bounds box to the update
			region - which would generate another update event!  Since everything
			gets redrawn below, the ValidRect is used to the update.
		*/
		if (windno == CurrentWindow)
		{
			setCmdScreen();

			HideControl(CmdScroll);
			r = (**CmdScroll).contrlRect;
			ValidRect(&r);
			r = CmdWind->portRect;
			r.left = r.right - 15;
			++r.right;
			r.bottom -= 14;
			--r.top;
			SizeControl(CmdScroll, r.right - r.left, r.bottom - r.top);
			MoveControl(CmdScroll, r.left, r.top);
			ShowControl(CmdScroll);
			ScrollToInsertPt();
		} /*if (isCmdWindow)*/
	} /*if (resized)*/
	DrawGrowBox(wp->cmdWind);
	DrawControls(wp->cmdWind);	/* redraw scroll bar */
	r = ViewRectangle(wp->teCmd);
	EraseRect(&r);
	TEUpdate(&r, wp->teCmd);	/* redraw text display */

} /*CmdWindUpdate()*/

void CmdWindPrint(Integer windno)
{
	GrafPtr         thePort;
	Rect            r;
	cmdWindowInfoPtr wp = CmdWindows + windno;
	WHERE("CmdWindPrint");

	if (TextLength(wp->teCmd) > 0)
	{
		GetPort(&thePort);
		TextWindPrint(windno);
		SetPort(thePort);
		r = wp->cmdWind->portRect;
		InvalRect(&r);
		BeginUpdate(wp->cmdWind);
		CmdWindUpdate(0);
		EndUpdate(wp->cmdWind);
		SetPort(thePort);
	}
} /*CmdWindPrint()*/

