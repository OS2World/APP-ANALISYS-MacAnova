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
   NOTE: As of 960717 files unxHandl.c, dosHandl.c and wxhandl.c are 
   IDENTICAL.  As maintained on stat.umn.edu dosHandl.c and wxhandl.c
   in directories dos and wx, respectively, are actualll soft links to
   unxHandle.c in the directory containing most of the source files.

   971023 changed mySetHandleSize so that argument is disposed of only
          if new allocation is successful

   981216 changed one remaining free() in freeList() to mvFree().

   990215 changed myerrorout() to putOutMsg()
*/

/*
   Functions to handle MacAnova memory management on Unix and other systems
   where malloc() and its relatives are used.

   If memory management should be changed so that the blocks of memory
   allocated can move, as on the Macintosh, one thing to look out for is
   making sure that collectGarbage(1) does not move stuff.
*/


#if !defined(WXWINMOTIF) && !defined(WXWINMSW)
#include "globals.h"
#else /*WXWIN*/
#include "../globals.h"
#include "wxProto.h"
#endif /*WXWIN*/

#ifdef BCPP
#include <alloc.h>
#define malloc farmalloc
#define realloc farrealloc
#define free farfree
#endif  /* BCPP */

#define DATAOFFSET sizeof(infoblock)

#define MEMINC(N) (CurrentMemory += (N) + DATAOFFSET,\
				   MaxMemory = (CurrentMemory > MaxMemory) ?\
				   CurrentMemory : MaxMemory)

#define MEMDEC(N) (CurrentMemory -= (N) + DATAOFFSET)

#ifndef WXWIN
#define mvMalloc(N)    malloc(N)
#define mvRealloc(P,N) realloc(P,N)
#define mvFree(P)      free(P)
#else /*WXWIN*/
#define mvMalloc(N)    wxMalloc(N)
#define mvRealloc(P,N) wxRealloc(P,N)
#define mvFree(P)      wxFree(P)
#endif /*WXWIN*/

/*
  930504: Installed warehousing of handles of popular sizes.  When they
  are disposed of, the storage is not given up and thus they can be reused
  without call to malloc().

	Warehouse handles of sizes {1,2,...,NWAREHOUSES}*DOUBLESIZE in
	Warehouse[0],...,Warehouse[NWAREHOUSES-1], LISTSIZE
	in Warehouse[NWAREHOUSES], and SYMBOLSIZE in Warehouse[NWAREHOUSES+1]

  931105: Added function collectGarbage() to purge the warehouses

  950804: Implemented maximum warehouse sizes.  Currently all warehouse have
          length of MAXWAREHOUSELENGTH (settable in Makefile).  Another
          possibility would be to have an array MaxWareHouseLength[]
          initialized in myInitHandles() with differing lengths depending
          on the warehouse

  980721  ShortSymbols are now cached
*/

#ifndef NWAREHOUSES
#define NWAREHOUSES    9 /* number of basic warehouses */
#endif /*NWAREHOUSES*/

#define EXTRAWAREHOUSES 3 /* number of additional warehouses */

#ifndef MAXWAREHOUSELENGTH /* maximum number of handles in any warehouse */
#define MAXWAREHOUSELENGTH      16
#endif /*MAXWAREHOUSELENGTH*/

#define LISTWAREHOUSE        NWAREHOUSES
#define SYMBOLWAREHOUSE      (NWAREHOUSES+1)
#define SHORTSYMBOLWAREHOUSE (NWAREHOUSES+2)

#define DOUBLESIZE         sizeof(double)
#define SYMBOLSIZE         sizeof(Symbol)
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

typedef union infoblock
{
	double         dummy; /* to force correct alignment */
	struct
	{
		char            *data; /* points to actual data */
		long             length;  /* number of bytes */
		union infoblock *next; /* points to next handle in warehouse, if any*/
	} info;
} infoblock, *infoblockptr;

/*
   The following are intialized in myInitHandles() as follows:
	InvalidData = &InvalidBlock.info.data;
	InvalidBlock.info.data = InvalidData;
	InvalidBlock.info.length = 0;
	InvalidBlock.info.next = (infoblockptr) 0;
*/

static char           *InvalidData;
static infoblock       InvalidBlock;

static infoblockptr Warehouse[NWAREHOUSES+EXTRAWAREHOUSES];

static long         WarehouseLength[NWAREHOUSES+EXTRAWAREHOUSES];
static short        Nwarehouses = NWAREHOUSES+EXTRAWAREHOUSES;

static unsigned short  WarehouseLimits[NWAREHOUSES+EXTRAWAREHOUSES];

#ifdef COUNTMISSES
static long         WarehouseHits[NWAREHOUSES+EXTRAWAREHOUSES];
static long         WarehouseMisses[NWAREHOUSES+EXTRAWAREHOUSES];
#endif /*COUNTMISSES*/

#define maxWarehouseLength(I)  (WarehouseLimits[I])

#ifdef USEFINDWAREHOUSE
/* cost of using findWarehouse is a function call*/
static short findWarehouse(long length)
{
	if (length == SYMBOLSIZE)
	{
		return(SYMBOLWAREHOUSE);
	}

	if(length == LISTSIZE)
	{
		return (LISTWAREHOUSE);
	}

	if (MODDOUBLE(length) == 0 && length <= MAXDBLWAREHOUSESIZE)
	{
		return (DIVDOUBLE(length) - 1);
	}

	if (length == SHORTSYMBOLSIZE)
	{
		return(SHORTSYMBOLWAREHOUSE);
	}

	return (-1);
} /*findWarehouse()*/
#endif /*USEFINDWAREHOUSE*/

/*
  Unix & MSDOS version of freeList()
  Called only by collectGarbage()
  981216 changed free() to mvFree().  It should have been causing
         problems but wasn't
*/

static void    freeList(infoblockptr list)
{
	infoblockptr    link, next;
	char           *p;

	for(link = list;link != (infoblockptr) 0;link = next)
	{
		next = link->info.next;
		MEMDEC(link->info.length);
		link->info.data = InvalidData;
		p = (char *) link;
		mvFree((void *) p);
	}
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
/*
   Since this memory management is malloc() based, the only garbage
   collection possible is freeing handles in the Warehouses
*/

static void    collectGarbage(long keepLocked)
{
	register short  i = keepLocked; /* just to use argument */

	cleanInputlevels(0); /* dispose of any extra input strings */

	/* discard warehoused handles */
	for(i = 0;i < Nwarehouses;i++)
	{
		freeList(Warehouse[i]);
		Warehouse[i] = (infoblockptr) 0;
		WarehouseLength[i] = 0;
#ifdef COUNTMISSES
		WarehouseHits[i] = 0;
		WarehouseMisses[i] = 0;
#endif /*COUNTMISSES*/
	}
} /*collectGarbage()*/

#define validHandle(H) (H != (void **) 0 &&\
	((infoblockptr) H)->info.data == ((char *) H) + DATAOFFSET)

long myValidHandle(void **h)
{
	return (validHandle(h));
} /*myValidHandle()*/
	
void **myFindHandle(void ** h)
{
	if(!validHandle(h))
	{
		putOutErrorMsg("WARNING: Probable memory corruption");
		h = (void **) 0;
	}
	return (h);
} /*myFindHandle()*/

/*
   930505 version
   941027 added argument keepLocked
   In MacAnova versions where storage may move, keepLocked is
   directs that nothing should be done that could move handles.  In the
   current Unix implementation, storage does not float and hence keepLocked
   is ignored.  
   950805 added code to decrement WarehouseLength[whichWarehouse]
*/
void          **myNewHandle(long length, long keepLocked)
{
	/* get handle to length bytes */
	char           *p;
	short           whichWarehouse;
	infoblockptr    p1;
	WHERE("myNewHandle");
	
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

	p1 = (whichWarehouse >= 0) ?
		Warehouse[whichWarehouse] : (infoblockptr) 0;

	if (p1 != (infoblockptr) 0)
	{ /* found warehoused handle */
		Warehouse[whichWarehouse] = p1->info.next;
		p = p1->info.data;
		p1->info.data += DATAOFFSET; /* fixup pointer */
		p1->info.next = (infoblockptr) 0;
		WarehouseLength[whichWarehouse]--;
#ifdef COUNTMISSES
		WarehouseHits[whichWarehouse]++;
#endif /*COUNTMISSES*/
	}
	else
	{ /* no warehouse handle of right size */
#ifdef COUNTMISSES
		if (whichWarehouse >= 0)
		{
			WarehouseMisses[whichWarehouse]++;
		}
#endif /*COUNTMISSES*/

		p = (char *) mvMalloc((size_t) length + DATAOFFSET);

		if (p != (char *) 0)
		{
			p1 = (infoblockptr) p;
			p1->info.data = p + DATAOFFSET;
			p1->info.length = length;
			p1->info.next = (infoblockptr) 0;
			MEMINC(length);
		} /*if (p != (char *) 0)*/
		else
		{
			putOutErrorMsg("Compacting memory, please stand by");
/* free up warehoused handles and try again */
			collectGarbage(keepLocked);
			p = (char *) mvMalloc((size_t) length + DATAOFFSET);
			if (p != (char *) 0)
			{
				p1 = (infoblockptr) p;
				p1->info.data = p + DATAOFFSET;
				p1->info.length = length;
				p1->info.next = (infoblockptr) 0;
				MEMINC(length);
			}
		} /*if (p != (char *) 0){}else{}*/
	}

	return ((void **) p);
} /*myNewHandle()*/

/*
   930505 version
   950804: added code to check and increment WarehouseLength[whichWarehouse]
 */
void myDisposHandle(void **h)
{
	infoblockptr  p1 = (infoblockptr) h;
	long          length;
	short         whichWarehouse = -1;
	WHERE("myDisposHandle");

/* dispose of a handle */
	if (h != (void **) 0)
	{
		length = p1->info.length;

#ifdef USEFINDWAREHOUSE
		whichWarehouse = findWarehouse(length);
#else /*USEFINDWAREHOUSE*/
		if (length == SYMBOLSIZE)
		{
			whichWarehouse = SYMBOLWAREHOUSE;
		}
		else if (length == LISTSIZE)
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
		if(whichWarehouse < 0 ||
		   WarehouseLength[whichWarehouse] >= maxWarehouseLength(whichWarehouse))
		{/* not warehousable */
			MEMDEC(length);
			p1->info.data = InvalidData;
			free((void *) h);
		}
		else
		{
			p1->info.data -= DATAOFFSET; /* make it fail check if reused improperly */
			p1->info.next = Warehouse[whichWarehouse];
			Warehouse[whichWarehouse] = p1;
			WarehouseLength[whichWarehouse]++;
		}
	} /*if (h != (void **) 0)*/
} /*myDisposHandle()*/

/*
   Function analogous to realloc() to change the size of a handle
   It does not use realloc() so as not to put the original handle
   in jeopardy.

   941027 added argument keepLocked
   960718 Added check for BADHANDLE after call to myNewHandle()
*/
void          **mySetHandleSize(void **h, long newn, long keepLocked)
{
	/* change the size of a handle */
	char          **h1 = (char **) h, *h2;
	infoblockptr    p = (infoblockptr) h;
	long            length = p->info.length;
	short           whichWarehouse;
	WHERE("mySetHandleSize");
	
	if(length != newn)
	{ /* reallocate only if length is different */
#if (0) /*code no longer used*/
/*
   following code uses realloc which can invalidate existing handle on failure
*/
#ifdef MAXHANDLESIZE
		if (newn > MAXHANDLESIZE)
		{
			putOutErrorMsg("ERROR: Request for too big a chunk of memory");
			return (BADHANDLE);
		}
#endif   /* MAXHANDLESIZE */
		MEMDEC(length);
		h2 = (char *) mvRealloc((void *) (*h1 - DATAOFFSET),
							  (size_t) newn + DATAOFFSET);
		if (h2 != (char *) 0)
		{
			p = (infoblockptr) h2;
			p->info.data = h2 + DATAOFFSET;
			p->info.length = newn;
			p->info.next = (infoblockptr) 0;
			MEMINC(newn);
		} /*if (h2 != (char *) 0)*/
		else
		{
/*
   Free up warehoused space.  However, it is unsafe to try again since
   the attempt may have destroyed h1
*/
			putOutErrorMsg("Reclaiming memory, please stand by");
			collectGarbage(keepLocked);
		}
#else /*0*/
		/* allocate, copy, and dispose*/
		h2 = (char *) myNewHandle(newn, keepLocked);

		if (h2 != (char *) 0 && h2 != (char *) BADHANDLE)
		{
			char    *new = ((infoblockptr) h2)->info.data;
			char    *old = *h1;
			
			length = (length < newn) ? length : newn;
			
#ifndef USEMEMCPY
			while (length-- > 0)
			{
				*new++ = *old++;
			}
#else /*USEMEMCPY*/
			memcpy(new, old, length);
#endif /*USEMEMCPY*/
			/*
			  dispose of original only if successful allocation
			  This can lead to memory leak if caller doesn't dispose of h
			  but allows caller to handle failure better.
			*/
			myDisposHandle(h);
		} /*if (h2 != (char *) 0 && h2 != (char *) BADHANDLE)*/
#endif /*0*/
	}
	else
	{
		h2 = (char *) h1;
	}
	
	return ((void **) h2);
} /*mySetHandleSize()*/


/*
   Function to duplicate a handle.

   930505 version
   941027 added argument keepLocked
   960718 Added check for BADHANDLE after call to myNewHandle()
*/
void          **myDupHandle(void **h, long keepLocked)
{
	/* duplicate a handle and its contents */
	register long   length = ((infoblockptr) h)->info.length;
	register char  *old, *new;
	char           *h2;
	WHERE("myDupHandle");
	
	h2 = (char *) myNewHandle(length, keepLocked);
	if (h2 != (char *) 0 && h2 != (char *) BADHANDLE)
	{
		old = ((infoblockptr) h)->info.data;
		new = ((infoblockptr) h2)->info.data;
#ifndef USEMEMCPY
		while (length-- > 0)
		{
			*new++ = *old++;
		}
#else /*USEMEMCPY*/
		memcpy(new, old, length);
#endif /*USEMEMCPY*/
	} /*if (h2 != (char *) 0 && h2 != (char *) BADHANDLE)*/

	return ((void **) h2);
} /*myDupHandle()*/

long myGetHandleSize(void **h)
{
	return (((infoblockptr) h)->info.length);
} /*myGetHandleSize()*/

void myHLock(void **ch)
{
} /*myHLock()*/

void myHUnlock(void **ch)
{
} /*myHUnlock()*/

void *myNewPtr(long n)
{
	return ((void *) mvMalloc((size_t) n));
} /*myNewPtr()*/

void myDisposePtr(void *p)
{
	free(p);
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

void myInitHandles(void)
{
	short              i;
	static int         first = 1;
	
	if(first)
	{ /* test should be unnecessary but play it safe */
		first = 0;
		for (i=0;i < Nwarehouses;i++)
		{
			Warehouse[i] = (infoblockptr) 0;
			WarehouseLength[i] = 0;
		} /*for (i=0;i < Nwarehouses;i++)*/
		setWarehouseLimits(MAXWAREHOUSELENGTH);
		InvalidData = (char *) &InvalidBlock.info.data;
		InvalidBlock.info.data = InvalidData;
		InvalidBlock.info.length = 0;
		InvalidBlock.info.next = (infoblockptr) 0;
	} /*if(first)*/
} /*myInitHandles()*/

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

