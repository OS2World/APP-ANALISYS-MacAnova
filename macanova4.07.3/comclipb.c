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


#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment Symbol
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

/*
  970115 modifications to help connect CLIPBOARD with Windows Clipboard
     Moved Macintosh specific code to macUtils.c as mygetscrap() and
	 myputscrap().
  To connect CLIPBOARD on a new platform with a Clipboard, you need to
  (a)  Add #define HASCLIPBOARD to platform.h
  (b)  Add an appropriate #include "xxIface.h" here
  (c)  Write appropriate versions of mygetscrap() and myputscrap().  For
       Windows these are in wxtwind.cc and presumably versions for Motif will
	   be there as well.

  970124 modification to connect SELECTION with Motif XA_PRIMARY selection
  980215 bug fix in deleteClipboard()
*/

#include "globals.h"

#ifdef MACINTOSH
#include "macIface.h"
#endif /*MACINTOSH*/

#ifdef WXWIN
#include "wx/wxIface.h"
#endif /*WXWIN*/

/*
   Discard contents of CLIPBOARDSYMBOL (or SELECTIONSYMBOL) and
   replace with null string
   Does not interact with Clipboard or any Selection at all

   971104 added check for value of myhandlelength()
   980215 Fixed bug in check of handleLength
*/

static void deleteClipboard(Symbolhandle clipSymh)
{
	char       **contents;
	
	if (clipSymh != (Symbolhandle) 0)
	{
		long         handleLength;
		
		contents = STRING(clipSymh);
		handleLength = myhandlelength(contents);
		
		if (handleLength == NULLHANDLE || handleLength > 1)
		{
			setSTRING(clipSymh, mygethandle(1));
			if (STRING(clipSymh) != (char **) 0)
			{
				mydisphandle(contents);
				STRINGVALUE(clipSymh, 0) = '\0';
			}
			else
			{
				setSTRING(clipSymh, contents); /* restore to what it was */
			}
		}
		else if (handleLength != CORRUPTEDHANDLE)
		{
			(*contents)[0] = '\0';
		}
	} /*if (clipSymh != (Symbolhandle) 0)*/
} /*deleteClipboard()*/

#if !defined(HASCLIPBOARD) && !defined(HASSELECTION)

/*
   thisSymh should be CLIPBOARDSYMBOL
*/
static Symbolhandle copyFromClipboard(Symbolhandle thisSymh)
{	
	WHERE("copyFromClipboard");
	
	if (STRING(thisSymh) == (char **) 0)
	{ /* should never happen */
		if ((TMPHANDLE = mygethandle(1)) != (char **) 0)
		{
			setSTRING(thisSymh, TMPHANDLE);
			setNDIMS(thisSymh,1);
			setDIM(thisSymh,1,1);
			setNAME(thisSymh, CLIPBOARDNAME);
			(*TMPHANDLE)[0] = '\0'; /* null string */
		}
		else
		{
			thisSymh = (Symbolhandle) 0;
		}
	} /*if (STRING(thisSymh) == (char **) 0)*/

	return (thisSymh);
	
} /*copyFromClipboard()*/

#else /*!HASCLIPBOARD&&!HASSELECTION*/

/*
   Should always return a non-zero symbol, even if there is not enough
   memory to copy from the Clipboard (or selection buffer), in which case
   it returns an empty string

   thisSymh should be CLIPBOARDSYMBOL or SELECTIONSYMBOL
*/

static Symbolhandle copyFromClipboard(Symbolhandle thisSymh)
{
	char         **contents;
	WHERE("copyFromClipboard");
	
	/* make sure contents is null string, discardinging current contents */
	
	deleteClipboard(thisSymh);

	contents = STRING(thisSymh);
#ifndef HASSELECTION
	setSTRING(thisSymh, mygetscrap()); /* get whatever is on Clipboard */
#else /*HASSELECTION*/
	setSTRING(thisSymh,
			  (thisSymh == CLIPBOARDSYMBOL) ?
			  mygetscrap() : mygetselection());
#endif /*HASSELECTION*/
	if (STRING(thisSymh) != (char **) 0)
	{
		mydisphandle(contents);
	}
	else
	{
		setSTRING(thisSymh, contents); /*restore previous contents*/
	}
	
	return (thisSymh);
} /*copyFromClipboard()*/

#endif /*HASCLIPBOARD*/

#define CLIPFIELDSEP   '\t'

#ifdef MACINTOSH
#define CLIPLINESEP    13  /*\r*/
#else /*MACINTOSH*/
#define CLIPLINESEP    10  /*\n*/
#endif /*MACINTOSH*/

#define CLIPFORMAT     "%0.17g"

#define CLIPMISSING    "?"

/*
  When op is CHECKLEFTSPECIAL or CLEARSPECIAL, arg is the Special
  symbol in question.

  On CHECKLEFTSPECIAL, arg is saved as static thisSymh so that
  an immediately following call with op == ASSIGNSPECIAL knows
  which symbol is being assigned to.  A call with op ==
  ASSIGNSPECIAL is always preceded by a call with op ==
  CHECKLEFTSPECIAL.
  
  When op is CHECKRIGHTSPECIAL or ASSIGNSPECIAL, arg is the
  variable being assigned to the special symbol.
*/

static Symbolhandle copyToClipboard(Symbolhandle arg, long op)
{
	Symbolhandle        result = arg;
	static Symbolhandle thisSymh;
	char               *stringForMissing = CLIPMISSING;
	long                type;
	WHERE("copyToClipboard");
	
	switch (op)
	{
	  case CHECKLEFTSPECIAL:
		thisSymh = arg;
		/* always O.K. to assign to CLIPBOARD or SELECTION */
		break;
		
	  case CHECKRIGHTSPECIAL:
		/* check type of what is to be assigned to CLIPBOARD or SELECTION */
		type = TYPE(arg);
		if (type != REAL && type != LOGIC && type != CHAR && type != MACRO)
		{
			sprintf(OUTSTR,
					"ERROR: cannot assign variable of type %s to %s",
					typeName(type), NAME(thisSymh));
			putErrorOUTSTR();
			result = (Symbolhandle) 0;
		}
		break;
		
	  case CLEARSPECIAL:
		/* replace contents of CLIPBOARD or SELECTION with null string */
		deleteClipboard(arg);
		result = (STRING(arg) != (char **) 0) ?
			arg : (Symbolhandle) 0;
		break;
		
	  case ASSIGNSPECIAL:
		
		result = multipaste(arg, CLIPFORMAT, &stringForMissing,
							CLIPFIELDSEP, CLIPLINESEP);
		/* This should work even if arg is CLIPBOARDSYMBOL */

#ifdef MACINTOSH
		UNLOADSEG(multipaste);
#endif /*MACINTOSH*/

		/* delete current contents of thisSymh in any case */
		deleteClipboard(thisSymh);
		if (result != (Symbolhandle) 0)
		{
			/* transfer contents to thisSymh*/
			TMPHANDLE = STRING(result);
			setSTRING(result, (char **) 0);
			setSTRING(thisSymh, TMPHANDLE);
			setDIM(thisSymh, 1, 1);
			Removesymbol(result); /* remove from the symbol table and delete*/
			result = thisSymh;
#ifdef HASCLIPBOARD
			if (thisSymh == CLIPBOARDSYMBOL && !myputscrap(TMPHANDLE))
			{
				putOutErrorMsg("WARNING: unable to copy to Clipboard");
			}
#endif /*HASCLIPBOARD*/
#ifdef HASSELECTION
			if (thisSymh == SELECTIONSYMBOL && !myputselection(TMPHANDLE))
			{
				putOutErrorMsg("WARNING: unable to copy as Selection");
			}
#endif /*HASSELECTION*/
		} /*if (result != (Symbolhandle) 0)*/
		break;
		
	} /*switch (op)*/
	return (result);
	
} /*copyToClipboard()*/

/* only public function */
long iniClipboard(char * name)
{
	Symbolhandle     clipSymh;
#ifdef HASSELECTION
	int              clipboard = strcmp(name, CLIPBOARDNAME) == 0;

	clipSymh = (clipboard) ? CLIPBOARDSYMBOL : SELECTIONSYMBOL;
#else /*HASSELECTION*/
	clipSymh = CLIPBOARDSYMBOL;
#endif /*HASSELECTION*/

	if (clipSymh == (Symbolhandle) 0)
	{ /* only do something if clipSymh not initialized */
		clipSymh = Makespecialsymbol(CHAR, name,
									   copyToClipboard, copyFromClipboard);
#ifndef HASSELECTION
		CLIPBOARDSYMBOL = clipSymh;
#else /*HASSELECTION*/
		if (clipboard)
		{
			CLIPBOARDSYMBOL = clipSymh;
		}
		else
		{
			SELECTIONSYMBOL = clipSymh;
		}
#endif /*HASSELECTION*/
		if (clipSymh != (Symbolhandle) 0 &&
			SETSPECIAL((SpecialSymbolhandle) clipSymh)(clipSymh,
										 CLEARSPECIAL) != (Symbolhandle) 0)
		{
			setNDIMS(clipSymh, 1);
			setDIM(clipSymh, 1, 1);
			Addsymbol(clipSymh); /* add it to symbol table */
		}
	} /*if (clipSymh == (Symbolhandle) 0)*/
	
	return (clipSymh != (Symbolhandle) 0);
} /*iniClipboard()*/
