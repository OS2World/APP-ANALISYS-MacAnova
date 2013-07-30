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
#pragma segment Partacf
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

/*
  980303 fixed bugs, clean out some fossils
  990226 Replaced most use of putOUTSTR() by putErrorOUTSTR() and slightly
         changed other code
*/

#include "globals.h"

/* prototypes */
static void qpacf(double[],double[],long,long,double[],long[],long);
static void updowndate(double [], long, long);

/*
 ****	C. Bingham, Applied Statistics, U of Minnesota
	Converted from F77
 	920306
*/
#define UP        1
#define DOWN      0
#define TOL       1e-8

enum pacfOpMasks
{
	PACF    = 0x00,
	INVERSE = 0x01,
	YULEW   = 0x02
};


/* fill array x with value val */

static void fillvec(double value, double *vector, long n)
{
	register double    val = value, *x = vector;

	while (n-- > 0)
	{
		*x++ = val;
	}
} /*fillvec()*/


/*
	Subroutine to perform operations 'acf', 'yulewalker'
     iop = 0 PACF           pacf
     iop = 1 PACF|INVERSE   pacf  inverse
     iop = 2 YULEW          yulewalker
     iop = 3 YULEW|INVERSE  yulewalker  inverse

  Input matrix is in a, in Fortran order, i.e., by columns
  For pacf and yulewalker, a[i,j] is rho[i+1,j], i = 0, ...
  For pacf inverse, a[i,j] is pac[i+1,j]
  for yulewalker invers a[i,j] is phi[i+1,j]

  Output is put in b
  For pacf, b[i,j] is set to pac[i+1,j]
  For yulewalker, b[i,j] is set to phi[i+1,j]
  For pacf inverse and yulewalker inverse, b[i,j] is set to rho[i+1,j]

  a and b are nrows by ncols, in Fortran order

  scr1 is a single column of scratch space of length nrows
  status reports status, status[j] == 0 means j-th column o.k.;
  status[j] == i > 0 means problem found in row i-1
  return value is 0 if any problems found; otherwise 1
*/

static void qpacf(double a[], double b[], long nrows, long ncols,double scr1[],
				  long status[], long iop)
{
	double         *aj = a, *bj = b, *rho, *phi;
	double         sig,factor;
	long           i,ii,j,l;
	WHERE("qpacf");
	
	for (j = 0;j<ncols ;j++)
	{ /* loop over columns */
		bj[0] = aj[0];
		if (!(iop & INVERSE) && fabs(aj[0]) >= 1.0-.5*TOL)
		{
			bj[0] = aj[0]/fabs(aj[0]);
			status[j] = 1;
			fillvec(0.0,bj+1,nrows-1);
		} /*if (fabs(aj[0]) >= 1.0-.5*TOL)*/
		else
		{
			status[j] = 0;
			if (nrows != 1)
			{
				if (iop & INVERSE)
				{ /* pacf inverse or yulewalker inverse */
					phi = scr1;
					rho = bj;
					for (i=0;i<nrows;i++)
					{
						phi[i] = aj[i];
					}
					
					if (iop & YULEW)
					{ /* yulewalker inverse */
	/* For yulewalker inverse first transform from phi to pacf */
						for (i = nrows-1;i>0;i--)
						{
							if (fabs(phi[i]) >= 1.0)
							{
								break;
							}
							updowndate(phi,i,DOWN);
						}

						if (i > 0)
						{
							status[j] = -1;
							fillvec(0.0,rho,nrows);
							continue;
						}
					} /*if (iop & YULEW)*/
/*
  At this point phi should contain the pacf for column j.
  The acf is built up in rho while phi is updated to autoregressive
  coefficients
*/
					factor = 1.0;
					rho[0] = phi[0];
					sig = 1.0 - phi[0]*phi[0];
					for (i = 1;i<nrows ;i++)
					{
						if (sig <= TOL && status[j] == 0)
						{
							status[j] = i;
							sig = 0.0;
							factor = 0.0;
						}
						phi[i] *= factor;
						rho[i] = sig*phi[i];
						for (l = 0;l<i ;l++)
						{
							rho[i] += phi[l]*rho[i-l-1];
						}
						if (i<nrows)
						{
							updowndate(phi,i,UP);
						}
						sig *= (1.0 - phi[i]*phi[i]);
					} /*for (i = 1;i<nrows ;i++)*/
				} /*if (iop & INVERSE)*/
				else
				{ /* pacf or yulewalker */
/*
  Use Durbin's recursion to go from acf to pacf (iop == PACF) or AR
  coefficients (iop == YULEW).
  At end of loop, a[*][j] contains auto regressive coefficients of order i
*/
					rho = aj;
					phi = scr1;
					phi[0] = rho[0];
					sig = 1.0 - phi[0]*phi[0];
					for (i = 1;i<nrows ;i++)
					{
						if (sig <= TOL)
						{
							break;
						}							
						phi[i] = rho[i];
						for (l = 1;l<=i ;l++)
						{
							phi[i] -= phi[l-1]*rho[i-l];
						}
						phi[i] /= sig;
						if (fabs(phi[i]) > 1.0)
						{
							status[j] = i+1;
							phi[i] /= fabs(phi[i]);
						}
						/* ****do not update phi on last cycle for pacf. */
						if (i < nrows-1 || iop & YULEW)
						{
							updowndate(phi,i,UP);
						}
						/* ****keep this pac for pacf */
						if (iop == PACF)
						{
							bj[i] = phi[i];
						}
						sig *= 1.0 - phi[i]*phi[i];
					} /*for (i = 1;i<nrows ;i++)*/

					if (i < nrows)
					{
	/* 	acf apparently not positive definite.  set rest of column to zero */
						ii = i-1;
						phi[ii] /= fabs(phi[ii]);
						if (iop == PACF)
						{
							bj[ii] = phi[ii];
						}
						status[j] = i;
						fillvec(0.0, bj+i, nrows-i);
					} /*if (i < nrows)*/
					
					if (iop == YULEW)
					{ /* keep all current coefficients for yulewalker */
						for (l = 0;l < i ;l++)
						{
							bj[l] = phi[l];
						}
					}
				} /*if (iop & INVERSE){...}else{...}*/
			} /*if (nrows != 1)*/
		} /*if (fabs(aj[0]) >= 1.0-.5*TOL){...}else{...}*/
		aj += nrows;
		bj += nrows;
	} /*for (j = 0;j<ncols ;j++)*/
} /*qpacf()*/


/*
    subroutine to update array phi in qpacf
    up = 1 means go from phi(p-1,i) to phi(p,i), for i = 1,...,p-1
          assuming phi(p,p) is already in phi(p)
    up = 0 means go from phi(p,i) to phi(p-1,i), for i = 1,...,p-1.
*/


static void updowndate(double phi[],long p,long up)
{
	double           fac1, fac2, phil, phipml;
	long             l, l2;

	if (p >= 1)
	{
		fac1 = (up) ? phi[p] :-phi[p];
		fac2 = (up) ? 1.0 : (1.0 - fac1*fac1);
		
		l2 = p/2;
		for (l = 0;l<l2 ;l++)
		{
			phil = phi[l];
			phipml = phi[p-1-l];
			phi[l] = (phil - fac1*phipml)/fac2;
			phi[p-1-l] = (phipml - fac1*phil)/fac2;
		}

		if (p & 1)
		{
			phi[l2] = (phi[l2]-fac1*phi[l2])/fac2;
		}
	} /*if (p >= 1)*/
} /*updowndate()*/


enum partacfScratch
{
	GSCR = 0,
	GSTATUS,
	NTRASH
};

/*
  980303 Added check for MISSING values in argument 1
*/

Symbolhandle partacf(Symbolhandle list)
{
	Symbolhandle      result = (Symbolhandle) 0;
	Symbolhandle      arg = COMPVALUE(list,0), symhkey;
	char             *keyword;
	long              op, nargs = NARGS(list);
	long              dims[2], j, msgs, nvalues;
	long            **status = (long **) 0;
	double          **scr = (double **) 0;
	WHERE("partacf");
	TRASH(NTRASH, errorExit);
	
	op = (strcmp(FUNCNAME,"partacf") == 0) ? PACF : YULEW;
	if (nargs > 2)
	{
		badNargs(FUNCNAME,-2);
		goto errorExit;
	}

	if (!argOK(arg,0,(nargs > 1) ? 1 : 0))
	{
		goto errorExit;
	}

	if (TYPE(arg) != REAL || !isMatrix(arg,dims))
	{
		sprintf(OUTSTR,
				"ERROR: argument%s to %s() not REAL vector or matrix",
				(nargs > 1) ? " 1" : "", FUNCNAME);
		goto errorExit;
	}
	
	if (anyMissing(arg))
	{
		sprintf(OUTSTR,
				"ERROR: argument%s to %s() contains MISSING values",
				(nargs > 1) ? " 1" : "", FUNCNAME);
		goto errorExit;
	}
	
	if (nargs == 2)
	{
		symhkey = COMPVALUE(list,1);
		if ((keyword = isKeyword(symhkey)))
		{
			if (strncmp(keyword,"inv",3) != 0)
			{
				badKeyword(FUNCNAME,keyword);
				goto errorExit;
			}
		}
		if (!isTorF(symhkey))
		{
			notTorF((keyword) ? keyword : "2nd argument");
			goto errorExit;
		}
		if (DATAVALUE(symhkey,0) != 0.0)
		{
			op |= INVERSE;
		}
	} /*if (nargs == 2)*/

	nvalues = dims[0]*dims[1];
	if (!getScratch(scr,GSCR,dims[0],double) ||
	   !getScratch(status,GSTATUS,dims[1],long))
	{
		goto errorExit;
	}
	result = RInstall(SCRATCH,nvalues);
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	if (dims[1] > 1)
	{
		setNDIMS(result,2);
		setDIM(result,2,dims[1]);
	}
	else
	{
		setNDIMS(result,1);
	}
	setDIM(result,1,dims[0]);

	qpacf(DATAPTR(arg), DATAPTR(result), dims[0], dims[1], *scr/* + nvalues*/,
			  *status, op);
	if (PRINTWARNINGS)
	{
		for (j = 0;j<dims[1];j++)
		{
			if ((msgs = (*status)[j]) != 0)
			{
				if (msgs < 0)
				{
					sprintf(OUTSTR,
							"WARNING: Column %ld of argument does not correspond to a stationary operator",
							j+1);
					putOUTSTR();
					sprintf(OUTSTR,
							"         column %ld of result set to zeros",j+1);
				}
				else if (!(op & INVERSE))
				{
					sprintf(OUTSTR,
							"WARNING: Col. %ld of argument not positive definite at row %ld",
							j+1, msgs);
					putOUTSTR();
					sprintf(OUTSTR,
							"         Remaining elements in column assumed zero");
				}
				else
				{
					sprintf(OUTSTR,
							"WARNING: Col. %ld of argument seems to be a deterministic operator of length %ld",
							j+1,msgs);
				}
				putErrorOUTSTR();
			} /*if ((msgs = (*status)[j]) != 0)*/
		} /*for (j = 0;j<dims[1];j++)*/
	} /*if (PRINTWARNINGS)*/	
	
	emptyTrash();
	return (result);
	
  errorExit:
	putErrorOUTSTR();
	Removesymbol(result);
	emptyTrash();
	
	return (0);
} /*partacf()*/

