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
  970912 Made changes to avoid Motif focus problems and modify menu
		 keyboard equivalents
  970916 Implemented Hide on the Windows menu using new functions
		 MacAnovaBaseFrame::EnableAllTextWindowMenus() and
		 MacAnovaBaseFrame::NVisible()
*/
#ifdef __GNUG__
#pragma implementation
#endif

// For compilers that support precompilation, includes "wx.h".
#include "wx_prec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx.h"
#endif

#include <math.h>
#include "wxIface.h"

extern "C" {
#include "../globals.h"
}

#ifdef wx_motif
#include <Xm/Text.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#endif

#include <ctype.h>
#include "wx_timer.h"
#include "wx_mf.h"
#include "wxmain.h"

#if defined(wx_msw) && !defined(__WATCOMC__)
#include "mmsystem.h"
#endif

long  FromMenu = 0;

#ifdef wx_x
#include "aiai.xbm"
#include "fload.xbm"
#endif

#ifndef FOCUSDELAY
#define FOCUSDELAY 1000
#endif

#if (0)
/*
	fossil; seems not to be referenced as another textWindowFont
	is a member of MacAnovaTextFrame
*/
wxFont   *textWindowFont;
#endif /*0*/

// Define my frame constructor
/*
	970127 Initialize text_window to 0 in call to wxFrame constructor
	980217 Initialize new elements JustCreated and MemoryError
*/

MacAnovaTextFrame::MacAnovaTextFrame(wxFrame *frame, Const char *title,
									 int x, int y, int w, int h ):
									 wxFrame(frame, title,
											 (text_window = NULL,
											  textWindowFont = NULL, x),
											 y, w, h)
{
	CreateStatusLine(2);
    // Load icon and bitmap

#if (0)
#ifdef wx_msw
	wxIcon        *test_icon = new wxIcon("aiai_icn");
	wxBitmap      *test_bitmap = new wxBitmap("fload") ;
#endif /*wx_msw*/
#endif /*0*/

#ifdef wx_x
	wxIcon        *test_icon = new wxIcon(aiai_bits, aiai_width, aiai_height);
#if (0)
	wxBitmap      *test_bitmap = new wxBitmap(fload_bits,
											  fload_width,fload_height) ;
#endif /*(0)*/

	SetIcon(test_icon);
#endif /*wx_x*/
	
	SetMenuBar(SetupMenuBar());
	
	// Make a text window
	text_window = new MacAnovaTextWindow(this, 0, 250, 400, 250,
										 wxNATIVE_IMPL | wxHSCROLL);
	textWindowFont = new wxFont(DefaultTextFontSize,
								wxMODERN, wxNORMAL, wxNORMAL);
	text_window->SetFont(textWindowFont);
	OnSize(DefaultTextWindowWidth,DefaultTextWindowHeight);
	JustCreated = TRUE;
	MemoryError = FALSE;

	Show(TRUE);
	//SetStatusText("Waiting");
	
} /*MacAnovaTextFrame::MacAnovaTextFrame()*/

MacAnovaTextFrame::~MacAnovaTextFrame(void)
{
	delete text_window;
	text_window = NULL;
	delete textWindowFont;
	textWindowFont = NULL;
}

#ifdef wx_motif
void MacAnovaTextFrame::SetFocus(void)
{
	Display       *display = XtDisplay((Widget) handle);
	Window         thisWindow = XtWindow((Widget) handle);

	XSetInputFocus(display,thisWindow,RevertToPointerRoot,CurrentTime);
}
#endif /*wx_motif*/

// Intercept menu commands
void MacAnovaTextFrame::OnMenuCommand(int id)
{
	WHERE("MacAnovaTextFrame::OnMenuCommand");
	The_timer->Stop();
	
	// process show window menu commands
	if(id >= 10*MACANOVA_WINDOWS_TEXTWINDOW &&
	   id < 10*MACANOVA_WINDOWS_TEXTWINDOW + NUMTEXTFRAMES)
	{
		id = id - 10*MACANOVA_WINDOWS_TEXTWINDOW;
		if(id < BaseFrame->numTextFrames)
		{
			BaseFrame->FindTextFrame(id);
		}
		if( !BaseFrame->Running && BDEPTH <= 0)
		{
			The_timer->Start(MACANOVATIMERINCREMENT);
		}
		return;
	}

	// process show graph menu commands
	if(id >= 10*MACANOVA_WINDOWS_GRAPH &&
	   id < 10*MACANOVA_WINDOWS_GRAPH + NUMCANVASFRAMES)
	{
		id = id - 10*MACANOVA_WINDOWS_GRAPH;
		BaseFrame->FindGraphFrame(id);
		if( !BaseFrame->Running && BDEPTH <= 0)
		{
			The_timer->Start(MACANOVATIMERINCREMENT);
		}
		return;
	}

#if (0)
	if(! BaseFrame->Running)
	{
		/* if we're not Running, then we don't have a Running{Frame,Window},
		   so we don't know where to write, if we do any writing
		   Thus we must set one up.
	   */
		BaseFrame->RunningFrame = TextFrameIndex;
		BaseFrame->RunningWindow = text_window;
	}
#endif /*0*/

	switch (id)
	{
	  case MACANOVA_FILE_OPEN:
		{
			// start a titled frame
			BaseFrame->NewTextFrame(FALSE);
			break;
		}
		
	  case MACANOVA_FILE_SAVEWINDOW:
		{
			SaveWindow();
			break;
		}
		
	  case MACANOVA_FILE_SAVEWINDOWAS:
		{
			SaveWindowAs();
			break;
		}
		
	  case MACANOVA_FILE_PRINTSELECTION:
	  case MACANOVA_FILE_PAGESETUP:
		{
#ifdef wx_msw
			myApp.SetPrintMode(wxPRINT_WINDOWS);
#else
			myApp.SetPrintMode(wxPRINT_POSTSCRIPT);
#endif

			/*
			  The MacAnovaTPrintout constructor appears to create a DC
			  and put up the basic dialog boxes;
			  hence current DC parameters such as orientation and
			  scaling need to be set here
			*/
#ifdef wx_motif
			printSetupData *setupData = &BaseFrame->TextPrintSetupData;

			int    orientation = setupData->orientation;
#if (0) // code to allow tuning of translation values
			Symbolhandle    symh = Lookup("TRANSLATEX");
			
			if (isInteger(symh, ANYVALUE))
			{
			
				setupData->translateX[orientation] = DATAVALUE(symh, 0);
			}
			symh = Lookup("TRANSLATEY");
			if (isInteger(symh, ANYVALUE))
			{
				setupData->translateY[orientation] = DATAVALUE(symh, 0);
			}
#endif /*0*/

			wxSetPrinterOrientation(orientation);
			wxSetPrinterScaling(setupData->scaleX[orientation],
								setupData->scaleY[orientation]);
			wxSetPrinterTranslation(setupData->translateX[orientation],
									setupData->translateY[orientation]);
			wxSetPrinterMode(setupData->mode);
#endif /*wx_motif*/
			if (id == MACANOVA_FILE_PRINTSELECTION)
			{
				wxPrinter printer;

				printer.GetPrintData().SetAllPages(TRUE);
				printer.GetPrintData().EnablePageNumbers(FALSE);
				MacAnovaTPrintout printout("My printout",text_window);
				printer.Print(this, &printout, TRUE);
			}
			else
			{
				wxPrintDialog printerDialog(this);
				printerDialog.GetPrintData().SetSetupDialog(TRUE);
				printerDialog.Show(TRUE);
			}
			
#ifdef wx_motif
			setupData->orientation = orientation = wxGetPrinterOrientation();
#if (0) //scaling and translation should not change
			wxGetPrinterScaling(&setupData->scaleX[orientation],
								&setupData->scaleY[orientation]);
			wxGetPrinterTranslation(&setupData->translateX[orientation],
									&setupData->translateY[orientation]);
#endif /*0*/
			setupData->mode = wxGetPrinterMode();
#endif /*wx_motif*/
			break;
			
		}    
		
	  case MACANOVA_FILE_INTERRUPT:
		{
			BaseFrame->KeyInterrupt();
			break;
		}

	  case MACANOVA_FILE_GOON:
		{
			BaseFrame->GraphicsPause = 0;
			break;
		}

		
	  case MACANOVA_FILE_SAVEWORKSPACE:
		{
			BaseFrame->SaveWorkspace(FALSE);
			break;
		}
		
	  case MACANOVA_FILE_SAVEWORKSPACEAS:
		{
			BaseFrame->SaveWorkspace(TRUE);
			break;
		}
		
	  case MACANOVA_FILE_RESTOREWORKSPACE:
		{
			BaseFrame->RestoreWorkspace((char *) 0);
			break;
		}
		
	  case MACANOVA_FILE_QUIT:
		{
			BaseFrame->DoQuit(CANCANCEL);
			break;
		}
		
	  case MACANOVA_EDIT_UNDO:
		{
			text_window->Undo();
			ResetEditMenu(text_window->UndoStatus);
			break;
		}
		
	  case MACANOVA_EDIT_CUT:
		{
			text_window->Cut();
			ResetEditMenu(text_window->UndoStatus);
			break;
		}
		
	  case MACANOVA_EDIT_COPY:
		{
			text_window->Copy();
			ResetEditMenu(text_window->UndoStatus);
			break;
		}
		
	  case MACANOVA_EDIT_PASTE:
		{
			text_window->Paste();
			ResetEditMenu(text_window->UndoStatus);
			break;
		}
		
	  case MACANOVA_EDIT_COPYTOEND:
		{
			text_window->CopyToEnd();
			ResetEditMenu(text_window->UndoStatus);
			break;
		}
		
	  case MACANOVA_EDIT_DUMPTEXT:
		{
			text_window->DumpText();
			break;
		}
		
	  case MACANOVA_EDIT_EXECUTE:
		{
			text_window->Execute();
			break;
		}
		
#ifdef SAVEHISTORY
	  case MACANOVA_EDIT_UPHISTORY:
	  case MACANOVA_EDIT_DOWNHISTORY:
		{
			text_window->UpDownHistory(id);
			break;
		}
#endif /*SAVEHISTORY*/

	  case MACANOVA_WINDOWS_HIDE:
		{
			int      nVisible = BaseFrame->NVisible();
			
			if (nVisible > 1)
			{
				Show(FALSE);
				BaseFrame->EnableAllTextWindowMenus(MACANOVA_WINDOWS_HIDE,
													BaseFrame->NVisible() > 1);
			}
			break;
		}
		
	  case MACANOVA_WINDOWS_CLOSE:
		{
			if (OnClose())
			{
				delete this;
				BaseFrame->EnableAllTextWindowMenus(MACANOVA_WINDOWS_HIDE,
													BaseFrame->NVisible() > 1);
			}
			break;
		}
	  case MACANOVA_WINDOWS_NEWWINDOW:
		{
			// start up an untitled frame
			BaseFrame->NewTextFrame(TRUE);
			break;
		}
		
	  case MACANOVA_WINDOWS_GOTOTOP:
		{
			text_window->ShowPosition(0);
			break;
		}
		
	  case MACANOVA_WINDOWS_GOTOEND:
		{
			long      lastPosition = text_window->GetLastPosition();
			
			text_window->SetInsertionPoint(lastPosition);
#ifdef wx_motif
			text_window->SetSelection(lastPosition,lastPosition);
#endif /*wx_motif*/
			break;
		}
		
	  case MACANOVA_WINDOWS_GOTOCOMMANDPOINT:
		{
			text_window->SetInsertionPoint(text_window->CmdStrPos);
#ifdef wx_motif
			text_window->SetSelection(text_window->CmdStrPos,text_window->CmdStrPos);
#endif /*wx_motif*/
			break;
		}
		
#ifdef wx_msw
	  case MACANOVA_WINDOWS_TEXTWFONT:
		{
			wxFontDialog *chooseFontDialog;
			chooseFontDialog = new wxFontDialog(this);
			if(chooseFontDialog->Show(TRUE))
			{
				((wxTextWindow *)text_window)->SetFont(chooseFontDialog->GetFontData().GetChosenFont());
                ((wxTextWindow *)text_window)->Refresh(TRUE);
			}
			delete chooseFontDialog;
			break;
		}
#endif /*wx_msw*/
	  case MACANOVA_HELP_ABOUT:
		{
			putAboutBox();
			break;
		}

	  case MACANOVA_HELP_HELP:
		{
			putHelpBox();
			break;
		}
		
	  default:
		{
			putUnimplementedBox();
			break;
		}
	} /*switch (id)*/
	
	if( !BaseFrame->Running && BDEPTH <=0 )
	{
		The_timer->Start(MACANOVATIMERINCREMENT);
	}
} /*MacAnovaTextFrame::OnMenuCommand()*/

wxMenuBar * MacAnovaTextFrame::SetupMenuBar(void)
{
	int        i;
	char       buffer1[80], buffer2[80];
	
	wxMenu *text_file_menu = new wxMenu;
	
	text_file_menu->Append(MACANOVA_FILE_OPEN,
						   "&Open\tCtrl+O", "Open a text file");
	text_file_menu->Append(MACANOVA_FILE_SAVEWINDOW,
						   "&Save Window\tCtrl+S", "Save the text window");
	text_file_menu->Append(MACANOVA_FILE_SAVEWINDOWAS,
						   "Save Window As ...",
						   "Rename and save the text window");
	text_file_menu->AppendSeparator();
	text_file_menu->Append(MACANOVA_FILE_PAGESETUP,"Page Setup",
						   "Set up the printer");
	text_file_menu->Append(MACANOVA_FILE_PRINTSELECTION,
						   "&Print Window\tCtrl+P",
						   "Print this command window");
	text_file_menu->AppendSeparator();
	text_file_menu->Append(MACANOVA_FILE_INTERRUPT,
						   "&Interrupt\tCtrl+I", "Interrupt processing");
#if (0)
	text_file_menu->Append(MACANOVA_FILE_GOON, "Go On", "Resume processing");
#endif /*(0)*/
	text_file_menu->AppendSeparator();
	text_file_menu->Append(MACANOVA_FILE_RESTOREWORKSPACE,
						   "&Restore Workspace\tCtrl+R",
						   "Load a workspace from a save file");
	text_file_menu->Append(MACANOVA_FILE_SAVEWORKSPACE,
						   "Save Wor&kspace\tCtrl+K",
						   "Save the workspace on a file");
	text_file_menu->Append(MACANOVA_FILE_SAVEWORKSPACEAS,
						   "Save Workspace As ...",
						   "Rename and save the workspace on a file");
	text_file_menu->AppendSeparator();
	text_file_menu->Append(MACANOVA_FILE_QUIT,
						   "&Quit\tCtrl+Q",   "Quit MacAnova");
	
	wxMenu *edit_menu = new wxMenu;
	
	edit_menu->Append(MACANOVA_EDIT_UNDO, "U&ndo\tCtrl+Z", "Undo last change");
	edit_menu->AppendSeparator();
	edit_menu->Append(MACANOVA_EDIT_CUT, "Cut\tCtrl+X", "Cut selection");
	edit_menu->Append(MACANOVA_EDIT_COPY, "Copy\tCtrl+C", "Copy selection");
	edit_menu->Append(MACANOVA_EDIT_PASTE, "Paste\tCtrl+V", "Paste selection");
	edit_menu->AppendSeparator();
	edit_menu->Append(MACANOVA_EDIT_COPYTOEND,
					  "&Copy to end\tF5", "Copy selection to end");
	edit_menu->Append(MACANOVA_EDIT_EXECUTE, "&Execute\tF6",
					  "Execute command line or selection");
#ifdef SAVEHISTORY
	edit_menu->AppendSeparator();
	edit_menu->Append(MACANOVA_EDIT_UPHISTORY, "&Up History\tF7",
					  "Insert &previous command line from history");
	edit_menu->Append(MACANOVA_EDIT_DOWNHISTORY, "&Down History\tF8",
					  "Insert next command line from history");
#endif /*SAVEHISTORY*/
	wxMenu *windownames_menu = new wxMenu;
	
	for (i = 0;i < NUMTEXTFRAMES;i++)
	{
		windownames_menu->Append(10*MACANOVA_WINDOWS_TEXTWINDOW+i,
								 "","Bring window forward");
	} /*for (i = 0;i < NUMTEXTFRAMES;i++)*/
	
	WindowNamesMenu = windownames_menu;
	
	wxMenu *graphicswindows_menu = new wxMenu;
	
	for (i=0;i<NUMCANVASFRAMES;i++)
	{
		sprintf(buffer1, "Graph %d\tCtrl+F%d", i+1, i+1);
		sprintf(buffer2,"Bring Graph %d forward",i+1);
		graphicswindows_menu->Append(10*MACANOVA_WINDOWS_GRAPH+i,
									 buffer1,buffer2);
	} /*for (i=0;i<NUMCANVASFRAMES;i++)*/
	
	CanvasNamesMenu = graphicswindows_menu;
	
	wxMenu *windows_menu = new wxMenu;

	windows_menu->Append(MACANOVA_WINDOWS_HIDE,"&Hide","Hide window");
	windows_menu->Append(MACANOVA_WINDOWS_CLOSE,"&Close\tCtrl+W","Close window");
	windows_menu->Append(MACANOVA_WINDOWS_NEWWINDOW,"&New\tCtrl+N","New window");
	windows_menu->AppendSeparator();
	windows_menu->Append(MACANOVA_WINDOWS_GRAPH,
						 "&Graphs",graphicswindows_menu,"Select graph");
	windows_menu->AppendSeparator();
	windows_menu->Append(MACANOVA_WINDOWS_TEXTWINDOW,
						 "&Windows",windownames_menu,"Select window");
#ifdef wx_msw /* doesn't work on Motif*/
	windows_menu->AppendSeparator();
	windows_menu->Append(MACANOVA_WINDOWS_TEXTWFONT,
						 "Set Font", "Set the font for this window");
#endif /*wx_msw*/
	windows_menu->AppendSeparator();
	windows_menu->Append(MACANOVA_WINDOWS_GOTOTOP,
						 "Scroll to &Top\tCtrl+T",
						 "Scroll to top of window");
	windows_menu->Append(MACANOVA_WINDOWS_GOTOEND,
						 "Go to &End\tCtrl+E",
						 "Move cursor to last position in window");
	windows_menu->Append(MACANOVA_WINDOWS_GOTOCOMMANDPOINT,
						 "Go to &Prompt\tCtrl+A",
						 "Move cursor right after prompt");
#if (0)
	windows_menu->Append(MACANOVA_WINDOWS_PAGEUP,
						 "Page &Up","Move one page up");
	windows_menu->Append(MACANOVA_WINDOWS_PAGEDOWN,
						 "Page &Down","Move one page down");
#endif /*(0)*/
	
	
	wxMenu *help_menu = new wxMenu;
	help_menu->Append(MACANOVA_HELP_ABOUT, "&About",  "About MacAnova 4.0");
	help_menu->Append(MACANOVA_HELP_HELP, "&Help",    "Help on MacAnova 4.0");
	
	wxMenuBar *theMenuBar = new wxMenuBar;
	
	theMenuBar->Append(text_file_menu, "&File");
	theMenuBar->Append(edit_menu, "&Edit");
	theMenuBar->Append(windows_menu, "&Windows");
	theMenuBar->Append(help_menu, "&Help");
	
	theMenuBar->Enable(MACANOVA_EDIT_UNDO,FALSE);
	//	theMenuBar->Enable(MACANOVA_WINDOWS_HIDE,FALSE);
	
	LoadAccelerators("menus_accel");
	return(theMenuBar);
} /*MacAnovaTextFrame::SetupMenuBar()*/

void MacAnovaTextFrame::ResetWindowsMenu(void)
{
	int         i;

	// loop through all text frames and set the window name
	// menu labels to the text frame titles.  Set extra
	// menu labels to empty
	for(i=0;i<BaseFrame->numTextFrames;i++)
	{
		WindowNamesMenu->SetLabel(10*MACANOVA_WINDOWS_TEXTWINDOW+i,
								  BaseFrame->TextFrames[i]->GetTitle() );
	}
	for(i=BaseFrame->numTextFrames;i<NUMTEXTFRAMES;i++)
	{
		WindowNamesMenu->SetLabel(10*MACANOVA_WINDOWS_TEXTWINDOW+i,"");
	}
	for(i=0;i<NUMCANVASFRAMES;i++)
	{
		if(BaseFrame->CanvasFrames[i] == NULL)
		{
			CanvasNamesMenu->Enable(10*MACANOVA_WINDOWS_GRAPH+i,FALSE);
		}
		else
		{
			CanvasNamesMenu->Enable(10*MACANOVA_WINDOWS_GRAPH+i,TRUE);
		}
	} /*for(i=0;i<NUMCANVASFRAMES;i++)*/
} /*MacAnovaTextFrame::ResetWindowsMenu()*/

void MacAnovaTextFrame::ResetEditMenu(long UndoStatus)
{
	
	wxMenuBar          *theMenuBar;
	
	theMenuBar = GetMenuBar();
	
	if(UndoStatus == CANTUNDO)
	{
		theMenuBar->SetLabel(MACANOVA_EDIT_UNDO,"Can't Undo");
		theMenuBar->SetHelpString(MACANOVA_EDIT_UNDO,"No Undo is possible");
	}
	else if(UndoStatus == OUTPUTUNDO)
	{
		theMenuBar->SetLabel(MACANOVA_EDIT_UNDO,"Undo Output\tCtrl+Z");
		theMenuBar->SetHelpString(MACANOVA_EDIT_UNDO,
								  "Remove latest MacAnova response");
	}
	else if(UndoStatus == CUTUNDO)
	{
		theMenuBar->SetLabel(MACANOVA_EDIT_UNDO,"Undo Cut\tCtrl+Z");
		theMenuBar->SetHelpString(MACANOVA_EDIT_UNDO,
								  "Undo last CUT operation");
	}
	else if(UndoStatus == PASTEUNDO)
	{
		theMenuBar->SetLabel(MACANOVA_EDIT_UNDO,"Undo Paste\tCtrl+Z");
		theMenuBar->SetHelpString(MACANOVA_EDIT_UNDO,
								  "Undo last PASTE operation");
	}
	else if(UndoStatus == TYPINGUNDO || UndoStatus == DELETEUNDO)
	{
		theMenuBar->SetLabel(MACANOVA_EDIT_UNDO,
							 "Undo Typing\tCtrl+Z");
		theMenuBar->SetHelpString(MACANOVA_EDIT_UNDO,
								  "Undo last Typing to the window");
	}
	else if(UndoStatus == OUTPUTREDO)
	{
		theMenuBar->SetLabel(MACANOVA_EDIT_UNDO,"Redo Output\tCtrl+Z");
		theMenuBar->SetHelpString(MACANOVA_EDIT_UNDO,"Put the output back");
	}
	else if(UndoStatus == TYPINGREDO || UndoStatus == DELETEREDO)
	{
		theMenuBar->SetLabel(MACANOVA_EDIT_UNDO,"Redo Typing\tCtrl+Z");
		theMenuBar->SetHelpString(MACANOVA_EDIT_UNDO,"Put the Typing back");
	}
	if(UndoStatus == CANTUNDO)
	{
		theMenuBar->Enable(MACANOVA_EDIT_UNDO, FALSE);
	}
	else
	{
		theMenuBar->Enable(MACANOVA_EDIT_UNDO, TRUE);
	}
	
} /*MacAnovaTextFrame::ResetEditMenu()*/


// Size the subwindows when the frame is resized
void MacAnovaTextFrame::OnSize(int w, int h)
{
	if ( text_window)
	{
		int      width, height;

		GetClientSize(&width, &height);
		text_window->SetSize(0, 0, width, height);
	}
} /*MacAnovaTextFrame::OnSize()*/

Bool MacAnovaTextFrame::SaveWindow(void)
{
	// return TRUE for a successful save, FALSE otherwise
	// do a save as if untitled, otherwise a straight write
	// to the file in the frame title
	if(Untitled)
	{
		return(SaveWindowAs());
	}
	if( !(text_window->SaveFile(GetTitle())) )
	{
		(void) wxMessageBox("File Save Unsuccessful", "ERROR!", wxOK|wxCENTRE);
		return FALSE;
	}
	return TRUE;
} /*MacAnovaTextFrame::SaveWindow()*/

Bool MacAnovaTextFrame::SaveWindowAs(void)
{
	// return TRUE for a successful save, FALSE otherwise
	// prompt for a file name and save to that file
    WHERE("MacAnovaTextFrame::SaveWindowAs");
	char       *s = wxFileSelector("File for output window",
								   NULL, NULL, NULL, "*");

	if (!s)						// pressed cancel
	{
		return FALSE;
	}

	if( !(text_window->SaveFile(s)) )
	{
		(void) wxMessageBox("File Save Unsuccessful", "ERROR!", wxOK|wxCENTRE);
		return FALSE;
	}
 	// housekeeping after a frame gets a new name
	Untitled = FALSE;
	SetTitle(s);
	BaseFrame->ResetAllWindowsMenus();
	return TRUE;
} /*MacAnovaTextFrame::SaveWindowAs()*/


/*
  Returns TRUE if it is OK to close this frame.  It's OK if the file is
  titled and unmodified or if the user doesn't wish to save the file or if
  the file is successfully saved.  Cancels and unsuccessful saves return
  false.
*/
Bool MacAnovaTextFrame::OKToClose(void)
{
	int          decision;
	char         message[400];

	if (Untitled || text_window->Modified())
	{
		sprintf(message,"Do you wish to save the window\n'%s'?", GetTitle());
		decision = wxMessageBox(message,"Save Window?",
								wxYES_NO | wxCANCEL | wxCENTRE);
		if( decision == wxCANCEL )
		{
			return FALSE;
		}
		if( decision == wxYES )
		{
			if(!SaveWindow())
			{
				return FALSE;
			}
		}
	} /*if (Untitled || text_window->Modified())*/
	return (TRUE);
} /*MacAnovaTextFrame::OKToClose()*/

void MacAnovaTextFrame::OnActivate(Bool arg)
{
	if (arg)
	{
		text_window->SetFocus();
		BaseFrame->RunningFrame = TextFrameIndex;
		BaseFrame->RunningWindow = text_window;
	}
} /*MacAnovaTextFrame::OnActivate*/

/*
  If we hit the close button, check to see if OKtoclose
  (that saves if necessary).  If OK, cut the frame from
  the main list (frame will be deleted by other mechanisms)
  If this is the last text frame, do a baseframe quit as well.
*/
Bool MacAnovaTextFrame::OnClose(void)
{
	The_timer->Stop();

#ifdef wx_msw
	if (GetKeyState(VK_MENU) & 0x80)
	{
		SaveOnClose = FALSE;
	}
#endif /*wx_msw*/

	if (BaseFrame->numTextFrames == 1)
	{
		if (!SaveOnClose)
		{
			SaveOnQuit = FALSE;
		}
		BaseFrame->DoQuit(CANCANCEL);
		The_timer->Start(MACANOVATIMERINCREMENT);
		return FALSE; // if we get to here, we cancelled
	} /*if (BaseFrame->numTextFrames == 1)*/


	if(SaveOnClose && !OKToClose() )
	{
		return FALSE;
	}
	SaveOnClose = TRUE;

	BaseFrame->CutTextFrame(TextFrameIndex);
	Show(FALSE);
	
#if (0) /* now done by destructors */
	delete text_window;
	text_window = NULL;

	delete textWindowFont;
	textWindowFont = NULL;
#endif /*1*/

	if (BaseFrame->NVisible() <= 0)
	{
		/* everything invisible; display first window */
		BaseFrame->FindTextFrame(0);
	}
	// don't want timer to hit too soon
	The_timer->Start(MACANOVATIMERINCREMENT);
	return (TRUE);
	
} /*MacAnovaTextFrame::OnClose()*/

void MacAnovaTextFrame::OnSetFocus(void)
{
	BaseFrame->RunningFrame = TextFrameIndex;
	BaseFrame->RunningWindow = text_window;
} /*MacAnovaTextFrame::OnSetFocus()*/

