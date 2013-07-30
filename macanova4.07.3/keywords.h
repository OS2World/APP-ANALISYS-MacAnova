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

#ifndef KEYWORDSH__
#define KEYWORDSH__

#undef EXTERN
#undef INIT
#undef INITDIM
#undef INITARRAY
#undef COMMA

#ifdef MAIN__
#define EXTERN
#define INIT(VAL) = VAL
#define INITDIM(N) N
#define INITARRAY(VALS) = { VALS }
#else
#define EXTERN extern
#define INIT(VAL)
#define INITDIM(N)
#define INITARRAY(VALS)
#endif /*MAIN__*/

#define COMMA ,

/* structures, enums, and defines related to keyword processing */
/*
   Typically there will be an array of struct keywordInfo and the values
   will be filled in by invoking getKeyValue for each element in the array
   Keywords whose values may be non-scalar or not REAL or LOGICAL should
   have their type initialized to SYMH.

   951103 CHAR type is now allowed.  The value is set to the handle.
   See glm.c for example.

   970711 Defines for various LONG types (used only by User()
   980407 Added {} around last component in InitKeyEntry macro
   980717 Added GRAPHVALUE define
   980724 Added component notes to struct plotKeyValues
*/
typedef struct keywordList
{
	char         *keyname; /* full keyword name */
	int           length; /* 0 means need exact match;otherwise match length */
	long          legalOps; /* mask for legal operations, 0 means all */
	long          type; /* REAL, LOGIC, LONG, or SYMH */
	union 
	{
		Symbolhandle   symhVal;
		long           intVal;
		long           logVal;
		double         realVal;
		char         **handleVal;
	} value;
} keywordList, *keywordListPtr, **keywordListHandle; 

#define NKeys(A)          (sizeof(A)/sizeof(keywordList))
/*
   Macro for initializing an entry in a keywordList[]
*/
#define InitKeyEntry(Name, Length, Ops, Type) {Name,Length,Ops,Type,{0L}}

/* Macros for accessing information in keywordList */
#define KeyValue(A,I)     (A[I].value)
#define KeySymhValue(A,I) (KeyValue(A,I).symhVal)
#define KeyIntValue(A,I)  (KeyValue(A,I).intVal)
#define KeyLogValue(A,I)  (KeyValue(A,I).logVal)
#define KeyRealValue(A,I) (KeyValue(A,I).realVal)
#define KeyHandleValue(A,I) (KeyValue(A,I).handleVal)
#define KeyCharValue(A,I) KeyHandleValue(A,I)
#define KeyType(A,I)      (A[I].type)
#define KeyOps(A,I)       (A[I].legalOps)
#define KeyName(A,I)      (A[I].keyname)
#define KeyLength(A,I)    (A[I].length)

/*
  The following defines of value types are duplicated in dynload.h
  If any change is made either here or in dynload.h, the defines must be
  kept the same.
*/

/*
  The following defines of value types are duplicated in Userfun.h
  If any change is made either here or in Userfun.h, the defines must be
  kept the same.
*/
#ifndef VALUETYPESDEFINED
#define VALUETYPESDEFINED

#if(!defined(WXWINMSW) && !defined(BCPP))
enum mvTypeShapeCodes
{
	REALVALUE       = 0x00001000UL,
	LOGICVALUE      = 0x00002000UL,
	CHARVALUE       = 0x00004000UL,
	SYMHVALUE       = 0x00008000UL,
	INTEGERTYPE     = 0x00010000UL,
	POSITIVETYPE    = 0x00020000UL,
	NONNEGATIVETYPE = 0x00040000UL,
	SCALARTYPE      = 0x00080000UL,
	VECTORTYPE      = 0x00100000UL,
	MATRIXTYPE      = 0x00200000UL,
	ARRAYTYPE       = 0x00400000UL,
	NONMISSINGTYPE  = 0x00800000UL,
	SQUARETYPE      = 0x01000000UL,
	STRUCTURETYPE   = 0x02000000UL,
	NULLSYMTYPE     = 0x04000000UL,
	LONGVALUE       = 0x08000000UL,
	GRAPHVALUE      = 0x10000000UL,
	REALSCALAR            = (REALVALUE|SCALARTYPE),
	NONMISSINGREAL        = (REALSCALAR|NONMISSINGTYPE),
	NONNEGATIVEREAL       = (NONMISSINGREAL|NONNEGATIVETYPE),
	POSITIVEREAL          = (NONMISSINGREAL|POSITIVETYPE),
	REALVECTOR            = (REALVALUE|VECTORTYPE),
	NONMISSINGREALVECTOR  = (REALVALUE|VECTORTYPE|NONMISSINGTYPE),
	NONNEGATIVEVECTOR     = (NONMISSINGREALVECTOR|NONNEGATIVETYPE),
	POSITIVEVECTOR        = (NONMISSINGREALVECTOR|POSITIVETYPE),
	REALVECTORORNULL      = (REALVECTOR|NULLSYMTYPE),
	REALMATRIX            = (REALVALUE|MATRIXTYPE),
	NONMISSINGREALMATRIX  = (REALVALUE|MATRIXTYPE|NONMISSINGTYPE),
	NONNEGATIVEMATRIX     = (NONMISSINGREALMATRIX|NONNEGATIVETYPE),
	POSITIVEMATRIX        = (NONMISSINGREALMATRIX|POSITIVETYPE),
	REALMATRIXORNULL      = (REALMATRIX|NULLSYMTYPE),
	REALARRAY             = (REALVALUE|ARRAYTYPE),
	NONMISSINGREALARRAY   = (REALVALUE|ARRAYTYPE|NONMISSINGTYPE),
	NONNEGATIVEARRAY      = (NONMISSINGREALARRAY|NONNEGATIVETYPE),
	POSITIVEARRAY         = (NONMISSINGREALARRAY|POSITIVETYPE),
	REALSTRUCTURE         = (REALVALUE|STRUCTURETYPE),
	INTSCALAR             = (NONMISSINGREAL|INTEGERTYPE),
	NONNEGATIVEINT        = (INTSCALAR|NONNEGATIVETYPE),
	POSITIVEINT           = (INTSCALAR|POSITIVETYPE),
	INTVECTOR             = (NONMISSINGREALVECTOR|INTEGERTYPE),
	NONNEGATIVEINTVECTOR  = (INTVECTOR|NONNEGATIVETYPE),
	POSITIVEINTVECTOR     = (INTVECTOR|POSITIVETYPE),
	INTMATRIX             = (NONMISSINGREALMATRIX|INTEGERTYPE),
	NONNEGATIVEINTMATRIX  = (INTMATRIX|NONNEGATIVETYPE),
	POSITIVEINTMATRIX     = (INTMATRIX|POSITIVETYPE),
	INTARRAY              = (NONMISSINGREALARRAY|INTEGERTYPE),
	NONNEGATIVEINTARRAY   = (INTARRAY|NONNEGATIVETYPE),
	POSITIVEINTARRAY      = (INTARRAY|POSITIVETYPE),
	LOGICSCALAR           = (LOGICVALUE|SCALARTYPE|NONMISSINGTYPE),
	LOGICVECTOR           = (LOGICVALUE|VECTORTYPE|NONMISSINGTYPE),
	LOGICMATRIX           = (LOGICVALUE|MATRIXTYPE|NONMISSINGTYPE),
	LOGICARRAY            = (LOGICVALUE|ARRAYTYPE|NONMISSINGTYPE),
	LOGICSTRUCTURE        = (LOGICVALUE|STRUCTURETYPE),
	CHARSCALAR            = (CHARVALUE|SCALARTYPE),
	CHARVECTOR            = (CHARVALUE|VECTORTYPE),
	CHARMATRIX            = (CHARVALUE|MATRIXTYPE),
	CHARARRAY             = (CHARVALUE|ARRAYTYPE),
	CHARSTRUCTURE         = (CHARVALUE|STRUCTURETYPE),
	LONGSCALAR            = (LONGVALUE|SCALARTYPE),
	NONNEGATIVELONG       = (LONGSCALAR|NONNEGATIVETYPE),
	POSITIVELONG          = (LONGSCALAR|POSITIVETYPE),
	LONGVECTOR            = (LONGVALUE|VECTORTYPE),
	NONNEGATIVELONGVECTOR = (LONGVECTOR|NONNEGATIVETYPE),
	POSITIVELONGVECTOR    = (LONGVECTOR|POSITIVETYPE),
	LONGMATRIX            = (LONGVALUE|MATRIXTYPE),
	NONNEGATIVELONGMATRIX = (LONGMATRIX|NONNEGATIVETYPE),
	POSITIVELONGMATRIX    = (LONGMATRIX|POSITIVETYPE),
	LONGARRAY             = (LONGVALUE|ARRAYTYPE),
	NONNEGATIVELONGARRAY  = (LONGARRAY|NONNEGATIVETYPE),
	POSITIVELONGARRAY     = (LONGARRAY|POSITIVETYPE),
	SQUAREMATRIX          = (MATRIXTYPE|SQUARETYPE),
	REALSQUAREMATRIX      = (REALVALUE|SQUAREMATRIX)
};
#else /*!defined(WXWINMSW) && !defined(BCPP)*/
#define REALVALUE       0x00001000UL
#define LOGICVALUE      0x00002000UL
#define CHARVALUE       0x00004000UL
#define SYMHVALUE       0x00008000UL
#define INTEGERTYPE     0x00010000UL
#define POSITIVETYPE    0x00020000UL
#define NONNEGATIVETYPE 0x00040000UL
#define SCALARTYPE      0x00080000UL
#define VECTORTYPE      0x00100000UL
#define MATRIXTYPE      0x00200000UL
#define ARRAYTYPE       0x00400000UL
#define NONMISSINGTYPE  0x00800000UL
#define SQUARETYPE      0x01000000UL
#define STRUCTURETYPE   0x02000000UL
#define NULLSYMTYPE     0x04000000UL
#define LONGVALUE       0x08000000UL
#define GRAPHVALUE      0x10000000UL

#define REALSCALAR            (REALVALUE|SCALARTYPE)
#define NONMISSINGREAL        (REALSCALAR|NONMISSINGTYPE)
#define NONNEGATIVEREAL       (NONMISSINGREAL|NONNEGATIVETYPE)
#define POSITIVEREAL          (NONMISSINGREAL|POSITIVETYPE)

#define REALVECTOR            (REALVALUE|VECTORTYPE)
#define NONMISSINGREALVECTOR  (REALVALUE|VECTORTYPE|NONMISSINGTYPE)
#define NONNEGATIVEVECTOR     (NONMISSINGREALVECTOR|NONNEGATIVETYPE)
#define POSITIVEVECTOR        (NONMISSINGREALVECTOR|POSITIVETYPE)
#define REALVECTORORNULL      (REALVECTOR|NULLSYMTYPE)

#define REALMATRIX            (REALVALUE|MATRIXTYPE)
#define NONMISSINGREALMATRIX  (REALVALUE|MATRIXTYPE|NONMISSINGTYPE)
#define NONNEGATIVEMATRIX     (NONMISSINGREALMATRIX|NONNEGATIVETYPE)
#define POSITIVEMATRIX        (NONMISSINGREALMATRIX|POSITIVETYPE)
#define REALMATRIXORNULL      (REALMATRIX|NULLSYMTYPE)

#define REALARRAY             (REALVALUE|ARRAYTYPE)
#define NONMISSINGREALARRAY   (REALVALUE|ARRAYTYPE|NONMISSINGTYPE)
#define NONNEGATIVEARRAY      (NONMISSINGREALARRAY|NONNEGATIVETYPE)
#define POSITIVEARRAY         (NONMISSINGREALARRAY|POSITIVETYPE)
#define REALSTRUCTURE         (REALVALUE|STRUCTURETYPE)

#define INTSCALAR             (NONMISSINGREAL|INTEGERTYPE)
#define NONNEGATIVEINT        (INTSCALAR|NONNEGATIVETYPE)
#define POSITIVEINT           (INTSCALAR|POSITIVETYPE)

#define INTVECTOR             (NONMISSINGREALVECTOR|INTEGERTYPE)
#define NONNEGATIVEINTVECTOR  (INTVECTOR|NONNEGATIVETYPE)
#define POSITIVEINTVECTOR     (INTVECTOR|POSITIVETYPE)

#define INTMATRIX             (NONMISSINGREALMATRIX|INTEGERTYPE)
#define NONNEGATIVEINTMATRIX  (INTMATRIX|NONNEGATIVETYPE)
#define POSITIVEINTMATRIX     (INTMATRIX|POSITIVETYPE)

#define INTARRAY              (NONMISSINGREALARRAY|INTEGERTYPE)
#define NONNEGATIVEINTARRAY   (INTARRAY|NONNEGATIVETYPE)
#define POSITIVEINTARRAY      (INTARRAY|POSITIVETYPE)

#define LOGICSCALAR           (LOGICVALUE|SCALARTYPE|NONMISSINGTYPE)
#define LOGICVECTOR           (LOGICVALUE|VECTORTYPE|NONMISSINGTYPE)
#define LOGICMATRIX           (LOGICVALUE|MATRIXTYPE|NONMISSINGTYPE)
#define LOGICARRAY            (LOGICVALUE|ARRAYTYPE|NONMISSINGTYPE)
#define LOGICSTRUCTURE        (LOGICVALUE|STRUCTURETYPE)

#define CHARSCALAR            (CHARVALUE|SCALARTYPE)
#define CHARVECTOR            (CHARVALUE|VECTORTYPE)
#define CHARMATRIX            (CHARVALUE|MATRIXTYPE)
#define CHARARRAY             (CHARVALUE|ARRAYTYPE)
#define CHARSTRUCTURE         (CHARVALUE|STRUCTURETYPE)

#define LONGSCALAR            (LONGVALUE|SCALARTYPE)
#define NONNEGATIVELONG       (LONGSCALAR|NONNEGATIVETYPE)
#define POSITIVELONG          (LONGSCALAR|POSITIVETYPE)

#define LONGVECTOR            (LONGVALUE|VECTORTYPE)
#define NONNEGATIVELONGVECTOR (LONGVECTOR|NONNEGATIVETYPE)
#define POSITIVELONGVECTOR    (LONGVECTOR|POSITIVETYPE)

#define LONGMATRIX            (LONGVALUE|MATRIXTYPE)
#define NONNEGATIVELONGMATRIX (LONGMATRIX|NONNEGATIVETYPE)
#define POSITIVELONGMATRIX    (LONGMATRIX|POSITIVETYPE)

#define LONGARRAY             (LONGVALUE|ARRAYTYPE)
#define NONNEGATIVELONGARRAY  (LONGARRAY|NONNEGATIVETYPE)
#define POSITIVELONGARRAY     (LONGARRAY|POSITIVETYPE)

#define SQUAREMATRIX          (MATRIXTYPE|SQUARETYPE)
#define REALSQUAREMATRIX      (REALVALUE|SQUAREMATRIX)
#endif /*!defined(WXWINMSW) && !defined(BCPP)*/
#endif /*VALUETYPESDEFINED*/

/* Values that indicate a keyword has not been set */
EXTERN long UNSETLOGICAL INIT(-1);/* value of LOGICAL keyword not set T or F */
EXTERN long UNSETLONG    INIT(-2079983620); /*arbitrary*/
EXTERN double UNSETREAL  INIT(HUGEDBL); /* value of REAL keyword not set */
#define UNSETCHAR    ((char) -1)
#define UNSETSHORT   ((short) -1)
#define UNSETPOINTER ((char *) 0)
#define UNSETHANDLE ((char **) 0)
#define UNSETSYMH   ((Symbolhandle) 0)

/* macros to initialize keyword values */
#define unsetLogical(X)     (X = UNSETLOGICAL)
#define unsetLong(X)        (X = UNSETLONG)
#define unsetReal(X)        (X = UNSETREAL)
#define unsetHandle(X)      (X = UNSETHANDLE)
#define unsetChar(X)        unsetHandle(X)
#define unsetSymh(X)        (X = UNSETSYMH)
#define setLogicalTrue(X)   (X = 1)
#define setLogicalFalse(X)  (X = 0)

/* macros to test whether value from keyword was set */
#define realIsSet(X)        (isMissing(X) || (X) != UNSETREAL)
#define longIsSet(X)        ((X) != UNSETLONG)
#define logicalIsSet(X)     ((X) != UNSETLOGICAL)
#define symhIsSet(X)        ((X) != UNSETSYMH)
#define handleIsSet(X)      ((X) != UNSETHANDLE)
#define charIsSet(X)        handleIsSet(X)

/*
  structure to encapsulate all values for graphics keywords
*/
typedef struct plotKeyValues
{
	char             title[TITLESIZE+2];
	char             xlab[XLABELSIZE+2];
	char             ylab[YLABELSIZE+2];

	double           xmin;
	double           ymin;
	double           xmax;
	double           ymax;
	
	unsigned short   xticklength;
	unsigned short   yticklength;
	
	Symbolhandle     xticks;
	Symbolhandle     yticks;

	/* LOGICAL flags */
	char             logx;
	char             logy;
	char             xaxis;
	char             yaxis;
	char             dumb;
	char             keep;

	char             show;
	char             add;
	char             pause;
	char             impulses;
	char             lines;
	char             landscape;

	char             ps;
	char             epsf;
	char             newFile;

	long             window;
	long             height;
	long             width;
	
	long             justify;
	long             linetype;
	double           thickness;

	/* file names */
	char            *file;
	char            *screendump;

	/* other */
	Symbolhandle     notes;
} plotKeyValues, *plotKeyValuesPtr;

/*
  Following to initialize members of PlotKeyValues to indicate
  the keyword is not legal
*/
EXTERN long DONTSETLOGICAL INIT(-2);
EXTERN long DONTSETLONG    INIT(-2079983621); /*arbitrary*/
EXTERN double DONTSETREAL  INIT(HUGEDBL/2.0);
#define DONTSETSHORT       ((unsigned short) -2) 
#define DONTSETPOINTER     ((char *) -2)
#define DONTSETSYMH        ((Symbolhandle) -2)
#define DONTSETCHAR        ((char) -2)

#undef	EXTERN
#undef	INIT
#undef	INITDIM
#undef	INITARRAY
#undef  COMMA

#endif /*KEYWORDSH__*/
