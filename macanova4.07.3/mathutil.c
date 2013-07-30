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
#pragma segment Utils
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
#include "plot.h"
#include <ctype.h>

/*
   Contains some purely mathematical routines, possibly of use in
   more than one MacAnova function.
     doSwp(), fsolve()
	 epslon()
   Also contains substitutes for various functions that may not be
   available on all systems.
     mystrtod() (synonym for strtod()), pythag() (synonym for hypot())
	 intpow() (replacement for pow(x,y) when y is an integer)
	 mylgamma() (synonym for lgamma())
	 index(), rindex()
  970217 added fsolve()
*/

/*
   Do single non-symmetric Beaton SWP on matrix.
	This version of swp does a non-symmetric sweep of the	m by n  matrix cp.
	The pivot row/column is specified by k.  The original k-th diagonal
	element is cpdiag

	If full != 0, a complete swp of all of cp is done.
	If full == 0, only columns > k are swept;  use this if you are
       sweeping on an increasing sequence of columns and are primarily
	   interested in the lower right hand corner of cp.
	In case of singularity, 0 is returned; otherwise 1
	
*/
#define SINGULAR    0
#define NONSINGULAR 1

long doSwp(double * cp,long m,long n,long k,double cpdiag, long full)
{
	long             i,j, istart, jstart;
	register double  beta;
	double           pivot;
	double          *cpj, *cpk;
	WHERE("doSwp");

	istart = jstart = (full) ? 0 : k + 1;
	cpk = cp + m*k;		/* column k of cp */
	cpj = cp + m*jstart; /* column j of cp */

	pivot = cpk[k]; /* cp[k,k]*/
	if(fabs(pivot) <= REGTOL*fabs(cpdiag))
	{
		return (SINGULAR);
	}

	/* sweep out of all columns except k */
	for (j = jstart;j < n;j++)
	{
		if(j != k)
		{
			beta =  cpj[k]/pivot; /* cp[k,j]/c[k,k] */
			for (i = istart;i < m;i++)
			{ /* no test for i == k => cost 1 redundant operation */
				cpj[i] -= beta*cpk[i]; /* cp[i,j]-=cp[i,k]*beta */
			} /*for (i = istart;i < m;i++)*/
			if (full)
			{
				cpj[k] = -beta; /* cp[k,j] = -beta*/
			}
		} /*if(j != k)*/
		cpj += m;
	} /*for (j = jstart;j < n;j++)*/

	/* Now fix up column k*/
	if (full)
	{
		for (i = 0;i < m;i++)
		{
			if(i != k)
			{
				cpk[i] /= pivot; /* cp[i,k] /= cp[k,k];*/
			} /*if (i != k)*/
			else
			{
				cpk[i] = 1.0/pivot;	/* cp[k,k] = 1/cp[k,k]; */
			}
		} /*for (i = 0;i < m;i++)*/
	} /*if (full)*/
	
	return (NONSINGULAR);

} /*doSwp()*/

#undef MAX
#undef MIN
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

#undef INT
#define INT       long

/*
  Routine to find the solution z of f(z)=a where the function f(.)
  is strictly monotonic either increasing or decreasing.

  Arguments:
  All real arguments and func are in double precision.
  arg    the value for which the solution is to be found
         in other words we want z to solve f(z) = arg
  func   an external function that evaluates f(z) and returns
         as its value f(z)
  param, nparam
         additional arguments that func may require.  They are passed
         straight through when func is called.  func
         is called as  y = func(z,param,nparam).  param is a vector
         of additional parameters, and nparam is the number of
         such additional parameters.
  ntab  the size of the working arrays x,y, and pr
  xmax, xmin
        given limits within which the solution
        is known a priori to lie.  If you don't know the range
        of possible values, set large positive and negative values
        for xmax and xmin respectively.
  eps   the value |f(z)-arg| must attain for the procedure
        to be considered to have converged.
  x and y
        work arrays for intermediate trial z values and their associated
        f(z)-arg. On input, the first two elements of x must be set by the
        user to initial guesses at the solution.  Ideally
        these guesses should straddle the true solution.
  nxy is returned
        The number of x,y, pairs studied.
  pr    a work array.
  soln  the solution z to the equation - that is
        func(soln,param,nparam) = arg
  iter is returned
        the number of iterations
  ifault is an error code.  it should be zero.

   Code received from Douglas Hawkins, Applied Statistics,
   University of Minnesota, 1/23/96

   Modified by C. Bingham
   970217 added argument itmax  also nxy, iter and ifault are returned
   in argument vecto report

   971016 func() now has a 4th long * argument to report errors
   or interruption
*/
#define Nxy    report[0]
#define Iter   report[1]
#define Ifault report[2]


double fsolve(double arg, double (*func) (double ,double [], INT, INT *),
			  double param [], INT nparam, double xmax, double xmin,
			  double eps, INT itmax, INT ntab,
			  double x [], double y [], double pr [], INT report[])
/*	double x[ntab], y[ntab], param[nparam], pr[ntab];*/
{
	double     aminim = 0.8, big = 1e20, small = 1.e-20;
	double     step, cerr, ocerr, up, xlo, xhi, ydif, prnew, guess;
	double     off;
	INT        nt, i, iii, low, skip;
	WHERE("fsolve");
	
	Iter = 2;
	Ifault = 0;
	nt = 1;
	iii = 1;
	low = -1;
	step = 1.0;
	if (ntab < 4 || xmax <= xmin || eps <= 0.0)
	{
		Ifault = FSOLVEBADARGUMENTS;
		goto errorExit;
	} /*if (ntab < 4 || xmax <= xmin || eps <= 0.0)*/

	y[0] = func(x[0], param, nparam, &Ifault) - arg;
	if (Ifault)
	{
		goto errorExit;
	}
	
	y[1] = func(x[1], param, nparam, &Ifault) - arg;
	if (Ifault)
	{
		goto errorExit;
	}

	if (fabs(y[0] - y[1]) < small)
	{
		Ifault = FSOLVEBADGUESSES;
		goto errorExit;
	}

	cerr = MIN(fabs(y[0]), fabs(y[1]));

	up = ((x[0] - x[1])*(y[0] - y[1]) > 0.0) ? 1.0 : -1.0;

	if (y[0]*y[1] <= 0.0)
	{
		xlo = MIN(x[0], x[1]);
		xhi = MAX(x[0], x[1]);
	} /*if (y[0]*y[1] <= 0.0)*/
	else if (y[0] * up > 0.0)
	{
		xlo = xmin;
		xhi = MIN(x[0], x[1]);
	}
	else
	{
		xlo = MAX(x[0], x[1]);
		xhi = xmax;
	}
	
	while(1)
	{
		guess = 0.0;
		if (low < 2)
		{
			low++;
		}
		
		if (nt - low < 2)
		{
			pr[low] = x[low];
		}
		prnew = x[nt];
		for(i = low;i < nt ;i++)
		{
			ydif = y[i] - y[nt];
			prnew *= y[i]/ydif;
			pr[i] *= -y[nt]/ydif;
			guess += pr[i];
		} /*for(i = low;i < nt ;i++)*/

		pr[nt] = prnew;
		guess += prnew;
		skip = ((xhi - guess)*(xlo - guess) > 0.0);
		while (2)
		{
			if (!skip)
			{
/*70*/			if (cerr < eps)
				{
					goto normalExit;
				}

				if (++Iter > itmax)
				{
					Ifault = FSOLVENOTCONVERGED;
					goto errorExit;
				}
				off = func(guess, param, nparam, &Ifault) - arg;
				if (Ifault)
				{
					goto errorExit;
				}
				for(i = 0;i <= nt ;i++)
				{
					if ((y[i] - off)*up*(x[i] - guess) < 0.0)
					{
						Ifault = FSOLVENONMONOTONIC;
						goto errorExit;
					}
/*80*/			} /*for(i = 0;i <= nt ;i++)*/
				if (up*off < 0.0)
				{
					xlo = guess;
				}
				if (up*off > 0.0)
				{
					xhi = guess;
				}
				ocerr = cerr;
				cerr = fabs(off);
				if (cerr <= aminim*ocerr)
				{
					break;
				}
			} /*if (!skip)*/
			else
			{
				skip = 0;
			}
			
			if (xlo > (-big) && xhi < big) /*60*/
			{
				guess = 0.5*(xlo + xhi);
			} /*if (xlo > (-big) && xhi < big)*/
			else if (cerr > eps) /*40*/
			{
				if (xlo <= (-big))
				{
					guess = xhi - step;
					guess = MAX(guess, xmin);
				}
				else
				{
					guess = xlo + step;
					guess = MIN(guess, xmax);
				} 
				step += step;
			} /*if (xlo > (-big) && xhi < big){}else if (cerr > eps){}*/
		} /*while (2)*/

		if (nt < ntab - 1)
		{
			nt++;
		} /*if (nt < ntab - 1)*/
		else
		{
			if (++iii >= ntab)
			{
				iii = 2;
			}
			for(i=low;i < ntab ;i++)
			{
				pr[i] *= -(y[i] - y[iii])/y[iii];
			}
			x[iii] = x[nt];
			y[iii] = y[nt];
			pr[iii] = pr[nt];
		} /*if (nt < ntab - 1){}else{}*/
		x[nt] = guess;
		y[nt] = off;
	} /*while(1)*/
	
  normalExit:
	/*fall through*/
	
  errorExit:
	Nxy = nt;
	return (guess);
	
} /*fsolve()*/

/*NOSTRTOD defined implies no working strtod in standard library*/
#if defined(NOSTRTOD)

/*
  version of strtod not using sscanf

  Note:  This is non-conforming since it always expects '.' as "radix
  character", regardless of locale

  C. Bingham, 911012
  960403 Corrections made
*/

#define MAXPOW 20
static double powers[MAXPOW+1] =
{
	1e0,1e1,1e2,1e3,1e4,1e5,1e6,1e7,1e8,1e9,1e10,
	1e11,1e12,1e13,1e14,1e15,1e16,1e17,1e18,1e19,1e20
};

double mystrtod(char *nptr, char **eptr)
{
	long       ssign = 1, esign = 1, decPlaces = 0;
	double     mantissa = 0.0, expon = 0.0;
	char      *endptr;
	long       place;
	short      c;
	WHERE("mystrtod");
	
	if(eptr != (char **) 0)
	{
		*eptr = nptr;
	}
	if(*nptr == '\0')
	{
		return (0.0);
	}
	/* expected pattern is [-+]?[0-9]*\.?[0-9]*\([eE][ -+]?[0-9][0-9]*\)? */

	place = 0;
	while( isspace( nptr[place] ))
	{
		place++;					/* skip leading nonnumeric */
	}

	c = nptr[place];
	if(c == '+' || c == '-')
	{
		if(c == '-')
		{
			ssign = -1;
		}
		c = nptr[++place];
	}

	if(isdigit(c))
	{							/* decode integral part of mantissa */
		do
		{
			mantissa = 10.0*mantissa + (c - '0');
			c = nptr[++place];
		} while(isdigit(c));
	}
	else if(c != '.' || !isdigit(nptr[place+1]))
	{							/* not a number */
		return (0.0);
	}

	if(c == '.')
	{							/* decode fractional part of mantissa */
		c = nptr[++place];
		if(isdigit(c))
		{
			do
			{
				mantissa = 10.*mantissa + (c - '0');
				decPlaces++;
				c = nptr[++place];
			}while(isdigit(c));
		} /*if(isdigit(c))*/
	} /*if(c == '.')*/

	endptr = nptr + place;
	place++;

	if(c == 'e' || c == 'E')
	{ /* decode exponent */
		if((c = nptr[place]) == '-' || c == '+' || c == ' ')
		{
			if(c == '-')
			{
				esign = -1;
			}
			c = nptr[++place];
		}
		if(isdigit(c))
		{
			do
			{ /* decode values of exponent */
				expon = 10.0*expon + (c - '0');
				c = nptr[++place];
			} while(isdigit(c));
			endptr = nptr + place;
			if(esign < 0)
			{
				expon = -expon;
			}
		}
	}
	expon -= (double) decPlaces;

	if(fabs(expon) > (double) MAXPOW)
	{
#ifdef DBL_MAX_10_EXP
		if (expon > DBL_MAX_10_EXP)
		{
			mantissa = TOOBIGVALUE;
		}
		else
#endif /*DBL_MAX_10_EXP*/
		mantissa *= pow(10.,expon);
	} /*if(fabs(expon) > (double) MAXPOW)*/
	else
	{
		if(expon >= 0.0)
		{
			mantissa *= powers[(long) expon];
		}
		else
		{
			mantissa /= powers[-(long) expon];
		}
#ifdef HASINFINITY
		if (isInfinite(mantissa))
		{
			mantissa = HUGE_VAL;
		}
#endif /*HASINFINITY*/
#ifdef HASNAN
		if (isNaN(mantissa))
		{
			mantissa = HUGE_VAL;
		}
#endif /*HASNAN*/
	} /*if(fabs(expon) > (double) MAXPOW){}else{}*/

	if(eptr != (char **) 0)
	{
		*eptr = endptr;
	}

	return ((ssign < 0) ? -mantissa : mantissa);

} /*mystrtod()*/
#else /*NOSTRTOD*/
/* system library has strtod() */
double mystrtod(char *nptr, char **eptr)
{
	return (strtod(nptr, eptr));
} /*mystrtod()*/

#endif /*NOSTRTOD*/

#ifdef NOHYPOT
#undef MAX
#define MAX(A,B) (((A) > (B)) ? (A) : (B))
#undef MIN
#define MIN(A,B) (((A) < (B)) ? (A) : (B))

/*
   Finds sqrt(a**2+b**2) without overflow or destructive underflow

   980821 added check for overflow
*/
#if (1)
/*
  Version using sqrt()
  On HPGCC version, gives same result as library hypot(), more accurate
  than iterative version below
*/
double pythag(double a, double b)
{
	double        p;

	a = fabs(a);
	b = fabs(b);
	p = MAX(a, b);

	if (p != 0.0)
	{
		double        r = MIN(a, b)/p;
		double        p1 = sqrt(1.0 + r*r);

		p = (p > TOOBIGVALUE/p1) ? TOOBIGVALUE : (p1*p);
	} /*if (p != 0.0)*/
	return (p);
} /*pythag()*/
#else /*1*/
/*
  Version adapted from function PYTHAG in Eispack that computes
  square root iteratively
*/
double pythag(double a, double b)
{
	double        p;

	a = fabs(a);
	b = fabs(b);
	p = MAX(a, b);

	if (p != 0.0)
	{
		double        r = MIN(a, b)/p;
		double        p1 = 1.0;
		double        s, t, u, temp;
		
		r *= r;
		while(1)
		{
			t = 4.0 + r;
			if (t == 4.0)
			{
				break ;
			}
			s = r/t;
			u = 1.0 + 2.0*s;
			p1 *= u;
			temp = s/u;
			r *= temp*temp;
		} /*while(1)*/
		p = (p > TOOBIGVALUE/p1) ? TOOBIGVALUE : (p1*p);
	} /*if (p != 0.0)*/
	return (p);
} /*pythag()*/
#endif /*1*/
#else /*NOHYPOT*/
#ifndef MPW /*following is inconsistant with MPW*/
extern double hypot(double, double);
#endif /*MPW*/

double pythag(double a, double b)
{
	return hypot(a,b);
} /*pythag()*/
#endif /*NOHYPOT*/

#ifndef USEPOW
/*
  To be used when system pow() is inefficient with integer exponents
  as with MPW C, or is inaccurate as with gcc in HPUX
*/

double intpow(register double x, double p)
{
	register double         y;
	register long           iexp = labs((long) p);
	
	y = (iexp & 1) ? x : 1.0;
	
	for (iexp >>= 1 ;iexp != 0;iexp >>= 1)
	{
		x *= x;
		if(iexp & 1)
		{
			y *= x;
		}
	} /*for (iexp >>= 1 ;iexp != 0;iexp >>= 1)*/
	
	return ((p < 0.0) ? 1.0/y : y);
} /*intpow()*/
#else /*USEPOW*/
double intpow(double x, double p)
{
	double       y = pow(fabs(x), p);

	return ((x < 0 && ((long) p) % 2 != 0) ? -y : y);
} /*intpow()*/
#endif /*USEPOW*/

#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment Trans
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

#if defined(MW_CW) && !defined(powerpc)
/*
	Avoid problems with using both math.h and fp.h on 68K Mac compile
*/
#define NOLGAMMA
#endif

#ifndef NOLGAMMA
#ifdef MW_CW
#include <fp.h>
#else /*MW_CW*/
extern double lgamma(double);
#endif /*MW_CW*/

double mylgamma(double x)
{
	return (lgamma(x));
} /*mylgamma()*/
#else /*NOLGAMMA*/
/* No system lgamma available */

static double   ln2pio2 = 0.9189385332046727417803297;

static double   clocal[] =
{
	-0.577191652,
	0.988205891,
	-0.897056937,
	0.918206857,
	-0.756704078,
	0.482199394,
	-0.193527818,
	0.035868343,
};
static double   casymp[] =
{
	12.,
	-360.,
	1260.,
	-1680.,
};


double          mylgamma(double x)
{
	long            i;
	double          out, z, z2, value;

	if (x > 8.)
	{			/* asymptotic, 6.1.41 of Abramowitz and Stegun */
		z = 1 / x;
		z2 = z * z;
		out = 0.0;
		for (i = 0; i < 4; i++)
		{
			out += z / casymp[i];
			z *= z2;
		}
		return ((x - .5) * log(x) - x + ln2pio2 + out);
	}

	/* small x, 6.1.36 of Abramowitz and Stegun */
	value = 0.0;
	while (x < 1.0)
	{
		value -= log(x++);
	}
	while(x > 2.0)
	{
		value += log(--x);
	}
	
	z = x - 1.;
	z2 = z;
	out = 1.;
	for (i = 0; i < 8; i++)
	{
		out += clocal[i] * z2;
		z2 *= z;
	}
	return (log(out) + value);
} /*mylgamma()*/

#endif /*NOLGAMMA*/

/* From Eispack.  Moved here from eigutils.c */
/*      estimate unit roundoff in quantities of size x. */


/*
	This program should function properly on all systems
      satisfying the following two assumptions,
         1.  The base used in representing floating point
             numbers is not a power of three.
         2.  The quantity  a  in statement 10 is represented to
             the accuracy used in floating point variables
             that are stored in memory.
      The label LAB10 and the goto LAB10 are intended to
      force optimizing compilers to generate code satisfying
      assumption 2.
      Under these assumptions, it should be true that,
             a  is not exactly equal to four-thirds,
             b  has a zero for its last bit or digit,
             c  is not exactly equal to one,
             eps  measures the separation of 1.0 from
                  the next larger floating point number.
      The developers of eispack would appreciate being informed
      about any systems where these assumptions do not hold.

      This version dated 4/6/83.
*/

double          epslon(double x)
{
	double          a, b, c, eps;

	a = 4.0 / 3.0;
/* use of label designed to out-fox optimizing compiler; see above */
  LAB10:
	b = a - 1.0;
	c = b + b + b;
	eps = fabs(c - 1.0);
	if (eps == 0)
	{
		goto LAB10;
	}
	return (eps * fabs(x));
} /*epslon()*/

#ifdef PROVIDEINDEX
/*
   These are needed under gcc on Epx since one of the libraries
   refers to them.  It seemed easier to provide the code
   than to try to find which, if any, additional library to link
   If these are needed for an installation, PROVIDEINDEX should
   be defined in platform.h
*/
char *index(char *s, char c)
{
	return (strchr(s, c));
}

char *rindex(char *s, char c)
{
	return (strrchr(s, c));
}

#endif /*PROVIDEINDEX*/
