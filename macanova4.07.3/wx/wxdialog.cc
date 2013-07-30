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

#ifdef __GNUG__
#pragma implementation
#endif

/*
  Implements any special dialog boxes
  971001 Special Page Set Up dialog box for graphic printing
         Modified from stuff in WxWin files wb_ps.cc
*/

// For compilers that support precompilation, includes "wx.h".
#include "wx_prec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx.h"
#endif

// 980403 added include globals.h and math.h;
#include <math.h>
extern "C"
{
#include "../globals.h"
}

#include "wxIface.h"
#include "wxmain.h"
#ifdef wx_motif
Bool MacAnovaXPrinterDialog (wxWindow * parent); //prototype

MacAnovaPostScriptDC::MacAnovaPostScriptDC (void) : wxPostScriptDC()
{
}

MacAnovaPostScriptDC::MacAnovaPostScriptDC (char *output, Bool interactive,
	wxWindow *parent)
{
	Create(output, interactive ,parent);
} /*MacAnovaPostScriptDC::MacAnovaPostScriptDC()*/

/*
  980402 removed reference to wxTYPE_DC_POSTSCRIPT unless
         WX165 is defined
*/
Bool MacAnovaPostScriptDC::Create(char *file, Bool interactive,
								  wxWindow *parent)
{
#ifdef WX165
	__type = wxTYPE_DC_POSTSCRIPT;
#endif /*WX165*/
	wx_interactive = interactive;
	font = NULL;
	device = wxDEVICE_EPS;
	clipping = FALSE;

	logical_origin_x = 0;
	logical_origin_y = 0;

	device_origin_x = 0;
	device_origin_y = 0;

	logical_scale_x = 1.0;
	logical_scale_y = 1.0;

	user_scale_x = 1.0;
	user_scale_y = 1.0;

	mapping_mode = MM_TEXT;

	yorigin = 792;		// For EPS output

	min_x = 1000.0;
	min_y = 1000.0;
	max_x = -1000.0;
	max_y = -1000.0;
	title = NULL;
	if (file)
	{
		filename = copystring (file);
	}
	else
	{
		filename = NULL;
	}
	
	pstream = NULL;

#ifdef wx_msw
	// Can only send to file in Windows
	wxThePrintSetupData->SetPrinterMode(PS_FILE);
#endif

	if (interactive)
	{
		if ((ok = PrinterDialog (parent) ) == FALSE)
		{
			return FALSE;
		}
	}
	else
	{
		ok = TRUE;
	}
	

	currentRed = 255;
	currentGreen = 255;
	currentBlue = 0;

	current_logical_function = -1;
	current_pen = NULL;
	current_brush = NULL;
	current_background_brush = wxWHITE_BRUSH;
	current_text_foreground = *wxBLACK;
	Colour = wxColourDisplay ();

	return ok;
}

MacAnovaPostScriptDC::~MacAnovaPostScriptDC (void)
{
	if (pstream)
	{
		delete pstream;
	}
	if (filename)
	{
		delete[] filename;
	}
} /*MacAnovaPostScriptDC::~MacAnovaPostScriptDC()*/


Bool MacAnovaPostScriptDC::PrinterDialog(wxWindow *parent)
{
	ok = MacAnovaXPrinterDialog (parent);
	if (!ok)
	{
		return FALSE;
	}

	if (!filename &&
		(wxThePrintSetupData->GetPrinterMode() == PS_PREVIEW  ||
		 wxThePrintSetupData->GetPrinterMode() == PS_PRINTER))
	{
		// steve, 05.09.94
#ifdef VMS
		wxThePrintSetupData->SetPrinterFile("preview");
#else
		// For PS_PRINTER action this depends on a Unix-style print spooler
		// since the wx_printer_file can be destroyed during a session
		// @@@ TODO: a Windows-style answer for non-Unix
		char         userId[256];
		wxGetUserId (userId, sizeof (userId) / sizeof (char));
		char         tmp[256];
		strcpy (tmp, "/tmp/preview_");
		strcat (tmp, userId);
		wxThePrintSetupData->SetPrinterFile(tmp);
#endif
		char         tmp2[256];
		strcpy(tmp2, wxThePrintSetupData->GetPrinterFile());
		strcat (tmp2, ".ps");
		wxThePrintSetupData->SetPrinterFile(tmp2);
		filename = copystring(tmp2);
    }
    else if (!filename && (wxThePrintSetupData->GetPrinterMode() == PS_FILE))
    {
		char *file = wxSaveFileSelector ("PostScript", "ps");
		if (!file)
		{
			ok = FALSE;
			return FALSE;
		}
		wxThePrintSetupData->SetPrinterFile(file);
		filename = copystring(file);
		ok = TRUE;
    }

	return ok;
} /*MacAnovaPostScriptDC::PrinterDialog()*/


class wxPrinterDialogBox:public wxDialogBox
{
  public:
  wxPrinterDialogBox (wxWindow *parent, char *title, Bool modal = FALSE,
		      int x = -1, int y = -1, int
		      width = -1, int height = -1);
};

extern Bool wxPrinterDialogAnswer;

extern void
wxPrinterDialogOk (wxButton & button, wxEvent & WXUNUSED(event));

extern void
wxPrinterDialogCancel (wxButton & button, wxEvent & WXUNUSED(event));


# define PS_VIEWER_PROG "ghostview"
enum printerDialogItems
{
	prOkButton,
	prCancelButton,
	prCommand,
	prOptions,
	prRadioOrientation,
	prRadioMode,
	prScaleTransButton,
	prXScale,
	prYScale,
	prXTranslate,
	prYTranslate
};

static void setScaleTrans(wxDialogBox * dialogBox, float scaleX, float scaleY,
						  float translateX, float translateY)
{
	wxText       *textXScale = (wxText *) dialogBox->GetChild(prXScale);
	wxText       *textYScale = (wxText *) dialogBox->GetChild(prYScale);
	wxText       *textXTranslate =
	  (wxText *) dialogBox->GetChild(prXTranslate);
	wxText       *textYTranslate =
	  (wxText *) dialogBox->GetChild(prYTranslate);

	char        buf[100];
	sprintf(buf, "%.2f", scaleX);
	textXScale->Replace(0, textXScale->GetLastPosition(), buf);
	sprintf(buf, "%.2f", scaleX);
	textYScale->Replace(0, textYScale->GetLastPosition(), buf);

	sprintf(buf, "%.2f", translateX);
	textXTranslate->Replace(0, textXTranslate->GetLastPosition(), buf);

	sprintf(buf, "%.2f", translateY);
	textYTranslate->Replace(0, textYTranslate->GetLastPosition(), buf);
} /*setScaleTrans()*/

static void resetScaleTrans(wxRadioBox & orientRadio,
							wxEvent & WXUNUSED(event))
{
	int             orientation = orientRadio.GetSelection();
	printSetupData *setupData = &BaseFrame->GraphPrintSetupData;
	
	setScaleTrans((wxDialogBox *) orientRadio.GetParent(),
				  setupData->scaleX[orientation],
				  setupData->scaleY[orientation],
				  setupData->translateX[orientation],
				  setupData->translateY[orientation]);
} /*resetScaleTrans()*/

static void setScaleTransDefaults(wxRadioBox & radioBox,
								  wxEvent & WXUNUSED(event))
{
	if (radioBox.GetSelection() == 0)
	{
		wxDialogBox  *dialogBox = (wxDialogBox *) radioBox.GetParent();
		wxRadioBox   *orientRadio = 
		  (wxRadioBox *) dialogBox->GetChild(prRadioOrientation);
		float         scale, translateX, translateY;

		if (orientRadio->GetSelection() == PS_PORTRAIT)
		{
			scale = GRAPHPORTRAITSCALING;
			translateX = GRAPHPORTRAITTRANSLATIONX;
			translateY = GRAPHPORTRAITTRANSLATIONY;
		}
		else
		{
			scale = GRAPHLANDSCAPESCALING;
			translateX = GRAPHLANDSCAPETRANSLATIONX;
			translateY = GRAPHLANDSCAPETRANSLATIONY;
		}
		setScaleTrans(dialogBox, scale, scale, translateX, translateY);
		
	} /*if (radioBox.GetSeletion() == 0)*/
} /*setScaleTransDefaults()*/


Bool
MacAnovaXPrinterDialog (wxWindow *parent)
{
	wxBeginBusyCursor();
	char       buf[100];
	wxPrinterDialogBox dialog (parent, "Printer Settings", TRUE, 150, 150, 400, 400);
	dialog.SetLabelPosition(wxVERTICAL);

	wxButton *okBut = new wxButton ((wxPrinterDialogBox *)&dialog,
									(wxFunction) wxPrinterDialogOk,
									wxSTR_BUTTON_OK);
	(void) new wxButton ((wxPrinterDialogBox *)&dialog,
						 (wxFunction) wxPrinterDialogCancel,
						 wxSTR_BUTTON_CANCEL);
	dialog.NewLine ();
	dialog.NewLine ();
	okBut->SetDefault();

#ifdef wx_x
	wxText text_prt (&dialog, (wxFunction) NULL, "Printer Command: ",
					 wxThePrintSetupData->GetPrinterCommand(),
					 -1, -1, 100, -1);

	wxText text0 (&dialog, (wxFunction) NULL, "Printer Options: ",
				  wxThePrintSetupData->GetPrinterOptions(),
				  -1, -1, 150, -1);
	dialog.NewLine ();
	dialog.NewLine ();
#endif

	//  char *orientation[] = {"Portrait", "Landscape"};  // HP compiler complains
	char      *orientation[2];
	orientation[0] = "Portrait";
	orientation[1] = "Landscape";
	wxRadioBox radio0 ((wxPrinterDialogBox *)&dialog,
					   (wxFunction) resetScaleTrans,
					   "Orientation: ",-1,-1,-1,-1,2,orientation,2,wxFLAT);
	radio0.SetSelection((int)wxThePrintSetupData->GetPrinterOrientation());

	// @@@ Configuration hook
	if (wxThePrintSetupData->GetPrintPreviewCommand() == NULL)
	{
		wxThePrintSetupData->SetPrintPreviewCommand(PS_VIEWER_PROG);
	}
	
#if USE_RESOURCES
	wxGetResource ("wxWindows",
				   "PSView", &wxThePrintSetupData->previewCommand);
#endif
	//  char *print_modes[] = {"Send to Printer", "Print to File", "Preview Only"};
	char        *print_modes[3];
	print_modes[0] = "Send to Printer";
	print_modes[1] = "Print to File";
	print_modes[2] = "Preview Only";
	int features = (wxThePrintSetupData->GetPrintPreviewCommand() &&
					*wxThePrintSetupData->GetPrintPreviewCommand()) ? 3 : 2;
	wxRadioBox radio1 (&dialog, (wxFunction)NULL, "PostScript:",
					   -1,-1,-1,-1, features, print_modes, features, wxFLAT);
#ifdef wx_msw
	radio1.Enable(0, FALSE);
	if (wxThePrintSetupData->GetPrintPreviewCommand() &&
		*wxThePrintSetupData->GetPrintPreviewCommand())
	{
		radio1.Enable(2, FALSE);
	}
#endif
	radio1.SetSelection((int)wxThePrintSetupData->GetPrinterMode());

	dialog.NewLine ();
	dialog.NewLine ();

	wxButton *setDefault = new wxButton ((wxPrinterDialogBox *)&dialog,
										 (wxFunction) setScaleTransDefaults,
										 "Default Scale/Translation");

	float       wx_printer_translate_x, wx_printer_translate_y;
	float       wx_printer_scale_x, wx_printer_scale_y;

	wxThePrintSetupData->GetPrinterTranslation(&wx_printer_translate_x,
											   &wx_printer_translate_y);
	wxThePrintSetupData->GetPrinterScaling(&wx_printer_scale_x,
										   &wx_printer_scale_y);

	dialog.NewLine ();

	wxText text1 ((wxPrinterDialogBox *)&dialog, (wxFunction) NULL,
				  "X Scaling: ", "", -1, -1, 100, -1);

	wxText text2 ((wxPrinterDialogBox *)&dialog, (wxFunction) NULL,
				  "Y Scaling: ", "", -1, -1, 100, -1);

	dialog.NewLine ();

	wxText text3 ((wxPrinterDialogBox *)&dialog, (wxFunction) NULL,
				  "X Translation: ", "", -1, -1, 100, -1);

	wxText text4 ((wxPrinterDialogBox *)&dialog, (wxFunction) NULL,
				  "Y Translation: ", "", -1, -1, 100, -1);
	
	setScaleTrans(&dialog, wx_printer_scale_x, wx_printer_scale_y,
				  wx_printer_translate_x, wx_printer_translate_y);

	dialog.NewLine ();
	dialog.NewLine ();

	dialog.Fit ();

	wxEndBusyCursor();
	dialog.Show (TRUE);

	if (wxPrinterDialogAnswer)
    {
		StringToFloat (text1.GetValue (),
					   &wxThePrintSetupData->printerScaleX);
		StringToFloat (text2.GetValue (),
					   &wxThePrintSetupData->printerScaleY);
		StringToFloat (text3.GetValue (),
					   &wxThePrintSetupData->printerTranslateX);
		StringToFloat (text4.GetValue (),
					   &wxThePrintSetupData->printerTranslateY);

#ifdef wx_x
		wxThePrintSetupData->SetPrinterOptions(text0.GetValue ());
		wxThePrintSetupData->SetPrinterCommand(text_prt.GetValue ());
#endif

		wxThePrintSetupData->SetPrinterOrientation((radio0.GetSelection() == PS_LANDSCAPE ?
													PS_LANDSCAPE : PS_PORTRAIT));


		// C++ wants this
		switch ( radio1.GetSelection() )
		{
		  case PS_PREVIEW:
			wxThePrintSetupData->SetPrinterMode(PS_PREVIEW);
			break;
		  case PS_FILE:
			wxThePrintSetupData->SetPrinterMode(PS_FILE);
			break;
		  case PS_PRINTER:
			wxThePrintSetupData->SetPrinterMode(PS_PRINTER);
			break;
		} /*switch ( radio1.GetSelection() )*/

    } /*if (wxPrinterDialogAnswer)*/

	return wxPrinterDialogAnswer;
} /*MacAnovaXPrinterDialog ()*/
#endif /*wx_motif*/
