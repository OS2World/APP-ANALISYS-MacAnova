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
#pragma segment Iterglm
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

/* define new names used here and iterfns.c */
#define NewresidH     RESIDUALS
#define FitH          WTDRESIDUALS    
#define LastresidH    HII             

/*
  Routine for controlling model fitting by iterative least squares.
  maxiter     maximum number of iterations allowed
  epsilon     convergence criterion for change in objective function

  Other parameters may be in global vector GlmGlobals.  For example, for
  robust, RobTruncation, defined in glm.h to be GlmGlobals[0], is
  the truncation point for the Huber psi-function.  It is referenced in
  appropriate functions in iterfns.c.  At present (951016), only robust()
  uses GlmGlobals.

  RESIDUALS	handle to response vector (which will be changed)
  NewresidH defined to be RESIDUALS
  newresid[i] is (*RESIDUALS)[i]
  SS	handle to vector or sums of squares for each term
  DF	handle of term df

  951016 Modified post-fitting computation for robust() so as to compute
         proper (approximate) ANOVA sums of squares following the
         suggestion of Huber (1981) eq. 7.10.5 and 7.10.6

  980710 Added argument clamp

  990212 Changed putOUTSTR() to putErrorOUTSTR()
*/

long iterglm(long maxiter, double epsilon, double clamp)
{
	long            i, j, k, k2, jk, ifit;
	long            ndata = (long) NDATA; /* number of cases */
	long            nregx, nterms = (long) NTERMS;
	long            nfits, errordf;
	long            ny = (long) NY;
	long            robustreg = (MODELTYPE & ROBUSTREG);
	long            binomreg = (MODELTYPE & BINOMREG);
	long            doCoefs = MODELTYPE & DOCOEFS;
	long            doRinv = (doCoefs) ? DORINV : DONTDORINV;
	double         *newresid, *lastresid, *fit;
	double          weight, xval;
	double          curdev, olddev, newdev;
	double         *regx, *regx2, *allwts, *misswts;
	double         *regss, *df, *hii, *ss;
	long            returnValue = ITERERROR;
	WHERE("iterglm");
	
	ALLWTS = (double **) mygethandle(ndata * sizeof(double));
	if (ALLWTS == (double **) 0)
	{
		goto errorExit;
	}
	/*
	   No further memory allocation after this point.  If this should
	   change, more attention will need to be paid to when handles should
	   be dereferenced.
	*/
	newresid = *NewresidH; /* done after ALLWTS allocated */
	ss = *SS;
	regss = *REGSS;
	df = *DF;
	allwts = *ALLWTS;
	regx = *REGX;

	if (binomreg)
	{
		BinomPlimit = clamp;
		HitPlimit = 0;
	}
	
	if (robustreg)
	{
		/*
		  Do Gramschmidt for full model once and for all with DOSWEEP
		  Use DONTDOSWEEP in later calls to gramschmidt()
		*/
		if(doCoefs)
		{
			xtxinvInit();
		}
		
		gramschmidt(DONTNOTIFY | DOSWEEP | DONTDOSS | doRinv | DONTDOCOEFS);
		checkInterrupt(errorExit);
	} /*if (robustreg)*/

	/* zero out SS */
	doubleFill(ss, 0.0, (nterms + 1)*ny*ny);

	if (INCREMENTAL)
	{
		nregx = 0;
		nfits = nterms;
	} /*if (INCREMENTAL)*/
	else
	{
		nregx = (long) NREGX;
		nfits = 1;
	}
	

	for (ifit = 0; ifit < nfits; ifit++)
	{
		if(INCREMENTAL)
		{
			NREGX = (double) (nregx += df[ifit]);
		}
			
		olddev = -10.0e30;
		glmlink (INITIAL);
		checkInterrupt(errorExit);
		glmvar (INITIAL);
		if(ifit == 0)
		{
			curdev = glmdev (INITIAL);
		}

		if(!robustreg)
		{
			regx2 = *REGX2;
		}
		
		for (k = 0; k < maxiter; k++)
		{
			for (j = 0; j < ndata; j++)
			{
				weight = allwts[j];
				if(weight != 0.0)
				{
					if(!robustreg)
					{
						jk = j;	/* j + k2*ndata */
						for (k2 = 0; k2 < nregx; k2++)
						{
							regx[jk] = weight*regx2[jk];
							jk += ndata;
						}
					} /*if(!robustreg)*/
					
					jk = j;		/* j + k2*ndata */
					for (k2 = 0; k2 < ny; k2++)
					{
						newresid[jk] *= weight;
						jk += ndata;
					} /*for (k2 = 0; k2 < ny; k2++)*/
				} /*if(weight != 0.0)*/				
			} /*for (j = 0; j < ndata; j++)*/

			/*
			   Note, coefficients not computed until iteration is done
			   Also, for robust(), X-variables are already orthogonalized
			   so no further sweeping is done
			*/
			gramschmidt(DONTNOTIFY | ((robustreg) ? DONTDOSWEEP : DOSWEEP) |
						DONTDOSS | DONTDORINV | DONTDOCOEFS);

			checkInterrupt(errorExit);

			newdev = glmdev (STEP);

			if (fabs(olddev - newdev) <= epsilon * fabs(olddev + newdev))
			{ /* converence achieved */
				break;
			}
			olddev = newdev;
			if (k < maxiter - 1)
			{
				glmlink (STEP);
				checkInterrupt(errorExit);
				glmvar (STEP);
			} /*if (k < maxiter - 1)*/
		} /*for (k = 0; k < maxiter; k++)*/

		if(k == maxiter)
		{
			if(INCREMENTAL && ifit < nterms-1)
			{
				sprintf(OUTSTR,
						"WARNING: term %ld not fully converged in %ld iterations",
						ifit + 1, maxiter);
			}
			else
			{
				sprintf(OUTSTR,
						"WARNING: final deviance not fully converged in %ld iterations",
						maxiter);
			}
			
			putErrorOUTSTR();
			returnValue = ITERUNCONVERGED;
		} /*if(k == maxiter)*/
			
		if (!robustreg) /*robust() computes SS later*/
		{
			if(INCREMENTAL)
			{
				ss[ifit] = curdev - newdev;	/* set dev for term */
				curdev = newdev;
			}
			else
			{
				ss[nterms-1] = curdev - newdev;
			}
		} /*if (!robustreg)*/
	} /* for(ifit=0;ifit<nfits;ifit++)*/

	if (!robustreg)
	{
		ss[nterms] = newdev;
	}
	
	/* Now do one final Gram-Schmidt to get final coefficients */

	/* Setup xtxinv if needed */

	if(!robustreg && doCoefs)
	{
		xtxinvInit();
	}

	glmlink (STEP);

	checkInterrupt(errorExit);
	glmvar (STEP);

	if (!robustreg)
	{
		for (k2 = 0; k2 < nregx; k2++)
		{
			for (i = 0; i < ndata; i++)
			{
				regx[i] = allwts[i]*regx2[i];
			} /*for (i = 0; i < ndata; i++)*/
			regx += ndata;
			regx2 += ndata;
		} /*for (k2 = 0; k2 < nregx; k2++)*/
			
		for (k2 = 0; k2 < ny; k2++)
		{
			for (i = 0; i < ndata; i++)
			{
				newresid[i] *= allwts[i];
			} /*for (i = 0; i < ndata; i++)*/
			newresid += ndata;
		} /*for (k2 = 0; k2 < ny; k2++)*/

		gramschmidt(DONTNOTIFY | ((robustreg) ? DONTDOSWEEP : DOSWEEP) |
					DONTDOSS | ((robustreg) ? DONTDORINV : doRinv) | doCoefs);
	} /*if(!robustreg)*/
	else
	{
		/*
		   Create pseudo data equal to to fit + rescaled modified residuals
		   following the suggestion of Huber (1981) eq. 7.10.5 and 7.5.6
		   At this point, newresid contains what would be dependent variable
		   in the next iteration and hence is expendable and lastresid
		   contains the modified resituals
           CURRENTRSC*psi((y[i]-fit[i])/CURRENTRSC)
		*/
		fit = *FitH; /*WTDRESIDUALS*/
		newresid = *NewresidH; /*RESIDUALS*/
		lastresid = *LastresidH; /*HII*/
		
		for (i = 0; i < ndata;i++)
		{
			newresid[i] = (allwts[i] != 0.0) ?
				fit[i] + ROBUSTVARK*lastresid[i] : 0.0;
		}

		/* Compute approximate sums of squares */
		gramschmidt(DOSS | DONTNOTIFY | DONTDOSWEEP | DONTDORINV | doCoefs);

		doubleCopy(lastresid, newresid, ndata);
	} /*if(!robustreg){}else{}*/
	
	if (binomreg && HitPlimit)
	{
		sprintf(OUTSTR,
				"WARNING: problimit = %.5g was hit by %s() at least once",
				BinomPlimit, FUNCNAME);
		putErrorOUTSTR();
	} /*if (binomreg && HitPlimit)*/
	
	checkInterrupt(errorExit);

	/*
	   Put y[i] - fit[i] in RESIDUALS and
	   modified or weighted resdiduals in WTDRESIDUALS.
	*/
	glmresid();


	/* fix up DF and, if robustreg, compute ss */

	if (robustreg)
	{
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
	} /*if (robustreg)*/

	/* compute actual non-aliased DF */
	compDF();
	
	df = *DF;
	ss = *SS;
	regss = *REGSS;
	regx = *REGX;
	errordf = (long) df[nterms];

	if(errordf == 0.0)
	{
		ss[nterms] = 0.0;
	}
	else if (robustreg)
	{
		ss[nterms] = regss[nregx];
	}

	if (isMissing(GLMSCALE))
	{ /* scale:? was an argument */
		GLMSCALE = (errordf > 0) ? sqrt(ss[nterms]/(double) errordf) : 0.0;
	}
	
	/*
	   fix up HII which has hitherto been used as scratch to hold previous
	   residuals
    */
	hii = *HII;
	misswts = (MODELTYPE & MISSWEIGHTS) ? *MISSWTS : (double *) 0;
	for (i = 0;i < ndata;i++)
	{
		if (misswts == (double *) 0 || misswts[i] != 0.0)
		{
			hii[i] = 0.0;
		}
		else
		{
			setMissing(hii[i]);
		}
	} /*for (i = 0;i < ndata;i++)*/

	for (j = 0; j < nregx; j++)
	{
		if(regss[j] > -0.5)
		{
			for (i = 0;i < ndata;i++)
			{
				if (!isMissing(hii[i]))
				{
					xval = regx[i];
					hii[i] += xval*xval;
				}
			} /*for (i = 0;i < ndata;i++)*/			
		} /*if(regss[j] > -0.5)*/
		regx += ndata;			
	} /*for (j = 0; j < nregx; j++)*/

	returnValue = (returnValue != ITERERROR) ? returnValue : ITERCONVERGED;
/* fall through */

  errorExit:
	*OUTSTR = '\0';

#ifdef MACINTOSH
	UNLOADSEG(glmvar);
#endif /*MACINTOSH*/
	
	return (returnValue);
	
} /*iterglm()*/
