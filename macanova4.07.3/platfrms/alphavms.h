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

/* defines for alpha VMS */

#  define PLATFORM PLATFORM_VMSALPHACC
#  ifndef VMS
#    define VMS
#  endif

#define NOHYPOT
#define NOLGAMMA
#define BDOUBLEHILOW
/* Here we assume double as G_floating 	(0.56e-308 ... 0.899e308)
   Must be modified if using D_floating (0.29e-28  ... 1.7e38)
 */

#define HUGEDBL 8.9e307
#define INFINY  8.9e307
#define SMALNO  2.2250738585072e-308

#define CASEBLINDLINKER  /* linker can't tell isatty from ISATTY */
#define NOSEPARATOR      /* directory names do not end in separator charactor*/
#define NOTIMEH    /*<time.h> is not available*/
#define NOVALUESH  /*<values.h> is not available*/
#define MAXINTEGER 4503599627370496.0 /*2^52, may not be appropriate*/
#define NOFLOATH   /*<float.h> is not available*/
#define HASSYSTEM  /*Unix normally does not use system() directly in shell()*/

#ifndef ACTIVEUPDATE /* allow it to be set in Makefile */
#define ACTIVEUPDATE 0 /* update console after plotting commands */
#endif /*ACTIVEUPDATE*/

#undef NPRODUCTS    /*actual interrupt is available*/

#if (0) /* hope macro ISATTY helps */
#undef HASISATTY	/* Problems with ISATTY vs isatty()  under VMS*/
#endif /*0*/

#define COMMANDLINE	/*has command line arguments*/
#define HASPOPEN    /*VMS has popen() and pclose() (used in shell())*/

/*
   VMS default is to define USEPOW ; undef it below if necessary
   for a particular compiler.
*/
#define USEPOW		/*use pow() for integer exponents*/

#define DEFINEEDIT /*macro edit is pre-defined*/
#define DEFINEMORE /*macro more is pre-defined*/

#ifndef TEK
#define TEK   /* include Textronix driver */
#endif /*TEK*/

#ifndef MACANOVAINI
#define MACANOVAINI  "macanova.ini"
#endif /*MACANOVAINI*/
/* end of generic defines for VMS platforms */

#define TEXTREADMODE     "r" /*could this be "rb"?*/
#define BINARYREADMODE   "rb"

#define TEXTWRITEMODE    "w"
#define BINARYWRITEMODE  "wb"

#define TEXTAPPENDMODE   "a"
#define BINARYAPPENDMODE "ab"

