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

/* defines for MPW Macintosh Programmers' Workbench */

#define PLATFORM       PLATFORM_MPW
#define PLATFORM_ALT1  PLATFORM_CW
#define PLATFORM_ALT2  PLATFORM_MAC
#define MPW3
#define MPW3_2
#define MACINTOSH
#define NOVALUESH          /* no useful values.h available*/
#define MAXINTEGER 4503599627370496.0 /*2^52*/
#define NOLGAMMA           /* no function lgamma() */

/* no distinction between binary and text files, e.g., Unix*/
#define TEXTREADMODE     "r"
#define BINARYREADMODE   "r"

#define TEXTWRITEMODE    "w"
#define BINARYWRITEMODE  "w"

#define TEXTAPPENDMODE   "a"
#define BINARYAPPENDMODE "a"

/*
	SEGMENTED not defined here; must have '-d SEGMENTED' as C compile option
	in Makefile if compiling under MPW or '#define SEGMENTED' in
	ProjectDefines.pch if compiline under IDE
*/
#define NPRODUCTS   50000  /*number of products between interrupt checks*/
#define CANSETFONTS        /*options 'font' and 'fontsize' meaningful*/
#define HASCLIPBOARD       /*CLIPBOARD "connected" to clipboard*/
#define MULTIPLOTWINDOWS   /* several plotting windows available */
#define EPSFPLOTSOK  1     /* make EPSF plots using quickdraw for preview */
#define SCREENDUMP         /*screendump implemented for plotting commands*/
#define SCROLLABLEWINDOW   /* output window is scrollable */
#define DEFAULTPLOTPAUSE  0 /* default is not pause after high resolution plot */
#define HASFINDFILE       /* has find file dialog box */
#define HASCONSOLEDIALOG  /* console input in dialog box */
#define HASDYNLOAD        /*has capability of dynamic loading*/
#define SIZEINTEGER  short /* used in typedef SIZEINTEGER Integer */
#define SIZELONGINT  long  /* used in typedef SIZELONGINT LongInt */
#define BDOUBLEHILOW
#define HASINFINITY
#define HASNAN
#define SAVEHISTORY
#define DEFAULTHISTORYLENGTH  100 /*980824 changed from 50*/
#ifndef MACANOVAINI
#define MACANOVAINI  "MacAnova.ini"
#endif /*MACANOVAINI*/

#define isNewline(C) ((C) == '\n' || (C) == '\r')

