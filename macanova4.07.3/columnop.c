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
#pragma segment Columnop
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


/*
  Operations sum, prod, max, min
  modified 12/7/91 so that op(a,b,c,...) is equivalent to op(cat(a,b,c,...)),
  where a, b, c, ... are all scalars.
  931121 op(a,b,c,...) now allows vectors.

  Modified February, 1993 to operate recursively on structures.
  Modified March, 1993 to accept logical arguments
  960405 Minor modification in coding of op codes and initial values
  990222 Fixed initialization bug affecting results when a column is all MISSING
         Removed unnecessary 'extern' statements
*/

#include "globals.h"

static char       *OpNames[] =
{
	"sum",
	"prod",
	"max",
	"min",
	(char *) 0
};

enum columnOpCodes
{
	ISUM = 1,
	IPROD,
	IMAX,
	IMIN
};

static Symbolhandle   doColOps(Symbolhandle arg,double *params,
							unsigned long control, unsigned long *status)
{
	Symbolhandle   result = (Symbolhandle) 0;
	double         *x, *resultVals;
	double          s, y, startup;
	long            i, j, nrows, ndims, type = TYPE(arg);
	long            size, foundOK;
	int             op = control & OPMASK;
	int             extremes = (op == IMAX || op == IMIN);
	WHERE("doColOps");
	
	startup = (op == ISUM) ? 0.0 :
		((op == IPROD) ? 1.0 :
		 ((op == IMAX) ? -HUGEDBL :
		  HUGEDBL));
	ndims = NDIMS(arg);
	nrows = DIMVAL(arg,1);
	size = (nrows > 0) ? symbolSize(arg)/nrows : 0;

	result = (size > 0) ?
		RInstall(SCRATCH,size) : Install(NULLSCRATCH, NULLSYM);

	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	if (size > 0)
	{
		/*
		   951010 Always keep the same number of dimensions
		*/
		Setdims(result, ndims, &DIMVAL(arg, 1));
		setDIM(result, 1, 1); /* all the same except 1st */

		if (HASLABELS(arg) && !isScalar(result))
		{
		/*
		   Make label for first dimension "@" and copy last ndims - 1 
		   dimension labels
		*/
			long            labLengths[MAXDIMS];
			char           *labels[MAXDIMS];
			char            numericLabel[2];
			
			getAllLabels(arg, labels, labLengths, (long *) 0);

			labLengths[0] = 2;
			TMPHANDLE = createLabels(ndims, labLengths);

			if (TMPHANDLE != (char **) 0 && setLabels(result, TMPHANDLE))
			{
				numericLabel[0] = NUMERICLABEL;
				numericLabel[1] = '\0';
				
				getAllLabels(arg, labels, (long *) 0, (long *) 0);
				labels[0] = numericLabel;
				for (i = 0; i < ndims; i++)
				{
					appendLabels(result, labels[i], i, dontExpand);
				}
			} /*if (TMPHANDLE != (char **) 0 && setLabels(result, TMPHANDLE))*/
			else
			{
				mydisphandle(TMPHANDLE);
			}
		} /*if (HASLABELS(arg))*/

#if (0) /* code in Version 3.35 and earlier */
		if (ndims > 2)
		{
			setNDIMS(result,ndims - 1);
			for (i = 1; i < ndims; i++)
			{
				setDIM(result,i,DIMVAL(arg,i+1));
			}
		}
		else if (ndims == 2)
		{/* make result a row vector if the argument is a matrix */
			setNDIMS(result,2);
			setDIM(result,1,1);
			setDIM(result,2,DIMVAL(arg,2));
		}
		else
		{
			setNDIMS(result,1);
			setDIM(result,1,1);
		}
#endif /*0*/

		x = DATAPTR(arg);
		resultVals = DATAPTR(result);

		for (j = 0; j < size;j++)
		{
			s = startup;
			foundOK = 0;
			for (i = 0; i < nrows; i++)
			{
				y = x[i];
				if (!isMissing(y))
				{
					foundOK = 1;
					if (type == REAL)
					{
						switch (op)
						{
						  case ISUM:
							s += y;
							break;
						  case IPROD:
							s *= y;
							break;
						  case IMIN:
							s = (y < s) ? y : s;
							break;
						  case IMAX:
							s = (y > s) ? y : s;
							break;
						} /* switch(op) */
					} /*if (type == REAL)*/
					else
					{/* LOGIC */
						switch (op)
						{
						  case ISUM:
							if (y != 0.0)
							{
								s++;
							}
							break;
						  case IPROD:
						  case IMIN:
							s = (y != 0.0) ? s : 0.0;
							break;
						  case IMAX:
							s = (y != 0.0) ? 1.0 : s;
							break;
						} /* switch(op) */
					} /*if (type == REAL){}else{}*/
				} /*if (!isMissing(y))*/
				else
				{
					*status |= FOUNDMISSING;
				}
			} /*for (i = 0; i < nrows; i++)*/

			if (extremes && s == startup)
			{
				if (!foundOK)
				{
					setMissing(resultVals[j]);
				}
				else
				{/* must be type LOGIC */
					resultVals[j] = (op == IMAX) ? 0.0 : 1.0;
				}
			} /*if (extremes && s == startup)*/
#ifdef HASINFINITY
			else if (isInfinite(s))
			{
				*status |= FOUNDOVERFLOW;
				setMissing(resultVals[j]);
			}
#endif /*HASINFINITY*/
			else
			{
				resultVals[j] = s;
			}
			x += nrows;
		} /*for (j = 0; j < size;j++)*/
	} /*if (size > 0)*/
	
	return (result);
	
  errorExit:
	Removesymbol(result);
	return (0);
} /*doColOps()*/

				
/*
  operations sum, prod, max, min
  930202 These now all operate on structures
  931129 Bug fix: If structure component is empty, so is result
  951010 Changed behavior; results always have same number of dimensions
         as argument, with the first replaced by 1.  Previously, an array
         with more than 2 dimensions resulted in an array with one less
		 dimension
  960311 Add code to copy labels to doColOps().  If result is not a scalar and
         the argument has labels, so does the result, with the label for the
         first dimension null and the remaining labels copied from arg
  980220 NULL arguments are now acceptable.  If they are all NULL the result
         is NULL
  980308 Keyword phrase 'silent:T' can be the final argument to suppress
         warning messages
*/

Symbolhandle    columnops(Symbolhandle list)
{
	Symbolhandle    symh, arg1 = (Symbolhandle) 0, result = (Symbolhandle) 0;
	long            i;
	unsigned long   status = 0, control = 0;
	int             op, foundNonNull = 0;
	long            nargs = NARGS(list), type = NULLSYM, typei, length;
	double         *source, *dest;
	char           *keyword;
	int             silent = 0;
	WHERE("columnops");

	*OUTSTR = '\0';

	op = matchKey(FUNCNAME,OpNames, (long *) 0);

	if (nargs > 1 && (keyword = isKeyword(COMPVALUE(list, nargs - 1))))
	{
		if (strcmp(keyword, "silent") != 0)
		{
			badKeyword(FUNCNAME, keyword);
		}
		symh = COMPVALUE(list, nargs - 1);
		if (!isTorF(symh))
		{
			notTorF(keyword);
			goto errorExit;
		}
		silent = DATAVALUE(symh, 0) != 0.0;
		nargs--;
	} /*if (nargs > 1 && (keyword = isKeyword(COMPVALUE(list, nargs - 1))))*/
	
	arg1 = COMPVALUE(list, 0);
	if (nargs == 1)
	{
		if (!argOK(arg1, NULLSYM, 0))
		{
			return (0);
		}
		type = TYPE(arg1);

		if (type != STRUC && type != REAL && type != LOGIC && type != NULLSYM)
		{
			badType(FUNCNAME,-type,0);
			goto errorExit;
		}
		foundNonNull = (type != NULLSYM);
		
		if (type == STRUC)
		{
			type = getStrucTypes(arg1);
			if (!(type & (REALTYPE1|LOGICTYPE1)) ||
			   (type & ~(REALTYPE1|LOGICTYPE1)))
			{
				sprintf(OUTSTR,
							"ERROR: non-NULL components of argument to %s() must all be REAL or LOGICAL",
						FUNCNAME);
				goto errorExit;
			}
		}
	} /*if (nargs == 1)*/
	else
	{ /* allow list of vectors as equivalent to vector */
		length = 0;
		for (i=0;i<nargs;i++)
		{
			symh = COMPVALUE(list,i);
			if (!argOK(symh, NULLSYM, i+1))
			{
				goto errorExit;
			}
			typei = TYPE(symh);

			if (typei != REAL && typei != LOGIC && typei != NULLSYM)
			{
				badType(FUNCNAME,-typei,0);
				goto errorExit;
			}

			if (typei != NULLSYM)
			{
				if (type == NULLSYM)
				{
					type = typei;
				}
				else if (typei != type)
				{
					sprintf(OUTSTR,
							"ERROR: all non-NULL arguments to %s() must have same type",
							FUNCNAME);
					goto errorExit;
				}
			
				if (!isVector(symh))
				{
					sprintf(OUTSTR,
							"ERROR: non-NULL multiple arguments to %s() must all be vectors",
							FUNCNAME);
					goto errorExit;
				}
				length += symbolSize(symh);
			}
			
		} /*for (i=0;i<nargs;i++)*/
		if (type == NULLSYM)
		{
			return (NULLSYMBOL);
		}

		symh = COMPVALUE(list,0);
		/* put new arg1 in the list so that it will eventually be deleted */
		arg1 = COMPVALUE(list,0) = RInstall(SCRATCH,length);
		if (arg1 == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		setTYPE(arg1,type);
		dest = DATAPTR(arg1);
		for (i=0;i<nargs;i++)
		{
			if (i > 0)
			{
				symh = COMPVALUE(list,i);
			}
			if (TYPE(symh) != NULLSYM)
			{
				source = DATAPTR(symh);
				length = symbolSize(symh);
				while (length-- > 0)
				{
					*dest++ = *source++;
				}
			}
			
			if (i == 0 && isscratch(NAME(symh)))
			{
				Removesymbol(symh);
			}
		} /*for (i=0;i<nargs;i++)*/
	} /*if (nargs == 1){}else{}*/

	control = (isStruc(arg1)) ? LEFTRECUR : 0;

	result = doRecur1(doColOps,arg1,(double *) 0, control | op, &status);
	/* Note: result is already in symbol table */
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	if (!silent && status & FOUNDMISSING)
	{
		sprintf(OUTSTR,"WARNING: MISSING values found by %s()", FUNCNAME);
		putErrorOUTSTR();
	} /*if (status & FOUNDMISSING)*/

#ifdef HASINFINITY
	if (!silent && status & FOUNDOVERFLOW)
	{
		sprintf(OUTSTR,"WARNING: too large result(s) of %s() set to MISSING",
				FUNCNAME);
		putErrorOUTSTR();
	} /*if (status & FOUNDOVERFLOW)*/
#endif /*HASINFINITY*/

	return (result);

  errorExit:
	putErrorOUTSTR();

	return (0);
} /*columnops()*/
