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


#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment Initialize
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

#include "version.h"

#include "globals.h"

#ifdef MACINTOSH
#include "macIface.h"
#endif /*MACINTOSH*/

#ifdef WXWIN
#include "wx/wxIface.h"
#endif  /* WXWIN */

#ifdef READLINE
#undef RETURN
#if defined (READLINE_LIBRARY)
#  include "readline.h"
#  include "history.h"
#else /*READLINE_LIBRARY*/
#  include <readline/readline.h>
#  include <readline/history.h>
#endif /*READLINE_LIBRARY*/

/*
   By suitable defines in a Makefile you can change the default
   location of the readline setup file.

   On any platform, if INPUTRCFILE is defined, it is assumed to be
   a complete path name for the setup file.

   If DEFAULTINPUTRC is defined (but not INPUTRCFILE) it will be
   DATAPATH/DEFAULTINPDATAPATH where DATAPATH is taken from symbol
   "DATAPATH" created by initialize().  This may be set by including
   a -D'DATAPATHNAME="name"' in the compile.  Otherwise it is
   either the user's home directory (unix) or the directory where
   MacAnova is located.

   Neither INPUTRCFILE or DEFAULTINPUTRC defined:
     djgpp:  DATAPATH/INPUTRC
	 Unix:  ~/.inputrc (readline default).
*/
#ifndef DEFAULTINPUTRC
#ifdef DJGPP
#define DEFAULTINPUTRC "INPUTRC"
#endif /*DJGPP*/
#endif /*DEFAULTINPUTRC*/
#endif /*READLINE*/

/*
  970611 added User() and loadUser() to function list
  970627 added asLong() to function list
  971012 added invdunnett() and cumdunnett() to function list
  971109 added rpoi() to function list
  971210 added gethistory() and sethistory() to function list
  980106 initialize() now calls rl_initialize()
  980310 added rbin() to function list
  980317 added isarray() to function list
  980329 addit batchInit() to allocate and initialize globals used by
         batch() and putprompt()
  980401 Added new argument prompt to initialize and use it to initialize
         prompt.
  980723 Added getnotes() to function list
  980727 Added setlabels(), attachnotes() and addnotes() to function list
  980801 Added argvalue() to function list
  980903 Added getascii() to function list
  990304 Added qrdecode() to function list
  990325 Added Arima.mac to initial MACROFILES variable
         Added isname() to function list
*/
#ifdef MACINTOSH
#undef LF
#undef CR
#define LF      10
#define CR      13
#endif /*MACINTOSH*/

#if (0)
extern Symbolhandle Transform();
extern Symbolhandle anovacoefs();
extern Symbolhandle array();
extern Symbolhandle batch();
extern Symbolhandle bin();
extern Symbolhandle boxplot();
extern Symbolhandle callC(); /*kb*/
extern Symbolhandle cellstats();
extern Symbolhandle changestr(); /*kb*/
extern Symbolhandle cholesky(); /*kb*/
extern Symbolhandle cluster(); /*kb*/
extern Symbolhandle columnops(); /*kb*/
extern Symbolhandle compnames();
extern Symbolhandle concat();
extern Symbolhandle contrast();
extern Symbolhandle cor();
extern Symbolhandle cumcdf();
extern Symbolhandle deleter();
extern Symbolhandle describe();
extern Symbolhandle diag(); /*kb*/
extern Symbolhandle dim(); /*kb*/
extern Symbolhandle dynload(); /*kb*/
extern Symbolhandle eigen();
extern Symbolhandle elapsedTime();
extern Symbolhandle evaluate(); /*kb*/
extern Symbolhandle fft();
extern Symbolhandle getHistory();
extern Symbolhandle getlabs();
extern Symbolhandle getseed();
extern Symbolhandle glm();
extern Symbolhandle hconcat(); /*kb*/
extern Symbolhandle help();
extern Symbolhandle iswhat(); /*kb*/
extern Symbolhandle inforead(); /*kb*/
extern Symbolhandle keyvalue(); /*kb*/
extern Symbolhandle kmeans(); /*kb*/
extern Symbolhandle listbrief();
extern Symbolhandle macro();
extern Symbolhandle macrousage(); /*kb*/
extern Symbolhandle makefactor();
extern Symbolhandle makestr();
extern Symbolhandle matrix();
extern Symbolhandle modelvar();
extern Symbolhandle movavg();
extern Symbolhandle myplot();
extern Symbolhandle nameOf(); /*kb*/
extern Symbolhandle ndims(); /*kb*/
extern Symbolhandle outer();
extern Symbolhandle partacf();
extern Symbolhandle paste();
extern Symbolhandle polyroot();
extern Symbolhandle power1();
extern Symbolhandle predtable();
extern Symbolhandle print();
extern Symbolhandle putascii();
extern Symbolhandle qr();
extern Symbolhandle rangen();
extern Symbolhandle rational();
extern Symbolhandle readdata();
extern Symbolhandle regpred();
extern Symbolhandle renamer();
extern Symbolhandle rep();
extern Symbolhandle restore();
extern Symbolhandle xvarsrows(); /*kb*/
extern Symbolhandle run();
extern Symbolhandle samplesize();
extern Symbolhandle save();
extern Symbolhandle selecter(); /*kb*/
extern Symbolhandle setlabs(); /*kb*/
extern Symbolhandle setoptions(); /*kb*/
extern Symbolhandle setseed();
extern Symbolhandle sort();
extern Symbolhandle shell();
extern Symbolhandle solve();
extern Symbolhandle split();
extern Symbolhandle spool();
extern Symbolhandle stemleaf();
extern Symbolhandle svd(); /*kb*/
extern Symbolhandle swp(); /*kb*/
extern Symbolhandle t2val();
extern Symbolhandle tabs();
extern Symbolhandle toeplitz();
extern Symbolhandle transpose();
extern Symbolhandle trideigen();
extern Symbolhandle tserops();
extern Symbolhandle tval();
extern Symbolhandle varnames(); /*kb*/
extern Symbolhandle xvariables(); /*kb*/
extern Symbolhandle yates();
#endif /*0*/

static struct
{                               /* constants */
	char           *name;
	double          value;
}                   constants[] =
{
	{"DELTAT",    1},
	{"PI",        3.14159265358979323846},
	{"E",         2.71828182845904523536},
	{"DEGPERRAD", 57.29577951308232087680},
	{0,           0} 
};

static struct
{
	char           *name;
	char           *string;
}               stringvars[] =

{
#ifdef CONSOLENAME
	{"CONSOLE", CONSOLENAME}, /* "/dev/tty" or "Dev:Console" */
#endif /*CONSOLENAME*/
	{(char *) 0, (char *) 0}
};

static struct
{/* built in functions */
	char           *name;
    Symbolhandle  (*function) ();
}               builtins[] =

{
	{"cat",          concat},
	{"vector",       concat}, /* identical to cat()*/
	{"macro",        macro},
	{"makestr",      makestr},
	{"structure",    makestr}, /*synonym for makestr()*/
	{"strconcat",    makestr},     /*kb*/
	{"compnames",    compnames},   /*kb*/
	{"varnames",     varnames}, /*kb*/
	{"getlabels",    getlabs},   /*kb*/
	{"getnotes",     getlabs},   /*kb*/
	{"setlabels",    setlabs},   /*kb*/
	{"appendnotes",  setlabs},   /*kb*/
	{"attachnotes",  setlabs},   /*kb*/
	{"modelvars",    modelvars}, /*kb*/
	{"modelinfo",    xvariables}, /*kb*/
	{"xvariables",   xvariables}, /*kb*/
	{"xrows",        xvarsrows}, /*kb*/
	{"anova",        glm},
	{"factor",       makefactor},  /*kb*/
	{"match",        makefactor},  /*kb*/
	{"unique",       makefactor},  /*kb*/
	{"matrix",       matrix},
	{"delete",       deleter},
	{"list",         listbrief},
	{"listbrief",    listbrief},
	{"hconcat",      hconcat},     /*kb*/
	{"vconcat",      hconcat},     /*kb*/
	{"select",       selecter},      /*kb*/
	{"paste",        paste},       /*kb*/
	{"shell",        shell},       /*kb*/
	{"User",         callC},       /*kb*/
	{"loadUser",     dynload},     /*kb*/
	{"asLong",       asLong},      /*kb*/
	{"array",        array},
	{"print",        print},
	{"fprint",       print},       /*kb*/
	{"matprint",     print},       /*kb*/
	{"write",        print},       /*kb*/
	{"fwrite",       print},       /*kb*/
	{"matwrite",     print},       /*kb*/
	{"macrowrite",   print},       /*kb*/
	{"error",        print},       /*kb*/
	{"putascii",     putascii},    /*kb*/
	{"getascii",     putascii},    /*kb*/
	{"contrast",     contrast},
	{"anovacoefs",   anovacoefs},
	{"log",          Transform},
	{"log10",        Transform},
	{"exp",          Transform},
	{"abs",          Transform},
	{"cellstats",    cellstats},
	{"describe",     describe},
	{"halfnorm",     sort},        /*kb*/
	{"rankits",      sort},
	{"regress",      glm},
	{"setoptions",   setoptions},
	{"getoptions",   setoptions},
	{"gettime",      elapsedTime}, /*kb*/
	{"sort",         sort},
	{"split",        split},
	{"tval",         tval},
	{"t2val",        t2val},
	{"tint",         tval},
	{"t2int",        t2val},
	{"bin",          bin},
	{"stemleaf",     stemleaf},
	{"regpred",      regpred},
	{"glmpred",      regpred},
	{"yates",        yates},
	{"rational",     rational},    /*kb*/
	{"partacf",      partacf},     /*kb*/
	{"yulewalker",   partacf},     /*kb*/
	{"movavg",       movavg},      /*kb*/
	{"autoreg",      movavg},      /*kb*/
	{"toeplitz",     toeplitz},      /*kb*/
	{"polyroot",     polyroot},      /*kb*/
	{"rft",          fft},         /*kb*/
	{"hft",          fft},         /*kb*/
	{"cft",          fft},         /*kb*/
	{"hconj",        tserops},     /*kb*/
	{"cconj",        tserops},     /*kb*/
	{"hreal",        tserops},     /*kb*/
	{"creal",        tserops},     /*kb*/
	{"himag",        tserops},     /*kb*/
	{"cimag",        tserops},     /*kb*/
	{"hcopyc",       tserops},     /*kb*/
	{"ccopyh",       tserops},     /*kb*/
	{"htoc",         tserops},     /*kb*/
	{"ctoh",         tserops},     /*kb*/
	{"hprdh",        tserops},     /*kb*/
	{"hprdhj",       tserops},     /*kb*/
	{"cprdc",        tserops},     /*kb*/
	{"cprdcj",       tserops},     /*kb*/
	{"cmplx",        tserops},     /*kb*/
	{"hpolar",       tserops},     /*kb*/
	{"cpolar",       tserops},     /*kb*/
	{"hrect",        tserops},     /*kb*/
	{"crect",        tserops},     /*kb*/
	{"unwind",       tserops},     /*kb*/
	{"convolve",     tserops},     /*kb*/
	{"rotate",       tserops},     /*kb*/
	{"rotation",     rotatefac},   /*kb*/
	{"reverse",      tserops},     /*kb*/
	{"padto",        tserops},     /*kb*/
	{"sin",          Transform},
	{"cos",          Transform},
	{"atan",         Transform},
	{"atan2",        Transform},   /*kb*/
	{"hypot",        Transform},   /*kb*/
	{"sqrt",         Transform},
	{"lgamma",       Transform},
	{"asin",         Transform},
	{"acos",         Transform},
	{"tan",          Transform},
	{"sinh",         Transform},
	{"cosh",         Transform},
	{"tanh",         Transform},
	{"atanh",        Transform},
	{"floor",        Transform},
	{"ceiling",      Transform},
	{"round",        Transform},
	{"nbits",        Transform},
	{"predtable",    predtable},
	{"glmtable",     predtable}, /*kb*/
	{"rep",          rep},
	{"run",          run},
	{"fastanova",    glm},
	{"screen",       glm},
	{"setseeds",     setseed},
	{"getseeds",     getseed},
	{"rnorm",        rangen},
	{"rpoi",         rangen},
	{"rbin",         rangen},
	{"runi",         rangen},
	{"ismissing",    sort},
	{"anymissing",   sort},        /*kb*/
	{"isreal",       iswhat},      /*kb*/
	{"ischar",       iswhat},      /*kb*/
	{"islogic",      iswhat},      /*kb*/
	{"isvector",     iswhat},      /*kb*/
	{"ismatrix",     iswhat},      /*kb*/
	{"isarray",      iswhat},      /*kb*/
	{"isscalar",     iswhat},      /*kb*/
	{"isfactor",     iswhat},      /*kb*/
	{"isnull",       iswhat},      /*kb*/
	{"isdefined",    iswhat},      /*kb*/
	{"isstruc",      iswhat},      /*kb*/
	{"ismacro",      iswhat},      /*kb*/
	{"isgraph",      iswhat},      /*kb*/
	{"isname",       iswhat},      /*kb*/
	{"argvalue",     keyvalue},    /*kb*/		
	{"keyvalue",     keyvalue},    /*kb*/		
	{"rename",		renamer},      /*kb*/
	{"nameof",       nameOf},      /*kb*/
	{"wtregress",    glm},
	{"wtanova",      glm},
	{"manova",       glm},
	{"wtmanova",     glm},
	{"grade",        sort},        /*kb*/
	{"rank",         sort},
	{"sum",          columnops},   /*kb*/
	{"prod",         columnops},   /*kb*/
	{"max",          columnops},   /*kb*/
	{"min",          columnops},   /*kb*/
	{"dim",          dim},         /*kb*/
	{"nrows",        dim},         /*kb*/
	{"ncols",        dim},         /*kb*/
	{"ndims",        dim},         /*kb*/
	{"ncomps",       dim},         /*kb*/
	{"length",       dim},         /*kb*/
	{"diag",         diag},        /*kb*/
	{"dmat",         diag},        /*kb*/
	{"swp",          swp},         /*kb*/
	{"bcprd",        swp},         /*kb*/
	{"det",          solve},       /*kb*/
	{"solve",        solve},       /*kb*/
	{"rsolve",       solve},       /*kb*/
	{"cholesky",     cholesky},    /*kb*/
	{"trilower",     qr},          /*kb*/
	{"triupper",     qr},          /*kb*/
	{"triunpack",    qr},          /*kb*/
	{"qrdecode",     qr},          /*kb*/
	{"qr",           qr},          /*kb*/
	{"regsqr",       qr},          /*kb*/
	{"glmfit",       glm},         /*kb*/
	{"robust",       glm},
	{"poisson",      glm},
	{"ipf",          glm},
	{"tabs",         tabs},
	{"logistic",     glm},
	{"probit",       glm},         /*kb*/
	{"secoefs",      anovacoefs},
	{"coefs",        anovacoefs},
	{"cor",          cor},
	{"trace",        eigen},
	{"plot",         myplot},
	{"fplot",        myplot},
	{"addpoints",    myplot},
	{"chplot",       myplot},
	{"fchplot",      myplot},
	{"addchars",     myplot},
	{"lineplot",     myplot},
	{"flineplot",    myplot},
	{"addlines",     myplot},
	{"addstrings",   myplot},
	{"showplot",     myplot},
	{"eigen",        eigen},
	{"eigenvals",    eigen},
	{"releigen",     eigen},
	{"releigenvals", eigen},       /*kb*/
	{"trideigen",    trideigen},   /*kb*/
	{"svd",          svd},         /*kb*/
	{"cluster",      cluster},     /*kb*/
	{"kmeans",       kmeans},      /*kb*/
	{"changestr",    changestr},   /*kb*/
	{"t",            transpose},
	{"outer",        outer},
	{"boxplot",      boxplot},
	{"fboxplot",     boxplot},
	{"save",         save},
	{"asciisave",    save},
	{"restore",      restore},
	{"batch",        batch},
	{"cumnor",       cumcdf},
	{"invnor",       cumcdf},      /*kb*/
	{"cumchi",       cumcdf},
	{"invchi",       cumcdf},      /*kb*/
	{"cumstu",       cumcdf},
	{"invstu",       cumcdf},      /*kb*/
	{"cumF",         cumcdf},
	{"invF",         cumcdf},
	{"cumgamma",     cumcdf},      /*kb*/
	{"invgamma",     cumcdf},      /*kb*/
	{"cumbeta",      cumcdf},      /*kb*/
	{"invbeta",      cumcdf},      /*kb*/
	{"cumstudrng",   cumcdf},      /*go*/
	{"invstudrng",   cumcdf},      /*go*/
	{"cumdunnett",   cumcdf},      /*go*/
	{"invdunnett",   cumcdf},      /*go*/
	{"cumbin",       cumcdf},
	{"cumpoi",       cumcdf},
	{"vecread",      readdata},    /*kb*/
	{"matread",      readdata},    /*kb*/
	{"macroread",    readdata},    /*kb*/
	{"inforead",     readdata},    /*kb*/
	{"read",         readdata},    /*kb*/
	{"spool",        spool},
	{"power",        power1},
	{"power2",       power1},
	{"samplesize",   samplesize},
	{"Help",         help},
	{"evaluate",     evaluate},
	{"help",         help},
	{"usage",        help},
	{"macrousage",   macrousage},
	{"gethistory",   getHistory},
	{"sethistory",   getHistory},
	{(char *) 0,     (Symbolhandle (*) ()) 0}
};


extern Symbolhandle         Install(), RInstall(), Makesymbol();
extern Symbolhandle         Makestruc();
extern Symbolhandle         batch();
extern char               **mygethandle();
extern void                 clearGlobals();

/*
  970612 added inputInit() to allocate pointers to stack of input strings
*/

static int inputInit(void)
{
	INPUTSTACK = (mvMacroPtr) mygetpointer(NINPUTSTRINGS * sizeof(mvMacro));

	if (INPUTSTACK == (mvMacroPtr) 0)
	{
		return (0);
	}
	
	for (INPUTLEVEL = 0; INPUTLEVEL < NINPUTSTRINGS; INPUTLEVEL++)
	{
		ThisInputstring = (unsigned char **) 0;
		ThisInputstart = 0;
		ThisInputend = 0;
		ThisBracketlev = 0;
		ThisParenlev = 0;
		ThisWdepth = 0;
		ThisInputname[0] = '\0';
		ThisScratchHandle = (char **) 0;
	} /*for (INPUTLEVEL = 0; INPUTLEVEL < NINPUTSTRINGS; INPUTLEVEL++)*/
	INPUTLEVEL = 0;

	strcpy(ThisInputname, "prompt level");

	INPUTSTRING = ThisInputstring;
	ISTRCHAR = ThisIstrchar;
	BRACKETLEV = ThisBracketlev;
	PARENLEV = ThisParenlev;
	WDEPTH = ThisWdepth;
	
	MACRONAMES = (mvMacroName *) mygetpointer(2*MAXMDEPTH*sizeof(mvMacroName));
	
	if (MACRONAMES == (mvMacroName *) 0)
	{
		return (0);
	}

	for (ACTIVEMACROS = 1; ACTIVEMACROS <= 2*MAXMDEPTH; ACTIVEMACROS++)
	{
		ThisMacroName[0] = '\0';
	}
	ACTIVEMACROS = 0;

	return (1);
} /*inputInit()*/

#ifndef STANDARDPROMPT
#define STANDARDPROMPT "Cmd> "
#endif /*STANDARDPROMPT*/

static int batchInit(void)
{
	int        i;
	
	BATCHECHO = (long *) mygetpointer(MAXBDEPTH * sizeof(long));

	if (BATCHECHO == (long *) 0)
	{
		return (0);
	}
	INPUTFILE = (FILE **) mygetpointer(MAXBDEPTH * sizeof(FILE *));

	if (INPUTFILE == (FILE **) 0)
	{
		return (0);
	}

	INPUTFILENAMES = (char ***) mygetpointer(MAXBDEPTH * sizeof(char **));

	if (INPUTFILENAMES == (char ***) 0)
	{
		return (0);
	}
	/* Flag for whether last input character read was CR*/
	LASTINPUTWASCR = (char *) mygetpointer(MAXBDEPTH * sizeof(char));

	if (LASTINPUTWASCR == (char *) 0)
	{
		return (0);
	}
	PROMPTS = (promptType *)
	  mygetpointer(2 * MAXBDEPTH * sizeof (promptType));

	if (PROMPTS == (promptType *) 0)
	{
		return (0);
	}
	
	DEFAULTPROMPTS = PROMPTS + MAXBDEPTH;

	for (i = 0; i < MAXBDEPTH; i++)
	{
		BATCHECHO[i] = (i == 0) ? 1 : 0;
		INPUTFILE[i] = (FILE *) 0;
		INPUTFILENAMES[i] = (char **) 0;
		LASTINPUTWASCR[i] = '\0';
		strcpy((char *) DEFAULTPROMPTS[i],
			   (i == 0) ? STANDARDPROMPT : NullString);
	}
	strcpy((char *) MOREPROMPT, "More> ");
	return (1);
} /*batchInit()*/

char     *StdinName = "standard input";

static void      cleanString(char *stringvar)
{
#ifdef MACINTOSH
	while (*stringvar != '\0')
	{
		if (*stringvar == LF)
		{
			*stringvar = CR;
		}
		stringvar++;
	}
#endif /*MACINTOSH*/
} /*cleanString()*/

#ifdef MSDOS
/*  '/' equivalent to '\' on MSDOS*/
#define isSlash(C) ((C) == DIRSEPARATOR[0] || (C) == '/')
#else /*MSDOS*/
#define isSlash(C) ((C) == DIRSEPARATOR[0])
#endif /*MSDOS*/

#ifndef DATAPATHNAME
#ifdef MACINTOSH
#define DATAPATHNAME ""
#endif /*MACINTOSH*/
#ifdef UNIX
#define DATAPATHNAME "./"
#endif /*UNIX*/
#ifdef MSDOS
#define DATAPATHNAME ".\\"
#endif /*MSDOS*/
#endif /*DATAPATHNAME*/

#ifndef MACROPATHNAME
#define MACROPATHNAME DATAPATHNAME
#endif /*MACROPATHNAME*/

#ifndef HELPPATHNAME
#ifdef MACINTOSH
#define HELPPATHNAME ""
#endif /*MACINTOSH*/
#ifdef UNIX
#define HELPPATHNAME "./"
#endif /*UNIX*/
#ifdef MSDOS
#define HELPPATHNAME ".\\"
#endif /*MSDOS*/
#endif /*HELPPATHNAME*/

#ifndef DATAFILENAME
#define DATAFILENAME "macanova.dat"
#endif /*DATAFILENAME*/

#ifndef MACROFILENAME
#define MACROFILENAME "macanova.mac"
#endif /*MACROFILENAME*/

#ifndef TSMACROFILENAME
#define TSMACROFILENAME ""
#endif /*TSMACROFILENAME*/

#ifndef DESIGNMACROFILENAME
#define DESIGNMACROFILENAME ""
#endif /*DESIGNMACROFILENAME*/

#ifndef ARIMAMACROFILENAME
#define ARIMAMACROFILENAME ""
#endif /*ARIMAMACROFILENAME*/

/* Use fake handles for builtin functions*/
 /* Array of pointers to builtins */
static Symbol             **FunctionPointers;
 /* array of Function Symbols */
static FunctionSymbol      *FunctionSymbols;

/*
   Initialize everything in sight.  Called once from main().
   950921 Moved initial reading of "~/.inputrc" ("INPUTRC" on DOS) here

   960426 Added initialization of CHARACTER vector MACROFILES

   960502 Added code to get path of MacAnova folder on Macintosh

   960503 Added initialization of CHARACTER variable DATAPATHS

   961003 Added arguments dataPath and macroPath
          DATAPATH is initialized to dataPath
		  DATAPATHS is initialized to vector(dataPath,macroPath),
		  except only 1 if they are identical.  If either is
		  (char *) 0, defines DATAPATHNAME and/or MACROPATHNAME
		  are used
		  Also, the default initialization of MACROFILE, DATAFILE
		  and MACROFILES no longer includes a complete path
   960124 Added initialization of SELECTION under Motif
   970324 Add function read() which combines matread() and macroread()
   970530 Added initialization of math constants MV_PI, MV_E, ...
   970612 Added initialization of INPUTSTRINGS, etc.
   990324 arima.mac now part of MACROFILES
*/

#define NMACROFILES   4 /* standard, time series, design, arima macro files*/

#include <float.h>

void initialize(char * helpFileName, char * dataFileName,
				char * macroFileName,
				char * dataPath, char * macroPath, char * homePath,
				promptType prompt)
{
	long            i;
	Symbolhandle    symh, symh1;
	long            length, nMacroFiles = 0, nPaths = 0;
	long            nBuiltins = 0;
	char           *dataPathName = (char *) 0, *macroPathName = (char *) 0;
	char           *macroFileNames[NMACROFILES+1];
	char           *place;
	char            completePath[PATHSIZE+1];
	char           *pathName;
	double          value;
#ifdef READLINE
#if defined(INPUTRCFILE) || defined(DEFAULTINPUTRC)
	Keymap           keymap; /* current keymap */
	char            *backwardkillword = "\033\010"; /*Esc-Backspace*/
	FILE            *readlineBindings = (FILE *) 0;
	Symbolhandle     symhPath = (Symbolhandle) 0;
#endif /*defined(INPUTRCFILE) || defined(DEFAULTINPUTRC)*/
#endif /*READLINE*/
	WHERE("initialize");

	getElapsedTime((double *) 0); /* save current time */
	getElapsedTime((double *) 0); /* do it twice for good measure */


#if defined(MSDOS)
	pathName = get_dataPath();
	for (i = 0; pathName[i] != '\0'; i++)
	{ /* make sure all path separators are '\\' */
		if (pathName[i] == '/')
		{
			pathName[i] = DIRSEPARATOR[0];
		}
	} /*for (i == 0; pathName[i] != '\0'; i++)*/
#elif defined(MACINTOSH)
	/* macGetPath() returns address of completePath */
	pathName = macGetPath(completePath, HomeVolume, HomeDirectory);
#elif defined(UNIX)
	pathName = getenv("HOME");
#elif defined(VMS)
	pathName = getenv("ANOVA$HOME");
	dataPathName = 	macroPathName = pathName;
#endif

	if (pathName == (char *) 0)
	{
		completePath[0] = '\0';
	} /*if (pathName == (char *) 0)*/
	else
	{		
		if (pathName != completePath)
		{
			strncpy(completePath, pathName, PATHSIZE);
			completePath[PATHSIZE] = '\0';
		}
		length = strlen(completePath);
#ifndef NOSEPARATOR
		if (completePath[length-1] != DIRSEPARATOR[0] && length < PATHSIZE)
		{
			strcat(completePath, DIRSEPARATOR);
		}
#endif /*NOSEPARATOR*/
	} /*if (pathName == (char *) 0){}else{}*/

	homePath = (homePath != (char *) 0) ? homePath : completePath ;

#if !defined(UNIX) && !defined(VMS)
	if (completePath[0] != '\0')
	{
		dataPathName = (dataPath != (char *) 0) ? dataPath : completePath;
		macroPathName = (macroPath != (char *) 0) ? macroPath : completePath;
	}
	else
#endif /*UNIX*/
/* use defined values on Unix which should have trailing '/' */
	{
		dataPathName = (dataPath != (char *) 0) ? dataPath : DATAPATHNAME;
		macroPathName = (macroPath != (char *) 0) ? macroPath : MACROPATHNAME;
	}
	
/*  initialized math constants */
	MV_E    = exp(1);
	MV_PI_4 = atan(1.0);
	MV_PI_2 = 2.0*MV_PI_4;
	MV_PI   = 4.0*MV_PI_4;

#ifdef M_E
	MV_LOG2E    =       M_LOG2E;
	MV_LOG10E   =      M_LOG10E;
	MV_LN2      =         M_LN2;
	MV_LN10     =        M_LN10;
	MV_1_PI     =        M_1_PI;
	MV_2_PI     =        M_2_PI;
	MV_2_SQRTPI =    M_2_SQRTPI;
	MV_SQRT2    =       M_SQRT2;
#ifdef M_SQRT1_2
	MV_SQRT1_2  =     M_SQRT1_2;
#else /*M_SQRT1_2*/
	MV_SQRT1_2  =      M_SQRT_2; /*Borland math.h name of constant */
#endif /*M_SQRT1_2*/
#else /*M_E*/
	MV_LOG2E    =  1.4426950408889634074;
	MV_LOG10E   = 0.43429448190325182765;
	MV_LN2      = 0.69314718055994530942;
	MV_LN10     = 2.30258509299404568402;
	MV_1_PI     = 0.31830988618379067154;
	MV_2_PI     = 0.63661977236758134308;
	MV_2_SQRTPI = 1.12837916709551257390;
	MV_SQRT2    = 1.41421356237309504880;
	MV_SQRT1_2  = 0.70710678118654752440;
#endif /*M_E*/

	if (!inputInit())
	{ /* allocate space for INPUTSTRINGS and ISTRCHARS*/
		goto fatalError;
	}

	/*
	  Allocate space for globals, BATCHECHO, INPUTFILE, INPUTFILENAMES,
	  LASTINPUTWASCR, PROMPTS
	  */
	if (!batchInit())
	{
		goto fatalError;
	}
	
	if(!Symbolinit())
	{ /* create size TABLESIZE symbol table and zero it out*/
		goto fatalError;
	}

	if(!glmInit())
	{ /* allocate glm related globals */
		goto fatalError;
	}

	/* create fake handles for function symbols */
	for (nBuiltins = 0; builtins[nBuiltins].name; nBuiltins++)
	{ /* count builtins */
		;
	}
	FunctionPointers = (Symbol **) mygetpointer(nBuiltins * sizeof(Symbol *));
	FunctionSymbols = (FunctionSymbol *) mygetpointer(nBuiltins*
													  sizeof(FunctionSymbol));
	if(FunctionPointers == (Symbol **) 0 ||
	   FunctionSymbols == (FunctionSymbol *) 0)
	{
		goto fatalError;
	}

	for (i = 0;i<nBuiltins; i++)
	{ /* make fake Symbolhandle for each builtin function */
		symh = FunctionPointers + i;
		*symh = (Symbol *) (FunctionSymbols+i);
		setTYPE(symh, BLTIN);
		markFakeSymbol(symh); /* indicates it's fake if we ever need to know */
		setNDIMS(symh, 1);
		setDIM(symh, 1, 1);
		setPREV(symh, (Symbolhandle) 0);
		setNEXT(symh, (Symbolhandle) 0);
		setFPTR(symh, builtins[i].function);
		setNAME(symh, builtins[i].name);
		Addsymbol(symh);
	} /*for (i = 0;i<nBuiltins; i++)*/

	/* now install pre-defined constants */
	for (i = 0; constants[i].name; i++)
	{
		symh = RInstall(constants[i].name, 1L);
		if(symh == (Symbolhandle) 0)
		{
			goto fatalError;
		}
		if(strcmp(NAME(symh),"PI") == 0)
		{
			value = MV_PI;
		}
		else if(strcmp(NAME(symh),"E") == 0)
		{
			value = MV_E;
		}
		else if(strcmp(NAME(symh),"DEGPERRAD") == 0)
		{
			value = 45.0/MV_PI_4;
		}
		else
		{
			value = constants[i].value;
		}
		DATAVALUE(symh,0) = value;
	} /*for (i = 0; constants[i].name; i++)*/

	/*
	  Create NULLSYMBOL (value returned by print(), etc.
	  Note that it is *not* installed in the symbol table
	*/
	NULLSYMBOL = Makesymbol(NULLSYM);
	if(NULLSYMBOL == (Symbolhandle) 0)
	{
		goto fatalError;
	}
	setNAME(NULLSYMBOL, "NULLSYMBOL");

	 /* initialize special symbol CLIPBOARD */
	if (!iniClipboard(CLIPBOARDNAME))
	{
		goto fatalError;
	}

#ifdef HASSELECTION
	 /* initialize special symbol SELECTION */
	if (!iniClipboard(SELECTIONNAME))
	{
		goto fatalError;
	}
#endif /*HASSELECTION*/
	/* install pre-defined macros */
	if (!iniMacros())
	{
		goto fatalError;
	}
	
	/* install pre-defined string variables (they must all be scalar) */

	for (i = 0; stringvars[i].name; i++)
	{
		
		symh = CInstall(stringvars[i].name, strlen(stringvars[i].string) + 1);
		if(symh == (Symbolhandle) 0)
		{
			goto fatalError;
		}
		strcpy(STRINGPTR(symh), stringvars[i].string);
		cleanString(STRINGPTR(symh));
	} /*for (i = 0; stringvars[i].name; i++)*/

	/*
	   set value for MISSING and infinities
	*/

#if LONGSPERDOUBLE == 2
	setBdouble(Missing, HIMISSING, LOWMISSING);
#ifdef HASINFINITY
	setBdouble(PlusInfinity, HIPLUSINFINITY, LOWPLUSINFINITY);
	setBdouble(MinusInfinity, HIMINUSINFINITY, LOWMINUSINFINITY);
#endif /*HASINFINITY*/
#else /*LONGSPERDOUBLE == 2*/
	MISSING = MISSINGVALUE; /* may have to do something fancier than this */
#ifdef HASINFINITY
	PLUSINFINITY = 1.0/0.0;
	MINUSINFINITY = -1.0/0.0;
#endif /*HASINFINITY*/
#endif /*LONGSPERDOUBLE == 2*/

/*
  TOOBIGVALUE should be HUGE_VAL which may be infinite
*/
#ifndef NOSTRTOD
	TOOBIGVALUE = mystrtod(TOOBIGVALUESTRING,(char **) 0);
#else /*NOSTRTOD*/
#ifdef HUGE_VAL
	TOOBIGVALUE = HUGE_VAL;
#else /*HUGE_VAL*/
	TOOBIGVALUE = exp(1e200);
#endif /*HUGE_VAL*/
#endif /*NOSTRTOD*/

	OLDMISSING = OLDMISSINGVALUE;  /* -99999.9999 */
	
	setDefaultOptions(); /* set default options */
	if (prompt[0] != '\0')
	{
		strcpy((char *) DEFAULTPROMPTS[0], (char *) prompt);
	}
	strcpy((char *) PROMPTS[0], (char *) DEFAULTPROMPTS[0]);
	
#ifdef VERSION
	sprintf(OUTSTR,"MacAnova %s ",VERSION);
#ifdef TODAY
	strcat(OUTSTR,TODAY);
#endif /*TODAY*/
	symh = CInstall("VERSION", strlen(OUTSTR) + 1);
	if(symh == (Symbolhandle) 0)
	{
		goto fatalError;
	}
	strcpy(STRINGPTR(symh),OUTSTR);
	*OUTSTR = '\0';
	VERSION_ID = myduphandle(STRING(symh));
	if(VERSION_ID == (char **) 0)
	{
		goto fatalError;
	}
#endif /*VERSION*/

	/* Create global HELPFILENAME*/
	if(helpFileName == (char *) 0)
	{
		helpFileName = HELPNAME;
#ifdef UNIX
		pathName = HELPPATHNAME;
#elif	defined(VMS)
		pathName = getenv("ANOVA$HOME");
#else /*UNIX*/
		pathName = completePath;
#endif /*UNIX*/
	}
	else
	{
		pathName = NullString;
	}
	length = strlen(pathName) + strlen(helpFileName);
	
	HELPFILENAME = mygethandle(length + 1);
	if(HELPFILENAME == (char **) 0)
	{
		goto fatalError;
	}
	strcpy(*HELPFILENAME,pathName);
	strcat(*HELPFILENAME,helpFileName);

/*   Install CHARACTER scalar DATAPATH containing dataPathName */
	length = strlen(dataPathName);
#ifndef NOSEPARATOR
	if (dataPathName[length-1] != DIRSEPARATOR[0])
	{
		length++;
	}
#endif /*NOSEPARATOR*/
	symh = CInstall("DATAPATH",length + 1);
	if (symh == (Symbolhandle) 0)
	{
		goto fatalError;
	}
	strcpy(STRINGPTR(symh),dataPathName);
#ifndef NOSEPARATOR
	STRINGVALUE(symh, length-1) = DIRSEPARATOR[0];
	STRINGVALUE(symh, length) = '\0';
#endif /*NOSEPARATOR*/
/* 
   Install CHARACTER vector DATAPATHS containing dataPathName and
   macroPathName.  If they are the same, they are not duplicated
*/
	nPaths = 1;
	if (strcmp(macroPathName, dataPathName) != 0 &&
		strcmp(macroPathName, STRINGPTR(symh)) != 0)
	{
		long      length2 = strlen(macroPathName);
		
		length++; /* account for trailing null of dataPathName */
		nPaths = 2;
#ifndef NOSEPARATOR
		if (macroPathName[length2-1] != DIRSEPARATOR[0])
		{
			length2++;
		}
#endif /*NOSEPARATOR*/
		length += length2;
	}
	
	symh1 = CInstall("DATAPATHS",length + 1);
	if (symh1 == (Symbolhandle) 0)
	{
		goto fatalError;
	}
	strcpy(STRINGPTR(symh1),STRINGPTR(symh));
	if (nPaths > 1)
	{
		strcpy(skipStrings(STRINGPTR(symh1), 1), macroPathName);
#ifndef NOSEPARATOR
		STRINGVALUE(symh1,length-1) = DIRSEPARATOR[0];
		STRINGVALUE(symh1,length) = '\0';
#endif /*NOSEPARATOR*/
	}
	setDIM(symh1, 1, nPaths);
	
	/* pre-install CHARACTER scalar HOME */
	length = strlen(homePath);

	if(length > 0)
	{
#ifndef NOSEPARATOR
		if (homePath[length-1] != DIRSEPARATOR[0])
		{ /* make room for terminating separator */
			length++;
		}
#endif /*NOSEPARATOR*/
		symh = CInstall("HOME",length + 1);
		if (symh == (Symbolhandle) 0)
		{
			goto fatalError;
		}
		strcpy(STRINGPTR(symh),homePath);
#ifndef NOSEPARATOR
		/* make sure HOME terminates with DIRSEPARATOR[0]*/
		STRINGVALUE(symh, length-1) = DIRSEPARATOR[0];
		STRINGVALUE(symh, length) = '\0';
#endif /*NOSEPARATOR*/
#ifdef MSDOS
		{
			char       *pc;
		
			for (pc = STRINGPTR(symh); *pc != '\0'; pc++)
			{ /* ensure all separators are standard backslashes*/
				if (*pc == '/')
				{
					*pc = DIRSEPARATOR[0];
				}
			} /*for (pc = STRINGPTR(symh); *pc != '\0'; pc++)*/
		}
#endif /*MSDOS*/
	} /*if(length > 0)*/

	/* install CHARACTER scalar DATAFILE */
	if(dataFileName == (char *) 0)
	{
		dataFileName = DATAFILENAME;
	}
	
	length = strlen(dataFileName);

	symh = CInstall("DATAFILE", length + 1);
	if (symh == (Symbolhandle) 0)
	{
		goto fatalError;
	}

	strcpy(STRINGPTR(symh), dataFileName);

	/* Install CHARACTER scalar MACROFILE and CHARACTER vector MACROFILES*/
	for (i = 0; i < NMACROFILES + 1; i++)
	{
		macroFileNames[i] = NullString;
	}
	if (macroFileName != (char *) 0)
	{
		macroFileNames[nMacroFiles++] = macroFileName;
	}
	macroFileNames[nMacroFiles++] = MACROFILENAME;
	macroFileNames[nMacroFiles++] = TSMACROFILENAME;
	macroFileNames[nMacroFiles++] = DESIGNMACROFILENAME;
	macroFileNames[nMacroFiles++] = ARIMAMACROFILENAME;

	length = strlen(macroFileNames[0]) + 1;

	symh = CInstall("MACROFILE",length);
	if(symh == (Symbolhandle) 0)
	{
		goto fatalError;
	}
	strcpy(STRINGPTR(symh), macroFileNames[0]);
	
	length = 0;
	for (i = 0; i < nMacroFiles; i++)
	{
		length += strlen(macroFileNames[i]) + 1;
	}
	symh = CInstall("MACROFILES", length);
	if (symh == (Symbolhandle) 0)
	{
		goto fatalError;
	}
	setDIM(symh, 1, nMacroFiles);
	place = STRINGPTR(symh);
	for (i = 0; i < nMacroFiles; i++)
	{
		place = copyStrings(macroFileNames[i], place, 1);
	}
	
#ifdef READLINE
	/* Initialize line editing and history */
#if defined(INPUTRCFILE) || defined(DEFAULTINPUTRC)
	using_history(); /*added 980108*/
	rl_initialize(); /*added 980106 */
	rl_initialize_funmap();
	keymap = rl_get_keymap();
	if (keymap != (Keymap) 0)
	{ /* this binding is standard but not the default in readline 1.1 */
		rl_set_key (backwardkillword, rl_backward_kill_word, keymap);
	} /*if (keymap != (Keymap) 0)*/
#ifndef INPUTRCFILE
	pathName = OUTSTR;
	strcpy(pathName, dataPathName);
	strcat(pathName, DEFAULTINPUTRC);
#else /*INPUTRCFILE*/
	pathName = INPUTRCFILE;
#endif /*INPUTRCFILE*/
	if (pathName[0] != '\0')
	{
		readlineBindings = fmyopen(pathName, TEXTREADMODE);
		if (readlineBindings != (FILE *) 0)
		{
			fclose(readlineBindings);
			rl_read_init_file(pathName);
		} /*if (readlineBindings != (FILE *) 0)*/
	} /*if (pathName[0] != '\0')*/
#endif /*defined(INPUTRCFILE) || defined(DEFAULTINPUTRC)*/
#endif /*READLINE*/

/*
   initialize or clear virtually everything in sight
*/
	*OUTSTR = '\0';
	for (i = 0;i < MAXWDEPTH;i++)
	{
		WHILELIMITS[i] = MAXWHILE;
		FORVECTORS[i] = (double **) 0;
	}
	clearGlobals();

	INPUTFILE[0] = STDIN;
	INPUTFILENAMES[0] = &StdinName;

	for (i=1;i<MAXBDEPTH;i++)
	{
		INPUTFILE[i] = (FILE *) 0;
		INPUTFILENAMES[i] = (char **) 0;
	}

	TMPHANDLE = mygethandle(sizeof(double)); /*warehouseable length*/
	if(TMPHANDLE == (char **) 0)
	{
		goto fatalError;
	}
	INPUTSTRING = ThisInputstring = (unsigned char **) TMPHANDLE;
	(*INPUTSTRING)[0] = '\0';

	GUBED = 0; /* this is here to make it easy to change for debugging */

#ifdef DJGPP
	INTERRUPT = INTSET;
	(void) interrupted(0); /* to get DJGPP started */
	INTERRUPT = INTNOTSET;
#endif /*DJGPP*/
#if defined(BCPP) || defined (wx_msw)
	(void) _control87(EM_OVERFLOW,EM_OVERFLOW);
#endif /*BCPP||wxmsw*/

#ifdef wx_motif
#if (0) /* inititalization now done in base frame initializer */
	initializePSPrinter();
#endif
#endif /*wx_motif*/
	return ;

  fatalError:
	FatalError = 1;
	
} /*initialize()*/



