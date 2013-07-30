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


#ifndef WXWINMOTIF
#define WXWINMOTIF
#endif /*WXWINMOTIF*/
#ifndef UNIX
#define UNIX
#endif /*UNIX*/

/* Defines for generic Unix platforms. */
#include "genunix.h"


/* defines for WXwin version on SUN Motif */

#ifndef wx_motif
#define wx_motif
#endif /*wx_motif*/

#ifndef WXWIN
#define WXWIN
#endif /*WXWIN*/

#define SOLARIS

#define SIGNALARG /*signal handler of form void handler(int) */

#define NPRODUCTS  1000000 /*number of products between interrupt checks*/

#define PLATFORM       PLATFORM_WXSUN
#define PLATFORM_ALT1  PLATFORM_SGI
#define PLATFORM_ALT2  PLATFORM_HPGCC
#define PLATFORM_ALT3  PLATFORM_WXHPGCC
#define PLATFORM_ALT4  PLATFORM_SUN
#define PLATFORM_ALT5  PLATFORM_WXSGI


#define HASCLIPBOARD	/*CLIPBOARD "connected" to clipboard*/
#define HASSELECTION	/*SELECTION "connected" to X-selection*/

#define HASDYNLOAD  
#define HASGETTIMEOFDAY
#define _STRUCT_TIMEVAL /*structure timeval type defd in headers*/


/* We hope we will some day be able to activate the following defines */
#if (0)
#define CANSETFONTS      /*options 'font' and 'fontsize' meaningful*/
#endif /*(0)*/

#if (0)
#define USETICKTIMER
#endif /*0*/

#define SAVEHISTORY
#undef  ACTIVEUPDATE
#define MULTIPLOTWINDOWS   /* several plotting windows available */
#define SCROLLABLEWINDOW   /* output window is scrollable */
#define DEFAULTPLOTPAUSE  0 /* default is not pause after high resolution plot */
#define HASFINDFILE       /* has find file dialog box */
#define HASCONSOLEDIALOG  /* console input in dialog box */

#define BDOUBLEHILOW
#define HASINFINITY
#define HASNAN


