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
   Defines for WXwin version on x86 Linux Motif
   980714 Removed #undef DISABLECONTROLY which is no longer defined
          in genunix.h
*/

#ifndef WXWINMOTIF
#define WXWINMOTIF
#endif /*WXWINMOTIF*/
#ifndef UNIX
#define UNIX
#endif /*UNIX*/

/* Defines for generic Unix platforms. */
#include "genunix.h"

#ifndef wx_motif
#define wx_motif
#endif /*wx_motif*/

#ifndef WXWIN
#define WXWIN
#endif /*WXWIN*/

#define LINUX

/*
   Defines for Linux (2.0.24) contributed by Jan Erik Backlund 
   backl003@gold.tc.umn.edu
*/
#define PLATFORM       PLATFORM_LINUX86WX
#define PLATFORM_ALT1  PLATFORM_DJGPP
#define PLATFORM_ALT2  PLATFORM_WXBCPP
#define PLATFORM_ALT3  PLATFORM_BCPP
#define PLATFORM_ALT4  PLATFORM_LINUX86
#define NOFLOATH 
#define HASGETTIMEOFDAY 
#define BDOUBLELOWHI
#define HASINFINITY
#define HASNAN
#define SIGNALARG
#define NPRODUCTS 50000  /*number of products between interrupt checks*/
#undef  USEPOW
#undef  DEFINEEDIT
#undef  DEFINEMORE
#undef  TEK

/* We hope we will some day be able to activate the following defines */
#if (0)
#define CANSETFONTS      /*options 'font' and 'fontsize' meaningful*/
#endif /*(0)*/
#if (0)
#define USETICKTIMER
#endif /*0*/


#define HASCLIPBOARD	/*CLIPBOARD "connected" to clipboard*/
#define HASSELECTION	/*SELECTION "connected" to X-selection*/


#define SAVEHISTORY
#undef  ACTIVEUPDATE
#define MULTIPLOTWINDOWS   /* several plotting windows available */
#define SCROLLABLEWINDOW   /* output window is scrollable */
#define DEFAULTPLOTPAUSE  0 /* default is not pause after high resolution plot */
#define HASFINDFILE       /* has find file dialog box */
#define HASCONSOLEDIALOG  /* console input in dialog box */


