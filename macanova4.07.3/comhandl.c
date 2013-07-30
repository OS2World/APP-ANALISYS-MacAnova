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


#include "globals.h"

#ifdef MACINTOSH
#include "macIface.h"
#endif /*MACINTOSH*/

#ifdef WXWIN
#include "wx/wxIface.h"
#endif  /* WXWIN */

#ifdef MPW
#include <Memory.h>
#define entry Entry
#endif /*MPW*/

/*
  971104  added checks for length asked for being positive
  990215  Changed myerrorout() to putOutErrorMsg()
*/
/* macros are in to ignore and then reenable SIGINT but the cost is too high */
#undef IGNOREINT
#undef CATCHINT
#define IGNOREINT
#define CATCHINT

static void          **checkhandle(void **h)
{

	if (h == (void **) 0)
	{ /* unable to allocate memory */
		char        *outofmemory;
	
#ifndef MACINTOSH
		outofmemory = "ERROR: Not enough memory.  Try deleting variables";  
#else /*MACINTOSH*/
		outofmemory = "ERROR: Out of memory.  Try deleting variables; type 'help(memory)' for info";  
#endif /*MACINTOSH*/
		if(!FatalError)
		{
			putOutErrorMsg(outofmemory);
		}
	}
	else if(h == BADHANDLE)
	{ /* myNewHandle reports impossible request and has printed error message*/
		h = (void **) 0;
	}
	return (h);
} /*checkhandle()*/


static char          **gethandle(long n,long keepLocked)
/* get handle to n bytes and put into table */
{
	void          **h;
	WHERE("gethandle");
	
	IGNOREINT;
	
	if (n > 0)
	{
		h = checkhandle(myNewHandle(n, keepLocked));

		if (h != (void **) 0)
		{
			Nhandles++;
			if(Nhandles > Maxhandles)
			{
				Maxhandles = Nhandles;
			}
		}
	} /*if (n > 0)*/
	else
	{
		h = (void **) 0;
	} /*if (n > 0){}else{}*/
	
	CATCHINT;
	
	return ((char **) h);

} /* gethandle()*/

static char   **growhandle(char **h, long n, long keepLocked)
/* make a handle bigger */
{
	void          **h2;
	void          **entry;
	WHERE("growhandle");
	
	IGNOREINT;
	/* find handle in entry list */
	entry = myFindHandle((void **) h);

	if (entry == (void **) 0 || n <= 0)
	{
		h2 = (void **) 0;
	}
	else
	{
		h2 = checkhandle(mySetHandleSize(entry, n, keepLocked));
	}
	CATCHINT;
	
	return ((char **) h2);
} /* growhandle()*/

static char   **duphandle(char **h, long keepLocked)
/* duplicate a handle */
{
	void          **h2;
	void          **entry;
	WHERE("duphandle");
	
	IGNOREINT;

	/* find handle in entry list */
	entry = myFindHandle((void **) h);
	if (entry == (void **) 0)
	{
		h2 = (void **) 0;
	}
	else
	{
		h2 = checkhandle(myDupHandle(entry, keepLocked));

		if (h2 != (void **) 0)
		{
			Nhandles++;
		}
	}
	CATCHINT;
	
	return ((char **) h2);
} /*duphandle()*/

char      **mygethandle(long n)
{
	char       **newhandle;

	newhandle = (n > 0) ? gethandle(n, 0) : (char **) 0;

	return (newhandle);
} /* mygethandle()*/

/*
   special entry to be used if it may not be safe to move storage
*/

char      **mygetsafehandle(long n)
{
	return ((n > 0) ? gethandle(n, 1) : (char **) 0);
} /* mygetsafehandle()*/

void mydisphandle(char **h)
{
    void          **entry;
	
	WHERE("mydisphandle");
	
	if (h != (char **) 0)
	{ /* something to dispose of */
		IGNOREINT;
		
		/* find entry in handle table */
		entry = myFindHandle((void **) h);
	
		if (entry != (void **) 0)
		{
			myDisposHandle(entry);
			Nhandles--;
		}
		CATCHINT;	
	}
} /*mydisphandle()*/

/* dispose of up to 5 handles on one call */
void mydisphandle5(char ** h1, char ** h2, char ** h3, char ** h4, char ** h5)
{
	mydisphandle(h1);
	mydisphandle(h2);
	mydisphandle(h3);
	mydisphandle(h4);
	mydisphandle(h5);
} /*mydisphandle5()*/

/* dispose of n handles on one call */
void mydispnhandles(char *** h, long n)
{
	if(h != (char ***) 0)
	{
		while(n-- > 0)
		{
			mydisphandle(*h);
			*h++ = (char **) 0;
		}
	}	
} /*mydispnhandles()*/

char          **mygrowhandle(char ** h, long n)
{
	return ((n > 0) ? growhandle(h, n, 0) : (char **) 0);
} /*mygrowhandle()*/

char          **mygrowsafehandle(char ** h, long n)
{
	return ((n > 0) ? growhandle(h, n, 1) : (char **) 0);
} /*mygrowsafehandle()*/


char          **myduphandle(char **h)
{
	return (duphandle(h, 0));
} /*myduphandle()*/

char          **mydupsafehandle(char **h)
{
	return (duphandle(h, 1));
} /*mydupsafehandle()*/

long myvalidhandle(char **h)
{
	return (myValidHandle((void **) h));
} /*myvalidhandle()*/

long myhandlelength(char ** h)
{
	void        **entry;
	long          length;
	WHERE("myhandlelength");
	
	IGNOREINT;
	
	if (h == (char **) 0)
	{
		length = NULLHANDLE;
	}
	else
	{		
		/* find entry in handle table */
		entry = myFindHandle((void **) h);
		if (entry == (void **) 0)
		{
			length = CORRUPTEDHANDLE;
		}
		else
		{
			length = myGetHandleSize(entry);
		}
	}
	CATCHINT;
	return (length);
} /*myhandlelength()*/

void mylockhandle(char **h)
{
	void        **entry;
	
	IGNOREINT;
	
	if (h != (char **) 0 && (entry = myFindHandle((void **) h)) != (void **) 0)
	{
		myHLock(entry);
	}
	CATCHINT;
} /*mylockhandle()*/

void myunlockhandle(char **h)
{
	void        **entry;
	
	IGNOREINT;
	if (h != (char **) 0 && (entry = myFindHandle((void **) h)) != (void **) 0)
	{
		myHUnlock(entry);
	}
	CATCHINT;
} /*myunlockhandle()*/

char *mygetpointer(long n)
{
	char           *newpointer;

	IGNOREINT;
	newpointer = (n > 0) ? (char *) myNewPtr(n) : (char *) 0;
	CATCHINT;

	return (newpointer);
} /*mygetpointer()*/

void myfreepointer(char *p)
{
	IGNOREINT;
	myDisposePtr((void *) p);
	CATCHINT;
} /*myfreepointer()*/
