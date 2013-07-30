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
   950803 removed all references to very obsolete type MACRO1
   950922 added prototypes for all functions in file
   960307-10 Made modifications for dealing with Symbol labels
   960717 Changed types Long, Char, etc to long, char, etc.
   970124 Added check for SELECTION to Findspecial()
   970620 Made changes so that <<expression>> is legal (uses mvEval())
   971104 Added checks for value of myhandlelength()
   980621 Added function Setdims()
   980723 Added functions setNotes(), setLabels(), clearNotes(), clearLabels
   980729 Added argument to reuseArg()
*/


#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment Symbol
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


/*
   This file contains all the functions for working with symbols, the
   the fundamental objects manipulated by the user.

   It maintains the symbol table(s), creates, deletes, and copies symbols

*/
#include "globals.h"
#include "mainpars.h"

#undef USEDUMPSYMBOLS

/* Prototypes of entry points*/
long            Symbolinit(void);
Symbolhandle    Firstsymbol(int /*scratch*/);
Symbolhandle    Lastsymbol(void);
Symbolhandle    Nextsymbol(Symbolhandle /*symh*/, int /*scratch*/);
Symbolhandle    Prevsymbol(Symbolhandle /*symh*/);
void            Addsymbol(Symbolhandle /*symh*/);
void            Cutsymbol(Symbolhandle /*symh*/);
void            Setname(Symbolhandle /*symh*/, char */*name*/);
Symbolhandle    Findspecial(char * /*name*/);
Symbolhandle    Lookup(char * /*symbolName*/);
Symbolhandle    Makesymbol(long /*type*/);
Symbolhandle    Makespecialsymbol(long /*type*/, char * /*name*/,
				Symbolhandle (* /*setSpecial*/)(Symbolhandle,long),
				Symbolhandle (* /*getSpecial*/)(Symbolhandle));
Symbolhandle    Makestruc(long /*ncomp*/);
Symbolhandle    Makereal(long /*length*/);
Symbolhandle    Makechar(long /*length*/);
Symbolhandle    Makelong(long /*length*/);
Symbolhandle    Install(char * /*name*/, long /*type*/);
Symbolhandle    RInstall(char * /*name*/, long /*length*/);
Symbolhandle    CInstall(char * /*name*/, long /*length*/);
Symbolhandle    GraphInstall(char * /*name*/);
Symbolhandle    LongInstall(char * /*name*/, long /*length*/);
Symbolhandle    GarbInstall(long /*length*/);
Symbolhandle    StrucInstall(char * /*name*/, long /*ncomp*/);
Symbolhandle    RSInstall(char * /*name*/, long /*ncomp*/,
						  char * /*compNames*/[], long /*length*/);
Symbolhandle    RSInstall2(char * /*name*/, long /*ncomp*/,
						   char ** /*compNames*/, long /*length*/);
Symbolhandle    RSInstall3(char * /*name*/, long /*ncomp*/,
						   char ** /*compNames*/, long /*ncomp2*/,
						   char * /*compNames2*/[], long /*length*/);
void            Remove(char * /*name*/);
void            Removesymbol(Symbolhandle /*symh*/);
void            Removelist(Symbolhandle /*list*/);
void            Delete(Symbolhandle /*symh*/);
void            DeleteContents(Symbolhandle /*symh*/);
void            Unscratch(void);
void            CleanupList(Symbolhandle /*list*/);
void            RemoveUndef(void);
void            cleanAssignedStack(void);
long            isPendingAssign(Symbolhandle /*arg*/);
Symbolhandle    Assign(Symbolhandle /*arg1*/, Symbolhandle /*arg2*/,
					   long /*keyword*/);
int             Copy(Symbolhandle /*arg1*/, Symbolhandle /*arg2*/);
int             CopyDoubleToLong(Symbolhandle /*from*/, Symbolhandle /*to*/);
int             CopyLongToDouble(Symbolhandle /*from*/, Symbolhandle /*to*/);
Symbolhandle    reuseArg(Symbolhandle /*list*/, long /*argno*/,
                       int /*keeplabels*/, int /*keepnotes*/);
Symbolhandle    Extract(Symbolhandle /*strarg*/, Symbolhandle /*namearg*/,
						long /*named*/);
Symbolhandle    Makelist(void);
Symbolhandle    Addlist(Symbolhandle /*list*/, Symbolhandle /*arg*/);
Symbolhandle    Growlist(Symbolhandle /*mylist*/);
Symbolhandle    Byname(Symbolhandle /*symh*/, long * /*byNameType*/,
					   long * /*parseMacroArgs*/);
#ifdef USEDUMPSYMBOLS
void            Dumpsymbols(int /*scratch*/, char * /*msgs*/, long /*count*/);
#endif /*USEDUMPSYMBOLS*/
Symbolhandle    Buildlist(long /*nargs*/, long /*typeList*/[],
						  char * /*valueList*/[], char * /*keyList*/[]);
long            SizeofSymbol(Symbolhandle symh);

/* Prototypes of static functions */
static void     zeroSymbol(Symbolhandle /*symh*/);
static int      symbolHash(char * /*name*/);
static int      pushAssignedSymbol(Symbolhandle /*symh*/);
static void     popAssignedSymbol(void);
static Symbolhandle literalToSymbol(char * /*string*/, long * /*byNameType*/);

/* list of entry points */
#if (0)
long            Symbolinit(); /* initialize symbol tables */
/* functions for searching symbol tables*/
Symbolhandle    Lookup();  /* search symbol table for named symbol*/
Symbolhandle    Firstsymbol(); /* find first symbol in a symbol table*/
Symbolhandle    Lastsymbol(); /* find last symbol in a symbol table*/
Symbolhandle    Nextsymbol(); /* find the next symbol in a symbol table*/
Symbolhandle    Prevsymbol();/* find the previous symbol in a symbol table*/
Symbolhandle    Findspecial(); /*locate special symbol*/
/* Functions to allocate storage for various types of symbols*/
Symbolhandle    Makesymbol(); /* allocate storage for new symbol */
Symbolhandle    Makespecialsymbol() /*create special symbol like CLIPBOARD*/
Symbolhandle    Makestruc(); /* allocate storage for STRUC */
Symbolhandle    Makereal(); /* allocate storage for REAL vector */
Symbolhandle    Makelong(); /* allocate storage for LONG scratch vector */
Symbolhandle    Makechar(); /* allocated storage for CHARACTER vector */

/* functions for maintaining symbol table*/
void            Addsymbol(); /* add symbol to symbol table */
void            Cutsymbol(); /* remove symbol from symbol table */

/* Functions to create symbol and install it in the symbol table */
Symbolhandle    Install(); /*make symbol and add it to table */
Symbolhandle    RInstall(); /*make REAL symbol and add it to table */
Symbolhandle    LongInstall(); /*make LONG symbol and add it to table */
Symbolhandle    GarbInstall(); /*make GARB symbol and add it to table */
Symbolhandle    GraphInstall(); /*make PLOTINFO symbol and add it to table */
Symbolhandle    StrucInstall(); /*make STRUC symbol and add it to table */
Symbolhandle    RSInstall(); /*make STRUC w/REAL comps and add it to table */
Symbolhandle    RSInstall2();/*make STRUC w/REAL comps, all same size, add it*/
Symbolhandle    RSInstall3();/*make STRUCT w/STRUC comps, each with same
                               number of same length REAL comps*/

/* Functions involved in destroying symbols */
void            Remove(); /* remove named symbol from table and delete it */
void            Removesymbol(); /* remove symbold from table and delete it */
void            Delete(); /* deallocate storage associated with symbol */
void            DeleteContents(); /* delete contents of a symbol */
void            Removelist(); /* remove list from table after cleaning up */
void            Unscratch(); /* Remove all SCRATCH and keyword variables */
void            RemoveKeywd(); /* Remove all keyword variables */
void            RemoveUndef(); /* remove all variables with TYPE == UNDEF */
void            CleanupList(); /* remove SCRATCH & keyword vars from list */

/* Functions for building lists of arguments and subscripts*/
Symbolhandle    Makelist();/*start new LIST symbol for args or subscripts*/
Symbolhandle    Addlist();/*insert symbol at end of LIST of args, subscripts*/
Symbolhandle    Growlist();/*increase size of LIST of args or subscripts*/

/* Miscellaneous functions */
void            Setname(); /* set name of symbol */
int             Copy(); /* duplicate symbol */
int             CopyDoubleToLong(); /*create LONG symbol from REAL*/
int             CopyLongToDouble(); /*coerce LONG symbol to REAL*/
Symbolhandle    reuseArg(); /* reuse element in argument list */

/* Functions implementing features of language*/
void            cleanAssignedStack();/*clear list of assigned symbols*/
long            isPendingAssign(); /*check if symbol has appeared to left
									 of <- */
Symbolhandle    Assign(); /* assignment & subscript assignment */
Symbolhandle    Extract(); /* extract component of structure */
Symbolhandle    Byname(); /*decode "byname" expression*/

/* Other functions */
Symbolhandle    Buildlist(); /* for creating an argument list to execute a
								commands without going through parser*/
long            SizeofSymbol(); /*compute number of elements in symbol*/

#endif

/* start of linked list of scratch, keyword, and undefined symbols */
Symbolhandle    Scratchstart = (Symbolhandle) 0;

static char          *Name__;
#define inScratchTable(symh) \
	(Name__ = NAME(symh),\
	 (Name__[0] == TEMPPREFIX && \
	  (Name__[1] == KEYPREFIX2 || Name__[1] == SCRATCHPREFIX2))\
	 || TYPE(symh) == UNDEF || ISSHORT(symh) && TYPE(symh) != BLTIN )

#define TABLESIZE     512
#define TABLEMASK     511

static Symbolhandle  *Symboltable;
static long           Lastplace = -1;

/*
   function to zero the handle to the contents and all the dimension
   information and set initialize NCLASS
*/

static void zeroSymbol(Symbolhandle symh)
{
	WHERE("zeroSymbol");
	if (symh != (Symbolhandle) 0)
	{
		long     type = TYPE(symh);
		int      maxdims = (type == BLTIN || ISSHORT(symh)) ? 1 : MAXDIMS;

		setNCLASS(symh,-1);		/* a variate by default */

#ifdef ONETYPEHANDLE			/* all **var always has same form */
		setDATA(symh,(double **) 0);
#else  /*ONETYPEHANDLE*/
		if(type == REAL || type == LOGIC)
		{
			setDATA(symh,(double **) 0);
		}
		else if(type == CHAR || type == MACRO || type == UNDEF ||
				type == ASSIGNED || type == NULLSYM)
		{
			setSTRING(symh,(char **) 0);
		}
		else if(type == STRUC || type == LIST)
		{
			setCOMP(symh,(Symbolhandle**) 0);
		}
		else if(type == GARB)
		{
			setGARBAGE(symh,(char ****) 0);
		}
		else if(type == BLTIN)
		{
			setFPTR(symh,(Symbol **(*) ()) 0);
		}
		else if(type == PLOTINFO)
		{
			setGRAPH(symh,(whole_graph **) 0);
		}
#endif /*ONETYPEHANDLE*/

		setLABELS(symh, (LabelsHandle) 0);
		setNOTES(symh, (NotesHandle) 0);

#if (USENOMISSING)
		clearNOMISSING(symh);
#endif /*USENOMISSING*/

		/* Zero all dimension information */
#ifdef USEMEMSET
		memset((void *) DIM(symh) , 0, (maxdims+1)*sizeof(long));
#else  /*USEMEMSET*/
		{
			int    i;
			
			setNDIMS(symh,0);
			for (i = 1;i <= maxdims;i++)
			{
				setDIM(symh,i,0);
			}
		}
#endif /*USEMEMSET*/
	} /*if (symh != (Symbolhandle) 0)*/	
} /*zeroSymbol()*/

/*
  Compute hash code = sum of ascii codes in name mod TABLESIZE
*/
static int  symbolHash(char * name)
{
	register int     code = 0;
	register char   *pc = name;
	
	while (*pc != '\0')
	{
		code += *pc++;
	}
	return (code & TABLEMASK); /* code % TABLESIZE */
} /*symbolHash()*/

/*
  Allocate and initialize storage for not-scratch symbol table.  Each entry
  is a starting node for a doubly linked list of symbols with the same
  hash code.  Hence the symbol table is not limited in size.
*/

long Symbolinit(void)
{
	int        i;

	Symboltable = (Symbolhandle *) mygetpointer((TABLESIZE+1)*sizeof(Symbolhandle));
	if (Symboltable == (Symbolhandle *) 0)
	{
		return (0);
	}
	for (i = 0;i < TABLESIZE;i++)
	{
		Symboltable[i] = (Symbolhandle) 0;
	}
	Symboltable[TABLESIZE] = ENDPREV; /* non-zero to stop searches */

	Scratchstart = (Symbolhandle) 0; /* just for good measure */

	return (1);
} /*Symbolinit()*/

/* find start of scratch chain or chain nearest beginning of Symboltable[] */
Symbolhandle Firstsymbol(int scratch)
{
	register Symbolhandle *psymh;
	WHERE("Firstsymbol");

	if(scratch)
	{
		return Scratchstart;
	}

	for(psymh = Symboltable; *psymh == (Symbolhandle) 0;psymh++)
	{
		;
	}
	Lastplace = psymh - Symboltable;
/*
  Firstsymbol should never be called when table is empty; hence don't test
  for validity of psymh
*/
	return (*psymh);
} /*Firstsymbol()*/

/*
  Find the end of the chain starting nearest the end of Symboltable[]
*/
Symbolhandle Lastsymbol(void)
{
	register Symbolhandle *psymh;
	register Symbolhandle  last;
	
	for(psymh = Symboltable + TABLESIZE-1;*psymh == (Symbolhandle) 0;psymh--)
	{/* because of functions, a non-zero *psymh will always be found */
		;
	}

	for(last = *psymh;NEXT(last) != (Symbolhandle) 0; last = NEXT(last))
	{ /* find end of list pointed to by psymh */
		;
	}

	return (last);
} /*Lastsymbol()*/

/*
  Used only when traversing the symbol table.  If it is used otherwise, it
   would need to be changed because it is remembering the last position in
   Symboltable examined.  It's most important use is by RemoveUndef().
*/

Symbolhandle Nextsymbol(Symbolhandle symh, int scratch)
{
	register Symbolhandle     next = NEXT(symh);
	register Symbolhandle    *place;
	WHERE("Nextsymbol");
	
	if(next == (Symbolhandle) 0 && !scratch)
	{ /* symh is at end of chain */
		/* proceed along Symboltable looking for a chain */
		if(Lastplace < 0)
		{ /* just starting, should not normally occur */
			Lastplace = symbolHash(NAME(symh)) + 1;
		}
		else
		{
			Lastplace++;
		}
			
		for(place = Symboltable + Lastplace;*place == (Symbolhandle) 0;place++)
		{ /* guaranteed to stop because Symboltable[TABLESIZE] is ENDPREV */
			;
		}
		if((Lastplace = place - Symboltable) < TABLESIZE)
		{ /* found a symbol */
			next = *place;
		}
		else
		{ /* run out of symbols */
			next = (Symbolhandle) 0;
			Lastplace = -1;
		}
	} /*if(next == (Symbolhandle) 0 && !scratch)*/
	
	return (next);
} /*Nextsymbol()*/

/*
  Used by save() to traverse symbol table backward from end of Symboltable[]
*/

Symbolhandle Prevsymbol(Symbolhandle symh)
{
	register Symbolhandle     prev = PREV(symh);
	register int              place;
	int                       scratch;

	if(prev == ENDPREV)
	{ /* most recent added to chain */
		scratch = inScratchTable(symh);
		if(!scratch)
		{ /* Backdown Symboltable looking for a chain */
			for(place = symbolHash(NAME(symh)) - 1;
				place >= 0 && Symboltable[place] == (Symbolhandle) 0;
				place--)
			{
				;
			}
			if(place >= 0)
			{ /* found one; find the first element added */
				for(prev = Symboltable[place];NEXT(prev) != (Symbolhandle) 0;
					prev = NEXT(prev))
				{
					;
				}
			}
		} /*if(!scratch)*/
	} /*if(prev == ENDPREV)*/
	return (prev);
} /*Prevsymbol()*/

/* 
  Function to add symbol to symbol table.  If the symbol is scratch, UNDEF,
  or a keyword, it is added to the beginning of the scratch symbol table;
  otherwise it is always added at the beginning of the chain starting
  at Symboltable[symbolHash(NAME(symh))] in the regular symbol table.
  Note that the table to which it is added depends on the name.  Thus the
  name should always be set *before* adding a symbol.  This is particularly
  important after a call to Copy(), since Copy() copies the name.

  No storage is allocated.  Do nothing if symh is null.
*/

void    Addsymbol(Symbolhandle symh)
{
	int           scratch, place;
	Symbolhandle  start;
	WHERE("Addsymbol");
	
	if(symh != (Symbolhandle) 0)
	{
		if(NEXT(symh) == (Symbolhandle) 0 && PREV(symh) == (Symbolhandle) 0)
		{						/* add only if symh is not in symbol table */
			scratch = inScratchTable(symh);
			start = (scratch) ?
				Firstsymbol(1) : Symboltable[place = symbolHash(NAME(symh))];
			if (start != (Symbolhandle) 0)
			{					/* another symbol is in list */
				setPREV(start,symh);
			}
		
			setNEXT(symh,start);
			setPREV(symh,ENDPREV);
			if(scratch)
			{
				Scratchstart = symh;
				Nscratch++;
				Maxscratch = (Maxscratch >= Nscratch) ? Maxscratch : Nscratch;
			}
			else
			{
				Symboltable[place] = symh;
				Nsymbols++;
				Maxsymbols = (Maxsymbols >= Nsymbols) ? Maxsymbols : Nsymbols;
				if(TYPE(symh) == BLTIN)
				{
					Nfunctions++;
				}
			}
		} /*if(NEXT(symh)==(Symbolhandle) 0 && PREV(symh)==(Symbolhandle) 0)*/
		else
		{/* should not happen */
			putOutErrorMsg("ERROR: (Internal) attempt to add bad symbol");
		}
	} /*if(symh != (Symbolhandle) 0)*/
} /*Addsymbol()*/

/*
  Function to cut symbol out of symbol table; does not release any storage
  Do nothing if symh is null.
*/

void Cutsymbol(Symbolhandle symh)
{
	Symbolhandle        next, prev;
	int                 scratch;
	WHERE("Cutsymbol");
	
/*
  Note: NEXT(symh) should be zero only if symh was the first symbol put
  in its list.  PREV(symh) should be a valid address, except when symh was
  the most recently added symbol to its list.  If symh was the most recent
  symbol, PREV(symh) should be ENDPREV.
*/
	if(symh != (Symbolhandle) 0)
	{
		prev = PREV(symh);
		next = NEXT(symh);
		if(next != (Symbolhandle) 0 || prev != (Symbolhandle) 0)
		{						/* symbol actually in a symbol table */
			scratch = inScratchTable(symh);
			if (next != (Symbolhandle) 0) /* if there is a following symbol */
			{
				setPREV(next,prev);
			}
		
			if (prev != ENDPREV)
			{					/* if there is a preceding symbol */
				setNEXT(prev,next);
			}
			else if(scratch)
			{
				Scratchstart = next;
			}
			else
			{
				Symboltable[symbolHash(NAME(symh))] = next;
			}
			setPREV(symh,(Symbolhandle) 0);
			setNEXT(symh,(Symbolhandle) 0);
			if(scratch)
			{
				Nscratch--;
			}
			else
			{
				Nsymbols--;
			}
		} /*if(next != (Symbolhandle) 0 || prev != (Symbolhandle) 0)*/
	} /*if(symh != (Symbolhandle) 0)*/	
} /*Cutsymbol()*/

void Setdims(Symbolhandle symh, long ndims, long dims [])
{
	int        i;
	int        maxdims = (ISSHORT(symh)) ? 1 : MAXDIMS;

	setNDIMS(symh, (ISSHORT(symh)) ? 1 : ndims);
	for (i = 1; i <= maxdims; i++)
	{
		if (i <= ndims)
		{
			setDIM(symh, i, dims[i-1]);
		}
		else
		{
			setDIM(symh, i, 0);
		}
	} /*for (i = 1; i <= maxdims; i++)*/
} /*Setdims()*/
	
void Setname(Symbolhandle symh, char * name)
{
	strncpy(NAME(symh),name,NAMELENGTH);
	NAME(symh)[NAMELENGTH] = '\0';
} /*Setname()*/

#ifndef LABELSARECHAR
void setLabelsDim(Symbolhandle symh)
{
	if (HASLABELS(symh))
	{
		long       nlabels = 0, ndims = NDIMS(symh), i;

		setNDIMS(LABELS(symh), 1);
		for (i = 1; i <= ndims; i++)
		{
			nlabels += DIMVAL(symh, i);
		}
		setDIM(LABELS(symh), 1, nlabels);
	}
} /*setLabelsDim()*/
#endif /*LABELSARECHAR*/

#ifndef NOTESARECHAR
void setNotesDim(Symbolhandle symh)
{
	if (HASNOTES(symh))
	{
		long        nlines = 0;
		long        notesLength;
		char       *notes = NOTESPTR(symh);
		char       *place = notes;
		
		notesLength = myhandlelength(NOTESHANDLE(symh));
		
		while (place - notes < notesLength)
		{
			place = skipStrings(place, 1);
			nlines++;
		}
		
		if (place - notes > notesLength)
		{ /* be paranoid*/
			notes[notesLength - 1] = '\0';
			nlines++;
		}
		setNDIMS(NOTES(symh), 1);
		setDIM(NOTES(symh), 1, nlines);
	} /*if (HASNOTES(symh))*/
} /*setNotesDim()*/
#endif /*NOTESARECHAR*/

int setLabels(Symbolhandle symh, char ** labels)
{
	WHERE("setLabels");

	if (HASLABELS(symh))
	{
		/* discard contents of existing labels */
		mydisphandle(LABELSHANDLE(symh)); 
		setLABELSHANDLE(symh, (char **) 0);
	}
#ifndef LABELSARECHAR
	else
	{/* if symh has no labels, create short Symbol For them*/
		LabelsHandle        shortSymh = (LabelsHandle) Makesymbol(SHORTCHAR);
		
		setLABELS(symh, shortSymh);
		if (shortSymh == (LabelsHandle) 0)
		{
			return (0);
		}
		Setname((Symbolhandle) shortSymh, LABELSSCRATCH);
	} /*if (!HASLABELS(symh))*/
	
	setLABELSHANDLE(symh, labels);
	setLabelsDim(symh);
#else /*LABELSARECHAR*/
	setLABELS(symh, (LabelsHandle) labels);
#endif /*LABELSARECHAR*/
	return (1);
} /*setLabels()*/

int setNotes(Symbolhandle symh, char ** notes)
{
	if (HASNOTES(symh))
	{
		mydisphandle(NOTESHANDLE(symh)); 
	}

#ifdef NOTESARECHAR
	NOTES(symh) = (NotesHandle) notes;
#else /*NOTESARECHAR*/
	if (!HASNOTES(symh))
	{
		NotesHandle        shortSymh = (NotesHandle) Makesymbol(SHORTCHAR);

		setNOTES(symh, shortSymh);
		if (NOTES(symh) == (NotesHandle) 0)
		{
			return (0);
		}
		Setname((Symbolhandle) shortSymh, NOTESSCRATCH);
	} /*if (!HASNOTES(symh))*/
	setNOTESHANDLE(symh, notes);
	setNotesDim(symh);
#endif /*NOTESARECHAR*/
	return (1);
} /*setNotes()*/

void    clearLabels(Symbolhandle symh)
{
	if (HASLABELS(symh))
	{
#ifdef LABELSARECHAR
		mydisphandle((char **) LABELS(symh));
#else /*LABELSARECHAR*/
		Delete((Symbolhandle) LABELS(symh));
#endif /*LABELSARECHAR*/
		LABELS(symh) = (LabelsHandle) 0;
	}
} /*clearLabels()*/

void    clearNotes(Symbolhandle symh)
{
	if (HASNOTES(symh))
	{
#ifdef NOTESARCHAR
		mydisphandle((char **) NOTES(symh));
#else /*NOTESARECHAR*/
		Delete((Symbolhandle) NOTES(symh));
#endif /*NOTESARECHAR*/
		setNOTES(symh, (NotesHandle) 0);
	}
} /*clearNotes()*/

/*
   Is name the name of a special symbol?  (only ones at present are CLIPBOARD
   and, under Motif, SELECTION)
   If so, return the symbol; if not return (Symbolhandle) 0
   Called by Lookup() and yylex1()

   Names of special symbols will normally be in the regular symbol table
   but need not be.  CLIPBOARD is in the regular symbol table and cannot be
   deleted (delete(CLIPBOARD) clears its contents).  Same for SELECTION.
   970124 Added check for SELECTION when HASSELECTION is defined (Motif)
*/
Symbolhandle Findspecial(char *name)
{
	Symbolhandle     symh = (Symbolhandle) 0;
	
	/* CLIPBOARD is in the symbol table and will be found by Lookup()*/
	if (name[0] == CLIPBOARDNAME[0] && strcmp(name,CLIPBOARDNAME) == 0)
	{
		symh = CLIPBOARDSYMBOL;
	} /*if(firstchar==CLIPBOARDNAME[0]&&strcmp(symbolName,CLIPBOARDNAME)==0)*/
#ifdef HASSELECTION
	else if(name[0] == SELECTIONNAME[0] && strcmp(name,SELECTIONNAME) == 0)
	{
		symh = SELECTIONSYMBOL;
	} /*if(firstchar==SELECTIONNAME[0]&&strcmp(symbolName,SELECTIONNAME)==0)*/
#endif /*HASSELECTION*/
	return (symh);
} /*Findspecial()*/

/*
  Search the regular symbol table table for first occurence of name symbolName
*/
Symbolhandle    Lookup(char *symbolName)
{
	int                      place = symbolHash(symbolName);
	register Symbolhandle    symh;
	register char            firstchar = symbolName[0], *name;
	WHERE("Lookup");
	
	/* search regular symbol table only */
	for (symh = Symboltable[place]; symh != (Symbolhandle) 0;
		 symh = NEXT(symh))
	{
		if (firstchar == *(name = NAME(symh)) && strcmp(name, symbolName) == 0)
		{
			break;
		}
	}

	if (symh == (Symbolhandle) 0)
	{
		/* not found; maybe it's a special symbol not in table */
		symh = Findspecial(symbolName);
	}
	
	if (isSpecial(symh))
	{ /* process it */
		symh = GETSPECIAL((SpecialSymbolhandle) symh)(symh);
	}
	return (symh);
	
} /*Lookup()*/

/*
  routine to create a new Symbol, not yet in symbol table
*/

Symbolhandle    Makesymbol(long type)
{
	Symbolhandle    symh;
	long            thisSize;
	WHERE("Makesymbol");
	
	if (type == BLTIN)
	{
		thisSize = sizeof(FunctionSymbol);
	}
	else if (type & SHORTSYMBOL)
	{
		thisSize = sizeof(ShortSymbol);
	}
	else
	{
		thisSize = sizeof(Symbol);
	}
	
	symh = (Symbolhandle) mygethandle(thisSize);
	
	if (symh != (Symbolhandle) 0)
	{
		setTYPE(symh, type);

		zeroSymbol(symh);

		if (type == NULLSYM)
		{
			setNAME(symh, NULLSCRATCH);
			setNDIMS(symh,1);
			setDIM(symh, 1, 0);
		}
		else
		{
			setNAME(symh, SCRATCH);
		}
		setPREV(symh,(Symbolhandle) 0);
		setNEXT(symh,(Symbolhandle) 0);
	} /*if (symh != (Symbolhandle) 0)*/
	return (symh);
} /*Makesymbol()*/

/*
  routine to create a new SpecialSymbol, not yet in symbol table
*/

Symbolhandle Makespecialsymbol(long type, char *name,
							   Symbolhandle (*setSpecial)(Symbolhandle,long),
							   Symbolhandle (*getSpecial)(Symbolhandle))
{
	Symbolhandle           symh;
	SpecialSymbolhandle    specialSymh;
	WHERE("Makespecialsymbol");
	
	specialSymh = (SpecialSymbolhandle) mygethandle(sizeof(SpecialSymbol));
	symh = (Symbolhandle) specialSymh;
	
	if (symh != (Symbolhandle) 0)
	{
		setSPECIALTYPE(symh, type);
		zeroSymbol(symh);
		setPREV(symh,(Symbolhandle) 0);
		setNEXT(symh,(Symbolhandle) 0);
		setNAME(symh, name);
		setSETSPECIAL(specialSymh, setSpecial);
		setGETSPECIAL(specialSymh, getSpecial);
	} /*if (specialSymh != (SpecialSymbolhandle) 0)*/
	return (symh);
} /*Makespecialsymbol()*/

/*
  Make a structure with ncomp components, initializing all component
  handles to 0 and not installing in symbol table
*/

Symbolhandle    Makestruc(long ncomp)
{
	/* make a structure with ncomp components, not in symbol table */
	Symbolhandle       result;
	long               i;
	WHERE("Makestruc");
	
	result = Makesymbol(STRUC);
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	TMPHANDLE = mygethandle(ncomp * sizeof(Symbolhandle));
	if (TMPHANDLE == (char **) 0)
	{
		goto errorExit;
	}
	setCOMP(result,(Symbolhandle**) TMPHANDLE);

	for (i = 0; i < ncomp; i++)
	{
		COMPVALUE(result,i) = (Symbolhandle) 0;
	}
	setNDIMS(result,1);
	setNCOMPS(result,ncomp);

	return (result);

  errorExit:
	Delete(result);
	return (0);

} /*Makestruc()*/

/*
  Make real vector, not in symbol table

  Note: Makereal(0) differs from Makesymbol(REAL) in setting NDIMS(result) 
  to 1
*/

Symbolhandle    Makereal(long length)	
{
	Symbolhandle    result;
	WHERE("Makereal");
	
	result = Makesymbol(REAL);
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	setNDIMS(result,1);
	setDIM(result,1,length);
	if (length > 0)
	{
		TMPHANDLE = mygethandle(length * sizeof(double));
		if (TMPHANDLE == (char **) 0)
		{
			goto errorExit;
		}
		setDATA(result,(double **) TMPHANDLE);
	}
	return (result);

  errorExit:
	Delete (result);
	return (0);
} /*Makereal()*/

Symbolhandle    Makechar(long length)	
{
	/* make CHAR scalar, not in symbol table */
	Symbolhandle    result;
	WHERE("Makechar");
	
	result = Makesymbol(CHAR);
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	setNDIMS(result,1);
	setDIM(result,1,(length > 0) ? 1 : 0);
	if (length > 0)
	{
		TMPHANDLE = mygethandle(length);
		if (TMPHANDLE == (char **) 0)
		{
			goto errorExit;
		}
		setSTRING(result,TMPHANDLE);
		(*TMPHANDLE)[0] = '\0';
	}
	return (result);

  errorExit:
	Delete (result);
	return (0);
} /*Makechar()*/

Symbolhandle    Makelong(long length)
{
	/* make long vector, not in symbol table */
	Symbolhandle    result;
	WHERE("Makelong");
	
	result = Makesymbol(LONG);
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	setNDIMS(result,1);
	setDIM(result,1,length);
	if (length > 0)
	{
		TMPHANDLE = mygethandle(length * sizeof(long));
		if (TMPHANDLE == (char **) 0)
		{
			goto errorExit;
		}
		setLONGDATA(result,(long **) TMPHANDLE);
	}
	return (result);

  errorExit:
	Delete (result);
	return (0);
} /*Makelong()*/

Symbolhandle    Install(char * name, long type)
{
	/* routine to put new symbol in table */

	Symbolhandle    symh;
	char            tempname[NAMELENGTH+1];
	WHERE("Install");

	strncpy(tempname, name, NAMELENGTH);
	tempname[NAMELENGTH] = '\0';
	symh = Makesymbol(type);
	if(symh != (Symbolhandle) 0)
	{
		setNAME(symh, tempname);
		Addsymbol(symh);
	}
	return (symh);
} /*Install()*/

/*
   Install REAL symbol in symbol table.
   If length > 0, it will be a vector with length elements
   If length == 0, the DATA handle will be (double **) 0
   It can also be used to install a LOGICAL symbol if the type
   is changed after installation.
*/

Symbolhandle    RInstall(char * name, long length)
{
	Symbolhandle    symh;
	WHERE("RInstall");

	symh = Makereal(length);
	if (symh != (Symbolhandle) 0)
	{
		setNAME(symh,name);
		Addsymbol(symh);
	}	
	return (symh);
} /*RInstall()*/

/*
   Install CHAR symbol in symbol table.
   If length > 0, it will be a scalar with room for a string with
   length characters, including the terminating '\0'
   If length == 0, the STRING handle will be (char **) 0
*/

Symbolhandle    CInstall(char * name, long length)
{
	Symbolhandle    symh;
	WHERE("CInstall");

	symh = Makechar(length);
	if (symh != (Symbolhandle) 0)
	{
		setNAME(symh,name);
		Addsymbol(symh);
	}	
	return (symh);
} /*CInstall()*/

/*
   Install a symbol of type PLOTINFO (GRAPH)
*/
Symbolhandle    GraphInstall(char * name)
{
	Symbolhandle            symh;
	whole_graph           **graph = (whole_graph **) 0;
	WHERE("GraphInstall");

	symh = Makesymbol(PLOTINFO);
	if (symh == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	setNAME(symh,name);
	Addsymbol(symh);
	graph = (whole_graph **) mygethandle(sizeof(whole_graph));
	if(graph == (whole_graph **) 0)
	{
		goto errorExit;
	}
	GRAPHNPLOTS(graph) = 0;
	GRAPHFIRST_PLOT(graph) = (curve_points **) 0;
	GRAPHNSTRINGS(graph) = 0;
	GRAPHFIRST_STRING(graph) = (plot_string **) 0;
	GRAPHNXTICKS(graph) = GRAPHNYTICKS(graph) = 1;
	GRAPHXTICKLENGTH(graph) = DEFAULTXTICKLENGTH;
	GRAPHYTICKLENGTH(graph) = DEFAULTYTICKLENGTH;
	GRAPHXTICKS(graph) = GRAPHYTICKS(graph) = (double **) 0;
	GRAPHBGCOLOR(graph) = DEFAULTBGCOLOR;
	GRAPHFGCOLOR(graph) = DEFAULTFGCOLOR;
	GRAPHFRAMECOLOR(graph) = DEFAULTFRAMECOLOR;

	setGRAPH(symh,graph);
	setNDIMS(symh,1);
	setDIM(symh,1,1);

	return (symh);

  errorExit:
	Removesymbol(symh);
	return ((Symbolhandle) 0);
} /*GraphInstall()*/

/*
   Install a LONG symbol with length units available
*/
Symbolhandle    LongInstall(char * name, long length)
{

	Symbolhandle    symh;
	WHERE("LongInstall");

	symh = Makelong(length);
	if (symh != (Symbolhandle) 0)
	{
		setNAME(symh,name);
		Addsymbol(symh);
	}	
	return (symh);
} /*LongInstall()*/

/*
   Install a symbol of type GARB (used as a container for scratch
   variables to avoid memory leaks) with room for length handles.
   A GARB symbol is similar to a structure except is components are char **
   and not Symbolhandle
*/
Symbolhandle    GarbInstall(long length)
{
	/* install a GARB symbol with room for length char **'s  */

	Symbolhandle    symh;
	long            i;
	WHERE("GarbInstall");

	symh = Install(GARBSCRATCH,GARB);
	if (symh == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	setNDIMS(symh,1);
	setDIM(symh,1,length);

	if (length > 0)
	{
		TMPHANDLE = mygethandle(length * sizeof(char **));
		setGARBAGE(symh,(char ****) TMPHANDLE);
		if (TMPHANDLE == (char **) 0)
		{
			goto errorExit;
		}
		for(i=0;i<length;i++)
		{
			GARBAGEVALUE(symh,i) = (char **) 0;
		}
	} /*if (length > 0)*/
	
	return (symh);

  errorExit:
	Removesymbol (symh);
	return (0);
} /*GarbInstall()*/

/*
   Install a structure with ncomp components
*/
Symbolhandle StrucInstall(char * name, long ncomp)
{
	Symbolhandle      result;
	
	result = Makestruc(ncomp);
	if(result != (Symbolhandle) 0)
	{
		setNAME(result,name);
		Addsymbol(result);
	}
	return (result);
} /*StrucInstall()*/

/*
   Install a structure with ncomp REAL components whose names are
   in compNames, an array of character pointers; all components to be
   real vectors of the same size length, possible 0
*/

Symbolhandle    RSInstall(char * name, long ncomp, char * compNames[],
						  long length)
{
	Symbolhandle    result, symh;
	long            i;

	result = StrucInstall(name,ncomp);
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	for (i = 0; i < ncomp; i++)
	{
		symh = COMPVALUE(result, i) = Makereal(length);
		if (symh == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		if(compNames != (char **) 0)
		{
			setNAME(symh, compNames[i]);
		}
	}
	return (result);

  errorExit:
	Removesymbol (result);
	return (0);
	
} /*RSInstall()*/

/*
   Install a structure with ncomp REAL components whose names are
   in compNames, a char **, probably of the form STRING(symh).
   all components to be real vectors of the same size length, possibly 0
*/

/*  install a REAL structure with ncomp components
	component names in compNames, a handle to CHAR data.
	real vectors all with length length  (if zero, no assignment )
*/
Symbolhandle    RSInstall2(char * name, long ncomp, char ** compNames,
						   long length)
{
	Symbolhandle    result, symh;
	long            i;
	char           *names;
	
	result = StrucInstall(name,ncomp);
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	for (i = 0; i < ncomp; i++)
	{
		symh = COMPVALUE(result, i) = Makereal(length);
		if (symh == (Symbolhandle) 0)
		{
			goto errorExit;
		}
	} /*for (i = 0; i < ncomp; i++)*/

	names = *compNames;
	
	for (i = 0; i < ncomp; i++)
	{
		setNAME(COMPVALUE(result, i), names);
		names = skipStrings(names, 1);
	}

	return (result);

  errorExit:
	Removesymbol(result);
	return (0);
	
} /*RSInstall2()*/


/*  
   Install a structure with ncomp components each of which is a structure
   with ncomp2 REAL components of size length and names taken from compNames2
   The top level names are taken from *compNames, 
*/

Symbolhandle    RSInstall3(char * name, long ncomp, char ** compNames,
						   long ncomp2, char * compNames2[], long length)
{
	Symbolhandle    result, symh;
	long            i;
	char           *names;
	WHERE("RSInstall3");

	result = StrucInstall(name,ncomp);

	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	for (i = 0; i < ncomp; i++)
	{
		symh = RSInstall(SCRATCH,ncomp2,compNames2,length);
		if(symh == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		Cutsymbol(symh); /* remove from symbol table */
		COMPVALUE(result,i) = symh;
	} /*for (i = 0; i < ncomp; i++)*/

	if (compNames != (char **) 0)
	{
		names = *compNames;
		
		for (i = 0; i < ncomp; i++)
		{
			setNAME(COMPVALUE(result, i), names);
			names = skipStrings(names, 1);
		}
	} /*if (compNames != (char **) 0)*/
	
	return (result);

  errorExit:
	Removesymbol(result);
	return (0);
} /*RSInstall3()*/


/*
  Remove() is used only by glmcleanup() in glm.c and in
  boxplot.c, myplot.c and restore.c.  Elsewhere symbols in the
  symbol table are removed by Removesymbol(symh)
*/

/* Find entry in symbol table and remove it */

void Remove(char * name)
{
	Removesymbol(Lookup(name));
} /*Remove()*/

/* Remove symbol from symbol table or clear special symbols such as CLIPBOARD*/
void Removesymbol(Symbolhandle symh)
{
	if (isSpecial(symh))
	{
		/*
		   special treatment for special symbols
		   For CLIPBOARD, just the contents are cleared, but the
		   symbol survives.
		*/
		(void) SETSPECIAL((SpecialSymbolhandle) symh)(symh, CLEARSPECIAL);
	}
	else if (symh != (Symbolhandle) 0 && TYPE(symh) != BLTIN &&
			 symh != NULLSYMBOL)
	{/* don't remove permanent things */
		/* fix up table pointers */
		Cutsymbol(symh);
		/* free up storage (works recursively for structures) */
		Delete(symh);
	}
} /*Removesymbol()*/

/* clean up argument list and remove it */

void Removelist(Symbolhandle list)
{
	WHERE("Removelist");

	CleanupList(list); /* delete SCRATCH and keyword entries  */
	Removesymbol(list);
} /*Removelist()*/

/* 
  Get rid of a symbol handle but does not remove from symbol table if there;
  hence Cutsymbol() should always be called before Delete() if symh is in the
  symbol table.  In fact, that is what Removesymbol() does.
  Note that NULLSYMBOL cannot be deleted
 */
void Delete(Symbolhandle symh)
{
	WHERE("Delete");

	if (symh != (Symbolhandle) 0 && TYPE(symh) != BLTIN &&
		 symh != NULLSYMBOL)
	{
		DeleteContents(symh);
	
		if(!isSpecial(symh) && !isFakeSymbol(symh))
		{						/* not fake Symbol handle */
			mydisphandle((char **) symh);
		}
	}	
} /*Delete()*/

/*
  Get rid of a contents of symbol but do not delete symbol
  The name is changed to SCRATCH and the type is changed to UNDEF; hence
  symh should be removed from the symbol table before calling DeleteContents
*/
void DeleteContents(Symbolhandle symh)
{
	Symbolhandle  **components;
	char        ****trash;
	void          **handle = (void **) 0;
	long            i, ncomps;
	long            type;
	WHERE("DeleteContents");

	if (symh != (Symbolhandle) 0 && (type = TYPE(symh)) != BLTIN)
	{
		clearLabels(symh);
		clearNotes(symh);
		
		switch (type)
		{
		  case LOGIC:
		  case REAL:
			handle = (void **) DATA(symh);
			break;

		  case LONG:
			handle = (void **) LONGDATA(symh);
			break;

		  case LIST:
			/* 
			  DeleteContents(list) should be called only following
			  CleanupList(list).  This is done automatically if list is
			  removed by Removelist().

			  CleanupList removes scratch symbols, keyword symbols, and
			  undefined symbols that are in the list but ignores other
			  symbols.  All DeleteContents does is dispose of the list handle
			  itself.  This can be contrasted with what happens with STRUCs,
			  where all components are disposed of. 
			  */
			handle = (void **) COMP(symh);
			break;

		  case GARB:
			handle = (void **) GARBAGE(symh);
			if (handle != (void **) 0)
			{
				trash = (char ****) handle;
				ncomps = DIMVAL(symh,1);
				for (i = 0; i < ncomps; i++)
				{
					mydisphandle((*trash)[i]);
				}				/* delete all handles */
			}
			break;
			
		  case CHAR:
		  case MACRO:
			handle = (void **) STRING(symh);
			break;

		  case STRUC:
			handle = (void **) COMP(symh);
			if (handle != (void **) 0)
			{
				components = (Symbolhandle **) handle;
				ncomps = NCOMPS(symh);
				for (i = 0; i < ncomps; i++)
				{
					Delete((*components)[i]);
				}				/* delete all subcomponents */
			}
			break;

		  case PLOTINFO:
			handle = (void **) GRAPH(symh);
			if(handle != (void **) 0)
			{
				deleteGraph((whole_graph **) handle); /* in plotutil.c */
				/* Note: deleteGraph disposes of its argument */
				handle = (void **) 0;
			}
			break;

		  case UNDEF:
		  case ASSIGNED:
		  case NULLSYM:
			break;

		  default:
			sprintf(OUTSTR,
					"WARNING: (internal) deleting variable %s of unknown type %ld",
					NAME(symh), type);
			putOUTSTR();
		} /*switch (type)*/
		mydisphandle((char **) handle);
		if (isSpecial(symh))
		{
			setSPECIALTYPE(symh, UNDEF);
		}
		else
		{
			setTYPE(symh, UNDEF | ((ISSHORT(symh)) ? SHORTSYMBOL : 0));
		}

		zeroSymbol(symh);
		setNAME(symh,SCRATCH);
	} /*if (symh != (Symbolhandle) 0 && (type = TYPE(symh)) != BLTIN)*/
} /*DeleteContents()*/
	
void Unscratch(void)
{
	/* remove all SCRATCH variables & keyword variables from symbol list */
	Symbolhandle       symh, next;
	WHERE("Unscratch");
	
	for (symh = Scratchstart; symh != (Symbolhandle) 0; symh = next)
	{ /* clear out entire scratch list */
		next = NEXT(symh);
		Removesymbol(symh);
	}
} /*Unscratch()*/

/*
  Remove all keyword & SCRATCH temporary variables in argument list
  To be called by mainpars on return from a function, to avoid buildup of
  unused symbols in symbol table during looping
  Note: Components of a LIST are either in the regular symbol table or the
  scratch symbol table.  This is one difference from a STRUC.
*/

void CleanupList(Symbolhandle list)
{
	long              i, ncomps;
	Symbolhandle      arg;
	WHERE("CleanupList");

	if(list != (Symbolhandle) 0 && TYPE(list) == LIST)
	{
		ncomps = NCOMPS(list);
		for(i=0;i<ncomps;i++)
		{
			if((arg = COMPVALUE(list,i)) != (Symbolhandle) 0 &&
			   inScratchTable(arg))
			{/* remove scratch, keyword, or undefined symbols */
				Removesymbol(arg);
				COMPVALUE(list,i) = (Symbolhandle) 0;
			}			
		} /*for(i=0;i<ncomps;i++)*/
	} /*if(list != (Symbolhandle) 0 && TYPE(list) == LIST)*/
} /*CleanupList()*/

			   
/*
  remove all typed ASSIGNED and temporary variables from regular symbol table
  Called only after Unscratch() which should clear out scratch symbol table
*/

void RemoveUndef(void)
{
	Symbolhandle       symh, next;
	WHERE("RemoveUndef");

	for (symh = Firstsymbol(0); symh != (Symbolhandle) 0; symh = next)
	{
		next = Nextsymbol(symh,0);
		if(istempname(NAME(symh)) || TYPE(symh) == ASSIGNED)
		{ /* either undefined or a temporary variable */
			Removesymbol(symh);
		}
	} /*for (symh = Firstsymbol(0); symh != (Symbolhandle) 0; symh = next)*/
} /*RemoveUndef()*/

/*
	Copy dimension information and labels, if any
	980618 added argument copyLabels
*/
static int copySymbolInfo(Symbolhandle from, Symbolhandle to, int copyLabels)
{
	long            toIsShort = (ISSHORT(to)) ? SHORTSYMBOL : 0;
	WHERE("copySymbolInfo");
	
	setTYPE(to, TYPE(from) | toIsShort);

#if (USENOMISSING)
	if (NOMISSING(from))
	{
		setNOMISSING(to);
	}
#endif /*USENOMISSING*/
	setNCLASS(to,NCLASS(from));

#if (1) /*980621*/
	Setdims(to, NDIMS(from), &DIMVAL(from, 1));
#else /*1*/
	{
		int 	ndims = NDIMS(from), i;

		for (i = 1; i <= MAXDIMS; i++)
		{
			if (i <= ndims)
			{
				setDIM(to, i, DIMVAL(from,i));
			}
			else
			{
				setDIM(to, i, 0);
			}
		} /*for (i = 1; i <= MAXDIMS; i++)*/
	}
	
#endif /*1*/

	if (copyLabels)
	{
		if (HASLABELS(from))
		{
			char      **labels;

			labels = myduphandle(LABELSHANDLE(from));
			if (labels == (char **) 0 || !setLabels(to, labels))
			{
				mydisphandle(labels);
				return (0);
			}
		} /*if (HASLABELS(from))*/

		if (HASNOTES(from))
		{
			char      **notes;

			notes = myduphandle(NOTESHANDLE(from));
			if (notes == (char **) 0)
			{
				return (0);
			}
			if (!setNotes(to, notes))
			{
#ifndef NOTESARECHAR
				mydisphandle(notes);
#endif /*NOTESARECHAR*/
				return (0);
			}
		} /*if (HASNOTES(from))*/
	} /*if (copyLabels)*/
	
	return (1);
	
} /*copySymbolInfo()*/

long            Nassigned = 0; /* global to communicate with memoryUsage() */
#define  ASSIGNSTACKCHUNK   10
static long     AssignedStackSize = 0; /*amount of space allocated*/
static long     AssignedStackTop = -1; /* index of current assignment  */
static Symbolhandle **AssignedStack = (Symbolhandle **) 0;

/*
   maintain stack of pending assignments
*/
static int pushAssignedSymbol(Symbolhandle symh)
{
	WHERE("pushAssignedSymbol");
	
	if (++AssignedStackTop >= AssignedStackSize)
	{
		AssignedStack = (Symbolhandle **)
			((AssignedStack == (Symbolhandle **) 0) ?
			mygethandle(ASSIGNSTACKCHUNK * sizeof(Symbolhandle)) :
			mygrowhandle((char **) AssignedStack,
						 (AssignedStackSize + ASSIGNSTACKCHUNK) * sizeof(Symbolhandle)));
		if (AssignedStack == (Symbolhandle **) 0)
		{
			return (0);
		}
		AssignedStackSize += ASSIGNSTACKCHUNK;
	} /*if (++AssignedStackTop >= AssignedStackSize)*/

	(*AssignedStack)[AssignedStackTop] = symh;
	return (1);
} /*pushAssignedSymbol()*/

static void popAssignedSymbol(void)
{
	WHERE("popAssignedSymbol");
	
	if (AssignedStackTop >= 0)
	{
		(*AssignedStack)[AssignedStackTop--] = (Symbolhandle) 0;
	}
} /*popAssignedSymbol()*/

void cleanAssignedStack(void)
{
	if (AssignedStack != (Symbolhandle **) 0)
	{
		(*AssignedStack)[0] = (Symbolhandle) 0;
	}
	AssignedStackTop = -1;
	Nassigned = 0;
} /*cleanAssignedStack()*/

/*
   980218 Fixed bug allowing reference to AssignedStack before it was
          started
*/
long isPendingAssign(Symbolhandle arg)
{
	WHERE("isPendingAssign");

   if (AssignedStackTop >= 0)
   {
		Symbolhandle  *stack = *AssignedStack;
		long           i;

		for (i = 0; i <= AssignedStackTop; i++)
		{
			if (arg == stack[i])
			{
				return (1);
			}
      }
	} /*for (i = 0; i <= AssignedStackTop; i++)*/
	return (0);
} /*isPendingAssign()*/

/*
   Function to do assignments to keywords (keyword == KEYWORDASSIGN)
   or to variables
   Called with keyword == LEFTHANDSIDE by parser after parsing the left
   side of an assignment statement, before parsing the right side.
   Called with keyword == RIGHTHANDSIDE by parser after parsing the right'
   side.
   Called with keyword == -Op, where Op is one of ADD, SUB, MULT, DIV, MOD,
   and EXP to implement <-+, <--, <-*, <-/, <-%%, <-^

   970629 added capability to work with LONG symbols in conjuction with
   new function asLong().  Assign() coerces an ordinary assignment of
   a LONG variable to REAL using CopyLongToDouble();  a keyword assignment
   does not coerce the r.h.s.

   980618 replaced some code with call to copySymbolInfo()
*/

/*
   #defines added 950808
   These must be defined identically in mainpars.y.  They allow checking
   of validity of left side of assignment before right side is
   evaluated.
*/
#define LEFTHANDSIDE         1
#define RIGHTHANDSIDE        2
#define KEYWORDASSIGN        3

Symbolhandle    Assign(Symbolhandle arg1, Symbolhandle arg2, long keyword)
{
	/* assign contents of arg2 to variable with name as in arg1 */

	char            name[NAMELENGTH+1];
	char           *namearg1;
	Symbolhandle    result = (Symbolhandle) 0;
	char          **temphandle;
	long            type1, type2;
	long            op = 0;
	char           *msgs, *what = "";
	short           assignop = (keyword < 0);
	short           keyassign = (keyword == KEYWORDASSIGN);
	short           leftside = (keyword == LEFTHANDSIDE);
	short           rightside = (keyword == RIGHTHANDSIDE);
	long            leftIsSpecial = 0;
	char            tOrF = '\0';
	WHERE("Assign");
	
	msgs = (keyassign) ? "keyword " : "";

	OUTSTR[0] = '\0';
	/* check left hand side */

	if (assignop)
	{
		op = -keyword;
		switch (op)
		{
		  case ADD:
			op = ASSIGNADD;
			break;
		  case SUB:
			op = ASSIGNSUB;
			break;
		  case MULT:
			op = ASSIGNMULT;
			break;
		  case DIV:
			op = ASSIGNDIV;
			break;
		  case EXP:
			op = ASSIGNPOW;
			break;
		  case MOD:
			op = ASSIGNMOD;
			break;
		  default:
			op = -1; /* should not happen */
		} /*switch (op)*/
	} /*if (assignop)*/
	
	if(arg1 == (Symbolhandle) 0)
	{
		sprintf(OUTSTR,"ERROR: apparently out of space during %sassign",msgs);
	}
	else if ((assignop || leftside) && TYPE(arg1) == BLTIN)
	{
		if (leftside)
		{
			sprintf(OUTSTR,"ERROR: illegal assignment to built-in function");
		}
		else
		{
			sprintf(OUTSTR,
					"ERROR: %s illegal with built-in function", opName(op));
		}
	}
	else if (!keyassign && !myvalidhandle((char **) arg1))
	{ /* to protect against something like 'a <- delete(a)' */
		sprintf(OUTSTR,
				"ERROR: right side of assignment damaged variable on left side");
	}
	else if ((namearg1 = NAME(arg1) , type1 = TYPE(arg1),
			  leftIsSpecial = isSpecial(arg1)))
	{
		; /* do nothing here, check for assignability below */
	}
	else if (keyassign && !isscratch(namearg1) && !iskeystart(namearg1[0]))
	{
		sprintf(OUTSTR,
				"ERROR: keyword name cannot start with '%c'", namearg1[0]);
	}
	else if (type1 == NULLSYM && keyassign)
	{
		 /*
			can use NULL or other NULL variable as a keyword
			The parser should never allow a scratch variable
			or a function returning NULLSYMBOL as a keyword
		 */
		if (strcmp(namearg1, NULLSCRATCH) == 0)
		{
			tOrF = 'N';
		}
	}
	else if (keyassign && type1 == LOGIC &&
			 strcmp(namearg1, LOGICSCRATCH) == 0)
	{
		tOrF = (DATAVALUE(arg1, 0) != 0) ? 'T' : 'F';
	}
	else if (isscratch(namearg1) || arg1 == NULLSYMBOL)
	{
		if(type1 == LOGIC)
		{
			what = "T or F, LOGICAL expression, or function result";
		}
		else if(type1 == CHAR)
		{
			what = "string or function result";
		}
		else if(type1 == STRUC)
		{
			what = "expression or function result";
		}
		else if(type1 == NULLSYM)
		{
			what = "NULL";
		}
		else
		{
			what = "number, expression, or function result";
		}
		sprintf(OUTSTR, "ERROR: %sassignment to %s",msgs,what);
	}
	if(*OUTSTR)
	{
		goto errorExit;
	}

	if (assignop && (type1 == UNDEF || type1 == ASSIGNED))
	{
		sprintf(OUTSTR,
				"ERROR: UNDEFINED %s ...",opName(op));
		goto errorExit;
	} 

	if(assignop || leftside)
	{/* we are processing only l.h.s. of non-keyword assignment*/
		if(leftside)
		{
			if (type1 == UNDEF)
			{
				/* move from scratch table to regular table */
				Cutsymbol(arg1);
				setTYPE(arg1,ASSIGNED);
				Addsymbol(arg1);
				Nassigned++;
			} /*if(type1 == UNDEF)*/
			else if(leftIsSpecial)
			{/* assigning to a special symbol such as CLIPBOARD*/
				/* check whether non-keyword assignment is legal */
				if(SETSPECIAL((SpecialSymbolhandle) arg1)
				   (arg1, CHECKLEFTSPECIAL) == (Symbolhandle) 0)
				{
					goto errorExit;
				}
			}
		} /*if(leftside)*/

		if (!leftIsSpecial && !pushAssignedSymbol(arg1))
		{
			FatalError = 1;
			return (0);
		}
		return (arg1);
	} /*if(assignop || leftside)*/

	/* now check right hand side */
	if (arg2 == (Symbolhandle) 0 ||
		isDefined(arg2) && DIMVAL(arg2,1) == 0 &&
		(type2 = TYPE(arg2)) != NULLSYM)
	{
		sprintf(OUTSTR,"ERROR: %sassignment from null operation",msgs);
	}
	else if (!isDefined(arg2))
	{
		sprintf(OUTSTR, "ERROR: %sassignment from undefined variable %s",
				msgs,NAME(arg2));
	}
	else if ((type2 = TYPE(arg2)) == LIST || type2 == BLTIN)
	{						/* LIST should be impossible, but play it safe */
		sprintf(OUTSTR,"ERROR: %sassignment from %s ",
				msgs,(type2 == LIST) ? "LIST" : "built-in function");
	}
	else if (leftIsSpecial && rightside &&
			 SETSPECIAL((SpecialSymbolhandle) arg1)(arg2, CHECKRIGHTSPECIAL)
			 == (Symbolhandle) 0)
	{ /* check to see if value is appropriate */
		goto errorExit;
	}

	if(*OUTSTR)
	{
		goto errorExit;
	}

	if (rightside)
	{
		if (!leftIsSpecial)
		{
		/* must be existing variable, delete contents unless identical to RHS*/
			if(arg1 == arg2)
			{/* assignment to self */
				popAssignedSymbol();
				return (arg1);
			}

			strcpy(name, namearg1);
			if(name[0] == 'S' && strcmp(name,"STRMODEL") == 0)
			{/* changing STRMODEL invalidates all global glm info */
				clearGlobals();
			}
			Cutsymbol(arg1);

			/* clean out arg1, but save for re-use */
			if(type1 == ASSIGNED)
			{
				Nassigned--;
			}
			else
			{
				DeleteContents(arg1);
			}
		} /*if (!leftIsSpecial)*/
		else
		{/* copy right side to special variable*/
			return (SETSPECIAL((SpecialSymbolhandle) arg1)(arg2, ASSIGNSPECIAL));
		}
	} /*if(rightside)*/
	else
	{/* keyword: create name of form @@name */
		if(tOrF == '\0' && strlen(NAME(arg1)) > NAMELENGTH-2)
		{
			sprintf(OUTSTR,
					"ERROR: keyword name '%s' longer than %ld characters",
					NAME(arg1),(long) NAMELENGTH-2);
			goto errorExit;
		} /*if(strlen(NAME(arg1)) > NAMELENGTH-2)*/
		else
		{
			name[0] = KEYPREFIX1;
			name[1] = KEYPREFIX2;
			if (tOrF)
			{
				if (tOrF == 'N')
				{
					strcpy(name+2, NULLNAME);
				}
				else
				{
					name[2] = tOrF;
					name[3] = '\0';
				}
			}
			else
			{
				strcpy(name+2,namearg1);
			}
			
			/* the following should stop the build up of UNDEFs in macros*/
			if(type1 == UNDEF)
			{/* save for reuse */
				Cutsymbol(arg1);
			}
			else
			{/* can't reuse, so forget about arg1 */
				arg1 = (Symbolhandle) 0;
			}
		} /*if(strlen(NAME(arg1)) > NAMELENGTH-2){}else{}*/
	} /*if(rightside){}else{}*/

	if (type2 == LONG && !keyassign)
	{
		/* coerce long to double */
		Symbolhandle     symhTmp;
		
		symhTmp = RInstall(SCRATCH, 0);
		
		/* coerce arg2 to a REAL symbol*/
		if (symhTmp == (Symbolhandle) 0 ||
			!CopyLongToDouble(arg2, symhTmp))
		{
			Removesymbol(symhTmp);
			goto errorExit;
		}
		setNAME(symhTmp, SCRATCH);
		if (isscratch(NAME(arg2)))
		{
			Removesymbol(arg2);
		}
		arg2 = symhTmp;
		type2 = REAL;
	} /*if (type2 == LONG)*/

	if (isscratch(NAME(arg2)))
	{/* if from SCRATCH, reuse or move content handles, do not copy */
		Cutsymbol(arg2);
		if(arg1 == (Symbolhandle) 0)
		{/* no l.h.s. to reuse, so reuse r.h.s symbol */
			result = arg2;
		} /*if(arg1 == (Symbolhandle) 0)*/
		else
		{/* reuse l.h.s. symbol; move content handles, do not Copy() */
			result = arg1;
			/* copy NCLASS, NOMISSING, TYPE, NDIMS, DIMS, but not LABELS,NOTES */
#if (1) /* changed 980718*/
			copySymbolInfo(arg2, result, 0);
#else /*1*/
			setTYPE(result,type2);
			setNCLASS(result,NCLASS(arg2));
#if (USENOMISSING)
			if (NOMISSING(arg2))
			{
				setNOMISSING(result);
			}
#endif /*USENOMISSING*/
			setNDIMS(result,NDIMS(arg2));
			for(i=1;i<=NDIMS(arg2);i++)
			{
				setDIM(result,i,DIMVAL(arg2,i));
			}
#endif /*1*/
			/* transfer, do not duplicate, LABELS and NOTES*/
			temphandle = (char **) LABELS(arg2);
			setLABELS(arg2, (LabelsHandle) 0);
			setLABELS(result, (LabelsHandle) temphandle);

			temphandle = (char **) NOTES(arg2);
			setNOTES(arg2, (NotesHandle) 0);
			setNOTES(result, (NotesHandle) temphandle);

			switch (type2)
			{
			  case CHAR:
			  case MACRO:
				temphandle = STRING(arg2);
				setSTRING(arg2,(char **) 0);
				setSTRING(result,temphandle);
				break;

			  case REAL:
			  case LOGIC:
				temphandle = (char **) DATA(arg2);
				setDATA(arg2,(double **) 0);
				setDATA(result,(double **) temphandle);
				break;

			  case LONG:
				/*should occur only for keyword*/
				temphandle = (char **) LONGDATA(arg2);
				setLONGDATA(arg2,(long **) 0);
				setLONGDATA(result,(long **) temphandle);
				break;

			  case STRUC:
				temphandle = (char **) COMP(arg2);
				setCOMP(arg2,(Symbolhandle **) 0);
				setCOMP(result,(Symbolhandle **) temphandle);
				break;

			  case NULLSYM:
				setSTRING(result, (char **) 0);
				break;
				
			  default:		/* safety net */
				temphandle = (char **) COMP(arg2);
				setCOMP(arg2,(Symbolhandle **) 0);
				setCOMP(result,(Symbolhandle **) temphandle);
			} /*switch (type2)*/
			Delete(arg2);
		} /*if(arg1 == (Symbolhandle) 0){}else{}*/
		arg2 = (Symbolhandle) 0;
	} /*if (isscratch(NAME(arg2)))*/
	else
	{/* r.h.s. not scratch, copy to l.h.s. */
		if(arg1 == (Symbolhandle) 0)
		{/* no l.h.s. symbol remembered; must be not UNDEF keyword */
			result = Makesymbol(type2);
		}
		else
		{/* re-use l.h.s. symbol */
			result = arg1;
			clearLabels(result);
		}

		if (result == (Symbolhandle) 0 || Copy(arg2, result) == 0)
		{
			goto errorExit;
		}
	} /*if (isscratch(NAME(arg2))){}else{}*/
	setNAME(result, name);
	Addsymbol(result);
	if (rightside)
	{
		popAssignedSymbol();
	}
	
	return (result);

  errorExit:				/* from Assign */
	if(*OUTSTR)
	{
		if(!rightside)
		{
			yyerror(OUTSTR);
		}
		else
		{
			putOUTSTR();
		}
	}
	Delete(result);
	return (0);
} /*Assign()*/


/* 
  Recursively copy contents of from into to

  As of 930503, it is the responsibility of the calling program to get
  rid of to if the copy could not be completed.
*/

int Copy(Symbolhandle from, Symbolhandle to)
{
	Symbolhandle    symh;
	long            i, j, tot;
	WHERE("Copy");
	
	if (to == (Symbolhandle) 0 || from == (Symbolhandle) 0)
	{
		putOutErrorMsg("ERROR: (internal) illegal copy argument(s)");
		goto errorExit;
	}

	/* copy dimensions and labels, if any */

	if (!copySymbolInfo(from, to, 1))
	{
		goto errorExit;
	}

	/* copy contents, if any */
	switch ((int) TYPE(from))
	{
	  case UNDEF:
	  case NULLSYM:
		break;
		
	  case MACRO:
	  case CHAR:
		TMPHANDLE = STRING(from);
		if(TMPHANDLE != (char **) 0)
		{
			TMPHANDLE = myduphandle(TMPHANDLE);
			if(TMPHANDLE == (char **) 0)
			{
				goto errorExit;
			}
		}
		setSTRING(to,TMPHANDLE);
		break;

	  case LIST:
	  case STRUC:
		tot = symbolSize(from);
		TMPHANDLE = (char **) COMP(from);
		if(TMPHANDLE != (char **) 0)
		{
			long       handleLength = myhandlelength(TMPHANDLE);
			
			if (handleLength == CORRUPTEDHANDLE)
			{
				goto errorExit;
			}
			
			TMPHANDLE = mygethandle(handleLength);
			setCOMP(to,(Symbolhandle**) TMPHANDLE);
			if(TMPHANDLE == (char **) 0)
			{
				goto errorExit;
			}
			for (i = 0; i < tot; i++)
			{
				COMPVALUE(to, i) = (Symbolhandle) 0;
			}
			
			for (i = 0; i < tot; i++)
			{
				symh = COMPVALUE(to, i) = Makesymbol(TYPE(COMPVALUE(from, i)));
				if (symh == (Symbolhandle) 0 ||
					Copy(COMPVALUE(from, i), symh) == 0)
				{
					for (j = 0; j <= i; j++)
					{
						Delete(COMPVALUE(to, j));
						COMPVALUE(to,j) = 0;
					}
					goto errorExit;
				}
			} /*for (i = 0; i < tot; i++)*/
		} /*if(TMPHANDLE != (char **) 0)*/
		else
		{
			setCOMP(to,(Symbolhandle**) 0);
		}
		
		break;

	  case LOGIC:
	  case REAL:
		if(DATA(from) != (double **) 0)
		{
			TMPHANDLE = myduphandle((char **) DATA(from));
			if(TMPHANDLE == (char **) 0)
			{
				goto errorExit;
			}
			setDATA(to,(double **) TMPHANDLE);
		}
		else
		{
			setDATA(to,(double **) 0);
		}
		break;

	  case LONG:
		TMPHANDLE = (char **) LONGDATA(from);
		if(TMPHANDLE != (char **) 0)
		{
			TMPHANDLE = myduphandle(TMPHANDLE);
			if(TMPHANDLE == (char **) 0)
			{
				goto errorExit;
			}
		}
		setLONGDATA(to,(long **) TMPHANDLE);
		
		break;

	  case PLOTINFO:
		setGRAPH(to,copyGraph(GRAPH(from)));
		if(GRAPH(to) == (whole_graph **) 0)
		{
			goto errorExit;
		}
		break;

	  default:
		putOutErrorMsg("ERROR: (internal) copy of illegal variable type");
		goto errorExit;
	} /*switch ((int) TYPE(from))*/
/*
  Note that name is copied.  Sometimes that is not wanted.  In
  any event that is done last, so name of to is not changed until it is
  complete
*/
	setNAME(to, NAME(from));
	return (1);		/* successful */

  errorExit: /* from copy */
	/* leave it to calling function to cleanup to */
	
	return (0);
	
} /*Copy()*/

/*
  Coerce LONG symbol from into REAL symbol to, which must be defined.
  In case of error, calling function must clean up to
*/
int CopyLongToDouble(Symbolhandle from, Symbolhandle to)
{
	long            tot;
	WHERE("CopyLongToDouble");
	
	if (to == (Symbolhandle) 0 || from == (Symbolhandle) 0)
	{
		putOutErrorMsg("ERROR: (internal) illegal CopyLongToDouble argument(s)");
		goto errorExit;
	}

	/* copy dimensions and labels, if any */
	if (!copySymbolInfo(from, to, 1))
	{
		goto errorExit;
	}
	setTYPE(to, REAL);
#if (USENOMISSING)
	clearNOMISSING(to);
#endif /*USENOMISSING*/
	setNCLASS(to, -1);
	tot = symbolSize(from);
	TMPHANDLE = mygethandle(tot * sizeof(double));
	if (TMPHANDLE == (char **) 0)
	{
		goto errorExit;
	}
	setDATA(to, (double **) TMPHANDLE);
	longToDouble(LONGDATAPTR(from), DATAPTR(to), tot);
	setNAME(to, NAME(from));
	return (1);
	
  errorExit: /* from CopyLongToDouble */
	/* leave it to calling function to cleanup to */
	
	return (0);
} /*CopyLongToDouble()*/

/*
  Coerce REAL symbol from into LONG symbol to, which must be defined.
  In case of error, calling function must clean up to
*/
int CopyDoubleToLong(Symbolhandle from, Symbolhandle to)
{
	long            tot;
	WHERE("CopyDoubleToLong");
	
	if (to == (Symbolhandle) 0 || from == (Symbolhandle) 0)
	{
		putOutErrorMsg("ERROR: (internal) illegal CopyDoubleToLong argument(s)");
		goto errorExit;
	}

	/* copy dimensions and labels, if any */
	if (!copySymbolInfo(from, to, 1))
	{
		goto errorExit;
	}
	setTYPE(to, LONG);
#if (USENOMISSING)
	clearNOMISSING(to);
#endif /*USENOMISSING*/
	setNCLASS(to, -1);
	tot = symbolSize(from);
	TMPHANDLE = mygethandle(tot * sizeof(long));
	if (TMPHANDLE == (char **) 0)
	{
		goto errorExit;
	}
	setLONGDATA(to, (long **) TMPHANDLE);
	doubleToLong(DATAPTR(from), LONGDATAPTR(to), tot);
	setNAME(to, NAME(from));

	return (1);
	
  errorExit: /* from CopyDoubleToLong */
	/* leave it to calling function to cleanup to */
	
	return (0);
} /*CopyDoubleToLong()*/


/*
  Routine to centralize what needs to be done when re-using an
  element of the argument list. This should normally only be
  called when the argno-th (base 0) argument is a scratch variable
*/

Symbolhandle reuseArg(Symbolhandle list, long argno,
					  int keepLabels, int keepNotes)
{
	Symbolhandle      result = COMPVALUE(list, argno);
	
	COMPVALUE(list, argno) = (Symbolhandle) 0;
	if (result != (Symbolhandle) 0)
	{
		if (!keepLabels)
		{
			clearLabels(result);
		}
		if (!keepNotes)
		{
			clearNotes(result);
		}
#if (USENOMISSING)
		clearNOMISSING(result);
#endif /*USENOMISSING*/
	}
	return (result);
} /*reuseArg()*/

/*
  Routine to extract a component of a structure with same name as name of
  namearg.

  Called by mainpars for str$name (named == 1) or str[[expr]] (named == 0),
  when str is a structure.    It is also called indirectly for str[list]
  when str is a structure.  For str[[expr]], the parser insures there must
  be exactly index.  The parser interprets str[list] as subscripting and
  the subscript processor (Element() in Lang.c) calls Extract.  If list has
  more than one element, all but the first must be 1, T, or empty.

  In addition, expr[[1]] is legal when expr is not a structure and is
  synonymous with expr.  The use of [[...]] is no longer necessary and should
  probably be phased out.

  950906 fixed things so that str$NULL is legal as earlier str$T and str$F were
  legal

  980218 Modified so that str[NULL] is legal and is NULL
*/

Symbolhandle    Extract(Symbolhandle strarg, Symbolhandle namearg,
						long named)
{
	long            ncomps = 1, i, j, length, **subvals = (long **) 0;
	long            nsubscripts;
	Symbolhandle    result = (Symbolhandle) 0, symh;
	Symbolhandle    longTrash = (Symbolhandle) 0;
	char            tOrF[2], *name;
	int             foundNullSubscr = 0;
	WHERE("Extract");

	OUTSTR[0] = '\0';
	if (!isFakeSymbol(strarg) && !myvalidhandle((char **) strarg) ||
		TYPE(strarg) == LIST)
	{
		sprintf(OUTSTR,
				"ERROR: structure component extraction from damaged (deleted ?) variable");
	}
	else if (isNull(strarg))
	{
		sprintf(OUTSTR,
				"ERROR: structure component extraction from null variable");
	}
	else if(!isDefined(strarg))
	{
		sprintf(OUTSTR,
				"ERROR: structure component extraction from undefined variable %s",
				NAME(strarg));
	}
	else if(isStruc(strarg))
	{
		ncomps = NCOMPS(strarg);
	}
	else if(named)
	{
		sprintf(OUTSTR,"ERROR: use of expr$name when expr is not a structure");
	}
	if(*OUTSTR)
	{
		goto yyerrorMsg;
	}
	if(!named)
	{/* str[vec] or str[[i]] */
		if (namearg == (Symbolhandle) 0)
		{
			sprintf(OUTSTR, "ERROR: missing structure subscript");
			goto yyerrorMsg;
		}
		if(TYPE(namearg) == LIST)
		{ /* str[vec] */
			nsubscripts = NCOMPS(namearg);
		}
		else if(!isDefined(namearg))
		{ /* str[[expr]] */
			sprintf(OUTSTR,
					"ERROR: undefined structure index %s",NAME(namearg));
			goto yyerrorMsg;
		}
		else
		{ /* str[[expr]] */
			nsubscripts = 1;
		}
		
		/* 
		  Check subscripts
		  Note: if nsubscripts > 1, all extra subscripts must be empty or
		  equivalent to cat(1) or cat(T)
		*/
		for(i=0;i<nsubscripts;i++)
		{
			symh = (i > 1 || TYPE(namearg) == LIST) ?
				COMPVALUE(namearg,i) : namearg;
			if(symh != (Symbolhandle) 0)
			{
				if (i == 0 && TYPE(namearg) == LIST && TYPE(symh) == NULLSYM)
				{
					foundNullSubscr = 1;
				}
				else if(TYPE(symh) != REAL && TYPE(symh) != LOGIC ||
				   !isVector(symh))
				{
					sprintf(OUTSTR,
							"ERROR: structure index not REAL or LOGICAL vector");
				}
				else if(anyMissing(symh))
				{
					sprintf(OUTSTR,
							"ERROR: MISSING values not allowed in structure index");
				}
				else if(i>0 && (!isScalar(symh) || DATAVALUE(symh,0) != 1.0))
				{
					sprintf(OUTSTR,
							"ERROR: extra subscripts on structures must have value 1 or T");
				}						
				if(*OUTSTR)
				{
					goto yyerrorMsg;
				}
			}/*if(symh != (Symbolhandle) 0)*/
		}/*for(i=0;i<nsubscripts;i++)*/

		if(TYPE(namearg) == LIST)
		{ /* any extra subscripts are irrelevent */
			namearg = COMPVALUE(namearg,0);
		}
		
		if (!isStruc(strarg))
		{
			/* must be expr[[1]] which is allowed, even if expr is not STRUC*/
			if(!isScalar(namearg) || DATAVALUE(namearg,0) != 1.0)
			{ /* Note: parser does not allow expr[[]] */
				sprintf(OUTSTR,
						"ERROR: use of a[[i]] with i > 1 when a is not a structure");
				goto yyerrorMsg;
			}

			/* return entire expression */
			result = Install(SCRATCH,TYPE(strarg));
			if(result == (Symbolhandle) 0 || Copy(strarg,result) == 0)
			{
				goto errorExit;
			}
			goto normalExit;
		} /* if(!isStruc(strarg)) */
		if (foundNullSubscr)
		{
			result = Install(SCRATCH, NULLSYM);
			if (result == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			goto normalExit;
		}
		/* must be structure with subscripts */
		if ((length = subscriptLength(namearg,ncomps)) == 0 ||
		   (longTrash = LongInstall(SCRATCH,length)) == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		subvals = LONGDATA(longTrash);
		if ((length = buildSubscript(namearg,ncomps,length,*subvals, (int *) 0)) < 0)
		{
			goto errorExit;
		}
		
		if(length > 1 &&
		   (result = StrucInstall(SCRATCH,length)) == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		else if (length == 0 && (result = Install(NULLSCRATCH, NULLSYM)) == 0)
		{
			goto errorExit;
		}
		/* If length == 0, one subscript all F */

		if (length > 1 && HASLABELS(strarg))
		{
			long         needed = 0;
			char        *pos;

			for (i = 0; i < length; i++)
			{
				needed += strlen(getOneLabel(strarg, 0, (*subvals)[i])) + 1;
			}
			TMPHANDLE = mygethandle(needed);
			
			if (TMPHANDLE == (char **) 0 || !setLabels(result, TMPHANDLE))
			{
				mydisphandle(TMPHANDLE);
				goto errorExit;
			}
			pos = *TMPHANDLE;
			for (i = 0; i < length; i++)
			{
				pos = copyStrings(getOneLabel(strarg, 0, (*subvals)[i]),
								  pos, 1);
			} /*for (i = 0; i < length; i++)*/
		} /*if (!named && length > 1 && HASLABELS(strarg))*/
		
		for (i = 0;i < length;i++)
		{
			j = (*subvals)[i];
			symh = Makesymbol(REAL);
			if (symh == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			
			if(length == 1)
			{
				Addsymbol(symh);
				result = symh;
			}
			else
			{
				COMPVALUE(result,i) = symh;
			}

			if (symh == (Symbolhandle) 0 ||
				Copy(COMPVALUE(strarg, j), symh) == 0)
			{
				goto errorExit;
			}
		} /*for(i=0;i<length;i++)*/
		Removesymbol(longTrash);
		if (length == 1)
		{
			strarg = COMPVALUE(strarg, j);
		}
	} /*if(!named)*/
	else 
	{ /* str$name */
		name = NAME(namearg);
		
		if(isscratch(name))
		{
			if (strcmp(name, LOGICSCRATCH) != 0 &&
				strcmp(name, NULLSCRATCH) != 0)
			{
				sprintf(OUTSTR,
						"ERROR: expression or string in place of component name");
			}
			else
			{ /* allow strName$T, strName$F, strName$NULL */
				if (TYPE(namearg) == NULLSYM)
				{
					name = NULLNAME;
				}
				else
				{
					tOrF[0] = (DATAVALUE(namearg, 0) != 0.0) ? 'T' : 'F';
					tOrF[1] = '\0';
					name = tOrF;
				}
			}
		} /*if(isscratch(name))*/
		if (*OUTSTR != '\0')
		{
			goto yyerrorMsg;
		}			
		for (i = 0; i < ncomps; i++)
		{
			if (strcmp(NAME(COMPVALUE(strarg, i)), name) == 0)
			{
				break;
			}
		} /*for (i = 0; i < ncomps; i++)*/
		if(i == ncomps)
		{
			sprintf(OUTSTR,"ERROR: no component with name %s",name);
			goto yyerrorMsg;
		}
		if (TYPE(namearg) == UNDEF)
		{
			Removesymbol(namearg);
		}
		result = Install(SCRATCH,TYPE(COMPVALUE(strarg, i)));
		if(result == (Symbolhandle) 0 || Copy(COMPVALUE(strarg,i),result) == 0)
		{
			goto errorExit;
		}
		strarg = COMPVALUE(strarg, i);
	} /*if(!named){}else{}*/

  normalExit:
	if (TYPE(result) == MACRO)
	{
		setScratchMacroName(result, NAME(strarg));
	}
	else
	{
		setNAME(result, SCRATCH);
	}
	return (result);

  yyerrorMsg:
	yyerror(OUTSTR);
	/* fall through */
	
  errorExit: /* in Extract() */
	Removesymbol(longTrash);
	Removesymbol(result);
	
	return (0);
} /*Extract()*/

/*
   Start a new list of arguments or subscripts
   To save thrashing as list grows, room for DEFAULTLISTLENGTH
   items is allocated initially
   Called only from parser and Arith() (in Lang.c)
 */
Symbolhandle    Makelist(void)
{
	Symbolhandle    result;
	int             i;
	
	result = Install(LISTSCRATCH,LIST);
	
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	
	TMPHANDLE = mygethandle(DEFAULTLISTLENGTH * (long) sizeof(Symbolhandle));
	setCOMP(result,(Symbolhandle**) TMPHANDLE);
	if (TMPHANDLE == (char **) 0)
	{
		goto errorExit;
	}
	setNDIMS(result,1);
	setNCOMPS(result,1);

	for(i = 0;i<DEFAULTLISTLENGTH;i++)
	{
		COMPVALUE(result, i) = (Symbolhandle) 0;
	}
	
	return (result);

  errorExit:
	Removelist(result);
	return (0);
} /*Makelist()*/


/*
   Puts arg in last spot in list.  It is assumed that NCOMPS(list) has
   already been incremented and that there is a place for arg
   It does nothing but return list if list is 0
   Called only from parser and Arith() (in Lang.c)
*/
Symbolhandle    Addlist(Symbolhandle list, Symbolhandle arg)
{
	/* put arg in the last space of the list */
	if (list != (Symbolhandle) 0)
	{
		if(TYPE(list) != LIST)
		{
			putOutErrorMsg("ERROR: (internal) not an argument list");
			return (0);
		}
		COMPVALUE(list, NCOMPS(list) - 1) = arg;
	}
	
	return (list);
} /*Addlist()*/

/*
   Make an additional slot in the list an increment NCOMPS(list)
   It knows list may not be full yet.
   It does nothing but return list if list is 0
   Called only from parser and Arith() (in Lang.c)
 */

Symbolhandle    Growlist(Symbolhandle list)
{
	long        ncomps;
	long        maxcomps, i;

	if(list != (Symbolhandle) 0)
	{
		long          handleLength = myhandlelength((char **) COMP(list));
		
		if (handleLength == CORRUPTEDHANDLE)
		{
			goto errorExit;
		}
		
		maxcomps = handleLength/sizeof(Symbolhandle);
		ncomps = NCOMPS(list) + 1;
		if(ncomps > maxcomps)
		{
			maxcomps += DEFAULTLISTLENGTH;
			TMPHANDLE = mygrowhandle((char **) COMP(list),
									 maxcomps * sizeof(Symbolhandle));
			setCOMP(list,(Symbolhandle**) TMPHANDLE);
			if (TMPHANDLE == (char **) 0)
			{
				goto errorExit;;
			}
			for(i=ncomps;i<maxcomps;i++)
			{
				COMPVALUE(list,i) = (Symbolhandle) 0;
			}
		} /*if(ncomps > maxcomps)*/
		setNCOMPS(list,ncomps);
	
		COMPVALUE(list, ncomps - 1) = (Symbolhandle) 0;
	} /*if(list != (Symbolhandle) 0)*/
	
	return (list);

  errorExit:
	Removelist (list);
	return (0);
} /*Growlist()*/

/*
   if string represents a literal (number, '?', 'T', 'F', 'NULL', "string"),
   return a scalar of the appropriate type; otherwise return (Symbolhandle) 0
*/
static Symbolhandle literalToSymbol(char * string, long * byNameType)
{
	Symbolhandle   symh = (Symbolhandle) 0;
	double         y, absy;
	char          *pc = string;
	long           length = strlen(string), type = 0;
	
	*byNameType = 0;
	
	if (length == 1 && (string[0] == 'T' || string[0] == 'F'))
	{
		type = LOGIC;
		symh = RInstall(LOGICSCRATCH, 1);
		if (symh != (Symbolhandle) 0)
		{
			DATAVALUE(symh, 0) = (string[0] == 'T') ? 1.0 : 0.0;
		}
	}
	else if(strcmp(string, NULLNAME) == 0)
	{
		type = NULLSYM;
		symh = Install(NULLSCRATCH, type);
	}
	else if (string[0] == '"')
	{
		if (string[length-1] != '"')
		{
			goto notLiteral;
		}
		type = CHAR;
		length -= 2;
		symh = CInstall(STRINGSCRATCH, length + 1);
		if (symh != (Symbolhandle) 0)
		{
			strncpy(STRINGPTR(symh), string+1,length);
			STRINGVALUE(symh, length) = '\0';
		}
	}
	else if (strcmp(string,"?") == 0)
	{
		type = REAL;
		setMissing(y);
	}
	else
	{
		type = REAL;
		y = mystrtod(string, &pc);
		if (pc == string)
		{
			goto notLiteral;
		}
		while(*pc)
		{
			if (!isspace(*pc))
			{
				goto notLiteral;
			}
			pc++;
		} /*while(*pc)*/
		if ((absy = fabs(y), doubleEqualBdouble(absy, TooBigValue)))
		{
			yyerror("ERROR: byname expression has value too big to be represented in computer");
			*byNameType = ERROR;
			goto notLiteral;
		}
	}

	if (type == REAL)
	{
		symh = RInstall(NUMSCRATCH, 1);
		if (symh != (Symbolhandle) 0)
		{
			DATAVALUE(symh, 0) = y;
		}
	} /*if (type == REAL)*/
	
	if (type != 0)
	{
		if (symh != (Symbolhandle) 0)
		{
			setTYPE(symh, type);
			*byNameType = VAR;
		}
		else
		{
			*byNameType = FATALERROR;
		}
	} /*if (type != 0)*/
	/* fall through*/
	
  notLiteral:
	return (symh);
} /*literalToSymbol()*/

/*
   Decode "by-name" expression <<CharScalar>> as indirect refernce
   to variable whose name is in CharScalar
   If CharScalar is "?" or a quoted number, decode to REAL
   (<<"3.14">> is the same as 3.14)
   950906 If CharScalar is "T", "F", "NULL" decode appropriately
   960816 If CharScalar is "\"a string without non-escaped quotes\"" decode
          to "a string without non-escaped quotes"
*/
Symbolhandle Byname(Symbolhandle symh, long * byNameType,
					long * parseMacroArgs)
{
	Symbolhandle         result = (Symbolhandle) 0;
	long                 savePlace;
	long                 i, tempname = 0;
	int                  isName = 0;
	char                 c, name0, name1;
	char                 name[NAMELENGTH+1];
	char                *string;
	WHERE("Byname");
	
	*parseMacroArgs = 0;
	OUTSTR[0] = '\0';
	if(TYPE(symh) != CHAR || !isScalar(symh))
	{
		sprintf(OUTSTR,
				"ERROR: byname expression is not quoted string or CHARACTER scalar");
		goto errorExit;
	}
	
	string = STRINGPTR(symh);
	result = literalToSymbol(string, byNameType);
	if (*byNameType != 0)
	{
		return (result);
	} /*if (result != (Symbolhandle) 0)*/
	
	name0 = string[0];
	name1 = string[1];
	tempname = (name0 == TEMPPREFIX && isnamestart(name1));
	isName = (strlen(string) <= NAMELENGTH &&
			  (tempname || !tempname && isnamestart(name0)));
	
	for (i = 1; isName && (c = string[i]) != '\0'; i++)
	{
		isName = isnamechar(c);
	}
	
	if (isName)
	{
		strcpy(name,string);
		result = Lookup(name);
		if (result == (Symbolhandle) 0)
		{
			result = Install(name, UNDEF);
			if(result == (Symbolhandle) 0)
			{
				*byNameType = FATALERROR;
				return (0);
			}
			setNDIMS(result,1);
		} /*if (result == (Symbolhandle) 0)*/
	}
	else
	{
		result = mvEval(STRING(symh));
		if (result== (Symbolhandle) 0)
		{
			return (0);
		}
	}
	
	/*
	   920623 set *byNameType to VAR for everything except BLTIN or MACRO
	   followed by '('
	   */
	switch ((int) TYPE(result))
	{
	  case BLTIN:
		if(findLParen(0, '('))
		{
			if(strcmp(NAME(result), "batch") == 0)
			{
				*byNameType = BATCH;
			}
			else
			{
				*byNameType = BLTINTOKEN;
			}
		}
		else
		{
			*byNameType = VAR;
		}

		break;

	  case MACRO:
		savePlace = ISTRCHAR;
		if(findLParen(0, '('))
		{
			*parseMacroArgs = 1;
			*byNameType = MACROTOKEN;
		}
		else
		{
			ISTRCHAR = savePlace;
			*byNameType = VAR;
		}
		break;

	  default:
		*byNameType = VAR;
		break;
	} /*switch ((int) TYPE(result))*/

	return (result);
	
    errorExit: /* from Byname */
	yyerror(OUTSTR);

	return(0);
} /*Byname()*/

/* for debugging purposes only */


	
#ifdef USEDUMPSYMBOLS
void Dumpsymbols(int scratch, char *msgs, long count)
{
	Symbolhandle     symh,start;
	char             value[80];
	long             type;
	char            *name;
	WHERE("Dumpsymbols");
	
	sprintf(OUTSTR,"%s symbol table",(scratch) ? "Scratch" : "Regular");
	putOUTSTR();
	start = Firstsymbol(scratch);

	for (symh = start; count-- > 0 && symh != (Symbolhandle) 0;
		 symh = Nextsymbol(symh,scratch))
	{
		name = NAME(symh);
		if(name == (char *) 0)
		{
			break;
		}
		type = TYPE(symh);
		if(type == REAL && DATA(symh) != (double **) 0)
		{
			sprintf(value,"%.15g",DATAVALUE(symh,0));
		}
		if(type == LONG && LONGDATA(symh) != (long **) 0)
		{
			sprintf(value,"%15ld",LONGDATAVALUE(symh,0));
		}
		else if(type == LOGIC && DATA(symh) != (double **) 0)
		{
			sprintf(value,"%s",(DATAVALUE(symh,0) != 0.0) ? "T" : "F");
		}
		else if(type == CHAR && STRING(symh) != (char **) 0)
		{
			strncpy(value,STRINGPTR(symh),79);
			value[79] = '\0';
		}
		else
		{
			strcpy(value,"none");
		}
		
		sprintf(OUTSTR,
				"%s: symh = %08x, name = %s, type = %s, value = %s",msgs,symh,
				name,typeName(type),value);
		putOUTSTR();
	}
} /*Dumpsymbols()*/
#endif /*USEDUMPSYMBOLS*/

/*
	function to dummy up list for MacAnova command with narg arguments
	The arguments must all be scalars (vectors of length 1) with values given
	as strings in valueList[], types in typeList[], and if a keyword argument,
	with the keyword in keyList.
.
	For use in executing commands without going through parser.
	Calling routine should call Removelist(list) after command is executed.
*/
Symbolhandle Buildlist(long nargs, long typeList[], char * valueList[],
					   char * keyList[])
{
	Symbolhandle list, arg = (Symbolhandle) 0;
	long         i;
	char       **pc = (char **) 0, name[NAMELENGTH+1];
	double       value;
	WHERE("Buildlist");
	
	OUTSTR[0] = '\0';
	list = StrucInstall(SCRATCH,(nargs == 0) ? 1 : nargs);
	if (list == (Symbolhandle) 0)
	{
		return (0);
	}
	setTYPE(list,LIST);
	for (i=0;i<nargs;i++)
	{
		if(typeList[i] == REAL || typeList[i] == LOGIC)
		{
			arg = RInstall(SCRATCH,1);
			COMPVALUE(list,i) = arg;
			if(arg == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			setTYPE(arg,typeList[i]);
			if(typeList[i] == REAL)
			{
				if(sscanf(valueList[i],"%lg",&value) != 1)
				{
					putOutErrorMsg("ERROR: invalid value for Buildlist");
					goto errorExit;
				}
			}
			else
			{
				value = (valueList[i][0] == 'T') ? 1.0 : 0.0;
			}
			DATAVALUE(arg,0) = value;
		}
		else if(typeList[i] == CHAR)
		{
			arg = Install(SCRATCH,CHAR);
			COMPVALUE(list,i) = arg;
			pc = mygethandle((long) strlen(valueList[i]) + 1);
			if(arg == (Symbolhandle) 0 || pc == (char **) 0)
			{
				goto errorExit;
			}
			setSTRING(arg,pc);
			strcpy(*pc,valueList[i]);
			pc = (char **) 0;
			setNDIMS(arg,1);
			setDIM(arg,1,1);
		}
		else if(typeList[i] != 0)
		{
			sprintf(OUTSTR,
					"ERROR: illegal type in argument list to Buildlist");
			goto errorExit;
		}
		if (typeList[i] != 0)
		{
			if(keyList[i][0] != '\0')
			{
				if(strlen(keyList[i]) > NAMELENGTH-2)
				{
					sprintf(OUTSTR,
							"ERROR: Buildlist keyword name longer than %d characters",
							(int) NAMELENGTH-2);
					goto errorExit;
				}
				name[0] = KEYPREFIX1;
				name[1] = KEYPREFIX2;
				strncpy(name+2,keyList[i],NAMELENGTH-2);
				name[NAMELENGTH] = '\0';
				setNAME(arg,name);
			}
			arg = (Symbolhandle) 0;
		} /*if (typeList[i] != 0)*/
		else
		{	/* empty argument if typeList[i] == 0 */
			COMPVALUE(list,i) = (Symbolhandle) 0;
		}
	} /*for (i=0;i<nargs;i++)*/
	return (list);

  errorExit: /* from Buildlist */
	putOUTSTR();
	Removesymbol(arg);
	mydisphandle(pc);
	Removelist(list);
	return (0);
} /*Buildlist()*/

/* 
   Function to return the total size of memory (in bytes) associated with
   a symbol
*/

long SizeofSymbol(Symbolhandle symh)
{
	long          size = 0;
	long          type, ncomps, i;
	long          handleLength;
	char        **dataHandle;
	
	if (symh != (Symbolhandle) 0)
	{
		type = TYPE(symh);	
		if (type == BLTIN)
		{
			size = sizeof(FunctionSymbol);
		} /*if (type == BLTIN)*/
		else
		{
			long         labelsLength, notesLength;

			handleLength = myhandlelength((char **) symh);

			if (handleLength == CORRUPTEDHANDLE)
			{
				goto errorExit;
			}
			
			labelsLength = sizeOfLabels(symh);
			notesLength = sizeOfNotes(symh);

			if (labelsLength < 0 || notesLength < 0)
			{
				goto errorExit;
			}
			
			size = handleLength + labelsLength + notesLength;

			dataHandle = STRING(symh);
		
			if(dataHandle != (char **) 0)
			{
				handleLength = myhandlelength(dataHandle);
				if (handleLength == CORRUPTEDHANDLE)
				{
					goto errorExit;
				}
				
				size += handleLength;
				if(type == STRUC || type == LIST)
				{
					ncomps = NCOMPS(symh);
					for (i = 0;i < ncomps; i++)
					{
						size += SizeofSymbol(COMPVALUE(symh,i));
					}
				}
				else if (type == PLOTINFO)
				{
					handleLength = sizeofPlot((whole_graph **) dataHandle);

					if (handleLength == CORRUPTEDHANDLE)
					{
						goto errorExit;
					}
					size += handleLength;
				}
			} /*if(dataHandle != (char **) 0)*/
		} /*if (type == BLTIN){}else{}*/		
	} /*if (symh != (Symbohandle) 0)*/
	return (size);

  errorExit:
	return (CORRUPTEDHANDLE);
} /*SizeofSymbol()*/
				  
