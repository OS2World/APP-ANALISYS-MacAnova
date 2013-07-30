/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 4.06 or later
*(C)*
*(C)* Copyright (c) 1998 by Gary Oehlert and Christopher Bingham
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
#pragma segment Ipf
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

extern void            getterm();
extern double          cellstat();
extern long            buildXmatrix();

/*

 	routine for computing ss and df for balanced designs

	RESIDUALS	handle to response vector (which will be changed)
	residuals	pointer to residuals
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

enum ipfScratch
{
	GMEANS = 0,
	GFITS  = 0,
	GOMARG,
	GFMARG,
	NTRASH
};

static double compDev(double *y, double *fits, long ndata)
{
	long          i;
	double        yi, dev = 0.0;
	WHERE("compDev");
	
	for (i = 0; i < ndata; i++)
	{
		yi = y[i];

		if (yi > 0.0)
		{
			dev += yi * log(yi/fits[i]) - yi + fits[i];
		}
		else
		{
			dev += fits[i];
		}
	} /*for (i = 0; i < ndata; i++)*/
	return (2.0 * dev);
} /*compDev()*/

long ipf(long maxiter, double epsilon)
{
	long            i, j, k, term[MAXVARS];
	modelPointer    iterm;
	double          curdev, newdev, olddev, curss, newss;
	double        **meansH = (double **) 0;
	double        **fitsH = (double **) 0, **omargH = (double **) 0;
	double        **fmargH = (double **) 0;
	double         *fits, *omarg, *fmarg, *y;
	double         *ss, *df;
	double         *residuals, *wtdresiduals;
	double          yi;
	long            istart, ii, icell;
	long            ndata = (long) NDATA, nterms = (long) NTERMS;
	long            nvars = (long) NVARS;
	long            nfactors = (long) NFACTORS, ncells, space = 0;
	long            iter;
	long            returnValue = ITERERROR;
	WHERE("ipf");
	TRASH(NTRASH,errorExit);
	
	/* first compute maximum possible df's */

	SETUPINT(errorExit);
	
	k = 0;			/* total of cols before this term */
	df = *DF;
	for (i = 0; i < nterms; i++)
	{/* for each term */
		df[i] = (double) buildXmatrix(i, (double *) 0, MODELINFO);
	}

	df[nterms] = ndata;
	for (i = 0; i < nterms; i++)
	{
		df[nterms] -= df[i];
	}

	ncells = 1;
	
	for (i = 0; i < nfactors; i++)
	{
		ncells *= NCLASSES[i];
	}
	
	/* zero out SS */
	doubleFill(*SS, 0.0, nterms+1);

	/* now go through terms in model getting residual ss for each term
	   and sweeping out effects */

	
   	if (MODELTYPE & ANOVA)
	{
		/* find maximum marginal table size */
		for (i = 0;i < nterms;i++)
		{
			getterm(modelterm(MODEL,i), term, nvars);
			ncells = setterm(term, nvars, NCLASSES);
			space = (ncells > space) ? ncells : space;
		}
		
		if (!getScratch(meansH,GMEANS,space,double))
		{
			goto errorExit;
		}
		residuals = *RESIDUALS;
		ss = *SS;
		df = *DF;

		curss = 0.0;
		for (i = 0; i < ndata; i++)
		{
			curss += residuals[i] * residuals[i];	/*  get total SS */
		}
		
		for (i = 0; i < nterms; i++)
		{
			if (interrupted(DEFAULTTICKS) != INTNOTSET)
			{
				goto errorExit;
			}
			 /* get means and sweep out term*/
			getterm(modelterm(MODEL,i), term, nvars);
			newss = cellstat(RESIDUALS, term, meansH,
							 (double **) 0, (double **) 0, CELLMEANSANDSWEEP);
			/* set ss for term */
			ss[i] = (curss > newss) ? curss - newss : 0.0;
			curss = newss;
		} /*for (i = 0; i < nterms; i++)*/

		ss[nterms] = (df[nterms] > 0 ) ? curss : 0.0;	/* error ss */
	} /*if (MODELTYPE & ANOVA)*/
	else
	{/* IPF */
		if (!getScratch(fitsH,GFITS,ndata,double) ||
		   !getScratch(omargH,GOMARG,ndata,double) ||
		   !getScratch(fmargH,GFMARG,ndata,double))
		{
			goto errorExit;
		}
		fits = *fitsH;
		omarg = *omargH;
		fmarg = *fmargH;
		ss = *SS;
		residuals = *RESIDUALS;
		wtdresiduals = *WTDRESIDUALS;
		y = *Y;

		doubleFill(fits, 1.0, ndata);
		curdev = compDev(y, fits, ndata);
		
		if (INCREMENTAL)
		{
			istart = 1;
		}
		else
		{
			istart = nterms;
		}
		for (ii = istart; ii <= nterms; ii++)
		{
			newdev = 0.;
			olddev = -1.;
			iter = 0;
			while (fabs(olddev - newdev) / fabs(olddev + newdev) > epsilon)
			{
				if (++iter > maxiter)
				{
					returnValue = ITERUNCONVERGED;
					break;
				}
				
				olddev = newdev;
				for (i = 0; i < ii; i++)
				{
					if (interrupted(DEFAULTTICKS) != INTNOTSET)
					{
						goto errorExit;
					}
					iterm = modelterm(MODEL,i);
					getterm(iterm, term, nvars);
					/*get marginal sums of data for term*/
					(void) cellstat(Y, term, omargH,
									(double **) 0, (double **) 0, CELLSUMS);
					/* get marginal sums of current fits*/
					getterm(iterm, term, nvars);
					(void) cellstat(fitsH, term, fmargH,
									(double **) 0, (double **) 0, CELLSUMS);
					/* note cellstat() modifies term to contain offsets */
					for (j = 0; j < ndata; j++)
					{
						icell = 0;
						for (k = 0; k < nvars; k++)
						{
							icell += term[k] * ((*X[k])[j] - 1);
						}
						if (fmarg[icell] > 0.)
						{ /* adjust fits */
							fits[j] *= omarg[icell] / fmarg[icell];
						}
						else
						{
							fits[j] = 0.;
						}
					} /*for (j = 0; j < ndata; j++)*/
				} /*for (i = 0; i < ii; i++)*/
				newdev = compDev(y, fits, ndata);
			} /*while (fabs(olddev - newdev) / fabs(olddev + newdev) > epsilon)*/
			ss[ii - 1] = curdev - newdev;
			curdev = newdev;
		} /*for (ii = istart; ii <= nterms; ii++)*/
		ss[nterms] = curdev;

		for (i = 0; i < ndata; i++)
		{
			yi = fits[i];
			residuals[i] = y[i] - yi;
			if (yi > 0.0)
			{
				wtdresiduals[i] = residuals[i] / sqrt(yi);
			}
			else
			{
				wtdresiduals[i] = 0.;
			}
		}
	} /*if (MODELTYPE & ANOVA){}else{}*/

	/* compute HII */
	doubleFill(*HII, (NDATA - (*DF)[nterms]) / NDATA, ndata);

	returnValue = (returnValue != ITERERROR) ? returnValue : ITERCONVERGED;
	/* fall through */

  errorExit:
	emptyTrash(); /* cleanup scratch */
	return (returnValue);

} /*ipf()*/

