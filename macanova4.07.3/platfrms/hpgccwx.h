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

/* defines for WXwin version on HP Motif */

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

#define HPUX

#define _INCLUDE_POSIX_SOURCE
#define SIGNALARG /*signal handler of form void handler(int) */

#define NPRODUCTS  1000000 /*number of products between interrupt checks*/

#define PLATFORM       PLATFORM_WXHPGCC
#define PLATFORM_ALT1  PLATFORM_HPGCC
#define PLATFORM_ALT2  PLATFORM_WXSGI
#define PLATFORM_ALT3  PLATFORM_SGI
#define PLATFORM_ALT4  PLATFORM_HPCC

#define HASCLIPBOARD	/*CLIPBOARD "connected" to clipboard*/
#define HASSELECTION	/*SELECTION "connected" to X-selection*/

#undef USEPOW /*gcc gave inaccurate integer powers on HP/UX*/
#define HASDYNLOAD  /* this assumes that the dld library is available */

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


