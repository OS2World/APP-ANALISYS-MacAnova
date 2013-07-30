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
#pragma segment Pvalsub
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include <math.h>
#ifndef HUGEDBL
#define HUGEDBL 1e38
#endif


#define TWOVRPI 0.636619772367581343
#define HALF_PI 1.5707963268
#define EPSILON .000001

void studentbase(double * /*x*/, double * /*df*/, double * /*cdf*/);
double ppstudent(double /*pp*/, double /*n*/, long * /*ifault*/);
double betai(double /*x*/, double /*a*/, double /*b*/);
void normbase(double * /*x*/, double * /*phi*/);
double ppbeta(double /*p*/, double /*a*/, double /*b*/, long * /*ifault*/);
double ppnd(double /*p*/, long * /*ifault*/);

extern double   ppnd(), ppbeta();

/* CACM Algorithm 395, by G. W. Hill */

void studentbase(double * x, double * df, double * cdf)
{
	double          t, y, b, a, z, j, n;

	n = *df;
	z = 1.0;
	t = (*x) * (*x);
	y = t / n;
	b = 1.0 + y;

/* kb changed n > 20 in next line to n > 200 as in Alg 395 */
	if (n > floor(n) || (n >= 20 && t < n) || (n > 200))
	{
		if (n < 2.0 && n != 1.0)
		{
			/* beta integral aproximation for small df */
			double          da = 0.5, db = 0.5 * n, dx, dp;
			long            ia = 0, ib = floor(db);

			dx = db / (db + da * t);
			dp = betai(dx, db, da);
			*cdf = (*x >= 0) ? 1.0 - .5 * dp : .5 * dp;
		}
		else
		{
			/* asymptotic series for large or non-integer df */
			if (y > EPSILON)
			{
				y = log(b);
			}
			a = n - 0.5;
			b = 48.0 * a * a;
			y *= a;
			y = (((((-0.4 * y - 3.3) * y - 24.0) * y - 85.5) /
				  (0.8 * y * y + 100.0 + b) + y + 3.0) / b + 1.0) *
					  sqrt(y);

			y = -y;
			normbase(&y, cdf);
			if (*x > 0.0)
			{
				*cdf = 1.0 - *cdf;
			}
		}
	} /*if (n > floor(n) || (n >= 20 && t < n) || (n > 200))*/
	else
	{
		/* nested summation of cosine series */
		if (n < 20 && t < 4.0)
		{
			a = y = sqrt(y);
			if (n == 1.0)
			{
				a = 0.0;
			}
		}
		else
		{
			a = sqrt(b);
			y = a * n;
			for (j = 2; fabs(a - z) > EPSILON; j += 2.0)
			{
				z = a;
				y = (y * (j - 1)) / (b * j);
				a += y / (n + j);
			}
			n += 2.0;
			z = y = 0.0;
			a = -a;
		}
		for (n = n - 2.0; n > 1.0; n -= 2.0)
		{
			a = ((n - 1.0) / (b * n)) * a + y;
		}
		a = (fabs(n) < EPSILON) ? a / sqrt(b) : TWOVRPI * (atan(y) + a / b);
		*cdf = z - a;
		if (*x > 0.0)
		{
			*cdf = 1.0 - 0.5 * *cdf;
		}
		else
		{
			*cdf = 0.5 * *cdf;
		}
	} /*if (n > floor(n) || (n >= 20 && t < n) || (n > 200)){}else{}*/
}

/* CACM Algorithm 396, by G. W. Hill */

double          ppstudent(double pp, double n, long * ifault)
{
	double          sq, p, a, b, c, d, x, y;

	/* convert to double upper tailed probability */
	p = (pp < 0.5) ? 2.0 * pp : 2.0 * (1.0 - pp);

	if (n <= 3.0)
	{
		if (n == 1)
		{
			sq = tan(HALF_PI * (1.0 - p));
		}
		else if (n == 2.0)
		{
			sq = sqrt(2.0 / (p * (2.0 - p)) - 2.0);
		}
		else
		{
			sq = ppbeta(p, 0.5 * n, 0.5, ifault);
			if (sq != 0.0)
			{
				sq = sqrt((n / sq) - n);
			}
		}
	} /*if (n <= 3.0)*/
	else
	{
		a = 1.0 / (n - 0.5);
		b = 48.0 / (a * a);
		c = ((20700.0 * a / b - 98.0) * a - 16) * a + 96.36;
		d = ((94.5 / (b + c) - 3.0) / b + 1.0) * sqrt(a * HALF_PI) * n;
		x = d * p;
		y = pow(x, 2.0 / n);
		if (y > 0.05 + a)
		{
			/* asymptotic inverse expansion about normal */
			x = ppnd(0.5 * p, ifault);
			y = x * x;
			if (n < 5)
			{
				c += 0.3 * (n - 4.5) * (x + 0.6);
			}
			c += (((0.05 * d * x - 5.0) * x - 7.0) * x - 2.0) * x + b;
			y = (((((0.4 * y + 6.3) * y + 36.0) * y + 94.5) / c -
				  y - 3.0) / b + 1.0) * x;
			y = a * y * y;
			y = (y > .002) ? exp(y) - 1.0 : 0.5 * y * y + y;
		}
		else
		{
			y = ((1.0 / (((n + 6.0) / (n * y) - 0.089 * d - 0.822)
						* (n + 2.0) * 3.0) + 0.5 / (n + 4.0)) * y - 1.0)
				* (n + 1.0) / (n + 2.0) + 1.0 / y;
		}
		sq = sqrt(n * y);
	} /*if (n <= 3.0){}else{}*/

	/* decode based on tail */
	return ((pp < 0.5) ? -sq : sq);
}
