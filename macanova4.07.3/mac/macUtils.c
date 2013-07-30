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
	Various routines for dealing with windows formerly in macInput.c and
	macMain.c

    990215 replaced myerrorout() by putOutErrorMsg()
*/

#include "globals.h"
#include "macProto.h"
#include "macIface.h" /* Note: macIface.h replaces MultiSkel.h */

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

#ifndef CLIPLINES
#define CLIPLINES  20 /* number of lines to copy to clipboard on Fatal Error */
#endif /*CLIPLINES*/

/*
	Create CmdScrap and copy selection to it, saving the start place
	of the selection in UndoPlace.
	If nothing is selected, nothing is done.  If a handle cannot be
	allocated, undo information is cleared.
*/

void toMyScrap(void)
{
	LongInt        selStart = SelStart(teCmd), selEnd = SelEnd(teCmd);
	LongInt        length = selEnd - selStart;

	if (length > 0)
	{ /* there is something to save */
		CmdScrap = NewHandle((Size) length);
		if(CmdScrap != (Handle) 0)
		{
			HLock(CmdScrap);
			HLock(TextHandle(teCmd));
			BlockMove((Ptr) *TextHandle(teCmd) + selStart,
					  (Ptr) *CmdScrap, (Size) length);
			HUnlock(CmdScrap);
			HUnlock(TextHandle(teCmd));
		}
		else
		{
			clearUndoInfo(); /* can't undo*/
		}
	} /*if(length > 0)*/
	UndoPlace = selStart;
} /*toMyScrap()*/

/* restore stuff in CmdScrap to command window */
LongInt fromMyScrap(void)
{
	LongInt      scrapLength;
	
	if(CmdScrap != (Handle) 0)
	{
		scrapLength = GetHandleSize(CmdScrap);
		HLock(CmdScrap);
		TESetSelect(UndoPlace,UndoPlace, teCmd);
		TEInsert((Ptr) *CmdScrap, scrapLength, teCmd);
		HUnlock(CmdScrap);
		TESetSelect(UndoPlace,UndoPlace+scrapLength, teCmd);
	}
	else
	{
		scrapLength = 0;
	}
	if(OldCmdStrPos >= 0)
	{
		CmdStrPos = OldCmdStrPos;
	}
	clearUndoInfo(); /* can't redo */

	return (scrapLength);
} /*fromMyScrap()*/

/*
	functions related to multiple command windows
*/

/*
	Clear any saved info for undo associated with CurrentWindow
*/
void clearUndoInfo(void)
{
	cmdWindowInfoPtr     wp = CmdWindows+CurrentWindow;

	if(CmdScrap != (Handle) 0)
	{
		DisposHandle(CmdScrap);
		wp->cmdScrap = CmdScrap = (Handle) 0;
	}
	if(UndoStatus != CANTUNDO)
	{
		UndoStatus = CANTUNDO;
		if(FrontWindow() == wp->cmdWind)
		{
			setCmdEditItems();
		}
	}
	wp->pasteLength = PasteLength = -1;
	wp->undoPlace = UndoPlace = -1;
	wp->lastTypingPos = LastTypingPos = -1;
	wp->lastCmdStrPos = LastCmdStrPos = -1;
	wp->lastLength = LastLength = -1;
	wp->oldCmdStrPos = OldCmdStrPos = -1;
} /*clearUndoInfo()*/

Integer nVisible(void)
{
	WindowPeek wPeek;
	Integer    nvisible = 0, i;

	for(i=0;i<MAXWINDOWS;i++)
	{
		wPeek = (WindowPeek) CmdWindows[i].cmdWind;
		if(wPeek != (WindowPeek) 0 && wPeek->visible)
		{
			nvisible++;
		}
	}
	return (nvisible);
} /*nVisible()*/

/*
	Returns position in MacAnova list of output windows of current window
	or -1 if current window is not an output window
*/
Integer whichCmdWindow(WindowPtr theWindow)
{
	Integer          windno;
	cmdWindowInfoPtr wp;
	
	if(!UseWindows)
	{
		return (-1);
	}
	for(windno = 0, wp = CmdWindows ;windno < MAXWINDOWS; windno++, wp++)
	{
		if(wp->cmdWind == theWindow)
		{
			break;
		}
	}
	return ((windno < MAXWINDOWS) ? windno : -1);
} /*whichCmdWindow()*/

/*
	Determine which graph window corresponds to theWindow.
	Return NGRAPH if it's the panel window, 0 through NGRAPH-1 if it
	is a regular graph window and -1 otherwise
*/

Integer whichGraphWindow(WindowPtr theWindow)
{
	Integer      windno, panelno;

	if (theWindow == PreviewWind)
	{
		return (ThisWindow);
	}
	if(!UseWindows || theWindow == (WindowPtr) 0)
	{
		return (FINDEMPTYWINDOW);
	}
	
	for (panelno = 0; panelno < NPANELS; panelno++)
	{
		if (theWindow == PanelWind[panelno])
		{
			return (NGRAPHS + panelno);
		}
	}

	for (windno = 0;windno < NGRAPHS;windno++)
	{
		if(theWindow == GraphWind[windno])
		{
			return (windno);
		}
	}
	return (-1);
} /*whichGraphWindow()*/


static KeyMap        TheKeys;

void scanKeyboard(void)
{
	GetKeys (TheKeys);
} /*scanKeyboard()*/

/*
	970115 moved isKeyPressed here from macMain() (where it was static)

	Note that byte order of integers is from most significant to least, that
	is, in the order of the usual hex representation.  This is relevent
	since a KeyMap variable (long [4]) represents a packed array of
	Boolean in Pascal.
*/

Boolean isKeyPressed (Integer keyCode)
{
	Integer       word = keyCode >> 5, bit = keyCode & 0x1f;
	Integer       byte = bit >> 3;
	unsigned long ul;
	unsigned char uc;

	bit &= 0x07;
	ul = TheKeys[word];
	uc = (0xff & (ul >> (8*(3 - byte))));
	return (((uc >> bit) & 1) != 0);
} /*isKeyPressed ()*/

/*
	If ch corresponds to Option n, where 1 <= n <= 9, keyToNumber returns
	n - 1; otherwise, it returns -1
*/

Integer optionKeyToNumber(unsigned char ch)
{
	Integer           number = -1;
	WHERE("optionKeyToWindno");

	switch( (int) ch)
	{
	  case Option1:
		number = 1;
		break;

	  case Option2:
		number = 2;
		break;

	  case Option3:
		number = 3;
		break;

	  case Option4:
		number = 4;
		break;

	  case Option5:
		number = 5;
		break;

	  case Option6:
		number = 6;
		break;

	  case Option7:
		number = 7;
		break;

	  case Option8:
		number = 8;
		break;

	  case Option9:
		number = 9;
		break;

	  case Option0:
		number = 0;
		break;

	} /*switch( (int) ch)*/
	return (number);
} /*optionKeyToNumber()*/

Integer functionKeyToNumber(Integer code)
{
	switch (code)
	{
	  case    F1:
		return (1);
	  case    F2:
		return (2);
	  case    F3:
		return (3);
	  case    F4:
		return (4);
	  case    F5:
		return (5);
	  case    F6:
		return (6);
	  case    F7:
		return (7);
	  case    F8:
		return (8);
	  case    F9:
		return (9);
	  case    F10:
		return (10);
	  case    F11:
		return (11);
	  case    F12:
		return (12);
	  case    F13:
		return (13);
	  case    F14:
		return (14);
	  case    F15:
		return (15);
	  default:
		return (-1);
	} /*switch (code)*/
} /*functionKeyToNumber()*/


void clearWindowInfo(Integer windNo)
{
	cmdWindowInfoPtr     wp = CmdWindows + windNo;

	wp->cmdWind = (WindowPtr) 0;
	wp->teCmd = (TEHandle) 0;
	wp->menuItem = -1;
	wp->cmdVRefNum = 0;
	wp->cmdDirty = false;
	wp->cmdEditable = false;
	wp->cmdScroll = (ControlHandle) 0;
	wp->cmdLine = -1;
	wp->cmdStrPos = -1;
	wp->cmdEditable = true;
	wp->undoStatus = CANTUNDO;
	wp->undoPlace = -1;
	wp->pasteLength = 0;
	wp->oldCmdStrPos = -1;
	wp->lastTypingPos = -1;
	wp->lastLength = -1;
	wp->lastCmdStrPos = -1;
	if(wp->cmdScrap != (Handle) 0)
	{
		DisposHandle(wp->cmdScrap);
	}
	wp->cmdScrap = (Handle) 0;
} /*clearWindowInfo()*/

void saveWindowInfo(Integer windNo)
{
	cmdWindowInfoPtr    wp = CmdWindows + windNo;

	if(windNo >= 0 && windNo < MAXWINDOWS)
	{
		wp->cmdWind = CmdWind;
		wp->teCmd = teCmd;
		wp->cmdDirty = CmdDirty;
		wp->cmdEditable = CmdEditable;
		wp->cmdScroll = CmdScroll;
		wp->cmdLine = CmdLine;
		wp->cmdStrPos = CmdStrPos;
		wp->undoStatus = UndoStatus;
		wp->undoPlace = UndoPlace;
		wp->pasteLength = PasteLength;
		wp->oldCmdStrPos = OldCmdStrPos;
		wp->lastTypingPos = LastTypingPos;
		wp->lastLength = LastLength;
		wp->lastCmdStrPos = LastCmdStrPos;
		wp->cmdScrap = CmdScrap;
	}
} /* saveWindowInfo() */

void restoreWindowInfo(Integer windNo)
{
	cmdWindowInfoPtr    wp = CmdWindows + windNo;
	
	CmdWind = wp->cmdWind;
	teCmd = wp->teCmd;
	CmdDirty = wp->cmdDirty;
	CmdEditable = wp->cmdEditable;
	CmdScroll = wp->cmdScroll;
	CmdLine = wp->cmdLine;
	CmdStrPos = wp->cmdStrPos;
	UndoStatus = wp->undoStatus;
	UndoPlace = wp->undoPlace;
	PasteLength = wp->pasteLength;
	OldCmdStrPos = wp->oldCmdStrPos;
	LastTypingPos = wp->lastTypingPos;
	LastLength = wp->lastLength;
	LastCmdStrPos = wp->lastCmdStrPos;
	CmdScrap = wp->cmdScrap;
	CheckItem(WindowMenu, wp->menuItem, true);

	if(CurrentWindow >= 0)
	{
		wp = CmdWindows + CurrentWindow;
		CheckItem(WindowMenu, wp->menuItem, false);
	}
	CurrentWindow = windNo;
	setCmdScreen();
} /* restoreWindowInfo() */

void setCommandM(Integer windno)
{
	Integer       item, i;;

	/* First unset all text window bindings */
	for(i=0;i<MAXWINDOWS;i++)
	{
		if((item = CmdWindows[i].menuItem) > 0)
		{
			SetItemCmd(WindowMenu, item, 0);
		}
	}
	if(windno >= 0 && windno < MAXWINDOWS &&
	   (item = CmdWindows[windno].menuItem) > 0)
	{
		SetItemCmd(WindowMenu, item, 'M');
	}
} /*setCommandM()*/

/*
	set leading to zero for specific font/font size combinations
*/
void adjustLeading(Integer fontNumber, Integer fontSize, FontInfo *aFont)
{
	Str255       fontName;

	GetFontName(fontNumber, fontName);
	if (str255cmp(fontName, "\pCourier") == 0 && fontSize == 10 ||
		str255cmp(fontName, "\pMonaco") == 0 && fontSize == 12 ||
		str255cmp(fontName, MACANOVAFONTNAME) == 0 && fontSize == 12)
	{
		aFont->leading = 0; /* use 0 leading */
	}
}
	
/*
	Get bounding box of main screen, without menu bar
*/
void getScreenRect(Rect * r)
{
#if (1)
	if (!SkelQuery (skelQHasColorQD) )
	{
		/* no Color QuickDraw => the one screen is the main device */
#if defined( MPW) || defined(MW_CW)
		GrafPtr    screenPort;

		GetWMgrPort (&screenPort);
		*r = screenPort->portRect;
#else /*MPW*/
		*r = screenBits.bounds;
#endif/*MPW*/
	}
	else
	{
		*r = (**GetMainDevice()).gdRect;
	}
	r->top += SkelQuery (skelQMBarHeight);

#else /*1*/
	/* old method which didn't work well with 2 or more screens */
	RgnHandle          grayRgnH = (RgnHandle) SkelQuery(skelQGrayRgn);

	if (grayRgnH != (RgnHandle) 0)
	{
		*r = (*grayRgnH)->rgnBBox;
		DisposHandle((Handle) grayRgnH);
	}
	else
	{
		r->top = r->left = r->bottom = r->right = 0;
	}
#endif /*1*/
} /*getScreenRect()*/

/* routine to create a new text window */
static Integer  WindowNumber = 0; /* Incremented for every window created */

Integer createWindow(Str255 wTitle)
{
	Rect               r, r1, bBox;
	Integer            windno;
	Integer            downoffset, rightoffset;
	cmdWindowInfoPtr   wp;
	Str255             untitled, *title;
	WStateData       **wStateHandle;
	FontInfo           thisFont;
	WHERE("createWindow");

	if(Nwindows < 0)
	{ /* initialize things the first time through */
		for(windno=0;windno<MAXWINDOWS;windno++)
		{
			clearWindowInfo(windno);
		}
		Nwindows = 0;
	}
	for(windno=0;windno<MAXWINDOWS;windno++)
	{
		if(CmdWindows[windno].cmdWind == (WindowPtr) 0)
		{
			break;
		}
	}
	if(windno == MAXWINDOWS)
	{
		return (windno);
	}
	if(CurrentWindow >= 0)
	{
		saveWindowInfo(CurrentWindow);
	}
	clearWindowInfo(windno);
	
	wp = CmdWindows + windno;
	
	wp->cmdWind = GetNewWindow(COMMANDWINDOW,(Ptr) nil,(WindowPtr) -1L);
	if(wp->cmdWind == (WindowPtr) 0 ||
	   !SkelWindow(wp->cmdWind,
				  (SkelWindMouseProcPtr) CmdWindMouse,
				  (SkelWindKeyProcPtr) CmdWindKey,
				  (SkelWindUpdateProcPtr) CmdWindUpdate,
				  (SkelWindActivateProcPtr) CmdWindActivate,
				  (SkelWindCloseProcPtr) CmdWindClose,
				  (SkelWindClobberProcPtr) CmdWindClobber,
				  (SkelWindIdleProcPtr) nil,
				  false))
	{
		goto errorExit;
	}

	wStateHandle = (WStateData **) ((WindowPeek) wp->cmdWind)->dataHandle; 
	HLock((Handle) wStateHandle);
	/*
	  Use left and right of userState (from resource rect), but stagger top
	  and set bottom to bottom of stdState
	*/
	r = (*wStateHandle)->userState;
	r1 = (*wStateHandle)->stdState;

	downoffset = windno*STACKOFFSET;
	rightoffset = windno*STACKOFFSET/2;
	
	getScreenRect(&bBox);

#if (0)
	if (bBox.top == 0 && bBox.left == 0 && bBox.bottom == 0 && bBox.right == 0)
	{
		bBox = r;
	}
#endif /*0*/

	if (r.right + rightoffset > bBox.right)
	{
		rightoffset = bBox.right - r.right;
	}

	if (r.right + rightoffset > bBox.right)
	{
		rightoffset = bBox.right - r.right;
	}
	r.top += downoffset;
	r.left += rightoffset;
	r.bottom = r1.bottom;
	r.right += rightoffset;
	(*wStateHandle)->userState = r;
	HUnlock((Handle) wStateHandle);

	/* resize and move window */	
	SetPort((GrafPtr) wp->cmdWind);
	ZoomWindow(wp->cmdWind, inZoomIn, false);

	if(wTitle == (STR255) 0)
	{
		sprintf((char *) untitled, "Untitled-%d",++WindowNumber);
		CtoPstr((char *) untitled);
		title = (Str255 *) untitled;
	}
	else
	{
		title = (Str255 *) wTitle;
	}
	SetWTitle(wp->cmdWind, /*(Str255)*/ *title);
	TextFont(CmdFont);
	TextSize(CmdFontSize);
	r = (wp->cmdWind)->portRect;
	r.left += LEFTOFFSET;
	r.bottom -= BOTTOMOFFSET;
	r.top += TOPOFFSET;
	r.right -= RIGHTOFFSET;

	wp->teCmd = TENew(&r, &r);
	if(wp->teCmd == (TEHandle) 0)
	{
		goto errorExit;
	}

	GetFontInfo(&thisFont);
	adjustLeading(CmdFont, CmdFontSize, &thisFont);
	(*(wp->teCmd))->lineHeight = thisFont.ascent + thisFont.descent +
		thisFont.leading;
	(*(wp->teCmd))->fontAscent = thisFont.ascent;
	(*(wp->teCmd))->txFont = CmdFont;
	(*(wp->teCmd))->txSize = CmdFontSize;
	SetClikLoop (/*(ClikLoopProcPtr)*/ MyTEClickLoopPtr, wp->teCmd);
	wp->cmdStrPos = TextLength(wp->teCmd);
	wp->cmdLine = 0;  /* number of lines before line at top of screen */
	
	/* rectangle for scroll bar */
	r = (wp->cmdWind)->portRect;
	r.left = r.right - 15;
	r.bottom -= 14;
	--r.top;
	++r.right;

	/* build scroll bar */
	wp->cmdScroll = NewControl(wp->cmdWind, &r, (const unsigned char *) "", true, wp->cmdLine, 0,
					0, scrollBarProc, 0);
	if(wp->cmdScroll == (ControlHandle) 0)
	{
		goto errorExit;
	}

	Nwindows++;
	
	ShowWindow(wp->cmdWind);

	ValidRect(&(wp->cmdWind)->portRect);
	return (windno);

  errorExit:
	if(wp->cmdWind != (WindowPtr) 0)
	{
		if(wp->teCmd != (TEHandle) 0)
		{
			TEDispose(wp->teCmd);
			
		}
		SkelRmveWind(wp->cmdWind);
	}
	
	return (-1);  
} /* createWindow()*/

/*
	Function to put check next to Current font in Font menu,
	put check next to current font size in Size menu and to
	outline the fontsizes that are available
*/

void decorateFontMenus(Integer fontNumber, Integer fontSize)
{
	Str255         itemString;
	Integer        nItems = CountMItems(FontMenu), number;
	int            item, size;

	for (item=3;item<=nItems; item++)
	{
		GetItem(FontMenu, item, itemString);
		GetFNum(itemString, &number);
		CheckItem(FontMenu, item, (fontNumber == number) ? true : false);
	}

	nItems = CountMItems(FontSizeMenu);
	for (item=1;item<=nItems; item++)
	{
		GetItem(FontSizeMenu, item, itemString);
		PtoCstr(itemString);
		sscanf((char *) itemString, "%d", &size);
		CheckItem(FontSizeMenu, item, (size == fontSize) ? true : false);
		SetItemStyle(FontSizeMenu, item,
					 (RealFont(fontNumber, (Integer) size)) ? outline : normal);
	} /*for (item=1;item<=nItems; item++)*/
} /*decorateFontMenus()*/
/*
	reset the font for the command window
*/
void setCmdWindFont(Integer fontNumber, Integer fontSize)
{
	cmdWindowInfoPtr   wp;
	GrafPtr            thePort;
	FontInfo           thisFont;
	
	GetPort(&thePort);
	
	if (CurrentWindow >= 0)
	{
		decorateFontMenus(fontNumber, fontSize);
		CmdFont = fontNumber;
		CmdFontSize = fontSize;
		GetFontName(CmdFont, CmdFontName);
		wp = CmdWindows + CurrentWindow;
		SetPort(wp->cmdWind);
		TextFont(CmdFont);
		TextSize(CmdFontSize);
		GetFontInfo(&thisFont);
		adjustLeading(CmdFont, CmdFontSize, &thisFont);
		(*(wp->teCmd))->lineHeight = thisFont.ascent + thisFont.descent +
			thisFont.leading;
		(*(wp->teCmd))->fontAscent = thisFont.ascent;
		(*(wp->teCmd))->txFont = CmdFont;
		(*(wp->teCmd))->txSize = CmdFontSize;
		(*(wp->teCmd))->destRect = (*(wp->teCmd))->viewRect;
		CmdLine = 0;
		InvalRect(&(wp->cmdWind)->portRect);
		BeginUpdate(wp->cmdWind);
		CmdWindUpdate(true);
		EndUpdate(wp->cmdWind);
	}
	SetPort(thePort);
} /*setCmdWindFont()*/

/*
   Function to set the font for the current window and the default font used
   in new windows.  If newFontSize > 0,  the font size is also set.  Called
   from functions setDefaultOptions() and setOpt() in setOptio.c

   Note: setDefaultOptions() and hence macSetNewFont() is called from
   initialize() which might be (and is, on a Macintosh) before any
   windows have been created.  Hence special care needs to be taken.
   On the Macintosh, at that stage, Usewindows is always 0.
*/
#define DEFAULTFONT        "default" /* also defined in setOptio.c */
#define DEFAULTFONTLENGTH  7 /*strlen(DEFAULTFONT)*/
void macSetNewFont(char *newFontName, short newFontSize)
{
	Str27      fontName;
	Integer    fontSize = newFontSize;
	Integer    fontNumber = -1;
	WHERE("macSetNewFont");
	
	if (UseWindows)
	{
		if (newFontName == (char *) 0)
		{
			str255cpy(fontName, CmdFontName);
		}
		else if (mystrncmp(newFontName, DEFAULTFONT,DEFAULTFONTLENGTH) == 0 ||
			 mystrncmp(newFontName, "orig", 4) == 0)
		{
			str255cpy(fontName, CMDFONTNAME);
			if (fontSize < 0)
			{
				fontSize = CMDFONTSIZE;
			} /*if (fontSize < 0)*/
		}
		else
		{
			strcpy((char *) fontName, newFontName);
			CtoPstr((char *) fontName);
		}

		if (fontSize < 0)
		{
			fontSize = CmdFontSize;
		}

		if (fontSize != CmdFontSize || str255cmp(fontName, CmdFontName) != 0)
		{ /* font size or name has been changed */
			GetFNum(fontName, &fontNumber);
			if (fontNumber == 0 && str255cmp(fontName, "\pChicago") != 0)
			{
				PtoCstr(fontName);
				sprintf(OUTSTR,
						"WARNING: font \"%s\" is not available; not changed",
						fontName);
				putOUTSTR();
			}
			else
			{
				setCmdWindFont(fontNumber,fontSize);
			}
		} /*if (fontSize != CmdFontSize || str255cmp(fontName, CmdFontName) != 0)*/
		else
		{ /* reset SCREENWIDTH and SCREENHEIGHT even if no font change */
			setCmdScreen();
		}
	} /*if (UseWindows)*/
} /*macSetNewFont()*/

/*
	The following is supposed to copy the last few lines of the command window
	to the Clipboard in case of a fatal error, but it does not seem always
	to work.  Unless a copy was done in MacAnova, on exit, the Clipboard
	holds what it did before invoking MacAnova
*/
void onFatalError(void)
{
	Integer     nLines;
	Integer     selStart;
	Integer     selEnd;

	myAlert("FATAL ERROR: Execution terminated");
	nLines = NLines(teCmd);
	selStart = LineStarts(teCmd)[nLines-CLIPLINES];
	selEnd = TextLength(teCmd);
	if(selEnd > 0)
	{
		TESetSelect((selStart > 0) ? selStart : 0 , selEnd, teCmd);
		DoEdit(copyit);
	}
} /*onFatalError()*/

/*
	Set screen dimensions, etc. for CmdWind
*/

void setCmdScreen(void)
{
	Rect         r = (*teCmd)->viewRect;

	CmdFont = (*teCmd)->txFont;
	CmdFontSize = (*teCmd)->txSize;
	GetFontName(CmdFont, CmdFontName);

	installScreenDims((r.right - r.left - 1)/CharWidth((Integer) Zero),
					  (r.bottom - r.top)/ (*teCmd)->lineHeight);
	saveFormat();
	ViewLines = SCREENHEIGHT;
	CmdScrollLines = (*teCmd)->nLines - ViewLines;
	HalfPage = ViewLines / 2;
	CmdInsertLine = GetCmdIL();

} /*setCmdScreen()*/

/*
	General deactivate procedure to be used before bringing up modal dialog
*/
void DeactivateWindow(void)
{
	GrafPtr         thePort;
	WindowPtr       theWindow  = FrontWindow();
	Integer         windno;

	if(UseWindows > 0)
	{
		GetPort(&thePort);
		SetPort((GrafPtr) theWindow);
		HiliteWindow(theWindow, false);
		if((windno = whichCmdWindow(theWindow)) >= 0)
		{
			CmdWindActivate(false);
		}
		else if((windno = whichGraphWindow(theWindow)) >= 0)
		{
			GraphWindActivate(false);
		}
		SetPort(thePort);
	}
} /*DeactivateWindow()*/

/*
	General activate/deactivate procedure to be used before bringing up modal dialog
*/
pascal void ActivateWindow(Boolean active)
{
	GrafPtr         thePort;
	WindowPtr       theWindow  = FrontWindow();
	Integer         windno;

	if(UseWindows > 0)
	{
		GetPort(&thePort);
		SetPort((GrafPtr) theWindow);
		HiliteWindow(theWindow, active);
		if((windno = whichCmdWindow(theWindow)) >= 0)
		{
			CmdWindActivate(active);
		}
		else if((windno = whichGraphWindow(theWindow)) >= 0)
		{
			GraphWindActivate(active);
		}
		SetPort(thePort);
	}
} /*ActivateWindow()*/
/*
	Route update to either Command Window or Grapoh Window as appropriate
*/
void macUpdate(WindowPtr wind)
{
	GrafPtr            thePort;
	Integer            windno;
	Boolean            all = (wind == (WindowPtr) 0);
	Integer            i, i1, i2;	
	WHERE("macUpdate");

	GetPort(&thePort);

	if(all)
	{
		i1 = -NGRAPHS - 1;
		i2 = MAXWINDOWS;
	} /*if(all)*/
	else
	{
		if((windno = whichCmdWindow(wind)) >= 0)
		{
			i1 = windno;
		}
		else if((windno = whichGraphWindow(wind)) >= 0)
		{
			i1 = -windno - 1;
		}
		else
		{
			return;
		}
		i2 = i1 + 1;
	} /*if(all){}else{}*/

	for(i = i1;i < i2;i++)
	{
		if(i >= 0)
		{
			wind = CmdWindows[i].cmdWind;
		}
		else if(-i <= NGRAPHS)
		{
			wind = GraphWind[-i-1];
		}
		else
		{
			wind = PanelWind[-i - 1 - NGRAPHS];
		}

		if(wind != (WindowPtr) 0 && ((WindowPeek) wind)->visible)
		{
			SetPort((GrafPtr) wind);
			BeginUpdate(wind);
			if(i >= 0)
			{
				CmdWindUpdate(false);
			}
			else
			{
				GraphWindUpdate(false);
			}
			EndUpdate(wind);
		} /*if(wind != (WindowPtr) 0 && ((WindowPeek) wind)->visible)*/
	} /*for(i = i1;i < i2;i++)*/
	
	SetPort(thePort);
} /*macUpdate()*/

/*
	Show a window if it's not visible.  Select the window FIRST, then
	show it, so that it comes up in front.  Otherwise it will be drawn
	in back then brought to the front, which is ugly.

	The test for visibility must be done carefully:  the window manager
	stores 255 and 0 for true and false, not real boolean values.
*/

void MyShowWindow(WindowPtr wind)
{
	if (wind != (WindowPtr) 0)
	{
		SelectWindow(wind);
		ShowWindow(wind);
		if (wind == BatchDialog)
		{
			DrawControls(BatchDialog);
		}
	} /*if (wind != (WindowPtr) 0)*/
} /*MyShowWindow()*/

/*
	Print error messages related to file handling
*/
void fileErrorMsg(Integer errorFlag,char * fileName)
{
	char       *msg;
	
	if(errorFlag != noErr)
	{
		if(errorFlag == dskFulErr)
		{
			putOutErrorMsg("ERROR: disk full");
		}
		else if(errorFlag == vLckdErr || errorFlag == wPrErr)
		{
			putOutErrorMsg("ERROR: disk locked");
		}
		else if(errorFlag == dirFulErr)
		{
			putOutErrorMsg("ERROR: File directory full");
		}
		else if(errorFlag == bdNamErr)
		{
			putOutErrorMsg("ERROR: Bad file or volume name");
		}
		else if(errorFlag == eofErr)
		{
			myprint("ERROR: End file hit while reading ");
			putOutErrorMsg(fileName);
		}
		else if(errorFlag == nsvErr)
		{
			putOutErrorMsg("ERROR: no such volume");
		}
		else if(errorFlag == ioErr)
		{
			putOutErrorMsg("ERROR: I/O problem");
		}
		else if(errorFlag == tmfoErr)
		{
			putOutErrorMsg("ERROR: too many files open");
		}
		else if(errorFlag == vLckdErr || errorFlag == wPrErr)
		{
			putOutErrorMsg("ERROR: disk is locked");
		}
		else if(errorFlag == fBsyErr || errorFlag == fLckdErr ||
			errorFlag == fnfErr || errorFlag == fnOpnErr ||
			errorFlag == opWrErr || errorFlag == gfpErr ||
			errorFlag == wrPermErr)
		{
			if(errorFlag == fnfErr)
			{
				msg = " not found";
			}
			else if(errorFlag == fnOpnErr)
			{
				msg = " not open";
			}
			else if(errorFlag == fBsyErr)
			{
				msg = " busy";
			}
			else if(errorFlag == fLckdErr)
			{
				msg = " locked";
			}
			else if(errorFlag == opWrErr)
			{
				msg = " is already open for writing";
			}
			else if(errorFlag == wrPermErr)
			{
				msg = " not available for writing";
			}
			else
			{
				msg = " has a problem";
			}
			myprint("ERROR: File ");
			myprint(fileName);
			putOutErrorMsg(msg);
		}
		else
		{
			putOutErrorMsg("ERROR: File or disk problem");
		}
	}
} /*fileErrorMsg()*/

/*
	960502 Function to return full path name of directory
	In case of error, it returns a null string ("")
	
	volume and directory for current default directory can be obtained by
		errorFlag = HGetVol((StringPtr) 0, &vvolume, &directory);

	Following code adapted from function GetFullPath on p. 2-45 of
	Inside Macintosh - Files
*/
char * macGetPath(char *fullPath, Integer volume, LongInt directory)
{
	CInfoPBRec      pb;
	Integer         errorFlag;
	char           *place1, *place2;
	Str255          dirName;
	WHERE("macGetPath");
	
	*fullPath = '\0';
	pb.hFileInfo.ioCompletion = 0;
	pb.hFileInfo.ioNamePtr = dirName;
	pb.hFileInfo.ioVRefNum = volume;
	pb.dirInfo.ioDrParID = directory;
	pb.dirInfo.ioFDirIndex = -1;
	do
	{
		pb.dirInfo.ioDrDirID = pb.dirInfo.ioDrParID;
		errorFlag = PBGetCatInfo(&pb, false);
		if (errorFlag != noErr)
		{
			goto errorExit;
		}
		PtoCstr(dirName);

		place1 = fullPath + strlen(fullPath);
		place2 = place1 + strlen((char *) dirName) + 1;
		if (place2 - fullPath > PATHSIZE)
		{ /* path too long */
			goto errorExit;
		}
		while (place1 >= fullPath)
		{
			*place2-- = *place1--;
		}
		strcpy(fullPath, (char *) dirName);
		*place2 = ':';
		CtoPstr((char *) dirName);
	} while (pb.dirInfo.ioDrDirID != fsRtDirID);
	return (fullPath);

  errorExit:
	*fullPath = '\0';
	return (fullPath);
} /*macGetPath()*/

/*
	Let i = abs(doit) - 1
	doit > 0 means call PrStlDialog whether or not PrintHndl[i] != 0
	doit < 0 means call PrStlDialog only if PrintHndl[i] == 0
*/
Integer doPageSetup(Integer doit)
{
	OSErr            errorCode;
	Integer          windowType = (abs(doit) == TEXTPRINT) ? 0 : 1;
	Integer          reply = 1;
	WHERE("doPageSetup");

	PrOpen();
	if(PrintHdl[windowType] == (THPrint) 0)
	{
		PrintHdl[windowType] = (THPrint) NewHandle(sizeof(TPrint));
		errorCode = MemError();
		if(errorCode == memFullErr)
		{
			putOutErrorMsg("WARNING: apparently out of memory");
			reply = doit = 0;
			PrintHdl[windowType] = (THPrint) 0;
		}
		else
		{
			PrintDefault(PrintHdl[windowType]);
			doit = 1;			
		}
	} /*if(PrintHdl[windowType] == (THPrint) 0)*/

	if(doit > 0)
	{
		ActivateWindow(false);
		if(!PrStlDialog(PrintHdl[windowType]))
		{
			reply = 0;
			DisposHandle((Handle) PrintHdl[windowType]);
			PrintHdl[windowType] = (THPrint) 0;
		}
	} /*if(doit > 0)*/

	PrClose();

	return (reply);
} /*doPageSetup()*/


/*
	Dialog and Alert Utilities
*/

void myAlert(char * msgs)
{
	Integer       id = MYALERT;
	Str15         nullString = "\p";
	
	ActivateWindow(false); /* deactivate Front Window */
	CtoPstr(msgs);
	ParamText((const unsigned char *) msgs, nullString, nullString, nullString);
	PtoCstr((unsigned char *) msgs);
	DialogEdit = false;
	NDialogButtons = 1;
	ButtonChars[0] = 'y'; /*OK*/
	(void) Alert(id, MyDialogFilterPtr);
}/*myAlert()*/
	
/*
	Put up three button alert box with choices Don't Save, Cancel, Save
*/
Integer saveAlert(Str255 fileName, Boolean cmdwind)
{
	Str15       nullString = "\p";
	Str255      message;

	if(cmdwind)
	{
		PtoCstr(fileName);
		sprintf((char *) message, "Save Changes to Window “%s” Before Closing?",
				fileName);
		CtoPstr((char *) fileName);
	}
	else
	{
		if(KEEPFILENAME == (char **) 0)
		{
			sprintf((char *) message, "Save Workspace Before Quitting?");
		}
		else
		{
			sprintf((char *) message, "Save Workspace “%s” Before Quitting?",
					*KEEPFILENAME);
		}
	}
	
	CtoPstr((char *) message);
	DialogEdit = false;
	NDialogButtons = 3;
	ButtonChars[0] = 's'; /*Save*/
	ButtonChars[1] = 'c'; /*Cancel*/
	ButtonChars[2] = 'd'; /*Don't Save*/
	ParamText(message,  nullString,  nullString,
			   nullString);
	return (CautionAlert(SAVEALERT, MyDialogFilterPtr));
} /*saveAlert()*/

/*
	ok button of Dialog Box is labelled Cancel
	cancel button is labelled Quit
*/

Boolean quitAlert(char ch)
{
	char     *why = "";
	if(!twoChoiceAlert("Cancel", "Quit", "nq", "Do you really want to quit?"))
	{
		if(ch == 'q' || ch == 'Q')
		{
			why = " by Quit";
		}
		else if(ch == 'i' || ch == 'I')
		{
			why = " by Interrupt";
		}
		else if(ch == 'e' || ch == 'E')
		{
			why = " by Error";
		}

		myeol();
		sprintf(OUTSTR,	"MacAnova terminated%s", why);
		putOUTSTR();
		return (false);
	}
	return (true);
} /*quitAlert()*/

/*
	Function to put up generic two choice modal dialog.
	Button 1 is default and will be outlined.
	Strings label1 and label2 are button labels (e.g., "Cancel", "Quit")
	msgs is string for static text item.
	Returns true for ok and false for cancel
*/

#define TWOCHOICEDLOG 2005  /* resource number of generic two button model dialog*/

Boolean twoChoiceAlert(char *label1, char *label2, char *keys, char *msgs)
{
	DialogPtr     theDialog;
	Str255        message;
	Str15         nullString = "\p";
	Integer       itemHit, itemType;
	Rect          r;
	ControlHandle itemHandle;

	theDialog = getDlog(TWOCHOICEDLOG, false, 2, keys);

	strncpy((char *) message, msgs, 255);
	message[255] = '\0';
	CtoPstr((char *) message);
	ParamText(message,  nullString,  nullString,
			   nullString);

	strcpy((char *) message, label1);
	CtoPstr((char *) message);
	GetDItem(theDialog, ok, &itemType, (Handle *) &itemHandle, &r);
	SetCTitle(itemHandle,  message);

	strcpy((char *) message, label2);
	CtoPstr((char *) message);
	GetDItem(theDialog, cancel, &itemType, (Handle *) &itemHandle, &r);
	SetCTitle(itemHandle,  message);

	ShowWindow((WindowPtr) theDialog);
/*	DrawDialog(theDialog);*/

	while (1)
	{
		ModalDialog(MyDialogFilterPtr, &itemHit);
		if(itemHit <= cancel)
		{
			break;
		}
	}
	DisposDialog(theDialog);
	return (itemHit == ok);
} /*twoChoiceAlert()*/

/*
	Procedure to outline default button in dialog and alert boxes
*/
pascal void outlineOK(WindowPtr theWindow,Integer itemNo)
{
	DialogPtr       theDialog = (DialogPtr) theWindow;
	Rect            r;
	Handle          itemHandle;
	Integer         itemType;

	GetDItem(theDialog,itemNo, &itemType, &itemHandle, &r);
	PenSize(3,3);
	InsetRect(&r,-4,-4);
	FrameRoundRect(&r,16,16);

} /*outlineOK()*/

#define CountDITL countDitl

static Integer countDitl(DialogPtr theDialog)
{
	DialogPeek       dPeek = (DialogPeek) theDialog;
	Handle           resList = dPeek->items;
	Integer          count;

	HLock(resList);
	count = ((Integer *) *resList)[0] + 1;
	HUnlock(resList);
	return (count);
}
/*
	Event filter for dialog boxes
	
	The number of buttons should be in global NDialogButtons
	Keyboard equivalents should be in ButtonChars[i],i=0,...,NdialogButtons-1

	The dialog has an editable text item if DialogEdit is true.  Generally,
	use 'n' for cancel and 'y' (or 's' for Save) for ok.

	Also, if NDialogButtons > ok (1), Escape and command period mean cancel.
	If NDialogButtons >= ok, Return and Enter mean ok.

	If the cursor is over an editText item, the cursor is set to the I-beam.
*/

pascal Boolean myDialogFilter (DialogPtr theDialog, EventRecord *theEvent,
							   Integer *itemHit)
{
	Point           evtPt = theEvent->where;
	char			evtChar, c;
	Integer         evtMods = theEvent->modifiers;
	Integer         nItems = CountDITL(theDialog);
	Integer         i;
	Integer         itemType;
	Handle          itemHandle;
	Rect            box;
	Point           mouseLoc;
	Boolean         retval = false;
	GrafPtr         thePort;
	WHERE("myDialogFilter");

	mouseLoc.v = theEvent->where.v;
	mouseLoc.h = theEvent->where.h;
	GetPort(&thePort);
	SetPort((GrafPtr) theDialog);
	GlobalToLocal(&mouseLoc);
	SetPort(thePort);
	for(i=1;i<=nItems;i++)
	{
		GetDItem(theDialog, i, &itemType, &itemHandle, &box);
		if((itemType & 0x7f) == editText && PtInRect(mouseLoc, &box))
		{
			break;
		}
	} /*for(i=1;i<=nItems;i++)*/


	if(i > nItems)
	{
		InitCursor();
	}
	else
	{
		SetCursor(&IBEAM);
	}

	switch (theEvent->what)
	{
/*
	If Command key event, compare Command Key with ButtonChars or process
	editing command key combinations

	If ordinary key event, process '.' CR, Enter, and Escape.
*/
	  case keyDown:
	  case autoKey:
		evtChar = theEvent->message & charCodeMask;
		if (evtMods & cmdKey)
		{	/* Command keys */
			if(isupper(evtChar))
			{
				evtChar = tolower(evtChar);
			}
			for(i=1;i<=NDialogButtons;i++)
			{ /* is it keyboard equivalent of a button? */
				c = ButtonChars[i-1];
				if(c != '\0' && evtChar == c)
				{
					*itemHit = i;
					retval = true;
					break;
				}
			} /*for(i=1;i<=NDialogButtons;i++)*/

			if(DialogEdit && evtChar == Newline || evtChar == Enter)
			{ /* turn Command Return into Return */
				theEvent->modifiers = evtMods ^ cmdKey;
				theEvent->message = (theEvent->message & ~charCodeMask) | 0x0d;
			}
			else if(!retval)
			{
				c = ButtonChars[cancel - 1];
				if(NDialogButtons >= cancel &&
						c != '\0' && c != 'q' && evtChar == Period)
				{
					*itemHit = cancel;
					retval = true;
				}
				if(DialogEdit)
				{/* are they editing characters*/
					if(evtChar == 'c')
					{
						DlgCopy(theDialog);
						(void) ZeroScrap();
						(void) TEToScrap();
						theEvent->what = nullEvent;
					}
					else if(evtChar == 'x')
					{
						DlgCut(theDialog);
						(void) ZeroScrap();
						(void) TEToScrap();
						theEvent->what = nullEvent;
					}
					else if(evtChar == 'v')
					{
						(void) TEFromScrap();
						if(TEGetScrapLen() > 0)
						{
							DlgPaste(theDialog);
						}
						theEvent->what = nullEvent;
					}
				} /*if(DialogEdit)*/
			} /*if(!retval)*/
		}  /*if (evtMods & cmdKey)*/
		else
		{ /* not command key */
			if(ButtonChars[0] != '\0' && (evtChar == Return || evtChar == Enter))
			{
				*itemHit = ok;
				retval = true;
			}
			else if(NDialogButtons >= cancel && ButtonChars[cancel-1] != '\0' &&
					evtChar == EscapeKey)
			{
				*itemHit = cancel;
				retval = true;
			}
		} /*if (evtMods & cmdKey){}else{}*/
		break;

	  default:
		break;
	} /*switch (theEvent->what)*/
	return (retval);
} /*myDialogFilter*/

/*
	general routine to startup dialog boxes with buttons, outlining
	OK button and centering on the screen
	
	970503 fixed so that it uses rectangle of main device, not the
	total gray area for centering, to fix problem of dialog boxes being
	split between multiple screens
	
*/

#define FRACTION    3  /* 1/FRACTION of unused vertical distance above window*/

DialogPtr getDlog(Integer id, Boolean dialogEdit, Integer nButtons,
				  char *buttonCh)
{
	DialogPtr       theDialog;
	Rect            r, r1;
	Handle          itemHandle;
	Integer         itemType, i;
	WHERE("getDlog");

	ActivateWindow(false); /* deactivate front window */

	theDialog =	GetNewDialog(id, (Ptr) nil, (WindowPtr) -1);

	/* Center dialog box */
	r = theDialog->portRect;
	getScreenRect(&r1);

#if (0)
	if (r1.top == 0 && r1.left == 0 && r1.bottom == 0 && r1.right == 0)
	{
		r1 = r;
	}
#endif

	MoveWindow((WindowPtr) theDialog,
		r1.left + (r1.right - r1.left - (r.right - r.left))/2,
		r1.top + (r1.bottom - r1.top - (r.bottom - r.top))/FRACTION, false);

	GetDItem(theDialog, nButtons+1, &itemType, &itemHandle, &r);

	if((itemType & 0x7f) == userItem)
	{ /* set up outlining of OK button */
		GetDItem(theDialog, ok, &i, &itemHandle, &r);
		SetDItem(theDialog, nButtons+1, itemType, (Handle) OutlineOKUserItemPtr, &r);
	}

	DialogEdit = dialogEdit;
	NDialogButtons = nButtons;
	for(i = 0;i<nButtons;i++)
	{
		ButtonChars[i] = buttonCh[i];
	}
	if(dialogEdit)
	{
		SetCursor(&IBEAM);
	}

	return (theDialog);
} /*getDlog()*/

/*
	general routine to get rid of dialog boxes
*/

/*
	General routine to set number in text edit field
*/

void setDlogNumber(DialogPtr theDialog, Integer item, double value, char * fmt,
				   Boolean select)
{
	Integer          itemType;
	Handle           itemHandle;
	Rect             r;
	Str255           textItem;
	
	GetDItem(theDialog, item, &itemType, &itemHandle, &r);
	itemType &= 0x7f;
	if (itemType == editText || itemType == statText)
	{
		sprintf((char *) textItem, fmt, value);
		CtoPstr((char *) textItem);
		SetIText(itemHandle, textItem);
		if(select)
		{
			SelIText(theDialog, item, 0, 32767);
		}
	}
} /*setDlogNumber()*/

/*
	function to set nitems consecutiveradio buttons or check boxes,
	starting with, 	control number item, using bits in value
*/
void setDlogItemValues(DialogPtr theDialog, Integer item, Integer nitems,
					   Integer value)
{
	Integer          itemType, i;
	ControlHandle    itemHandle;
	Rect             r;
	WHERE("setDlogItemValues");

	for(i=0;i < nitems;i++)
	{
		GetDItem(theDialog, item, &itemType, (Handle *) &itemHandle, &r);
		itemType = (itemType & 0x7f) - ctrlItem;
		if(itemType == radCtrl || itemType == chkCtrl)
		{
			SetCtlValue (itemHandle, (Integer) (value & 1));
		}
		item++;
		value >>= 1;
	}
} /*setDlogItemValues()*/

Integer getDlogItemValues(DialogPtr theDialog, Integer item, Integer nitems)
{
	Integer          itemType, value = 0, i, mask = 1;
	ControlHandle    itemHandle;
	Rect             r;
	WHERE("getDlogItemValues");

	item += nitems-1;
	for(i = 0; i < nitems;i++)
	{
		value <<= 1;
		GetDItem(theDialog, item, &itemType, (Handle *) &itemHandle, &r);
		itemType = (itemType & 0x7f) - ctrlItem;
		if(itemType == radCtrl || itemType == chkCtrl)
		{
			value |= GetCtlValue(itemHandle);
		}
		item--;
	}
	return (value);
} /*getDlogItemValues()*/

pascal void DialogClobber(void)
{
	GrafPtr    thePort;

	GetPort(&thePort);
	DisposDialog((DialogPtr) thePort);

} /*DialogClobber()*/

LongInt scrapSize(void)
{
	ScrapStuff        *info;
	
	info = InfoScrap();
	return (info->scrapSize);
} /*scrapSize()*/

/*
	Does not change scrap if text is zero length
*/
int myputscrap(char **text)
{
	long        length = myhandlelength(text) - 1;
	
	return ((length == 0 || length > 0 && (ZeroScrap() == noErr &&
							PutScrap(length, 'TEXT', (Ptr) *text) == noErr)));
} /*myputscrap()*/

char ** mygetscrap(void)
{
	char         **clipText;
	LongInt        length;
	Handle         hDest = (Handle) 0;
	LongInt        offset;
	WHERE("mygetscrap");
	
	hDest = NewHandle(0);
	length = (hDest == (Handle) 0) ? 0 : GetScrap (hDest, 'TEXT', &offset);
	if (length == noTypeErr)
	{ /* no 'TEXT' stuff on Clipboard */
		length = 0;
	}
	else
	{
		clipText = mygethandle(length+1);
		if (clipText != (char **) 0)
		{
			if (length > 0)
			{
				HLock(hDest);
				BlockMove(*hDest, *clipText, length);
			}		
			(*clipText)[length] = '\0';
		}
		else
		{
			putOutErrorMsg("WARNING: not enough memory to copy from Clipboard");
		}
	}
	
	if(hDest != (Handle) 0)
	{
		DisposHandle(hDest);
	}
	return (clipText);
} /*mygetscrap()*/

/*
	str255cmp returns 0 when str1 and str2 are identical, 1 otherwise
	It preserves no ordering information at present
*/
Integer str255cmp(Str255 str1, Str255 str2)
{
	Integer        i, n = str1[0];

	for (i = 0; i <= n; i++)
	{
		if (str1[i] != str2[i])
		{
			return (1);
		}
	}
	return (0);
}

/*
	copy str2 to str1
*/
void str255cpy(Str255 str1, Str255 str2)
{
	Integer        i, n = str2[0];

	for (i = 0; i <= n; i++)
	{
		str1[i] = str2[i];
	}
}

void myDebug(char *msgs)
{
	CtoPstr(msgs);
	DebugStr((StringPtr) msgs);
	PtoCstr((unsigned char *) msgs);
} /*myDebug()*/
