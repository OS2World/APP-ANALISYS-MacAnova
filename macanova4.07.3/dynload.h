/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 4.05 or later
*(C)*
*(C)* Copyright (c) 1997 by Gary Oehlert and Christopher Bingham
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
  970813 added new component seterror to structure MacAnovaCBS
*/
#ifndef DYNLOAD_H
#define DYNLOAD_H
typedef struct MacAnovaCBS
{
	void			(*print)(char *);
	void            (*alert)(char *);
	Symbolhandle	(*eval)(char **);
	long            (*ismissing)(double *);
	void            (*seterror)(long);
	void *			(*findfun)(char *);
} MacAnovaCBS, *MacAnovaCBSPtr, **MacAnovaCBSH;

/* values relevent to seterror */
enum    callBackErrorCodes
{
	noCallbackError = 0,
	silentCallbackError = -1
};

#ifndef POINTERARGS
#ifdef MACINTOSH
#define POINTERARGS 0
#else /*MACINTOSH*/
#define POINTERARGS 1
#endif /*MACINTOSH*/
#endif /*POINTERARGS*/

enum   argInfoCodes
{
	NOCALLBACK       = 0x00,
	DOESCALLBACK     = 0x01,
	USESPOINTERS     = 0x00,
	USESHANDLES      = 0x02,
#if POINTERARGS
	POINTERUSE       = USESPOINTERS,
#else /*POINTERARGS*/
	POINTERUSE       = USESHANDLES,
#endif /*POINTERARGS*/
	NOSYMBOLARGS     = 0x00,
	SYMBOLARGS       = 0x04,
	COPROCESSOROK    = 0x00,
	COPROCESSORERROR = 0x08
};


#ifdef MACINTOSH
/*
	Resource types for User Functions
*/
#define USERTYPEPPC   'MVPP'  /*resource requiring PPC*/
#define USERTYPE68KC  'MV6c'  /*resource requiring 68881 coprocessor*/
#define USERTYPE68KN  'MV6n'  /*resource not requiring 68881 coprocessor*/
#ifndef powerc
/* no special includes */
#else /*powerc*/

#ifndef MW_CW_New
#include <FragLoad.h>
#else /*MW_CW_New*/
#include <CodeFragments.h>
#endif /*MW_CW_New*/

#include <MixedMode.h>

#define LongArg(I) STACK_ROUTINE_PARAMETER(I, SIZE_CODE(sizeof(long)))

#define uppArgInfoEntry kCStackBased   | RESULT_SIZE(SIZE_CODE(sizeof(long)))
#define uppMainEntry01  kCStackBased   | LongArg(1)
#define uppMainEntry02  uppMainEntry01 | LongArg(2)
#define uppMainEntry03  uppMainEntry02 | LongArg(3) 
#define uppMainEntry04  uppMainEntry03 | LongArg(4) 
#define uppMainEntry05  uppMainEntry04 | LongArg(5) 
#define uppMainEntry06  uppMainEntry05 | LongArg(6) 
#define uppMainEntry07  uppMainEntry06 | LongArg(7) 
#define uppMainEntry08  uppMainEntry07 | LongArg(8) 
#define uppMainEntry09  uppMainEntry08 | LongArg(9) 
#define uppMainEntry10  uppMainEntry09 | LongArg(10)
#define uppMainEntry11  uppMainEntry10 | LongArg(11)
#define uppMainEntry12  uppMainEntry11 | LongArg(12)
#define uppMainEntry13  uppMainEntry12 | LongArg(13)

/*
	The following codes can be used to set up the long * foo_argtyping(void) entry
	(uppArgEntryProcInfo) and the void foo(arg01, ..., argNN) entries 
	(uppMainEntryProcInfoNN) in user functions for PPC
*/

enum uppEntryProcs
{
	uppArgInfoEntryProcInfo = uppArgInfoEntry,
	uppMainEntryProcInfo01 = uppMainEntry01,
	uppMainEntryProcInfo02 = uppMainEntry02,
	uppMainEntryProcInfo03 = uppMainEntry03,
	uppMainEntryProcInfo04 = uppMainEntry04,
	uppMainEntryProcInfo05 = uppMainEntry05,
	uppMainEntryProcInfo06 = uppMainEntry06,
	uppMainEntryProcInfo07 = uppMainEntry07,
	uppMainEntryProcInfo08 = uppMainEntry08,
	uppMainEntryProcInfo09 = uppMainEntry09,
	uppMainEntryProcInfo10 = uppMainEntry10,
	uppMainEntryProcInfo11 = uppMainEntry11,
	uppMainEntryProcInfo12 = uppMainEntry12,
	uppMainEntryProcInfo13 = uppMainEntry13
};

#endif /*powerc*/
#endif /*MACINTOSH*/
#endif /*DYNLOAD_H*/
