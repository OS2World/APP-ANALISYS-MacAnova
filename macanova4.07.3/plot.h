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
 *
 *    G N U P L O T  --  plot.h
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

/*
   960716 Changed types Long, Char, Double to long, char, double
   970625 Removed include of typedef.h and remaining references to
          type uLong, as well as defines needed to fake typedef on
          DEC ultrix
   980714 Put POINTS, LINES, ... in enum
          Modified structure plot_string and added macros STRINGCOLOR
          and STRINGSTYLE
          Changed prototype for horstring()
*/

#include "platform.h"  /*kb*/

#ifndef PLOTH__
#define PLOTH__

#include <math.h>

#define ZERO	1e-8		/* default for 'zero' set option */

/*
#define TRUE 1
#define FALSE 0
*/
/*
  Error codes that can be used by screen dump routine, if any
  980717 Moved here from plotutil.c (were defines)
*/

enum screenDumpErrorCodes
{
	SCREENDUMPUNDEFINED =  0,
	SCREENDUMPOK        =  1,
	BADSCREENMODE       = -1,
	BADCOLORS           = -2,
	BADWIDTH            = -3,
	CANTOPEN            = -4,
	TOOMANYDUMPS        = -5,
	FILEERROR           = -6
};

/*
 * note about HUGE:  this number is just used as a flag for really
 *   big numbers, so it doesn't have to be the absolutely biggest number
 *   on the machine.
 *
 * define HUGE here if your compiler doesn't define it in <math.h>
 * example:
 */
#ifndef HUGE
#define HUGE 1e38
#endif /*HUGE*/

#define USEHANDLES /* MacAnova assumes handles used*/

#ifdef USEHANDLES
#define STAR_ **
#else /*USEHANDLES*/
#define STAR_ *
#endif /*USEHANDLES*/

#define XLABELSIZE       50
#define YLABELSIZE       20
#define TITLESIZE        75
#define PLOTSTRINGSIZE    3
#define TICKUNIT         64 /* 1 standard tick length */
#define GRIDLINES       255 /* ticklength code for drawing grid lines */

#define MAXCHARITEMSIZE 200 /* max length of strings for addstrings() */

enum TerminalTypes
{
	UNKNOWNTERM = 0,
	STANDARDTERM,    /* default terminal type for platform */
	DUMBTERM,        /* dumb terminal output to screen or file */
#ifdef POSTSCRIPT
	PSTERM,          /* PostScript output to file */
#endif /*POSTSCRIPT*/
#ifdef EPSFPLOTSOK
	PREVIEWTERM,     /* Preview mode for EPSF output */
#endif /*EPSFPLOTSOK*/
#ifdef DUMBFILETERM
	FILETERM = DUMBTERM
#else
	FILETERM         /* non-dumb, non-PostScript output to file */
#endif
};
/*
	EPSFPLOTSOK should be defined in platform.h to the index in the
	term_tbl[] array defined in term.c that corresponds to creating
	a preview picture for encapsulated PostScript.  On a Mac this
	will be STANDARDTERM = 1 corresponding to Quickdraw
	For other platforms, new Gnuplot code will probably have to
	be written
*/

enum pointCodes
{
	DIAMONDPOINT = 0,
	PLUSPOINT,
	BOXPOINT,
	CROSSPOINT,
	TRIANGLEPOINT,
	STARPOINT,
	DOTPOINT,
	SMALLCROSSPOINT,
	POINT_TYPES /* = number of distinct types of points */
};

#ifdef TERMC__ /* should be defined only in term.c */
long            PutProlog = 1;
long            term = STANDARDTERM;
long            DumbHeight;
long            DumbWidth;
#ifdef MACINTOSH
long            PauseAfterPlot = 0;
#else /*MACINTOSH*/
long            PauseAfterPlot = 1;
#endif /*MACINTOSH*/
#else /*TERMC__*/
extern long     PutProlog;
extern long     term;
extern long     DumbHeight;
extern long     DumbWidth;
extern long     PauseAfterPlot;
#endif /*TERMC__*/

/* map floating point x and y to screen */
#define mapXToScreen(x) \
   (long)(GraphMap.xoffset + (x - GraphMap.xmin)*GraphMap.xscale)
#define mapYToScreen(y) \
   (long)(GraphMap.yoffset + (y - GraphMap.ymin)*GraphMap.yscale)

/* map screen coordinates to floating point x and y */
#define mapScreenToX(ix) \
	(GraphMap.xmin + ((double) ix - GraphMap.xoffset)/GraphMap.xscale)
#define mapScreenToY(iy) \
	(GraphMap.ymin + ((double) iy - GraphMap.yoffset)/GraphMap.yscale)

typedef struct graph_map
{
	double                xmin;
	double                xscale;
	double                xoffset;
	double                ymin;
	double                yscale;
	double                yoffset;
	long                  logx;
	long                  logy;
	long                  screenxmax;
	long                  screenymax;
} graph_map;

#ifdef GRAPHICSC__    /* defined only in graphics.c */
graph_map                 GraphMap;
#else /*GRAPHICSC__*/
extern graph_map          GraphMap;
#endif /*GRAPHICSC__*/

/*
typedef long BOOLEAN;
*/

/* See below for prototypes for public functions in term.c and graphics.c */

typedef int     (*FUNC_PTR) ();

/*
  Plot style constants used in version 3.33 and later
*/	

#ifdef wx_msw
#undef POINTS
#define POINTS POINTS_MV /* required because of POINTS in a windows header*/
#endif /*wx_msw*/

enum PlotStyles
{ /* Plot style constants used in Version 3.1x */
	LINES     = 0x01,         
	POINTS    = 0x02,         
	IMPULSES  = 0x04,         
	CHARS     = 0x08,         
	BOTHLANDP = LINES | POINTS, 
	BOTHLANDC = LINES | CHARS,  
	BOTHLANDI = LINES | IMPULSES
};

enum PlotStylesV31
{ /* Plot style constants used in Version 3.1x */
	LINES_V31,
	POINTS_V31,
	IMPULSES_V31,
	CHARS_V31,
	BOTHLANDP_V31,
	BOTHLANDC_V31
};

/* Line type is coded as
   (long) (MAXLINETYPE*(fraction*(UNITWIDTH/MAXLINETYPE)))+linetype
   where fraction is between .1 and 10 and specified the width of the line
   and linetype is between 0 and MAXLINETYPE-1 and specifies the patter of
   the line, e.g., solid, dashed, dotted, etc.
*/

#define POINTLINETYPE   (-4) /* for drawing diamond, box, ... */
#define XAXISLINETYPE   (-3) /* for drawing x-axis */
#define YAXISLINETYPE   (-2) /* for drawing y-axis */
#define BORDERLINETYPE  (-1) /* for drawing frame */

#define UNITWIDTH      1000000 /* code line width as (long) (width*UNITWIDTH)*/
#define MINLINETYPE    POINTLINETYPE
#define MAXLINETYPE    100     /* linetype must be < MAXLINETYPE */

enum JUSTIFY
{
	LEFT, CENTRE, RIGHT
};

enum STRINGSTYLES
{
	PLAIN, ITALIC, BOLD
};

/*
  980714 made justify unsigned char and added components stringcolor,
         stringstyle and reserved
*/
typedef struct plot_string 
{
	double                x, y;
	unsigned char         justify;
	unsigned char         stringcolor;
	unsigned char         stringstyle;
	unsigned char         reserved;
	char                  STAR_ string;
	struct plot_string    STAR_ next_string;
} plot_string;

/*
   960617  changed undefined from long to unsigned char
           added components  pointcolor, linecolor
*/
typedef struct coordinate
{
	/*	BOOLEAN undefined;	*//* TRUE if value off screen */
	unsigned char   undefined; /* != 0 means don't plot */
	unsigned char   pointcolor;
	unsigned char   linecolor;
	unsigned char   reserved;
	double          x, y;
	char            string[PLOTSTRINGSIZE+1]; /* changed from 3 by kb */
} coordinate;

/*
  If USEHANDLES is defined, the argument should be a (struct curve_points **),
  e.g., PLOTSTYLE(GRAPH(symh)), PLOTSTYLE(NEXTGRAPH(GRAPH(symh)))
  and the macros pre-suppose that all pointers in curve_point are
  changed to be handles 

  If USEHANDLES is not defined, the argument should be a (struct curve_points *)
  and the original definitions (points) in curve_point are assumed
*/

#ifdef USEHANDLES
/*
   A should be (struct curve-point **)
   B should be (struct plot_string  **)
   G should be (struct whole_graph  **)
*/
#define NEXTGRAPH(A)    ((*(A))->next_cp) /* handle to next graph */
#define PLOTSTYLE(A)    ((*(A))->plot_style)
#define POINTTYPE(A)    ((*(A))->point_type)
#define LINETYPE(A)     ((*(A))->line_type)
#define PCOUNT(A)       ((*(A))->p_count)
#define GRAPHPOINTS(A)  ((*(A))->points) /* handle to data */
#define GRAPHPOINT(A,I) ((*GRAPHPOINTS(A))[I])

/* in the following B should be a handle to a plot_string structure */

#define STRINGX(B)     ((*(B))->x)
#define STRINGY(B)     ((*(B))->y)
#define JUSTIFYSTR(B)  ((*(B))->justify)
#define STRINGCOLOR(B) ((*(B))->stringcolor)
#define STRINGSTYLE(B) ((*(B))->stringstyle)
#define PLOTSTRING(B)  ((*(B))->string) /* handle */
#define NEXTSTRING(B)  ((*(B))->next_string)

/* in the following G should be a handle to a whole_graph structure */

#define GRAPHNPLOTS(G)       ((*(G))->nplots)
#define GRAPHNSTRINGS(G)     ((*(G))->nstrings)
#define GRAPHXMIN(G)         ((*(G))->xmin)
#define GRAPHXMAX(G)         ((*(G))->xmax)
#define GRAPHYMIN(G)         ((*(G))->ymin)
#define GRAPHYMAX(G)         ((*(G))->ymax)
#define GRAPHLOGX(G)         ((*(G))->logx)
#define GRAPHLOGY(G)         ((*(G))->logy)
#define GRAPHNXTICKS(G)      ((*(G))->nxticks)
#define GRAPHNYTICKS(G)      ((*(G))->nyticks)
#define GRAPHXTICKLENGTH(G)  ((*(G))->xticklength)
#define GRAPHYTICKLENGTH(G)  ((*(G))->yticklength)
#define GRAPHXTICKS(G)       ((*(G))->xticks)
#define GRAPHYTICKS(G)       ((*(G))->yticks)
#define GRAPHXAXIS(G)        ((*(G))->xaxis)
#define GRAPHYAXIS(G)        ((*(G))->yaxis)
#define GRAPHFGCOLOR(G)      ((*(G))->foregroundcolor)
#define GRAPHBGCOLOR(G)      ((*(G))->backgroundcolor)
#define GRAPHFRAMECOLOR(G)   ((*(G))->framecolor)
#define GRAPHTITLE(G)        ((*(G))->title)
#define GRAPHXLAB(G)         ((*(G))->xlab)
#define GRAPHYLAB(G)         ((*(G))->ylab)
#define GRAPHFIRST_PLOT(G)   ((*(G))->first_plot)
#define GRAPHFIRST_STRING(G) ((*(G))->first_string)
#else /*USEHANDLES*/
/*
   A should be (struct curve-point *)
   B should be (struct plot_string  *)
   G should be (struct whole_graph  *)
*/

#define NEXTGRAPH(A)    ((A)->next_cp) /* pointer to next graph */
#define PLOTSTYLE(A)    ((A)->plot_style)
#define POINTTYPE(A)    ((A)->point_type)
#define LINETYPE(A)     ((A)->line_type)
#define PCOUNT(A)       ((A)->p_count)
#define GRAPHPOINTS(A)  ((A)->points) /* pointer to data */
#define GRAPHPOINT(A,I) (GRAPHPOINTS(A)[I])

#define STRINGX(B)     ((B)->x)
#define STRINGY(B)     ((B)->y)
#define JUSTIFYSTR(B)  ((B)->justify)
#define STRINGCOLOR(B) ((B)->stringcolor)
#define STRINGSTYLE(B) ((B)->stringstyle)
#define PLOTSTRING(B)  ((B)->string) /* pointer */
#define NEXTSTRING(B)  ((B)->next_string)

#define GRAPHNPLOTS(G)       ((G)->nplots)
#define GRAPHNSTRINGS(G)     ((G)->nstrings)
#define GRAPHXMIN(G)         ((G)->xmin)
#define GRAPHXMAX(G)         ((G)->xmax)
#define GRAPHYMIN(G)         ((G)->ymin)
#define GRAPHYMAX(G)         ((G)->ymax)
#define GRAPHLOGX(G)         ((G)->logx)
#define GRAPHLOGY(G)         ((G)->logy)
#define GRAPHNXTICKS(G)      ((G)->nxticks)
#define GRAPHNYTICKS(G)      ((G)->nyticks)
#define GRAPHXTICKLENGTH(G)  ((G)->xticklength)
#define GRAPHYTICKLENGTH(G)  ((G)->yticklength)
#define GRAPHXTICKS(G)       ((G)->xticks)
#define GRAPHYTICKS(G)       ((G)->yticks)
#define GRAPHXAXIS(G)        ((G)->xaxis)
#define GRAPHYAXIS(G)        ((G)->yaxis)
#define GRAPHFGCOLOR(G)      ((G)->foregroundcolor)
#define GRAPHBGCOLOR(G)      ((G)->backgroundcolor)
#define GRAPHFRAMECOLOR(G)   ((G)->framecolor)
#define GRAPHTITLE(G)        ((G)->title)
#define GRAPHXLAB(G)         ((G)->xlab)
#define GRAPHYLAB(G)         ((G)->ylab)
#define GRAPHFIRST_PLOT(G)   ((G)->first_plot)
#define GRAPHFIRST_STRING(G) ((G)->first_string)
#endif /*USEHANDLES*/

/* the following are independent of status of USEHANDLES */
#define POINTUNDEF(A,I)  (GRAPHPOINT(A,I).undefined)
#define POINTPCOLOR(A,I) (GRAPHPOINT(A,I).pointcolor)
#define POINTLCOLOR(A,I) (GRAPHPOINT(A,I).linecolor)
#define XCOORD(A,I)      (GRAPHPOINT(A,I).x)
#define YCOORD(A,I)      (GRAPHPOINT(A,I).y)
#define POINTSTRING(A,I) (GRAPHPOINT(A,I).string)


typedef struct curve_points
{
	struct curve_points STAR_ next_cp;	/* handle for next plot in linked list */
	long            plot_style;
	long            point_type;
	long            line_type;
	long            p_count;/* count of points in .points[] */

	coordinate STAR_ points; /* handle for data */
} curve_points;

/*
   960617 changed whole_graph
     logx, logy, xaxis, yaxis are unsigned char instead of long
     added components backgroundcolor, foregroundcolor, framecolor, reserved
	 added components nxticks, nyticks, xticks, yticks, xticklength,
	 yticklength
*/
typedef struct whole_graph
{
	long                  nplots; /*length of curve point_structure chain*/
	long                  nstrings;/*length of plot_string structure chain*/
	double                xmin; /*min X-value to be displayed*/
	double                xmax; /*max X-value to be displayed*/
	double                ymin; /*min Y-value to be displayed*/
	double                ymax; /*max Y-value to be displayed*/
	unsigned char         logx; /*log scale flag for x (not implemented)*/
	unsigned char         logy; /*log scale flag for y (not implemented)*/
	unsigned char         xaxis;/*draw x-axis scale flag*/
	unsigned char         yaxis;/*draw y-axis scale flag*/
	unsigned char         foregroundcolor;
	unsigned char         backgroundcolor;
	unsigned char         framecolor;
	unsigned char         reserved;
	unsigned char         xticklength; /*see below*/
	unsigned char         yticklength; /*see below*/
	unsigned char         nxticks; /*no. of user-specified x tick locations*/
	unsigned char         nyticks; /*no. of user-specified y tick locations*/
	double          STAR_ xticks; /* user-specified x tick locations*/
	double          STAR_ yticks; /* user-specified y tick locations*/
/* NB: title, xlab and ylab used to be in struct curve_points */
	char                  title[TITLESIZE+2]; /*kb*/
	char                  xlab[XLABELSIZE+2]; /*kb*/
	char                  ylab[YLABELSIZE+2]; /*kb*/
	curve_points STAR_ first_plot;
	plot_string  STAR_ first_string;
} whole_graph ;

/*
   Note: When xticklength or yticklength <= 2*TICKUNIT (128), they are
   interpreted as fractions {x,y}ticklengths/TICKUNIT.  When they > 
   2*TICKUNIT, they are interpreted as -({x,y}ticklengths-2*TICKUNIT)/TICKUNIT
   The resulting fraction determines the length of the drawn tick, with +1
   corresponding to the default, and values < 0 being drawn outside the
   frame
*/

/*
  970625 removed various typedef-like defines of uLong, etc.
*/

typedef struct termentry
{
	char           *name;
	unsigned long   xmax, ymax, v_char, h_char, v_tic, h_tic;
	int             (*init)(void);
	void            (*reset)(void);
	void            (*text)(void);
	int             (*graphics)(void);
	void            (*move)(unsigned long, unsigned long);
	void            (*vector)(unsigned long, unsigned long);
	void            (*linetype)(long);
	void            (*chr_text)(char);
	void            (*str_text)(char *);
	void            (*point)(long, long, long);
} termentry;

/* prototypes for public functions in term.c */
void horstring(long /*x*/, long /*y*/, char * /*str*/,
			   enum JUSTIFY /*justify*/);
void vertstring(long /*x*/, long /*y*/, char * /*str*/);

/* prototypes for public function in graphics.c */
long do_plot(curve_points STAR_ /*plots*/,long /*pcount*/,
	plot_string STAR_ /*strings*/, long /*scount*/,
	double /*xmin*/, double /*xmax*/, double /*ymin*/, double /*ymax*/,
	double * /*xticks*/, double * /*yticks*/,
	short /*nxticks*/, short /*nyticks*/,
	short /*xticklength*/, short /*yticklength*/,
	short /*logx*/, short /*logy*/, short /*xaxis*/, short /*yaxis*/,
	char * /*xlab*/, char * /*ylab*/, char * /*title*/);

#ifdef DEFINETYPES
#undef Void
#undef Double
#undef Long
#undef uLong
#undef Char
#undef uChar
#undef Int
#undef Short
#endif /*DEFINETYPES*/

#if (0)
/* original form of declaration */
struct termentry
{
	char           *name;
	unsigned long   xmax, ymax, v_char, h_char, v_tic, h_tic;
	FUNC_PTR        init, reset, text, graphics, move, vector, linetype,
	                chr_text, str_text, point;
};
#endif
#endif /*PLOTH__*/
