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

/* defines for WXwin version for MS Windows (win32) */

#ifndef WXWIN
#define WXWIN
#endif /*WXWIN*/

#ifndef wx_msw
#define wx_msw
#endif /*wx_msw*/

#ifndef MSDOS
#define MSDOS
#endif /*MSDOS*/

#define PLATFORM       PLATFORM_WXBCPP
#define PLATFORM_ALT1  PLATFORM_BCPP
#define PLATFORM_ALT2  PLATFORM_DJGPP
#define PLATFORM_ALT3  PLATFORM_LINUX86
#define PLATFORM_ALT4  PLATFORM_LINUX86WX

#define NOVALUESH
#define MAXINTEGER 4503599627370496.0 /*2^52*/

/* We hope we will some day be able to activate the following defines */
#if (0)
#define CANSETFONTS      /*options 'font' and 'fontsize' meaningful*/
#endif /*(0)*/

#define DEFINEEDIT /*edit a pre-defined macro*/
#define HASCLIPBOARD /*CLIPBOARD "connected" to clipboard*/
#define HASPOPEN    /*WXBCPP has popen() and pclose() (used in shell())*/
#undef  HASSYSTEM   /*WXBCPP does not directly use system() in shell()*/
#define NPRODUCTS   50000 /*number of products between interrupt checks*/
#define NOLGAMMA    /* no function lgamma() */
#define NOSTRTOD    /* don't use library strtod(); it's buggy */
#define USEPOW		/*use pow() for integer exponents*/
#define NEEDSFTIME  /* need our own ftime() function */
#define HASDYNLOAD  /* do dynload with dlls */
/* end of defines for WXwin version for MS Windows */

#define SAVEHISTORY
#undef  ACTIVEUPDATE
#define MULTIPLOTWINDOWS   /* several plotting windows available */
#define SCROLLABLEWINDOW   /* output window is scrollable */
#define DEFAULTPLOTPAUSE  0 /* default is not pause after high resolution plot */
#define HASFINDFILE       /* has find file dialog box */
#define HASCONSOLEDIALOG  /* console input in dialog box */

/*
   Note: MacAnova takes care of correctly interpreting CR/LF on
   on all systems
   Actual mode for just plain "r", "w" and "b" depends on environmental
   variable.  "rt", "wt", and "at" force text mode.
*/

#define TEXTREADMODE     "rb" /*that's right; open text files as binary*/
#define BINARYREADMODE   "rb"

#define TEXTWRITEMODE    "wt"
#define BINARYWRITEMODE  "wb"

#define TEXTAPPENDMODE   "at"
#define BINARYAPPENDMODE "ab"

#define COMMANDLINE     /*has command line arguments*/
#define BINARYHELPFILE  /* open help file as binary file */
#define BDOUBLELOWHI
#define HASINFINITY
#define HASNAN

#ifndef MACANOVAINI
#define MACANOVAINI  "MACANOVA.INI"
#endif /*MACANOVAINI*/

