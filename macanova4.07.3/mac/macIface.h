/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 4.07 or later
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

/*
  Header unifying Mac interface

  960513  Added InterruptInterval
          Changed initialization value for BackgroundSleep
  960515  Added capability for more graph and panel windows
  960521  Add globals for a preview picture for EPSF file
  960718  Added item copyandexecute to Edit menu
  970714  Added global Has68881
  971128  Added ifdefs for MW Codewarrior 11
  980515  Corrected preceding for MWCW 11 68K compilation
  980517  Changed many defines of keycodes, etc. to enums
  980825  Added HelpMenu and related items; more cosmetic changes.
*/
#ifndef MACIFACEH__
#define MACIFACEH__
#undef EXTERN
#undef COMMA
#define COMMA ,

#ifdef MAIN__
#define EXTERN
#define INIT(VAL) = VAL
#define INITDIM(N) N
#define INITARRAY(VALS) = { VALS }
#else
#define EXTERN extern
#define INIT(VAL)
#define INITDIM(N)
#define INITARRAY(VALS)
#endif /*MAIN__*/

#ifndef PLATFORMH__
#include "platform.h"
#endif /* PLATFORMH__ */

#if defined(MPW) || defined(MW_CW)
#ifndef MACHEADERSH__
#define PtoCstr p2cstr
#define CtoPstr c2pstr
#include <Dialogs.h>	/* pulls in Windows.h, Types.h, QuickDraw.h */
#include <Events.h>
#ifndef MW_CW_New
# include <Desk.h>
#endif /*MW_CW_New*/
#include <Scrap.h>
#include <SegLoad.h>
#include <Memory.h>
#include <DiskInit.h>
#include <Sound.h>
#ifndef MW_CW_New
#include <OSEvents.h>
#endif /*MW_CW_New*/
#include <Traps.h>
#include <Errors.h>
#include <Resources.h>
#include <Fonts.h>
#include <ToolUtils.h>
#include <Controls.h>
#include <Packages.h>
#include <Files.h>
#include <Menus.h>
#include <Balloons.h>
#include <strings.h>
#include <string.h>
#include <Printing.h>
#include <TextEdit.h>
#endif MACHEADERSH__
#else
#include <WindowMgr.h>	/* pulls in MacTypes.h, QuickDraw.h */
#include <ControlMgr.h>
#include <StdFilePkg.h>
#include <FileMgr.h>
#include <MenuMgr.h>
#include <string.h>
#include <TextEdit.h>
#include <OSUtil.h>
#include <PrintTraps.h>
#endif

#if defined(MW_CW_New) && !defined(NEWNAMES_SET) /*various name changed required in CW 11*/
#define NEWNAMES_SET

/*
	The following sets of defines, each headed by a comment from "xxxx.h"
	are taken from headers that came with Metrowerks CodeWarrior compiler 11
	and are copyright by Metrowerks or Apple
*/
/* from Menu.h */
#define AddResMenu(theMenu, theType) AppendResMenu(theMenu, theType)
#define InsMenuItem(theMenu, itemString, afterItem)	InsertMenuItem(theMenu, itemString, afterItem)
#define DelMenuItem( theMenu, item ) DeleteMenuItem( theMenu, item )
#define SetItem(theMenu, item, itemString) SetMenuItemText(theMenu, item, itemString)
#define GetItem(theMenu, item, itemString) GetMenuItemText(theMenu, item, itemString)
#define GetMHandle(menuID) GetMenuHandle(menuID)
#define DelMCEntries(menuID, menuItem) DeleteMCEntries(menuID, menuItem)
#define DispMCInfo(menuCTbl) DisposeMCInfo(menuCTbl)

/* from MacMemory.h*/
#define ResrvMem(cbNeeded) ReserveMem(cbNeeded)
#define DisposPtr(p) DisposePtr(p)
#define DisposHandle(h) DisposeHandle(h)
#define ReallocHandle(h, byteCount) ReallocateHandle(h, byteCount)

/* from Controls.h*/
#define SetCtlMax(theControl, maxValue) SetControlMaximum(theControl, maxValue)
#define GetCtlMax(theControl) GetControlMaximum(theControl)
#define SetCtlValue(theControl, theValue) SetControlValue(theControl, theValue)
#define GetCtlValue(theControl) GetControlValue(theControl)
#define GetCRefCon(theControl) GetControlReference(theControl)
#define SetCRefCon(theControl, data) SetControlReference(theControl, data)
#define GetCTitle(theControl, title) GetControlTitle(theControl, title)
#define SetCTitle(theControl, title) SetControlTitle(theControl, title)
enum {
	inLabel						= kControlLabelPart,
	inMenu						= kControlMenuPart,
	inTriangle					= kControlTrianglePart,
	inButton					= kControlButtonPart,
	inCheckBox					= kControlCheckBoxPart,
	inUpButton					= kControlUpButtonPart,
	inDownButton				= kControlDownButtonPart,
	inPageUp					= kControlPageUpPart,
	inPageDown					= kControlPageDownPart,
	inThumb						= kControlIndicatorPart
};

/* from TextEdit.h*/
#define TEGetScrapLen() TEGetScrapLength()
#define SetClikLoop(clikProc, hTE) TESetClickLoop(clikProc, hTE)

/* from Dialog.h*/
#define GetDItem(theDialog, itemNo, itemType, item, box) GetDialogItem(theDialog, itemNo, itemType, item, box)
#define SetDItem(theDialog, itemNo, itemType, item, box) SetDialogItem(theDialog, itemNo, itemType, item, box)
#define SelIText(theDialog, itemNo, strtSel, endSel) SelectDialogItemText(theDialog, itemNo, strtSel, endSel)
#define GetIText(item, text) GetDialogItemText(item, text)
#define SetIText(item, text) SetDialogItemText(item, text)
#define DlgCut(theDialog) DialogCut(theDialog)
#define DlgPaste(theDialog) DialogPaste(theDialog)
#define DlgCopy(theDialog) DialogCopy(theDialog)
#define SetDAFont(fontNum) SetDialogFont(fontNum)
#define DisposDialog(theDialog) DisposeDialog(theDialog)

/* from Fonts.h*/
enum {
	newYork						= kFontIDNewYork,
	geneva						= kFontIDGeneva,
	monaco						= kFontIDMonaco,
	venice						= kFontIDVenice,
	london						= kFontIDLondon,
	athens						= kFontIDAthens,
	sanFran						= kFontIDSanFrancisco,
	toronto						= kFontIDToronto,
	cairo						= kFontIDCairo,
	losAngeles					= kFontIDLosAngeles,
	times						= kFontIDTimes,
	helvetica					= kFontIDHelvetica,
	courier						= kFontIDCourier,
	symbol						= kFontIDSymbol,
	mobile						= kFontIDMobile
};

/* from SegLoad.h */
#if (!TARGET_CPU_68K)
enum {
	appOpen						= 0,							/*Open the Document (s)*/
	appPrint					= 1								/*Print the Document (s)*/
};

struct AppFile {
	short 							vRefNum;
	OSType 							fType;
	short 							versNum;					/*versNum in high byte*/
	Str255 							fName;
};
typedef struct AppFile AppFile;

EXTERN_API( void )
CountAppFiles					(short *				message,
								 short *				count);

EXTERN_API( void )
GetAppFiles						(short 					index,
								 AppFile *				theFile);

EXTERN_API( void )
ClrAppFiles						(short 					index);

EXTERN_API( void )
GetAppParms						(Str255 				apName,
								 short *				apRefNum,
								 Handle *				apParam) ONEWORDINLINE(0xA9F5);
#endif /*!TARGET_CPU_68K*/

/* from Resources.h */
#define SizeResource(theResource) GetResourceSizeOnDisk(theResource)
#define MaxSizeRsrc(theResource) GetMaxResourceSize(theResource)
#define RmveResource(theResource) RemoveResource(theResource)
#endif /*MW_CW_New*/

#ifdef MW_CW
#define STR255 unsigned char *
#else /*MW_CW*/
#define STR255 Str255
#endif /*MW_CW*/
#ifndef MACTYPEDEF

/* Typedefs for Macintosh */
typedef SIZEINTEGER          Integer;
typedef SIZELONGINT          LongInt;
typedef unsigned SIZEINTEGER uInteger;
typedef unsigned SIZELONGINT uLongInt;

#ifndef MPW3_2
typedef pascal void (*ResumeProcPtr)(void); 
#endif

#ifdef MPW

#ifndef CPROTO
typedef long Size;
#else /* get around problem with cproto */
#define Size long
#endif /*CPROTO*/
#endif /*MPW*/


#define MACTYPEDEF /* indicates preceding typedefs done */
#endif /*MACTYPEDEF*/

#ifdef MW_CW
#define ModalFilterProcPtr ModalFilterUPP
#define DlgHookProcPtr DlgHookUPP
#define FileFilterProcPtr FileFilterUPP
#endif /* MW_CW */

#include "TransSkel.h"
#ifndef CMDSLEEP
#define CMDSLEEP   3L
#endif /*CMDSLEEP*/

#ifndef BATCHSLEEP
#define BATCHSLEEP 0L  /* minimizes time lost by WaitNextEvent() */
#endif /*BATCHSLEEP*/

#ifndef INTERRUPTSLEEP
#define INTERRUPTSLEEP 0L  /* minimizes time lost by WaitNextEvent() */
#endif /*INTERRUPTSLEEP*/

#ifndef BACKGROUNDSLEEP
#define BACKGROUNDSLEEP 2L  /* wait time while in background */
#endif /*BACKGROUNDSLEEP*/

EXTERN LongInt     Sleep INIT(CMDSLEEP);
EXTERN LongInt     CmdSleep INIT(CMDSLEEP);
EXTERN LongInt     BatchSleep INIT(BATCHSLEEP);
EXTERN LongInt     InterruptSleep INIT(INTERRUPTSLEEP);
EXTERN LongInt     BackgroundSleep INIT(BACKGROUNDSLEEP);

/*
	The following is in handlers.h
	EXTERN LongInt     InterruptInterval INIT(INTERRUPTINTERVAL); 
*/

#ifndef nil
# define	nil		0L
#endif /*nil*/

/*
	Globals related to non-interactive mode
	UseWindows indicates the status of non-interactive mode
	If non-zero all calles to print to windows are short circuited
	All graphics will be DUMB
	Value < 0 means during startup sequence
	Value > 0 means use windows
	Value of 0 means don't use windows
*/

EXTERN Integer UseWindows INIT(0);
EXTERN Boolean SaveOnQuit INIT(true);
EXTERN Boolean Has68881   INIT(false);
/*
	Menu handles.
*/

EXTERN MenuHandle      AppleMenu;
EXTERN MenuHandle      FileMenu;
EXTERN MenuHandle      EditMenu;
EXTERN MenuHandle      WindowMenu;
EXTERN MenuHandle      CommandMenu;
EXTERN MenuHandle      OptionsMenu;
EXTERN MenuHandle      FontMenu;
EXTERN MenuHandle      FontSizeMenu;
EXTERN MenuHandle      HelpMenu;

/*
	Most recent volume IDs
*/

EXTERN Integer         KeepVolume INIT(0); /* goes with save() */
EXTERN Integer         RestoreVolume INIT(0);
EXTERN Integer         SpoolVolume INIT(0);
EXTERN Integer         BatchVolume INIT(0);
EXTERN Integer         ReadVolume INIT(0);
EXTERN Integer         PrintVolume INIT(0);
EXTERN Integer         SaveVolume INIT(0); /* goes with Save Window */
EXTERN Integer         HelpVolume INIT(0);
EXTERN Integer         PlotVolume INIT(0);
EXTERN Integer         DynloadVolume INIT(0);
EXTERN Integer         HomeVolume INIT(0);
EXTERN LongInt         HomeDirectory INIT(0);

/*
	Global print record handle
*/
EXTERN THPrint         PrintHdl[2] INITARRAY(0 COMMA 0);
enum printTypeCodes
{
	TEXTPRINT  = 1,
	GRAPHPRINT
};

/* constants for macFindFile */
enum macFindFileCodes
{
	READIT =  1,
	WRITEIT
};

EXTERN SFReply         Reply;  /* global used by macFindFile()*/

/*
	Graph windows
*/
#define NGRAPHS		8 /* Maximum number of graph windows excluding panels*/
#define NPERPANEL   4 /* Panes per panel (some code assumes 4) */
#define NPANELS     ((NGRAPHS + NPERPANEL - 1)/NPERPANEL) /* Number of panels */

/*Global WindowPtr's*/
EXTERN WindowPtr       GraphWind[INITDIM(NGRAPHS)] INITARRAY((WindowPtr) 0 COMMA\
							(WindowPtr) 0 COMMA (WindowPtr) 0 COMMA (WindowPtr) 0);

EXTERN WindowPtr       PanelWind[INITDIM(NPANELS)] INITARRAY(0 COMMA 0);
EXTERN WindowPtr       PreviewWind INIT((WindowPtr) 0);

/*Global PicHandle's*/
EXTERN PicHandle       myPic[INITDIM(NGRAPHS)] INITARRAY(0 COMMA 0 COMMA 0 COMMA 0 COMMA 0 COMMA 0 COMMA 0 COMMA 0);
EXTERN PicHandle       PanelPic[INITDIM(NPANELS)] INITARRAY(0 COMMA 0);
EXTERN PicHandle       PreviewPic INIT((PicHandle) 0);

EXTERN unsigned char  *PanelTitle[INITDIM(NPANELS)]
#if (NPANELS == 1)
							INITARRAY("\pPanel of Graphs");
#elif (NPANELS == 2)
							INITARRAY("\pPanel of Graphs 1-4" COMMA "\pPanel of Graphs 5-8");
#else
							INITARRAY("\pPanel of Graphs 1-4" COMMA\
							          "\pPanel of Graphs 5-8" COMMA\
							          "\pPanel of Graphs 9-12");
#endif

/*State information for basic graphic windows*/
EXTERN WStateData      GraphWState[INITDIM(NGRAPHS)];
EXTERN Integer         GraphWInZoom[INITDIM(NGRAPHS)] INITARRAY(inZoomOut COMMA inZoomOut COMMA inZoomOut COMMA inZoomOut);

EXTERN Boolean         GraphWindPaused INIT(false);

/*
	The following are related to the introduction of multiple command windows
	Most of the items are just copies of the analogous globals when the window
	is in front.
*/
typedef struct cmdWindowInfo
{
	WindowPtr          cmdWind;
	TEHandle           teCmd;
	Integer            menuItem;    /* item in Windows menu */
	Integer            cmdVRefNum;  /* vRefNum for associated file */
	Boolean            cmdDirty;    /* window used flag */
	Boolean            cmdEditable; /* user can change window flag */
	ControlHandle      cmdScroll;
	Integer            cmdLine;
	Integer            cmdStrPos;
	Integer            undoStatus;
	Integer            undoPlace;
	Integer            pasteLength;
	LongInt            oldCmdStrPos;
	LongInt            lastTypingPos;
	LongInt            lastLength;
	LongInt            lastCmdStrPos;
	Handle             cmdScrap;
} cmdWindowInfo, *cmdWindowInfoPtr, **cmdWindowInfoHandle;

#ifndef CMDWINDOWLIMIT
#define CMDWINDOWLIMIT   32000  /* threshhold for window to be full */
#endif /*CMDWINDOWLIMIT*/

#ifndef CMDWINDOWNEWSIZE
#define CMDWINDOWNEWSIZE 25000  /* target size after trimming */
#endif /*CMDWINDOWNEWSIZE*/

#define MAXWINDOWS     9

EXTERN Integer         CurrentWindow INIT(-1);
EXTERN Integer         Nwindows INIT(-1);
EXTERN cmdWindowInfo   CmdWindows[INITDIM(MAXWINDOWS)];


/*
	Globals related to the Command Window - a simple text editing window
*/

EXTERN WindowPtr       CmdWind;     /* Pointer to active command window */
EXTERN TEHandle        teCmd;		/* handle to text window TextEdit record */
EXTERN Boolean         CmdDirty;    /* window changed after first prompt */
EXTERN Boolean         CmdEditable; /* is window editable */
EXTERN ControlHandle   CmdScroll;	/* command window scroll bar */
EXTERN Integer         CmdLine;	    /* line currently at top of window */
EXTERN Integer         CmdScrollLines;	/* number of lines not visible */
EXTERN Integer         HalfPage;	/* number of lines in half a window */
EXTERN Integer         CmdInsertLine;	/* line of current insertion */
EXTERN Integer         ViewLines;	/* number of lines on screen */
EXTERN LongInt         CmdStrPos;	/* position to begin looking for commands*/

/*
	Globals related to the Batch (non interactive mode) window
*/

EXTERN WindowPtr       BatchWind INIT(0);
EXTERN DialogPtr       BatchDialog INIT(0);

EXTERN Integer         Running INIT(0); /* !0 means under control of yyparse */

/*
	Globals and defines related to undoing and redoing
*/
EXTERN Integer         UndoStatus INIT(0); /* current undo status */

/* possible values for UndoStatus */
enum undoStatusCodes
{
	DELETEUNDO =    1,
	OUTPUTUNDO =    2,
	PASTEUNDO  =    3,
	CUTUNDO    =    4,
	TYPINGUNDO =    5,
	CANTUNDO   =    0,
	DELETEREDO =   -1, /* not currently used */
	OUTPUTREDO =   -2,
	PASTEREDO  =   -3, /* not currently used */
	CUTREDO    =   -4, /* not currently used */
	TYPINGREDO =   -5
};
/*
	When UndoStatus == OUTPUTUNDO, LastLength is the length of the buffer,
	less 1 for the trailing newline, just before a command line is executed,
	and LastCmdStrPos is the value of CmdStrPos at that time.  
*/
EXTERN LongInt         LastLength INIT(-1);
EXTERN LongInt         LastCmdStrPos INIT(-1);
EXTERN Integer         UndoPlace INIT(-1); /* start of change */
EXTERN Integer         PasteLength INIT(-1); /* length of paste */
EXTERN Handle          CmdScrap INIT( (Handle) 0); /* handle to save stuff*/

/* OldCmdStrPos is CmdStrPos at time of a deletion, replacement, cut, or paste*/
EXTERN LongInt         OldCmdStrPos INIT(-1);
EXTERN LongInt         LastTypingPos INIT(-1);

/*
   Macros to access pieces of TextEdit structure
*/
#define TextLength(TECMD)    ((**(TECMD)).teLength)
#define TextHandle(TECMD)    ((**(TECMD)).hText)
#define ViewRectangle(TECMD) ((**(TECMD)).viewRect)
#define DestRectangle(TECMD) ((**(TECMD)).destRect)
#define LineHeight(TECMD)    ((**(TECMD)).lineHeight)
#define NLines(TECMD)        ((**(TECMD)).nLines)
#define SelStart(TECMD)      ((**(TECMD)).selStart)
#define SelEnd(TECMD)        ((**(TECMD)).selEnd)
#define LineStarts(TECMD)    ((**(TECMD)).lineStarts)

/* Defines and globals related to fonts */
#define MACANOVAFONT     16000
#define MACANOVAFONTNAME "\pMcAOVMonaco"

#define REGULARFONT      monaco 
#define REGULARFONTNAME  "\pMonaco"

#ifndef CMDFONT
#define CMDFONT MACANOVAFONT
#endif /*CMDFONT*/

#ifndef CMDFONTSIZE
#define CMDFONTSIZE 9
#endif /*CMDFONTSIZE*/

#ifndef CMDFONTNAME
#define CMDFONTNAME MACANOVAFONTNAME
#endif /*CMDFONTNAME*/

#ifndef GRAPHFONT
#define GRAPHFONT MACANOVAFONT
#endif /*GRAPHFONT*/

#ifndef GRAPHFONTSIZE
#define GRAPHFONTSIZE 9
#endif /*GRAPHFONTSIZE*/

#ifndef GRAPHFONTNAME
#define GRAPHFONTNAME MACANOVAFONTNAME
#endif /*GRAPHFONTNAME*/

#ifndef PRINTFONT
#define PRINTFONT REGULARFONT
#endif /*PRINTFONT*/

#ifndef PRINTFONTSIZE
#define PRINTFONTSIZE 9
#endif /*PRINTFONTSIZE*/

#ifndef PRINTFONTNAME
#define PRINTFONTNAME REGULARFONTNAME
#endif /*PRINTFONTNAME*/

EXTERN Integer         CmdFont INIT(CMDFONT);
EXTERN Integer         CmdFontSize INIT(CMDFONTSIZE);
EXTERN Str27           CmdFontName INIT(CMDFONTNAME);

EXTERN Integer         GraphFont INIT(GRAPHFONT);
EXTERN Integer         GraphFontSize INIT(GRAPHFONTSIZE);
EXTERN Str27           GraphFontName INIT(GRAPHFONTNAME);

EXTERN Integer         PrintFont INIT(PRINTFONT);
EXTERN Integer         PrintFontSize INIT(PRINTFONTSIZE);
EXTERN Str27           PrintFontName INIT(PRINTFONTNAME);

/* Defines and Globals related to files */
#ifndef KEEPTYPE
#define KEEPTYPE      'S000'  /* File type for save() file */
#endif /*KEEPTYPE*/

#ifndef ASCIIKEEPTYPE
#define ASCIIKEEPTYPE 'Sasc' /*File type for asciisave() file */
#endif /*KEEPTYPE*/

#ifndef CREATOR
#define CREATOR       'mat2' /*Creator for save(), asciisave(), Cmd window files*/
#endif /*CREATOR*/

#ifndef TEXTCREATOR
#define TEXTCREATOR   'ttxt'  /* Creator for output files */
#endif /*TEXTCREATOR*/

#ifndef MACANOVAINI
#define MACANOVAINI   "MacAnova.ini"  /*Startup initialization file*/
#endif /*MACANOVAINI*/

#ifndef HELPNAME
#define HELPNAME      "MacAnova.hlp"  /*Default Help file name*/
#endif  /*HELPNAME*/

EXTERN OSType          ASCIIKEEPFILETYPE INIT(ASCIIKEEPTYPE);
EXTERN OSType          KEEPFILETYPE INIT(KEEPTYPE);
EXTERN OSType          KEEPCREATOR INIT(CREATOR);

/* Prototypes removed because they were all in macProto.h */

/* Cursors */
EXTERN Cursor          IBEAM;
EXTERN Cursor          WATCH;
EXTERN Cursor          CROSS;

/* Globals used by myDialogFilter */
EXTERN Boolean         DialogEdit INIT(false);
EXTERN Integer         NDialogButtons INIT(0);
EXTERN unsigned char   ButtonChars[4] INITARRAY(0 COMMA 0 COMMA 0 COMMA 0);
#ifdef MW_CW
	#ifdef powerc
	#define ThisISA kPowerPCISA
	#else /*MW_CW*/
	#define ThisISA kM68kISA
	#endif /*MW_CW*/
	#ifndef __MIXEDMODE__
		#include <MixedMode.h>
	#endif /* MIXEDMODE */
	EXTERN UniversalProcPtr   MyDialogFilterUPP INIT(NULL);
	EXTERN ModalFilterUPP     MyDialogFilterPtr;
	EXTERN UniversalProcPtr   AboutDialogFilterUPP INIT(NULL);
	EXTERN ModalFilterUPP     AboutDialogFilterPtr;
	EXTERN UniversalProcPtr   NullDialogHookUPP INIT(NULL);
	EXTERN DlgHookUPP         NullDialogHookPtr;
	EXTERN UniversalProcPtr   NullFileFilterUPP INIT(NULL);
	EXTERN FileFilterUPP      NullFileFilterPtr;
	EXTERN UniversalProcPtr   MyTEClickLoopUPP INIT(NULL);
	EXTERN TEClickLoopUPP     MyTEClickLoopPtr;
	EXTERN UniversalProcPtr   MyControlActionUPP INIT(NULL);
	EXTERN ControlActionUPP   MyControlActionPtr;
	EXTERN UniversalProcPtr   NullControlActionUPP INIT(NULL);
	EXTERN ControlActionUPP   NullControlActionPtr;
	EXTERN UniversalProcPtr   AboutUserItemUPP INIT( NULL);
	EXTERN UserItemProcPtr    AboutUserItemPtr;
	EXTERN UniversalProcPtr   ConsoleUserItemUPP INIT( NULL);
	EXTERN UserItemProcPtr    ConsoleUserItemPtr;
	EXTERN UniversalProcPtr   EditCommandUserItemUPP INIT( NULL);
	EXTERN UserItemProcPtr    EditCommandUserItemPtr;
	EXTERN UniversalProcPtr   OutlineOKUserItemUPP INIT( NULL);
	EXTERN UserItemProcPtr    OutlineOKUserItemPtr;
#else /* MW_CW */
	EXTERN ModalFilterProcPtr MyDialogFilterPtr;
	EXTERN ModalFilterProcPtr AboutDialogFilterPtr;
	EXTERN DlgHookProcPtr     NullDialogHookPtr;
	EXTERN FileFilterProcPtr  NullFileFilterPtr;
	EXTERN ClikLoopProcPtr    MyTEClickLoopPtr;
	EXTERN ProcPtr            MyControlActionPtr;
	EXTERN ProcPtr            NullControlActionPtr;
	EXTERN ProcPtr            AboutUserItemPtr;
	EXTERN ProcPtr            ConsoleUserItemPtr;
	EXTERN ProcPtr            EditCommandUserItemPtr;
	EXTERN ProcPtr            OutlineOKUserItemPtr;
#endif /* MW_CW */

#ifndef MACPROTOH__
#include "macProto.h"
#endif /*MACPROTOH__*/

/*
	Handles for Commands associated with Command Menu
*/

#define NCOMMANDS        8  /* number of commands in menu*/
EXTERN char          **CommandItems[INITDIM(NCOMMANDS)];

#include "macResource.h"  /* resource ID's */

/*
****defines and enum declarations only after this point****
*/

#undef LF
#undef CR
#define LF            10
#define CR            13

/* key codes */
enum macKeyCodes
{
	HomeKey =      1,
	Enter =        3,
	EndKey =       4,
	HelpKey =      5,
	Backspace =    8,
	TabKey =       9,
	Pageup =      11,
	Pagedown =    12,
	Return =      13,
	ClearKey =    27,
	EscapeKey =   27,
	LeftArrow =   28,
	RightArrow =  29,
	UpArrow =     30,
	DownArrow =   31,
	Space =       32,  /*' '*/
	Period =      46,  /*'.'*/
	Zero =        48,  /*'0'*/
	DeleteKey =  127,

	LineStart =   15,  /* MacAnova code to move to start of line */

#define Newline       CR
/*
	Standard option number key codes
*/

	Option0 =   0xbc,
	Option1 =   0xc1,
	Option2 =   0xaa,
	Option3 =   0xa3,
	Option4 =   0xa2,
	Option5 =   0xb0,
	Option6 =   0xa4,
	Option7 =   0xa6,
	Option8 =   0xa5,
	Option9 =   0xbb,

/*
	Standard lower case option key character codes
	Dead key codes are coded as the code of the character
	produced by the sequence Option letter, letter (e.g. option e, e)
*/

	OptionA =   0x8c,
	OptionB =   0xba,
	OptionC =   0x8d,
	OptionD =   0xb6,
	OptionE =   0x8e,
	OptionF =   0xc4,
	OptionG =   0xa9,
	OptionH =   0xfa,
	OptionI =   0x94,
	OptionJ =   0xc6,
	OptionK =   0xfb,
	OptionL =   0xc2,
	OptionM =   0xb5,
	OptionN =   0x96,
	OptionO =   0xbf,
	OptionP =   0xb9,
	OptionQ =   0xcf,
	OptionR =   0xa8,
	OptionS =   0xa7,
	OptionT =   0xa0,
	OptionU =   0x9f,
	OptionV =   0xc3,
	OptionW =   0xb7,
	OptionX =   0xc5,
	OptionY =   0xb4,
	OptionZ =   0xbd
};

enum macVirtualKeyCodes
{
	COMMANDKEYCODE = 0x37, /*Virtual key code for Command key */
	OPTIONKEYCODE =  0x3a, /*Virtual key code for Option key */
	SHIFTKEYCODE =   0x38, /*Virtual key code for Shift key */
	FUNCTIONKEY =    0x10, /* character code for F1 through F15 */
	NOTCOMMAND =     0xff, /*Not a virtual key code */

	SPACEKEYCODE =   0x31, /*Virtual key code for space bar */
	AKEYCODE =       0x00, /*Virtual key code for 'A' key */
	BKEYCODE =       0x0b, /*Virtual key code for 'B' key */
	CKEYCODE =       0x08, /*Virtual key code for 'C' key */
	DKEYCODE =       0x02, /*Virtual key code for 'D' key */
	EKEYCODE =       0x0e, /*Virtual key code for 'E' key */
	FKEYCODE =       0x03, /*Virtual key code for 'F' key */
	GKEYCODE =       0x05, /*Virtual key code for 'G' key */
	HKEYCODE =       0x04, /*Virtual key code for 'H' key */
	IKEYCODE =       0x22, /*Virtual key code for 'I' key */
	JKEYCODE =       0x26, /*Virtual key code for 'J' key */
	KKEYCODE =       0x28, /*Virtual key code for 'K' key */
	LKEYCODE =       0x25, /*Virtual key code for 'L' key */
	MKEYCODE =       0x2e, /*Virtual key code for 'M' key */
	NKEYCODE =       0x2d, /*Virtual key code for 'N' key */
	OKEYCODE =       0x1f, /*Virtual key code for 'O' key */
	PKEYCODE =       0x23, /*Virtual key code for 'P' key */
	QKEYCODE =       0x0c, /*Virtual key code for 'Q' key */
	RKEYCODE =       0x0f, /*Virtual key code for 'R' key */
	SKEYCODE =       0x01, /*Virtual key code for 'S' key */
	TKEYCODE =       0x11, /*Virtual key code for 'T' key */
	UKEYCODE =       0x20, /*Virtual key code for 'U' key */
	VKEYCODE =       0x09, /*Virtual key code for 'V' key */
	WKEYCODE =       0x0d, /*Virtual key code for 'W' key */
	XKEYCODE =       0x07, /*Virtual key code for 'X' key */
	YKEYCODE =       0x10, /*Virtual key code for 'Y' key */
	ZKEYCODE =       0x06, /*Virtual key code for 'Z' key */
	ZEROKEYCODE =    0x1d, /*Virtual key code for '0' key */
	ONEKEYCODE =     0x12, /*Virtual key code for '1' key */
	TWOKEYCODE =     0x13, /*Virtual key code for '2' key */
	THREEKEYCODE =   0x14, /*Virtual key code for '3' key */
	FOURKEYCODE =    0x15, /*Virtual key code for '4' key */
	FIVEKEYCODE =    0x17, /*Virtual key code for '5' key */
	SIXKEYCODE =     0x16, /*Virtual key code for '6' key */
	SEVENKEYCODE =   0x1a, /*Virtual key code for '7' key */
	EIGHTKEYCODE =   0x1c, /*Virtual key code for '8' key */
	NINEKEYCODE =    0x19, /*Virtual key code for '9' key */

/*
	The following constants are the virtual key codes for the
	function keys on the Apple Extended keyboard as pictured on page
	2-43 of IM, Macintosh Toolbox Essentials
*/

	F1 =        0x7a,
	F2 =        0x78,
	F3 =        0x63,
	F4 =        0x76,
	F5 =        0x60,
	F6 =        0x61,
	F7 =        0x62,
	F8 =        0x64,
	F9 =        0x65,
	F10 =       0x6d,
	F11 =       0x67,
	F12 =       0x6f,
	F13 =       0x69,
	F14 =       0x6b,
	F15 =       0x71,
	UNDOKEY =   F1,
	CUTKEY =    F2,
	COPYKEY =   F3,
	PASTEKEY =  F4
};


/*
	Information related to windows
*/

/* offsets for view rectangle in command windows */
enum windowOffsets
{
	TOPOFFSET    =  2,
	LEFTOFFSET   =  4,
	BOTTOMOFFSET =  2,
	RIGHTOFFSET  = 19,
/* offset of top of window is windno * STACKOFFSET */
	STACKOFFSET  = 10
};

/*
	information related to menus
*/

typedef enum
{ /* item numbers for Apple Menu */
	aboutmacanova = 1,
	helpit
}	appleItems;

typedef enum
{ /* Item numbers for File Menu */
	openit = 1,
	saveit,
	saveitas,
	/* --- */
	pagesetup = saveitas + 2,
	printit,
	/* --- */
	interrupt = printit + 2,
	go_on,
	/* --- */
	restoreit = 11,
	keepit,
	keepitas,
	/* --- */
	batchit = keepitas + 2,
	spoolit,
	/* --- */
	quit = spoolit + 2
}               fileItems;

typedef enum
{ /* item numbers for Edit Menu */
	undoit = 1,
	/* --- */
	cutit = undoit + 2,
	copyit,
	pastit,
	/* --- */
	copytoend = pastit + 2,
	copyandexecute
#ifdef SAVEHISTORY
	/* --- */
	,
	backwardhistory = copyandexecute + 2,
	forwardhistory
#endif /*SAVEHISTORY*/
}               editItems;

typedef enum
{/* item numbers for Window Menu */
	hideit = 1,
	closeit,
	newwind,
	/* --- */
	graph1 = newwind + 2,
	graph2,
	graph3,
	graph4,
#if (NGRAPHS > 4)
	graph5,
	graph6,
	graph7,
	graph8,
#endif
#if (NGRAPHS > 8)
	graph9,
	graph10,
	graph11,
	graph12,
#endif	
#if (NGRAPHS <= NPERPANEL)	
	panel1,
	/* --- */
	cmd1 = panel1 + 2,
#elif (NGRAPHS <= 2*NPERPANEL)
	panel1,
	panel2,	
	/* --- */
	cmd1 = panel2 + 2,
#else
	panel2,	
	panel3,	
	/* --- */
	cmd1 = panel3 + 2,
#endif
	cmd2,
	cmd3,
	cmd4,
	cmd5,
	cmd6,
	cmd7,
	cmd8,
	cmd9, /* must be exactly MAXWINDOWS of these */
	/* --- */
	gototop = cmd9 + 2, /* cmd1 + MAXWINDOWS + 1 */
	gotobottom,
	gotoprompt,
	pageup,
	pagedown
} windItems;

typedef enum
{/* item numbers for Command Menu */
	editcommands = 1,
	/* --- */
	command1 = editcommands + 2,
	command2,
	command3,
	command4,
	command5,
	command6,
	command7,
	command8,   /* Currently only NCOMMANDS == 8 are used */
	command9,
	command10,
	command11,
	command12,
	command13,
	command14,
	command15
}               commandItems;

typedef enum
{/* item numbers for Options Menu */
	insig = 1,
	iformats,
	iseeds,
	iangles,
	iglmoptions,
	iboptions,
	iotheroptions
}               optionsItems;

typedef enum
{/* item numbers for Help Menu */
	helpit1 = 1,
	helptopics
}               helpItems;

EXTERN Integer         UseHelpMenu    INIT(0);
EXTERN Integer         HelpItemNumber INIT(helpit);
 
/* end of globals and definitions */

#undef	INIT
#undef	INITDIM
#undef	INITARRAY
#undef  COMMA
#undef  EXTERN

#endif /*MACIFACEH__*/
