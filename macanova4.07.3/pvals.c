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
#pragma segment Pvals
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

/*
  971012 added cumdunnett() and invdunnett()
  971023 modified treatment of 3rd (optional) parameter
  971030 epsilon is now set by keyword rather than being a parameter
		 on invchi(), invstudrng() and cumstudrng()
  990212 Changed putOUTSTR() to putErrorOUTSTR()
*/
#define MIN(a,b) (a < b ? a : b)
#define INTGR(X)        (!isMissing(X) && floor(X) == (X))
#define POSREAL(X)      (!isMissing(X) && (X) > 0)
#define POSINT(X)       (INTGR(X) && (X) > 0)
#define ATLEAST(X,A)    (!isMissing(X) && (X) >= (A))
#define NONNEG(X)       ATLEAST(X, 0.0)
#define INBOUNDS(X,A,B) (!isMissing(X) && (X) >= (A) && (X) <= (B))


#include "globals.h"
/*
	No call to PDunnett() or QDunnett() is compiled unless DUNNETT
	is defined and nothing in dunnett.c is compiled if DUNNETT
	is not defined there
*/
#define DUNNETT

#define MAXBINOMN    100000  /* max allow n for binomial */
#define MAXCHILAMBDA 1419.56542578676
/*
  Maximum number of groups for cumdunnett() and invdunnett();
  similarly defined in dunnett.c
*/
#define NGROUPMAX    50
enum distCodes
{
	CUMULATIVE =    0x0001,
	INVERSE =       0x0002,
	INORDIST =      0x0010,
	ICHIDIST =      0x0020,
	ISTUDIST =      0x0040,
	IFDIST =        0x0080,
	IPOIDIST =      0x0100,
	IBINDIST =      0x0200,
	IGAMMADIST =    0x0400,
	ISTUDRNGDIST =  0x0800,
	IBETADIST =     0x1000,
	IDUNNETTDIST =  0x2000
};

enum pvalsOpCodes
{
	ICUMNOR     = CUMULATIVE | INORDIST,
	ICUMCHI     = CUMULATIVE | ICHIDIST,
	ICUMSTU     = CUMULATIVE | ISTUDIST,
	ICUMF       = CUMULATIVE | IFDIST,
	ICUMPOI     = CUMULATIVE | IPOIDIST,
	ICUMBIN     = CUMULATIVE | IBINDIST,
	IINVNOR     = INVERSE    | INORDIST,
	IINVCHI     = INVERSE    | ICHIDIST,
	IINVSTU     = INVERSE    | ISTUDIST,
	IINVF       = INVERSE    | IFDIST,
	ICUMGAMMA   = CUMULATIVE | IGAMMADIST,
	IINVGAMMA   = INVERSE    | IGAMMADIST,
	ICUMBETA    = CUMULATIVE | IBETADIST,
	IINVBETA    = INVERSE    | IBETADIST,
	ICUMSTUDRNG = CUMULATIVE | ISTUDRNGDIST,
	IINVSTUDRNG = INVERSE    | ISTUDRNGDIST,
	ICUMDUNNETT = CUMULATIVE | IDUNNETTDIST,
	IINVDUNNETT = INVERSE    | IDUNNETTDIST
}; /*enum pvalsOpCodes*/

typedef struct opEntry
{
	char *name;
	short iop;
	short nargs;
} opEntry;

static opEntry Ops[] =
{ /* name        iop      nargs */
	{"cumnor",    ICUMNOR,     1}, /* cumnor(x) */
	{"cumchi",    ICUMCHI,    -3}, /* cumchi(x,df[,lam]) */
	{"cumstu",    ICUMSTU,    -3}, /* cumstu(x,df[,lam]) */
	{"cumF",      ICUMF,      -4}, /* cumF(x,ndf,ddf[,lam]) */
	{"cumpoi",    ICUMPOI,     2}, /* cumpoi(x,lambda) */
	{"cumbin",    ICUMBIN,     3}, /* cumbin(x,n,p) */
	{"invnor",    IINVNOR,     1}, /* invnor(p) */
	{"invchi",    IINVCHI,    -4}, /* invchi(p,df[,lam[,epsilon:eps]) */
	{"invstu",    IINVSTU,     2}, /* invstu(p,df) */
	{"invF",      IINVF,       3}, /* invF(p,ndf,ddf) */
	{"cumgamma",  ICUMGAMMA,   2}, /* cumgamma(x,a) */
	{"invgamma",  IINVGAMMA,   2}, /* invgamma(p,a) */
	{"cumbeta",   ICUMBETA,   -4}, /* cumbeta(x,a,b[,lam]) */
	{"invbeta",   IINVBETA,    3}, /* invbeta(p,a,b) */
	{"cumstudrng",ICUMSTUDRNG,-4}, /* cumstudrng(x,ngrp,df [,epsilon:eps]) */
	{"invstudrng",IINVSTUDRNG,-4}, /* invstudrng(p,ngrp,df [,epsilon:eps]) */
	{"cumdunnett",ICUMDUNNETT,-6}, /* cumdunnett(x,ngrp,df [,groupsizes][,onsided:T, epsilon:eps]) */
	{"invdunnett",IINVDUNNETT,-6}, /* invdunnett(p,ngrp,df [,groupsizes][,onsided:T][,epsilon:eps]) */
	{(char *) 0,      0,       0}
}; /*opEntry Ops[]*/
/*
   On cumstudrng() and invstudrng, ngrp >= 2, df >= 1.0 and
   optional eps controls accuracy
*/

/*  check parameter values for cumdunnett() and invdunnett() */

static void checkDunnettParams(long checkedParam [], double param [],
							   long foundBadParam [], double dunnettGS [],
							   long nargs, Symbolhandle param3,
							   long dunnettLastDim, long dunnettOffset,
							   long place, double * x)
{
	long        j;

	/* Number of groups must be integer >= 2 and <= NGROUPMAX */

	if (!checkedParam[0] &&
		(!INTGR(param[0]) || !INBOUNDS(param[0], 2.0, NGROUPMAX)))
	{
		foundBadParam[0] = 1;
		setMissing(*x);
	}

	/* DF must be real >= 1 */
	if (!checkedParam[1] && !ATLEAST(param[1], 1.0))
	{
		foundBadParam[1] = 1;
		setMissing(*x);
	} /*if (!checkedParam[1] && !ATLEAST(param[1], 1.0))*/

	/*
	  Each row (last dimension) of groupSizes must be
	  be a vector.
	  (i) If an element is 0, all following elements must be zero
		  and the last non-zero element is considered replicated
		  so that there are ngroups values > 0
	  (ii) If its length < ngroups, the last element is replicated.
	  (ii) If its length > ngroups, the extra elements are ignored.
	*/

	if (!isMissing(*x))
	{
		if (nargs == 4) 
		{
			double  value;
			long    jj = place;
			int     foundZero = 0;

			for (j = 0; j < dunnettLastDim; j++)
			{
				value = DATAVALUE(param3, jj);
				if (!NONNEG(value) || j == 0 && value == 0.0 ||
					foundZero && value > 0.0)
				{
					foundBadParam[2] = 1;
					setMissing(*x);
					break;
				}
				if (value == 0.0)
				{
					value = dunnettGS[j-1];
					foundZero = 1;
				}
				dunnettGS[j] = value;
				jj += dunnettOffset;
			} /*for (j = 0; j < dunnettLastDim; j++)*/

			for (j = dunnettLastDim; j < param[0]; j++)
			{
				dunnettGS[j] = dunnettGS[j-1];
			}
		} /*if (nargs == 4) */
		else
		{
			for (j = 0; j < param[0]; j++)
			{
				dunnettGS[j] = 1.0;
			}
		}
	} /*if (!isMissing(*x))*/
} /*checkDunnettParams()*/


Symbolhandle    cumcdf(Symbolhandle list)
{
	Symbolhandle    arg[4], result = (Symbolhandle) 0, symhKey;
	double        **y;
	double          epsilon = -1.0;
	double          x, param[3];
	long            ndims, dims[4][MAXDIMS+1],sizes[4], ii[3];
	long            dim[MAXDIMS+1], size = 1;
	long            minargs, margs, nargs = NARGS(list);
	long            checkedParam[3], foundBadParam[3], foundOutOfRange = 0;
	long            misMatch = 0, nonConverge = 0, nerrors = 0;
	long            nmissing = 0;
	long            iarg, iparam;
	long            i, j;
	long            arithMissingFound = 0;
	long            dunnettLastDim, dunnettOffset, dunnett2sided = -1;
	double          dunnettGS[NGROUPMAX];
	char           *keyword;
	int             iop, hasKeywords, optionalArg;
	opEntry        *op;
	char           *parName[3];
	char           *dfName = "degrees of freedom";
	char           *noncenName = "noncentrality";
	WHERE("cumcdf");
	
	*OUTSTR = '\0';
	
	for (iarg = 0; iarg < 4;iarg++)
	{
		sizes[iarg] = 0;
	}
	
	for (op=Ops; op->iop; op++)
	{
		if (strcmp(op->name, FUNCNAME) == 0)
		{
			break;
		}
	} /*for (op=Ops; op->iop; op++)*/

	iop = op->iop;
	
#ifndef DUNNETT
	if (iop & IDUNNETTDIST)
	{
		notImplemented(FUNCNAME);
		return (0);
	}
#endif /*DUNNETT*/

	margs = op->nargs;
	hasKeywords = (iop & IDUNNETTDIST) || iop == IINVCHI ||
	  (iop & ISTUDRNGDIST);
	optionalArg = iop == ICUMSTU || iop == ICUMF || (iop & ICHIDIST) ||
	  iop == ICUMBETA || (iop & IDUNNETTDIST);	

	minargs = (margs > 0) ? margs : -margs;
	if (hasKeywords)
	{
		minargs -= (iop & IDUNNETTDIST) ? 2 : 1;
	}
	if (optionalArg)
	{
		minargs--;
	}
	
	if (iop == 0)
	{ /* should never occur */
		sprintf(OUTSTR, "ERROR: unrecognized pvalue or inv cdf name %s",
				FUNCNAME);
		goto errorExit;
	}
	

	while (1)
	{
		symhKey = COMPVALUE(list, nargs-1);
		if ((keyword = isKeyword(symhKey)))
		{
			if (((iop & IDUNNETTDIST) || iop == IINVCHI ||
					  (iop & ISTUDRNGDIST)) && strncmp(keyword, "eps", 3) == 0)
			{
				if (!isNumber(symhKey, POSITIVEVALUE))
				{
					notPositiveReal(keyword);
					goto errorExit;
				}
				if (epsilon >= 0)
				{
					sprintf(OUTSTR,
							"ERROR: keyword %s used more than once in %s()",
							keyword, FUNCNAME);
					goto errorExit;
				}
				epsilon = DATAVALUE(symhKey, 0);
				epsilon = (epsilon > .01) ? .01 : epsilon;
			}
#ifdef DUNNETT
			else if ((iop & IDUNNETTDIST) &&
					 strncmp(keyword, "oneside", 7) == 0)
			{
				if (!isTorF(symhKey))
				{
					notTorF(keyword);
					goto errorExit;
				}
				if (dunnett2sided >= 0)
				{
					sprintf(OUTSTR,
							"ERROR: keyword %s used more than once in %s()",
							keyword, FUNCNAME);
					goto errorExit;
				}
				dunnett2sided = (DATAVALUE(symhKey, 0) == 0.0);
			}
#endif /*DUNNETT*/
			else
			{
				badKeyword(FUNCNAME, keyword);
				goto errorExit;
			}
			nargs--;
		} /*if ((keyword = isKeyword(symhKey)))*/
		else
		{
			break;
		}
	} /*while (1)*/

	if (hasKeywords)
	{
		margs += (iop & IDUNNETTDIST) ? 2 : 1;
	}
	
	if (margs > 0 && nargs != margs || margs < 0 && nargs > -margs)
	{
		badNargs(FUNCNAME, margs);
		goto errorExit;
	}
	else if (margs < 0 && nargs < minargs)
	{
		badNargs(FUNCNAME, -(minargs+1000));
		goto errorExit;
	}
	
	switch(iop)
	{
	  case ICUMCHI:
	  case ICUMSTU:
	  case IINVCHI:
	  case IINVSTU:
		parName[0] = dfName;
		parName[1] = noncenName;
		break;

	  case ICUMF:
	  case IINVF:
		parName[0] = "numerator. d.f.";
		parName[1] = "denominator. d.f.";
		parName[2] = noncenName;
		break;

	  case ICUMPOI:
		parName[0] = "mean";
		break;

	  case ICUMBIN:
		parName[0] = "N";
		parName[1] = "P";
		break;

	  case ICUMGAMMA:
	  case IINVGAMMA:
		parName[0] = "alpha";
		break;

	  case ICUMBETA:
	  case IINVBETA:
		parName[0] = "alpha";
		parName[1] = "beta";
		parName[2] = noncenName;
		break;

	  case ICUMSTUDRNG:
	  case IINVSTUDRNG:
		parName[0] = "number of groups";
		parName[1] = dfName;
		break;

#ifdef DUNNETT
	  case ICUMDUNNETT:
	  case IINVDUNNETT:
		parName[0] = "number of groups";
		parName[1] = dfName;
		parName[2] = "group sizes";
		break;
#endif /*DUNNETT*/

	  default:
		;
	} /*switch(iop)*/
	
	for (iarg = 0; iarg < nargs;iarg++)
	{
		arg[iarg] = COMPVALUE(list, iarg);

		if (!argOK(arg[iarg], REAL, iarg+1))
		{
			goto errorExit;
		}

		getDims(dims[iarg], arg[iarg]);
		sizes[iarg] = symbolSize(arg[iarg]);
		if (iarg == 3 && (iop & IDUNNETTDIST))
		{
			dunnettLastDim = dims[iarg][dims[iarg][0]];
			if (dunnettLastDim > NGROUPMAX)
			{
				sprintf(OUTSTR,
						"ERROR: last dimension of groupSizes for %s() > %ld",
						FUNCNAME, (long) NGROUPMAX);
				goto errorExit;
			}
			
			sizes[iarg] = sizes[iarg]/dunnettLastDim;
			dunnettOffset = sizes[iarg];
			/* divide by last dimension to get number of
               different sample sizes specified */
		} /*if (iarg == 3 && (iop & IDUNNETTDIST))*/

		if (iarg == 0 || size == 1 && sizes[iarg] > 1)
		{
			size = sizes[iarg];
			ndims = NDIMS(arg[iarg]);

			for (j = 0;j <= ndims;j++)
			{
				dim[j] = dims[iarg][j];
			}
			if (iarg == 3 && (iop & IDUNNETTDIST))
			{
				dim[0]--; /* fix for dunnett vector parm dims */
			}
		} /*if (iarg == 0 || size == 1 && sizes[iarg] > 1)*/
		
		if (iarg >= 1)
		{
			if (sizes[iarg] != 1)
			{
				long      nd = dims[iarg][0];

				/* more than 1 value of this parameter */
				if (sizes[iarg] != size)
				{
					/* can only happen if earlier argument had size > 1 */
					misMatch = 1;
				}
				else if (iarg == 3 && (iop & IDUNNETTDIST))
				{
					if (nd > 1)
					{
						if (nd != dim[0] + 1)
						{
							misMatch = 1;
						}
						else
						{
							for (j = 1; j <= dim[0];j++)
							{
								if (dim[j] != dims[iarg][j])
								{
									misMatch = 1;
									break;
								}
							} /*for (j = 1; j <= dim[0];j++)*/
						}
					} /*if (nd > 1)*/
				}
				else
				{
					for (j = 1; j <= nd;j++)
					{
						/* this allows trailing dimensions of 1*/
						if ((j<=dim[0] && dim[j] != dims[iarg][j]) ||
						   (j > dim[0] && dim[j] > 1))
						{
							misMatch = 1;
							break;
						}
					} /*for (j = 1; j <= nd;j++)*/
				}

				if (misMatch)
				{
					sprintf(OUTSTR,
							"ERROR: %s for %s() not scalar and has wrong dimensions",
							parName[iarg-1], FUNCNAME);
					goto errorExit;
				} /*if (misMatch)*/
			} /*if (sizes[iarg] != 1)*/
		} /*if (iarg >= 1)*/
	} /*for (iarg = 0; iarg < nargs;iarg++)*/
	
	ndims = dim[0];

	result = RInstall(SCRATCH, size);
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	y = DATA(result);
	setNDIMS(result, ndims);
	for (i = 1; i <= ndims; i++)
	{
		setDIM(result, i, dim[i]);
	}
/*
   restrictions on parameter values:
   cumnor(x)
   cumstu(x,PosReal[,NonNegReal])
   cumchi(x,PosReal[,NonNegReal])
   cumF(x,PosReal,PosReal[,Real])
   cumpois(x,PosReal)
   cumbin(x,PosInt,p)
   invnor(p)
   invstu(p,PosReal)
   invchi(p,PosReal[,NonNegReal[,PosReal]])
   invF(p,PosReal,PosReal)
   cumgamma(x,PosReal)
   invgamma(p,PosReal)
   cumbeta(x,PosReal,PosReal[,PosReal])
   invbeta(p,PosReal,PosReal)
   cumstudrng(x,PosInt>=2,PosInt,PosReal)
   invstudrng(p,PosInt>=2,PosInt,PosReal)
   cumdunnett(x,PosInt>=3,PosInt,PosRealVVector)
   invdunnett(p,PosInt>=3,PosInt,PosRealVVector)
   where x is real and 0 <= p <= 1 is real
*/

	for (iparam = 0; iparam < 3; iparam++)
	{
		param[iparam] = 0.0;
		ii[iparam] = foundBadParam[iparam] = checkedParam[iparam] = 0;
	} /*for (iparam = 0; iparam < 3; iparam++)*/
	
	for (i = 0;i < size;i++)
	{
#if defined (WXWIN) || defined (MACINTOSH) || defined(DJGPP)
		if (interrupted(DEFAULTTICKS) != INTNOTSET)
		{
			goto intrrupt;
		}
#endif /*WXWIN || MACINTOSH || DJGPP*/
	
		if (sizes[0] == 1)
		{
			x = DATAVALUE(arg[0], 0);
		}
		else
		{
			x = DATAVALUE(arg[0], i);
		}
		if (isMissing(x))
		{
			arithMissingFound++;
		}
		
		for (j = 1; j < nargs;j++)
		{
			if (!checkedParam[j-1])
			{
				param[j-1] = DATAVALUE(arg[j], ii[j-1]++);
			}
		} /*for (j = 1; j < nargs;j++)*/

		switch (iop)
		{
		  case ICUMNOR:
			if (!isMissing(x))
			{
				x = Cnor(x);
			}
			break;

		  case ICUMCHI:
		  case ICUMGAMMA:
			if (!checkedParam[0] && !POSREAL(param[0]))
			{
				foundBadParam[0] = 1;
				setMissing(x);
			}
			if (nargs == 3 && !checkedParam[1] &&
				!INBOUNDS(param[1], 0.0, MAXCHILAMBDA))
			{
				foundBadParam[1] = 1;
				setMissing(x);
			}
			if (!isMissing(x))
			{
				if (nargs == 2 || param[1] == 0.0)
				{
					x = (iop == ICUMCHI) ?
						Cchi(x, param[0]) : Cgam(x, param[0], 1.0);
				} /*if (nargs == 2 || param[1] == 0.0)*/
				else
				{
					/* can't happen for cumgamma() */
					x = noncenCchi(x, param[0], param[1]);
					if (x < 0)
					{
						nonConverge++;
						x = -x;
					}
				} /*if (nargs == 2 || param[1] == 0.0){}else{}*/
			}
			break;

		  case ICUMSTU:
			if (!checkedParam[0] && !POSREAL(param[0]))
			{
				foundBadParam[0] = 1;
				setMissing(x);
			}
#if (0) /* no need for noncentrality parameter to be >= 0 */
			if (nargs == 3 && !checkedParam[1] && !NONNEG(param[1]))
			{
				foundBadParam[1] = 1;
				setMissing(x);
			}
#endif /*0*/
			if (!isMissing(x))
			{
				if (nargs == 2 || param[1] == 0.0)
				{
					x = Cstu(x, param[0]);
				}
				else
				{
					x = noncenCstu(x, param[0], param[1]);
					if (x < 0)
					{
						nonConverge++;
						x = -x;
					}
				}
			}
			break;

		  case ICUMF:
		  case ICUMBETA:
			if (!checkedParam[0] && !POSREAL(param[0]))
			{
				foundBadParam[0] = 1;
				setMissing(x);
			}
			if (!checkedParam[1] && !POSREAL(param[1]))
			{
				foundBadParam[1] = 1;
				setMissing(x);
			}
			if (nargs == 4 && !checkedParam[2] && !NONNEG(param[2]))
			{
				foundBadParam[2] = 1;
				setMissing(x);
			}
			if (!isMissing(x))
			{
				x = (iop == ICUMF) ? noncentF(x, param[2], param[0], param[1]) :
					noncentBeta(x, param[2], param[0], param[1]);
				if (nargs == 4 && x > 1.0)
				{
					nonConverge++;
					x = x - floor(x);
				}
			}
			break;

		  case ICUMPOI:
			if (!checkedParam[0] && !NONNEG(param[0]))
			{
				foundBadParam[0] = 1;
				setMissing(x);
			}
			if (!isMissing(x))
			{
				x = Cpoi((long) floor(x), param[0]);
			}
			break;

		  case ICUMBIN:
			if (!checkedParam[0] &&
				(!POSINT(param[0]) || param[0] > MAXBINOMN))
			{
				foundBadParam[0] = 1;
				setMissing(x);
			}
			if (!checkedParam[1] && (!NONNEG(param[1]) || param[1] > 1.0))
			{
				foundBadParam[1] = 1;
				setMissing(x);
			}
			if (!isMissing(x))
			{
				x = Cbin((long) floor(x), (long) param[0], param[1]);
				x = (x < 0.) ? 0.0 : ((x > 1.0) ? 1.0 : x);
			}
			break;

		  case ICUMSTUDRNG:
			/* Ngroups >= 2 */
			if (!checkedParam[0] && (!INTGR(param[0]) || param[0] < 2.0))
			{
				foundBadParam[0] = 1;
				setMissing(x);
			}
			/* DF >= 1 */
			if (!checkedParam[1] && !ATLEAST(param[1], 1.0))
			{
				foundBadParam[1] = 1;
				setMissing(x);
			}

			if (!isMissing(x))
			{
				x = Pstudrange(x, param[0], param[1], epsilon);
				if (x < 0.0)
				{
					nonConverge++;
					x -= NONCONVERGED;
				}
			}
			break;
			
#ifdef DUNNETT
		  case ICUMDUNNETT:
			if (!isMissing(x) && dunnett2sided)
			{
				x = (x < 0) ? 0 : x;
			}
			checkDunnettParams(checkedParam, param, foundBadParam,
							   dunnettGS, nargs, arg[3], dunnettLastDim,
							   dunnettOffset, ii[2] - 1, &x);
			
			if (!isMissing(x))
			{
				x = PDunnett(x, param[0], dunnettGS, param[1], dunnett2sided,
							 epsilon);
			}
			break;
#endif /*DUNNETT*/

		  default:
			/* all the inverses */
			if (!isMissing(x) && (x <= 0 || x >= 1.))
			{
				foundOutOfRange++;
				setMissing(x);
			}
			switch (iop)
			{ /* all the inverses */
			  case IINVNOR:
				if (!isMissing(x))
				{
					x = Qnor(x);
				}
				break;

			  case IINVCHI:
			  case IINVGAMMA:
				if (!checkedParam[0] && !POSREAL(param[0]))
				{
					foundBadParam[0] = 1;
					setMissing(x);
				}
				if (nargs > 2 && !checkedParam[1] && !NONNEG(param[1]))
				{
					foundBadParam[1] = 1;
					setMissing(x);
				}
				if (!isMissing(x))
				{
					if (param[1] == 0.0)
					{
						x = (iop == IINVCHI) ? 
							Qchi(x, param[0]) : Qgam(x, param[0], 1.0);
					}
					else
					{
						x = noncenQchi(x, param[0], param[1], epsilon);
						if (isMissing(x))
						{
							nerrors++;
						}
						else if (x < 0)
						{
							nonConverge++;
							x = -x;
						}
					}
				} /*if (!isMissing(x))*/
				break;

			  case IINVSTU:
				if (!checkedParam[0] && !POSREAL(param[0]))
				{
					foundBadParam[0] = 1;
					setMissing(x);
				}
				if (!isMissing(x))
				{
					x = Qstu(x, param[0]);
				}
				break;

			  case IINVF:
			  case IINVBETA:
				if (!checkedParam[0] && !POSREAL(param[0]))
				{
					foundBadParam[0] = 1;
					setMissing(x);
				}
				if (!checkedParam[1] && !POSREAL(param[1]))
				{
					foundBadParam[1] = 1;
					setMissing(x);
				}
				if (!isMissing(x))
				{
					x = (iop == IINVF) ? Qsne(x, param[0], param[1]) :
						Qbeta(x, param[0], param[1]);
				}
				break;

			  case IINVSTUDRNG:
				if (!checkedParam[0] && (!INTGR(param[0]) || param[0] < 2.0))
				{
					foundBadParam[0] = 1;
					setMissing(x);
				}
				if (!checkedParam[1] && !ATLEAST(param[1], 1.0))
				{
					foundBadParam[1] = 1;
					setMissing(x);
				}
				if (!isMissing(x))
				{
					x = Qstudrange(x, param[0], param[1], epsilon);
					if (x < 0.0)
					{
						if (x < NONCONVERGED)
						{
							nerrors++;
							setMissing(x);
						}
						else
						{
							nonConverge++;
							x -= NONCONVERGED;
						}						
					} /*if (x < 0.0)*/
				} /*if (!isMissing(x))*/
				break;

#ifdef DUNNETT			
		  case IINVDUNNETT:
			checkDunnettParams(checkedParam, param, foundBadParam,
							   dunnettGS, nargs, arg[3], dunnettLastDim,
							   dunnettOffset, ii[2] - 1, &x);

			if (!isMissing(x))
			{
				long       ierr;
				
				x = QDunnett(x, param[0], dunnettGS, param[1], dunnett2sided,
							 epsilon, &ierr);

				if (interrupted(DEFAULTTICKS))
				{
					goto intrrupt;
				}
				if (ierr)
				{
					if (ierr == NONCONVERGED)
					{
						nonConverge++;
					}
					else
					{
						setMissing(x);
					}
				}
			} /*if (!isMissing(x))*/
			break;
#endif /*DUNNETT*/

			} /*switch (iop)*/
		} /*switch (iop)*/

		if (sizes[1] == 1)
		{ /* nargs > 1 */
			if (foundBadParam[0])
			{
				break;
			}
			else
			{
				checkedParam[0] = 1;
			}
		} /*if (sizes[1] == 1)*/

		if (sizes[2] == 1)
		{ /* nargs > 2 */
			if (foundBadParam[1])
			{
				break;	
			}
			else
			{
				checkedParam[1] = 1;
			}
		} /*if (sizes[2] == 1)*/

		if (sizes[3] == 1)
		{ /* nargs > 3 */
			if (foundBadParam[2])
			{
				break;	
			}
			else
			{
				checkedParam[2] = 1;
			}
		} /*if (sizes[3] == 1)*/
		if (isMissing(x))
		{
			nmissing++;
		}
		(*y)[i] = x;
	} /*for (i = 0;i < size;i++)*/

#ifdef SEGMENTED
	UNLOADSEG(Cnor);
	UNLOADSEG(Cchi);
	UNLOADSEG(Cstu);
	UNLOADSEG(noncentF);
	UNLOADSEG(noncentBeta);
	UNLOADSEG(Cpoi);
	UNLOADSEG(Cbin);
	UNLOADSEG(Qnor);
	UNLOADSEG(Qchi);
	UNLOADSEG(Qstu);
	UNLOADSEG(Qchi);
	UNLOADSEG(Qsne);
	UNLOADSEG(Qbeta);
	UNLOADSEG(Pstudrange);
	UNLOADSEG(Qstudrange);
#ifdef DUNNETT
	UNLOADSEG(PDunnett);
	UNLOADSEG(QDunnett);
#endif /*DUNNETT*/
#endif /*SEGMENTED*/	

	for (j = 1; j < nargs;j++)
	{
		if (foundBadParam[j-1])
		{
			if (sizes[j] == 1)
			{
				sprintf(OUTSTR,
						"ERROR: illegal value of %s for %s",
						parName[j-1], FUNCNAME);
				goto errorExit;
			}
			else
			{
				sprintf(OUTSTR,
						"WARNING: found illegal value of %s for %s",
						parName[j-1], FUNCNAME);
				putErrorOUTSTR();
			}
		} /*if (foundBadParam[j-1])*/
	} /*for (j = 1; j < nargs;j++)*/
	*OUTSTR = '\0';

	if (foundBadParam[0] || foundBadParam[1])
	{
		strcpy(OUTSTR, "WARNING: result set to MISSING");
		putErrorOUTSTR();
	}
	
	if (nonConverge)
	{
		if (size == 1)
		{
			sprintf(OUTSTR,
					"WARNING: computation for %s() not fully converged",
					FUNCNAME);
		}
		else
		{
			sprintf(OUTSTR,
					"WARNING: computation for %ld value%s of %s() not fully converged",
					nonConverge, (nonConverge > 1) ? "s" : "", FUNCNAME);
		}
		putErrorOUTSTR();
	}
	else if (nerrors > 0)
	{
		sprintf(OUTSTR,
				"WARNING: problems in computing %ld values for %s(); results set to MISSING",
				nerrors, FUNCNAME);
		putErrorOUTSTR();
	}

	if (foundOutOfRange)
	{
		sprintf(OUTSTR,
				"WARNING: Arg. 1 to %s() has %ld values < 0 or > 1; results set to MISSING",
				FUNCNAME, foundOutOfRange);
		putErrorOUTSTR();
	} /*if (foundOutOfRange)*/

	if (arithMissingFound)
	{
		sprintf(OUTSTR,
				"WARNING: Arg. 1 to %s() has %ld MISSING values; results set to MISSING",
				FUNCNAME, arithMissingFound);
		putErrorOUTSTR();
	} /*if (arithMissingFound)*/

	return (result);

  errorExit:
	putErrorOUTSTR();
	/* fall through */

  intrrupt:
	Removesymbol(result);
	return (0);
	
} /*cumcdf()*/

