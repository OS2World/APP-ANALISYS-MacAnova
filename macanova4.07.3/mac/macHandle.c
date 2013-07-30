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
#pragma segment Handles
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

/*
   971023 changed mySetHandleSize so that argument is disposed of only
          if new allocation is successful
*/
#if defined(MPW) || defined (MW_CW)
#include <Memory.h>
#include <Errors.h>
#include <Stdlib.h>

#ifdef MPW1
void ResrvMem();
OSErr MemError();
#endif /*MPW1*/

#define MemErr MemError()
#else /*MPW*/
#include <MemoryMgr.h>
#endif /*MPW*/

#ifdef USEMAXBLOCK
#define isRoom(N) (N < MaxBlock())
#else /*USEMAXBLOCK*/
#define isRoom(N) (1)
#endif /*USEMAXBLOCK*/

#include "globals.h"

#ifndef MACIFACEH__
#include "macIface.h"
#endif /*MACIFACEH__*/

/*
  New version of 930125

  931015  infoblock structures no longer have a component 'next'

  A static array of infoblock structures is now used so as to reduce the number
  of ROM references.  The first long word of the data contains the true
  handle.

  Handlestart is the most recent ("youngest") handle allocated.

  infoblock(h).next is next "older" handle, or 0 if h is the "oldest"

  Version of 950125 added myValidHandle() and added static InvalidHandle

  Version of 950804 implemented limits on warehouse lengths

  Version of 960718 added check for MAXHANDLESIZE
*/

#define MEMINC(N) \
				(\
					CurrentMemory += (N) + DATAOFFSET, \
					MaxMemory = (CurrentMemory > MaxMemory) ?\
						CurrentMemory : MaxMemory\
				)
#define MEMDEC(N) (CurrentMemory -= (N) + DATAOFFSET)

typedef struct infoblock
{
	char       *data;         /* pointer to data */
	char      **realHandle;   /* true system handle */
	long        length;       /* number of bytes of data*/
	short       next;         /* index of next handle in chain */
} infoblock, *infoblockptr;

#define DOUBLESIZE    sizeof(double)
#define SYMBOLSIZE    sizeof(Symbol)
#define SHORTSYMBOLSIZE    sizeof(ShortSymbol)

#define LISTSIZE (DEFAULTLISTLENGTH * sizeof(Symbolhandle))
#define MAXDBLWAREHOUSESIZE  (NWAREHOUSES * DOUBLESIZE)

#define POW2          3  /*log2 DOUBLESIZE*/
#define MASK          0x07 /*DOUBLESIZE - 1*/

#ifdef POW2
#define MODDOUBLE(A)  ((A) & MASK)
#define DIVDOUBLE(A)  ((A) >> POW2)
#else /*POW2*/
#define MODDOUBLE(A)  ((A) % DOUBLESIZE)
#define DIVDOUBLE(A)  ((A) / DOUBLESIZE)
#endif /*POW2*/

#define DATAOFFSET    DOUBLESIZE /* DOUBLESIZE to retain alignment */

extern long            CurrentMemory, MaxMemory; /* defined in comhandl.c */
long                   UseResrvMem = 0;
static infoblockptr    Handles = (infoblockptr) 0;

/*
	myInitHandles() initializes these as follows:
	
	char **invalidThing = (char **) &InvalidThing;
	InvalidThing[DATAOFFSET] = '\0';
	invalidThing[0] = InvalidThing;
	InvalidData = InvalidThing + DATAOFFSET;
	InvalidRealHandle = (char **) (InvalidThing + 1);
	InvalidBlock.data = InvalidData;
	InvalidBlock.realHandle = InvalidRealHandle;
	InvalidBlock.length = 0;
	InvalidBlock.next = -1;
	InvalidHandle = &InvalidBlock.data;

	InvalidThing corresponds to actual memory allocated by NewHandle()
	InvalidRealHandle is different from what a valid infoblock.realHandle
	should be.
	InvalidBlock has same relationship to InvalidThing as a valid infoblock
	has to the actual memory allocated by NewHandle(), except that its
	realHandle is differs from the value of the first 4 bytes of InvalidThing
	**InvalidHandle points is InvalidThing[DATAOFFSET].
*/

static char            InvalidThing[DATAOFFSET+1];
static infoblock       InvalidBlock;
static char          **InvalidHandle;
static char           *InvalidData;
static char          **InvalidRealHandle;

/*
	Warehouse handles of sizes {1,2,...,NWAREHOUSES}*DOUBLESIZE in
	Warehouse[0],...,Warehouse[NWAREHOUSES-1], LISTSIZE
	in Warehouse[NWAREHOUSES], and SYMBOLSIZE in Warehouse[NWAREHOUSES+1]

  950804: Implemented maximum warehouse sizes.  Currently all warehouse have
          length of MAXWAREHOUSELENGTH (settable in Makefile).  Another
          possibility would be to have an array MaxWareHouseLength[]
          initialized in myInitHandles() with differing lengths depending
          on the warehouse
*/

#ifndef NWAREHOUSES
#define NWAREHOUSES    10 /* number of basic warehouses */
#endif /*NWAREHOUSES*/
#define EXTRAWAREHOUSES 3 /* number of additional warehouses */

#ifndef MAXWAREHOUSELENGTH /* maximum number of handles in any warehouse */
#define MAXWAREHOUSELENGTH      16
#endif /*MAXWAREHOUSELENGTH*/

#define LISTWAREHOUSE   NWAREHOUSES
#define SYMBOLWAREHOUSE (NWAREHOUSES+1)
#define SHORTSYMBOLWAREHOUSE (NWAREHOUSES+2)

/*
  Warehouse is a static array (allocated by NewPtr), each element of which is
  the start of a singly linked list of allocated (by the operating system)
  handles of certain fixed sizes, which have been de-allocated by
  mydisphandle() but are not given up to the system in the expectation that
  they will be needed again.  There are warehouses for handles of
  j*sizeof(double), j = 1,...,NWAREHOUSES and for handles whose size is
  sizeof(Symbol) and for handles whose size is that of the default size for a
  LIST. Warehouse[i] contains the index (in array Handles) of the most
  recently disposed of handle in that warehouse.  Handles[Warehouse[i]].next
  is the index of the next in the warehouse, etc.  Warehouse[i] == -1 means
  the warehouse is empty, and the component 'next' of the first handle put in
  a warehouse (the end of the chain) is -1.
*/
static short       *Warehouse; /*Warehouse[NWAREHOUSES+EXTRAWAREHOUSES]*/

/* The following are globals for debugging purposes; see mainpars.y */
long                WarehouseLength[NWAREHOUSES+EXTRAWAREHOUSES];
short               Nwarehouses = NWAREHOUSES+EXTRAWAREHOUSES;
unsigned short      WarehouseLimits[NWAREHOUSES+EXTRAWAREHOUSES];

#ifdef COUNTMISSES
long                WarehouseHits[NWAREHOUSES+EXTRAWAREHOUSES];
long                WarehouseMisses[NWAREHOUSES+EXTRAWAREHOUSES];
#endif /*COUNTMISSES*/

/*
   If a change is made to have varying maximum warehouse lengths, change
   the following macro
*/

#define maxWarehouseLength(I)  WarehouseLimits[I]

#ifdef USEFINDWAREHOUSE
/* cost of using findWarehouse is a function call*/
static short findWarehouse(long length)
{
	if(length == SYMBOLSIZE)
	{
		return(SYMBOLWAREHOUSE);
	}
	if(length == LISTSIZE)
	{
		return (LISTWAREHOUSE);
	}
	if (length == SHORTSYMBOLSIZE)
	{
		return(SHORTSYMBOLWAREHOUSE);
	}

	return ((MODDOUBLE(length) == 0 && length <= MAXDBLWAREHOUSESIZE) ?
			DIVDOUBLE(length) - 1 : -1);
} /*findWarehouse()*/
#endif /*USEFINDWAREHOUSE*/

/*
  FreeList is the start of a singly linked list of "slots" in array Handles
  which do not have actual handles assigned.  Specifically, if FreeList == i,
  Handles[i] has not been allocated, and Handles[i].next is the index of the
  the next unallocated slot.  The last unallocated slot has 'next' component
  -1, and FreeList == -1 means there are no more unallocated slots.

*/
static short           FreeList = -1;

/* LastHandle is the highest index in Handles that has been used */
static short           LastHandle = -1;

/* MaxHandles is the number of handles that can be used */
static short           MaxHandles = -1;

static void    freeList(short list)
{
	register short  i, next;
	infoblockptr    info;
	char          **h;

	for(i = list;i >= 0;i = next)
	{
		info = Handles + i;
		next = info->next;
		MEMDEC(info->length);
		h = info->realHandle;
		DisposHandle((Handle) h);
		info->realHandle = InvalidRealHandle;;
		info->data = InvalidData;
		info->length = 0;
		/* add to free list */
		info->next = FreeList;
		FreeList = i;
	} /*for(i = list;i >= 0;i = next)*/
} /*freeList()*/

/*
   keepLocked != 0 is a signal that nothing should be done that will
   move the contents of memory that has already been allocated.  In
   the current implementation, everything is allocated using malloc() and
   its relatives and so keepLocked is ignored.  On a Mac, collectGarbage(0)
   unlocks all handles and allows the system to move them, while
   collectGarbage(1) releases only handles in warehouses

   950805 added code to zero WarehouseLength[]

   970620 now also frees up any extra input strings
*/

void    collectGarbage(long keepLocked)
{
	register short  i;
	infoblockptr    info = Handles;
	char          **h;

	cleanInputlevels(0); /* dispose of any extra input strings */

	/* discard warehoused handles */
	for(i = 0;i<Nwarehouses;i++)
	{
		freeList(Warehouse[i]);
		Warehouse[i] = -1;
		WarehouseLength[i] = 0;
#ifdef COUNTMISSES
		WarehouseHits[i] = 0;
		WarehouseMisses[i] = 0;
#endif /*COUNTMISSES*/
	} /*for(i = 0;i<Nwarehouses;i++)*/

	/* at this point there should be no warehoused handles */

	if (!keepLocked)
	{
		for(i = 0;i<LastHandle;i++)
		{
			if((h = info->realHandle) != InvalidRealHandle)
			{					/* unlock all handles in use */
				HUnlock(h);
			}
			info++;
		}

		info = Handles;
		for(i = 0;i<LastHandle;i++)
		{
			if((h = (char **) info->realHandle) != InvalidRealHandle)
			{					/* MoveHHi all handles in use and relock */
				MoveHHi(h);
				HLock(h);
				info->data = (char *) (*h + DATAOFFSET);
			}
			info++;
		} /*for(i = 0;i<LastHandle;i++)*/
	} /*if (!keepLocked)*/	
} /*collectGarbage()*/

#define validHandle(H) ((H != (void **) 0) &&\
	((infoblockptr) H)->realHandle ==\
	*((char ***) (((infoblockptr) H)->data - DATAOFFSET)))

long myValidHandle(void ** h)
{
#ifdef validHandle
	return (validHandle(h));
#else /*validHandle*/
	infoblockptr   info = (infoblockptr) h;
	char        ***self;

	if (h != (void **) 0)
	{
		self = (char ***) (info->data - DATAOFFSET);
		if(info->realHandle == *self)
		{
			return (1);
		}
	}
	return (0);
#endif /*validHandle*/
} /*myValidHandle()*/

#ifndef validHandle
#define validHandle(H) myValidHandle(H)
#endif /*validHandle*/

void **myFindHandle(void ** h)
{
	if (!validHandle(h))
	{
		putOutErrorMsg("WARNING: Probable memory corruption");
		h = (void **) 0;
	}

	return (h);
} /*myFindHandle()*/


/*
   get handle to needed bytes

   941027 added argument keepLocked
   if keepLocked != 0, then the only garbage collection is freeing of
   warehoused handles.
   In MacAnova versions where storage may move, keepLocked is
   directs that nothing should be done that could move handles.

   950805 added code to decrement WarehouseLength[whichWarehouse]
 */
void          **myNewHandle(long length, long keepLocked)
{
	long            needed = length + DATAOFFSET;
	char          **h = (char **) 0;
	char         ***self;
	infoblockptr    info;
	short           handleIndex, whichWarehouse, next;
	OSErr           err = memFullErr;

#ifdef MAXHANDLESIZE
	if (length > MAXHANDLESIZE)
	{
		putOutErrorMsg("ERROR: Request for too big a chunk of memory");
		return(BADHANDLE);
	}
#endif /*MAXHANDLESIZE*/

#ifdef USEFINDWAREHOUSE
	whichWarehouse = findWarehouse(length);
#else /*USEFINDWAREHOUSE*/
	if(length == SYMBOLSIZE)
	{
		whichWarehouse = SYMBOLWAREHOUSE;
	}
	else if(length == LISTSIZE)
	{
		whichWarehouse = LISTWAREHOUSE;
	}
	else if (MODDOUBLE(length) == 0 && length <= MAXDBLWAREHOUSESIZE)
	{
		whichWarehouse = DIVDOUBLE(length) - 1;
	}
	else if(length == SHORTSYMBOLSIZE)
	{
		whichWarehouse = SHORTSYMBOLWAREHOUSE;
	}
	else
	{
		whichWarehouse = -1;
	}
#endif /*USEFINDWAREHOUSE*/

#ifdef COUNTMISSES
	if (whichWarehouse >= 0)
	{
		if (Warehouse[whichWarehouse] < 0)
		{
			WarehouseMisses[whichWarehouse]++;
		}
		else
		{
			WarhouseHits[whichWarehouse]++;
		}
	}
	
#endif /*COUNTMISSES*/
	if(whichWarehouse < 0 || (handleIndex = Warehouse[whichWarehouse]) < 0)
	{ /* none warehoused of right size */
		handleIndex = FreeList;
		whichWarehouse = -1;
	}

	if(handleIndex < 0)
	{ /* nothing in FreeList; out of slots */
		putOutErrorMsg("Compacting memory, please stand by");
		collectGarbage(keepLocked);
		handleIndex = FreeList;
		if(handleIndex < 0)
		{
			myAlert("Cannot get more memory, probably because of too many variables or too long a list.\nThere is no way to continue");
			FatalError = 1;
			return ((void **) 0);
		}
	} /*if(handleIndex < 0)*/
	if(handleIndex == LastHandle)
	{
		LastHandle++;
	}
	info = Handles + handleIndex;
	if(whichWarehouse < 0)
	{ /* was not warehoused handle */
		if (isRoom(needed))
		{
			if(!UseResrvMem || (ResrvMem(needed),err = MemErr) == noErr)
			{
				h = (char **) NewHandle(needed);
				err = MemErr;
			}
		} /*if (isRoom(needed))*/

		if(err != noErr)
		{ /* try again, after allowing system to compact relocatable blocks */
			putOutErrorMsg("Compacting memory, please stand by");
			collectGarbage(keepLocked);
			if(!UseResrvMem || (ResrvMem(needed),err = MemErr) == noErr)
			{
				h = (char **) NewHandle(needed);
				err = MemErr;
			}
		} /*if(err != noErr)*/

		if(err == noErr)
		{
			MEMINC(length);
		}
	} /*if(whichWarehouse<0)*/
	else
	{ /* was warehoused handle */
		err = noErr;
		h = info->realHandle;
		WarehouseLength[whichWarehouse]--;
	} /*if(whichWarehouse<0){}else{}*/

	if(err == noErr)
	{ /* got handle; now initialize */
		HNoPurge(h);
		HLock(h);
		info->data = *h + DATAOFFSET;
		info->realHandle = h;
		info->length = length;
		self = (char ***) *h;
		*self = h; /* used to validate handle */
		h = (char **) info;
		next = info->next;
		if(whichWarehouse >= 0)
		{/* was warehoused handle */
			Warehouse[whichWarehouse] = next;
		}
		else
		{
			FreeList = next;
		}
		info->next = 0;
	} /* if(err == noErr) */
	else
	{
		h = (char **) 0;
	}

	return ((void **) h);
}

/*
   dispose of a handle

   950804: added code to check and increment WarehouseLength[whichWarehouse]
 */

void myDisposHandle(void **h1)
{
	infoblockptr info = (infoblockptr) h1;
	short        handleIndex = info - Handles;
	short        whichWarehouse, k;
	long         length;
	Handle       h;
	char      ***self;

	if(h1 != (void **) 0)
	{
		h = (Handle) info->realHandle;
		/* make sure myFindHandle would report not found */
		self = (char ***) (*h);
		*self = InvalidHandle;
		info->data = InvalidData;
		length = info->length;

		/* warehouse Symbols, standard lists and small REAL vectors */
#ifdef USEFINDWAREHOUSE
		whichWarehouse = findWarehouse(length);
#else /*USEFINDWAREHOUSE*/
		if(length == SYMBOLSIZE)
		{
			whichWarehouse = SYMBOLWAREHOUSE;
		}
		else if(length == LISTSIZE)
		{
			whichWarehouse = LISTWAREHOUSE;
		}
		else if (MODDOUBLE(length) == 0 && length <= MAXDBLWAREHOUSESIZE)
		{
			whichWarehouse = DIVDOUBLE(length) - 1;
		}
		else if (length == SHORTSYMBOLSIZE)
		{
			whichWarehouse = SHORTSYMBOLWAREHOUSE;
		}
		else
		{
			whichWarehouse = -1;
		}
#endif /*USEFINDWAREHOUSE*/

		if(whichWarehouse >= 0 &&
		   WarehouseLength[whichWarehouse] < maxWarehouseLength(whichWarehouse))
		{
			if((k = Warehouse[whichWarehouse]) >= 0)
			{ /* warehouse is not empty */
				info->next = k;
			}
			else
			{
				info->next = -1;
			}
			Warehouse[whichWarehouse] = handleIndex;
			WarehouseLength[whichWarehouse]++;
			HUnlock(h);
		} /*if(whichWarehouse >= 0)*/
		else
		{ /* dispose of everything else */
			MEMDEC(info->length);
			info->length = 0;
			info->realHandle = InvalidRealHandle;
			info->data = InvalidData;
			info->next = FreeList;
			FreeList = handleIndex;
			DisposHandle(h);
		} /*if(whichWarehouse >= 0){}else{}*/
	} /*if(h1 != (void **) 0)*/
} /*myDisposHandle()*/

/*
   Change the size of a handle.  This no longer uses SetHandleSize() so
   as not to damage input handle if reallocation doesn't go.

   960718 Added check for BADHANDLE
*/

void          **mySetHandleSize(void **h1, long newn, long keepLocked)
{
	long            needed = newn + DATAOFFSET;
	long            oldn = myGetHandleSize(h1);
	infoblockptr    info = (infoblockptr) h1;
	Handle          h, newh;
	OSErr           err = memFullErr;
	WHERE("mySetHandleSize");

	if(newn != oldn)
	{ /* only do something if desired size differs */
		h = (Handle) info->realHandle;
		HUnlock(h);
#if (0) /*code no longer used*/
		/* this code uses SetHandleSize which can destroy h if unsuccessful */
#ifdef MAXHANDLESIZE
		if (newn > MAXHANDLESIZE)
		{
			putOutErrorMsg("ERROR: Request for too big a chunk of memory");
			return (BADHANDLE);
		}
#endif   /* MAXHANDLESIZE */
		if (isRoom(needed))
		{
			if(!UseResrvMem || (ResrvMem(needed),err = MemErr) == noErr)
			{
				SetHandleSize(h, needed);
				err = MemErr;
			}
		}
		if(err != noErr)
		{/* try again, after allowing system to compact relocatable blocks */
			putOutErrorMsg("Compacting memory, please stand by");
			collectGarbage(keepLocked);
			if(!UseResrvMem || (ResrvMem(needed),err = MemErr) == noErr)
			{
				SetHandleSize(h, needed);
				err = MemErr;
			}
		} /*if(err == noErr)*/
		if(err == noErr)
		{
			HNoPurge(h);
			HLock(h);
			MEMDEC(oldn);
			MEMINC(newn);
			/* info->realHandle should still be correct */
			info->data = (char *) *h + DATAOFFSET;
	 		info->length = newn; /* set new length */
			h = (Handle) info;
		} /*if(err == noErr)*/
#else /*0*/
		/* This code gets new space and then copies old contents */
		newh = (Handle) myNewHandle(newn, keepLocked);
		if(newh != (Handle) 0 && newh != (Handle) BADHANDLE)
		{
			BlockMove((Ptr) *h1, (Ptr) *newh, (oldn < newn) ? oldn : newn);
			myDisposHandle(h1);
		} /*if(newh != (Handle) 0 && newh != (Handle) BADHANDLE)*/
#endif /*0*/
		h = newh;
		/* get rid of handle */
	} /*if(newn != oldn)*/
	else
	{
		h = (Handle) h1;
	}

	return ((void **) h);

} /*mySetHandleSize()*/

/*
   Function to duplicate a handle.

   941027 added argument keepLocked
   960718 Added check for BADHANDLE after call to myNewHandle()
*/

void          **myDupHandle(void **h, long keepLocked)
{
	long        length = myGetHandleSize(h);
	char      **h1 = (char **) h;
	char      **h2 = (char **) myNewHandle(length, keepLocked);

	if(h2 != (char **) 0 && h2 != (char **) BADHANDLE)
	{
		BlockMove((Ptr) *h1, (Ptr) *h2, length);
	} /*if(h2 != (char **) 0 && h2 != (char **) BADHANDLE)*/

	return ((void **) h2);
} /*myDupHandle()*/


long myGetHandleSize(void ** h)
{
	infoblockptr    info = (infoblockptr) h;

	return (info->length);
} /*myGetHandleSize()*/

void myHLock(void **h)
{
	infoblockptr   info = (infoblockptr) h;

	HLock((Handle) info->realHandle);
} /*myHLock()*/

void myHUnlock(void **h)
{
	infoblockptr   info = (infoblockptr) h;

	HUnlock((Handle) info->realHandle);
} /*myHUnlock()*/

/* Note: memory allocated by myNewPtr is not counted */
void *myNewPtr(long length)
{
	Ptr         ptr = NewPtr(length);

	if(MemErr != noErr)
	{
		ptr = (Ptr) 0;
	}
	return ((void *) ptr);
} /*myNewPtr()*/

void myDisposePtr(void *p)
{
	DisposPtr((Ptr) p);
} /*myDisposePtr()*/

/*
   Added 950809
*/

void setWarehouseLimits(long standard)
{
	int         i;
	
	for (i=0;i < Nwarehouses;i++)
	{
		WarehouseLimits[i] = standard;
#ifdef COUNTMISSES
		WarehouseHits[i] = WarehouseMisses[i] = 0;
#endif /*COUNTMISSES*/
	} /*for (i=0;i < Nwarehouses;i++)*/
	WarehouseLimits[0] = 10*standard;
	WarehouseLimits[SYMBOLWAREHOUSE] = 4*standard;
} /*setWarehouseLimits()*/

/*
   950805 added code to initialize WarehouseLength[]
*/

long myInitHandles(short maxhandles)
{
	infoblockptr       info;
	short              i;
	char             **invalidThing = (char **) &InvalidThing;
	
	if(Handles == (infoblockptr) 0)
	{ /* test should be unnecessary but play it safe */
		InvalidThing[DATAOFFSET] = '\0';
		invalidThing[0] = InvalidThing;
		InvalidData = InvalidThing + DATAOFFSET;
		InvalidRealHandle = (char **) (InvalidThing + 1);
		InvalidBlock.data = InvalidData;
		InvalidBlock.realHandle = InvalidRealHandle;
		InvalidBlock.length = 0;
		InvalidBlock.next = -1;
		InvalidHandle = &InvalidBlock.data;
		MaxHandles = maxhandles;
		Handles = (infoblockptr) myNewPtr((size_t) maxhandles * sizeof(infoblock));
		if(Handles == (infoblockptr) 0)
		{
			FatalError = 1;
			return (0);
		}
		info = Handles;
		for(i = 0;i<maxhandles;i++, info++)
		{
			info->data = InvalidData;
			info->realHandle = InvalidRealHandle;
			info->length = 0;
			info->next = (i+1 < maxhandles) ? i+1 : -1;
		} /*for(i = 0;i<maxhandles;i++, info++)*/

		FreeList = 0; /* index of first free handle */
		LastHandle = 0; /* index of last handle allocated */
		Warehouse = (short *) NewPtr((size_t)(Nwarehouses * sizeof(short)));
		if(MemErr != noErr)
		{
			FatalError = 1;
			return (0);
		}

		for(i = 0;i < Nwarehouses;i++)
		{
			Warehouse[i] = -1;
			WarehouseLength[i] = 0;
		} /*for(i = 0;i < Nwarehouses;i++)*/
		setWarehouseLimits(MAXWAREHOUSELENGTH);
 	} /*if(Handles == (infoblockptr) 0)*/
	return (1);
} /*myInitHandles()*/

/* dispose of all handles */
void myDispAll(void)
{
	infoblockptr       info = Handles;
	short              i;

	if(info != (infoblockptr) 0)
	{
		for(i=0;i<MaxHandles;i++, info++)
		{
			if (info->realHandle != InvalidRealHandle)
			{
				DisposHandle((Handle) info->realHandle);
			}
		}
		myDisposePtr((void *) Handles);
		Handles = (infoblockptr) 0;
	}
} /*myDispAll()*/

/* global for communication with Assign() in Symbol.c */
extern long    Nassigned; /* number of type ASSIGNED */

static void printMemoryUsage(char * where, char * what, long data [])
{
	char          *dbout = DBOUT;
	int            i;
	
	sprintf(dbout, "%s: %s", where, what);
	dbout += strlen(dbout);

	for (i = 0; i < Nwarehouses; i++)
	{
		sprintf(dbout, " %ld,", data[i]);
		dbout += strlen(dbout);
	}
	*(--dbout) = '\0';
	putOutMsg(DBOUT);
} /*printMemoryUsage()*/

/*
   Summarize memory usage.  May be called from cleanitup() or doMacAnova()

   971215 now reports total of warehoused handles
   In interpreting output, don't forget the history of commands;  each command
   in history uses up a handle, up to the total history length allowed (global
   HISTORY)
*/

void memoryUsage(char *where)
{
	long          warehoused = 0, i;
	
	for (i = 0; i < Nwarehouses; i++)
	{
		warehoused += WarehouseLength[i];
	}
	sprintf(DBOUT,
			"%s: Nscratch = %ld, Maxscratch = %ld, Nsymbols = %ld, Maxsymbols = %ld",
			where, Nscratch, Maxscratch, Nsymbols, Maxsymbols);
	putOutMsg(DBOUT);

	sprintf(DBOUT,
			"%s: Nhandles = %ld, Maxhandles = %ld, Nassigned = %ld, warehoused = %ld",
			where, Nhandles, Maxhandles, Nassigned, warehoused);
	putOutMsg(DBOUT);
	
	sprintf(DBOUT, "%s: CurrentMemory = %ld, MaxMemory = %ld",
			where, CurrentMemory, MaxMemory);
	putOutMsg(DBOUT);

	printMemoryUsage(where, "WarehouseLength[] =", WarehouseLength);
#ifdef COUNTMISSES
	printMemoryUsage(where, "WarehouseMisses[] =", WarehouseMisses);
	printMemoryUsage(where, "WarehouseHits[]   =", WarehouseHits);
#endif /*COUNTMISSES*/

} /*void memoryUsage()*/
