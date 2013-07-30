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
#pragma segment Yates
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

/*
  980317 removed some obsolete externs, added check for overflow
         Now spots nrows(x) == 1 as an error
  990226 Replaced putOUTSTR() by putErrorOUTSTR()
*/

enum yatesScratch
{
	GXSCR  = 0,
	NTRASH
};

Symbolhandle    yates(Symbolhandle list)
{
	Symbolhandle    symh, result = (Symbolhandle) 0;
	double        **xscr = (double **) 0;
	double         *xold, *xnew, *xtmp;
	double         *xin[2], *xout[2];
	long            n, ncols, i, j, k, halfn, nfact, dim[2];
#ifdef HASINFINITY
	int             missingSet = 0;
#endif /*HASINFINITY*/
	WHERE("yates");
	TRASH(NTRASH,errorExit);
	
	*OUTSTR = '\0';
	if (NARGS(list) > 1)
	{
		badNargs(FUNCNAME,1);
		goto errorExit;
	}
	
	symh = COMPVALUE(list, 0);
	if (!argOK(symh,0,1))
	{
		goto errorExit;
	}
	
	if (TYPE(symh) != REAL || !isMatrix(symh,dim))
	{
		sprintf(OUTSTR,"ERROR: argument to %s() must be REAL vector or matrix",
				FUNCNAME);
		goto errorExit;
	}

	if (anyMissing(symh))
	{
		sprintf(OUTSTR,
				"ERROR: missing values not allowed in argument to %s()",
				FUNCNAME);
		goto errorExit;
	}
	
	n = dim[0];
	ncols = dim[1];
	halfn = 1;
	
	for (nfact = 0; halfn < n; nfact++)
	{
		halfn *= 2;
	}

	if (n == 1 || n != halfn)
	{
		sprintf(OUTSTR,
				"ERROR: %s argument to %s() must be >= 2 and a power of two",
				(ncols == 1) ? "length of" : "number of rows in",
				FUNCNAME);
		goto errorExit;
	} /*if (n == 1 || n != halfn)*/

	halfn = n / 2;

	result = RInstall(SCRATCH,(n - 1) * ncols);
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	setDIM(result,1,n-1);
	if (ncols > 1)
	{
		setNDIMS(result, 2);
		setDIM(result,2,ncols);
	}
	
	if (!getScratch(xscr,GXSCR,2*n,double))
	{
		goto errorExit;
	}

	xold = *xscr;
	xnew = xold + n;
	
	for (j=0;j<ncols;j++)
	{
		xin[0] = DATAPTR(symh) + j*n;
		xout[0] = xold;
	
		for (i = 0; i < n; i++)
		{
			*xout[0]++ = *xin[0]++;
		}
	
		for (k = 0;k < nfact;k++)
		{
			xin[0] = xold;
			xin[1] = xin[0] + 1;
			xout[0] = xnew;
			xout[1] = xout[0] + halfn;
			for (i = 0; i < halfn; i++)
			{
				*xout[0]++ = *xin[0] + *xin[1];
				*xout[1]++ = -*xin[0] + *xin[1];
				xin[0] += 2;
				xin[1] += 2;
			}
			/* swap xnew and xold */
			xtmp = xnew;
			xnew = xold;
			xold = xtmp;
		} /*for (k = 0;k < nfact;k++)*/

		xin[0] = xtmp + 1;
		xout[0] = DATAPTR(result) + j*(n-1);
	
		for (i = 1; i < n; i++)
		{
			double     value = *xin[0]++/(double) halfn;

#ifdef HASINFINITY
			if (isInfinite(value))
			{
				setMissing(value);
				missingSet = 1;
			}
#endif /*HASINFINITY*/
			*xout[0]++ = value;
		} /*for (i = 1; i < n; i++)*/
	} /*for (j=0;j<ncols;j++)*/
	
	emptyTrash();
#ifdef HASINFINITY
	if (missingSet)
	{
		sprintf(OUTSTR,
				"WARNING: value computed by %s() large, set to MISSING",
				FUNCNAME);
		putErrorOUTSTR();
	} /*if (missingSet)*/
#if (USENOMISSING)
	if (missingSet)
	{
		clearNOMISSING(result);
	}
	else
	{
		setNOMISSING(result);
	}
#endif /*USENOMISSING*/
#endif /*HASINFINITY*/

	return (result);

  errorExit:
	putErrorOUTSTR();

	emptyTrash();
	Removesymbol(result);

	return (0);
	
} /*yates()*/
