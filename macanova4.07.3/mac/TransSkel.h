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
  971128  Added ifdefs for new MW Codewarrior
*/

/*
*(C)* Most modules in this file are Copyright (c) by Paul DuBois and have
*(C)* been placed in the public domain.  The additions and changes made by
*(C)* C. Bingham for use with MPW and interfacing with MacAnova are also being
*(C)* placed in the public domain.
*(C)* Macanova related changes are bracketed by #ifdef KB ... #endif
*(C)* Purely MPW changes are bracketed by #ifdef MPW ... #endif
*/
/*
 * prototypes for TransSkel public routines
 *
 * Written for THINK C 6.0.1.
 *
 * Assumes that compiler understands nested prototypes.
 *
 * You must have THINK C Language Extensions turned on in the
 * Edit/Options.../Language Settings dialog (so that the "pascal" keyword
 * is recognized, for one thing).
 *
 * for:			TransSkel 3.12
 * last edit:	14 Apr 94
 */

# ifndef	__TRANSSKEL_H__

# define	__TRANSSKEL_H__

#if defined(MPW) || defined(MW_CW)
#ifndef MACIFACEH__
# include	<Dialogs.h>	/* pulls in Windows.h, MacTypes.h, QuickDraw.h */
# include   <Resources.h>
# include	<Events.h>
# include	<Menus.h>
# include 	<Fonts.h>
# include	<TextEdit.h>
# include	<ToolUtils.h>
#ifndef MW_CW_New
# 	include	<Desk.h>
# endif /*MW_CW_New*/

# include	<Memory.h>
# include	<DiskInit.h>
#ifndef MW_CW_New
# include	<OSEvents.h>
#endif /*MW_CW_New*/
# include	<Traps.h>
# include	<Script.h>
#endif /*MACIFACEH__*/
#endif /*MPW*/

#ifdef KB

#ifndef MACTYPEDEF

#ifndef SIZEINTEGER
#undef  SIZELONGINT

#if defined(MPW) || defined(MW_CW)
#define SIZEINTEGER short
#define SIZELONGINT long
#else
#define SIZEINTEGER int
#define SIZELONGINT long
#endif /*MPW*/

#endif /*SIZEINTEGER*/

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

#if defined(MW_CW_New) && !defined(NEWNAMES_SET) /*various name changed required in CW 11*/
#define NEWNAMES_SET

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
/* from Resources.h */
#define SizeResource(theResource) GetResourceSizeOnDisk(theResource)
#define MaxSizeRsrc(theResource) GetMaxResourceSize(theResource)
#define RmveResource(theResource) RemoveResource(theResource)

#endif /*MW_CW_New*/

#else /*KB*/
#ifndef MACTYPEDEF
#define Integer    short
#define LongInt    long

#ifndef MPW3_2
typedef pascal void (*ResumeProcPtr)(void); 
#endif

#define MACTYPEDEF
#endif /*MACTYPEDEF*/
#endif /*KB*/

# define	skelMajorRelease	3
# define	skelMinorRelease	12


/* window property numbers */

# define	skelWPropAll			0		/* pseudo-property */
# define	skelWPropModeless		1		/* modeless dialog */
# define	skelWPropModal			2		/* modal dialog */
# define	skelWPropTool			3		/* tool window */
# define	skelWPropMovableModal	4		/* movable modal dialog */
# define	skelWPropHelp			5		/* help window */
# define	skelWPropText			6		/* text window */
# define	skelWPropDisplayWind	7		/* TransDisplay window */
# define	skelWPropEditWind		8		/* TransEdit window */
# define	skelWPropApplBase		256		/* general-use prop num base */


/* SkelQuery() query selectors */

# define	skelQVersion		1			/* TransSkel version */
# define	skelQSysVersion		2			/* System software version */
# define	skelQHasWNE			3			/* whether has WaitNextEvent() */
# define	skelQMBarHeight		4			/* menu bar height */
# define	skelQHas64KROM		5			/* whether has 64K ROM */
# define	skelQHasColorQD		6			/* whether has Color QuickDraw */
# define	skelQInForeground	7			/* whether in foreground */
# define	skelQHasGestalt		8			/* whether has Gestalt() */
# define	skelQHasAppleEvents	9			/* whether has Apple Events */
# define	skelQQDVersion		10			/* QuickDraw version */
# define	skelQGrayRgn		11			/* Desktop region */


/* window property types */

typedef struct SkelWindProperty	SkelWindProperty, **SkelWindPropHandle;

struct SkelWindProperty
{
	Integer				skelWPropType;
	LongInt				skelWPropData;
	SkelWindPropHandle	skelWPropNext;
};


/* initialization parameters */

typedef struct SkelInitParams SkelInitParams, *SkelInitParamsPtr;

struct SkelInitParams
{
	Integer			skelMoreMasters;
	GrowZoneProcPtr skelGzProc;
	ResumeProcPtr	skelResumeProc;
	Size			skelStackAdjust;
};


/*
 * typedefs for pointers to various sorts of functions used by TransSkel
 * routines.
 */

typedef pascal void (*SkelIdleProcPtr) (void);
typedef	pascal Boolean (*SkelEventHookProcPtr) (EventRecord *);
typedef pascal void (*SkelSuspendResumeProcPtr) (Boolean inForeground);
typedef pascal void (*SkelClipCvtProcPtr) (Boolean inForeground);
typedef pascal void (*SkelAEHandlerProcPtr) (EventRecord *);
typedef	pascal void (*SkelDlogItemProcPtr) (DialogPtr d, Integer item);


/* ------------- */
/* Core routines */
/* ------------- */

/* initialization/termination routines */

pascal void SkelGetInitParams (SkelInitParamsPtr initParams);
pascal void SkelInit (SkelInitParamsPtr initParams);
pascal void SkelCleanup (void);

/* event-loop-related routines */

pascal void SkelEventLoop (void);
pascal void SkelStopEventLoop (void);
pascal void SkelRouteEvent (EventRecord *theEvent);
pascal void SkelActivate (WindowPtr w, Boolean active);
pascal void SkelClose (WindowPtr w);
pascal EventRecord *SkelGetCurrentEvent (void);
pascal Integer SkelGetModifiers (void);
pascal void SkelSetEventMask (Integer mask);
pascal Integer SkelGetEventMask (void);
pascal void SkelSetDlogMask (Integer mask);
pascal Integer SkelGetDlogMask (void);
pascal void SkelSetIdle (SkelIdleProcPtr p);
pascal SkelIdleProcPtr SkelGetIdle (void);
pascal void SkelSetEventHook (SkelEventHookProcPtr p);
pascal SkelEventHookProcPtr SkelGetEventHook (void);
pascal void SkelSetSuspendResume (SkelSuspendResumeProcPtr p);
pascal SkelSuspendResumeProcPtr SkelGetSuspendResume (void);
pascal void SkelSetClipCvt (SkelClipCvtProcPtr p);
pascal SkelClipCvtProcPtr SkelGetClipCvt (void);
pascal void SkelSetWaitTimes (LongInt fgTime, LongInt bgTime);
pascal void SkelGetWaitTimes (LongInt *pFgTime, LongInt *pBgTime);

pascal void SkelSetAEHandler (SkelAEHandlerProcPtr p);
pascal SkelAEHandlerProcPtr SkelGetAEHandler (void);


/* window handling routines */

typedef	pascal void (*SkelWindMouseProcPtr) (Point where, LongInt when, Integer modifiers);
/*
 * Key handler needs special treatment because for prototyped functions
 * (which TransSkel uses), THINK C passes character arguments in the *high*
 * byte of a two-byte stack value.  To make sure the values are passed in the
 * low byte from either C or Pascal key handlers, the first two arguments are
 * passed in shorts.  The Pascal key procedure should
 * look like this:
 *     procedure Key (c: char; code: Integer; modifiers: Integer);
 */
typedef	pascal void (*SkelWindKeyProcPtr) (Integer c, Integer code, Integer modifiers);
typedef	pascal void (*SkelWindUpdateProcPtr) (Boolean resized);
typedef	pascal void (*SkelWindActivateProcPtr) (Boolean active);
typedef	pascal void (*SkelWindCloseProcPtr) (void);
typedef	pascal void (*SkelWindClobberProcPtr) (void);
typedef	pascal void (*SkelWindIdleProcPtr) (void);

typedef	pascal void (*SkelWindEventProcPtr) (Integer item, EventRecord *event);

typedef	pascal void (*SkelWindZoomProcPtr) (WindowPtr w, Integer zoomDir);

pascal Boolean SkelWindow (WindowPtr w,
					SkelWindMouseProcPtr pMouse,
					SkelWindKeyProcPtr pKey,
					SkelWindUpdateProcPtr pUpdate,
					SkelWindActivateProcPtr pActivate,
					SkelWindCloseProcPtr pClose,
					SkelWindClobberProcPtr pClobber,
					SkelWindIdleProcPtr pIdle,
					Boolean idleFrontOnly);
pascal Boolean SkelDialog (DialogPtr dlog,
					SkelWindEventProcPtr pEvent,
					SkelWindCloseProcPtr pClose,
					SkelWindClobberProcPtr pClobber);
pascal void SkelRmveWind (WindowPtr w);
pascal void SkelRmveDlog (DialogPtr dlog);
pascal Boolean SkelWindowRegistered (WindowPtr w);
pascal void SkelSetGrowBounds (WindowPtr w,
							Integer hLo, Integer vLo,
							Integer hHi, Integer vHi);
pascal void SkelSetZoom (WindowPtr w, SkelWindZoomProcPtr pZoom);
pascal SkelWindZoomProcPtr SkelGetZoom (WindowPtr w);
pascal Boolean SkelGetRectDevice (Rect *rp, GDHandle *gd, Rect *devRect, Boolean *isMain);
pascal Boolean SkelGetWindowDevice (WindowPtr w, GDHandle *gd, Rect *devRect);
pascal void SkelGetWindContentRect (WindowPtr w, Rect *rp);
pascal void SkelGetWindStructureRect (WindowPtr w, Rect *rp);
pascal Integer SkelGetWindTitleHeight (WindowPtr w);

pascal Boolean SkelAddWindProp (WindowPtr w, Integer propType, LongInt propData);
pascal void SkelRmveWindProp (WindowPtr w, Integer propType);
pascal SkelWindPropHandle SkelGetWindProp (WindowPtr w, Integer propType);


/* menu handling routines */

typedef	pascal void (*SkelMenuSelectProcPtr) (Integer item);
typedef	pascal void (*SkelMenuClobberProcPtr) (MenuHandle m);
typedef pascal void (*SkelMenuHookProcPtr) (void);

pascal Boolean SkelMenu (MenuHandle m,
					SkelMenuSelectProcPtr pSelect,
					SkelMenuClobberProcPtr pClobber,
					Boolean subMenu,
					Boolean drawBar);
pascal void SkelRmveMenu (MenuHandle m);

pascal void SkelSetMenuHook (SkelMenuHookProcPtr p);
pascal SkelMenuHookProcPtr SkelGetMenuHook (void);


/* environment information routines */

pascal LongInt SkelQuery (Integer selector);
pascal Boolean SkelTrapAvailable (Integer theTrap);


/* ------------------ */
/* Auxiliary routines */
/* ------------------ */

# define	skelAppleMenuID		128

pascal void SkelApple (StringPtr items, SkelMenuSelectProcPtr pSelect);
pascal MenuHandle SkelGetAppleMenu(void); /*Added by C. Bingham*/

pascal void SkelDoEvents (Integer mask);
pascal void SkelDoUpdates (void);
pascal ModalFilterProcPtr SkelDlogFilter (ModalFilterProcPtr filter, Boolean doReturn);
pascal ModalFilterYDProcPtr SkelDlogFilterYD (ModalFilterYDProcPtr filter, Boolean doReturn);
pascal void SkelRmveDlogFilter (void);
pascal void SkelDlogDefaultItem (Integer item);
pascal void SkelDlogCancelItem (Integer item);
pascal void SkelDlogTracksCursor (Boolean track);

pascal Boolean SkelCmdPeriod (EventRecord *evt);


/* -------------------- */
/* Convenience routines */
/* -------------------- */

/* positioning types for SkelGetReferenceRect()/SkelPositionWindow() */

# define	skelPositionNone			0	/* leave as is */
# define	skelPositionOnMainDevice	1	/* position on main device */
# define	skelPositionOnParentWindow	2	/* position on FrontWindow() */
# define	skelPositionOnParentDevice	3	/* position on FrontWindow()'s device */

pascal void SkelGetMainDeviceRect (Rect *r);
pascal void SkelPositionRect (Rect *refRect, Rect *r, Fixed hRatio, Fixed vRatio);
pascal void SkelGetReferenceRect (Rect *r, Integer positionType);
pascal void SkelPositionWindow (WindowPtr w, Integer positionType, Fixed hRatio, Fixed vRatio);
pascal Boolean SkelTestRectVisible (Rect *r);

/* alert presentation routines */

pascal Integer SkelAlert (Integer alrtResNum, ModalFilterProcPtr filter, Integer positionType);
pascal void SkelSetAlertPosRatios (Fixed hRatio, Fixed vRatio);
pascal void SkelGetAlertPosRatios (Fixed *hRatio, Fixed *vRatio);

/* control manipulation routines */

pascal Boolean SkelHiliteControl (ControlHandle ctrl, Integer hilite);
pascal void SkelDrawButtonOutline (ControlHandle ctrl);
pascal void SkelEraseButtonOutline (ControlHandle ctrl);
pascal void SkelFlashButton (ControlHandle ctrl);
pascal Integer SkelToggleCtlValue (ControlHandle ctrl);

/* dialog item manipulation routines */

pascal ControlHandle SkelGetDlogCtl (DialogPtr d, Integer item);
pascal Boolean SkelSetDlogCtlHilite (DialogPtr d, Integer item, Integer hilite);
pascal Integer SkelGetDlogCtlHilite (DialogPtr d, Integer item);
pascal void SkelSetDlogCtlValue (DialogPtr d, Integer item, Integer value);
pascal Integer SkelGetDlogCtlValue (DialogPtr d, Integer item);
pascal Integer SkelToggleDlogCtlValue (DialogPtr d, Integer item);
pascal void SkelSetDlogCtlRefCon (DialogPtr d, Integer item, LongInt value);
pascal LongInt SkelGetDlogCtlRefCon (DialogPtr d, Integer item);
pascal void SkelSetDlogStr (DialogPtr d, Integer item, StringPtr str);
pascal void SkelGetDlogStr (DialogPtr d, Integer item, StringPtr str);
pascal void SkelSetDlogRect (DialogPtr d, Integer item, Rect *r);
pascal void SkelGetDlogRect (DialogPtr d, Integer item, Rect *r);
pascal void SkelSetDlogProc (DialogPtr d, Integer item, SkelDlogItemProcPtr p);
pascal SkelDlogItemProcPtr SkelGetDlogProc (DialogPtr d, Integer item);
pascal void SkelSetDlogType (DialogPtr d, Integer item, Integer type);
pascal Integer SkelGetDlogType (DialogPtr d, Integer item);
pascal void SkelSetDlogRadioButtonSet (DialogPtr dlog, Integer first, Integer last, Integer choice);
pascal void SkelSetDlogButtonOutliner (DialogPtr d, Integer item);

pascal void SkelPause (LongInt ticks);

# endif /* __TRANSSKEL_H__ */
