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

#ifndef SYMBOLH__
#define SYMBOLH__

#include "plot.h"
/*
   950904 changed what identifies a special type from a number < 0 to
   having bit specified by SPECIALMASK set and changes various macros
   accordingly

   960717 changed types Long, Double, Char to long, double, char
   970619 added macros isInline and setInline
   971104 added badHandleCodes
   980312 added NOMISSING bit to type component.  If set, symbol is guaranteed
          to have no missing values.  This can make anyMissing() check more
          efficient.  It will be optionally set when a function knows
          a symbol has no missing values.  Care must be taken in subscript
          assignment to make sure the NOMISSING bit is unset if any assigned
          value is MISSING.  It should be preserved by subscript selection.
          Its use will be phased in gradually.
   980719 Added new component notes and macros to go with it.
   980720 Added new type ShortSymbol and macros to go with it.  Also
          define types LabelsType and NotesType (currently both (char **))
   990211 Added (long) cast in definition of TYPE and other macros using masks

*/
#ifndef DEFAULTLISTLENGTH
#define DEFAULTLISTLENGTH 20 /* default length for LIST symbols  */
#endif /*DEFAULTLISTLENGTH*/

#define SYMBOLTYPESDEFINED /* prevents dynload.h from re-defining*/

struct           Symbol;
struct           ShortSymbol;

/*
  If LABELSARECHAR is defined, labels are raw character handles
  directly attached to a Symbol.  Otherwise, labels are short CHARACTER
  symbols whose handles directly attached to Symbols.  The save() and
  restore() formats differ, too.

  Similarly, depending on whether NOTESARECHAR is or is not defined, notes
  are either raw character handles or short CHARACTER symbols, with differening
  save() and restore() formats.

  980721 Successful test runs both with and without LABELSARECHAR defined
*/  
	
#if (0)
#define LABELSARECHAR
#define NOTESARECHAR
#endif

#ifdef LABELSARECHAR
typedef char                LabelsType;
#else /*LABELSARECHAR*/
typedef struct ShortSymbol  LabelsType;
#endif /*LABELSARECHAR*/

#ifdef NOTESARECHAR
typedef char                NotesType;
#else /*NOTESARECHAR*/
typedef struct ShortSymbol  NotesType;
#endif /*NOTESARECHAR*/

typedef struct Symbol
{				/* symbol table entry */
	char            name[NAMELENGTH+1];
	long            type;	/* CHAR, REAL, BLTIN, LIST, STRUC, MACRO, UNDEF, LOGIC */
	union
	{
		double        **data;	/* for real data */
		char          **string;	/* for a character string, or MACRO */
		long          **longdata;  /* primarily for model parsing */
		whole_graph   **graph; /* for info on graph */
		struct Symbol **(*fptr) (struct Symbol **);	/* for a builtin function */
		struct Symbol ****Comp;
		/* for components of a structure
		    or elements of a list.  The difference
		    between a list and a structure is that
		    elements of a list are in the symbol
		    table while structure components are not
		*/
		char        ****garbage; /* for keeping track of temporary handles */
	}               value;
	struct Symbol **next;	/* next element in symbol table */
	struct Symbol **prev;	/* previous element , ie doubly linked list */
	long            nclass;	/* number of classes for factors */
	LabelsType **   labels; /* row, column etc. labels */
	NotesType  **   notes;  /* descriptive information */ 
	long            dim[MAXDIMS + 1];/* dimensions, dim[0] is number of dims */
} Symbol, **Symbolhandle;

/*
  Symbol type for one dimensional objects.  It is identical to Symbol
  except component dim is of length 2.  It should never be put in the
  symbol table but should be reserved for things to attach to other
  symbols like labels or notes
*/
typedef struct ShortSymbol
{				/* symbol table entry */
	char            name[NAMELENGTH+1];
	long            type;	/* CHAR, REAL, BLTIN, LIST, STRUC, MACRO, UNDEF, LOGIC, PLOTINFO */
	union
	{
		double        **data;	/* for real data */
		char          **string;	/* for a character string, or MACRO */
		long          **longdata;  /* primarily for model parsing */
		whole_graph   **graph; /* for info on graph */
		struct Symbol **(*fptr) (Symbol **);	/* for a builtin function */
		struct Symbol ****Comp;
		/* for components of a structure
		    or elements of a list.  The difference
		    between a list and a structure is that
		    elements of a list are in the symbol
		    table while structure components are not
		*/
		char        ****garbage; /* for keeping track of temporary handles */

	}               value;
	struct Symbol **next;	/* next element in symbol table */
	struct Symbol **prev;	/* previous element , ie doubly linked list */
	long            nclass;	/* number of classes for factors */
	LabelsType **   labels; /* row, column etc. labels */
	NotesType  **   notes;  /* descriptive information */ 
	long            dim[2];	/* dimensions, dim[0] should be 1 */
} ShortSymbol, **ShortSymbolhandle;

/*
   Type SpecialSymbol is identical to Symbol with the addition of two
   function pointers
*/

typedef struct SpecialSymbol
{				/* symbol table entry */
	char            name[NAMELENGTH+1];
	long            type;	/* CHAR, REAL, BLTIN, LIST, STRUC, MACRO, UNDEF, LOGIC */
	union
	{
		double        **data;	/* for real data */
		char          **string;	/* for a character string, or MACRO */
		long          **longdata;  /* primarily for model parsing */
		whole_graph   **graph; /* for info on graph */
		struct Symbol **(*fptr) (struct Symbol **);	/* for a builtin function */
		struct Symbol ****Comp;
		/* for components of a structure
		    or elements of a list.  The difference
		    between a list and a structure is that
		    elements of a list are in the symbol
		    table while structure components are not
		*/
		char        ****garbage; /* for keeping track of temporary handles */
	}               value;
	struct Symbol **next;	/* next element in symbol table */
	struct Symbol **prev;	/* previous element , ie doubly linked list */
	long            nclass;	/* number of classes for factors */
	LabelsType **   labels; /* row, column etc. labels */
	NotesType  **   notes;  /* descriptive information */ 
	long            dim[MAXDIMS + 1];/* dimensions, dim[0] is number of dims */
	struct Symbol **(*setSpecial) (struct Symbol **, long);
	struct Symbol **(*getSpecial) (struct Symbol **);
}               SpecialSymbol, **SpecialSymbolhandle;

/*
  Type for function symbols (that do not change).  Same as ShortSymbol for
  a storage saving of MAXDIMS-2 longs
*/

typedef struct ShortSymbol FunctionSymbol, **FunctionSymbolHandle;
typedef LabelsType  ** LabelsHandle;
typedef NotesType   ** NotesHandle;

#define ENDPREV ((Symbolhandle) -1)
#define BADHANDLE ((void **) -1)
enum badHandleCodes
{
	NULLHANDLE = -1,
	CORRUPTEDHANDLE = -2
};

/* The following assumes that the type of a symbol is an integer < 65536 */
#define TYPEMASK        0x0000ffffUL /*masks actual symbol type */
#define NONTYPEMASK     (~TYPEMASK)  /*selects remaining bits in Symbol.type*/
#define SPECIALMASK     0x00010000UL /* defines Special bit in Symbol.type */
#define NOMISSINGMASK   0x00020000UL /* defines NOMISSING bit in Symbol.type*/
#define SHORTSYMBOL     0x00040000UL /* define ShortSymbol bit in Symbol.type*/

#define DIM(A)  ((*(A))->dim)
#define DIMVAL(A,I) DIM(A)[I]  /*length of I-th dimension*/
#define COMPLETETYPE(A) ((*(A))->type)
#define TYPE(A) ((long)(COMPLETETYPE(A) & TYPEMASK)) /*type of symbol*/
#define NONTYPE(A) ((long)(COMPLETETYPE(A) & NONTYPEMASK))
#define NOMISSING(A) ((long)(COMPLETETYPE(A) & NOMISSINGMASK))
#define ISSHORT(A) ((long)(COMPLETETYPE(A) & SHORTSYMBOL))
#define NAME(A) ((*(A))->name)
#define NCLASS(A) ((*(A))->nclass)
#define NEXT(A) ((*(A))->next)
#define PREV(A) ((*(A))->prev)
#define LABELS(A) ((*(A))->labels)
#define NOTES(A) ((*(A))->notes)

/* The following two macros are used only when A is a SpecialSymbolhandle */
#define SETSPECIAL(A) ((*(A))->setSpecial)
#define GETSPECIAL(A) ((*(A))->getSpecial)

#define FPTR(A) ((*(A))->value.fptr)

#define COMP(A) ((*(A))->value.Comp)
#define COMPPTR(A) (*COMP(A))
#define COMPVALUE(A,I) (COMPPTR(A)[I])

#define DATA(A) ((*(A))->value.data)
#define DATAPTR(A) (*DATA(A))
#define DATAVALUE(A,I) (DATAPTR(A)[I])

#define STRING(A) ((*(A))->value.string)
#define STRINGPTR(A) (*STRING(A))
#define STRINGVALUE(A,I) (STRINGPTR(A)[I])

#define LONGDATA(A) ((*(A))->value.longdata)
#define LONGDATAPTR(A) (*LONGDATA(A))
#define LONGDATAVALUE(A,I) (LONGDATAPTR(A)[I])

#define GARBAGE(A) ((*(A))->value.garbage)
#define GARBAGEPTR(A) (*GARBAGE(A))
#define GARBAGEVALUE(A,I) (GARBAGEPTR(A)[I]) /* handle for I-th scratch var*/

#define GRAPH(A) ((*(A))->value.graph)  /* handle of graph */
#define GRAPHPTR(A) (*GRAPH(A))       /* pointer to graph */

#define HASLABELS(A) (LABELS(A) != (LabelsHandle) 0)
#define HASNOTES(A) (NOTES(A) != (NotesHandle) 0)

#ifdef LABELSARECHAR
#define LABELSHANDLE(A) (LABELS(A))
#else /*LABELSARECHAR*/
#define LABELSHANDLE(A) (STRING(LABELS(A)))
#endif /*LABELSARECHAR*/

#ifdef NOTESARECHAR
#define NOTESHANDLE(A)  (NOTES(A))
#else /*NOTESARECHAR*/
#define NOTESHANDLE(A)  (STRING(NOTES(A)))
#endif /*NOTESARECHAR*/

#define LABELSPTR(A) (*LABELSHANDLE(A))
#define NOTESPTR(A) (*NOTESHANDLE(A))

#define NDIMS(A) (DIM(A)[0])
#define NARGS(A) (DIM(A)[1])
#define NCOMPS(A) (DIM(A)[1])

#define isReal(A)  (getSingleType((Symbolhandle) A)==REAL) /* checks entire structure */
#define isLogic(A) (getSingleType((Symbolhandle) A)==LOGIC) /* checks entire structure */
#define isChar(A)  (getSingleType((Symbolhandle) A)==CHAR) /* checks entire structure */
#define isUndef(A) (((Symbolhandle) A) != (Symbolhandle) 0 &&\
					TYPE(A) == UNDEF)
#define isMacro(A) (((Symbolhandle) A) != (Symbolhandle) 0 &&\
					TYPE(A) == MACRO)
#define isGraph(A) (((Symbolhandle) A) != (Symbolhandle) 0 &&\
					TYPE(A) == PLOTINFO)
#define isStruc(A) (((Symbolhandle) A) != (Symbolhandle) 0 &&\
					TYPE(A) == STRUC)
#define isBltin(A) (((Symbolhandle) A) != (Symbolhandle) 0 &&\
					TYPE(A) == BLTIN)
#define isSpecial(A) (((Symbolhandle) A) != (Symbolhandle) 0 &&\
					  COMPLETETYPE(A) & SPECIALMASK)

#define setDATA(A,H)       (DATA(A)      =   (H))
#define setLONGDATA(A,H)   (LONGDATA(A)  =   (H))
#define setCOMP(A,H)       (COMP(A)      =   (H))
#define setGRAPH(A,H)      (GRAPH(A)     =   (H))
#define setSTRING(A,H)     (STRING(A)    =   (H))
#define setFPTR(A,H)       (FPTR(A)      =   (H))
#define setGARBAGE(A,H)    (GARBAGE(A)   =   (H))
#define setPREV(A,H)       (PREV(A)      =   (H))
#define setNEXT(A,H)       (NEXT(A)      =   (H))

#define setLABELS(A,H)     (LABELS(A)    =   (H))
#define setLABELSHANDLE(A,H) (LABELSHANDLE(A) = (H))
#define clearLABELS(A)     clearLabels(A)

#define setNOTES(A,H)      (NOTES(A)     =   (H))
#define clearNOTES(A)      clearNotes(A)
#define setNOTESHANDLE(A,H) (NOTESHANDLE(A) = (H))

/* The following two macros are used only when A is a SpecialSymbolhandle */
#define setSETSPECIAL(A,FNPTR) (SETSPECIAL(A) = (FNPTR))
#define setGETSPECIAL(A,FNPTR) (GETSPECIAL(A) = (FNPTR))

#define setNDIMS(A,N)    (NDIMS(A)    = (N))
#define setNCOMPS(A,N)   (NCOMPS(A)   = (N))
#define setDIM(A,I,N)    (DIM(A)[I]   = (N)) /*I = 1 means 1st subscript */

#define setTYPE(A,T)     (COMPLETETYPE(A) = (T))
#define setSPECIALTYPE(A,T) (COMPLETETYPE(A) = ((T) | SPECIALMASK))
#define setNONTYPE(A,T)  (COMPLETETYPE(A) = TYPE(A) | ((T) & NONTYPEMASK))
#define setNOMISSING(A)  (COMPLETETYPE(A) |= NOMISSINGMASK)
#define clearNOMISSING(A) (COMPLETETYPE(A) &= ~NOMISSINGMASK)
#define setNCLASS(A,N)   (NCLASS(A)   = (N))
#define setNAME(A,P)     Setname(A,P)
#define setInline(A, I)  (NCLASS(A) = (I) ? -1 : 0)
#define isInline(A)      (NCLASS(A) < 0)

/* op codes for SETSPECIAL */
enum setSpecialCodes
{
	CHECKLEFTSPECIAL  = 1, /* check whether it's o.k. to assign */
	CHECKRIGHTSPECIAL,     /* check suitability of right hand side */
	CLEARSPECIAL,          /* clear contents */
	ASSIGNSPECIAL          /* assign new value to */
};

/*
   950215  A new class of special symbols has been implemented.

   A special symbol specialSymh has the same properties as a regular symbol,
   except:

   (a)  specialSymh may not be in the Symbol table and its name must be
        recognizable by Findspecial().
   (b)  isSpecial(specialSymh) != 0
   (c)  Additional component getSpecial is function pointer to
        Symbolhandle GETSPECIAL(specialSymh)(Symbolhandle)
   (d)  Additional component setSpecial function pointer to
        Symbolhandle SETSPECIAL(specialSymh)(Symbolhandle, long)

   For most purposes a SymbolHandle will be used to refer to a SpecialSymbol.
   Although at present, the only SpecialSymbol is CLIPBOARD which is not
   in the Symbol table, probably eventually is should be in the symbol table.

   The argument to GETSPECIAL will normally be specialSymh and it
   will normally return specialSymh, updating its value if necessary.

   SETSPECIAL(specialSymh)(specialSymh, CHECKLEFTSPECIAL) should return a
   non-zero value if and only if assignment to the special symbol is legal

   SETSPECIAL(specialSymh)(arg, CHECKRIGHTSPECIAL) should return a non-zero
   value if and only if the value of arg is appropriate to be assigned to
   specialSymh

   SETSPECIAL(specialSymh)(arg, CLEARSPECIAL) clears the contents of
   specialSymh.  They should be replaced by something innocuous such
   as a null string.

   SETSPECIAL(specialSymh)(arg, ASSIGNSPECIAL) assigns the value of arg to
   specialSymh, possibly transforming it in the process.  If it is unable
   to do so it should return (Symbolhandle) 0

   As of now there is only one special symbol, CLIPBOARD.
*/

/*
   960307  Added handles for row and column labels to Symbol structures
*/

#define FAKESYMBOLCLASS   -32768
#define markFakeSymbol(S)    (setNCLASS(S,FAKESYMBOLCLASS),\
							  setTYPE(S, TYPE(S) | SHORTSYMBOL))
#define isFakeSymbol(S)      (NCLASS(S) == FAKESYMBOLCLASS)

/*
  Macros to make it easier to allocate scratch handles that will automatically
  be destroyed in case of an interrupt or aborted output.  The handles are
  saved in GARB Symbol with name 'TRASH__' which is assumed to be in the
  scratch symbol table and hence will be released at prompt time.  Typical
  usage of these macros is as follows:

#define GTMP   0
#define GY     1
#define NTRASH 2
. . . . .
		double    **y;
		long      **tmp;
		WHERE("name")
		TRASH(NTRASH,errorExit);  * allocates trashbasket for 2 handles *

		if(!getScratch(tmp,GTMP,n,long) ||
		   !getScratch(y,GY,n,double))
		{
			goto errorExit;
		}

  Before returning, use
		emptyTrash();

  If it turns out that y is not to be discarded, but, say, is to be returned
  as part of symbol result, then use
		unTrash(GY);
		DATA(result) = y;
		emptyTrash();
		return (result);
....
	  errorExit:
		emptyTrash();
		return (0);
*/

#define NAMEFORTRASH  TRASH__

/*
  Following macro MUST immediately follow declarations, including macro WHERE,
  before any executable statements.
  It declares scratch GARB Symbol TRASH__ with space for N handles,
  transferring to LABEL in case of error.
*/

#define TRASH(N,LABEL)	Symbolhandle  NAMEFORTRASH = GarbInstall(N);\
						if(NAMEFORTRASH == (Symbolhandle) 0)goto LABEL

/*
  allocate a handle of size LENGTH*sizeof(DATATYPE) to variable NAME and put
  it in slot INDEX of TRASH__.  getScratch returns a value of zero in case
  space cannot be allocated. The handle will automatically be freed when
  TRASH__ is deleted.
*/

#define getScratch(NAME,INDEX,LENGTH,DATATYPE) \
	(\
		GARBAGEVALUE(NAMEFORTRASH,INDEX) = mygethandle((LENGTH) * sizeof(DATATYPE)),\
		(NAME = (DATATYPE **) GARBAGEVALUE(NAMEFORTRASH,INDEX)) != (DATATYPE **) 0\
	)

/* add handle to items in trash */
#define toTrash(NAME,INDEX) GARBAGEVALUE(NAMEFORTRASH,INDEX) = (char **) (NAME)

/*
  Clear handle slot INDEX in TRASH__ so that deleting TRASH__ will not delete
  the handle in that slot
*/
#define unTrash(INDEX) 	GARBAGEVALUE(NAMEFORTRASH,INDEX) = (char **) 0

/*
  Immediately free space associated with handle NAME in slot INDEX of TRASH__
*/
#define discardScratch(NAME,INDEX,DATATYPE) (unTrash(INDEX),\
											 mydisphandle((char **) NAME),\
											 NAME = (DATATYPE **) 0)
/*
  Remove TRASH__ from the scratch symbol table and delete it, along with
  all the handles in its slots.  It has an argument solely so that it can
  be invoked as if it were a function.
*/
#define emptyTrash()	(Removesymbol(NAMEFORTRASH) , NAMEFORTRASH = (Symbolhandle) 0)

#endif /*SYMBOLH__*/
