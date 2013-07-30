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

#ifndef MACPROTOH__
#define MACPROTOH__

#ifndef GLOBALSH__
#include <stdio.h> /* needed for FILE * */
#endif /*GLOBALSH__*/

#ifndef MACIFACEH__
#include "macIface.h"
#endif /*MACIFACEH__*/

/*
	970111 added initHistory(), saveInHistory() and recallHistory()
	970115 added scanKeyboard() and isKeyPressed()
	970223 added setCmdHelpItems(), setGraphHelpItems()
	970503 added getScreenRect()
	980511 changed fillLINE() to fileFromDialog() in conjuction of
           move of fillLINE() to commonio.c.  fillLINE() is now defined
           in matProto.h
*/
/* TransSkel.c */
/* all TransSkel prototypes are now in TransSkel.h */
pascal void SkelChkOneMaskedEvent (Integer /*eventMask*/, LongInt /*waitTime*/);
pascal void SkelChkOneEvent (void);
/* macGraph.c */
/* Note:  All TransSkel called routines now must be pascal */
char *findPlotFile(char * /*fileName*/, char * /*kind*/, long /*ps*/,
				   long /*epsf*/, long /*newFile*/);
Boolean writePictFile(PicHandle /*picture*/, FILE * /*pictFile*/);
void doSaveGraph(Integer /*windno*/, Integer /*item*/);
void setPanelRect(Rect * /*r*/, Integer /*panelno*/);
void GraphRectInit(Integer /*windno*/);
PicHandle GraphWindInit(Integer  /*count*/);
PicHandle GraphWindSetup(void);
void GraphWindPause(void);
void GraphOver(Integer /*errorCode*/);
pascal void GraphWindIdle(void);
pascal void GraphWindClobber(void);
pascal void GraphWindClose(void);
pascal void GraphWindZoom(WindowPtr /*theWindow*/, Integer /*partCode*/);
pascal void GraphWindActivate(Boolean /*active*/);
pascal void GraphWindMouse(Point /*thePt*/, LongInt /*when*/, Integer /*mods*/);
pascal void GraphWindKey(Integer /*ch*/, Integer /*code*/, Integer /*mods*/);
pascal void GraphWindUpdate(Boolean /*resized*/);
void GraphWindPrint(Integer /*windno*/);
void GraphWindEditMenu(Integer /*item*/, WindowPtr /*whichWindow*/);
void setGraphHelpItems(void);
void setGraphFileItems(Boolean /*active*/);
void setGraphEditItems(void);

/* macInput.c */
/* Note:  All TransSkel called routines now must be pascal */
void CmdWindInit(Boolean /*quiet*/);
pascal void CmdWindClobber(void);
pascal void CmdWindMain(void);
void closeBatch(long);
void insertCmd(char * /*command*/, Boolean /*replace*/);
Integer inputReady(void);
Integer DoStats(void);
pascal void CmdWindKey(Integer /*ch*/, Integer /*code*/, Integer /*mods*/);
unsigned char nonTextKey(unsigned char /*ch*/, unsigned char /*code*/, Integer /*mods*/);
void ScrollToInsertPt(void);
Integer GetCmdChL(Integer /*n*/);
Integer GetCmdIL(void);
pascal void CmdWindClose(void);
pascal void CmdWindZoom(Integer /*partCode*/, WindowPeek /*theWindow*/);
pascal void CmdWindActivate(Boolean /*active*/);
void clearUndoInfo(void);
void CmdWindEditMenu(Integer /*item*/);
void setCmdHelpItems(void);
void setCmdFileItems(Boolean /*infront*/);
void setCmdEditItems(void);
void DoScroll(Integer /*lDelta*/);
pascal void TrackScroll(ControlHandle /*theScroll*/, Integer /*partCode*/);
pascal Boolean doClikLoop(void);
pascal Boolean wordBreak(Ptr /*text*/, Integer /*charPos*/);
pascal void CmdWindMouse(Point /*thePt*/, LongInt /*when*/, Integer /*mods*/);
pascal void CmdWindUpdate(Boolean /*resized*/);
void CmdWindPrint(Integer /*windno*/);

/* macMain.c */
/* Note:  All TransSkel called routines now must be pascal */
void main(void);
Boolean SetUpMenus(void);
pascal void DoApple(Integer /*item*/);
pascal void DoFile(Integer /*item*/);
pascal void DoWind(Integer /*item*/);
pascal void DoCommand(Integer /*item*/);
pascal void DoSuspendResume(Boolean /*inForeground*/);
pascal void DoOptions(Integer /*item*/);
pascal void DoFont(Integer /*item*/);
pascal void DoFontSize(Integer /*item*/);
pascal void DoEdit(Integer /*item*/);
void DrawGrowBox(WindowPtr /*wind*/);
void SetWindClip(WindowPtr /*wind*/);
void ResetWindClip(void);
Integer quitIt(void);
Boolean doBatch(char * /*fileName*/, int /*echo*/);
void doSave(cmdWindowInfoPtr /*wp*/, Integer /*item*/);
Integer loadWindow(char * /*fileName*/, Integer /*vRefNum*/, Boolean /*new*/);
pascal void aboutUserItem(WindowPtr, Integer);
pascal void editCommandUserItem(WindowPtr, Integer);
pascal Boolean aboutDialogFilter(DialogPtr, EventRecord *, Integer *); 

/* macFindFile.c */
/* formerly macOpen.c and macOpen() */
char *macFindFile(char */*fileName*/, Str255 /*message*/,
				  Str255 /*defaultname*/, Integer /*readWrite*/,
				  Integer /*ntypes*/, OSType * /*types*/,
				  Integer * /*volume*/);
void macSetInfo(Integer /*vRefNum*/, char * /*fName*/, OSType /*type*/,
				OSType /*creator*/);

/* macPrint.c */
void PictWindPrint(Integer /*windno*/);
void TextWindPrint(Integer /*windno*/);

/* macHandle.c */
void collectGarbage(long /*keepLocked*/);
void **myFindHandle(void ** /*h*/);
void **myNewHandle(long /*n*/, long /*keepLocked*/);
void myDisposHandle(void ** /*name*/);
void **mySetHandleSize(void ** /*name*/, long /*newn*/, long /*keepLocked*/);
void **myDupHandle(void ** /*name*/, long /*keepLocked*/);
long myValidHandle(void ** /*h*/);
long myGetHandleSize(void ** /*h*/);
void myHLock(void ** /*h*/);
void myHUnlock(void ** /*h*/);
void *myNewPtr(long /*n*/);
void myDisposePtr(void * /*p*/);
void setWarehouseLimits(long /*standard*/);
long myInitHandles(short /* maxhandles */);
void myDispAll(void);
void memoryUsage(char * /*where*/);

/* macIo.c */
void incrementNLINES(char * /*msg*/);
FILE *fmyopen(char * /*fileName*/, char * /*mode*/);
void fmyprint(char * /*msg*/, FILE * /*fp*/);
void fmyeol(FILE * /*fp*/);
void fmyflush(FILE * /*fp*/);
void mybeep(long /*n*/);
int fillFromDialog(void);
void putprompt(char * /*prompt*/);
#ifdef SAVEHISTORY
void initHistory(long /*historyLength*/);
void saveInHistory(char ** /*lineH*/);
char * recallHistory(unsigned char /*ch*/, Integer /*mods*/);
long getSomeHistory(long /*nlines*/, char *** /*historyHandle*/);
void setSomeHistory(long /*nlines*/, char ** /*lines*/);
#endif /*SAVEHISTORY*/
char *expandFilename(char * /*fname*/); 
long okfilename(char * /*fname*/);
long okpathname(char * /*fname*/);
long isfilename(char * /*fname*/);
pascal void consoleUserItem(WindowPtr, Integer);


/* macBatchMode.c */
void BatchModeInit(Boolean /*quiet*/);
pascal void BatchModeMain(void);
pascal void BatchDialogEvent (Integer /*itemHit*/, EventRecord * /*theEvent*/);

/* macUtils.c */
void toMyScrap(void);
LongInt fromMyScrap(void);
Integer nVisible(void);
Integer whichCmdWindow(WindowPtr /*theWindow*/);
Integer whichGraphWindow(WindowPtr /*theWindow*/);
void clearWindowInfo(Integer /*windNo*/);
void saveWindowInfo(Integer /*windNo*/);
void restoreWindowInfo(Integer /*windNo*/);
void setCommandM(Integer /*windno*/);
void adjustLeading(Integer /*fontNumber*/, Integer /*fontSize*/, FontInfo */*aFont*/);
void getScreenRect(Rect * /*r*/);
Integer createWindow(Str255 /*wTitle*/);
void setFrontOnly(Integer /*windno*/, Boolean /*frontOnly*/);
void clearUndoInfo(void);
void decorateFontMenus(Integer /*fontNumber*/, Integer /*fontSize*/);
void setCmdWindFont(Integer /*fontNumber*/, Integer /*fontSize*/);
void macSetNewFont(char * /*newFontName*/, short /*newFontSize*/);
void onFatalError(void);
void scanKeyboard(void);
Boolean isKeyPressed (Integer /*keyCode*/);
Integer optionKeyToNumber(unsigned char /*ch*/);
Integer functionKeyToNumber(Integer /*code*/);
void setCmdScreen(void);
void DeactivateWindow(void);
pascal void ActivateWindow(Boolean /*active*/);
void macUpdate(WindowPtr /*wind*/);
void MyShowWindow(WindowPtr /*wind*/);
void fileErrorMsg(Integer /*errorFlag*/, char * /*fileName*/);
char * macGetPath(char * /*fullPath*/, Integer /*volume*/,
				  LongInt /*directory*/);
Integer doPageSetup(Integer /*doit*/);
void myAlert(char * /*msgs*/);
Integer saveAlert(Str255 /*fileName*/, Boolean /*cmdwind*/);
Boolean quitAlert(char /*ch*/);
Boolean twoChoiceAlert(char * /*label1*/, char * /*label2*/, char * /*keys*/, char * /*msgs*/);
pascal Boolean myDialogFilter (DialogPtr /*theDialog*/, EventRecord * /*theEvent*/,
							   Integer * /*itemHit*/);
DialogPtr getDlog(Integer /*id*/, Boolean /*dialogEdit*/, Integer /*nButtons*/,
			 char  * /*buttonCh*/);
void setDlogNumber(DialogPtr /*theDialog*/, Integer /*item*/, double /*value*/,
				   char * /*fmt*/, Boolean /*select*/);
void setDlogItemValues(DialogPtr /*theDialog*/, Integer /*item*/,
						Integer /*nitems*/, Integer /*value*/);
Integer getDlogItemValues(DialogPtr /*theDialog*/, Integer /*item*/, Integer /*nitems*/);
pascal void DialogClobber(void);
pascal void outlineOK(WindowPtr /*theWindow*/, Integer /*itemNo*/);
LongInt scrapSize(void);
int myputscrap(char ** /*text*/);
char ** mygetscrap(void);
Integer str255cmp(Str255 /*str1*/, Str255 /*str2*/);
void str255cpy(Str255 /*str1*/, Str255 /*str2*/);
void myDebug(char * /*msgs*/);

/*handlers.c*/
void recordTicks(void);

#endif /*MACPROTOH__*/
