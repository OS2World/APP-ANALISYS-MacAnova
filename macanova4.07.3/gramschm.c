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
#pragma segment Gramschm
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
#include "blas.h"

#define TINY 1e-28

/*
   Routine to do gramschmidt regression
	
	   print = 1 says to notify if a column of x is singular
	
	   REGX		X matrix of regression
	   NREGX	number of cols in X matrix
	   NDATA	number of rows in X matrix
	   RESIDUALS
	   XTXINV	X'X inverse
	   REGCOEF	coefficients
	   REGSS        sequential SS for terms
	
   961204 Added macros incNproducts() and testNproducts() as new way
   to allow interrupting on Mac and WXwin versions

   990212 Changed putOUTSTR() to putErrorOUTSTR()
*/

#undef PRINTSTUFF
#ifdef PRINTSTUFF
/* Following used in debugging.  It prints out current value of *XTXINV,
   prepended with the current value of *REGCOEF
*/
static void PRINTstuff(void)
{
	double     *xtxinv = *XTXINV, *xxij;
	double     *coef = *REGCOEF;
	long        nregx = (long) NREGX;
	long        i, j;

	if(GUBED & 8192)
	{
		for(i=0;i<nregx;i++)
		{
			printf("%12.5g",coef[i]);
			
			xxij = xtxinv+i;
			for(j=0;j<nregx;j++)
			{
				printf(" %12.5g",*xxij);
				xxij += nregx;
			}
			printf("\n");
		}
		printf("\n");
	}	
} /*PRINTstuff()*/
#else /*PRINTSTUFF*/
#define PRINTstuff()
#endif /*PRINTSTUFF*/

/* these macros no longer used */
#define x(i,j) ( regx[ i + j *  ndata ] )
#define xtx(i,j) ( xtxinv[ i + j* nregx] )

#define JUSTRSS    -1 /*op code for updateRegss()*/

/*
  Compute new Residual SS or SSCP matrix.  If jx >= 0 also compute regression
  SS or SSCP for jx-th X-variable
*/
static void     updateRegss(long jx)
{
	register long    j1, j2, l1, l2, m1, m2;
	long             k1, k2;
	long             rstep, sstep, sstep2;
	long             incx = 1, incy = 1;
	long             ndata = (long) NDATA, nregx = (long) NREGX;
	long             ny = (long) NY;
	int              computeRegss = (jx != JUSTRSS);
	double          *ss = *REGSS;
	double          *rss = ss + nregx, *residuals = *RESIDUALS;
	double          *regss = ss + jx;
	double           ssres, ssreg;
	WHERE("updateRegss");

	/* set up total ss in ss */
	/* Note: DDOT does not change first argument */
	if(ny == 1)
	{
		ssres = DDOT(&ndata, residuals, &incx, residuals, &incy);
		incNproducts(ndata);
		
#ifdef HASINFINITY
		if (isInfinite(ssres))
		{
			goto overflowExit;
		} /*if (isInfinite(ssres))*/			
#endif /*HASINFINITY*/
		if(computeRegss)
		{
			*regss = *rss - ssres;
			if(*regss < 0.0)
			{
				*regss = 0.0;
			}
		}
		*rss = ssres;
	} /*if(ny == 1)*/
	else
	{
		rstep = ndata;
		sstep = nregx + 1;
		sstep2 = sstep * ny;
		j1 = 0; /* k1 * rstep */
		l1 = 0;  /* k1 * sstep */
		m1 = 0;  /* k1 * sstep2 */
		for (k1 = 0; k1 < ny; k1++)
		{
			/* NOTE: blas routines do not change ndata */
			ssres = DDOT(&ndata, residuals + j1, &incx, residuals + j1, &incy);
			incNproducts(ndata);
#ifdef HASINFINITY
			if (isInfinite(ssres))
			{
				goto overflowExit;
			} /*if (isInfinite(ssres))*/			
#endif /*HASINFINITY*/
			if(computeRegss)
			{
				ssreg = rss[l1 + k1 * sstep2] - ssres;
				if(ssreg < 0.0)
				{
					ssreg = 0.0;
				}
				regss[m1 + k1 * sstep] = ssreg;
			}
			else
			{
				ssreg = -1.0;
			}
			
			rss[l1 + k1 * sstep2] = ssres;
			
			j2 = 0; /* k2 * rstep*/
			l2 = l1; /* k1* sstep + k2 * sstep2 */
			m2 = m1; /* k1 * sstep2 + k2 * sstep */
			for (k2 = 0; k2 < k1; k2++)
			{
				/* NOTE: blas routines do not change ndata */
				if(ssreg != 0.0)
				{
					ssres = DDOT(&ndata, residuals + j1, &incx,
								 residuals + j2, &incy);
					incNproducts(ndata);
#ifdef HASINFINITY
					if (isInfinite(ssres))
					{
						goto overflowExit;
					} /*if (isInfinite(ssres))*/			
#endif /*HASINFINITY*/
					if(computeRegss)
					{
						regss[m2] = regss[l2] = rss[l2] - ssres;
					}
					rss[m2] = rss[l2] = ssres;
				}
				else if(computeRegss)
				{
					regss[m2] = regss[l2] = 0.0;
				}
					
				j2 += rstep;
				l2 += sstep2;
				m2 += sstep;
			} /*for (k2 = 0; k2 < k1; k2++)*/
			j1 += rstep;
			l1 += sstep;
			m1 += sstep2;
		} /*for (k1 = 0; k1 < ny; k1++)*/
	} /*if(ny == 1){...}else{...}*/

	return;

  overflowExit:
	INTERRUPT = FPESET;
	
} /*updateRegss()*/

/*
  Carry out gramschmidt orthonormalization of X-variables followed and
  sweep them out of RESIDUALS.

  If (control & DONTDOSWEEP) != 0, X-variables are already orthonormal, so
  they only need to be swept out of RESIDUALS

  When (control & DOSS) != 0 (only for OLS or weighted OLS), compute SS or SSCP
  for each degree of freedom.

  When (control & DOCOEFS1) != 0, compute R^(-1) in upper triangle of
  (*XTXINV) and coefficients of orthonormalized regressors in (*REGCOEF)
  from accumulated information, where R is part of the Q-R decomposition
  of the matrix of regressors, computed by modified Gramschmidt

  When (control & DOCOEFS2) != 0, compute (x'x)^-1 in (*XTXINV) from R^(-1)
  and compute actual regression coefficients from the orthonormal coefficients
  and R
*/
void gramschmidt(long control)
{
	register long   l1, l2, l3, l4, j1, j2;
	long             i, j, k, k2;
	long             colno;
	long             incx, incy, length;
	long             ndata = (long) NDATA, nregx = (long) NREGX;
	long             ny = (long) NY;
	long             aliased = 0;
	register double *regx1, *regx2, *xtxinvij;
	register double *coefs2;
	double          *xtxinv, *coef;
	double          *ss = *REGSS, *residuals = *RESIDUALS;
	double          *regx = *REGX;
	double           xbar, tmpxx, tmpxy, d;
	WHERE("gramschmidt");
	
	clearNproducts();
	
	incx = 1;
	incy = 1;
	xtxinv = (XTXINV != (double **) 0) ? *XTXINV : (double *) 0;
	coef = (REGCOEF != (double **) 0) ? *REGCOEF : (double *) 0;
	
	REGDIM = 0.0; /* defined in glm.h, used in iterglm.c */
	
	if (control & DOSWEEP)
	{
		/* first get within column sum of sq for singularity checking */
		j1 = 0; /* colno*ndata */
		for (colno = 0; colno < nregx; colno++)
		{
			ss[colno] = 0.;
			xbar = 0.;
			regx1 = *REGX + j1;
			for (i = 0; i < ndata; i++)
			{
				xbar += *regx1++;
			}
			xbar /= NDATA;
			regx1 = *REGX + j1; /* *regx1 is x(i,colno) */
			for (i = 0; i < ndata; i++)
			{
				d = regx1[i] - xbar;
				ss[colno] += d*d;
			}
			incNproducts(ndata);
			if(!modeltermEqual(modelterm(MODEL,0),(modelPointer) NULLTERM) ||
				(fabs(xbar) > TINY && ss[colno] / (NDATA * xbar * xbar) < TINY))
			{
				/* if constant not in model or the
	                           column is too close to constant */
				ss[colno] += NDATA * xbar * xbar;
			}
			j1 += ndata;
		} /* for (colno = 0; colno < nregx; colno++) */
	} /* if(control & DOSWEEP) */

	if(control & DOSS)
	{ /* compute raw SS or SSCP in error SS or error SSCP */
		updateRegss(JUSTRSS);

		testNproducts(interruptExit);

#ifdef HASINFINITY
		if (INTERRUPT & FPESET)
		{
			goto overflowExit;
		}
#endif /*HASINFINITY*/
	} /*if(control & DOSS)*/
	
	/* now do gramschmidt */

	l1 = 0; /* colno * ndata */
	l2 = 0; /* colno * nregx */
	for (colno = 0; colno < nregx; colno++)
	{			/* for each column */
		if(control & DOSWEEP)
		{
			tmpxx = DDOT(&ndata, *REGX + l1, &incx, *REGX + l1, &incy);
			incNproducts(ndata);
			testNproducts(interruptExit);
			
#ifdef HASINFINITY
			if (isInfinite(tmpxx))
			{
				goto overflowExit;
			} /*if (isInfinite(tmpxx))*/			
#endif /*HASINFINITY*/		
		}
		/* check on singularity */
		if ((control & DOSWEEP) && tmpxx <= REGTOL * ss[colno] || ss[colno] < -.5)
		{		/* singular */
			/* Remark: I don't think the following check is necessary */
			if(ss[colno] >= -.5)
			{
				ss[colno] = -1.;
				aliased++; /* count number of singular columns */
				if(control & DORINV)
				{
					xtxinvij = xtxinv + l2; /* *xtxinvij is xtx(i,colno) */
					for (i = 0; i <= colno; i++)
					{/* zero column of xtxinv */
						*xtxinvij++ = 0.0;
					}
				} /*if(control & DORINV)*/
			} /*if(ss[colno] >= -.5)*/			
		} /*if (tmpxx <= REGTOL * ss[colno])*/
		else
		{
			/* not singular */
			REGDIM++;
			if (control & DOSWEEP)
			{/* normalize colno-th X-variable and sweep it out of later ones */
				tmpxx = sqrt(tmpxx);
				regx1 = *REGX + l1; /* *regx1 is x(i,colno) */
				/* normalize X-variables */
				for (i = 0; i < ndata; i++)
				{
					*regx1++ /= tmpxx;
				}
				incNproducts(ndata);
				if(control & DORINV)
				{
					xtxinvij = xtxinv + l2; /* *xtxinvij is xtx(i,colno) */
					/* scale corresponding column of xtxinv */
					for (i = 0; i <= colno; i++)
					{
						*xtxinvij++ /= tmpxx;
					}
					incNproducts(ndata);
				}
				
				/* sweep col colno out of later cols */
				l3 = (colno + 1)*ndata; /* j2 * ndata */
				l4 = (colno + 1)*nregx; /* j2 * nregx */
				for (j2 = colno + 1; j2 < nregx; j2++)
				{
					if (interrupted(DEFAULTTICKS) != INTNOTSET)
					{
						goto interruptExit;
					}
					tmpxy = -DDOT(&ndata, *REGX+l1, &incx, *REGX+l3, &incy);
					incNproducts(ndata);
#ifdef HASINFINITY
					if (isInfinite(tmpxy))
					{
						goto overflowExit;
					} /*if (isInfinite(tmpxy))*/			
#endif /*HASINFINITY*/		
					DAXPY(&ndata, &tmpxy, *REGX+l1, &incx, *REGX+l3, &incy);
					incNproducts(ndata);
					l3 += ndata;
					if(control & DORINV)
					{ /* update upper triangle of xtxinv */
						length = colno + 1;
						DAXPY(&length, &tmpxy,
							  xtxinv + l2, &incx, xtxinv + l4, &incy);
						incNproducts(length);
						l4 += nregx;
					} /*if(control & DORINV)*/					
					testNproducts(interruptExit);
				} /*for (j2 = colno + 1; j2 < nregx; j2++)*/
			} /* if(control & DOSWEEP) */

			/* sweep out of y and save coefficient(s)s */
			if(ny == 1)
			{
				tmpxy = -DDOT(&ndata, regx + l1, &incx, residuals, &incy);
				incNproducts(ndata);
#ifdef HASINFINITY
				if (isInfinite(tmpxy))
				{
					goto overflowExit;
				} /*if (isInfinite(tmpxy))*/			
#endif /*HASINFINITY*/		
				DAXPY(&ndata, &tmpxy, regx + l1, &incx, residuals, &incy);
				incNproducts(ndata);
				testNproducts(interruptExit);
				/* coefs */
				if(control & DOCOEFS)
				{
					coef[colno] = -tmpxy;
				}
			} /*if(ny == 1)*/
			else
			{
				l3 = 0;			/* k2 * ndata */
				l4 = colno;			/* colno + k2 * nregx */
				for (k2 = 0; k2 < ny; k2++)
				{
					tmpxy = -DDOT(&ndata, regx + l1, &incx, residuals + l3,
								   &incy);
					incNproducts(ndata);
#ifdef HASINFINITY
					if (isInfinite(tmpxy))
					{
						goto overflowExit;
					} /*if (isInfinite(tmpxy))*/			
#endif /*HASINFINITY*/		
					DAXPY(&ndata, &tmpxy, regx + l1, &incx, residuals + l3,
						   &incy);
					incNproducts(ndata);
					testNproducts(interruptExit);
					l3 += ndata;
					/* coefs */
					if(control & DOCOEFS)
					{
						coef[l4] = -tmpxy;
						l4 += nregx;
					}					
				} /*for (k2 = 0; k2 < ny; k2++)*/
			} /*if(ny == 1){...}else{...}*/

			if(control & DOSS)
			{ /* compute updated RSS and compute SS for term */
				updateRegss(colno);
				testNproducts(interruptExit);
#ifdef HASINFINITY
				if (INTERRUPT & FPESET)
				{
					goto overflowExit;
				}
#endif /*HASINFINITY*/
			} /*if(control & DOSS)*/
		} /*if (tmpxx <= REGTOL * ss[colno]){...}else{...}*/
		l1 += ndata;
		l2 += nregx;
	} /*for (colno = 0; colno < nregx; colno++)*/

/*
   The code replacing the code skipped by ifdef UNDEFINED__
   saves over 95% in time in certain cases, (19 non-empty cells in a
   6 by 23 by 7 by 5 factorial (4830 cells) with 130 cases and
   model y = a*b + c + d.a.b.  xtxinv is 810 by 810
   Practically all the savings is in the code computing xtxinv xtxinv'
*/
	/* now fix up coef and compute xtxinv */

	if(control & DOCOEFS)
	{ 
		/* fixup coefficients */
#ifdef NOCACHE  /* no cache code not up-to-date */
		for (colno = 0; colno < nregx; colno++)
		{					/* for each coefficient */
			if (interrupted(DEFAULTTICKS) != INTNOTSET)
			{
				goto interruptExit;
			}

			if (ss[colno] >= -0.5)
			{				/* not removed */
				l1 = colno;		/* colno + k * nregx */
				for (k = 0; k < ny; k++)
				{
					tmpxy = 0;
					l2 = l1; /* j2 + k * nregx */
					l3 = colno*nregx; /* colno + j2 * nregx */
					for (j2 = colno; j2 < nregx; j2++)
					{		/* remember, upper triangular */
						if (ss[j2] > -.5)
						{
							/* tmpxy += xtx(colno, j2) * (*REGCOEF)[l2];*/
							tmpxy += xtxinv[l3] * coef[l2];
						}
						l2++;
						l3 += nregx;
					} /*for (j2 = colno; j2 < nregx; j2++)*/
					incNproducts(nregx-colno);
					coef[l1] = tmpxy;
					l1 += nregx;
				} /*for (k = 0; k < ny; k++)*/
			} /*if (ss[colno] >= -0.5)*/
		} /*for (colno = 0; colno < nregx; colno++)*/
#else /*NOCACHE*/
		PRINTstuff();
		
		for (k = 0; k < ny; k++)
		{
			/* use first column of xtxinv as scratch */
			regx1 = xtxinv + 1;
			for (i = 1; i < nregx; i++)
			{ /* zero space used to accumulate stuff */
				*regx1++ = 0.0;
			}
		
			l1 = 0; /* colno * nregx */
			l2 = k * nregx; /* colno + k * nregx */
			for (colno = 0; colno < nregx; colno++)
			{
				if (interrupted(DEFAULTTICKS) != INTNOTSET)
				{
					goto interruptExit;
				}
				if (ss[colno] >= -0.5)
				{/* not removed */
					tmpxy = coef[l2];
					k2 = (colno < nregx - 1) ? colno : (nregx - 2);
					length = k2 + 1;
					DAXPY(&length, &tmpxy, xtxinv + l1, &incx, xtxinv+1, &incy);
					incNproducts(length);
					
				} /*if (ss[colno] >= -0.5)*/
				l1 += nregx;
				l2++;
			} /*for (colno = 0; colno < nregx; colno++)*/
			PRINTstuff();
			testNproducts(interruptExit);
			regx1 = xtxinv + 1;
			coefs2 = coef + k * nregx;

			length = nregx - 1;
			doubleCopy(regx1, coefs2, length);
			colno = nregx-1;
			PRINTstuff();
			coefs2[length] *= xtxinv[nregx*nregx-1];
		} /*for (k = 0; k < ny; k++)*/
#endif /*NOCACHE*/

	/* 
	   Now fix up xtxinv, effectively computing xtxinv = xtxinv xtxinv'
	*/

#ifdef NOCACHE2
	/* work from bottom up so that we don't overwrite ourselves */

		for (j1 = 0; j1 < nregx; j1++)
		{
			if (interrupted(DEFAULTTICKS) != INTNOTSET)
			{
				goto interruptExit;
			}
			l1 = nregx - 1 + j1*nregx; /* j2 + j1 * nregx */
			for (j2 = nregx - 1; j2 >= j1; j2--)
			{
				if (ss[j1] < -0.5 || ss[j2] < -0.5)
				{
					xtxinv[l1] = 0.0; /* xtx(j2, j1) = 0.;*/
				} /*if (ss[j1] < -0.5 || ss[j2] < -0.5)*/
				else
				{
					regx1 = xtxinv + j1 + j2*nregx;
					regx2 = xtxinv + j2 + j2*nregx;
					tmpxy = 0.;
					for (i = j2; i < nregx; i++)
					{
						if (ss[i] > -0.5)
						{
							tmpxy += *regx1 * *regx2;
						}
						regx1 += nregx;
						regx2 += nregx;
					} /*for (i = j2; i < nregx; i++)*/
					/* xtx(j2, j1) = tmpxy;*/
					xtxinv[l1] = tmpxy;
				} /*if (ss[j1] < -0.5 || ss[j2] < -0.5){...}else{...}*/
				l1--;
			} /*for (j2 = nregx - 1; j2 >= j1; j2--)*/
		} /*for (j1 = 0; j1 < nregx; j1++)*/

#else /*NOCACHE2*/

/*
   The following is the strech of code that took most of the time in the
   test case described above.  It is unnecessary if singular columns
   of xtxinv are zero'd when discovered.
*/
#if (0)
		for (j = 0; j < nregx; j++)
		{
			if(ss[j] < -.5)
			{
				regx1 = xtxinv + j*nregx;
				for(i=0;i<=j;i++)
				{
					*regx1++ = 0.0;
				}
			}
		} /*for (j = 0; j < nregx; j++)*/
#endif /*0*/
		for (i = 0; i < nregx; i++)
		{
			regx1 = xtxinv + i * nregx;
			regx1[i] *= regx1[i];
			for (j = i + 1; j < nregx; j++)
			{
				regx1[j] = 0.;
			}
	
			regx2 = regx1;		/* xtxinv + j * nregx */
			for (j = i + 1; j < nregx; j++)
			{
				regx2 += nregx;

				length = j - i + 1;
				DAXPY(&length, regx2 + i, regx2 + i, &incx, regx1+i, &incy);
				incNproducts(length);
			} /*for (j = i + 1; j < nregx; j++)*/
			testNproducts(interruptExit);
		} /*for (i = 0; i < nregx; i++)*/

#endif /*NOCACHE2*/

		for (j = 0; j < nregx; j++)
		{
			length = j + 1;
			DCOPY(&length, xtxinv + j, &nregx, xtxinv + j*nregx, &incy);
		} /*for (j = 0; j < nregx; j++)*/
	} /*if(control & DOCOEFS)*/
	
	if (aliased > 0 && control & DONOTIFY)
	{
		sprintf(OUTSTR,
				"WARNING: %ld singular column%s removed", 
				aliased, (aliased == 1) ? "" : "s");
		putErrorOUTSTR();
	}
	
	/* fall through */

  interruptExit:
	return;
	
  overflowExit:
	strcpy(OUTSTR,
		   "ERROR: arithmetic overflow in Gram-Schmidt orthogonalization");
	putErrorOUTSTR();
	INTERRUPT = FPESET;
} /*gramschmidt()*/

