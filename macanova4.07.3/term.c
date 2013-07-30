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



#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment Graphics
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


/*
 *
 *    G N U P L O T  --  term.c
 *
 *  Copyright (C) 1986, 1987  Colin Kelley, Thomas Williams
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
 *  Modified for MacAnova by Gary W. Oehlert
 *  Further modified by C. Bingham
 *  if t is term_tbl[term], (*t->init)() should return 0 if o.k., 1, if not
 */

#undef TERMC__
#define TERMC__ /* should be defined only here */

#include "globals.h"
#include "plot.h"
#include <stdio.h>

#undef ASSERTIONS /* define to compile some assert macros */

#define PlotFile ((PLOTFILE == STDOUT) ? stdout : PLOTFILE)

#ifdef MACINTOSH
#include "macIface.h"
#endif /*MACINTOSH*/
enum entryCodes
{
	initCode,
	graphicsCode,
	textCode,
	resetCode,
	linetypeCode,
	moveCode,
	vectorCode,
	str_textCode,
	pointCode,
	chr_textCode
};

enum directionCodes /* for selecting graph patterns */
{
	NWSE = 0,
	NESW
};
	
void do_point(long /*x*/, long /*y*/, long /*number*/);

#define SCREENDUMPUNDEFINED 0

extern FILE   *PLOTFILE; /*kb*/
extern char   *TERMINAL;
extern char   *ScreenDumpFile;
extern short   ScreenDumpError;
extern struct termentry term_tbl[];

#if (0) /* following are in plot.h */
long            PutProlog = 1;
long            term = STANDARDTERM;
long            DumbHeight;
long            DumbWidth;
long            PauseAfterPlot = 0; /*or 1*/
#define POINT_TYPES		7
#endif /*0*/


#define sign(X)    (((X) >= 0) ? 1 : -1)

#ifdef MACINTOSH


#define MAC_XMAX 465
#define MAC_YMAX 263 /*273*/
#define MAC_XLAST (MAC_XMAX - 1)
#define MAC_YLAST (MAC_YMAX - 1)

#define MAC_VCHAR	11
#define MAC_HCHAR	6
#define MAC_VTIC	8
#define MAC_HTIC	7

#define HAIRLINESCALE   2

#define SetLineWidth  182  /* PicComment code from ISM */
typedef Point         TLineWidth, *TLineWidthPtr, **TLineWidthHdl;

#define NLINETYPES 5

#if (0) /*previous patterns */
static Pattern LinePatterns[NLINETYPES + 4] =
{ /* all slant strip patterns */
	{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, /* drawn points (solid) */
	{0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee}, /* x-axis (dashed)*/
	{0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00}, /* y-axis (dashed) */
	{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, /* border (solid) */
	{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, /* solid */
	{0xee, 0x77, 0xbb, 0xdd, 0xee, 0x77, 0xbb, 0xdd}, /* slant 11101110 */
	{0xaa, 0x66, 0x33, 0x99, 0xaa, 0x66, 0x33, 0x99}, /* slant 10101010 */
	{0x88, 0x44, 0x22, 0x11, 0x88, 0x44, 0x22, 0x11}, /* slant 10001000 */
	{0xe8, 0x74, 0x3a, 0x1d, 0x8e, 0x47, 0xa3, 0xd1}  /* slant 11101000 */
};
#endif /*0*/

static Pattern LinePatterns[2][NLINETYPES + 4] =
{ /* all slant strip patterns */
	{
		/*SouthEast - NorthWest patterns */
		{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, /* drawn points (solid) */
		{0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee}, /* x-axis (dashed)*/
		{0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00}, /* y-axis (dashed) */
		{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, /* border (solid) */
		{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, /* solid 11111111*/
		{0xfc, 0x7e, 0x3f, 0x9f, 0xcf, 0xe7, 0xf3, 0xf9}, /* slant 11111100 */
		{0xf8, 0x7c, 0x3e, 0x1f, 0x8f, 0xc7, 0xe3, 0xf1}, /* slant 11111000 */
		{0xf0, 0x78, 0x3c, 0x1e, 0x0f, 0x87, 0xc3, 0xe1}, /* slant 11110000 */
		{0xe0, 0x70, 0x38, 0x1c, 0x0e, 0x07, 0x83, 0xc1}  /* slant 11100000 */
#if (0) /*possible alternates*/
		{0xcc, 0x66, 0x33, 0x99, 0xcc, 0x66, 0x33, 0x99}  /* slant 11001100 */
		{0xee, 0x77, 0xbb, 0xdd, 0xee, 0x77, 0xbb, 0xdd}  /* slant 11101110 */
#endif /*0*/
	},
	{
		/*NorthEast - SouthWest patterns */
		{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, /* drawn points (solid) */
		{0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee}, /* x-axis (dashed)*/
		{0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00}, /* y-axis (dashed) */
		{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, /* border (solid) */
		{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, /* solid 11111111*/
		{0x3f, 0x7e, 0xfc, 0xf9, 0xf3, 0xe7, 0xcf, 0x9f}, /* slant 00111111 */
		{0x1f, 0x3e, 0x7c, 0xf8, 0xb3, 0xe3, 0xc7, 0x8f}, /* slant 00011111 */
		{0x0f, 0x1e, 0x3c, 0x78, 0xf0, 0xe1, 0xc3, 0x87}, /* slant 00001111 */
		{0x07, 0x0e, 0x1c, 0x38, 0x70, 0xe0, 0xc1, 0x83}  /* slant 00000111 */
#if (0) /*possible alternates*/
		{0x33, 0x66, 0xcc, 0x99, 0x33, 0x66, 0xcc, 0x99}  /* slant 00110011 */
		{0x77, 0xee, 0xdd, 0xbb, 0x77, 0xee, 0xdd, 0xbb}  /* slant 01110111 */
#endif /*0*/
	}
}; /*LinePatterns*/

static TLineWidthHdl LineWidthH = (TLineWidthHdl) 0;
static Integer       MacThickness;
static Integer       MacLinetype;
static Integer       MacFrameTop;
static Rect          MacPicFrame;
static PicHandle     MacPicH = (PicHandle) 0;
static Integer       MacError = noErr;
static unsigned long MacCurrentx, MacCurrenty;
static int           MacLastDirection = -1;


int            MAC_init(void);
int            MAC_graphics(void);
void           MAC_text(void);
void           MAC_reset(void);
void           MAC_linetype(long /*linetype*/);
void           MAC_move(unsigned long /*x*/, unsigned long /*y*/);
void           MAC_vector(unsigned long /*x*/, unsigned long /*y*/);
void           MAC_str_text(char * /*str*/);
void           MAC_point(long /*x*/, long /*7*/, long /*number*/);
void           MAC_chr_text(char /*c*/);

static void MAC_checkerror(int code)
{
	static int         found = 0;
	unsigned char     *temp = (unsigned char *) *MacPicH;
	WHERE("MAC_checkerror");

	if (MacError == noErr && temp[0] == 255 &&
		GetHandleSize((Handle) MacPicH) < 50)
	{
		MacError = memFullErr;
	}
} /*MAC_checkerror()*/

static void setLineScale(Integer scale)
{
	TLineWidth           oldLineWidth = **LineWidthH;

	/* undo previous width setting */
	(*LineWidthH)->v = oldLineWidth.h;
	(*LineWidthH)->h = oldLineWidth.v;
	PicComment(SetLineWidth, sizeof(TLineWidth), (Handle) LineWidthH);
	(*LineWidthH)->v = 1;
	(*LineWidthH)->h = scale;
	PicComment(SetLineWidth, sizeof(TLineWidth), (Handle) LineWidthH);
} /*setLineScale()*/

int MAC_init(void)
{
	FontInfo          fontInfo;
	struct termentry *t = term_tbl + term;
	WHERE("MAC_init");

	MacError = noErr;
	if (LineWidthH == (TLineWidthHdl) 0)
	{
		LineWidthH = (TLineWidthHdl) NewHandle(sizeof(TLineWidth));
	}
	if (LineWidthH != (TLineWidthHdl) 0)
	{
		MacPicH = GraphWindSetup();
		if (MacPicH == (PicHandle) -1)
		{
			putOutErrorMsg("ERROR: graph windows full - delete a graph and type showplot()");
			return (1);
		}
	} /*if (LineWidthH != (TLineWidthHdl) 0)*/
	if (MacPicH == (PicHandle) 0)
	{
		putOutErrorMsg("ERROR: cannot create graph. Out of memory (?)");
		return (1);
	}

	MacPicFrame = (*MacPicH)->picFrame;

	GetFontInfo(&fontInfo);
	fontInfo.leading = 0; /* use 0 leading */
	fontInfo.widMax = CharWidth((Integer) 'M');
	t->v_char = fontInfo.ascent + fontInfo.descent +
		fontInfo.leading;
	t->h_char = fontInfo.widMax;
	t->v_tic = t->h_tic = fontInfo.widMax;
	t->xmax = MacPicFrame.right - MacPicFrame.left;
	t->ymax = MacPicFrame.bottom - MacPicFrame.top;
	MacFrameTop = t->ymax;
	MacThickness = 1;

	(*LineWidthH)->v = 1;
	(*LineWidthH)->h = 1;
	PicComment(SetLineWidth, sizeof(TLineWidth), (Handle) LineWidthH);
	MAC_checkerror(initCode);

	return (0);
} /*MAC_init()*/


int MAC_graphics(void)
{
	return (0);
} /*MAC_graphics()*/


void MAC_text(void)
{
	struct termentry *t = term_tbl + term;

	GraphOver(MacError);
	if (MacError == noErr)
	{
		if (ScreenDumpFile != (char *) 0)
		{
			ScreenDumpError = screenDump(ScreenDumpFile);
		}
		if (PauseAfterPlot)
		{
			GraphWindPause();
		}
	} /*if (MacError == noErr)*/

	t->reset();
} /*MAC_text()*/

void MAC_linetype(long linetype)
{
	Integer    heavy = (10*linetype >= UNITWIDTH);
	double     thickness = (!heavy) ? 1.0 :
		(double) (MAXLINETYPE * (linetype/MAXLINETYPE)) / (double) UNITWIDTH;
	WHERE("MAC_linetype");

	MacThickness = (Integer) ceil(thickness);
	if(heavy)
	{
		linetype %= MAXLINETYPE;
	}
	PenSize(MacThickness, MacThickness);
	MAC_checkerror(linetypeCode);
	if(linetype < MINLINETYPE)
	{
		linetype = BORDERLINETYPE;
	}
	linetype = ((linetype < 0) ? linetype: linetype % NLINETYPES) -
		MINLINETYPE;
	MacLinetype = linetype;
	MacLastDirection = -1;

	setLineScale(HAIRLINESCALE);
	MAC_checkerror(linetypeCode);
} /*MAC_linetype()*/

void MAC_move(unsigned long x, unsigned long y)
{
	MoveTo(x, MacFrameTop - y);
	MacCurrentx = x;
	MacCurrenty = y;
	MacLastDirection = -1;
	MAC_checkerror(moveCode);
} /*MAC_move()*/


void MAC_vector(unsigned long x, unsigned long y)
{
	Integer    adjust = (MacThickness-1)/2;
	int        direction;

	if (MacLinetype > 0)
	{
		if (x > MacCurrentx)
		{
			direction = (y > MacCurrenty) ? NWSE : NESW;
		}
		else
		{
			direction = (y > MacCurrenty) ? NESW : NWSE;
		}
	}
	else
	{
		direction = NWSE;
	}
	
	if (direction != MacLastDirection)
	{
#if defined(MPW3_2)
		PenPat((ConstPatternParam) LinePatterns[direction] + MacLinetype);
#elif defined(MW_CW) /*MPW3_2*/
		PenPat((Pattern *) LinePatterns[direction] + MacLinetype);
#else /*MPW3_2*/
		PenPat((Pattern) LinePatterns[direction] + MacLinetype);
#endif /*MPW3_2*/
		MAC_checkerror(vectorCode);
		MacLastDirection = direction;
	}

	if(adjust)
	{  /* makesure line is centered */
		Move(-adjust,-adjust);
		LineTo(x-adjust, MacFrameTop - y - adjust);
		Move(adjust,adjust);
	}
	else
	{
		LineTo(x, MacFrameTop - y);
	}
	MAC_checkerror(vectorCode);
	MacCurrentx = x;
	MacCurrenty = y;
} /*MAC_vector()*/

void MAC_point(long x, long y, long number)
{
	if(number < 0 || number % POINT_TYPES == DOTPOINT)
	{
		setLineScale(1); /* make sure dot is big enough to see */
	}
	do_point(x, y, number);
} /*MAC_point()*/

/* write string to screen while still in graphics mode */
void MAC_str_text(char str [])
{
	DrawText(str, 0, strlen(str));
	MAC_checkerror(str_textCode);
} /*MAC_str_text()*/


 /* write character to screen while still in graphics mode */
void MAC_chr_text(char chr)
{
	DrawText(&chr, 0, 1);
	MAC_checkerror(chr_textCode);
} /*MAC_chr_text()*/


void MAC_reset(void)
{
} /*MAC_reset()*/

#endif /*MACINTOSH*/

#ifdef WXWIN
#include "wx/wxIface.h"
#define WX_XMAX DEFAULTCANVASWINDOWWIDTH
#define WX_YMAX DEFAULTCANVASWINDOWHEIGHT 
#define WX_XLAST (MAC_XMAX - 1)
#define WX_YLAST (MAC_YMAX - 1)

#define WX_VCHAR	11
#define WX_HCHAR	6
#define WX_VTIC	8
#define WX_HTIC	7

/* Functions defined in wxgraph.cc */
extern int            WX_init(void);
extern int            WX_graphics(void);
extern void           WX_text(void);
extern void           WX_reset(void);
extern void           WX_linetype(long linetype);
extern void           WX_move(unsigned long x, unsigned long y);
extern void           WX_vector(unsigned long x, unsigned long y);
extern void           WX_str_text(char * str);
extern void           WX_chr_text(char c);

static void setLineScale(int scale){}
void WX_point(long x, long y, long number)
{
	if(number < 0 || number % POINT_TYPES == 6)
	{
		setLineScale(1); /* make sure dot is big enough to see */
	}
	do_point(x, y, number);
} /*WX_point()*/

#endif /*WXWIN*/

#ifdef DJGPP
/* For djgpp compiler for 32 bit protected mode MSDOS */
/*
  970609 GO enabled multiple line types using filled polygons of
         appropriate shapes and sized
		 */
#ifndef GRX20
#include <grx.h>
#else /*GRX20*/
#include <grx20.h>
#endif /*GRX20*/

#include <pc.h>
#include <osfcn.h>

#define pcxdump   screenDump
#include "pcxdump.h"

#ifndef GRX20
#define FontWidth  (FontInfo->fnt_width)
#define FontHeight (FontInfo->fnt_height)
#else /*GRX20*/
#define FontWidth (FontInfo->maxwidth)
#define FontHeight GrFontCharHeight(FontInfo,'M')
#endif /*GRX20*/

#define setTextFont(FI) (TextOptions.txo_font = (FI))

#ifndef GRX20
#define setTextMag(XMAG,YMAG)\
	(TextOptions.txo_xmag = (XMAG), TextOptions.txo_ymag = (YMAG))
#else /*GRX20*/
#define setTextMag(XMAG,YMAG) /*nothing*/
#endif /*GRX20*/

#define setTextColors(FG, BG) \
	(TextOptions.txo_fgcolor.v = (FG), TextOptions.txo_bgcolor.v = (BG))
#define setTextDirection(D) (TextOptions.txo_direct = (D))
#define setTextAlignment(A, B)\
	(TextOptions.txo_xalign = (A), TextOptions.txo_yalign = (B))
#define setTextChrtype(T) (TextOptions.txo_chrtype = (T))

#ifndef GRX20
#define DrawLine(X1, Y1, X2, Y2)\
	GrCustomLine(X1, pc_ymax-1-Y1, X2, pc_ymax-1-Y2, &LineOptions)
#else /*GRX20*/
#if (0)
#define DrawLine(X1, Y1, X2, Y2)\
	 GrCustomLine(X1, pc_ymax-1-Y1, X2, pc_ymax-1-Y2, &LineOptions) 
	/*  GrLine(X1, pc_ymax-1-Y1, X2, pc_ymax-1-Y2, GrBlack()) */
#else /*(0)*/
#define DrawLine(X1, Y1, X2, Y2)\
	GrPatternFilledPolygon(2, GPFP, thisGrPat);
#endif /*(0)*/
#endif /*GRX20*/

#define setLineColor(C) (LineOptions.lno_color = (C))
#define setLineWidth(W) (LineOptions.lno_width = (W))
#define setLinePattlen(L) (LineOptions.lno_pattlen = (L))
#define setLineDashpat(P) (LineOptions.lno_dashpat = (P))

#undef FALSE
#define FALSE 0
#undef TRUE
#define TRUE 1

int            DJG_init(void);
int            DJG_graphics(void);
void           DJG_text(void);
void           DJG_reset(void);
void           DJG_linetype(long /*linetype*/);
void           DJG_move(unsigned long /*x*/, unsigned long /*y*/);
void           DJG_vector(unsigned long /*x*/, unsigned long /*y*/);

#ifdef GNUPLOT3
int            DJG_text_angle(int /*ang*/);
int            DJG_justify_text(enum JUSTIFY /*mode*/);
void           DJG_put_text(long /*x*/,long /*y*/,char * /*str*/);
#else /*GNUPLOT3*/
void           DJG_str_text(char * /*str*/);
void           DJG_chr_text(char /*c*/);
#endif /*GNUPLOT3*/

#define NLINETYPES 5
typedef struct
{
	unsigned char pattern[8];
} Pattern;

static Pattern LinePatterns[2][NLINETYPES + 4] =
{ /* all slant strip patterns, same as Macintosh */
	{
		/*SouthEast - NorthWest patterns */
		{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, /* drawn points (solid) */
		{0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee}, /* x-axis (dashed)*/
		{0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00}, /* y-axis (dashed) */
		{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, /* border (solid) */
		{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, /* solid 11111111*/
		{0xfc, 0x7e, 0x3f, 0x9f, 0xcf, 0xe7, 0xf3, 0xf9}, /* slant 11111100 */
		{0xf8, 0x7c, 0x3e, 0x1f, 0x8f, 0xc7, 0xe3, 0xf1}, /* slant 11111000 */
		{0xf0, 0x78, 0x3c, 0x1e, 0x0f, 0x87, 0xc3, 0xe1}, /* slant 11110000 */
		{0xe0, 0x70, 0x38, 0x1c, 0x0e, 0x07, 0x83, 0xc1}  /* slant 11100000 */
#if (0) /*possible alternates*/
		{0xcc, 0x66, 0x33, 0x99, 0xcc, 0x66, 0x33, 0x99}  /* slant 11001100 */
		{0xee, 0x77, 0xbb, 0xdd, 0xee, 0x77, 0xbb, 0xdd}  /* slant 11101110 */
#endif /*0*/
	},
	{
		/*NorthEast - SouthWest patterns */
		{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, /* drawn points (solid) */
		{0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee}, /* x-axis (dashed)*/
		{0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00}, /* y-axis (dashed) */
		{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, /* border (solid) */
		{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, /* solid 11111111*/
		{0x3f, 0x7e, 0xfc, 0xf9, 0xf3, 0xe7, 0xcf, 0x9f}, /* slant 00111111 */
		{0x1f, 0x3e, 0x7c, 0xf8, 0xb3, 0xe3, 0xc7, 0x8f}, /* slant 00011111 */
		{0x0f, 0x1e, 0x3c, 0x78, 0xf0, 0xe1, 0xc3, 0x87}, /* slant 00001111 */
		{0x07, 0x0e, 0x1c, 0x38, 0x70, 0xe0, 0xc1, 0x83}  /* slant 00000111 */
#if (0) /*possible alternates*/
		{0x33, 0x66, 0xcc, 0x99, 0x33, 0x66, 0xcc, 0x99}  /* slant 00110011 */
		{0x77, 0xee, 0xdd, 0xbb, 0x77, 0xee, 0xdd, 0xbb}  /* slant 01110111 */
#endif /*0*/
	}
}; /*LinePatterns*/

#if (0)
static Pattern LinePatterns[NLINETYPES + 4] =
{ /* all slant strip patterns, same as Macintosh */
	{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, /* drawn points (solid) */
	{0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee}, /* x-axis (dashed)*/
	{0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00}, /* y-axis (dashed) */
	{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, /* border (solid) */
	{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, /* solid */
	{0xee, 0x77, 0xbb, 0xdd, 0xee, 0x77, 0xbb, 0xdd}, /* slant 11101110 */
	{0xaa, 0x66, 0x33, 0x99, 0xaa, 0x66, 0x33, 0x99}, /* slant 10101010 */
	{0x88, 0x44, 0x22, 0x11, 0x88, 0x44, 0x22, 0x11}, /* slant 10001000 */
	{0xe8, 0x74, 0x3a, 0x1d, 0x8e, 0x47, 0xa3, 0xd1}  /* slant 11101000 */
};
#endif /*0*/

static GrBitmap ThisGrBM;

#if (0) /*obsolete*/
/*
	These patterns are defined, but currently (940811) do not get used by
	the grx library
*/
static unsigned char pattern[5][2] =
{
	{0, 0},
	{2, 2},
	{5, 5},
	{5, 2},
	{1, 1}
};  
	/* 0xffff, 0xaaaa, 0x3333, 0x3f3f, 0x0f0f */
static int patternLength[] =
{
	0, 2, 2, 2, 2
};
#endif /*0*/

static int           graphics_on = FALSE;
static unsigned long DJGCurrentx, DJGCurrenty;
static int           DJGLastDirection = -1;
static int           pc_angle;

#define RIGHTOFFSET 5

/*
   These defines are essentially ignored since info comes from calls
   to functions
*/

#define DJG_XMAX 640
#define DJG_YMAX 480

#define DJG_VCHAR 12
#define DJG_HCHAR 8
#define DJG_VTIC 8
#define DJG_HTIC 8

static GrFont           *FontInfo = (GrFont *) 0;
static int               TextGrMode = GR_80_25_text, GraphicGrMode = -1, pc_ymax;
static GrLineOption      LineOptions;
static GrTextOption      TextOptions;
static int               DJGthickness;
static int               DJGlinetype;


void DJG_text(void)
{
	struct termentry *t = term_tbl + term;
	if (graphics_on)
	{
		if (ScreenDumpFile != (char *) 0)
		{
			ScreenDumpError = pcxdump(ScreenDumpFile);
		}
		graphics_on = FALSE;
		if((ISATTY & ITTYIN) && PauseAfterPlot)
		{
			(void) getkey(); /* wait for CR */
		}
	}
	if( TextGrMode >= 0)
	{
		GrSetMode( TextGrMode );
	}
	else
	{
		GrSetMode(GR_80_25_text);
	}
	t->reset();
} /*DJG_text()*/

void DJG_reset(void)
{
} /*DJG_reset()*/

int DJG_init(void)
{
	char             *path;
	struct termentry *t = term_tbl + term;

	if( !graphics_on && TextGrMode < 0)
	{
		TextGrMode = GrCurrentMode();
	}

	GrSetMode( GR_default_graphics );
	GrClearScreen( GrWhite() );

	if( GraphicGrMode < 0)
	{
		t->xmax = GrSizeX() - RIGHTOFFSET;
		t->ymax = GrSizeY();
		pc_ymax = t->ymax;

		GrSetFontPath( get_dataPath() );
		FontInfo = GrLoadFont("cour14.fnt");
		if( FontInfo == (GrFont *) 0) 
		{
			putOutErrorMsg("ERROR: graphics font file not found");
			return(1);
		}
		t->v_char = FontHeight;
		t->h_char = FontWidth;
		t->v_tic = FontWidth;
		t->h_tic = FontWidth;
#if (0) /* original code */
		t->v_char = FontInfo->fnt_height;
		t->h_char = FontInfo->fnt_width;
		t->v_tic = FontInfo->fnt_width;
		t->h_tic = FontInfo->fnt_width;
#endif
		GraphicGrMode =  GrCurrentMode();

		setTextFont(FontInfo);
		setTextMag(1, 1);
		setTextColors(GrBlack(), GrNOCOLOR);
		setTextDirection(GR_TEXT_RIGHT);
		setTextAlignment(GR_ALIGN_LEFT, GR_ALIGN_BOTTOM);
		setTextChrtype(GR_BYTE_TEXT);

#if (0) /* original code */
		TextOptions.txo_font = FontInfo;
		TextOptions.txo_xmag = 1;
		TextOptions.txo_ymag = 1;
		TextOptions.txo_fgcolor.v = GrBlack();
		TextOptions.txo_bgcolor.v = GrNOCOLOR;
		TextOptions.txo_direct = GR_TEXT_RIGHT;
		TextOptions.txo_xalign = GR_ALIGN_LEFT;
		TextOptions.txo_yalign = GR_ALIGN_BOTTOM;
		TextOptions.txo_chrtype = GR_BYTE_TEXT;
#endif
	}

	graphics_on = TRUE;

	setLineColor(GrBlack());
	setLineWidth(1);
	setLinePattlen(0);

#if (0)
	LineOptions.lno_color = GrBlack();
	LineOptions.lno_width = 1;
	LineOptions.lno_pattlen = 0; /* continuous lines by default */
#endif


	return(0);
} /*DJG_init()*/


int DJG_graphics(void)
{
	return(0);
} /*DJG_graphics()*/


void DJG_linetype(long linetype)
{
	long       heavy = (10*linetype >= UNITWIDTH);
	double     thickness = (heavy) ?
		(double) (MAXLINETYPE * (linetype/MAXLINETYPE)) / (double) UNITWIDTH : 1.0;

	ThisGrBM.bmp_ispixmap = 0;
	ThisGrBM.bmp_height = 8;
	ThisGrBM.bmp_fgcolor = GrBlack();
	ThisGrBM.bmp_bgcolor = GrWhite();
	ThisGrBM.bmp_memflags = 0;

	if(heavy)
	{
		linetype %= UNITWIDTH;
	}
	DJGthickness = (int) ceil(thickness);
	if(thickness <=1.0)
	{
		heavy = 0;
	}
	if(linetype > 0)
	{
		linetype %= NLINETYPES;
	}
	DJGlinetype = linetype;
	DJGLastDirection = -1;

#if (0)
	setLineWidth(thickness);
	setLinePattlen(patternLength[linetype]);

	if(linetype > 0)
	{
		setLineDashpat(pattern[linetype]);
	}
#endif /*(0)*/

#if (0) /* original code */
	LineOptions.lno_width = (heavy)? 3 : 1;
	LineOptions.lno_pattlen = 0;
	if(linetype)
	{
		LineOptions.lno_pattlen = patternLength[linetype];
		LineOptions.lno_dashpat = pattern[linetype];
	}
#endif
} /*DJG_linetype()*/

#ifdef GNUPLOT3
void DJG_put_text(long x, long y, char * str)
{
	struct termentry *t = term_tbl + term;

	strcpy(buf,str);
	outtextxy((int) x,(int) (t->ymax-1-y),buf);
} /*DJG_put_text()*/
#else /*GNUPLOT3*/
void DJG_str_text(char * str)
{
	GrDrawString( str, strlen(str), DJGCurrentx, pc_ymax - 1 - DJGCurrenty,
				  &TextOptions);
} /*DJG_str_text()*/

void DJG_chr_text(char c)
{
	char cbuf[2];
	cbuf[0] = c;
	cbuf[1] = '\0';
	DJG_str_text(cbuf);
} /*DJG_chr_text()*/
#endif /*GNUPLOT3*/

void DJG_move(unsigned long x,unsigned long y)
{
	DJGCurrentx = x;
	DJGCurrenty = y;
	DJGLastDirection = -1;
} /*DJG_move()*/

unsigned char thisGrdata = 3+3*16;
int GPFP[4][2];

void DJG_vector(unsigned long x, unsigned long y)
{
	int         drop, lift;
	int         direction;
	int         npts;

	if (DJGlinetype > 0)
	{
		if (x > DJGCurrentx)
		{
			direction = (y > DJGCurrenty) ? NWSE : NESW;
		}
		else
		{
			direction = (y > DJGCurrenty) ? NESW : NWSE;
		}
	}
	else
	{
		direction = NESW;
	}
	
	if (direction != DJGLastDirection)
	{
		ThisGrBM.bmp_data =
			(unsigned char *) &LinePatterns[direction][DJGlinetype - MINLINETYPE];
		DJGLastDirection = direction;
	}

#if (0)
	DrawLine(DJGCurrentx, DJGCurrenty, x, y);
#endif /*(0)*/

	if(DJGthickness == 1)
	{
		npts = 2;
		drop = 0;
	}
	else
	{
		npts = 4;
		drop = (DJGthickness)/2;
		lift = (DJGthickness-1)/2;
		GPFP[2][0] = x;
		GPFP[3][0] = DJGCurrentx;
		GPFP[2][1] = pc_ymax - 1 - y + lift;
		GPFP[3][1] = pc_ymax - 1 - DJGCurrenty + lift;	
	}
	
	GPFP[0][0] = DJGCurrentx;
	GPFP[1][0] = x;
	GPFP[0][1] = pc_ymax - 1 - DJGCurrenty - drop;
	GPFP[1][1] = pc_ymax - 1 - y - drop;

	GrPatternFilledPolygon(npts, GPFP, (GrPattern *) &ThisGrBM);

	DJGCurrentx = x;
	DJGCurrenty = y;
} /*DJG_vector()*/


#ifdef GNUPLOT3
int DJG_text_angle(int ang)
{
	int       size = getmaxy() > 599 ? 2 : 1;

	pc_angle = ang;

	switch (ang)
	{
	  case 0:
		settextstyle(DEFAULT_FONT,HORIZ_DIR,size);
		break;
	  case 1:
		settextstyle(DEFAULT_FONT,VERT_DIR,size);
		break;
	}
	return TRUE;
} /*DJG_text_angle()*/

int DJG_justify_text(enum JUSTIFY mode)
{
	switch(mode)
	{
	  case LEFT :
		settextjustify(LEFT_TEXT,CENTER_TEXT);
		break;
	  case CENTRE :
		settextjustify(CENTER_TEXT,CENTER_TEXT);
		break;
	  case RIGHT:
		settextjustify(RIGHT_TEXT,CENTER_TEXT);
		break;
	} /*switch(mode)*/
	return TRUE;
} /*DJG_justify_text()*/
#endif /*GNUPLOT3*/

#endif /* DJGPP */

#ifdef BCPP
/* For Borland C++ compiler */


/* GNUPLOT - pc.trm */
/*
 * Copyright (C) 1990
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the modified code.  Modifications are to be distributed
 * as patches to released version.
 *
 * This software  is provided "as is" without express or implied warranty.
 *
 * This file is included by ../term.c.
 *
 * This terminal driver supports:
 *  Under Microsoft C
 *      cga, egabios, egalib, vgabios, hercules, corona325, att
 *  Under Turboc C
 *      egalib, vgalib, vgamono, svga, mcga, cga, hercules, att
 *
 * AUTHORS
 *  Colin Kelley, Thomas Williams, William Wilson, Russell Lang
 *
 * send your comments or suggestions to (pixar!info-gnuplot@sun.com).
 *
 */

#include <graphics.h>
#include <conio.h>
#include <dos.h>

char *getenv();

#define FALSE 0
#define TRUE 1
int            BC_init(void);
int            BC_graphics(void);
void           BC_text(void);
void           BC_reset(void);
void           BC_linetype(long /*linetype*/);
void           BC_move(unsigned long /*x*/, unsigned long /*y*/);
void           BC_vector(unsigned long /*x*/, unsigned long /*y*/);

#ifdef GNUPLOT3
int            BC_text_angle(int /*ang*/);
int            BC_justify_text(enum JUSTIFY /*mode*/);
void           BC_put_text(long /*x*/,long /*y*/,char * /*str*/);
#else /*GNUPLOT3*/
void           BC_str_text(char * /*str*/);
void           BC_chr_text(char /*c*/);
#endif /*GNUPLOT3*/

static int              g_driver = DETECT, g_mode, g_error;

static struct text_info tinfo; /* So we can restore starting text mode. */

static char buf[80];       /* kludge since EGA.LIB is compiled SMALL */

#if (0)
static int 	   pattern[] =
{
	0xffff, 0x0f0f, 0xffff, 0xaaaa, 0x3333, 0x3f3f, 0x0f0f
};
#else /*0*/
static int     pattern[] =
{
	0xffff, 0xaaaa, 0x3333, 0x3f3f, 0x0f0f
};
#endif /*-*/

static int     graphics_on = FALSE;
int            startx, starty;

int            pc_angle;


void BC_text(void)
{
	struct termentry *t = term_tbl + term;

	if (ScreenDumpFile != (char *) 0)
	{
		ScreenDumpError = SCREENDUMPUNDEFINED;
	}
	if (graphics_on)
	{
		graphics_on = FALSE;
		if((ISATTY & ITTYIN) && PauseAfterPlot)
		{
			(void) getch(); /* wait for CR */
		}
	}
	restorecrtmode();
	t->reset();
} /*BC_text()*/

void BC_reset(void)
{
#if (0)
	closegraph();
#endif /*0*/
	textmode(tinfo.currmode);
    clrscr();
} /*BC_reset()*/

#define BC_XMAX 640
#define BC_YMAX 480

#define BC_VCHAR 12
#define BC_HCHAR 8
#define BC_VTIC 8
#define BC_HTIC 8

#ifdef GNUPLOT3
int BC_text_angle(int ang)
{
	int        size = getmaxy() > 599 ? 2 : 1;

	pc_angle = ang;

	switch (ang)
	{
	  case 0:
		settextstyle(DEFAULT_FONT,HORIZ_DIR,size);
		break;
	  case 1:
		settextstyle(DEFAULT_FONT,VERT_DIR,size);
		break;
	} /*switch (ang)*/
	return TRUE;
} /*BC_text_angle()*/

int BC_justify_text(enum JUSTIFY mode)
{
	switch(mode)
	{
	  case LEFT :
		settextjustify(LEFT_TEXT,CENTER_TEXT);
		break;
	  case CENTRE :
		settextjustify(CENTER_TEXT,CENTER_TEXT);
		break;
	  case RIGHT:
		settextjustify(RIGHT_TEXT,CENTER_TEXT);
		break;
	} /*switch(mode)*/
	return TRUE;
} /*BC_justify_text()*/
#endif /*GNUPLOT3*/

int BC_init(void)
{
	char             *path;
	struct termentry *t = term_tbl + term;

	if( g_driver == DETECT)
	{
		gettextinfo(&tinfo);
		path = get_dataPath();
		path[strlen(path)-1] = '\0'; /* wipe out trailing '\' */
		initgraph(&g_driver,&g_mode,(char far *) path);
		path[strlen(path)] = '\\';
	}

	if(g_driver<0)
	{
		switch (g_driver)
		{
		  case -2:
			putOutErrorMsg("ERROR: Graphics card not detected.");
			break;
		  case -3:
			putOutErrorMsg("ERROR: appropriate graphics driver cannot be found.");
			break;
		  case -5:
			putOutErrorMsg("ERROR: Insufficient memory to load graphics driver.");
			break;
		  default:
			putOutErrorMsg("ERROR: problem in initializing graphics");
		}
		g_driver = DETECT;
		return (1);
	} /*if(g_driver<0)*/

	t->xmax = getmaxx() + 1;
	t->ymax = getmaxy() + 1;
	if( getmaxy() > 599)
	{
		t->v_char = 2*BC_VCHAR;
		t->h_char = 2*BC_HCHAR;
		t->v_tic = 2*BC_VTIC;
		t->h_tic = 2*BC_HTIC;
	}
	else {
		t->v_char = BC_VCHAR;
		t->h_char = BC_HCHAR;
		t->v_tic = BC_VTIC;
		t->h_tic = BC_HTIC;
	}
	restorecrtmode();
	textmode(tinfo.currmode);

	return(0);
} /*BC_init()*/


int BC_graphics(void)
{
	graphics_on = TRUE;
	gettextinfo(&tinfo);
	setgraphmode(g_mode);
	settextjustify(LEFT_TEXT,BOTTOM_TEXT);

	if (getmaxy() > 599)		/* Double the tic/font sizes. */
	{
		settextstyle(DEFAULT_FONT,HORIZ_DIR,2);
	}
	else
	{
		settextstyle(DEFAULT_FONT,HORIZ_DIR,1);
	}
	return(0);
} /*BC_graphics()*/


void BC_linetype(long linetype)
{
	long       heavy = (10*linetype >= UNITWIDTH);
	double     thickness = (!heavy) ? 1.0 :
		(double) (MAXLINETYPE * (linetype/MAXLINETYPE)) / (double) UNITWIDTH;

	if(heavy)
	{
		linetype %= UNITWIDTH;
	}

	if(linetype < 0)
	{
		if(linetype == XAXISLINETYPE || linetype == YAXISLINETYPE)
		{
			linetype = 2;
		}
		else
		{
			linetype = 0;
		}
	} /*if(linetype < 0)*/
	else
	{
		linetype %= 5;
	} /*if(linetype < 0){}else{}*/

	setlinestyle(USERBIT_LINE,pattern[linetype],
				 (thickness > 1.0) ? THICK_WIDTH : NORM_WIDTH);
} /*BC_linetype()*/

#ifdef GNUPLOT3
void BC_put_text(long x, long y, char * str)
{
	struct termentry *t = term_tbl + term;

	strcpy(buf,str);
	outtextxy((int) x,(int) (t->ymax-1-y),buf);
} /*BC_put_text()*/
#else /*GNUPLOT3*/
void BC_str_text(char * str)
{
	outtext(str);
} /*BC_str_text()*/

void BC_chr_text(char c)
{
	char         cbuf[2];
	cbuf[0] = c;
	cbuf[1] = '\0';
	BC_str_text(cbuf);
} /*BC_chr_text()*/
#endif /*GNUPLOT3*/

void BC_move(unsigned long x, unsigned long y)
{
	moveto((int) x, (int)(getmaxy()-y));
} /*BC_move()*/

void BC_vector(unsigned long x, unsigned long y)
{
	lineto((int) x, (int) (getmaxy()-y));
} /*BC_vector()*/


#endif /*BCPP*/

#ifdef POSTSCRIPT
/* GNUPLOT - post.trm */
/*
 * Copyright (C) 1990
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the modified code.  Modifications are to be distributed
 * as patches to released version.
 *
 * This software  is provided "as is" without express or implied warranty.
 *
 * This file is included by ../term.c.
 *
 * This terminal driver supports:
 *     postscript
 *
 * AUTHORS
 *  Russell Lang
 *
 * send your comments or suggestions to (pixar!info-gnuplot@sun.com).
 *
 * The 'postscript' driver produces landscape output 10" wide and 7" high.
 * To get a smaller epsf output use 'set size 0.5,0.5',
 * 'set term postscript portrait', make only one plot per file
 * and change the first line of the postscript file from
 * '%!PS-Adobe-2.0' to '%!PS-Adobe-2.0 EPSF-2.0'
 * To change font to Times-Roman and font size to 20pts use
 * 'set term postscript "Times-Roman" 20'.
 */

/* PostScript driver by Russell Lang, rjl@monu1.cc.monash.edu.au */
#define PS_FONTSIZE  12
#define PS_PORTRAIT  1

#ifndef xsize
#define xsize   1.0  /* global defined in setshow.c in gnuplot 3 */
#endif /*xsize*/
#ifndef ysize
#define ysize   1.0  /* global defined in setshow.c in gnuplot 3 */
#endif /*ysize*/

void           PS_options(void);
int            PS_init(void);
int            PS_graphics(void);
void           PS_text(void);
void           PS_reset(void);
void           PS_linetype(long /*linetype*/);
void           PS_move(unsigned long /*x*/, unsigned long /*y*/);
void           PS_point(long /*x*/, long /*y*/, long /*point*/);
void           PS_vector(unsigned long /*x*/, unsigned long /*y*/);
void           PS_str_text(char * /*str*/);
void           PS_chr_text(char /*c*/);

char           ps_font[] = "Courier" ; /* name of font */
int            ps_fontsize = PS_FONTSIZE; /* size of font in pts */
long           ps_portrait = PS_PORTRAIT;				 /* vertical page */
long           ps_color = 0;
int            ps_page=0;			/* page count */
int            ps_path_count=0; 	/* count of lines in path */
int            ps_ang=0;			/* text angle */
enum JUSTIFY   ps_justify=LEFT;	/* text is flush left */
int            ps_xoff, ps_yoff; /* offsets (added by kb 960528)*/
double         ps_aspect = 1.6;  /* xmax/ymax (added by kb 960528)*/

FILE *outfile =  (FILE *) 0;

static char *PS_header[] =
{
	"/vpt2 vpt 2 mul def\n", /*max height of drawn symbols*/
	"/hpt2 hpt 2 mul def\n", /*max width of drawn symbols*/
	"/vpt23 vpt 2 mul 3 div def\n",
	"/hpt23 hpt 2 mul 3 div def\n",
	"/vpt43 vpt 4 mul 3 div def\n",
	"/hpt43 hpt 4 mul 3 div def\n",
	/* flush left show */
	"/Lshow { currentpoint stroke moveto\n",
	"  0 vshift rmoveto show } def\n",
	/* flush right show */
	"/Rshow { currentpoint stroke moveto\n",
	"  dup stringwidth pop neg vshift rmoveto show } def\n",
	/* centred show */
	"/Cshow { currentpoint stroke moveto\n",
	"  dup stringwidth pop -2 div vshift rmoveto show } def\n",
	/* Dash or Color Line */
	"/DL { Color {setrgbcolor [] 0 setdash pop}\n",
	" {pop pop pop 0 setdash} ifelse } def\n",
	/* set line width */
	"/LW	{ gnulinewidth mul /mylinewidth exch def } def\n",
	/* Border Lines */
/*	"/BL { stroke mylinewidth 2 mul setlinewidth } def\n", */
	"/BL { stroke mylinewidth setlinewidth } def\n",
	/* Axes Lines */
	"/AL { stroke mylinewidth 2 div setlinewidth } def\n",
	/* Plot Lines */
	"/PL { stroke mylinewidth setlinewidth } def\n",
	/* Line Types */
	"/LTa { AL [1 dl 2 dl] 0 setdash 0 0 0 setrgbcolor } def\n", /* axes */
	"/LTb { BL [] 0 0 0 DL } def\n", /* border */
	"/LT0 { PL [] 0 1 0 DL } def\n",
	"/LT1 { PL [4 dl 2 dl] 0 0 1 DL } def\n",
	"/LT2 { PL [2 dl 3 dl] 1 0 0 DL } def\n",
	"/LT3 { PL [1 dl 1.5 dl] 1 0 1 DL } def\n",
	"/LT4 { PL [5 dl 2 dl 1 dl 2 dl] 0 1 1 DL } def\n",
	"/LT5 { PL [4 dl 3 dl 1 dl 3 dl] 1 1 0 DL } def\n",
	"/LT6 { PL [2 dl 2 dl 2 dl 4 dl] 0 0 0 DL } def\n",
	"/LT7 { PL [2 dl 2 dl 2 dl 2 dl 2 dl 4 dl] 1 0.3 0 DL } def\n",
	"/LT8 { PL [2 dl 2 dl 2 dl 2 dl 2 dl 2 dl 2 dl 4 dl] 0.5 0.5 0.5 DL } def\n",
	"/M {moveto} def\n",
	"/L {lineto} def\n",
	/* Point */
	"/P { stroke [] 0 setdash\n",
	"  currentlinewidth 2 div sub moveto\n",
	"  0 currentlinewidth rlineto  stroke } def\n",
	/* Diamond */
	"/D { stroke [] 0 setdash  2 copy  vpt add moveto\n",
	"  hpt neg vpt neg rlineto  hpt vpt neg rlineto\n",
	"  hpt vpt rlineto  hpt neg vpt rlineto  closepath  stroke\n",
	"  P  } def\n",
	/* Plus (Add) */
	"/A { stroke [] 0 setdash  vpt sub moveto  0 vpt2 rlineto\n",
	"  currentpoint stroke moveto\n",
	"  hpt neg vpt neg rmoveto  hpt2 0 rlineto stroke\n",
	"  } def\n",
	/* Box */
	"/B { stroke [] 0 setdash  2 copy  exch hpt sub exch vpt add moveto\n",
	"  0 vpt2 neg rlineto  hpt2 0 rlineto  0 vpt2 rlineto\n",
	"  hpt2 neg 0 rlineto  closepath  stroke\n",
	"  P  } def\n",
	/* Cross */
	"/C { stroke [] 0 setdash  exch hpt sub exch vpt add moveto\n",
	"  hpt2 vpt2 neg rlineto  currentpoint  stroke  moveto\n",
	"  hpt2 neg 0 rmoveto  hpt2 vpt2 rlineto stroke  } def\n",
	/* small Cross */
	"/c { stroke [] 0 setdash  exch hpt23 sub exch vpt23 add moveto\n",
	"  hpt43 vpt43 neg rlineto  currentpoint  stroke  moveto\n",
	"  hpt43 neg 0 rmoveto  hpt43 vpt43 rlineto stroke  } def\n",
	/* Triangle */
	"/T { stroke [] 0 setdash  2 copy  vpt 1.12 mul add moveto\n",
	"  hpt neg vpt -1.62 mul rlineto\n",
	"  hpt 2 mul 0 rlineto\n",
	"  hpt neg vpt 1.62 mul rlineto  closepath  stroke\n",
	"  P  } def\n",
	/* Star */
	"/S { 2 copy A c} def\n", /*960603 changed 'C' to 'c' */
	NULL
}; /*char *PS_header[]*/

#define POINTSPERINCH   72

#define PS_SC (10)				/* scale is 1pt = 10 units */

#define PS_XOFF	50	/* page offset in pts */
#define PS_YOFF	250

#define PS_XMAX (8*POINTSPERINCH*PS_SC)    /* 8" */
#define PS_YMAX (10*POINTSPERINCH*PS_SC)   /* 10" */

#define PS_XMAXP (PS_XMAX-2*PS_SC*PS_XOFF) /* for portrait mode */
#define PS_YMAXP (5*POINTSPERINCH*PS_SC)   /* 5" for portrait mode */

#if (1)
#define PS_XMAXL PS_YMAX /* for landscape mode */
#define PS_YMAXL PS_XMAX /* for landscape mode */
#else
#define PS_XMAXL (PS_YMAX-2*PS_SC*PS_XOFF) /* for landscape mode */
#define PS_YMAXL PS_XMAXP  /* for landscape mode */
#endif

#define PS_XLAST (PS_XMAX - 1)
#define PS_YLAST (PS_YMAX - 1)

#define PS_VTIC (PS_XMAX/80)
#define PS_HTIC (PS_XMAX/80)

#define	PS_LW (0.5*PS_SC)		/* linewidth = 0.5 pts */

#define PS_VCHAR (PS_FONTSIZE*PS_SC)		/* default is PS_FONTSIZE point characters */
#define PS_HCHAR (PS_FONTSIZE*PS_SC*6/10)


#ifdef DO_PS_OPTIONS

void PS_options(void)
{
	extern struct value *const_express();
	extern double real();

	if (!END_OF_COMMAND)
	{
		if (almost_equals(c_token,"p$ortrait"))
		{
			ps_portrait=TRUE;
			c_token++;
		}
		else if (almost_equals(c_token,"l$andscape"))
		{
			ps_portrait=FALSE;
			c_token++;
		}
		else if (almost_equals(c_token,"d$efault"))
		{
			ps_portrait=FALSE;
			ps_color=FALSE;
			strcpy(ps_font,"Courier");
			ps_fontsize = PS_FONTSIZE;
			c_token++;
		}
	}

	if (!END_OF_COMMAND)
	{
		if (almost_equals(c_token,"m$onochrome"))
		{
			ps_color=FALSE;
			c_token++;
		}
		else if (almost_equals(c_token,"c$olor"))
		{
			ps_color=TRUE;
			c_token++;
		}
	}

	if (!END_OF_COMMAND && isstring(c_token))
	{
		quote_str(ps_font,c_token);
		c_token++;
	}

	if (!END_OF_COMMAND)
	{
		/* We have font size specified */
		struct value a;
		ps_fontsize = (int)real(const_express(&a));
		c_token++;
		term_tbl[term].v_char = (unsigned int)(ps_fontsize*PS_SC);
		term_tbl[term].h_char = (unsigned int)(ps_fontsize*PS_SC*6/10);
	}

	sprintf(term_options,"%s %s \"%s\" %d",
		ps_portrait ? "portrait" : "landscape",
		ps_color ? "color" : "monochrome",ps_font,ps_fontsize);
} /*PS_options()*/
#endif /*DO_PS_OPTIONS*/


int PS_init(void)
{
	struct termentry *t = &term_tbl[term];
	int i;
	double      xscale = xsize/PS_SC, yscale = ysize/PS_SC;
	int         left, bottom, right, top;
	char       *firstLine;
	WHERE("PS_init");
	
	t->v_char = (unsigned int)(ps_fontsize*PS_SC);
	t->h_char = (unsigned int)(ps_fontsize*PS_SC*6/10);
	t->v_tic  = PS_VTIC;
	t->h_tic  = PS_HTIC;

	if (PutProlog >= 0)
	{ /* non-EPSF*/
		ps_xoff = PS_XOFF;
		ps_yoff = PS_YOFF;
		t->xmax   = (ps_portrait) ? PS_XMAXP : PS_XMAXL;
#if (1) /*change of 960528*/
		t->ymax   = t->xmax/ps_aspect + .5;
#else
		t->ymax   = (ps_portrait) ? PS_YMAXP : PS_YMAXL;
#endif
		left = ps_xoff;
		bottom = ps_yoff;
		right = left + (int) (0.5 + (ps_portrait) ?
				xscale*t->xmax : yscale*t->ymax);
		top = bottom + (int) (0.5 + (ps_portrait) ?
				yscale*t->ymax : xscale*t->xmax);
		firstLine = "%!PS-Adobe-2.0";
	}
	else
	{ /* EPSF file*/
#ifdef MACINTOSH
		ps_xoff = 0;
		ps_yoff = 0;
		t->xmax = PS_SC*(MacPicFrame.right - MacPicFrame.left);
		t->ymax = PS_SC*(MacPicFrame.bottom - MacPicFrame.top);
		left = ps_xoff;
		bottom = ps_yoff;
		right = left + (int) (0.5 + xscale*t->xmax);
		top = bottom + (int) (0.5 + yscale*t->ymax);
		firstLine = "%!PS-Adobe-2.0 EPSF-1.2";
#endif /*MACINTOSH*/
	}

	outfile = PlotFile;
	if(PutProlog)
	{
		if (PutProlog < 2)
		{
			ps_page = 0;
		}
		
		fprintf(outfile, "%s\n", firstLine);
		fprintf(outfile,"%%%%Creator: MacAnova/gnuplot\n");
		fprintf(outfile,"%%%%DocumentFonts: %s\n", ps_font);
		fprintf(outfile,"%%%%BoundingBox: %ld %ld %ld %ld\n",
				(long) left, (long) bottom,(long) right, (long) top);
		fprintf(outfile,"%%%%Pages: (atend)\n");
		fprintf(outfile,"%%%%EndComments\n");
		/* 970512 changed 40 in following to 80 to make it work for VMS*/
		fprintf(outfile,"/gnudict 80 dict def\ngnudict begin\n");
		fprintf(outfile,"/Color %s def\n",ps_color ? "true" : "false");
		fprintf(outfile,"/gnulinewidth %.3f def\n",PS_LW);
		fprintf(outfile,"/vshift %d def\n", (int)(t->v_char)/(-3));
		fprintf(outfile,"/dl {%d mul} def\n",PS_SC); /* dash length */
		fprintf(outfile,"/hpt %.1f def\n",PS_HTIC/2.0);
		fprintf(outfile,"/vpt %.1f def\n",PS_VTIC/2.0);
		for ( i=0; PS_header[i] != NULL; i++)
		{
			fprintf(outfile,"%s",PS_header[i]);
		}
		fprintf(outfile,"end\n");
		fprintf(outfile,"%%%%EndProlog\n");
	}
	return(0);
} /*PS_init()*/


int PS_graphics(void)
{
	struct termentry *t = &term_tbl[term];
	double            xscale = xsize/PS_SC, yscale = ysize/PS_SC;

	ps_page++;
	fprintf(outfile,"%%%%Page: %d %d\n",ps_page,ps_page);
	fprintf(outfile,"gnudict begin\n");
	fprintf(outfile,"gsave\n");
	if (ps_portrait)
	{
		fprintf(outfile,"%d %d translate\n",ps_xoff,ps_yoff);
	    fprintf(outfile,"%.3f %.3f scale\n", xscale, yscale);
	}
	else
	{
	    fprintf(outfile,"%.3f %.3f scale\n", yscale, xscale);
	    fprintf(outfile,"90 rotate\n");
#if (1) /* change of 960528*/
		fprintf(outfile,"%d %d translate\n",
				PS_SC*35*POINTSPERINCH/100 ,-(PS_XMAX) + PS_SC*ps_xoff);
#else
		fprintf(outfile,"%d %d translate\n", 0 ,-(PS_XMAX));
#endif
	}
	fprintf(outfile,"0 setgray\n");
	fprintf(outfile,"/%s findfont %ld ", ps_font, (t->v_char) );
	fprintf(outfile,"scalefont setfont\n");
	fprintf(outfile,"newpath\n");
	ps_path_count = 0;

	return(0);
} /*PS_graphics()*/


void PS_text(void)
{
	struct termentry *t = term_tbl + term;

	if (ScreenDumpFile != (char *) 0)
	{
		ScreenDumpError = SCREENDUMPUNDEFINED;
	}
	ps_path_count = 0;
	fprintf(outfile,"stroke\ngrestore\nend\nshowpage\n");
	
	t->reset();
} /*PS_text()*/


void PS_reset(void)
{
	fprintf(outfile,"%%%%Trailer\n");
	fprintf(outfile,"%%%%Pages: %d\n",ps_page);
} /*PS_reset()*/


void PS_linetype(long linetype)
{
	char *line = "aaab012345678";
	long         heavy = (10*linetype >= UNITWIDTH);
	double       thickness = (heavy) ? (double) linetype / (double) UNITWIDTH: 1.0;

	if(heavy)
	{
		linetype %= MAXLINETYPE;
	}

	if(linetype < 0)
	{
		linetype = (linetype < MINLINETYPE) ? BORDERLINETYPE : linetype;
	}
	else
	{
		linetype %= 9;
	}
	fprintf(outfile,"%5.3f LW\n",thickness);
	fprintf(outfile,"LT%c\n", line[linetype - MINLINETYPE]);
	ps_path_count = 0;
} /*PS_linetype()*/


void PS_move(unsigned long x, unsigned long y)
{
	fprintf(outfile,"%ld %ld M\n", x, y);
	ps_path_count += 1;
} /*PS_move()*/


void PS_vector(unsigned long x, unsigned long y)
{
	fprintf(outfile,"%ld %ld L\n", x, y);
	ps_path_count += 1;
	if (ps_path_count >= 400)
	{
		fprintf(outfile,"currentpoint stroke moveto\n");
		ps_path_count = 0;
	}
} /*PS_vector()*/


void PS_str_text(char str [])
{
	char       ch;

	putc('(',outfile);
	ch = *str++;
	while(ch!='\0')
	{
		if ( (ch=='(') || (ch==')') || (ch=='\\') )
		{
			putc('\\',outfile);
		}
		putc(ch,outfile);
		ch = *str++;
	} /*while(ch!='\0')*/
	switch(ps_justify)
	{
	  case LEFT :
		fprintf(outfile,") Lshow\n");
		break;
	  case CENTRE :
		fprintf(outfile,") Cshow\n");
		break;
	  case RIGHT :
		fprintf(outfile,") Rshow\n");
		break;
	} /*switch(ps_justify)*/
	ps_path_count = 0;
} /*PS_str_text()*/

void PS_chr_text(char ch)
{
	char str[2];

	str[0] = ch;
	str[1] = '\0';
	PS_str_text(str);
} /*PS_chr_text()*/

#ifdef GNUPLOT3
PS_put_text(unsigned int x, unsigned int y, char * str)
{
	char         ch;
	PS_move(x,y);
	if (ps_ang != 0)
	{
		fprintf(outfile,"currentpoint gsave translate %d rotate 0 0 moveto\n"
				,ps_ang*90);
	}
	putc('(',outfile);
	ch = *str++;
	while(ch!='\0')
	{
		if ( (ch=='(') || (ch==')') || (ch=='\\') )
			putc('\\',outfile);
		putc(ch,outfile);
		ch = *str++;
	}
	switch(ps_justify)
	{
	  case LEFT :
		fprintf(outfile,") Lshow\n");
		break;
	  case CENTRE :
		fprintf(outfile,") Cshow\n");
		break;
	  case RIGHT :
		fprintf(outfile,") Rshow\n");
		break;
	}
	if (ps_ang != 0)
	{
		fprintf(outfile,"grestore\n");
	}
	ps_path_count = 0;
} /*PS_put_text()*/

int PS_text_angle(int ang)
{
	ps_ang=ang;
	return TRUE;
} /*PS_text_angle()*/

int PS_justify_text(enum JUSTIFY mode)
{
	ps_justify=mode;
	return TRUE;
} /*PS_justify_text()*/


#endif /*GNUPLOT3*/
/* postscript point routines */
void PS_point(long x, long y, long number)
{
	/*diamond,plus,box,cross,triangle,star,dot,small cross*/
	char *point = "DABCTSPc";
	number %= POINT_TYPES;
 	if (number < 0)
	{
		number = DOTPOINT;			/* negative types are all 'dot' */
	}
	fprintf(outfile,"%ld %ld %c\n", x, y, point[number]);
	ps_path_count = 0;
} /*PS_point()*/

#endif /*POSTSCRIPT*/


#ifdef TEK
/*	For Tektronix 4014 terminal */
#if (0)
#define TEK40XMAX 1024
#define TEK40YMAX 780
#endif /*0*/

#define TEK40XMAX 974
#define TEK40YMAX 780

#define TEK40XLAST (TEK40XMAX - 1)
#define TEK40YLAST (TEK40YMAX - 1)

#define TEK40VCHAR		25
#define TEK40HCHAR		14
#define TEK40VTIC		14 /* was 11, kb*/
#define TEK40HTIC		14 /* was 11, kb*/

#define HX 0x20			/* bit pattern to OR over 5-bit data */
#define HY 0x20
#define LX 0x40
#define LY 0x60

#define LOWER5 31
#define UPPER5 (31<<5)

int            TEK40init(void);
int            TEK40graphics(void);
void           TEK40text(void);
void           TEK40reset(void);
void           TEK40linetype(long /*linetype*/);
void           TEK40move(unsigned long /*x*/, unsigned long /*y*/);
void           TEK40vector(unsigned long /*x*/, unsigned long /*y*/);
void           TEK40str_text(char * /*str*/);
void           TEK40chr_text(char /*c*/);

static char *Terminal = (char *) 0;

int TEK40init(void)
{
	struct termentry *t = term_tbl + term;

	t->v_char = TEK40VCHAR;
	t->h_char = TEK40HCHAR;
	t->v_tic  = TEK40VTIC;
	t->h_tic  = TEK40HTIC;
	t->xmax   = TEK40XMAX;
	t->ymax   = TEK40YMAX;

	if(Terminal == '\0')
	{
		Terminal = TERMINAL;
	}

	return (0);
} /*TEK40init()*/


int TEK40graphics(void)
{
	WHERE("TEK40graphics");

	if(PLOTFILE == STDOUT && Terminal != (char *) 0 &&
	   strcmp(Terminal,"xterm") == 0)
	{ /* switch on xterm tek window */
		fprintf(stdout,"\033\133\077\063\070\150\033\070");
	}

	fprintf(PlotFile,"\033\014");
	return (0);
	/*                   1
		1. clear screen
	*/
} /*TEK40graphics()*/


void TEK40text(void)
{
	struct termentry *t = term_tbl + term;

	TEK40move(0L, 24L);
	fprintf(PlotFile,"\037");
	if (ScreenDumpFile != (char *) 0)
	{
		ScreenDumpError = SCREENDUMPUNDEFINED;
	}
	if (PLOTFILE == STDOUT && (ISATTY & ITTYIN) && PauseAfterPlot)
	{
		getchar();				/* wait for CR */
	}
	t->reset();

	/*                   1
		1. into alphanumerics
	*/
} /*TEK40text()*/


void TEK40linetype(long linetype)
{
	linetype = (linetype < 0) ? 0 : (linetype % UNITWIDTH) % 5;
	fprintf(PlotFile,
			"\035\033%c", '\140' + (int) linetype);
} /*TEK40linetype()*/


void TEK40move(unsigned long x, unsigned long y)
{
	putc('\035', PlotFile);	/* into graphics */
	TEK40vector(x, y);
} /*TEK40move()*/


void TEK40vector(unsigned long x, unsigned long y)
{
	FILE     *plotfile = PlotFile;

	putc((HY | (y & UPPER5) >> 5), plotfile);
	putc((LY | (y & LOWER5)), plotfile);
	putc((HX | (x & UPPER5) >> 5), plotfile);
	putc((LX | (x & LOWER5)), plotfile);
} /*TEK40vector()*/


void TEK40str_text(char str [])
{
	fprintf(PlotFile,"\037%s", str);
} /*TEK40str_text()*/


void TEK40chr_text(char chr)
{
	fprintf(PlotFile,"\037%c", chr);
} /*TEK40chr_text(*/

void TEK40reset(void)
{
	if (PLOTFILE == STDOUT && strcmp(Terminal,"xterm") == 0)
	{/* switch back to normal vt100 window */
		fprintf(PlotFile,"\033\003");
	}
} /*TEK40reset()*/

#endif /*TEK*/

#ifdef DUMB
/* For character based printer plot */
/* Modified version of code from GNUPLOT - dumb.trm */
/*
 * Copyright (C) 1991
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the modified code.  Modifications are to be distributed
 * as patches to released version.
 *
 * This software  is provided "as is" without express or implied warranty.
 *
 * This file is included by ../term.c.
 *
 * This terminal driver supports:
 *   DUMB terminals
 *
 * AUTHORS
 *   Francois Pinard, 91-04-03
 *           INTERNET: pinard@iro.umontreal.ca
 * Modified for use in MacAnova by Christopher Bingham, kb@umnstat.stat.umn.edu
 * send your comments or suggestions to (pixar!info-gnuplot@sun.com).
 *
 */

#ifdef ASSERTIONS
#define DUMBassert(X,Y)  	assert((X) >= 0);\
							assert((X) < dumb_xmax);\
							assert((Y) >= 0);\
							assert((Y) < dumb_ymax)
#endif /*ASSERTIONS*/

#define DUMB_AXIS_CONST '\1'
#define DUMB_BORDER_CONST '\2'

#define DUMB_XMAX 79
#define DUMB_YMAX 24
#define DUMB_VCHAR 1
#define DUMB_HCHAR 1
#define DUMB_VTIC  0
#define DUMB_HTIC  0

void           dumb_set_pixel(long /*x*/,long /*y*/,long /*v*/,long /*p*/);

int            DUMB_init(void);
int            DUMB_graphics(void);
void           DUMB_text(void);
void           DUMB_reset(void);
void           DUMB_linetype(long /*linetype*/);
void           DUMB_move(unsigned long /*x*/, unsigned long /*y*/);
void           DUMB_point(long /*x*/, long /*y*/, long /*point*/);
void           DUMB_vector(unsigned long /*x*/, unsigned long /*y*/);
void           DUMB_str_text(char * /*str*/);
void           DUMB_chr_text(char /*c*/);

#define Trash  NAMEFORTRASH
#define GDUMBMATRIX   0
#define GDUMBPRIORITY 1
#define NTRASH        2

static Symbolhandle Trash = (Symbolhandle) 0;
static char **dumb_matrix = (char **) 0;    /* matrix of characters */
static char **dumb_priority = (char **) 0;/* matrix of priority at each position */

static char dumb_pen;                /* current character used to draw */
static unsigned long dumb_x;                    /* current X position */
static unsigned long dumb_y;                    /* current Y position */
static unsigned long dumb_xmax = DUMB_XMAX;
static unsigned long dumb_ymax = DUMB_YMAX;

#define DUMB_PIXEL(x,y) (*dumb_matrix)[dumb_xmax*(y)+(x)]

void dumb_set_pixel(long x, long y, long v, long p)
{
	/* let's play it safe */
	if (x >= 0 && x < dumb_xmax && y >= 0 && y < dumb_ymax)
	{
		long      j = dumb_xmax*y+x;
		
#ifdef DUMBassert
		DUMBassert(x,y);
#endif /*DUMBassert*/

		if (p >= (*dumb_priority)[j]) /* kb changed from > */
		{
			(*dumb_matrix)[j] = v;
			(*dumb_priority)[j] = p;
		}
	} /*if (x >= 0 && x < dumb_xmax && y >= 0 && y < dumb_ymax)*/	
} /*dumb_set_pixel()*/

int DUMB_init(void)
{
	struct termentry *t = term_tbl + term;
	long npixels;
	WHERE("DUMB_init");

	Trash = GarbInstall(NTRASH);
	if(Trash == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	t->v_char = DUMB_VCHAR;
	t->h_char = DUMB_HCHAR;
	t->v_tic  = DUMB_VTIC;
	t->h_tic  = DUMB_HTIC;

	/* 970108 changed to used Globals */
	t->xmax = dumb_xmax = DumbWidth - 1;
	t->ymax = dumb_ymax = DumbHeight;

	npixels = dumb_xmax * dumb_ymax;
	if(!getScratch(dumb_matrix,GDUMBMATRIX,npixels + 1,char) ||
	   !getScratch(dumb_priority,GDUMBPRIORITY,npixels,char))
	{
		goto errorExit;
	}
	mylockhandle(dumb_matrix);
	mylockhandle(dumb_priority);

	return (0);

  errorExit:
	emptyTrash();
	Trash = (Symbolhandle) 0;
	return (1);

} /*DUMB_init()*/


int DUMB_graphics (void)
{
	long npixels = dumb_xmax * dumb_ymax, i;

	for(i=0;i<npixels;i++)
	{
		(*dumb_matrix)[i] = ' ';
		(*dumb_priority)[i] = 0;
	}
	(*dumb_matrix)[npixels] = '\0';
	return (0);
} /*DUMB_graphics ()*/


void DUMB_text (void)
{
	long              y, l;
	char              ctemp;
	struct termentry *t = term_tbl + term;

	NLINES = -2;
	for (y = dumb_ymax - 1; y >= 0; y--)
    {
		/* trim off trailing spaces */
		for (l = dumb_xmax; l > 0 && DUMB_PIXEL (l - 1, y) == ' '; l--)
		{
			;
		}
		ctemp = DUMB_PIXEL(l,y);
		DUMB_PIXEL(l,y) = '\0'; /* close off line */
		fmyprint((*dumb_matrix) + y*dumb_xmax,PLOTFILE);
		DUMB_PIXEL(l,y) = ctemp;

		if(PLOTFILE != STDOUT || y > 0)
		{
			fmyeol(PLOTFILE);
		}
		if(INTERRUPT != INTNOTSET)
		{
			break;
		}
    } /*for (y = dumb_ymax - 1; y >= 0; y--)*/

	if(PLOTFILE == STDOUT) /* standard output in MacAnova */
	{
#if !( defined(MACINTOSH) || defined(WXWIN) )
		if(ISATTY & ITTYIN)
		{
			if(INTERRUPT == INTNOTSET && PauseAfterPlot)
			{
				(void) getchar(); /* wait for CR */
			}
			else
			{
				myeol();
			}
			if (SPOOLFILE != (FILE *) 0)
			{ /* make sure spool file gets new line */
				fmyeol(SPOOLFILE);
			}
		}
		else
		{
			myeol();
		}
#else /*!( defined(MACINTOSH) || defined(WXWIN) )*/
		myeol();
#endif /*!( defined(MACINTOSH) || defined(WXWIN) )*/
		NLINES = 0;
	} /*if(PLOTFILE == STDOUT)*/
	t->reset();
} /*DUMB_text()*/


void DUMB_reset(void)
{
	WHERE("DUMB_reset");

	emptyTrash();
	Trash = (Symbolhandle) 0;
} /*DUMB_reset()*/


static char pen_type[] =
{
	DUMB_BORDER_CONST, DUMB_AXIS_CONST, DUMB_AXIS_CONST, DUMB_BORDER_CONST, '.', '-', '=', 'o', ':'
};

#define NLINETYPE (sizeof(pen_type)/sizeof(char) - 4)
void DUMB_linetype(long linetype)
{
	if(linetype < MINLINETYPE)
	{
		linetype = BORDERLINETYPE;
	}
	linetype = ((linetype < 0) ?
				linetype : (linetype % MAXLINETYPE) % NLINETYPE) -
				MINLINETYPE;
	dumb_pen = pen_type[linetype];
} /*DUMB_linetype()*/


void DUMB_move(unsigned long x, unsigned long y)
{
	WHERE("DUMB_move");

	dumb_x = x;
	dumb_y = y;
} /*DUMB_move()*/


#define DOT     '.'

/* modified by kb.  Original always used point % 26 + 'A' unless point = -1 */
/* diamond, plus, box, cross, triangle, star, dot */
#if (1) /* new (960531) set of plotting characters */
static    char point_chars[] = { 'o', '+', '#', 'X', '%', '*', '.', 'x'};
#else
static    char point_chars[] = { '+', 'O', 'X', '%', '#', '*', '.'};
#endif
void DUMB_point(long x, long y, long point)
{
	long      c;

	if (point > 26)
	{
		c  = point % 26 + 'A';
	}
	else
	{
		if (point < 0)
		{
			point = DOTPOINT;
		}
		c = point_chars[point % POINT_TYPES];
	}
#ifdef DUMBassert
	DUMBassert(x,y);
#endif /*DUMBassert*/
	dumb_set_pixel (x, y, c, 4L);
} /*DUMB_point()*/

#define CORNER       '+'
#define HORIZBORDER  '-'
#define VERTBORDER   '|'
#define HORIZAXIS    '.'
#define VERTAXIS     ':'

void DUMB_vector(unsigned long x, unsigned long y)
{
	char      pen, pen1;
	long      priority;
	long      xtmp,ytmp;
	long      x1 = (long) dumb_x, y1 = (long) dumb_y; /* start position */
	long      x2 = (long) x, y2 = (long) y; /*end position */
	long      delta, dx = x2 - x1, dy = y2 - y1;
	double    ratio;
	WHERE("DUMB_vector");

	if (labs (dy) > labs (dx))
    {
		switch (dumb_pen)
		{
		  case DUMB_AXIS_CONST:
			pen = VERTAXIS;
			pen1 = CORNER;
			priority = 1;
			break;

		  case DUMB_BORDER_CONST:
			pen = VERTBORDER;
			pen1 = CORNER;
			priority = 2;
			break;

		  default:
			pen = dumb_pen;
			pen1 = pen;
			priority = 3;
			break;

		} /*switch (dumb_pen)*/
#ifdef DUMBassert
		DUMBassert(x1,y1);
#endif /*DUMBassert*/
		dumb_set_pixel (x1, y1, pen1, priority);
		ratio = (double) dx/(double) labs(dy);
		for (delta = 1; delta < labs (dy); delta++)
		{
			xtmp = (long) (x1 + delta*ratio + 0.5);
			ytmp = y1 + delta * ((y2 > y1) ? 1 : -1);
#ifdef DUMBassert
			DUMBassert(xtmp,ytmp);
#endif /*DUMBassert*/
			dumb_set_pixel (xtmp,ytmp,pen, priority);
		}
#ifdef DUMBassert
		DUMBassert(x2,y2);
#endif /*DUMBassert*/
		dumb_set_pixel (x2, y2, pen1, priority);
    } /*if (labs (dy) > labs (dx))*/
	else if (labs (dx) > labs (dy))
    {
		switch (dumb_pen)
		{
		  case DUMB_AXIS_CONST:
			pen = HORIZAXIS;
			pen1 = CORNER;
			priority = 1;
			break;

		  case DUMB_BORDER_CONST:
			pen = HORIZBORDER;
			pen1 = CORNER;
			priority = 2;
			break;

		  default:
			pen = dumb_pen;
			pen1 = pen;
			priority = 3;
			break;
		} /*switch (dumb_pen)*/
#ifdef DUMBassert
		DUMBassert(x1,y1);
#endif /*DUMBassert*/
		dumb_set_pixel (x1, y1, pen1, priority);
		ratio = (double) dy/(double) labs(dx);
		for (delta = 1; delta < labs (dx); delta++)
		{
			xtmp = x1 + delta * ((x2 > x1) ? 1 : -1);
			ytmp = (long) (y1 + delta*ratio + 0.5);
#ifdef DUMBassert
			DUMBassert(xtmp,ytmp);
#endif /*DUMBassert*/
			dumb_set_pixel (xtmp, ytmp,	pen, priority);
		} /*for (delta = 1; delta < labs (dx); delta++)*/
#ifdef DUMBassert
		DUMBassert(x2,y2);
#endif /*DUMBassert*/
		dumb_set_pixel (x2, y2, pen1, priority);
	} /*if (labs (dy) > labs (dx)){}else if (labs (dx) > labs (dy))*/
	else
    { /* labs (dx) == labs (dy) */
		switch (dumb_pen)
		{
		  case DUMB_AXIS_CONST:	/* zero length axis */
			pen = CORNER;
			priority = 1;
			break;

		  case DUMB_BORDER_CONST: /* zero length border */
			pen = CORNER;
			priority = 2;
			break;

		  default:
			pen = dumb_pen;
			pen1 = pen;
			priority = 3;
			break;
		} /*switch (dumb_pen)*/

		for (delta = 0; delta <= labs (dx); delta++)
		{
			xtmp = (long) (x1 + delta * sign (dx) + 0.5);
			ytmp = (long) (y1 + delta * sign (dy) + 0.5);
#ifdef DUMBassert
			DUMBassert(xtmp,ytmp);
#endif /*DUMBassert*/
			dumb_set_pixel (xtmp, ytmp,	pen, priority);
		}/*for (delta = 0; delta <= labs (dx); delta++)*/
    }
	dumb_x = x;
	dumb_y = y;
} /*DUMB_vector()*/


void DUMB_str_text(char * str)
{
	long length;
	long x = (long) dumb_x, y = (long) dumb_y;/* correct vertical position */
	WHERE("DUMB_str_text");

	if (ScreenDumpFile != (char *) 0)
	{
		ScreenDumpError = SCREENDUMPUNDEFINED;
	}
	length = strlen(str);
	if (x + length > dumb_xmax)
	{
		x = (dumb_xmax > length) ? dumb_xmax - length : 0 ;
	}

	for (; x < dumb_xmax && *str; x++, str++)
	{
#ifdef DUMBassert
		DUMBassert(x,y);
#endif /*DUMBassert*/
		dumb_set_pixel (x, y, *str, 5L);
	}
} /*DUMB_str_text()*/


void DUMB_chr_text (char c)
{
	long     x = (long) dumb_x, y = (long) dumb_y;

#ifdef DUMBassert
	DUMBassert(x,y);
#endif /*DUMBassert*/
	dumb_set_pixel (x, y, c, 5L);
} /*DUMB_chr_text()*/

#endif /*DUMB*/

static void UNKNOWN(void)
{
} /*UNKNOWN()*/

#if (0)
int UNKNOWN_nullint(void)
{
	return (0);
} /*UNKNOWN_nullint()*/

void UNKNOWN_nullvoid(void)
{

} /*UNKNOWN_nullvoid()*/
#endif

void do_point(long x, long y, long number)
{
	register long   htic, vtic, htic23, vtic23;
	register struct termentry *t;

 	if (number < 0)
	{
		number = DOTPOINT;			/* negative types are all 'dot' */
	}
	number %= POINT_TYPES;
	t = &term_tbl[term];
	htic = (t->h_tic / 2);		/* should be in term_tbl[] in later version */
	vtic = (t->v_tic / 2);
	htic23 = 2*htic/3; /*kb*/
	vtic23 = 2*vtic/3; /*kb*/
	/*
	  if ( x < t->h_tic || y < t->v_tic || x >= t->xmax-t->h_tic ||
	  	y >= t->ymax-t->v_tic )
	  return;
	*/		/* add clipping in later version maybe */

	switch ((int) number)
	{
	  case DOTPOINT: /* dot */
		(*t->move) (x, y);
		(*t->vector) (x, y);
		break;

	  case DIAMONDPOINT:					/* do diamond */
		(*t->move) (x - htic, y);
		(*t->vector) (x, y - vtic);
		(*t->vector) (x + htic, y);
		(*t->vector) (x, y + vtic);
		(*t->vector) (x - htic, y);
		(*t->move) (x, y);
		(*t->vector) (x, y);
		break;

	  case PLUSPOINT:					/* do plus */
		(*t->move) (x - htic, y);
		(*t->vector) (x - htic, y);
		(*t->vector) (x + htic, y);
		(*t->move) (x, y - vtic);
		(*t->vector) (x, y - vtic);
		(*t->vector) (x, y + vtic);
		break;

	  case BOXPOINT:					/* do box */
		(*t->move) (x - htic, y - vtic);
		(*t->vector) (x + htic, y - vtic);
		(*t->vector) (x + htic, y + vtic);
		(*t->vector) (x - htic, y + vtic);
		(*t->vector) (x - htic, y - vtic);
		(*t->move) (x, y);
		(*t->vector) (x, y);
		break;

	  case CROSSPOINT:					/* do X */
		(*t->move) (x - htic, y - vtic);
		(*t->vector) (x - htic, y - vtic);
		(*t->vector) (x + htic, y + vtic);
		(*t->move) (x - htic, y + vtic);
		(*t->vector) (x - htic, y + vtic);
		(*t->vector) (x + htic, y - vtic);
		break;

	  case TRIANGLEPOINT:					/* do triangle */
#if (0) /*kb slightly different shaped triangle*/
		(*t->move) (x, y + vtic);
		(*t->vector) (x - htic, y - vtic);
		(*t->vector) (x + htic, y - vtic);
		(*t->vector) (x, y + vtic);
#else
		(*t->move) (x, y + 2*vtic23);
		(*t->vector) (x - 2*htic23, y - vtic23);
		(*t->vector) (x + 2*htic23, y - vtic23);
		(*t->vector) (x, y + 2*vtic23);
#endif
		(*t->move) (x, y);
		(*t->vector) (x, y);
		break;

	  case STARPOINT:					/* do star */
		(*t->move) (x - htic, y);
		(*t->vector) (x - htic, y);
		(*t->vector) (x + htic, y);
		(*t->move) (x, y - vtic);
		(*t->vector) (x, y - vtic);
		(*t->vector) (x, y + vtic);
#if (1)
		(*t->move) (x - htic23, y - vtic23);  /*kb: make diagonals shorter*/
		(*t->vector) (x - htic23, y - vtic23);
		(*t->vector) (x + htic23, y + vtic23);
		(*t->move) (x - htic23, y + vtic23);
		(*t->vector) (x - htic23, y + vtic23);
		(*t->vector) (x + htic23, y - vtic23);
#else /* original code */
		(*t->move) (x - htic, y - vtic);
		(*t->vector) (x - htic, y - vtic);
		(*t->vector) (x + htic, y + vtic);
		(*t->move) (x - htic, y + vtic);
		(*t->vector) (x - htic, y + vtic);
		(*t->vector) (x + htic, y - vtic);
#endif
		break;

	  case SMALLCROSSPOINT:     /* do x */
		(*t->move) (x - htic23, y - vtic23);
		(*t->vector) (x - htic23, y - vtic23);
		(*t->vector) (x + htic23, y + vtic23);
		(*t->move) (x - htic23, y + vtic23);
		(*t->vector) (x - htic23, y + vtic23);
		(*t->vector) (x + htic23, y - vtic23);
		break;
		
	} /*switch ((int) number)*/
} /*do_point()*/


/* 
   Print a horizontal string centered at x,y
   980714 changed justify to enum JUSTIFY instead of long
*/
void horstring(long x, long y, char * str, enum JUSTIFY justify)
{
	struct termentry *t;
	long              left;

	if(*str)
	{
		t = &term_tbl[term];
		/* attempt to center character vertically on y */
		if(term == PSTERM)
		{
			y += PS_SC; /* move up 1 point */
		}
		else if(term != DUMBTERM &&
		        (
#ifdef EPSFPLOTSOK
				 term == PREVIEWTERM ||
#endif /*EPSFPLOTSOK*/
				 term == STANDARDTERM || term == FILETERM))
		{
#ifdef TEK
			y -= (t->v_char)/2 - 4;
#else
			y -= (t->v_char)/2 - 1;
#endif
		}

		if(y >= 0 && y < t->ymax)
		{
			if(justify == CENTRE)
			{
				left = x - (strlen(str) * (t->h_char)) / 2;
			}
			else if(justify == LEFT)
			{
				left = x;
			}
			else
			{
				left = x - strlen(str)*t->h_char;
			}

			if (left < 1)
			{
				left = 1;
			}
			if (left + strlen(str) * t->h_char > t->xmax - 1)
			{
				left = t->xmax - 1 - strlen(str) * t->h_char;
			}
			if (left < 1)
			{
				left = 1;
			}
			(*t->move) (left, y);
			(*t->str_text) (str);
		} /*if(y >= 0 && y < t->ymax)*/
	} /*if(*str)*/
} /*horstring()*/

/* print a vertical string centered at x.y */
void vertstring(long x, long y, char *str)
{
	struct termentry *t;
	long            i, base;

	if(*str)
	{
		t = &term_tbl[term];
		base = y + ((strlen(str) - 1) * (long) (t->v_char)) / (long) 2;
		for (i = 0; i < strlen(str); i++)
		{
			(*t->move) (x, base - i * (t->v_char));
			(*t->chr_text) (str[i]);
		} /*for (i = 0; i < strlen(str); i++)*/
	} /*if(*str)*/
} /*vertstring()*/

/*
 * term_tbl[] contains an entry for each terminal.  "unknown" must be the
 *   first, since term is initialized to 0.
 *   Then (for MacAnova) must come the standard device entry
 *   Then must come the PostScript entry
 */
struct termentry term_tbl[] =
{
	{
/*Definition of UNKNOWNTERM*/
		"unknown",
		100, 100, 1, 1,	1, 1,
		(int (*) (void)) UNKNOWN, /*UNKNOWN_init()*/
		(void (*) (void)) UNKNOWN, /*UNKNOWN_reset()*/
		(void (*) (void)) UNKNOWN, /*UNKNOWN_text()*/
		(int (*) (void)) UNKNOWN, /*UNKNOWN_graphics()*/
		(void (*) (unsigned long, unsigned long)) UNKNOWN, /*UNKNOWN_move()*/
		(void (*) (unsigned long, unsigned long)) UNKNOWN, /*UNKNOWN_vector()*/
		(void (*) (long)) UNKNOWN, /*UNKNOWN_linetype()*/
		(void (*) (char)) UNKNOWN, /*UNKNOWN_chr_text()*/
		(void (*) (char *)) UNKNOWN, /*UNKNOWN_str_text()*/
		(void (*) (long, long, long)) UNKNOWN /*UNKNOWN_point()*/
	}
/*Definition of STANDARDTERM*/
#if defined(MACINTOSH)
	,
	{
		"Macintosh", MAC_XMAX, MAC_YMAX, MAC_VCHAR, MAC_HCHAR,
		MAC_VTIC, MAC_HTIC, MAC_init, MAC_reset,
		MAC_text, MAC_graphics, MAC_move, MAC_vector,
		MAC_linetype, MAC_chr_text, MAC_str_text, MAC_point
	}
#elif defined(WXWIN)
	,
	{
		"wxWindows", WX_XMAX, WX_YMAX, WX_VCHAR, WX_HCHAR,
		WX_VTIC, WX_HTIC, WX_init, WX_reset,
		WX_text, WX_graphics, WX_move, WX_vector,
		WX_linetype, WX_chr_text, WX_str_text, WX_point
	}
#elif defined(TEK)
	,
	{
		"tek40xx", TEK40XMAX, TEK40YMAX, TEK40VCHAR, TEK40HCHAR,
		TEK40VTIC, TEK40HTIC, TEK40init, TEK40reset,
		TEK40text, TEK40graphics, TEK40move, TEK40vector,
		TEK40linetype, TEK40chr_text, TEK40str_text, do_point
	}
#elif defined(BCPP)
	,
	{
		"BC", BC_XMAX, BC_YMAX, BC_VCHAR, BC_HCHAR,
		BC_VTIC, BC_HTIC, BC_init, BC_reset,
		BC_text, BC_graphics, BC_move, BC_vector,
		BC_linetype, BC_chr_text, BC_str_text, do_point
	}
#elif defined(DJGPP)
	,
	{
		"DJG", DJG_XMAX, DJG_YMAX, DJG_VCHAR, DJG_HCHAR,
		DJG_VTIC, DJG_HTIC, DJG_init, DJG_reset,
		DJG_text, DJG_graphics, DJG_move, DJG_vector,
		DJG_linetype, DJG_chr_text, DJG_str_text, do_point
	}
#endif

#ifdef DUMB
/*Definition of DUMBTERM*/
    ,
	{
		"dumb", DUMB_XMAX, DUMB_YMAX, DUMB_VCHAR,DUMB_HCHAR,
		DUMB_VTIC, DUMB_HTIC, DUMB_init, DUMB_reset,
		DUMB_text, DUMB_graphics, DUMB_move, DUMB_vector,
		DUMB_linetype, DUMB_chr_text, DUMB_str_text, DUMB_point
	}
#endif /* DUMB*/
#ifdef POSTSCRIPT
/*Definition of PSTERMTERM*/
    ,
	{
		"postscript",PS_XMAX,PS_YMAX,PS_VCHAR,PS_HCHAR,
		PS_VTIC, PS_HTIC, PS_init, PS_reset,
		PS_text, PS_graphics, PS_move, PS_vector,
		PS_linetype, PS_chr_text, PS_str_text, PS_point
	}
#endif /*POSTSCRIPT*/

#ifdef EPSFPLOTSOK
/*Definition of PREVIEWTERM*/
#ifdef MACINTOSH /*Currently (960523) only platform allowing epsf*/
	,
	{
		"Macintosh", MAC_XMAX, MAC_YMAX, MAC_VCHAR, MAC_HCHAR,
		MAC_VTIC, MAC_HTIC, MAC_init, MAC_reset,
		MAC_text, MAC_graphics, MAC_move, MAC_vector,
		MAC_linetype, MAC_chr_text, MAC_str_text, MAC_point
	}
#else /*MACINTOSH*/
ERROR
#endif /*MACINTOSH*/
#endif /*EPSFPLOTSOK*/

/*Definition of FILETERM when different from DUMBTERM*/
#ifdef MACINTOSH /*file will be PICT file*/
	,
	{
		"Macintosh", MAC_XMAX, MAC_YMAX, MAC_VCHAR, MAC_HCHAR,
		MAC_VTIC, MAC_HTIC, MAC_init, MAC_reset,
		MAC_text, MAC_graphics, MAC_move, MAC_vector,
		MAC_linetype, MAC_chr_text, MAC_str_text, MAC_point
	}
#elif defined(TEK) /*file with contain Tektronix commands and data*/
	,
	{
		"tek40xx", TEK40XMAX, TEK40YMAX, TEK40VCHAR, TEK40HCHAR,
		TEK40VTIC, TEK40HTIC, TEK40init, TEK40reset,
		TEK40text, TEK40graphics, TEK40move, TEK40vector,
		TEK40linetype, TEK40chr_text, TEK40str_text, do_point
	}
#endif
};

long       TermCount =  sizeof(term_tbl)/sizeof(struct termentry);
