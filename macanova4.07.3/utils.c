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
#pragma segment Utils
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
#include "mainpars.h"
#include "plot.h"
#include "blas.h"
#include "keywords.h"
#include "mvsave.h"

/*
   This file contains various utilities widely used in many functions
   950803 Removed all references to very obsolete type MACRO1
   960711 Added function isTooBig()
   960828 Added function isTorF(), isInteger()
   970226 Modified decodeString()
   970618 Added checkBalance()
   970730 Added function isNumber()
   980312 Added checks for NOMISSING bit if NOMISSING != 0
   980315 Added anyNaN() (no op if HASNAN not defined
   980315 Added anyInfinite() (no op if HASINFINITY not defined
   980722 Added encodeItem() and decodeItem() and included "mvsave.h"
   980724 Changed return type of checkArgType() to int
   980826 Added entries for NEXTREP to opName() and tokenName()
   981007 Added n_th(n)
   990112 Modified copyField to make it possible to determine the
          size of the field copied
   990212 Replace putOUTSTR() by putErrorOUTSTR()
*/
/*
   Prototypes of functions in file
*/

/* functions to check properties of function arguments */
long argOK(Symbolhandle /*arg*/, long /*type*/, long /*argno*/);
long isTorF(Symbolhandle /*arg*/);
long isNumber(Symbolhandle /*symh*/, int /*kind*/);
long isInteger(Symbolhandle /*symh*/, int /*kind*/);
long isCharOrString(Symbolhandle /*symh*/);
long isNull(Symbolhandle /*arg*/);
long isDefined(Symbolhandle /*arg*/);
long isAssigned(Symbolhandle /*arg*/);
long isScalar(Symbolhandle /*arg*/);
long isVector(Symbolhandle /*arg*/);
long isMatrix(Symbolhandle /*arg*/, long * /*dim*/);
long isFactor(Symbolhandle /*arg*/);
char * isKeyword(Symbolhandle /*arg*/);
int checkArgType(Symbolhandle /*arg*/, char * /*what*/,
				 long /*targetType*/);
long checkSymmetry(double  * /*a*/, long /*nrows*/);

/* functions to get information about symbol dimensions*/
long symbolSize(Symbolhandle /*arg*/);
long isTooBig(long /*n1*/, long /*n2*/, size_t /*size*/);
void getDims(long * /*dims*/, Symbolhandle /*arg*/);

/* Functions for identifying and decoding keyword phrases*/

/*
   All moved to file keywords.c, 951222
*/

/* Functions for encoding and decoding save file identifiers */
char * encodeItem(char * /*root*/, int /*itemNumber*/, int /*itemType*/);
int decodeItem(char * /*itemCode*/, char /*root*/ [], int * /*itemNumber*/, 
			   int * /*itemType*/);

/* Miscellaneous functions for comparing and decoding strings */
long mystrncmp(char * /*s1*/, char * /*s2*/, int /*n*/);
double decodeString(char * /*string*/, char * /*seps*/, long * /*error*/);
char findBracket(long /*goalLevel*/, char /*leftbracket*/, 
				 unsigned char * /*inputString*/,
				 long * /*thisLevel*/, long * /*thisPlace*/, 
				 long * /*lastchar*/);
long copyField(char * /*line*/, char * /*outstr*/);
/* functions related to changing and restoring the default output formats */
long setFormat(char * /*format*/, long /*fmt*/[]);
void installFormat(long /*beforeDec*/, long  /*afterDec*/, long  /*fmtType*/);
void saveFormat(void);
void restoreFormat(void);

/* functions to test for MISSING, infinite and NaN values*/
int anyMissing(Symbolhandle /*a*/);
int anyNaN(Symbolhandle /*a*/);
int anyInfinite(Symbolhandle /*a*/);
int anyDoubleMissing(double * /*a*/, long /*n*/);
int anyDoubleNaN(double * /*a*/, long /*n*/);
int anyDoubleInfinite(double * /*a*/, long /*n*/);

/* function to step "odometer*" */
void stepOdometer(long ind [], long bounds[], long ndims, long base,
				  long reverse);

/* functions for setting symbol names */
void setScratchName(Symbolhandle /*symh*/);
void setCompName(Symbolhandle /*symh*/, char * /*name*/);

/* functions to return character representation of types, operations, tokens*/
char * typeName(long /*type*/);
char * opName(long /*op*/);
char * tokenName(long /*token*/);

/* functions to find start and end of non-white contents of string */
char * fieldStart(char * /*stringBuf*/);
void trimBuffer(char * /*buffer*/, unsigned long /*control*/);

/* functions to indent and center string for output */
long indentBuffer(char * /*buffer*/, int /*nplaces*/);
long centerBuffer(char * /*buffer*/, char * /*value*/, long /*length*/);

/* functions to aid in building line to be printed */
long formatChar(char * /*buffer*/, char * /*value*/,
				unsigned long /*control*/);
long formatDouble(char * /*buffer*/, double /*value*/,
				  unsigned long /*control*/);
long escapedOctal(unsigned char /*c*/, unsigned char * /*buffer*/);
char * n_th(long /*n*/);

/* functions for locating or copying elements in CHAR symbol*/
char * skipStrings(char * /*ch*/, long /*l*/);
char * copyStrings(char * /*in*/, char * /*out*/, long /*l*/);

/* functions to match string against pattern with wild card character(s)*/
short scanPat(char * /*string*/,long * /*matchType*/, char * /*pattern*/,
			  int /*checkName*/, long /*maxLength*/);
short matchName(char * /*name*/, long /*matchType*/, char * /*pattern*/);

/* functions related to time or time and date */
char * getTimeAndDate(void);
void getElapsedTime(double [2]);

/* functions for computing extremes of REAL symbol and filling
   REAL symbol with constant */
void symExtremes(Symbolhandle /*symh*/, double /*vec*/ [2]);
void symFill(Symbolhandle /*symh*/, double /*value*/);

/* functions to fill double array with constant or copy it */
void doubleFill(double * /*x*/, double /*value*/, long /*length*/);
void doubleCopy(double * /*from*/, double * /*to*/, long /*length*/);

/* functions for finding extremes of double and long vectors */
double doubleMin(double * /*x*/, long /*n*/);
double doubleMax(double * /*x*/, long /*n*/);
long longMin(long * /*x*/, long /*n*/);
long longMax(long * /*x*/, long /*n*/);

/* functions for converting double vector to long and long vector to double*/
void doubleToLong(double * /*in*/, long * /*out*/, long /*length*/);
void longToDouble(long * /*in*/, double * /*out*/, long /*length*/);

/*
   Check argument and print error message if needed.  It returns non-zero only
   if argument appears OK.

   In every case, argOK() returns 0 if arg == (Symbolhandle)  0 (missing
   argument) or if argument is not defined.

   When type != NULLSYM, argOK() returns 0 whenever DIMVAL(arg,1) == 0 (NULL
   argument).

   If type == NULLSYM, argOK() returns 1 if TYPE(arg) == NULLSYM or if
   DIMVAL(arg,1) != 0, regardless of type.  It returns 0 if DIMVAL(arg,1) == 0
   but TYPE(arg) != NULLSYM (should never happen). (allows NULL argument)

   If type > 0 && type != NULLSYM, argOK() returns 0 if TYPE(arg) != type
   (type is required)

   If type == 0, type is not checked.

   If type < 0, argOK() returns 0 if TYPE(arg) == abs(type). (-type is not
   permitted,
   
   If argno != 0 its value is included in the error message.
*/
long argOK(Symbolhandle arg, long type, long argno)
{
	long             reply = 0;

	if (arg == (Symbolhandle) 0)
	{
		noData(FUNCNAME, labs(argno));
	}
	else if (!isFakeSymbol(arg) && !myvalidhandle((char **) arg))
	{
		invalidSymbol(FUNCNAME, labs(argno));
	}
	else if (!isDefined(arg) || isNull(arg) &&
			 (type != NULLSYM || TYPE(arg) != NULLSYM))
	{
		undefArg(FUNCNAME,arg,labs(argno));
	}
	else if (type == NULLSYM)
	{
		reply = 1;
	}
	else if (type > 0 && TYPE(arg) != type || type < 0 && TYPE(arg) == -type)
	{ /* do not check type if type == 0 */
		badType(FUNCNAME,type,argno);
	}
	else
	{
		reply = 1;
	}
	return (reply);
} /*argOK()*/

/*
   Check symh to make sure it is a LOGICAL scalar whose value is not MISSING
   Added 960828
*/
long isTorF(Symbolhandle symh)
{
	return (symh != (Symbolhandle) 0 && TYPE(symh) == LOGIC &&
			symbolSize(symh) == 1 && !isMissing(DATAVALUE(symh,0)));
} /*isTorF()*/


/*
   Function to check symh to make sure it is a non MISSING REAL scalar 
   (kind == ANYVALUE), a positive scalar (kind == POSITIVEVALUE,
   a non-negative scalar (kind == NONNEGATIVEVALUE) or a negative scalar
   (kind == NEGATIVEVALUE)
   Added 970730
*/

long isNumber(Symbolhandle symh, int kind)
{
	double       value;
	
	return (isScalar(symh) && TYPE(symh) == REAL &&
			(value = DATAVALUE(symh, 0), !isMissing(value)) &&
			(kind == NONNEGATIVEVALUE && value >= 0.0 ||
			 kind == POSITIVEVALUE && value > 0.0 ||
			 kind == NEGATIVEVALUE && value < 0.0 || kind == ANYVALUE));
} /*isNumber()*/

/*
   Function to check symh to make sure it is a REAL scalar whose value is
   an unrestricted integer (kind == ANYVALUE), a positive integer
   (kind == POSITIVEVALUE), a non-negative integer (kind == NONNEGATIVEVALUE),
   or a negative integer (kind == NEGATIVEVALUE)
   Added 960828
*/

long isInteger(Symbolhandle symh, int kind)
{
	return (isNumber(symh, kind) &&
			DATAVALUE(symh, 0) == floor(DATAVALUE(symh, 0)));
} /*isInteger()*/

/*
   Function to check symh to make sure it is a CHARACTER scalar
   Added 960828
*/

long isCharOrString(Symbolhandle symh)
{
	return (isScalar(symh) && TYPE(symh) == CHAR);
} /*isCharOrString()*/

/*
	Note: This is not an explicit check for NULLSYM.  It is true if arg is 0
	or if arg is defined (not type UNDEF or ASSIGNED) and the first dimension
	is 0.  If arg is not 0 but is not defined, isNull() returns false.
*/
long isNull(Symbolhandle arg)
{
	/* NOTE calling program should check separately for UNDEF */
	return (arg == (Symbolhandle) 0 || (DIMVAL(arg,1) == 0 && isDefined(arg)));
} /*isNull()*/

/*
   Returns 1 if and only if the handle is non-null and the type is neither
   UNDEF or ASSIGNED (an ASSIGNED is an UNDEF that was added to the
   regular symbol table because it appeared on the l.h.s. of an assignment
*/
long isDefined(Symbolhandle arg)
{
	return (arg != (Symbolhandle) 0 && TYPE(arg) != UNDEF &&
			TYPE(arg) != ASSIGNED);
} /*isDefined()*/

/*
   return non-zero if and only if the only definition of arg is as the l.h.s.
   of a pending assignment.  A stack of pending assignments is built by
   Assign() in Symbol.c
*/
long isAssigned(Symbolhandle arg)
{
	return (arg != (Symbolhandle) 0 && isPendingAssign(arg));
} /*isAssigned()*/


/*
   return non-zero if and only if arg is a non-null REAL, CHARACTER or
   LOGICAL variable of length 1
*/

long isScalar(Symbolhandle arg)
{
	long       type;
	long       i, ndims;
	
	if (arg == (Symbolhandle) 0 ||
		(type = TYPE(arg)) != REAL && type != LOGIC && 
		type != CHAR && type != LONG)
	{
		return (0);
	}
	
	ndims = NDIMS(arg);
	for (i=1;i <= ndims; i++)
	{
		if (DIMVAL(arg,i) != 1)
		{
			return (0);
		}
	}
	return (1);
} /*isScalar()*/

/*
  Anything whose only dimension > 1 is its first is to be considered a vector
*/
long isVector(Symbolhandle arg)
{
	long         type;
	long         ndims, i;

	if (arg == (Symbolhandle) 0 ||
		(type = TYPE(arg)) != REAL && type != LOGIC &&
		type != CHAR && type != LONG)
	{
		return (0);
	}

	ndims = NDIMS(arg);
	for (i = 2; i <= ndims; i++)
	{
		if (DIMVAL(arg, i) > 1)
		{
			return (0);
		}
	} /*for (i = 2; i <= ndims; i++)*/
	return (1);

} /*isVector()*/

/*
  Anything with at most 2 dimensions > 1 is to be considered a matrix.

  An object with length 1 is considered 1 by 1.

  An vector of length n is considered to be a n by 1 matrix.

  A [1 x ] ... [x 1 x] m x [1 x ] ... [x 1 x] n [x 1 x ... x 1] object is
  considered to be an m by n matrix, where m>1,n>1.

  A m x 1 [x 1 ... x 1] object is considered to be a m by 1 object.

  A 1 x n [x 1 x . . . x 1] object is considered to be a 1 by n object

  The row and column dimensions are returned in dim[].
*/

long isMatrix(Symbolhandle arg, long * dim)
{
	long         type;
	long         ndims, i, m, mdims = 0;
	WHERE("isMatrix");

	if (arg == (Symbolhandle) 0 ||
		(type = TYPE(arg)) != REAL && type != LOGIC && type != CHAR &&
		type != LONG)
	{
		goto notMatrix;
	}

	ndims = NDIMS(arg);
	for (i = 1; i <= ndims; i++)
	{
		m = DIMVAL(arg, i);
		if (m > 1)
		{
			if (mdims == 2)
			{
				goto notMatrix;
			}
			if (dim != (long *) 0)
			{
				dim[mdims] = m;
			}
			mdims++;
		} /*if (m > 1)*/
	} /*for (i = 1; i <= ndims; i++)*/

	if (dim != (long *) 0)
	{
		if (mdims == 0)
		{
			dim[0] = dim[1] = 1;
		}
		else if (mdims == 1)
		{
			if (DIMVAL(arg, 1) > 1)
			{
				dim[1] = 1;
			}
			else
			{
				dim[1] = dim[0];
				dim[0] = 1;
			}
		}		
	} /*if (mdims == 1 && dim != (long *) 0)*/
	
	return (1);

  notMatrix:
	if (dim != (long *) 0)
	{
		dim[0] = dim[1] = 0;
	}
	return (0);
} /*isMatrix()*/

/*
  returns 0 if all MISSING or if there are non positive integer values
  returns NCLASS(symh) if NCLASS(symh) > 0
  returns -(max value) otherwise, i.e., -NCLASS(symh) if symh were strict
  factor
  It treats logical variable as having levels 1 (F) and 2(T)
*/

long isFactor(Symbolhandle symh)
{
	long            nclass = -1;
	long            n, i, type;
	double         *in;
	double          y;

	if (isVector(symh) &&
		((type = TYPE(symh)) == REAL || type == LOGIC))
	{
		if ((nclass = NCLASS(symh)) < 0)
		{ /* not a 'factor'; check to see if it could be */
			nclass = 0;
			n = DIMVAL(symh,1);
			in = DATAPTR(symh);
			for (i = 0; i < n; i++)
			{
				y = in[i];
				if (!isMissing(y))
				{
					if (type == REAL)
					{
						if (y != floor(y) || y < 1.0)
						{		/* not positive integer */
							nclass = 0;
							break;
						}
						if (y > nclass)
						{
							nclass = (long) y;
						}
					}
					else
					{ /* at least 1 non-missing */
						nclass = 2;
						break;
					}
				}/*if (!isMissing(y))*/
			} /*for (i = 0; i < n; i++)*/
			if (nclass == -1)
			{ /* all were missing */
				nclass = 0;
			}
			else
			{
				nclass = -nclass; /* value < 0 => not true factor */
			}
		} /*if ((nclass = NCLASS(symh)) < 0)*/
	}
	else
	{
		nclass = 0;
	}

	return (nclass);
} /*isFactor()*/

static char      TempKeywordName[NAMELENGTH+1];
/*
  returns a pointer to the keyword name Name if NAME(arg) is of the form
  '@@Name'.  If NAME(arg) is not of this form it return (char *) 0.
  980508 isKeyword() returns false for undefined arg
*/

char *isKeyword(Symbolhandle arg)
{
	char        *name;
	WHERE("isKeyword");

	if (!isDefined(arg) || *(name = NAME(arg)) != KEYPREFIX1 ||
		name[1] != KEYPREFIX2)
	{
		return (char *) 0;
	}

	strncpy(TempKeywordName, name+2, NAMELENGTH);
	TempKeywordName[NAMELENGTH] = '\0';

	return ((char *) TempKeywordName);
} /*isKeyword()*/

enum checkArgTypeCodes
{
	NOPROBLEM    = 0x0000,
	WRONGVALUE   = 0x0001,
	WRONGSHAPE   = 0x0002,
	OUTOFRANGE   = 0x0004,
	SOMEMISSING  = 0x0008,
	NONINTEGER   = 0x0010,
	NONSTRUCTURE = 0x0020
};


/*
  checkArgType() checks various characteristics of arg against type and
  value information encoded in targetType.  what is a descriptor for what is
  being checked, say a keyword name.  If the value is OK, arg is returned;
  otherwise 0 is returned.

  For any "qualified" REAL (POSITIVEINT, NONNEGATIVEINT, LONG,
  POSITIVEREAL, NONNEGATIVEREAL, NONMISSINGREAL), a MISSING
  value fails the test.

  970625 created by taking code out of checkAndGetKey() in keywords.c so
  as to make the facility more generally available

  970711 added checks for various LONG types (used by User())

  980602 made modifications for its use by keyvalue(); basically allowing
         for more possibilities such as targetType == VECTORTYPE, with
         not value type specified, or targetType = REALVALUE | POSITIVETYPE
         with no shape specified.

  980724 Changed return value from Symbolhandle to int
*/

static char *Articles[2] = {"a", "an"};

static char * article(char * word)
{
	return ((strchr("aeiou", word[0]) != (char *) 0) ?
			Articles[1] : Articles[0]);	
} /*isVowel()*/

int checkArgType(Symbolhandle arg, char *what, long targetType)
{
	long         argType = TYPE(arg);
	long         length;
	double      *values;
	long        *longValues;
	int          notOK = NOPROBLEM;
	int          notName = 0;
	double       value;
	long         i, dims[2];
	char        *valueName = NullString, *shapeName = NullString;
	char        *rangeName = NullString, *typeName = NullString;
	char        *outstr;
	char         valueError[20];
	WHERE("checkArgType");
	
	if (targetType & SYMHVALUE ||
		(targetType == NULLSYMTYPE) && argType == NULLSYM)
	{ /*ok*/
		return (1);
	} /*if (targetType & SYMHVALUE)*/

	if (targetType & STRUCTURETYPE)
	{
		if (argType != STRUC)
		{
			notOK |= NONSTRUCTURE;
		} /*if (argType != STRUC)*/
		else
		{
			long        ncomps = NCOMPS(arg), componentType;
			
			for (i = 0; i < ncomps; i++)
			{
				componentType = (COMPVALUE(arg, i) != (Symbolhandle) 0) ?
					TYPE(COMPVALUE(arg, i)) : -1;		
				if (!((targetType & NULLSYMTYPE) &&
					  componentType == NULLSYM) &&
					(targetType & REALVALUE) && (componentType != REAL) ||
					(targetType & LOGICVALUE) && (componentType != LOGIC) ||
					(targetType & LONGVALUE) && (componentType != LONG) ||
					(targetType & CHARVALUE) && (componentType != CHAR))
				{
					notOK = WRONGVALUE;
					break;
				}
			} /*for (i = 0; i < ncomps; i++)*/
		} /*if (argType != STRUC){}else{}*/
	} /*if (targetType & STRUCTURETYPE)*/
	else if ((targetType & NONMISSINGTYPE) &&
		(argType == REAL || argType == LOGIC) && anyMissing(arg))
	{
		notOK |= SOMEMISSING;
	}
	else if (!(targetType & NULLSYMTYPE) || argType != NULLSYM)
	{
		if ((targetType & REALVALUE) && (argType != REAL) ||
			(targetType & LOGICVALUE) && (argType != LOGIC) ||
			(targetType & LONGVALUE) && (argType != LONG) ||
			(targetType & CHARVALUE) && (argType != CHAR))
		{
			notOK = WRONGVALUE;
		}

		if ((targetType & SCALARTYPE) && !isScalar(arg) ||
			(targetType & VECTORTYPE) && !isVector(arg) ||
			(targetType & MATRIXTYPE) && !isMatrix(arg, dims) ||
			(targetType & SQUARETYPE) && dims[0] != dims[1] ||
			(targetType & ARRAYTYPE) && argType != REAL && argType != LOGIC &&
			argType != CHAR)
		{
			notOK |= WRONGSHAPE;
		}
	}
	
	if (notOK == NOPROBLEM  && argType != NULLSYM &&
		!(targetType & STRUCTURETYPE) &&
		(targetType & (REALVALUE | LONGVALUE)))
	{
		/* type and shape ok; check for legal values*/
		length = symbolSize(arg);
		if (argType == REAL)
		{
			values = DATAPTR(arg);
		}
		else
		{
			longValues = LONGDATAPTR(arg);
		}
		
		if (targetType & (POSITIVETYPE | NONNEGATIVETYPE))
		{
			value = (argType == REAL) ?
				doubleMin(values, length) : (double) longMin(longValues, length);
			if (value < 0.0 || (targetType & POSITIVETYPE) && value == 0)
			{
				notOK |= OUTOFRANGE;
			}
		} /*if (targetType & (POSITIVETYPE | NONNEGATIVETYPE))*/
		
		if (!(notOK & OUTOFRANGE) && (targetType & INTEGERTYPE))
		{
			for (i = 0; i < length; i++)
			{
				if (values[i] != floor(values[i]))
				{
					notOK |= NONINTEGER;
					break;
				}
			} /*for (i = 0; i < length; i++)*/
		} /*if (!(notOK & OUTOFRANGE) && (targetType & INTEGERTYPE))*/
	} /*if (notOK == NOPROBLEM)*/
	
	if (notOK == NOPROBLEM)
	{
		/* Everything is OK */
		return (1);
	}
	
	/* what[0] == '#' signals the call is from argvalue() */
	sprintf(valueError, "ERROR:%s ", (what[0] != '#') ?
			" value of" : NullString);
	if (what[0] == '#')
	{
		what++;
	}
	
	if (notOK & SOMEMISSING)
	{
		sprintf(OUTSTR,
				"%s%s %s", valueError, what,
				((targetType & SCALARTYPE) && isScalar(arg)) ?
				"is MISSING" : "has MISSING elements");
	} /*if (notOK & SOMEMISSING)*/
	else if (notOK & NONSTRUCTURE)
	{
		sprintf(OUTSTR,
				"%s%s is not a structure", valueError, what);
	}
	else if (targetType & SCALARTYPE)
	{
		if (targetType & CHARVALUE)
		{
			notCharOrString(what);
		}
		else if (targetType & LOGICVALUE)
		{
			notTorF(what);
		}
		else if (targetType & INTEGERTYPE)
		{
			if (targetType & POSITIVETYPE)
			{
				notPositiveInteger(what);
			}
			else if (targetType & NONNEGATIVETYPE)
			{
				notNonNegativeInteger(what);
			}
			else
			{
				notInteger(what);
			}
		}
		else if (targetType & LONGVALUE)
		{
			if (targetType & POSITIVETYPE)
			{
				notPositiveLong(what);
			}
			else if (targetType & NONNEGATIVETYPE)
			{
				notNonNegativeLong(what);
			}
			else
			{
				notLong(what);
			}
		}
		else
		{
			if (targetType & POSITIVETYPE)
			{
				notPositiveReal(what);
			}
			else if (targetType & NONNEGATIVETYPE)
			{
				notNonNegativeReal(what);
			}
			else if (targetType & REALVALUE)
			{
				notNumberOrReal(what);
			}
			else
			{
				sprintf(OUTSTR, "%s%s is not scalar", valueError, what);
			}
		}
	} /*else if (targetType & SCALARTYPE)*/
	else
	{ /*non-scalar, non-structure*/
		sprintf(OUTSTR, "%s%s is not ", valueError, what);
		outstr = OUTSTR + strlen(OUTSTR);
		if (targetType & INTEGERTYPE)
		{
			valueName = "integer";
			typeName = " integers";
		}
		else if (targetType & REALVALUE)
		{
			valueName = "REAL";
			typeName = " REALs";
		}
		else if (targetType & LOGICVALUE)
		{
			valueName = "LOGICAL";
		}
		else if (targetType & CHARVALUE)
		{
			valueName = "CHARACTER";
		}
		
		if (targetType & VECTORTYPE)
		{
			shapeName = " vector";
		}
		else if (targetType & MATRIXTYPE)
		{
			shapeName = (targetType & SQUARETYPE) ?
				" square matrix" : " matrix";
		}
		else if (targetType & ARRAYTYPE)
		{
			shapeName = " array";
		}
		else if (targetType & STRUCTURETYPE)
		{
			shapeName = " structure";
		}
		
		if (targetType & POSITIVETYPE)
		{
			rangeName = " positive";
		}
		else if (targetType & NONNEGATIVETYPE)
		{
			rangeName = " nonnegative";
		}
		if ((targetType & REALVALUE) && *shapeName)
		{
			typeName = (targetType & INTEGERTYPE) ? " integers" : " REALs";
		}
		
		if (*shapeName == '\0' && *rangeName == '\0' && *typeName == '\0')
		{
			sprintf(outstr, "%s", valueName);
		}
		else if (targetType & STRUCTURETYPE)
		{
			if (*valueName)
			{
				sprintf(outstr, "a %s of %s components", shapeName, valueName);
			}
			else
			{
				sprintf(outstr, "a %s", shapeName);
			}
		}
		else if (targetType & (LOGICVALUE | CHARVALUE))
		{
			if (*shapeName)
			{
				sprintf(outstr, "%s %s%s",
						article((*valueName) ? valueName : shapeName + 1),
						valueName, shapeName);
			}
			else
			{
				sprintf(outstr, "%s", valueName);
			}
		}
		else
		{
			/*
			  Must be REAL or have a shape but not range or type
			  Possible combinations are 
			  *rangeName *typeName *shapeName *valueName
			     No          No        No        Yes
				 Yes         Yes       No
				 Yes         Yes       Yes
				 No          No        Yes
				 No          Yes       No
				 No          Yes       Yes
			*/
			
			if (*rangeName || *typeName || *shapeName)
			{
				if (*shapeName && *typeName)
				{ /*NYY and YYY*/
					if (*rangeName)
					{
						sprintf(outstr, "%s%s of%s%s",
								article(shapeName + 1),
								shapeName, rangeName, typeName);
					}
					else
					{
						sprintf(outstr, "%s %s%s",
								article((*valueName) ?
										valueName : shapeName + 1),
								valueName, shapeName);
					}
				}
				else if (*shapeName)
				{ /*NNY*/
					/* must be just test of shape of unspecified type*/
					sprintf(outstr, "%s%s", article(shapeName + 1), shapeName);
				}
				else if (*rangeName)
				{ /*YYN*/
					sprintf(outstr - 1, "%s%s", rangeName, typeName);
				}
				else
				{ /*NYN*/
					sprintf(outstr - 1, "%s", typeName);
				}
			}
			else
			{ /* NNN*/
				sprintf(outstr, "%s", valueName);
			}
		}

		if (targetType & NULLSYMTYPE)
		{
			outstr += strlen(OUTSTR);
			sprintf(outstr, " or NULL");
		}
	}

	putErrorOUTSTR();	
	return (0);
	
} /*checkArgType()*/

/*
  971205 corrected problems with inaccurate error messages
*/
int checkBalance(unsigned char * line, char *name)
{
	unsigned char         c;
	long                  parenLevel = 0;
	long                  bracketLevel = 0;
	long                  braceLevel = 0;
	long                  place = 0;
	int                   inquotes = 0, escaped = 0;
	int                   isMacro = (name != (char *) 0);
	char                 *symptom;
	WHERE("checkBalance");

	while ((c = line[place++], c))
	{
		if (!inquotes)
		{
			if (c == '#')
			{/* skip to end of line or end of string */
				while((c = line[place++] , c) && !isNewline(c))
				{
					;
				}
				
				if(c == '\0')
				{
					break;
				}
			}
			else if (c == '{')
			{
				braceLevel++;
			}
			else if (c == '}')
			{
				braceLevel--;
			}
			else if (c == '[')
			{
				bracketLevel++;
			}
			else if (c == ']')
			{
				bracketLevel--;
			}
			else if (c == '(')
			{
				parenLevel++;
			}
			else if (c == ')')
			{
				parenLevel--;
			}
			else if (c == '"')
			{
				inquotes = 1;
			}
			if (bracketLevel < 0 || braceLevel < 0 || parenLevel < 0)
			{
				symptom = "too many";
				goto unbalanced;
			}
		} /*if (!inquotes)*/
		else
		{
			if (c == '\\')
			{
				escaped = !escaped;
			}
			else if (c == '"')
			{
				if (!escaped)
				{
					inquotes = 0;
				}
				escaped = 0;
			}
			else
			{
				escaped = 0;
			}
		} /*if (!inquotes){}else{}*/
	} /*while ((c = line[place++], c))*/

	if (!inquotes && !parenLevel && !bracketLevel && !braceLevel)
	{
		return (1);
	}
	
	if (inquotes)
	{
		c = '"';
		symptom = "unbalanced";
	} /*if (inquotes)*/
	else
	{
		symptom = "too few";

		if (parenLevel)
		{
			c = ')';
		}
		else if (bracketLevel)
		{
			c = ']';
		}
		else if (braceLevel)
		{
			c = '}';
		}
	} /*if (inquotes){}else{}*/
	
	/* fall through*/

  unbalanced:
	sprintf(OUTSTR, "ERROR: %s %c's in %s%s being expanded",
			symptom, c, (isMacro) ? "macro " : "string",
			(isMacro) ? name : NullString);
	putErrorOUTSTR();
	return (0);
} /*checkBalance()*/

#define SYMMETRYFUZZ  1e-7
/*
   Check the symmetry of a square matrix.  It returns non-zero if and only
   if |a[i,j]-a[j,i]| < SYMMETRYFUZZ * average(|a[*,*]|) all i and j
   It is assumed there are no MISSING values (caller must screen)
*/

long checkSymmetry(double *a, long nrows)
{
	double            sum, *aj = a, *aii, fuzz;
	long              i, j, k;
	
	sum = 0.0;

	for (j = 0; j < nrows; j++)
	{
		for (i = 0; i < nrows; i++)
		{
			sum += fabs(aj[i]);
		}
		aj += nrows;
	} /*for (j = 0; j < nrows; j++)*/

	if (sum == 0.0)
	{
		return (1);
	}

	fuzz = SYMMETRYFUZZ * (sum / (double) (nrows * nrows));
	aii = aj = a;
	for (j = 0; j < nrows; j++)
	{
		k = nrows;
		for (i = j+1; i < nrows; i++)
		{
			if (fabs(aj[i] - aii[k]) > fuzz)
			{
				return (0);
			}
			k += nrows;
		}
		aii += nrows+1;
		aj += nrows;
	}
	return (1);
} /*checkSymmetry()*/

/*
   Returns the product of the dimensions.  For a STRUC this will be the
   number of components
*/
long symbolSize(Symbolhandle arg)
{
	long totl;
	long i,n;

	if (!isDefined(arg) || TYPE(arg) == NULLSYM)
	{
		return (0);
	}

	n = NDIMS(arg);
	totl = DIMVAL(arg,1);
	for (i=2;i<=n;i++)
	{
		totl *= DIMVAL(arg,i);
	}
	return totl;
} /*symbolSize()*/

long isTooBig(long n1, long n2, size_t size)
{
#ifdef MAXHANDLESIZE
	return (MAXHANDLESIZE/n1 < n2*size);
#else /*MAXHANDLESIZE*/
	return (0);
#endif /*MAXHANDLESIZE*/
} /*isTooBig()*/

/*	
   Copy dimensions of a symbol.
   Set dims[0] to NDIMS(arg) and dims[i] to DIMVAL(arg,i),i=1,NDIMS(arg)
   Replaces dupDims() in earlier versions of MacAnova
*/
void getDims(long * dims, Symbolhandle arg)
{
	long       i, n;

	if (isNull(arg))
	{
		dims[0] = 0;
	}
	else
	{
		dims[0] = n = NDIMS(arg);
		for (i=1; i<=n; i++)
		{
			dims[i] = DIMVAL(arg,i);
		}
	}
} /*getDims()*/

/* 
   encodeItem() and decodeItem() are functions for encoding
   and decoding save file identifiers
*/

static char          CodedItem[ItemCodeLength + 1];

char * encodeItem(char * root, int itemNumber, int itemType)
{
	if (strlen(root) != 2 || itemNumber > EndItems || itemNumber < 0 ||
		itemType > 255 || itemType < 0)
	{
		return (char *) 0;
	}
	sprintf(CodedItem, "%s%02d_%02X", root, itemNumber, itemType);
	return (CodedItem);
} /*encodeItem()*/


int decodeItem(char * itemCode, char root [], int * itemNumber, int * itemType)
{
	char       *epc;
	long        lval;
	
	if (strlen(itemCode) != ItemCodeLength || itemCode[4] != '_')
	{
		return (0);
	}

	root[0] = itemCode[0];
	root[1] = itemCode[1];
	root[2] = '\0';

	itemCode += 2;
	lval = strtol(itemCode, &epc, 10);
	if (lval < 0 || epc != itemCode + 2)
	{
		return (0);
	}
	*itemNumber = lval;
	itemCode += 3;
	lval = strtol(itemCode, &epc, 16);
	if (lval < 0 || epc != itemCode + 2)
	{
		return (0);
	}
	*itemType = lval;
	return (1);
} /*decodeItem()*/

	
/*
   comparison of strings ignoring case
   If n > 0, match the first n characters (exact match if strlen(s1) < n and
   strlen(s2) < n.
   If n == 0, requires exact match.
   Return 0 if matched, otherwise > 0 or < 0 depending on collating order
 */
long mystrncmp(char *s1, char *s2, int n)
{
	int         c1, c2;
	int         i;
	WHERE("mystrncmp");

	for (i=0;n == 0 || i < n;i++)
	{
		c1 = s1[i];
		c2 = s2[i];
		c1 = (isupper(c1)) ? tolower(c1) : c1;
		c2 = (isupper(c2)) ? tolower(c2) : c2;
		if (c1 != c2)
		{
			return c1-c2;
		}
		else if (c1 == '\0')
		{
			return 0;
		}
	} /*for (i=0;n == 0 || i < n;i++)*/

#if (0) /* no longer check for trailing space or terminating null */
	c1 = s1[n];
	c2 = s2[n];
	return (((c1 == '\0' || isspace(c1)) && (c2 == '\0' || isspace(c2))) ?
			0 : 1);
#else
	return (0); /* matched through n characters */
#endif
} /*mystrncmp()*/

/*
	Decode string and check whether it is a number
	If string is a null string,   *error = EMPTYSTRING
	If the start is not a number, *error = NOTANUMBER
	If the start is a number, that number is returned
	  If (seps is NULL and next character is not '\0', '\n' or ' ')
	    or (any following character is not in seps) then *error =  TRAILINGSTUFF
	  else *error = OKNUMBER = 0
*/

enum decodeStringCodes
{
	OKNUMBER = 0,
	EMPTYSTRING,
	NOTANUMBER,
	TRAILINGSTUFF
};

double decodeString(char * string, char * seps, long * error)
{
	char        *eptr, c;
	double       value = mystrtod(string, &eptr);
	WHERE("decodeString");

	*error = (*string == '\0') ? EMPTYSTRING :
		((eptr == string) ? NOTANUMBER : OKNUMBER);

	if (!*error && *eptr != '\0' && *eptr != '\n')
	{
		if (seps == (char *) 0)
		{
			*error = (*eptr == ' ' || *eptr == '\t') ?
			  OKNUMBER : TRAILINGSTUFF;
		}
		else
		{
			while (strchr(seps, c = *eptr))
			{  /*skip separator characters*/
				eptr++;
			}
			*error = (*eptr == '\n' || *eptr == '\0') ?
			  OKNUMBER : TRAILINGSTUFF;
		}
	} /*if (!*error && *eptr != '\0' && *eptr != '\n')*/
	return (value);
} /*decodeString()*/

/*
  Find a matching right bracket or paren in inputString that matches
  leftbracket ('{', '(', or '[') at level goalLevel.
  It skips quoted material and comments starting with '#'
  *thisLevel is set to the achieved level, which should be goalLevel
  *thisPlace is set to the next position in inputString to be scanned
  If lastchar is not NULL it is set to the last non-blank character, not
  in quotes, before the final right bracket
*/
char findBracket(long goalLevel, char leftbracket, 
						unsigned char *inputString,
						long *thisLevel, long * thisPlace, long *lastchar)
{
	char             c, clast = '\0', lastc = '\0';
	char             rightbracket;
	long             inQuotes = 0, foundComment = 0;
	long             place = *thisPlace, level = *thisLevel;
	WHERE("skipBrackets");

	rightbracket = (leftbracket == '{') ? '}' : 
		((leftbracket == '(') ? ')' : ']');
	
	c = inputString[place];
	if (c != '\0')
	{
		while(level > goalLevel)
		{
			c = inputString[place];
			if (c == '\0')
			{
				break;
			}
			place++;
			if (!foundComment)
			{
				if (c == '"' && (!inQuotes || clast != '\\'))
				{
					inQuotes = !inQuotes;
				}

				clast = (inQuotes && c == '\\' && clast == c) ? '\0' : c;

				if (!inQuotes)
				{
					if (c == rightbracket)
					{
						level--;
					}
					else if (c == leftbracket)
					{
						level++;
					}
					else if (c == '#')
					{
						foundComment = 1;
					}
					else if (c != ' ' && c != '\t')
					{
						lastc = c;
					}
				} /*if (!inQuotes)*/
			} /*if (!foundComment)*/
			if (isNewline(c))
			{
				foundComment = 0;
			}
		} /*while(level > goalLevel)*/
	} /*if (c != '\0')*/

	*thisPlace = place;
	*thisLevel = level;
	if (lastchar)
	{
		/*
		  return the last character read;
		  if it was '\0', place still points to it
		*/
		*lastchar = lastc;
	}
	
	return (c);
} /*findBracket()*/

#undef DEL
#define DEL 0x7f

#ifdef MACINTOSH
#define skipChar(C) (isspace(C) || (C) == DEL || ((C) < ' '))
#else /*MACINTOSH*/
#define skipChar(C) (isspace(C) || (C) >= DEL || ((C) < ' '))
#endif /*MACINTOSH*/

/*
   Skip white space and non-printable characters;
   then copy all printable characters and skip following
   white space and non-printable characters

   Return the total number of characters skipped or copied
   so that line + number = next place to scan

   990112 Added following feature
    When outstr == (char *) 0, no copying is done but the number
    of characters (including '\n') that would be copied is returned
*/
long copyField(char * line, char * outstr)
{
	unsigned char    *line1 = (unsigned char *) line;
	unsigned char     c, *line2 = line1;
	int               doCopy = (outstr != (char *) 0);
	long              nc = 0;

	while((c = *line2) != '\0' && skipChar(c))
	{/*skip whitespace */
		line2++;
	}

	if (c == '\0')
	{
		return (0);
	}
	
	/* Found non-white space */
	while ((c = *line2) != '\0' && !skipChar(c))
	{
		if (doCopy)
		{
			*outstr++ = (char) c;
		}
		else
		{
			nc++;
		}
		line2++;
	} /*while ((c = *line2) != '\0' && !skipChar(c))*/

	if (doCopy)
	{
		*outstr = '\0';

		while((c = *line2) != '\0' && skipChar(c))
		{/*skip whitespace */
			line2++;
		}
		if (c == '\0')
		{
			line2++;
		}
	}
	else
	{
		nc++;
	}
	return ((doCopy) ? line2 - line1 : nc);
} /*copyField()*/

/*
   break out contents of quoted field in line, putting the field
   in outstr.
   commaOK should be 0 before first field in a line, non-zero otherwise
   seekQuote should be 1 if this is not a continuation line of a multiline
     quoted field
   
   return n if a closing quote is found, -n otherwise, where
   n = the number of characters scanned

   Space allocated for outstr should be at least strlen(line+1)
*/

long copyQuotedField(char *line, char *outstr, long commaOK,
					 long seekQuote, long *error)
{
	char          *line1 = line;
	int            ncommas = (commaOK) ? 0 : 1;
	char           c;
	WHERE("copyQuotedField");
	
	/* skip to the next quote */
	if (seekQuote)
	{
		while ((c = *line1) != '\0' && c != '"')
		{
			if (!isspace(c))
			{
				if (c == ',')
				{
					ncommas++;
				}
				else
				{
					*error = 1;
				}
			} /*if (!isspace(c))*/
			line1++;
		} /*while ((c = *line1) != '\0' && c != '"')*/

		if (ncommas > 1)
		{
			*error = 1;
		}
		if (c == '\0')
		{ /* no starting quote found */
			return (0);
		}
		line1++;
	} /*if (seekQuote)*/

	/* Found quote; now look for closing quote */
	while ((c = *line1) != '\0' && c != '"')
	{
		if (c == '\\')
		{
			c = *(++line1);
			if (c == '\0')
			{
				c = '\\';
				line1--;
			}
			else if (isdigit(c))
			{
				unsigned char      oct = c - '0';
				int                nc;

				for (nc = 1; nc < 3 && (c = *(++line1), isdigit(c)) && c < '8';
					 nc++)
				{
					oct = 8*oct + (c - '0');
				}
				c = (oct != 0) ? oct : ' ';
				if (oct == 0)
				{
					strcpy(OUTSTR,
						   "WARNING: \\000 in quoted field set to space");
					putErrorOUTSTR();
				}
				if (nc < 3 || *line1 == '\0')
				{
					line1--;
				}
			}
			else if (c == 'n')
			{
				c = '\n';
			}
			else if (c == 'r')
			{
				c = '\r';
			}
			else if (c == 't')
			{
				c = '\t';
			}
		} /*if (c == '\\')*/

		*outstr++ = c;
		line1++;
	} /*while ((c = *line1) != '\0' && c != '"')*/
	if (c == '"')
	{
		line1++;
	}
	*outstr = '\0';
	return ((c == '"') ? line1 - line : -(line1 - line));
} /*copyQuotedFields()*/

/*
  calculate the number of digits required to print positive integer N
*/
int nDigits(long N)
{
	int      ndigits = (N == 0) ? 1 : 0;
	
	while (N != 0)
	{
		N /= 10;
		ndigits++;
	}
	return (ndigits);
} /*nDigits()*/

/*
   Functions for controlling format of output

   setFormat() analyzes a format in character form, abstracting the
    field width, the precision or number of decimals, and the format type,
    'g' or 'f'
   installFormat() initializes DATAFMT, FLOATFMT, and FIXEDFMT
   saveFormat() saves current formatting parameters in static memory
   restoreFormat() restores formatting parameters saved by saveFormat()

   Typical usage:
   
   int     fmt[3];
   char   *cfmt = "17.10g"

   setFormat(cfmt, fmt);
   saveFormat();
   installFormat(fmt[0], fmt[1], fmt[2]);
   ....
     code to use format
   ....
   restoreFormat();

*/
/*
   Function to decode format strings
   Complete format "w.df" or "w.dg"
   If w is omitted, it is assumed to be d+7
   If d is omitted, it is assumed to be 6

   941205  modified to accept "fw.d" or "gw.d"

   950614  modified to limit maximum size of w and d and return negative
   value if limit operative.  
*/

long setFormat(char * format, long fmt[])
{
	char        *legalChars = "fg";
	char        *pc, *pwidth, *pdec;
	long         retvalue = 1;
	fmt[0] = fmt[1] = fmt[2] = 0;

	for (pc = legalChars;*pc;pc++)
	{ /* look for 'f' or 'g' at beginning */
		if (*pc == *format)
		{
			fmt[2] = (long) *format++;
			break;
		}
	}
	pwidth = format;
	while (isdigit(*format))
	{
		format++;
	} /*while (isdigit(*format))*/
	if (pwidth == format)
	{ /* no width specified */
		pwidth = (char *) 0;
	}

	if (*format == '.')
	{ /* found decimal point*/
		format++;
	}

	pdec = format;
	while (isdigit(*format))
	{
		format++;
	} /*while (isdigit(*format))*/
	if (format == pdec)
	{ /* number of digits not specified */
		fmt[1] = 6;
		pdec = (char *) 0;
	}

	if (*format != '\0')
	{
		for (pc = legalChars;*pc;pc++)
		{
			if (*pc == *format)
			{
				if (fmt[2] != 0)
				{				/* found 'f' or 'g' at start */
					return (0);
				}
				fmt[2] = (long) *pc;
				format++;
				break;
			} /*if (*pc == *format)*/
		} /*for (pc = legalChars;*pc;pc++)*/
	} /*if (*format != '\0')*/

	if (fmt[2] == 0 || *format != '\0')
	{
		return (0);
	}

	if (pdec != (char *) 0)
	{
		sscanf(pdec,"%ld",&fmt[1]);
	}
	if (pwidth != (char *) 0)
	{
		sscanf(pwidth,"%ld",&fmt[0]);
	}
	else
	{
		fmt[0] = fmt[1] + 7;
	}

	if (fmt[0] > MAXFMTWIDTH)
	{
		fmt[0] = MAXFMTWIDTH;
		retvalue = -1;
	}
	if (fmt[1] > MAXFMTDIGITS)
	{
		fmt[1] = MAXFMTDIGITS;
		retvalue -= 10;
	}
	
	return (retvalue);
} /*setFormat()*/

/*
   function to set globals BEFOREDEC, AFTERDEC, FMTTYPE, FIXEDFMT,
   FLOATFMT, DATAFMT, MISSINGFMT, MAXFIXED, MINFIXED
*/

void installFormat(long beforeDec, long  afterDec, long  fmtType)
{
	WHERE("installFormat");

	BEFOREDEC = beforeDec;
	AFTERDEC = afterDec;
	FMTTYPE = fmtType;
	sprintf(FIXEDFMT, " %%%ld.%ldf", BEFOREDEC, AFTERDEC);
	sprintf(FLOATFMT, " %%%ld.%ldg", BEFOREDEC, AFTERDEC);
	strcpy(DATAFMT, (FMTTYPE == 'f') ? FIXEDFMT : FLOATFMT);
	sprintf(MISSINGFMT, " %%%lds", BEFOREDEC);
	FIELDWIDTH = BEFOREDEC + 1;
#ifdef USEPOW
	MAXFIXED = pow(10.0, (double) (BEFOREDEC - AFTERDEC - 1));
#else /*USEPOW*/
	MAXFIXED = intpow(10.0, (double) (BEFOREDEC - AFTERDEC - 1));
#endif /*USEPOW*/
	MINFIXED = .0950001;
	if (AFTERDEC == 0)
	{
		MINFIXED *= 10.0;
		MAXFIXED *= 10.0;
	}
	if (MAXFIXED < MINFIXED)
	{
		MAXFIXED = -1.0;
	}
} /*installFormat()*/

/*
  970109 set SCREENWIDTH and SCREENLENGTH
*/

void installScreenDims(long width, long height)
{
	SCREENWIDTH = (width >= MINSCREENWIDTH) ? width : DEFAULTSCREENWIDTH;
	SCREENHEIGHT = (height == 0 || height >= MINSCREENHEIGHT) ?
		height : DEFAULTSCREENHEIGHT;
} /*installScreenDims()*/

static char      SaveFIXEDFMT[20], SaveFLOATFMT[20];
static char      SaveFMTTYPE;
static char      SaveMISSINGFMT[20];
static long      SaveFIELDWIDTH,  SaveAFTERDEC,  SaveBEFOREDEC;
static double    SaveMINFIXED, SaveMAXFIXED;
static long      SaveSCREENWIDTH, SaveSCREENHEIGHT;

/*
   function to save various format- and screen-related globals
   970109 added SCREENWIDTH and SCREENHEIGHT to list of things saved
*/
void saveFormat(void)
{
	strcpy(SaveMISSINGFMT, MISSINGFMT);
	strcpy(SaveFIXEDFMT, FIXEDFMT);
	strcpy(SaveFLOATFMT, FLOATFMT);
	SaveFIELDWIDTH = FIELDWIDTH;
	SaveFMTTYPE = FMTTYPE;
	SaveAFTERDEC = AFTERDEC;
	SaveBEFOREDEC = BEFOREDEC;
	SaveMINFIXED = MINFIXED;
	SaveMAXFIXED = MAXFIXED;
	SaveSCREENWIDTH = SCREENWIDTH;
	SaveSCREENHEIGHT = SCREENHEIGHT;

} /*saveFormat()*/

/*
   function to restore various format- and screen-related globals saved
   by saveFormat()
   970109 added SCREENWIDTH and screen HEIGHT
*/
void restoreFormat(void)
{
	FIELDWIDTH = SaveFIELDWIDTH;
	FMTTYPE = SaveFMTTYPE;
	AFTERDEC = SaveAFTERDEC;
	BEFOREDEC = SaveBEFOREDEC;
	MINFIXED = SaveMINFIXED;
	MAXFIXED = SaveMAXFIXED;
	strcpy(MISSINGFMT, SaveMISSINGFMT);
	strcpy(FIXEDFMT, SaveFIXEDFMT);
	strcpy(FLOATFMT, SaveFLOATFMT);
	strcpy(DATAFMT, (FMTTYPE == 'g') ? SaveFLOATFMT : SaveFIXEDFMT);
	SCREENWIDTH = SaveSCREENWIDTH;
	SCREENHEIGHT = SaveSCREENHEIGHT;
} /*restoreFormat()*/

/* Function to step "odometer" */
void stepOdometer(long ind [], long bounds[], long ndims, long base,
				  long reverse)
{
	long        i;
	
	if (reverse)
	{
		for(i=ndims-1;i >= 0;i--)
		{
			ind[i]++;
			if(ind[i] < bounds[i] + base)
			{
				break;
			}
			ind[i] = base;
		} /*for(i=ndims-1;i >= 0;i--)*/
	} /*if (reverse)*/
	else
	{
		for (i = 0; i < ndims; i++)
		{
			ind[i]++;
			if(ind[i] < bounds[i] + base)
			{
				break;
			}
			ind[i] = base;
		} /*for(i=ndims-1;i >= 0;i--)*/
	}
} /*stepOdometer()*/


/*
  Function to check for missing values in argument

  980312 modified to check for NOMISSING bit; see Symbol.h
         Eventually when places like subscript assignment have been
         updated, it will set the NOMISSING bit when no missing values
         are found.
*/

int anyMissing(Symbolhandle a)
{
	register long       n;
	register char      *string;

	if (!isNull(a))
	{
		n = symbolSize(a);
		if (TYPE(a) == REAL || TYPE(a) == LOGIC)
		{
#if (USENOMISSING)
			if (NOMISSING(a))
			{
				return (0);
			}
#endif /*USENOMISSING*/
			if (anyDoubleMissing(DATAPTR(a), n))
			{
				return (1);
			}
#if (USENOMISSING)
#if (1)
/*
  Activate this code only when safe.  That is when suitable checks have
  been added to subscript assignment and anywhere else needed
*/
			setNOMISSING(a);
#endif /*1*/
#endif /*USENOMISSING*/
		}
		else if (TYPE(a) == CHAR)
		{
			string = STRINGPTR(a);
			while (n-- > 0)
			{
				if (*string == '\0')
				{
					return (1);
				}
				string = skipStrings(string, 1);
			} /*while (n-- > 0)*/
		}
	} /*if (!isNull(a))*/

	return (0);
} /*anyMissing()*/

/*
  Function to check for NaN values in argument.  Any NaN values
  are replaced by MISSING.
  Added 980315
*/

int anyNaN(Symbolhandle a)
{
	int                 foundNaN = 0;
#ifdef HASNAN
	register long       n;
	register double    *x;

	if (!isNull(a) && (TYPE(a) == REAL || TYPE(a) == LOGIC))
	{
		n = symbolSize(a);
		x = DATAPTR(a);
		while (n-- > 0)
		{
			double           val = *x;

			if (isNaN(val))
			{
				foundNaN = 1;
				setMissing(*x);
			}
			x++;
		} /*while (n-- > 0)*/
#if (USENOMISSING)
		if (foundNaN)
		{
			clearNOMISSING(a);
		}
#endif /*USENOMISSING*/
#endif /*HASNAN*/
	} /*(!isNull(a) && (TYPE(a) == REAL || TYPE(a) == LOGIC))*/
	return (foundNaN);
} /*anyNaN()*/

/*
  Function to check for Infinite values in argument.  Any infinite values
  are replaced by MISSING.
  Added 980315
*/

int anyInfinite(Symbolhandle a)
{
	int                 foundInfinite = 0;
#ifdef HASINFINITY
	register long       n;
	register double    *x;

	if (!isNull(a) && (TYPE(a) == REAL || TYPE(a) == LOGIC))
	{
		n = symbolSize(a);
		x = DATAPTR(a);
		while (n-- > 0)
		{
			double              val = *x;

			if (isInfinite(val))
			{
				foundInfinite = 1;
				setMissing(*x);
			}
			x++;
		} /*while (n-- > 0)*/
#if (USENOMISSING)
		if (foundInfinite)
		{
			clearNOMISSING(a);
		}
#endif /*USENOMISSING*/
	} /*(!isNull(a) && (TYPE(a) == REAL || TYPE(a) == LOGIC))*/
#endif /*HASINFINITY*/
	return (foundInfinite);
} /*anyInfinite()*/

int anyDoubleMissing(double * x, long n)
{
	while (n-- > 0)
	{
		if (isMissing(*x++))
		{
			return (1);
		}
	}
	return (0);
} /*anyDoubleMissing()*/

int anyDoubleNaN(double * x, long n)
{
#ifdef HASNAN
	while (n-- > 0)
	{
		if (isNaN(*x++))
		{
			return (1);
		}
	}
#endif /*HASNAN*/
	return (0);
} /*anyDoubleNaN()*/

int anyDoubleInfinite(double * x, long n)
{
#ifdef HASINFINITY
	while (n-- > 0)
	{
		if (isInfinite(*x++))
		{
			return (1);
		}
	}
#endif /*HASINFINITY*/
	return (0);
} /*anyDoubleInfinite()*/

/*
  Set a name appropriate for type and shape for a scratch variable
  970629 fixed bug that caused problems with new types (LONG) by
  adding default to switch
  */
void setScratchName(Symbolhandle symh)
{
	char          *name, newname[NAMELENGTH + 1];
	int            type;

	if (symh != (Symbolhandle) 0 && isDefined(symh) && isscratch(NAME(symh)))
	{
		type = TYPE(symh);
		if (type != MACRO)
		{
			switch (NDIMS(symh))
			{
			  case 1:
				if (type == STRUC)
				{
					name = STRUCSCRATCH;
				}
				else if (type == PLOTINFO)
				{
					name = GRAPHSCRATCH;
				}
				else if (type == NULLSYM)
				{
					name = NULLSCRATCH;
				}
				else if (DIMVAL(symh,1) == 1)
				{
					switch (type)
					{
					  case CHAR:
						name = STRINGSCRATCH;
						break;

					  case REAL:
						name = NUMSCRATCH;
						break;

					  case LOGIC:
						name = LOGICSCRATCH;
						break;
						
					  default:
						name = SCRATCH;
					} /* switch(type) */
				}
				else
				{
					name = VECSCRATCH;
				}
				break;

			  case 2:
				name = MATSCRATCH;
				break;

			  default:
				name = ARRAYSCRATCH;
			} /* switch(ndims) */
		} /*if (type != MACRO)*/
		else
		{
			name = MACROSCRATCH;
		}

		if (invisname(NAME(symh)))
		{
			strncpy(newname, INVISSCRATCH, 3);
			strncpy(newname+3, name+2, NAMELENGTH-3);
			newname[NAMELENGTH] = '\0';
			name = newname;
		}
		setNAME(symh,name);
	} /*if(symh!=(Symbolhandle)0&&isDefined(symh)&&isscratch(NAME(symh)))*/
} /*setScratchName()*/

void setScratchMacroName(Symbolhandle symh, char * macroName)
{
	char    name[NAMELENGTH+1];

	if (isscratch(macroName))
	{
		macroName += 2;
	}
	if (macroName[0] != '\0')
	{
		name[0] = SCRATCHPREFIX1;
		name[1] = SCRATCHPREFIX2;
		strncpy(name + 2, macroName, NAMELENGTH - 2);
		name[NAMELENGTH] = '\0';
	}
	else
	{
		strcpy(name, MACROSCRATCH);
	}
	setNAME(symh, name);
} /*setScratchMacroName()*/


/* set component names */
void setCompName(Symbolhandle symh, char * name)
{
	int            type;
	WHERE("setCompName");

	if (symh != (Symbolhandle) 0)
	{
		if (strcmp(name, LOGICSCRATCH + 2) == 0 ||
			strcmp(name, NUMSCRATCH + 2) == 0 ||
			strcmp(name, NULLSCRATCH + 2) == 0 ||
			strcmp(name, STRINGSCRATCH + 2) == 0 ||
			strcmp(name, SCALSCRATCH + 2) == 0 ||
			strcmp(name, VECSCRATCH + 2) == 0 ||
			strcmp(name, MATSCRATCH + 2) == 0 ||
			strcmp(name, ARRAYSCRATCH + 2) == 0 ||
			strcmp(name, MACROSCRATCH + 2) == 0 ||
			strcmp(name, GRAPHSCRATCH + 2) == 0 ||
			strcmp(name, STRUCSCRATCH + 2) == 0 ||
			strcmp(name, NULLNAME) == 0 ||
			strcmp(name, NAME(NULLSYMBOL)) == 0 && TYPE(symh) == NULLSYM)
		{
			name = SCRATCH;
		}

		if (isscratch(name))
		{
			type = TYPE(symh);
			switch (type)
			{
			  case STRUC:
				name = STRUCSCRATCH+2;
				break;

			  case MACRO:
				name = MACROSCRATCH+2;
				break;

			  case PLOTINFO:
				name = GRAPHSCRATCH+2;
				break;

			  default:
				if (NDIMS(symh) == 0 || DIMVAL(symh,1) == 0)
				{
					name = "NULL";
				}
				else if (NDIMS(symh) == 1)
				{
					if (DIMVAL(symh,1) == 1)
					{
						switch (type)
						{
						  case CHAR:
							name = STRINGSCRATCH+2;
							break;

						  case REAL:
							name = NUMSCRATCH+2;
							break;

						  case LOGIC:
							name = LOGICSCRATCH+2;
							break;
						} /* switch(type) */
					}
					else
					{
						name = VECSCRATCH+2;
					}
				}
				else if (NDIMS(symh) == 2)
				{
					name = MATSCRATCH+2;
				}
				else
				{
					name = ARRAYSCRATCH+2;
				}
			} /* switch(type) */
		} /*if (isscratch(name))*/
		else if (iskeyname(name))
		{
			name += 2;
		}
		else if (istempname(name))
		{
			name++;
		}
		setNAME(symh,name);
	} /*if (symh != (Symbolhandle) 0)*/
} /*setCompName()*/

static char TempName[36];

/* 980721 added SHORT{CHAR,REAL,LONG} */
char *typeName(long type)
{
	if (type & SHORTSYMBOL)
	{
		type &= TYPEMASK;

		switch ((int) type)
		{
		  case CHAR:
			return ("SHORTCHAR");
		  case REAL:
			return ("SHORTREAL");
		  case LONG:
			return ("SHORTLONG");
		  default:
			sprintf(TempName,"unknown short type = %ld",type | SHORTSYMBOL);
			return ((char *) TempName);
		}
	} /*if (type & SHORTSYMBOL)*/
	switch ((int) type)
	{
	  case REAL:
		return ("REAL");
	  case LOGIC:
		return ("LOGIC");
	  case CHAR:
		return ("CHAR");
	  case LONG:
		return ("LONG");
	  case GARB:
		return ("GARBAGE");
	  case LIST:
		return("LIST");
	  case BLTIN:
		return ("BLTIN");
	  case NULLSYM:
		return ("NULL");
	  case MACRO:
		return ("MACRO");
	  case STRUC:
		return ("STRUC");
	  case PLOTINFO:
		return ("GRAPH");
	  case UNDEF:
		return ("UNDEF");
	  case ASSIGNED:
		return ("ASSIGNED");
	  default:
		sprintf(TempName,"unknown type = %ld",type);
		return ((char *) TempName);
	} /*switch ((int) type)*/
} /*typeName()*/

static char    TempOpName[36];

char *opName(long op)
{
	char    *name;

	switch ((int) op)
	{
	  case ADD:
	  case '+':
		name = "+";
		break;
	  case SUB:
	  case '-':
		name = "-";
		break;
	  case MULT:
		name = "*";
		break;
	  case MATMULT:
		name = "%*%";
		break;
	  case TRMATMULT:
		name = "%c%";
		break;
	  case MATMULTTR:
		name = "%C%";
		break;
	  case MATDIV:
		name = "%/%";
		break;
	  case DIVMAT:
		name = "%\\%";
		break;
#ifdef BITOPS
	  case BITNOT:
		name = "%!";
		break;
	  case BITAND:
		name = "%&";
		break;
	  case BITXOR:
		name = "%^";
		break;
	  case BITOR:
		name = "%|";
		break;
#endif /*BITOPS*/
	  case DIV:
		name = "/";
		break;
	  case MOD:
		name = "%%";
		break;
	  case EXP:
		name = "^";
		break;
	  case EQ:
		name = "==";
		break;
	  case NE:
		name = "!=";
		break;
	  case LE:
		name = "<=";
		break;
	  case GE:
		name = ">=";
		break;
	  case LT:
		name = "<";
		break;
	  case GT:
		name = ">";
		break;
	  case AND:
		name = "&&";
		break;
	  case OR:
		name = "||";
		break;
	  case NOT:
		name = "!";
		break;
	  case ASSIGNADD:
		name = "<-+";
		break;
	  case ASSIGNSUB:
		name = "<--";
		break;
	  case ASSIGNMULT:
		name = "<-+*";
		break;
	  case ASSIGNDIV:
		name = "<-/";
		break;
	  case ASSIGNPOW:
		name = "<-^";
		break;
	  case ASSIGNMOD:
		name = "<-%%";
		break;
	  case IF:
		name = "if";
		break;
	  case ELSEIF:
		name = "elseif";
		break;
	  case ELSE:
		name = "else";
		break;
	  case WHILE:
		name = "while";
		break;
	  case FOR:
		name = "for";
		break;
	  case NEXTREP:
		name = "next";
		break;
	  default:
		sprintf(TempOpName,"unknown operation %ld",op);
		return ((char *) TempOpName);
	} /*switch ((int) op)*/
	strcpy((char *) TempOpName, name);
	return ((char *) TempOpName);
} /*opName()*/


char *tokenName(long token)
{
	switch ((int) token)
	{
#if (0) /*these are now pure type names and are never tokens*/
	  case REAL:
	  case LOGIC:
	  case CHAR:
	  case LIST:
	  case BLTIN:
	  case MACRO:
	  case STRUC:
	  case UNDEF:
	  case ASSIGNED:
	  case NULLSYM:
		return (typeName(token));
#endif /*0*/
	  case BLTINTOKEN:
		return("BLTINTOKEN");
	  case MACROTOKEN:
		return("MACROTOKEN");
	  case VAR:
		return("VAR");
	  case END:
		return("END");
	  case NUMBER:
		return("NUMBER");
	  case IF:
		return("IF");
	  case ELSE:
		return("ELSE");
	  case ELSEIF:
		return("ELSEIF");
	  case WHILE:
		return("WHILE");
	  case FOR:
		return("FOR");
	  case BREAK:
		return("BREAK");
	  case BREAKALL:
		return("BREAKALL");
	  case NEXTREP:
		return("NEXTREP");
	  case BATCH:
		return("BATCH");
	  case ERROR:
		return("ERROR");
	  case FATALERROR:
		return("FATALERROR");
	  case LEFTANGLE:
		return("LEFTANGLE");
	  case RIGHTANGLE:
		return("RIGHTANGLE");
	  case LEFTBRACK2:
		return("LEFTBRACK2");
	  case RIGHTBRACK2:
		return("RIGHTBRACK2");
	  case MULT:
		return("MULT");
	  case ADD:
		return("ADD");
	  case DIV:
		return("DIV");
	  case SUB:
		return("SUB");
	  case MOD:
		return ("MOD");
#ifdef BITOPS
	  case BITNOT:
		return ("BITNOT");
	  case BITAND:
		return ("BITAND");
	  case BITXOR:
		return ("BITXOR");
	  case BITOR:
		return ("BITOR");
#endif /*BITOPS*/
	  case EXP:
		return("EXP");
	  case GE:
		return("GE");
	  case LE:
		return("LE");
	  case GT:
		return("GT");
	  case LT:
		return("LT");
	  case EQ:
		return("EQ");
	  case NE:
		return("NE");
	  case AND:
		return("AND");
	  case OR:
		return("OR");
	  case NOT:
		return("NOT");
	  case ENDOFL:
		return("ENDOFL");
	  case POW:
		return("POW");
	  case MATMULT:
		return("MATMULT");
	  case TRMATMULT:
		return("TRMATMULT");
	  case MATMULTTR:
		return("MATMULTTR");
	  case MATDIV:
		return("MATDIV");
	  case DIVMAT:
		return("DIVMAT");
	  case ASSIGN:
		return("ASSIGN");
	  case ASSIGN1:
		return("ASSIGN1");
	  case ASSIGNADD:
		return ("ASSIGNADD");
	  case ASSIGNSUB:
		return ("ASSIGNSUB");
	  case ASSIGNMULT:
		return ("ASSIGNMULT");
	  case ASSIGNDIV:
		return ("ASSIGNDIV");
	  case ASSIGNPOW:
		return ("ASSIGNPOW");
	  case ASSIGNMOD:
		return ("ASSIGNMOD");
	  case SEMI1:
		return ("SEMI1");
	  case INLINEHEAD:
		return ("INLINEHEAD");
	  case OUTLINEHEAD:
		return ("OUTLINEHEAD");
	  case EVALHEAD:
		return ("EVALHEAD");
	  case BADTOKEN:
		return("BADTOKEN");

	  default:
		if (token > ' ' && token <= '~')
		{
			sprintf(TempName,"'%c'", (int) token);
		}
		else
		{
			sprintf(TempName,"%02x", (int) token);
		}

		return ((char *) TempName );
	} /*switch ((int) token)*/
} /*tokenName()*/

/* Return the address of the first nonwhite character in stringBuf */
char *fieldStart(char *stringBuf)
{
	while (isspace(*stringBuf))
	{
		stringBuf++;
	}
	return (stringBuf);
} /*fieldStart()*/

#if  (0)
/*   Macros defined in globkb.h*/
#define DODEFAULT   0x00  /* use default format */
#define DOFIXED     0x01  /* use fixed point format */
#define DOFLOAT     0x02  /* use floating point format */
#define TRIMRIGHT   0x04  /* trim whitespace on right end */
#define TRIMLEFT    0x08  /* trim whitespace on left end */

   formatChar() and formatDouble() are to aid in building up a string
   buffer for output, using the default formats.

   Typical usage might be
		outstr = OUTSTR;
		outstr += formatChar(outstr, "F-statistic = ", TRIMRIGHT);
		outstr += formatDouble(outstr, fStat, DOFIXED | TRIMRIGHT);
		putOUTSTR();
   If control & TRIMRIGHT is non-zero, white space on the right is squeezed
   out.
   If control & TRIMLEFT is non-zero, white space on the left is squeezed
   out.
   It is assumed there is room in buffer for the formatted item before
   any trimming
#endif /*if (0)*/

void trimBuffer(char *buffer, unsigned long control)
{
	char      *pc;
	
	if (control & TRIMLEFT)
	{
		strcpy(buffer, fieldStart(buffer));
	}
	if (control & TRIMRIGHT)
	{
		pc = buffer + strlen(buffer);
		while (--pc > buffer && isspace(*pc))
		{
			*pc = '\0';
		}
	} /*if (control & TRIMRIGHT)*/
} /*trimBuffer()*/

long indentBuffer(char *buffer, int nplaces)
{
	int        i;

	nplaces = (nplaces > 0) ? nplaces : 0;
	for (i = 0; i < nplaces; i++)
	{
		buffer[i] = ' ';
	}
	return ((long) nplaces);
} /*indentBuffer()*/
	
/*
   center value in buffer of length length
*/
long centerBuffer(char *buffer, char *value, long length)
{
	strcpy(buffer + indentBuffer(buffer, (length - strlen(value) + 1)/2),
		   value);
	return (strlen(buffer));
} /*centerBuffer()*/

long formatChar(char *buffer, char *value, unsigned long control)
{

	if (!(control & CHARASIS))
	{
		sprintf(buffer, MISSINGFMT, value);
		if (control & (TRIMLEFT | TRIMRIGHT))
		{
			trimBuffer(buffer, control);
		}
	}
	else
	{
		strcpy(buffer, value);
	}
	return (strlen(buffer));
} /*formatChar()*/

/*
   formatDouble() still needs work.

   If control & (DOFIXED | DOFLOAT) != 0), the intent is that it
     should use FIXEDFMT or FLOATFMT if that will format to a string
	 with length < FIELDWIDTH.   If its length >= FIELDWIDTH, it
	 should find a suitable format that will fit in FIELDWIDTH - 1 if that
	 is possible.
*/

long formatDouble(char *buffer, double value, unsigned long control)
{
	char    *fmt;
	long     useFloat = control & DOFLOAT;
	long     useFixed = control & DOFIXED;
	
	if (isMissing(value))
	{
		sprintf(buffer, MISSINGFMT, NAMEFORMISSING);
	} /*if (isMissing(value))*/
	else
	{
		if (useFloat || !useFixed && FMTTYPE == 'g')
		{
			fmt = FLOATFMT;
		}
		else if (MAXFIXED < 0 || value >= MAXFIXED || -value >= MAXFIXED/10)
		{
			fmt = FLOATFMT;
		}
		else
		{
			fmt = FIXEDFMT;
		}
		sprintf(buffer, fmt, (value) ? value : 0.0);
	} /*if (isMissing(value)){}else{}*/
	
	if (control & (TRIMLEFT | TRIMRIGHT))
	{
		trimBuffer(buffer, control);
	}
	return (strlen(buffer));
} /*formatDouble()*/


/*
   Put escaped octal representation of character c in buffer and return length
   If buffer == (char *) 0, just return length
   As written, octals are always 3 digits, length is always 4

   970415 Added provision for buffer to be (char *) 0.
*/


long escapedOctal(unsigned char c, unsigned char * buffer)
{
	if (buffer != (unsigned char *) 0)
	{
		*buffer++ = '\\';
		*buffer++ = (c >> 6) + '0';
		*buffer++ = ((c >> 3) & 0x07) + '0';
		*buffer = (c & 0x07) + '0';
	} /*if (buffer != (unsigned char *) 0)*/

	return (4);
} /*escapedOctal()*/

static char      *Endings[4] =
{
	"th", "st", "nd", "rd"
};

char * n_th(long n)
{
	int        ones = (int) (n % 10);
	int        tens = (int) ((n/10) % 10);
	
	return ((tens == 1 || ones > 3) ? Endings[0] : Endings[ones]);
} /*n_th()*/

/* compute the start of the l-th string after ch */
char * skipStrings(char * ch, long l)
{
	long          i;

	for (i=0;i<l;i++)
	{
		ch += strlen(ch) + 1;
	}
	return (ch);
} /*skipStrings()*/

 /* copy l strings from in to out, returning pointer after last character 
	copied to out
 */
char * copyStrings(char * in, char * out, long l)
{
	register char  *cin = in , *cout = out;
	register long   i;

	for (i=0;i < l;i++)
	{
		do
		{
			*cout++ = *cin;
		} while (*cin++ != '\0');
	}
	return (cout);
} /*copyStrings()*/

/* 980521 moved matching codes to globdefs.h */

/*
  Decode pattern in string into pattern.  
  Allowable patterns are
     "part", "*part", "part*" and "*part*"
	  where "part" must consist of name characters.  "part" and "*part*"
  are equivalent.  If part is empty, any name will be matched.  Note there
  is no check for length of string; do that before calling.

  980521
   Added argument checkName.  If checkName == topicNameCheck, the string
   must consist of name characters ([A-Za-z_0-9]); if checkName ==
   variableNameCheck, string must consist of name characters, but the first
   character can be '@'; if checkName == noNameCheck, no checking of characters
   is done

  980529
   Almost completely redid scanPat() and matchName(), enhancing the use of
   wild cards.  Patterns of the form
     start*  start*end  start*mid*  start*mid*end  start*mid*mid* ...
     *end   *mid*  *mid*end  *mid*mid*  *mid*mid*end ...
   are now recognized.
   All the pieces are put in pattern, separated by '\0' (like a CHARACTER
   vector, and 32*nPieces is added to *matchType.
   
  980801
    '?' now reconized as matching any character
*/


static int matchstr(char * name, char * pattern, int questOK, int n)
{
	int          i;
	int          n1 = (n > 0) ? n : strlen(pattern);
	
	for (i = 0; i < n1; i++)
	{
		if (name[i] == '\0' || pattern[i] == '\0')
		{
			return (name[i] == pattern[i] && i == n1 - 1);
		}
		if (name[i] != pattern[i] && (!questOK || pattern[i] != ANYCHAR))
		{
			return (0);
		}
	} /*for (i = 0; i < n1; i++)*/
	return (n > 0 || name[n1] == '\0');
} /*matchstr()*/

short scanPat(char * string,long * matchType, char * pattern, int checkName,
			  long maxLength)
{
	long             sLength = strlen(string), nc = 0;
	int              nPieces = 0, nWild = 0, i;
	char             c, lastc = '\0', *s1, *s2;
	WHERE("scanPat");

	*matchType = 0;
	s1 = string;
	/* 
	   count number of non-repeated wild cards and check for name characters
	   if necessary.  nc is the total length of all the non-wild card pieces,
	   counting needed '\0's except a terminating one
	*/

	while ((c = *s1++))
	{
		/*
		  When checkName == variableNameCheck, allow first 
		  character to be '@'.  All other characters must
		  be legal name characters 
		*/
		if (lastc != WILDCARD)
		{
			nc++;
		}
		
		if (c == WILDCARD)
		{
			if (lastc != WILDCARD)
			{
				nWild++;
			}
		}
		else if (c == ANYCHAR)
		{
			*matchType = FOUNDANYCHAR;
		}
		else if (checkName != noNameCheck)
		{
			if (!isnamechar(c) &&
				(checkName != variableNameCheck || c != TEMPPREFIX))
			{
				*matchType = 0;
				return (0);
			}
			checkName = topicNameCheck;
		}
		lastc = c;
	} /*while ((c = *s1++))*/

	if (nc > maxLength)
	{
		*matchType |= PATTERNTOOLONG;
		pattern[0] = '\0';
		return (0);
	} /*if (nc > maxLength)*/

	if (nWild == 0)
	{
		/* no '*'s */
		*matchType |= EXACTMATCH;
		strcpy(pattern,string);
		return (1);
	}

	if (nWild == sLength)
	{
		/* all wild cards */
		*matchType |= ANYMATCH;
		pattern[0] = '\0';
		return (1);
	} /*if (nWild == sLength)*/
	
	nPieces = nWild - 1;
	if (string[0] != WILDCARD)
	{
		*matchType |= STARTMATCH;
		nPieces++;
	}
	if (string[sLength - 1] != WILDCARD)
	{
		*matchType |= ENDMATCH;
		nPieces++;
	}
	
	s1 = string;
	s2 = pattern;

	for (i = 0; i < nPieces; i++)
	{
		while(*s1 == WILDCARD)
		{
			s1++;
		}
		while (*s1 && *s1 != WILDCARD)
		{
			*s2++ = *s1++;
		}
		*s2++ = '\0';
	} /*for (i = 0; i < nPieces; i++)*/
	*matchType += MATCHTYPEOFFSET*nPieces;

#if (0) /* for debugging*/
	PRINT("*matchType = %08x, pattern[0] = '%s'\n",*matchType,pattern,0,0);
	for (i = 1; i < nPieces; i++)
	{
		pattern += strlen(pattern) + 1;
		PRINT("pattern[%d] = '%s'\n",i,pattern,0,0);
	}
#endif /*0*/
	
	return (1);
} /*scanPat()*/

short matchName(char * name, long matchType, char * pattern)
{
	short        reply;
	WHERE("matchName");
	
	if (matchType < 0)
	{
		reply = 0;
	}
	else if (matchType & ANYMATCH)
	{
		reply = 1;
	}
	else if (matchType & EXACTMATCH)
	{
		reply = matchstr(name, pattern, matchType & FOUNDANYCHAR, 0);
	}
	else
	{
		long      j, nPieces = matchType/MATCHTYPEOFFSET;
			
		reply = 1;
		for (j = 0; reply && j < nPieces; j++)
		{
			long          n1 = strlen(pattern);  /*length of nextPiece of pattern*/
			long          n2 = strlen(name); /*length of unmatched part of name*/

			if (n2 < n1)
			{
				reply = 0;
			}
			else if (j == 0 && (matchType & STARTMATCH))
			{
				reply = matchstr(name, pattern, matchType & FOUNDANYCHAR, n1);
			}
			else if (j == nPieces-1 && (matchType & ENDMATCH))
			{
				reply = matchstr(name + n2 - n1, pattern, matchType & FOUNDANYCHAR, n1);
			}
			else
			{
				long      m = n1 - 1, i;
				char      c1 = pattern[0];
				char      c2 = pattern[m];
;
				for (i = n1;i <= n2; i++, name++)
				{
					if (matchstr(name, pattern, matchType & FOUNDANYCHAR, n1))
					{
						break;
					}
				} /*for (i=n1;i <= n2;i++, name++)*/
				reply = (i <= n2);
			}
			name += n1;
			pattern += n1 + 1;
		} /*for (j = 0; reply && j < nPieces; j++)*/
	}
	return (reply);
} /*matchName()*/

#ifndef NOTIMEH
#ifndef EPX
#include <time.h>
#else /*EPX*/
#include <bsd43/time.h>
typedef long time_t;
#endif /*EPX*/
#endif /*NOTIMEH*/

char        *getTimeAndDate(void)
{
	char           *timeAndDate;
	int             length;
#ifndef NOTIMEH
	time_t          now;

	now = time((time_t *) 0);
	timeAndDate = asctime(localtime(&now));
#else /*NOTIMEH*/
	timeAndDate = "unknown date and time";
#endif /*NOTIMEH*/
	length = strlen(timeAndDate);
	if (isNewline(timeAndDate[length-1]))
	{
		timeAndDate[length-1] = '\0';
	}

	return (timeAndDate);
} /*getTimeAndDate)*/

#if defined(DJGPP) || defined(BCPP)
#include <dos.h>
#endif /*DJGPP||BCPP*/

#if defined(HASGETTIMEOFDAY)
#include <sys/time.h>
# ifndef _STRUCT_TIMEVAL
   /* Structure returned by gettimeofday(2) system call and others */
     struct timeval {
	  unsigned long	tv_sec;		/* seconds */
	  long		tv_usec;	/* and microseconds */
     };

   /* Structure used to represent timezones for gettimeofday(2) and others */
   struct timezone {
	int	tz_minuteswest;	/* minutes west of Greenwich */
	int	tz_dsttime;	/* type of dst correction */
   };
#endif /*_STRUCT_TIMEVAL*/
#endif /*HASGETTIMEOFDAY*/


/*
   return cumulative time in seconds
*/
double mygettime(void)
{
	double        current = -1.0;

#if defined(HASGETTIMEOFDAY)
	struct timeval    tv;
	struct timezone   tz;

	gettimeofday(&tv, &tz);
	current = tv.tv_sec + tv.tv_usec/1e6;

#elif defined(DJGPP) || defined(BCPP)
	struct time       tv;

	gettime(&tv);
	current = 3600.0*tv.ti_hour + 60.0*tv.ti_min +
		tv.ti_sec + tv.ti_hund/100.0;

#elif defined(CLOCKS_PER_SEC)
	clock_t           tv = clock();

	current = tv/(double) CLOCKS_PER_SEC;
#elif defined(EPX)
	current = clock()/1e6;
#else
	/* nothing */
#endif
	return (current);
} /*mygettime()*/

static double            LastEpoch = 0.0;
static double            FirstEpoch = -2.0;

void getElapsedTime(double times[2])
{
	double       last = LastEpoch;
	WHERE("getElapsedTime");
	
	LastEpoch = mygettime();

	if (times == (double *) 0)
	{
		FirstEpoch = LastEpoch;
		LastEpoch = 0;
	}
	else
	{
		times[0] = LastEpoch - last;
		times[1] = LastEpoch - FirstEpoch;
	}
} /*getElapsedTime()*/

void symExtremes(Symbolhandle symh, double vec[2])
{
	long      i, n;
	double    *x;

	vec[1] = -(vec[0] = HUGEDBL);
	if (!isNull(symh))
	{
		n = symbolSize(symh);
		x = DATAPTR(symh);
		if (!isMissing(x[0]))
		{
			vec[0] = vec[1] = x[0];
		}
		

		for (i = 1; i < n; i++)
		{
			if (!isMissing(x[i]))
			{
				if (x[i] < vec[0])
				{
					vec[0] = x[i];
				}
				else if (x[i] > vec[1])
				{
					vec[1] = x[i];
				}
			}
		} /*for (i = 1; i < n; i++)*/
	} /*if (!isNull(symh))*/
} /*symExtremes()*/

void symFill(Symbolhandle symh, double value)
{
	doubleFill(DATAPTR(symh), value, symbolSize(symh));
} /*symFill()*/

void doubleFill(double * x, double value, long length)
{
	while (length-- > 0)
	{
		*x++ = value;
	}
} /*doubleFill()*/

void doubleCopy(double * from, double * to, long length)
{
	INT        inc = 1;

	DCOPY(&length, from, &inc, to, &inc);
} /*doubleCopy*/

/*
   Functions to compute min or max of double vector
   No check is made for Missing.  If there is a possibility of
   missing values, check first with anyMissing()
*/
double doubleMin(double *x, long n)
{
	long         i;
	double       xmin = x[0];

	for (i = 1; i < n; i++)
	{
		if (x[i] < xmin)
		{
			xmin = x[i];
		}
	} /*for (i = 1; i < n; i++)*/
	return (xmin);
} /*doubleMin()*/

double doubleMax(double *x, long n)
{
	long         i;
	double       xmax = x[0];

	for (i = 1; i < n; i++)
	{
		if (x[i] > xmax)
		{
			xmax = x[i];
		}
	} /*for (i = 1; i < n; i++)*/
	return (xmax);
} /*doubleMax()*/

/*
   Functions to compute min or max of long vector
*/
long longMin(long *x, long n)
{
	long         i;
	long         xmin = x[0];

	for (i = 1; i < n; i++)
	{
		if (x[i] < xmin)
		{
			xmin = x[i];
		}
	} /*for (i = 1; i < n; i++)*/
	return (xmin);
} /*longMin()*/

long longMax(long *x, long n)
{
	long         i;
	long         xmax = x[0];

	for (i = 1; i < n; i++)
	{
		if (x[i] > xmax)
		{
			xmax = x[i];
		}
	} /*for (i = 1; i < n; i++)*/
	return (xmax);
} /*longMax()*/

/*
	function for converting double vector to long
	in should not contain missing values and will usually contain
	only integers.
*/
void doubleToLong(double * in, long * out, long length)
{
	long             i;
	
	for (i = 0; i < length; i++)
	{
		out[i] = (long) in[i];
	} /*for (i = 0; i < length; i++)*/
} /*doubleToLong()*/
/*
	function for converting long vector to double
*/
void longToDouble(long * in, double * out, long length)
{
	long             i;
	
	for (i = 0; i < length; i++)
	{
		out[i] = (double) in[i];
	} /*for (i = 0; i < length; i++)*/
} /*longToDouble()*/
