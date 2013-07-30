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

/*
   Version of 951201
   960716 Changed types Long, Char, Double, uChar to long, char, double,
          unsigned char
   961127 Added Global NPRODUCTS
   961209 Moved global and related macros to handlers.h
   970124 Added globals SELECTIONSYMBOL and SELECTIONNAME when
          HASSELECTION is defined (Motif)
   970312 Added DEFAULTLEFTBRACKETS and LEFTBRACKETS, Lparens and Rparens
          for use in Labeling; Also DEFAULTUSECOLLABS and USECOLLABS
          Thes may someday be settable by setoption()
   970409 Added LASTINPUTWASCR[], flags to indicate whether last character
          from input file was CR
   970506 Removed initialization of INPUTFILE (it's done by initialize)
          Also added casts to 0 initializations of pointers
   970530 Added globals with various mathematical constants MV_PI, MV_E, etc.
   970613 Added INPUTLEVEL, INPUTSTRINGS and ISTRCHARS and made INPUTSTRING
          and ISTRCHAR macros
   970616 Undid some of previous change, replacing them with INPUTSTACK and
          various macros for accessing components of INPUTSTACK[INPUTLEVEL]
          INPUTSTRING and ISTRCHAR remain actual variables for efficiency
   970714 Added global EVALDEPTH, to be incremented and then decremented
          by each call to evaluate() (or to mvEval() from user function
   970805 Added global ThisMachine and enum osCodes.  ThisMachine
		  is set to the appropriate code in initialize()
   970818 Added global SCRATCHHANDLE for ephemeral scratch use to avoid
          memory leaks.
   970820 Added scratchhandle to struct mvMacro and additional macros
          Added global EVALSCRATCH and macro isevaluated
   970823 Added funcname to struct mvMacro and additional macros, including
          replacement for global FUNCNAME
   980329 Reorganized handling of prompts to make possible a 'prompt' keyword
          for batch() and a command line argument.  Also changed globals
          BATCHECHO, INPUTFILE, INPUTFILENAMES, LASTINPUTWASCR
          to pointers with initialize() allocating space for them.
          New type promptType defined.
   980420 Further changes with prompts.  New global array DEFAULTPROMPTS
          (allocated in initialize()).  If a -p flag is used at startup
          DEFAULTPROMPTS[0] is set to its value; if prompt:newprompt is an
          argument to batch(), DEFAULTPROMPTS[BDEPTH] is set to newprompt;
          setoptions(default:T) resets array PROMPTS from DEFAULTPROMPTS
          Macro DEFAULTPROMPT redefined.  New macro STANDARDPROMPT is
          defined in initiali.c
   980624 Added globals HISTRY and DEFAULTSAVEHISTORY in connection with
          new keyword phrase history on save().  DEFAULTSAVEHISTORY
          will allow new option 'savehistory' for setoptions.
   980718 Moved stuff related to save() and restore() to mvsave.h
   980722 Moved stuff related to help to new header mvhelp.h
   980723 Added globals LABELSSCRATCH and NOTESSCRATCH providing names
          for symbols for labels and notes.
   980902 Corrected initialization for MLEVEL
   981118 Added enum and globals to control automatic searches for
          undefined macros
*/
#ifndef GLOBKBH__
#define GLOBKBH__

#include "typedefs.h"

#undef UNDEFINED__

#undef EXTERN
#undef INIT
#undef INITDIM
#undef INITARRAY
#undef COMMA

#ifdef MAIN__
#define EXTERN
#define INIT(VAL) = VAL
#define INITDIM(N) N
#define INITARRAY(VALS) = { VALS }
#else
#define EXTERN extern
#define INIT(VAL)
#define INITDIM(N)
#define INITARRAY(VALS)
#endif /*MAIN__*/

#define COMMA ,

#define INVISIBLESYMBOLS

#ifndef MISSINGVALUE
#define MISSINGVALUE -99999.9999
#endif

#define OLDMISSINGVALUE -99999.9999 /* MISSING through 3.36 (at least) */

#ifndef FILE
#include <stdio.h>
#endif /*FILE*/

/* globals maintained by Addsymbol & Cutsymbol*/
EXTERN long Nscratch INIT(0);
EXTERN long Maxscratch INIT(0);
EXTERN long Nsymbols INIT(0);
EXTERN long Maxsymbols INIT(0);
EXTERN long Nfunctions INIT(0);

/*globals for keeping track of memory usage */
EXTERN long Nhandles INIT(0);
EXTERN long Maxhandles INIT(0);
EXTERN long CurrentMemory INIT(0);
EXTERN long MaxMemory INIT(0);

#ifdef ACTIVEUPDATE
EXTERN short ActiveUpdate INIT(ACTIVEUPDATE);
#else /*ACTIVEUPDATE*/
EXTERN short ActiveUpdate INIT(0);
#endif /*ACTIVEUPDATE*/

typedef union Bdouble
{
	unsigned long     bits[LONGSPERDOUBLE];
	double            value;
} Bdouble, *BdoublePtr, **BdoubleHandle;

/* in the following, X must be of type Bdouble */
#define BdoublePart(X, I) ((X).bits[I])
#define BdoubleHi(X) BdoublePart(X, BDOUBLEHI)
#define BdoubleLow(X) BdoublePart(X, BDOUBLELOW)
#ifdef BDOUBLEMID
#define BdoubleMid(X) BdoublePart(X, BDOUBLEMID)
#endif /*BDOUBLEMID*/

/* In the following, X must be an lvalue */
#define DoublePart(X, I) (((BdoublePtr) &(X))->bits[I])
#define DoubleHi(X) DoublePart(X, BDOUBLEHI)
#define DoubleLow(X) DoublePart(X, BDOUBLELOW)
#ifdef BDOUBLEMID
#define DoubleMid(X) DoublePart(X, BDOUBLEMID)
#endif /*BDOUBLEMID*/

/* These should be initialized by initialize() */
/* internal representation of missing value */
EXTERN Bdouble Missing;    /* current missing value */
EXTERN Bdouble OldMissing; /* <= version 3.36 missing value */

#define MISSING (Missing.value)
#define OLDMISSING (OldMissing.value)

#ifdef HASINFINITY
/* internal representations for +infinity and -infinity */
EXTERN Bdouble PlusInfinity;
EXTERN Bdouble MinusInfinity;
#define PLUSINFINITY (PlusInfinity.value)
#define MINUSINFINITY (MinusInfinity.value)
#endif /*HASINFINITY*/

EXTERN Bdouble TooBigValue;/*set to  mystrtod(TOOBIGVALUESTRING,(char *) 0)*/
#define TOOBIGVALUE TooBigValue.value

/* Numerical Constants initialized in initiali.c */
EXTERN double MV_E;           /*2.7182818284590452354*/
EXTERN double MV_LOG2E;       /*1.4426950408889634074*/
EXTERN double MV_LOG10E;     /*0.43429448190325182765*/
EXTERN double MV_LN2;        /*0.69314718055994530942*/
EXTERN double MV_LN10;       /*2.30258509299404568402*/
EXTERN double MV_PI;         /*3.14159265358979323846*/
EXTERN double MV_PI_2;       /*1.57079632679489661923*/
EXTERN double MV_PI_4;       /*0.78539816339744830962*/
EXTERN double MV_1_PI;       /*0.31830988618379067154*/
EXTERN double MV_2_PI;       /*0.63661977236758134308*/
EXTERN double MV_2_SQRTPI;   /*1.12837916709551257390*/
EXTERN double MV_SQRT2;      /*1.41421356237309504880*/
EXTERN double MV_SQRT1_2;    /*0.70710678118654752440*/

EXTERN char      NullString [] INITARRAY('\0');

/* name for MISSING in output */
#define LENGTHMISSING   20
EXTERN char   DEFAULTNAMEFORMISSING[]  INIT("MISSING");
EXTERN char   NAMEFORMISSING[LENGTHMISSING+1];

#ifndef isMissing
#ifdef MISSINGISNAN
#define isMissing(X)  doubleEqualBdouble(X, Missing)
#else /*MISSINGISNAN*/
#define isMissing(X)  ((X) == MISSING)
#endif /*MISSINGISNAN*/
#endif /*isMissing*/

#ifndef setMissing
#define setMissing(X) (X = MISSING)
#endif /*setMissing*/

#ifdef HASINFINITY
/* name for Infinity in output if desired*/
#define LENGTHINFINITY  20
EXTERN char   DEFAULTNAMEFORINFINITY[]  INIT("Infinity");
EXTERN char   NAMEFORINFINITY[LENGTHINFINITY+1];

#ifndef isInfinite
#define isInfinite(X) (doubleEqualBdouble(X, PlusInfinity) ||\
					   doubleEqualBdouble(X, MinusInfinity))
#endif /*isInfinite*/

#ifndef setPlusInfinity
#define setPlusInfinity(X) (X = PLUSINFINITY)
#endif /*setPlusInfinity*/

#ifndef setMinusInfinity
#define setMinusInfinity(X) (X = MINUSINFINITY)
#endif /*setMinusInfinity*/

#endif /*HASINFINITY*/

/* global variables related to input processing */

/*
  structure for holding information about where yylex() gets characters
  Component    Meaning
  inputstring  handle to current character vector containing commands being
               parsed, either from expanded out-of-line macro or evaluate()
               argument
  scratchhandle handle for use as transitory scratch space, e.g. for
               a command to be evaluated.
  start        position in inputstring of start of macro whose expanded
               text is in the next inputstring on the stack
               (0 if next is evaluate() string)
  end          next position in inputstring; set from ISTRCHAR
               on push and set to ISTRCHAR on pop
  bracketlev   BRACKETLEV before push; used to reset BRACKETLEV on pop
  parenlev     PARENLEV before push; used to reset PARENLEV on pop
  wdepth       WDEPTH before push; used to reset WDEPTH on pop
  funcname     name of current function
  inputname    name of expanded out-of-line macro or EVALNAME (see below)

  These are generally only updated on a call to pushInputLevel()
*/
typedef struct 
{
	unsigned char ** inputstring;
    char          ** scratchhandle; /*handle for transitory storage*/
	long             start;  /*1st character of macro call or inline expansion*/
	long             end;    /*1st character after macro call or inline expansion*/
	long             bracketlev; /*BRACKETLEV at start */
	long             parenlev; /*PARENLEV at start */
	long             wdepth; /*WDEPTH at start*/
	char             funcname[NAMELENGTH+1]; /*name of current function*/
	char             inputname[NAMELENGTH+1];
} mvMacro, *mvMacroPtr, **mvMacroHandle;

/* INPUTSTACK of length MAXMDEPTH+1 allocated in initialize() */
EXTERN mvMacroPtr      INPUTSTACK;

#define NINPUTSTRINGS  (MAXMDEPTH+1)

EXTERN char            EVALNAME[INITDIM(NAMELENGTH+1)] INIT("@$EVAL$$$$$$");

EXTERN int             INPUTLEVEL   INIT(0);

EXTERN long            MACROSTART; /*start of macro in current line*/

/* will be same as INPUTSTACK[INPUTLEVEL].inputstring */
EXTERN unsigned char **INPUTSTRING INIT((unsigned char **) 0);

/* will be same as INPUTSTACK[INPUTLEVEL].end */
EXTERN long            ISTRCHAR INIT(0);

/* bracket level at current parse level; must be restored on pop*/
EXTERN long            BRACKETLEV INIT(0); 
/* parenthesis level at current parse level; must be restored on pop */
EXTERN long            PARENLEV INIT(0); 

#define ThisInputstring     (INPUTSTACK[INPUTLEVEL].inputstring)
#define ThisInputstart      (INPUTSTACK[INPUTLEVEL].start)
#define ThisInputend        (INPUTSTACK[INPUTLEVEL].end)
#define ThisBracketlev      (INPUTSTACK[INPUTLEVEL].bracketlev)
#define ThisParenlev        (INPUTSTACK[INPUTLEVEL].parenlev)
#define ThisWdepth          (INPUTSTACK[INPUTLEVEL].wdepth)
#define ThisFuncname        (INPUTSTACK[INPUTLEVEL].funcname)
#define ThisInputname       (INPUTSTACK[INPUTLEVEL].inputname)
#define ThisIstrchar        ThisInputend
#define ThisScratchHandle   (INPUTSTACK[INPUTLEVEL].scratchhandle)

#define NextInputstring     (INPUTSTACK[INPUTLEVEL + 1].inputstring)
#define LastInputstring     (INPUTSTACK[INPUTLEVEL - 1].inputstring)
#define LastIstrchar        (INPUTSTACK[INPUTLEVEL - 1].end)

/*
  970823 Following replaced global FUNCNAME
*/
#define FUNCNAME            ThisFuncname

/*
  SCRATCHHANDLE is to be used for temporary scratch; it should be disposed
  of and set to (char **) as soon possible.  If not zero, cleanitup
  will dispose of it
*/
#define SCRATCHHANDLE        ThisScratchHandle


#define clearSCRATCHHANDLE() {\
								mydisphandle(SCRATCHHANDLE);\
								SCRATCHHANDLE = (char **) 0;\
							 }

#define DEFAULTEXPANDINLINE  1
/* 
   If EXPANDINLINE != 0, the default for macro expansion is in line
   settable by setoptions()
*/
EXTERN int EXPANDINLINE INIT(DEFAULTEXPANDINLINE);
/*
  981118 codes and global related to automatic search for macro when
         name(... is encounted, where name is undefined
*/
enum macroSearchCodes
{
	noMacroSearch = 0,
	verboseMacroSearch,
	quietMacroSearch
};

EXTERN long    DEFAULTMACROSEARCH INIT(verboseMacroSearch);
EXTERN long    MACROSEARCH INIT(verboseMacroSearch);

/* set by yyparse() to value of evaluated string */
EXTERN Symbolhandle   PARSERVALUE INIT((Symbolhandle) 0);

/* 
   Global variables related to syntax and loops

   MDEPTH is the current depth to which macros are nested
   MLEVEL is the number of macros using '$$' since last prompt, used
   as value for $$ in a macro
*/

#ifndef DEFAULTMAXWHILE
#define DEFAULTMAXWHILE   1001
#endif /*DEFAULTMAXWHILE*/

#ifndef MINMAXWHILE
#define MINMAXWHILE      10 /*minimum legal value for option maxwhile*/
#endif /*MINMAXWHILE*/

 /* maximum number of repetitions of while loop */

EXTERN long            MAXWHILE INIT(DEFAULTMAXWHILE);

typedef char    mvMacroName[NAMELENGTH + 1];

EXTERN long            MDEPTH INIT(0);
EXTERN long            MLEVEL INIT(MAXMDEPTH + 1);
EXTERN long            ACTIVEMACROS INIT(0); 
EXTERN mvMacroName    *MACRONAMES; /* allocated in initialize ()*/

#define ThisMacroName  MACRONAMES[ACTIVEMACROS-1]
#define PushMacroName(NAME) (ACTIVEMACROS++, strcpy(ThisMacroName, NAME))
#define PopMacroName() ((ACTIVEMACROS == 0) ? 0 :\
						(ThisMacroName[0] = '\0',--ACTIVEMACROS))

/*
  depth of nested calls to evaluate(), including calls to mvEval()
  from user functions
*/
EXTERN long            EVALDEPTH INIT(0);

EXTERN long            WDEPTH INIT(0);
EXTERN long            WHILESTARTS[INITDIM(MAXWDEPTH)];
EXTERN long            WHILECONDITIONS[INITDIM(MAXWDEPTH)];
EXTERN long            WHILELIMITS[INITDIM(MAXWDEPTH)];
EXTERN long            WHILEBRACKETLEV[INITDIM(MAXWDEPTH)];
EXTERN double        **FORVECTORS[INITDIM(MAXWDEPTH)];

EXTERN long            IDEPTH INIT(0);
EXTERN long            IFCONDITIONS[INITDIM(MAXIDEPTH)];
EXTERN long            IFBRACKETLEV[INITDIM(MAXIDEPTH)];

/* 970823 replaced following by define; see above */
#if (0)
EXTERN char            FUNCNAME[INITDIM(NAMELENGTH+1)];
#endif /*0*/
EXTERN Symbolhandle    NULLSYMBOL;
EXTERN char           *NULLNAME INIT("NULL");

EXTERN Symbolhandle    CLIPBOARDSYMBOL INIT((Symbolhandle) 0);
EXTERN char           *CLIPBOARDNAME INIT("CLIPBOARD");

#ifdef wx_motif
/*
  SELECTIONSYMBOL is like CLIPBOARDSYMBOL except it is connected
  with the XA_PRIMARY X-selection instead of the Clipboard
*/
EXTERN Symbolhandle    SELECTIONSYMBOL INIT((Symbolhandle) 0);
EXTERN char           *SELECTIONNAME INIT("SELECTION");
#endif /*wx_motif*/

#define TEMPPREFIX     '@'        /* 1st character of temporary names */

#define KEYPREFIX1     TEMPPREFIX /* 1st character of keyword names */
#define KEYPREFIX2     TEMPPREFIX /* 2nd character of keyword names */

#define SCRATCHPREFIX1 TEMPPREFIX /* 1st character of scratch names */
#define SCRATCHPREFIX2 '$'        /* 2nd character of scratch names */

#define INVISPREFIX    '_'   /* _.*  @_.*  and @$_.* are invisible names*/

/* standard name for used up keyword, must start with KEYPREFIXs */
EXTERN char             USEDKEYWORD[] INIT("@@@USED");

/* standard names for scratch variables.  Must start with SCRATCHPREFIXs */
#define LENGTHSCRATCH  9
EXTERN char             SCRATCH[LENGTHSCRATCH+1] INIT("@$SCRATCH");

#define LENGTHLOGSCRATCH  9
EXTERN char             LOGICSCRATCH[LENGTHLOGSCRATCH+1] INIT("@$LOGICAL");

#define LENGTHNUMSCRATCH  8
EXTERN char             NUMSCRATCH[LENGTHNUMSCRATCH+1] INIT("@$NUMBER");

#define LENGTHNULLSCRATCH  6
EXTERN char             NULLSCRATCH[LENGTHNULLSCRATCH+1] INIT("@$NULL");

#define LENGTHSTRINGSCRATCH  8
EXTERN char             STRINGSCRATCH[LENGTHSTRINGSCRATCH+1] INIT("@$STRING");

#define LENGTHSCALSCRATCH  8
EXTERN char             SCALSCRATCH[LENGTHSCALSCRATCH+1] INIT("@$SCALAR");

#define LENGTHVECSCRATCH  8
EXTERN char             VECSCRATCH[LENGTHVECSCRATCH+1] INIT("@$VECTOR");

#define LENGTHMATSCRATCH  8
EXTERN char             MATSCRATCH[LENGTHMATSCRATCH+1] INIT("@$MATRIX");

#define LENGTHARRAYSCRATCH  7
EXTERN char             ARRAYSCRATCH[LENGTHARRAYSCRATCH+1] INIT("@$ARRAY");

#define LENGTHSTRUCSCRATCH  11
EXTERN char             STRUCSCRATCH[LENGTHSTRUCSCRATCH+1] INIT("@$STRUCTURE");

#define LENGTHGARBSCRATCH  9
EXTERN char             GARBSCRATCH[LENGTHGARBSCRATCH+1] INIT("@$GARBAGE");

#define LENGTHMACROSCRATCH  7
EXTERN char             MACROSCRATCH[LENGTHMACROSCRATCH+1] INIT("@$MACRO");

#define LENGTHLISTSCRATCH  6
EXTERN char             LISTSCRATCH[LENGTHLISTSCRATCH+1] INIT("@$LIST");

#define LENGTHGRAPHSCRATCH  7
EXTERN char             GRAPHSCRATCH[LENGTHGRAPHSCRATCH+1] INIT("@$GRAPH");

#ifdef INVISIBLESYMBOLS
#define LENGTHINVISSCRATCH  10
EXTERN char             INVISSCRATCH[LENGTHINVISSCRATCH+1] INIT("@$_SCRATCH");
#endif /*INVISIBLESYMBOLS*/

#define LENGTHEVALSCRATCH   10
EXTERN char             EVALSCRATCH[LENGTHEVALSCRATCH+1] INIT("@$EVALUATE");

EXTERN char             LABELSSCRATCH[] INIT("@$LABELS");
EXTERN char             NOTESSCRATCH[]  INIT("@$NOTES");

/*
  980718
  Moved globals such as as OPTIONS related to save() and restore() to mvsave.h
*/

/*
   960611 made '_' a legal character to start a name but any symbol
          with a name starting with '_' is "invisible" to list() and
          listbrief().  In addition, an unassigned invisible value is not
          printed.
   960612 names starting @_ and @$_ are also invisible.  
*/

#ifndef INVISIBLESYMBOLS  /* previous definition */
#define isnamestart(C) isalpha(C) /* check for legal start to name */
#define invisname(A) (0)
#else /*INVISIBLESYMBOLS*/
#define isnamestart(C)  (isalpha(C) || (C) == INVISPREFIX)
						/* check for legal characters to start name */
#define invisname(A) ((A)[0] == '_' ||\
					  istempname(A) && (A)[1] == INVISPREFIX ||\
					  isscratch(A) && (A)[2] == INVISPREFIX)
#endif /*INVISIBLESYMBOLS*/

#define isnamechar(C)  (isalnum(C) || (C) == '_')
						/* check for legal characters in name */
#define iskeystart(C)  isalpha(C) /* check for legal start to keyword name */
#define iskeyname(A)  ((A)[0] == KEYPREFIX1 && (A)[1] == KEYPREFIX2)
#define istempname(A) ((A)[0] == TEMPPREFIX)
#define isscratch(A)  ((A)[0] == SCRATCHPREFIX1 && (A)[1] == SCRATCHPREFIX2)
#define isevaluated(A) (strcmp((A), EVALSCRATCH) == 0)

#ifndef STDOUT
#define STDOUT (FILE *) 0
#endif /*STDOUT*/

#ifndef STDIN
#if defined(MACINTOSH) || defined(WXWIN)
#define STDIN (FILE *) 0
#else /*defined(MACINTOSH) || defined(WXWIN)*/
#define STDIN stdin
#endif /*defined(MACINTOSH) || defined(WXWIN)*/
#endif /*STDIN*/

#ifndef EXPANSIONPREFIX
#define EXPANSIONPREFIX   '~'
#endif /*EXPANSIONPREFIX*/

/* standard separators used in paths*/
#if defined(MACINTOSH)
EXTERN char * DIRSEPARATOR INIT(":");
#elif defined(MSDOS)
EXTERN char * DIRSEPARATOR INIT("\\");
#else
EXTERN char * DIRSEPARATOR INIT("/");
#endif /*MACINTOSH*/

#ifndef UNLOADSEG
#if defined(MACINTOSH) && !defined(powerc)
#include <SegLoad.h>
#define UNLOADSEG(F)    myUnloadSeg((Ptr) (F))
EXTERN         int UNLOADSEGMENTS INIT(0);
#else /*MACINTOSH && !powerc*/
#define UNLOADSEG(F)
#endif /*MACINTOSH && !powerc*/
#endif /*UNLOADSEG*/


/* define initialization file name */
#ifndef MACANOVAINI
#if defined(MSDOS)
#define MACANOVAINI "MACANOVA.INI"
#elif defined(MACINTOSH)
#define MACANOVAINI "MacAnova.ini"
#else
#define MACANOVAINI ".macanova.ini"
#endif
#endif /*MACANOVAINI*/

/* global SPOOL file */
EXTERN FILE           *SPOOLFILE INIT((FILE *) 0);
EXTERN char          **SPOOLFILENAME INIT((char **) 0); /* handle */

/* global HELP file (kb) */
EXTERN FILE           *HELPFILE  INIT((FILE *) 0);
EXTERN char          **HELPFILENAME INIT((char **) 0); /* handle */

/* Global restore file info */
EXTERN FILE           *RESTOREFILE INIT((FILE *) 0);

/* Global save file info */
EXTERN FILE           *KEEPFILE INIT((FILE *) 0);/*file pointer for save file*/
EXTERN char          **KEEPFILENAME  INIT((char **) 0); /* handle */

/* Global plotting info (kb) */
EXTERN FILE           *PLOTFILE  INIT((FILE *) 0);
EXTERN char          **PLOTFILENAME INIT((char **) 0); /* handle */
EXTERN long            DEFAULTDUMBPLOT INIT(0);

/* DEF.*COLOR are defined in platform.h, possibly platform specific */
EXTERN unsigned char   DEFAULTPOINTCOLOR INIT((unsigned char) DEFPOINTCOLOR);
EXTERN unsigned char   DEFAULTLINECOLOR INIT((unsigned char) DEFLINECOLOR);
EXTERN unsigned char   DEFAULTSTRINGCOLOR INIT((unsigned char) DEFSTRINGCOLOR);
EXTERN unsigned char   DEFAULTFGCOLOR INIT((unsigned char) DEFFGCOLOR);
EXTERN unsigned char   DEFAULTBGCOLOR INIT((unsigned char) DEFBGCOLOR);
EXTERN unsigned char   DEFAULTFRAMECOLOR INIT((unsigned char) DEFFRAMECOLOR);

EXTERN unsigned char   DEFAULTXTICKLENGTH INIT((unsigned char) TICKUNIT);
EXTERN unsigned char   DEFAULTYTICKLENGTH INIT((unsigned char) TICKUNIT);

/* for global info on last plot */
EXTERN Symbolhandle    PLOT INIT((Symbolhandle) 0); 

/* values for global ThisWindow */
enum thisWindowCodes
{
	FINDEMPTYWINDOW  = -1, /*no window keyword on plotting command*/
	PREVIEWWINDOW    = -2, /*creating picture for preview part of EPSF file*/
	PICTWINDOW       = -3  /*creating picture for writing to file*/
};


EXTERN long  ThisWindow INIT(FINDEMPTYWINDOW); /*current graphics window*/

/* default values for options affecting save() or restore()*/
EXTERN long            DEFAULTSAVEHISTORY INIT(0);
EXTERN long            DEFAULTRESTOREDEL INIT(1); /*delete:T or F restore() default*/

/*
  980718 Moved other stuff having to do with save() and restore() to mvsave.h
*/

/* Stack of input files, updated by batch() (kb) */
EXTERN long            BDEPTH INIT(0);
EXTERN long           *BATCHECHO; /*BATCHECHO[MAXBDEPTH]*/
EXTERN FILE          **INPUTFILE;
EXTERN char         ***INPUTFILENAMES;

/* Flag for whether last input character read was CR*/
EXTERN char           *LASTINPUTWASCR;

/* Descriptor of version, e.g., "MacAnova 3.32 of 11/01/93" */
EXTERN char          **VERSION_ID INIT((char **) 0);

/*
	Encoding of information about the machine/system
	ThisMachine is to be set in main() for each version
*/

enum osCodes
{
	mvMsDos         = 0x0100,
	mvMacintosh     = 0x0200,
	mvUnix          = 0x0400,
	mvVMS           = 0x0800,
	mvWin32s        = mvMsDos        | 0x0001,
	mvWindows95     = mvWin32s       | 0x0002,
	mvWindowsNT     = mvWin32s       | 0x0004,
	mvLinux         = mvMsDos        | 0x0008,
	mvMacintosh68K  = mvMacintosh    | 0x0001,
	mvMacintosh68Kc = mvMacintosh68K | 0x0010,
	mvMacintosh68Kn = mvMacintosh68K | 0x0020,
	mvMacintoshPPC  = mvMacintosh    | 0x0002,
	mvMotif         = mvUnix         | 0x0001,
	mvMotifVMS      = mvVMS          | 0x0001
};

EXTERN unsigned long   ThisMachine INIT(0);

/* resettable by option batchecho */
EXTERN long            DEFAULTBATCHECHO INIT(1);

/* resettable by option warning */
EXTERN long            PRINTWARNINGS INIT(1);

/* global labelling defaults; may sometime be settable by setoptions()*/
EXTERN long            DEFAULTUSECOLLABS INIT(0);
EXTERN long            USECOLLABS INIT(0);
EXTERN char            DEFAULTLEFTBRACKET[] INITARRAY("(");
EXTERN char            LEFTBRACKET[] INITARRAY("(");
EXTERN char            RIGHTBRACKET[] INITARRAY(")");
EXTERN char            Lparens[] INITARRAY("#[({</\\");
EXTERN char            Rparens[] INITARRAY("#])}>/\\");

enum BracketIndices
{
	PoundIndex = 0,
	BracketIndex,
	ParenIndex,
	BraceIndex,
	LTIndex,
	SlashIndex,
	BslashIndex
};
/* prefix for labels expandable a print time */
#define NUMERICLABEL ((char) 0x40) /* '@' */

#ifdef CONSOLENAME
EXTERN char           *CONSOLE INIT(CONSOLENAME);
#endif /*CONSOLENAME*/

#ifndef MILLISECONDSPERTICK
#ifdef WXWIN
#ifdef USETICKTIMER
#define MILLISECONDSPERTICK 500
#else /*USETICKTIMER*/
#define MILLISECONDSPERTICK 1000
#endif /*USETICKTIMER*/
#elif defined(MACINTOSH)
#define MILLISECONDSPERTICK  17
#else
#define MILLISECONDSPERTICK 500
#endif
#endif /*MILLISECONDSPERTICK*/

EXTERN long            Ticks INIT(0);
EXTERN long            MilliSecondsPerTick INIT(MILLISECONDSPERTICK);

/* global variables related to error handling */
/*
  MAXERRORS is settable by setoptions() and is a global error limit.
  MAXERRORS1 is the operative limit.  At the stdin level is should be the
  same as MAXERRORS.  For batch files, MAXERRORS1 will be 1, if MAXERRORS == 0,
  or MAXERRORS, otherwise.

  if MAXERRORS1 != 0 NERRORS gets incremented by myprint() for every line
  starting with "ERROR:"  If MAXERRORS == 0, it is reset to zero before
  the prompt when BDEPTH == 0
*/
EXTERN long            NERRORS INIT(0); /* cumulative number of errors */
EXTERN long            MAXERRORS INIT(0);
	/* settable error limit, 0 means none */
EXTERN long            MAXERRORS1 INIT(0);/* operative error limit */
EXTERN long            WARNINGLEVEL INIT(0);/* 1 or 2 suppress WARNINGs or ERRORs*/
EXTERN long            FatalError INIT(0); /* != 0 if Fatal Error occurred */

/* global buffer of length BUFFERLENGTH for building output strings */
EXTERN char           *OUTSTR INIT((char *) 0);

/* global handle for line input buffer (was in matdat.h) */
EXTERN char          **LINE INIT((char **) 0);

EXTERN char          **TMPHANDLE INIT((char **) 0);/*generic temporary handle*/

#ifdef SAVEHISTORY
/* global variable for history lines to keep */

#ifndef DEFAULTHISTORYLENGTH
#define DEFAULTHISTORYLENGTH  100
#endif /*DEFAULTHISTORYLENGTH*/

EXTERN long            HISTORY INIT(DEFAULTHISTORYLENGTH);
#endif /*SAVEHISTORY*/

/* global variables related to algorithms */
EXTERN double          ANGLES2RADIANS INIT(1.0);
	/* either 1 (radians), 2*pi (cycles), or 2*pi/360 (degrees)*/

/* variables related to random number generation */
EXTERN long         RANDS1 INIT(0) ; /* random number generator seed */
EXTERN long         RANDS2 INIT(0) ; /* random number generator seed */

/* origin for subscripts (for possible future use) */
EXTERN long         ORIGIN INIT(1);

/* hidden option for debugging */
EXTERN long         GUBED  INIT(0);

/* global variables related to output formatting */
EXTERN long         NLINES INIT(0);
	/* number of output lines since start of command*/
#ifndef DEFAULTPRINTDEC
#define DEFAULTPRINTDEC  5
#endif /*DEFAULTPRINTDEC*/
#ifndef DEFAULTWRITEDEC
#define DEFAULTWRITEDEC  9
#endif /*DEFAULTWRITEDEC*/

#ifndef MINSCREENHEIGHT
#define MINSCREENHEIGHT  12
#endif /*MINSCREENHEIGHT*/

#ifndef MINSCREENWIDTH
#define MINSCREENWIDTH   30
#endif /*MINSCREENWIDTH*/

#ifndef DEFAULTSCREENHEIGHT
#ifdef MSDOS
#define DEFAULTSCREENHEIGHT 25
#else /*MSDOS*/
#define DEFAULTSCREENHEIGHT 24
#endif /*MSDOS*/
#endif /*DEFAULTSCREENHEIGHT*/

#ifndef DEFAULTSCREENWIDTH
#define DEFAULTSCREENWIDTH 80
#endif /*DEFAULTSCREENWIDTH*/

EXTERN long         SCREENHEIGHT INIT(0); /* number of lines on screen */
EXTERN long    		SCREENWIDTH INIT(DEFAULTSCREENWIDTH);   /* columns on screen */

/*
	globals related to automatic scrolling back after help()
	and possibly other commands.
	SCROLLBACK will have the value option 'scrollback' if and when implemented
	CURRENTSCROLLBACK determines behavior after current command line, which
	may reset it. 
*/
#ifdef SCROLLABLEWINDOW
#ifndef DEFAULTSCROLLBACK
#define DEFAULTSCROLLBACK   0
#endif /*DEFAULTSCROLLBACK*/

EXTERN long          SCROLLBACK INIT(DEFAULTSCROLLBACK);
EXTERN long          CURRENTSCROLLBACK INIT(DEFAULTSCROLLBACK);
#endif /*SCROLLABLEWINDOW*/

/* Format related items */
enum formatCodes
{
	DODEFAULT    = 0x00,  /* use default format */
	DOFIXED      = 0x01,  /* use fixed point format */
	DOFLOAT      = 0x02,  /* use floating point format */
	NOTRIM       = 0x00,  /* no trimming */
	TRIMRIGHT    = 0x04,  /* trim whitespace on right end */
	TRIMLEFT     = 0x08,  /* trim whitespace on left end */
	CHARASIS     = 0x10   /* put string as is, regardless of MISSINGFMT*/
};


EXTERN double       MAXFIXED; /* maximum size for fixed format number */
EXTERN double       MINFIXED; /* minimum size for fixed format */
EXTERN long         FIELDWIDTH INIT(DEFAULTPRINTDEC+7+1);  /* width of output field */
EXTERN char         FIXEDFMT[20]; /* e.g., " %12.5f" */
EXTERN char         FLOATFMT[20]; /* e.g., " %12.5g" */
EXTERN char         DATAFMT[20]; /* one of FIXEDFMT or FLOATFMT */
EXTERN char         MISSINGFMT[10]; /* for MISSING, e.g., " %12s" */
EXTERN long         BEFOREDEC INIT(DEFAULTPRINTDEC+7);   /* e.g. 12 in 12.5g */
EXTERN long         AFTERDEC INIT(DEFAULTPRINTDEC);
	/* # after dp in format, e.g., 5 in 12.5g */
EXTERN char         FMTTYPE INIT('g');
	/* output format type, e.g., 'g' in 12.5g */
EXTERN long         PRINTFORMAT[]
	INITARRAY(DEFAULTPRINTDEC+7 COMMA DEFAULTPRINTDEC COMMA (long) 'g');
EXTERN long         WRITEFORMAT[]
	INITARRAY(DEFAULTWRITEDEC+7 COMMA DEFAULTWRITEDEC COMMA (long) 'g');

/* Global promps */
#define MAXPROMPT 21
typedef char   promptType[MAXPROMPT];

EXTERN promptType    *PROMPTS;
EXTERN promptType    *DEFAULTPROMPTS;
#define PROMPT        (char *) PROMPTS[BDEPTH]
#define DEFAULTPROMPT (char *) DEFAULTPROMPTS[BDEPTH]
EXTERN promptType     MOREPROMPT;

/* globals related to environment */
EXTERN long         ISATTY INIT(0); /* is input from keyboard */
EXTERN char        *TERMINAL INIT((char *) 0) ; /* on Unix set to $TERM */
/* constants for masking ISATTY */
#define ITTYIN      0x01
#define ITTYOUT     0x02


/* macro for emergency error messages */
#ifdef MACINTOSH
#define PUTALERT(MSGS) (myAlert(MSGS), myerrorout(MSGS))
#else /*MACINTOSH*/
#define PUTALERT(MSGS) (fprintf(stderr,"%s\n",(MSGS)), myerrorout(MSGS))
#endif /*MACINTOSH*/

#undef	EXTERN
#undef	INIT
#undef	INITDIM
#undef	INITARRAY
#undef  COMMA

#ifdef MAIN__
/* copyright notice */
static char     copyright1[] =
	"MacAnova, (C) Copyright 1994 Gary W. Oehlert and Christopher Bingham";
#ifdef MACINTOSH
#ifndef MPW
/* copyright notice */
static char     copyright2[] = "(C) Copyright 1986 THINK Technologies, Inc.  \n\
Certain portions of the Macintosh version of this software are copyrighted\n\
by THINK Technologies, Inc.";
#endif /*MPW*/
#endif /*MACINTOSH*/
#endif /* MAIN__ */

#endif /*GLOBKBH__*/
