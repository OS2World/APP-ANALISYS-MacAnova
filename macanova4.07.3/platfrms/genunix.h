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
   Defines for generic Unix platforms.
   These mainly reflect characteristics of the Unix system.

   If any of the generic Unix defines do *not* apply to your Unix platform,
   insert an appropriate #undefine in a platform-specific include 

   If any of the skipped NOXXXXXXH defines *do* apply to your Unix platform,
   insert appropriate #defines in a platform-specific section below

   980714 removed conditional define of ENABLECONTROLY.  This should
          be defined in specific unix platform files as needed.
*/

#define HASPWDH		/*has <pwd.h>*/
#define HASSYSTEM   /*system() implemented*/
#undef NPRODUCTS    /*actual interrupt is available*/
#define HASISATTY	/*has isatty()*/
#define COMMANDLINE	/*has command line arguments*/
#define HASPOPEN    /*Unix has popen() and pclose() (used in shell())*/
#define DEFINEEDIT /*macro edit is pre-defined for Unix*/
#define DEFINEMORE /*macro more is pre-defined for Unix*/

#if 0
#define NOTIMEH    /*<time.h> is not available*/
#define NOSTDLIBH  /*<stdlib.h> is not available*/
#define NOVALUESH  /*<values.h> is not available*/
#define MAXINTEGER 4503599627370496.0 /*2^52*/
#define NOFLOATH   /*<float.h> is not available*/
#undef  HASSYSTEM	/* system() not available or not usable*/
#endif /*0*/

/*
   Unix default is to define USEPOW ; undef it after include of this file
   for a particular compiler, e.g., HPGCC, since gcc gave inaccurate
   results on HP/UX
*/
#define USEPOW		/*use pow() for integer exponents*/


#if defined(READLINE)
#if (0) /*980714 should be defined as needed in specific platform files */
/*
   The following is for use when READLINE is defined to prevent ^Y from
   causing process suspension instead of a yank.  It may be unnecessary if
   the READLINE library is properly compiled.  See rltty.c
*/
#define DISABLECONTROLY  /*keep ^Y from causing suspension instead of yank*/
#endif /*0*/

#define DEFAULTHISTORYLENGTH 100
#endif /*READLINE*/

#ifndef ACTIVEUPDATE /* allow it to be set in Makefile */
/*
  Enable code for updating console after a plotting command with
  default not to update
*/
#define ACTIVEUPDATE 0
#endif /*ACTIVEUPDATE*/

#ifndef TEK
#define TEK   /* include Textronix driver */
#endif /*TEK*/

#ifndef MACANOVAINI
#define MACANOVAINI  ".macanova.ini"
#endif /*MACANOVAINI*/

/* no distinction between binary and text files, e.g., Unix*/
#define TEXTREADMODE     "r"
#define BINARYREADMODE   "r"

#define TEXTWRITEMODE    "w"
#define BINARYWRITEMODE  "w"

#define TEXTAPPENDMODE   "a"
#define BINARYAPPENDMODE "a"


