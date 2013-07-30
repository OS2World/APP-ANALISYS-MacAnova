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
#pragma segment Stemleaf
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#ifdef MPW
#include <Memory.h>
#endif /*MPW*/

#include "globals.h"
#include "plot.h"
#ifdef MACINTOSH
#include "macIface.h"
#endif /*MACINTOSH*/

#ifdef WXWIN
#include "wx/wxIface.h"
#endif  /* WXWIN */

#define MAXBOX 40

/*
	"standard" numbers of boxes; used to determine width or height for
	small number of boxes.	
*/
#define HORSTANDARD    4
#define VERTSTANDARD   5

extern char    *LastGraphName; /* global defined in myplot.c */

#define INNERPOINT     STARPOINT  /* star */
#define OUTERPOINT     DIAMONDPOINT  /* diamond */

#if (0)
#define INNERCHAR     '*'
#define OUTERCHAR     'o' /* for dumb terminal */
#else /*0*/
#define INNERCHAR     (INNERPOINT + 1)
#define OUTERCHAR     (OUTERPOINT + 1)
#endif /*0*/

enum boxPlotScratch
{
	GYH = 0,
	NTRASH
};

/*
   Make box plots of data

   Uses function boxprep(y, n, fivenum, nout)
   where y[] is a vector of n not missing values
   fivenum[0] is set to smallest value >= than lower innter fence
   fivenum[1] is set to Q1 (lower quartile)
   fivenum[2] is set to Q2 (median)
   fivenum[3] is set to Q3 (upper quartile)
   fivenum[4] is set to smallest value <= than upper innter fence
   lower inner and outer fences are Q1 - 1.5*IQR and Q1 - 3.0*IQR, respectively
   upper inner and outer fences are Q1 + 1.5*IQR and Q1 + 3.0*IQR, respectively

   nout[0] = # of cases < lower outer fence
   nout[1] = # of case >= lower outer fence and < lower inner fence
   nout[2] = # of case <= upper outer fence and > upper inner fence
   nout[3] = # of cases > upper outer fence

   Version 951206
	 Tidied up
	 box is now drawn as 4 connected line segments
	 De-referenced handles
   960705 moved boxprep() here from boxprep.c
   970105 added support for keyword height and width

   970307 moved preparation of the GRAPH symbol to new function buildBoxplot()

   971215 changed call to getPlotKeys() and modified code accordingly
   980724 Implemented notes:charVector
   980824 When only one box is drawn and the variable is not a scratch
          variable, its name is used instead of "Values" as an axis label
   981007 Minor changes to error messages
   981206 Changed default sizes for boxes when nbox is small, to avoid big
          fat boxes.  For horizontal boxes with nbox < 4, their heights are
          the same as the standard with nbox = 4, while for vertical boxes
          with nbox < 5 the widths are the same as the standard with nbox = 5.
          For horizontal boxes, including either ymin:? or ymax:? gives
          previous behavior, while for vertical boxes, xmin:? or xmax:?
          gives previous behavior.
   990212 Replaced putOUTSTR() by putErrorOUTSTR()
*/
#define TICKLENGTHNOTSET (4*TICKUNIT)

/*
  Function to do preparatory computation for boxplots and stem and leaf
  displays.  Originally in boxprep.c
  On output:
    array xsc has been sorted
    fivenum[0] = adjlo   nout[0] = number < lower outer fence
    fivenum[1] = Q1      nout[1] = number < lower inner fence, >= outer fence
    fivenum[2] = median  nout[2] = number > upper inner fence, <= outer fence
    fivenum[3] = Q3      nout[3] = number > upper outer fence
    fivenum[4] = adjhi

    where adjlo = whisker end = smallest number >= lower inner fence 
          adjhi = whisker end = largest number <= upper inner fence
*/

void boxprep(double * xsc, long n, double fivenum [], long nout [])
{
	long            i, j, k;
	double          adjlo, adjhi, lf1, lf2, uf1, uf2;
	double          iqr, q1,q2,q3;

	sortquick(xsc, n);

	i = (n + 1) / 2;
	j = (n / 2) + 1;
	q2 = (xsc[i - 1] + xsc[j - 1]) / 2.;
	j = (i + 1) / 2;
	k = (i / 2) + 1;
	q1 = (xsc[j - 1] + xsc[k - 1]) / 2.;
	q3 = (xsc[n - j] + xsc[n - k]) / 2.;

	iqr = q3 - q1;

	/* compute fences*/
	lf1 = q1 - 1.5 * iqr;
	lf2 = q1 - 3.0 * iqr;
	uf1 = q3 + 1.5 * iqr;
	uf2 = q3 + 3.0 * iqr;

	nout[0] = 0; /* number < lower outer fence */
	nout[1] = 0; /* number >= lower outer fence  && < lower inner fence*/
	nout[2] = 0; /* number > upper outer fence */
	nout[3] = 0; /* number <= upper outer fence  && > upper inner fence*/

	i = 0;
	while (xsc[i] < lf2)
	{
		i++;
		nout[0]++;
	}
	while (xsc[i] < lf1)
	{
		i++;
		nout[1]++;
	}
	adjlo = xsc[i]; /* smallest >= lower inner fence */

	i = n - 1;
	while (xsc[i] > uf2)
	{
		i--;
		nout[3]++;
	}
	while (xsc[i] > uf1)
	{
		i--;
		nout[2]++;
	}
	adjhi = xsc[i]; /* largest <= upper inner fence */

	fivenum[0] = adjlo;
	fivenum[1] = q1;
	fivenum[2] = q2;
	fivenum[3] = q3;
	fivenum[4] = adjhi;

} /*boxprep()*/

#define OFFSETFROMTICK   0.0  /* was 0.05 */

#undef SWAP
#define SWAP(X,Y,T) (T = X, X = Y, Y = T)

static void set_box_point(curve_points ** this_plot, long pointNo,
						  double x, double y, char * pointLab,
						  unsigned char colors [], long vertical)
{
	if (vertical)
	{
		double         tmp;

		SWAP(x, y, tmp);
	} /*if (vertical)*/
	set_point(this_plot, pointNo, x, y, pointLab, colors[0], colors[1]);
} /*set_box_point()*/

/*
	980826 moved lines in following from the call to setGraphStuff() because
	CodeWarrior crashed on the GRAPHLOGY line
*/
static void setGraphStuff(whole_graph  ** graph, plotKeyValuesPtr keyValues)
{
	strcpy(GRAPHTITLE(graph), keyValues->title);
	strcpy(GRAPHXLAB(graph), keyValues->xlab);
	strcpy(GRAPHYLAB(graph), keyValues->ylab);
	GRAPHXMIN(graph) = keyValues->xmin;
	GRAPHXMAX(graph) = keyValues->xmax;
	GRAPHYMIN(graph) = keyValues->ymin;
	GRAPHYMAX(graph) = keyValues->ymax;
	GRAPHLOGX(graph) = keyValues->logx;
	GRAPHLOGY(graph) = keyValues->logy;
	GRAPHXAXIS(graph) = keyValues->xaxis;
	GRAPHYAXIS(graph) = keyValues->yaxis;
}


static Symbolhandle buildBoxplot(Symbolhandle list, long firstArg, long nbox,
								 unsigned char colors [],
								 long vertical,
								 plotKeyValuesPtr keyValues)
{
	Symbolhandle   graphsymbol, symhy;
	whole_graph  **graph = (whole_graph **) 0;
	curve_points **this_plot = (curve_points **) 0;
	curve_points **last_plot = (curve_points **) 0;
	double       **xH, **yH;
	double        *x, *y;
	double         hugedbl = HUGEDBL;
	double       **defaultTicks = (double **) 0;
	double         defaultXmin = hugedbl, defaultXmax = -hugedbl;
	double         defaultYmin, defaultYmax;
	double         fivenum[5];	
	double         left, right, top, bottom, median, mid;
	long           tot, notmiss, nout[4], npoints;
	long           defaultNticks = (nbox < 20) ? nbox : (nbox + 1)/2;
	long           standardNbox = (vertical) ? VERTSTANDARD : HORSTANDARD;
	unsigned short defaultXlen = TICKUNIT, defaultYlen = 0;
	char           coordLabels[2][XLABELSIZE+2];
	char          *defaultXlab = coordLabels[0];
	char          *defaultYlab = coordLabels[1];
	char          *defaultTitle = "Box Plot";
	long           nxticks = -1, nyticks = -1;
	long           ibox, j, k;
	long           pointNo;
	char           innerPoint[2], outerPoint[2];
	WHERE("buildBoxplot");
	TRASH(NTRASH,errorExit);

	if (nbox == 1 && !isscratch(NAME(COMPVALUE(list, firstArg))))
	{
		char      *name = NAME(COMPVALUE(list, firstArg));
		
		strcpy(coordLabels[0], (istempname(name)) ? name + 1 : name);
	}
	else
	{
		strcpy(coordLabels[0], "Values");
	}
	
	strcpy(coordLabels[1], "Box Number");
	innerPoint[0] = (keyValues->dumb) ? INNERCHAR : INNERPOINT + 1;
	outerPoint[0] = (keyValues->dumb) ? OUTERCHAR : OUTERPOINT + 1;
	innerPoint[1] = outerPoint[1] = '\0';

	/*
	  Be very careful to keep the structure of graph up to date.
	  All handles are zeroed prior to attempting to get storage;
	  GRAPHNPLOTS(graph) is updated as soon a a new struct curve_points
	  is allocated, as is GRAPHFIRST_PLOT(graph) or NEXTGRAPH(last_plot);
	  NEXTGRAPH(this_plot) and GRAPHPOINTS(this_plot) are zeroed as soon
	  as this_plot is allocated.
	  This allows cleanup to be done by deleteGraph
	*/
	if ((graphsymbol = GraphInstall(SCRATCH)) == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	graph = GRAPH(graphsymbol);

	for (ibox = 0; ibox < nbox; ibox++)
	{
		symhy = COMPVALUE(list, firstArg + ibox);
		if (!isNull(symhy))
		{/* null symbol is not an error */
			tot = symbolSize(symhy);
			xH = DATA(symhy);
			if (!getScratch(yH, GYH, tot, double))
			{
				goto errorExit;
			}
			x = *xH;
			y = *yH;
			notmiss = 0;
			for (j = 0; j < tot; j++)
			{
				if (!isMissing(x[j]))
				{
					y[notmiss++] = x[j];
				}
			}
			if (notmiss < tot)
			{
				sprintf(OUTSTR, "WARNING: %ld missing values in box %ld",
						tot - notmiss, ibox+1);
				putErrorOUTSTR();
			}

			if (notmiss > 0)
			{
				boxprep(y, notmiss, fivenum, nout);
	
				if (y[0] < defaultXmin)
				{
					defaultXmin = y[0];
				} /*if (y[0] < defaultXmin)*/
				
				if (y[notmiss - 1] > defaultXmax)
				{
					defaultXmax = y[notmiss - 1];
				} /*if (y[notmiss - 1] > defaultXmax)*/

				mid = (double) ibox + 1 + OFFSETFROMTICK;
				bottom = mid - .33;
				top = mid + .33;
				left = fivenum[1];
				median = fivenum[2];
				right = fivenum[3];
			
				/* always draw whiskers */
				this_plot = (curve_points **)
					mygethandle(sizeof(curve_points));
				if (this_plot == (curve_points **) 0)
				{
					goto errorExit;
				}
				NEXTGRAPH(this_plot) = (curve_points **) 0;
				GRAPHPOINTS(this_plot) = (coordinate **) 0;
				GRAPHNPLOTS(graph)++;

				if (GRAPHFIRST_PLOT(graph) == (curve_points **) 0)
				{
					GRAPHFIRST_PLOT(graph) = this_plot;
				}
				else
				{
					NEXTGRAPH(last_plot) = this_plot;
				}
				mylockhandle((char **) this_plot);
				last_plot = this_plot;
			
				/* whiskers (2 segments) */
#define WHISKERSIZE   5 /* # pts to define whiskers, including undef pts*/

				TMPHANDLE = mygethandle(WHISKERSIZE * sizeof(coordinate));
				if (TMPHANDLE == (char **) 0)
				{
					goto errorExit;
				}
				GRAPHPOINTS(this_plot) = (coordinate **) TMPHANDLE;
				mylockhandle(TMPHANDLE); /* needed in Mac version */

				PLOTSTYLE(this_plot) = LINES;
				POINTTYPE(this_plot) = INNERPOINT; /* ignored */
				LINETYPE(this_plot) = BORDERLINETYPE;
			
				PCOUNT(this_plot) = WHISKERSIZE;
				/* left whisker */
				pointNo = 0;
				set_box_point(this_plot,pointNo++,fivenum[0],mid,(char *) 0,
							  colors, vertical);
				set_box_point(this_plot,pointNo++,left,mid,(char *) 0,
							  colors, vertical);
				undef_point(this_plot,pointNo++);

				/* right whisker */
				set_box_point(this_plot,pointNo++,right,mid,(char *) 0,
							  colors, vertical);
				set_box_point(this_plot,pointNo++,fivenum[4],mid,(char *) 0,
							  colors, vertical);

				/* draw box with median line (5 segments) */
				this_plot =
				  (curve_points **) mygethandle(sizeof(curve_points));
				if (this_plot == (curve_points **) 0)
				{
					goto errorExit;
				}
				NEXTGRAPH(this_plot) = (curve_points **) 0;
				GRAPHPOINTS(this_plot) = (coordinate **) 0;
				NEXTGRAPH(last_plot) = this_plot;
				GRAPHNPLOTS(graph)++;
				mylockhandle((char **) this_plot);
				last_plot = this_plot;

#define BOXSIZE       8 /* # pts to define box and median line, incld undef*/

				TMPHANDLE = mygethandle(BOXSIZE * sizeof(coordinate));
				if (TMPHANDLE == (char **) 0)
				{
					goto errorExit;
				}
	
				GRAPHPOINTS(this_plot) = (coordinate **) TMPHANDLE;
				mylockhandle(TMPHANDLE); /* needed in Mac version */

				PLOTSTYLE(this_plot) = LINES;
				POINTTYPE(this_plot) = INNERPOINT; /* ignored */
				LINETYPE(this_plot) = BORDERLINETYPE;

				PCOUNT(this_plot) = BOXSIZE;

				/* box frame */
				pointNo = 0;
				set_box_point(this_plot,pointNo++,left,top,(char *) 0,
							  colors, vertical);
				set_box_point(this_plot,pointNo++,right,top,(char *) 0,
							  colors, vertical);
				set_box_point(this_plot,pointNo++,right,bottom,(char *) 0,
							  colors, vertical);
				set_box_point(this_plot,pointNo++,left,bottom,(char *) 0,
							  colors, vertical);
				set_box_point(this_plot,pointNo++,left,top,(char *) 0,
							  colors, vertical);
				undef_point(this_plot,pointNo++);

				/* mid line */
				set_box_point(this_plot,pointNo++,median,bottom,(char *) 0,
						  colors, vertical);
				set_box_point(this_plot,pointNo++,median,top,(char *) 0,
						  colors, vertical);

				/* outside values */
				if ((npoints = nout[1] + nout[2]) > 0)
				{
					this_plot = (curve_points **)
						mygethandle(sizeof(curve_points));
					if (this_plot == (curve_points **) 0)
					{
						goto errorExit;
					}
					NEXTGRAPH(this_plot) = (curve_points **) 0;
					GRAPHPOINTS(this_plot) = (coordinate **) 0;
					NEXTGRAPH(last_plot) = this_plot;
					GRAPHNPLOTS(graph)++;
					mylockhandle((char **) this_plot);
					last_plot = this_plot;

					TMPHANDLE = mygethandle(npoints*sizeof(coordinate));
					if (TMPHANDLE == (char **) 0)
					{
						goto errorExit;
					}
					GRAPHPOINTS(this_plot) = (coordinate **) TMPHANDLE;
					mylockhandle(TMPHANDLE); /* needed in Mac version */

					PLOTSTYLE(this_plot) = CHARS;
					POINTTYPE(this_plot) = INNERPOINT; /* ignored */
					LINETYPE(this_plot) = BORDERLINETYPE;

					PCOUNT(this_plot) = npoints;

					pointNo = 0;
					/* low outside (between inner and outer fences) */
					k = nout[0];
					y = *yH;
					for (j = 0; j < nout[1]; j++)
					{
						set_box_point(this_plot,pointNo++,y[k++],mid,innerPoint,
								  colors, vertical);
					}
					/* hi out */
					k = notmiss - 1 - nout[3];
					for (j = 0; j < nout[2]; j++)
					{
						set_box_point(this_plot,pointNo++,y[k--],mid,innerPoint,
								  colors, vertical);
					}
				} /*if ((npoints = nout[1] + nout[2]) > 0)*/

				/* far outside values (beyond outer fences) */
				if ((npoints = nout[0] + nout[3]) > 0)
				{
					this_plot = (curve_points **)
						mygethandle(sizeof(curve_points));
					if (this_plot == (curve_points **) 0)
					{
						goto errorExit;
					}
					NEXTGRAPH(this_plot) = (curve_points **) 0;
					GRAPHPOINTS(this_plot) = (coordinate **) 0;
					NEXTGRAPH(last_plot) = this_plot;
					GRAPHNPLOTS(graph)++;
					mylockhandle((char **) this_plot);
					last_plot = this_plot;
					
					TMPHANDLE = mygethandle(npoints*sizeof(coordinate));
					if (TMPHANDLE == (char **) 0)
					{
						goto errorExit;
					}
		/* NOTE TMPHANDLE is indirectly disposed of by mydispnhandles */
					GRAPHPOINTS(this_plot) = (coordinate **) TMPHANDLE;
					mylockhandle(TMPHANDLE); /* needed in Mac version */
				
					PLOTSTYLE(this_plot) = CHARS;
					POINTTYPE(this_plot) = 0; /* ignored */
					LINETYPE(this_plot) = BORDERLINETYPE;

					PCOUNT(this_plot) = npoints;

					pointNo = 0;
					y = *yH;
					k = 0;
					for (j = 0; j < nout[0]; j++)
					{
						set_box_point(this_plot,pointNo++,y[k++],mid,outerPoint,
								  colors, vertical);
					}
					k = notmiss - 1;
					for (j = 0; j < nout[3]; j++)
					{
						set_box_point(this_plot,pointNo++,y[k--],mid,outerPoint,
								  colors, vertical);
					}
				} /*if ((npoints = nout[0] + nout[3]) > 0)*/
			} /*if (notmiss > 0)*/
			discardScratch(yH,0,double);
		} /* if (!isNull(symhy) */
	} /*for (ibox = 0; ibox < nbox; ibox++)*/

	if (GRAPHNPLOTS(graph) == 0)
	{
		sprintf(OUTSTR,"ERROR: no data!");
		goto errorExit;
	}

/*
	Limits are always set to include all boxes and
	all data although xmin, xmax, ymin and ymax may be
	used to set limits outside these extremes

	981206 Changed default size of boxes to avoid big "fat boxes"
	For vertical plots with nbox < 5, width of boxes is <= width for 5
	box case
	For horizontal plotsi with nbox < 4, height of boxes is <= width for
	4 box case
	This is overridden when the user sets ymin or ymax (horizontal boxes)
	or xmin or xmax (vertical boxes)
*/
#if (0) /*the following would treat horizontal dumb plots specially */
	if (keyValues->dumb && !vertical &&
		((keyValues->height != UNSETLONG) ? keyValues->height : SCREENHEIGHT) <= 25)
	{
		standardNbox--;
	}
#endif /*0*/

	if (nbox >= standardNbox ||
		vertical && (realIsSet(keyValues->xmin) || realIsSet(keyValues->xmax)) ||
		!vertical && (realIsSet(keyValues->ymin) || realIsSet(keyValues->ymax)))
	{
		defaultYmin = 0.5;
		defaultYmax = defaultYmin + (double) nbox;
	}
	else
	{
		defaultYmin = 0.5 - .5*standardNbox + .5*nbox;
		defaultYmax = defaultYmin + standardNbox;
	}

	keyValues->ymin =
	  (!realIsSet(keyValues->ymin) || isMissing(keyValues->ymin)) ?
	  hugedbl : keyValues->ymin;
	keyValues->ymax =
	  (!realIsSet(keyValues->ymax) || isMissing(keyValues->ymax)) ?
	  -hugedbl : keyValues->ymax;

	keyValues->xmin =
	  (!realIsSet(keyValues->xmin) || isMissing(keyValues->xmin)) ?
	  hugedbl : keyValues->ymin;
	keyValues->xmax =
	  (!realIsSet(keyValues->xmax) || isMissing(keyValues->xmax)) ?
	  -hugedbl : keyValues->xmax;

	if (vertical)
	{
		/* swap extremes, tick information, default labels */
		double       **tmpH, tmpD;
		unsigned short tmpUs;
		char          *tmpCp;
		
		SWAP(defaultXmin, defaultYmin, tmpD);
		SWAP(defaultXmax, defaultYmax, tmpD);

		SWAP(defaultXlen, defaultYlen, tmpUs);
		
		SWAP(defaultXlab, defaultYlab, tmpCp);

		SWAP(GRAPHXTICKS(graph), GRAPHYTICKS(graph), tmpH);
	} /*if (vertical)*/

	keyValues->xticklength = (keyValues->xticklength == TICKLENGTHNOTSET) ?
		defaultXlen : keyValues->xticklength;
	keyValues->yticklength = (keyValues->yticklength == TICKLENGTHNOTSET) ?
		defaultYlen : keyValues->yticklength;
	
	if (keyValues->xticks != UNSETSYMH)
	{
		nxticks = symbolSize(keyValues->xticks);
		GRAPHXTICKS(graph) = DATA(keyValues->xticks);
		setDATA(keyValues->xticks, (double **) 0);
	} /*if (keyValues->xticks != UNSETSYMH)*/

	if (keyValues->yticks != UNSETSYMH)
	{
		nyticks = symbolSize(keyValues->yticks);
		GRAPHYTICKS(graph) = DATA(keyValues->yticks);
		setDATA(keyValues->yticks, (double **) 0);
	} /*if (keyValues->yticks != UNSETSYMH)*/

	if (!vertical && keyValues->yticks == UNSETSYMH ||
		vertical && keyValues->xticks == UNSETSYMH)
	{
		TMPHANDLE = mygethandle(defaultNticks*sizeof(double));
		if (TMPHANDLE == (char **) 0)
		{
			goto errorExit;
		}
		defaultTicks = (double **) TMPHANDLE;
		if (vertical)
		{
			GRAPHXTICKS(graph) = defaultTicks;
			nxticks = defaultNticks;
		}
		else
		{
			GRAPHYTICKS(graph) = defaultTicks;
			nyticks = defaultNticks;
		}
		
		for (ibox = 0; ibox < nbox; ibox++)
		{
			k = (nbox < 20) ? ibox : ((ibox % 2 == 0) ? ibox / 2 : -1);
			if (k >= 0)
			{
				(*defaultTicks)[k] = (double) (ibox + 1);
			}
		} /*for (ibox = 0; ibox < nbox; ibox++)*/
	}
	
	GRAPHNXTICKS(graph) = nxticks;
	GRAPHNYTICKS(graph) = nyticks;
	GRAPHXTICKLENGTH(graph) = keyValues->xticklength;
	GRAPHYTICKLENGTH(graph) = keyValues->yticklength;

	if (keyValues->title[0] == '\0')
	{
		strcpy(keyValues->title, defaultTitle);
	}
	if (keyValues->xlab[0] == '\0')
	{
		strcpy(keyValues->xlab, defaultXlab);
	}
	if (keyValues->ylab[0] == '\0')
	{
		strcpy(keyValues->ylab, defaultYlab);
	}
	keyValues->xmin =
	  (keyValues->xmin < defaultXmin) ? keyValues->xmin : defaultXmin;
	keyValues->xmax =
	  (keyValues->xmax > defaultXmax) ? keyValues->xmax : defaultXmax;
	keyValues->ymin =
	  (keyValues->ymin < defaultYmin) ? keyValues->ymin : defaultYmin;
	keyValues->ymax =
	  (keyValues->ymax > defaultYmax) ? keyValues->ymax : defaultYmax;
	
	setGraphStuff(graph, keyValues);

	emptyTrash();
	return (graphsymbol);

  errorExit:
	putErrorOUTSTR();
	
	Removesymbol(graphsymbol);
	emptyTrash();
	return ((Symbolhandle) 0);
	
} /*buildBoxplot()*/

Symbolhandle    boxplot(Symbolhandle list)
{
	/* routine to do boxplot */

	Symbolhandle   symhy, symhKey, symhfile;
	Symbolhandle   graphsymbol = (Symbolhandle) 0;
	whole_graph  **graph = (whole_graph **) 0;
	plotKeyValues  keyValues;
	long           j, ibox, nbox;
	long           nargs = NARGS(list), firstArg = 0, first;
	long           isStruct = 0;	
	long           windno = FINDEMPTYWINDOW;
	long           vertical = 0;
	long           toFile = 0, termType = STANDARDTERM;
	FILE          *plotfile = (FILE *) 0;
	unsigned char  pointColor = DEFAULTLINECOLOR;
	unsigned char  lineColor = DEFAULTLINECOLOR;
	unsigned char  colors[2];
	char          *msgs;
	char           filePath[PATHSIZE+1];
#ifdef HASFINDFILE
	char          *tmpFileName;
#endif /*HASFINDFILE*/
#ifdef MACINTOSH
	OSType         fileType = 'TEXT';
#endif /*MACINTOSH*/
	WHERE("boxplot");
	
	unsetPlotKeys(&keyValues,1);
	keyValues.xticklength = TICKLENGTHNOTSET;
	keyValues.yticklength = TICKLENGTHNOTSET;
	keyValues.logx = keyValues.logy = 0;
	keyValues.xaxis = keyValues.yaxis = 0;
	keyValues.pause = DEFAULTPLOTPAUSE;
	keyValues.landscape = 0;
	OUTSTR[0] = '\0';

	if (FUNCNAME[0] == 'f')
	{
		toFile = 1;
		firstArg = 1;
		symhfile = COMPVALUE(list,0);
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
	} /*if (FUNCNAME[0] == 'f')*/
	
	if (nargs < firstArg + 1)
	{
		badNargs(FUNCNAME,-(firstArg + 1));
		goto errorExit;
	}

	for (j = firstArg + 1; j < nargs; j++)
	{
		if (isKeyword(COMPVALUE(list, j)))
		{
			break;
		}
	} /*for (j = firstArg + 1; j < nargs; j++)*/

	if (j < nargs)
	{
		keyValues.show = (!toFile) ? keyValues.show : DONTSETCHAR;
		keyValues.add = DONTSETCHAR;
		keyValues.impulses = keyValues.lines = DONTSETCHAR;
		keyValues.linetype = DONTSETLONG;
		keyValues.thickness = DONTSETREAL;
		
#ifndef EPSFPLOTSOK
		keyValues.epsf = DONTSETCHAR;
#endif /*EPSFPLOTSOK*/
		keyValues.justify = DONTSETCHAR;
#ifndef SCREENDUMP
		keyValues.screendump = DONTSETPOINTER;
#endif /*SCREENDUMP*/
		first = getPlotKeys(list, j, &keyValues);
		unsetPlotKeys(&keyValues, 0); /* change DONTSETs to UNSETs */

		/* File names are now (951113) expanded by getPlotKeys()*/
		if (first < 0)
		{
			goto errorExit;
		} /*if (first < 0)*/
		else
		{
			char     *keyword = "vertical";
			long      j1 = findKeyword(list, keyword, j);

			if (j1 >= 0)
			{
				symhKey = COMPVALUE(list, j1);
				if (!isTorF(symhKey))
				{
					notTorF(keyword);
					goto errorExit;
				}
				vertical = (DATAVALUE(symhKey, 0) != 0.0);
			} /*if (j1 >= 0)*/
			first = (first > 0) ? first : j + 1;
		} /*if (first < 0){}else{}*/
	} /*if (j < nargs)*/
	else
	{
		first = 0;
	}
	
	toFile = (keyValues.file != UNSETPOINTER);
	
	if (!checkPlotKeys(toFile, &keyValues, &termType))
	{
		goto errorExit;
	}

	if (toFile && keyValues.file != filePath)
	{
		strcpy(filePath, keyValues.file);
		keyValues.file = filePath;
	}

#ifdef SCREENDUMP
	if (keyValues.screendump != UNSETPOINTER)
	{
		if (termType != STANDARDTERM)
		{
			sprintf(OUTSTR,
					"ERROR: screendump:fileName can only be used with high resolution plot");
			goto errorExit;
		}
#ifdef MACINTOSH
		if (keyValues.screendump[0] != '\0' &&
			!isfilename(keyValues.screendump))
#else  /*MACINTOSH*/
		if (!isfilename(keyValues.screendump))
#endif /*MACINTOSH*/
		{
			goto errorExit;
		}
	} /*if (keyValues.screendump != UNSETPOINTER)*/
#endif /*SCREENDUMP*/

	windno = (keyValues.window != UNSETLONG) ?
	  keyValues.window : FINDEMPTYWINDOW;

	nbox = ((first == 0) ? nargs : first-1) - firstArg;
	symhy = COMPVALUE(list, firstArg);
	if (!argOK(symhy,(long) 0, firstArg+1))
	{
		goto errorExit;
	}
	
	if (nbox == 1 && TYPE(symhy) != STRUC && !isVector(symhy))
	{
		sprintf(OUTSTR,
				"ERROR: argument %ld to %s() is not vector or structure",
				firstArg + 1, FUNCNAME);
		goto errorExit;
	}
	if (nbox == 1 && TYPE(symhy) == STRUC)
	{
		isStruct = 1;
		msgs = "structure component";
		list = symhy;	/* if structure form, set list to structure */
		nbox = NCOMPS(list);
		firstArg = 0;
	} /*if (nbox == 1 && TYPE(symhy) == STRUC)*/
	else
	{
		msgs = "argument";
	}
	
	if (nbox > MAXBOX)
	{
		sprintf(OUTSTR,"ERROR: attempting to plot more than %ld boxes",
				(long) MAXBOX);
		goto errorExit;
	}

	for (ibox = 0; ibox < nbox; ibox++)
	{ /* check all args or components before doing anything */
		int    argno = firstArg + ibox + 1;

		symhy = COMPVALUE(list, firstArg+ibox);
		if (isNull(symhy))
		{
			if (!isStruct)
			{
				sprintf(OUTSTR,"ERROR: %d%s %s for %s is missing or null",
						argno, n_th(argno), msgs, FUNCNAME);
			}
		}
		else if (!isDefined(symhy))
		{
			sprintf(OUTSTR,"ERROR: %d%s %s (%s) for %s is undefined",
					argno, n_th(argno), msgs, NAME(symhy),FUNCNAME);
		}
		else if (TYPE(symhy) != REAL)
		{
			sprintf(OUTSTR,"ERROR: %d%s %s for %s is %s, not REAL",
					argno, n_th(argno), msgs, FUNCNAME,typeName(TYPE(symhy)));
		}
		else if (!isVector(symhy))
		{
			sprintf(OUTSTR,"ERROR: %d%s %s for %s is not vector",
					argno, n_th(argno), msgs, FUNCNAME);
		}
		if (*OUTSTR)
		{
			goto errorExit;
		}
	} /*for (ibox = 0; ibox < nbox; ibox++)*/
	
	if (toFile)
	{
		/* keyValues.file must be pointing to filePath */
#ifdef HASFINDFILE
		tmpFileName = findPlotFile(keyValues.file, "Boxplot",
								   keyValues.ps, keyValues.epsf,
								   keyValues.newFile);
		
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
		
		plotfile = openPlotFile(keyValues.file, keyValues.newFile);
		if (plotfile == (FILE *) 0)
		{
			goto errorExit;
		}
	} /*if (toFile)*/
	else
	{
		plotfile = STDOUT;
	}
	
	colors[0] = pointColor;
	colors[1] = lineColor;
	
	graphsymbol = buildBoxplot(list, firstArg, nbox, colors, vertical,
							   &keyValues);
	if (graphsymbol == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	graph = GRAPH(graphsymbol);
	if (keyValues.keep)
	{
		Remove(LastGraphName);
		Cutsymbol(graphsymbol);
		setNAME(graphsymbol, LastGraphName);
		Addsymbol(graphsymbol);
	} /*if (keyValues.keep)*/	

	if (keyValues.show &&
	   !do_whole_graph(graph, plotfile, termType, windno, &keyValues))
	{
		if (keyValues.keep)
		{
			graph = (whole_graph **) 0;
		}
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
	
	if (!keyValues.keep)
	{
		Removesymbol(graphsymbol);
		graphsymbol = (Symbolhandle) 0;
	}

	if (graphsymbol != (Symbolhandle) 0 && keyValues.notes != (Symbolhandle) 0)
	{
		if (setNotes(graphsymbol, STRING(keyValues.notes)))
		{
			setSTRING(keyValues.notes, (char **) 0);
		}
		else
		{
			sprintf(OUTSTR, "WARNING: notes not set in %s()", FUNCNAME);
			putErrorOUTSTR();
		}
	} /*if (keyValues.notes != (Symbolhandle) 0)*/
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
	Removesymbol(graphsymbol);
	
#ifdef MACINTOSH
	UNLOADSEG(do_whole_graph);
#endif /*MACINTOSH*/
	return (0);
	
} /*boxplot()*/


