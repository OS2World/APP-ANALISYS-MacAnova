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
#pragma segment Polyroot
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

#ifndef USEPOW
#undef pow
#define pow intpow /*all uses of pow are with integer power*/
#endif /*USEPOW*/
/* globals for polyroot */

static double         *P, *Qp, *K;
static double         *Qk, *Svk;
static double         Sr, Si, U, V, A, B, C, D;
static double         A1, A3, A7, E, F, G, H, Szr, Szi, Lzr, Lzi;

static double         Eta, Are, Mre;

static long           N, Nn;
#define Criter        20

#define SCALEBYC      1
#define SCALEBYD      2
#define UNSCALED      3

#define quad  myquad /* to overcome a type conflict with gcc */

/*static double         Criter;*/

/*
	common /global/ p, qp, k, qk, svk, sr, si, u,
     * v, a, b, c, d, a1, a2, a3, a6, a7, e, f, g,
     * h, szr, szi, lzr, lzi, eta, are, mre, n, nn, criter
*/

/* stuff formerly in rpoly.h */
#define MAXLEN 121 
/*
	  The following defines set machine constants used
	  in various parts of the program. The meaning of the
	  four constants are...
	  ETA     the maximum relative representation error
	  which can be described as the smallest
	  positive floating point number such that
	  1.0+ETA is greater than 1.
	  INFINY  the largest floating-point number.
	  SMALNO  the smallest positive floating-point number
	  if the exponent range differs in single and
	  double precision then SMALNO and INFINY
	  should indicate the smaller range.
	  kb's note: since all computations are in double i used dp value
	  BASE    the base of the floating-point number
	  in the system used.
	  */
/*
  The values below correspond to the burroughs b6700 from original code
*/
/*
#define BASE      8.0
#define ETA       .5*pow(BASE,(double) 1 - 26)
#define INFINY    4.3e68
#define SMALNO    1.0e-45
 */
/*
  single precision values appropriate to decstation 3100 (kb)
*/
/*
#define BASE      2.0
#define ETA       pow(BASE,(double) -23)
#define INFINY    3.402835e38
#define SMALNO    1.1754942e-38
*/

/*
	  double precision values appropriate to decstation 3100 (kb)
*/
#define BASE       2.0
#ifndef ETA
#define ETA        pow(BASE,-52.0)
#endif /*ETA*/
#ifndef INFINY
#define INFINY     1.79769313486231e+308 /*  INFINY ~= 2^1024 */
#endif /*INFINY*/
#ifndef SMALNO
#ifdef MW_CW
#define SMALNO     4.4501477170144e-308  /*  2^-1021 */  /* for PPC */
#else /*MW_CW*/
#define SMALNO     2.2250738585072e-308  /*  2^-1022 */
#endif /*MW_CW*/
#endif /*SMALNO*/

/*
  Note:
	  SMALNO = 4.94065645841247e-324 (2**(-1075)) works fine in C using
	  	DEC ultrix C compiler on DecStation 3100, but
	  	does not work with SGI compiler and new DEC f77 compiler
	  2**(-1023) compiles to 0
*/

/* ====> quad.c <==== */

/*
	Function translated from TOMS Algorithm 493, with modifications,
	by C. Bingham
	Calculate the zeros of the quadratic a*z**2+b1*z+c.  The quadratic
	formula, modified to avoid overflow, is used to find the larger zero if
	the zeros are real and both zeros are complex.  The smaller real zero
	is found directly from the product of the zeros c/a.
*/

static void quad(double a, double b1, double c, 
		  double *sr, double *si, double *lr, double *li)
{
	double b, d, e;

	*si = *li = 0.0;
	
	if (a == 0.0)
	{
		*sr = (b1 != 0.0) ? -c/b1 : 0.0;
		*lr = 0.0;
	}
	else if (c == 0.0)
	{
		*sr = 0.0;
		*lr = -b1/a;
	}
	else
	{
		b = b1/2.0;
		if (fabs(b) >= fabs(c))
		{
			e = 1.0 - (a/b)*(c/b);
			d = sqrt(fabs(e))*fabs(b);
		}
		else
		{
			/* compute discriminant, avoiding overflow */
			e = (c >= 0.0) ? a : -a;
			e = b*(b/fabs(c)) - e;
			d = sqrt(fabs(e))*sqrt(fabs(c));
		}
		if (e >= 0.0)
		{
			/*  real zeros */
			if (b >= 0.0)
			{
				d = -d;
			}
			*lr = (-b+d)/a;
			*sr = (*lr != 0.0) ? (c/(*lr))/a : 0.0;
		}
		else
		{
			/* complex conjugate zeros */
			*lr = *sr = -b/a;
			*li = -(*si = fabs(d/a));
		}
	}
} /*quad()*/

/* ====> quadsd.c <==== */


/*
	Function translated from TOMS Algorithm 493, with modifications,
	by C. Bingham
	Divides p by the quadratic 1,u,v placing the quotient in q and the
	remainder in a,b
*/

static void quadsd(long nn, double u, double v,
			double *p, double *q, double *a, double *b)
/*double p[nn],q[nn],u,v,a,b;*/
{
	double          c;
	long            i;

	*b = p[0];
	q[0] = *b;
	*a = p[1] - u*(*b);
	q[1] = *a;
	for(i=2;i < nn;i++)
	{
		c = q[i] = p[i] - u*(*a) - v*(*b);
		*b = *a;
		*a = c;
	}
} /*quadsd()*/

/* ====> realit.c <==== */

/*
	Function translated from TOMS Algorithm 493, with modifications,
	by C. Bingham

	variable-shift h polynomial iteration for a real zero.
	sss   - starting iterate
	nz    - number of zero found
	iflag - flag to indicate a pair of zeros near real axis.
*/
static void realit(double *sss, long *nz, long *iflag)
{
	double           pv, kv, t = 0.0, s;
	double           ms, mp, omp = 0, ee;
	long             i, j;

	*nz = 0;
	s = *sss;
	*iflag = 0;
	j = 0;

	/* main loop */
	while(1)
	{
		/*  evaluate P at s */
		pv = Qp[0] = P[0];
		for(i=1;i < Nn;i++)
		{
			pv = pv*s+P[i];
			Qp[i] = pv;
		}
		mp = fabs(pv);
		/*  compute a rigorous bound on the error in evaluating P*/
		ms = fabs(s);
		ee = (Mre/(Are + Mre))*fabs(Qp[0]);
		for(i=1;i < Nn;i++)
		{
			ee = ee*ms + fabs(Qp[i]);
		}
		/*
			iteration has converged sufficiently if the
			polynomial value is less than Criter times this bound
			original had value of Criter=20 hardwired in
		*/
		if (mp <= Criter*((Are + Mre)*ee - Mre*mp))
		{
			break;
		}
		/*  stop iteration after 10 steps */
		if (++j > 10)
		{
			return;
		}
		if (j >= 2 && fabs(t) <= 0.001*fabs(s - t) && mp > omp)
		{
			/*
			  a cluster of zeros near the real axis has been
			  encountered return with iflag set to initiate a
			  quadratic iteration
			  */
			*iflag = 1;
			*sss = s;
			return;
		}
		omp = mp;
		/*  compute t, the next polynomial, and the new iterate */
		kv = Qk[0] = K[0];
		for(i=1;i < N;i++)
		{
			Qk[i] = kv = kv*s + K[i];
		}
		if (fabs(kv) <= fabs(K[N-1])*10.0*Eta)
		{
			K[0] = 0.0;
			for(i=1;i < N;i++)
			{
				K[i] = Qk[i-1];
			}
		}
		else
		{
			/*  use the scaled form of the recurrence if the value */
			/*  of K at s is nonzero */
			t = -pv/kv;
			K[0] = Qp[0];
			for(i=1;i < N;i++)
			{
				K[i] = t*Qk[i-1] + Qp[i];
			}
		}
		kv = K[0];
		for(i=1;i < N;i++)
		{
			kv = kv*s + K[i];
		}
		t = (fabs(kv) > fabs(K[N-1])*10.0*Eta) ? -pv/kv : 0.0;
		s += t;
	} /* while(1) */
	*nz = 1;
	Szr = s;
	Szi = 0.0;
} /*realit()*/

/* ====> newest.c <==== */

/*
	Function translated from TOMS Algorithm 493, with modifications,
	by C. Bingham

	Compute new estimates of the quadratic coefficients using the scalars
	computed in calcsc.
*/

static void newest(long type, double *uu, double *vv)
{
	double a4, a5, b1, b2, c1, c2, c3, c4, temp;

	/*  use formulas appropriate to setting of type. */
	if(type == UNSCALED)
	{
		*uu = *vv = 0.0;
	} /*if(type == UNSCALED)*/
	else
	{
		if (type == SCALEBYD)
		{
			a4 = (A + G)*F + H;
			a5 = (F + U)*C + V*D;
		}
		else
		{
			a4 = A + U*B + H*F;
			a5 = C + (U + V*F)*D;
		}
		b1 = -K[N-1]/P[Nn-1];
		b2 = -(K[N-2] + b1*P[N-1])/P[Nn-1];
		c1 = V*b2*A1;
		c2 = b1*A7;
		c3 = b1*b1*A3;
		c4 = c1 - c2 - c3;
		temp = a5 + b1*a4 - c4;
		if (temp != 0.0)
		{
			*uu = U - (U*(c3 + c2) + V*(b1*A1 + b2*A7))/temp;
			*vv = V*(1.0 + c4/temp);
		}
		else
		{
			*uu = *vv = 0.0;
		}
	} /*if(type == UNSCALED){...}else{...}*/
} /*newest()*/



/* ====> nextk.c <==== */

/*
	Function translated from TOMS Algorithm 493, with modifications,
	by C. Bingham

	Computes the next K polynomials using scalars computed in calcsc
*/

static void nextk(long type)
{
	double        temp;
	long          i;

	if (type == UNSCALED)
	{
		/*  use unscaled form of the recurrence if type is UNSCALED */
		K[0] = K[1] = 0.0;
		for(i=2;i < N;i++)
		{
			K[i] = Qk[i-2];
		}
	}
	else
	{
		temp = (type == SCALEBYC) ? B : A;
		if (fabs(A1) <= fabs(temp)*Eta*10.0)
		{
			/* if A1 is nearly zero then use a special form of the recurrence*/
			K[0] = 0.0;
			K[1] = -A7*Qp[0];
			for(i=2;i < N;i++)
			{
				K[i] = A3*Qk[i-2] - A7*Qp[i-1];
			}
		}
		else
		{
			/* use scaled form of the recurrence */
			A7 /= A1;
			A3 /= A1;
			K[0] = Qp[0];
			K[1] = Qp[1] - A7*Qp[0];
			for(i=2;i < N;i++)
			{
				K[i] = A3*Qk[i-2] - A7*Qp[i-1] + Qp[i];
			}
		}
	}
} /*nextk()*/

/* ====> calcsc.c <==== */

/*
	Function translated from TOMS algorithm 493, with modifications,
	by C. Bingham
	This routine calculates scalar quantities used to compute the next K
	polynomial and new estimates of the quadratic coefficients.
	type - integer variable set here indicating how the calculations are
	normalized to avoid overflow
*/

static void calcsc(long *type)
{
	/*  synthetic division of K by the quadratic 1,U,V */
	quadsd(N, U, V, K, Qk, &C, &D);
	if (fabs(C) <= fabs(K[N-1])*100.0*Eta && fabs(D) <= fabs(K[N-2])*100.0*Eta)
	{
		*type = UNSCALED;
	}
	else if (fabs(D) < fabs(C))
	{
		*type = SCALEBYC;
		/*  type= SCALEBYC indicates that all formulas are divided by C */
		E = A/C;
		F = D/C;
		G = U*E;
		H = V*B;
		A3 = A*E + (H/C + G)*B;
		A1 = B - A*(D/C);
		A7 = A + G*D + H*F;
	}
	else
	{
		*type = SCALEBYD;
		/*  type= SCALEBYD indicates that all formulas are divided by D */
		E = A/D;
		F = C/D;
		G = U*B;
		H = V*B;
		A3 = (A + G)*E + H*(B/D);
		A1 = B*F - A;
		A7 = (F + U)*A + H;
	}
} /*calcsc()*/

/* ====> quadit.c <==== */

/*
	Function translated from TOMS Algorithm 493, with modifications,
	by C. Bingham

	Variable-shift K-polynomial iteration for a quadratic factor converges
	only if the zeros are equimodular or nearly so.
	uu,vv - coefficients of starting quadratic
	nz - number of zero found
*/

static void quadit(double uu, double vv, long *nz)
{
	double            ui, vi;
	/* 	real ms, mp, omp, ee, relstp, t, zm */
	double            mp, omp = 0.0, ee, relstp = 0.0, t, zm;
	long              type, i, j;
	long              tried;

	*nz = 0;
	tried = 0;
	U = uu;
	V = vv;
	j = 0;
	/* main loop */
	while(1)
	{
		quad(1.0, U, V, &Szr, &Szi, &Lzr, &Lzi);
		/*
			return if roots of the quadratic are real and not
			close to multiple or nearly equal and  of opposite
			sign
		*/
		if (fabs(fabs(Szr) - fabs(Lzr)) > 0.01*fabs(Lzr))
		{
			break;
		}

		/*  evaluate polynomial by quadratic synthetic division */
		quadsd(Nn, U, V, P, Qp, &A, &B);
		mp = fabs(A - Szr*B) + fabs(Szi*B);
		/*
			compute a rigorous  bound on the rounding error in
			evaluting p
		*/
		zm = sqrt(fabs(V));
		ee = 2.0*fabs(Qp[0]);
		t = -Szr*B;
		for(i=1;i < N;i++)
		{
			ee = ee*zm + fabs(Qp[i]);
		}

		ee = ee*zm + fabs(A + t);
		ee = (5.0*Mre + 4.0*Are)*ee -
			(5.0*Mre + 2.0*Are)*(fabs(A + t) + fabs(B)*zm) + 2.0*Are*fabs(t);
		/*
			iteration has converged sufficiently if the
			polynomial value is less than Criter times this bound
			original had Criter = 20 hardwired in
		*/
		if (mp <= Criter*ee)
		{
			*nz = 2;
			break;
		}
		j++;
		/*  stop iteration after 20 steps */
		if (j > 20)
		{
			break;
		}
		if (j >= 2 && relstp <= 0.01 && mp >= omp && !tried)
		{
			/*
			  a cluster appears to be stalling the convergence.
			  five fixed shift steps are taken with a U,V close
			  to the cluster
			  */
			if (relstp < Eta)
			{
				relstp = Eta;
			}
			relstp = sqrt(relstp);
			U -= U*relstp;
			V += V*relstp;
			quadsd(Nn, U, V, P, Qp, &A, &B);
			for(i=0;i < 5;i++)
			{
				calcsc(&type);
				nextk(type);
			}
			tried = 1;
			j = 0;
		} /*if (j >= 2 && relstp <= 0.01 && mp >= omp && !tried)*/

		omp = mp;
		/*  calculate next K polynomial and new U and V */
		calcsc(&type);
		nextk(type);
		calcsc(&type);
		newest(type, &ui, &vi);
		/*  if vi is zero the iteration is not converging */
		if (vi == 0.0)
		{
			break;
		}
		relstp = fabs((vi-V)/vi);
		U = ui;
		V = vi;
	} /*while(1)*/
} /*quadit()*/

/* ====> fxshfr.c <==== */

/*
	Function translated from TOMS Algorithm 493, with modifications,
	by C. Bingham

	Computes up to l2 fixed shift K-polynomials, testing for convergence in
	the linear or quadratic case. Initiates one of the variable shift
	iterations and returns with the number of zeros found.

	l2 - limit of fixed shift steps
	nz - number of zeros found
*/

static void fxshfr(long l2,long *nz)
{
	double             svu, svv, ui, vi, s;
	double             betas, betav, oss, ovv, ss, vv;
	double             ts, tv, ots = 0.0, otv = 0.0, tvv, tss;
	long               type, i, j, iflag;
	long               vpass, spass, vtry, stry;
	long               skip = 0, breakout = 0;
	
	*nz = 0;
	betas = betav = 0.25;
	oss = Sr;
	ovv = V;

	/*  evaluate polynomial by synthetic division */
	quadsd(Nn, U, V, P, Qp, &A, &B);
	calcsc(&type);

	for(j=0;j < l2;j++)
	{
		/*  calculate next K polynomial and estimate V */
		nextk(type);
		calcsc(&type);
		newest(type, &ui, &vi);
		vv = vi;

		/*  estimate s */

		ss = (K[N-1] != 0.0) ? -P[Nn-1]/K[N-1] : 0.0;

		tv = ts = 1.0;
		if (j != 0 && type != UNSCALED)
		{
			/*  compute relative measures of convergence of s and V */
			/*  sequences */
			if (vv != 0.0)
			{
				tv = fabs((vv-ovv)/vv);
			}
			if (ss != 0.0)
			{
				ts = fabs((ss-oss)/ss);
			}
			/*  if decreasing, multiply two most recent */
			/*  convergence measures */
			tvv = (tv < otv) ? tv*otv : 1.0;
			tss = (ts < ots) ? ts*ots : 1.0;

			/*  compare with convergence criteria */
			vpass = (tvv < betav);
			spass = (tss < betas);
			if (spass || vpass)
			{
				/*  at least one sequence has passed the convergence */
				/*  test. Store variables before iterating */
				svu = U;
				svv = V;
				for(i=0;i < N;i++)
				{
					Svk[i] = K[i];
				}
				s = ss;
				/*  choose iteration according to the fastest */
				/*  converging sequence */
				stry = vtry = 0;
				skip = (!spass || vpass && tss >= tvv);
						
				while(1)
				{
					if(!skip)
					{
						realit(&s, nz, &iflag);
						if (*nz > 0)
						{
							return;
						}
						/*  linear iteration has failed. Flag that it has been */
						/*  tried and decrease the convergence criterion */
						stry = 1;
						betas *= 0.25;
						if (iflag != 0)
						{
							/*  if linear iteration signals an almost double real */
							/*  zero attempt quadratic interation */
							ui = -(s+s);
							vi = s*s;
							skip = 1;
						}
					} /*if(!skip)*/
					
					do
					{
						if(!skip)
						{
							U = svu;
							V = svv;
							for(i=0;i < N;i++)
							{
								K[i] = Svk[i];
							}
				/*  try quadratic iteration if it has not been tried */
				/*  and the V sequence is converging */
							breakout = (!vpass || vtry);
							if(breakout)
							{
								break;
							}
						} /*if(!skip)*/
						else
						{
							skip = 0;
						}
						quadit(ui,vi,nz);
						if (*nz > 0)
						{
							return;
						}
						/*  Quadratic iteration has failed. Flag that it has */
						/*  been tried and decrease the convergence criterion*/
						vtry = 1;
						betav *= 0.25;
					}while(stry || !spass);
					if(breakout)
					{
						breakout = 0;
						break;
					}
					
					for(i=0;i < N;i++)
					{
						K[i] = Svk[i];
					}
				} /*while(1) */

				/*
				  recompute Qp and scalar values to continue the second stage
				  */
				quadsd(Nn, U, V, P, Qp, &A, &B);
				calcsc(&type);
			} /*if (spass || vpass) */
		} /*if (j != 0 && type != UNSCALED) */
		ovv = vv;
		oss = ss;
		otv = tv;
		ots = ts;
	} /*for(j=0;j < l2;j++)*/
} /*fxshfr()*/


/* ====> rpoly.c <==== */

/*

	TOMS Algorithm 493
	Translated to C, with minor modifications, by C. Bingham

	Finds the zeros of a real polynomial
	op  - double precision vector of coefficients in
	      order of decreasing powers.
	degree   - integer degree of polynomial.
	zeror, zeroi - output double precision vectors of
	               real and imaginary parts of the
	               zeros.
	fail  - output logical parameter, true only if
	        leading coefficient is zero or if rpoly
	        has found fewer than degree zeros.
	        in the latter case degree is reset to
	        the number of zeros found.

	To change the size of polynomials which can be
	solved, reset the dimensions of the arrays in the
	common area and in the following declarations.
	The subroutine uses single precision calculations
	for scaling, bounds and error calculations. All
	calculations for the iterations are done in double
	precision.

	C. Bingham made the following modifications to Fortran version  12/26/91
	All real storage and computations changed to double precision
	All declarations of /global/ put in file 'rpoly.inc' (rpoly.h in C)
	In this version, contents of rpoly.h have been explicitly included
	Value used convergence criterion used in quadit and realit
	made variable Criter in /global/ instead of hard wired value of 20
	and added Criter to argument list for rpoly
	Replaced all references to dabs and dsqrt by abs and sqrt

	To translate to C, struct was used to restructure and translate to
	Ratfor.  Then script ratfor2c was used to do most of the detail work of
	translating to C.  This was followed by hand editing.
*/

static void rpoly(double op[], long *degree, double *roots, 
		   long *fail, double *scratch)
{
	double           t, aa, bb, cc, factor;
	double           lo, xmax, xmin, xx, yy;
	double          *temp;
	double          *pt;
	double          *zeror,*zeroi;
	double           cosr,  sinr, xxx, x, sc, bnd, xm, ff, df, dx;
	long             cnt, nz, i, j, jj, nm1;
	long             l;
	long             zerok;
	WHERE("rpoly");

	/*
	  Are and Mre refer to the unit error in + and *
	  respectively. They are assumed to be the same as
	  ETA.
	  */
	Eta = ETA;
	Are = Eta;
	Mre = Eta;
	lo = SMALNO/Eta;

	/*  initialization of constants for shift rotation */
	xx = 0.70710678;				/* sqrt(2.0)*/
	yy = -xx;
	cosr = -0.069756474;			/* acos(94 degrees) */
	sinr = 0.99756405;			/* asin(94 degrees) */
	*fail = 0;
	N = *degree;
	Nn = N+1;

	zeror = roots;
	zeroi = roots + N;
	
/* scratch should have length at least 8 * (*degree) + 3 */	
	P = scratch;
	scratch += Nn;
	Qp = scratch;
	scratch += Nn;
	K = scratch;
	scratch += N;
	Qk = scratch;
	scratch += N;
	Svk = scratch;
	scratch += N;
	temp = scratch;
	scratch += N;
	pt = scratch;
	scratch += Nn;
	
	/*  algorithm fails if the leading coefficient is zero. */
	if (op[0] == 0.0)
	{
		*fail = 1;
		*degree = 0;
	} /*if (op[0] == 0.0)*/
	else
	{
		/* remove the zeros at the origin if any */
		j = *degree - N;
		while (op[Nn-1] == 0.0)
		{
			zeror[j] = zeroi[j] = 0.0;
			Nn--;
			N--;
			j++;
		}

		/* make a copy of the coefficients */
		for(i=0;i < Nn;i++)
		{
			P[i] = op[i];
		}

		/* start algorithm for one zero */
		while (N > 2)
		{
			/* find largest and smallest moduli of coefficients */
			xmax = 0.0;
			xmin = INFINY;
			for(i=0;i < Nn;i++)
			{
				x = fabs(P[i]);
				if (x > xmax)
				{
					xmax = x;
				}
				if (x != 0.0 && x < xmin)
				{
					xmin = x;
				}
			} /*for(i=0;i < Nn;i++)*/
			/*
			  scale if there are large or very small coefficients
			  computes a scale factor to multiply the
			  coefficients of the polynomial. The scaling is done
			  to avoid overflow and to avoid undetected underflow
			  interfering with the convergence criterion.
			  the factor is a power of the base
			  */

			sc = lo/xmin;
			
			if (sc > 1.0 && INFINY/sc >= xmax ||
				sc <= 1.0 && xmax >= 10.0)
			{
				if(sc == 0.0)
				{
					sc = SMALNO;
				}
			
				l = log(sc)/log(BASE) + 0.5;
				factor = pow(BASE, (double) l);
				if (factor != 1.0)
				{
					for(i=0;i < Nn;i++)
					{
						P[i] *= factor;
					}
				}
			}		

			/* compute lower bound on moduli of zeros */
			for(i=0;i < Nn;i++)
			{
				pt[i] = fabs(P[i]);
			}
			pt[Nn-1] = -pt[Nn-1];

			/*  compute upper estimate of bound */
			x = exp((log(-pt[Nn-1]) - log(pt[0]))/(double) N);

			if (pt[N-1] != 0.0)
			{
				/*  If Newton step at the origin is better, use it. */
				xm = -pt[Nn-1]/pt[N-1];
				if (xm < x)
				{
					x = xm;
				}
			} /*if (pt[N-1] != 0.0)*/

			/* chop the interval (0,x) until ff <= 0 */
			while(1)
			{
				xm = x*0.1;
				ff = pt[0];
				for(i=1;i < Nn;i++)
				{
					ff = ff*xm + pt[i];
				}
				if(ff <= 0.0)
				{
					break;
				}
				x = xm;
			} /*while (1)*/
			dx = x;
			/* do Newton iteration until x converges to two decimal places */
			while (fabs(dx/x) > 0.005)
			{
				df = ff = pt[0];
				for(i=1;i < N;i++)
				{
					ff = ff*x + pt[i];
					df = df*x + ff;
				}
				ff = ff*x + pt[Nn-1];
				dx = ff/df;
				x -= dx;
			} /*while (fabs(dx/x) > 0.005)*/

			bnd = x;

			/*
			  compute the derivative as the initial K polynomial
			  and do 5 steps with no shift
			  */
			nm1 = N-1;
			for(i=1;i < N;i++)
			{
				K[i] = (double) (Nn - i - 1)*P[i]/(double) N;
			} /*for(i=1;i < N;i++)*/
			K[0] = P[0];

			aa = P[Nn-1];
			bb = P[N-1];
			zerok = (K[N-1] == 0.0);
			for(jj=0;jj < 5;jj++)
			{
				cc = K[N-1];
				if (zerok)
				{
					/* use unscaled form of recurrence */
					j = Nn - 2;
					for(i=0;i < nm1;i++)
					{
						K[j] = K[j-1];
						j--;
					}
					K[0] = 0.0;
					zerok = (K[N-1] == 0.0);
				} /*if (zerok)*/
				else
				{
					/*
					  use scaled form of recurrence if value of K at 0 is
					  nonzero
					  */
					t = -aa/cc;
					j = Nn - 2;
					for(i=0;i < nm1;i++)
					{
						K[j] = t*K[j-1] + P[j];
						j--;
					}
					K[0] = P[0];
					zerok = (fabs(K[N-1]) <= fabs(bb)*ETA*10.0);
				} /*if (zerok){...}else{...}*/
			} /*for(jj=0;jj < 5;jj++)*/

			/*  save K for restarts with new shifts */
			for(i=0;i < N;i++)
			{
				temp[i] = K[i];
			} /*for(i=0;i < N;i++)	*/

			/*
			  loop to select the quadratic  corresponding to each
			  new shift
			  */
			for(cnt=1;cnt <= 20;cnt++)
			{
				/*
				  quadratic corresponds to a double shift to a
				  non-real point and its complex conjugate. the point
				  has modulus bnd and amplitude rotated by 94 degrees
				  from the previous shift
				  */
				xxx = cosr*xx - sinr*yy;
				yy = sinr*xx + cosr*yy;
				xx = xxx;
				Sr = bnd*xx;
				Si = bnd*yy;
				U = -2.0*Sr;
				V = bnd;
				/*  second stage calculation, fixed quadratic */
				fxshfr(20*cnt, &nz);
				if (nz != 0)
				{
					break;
				}
				/*
				  if the iteration is unsuccessful another quadratic
				  is chosen after restoring K
				  */
				for(i=0;i < N;i++)
				{
					K[i] = temp[i];
				}
			} /*for(cnt=1;cnt <= 20;cnt++)*/

			if(nz == 0)
			{
				/*  return with failure if no convergence with 20 shifts*/
				*fail = 20;
				*degree -= N;
				break;
			} /*if(nz == 0)*/

			/*
			  The second stage jumps directly to one of the third
			  stage iterations and returns here if successful.
			  Deflate the polynomial, store the zero or zeros and
			  return to the main algorithm.
			  */
			j = *degree - N;
			zeror[j] = Szr;
			zeroi[j] = Szi;
			Nn -= nz;
			N = Nn-1;
			for(i=0;i < Nn;i++)
			{
				P[i] = Qp[i];
			}
			if (nz != 1)
			{
				zeror[j+1] = Lzr;
				zeroi[j+1] = Lzi;
			}
			if (interrupted(DEFAULTTICKS) != INTNOTSET)
			{
				return;
			}
		} /*while (N > 2)*/
	} /*if (op[0] == 0.0){...}else{...}*/

	if (!(*fail) && N >= 1)
	{
		/*  calculate the final zero or pair of zeros */
		if (N == 2)
		{
			quad(P[0], P[1], P[2],
				 zeror + *degree - 2, zeroi + *degree - 2, 
				 zeror + *degree - 1, zeroi + *degree - 1);
		}
		else
		{
			zeror[*degree-1] = -P[1]/P[0];
			zeroi[*degree-1] = 0.0;
		}
	} /*if (!(*fail) && N >= 1)*/
} /* rpoly() */

/*
  980303 Added check for MISSING values in the argument
  990226 Replaced putOUTSTR() by putErrorOUTSTR()
*/
Symbolhandle polyroot(Symbolhandle list)
{
	Symbolhandle       arg = COMPVALUE(list,0), result = (Symbolhandle) 0;
	double           **scratchH = (double **) 0, *scratch;
	double            *coefs, *scrcoefs, *roots;
	long               nargs = NARGS(list);
	long               degree, ncols, fail = 0;
	long               dim[2], j, i;
	WHERE("polyroot");
	TRASH(1,errorExit);
	
	*OUTSTR = '\0';
	
	if (nargs != 1 || arg == (Symbolhandle) 0)
	{
		badNargs(FUNCNAME,1);
		goto errorExit;
	}
	if (!argOK(arg,REAL,0))
	{
		goto errorExit;
	}
	if (!isMatrix(arg,dim) || anyMissing(arg))
	{
		sprintf(OUTSTR,
				"ERROR: argument to %s() is not a REAL matrix with no MISSING elements",
				FUNCNAME);
	}
	else if (dim[0] > MAXLEN)
	{
		sprintf(OUTSTR,
				"ERROR: number of rows of argument to %s() greater than %ld",
				FUNCNAME, (long) MAXLEN);
	}
	
	if (*OUTSTR)
	{
		goto errorExit;
	}
	
	degree = dim[0];
	ncols = dim[1];
	result = RInstall(SCRATCH, 2*ncols*degree);
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	if (!getScratch(scratchH, 0, 9*degree + 4, double))
	{
		goto errorExit;
	}
	setNDIMS(result,2);
	setDIM(result,1,degree);
	setDIM(result,2,2*ncols);
	coefs = DATAPTR(arg);
	roots = DATAPTR(result);
	scrcoefs = scratch = *scratchH;
	scratch += degree + 1;

	for(j=0;j<ncols;j++)
	{
		scrcoefs[0] = 1.0;
		for(i=0;i<degree;i++)
		{
			scrcoefs[i+1] = -coefs[i];
			roots[i] = roots[i + degree] = 0.0;
		}
		rpoly(scrcoefs, &degree, roots, &fail, scratch);
		checkInterrupt(errorExit);
		if (fail)
		{
			sprintf(OUTSTR,
					"WARNING: %s did not fully converge on root %ld of column %ld",
					FUNCNAME,degree,j+1);
			putErrorOUTSTR();
		}
		coefs += degree;
		roots += 2*degree;
	} /*for(j=0;j<ncols;j++)*/

	emptyTrash();

	return (result);

  errorExit:
	
	putErrorOUTSTR();
	emptyTrash();
	Removesymbol(result);

	return (0);
} /*polyroot()*/
