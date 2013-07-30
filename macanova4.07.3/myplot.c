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


#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment Myplot
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

/*
   960312 Replaced all explicit usages of form (*xxx)->yyy by suitable
   macro YYY(xxx), adding new definitions to plot.h

   960612 Found and removed memory leak on showplot() (an unnecessary
   whole_graph was allocated by not deleted).
   Also changed code so that data being plotted is not duplicated by
   showplot().

   970105 added support for keyword height and width

   970513 allow structure argument to have more than 2 components, with
   all but the first 2 completely ignored.

   970515 moved code out of myplot() into setTicks()

   970520 Reorganized some of myplot(), fixing some minor bugs
          plot(graph,x,y), chplot(graph,x,y,c) and lineplot(graph,x,y)
          imply the use of add:T when graph is a GRAPH variable

   970522 moved code out of buildPlot() into setGraphStrings()

   971203 added check for undefined arguments (missing arguments may be
          OK because of the use of $K in macros)
   971215 changed code to use new structure plotKeyValues in calls to
          getPlotKeys(), etc.
   980618 lineplot() with lines:F is treated as plot()
   980716 Strings to be drawn by addstrings must be no longer than
          MAXCHARITEMSIZE (200) characters
   980724 Implemented notes:charVector
   990211 Replaced putOUTSTR() by putErrorOUTSTR() and substituted
          putOutErrorMsg() for myerrorout().
*/

#include "globals.h" /* brings matProto.h which brings plot.h*/

#ifdef WXWIN
#include "wx/wxIface.h"
#endif /*WXWIN*/

#ifdef MACINTOSH
#include "macIface.h"
#endif /*MACINTOSH*/

#ifdef MPW
#include <Memory.h>
#endif

#ifndef LASTGRAPH
#define LASTGRAPH "LASTPLOT"
#endif /*LASTGRAPH*/

enum plotScratch
{
	GGRAPH = 0,
	GPLOTC = GGRAPH,
	NTRASH
};

#define trash NAMEFORTRASH

char               *LastGraphName = LASTGRAPH; /* external variable */

enum myPlotCodes
{
	NEEDXY      = 0x01,
	PTPLOT      = 0x02,
	CHARPLOT    = 0x04,
	LINEPLOT    = 0x08,
	STRINGPLOT  = 0x10,
	TOFILE      = 0x20,
	SHOW        = 0x40,
	ADDTOPLOT   = 0x80,

	IPLOT       = (PTPLOT    | NEEDXY),
	ICHPLOT     = (CHARPLOT  | NEEDXY),
	ILINEPLOT   = (LINEPLOT  | NEEDXY),
	IADDPOINTS  = (IPLOT     | ADDTOPLOT),
	IADDCHARS   = (ICHPLOT   | ADDTOPLOT),
	IADDLINES   = (ILINEPLOT | ADDTOPLOT),
	IADDSTRINGS = (STRINGPLOT| ADDTOPLOT | NEEDXY),
	ISHOWPLOT   = (SHOW),
	IFPLOT      = (IPLOT     | TOFILE),
	IFCHPLOT    = (ICHPLOT   | TOFILE),
	IFLINEPLOT  = (ILINEPLOT | TOFILE)
} /*enum myPlotCodes*/;

typedef struct opEntry
{
	char *name;
	short iop;
} opEntry;

static opEntry Ops[] =
{ /* name        iop  */
	{"plot",      IPLOT},
	{"chplot",    ICHPLOT},
	{"lineplot",  ILINEPLOT},
	{"addpoints", IADDPOINTS},
	{"addchars",  IADDCHARS},
	{"addlines",  IADDLINES},
	{"showplot",  ISHOWPLOT},
	{"fplot",     IFPLOT},
	{"fchplot",   IFCHPLOT},
	{"flineplot", IFLINEPLOT},
	{"addstrings",IADDSTRINGS},
	{(char *) 0,  0}
} ;

#define generateChars (cmax < 0)

enum fineExtremeCodes
{
	FINDXMIN    = 0x01,
	FINDXMAX    = 0x02,
	FINDYMIN    = 0x04,
	FINDYMAX    = 0x08
};

/*
  970522 setGraphStrings() added with code taken from buildPlot()
         It implements addstrings()
*/
static int setGraphStrings(double **x, double **y, long n,
						   Symbolhandle stringsymbol, whole_graph **graph,
						   long justify)
{
	long             cplace = 0;
	long             i, length;
	long             nstrings;
	double           xi, yi;
	plot_string    **this_string = (plot_string**) 0;
	plot_string    **last_string = (plot_string**) 0;
	WHERE("setGraphStrings");
	
	nstrings = GRAPHNSTRINGS(graph);
	last_string = GRAPHFIRST_STRING(graph);
	for (i = 1;i < nstrings;i++)
	{/* find final struct plot_string */
		last_string = NEXTSTRING(last_string);
	}

	for (i = 0;i < n;i++)
	{
		xi = (*x)[i];
		yi = (*y)[i];
		length = strlen(STRINGPTR(stringsymbol) + cplace);
		if (length > 0 && !isMissing(xi) && !isMissing(yi))
		{
			TMPHANDLE = mygethandle(sizeof(plot_string));
			this_string = (plot_string **) TMPHANDLE;
			if (nstrings == 0)
			{
				GRAPHFIRST_STRING(graph) = this_string;
			}
			else
			{
				NEXTSTRING(last_string) = this_string;
			}
			if (TMPHANDLE == (char **) 0)
			{
				goto errorExit;
			}
			NEXTSTRING(this_string) = (plot_string **) 0;
			PLOTSTRING(this_string) = (char **) 0;

			GRAPHNSTRINGS(graph) = ++nstrings;
			STRINGX(this_string) = xi;
			STRINGY(this_string) = yi;
			last_string = this_string;

			TMPHANDLE = mygethandle(length + 1);
			PLOTSTRING(this_string) = TMPHANDLE;
			if (TMPHANDLE == (char **) 0)
			{
				goto errorExit;
			}
			strcpy(*TMPHANDLE,STRINGPTR(stringsymbol) + cplace);
			JUSTIFYSTR(this_string) = justify;
#if (1) /* 980714 added in anticipation of having color and styles */
			STRINGCOLOR(this_string) = DEFAULTSTRINGCOLOR;
			STRINGSTYLE(this_string) = PLAIN;
#else /*1*/
			STRINGCOLOR(this_string) = 0;
			STRINGSTYLE(this_string) = 0;
#endif /*1*/
		} /*if (length > 0 && !isMissing(xi) && !isMissing(yi))*/
		cplace += length + 1;
	} /*for (i = 0;i < n;i++)*/
	return (1);
	
  errorExit:
	return (0);
} /*setGraphStrings()*/

/*
  970522 added setGraphPoints() with code from buildPlot()
*/

static int setGraphPoints(long op, double **x, double **y, Symbolhandle symhc,
						  long nrows, long ncols, long cRows, long cCols,
						  long nchars, long cmax,
						  whole_graph **graph, long style, long linetype,
						  long add)
{
	double           xi, yi;
	long             nplots;
	long             i, iplot, j, ij;
	long             cplace, cstart;
	curve_points   **this_plot = (curve_points **) 0;
	curve_points   **last_plot = (curve_points **) 0;
	unsigned char    pointColor = DEFAULTPOINTCOLOR;
	unsigned char    lineColor = DEFAULTLINECOLOR;
	char           **plotcH = (char **) 0;
	TRASH(NTRASH, errorExit);

	if (op & CHARPLOT)
	{
		if (generateChars || TYPE(symhc) == REAL)
		{
			long        nch = nDigits(labs(cmax)), cval;
			char       *plotcP;

			if (!getScratch(plotcH,GPLOTC,nchars*(nch+1),char))
			{
				goto errorExit;
			}
			plotcP = *plotcH;
			for (i = 0;i < nchars;i++)
			{
				cval = (generateChars) ?
					(i+1) % 1000 : (long) DATAVALUE(symhc,i);
				sprintf(OUTSTR,"%ld", cval);
				plotcP = copyStrings(OUTSTR, plotcP, 1);
			} /*for (i = 0;i < nchars;i++)*/
			*OUTSTR = '\0';
		}
		else
		{
			plotcH = STRING(symhc);
		}
	} /*if (op & CHARPLOT)*/

	if (add)
	{/* use existing struct whole_graph */
		nplots = GRAPHNPLOTS(graph);
		last_plot = GRAPHFIRST_PLOT(graph);
		for (i = 1;i < nplots;i++)
		{/* find final struct curve_points */
			last_plot = NEXTGRAPH(last_plot);
		} /*for (i = 1;i < nplots;i++)*/
	} /*if (add)*/
	else
	{
		nplots = 0;
	}

/* loop over columns of y, adding new struct curve_point for each col*/
	cstart = 0;
	for (iplot = 0;iplot < ncols; iplot++)
	{
		TMPHANDLE = mygethandle(sizeof(curve_points));
		this_plot = (curve_points **) TMPHANDLE;
		if (nplots == 0)
		{
			GRAPHFIRST_PLOT(graph) = this_plot;
		}
		else
		{
			NEXTGRAPH(last_plot) = this_plot;
		}
		if (TMPHANDLE == (char **) 0)
		{
			goto errorExit;
		}
		mylockhandle(TMPHANDLE);

		last_plot = this_plot;
		nplots++;
		GRAPHNPLOTS(graph) = nplots;
		NEXTGRAPH(this_plot) = (curve_points **) 0;
		GRAPHPOINTS(this_plot) = (coordinate **) 0;

		TMPHANDLE = mygethandle(nrows * sizeof(coordinate));
		GRAPHPOINTS(this_plot) = (coordinate **) TMPHANDLE;
		if (TMPHANDLE == (char **) 0)
		{
			goto errorExit;
		}
		mylockhandle(TMPHANDLE);

		PLOTSTYLE(this_plot) = style;

		POINTTYPE(this_plot) = (iplot == 0) ? 5 : iplot-1;
		LINETYPE(this_plot) = iplot + linetype;

		PCOUNT(this_plot) = nrows;

		if (op & CHARPLOT)
		{
			cstart = (cCols > 1 && iplot > 0) ?
				skipStrings(*plotcH+cstart,cRows) - *plotcH  : 0;
			cplace = cstart;
			j = 0;
		} /*if (op & CHARPLOT)*/

		ij = iplot*nrows;		/* iplot*nrows + i */
		for (i = 0; i < nrows; i++, ij++)
		{
			xi = (*x)[i];
			yi = (*y)[ij];
			if (isMissing(xi) || isMissing(yi))
			{
				undef_point(this_plot,i);
			}
			else
			{
				set_point(this_plot,i,xi,yi,
						  (op & CHARPLOT) ? *plotcH + cplace : (char *) 0,
						  pointColor, lineColor);
			}

			if (op & CHARPLOT)
			{
				j++;
				/* characters are wrapped around cyclically (kb) */
				if (j >= cRows)
				{
					j = 0;
					cplace = cstart;
				}
				else
				{
					cplace = skipStrings(*plotcH + cplace,1) - *plotcH;
				}
			} /*if (op & CHARPLOT)*/
		} /*for (i = 0; i < nrows; i++, ij++)*/
	} /*for (iplot = 0;iplot < ncols; iplot++)*/
	return (1);

  errorExit:
	emptyTrash();
	return(0);
	
} /*setGraphPoints()*/

/*
   build or add components to whole_graph structure graph
   970515 modified so that it handles showplot() case, too, setting
   only extremes
*/
static long buildPlot(long op, whole_graph **graph,
					  Symbolhandle symhx, Symbolhandle  symhy,
					  Symbolhandle symhc,
					  long style, long cmax, long nx, long ny, long nchars,
					  long ncols, long cRows, long cCols,
					  unsigned long findExtremes,
					  plotKeyValuesPtr keyValues)
{
	int                   ok = !(op & NEEDXY);
	WHERE("buildPlot");

	while (!ok) /* loop traversed at most once */
	{ /* not showplot() */
		double              **x = DATA(symhx), **y = DATA(symhy);
		double                value;
		long                  nvals = ncols*ny;
		long                  notMissing = 0;
		long                  i;
		long                  findXmin = (findExtremes & FINDXMIN);
		long                  findXmax = (findExtremes & FINDXMAX);
		long                  findYmin = (findExtremes & FINDYMIN);
		long                  findYmax = (findExtremes & FINDYMAX);

		for (i = 0;i < nx;i++)
		{
			value = (*x)[i];
			if (!isMissing(value))
			{
				notMissing++;
				keyValues->xmin = (findXmin && value < keyValues->xmin) ? value : keyValues->xmin;
				keyValues->xmax = (findXmax && value > keyValues->xmax) ? value : keyValues->xmax;
			}
		} /*for (i = 0;i < nx;i++)*/
		if (notMissing == 0)
		{
			sprintf(OUTSTR,"ERROR: no non-missing x values for %s to plot",
					FUNCNAME);
			goto errorExit;
		}

		notMissing = 0;

		for (i = 0;i < nvals;i++)
		{
			value = (*y)[i];
			if (!isMissing(value))
			{
				notMissing++;
				keyValues->ymin = (findYmin && value < keyValues->ymin) ? value : keyValues->ymin;
				keyValues->ymax = (findYmax && value > keyValues->ymax) ? value : keyValues->ymax;
			}
		} /*for (i = 0;i < nvals;i++)*/
		if (notMissing == 0)
		{
			sprintf(OUTSTR,"ERROR: no non-missing y values for %s to plot",
					FUNCNAME);
			goto errorExit;
		}
		
		if (!(op & STRINGPLOT))
		{
			if (!setGraphPoints(op, x, y, symhc, ny, ncols, cRows, cCols,
								nchars, cmax, graph, style,
								keyValues->linetype, keyValues->add))
			{
				goto errorExit;
			}
		} /*if (!(op & STRINGPLOT))*/
		else
		{
			/* addstrings()*/
			if (!setGraphStrings(x, y, ny, symhc, graph, keyValues->justify))
			{
				goto errorExit;
			}
		} /*if (!(op & STRINGPLOT)){}else{}*/
		ok = 1;
		/* fall through */

	  errorExit:
		if (!ok)
		{
			putErrorOUTSTR();
			break;
		}
	} /*while (!ok)*/
	
	if (ok)
	{
		GRAPHXMIN(graph) = keyValues->xmin;
		GRAPHXMAX(graph) = keyValues->xmax;
		GRAPHYMIN(graph) = keyValues->ymin;
		GRAPHYMAX(graph) = keyValues->ymax;
	}
	
	return (ok);
} /* buildPlot()*/

#define TICKLENGTHNOTSET (4*TICKUNIT)
/*
  970515 moved (and appropriately modified) code from myplot() to here

  Add x- (isX != 0) or y- ticks (isX == 0) to graph
  Dispose of previous ticks if necessary (we're modifying and replacing
  LASTPLOT)
  
  Argument nticks is ignored if ticks != (Symbolhandle) 0
*/
static void setTicks(whole_graph ** graph, Symbolhandle ticks, long nticks,
					 unsigned short ticklength, int dispose, int isX)
{
	if (ticks != UNSETSYMH)
	{
		nticks = symbolSize(ticks);
		if (dispose)
		{ /* graph from LASTPLOT which will be updated */
			mydisphandle((char **) ((isX) ?
									GRAPHXTICKS(graph) : GRAPHYTICKS(graph)));
			if (isX)
			{
				GRAPHXTICKS(graph) = (double **) 0;
			}
			else
			{
				GRAPHYTICKS(graph) = (double **) 0;
			}
		} /*if (dispose)*/

		if (nticks > 1 || !isNull(ticks) && !isMissing(DATAVALUE(ticks, 0)))
		{
			if (isX)
			{
				GRAPHXTICKS(graph) = DATA(ticks);
			}
			else
			{
				GRAPHYTICKS(graph) = DATA(ticks);
			}
		}
		setDATA(ticks, (double **) 0);
	} /*if (ticks != UNSETSYMH)*/

	if (ticklength == TICKLENGTHNOTSET)
	{
		ticklength = TICKUNIT;
	} /*if (ticklength == TICKLENGTHNOTSET)*/
	
	if (isX)
	{
		GRAPHNXTICKS(graph) = nticks;
		GRAPHXTICKLENGTH(graph) = ticklength;
	} /*if (isX)*/
	else
	{
		GRAPHNYTICKS(graph) = nticks;
		GRAPHYTICKLENGTH(graph) = ticklength;
	} /*if (is(X)){}else{}*/
} /*setTicks()*/

#ifdef EPX
static void outwitGcc(int i)
{
	;
} /*outwitGcc()*/
#endif /*EPX*/

/* for debugging purposes*/
#ifdef DUMPPLOTKEYS
void dumpPlotKeys(char * /*where*/, char * /*msg*/,
				  plotKeyValuesPtr /*keyValues*/);
#endif /*DUMPPLOTKEYS*/
/*
   General entry for plot(), lineplot(), chplot(), fplot(), flineplot(),
   fchplot(), addpoints(), addlines(), addchars(), addstrings()

   960503 changed macOpen() to macFindFile()
   960522 added capability to save PICT and EPSF files on mac
   960524 Default value for window is 0 (use last window) with add:T or any of
          addlines(), addpoints(), addchars(), addstrings().
*/

Symbolhandle    myplot(Symbolhandle list)
{
	Symbolhandle          symhx = (Symbolhandle) 0;
	Symbolhandle          symhy = (Symbolhandle) 0;
	Symbolhandle          symhc = (Symbolhandle) 0;
	Symbolhandle          symhstruc = (Symbolhandle) 0;
	Symbolhandle          symhfile = (Symbolhandle) 0;
	Symbolhandle          graphsymbol = (Symbolhandle) 0;
	Symbolhandle          lastplot = (Symbolhandle) 0;
	Symbolhandle          oldplot = (Symbolhandle) 0;
	Symbolhandle          stringsymbol = (Symbolhandle) 0;
	Symbolhandle          trash = (Symbolhandle) 0;
	whole_graph         **graph = (whole_graph **) 0;
	plotKeyValues         keyValues;
	char                  filePath[PATHSIZE+1];
#ifdef HASFINDFILE
	char                 *tmpFileName;
#endif /*HASFINDFILE*/
	FILE                 *plotfile = STDOUT;
	long                  termType = STANDARDTERM;
	long                  xisIndex = 0;
	long                  minNargs, nargs, startArg, startKey, first;
	long                  charArg = -1, plotArg = -1;
	long                  i, nchars;
	long                  dims[2], nx, ny, cRows, cCols;
	long                  op, toFile = 0, showPlot, strucArg = 0;
	long                  ncols = 0;
	long                  style = POINTS;
	long                  nxticks = 1, nyticks = 1;
	long                  windno;
	int                   usePreviousGraph, useLASTPLOT;
	double                hugedbl = HUGEDBL;
	double                extremes[4];
	unsigned long         findExtremes = 0;
	double                x0, incx;
	double                cmax = 0.0, value;
	char                 *keyword, *name;
	WHERE("myplot");

	OUTSTR[0] = '\0';
	
	for (i=0; Ops[i].name != (char *) 0;i++)
	{
		if (strcmp(FUNCNAME,Ops[i].name) == 0)
		{
			op = Ops[i].iop;
			break;
		}
	}
	if (Ops[i].name == (char *) 0)
	{ /* should never happen */
		sprintf(OUTSTR,"ERROR: (internal) %s not recognized in myplot()",
				FUNCNAME);
		goto errorExit;
	}

	unsetPlotKeys(&keyValues, 1);
	keyValues.landscape = 0;
	keyValues.justify = (op & STRINGPLOT) ? CENTRE : keyValues.justify;
	keyValues.xticklength = keyValues.yticklength = TICKLENGTHNOTSET;
	keyValues.pause = DEFAULTPLOTPAUSE;
	
	showPlot = !(op & NEEDXY);

#ifdef EPX
/*
  the following was added to outflank a possible gcc compiler bug
  without it, showPlot turned up as 1 later on even when it was 0 here
*/
	outwitGcc(showPlot);
#endif /*EPX*/

	nargs = NARGS(list);
	
	/* scan for undefined arguments */
	for (i = 0; i < nargs; i++)
	{
		Symbolhandle        argi = COMPVALUE(list, i);
		
		if (argi != (Symbolhandle) 0 && !isDefined(argi))
		{
			undefArg(FUNCNAME, argi, i+1);
			goto errorExit;
		}
	} /*for (i = 0; i < nargs; i++)*/
	
	/* minNargs is the minimum number of required arguments */	
	minNargs = (showPlot) ? 0 : 1;
	minNargs = (op & STRINGPLOT) ? 2 : minNargs;

	/* COMPVALUES(list,minNargs) is the first possible keyword argument*/
	startKey = minNargs;

	/* COMPVALUES(list,startArg) is first non-file name, non GRAPH argument*/
	startArg = 0;

	symhfile = COMPVALUE(list,0);

	toFile = (op & TOFILE ||
			  (keyword = isKeyword(symhfile)) && strcmp(keyword,"file") == 0);
	if (toFile)
	{
		minNargs++;
		startKey++;
		startArg++;
		if (!argOK(symhfile,CHAR,1))
		{
			goto errorExit;
		}
		if (!isScalar(symhfile))
		{
			notCharOrString("file name");
			goto errorExit;
		}
		keyValues.file = expandFilename(STRINGPTR(symhfile));
		if (keyValues.file == (char *) 0)
		{
			goto errorExit;
		}
		strcpy(filePath, keyValues.file);
		keyValues.file = filePath;
	} /*if (toFile)*/

	oldplot = COMPVALUE(list, startArg);
	if (oldplot != (Symbolhandle) 0 && TYPE(oldplot) == PLOTINFO)
	{
		plotArg = startArg++;
		startKey++;
		minNargs++;
	}
	else
	{
		oldplot = (Symbolhandle) 0;
	}
	
	if (!showPlot)
	{
		strucArg = (nargs > startArg && isStruc(COMPVALUE(list,startArg)));
		if (strucArg)
		{
			symhstruc = COMPVALUE(list,startArg);
			if (NCOMPS(symhstruc) < 2)
			{
				sprintf(OUTSTR,
						"ERROR: Structure argument to %s() must have at least 2 components",
						FUNCNAME);
				goto errorExit;
			}
		} /*if (strucArg)*/
		else
		{
			minNargs++;
			startKey++;
		}
		if (op & CHARPLOT && nargs > startKey &&
			!isKeyword(COMPVALUE(list, startKey)))
		{
			charArg = startKey++;
		}
	} /*if (!showPlot)*/
	
	if (nargs < minNargs)
	{
		badNargs(FUNCNAME,-(minNargs + 1000));
		goto errorExit;
	}

#ifndef MULTIPLOTWINDOWS
	keyValues.window = DONTSETLONG;
#endif /*MULTIPLOTWINDOWS*/

#ifndef SCREENDUMP
	keyValues.screendump = DONTSETPOINTER;
#endif /*SCREENDUMP*/

#ifndef EPSFPLOTSOK
	keyValues.epsf = DONTSETCHAR;
#endif /*EPSFPLOTSOK*/

	keyValues.show = (toFile) ? DONTSETCHAR : UNSETCHAR;
	keyValues.add = (op & ADDTOPLOT) ? DONTSETCHAR : UNSETCHAR;
	keyValues.impulses = (showPlot) ? DONTSETCHAR : 0;
	keyValues.lines = (showPlot) ? DONTSETCHAR : ((op & LINEPLOT) != 0);
	keyValues.linetype = (showPlot) ? DONTSETLONG : UNSETLONG;
	keyValues.thickness = (showPlot) ? DONTSETREAL : UNSETREAL;
	
	first = getPlotKeys(list, startKey, &keyValues);
	/* File names are now (951113) expanded by getPlotKeys()*/

	if (first < 0)
	{ /*error found by getPlotKeys()*/
		goto errorExit;
	}
	
	unsetPlotKeys(&keyValues, 0); /* change DONTSETs to UNSETs */
	keyValues.add = (op & ADDTOPLOT) ? 1 : keyValues.add;

	toFile = (keyValues.file != UNSETPOINTER);
	
	if (!checkPlotKeys(toFile, &keyValues, &termType))
	{
		goto errorExit;
	}
#ifdef DUMPPLOTKEYS
	if (GUBED & 8)
	{
		dumpPlotKeys(NAMEFORWHERE, " after checkPlotKeys()",
					 &keyValues);
	}
#endif /*DUMPPLOTKEYS*/

	if (plotArg >= 0)
	{
		if (!keyValues.add)
		{
			sprintf(OUTSTR,
					"ERROR: add:F is illegal on %s() when GRAPH variable is argument",
					FUNCNAME);
			goto errorExit;
		}
		keyValues.add = !showPlot;
	} /*if (plotArg >= 0)*/
	else
	{
		keyValues.add = (keyValues.add != UNSETCHAR) ? keyValues.add : 0;
	}

	if (keyValues.add)
	{
		op |= ADDTOPLOT;
	}

	if (op & LINEPLOT && !keyValues.lines)
	{
#if (1) /*980616*/
		/* pretend it's plot()*/
		op = PTPLOT | (op & ~LINEPLOT);
#else /*1*/
		sprintf(OUTSTR,
				"ERROR: lines:F illegal with %s()", FUNCNAME);
		goto errorExit;
#endif /*1*/
	}

	if (!showPlot && keyValues.linetype != UNSETLONG)
	{
		keyValues.linetype--; /* now between 0 and MAXLINETYPE-1 */
		if (!keyValues.lines)
		{
			putOutErrorMsg("WARNING: keyword 'linetype' ignored when not drawing lines");
		}
	} /*if (!showPlot && keyValues.linetype != UNSETLONG)*/
	else
	{
		keyValues.linetype = 0;
	}
	
	if (!showPlot && keyValues.thickness != UNSETREAL)
	{
		if (!keyValues.lines)
		{
			putOutErrorMsg("WARNING: keyword 'thickness' ignored when not drawing lines");
		}
		else
		{
	/*
	  Code thickness into linetype so actual type is linetype % MAXLINETYPE
	  After this point, keyValues.thickness is irrelevant
	 */
			keyValues.linetype += MAXLINETYPE *
				(long) ((double) UNITWIDTH * keyValues.thickness / (double) MAXLINETYPE);
		}
	} /*if (!showPlot && thickness != UNSETREAL)*/
	
	if (toFile && keyValues.file != filePath)
	{
		strcpy(filePath, keyValues.file);
		keyValues.file = filePath;
	} /*if (toFile && keyValues.file != filePath)*/	

#ifdef SCREENDUMP
	/* Expanding file name is now done by getPlotKeys() */
	if (keyValues.screendump != UNSETPOINTER)
	{
		if (termType != STANDARDTERM)
		{
			sprintf(OUTSTR,
					"ERROR: screendump:fileName can only be used with high resolution plot");
			goto errorExit;
		}
#ifdef MACINTOSH
		if(keyValues.screendump[0] != '\0' && !isfilename(keyValues.screendump))
#else  /*MACINTOSH*/
		if(!isfilename(keyValues.screendump))
#endif /*MACINTOSH*/
		{
			goto errorExit;
		}
	} /*if (keyValues.screendump != UNSETPOINTER)*/
#endif /*SCREENDUMP*/

#ifdef DUMPPLOTKEYS
	if (GUBED & 8)
	{
		dumpPlotKeys(NAMEFORWHERE, " before Lookup()",
					 &keyValues);
	}
#endif /*DUMPPLOTKEYS*/

	lastplot = Lookup(LastGraphName); /*find LASTPLOT */
		
	usePreviousGraph = keyValues.add || showPlot;
	useLASTPLOT = usePreviousGraph && lastplot != (Symbolhandle) 0 &&
	  (oldplot == (Symbolhandle) 0 || oldplot == lastplot);
	
	if (usePreviousGraph)
	{
		/* adding to or displaying data from earlier plot */

		if (useLASTPLOT && oldplot != (Symbolhandle) 0)
		{ /*LASTPLOT is argument; remove from arg list */
			COMPVALUE(list,plotArg) = (Symbolhandle) 0;
		}

		/*
		   If oldplot != 0 it is known to have type PLOTINFO and
		   may or may not be the same as LASTPLOT
		*/
		if (oldplot == (Symbolhandle) 0)
		{
			/* use LASTPLOT if no PLOTINFO variable provided */
			if (lastplot == (Symbolhandle) 0)
			{
				sprintf(OUTSTR,
						"ERROR: %s not found by %s()",LastGraphName,FUNCNAME);
			}
			else if (TYPE(lastplot) != PLOTINFO)
			{
				sprintf(OUTSTR,"ERROR: variable %s not of type %s",
						LastGraphName, typeName(PLOTINFO));
			}
			if (*OUTSTR)
			{
				goto errorExit;
			}
			oldplot = lastplot;
		} /*if (oldplot == (Symbolhandle) 0 || TYPE(oldplot) != PLOTINFO)*/
		/*
		   At this point oldplot is either a supplied graph symbol or
		   lastplot
		*/

		if (showPlot)
		{
			/*
			   Duplicate just GRAPH(oldplot), but not contents of
			   its data handles
			*/
			trash = GarbInstall(NTRASH);
			if (trash == (Symbolhandle) 0 ||
				!getScratch(graph, GGRAPH, 1, whole_graph))
			{
				goto errorExit;
			}
			memcpy((void *) *graph, (void *) GRAPHPTR(oldplot),
				   sizeof(whole_graph));
			/*
			   Note that the data associated with graph will not be deleted
			   when the trash is emptied, only the whole_graph structure
			   itself
			*/
		} /*if (showPlot)*/
		else
		{ /*add != 0*/
			/* make copy of entire oldplot including data*/
			graphsymbol = Install(SCRATCH, PLOTINFO);
			if (graphsymbol == (Symbolhandle) 0 ||
				!Copy(oldplot, graphsymbol))
			{
				goto errorExit;
			}
			setNAME(graphsymbol, SCRATCH);
			graph = GRAPH(graphsymbol);
		} /*if (showPlot){}else{}*/

		/* save info from graph only if not replaced by keywords */
		keyValues.xmin =
		  (isMissing(keyValues.xmin) || keyValues.xmin != UNSETREAL) ?
		  keyValues.xmin : GRAPHXMIN(graph);
		keyValues.xmax =
		  (isMissing(keyValues.xmax) || keyValues.xmax != UNSETREAL) ?
		  keyValues.xmax : GRAPHXMAX(graph);
		keyValues.ymin =
		  (isMissing(keyValues.ymin) || keyValues.ymin != UNSETREAL) ?
		  keyValues.ymin : GRAPHYMIN(graph);
		keyValues.ymax =
		  (isMissing(keyValues.ymax) || keyValues.ymax != UNSETREAL) ?
		  keyValues.ymax : GRAPHYMAX(graph);

		if (isMissing(keyValues.xmin) ||
			!isMissing(keyValues.xmax) && keyValues.xmin >= keyValues.xmax)
		{
			findExtremes |= FINDXMIN;
		}
		if (isMissing(keyValues.xmax) ||
			!isMissing(keyValues.xmin) && keyValues.xmin >= keyValues.xmax)
		{
			findExtremes |= FINDXMAX;
		}
		if (isMissing(keyValues.ymin) ||
			!isMissing(keyValues.ymax) && keyValues.ymin >= keyValues.ymax)
		{
			findExtremes |= FINDYMIN;
		}
		if (isMissing(keyValues.ymax) ||
			!isMissing(keyValues.ymin) && keyValues.ymin >= keyValues.ymax)
		{
			findExtremes |= FINDYMAX;
		}
		if (findExtremes & (FINDXMIN | FINDXMAX | FINDYMIN | FINDYMAX))
		{
			getGraphExtremes(graph, extremes, 1);

			keyValues.xmin = (findExtremes & FINDXMIN) ?
			  extremes[0] : keyValues.xmin;
			keyValues.xmax = (findExtremes & FINDXMAX) ?
			  extremes[1] : keyValues.xmax;
			keyValues.ymin = (findExtremes & FINDYMIN) ?
			  extremes[2] : keyValues.ymin;
			keyValues.ymax = (findExtremes & FINDYMAX) ?
			  extremes[3] : keyValues.ymax;
		} /*if (findExtremes & (FINDXMIN | FINDXMAX | FINDYMIN | FINDYMIN))*/
		
		nxticks = GRAPHNXTICKS(graph);
		nyticks = GRAPHNYTICKS(graph);
		keyValues.xticklength = (keyValues.xticklength != TICKLENGTHNOTSET) ?
			keyValues.xticklength : GRAPHXTICKLENGTH(graph);
		keyValues.yticklength = (keyValues.yticklength != TICKLENGTHNOTSET) ?
			keyValues.yticklength : GRAPHYTICKLENGTH(graph);
		keyValues.logx = (keyValues.logx != UNSETCHAR) ?
		  keyValues.logx : GRAPHLOGX(graph);
		keyValues.logy = (keyValues.logy != UNSETCHAR) ?
		  keyValues.logy : GRAPHLOGY(graph);
		keyValues.xaxis = (keyValues.xaxis != UNSETCHAR) ?
		  keyValues.xaxis : GRAPHXAXIS(graph);
		keyValues.yaxis = (keyValues.yaxis != UNSETCHAR) ?
		  keyValues.yaxis : GRAPHYAXIS(graph);
		if (keyValues.xlab[0] == '\0')
		{
			strcpy(keyValues.xlab, GRAPHXLAB(graph));
		}
		if (keyValues.ylab[0] == '\0')
		{
			strcpy(keyValues.ylab, GRAPHYLAB(graph));
		}
		if (keyValues.title[0] == '\0')
		{
			strcpy(keyValues.title, GRAPHTITLE(graph));
		}
	} /*if (usePreviousGraph)*/
	else
	{ /* not showplot() or adding information */
		graphsymbol = GraphInstall(SCRATCH);
		if (graphsymbol == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		graph = GRAPH(graphsymbol);
		if (isMissing(keyValues.xmin) || keyValues.xmin == UNSETREAL)
		{
			findExtremes |= FINDXMIN;
		}
		if (isMissing(keyValues.xmax) || keyValues.xmax == UNSETREAL)
		{
			findExtremes |= FINDXMAX;
		}
		if (isMissing(keyValues.ymin) || keyValues.ymin == UNSETREAL)
		{
			findExtremes |= FINDYMIN;
		}
		if (isMissing(keyValues.ymax) || keyValues.ymax == UNSETREAL)
		{
			findExtremes |= FINDYMAX;
		}
		if (!(findExtremes & (FINDXMIN | FINDXMAX)) &&
			keyValues.xmin >= keyValues.xmax)
		{
			findExtremes |= FINDXMIN | FINDXMAX;
		}
		if (!(findExtremes & (FINDYMIN | FINDYMAX)) &&
			keyValues.ymin >= keyValues.ymax)
		{
			findExtremes |= FINDYMIN | FINDYMAX;
		}

		keyValues.xmin = (findExtremes & FINDXMIN) ? hugedbl : keyValues.xmin;
		keyValues.xmax = (findExtremes & FINDXMAX) ? -hugedbl : keyValues.xmax;
		keyValues.ymin = (findExtremes & FINDYMIN) ? hugedbl : keyValues.ymin;
		keyValues.ymax = (findExtremes & FINDYMAX) ? -hugedbl : keyValues.ymax;

	} /*if (usePreviousGraph){}else{}*/

	keyValues.logx = (keyValues.logx != UNSETCHAR) ? keyValues.logx : 0;
	keyValues.logy = (keyValues.logy != UNSETCHAR) ? keyValues.logy : 0;
	keyValues.xaxis = (keyValues.xaxis != UNSETCHAR) ? keyValues.xaxis : 1;
	keyValues.yaxis = (keyValues.yaxis != UNSETCHAR) ? keyValues.yaxis : 1;

	if (!showPlot)
	{ /* get x and y */
		char    *whatx = (strucArg) ? "component 1 of structure" : "x";
		char    *whaty = (strucArg) ? "component 2 of structure" : "y";

		if (!strucArg)
		{
			symhx = COMPVALUE(list, startArg );
			symhy = COMPVALUE(list, startArg+1);
			if (!argOK(symhx, 0, startArg+1) ||
				!argOK(symhy, 0, startArg+2))
			{
				goto errorExit;
			}
		} /*if (!strucArg)*/
		else
		{
			symhx = COMPVALUE(symhstruc,0);
			symhy = COMPVALUE(symhstruc,1);
		}
		
		nx = symbolSize(symhx);
		xisIndex = (!(op & STRINGPLOT) && nx <= 2 && isVector(symhx)) ? 1 : 0;
		if (TYPE(symhx) != REAL || !isVector(symhx))
		{
			sprintf(OUTSTR,
					"ERROR: %s must be a REAL vector for %s()",
					whatx, FUNCNAME);
		}
		else if (xisIndex && anyMissing(symhx))
		{
			sprintf(OUTSTR,
					"ERROR: %s for %s() must have no MISSING values when it is to be expanded",
					whatx, FUNCNAME);
		}
		else if (op & STRINGPLOT &&
				 (TYPE(symhy) != REAL || !isMatrix(symhy,dims) && dims[1] > 1))
		{
			sprintf(OUTSTR,
					"ERROR: %s must be a REAL vector for %s()",
					whaty, FUNCNAME);
		}
		else if (!(op & STRINGPLOT) &&
				 (TYPE(symhy) != REAL || !isMatrix(symhy,dims)))
		{
			sprintf(OUTSTR,
					"ERROR: %s must be a REAL vector or matrix for %s()",
					whaty, FUNCNAME);
		}
		else if (nx != (ny = dims[0]) && !xisIndex)
		{
			sprintf(OUTSTR,
					"ERROR: %s and %s do not have same number of rows for %s()",
					whatx, whaty, FUNCNAME);
		}
		else if (!keyValues.add)
		{
			/*
				use names or keywords as axis labels only when not adding info
				and when they have not been set by keywords 'xlab' & 'ylab'
			*/
			if (keyValues.xlab[0] == '\0')
			{
				name = NAME(symhx);
				if ((keyword = isKeyword(symhx)))
				{
					strncpy(keyValues.xlab,keyword,XLABELSIZE);
				}
				else if (!isscratch(name) && strcmp(name,NUMSCRATCH+2) != 0 &&
						strcmp(name,VECSCRATCH+2) != 0 &&
						strcmp(name,MATSCRATCH+2) != 0)
				{
					strncpy(keyValues.xlab,name + ((istempname(name)) ? 1 : 0),
							XLABELSIZE);
				}
				keyValues.xlab[XLABELSIZE] = '\0';
			} /*if (keyValues.xlab[0] == '\0')*/

			if (keyValues.ylab[0] == '\0')
			{
				name = NAME(symhy);
				if ((keyword = isKeyword(symhy)))
				{
					strncpy(keyValues.ylab,keyword,YLABELSIZE);
				}
				else if (!isscratch(name) && strcmp(name,NUMSCRATCH+2) != 0 &&
						strcmp(name,VECSCRATCH+2) != 0 &&
						strcmp(name,MATSCRATCH+2) != 0)
				{
					strncpy(keyValues.ylab,name + ((istempname(name)) ? 1 : 0),
							YLABELSIZE);
				}
				keyValues.ylab[YLABELSIZE] = '\0';
			} /*if (keyValues.ylab[0] == '\0')*/
		} /*if (!keyValues.add)*/
		ncols = dims[1];	/* number of columns of y */
	} /* if (!(showPlot) */

	if (*OUTSTR)
	{
		goto errorExit;
	}

	if (toFile)
	{
		/* fileName must be pointing to filePath */
#ifdef HASFINDFILE
		tmpFileName = findPlotFile(keyValues.file, "Graph", keyValues.ps, keyValues.epsf, keyValues.newFile);

		if (tmpFileName == (char *) 0)
		{ /* Canceled */
			return (0);
		}
		if (tmpFileName != keyValues.file)
		{
			strncpy(keyValues.file, tmpFileName, PATHSIZE);
		}
#endif /*HASFINDFILE*/
		if (!isfilename(keyValues.file))
		{
			goto errorExit;
		}
		/* should have filename */
	} /* if (toFile) */

 /* create equally spaced x-variable if needed */
	xisIndex = (!showPlot && nx != ny);
	if (xisIndex)
	{
		x0 = DATAVALUE(symhx,0);
		incx = (nx > 1) ? DATAVALUE(symhx,1) : 1.0;
		nx = ny;
		/* at this point we are done with original symhx; make a new one */
		symhx = RInstall(SCRATCH,nx);
		if (symhx == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		xisIndex = 2;		/* indicates symhx needs to be disposed of */
		for (i = 0;i < nx;i++)
		{
			DATAVALUE(symhx,i) = x0 + incx * (double) i;
		}
	} /*if (xisIndex)*/

	if (op & CHARPLOT)
	{/* find out what characters to plot */
		if (charArg >= 0)
		{ /* characters or integers are supplied; known not keyword */
			if ((symhc = COMPVALUE(list,charArg)) == (Symbolhandle) 0)
			{
				sprintf(OUTSTR,"ERROR: %s(%s%schars)", FUNCNAME,
						(toFile) ? "filename," : "",
						(strucArg) ? "structure(x,y)," : "x,y,");
			}
			else if (TYPE(symhc) != REAL && TYPE(symhc) != CHAR)
			{
				sprintf(OUTSTR,
						"ERROR: chars must be REAL or CHARACTER for %s()",
						FUNCNAME);
			}
			else if (!isMatrix(symhc,dims))
			{
				sprintf(OUTSTR,
						"ERROR: chars must be a vector%s for %s()",
						(ncols > 0) ? " or matrix" : "", FUNCNAME);
			}
			else if ((cCols = dims[1]) != 1 && cCols != ncols)
			{
				sprintf(OUTSTR,"ERROR: ncols(chars) must be 1 or match ncols(y)");
			}
			else if (TYPE(symhc) == REAL)
			{ /* check to see values are integers */
				cRows = dims[0];
				if (cCols == 1 && cRows == ncols)
				{
					cCols = ncols;
					cRows = 1;
				}

				nchars = cRows*cCols;

				for (i = 0;i < nchars;i++)
				{
					if ((value = DATAVALUE(symhc,i)) < 0.0 || value > 999.0 ||
						value != floor(value))
					{
						sprintf(OUTSTR,
								"ERROR: numerical chars for %s not integers or < 0 or >= 1000",
								FUNCNAME);
						break;
					}
					cmax = (value > cmax) ? value : cmax;
				} /*for (i = 0;i < nchars;i++)*/
			}
			else
			{ /* characters */
				cRows = dims[0];
				if (cCols == 1 && cRows == ncols)
				{
					cCols = ncols;
					cRows = 1;
				}
			}
		} /*if (charArg >= 0)*/
		else
		{ /* no labelling information supplied */
			symhc = (Symbolhandle) 0;
			if (ncols > 1)
			{
				cRows = 1;
				cCols = ncols;
			}
			else
			{
				cRows = nx;
				cCols = 1;
			}
			nchars = cRows*cCols;
			/* negative cmax indicates labels will need to be built */
			cmax = -((nchars < 999) ? nchars : 999);
		} /*if (charArg >= 0){}else{}*/
	} /* if (op & CHARPLOT) */
	else if (op & STRINGPLOT)
	{
		char     *pc;
		
		stringsymbol = COMPVALUE(list,startArg+2);
		if (!isVector(stringsymbol) || TYPE(stringsymbol) != CHAR ||
		   symbolSize(stringsymbol) != ny)
		{
			sprintf(OUTSTR,
					"ERROR: strings not a CHARACTER vector of the same length as x for %s()",
					FUNCNAME);
		}
		pc = STRINGPTR(stringsymbol);
		
		for (i = 0; i < ny; i++)
		{
			if (strlen(pc) > MAXCHARITEMSIZE)
			{
				sprintf(OUTSTR,
						"ERROR: all strings for %s() must be no longer than %ld characters",
						FUNCNAME, (long) MAXCHARITEMSIZE);
				break;
			}
			pc = skipStrings(pc, 1);
		} /*for (i = 0; i < ny; i++)*/
	}

	if (*OUTSTR)
	{
		goto errorExit;
	}

	/* set style of points to be plotted */
	if (op & CHARPLOT)
	{
		style = CHARS | ((keyValues.impulses) ? IMPULSES : 0) |
		  ((keyValues.lines) ? LINES : 0);
	}
	else if (op & PTPLOT)
	{
		style = ((keyValues.impulses) ? IMPULSES : POINTS) |
		  ((keyValues.lines) ? LINES : 0);
	}
	else if (op & LINEPLOT)
	{
		style = LINES | ((keyValues.impulses) ? IMPULSES : 0);
	}

	windno = (keyValues.window != UNSETLONG) ?
	  keyValues.window : ((keyValues.add) ? 0 : FINDEMPTYWINDOW);

#if defined(EPSFPLOTSOK) && 0
	if (keyValues.epsf)
	{
		putOutErrorMsg("WARNING: encapsulated PostScript not yet implemented");
		keyValues.epsf = 0;
	}
#endif /*EPSFPLOTSOK*/

	if (toFile)
	{
		plotfile = openPlotFile(keyValues.file,keyValues.newFile);
		if (plotfile == (FILE *) NULL)
		{
			goto errorExit;
		}
    }
	else
	{
		plotfile = STDOUT;
	}

	
#ifdef DUMPPLOTKEYS
	if (GUBED & 8)
	{
		dumpPlotKeys(NAMEFORWHERE, " before buildPlot()",
					 &keyValues);
	}
#endif /*DUMPPLOTKEYS*/
	if (!buildPlot(op, graph, symhx, symhy,
				   (op & STRINGPLOT) ? stringsymbol : symhc,
				   style, (long) cmax, nx, ny, nchars, ncols, cRows, cCols,
				   findExtremes, &keyValues))
	{
		goto errorExit;
	}

	setTicks(graph, keyValues.xticks, nxticks, keyValues.xticklength,
			 keyValues.keep && oldplot == lastplot, 1);
	setTicks(graph, keyValues.yticks, nyticks, keyValues.yticklength,
			 keyValues.keep && oldplot == lastplot, 0);

	GRAPHFGCOLOR(graph) = DEFAULTFGCOLOR;
	GRAPHBGCOLOR(graph) = DEFAULTBGCOLOR;
	GRAPHFRAMECOLOR(graph) = DEFAULTFRAMECOLOR;

	GRAPHLOGX(graph) = keyValues.logx;
	GRAPHLOGY(graph) = keyValues.logy;

	GRAPHXAXIS(graph) = keyValues.xaxis;
	GRAPHYAXIS(graph) = keyValues.yaxis;

	strcpy(GRAPHXLAB(graph),keyValues.xlab);
	strcpy(GRAPHYLAB(graph),keyValues.ylab);
	strcpy(GRAPHTITLE(graph),keyValues.title);

	if (keyValues.keep)
	{
		/* save plotting info in LASTPLOT */
		if (showPlot)
		{
			/*
			  lastplot is either LASTPLOT or (Symbolhandle) 0
			  oldplot is a GRAPH symbol argument, if there was one
			    or lastplot

			   At this point graph is a scratch handle but its data handles
			   are the same as those of oldplot.  If oldplot != lastplot,
			   the data handles must be duplicated.
			   */
			if (oldplot != lastplot)
			{
				Removesymbol(lastplot);
				lastplot = (Symbolhandle) 0;
			}
			
			if (lastplot == (Symbolhandle) 0)
			{
				lastplot = Install(LastGraphName, PLOTINFO);
				if (lastplot == (Symbolhandle) 0)
				{
					goto errorExit;
				}
			}
			else
			{ /* updating LASTPLOT, graph is copy of GRAPH(lastplot) */
				mydisphandle((char **) GRAPH(lastplot));
				setGRAPH(lastplot, (whole_graph **) 0);
			}

			if (oldplot != lastplot)
			{ 
				/*
				  graph is a copy of GRAPH(oldplot), but data && tick info
				  was not duplicated.
				  Easiest thing is to reduplicate with its contents and
				  then discard graph without destroying its contents
				*/

				whole_graph    **graph1 = copyGraph(graph);

				if (graph1 == (whole_graph **) 0)
				{
					goto errorExit;
				} 
				discardScratch(graph, GGRAPH, whole_graph);
				graph = graph1;
			} /*if (oldplot != lastplot)*/
			else
			{ /*updating LASTPLOT; we will use scratch handle graph*/
				unTrash(GGRAPH);
			}

			setGRAPH(lastplot, graph);
			setNDIMS(lastplot, 1);
			setDIM(lastplot,1,1);
			emptyTrash();
		} /*if (showPlot)*/
		else
		{
			/*
			   graphsymbol is a new symbol with GRAPH(graphsymbol) = graph.
			*/
			Removesymbol(lastplot); /* make sure LASTPLOT is removed */
			Cutsymbol(graphsymbol);
			setNAME(graphsymbol,LastGraphName);
			Addsymbol(graphsymbol);
		} /*if (showPlot){}else{}*/

		if (keyValues.notes != (Symbolhandle) 0)
		{
			Symbolhandle    theGraph = (showPlot) ? lastplot : graphsymbol;
			
			if (setNotes(theGraph, STRING(keyValues.notes)))
			{
				setSTRING(keyValues.notes, (char **) 0);
			}
			else
			{
				sprintf(OUTSTR, "WARNING: notes not set in %s()", FUNCNAME);
				putErrorOUTSTR();
			}
		} /*if (keyValues.notes != (Symbolhandle) 0)*/

		graphsymbol = (Symbolhandle) 0;
	} /*if (keyValues.keep)*/

	if (keyValues.show && !do_whole_graph(graph, plotfile,
										  termType, windno, &keyValues))
	{
		goto errorExit;
	}

	if (plotfile != STDOUT)
	{
#ifdef MACINTOSH
		macSetInfo(PlotVolume, keyValues.file,
				   (keyValues.epsf) ? 'EPSF' : ((!keyValues.dumb && !keyValues.ps) ? 'PICT' : 'TEXT'),
				   CREATOR);
#endif /*MACINTOSH*/
		fclose(plotfile);
	} /*if (plotfile != STDOUT)*/

	if (!showPlot && !keyValues.keep)
	{
		Removesymbol(graphsymbol);
		graphsymbol = (Symbolhandle) 0;
	}
	
	if (xisIndex == 2)
	{
		Removesymbol(symhx);
	}

	*OUTSTR = '\0';

#ifdef MACINTOSH
	UNLOADSEG(do_whole_graph);
#endif /*MACINTOSH*/
	return (NULLSYMBOL);

  errorExit:
	putErrorOUTSTR();

	if (plotfile != STDOUT)
	{
		fclose(plotfile);
	}

	if (!showPlot)
	{
		Removesymbol(graphsymbol);
	}
	else
	{
		emptyTrash();
	}
	
	if (xisIndex == 2)
	{
		Removesymbol(symhx);
	}

#ifdef MACINTOSH
	UNLOADSEG(do_whole_graph);
#endif /*MACINTOSH*/
	return (0);
} /*myplot()*/

