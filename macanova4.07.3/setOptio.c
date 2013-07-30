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
#pragma segment Setoptio
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

#if defined(MACINTOSH)
#include "macIface.h"
#elif defined(WXWIN)
#include "wx/wxIface.h"
#endif /*MACINTOSH*/

/*
  981007 Minor change to an error message
  981118 Added new option 'findmacros' to control automatic
         searching for undefined macros
  981218 Added new option 'plotdelay' to control wait before plotting (Motif only)
  990212 Replaced most putOUTSTR() by putErrorOUTSTR()
*/

#define MAXSEED 0x7fffffffL

/*
   The option codes must be consecutive integers starting with 1
   When adding a new option, don't forget
     (a) Makesure NOPTIONS is still correct; will be so if option put
	     before history
     (b) Update structure OptionInfo
     (c) Add initialization code to setDefaultOpt()
     (d) Add code to setopt() and getopt()
*/
enum optionCodes
{
	ISEEDS = 1,
	INSIG,
	IFORMAT,
	IWFORMAT,
	IANGLES,
	ILINES,
	IHEIGHT,
	IWIDTH,
	IERRORS,
	IPROMPT,
	IBATCHECHO,
	IRESTOREDEL,
	IDUMBPLOT,
	ISCROLLBACK,
	IUPDATE,
	IMISSING,
	IWARNINGS,
	IFSTATS,
	IPVALS,
	IFONTSIZE,
	IFONT,
	IMAXWHILE,
	ILABELABOVE,
	ILABELSTYLE,
	IINLINE,
	IFINDMACROS,
	IPLOTDELAY,
	ISAVEHISTRY,
	IHISTORY,
	NOPTIONS = IHISTORY, /* number of retrievable options */
/* the remainder are not retrievable */
	IDEFAULTS, /*(NOPTIONS+1)*/
	IGUBED,    /*(NOPTIONS+2)*/
#ifdef MACINTOSH
	IPEELS,     /*(NOPTIONS+3)*/
#endif /*MACINTOSH*/
	NOPTIONSALL  /*number of all options*/
};
/*
  since "lines" and "height" are synonyms, actual there are actuall
  NOPTIONS - 1 retrievable options
*/
typedef struct optionInfo
{
	char   *name;
	short  length;
	short  code;
} optionInfo;

/*
   option names must not exceed 10 characters and in same order as codes
   0 < Options[j].code <= NOPTIONS  regular option in this version
   -NOPTIONS < Options[j].code < 0  regular option not in this version
   Options[j].code < -NOPTIONS, non-retrievable special option
   Options[j].code > NOPTIONS, retrievable special option
*/
static optionInfo   Options[] = 
{
	{"seeds",     4, ISEEDS}, /* random number seeds (cat(posInt1,PosInt2)) */
	{"nsig",      4, INSIG},  /* sets default format to %{nsig}+7).{nsig}g */
	{"format",    6, IFORMAT},/* default format and default format for print */
	{"wformat",   7, IWFORMAT},/* default format for write */
	{"angles",    5, IANGLES}, /* value "radians","cycles", "degrees" */
	{"lines",     4, ILINES},  /* lines of text on screen */
	{"height",    5, IHEIGHT}, /* synonym for "lines" */
	{"width",     5, IWIDTH},  /* number of characters per output line */
	{"errors",    5, IERRORS}, /* limit on number of errors allowed */
	{"prompt",    6, IPROMPT}, /* default prompt */
	{"batchecho", 9, IBATCHECHO}, /* should commands be echoed in batch mode */
	{"restoredel",10,IRESTOREDEL}, /*default for 'delete' on restore()*/
	{"dumbplot",  4, IDUMBPLOT}, /*default for 'dumb' on plotting commands*/
#ifdef SCROLLABLEWINDOW
	{"scrollback",10, ISCROLLBACK}, /*default for 'scrollback' on help(), etc*/
#else /*SCROLLABLEWINDOW*/
	{"scrollback",10, -ISCROLLBACK}, /*not available in this version*/
#endif /*SCROLLABLEWINDOW*/
#ifdef ACTIVEUPDATE
	{"update",    6, IUPDATE}, /*enables active updating of screen */
#else /*ACTIVEUPDATE*/
	{"update",    6, -IUPDATE}, /*not available in version */
#endif /*ACTIVEUPDATE*/
	{"missing",   7, IMISSING},  /*filler to be used for MISSING*/
	{"warnings",  7, IWARNINGS},  /* should WARNING: be printed */
	{"fstats",    5, IFSTATS},  /* default value for glm keyword 'fstat'*/
	{"pvals",     4, IPVALS},   /* default value for glm keyword 'pvals'*/
#ifdef CANSETFONTS
	{"fontsize",  7, IFONTSIZE},  /*"fontName Size"*/
	{"font",      4, IFONT},  /*"fontName Size"*/
#else /*CANSETFONTS*/
	{"fontsize",  7, -IFONTSIZE},  /*not available in version*/
	{"font",      4, -IFONT},  /*not available in version*/
#endif /*CANSETFONTS*/
	{"maxwhile",  8, IMAXWHILE},  /* max number of while(){} repetitions */
	{"labelabove",6, ILABELABOVE}, /* last coord label above */
	{"labelstyle",6, ILABELSTYLE}, /* default paren for labelling */ 
	{"inline",    6, IINLINE},     /* default type of macro expansion */
	{"findmacros",7, IFINDMACROS}, /* controls automatic search for macros */
#ifdef wx_motif
	{"plotdelay", 7, IPLOTDELAY},  /* controls wait before opening graph window*/
#else /*wx_motif*/
	{"plotdelay", 7, -IPLOTDELAY},  /* not available in version*/
#endif /*wx_motif*/
#ifdef SAVEHISTORY
	{"savehistry",8, ISAVEHISTRY}, /* default value for save keyword history */
	{"history",   4, IHISTORY}, /*number of history lines to save*/
#else /*SAVEHISTORY*/
	{"savehistry",8, -ISAVEHISTRY},/* not available in version */
	{"history",   4, -IHISTORY}, /*not available in version*/
#endif /*SAVEHISTORY*/
	{"defaults",  7, IDEFAULTS},	/* reset all options to defaults*/
	{"GUBED",     5, -IGUBED},    /* DEBUG backwards, for debugging */
#ifdef MACINTOSH
	{"PEELS",     5, -IPEELS},    /* SLEEP backwards for testing waitNextEvent */
#endif /*MACINTOSH*/
	{(char *) NULL, 0, 0}
};

#ifdef CANSETFONTS
#define DEFAULTFONT "default"

#ifdef MACINTOSH
#define FONTNAMESIZE  27
#define MINFONTSIZE    9  
#define MAXFONTSIZE   64
#endif /*MACINTOSH*/

#define OKFONTSPEC       0
#define BADFONTSIZE     -1
#define FONTNAMETOOLONG -2
#define BADFONTSPEC     -3

/*
	Function to decode a font of the form "FontName size"
	If font is of form "FontName", set *fontsize to -1
*/
static int decodeFont(char *font, char **fontName, long *fontSize)
{
	static char     name[FONTNAMESIZE+1];
	long            length = strlen(font);
	char           *sizePtr = font + length - 1;
	WHERE("decodeFont");
	
	*fontSize = -1;
	while(sizePtr > font && *sizePtr == ' ')
	{ /* skip backwara over trailing spaces */
		length--;
		sizePtr--;
	}
	if (isdigit(*sizePtr))
	{ /* may be of form "Name Size" */
		while(sizePtr > font && isdigit(*sizePtr))
		{
			sizePtr--;
		}
		if (*sizePtr == ' ' || *sizePtr == '-' || *sizePtr == '+')
		{
			if (*sizePtr == ' ')
			{
				sizePtr++;
			}
			/* sizePtr should point to number to be decoded */
			if (*(sizePtr-1) != ' ')
			{ /* not of form "Name Number" */
				*fontSize = -1;
				length = strlen(font);
			}
			else
			{
				if (sscanf(sizePtr, "%ld", fontSize) != 1 ||
					*fontSize < MINFONTSIZE || *fontSize > MAXFONTSIZE)
				{
					return (BADFONTSIZE);
				}
				sizePtr--;
				while(sizePtr > font && *sizePtr == ' ')
				{
					sizePtr--;
				}
				sizePtr++;
				length = sizePtr - font;
			}
		} /*if (*sizePtr == ' ' || *sizePtr == '-' || *sizePtr == '+')*/
	} /*if (isdigit(*sizePtr))*/
	if (length > FONTNAMESIZE)
	{
		return (FONTNAMETOOLONG);
	}
	if (length == 0)
	{
		return (BADFONTSPEC);
	}
	strncpy(name, font, length);
	name[length] = '\0';
	*fontName = name;

	return (OKFONTSPEC);
} /*decodeFont()*/

/*
   Function to set the font for the current window and the default font used
   in new windows.  If newFontSize > 0,  the font size is also set.  Called
   from functions setDefaultOptions() and setOpt().

   Actual function called depends on platform.

   Note: setDefaultOptions() and hence setNewFont() is called from
   initialize() which might be (and is, on a Macintosh) before any
   windows have been created.  Hence special care needs to be taken in the
   code implementing the change.
*/

static void setNewFont(char * newFontName, short newFontSize)
{
#ifdef MACINTOSH
	macSetNewFont(newFontName, newFontSize);
#else /*MACINTOSH*/
	/* add calls needed on other platforms here */
#endif /*MACINTOSH*/
} /*setNewFont()*/

/*
	returns 1 if newFontName and newFontSize match current
	does not check newFontName when (char *) 0
	does not check newFontSize when <= 0
*/
static int    isCurrentFont(char * newFontName, short newFontSize)
{
	int       reply = 1;
#ifdef MACINTOSH
	if (newFontName != (char *) 0)
	{
		CtoPstr(newFontName);
		reply = (str255cmp((unsigned char *) newFontName, CmdFontName) == 0);
		PtoCstr((unsigned char *) newFontName);
	}
	if (reply && newFontSize > 0)
	{
		reply = (newFontSize == CmdFontSize);
	}
#endif /*MACINTOSH*/
	return (reply);
} /*isCurrentFont()*/

static void   setCurrentFontName(char * fontName)
{
#ifdef MACINTOSH
	str255cpy((STR255) fontName, CmdFontName);
	PtoCstr((unsigned char *) fontName);
#endif /*MACINTOSH*/
} /*setCurrentFontName()*/
#endif /*CANSETFONTS*/

/*
   set default values of all options
   971230 set default for restoredel to T to conform with help file
*/

void setDefaultOptions(void)
{
	int          i;
#ifndef MACINTOSH
	long         height, width;
#endif /*MACINTOSH*/
	WHERE("setDefaultOptions");

	/* ISEEDS */
	RANDS1 = 0;
	RANDS2 = 0;

	/* IHEIGHT and IWIDTH */
#ifndef MACINTOSH
	BATCHECHO[0] = (ISATTY != (ITTYIN | ITTYOUT));
	height = (ISATTY & ITTYOUT) ?  DEFAULTSCREENHEIGHT : 0;
	width = DEFAULTSCREENWIDTH;
	installScreenDims(width, height);
#else /*MACINTOSH*/
	if (teCmd != (TEHandle) 0)
	{
		setCmdScreen();
	}
#endif /*MACINTOSH*/

	/* INSIG and IFORMAT */
	PRINTFORMAT[0] = DEFAULTPRINTDEC+7;
	PRINTFORMAT[1] = DEFAULTPRINTDEC;
	PRINTFORMAT[2] = (long) 'g';
	installFormat(PRINTFORMAT[0],PRINTFORMAT[1],PRINTFORMAT[2]);
	saveFormat(); /* save copy of format and SCREENWIDTH and SCREENHEIGHT */

	/* IWFORMAT */
	WRITEFORMAT[0] = DEFAULTWRITEDEC+7;
	WRITEFORMAT[1] = DEFAULTWRITEDEC;
	WRITEFORMAT[2] = (long) 'g';

	/* IANGLES */
	ANGLES2RADIANS = 1.0;  /* "radians" */

	/* IERRORS */
	MAXERRORS1 = MAXERRORS = 0;

	/* IPROMPT */
	for (i = 0; i <= BDEPTH; i++)
	{
		strcpy((char *) PROMPTS[i], (char *) DEFAULTPROMPTS[i]);
	}
	
	 /*IBATCHECHO*/
	DEFAULTBATCHECHO = 1;

	 /*IRESTOREDEL*/
	DEFAULTRESTOREDEL = 1;

	 /*IDUMBPLOT*/
#ifdef MULTIPLOTWINDOWS
	DEFAULTDUMBPLOT = !UseWindows;
#else /*MULTIPLOTWINDOWS*/
	DEFAULTDUMBPLOT = 0;
#endif /*MULTIPLOTWINDOWS*/

#ifdef SCROLLABLEWINDOW
	 /*ISCROLLBACK*/
	SCROLLBACK = DEFAULTSCROLLBACK;
#endif /*SCROLLABLEWINDOW*/

	/* IUPDATE */
#ifdef ACTIVEUPDATE
	ActiveUpdate = ACTIVEUPDATE;
#else /*ACTIVEUPDATE*/
	ActiveUpdate = 0;
#endif /*ACTIVEUPDATE*/

	/* IMISSING */
	strncpy(NAMEFORMISSING,DEFAULTNAMEFORMISSING,LENGTHMISSING);
	NAMEFORMISSING[LENGTHMISSING] = '\0';
#ifdef HASINFINITY
	strncpy(NAMEFORINFINITY,DEFAULTNAMEFORINFINITY,LENGTHINFINITY);
	NAMEFORINFINITY[LENGTHINFINITY] = '\0';
#endif /*HASINFINITY*/

	/* IWARNINGS */
	PRINTWARNINGS = 1;

	/* IFSTATS and IPVALS */
	PRINTFSTATS = DEFAULTPRINTFSTATS;
	PRINTPVALS = DEFAULTPRINTPVALS;

#ifdef CANSETFONTS
	/* IFONTSIZE and IFONT */
	setNewFont(DEFAULTFONT, -1);
#endif /*CANSETFONTS*/

	/* IMAXWHILE */
	MAXWHILE = DEFAULTMAXWHILE;
	
	/* ILABELATTOP */
	USECOLLABS = DEFAULTUSECOLLABS;
	
	/* ILABELSTYLE */
	LEFTBRACKET[0] = DEFAULTLEFTBRACKET[0];
	RIGHTBRACKET[0] = Rparens[strchr(Lparens, LEFTBRACKET[0]) - Lparens];
	
	/* IINLINE */
	EXPANDINLINE = DEFAULTEXPANDINLINE;

	/* IFINDMACROS */
	MACROSEARCH = DEFAULTMACROSEARCH;

#ifdef wx_motif
	FocusDelay = DefaultFocusDelay;
#endif /*wx_motif*/

#ifdef SAVEHISTORY
	/* ISAVEHISTRY */
	DEFAULTSAVEHISTORY = ISATTY & ITTYIN;
	/* IHISTORY */
	initHistory(DEFAULTHISTORYLENGTH);
#endif /*SAVEHISTORY*/

} /*setDefaultOptions()*/

/*
  functions to set and retrieve various options using keywords
  usage:
  	setoptions(option1,...,optionk)
  where optionj is of the form keywordj:valuej, e.g., 'nsig:5'

    setoptions(str)
	where str is structure whose component names are option names

	setoptions(default:T) resets all options to their default values

	getoptions() or getoptions(all:T)
	  returns a structure with all current values of options

	getoptions(keyword1:T [,keyword2:T...])
	  returns the specified options.  If there are more than one specified,
	  a structure is returned.

	getoptions(all:T, keyword1:F, ...)
	  returns all except the indicated options

  For backward compatibility, for getoptions, you can use, for example,
  "nsig" instead of nsig:T

  In addition to the public options (id numbers > 0) there are non-public
  options that may be set but not retrieved.  Chief among these is GUBED
  (DEBUG backwards) which is used to control various features useful in
  debugging

  If a new option is added, you must do all of the following
   (a)  Add a new item to enum optionCodes (above)
   (b)  Add an new element to optionInfo (above)
   (c)  Add a new global for the option value, probably in globkb.h
   (d)  Modify code where the option has effect to use the global
   (e)  Add code to setDefaultOptions() to set default value (above)
   (f)  Add new case and code to getOpt() to retrieve the value (below)
   (g)  Add new case and code to setOpt() to set the value (below)

  960418 Added option restoredel to provide default value for keyword delete
         on restore()
  960419 Added option dumbplot to provide default value for keyword dumb
         on plotting commands such as boxplot(), plot() and showplot()
  970211 Fixed bug (dimension of whichOptions)
  970313 Add options labelstyle and labelabove to control default labelling
         of unlabeled vectors, matrices and arrays
  970619 Added option inline to control default expansion of macros.
  971226 Fixed handling of "all" so that option 'lines' not set; only 'height'
  980624 Add option savehistry to provide default value for save()/
         asciisave() option 'history'
  981219 Add option plotdelay for Motif version to provide default delay
         length before drawing to graph window to overcome failure to
         set focus problem
*/

Symbolhandle    setoptions(Symbolhandle list)
{
	Symbolhandle     option, arg;
	long             isStructure = 0, nOptions = 0;
	long             set, i, j, nargs = NARGS(list), ncomps = nargs;
	long             optionNo, getit, argno;
	short            whichOptions[NOPTIONSALL];
	char            *keyword;
	char            *notavailFmt =
		"WARNING: option %s is not available in this version%s";
	WHERE("setoptions");

	OUTSTR[0] = '\0';
	set = (strncmp(FUNCNAME,"set",3) == 0);

	if(nargs == 1)
	{
		arg = COMPVALUE(list,0);
		if(!set && arg == (Symbolhandle) 0)
		{/* empty argument list to getoptions(); get everything */
			option = getOpt((short *) 0);
			return (option);
		}
		else if(set && arg != (Symbolhandle) 0 && TYPE(arg) == STRUC)
		{ /* single STRUCT argument to setoptions() */
			isStructure = 1;
			list = arg;
			ncomps = NCOMPS(list);
		}
	} /*if(nargs == 1)*/
	else if (set && (argno = findKeyword(list,"default",0)) >= 0 &&
			 isTorF(arg = COMPVALUE(list,argno)) && DATAVALUE(arg,0) != 0.0)
	{
		sprintf(OUTSTR,
				"ERROR: '%s:T' must be the only argument", isKeyword(arg));
		goto errorExit;
	}

	for (i = 0;i < NOPTIONSALL;i++)
	{
		whichOptions[i] = 0;
	}
	
/*
   At this point either list is the actual argument list (isStructure == 0)
   or the command is setOptions(Struc) and list is Struc (isStructure != 0)

   If the command is setOptions() and isStructure == 0, all arguments must be
   keyword phrases.

   If the command is getOptions() all arguments must be keyword phrases or
   CHARACTER scalars containing option names.
*/
	for (i=0;i<ncomps;i++)
	{
		arg = COMPVALUE(list,i);
		if(arg == (Symbolhandle) 0)
		{
			sprintf(OUTSTR,"ERROR: missing argument to %s()", FUNCNAME);
		}
		else if(isStructure || (keyword = isKeyword(arg)) != (char *) 0)
		{
			if(isStructure)
			{
				keyword = NAME(arg);
			} /*if(isStructure)*/
			else
			{ /* argument is keyword*/
				if(!set && strcmp(keyword, "all") == 0)
				{
					if (!isTorF(arg) || DATAVALUE(arg, 0) == 0.0)
					{
						sprintf(OUTSTR,
								"ERROR: %s must have value T", keyword);
						goto errorExit;
					}
					
					nOptions = 0;
					for(j=0;j<NOPTIONS;j++)
					{ /* set everything T*/
						getit = (Options[j].code > 0 && j != ILINES - 1) ? 1 : 0;
						whichOptions[j] = getit;
						nOptions += getit;
					} /*for(j=0;j<NOPTIONS;j++)*/
					continue;
				} /*if(!set && strcmp(keyword, "all") == 0)*/
			} /*if(isStructure){}else{}*/
			
			if(!(optionNo = isOption(keyword)) || !set && optionNo < -NOPTIONS)
			{
				sprintf(OUTSTR,"ERROR: %s is unrecognized %s %s()", keyword,
						(isStructure) ? "component name in argument to" : 
							"keyword on",
						FUNCNAME);
			}
			else if (-NOPTIONS <= optionNo && optionNo < 0)
			{ /* option unavailable in version*/
				sprintf(OUTSTR, notavailFmt, Options[-optionNo-1].name,
						(set) ? "; not set" : "; ignored");
			}
			else if (optionNo > NOPTIONS && (!set || optionNo != IDEFAULTS))
			{
				sprintf(OUTSTR,
						"ERROR: option %s illegal on %s()",
						Options[optionNo-1].name, FUNCNAME);
			}
			else if(!set)
			{ /* getoptions() with keyword argument */
				keyword = Options[optionNo-1].name;
				if (!isTorF(arg))
				{
					notTorF(keyword);
					goto errorExit;
				}

				nOptions -= whichOptions[optionNo - 1];
				getit = (DATAVALUE(arg, 0) != 0.0) ? 1 : 0;
				whichOptions[optionNo - 1] = getit;
				nOptions += getit;
			} 
			else if (optionNo > 0)
			{
				whichOptions[optionNo - 1] = 1;
			}
		}
		else if(set)
		{ /* setoptions() with non-keyword argument*/
			sprintf(OUTSTR,
					"ERROR: %ld%s argument to %s() not keyword phrase",
					i+1, n_th(i+1), FUNCNAME);
		}
		else
		{ /*getoptions() with non-keyword argument*/
			if (!isCharOrString(arg) ||
				!(optionNo = isOption(STRINGPTR(arg))) ||
				optionNo < -NOPTIONS)
			{ /* can get values of public options only */
				sprintf(OUTSTR,
						"ERROR: non-keyword argument to %s() must be CHARACTER option name",
						FUNCNAME);
			}
			else if(optionNo < 0)
			{
				sprintf(OUTSTR, notavailFmt, Options[-optionNo-1].name,
						"; ignored"); 
			}
			else
			{
				whichOptions[optionNo - 1] = 1;
				nOptions++;
			}
		}
		
		if (OUTSTR[0] == 'W')
		{/* WARNING only, don't quit */
			putErrorOUTSTR();
		}
		else if (*OUTSTR)
		{
			goto errorExit;
		}
	} /*for (i=0;i<ncomps;i++)*/

	if(!set && nOptions == 0)
	{
		sprintf(OUTSTR,
				"ERROR: no legal options specified for %s()", FUNCNAME);
		goto errorExit;
	}

	if (whichOptions[ILINES-1] && whichOptions[IHEIGHT-1])
	{
		strcpy(OUTSTR,
			   "WARNING: 'height' and 'lines' are synonyms; 'lines' ignored");
		putErrorOUTSTR();
		whichOptions[ILINES-1] = 0;
	} /*if (whichOptions[ILINES-1] && whichOptions[IHEIGHT-1])*/

	if(!set)
	{ /* getoptions */
		option = getOpt(whichOptions);
	} /*if(!set)*/
	else
	{ /* setoptions */
		setOpt(list, 0);
		option = NULLSYMBOL;
	} /*if(!set){...}else{...} */
	
	return (option);

  errorExit:
	putErrorOUTSTR();
	
	return (0);
	
} /*setoptions()*/

#define TEMPSTRINGSIZE 100

Symbolhandle getOpt(short * optionlist)
{
	Symbolhandle       optionStructure = (Symbolhandle) 0;
	Symbolhandle       symh = (Symbolhandle) 0;
	char              *optionName;
	char               tempstring[TEMPSTRINGSIZE+1];
	long               i, jcomp, all = (optionlist == (short *) 0);
	long               nOptions;
	long               getit, option;
	WHERE("getOpt");
	
	OUTSTR[0] = '\0';
	nOptions = 0;
	for (i = 0; i < NOPTIONS; i++)
	{
		getit = (all && Options[i].code > 0 || !all && optionlist[i]) ? 1 : 0;
		nOptions += getit;
	} /*for (i = 0; i < NOPTIONS; i++)*/
	if (all)
	{
		nOptions--; /*adjust for height and lines being the same */
	}
	
	/*
	   nOptions must be > 0 or we wouldn't be in getOpt()
	*/
	if(nOptions > 1)
	{ /* create structure for output */
		optionStructure = StrucInstall(SCRATCH,nOptions);
		if(optionStructure == (Symbolhandle) 0)
		{
			goto errorExit;
		}
	} /*if(nOptions > 1)*/
	else
	{ /* create SCRATCH variable for output */
		symh = Install(SCRATCH,REAL); /* may be CHAR or LOGIC */
		if(symh == (Symbolhandle) 0)
		{
			goto errorExit;;
		}
		setNDIMS(symh,1);
		setDIM(symh,1,1);
	} /*if(nOptions > 1){...}else if(nOptions == 1){...} */
	
	jcomp = 0;
	
	for (i=0;i<NOPTIONS;i++)
	{
		option = Options[i].code;
		optionName = Options[i].name;
		if (all && option > 0 && option != ILINES || !all && optionlist[i])
		{
			if(nOptions > 1)
			{
				symh = COMPVALUE(optionStructure,jcomp++) = Makesymbol(REAL);
				/* Note symh may end up CHAR or LOGIC so don't use Makereal*/
				if(symh == (Symbolhandle) 0)
				{
					goto errorExit;
				}
				setNDIMS(symh,1);
				if(nOptions > 1)
				{
					setNAME(symh,optionName);
				}
			} /*if(nOptions > 1)*/
		
			switch (option)
			{
			  case ISEEDS:
				TMPHANDLE = mygethandle(2 * sizeof(double));
				setDATA(symh,(double **) TMPHANDLE);
				if(TMPHANDLE == (char **) NULL)
				{
					goto errorExit;
				}
				setTYPE(symh,REAL);
				setDIM(symh,1,2);
				DATAVALUE(symh,0) = (double) RANDS1;
				DATAVALUE(symh,1) = (double) RANDS2;
				break;

				/* single double value */
			  case INSIG:

				/* single LOGICAL values */
			  case IBATCHECHO:
			  case IRESTOREDEL:
			  case IDUMBPLOT:
#ifdef SCROLLABLEWINDOW
			  case ISCROLLBACK:
#endif /*SCROLLABLEWINDOW*/
#ifdef ACTIVEUPDATE
			  case IUPDATE:
#endif /*ACTIVEUPDATE*/
			  case IWARNINGS:
			  case IFSTATS:
			  case IPVALS:
			  case ILABELABOVE:
			  case IINLINE:
#ifdef SAVEHISTORY
			  case ISAVEHISTRY:
#endif /*SAVEHISTORY*/				
				TMPHANDLE = mygethandle(sizeof(double));
				setDATA(symh,(double **) TMPHANDLE);
				if(TMPHANDLE == (char **) NULL)
				{
					goto errorExit;
				}	
				setDIM(symh,1,1);
				setTYPE(symh, (option == INSIG) ? REAL : LOGIC);
				if(option == INSIG)
				{
					DATAVALUE(symh,0) = (double) PRINTFORMAT[1];
				}
				else if(option == IBATCHECHO)
				{
					DATAVALUE(symh,0) = (DEFAULTBATCHECHO) ? 1.0 : 0.0;;
				}
				else if(option == IRESTOREDEL)
				{
					DATAVALUE(symh,0) = (DEFAULTRESTOREDEL) ? 1.0 : 0.0;;
				}
				else if(option == IDUMBPLOT)
				{
					DATAVALUE(symh,0) = (DEFAULTDUMBPLOT) ? 1.0 : 0.0;;
				}
#ifdef SCROLLABLEWINDOW
				else if(option == ISCROLLBACK)
				{
					DATAVALUE(symh,0) = (SCROLLBACK) ? 1.0 : 0.0;;
				}
#endif /*SCROLLABLEWINDOW*/
#ifdef ACTIVEUPDATE
				else if(option == IUPDATE)
				{
					DATAVALUE(symh,0) = (ActiveUpdate) ? 1.0 : 0.0;;
				}
#endif /*ACTIVEUPDATE*/
				else if(option == IWARNINGS)
				{
					DATAVALUE(symh,0) = (PRINTWARNINGS) ? 1.0 : 0.0;
				}
				else if(option == IFSTATS)
				{
					DATAVALUE(symh,0) = (PRINTFSTATS) ? 1.0 : 0.0;
				}
				else if(option == IPVALS)
				{
					DATAVALUE(symh,0) = (PRINTPVALS) ? 1.0 : 0.0;
				}
				else if(option == ILABELABOVE)
				{
					DATAVALUE(symh,0) = (USECOLLABS) ? 1.0 : 0.0;
				}
				else if(option == IINLINE)
				{
					DATAVALUE(symh,0) = (EXPANDINLINE) ? 1.0 : 0.0;
				}
#ifdef SAVEHISTORY
				else if(option == ISAVEHISTRY)
				{
					DATAVALUE(symh,0) = (DEFAULTSAVEHISTORY) ? 1.0 : 0.0;
				}
#endif /*SAVEHISTORY*/
				else
				{				/* should not happen */
					setMissing(DATAVALUE(symh,0));
				}
				break;
				
				/* single string values */
			  case IFORMAT:
			  case IWFORMAT:
			  case IANGLES:
			  case IPROMPT:
			  case IMISSING:
			  case ILABELSTYLE:
			  case IFINDMACROS:
#ifdef CANSETFONTS
			  case IFONT:
#endif /*CANSETFONTS*/
				if(option == IFORMAT)
				{
					sprintf(tempstring,"%ld.%ld%c",
							PRINTFORMAT[0],PRINTFORMAT[1],(char) PRINTFORMAT[2]);
				}
				else if(option == IWFORMAT)
				{
					sprintf(tempstring,"%ld.%ld%c",
							WRITEFORMAT[0],WRITEFORMAT[1],(char) WRITEFORMAT[2]);
				}
				else if(option == IANGLES)
				{
					double angles2cycles = 8.0*MV_PI_4;
					double angles2degrees = MV_PI_4/45.0;

#ifdef DJGPP
/*
	971231
	Following line added to overcome problem arising from optimization
	Without it, the "degrees" value of ANGLES2RADIANS is be recognized
	It can also be fixed by compiling without -O flag
*/
					fprintf(stdout,"");
#endif /*DJGPP*/
					if(ANGLES2RADIANS == 1.0)
					{
						strcpy(tempstring,"radians");
					}
					else if(ANGLES2RADIANS == angles2cycles)
					{
						strcpy(tempstring,"cycles");
					}
					else if(ANGLES2RADIANS == angles2degrees)
					{
						strcpy(tempstring,"degrees");
					}
					else		/* should not happen */
					{
						strcpy(tempstring,"radians");
						ANGLES2RADIANS = 1.0;
					}
				}
				else if(option == IPROMPT)
				{
					strncpy(tempstring, (char *) PROMPTS[0], TEMPSTRINGSIZE);
				}
				else if(option == IMISSING)
				{
					strncpy(tempstring, NAMEFORMISSING, TEMPSTRINGSIZE);
				}
				else if (option == ILABELSTYLE)
				{
					strcpy(tempstring, LEFTBRACKET);
				}
				else if (option == IFINDMACROS)
				{
					if (MACROSEARCH == noMacroSearch)
					{
						strcpy(tempstring, "no");
					}
					else if (MACROSEARCH == verboseMacroSearch)
					{
						strcpy(tempstring, "yes");
					}
					else
					{
						strcpy(tempstring, "silent");
					}
				}				
#ifdef CANSETFONTS
#ifdef MACINTOSH
				else if(option == IFONT)
				{
					str255cpy((STR255) tempstring, CmdFontName);
					PtoCstr((unsigned char *) tempstring);
				}				
#endif /*MACINTOSH*/
#endif /*CANSETFONTS*/
				tempstring[TEMPSTRINGSIZE] = '\0';
				
				TMPHANDLE = mygethandle((strlen(tempstring)+1)*sizeof(char));
				setSTRING(symh,TMPHANDLE);
				if(TMPHANDLE == (char **) NULL)	
				{
					goto errorExit;
				}	
				setTYPE(symh,CHAR);
				setDIM(symh,1,1);
				strcpy(STRINGPTR(symh),tempstring);
				break;

				/* single integer values */
			  case ILINES:
			  case IHEIGHT:
			  case IWIDTH:
			  case IERRORS:
#ifdef CANSETFONTS
			  case IFONTSIZE:
#endif /*CANSETFONTS*/
			  case IMAXWHILE:
#ifdef wx_motif
			  case IPLOTDELAY:
#endif /*wx_motif*/
#ifdef SAVEHISTORY
			  case IHISTORY:
#endif /*SAVEHISTORY*/
			  case IGUBED:
#ifdef MACINTOSH
			  case IPEELS:
#endif /*MACINTOSH*/
				TMPHANDLE = mygethandle(sizeof(double));
				setDATA(symh,(double **) TMPHANDLE);
				if(TMPHANDLE == (char **) NULL)
				{
					goto errorExit;
				}	
				setTYPE(symh,REAL);
				setDIM(symh,1,1);

				if(option == ILINES || option == IHEIGHT)
				{
					DATAVALUE(symh,0) = (double) SCREENHEIGHT;
				}
				else if(option == IWIDTH)
				{
					DATAVALUE(symh,0) = (double) SCREENWIDTH;
				}
				else if(option == IERRORS)
				{
					DATAVALUE(symh,0) = (double) MAXERRORS;
				}
#ifdef CANSETFONTS
#ifdef MACINTOSH
				else if(option == IFONTSIZE)
				{
					DATAVALUE(symh,0) = (double) CmdFontSize;
				}
#endif /*MACINTOSH*/
#endif /*CANSETFONTS*/
				else if(option == IMAXWHILE)
				{
					DATAVALUE(symh,0) = (double) MAXWHILE - 1;
				}
#ifdef wx_motif
				else if(option == IPLOTDELAY)
				{
					DATAVALUE(symh,0) = (double) FocusDelay;
				}
#endif /*wx_motif*/
#ifdef SAVEHISTORY
				else if(option == IHISTORY)
				{
					DATAVALUE(symh,0) = (double) HISTORY;
				}
#endif /*SAVEHISTORY*/
				else if(option == IGUBED)
				{
					DATAVALUE(symh,0) = (double) GUBED;
				}
#ifdef MACINTOSH
				else if(option == IPEELS)
				{
					DATAVALUE(symh,0) =
					  (double) (CmdSleep +
								100 * BackgroundSleep +
								10000 * FgInterruptInterval +
								1000000 * InterruptInterval);
				}
#endif /*MACINTOSH*/
				break;

			  default:
				/* should not get here */
				sprintf(OUTSTR,"WARNING: unrecognized option '%s'",optionName);
				putErrorOUTSTR();
			} /*switch (option)*/
		} /*if (all && option > 0 || !all && optionlist[i])*/
	} /*for(i=0;i<NOPTIONS;i++)*/

	return ((nOptions > 1) ? optionStructure : symh);

  errorExit:
	
	if (nOptions > 1)
	{
		Removesymbol(optionStructure);
	}
	else
	{
		Removesymbol(symh);
	}

	return  (0);
} /*getOpt()*/


/*
  Function to do the actual work of setting options, called from setoptions()
  or restore().  If called from setoptions() list should either be an
  argument list (type LIST) of keyword arguments, in which case their names
  have already been checked for validity, or a structure (type STRUC), the
  names of whose components are option names which have also been checked
  for validity.  If called from restore(), list is a structure whose names
  have not been checked.  In any case, if an unrecognized name is found and
  quiet == 0, a warning message is printed. 
*/  

void           setOpt(Symbolhandle list, long quiet)
{
	long            icomp, k, ncomps, length;
	double          value[3];
	long            isStructure;
	long            fmt[3];
	long            option, logValue;
	Symbolhandle    arg;
	Symbolhandle    symhLines = (Symbolhandle) 0;
	Symbolhandle    symhHeight = (Symbolhandle) 0;
	char           *keyword;
	int             reply;
	long            newWidth = -1, newLines = -1;
	char           *mustbe = "WARNING: value for %s must be %s";
	char           *mustbeChar =
		"WARNING: value for %s must be CHARACTER scalar";
	char           *mustbe2 = "WARNING: value for %s must be integer >= %ld";
	char           *toolongChar =
		"WARNING: %s must have no more than %ld characters";
#ifdef CANSETFONTS
	int             changeFont = 0;
	long            newFontSize = -1, fontSize;
	char           *newFontName = (char *) 0;
#endif /*CANSETFONTS*/
	WHERE("setOpt");
	
/*
   it should be the case that all arguments should be valid option names
   Also, unless called from restore() (quiet != 0), any unavailable options
   should have been diagnosed
*/
	isStructure = (TYPE(list) == STRUC);
	ncomps = NCOMPS(list);
	for (icomp=0;icomp<ncomps;icomp++)
	{
		arg = COMPVALUE(list,icomp);
		if(isStructure)
		{
			keyword = NAME(arg);
		}
		else
		{
			keyword = isKeyword(arg);
			/* should not need to check that non-null */
		}
		option = isOption(keyword);
		if (option < -NOPTIONS || option > 0)
		{
			OUTSTR[0] = '\0';
			option = (option < 0) ? -option  : option;
			keyword = Options[option-1].name;
			length = symbolSize(arg);

			switch (option)
			{
			  case ISEEDS:
				if(TYPE(arg) != REAL || length != 2)
				{
					sprintf(OUTSTR, mustbe, keyword, "vector of two integers");
				}
				else if((value[0] = DATAVALUE(arg,0)) < 0 || 
						(value[1] = DATAVALUE(arg,1)) < 0 ||
						value[0] > MAXSEED ||
						value[1] > MAXSEED ||
						value[0] != floor(value[0]) ||
						value[1] != floor(value[1]))
				{
					sprintf(OUTSTR,
							"WARNING: %s must be positive integers <= %ld",
							keyword,MAXSEED);
				}
				else
				{
					RANDS1 = (long) value[0];
					RANDS2 = (long) value[1];
				}
				break;
			
			  case INSIG:
			
				if (!isInteger(arg, NONNEGATIVEVALUE) ||
					(value[0] = DATAVALUE(arg, 0)) > 20)
				{
					sprintf(OUTSTR, mustbe, keyword, "integer between 0 and 20");
				}
				else
				{
					PRINTFORMAT[0] = (long) value[0] + 7;
					PRINTFORMAT[1] = (long) value[0];
					PRINTFORMAT[2] = (long) 'g';
					installFormat(PRINTFORMAT[0],PRINTFORMAT[1],
								  PRINTFORMAT[2]);
				}
				break;

			  case IBATCHECHO:
			  case IRESTOREDEL:
#ifdef SCROLLABLEWINDOW
			  case ISCROLLBACK:
#endif /*SCROLLABLEWINDOW*/
			  case IDUMBPLOT:
#ifdef ACTIVEUPDATE
			  case IUPDATE:
#endif /*ACTIVEUPDATE*/
			  case IWARNINGS:
			  case IFSTATS:
			  case IPVALS:
			  case IDEFAULTS:
			  case ILABELABOVE:
			  case IINLINE:
#ifdef SAVEHISTORY
			  case ISAVEHISTRY:
#endif /*SAVEHISTORY*/
				if (!isTorF(arg))
				{
					sprintf(OUTSTR, mustbe, keyword, "T of F");
				}
				else
				{
					logValue = DATAVALUE(arg,0) != 0.0;
					if(option == IBATCHECHO)
					{
						DEFAULTBATCHECHO = logValue;
					}
					else if (option == IRESTOREDEL)
					{
						DEFAULTRESTOREDEL = logValue;
					}
					else if (option == IDUMBPLOT)
					{
#ifdef MACINTOSH
						if (!UseWindows && !logValue)
						{
							sprintf(OUTSTR,
									"WARNING: %s:F illegal in non-interactive mode",
									keyword);
							logValue = 1;
						}
#endif /*MACINTOSH*/
						DEFAULTDUMBPLOT = logValue;
					}
#ifdef SCROLLABLEWINDOW
					else if(option == ISCROLLBACK)
					{
						SCROLLBACK = logValue;
					}
#endif /*SCROLLABLEWINDOW*/
#ifdef ACTIVEUPDATE
					else if(option == IUPDATE)
					{
						ActiveUpdate = logValue;
					}
#endif /*ACTIVEUPDATE*/
					else if(option == IWARNINGS)
					{
						PRINTWARNINGS = logValue;
					}
					else if(option == IFSTATS)
					{
						PRINTFSTATS = logValue;
					}
					else if(option == IPVALS)
					{
						PRINTPVALS = logValue;
					}
					else if(option == ILABELABOVE)
					{
						USECOLLABS = logValue;
					}
					else if(option == IINLINE)
					{
						EXPANDINLINE = logValue;
					}
#ifdef SAVEHISTORY
					else if(option == ISAVEHISTRY)
					{
						DEFAULTSAVEHISTORY =
						  (ISATTY & ITTYIN) ?  logValue : 0;
					}
#endif /*SAVEHISTORY*/
					else if(logValue)
					{			/* set defaults, do nothing if F */
						setDefaultOptions();
					}
				}
			
				break;
			
			  case IFORMAT:
			  case IWFORMAT:
				if (!isCharOrString(arg))
				{
					sprintf(OUTSTR, mustbeChar, keyword);
				}
				else if(!(reply = setFormat(STRINGPTR(arg),fmt)))
				{
					sprintf(OUTSTR,"WARNING: '%s' is not proper value for %s",
							STRINGPTR(arg), keyword);
				}
				else
				{
					for(k = 0;k<3;k++)
					{
						if(option == IFORMAT)
						{
							PRINTFORMAT[k] = fmt[k];
						}
						else
						{
							WRITEFORMAT[k] = fmt[k];
						}
					}
					if(option == IFORMAT)
					{
						installFormat(PRINTFORMAT[0],PRINTFORMAT[1],
									  PRINTFORMAT[2]);
					}
					if (reply < 0)
					{
						reply = -reply;
						if (reply % 10)
						{
							sprintf(OUTSTR,
									"WARNING: %s width > %ld reduced to %ld",
									keyword, (long) MAXFMTWIDTH,
									(long) MAXFMTWIDTH);
							putErrorOUTSTR();
						} /*if (reply % 10)*/
						if (reply / 10)
						{
							sprintf(OUTSTR,
									"WARNING: %s %s > %ld reduced to %ld",
									keyword,
									(fmt[2] == 'g') ?
									"significant digits" : "decimals",
									(long) MAXFMTDIGITS, (long) MAXFMTDIGITS);
							putErrorOUTSTR();
						} /*if (reply / 10)*/
					} /*if (reply < 0)*/
				}
				break;

			  case IANGLES:
				if (!isCharOrString(arg))
				{
					sprintf(OUTSTR, mustbeChar, keyword);
				}
				else if(strncmp(STRINGPTR(arg),"radian",6) == 0)
				{
					ANGLES2RADIANS = 1.0;
				}
				else if(strncmp(STRINGPTR(arg),"cycle",5) == 0)
				{
					ANGLES2RADIANS = 8.0*MV_PI_4;
				}
				else if(strncmp(STRINGPTR(arg),"degree",6) == 0)
				{
					ANGLES2RADIANS = MV_PI_4/45.0;
				}
				else
				{
					sprintf(OUTSTR, mustbe, keyword,
							"\"radians\", \"cycles\", or \"degrees\"");
				}
				break;

			  case IFINDMACROS:
				if (!isCharOrString(arg))
				{
					sprintf(OUTSTR, mustbeChar, keyword);
				}
				else if(strncmp(STRINGPTR(arg),"n",1) == 0)
				{
					MACROSEARCH = noMacroSearch;
				}
				else if(strncmp(STRINGPTR(arg),"y",1) == 0)
				{
					MACROSEARCH = verboseMacroSearch;
				}
				else if(strncmp(STRINGPTR(arg),"silent",6) == 0)
				{
					MACROSEARCH = quietMacroSearch;
				}
				else
				{
					sprintf(OUTSTR, mustbe, keyword,
							"\"yes\", \"no\", or \"silent\"");
				}
				break;

#ifdef CANSETFONTS
			  case IFONT:
				if (!isCharOrString(arg))
				{
					sprintf(OUTSTR, mustbeChar, keyword);
				} /*if (!isCharOrString(arg))*/
				else
				{
					reply = decodeFont(STRINGPTR(arg), &newFontName, &fontSize);
					if (reply == BADFONTSIZE)
					{
						sprintf(OUTSTR,
								"WARNING: %s specifies a bad font size",
								STRINGPTR(arg));
					}
					else if (reply == FONTNAMETOOLONG)
					{
						sprintf(OUTSTR,
								"WARNING: %s specifies too long a font name",
								STRINGPTR(arg));
					}
					else if (reply == BADFONTSPEC)
					{
						sprintf(OUTSTR,
								"WARNING: '%s' not a valid font specification",
								STRINGPTR(arg));
					}
					else
					{
						if (fontSize > 0)
						{
							newFontSize = fontSize;
						}
						if (!changeFont)
						{
							changeFont = !isCurrentFont(newFontName, newFontSize);
						}
					}					
				} /*if (!isCharOrString(arg)){}else{}*/
				break;
#endif /*CANSETFONTS*/
			
			  case IPROMPT:
				if (!isCharOrString(arg))
				{
					sprintf(OUTSTR, mustbeChar, keyword);
				}
				else if(strlen(STRINGPTR(arg)) > MAXPROMPT - 1)
				{
					sprintf(OUTSTR, toolongChar,
							keyword, (long) MAXPROMPT - 1);
				}
				else
				{
					strcpy(PROMPT, STRINGPTR(arg));
				}
				break;

			  case IMISSING:
				if (!isCharOrString(arg))
				{
					sprintf(OUTSTR, mustbeChar, keyword);
				}
				else if(strlen(STRINGPTR(arg)) > LENGTHMISSING)
				{
					sprintf(OUTSTR, toolongChar,
							keyword, (long) LENGTHMISSING);
				}
				else
				{
					strcpy(NAMEFORMISSING,STRINGPTR(arg));
				}
				break;

			  case ILABELSTYLE:
				if (!isCharOrString(arg))
				{
					sprintf(OUTSTR, mustbeChar, keyword);
				}
				{
					char      c = STRINGVALUE(arg, 0);
					char     *pc = (c) ? strchr(Lparens+1, c) : (char *) 0;
					
					if (strlen(STRINGPTR(arg)) != 1 || pc == (char *) 0)
					{
						sprintf(OUTSTR, mustbe, keyword,
						"one of \"(\", \"[\", \"{\", \"<\", \"/\", or \"\\\"");
					}
					else
					{
						LEFTBRACKET[0] = c;
						RIGHTBRACKET[0] = Rparens[pc - Lparens];
					}
				}
				break;
				
				/*
				   integer valued options
				   */
			  case ILINES:
			  case IHEIGHT:
			  case IWIDTH:
			  case IERRORS:
			  case IFONTSIZE:
			  case IMAXWHILE:
#ifdef wx_motif
			  case IPLOTDELAY:
#endif /*wx_motif*/
#ifdef SAVEHISTORY
			  case IHISTORY:
#endif /*SAVEHISTORY*/
			  case IGUBED:
#ifdef MACINTOSH
			  case IPEELS:
#endif /*MACINTOSH*/
				value[0] = DATAVALUE(arg,0);
				if(TYPE(arg) != REAL || length != 1 || value[0] != floor(value[0]))
				{
					sprintf(OUTSTR, mustbe, keyword, "single integer");
				}
				else if(option == ILINES || option == IHEIGHT)
				{
					if(value[0] != 0.0 && value[0] < MINSCREENHEIGHT)
					{
						sprintf(OUTSTR,
								"WARNING: value for %s must be 0 or an integer >= %ld",
								keyword, (long) MINSCREENHEIGHT);
					}
					else
					{
						newLines = (long) value[0];
						if (option == ILINES)
						{
							symhLines = arg;
						}
						else
						{
							symhHeight = arg;
						}
					}
				}
				else if(option == IWIDTH)
				{
					if(value[0] < MINSCREENWIDTH)
					{
						sprintf(OUTSTR, mustbe2,
								keyword, (long) MINSCREENWIDTH);
					}
					newWidth = (long) value[0];
				}
				else if(option == IERRORS)
				{
					if(value[0] < 0)
					{
						sprintf(OUTSTR, mustbe2, keyword, 0L);
					}
					else
					{
						if(BDEPTH == 0)
						{
							MAXERRORS1 = MAXERRORS = (long) value[0];
						}
						else
						{
							MAXERRORS1 = (long) value[0];
						}
					}
				}
#ifdef CANSETFONTS
				else if (option == IFONTSIZE)
				{
					newFontSize = (long) value[0];
					if (newFontSize < MINFONTSIZE || newFontSize > MAXFONTSIZE)
					{
						sprintf(OUTSTR,
								"WARNING: illegal font size %ld", newFontSize);
					}
					else if (!changeFont)
					{
						changeFont = !isCurrentFont((char *) 0, newFontSize);
					}
				}
#endif /*CANSETFONTS*/
				else if (option == IMAXWHILE)
				{
					long      newMaxwhile = (long) value[0];

					if (newMaxwhile < MINMAXWHILE)
					{
						sprintf(OUTSTR, mustbe2, keyword, MINMAXWHILE);
					}
					else
					{
						MAXWHILE = newMaxwhile + 1;
					}
				}
#ifdef wx_motif
				else if (option == IPLOTDELAY)
				{
					long      newPlotdelay = (long) value[0];

					if (newPlotdelay < 0)
					{
						sprintf(OUTSTR, mustbe2, keyword, 0L);
					}
					else
					{
						FocusDelay = newPlotdelay;
					}
				}
#endif /*wx_motif*/
#ifdef SAVEHISTORY
				else if (option == IHISTORY)
				{
					long      newHistory = (long) value[0];

					if (newHistory < 0)
					{
						sprintf(OUTSTR, mustbe2, keyword, 0L);
					}
					else
					{
						initHistory(newHistory);
					}
				}
#endif /*SAVEHISTORY*/
				else if(option == IGUBED)
				{
					GUBED = (long) value[0];
				}

#ifdef MACINTOSH
				else if(option == IPEELS && value[0] >= 0)
				{
					long         isleep;
					/*
						CmdSleep +
						    100 * BackgroundSleep +
						  10000 * FgInterruptInterval +
						1000000 * InterruptInterval
						Sleep value of 99 means do not change
						Example:
						  setoptions(PEELS:99300299)
					 	    sets  BackgroundSleep to 2
							sets  FgInterruptInterval to 30
							does not change CmdSleep
							does not change BgInterruptInterval
					*/
#if (0)
					PRINT("CmdSleep = %d,BackgroundSleep = %d,FgInterruptInterval = %d,BgInterruptInterval = %d\n",
						  CmdSleep,BackgroundSleep,FgInterruptInterval,
						  BgInterruptInterval);
#endif /*(0)*/
					isleep = (long) value[0];
					if (isleep % 100 != 99)
					{
						CmdSleep = isleep % 100;
					}
					isleep /= 100;
					if (isleep % 100 != 99)
					{
						BackgroundSleep = isleep % 100;
					}
					isleep /= 100;
					if (isleep % 100 != 99)
					{
						FgInterruptInterval = isleep % 100;
					}
					isleep /= 100;
					if (isleep % 100 != 99)
					{
						BgInterruptInterval = isleep % 100;
					}
					SkelSetWaitTimes(CmdSleep, BackgroundSleep);
#if (0)
					PRINT("CmdSleep = %d,BackgroundSleep = %d,FgInterruptInterval = %d,BgInterruptInterval = %d\n",
						  CmdSleep,BackgroundSleep,FgInterruptInterval,
						  BgInterruptInterval);
#endif /*(0)*/
				}
#endif /*MACINTOSH*/
				break;

			  default:
				/* should not happen */
				sprintf(OUTSTR,"WARNING: unrecognized option '%s'",keyword);
			
			} /*switch (option)*/

			if(*OUTSTR)
			{
				if(!quiet)
				{
					sprintf(OUTSTR + strlen(OUTSTR),"; %s not set",keyword);
					putErrorOUTSTR();
				}
				else
				{
					*OUTSTR = '\0';
				}
			} /*if(*OUTSTR)*/
		} /*if (option < -NOPTIONS || option > 0)*/
	} /*for (icomp=0;icomp<ncomps;icomp++)*/

#ifdef CANSETFONTS
	if (changeFont)
	{
		setNewFont(newFontName, newFontSize);
	}
#endif /*CANSETFONTS*/
	/*
		set SCREENWIDTH and SCREENHEIGHT after setNewFont()
		since setNewFont() can change them
	*/
	if (symhLines != (Symbolhandle) 0 && symhHeight != (Symbolhandle) 0)
	{
		newLines = (long) DATAVALUE(symhHeight, 0);
	}
	installScreenDims((newWidth > 0) ? newWidth : SCREENWIDTH,
					  (newLines >= 0) ? newLines : SCREENHEIGHT);
	saveFormat(); /* save copy of formats and screen dimensions*/

#ifdef ACTIVEUPDATE
	if (newLines >= 0)
	{
		activeUpdateInit();
	}
#endif /*ACTIVEUPDATE*/
}  /*setOpt()*/

/*
  Check keyword name to see if it is valid option name
*/

long isOption(char *keyword)
{
	optionInfo    *option;
	
	for(option = Options;option->length;option++)
	{
		if(strncmp(option->name, keyword, (int) option->length) == 0)
		{
			return (option->code);
		}
	} /*for(option = Options;option->length;option++)*/
	return (0);
} /*isOption()*/

#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment Macmain
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

/*
   Implements function elapsedtime()
   Usage:
     gettime()                   prints time in seconds since start
	 gettime(quiet:F)            ditto
     gettime(quiet:T)            saves time since start for future reference
     gettime(keep:T)             returns time since start but prints nothing
	 gettime(keep:T,quiet:F)     returns and prints time since start
	 gettime(interval:T)         prints time in seconds since previous call
     gettime(interval:T,quiet:F) ditto
     gettime(interval:T,keep:T)  returns time since previous call, no print
     gettime(interval:T,keep:T,quiet:F) returns and prints time since start

   960306 Installed
   960307 Modified to add keyword interval
*/
#define TimeSinceLast   times[0]
#define TimeSinceFirst  times[1]

Symbolhandle elapsedTime(Symbolhandle list)
{
	Symbolhandle       result = (Symbolhandle) 0, symhKey;
	long               nargs = NARGS(list);
	char              *keyword;
	long               reportit = -1, keepit = 0, interval = 0, i;
	double             times[2], timeWanted;
	
	getElapsedTime(times);
	if (nargs > 3)
	{
		badNargs(FUNCNAME, -3);
		goto errorExit;
	}
	
	if (nargs > 1 || COMPVALUE(list, 0) != (Symbolhandle) 0)
	{
		for (i = 0; i < nargs; i++)
		{
			symhKey = COMPVALUE(list, i);
			if (!argOK(symhKey, 0, (nargs > 1) ? i+1 : 0))
			{
				goto errorExit;
			}
			if (!(keyword = isKeyword(symhKey)))
			{
				sprintf(OUTSTR,
						"ERROR: argument %ld to %s() not keyword phrase",
						i+1, FUNCNAME);
				goto errorExit;
			}
			if (strcmp(keyword, "quiet") != 0 && strcmp(keyword, "keep") != 0
				&& strncmp(keyword, "int", 3) != 0)
			{				
				badKeyword(FUNCNAME, keyword);
				goto errorExit;
			}
			if (!isTorF(symhKey))
			{
				notTorF(keyword);
				goto errorExit;
			}
			if (keyword[0] == 'q')
			{
				reportit = (DATAVALUE(symhKey, 0) == 0.0);
			}
			else if (keyword[0] == 'i')
			{
				interval = (DATAVALUE(symhKey, 0) != 0.0);
			}
			else
			{
				keepit = (DATAVALUE(symhKey, 0) != 0.0);
			}
		} /*for (i = 0; i < nargs; i++)*/
		if (keepit && reportit < 0)
		{
			reportit = 0;
		}
	} /*if (nargs > 1 || COMPVALUE(list, 0) != (Symbolhandle) 0)*/

	timeWanted = (interval) ? TimeSinceLast : TimeSinceFirst;
	if (reportit)
	{
		char      *buffer = OUTSTR;
		
		strcpy(buffer,
			   (interval) ? "Elapsed time is " : "Time since start is ");
		buffer += strlen(buffer);
		buffer += formatDouble(buffer, timeWanted , TRIMLEFT | TRIMRIGHT);
		strcpy(buffer," seconds");
		putOUTSTR();
	} /*if (reportit)*/

	if (keepit)
	{
		result = RInstall(SCRATCH, 1);
		if (result == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		DATAVALUE(result, 0) = timeWanted;
	} /*if (keepit)*/
	else
	{
		result = NULLSYMBOL;
	}
	
	return (result);

  errorExit:
	putErrorOUTSTR();
	return (0);

} /*elapsedTime()*/
