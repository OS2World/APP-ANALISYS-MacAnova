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


#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment Graphics
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


/*
 *
 *    G N U P L O T  --  graphics.c
 *
 *  Copyright (C) 1986, 1987  Thomas Williams, Colin Kelley
 *
 *  You may use this code as you wish if credit is given and this message
 *  is retained.
 *
 *  Please e-mail any useful additions to vu-vlsi!plot so they may be
 *  included in later releases.
 *
 *  This file should be edited with 4-column tabs!  (:set ts=4 sw=4 in vi)
 */

#include <math.h>
#undef  GRAPHICSC__
#define GRAPHICSC__ /* this should be defined only here */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "plot.h"
#include "dbug.h"

void myprint(char * /*msg*/); /*used by debugging macros*/

extern long GUBED;
/*BOOLEAN log_x = 0, log_y = 0; *//* set to false */
long            log_x = 0, log_y = 0;

#ifdef UNDEFINED__ /* following is now defined in plot.h */
extern long     term;
#endif /*UNDEFINED__*/

/*extern BOOLEAN term_init;*/
extern long     term_init;

extern struct termentry term_tbl[];

#ifdef UNDEFINED__
#define map_x(x) (long)((x-xmin)*xscale+xoffset)	/* maps floating point x to screen */
#define map_y(y) (long)((y-ymin)*yscale+yoffset)	/* same for y */
#endif /*UNDEFINED__*/

#define map_x(x) mapXToScreen(x) /* defined in plot.h */
#define map_y(x) mapYToScreen(x)

static double   raise(double x, long y)
{
	long            i;
	double          val;

	val = 1.0;
	for (i = 0; i < abs((int) y); i++)
	{
		val *= x;
	}
	if (y < 0)
	{
		return (1.0 / val);
	}
	return (val);
} /*raise()*/


static double          make_tics(double tmin, double tmax, long logscale)
{
	double          xr, xnorm, tics, tic, l10;

	xr = fabs(tmin - tmax);

	l10 = log10(xr);
	if (logscale)
	{
		tic = raise(10.0, (l10 >= 0.0) ? (long) l10 : (long) floor(l10));
		if (tic < 1.0)
		{
			tic = 1.0;
		}
	}
	else
	{
		xnorm = pow(10.0, l10 -
					(double) ((l10 >= 0.0) ? (long) l10 : (long) floor(l10)));

		if (xnorm <= 2)
		{
			tics = 0.2;
		}
		else if (xnorm <= 5)
		{
			tics = 0.5;
		}
		else
		{
			tics = 1.0;
		}
		tic = tics * raise(10.0,
						   (l10 >= 0.0) ? (long) l10 : (long) floor(l10));
#ifdef UNDEFINED__
/* fossil code */
		tmin = tic * floor(tmin/tic);
		tmax = tic * ceil(tmax/tic);
		count = (tmax-tmin)/tic;
		if( count > 6 && tics == 0.2 )
		{
			tics = 0.5;
			tic = tics * raise(10.0,(l10 >= 0.0 ) ? (long)l10 : (long)floor(l10));
		}
		else if (count > 6 && tics == 0.5)
		{
			tics = 1.0;
			tic = tics * raise(10.0,(l10 >= 0.0 ) ? (long)l10 : (long)floor(l10));
		}
#endif /*UNDEFINED__*/
	}
	return (tic);
} /*make_tics()*/

static char           *idx(char *a, char b)
{
	do
	{
		if (*a == b)
		{
			return (a);
		}
	} while (*a++);
	return (0);
} /*idx()*/


static void num2str(double num, char str[])
{
	static char     temp[80];
	double          d;
	char           *a, *b;

	if ((d = fabs(num)) > 9999.0 || d < 0.001 && d > 1.e-10)
	{
		(void) sprintf(temp, "%-.3e", num);
	}
	else
	{
		(void) sprintf(temp, "%-.3f", num);
	}
	if ((b = idx(temp, 'e')))
	{
		a = b;
		while (*(--a) == '0')	/* trailing zeros */
		{
			;
		}
		if (*a == '.')
		{
			a--;
		}
		(void) strncpy(str, temp, (size_t) (a - temp) + 1);
		str[(long) (a - temp) + 1] = '\0';
		a = b + 1;	/* point to 1 after 'e' */
		(void) strcat(str, "e");
		if (*a == '-')
		{
			(void) strcat(str, "-");
		}
		a++;		/* advance a past '+' or '-' */
		while (*a == '0' && *(a + 1) != '\0')	/* leading zeroes */
		{
			a++;
		}
		(void) strcat(str, a);	/* copy rest of string */
	} /*if ((b = idx(temp, 'e')))*/
	else
	{
		(void) strcpy(str, temp);
		a = str + strlen(str) - 1;
		while (*a == '0')
		{
			*a = 0;
			a--;
		}
		if (*a == '.')
		{
			*a = 0;
		}
	} /*if ((b = idx(temp, 'e'))){}else{}*/
	if (strcmp(str, "-0") == 0)
	{
		str[0] = '0';
		str[1] = '\0';
	}
} /*num2str()*/

#define trimx(X) ((Tmp__ = ((X) >= Ixmin) ? (X) : Ixmin) , \
				  ((Tmp__ <= Ixmax) ? Tmp__ : Ixmax))
#define trimy(Y) ((Tmp__ = ((Y) >= Iymin) ? (Y) : Iymin) , \
				  ((Tmp__ <= Iymax) ? Tmp__ : Iymax))

static long Ixmin, Ixmax, Iymin, Iymax, Tmp__;

static void findEdge(long xin,long yin,long *xout,long *yout)
{
	long      xEdge, yEdge, dx, dy;
	double    slope;

	dx = *xout - xin;
	dy = *yout - yin;
	xEdge = trimx(*xout);
	yEdge = trimy(*yout);
	if(dx != 0 && dy != 0)
	{
		slope = (double) dy / (double) dx;
		xEdge = (dx > 0) ? Ixmax : Ixmin;
		yEdge = yin + slope * (xEdge - xin);
		if(trimy(yEdge) != yEdge)
		{
			yEdge = (dy > 0) ? Iymax : Iymin;
			xEdge = xin + (yEdge - yin)/slope;
		}
	}
	*xout = xEdge;
	*yout = yEdge;
} /*findEdge()*/

/* 
  findEdges() is called to find the visible portion if any of the line
  connecting (*x1,*y1) and (*x2,*y2).  If the line does not intersect the
  frame, findEdges() returns 0 without modifying its arguments.  Otherwise it
  returns 1 and the points are modified so as to lie on the boundary of the
  frame.
 */

static long findEdges(long *x1,long *y1,long *x2,long *y2)
{
	long      xEdge1, xEdge2, yEdge1, yEdge2, dx, dy;
	long      trialx, trialy;
	double    slope;

	xEdge1 = trimx(*x1);
	yEdge1 = trimy(*y1);
	xEdge2 = trimx(*x2);
	yEdge2 = trimy(*y2);

	if((dx = *x2 - *x1) == 0)
	{  /* vertical line */
		if(*x1 != xEdge1 || yEdge1 == yEdge2)
		{
			return (0);
		}
		*y1 = yEdge1;
		*y2 = yEdge2;
	}
	else if((dy = *y2 - *y1) == 0)
	{ /* horizontal line */
		if(*y1 != yEdge1 || xEdge1 == xEdge2)
		{
			return (0);
		}
		*x1 = xEdge1;
		*x2 = xEdge2;
	}
	else
	{ /* line not horizontal or vertical */
		if(xEdge1 == xEdge2 || yEdge1 == yEdge2)
		{ /* both points on same side of frame */
			return (0);
		}
		slope = (double) dy / (double) dx;
		trialx = xEdge1;
		trialy = *y1 + slope * (trialx - *x1);
		if(trialy != trimy(trialy))
		{
			trialy = yEdge1;
			trialx = *x1 + (trialy - *y1) / slope;
			if(trialx != trimx(trialx))
			{
				return (0);
			}
		}
		
		*x1 = trialx;
		*y1 = trialy;
		findEdge(trialx,trialy,x2,y2);
	}
	return (1);
} /*findEdges()*/

enum tickCodes
{
	XTICK,
	YTICK
};

static int puttick(int which, double ticplace, int ticklength)
{
	struct termentry    *t = &term_tbl[term];
	int                  offset = 1;
	char                 label[80];
	
	num2str(ticplace, label);
	if (which == YTICK)
	{
		if (ticklength != 0)
		{
			(*t->move) (Ixmin, map_y(ticplace));
			if (ticklength != GRIDLINES)
			{				
				ticklength = (ticklength * (long) t->h_tic)/TICKUNIT;
				(*t->vector) (Ixmin + ticklength, map_y(ticplace));
				(*t->move) (Ixmax, map_y(ticplace));
				(*t->vector) (Ixmax - ticklength, map_y(ticplace));
			} /*if (ticklength != GRIDLINES)*/
			else
			{
				ticklength = 0;
				(*t->vector) (Ixmax, map_y(ticplace));
			}
		} /*if (ticklength != 0)*/
		offset = (ticklength >= 0) ? 1 : -ticklength + 1;
		if(t->h_char > 1)
		{
			horstring(Ixmin - (strlen(label)*(long) t->h_char) / 2 - offset,
					  map_y(ticplace), label, CENTRE);
		}
		else
		{ /*dumb plot*/
			horstring(Ixmin - (strlen(label)*(long) t->h_char - 1) / 2 - offset,
					  map_y(ticplace), label, CENTRE);
		}
		offset--;
	} /*if (which == YTICK)*/
	else
	{
		if (ticklength != 0)
		{
			(*t->move) (map_x(ticplace), Iymin);
			if (ticklength != GRIDLINES)
			{
				ticklength = (ticklength * (long) t->v_tic)/TICKUNIT;
				(*t->vector) (map_x(ticplace), Iymin + ticklength);
				(*t->move) (map_x(ticplace), Iymax);
				(*t->vector) (map_x(ticplace), Iymax - ticklength);
			} /*if (ticklength != GRIDLINES)*/
			else
			{
				(*t->vector) (map_x(ticplace), Iymax);
				ticklength = 0;
			}
		} /*if (ticklength != 0)*/
		offset = (ticklength >= 0) ? 0 : -ticklength;
		
		if(t->v_char > 1)
		{
			horstring(map_x(ticplace),
					  Iymin - t->v_char/2 - offset, label, CENTRE);
		}
		else
		{
			horstring(map_x(ticplace), (long) 1, label, CENTRE);
		}
	} /*if (which == YTICK){}else{}*/
	return (offset);
} /*puttick()*/

/*
	plots        linked list of plots
	pcount       count of plots in linked list
	strings      linked list of strings
	scount       count of strings in linked list
	xmin, xmax, ymin, ymax
	             actual plotting limits to use, may exclude data
    xticks, yticks
                 if non-null, values for ticks. 
	nxticks, nyticks
	             n{x,y}ticks == 0 means no {x,y} ticks
                 n{x,y}ticks > 0 means labelled ticks
				 When {x,y}ticks non null, n{x,y}ticks is number of
                 ticks
	xticklength, yticklength
                 Lengths of drawn ticks will be
				 ([xy]ticklength/TICKUNIT)*[hv]_tic with positive lengths
				 extending inside frame and negative lengths outside.
                 [xy]ticklength == GRIDLINES means draw entire grid line.
	logx, logy   determine whether variable should be plotted using log
                 scale (still ignored as of 960620)
    xaxis, yaxis determine whether axes are drawn if in the plot
    xlab, ylab   pointers to axis labels (used to be in struct curve_points)
    title        pointer to plot title

	960617 Added arguments {x,y}ticks, n{x,y}ticks {x,y}ticklength
	to allow greater control over tick drawing
*/
long do_plot(curve_points STAR_ plots, long pcount,
			 plot_string STAR_ strings, long scount,
			 double xmin, double xmax, double ymin, double ymax,
			 double * xticks, double * yticks, short nxticks, short nyticks,
			 short xticklength, short yticklength,
			 short logx, short logy, short xaxis, short yaxis,
			 char * xlab, char * ylab, char * title)
{
	long                 i, n, x, y, xt, yt;
	long                 xlast, ylast;
	struct termentry *t = &term_tbl[term];
	/*BOOLEAN prev_undef;*/
	long                 prev_undef;
	long                 curve, string, xaxis_y, yaxis_x;
	int                  style;
#ifdef USEHANDLES
	curve_points        **this_plot;
	plot_string         **this_string;
#else /*USEHANDLES*/
	curve_points *this_plot;
	plot_string         *this_string;
#endif /*USEHANDLES*/
	double               xscale, yscale, xoffset, yoffset;
	double               ytic, xtic, ticplace, ticplace1;
	int                  xlaboffset = 0, ylaboffset = 0;
	unsigned long        c;
	WHERE("do_plot");

/*
	for MAC & conceivably for other terminals this initializes t->xmax,
	t->xmin, etc.  Hence it must precede their use.
*/
	if((*t->init) () > 0)
	{
		return (1);
	}
	
	if ((*t->graphics) () > 0)
	{
		return (1);
	}

	GraphMap.logx = log_x = logx;
	GraphMap.logy = log_y = logy;
	GraphMap.xmin = xmin;
	GraphMap.ymin = ymin;
	GraphMap.screenxmax = t->xmax;
	GraphMap.screenymax = t->ymax;

	if(t->h_char > 1)
	{
		GraphMap.xoffset = xoffset = (t->xmax - 2) * 0.15;		
		GraphMap.xscale = xscale = (t->xmax - 2) / (xmax - xmin) * 0.85;
		Ixmin = map_x(xmin);
		Ixmax = map_x(xmax);
	}
	else
	{
		GraphMap.xoffset = xoffset = 11;
		GraphMap.xscale = xscale = (t->xmax - 1 - xoffset)/(xmax - xmin);
		Ixmin = xoffset;
		Ixmax = t->xmax - 1;
	}
	if(t->v_char > 1)
	{
		GraphMap.yoffset = yoffset = (t->ymax - 2*t->v_char - 2) * 0.1;
		GraphMap.yscale = yscale = (t->ymax - 2*t->v_char - 2) / (ymax - ymin) * 0.9;
		Iymin = map_y(ymin);
		Iymax = map_y(ymax);
	}
	else
	{
		GraphMap.yoffset = yoffset = 2;
		GraphMap.yscale = yscale = (t->ymax - 2 - yoffset)/(ymax - ymin);
		Iymin = yoffset;
		Iymax = t->ymax - 2;
	}

	(*t->linetype) ((long) BORDERLINETYPE);	/* border linetype */

	/* draw plot border */
	(*t->move) (Ixmin, Iymin);
	(*t->vector) (Ixmax, Iymin);
	(*t->vector) (Ixmax, Iymax);
	(*t->vector) (Ixmin, Iymax);
	(*t->vector) (Ixmin, Iymin);

	if (nyticks != 0)
	{
		nyticks = labs(nyticks);
		if (yticks == (double *) 0)
		{
			ytic = make_tics(ymin, ymax, log_y);

			ticplace = ytic * floor(ymin / ytic);

			/* Draw and label y-axis ticks*/
			while(ticplace <= ymax)
			{
				if (ticplace >= ymin)
				{
					ylaboffset = puttick(YTICK, ticplace, yticklength);
				}
				ticplace1 = ticplace + ytic;
				if(ticplace == ticplace1)
				{				/* make sure we don't get stuck*/
					break;
				}
				ticplace = ticplace1;
			} /*while(ticplace <= ymax)*/
		} /*if (yticks == (double *) 0)*/
		else
		{
			for(i = 0; i < nyticks;i++)
			{
				if (yticks[i] >= ymin && yticks[i] <= ymax)
				{
					ylaboffset = puttick(YTICK, yticks[i], yticklength);
				}
			} /*for(i = 0; i < nyticks;i++)*/
		} /*if (yticks == (double *) 0){}else{}*/
	} /*if (nyticks != 0)*/
	
	if (nxticks != 0)
	{
		if (xticks == (double *) 0)
		{
			xtic = make_tics(xmin, xmax, log_x);
			ticplace = xtic * floor(xmin / xtic);
			/* Draw and label x-axis ticks*/
			while(ticplace <= xmax)
			{
				if (ticplace >= xmin)
				{
					xlaboffset = puttick(XTICK, ticplace, xticklength);
				} /*if (ticplace >= xmin)*/
			
				ticplace1 = ticplace + xtic;
				if(ticplace == ticplace1)
				{/* make sure we don't get stuck */
					break;
				}
				ticplace = ticplace1;
			} /* while(ticplace <= xmax)*/
		} /*if (xticks == (double *) 0)*/
		else
		{
			for (i = 0; i < nxticks; i++)
			{
				if (xticks[i] >= xmin && xticks[i] <= xmax)
				{
					xlaboffset = puttick(XTICK, xticks[i], xticklength);
				}
			} /*for (i = 0; i < nxticks; i++)*/
		} /*if (xticks == (double *) 0){}else{}*/
	} /*if (nxticks != 0)*/

	/* DRAW AXES */
	xaxis_y = map_y(0.0);
	yaxis_x = map_x(0.0);

	if (logy || xaxis_y <= Iymin)
	{
		xaxis_y = Iymin;	/* save for impulse plotting */
	}
	else if (xaxis_y >= Iymax)
	{
		xaxis_y = Iymax;
	}
	else if (xaxis)
	{
		(*t->linetype) ((long) XAXISLINETYPE);	/* x-axis line type */
		(*t->move) (Ixmin, xaxis_y);
		(*t->vector) (Ixmax, xaxis_y);
	}

	if (yaxis && !log_x && yaxis_x > Ixmin && yaxis_x < Ixmax)
	{
		(*t->linetype) ((long) YAXISLINETYPE);	/* y-axis line type */
		(*t->move) (yaxis_x, Iymin);
		(*t->vector) (yaxis_x, Iymax);
	}

	this_plot = plots;
	/* DRAW TITLE (kb) */

	if (*title != '\0')
	{
		horstring(map_x((xmax + xmin)/2.),Iymax + t->v_char + xlaboffset,
				  title, CENTRE);
	}

	if (*xlab)
	{
		if(t->v_char > 1)
		{
			horstring(map_x((xmax + xmin) / 2.),
					  Iymin - 3*t->v_char/2 - xlaboffset,xlab, CENTRE);
		}
		else
		{
			horstring(map_x((xmax + xmin) / 2.), (long) 0, xlab, CENTRE);	
		}
	} /*if (*xlab)*/
	

	if (*ylab)
	{
		ticplace = ymax + ymin;
		if(t->h_char > 1)
		{
			vertstring((long) (0.2 * xoffset) - ylaboffset,
					   map_y(ticplace / 2.), ylab);
		}
		else
		{		
			/* Try to ensure rounding is consistant */
			if (ticplace == floor(ticplace) && floor(ticplace/2.0) != ticplace/2.0)
			{					/*ticplace is an odd integer*/
				ticplace += 1e-6*yscale;
			}
			vertstring(1,map_y(ticplace / 2.), ylab);
		}
	} /*if (*ylab)*/
	

	/* DRAW CURVES */

	for (curve = 0; curve < pcount; this_plot = NEXTGRAPH(this_plot), curve++)
	{
		n = PCOUNT(this_plot);
		style = PLOTSTYLE(this_plot);
		/* draw lines first */
		if(style & LINES)
		{
			(*t->linetype) (LINETYPE(this_plot));

			prev_undef = 1;
			for (i = 0; i < n; i++)
			{
				if (!POINTUNDEF(this_plot,i))
				{
					x = map_x(XCOORD(this_plot,i));
					y = map_y(YCOORD(this_plot,i));

					xt = trimx(x);
					yt = trimy(y);
					if(x == xt && y == yt)
					{ /* new point inside rectangle */
						if (prev_undef == 1)
						{
							(*t->move) (x,y);
						}
						else
						{
							if(prev_undef == 2)
							{	/* last point outside rectangle */
								findEdge(x,y,&xlast, &ylast);
							}
							(*t->move) (xlast,ylast);
							(*t->vector) (x,y);
						}
						prev_undef = 0;
					} /* if(x == xt && y == yt) */
					else
					{ /* current point outside rectangle */
						xt = x;
						yt = y;
						if(prev_undef == 0)
						{ /* last point in rectangle */
							findEdge(xlast,ylast,&xt,&yt);
							(*t->vector) (xt,yt);
						}
						else if(prev_undef == 2 &&
								findEdges(&xlast,&ylast,&xt, &yt))
						{ /* last point outside && edge/line intersection */
							(*t->move) (xlast,ylast);
							(*t->vector) (xt,yt);
						}
						prev_undef = 2;
					}  /* if(x == xt && y == yt){}else{} */
					xlast = x;
					ylast = y;
				} /* if (!POINTUNDEF(this_plot,i)) */
				else
				{
					prev_undef = 1;
				}
			} /*for (i = 0; i < n; i++)*/
		} /*if(style & LINES)*/
		
		if(style & (POINTS | IMPULSES | CHARS))
		{
			for(i = 0; i < n; i++)
			{
				if (!POINTUNDEF(this_plot,i))
				{
					x = map_x(XCOORD(this_plot,i));
					y = map_y(YCOORD(this_plot,i));
					xt = trimx(x);
					yt = trimy(y);

					if(style & IMPULSES && x == xt)
					{
						(*t->linetype) (LINETYPE(this_plot));
						(*t->move) (x, xaxis_y);
						(*t->vector) (x, yt);
					}

					if((style & (POINTS | CHARS)) && x == xt && y == yt)
					{ /* new point inside rectangle */
						(*t->move) (x,y);
						if(style & CHARS)
						{
							if((c = POINTSTRING(this_plot,i)[0]) != '\0' &&
							   c < ' ')
							{	/* drawn character */
								(*t->linetype) ((long) POINTLINETYPE);
								(*t->point) (x, y, c-1);
							}
							else
							{	/* printable character(s) */
								horstring(x, y, POINTSTRING(this_plot,i),
										  CENTRE);
							}
						} /*if(style & CHARS)*/

						if(style & POINTS)
						{
							(*t->linetype) ((long) POINTLINETYPE);
							(*t->point) (x, y, POINTTYPE(this_plot));
						}
					} /*if((style & (POINTS | CHARS)) && x == xt && y == yt)*/
				} /*if (!POINTUNDEF(this_plot,i))*/
			} /*for(i = 0; i < n; i++)*/
		} /*if(style & (POINTS | IMPULSES | CHARS))*/
	}/*for(curve=0;curve<pcount;this_plot=NEXTGRAPH(this_plot),curve++)*/

	/* DRAW STRINGS */
	this_string = strings;
	for (string = 0; string < scount; string++)
	{ /* Note: (x,y) may be outside frame */
		x = map_x(STRINGX(this_string));
		y = map_y(STRINGY(this_string));
#ifdef USEHANDLES
		horstring(x, y, *PLOTSTRING(this_string), (enum JUSTIFY) JUSTIFYSTR(this_string));
#else /*USEHANDLES*/
		horstring(x, y, PLOTSTRING(this_string), (enum JUSTIFY) JUSTIFYSTR(this_string));
#endif /*USEHANDLES*/
		this_string = NEXTSTRING(this_string);
	} /*for (string = 0; string < scount; string++)*/
	
	(*t->linetype) ((long) BORDERLINETYPE);
	(*t->text) ();

	return (0);
} /*do_plot()*/

