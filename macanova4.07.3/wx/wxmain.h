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

/*
  970910 added member function SetFocus() to MacAnovaTextFrame and
         MacAnovaCanvasFrame as part of changes to correct focus problems
  970917 added MacAnovaBaseFrame::NVisible()
  970918 added MacAnovaBaseFrame::KeyInterrupt()
               MacAnovaBaseFrame::FindGraphFrame()
               MacAnovaBaseFrame::FindTextFrame()
  971001 added class MacAnovaPostScriptDC
  971117 added destructors for MacAnovaTextFrame, MacAnovaTextWindow
  980119 added OverflowTextFrame() to MacAnovaBaseFrame
  980216 added JustCreated and MemoryError to MacAnovaTextFrame
  981218 added plotDelay to MacAnovaCommandLineArgs (Motif only)
*/
#ifndef WXMAIN_H
#ifdef __GNUG__
#pragma interface
#endif

#if 0
#include "wxstring.h"
#endif
#include "wx_timer.h"
#include "wx_mf.h"
#include "wx_print.h"

#ifndef ENVBUFLENGTH
#define ENVBUFLENGTH  512
#endif /*ENVBUFLENGTH*/

/*
  980403 added components commandPrompt and batchPrompt
*/
class MacAnovaCommandLineArgs
{
  public:
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
#ifdef wx_motif
	long             plotDelay;
#endif /*wx_motif*/
#ifdef SAVEHISTORY
	long             historyLength;
#endif /*SAVEHISTORY*/
	long             quiet;
	MacAnovaCommandLineArgs(void);
}; /*class MacAnovaCommandLineArgs*/



// Define a new application
class MacAnovaApp: public wxApp
{
  public:
	MacAnovaApp(void) ;
	wxFrame                 *OnInit(void);
	MacAnovaCommandLineArgs *ProcessArgs(void);
}; /*class MacAnovaApp: public wxApp*/

// Define a new frame
class MacAnovaTextFrame;
class MacAnovaTextWindow;
class MacAnovaCanvasFrame;
class MacAnovaCanvas;
struct printSetupData
{
  int      orientation;
  float    scaleX[2];
  float    scaleY[2];
  float    translateX[2];
  float    translateY[2];
  // 1 = Preview, 2 = print to file, 3 = send to printer
  int      mode;
}; /*struct printSetupData*/

typedef struct printSetupData printSetupData;

class MacAnovaBaseFrame: public wxFrame
{
  public:
	wxPanel *panel;
	MacAnovaTextFrame            *TextFrames[NUMTEXTFRAMES];
	int                           numTextFrames;
	int                           untitled_sequence;
	MacAnovaCanvasFrame          *CanvasFrames[NUMCANVASFRAMES];
	int                           numCanvasFrames;
	Bool                          Running;
	int	                          RunningFrame;
	MacAnovaTextWindow           *RunningWindow;
#ifdef wx_motif
	printSetupData                TextPrintSetupData;
	printSetupData                GraphPrintSetupData;
#endif /*wx_motif*/
	void                          GetRunningWindowParms(long * cmdStrPos,
														long * textLength);
	int                           GetC(long iplace, int *done);
	int                           GraphicsPause;
	MacAnovaBaseFrame(wxFrame *frame, Const char *title,
					  int x, int y, int w, int h);
	~MacAnovaBaseFrame(void);
#ifdef wx_motif
	void defaultTextPrintSetupData(int orientation);
	void defaultGraphPrintSetupData(int orientation);
#endif /*wx_motif*/
	void NewTextFrame(Bool untitled, Bool initialPrompt = TRUE);
	void CutTextFrame(int frame_index);
	void OverflowTextFrame(int outOfMemory, long cutLength);
	int  NewCanvasFrame(void);
	void CutCanvasFrame(int frame_index);
	void ResetAllWindowsMenus(void);
	void EnableAllTextWindowMenus(int Id, Bool Flag);
	void DoQuit(int mustCancel);
	Bool OnClose(void);
	void OnActivate(Bool);
	int CheckForInput(void);
	void ProcessTypeAhead(void);
	void CloseBatch(int all);
	int  DoStats(void);
	void PutPrompt(char * prompt);
	void PutPrompt(char * prompt, long frameIndex);
	void lineToWindow(char *msg, long msgLength);
	void fmyeol(FILE *fp);
	Bool SaveWorkspace(Bool doSaveAs);
	Bool RestoreWorkspace(char * fileName);
	void FindGraphFrame(int windno, Bool mustwait = FALSE);
	void FindTextFrame(int windno, Bool mustwait = FALSE);
	void KeyInterrupt(void);
	int NVisible(void);
}; /*class MacAnovaBaseFrame: public wxFrame*/


class MacAnovaTextFrame: public wxFrame
{
  public:
	MacAnovaTextWindow   *text_window;
	wxFont               *textWindowFont;
	int                   TextFrameIndex;
	Bool                  Untitled;
	wxMenu               *WindowNamesMenu;
	wxMenu               *CanvasNamesMenu;
	MacAnovaTextFrame(wxFrame *frame, Const char *title, int x, int y, int w, int h);
	~MacAnovaTextFrame(void);
	void       OnSize(int w, int h);
	Bool       OKToClose(void);
	Bool       OnClose(void);
	void       OnMenuCommand(int id);
	void       ResetWindowsMenu(void);
	void       ResetEditMenu(long undoStatus);
	Bool       SaveWindow(void);
	Bool       SaveWindowAs(void);
#ifdef wx_motif
	void       SetFocus(void);
#endif /*wx_motif*/
	wxMenuBar *SetupMenuBar(void);
	void       OnActivate(Bool arg);
	void       OnSetFocus(void);
	Bool       JustCreated;
	Bool       MemoryError;
}; /*class MacAnovaTextFrame: public wxFrame*/

class MacAnovaTextWindow: public wxTextWindow
{
  public:
	MacAnovaTextWindow(wxFrame *frame, int x, int y, int width, int height, long style);
	~MacAnovaTextWindow(void);

	void         OnChar(wxKeyEvent& event);
	void         Copy(void);
	void         Cut(void);
	void         Undo(void);
	void         Paste(void);
#ifdef wx_msw
	void         Remove(long, long);
#endif /*wx_msw*/
    Bool         SaveFile(char *); //replacement for wxwin SaveFile
    Bool         LoadFile(char *);  //and for wxwin LoadFile
	void         GetSelectionPosition(long *, long*);
	char        *GetSelection(long from, long to);
	long         KludgeForBackSpaceChar(wxKeyEvent&);
	long         GetClipLength(void);
	void         Execute(void);
	void         CopyToEnd(void);
#ifdef SAVEHISTORY
	void         UpDownHistory(int);
	void         setCurrentCommand(char *);
#endif /*SAVEHISTORY*/
	long         CmdStrPos;
	long         CmdLine;
	long         CmdInsertLine;
	long         ControlActionStatus;
	long         OldCmdStrPos;
	long         LastCmdStrPos;
	long         LastTypingPos;
	long         LastLength;
	long         PasteLength;
	int          UndoStatus;
	long         UndoPlace;
	char        *UndoBuffer;
	long         UndoBufferAllocation;
	long         typeAhead[NAHEAD];
	long         LastChar;
	long         Nahead;
	long         selFrom;
	long         selTo;
#ifdef wx_motif
	long         timeStamp;
#endif //wx_motif
	void         PutPrompt(char * prompt);
	void         ToUndoBuffer(void);
	long         FromUndoBuffer(void);
	Bool       inputReady(void);
	void       clearUndoInfo(void);
	void       DumpText(void);
	void       matchParen(char ch);
	long       GetInsertionLine(void)
	  {
		  long lineno,charno;
		  PositionToXY(GetInsertionPoint(),&lineno,&charno);
		  return(lineno);
	  }
	int        GetLineText(long lineNo, char *buf, int bufLen);
    int        CheckForError(void);
}; /*class MacAnovaTextWindow: public wxTextWindow*/

class MacAnovaMainTimer: public wxTimer
{
  public:
	void Notify(void);
}; /*class MacAnovaMainTimer: public wxTimer*/


class MacAnovaCanvasFrame: public wxFrame
{
  public:
	MacAnovaCanvas *canvas;
	wxCanvasDC     *canvasDC;
	int             CanvasFrameIndex;
	wxMenu         *WindowNamesMenu;
	wxMenu         *CanvasNamesMenu;
	int             OrigWidth;
	int             OrigHeight;
	char           *Picture;
	long            PictureLength;
	long            PictureAllocation;
	MacAnovaCanvasFrame(wxFrame *frame, Const char *title, int x, int y,
						int w, int h);
	void            OnSize(int w, int h);
	Bool            OnClose(void);
	void            OnMenuCommand(int id);
	void            Draw(wxDC& dc);
	void            OnActivate(Bool);
	void            ResetCanvas(void           );
	void            ResetWindowsMenu(void           );
	void            Short2Picture(unsigned short data);
	unsigned short  Picture2Short(long index);
#ifdef wx_motif
	void            SetFocus(void);
#endif /*wx_motif*/
	wxMenuBar      *SetupMenuBar(void);
	void            OnChar(wxKeyEvent& event);
}; /*class MacAnovaCanvasFrame: public wxFrame*/

// Define a new canvas which can receive some events
class MacAnovaCanvas: public wxCanvas
{
	Bool            JustCreated;
  public:
	MacAnovaCanvas(wxFrame *frame, int x, int y, int w, int h,
				   long style = wxRETAINED);
	~MacAnovaCanvas(void) ;
	void OnPaint(void);
	void OnChar(wxKeyEvent& event);
}; /*class MacAnovaCanvas: public wxCanvas*/


class MacAnovaTPrintout: public wxPrintout
{
  public:
	MacAnovaTextWindow *PrintingWindow;
	long                PageHeight;
	long                LineHeight;
	long                TopMargin;
	long                LeftMargin;
	long                TotalLines;
	long                LinesPerPage;
	long                TotalPages;
	float               OverallScale;
	MacAnovaTPrintout(char *title, MacAnovaTextWindow *window);
	Bool                OnPrintPage(int page);
	Bool                HasPage(int page);
	Bool                OnBeginDocument(int startPage, int endPage);
	void GetPageInfo(int *minPage, int *maxPage, int *pageFrom, int *pageTo);
}; /*class MacAnovaTPrintout: public wxPrintout*/

class MacAnovaGPrintout: public wxPrintout
{
  public:
	MacAnovaCanvasFrame *PrintingCFrame;
	long                 PageHeight;
	long                 TopMargin;
	long                 LeftMargin;
	float                OverallScale;
	MacAnovaGPrintout(char *title,
					  MacAnovaCanvasFrame *frame);
	Bool                 OnPrintPage(int page);
	Bool                 HasPage(int page);
	Bool                 OnBeginDocument(int startPage, int endPage);
	void                 GetPageInfo(int *minPage, int *maxPage, int *pageFrom,
									 int *pageTo);
}; /*class MacAnovaGPrintout: public wxPrintout*/

#ifdef wx_motif
class MacAnovaPostScriptDC: public wxPostScriptDC
{
  public:
	MacAnovaPostScriptDC(void);
	MacAnovaPostScriptDC(char *output, Bool interactive = TRUE,
						 wxWindow *parent = NULL);
	~MacAnovaPostScriptDC(void);
	PrinterDialog(wxWindow *parent);
	Bool Create(char *output, Bool interactive = TRUE,
				wxWindow *parent = NULL);
}; /*class MacAnovaPostScriptDC*/
#endif /*wx_motif*/

#define wxLEADING                            0

#define MACANOVA_FILE_OPEN                 100
#define MACANOVA_FILE_SAVEWINDOW           101
#define MACANOVA_FILE_SAVEWINDOWAS         102
#define MACANOVA_FILE_PAGESETUP            103
#define MACANOVA_FILE_PRINTSELECTION       104
#define MACANOVA_FILE_INTERRUPT            105
#define MACANOVA_FILE_GOON                 106
#define MACANOVA_FILE_RESTOREWORKSPACE     107
#define MACANOVA_FILE_SAVEWORKSPACE        108
#define MACANOVA_FILE_SAVEWORKSPACEAS      109
#define MACANOVA_FILE_QUIT                 110

#define MACANOVA_EDIT_UNDO                 120
#define MACANOVA_EDIT_CUT                  121
#define MACANOVA_EDIT_COPY                 122
#define MACANOVA_EDIT_PASTE                123
#define MACANOVA_EDIT_COPYTOEND            124
#define MACANOVA_EDIT_DUMPTEXT             125
#define MACANOVA_EDIT_EXECUTE              126
#ifdef SAVEHISTORY
#define MACANOVA_EDIT_UPHISTORY            127
#define MACANOVA_EDIT_DOWNHISTORY          128
#endif /*SAVEHISTORY*/
#define MACANOVA_WINDOWS_HIDE              130
#define MACANOVA_WINDOWS_CLOSE             131
#define MACANOVA_WINDOWS_NEWWINDOW         132
#define MACANOVA_WINDOWS_GRAPH             133
#define MACANOVA_WINDOWS_GOTOTOP           134
#define MACANOVA_WINDOWS_GOTOEND           135
#define MACANOVA_WINDOWS_PAGEUP            136
#define MACANOVA_WINDOWS_PAGEDOWN          137
#define MACANOVA_WINDOWS_TEXTWINDOW        138
#define MACANOVA_WINDOWS_TEXTWFONT         139
#define MACANOVA_WINDOWS_GOTOCOMMANDPOINT  140

#define MACANOVA_WINDOWS_GRAPH1           1331
#define MACANOVA_WINDOWS_GRAPH2           1332
#define MACANOVA_WINDOWS_GRAPH3           1333
#define MACANOVA_WINDOWS_GRAPH4           1334
#define MACANOVA_WINDOWS_GRAPH5           1335
#define MACANOVA_WINDOWS_GRAPH6           1336
#define MACANOVA_WINDOWS_GRAPH7           1337
#define MACANOVA_WINDOWS_GRAPH8           1338
#define MACANOVA_WINDOWS_PANEL1           1339
#define MACANOVA_WINDOWS_PANEL2           1340

#define MACANOVA_WINDOWS_TEXTWINDOW0      1380
#define MACANOVA_WINDOWS_TEXTWINDOW1      1381
#define MACANOVA_WINDOWS_TEXTWINDOW2      1382
#define MACANOVA_WINDOWS_TEXTWINDOW3      1383
#define MACANOVA_WINDOWS_TEXTWINDOW4      1384
#define MACANOVA_WINDOWS_TEXTWINDOW5      1385
#define MACANOVA_WINDOWS_TEXTWINDOW6      1386
#define MACANOVA_WINDOWS_TEXTWINDOW7      1387
#define MACANOVA_WINDOWS_TEXTWINDOW8      1388
#define MACANOVA_WINDOWS_TEXTWINDOW9      1389

#define MACANOVA_COMMAND_0                 150
#define MACANOVA_COMMAND_1                 151
#define MACANOVA_COMMAND_2                 152
#define MACANOVA_COMMAND_3                 153
#define MACANOVA_COMMAND_4                 154
#define MACANOVA_COMMAND_5                 155
#define MACANOVA_COMMAND_6                 156
#define MACANOVA_COMMAND_7                 157
#define MACANOVA_COMMAND_8                 158
#define MACANOVA_COMMAND_9                 159

#define MACANOVA_OPTIONS_DIGITS            160


#define MACANOVA_FONT_WHATDOIDO            170

#define MACANOVA_HELP_ABOUT                200
#define MACANOVA_HELP_HELP                 201

#define MACANOVA_GRAPHICS_PAGESETUP        300
#define MACANOVA_GRAPHICS_PRINT            301
#define MACANOVA_GRAPHICS_HIDE             302
#define MACANOVA_GRAPHICS_CLOSE            303
#define MACANOVA_GRAPHICS_EPSPRINT         304

extern MacAnovaBaseFrame *BaseFrame;

extern MacAnovaApp        myApp;

extern MacAnovaMainTimer *The_timer;

#endif /*WXMAIN_H*/
