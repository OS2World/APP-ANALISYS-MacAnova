/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 4.06 or later
*(C)*
*(C)* Copyright (c) 1997 by Gary Oehlert and Christopher Bingham
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
*(C)* Most modules in this file are Copyright (c) by Paul DuBois and have
*(C)* been placed in the public domain.  The additions and changes made by
*(C)* C. Bingham for use with MPW and interfacing with MacAnova are also being
*(C)* placed in the public domain.
*(C)* Macanova related changes are bracketed by #ifdef KB ... #endif
*(C)* Purely MPW changes are bracketed by #ifdef MPW ... #endif
*(C)* In addition there have been a number number of cosmetic changes. See below
*/
#ifdef KB

#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment TransSkel
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

/*
	Summary of changes made by C. Bingham

	940428 Reformatted using indent and emacs to the style I like.  In
	particular, all non-compound statements following if(), while(), or
	else are now enclosed in {...}.
	Another cosmetic change has been to restore the use of LongInt and
	Integer instead of long and short.
*/
#define TRANSSKELC__
#include "dbug.h"
void myAlert(char * /*msgs*/);

#endif /*KB*/

/*
 * TransSkel version 3.12 - Transportable Macintosh application skeleton
 *
 * Please report problems to Paul DuBois.
 *
 * Need to test:
 * - Window dragging, growing, zooming on multiple monitor system.
 *
 * Things does not do yet:
 * - No movable modal dialog support.
 * - Use flags to implement "hide window on suspend if visible".
 *
 *
 * TransSkel is in the public domain and was originally written by:
 *
 * 			Paul DuBois
 * 			Wisconsin Regional Primate Research Center
 * 			1220 Capitol Court
 * 			Madison, WI  53715-1299  USA
 *
 * Internet:	dubois@primate.wisc.edu
 *
 * Additional changes were made by:
 *
 * 			Owen Hartnett
 * 			OHM Software Company
 * 			163 Richard Drive
 * 			Tiverton, RI 02878  USA
 *
 * Internet:	omh@cs.brown.edu
 * CSNET:		omh@cs.brown.edu.CSNET
 * UUCP:		uunet!brunix!omh
 *
 * Owen is also responsible for the port to THINK Pascal.
 *
 * Bob Schumaker joined the cast between versions 2.0 and 3.0.
 *
 * 			Bob Schumaker
 * 			The AMIX Corporation
 * 			1881 Landings Drive
 * 			Mountain View, CA 94043-0848
 *
 * Internet:	bob@markets.amix.com
 * UUCP:		{sun, uunet, netcom}!markets!bob
 * CIS:			72227,2103
 * AOL:			BSchumaker
 *
 * This version of TransSkel is written for THINK C 6.0.1.
 * THINK C is a trademark of:
 *
 * 	Symantec Corporation
 * 	10201 Torre Avenue
 * 	Cupertino, CA 95014  USA
 *
 * Reference key:
 * IM		Inside Macintosh
 * TN		Macintosh Technical Notes
 * HIN		Human Interface Notes
 * PGMF		Programmer's Guide to MultiFinder (APDA)
 * MMWZ		movable-modal-wdef-zen.txt (ftp.apple.com)
 * TPN		TransSkel Programmer's Notes
 *
 * Recent history is given below.  Earlier change history is in TSHistory.
 * If you've been writing applications with an earlier release, READ THAT FILE!
 *
 * 06 Feb 94 Release 3.10
 * - Modified entire library source so that it can be compiled with either
 * C-compatible or Pascal-compatible bindings.  This allows a binary library
 * to be compiled that can be linked into THINK Pascal applications while
 * preserving compatibility with existing TransSkel C applications.  This
 * modification was suggested by Lionel Cons, who also contributed a working
 * prototype and the major portion of the Pascal header file.
 * - Fixed cast bug in SkelEventLoop().
 * 07 Feb 94
 * - SkelApple() now takes an empty string or nil to indicate no application
 * item, rather than nil only.
 * 08 Feb 94
 * - SkelPositionWindow() positions windows using the structure rectangle
 * rather than the content rectangle now.  Give better positioning.
 *
 * 17 Feb 94 Release 3.11
 * - New convenience function SkelTestRectVisible() to check whether or not a
 * rectangle is completely contained within the desktop region.
 * - SkelAlert() uses SkelTestRectVisible() to check whether an alert will be
 * entirely visible when it's supposed to be positioned on the parent window.
 * If not, it's positioned on the parent device instead.
 * - When position type is skelPositionOnParentWind, SkelGetReferenceRect() now
 * returns the structure rectangle rather than the content rectangle.
 * - SkelPositionWindow() now accounts for difference between width of
 * structure and content rectangle of window to be positioned.
 * 20 Feb 94
 * - Pascal bindings for the interface functions are now standard.  C bindings
 * are no longer used.
 *
 * 14 Apr 94 Release 3.12
 * - Added SkelDlogTracksCursor() auxiliary function.
 */


/*
 * The following symbol controls support for (modeless) dialogs.
 * "#define supportDialogs" enables support.
 * "#undef supportDialogs" disables support.
 */

#define	supportDialogs


#include	<Traps.h>
#ifndef MW_CW_New
#include	<GestaltEqu.h>
#else /*MW_CW_new*/
#include	<Gestalt.h>
#endif /*MW_CW_new*/
#include	<EPPC.h>


/*
 * TransSkel.h contains defines, typedefs, and public function
 * prototypes
 */

#include	"TransSkel.h"
#include "dbug.h"

#ifndef	nil
#define	nil		((void *) 0)
#endif

/*
 * New(TypeName) returns handle to new object, for any TypeName.
 * If there is insufficient memory, the result is nil.
 */

#define	New(type)	(type **) NewHandle ((Size) sizeof (type))


/* -------------- */
/* Internal types */
/* -------------- */


/*
 * Private data types for window and menu handlers
 */

typedef struct WHandler WHandler, *WHPtr, **WHHandle;

struct WHandler
{
	WindowPtr                 whWind;	/* window/dialog to handle */
	SkelWindMouseProcPtr      whMouse;	/* mouse-click handler */
	SkelWindKeyProcPtr        whKey;	/* key-click handler */
	SkelWindUpdateProcPtr     whUpdate;	/* update handler */
	SkelWindActivateProcPtr   whActivate;	/* activate event handler */
	SkelWindCloseProcPtr      whClose;	/* close "event" handler */
	SkelWindClobberProcPtr    whClobber;	/* window disposal proc */
	SkelWindIdleProcPtr       whIdle;	/* main loop idle proc*/
	SkelWindZoomProcPtr       whZoom;	/* zoom proc*/
#ifdef	supportDialogs
	SkelWindEventProcPtr      whEvent;	/* event proc */
#endif
	Rect                      whGrow;	/* limits on window sizing */
	Boolean                   whSized;/* true = window was resized */
	Boolean                   whFrontOnly;	/* idle only when window active */
	Integer                   whFlags;/* various flags */
	SkelWindPropHandle        whProperties;	/* property list */
	WHHandle                  whNext;	/* next window handler */
};

typedef struct MHandler MHandler, *MHPtr, **MHHandle;

struct MHandler
{
	Integer                     mhID;	/* menu id */
	SkelMenuSelectProcPtr     mhSelect;	/* item selection handler */
	SkelMenuClobberProcPtr    mhClobber;	/* menu disposal proc */
	Boolean                   mhSubMenu;	/* whether submenu */
	MHHandle                  mhNext;	/* next menu handler */
};


/* ------------------------------------------- */
/* Prototypes for internal (private) functions */
/* ------------------------------------------- */

static WHHandle GetWDHandler(WindowPtr w);
static WHHandle GetWHandler(WindowPtr w);
static WHHandle GetDHandler(DialogPtr theDialog);
static void     DetachWDHandler(WHHandle wh);

static void     DoMenuCommand(LongInt command);
static void     DoMenuHook(void);

static void     DoMouse(WHHandle h, EventRecord * theEvent);
static void     DoKey(WHHandle h, char ch, unsigned char code, Integer mods);
static void     DoUpdate(WHHandle h);
static void     DoActivate(WHHandle h, Boolean active);
static void     DoClose(WHHandle h);
static void     DoClobber(WHHandle h);

static Boolean  DoDialog(EventRecord * theEvent);

static void     DoGrow(WHHandle h, Point startPt);
static void     DoZoom(WHHandle h, Integer partCode);


/* ------------------ */
/* Internal variables */
/* ------------------ */


/*
 * Window and menu handler variables.
 *
 * whList and mhList are the lists of window and menu handlers.
 * mhClobOnRmve is true if the menu handler disposal proc
 * is to be called when a handler is removed.  It is temporarily set
 * false when handlers are installed for menus that already
 * have handlers - the old handler is removed WITHOUT calling the
 * disposal proc.  The effect is to replace the handler for the menu
 * without destroying the menu itself.
 *
 * dragRect determines the limits on window dragging.  It is set in
 * SkelInit() to the bounding box of the desktop region inset by 4 pixels.
 *
 * growRect contains the default limits on window sizing.  It is set in
 * SkelInit().  The lower limits on window sizing of 80 pixels both directions
 * is sufficient to allow text windows room to draw a grow box and scroll
 * bars without having the thumb and arrows overlap.  The upper limits are
 * determined from the screen size. (Probably incorrectly for the case of > 1
 * screen.)
 * These default values may be changed if with SkelGrowBounds if they are
 * not appropriate.
 *
 * zoomProc is the default zoom procedure to use if the window does not have
 * one of its own.  zoomProc may be nil, in which case the default is to zoom
 * to just about full window size.
 *
 * mhDrawBarOnRmve determines whether the menu bar is redrawn by
 * SkelRmveMenu() after taking a menu out of the menu bar.  Normally
 * it's true, but SkelClobber() sets it false temporarily to avoid
 * flicker as each menu is removed.
 */


static WHHandle whList = (WHHandle) nil;
static Rect     dragRect;
static Rect     growRect;
static SkelWindZoomProcPtr zoomProc = (SkelWindZoomProcPtr) nil;


static MHHandle mhList = (MHHandle) nil;
static Boolean  mhClobOnRmve = true;
static Boolean  mhDrawBarOnRmve = true;


/*
 * Miscellaneous
 *
 * - skelEnv contains SysEnvirons() information.
 * - sysVersion contains the system software version.
 * - hasGestalt is true if Gestalt() is supported.
 * - has64KROM is true if the current machine has the 64K ROM.
 * - mBarHeight is menu bar height.  Window sizing, zooming and dragging
 * code takes this into account.  Initialized in SkelInit(), which see
 * for teeth-gnashing over such a simple thing.
 * - doneFlag determines when SkelEventLoop() returns.  It is set by calling
 * SkelStopEventLoop(), which is how the host requests a halt.
 * - pIdle points to a background procedure, to be run during event
 * processing.  Set it with SkelSetIdle().  If nil, there's no
 * procedure.
 * - pEvent points to an event-inspecting hook, to be run whenever an
 * event occurs.  Set it with SkelSetEventHook().  If nil, there's no
 * procedure.
 * - eventMask controls the event types requested by GetNextEvent() or
 * WaitNextEvent() in SkelEventLoop().
 * - pMenuHook points to a procedure called whenever a menu selection is about
 * to be executed.  nil if no hook.
 * - diskInitPt is the location at which the disk initialization dialog
 * appears, if an uninitialized disk is inserted.
 * - eventModifiers is the value of the modifiers field of the current event.
 * - eventPtr points to the current event (nil if none seen yet).
 * - defInitParams contains the default SkelInit() parameters if caller passes
 * nil.
 */

static SysEnvRec skelEnv;
static LongInt  sysVersion = 0;
static Boolean  hasGestalt;
static Boolean  has64KROM;
static Integer  mBarHeight;
static Integer  doneFlag;
static Integer  eventMask = everyEvent ^ keyUpMask;
static Integer  eventModifiers = 0;
static EventRecord *eventPtr = (EventRecord *) nil;
static Point    diskInitPt =
{ /* v = */ 120, /* h = */ 100};

static SkelIdleProcPtr pIdle = (SkelIdleProcPtr) nil;
static SkelEventHookProcPtr pEvent = (SkelEventHookProcPtr) nil;
static SkelMenuHookProcPtr pMenuHook = (SkelMenuHookProcPtr) nil;

static SkelInitParams defInitParams =
{
	6,			/* no. of times to call MoreMasters() */
	(GrowZoneProcPtr) nil,	/* GrowZone proc */
	(ResumeProcPtr) nil,	/* resume proc */
	0L			/* stack adjustment */
};

/*
 * Multitasking support stuff
 *
 * hasWNE is true if WaitNextEvent() is available.
 *
 * inForeground is true if application is running in foreground (not
 * suspended).  Initially true, per PGMF 3-1.
 *
 * getFrontClicks indicates whether the application wants to receive
 * content-area clicks that bring it to the foreground.
 *
 * fgWaitTime and bgWaitTime are WaitNextEvent() times for foreground and
 * background states.
 */

static Boolean  hasWNE;
static Boolean  inForeground = true;
static LongInt  fgWaitTime = 6L;/* 0.1 seconds */
static LongInt  bgWaitTime = 300L;	/* 5.0 seconds */
static Boolean  getFrontClicks = false;
static SkelSuspendResumeProcPtr pSuspendResume = (SkelSuspendResumeProcPtr) nil;
static SkelClipCvtProcPtr pClipCvt = (SkelClipCvtProcPtr) nil;

static WindowPtr oldWindow = (WindowPtr) nil;
static WHHandle oldWDHandler = (WHHandle) nil;

/*
 * Apple Event support
 */

static Boolean              hasAppleEvents = 0;
static SkelAEHandlerProcPtr pAEHandler = (SkelAEHandlerProcPtr) nil;


#ifdef	supportDialogs

/*
 * IsDialogEvent() returns true for any events when a dialog is
 * frontmost, but dlogEventMask specifies which events will actually
 * be passed to dialogs by TransSkel. Others are ignored.  Standard
 * mask below passes, mousedown, keydown, autokey, keyup, update,
 * activate, and null events.  Null events are controlled
 * by bit 0 (always forced on).
 */

static Integer  dlogEventMask =
activMask   |
updateMask  |
autoKeyMask |
keyDownMask |
mDownMask   |
1;
#endif


/* --------------------------- */
/* Initialization and shutdown */
/* --------------------------- */

/*
 * Initialize the various Macintosh Managers and lots of other stuff.
 *
 * FlushEvents does NOT toss disk insert events; this is so disks
 * inserted while the application is starting up don't result
 * in dead drives.
 *
 * initParams contains initialization parameters:
 * - the number of times to call MoreMasters
 * - the address of a grow zone procedure to call if memory allocation
 * problems occur (nil if none to be used)
 * - the address of a resume procedure to pass to InitDialogs()
 * (nil if none is to be used)
 * - amount to adjust the application stack size by (default 0; no adjustment)
 *
 * if initParams is nil, defaults are used.
 */

pascal void
SkelInit(SkelInitParamsPtr initParams)
{
	EventRecord     dummyEvent;
	Handle          h;
	LongInt         result;
	Integer         i;

	if (initParams == (SkelInitParams *) nil)
	{
		initParams = &defInitParams;
	}
	
	if (initParams->skelGzProc != (GrowZoneProcPtr) nil)
	{
#ifdef MW_CW
		SetGrowZone((GrowZoneUPP) initParams->skelGzProc);
#else /*MW_CW*/
		SetGrowZone(initParams->skelGzProc);
#endif /*MW_CW*/
	}
	
	SetApplLimit(GetApplLimit() - initParams->skelStackAdjust);

	MaxApplZone();

	for (i = 0; i < initParams->skelMoreMasters; i++)
	{
		MoreMasters();
	}

	FlushEvents(everyEvent - diskMask, 0);
#if defined(MPW) || defined(MW_CW)
	InitGraf(&qd.thePort);
#else /*MPW||MW_CW*/
	InitGraf (&thePort);
#endif /*MPW||MW_CW*/
	InitFonts();
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(initParams->skelResumeProc);
	InitCursor();

	(void) SysEnvirons(1, &skelEnv);

	sysVersion = (LongInt) skelEnv.systemVersion;

	has64KROM = (skelEnv.machineType == envMac || skelEnv.machineType == envXL);

	/*
	 * If 64K ROM machine, use hard-coded value of 20.  Otherwise use
	 * Script Manager routine GetMBarHeight().  (This assumes, just to be
	 * safe, that GetMBarHeight() glue doesn't return 20 on 64K ROM systems,
	 * which it very well may.  The low memory variable MBarHeight (0x0BAA)
	 * isn't used because it doesn't exist on 64K ROM machines (TN OV 4, p.7).
	 */

	mBarHeight = (has64KROM ? 20 : GetMBarHeight());

	/*
	 * Determine whether WaitNextEvent() is implemented (TN's OV 16 and TB 14,
	 * TN 158
	 */

	if (has64KROM)
	{
		hasWNE = false;
	}
	else
	{
		hasWNE = SkelTrapAvailable(_WaitNextEvent);
	}

	hasGestalt = SkelTrapAvailable(_Gestalt);
	hasAppleEvents = hasGestalt
		&& Gestalt(gestaltAppleEventsAttr, &result) == noErr
		&& (result & (1 << gestaltAppleEventsPresent));

	/*
	 * Check whether application wants to get "bring to front" clicks.
	 */

	if ((h = GetResource('SIZE', -1)) != (Handle) nil)
	{
		getFrontClicks = (((**(Integer **) h) & 0x200) != 0);
		ReleaseResource(h);
	}

	/*
	 * Window dragging limits are determined from bounding box of desktop.
	 * Upper limits of window sizing are related to that.  Both can involve
	 * multiple monitors, and should allow for menu bar.  dragRect is inset
	 * so as to leave at least 4 pixels of window title bar visible in both
	 * directions (IM I-289).
	 *
	 * GetGrayRgn() bounding box gives desktop extents.  On 64K ROM
	 * machines, GetGrayRgn() might not be present; could use GrayRgn
	 * bounding box, but use screenBits.bounds - menu bar, to avoid
	 * low memory access.  The two should be equivalent.
	 */

	if (has64KROM)
	{
#if defined(MPW) || defined(MW_CW)
		GrafPtr    screenPort;

		GetWMgrPort (&screenPort);
		dragRect = screenPort->portRect;
#else /*MPW||MW_CW*/
		dragRect = screenBits.bounds;
#endif/*MPW||MW_CW*/
		dragRect.top += mBarHeight;
	}
	else
	{
		/* GetGrayRgn () already takes menu bar into account */
		dragRect = (**GetGrayRgn()).rgnBBox;
	}

	SetRect(&growRect, 80, 80,
			dragRect.right - dragRect.left,
			dragRect.bottom - dragRect.top);

	InsetRect(&dragRect, 4, 4);

/* let application come to front in multitasking environment, TN TB 35, p.8 */

	(void) EventAvail(everyEvent, &dummyEvent);
	(void) EventAvail(everyEvent, &dummyEvent);
	(void) EventAvail(everyEvent, &dummyEvent);
	(void) EventAvail(everyEvent, &dummyEvent);
} /*SkelInit()*/


/*
 * Copy the default initialization parameters into the structure
 * pointed to by initParams.
 */

pascal void
SkelGetInitParams(SkelInitParamsPtr initParams)
{
	*initParams = defInitParams;
} /*SkelGetInitParams()*/


/*
 * Clobber all the menu, window and dialog handlers.  Tell SkelRmveMenu()
 * not to redraw menu bar so it doesn't flicker as menus are removed,
 * then redraw it manually.
 *
 * Before removing window handlers, hide all the windows.  Do this from
 * back to front (more esthetic and speedier).  If a window belongs to a DA,
 * close the DA.  (For early systems (e.g., 4.1), if you leave a DA open,
 * the system crashes the next time you try to open that DA.)
 */

pascal void
SkelCleanup(void)
{
	Boolean         oldFlag;
	Integer         theKind;
	WindowPeek      w;
	WindowPtr       lastVis;

	for (;;)
	{
		lastVis = (WindowPtr) nil;
		for (w = (WindowPeek) FrontWindow(); w != (WindowPeek) nil;
			 w = w->nextWindow)
		{
			if (w->visible)
			{
				lastVis = (WindowPtr) w;
			}
		}
		if (lastVis == (WindowPtr) nil)	/* no more visible windows */
		{
			break;
		}
		if (lastVis != (WindowPtr) nil)
		{
			theKind = ((WindowPeek) lastVis)->windowKind;
			if (theKind < 0)	/* DA, close it */
			{
				CloseDeskAcc(theKind);
			}
			else
			{
				HideWindow(lastVis);
			}
		}
	}

	while (whList != (WHHandle) nil)
	{
		SkelRmveWind((**whList).whWind);
	}

	oldFlag = mhDrawBarOnRmve;
	mhDrawBarOnRmve = false;
	while (mhList != (MHHandle) nil)
	{
		SkelRmveMenu(GetMHandle((**mhList).mhID));
	}
	mhDrawBarOnRmve = oldFlag;
	DrawMenuBar();
} /*SkelCleanup()*/


/* ----------------------------------- */
/* Execution environment interrogation */
/* ----------------------------------- */



#define trapMask	0x0800

static Integer
NumToolboxTraps(void)
{
	if (NGetTrapAddress(_InitGraf, ToolTrap)
		== NGetTrapAddress(0xaa6e, ToolTrap))
	{
		return (0x200);
	}
	return (0x400);
} /*NumToolboxTraps()*/


static TrapType
GetTrapType(Integer theTrap)
{
	return ((theTrap & trapMask) ? ToolTrap : OSTrap);
} /*GetTrapType()*/


pascal Boolean
SkelTrapAvailable(Integer theTrap)
{
	TrapType        tType;

	if ((tType = GetTrapType(theTrap)) == ToolTrap)
	{
		theTrap &= 0x07ff;
		if (theTrap >= NumToolboxTraps())
		{
			theTrap = _Unimplemented;
		}
	}
	return (NGetTrapAddress(theTrap, tType)
		!= NGetTrapAddress(_Unimplemented, ToolTrap));
} /*SkelTrapAvailable()*/


/*
 * Query the TransSkel execution environment.  Shouldn't be called until
 * after SkelInit() has been called.  Result is undefined if selector isn't
 * legal.
 */

pascal LongInt
SkelQuery(Integer selector)
{
	LongInt         result;
	Rect            r;
	RgnHandle       rgn;

	switch (selector)
	{
	  case skelQVersion:
		result = ((LongInt) skelMajorRelease << 16) | skelMinorRelease;
		break;
	  case skelQSysVersion:
		result = sysVersion;
		break;
	  case skelQHasWNE:
		result = hasWNE ? 1 : 0;
		break;
	  case skelQHas64KROM:
		result = has64KROM ? 1 : 0;
		break;
	  case skelQMBarHeight:
		result = mBarHeight;
		break;
	  case skelQHasColorQD:
		result = skelEnv.hasColorQD ? 1 : 0;
		break;
	  case skelQQDVersion:
		/* get QuickDraw version number */
		if (!hasGestalt
			|| Gestalt(gestaltQuickdrawVersion, &result) != noErr)
		{
			result = 0;			/* assume original QuickDraw */
		}
		break;
	  case skelQInForeground:
		result = inForeground ? 1 : 0;
		break;
	  case skelQHasGestalt:
		result = hasGestalt ? 1 : 0;
		break;
	  case skelQHasAppleEvents:
		result = hasAppleEvents ? 1 : 0;
		break;
	  case skelQGrayRgn:
		rgn = NewRgn();
		if (rgn != (RgnHandle) nil)
		{
			if (has64KROM)
			{
#if defined(MPW) || defined(MW_CW)
				GrafPtr    screenPort;

				GetWMgrPort (&screenPort);
				r = screenPort->portRect;
#else /*MPW||MW_CW*/
				r = screenBits.bounds;
#endif /*MPW||MW_CW*/
				r.top += mBarHeight;
				RectRgn(rgn, &r);
			}
			else
			{
				/* GetGrayRgn () already takes menu bar into account */
				CopyRgn(GetGrayRgn(), rgn);
			}
		}
		result = (LongInt) rgn;
		break;
	  default:
		/* result is undefined! */
		break;
	}
	return (result);
} /*SkelQuery()*/

/* ------------------------------------- */
/* Event loop initiation and termination */
/* ------------------------------------- */

#ifdef UNDEFINED__
#ifdef KB
/*
	SkelChngWind allows you to change items in the handler structure.

	If theWind is nil or is not in the list of Windows, then nothing is done
	Otherwise, non-nil procs in the argument list are put in the window
	handler structure, as is the value of idleFrontOnly.

	Note this does not allow you to change a proc pointer to nil.  Use
	SkelWindow for that.

	Added by kb, 940422
*/

pascal void SkelChngWind (WindowPtr theWind,
						  SkelWindMouseProcPtr doMouse,
						  SkelWindKeyProcPtr doKey,
						  SkelWindUpdateProcPtr doUpdate,
						  SkelWindActivateProcPtr doActivate,
						  SkelWindCloseProcPtr doClose,
						  SkelWindClobberProcPtr doClobber,
						  SkelWindIdleProcPtr doWIdle,
						  Boolean idleFrontOnly)
{
	register WHandler	**hHand, *hPtr;
	WHERE("SkelChngWind");

	if(theWind == (WindowPtr) 0 ||
	   (hHand = GetWHandler (theWind)) == (WHandler **) 0)
	{ /* non-existent or not our window */
		return;
	}
	

/*
	Fill in handler fields
*/
	HLock((Handle) hHand);
	hPtr = *hHand;
	if(doMouse != nil)
	{
		hPtr->whMouse = doMouse;
	}
	if(doKey != nil)
	{
		hPtr->whKey = doKey;
	}
	if(doUpdate != nil)
	{
		hPtr->whUpdate = doUpdate;
	}
	if(doActivate != nil)
	{
		hPtr->whActivate = doActivate;
	}
	if(doClose != nil)
	{
		hPtr->whClose = doClose;
	}
	if(doClobber != nil)
	{
		hPtr->whClobber = doClobber;
	}
	if(doIdle != nil)
	{
		hPtr->whIdle = doWIdle;
	}
	hPtr->whFrontOnly = idleFrontOnly;
	HUnlock((Handle) hHand);
} /*SkelChngWind()*/

#endif /*KB*/
#endif /*UNDEFINED__*/

/*
 * Main event loop.
 *
 * - Take care of DA's with SystemTask() if necessary.
 * - Get an event.
 * - Pass event to event router.
 * - If no event, call the "no-event" handler for the front window and for
 * any other windows with idle procedures that are always supposed
 * to run.  This is done in such a way that it is safe for idle procs
 * to remove the window handler for their own window if they want
 * (unlikely, but...)  This loop doesn't check whether the window is
 * really a dialog window or not, but it doesn't have to, because such
 * things always have a nil idle proc.
 *
 * doneFlag is restored to its previous value upon exit.  This allows
 * SkelEventLoop() to be called recursively.
 */

pascal void
SkelEventLoop(void)
{
	EventRecord     theEvent;
	WHHandle        wh, wh2;
	WindowPtr       w;
	Boolean         haveEvent;
	GrafPtr         tmpPort;
	Boolean         oldDoneFlag;
	LongInt         waitTime;
	SkelWindIdleProcPtr p;

	oldDoneFlag = doneFlag;	/* save in case this is a recursive call */
	doneFlag = false;	/* set for this call */
	while (!doneFlag)
	{
	/*
	 * Cannot assume the event contains a null event if the GetNextEvent()
	 * return value is false.  GetNextEvent() calls SystemEvent() to handle
	 * some DA events, and returns false if the event was handled.  However,
	 * in such cases the event record may still have the event that occurred,
	 * *not* a null event.  To avoid problems later with misinterpreting the
	 * event as non-null, force it to look like a null event.
	 */

		if (hasWNE)
		{
			waitTime = (inForeground ? fgWaitTime : bgWaitTime);
			haveEvent = WaitNextEvent(eventMask, &theEvent, waitTime, nil);
		}
		else
		{
			SystemTask();
			haveEvent = GetNextEvent(eventMask, &theEvent);
		}
		if (!haveEvent)
		{
			theEvent.what = nullEvent;
		}

		SkelRouteEvent(&theEvent);

		/*
		 * Run applicable window idle procs.  Make sure to save and restore
		 * the port, since idle procs for the non-active window may be run.
		 * None of them run when the application is suspended.
		 */

		if (inForeground && !haveEvent)
		{
			GetPort(&tmpPort);
			for (wh = whList; wh != (WHHandle) nil; wh = wh2)
			{
				wh2 = (**wh).whNext;
				w = (**wh).whWind;
				if (w == FrontWindow() || !(**wh).whFrontOnly)
				{
					if ((p = (**wh).whIdle) != (SkelWindIdleProcPtr) nil)
					{
						if (!hasWNE)
						{
							SystemTask();
						}
						SetPort(w);
						(*p) ();
					}
				}
			}
			SetPort(tmpPort);
		} /*if (inForeground && !haveEvent)*/
	} /*while (!doneFlag)*/
	doneFlag = oldDoneFlag;	/* restore in case this was a recursive call */
} /*SkelEventLoop()*/


/*
 * Tell current instance of SkelEventLoop() to drop dead
 */

pascal void
SkelStopEventLoop(void)
{
	doneFlag = true;
} /*SkelStopEventLoop()*/


/* ----------------- */
/* Event dispatching */
/* ----------------- */


/*
 * General event dispatch routine.
 *
 * If there is an event-handling hook and it handles the event, the
 * event is not further processed here.  Otherwise, run the application's idle
 * time process if the event is a null event, then process the event.
 *
 * Null events are sent through the dialog event handler.  This is
 * necessary to make sure, if dialogs are supported, that DialogSelect()
 * gets called repeatedly, or the caret won't blink if a dialog has any
 * editText items.
 *
 * Network events are not supported as per the deprecation in TN NW 07.
 * Application-defined events 1, 2 and 3 are not handled, either.
 */

pascal void
SkelRouteEvent(EventRecord * theEvent)
{
	Point           evtPt;
	GrafPtr         evtPort;
	Integer         evtPart;
	Integer         evtMods;
	char            evtChar;
	LongInt         evtMsge;
	unsigned char   evtCode;
	WindowPtr       frontWind = FrontWindow();
	WHHandle        wh;
	Integer         osMsge;
	Boolean         osResume;
	Boolean         osClipCvt;
	Rect            r1, r2;
	WStateData    **wdh;
	SignedByte      state;

	/* save values for SkelGetCurrentEvent() and SkelGetModifiers() */

	eventPtr = theEvent;
	eventModifiers = theEvent->modifiers;

	/* don't bother handling below if event hook does so here */

	if (pEvent != (SkelEventHookProcPtr) nil && (*pEvent) (theEvent))
	{
		return;
	}

	/*
	 * Give idle proc a chance at null events since otherwise DoDialog()
	 * will "handle" them if there's a dialog in front, and the idle
	 * proc will never see them.
	 */

	if (theEvent->what == nullEvent && pIdle != (SkelIdleProcPtr) nil)
	{
		(*pIdle) ();
	}

#ifdef	supportDialogs

	if (DoDialog(theEvent))
	{
		return;
	}

#endif

	evtPt = theEvent->where;
	evtMods = theEvent->modifiers;
	evtMsge = theEvent->message;

	switch (theEvent->what)
	{
	  case nullEvent:
		/* do nothing */
		break;

		/*
		 * Mouse click.  Get the window that the click occurred in, and
		 * the part of the window.  GetWDHandler() is called here, not
		 * GetWHandler(), since we need the handler for a window which
		 * might turn out to be a dialog window, e.g., if the click is in
		 * a close box.
		 */
	  case mouseDown:
		evtPart = FindWindow(evtPt, &evtPort);
		wh = GetWDHandler(evtPort);

		switch (evtPart)
		{
			/*
			 * Click in desk accessory window.  Pass back to the system.
			 */
		  case inSysWindow:
			SystemClick(theEvent, evtPort);
			break;

			/*
			 * Click in menu bar.  Track the mouse and execute
			 * selected command, if any.
			 */
		  case inMenuBar:
			DoMenuHook();
			DoMenuCommand(MenuSelect(evtPt));
			break;

			/*
			 * Click in grow box.  Resize window.
			 */
		  case inGrow:
			DoGrow(wh, evtPt);
			break;

			/*
			 * Click in title bar.  Drag the window around.
			 * Problem fix:  DragWindow() seems to call StillDown()
			 * first, so that clicks in drag regions while machine is
			 * busy don't otherwise bring window to front if the mouse
			 * is already up by the time DragWindow() is called.  So the
			 * window is selected first to make sure it's at least
			 * activated (unless the command key is down, IM I-289).
			 *
			 * Also offset the window's userState by the amount of the drag
			 * (it'd be simpler to set it to the final content rect but the
			 * window might be in zoomed state rather than user state).
			 */
		  case inDrag:
			if (evtPort != frontWind && (evtMods & cmdKey) == 0)
			{
				SelectWindow(evtPort);
			}
			SkelGetWindContentRect(evtPort, &r1); /* post-drag position */
			DragWindow(evtPort, evtPt, &dragRect);
			SkelGetWindContentRect(evtPort, &r2); /* post-drag position */
			wdh = (WStateData **) (((WindowPeek) evtPort)->dataHandle);
			state = HGetState((Handle) wdh);
			HLock((Handle) wdh);
			OffsetRect(&(**wdh).userState, r2.left - r1.left, r2.top - r1.top);
			HSetState((Handle) wdh, state);
			break;

			/*
			 * Click in close box.  Call the close proc if the window
			 * has one.
			 */
		  case inGoAway:
			if (TrackGoAway(evtPort, evtPt))
			{
				DoClose(wh);
			}
			break;

			/*
			 * Click in zoom box.  Track the click and then zoom the
			 * window if necessary.
			 */
		  case inZoomIn:
		  case inZoomOut:
			if (TrackBox(evtPort, evtPt, evtPart))
			{
				DoZoom(wh, evtPart);
			}
			break;

			/*
			 * Click in content region.  If the window wasn't frontmost
			 * (active), just select it, otherwise pass the click to the
			 * window's mouse click handler.  Exception: if the application
			 * wants to receive content clicks event in non-frontmost windows,
			 * select the window and "repeat" the click.
			 */
		  case inContent:
			if (evtPort != frontWind)
			{
				SelectWindow(evtPort);
				if (getFrontClicks)
				{
					SetPort(evtPort);
					DoMouse(wh, theEvent);
				}
			}
			else
			{
				DoMouse(wh, theEvent);
			}
			break;

		}
		break;					/* mouseDown */

		/*
		 * Key down event.  If the command key was down, process as menu
		 * item selection, otherwise pass the character and the modifiers
		 * flags to the active window's key handler.
		 *
		 * If dialogs are supported, there's no check for command-key
		 * equivalents, since that would have been checked in DoDialog.
		 */
	  case keyDown:
	  case autoKey:
		evtChar = evtMsge & charCodeMask;
		evtCode = (evtMsge & keyCodeMask) >> 8;	/* hope bit 7 isn't set! */

#ifndef	supportDialogs

		if (evtMods & cmdKey)	/* try menu equivalent */
		{
			DoMenuHook();
			DoMenuCommand(MenuKey(evtChar));
			break;
		}

#endif

		DoKey(GetWHandler(frontWind), evtChar, evtCode, evtMods);
		break;

		/*
		 * Key up event.  Key-ups are signified by setting the high bit
		 * of the key code.  This never executes unless application
		 * changes system event mask *and* TransSkel event mask.
		 */
	  case keyUp:
		evtChar = evtMsge & charCodeMask; /* probably 0? */
		evtCode = ((evtMsge & keyCodeMask) >> 8) | 0x80;
		DoKey(GetWHandler(frontWind), evtChar, evtCode, evtMods);
		break;

		/*
		 * Update a window.
		 */
	  case updateEvt:
		DoUpdate(GetWHandler((WindowPtr) evtMsge));
		break;

		/*
		 * Activate or deactivate a window.
		 */
	  case activateEvt:
		DoActivate(GetWHandler((WindowPtr) evtMsge),
				   (Boolean) ((evtMods & activeFlag) != 0));
		break;

		/*
		 * handle inserts of uninitialized disks.  Deactivate the frontmost
		 * window since the disk-init dialog doesn't do anything with
		 * activate events for other windows.
		 */
	  case diskEvt:
		if (HiWord(evtMsge) != noErr)
		{
			SkelActivate(FrontWindow(), false);
			DILoad();
			(void) DIBadMount(diskInitPt, evtMsge);
			DIUnload();
		}
		break;

	  case osEvt:				/* aka app4Evt aka MultiFinder event */
		/* rip the message field into constituent parts */
		osMsge = ((evtMsge >> 24) & 0xff); /* high byte */
		osResume = (Boolean) ((evtMsge & resumeFlag) != 0);
		osClipCvt = (Boolean) ((evtMsge & convertClipboardFlag) != 0);

		switch (osMsge)
		{
		  case suspendResumeMessage:
			/*
			 * Tell application it's being suspended or resumed
			 * Tell application to convert scrap if necessary
			 */

			inForeground = osResume;
			if (pSuspendResume != (SkelSuspendResumeProcPtr) nil)
			{
				(*pSuspendResume) (inForeground);
			}
			if (!osResume)		/* always convert on suspend */
			{
				osClipCvt = true;
			}
			if (osClipCvt && pClipCvt != (SkelClipCvtProcPtr) nil)
			{
				(*pClipCvt) (inForeground);
			}
			break;

		  case mouseMovedMessage:
			/* recompute mouse region -- not implemented */
			break;

			/*
			 * 0xfd is a child-died event -- not implemented here since it's
			 * only had limited use, e.g., by certain debuggers.  The child pid
			 * is byte 2 ((evtMsge >> 16) & 0xff)
			 case 0xfd:
			 break;
			 */

		  default:				/* other event */
			/* pass event to catch-all handler */
			break;
		}
		break;

	  case kHighLevelEvent:
		if (pAEHandler != (SkelAEHandlerProcPtr) nil)
		{
			(*pAEHandler) (theEvent);
		}
		break;
	}
} /*SkelRouteEvent()*/


/*
 * Activate or deactivate a window by synthesizing a fake
 * activate event and putting it through the event router.
 * Useful for activating a window when you don't know its
 * activate function.
 */

pascal void
SkelActivate(WindowPtr w, Boolean active)
{
	EventRecord     evt;

	if (w != (WindowPtr) nil)
	{
		evt.what = activateEvt;
		evt.modifiers = active ? activeFlag : 0;
		evt.when = TickCount();
		SetPt(&evt.where, 0, 0);
		evt.message = (LongInt) w;
		SkelRouteEvent(&evt);
	}
} /*SkelActivate()*/


/*
 * Call a window's close procedure.  Useful for closing a window when you
 * don't know its close function.
 *
 * Also knows how to close Desk Accessories.
 */

pascal void
SkelClose(WindowPtr w)
{
	if (w != (WindowPtr) nil)
	{
		if (((WindowPeek) w)->windowKind < 0)	/* DA window */
		{
			CloseDeskAcc(((WindowPeek) w)->windowKind);
		}
		else
		{
			DoClose(GetWDHandler(w));
		}
	}
} /*SkelClose()*/




#ifdef	supportDialogs

/*
 * Dialog event handler
 *
 * Handle event if it's for a (modeless) dialog.  The event must be one
 * of those that is passed to dialogs according to dlogEventMask.  Other
 * events are simply passed back to the caller for it to process.  An
 * exception is that command keys are handled here no matter what
 * the mask says.
 *
 * In accordance with the TransSkel port-setting model, the port is
 * set to any dialog window coming active.  (For non-dialog windows, this
 * happens in DoActivate(); for dialog windows, there's no such thing, so
 * it's done here.)
 */
static Boolean
DoDialog(EventRecord * theEvent)
{
	WHHandle        dh;
	DialogPtr       theDialog;
	Integer         what;
	Integer         item;
	GrafPtr         tmpPort;
	WindowPeek      w;
	/*
	 * handle command keys before they get to IsDialogEvent
	 */

	what = theEvent->what;
	if ((what == keyDown || what == autoKey) && (theEvent->modifiers & cmdKey))
	{
#ifdef KB
/*
	Modified so that it does not swallow non-menu Command Key combinations
	If it is a dialog event, the handler is called with itemHit == 0
*/
		LongInt     command;
		
		DoMenuHook();
		command = MenuKey(theEvent->message & charCodeMask);

		if(HiWord (command))
		{
			DoMenuCommand(command);
			return (true);
		}
		else /*if(HiWord (command))*/
		{
			if(((1 << what) & dlogEventMask)
			   && FrontWindow() != (WindowPtr) nil
			   && IsDialogEvent(theEvent))
			{
				(void) DialogSelect(theEvent, &theDialog, &item);
				dh = GetDHandler(theDialog);
				if(dh != (WHHandle) nil &&
				   (**dh).whEvent != (SkelWindEventProcPtr) nil)
				{
					(*(**dh).whEvent) (0, theEvent);
				}
				return (true);
			}
			return (false);
		} /*if(HiWord (command)){}else{}*/
#else /*KB*/
		DoMenuHook();
		DoMenuCommand(MenuKey(theEvent->message & charCodeMask));
		return (true);
#endif /*KB*/
	} /*if is command key combination */

	/*
	 * Look at the event if it's one we're interested in, and there is a front
	 * window (IsDialogEvent() has a bug causing it to fail if FrontWindow()
	 * = nil), and it's a dialog event.
	 */
	if (((1 << what) & dlogEventMask)
		&& FrontWindow() != (WindowPtr) nil
		&& IsDialogEvent(theEvent))
	{
		/*
		 * *** ugly programming award semi-finalist follows ***
		 *
		 * If the event is an activate (not deactivate) for a dialog
		 * window, set the current port to it.
		 */
		if (theEvent->what == activateEvt
			&& (theEvent->modifiers & activeFlag)
			&& (w = (WindowPeek) theEvent->message)->windowKind
			== dialogKind)
		{
			SetPort((GrafPtr) w);
		}
		/*
		 * If the event is for a dialog for which the handler can be found
		 * and the handler has an event procedure, pass the event to it.
		 * (If not, it's tossed; tsk, tsk.)
		 */
		if (DialogSelect(theEvent, &theDialog, &item)
			&& (dh = GetDHandler(theDialog)) != (WHHandle) nil
			&& (**dh).whEvent != (SkelWindEventProcPtr) nil)
		{
			(*(**dh).whEvent) (item, theEvent);
		}
		return (true);
	}
	return (false);
} /*DoDialog()*/

#endif


/*
 * Set the TransSkel event mask.  Does not have anything to do with the
 * system event mask.
 */

pascal void
SkelSetEventMask(Integer mask)
{
	eventMask = mask;
} /*SkelSetEventMask()*/


/*
 * Return the event mask.
 */

pascal Integer
SkelGetEventMask(void)
{
	return (eventMask);
} /*SkelGetEventMask()*/


#ifdef	supportDialogs

/*
 * Set the mask for event types that will be passed to dialogs.
 * Bit 1 is always set, so that null events will be examined.
 * (If this is not done, the caret does not blink in editText items.)
 */

pascal void
SkelSetDlogMask(Integer mask)
{
	dlogEventMask = mask | 1;
} /*SkelSetDlogMask()*/


/*
 * Return the current dialog event mask.
 */

pascal Integer
SkelGetDlogMask(void)
{
	return (dlogEventMask);
} /*SkelGetDlogMask()*/

#endif


/*
 * Install an idle-time task.  If p is nil, the current task is
 * disabled.
 */

pascal void
SkelSetIdle(SkelIdleProcPtr p)
{
	pIdle = p;
} /*SkelSetIdle()*/


/*
 * Return the current idle-time task.  Return nil if none.
 */

pascal SkelIdleProcPtr
SkelGetIdle(void)
{
	return (pIdle);
} /*SkelGetIdle()*/


/*
 * Install an event-inspecting hook.  If p is nil, the hook is
 * disabled.
 */

pascal void
SkelSetEventHook(SkelEventHookProcPtr p)
{
	pEvent = p;
} /*SkelSetEventHook()*/


/*
 * Return the current event-inspecting hook.  Return nil if none.
 */

pascal SkelEventHookProcPtr
SkelGetEventHook(void)
{
	return (pEvent);
} /*SkelGetEventHook()*/


pascal void
SkelSetSuspendResume(SkelSuspendResumeProcPtr p)
{
	pSuspendResume = p;
} /*SkelSetSuspendResume()*/


pascal SkelSuspendResumeProcPtr
SkelGetSuspendResume(void)
{
	return (pSuspendResume);
} /*SkelGetSuspendResume()*/


pascal void
SkelSetClipCvt(SkelClipCvtProcPtr p)
{
	pClipCvt = p;
} /*SkelSetClipCvt()*/


pascal SkelClipCvtProcPtr
SkelGetClipCvt(void)
{
	return (pClipCvt);
} /*SkelGetClipCvt()*/


pascal void
SkelSetWaitTimes(LongInt fgTime, LongInt bgTime)
{
	fgWaitTime = fgTime;
	bgWaitTime = bgTime;
} /*SkelSetWaitTimes()*/


pascal void
SkelGetWaitTimes(LongInt *pFgTime, LongInt *pBgTime)
{
	if (pFgTime != (LongInt) nil)
	{
		*pFgTime = fgWaitTime;
	}
	if (pBgTime != (LongInt) nil)
	{
		*pBgTime = bgWaitTime;
	}
} /*SkelGetWaitTimes()*/


pascal EventRecord *
SkelGetCurrentEvent(void)
{
	return (eventPtr);
} /*SkelGetCurrentEvent()*/


pascal Integer
SkelGetModifiers(void)
{
	return (eventModifiers);
} /*SkelGetModifiers()*/


pascal void
SkelSetAEHandler(SkelAEHandlerProcPtr p)
{
	pAEHandler = p;
} /*SkelSetAEHandler()*/


pascal SkelAEHandlerProcPtr
SkelGetAEHandler(void)
{
	return (pAEHandler);
} /*SkelGetAEHandler()*/


/* -------------------------------------------------------------------- */
/*					Window-handler event routing routines				*/
/*																		*/
/*	See manual for discussion of port-setting behavior: the current		*/
/*	port is set to a window when it becomes active.						*/
/*	This is done in DoActivate for non-dialog windows, in DoDialog		*/
/*	for dialog windows.													*/
/* -------------------------------------------------------------------- */


/*
 * Pass local mouse coordinates, click time, and the modifiers flag
 * word to the handler.  Should not be necessary to set the port, as
 * the click is passed to the active window's handler.
 */

static void
DoMouse(WHHandle h, EventRecord * theEvent)
{
	Point           thePt;

	if (h != (WHHandle) nil && (**h).whMouse != (SkelWindMouseProcPtr) nil)
	{
		thePt = theEvent->where;	/* make local copy */
		GlobalToLocal(&thePt);
		(*(**h).whMouse) (thePt, theEvent->when, theEvent->modifiers);
	}
} /*DoMouse()*/


/*
 * Pass the character code, key code and the modifiers flag word to
 * the handler. Should not be necessary to set the port, as the click
 * is passed to the active window's handler.
 */

static void
DoKey(WHHandle h, char c, unsigned char code, Integer mods)
{
	if (h != (WHHandle) nil && (**h).whKey != (SkelWindKeyProcPtr) nil)
	{
		(*(**h).whKey) ((Integer) c, (Integer) code, mods);
	}
} /*DoKey()*/


/*
 * Call the window updating procedure, passing to it an indicator whether
 * the window has been resized or not.  Then clear the flag, assuming
 * the update proc took whatever action was necessary to respond to
 * resizing.
 *
 * The Begin/EndUpdate stuff is done to clear the update region even if
 * the handler doesn't have any update proc.  Otherwise the Window
 * Manager will keep generating update events for the window, stalling
 * updates of other windows.
 *
 * Make sure to save and restore the port, as it's not always the
 * active window that is updated.
 */

static void
DoUpdate(WHHandle h)
{
	GrafPtr         updPort;
	GrafPtr         tmpPort;

	if (h != (WHHandle) nil)
	{
		GetPort(&tmpPort);
		SetPort(updPort = (**h).whWind);
		BeginUpdate(updPort);
		if ((**h).whUpdate != (SkelWindUpdateProcPtr) nil)
		{
			(*(**h).whUpdate) ((**h).whSized);
			(**h).whSized = false;
		}
		EndUpdate(updPort);
		SetPort(tmpPort);
	}
} /*DoUpdate()*/


/*
 * Pass activate/deactivate notification to handler.  On activate,
 * set the port to the window coming active.  Normally this is done by
 * the user clicking in a window.
 *
 * *** BUT ***
 * Under certain conditions, a deactivate may be generated for a window
 * that has not had the port set to it by a preceding activate.  If an
 * application puts up window A, then window B in front of A, then
 * starts processing events, the first events will be a deactivate for A
 * and an activate for B.  Since it therefore can't be assumed the port
 * was set to A by an activate, the port needs to be set for deactivates
 * as well.
 *
 * This matters not a whit for the more usual cases that occur.  If a
 * deactivate for one window is followed by an activate for another, the
 * port will still be switched properly to the newly active window.  If
 * no activate follows the deactivate, the deactivated window is the last
 * one, and it doesn't matter what the port ends up set to, anyway.
 *
 * On deactivate, port is saved and restored in case deactivate is due to
 * dialog having been brought in front and port changed to it explicitly
 * by application.  The deactivate shouldn't leave the port changed!
 */

static void
DoActivate(WHHandle h, Boolean active)
{
	GrafPtr         tmpPort;

	if (h != (WHHandle) nil)
	{
		GetPort(&tmpPort);	/* save so can restore if deactivate */
		SetPort((**h).whWind);
		if ((**h).whActivate != (SkelWindActivateProcPtr) nil)
		{
			(*(**h).whActivate) (active);
		}
		if (!active)
		{
			SetPort(tmpPort);
		}
	}
} /*DoActivate()*/


/*
 * Execute a window handler's close box proc.  The close proc for
 * handlers for temp windows that want to remove themselves when the
 * window is closed can call SkelRmveWind to dispose of the window
 * and remove the handler from the window handler list.  Thus, windows
 * may be dynamically created and destroyed without filling up the
 * handler list with a bunch of invalid handlers.
 *
 * If the handler doesn't have a close proc, just hide the window.
 * The host should provide some way of reopening the window (perhaps
 * a menu selection).  Otherwise the window will be lost from user
 * control if it is hidden, since it won't receive user-initiated
 * events.
 *
 * This is called both for regular and dialog windows.
 *
 * Normally this is invoked because the close box of the active window
 * is clicked, in which case the port will be set to the window.  However,
 * SkelClose() allows application to close an aritrary window, not just
 * the frontmost one -- so the port is saved and restored.
  */

static void
DoClose(WHHandle h)
{
	GrafPtr         tmpPort;

	if (h != (WHHandle) nil)
	{
		GetPort(&tmpPort);
		SetPort((**h).whWind);
		if ((**h).whClose != (SkelWindCloseProcPtr) nil)
		{
			(*(**h).whClose) ();
		}
		else
		{
			HideWindow((**h).whWind);
		}
		SetPort(tmpPort);
	}
} /*DoClose()*/


/*
 * Execute a window handler's clobber proc.  This is called both
 * for regular and dialog windows.
 *
 * Must save, set and restore port, since any window (not just active
 * one) may be clobbered at any time.
 *
 * Don't need to check whether handler is nil, as in other handler
 * procedures, since this is only called by SkelRmveWind with a
 * known-valid handler.
 */

static void
DoClobber(WHHandle h)
{
	GrafPtr         tmpPort;

	GetPort(&tmpPort);
	SetPort((**h).whWind);
	if ((**h).whClobber != (SkelWindClobberProcPtr) nil)
	{
		(*(**h).whClobber) ();
	}
	SetPort(tmpPort);
} /*DoClobber()*/


/*
 * Handlers for window events not requiring application handler routines
 * to be called.
 */


/*
 * Have either zoomed a window or sized it manually.  Invalidate
 * it to force an update and set the 'resized' flag in the window
 * handler true.  The port is assumed to be set to the port that changed
 * size.  Handler is assumed non-nil.
 */

static void
TriggerUpdate(WHHandle h)
{
	GrafPtr         port = (**h).whWind;

	InvalRect(&port->portRect);
	(**h).whSized = true;
} /*TriggerUpdate()*/


/*
 * Size a window, using the grow limits in the handler record.
 *
 * The portRect is invalidated to force an update event.  The window's
 * update handler procedure should check the parameter passed to it to
 * check whether the window has changed size, if it needs to adjust
 * itself to the new size.  THIS IS A CONVENTION.  Update procs must
 * notice grow "events", there is no procedure specifically for that.
 *
 * The clipping rectangle is not reset.  If the host application
 * keeps the clipping set equal to the portRect or something similar,
 * then it will have to arrange to treat window growing with more
 * care.
 *
 * Since the grow region of only the active window may be clicked,
 * it should not be necessary to set the port.
 */

static void
DoGrow(WHHandle h, Point startPt)
{
	GrafPtr         growPort;
	Rect            growRect;
	LongInt         growRes;
	WHERE("DoGrow");
	if (h != (WHHandle) nil)
	{
		growPort = (**h).whWind;
		growRect = (**h).whGrow;

		/* growRes will be zero if the size was not actually changed */

		if (growRes = GrowWindow(growPort, startPt, &growRect))
		{
			SizeWindow(growPort, LoWord(growRes), HiWord(growRes), false);
			TriggerUpdate(h);
		}
	}
} /*DoGrow()*/


/*
 * Zoom the current window.  Very similar to DoGrow, but window
 * is erased before zooming for nicer visual effect (per IM IV-50,
 * TN TB 30, p.4).
 *
 * Normally, since only the active window has a visible zoom box and
 * TransSkel sets the port to active window, this routine is triggered
 * by user-initiated clicks in zoom box and the port will be set to
 * the zoomed window.
 *
 * However, it is possible for zooms to be software initiated by the
 * application itself on any window; for such cases the port needs
 * to be saved and set before the zoom and restored afterward.
 */

static void
DoZoom(WHHandle h, Integer zoomDir)
{
	GrafPtr         w;
	GrafPtr         tmpPort;
	Rect            r, growRect;

	if (h != (WHHandle) nil)
	{
		w = (**h).whWind;
		GetPort(&tmpPort);	/* save port and set to */
		SetPort(w);	/* zoomed window */
		if ((**h).whZoom != (SkelWindZoomProcPtr) nil)
		{
			((**h).whZoom) (w, zoomDir);	/* custom zoom proc */
		}
		else if (zoomProc != (SkelWindZoomProcPtr) nil)
		{
			(*zoomProc) (w, zoomDir);	/* custom default zoom proc */
		}
		else
			/* default zooming */
		{
			EraseRect(&w->portRect);
			if (zoomDir == inZoomOut)	/* zooming to default state */
			{
				/*
				 * Get the usable area of the device containing most of the
				 * window.  (Can ignore the result because the rect is always
				 * correct.  Pass nil for device parameter because it's
				 * irrelevant.)  Then adjust rect for title bar height, and
				 * inset it slightly.
				 */
				(void) SkelGetWindowDevice(w, (GDHandle *) nil, &r);
				r.top += SkelGetWindTitleHeight(w) - 1;
				/* leave 3-pixel border */
				InsetRect(&r, 3, 3);
				/* clip to grow limits */
				growRect = (**h).whGrow;
				growRect.left = growRect.top = 0;
				OffsetRect(&growRect, r.left, r.top);
				SectRect(&r, &growRect, &r);
				(**(WStateData **) (((WindowPeek) w)->dataHandle)).stdState = r;
			}
			ZoomWindow(w, zoomDir, false);
		}
		SetPort(tmpPort);	/* restore original port */
		TriggerUpdate(h);
	}
} /*DoZoom()*/


/* --------------------------------------------------------- */
/* Window handler installation/removal/modification routines */
/* --------------------------------------------------------- */


/*
 * Install handler for a window and set current port to it.  Remove
 * any previous handler for it.  Pass the following parameters:
 *
 * w
 *		Pointer to the window to be handled.  Must be created by host.
 * doMouse
 *		Proc to handle mouse clicks in window.  The proc will be
 * 		passed the point (in local coordinates), the time of the
 * 		click, and the modifier flags word.
 * doKey
 *		Proc to handle key clicks in window.  The proc will be passed
 * 		the character and the modifier flags word.
 * doUpdate
 *		Proc for updating window.  TransSkel brackets calls to update
 * 		procs with calls to BeginUpdate and EndUpdate, so the visRgn
 * 		is set up correctly.  A flag is passed indicating whether the
 * 		window was resized or not.  BY CONVENTION, the entire portRect
 * 		is invalidated when the window is resized or zoomed.  That way,
 * 		the handler's update proc can redraw the entire content region
 * 		without interference from BeginUpdate/EndUpdate.  The flag
 * 		is set to false after the update proc is called; the
 * 		assumption is made that the proc will notice the resizing and
 * 		respond appropriately.
 * doActivate
 *		Proc to execute when window is activated or deactivated.
 * 		A boolean is passed to it which is true if the window is
 * 		coming active, false if it's going inactive.
 * doClose
 *		Proc to execute when mouse clicked in close box.  Useful
 * 		mainly to temp window handlers that want to know when to
 * 		self-destruct (with SkelRmveWind).
 * doClobber
 *		Proc for disposal of handler's data structures
 * doWIdle
 *		Proc to execute when no events are pending.
 * idleFrontOnly
 *		True if doWIdle should execute on no events only when
 * 		w is frontmost, false if executes all the time.  Note
 * 		that if it always goes, everything else may be slowed down!
 *
 * If a particular procedure is not needed (e.g., key events are
 * not processed by a handler), pass nil in place of the appropriate
 * procedure address.
 *
 * Return true if successful, false if no handler could be allocated.
 * If false is returned, the port will not have been changed.
 */

pascal Boolean
SkelWindow(WindowPtr w,
		   SkelWindMouseProcPtr doMouse,
		   SkelWindKeyProcPtr doKey,
		   SkelWindUpdateProcPtr doUpdate,
		   SkelWindActivateProcPtr doActivate,
		   SkelWindCloseProcPtr doClose,
		   SkelWindClobberProcPtr doClobber,
		   SkelWindIdleProcPtr doWIdle,
		   Boolean idleFrontOnly)
{
	WHHandle        whNew, whCur;
	SkelWindPropHandle wph = (SkelWindPropHandle) nil;

	/* Get new handler immediately, fail if can't allocate */

	if ((whNew = New(WHandler)) == (WHHandle) nil)
	{
		return (false);
	}

	/*
	 * If there's a current handler for the window, remove it, but first
	 * grab the property list from it so it can be transferred to the new
	 * handler.
	 */

	if ((whCur = GetWDHandler(w)) != (WHHandle) nil)
	{
		wph = (**whCur).whProperties;
		(**whCur).whProperties = (SkelWindPropHandle) nil;
		DetachWDHandler(whCur);
	}

	/*
	 * Attach new handler to list of handlers.  It is attached to the
	 * beginning of the list, which is simpler; the order is presumably
	 * irrelevant to the host, anyway.
	 *
	 * Then fill in handler fields (including properties attached to any
	 * previous handler).
	 */

	(**whNew).whNext = whList;
	whList = whNew;

	(**whNew).whWind = w;
	(**whNew).whMouse = doMouse;
	(**whNew).whKey = doKey;
	(**whNew).whUpdate = doUpdate;
	(**whNew).whActivate = doActivate;
	(**whNew).whClose = doClose;
	(**whNew).whClobber = doClobber;
	(**whNew).whZoom = (SkelWindZoomProcPtr) nil;
	(**whNew).whIdle = doWIdle;
	(**whNew).whGrow = growRect;
	(**whNew).whSized = false;
	(**whNew).whFrontOnly = idleFrontOnly;
	(**whNew).whFlags = 0;
	(**whNew).whProperties = wph;
	SetPort(w);

	return (true);
} /*SkelWindow()*/


/*
 * Remove a window handler.  This calls the handler's window disposal
 * routine and then takes the handler out of the handler list and
 * disposes of it (including its property list).
 *
 * SkelRmveWind is also called by SkelRmveDlog.
 */

pascal void
SkelRmveWind(WindowPtr w)
{
	WHHandle        h;

	if ((h = GetWDHandler(w)) == (WHHandle) nil)
	{
		return;
	}
	
	DoClobber(h);		/* call disposal routine */
	SkelRmveWindProp(w, skelWPropAll);	/* toss properties */

	DetachWDHandler(h);	/* remove handler for window from list */
} /*SkelRmveWind()*/


#ifdef	supportDialogs


/*
 * Install a handler for a modeless dialog window and set the port
 * to it.  Remove any previous handler for it. SkelDialog calls
 * SkelWindow as a subsidiary to install a window handler, then sets
 * the event procedure on return.  A property is also added to the window
 * to indicate that it's a modeless dialog.
 *
 * Pass the following parameters:
 *
 * theDialog
 *		Pointer to the dialog to be handled.  Must be created by host.
 * pEvent
 *		Event-handling proc for dialog events.
 * doClose
 *		Proc to execute when mouse clicked in close box.  Useful
 * 		mainly to dialog handlers that want to know when to
 * 		self-destruct (with SkelRmveDlog).
 * doClobber
 *		Proc for disposal of handler's data structures
 *
 * If a particular procedure is not needed, pass nil in place of
 * the appropriate procedure address.
 *
 * Return true if successful, false if no handler could be allocated.
 * If false is returned, the port will not have been changed.
 */

pascal Boolean
SkelDialog(DialogPtr theDialog,
		   SkelWindEventProcPtr pEvent,
		   SkelWindCloseProcPtr doClose,
		   SkelWindClobberProcPtr doClobber)
{
	if (!SkelWindow(theDialog, nil, nil, nil, nil, doClose, doClobber,
					nil, false))
	{
		return (false);
	}

	if (!SkelAddWindProp(theDialog, skelWPropModeless, (LongInt) 0))
	{
		SkelRmveDlog(theDialog);
		return (false);
	}

	(**GetWDHandler(theDialog)).whEvent = pEvent;
	return (true);
} /*SkelDialog()*/


/*
 * Remove a dialog and its handler
 */

pascal void
SkelRmveDlog(DialogPtr theDialog)
{
	SkelRmveWind(theDialog);
} /*SkelRmveDlog()*/

#endif


/*
 * Override the default sizing limits for a window, or, if w
 * is nil, reset the default limits used by SkelWindow.
 */

pascal void
SkelSetGrowBounds(WindowPtr w, Integer hLo, Integer vLo, Integer hHi,
				  Integer vHi)
{
	WHHandle        wh;
	Rect            r;

	if (w == (WindowPtr) nil)
	{
		SetRect(&growRect, hLo, vLo, hHi, vHi);
	}
	else if ((wh = GetWDHandler(w)) != (WHHandle) nil)
	{
		SetRect(&r, hLo, vLo, hHi, vHi);
		(**wh).whGrow = r;
	}
} /*SkelSetGrowBounds()*/


pascal void
SkelSetZoom(WindowPtr w, SkelWindZoomProcPtr pZoom)
{
	WHHandle        h;

	if (w == (WindowPtr) nil)
	{
		zoomProc = pZoom;
	}
	else if ((h = GetWDHandler(w)) != (WHHandle) nil)
	{
		(**h).whZoom = pZoom;
	}
} /*SkelSetZoom()*/


/*
 * Return zoom proc associated with window, nil if there isn't one.
 * Return default zoom proc if window is nil.
 */

pascal SkelWindZoomProcPtr
SkelGetZoom(WindowPtr w)
{
	WHHandle        h;

	if (w == (WindowPtr) nil)
	{
		return (zoomProc);
	}
	if ((h = GetWDHandler(w)) != (WHHandle) nil)
	{
		return ((**h).whZoom);
	}
	return ((SkelWindZoomProcPtr) nil);
} /*SkelGetZoom()*/


pascal Boolean
SkelWindowRegistered(WindowPtr w)
{
	return ((Boolean) (GetWDHandler(w) != (WHHandle) nil));
} /*SkelWindowRegistered()*/


/* ------------------------ */
/* Handler finders/removers */
/* ------------------------ */

/*
 * Get handler associated with user or dialog window.
 * Return nil if window doesn't belong to any known handler.
 * (Returns nil if window is nil, due to the way oldWDHandler
 * is set by DetachWDHandler.)
 *
 * This routine is absolutely fundamental to TransSkel.
 */


static WHHandle
GetWDHandler(WindowPtr w)
{
	WHHandle        h;

	if (w == oldWindow)
	{
		return (oldWDHandler);	/* return handler of cached window */
	}

	for (h = whList; h != (WHHandle) nil; h = (**h).whNext)
	{
		if ((**h).whWind == w)
		{
			oldWindow = w;	/* set cached window and handler */
			oldWDHandler = h;
			return (h);
		}
	}
	return ((WHHandle) nil);
} /*GetWDHandler()*/


/*
 * Get handler associated with user window.
 * Return nil if window doesn't belong to any known handler.
 * The order of the two tests is critical:  w might be nil.
 */

static WHHandle
GetWHandler(WindowPtr w)
{
	WHHandle        h;

	if ((h = GetWDHandler(w)) != (WHHandle) nil
		&& ((WindowPeek) w)->windowKind != dialogKind)
	{
		return (h);
	}
	return ((WHHandle) nil);
} /*GetWHandler()*/


#ifdef	supportDialogs

/*
 * Get handler associated with dialog window.
 * Return nil if window doesn't belong to any known handler.
 * The order of the two tests is critical:  theDialog might be nil.
 */

static WHHandle
GetDHandler(DialogPtr theDialog)
{
	WHHandle        h;

	if ((h = GetWDHandler(theDialog)) != (WHHandle) nil
		&& ((WindowPeek) theDialog)->windowKind == dialogKind)
	{
		return (h);
	}
	return ((WHHandle) nil);
} /*GetDHandler()*/

#endif


/*
 * Detach a handler from the handler list and destroy it.
 *
 * Clear window cache variable, just in case it points to the window
 * whose hander is being destroyed (and thus has become invalid).
 */

static void
DetachWDHandler(WHHandle wh)
{
	WHHandle        h, h2;

	if (whList != (WHHandle) nil)	/* if list empty, ignore */
	{
		if (whList == wh)	/* is it the first element? */
		{
			h2 = whList;
			whList = (**whList).whNext;
		}
		else
		{
			for (h = whList; h != (WHHandle) nil; h = h2)
			{
				h2 = (**h).whNext;
				if (h2 == (WHHandle) nil)
				{
					return;	/* handler not in list! (huh?) */
				}
				if (h2 == wh)	/* found it */
				{
					(**h).whNext = (**h2).whNext;
					break;
				}
			}
		}
		DisposeHandle((Handle) h2);	/* get rid of handler record */
	}

	oldWindow = (WindowPtr) nil;	/* clear window cache variables */
	oldWDHandler = (WHHandle) nil;
} /*DetachWDHandler()*/


/* ------------------------------------------------------- */
/* Menu handler installation/removal/modification routines */
/* ------------------------------------------------------- */


/*
 * Install handler for a menu.  Remove any previous handler for it.
 * Pass the following parameters:
 *
 * theMenu
 *		Handle to the menu to be handled.  Must be created by host.
 * doSelect
 *		Proc that handles selection of items from menu.  If this is
 * 		nil, the menu is installed, but nothing happens when items
 * 		are selected from it.
 * doClobber
 *		Proc for disposal of handler's data structures.  Usually
 * 		nil for menus that remain in menu bar until program
 * 		termination.
 * subMenu
 *		True if the menu is a submenu (not installed in menu bar).
 * drawBar
 *		True if menu bar is to be drawn after menu is installed.
 * 		(Ignored if the menu is a submenu.)
 *
 * Return true if successful, false if no handler could be allocated.
 */

pascal Boolean
SkelMenu(MenuHandle m,
		 SkelMenuSelectProcPtr doSelect,
		 SkelMenuClobberProcPtr doClobber,
		 Boolean subMenu,
		 Boolean drawBar)
{
	MHHandle        mh;
	Boolean         oldFlag;

	oldFlag = mhClobOnRmve;	/* remove any previous handler for */
	mhClobOnRmve = false;	/* menu, without redrawing menu bar */
	SkelRmveMenu(m);
	mhClobOnRmve = oldFlag;

	if ((mh = New(MHandler)) != (MHHandle) nil)
	{
		(**mh).mhNext = mhList;
		mhList = mh;
		(**mh).mhID = (**m).menuID;	/* get menu id number */
		(**mh).mhSelect = doSelect;	/* install selection handler */
		(**mh).mhClobber = doClobber;	/* install disposal handler */
		(**mh).mhSubMenu = subMenu;	/* set submenu flag */
		/* install menu in menu bar if not a submenu */
		InsertMenu(m, subMenu ? -1 : 0);
	}
	if (drawBar && !subMenu)
	{
		DrawMenuBar();
	}
	return ((Boolean) (mh != (MHHandle) nil));
} /*SkelMenu()*/


/*
 * Remove a menu handler.  This calls the handler's menu disposal
 * routine and then takes the handler out of the handler list and
 * disposes of it.  The menu bar is redrawn if the menu was not a
 * submenu and the global redraw flag hasn't been cleared.
 *
 * Note that the menu MUST be deleted from the menu bar before calling
 * the clobber proc, because the menu bar will end up filled with
 * garbage if the menu was allocated with NewMenu (IM I-352).
 */

pascal void
SkelRmveMenu(MenuHandle m)
{
	Integer         mID;
	MHHandle        h, h2;
	SkelMenuClobberProcPtr p;

	mID = (**m).menuID;
	if (mhList != (MHHandle) nil)	/* if list empty, ignore */
	{
		if ((**mhList).mhID == mID)	/* is it the first element? */
		{
			h2 = mhList;
			mhList = (**mhList).mhNext;
		}
		else
		{
			for (h = mhList; h != (MHHandle) nil; h = h2)
			{
				h2 = (**h).mhNext;
				if (h2 == (MHHandle) nil)
				{
					return;	/* menu not in list! */
				}
				if ((**h2).mhID == mID)	/* found it */
				{
					(**h).mhNext = (**h2).mhNext;
					break;
				}
			}
		}
		DeleteMenu(mID);
		if (mhDrawBarOnRmve && !(**h2).mhSubMenu)
		{
			DrawMenuBar();
		}
		if (mhClobOnRmve
			&& (p = (**h2).mhClobber) != (SkelMenuClobberProcPtr) nil)
		{
			(*p) (m);	/* call disposal routine */
		}
		DisposeHandle((Handle) h2);	/* get rid of handler record */
	}
} /*SkelRmveMenu()*/


/*
 * General menu-selection handler.  Just passes selection to the handler's
 * select routine.  If the select routine is nil, selecting items from
 * the menu is a nop.
 */

static void
DoMenuCommand(LongInt command)
{
	Integer         menu;
	Integer         item;
	MHHandle        mh;
	menu = HiWord(command);
	item = LoWord(command);
	for (mh = mhList; mh != (MHHandle) nil; mh = (**mh).mhNext)
	{
		if (menu == (**mh).mhID &&
			(**mh).mhSelect != (SkelMenuSelectProcPtr) nil)
		{
			(*(**mh).mhSelect) (item);
			break;
		}
	}
	HiliteMenu(0);		/* command done, turn off menu hiliting */
} /*DoMenuCommand()*/


/*
 * Menu is about to be pulled down or command-key executed.  Call menu
 * hook if there is one so application can set menus/items appropriately.
 */

static void
DoMenuHook(void)
{
	if (pMenuHook != (SkelMenuHookProcPtr) nil)
	{
		(*pMenuHook) ();
	}
} /*DoMenuHook()*/


pascal void
SkelSetMenuHook(SkelMenuHookProcPtr p)
{
	pMenuHook = p;
} /*SkelSetMenuHook()*/


pascal SkelMenuHookProcPtr
SkelGetMenuHook(void)
{
	return (pMenuHook);
} /*SkelGetMenuHook()*/


/* ------------------------ */
/* Window property routines */
/* ------------------------ */


/*
 * Add a property to a window.  Fail if the window is unregistered
 * or can't allocate memory for a new property structure.  If the
 * window already has such a property, fail.
 */

pascal Boolean
SkelAddWindProp(WindowPtr w, Integer propType, LongInt propData)
{
	WHHandle        wh;
	SkelWindPropHandle ph;

	if (propType == skelWPropAll)
	{
		return (false);
	}
	if ((ph = SkelGetWindProp(w, propData)) != (SkelWindPropHandle) nil)
	{
		return (false);
	}
	/* if window is unregistered, or can't allocate structure, fail */
	if ((wh = GetWDHandler(w)) == (WHHandle) nil
		|| (ph = New(SkelWindProperty)) == (SkelWindPropHandle) nil)
	{
		return (false);
	}
	(**ph).skelWPropType = propType;
	(**ph).skelWPropData = propData;
	(**ph).skelWPropNext = (**wh).whProperties;
	(**wh).whProperties = ph;
	return (true);
} /*SkelAddWindProp()*/


/*
 * Remove a window property.  Does nothing if the window isn't
 * registered or if the window doesn't have the given property.
 *
 * If propType is skelWPropAll, SkelRmveWindProp() calls itself
 * recursively to remove all the properties on a window.  This
 * means that if you put skelWPropAll into the skelWPropType field
 * of a property, you'll get an infinite loop here.
 */

pascal void
SkelRmveWindProp(WindowPtr w, Integer propType)
{
	WHHandle        wh;
	SkelWindPropHandle ph, ph2, pNext;

	if ((wh = GetWDHandler(w)) == (WHHandle) nil
		|| (ph = SkelGetWindProp(w, propType)) == (SkelWindPropHandle) nil)
	{
		return;
	}

	if (propType == skelWPropAll)	/* remove all properties */
	{
		while ((ph = (**wh).whProperties) != (SkelWindPropHandle) nil)
		{
			SkelRmveWindProp(w, (**ph).skelWPropType);
		}
		return;
	}

	/* remove particular property */
	if ((ph2 = (**wh).whProperties) == ph)	/* remove first in list */
	{
		(**wh).whProperties = (**ph).skelWPropNext;
	}
	else
	{
		while ((pNext = (**ph2).skelWPropNext) != (SkelWindPropHandle) nil)
		{
			if (pNext == ph)
			{
				(**ph2).skelWPropNext = (**ph).skelWPropNext;
				break;
			}
			ph2 = pNext;
		}
	}
	DisposeHandle((Handle) ph);
} /*SkelRmveWindProp()*/


/*
 * Find the given property for the window.  Fail if window is
 * unregistered or has no such property.
 */

pascal SkelWindPropHandle
SkelGetWindProp(WindowPtr w, Integer propType)
{
	WHHandle        wh;
	SkelWindPropHandle ph = (SkelWindPropHandle) nil;

	if ((wh = GetWDHandler(w)) != (WHHandle) nil)
	{
		if (propType == skelWPropAll)	/* return head of list */
		{
			ph = (**wh).whProperties;
		}
		else
		{
			for (ph = (**wh).whProperties; ph != (SkelWindPropHandle) nil;
				 ph = (**ph).skelWPropNext)
			{
				if ((**ph).skelWPropType == propType)
				{
					break;
				}
			}
		}
	}
	return (ph);
} /*SkelGetWindProp()*/

