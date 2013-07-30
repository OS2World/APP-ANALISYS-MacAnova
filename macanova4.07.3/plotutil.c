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
#pragma segment PlotUtil
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
#include "mvsave.h"

#ifdef MACINTOSH
#include "macIface.h"
#endif /*MACINTOSH*/

#ifdef WXWIN
#include "wx/wxIface.h"
#endif  /* WXWIN */

#include "plot.h"

#ifdef MPW
#include <Memory.h>
#endif

#define ALWAYSPROLOG /* if defined, PS prolog always written*/
/*
   960502 changed fopen() to fmyopen() throughout
   960524 Added code to implement epsf plots
   960528 Added global ps_aspect
   960603 PostScript prolog always written
   970105 added support for keyword height and width
   970506 Used macros for specifying modes for fmyopen()
   980713 Fixed bugs in ascii restorePoints() and savePoints()
   980714 Changed type and arguments to restoreGraph()
   980715 Made substantial changes implementing new format for
          saving PLOTINFO symbols.  In particular, xticks, yticks
          xticklength, yticklength, and foreground, background and
		  frame colors are saved and restored.
   980820 Fixed bug in unsetPlotKeys() which crashed BC version when
          xmin, xmax, ymin, or ymax was MISSING
   990226 Replaced most uses of putOUTSTR() by putErrorOUTSTR()
*/

#define Binary     (ASCII < 0)
#define Pre407Save (ASCII > SAVEBINARY7 && ASCII < SAVEASCII7)

#ifndef DEFAULTASPECT
#define DEFAULTASPECT   1.6 /*aspect ratio of standard MacAnova graph window*/
#endif /*DEFAULTASPECT*/

extern long        ps_portrait; /* defined in term.c */
extern double      ps_aspect;   /* defined in term.c */

void set_point(curve_points **plot, long ipoint, double x, double y,
			   char *string, unsigned char pointColor, unsigned char lineColor)
{
	WHERE("set_point");

	POINTUNDEF(plot,ipoint) = 0;
	POINTPCOLOR(plot,ipoint) = pointColor;
	POINTLCOLOR(plot,ipoint) = lineColor;
	XCOORD(plot,ipoint) = x;
	YCOORD(plot,ipoint) = y;

	if (string == (char *) 0)
	{
		POINTSTRING(plot,ipoint)[0] = '\0';
	}
	else
	{
		strncpy(POINTSTRING(plot,ipoint), string, PLOTSTRINGSIZE);
		POINTSTRING(plot,ipoint)[PLOTSTRINGSIZE] = '\0';
	}

} /*set_point()*/

void undef_point(curve_points **plot,long ipoint)
{
	POINTUNDEF(plot,ipoint) = 1;
	POINTPCOLOR(plot,ipoint) = 0;
	POINTLCOLOR(plot,ipoint) = 0;
	XCOORD(plot,ipoint) = 0.0;
	YCOORD(plot,ipoint) = 0.0;
	POINTSTRING(plot,ipoint)[0] = '\0';
} /*undef_point()*/


#define MAXDUMPCOUNT    10000L

#if (0) /* following are in plot.h */
enum screenDumpErrorCodes
{
	SCREENDUMPOK        =  1,
	SCREENDUMPUNDEFINED =  0,
	BADSCREENMODE       = -1,
	BADCOLORS           = -2,
	BADWIDTH            = -3,
	CANTOPEN            = -4,
	TOOMANYDUMPS        = -5,
	FILEERROR           = -6
};
extern long     PutProlog;
extern long     term;
extern long     DumbHeight;
extern long     DumbWidth;
extern long     PauseAfterPlot;
#endif /*0*/

short  ScreenDumpError = SCREENDUMPOK;
char  *ScreenDumpFile = (char *) 0;

static void screenDumpMsg(void)
{
	*OUTSTR = '\0';
	
	switch (ScreenDumpError)
	{
	  case SCREENDUMPOK:
		return;

	  case SCREENDUMPUNDEFINED:
		sprintf(OUTSTR,
				"ERROR: screen dump is not available in this plotting mode");
		break;

	  case BADSCREENMODE:
	  case BADCOLORS:
	  case BADWIDTH:
		sprintf(OUTSTR,
				"ERROR: screen characteristics make screen dump impossible");
		break;

	  case CANTOPEN:
	  case FILEERROR:

		putPieces((ScreenDumpError == CANTOPEN) ? "ERROR: can't open file " :
				  "ERROR: error writing file ",
				  ScreenDumpFile, ".", (char *) 0);
		break;

	  case TOOMANYDUMPS:
		sprintf(OUTSTR,
				"ERROR: more than %ld screen dumps",MAXDUMPCOUNT - 1);
		break;
		
	  default:
		;
	} /*switch (ScreenDumpError)*/
	putErrorOUTSTR();
} /*screenDumpMsg()*/

/*
   960619 added components with tick information to graph
   970105 added support for keyword height and width
*/
long do_whole_graph(whole_graph ** graph, FILE * fp,
					long termType, long windno, plotKeyValuesPtr keyValues)
{
	double              value, offset;
	long                size;
	double              xmin = GRAPHXMIN(graph), xmax = GRAPHXMAX(graph);
	double              ymin = GRAPHYMIN(graph), ymax = GRAPHYMAX(graph);
	double            **xticksH = GRAPHXTICKS(graph);
	double            **yticksH = GRAPHYTICKS(graph);
	short               nxticks = GRAPHNXTICKS(graph);
	short               nyticks = GRAPHNYTICKS(graph);
	short               xticklength, yticklength;
	double             *xticks = (xticksH != (double **) 0) ?
								  *xticksH : (double *) 0;
	double             *yticks = (yticksH != (double **) 0) ?
								  *yticksH : (double *) 0;
	long                plotOK;
#ifdef MULTIPLOTWINDOWS
	long                prevWindow;
#endif /*MULTIPLOTWINDOWS*/
	WHERE("do_whole_graph");

	xticklength = GRAPHXTICKLENGTH(graph);
	if (xticklength > 2*TICKUNIT && xticklength != GRIDLINES)
	{
		xticklength = -(xticklength - 2*TICKUNIT);
	}

	yticklength = GRAPHYTICKLENGTH(graph);
	if (yticklength > 2*TICKUNIT && yticklength != GRIDLINES)
	{
		yticklength = -(yticklength - 2*TICKUNIT);
	}

	if (termType == PSTERM)
	{
		ps_portrait = !keyValues->landscape;
		ps_aspect = DEFAULTASPECT; /*ratio xmax/xmin*/
	}
	else if (keyValues->landscape)
	{
		putOutErrorMsg("WARNING: landscape mode ignored except for PostScript output");
		keyValues->landscape = 0;
	}

	term = termType;

	PLOTFILE = fp;

	if (termType != DUMBTERM)
	{
		value = .02 * (xmax - xmin);
		if (xmin == (xmin - value) || xmax == xmax + value)
		{
			value = 1.0;
		}
		xmin -= value;
		xmax += value;
		value = .02 * (ymax - ymin);
		if (ymin == (ymin - value) || ymax == ymax + value)
		{
			value = 1.0;
		}
		ymin -= value;
		ymax += value;
	} /*if (termType != DUMBTERM)*/
	else
	{
		/* DumbWidth is a global used in term.c*/
		DumbWidth = (keyValues->width != UNSETLONG) ? keyValues->width : SCREENWIDTH;
		DumbWidth = (DumbWidth >= MINSCREENWIDTH) ?
			DumbWidth : DEFAULTSCREENWIDTH;;
		size = DumbWidth - 1;
		offset = 11.0;
		value = (xmax - xmin) / (size - 3.0 - offset);
		if (value != 0 && fabs(xmin)/value < 1e10 && fabs(xmax)/value < 1e10)
		{
			xmin -= 1.00001*value;
			xmax += 0.99999*value;
		}
		else
		{
			xmin -= 1.0;
			xmax += 1.0;
		}

		/* DumbHeight is a global used in term.c*/
		DumbHeight = (keyValues->height != UNSETLONG) ? keyValues->height : SCREENHEIGHT;
		DumbHeight = (DumbHeight >= MINSCREENHEIGHT) ?
			DumbHeight : DEFAULTSCREENHEIGHT;
		size = DumbHeight - 1;
		offset = 2.0;
		value = (ymax - ymin) / (size - 3.0 - offset);
		if (value != 0 && fabs(ymin)/value < 1e10 && fabs(ymax)/value < 1e10)
		{
			ymin -= 1.00001*value;
			ymax += 0.99999*value;
		}
		else
		{
			ymin -= 1.0;
			ymax += 1.0;
		}
#ifndef SCROLLABLEWINDOW
		if (BDEPTH > 0 && SCREENHEIGHT > 0)
		{
			NLINES = SCREENHEIGHT;
			incrementNLINES("\n");
		}
#endif /*SCROLLABLEWINDOW*/
	} /*if (termType != DUMBTERM){}else{}*/

#ifdef MULTIPLOTWINDOWS
	prevWindow = ThisWindow;
	if (windno < 0)
	{
		ThisWindow = windno;
	}
	else if (windno > 0)
	{
		ThisWindow = windno - 1;
	}
/* if windno == 0, do not change ThisWindow */
#endif /*MULTIPLOTWINDOWS*/

	PauseAfterPlot = keyValues->pause;

	ScreenDumpError = SCREENDUMPOK;
	ScreenDumpFile = (keyValues->screendump != UNSETPOINTER) ?
	  keyValues->screendump : (char *) 0;

	plotOK = !do_plot(GRAPHFIRST_PLOT(graph), GRAPHNPLOTS(graph),
						GRAPHFIRST_STRING(graph), GRAPHNSTRINGS(graph),
					xmin, xmax, ymin, ymax,
					xticks, yticks, nxticks, nyticks,
					xticklength, yticklength,
					GRAPHLOGX(graph), GRAPHLOGY(graph),
					GRAPHXAXIS(graph), GRAPHYAXIS(graph),
					GRAPHXLAB(graph), GRAPHYLAB(graph), GRAPHTITLE(graph));
#ifdef EPSFPLOTSOK
	if (termType == PREVIEWTERM)
	{
		term = PSTERM;
		plotOK = !do_plot(GRAPHFIRST_PLOT(graph), GRAPHNPLOTS(graph),
							GRAPHFIRST_STRING(graph), GRAPHNSTRINGS(graph),
						xmin, xmax, ymin, ymax,
						xticks, yticks, nxticks, nyticks,
						xticklength, yticklength,
						GRAPHLOGX(graph), GRAPHLOGY(graph),
						GRAPHXAXIS(graph), GRAPHYAXIS(graph),
						GRAPHXLAB(graph), GRAPHYLAB(graph), GRAPHTITLE(graph));
		ThisWindow = prevWindow;
	}
#endif /*EPSFPLOTSOK*/

#ifdef ACTIVEUPDATE
	if (ActiveUpdate && keyValues->pause && termType == STANDARDTERM)
	{
		updateConsole();
	}
#endif /*ACTIVEUPDATE*/

	if (keyValues->screendump != UNSETPOINTER)
	{
		screenDumpMsg();
	}
	ScreenDumpFile = (char *) 0;
	
#ifdef MULTIPLOTWINDOWS
	if (!plotOK)
	{
		ThisWindow = prevWindow;
	}
#endif /*MULTIPLOTWINDOWS*/

	PLOTFILE = (FILE *) 0;
	return (plotOK);

} /*do_whole_graph()*/

/*
  For this to work, as a struct whole_graph is being constructed, it should
  be kept "up-to-date", incrementing GRAPHNPLOTS(graphs) every time a
  (struct curve_points **) plot is added and zeroing NEXTGRAPH(plot) and
  GRAPHPOINTS(plots).  That way, all handles are either zero or are the values
  of valid handles.
*/
void deleteGraph(whole_graph ** graph)
{
	curve_points       **this_plot, **next_plot;
	plot_string        **this_string, **next_string;
	long                 i, nplots, nstrings;
	WHERE("deleteGraph");

	if (graph != (whole_graph **) 0)
	{
		next_plot = GRAPHFIRST_PLOT(graph);
		nplots = GRAPHNPLOTS(graph);
		if (GRAPHXTICKS(graph) != (double **) 0)
		{
			mydisphandle((char **) GRAPHXTICKS(graph));
		}
		if (GRAPHYTICKS(graph) != (double **) 0)
		{
			mydisphandle((char **) GRAPHYTICKS(graph));
		}

		for (i = 0;i < nplots;i++)
		{
			this_plot = next_plot;
			next_plot = NEXTGRAPH(this_plot);
			mydisphandle((char **) GRAPHPOINTS(this_plot));
			mydisphandle((char **) this_plot);
		} /*for (i = 0;i<nplots;i++)*/

		nstrings = GRAPHNSTRINGS(graph);
		next_string = GRAPHFIRST_STRING(graph);
		for (i = 0;i < nstrings;i++)
		{
			this_string = next_string;
			next_string = NEXTSTRING(this_string);
			mydisphandle(PLOTSTRING(this_string));
			mydisphandle((char **) this_string);
		} /*for (i = 0;i<nstrings;i++)*/
		mydisphandle((char **) graph);
	} /*if (graph != (whole_graph **) 0)*/
} /*deleteGraph()*/

whole_graph **copyGraph(whole_graph **graphIn)
{
	Symbolhandle         graphTrash = (Symbolhandle) 0;
	whole_graph        **graphOut;
	curve_points       **next_plot = GRAPHFIRST_PLOT(graphIn);
	curve_points       **this_plot = (curve_points **) 0;
	curve_points       **out_plot = (curve_points **) 0;
	curve_points       **last_plot = (curve_points **) 0;
	plot_string        **next_string = GRAPHFIRST_STRING(graphIn);
	plot_string        **this_string = (plot_string **) 0;
	plot_string        **out_string = (plot_string **) 0;
	plot_string        **last_string = (plot_string **) 0;
	long                 i, nplots = GRAPHNPLOTS(graphIn);
	long                 nstrings = GRAPHNSTRINGS(graphIn);
	WHERE("copyGraph");

	graphTrash = Install(SCRATCH,PLOTINFO);
	TMPHANDLE = myduphandle((char **) graphIn);
	if (graphTrash == (Symbolhandle) 0 || TMPHANDLE == (char **) 0)
	{
		goto errorExit;
	}

	graphOut = (whole_graph **) TMPHANDLE;
	GRAPHXTICKS(graphOut) = GRAPHYTICKS(graphOut) = (double **) 0;
	if (GRAPHXTICKS(graphIn) != (double **) 0)
	{
		TMPHANDLE = myduphandle((char **) GRAPHXTICKS(graphIn));
		GRAPHXTICKS(graphOut) = (double **)TMPHANDLE;
		if (TMPHANDLE == (char **) 0)
		{
			goto errorExit;
		}
	} /*if (GRAPHXTICKS(graphIn) != (double **) 0)*/

	if (GRAPHYTICKS(graphIn) != (double **) 0)
	{
		TMPHANDLE = myduphandle((char **) GRAPHYTICKS(graphIn));
		GRAPHYTICKS(graphOut) = (double **) TMPHANDLE;
		if (TMPHANDLE == (char **) 0)
		{
			goto errorExit;
		}
	} /*if (GRAPHYTICKS(graphIn) != (double **) 0)*/
	
	GRAPHNPLOTS(graphOut) = 0;
	GRAPHFIRST_PLOT(graphOut) = (curve_points **) 0;
	GRAPHNSTRINGS(graphOut) = 0;
	GRAPHFIRST_STRING(graphOut) = (plot_string **) 0;
	setGRAPH(graphTrash,graphOut);
	for (i = 0;i < nplots;i++)
	{
		this_plot = next_plot;
		next_plot = NEXTGRAPH(this_plot);
		TMPHANDLE = myduphandle((char **) this_plot);
		out_plot = (curve_points **) TMPHANDLE;
		if (TMPHANDLE == (char **) 0)
		{
			goto errorExit;
		}
		if (i == 0)
		{
			GRAPHFIRST_PLOT(graphOut) = out_plot;
		}
		else
		{
			NEXTGRAPH(last_plot) = out_plot;
		}
		GRAPHNPLOTS(graphOut)++;
		GRAPHPOINTS(out_plot) = (coordinate **) 0;
		NEXTGRAPH(out_plot) = (curve_points **) 0;
		last_plot = out_plot;

		TMPHANDLE = myduphandle((char **) GRAPHPOINTS(this_plot));
		GRAPHPOINTS(out_plot) = (coordinate **) TMPHANDLE;
		if (TMPHANDLE == (char **) 0)
		{
			goto errorExit;
		}
	} /*for (i = 0;i< nplots;i++)*/

	for (i=0;i<nstrings;i++)
	{
		this_string = next_string;
		next_string = NEXTSTRING(this_string);
		TMPHANDLE = myduphandle((char **) this_string);
		out_string = (plot_string **) TMPHANDLE;
		if (TMPHANDLE == (char **) 0)
		{
			goto errorExit;
		}
		PLOTSTRING(out_string) = (char **) 0;
		NEXTSTRING(out_string) = (plot_string **) 0;
		GRAPHNSTRINGS(graphOut)++;
		if (i == 0)
		{
			GRAPHFIRST_STRING(graphOut) = out_string;
		}
		else
		{
			NEXTSTRING(last_string) = out_string;
		}
		last_string = out_string;
		TMPHANDLE = myduphandle((char **) PLOTSTRING(this_string));
		PLOTSTRING(out_string) = TMPHANDLE;
		if (TMPHANDLE == (char **) 0)
		{
			goto errorExit;
		}
	} /*for (i=0;i<nstrings;i++)*/

	setGRAPH(graphTrash,(whole_graph **) 0);
	Removesymbol(graphTrash);

	return (graphOut);

  errorExit:
	Removesymbol(graphTrash);

	return (0);
} /*copyGraph()*/

/*
   function to determin actual minimum and maximum of points and
   strings in **graph
*/
void getGraphExtremes(whole_graph ** graph, double *extremes, long pointsOnly)
{
	curve_points       **next_plot = GRAPHFIRST_PLOT(graph);
	curve_points       **this_plot = (curve_points **) 0;
	plot_string        **next_string = GRAPHFIRST_STRING(graph);
	plot_string        **this_string = (plot_string **) 0;
	long                 iplot, nplots = GRAPHNPLOTS(graph);
	long                 ipoint, npoints;
	long                 istring, nstrings = GRAPHNSTRINGS(graph);
	double               hugedbl = HUGEDBL;
	double               xmin = hugedbl, ymin = hugedbl;
	double               xmax = -hugedbl, ymax = -hugedbl;
	double               x, y;
	WHERE("getGraphExtremes");
	
	for (iplot = 0; iplot < nplots; iplot++)
	{
		this_plot = next_plot;
		next_plot = NEXTGRAPH(this_plot);
		npoints = PCOUNT(this_plot);
		for (ipoint = 0; ipoint < npoints; ipoint++)
		{
			if (!POINTUNDEF(this_plot, ipoint))
			{
				x = XCOORD(this_plot, ipoint);
				y = YCOORD(this_plot, ipoint);
				xmax = (x > xmax) ? x : xmax;
				xmin = (x < xmin) ? x : xmin;
				ymax = (y > ymax) ? y : ymax;
				ymin = (y < ymin) ? y : ymin;
			} /*if (!POINTUNDEF(this_plot, ipoint))*/
		} /*for (ipoint = 0; i < npoints; ipoint++)*/
	} /*for (iplot = 0; iplot < nplots; iplot++)*/

	if (!pointsOnly)
	{
		for (istring = 0; istring < nstrings; istring++)
		{
			this_string = next_string;
			next_string = NEXTSTRING(this_string);
			x = STRINGX(this_string);
			y = STRINGY(this_string);
			xmax = (x > xmax) ? x : xmax;
			xmin = (x < xmin) ? x : xmin;
			ymax = (y > ymax) ? y : ymax;
			ymin = (y < ymin) ? y : ymin;
		} /*for (istring = 0; istring < nstrings; i++)*/
	} /*if (!pointsOnly)*/
	
	extremes[0] = xmin;
	extremes[1] = xmax;
	extremes[2] = ymin;
	extremes[3] = ymax;
} /*getGraphExtremes()*/

/*
  Check argument list for plotting parameters returning their values.
  StartKey (will always be > 0) is where to start looking.
  The return value is the index of the earliest keyword matched, or 0
  if none is matched.
  -1 is returned if an error is found.
*/

enum plotKeys
{
	KTITLE = 0,
	KXLAB,
	KYLAB,
	KXMIN,
	KYMIN,
	KXMAX,
	KYMAX,
	KXTICKLENGTH,
	KYTICKLENGTH,
	KXTICKS,
	KYTICKS,
	KLOGX,         /* plot logs of x (not implemented)*/
	KLOGY,         /* plot logs of y (not implemented)*/
	KXAXIS,        /* != => draw y = 0 line */
	KYAXIS,        /* != => draw x = 0 line */
	KDUMB,         /* character plot */
	KHEIGHT,       /* number of lines in character plot */
	KWIDTH,        /* number of columns in character plot */
	KKEEP,         /* save plot in variable LASTPLOT */
	KSHOW,         /* don't display plot if F */
	KADD,          /* add to existing plot */
	KPS,           /* write PostScript file if T */
	KEPSF,         /* write encapsulated PostScript file (not fully implemented) */
	KNEW,          /* rewind output file if T */
	KFILE,         /* write to file */
	KJUSTIFY,      /* justification code */
	KWINDOW,       /* window number */
	KPAUSE,        /* pause after plot if T */
	KIMPULSE,      /* draw lines from x-axis or edge to points */
	KLINES,        /* connect points with lines */
	KLINETYPE,     /* line type, integer */
	KTHICKNESS,    /* line thickness, REAL >= 1.0 */
	KLANDSCAPE,    /* landscape mode */
	KSCREENDUMP,   /* save the screen */
	KNOTES         /* notes to attach to PLOTINFO symbol */
};

#define PLOTSUBS  0x01

static keywordList    PlotKeys[] =
{
	InitKeyEntry("title",0,PLOTSUBS,CHARSCALAR),
	InitKeyEntry("xlab",0,PLOTSUBS,CHARSCALAR),
	InitKeyEntry("ylab",0,PLOTSUBS,CHARSCALAR),
	InitKeyEntry("xmin",0,PLOTSUBS,REALSCALAR),
	InitKeyEntry("ymin",0,PLOTSUBS,REALSCALAR),
	InitKeyEntry("xmax",0,PLOTSUBS,REALSCALAR),
	InitKeyEntry("ymax",0,PLOTSUBS,REALSCALAR),
	InitKeyEntry("xticklen",7,PLOTSUBS,NONMISSINGREAL),
	InitKeyEntry("yticklen",7,PLOTSUBS,NONMISSINGREAL),
	InitKeyEntry("xticks",5,PLOTSUBS,REALVECTORORNULL),
	InitKeyEntry("yticks",5,PLOTSUBS,REALVECTORORNULL),
	InitKeyEntry("logx",0,PLOTSUBS,LOGICSCALAR),
	InitKeyEntry("logy",0,PLOTSUBS,LOGICSCALAR),
	InitKeyEntry("xaxis",0,PLOTSUBS,LOGICSCALAR),
	InitKeyEntry("yaxis",0,PLOTSUBS,LOGICSCALAR),
	InitKeyEntry("dumb",0,PLOTSUBS,LOGICSCALAR),
	InitKeyEntry("height",0,PLOTSUBS,POSITIVEINT),
	InitKeyEntry("width",0,PLOTSUBS,POSITIVEINT),
	InitKeyEntry("keep",0,PLOTSUBS,LOGICSCALAR),
	InitKeyEntry("show",0,PLOTSUBS,LOGICSCALAR),
	InitKeyEntry("add",0,PLOTSUBS,LOGICSCALAR),
	InitKeyEntry("ps",0,PLOTSUBS,LOGICSCALAR),
	InitKeyEntry("epsf",3,PLOTSUBS,LOGICSCALAR),
	InitKeyEntry("new",0,PLOTSUBS,LOGICSCALAR),
	InitKeyEntry("file",0,PLOTSUBS,CHARSCALAR),
	InitKeyEntry("justify",4,PLOTSUBS,CHARSCALAR),
	InitKeyEntry("window",4,PLOTSUBS,NONNEGATIVEINT),
	InitKeyEntry("pause",0,PLOTSUBS,LOGICSCALAR),
	InitKeyEntry("impulses",7,PLOTSUBS,LOGICSCALAR),
	InitKeyEntry("lines",0,PLOTSUBS,LOGICSCALAR),
	InitKeyEntry("linetype",5,PLOTSUBS,POSITIVEINT),
	InitKeyEntry("thickness",5,PLOTSUBS,REALSCALAR),
	InitKeyEntry("landscape",4,PLOTSUBS,LOGICSCALAR),
	InitKeyEntry("screendump",7,PLOTSUBS,CHARSCALAR),
	InitKeyEntry("notes",4,PLOTSUBS,CHARVECTOR)
};

#define TitleKey      KeyCharValue(PlotKeys,KTITLE)
#define XlabKey       KeyCharValue(PlotKeys,KXLAB)
#define YlabKey       KeyCharValue(PlotKeys,KYLAB)
#define XminKey       KeyRealValue(PlotKeys,KXMIN)
#define YminKey       KeyRealValue(PlotKeys,KYMIN)
#define XmaxKey       KeyRealValue(PlotKeys,KXMAX)
#define YmaxKey       KeyRealValue(PlotKeys,KYMAX)
#define XtickLength   KeyRealValue(PlotKeys,KXTICKLENGTH)
#define YtickLength   KeyRealValue(PlotKeys,KYTICKLENGTH)
#define XticksKey     KeySymhValue(PlotKeys,KXTICKS)
#define YticksKey     KeySymhValue(PlotKeys,KYTICKS)
#define LogxKey       KeyLogValue(PlotKeys,KLOGX)
#define LogyKey       KeyLogValue(PlotKeys,KLOGY)
#define XaxisKey      KeyLogValue(PlotKeys,KXAXIS)
#define YaxisKey      KeyLogValue(PlotKeys,KYAXIS)
#define DumbKey       KeyLogValue(PlotKeys,KDUMB)
#define HeightKey     KeyIntValue(PlotKeys,KHEIGHT)
#define WidthKey      KeyIntValue(PlotKeys,KWIDTH)
#define KeepKey       KeyLogValue(PlotKeys,KKEEP)
#define ShowKey       KeyLogValue(PlotKeys,KSHOW)
#define AddKey        KeyLogValue(PlotKeys,KADD)
#define PsKey         KeyLogValue(PlotKeys,KPS)
#define EpsfKey       KeyLogValue(PlotKeys,KEPSF)
#define NewKey        KeyLogValue(PlotKeys,KNEW)
#define FileKey       KeyCharValue(PlotKeys,KFILE)
#define JustifyKey    KeyCharValue(PlotKeys,KJUSTIFY)
#define WindowKey     KeyIntValue(PlotKeys,KWINDOW)
#define PauseKey      KeyLogValue(PlotKeys,KPAUSE)
#define ImpulsesKey   KeyLogValue(PlotKeys,KIMPULSE)
#define LinesKey      KeyLogValue(PlotKeys,KLINES)
#define LinetypeKey   KeyIntValue(PlotKeys,KLINETYPE)
#define ThicknessKey  KeyRealValue(PlotKeys,KTHICKNESS)
#define LandscapeKey  KeyLogValue(PlotKeys,KLANDSCAPE)
#define ScreendumpKey KeyCharValue(PlotKeys,KSCREENDUMP)
#define NotesKey      KeySymhValue(PlotKeys,KNOTES)

/*
   Safe places to put complete file names
   It should be safe for calling programs to copy into the address returned
   by screendump and file
*/
#ifdef SCREENDUMP
static char           ScreenDumpPath[PATHSIZE+1];
#endif /*SCREENDUMP*/

static char           FilePath[PATHSIZE+1];

/*
  getPlotKeys returns -1 if an error is found
                       0 if none of the keywords are found
					   j if first keyword is in COMPVALUE(list,j - 1)

  WARNING: CHARACTER keys are returned as dereferenced handles.  They
  should be copied to local storage before any storage is allocated

   970105 added support for keywords height and width
*/

long getPlotKeys(Symbolhandle list, long startKey, plotKeyValuesPtr keyValues)
{
	long           first = 0, nargs = NARGS(list);
	char          *keyword, c;
	char          *keystring, *tmp;
	char           legalRange[40];
	long           nkeys = NKeys(PlotKeys);
	keywordListPtr keys = PlotKeys;
	long           keyStatus;
	WHERE("getPlotKeys");

	unsetKeyValues(keys, nkeys);

	for (first = startKey;first < nargs;first++)
	{ /* find first keyword argument */
		if (isKeyword(COMPVALUE(list,first)))
		{
			break;
		}
	} /*for (first = startKey;first < nargs;first++)*/
	
	if (first >= nargs)
	{ /*no keywords */
		return (0);
	}
	first++;
	/*
	   getAllKeyValues() loops over the possible keys.  By doing this rather
	   than looping over the elements of list, we get only the first
	   argument involving a given keyword.  Especially for the plotting
	   routines this is desirable, since a macro can provide default values
	   for keys by putting them at the end of the argument list to a
	   plotting command
		plot($1,$2,$K,title:paste("$2 vs $1",xlab:"$1",ylab:"$2"))
	   It is not an error to have duplicate keywords.
    */
	keyStatus = getAllKeyValues(list, startKey, PLOTSUBS, keys, nkeys);
	if (keyStatus < 0)
	{
		goto errorExit;
	}
	if (keyStatus == 0)
	{
		return (0);
	}
	
	if (charIsSet(TitleKey))
	{
		if (keyValues->title[0] == DONTSETCHAR)
		{
			keyword = KeyName(keys, KTITLE);
			goto illegalKey;
		}
		keystring = *TitleKey;
		strncpy(keyValues->title, keystring, TITLESIZE);
		keyValues->title[TITLESIZE] = '\0';
		if (strlen(keystring) > TITLESIZE)
		{
#ifdef MACINTOSH
			keyValues->title[TITLESIZE] = (unsigned char) 0xc9; /* '...' */
#else  /*MACINTOSH*/
			keyValues->title[TITLESIZE] = (unsigned char) '+';
#endif /*MACINTOSH*/
			keyValues->title[TITLESIZE+1] = '\0';
		}
	} /*if (charIsSet(TitleKey))*/

	if (charIsSet(XlabKey))
	{
		/* xlab should have length at least XLABELSIZE+2*/
		if (keyValues->xlab[0] == DONTSETCHAR)
		{
			keyword = KeyName(keys, KXLAB);
			goto illegalKey;
		}
		keystring = *XlabKey;
		strncpy(keyValues->xlab, keystring,XLABELSIZE+1);
		keyValues->xlab[XLABELSIZE+1] = '\0';
		if (strlen(keystring) > XLABELSIZE+1)
		{						/* add character to show label is incomplete */
#ifdef MACINTOSH
			keyValues->xlab[XLABELSIZE] = (unsigned char) 0xc9; /* '...' */
#else  /*MACINTOSH*/
			keyValues->xlab[XLABELSIZE] = (unsigned char) '+';
#endif /*MACINTOSH*/
		}
	} /*if (charIsSet(XlabKey))*/
	
	if (charIsSet(YlabKey))
	{
		/* ylab should have length at least YLABELSIZE+2*/
		if (keyValues->ylab[0] == DONTSETCHAR)
		{
			keyword = KeyName(keys, KYLAB);
			goto illegalKey;
		}
		keystring = *YlabKey;
		strncpy(keyValues->ylab, keystring,YLABELSIZE+1);
		keyValues->ylab[YLABELSIZE+1] = '\0';
		if (strlen(keystring) > YLABELSIZE+1)
		{						/* add character to show label is incomplete */
#ifdef MACINTOSH
			keyValues->ylab[YLABELSIZE] = (unsigned char) 0xc9; /* '...' */
#else  /*MACINTOSH*/
			keyValues->ylab[YLABELSIZE] = (unsigned char) '+';
#endif /*MACINTOSH*/
		}
	} /*if (charIsSet(YlabKey))*/
	
	if (realIsSet(XminKey))
	{
		if (keyValues->xmin == DONTSETREAL)
		{
			keyword = KeyName(keys, KXMIN);
			goto illegalKey;
		}
		keyValues->xmin = XminKey;
	} /*if (realIsSet(XminKey))*/
	
	if (realIsSet(XmaxKey))
	{
		if (keyValues->xmax == DONTSETREAL)
		{
			keyword = KeyName(keys, KXMAX);
			goto illegalKey;
		}
		keyValues->xmax = XmaxKey;
	} /*if (realIsSet(XmaxKey))*/
	
	if (realIsSet(YminKey))
	{
		if (keyValues->ymin == DONTSETREAL)
		{
			keyword = KeyName(keys, KYMIN);
			goto illegalKey;
		}
		keyValues->ymin = YminKey;
	} /*if (realIsSet(YminKey))*/
	
	if (realIsSet(YmaxKey))
	{
		if (keyValues->ymax == DONTSETREAL)
		{
			keyword = KeyName(keys, KYMAX);
			goto illegalKey;
		}
		keyValues->ymax = YmaxKey;
	} /*if (realIsSet(YmaxKey))*/
	
	if (realIsSet(XtickLength))
	{
		keyword = KeyName(keys, KXTICKLENGTH);
		if (keyValues->xticklength == DONTSETSHORT)
		{
			goto illegalKey;
		}
		if (XtickLength < -1.0)
		{
			sprintf(legalRange, "REAL >= -1");
			goto badNumber;
		}
		if (XtickLength <= 2.0)
		{
			XtickLength *= TICKUNIT;
			keyValues->xticklength = (unsigned short) (XtickLength >= 0) ?
				XtickLength : 2*TICKUNIT - XtickLength;
		}
		else
		{
			keyValues->xticklength = GRIDLINES;
		}
	} /*if (realIsSet(XtickLength))*/

	if (realIsSet(YtickLength))
	{
		keyword = KeyName(keys, KYTICKLENGTH);
		if (keyValues->yticklength == DONTSETSHORT)
		{
			goto illegalKey;
		}
		if (YtickLength < -1.0)
		{
			sprintf(legalRange, "REAL >= -1");
			goto badNumber;
		}
		if (YtickLength <= 2.0)
		{
			YtickLength *= TICKUNIT;
			keyValues->yticklength = (unsigned short) (YtickLength >= 0) ?
				YtickLength : 2*TICKUNIT - YtickLength;
		}
		else
		{
			keyValues->yticklength = GRIDLINES;
		}
	} /*if (realIsSet(YtickLength))*/

	if (symhIsSet(XticksKey))
	{
		keyword = KeyName(keys, KXTICKS);
		if (keyValues->xticks == DONTSETSYMH)
		{
			goto illegalKey;
		}
		if (!isScalar(XticksKey) && anyMissing(XticksKey))
		{
			goto foundMissing;
		}
		
		keyValues->xticks = XticksKey;
	} /*if (symhIsSet(XticksKey))*/
	
	if (symhIsSet(YticksKey))
	{
		keyword = KeyName(keys, KYTICKS);
		if (keyValues->yticks == DONTSETSYMH)
		{
			goto illegalKey;
		}
		if (!isScalar(YticksKey) && anyMissing(YticksKey))
		{
			goto foundMissing;
		}
		keyValues->yticks = YticksKey;
	} /*if (symhIsSet(YticksKey))*/
	
	if (logicalIsSet(LogxKey))
	{
		if (keyValues->logx == DONTSETLOGICAL)
		{
			keyword = KeyName(keys, KLOGX);
			goto illegalKey;
		}
		keyValues->logx = LogxKey;
	} /*if (logicalIsSet(LogxKey))*/
	
	if (logicalIsSet(LogyKey))
	{
		if (keyValues->logy == DONTSETLOGICAL)
		{
			keyword = KeyName(keys, KLOGY);
			goto illegalKey;
		}
		keyValues->logy = LogyKey;
	} /*if (logicalIsSet(LogyKey))*/
	
	if (logicalIsSet(XaxisKey))
	{
		if (keyValues->xaxis == DONTSETLOGICAL)
		{
			keyword = KeyName(keys, KXAXIS);
			goto illegalKey;
		}
		keyValues->xaxis = XaxisKey;
	} /*if (logicalIsSet(XaxisKey))*/
	
	if (logicalIsSet(YaxisKey))
	{
		if (keyValues->yaxis == DONTSETLOGICAL)
		{
			keyword = KeyName(keys, KYAXIS);
			goto illegalKey;
		}
		keyValues->yaxis = YaxisKey;
	} /*if (logicalIsSet(YaxisKey))*/
	
	if (logicalIsSet(DumbKey))
	{
		if (keyValues->dumb == DONTSETLOGICAL)
		{
			keyword = KeyName(keys, KDUMB);
			goto illegalKey;
		}
		keyValues->dumb = DumbKey;
	} /*if (logicalIsSet(DumbKey))*/
	
	if (longIsSet(HeightKey))
	{
		keyword = KeyName(keys, KHEIGHT);
		if (keyValues->height == DONTSETLONG)
		{
			goto illegalKey;
		}

		keyValues->height = HeightKey;
	} /*if (longIsSet(HeightKey))*/
	
	if (longIsSet(WidthKey))
	{
		keyword = KeyName(keys, KWIDTH);
		if (keyValues->width == DONTSETLONG)
		{
			goto illegalKey;
		}

		keyValues->width = WidthKey;
	} /*if (longIsSet(WidthKey))*/
	
	if (logicalIsSet(KeepKey))
	{
		if (keyValues->keep == DONTSETLOGICAL)
		{
			keyword = KeyName(keys, KKEEP);
			goto illegalKey;
		}
		keyValues->keep = KeepKey;
	} /*if (logicalIsSet(KeepKey))*/

	if (logicalIsSet(ShowKey))
	{
		if (keyValues->show == DONTSETLOGICAL)
		{
			keyword = KeyName(keys, KSHOW);
			goto illegalKey;
		}
		keyValues->show = ShowKey;
	} /*if (logicalIsSet(ShowKey))*/

	if (logicalIsSet(AddKey))
	{
		if (keyValues->add == DONTSETLOGICAL)
		{
			keyword = KeyName(keys, KADD);
			goto illegalKey;
		}
		keyValues->add = AddKey;
	} /*if (logicalIsSet(AddKey))*/

	if (logicalIsSet(PsKey))
	{
		if (keyValues->ps == DONTSETLOGICAL)
		{
			keyword = KeyName(keys, KPS);
			goto illegalKey;
		}
		keyValues->ps = PsKey;
	} /*if (logicalIsSet(PsKey))*/

	if (logicalIsSet(EpsfKey))
	{
		if (keyValues->epsf == DONTSETLOGICAL)
		{
			keyword = KeyName(keys, KEPSF);
			goto illegalKey;
		}
		keyValues->epsf = EpsfKey;
	} /*if (logicalIsSet(EpsfKey))*/

	if (logicalIsSet(NewKey))
	{
		if (keyValues->newFile == DONTSETLOGICAL)
		{
			keyword = KeyName(keys, KNEW);
			goto illegalKey;
		}
		keyValues->newFile = NewKey;
	} /*if (logicalIsSet(NewKey))*/

	if (charIsSet(FileKey))
	{
		if (keyValues->file == DONTSETPOINTER)
		{
			keyword = KeyName(keys, KFILE);
			goto illegalKey;
		}
		if (keyValues->file != UNSETPOINTER)
		{
			sprintf(OUTSTR,
					"ERROR: file name for %s() specified twice", FUNCNAME);
			goto errorExit;
		}

		if((tmp = expandFilename(*FileKey)) == (char *) 0)
		{
			goto errorExit;
		}
		strncpy(FilePath, tmp, PATHSIZE);
		keyValues->file = FilePath;
	} /*if (charIsSet(FileKey))*/

	if (charIsSet(JustifyKey))
	{
		keyword = KeyName(keys, KJUSTIFY);
		if (keyValues->justify == DONTSETCHAR)
		{
			goto illegalKey;
		}
		keystring = *JustifyKey;
		c = *keystring;
		switch ((int) c)
		{
		  case 'c':
		  case 'C':
			c = CENTRE;
			break;
		  case 'l':
		  case 'L':
			c = LEFT;
			break;
		  case 'r':
		  case 'R':
			c = RIGHT;
			break;
		  default:
			goto badString;
		} /*switch ((int) c)*/
		keyValues->justify = c;		
	} /*if (charIsSet(JustifyKey))*/

	if (longIsSet(WindowKey))
	{
		keyword = KeyName(keys, KWINDOW);
		if (keyValues->window == DONTSETLONG)
		{
			goto illegalKey;
		}
#ifdef MULTIPLOTWINDOWS
		if (WindowKey > NGRAPHS)
		{
			sprintf(legalRange,"integer between 0 and %ld",
					(long) NGRAPHS);
			goto badNumber;
		}
		keyValues->window = WindowKey;
#endif /*MULTIPLOTWINDOWS*/
	} /*if (longIsSet(WindowKey))*/
	
	if (logicalIsSet(PauseKey))
	{
		if (keyValues->pause == DONTSETLOGICAL)
		{
			keyword = KeyName(keys, KPAUSE);
			goto illegalKey;
		}
		keyValues->pause = PauseKey;
	} /*if (logicalIsSet(PauseKey))*/
	
	if (logicalIsSet(ImpulsesKey))
	{
		if (keyValues->impulses == DONTSETLOGICAL)
		{
			keyword = KeyName(keys, KIMPULSE);
			goto illegalKey;
		}
		keyValues->impulses = ImpulsesKey;
	} /*if (logicalIsSet(ImpulsesKey))*/
	
	if (logicalIsSet(LinesKey))
	{
		if (keyValues->lines == DONTSETLOGICAL)
		{
			keyword = KeyName(keys, KLINES);
			goto illegalKey;
		}
		keyValues->lines = LinesKey;
	} /*if (logicalIsSet(LinesKey))*/
	
	if (longIsSet(LinetypeKey))
	{
		keyword = KeyName(keys, KLINETYPE);
		if (keyValues->linetype == DONTSETLONG)
		{
			goto illegalKey;
		}
		if (LinetypeKey > (double) MAXLINETYPE)
		{
			sprintf(legalRange,"positive integer <= %ld",
					(long) MAXLINETYPE);
			goto badNumber;
		}

		keyValues->linetype = LinetypeKey;
	} /*if (longIsSet(LinetypeKey))*/
	
	if (realIsSet(ThicknessKey))
	{
		keyword = KeyName(keys, KTHICKNESS);
		if (keyValues->thickness == DONTSETREAL)
		{
			goto illegalKey;
		}
		keyValues->thickness = ThicknessKey;
		if (ThicknessKey < .1 || ThicknessKey > 10.0)
		{
			sprintf(legalRange,"between 0.1 and 10.0");
			goto badNumber;
		}
	} /*if (realIsSet(ThicknessKey))*/
	
	if (logicalIsSet(LandscapeKey))
	{
		if (keyValues->landscape == DONTSETLOGICAL)
		{
			keyword = KeyName(keys, KLANDSCAPE);
			goto illegalKey;
		}
		keyValues->landscape = LandscapeKey;
	} /*if (logicalIsSet(LandscapeKey))*/
	
	if (charIsSet(ScreendumpKey))
	{
		if (keyValues->screendump == DONTSETPOINTER)
		{
			keyword = KeyName(keys, KSCREENDUMP);
			goto illegalKey;
		}
#ifdef SCREENDUMP
		if ((tmp = expandFilename(*ScreendumpKey)) == (char *) 0)
		{
			goto errorExit;
		}
		strncpy(ScreenDumpPath, tmp, PATHSIZE);
		keyValues->screendump = ScreenDumpPath;
#endif /*SCREENDUMP*/
	} /*if (charIsSet(ScreendumpKey))*/

	if (symhIsSet(NotesKey))
	{
		keyword = KeyName(keys, KNOTES);
		if (keyValues->yticks == DONTSETSYMH)
		{
			goto illegalKey;
		}
		keyValues->notes = NotesKey;
	} /*if (symhIsSet(NotesKey))*/
	
	return (first);

  illegalKey:
	badKeyword(FUNCNAME, keyword);
	goto errorExit;

  badString:
	sprintf(OUTSTR,"ERROR: '%s' is not legal value for %s",keystring,
			keyword);
	goto errorExit;

  badNumber:
	sprintf(OUTSTR,"ERROR: value for %s must be %s",keyword,legalRange);
	goto errorExit;

  foundMissing:
	sprintf(OUTSTR,
			"ERROR: no elements of non-scalar value for %s may be MISSING",
			keyword);
	/* fall through */

  errorExit:
	putErrorOUTSTR();

	return (-1);
} /*getPlotKeys()*/

void unsetPlotKeys(plotKeyValuesPtr keyValues, long all)
{
	if (all)
	{
		keyValues->title[0] = '\0';
		keyValues->xlab[0] = '\0';
		keyValues->ylab[0] = '\0';
	
		keyValues->xmin = keyValues->xmax = UNSETREAL;
		keyValues->ymin = keyValues->ymax = UNSETREAL;
	
		keyValues->xticklength = keyValues->yticklength = UNSETSHORT;
	
		keyValues->xticks = keyValues->yticks = UNSETSYMH;
	
		keyValues->logx = keyValues->logy = UNSETCHAR;
		keyValues->xaxis = keyValues->yaxis = UNSETCHAR;
		keyValues->dumb = keyValues->keep = UNSETCHAR;
		keyValues->show = keyValues->add = UNSETCHAR;
		keyValues->pause = keyValues->impulses = keyValues->lines = UNSETCHAR;
		keyValues->landscape = keyValues->ps = keyValues->epsf = UNSETCHAR;
		keyValues->newFile = UNSETCHAR;
		keyValues->window = UNSETLONG;
		keyValues->height = keyValues->width = UNSETLONG;
	
		keyValues->justify = keyValues->linetype = UNSETLONG;
		keyValues->thickness = UNSETREAL;
	
		keyValues->file = keyValues->screendump = UNSETPOINTER;

		keyValues->xticks = keyValues->notes = UNSETSYMH;
	}
	else
	{
		keyValues->title[0] = (keyValues->title[0] != DONTSETCHAR) ?
		  keyValues->title[0] : '\0';
		keyValues->xlab[0] = (keyValues->xlab[0] != DONTSETCHAR) ?
		  keyValues->xlab[0] : '\0';
		keyValues->ylab[0] = (keyValues->ylab[0] != DONTSETCHAR) ?
		  keyValues->ylab[0] : '\0';

		keyValues->xmin =
		  (isMissing(keyValues->xmin) || keyValues->xmin != DONTSETREAL) ?
		  keyValues->xmin : UNSETREAL;
		keyValues->xmax =
		  (isMissing(keyValues->xmax) || keyValues->xmax != DONTSETREAL) ?
		  keyValues->xmax : UNSETREAL;
		keyValues->ymin =
		  (isMissing(keyValues->ymin) || keyValues->ymin != DONTSETREAL) ?
		  keyValues->ymin : UNSETREAL;
		keyValues->ymax =
		  (isMissing(keyValues->ymax) || keyValues->ymax != DONTSETREAL) ?
		  keyValues->ymax : UNSETREAL;

		keyValues->xticklength = (keyValues->xticklength != DONTSETSHORT) ?
		  keyValues->xticklength : UNSETSHORT;
		keyValues->yticklength = (keyValues->yticklength != DONTSETSHORT) ?
		  keyValues->yticklength : UNSETSHORT;
	
		keyValues->xticks = (keyValues->xticks != DONTSETSYMH) ?
		  keyValues->xticks : UNSETSYMH;
		keyValues->yticks = (keyValues->yticks != DONTSETSYMH) ?
		  keyValues->yticks : UNSETSYMH;
	
		keyValues->logx = (keyValues->logx != DONTSETCHAR) ?
		  keyValues->logx : UNSETCHAR;
		keyValues->logy = (keyValues->logy != DONTSETCHAR) ?
		  keyValues->logy : UNSETCHAR;
		  
		keyValues->xaxis = (keyValues->xaxis != DONTSETCHAR) ?
		  keyValues->xaxis : UNSETCHAR;
		keyValues->yaxis = (keyValues->yaxis != DONTSETCHAR) ?
		  keyValues->yaxis : UNSETCHAR;
		  
		keyValues->dumb = (keyValues->dumb != DONTSETCHAR) ?
		  keyValues->dumb : UNSETCHAR;
		keyValues->keep = (keyValues->keep != DONTSETCHAR) ?
		  keyValues->keep : UNSETCHAR;
		  
		keyValues->show = (keyValues->show != DONTSETCHAR) ?
		  keyValues->show : UNSETCHAR;
		keyValues->add = (keyValues->add != DONTSETCHAR) ?
		  keyValues->add : UNSETCHAR;
		  
		keyValues->pause = (keyValues->pause != DONTSETCHAR) ?
		  keyValues->pause : UNSETCHAR;
		keyValues->impulses = (keyValues->impulses != DONTSETCHAR) ?
		  keyValues->impulses : UNSETCHAR;
		keyValues->lines = (keyValues->lines != DONTSETCHAR) ?
		  keyValues->lines : UNSETCHAR;

		keyValues->landscape = (keyValues->landscape != DONTSETCHAR) ?
		  keyValues->landscape : UNSETCHAR;
		keyValues->ps = (keyValues->ps != DONTSETCHAR) ?
		  keyValues->ps : UNSETCHAR;
		keyValues->epsf = (keyValues->epsf != DONTSETCHAR) ?
		  keyValues->epsf : UNSETCHAR;
		  
		keyValues->newFile = (keyValues->newFile != DONTSETCHAR) ?
		  keyValues->newFile : UNSETCHAR;
		  
		keyValues->window = (keyValues->window != DONTSETLONG) ?
		  keyValues->window : UNSETLONG;
		  
		keyValues->height = (keyValues->height != DONTSETLONG) ?
		  keyValues->height : UNSETLONG;
		keyValues->width = (keyValues->width != DONTSETLONG) ?
		  keyValues->width : UNSETLONG;

		keyValues->justify = (keyValues->justify != DONTSETLONG) ?
		  keyValues->justify : UNSETLONG;
		keyValues->linetype = (keyValues->linetype != DONTSETLONG) ?
		  keyValues->linetype : UNSETLONG;

		keyValues->thickness = (keyValues->thickness != DONTSETREAL) ?
		  keyValues->thickness : UNSETREAL;

		keyValues->file = (keyValues->file != DONTSETPOINTER) ?
		  keyValues->file : UNSETPOINTER;
		keyValues->screendump = (keyValues->screendump != DONTSETPOINTER) ?
		  keyValues->screendump : UNSETPOINTER;

		keyValues->notes = (keyValues->notes != DONTSETSYMH) ?
		  keyValues->notes : UNSETSYMH;
	}

} /*unsetPlotKeys(keyValues)*/

/*
   970105 added support for keywords height and width
   971215 made explicit tests for UNSETLONG
*/
long checkPlotKeys(long toFile, plotKeyValuesPtr keyValues, long * termType)
{
	char          *ignoreFmt =
				      "WARNING: %s:%s on %s() ignored %s";
	char          *errorFmt =
				      "ERROR: %s:%s on %s() illegal %s";
	keywordListPtr keys = PlotKeys;
	char           truth[2][2];
	WHERE("checkPlotKeys");
	
	truth[0][0] = 'F';
	truth[1][0] = 'T';
	truth[0][1] = truth[1][1] = '\0';
	
	if (!keyValues->show)
	{
		char     *withShowF = "with show:F";

		if (!keyValues->keep)
		{
			sprintf(OUTSTR,	errorFmt, KeyName(keys, KKEEP), truth[0],
					FUNCNAME, withShowF);
			goto errorExit;
		}
		if (keyValues->screendump != (char *) 0)
		{
			sprintf(OUTSTR,	errorFmt, KeyName(keys, KSCREENDUMP), "fileName",
					FUNCNAME, withShowF);
			goto errorExit;
		}
	} /*if (keyValues->show == 0)*/

	if (toFile)
	{
		char     *whenWriting = "when writing plot to file";
		
		if (keyValues->window != UNSETLONG && keyValues->window != DONTSETLONG)
		{
			sprintf(OUTSTR, errorFmt, KeyName(keys, KWINDOW), "n",
					FUNCNAME, whenWriting);
			goto errorExit;
		}
		if (keyValues->screendump != UNSETPOINTER)
		{
			sprintf(OUTSTR, errorFmt, KeyName(keys, KSCREENDUMP), "fileName",
					FUNCNAME, whenWriting);
			goto errorExit;
		}
		
		if (!(keyValues->show))
		{
			sprintf(OUTSTR, ignoreFmt, KeyName(keys, KSHOW), truth[0],
					FUNCNAME, whenWriting);
			putErrorOUTSTR();
			keyValues->show = 1;
		}

/* default is PostScript unless ps:F, dumb:T or epsf:T is argument */
#ifdef EPSFPLOTSOK
		if (keyValues->epsf && keyValues->epsf != UNSETCHAR)
		{ /*epsf:T*/
			char    *withEpsf = "with epsf:T";

			if (!keyValues->newFile)
			{
				sprintf(OUTSTR, errorFmt, KeyName(keys, KNEW), truth[0],
						FUNCNAME, withEpsf);
				goto errorExit;
			}			
			if (keyValues->dumb && keyValues->dumb != UNSETCHAR &&
				!DEFAULTDUMBPLOT)
			{
				sprintf(OUTSTR, errorFmt, KeyName(keys, KDUMB), truth[1],
						FUNCNAME, withEpsf);
				goto errorExit;
			}
			if (!keyValues->ps)
			{
				sprintf(OUTSTR, ignoreFmt, KeyName(keys, KPS), truth[0],
						FUNCNAME, withEpsf);
				putErrorOUTSTR();
			}			
			keyValues->ps = keyValues->newFile = 1;
			keyValues->dumb = 0;
			*termType = PREVIEWTERM;
			keyValues->window = PREVIEWWINDOW;
		} /*if (keyValues->epsf && keyValues->epsf != UNSETCHAR)*/
		else
#endif /*EPSFPLOTSOK*/
		if (keyValues->dumb && keyValues->dumb != UNSETCHAR)
		{ /*ps:T*/
			*termType = DUMBTERM;
		}
		else if (keyValues->ps == 0)
		{ /*ps:F*/
			*termType = FILETERM;
#ifndef DUMBFILETERM
			if (!keyValues->newFile)
			{
				sprintf(OUTSTR, errorFmt, KeyName(keys, KNEW), truth[0],
						FUNCNAME, "when writing to file with ps:F");
				goto errorExit;
			}
			keyValues->newFile = 1;	
			keyValues->window = PICTWINDOW;
			keyValues->dumb = 0;
#else /*DUMBFILETERM*/
			keyValues->dumb = 1;
#endif  /*DUMBFILETERM*/
		}
		else
		{ /* either ps:T or no mention of ps */
			*termType = PSTERM;
			keyValues->ps = 1;
		}
#ifdef ALWAYSPROLOG /* always put prolog when ps:T even with new:F */
		PutProlog = (keyValues->ps) ? ((keyValues->epsf > 0) ? -1 : 1) : 0;
		if (PutProlog > 0 && keyValues->newFile <= 0)
		{ /*PutProlog == 2 means either new:F or new not specified*/
			PutProlog = 2;
		}
#else /*ALWAYSPROLOG*/
		/* if compiled, put prolog only for EPSF or with new:T */
		PutProlog = (keyValues->newFile > 0 && keyValues->ps) ? ((keyValues->epsf > 0) ? -1 : 1) : 0;
#endif /*ALWAYSPROLOG*/
	} /*if (toFile)*/
	else
	{
		char    *whenNotWriting = "when not writing plot to file";

		if (keyValues->newFile != UNSETCHAR)
		{
			sprintf(OUTSTR, ignoreFmt, KeyName(keys, KNEW),
					(keyValues->newFile) ? truth[1] : truth[0], FUNCNAME, whenNotWriting);
			putErrorOUTSTR();
		}
		if (keyValues->ps != UNSETCHAR)
		{
			sprintf(OUTSTR, ignoreFmt, KeyName(keys, KPS),
					(keyValues->ps) ? truth[1] : truth[0], FUNCNAME,
					whenNotWriting);
			putErrorOUTSTR();
		}

		if (keyValues->epsf != UNSETCHAR)
		{
			sprintf(OUTSTR, ignoreFmt, KeyName(keys, KEPSF),
					(keyValues->epsf) ? truth[1] : truth[0], FUNCNAME, whenNotWriting);
			putErrorOUTSTR();
		}
#ifdef MACINTOSH
		if (!UseWindows)
		{
			if (keyValues->dumb == 0)
			{
				sprintf(OUTSTR, ignoreFmt, KeyName(keys, KDUMB), truth[0],
						FUNCNAME, "in non-interactive mode");
				putErrorOUTSTR();
			}
			keyValues->dumb = 1;
		}
#endif /*MACINTOSH*/
		keyValues->newFile = 0;
		keyValues->ps = 1;
		keyValues->epsf = 0;
	} /*if (toFile){}else{}*/
	
	keyValues->newFile = (keyValues->newFile == UNSETCHAR) ? 0 : keyValues->newFile;
	keyValues->dumb = (keyValues->dumb == UNSETCHAR) ? DEFAULTDUMBPLOT : keyValues->dumb;
	keyValues->epsf = (keyValues->epsf == UNSETCHAR) ? 0 : keyValues->epsf;
	keyValues->show = (keyValues->show == UNSETCHAR) ? 1 : keyValues->show;
	keyValues->keep = (keyValues->keep == UNSETCHAR) ? 1 : keyValues->keep;
	keyValues->notes = (keyValues->notes == UNSETSYMH) ?
	  (Symbolhandle) 0 : keyValues->notes;

	if (!keyValues->keep && keyValues->notes != (Symbolhandle) 0)
	{
		sprintf(OUTSTR,
				"WARNING: keyword '%s' ignored with %s:F on %s()",
				KeyName(keys, KNOTES), KeyName(keys, KKEEP), FUNCNAME);
		putErrorOUTSTR();
		keyValues->notes = (Symbolhandle) 0;
	}
	
	if (keyValues->dumb)
	{
		char      *warning = "WARNING: %s = %ld < %ld on %s(); ignored";
		
		if (keyValues->height != UNSETLONG && keyValues->height < MINSCREENHEIGHT)
		{
			sprintf(OUTSTR, warning,
					KeyName(keys, KHEIGHT), keyValues->height,
					(long) MINSCREENHEIGHT, FUNCNAME);
			putErrorOUTSTR();
			keyValues->height = UNSETLONG;
		}
		if (keyValues->width != UNSETLONG && keyValues->width < MINSCREENWIDTH)
		{
			sprintf(OUTSTR, warning,
					KeyName(keys, KWIDTH), keyValues->width,
					(long) MINSCREENWIDTH, FUNCNAME);
			putErrorOUTSTR();
			keyValues->width = UNSETLONG;
		}
		*termType = DUMBTERM;
	}
	else if (keyValues->height != UNSETLONG || keyValues->width != UNSETLONG)
	{
		char     *warning = "WARNING: '%s' ignored without '%s:T'";
		
		if (keyValues->height != UNSETLONG)
		{
			sprintf(OUTSTR,warning, KeyName(keys, KHEIGHT),
					KeyName(keys, KDUMB));
			putErrorOUTSTR();
		}
		if (keyValues->width != UNSETLONG)
		{
			sprintf(OUTSTR,warning, KeyName(keys, KWIDTH),
					KeyName(keys, KDUMB));
			putErrorOUTSTR();
		}
		keyValues->height = UNSETLONG;
	}
	
	return (1);
	
  errorExit:
	putErrorOUTSTR();
	return (0);
} /*checkPlotKeys()*/

#ifdef DUMPPLOTKEYS /* for debugging purposes*/
/*
  added 971216 to help in debugging keyword handling
*/
void dumpPlotKeys(char * /*where*/, char * /*msg*/,
				  plotKeyValuesPtr /*keyValues*/);

void dumpPlotKeys(char * where, char *msg, plotKeyValuesPtr keyValues)
{
	sprintf(OUTSTR,
			"%s: plotting keyword values%s",where,(msg) ? msg : NullString);
	putOUTSTR();
	sprintf(OUTSTR, "title[0] = %02x, xlab[0] = %02x, ylab[0] = %02x",
			keyValues->title[0], keyValues->xlab[0], keyValues->ylab[0]);
	putOUTSTR();
	sprintf(OUTSTR, "xmin = %.15g, ymin = %.15g, xmax = %.15g, ymax = %.15g",
			keyValues->xmin, keyValues->ymin,
			keyValues->xmax, keyValues->ymax);
	putOUTSTR();
	sprintf(OUTSTR, "xticklength = %d, yticklength = %d",
			(int) keyValues->xticklength, (int) keyValues->yticklength);
	putOUTSTR();
	sprintf(OUTSTR, "xticks = %08x, yticks = %08x",
			(int) keyValues->xticks, (int) keyValues->yticks);
	putOUTSTR();
	sprintf(OUTSTR, "logx = %d, logy = %d, xaxis = %d, yaxis = %d",
			(int) keyValues->logx, (int) keyValues->logy,
			(int) keyValues->xaxis, (int) keyValues->yaxis);
	putOUTSTR();
	sprintf(OUTSTR, "dumb = %d, keep = %d, show = %d, add = %d",
			(int) keyValues->dumb, (int) keyValues->keep,
			(int) keyValues->show, (int) keyValues->add);
	putOUTSTR();
	sprintf(OUTSTR, "pause = %d, impulses = %d, lines = %d, landscape = %d",
			(int) keyValues->pause, (int) keyValues->impulses,
			(int) keyValues->lines, (int) keyValues->landscape);
	putOUTSTR();
	sprintf(OUTSTR, "ps = %d, epsf = %d, new = %d",
			(int) keyValues->ps, (int) keyValues->epsf, (int) keyValues->newFile);
	putOUTSTR();
	sprintf(OUTSTR, "window = %ld, height = %ld, width = %ld",
			keyValues->window, keyValues->height, keyValues->width);
	putOUTSTR();
	sprintf(OUTSTR, "justify = %ld, linetype = %ld, thickness = %.15g",
			keyValues->justify, keyValues->linetype, keyValues->thickness);
	putOUTSTR();
	sprintf(OUTSTR, "file = \"%s\", screendump = \"%s\"",
			SAFESTRING(keyValues->file), SAFESTRING(keyValues->screendump));
	putOUTSTR();
} /*dumpPlotKeys()*/
#endif /*DUMPPLOTKEYS*/

/*
  Code having to do with saving and restoring PLOTINFO symbols
*/

/*
  The default format that is written by saveGraphHandle() has been
  changed so that all items are named.  It is done in such a way that
  it should not be difficult to add more descriptive stuff to
  be part of structure whole_graph and have it saved and restored, while
  remaining compatible with earlier releases.
*/

enum wholeGraphItemIndices
{
	WGNPLOTS,
	WGNSTRINGS,
	WGXMIN,
	WGXMAX,
	WGYMIN,
	WGYMAX,
	WGLOGX,
	WGLOGY,
	WGCOLORSWG,
	WGXTICKLENGTH,
	WGYTICKLENGTH,
	WGXTICKS,
	WGYTICKS,
	WGXAXIS,
	WGYAXIS,
	WGXLAB,
	WGYLAB,
	WGTITLE,
	WGENDWG
};

/* Note: names of items must not exceed ItemNameLength */
static char  WholeGraphPrefix [] = "WG";

static SaveItemTypes WholeGraphItemTypes[] =
{
	SAVELONGSCALAR /*nplots*/,
	SAVELONGSCALAR /*nstrings*/,
	SAVEREALSCALAR /*xmin*/,
	SAVEREALSCALAR /*xmax*/,
	SAVEREALSCALAR /*ymin*/,
	SAVEREALSCALAR /*ymax*/,
	SAVELONGSCALAR /*logx*/,
	SAVELONGSCALAR /*logy*/,
	SAVELONGSCALAR /*colorswg*/,
	SAVELONGSCALAR /*xticklength*/,
	SAVELONGSCALAR /*yticklength*/,
	SAVEREALVECTOR /*xticks*/,
	SAVEREALVECTOR /*yticks*/,
	SAVELONGSCALAR /*xaxis*/,
	SAVELONGSCALAR /*yaxis*/,
	SAVECHARSCALAR /*xlab*/,
	SAVECHARSCALAR /*ylab*/,
	SAVECHARSCALAR /*title*/
};

#define WGITEMTYPE(I) WholeGraphItemTypes[I]
#define WGITEMNAME(I) (encodeItem(WholeGraphPrefix, I,\
	((I) != EndItems) ? WGITEMTYPE(I) : 0))

enum curvePointItemIndices
{
	CPSTYLE,
	CPPTYPE,
	CPLTYPE,
	CPNP,
	CPENDCP
};

static char         CurvePointPrefix[] = "CP";

static SaveItemTypes CurvePointTypes[] =
{
	SAVELONGSCALAR /*style*/,
	SAVELONGSCALAR /*ptype*/,
	SAVELONGSCALAR /*ltype*/,
	SAVELONGSCALAR /*np*/
};

#define CPITEMTYPE(I) CurvePointTypes[I]
#define CPITEMNAME(I) (encodeItem(CurvePointPrefix, I,\
	((I) != EndItems) ? CPITEMTYPE(I) : 0))

enum plotStringItemIndices
{
	PSSTYLE,
	PSXPS,
	PSYPS,
	PSSTRINGPS,
	PSENDPS
};

static char  CoordinatePrefix [] = "CO";

static SaveItemTypes CoordinateTypes[] =
{
	SAVELONGVECTOR /*style*/,
	SAVEREALVECTOR /*xy*/,
	SAVECHARVECTOR /*strings*/
};

#define COITEMTYPE(I) CoordinateTypes[I]
#define COITEMNAME(I) (encodeItem(CoordinatePrefix, I,\
	((I) != EndItems) ? COITEMTYPE(I) : 0))

static char  PlotStringPrefix [] = "PS";

static SaveItemTypes PlotStringTypes[] =
{
	SAVELONGSCALAR /*psstyle*/,
	SAVEREALSCALAR /*xps*/,
	SAVEREALSCALAR /*yps*/,
	SAVECHARSCALAR /*stringps*/
};


#define PSITEMTYPE(I) PlotStringTypes[I]
#define PSITEMNAME(I) (encodeItem(PlotStringPrefix, I,\
	((I) != EndItems) ? PSITEMTYPE(I) : 0))

enum coordinateItemIndices
{
	COSTYLE,
	COXY,
	COSTRING,
	COENDCO
};

/*
   Functions used by save() to write GRAPH objects
   saveGraphHandle(whole_graph **)
    Saves information applying to entire graph (nplots, nstrings,
    xmin,xmax,ymin,ymax, logx, logy, xaxis, yaxis, xlab, ylab, title)

   saveCurveHandle(curve_points **)
    Saves information applying to single plot component of whole_graph
    (plot_style (line, point, char, etc), point_type (triangle, box, etc),
	p_count (# of points in component))

  savePoints(curve_points **)
    saves all the coordinate structures (undefined, x, y, string) making
    up a component.  For binary save this is saved as a single binary
    block.
  saveStringsHandle(plot_string **)
    Save information about strings being plotted (justify, x, y, string)
  saveGraph(whole_graph **)
    General dispatcher to the preceding for saving a GRAPH
*/

static long saveGraphHandle(whole_graph **graph)
{
/*
  whole_graph info is always saved in ascii form
*/
	if (Pre407Save)
	{
		/*
		  tick and color information is not saved
	    */
		if (!saveLong(GRAPHNPLOTS(graph), (char *) 0)   ||
			!saveLong(GRAPHNSTRINGS(graph), (char *) 0) ||
			!saveDouble(GRAPHXMIN(graph), (char *) 0)   ||
			!saveDouble(GRAPHXMAX(graph), (char *) 0)   ||
			!saveDouble(GRAPHYMIN(graph), (char *) 0)   ||
			!saveDouble(GRAPHYMAX(graph), (char *) 0)   ||
			!saveLong(GRAPHLOGX(graph), (char *) 0)     ||
			!saveLong(GRAPHLOGY(graph), (char *) 0)     ||
			!saveLong(GRAPHXAXIS(graph), (char *) 0)    ||
			!saveLong(GRAPHYAXIS(graph), (char *) 0)    ||
			!saveString(GRAPHXLAB(graph), (char *) 0)   ||
			!saveString(GRAPHYLAB(graph), (char *) 0)   ||
			!saveString(GRAPHTITLE(graph), (char *) 0)) 
		{
			return(0);
		}
	} /*if (Pre407Save)*/
	else
	{
		long      colors = GRAPHFGCOLOR(graph) + 
		  Byte2*GRAPHBGCOLOR(graph) + Byte3*GRAPHFRAMECOLOR(graph);

		if (!saveLong(GRAPHNPLOTS(graph), WGITEMNAME(WGNPLOTS))   ||
			!saveLong(GRAPHNSTRINGS(graph), WGITEMNAME(WGNSTRINGS)) ||
			!saveDouble(GRAPHXMIN(graph), WGITEMNAME(WGXMIN))   ||
			!saveDouble(GRAPHXMAX(graph), WGITEMNAME(WGXMAX))   ||
			!saveDouble(GRAPHYMIN(graph), WGITEMNAME(WGYMIN))   ||
			!saveDouble(GRAPHYMAX(graph), WGITEMNAME(WGYMAX))   ||
			!saveLong(GRAPHLOGX(graph), WGITEMNAME(WGLOGX))     ||
			!saveLong(GRAPHLOGY(graph), WGITEMNAME(WGLOGY))     ||
			!saveLong(colors, WGITEMNAME(WGCOLORSWG)) ||
			!saveLong(GRAPHXTICKLENGTH(graph), WGITEMNAME(WGXTICKLENGTH)) ||
			!saveLong(GRAPHYTICKLENGTH(graph), WGITEMNAME(WGYTICKLENGTH)) ||
			!saveHandle((char **) GRAPHXTICKS(graph), DOUBLEHANDLE,
						GRAPHNXTICKS(graph), WGITEMNAME(WGXTICKS)) ||
			!saveHandle((char **) GRAPHYTICKS(graph), DOUBLEHANDLE, 
						GRAPHNYTICKS(graph), WGITEMNAME(WGYTICKS)) ||
			!saveLong(GRAPHXAXIS(graph), WGITEMNAME(WGXAXIS))   ||
			!saveLong(GRAPHYAXIS(graph), WGITEMNAME(WGYAXIS))   ||
			!saveString(GRAPHXLAB(graph), WGITEMNAME(WGXLAB))   ||
			!saveString(GRAPHYLAB(graph), WGITEMNAME(WGYLAB))   ||
			!saveString(GRAPHTITLE(graph), WGITEMNAME(WGTITLE)) ||
			!saveName(WGITEMNAME(EndItems)))
		{
			return(0);
		}
	} /*if (Pre407Save)*/
	
	return (1);
} /*saveGraphHandle()*/

/*
   Every component is saved using the same utilities as in save().
   The format is arcane but is understood by restore.
   In binary mode, the entire GRAPHPOINTS array of
   coordinate structures is written as a single binary block.
*/
static long saveCurveHandle(curve_points **curve)
{
	long      plotStyle = PLOTSTYLE(curve);
	long      pointType = POINTTYPE(curve);
	long      lineType = LINETYPE(curve);
	long      pCount = PCOUNT(curve);

	if (Pre407Save)
	{
		if (ASCII < SAVEASCII4 && ASCII > SAVEBINARY4)
		{
			if (plotStyle == POINTS)
			{
				plotStyle = POINTS_V31;
			}
			else if (plotStyle == CHARS)
			{
				plotStyle = CHARS_V31;
			}
			else if (plotStyle == LINES)
			{
				plotStyle = LINES_V31;
			}
			else if ((plotStyle & IMPULSES) != 0)
			{
				plotStyle = IMPULSES_V31;
			}
			else if (plotStyle == BOTHLANDP)
			{
				plotStyle = BOTHLANDP_V31;
			}
			else if (plotStyle == BOTHLANDC)
			{
				plotStyle = BOTHLANDC_V31;
			}
			else
			{
				plotStyle = POINTS_V31;
			}
			lineType %= MAXLINETYPE;
		} /*if (ASCII < SAVEASCII4 && ASCII > SAVEBINARY4)*/

		if (!saveLong(plotStyle, (char *) 0) ||
			!saveLong(pointType, (char *) 0) ||
			!saveLong(lineType, (char *) 0)  ||
			!saveLong(pCount, (char *) 0))
		{
			return(0);
		}
	} /*if (Pre407Save)*/
	else
	{
		if (!saveLong(plotStyle, CPITEMNAME(CPSTYLE))   ||
			!saveLong(pointType, CPITEMNAME(CPPTYPE)) ||
			!saveLong(lineType, CPITEMNAME(CPLTYPE)) ||
			!saveLong(pCount, CPITEMNAME(CPNP)) ||
			!saveName(CPITEMNAME(EndItems)))
		{
			return(0);
		}
		
	} /*if (Pre407Save){}else{}*/
	
	return (1);
} /*saveCurveHandle()*/


static long savePoints(curve_points **curve)
{
	long             npoints = PCOUNT(curve), i, styles;

	if (Pre407Save)
	{
		if (Binary)
		{ /* binary format */
			char     **graphpoints = (char **) GRAPHPOINTS(curve);
			long       length = myhandlelength(graphpoints);
		
			if (length == CORRUPTEDHANDLE ||
				!saveRawHandle(graphpoints, length))
			{
				return (0);
			}
		} /*if (Binary)*/
		else
		{
			for (i = 0;i < npoints;i++)
			{
				styles = POINTUNDEF(curve, i);

				if (!saveLong(styles, (char *) 0))
				{
					return(0);
				}
				if (!saveDouble(XCOORD(curve,i), (char *) 0))
				{
					return(0);
				}
				if (!saveDouble(YCOORD(curve,i), (char *) 0))
				{
					return(0);
				}

				if (!saveString(POINTSTRING(curve,i), (char *) 0))
				{
					return (0);
				}
			} /*for (i = 0;i < npoints;i++)*/
		} /*if (Binary){}else{}*/
	} /*if (Pre407Save)*/
	else
	{
		/*
		  Save the styles (defined, point color, line color) of every point
		*/
		if (!saveNamedVector(COITEMNAME(COSTYLE), SAVELONGVECTOR, npoints,
							 (long *) 0, (double *) 0, (char *) 0))
		{
			return (0);
		}
		for (i = 0; i < npoints; i++)
		{
			styles = POINTUNDEF(curve, i) + Byte2*POINTPCOLOR(curve, i) +
			  Byte3*POINTLCOLOR(curve, i);

			if (!saveLong(styles, (char *) 0))
			{
				return(0);
			}
		} /*for (i = 0; i < npoints; i++)*/

		/*
		  Save the x and y coordinates in order x[0], y[0], x[1], y[1], ...
		*/
		if (!saveNamedVector(COITEMNAME(COXY), SAVEREALVECTOR, 2*npoints,
							 (long *) 0, (double *) 0, (char *) 0))
		{
			return (0);
		}

		for (i = 0; i < npoints; i++)
		{
			if (!saveDouble(XCOORD(curve,i), (char *) 0))
			{
				return(0);
			}
			if (!saveDouble(YCOORD(curve,i), (char *) 0))
			{
				return(0);
			}
		} /*for (i = 0; i < npoints; i++)*/

		/*
		  If there are any, save the strings to be plotted at each point
		*/
		if (PLOTSTYLE(curve) & CHARS)
		{
			if (!saveNamedVector(COITEMNAME(COSTRING), SAVECHARVECTOR, npoints,
								 (long *) 0, (double *) 0, (char *) 0))
			{
				return (0);
			}
			for (i = 0; i < npoints; i++)
			{
				if (!saveString(POINTSTRING(curve,i), (char *) 0))
				{
					return(0);
				}
			}
		} /*if (PLOTSTYLE(curve) & CHARS)*/
		
		/* Save end marker for coordinate */
		if (!saveName(COITEMNAME(EndItems)))
		{
			return (0);
		}
	} /*if (Pre407Save){}else{}*/
	
	return (1);
} /*savePoints()*/

static long saveStringsHandle(plot_string **string)
{
	long       styles = (long) JUSTIFYSTR(string);
	double     x = STRINGX(string), y = STRINGY(string);
	char      *thisString = *PLOTSTRING(string);
	
	if (Pre407Save)
	{
		if (!saveLong(styles, (char *) 0) ||
			!saveDouble(x, (char *) 0)  || !saveDouble(y, (char *) 0)  ||
			!saveString(thisString, (char *) 0))
		{
			return(0);
		}
	} /*if (Pre407Save)*/
	else
	{
		styles += Byte2*STRINGCOLOR(string);
		styles += Byte3*STRINGSTYLE(string);

		if (!saveLong(styles, PSITEMNAME(PSSTYLE)) ||
			!saveDouble(x, PSITEMNAME(PSXPS))  ||
			!saveDouble(y, PSITEMNAME(PSYPS))  ||
			!saveString(thisString, PSITEMNAME(PSSTRINGPS)) ||
			!saveName(PSITEMNAME(EndItems)))
		{
			return (0);
		}
	} /*if (Pre407Save){}else{}*/
	return (1);
} /*saveStringsHandle()*/

/* 
   Save to KEEPFILE using global flag ASCII

   980715 changed calling sequence;  saveGraph() now does all the
          saving, not just the contents
*/
int saveGraph(Symbolhandle symh, char * name)
{
	whole_graph  **graph = GRAPH(symh);
	long           i, nplots = GRAPHNPLOTS(graph);
	long           nstrings = GRAPHNSTRINGS(graph);
	curve_points **next_plot;
	curve_points **this_plot;
	plot_string  **next_string;
	plot_string  **this_string;

	if (!saveSymbolInfo(symh, name, PLOTINFO))
	{
		return (0);
	}
	
	if (!saveGraphHandle(graph))
	{
		return (0);
	}
	next_plot = GRAPHFIRST_PLOT(graph);
	for (i=0;i<nplots;i++)
	{
		this_plot = next_plot;
		next_plot = NEXTGRAPH(this_plot);

		if (!saveCurveHandle(this_plot) || !savePoints(this_plot))
		{
			return (0);
		}
	} /*for (i=0;i<nplots;i++)*/
	next_string = GRAPHFIRST_STRING(graph);
	for (i=0;i<nstrings;i++)
	{
		this_string = next_string;
		next_string = NEXTSTRING(this_string);
		if (!saveStringsHandle(this_string))
		{
			return (0);
		}
	} /*for (i=0;i<nstrings;i++)*/

	return (1);
} /*saveGraph()*/

/*
   Functions used by restore() to restore GRAPH symbols

   int restoreGraphHandle(whole_graph **)
	Restores information applying to the entire GRAPH symbol (nplots, nstrings,
	 xmin,xmax,ymin,ymax, logx, logy, xaxis, yaxis, xlab, ylab, title)
   int restoreCurvehandle(curve_points **)
     Restores information applying to a single component (set of coordinate
     structures) of a GRAPH symbol (plot_style, point_type, line_type,
     p_count).
   int restorePoints(curve_points **)
     Restores all the coordinate structures making up a component of a GRAPH
     symbol (each consists of undefine, x, y, string).
   int restoreStringsHandle(plot_string **)
     Restores one string plotted at a particular position (justify, x, y,
     string)
   int restoreGraph(symh)
     Dispatcher and constructor for restoring a whole_graph structure.
*/

static long restoreGraphHandle(whole_graph ** graph)
{
	long           logx, logy, xaxis, yaxis, colors;
	WHERE("restoreGraphHandle");

	GRAPHFIRST_PLOT(graph) = (curve_points **) 0;
	GRAPHXTICKS(graph) = GRAPHYTICKS(graph) = (double **) 0;
	GRAPHNXTICKS(graph) = GRAPHNYTICKS(graph) = 1;
	GRAPHXTICKLENGTH(graph) = GRAPHYTICKLENGTH(graph) = TICKUNIT;

	if (Pre407Save)
	{
		/* Note: This does not restore tick and color info */
		if (!restoreLong(&GRAPHNPLOTS(graph))   ||
			!restoreLong(&GRAPHNSTRINGS(graph)) ||
			!restoreDouble(&GRAPHXMIN(graph))   ||
			!restoreDouble(&GRAPHXMAX(graph))   ||
			!restoreDouble(&GRAPHYMIN(graph))   ||
			!restoreDouble(&GRAPHYMAX(graph))   ||
			!restoreLong(&logx)     ||
			!restoreLong(&logy)     ||
			!restoreLong(&xaxis)    ||
			!restoreLong(&yaxis)    ||
			!restoreString(GRAPHXLAB(graph))    ||
			!restoreString(GRAPHYLAB(graph))    ||
			!restoreString(GRAPHTITLE(graph)))
		{
			return (0);
		}
		/* Not yet (960618) restoring tick information*/
		colors = DEFAULTFGCOLOR + Byte2*DEFAULTBGCOLOR +
		  Byte3*DEFAULTFRAMECOLOR;
	} /*if (Pre407Save)*/
	else
	{
		double        itemDvalue;
		long          itemLvalue, itemLength;
		int           itemNumber, itemType;
		char          itemCvalue[MAXCHARITEMSIZE + 1];

		/* Look for whole_graph items until itemNumber == EndItems */
		while (1)
		{
			if (!getNextRestoreItem(WholeGraphPrefix, &itemNumber, &itemType,
									&itemLength, &itemDvalue, &itemLvalue,
									itemCvalue))
			{
				return (0);
			}

			if (itemNumber == EndItems)
			{
				break;
			}
			
			if (itemNumber < WGENDWG)
			{ /* known item */
				long        i;
				double    **ticks;
				
				if (itemType != WGITEMTYPE(itemNumber))
				{
					return (0);
				}
				switch (itemNumber)
				{
				  case WGNPLOTS:
					GRAPHNPLOTS(graph) = itemLvalue;
					break;
				  case WGNSTRINGS:
					GRAPHNSTRINGS(graph) = itemLvalue;
					break;
				  case WGXMIN:
					GRAPHXMIN(graph) = itemDvalue;
					break;
				  case WGXMAX:
					GRAPHXMAX(graph) = itemDvalue;
					break;
				  case WGYMIN:
					GRAPHYMIN(graph) = itemDvalue;
					break;
				  case WGYMAX:
					GRAPHYMAX(graph) = itemDvalue;
					break;
				  case WGLOGX:
					logx = itemLvalue;
					break;
				  case WGLOGY:
					logy = itemLvalue;
					break;
				  case WGCOLORSWG:
					colors = itemLvalue;
					break;
				  case WGXTICKLENGTH:
					GRAPHXTICKLENGTH(graph) = itemLvalue;
					break;
				  case WGYTICKLENGTH:
					GRAPHYTICKLENGTH(graph) = itemLvalue;
					break;
				  case WGXTICKS:
				  case WGYTICKS:
					TMPHANDLE = mygethandle(itemLength * sizeof(double));
					
					if (TMPHANDLE == (char **) 0)
					{
						return (0);
					}
					ticks = (double **) TMPHANDLE;
					if (itemNumber == WGXTICKS)
					{
						GRAPHNXTICKS(graph) = itemLength;
						GRAPHXTICKS(graph) = ticks;
					}
					else
					{
						GRAPHNYTICKS(graph) = itemLength;
						GRAPHYTICKS(graph) = ticks;
					}
					
					for (i = 0; i < itemLength; i++)
					{
						if (!restoreDouble(&itemDvalue))
						{
							return (0);
						}
						(*ticks)[i] = itemDvalue;
					}
					break;
				  case WGXAXIS:
					xaxis = itemLvalue;
					break;
				  case WGYAXIS:
					yaxis = itemLvalue;
					break;
				  case WGXLAB:
					if (strlen(itemCvalue) > XLABELSIZE + 1)
					{
						return (0);
					}
					strcpy(GRAPHXLAB(graph), itemCvalue);
					break;
				  case WGYLAB:
					if (strlen(itemCvalue) > YLABELSIZE + 1)
					{
						return (0);
					}
					strcpy(GRAPHYLAB(graph), itemCvalue);
					break;
				  case WGTITLE:
					if (strlen(itemCvalue) > TITLESIZE + 1)
					{
						return (0);
					}
					strcpy(GRAPHTITLE(graph), itemCvalue);
					break;
				}
			} /*if (itemNumber < WGENDWG)*/
			else if (itemType & SAVEVECTOR)
			{ /*unrecognized vector type ; skip value*/
				if (!skipRestoreVector(itemType, itemLength))
				{
					return (0);
				}
			} /*else if (itemType & SAVEVECTOR)*/
		} /*while (1)*/
	} /*if (Pre407Save){}else{}*/
	
	GRAPHLOGX(graph) = logx;
	GRAPHLOGY(graph) = logy;
	GRAPHXAXIS(graph) = xaxis;
	GRAPHYAXIS(graph) = yaxis;
	GRAPHFGCOLOR(graph) = colors % ByteSize;
	colors /= ByteSize;
	GRAPHBGCOLOR(graph) = colors % ByteSize;
	colors /= ByteSize;
	GRAPHFRAMECOLOR(graph) = colors % ByteSize;

	GRAPHXLAB(graph)[XLABELSIZE+1] = '\0';
	GRAPHYLAB(graph)[YLABELSIZE+1] = '\0';
	GRAPHTITLE(graph)[TITLESIZE+1] = '\0';

	return (1);
} /*restoreGraphHandle()*/

static long restoreCurveHandle(curve_points ** curve)
{
	long             style;

	if (Pre407Save)
	{
		if (!restoreLong(&style) ||
			!restoreLong(&POINTTYPE(curve)) ||
			!restoreLong(&LINETYPE(curve))  ||
			!restoreLong(&PCOUNT(curve)))
		{
			return(0);
		}
	} /*if (Pre407Save)*/
	else
	{
		double        itemDvalue;
		long          itemLvalue, itemLength;
		int           itemNumber, itemType;
		char          itemCvalue[MAXCHARITEMSIZE + 1];

		/* Look for curve_points items until itemNumber == EndItems */
		while (1)
		{
			if (!getNextRestoreItem(CurvePointPrefix, &itemNumber, &itemType,
									&itemLength, &itemDvalue, &itemLvalue,
									itemCvalue))
			{
				return (0);
			}

			if (itemNumber == EndItems)
			{
				break;
			}
			
			if (itemNumber < CPENDCP)
			{ /* known item */
				if (itemType != CPITEMTYPE(itemNumber))
				{
					return (0);
				}
				switch (itemNumber)
				{
				  case CPSTYLE:
					style = itemLvalue;
					break;
				  case CPPTYPE:
					POINTTYPE(curve) = itemLvalue;
					break;
				  case CPLTYPE:
					LINETYPE(curve) = itemLvalue;
					break;
				  case CPNP:
					PCOUNT(curve) = itemLvalue;
					break;
				}
			} /*if (itemNumber < CPENDCP)*/
			else if (itemType & SAVEVECTOR)
			{ /*unrecognized vector type ; skip value*/
				if (!skipRestoreVector(itemType, itemLength))
				{
					return (0);
				}
			} /*if (itemNumber < CPENDCP){}else{}*/
		} /*while (1)*/
	} /*if (Pre407Save){}else{}*/

	if (ASCII < SAVEASCII4 && ASCII > SAVEBINARY4)
	{
		if (style == LINES_V31)
		{
			style = LINES;
		}
		else if (style == IMPULSES_V31)
		{
			style = IMPULSES;
		}
		else if (style == CHARS_V31)
		{
			style = CHARS;
		}
		else if (style == BOTHLANDP_V31)
		{
			style = BOTHLANDP;
		}
		else if (style == BOTHLANDC_V31)
		{
			style = BOTHLANDC;
		}
		else
		{ /* case of doubt */
			style = POINTS;
		}
	} /*if (ASCII < SAVEASCII4 && ASCII > SAVEBINARY4)*/

	PLOTSTYLE(curve) = style;

	return (1);
} /*restoreCurveHandle()*/

static long restorePoints(curve_points ** curve)
{
	long             npoints = PCOUNT(curve), i;
	int              foundCostrings = 0;
	WHERE("restorePoints");
	
	if (Pre407Save)
	{
		if (Binary)
		{
			long             n;
			int              c;
			char           **hand;

			n = npoints * sizeof(coordinate);
			hand = (char **) GRAPHPOINTS(curve);
			for (i = 0;i < n;i++)
			{
				if ((c = fgetc(RESTOREFILE)) == EOF)
				{
					return (0);
				}
				(*hand)[i] = (char) c;
			} /*for (i = 0;i < n;i++)*/
		} /*if (Binary)*/
		else
		{ /* ASCII */
			for (i = 0;i < npoints;i++)
			{
				double          x, y;
				long            styles;
				unsigned char   undefined, pointcolor, linecolor;
				char     string[100];

				if (!restoreLong(&styles) ||
					!restoreDouble(&x) || !restoreDouble(&y) ||
					!restoreString(string) || strlen(string) > PLOTSTRINGSIZE)
				{
					return (0);
				}

				undefined = styles % ByteSize;
				styles /= ByteSize;
				pointcolor = styles % ByteSize;
				styles /= ByteSize;
				linecolor = styles % ByteSize;

				if (undefined)
				{
					undef_point(curve, i);
				}
				else
				{
					set_point(curve, i, x, y, string, pointcolor, linecolor);
				}
			} /*for (i = 0;i < npoints;i++)*/
		} /*if (Binary){...}else{...}*/
	} /*if (Pre407Save)*/
	else
	{
		double        itemDvalue;
		long          itemLvalue, itemLength;
		int           itemNumber, itemType;
		char          itemCvalue[MAXCHARITEMSIZE + 1];
		
		/* Look for coordinate items until itemNumber == EndItems */
		while (1)
		{
			if (!getNextRestoreItem(CoordinatePrefix, &itemNumber, &itemType,
									&itemLength, &itemDvalue, &itemLvalue,
									itemCvalue))
			{
				return (0);
			}

			if (itemNumber == EndItems)
			{
				break;
			}
			
			if (itemNumber < COENDCO)
			{ /* known item */
				long        i;
				
				if (itemType != COITEMTYPE(itemNumber))
				{
					return (0);
				}
				if (itemNumber == COXY)
				{
					itemLength /= 2;
				}
				if (itemLength != npoints)
				{
					return (0);
				}
				
				for (i = 0; i < itemLength; i++)
				{					
					double          x, y;
					long            styles;

					switch (itemNumber)
					{
					  case COSTYLE:
						if (!restoreLong(&styles))
						{
							return (0);
						}
						POINTUNDEF(curve, i) = styles % ByteSize;
						styles /= ByteSize;
						POINTPCOLOR(curve, i) = styles % ByteSize;
						styles /= ByteSize;
						POINTLCOLOR(curve, i) = styles % ByteSize;
						break;

					  case COXY:
						if (!restoreDouble(&x) || !restoreDouble(&y))
						{
							return (0);
						}
						if (POINTUNDEF(curve, i))
						{
							XCOORD(curve, i) = YCOORD(curve, i) = 0.0;
						}
						else
						{
							XCOORD(curve, i) = x;
							YCOORD(curve, i) =y;
						}
						break;
						
					  case COSTRING:
						foundCostrings = 1;
						
						if (!restoreString(itemCvalue) ||
							strlen(itemCvalue) > PLOTSTRINGSIZE)
						{
							return (0);
						}
						if (POINTUNDEF(curve, i))
						{
							itemCvalue[0] = '\0';
						}
						strcpy(POINTSTRING(curve, i), itemCvalue);
						break;
					} /*switch (itemNumber)*/
				} /*for (i = 0; i < itemLength; i++)*/
			} /*if (itemNumber < CPENDCP)*/
			else if (itemType & SAVEVECTOR)
			{ /*unrecognized vector type ; skip value*/
				if (!skipRestoreVector(itemType, itemLength))
				{
					return (0);
				}
			} /*if (itemNumber < CPENDCP){}else{}*/
		} /*while (1)*/
		
		if (!foundCostrings)
		{
			if (PLOTSTYLE(curve) & CHARS)
			{
				return (0);
			}
			for (i = 0; i < npoints; i++)
			{
				POINTSTRING(curve,i)[0] = '\0';
			}
		} /*if (!foundCostrings)*/
	} /*if (Pre407Save){}else{}*/
	return (1);
} /*restorePoints()*/

static long restoreStringsHandle(plot_string ** string)
{
	long             length;
	long             styles;
	double           x, y;
	char             thisString[MAXCHARITEMSIZE+1];
	WHERE("restoreStringsHandle");

	if (Pre407Save)
	{
		if (!restoreLong(&styles) ||
			!restoreDouble(&x)  ||
			!restoreDouble(&y)  ||
			!restoreLong(&length) || length > MAXCHARITEMSIZE ||
			!restoreNString(length, thisString))
		{
			return(0);
		}
	} /*if (Pre407Save)*/
	else
	{
		double        itemDvalue;
		long          itemLvalue, itemLength;
		int           itemNumber, itemType;
		char          itemCvalue[MAXCHARITEMSIZE + 1];

		/* Look for curve_points items until itemNumber == EndItems */
		while (1)
		{
			if (!getNextRestoreItem(PlotStringPrefix, &itemNumber, &itemType,
									&itemLength, &itemDvalue, &itemLvalue,
									itemCvalue))
			{
				return (0);
			}

			if (itemNumber == EndItems)
			{
				break;
			}
			
			if (itemNumber < PSENDPS)
			{ /* known item */
				if (itemType != PSITEMTYPE(itemNumber))
				{
					return (0);
				}
				switch (itemNumber)
				{
				  case PSSTYLE:
					styles = itemLvalue;
					break;
					
				  case PSXPS:
					x = itemDvalue;
					break;
					
				  case PSYPS:
					y = itemDvalue;
					break;
						
				  case PSSTRINGPS:
					length = strlen(itemCvalue);
					strcpy(thisString, itemCvalue);
					break;
				} /*switch (itemNumber)*/
			} /*if (itemNumber < PSENDPS)*/
			else if (itemType & SAVEVECTOR)
			{ /*unrecognized vector type ; skip value*/
				if (!skipRestoreVector(itemType, itemLength))
				{
					return (0);
				}
			} /*if (itemNumber < PSENDPS){}else{}*/
		} /*while (1)*/
	} /*if (Pre407Save){}else{}*/
	
	JUSTIFYSTR(string) = styles % ByteSize;
	if (Pre407Save)
	{
		STRINGCOLOR(string) = DEFAULTSTRINGCOLOR;
		STRINGSTYLE(string) = PLAIN;
	}
	else
	{
		styles /= ByteSize;
		STRINGCOLOR(string) = styles % ByteSize;
		styles /= ByteSize;
		STRINGSTYLE(string) = styles % ByteSize;
	}
	
	STRINGX(string) = x;
	STRINGY(string) = y;

	PLOTSTRING(string) = TMPHANDLE = mygethandle(length+1);

	if (TMPHANDLE == 0)
	{
		return (0);
	}
	strcpy(*TMPHANDLE, thisString);

	return (1);
} /*restoreStringsHandle()*/

/* restore from RESTOREFILE using global flag ASCII */
int restoreGraph(Symbolhandle symhgraph)
{
	whole_graph      **graph = (whole_graph **) 0;
	curve_points     **last_plot = (curve_points **) 0;
	curve_points     **this_plot = (curve_points **) 0;
	plot_string      **last_string = (plot_string **) 0;
	plot_string      **this_string = (plot_string **) 0;
	long               i, nplots, npoints, nstrings;
	WHERE("restoreGraph");

	graph = (whole_graph **) mygethandle(sizeof(whole_graph));
	if (graph == (whole_graph **) 0)
	{
		goto errorExit;
	}
	GRAPHFIRST_PLOT(graph) = (curve_points **) 0;
	GRAPHNPLOTS(graph) = 0;
	GRAPHFIRST_STRING(graph) = (plot_string **) 0;
	GRAPHNSTRINGS(graph) = 0;
	setGRAPH(symhgraph,graph);

	if (!restoreGraphHandle(graph))
	{
		GRAPHNPLOTS(graph) = GRAPHNSTRINGS(graph) = 0;
		goto errorExit;
	}
	
	nplots = GRAPHNPLOTS(graph);
	nstrings = GRAPHNSTRINGS(graph);
	GRAPHNSTRINGS(graph) = GRAPHNPLOTS(graph) = 0;

	for (i = 0;i < nplots;i++)
	{
		if ((TMPHANDLE = mygethandle(sizeof(curve_points))) ==
			(char **) 0)
		{
			goto errorExit;
		}
 		this_plot = (curve_points **) TMPHANDLE;
		NEXTGRAPH(this_plot) = (curve_points **) 0;
		GRAPHPOINTS(this_plot) = (coordinate **) 0;
		GRAPHNPLOTS(graph)++;
		if (i == 0)
		{
			GRAPHFIRST_PLOT(graph) = this_plot;
		}
		else
		{
			NEXTGRAPH(last_plot) = this_plot;
		}
		if (!restoreCurveHandle(this_plot))
		{
			goto errorExit;
		}
		npoints = PCOUNT(this_plot);

		TMPHANDLE = mygethandle(npoints * sizeof(coordinate));
		GRAPHPOINTS(this_plot) = (coordinate **) TMPHANDLE;
		if (TMPHANDLE == (char **) 0)
		{
			goto errorExit;
		}
		if (!restorePoints(this_plot))
		{
			goto errorExit;
		}

		last_plot = this_plot;
	} /*for (i = 0;i < nplots;i++)*/

	for (i = 0;i < nstrings;i++)
	{
		if ((TMPHANDLE = mygethandle(sizeof(plot_string))) == (char **) 0)
		{
			goto errorExit;
		}
		this_string = (plot_string **) TMPHANDLE;
		NEXTSTRING(this_string) = (plot_string **) 0;
		PLOTSTRING(this_string) = (char **) 0;
		GRAPHNSTRINGS(graph)++;
		if (i == 0)
		{
			GRAPHFIRST_STRING(graph) = this_string;
		}
		else
		{
			NEXTSTRING(last_string) = this_string;
		}

		if (!restoreStringsHandle(this_string))
		{
			goto errorExit;
		}
		last_string = this_string;
	} /*for (i = 0;i < nstrings;i++)*/

	return (1);

  errorExit:
	return (0);
} /*restoreGraph()*/

/*
  End of code having to do with saving and restoring PLOTINFO symbols
*/

/*
   Function to open file for plotting information (PostScript, PICT,
   Tektronix codes, dumb plot, etc.)
   971104 added check for value of myhandlelength()
*/
FILE *openPlotFile(char * fileName, long newFile)
{
	FILE      *plotfile = (FILE *) 0;
	long       length = strlen(fileName) + 1;
	long       handleLength = myhandlelength(PLOTFILENAME);
	char      *mode;

	if (handleLength != CORRUPTEDHANDLE)
	{
		if (handleLength != length)
		{
			mydisphandle(PLOTFILENAME);
			PLOTFILENAME = mygethandle(length);
		}
		if (PLOTFILENAME != (char **) 0)
		{
			strcpy(*PLOTFILENAME, fileName);
			mode = (newFile) ? TEXTWRITEMODE : TEXTAPPENDMODE;
			if ((plotfile = fmyopen(fileName,mode)) == (FILE *) 0)
			{
				sprintf(OUTSTR,"ERROR: %s cannot open file %s",FUNCNAME,fileName);
				putErrorOUTSTR();
			}
		}
	}
	
	return (plotfile);
} /*openPlotFile()*/

/*
   Function to return the total number of bytes used in a curve_points
   structure
*/

static long sizeofC_P(curve_points ** points)
{
	long          size = 0;
	
	
	if (points != (curve_points **) 0)
	{
		size = sizeof(curve_points) + PCOUNT(points) * sizeof(coordinate);
	}
	return (size);
} /*sizeofC_P()*/

/*
   Function to return the total number of bytes used in a plot_string
   structure
*/

static long sizeofP_S(plot_string ** strings)
{
	long          size = 0;
	
	if (strings != (plot_string **) 0)
	{
		size = myhandlelength(PLOTSTRING(strings));
		
		if (size != CORRUPTEDHANDLE)
		{
			size = sizeof(plot_string) + ((size > 0) ? size : 0);
		}
	} /*if (strings != (plot_string **) 0)*/
	return (size);
} /*sizeofP_S()*/

/*
   Function to return the total number of bytes used in a whole_graph
   structure
*/

long sizeofPlot(whole_graph ** graph)
{
	long           size = 0, length;
	curve_points **this_plot;
	plot_string  **this_string;

	if (graph != (whole_graph **) 0)
	{
		size = sizeof(whole_graph);
		for (this_plot = GRAPHFIRST_PLOT(graph);
			 this_plot != (curve_points **) 0;
			 this_plot = NEXTGRAPH(this_plot))
		{
			size += sizeofC_P(this_plot);
		}
		for (this_string = GRAPHFIRST_STRING(graph);
			 this_string != (plot_string **) 0;
			 this_string = NEXTSTRING(this_string))
		{
			length = sizeofP_S(this_string);
			
			if (length == CORRUPTEDHANDLE)
			{
				size = CORRUPTEDHANDLE;
				break;
			}
			size += sizeofP_S(this_string);
		}
	} /*if (graph != (whole_graph **) 0)*/
	return (size);
} /*sizeofPlot()*/

#ifdef MACINTOSH
int        screenDump(char *fname)
{
	doSaveGraph(ThisWindow, saveitas);
	return (SCREENDUMPOK);
}
#endif /*MACINTOSH*/

#ifdef DJGPP
/* Function to writeout screen in PCX format */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef GRX20
#include <grx.h>
#include <mousex.h>
#else /*GRX20*/
#include <grx20.h>
#endif /*GRX20*/

#include <io.h>

typedef unsigned char uchar;
typedef unsigned short ushort;

typedef struct
{
	uchar           maker;
	uchar           version;
	uchar           code;
	uchar           bpp;    /* bits per pixel */
	ushort          x1, y1, x2, y2; /* image position */
	ushort          hres, vres;     /* image size */
	struct
	{
		uchar           R, G, B;
	}               cmap[16];       /* palette */
	uchar           vmode;  /* video mode to display it */
	uchar           nplanes;/* number of planes */
	ushort          bpl;    /* bytes per scan line */
	char            filler[128 - 68];
} pcxhdr;

#if     defined(__GNUC__) && defined(__MSDOS__)
#define MAX_COLORS     32768    /* maximum color graphics mode supported */
#else
#define MAX_COLORS     256      /* maximum color graphics mode supported */
#endif
#define MAX_OUTCOLORS  16       /* max number of simultaneous colors */
#define MAX_WIDTH      1280     /* max horizontal image size */

#ifdef GRX20
#define MouseCursorIsDisplayed GrMouseCursorIsDisplayed
#define MouseEraseCursor       GrMouseEraseCursor
#define MouseDisplayCursor     GrMouseDisplayCursor
#endif /*GRX20*/

static int      DumpCount = 0;

int        screenDump(char *fname)
{
	GrContext       save, xfer;
	pcxhdr          hdr;
	uchar           rows[4][MAX_WIDTH / 8];
	char            colortable[MAX_COLORS];
	char            pcxfname[100];
	FILE           *pcxfile;
	int             numcolors, block, fast;
	int             ii, xx, yy, wdt, hgt;

	if (GrCurrentMode() < GR_320_200_graphics)
	{
		return (BADSCREENMODE);
	}
	if (GrNumColors() > MAX_COLORS)
	{
		return (BADCOLORS);
	}
	wdt = GrScreenX();
	hgt = GrScreenY();
	if (wdt > MAX_WIDTH)
	{
		return (BADWIDTH);
	}
	if (fname == (char *) 0)
	{
		while (1)
		{
			/* find a new file name */
			sprintf(pcxfname, "dump%04d.pcx", DumpCount);
			if (access(pcxfname, 0) != 0)
			{
				pcxfile = fmyopen(pcxfname, BINARYWRITEMODE);
				if (pcxfile == NULL)
				{
					return (CANTOPEN);
				}
				break;
			}
			if (++DumpCount >= MAXDUMPCOUNT)
			{
				return (TOOMANYDUMPS);
			}
		}
	} /*if (fname == (char *) 0)*/
	else
	{
		strcpy(pcxfname, fname);
		pcxfile = fmyopen(pcxfname, BINARYWRITEMODE);
		if (pcxfile == NULL)
		{
			return (CANTOPEN);
		}
	} /*if (fname == (char *) 0){}else{}*/
	GrSaveContext(&save);
	GrCreateContext(wdt, 1, NULL, &xfer);
	GrSetContext(&xfer);
	memset(&hdr, 0, sizeof(pcxhdr));
	memset(colortable, -1, sizeof(colortable));
	hdr.maker = 10;
	hdr.version = 5;
	hdr.code = 1;
	hdr.x1 = 0;
	hdr.y1 = 0;
	hdr.x2 = wdt - 1;
	hdr.y2 = hgt - 1;
	hdr.hres = wdt;
	hdr.vres = hgt;
	hdr.bpp = 1;
	hdr.bpl = (wdt + 7) / 8;
	hdr.vmode = 0x12;       /* standard 640x480 16 color VGA ! */
	hdr.nplanes = 4;
	numcolors = 0;
	fast = (GrNumColors() == 256) ? 1 : 0;
	block = MouseCursorIsDisplayed();
	if (block)
	{
		MouseEraseCursor();
	}
	fwrite(&hdr, sizeof(pcxhdr), 1, pcxfile);
	for (yy = 0; yy < hgt; yy++)
	{
		uchar           mask = 0x80;
		int             bytepos = 0;
		int             numbits = 0;
		GrBitBlt(&xfer, 0, 0, &save, 0, yy, wdt - 1, yy, GrWRITE);
		memset(rows, 0, sizeof(rows));
		for (xx = 0; xx < wdt; xx++)
		{
			int         color = fast ? xfer.gc_baseaddr[xx] : GrPixel(xx, 0);
			char        outcolor = colortable[color];

			if (outcolor < 0)
			{       /* new color */
				int             r, g, b;
				GrQueryColor(color, &r, &g, &b);
				for (ii = 0; ii < numcolors; ii++)
				{
					if (hdr.cmap[ii].R != r)
					{
						continue;
					}
					if (hdr.cmap[ii].G != g)
					{
						continue;
					}
					if (hdr.cmap[ii].B != b)
					{
						continue;
					}
					outcolor = ii;
					break;
				} /*for (ii = 0; ii < numcolors; ii++)*/
				if (outcolor < 0)
				{
					if (numcolors >= MAX_OUTCOLORS)
					{
						goto error;
					}
					outcolor = ii = numcolors++;
					hdr.cmap[ii].R = r;
					hdr.cmap[ii].G = g;
					hdr.cmap[ii].B = b;
				} /*if (outcolor < 0)*/
				colortable[color] = outcolor;
			} /*if (outcolor < 0)*/
			if (outcolor & 1)
			{
				rows[0][bytepos] |= mask;
			}
			if (outcolor & 2)
			{
				rows[1][bytepos] |= mask;
			}
			if (outcolor & 4)
			{
				rows[2][bytepos] |= mask;
			}
			if (outcolor & 8)
			{
				rows[3][bytepos] |= mask;
			}
			mask >>= 1;
			if (++numbits == 8)
			{
				mask = 0x80;
				numbits = 0;
				bytepos++;
			}
		} /*for (xx = 0; xx < wdt; xx++)*/
		for (ii = 0; ii < 4; ii++)
		{
			uchar          *ptr = rows[ii];
			uchar           pixval = *ptr;
			numbits = 0;
			for (bytepos = hdr.bpl; --bytepos >= 0; ptr++)
			{
				if (pixval != *ptr)
				{
					while (numbits > 0x3f)
					{
						putc((0xc0 | 0x3f), pcxfile);
						putc(pixval, pcxfile);
						numbits -= 0x3f;
					} /*while (numbits > 0x3f)*/
					if (numbits > 0)
					{
						if ((numbits > 1) || (pixval >= 0xc0))
						{
							putc((0xc0 | numbits), pcxfile);
						}
						putc(pixval, pcxfile);
					}
					numbits = 0;
				}
				pixval = *ptr;
				numbits++;
			} /*for (bytepos = hdr.bpl; --bytepos >= 0; ptr++)*/
			while (numbits > 0x3f)
			{
				putc((0xc0 | 0x3f), pcxfile);
				putc(pixval, pcxfile);
				numbits -= 0x3f;
			} /*while (numbits > 0x3f)*/
			if (numbits > 0)
			{
				if ((numbits > 1) || (pixval >= 0xc0))
				{
					putc((0xc0 | numbits), pcxfile);
				}
				putc(pixval, pcxfile);
			} /*if (numbits > 0)*/
		} /*for (ii = 0; ii < 4; ii++)*/
	} /*for (yy = 0; yy < hgt; yy++)*/
	if (ferror(pcxfile))
	{
		goto error;
	}
	if (block)
	{
		MouseDisplayCursor();
	}
	GrSetContext(&save);
	GrDestroyContext(&xfer);
	fseek(pcxfile, 0L, 0);
	fwrite(&hdr, sizeof(pcxhdr), 1, pcxfile);
	fclose(pcxfile);
	return (SCREENDUMPOK);

  error:
	if (block)
	{
		MouseDisplayCursor();
	} /*if (block)*/
	GrSetContext(&save);
	GrDestroyContext(&xfer);
	fclose(pcxfile);
	unlink(pcxfname);
	return (FILEERROR);
} /*pcxdump()*/
#endif /*DJGPP*/

