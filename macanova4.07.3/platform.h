/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 4.07 or later
*(C)*
*(C)* Copyright (c) 1999 by Gary Oehlert and Christopher Bingham
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
   for defines that differ between platforms
   Normally not in dependency list in Makefile
*/

#ifndef PLATFORMH__
#define PLATFORMH__
#undef UNDEFINED__

/*
   950623 platform.h modified in several ways to allow for new handling of
   missing values and allowing porting to new platforms with minimum changes

   960621 added first color initializations

   960710 Incorporated changes for WX win version

   961127 Added macro NPRODUCTS

   961216 Incorporated changes for Linux

   970110 Added provision for history saving without readline

   970123 Added HASSELECTION for use with Motif version

   970129 Added HASPOPEN indicating popen() and pclose() available

   970203 Added RECOGNIZEBANG indicated lines starting with ! are special

   970409 Added TEXTREADMODE as mode to open text files for reading by
          batch(), restore(), help() and data and macro reading.  It
	      is "r" except on MSDOS/Windows when it is "rb".  MacAnova
          takes care of interpreting CR/LF as end of line on all systems.
          Added BINARYREADMODE as mode to open binary files.

   970506 Added NOHYPOT (was conditionally defined in mathutil.c
          Added TEXTWRITEMODE, BINARYWRITEMODE, TEXTAPPENDMODE, and
          BINARYAPPENDMODE
          Added defines for Alpha VMS version

   970508 Added CASEBLINDLINKER.  If defined, linker cannot distinguish
          such as isatty and ISATTY that differ only in case.  Needed for
          VMS and perhaps other systems.

   970512 Added NOSEPARATOR.  If not defined, path names without terminating
          file name should end with separator such as '/' or ':'.
   970611 Added HASDYNLOAD   If not defined, platform cannot dynamically
          load external program
   970730 Added MAXINTEGER to define largest exactly representable power of
          2 when values.h is not available (NOVALUESH defined)
   970903 Incorporated changes for SGI IRIX

   980313 Added define of USENOMISSING.  If defined the NOMISSING bit in
          Symbols is used to avoid unnecessary checks of all the elements
          when MISSING values are illegal, as in matrix multiplication and
          inversion.  If it is not wanted for a particular platform, put
          #undef USENOMISSING in an appropriate place

   980331 Revised to separate non-generic stuff into individual header files
          platfrms/{alphavms.h,bcpp.h,djgpp.h,genunix.h,hpgcc.h,hpgccwx.h
          linux86wx.h,mpw.h,mwcw.h,platform.h,sgi.h,sgiwx.h,vaxvms.h,win32.h}

   980331 Added USEA4PAPER.  Default will be US Letter paper on WXWINMOTIF.
          Define USEA4PAPER for A4 default.

   980507 Added HASCONSOLEDIALOG; should be defined for all WX versions and
          Macintosh.

   981029 Added defines PLATFORM_SUN and PLATFORM_WXSUN for SUN Solaris
*/

#ifndef WHITECOLOR
#define WHITECOLOR    0
#endif /*WHITECOLOR*/

#ifndef BLACKCOLOR
#define BLACKCOLOR    1
#endif /*BLACKCOLOR*/

/*
   If you are porting MacAnova to a new platform and don't want to monkey with
   this file, put the special defines for your platform in a header file
   whose name is specified by macro LOCALHEADERFILE, and modify the Makefile
   so that compile commands similar to the following are generated
    cc ... -D'LOCALHEADERFILE="hal.h"' ...  .  If it being ported to a new
   Unix system, you can also put defines in a file specified by SPECIALUNIX
   and all the Unix-specific defines below will be skipped.

   Your file or additions to this should define some or all of the
   following macros:
    PLATFORM      A name for the Computer/Compiler combination
    PLATFORM_ALT1 Names for computer/compiler combinations which are
    PLATFORM_ALT2 at the binary save level (see restore).
	PLATFORM_ALT3
	PLATFORM_ALT4
	PLATFORM_ALT5
	PLATFORM_ALT6
	PLATFORM_ALT7
	PLATFORM_ALT8
    NOSTRTOD      Defined only if strtod() is not an available function
    NOLGAMMA      Defined only if lgamma() is not an available function
    NOHYPOT       Defined only if hypot() is not an available function
    NOTIMEH       Defined only if <time.h> is not available
    NOSTDLIBH     Defined only if <stdlib.h> is not available
    NOVALUESH     Defined only if <values.h> is not available
                  If defined, MAXINTEGER should be defined (see below)                  
    NOFLOATH      Defined only if <float.h> is not available
    HASISATTY     Defined only if isatty() is an available function
    HASSYSTEM     Defined only if system() is to be used by shell()
    HASPOPEN      Defined only if popen() and pclose() are to be used by shell()
                  If neither HASSYSTEM or HASPOPEN is defined, shell() is
	              is unimplemented
    RECOGNIZEBANG Recognize lines starting with '!'.  Currently defined
                  whenever either HASSYSTEM or HASPOPEN is defined.
    HASGETTIMEOFDAY Defined only if gettimeofday() is available
    USEPOW        Defined if pow(x, p) is efficient when p == floor(p)
                  If not defined, intpow() (in mathutil.c) is used.
    NPRODUCTS     Should be defined for systems where actual interrupt is
                  not available (Macintosh, Motif, Windows, DJGPP).  Certain
                  operations (e.g., matrix multiplication) will check
                  for an interrupt about every NPRODUCTS products or
                  quotients.  If not defined, no such check takes place.
    PROVIDEINDEX  If defined, index() and rindex() are compiled.  Although
                  used directly, they may be referred to in the readline
                  library.  If so, and they are not otherwise available,
                  defining PROVIDEINDEX avoids a load error
    DISABLECONTROLY This probably needs to be defined under Unix to avoid
                  a problem with readline so that ^Y can be used as editing
                  command yank The problem is that ^Y may be interpreted by
                  the system as suspend.  Additional changes may need to be
                  made in unxio.c to implement this on a new system.
    SCREENDUMP    Defined if screendump is implemented for plotting commands,
                  with appropriate code added to term.c and elsewhere
    BINARYHELPFILE Defined if a file must be opened in binary mode for
                  fseek() to work reliably and CR/LF marks end of line.
    DEFINEEDIT    Defined if macro edit is to be pre-defined in
                  iniMacro.c; versions are available there for Unix and DOS
    CANSETFONTS   Defined only if options 'font' and 'fontsize' are meaningful
    BDOUBLEHILOW  Defined only if sizeof(double) == 2 * sizeof(long)
                  and higher order half of a double, including the
                  exponent, comes before the lower order half
    BDOUBLELOWHI  Defined only if sizeof(double) == 2 * sizeof(long)
                  and higher order half of a double, including the
                  exponent, comes after the lower order half
    HASNAN        Defined only if NaN's are available to be used as
                  internal value for MISSING
    MISSINGISNUMBER May be defined when HASNAN is defined to specify that the
                  internal value for MISSING is a double and not a NaN.
                  There is no apparent reason to define it unless using a NaN
                  for MISSING causes problems.
    HIMISSING     High part of MISSING.  Define with a suitable value if the
                  default value is not OK.  Default is 0xffffffffUL if
                  HASNAN is defined and MISSINGISNUMBER is not defined, and
                  is 0x0000ffffUL otherwise
    LOWMISSING    Low part of MISSING.  Define with a suitable value if the
                  default value is not OK.  The default is 0x00000000UL,
                  whether or not HASNAN is defined
    HASINFINITY   Defined only if +- infinity have representations
    HIPLUSINFINITY Define with a suitable value if 0x7ff00000UL is not
                  appropriate for high part of NaN designating +infinity
    HIMINUSINFINITY Define with a suitable value if 0x00000000UL is not
                  appropriate for low part of NaN designating +infinity
    LOWPLUSINFINITY Define with a suitable value if 0xfff00000UL is not
                  appropriate for high part of NaN designating -infinity
    LOWMINUSINFINITY Define with a suitable value if 0x00000000UL is not
                  appropriate for low part of NaN designating -infinity
    MAXINTEGER    If NOVALUESH is defined, define MAXINTEGER as 2^N
                  where 2^N is largest power of 2 exactly representable
                  as a double (4503599627370496 = 2^52 on many computers)
    MACANOVAINI   defined as "name for start up file" if not defined in
                  Makefile
    HASCLIPBOARD  Defined only if "real" clipboard exists and code has
                  been added to comclipb.c to handle it
    HASSELECTION  Defined only if X selection mechanism is available as
                  an alternative to the Clipboard and code has been added
                  to comclipb.c to handle it.
    HASDYNLOAD    Defined only if functions callC() and dynload() are
                  implemented.
    MULTIPLOTWINDOWS Defined only if several plotting windows are available
                  and code has been provided defining and using global
                  ThisWindow (see mac/macGraph.c)
    EPSFPLOTSOK   Defined only if you can make encapsulated PostScript files.
                  It should be defined as an integer which can be used as
                  an index in array term_tbl defined in term.c.  On a
                  Macintosh it should be 1 corresonding to Quickdraw
                  pictures.  On other platforms, code will need to be
                  added to term.c
    SCROLLABLEWINDOW Defined only if regular screenwindow is scrollable and
                  thus output does not need to be paused every screenful
    DEFAULTPLOTPAUSE Defined non-zero only if default after high resolution
                  plotting is to pause.
    HASFINDFILE   Defined only on systems where you can locate a file
                  using a dialog box
    HASCONSOLEDIALOG Defined on on systems where interactive reading from
                  CONSOLE uses a dialog box
    ACTIVEUPDATE  If defined, code is generated to update a non-scrollable
                  screen after high resolution plotting and option 'update'
                  is implemented with the value of ACTIVEUPDATE as its default.
                  A non-zero value means updating is on by default; 0 means
                  it's off.
    CASEBLINDLINKER Defined only on systems such as VMS where the linker
                  cannot distinguish names that differ only in the case
                  of letters (isatty and ISSATTY or varnames and VARNAMES).
    NOSEPARATOR   Defined only on systems such as VMS where directory names
                  should not end with separator that separates directory
                  names in a path.
    SAVEHISTORY   If defined, save a history of command lines
    MAXHANDLESIZE Maximum possible handle size in bytes.  Can be set in
                  Makefile.  At present the limit is meaningful
                  only for the generic MSDOS version.
	
	TEXTREADMODE  Mode to open text files for reading by batch(), restore(),
                  help() and data and macro reading.  It is "r" except on
                  MSDOS/Windows when it is "rb".  MacAnova takes care of
                  interpreting CR/LF as end of line on all systems.

	BINARYREADMODE Mode to open binary files for reading; "r" except on
                  MSDOS/Windows when it is "rb"

    TEXTWRITEMODE Mode to open text files for writing by asciisave(), print(),
                  etc.

    BINARYWRITEMODE Mode to open binary files for writing.

    TEXTAPPENDMODE Mode to open text files for appending, print(), etc.

    BINARYAPPENDMODE Mode to open binary files for appending (probably no used
                  but here for completeness

    isNewline()   Macro to test whether a character is to be considered
                  a new line.  Could define a function call.

    USEA4PAPER    US Letter is default paper in wxwinmotif.  Define this to
                  make A4 the default paper.

   If you need to modify existing code, you should also define a macro,
   probably preferably in your Makefile whose definition signals your
   platform and bracket your changes with suitable #ifdef's and #endif's.
   Examples of these are MPW, DJGPP, etc.

   If it is a Unix platform, you may similarly define macro SPECIALUNIX to
   be the name of a header file giving defines appropriate for your machine;
   see below
   
*/
/*
   The following are used to define PLATFORM, PLATFORM_ALT1 and PLATFORM_ALT2
*/
#define PLATFORM_MPW "Macintosh (MPW)"
#define PLATFORM_CW  "Macintosh (CodeWarrior)"
#define PLATFORM_MAC "Macintosh"

#define PLATFORM_LINUX86WX "Linux-gcc wxWindows on Intel x86"
#define PLATFORM_LINUX86   "Linux-gcc on Intel x86"
#define PLATFORM_BCPP      "Generic MSDOS (BCPP)"
#define PLATFORM_DJGPP     "32 bit MSDOS protected mode (DJGPP)"
#define PLATFORM_WXBCPP    "32 bit Windows (Borland C++ 4.5)"

#define PLATFORM_SGI     "Silicon Graphics Irix"
#define PLATFORM_WXSGI   "wxWindows on Silicon Graphics Irix"

#define PLATFORM_SUN     "SUN Solaris"
#define PLATFORM_WXSUN   "wxWindows on SUN Solaris"

#define PLATFORM_HPCC    "Hewlett-Packard HPUX on HP-PA"
#define PLATFORM_HPGCC   "GCC HPUX on HP-PA"
#define PLATFORM_WXHPGCC "GCC wxWindows on HP-PA"

#define PLATFORM_VMSALPHACC "CC VMS on Dec Alpha"
#define PLATFORM_VMSVAXCC   "CC VMS on Dec VAX"

#define PLATFORM_OS2EMX  "OS2 EMX+gcc"

#ifdef LOCALHEADERFILE  /* to be defined in Makefile if used; see above*/
#include LOCALHEADERFILE
#elif defined(SGI) && !defined(WXWINMOTIF) /*LOCALHEADERFILE*/
#include "platfrms/sgi.h"
#elif defined(SGI) && defined(WXWINMOTIF) /*LOCALHEADERFILE*/
#include "platfrms/sgiwx.h"
#elif defined(SUN) && !defined(WXWINMOTIF) /*LOCALHEADERFILE*/
#include "platfrms/sun.h"
#elif defined(SUN) && defined(WXWINMOTIF) /*LOCALHEADERFILE*/
#include "platfrms/sunwx.h"
#elif defined(HPGCC) && !defined(WXWINMOTIF) /*LOCALHEADERFILE*/
#include "platfrms/hpgcc.h"
#elif defined(HPGCC) && defined(WXWINMOTIF) /*LOCALHEADERFILE*/
#include "platfrms/hpgccwx.h"
#elif defined(HPPA) && !defined(WXWINMOTIF) /*LOCALHEADERFILE*/
#include "platfrms/hpcc.h"
#elif defined(LINUX86) && !defined(WXWINMOTIF) /*LOCALHEADERFILE*/
#include "platfrms/linux86.h"
#elif defined(LINUX86) && defined(WXWINMOTIF) /*LOCALHEADERFILE*/
#include "platfrms/linux86wx.h"
#elif defined(OS2EMX)  /*LOCALHEADERFILE*/
#include "platfrms/os2emx.h"
#elif defined(BCPP)  /*LOCALHEADERFILE*/
#include "platfrms/bcpp.h"
#elif defined(DJGPP)  /*LOCALHEADERFILE*/
#include "platfrms/djgpp.h"
#elif defined(WXWINMSW)  /*LOCALHEADERFILE*/
#include "platfrms/wxwinmsw.h"
#elif defined(MPW)  /*LOCALHEADERFILE*/
#include ":platfrms:mpw.h"
#elif defined(MW_CW)  /*LOCALHEADERFILE*/
#include ":platfrms:mwcw.h"
#elif defined(ALPHAVMS) /*LOCALHEADERFILE*/
#include "platfrms/alphavms.h"
#elif defined(VAXVMS) /*LOCALHEADERFILE*/
#include "platfrms/vaxvms.h"
#endif /*LOCALHEADERFILE*/

#ifndef USENOMISSING
#define USENOMISSING 1 /* define this below as 0 if you dont want it*/
#endif /*USENOMISSING*/

#ifndef DEFAULTPLOTPAUSE
#define DEFAULTPLOTPAUSE  1 /* default is pause after high resolution plot */
#endif /*DEFAULTPLOTPAUSE*/

/*
  RECOGNIZEBANG is defined on all platforms; lines starting with '!'
  will produce an error message if neither HASSYSTEM or HASPOPEN is defined.
*/
#define RECOGNIZEBANG
#ifndef BANG
#define BANG   '!'
#endif /*BANG*/

#ifndef MAXHANDLESIZE
#define MAXHANDLESIZE  2000000000 /* about 2147483647 = 2^31 */
#endif /*MAXHANDLESIZE*/

#if !defined(MACINTOSH) && !defined (TEK)
#define DUMBFILETERM  /*Non-postscript plot files will be dumb*/
#endif /*!MACINTOSH && !TEK*/

#ifdef BDOUBLEHILOW
#define LONGSPERDOUBLE 2
#define BDOUBLEHI      0
#define BDOUBLELOW     1
#endif /*BDOUBLEHILOW*/

#ifdef BDOUBLELOWHI
#define LONGSPERDOUBLE 2
#define BDOUBLEHI      1
#define BDOUBLELOW     0
#endif /*BDOUBLELOWHI*/

/*
   Macros related to MISSING and INFINITY
   Those available assume sizeof(double) == 2*sizeof(long)
   Those having to do with NaN, assume a NaN is characterized by
   having all exponent bits (except its sign) set,
*/

#if LONGSPERDOUBLE == 2
/* see globkb.h for macros BdoubleHI, BdoubleLO, DoubleHI, DoubleLO */
#define setBdouble(X,H,L) (BdoubleHi(X) = H, BdoubleLow(X) = L)

/* 
   The first argument to doubleEqualBdouble must be a double lvalue; 
   the second must be a Bdouble
*/

#define doubleEqualBdouble(X, Y) (DoubleHi(X) == BdoubleHi(Y) &&\
								  DoubleLow(X) == BdoubleLow(Y))

#ifdef HASNAN
#define isNaN(X) (((DoubleHi(X) & 0x7ff00000UL) == 0x7ff00000UL))

/* If problems occur using a NaN as MISSING, define MISSINGISNUMBER */
#ifndef MISSINGISNUMBER
#define MISSINGISNAN
#endif /*MISSINGISNUMBER*/

#else /*HASNAN*/
#define isNaN(X) (0)

#undef MISSINGISNAN

#ifndef MISSINGISNUMBER
#define MISSINGISNUMBER
#endif /*MISSINGISNUMBER*/

#endif /*HASNAN*/

#ifndef HIMISSING  /* could be defined above or in Makefile */

#ifdef MISSINGISNUMBER
#define HIMISSING         0x0000ffffUL  /* a very small number */
#define LOWMISSING        0x00000000UL
#endif /*MISSINGISNUMBER*/

#ifdef MISSINGISNAN
#define HIMISSING         0xffffffffUL  /* a NaN */
#define LOWMISSING        0x00000000UL
#endif /*MISSINGISNAN*/

#endif /*HIMISSING*/

/*
   The following macro is here because it might depend on the architecture
   Note that only the high part of Y is checked.
   If not defined in platform.h, it will be defined in globkb.h explicitly
   comparing the whole value
*/
#define isMissing(Y) (HIMISSING == DoubleHi(Y))

#if defined(READLINE) && !defined(SAVEHISTORY)
#define SAVEHISTORY
#endif /*READLINE*/

#ifdef HASINFINITY

#ifndef HIPLUSINFINITY
#define HIPLUSINFINITY    0x7ff00000UL
#define LOWPLUSINFINITY   0x00000000UL
#define HIMINUSINFINITY   0xfff00000UL
#define LOWMINUSINFINITY  0x00000000UL
#endif /*HIPLUSINFINITY*/

/*
   The following macro is here because it might depend on the architecture
   Note that only the high part of Y is checked.
   If not defined in platform.h, it will be defined in globkb.h explicitly
   comparing the whole value
*/
#define isInfinite(Y) (HIPLUSINFINITY == (DoubleHi(Y) & 0x7fffffffUL))
#endif /*HASINFINITY*/

#else /*if LONGSPERDOUBLE == 2*/
/* Place for macros for other architectures */
#endif /*if LONGSPERDOUBLE == 2*/

#ifdef CASEBLINDLINKER
#define varnames Getvarnames
#define ISATTY   ISITATTY
#endif /*CASEBLINDLINKER*/

#ifndef isNewline
#define isNewline(C) ((C) == '\n')
#endif /*isNewline*/

/*
   Definition of default foregound (FG), background (BG), frame, point,
   line and string colors.
*/

#ifndef DEFFGCOLOR
#define DEFFGCOLOR BLACKCOLOR
#endif /*DEFFGCOLOR*/
#ifndef DEFBGCOLOR
#define DEFBGCOLOR WHITECOLOR
#endif /*DEFBGCOLOR*/
#ifndef DEFFRAMECOLOR
#define DEFFRAMECOLOR BLACKCOLOR
#endif /*DEFFRAMECOLOR*/
#ifndef DEFPOINTCOLOR
#define DEFPOINTCOLOR BLACKCOLOR
#endif /*DEFPOINTCOLOR*/
#ifndef DEFLINECOLOR
#define DEFLINECOLOR BLACKCOLOR
#endif /*DEFLINECOLOR*/
#ifndef DEFSTRINGCOLOR
#define DEFSTRINGCOLOR BLACKCOLOR
#endif /*DEFSTRINGCOLOR*/

/* These might be platform dependent, but currently are not*/ 
#ifndef MAXFMTWIDTH
#define MAXFMTWIDTH 27    /* maximum width allowed on formats */
#endif /*MAXFMTWIDTH*/
#ifndef MAXFMTDIGITS
#define MAXFMTDIGITS 20   /* maximum number of decimals or significant digits*/
#endif /*MAXFMTDIGITS*/

#ifndef CONSOLENAME
#define CONSOLENAME "CONSOLE"	/* CONSOLE recognized as pseudo file name */
#endif /*CONSOLENAME*/

/* following are (and probably should be) defined on all platforms */
#define DUMB		/* "dumb" plots an option on plotting commands */
#define BITOPS		/* bit (%&, %|, etc.) operations will be defined */
#define POSTSCRIPT	/* postscript output an option on plotting commands */
#define ONETYPEHANDLE /* all pointers to pointers have same form */
#define USEMEMSET 	/* use memset() to initialize symbols */
#define USEMEMCPY   /* use memcpy() in duplicating handles*/
#define USEFSEEK   /* use ftell() & fseek() to locate help items */

#ifndef TOOBIGVALUESTRING
#define TOOBIGVALUESTRING "1e3000" /*non-representable value*/
#endif /* TOOBIGVALUESTRING*/

/* 1st two lines of copyright information for all platforms */
#define COPYRIGHT1 "Type 'help(copyright)' for copyright and warranty info"
#define COPYRIGHT2 "Copyright (C) 1994 - 1999 Gary W. Oehlert and Christopher Bingham"

#endif /*PLATFORMH__*/
