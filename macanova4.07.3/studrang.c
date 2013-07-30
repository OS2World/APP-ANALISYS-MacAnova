/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 4.00 or later
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
#pragma segment Studrang
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
#define lgamma mylgamma /*960220*/

#define INT         long

#define VMAX       1000.0    /* original 120 */

/*
   No thorough study has been made as to the appropriateness of the
   following constants.
*/
/*
   "tuning constants" for prtrng()
*/
#define DEFAULTEPS1 1e-7     /* original 1e-5 */
#define STEP1       0.225     /* original 0.45*/
#define JMIN1        10      /* original 3 */
#define JMAX1        75     /* original 15 */
#define KMIN1        15      /* original 7 */
#define KMAX1        75      /* original 15 */

/* "tuning constants" for qtrng() */
#define DEFAULTEPS2 1e-5     /* original 1e-3 */
#define JMAX2        20      /* original 8 */

#define SMALLISHP    .2
#define LARGISHP     .8

#define BADARGS      -3000
#define alnorm(X,I)  Cnor(-(X))

static double       Epsilon1;
static double       Epsilon2;

/*
  Algorithm AS 190  Appl. Statist. (1983) Vol.32, No. 2
  Incorporates corrections from Appl. Statist. (1985) Vol.34 (1)

  Evaluates the probability from 0 to q for a studentized
  range having v degrees of freedom and r samples.

  Uses Subroutine Alnorm = Algorithm AS66.

  Arrays vw and qw store transient values used in the
  quadrature summation.  Node spacing is controlled by
  step.  pcutj and pcutk control truncation.
  Minimum and maximum number of steps are controlled by
  jmin, jmax, kmin and kmax.  Accuracy can be increased
  by use of a finer grid - increase sizes of arrays vw
  and qw, and jmin, jmax, kmin, kmax and 1/step proportionally.

  960115 Translated to C by C. Bingham (kb@umnstat.stat.umn.edu)
  960117 Modified, use lgamma, changed tuning constants
*/
#define SQRTTWOPI    2.506628274631000502415765

double lgamma(double /*x*/);
static double  Lastv = -1.0, Lastc = 0.0;

static double prtrng(double q, double v, double r, INT *ifault)
{
	double         vw[2*JMAX1], qw[2*JMAX1];
	double         pcutj = 3*Epsilon1, pcutk = 10*Epsilon1;
	double         step = STEP1, vmax = VMAX;
	double         cvmax = 1.0/SQRTTWOPI;
	double         g, gmid, r1, c, h, v2, gstep, pk1, pk2, gk, pk;
	double         w0, pz, x, hj, ehj, pj;
	double         value = 0.0;
	INT            j, k, jj, jump;
	INT            jmin = JMIN1, jmax = JMAX1, kmin = KMIN1, kmax = KMAX1;
	int            nevals = 0;
	WHERE("prtrng");

	/*         check initial values */

	if (v < 1.0 || r < 2.0)
	{
		*ifault = BADARGS;
		goto errorExit;
	}
	*ifault = 0;
	
	if (q > 0.0)
	{
		/* computing constants, local midpoint, adjusting steps. */

		r1 = r - 1.0;

		if (v != Lastv)
		{ /* compute constant if DF has changed */
			if (v <= vmax)
			{
#if (1)
				v2 = v*0.5;
				/*log (C * exp(-v/2)/sqrt(2*PI)), C constant for dist of S */
				c = -log(SQRTTWOPI/2.0) - v2 + v2*log(v2) - lgamma(v2);
#else  /* original code*/
				double         cv1 = 0.193064705; /*     exp(-.5)/PI      */
				double         cv2 = 0.293525326; /* 2*exp(-1)/sqrt(2*PI) */
				double         cv[4];

				if (v == 1.0)
				{
					c = cv1;
				}
				else if (v == 2.0)
				{
					c = cv2;
				}
				else
				{
					cv[0] = 0.318309886; /*    1/PI    */
					cv[1] = -0.00268132716; /* -139/51840 */
					cv[2] = 0.00347222222; /*    1/288   */
					cv[3] = 0.0833333333; /*    1/12    */

					v2 = v*0.5;
					c = sqrt(v2)*cv[0]/(1.0 + ((cv[1]/v2 + cv[2])/v2 + cv[3])/v2);
				}
#endif		
			} /*if (v <= vmax)*/
			else
			{
				c = log(cvmax);
			}
			Lastv = v;
			Lastc = c;
		} /*if (v != Lastv)*/
		else
		{
			c = Lastc;
		}
		
		g = step*pow(r, -0.2);
		gmid = 0.5*log(r);

		if (v <= vmax)
		{
			h = step/sqrt(v);
			c += log(r*g*h);
		}
		else
		{
			c += log(r*g);
		}

/*
	Computing integral
	Given a row k, the procedure starts at the midpoint and works
	outward (index j) in calculating the probability at nodes
	symmetric about the midpoint.  The rows (index k) are also
	processed outwards symmetrically about the midpoint.  The
	centre row is unpaired.
*/
		gstep = g;
		qw[0] = qw[jmax] = -1.0;
		pk1 = pk2 = 1.0;
		for (k = 0;k < kmax ;k++)
		{
			gstep -= g;
			do
			{ /* loop twice, except when k == 0 */
				gstep = -gstep;
				gk = gmid + gstep;
				pk = 0.0;
				if (pk2 > pcutk || k < kmin)
				{
					w0 = c - gk*gk*0.5;
					pz = alnorm(gk, UPPERTAIL );
					x = alnorm(gk - q, UPPERTAIL ) - pz;
					if (x > 0.0)
					{
						pk = exp(w0 + r1*log(x));
					}
					if (v <= vmax)
					{
						jump = -jmax;
						do
						{  /* loop is executed exactly twice) */
							jump += jmax;
							for (j=0;j < jmax ;j++)
							{
								jj = j + jump;
								if (qw[jj] <= 0.0)
								{
									hj = h*(j + 1);
									if (j < jmax - 1)
									{
										qw[jj + 1] = -1.0;
									}
									ehj = exp(hj);
									qw[jj] = q*ehj;
									vw[jj] = v*(hj + 0.5 - ehj*ehj*0.5);
								} /*if (qw[jj] <= 0.0)*/

								pj = 0.0;
								x = alnorm(gk - qw[jj], UPPERTAIL ) - pz;
								if (x > 0.0)
								{
									pj = exp(w0 + vw[jj] + r1*log(x));
								}
								pk += pj;
								nevals++;
								if (pj <= pcutj && (jj >= jmin || k >= kmin))
								{
									break;
								}
							} /*for (j=0;j < jmax ;j++)*/
							h = -h;
						} while (h < 0.0);
					} /*if (v <= vmax)*/
				} /*if (pk2 > pcutk || k < kmin)*/

				value += pk;
				if (k >= kmin && pk <= pcutk && pk1 <= pcutk)
				{
					goto normalExit;
				}
				pk2 = pk1;
				pk1 = pk;
			} while (gstep > 0.0);
		} /*for (k = 0;k < kmax ;k++)*/
		*ifault = NONCONVERGED;
	} /*if (q > 0.0)*/

  errorExit:
  normalExit:
	return (value);
} /*prtrng()*/


/*
  Algorithm AS 190.2  Appl. Statist. (1983) Vol.32, No.2

  Calculates an initial quantile p for a studentized range
  distribution having v degrees of freedom and r samples
  for probability p, p.gt.0.80 .and. p.lt.0.995.

  Uses function ppnd - Algorithm AS 111

  960115 Translated to C by C. Bingham (kb@umnstat.stat.umn.edu)

*/
/* original qtrng0() */
static double qtrng0a(double p, double v, double r, INT *ifault)
{	
	double        q, t, vmax = VMAX;
	double        c1 = 0.8843, c2 = 0.2368, c3 = 1.214;
	double        c4 = 1.208, c5 = 1.4142;
	WHERE("qtrng0a");

	t = ppnd(0.5 + 0.5*p, ifault);
	if (v < vmax)
	{
		t += (t*t*t + t)/v/4.0;
	}
	q = c1 - c2*t;
	if (v < vmax)
	{
		q -= c3/v - c4*t/v;
	}
	return (t*(q*log(r - 1.0) + c5));
} /*qtrng0a()*/

#define NCUTS  10 /* number of bisections for initial value */

/*
   Modified routine for getting starting value for iteration on inverse
   If p > LARGISHP it returns qtrng0a(p,v,r,ifault);  otherwise it does
   a form of bisection with secant method on log-log scale on last
   step
*/

static double qtrng0(double p, double v, double r, INT *ifault)
{	
	double        trialq, lasttrialq;
	double        testp, lasttestp;
	double        logp, logp1, logp2, logq1, logq2;
	INT           icut;
	WHERE("qtrng0");

	
	testp = (p >= LARGISHP) ? p : .999;
	trialq = qtrng0a(testp, v, r, ifault);

	if (p < LARGISHP)
	{
	/*
	   Strategy is to bisect until below max(target,SMALLISHP); then use secant
	   in log/log scale on last two points to estimate probability point
	 */
		icut = 0;
		while (1)
		{
			testp = prtrng(trialq, v, r, ifault);
			if (icut++ == 0 || testp > p && testp > SMALLISHP && icut < NCUTS)
			{
				lasttestp = testp;
				lasttrialq = trialq;
				trialq *= 0.5;
			}
			else
			{
				logp1 = log(lasttestp);
				logp2 = log(testp);
				logq1 = log(lasttrialq);
				logq2 = log(trialq);
				logp = log(p);
				trialq = exp(((logp2 - logp)*logq1 + (logp - logp1)*logq2)/
							 (logp2 - logp1));
				break;
			} 
		} /*while (1)*/
	} /*if (p < LARGISHP)*/
	
	return (trialq);
} /*qtrng0()*/

/*
  Algorithm AS 190.1  Appl. Statist. (1983) Vol.32, No. 2

  Approximates the quantile p for a studentized range
  distribution having v degrees of freedom and r samples
  for probability p, p.ge.0.90 .and. p.le.0.99.

  Uses functions  alnorm, ppnd, prtrng and qtrng0 -
  Algorithms AS 66, AS 111, AS 190 and AS 190.2

  960115 Translated to C by C. Bingham (kb@umnstat.stat.umn.edu)
  960117 Made modifications.
   Removed check for p being between .9 and .99
   Changed tuning constants
   Corrected apparent error in computing first stab at p2
*/

static double qtrng(double p, double v, double r, INT *ifault)
{
	double       pcut = Epsilon2; /*original was 0.001 */
	double       d, q1, p1, q2, p2, e1, e2;
	double       logp, logq1, logq2, logp1, logp2;
	double       eps = Epsilon2/100.0; /* was 1e-4*/
	double       unsetDouble = HUGEDBL;
	double       value = 0, logvalue = unsetDouble;
	INT          j, jmax = JMAX2, nfault = 0;
	INT          prtrngError = 0;
	WHERE("qtrng");
	
  /* check input parameters */

	*ifault = 0;

	if (v < 1.0 || r < 2.0)
	{ /* should not happen in MacAnova */
		*ifault = BADARGS;
	}
#if (0) /*kb*/
	else if (p < 0.90 || p > 0.99)
#else
	else if (p <= 0.0 || p >= 1.0)
#endif
	{ /* should not happen in MacAnova */
		*ifault = BADARGS;
	}
	if (*ifault != 0)
	{
		goto errorExit;
	}
	
	/* obtain initial values */

	value = qtrng0(p, v, r, &nfault);
	q1 = value;
	p1 = prtrng(q1, v, r, &nfault);
	if (fabs(p1/p - 1) >= pcut)
	{
		if (p1 > p)
		{
			/* kb changed following from p1 = 1.75 ... */
#if (1)
			p2 = p + .75*(p - p1)*p/p1;
#else
			/* original; destination wrong; also can be negative */
			p1 = 1.75*p - 0.75*p1; /*p + .75*(p-p1)*/
#endif
		}
		else
		{
			p2 = p + (p - p1)*(1.0 - p)/(1.0 - p1)*0.75;
		}
#if (0) /*kb eliminate check for limits*/
		if (p2 < 0.80)
		{
			p2 = 0.80;
		}
		else if (p2 > 0.995)
		{
			p2 = 0.995;
		}
#endif
		q2 = qtrng0(p2, v, r, &nfault);
		if (p <= SMALLISHP)
		{
			logp = log(p);
			logp1 = log(p1);
			logq1 = log(q1);
			logq2 = log(q2);
		}

		/* refine approximation */
		for (j=1; j < jmax ;j++)
		{
			p2 = prtrng(q2, v, r, &nfault);

			if (nfault)
			{
				prtrngError++;
			}
			
#if (0)
			if (nfault != 0)
			{
				goto errorExit;
			}
#endif
			/* compute secant value unless p1-p2 too small */
			if (p > SMALLISHP)
			{

				e1 = p1 - p;
				e2 = p2 - p;
				d = e2 - e1;
				value = (fabs(d) > eps) ? (e2*q1 - e1*q2)/d : (q1 + q2)/2.0;
				if (fabs(p2 / p - 1.0) < pcut*5.0)
				{
					goto normalExit;
				}
				if (fabs(e1) >= fabs(e2))
				{
					q1 = q2;
					p1 = p2;
				}
				q2 = value;
			} /*if (p > SMALLISHP)*/
			else
			{
				/* use log scales for small p */
				logp2 = log(p2);
				e1 = logp1 - logp;
				e2 = logp2 - logp;
				d = e2 - e1;

				logvalue = (fabs(d) > eps) ?
					(e2*logq1 - e1*logq2)/d : (logq1 + logq2)/2.0;
				if (fabs(p2 / p - 1.0) < pcut*5.0)
				{
					goto normalExit;
				}
				if (fabs(e1) >= fabs(e2))
				{
					logq1 = logq2;
					logp1 = logp2;
				}
				logq2 = logvalue;
				q2 = exp(logq2);
			} /*if (p > SMALLISHP){}else{}*/
		} /*for (j=1; j < jmax ;j++)*/
		nfault = NONCONVERGED;
	} /*if (fabs(p1/p - 1) >= pcut)*/

  normalExit:
	*ifault = nfault;
	return ((logvalue == unsetDouble) ? value : exp(logvalue));

	
  errorExit:
	return ((logvalue == unsetDouble) ? value : exp(logvalue));
} /*qtrng()*/

double Pstudrange(double x, double ngroup, double edf, double eps)
{
	double          q;
	INT             error;
	WHERE("Pstudrange");
	
	Epsilon1 = (eps <= 0.0 || eps > 0.1) ? DEFAULTEPS1 : eps;	
	
	q = prtrng(x, edf, ngroup, &error);

	return (q + (double) error);
} /*Pstudrange()*/

double Qstudrange(double prob, double ngroup, double edf, double eps)
{
	double          x;
	INT             error = 0;
	WHERE("Qstudrange");
	
	Epsilon2 = (eps <= 0.0 || eps > 0.1) ? DEFAULTEPS2 : eps;
	Epsilon1 = Epsilon2/100.0;

	x = qtrng(prob, edf, ngroup, &error);

	return (x + (double) error);
} /*Qstudrange()*/

