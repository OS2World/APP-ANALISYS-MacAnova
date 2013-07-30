/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 4.07 or later
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
#pragma segment Iterfns
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

#define NewresidH     RESIDUALS       
#define FitH          WTDRESIDUALS    
#define LastresidH    HII             

/*
  This module contains three routines for each of the iterative least
  squares algorithms.   fit[i] is (*FitH)[i], newresid[i] is (*NewresidH)[i],
  and lastresid[i] is (*LastresidH)[i].

  In the following descriptions xxxxx stands for robust, poisson, or logit

  xxxxxvar() updates ALLWTS, taking into account MISSWTS, CASEWTS, and
  the current fit.

  xxxxxlink(INITIAL) initializes fit[], newresid[], and lastresid[]
  xxxxxlink(STEP) updates fit[], newresid[], and lastresid[]
	Except for robustlink(STEP), xxxxxlink(op) always sets newresid[i]
	and lastresid[i] the same.

  xxxxxdev(INITIAL) computes initial value of deviance or other criteria
  xxxxxdev(STEP) computes new value of deviance or other criteria

  xxxxxresid() puts the actual residual (*Y)[i] - fit[i] in newresid[i]
  and puts the previous value of newresid[i] in fit[i].  It is called just
  once, after the iterations
*/

/*
   Start of robust regression related code
*/
#if (0) /*951012 moved to glm.h*/
double          CURRENTRSC;
double          ROBUSTVARK; /* this is K from eq 7.7.11 of Huber 1981 */
double          REGDIM;  /* D.F. in Model */
#endif /*(0)*/

/*
   951012  Added globals RobBeta and RobTruncation to glm.h
           RobBeta is computed in glm.c as cumchi(c^2,3)+c^2*(1-cumnor(c)),
           where c = RobTruncation
		   The default value of RobTruncation is .75, resulting in a
		   value for RobBeta of 0.349994911, almost exactly the value
		   .35 set by definition previously

   960815  Hit problem of all residuals being truncated leading to
           floating point exception on DOS.  Made two changes.
           (1)  Went back to using median as starting value for pred;
                this had been changed to 0 so increment associated with
                fitting constant could be computed.  Since SS is
                now computed a completely different way, this no longer
                made sense.
           (2)  Keep doubling initial value of CURRENTRSC until at least
                one residual is not truncated.

			Also, ROBUSTVARK is not computed on any iteration where
            all residuals are truncated.
*/


/*
   Computations for robust() are based on Sec. 7.8 of Huber, Robust Statistics
   Wiley 1981
*/

	/*
	   We include a little slack to reduce dependence on rounding behavior
	   of compiler
	*/
#define ROBFUZZ 5e-15  /* slack for use when comparing with RobTruncation */

/*
  robustvar() updates ALLWTS, taking into account MISSWTS.
*/
static void     robustvar(long op)
{
	long            i;
	long            ndata = (long) NDATA;
	double         *allwts = *ALLWTS;
	double         *misswts = (MODELTYPE & MISSWEIGHTS) ?
		*MISSWTS : (double *) 0;

	for (i = 0; i < ndata; i++)
	{
		allwts[i]  = (misswts == (double *) 0) ? 1.0 : misswts[i];
	} /*for (i = 0; i < ndata; i++)*/
} /*robustvar()*/

/*
  robustlink(INITIAL) initializes fit[], newresid[], and lastresid[], and
  computes RobBeta and starting values
  robustlink(STEP) updates fit[], newresid[], and lastresid[]
*/
static void     robustlink(long op)
{
	long            i, j, k;
	double          z, pred;
	double          sumPsiSq, mu;
	double         *newresid = *NewresidH;
	double         *lastresid = *LastresidH;
	double         *fit = *FitH;
	double         *allwts = *ALLWTS, *y = *Y;
	double         *misswts = (MODELTYPE & MISSWEIGHTS) ?
		*MISSWTS : (double *) 0;
	double          fuzzyRobTrunc = RobTruncation + ROBFUZZ * RobTruncation;
	long            notTruncated;
	long            ndata = (long) NDATA;
	WHERE("robustlink");

	if(op == INITIAL)
	{
		RobBeta = Cchi(RobTruncation*RobTruncation,3.0) +
			2.0*RobTruncation*RobTruncation*Cnor(-RobTruncation);
		/* initialize CURRENTRSC to IQR/(2*invnor(.75)) */
		k = 0;
		for (i = 0; i < ndata; i++)
		{
			if (misswts == (double *) 0 || misswts[i] != 0.0)
			{
				newresid[k++] = y[i];
			}
		} /*for (i = 0; i < ndata; i++)*/

		sortquick(newresid, k);

		i = 3 * k / 4 - 1;
		j = k - 1 - i;
/*
   951012: changed from j = k/4 - 1 to the preceding for reasons of
   symmetry.  This makes a slight difference in results since
   the initial value of CURRENTRSC is propagated through to the end
   Also constant 1.345 changed to 2*invnor(.75) = 1.3489795
*/

		CURRENTRSC = (newresid[i] - newresid[j]) / 1.3489795;
		if (CURRENTRSC == 0.0)
		{
			CURRENTRSC = 1e-10;
		}

#if (1)  /*once again initialize to median*/
		if(modeltermEqual(modelterm(MODEL,0),(modelPointer) NULLTERM))
		{ /* constant term in model, set pred to median */
			i = (k - 1)/ 2;
			j = k/2;
			pred = .5*(newresid[i] + newresid[j]);
		}
		else
#endif /*1*/
		{
			pred = 0.0;
		}
		
		for (j = 0; j < 5; j++)
		{
			sumPsiSq = 0.;
			for (i = 0; i < ndata; i++)
			{
				if (misswts == (double *) 0 || misswts[i] != 0.0)
				{
					z = fabs(y[i] - pred) / CURRENTRSC;
					if (z >= RobTruncation)
					{
						z = RobTruncation;
					}
					sumPsiSq += z * z;
				}
			} /*for (i = 0; i < ndata; i++)*/
			CURRENTRSC = sqrt(sumPsiSq / (NOTMISSING * RobBeta)) * CURRENTRSC;
			if (CURRENTRSC == 0.0)
			{
				CURRENTRSC = 1e-10;
			}
		} /*for (j = 0; j < 5; j++)*/

		/* 
		   Keep doubling CURRENTRSC until at least one residual not
		   truncated
		*/
		while (1)
		{
			for (i = 0; i < ndata; i++)
			{
				if ((misswts == (double *) 0 || misswts[i] != 0.0) &&
					fabs((y[i] - pred) / CURRENTRSC) < RobTruncation)
				{
					break;
				}
			} /*for (i = 0; i < ndata; i++)*/
			if (i < ndata)
			{
				break;
			}
			CURRENTRSC *= 2.0;
		} /*while (1)*/
		
		for (i = 0; i < ndata; i++)
		{
			if (misswts == (double *) 0 || misswts[i] != 0.0)
			{
				z = (y[i] - pred) / CURRENTRSC;
				if (z >= RobTruncation)
				{
					z = fuzzyRobTrunc;
				}
				else if (z <= -RobTruncation)
				{
					z = -fuzzyRobTrunc;
				}
				z *= CURRENTRSC;
				fit[i] = pred;	/* old predicted values*/
				newresid[i] = pred + z; /* modified eta */
				lastresid[i] = z;
			} /*if (misswts == (double *) 0 || misswts[i] != 0.0)*/
			else
			{
				fit[i] = newresid[i] = lastresid[i] = 0.0;
			}
		} /*for (i = 0; i < ndata; i++)*/
	} /*if(op == INITIAL)*/
	else
	{ /*op == STEP*/
		/* Eq. 7.8.19 and 7.8.20 of Huber 1981 */

		for (i = 0; i < ndata; i++)
		{
			if (allwts[i] != 0.0)
			{
				/* update fit[i] by adding latest residual */
				fit[i] += lastresid[i] - newresid[i];
				pred = fit[i];
				/* old eta hat */
				z = (y[i] - pred) / CURRENTRSC;
				if (z >= RobTruncation)
				{
					z = fuzzyRobTrunc;
				}
				else if (z <= -RobTruncation)
				{
					z = -fuzzyRobTrunc;
				}
				z *= CURRENTRSC;
				newresid[i] = pred + z; /* modified eta */
				lastresid[i] = z;
			} /*if (allwts[i] != 0.0)*/
		} /*for (i = 0; i < ndata; i++)*/

	/*
	   fit[i] is latest fitted value for case i
       lastresid[i] = CURRENTRSC*psi((y[i] - fit[i])/CURRENTRSC) is 
	     the most recent modified residual
	   newresid[i] is fit[i] + lastresid[i]
	     It will be dependent variable in next iteration
	*/
		/* update CURRENTRSC */
		sumPsiSq = 0.;
		notTruncated = 0;

		for (i = 0; i < ndata; i++)
		{
			if (allwts[i] > 0.0)
			{
				z = fabs(lastresid[i])/CURRENTRSC;
				if (z >= RobTruncation)
				{
					z = fuzzyRobTrunc;
				}
				else
				{
					notTruncated++;
				}
				sumPsiSq += z * z;
			} /*if (allwts[i] > 0.0)*/
		} /*for (i = 0; i < ndata; i++)*/
		/* eq. 7.6.11 of Huber 1981*/

		if (notTruncated > 0)
		{
			mu = (double) notTruncated / NOTMISSING;
			/* compute K defined on page 40 of Huber 1977 */
			ROBUSTVARK = (1.0+(REGDIM/NOTMISSING)*(1.0-mu)/mu)/ mu;
		} /*if (notTruncated > 0)*/
		
		if (NOTMISSING == REGDIM || sumPsiSq == 0.0)
		{
			CURRENTRSC = 1e-10;
		}
		else
		{
			/* eq. 7.8.7 of Huber 1981 */
			CURRENTRSC *= sqrt(sumPsiSq/((NOTMISSING - REGDIM) * RobBeta));
		}
	} /*if(op == INITIAL){}else{}*/
} /*robustlink()*/

/*
  robustdev(op) ignores op, computing the current value of the criterion
  function using rho-function for eq. 7.7.14 of Huber (1981)
  Value is required to test for convergence of iteration
*/
static double       robustdev(long op)
{
	long            i;
	double          sumrho, z;
	double         *allwts = *ALLWTS, *y = *Y;
	double         *fit = *FitH;
	double          addcon1 = .5*RobBeta;
	double          addcon2 = -.5*RobTruncation*RobTruncation + addcon1;
	long            ndata = (long) NDATA;

	sumrho = 0.;

	for (i = 0; i < ndata; i++)
	{

		if (allwts[i] != 0.0)
		{
			z = fabs(y[i] - fit[i]) / CURRENTRSC;
			if (z < RobTruncation)
			{
				sumrho += 0.5 * z * z + addcon1;
			}
			else
			{
				sumrho += RobTruncation * z + addcon2;
			}
		} /*if (allwts[i] != 0.0)*/
	} /*for (i = 0; i < ndata; i++)*/
	return (sumrho * CURRENTRSC);
} /*robustdev()*/

/*
   Called just once, when iteration is over
   *NewresidH = *RESIDUALS is set to y[i] - fit[i] and
   *FitH = *WTDRESIDUALS is set to fit[i]
*/
static void     robustresid(void)
{
	long            i;
	double         *newresid = *NewresidH;
	double         *fit = *FitH;
	double         *allwts = *ALLWTS, *y = *Y;
	double        **tmpH;
	long            ndata = (long) NDATA;

	for (i = 0; i < ndata; i++)
	{
		if (allwts[i] == 0)
		{
			setMissing(newresid[i]);
			setMissing(fit[i]);
		}
		else
		{
			/* modify predicted to residual in place */
			fit[i] = y[i] - fit[i];
		}
	} /*for (i = 0; i < ndata; i++)*/
	tmpH = FitH;
	FitH = NewresidH;
	NewresidH = tmpH;
} /*robustresid()*/
/*
   End of robust regression related code
*/

/*
   Start of binomial distribution related code
*/
#define normDensity(Z)    0.3989422804014326779*exp(-.5*(Z)*(Z))
  /* constant is 1/sqrt(2*pi) */
/*
   4 private functions, binomvar(), binomlink(), binomdev(), and binomresid()
   for case when (GLMCONTROL & BINOMDIST) != 0
   951023 Probit and Logit link functions implemented

   980710 Value for "clamp" taken from global and macro PCLAMP with
          value 0.00005 removed

*/

/*
  binomvar() updates ALLWTS, taking into account MISSWTS, CASEWTS, and
  the current fit.
*/
static void binomvar(long op)
{
	long            i;
	long            ndata = (long) NDATA;
	double         *allwts = *ALLWTS, *binomn = *LOGITN;
	double         *misswts = (MODELTYPE & MISSWEIGHTS) ?
		*MISSWTS : (double *) 0;
	double         *casewts = (MODELTYPE & CASEWEIGHTS) ?
		*CASEWTS : (double *) 0;
	double         *fit = *FitH;
	double          probpred, pred, z;
	int             logitlink = (GLMCONTROL & LOGITLINK) != 0;
	int             probitlink = (GLMCONTROL & PROBITLINK) != 0;
	
	for (i = 0; i < ndata; i++)
	{
		allwts[i] = (misswts != (double *) 0) ? misswts[i] : 1.0;
		if (casewts != (double *) 0)
		{
			allwts[i] *= sqrt(casewts[i]);
		}
		probpred = fit[i];
		if (allwts[i] > 0.0 && probpred > 0.0 && probpred < 1.0)
		{
			if (logitlink)
			{
				allwts[i] *= sqrt(binomn[i] * (probpred * (1. - probpred)));
			}
			else if (probitlink)
			{
				pred = Qnor(probpred);
				z = normDensity(pred);
				allwts[i] *= z * sqrt(binomn[i] / (probpred * (1. - probpred)));
			}
		} /*if (allwts[i] > 0.0 && probpred > 0.0 && probpred < 1.0)*/
	} /*for (i = 0; i < ndata; i++)*/
} /*binomvar()*/

/*
  binomresid() puts the actual residual (*Y)[i] - fit[i] in newresid[i]
  and puts the previous value of newresid[i] in fit[i].  It is called just
  once, after the iterations
*/

static void     binomresid(void)
{
	long            i;
	double         *newresid = *NewresidH;
	double         *fit = *FitH;
	double         *allwts = *ALLWTS, *y = *Y, *binomn = *LOGITN;
	double        **tmpH;
	long            ndata = (long) NDATA;

	for (i = 0; i < ndata; i++)
	{
		if (allwts[i] == 0)
		{
			setMissing(newresid[i]);
			setMissing(fit[i]);
		}
		else
		{
			/* modify predicted to residual in place */
			fit[i] = (y[i] - binomn[i] * fit[i]);
		}
	} /*for (i = 0; i < ndata; i++)*/
	/* swap FitH (WTDRESIDUALS) and rewresidH (RESIDUALS) */
	tmpH = FitH;
	FitH = NewresidH;
	NewresidH = tmpH;
} /*binomresid()*/

	
/*
  binomdev(INITIAL) computes initial value of deviance
  binomdev(STEP) computes new value of deviance
*/
static double      binomdev(long op)
{
	long            i;
	double         *fit = *FitH;
	double          halfdev, binomni, p, probpred;
	double         *y = *Y, *binomn = *LOGITN;
	double         *offset = (USEGLMOFFSET) ? *GLMOFFSET : (double *) 0;
	double         *allwts = *ALLWTS;
	long            ndata = (long) NDATA;
	int             logitlink = (GLMCONTROL & LOGITLINK) != 0;
	int             probitlink = (GLMCONTROL & PROBITLINK) != 0;
	WHERE("binomdev");
	
	halfdev = 0.;
	if (op == INITIAL)
	{
		for (i = 0; i < ndata; i++)
		{
			if (allwts[i] != 0.0)
			{
				if (!USEGLMOFFSET)
				{
					probpred = 0.5;
				}
				else
				{
					if (logitlink)
					{
						probpred = 1.0/(1.0 + exp(-offset[i]));
					}
					else if (probitlink)
					{
						probpred = Cnor(offset[i]);
					}
				}
				
				binomni = binomn[i];
				p = y[i]/binomni;
				if(p == 0.0) 
				{
					halfdev += binomni*log(1.0/(1.0-probpred));
				}
				else if(p == 1.0) 
				{
					halfdev += binomni * log(1.0/probpred);
				} 
				else 
				{
					halfdev += binomni*
						(p*log(p/probpred) + (1.0 - p)*log((1.0 - p)/(1.0 - probpred)));
				}
			} /*if (allwts[i] != 0.0)*/
		} /*for (i = 0; i < ndata; i++)*/
	} /* if (op == INITIAL) */
	else
	{
		for (i = 0; i < ndata; i++)
		{
			if (allwts[i] != 0.0)
			{
				binomni = binomn[i];
				p = y[i]/binomni;
				probpred = fit[i];
				if (p == 0.0)
				{
					halfdev += binomni * log(1.0 / (1.0 - probpred));
				}
				else if (p == 1.0)
				{
					halfdev += binomni * log(1.0 / probpred);
				}
				else
				{
					halfdev += binomni *
						(p*log(p/probpred) + (1.0 - p)*log((1.0 - p)/(1. - probpred)));
				}
			} /*if (allwts[i] != 0.0)*/
		} /*for (i = 0; i < ndata; i++)*/
	} /* if (op == INITIAL){}else{} */
		return (2.0 * halfdev);
} /*binomdev()*/

/*
  binomlink(INITIAL) initializes fit[], newresid[], and lastresid[]
  binomlink(STEP) updates fit[], newresid[], and lastresid[]
	binomlink(op) always sets newresid[i] and lastresid[i] the same.
*/
static void     binomlink(long op)
{
	long            i;
	double         *newresid = *NewresidH;
	double         *lastresid = *LastresidH;
	double         *fit = *FitH;
	double         *allwts = *ALLWTS, *y = *Y, *binomn = *LOGITN;
	double         *misswts = (MODELTYPE & MISSWEIGHTS) ?
		*MISSWTS : (double *) 0;
	double         *offset = (USEGLMOFFSET) ? *GLMOFFSET : (double *) 0;
	double          probpred, pred, resid, z, denom;
	long            ndata = (long) NDATA;
	int             logitlink = (GLMCONTROL & LOGITLINK) != 0;
	int             probitlink = (GLMCONTROL & PROBITLINK) != 0;
	double          pclamp = BinomPlimit;
	static double   clampL, clampH;
	WHERE("binomlink");

	if(op == INITIAL)
	{
		if (probitlink)
		{
			clampL = Qnor(pclamp);
			clampH = -clampL;
			z = normDensity(0.0);
		}
		else if (logitlink)
		{
			clampL = -log(1.0/pclamp - 1.0);
			clampH = -clampL;
		}
		
		for (i = 0; i < ndata; i++)
		{
			/* predicted values */
			if (misswts == (double *) 0 || misswts[i] > 0.0)
			{
				fit[i] = 0.5;
				resid = y[i] / binomn[i] - 0.5;
				if (probitlink)
				{
					denom = z;
				}
				else if (logitlink)
				{
					denom = .25;
				}
				
				resid /= denom;
				if(USEGLMOFFSET)
				{
					resid -= offset[i];
				}
				lastresid[i] = newresid[i] = resid;
			}
			else
			{
				fit[i] = newresid[i] = lastresid[i] = 0.0;
			}
		} /*for (i = 0; i < ndata; i++)*/
	} /*if(op == INITIAL)*/
	else
	{ /*op == STEP */
		for (i = 0; i < ndata; i++)
		{
			if (allwts[i] != 0.0)
			{
				/* new eta hat */
				pred = lastresid[i] - newresid[i] / allwts[i];
				if(USEGLMOFFSET)
				{
					pred += offset[i];
				}
				/* clamp to prevent going off to infinity */
				if (pred > clampH)
				{
					pred = clampH;
					HitPlimit = 1;
				}
				else if (pred < clampL)
				{
					pred = clampL;
					HitPlimit = 1;
				}
				
				/* 
				   980807 modified code so that probpred <= .5 to avoid
				   too much cancellation when the fitted probability is
				   very close to 1
				 */
				if (logitlink)
				{
					probpred = 1.0/(1.0 + exp(fabs(pred)));
				}
				else if (probitlink)
				{
					probpred = Cnor(-fabs(pred));
				}
				
				if (pred < 0)
				{
					resid = y[i] / binomn[i] - probpred;
				}
				else
				{
					resid = probpred - (binomn[i] - y[i])/binomn[i];
				}
				
				fit[i] = (pred < 0) ? probpred : 1 - probpred;

				if (logitlink)
				{
					denom = probpred*(1.0 - probpred);
				}
				else if (probitlink)
				{
					denom = normDensity(pred);
				}
				
				/* compute working probit or logit */
				resid = pred + resid/denom;
				if(USEGLMOFFSET)
				{
					resid -= offset[i];
				}
				lastresid[i] = newresid[i] = resid;
			} /*if (allwts[i] != 0.0)*/
		} /*for (i = 0; i < ndata; i++)*/
	} /*if(op == INITIAL){}else{}*/
} /*binomlink()*/

/*
   End of binomial distribution related code
*/

/*
   Start of Poisson distribution related code
*/

/*
   Note: the only difference between when op == INITIAL and op != INITIAL
   has to do with the values used for pred
*/
/*
   private functions poissonvar(), poissondev(), poissonlink(), poissonresid()
   Only log link ((GLMCONTROL & LOGLINK) != 0
*/

/*
  poissonvar() updates ALLWTS, taking into account MISSWTS, CASEWTS, and
  the current fit.
*/
static void     poissonvar(long op)
{
	long            i;
	long            ndata = (long) NDATA;
	double         *allwts = *ALLWTS;
	double         *fit = *FitH;
	double         *misswts = (MODELTYPE & MISSWEIGHTS) ?
		*MISSWTS : (double *) 0;
	double         *casewts = (MODELTYPE & CASEWEIGHTS) ?
		*CASEWTS : (double *) 0;
	double          ypred;
	
	for (i = 0; i < ndata; i++)
	{
		allwts[i] = (misswts != (double *) 0) ? misswts[i] : 1.0;
		if (casewts != (double *) 0)
		{
			allwts[i] *= sqrt(casewts[i]);
		}
		ypred = fit[i];
		if (allwts[i] > 0.0 && ypred > 0.0)
		{
			allwts[i] *= sqrt(ypred);
		}
	} /*for (i = 0; i < ndata; i++)*/
} /*poissonvar()*/

/*
  poissondev(INITIAL) computes initial value of deviance
  poissondev(STEP) computes new value of deviance
*/
static double   poissondev(long op)
{
	long            i;
	double         *fit = *FitH;
	double          halfdev,ypred, yi;
	double         *allwts = *ALLWTS, *y = *Y;
	double         *offset = (USEGLMOFFSET) ? *GLMOFFSET : (double *) 0;
	long            ndata = (long) NDATA;

	halfdev = 0.;
	if (op == INITIAL)
	{
		for (i = 0; i < ndata; i++)
		{
			if (allwts[i] != 0.0)
			{
				ypred = (USEGLMOFFSET) ? exp(offset[i]) : 1.;
				if((yi = y[i]) > 0.0 ) 
				{
					halfdev += yi * log(yi/ypred) - yi + ypred;
				} 
				else 
				{
					halfdev += ypred;
				}
			} /*if (allwts[i] != 0.0)*/
		} /*for (i = 0; i < ndata; i++)*/
	} /*if (op == INITIAL)*/
	else
	{
		for (i = 0; i < ndata; i++)
		{
			if (allwts[i] != 0.0)
			{
				ypred = fit[i];
				if ((yi = y[i]) > 0.0)
				{
					halfdev += yi * log(yi / ypred) - yi + ypred;
				}
				else
				{
					halfdev += ypred;
				}
			} /*if (allwts[i] != 0.0)*/
		} /*for (i = 0; i < ndata; i++)*/
	} /*if (op == INITIAL){}else{}*/
	
	return (2.0 * halfdev);
} /*poissondev()*/

/*
  poissonlink(INITIAL) initializes fit[], newresid[], and lastresid[]
  poissonlink(STEP) updates fit[], newresid[], and lastresid[]
	poissonlink(op) always sets newresid[i] and lastresid[i] the same.
*/
static void     poissonlink(long op)
{
	long            i;
	long            ndata = (long) NDATA, nregx = (long) NREGX;
	long            nvals = ndata*nregx;
	double         *newresid = *NewresidH;
	double         *lastresid = *LastresidH;
	double         *fit = *FitH;
	double         *regx, *regx2;
	double         *y = *Y;
	double         *allwts = *ALLWTS;
	double         *misswts = (MODELTYPE & MISSWEIGHTS) ?
		*MISSWTS : (double *) 0;
	double         *offset = (USEGLMOFFSET) ? *GLMOFFSET : (double *) 0;
	double          logpred, ypred, resid;
	
	if (op == INITIAL)
	{
		/* Get starting values by OLS of Y on X-variables*/
		regx = *REGX;
		regx2 = *REGX2;
		doubleCopy(regx2, regx, nvals);

		for (i = 0; i < ndata; i++)
		{
			if (misswts != (double *) 0 && misswts[i] == 0.0)
			{
				fit[i] = lastresid[i] = newresid[i] = 0.0;
			}
			else
			{
				newresid[i] = y[i];
			}
		} /*for (i = 0; i < ndata; i++)*/

		gramschmidt(DONTNOTIFY | DOSWEEP | DONTDOSS | DONTDOCOEFS |
					DONTDORINV);
		checkInterrupt(interruptExit);
		for (i = 0; i < ndata; i++)
		{
			if (misswts == (double *) 0 || misswts[i] != 0.0)
			{
					/* predicted values */
				ypred = y[i] - newresid[i];
				if (ypred <= 0.5)
				{
					ypred = 0.5;
					/* modified eta */
				}
				fit[i] = ypred;
				resid = log(ypred) + (y[i] - ypred) / ypred;
				if(USEGLMOFFSET)
				{
					resid -= offset[i];
				}
				lastresid[i] = newresid[i] = resid;
			} /*if (misswts != (double *) 0 && misswts[i] == 0.0){}else{}*/
		} /*for (i = 0; i < ndata; i++)*/
	} /*if (op == INITIAL)*/
	else
	{ /*op == STEP*/
		for (i = 0; i < ndata; i++)
		{
			if (allwts[i] != 0.0)
			{
				/* new eta hat */
				logpred = lastresid[i] - newresid[i] / allwts[i];
				if(USEGLMOFFSET)
				{
					logpred += offset[i];
				}
				fit[i] = ypred = exp(logpred);
				/* modified eta */
				resid = logpred + (y[i] - ypred) / ypred;
				if(USEGLMOFFSET)
				{
					resid -= offset[i];
				}
				lastresid[i] = newresid[i] = resid;
			} /*if (allwats[i] != 0.0)*/
		} /*for (i = 0; i < ndata; i++)*/		
	} /*if (op == INITIAL){}else{}*/
	/* fall through*/
  interruptExit:
  	;
}  /*poissonlink()*/

/*
  poissonresid() puts the actual residual (*Y)[i] - fit[i] in newresid[i]
  and puts the previous value of newresid[i] in fit[i].  It is called just
  once, after the iterations
*/

static void     poissonresid(void)
{
	long            i;
	double         *fit = *FitH;
	double         *newresid = *NewresidH;
	double        **tmpH;
	double         *allwts = *ALLWTS, *y = *Y;
	long            ndata = (long) NDATA;

	for (i = 0; i < ndata; i++)
	{
		if (allwts[i] == 0)
		{
			setMissing(newresid[i]);
			setMissing(fit[i]);
		}
		else
		{
			/* modify predicted to residual in place */
			fit[i] = y[i] - fit[i];
		}
	}
	tmpH = FitH;
	FitH = NewresidH;
	NewresidH = tmpH;
} /*poissonresid()*/
/*
   End of Poisson distribution related code
*/

/*
   Public functions glmvar(), glmlink(), glmdev() and glmresid() for
   accessing xxxxvar(), xxxxlink(), xxxxdev(), and xxxxresid() where
   xxxx is robust, binom, or poisson.  Called from iterglm().
*/
void glmvar(long op)
{
	if (MODELTYPE & ROBUSTREG)
	{
		robustvar(op);
	}
	else if (GLMCONTROL & BINOMDIST)
	{
		binomvar(op);
	}
	else
	{ /*if (GLMCONTROL & POISSONDIST)*/
		poissonvar(op);
	}
} /*glmvar()*/

void glmlink(long op)
{
	if (MODELTYPE & ROBUSTREG)
	{
		robustlink(op);
	}
	else if(GLMCONTROL & BINOMDIST)
	{
		binomlink(op);
	}
	else
	{ /* if (GLMCONTROL & POISSONDIST)*/
		poissonlink(op);
	}
} /*glmlink()*/

double glmdev(long op)
{
	if (MODELTYPE & ROBUSTREG)
	{
		return (robustdev(op));
	}
	else if (GLMCONTROL & BINOMDIST)
	{ 
		return (binomdev(op));
	}
	else
	{ /* if (GLMCONTROL & POISSONDIST)*/
		return (poissondev(op));
	}
	
} /*glmdev()*/

/*
   Called just once, when iterations are over
*/

void glmresid(void)
{
	if (MODELTYPE & ROBUSTREG)
	{
		robustresid();
	}
	else if (GLMCONTROL & POISSONDIST)
	{
		poissonresid();
	}
	else if(GLMCONTROL & BINOMDIST)
	{
		binomresid();
	}
} /*glmresid()*/
