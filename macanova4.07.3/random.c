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
#pragma segment Random
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

/*
  971109 added implementation for rpoi()
  980310 added implementation for rbinom()
  980618 somewhat reorganized ranpois() and put in special treatment of
         E[X] < 10
  990212 changed most uses of putOUTSTR() to putErrorOUTSTR()
*/
#include "globals.h"

#ifdef MACINTOSH
#ifdef MPW
#include <OSUtils.h>
#endif /*MPW*/
#else /*MACINTOSH*/
#ifndef NOTIMEH
#include	<time.h>
#endif /*NOTIMEH*/
#endif /*MACINTOSH*/

#ifdef EPX
typedef long time_t;
#endif /*EPX*/

/*
static long            Rands1 = 0;
static long            Rands2 = 0;
*/

/*
  These are for the combined generator suggested for 32 bit machines
   in P. L'Ecuyer 1988 Comm. ACM
*/
#define Rands1 RANDS1  /* global long */
#define Rands2 RANDS2  /* global long */

#define RANDM1 2147483563L
#define RANDA1 40014
#define RANDQ1 53668
#define RANDR1 12211

#define RANDM2 2147483399L
#define RANDA2 40692
#define RANDQ2 52774
#define RANDR2 3791


/*
   Get single long random number
*/
long       iuni(void)
{
	long            z, k;

	k = Rands1 / RANDQ1;
	Rands1 = RANDA1 * (Rands1 - k * RANDQ1) - k * RANDR1;
	if (Rands1 < 0)
	{
		Rands1 += RANDM1;
	}
	
	k = Rands2 / RANDQ2;
	Rands2 = RANDA2 * (Rands2 - k * RANDQ2) - k * RANDR2;
	if (Rands2 < 0)
	{
		Rands2 += RANDM2;
	}
	
	z = Rands1 - Rands2;
	if (z < 1)
	{
		z += RANDM1 - 1;
	}
	

	return (z);

} /*iuni()*/

/* Get single double random number*/
double duni(void)
{
	return (iuni()/ (double) RANDM1);
} /*duni()*/

/* 
	From Kemp, C.D., Kemp, Adrienne W., Poisson random variate generation
 	Applied Statistics 40 (1991) 143-158

 	This subroutine is a variable parameter poisson generator
 	which uses highly accurate approximations to the modal and
 	cumulative modal probablities, due to A. W. Kemp (simple
 	Algorithms for the Poisson modal cumulative probability,
 	Communs Statist.Simuln, 17, 1495-1508 (1988)), to conduct
 	a fast modal search with squeezing.

 	l1 and l2 are used to avoid unnecessary initialization if the
 	parameter is unchanged from the previous call.

 	input:
 		l: poisson parameter
 	returns
 		generated poisson variate
 	input and output
 		dseed:  seed for the imsl uniform generagor ggubfs

    As revised 980618, algorithm is used only for l >= 10.0;  otherwise
    a straightforward upward inversion of the CDF is used.  This follows the
	recommendation in the article.
*/
#define   INT   long
#define RUNI() duni()

static INT ranpois(double l)
{
	static double  d = 1.0/2.0, e = 2.0/3.0;
	static double  c1 = 1.0/12.0, c2 = 1.0/24.0, c3 = 293.0/8640.0;
	static double  d1 = 23.0/270.0, d2 = 5.0/56.0, d3 = 30557.0/649152.0;
	static double  d4 = 34533608.0/317640015.0;
	static double  e1 = 1.0/4.0, e2 = 1.0/18.0;
	static double  f1 = 1.0/6.0, f2 = 3.0/20.0, f3 = 1.0/20.0;
	static double  h = 7.0/6.0, c = 0.39894228040143267793995;
	static double  l1 = 0.0, l2 = 0.0;
	static double  rm, a, aa, g, p, q, sp, sq;
	static INT     m, mmax;
	INT            i, mmin;
	double         qq;
	double         u;
	int            down;
	
	if (l == 0.0)
	{
		return (0);
	}
	
	while (1)
	{
		u = RUNI();
		/* 	test for unchanged parameter */
		if (l != l2)
		{
			if (l >= 10.0)
			{
				if (l != l1)
				{
					l1 = l;
					/* 	compute modal probablity */
					m = (INT) (l + d);
					mmax = 2*m + 30;
					rm = 1.0/(double) m;
					a = l - m;
					g = c/sqrt((double) m);
					aa = a*a;
					p = g*(1.0 - c1/(m + c2 + c3*rm));
					q = p*(1.0 - d*aa/(m + a*(e + a*(e1 - e2*rm))));
				} /*if (l != l1)*/
				/*5*/
				/* squeeze */
				if (u < d)
				{
					down = 1;
				}
				else if (u <= d + h*g)
				{
					l2 = l;
					/* 	compute cumulative modal probablity */
					sp = d + g*(e - d1/(m + d2 + d3/(m + d4)));
					sq = sp - a*p*(1.0 - f1*aa/(m + a*(d + a*(f2 - f3*rm))));
					down = (u <= sq);
				}
			} /*if (l >= 10.0)*/ 
			else
			{
				l1 = l2 = l;
				m = 0;
				mmax = 50;
				q = sq = exp(-l);
				down = (u <= sq);
			} /*if (l >= 10.0){}else{}*/ 
		} /*if (l != l2)*/
		/*6*/
		else /* search up or down ?*/
		{
			down = (u <= sq);
		}

		if (down)
		{
			/* downward search */
			/*165*/
			if (u <= q)
			{
				return(m);
			}
			
			qq = q;
			mmin = m - 1;
			for (i = 0;i < mmin ;i++)
			{
				u -= qq;
				qq *= (m - i)/l;
				if (u <= qq)
				{
					return (mmin - i);
				}
			} /*for (i = 0;i < mmin ;i++)*/
		} /*if (down)*/
		else
		{
			/* upward search*/
			/*200*/
			u = 1.0 - u;
			qq = q;
			for (i = m + 1;i <= mmax ;i++)
			{
				qq *= l/(double) i;
				if (u < qq)
				{
					return i;
				}
				u -= qq;
			} /*for(i = m + 1;i <= mmax ;i++)*/
		} /*if (down){}else{}*/
		/*
		  should only get here in unlikely event don't finish upward
		  search, in which case we start again
		*/
	} /*while (1)*/
} /*ranpois()*/

/*
   Algorithm 678,  Collected Algorithms from ACM.
   This work published in Transactions on Mathematical Software, 
   Vol. 15,  No. 4,  pp. 394 - 397.

  Binomial random variate generator
  mean .lt. 30 -- inverse cdf
  mean .ge. 30 -- algorithm btpe:  acceptance - rejection via
    four region composition.  the four regions are a triangle
    (symmetric in the center),  a pair of parallelograms (above
    the triangle),  and exponential left and right tails.

  btpe refers to binomial - triangle - parallelogram - exponential.
  btpec refers to btpe and "combined."  thus btpe is the
    research contribution and btpec is the implementation of a
    complete usable algorithm.
  Reference:  Voratas Kachitvichyanukul and Bruce Schmeiser,
    "Binomial random variate generation, "
    Communications of the ACM,  Volume 31,  number 2,  216 - 222,  1988

  Written:
           btpe :  September 1980.
           btpec : May 1985.
           Comments last modified June 1988

  Required subprogram:  rand() -- a uniform (0, 1) random number
                        generator
  Arguments

    n : number of Bernoulli trials            (input)
    pp : probability of success in each trial (input)
    iseed:  random number seed                (input and output)
    jx:  randomly generated observation       (output)

  Variables
    psave: value of pp from the last call to btpec
    nsave: value of n from the last call to btpec
    xnp:  value of the mean from the last call to btpec

    p: probability used in the generation phase of btpec
    ffm: temporary variable equal to xnp + p
    m:  integer representation of the current mode
    fm:  floating point representation of the current mode
    xnpq: temporary variable used in setup and squeezing steps
    p1:  area of the triangle
    c:  height of the parallelograms
    xm:  center of the triangle
    xl:  left end of the triangle
    xr:  right end of the triangle
    al:  temporary variable
    xll:  rate for the left exponential tail
    xlr:  rate for the right exponential tail
    p2:  area of the two parallelograms plus p1
    p3:  area of the left exponential tail plus p2
    p4:  area of the right exponential tail plus p3
    u:  a u(0, p4) random variate used first to select one of the
        four regions and then conditionally to generate a value
        from the region
    v:  a u(0, 1) random number used to generate the random value
        (region 1) or transformed into the variate to accept or
        reject the candidate value
    ix:  integer candidate value
    x:  preliminary continuous candidate value in region 2 logic
        and a floating point ix in the accept/reject logic
    k:  absolute value of (ix - m)
    f:  the height of the scaled density function used in the
        accept/reject decision when both m and ix are small
        also used in the inverse transformation
    r: the ratio p/q
    g: constant used in calculation of probability
    mp:  mode plus one,  the lower index for explicit calculation
         of f when ix is greater than m
    ix1:  candidate value plus one,  the lower index for explicit
          calculation of f when ix is less than m
    i:  index for explicit calculation of f for btpe
    amaxp: maximum error of the logarithm of the normal bound
    ynorm: logarithm of the normal bound
    alv:  natural logarithm of the accept/reject variate v
    x1, f1, z,w, z2, x2, f2,  and w2 are temporary variables
    used in the final accept/reject test
    qn: probability of no success in n trials

  remark
    ix and jx could logically be the same variable,  which would
    save a memory position and a line of code.  however,  some
    compilers (e.g., cdc mnf) optimize better when the arguments
    are not involved.

  iseed needs to be double precision if the imsl routine
  ggubfs is used to generate the uniform random number,  otherwise
  type of iseed is dictated by the uniform generator

  Original calling sequence:
      SUBROUTINE BTPEC(N,PP,ISEED,JX)
  New C calling sequence
      JX = ranbinom(N, PP)
  The seeds for the random number generator are globals
*/

#define rand(ISEED) RUNI()
#define  SERIES(X) (t_ = (X)*(X),\
	(13860.0 - (462.0 - (132.0 - (99.0 - 140.0/t_)/t_)/t_)/t_)/(X)/166320.0)
/*
  1/(12*X) - 1/(360*X*X*X) + 1/(1260*X*X*X*X*X) - 1/(1680*X*X*X*X*X*X*X)
  + 1/(1188*X*X*X*X*X*X*X*X*X)
*/

static INT ranbinom(long n, double pp)
{
	static double       psave = -1.0, xnp, xnpq, qn, p, q, r, g;
	static double       p1, p2, p3, p4, c;
	static double       xm, xr, xl, xlr, xll, fm;
	static INT          nsave = -1, m;
	double              f;
	double              u, v, x;
	double              amaxp, ynorm, alv;
	INT                 i, ix, k;
	WHERE("ranbinom");
	
/*
  determine appropriate algorithm and whether setup is necessary
*/
	if (pp != psave || n != nsave)
	{
	/* *****setup,  perform only when parameters change */
		nsave = n;
		psave = pp;
		p = (psave <= 0.5) ? psave : 1.0 - psave;
		q = 1.0 - p;
		xnp = n*p;

		if (xnp < 30.0) 
		{

			/*      inverse cdf logic for mean less than 30 */

			qn = pow(q, (double) n);
			r = p/q;
			g = r*(n + 1);
		} /*if (xnp < 30.0) */
		else
		{
			double        ffm = xnp + p;
			double        al;

			m = (INT) ffm;
			fm = (double) m; /* floor(ffm) */
			xnpq = xnp*q;
			p1 = (INT) (2.195*sqrt(xnpq) - 4.6*q) + 0.5;
			xm = fm + 0.5;
			xl = xm - p1;
			xr = xm + p1;
			c = 0.134 + 20.5/(15.3 + fm);

			al = (ffm - xl)/(ffm - xl*p);
			xll = al*(1.0 + .5*al);

			al = (xr - ffm)/(xr*q);
			xlr = al*(1.0 + .5*al);

			p2 = p1*(1.0 + c + c);
			p3 = p2 + c/xll;
			p4 = p3 + c/xlr;
		} /*if (xnp < 30.0){}else{}*/
	} /*if (pp != psave || n != nsave)*/

	if (xnp < 30.0)
	{
		int        done = 0;

		while (!done)
		{
			ix = 0;
			f = qn;
			u = rand(iseed);

			while (1)
			{
				if (u < f)
				{
					done = 1;
					break;
				}
				if (ix > 110)
				{
					break;
				}
				u -= f;
				ix++;
				f *= g/ix - r;
			} /*while (1)*/
		} /*while (!done)*/
	} /*if (xnp < 30.0)*/
	else
	{
		while (1)
		{
			/* *****generate variate */

			u = rand(iseed)*p4; /*2*/
			v = rand(iseed);


			if (u <= p1)
			{
				/*      triangular region */
				ix = (INT) (xm - p1*v + u);
				break;
			}


			if (u <= p2) /*3*/
			{
				/*      parallelogram region */
				x = xl + (u - p1)/c;
				v = v*c + 1.0 - fabs(xm - x)/p1;
				if (v > 1.0 || v <= 0.0)
				{
					continue; 
				}
				ix = (INT) x;
			} /*if (u <= p2) */
			else
			{
				if (u > p3) /*4*/ 
				{ /*5*/
					/*      right tail */

					ix = xr - log(v)/xlr;
					if (ix > n)
					{
						continue;
					}
					v *= (u - p3)*xlr;
				} /*if (u > p3)*/
				else 
				{
					/*      left tail */
					ix = xl + log(v)/xll;
					if (ix < 0)
					{
						continue;
					}
					v *= (u - p2)*xll;
				}
			} /*if (u <= p2) {}else{}*/

			/* *****determine appropriate way to perform accept/reject test */

			k = labs(ix - m); /*6*/
			if (k <= 20 || k >= xnpq/2 - 1) 
			{
				/*      explicit evaluation */

				f = 1.0;
				r = p/q;
				g = (n + 1)*r;
				if (m < ix) 
				{
					for (i = m + 1;i <= ix ;i++)
					{
						f *= g/i - r;
					}
				}
				else if (m != ix) 
				{
					for (i = ix+1;i <= m ;i++)
					{
						f /= g/i - r;
					}
				}
				if (v <= f)
				{
					goto normalExit;
				}
			} /*if (k <= 20 || k >= xnpq/2 - 1) */
			else 
			{
				/*12*/
				/*      squeezing using upper and lower bounds on alog(f(x)) */

				amaxp = (k/xnpq)*((k*(k/3.0 + .625) + .1666666666666)/xnpq + .5);
				ynorm = -k*k/(2.0*xnpq);
				alv = log(v);
				if (alv < ynorm - amaxp)
				{
					goto normalExit;
				}
				if (alv <= ynorm + amaxp) 
				{
					double       x1, f1, z, w;
					double       t_; /*temporary used by macro SERIES*/
					/*      Stirling's formula to machine accuracy for */
					/*      the final acceptance/rejection test */

					x1 = ix + 1;
					f1 = fm + 1.0;
					z = n + 1 - fm;
					w = n - ix + 1;

					if (alv <= xm*log(f1/x1) + (n - m + .5)*log(z/w) +
						(ix - m)*log(w*p/x1*q) +
						SERIES(f1) + SERIES(z) + SERIES(x1) + SERIES(w))
					{
						goto normalExit;
					}
				} /*if (alv <= ynorm + amaxp) */
			} /*if (k <= 20 || k >= xnpq/2 - 1) {}else{}*/
		} /*while (1)*/
	} /*if (xnp < 30.0){}else{}*/
	
  normalExit: /*16*/
	return ( (psave > 0.5) ? n - ix : ix);
} /*ranbinom()*/

void vuni(long count, double * rvec)
{
	long            i;
	double          randm1 = (double) RANDM1;
	
	for (i = 0; i < count; i++)
	{
		rvec[i] = iuni()/randm1;
	}
} /*vuni()*/

void vnorm(long count, double * rvec)
{
	long            i;
	double          u, v, x, z;
	double          randm1 = (double) RANDM1;
	
	for (i = 0; i < count; i++)
	{
		while (1)
		{
			u = iuni()/randm1;
			v = 1.715528 * (iuni()/randm1 - 0.5);
			x = v / u;
			z = x * x / 4.;
			if (z < 1 - u || z <= .25924 / u + 0.35 && z < -log(u))
			{
				break;
			}
		} /*while (1)*/
		rvec[i] = x;
	} /*for (i = 0; i < count; i++)*/
} /*vnorm()*/

static void vpoi(long count, double lambda [], long lengthLambda,
				 double * rvec)
{
	long            i, iL, incL = (lengthLambda == 1) ? 0 : 1;
	
	for (i = iL = 0; i < count; i++, iL += incL)
	{
		rvec[i] = (double) ranpois(lambda[iL]);
	}
} /*vpoi()*/

static void vbinom(long count, double n[], long lengthN,
				   double p[], long lengthP, double * rvec)
{
	long            i, iN, iP;
	long            incN = (lengthN == 1) ? 0 : 1;
	long            incP = (lengthP == 1) ? 0 : 1;
	WHERE("vbinom");
	
	for (i = iN = iP = 0; i < count; i++, iN += incN, iP += incP)
	{
		rvec[i] = ranbinom((long) n[iN], p[iP]);
	} /*for (i = 0; i < count; i++)*/
} /*vbinom()*/
	
void randomSeed(long verbose)
{
#ifdef  MACINTOSH
	GetDateTime((unsigned long *) &Rands1);
#else /*MACINTOSH*/
#ifndef NOTIMEH
	Rands1 = (long) time((time_t *) 0);
#else /*NOTIMEH*/
	Rands1 = 1891756027;
#endif /*NOTIMEH*/
#endif /*MACINTOSH*/

	Rands2 = Rands1 & 1360626473;
/* throw away first two numbers */
	(void) iuni();
	(void) iuni();
	if (verbose)
	{
		sprintf(OUTSTR,"NOTE: random number seeds set to %ld and %ld",
				Rands1, Rands2);
		putOUTSTR();
	}	
} /*randomSeed()*/


enum ranOpcodes
{
	IRNORM = 1,
	IRUNI,
	IRPOI,
	IRBINOM
};


Symbolhandle    rangen(Symbolhandle list)
{
	Symbolhandle    result = (Symbolhandle) 0, symhN, symhParam1, symhParam2;
	long            sampleSize, lengthParam1, lengthParam2;
	long            op, verbose = 1;
	long            nargs = NARGS(list), margs;
	char           *what1, *what2;
	char           *badarg = "argument %ld (%s) to %s()";
	char           *badParam =
	  "ERROR: argument %ld to %s() (%s) not REAL scalar or vector";
	char           *hasMissing =
	  "ERROR: argument %ld to %s() (%s) has 1 or more MISSING values";
	char           *badLength =
	  "ERROR: length of vector argument %ld to %s() (%s) differs from sample size";
	WHERE("rangen");
	
	/* generate a list of random numbers */
	if (strcmp(FUNCNAME, "rnorm") == 0)
	{
		op = IRNORM;
	}
	else if (strcmp(FUNCNAME, "rpoi") == 0)
	{
		op = IRPOI;
		what1 = "mean";
	}
	else if (strcmp(FUNCNAME, "rbin") == 0)
	{
		op = IRBINOM;
		what1 = "n";
		what2 = "p";
	}
	else
	{
		op = IRUNI;
	}
	margs = (op == IRBINOM) ? 3 : ((op == IRPOI) ? 2 : 1);
	
	OUTSTR[0] = '\0';
	
	if (nargs != margs)
	{
		badNargs(FUNCNAME, margs);
		goto errorExit;
	}
	
	symhN = COMPVALUE(list,0);

	if (!isInteger(symhN, POSITIVEVALUE))
	{
		char    outstr[30];
		
		sprintf(outstr, badarg, 1L, "sample size", FUNCNAME);
		notPositiveInteger(outstr);
		goto errorExit;
	} /*if (!isInteger(symhN, POSITIVEVALUE))*/

	sampleSize = (long) DATAVALUE(symhN,0);

	if (margs > 1)
	{
		symhParam1 = COMPVALUE(list, 1);
		if (!argOK(symhParam1, 0, 2))
		{
			goto errorExit;
		}
		lengthParam1 = symbolSize(symhParam1);
		if (TYPE(symhParam1) != REAL || !isVector(symhParam1))
		{
			sprintf(OUTSTR, badParam, 2L, what1, FUNCNAME);
		}
		else if (anyMissing(symhParam1))
		{
			sprintf(OUTSTR, hasMissing, 2L, what1, FUNCNAME);
		}
		else if (lengthParam1 > 1 && lengthParam1 != sampleSize)
		{
			sprintf(OUTSTR, badLength, 2L, what1, FUNCNAME);
		}
		else if (margs > 2)
		{
			symhParam2 = COMPVALUE(list, 2);
			if (!argOK(symhParam2, 0, 3))
			{
				goto errorExit;
			}

			lengthParam2 = symbolSize(symhParam2);
			if (TYPE(symhParam2) != REAL || !isVector(symhParam2))
			{
				sprintf(OUTSTR, badParam, 3L, what2, FUNCNAME);
			}
			else if (anyMissing(symhParam2))
			{
				sprintf(OUTSTR, hasMissing, 3L, what2, FUNCNAME);
			}
			else if (lengthParam2 > 1 && lengthParam2 != sampleSize)
			{
				sprintf(OUTSTR, badLength, 3L, what2, FUNCNAME);
			}
		}

		if (*OUTSTR)
		{
			goto errorExit;
		}
		
		if (op == IRPOI)
		{
			/*rpoi(lambda)*/
			if (doubleMin(DATAPTR(symhParam1), lengthParam1) < 0.0)
			{
				sprintf(OUTSTR,
						"ERROR: argument 2 (%s) to %s() has negative element",
						what1, FUNCNAME);
			}
		}
		else if (op == IRBINOM)
		{
			/*rbinom(samplesize,n,p)*/
			long        i;
			double     *n = DATAPTR(symhParam1);

			for (i = 0; i < lengthParam1; i++)
			{
				if (n[i] < 1 || n[i] != floor(n[i]))
				{
					sprintf(OUTSTR,
							"ERROR: not all elements of argument 2 (%s) to %s() are positive integers",
							what1, FUNCNAME);
					goto errorExit;
				}
			} /*for (i = 0; i < lengthParam1; i++)*/

			if (doubleMin(DATAPTR(symhParam2), lengthParam2) < 0.0 ||
					 doubleMax(DATAPTR(symhParam2), lengthParam2) > 1.0)
			{
				sprintf(OUTSTR,
						"ERROR: argument 3 (%s) to %s() has value < 0 or > 1",
						what2, FUNCNAME);
			}
		} /*else if (op == IRBINOM)*/

		if (*OUTSTR)
		{
			goto errorExit;
		}
	} /*if (margs > 1)*/

	if (Rands1 == 0 && Rands2 == 0)
	{
		randomSeed(verbose);
	} /*if (Rands1 == 0 && Rands2 == 0)*/

	result = RInstall(SCRATCH, sampleSize);
	if (result != (Symbolhandle) 0)
	{
		switch (op)
		{
		  case IRUNI:
			vuni(sampleSize, DATAPTR(result));
			break;
		  case IRNORM:
			vnorm(sampleSize, DATAPTR(result));
			break;
		  case IRPOI:
			vpoi(sampleSize, DATAPTR(symhParam1), lengthParam1,
				 DATAPTR(result));
			break;
		  case IRBINOM:
			vbinom(sampleSize, DATAPTR(symhParam1), lengthParam1,
				   DATAPTR(symhParam2), lengthParam2, DATAPTR(result));
			break;
		} /*switch (op)*/
	} /*if (result != (Symbolhandle) 0)*/

	return (result);

  errorExit:
	putErrorOUTSTR();

	return (0);
	
} /*rangen()*/

/*
   Seed the random number generator
   Usage: setseeds(seed1, seed2) or setseeds(cat(seed1,seed2))
*/

Symbolhandle    setseed(Symbolhandle list)
{
	Symbolhandle    symhf, symhl, symhKey;
	double          first, last;
	long            nargs = NARGS(list);
	long            margs = nargs;
	long            verbose = 1;
	char           *keyword;
	
	OUTSTR[0] = '\0';

	if (nargs > 3)
	{
		badNargs(FUNCNAME, -3);
		goto errorExit;
	}
	if (nargs > 1)
	{ /* check for quiet:T or simply logical scalar 3rd argument */
		symhKey = COMPVALUE(list,nargs-1);
		if (!argOK(symhKey, 0, nargs))
		{
			goto errorExit;
		}
		if ((keyword = isKeyword(symhKey)) && strcmp(keyword, "quiet") != 0)
		{
			badKeyword(FUNCNAME, keyword);
			goto errorExit;
		}
		if (isTorF(symhKey))
		{
			verbose = (DATAVALUE(symhKey,0) == 0);
			nargs--;
		}
		else if(keyword || nargs == 3)
		{
			notTorF((keyword) ? keyword : "argument 3");
			goto errorExit;
		}
	} /*if (nargs > 1)*/
	
	symhf = COMPVALUE(list,0);
	if(!argOK(symhf, REAL, (margs > 1) ? 1 : 0))
	{
		goto errorExit;
	}

	if(nargs == 2)
	{
		if(!argOK(symhl = COMPVALUE(list,1), REAL,2))
		{
			goto errorExit;
		}
		if (!isInteger(symhf, NONNEGATIVEVALUE) ||
			!isInteger(symhl, NONNEGATIVEVALUE))
		{
			sprintf(OUTSTR,
					"ERROR: when seeds are separate arguments, both must be integers >= 0");
		}
		else
		{
			first = DATAVALUE(symhf,0);
			last = DATAVALUE(symhl,0);
		}		
	} /*if(nargs == 2)*/
	else
	{
		if(!isVector(symhf) || symbolSize(symhf) != 2)
		{
			sprintf(OUTSTR,
					"ERROR: a single seed argument to %s must be a vector of length 2",
					FUNCNAME);
		}
		else
		{
			first = DATAVALUE(symhf,0);
			last = DATAVALUE(symhf,1);
		}
	} /*if(nargs == 2){}else{}*/
	if(*OUTSTR)
	{
		goto errorExit;
	}
	
	if(isMissing(first) || isMissing(last))
	{
		sprintf(OUTSTR,"ERROR: missing values not allowed as seeds");
	}
	else if(first != floor(first) || first < 0 ||
			last != floor(last) || last < 0 ||
			first > RANDM1 || last > RANDM2)
	{
		sprintf(OUTSTR,
				"ERROR: seeds must non-negative integers, seed1 <= %ld and seed2 <= %ld",
				RANDM1, RANDM2);
	}
	if(*OUTSTR)
	{
		goto errorExit;
	}
	

	if(first == 0 || last == 0)
	{
		randomSeed(verbose);
	}
	else
	{
		Rands1 = first;
		Rands2 = last;
	}	

	return (NULLSYMBOL);

  errorExit:
	
	putErrorOUTSTR();
	return (0);

} /*setseed()*/

Symbolhandle    getseed(Symbolhandle list)
{
	Symbolhandle    result = (Symbolhandle) 0;
	Symbolhandle    symhKey;
	char           *keyword;
	long            verbose = 1;
	long            nargs = NARGS(list);
	
	if(nargs > 1)
	{
		badNargs(FUNCNAME, -1);
		goto errorExit;
	}
	
	symhKey = COMPVALUE(list,0);
	if (symhKey != (Symbolhandle) 0)
	{
		if (!argOK(symhKey, 0, 0))
		{
			goto errorExit;
		}
		if ((keyword = isKeyword(symhKey)) && strcmp(keyword, "quiet") != 0)
		{
			badKeyword(FUNCNAME, keyword);
			goto errorExit;
		}
		if (!isTorF(symhKey))
		{
			notTorF("argument");
			goto errorExit;
		}
		verbose = (DATAVALUE(symhKey,0) == 0);
	}
	if (verbose)
	{
		sprintf(OUTSTR, "Seeds are %ld and %ld", Rands1, Rands2);
		putOUTSTR();
	}
	
	result = RInstall(SCRATCH, 2);
	
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}
#ifdef INVISIBLESYMBOLS
	if (verbose)
	{
		setNAME(result, INVISSCRATCH);
	}
#endif /*INVISIBLESYMBOLS*/

	DATAVALUE(result,0) = (double) Rands1;
	DATAVALUE(result,1) = (double) Rands2;

	return (result);

  errorExit:
	return ((Symbolhandle) 0);

} /*getseed()*/

