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
#pragma segment Bin
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
/*
  local function to count the number of values n x in bins specified
  by bin boundaries bnds[0], bnds[1], ..., bnds[abs(nbins)]
  If nbins == 0, boundaries are assumed equal spaced with values
  bnds[0], bnds[0] + width, bnds[0] + 2*width, bnds[0] + 3*width, ..., where
  width = bnds[1] - bnds[0].
*/
#define BIGNBINS    8 /* threshold for quartering */

#define COMPARELESS(X,B)  ((lessOrEqualUpper) ? ((X) <= (B)) : ((X) < (B)))

static void doBinning(double *x, double *bnds, double *counts,
					  long n, long nbins,
					  int equispaced, int lessOrEqualUpper,
					  long * nlow, long * nhigh, long * nmiss)
{
	long       i, jstart = 1, jbin, j1, j2, j3;
	long       nmissing = 0, bigNbins = (nbins > BIGNBINS);
	double     q1, q2, q3;
	double     b0 = bnds[0], bn = bnds[nbins], width, value;
	WHERE("doBinning");

	*nlow = *nhigh = *nmiss = 0;

	if (equispaced)
	{
		width = bnds[1] - bnds[0];
	} /*if (equispaced)*/
	else if (bigNbins)
	{
		j1 = nbins/4;
		j2 = nbins/2;
		j3 = nbins - j1;
		q1 = bnds[j1++];
		q2 = bnds[j2++];
		q3 = bnds[j3++];
	}

	for (i = 0; i < n; i++)
	{
		value = x[i];
		if (isMissing(value))
		{
			(*nmiss)++;
		}
		else if (COMPARELESS(value, b0))
		{
			(*nlow)++;
		}
		else if (!COMPARELESS(value, bn))
		{
			(*nhigh)++;
		}
		else
		{
			if (equispaced)
			{
				double     v = (value - b0)/width;
				double     binIndex = ceil(v);
				
				jbin = (long) binIndex;
				if (!lessOrEqualUpper && binIndex == v)
				{
					jbin++;
				}
			} /*if (equispaced)*/
			else
			{
				if (bigNbins)
				{
					jstart = (COMPARELESS(value, q1) ? 1 :
							  (COMPARELESS(value, q2) ? j1 :
							   (COMPARELESS(value, q3) ? j2 : j3)));
				}
				for (jbin = jstart; jbin < nbins ; jbin++)
				{
					if (COMPARELESS(value, bnds[jbin]))
					{
						break;
					}
				} /*for (jbin = jstart; jbin < nbins ; jbin++)*/
			} /*if (equispaced){}else{}*/
			counts[jbin-1]++;
		}
	} /*for (i = 0; i < n; i++)*/
	if (nmissing == n)
	{
		setMissing(counts[0]);
	}
} /*static void doBinning()*/

/*
  usage:
	bin(x [,leftendin:T])
	bin(x,nbins [,silent:T],[leftendin:T])   nbins integer > 0
	bin(x,cat(bndry,width) [,silent:T],[leftendin:T]) 
		bndry typical , width > 0
	bin(x,cat(b_0,b_1,b_2,...,b_nbins)[,silent:T],[leftendin:T])
        (b_0<b_1<...<b_nbins )

  Result: structure with components 'boundaries', 'counts'
	nrows(counts) = nbins = number of bins
	nrows(boundaries) = nrows(counts) + 1
	Default is that values on upper boundary of bin are counted in the bin.
	Values <= b_0 or > b_nbins are not counted; warning message is printed
	  unless silent:T is an argument

    If leftendin:T is an argument, values on lower boundary of bin are
      counted in the bin instead of the bin below and values < b_0 or
      >= b_nbins are not counted

  970212 3rd argument, if any, must now be silent:T or F.  Previously
    just T or F was recognized.

  970220 Added keyword leftendin.  leftendin:T means values on lower boundary
    of bin should be counted in the bin instead of the bin below.

  980303 Fixed an error message format
  990226 replaced putOUTSTR() by putErrorOUTSTR()
*/

Symbolhandle bin(Symbolhandle list)
{
	Symbolhandle          arg1 = COMPVALUE(list, 0), arg2;
	Symbolhandle          symhKey;
	Symbolhandle          result = (Symbolhandle) 0;
	Symbolhandle          symhBoundaries = (Symbolhandle) 0;
	Symbolhandle          symhCounts = (Symbolhandle) 0;
	long                  nargs = NARGS(list);
	long                  dim[2], nrows, ncols;
	long                  nlow, nhigh, nmissing;
	long                  nbins, length = 0;
	long                  icol, iarg, ibin;
	long                  maxnargs = 4, nonKeys = 0;
	int                   equispaced = 1;
	int                   lessOrEqualUpper = 1, silent = 0;
	double                extremes[2], range, hugedbl = HUGEDBL;
	double                boundary0, bottom, top, binwidth, temp;
	char                 *keyword = (char *) 0;
	WHERE("bin");

	setMissing(binwidth);
	if (nargs == 1 && arg1== (Symbolhandle) 0)
	{
		badNargs(FUNCNAME, -1001);
		goto errorExit;
	} /*if (nargs == 1 && arg1== (Symbolhandle) 0)*/
	if (nargs > maxnargs)
	{
		badNargs(FUNCNAME, -maxnargs);
		goto errorExit;
	}
	for (nonKeys = 0; nonKeys < nargs && !isKeyword(COMPVALUE(list,nonKeys));
		 nonKeys++)
	{
		;
	}
	if (nonKeys == 0)
	{
		sprintf(OUTSTR,
				"ERROR: argument 1 to %s() must not be keyword phrase",
				FUNCNAME);
		goto errorExit;
	} /*if (nonKeys == 0)*/

	if (nonKeys > 2)
	{
		sprintf(OUTSTR,
				"ERROR: more than 2 non-keyword arguments to %s()", FUNCNAME);
		goto errorExit;
	} /*if (nonKeys > 2)*/
	
	if (!argOK(arg1, 0, (nargs == 1) ? 0 : 1))
	{
		goto errorExit;
	}

	if (TYPE(arg1) != REAL || !isMatrix(arg1, dim))
	{
		sprintf(OUTSTR,
				"ERROR: argument%s to %s() not REAL matrix",
				(nargs == 1) ? "" : " 1", FUNCNAME);
		goto errorExit;
	} /*if (TYPE(arg1) != REAL || !isMatrix(arg1, dim))*/

	nrows = dim[0];
	ncols = dim[1];

	symExtremes(arg1, extremes);

	if (extremes[0] == hugedbl)
	{
		sprintf(OUTSTR,
				"ERROR: all values of first argument to %s() are MISSING",
				FUNCNAME);
		goto errorExit;
	} /*if (extremes[0] == hugedbl)*/

	range = extremes[1] - extremes[0];
	if (range <= 0.0)
	{
		range = 1;
	}

	for (iarg = nonKeys; iarg < nargs; iarg++)
	{
		symhKey = COMPVALUE(list, iarg);
		if (!(keyword = isKeyword(symhKey)))
		{
			sprintf(OUTSTR,
					"ERROR: non-keyword argument to %s() after a keyword argument",
					FUNCNAME);
			goto errorExit;
		} /*if (!(keyword = isKeyword(symhKey)))*/

		if (strcmp(keyword, "silent") == 0)
		{
			if (!isTorF(symhKey))
			{
				notTorF(keyword);
				goto errorExit;
			}
			silent = (DATAVALUE(symhKey, 0) != 0.0);
		} /*if (strcmp(keyword, "silent") == 0)*/
		else if (strcmp(keyword, "leftendin") == 0)
		{
			if (!isTorF(symhKey))
			{
				notTorF(keyword);
				goto errorExit;
			}
			lessOrEqualUpper = (DATAVALUE(symhKey, 0) == 0.0);
		}
		else
		{
			badKeyword(FUNCNAME, keyword);
			goto errorExit;
		}
	} /*for(iarg = nonKeys; iarg < nargs; iarg++)*/
	
	if (nonKeys == 1)
	{
		nbins = ceil(log((double) nrows) / log(2.0) + 1);
	} /*if (nonKeys == 1)*/
	else
	{
		arg2 = COMPVALUE(list, 1);
		if (!argOK(arg2, REAL, 2))
		{
			goto errorExit;
		}

		length = symbolSize(arg2);

		if (!isVector(arg2))
		{
			sprintf(OUTSTR,
					"ERROR: argument 2 to %s() must be REAL scalar or vector",
					FUNCNAME);
		}
		else if (anyMissing(arg2))
		{
			sprintf(OUTSTR,
					"ERROR: argument 2 to %s() contains MISSING values",
					FUNCNAME);
		}
		else if (isScalar(arg2))
		{
			/* arg2 is number of bins */
			double       fnbins = DATAVALUE(arg2, 0);

			if (fnbins <= 0 || fnbins != floor(fnbins))
			{
				sprintf(OUTSTR,
						"ERROR: number of classes for %s() not a positive integer",
						FUNCNAME);
			}
			else
			{
				nbins = (long) fnbins;
			}
		}
		else if (length == 2)
		{
			/* arg2 is vector(typical, width) */
			boundary0 = DATAVALUE(arg2, 0);
			binwidth = DATAVALUE(arg2, 1);
			if (binwidth <= 0.0)
			{
				sprintf(OUTSTR,
						"ERROR: grouping width must be positive for %s()",
						FUNCNAME);
			}
			else
			{
				double     boundary1;
				
				boundary0 -= floor((boundary0 - extremes[0])/binwidth) *
					binwidth;
				if (COMPARELESS(extremes[0], boundary0))
				{
					boundary0 -= binwidth;
				}
				nbins = ceil((extremes[1] - boundary0)/binwidth);
				boundary1 = boundary0 + nbins*binwidth;
				if (!COMPARELESS(extremes[1], boundary1))
				{
					nbins++;
				}
			} 
		}
		else
		{
			/*
			  arg2 is vector(bndry_1, bndry_2, bndry_3, ...)
			  Assume they are equally spaced until we find they're are not
			*/
			nbins = length - 1;
			boundary0 = bottom = DATAVALUE(arg2, 0);
			binwidth = DATAVALUE(arg2, 1) - bottom;
			for (ibin = 1; ibin <= nbins; ibin++)
			{
				top = DATAVALUE(arg2, ibin);
				temp = top - bottom;
				if (temp <= 0.0)
				{
					sprintf(OUTSTR,
							"ERROR: class limits for %s() not strictly increasing",
							FUNCNAME);
					break;
				} /*if (temp <= 0.0)*/
				if (equispaced && fabs(temp - binwidth)/binwidth > 1e-8)
				{
					equispaced = 0;
				}
				bottom = top;
			} /*for (ibin = 1; ibin <= nbins; ibin++)*/
		}
		if (*OUTSTR)
		{
			goto errorExit;
		}
	} /*if (nonKeys == 1){...}else{...}*/		


	if (isMissing(binwidth))
	{
		boundary0 = extremes[0] - .025*range;
		binwidth = 1.1 * range / (double) nbins;
	} /*if (isMissing(binwidth))*/

	result = StrucInstall(SCRATCH, 2);
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	symhCounts = COMPVALUE(result, 1) = Makereal(nbins * ncols);
	if (symhCounts == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	if (ncols > 1)
	{
		setNDIMS(symhCounts, 2);
		setDIM(symhCounts, 1, nbins);
		setDIM(symhCounts, 2, ncols);
	}
	setNAME(symhCounts, "counts");

	symFill(symhCounts, 0.0);

	if (length > 2 && isscratch(NAME(arg2)))
	{ /* reuse arg2 for boundaries */
		COMPVALUE(list, 1) = (Symbolhandle) 0;
		Cutsymbol(arg2);
		symhBoundaries = COMPVALUE(result, 0) = arg2;
		clearLabels(symhBoundaries);
	}
	else
	{
		symhBoundaries = COMPVALUE(result, 0) = Makereal(nbins+1);
		if (symhBoundaries == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		if (length > 2)
		{
			for (ibin = 0; ibin < length; ibin++)
			{
				DATAVALUE(symhBoundaries, ibin) = DATAVALUE(arg2, ibin);
			}
		}
		else
		{
			for (ibin = 0; ibin <= nbins; ibin++)
			{
				DATAVALUE(symhBoundaries, ibin) = boundary0 + ibin*binwidth;
			}
		}
	}
	setNAME(symhBoundaries, "boundaries");
	setNCLASS(symhBoundaries, -1);
	for (icol = 0; icol < ncols; icol++)
	{
		doBinning(DATAPTR(arg1) + icol * nrows, DATAPTR(symhBoundaries),
				  DATAPTR(symhCounts) + icol*nbins, nrows, nbins, equispaced,
				  lessOrEqualUpper, &nlow, &nhigh, &nmissing);

		if (!silent)
		{
			char                  buffer[30];

			buffer[0] = '\0';

			if (nlow + nhigh > 0)
			{
				if (ncols > 1)
				{
					sprintf(buffer,"in column %ld ", icol + 1);
				}

				sprintf(OUTSTR,
						"WARNING: %ld low and %ld high values %snot counted by %s()",
						nlow, nhigh, buffer, FUNCNAME);
				putErrorOUTSTR();
			} /*if (nlow + nhigh > 0)*/

			if (nmissing > 0)
			{
				if (ncols > 1)
				{
					sprintf(buffer, "column %ld of ", icol + 1);
				}
				if (nmissing == nrows)
				{
					sprintf(OUTSTR,
							"WARNING: %sdata for %s() is all MISSING",
							buffer, FUNCNAME);
				}
				else if (nmissing > 0)
				{
					sprintf(OUTSTR,
							"WARNING: %ld MISSING values in %sdata for %s()",
							nmissing, buffer, FUNCNAME);
				}
				putErrorOUTSTR();
			} /*if (nmissing > 0)*/			
		} /*if (!silent)*/
	} /*for (icol = 0; icol < ncols; icol++)*/

	return (result);

  errorExit:
	putErrorOUTSTR();

	Removesymbol(result);

	return (0);
} /*bin()*/
