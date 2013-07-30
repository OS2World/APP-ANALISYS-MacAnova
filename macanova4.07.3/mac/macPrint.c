/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 3.36 or later
*(C)*
*(C)* Copyright (c) 1994 by Gary Oehlert and Christopher Bingham
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


#include "globals.h"
#include "macIface.h"

#ifdef UNDEFINED__
static TPrintPeek(THPrint a, Boolean style)
{
	Rect         r;
	WHERE("TPrintPeek");
	
	if(style)
	{
		r = (*a)->rPaper;
		ALERT("rPaper = %d, %d, %d, %d",r.top,r.left,r.bottom,r.right);
		r = (*a)->prInfo.rPage;
		ALERT("prInfo.rPage = %d, %d, %d, %d",r.top,r.left,r.bottom,r.right);
		ALERT("prInfo.iVRes = %d, prInfo.iHRes = %d",(*a)->prInfo.iVRes,(*a)->prInfo.iHRes,0,0);
	}
	else
	{
		ALERT("pages=%d,%d, copies = %d, method = %d",(*a)->prJob.iFstPage,(*a)->prJob.iLstPage,
			  (*a)->prJob.iCopies, (*a)->prJob.bJDocLoop);
	}
}
#endif /*UNDEFINED__*/

void PictWindPrint(Integer windno)
{
	TPPrPort         myPPort;
	TPrStatus        prStatus;
	THPrint          printHdl;
	GrafPtr          thePort;
	Rect             r;
	WindowPtr        thisWindow;
	PicHandle        picture;
	Integer          panelno = windno - NGRAPHS;
	Boolean          doUpdate = true;
	WHERE("PictWindPrint");
	
	if(windno < NGRAPHS)
	{
		thisWindow = GraphWind[windno];
		picture = myPic[windno];
	}
	else
	{
		thisWindow = PanelWind[panelno];
		picture = PanelPic[panelno];
	}

	GetPort(&thePort);
	SetRect(&r, thisWindow->portRect.left, thisWindow->portRect.top,
			thisWindow->portRect.right, thisWindow->portRect.bottom);
	if (windno >= NGRAPHS)
	{ /* reduce rectangle if necessary for panel window */
		setPanelRect(&r,panelno);
	}
	if(doPageSetup(-GRAPHPRINT))
	{
		printHdl = PrintHdl[GRAPHPRINT-1];
		PrOpen();	
	
		if(PrValidate(printHdl))
		{
			putOutErrorMsg("ERROR: printing error");
		}
		else if((DeactivateWindow(), PrJobDialog(printHdl)) &&
				(*printHdl)->prJob.iFstPage <= 1)
		{
			macUpdate((WindowPtr) 0);
			doUpdate = false;
			myPPort = PrOpenDoc(printHdl, 0L, 0L);
			if(PrError() == noErr)
			{
				SetPort((GrafPtr) myPPort);
				TextFont (GraphFont);
				TextSize (GraphFontSize);
				PrOpenPage(myPPort, (TPRect) nil);
				if(PrError() == noErr)
				{
					DrawPicture(picture, &r);
				}
				PrClosePage(myPPort);
			} /*if(PrError() == noErr)*/
			PrCloseDoc(myPPort);
			PrPicFile(printHdl,
					  (TPPrPort) nil, (Ptr) nil, (Ptr) nil, &prStatus); 
		} 
		PrClose();
	} /*if(doPageSetup(-GRAPHPRINT))*/
	if(doUpdate)
	{
		macUpdate((WindowPtr) 0);
	}
	SetPort(thePort);
} /*PictWindPrint()*/

#ifndef POINTSPERPAGE
#define POINTSPERPAGE 726
#endif

void TextWindPrint(Integer windno)
{
	TPPrPort      myPPort;
	TPrStatus     prStatus;
	Integer       startline, startchar, endchar, numchar, pageNumber;
	Integer       linesperpage, linesthispage, lineHeight, fontAscent;
	Integer       pageHeight, pageStart, pageEnd, nextLineStart;
	Integer       printFont = PrintFont;
	Integer       printFontSize = PrintFontSize;
	Str255        printFontName;
	FontInfo      thisFont;
	THPrint       printHdl;
	cmdWindowInfoPtr wp = CmdWindows + windno;
	Integer       selStart = SelStart(wp->teCmd);
	Integer       selEnd = SelEnd(wp->teCmd);
	Boolean       all = (selStart == selEnd);
	Boolean       doUpdate = true;
	WHERE("TextWindPrint");
		
	if (CmdFont != CMDFONT)
	{
		printFont = CmdFont;
		printFontSize = CmdFontSize;
	}
	else if (CmdFontSize != CMDFONTSIZE)
	{
		printFontSize = CmdFontSize;
	}
	GetFontName(printFont, printFontName);

	if(doPageSetup(-TEXTPRINT))
	{
		printHdl = PrintHdl[TEXTPRINT-1];
		PrOpen();	
	
		if(PrValidate(printHdl))
		{
			putOutErrorMsg("ERROR: printing error");
		}
		else if((DeactivateWindow(), PrJobDialog(printHdl)) )
		{
			pageHeight = (*printHdl)->prInfo.rPage.bottom;
			pageStart = (*printHdl)->prJob.iFstPage;
			pageEnd = (*printHdl)->prJob.iLstPage;
			(*printHdl)->prJob.iFstPage = 1;
			(*printHdl)->prJob.iLstPage = iPrPgMax;
			macUpdate((WindowPtr) 0);
			doUpdate = false;
			myPPort = PrOpenDoc(printHdl, 0L, 0L);
			if(PrError() == noErr)
			{
				SetPort((GrafPtr) myPPort);
				TextFont (printFont);
				TextSize (printFontSize);
				GetFontInfo(&thisFont);
				adjustLeading(printFont, printFont, &thisFont);

				lineHeight = thisFont.ascent + thisFont.descent +
					thisFont.leading;
				fontAscent = thisFont.ascent;
				linesperpage = pageHeight / lineHeight;
				if(all)
				{
					startline = 0;
					startchar = 0;
					endchar = TextLength(wp->teCmd);
				}
				else
				{
					startchar = selStart;
					endchar = selEnd;
					startline = 0;
					while (LineStarts(wp->teCmd)[startline+1] <= startchar )
					{
						startline++;
					}
				}
				
				pageNumber = pageStart - 1;
				if(pageNumber > 0)
				{
					startline += pageNumber*linesperpage;
					if(startline < NLines(wp->teCmd))
					{
						startchar = LineStarts(wp->teCmd)[startline];
					}
					else
					{
						startchar = TextLength(wp->teCmd);
					}
				} /*if(pageNumber > 0)*/
				linesthispage = 0;
				while(startchar < endchar && pageNumber < pageEnd)
				{
					if(linesthispage == 0 &&
					   (PrOpenPage(myPPort, 0L), PrError() != noErr))
					{
						linesthispage = -1;
						break;
					}
					MoveTo(0, fontAscent + linesthispage*lineHeight);
					nextLineStart = LineStarts(wp->teCmd)[startline+1];
					numchar = ((endchar < nextLineStart) ?
							   endchar : nextLineStart) - startchar;
					DrawText(*TextHandle(wp->teCmd), startchar, numchar);
					linesthispage++;
					startline++;
					startchar += numchar;
					if( linesthispage >= linesperpage)
					{
						PrClosePage(myPPort);
						linesthispage = 0;
						TextFont (printFont);
						TextSize (printFontSize);
						pageNumber++;
					} /*if( linesthispage >= linesperpage)*/
				} /*while(startchar < endchar && pageNumber <= pageEnd)*/
				if(linesthispage != 0)
				{
					PrClosePage(myPPort);
				}
			} /*if(PrError() == noErr)*/
			PrCloseDoc(myPPort);
			PrPicFile(printHdl, 0L, 0L, 0L, &prStatus); 
		}
		PrClose();
	} /*if(doPageSetup(-TEXTPRINT))*/
	if(doUpdate)
	{
		macUpdate((WindowPtr) 0);
	}
} /*TextWindPrint()*/

