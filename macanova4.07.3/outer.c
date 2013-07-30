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
#pragma segment Outer
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

/*
   function to form the outer product of two or more REAL arrays
   usage:  outer(a,b,c,...)
   The dimension of the output is the concatenation of the dimension vectors
   of the input.

   951227 revised to be able to handle more than two arrays.
   960711 changed check for being too large a result
   990226 Replaced putOUTSTR() by putErrorOUTSTR()
*/

Symbolhandle    outer(Symbolhandle list)
{
	Symbolhandle    result = (Symbolhandle) 0, argi;
	double         *x[MAXDIMS], *lastx, lastxj;
	long            needed = 1, totdims = 0, i, j, k;
	long            section, lastdim;
	long            nargs = NARGS(list);
	long            ndims[MAXDIMS], lengths[MAXDIMS], dims[MAXDIMS];
	long            l[MAXDIMS];
	long            foundMissing = 0, foundOverflow = 0;
	double          p;
	double         *prods;
	double          tmp;
	WHERE("outer");

	if (nargs < 2)
	{
		badNargs(FUNCNAME,-1002);
		goto errorExit;
	}
	if (nargs > MAXDIMS)
	{
		badNargs(FUNCNAME, -MAXDIMS);
		goto errorExit;
	}
	
	k = 0;
	needed = 1;
	for (i = 0; i < nargs; i++)
	{
		argi = COMPVALUE(list, i);
		if (!argOK(argi, REAL, i+1))
		{
			goto errorExit;
		}
		ndims[i] = NDIMS(argi);
		totdims += ndims[i];
		if (totdims > MAXDIMS)
		{
			putOutErrorMsg("ERROR:  too many dimensions");
			goto errorExit;
		}
		lengths[i] = symbolSize(argi);
		if (isTooBig(lengths[i], needed, sizeof(double)))
		{
			resultTooBig(FUNCNAME);
			goto errorExit;
		}
		
		needed *= lengths[i];

		for (j = 1; j <= ndims[i]; j++, k++)
		{
			dims[k] = DIMVAL(argi, j);
		}
	} /*for (i = 0; i < nargs; i++)*/
	
	result = RInstall(SCRATCH, needed);
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	setNDIMS(result,totdims);

	k = 0;
	for (i = 0; i < nargs; i++)
	{
		for (j = 0; j < ndims[i]; j++, k++)
		{
			setDIM(result, k + 1, dims[k]);
		}
		
		x[i] = DATAPTR(COMPVALUE(list, i));
		l[i] = 0;
	} /*for (i = 0; i < nargs; i++)*/
		
	prods = DATAPTR(result);
	lastdim = lengths[nargs-1];
	section = needed/lastdim;
	
	lastx = x[nargs-1];
	
	for (j = 0; j < lastdim; j++)
	{
		lastxj = lastx[j];
		if (isMissing(lastxj) || lastxj == 0.0)
		{
			doubleFill(prods, lastxj, section);
			if (isMissing(lastxj))
			{
				foundMissing = 1;
			}
		} /*if (isMissing(lastxj) || lastxj == 0.0)*/
		else
		{
			for (k = 0; k < section; k++)
			{
				p = lastxj;
				for (i = 0; i < nargs-1; i++)
				{
					tmp = x[i][l[i]];
					if (!isMissing(tmp))
					{
						p *= tmp;
#ifdef HASINFINITY
						if (isInfinite(p))
						{
							foundOverflow = 1;
							setMissing(p);
							break;
						}
#endif /*HASINFINITY*/
					}
					else
					{
						foundMissing = 1;
						setMissing(p);
						break;
					}
				} /*for (i = 0; i < nargs-1; i++)*/
				prods[k] = p;

				/* step odometer */
				stepOdometer(l, lengths, nargs, 0, 0);
			} /*for (k = 0; k < section; k++)*/ 
		} /*if (isMissing(p) || p == 0.0){}else{}*/
		prods += section;
	} /*for (j = 0; j < lastdim; j++)*/
		
	if(foundMissing)
	{
		sprintf(OUTSTR, "WARNING: missing values found in argument(s) to %s()",
				FUNCNAME);
		putErrorOUTSTR();
	}
#ifdef HASINFINITY
	if(foundOverflow)
	{
		sprintf(OUTSTR,"WARNING: too large result(s) of %s() set to MISSING",
				FUNCNAME);
		putErrorOUTSTR();
	}
#endif /*HASINFINITY*/
#if (USENOMISSING)
	if (!foundOverflow && !foundMissing)
	{
		setNOMISSING(result);
	}
#endif /*USENOMISSING*/
	return (result);

  errorExit:
	putErrorOUTSTR();
	return (0);
} /*outer()*/
