/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 4.05 or later
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
  970910 made changes to fix Motif problems printing graphs
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

#include "wxmain.h"

extern "C"
{
	void myAlert(char *msg);
} /*extern "C"*/

#include <math.h>

#if (0)
#include <ctype.h>
#include "wx_mf.h"
#include "wx_print.h"
#endif /*0*/

wxFont         *PrinterTFont = NULL;

extern wxFont  *CanvasFont;

/*
  the wxPrintout initializer establishes a DC with dialog.GetPrintDC()
  Hence the appropriate orientation, scaling, etc. needs to be set
  before here.
*/
MacAnovaTPrintout::MacAnovaTPrintout(char *title,
								  MacAnovaTextWindow *window):wxPrintout(title)
{
	PrintingWindow = window;
	TotalLines = PrintingWindow->GetNumberOfLines();
} /*MacAnovaTPrintout::MacAnovaTPrintout()*/


Bool MacAnovaTPrintout::OnBeginDocument(int startPage, int endPage) 
{
	int          orientation;

#ifdef wx_motif
	/*
	  we need to reset things because the user may have clicked on
	  Setup in print dialog and changed the orientation
	*/
	orientation = wxGetPrinterOrientation();
	printSetupData *setupData = &BaseFrame->TextPrintSetupData;
	
	wxSetPrinterScaling(setupData->scaleX[orientation],
						setupData->scaleY[orientation]);
	wxSetPrinterTranslation(setupData->translateX[orientation],
							setupData->translateY[orientation]);
#endif /*wx_motif*/

	if (!wxPrintout::OnBeginDocument(startPage,endPage))
	{
		return(FALSE);
	}
	wxDC *printerDC = GetDC();
	if (PrinterTFont == NULL)
	{
		PrinterTFont =
			wxTheFontList->FindOrCreateFont(DefaultPrinterFontSize,
											wxMODERN, wxNORMAL, wxNORMAL);
	}
	printerDC->SetFont(PrinterTFont);

	int             pageMaxX,pageMaxY;		// pixels per page

	orientation = wxGetPrinterOrientation();

#ifdef wx_motif
	GetPageSizePixels(&pageMaxX,&pageMaxY);	// page dimensions in pixels,
	// which appear to be points (1/72 inch)
	PageHeight = (orientation == PS_PORTRAIT) ? pageMaxY : pageMaxX;
	LineHeight = (long) (printerDC->GetCharHeight() + wxLEADING);

	// we put a 1 in margin on top, .75 in on bottom
	// 5/8 in on left, the right will have to fend for itself
	TopMargin = 72;				// 1" assuming points
	LeftMargin = (72*5)/8;		// 5/8" assuming points

#if (0) // debugging code to allow tuning of TopMargin
	Symbolhandle    symh = Lookup("TOPMARGIN");

	if (isInteger(symh, POSITIVEVALUE))
	{
		TopMargin = DATAVALUE(symh, 0);
	}
#endif /*0*/

#endif //wx_motif

#ifdef wx_msw
	int             pagePPIX, pagePPIY; // pixels per inch on page
	int             ppix, ppiy, maxy;

	GetPageSizePixels(&pageMaxX,&pageMaxY);
	GetPPIPrinter(&pagePPIX,&pagePPIY);

	LineHeight = printerDC->GetCharHeight() + wxLEADING;

	ppix = (orientation == PS_PORTRAIT) ? pagePPIX : pagePPIY;
	ppiy = (orientation == PS_PORTRAIT) ? pagePPIY : pagePPIX;
	maxy = (orientation == PS_PORTRAIT) ? pageMaxY : pageMaxX;

	// it looks like lineHeight is itsy,bitsy, so rescale to get the
	// lineheight to be 11 points (11/72) inch
	OverallScale = (11.0/72.0)*ppiy/LineHeight;
	PageHeight = maxy/OverallScale;
	TopMargin = (5.0/6.0)*ppiy/OverallScale;
	LeftMargin = 0.458*ppix/OverallScale; //.458 = (5/8-1/6)

	printerDC->SetUserScale(OverallScale,OverallScale);
#endif // wx_msw

	LinesPerPage = (long) ceil((PageHeight - 1.75*TopMargin)/LineHeight);
	TotalPages = TotalLines/LinesPerPage;
	if(TotalLines > TotalPages*LinesPerPage)
	{
		TotalPages++;
	}
	return(TRUE);
} /*MacAnovaTPrintout::OnBeginDocument()*/


Bool MacAnovaTPrintout::OnPrintPage(int page)
{
	long       lineno, linebase, linelength;
	char      *lineText;
	char       header[100];
	
  	wxDC *dc = GetDC();
  	if (dc)
  	{
		dc->SetFont(PrinterTFont);
#ifdef wx_msw
		dc->SetUserScale(OverallScale, OverallScale);
#endif //wx_msw
		linebase = (page-1)*LinesPerPage;
		float       headerLine = TopMargin/2;
		float       line0 = TopMargin;

		for (lineno = 0; lineno < LinesPerPage;lineno++)
		{
			linelength = PrintingWindow->GetLineLength(linebase+lineno);
			lineText = new char[linelength+4];
			PrintingWindow->GetLineText(linebase+lineno, lineText,
										linelength);
			dc->DrawText(lineText, LeftMargin, line0 + lineno*LineHeight);
			delete [] lineText;
		} /*for (lineno = 0; lineno < LinesPerPage;lineno++)*/
		sprintf(header, "%s --- Page %d", 
				wxFileNameFromPath((PrintingWindow->GetParent())->GetTitle()), 
				page);
		dc->DrawText(header, LeftMargin, headerLine);/* char *, float, float*/
		return TRUE;
	} /*if (dc)*/
  	else
	{
		return FALSE;
	}
} /*MacAnovaTPrintout::OnPrintPage()*/


Bool MacAnovaTPrintout::HasPage(int pageNum)
{
	return (pageNum >= 1 && pageNum <= TotalPages);
} /*MacAnovaTPrintout::HasPage()*/

void MacAnovaTPrintout::GetPageInfo(int *minPage, int *maxPage,
									int *pageFrom, int *pageTo)
{
#ifdef wx_motif
	*minPage = 0;
#else /*wx_motif*/
	*minPage = 1;
#endif /*wx_motif*/
	*maxPage = 32000;
	*pageFrom = 1;
	*pageTo = 32000;
} /*MacAnovaTPrintout::GetPageInfo()*/

MacAnovaGPrintout::MacAnovaGPrintout(char *title,
								 MacAnovaCanvasFrame *frame):wxPrintout(title)
{
	PrintingCFrame = frame;
} /*MacAnovaGPrintout::MacAnovaGPrintout()*/


Bool MacAnovaGPrintout::OnBeginDocument(int startPage, int endPage)
{
	if( !wxPrintout::OnBeginDocument(startPage,endPage))
	{
		return(FALSE);
	}
	wxDC *printerDC = GetDC();
	
	float        w, h;
	float        maxX, maxY;
	float        scaleX, scaleY, actualscale;
	float        posX, posY;
#if (0)
	int          pageMaxX, pageMaxY, LineHeight; // pixels per page
#endif

	// calculate scaling factors
	printerDC->GetSize(&w,&h); // get actual size of device
	maxX = PrintingCFrame->OrigWidth + 2*50; // use 50 device unit margins
	maxY = PrintingCFrame->OrigHeight + 2*50; // use 50 device unit margins
	
	scaleX = w/maxX;
	scaleY = h/maxY;
	
	// get minimum scale
	actualscale = wxMin(scaleX,scaleY);
	printerDC->SetFont(CanvasFont);
	

#if (0)
	Symbolhandle    symh = Lookup("ACTUALSCALE");

	if (isInteger(symh, POSITIVEVALUE))
	{
		actualscale = DATAVALUE(symh, 0);
	}
#endif // 0

	// calculate a centering position
	posX = (w - PrintingCFrame->OrigWidth*actualscale)/2.;
	posY = (h - PrintingCFrame->OrigHeight*actualscale)/2.;

	// set scale and origin
	LeftMargin = (long) posX;
	TopMargin = (long) posY;
	OverallScale = actualscale;

    // It looks like these should be set in OnPrintPage instead of
    // in OnBeginDocument.  It works here for postscript, but not for
    // some other printers
//	printerDC->SetMapMode(MM_TEXT);
//	printerDC->SetUserScale(actualscale,actualscale);
//	printerDC->SetDeviceOrigin(posX,posY);

	return(TRUE);
} /*MacAnovaGPrintout::OnBeginDocument()*/

Bool MacAnovaGPrintout::OnPrintPage(int page)
{
  	wxDC        *dc = GetDC();
	
  	if (dc)
  	{
    	// These three lines moved from OnBeginDocument
        // seems to work better with nonPostScript printers
		dc->SetMapMode(MM_TEXT);
        dc->SetUserScale(OverallScale,OverallScale);
        dc->SetDeviceOrigin(LeftMargin,TopMargin);
        PrintingCFrame->Draw(*dc);
		return TRUE;
	}
	return FALSE;
} /*MacAnovaGPrintout::OnPrintPage()*/

Bool MacAnovaGPrintout::HasPage(int pageNum)
{
	return (pageNum == 1);
} /*MacAnovaGPrintout::HasPage()*/

void MacAnovaGPrintout::GetPageInfo(int *minPage, int *maxPage,
									int *pageFrom, int *pageTo)
{
	*minPage = 1;
	*maxPage = 1;
	*pageFrom = 1;
	*pageTo = 1;
} /*MacAnovaGPrintout::GetPageInfo()*/
