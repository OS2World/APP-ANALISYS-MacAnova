/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 4.02 or later
*(C)*
*(C)* Copyright (c) 1996 by Gary Oehlert and Christopher Bingham
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
#pragma segment Unbalano
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

/*
  routine for computing ss and df for unbalanced designs

  RESIDUALS	handle to response vector which is replaced by residuals
  residuals	pointer to *RESIDUALS
  X		array of handles to class variables
  NVARS	numberof variables (columns of x)
  NCLASSES	number of classes in each variable
  NDATA	number of data values
  MODEL	handle giving terms in order to be used in model
	  term i contains the variables in the binary expansion of i
	  0 is constant; 3 is AB; 5 is AC etc
  SS	handle to term sums of squares
  DF	handle of term df
*/

long             unbalanova(void)
{
	long            i, j, k, k1, k2;
	long            step1, step2, step3, step4;
	long            ny = (long) NY, ndata = (long) NDATA, nregx = (long) NREGX;
	long            nterms = (long) NTERMS;
	long            ik1, ik1a, ik1k2, ik1k2a;
	long            control = MODELTYPE & DOCOEFS;
	long            errordf;
	double         *hii;
	double         *allwts, *residuals, *wtdresiduals, *misswts, *casewts;
	double         *regx, *ss, *regss, *xij, *df;
	WHERE("unbalanova");

	SETUPINT(errorExit);
	step1 = nterms + 1;
	step2 = ny * step1;
	step3 = nregx + 1;
	step4 = ny * step3;

	/* setup xtxinv if required */
	if(control & DOCOEFS)
	{
		xtxinvInit();
		control |= DORINV;/* compute R^(-1) when computing coefficients */
	} /*if(control & DOCOEFS)*/

	/* notify of singular columns only if there are no factors */
	control |= ((NFACTORS > 0) ? DONTNOTIFY : DONOTIFY) | DOSWEEP | DOSS;


	if (MODELTYPE & USEWEIGHTS)
	{
		ALLWTS = (double **) mygethandle(ndata * sizeof(double));
		if (ALLWTS == (double **) 0)
		{
			goto errorExit;
		}

		allwts = *ALLWTS;
		misswts = (MODELTYPE & MISSWEIGHTS) ? *MISSWTS : (double *) 0;
		casewts = (MODELTYPE & CASEWEIGHTS) ? *CASEWTS : (double *) 0;
		regx = *REGX;
		residuals = *RESIDUALS;

		for (i = 0; i < ndata; i++)
		{
			allwts[i] = 1.0;
			if (misswts != (double *) 0)
			{
				allwts[i] *= misswts[i];
			}
			if (casewts != (double *) 0 && allwts[i])
			{
				allwts[i] *= sqrt(casewts[i]);
			}
		} /*for (i = 0; i < ndata; i++)*/

		xij = regx;
		for(j = 0;j < nregx; j++)
		{
			for(i = 0;i < ndata; i++)
			{
				xij[i] *= allwts[i];
			}
			xij += ndata;
		}
		for (j = 0; j < ny; j++)
		{
			for (i = 0; i < ndata; i++)
			{
				residuals[i] *= allwts[i];
			}
			residuals += ndata;
		} /*for (j = 0; j < ny; j++)*/
	} /*if (MODELTYPE & USEWEIGHTS)*/

	gramschmidt(control);

	checkInterrupt(errorExit);

	/* compute SS */
	if (GLMCONTROL & MARGINALSS)
	{
		if (!margSS())
		{
			goto errorExit;
		}
	}
	else
	{
		seqSS();
	}

	/* Compute actual non-aliased DF */

	compDF();

	df = *DF;
	ss = *SS;
	regss = *REGSS;
	errordf = (long) df[nterms];

	/* Copy residual SS or SSCP to SS */
	if(ny == 1)
	{
		if(errordf == 0)
		{
			regss[nregx] = 0.0;
		}
		ss[nterms] = regss[nregx];
	} /*if(ny == 1)*/
	else
	{
		ik1 = nterms; /* nterms + k1 * step1 */
		ik1a = nregx; /* nregx + k1 * (nregx + 1) */
		for (k1 = 0; k1 < ny; k1++)
		{
			ik1k2 = ik1; /* nterms + k1 * step1 + k2 * (ny * (nregx + 1)) */
			ik1k2a = ik1a; /* nregx + k1 * (nregx + 1) + k2 * (ny * (nregx + 1)) */
			for (k2 = 0; k2 < ny; k2++)
			{
				if(errordf == 0)
				{
					regss[ik1k2a] = 0.0;
				}
				ss[ik1k2] = regss[ik1k2a]; /* error ss */
				/*
				   ss[nterms + k1 * step1 + k2 * step2] =
				   regss[nregx + k1 * (nregx + 1) + k2 * (ny * (nregx + 1))];
				*/
				ik1k2 += step2;
				ik1k2a += step4;
			} /*for (k2 = 0; k2 < ny; k2++)*/
			ik1 += step1;
			ik1a += step3;
		} /*for (k1 = 0; k1 < ny; k1++)*/
	} /*if(ny == 1){}else{}*/
	
	
	/* fix up HII */
	misswts = (MODELTYPE & MISSWEIGHTS) ? *MISSWTS : (double *) 0;
	
	hii = *HII;

	for (i = 0; i < ndata; i++)
	{
		if (misswts == (double *) 0 || misswts[i] != 0.)
		{
			hii[i] = 0.0;
		}
		else
		{
			setMissing(hii[i]);
		}
	} /*for (i = 0; i < ndata; i++)*/
	
	xij = *REGX;
	
	for(j = 0; j < nregx; j++)
	{
		if (regss[j] > -0.5)
		{
			for (i = 0;i < ndata; i++)
			{
				if(!isMissing(hii[i]))
				{
					hii[i] += xij[i] * xij[i];
				}
			}
		} /*if (regss[j] > -0.5)*/		
		xij += ndata;
	} /*for(j = 0; j < nregx; j++)*/
	
	/* fix up RESIDUALS if needed */

	/* fix up real residuals if case or iter weights used */
	if (MODELTYPE & CASEWEIGHTS)
	{
		TMPHANDLE = (char **) WTDRESIDUALS;
		WTDRESIDUALS = RESIDUALS;	/* our residuals were actually weighted */
		RESIDUALS = (double **) TMPHANDLE;
		allwts = *ALLWTS;
		residuals = *RESIDUALS;
		wtdresiduals = *WTDRESIDUALS;
		for(k = 0; k < ny; k++)
		{
			for (i = 0; i < ndata; i++)
			{
				if (allwts[i] > 0.0)
				{
					residuals[i] = wtdresiduals[i] / allwts[i];
				}
				else
				{
					/*
					   set residuals and wtdresiduals MISSING for cases with zero case weight
					   or missing data
					   */
					setMissing(residuals[i]);
					setMissing(wtdresiduals[i]);
				}
			} /*for (i = 0; i < ndata; i++)*/
			residuals += ndata;
			wtdresiduals += ndata;
		} /*for(k = 0; k < ny; k++)*/
	} /*if (MODELTYPE & CASEWEIGHTS)*/
	else if (MODELTYPE & MISSWEIGHTS)
	{
		residuals = *RESIDUALS;
		for(k = 0; k < ny; k++)
		{
			for (i = 0; i < ndata; i++)
			{
				if (misswts[i] == 0.0)
				{
					setMissing(residuals[i]);
				}
			}
			residuals += ndata;
		}
	}

	return (2);

  errorExit:
	return (0);
	
} /*unbalanova()*/

