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

#ifdef __GNUG__
#pragma implementation
#endif

/*
  970912 Changes made to correct problems in Motif graph printing
  981124 Added prototypes for WX_* functions.
  		 Change to WX_graphics() to draw dot just outside right frame
         to force inclusion of right frame in clipping region on a copy.
         It's an ugly kluge but it seems to work
  990225 If changing the size of the window would reduc a canvas dimension
         to 0, 1 is used instead.  This should fix the crashing that occurred
         when the uses shrinks the graphic window too far.
*/

// For compilers that support precompilation, includes "wx.h".
#include "wx_prec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx.h"
#endif

#ifdef wx_motif
#include <X11/Xlib.h>
#include <X11/keysym.h>
#endif



#include <math.h>
extern "C" {
#include "../globals.h"
#include "../plot.h"
#include "wxIface.h"
void myAlert(char *msg);
}
#if (0)
#undef ALERT
#define ALERT(F,A,B,C,D)
#undef FPRINT
#define FPRINT(F,A,B,C,D)
#endif /*1*/

#include "wxgraph.h"
#include "wx_clipb.h"
#include "wx_mf.h"

#include <ctype.h>
#include "wx_timer.h"
#include "wx_mf.h"
#include "wxmain.h"

#if defined(wx_msw) && !defined(__WATCOMC__)
#include "mmsystem.h"
#endif

static unsigned long WX_currentx,WX_currenty,WX_ymax,WX_textv,WX_texth;
extern struct        termentry term_tbl[];
wxFont              *CanvasFont = NULL;



// Define a constructor for my canvas
MacAnovaCanvas::MacAnovaCanvas(wxFrame *frame, int x, int y, int w, int h, long style):
 wxCanvas(frame, x, y, w, h, style)
{
#ifdef wx_motif
	Widget          graphWidget = (Widget) handle;

	static char MacAnovaTranslationTable[] =
	  "Ctrl Mod1 <Key>q: DoFastQuit()\n\
Ctrl <Key>1: DoFindGraph()\n\
Ctrl <Key>2: DoFindGraph()\n\
Ctrl <Key>3: DoFindGraph()\n\
Ctrl <Key>4: DoFindGraph()\n\
Ctrl <Key>5: DoFindGraph()\n\
Ctrl <Key>6: DoFindGraph()\n\
Ctrl <Key>7: DoFindGraph()\n\
Ctrl <Key>8: DoFindGraph()\n\
Ctrl <Key>F1: DoFindGraph()\n\
Ctrl <Key>F2: DoFindGraph()\n\
Ctrl <Key>F3: DoFindGraph()\n\
Ctrl <Key>F4: DoFindGraph()\n\
Ctrl <Key>F5: DoFindGraph()\n\
Ctrl <Key>F6: DoFindGraph()\n\
Ctrl <Key>F7: DoFindGraph()\n\
Ctrl <Key>F8: DoFindGraph()\n\
Ctrl <Key>i: DoKeyInterrupt()";

	static XtActionsRec actions[] =
	{
		{"DoKeyInterrupt", (XtActionProc) DoKeyInterrupt},
		{"DoFastQuit", (XtActionProc) DoFastQuit},
		{"DoFindGraph", (XtActionProc) DoFindGraph}
	};

	XtTranslations MacAnovaTranslations;

	XtAppAddActions (wxTheApp->appContext, actions,
					 sizeof(actions)/sizeof(XtActionsRec));
	MacAnovaTranslations = XtParseTranslationTable(MacAnovaTranslationTable);
	XtOverrideTranslations(graphWidget, MacAnovaTranslations);
	XtFree ((char *) MacAnovaTranslations);

	// set any required resources
	Arg     args[10];
	int     n = 0;
	
	// suppress bell sounding on un-processed character
	XtSetArg(args[n], XmNverifyBell, FALSE);
	n++;
#if(0)
	XtSetArg(args[n],XmNbackground, 0);
	n++;
	XtSetArg(args[n],XmNforeground, 1);
	n++;
#endif /*0*/
	XtSetValues(graphWidget, args, n);

#endif /*wx_motif*/

	JustCreated = TRUE;
} /*MacAnovaCanvas::MacAnovaCanvas()*/

MacAnovaCanvas::~MacAnovaCanvas(void)
{
} /*MacAnovaCanvas::~MacAnovaCanvas()*/

// Define the repainting behaviour
void MacAnovaCanvas::OnPaint(void)
{
	MacAnovaCanvasFrame     *parent;

	parent = (MacAnovaCanvasFrame *) GetParent();
	parent->Draw(*(parent->canvasDC));

/*970915 added test for new member JustCreated*/

	if (JustCreated)
	{
		wxYield();
		SetFocus();
		JustCreated = FALSE;
	}

} /*MacAnovaCanvas::OnPaint()*/

void MacAnovaCanvas::OnChar(wxKeyEvent& event)
{
	long        ch = event.keyCode;

#ifdef wx_msw
	if (GetKeyState(VK_CONTROL) & 0x80)
	{
		if (ch >= WXK_F1 && ch < WXK_F1 + NGRAPHS)
		{
			BaseFrame->FindGraphFrame(ch - WXK_F1);
			return;
		} /*if (ch >= WXK_F1 && ch < WXK_F1 + NGRAPHS)*/
		if (ch == 0x09)
		{
			BaseFrame->KeyInterrupt();
			return;
		} /*if (ch == 0x09)*/
		if (ch == 'p' || ch == 0x10) /*Ctrl P*/
		{
			MacAnovaCanvasFrame     *parent =
				(MacAnovaCanvasFrame *) GetParent();
			parent->OnMenuCommand(MACANOVA_GRAPHICS_PRINT);
			return;
		} /*if (ch == 0x10)*/
	} /*if (GetKeyState(VK_CONTROL) & 0x80)*/
#endif /*wx_msw*/
#ifdef wx_motif
	if (ch >= WXK_F1 && ch < WXK_F1 + NGRAPHS && event.controlDown)
	{
		BaseFrame->FindGraphFrame(ch - WXK_F1);
		return;
	}
#endif /*wx_motif*/
	if (ch != WXK_RETURN)
	{
		if (ch != WXK_CONTROL)
		{
			mybeep(1);
		}
		
		return;
	} /*if (ch != WXK_RETURN)*/

	if(!BaseFrame->GraphicsPause)
	{
		BaseFrame->FindTextFrame(BaseFrame->RunningFrame);
	}
	else
	{
		BaseFrame->GraphicsPause = 0;
	}
} /*MacAnovaCanvas::OnChar()*/

void MacAnovaCanvasFrame::OnChar(wxKeyEvent& event)
{
	long          ch = event.keyCode;

	if (ch != WXK_RETURN)
	{
		if (ch != WXK_CONTROL)
		{
			mybeep(1);
		}
		return;
	} /*if (ch != WXK_RETURN)*/

	if(!BaseFrame->GraphicsPause)
	{
		BaseFrame->FindTextFrame(BaseFrame->RunningFrame);
	}
	else
	{
		BaseFrame->GraphicsPause = 0;
	}
} /*MacAnovaCanvasFrame::OnChar()*/

#ifdef wx_motif
/*
  970910 added to fix focus problems in Motif version
         If the canvas frame is in the process of being created or
		 being shown when hidden, a call to this SetFocus must
         be preceded by some sort of wait loop.  This would be
         unnecessary if there were a quick way to determin whether
         calling XSetInputFocus() was safe
*/
void MacAnovaCanvasFrame::SetFocus(void)
{
	Display         *display = XtDisplay((Widget) handle);
	Window           thisWindow = XtWindow((Widget) handle);

	/*
	  following is an unsuccessful attempt to determine if it is
	  safe to call XSetInputFocus()
	*/
#if (0)
	int              count = 100000;
	
	while(count-- > 0 && !XmIsTraversable((Widget) canvas->handle))
	{
		;
	}
	char       msg[50];
	sprintf(msg,"count = %d",count);
	if (GUBED & 4)
		myAlert(msg);
#endif /*0*/
#if (0)
	/*
	  Debugging code to allow tuning of focus delay
	  981219 FocusDelay now tunable by command line flags -plotdelay n
             or setoptions(plotdelat:n)
	  A call to waitForTicks is in MacAnovaBaseFrame::FindGraphFrame()
      in wxmain.cc
	*/
	Symbolhandle    symhDelay = Lookup("FOCUSDELAY");

	if (symhDelay && isInteger(symhDelay, POSITIVEVALUE))
	{
		FocusDelay = (int) DATAVALUE(symhDelay, 0);
	}
	wxYield();
	waitForTicks(FocusDelay);
#endif /*0*/


	XSetInputFocus(display,thisWindow,RevertToPointerRoot,CurrentTime);
} /*MacAnovaCanvasFrame::SetFocus()*/
#endif /*wx_motif*/



#define PICTUREMINALLOC 2000

// constructor for CanvasFrame
MacAnovaCanvasFrame::MacAnovaCanvasFrame(wxFrame *frame,
										 Const char *title, int x, int y,
										 int w, int h) :
	wxFrame(frame,title,x,y,
			(canvas = (MacAnovaCanvas *) 0, OrigWidth = w), OrigHeight = h)
{
	WHERE("MacAnovaCanvasFrame");

#if (0)
/* 
   The following superceded by setting OrigWidth and OrigHeight in
   call to wxFrame.  This means they should be set before any call
   back to MacAnovaCanvasFrame::OnSize() occurs, which is where they
   are used
   */
// 970121 Added following two lines; seems to fix various things
	OrigHeight = abs(h);
	OrigWidth = abs(w);
	// 970125 part of kludge
	DefaultCanvasWindowWidth = abs(DefaultCanvasWindowWidth);
	DefaultCanvasWindowHeight = abs(DefaultCanvasWindowHeight);

	canvas = (MacAnovaCanvas *) 0;
#endif /*0*/
	Picture = new char[PICTUREMINALLOC];

	PictureAllocation = PICTUREMINALLOC;
	SetMenuBar(SetupMenuBar());
	ResetCanvas();
} /*MacAnovaCanvasFrame::MacAnovaCanvasFrame()*/

void MacAnovaCanvasFrame::ResetWindowsMenu(void)
{
	int      i;

	// loop through all frames and set the window name
	// menu labels to the text frame titles.  Set extra
	// menu labels to empty
	for (i=0;i<BaseFrame->numTextFrames;i++)
	{
		WindowNamesMenu->SetLabel(10*MACANOVA_WINDOWS_TEXTWINDOW+i,
								  BaseFrame->TextFrames[i]->GetTitle() );
	}
	for (i=BaseFrame->numTextFrames;i<NUMTEXTFRAMES;i++)
	{
		WindowNamesMenu->SetLabel(10*MACANOVA_WINDOWS_TEXTWINDOW+i,"");
	}
	for (i=0;i<NUMCANVASFRAMES;i++)
	{
		if (BaseFrame->CanvasFrames[i] == NULL)
		{
			CanvasNamesMenu->Enable(10*MACANOVA_WINDOWS_GRAPH+i,FALSE);
		}
		else
		{
			CanvasNamesMenu->Enable(10*MACANOVA_WINDOWS_GRAPH+i,TRUE);
		}
	} /*for (i=0;i<NUMCANVASFRAMES;i++)*/
} /*MacAnovaCanvasFrame::ResetWindowsMenu()*/


wxMenuBar * MacAnovaCanvasFrame::SetupMenuBar(void)
{
	int        i;
	char       buffer1[80],buffer2[80];
	
	wxMenu *graphics_menu = new wxMenu;

	graphics_menu->Append(MACANOVA_FILE_INTERRUPT,
						  "&Interrupt\tCtrl+I", "Interrupt Processing");
	graphics_menu->Append(MACANOVA_FILE_GOON, "Go On", "Resume processing");
	graphics_menu->AppendSeparator();
#if (0)
	graphics_menu->Append(MACANOVA_GRAPHICS_PAGESETUP,"Page Setup",
						  "Setup the printer");
#endif /*0*/
#ifdef wx_msw
	graphics_menu->Append(MACANOVA_GRAPHICS_PRINT,
						  "&Print Graph\tCtrl+P","Print this graphics window");
#endif // wx_msw
#ifdef wx_motif
	graphics_menu->Append(MACANOVA_GRAPHICS_EPSPRINT,
						  "&Print Graph\tCtrl+P",
						  "Print this graph to a Postscript printer/file");
#endif // wx_motif
	graphics_menu->AppendSeparator();
	graphics_menu->Append(MACANOVA_FILE_QUIT,
						  "&Quit\tCtrl+Q",   "Quit MacAnova");

	wxMenu          *edit_menu = new wxMenu;

	edit_menu->Append(MACANOVA_EDIT_COPY,
					  "Copy\tCtrl+C","Copy Graph to Clipboard");

	wxMenu          *windownames_menu = new wxMenu;

	for (i=0;i<NUMTEXTFRAMES;i++)
	{
		windownames_menu->Append(10*MACANOVA_WINDOWS_TEXTWINDOW+i,
								 "","Bring window forward");
	} /*for (i=0;i<NUMTEXTFRAMES;i++)*/

	WindowNamesMenu = windownames_menu;

	wxMenu          *graphicswindows_menu = new wxMenu;

	for (i=0;i<NUMCANVASFRAMES;i++)
	{
		sprintf(buffer1, "Graph %d\tCtrl+F%d", i+1, i+1);
		sprintf(buffer2,"Bring Graph %d forward",i+1);
		graphicswindows_menu->Append(10*MACANOVA_WINDOWS_GRAPH+i,
									 buffer1,buffer2);
	} /*for (i=0;i<NUMCANVASFRAMES;i++)*/

	CanvasNamesMenu = graphicswindows_menu;

	wxMenu          *windows_menu = new wxMenu;

	windows_menu->Append(MACANOVA_WINDOWS_HIDE,"&Hide","Hide window");
	windows_menu->Append(MACANOVA_WINDOWS_CLOSE,"&Close\tCtrl+W","Close window");
	windows_menu->AppendSeparator();
	windows_menu->Append(MACANOVA_WINDOWS_GRAPH,
						 "&Graphs",graphicswindows_menu,"Select graph");
	windows_menu->AppendSeparator();
	windows_menu->Append(MACANOVA_WINDOWS_TEXTWINDOW,
						 "&Windows",windownames_menu,"Select window");

	wxMenu          *help_menu = new wxMenu;

	help_menu->Append(MACANOVA_HELP_ABOUT, "&About",  "About MacAnova 4.0");
	help_menu->Append(MACANOVA_HELP_HELP, "&Help",    "Help on MacAnova 4.0");

	wxMenuBar       *theMenuBar = new wxMenuBar;

	theMenuBar->Append(graphics_menu, "&File");
#ifdef wx_msw
	theMenuBar->Append(edit_menu, "&Edit");
#endif // wx_msw
	theMenuBar->Append(windows_menu, "&Windows");
	theMenuBar->Append(help_menu, "&Help");

	LoadAccelerators("menus_accel");
	return(theMenuBar);
} /*MacAnovaCanvasFrame::SetupMenuBar()*/


// Intercept menu commands
#define SCALINGFUZZ   .001
void MacAnovaCanvasFrame::OnMenuCommand(int id)
{
	WHERE("MacAnovaCanvasFrame::OnMenuCommand");

	The_timer->Stop();

	// process show window menu commands
	if (id >= 10*MACANOVA_WINDOWS_TEXTWINDOW &&
		id < 10*MACANOVA_WINDOWS_TEXTWINDOW + NUMTEXTFRAMES)
	{
		id = id - 10*MACANOVA_WINDOWS_TEXTWINDOW;
		if (id < BaseFrame->numTextFrames)
		{
			BaseFrame->FindTextFrame(id);
		}
		if ( !BaseFrame->Running && BDEPTH <= 0)
		{
			The_timer->Start(MACANOVATIMERINCREMENT);
		}
		return;
	}
	
	// process show graph menu commands
	if (id >= 10*MACANOVA_WINDOWS_GRAPH &&
		id < 10*MACANOVA_WINDOWS_GRAPH + NUMCANVASFRAMES)
	{
		id = id - 10*MACANOVA_WINDOWS_GRAPH;
		BaseFrame->FindGraphFrame(id);
		if ( !BaseFrame->Running && BDEPTH <= 0)
		{
			The_timer->Start(MACANOVATIMERINCREMENT);
		}
		return;
	}
	
	switch (id)
	{
	  case MACANOVA_FILE_INTERRUPT:
		{
			BaseFrame->KeyInterrupt();
			break;
		}
	  
		
	  case MACANOVA_FILE_GOON:
		BaseFrame->GraphicsPause = 0;
		break;
		
	  case MACANOVA_FILE_QUIT:
		BaseFrame->DoQuit(CANCANCEL);
		break;
		
#ifdef wx_motif
	  case MACANOVA_GRAPHICS_EPSPRINT:
		{
			printSetupData       *setupData = &BaseFrame->GraphPrintSetupData;
			int                   orientation = setupData->orientation;
			
			wxSetPrinterOrientation(orientation);
			wxSetPrinterScaling(setupData->scaleX[orientation],
								setupData->scaleY[orientation]);
			wxSetPrinterTranslation(setupData->translateX[orientation],
									setupData->translateY[orientation]);
			wxSetPrinterMode(setupData->mode);

#if (0)
			wxPostScriptDC dc(NULL, TRUE);
#endif /*0*/
			MacAnovaPostScriptDC dc(NULL, TRUE);

			if (dc.Ok())
			{
				orientation = wxGetPrinterOrientation();
				setupData->orientation = orientation;
				wxGetPrinterScaling(&setupData->scaleX[orientation],
									&setupData->scaleY[orientation]);
				wxGetPrinterTranslation(&setupData->translateX[orientation],
										&setupData->translateY[orientation]);
				setupData->mode = wxGetPrinterMode();

				if (dc.StartDoc("Printing graph"))
				{
					dc.StartPage();
					// last param required for cl386
					Draw(dc);
					dc.EndPage();
					dc.EndDoc();
				} /*if (dc.StartDoc("Printing graph"))*/
			} /*if (dc.Ok())*/
			break;
		} /*case MACANOVA_GRAPHICS_EPSPRINT*/

#endif							// wx_motif
		
#ifdef wx_msw
	  case MACANOVA_GRAPHICS_PRINT:
		{
			myApp.SetPrintMode(wxPRINT_WINDOWS);
			wxPrinter printer;
			MacAnovaGPrintout printout("My printout",this);
			printer.Print(this, &printout, TRUE);
			break;
		}
	  
#if (0)
		wxPrinterDC dc(NULL, NULL, NULL);
		if (dc.Ok())
		{
			if (dc.StartDoc("Printing graph"))
			{
				dc.StartPage();
				// last param required for cl386
				Draw(dc);
				dc.EndPage();
				dc.EndDoc();
			}
		}
		break;
#endif /*0*/
		
	  case MACANOVA_EDIT_COPY:
		{
			wxMetaFileDC     dc;

			if (dc.Ok())
			{
#if (0)
				float      maxX, maxY;

				maxX = OrigWidth + 2*10; // use 10 device unit margins
				maxY = OrigHeight + 2*10; // use 10 device unit margins
#endif /*0*/
				/*
				  981105 GO added following two lines as part of fix
				  of pasting of postage stamp size graph.
				*/
				::SetWindowOrgEx(dc.cdc,0,0,NULL);
                ::SetWindowExtEx(dc.cdc,OrigWidth+5,OrigHeight,NULL);

				dc.SetMapMode(MM_TEXT);
				Draw(dc);
				wxMetaFile *mf = dc.Close();
				if (mf)
				{
#if (0)
					Bool success = mf->SetClipboard((int)(dc.MaxX() + 1),
													(int)(dc.MaxY() + 1));
#endif /*0*/
					Bool success = mf->SetClipboard(OrigWidth, OrigHeight);
					delete mf;
				} /*if (mf)*/

			} /*if (dc.Ok())*/
			break;
		}
	  
#endif // wx_msw
		
	  case MACANOVA_WINDOWS_HIDE:
		Show(FALSE);
		break;
		
	  case MACANOVA_WINDOWS_CLOSE:
		OnClose();
		delete [] Picture;
		delete this;
		break;
		
	  case MACANOVA_HELP_ABOUT:
		putAboutBox();
		break;
		
	  case MACANOVA_HELP_HELP:
		putHelpBox();
		break;
		
	  default:
		putUnimplementedBox();
		break;
	} /*switch (id)*/
	
	if ( !BaseFrame->Running && BDEPTH <= 0)
	{
		The_timer->Start(MACANOVATIMERINCREMENT);
	}
} /*MacAnovaCanvasFrame::OnMenuCommand()*/

#if (0)
//initialization now done in base frame constructer
extern "C" {
	/* called from initialize()*/
#ifdef wx_motif
void initializePSPrinter(void)
{
	wxSetPrinterScaling(LANDSCAPESCALING,LANDSCAPESCALING);
	wxSetPrinterTranslation(LANDSCAPETRANSLATIONX,LANDSCAPETRANSLATIONY);
	wxSetPrinterOrientation(PS_LANDSCAPE);
	wxSetPrinterMode(PS_PRINTER);
}
#endif /*wx_motif*/

}
#endif /*0*/

unsigned short MacAnovaCanvasFrame::Picture2Short(long index)
{
	unsigned short       result1,result;

	index = index*2;
	result = (unsigned char) Picture[index];
	result1 = (unsigned char) Picture[index+1];
	result = 256*result + result1;
	return(result);
} /*MacAnovaCanvasFrame::Picture2Short()*/

void MacAnovaCanvasFrame::Short2Picture(unsigned short x)
{
	char          c, *newp;
	int           i;

	if (PictureLength + 4 > PictureAllocation)
	{
		newp = new char[2*PictureAllocation];
		if (!newp)
		{
			return;
		}
		for (i=0;i<PictureLength;i++)
		{
			newp[i] = Picture[i];
		}
		delete [] Picture;
		Picture = newp;
		PictureAllocation = 2*PictureAllocation;
	} /*if (PictureLength + 4 > PictureAllocation)*/
	c = x >> 8;
	Picture[PictureLength++] =c;
	c = x & PICTOpEndPic;
	Picture[PictureLength++] =c;
} /*MacAnovaCanvasFrame::Short2Picture()*/

void MacAnovaCanvasFrame::ResetCanvas(void)
{
	// Clear the picture here and reset size
	int         width, height;
	WHERE("ResetCanvas");
	GetClientSize(&width,&height);
	OrigWidth = width;
	OrigHeight = height;
	PictureLength = 0;
	// get new canvas with this size
	if (canvas)
	{
		delete canvas;
	}
	width = (width <= 0) ? 1 : width;
	height = (height <= 0) ? 1 : height;

	canvas = new MacAnovaCanvas(this,0,0,width,height,wxRETAINED);

	canvas->SetBackground(wxWHITE_BRUSH);
	canvasDC = canvas->GetDC();
	if (CanvasFont == NULL)
	{
		CanvasFont = wxTheFontList->FindOrCreateFont(DefaultCanvasFontSize,
												 wxMODERN, wxNORMAL, wxNORMAL);
	}
	canvasDC->SetFont(CanvasFont);
	canvasDC->SetBackgroundMode(wxTRANSPARENT);
	Show(TRUE);

	// write out Picture header info
	Short2Picture(0);
	Short2Picture(0);
	Short2Picture(OrigHeight);
	Short2Picture(OrigWidth);
	Short2Picture(PICTVersionOp); // VersionOp opcode
	Short2Picture(PICTVersion); // version opcode
	Short2Picture(PICTHeaderOp); // headerop ocode
	Short2Picture(PICTFENCE);
	Short2Picture(PICTFENCE);
	Short2Picture(0);Short2Picture(0); // fixed pt bounding rect
	Short2Picture(0);Short2Picture(0); // fixed pt bounding rect
	Short2Picture(OrigWidth);Short2Picture(0); // fixed pt bounding rect
	Short2Picture(OrigHeight);Short2Picture(0); // fixed pt bounding rect
	Short2Picture(0);
	Short2Picture(0);
	Short2Picture(PICTDefHilite); // DefHilite opcode
	Short2Picture(PICTClip); // Clip opcode
	Short2Picture(0x000a); // 10 bytes for region
	Short2Picture(0);
	Short2Picture(0);
	Short2Picture(OrigHeight);
	Short2Picture(OrigWidth);
} /*MacAnovaCanvasFrame::ResetCanvas()*/

void MacAnovaCanvasFrame::OnSize(int w, int h)
{
	// Clear the picture here and reset size
	int           width, height;
	float         wscale, hscale;
	WHERE("MacAnovaCanvasFrame::OnSize");

	GetClientSize(&width,&height);

	// get new canvas with this size
	if (canvas)
	{
		delete canvas;
	}

	width = (width <= 0) ? 1 : width;
	height = (height <= 0) ? 1 : height;
	canvas = new MacAnovaCanvas(this,0,0,width,height,wxRETAINED);
	canvasDC = canvas->GetDC();
	if (CanvasFont == NULL)
	{
		CanvasFont = wxTheFontList->FindOrCreateFont(DefaultCanvasFontSize,
													 wxMODERN, wxNORMAL,
													 wxNORMAL);
	}
	canvasDC->SetFont(CanvasFont);
#if (0)
/*part of superceded kludge; see MacAnovaCanvasFrame::MacAnovaCanvasFrame()*/
	if (DefaultCanvasWindowWidth < 0)
	{
		DefaultCanvasWindowWidth = -DefaultCanvasWindowWidth;
		OrigWidth = DefaultCanvasWindowWidth;
	}
	if (DefaultCanvasWindowHeight < 0)
	{
		DefaultCanvasWindowHeight = -DefaultCanvasWindowHeight;
		OrigHeight = DefaultCanvasWindowHeight;
	}
#endif /*0*/
	wscale = (float) width/OrigWidth;
	hscale = (float) height/OrigHeight;
	canvasDC->SetUserScale(wscale,hscale);
} /*MacAnovaCanvasFrame::OnSize()*/


/*
	According to WX documentation, OnActivate is called only in
	Windows version.  The SetFocus() here is essential if a graph
	window that is switched to have keyboard focus
*/
void MacAnovaCanvasFrame::OnActivate(Bool arg)
{
	if (arg && canvas)
	{
		canvas->SetFocus();
	}
} /*MacAnovaCanvasFrame::OnActivate*/

Bool MacAnovaCanvasFrame::OnClose(void)
{
	BaseFrame->GraphicsPause = 0;
	BaseFrame->CutCanvasFrame(CanvasFrameIndex);
	return(TRUE);
} /*MacAnovaCanvasFrame::OnClose()*/


void MacAnovaCanvasFrame::Draw(wxDC& dc)
{
#if (0)
  	dc.SetPen(wxGREEN_PEN);
  	dc.DrawLine(0.0, 0.0, 200.0, 200.0);
  	dc.DrawLine(200.0, 0.0, 0.0, 200.0);
#endif /*0*/
	// skip first 52 bytes (26 shorts), all header
	long         opcode,picIndex = 26,color,length,i;
	float        x1,y1,x2,y2;
	char        *string;
	long         linewidth,linetype,commenttype;
	wxPen        *thisPen;
	
    if (canvas == (MacAnovaCanvas *) 0)
    {
    	return;
    }
	if (CanvasFont == NULL)
	{
		CanvasFont = wxTheFontList->FindOrCreateFont(DefaultCanvasFontSize,
													 wxMODERN, wxNORMAL,
													 wxNORMAL);
	}
	dc.SetFont(CanvasFont);
	dc.SetBackgroundMode(wxTRANSPARENT);
	while (picIndex < (PictureLength/2) &&
		   (opcode = Picture2Short(picIndex++)) )
		// PICTOpEndPic is close picture
	{
		switch(opcode)
		{
		  case PICTFgColor: // set foreground color
			color = 256*Picture2Short(picIndex++) +
				Picture2Short(picIndex++);
			switch(color)
			{
			  case MacWhiteColor:
				dc.SetPen(wxWHITE_PEN);
				break;
			  case MacBlackColor:
				dc.SetPen(wxBLACK_PEN);
				break;
			  default:
				break;
			}
			break;
			
		  case PICTBgColor: // set background color
			color = 256*Picture2Short(picIndex++) +
				Picture2Short(picIndex++);
			switch(color)
			{
			  case MacWhiteColor:
				dc.SetBackground(wxWHITE_BRUSH);
				break;
			  case MacBlackColor:
				dc.SetBackground(wxBLACK_BRUSH);
				break;
			  default:
				break;
			}
#ifdef wx_motif
/*
	added for Motif to ensure background color takes hold
	It broke copying under Windows
*/
			dc.Clear();
#endif /*wx_motif*/
			break;
			
		  case PICTLine: // line opcode
			y1 = Picture2Short(picIndex++);
			x1 = Picture2Short(picIndex++);
			y2 = Picture2Short(picIndex++);
			x2 = Picture2Short(picIndex++);
			dc.DrawLine(x1,y1,x2,y2);
			break;
			
		  case PICTLongText: // text opcode
			y1 = Picture2Short(picIndex++);
			x1 = Picture2Short(picIndex++);
			length = Picture2Short(picIndex++);
			string = new char[length+1];
			for (i=0;i<length;i++)
			{
				string[i] = Picture2Short(picIndex++);
			}
			string[length] = 0;
			dc.DrawText(string,x1,y1-WX_textv/1.3);
			delete [] string;
			break;
			
		  case PICTeraseRect: // erase rectangle
			y1 = Picture2Short(picIndex++);
			x1 = Picture2Short(picIndex++);
			y2 = Picture2Short(picIndex++);
			x2 = Picture2Short(picIndex++);
			//dc.Clear(); // note, we actually ignore rectangle
			break;
			
		  case PICTLongComment: // long comment, used for lines
			commenttype = Picture2Short(picIndex++);
			if (commenttype == 100)		// line stuff
			{
				commenttype = Picture2Short(picIndex++); // length is 4
				linewidth = Picture2Short(picIndex++);
				linetype = Picture2Short(picIndex++);
				switch(linetype)
				{
				  case 2:
				  case 3:
				  case 4:
				  case 5:
					linetype = wxSOLID+linetype-1;
					break;
				  case 1:
				  default:
					linetype = wxSOLID;
					break;
				}
				thisPen = wxThePenList->FindOrCreatePen("BLACK",linewidth,linetype);
				dc.SetPen(thisPen);
			}
			break;
			
			
		  default:
			;
		} /*switch(opcode)*/		
	}	
} /*MacAnovaCanvasFrame::Draw()*/


/* Used to generate default file names */
static short           PlotFileCount[3] = {0, 0, 0};
static char           *Description[3][2] =
{
	{"Encapsulated PS", "epsf"},
	{"PostScript",      "ps"},
	{"PICT",            "pict"}
};

extern "C" {
/*
   wrapper for graphics usages of wxFindFile()
*/


char *findPlotFile(char *fileName, char *kind, long ps, long epsf,
				   long newFile)
{
	char        prompt[40], defaultName[20];
	char       *wxFileName;
	int         which = (epsf) ? 0 : ((ps) ? 1 : 2);

	if (fileName[0] == '\0')
	{
		if (PlotFileCount[which] == 0 || newFile)
		{
			PlotFileCount[which]++;
		}
		sprintf(prompt, "%s File...", Description[which][0]);
		sprintf(defaultName, "%s%02d.%s", kind, PlotFileCount[which],
				Description[which][1]);
	}
	else
	{
		prompt[0] = defaultName[0] = '\0';
	}

	wxFileName = wxFindFile(fileName, prompt, defaultName);
	if (wxFileName == (char *) 0)
	{ /* Canceled */
		if (PlotFileCount[which] == 1 || newFile)
		{
			PlotFileCount[which]--;
		}
	}
	return (wxFileName);
} /*findPlotFile()*/
	
} /*extern "C" */

extern "C" {

void Short2Picture(unsigned short x)
{
	MacAnovaCanvasFrame       *cf;

	cf = BaseFrame->CanvasFrames[ThisWindow];
	cf->Short2Picture(x);
} /*Short2Picture()*/

void           CanvasPause(void)
{
	while (BaseFrame->GraphicsPause)
	{
		wxYield();
	}
} /*CanvasPause()*/

/*
   Entries WX_init(), WX_graphics(), WX_text(), WX_reset(), WX_linetype(),
           WX_move(), WX_vector(), WX_str_text(), WX_chr_text(),
           WX_colors() for gnuplot use that would otherwise be in term.c
   981124 Added prototypes
*/
int            WX_init(void);
int            WX_graphics(void);
void           WX_text(void);
void           WX_reset(void);
void           WX_linetype(long);
void           WX_move(unsigned long, unsigned long);
void           WX_vector(unsigned long, unsigned long);
void           WX_str_text(char *);
void           WX_chr_text(char);
void           WX_colors(long, long);

int            WX_init(void)
{
	struct termentry       *t = term_tbl + term;
	MacAnovaCanvasFrame    *cf;

	if (BaseFrame->NewCanvasFrame() < 0)
	{
		return(1);
	}
	cf = BaseFrame->CanvasFrames[ThisWindow];

	cf->canvasDC->SetBackgroundMode(wxTRANSPARENT); // clear background for text

	t->v_char = (unsigned long) cf->canvasDC->GetCharHeight();
	t->h_char = (unsigned long) cf->canvasDC->GetCharWidth();
	t->v_tic = t->h_char;
	t->h_tic = t->h_char;
	t->xmax = cf->OrigWidth - t->h_tic;
	t->ymax = cf->OrigHeight - 1;

	WX_ymax = t->ymax;
	WX_textv = t->v_char;
	WX_texth = t->h_char;

	return(0);
} /*WX_init()*/
  
int            WX_graphics(void)
{
	MacAnovaCanvasFrame      *cf;
#ifdef wx_msw
	struct termentry       *t = term_tbl + term;
#endif /*wx_msw*/

	cf = BaseFrame->CanvasFrames[ThisWindow];
	Short2Picture(PICTBgColor); // set background color opcode
	Short2Picture(MacWhiteColorHigh);
	Short2Picture(MacWhiteColorLow); 
	Short2Picture(PICTFgColor); // set foreground color opcode
	Short2Picture(MacBlackColorHigh);
	Short2Picture(MacBlackColorLow);  
	Short2Picture(PICTeraseRect); // erase rectangle code
	Short2Picture(0); //y1
	Short2Picture(0); //x1
	Short2Picture(cf->OrigHeight); // y2
	Short2Picture(cf->OrigWidth); // x2

#ifdef wx_msw
 /*
 	981124 draw point just outside right frame to force inclusion of
           right frame boundary
 */
	WX_linetype((long) BORDERLINETYPE);
	WX_move(t->xmax+1, t->ymax/2);
	WX_vector(t->xmax+1, t->ymax/2);
#endif /*wx_msw*/

	return(0);
} /*WX_graphics()*/

void           WX_text(void)
{
	Short2Picture(PICTOpEndPic); // close picture

#ifdef wx_motif
	MacAnovaCanvasFrame       *cf;

	cf = BaseFrame->CanvasFrames[ThisWindow];
	BaseFrame->FindGraphFrame(cf->CanvasFrameIndex, TRUE);
#endif /*wx_motif*/

	if (PauseAfterPlot)
	{
		BaseFrame->GraphicsPause = 1;
		CanvasPause(); // wait till go on or return
	}
} /*WX_text()*/

void           WX_reset(void)
{
}

void           WX_linetype(long linetype)
{
	int        heavy = (10*linetype >= UNITWIDTH);
	double     thickness = (heavy) ?
		(double) (MAXLINETYPE * (linetype/MAXLINETYPE)) /
			(double) UNITWIDTH : 1.0;
	int        Thickness =  (int) ceil(thickness);

	Thickness = (Thickness > 10 ? 10 : Thickness); // just in case
	Short2Picture(PICTLongComment); // long picture comment
	Short2Picture(100); // kind of comment wx stuff
	Short2Picture(4); // length of comment
	Short2Picture(Thickness); // thickness

	if (heavy)
	{
		linetype %= MAXLINETYPE;
	}
	if (linetype < XAXISLINETYPE)
	{
		linetype = BORDERLINETYPE;
	}
	linetype = ((linetype < 0) ? linetype: linetype % 5) - XAXISLINETYPE;

	switch(linetype)
	{
	  case 0: // x-axis
	  case 1: // y-axis
	  case 6: // regular short dash
		Short2Picture(4);
		break;
	  case 4: // dotted
		Short2Picture(2);
		break;
	  case 5: // dotted
		Short2Picture(3);
		break;
	  case 7: // dotted
		Short2Picture(5);
		break;
	  case 2: // border
	  case 3: // regular solid
	  default:
		Short2Picture(1);
		break;
	} /*switch(linetype)*/


	//setLineScale(HAIRLINESCALE);
} /*WX_linetype()*/

void           WX_move(unsigned long x, unsigned long y)
{
	WX_currentx = x;
	WX_currenty = WX_ymax-y;
} /*WX_move()*/

void           WX_vector(unsigned long x, unsigned long y)
{
	Short2Picture(PICTLine); // line opcode
	Short2Picture(WX_currenty);
	Short2Picture(WX_currentx);
	Short2Picture(WX_ymax-y);
	Short2Picture(x);
	WX_currentx = x;
	WX_currenty = WX_ymax-y;
} /*WX_vector()*/

void           WX_str_text(char * str)
{
	int       i, stringlen;
	Short2Picture(PICTLongText); // text opcode
	Short2Picture(WX_currenty);
	Short2Picture(WX_currentx);
	stringlen = strlen(str);
	Short2Picture(stringlen);
	for (i=0;i<stringlen;i++)
	{
		Short2Picture((unsigned) str[i]);
	}
	WX_currentx += stringlen * WX_texth;
} /*WX_str_text()*/

void           WX_chr_text(char c)
{
	int         stringlen;

	Short2Picture(PICTLongText); // text opcode
	Short2Picture(WX_currenty);
	Short2Picture(WX_currentx);
	stringlen = 1;
	Short2Picture(stringlen);
	Short2Picture((unsigned) c);
	WX_currentx +=  WX_texth;
} /*WX_chr_text()*/

void           WX_colors(long option, long colorno)
{
	;
} /*WX_colors()*/
} /*extern "C" */
