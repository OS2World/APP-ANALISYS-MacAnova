/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 4.05 or later
*(C)*
*(C)* Copyright (c) 1997 by Gary Oehlert and Christopher Bingham
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
#pragma segment Dunnett
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

#include "globals.h"

/*
   The following code is a translation to C by C. Bingham (kb@stat.umn.edu)
   of Fortran Code of C. W. Dunnett.

   This code was obtained from statlib and translated to structured ratfor
   using Unix program struct.  It was then transformed to an approximation to
   C using a sed script and cleaned up by hand, making subscripts start at 0,
   among other things

   Dunnett's code consists of MVNPRD (published as Applied Statistics
   Algorithm 251 [Appl.Statist. (1989), Vol.38, pp. 564 - 579]) and MVSTUD.
   These codes originally called AS algorithms 66 (alnorm()) and 241
   (ppnd7()), but now call MacAnova functions Cnor() and Qnor().
   It incorporates the correction by Dunnett in Appl. Statist. 42 (1993) 709.

   Dunction sdist() as published computed the density of chi-squared
   with integral degrees of freedom.  It was rewritten to use mylgamma()
   to compute log(gamma(df/2)) for arbitrary d.f. >= 1.  Changing the
   type of degrees of freedom from long to double elsewhere allowed removing
   the restriction that the degrees of freedom be integral.
*/

#include <math.h>

extern double          Cnor(double /*z*/);
extern double          Qnor(double /*p*/);
extern double          Qstu(double /*x*/, double /*df*/);
extern double          intpow(double /*x*/, double /*p*/);

#ifdef NOLGAMMA
#define lgamma mylgamma
extern double          mylgamma(double);
#else /*NOLGAMMA*/
#if !defined(MW_CW_New) || !TARGET_CPU_68K
extern double          lgamma(double);
#endif /*MW_CW_New || !TARGET_CPU_68K*/
#endif /*NOLGAMMA*/

#define CUMNOR(X)    Cnor(X) /*original was alnorm(X, &ifault)*/
#define INVNOR(P)    Qnor(P) /*original was ppnd7(P, &ifault)*/

#define MAX(a,b) ((a) >= (b) ? (a) : (b))

#define NGROUPMAX  50 /*defined identically in pvals.c*/

#define SQRT2     1.4142135623730950488 /*sqrt(2.0)*/
#define SQRTPI    1.7724538509055160272 /*sqrt(pi)*/
#define SQRT2PI   2.5066282746310005024 /*sqrt(2.0*pi*/

#define SMALLNO   1e-8
#define SMALLERNO 1e-13
/*
  Algorithm AS 251.3  Appl.Statist. (1989), Vol.38, No.3
  Compute derivatives of normal cdf's.
*/
static void assign(double u, double bp, double ff [])
{
	double       sq2pi = SQRT2PI;
	double       u2, t1, t2, t3;
	double       umax = 8.0, small = SMALLNO;
	long         i;

	if (fabs(u) > umax)
	{
		for (i = 0;i < 4 ;i++)
		{
			ff[i] = 0.0;
		}
	} /*if (fabs(u) > umax)*/
	else 
	{
		u2 = u*u;
		t1 = bp*exp(-0.5*u2)/sq2pi;
		t2 = bp*t1;
		t3 = bp*t2;
		ff[0] = t1;
		ff[1] = -u*t2;
		ff[2] = (u2 - 1.0)*t3;
		ff[3] = (3.0 - u2)*u*bp*t3;
		for (i = 0;i < 4 ;i++)
		{
			if (fabs(ff[i]) < small)
			{
				ff[i] = 0.0;
			} /*if (fabs(ff[i]) < small)*/
		} /*for (i = 0;i < 4 ;i++)*/
	} /*if (fabs(u) > umax){}else{}*/
} /*assign()*/



/*
  Algorithm AS 251.5  Appl.Statist. (1989), Vol.38, No.3
  Multiply ff(i) by f for i = n to 4.  set to zero if too small.
*/
static void toosml(long n, double ff [], double f)
{
	double          small = SMALLERNO;
	long            i;

	for (i = n;i < 4 ;i++)
	{
		ff[i] *= f;
		if (fabs(ff[i]) <= small)
		{
			ff[i] = 0.0;
		}
	} /*for (i = n;i < 4 ;i++)*/
} /*void toosml()*/

/*
  Algorithm AS 251.2  Appl.Statist. (1989), Vol.38, No.3
  Compute function in integrand and its 4th derivative.

*/
static void pfunc(double z, double a [], double b [], double bpd [], long n, 
				  int inf [], double addn, double * deriv, double * funcn, 
				  long * ntm, int ierc, double * result)
{
	double      fou[NGROUPMAX], fou1[NGROUPMAX][4];
	double      gou[NGROUPMAX], gou1[NGROUPMAX][4];
	double      ff[4], gf[4], term[4], germ[4], tmp[4];
	double      small = SMALLERNO;
	double      u, u1, u2, bi, hi, hli, bp, rslt1, rslt2;
	double      den, phi, phi1, phi2, phi3, phi4, frm, grm, zsq = z*z;
	double      sqrt2 = SQRT2, sqrtpi = SQRTPI;
	long        infi, i, j, k, m, l, ik;

	*deriv = 0.0;
	(*ntm)++;
	rslt1 = rslt2 = 1.0;
	bi = 1.0;
	hi = a[0] + 1.0;
	hli = b[0] + 1.0;
	infi = -1;
	for (i = 0;i < n ;i++)
	{
		if (i > 0 && bpd[i] == bi && a[i] == hi && b[i] == hli &&
			inf[i] == infi) 
		{
			fou[i] = fou[i - 1];
			gou[i] = gou[i - 1];
			for (ik = 0;ik < 4 ;ik++)
			{
				fou1[i][ik] = fou1[i-1][ik];
				gou1[i][ik] = gou1[i-1][ik];
			} /*for (ik = 0;ik < 4 ;ik++)*/ /*10*/
		} /*if (bpd[i] == bi && a[i] == hi && b[i] == hli && inf[i] == infi) */
		else 
		{
			bi = bpd[i];
			hi = a[i];
			hli = b[i];
			infi = inf[i];

			if (bi == 0.0) 
			{
				if (infi < 1)
				{
					fou[i] = 1.0 - CUMNOR(hli);
				}
				else if (infi == 1)
				{
					fou[i] = CUMNOR(hi);
				}
				else
				{
					fou[i] = CUMNOR(hi) - CUMNOR(hli);
				}
				gou[i] = fou[i];
				for (ik = 0;ik < 4 ;ik++)
				{
					fou1[i][ik] = gou1[i][ik] = 0.0;
				} /*for (ik = 0;ik < 4 ;ik++)*/ /*20*/
			} /*if (bi == 0.0) */
			else 
			{
				den = sqrt(1.0 - bi*bi);
				bp = bi*sqrt2/den;

				if (infi < 1) 
				{
					u = -hli/den + z*bp;
					fou[i] = CUMNOR(u);
					assign(u, bp, fou1[i]);
					bp = -bp;
					u = -hli/den + z*bp;
					gou[i] = CUMNOR(u);
					assign(u, bp, gou1[i]);
				}
				else if (infi == 1) 
				{
					u = hi/den + z*bp;
					gou[i] = CUMNOR(u);
					assign(u, bp, gou1[i]);
					bp = -bp;
					u = hi/den + z*bp;
					fou[i] = CUMNOR(u);
					assign(u, bp, fou1[i]);
				}
				else 
				{
					u2 = -hli/den + z*bp;
					assign(u2, bp, fou1[i]);
					bp = -bp;
					u1 = hi/den + z*bp;
					assign(u1, bp, tmp);
					fou[i] = CUMNOR(u1) + CUMNOR(u2) - 1.0;
					for (ik = 0;ik < 4 ;ik++)
					{
						fou1[i][ik] += tmp[ik];
					} /*for (ik = 0;ik < 4 ;ik++)*/ /*30*/

					if (-hli == hi) 
					{
						gou[i] = fou[i];
						for (ik = 0;ik < 4 ;ik++)
						{
							gou1[i][ik] = fou1[i][ik];
						} /*for (ik = 0;ik < 4 ;ik++)*/ /*40*/
					} /*if (-hli == hi) */
					else 
					{
						u2 = -hli/den + z*bp;
						assign(u2, bp, gou1[i]);
						bp = -bp;
						u1 = hi/den + z*bp;
						gou[i] = CUMNOR(u1) + CUMNOR(u2) - 1.0;
						assign(u1, bp, tmp);
						for (ik = 0;ik < 4 ;ik++)
						{
							gou1[i][ik] += tmp[ik];
						} /*for (ik = 0;ik < 4 ;ik++)*/ /*50*/
					}/*if (-hli == hi) {}else{}*/
				}
			} /*if (bi == 0.0){}else{}*/
		} /*if (bpd[i] == bi && a[i] == hi && b[i] == hli && inf[i] == infi){}else{}*/

		rslt1 *= fou[i];
		rslt2 *= gou[i];
		rslt1 = (rslt1 <= small) ? 0.0 : rslt1;
		rslt2 = (rslt2 <= small) ? 0.0 : rslt2;
	} /*for (i = 0;i < n ;i++)*/ /*60*/

	*funcn = rslt1 + rslt2 + addn;
	*result = (*funcn)*exp(-zsq)/sqrtpi;

	/*
	  If 4th derivative is not wanted, stop here.
	  Otherwise, proceed to compute 4th derivative.
	*/

	if (ierc != 0) 
	{
		for (ik = 0;ik < 4 ;ik++)
		{
			ff[ik] = gf[ik] = 0.0;
		} /*for (ik = 0;ik < 4 ;ik++)*/ /*70*/

		for (i = 0;i < n ;i++)
		{
			frm = grm = 1.0;
			for (j = 1;j < n ;j++)
			{
				frm *= fou[j];
				grm *= gou[j];
				frm = (frm <= small) ? 0.0 : frm;
				grm = (grm <= small) ? 0.0 : grm;
			} /*for (j = 1;j < n ;j++)*/ /*80*/
			
			for (ik = 0;ik < 4 ;ik++)
			{
				ff[ik] += frm*fou1[i][ik];
				gf[ik] += grm*gou1[i][ik];
			} /*for (ik = 0;ik < 4 ;ik++)*/ /*90*/
		} /*for (i = 0;i < n ;i++)*/ /*10*/

		if (n > 2)
		{
			for (i = 0;i < n - 1 ;i++)
			{
				for (j = i + 1;j < n ;j++)
				{
					term[1] = fou1[i][0]*fou1[j][0];
					germ[1] = gou1[i][0]*gou1[j][0];
					term[2] = fou1[i][1]*fou1[j][0];
					germ[2] = gou1[i][1]*gou1[j][0];
					term[3] = fou1[i][2]*fou1[j][0];
					germ[3] = gou1[i][2]*gou1[j][0];
					term[0] = fou1[i][1]*fou1[j][1];
					germ[0] = gou1[i][1]*gou1[j][1];
					for (k = 0;k < n ;k++)
					{
						if (k != i && k != j) 
						{
							toosml(0, term, fou[k]);
							toosml(0, germ, gou[k]);
						} /*if (k != i && k != j) */
					} /*for (k = 0;k < n ;k++)*/ /*110*/
					
					ff[1] += 2.0*term[1];
					ff[2] += 2.0*term[2]*3.0;
					ff[3] += 2.0*(term[3]*4.0 + term[0]*3.0);
					gf[1] += 2.0*germ[1];
					gf[2] += 2.0*germ[2]*3.0;
					gf[3] += 2.0*(germ[3]*4.0 + germ[0]*3.0);
				} /*for (j = i + 1;j < n ;j++)*/ /*120*/
			} /*for (i = 0;i < n ;i++)*/ /*130*/
			
			for (i = 0;i < n - 2 ;i++)
			{
				for (j = i + 1;j < n - 1 ;j++)
				{
					for (k = j + 1;k < n ;k++)
					{
						double      tmp;

						tmp = fou1[j][0]*fou1[k][0];
						term[2] = fou1[i][0]*tmp;
						term[3] = fou1[i][1]*tmp;

						tmp = gou1[j][0]*gou1[k][0];
						germ[2] = gou1[i][0]*tmp;
						germ[3] = gou1[i][1]*tmp;

						if (n > 3)
						{
							for (m = 0;m < n ;m++)
							{
								if (m != i && m != j && m != k) 
								{
									toosml(2, term, fou[m]);
									toosml(2, germ, gou[m]);
								} /*if (m != i && m != j && m != k) */
							} /*for (m = 0;m < n ;m++)*/ /*140*/
						} /*if (n > 3)*/
						
						ff[2] += 6.0*term[2];
						ff[3] += 36.0*term[3];
						gf[2] += 6.0*germ[2];
						gf[3] += 36.0*germ[3];
					} /*for (k = j + 1;k < n ;k++)*/
				} /*for (j = i + 1;j < n - 1 ;j++)*/ /*160*/
			} /*for (i = 0;i < n - 2 ;i++)*/ /*170*/
			
			if (n > 3)
			{
				for (i = 0;i < n - 3 ;i++)
				{
					for (j = i + 1;j < n - 2 ;j++)
					{
						for (k = j + 1;k < n - 1 ;k++)
						{
							for (m = k + 1;m < n ;m++)
							{
								term[3] = fou1[i][0]*fou1[j][0]*fou1[k][0]*fou1[m][0];
								germ[3] = gou1[i][0]*gou1[j][0]*gou1[k][0]*gou1[m][0];
								if (n > 4)
								{
									for (l = 0;l < n ;l++)
									{
										if (l != i && l != j && l != k && l != m) 
										{
											toosml(3, term, fou[l]);
											toosml(3, germ, gou[l]);
										}
									} /*for (l = 0;l < n ;l++)*/ /*180*/
								} /*if (n > 4)*/
								ff[3] += 24.0*term[3];
								gf[3] += 24.0*germ[3];
							} /*for (m = k + 1;m < n ;m++)*/ /*190*/
						} /*for (k = j + 1;k < n-1 ;k++)*/ /*200*/
					} /*for (j = i + 1;j < n-2 ;j++)*/ /*210*/
				} /*for (i = 0;i < n-3 ;i++)*/ /*220*/
			} /*if (n > 3)*/
		} /*if (n > 2) */

		phi = exp(-zsq)/sqrtpi; /*230*/
		phi1 = -2.0*z*phi;
		phi2 = (4.0*zsq - 2.0)*phi;
		phi3 = (-8.0*zsq*z + 12.0*z)*phi;
		phi4 = (16.0*zsq*(zsq - 3.0) + 12.0)*phi;
		*deriv = phi*(ff[3] + gf[3]) + 4.0*phi1*(ff[2] + gf[2]) +
		  6.0*phi2*(ff[1] + gf[1]) + 4.0*phi3*(ff[0] + gf[0]) + phi4*(*funcn);
	} /*if (ierc != 0) */
} /*pfunc()*/



/*
  Algorithm AS 251.4  Appl.Statist. (1989), Vol.38, No.3
  Largest absolute value of quadratic function fitted
  to three points.
*/

static void wmax(double w1, double w2, double w3, double  * dlg)
{
	double       quad, qlim, qmin = 1e-5, b2c;

	*dlg = MAX(fabs(w1), fabs(w3));
	quad = w1 - w2*2.0 + w3;
	qlim = MAX(fabs(w1 - w3)/2.0, qmin);
	if (fabs(quad) > qlim) 
	{
		b2c = (w1 - w3)/quad/2.0;
		if (fabs(b2c) < 1.0)
		{
			double     tmp = fabs(w2 - b2c*quad*b2c/2.0);
			
			if (tmp > *dlg)
			{
				*dlg = tmp;
			}
		} /*if (fabs(b2c) < 1.0)*/
	} /*if (fabs(quad) > qlim) */
} /*wmax()*/

/*
  Algorithm AS 251.1  Appl.Statist. (1989), Vol.38, No.3

  For a multivariate normal vector with correlation structure
  defined by rho(i, j) = bpd(i) * bpd(j), computes the probability
  that the vector falls in a rectangle in n - space with error
  less than eps.
*/

#define MAXLVL1    22
enum mvnprderrors
{                       /*orig value */
	MVNPRDBADNGROUPS = 500, /* 1 */
	MVNPRDBADBDP,           /* 2 */
	MVNPRDBADINF,           /* 3 */
	MVNPRDBADARGS,          /* 4 */
	MVNPRDNTMTOOBIG,        /* 5 */
	MVNPRDBADEPS,           /* 6 */
	MVNPRDLVLTOOBIG,        /* 7 */
	MVNPRDNONCONVERGED      /* 8 */
};

static void mvnprd(double a [], double b [], double bpd [], double eps, 
				   long n, int inf [], int ierc, double * hinc, double * prob, 
				   double * bound, int * ifault)
{
	double      fv[5], fd[5];
	double      estt[MAXLVL1];
	double      f1t[MAXLVL1], f2t[MAXLVL1], f3t[MAXLVL1];
	double      g1t[MAXLVL1], g3t[MAXLVL1], psum[MAXLVL1];
	double      h[NGROUPMAX], hl[NGROUPMAX], bb[NGROUPMAX];
	int         inft[NGROUPMAX], ldir[MAXLVL1];
	double      small = 1.0e-10, dxmin = 1e-7, sqrt2 = SQRT2;
	double      errl, bi, start, z, addn, eps2, eps1;
	double      zu, z2, z3, z4, z5, zz;
	double      erfac, el, el1, part0, part2, part3;
	double      func0, func2, funcn, wt, contrb;
	double      dlg, dx, da, estl, estr, sum, excess, error, prob1, safe;
	long        i, ntm, nmax, lvl, nr, ndim;
	int         bisect;
	
	*prob = 0.0;
	*bound = 0.0;

	/*         check for input values out of range. */
	if (n < 1 || n > NGROUPMAX)
	{
		*ifault = MVNPRDBADNGROUPS;
		goto errorExit;
	}
	
	for (i = 0;i < n ;i++)
	{
		bi = fabs(bpd[i]);
		if (bi >= 1.0)
		{
			*ifault = MVNPRDBADBDP;
			goto errorExit;
		}
		if (inf[i] < 0 || inf[i] > 2)
		{
			*ifault = MVNPRDBADINF;
			goto errorExit;
		}
		if (inf[i] == 2 && a[i] <= b[i])
		{
			*ifault = MVNPRDBADARGS;
			goto errorExit;
		}
	} /*for (i = 0;i < n ;i++)*/ /*10*/

	*ifault = 0;
	*prob = 1.0;

	/*         check whether any bpd(i) = 0. */

	ndim = 0;
	for (i = 0;i < n ;i++)
	{
		if (bpd[i] != 0.0) 
		{
			h[ndim] = a[i];
			hl[ndim] = b[i];
			bb[ndim] = bpd[i];
			inft[ndim] = inf[i];
			ndim++;
		} /*if (bpd[i] != 0.0) */
		else 
		{
			/*
			  if any bpd(i) = 0, the contribution to prob for that
			  variable is computed from a univariate normal.
			*/

			if (inf[i] < 1)
			{
				*prob *= 1.0 - CUMNOR(b[i]);
			}
			else if (inf[i] == 1)
			{
				*prob *= CUMNOR(a[i]);
			}
			else
			{
				*prob *= CUMNOR(a[i]) - CUMNOR(b[i]);
			}
			if (*prob <= small)
			{
				*prob = 0.0;
			}
		} /*if (bpd[i] != 0.0){}else{}*/
	} /*for (i = 0;i < n ;i++)*/ /*20*/
	
	if (ndim == 0 || *prob == 0.0)
	{
		return;
	}

/*
  If not all bpd(i) = 0, prob is computed by Simpson's rule.
  But first, initialize the variables.
*/
	z = 0.0;
	*hinc = (*hinc <= 0.0) ? 0.24 : *hinc;
	addn = -1.0;
	for (i = 0;i < ndim ;i++)
	{
		if (inft[i] == 2 ||
			inft[i] != inft[0] && bb[i]*bb[0] > 0.0 ||
			inft[i] == inft[0] && bb[i]*bb[0] < 0.0)
		{
			addn = 0.0;
			break;
		}
	} /*for (i = 0;i < ndim ;i++)*/ /*30*/

/*
  The value of addn is to be added to the product expressions in
  the integrand to insure that the limiting value is zero.
*/
	prob1 = 0.0;
	ntm = 0;
	nmax = 400;
	if (ierc == 0)
	{
		nmax *= 2;
	}
	pfunc(z, h, hl, bb, ndim, inft, addn, &safe, &func0, &ntm, ierc, &part0);
	eps2 = eps*0.1*0.5;

	/* set upper bound on z and apportion eps. */

	if (eps2 <= 0.0 || eps2 > 1.0)
	{
		*ifault = MVNPRDBADEPS;
		goto errorExit;
	}
	zu = -INVNOR(eps2)/sqrt2;

	nr = (long) (zu/(*hinc)) + 1;
	erfac = (ierc != 0) ? 2880.0/intpow(*hinc, 5.0) : 1.0;
	el = (eps - eps2)/(double) nr*erfac;
	el1 = el;

	do /* while (z < zu && ntm < nmax)*/
	{
		/* Start computations for the interval (z, z + hinc). */

		/*40*/
		error = 0.0;
		lvl = -1;
		fv[0] = part0;
		fd[0] = safe;
		start = z;
		da = *hinc;
		z3 = start + 0.5*da;
		pfunc(z3, h, hl, bb, ndim, inft, addn, &fd[2], &funcn, &ntm, 
			  ierc, &fv[2]);
		z5 = start + da;
		pfunc(z5, h, hl, bb, ndim, inft, addn, &fd[4], &func2, &ntm, 
			  ierc, &fv[4]);
		part2 = fv[4];
		safe = fd[4];
		wt = da/6.0;
		contrb = wt*(fv[0] + 4.0*fv[2] + fv[4]);
		dlg = 0.0;

		if (ierc != 0) 
		{
			wmax(fd[0], fd[2], fd[4], &dlg);
			if (dlg > el)
			{
				dx = da;
				bisect = 0;
			}
			else
			{
				bisect = -1;
			}
		} /*if (ierc != 0) */
		else
		{
			lvl = 0;
			ldir[lvl] = 2;
			psum[lvl] = 0.0;
			bisect = 1;
		} /*if (ierc != 0){}else{}*/
		
		if (bisect >= 0)
		{
			while (1)
			{
				if (bisect)
				{
					/*50*/
					/*
					  Bisect interval.  If ierc = 1, compute estimate on left
					  half;if ierc = 0, on both halves.
					  */
					dx = 0.5*da;
					wt = dx/6.0;
					z2 = start + 0.5*dx;
					pfunc(z2, h, hl, bb, ndim, inft, addn, &fd[1], &funcn, 
						  &ntm, ierc, &fv[1]);
					estl = wt*(fv[0] + 4.0*fv[1] + fv[2]);

					if (ierc == 0) 
					{
						z4 = start + 1.5*dx;
						pfunc(z4, h, hl, bb, ndim, inft, addn, &fd[3], &funcn, 
							  &ntm, ierc, &fv[3]);
						estr = wt*(fv[2] + 4.0*fv[3] + fv[4]);
						sum = estl + estr;
						dlg = fabs(contrb - sum);
						eps1 = el/intpow(2.0, (double) lvl);
						errl = dlg;
					} /*if (ierc == 0) */
					else
					{
						fv[2] = fv[1];
						fd[2] = fd[1];
						wmax(fd[0], fd[2], fd[4], &dlg);
						errl = dlg/intpow(2.0, 5.0*(lvl + 1));
						sum = estl;
						eps1 = el*intpow(intpow(2.0, (double) (lvl + 1)), 4.0);
					} /*if (ierc == 0) {}else{}*/
				} /*if (bisect)*/
				
				/*
				  Stop subdividing interval when accuracy is sufficient, 
				  or if interval too narrow or subdivided too often.
				  */

				if (!bisect || dlg > eps1 && dlg >= small) 
				{
					if (*ifault == 0 && ntm >= nmax)
					{
						*ifault = MVNPRDNTMTOOBIG;
					}
					if (fabs(dx) <= dxmin || lvl >= MAXLVL1 - 1)
					{
						*ifault = MVNPRDLVLTOOBIG;
					}
					if (*ifault == 0 || !bisect) 
					{
						/*
						  Raise level.  store information for right half and
						  apply Simpson's rule to left half.
						  */
						/*60*/
						lvl++;
						ldir[lvl] = 1;
						f1t[lvl] = fv[2];
						f3t[lvl] = fv[4];
						da = dx;
						fv[4] = fv[2];
						if (ierc == 0) 
						{
							f2t[lvl] = fv[3];
							estt[lvl] = estr;
							contrb = estl;
							fv[2] = fv[1];
						}
						else 
						{
							g1t[lvl] = fd[2];
							g3t[lvl] = fd[4];
							fd[4] = fd[2];
						}
						bisect = 1;
						continue;
					} /*if (*ifault == 0 || !bisect) */
				} /*if (!bisect || dlg > eps1 && dlg >= small) */
				
				bisect = 1;
				/*
				  Accept approximate value for interval.
				  Restore saved information to process right half interval.
				  */

				/*70*/
				error += errl;
				while (lvl >= 0 && ldir[lvl] != 1)
				{
					/*80*/
					sum += psum[lvl];
					lvl--;
				}

				if (lvl < 0)
				{
					contrb = sum;
					lvl = 0;
					dlg = error;
					break;
				}

				psum[lvl] = sum;
				ldir[lvl] = 2;
				if (ierc == 0)
				{
					dx *= 2.0;
				}
				start += dx;

				da = *hinc/intpow(2.0, (double) lvl);
				fv[0] = f1t[lvl];

				if (ierc == 0) 
				{
					fv[2] = f2t[lvl];
					contrb = estt[lvl];
				}
				else 
				{
					fv[2] = f3t[lvl];
					fd[0] = g1t[lvl];
					fd[4] = g3t[lvl];
				}
				fv[4] = f3t[lvl];
			} /*while (1)*/
		} /*if (bisect >= 0)*/

		/*90*/
		prob1 += contrb;
		*bound += dlg;
		excess = el - dlg;
		el = el1;
		if (excess > 0.0)
		{
			el = el1 + excess;
		}

		if (func0 > 0.0 && func2 <= func0 ||
			func0 < 0.0 && func2 >= func0) 
		{
			zz = -sqrt2*z5;
			part3 = fabs(func2)*CUMNOR(zz) + *bound/erfac;
			if (part3 <= eps || ntm >= nmax || z5 >= zu)
			{
				break;
			}
		}
		z = z5;
		part0 = part2;
		func0 = func2;
	} while (z < zu && ntm < nmax);

	/*100*/
	*prob *= (prob1 - addn*0.5);
	*bound = part3;

	if (*ifault == 0)
	{
		if (ntm >= nmax)
		{
			*ifault = MVNPRDNTMTOOBIG;
		}
		else if (*bound > eps)
		{
			*ifault = MVNPRDNONCONVERGED;
		}
	} /*if (*ifault == 0)*/

	/*fall through*/
  errorExit:
	;
} /*mvnprd()*/

/*
  Compute y**(n/2 - 1) exp(-y) / gamma(n/2)
  (revised: 1994 - 01 - 19)
*/
#define MINTEST   -23.0
#if (1)
/*
  If lgamma() is not available in the standard math library, define
  NOLGAMMA and supply a version called mylgamma.  In MacAnova it is
  based on equations 6.1.36 and 6.1.41 of Abramowitz and Stegun.
*/

/*use MacAnova lgamma and accept double degrees of freedom*/
static double sdist(double y, double df)
{
	static double          lastdf = -10.0;
	static double          lastlgamma;
	
	if (lastdf != df)
	{
		lastlgamma = lgamma(.5*df);
		lastdf = df;
	}
	return (exp((.5*df - 1)*log(y) - y - lastlgamma));  
} /*sdist()*/
#else /*1*/
/* translation of original version of sdist, restricted to integer df*/
static double sdist(double y, long n)
{
	double        sqrtpi = SQRTPI;
	double        value;
	double        xn, test;
	long          j, jj, jk, jkp;

	if (y > 0.0)
	{
		jj = n/2 - 1;
		jk = 2*jj - n + 2;
		jkp = jj - jk;
		value = 1.0;
		if (jk < 0)
		{
			value /= sqrt(y)*sqrtpi;
		}
		if (jkp == 0)
		{
			value *= exp(-y);
		} /*if (jkp == 0)*/
		else
		{
			xn	 = (double) n*0.5;
			test = log(y) - y/(double) jkp;
			if (test < MINTEST)
			{
				value = 0.0;
			} /*if (test < MINTEST)*/
			else
			{
				value = log(value);
				for (j = 0;j < jkp ;j++)
				{
					xn -= 1.0;
					value += test - log(xn);
				} /*for (j = 0;j < jkp ;j++)*/
				value = (value >= MINTEST) ? exp(value) : 0.0;
			} /*if (test < MINTEST){}else{}*/
		} /*if (jkp == 0){}else{}*/
	} /*if (y > 0.0) */
	else
	{
		value = 0.0;
	}
	return (value);
} /*sdist()*/
#endif /*1*/
/*
    971016 changed ndf from long to double (KB)
*/

static void fun(double z, double ndf,
				double h [], double hl [], double bpd [], 
				double erb2, long n, int inf [], double d [], 
				double * f0, double * g0, long ierc, double * hnc, int * ier)
{
	double          a[NGROUPMAX], b[NGROUPMAX];
	double          small = SMALLNO;
	double          arg, bnd, prob, term;
	long            i;
	int             iflt = 0;
	
	*f0 = *g0 = 0.0;
	if (z > -1.0 && z < 1.0)
	{
		arg = (1.0 + z)/(1.0 - z);
		term = arg*ndf*2.0/((1.0 - z)*(1.0 - z))*
		  sdist(ndf/2.0*arg*arg, ndf);
		if (term > small)
		{
			for (i = 0;i < n ;i++)
			{
				a[i] = arg*h[i] - d[i];
				b[i] = arg*hl[i] - d[i];
			} /*for (i = 0;i < n ;i++)*/
			mvnprd(a, b, bpd, erb2, n, inf, ierc, hnc, &prob, &bnd, &iflt);
			*ier = (*ier == 0) ? iflt : *ier;
			*g0 = term*bnd;
			*f0 = term*prob;
		} /*if (term > small)*/
	} /*if (z > -1.0 && z < 1.0)*/
} /*fun()*/

/* Studentizes a multivariate integral using Simpson's rule. */

#define MAXLVL2  30
/*971016 changed long ndf to double ndf (KB)*/
static void simpsn(double ndf, double a [], double b [], double bpd [], 
				   double errb, long n, int inf [], double d [], int ierc, 
				   double * hnc, double * prob, double * bnd, int * iflt)
{
	double       fv[5], f1t[MAXLVL2], f2t[MAXLVL2], f3t[MAXLVL2];
	double       ldir[MAXLVL2], psum[MAXLVL2];
	double       estt[MAXLVL2], errr[MAXLVL2];
	double       gv[5], g1t[MAXLVL2], g2t[MAXLVL2], g3t[MAXLVL2], gsum[MAXLVL2];
	double       dxmin = 0.000004;
	double       bounda = 0.0, boundg = 0.0;
	double       start = -1.0, dax = 1.0;
	double       da, erb2, eps1, error, excess;
	double       contrb, contrg, dx;
	double       estl, estr, estgl, estgr, sum, sumg, dlg, errl;
	double       f0, g0, wt, z2, z3, z4;
	int          iflag = 0, ier = 0, lvl;
	
	*prob = 0.0;
	erb2 = errb*0.5;
	eps1 = erb2*0.5;

	fun(0.0, ndf, a, b, bpd, erb2, n, inf, d, &f0, &g0, ierc, hnc, &ier);

	while (1)
	{
		/*10*/
		fv[0] = gv[0] = 0.0;
		error = 0.0;
		da = dax;
		lvl = 0;
		z3 = start + 0.5*da;
		fun(z3, ndf, a, b, bpd, erb2, n, inf, d, &fv[2], &gv[2], 
			ierc, hnc, &ier);
		fv[4] = f0;
		gv[4] = g0;
		wt = fabs(da)/6.0;
		contrb = wt*(fv[0] + 4.0*fv[2] + fv[4]);
		contrg = wt*(gv[0] + 4.0*gv[2] + gv[4]);
		ldir[lvl] = 2;
		psum[lvl] = gsum[lvl] = 0.0;

		while (1)
		{
			/*20*/
			/* bisect interval;compute estimates for each half. */

			dx = 0.5*da;
			wt = fabs(dx)/6.0;
			z2 = start + 0.5*dx;
			fun(z2, ndf, a, b, bpd, erb2, n, inf, d, &fv[1], &gv[1], 
				ierc, hnc, &ier);
			z4 = start + 1.5*dx;
			fun(z4, ndf, a, b, bpd, erb2, n, inf, d, &fv[3], &gv[3], 
				ierc, hnc, &ier);
			estl = wt*(fv[0] + 4.0*fv[1] + fv[2]);
			estr = wt*(fv[2] + 4.0*fv[3] + fv[4]);
			estgl = wt*(gv[0] + 4.0*gv[1] + gv[2]);
			estgr = wt*(gv[2] + 4.0*gv[3] + gv[4]);
			sum = estl + estr;
			
			sumg = estgl + estgr;
			dlg = fabs(contrb - sum);
			errl = dlg;

			/*
			  Stop bisecting when accuracy sufficient, or if
			  interval too narrow or bisected too often.
			*/

			if (dlg > eps1)
			{
				if (fabs(dx) <= dxmin || lvl >= (MAXLVL2 - 1))
				{
					/* Accept approximate value for interval. */
					/*40*/
					iflag = 11;
				} /*if (fabs(dx) <= dxmin || lvl >= (MAXLVL2 - 1))*/
				else
				{
					/*
					  Raise level.  Store information for right half
					  and apply Simpson's rule to left half.
					*/

					lvl++;
					ldir[lvl] = 1;
					f1t[lvl] = fv[2];
					f2t[lvl] = fv[3];
					f3t[lvl] = fv[4];
					g1t[lvl] = gv[2];
					g2t[lvl] = gv[3];
					g3t[lvl] = gv[4];
					da = dx;
					fv[4] = fv[2];
					fv[2] = fv[1];
					gv[4] = gv[2];
					gv[2] = gv[1];
					estt[lvl] = estr;
					contrb = estl;
					contrg = estgl;
					eps1 *= 0.5;
					errr[lvl] = eps1;
					continue;
				} /*if (fabs(dx) <= dxmin || lvl >= (MAXLVL2 - 1)){}else{}*/
			} /*if (dlg > eps1)*/

			/*50*/
			error += errl;
			while (lvl >= 0 && ldir[lvl] != 1)
			{
				/*60*/
				sum += psum[lvl];
				sumg += gsum[lvl];
				lvl--;
			} /*while (ldir[lvl] != 1) */
			if (lvl < 0)
			{
				break;
			}

			/*         restore saved information to process right half. */

			/*70*/
			psum[lvl] = sum;
			gsum[lvl] = sumg;
			ldir[lvl] = 2;

			da = dax/intpow(2.0, (double) lvl);
			start += dx*2.0;

			fv[0] = f1t[lvl];
			fv[2] = f2t[lvl];
			fv[4] = f3t[lvl];
			gv[0] = g1t[lvl];
			gv[2] = g2t[lvl];
			gv[4] = g3t[lvl];

			contrb = estt[lvl];
			excess = eps1 - dlg;
			eps1 = errr[lvl];
			if (excess > 0.0)
			{
				eps1 += excess;
			}
		} /*while (1) */

		contrb = sum;
		contrg = sumg;
		lvl = 0;
		dlg = error;

		/*80*/
		*prob += contrb;
		boundg += contrg;
		bounda += dlg;
		if (z4 > 0.0)
		{
			break;
		}
		/*90*/
		eps1 = erb2*0.5;
		excess = eps1 - *bnd;
		if (excess > 0.0)
		{
			eps1 += excess;
		}
		start = 1.0;
		dax = -1.0;
	} /*while (1)*/
	
	if (*iflt == 0)
	{
		*iflt = ier;
	}
	if (*iflt == 0)
	{
		*iflt = iflag;
	}
	bounda += boundg;
	if (*bnd < bounda)
	{
		*bnd = bounda;
	}
} /*simpsn()*/


/*
  Compute multivariate student integral, 
  using mvnprd (Dunnett, Appl. Stat., 1989)
  if rho(i, j) = bpd(i)*bpd(j).

  If rho(i, j) has general structure, use
  mulnor (Schervish, Appl. Stat., 1984) and replace
  call mvnprd(a, b, bpd, eps, n, inf, ierc, hnc, prob, bnd, iflt)
  by call mulnor(a, b, sig, eps, n, inf, prob, bnd, iflt).

  Author: C.W. Dunnett, McMaster University

  Based on adaptive Simpson's rule algorithm
  described in Shampine & Allen: "Numerical
  Computing", (1974), page 240.

  Parameters are same as in Algorithm AS 251
  in Appl. Stat. (1989), Vol. 38: 564 - 579
  with the following additions:
       ndf   integer      input  degrees of freedom
       d     real array   input  non - centrality vector
  (put ndf = 0 for infinite d.f.)

  971016 changed long ndf to double ndf (KB)
*/

#define MAXDF   500

static void mvstud(double ndf, double a [], double b[], double bpd [],
				   double errb, long n, int inf [], double d [], int ierc,
				   double * hnc, double * prob, double * bnd, int * iflt)
{
	double     f[3], aa[NGROUPMAX], bb[NGROUPMAX];
	double     erb2;
	double     ax, bx, xx;
	long       i;
	double     maxdf, nf; /*was long (KB)*/

	for (i = 0;i < n ;i++)
	{
		aa[i] = a[i] - d[i];
		bb[i] = b[i] - d[i];
	}

	if (ndf <= 0)
	{
		mvnprd(aa, bb, bpd, errb, n, inf, ierc, hnc, prob, bnd, iflt);
	} /*if (ndf <= 0)*/
	else
	{
		*bnd = 0.0;
		*iflt = 0;
		maxdf = MAXDF;
		erb2 = errb;

		/*
		  check if d.f. exceed MAXdf;if yes, then prob
		  is computed by quadratic interpolation on 1./d.f.
		*/

		if (ndf <= maxdf)
		{
			simpsn(ndf, a, b, bpd, erb2, n, inf, d, ierc, hnc, prob, bnd, iflt);
		}
		else
		{
			mvnprd(aa, bb, bpd, erb2, n, inf, ierc, hnc, &f[0],
				   bnd, iflt);
			nf = maxdf/2;
			simpsn(nf, a, b, bpd, erb2, n, inf, d, ierc, hnc, &f[2], 
				   bnd, iflt);
			nf = nf*2;
			simpsn(nf, a, b, bpd, erb2, n, inf, d, ierc, hnc, &f[1], 
				   bnd, iflt);
			xx = (double) nf/(double) ndf;
			ax = f[2] - f[1]*2.0 + f[0];
			bx = f[1]*4.0 - f[2] - f[0]*3.0;
			*prob = f[0] + xx*(ax*xx + bx)*0.5;
		}
	} /*if (ndf <= 0){}else{}*/
} /*mvstud()*/


/*  end of Dunnett's code */


static int    PDevals;
#define PDUNNETTEPS   1e-5

double PDunnett(double x, double ngroup, double groupSizes [], double edf,
				long twosided, double epsilon)
{
	double      p, hnc, bnd;
	double      eps = (epsilon <= 0.0) ? PDUNNETTEPS : epsilon;
	double      a[NGROUPMAX], b[NGROUPMAX], bpd[NGROUPMAX], d[NGROUPMAX];
	long        i, ng;
	int         inf[NGROUPMAX], ierc = 0, iflt;
	WHERE("PDunnett");

	PDevals++; /*count evaluations (for tuning QDunnett())*/

	if (twosided && x <= 0.0)
	{
		p = 0.0;
	} /*if (twosided && x <= 0.0)*/
	else
	{
		if (ngroup == 2)
		{
			p = Cstu(x, edf);
			return ((twosided) ? 2.0*(p - 0.5) : p);
		}
		ng = ngroup - 1;
		hnc = 0.0;

		for (i = 0;i < ng;i++)
		{
			a[i] = x;
			if (twosided)
			{
				b[i] = -x;
				inf[i] = 2;
			}
			else
			{
				b[i] = 0.0;
				inf[i] = 1;
			}
			d[i] = 0.0;
			bpd[i] = 1.0/sqrt(1.0 + groupSizes[0]/groupSizes[i + 1]);
		} /*for (i = 0;i < ng;i++)*/

		mvstud(edf, a, b, bpd, eps, ng, inf, d, ierc, &hnc, &p, &bnd, &iflt);
	} /*if (twosided && x <= 0.0){}else{}*/
	
	return (p);
} /*PDunnett()*/


#define QDUNNETTEPS   1e-5
#define DECREASER     0.9

/*
   Use a secant-bracket method with the Illinois modification 
   upper starting value is Tukey modified Bonferroni value

   Code changed to use fsolve() in mathutil.c, working in the logit(p)
   scale
*/

static double    *GroupSizes; /*set by QDunnett()*/

#define Edf       param[0]
#define Ngroup    param[1]
#define Twosided  param[2]
#define Uselogit  param[3]
#define Uselogx   param[4]
#define Epsilon   param[5]

#define NPAR      6

#define Nxy       report[0]
#define Iter      report[1]
#define Ifault    report[2]

#define LOGIT(P)  logit(P)
/*
  define USEFSOLVE = 0 to use only code give here to find inverse
  define USEFSOLVE = 1 to use only fsolve() in mathutil.c to find inverse.
  define USEFSOLVE = 2 to allow either fsolve() or code given here,
  depending on whether (GUBED & 32) != 0

  Limited experimentation shows fsolve sometimes uses fewer function values,
  sometimes more, sometimes the same
*/
#define USELOGIT    1
#define USEFSOLVE   1

static double logit(double p)
{
	return (log(p/(1.0 - p)));
} /*logit()*/

#if (USEFSOLVE != 1)
static double invlogit(double x)
{
	return (1.0/(1.0 + exp(-x)));
} /*invlogit()*/
#endif /*USEFSOLVE != 1*/
enum pDunnettErrors
{
	DUNNETTINTERRUPT = 100,
	DUNNETTPTOOLOW,
	DUNNETTPTOOHIGH
};

static double pDunnett(double x, double param [], long nparam, long * ierr)
{
	double      p;
	WHERE("pDunnett");
	
	if (Uselogx)
	{
		x = exp(x);
	}
	
	p = PDunnett(x, Ngroup, GroupSizes, Edf, (long) Twosided, Epsilon);

	if (interrupted(DEFAULTTICKS) != INTNOTSET)
	{
		*ierr = DUNNETTINTERRUPT;
	}
	else if (p <= 0.0)
	{
		*ierr = DUNNETTPTOOLOW;
		p = 1e-6;
	}
	else if (p >= 1.0)
	{
		*ierr = DUNNETTPTOOHIGH;
		p = 1.0 - 1e-6;
	}
	
	return ((Uselogit) ? LOGIT(p) : p);
} /*pDunnett()*/

#define NTAB     7
static double solveDunnett(double p, double param [], double eps, long itmax,
						   long report[])
{
	double         edf = Edf;
	double         ngroup = Ngroup;
	long           twosided = Twosided;
	double        *groupSizes = GroupSizes;
	double         pp = (Uselogit) ? LOGIT(p) : p;
	double         value;
	double         factor = (twosided) ? 0.5 : 1.0;
	WHERE("solveDunnett");

#if (USEFSOLVE > 1)
	if (GUBED & 32)
#endif /*USEFSOLVE > 1*/
#if (USEFSOLVE)
	{
		
		double         xmin, xmax;
		long           ntab = NTAB;
		double         x[NTAB], y[NTAB], pr[NTAB];
	
		itmax = 50;

		xmin = Qstu(1.0 - factor*(1.0 - p), edf);
		xmax = Qstu(1.0 - factor*(1.0 - p)/(double) (ngroup-1), edf);
		x[0] = xmax -.05 * (xmax - xmin);
		x[1] = xmax;
		
		value = fsolve(pp, pDunnett, param, NPAR, xmax, xmin, eps, itmax,
					   ntab, x, y, pr, report);
	}
#endif /*USEFSOLVE*/
#if (USEFSOLVE > 1)
	else
#endif /*USEFSOLVE > 1*/
#if (USEFSOLVE != 1)
	{
		double         upperx, lowerx, upperxp, lowerxp, newx, newxp, lastxp;
		double         upperp, lowerp, tmp, newp;
		double         ux, lx, uxp, lxp;
		double         howfar;
		double         p1;
		double         posnegfuzz = 2.0*eps;
		int            useloglog = 0;
		long           iter = 0;

		upperx = Qstu(1.0 - factor*(1.0 - p)/(double) (ngroup-1), edf);
		upperp = pDunnett(upperx, param, NPAR, &Ifault);

		if (Ifault)
		{
			goto errorExit;
		}
		
		lowerp = 1e50;
		
		howfar = 2.0*(upperp - pp);

		while (1)
		{
			p1 = (Uselogit) ? invlogit(pp - howfar) : pp - howfar;
			tmp = 1.0 - factor*(1.0 - p1)/ (double) (ngroup-1);
			if (tmp > 0.0)
			{
				lowerx = Qstu(tmp, edf);
				lowerp = pDunnett(lowerx, param, NPAR, &Ifault);
				if (Ifault)
				{
					goto errorExit;
				}
				
				if (lowerp <= pp)
				{
					break;
				}
				upperx = lowerx;
				upperp = lowerp;
			}
			else
			{
				lowerx = Qstu(1.0 - factor*(1.0 - p), edf);
				lowerp = pDunnett(lowerx, param, NPAR, &Ifault);
				if (Ifault)
				{
					goto errorExit;
				}
				break;
			}
			howfar *= 2.0;
		} /*while(1)*/
	
		upperxp = upperp - pp;
		lowerxp = lowerp - pp;

		Uselogx = useloglog = (lowerx > 0);
		
		newx = upperx;
		lastxp = upperxp;

		while (fabs(upperx - lowerx) > eps)
		{
			if (fabs(upperxp - lowerxp) < posnegfuzz ||
				upperxp * lowerxp >= 0.0)
			{
				/* PDunnett has error bound of .00001, so don't expect
				   perfection here */
				goto normalExit;
			} /*if ( upperxp * lowerxp > 0)*/
			iter++;
			if (itmax > 0 && iter > itmax)
			{
				Ifault = NONCONVERGED;
				goto errorExit;
			}
		
			if (useloglog)
			{
				lx = log(lowerx);
				lxp = log((1 - pp)/(1 - lowerp));
				ux = log(upperx);
				uxp = log((1 - pp)/(1 - upperp));
			}
			else
			{
				lx = lowerx;
				lxp = lowerxp;
				ux = upperx;
				uxp = upperxp;
			}

			newx = (lx * uxp - ux * lxp)/(uxp - lxp);
			newp = pDunnett(newx, param, NPAR, &Ifault);
			if (Ifault)
			{
				goto errorExit;
			}
			newxp = newp - pp;
		
			if (newxp == 0.0)
			{
				upperx = lowerx = newx;
				break;
			}
		
			if (newxp * lowerxp < 0)
			{
				upperx = newx;
				upperxp = newxp;
				upperp = newp;
				if (newxp * lastxp > 0)
				{
					lxp /= 2.0;
					if (useloglog)
					{
						lowerxp = (1.0 - pp)*(1.0 - exp(-lxp));
					}
					else
					{
						lowerxp = lxp;
					}
					lowerp = pp + lowerxp;
				}
			} /*if (newxp * lowerxp < 0)*/
			else
			{
				lowerx = newx;
				lowerxp = newxp;
				lowerp = newp;
				if (newxp * lastxp > 0)
				{
					uxp /= 2.0;
					if (useloglog)
					{
						upperxp = (1.0 - pp)*(1.0 - exp(-uxp));
					}
					else
					{
						upperxp = uxp;
					}
					upperp = pp + upperxp;
				}
			} /*if (newxp * lowerxp < 0){}else{}*/
	
			lastxp = newxp;
		} /*while (fabs(upperx - lowerx) > eps)*/
		/* fall through */

	  normalExit:
		Ifault = 0;
		/* fall through*/
	  errorExit:
		Nxy = PDevals;
		Iter = iter;

		value = 0.5*(upperx + lowerx);
	}
#endif /*USEFSOLVE!=1*/

	return (value);
} /*solveDunnett()*/

double QDunnett(double p, double ngroup, double groupSizes [], double edf,
				long twosided, double epsilon, long * error)
{
	double         eps = (epsilon > 0.0) ? epsilon : QDUNNETTEPS;
	double         x;
	double         param[NPAR];
	long           report[3];
	long           itmax = 0;
	WHERE("QDunnett");

	Ifault = 0;

	if (ngroup > 2)
	{
		PDevals = 0;
		Edf = edf;
		Ngroup = ngroup;
		Twosided = (double) twosided;
		GroupSizes = groupSizes;
		Uselogit = USELOGIT;
		Uselogx = 0;
		Epsilon = PDUNNETTEPS;

		x = solveDunnett(p, param, eps, itmax, report);

		if (Ifault == MVNPRDNONCONVERGED || Ifault == FSOLVENOTCONVERGED)
		{
			Ifault = NONCONVERGED;
		}
	}
	else
	{ /*exact result for two groups */
		x = Qstu((twosided) ? (0.5 + 0.5*p) : p, edf);
	}
	
	*error = Ifault;
	return (x);

} /*QDunnett()*/

