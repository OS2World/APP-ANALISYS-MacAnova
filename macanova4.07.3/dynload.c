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
#pragma segment Dynload
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

#ifdef WXWINMSW
#include <windows.h>
#define PREPENDUSCORE
#endif /*WXWINMSW*/

#include "globals.h"
#ifdef MACINTOSH
#include "version.h"  /* just need NEEDS68881 */
#endif /*MACINTOSH*/
#include "dynload.h"

#ifdef WXWIN
#include "wx/wxProto.h"
#endif /*WXWIN*/

#ifdef MACINTOSH
#include "macIface.h"
#endif /*MACINTOSH*/

/*
  970611 New file to implement dynamic loading

  If a version has dynamic loading, HASDYNLOAD should be defined in
  platform.h or in the relevant *.h file included by platfrom.h

  981029 Modified preprocessor commands to recognize that SUN does things
         the same way as IRIX
  990215 Changed myerrorout() to putOutMsg()
  990226 replaced most uses of putOUTSTR() by putErrorOUTSTR()
*/

#ifndef HASDYNLOAD

/* stubs for callC() and dynload() */

Symbolhandle callC(Symbolhandle list)
{
	notImplemented(FUNCNAME);
	return (0);
} /*callC()*/

Symbolhandle dynload(Symbolhandle list)
{
	notImplemented(FUNCNAME);
	return (0);
} /*dynload()*/

#else /*HASDYNLOAD*/

/*
	defines for standard callback functions
*/
#define mvPrint   putOutMsg

#if defined(MACINTOSH) || defined(WXWIN)
#define mvAlert   myAlert
#else /*MACINTOSH || WXWIN*/
#define mvAlert   mvPrint
#endif /*MACINTOSH || WXWIN*/

#define mvIsmissing isMISSING

#if defined(IRIX) || defined(SUN)  /* || defined(LINUX) should work, hasn't been tried */
#define LIKEIRIX
#endif /*IRIX || SUN*/
/*
  For shared libraries (Unix) and DLLs, Dl_sym_labels and
  Dl_sym_values contain library names and handles.  For
  DJGPP, they contain the (MacAnova) calling name and
  a function pointer.
*/

#ifndef MAXDYNLOADS
#define MAXDYNLOADS    20
#endif /*MAXDYNLOADS*/
#define FILENAMELIMIT 64

#ifdef HPUX
#include <errno.h>
extern int       errno;

#include <dl.h>
#define LIBTYPE     shl_t
#endif /* HPUX */

#if defined(LIKEIRIX)
#include <dlfcn.h>
#define LIBTYPE     void *
#endif /* LIKEIRIX*/

#ifdef DJGPP
#include <sys/dxe.h>
#define LIBTYPE     void *
#endif /*DJGPP*/

#ifdef WXWINMSW
#define LIBTYPE     HINSTANCE
#endif /*WXWINMSW*/

#ifdef MACINTOSH
#define LIBTYPE     Integer
#endif /* MACINTOSH */

static long         Dynloadcount = 0;
static char         Dl_sym_labels[MAXDYNLOADS][FILENAMELIMIT+1];
static LIBTYPE      Dl_sym_values[MAXDYNLOADS];

/*
	Function to do dynamic loading (linking) of file libName, setting
	Dl_sym_labels[slot] and Dl_sym_values[slot] appropriately
*/
#ifndef MW_CW_New
static void         doDynload(char * libName, long slot)
#else /*MW_CW_New*/
static void         doDynload(unsigned char * libName, long slot)
#endif /*MW_CW_New*/
{
	LIBTYPE         lib;
	char           *what, *msg = (char *) 0;
	WHERE("doDynload");
	
#ifdef HPUX
	what = " library";

	lib = shl_load(libName, BIND_IMMEDIATE | BIND_FIRST | BIND_VERBOSE, 0);
#if (0)
	if (lib == (LIBTYPE) 0)
	{
		switch (errno)
		{
		  case ENOEXEC:
			msg = "The specified file is not a shared library, or a format error was detected.";
			break;

		  case ENOSYM:
			msg = "Some symbol required by the shared library could not be found.";
			break;

		  case EINVAL:
			msg = "The specified handle or index is not valid or an attempt was made to load a library at an invalid address.";
			break;

		  case ENOMEM:
			msg = "There is insufficient room in the address space to load the library.";
			break;

		  case ENOENT:
			msg = "The specified library does not exist.";
			break;

		  case EACCES:
			msg = "Read or execute permission is denied for the specified library.";
		} /*switch (errno)*/
	} /*if (lib == (LIBTYPE) 0)*/
#endif /*0*/
#elif defined(LIKEIRIX)
	what = " library";

	lib = dlopen(libName, RTLD_NOW);

#elif defined(WXWINMSW)
	what = " library";
	
	lib = LoadLibrary(libName);
	lib = (lib > 32) ? lib : 0;

#elif defined(DJGPP)
	void          (*routineptr)(void) = (void (*)(void)) 0;

	what = NullString;

	routineptr = _dxe_load(libName);
	lib = (LIBTYPE) routineptr;

#elif defined(MACINTOSH)
	what = " resource file";
	
	CtoPstr((char  *) libName);
	lib = OpenResFile((const unsigned char *) libName);
	PtoCstr(libName);

	if (lib != (LIBTYPE) -1)
	{
		UseResFile(lib);
	}
	else
	{
		lib = 0;
	}

#endif /* HPUX */

	if (lib == (LIBTYPE) 0)
	{
		sprintf(OUTSTR,"ERROR: %s() cannot load%s %s", FUNCNAME,
				what, libName);
		putErrorOUTSTR();
		if (msg)
		{
			putOutMsg(msg);
		}
	} /*if (lib == (LIBTYPE) 0)*/
	else
	{
		strncpy(Dl_sym_labels[slot], (const char  *) libName, FILENAMELIMIT);
	}
	Dl_sym_values[slot] = lib;
} /*doDynload()*/

static void clearSlot(long slot)
{
	WHERE("clearSlot");
	
	if (EVALDEPTH == 0 && Dl_sym_values[slot] != (LIBTYPE) 0)
	{
#if defined(MACINTOSH)
		CloseResFile(Dl_sym_values[slot]);
		if (ResError() == resFNotFound)
		{
			sprintf(OUTSTR, "WARNING: Resource file %s not found",
					Dl_sym_values[slot]);
			putErrorOUTSTR();
		}
#elif defined(HPUX)
		shl_unload(Dl_sym_values[slot]);
#elif defined(LIKEIRIX)
		dlclose(Dl_sym_values[slot]);
#else
		; /* nothing special done */
#endif
	}

	Dl_sym_labels[slot][0] = '\0';
	Dl_sym_values[slot] = (LIBTYPE) 0;
} /*clearSlot()*/

static int useFile(long slot)
{
	WHERE("useFile");

	if (Dl_sym_values[slot] == (LIBTYPE) 0)
	{
		return (0);
	}
#ifdef MACINTOSH
	UseResFile(Dl_sym_values[slot]);
	if (ResError() != noErr)
	{
		return (0);
	}
#endif
	return (1);
} /*useFile()*/

/*
	This function implements MacAnova function loadUser().
	It does dynamic loading of externally compiled
	routines into MacAnova.  It is inherently machine dependent.
	
	usage: loadUser(filename [,reload:T] [,clear:T]), where only
	1 keyword phrase is allowed.
*/
Symbolhandle    dynload(Symbolhandle list)
{
	char           *fileName, *keyword;
	long            minNargs, maxNargs, slot;
	long            nargs = NARGS(list);
	long            doReload = 0, doClear = 0;
	Symbolhandle    arg1, symhKey;
#ifdef MACINTOSH
	OSType          types[1];
#endif
#ifdef DJGPP
	char           *routinename;
	void          (*routineptr)(void) = (void (*)(void)) 0;
#endif /*DJGPP */
	char           *reloadName = "reload", *clearName = "clear";
	WHERE("dynload");

	minNargs = 1;
	maxNargs = 2;

	if (nargs > maxNargs)
	{
		badNargs(FUNCNAME,-maxNargs);
		goto errorExit;
	}
	if (nargs < minNargs)
	{
		badNargs(FUNCNAME,-minNargs-1000);
		goto errorExit;
	}

	if (nargs > minNargs)
	{
		symhKey = COMPVALUE(list,maxNargs-1);
		if (!argOK(symhKey, 0, maxNargs))
		{
			goto errorExit;
		}
			
		if (!(keyword = isKeyword(symhKey)))
		{
			sprintf(OUTSTR,
					"ERROR: argument %ld to %s() must be keyword phrase",
					maxNargs, FUNCNAME);
			goto errorExit;
		}

		if (strcmp(keyword, reloadName) != 0 &&
			strcmp(keyword, clearName) != 0)
		{
			badKeyword(FUNCNAME, keyword);
			goto errorExit;
		}
		
		if (!isTorF(symhKey))
		{
			notTorF(keyword);
			goto errorExit;
		}
		
		if (keyword[0] == reloadName[0])
		{
			doReload = (DATAVALUE(symhKey,0) != 0.0);
		}
		else
		{
			doClear = (DATAVALUE(symhKey,0) != 0.0);
		}
	} /*if (nargs == maxNargs)*/			
	
	arg1 = COMPVALUE(list,0);
	if (!argOK(arg1, 0, 1))
	{
		goto errorExit;
	}

	if (!isCharOrString(arg1))
	{
		char        msg[50];
		
		sprintf(msg, "argument 1 to %s()", FUNCNAME);
		notCharOrString(msg);
		goto errorExit;
	} /*if (!isCharOrString(arg1))*/
	
	fileName = STRINGPTR(arg1);

#ifdef HASFINDFILE
	{
		char         msg[50];
		
		sprintf(msg, "Specify the %s file", FUNCNAME);
		
#ifdef MACINTOSH
		CtoPstr(msg);
		types[0] = 'rsrc';
		fileName = macFindFile(fileName, (unsigned char *) msg, (STR255) 0 , READIT, 1, types,
							&DynloadVolume);
#endif /*MACINTOSH*/

#ifdef WXWIN
		fileName = wxFindFile(fileName, msg, (char *) 0);
#endif /*WXWIN*/

		if (fileName == (char *) 0)
		{ /* cancelled */
			sprintf(OUTSTR, "WARNING: %s() request cancelled", FUNCNAME);
			goto errorExit;
		}
	}
#endif /*HASFINDFILE*/

	fileName = expandFilename(fileName);
	/*
	   At this point fileName is a pointer to a static buffer, not a
	   dereferenced handle
	*/
	if (fileName == (char *) 0 || !isfilename(fileName))
	{
		goto errorExit;
	}

	if (doClear)
	{
		for (slot = 0; slot < Dynloadcount; slot++)
		{
			clearSlot(slot);
		} /*for (slot = 0; slot < Dynloadcount; slot++)*/
		slot = Dynloadcount = 0;
	} /*if (doClear)*/
	else
	{
		for (slot = 0; slot < Dynloadcount; slot++)
		{
			if (strcmp(fileName, Dl_sym_labels[slot]) == 0)
			{
				break;
			}
		} /*for (slot = 0; slot < Dynloadcount; slot++)*/
	
		if (slot >= MAXDYNLOADS)
		{
			sprintf(OUTSTR,"ERROR: limit of %ld %s() files exceeded",
					(long) MAXDYNLOADS, FUNCNAME);
			goto errorExit;
		} /*if (slot >= MAXDYNLOADS)*/
	
		if (slot < Dynloadcount)
		{ /* file previously loaded */
			if (doReload)
			{
				clearSlot(slot);
			}
			else if (!useFile(slot))
			{
				sprintf(OUTSTR,
						"ERROR: previously loaded file %s is no longer available",
						Dl_sym_labels[slot]);
				clearSlot(slot);
			}
		} /*if (slot < Dynloadcount)*/
	} /*if (doClear){}else{}*/

	if (slot == Dynloadcount || doReload)
	{
#ifndef MW_CW_New
		doDynload(fileName, slot);
#else /*MW_CW_New*/
		doDynload((unsigned char *) fileName, slot);
#endif /*MW_CW_New*/
		if (Dl_sym_values[slot] == (LIBTYPE) 0)
		{
			goto errorExit;
		}
	
		if (slot >= Dynloadcount)
		{
			Dynloadcount++;
		}
	} /*if (slot == Dynloadcount || doReload)*/
	
	return (NULLSYMBOL);

  errorExit:
	putErrorOUTSTR();
	return (0);
	
} /*dynload()*/

/*
  isMISSING(&x) is almost equivalent to macro isMissing(x),
  returning 1 if x == MISSING and 0 otherwise;  added to be used
  by calling back from a dynamically loaded user function
  
  Note that the argument is a pointer
*/

static long isMISSING(double * x)
{
	return (isMissing(*x));
} /*isMISSING()*/

typedef struct mvFunction
{
	char      *name;
	void      *fun;
} mvFunction, *mvFunctionPtr;

static mvFunction  Functions[] =
{
	{"mygethandle",   (void *) mygethandle},
	{"mydisphandle",  (void *) mydisphandle},
	{"mygrowhandle",  (void *) mygrowhandle},
	{"sprintf",       (void *) sprintf},
	{"sscanf",        (void *) sscanf},
	{(char *) 0, (void *) 0}
};

static void *findFunctions(char * funname)
{
	mvFunctionPtr     mvFuns;
	
	for (mvFuns = Functions; mvFuns->name != (char *) 0;mvFuns++)
	{
		if (strcmp(funname, mvFuns->name) == 0)
		{
			return (mvFuns->fun);
		}
	}

/*
  If defined, FINDFUNCTIONS should be the name of a function
  (void * (*)(char *) ) for doing a further search
*/

#ifdef FINDFUNCTIONS
	return (FINDFUNCTIONS(funname));
#else /*FINDFUNCTIONS*/
	return ((void *) 0);
#endif /*FINDFUNCTIONS*/

} /*findFunctions()*/

static long       CallbackError;

static void       mvSeterror(long errorCode)
{
	CallbackError = errorCode;
} /*static mvSeterror()*/

static MacAnovaCBS        Callbacks =
{
	mvPrint,       /*print*/
	mvAlert,       /*alert*/
	mvEval,        /*eval*/
	mvIsmissing,   /*ismissing*/
	mvSeterror,    /*seterror*/
	findFunctions  /*findfun*/
}; /*Callbacks*/

static MacAnovaCBSPtr     CallbacksPtr = &Callbacks;
static MacAnovaCBSH       CallbacksH = &CallbacksPtr;

/* this is an ugly hack.  What's the right way? */

#if defined(MACINTOSH) && defined(powerc)
#define MAXCALLCARGS 13
#else
#define MAXCALLCARGS 20
#endif /*MACINTOSH && powerc */
			
#if defined(MACINTOSH) && defined(powerc)
static ProcInfoType UserProcInfoTypes[13] =
{
	uppMainEntryProcInfo01,
	uppMainEntryProcInfo02,
	uppMainEntryProcInfo03,
	uppMainEntryProcInfo04,
	uppMainEntryProcInfo05,
	uppMainEntryProcInfo06,
	uppMainEntryProcInfo07,
	uppMainEntryProcInfo08,
	uppMainEntryProcInfo09,
	uppMainEntryProcInfo10,
	uppMainEntryProcInfo11,
	uppMainEntryProcInfo12,
	uppMainEntryProcInfo13
}; /*UserProcInfoTypes[]*/
			
static ProcInfoType     ArgInfoProcInfo = uppArgInfoEntryProcInfo;
static UniversalProcPtr TmpfunUPP;
static UniversalProcPtr ArgfunUPP;

#endif /*MACINTOSH && powerc */

#if defined(MACINTOSH) && defined(powerc)
#define CALLUSER(procptr, nargs, args)\
	CallUniversalProc(TmpfunUPP, UserProcInfoTypes[nargs-1], args)
#define CALLARGINFO(procptr)\
	(long *) CallUniversalProc(ArgfunUPP, ArgInfoProcInfo)
#else
#define CALLUSER(procptr, nargs, args)\
	(*procptr)(args)
#define CALLARGINFO(procptr)\
	(long *) (*procptr)()
#endif /*MACINTOSH && powerc */

/* ARGSi expands to args[0], args[1], ..., args[i-1] */
#define ARGS1  args[0]
#define ARGS2  ARGS1,  args[1]
#define ARGS3  ARGS2,  args[2]
#define ARGS4  ARGS3,  args[3]
#define ARGS5  ARGS4,  args[4]
#define ARGS6  ARGS5,  args[5]
#define ARGS7  ARGS6,  args[6]
#define ARGS8  ARGS7,  args[7]
#define ARGS9  ARGS8,  args[8]
#define ARGS10 ARGS9,  args[9]
#define ARGS11 ARGS10, args[10]
#define ARGS12 ARGS11, args[11]
#define ARGS13 ARGS12, args[12]
#define ARGS14 ARGS13, args[13]
#define ARGS15 ARGS14, args[14]
#define ARGS16 ARGS15, args[15]
#define ARGS17 ARGS16, args[16]
#define ARGS18 ARGS17, args[17]
#define ARGS19 ARGS18, args[18]
#define ARGS20 ARGS19, args[19]

/* VOIDARGSi expands to void *, void *, ..., void * */
#define VOIDARGS1  void *
#define VOIDARGS2  VOIDARGS1,  void *
#define VOIDARGS3  VOIDARGS2,  void *
#define VOIDARGS4  VOIDARGS3,  void *
#define VOIDARGS5  VOIDARGS4,  void *
#define VOIDARGS6  VOIDARGS5,  void *
#define VOIDARGS7  VOIDARGS6,  void *
#define VOIDARGS8  VOIDARGS7,  void *
#define VOIDARGS9  VOIDARGS8,  void *
#define VOIDARGS10 VOIDARGS9,  void *
#define VOIDARGS11 VOIDARGS10, void *
#define VOIDARGS12 VOIDARGS11, void *
#define VOIDARGS13 VOIDARGS12, void *
#define VOIDARGS14 VOIDARGS13, void *
#define VOIDARGS15 VOIDARGS14, void *
#define VOIDARGS16 VOIDARGS15, void *
#define VOIDARGS17 VOIDARGS16, void *
#define VOIDARGS18 VOIDARGS17, void *
#define VOIDARGS19 VOIDARGS18, void *
#define VOIDARGS20 VOIDARGS19, void *

static void callUser(long argcount, void * tmpfun, void * args [])
{
	void  (*user01)(VOIDARGS1);
	void  (*user02)(VOIDARGS2);
	void  (*user03)(VOIDARGS3);
	void  (*user04)(VOIDARGS4);
	void  (*user05)(VOIDARGS5);
	void  (*user06)(VOIDARGS6);
	void  (*user07)(VOIDARGS7);
	void  (*user08)(VOIDARGS8);
	void  (*user09)(VOIDARGS9);
	void  (*user10)(VOIDARGS10);
	void  (*user11)(VOIDARGS11);
	void  (*user12)(VOIDARGS12);
	void  (*user13)(VOIDARGS13);
#if !defined(MACINTOSH) || !defined(powerc)
	void  (*user14)(VOIDARGS14);
	void  (*user15)(VOIDARGS15);
	void  (*user16)(VOIDARGS16);
	void  (*user17)(VOIDARGS17);
	void  (*user18)(VOIDARGS18);
	void  (*user19)(VOIDARGS19);
	void  (*user20)(VOIDARGS20);
#endif /*!MACINTOSH || !powerpc*/

	switch (argcount)
	{
	  case 1:
		user01 = (void (*)(VOIDARGS1)) tmpfun;
		CALLUSER(user01, 1, ARGS1);
		break;

	  case 2:
		user02 = (void (*)(VOIDARGS2)) tmpfun;
		CALLUSER(user02, 2, ARGS2);
		break;

	  case 3:
		user03 = (void (*)(VOIDARGS3)) tmpfun;
		CALLUSER(user03, 3, ARGS3);
		break;

	  case 4:
		user04 = (void (*)(VOIDARGS4)) tmpfun;
		CALLUSER(user04, 4, ARGS4);
		break;

	  case 5:
		user05 = (void (*)(VOIDARGS5)) tmpfun;
		CALLUSER(user05, 5, ARGS5);
		break;

	  case 6:
		user06 = (void (*)(VOIDARGS6)) tmpfun;
		CALLUSER(user06, 6, ARGS6);
		break;

	  case 7:
		user07 = (void (*)(VOIDARGS7)) tmpfun;
		CALLUSER(user07, 7, ARGS7);
		break;

	  case 8:
		user08 = (void (*)(VOIDARGS8)) tmpfun;
		CALLUSER(user08, 8, ARGS8);
		break;

	  case 9:
		user09 = (void (*)(VOIDARGS9)) tmpfun;
		CALLUSER(user09, 9, ARGS9);
		break;

	  case 10:
		user10 = (void (*)(VOIDARGS10)) tmpfun;
		CALLUSER(user10, 10, ARGS10);
		break;

	  case 11:
		user11 = (void (*)(VOIDARGS11)) tmpfun;
		CALLUSER(user11, 11, ARGS11);
		break;

	  case 12:
		user12 = (void (*)(VOIDARGS12)) tmpfun;
		CALLUSER(user12, 12, ARGS12);
		break;

	  case 13:
		user13 = (void (*)(VOIDARGS13)) tmpfun;
		CALLUSER(user13, 13, ARGS13);
		break;

#if !defined(MACINTOSH) || !defined(powerc)
	  case 14:
		user14 = (void (*)(VOIDARGS14)) tmpfun;
		CALLUSER(user14, 14, ARGS14);
		break;

	  case 15:
		user15 = (void (*)(VOIDARGS15)) tmpfun;
		CALLUSER(user15, 15, ARGS15);
		break;

	  case 16:
		user16 = (void (*)(VOIDARGS16)) tmpfun;
		CALLUSER(user16, 16, ARGS16);
		break;

	  case 17:
		user17 = (void (*)(VOIDARGS17)) tmpfun;
		CALLUSER(user17, 17, ARGS17);
		break;

	  case 18:
		user18 = (void (*)(VOIDARGS18)) tmpfun;
		CALLUSER(user18, 18, ARGS18);
		break;

	  case 19:
		user19 = (void (*)(VOIDARGS19)) tmpfun;
		CALLUSER(user19, 19, ARGS19);
		break;

	  case 20:
		user20 = (void (*)(VOIDARGS20)) tmpfun;
		CALLUSER(user20, 20, ARGS20);
		break;
#endif /*!MACINTOSH || !powerpc*/

	  default:
		break;
	} /*switch (nargs-1)*/
} /*callUser()*/

#ifndef DJGPP
static long * callArginfo(long * (*arginfo)(void) )
{
	return (CALLARGINFO(arginfo));
} /*callArginfo()*/
#else /*DJGPP*/

#define isSlash(C) ((C) == DIRSEPARATOR[0] || (C) == '/')
/*
  return 1 if funName "matches" path which is a path name in any one of
  three ways, all ignoring case and return 0 otherwise.
  (a) match to all of path
  (b) match to the file name part of path
  (c) match to the file name part of path, excluding trailing ".exe"
  if present in path
  Thus if path is "c:\\data\\foo.dxe" it is matched by any of
  "c:\\data\\foo.dxe", "foo.dxe" or "foo"
*/

static int djmatchfunName(char * funName, char *path)
{
	int        i, length;
	char      *tail;
	
	if (mystrncmp(funName, path, 0) == 0)
	{ /* exact match to entire file or path name*/
		return (1);
	}

	/* try to match to file name excluding directory/volume info*/
	tail = path + strlen(path);

	do
	{
		tail--;
	} while (tail >= path && !isSlash(*tail) && *tail != ':');
	
	if (tail < path)
	{
		tail = path;
	}
	else if (tail > path && mystrncmp(tail, funName, 0))
	{ /* match entire file name*/
		return (1);
	}

	/* try to match filename, except for trailing ".dxe", if present*/
	length = strlen(tail) - 4;
	if (strlen(funName) == length &&
		mystrncmp(tail + length, ".dxe", 0) == 0 &&
		mystrncmp(funName, tail, length) == 0)
	{
		return (1);
	}

	/* no match*/
	return (0);
} /*djmatchfunName()*/

#endif /*DJGPP*/

/*
	callC() executes an external user function
	
	970807 fixed bug resulting from failure to close the connection to
	a PPC code fragment.  Now CloseConnection() is called before return
	whenever a connection has been made.

	970808 Windows version tries adding leading '_' and searches again
	if user function is not found and similarly for arginfo function.
	
	970812 Made it an error for PPC MacAnova to call a 68K User function with
	Symbol arguments (symbol:T or arginfo function sets SYMBOLARGS flag)

	970813 Added component seterror to MacAnovaCBS structure to allow a
	user function making callbacks to set an error code.  It sets new
	static variable CallbackError.  Error codes noCallbackError and
	silentCallbackError are defined in dynload.h and should thus be accessible
	to a user function that includes Userfun.h

	980730 added new argument to reuseArg() so that notes will be kept.
*/
#ifdef MACINTOSH
enum isaModes
{
	resourceNotFound = 0,
	resourcePPC,
	resource68Kc,
	resource68Kn
};
#ifdef MW_CW_New
enum MyFragCodes
{
	fragNoErr = 0,  /* not defined in any header I can find */
	kNoConnectionID = 0/*obsolete name with no replacement */
};
#endif /*MW_CW_New*/
#endif /*MACINTOSH*/

Symbolhandle    callC(Symbolhandle list)
{
	Symbolhandle    arg1, result = (Symbolhandle) 0;
	Symbolhandle    symhTmp = (Symbolhandle) 0;
	char           *keyword;
	char            funName[64];
	char            resource[64];
	long            i, argcount;
	long            nargs = NARGS(list);
	long            nargs1 = nargs - 1;
	long         *(*arginfoFun)(void) = (long * (*)(void)) 0;
	long           *arginfo = (long *) 0;
	long            argType;
#if !defined(MACINTOSH) || defined(powerc)
	long            reply;
#endif /*!MACINTOSH || powerc*/
	void           *args[MAXCALLCARGS];
	char           *leadingUS;
	char           *usage =
		"ERROR: usage is '%s(funName,arg1,...[,callback:T])', CHARACTER funName";
	long            resultargs[MAXCALLCARGS], nresults = 0, ncomp=0;
	int             coerce = 0;
	int             quiet = 0;
	int             doCallback = -1, pointerArgs = -1, symbolArgs = -1;
	void           *tmpfun = (void *) 0;
	char           *callbackName = "callback", *pointersName = "pointers";
	char           *resourceName = "resource", *symbolsName = "symbols";
	char           *quietName = "quiet";
#ifdef HPUX
	void           *null = (void *) 0;
#endif /*HPUX*/

#ifndef DJGPP
	char            arginfoEntry[50];
#endif	/*DJGPP*/

#ifdef MACINTOSH
	void          **funhandle = (void **) 0;
	void          **funhandle68K = (void **) 0;
	long        *(**arginfoFunhandle)();
	int             isaMode;
	Str63           funNameMac;
	Str63           arginfoEntryMac;
	Str63           resourceMac;
#ifdef powerc
#ifndef MW_CW_New
	ConnectionID    connID = kNoConnectionID;
	SymClass        theSymClass;
#else /*MW_CW_New*/
	CFragConnectionID    connID = kNoConnectionID;
	CFragSymbolClass     theSymClass;
#endif /*MW_CW_New*/
	Str255          errName;
#endif /*powerc */
#endif /*MACINTOSH*/
	WHERE("callC");

	if (nargs > MAXCALLCARGS+3)
	{
		badNargs(FUNCNAME,-(MAXCALLCARGS+1));
		goto errorExit;
	}
	
	if (nargs < 2)
	{
		badNargs(FUNCNAME,-1002);
		goto errorExit;
	}
	
	arg1 = COMPVALUE(list,0);
	if (!argOK(arg1, 0, 1))
	{
		goto errorExit;
	}
	if (!isCharOrString(arg1))
	{
		sprintf(OUTSTR, usage, FUNCNAME);
		goto errorExit;
	}
	
	strncpy(funName, STRINGPTR(arg1), 63);
	funName[63] = '\0';
	/*
	  If funName starts with '_' it is assumed that the arginfo
	  function name should also start with '_'.  Care needs to be
	  taken to make sure an extra '_' doesn't get inserted in
	  the middle of the arginfo function name.
	*/

	leadingUS = (funName[0] == '_') ? "_" : NullString;
	
	resource[0] = '\0';
	for (i = 1;i < nargs; i++)
	{
		resultargs[i-1] = 0;
		args[i-1] = (void *) 0;
		arg1 = COMPVALUE(list,i);
		if ((keyword = isKeyword(arg1)))
		{
			if (strcmp(keyword, resourceName) == 0)
			{
				if (!isCharOrString(arg1))
				{
					notCharOrString(keyword);
					goto errorExit;
				}
#ifndef MACINTOSH
				if (!quiet)
				{
					sprintf(OUTSTR,
							"WARNING: keyword %s ignored by %s() in this version",
							keyword, FUNCNAME);
					putErrorOUTSTR();
				}
				
#else /*MACINTOSH*/
				strncpy(resource, STRINGPTR(arg1), 63);
				resource[63] = '\0';
#endif /*MACINTOSH*/
				resultargs[i-1] = -1;
				nargs1--;
				setNAME(arg1, USEDKEYWORD);
			}
			else if (strcmp(keyword, callbackName) == 0 ||
					 strncmp(keyword, pointersName,5) == 0 ||
					 strncmp(keyword, symbolsName,6) == 0 ||
					 strcmp(keyword, quietName) == 0)
			{
				int           logValue;

				if (keyword[0] == callbackName[0] && doCallback >= 0 ||
					keyword[0] == pointersName[0] && pointerArgs >= 0 ||
					keyword[0] == symbolsName[0] && symbolArgs >= 0)
				{
					sprintf(OUTSTR,
							"ERROR: keyword %s used more than once as an argument to %s()",
							keyword, FUNCNAME);
					goto errorExit;
				}
				
				if (!isTorF(arg1))
				{
					notTorF(keyword);
					goto errorExit;
				} /*if (!isTorF(arg1))*/
				
				logValue = (DATAVALUE(arg1, 0) != 0.0);
				if (keyword[0] == callbackName[0])
				{
					doCallback = logValue;
				}
				else if (keyword[0] == pointersName[0])
				{
					pointerArgs = logValue;
				}
				else if (keyword[0] == symbolsName[0])
				{
					symbolArgs = logValue;
				}
				else
				{
					quiet = logValue;
				}
				resultargs[i-1] = -1;
				nargs1--;
			}
			else if (strcmp(keyword,"protect") != 0)
			{/* keyword argument will be returned unless keyword is 'protect'*/
				resultargs[i-1] = 1;
				nresults++;
			}
		} /*if ((keyword = isKeyword(arg1)))*/
		else
		{ /* argument not returned*/
			resultargs[i-1] = 0;
		}
	} /*for (i = 1;i < nargs; i++)*/

#ifndef DJGPP
	sprintf(arginfoEntry, "%sarginfo_%s", leadingUS,
			(*leadingUS) ? funName + 1 : funName);
#endif /*DJGPP*/

/*
	Find user and arginfo function entries;
	different platforms use different methods
*/

#ifdef HPUX
	reply = shl_findsym((shl_t *) &null, funName, TYPE_PROCEDURE,
						(void *) &tmpfun);
	/*
	  PREPENDUSCORE is not defined for HPUX.  This code is here as
	  an example for porting User() to another version of Unix
	*/

#ifdef PREPENDUSCORE
	if (reply != 0 && funName[0] != '_')
	{
		char     funName1[64];

		funName1[0] = '_';
		strcpy(funName1 + 1, funName);
		reply = shl_findsym((shl_t *) &null, funName1, TYPE_PROCEDURE,
							(void *) &tmpfun);
	} /*if (reply != 0 && funName[0] != '_')*/
#endif /*PREPENDUSCORE*/

	if (reply != 0)
	{
		goto cantFind;
	} /*if (reply != 0)*/
	
	reply = shl_findsym((shl_t *) &null, arginfoEntry, TYPE_PROCEDURE,
				(void *) &arginfoFun);

#ifdef PREPENDUSCORE
	if (reply != 0 && arginfoEntry[0] != '_')
	{
		char     arginfoEntry1[64];

		arginfoEntry1[0] = '_';
		strcpy(arginfoEntry1 + 1, arginfoEntry);
		reply = shl_findsym((shl_t *) &null, arginfoEntry1, TYPE_PROCEDURE,
							(void *) &arginfoFun);
	} /*if (reply != 0)*/
#endif /*PREPENDUSCORE*/
	/* if not found, nothing happens and no typechecking */
	if (arginfoFun)
	{
		arginfo = callArginfo(arginfoFun);
	}

#elif defined(LIKEIRIX)
	for (i = Dynloadcount-1; i >= 0; i--)
	{
		if ((tmpfun = dlsym(Dl_sym_values[i], funName)) != 0)
		{
			break;
		}
	} /*for (i = Dynloadcount-1; i >= 0; i--)*/

#ifdef PREPENDUSCORE
	if (i < 0 && funName[0] != '_')
	{
		char     funName1[64];

		funName1[0] = '_';
		strcpy(funName1 + 1, funName);
		for (i = Dynloadcount-1; i >= 0; i--)
		{
			if ((tmpfun = dlsym(Dl_sym_values[i], funName1)) != 0)
			{
				break;
			}
		} /*for (i = Dynloadcount-1; i >= 0; i--)*/
	} /*if (i < 0 && funName[0] != '_')*/
#endif /*PREPENDUSCORE*/
	if (i < 0)
	{
		goto cantFind;
	} /*if (i < 0)*/

	arginfoFun = (long * (*)(void)) dlsym(Dl_sym_values[i], arginfoEntry);
#ifdef PREPENDUSCORE
	if (!arginfoFun && arginfoEntry[0] != '_')
	{
		char       arginfoEntry1[64];
		
		arginfoEntry1[0] = '_';
		strcpy(arginfoEntry1 + 1, arginfoEntry);
		arginfoFun = (long * (*)(void)) dlsym(Dl_sym_values[i], arginfoEntry1);
	} /*if (!arginfoFun && arginfoEntry[0] != '_')*/
#endif /*PREPENDUSCORE*/
	/* if not found, nothing happens and no typechecking */
	if (arginfoFun)
	{
		arginfo = callArginfo(arginfoFun);
	}

#elif defined(MACINTOSH)
	
	if (resource[0] == '\0')
	{
		strcpy(resource, funName);
	}
	strcpy((char *) resourceMac, resource);
	CtoPstr((char *) resourceMac);

	
	/*
		Always first try to get PPC resource
		
		If not found or version is not PPC, then try to find appropriate
		68K resource (type MV6c or MV6n matching MacAnova version);
		if not found, then try inappropriate 68K resource (type MV6c or MV6n
		not matching MacAnova version).
		
		isaMode is set appriately to the resource type found, if any
	*/
	funhandle = GetNamedResource(USERTYPEPPC, resourceMac);
	isaMode = (funhandle != (void **) 0) ? resourcePPC : resourceNotFound;
#ifdef powerc
	if (isaMode == resourceNotFound)
#endif /*powerc*/
	{
#ifdef NEEDS68881
		/* appropriate type is USERTYP68KC for co-processor*/
		funhandle68K = GetNamedResource(USERTYPE68KC, resourceMac);
		isaMode =  (funhandle68K != (void **) 0) ? resource68Kc : isaMode;

		if (isaMode != resource68Kc)
		{
			funhandle68K = GetNamedResource(USERTYPE68KN, resourceMac);
			isaMode = (funhandle68K != (void **) 0) ? resource68Kn : isaMode;
		} /*if (isaMode == 0)*/
#else /*NEEDS68881*/
		/* appropriate type is USERTYP68KN for no-co-processor*/
		funhandle68K = GetNamedResource(USERTYPE68KN, resourceMac);
		isaMode =  (funhandle68K != (void **) 0) ? resource68Kn : isaMode;

		if (isaMode != resource68Kn)
		{
			funhandle68K = GetNamedResource(USERTYPE68KC, resourceMac);
			isaMode = (funhandle68K != (void **) 0) ? resource68Kc : isaMode;
		} /*if (isaMode == 0)*/
#endif /*NEEDS68881*/
	}

	if (isaMode == resourceNotFound)
	{
		goto cantFind;
	} /*if (isaMode == 0)*/
	
	if (isaMode == resource68Kc && !Has68881)
	{
		sprintf(OUTSTR,
				"ERROR: User function %s requires unavailable math co-processor",
				funName);
		goto errorExit;
	}

#ifndef powerc
	if (isaMode == resourcePPC)
	{
		sprintf(OUTSTR,
				"ERROR: 68K version of MacAnova cannot call PPC user function %s",
				funName);
		goto errorExit;
	} /*if (isaMode == resourcePPC)*/
#endif /*powerc*/

	if (isaMode != resourcePPC)
	{
		/* resource name must match function name*/
		if (strcmp(funName,resource) != 0)
		{
			sprintf(OUTSTR,
					"ERROR: resource name %s != function name %s for 68K user function",
					resource, funName);
			goto errorExit;
		}
		funhandle = funhandle68K;
	} /*if (isaMode != resourcePPC)*/

	HLock(funhandle);

	strcpy((char *) funNameMac, funName);
	CtoPstr((char *) funNameMac);

	strcpy((char *) arginfoEntryMac, arginfoEntry);
	CtoPstr((char *) arginfoEntryMac);

#ifdef powerc
	if (isaMode == resourcePPC)
	{
#ifndef MW_CW_New
		reply = GetMemFragment(*funhandle, 0L, funNameMac, kLoadLib,
							   &connID, &tmpfun, errName);
#else /*MW_CW_New*/
		reply = GetMemFragment(*funhandle, 0L, funNameMac, kLoadCFrag,
							   &connID, &tmpfun, errName);
#endif /*MW_CW_New*/

		if (reply == fragNoErr)
		{
			/* this is a PPC code resource */
			reply = FindSymbol(connID, funNameMac,
							   (Ptr *) &TmpfunUPP, &theSymClass);
	
			if (reply != fragNoErr)
			{
#ifndef MW_CW_New
				if (reply == fragSymbolNotFound)
#else /*MW_CW_New*/
				if (reply == cfragNoSymbolErr)
#endif /*MW_CW_New*/
				{
					sprintf(OUTSTR, "Code for user function %s not found",
							funName);
				}
				goto cantFind;
			}
	
			reply = FindSymbol(connID, arginfoEntryMac, (Ptr *) &ArgfunUPP,
							   &theSymClass);
			if (reply == fragNoErr)
			{
				arginfo = callArginfo(arginfoFun);
			}
		} /*if (reply == fragNoErr)*/
		else
		{
			char       *outstr = OUTSTR;
			
			sprintf(outstr, "User function \"s\" ", funName);
			outstr += strlen(outstr);
#ifndef MW_CW_New
			if (reply == fragLibNotFound)
#else /*MW_CW_New*/
			if (reply == cfragNoLibraryErr)
#endif /*MW_CW_New*/
			{
				sprintf(outstr,
						"not found in resource \"%s\"", resource);
			}
#ifndef MW_CW_New
			else if (reply == fragHadUnresolveds)
#else /*MW_CW_New*/
			else if (reply == kUnresolvedCFragSymbolAddress)
#endif /*MW_CW_New*/
			{
				sprintf(outstr, "has unresolved symbols");
			}
#ifndef MW_CW_New
			else if (reply == fragNoMem || reply == fragNoAddrSpace)
#else /*MW_CW_New*/
			else if (reply == cfragNoPrivateMemErr || reply == cfragNoClientMemErr)
#endif /*MW_CW_New*/
			{
				sprintf(outstr,
						"could not be run because of insufficient memory");
			}
			else
			{
				*OUTSTR = '\0';
			}
			goto cantFind;
		} /*if (reply == fragNoErr){}else{}*/
	} /*if (isaMode == resourcePPC)*/
	else
	{
		/* this must be a 68K code resource */
		TmpfunUPP = (UniversalProcPtr) *funhandle;
	} /*if (isaMode == resourcePPC){}else{}*/
#endif /*powerc*/

	tmpfun = *funhandle;

	if (isaMode != resourcePPC)
	{
		ResType      resourceType = 
			(isaMode == resource68Kc) ? USERTYPE68KC : USERTYPE68KN;
		/* do this for 68K code */
		arginfoFunhandle = 
		   ( long * (**)()) GetNamedResource(resourceType, arginfoEntryMac);

		/* if not found, nothing happens and no typechecking */
		if (arginfoFunhandle)
		{
			HLock((char **) arginfoFunhandle);
#ifdef powerc
			ArgfunUPP = (UniversalProcPtr) *arginfoFunhandle;
#endif /*powerc*/
			arginfo = callArginfo(*arginfoFunhandle);
			HUnlock((char **) arginfoFunhandle);
		} /*if (arginfoFunhandle)*/
	} /*isaMode != resourcePPC)*/

#elif defined(DJGPP)
	for (i = Dynloadcount-1; i >= 0; i--)
	{
		if (djmatchfunName(funName, Dl_sym_labels[i]))
		{
			break;
		}
	} /*for (i = Dynloadcount-1; i >= 0; i--)*/
#ifdef PREPENDUSCORE
	if (i < 0 && funName[0] != '_')
	{
		char     funName1[64];

		funName1[0] = '_';
		strcpy(funName1 + 1, funName);
		for (i = Dynloadcount-1; i >= 0; i--)
		{
			if (djmatchfunName1(funName1, Dl_sym_labels[i]))
			{
				break;
			}
		} /*for (i = Dynloadcount-1; i >= 0; i--)*/
	} /*if (i < 0)*/
#endif /*PREPENDUSCORE*/
	if (i < 0)
	{
		goto cantFind;
	} /*if (i < 0)*/
	
	tmpfun = Dl_sym_values[i];
	/* no type checking in DJG since dxe only allows one item per load */

#elif defined(wx_msw)
	for (i = Dynloadcount-1; i >= 0; i--)
	{
		if ((tmpfun = GetProcAddress(Dl_sym_values[i], funName)) != 0)
		{
			break;
		}
	} /*for (i = Dynloadcount-1; i >= 0; i--)*/

#ifdef PREPENDUSCORE
	if (i < 0 && funName[0] != '_')
	{
		char     funName1[64];

		funName1[0] = '_';
		strcpy(funName1 + 1, funName);
		for (i = Dynloadcount-1; i >= 0; i--)
		{
			if ((tmpfun = GetProcAddress(Dl_sym_values[i], funName1)) != 0)
			{
				break;
			}
		} /*for (i = Dynloadcount-1; i >= 0; i--)*/
	} /*if (i < 0 && funName[0] != '_')*/
#endif /*PREPENDUSCORE*/
	if (i < 0)
	{
		goto cantFind;
	} /*if (i < 0)*/

	arginfoFun = (long * (*)(void)) GetProcAddress(Dl_sym_values[i],
			arginfoEntry);
#ifdef PREPENDUSCORE
	if (!arginfoFun && arginfoEntry[0] != '_')
	{
		char       arginfoEntry1[64];
		
		arginfoEntry1[0] = '_';
		strcpy(arginfoEntry1 + 1, arginfoEntry);
		arginfoFun = (long * (*)(void)) GetProcAddress(Dl_sym_values[i],
													   arginfoEntry1);
	} /*if (!arginfoFun && arginfoEntry[0] != '_')*/
#endif /*PREPENDUSCORE*/
	/* if not found, nothing happens and no typechecking */
	if (arginfoFun)
	{
		arginfo = callArginfo(arginfoFun);
	}
#endif
/*  End of code finding user and arginfo function entries */

	/*
	  form of long vector returned by callArginfo()
	  arginfo[0]    number of arguments expected
	  arginfo[1]    NOCALLBACK | USESPOINTERS, NOCALLBACK | USESHANDLES,
                      CALLBACK | USESPOINTERS, or CALLBACK | USESHANDLES
      arginfo[2]    type and shape information about argument 1
      arginfo[3]    type and shape information about argument 2
	  . . . . . . .
	  On Macintosh, arginfo[1] & COPROCESSORERROR != 0 indicates the user
	  function requires a 68881 coprocessor which isn't available.
	*/

	if (arginfo)
	{
		long    targetNargs = arginfo[0];
		int     targetCallback = (arginfo[1] & DOESCALLBACK) ? 1 : 0;
		int     targetPointers = (arginfo[1] & USESHANDLES) ? 0 : 1;
		int     targetSymbols = (arginfo[1] & SYMBOLARGS) ? 1 : 0;
		char   *msg =
			"ERROR: %s:%c is inconsistent with information from %s";
		
		if (nargs1 != targetNargs)
		{
			sprintf(OUTSTR,
					"ERROR: user function %s expects %ld argument%s", funName,
					targetNargs, (targetNargs > 1) ? "s" : NullString);
			goto errorExit;
		} /*if (nargs1 != targetNargs)*/

		if (doCallback < 0)
		{
			doCallback = targetCallback;
		}
		else if (doCallback != targetCallback)
		{
			sprintf(OUTSTR, msg, callbackName, (doCallback) ? 'T' : 'F',
					funName);
			goto errorExit;
		}
		
		if (pointerArgs < 0)
		{
			pointerArgs = targetPointers;
		}
		else if (pointerArgs != targetPointers)
		{
			sprintf(OUTSTR, msg, pointersName, (pointerArgs) ? 'T' : 'F',
					funName);
			goto errorExit;
		}

		if (symbolArgs < 0)
		{
			symbolArgs = targetSymbols;
		}
		else if (symbolArgs != targetSymbols)
		{
			sprintf(OUTSTR, msg, symbolsName, (symbolArgs) ? 'T' : 'F',
					funName);
			goto errorExit;
		}

#ifdef MACINTOSH
		if (arginfo[1] & COPROCESSORERROR)
		{
			sprintf(OUTSTR,
					"ERROR: user function %s requires unavailable 68881 math co-processor",
					funName);
			goto errorExit;
		}
#endif /*MACINTOSH*/
	} /*if (arginfo)*/

	if (pointerArgs < 0)
	{
#ifdef MACINTOSH
		pointerArgs = 0;
#else /*MACINTOSH*/
		pointerArgs = 1;
#endif /*MACINTOSH*/
	}

	symbolArgs = (symbolArgs >= 0) ? symbolArgs : 0;
	doCallback = (doCallback >= 0) ? doCallback : 0;

#ifdef MACINTOSH
	if (doCallback)
	{
#ifdef powerc
		if (isaMode != resourcePPC)
		{
			/* we won't do callbacks to PPC MacAnova from 68K code resource */
			sprintf(OUTSTR,
					"ERROR: cannot do callbacks to PPC MacAnova from 68K user function");
			goto errorExit;
		}
#endif

#ifndef NEEDS68881
		if (isaMode == resource68Kc && !quiet)
		{
			sprintf(OUTSTR,
					"WARNING: callbacks from user function needing coprocessor are problematic");
			putErrorOUTSTR();
		}
#else /*NEEDS68881*/
		if (isaMode == resource68Kn && !quiet)
		{
			sprintf(OUTSTR,
					"WARNING: callbacks from user function not needing coprocessor are problematic");
			putErrorOUTSTR();
		}
#endif /*NEEDS68881*/

		if (pointerArgs && !quiet)
		{
			sprintf(OUTSTR,
					"WARNING: dangerous use of pointer arguments by %s() with call back",
					FUNCNAME);
			putErrorOUTSTR();
		} /*if (pointerArgs)*/
	} /*if (doCallback)*/

#ifdef powerc
	/* 
		Because of structure alignment problems, Symbol arguments (symbol:T or
		SYMBOLARGS flag set by arginfo functio) cannot be used when PPC MacAnova
		is calling a 68K User function .
	*/
	if (symbolArgs && isaMode != resourcePPC)
	{
		sprintf(OUTSTR,
				"ERROR: PPC MacAnova %s() cannot call 68K user function with symbol arguments",
				FUNCNAME);
		goto errorExit;
	} /*if (symbolArgs && isaMode != resourcePPC)*/
#endif /*powerc*/			
#endif /*MACINTOSH*/

	if (nresults == 0)
	{
		result = NULLSYMBOL;
	}
	else if (nresults > 1 &&
		(result = StrucInstall(SCRATCH, nresults)) == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	argcount = 0;
	for (i = 1; i < nargs; i++)
	{
		if (resultargs[i-1] >= 0)
		{
			/* 
			   argument wasn't callback:T or F, pointers:T or F or
			   resource:name
			*/
			arg1 = COMPVALUE(list, i);
			argType = TYPE(arg1);

			args[argcount] = (void *) 0;
			if (arginfo)
			{ /* check type */
				char         msg[100];
				
				sprintf(msg, "argument %ld to user function %s", argcount + 1,
						funName);
				
				if (!checkArgType(arg1, msg, arginfo[argcount + 2]))
				{
					goto errorExit;
				}
			} /*if (arginfo)*/

			if (argType != REAL && argType != LOGIC && argType != CHAR &&
				argType != LONG && !symbolArgs)
			{
				sprintf(OUTSTR,
						"ERROR: argument %ld to user function %s not REAL, LOGIC, or CHARACTER",
						argcount+1,funName);
				goto errorExit;
			}
			if (resultargs[i-1] > 0)
			{
				/* argument is to be returned */
				Cutsymbol(arg1);
				coerce = (argType == LONG) ? 1 : coerce;
				if (nresults == 1)
				{
					Setname(arg1, SCRATCH);
					Addsymbol(arg1);
					result = reuseArg(list, i, 1, 1);
				}
				else
				{
					Setname(arg1, isKeyword(arg1));
					COMPVALUE(result, ncomp++) = reuseArg(list, i, 1, 1);
				}
			} /*if (resultargs[i-1] == 1)*/
		
			if (symbolArgs)
			{
				args[argcount] = (!pointerArgs) ?
					(void *) arg1 : (void *) *arg1;
			}
			else if (argType == CHAR)
			{
				args[argcount] = (!pointerArgs) ?
					(void *) STRING(arg1) : (void *) STRINGPTR(arg1);
			}
			else if (argType == REAL || argType == LOGIC)
			{
				args[argcount] = (!pointerArgs) ?
					(void *) DATA(arg1) : (void *) DATAPTR(arg1);
			}
			else
			{
				args[argcount] = (!pointerArgs) ?
					(void *) LONGDATA(arg1) : (void *) LONGDATAPTR(arg1);
			}
			argcount++;
		} /*if (resultargs[i-1] >= 0)*/
	} /*for (i = 1; i < nargs; i++)*/

	if (doCallback)
	{
		args[argcount++] = (!pointerArgs) ?
			(void *) CallbacksH : (void *) CallbacksPtr;
		CallbackError = noCallbackError;
	} /*if (doCallback)*/

	callUser(argcount, tmpfun, args);
	
#ifdef MACINTOSH
#ifdef powerc
#ifndef MW_CW_New
	if (connID != kNoConnectionID)
#else /*MW_CW_New*/
	if (connID != (CFragConnectionID) kNoConnectionID)
#endif /*MW_CW_New*/
	{
		CloseConnection(&connID);
		connID = kNoConnectionID;
	}
#endif /*powerc*/
	if (EVALDEPTH == 0)
	{
		HUnlock(funhandle);
	} /*if (EVALDEPTH == 0)*/	
#endif /*MACINTOSH*/

	if (doCallback && CallbackError != noCallbackError)
	{
		if (CallbackError != silentCallbackError)
		{
			sprintf(OUTSTR,
					"ERROR: user function %s error number %ld occurred",
					funName, CallbackError);
		} /*if (CallbackError != silentCallbackError)*/
		CallbackError = noCallbackError;
		goto errorExit;
	} /*if (doCallback && CallbackError != noCallbackError)*/
	
	if (coerce)
	{ /* at least one argument to coerce from LONG to REAL */
		if (nresults == 1)
		{ /* coerce single result to REAL*/
			symhTmp = RInstall(SCRATCH, 0);
			if (symhTmp == (Symbolhandle) 0 ||
				!CopyLongToDouble(result, symhTmp))
			{
				goto errorExit;
			}
			setNAME(symhTmp, SCRATCH);
			Removesymbol(result);
			result = symhTmp;
		} /*if (nresults == 1)*/
		else
		{ /* coerce all LONG components of result to REAL*/
			for (i = 0; i < nresults; i++)
			{
				arg1 = COMPVALUE(result, i);

				if (TYPE(arg1) == LONG)
				{
					symhTmp = RInstall(SCRATCH, 0);
					if (symhTmp == (Symbolhandle) 0 ||
						!CopyLongToDouble(arg1, symhTmp))
					{
						goto errorExit;
					}
					setNAME(symhTmp, SCRATCH);
					Cutsymbol(symhTmp);
					setNAME(symhTmp, NAME(arg1));
					COMPVALUE(result, i) = symhTmp;
					Delete(arg1);
				} /*if (TYPE(arg1) == LONG)*/
			} /*for (i = 0; i < nresults; i++)*/
		} /*if (nresults == 1){}else{}*/
	} /*if (coerce)*/

	return(result);

  cantFind:
	putOUTSTR();
	sprintf(OUTSTR,"ERROR: unable to find user function %s", funName);
	/* fall through*/

  errorExit:
	putErrorOUTSTR();
  	Removesymbol(result);
  	Removesymbol(symhTmp);
#ifdef MACINTOSH
#ifdef powerc
	if (connID != (CFragConnectionID) kNoConnectionID)
	{
		CloseConnection(&connID);
	}
#endif /*powerc*/
	if (funhandle != (void **) 0 && EVALDEPTH == 0)
	{
		HUnlock(funhandle);
	} /*if (funhandle != (void **) 0 && EVALDEPTH == 0)*/

#endif /*MACINTOSH*/
	return (0);
	
} /*callC()*/

#endif /*HASDYNLOAD*/

Symbolhandle asLong(Symbolhandle list)
{
	Symbolhandle          arg, result = (Symbolhandle) 0;
	double                value, *values;
	long                  nargs = NARGS(list);
	long                  i, n;
	WHERE("asLong");

	if (nargs != 1)
	{
		badNargs(FUNCNAME, 1);
		goto errorExit;
	}
	arg = COMPVALUE(list, 0);
	if (!argOK(arg, REAL, 0))
	{
		goto errorExit;
	}
	if (anyMissing(arg))
	{
		sprintf(OUTSTR,
				"ERROR: argument to %s has MISSING values", FUNCNAME);
		goto errorExit;
	}
	n = symbolSize(arg);
	values = DATAPTR(arg);
	for (i = 0; i < n; i++)
	{
		value = values[i];
		if (2*fabs(value) > MAXBITVALUE)
		{
			sprintf(OUTSTR,
					"ERROR: argument to %s() has element too big to convert",
					FUNCNAME);
		}
		else if (value != floor(value))
		{
			sprintf(OUTSTR,
					"ERROR: argument to %s() has non-integer element",
					FUNCNAME);
		}
		if (*OUTSTR)
		{
			goto errorExit;
		}
	} /*for (i = 0; i < n; i++)*/
	
	result = LongInstall(SCRATCH, 0);
	if (result == (Symbolhandle) 0 || !CopyDoubleToLong(arg, result))
	{
		Removesymbol(result);
		goto errorExit;
	}
	setNAME(result,SCRATCH);
	
	return (result);

  errorExit:
	putErrorOUTSTR();
	return (0);
} /*asLong()*/
