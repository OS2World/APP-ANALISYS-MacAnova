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


#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment Grafwind
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


/*
	MacAnova graph window handler.  Modelled after TransSkel windows
	940429 Modified to work with TransSkel version 3.12
	
	960515 Modified to allow for more than 4 regular graph windows and several
	Panel windows.  Currently the code explicitly assumes each Panel window has
	4 Panes, although it uses macro NPERPANEL to compute which panel window is
	associated with a graph.
	
	960823  Fixed bug in GraphWindPrint that prevented Panel 5-8
	        from being printed
	980825  Modified setGraphHelpItems() and setGraphFileItems to
	        allow for using Help menu

	Macros defined in macIface.h
	 NGRAPHS       Number of graphs windows, excluding panel windows
	 NPANELS       Number of panel windows
	 NPERPANEL     Number of panes per panel
	 
	Menu entries for Panels are "Panel 1-4", "Panel 5-8"
	Command G goes with the panel associated with the frontmost non-panel
	graph window or, if there is only one panel window, with that window.
*/


#include	"globals.h"
#include	"macIface.h"

#ifdef MW_CW
#define LongInt long
#endif


Rect            PanelRect;

/* return whether the zoomed-in or zoomed-out status of window*/
static Integer GraphWindZoomStatus(Integer windno)
{
	Rect            r, stdState;
	WindowPtr       theWindow;
	WStateData    **wStateHandle;
	WHERE("GraphWindZoomStatus");

	theWindow = (windno < NGRAPHS) ? GraphWind[windno] : PanelWind[windno-NGRAPHS];

	wStateHandle = (WStateData **) ((WindowPeek) theWindow)->dataHandle;
	HLock((Handle) wStateHandle);
	stdState = (**wStateHandle).stdState;
	HUnlock((Handle) wStateHandle);

	r = theWindow->portRect;
	return ((r.top - r.bottom == stdState.top - stdState.bottom &&
			r.left - r.right == stdState.left - stdState.right) ?
	   			inZoomOut : inZoomIn);
} /*GraphWindZoomStatus()*/

static void setCommandG(Integer windno)
{
	int       ipanel, panelno;
	WHERE("setCommandG");

	for (ipanel = 0; ipanel < NPANELS; ipanel++)
	{
		SetItemCmd(WindowMenu, panel1 + ipanel, 0);
	}
	if (windno >= 0)
	{
		panelno = (windno < NGRAPHS) ? windno/NPERPANEL : (windno - NGRAPHS + 1) % NPANELS;
		if (PanelWind[panelno] != (WindowPtr) 0)
		{
			SetItemCmd(WindowMenu, panel1 + panelno, 'G');
		}
		else
		{
			windno = -1;
		}
	}	
	if (windno < 0)
	{
		for (windno = 0; windno < NGRAPHS && GraphWind[windno] == (WindowPtr) 0;
			windno++)
		{
			;
		}
		if (windno < NGRAPHS)
		{
			SetItemCmd(WindowMenu, panel1 + windno/NPERPANEL, 'G');
		}
	}
} /*setCommandG()*/

static PicHandle myOpenPicture(Rect *r)
{
	OpenCPicParams       header;
	Boolean              useOpenCPicture = 
		SkelQuery(skelQHasColorQD) && SkelQuery(skelQSysVersion) >= 0x0700;
	WHERE("myOpenPicture");
	
	if (!useOpenCPicture)
	{
		return (OpenPicture(r));
	}
	header.srcRect = *r;
	header.hRes = header.vRes = 0x480000;
	header.version = -2;
	header.reserved1 = header.reserved2 = 0;
	return (OpenCPicture(&header));	
} /*myOpenPicture*/

/* Used to generate default file names */
static short           PlotFileCount[3] = {0, 0, 0};
static char           *Description[3][2] =
{
	{"Encapsulated PS", "epsf"},
	{"PostScript",      "ps"},
	{"PICT",            "pict"}
};

char *findPlotFile(char *fileName, char *kind, long ps, long epsf, long newFile)
{
	char        prompt[40], defaultName[20];
	char       *macFileName;
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
	macFileName = macFindFile(fileName, CtoPstr(prompt), CtoPstr(defaultName),
							  WRITEIT, 0, (OSType *) 0, &PlotVolume);
	if (macFileName == (char *) 0)
	{ /* Canceled */
		if (PlotFileCount[which] == 1 || newFile)
		{
			PlotFileCount[which]--;
		}
	}
	return (macFileName);
} /*findPlotFile()*/

#define SIZEOFPICTHEADER 512

Boolean writePictFile(PicHandle picture, FILE * pictFile)
{
	char      *header = "#######PICT image from MacAnova######\n";
	char      filler = '\n';
	int       length = strlen(header);
	long      count;
	
	count = fwrite(header, 1, length, pictFile);
	if (count != length)
	{
		return (false);
	}
	length = SIZEOFPICTHEADER - length;
	while(length-- > 0)
	{
		if (fwrite(&filler, 1, 1, pictFile) != 1)
		{
			return (false);
		}
	} /*while(length-- > 0)*/
	length = GetHandleSize((Handle) picture);
	HLock((Handle) picture);
	count = fwrite((Ptr) *picture, 1, length, pictFile);
	if (count != length)
	{
		return (false);
	}
	HUnlock((Handle) picture);
	return (true);
} /*writePict()*/

/*
	Implements Save As... item on File menu when Graph Window
	is front.  Currently item is unused.
*/
extern char       *ScreenDumpFile;

void doSaveGraph(Integer windno, Integer item)
{
	FILE       *pictFile;
	char       *fileName;
	PicHandle   picture;
	WHERE("doSaveGraph");

	if (windno >= 0)
	{
		fileName = findPlotFile((ScreenDumpFile != (char *) 0) ? ScreenDumpFile : "",
								"Graph", 0, 0, 1);
		
		if (fileName == (char *) 0)
		{ /* cancelled */
			return;
		}
		picture = (windno < NGRAPHS) ? myPic[windno] : PanelPic[windno - NGRAPHS];
		pictFile = openPlotFile(fileName, 1L);
		if (pictFile == (FILE *) 0 || !writePictFile(picture, pictFile))
		{
			myAlert("Cannot write PICT file");
		}
		else
		{
			macSetInfo(PlotVolume, fileName, 'PICT', CREATOR);
		}

		if (pictFile != (FILE *) 0)
		{
			fclose(pictFile);
		}
	}
} /*doSaveGraph()*/
/*
	window handler procedures
*/


/*
	Dispose of graphWindow;  do nothing if graphWindow == 0
*/
static void GraphWindDispose(WindowPtr graphWindow)
{
	Integer         windno, panelno, firstpane, ipane;
	WindowPtr       nextWindow;
	WStateData    **wStateHandle, *wStatePtr;
	WHERE("GraphWindDispose");

	if (graphWindow != (WindowPtr) 0)
	{
		windno = whichGraphWindow(graphWindow);
		if (windno == PREVIEWWINDOW || windno == PICTWINDOW)
		{
			if (PreviewPic != (PicHandle) 0)
			{
				KillPicture(PreviewPic);
				PreviewPic = (PicHandle) 0;
			}
			if (PreviewWind != (WindowPtr) 0)
			{
				DisposeWindow(PreviewWind);
				PreviewWind = (WindowPtr) 0;
			}			
		} /*if (windno == PREVIEWWINDOW || windno == PICTWINDOW)*/
		else if (windno >= 0)
		{
			panelno = windno/NPERPANEL;
			firstpane = NPERPANEL*panelno;
			if (0 <= windno && windno < NGRAPHS)
			{
			/*
				Save zoom info for non-panel windows
			*/
				wStateHandle = (WStateData **) ((WindowPeek) graphWindow)->dataHandle;
				HLock((Handle) wStateHandle);
				wStatePtr = GraphWState + windno;
				wStatePtr->userState = (*wStateHandle)->userState;
				wStatePtr->stdState = (*wStateHandle)->stdState;
				HUnlock((Handle) wStateHandle);
			} /*if(0 <= windno && windno < NGRAPHS)*/
	
			SkelRmveWind(graphWindow); /* calls GraphWindClobber */
			
			if (windno < NGRAPHS && PanelWind[panelno] != (WindowPtr) 0)
			{
				for (ipane = 0;
					 ipane < NPERPANEL && GraphWind[firstpane+ipane] == (WindowPtr) 0;
					 ipane++)
				{
					;
				}
				if (ipane == NPERPANEL)
				{
					SkelRmveWind(PanelWind[panelno]);
				}
			}
			nextWindow = FrontWindow();
			if (nextWindow != (WindowPtr) 0)
			{
				SelectWindow(nextWindow);
			}
		} /*if (windno == PREVIEWWINDOW || windno == PICTWINDOW){}else{}*/
	} /*if (graphWindow != (WindowPtr) 0)*/
} /*GraphWindDispose()*/

/*
   Public function
	Set the frame for the panel window to the minimum size that will hold
	all the current panels.
	At entry, r should be the port rectangle of the window
*/
#define TOPHALFPANEL      0x03
#define BOTTOMHALFPANEL   0x0c
#define LEFTHALFPANEL     0x05
#define RIGHTHALFPANEL    0x0a

void setPanelRect(Rect *r, Integer panelno)
{
	Integer        powtwo = 1, windowPattern = 0;
	Integer        halfHeight, halfWidth;
	Integer        k, firstpane = panelno*NPERPANEL;
	WHERE("setPanelRect");

	powtwo = 1;
	for(k = 0; k < NPERPANEL;k++)
	{ /* set a bit for each window present */
		if (myPic[firstpane + k] != (PicHandle) 0)
		{
			windowPattern |= powtwo;
		}
		powtwo <<= 1;
	}
	r->right -= RIGHTOFFSET;
	halfHeight = (r->bottom - r->top)/2;
	halfWidth = (r->right - r->left)/2;

	if(!(windowPattern & TOPHALFPANEL))
	{ /* no panels in top half */
		r->top += halfHeight;
	}
	if(!(windowPattern & BOTTOMHALFPANEL))
	{ /* no panels in bottom half */
		r->bottom -= halfHeight;
	}
	if(!(windowPattern & LEFTHALFPANEL))
	{ /* no panels in left half*/
		r->left += halfWidth;
	}
	if(!(windowPattern & RIGHTHALFPANEL))
	{ /* no panels in right half */
		r->right -= halfWidth;
	}
} /*setPanelRect()*/

/*
	Draw copies of each of the basic graph windows in the corners of
	the pane window
*/ 
static void drawPanelPicture(Integer panelno)
{
	Rect           r, r1;
	Integer        k;
	Integer        powtwo = 1;
	Integer        halfHeight, halfWidth;
	WindowPtr      theWindow;
	WHERE("drawPanelPicture");

	SetRect(&r, -200, -200, 1483, 1275);
	ClipRect(&r);

	r = PanelRect;
	setPanelRect(&r, panelno);
	PanelPic[panelno] = myOpenPicture(&r);

	r = PanelRect;
	r.right -= RIGHTOFFSET;
	halfHeight = (r.bottom - r.top) / 2;
	halfWidth = (r.right - r.left) / 2;

	for (k = 0, powtwo = 1; k < NPERPANEL; k++, powtwo <<= 1)
	{
		r1.top = (powtwo & BOTTOMHALFPANEL) ? r.top + halfHeight : r.top;
		r1.bottom = r1.top + halfHeight;
		r1.left = (powtwo & RIGHTHALFPANEL) ? r.left + halfWidth : r.left;
		r1.right = r1.left + halfWidth;
		if((theWindow = GraphWind[panelno*NPERPANEL + k]) != (WindowPtr) 0)
		{
			DrawPicture(myPic[panelno*NPERPANEL + k], &r1);
		}
	} /*for (k = 0, powtwo = 1; k < NPERPANEL; k++, powtwo <<= 1)*/
	ClosePicture();
	EnableItem(WindowMenu, panel1+panelno);
} /*drawPanelPicture()*/

/*
	Called from macMain.c with windno == -1 to zero
	GraphWState[.].userState and GraphWState[.].stdState

	Called from GraphWindInit() to initialize GraphWState[windno].userState
	and GraphWState[windno].stdState

	970503 replaced code to get screen bounds by call to new function
	getScreenRect() (in macUtils.c)
*/

void GraphRectInit(Integer windno)
{
	Integer      i, panelno;
	Integer      downoffset;
	Integer      rightoffset;
	WStateData **wStateHandle;
	Rect        *r, bBox;
#if (0)
	RgnHandle    grayRgnH;
#endif /*0*/
	
	if(windno < 0)
	{ /* zero GraphWindState[i].{user,std}State, i = 0,...,NGRAPHS-1 */
		for(i=0;i < NGRAPHS;i++)
		{
			r = &GraphWState[i].userState;
			r->top = r->left = r->bottom = r->right = 0;
			r = &GraphWState[i].stdState;
			r->top = r->left = r->bottom = r->right = 0;
			GraphWInZoom[i] = inZoomIn;
			GraphWind[i] = (WindowPtr) 0;
			myPic[i] = (PicHandle) 0;
		}
		PreviewWind = (WindowPtr) 0;
		PreviewPic = (PicHandle) 0;
	} /*if(windno < 0)*/
	else
	{
	/*
		setup GraphWindState[windno].userState
	*/
		if (windno < NGRAPHS)
		{
			panelno = windno/NPERPANEL;
			wStateHandle = (WStateData **) ((WindowPeek) GraphWind[windno])->dataHandle;
			downoffset = (windno + 1 + panelno)*STACKOFFSET;
			rightoffset = (windno + 1 + panelno)*STACKOFFSET/2;
		}
		else
		{
			panelno = windno - NGRAPHS;
			wStateHandle = (WStateData **) ((WindowPeek) PanelWind[panelno])->dataHandle;
			downoffset = (panelno*(NPERPANEL + 1))*STACKOFFSET;
			rightoffset = (panelno*(NPERPANEL + 1))*STACKOFFSET/2;
		}
					

#if (1)
		getScreenRect(&bBox);
#else /*1*/
		grayRgnH = (RgnHandle) SkelQuery(skelQGrayRgn);
		if (grayRgnH != (RgnHandle) 0)
		{
			bBox = (*grayRgnH)->rgnBBox;
			DisposeHandle((Handle) grayRgnH);
		}
		else
		{
			bBox = (*wStateHandle)->stdState;
		}
#endif /*1*/
		if ((*wStateHandle)->userState.bottom + downoffset > bBox.bottom)
		{
			downoffset = bBox.bottom - (*wStateHandle)->userState.bottom;
		}
		if ((*wStateHandle)->userState.right + rightoffset > bBox.right)
		{
			rightoffset = bBox.right - (*wStateHandle)->userState.right;
		}
		(*wStateHandle)->userState.top += downoffset;
		(*wStateHandle)->userState.left += rightoffset;
		(*wStateHandle)->userState.bottom += downoffset;
		(*wStateHandle)->userState.right += rightoffset;
		if (windno < NGRAPHS)
		{
			GraphWState[windno].userState = (*wStateHandle)->userState;
			GraphWState[windno].stdState = (*wStateHandle)->stdState;
			GraphWInZoom[windno] = inZoomIn;
		}
	} /*if(windno < 0){}else{}*/
} /*GraphRectInit()*/

static void setGraphWindowItems(void)
{
	Integer       		offset = MAXWINDOWS - Nwindows;

	EnableItem(WindowMenu, hideit);
	EnableItem(WindowMenu, closeit);
	DisableItem(WindowMenu, newwind);
	DisableItem(WindowMenu, gototop - offset);
	DisableItem(WindowMenu, gotobottom - offset);
	DisableItem(WindowMenu, gotoprompt - offset);
	DisableItem(WindowMenu, pageup - offset);
	DisableItem(WindowMenu, pagedown - offset);
} /*setGraphWindowItems()*/

/*
	Initialize a specific graphics window, 0, 1, 2, ... or NGRAPHS - 1
	or a undisplayed window
*/
PicHandle GraphWindInit(Integer windno)
{
	Rect            r;
	Str15           str;
	Integer         initialize;
	WStateData    **wStateHandle, *wStatePtr;
	GrafPtr         thePort;
	WindowPtr       theWindow = (WindowPtr) 0;
	PicHandle       picH;
	WHERE("GraphWindInit");

	GetPort(&thePort);

	if (windno >= 0)
	{
		GraphWindDispose(GraphWind[windno]);
	}
	else if (windno == PREVIEWWINDOW || windno == PICTWINDOW)
	{
		GraphWindDispose(PreviewWind);
	}
	theWindow = GetNewWindow(GRAPHWINDOW, (Ptr) nil, (WindowPtr) -1L);
	if(theWindow == (WindowPtr) 0 || windno >= 0 && !SkelWindow(theWindow,
				   (SkelWindMouseProcPtr) GraphWindMouse,
				   (SkelWindKeyProcPtr) GraphWindKey,
				   (SkelWindUpdateProcPtr) GraphWindUpdate,
				   (SkelWindActivateProcPtr) GraphWindActivate,
				   (SkelWindCloseProcPtr) GraphWindClose,
				   (SkelWindClobberProcPtr) GraphWindClobber,
				   (SkelWindIdleProcPtr) nil, true))
	{
		goto errorExit;
	}

	if (windno == PREVIEWWINDOW || windno == PICTWINDOW)
	{
		PreviewWind = theWindow;
	}
	else
	{
		GraphWind[windno] = theWindow;
		sprintf((char *) str, "Graph %d",windno+1);
		CtoPstr((char *) str);
		SetWTitle(theWindow, str);
	
		wStateHandle = (WStateData **) ((WindowPeek) theWindow)->dataHandle;
		HLock((Handle) wStateHandle);

		wStatePtr = GraphWState + windno;
	
		initialize = (wStatePtr->userState.top == 0 &&
					  wStatePtr->userState.left == 0 &&
					  wStatePtr->userState.bottom == 0 &&
					  wStatePtr->userState.right == 0);
		if(initialize)
		{/* first time, so set up info for zooming */
			GraphRectInit(windno);
		}
		else
		{/* restore zoom information */
			(*wStateHandle)->userState = wStatePtr->userState;
			(*wStateHandle)->stdState = wStatePtr->stdState;
		}
		HUnlock((Handle) wStateHandle);
		/* always create picture with window zoomed in */
		ZoomWindow(GraphWind[windno], inZoomIn, true);
	
		setGraphHelpItems();
		setCmdFileItems(false);
		setGraphEditItems();
		setGraphWindowItems();
	} /*if (windno >= 0)*/
	SetPort((GrafPtr) theWindow);
	TextFont(GraphFont);
	TextSize(GraphFontSize);

	SetRect(&r, -200, -200, 1483, 1275);
	ClipRect(&r);
	r = theWindow->portRect;
	EraseRect(&r);

	r.right -= RIGHTOFFSET;

	picH = myOpenPicture(&r);
	if (picH == (PicHandle) 0)
	{
		goto errorExit;
	}
	if (windno == PREVIEWWINDOW || windno == PICTWINDOW)
	{
		PreviewPic = picH;
	}
	else
	{
		myPic[ThisWindow] = picH;
	}
	ShowPen();

	return (picH);

  errorExit:
	if(theWindow != (WindowPtr) 0)
	{
		DisposeWindow(theWindow);
		if (windno == PREVIEWWINDOW || windno == PICTWINDOW)
		{
			PreviewWind = (WindowPtr) 0;
		}
		else
		{
			GraphWind[windno] = (WindowPtr) 0;
		}
	}
	return ((PicHandle) 0);
} /*GraphWindInit()*/

/*
	Return a PicHandle (returned by GraphWindInit()) to draw into.
	
	If global ThisWindow >= 0, it specifies the window to be used and the
	PicHandle is associated with that window.
	
	If ThisWindow == -1, it indicates the lowest unused graph window,
	if any, should be used.  If there is none, (PicHandle) -1 is returned;
	
	If ThisWindow == PREVIEW (-2), no graphics window is to be used;  instead a GrafPort
	needs to be created and a PicHandle associated with it.  This is not
	yet (960521) implemented but would be needed in creating an EPSF file with
	preview picture.

	ThisWindow is set in do_whole_graph() (in plotutils.c), to -1 if window:windno
	not a plotting command argument, or to windno-1 when if windno > 0, or is
	left unchanged when windno == 0.
*/
PicHandle GraphWindSetup(void)
{
	int             windno;
	WHERE("GraphWindSetup");

	if (ThisWindow == FINDEMPTYWINDOW)
	{ /* find unused window */
		for (windno = 0; windno < NGRAPHS; windno++)
		{
			if (GraphWind[windno] == (WindowPtr) 0)
			{
				ThisWindow = windno;
				break;
			}
		}
		if (windno == NGRAPHS)
		{ /* no unused windows */
			return ((PicHandle) -1);
		}
	} /*if (ThisWindow == FINDEMPTYWINDOW)*/
/* at this point, 0 <= ThisWindow < NGRAPHS or ThisWindow < -1*/
	return (GraphWindInit(ThisWindow));
} /*GraphWindSetup()*/

/*
	Called from gnuplot to close out picture
	
	If the picture is for a window (windno >= 0), the picture is
	drawn to the window and the panel window is created or updated.
	
	If it is to be the preview part of an epsf file
	(windno == PREVIEWWINDOW), the existing resource fork, if
	any of the plotting file is cleared and a PICT resource
	with ID 256 is added.
	
	If it is to be written as a PICT file (windno == PICTWINDOW)
	the file is written
	
	960612 added argument errCode to give information about any
	quickdraw error found in gnuplot.  If errCode != 0, the graphics
	window being written is clobbered and the commandwindow is activated.
	
	970604 fixed bug; global window pointer now set to NULL when errCode
	indicates an error.
*/

#define EPSFPICTID 256

void GraphOver(Integer errCode)
{
	Rect            r;
	GrafPtr         thePort;
	Integer         windno, panelno;
	Integer         resFile, vRefNum, refNum;
	LongInt         dirID;
	char            nullByte = '\0';
	LongInt         count = 1;
	WindowPtr       theWindow;
	WHERE("GraphOver");

	ClosePicture();
	HidePen();

	GetPort(&thePort);
	theWindow = (WindowPtr) thePort;
	

	if (errCode != noErr)
	{
		/* Quickdraw couldn't complete picture*/
		sprintf(OUTSTR,
				"ERROR: %s while %s() was attempting to draw picture; not drawn",
				(errCode == memFullErr) ? "out of memory" : "problem hit",
				FUNCNAME);
		putOUTSTR();

		GraphWindDispose(theWindow);
			
		ActivateWindow(true);
		return;
	} /*if (errCode != noErr)*/
	
	windno = whichGraphWindow(theWindow);

	if (windno >= 0)
	{ /* regular graphics window */
		panelno = windno/NPERPANEL;
		ZoomWindow(GraphWind[windno], GraphWInZoom[windno], true);
		MyShowWindow(GraphWind[windno]);
		DrawGrowBox(thePort);
	
		r = thePort->portRect;
		EraseRect(&r);
		r.right -= RIGHTOFFSET;
		DrawPicture(myPic[windno], &r);
	
		EnableItem(WindowMenu, graph1 + windno);
		setGraphFileItems(true);
	
		if(PanelWind[panelno] == (WindowPtr) 0)
		{
			PanelWind[panelno] = GetNewWindow(GRAPHWINDOW, (Ptr) nil,
											  theWindow);
			if(PanelWind[panelno] == (WindowPtr) 0 ||
			   !SkelWindow(PanelWind[panelno],
						   (SkelWindMouseProcPtr) GraphWindMouse,
						   (SkelWindKeyProcPtr) GraphWindKey,
						   (SkelWindUpdateProcPtr) GraphWindUpdate,
						   (SkelWindActivateProcPtr) GraphWindActivate,
						   (SkelWindCloseProcPtr) GraphWindClose,
						   (SkelWindClobberProcPtr) GraphWindClobber,
						   (SkelWindIdleProcPtr) GraphWindIdle, true))
			{
				myAlert("Cannot create Panel graph window; out of memory (?)");
				DisposeWindow(PanelWind[panelno]);
				goto errorExit;
			}
			GraphRectInit(NGRAPHS + panelno);
			ZoomWindow(PanelWind[panelno], inZoomIn, false);
			PanelRect = PanelWind[panelno]->portRect;
		
			SkelSetZoom(PanelWind[panelno], (SkelWindZoomProcPtr) GraphWindZoom);
			SetWTitle(PanelWind[panelno], (unsigned char *) PanelTitle[panelno]);
		} /*if(PanelWind[0] == (WindowPtr) 0)*/
	
		if(PanelPic[panelno] != (PicHandle) 0)
		{
			KillPicture(PanelPic[panelno]);
			PanelPic[panelno] = (PicHandle) 0;
		}
	
		HideWindow(PanelWind[panelno]);
	
		drawPanelPicture(panelno);
		ShowWindow(PanelWind[panelno]);
		setCommandG(windno);
	 	SetPort(thePort);
		ValidRect(&thePort->portRect);
	} /*if (windno >= 0)*/
	else if (windno == PREVIEWWINDOW)
	{ /* Save PICT resource as preview picture*/
		CtoPstr(*PLOTFILENAME);
		/* Make sure Resource Fork is completely empty*/
		if (OpenRF((const unsigned char *) *PLOTFILENAME, 0, &refNum) != noErr)
		{
			goto noPreview;
		}
		if (FSWrite(refNum, &count, &nullByte) != noErr)
		{
			goto noPreview;
		}
		if (SetEOF(refNum, 0L) != noErr)
		{
			goto noPreview;
		}
		if (FSClose(refNum) != noErr)
		{
			goto noPreview;
		}
		/* now create new resource fork and add picHandle to it*/
		if (HGetVol((StringPtr) 0, &vRefNum, &dirID) != noErr)
		{
			goto noPreview;
		}

		HCreateResFile(vRefNum, dirID, (const unsigned char *) *PLOTFILENAME);
		if (ResError() != noErr)
		{
			goto noPreview;
		}
		
		resFile = OpenResFile((const unsigned char *) *PLOTFILENAME);
		if (ResError() != noErr)
		{
			goto noPreview;
		}			
	
		UseResFile(resFile);
		if (ResError() != noErr)
		{
			goto closePreview;
		}			
			
		AddResource((Handle) PreviewPic, 'PICT', EPSFPICTID, "\p");
		if (ResError() != noErr)
		{
			goto closePreview;
		}			
			
		PreviewPic = (PicHandle) 0;
		CloseResFile(resFile);
		goto cleanPreview;
	} /*if (windno >= 0){}elseif (windno == PREVIEWWINDOW){}*/
	else if (windno == PICTWINDOW)
	{ /*write PICT file*/
		if (!writePictFile(PreviewPic, PLOTFILE))
		{
			myAlert("Cannot write PICT file");
		}
		goto disposePreview;
	} /*if(windno>=0){}else if(windno == PREVIEWWINDOW){}else if (windno == PICTWINDOW){}*/
	return;

  closePreview:
	CloseResFile(resFile);
	/*fall through*/
  noPreview:
  	myAlert("Cannot create preview part of encapsulated PostScript file");
	/*fall through*/
  cleanPreview:
	PtoCstr((unsigned char *) *PLOTFILENAME);
	/*fall through*/
  disposePreview:
	GraphWindDispose(PreviewWind);
	/*fall through*/  
  errorExit:
 	SetPort(thePort);
} /*GraphOver()*/


/* Should be entered only when a graph window is in front */
pascal void GraphWindIdle(void)
{
	Integer      windno;

	windno = whichGraphWindow(FrontWindow());
	if(0 <= windno && windno < NGRAPHS + NPANELS) /* be paranoid*/
	{
		setGraphFileItems(true);
	}
} /*GraphWindIdle()*/

/*
	get rid of graph window and any associated picture.
	The picHandle and WindowPtr are set to nil
*/
pascal void
GraphWindClobber(void)
{
	GrafPtr         thePort;
	Integer         windno, panelno;
	WHERE("GraphWindClobber");

	GetPort(&thePort);
	windno = whichGraphWindow((WindowPtr) thePort);
	
	if(windno >= 0)
	{
		DisableItem(WindowMenu, graph1+windno);
		if(windno < NGRAPHS)
		{
			if (myPic[windno] != (PicHandle) 0)
			{
				KillPicture(myPic[windno]);
				myPic[windno] = (PicHandle) 0;
			}
			GraphWind[windno] = (WindowPtr) 0;
		}
		else if(windno >= NGRAPHS)
		{
			panelno = windno - NGRAPHS;
			if(PanelPic[panelno] != (PicHandle) 0)
			{
				KillPicture(PanelPic[panelno]);
				PanelPic[panelno] = (PicHandle) 0;
			}
			PanelWind[panelno] = (WindowPtr) 0;
			setCommandG(-1);				
		}
		GraphWindPaused = false;
	} /*if(windno >= 0)*/
	DisposeWindow((WindowPtr) thePort);
}/*GraphWindClobber()*/


/*
	Close box clicked or close on the Window menu
*/
pascal void GraphWindClose(void)
{
	WindowPtr      theWindow = FrontWindow();
	Integer        windno = whichGraphWindow(theWindow);
	Integer        panelno;
	WHERE("GraphWindClose");

	if (windno >= 0)
	{
		GraphWindDispose(theWindow);
		panelno = windno/NPERPANEL;
		/* throw away panel picture if we are closing non-panel graph window */
		if (windno < NGRAPHS && PanelPic[panelno] != (PicHandle) 0)
		{
			KillPicture(PanelPic[panelno]);
			PanelPic[panelno] = (PicHandle) 0;
			SetPort((GrafPtr) PanelWind[panelno]);
			InvalRect(&PanelWind[panelno]->portRect);
		} /*if (windno < NGRAPHS && PanelPic[panelno] != (PicHandle) 0)*/
	} /*if (windno >= 0)*/
} /*GraphWindClose()*/

pascal void GraphWindZoom(WindowPtr theWindow, Integer zoomDir)
{
	Integer         windno = whichGraphWindow(theWindow);

	if (0 <= windno && windno < NGRAPHS)
	{
		GraphWInZoom[windno] = zoomDir;
	} EraseRect(&theWindow->portRect);
	ZoomWindow(theWindow, zoomDir, false);
} /*GraphWindZoom()*/

pascal void GraphWindActivate(Boolean active)
{
	GrafPtr         thePort;
	Integer         windno;
	WHERE("GraphWindActivate");

	GetPort(&thePort);
	DrawGrowBox(thePort);
	if (active)
	{
		InitCursor();

		setGraphHelpItems();

		setGraphFileItems(true);
		EnableItem(FileMenu, (Integer) 0);

		setGraphEditItems();
		EnableItem(EditMenu, (Integer) 0);

#if (0) /*Don't reset command M; Return or Enter gets back to current window*/
		if(UseWindows > 0)
		{
			setCommandM(CurrentWindow);
		}
#endif
		if (!GraphWindPaused)
		{
			windno = whichGraphWindow((WindowPtr) thePort);
			if (0 <= windno)
			{
				setCommandG(windno);
			}
			EnableItem(WindowMenu, (Integer) 0);
			setGraphWindowItems();

			EnableItem(CommandMenu, (Integer) 0);
			EnableItem(OptionsMenu, (Integer) 0);
		} /*if(!GraphWindPaused)*/
	} /*if(active)*/ 
	else
	{
		setGraphFileItems(false);

		DisableItem(FileMenu, (Integer) 0);
		DisableItem(WindowMenu, (Integer) 0);

		DisableItem(CommandMenu, (Integer) 0);
		DisableItem(OptionsMenu, (Integer) 0);

		EnableItem(EditMenu, undoit);
		EnableItem(EditMenu, cutit);
		EnableItem(EditMenu, copyit);
		EnableItem(EditMenu, pastit);
	} /*if(active){...}else{...}*/
	DrawMenuBar();
}/*GraphWindActivate()*/

/*
	Only does anything for a panel window.  If you click in a panel
	it switches to the full sized version of the graph
*/

pascal void GraphWindMouse(Point thePt, LongInt when, Integer mods)
{
	GrafPtr         thePort;
	WindowPtr       graphWindow;
	Integer         whichGraph;
	Integer         halfHeight, halfWidth;
	Integer         panelno;
	Rect            r;
	Point           thePt2 = thePt;

	if(!GraphWindPaused)
	{
		when;			/*just to defeat compiler message */
		mods;			/*just to defeat compiler message */
		InitCursor();
		GetPort(&thePort);
		graphWindow = (WindowPtr) thePort;
		for (panelno = 0; panelno < NPANELS && graphWindow != PanelWind[panelno]; panelno++)
		{
			;
		}
		if (panelno < NPANELS)
		{
			whichGraph = panelno * NPERPANEL;
			r = PanelWind[panelno]->portRect;
			halfHeight = (r.bottom - r.top) / 2;
			halfWidth = (r.right - r.left) / 2;
			if (thePt.v > r.top + halfHeight)
			{
				whichGraph += 2;
			}
			if (thePt.h > r.left + halfWidth)
			{
				whichGraph++;
			}
			if ((graphWindow = GraphWind[whichGraph]) != (WindowPtr) 0)
			{
				MyShowWindow(graphWindow);
			}
		} /*if (panelno < NPANELS)*/
	} /*if(!GraphWindPaused)*/
} /*GraphWindMouse()*/

/*
	Process key strokes in Graph Window
		Command Period      Interrupt
		Command Option Q    Quit on File menu
		Command Option 1-8  Execute command 1 to 8 on Command Menu
		F3                  Copy on Edit menu
		Return, Enter       Switch to Command Window or end pause
    960719 F5 - F12 no longer execute items on Command Menu
*/

pascal void GraphWindKey(Integer ch1, Integer code, Integer mods)
{
	unsigned char   ch = (unsigned char) ch1;
	Integer         cmdNo = -1, editItem = -1, windowItem = -1;

	HLock((Handle) CommandMenu);
	if (ch1 == FUNCTIONKEY)
	{
		cmdNo = functionKeyToNumber(code);
		if (!(mods & cmdKey))
		{
			switch (cmdNo)
			{
			  case 1: /*F1 = undo*/
				editItem = undoit;
				break;
		
			  case 2: /*F2 = cut*/
				editItem = cutit;
				break;
		
			  case 3: /*F3 = copy*/
				editItem = copyit;
				break;
		
			  case 4: /*F4 = paste*/
				editItem = pastit;
				break;
		
			  case 5: /*F5 = copytoend*/
				editItem = copytoend;
				break;
		
			  case 6: /*F6 = copyandexecute*/
				editItem = copyandexecute;
				break;
	
			  default:
				;
			} /*switch (cmdNo)*/
		} /*if (!(mods & cmdKey))*/
		else
		{
			switch (cmdNo)
			{
			  case 1:
				windowItem = graph1;
				break;
		
			  case 2:
				windowItem = graph2;
				break;
		
			  case 3:
				windowItem = graph3;
				break;
		
			  case 4:
				windowItem = graph4;
				break;
#if (NGRAPHS > 4)		
			  case 5:
				windowItem = graph5;
				break;
		
			  case 6:
				windowItem = graph6;
				break;
	
			  case 7:
				windowItem = graph7;
				break;
		
			  case 8:
				windowItem = graph8;
				break;
#endif /*NGRAPHS > 4*/
#if (NGRAPHS > 8)
			  case 9:
				windowItem = graph9;
				break;
		
			  case 10:
				windowItem = graph10;
				break;
	
			  case 11:
				windowItem = graph11;
				break;
		
			  case 12:
				windowItem = graph12;
				break;
#endif /*NGRAPHS > 8*/
	
			  default:
			  	;
			} /*switch (cmdNo)*/
		} /*if (!(mods & cmdKey)){}else{}*/

		if (editItem == copyit)
		{
			DoEdit(copyit);
		}
		else if (windowItem >= 0)
		{
			DoWind(windowItem);
		}
	} /*if (ch1 == FUNCTIONKEY)*/
	else if (mods & cmdKey)
	{
		if (ch == Period)
		{
			DoFile(interrupt);
		}
		else if(!GraphWindPaused && ch == OptionQ)
		{
			SaveOnQuit = false;
			quitIt();
		}
		else if (((*CommandMenu)->enableFlags & 1) &&
				 (cmdNo = optionKeyToNumber(ch)) > 0 && cmdNo <= NCOMMANDS)
		{
			DoCommand(command1+cmdNo-1); /*simulate menu selection */
		}
	} /*if(mods & cmdKey)*/ 
	else
	{
		if (ch == Return || ch == Enter)
		{
			if(GraphWindPaused)
			{ /* terminate pause */
				GraphWindPaused = false;
			}
			else
			{ /* switch to command window */
				DoWind((CmdWindows + CurrentWindow)->menuItem);
			}
		} /*if (ch == Return || ch == Enter)*/
	}/*if(mods & cmdKey){}else{}*/
	HUnlock((Handle) CommandMenu);
} /*GraphWindKey()*/

/*
	Update Graph window.
*/

pascal void GraphWindUpdate(Boolean resized)
{
	Rect            r;
	GrafPtr         thePort;
	PicHandle       picture;
	int             windno, panelno;
	WHERE("GraphWindUpdate");

	GetPort(&thePort);
	windno = whichGraphWindow((WindowPtr) thePort);
	if(windno >= 0)
	{
		r = thePort->portRect;
		EraseRect(&r);
		r.right -= RIGHTOFFSET;
	
		if (0 <= windno && windno < NGRAPHS)
		{
			if (resized)
			{		/* save current zoom state */
				GraphWInZoom[windno] = GraphWindZoomStatus(windno);
			}
			picture =	myPic[windno];
		} /*if (0 <= windno && windno < NGRAPHS)*/
		else
		{ /* must be panel */
			panelno = windno - NGRAPHS;
			if(PanelPic[panelno] == (PicHandle) 0)
			{ /* create picture for panel */
				drawPanelPicture(panelno);
			} /*if(PanelPic[panelno] == (PicHandle) 0)*/
			picture = PanelPic[panelno];
			r = PanelWind[panelno]->portRect;
			setPanelRect(&r, panelno);
		}
		DrawPicture(picture, &r);
		DrawGrowBox(thePort);
	} /*if(windno >= 0)*/
}/*GraphWindUpdate()*/

/*
	Used only when pausing with Graph window forward
	Ignores all events except mouseDown in Close Box, Menu bar or
	  another application
*/

static pascal Boolean GraphWindEventHook(EventRecord *theEvent)
{
	Integer     evtPart;
	GrafPtr     evtPort;
	Point       evtPt;

	if(theEvent->what == mouseDown)
	{
		/* Don't allow user to switch windows, but allow access to menu bar */
		evtPt = theEvent->where;
		evtPart = FindWindow(evtPt, &evtPort);
		switch(evtPart)
		{
		  case inSysWindow:
		  case inGoAway:
		  case inMenuBar:
			return (false);
		  default:
			SysBeep(30);
			return (true);
		} /*switch(evtPart)*/
	} /*if(TheEvent->what == mouseDown)*/
	return (false);
} /*GraphWindEventHook()*/

void GraphWindPause(void)
{
	SkelEventHookProcPtr   oldHook = SkelGetEventHook();
	WHERE("GraphWindPause");

	SkelSetEventHook((SkelEventHookProcPtr) GraphWindEventHook);
	GraphWindPaused = true;
	setGraphFileItems(true);
	DisableItem(WindowMenu, (Integer) 0);

	DisableItem(CommandMenu, (Integer) 0);
	DisableItem(OptionsMenu, (Integer) 0);

	DrawMenuBar();

	while(GraphWindPaused)
	{
		SkelDoEvents(everyEvent ^ keyUpMask);
	}
	SkelSetEventHook(oldHook);

/* Note: we only get here by pausing in the middle of a plot command */
	Running = 0;
	setGraphFileItems(true);
	Running = 1;
	EnableItem(WindowMenu, (Integer) 0);
	EnableItem(CommandMenu, (Integer) 0);
	EnableItem(OptionsMenu, (Integer) 0);
	DrawMenuBar();

} /*GraphWindPause()*/

/*
	960823  Fixed bug that prevented Panel 5-8 from being printed
*/
void GraphWindPrint(Integer windno)
{
	GrafPtr     thePort;
	Rect        r;
	WHERE("GraphWindPrint");
	
	if(0 <= windno && windno < NGRAPHS + NPANELS) /* be paranoid */
	{
		GetPort(&thePort);
		PictWindPrint(windno);
		r = thePort->portRect;
		InvalRect(&r);
		BeginUpdate((WindowPtr) thePort);
		GraphWindUpdate((Boolean) 0);
		EndUpdate((WindowPtr) thePort);
	}
} /*GraphWindPrint()*/

void GraphWindEditMenu(Integer item, WindowPtr whichWindow)
{
	PicHandle          thisPicture = (PicHandle) 0;
	Integer            windno;
	WHERE("GraphWindEditMenu");

	switch (item)
	{
	  case undoit:			/* only works in cmd window */
		break;
		
	  case cutit:				/* only works in cmd window */
		break;

	  case copyit:
		windno = whichGraphWindow(whichWindow);
		if(windno >= NGRAPHS)
		{
			thisPicture = PanelPic[windno-NGRAPHS];
		}
		else if(windno >= 0)
		{
			thisPicture = myPic[windno];
		}

		if(thisPicture != (PicHandle) 0)
		{
			LongInt      answer;

			(void) ZeroScrap();
			HLock((Handle) thisPicture);
			answer = PutScrap((LongInt) GetHandleSize((Handle) thisPicture),
				(unsigned LongInt) 'PICT', (Ptr) *thisPicture);

			HUnlock((Handle) thisPicture);
		}
		break;

	  case pastit:				/* only works in cmd window */
		break;

	  case copytoend:			/* only works in cmd window */
		break;

	  case copyandexecute:		/* only works in cmd window */
		break;

	 } /*switch (item)*/
} /*GraphWindEditMenu()*/

void setGraphHelpItems(void)
{
	unsigned char     *helpMenuEntry = "\pHelp";

	SetItem(HelpMenu, HelpItemNumber, helpMenuEntry);

} /*setGraphHelpItems()*/

/* routine to set File menu items for graph window */
void setGraphFileItems(Boolean active)
{
	if(active)
	{
		EnableItem(FileMenu, saveitas);
		SetItem(FileMenu,saveitas, "\pSave Graph As…");
		EnableItem(FileMenu, pagesetup);
		EnableItem(FileMenu, printit);
		SetItem(FileMenu, printit, "\pPrint Graph…");
		EnableItem(FileMenu, interrupt);
		if (!GraphWindPaused)
		{
			if (!Running && BDEPTH <= 0)
			{
				EnableItem(HelpMenu, HelpItemNumber);
				EnableItem(FileMenu, keepit);
				EnableItem(FileMenu, keepitas);
				EnableItem(FileMenu, quit);
			}
			DisableItem(FileMenu, go_on);
		}	/*if(!GraphWindPaused)*/
		else
		{
			DisableItem(HelpMenu, HelpItemNumber);
			EnableItem(FileMenu, go_on);
			DisableItem(FileMenu, keepit);
			DisableItem(FileMenu, keepitas);
			DisableItem(FileMenu, quit);
		} /*if(!GraphWindPaused){}else{}*/
	} /*if(active)*/
	else
	{
		DisableItem(FileMenu, go_on);
	} /*if(active){}else{}*/
} /*setGraphFileItems()*/

void setGraphEditItems(void)
{
	DisableItem(EditMenu, undoit);
	DisableItem(EditMenu, cutit);
	DisableItem(EditMenu, pastit);
	DisableItem(EditMenu, copytoend);
	EnableItem(EditMenu, copyit);
} /*setGraphEditItems()*/
