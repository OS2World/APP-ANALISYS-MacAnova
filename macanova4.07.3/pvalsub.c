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
#pragma segment Pvalsub
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

#define lgamma mylgamma /*960220*/

#define USEBETAI   /* use cumulative beta to compute cumulative binomial */
#define USEGAMMABASE   /* use cumulative gamma to compute cumulative Poisson */

/*
  This C function returns the value  P( X <= x )
  where X is Beta with parameters  a & b
*/

double          Cbet(double x, double a, double b)
{
	return ((x <= 0.0) ? 0 :(( x >= 1 ) ? 1.0 : betai(x,a,b)));
}

double          Qbeta(double p, double a, double b)
{
	long            ifault;

	return (ppbeta(p, a, b, &ifault));
}


/*
  This C function returns the value  P( X <= x )
  where X is Binomial with parameters n & P
  Calling program should check parameters, not done here
*/

#ifndef USEBETAI
double          Cbin(long ix, long n, double P)
{
	long            i, ix0;
	double          prob = 1.0, sum = 0.0, px0;
	double          x = (double) ix, x0;
	double          di, dipl1, dnmni, dnmnipl1, dn = (double) n;
	
	ix0 = (long) (x0 = ceil(dn * P));
	prob = exp(lgamma(dn + 1.0) + x0*log(P) + (dn - x0)*log(1.0 - P) -
			   lgamma(x0 + 1.0) - lgamma(dn - x0 + 1.0));
	px0 = prob;

	dipl1 = (double) ix0;
	dnmni = dn - dipl1 + 1.0;
	if (ix < ix0)
	{
		for (i = ix0 - 1; i >= 0; i--)
		{
			prob *= (dipl1-- / P) * (1.0 - P) / dnmni++;
			if(i <= ix)
			{
				sum += prob;
			}
		}
	} /*if (ix < ix0)*/
	else
	{
		sum = px0;
		for (i = ix0 - 1; i >= 0; i--)
		{
			prob *= (dipl1-- / P) * (1.0 - P) / dnmni++;
			sum += prob;
		}
		prob = px0;
		di = (double) (ix0 + 1);
		dnmnipl1 = dn - di + 1.0;
		for (i = x0 + 1; i <= ix; i++)
		{
			prob *= ((P / di++) / (1.0 - P)) * dnmnipl1--;
			sum += prob;
		}
	} /*if (ix < ix0){}else{}*/

	return (sum);
} /*Cbin()*/
#else /*USEBETAI*/

double          Cbin(long ix, long n, double p)
{
	double          prob;
	
	if(ix < 0)
	{
		prob = 0.0;
	}
	else if(ix >= n)
	{
		prob = 1.0;
	}
	else
	{
		prob = betai(1.0 - p, (double) (n - ix), (double) (ix + 1));
	}
	
	return (prob);
} /*Cbin()*/

#endif /*USEBETAI*/

/*
  This C function returns the value  P( X <= x )
  where X is chi-square with f degrees of freedom

  NOTE: This function calls gammabase
        which can be found in gammabas.c
  Changed to accept non-integral d.f. by kb
*/

double          Cchi(double x, double f)
{
	double          cdf;

	if (f <= 0)
	{
		putOutMsg("The chi-square distribution must have positive df");
		return (-1);
	}

	if (x <= 0.0)
	{
		cdf = 0.0;
	}
	else
	{
		f *= .5;
		x *= .5;
		gammabase(&x, &f, &cdf);
	}
	
	return (cdf);
} /*Cchi()*/


double          Qchi(double p, double f)
{
	long            ifault;

	return (2. * ppgamma(p, f / 2., &ifault));
} /*Qchi()*/

/*
  This C function returns the value  P( X <= x )
  where X is Gamma with parameters a & b.
  The likelihood of the gamma is
                (a-1)    (-b*x)
      L(a,b) = x     *exp
*/

double          Cgam(double x, double a, double b)
{
	double          y, cdf;

	if (a <= 0.0 || b <= 0.0)
	{
		putOutMsg("Both parameters for the gamma distribution must be positive");
		return (-1);
	}

	y = x / b;
	gammabase(&y, &a, &cdf);

	return (cdf);

} /*Cgam()*/

double          Qgam(double p, double a, double b)
{
	long            ifault;

	return (b * ppgamma(p, a, &ifault));
} /*Qgam()*/

/*
   Adapted from
	Algorithm AS 275 Appl.Statist. (1992), Vol.41, No.2,
	by Cherng G. Ding

	Computes the noncentral chi-square distribution function
	with positive real degrees of freedom f and nonnegative
	noncentrality parameter theta

	Translation to C by C. Bingham, kb@stat.umn.edu October 1996
	Instead of a 4th argument ifault, chi2nc now returns
	-value, -2 and -3 where ifault would have been 1, 2 or 3
	Thus, provided arguments are correct, chi2nc(x,f,theta) < 0 indicates
	failure to converge, with -chi2nc(x,f,theta) then the best value.
*/

#ifndef CHIERRORMAX
#define CHIERRORMAX  1e-10
#endif /*CHIERRORMAX*/

#ifndef CHIMAXIT
#define CHIMAXIT    500
#endif /*CHIMAXIT*/

extern double      lgamma(double);

#if (0) /* used for tuning */
static setstuff(int * itmax, double * errmax)
{
	Symbolhandle     symh;
	
	if (GUBED & 8)
	{
		if ((symh = Lookup("CHIMAXIT")) != (Symbolhandle) 0 &&
			isInteger(symh, POSITIVEVALUE))
		{
			*itmax = (int) DATAVALUE(symh,0);
		}
		if ((symh = Lookup("CHIERRORMAX")) != (Symbolhandle) 0 &&
			isScalar(symh) && TYPE(symh) == REAL &&
			DATAVALUE(symh,0) > 0.0)
		{
			*errmax = DATAVALUE(symh,0);
		}
	}
} /*setstuff()*/
#endif /*0*/
#if (0)
/*
  code to see how save number of iterations in variable NITER so as
  to get data to determin how the number of iterations depends on arguments
  Approximately n = 4.3 + 4.7*sqrt(x) + .5*x - .5*f

  No checking is done
*/
void saveNiter(double  n);
{
	Symbolhandle      symh = Lookup("NITER");
	long              length;

	if (symh == (Symbolhandle) 0)
	{
		length = 1;
		symh = RInstall("NITER", length);
	}
	else
	{
		length = symbolSize(symh) + 1;
		setDATA(symh,
				(double **) mygrowhandle((char **) DATA(symh),
										 length * sizeof(double)));
		setDIM(symh, 1, length);
	}
	DATAVALUE(symh, length-1) = n;
} /*saveNiter()*/

#endif /*0*/
static double chi2nc(double x, double f, double theta)
{
	int               flag = 0;
	int               itrmax = CHIMAXIT;
	int               done = 0;
	double            errmax = CHIERRORMAX;
	double            lam, n, u, v, x2, f2, t, term, bound;
	double            value = x;
	WHERE("chi2nc");
	
#if (0)
	setstuff(&itrmax,&errmax);
#endif /*0*/

	if (f <= 0 || theta < 0.0)
	{
		value = -2.0;
	}
	else if (x < 0.0)
	{
		value = -3.0;
	}
	else if (x > 0.0)
	{
		lam = theta/2.0;

		/*        evaluate the first term */

		n = 1.0;
		u = exp(-lam);
		v = u;
		x2 = x/2.0;
		f2 = f/2.0;
#if (0) /* following caused overflow problems */
		t = pow(x2, f2)*exp(-x2)/exp(lgamma((f2+1.0)));
#else
		t = exp(f2*log(x2) - x2 - lgamma(f2+1.0));
#endif /*0*/
		value = term = v*t;


		while (!done)
		{
		/*        check if (f+2n) is greater than x */
			if (f + 2.0*n - x > 0.0)
			{
				flag =  -1;
			}

			while (!done)
			{
/*
   Evaluate the next term of the expansion and then the partial sum
*/
				if (flag >= 0)
				{
					if (lam != 0.0)
					{
						u *= lam/n;
						v += u;
					}					
					t *= x/(f + 2.0*n);
					term = v*t;
					value += term;
					n++;
					if (!flag)
					{
						break;
					}
				}
				else
				{
					flag = 1;
				}
				
				/* find the error bound and check for convergence */
				bound = t*x/(f + 2.0*n - x);

				if (bound <= errmax || (int) n > itrmax)
				{
					done = 1;
				}
			} /*while (!done)*/
		} /*while (!done)*/

#if (0)
		saveNiter(n);
#endif /*0*/

		if (bound > errmax)
		{
			value = -value;
		}
	}
	return (value);
} /*chi2nc()*/

double          noncenCchi(double x, double f, double lam)
{
	return (chi2nc(x, f, lam));
} /*noncenCchi()*/

static double   ncCchi(double x, double param [], long nparam, long * ifault)
{
	double         p;

	*ifault = 0;
	p = (nparam == 2) ? chi2nc(x, param[0], param[1]) : Cchi(x, param[0]);

	return (fabs(p));
} /*ncCchi()*/

#define Nxy    report[0]
#define Iter   report[1]
#define Ifault report[2]

#define ITMAXQCHI  20
#define NTABQCHI   10

double          noncenQchi(double p, double f, double lam, double epsilon)
{
	double      eps = (epsilon <= 0) ? 1e-10 : epsilon;
	double      x[NTABQCHI], y[NTABQCHI], wrk[NTABQCHI];
	double      xval, param[2];
	double      mu = f + lam, sigsq = 2.0*(mu + lam);
	double      fstar = 2.0*mu*mu/sigsq;
	double      start = mu*Qchi(p,fstar)/fstar;
	long        report[3];
	long        ierror = 0;
	long        itmax = ITMAXQCHI, ntab = NTABQCHI;
	WHERE("noncenQchi");
	
	param[0] = f;
	param[1] = lam;
	x[0] = .95*start;
	x[1] = 1.05*start;
#if (0)
	PRINT("start = %g, x[0] = %g, x[1] = %g\n",start,x[0],x[1],0);
#endif /*0*/
	xval = fsolve(p, ncCchi, param, (lam != 0.0) ? 2 : 1, 1e10, 1e-30,
			   eps, itmax, ntab, x, y, wrk, report);
	if (Ifault == 1)
	{
		xval = -xval;
	}
	else if (Ifault)
	{
#if (0)
		PRINT("Ifault = %d\n",Ifault,0,0,0);
#endif /*0*/
		setMissing(xval);
	}
	return (xval);
} /*noncenQchi()*/

/*
  This C function returns the value  P( X <= x )
  where X is Hypergeometric; i.e, X counts the
  number of marked balls drawn from an urn containing
  M balls (N of which are marked) when the size
  of the draw is K.
*/
#if (0) /* not currently used */
double          Chyp(long x, long M, long N, long K)
{
	long            i, x0;
	double          prob = 1.0, sum = 0.0, px0;

	if (M < 0 || N < 0 || K < 0)
	{
		putOutMsg("All hypergeometric parameters must be non-negative");
		return (-1);
	}
	if (M < N)
	{
		putOutMsg("In the hypergeometric, the number of marked balls");
		putOutMsg("  may not exceed the total number of balls");
		return (-1);
	}
	if (M < K)
	{
		putOutMsg("In the hypergeometric, the number of drawn balls");
		putOutMsg("  may not exceed the total number of balls");
		return (-1);
	}

	x0 = K * N / M + 1;
	for (i = 0; i < x0; i++)
	{
		prob *= (N - i) / (M - i) * (K - i) / (x0 - i);
	}
	for (i = 0; i < K - x0; i++)
	{
		prob *= (M - N - i) / (M - x0 - i);
	}
	px0 = prob;

	if (x < x0)
	{
		for (i = x0 - 1; i > x; i--)
		{
			prob *= (i + 1.0) / (N - i) * (M - N - K + i + 1.0) / (K - i);
		}
		for (i = x; i >= 0; i--)
		{
			prob *= (i + 1.0) / (N - i) * (M - N - K + i + 1.0) / (K - i);
			sum += prob;
		}
	}
	else
	{
		sum = px0;
		for (i = x0 - 1; i >= 0; i--)
		{
			prob *= (i + 1.0) / (N - i) * (M - N - K + i + 1.0) / (K - i);
			sum += prob;
		}
		prob = px0;
		for (i = x0 + 1; i <= x; i++)
		{
			prob *= (N - i + 1.0) / i * (K - i + 1.0) / (M - N - K + i);
			sum += prob;
		}
	}

	return (sum);
} /*Chyp()*/
#endif /*0*/
/*
  This C function returns the value  P( X <= x )
  where X is a standard Normal random variable
*/


double          Cnor(double x)
{
	double          cdf;

	normbase(&x, &cdf);

	return (cdf);
} /*Cnor()*/

double          Qnor(double p)
{
	long            ifault;

	return (ppnd(p, &ifault));
} /*Qnor()*/


/*
  This C function returns the value  P( X <= x )
  where X is Poisson with parameter l.
*/

#ifndef USEGAMMABASE
double          Cpoi(long x, double l)
{
	long            i, x0;
	double          k, px0, prob = 1.0, sum = 0.0;

	if (l > 225)
	{
		putOutMsg("The poisson parameter may not exceed 225");
		return (-1);
	}

	if(l <= 0.)
	{
		return ((x >= 0) ? 1.0 : 0.0);
	}
	
	x0 = ceil(l);
	k = exp(-l / x0);
	for (i = 0; i < x0; i++)
	{
		prob *= k / (x0 - i) * l;
	}
	px0 = prob;

	if (x < x0)
	{
		for (i = x0 - 1; i > x; i--)
		{
			prob *= (i + 1) / l;
		}
		for (i = x; i >= 0; i--)
		{
			prob *= (i + 1) / l;
			sum += prob;
		}
	} /*if (x < x0)*/
	else
	{
		sum = px0;
		for (i = x0 - 1; i >= 0; i--)
		{
			prob *= (i + 1) / l;
			sum += prob;
		}
		prob = px0;
		for (i = x0 + 1; i <= x; i++)
		{
			prob *= l / i;
			sum += prob;
		}
	} /*if (x < x0){}else{}*/
	return (sum);

} /*Cpoi()*/
#else /*USEGAMMABASE*/
double          Cpoi(long x, double l)
{
	double      prob;
	
	if(x < 0)
	{
		prob = 1.0;
	}
	else if(l <= 0)
	{
		prob = 0.0;
	}
	else
	{
		double	y = l;
		double 	alpha = x + 1;
		gammabase(&y, &alpha, &prob);
	}
	return (1.0 - prob);
} /*Cpoi()*/

#endif /*USEGAMMABASE*/

/*
  This C function returns the value  P( X <= x )
  where X is Snedecor's F with n1 d.f. in the
  numerator and n2 d.f in the denominator.

  NOTE: This function calls Cbet() above which in turn calls
  betabase in betabase.c
*/

double          Csne(double x, double n1, double n2)
{
	if (n1 <= 0.0 || n2 <= 0.0)
	{
		putOutMsg("Both df's in the F distribution must be positive");
		return (-1);
	}

	return (1.0 - Cbet(n2/(n2 + n1 * x), n2/2.0, n1/2.0));
} /*Csne()*/

double          Qsne(double p, double n1, double n2)
{
	long            ifault;

	return ((n2 / ppbeta(1.0 - p, n2/2.0, n1/2.0, &ifault) - n2) / n1);
} /*Qsne()*/


/*
  This C function returns the value  P( X <= x )
  where X is noncentral F with n1 d.f. in the
  numerator and n2 d.f in the denominator and noncentrality
  parameter lam = n1*(E[MSnum]/E[MSdenom] - 1)


  NOTE: This function calls betanc() in betabase.c
  If the summation in betanc fails to converge, it returns betanc()
  + the number of terms.
*/

double          noncentF(double x, double lam, double n1, double n2)
{
	double          cdf;
	long            niter = 0;
	WHERE("noncentF");
	
	if(x <= 0)
	{
		cdf = 0.0;
	}
	else
	{
		x = n1*x/(n2 + n1*x);
		cdf = betanc(x,n1/2, n2/2, lam, &niter);
		if(niter)
		{
			if(cdf >= 1.0)
			{
				cdf = 1-1e-10;
			}
			cdf += (double) niter;
		}
	}
	
	return (cdf);
} /*noncentF()*/
	
double          noncentBeta(double x, double lam, double a, double b)
{
	double          cdf;
	long            niter = 0;
	WHERE("noncentB");
	
	cdf = (x <= 0.0) ? 0.0 : ((x >= 1.0) ? 1.0 : betanc(x,a, b, lam, &niter));
	if(niter)
	{
		if(cdf >= 1.0)
		{
			cdf = 1-1e-10;
		}
		cdf += (double) niter;
	}
	return (cdf);
} /*noncentBeta()*/
	
/*
  This C function returns the value  P( X <= x )
  where X is a Student's t with n degrees of freedom

  NOTE: This function calls studentbase which can be found in studentb.c
*/

double          Cstu(double x, double n)
{
	double          cdf;

	if(n == floor(n))
	{
		studentbase(&x, &n, &cdf);
	} /*if(n == floor(n))*/
	else
	{
		double          a, b, y;

		a = b = .5*n;
		y = .5*(1.0 + sqrt(1.0 - n/(n + x*x)));

		cdf = betai(y, a, b);

		if(x < 0)
		{
			cdf = 1.0 - cdf;
		}
	} /*if(n == floor(n)){}else{}*/		
	return (cdf);

} /*Cstu()*/

double          Qstu(double p, double n)
{
	long            ifault;
	double          t;

	if(n == floor(n))
	{
		t = ppstudent(p, n, &ifault);
	}
	else
	{
		t = ppbeta(p, n/2.0, n/2.0, &ifault);
		t = sqrt(.25*n/(t*(1.0-t)) - n);
		if(p <= 0.5)
		{
			t = -t;
		}
	}
	return (t);
} /*Qstu()*/


/*
   Algorithm AS 243  Appl. Statist. (1989), Vol.38, No. 1

   Cumulative probability at t of the non-central t-distribution
   with df degrees of freedom (may be fractional) and non-centrality
   parameter delta.

   Note - requires the following auxiliary routines
   alogam (x)                         - ACM 291 or AS 245
   betain (x, a, b, albeta, ifault)   - AS 63 (updated in ASR 19)
   alnorm (x, upper)                  - AS 66

   961015 Translated to C by C. Bingham, kb@stat.umn.edu
   Formal parameter ifault eliminated.  When ifault = 2, value -2 returned
   When ifault = 1 (non convergence), -value returned, where value is
   value computed so far.
*/

#define R2PI   0.79788456080286535588 /* 1/(gamma(1.5)*sqrt(2)) = sqrt(2/pi)*/
#define ALNRPI 0.57236494292470008707  /*ln(sqrt(pi)*/

#define TNCMAXIT   100.1
#define TNCERRMAX  1.0e-10
/* Note - TNCMAXIT and TNCERRMAX may be changed to suit one's needs. */

extern double    lgamma(double);
extern double    betai(double, double, double);
extern double    Cnor(double);

#define betain(X,A,B,C,I) betai(X,A,B)
#define alnorm(X,UPPER)   Cnor(-(X))
#define alogam(X,I)       lgamma(X)
#define tnc               noncenCstu

double tnc(double t, double df, double delta)
{
	double           a, albeta, b, del, en;
	double           errbd = 1.0,  geven, godd, lambda;
	double           p, q, rxb, s, tt, x, xeven, xodd;
	double           r2pi = R2PI, alnrpi = ALNRPI;
	double           itrmax = TNCMAXIT, errmax = TNCERRMAX;
	double           value;
	int              negdel;

	if (df <= 0.0)
	{
		value = -2.0;
	} /*if (df <= 0.0)*/
	else		
	{
		negdel =  (t < 0.0);
		tt = (negdel) ? -t : t;
		del = (negdel) ? -delta : delta;

/*
   Initialize twin series (Guenther, J. Statist. Computn. Simuln.
   Vol.6, 199, 1978).
*/
		x = t*t/(t*t + df);
		if (x == 0.0)
		{
			value = 0.0;
		} /*if (x == 0.0)*/
		else
		{
			lambda = del*del;
			p = 0.5*exp(-0.5*lambda);
			q = r2pi*p*del;
			s = 0.5 - p;
			a = 0.5;
			b = 0.5*df;
			rxb = pow(1.0 - x, b);
			albeta = alnrpi + alogam(b, ifault) - alogam(a + b, ifault);
			xodd = betain(x, a, b, albeta, ifault);
			godd = 2.0*rxb*exp(a*log(x) - albeta);
			xeven = 1.0 - rxb;
			geven = b*x*rxb;
			value = p*xodd + q*xeven;

		/*      repeat until convergence */
			for (en = 1.0; errbd > errmax && en <= itrmax; en++)
			{
				a++;
				xodd -= godd;
				xeven -= geven;
				godd *= x*(a + b - 1.0)/a;
				geven *= x*(a + b - 0.5)/(a + 0.5);
				p *= lambda/(2.0*en);
				q *= lambda/(2.0*en + 1.0);
				s -= p;
				value += p*xodd + q*xeven;
				errbd = 2.0*s*(xodd - godd);
			} /*for (en = 1.0; errbd > errmax && en <= itrmax; en++)*/
		} /*if (x == 0.0){}else{}*/

		value += alnorm(del, 1 );
		if (negdel)
		{
			value = 1.0 - value;
		}
		if (en > itrmax)
		{
			value = -value;
		}
	} /*if (df <= 0.0){}else{}*/
	return (value);
} /*tnc()*/

