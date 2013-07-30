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

/* defines for generic msdos system (borland c) */

#define PLATFORM       PLATFORM_BCPP
#define PLATFORM_ALT1  PLATFORM_DJGPP
#define PLATFORM_ALT2  PLATFORM_WXBCPP
#define PLATFORM_ALT3  PLATFORM_LINUX86
#define PLATFORM_ALT4  PLATFORM_LINUX86WX

#ifndef MSDOS
#define MSDOS           /* operating system of PC compatible */
#endif /*MSDOS*/

#undef NPRODUCTS    /*actual interrupt is available*/
#define MAXHANDLESIZE   65000L  /* maximum size of handles allocated */
#define ACTIVEUPDATE 1 /* update console after plotting commands */
#define HASISATTY	/*bcpp has isatty()*/
#define HASSYSTEM	/*bcpp uses system() in shell()*/
#define NOLGAMMA    /* bcpp no function lgamma() */
#define USEPOW		/*use pow() for integer exponents*/

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

