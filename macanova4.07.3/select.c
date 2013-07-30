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
#pragma segment Choose
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

/*
   950310
   New command select()
   Usage:
	select(which, arg)
	where which is a REAL vector of positive integers, and arg is a matrix
	with nrows(arg) == nrows(which) and ncols(arg) >= max(which)
	If which is a LOGICAL vector, select(which, arg) is equivalent to
	select(which+1, arg), that is F is treated as 1 and T as 2.

   990226 replaced putOUTSTR() by putErrorOUTSTR()
*/

static long doChar(long type, long n, long nrows, double * which, char * arg,
				   char * result)
{
	int        findLength = (result == (char *) 0);
	long       i, j, length = 0;
	char      *rowstart = arg, *place = result, *cij;
	double     fi;
	WHERE("doChar");
	
	for (i = 0;i < n; i++)
	{
		fi = which[i];
		if (isMissing(fi))
		{
			if (findLength)
			{
				length++;
			}
			else
			{
				*place++ = '\0';
			}
		} /*if (isMissing(fi))*/
		else
		{
			j = (type == LOGIC) ? (fi != 0.0) : ((long) fi - 1);
			cij = skipStrings(rowstart, j*nrows);
			
			if (findLength)
			{
				length += strlen(cij) + 1;
			}
			else
			{
				place = copyStrings(cij, place, 1);
			}
		}
		rowstart = skipStrings(rowstart, 1);
	} /*for (i = 0;i < nrows; i++)*/
	return (length);
} /*doChar()*/

static void doReal(long type, long n, long nrows, double * which, double * arg,
				   double * result)
{
	long           i, j;
	double        *rowstart = arg, fi;
	
	for (i = 0; i< n; i++)
	{
		fi = which[i];
		if (isMissing(fi))
		{
			setMissing(result[i]);
		}
		else
		{
			j = (type == LOGIC) ? (fi != 0.0) : ((long) fi - 1);
			result[i] = rowstart[j*nrows];
		}
		rowstart++;
	} /*for (i = 0; i< nrows; i++)*/
} /*doReal()*/

Symbolhandle selecter(Symbolhandle list)
{
	Symbolhandle    which, arg, result = (Symbolhandle) 0;
	long            i, type, typearg, nargs = NARGS(list);
	long            length;
	long            n, nrows, ncols, dims[2];
	long            foundMissing = 0;
	double          tmp;
	WHERE("select");
	
	if (nargs != 2)
	{
		badNargs(FUNCNAME, 2);
		goto errorExit;
	}
	which = COMPVALUE(list,0);
	arg = COMPVALUE(list,1);
	if (!argOK(which, 0, 1))
	{
		goto errorExit;
	}
	if (!argOK(arg, 0, 2))
	{
		goto errorExit;
	}
	type = TYPE(which);
	typearg = TYPE(arg);
	
	if (type != REAL && type != LOGIC || !isVector(which))
	{
		sprintf(OUTSTR,
				"ERROR: 1st argument to %s must be REAL or LOGICAL vector",
				FUNCNAME);
		goto errorExit;
	}
	
	if (typearg != CHAR && typearg != REAL && typearg != LOGIC ||
		!isMatrix(arg, dims))
	{
		sprintf(OUTSTR,
				"ERROR: 2nd argument to %s must be REAL, LOGICAL, or CHARACTER matrix",
				FUNCNAME);
		goto errorExit;
	}
	n = DIMVAL(which,1);
	nrows = dims[0];
	ncols = dims[1];
	if (n > nrows)
	{
		sprintf(OUTSTR,
				"ERROR: argument 1 to %s must have no more rows than argument 2",
				FUNCNAME);
		goto errorExit;
	}

	for (i=0;i<n;i++)
	{
		tmp = DATAVALUE(which, i);
		if (!isMissing(tmp))
		{
			if (type == LOGIC)
			{
				if (tmp != 0.0 && ncols < 2)
				{
					sprintf(OUTSTR,
							"ERROR: True element in LOGICAL 1st argument to %s when ncols(arg2) = 1",
							FUNCNAME);
					goto errorExit;
				}
			}
			else if (tmp <= 0 || tmp > (double) ncols || tmp != floor(tmp))
			{
				sprintf(OUTSTR,
						"ERROR: REAL 1st argument to %s must be positive integers <= ncolss(arg2)",
						FUNCNAME);
				goto errorExit;
			}
		} /*if (!isMissing(tmp))*/
		else
		{
			foundMissing = 1;
		}
	} /*for (i=0;i<n;i++)*/

	if (typearg == CHAR)
	{
		length = doChar(type, n, nrows, DATAPTR(which), STRINGPTR(arg),
						(char *) 0);
		result = CInstall(SCRATCH, length);
		if (result == (Symbolhandle)  0)
		{
			goto errorExit;
		}
		setDIM(result, 1, n);
		(void) doChar(type, n, nrows, DATAPTR(which), STRINGPTR(arg),
							  STRINGPTR(result));
	}
	else
	{
		result = RInstall(SCRATCH, n);
		if (result == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		setTYPE(result, TYPE(arg));
		doReal(type, n, nrows, DATAPTR(which), DATAPTR(arg),
							  DATAPTR(result));
	}
	if (foundMissing)
	{
		sprintf(OUTSTR,
				"WARNING: MISSING values in 1st argument to %s", FUNCNAME);
		putErrorOUTSTR();
	}
	
	return (result);

  errorExit:
	putErrorOUTSTR();
	return (0);
} /*select()*/
