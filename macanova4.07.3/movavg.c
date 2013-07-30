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
#pragma segment Movavg
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/



#include "globals.h"
#include "blas.h"

#define      MIN(A,B)  (((A) < (B)) ? (A) : (B))
#define      MAX(A,B)  (((A) > (B)) ? (A) : (B))

enum armaCodes
{
	AUTOREG = 0x00,
	REVERSE = 0x01,
	MOVAVG  = 0x02
};

/*
  qauto() -- working function for AR or MA operator

  Vector coefs is assumed to contain autoregression coefficients phi or
  moving average coefficients theta
  Each column of b is assumed to contain a set of 'innovations'

  It is assumed that destination c has already been initialized
  rows of c before i1 or after i2 (if any) are not changed
  but for autoreg, rows before i1 (if iop == 1) or after i2 (if iop == 2)
  are used as starting values

  By C. Bingham, Applied Statistics, U. of Minnesota.
  version 901220 adapted from a version used in matter on the Cyber
  910401 cosmetic changes only
  Translated to C, 920306 for use in Matter2/MacAnova
  990106 added argument season
  990211 added check for infinity and NAN; qauto() now returns a status value

  subroutine to perform 'autoreg', 'movavg'
  iop = 0x00   autoreg
  iop = 0x01   autoreg  reverse
  iop = 0x02   movavg
  iop = 0x03   movavg   reverse
*/

/*
   Note: as written, qauto can handle a single column of coefficients
   and multiple columns
*/
static int qauto(double coefs[],double x[],double result[],
				  double tmpx[],double tmpresult[],
				  long m1, long m2, long n2, long season,
				  long i1, long i2, int iop)
/*
Matrices in Fortran ordering
double coefs[m1],x[m2][n2],result[m2][n2];
double tmpx[m2],tmpresult[m2];
*/
{
	int        ma = iop & MOVAVG, reverse = iop & REVERSE;
	double     signer;
	double    *xj = x, *resultj = result;
	double    *pastvalues = (ma) ? tmpx : tmpresult;
	long       inc, inc1, k1, kstart, kend, i, j, k, l, ii1, ii2;
	long       nmany;
	int        foundNaN = 0;
	WHERE("qauto");
	
	signer = (ma) ? -1.0 : 1.0;
	ii1 = MIN(i1, i2);
	ii2 = MAX(i1, i2);
	nmany = ii2 - ii1 + 1;

	if (!reverse)
	{
		inc = 1;
		kstart = ii1;
		kend = -1;
	}
	else
	{
		inc = -1;
		kstart = ii2;
		kend = m2;
	}
	inc1 = inc * season;

	for (j = 0;j < n2 ;j++)
	{ /* loop over columns */
		for (i = 0;i < m2 ;i++)
		{ /* copy x to tmpx */
			tmpx[i] = xj[i];
		}
		for (i = 0;i < ii1 ;i++)
		{ /* copy starting values, if any, to tmpresult */
			tmpresult[i] = resultj[i];
		}
		for (i = ii2+1;i < m2 ;i++)
		{ /* copy remaining starting values, if any, to tmpresult */
			tmpresult[i] = resultj[i];
		}
		k1 = kstart;
		for (i = 0;i < nmany ;i++)
		{
			double         value = tmpx[k1];
			
			k = k1;
			for (l = 0;l < m1 ;l++)
			{
				k -= inc1;
				if (!reverse && k <= kend || reverse && k >= kend) 
				{ /* off the end; time to quit */
					break;
				}

				value += signer * coefs[l] * pastvalues[k];
			} /*for (l = 0;l < m1 ;l++)*/
#ifdef HASINFINITY
			if (isInfinite(value))
			{
				setMissing(value);
				foundNaN = 1;
			}
#endif /*HASINFINITY*/
#ifdef HASNAN
			if (isNaN(value))
			{
				setMissing(value);
				foundNaN = 1;
			}
#endif /*HASNAN*/
			tmpresult[k1] = value;
			k1 += inc;
		} /*for (i = 0;i < nmany ;i++)*/

		for (i = ii1;i <= ii2 ;i++)
		{ /* copy computed values to output */
			resultj[i] = tmpresult[i];
		}

		xj += m2;
		resultj += m2;
	} /*for (j = 0;j < n2 ;j++)*/
	return (!foundNaN);
} /*qauto()*/

/*
  Operations movavg and autoreg
  Functions for applying autoregressive or moving average operator
  to (segments of) columns of argument.  They may be used to generate
  autoregressive or moving average time series if the columns of the
  argument are innovation series.
  usage:  y <- autoreg(phi,x)                     iop = 0x00
          y <- autoreg(phi,x,reverse:T)           iop = 0x01 REVERSE
          y <- movavg(theta,x)                    iop = 0x02 MOVAVG
          y <- movavg(theta,x,reverse:T)          iop = 0x03 REVERSE | MOVAVG
     or:
  usage:  y <- autoreg(phi,x,limits:c(i1,i2),start:y1)           iop = 0x00
          y <- autoreg(phi,x,reverse:T,limits:c(i1,i2),start:y1) iop = 0x01
          y <- movavg(theta,x,limits:c(i1,i2),start:y1)          iop = 0x02
          y <- movavg(theta,x,reverse:T,limits:c(i1,i2),start:y1)iop = 0x03

  Modified 950602 to allow multicolumn phi.  The following are all permitted
     ncols(phi) = 1 or ncols(theta) = 1
     ncols(phi) > 1 or ncols(theta) > 1 and ncols(x) = 1
     ncols(phi) == ncols(x) or ncols(theta) = x

	980730 added new argument to reuseArg() so that notes will not be kept.

	981231 coefs can now be NULL.  If so, the result is the same as x,
           without labels or notes, and a real vector or real matrix
           No checking of keywords is done
           
    990103 Modified to allow i1 == i2 (length 1)

    990106 Added optional keyword phrase seasonal:L directing that
           phi or theta be used as coefficients in a seasonal
           AR or MA operator with lag L.  qauto() modified correspondingly
           with a new argument to pass the value of L.
    990127 Keyword limits can have a scalar value.  limits:m is interpreted
           as either limits(m,nrows(x)) (without reverse:T) or
		   limits(1,m) (with reverse:T)
           When limits[2] == nrows(x) without reverse:T, start can have length
           limits[1] - 1.  Similarly, when limits[] == 1 with reverse:T,
           start can have length nrows(x) - limits[2].  So, for example,
           autoreg(phi,x,limits:90, start:x1[run(89)]) and
           autoreg(phi,x,reverse:T,limits:10,start:x1[run(11,nrows(x))])
           are acceptable.
*/

enum movavgScratch
{
	GSCRATCH = 0,
	NTRASH
};


Symbolhandle movavg(Symbolhandle list)
{
	Symbolhandle     symhCoefs, symhX, symhStart = (Symbolhandle) 0;
	Symbolhandle     symhKey;
	Symbolhandle     result = (Symbolhandle) 0;
	double         **scratch = (double **) 0;
	double          *pstart, *presult, *px, *pcoefs;
	long             limits[2], dimsx[2], dimsCoefs[2], dimsStart[2];
	long             season = 1, reverse = 0;
	long             nrowsx, ncolsx, nrowsCoefs, ncolsCoefs, ncols;
	long             i, j, nargs = NARGS(list), istart, nvals;
	long             iop;
	long             incx, incCoefs, inc1 = 1, incStart;
	int              nullCoefs = 0;
	int              foundNaN = 0;
	char            *keyword;
	WHERE("movavg");
	TRASH(NTRASH, errorExit);

	OUTSTR[0] = '\0';
	limits[0] = limits[1] = -1;
	iop = (strcmp(FUNCNAME,"movavg") == 0) ? MOVAVG : AUTOREG;
	
	if (nargs < 2 || nargs > 5)
	{
		badNargs(FUNCNAME,(nargs < 2) ? -1002 : -5);
		goto errorExit;
	}
	
	symhCoefs = COMPVALUE(list,0);
	symhX = COMPVALUE(list,1);
	if (isKeyword(symhCoefs) || isKeyword(symhX))
	{
		sprintf(OUTSTR,
				"ERROR: first two arguments to %s() must not be keyword phrases",
				FUNCNAME);
		goto errorExit;
	}
	
	nullCoefs = symhCoefs != (Symbolhandle) 0 && TYPE(symhCoefs) == NULLSYM;
	
	if (!nullCoefs && !argOK(symhCoefs, REAL, 1) || !argOK(symhX, REAL, 2))
	{
		goto errorExit;
	}
	
	if (!nullCoefs && !isMatrix(symhCoefs, dimsCoefs))
	{
		sprintf(OUTSTR,"ERROR: %s must be REAL vector or matrix for %s()",
				(iop & MOVAVG) ? "theta" : "phi" , FUNCNAME);
	}
	else if (!isMatrix(symhX,dimsx))
	{
		sprintf(OUTSTR, "ERROR: argument x to %s() must REAL vector or matrix",
				FUNCNAME);
	}
	else if (!nullCoefs && anyMissing(symhCoefs) || anyMissing(symhX))
	{
		sprintf(OUTSTR,
				"ERROR: 1st or 2nd argument to %s() contains missing value(s)",
				FUNCNAME);
	}
	if (*OUTSTR)
	{
		goto errorExit;
	}

	if (nullCoefs)
	{
		if (isscratch(NAME(symhX)))
		{
			result = reuseArg(list, 1, 0, 0);
		}
		else
		{
			result = Install(SCRATCH, REAL);
			if (result == (Symbolhandle) 0 || !Copy(symhX,result))
			{
				goto errorExit;
			}
			setNAME(result, SCRATCH);
			Setdims(result, (dimsx[1] == 1) ? 1 : 2, dimsx);
			clearLabels(result);
			clearNotes(result);
		}
	} /*if (nullCoefs)*/
	else
	{
		nrowsCoefs = dimsCoefs[0];
		ncolsCoefs = dimsCoefs[1];
		nrowsx = dimsx[0];
		ncolsx = dimsx[1];

		if (ncolsx > 1 && ncolsCoefs > 1 && ncolsx != ncolsCoefs)
		{
			sprintf(OUTSTR,
					"ERROR: arguments 1 and 2 to %s() are matrices with diffent number of columns",
					FUNCNAME);
			goto errorExit;
		} /*if (ncolsx > 1 && ncolsCoefs > 1 && ncolsx != ncolsCoefs)*/
	
		/* check any keywords */
		for (i = 2;i < nargs;i++)
		{
			symhKey = COMPVALUE(list,i);
			keyword = isKeyword(symhKey);
			if (keyword == (char *) 0)
			{
				sprintf(OUTSTR,
						"ERROR: arguments to %s() beyond the 2nd must be keyword phrases",
						FUNCNAME);
			}
			else if (strcmp(keyword,"reverse") == 0)
			{
				if (!isTorF(symhKey))
				{
					notTorF(keyword);
					goto errorExit;
				}
				if (DATAVALUE(symhKey,0) != 0.0)
				{
					iop |= REVERSE;
					reverse = 1;
				}
			}
			else if (strncmp(keyword,"limit",5) == 0)
			{
				int      nlim;
				
				if (!argOK(symhKey, 0, i+1))
				{
					goto errorExit;
				}
				if (TYPE(symhKey) != REAL || !isVector(symhKey) ||
					(nlim = symbolSize(symhKey)) > 2 || anyMissing(symhKey))
				{
					sprintf(OUTSTR,
							"ERROR: value for %s must be vector of one or two positive integers",
							keyword);
					goto errorExit;
				}

				for (j = 0;j < nlim;j++)
				{
					limits[j] = (long) DATAVALUE(symhKey, j) - 1;
					if (DATAVALUE(symhKey,j) != floor(DATAVALUE(symhKey,j)) ||
						limits[j] < 0 || limits[j] >= nrowsx)
					{
						sprintf(OUTSTR,
								"ERROR: elements of limits must be integers between 1 and nrows(x)");
						goto errorExit;
					}
				} /*for (j = 0;j < nlim;j++)*/
				if (nlim == 1)
				{
					limits[1] = nrowsx;
				}
				
				/* 990103 Changed >= to > */
				if (limits[0] > limits[1])
				{
					sprintf(OUTSTR,"ERROR: limits[1] > limits[2]");
				}
			}
			else if (strcmp(keyword,"start") == 0)
			{
				symhStart = symhKey;
				istart = i;
				if (!argOK(symhStart, 0, i+1))
				{
					goto errorExit;
				}
				
				if (TYPE(symhStart) != REAL || !isMatrix(symhStart,dimsStart) ||
					dimsStart[1] != ncolsx)
				{
					sprintf(OUTSTR,
							"ERROR: value for %s not a REAL matrix with ncols(x) columns",
							keyword);
				}
				else if (anyMissing(symhStart))
				{
					sprintf(OUTSTR,"ERROR: missing value(s) in %s for %s()",
							keyword, FUNCNAME);
				}
			}
			else if (strncmp(keyword, "season", 6) == 0)
			{
				if (!isInteger(symhKey, POSITIVEVALUE))
				{
					notPositiveInteger(keyword);
					goto errorExit;
				}
				season = (long) DATAVALUE(symhKey, 0);
			}
			else
			{
				badKeyword(FUNCNAME, keyword);
				goto errorExit;
			}
			if (*OUTSTR)
			{
				goto errorExit;
			}
		} /*for (i = 2;i < nargs;i++)*/

		if (symhStart != (Symbolhandle) 0 && limits[0] < 0 ||
			(symhStart == (Symbolhandle) 0 && limits[0] >= 0))
		{
			sprintf(OUTSTR,
					"ERROR: both 'limits' and 'start' must be specified when either is");
			goto errorExit;
		}
		if (limits[1] >= nrowsx)
		{
			if (reverse)
			{
				limits[1] = limits[0];
				limits[0] = 0;
			}
			else
			{
				limits[1] = nrowsx - 1;
			}
		}
		
		if (limits[0] < 0)
		{
			limits[0] = 0;
			limits[1] = nrowsx - 1;
		}
	
		if (symhStart != (Symbolhandle) 0 && dimsStart[0] != nrowsx)
		{
			if (reverse && limits[0] != 0 ||
				!reverse && limits[1] != nrowsx - 1)
			{
				sprintf(OUTSTR,
						"ERROR: when nows(start) != nrows(x), limits[%d] must be %s",
						(reverse) ? 1 : 2, (reverse) ? "1" : "nrows(x)");
			}
			else if (dimsStart[0] != nrowsx - (limits[1] - limits[0] + 1))
			{
				sprintf(OUTSTR,
						"ERROR: value of 'start' has wrong number of rows");
			}
			if (*OUTSTR)
			{
				goto errorExit;
			}
		} /*if (symhStart != (Symbolhandle) 0 && dimsStart[0] != nrowsx)*/
		
		if (!getScratch(scratch, GSCRATCH, 2*nrowsx, double))
		{
			goto errorExit;
		}

		ncols = (ncolsx > ncolsCoefs) ? ncolsx : ncolsCoefs;
		incx = (ncolsx == 1) ? 0 : nrowsx;
		incStart = (ncolsx == 1) ? 0 : dimsStart[0];
		incCoefs = (ncolsCoefs == 1) ? 0 : nrowsCoefs;
		nvals = nrowsx*ncols;

		if (symhStart != (Symbolhandle) 0 && isscratch(NAME(symhStart)) &&
			dimsStart[0] == nrowsx && (ncolsx == ncolsCoefs || ncolsx > 1))
		{ /* reuse symhStart if possible (symhX can't be reused)*/
			result = reuseArg(list, istart, 0, 0);
		}
		else
		{
			result = RInstall(SCRATCH, nvals);
			if (result == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			if (symhStart != (Symbolhandle) 0)
			{ /* copy starting values if any to result */
				long      nCopy = dimsStart[0];
				
				presult = DATAPTR(result);
				pstart = DATAPTR(symhStart);

				if (nCopy != nrowsx)
				{
					doubleFill(presult, 0.0, nrowsx*ncolsx);
					if (limits[0] == 0)
					{
						presult += limits[1] + 1;
					}
				} /*if (nCopy != nrowsx)*/				
				
				for (j = 0; j < ncols; j++)
				{
						
					DCOPY(&nCopy, pstart, &inc1, presult, &inc1);
					pstart += incStart;
					presult += nrowsx;
				} /*for (j = 0; j < ncols; j++)*/
			} /*if (symhStart != (Symbolhandle) 0)*/
		}
		dimsx[1] = ncols;
		Setdims(result, (ncols == 1) ? 1 : 2, dimsx);

		pcoefs = DATAPTR(symhCoefs);
		px = DATAPTR(symhX);
		presult = DATAPTR(result);
		for (j = 0; j < ncols; j++)
		{
			if (!qauto(pcoefs, px, presult, *scratch, *scratch + nrowsx,
					   nrowsCoefs, nrowsx, 1, season,
					   limits[0], limits[1], iop))
			{
				foundNaN = 1;
			}
			pcoefs += incCoefs;
			px += incx;
			presult += nrowsx;
			if (interrupted(DEFAULTTICKS) != INTNOTSET)
			{
				goto errorExit;
			}
		} /*for (j = 0; j < ncols; j++)*/
	} /*if (nullCoefs){}else{}*/
	if (foundNaN)
	{
		sprintf(OUTSTR,
				"WARNING: some values too large in %s() set to MISSING",
				FUNCNAME);
		putErrorOUTSTR();
	}	
	emptyTrash();

	return (result);

  errorExit:
	putErrorOUTSTR();

	emptyTrash();
	Removesymbol(result);

	return (0);
} /*movavg()*/

		
