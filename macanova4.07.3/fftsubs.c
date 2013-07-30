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
#pragma segment Fftsubs
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


/* Fourier transform functions slightly modified 920417 for use in MacAnova */

/* qft.c */
/*
   Adapted from Fortran Fast Fourier Transform routines written by
   Gordon Sande, circa 1968, University of Chicago

   Translation to C by C. Bingham, University of Minnesota, July 1991
*/


#include "globals.h"
#include "tserProt.h"
#include "tser.h"

/*
 * subroutine to carry out fourier transforms. iop = 1,2,3
 *     means rft,hft,cft.  When divByT is true the transform
 *     is divided by m.
 *   iop      Dims of a           Dims of b
 *    1	rft    m by n              m by n             IRFT
 *    2 hft    m by n              m by n             IHFT
 *    3 cft    m by n        m by (n even) ? n : n+1  ICFT
 *  error
 *    1 : largest factor too large (>19)              TOOBIGPRIME
 * 	  2 : number of factors too large (>19)           TOOMANYFACTORS
 *    3 : memory allocation problem                   OUTOFMEMORY
 */

#define GHX     0
#define GHSCR   1
#define NTRASH  2

/*
  990226 replaced putOUTSTR() by putErrorOUTSTR()
         brought function declarations up to C standard
*/
#define Trash   NAMEFORTRASH
static Symbolhandle Trash = (Symbolhandle) 0; /* repository for scratch */

long            qft(double * a, double * b, long m, long n, long divByT,
					long iop)
{
	long            i, j, nn, m1, m2, l;
	long            odd;
	long            error = 0;
	double         *aj, *bj, *bn;
	double          t;
	long            missingFound = 0;
	WHERE("qft");

	Trash = (Symbolhandle) 0;
	odd = n & 1;

	/* 
	   copy input to output; done even though they may be the same
	   because of the check for missing values
	*/
	aj = a;
	bj = b;
	
	for (j = 0; j < n; j++)
	{
		for (i = 0; i < m; i++)
		{
			t = aj[i];
			if(isMissing(t))
			{
				t = 0.0;
				missingFound = 1;
			}
			bj[i] = t;
		}
		aj += m;
		bj += m;
	}

	if(missingFound)
	{
		sprintf(OUTSTR,
				"WARNING: missing values in argument to %s set to zero",
				FUNCNAME);
		putErrorOUTSTR();
	}
	
	bj = b;
	if (iop != ICFT)
	{			/* rft and hft */
		for (j = 0; j < n; j++)
		{
			error = (iop == IRFT) ? realft(m, bj) : onehft(m, bj);
			if (error != 0)
			{
				goto errorExit;
			}
			bj += m;
		}
	} /*if (iop != ICFT)*/
	else
	{			/* cft */
		nn = n / 2;
		for (j = 0; j < nn; j++)
		{		/* do the fully complex columns */
			if ((error = onecft(m, bj, bj+m)))
			{
				goto errorExit;
			}
			bj += 2*m;
		} /*for (j = 0; j < nn; j++)*/
		if (odd)
		{		/* do the last, purely real series, if any */
			if ((error = realft(m, bj)))
			{
				goto errorExit;
			}
			/* convert hermitian series to fully complex */
			n++;
			bn = bj + m;
			m1 = m / 2;
			m2 = (m + 1) / 2;
			bn[0] = bn[m1] = 0.0;
			l = m;
			for (i = 1; i < m2; i++)
			{
				l--;
				bn[l] = -(bn[i] = bj[l]);
				bj[l] = bj[i];
			}
		} /*if (odd)*/
	} /*if (iop != ICFT){}else{}*/

	if (divByT)
	{
		t = (double) m;
		bj = b;
		for (j = 0; j < n; j++)
		{
			for (i = 0; i < m; i++)
			{
				bj[i] /= t;
			}
			bj += m;
		}
	}
/* fall through */
  errorExit:
	emptyTrash(); /* release scratch space, if any */
	Trash = (Symbolhandle) 0;

	return error;


} /*qft()*/

/* realft.c */
/*
   Adapted from Fortran Fast Fourier Transform routines written by
   Gordon Sande, circa 1968, University of Chicago

   Translation to C by C. Bingham, University of Minnesota, July 1991
*/


/*      real fourier transform */
long            realft(long pts, double x [])
{

	long            pmax = PMAX, pSym, twoGrp;
	long            factor[NEST + 1], sym[NEST + 1], unSym[NEST + 1];
	long            error = 0;
	long            sfrp();
	WHERE("realft");

	twoGrp = 8;

	if (pts > 1)
	{
		if (!(error = srfp(pts, &pmax, twoGrp, factor, sym, &pSym, unSym)))
		{
			siprp(pts, sym, pSym, unSym, x);
			rftkd(pts, factor, pmax, x,&error);
		}
	}
	return error;
} /*realft()*/


/* onecft.c */
/*
   Adapted from Fortran Fast Fourier Transform routines written by
   Gordon Sande, circa 1968, University of Chicago

   Translation to C by C. Bingham, University of Minnesota, July 1991
*/


/*      one complex fourier transform */
long            onecft(long n, double x [], double y [])
{
	double          xi, xr, yi, yr;
	long            j, k, nUpon2;
	long            error = 0;

	if (n > 1)
	{
		if ((error = realft(n, x)) || (error = realft(n, y)))
		{
			return error;
		}
		nUpon2 = (n - 1) / 2 + 1;
		for (j = 1, k = n-1; j < nUpon2; j++, k--)
		{
			xr = x[j];
			xi = x[k];
			yr = y[j];
			yi = -y[k];
			x[j] = xr + yi;
			x[k] = xr - yi;
			y[j] = yr + xi;
			y[k] = yr - xi;
		}
	}
	return 0;
} /*onecft()*/




/* onehft.c */
/*
   Adapted from Fortran Fast Fourier Transform routines written by
   Gordon Sande, circa 1968, University of Chicago

   Translation to C by C. Bingham, University of Minnesota, July 1991
*/


long            onehft(long n, double x [])
/*      one hermite fourier transform */
{

	double          xi, xr;
	long            j, k, nUpon2;
	long            error = 0;

	if (n > 1)
	{
		nUpon2 = (n - 1) / 2 + 1;
		if (nUpon2 >= 2)
		{
			for (j = 1, k = n-1; j < nUpon2; j++, k--)
			{
				xr = x[j];
				xi = x[k];
				x[j] = xr + xi;
				x[k] = xr - xi;
			}
		}
		if (!(error = realft(n, x)))
		{
			for (j = 1, k = n-1; j < nUpon2; j++, k--)
			{
				xr = x[j];
				xi = -x[k];
				x[j] = xr + xi;
				x[k] = xr - xi;
			}
		}
	} /*if (n > 1)*/
	return error;
} /*onehft()*/




/* siprp.c */
/*
   Adapted from Fortran Fast Fourier Transform routines written by
   Gordon Sande, circa 1968, University of Chicago

   Translation to C by C. Bingham, University of Minnesota, July 1991
*/


/*      single in place reordering programme */
void            siprp(long pts, long sym [], long pSym, long unSym [],
					  double x [])
{
	double          t;
	long            oneMod;
	long            modulo[NEST];
	long            nest = NEST;
	long            dk, i, il, j, jj, jl, k, kk, ks, lk, mods,
	                mult, pUnSym, test;
	WHERE("siprp");

	ssymrp(pts, sym, x);

	if (unSym[0] != 0)
	{
		pUnSym = pts / (pSym * pSym);
		mult = pUnSym / unSym[0];
		test = (unSym[0] * unSym[1] - 1) * mult * pSym;
		lk = mult;
		dk = mult;
		for (k = 1; k < nest && unSym[k] != 0; k++)
		{
			lk *= unSym[k - 1];
			dk /= unSym[k];
			modulo[k] = (lk - dk) * pSym;
		}
		mods = k;
		oneMod = (mods < 3);
		if (!oneMod)
		{
			k = (mods + 3) / 2;
			jj = mods; /* mods + 1 - j */
			for (j = 2; j < k; j++)
			{
				jj--;
				kk = modulo[j];
				modulo[j] = modulo[jj];
				modulo[jj] = kk;
			}
		}
		jl = (pUnSym - 3) * pSym;
		ks = pUnSym * pSym;

		for (j = pSym; j <= jl; j += pSym)
		{
			jj = j;
			do
			{
				jj *= mult;
				if (!oneMod)
				{
					for (i = 2; i < mods; i++)
					{
						jj %= modulo[i];
					}
				}
				if (jj < test)
				{
					jj %= modulo[1];
				}
				if (jj >= test)
				{
					jj = jj % modulo[1] + modulo[1];
				}
			} while (jj < j);

			if (jj != j)
			{
				lk = jj - j;
				il = j + pSym;
				for (i = j; i < il; i++)
				{
					kk = i + lk; /*k + lk*/
					for (k = i; k < pts; k += ks)
					{
						t = x[k];
						x[k] = x[kk];
						x[kk] = t;
						kk += ks;
					} /*for (k = i; k < pts; k += ks)*/
				} /*for (i = j; i < il; i++)*/
			} /*if (jj != j)*/
		} /*for (j = pSym; j <= jl; j += pSym)*/
	} /*if (unSym[0] != 0)*/
} /*siprp()*/




/* srfp.c */
/*
   Adapted from Fortran Fast Fourier Transform routines written by
   Gordon Sande, circa 1968, University of Chicago

   Translation to C by C. Bingham, University of Minnesota, July 1991
*/


/*      symmetrized reordering factoring programme */
long            srfp(long pts, long * pmax, long twoGrp, long factor [],
					 long sym [], long * pSym, long unSym [])
{
	long            pp[NEST / 2], qq[NEST];
	long            f = 2, j, n, p = 0, pTwo, q = 0, r;
	long            inc = 1;
	WHERE("srfp");

	n = pts;
	*pSym = 1;

	while (n > 1)
	{
		while (n % f)
		{
			f += inc;
		}
		if (f > 2)
		{
			inc = 2;
		}

		if (2 * p + q >= NEST)
		{
			return TOOMANYFACTORS;
		}

		if (f > *pmax)
		{
			*pmax = f;
			return TOOBIGPRIME;
		}

		n /= f;
		if (n % f != 0)
		{
			qq[q++] = f;
		}
		else
		{
			n /= f;
			pp[p++] = f;
			(*pSym) *= f;
		}
	}			/*while*/
	*pmax = f;

	r = (q > 0) ? 1 : 0;
	for (j = 0; j < p; j++)
	{
		factor[j] = sym[j] = pp[p - j - 1];
		factor[p + q + j] = sym[p + r + j] = pp[j];
	}

	for (j = 0; j < q; j++)
	{
		unSym[j] = factor[p + j] = qq[j];
	}

	if (q > 0)
	{
		sym[p] = pts / ((*pSym) * (*pSym));
	}

	factor[2 * p + q] = 0;
	pTwo = 1;
	j = 0;
	while (factor[j] != 0)
	{
		if (factor[j] == 2)
		{
			pTwo = pTwo * 2;
			factor[j] = 1;
			if (pTwo >= twoGrp || factor[j + 1] != 2)
			{
				factor[j] = pTwo;
				pTwo = 1;
			}
		}
		j++;
	} /*while (factor[j] != 0)*/
	if (p == 0)
	{
		r = 0;
	}
	sym[2 * p + r] = 0;
	if (q <= 1)
	{
		q = 0;
	}
	unSym[q] = 0;

	return NOERROR;
} /*srfp()*/

/* ssymrp.c */
/*
   Adapted from Fortran Fast Fourier Transform routines written by
   Gordon Sande, circa 1968, University of Chicago

   Translation to C by C. Bingham, University of Minnesota, July 1991
*/


/*      single symmetric reordering programme */
void            ssymrp(long pts, long sym [], double x [])
{

	double          t;
	long            j, jj, kk, level, loop, nest;

	long            i[NEST], k[NEST], l[NEST];

	nest = NEST;
	loop = NEST / 2;

	if (sym[0] != 0)
	{
		for (j = 0; j < nest; j++)
		{
			l[j] = 1;
			i[j] = 1;
		}
		kk = pts;
		for (j = 0; j < nest; j++)
		{
			if (sym[j] == 0)
			{
				break;
			}
			l[j] = kk;
			i[j] = kk / sym[j];
			kk /= sym[j];
		}

		kk = 0;
		level = nest;
		k[level - 1] = 0;
		do
		{
			level--;
			jj = level; /* level + loop - j - 1*/
			for (j = loop; j <= level; j++)
			{
				jj--;
				k[jj] = k[jj + 1];
			}
			for (k[9] = k[10]; k[9] < l[9]; k[9] += i[9])
			{
				for (k[8] = k[9]; k[8] < l[8]; k[8] += i[8])
				{
					for (k[7] = k[8]; k[7] < l[7]; k[7] += i[7])
					{
						for (k[6] = k[7]; k[6] < l[6]; k[6] += i[6])
						{
							for (k[5] = k[6]; k[5] < l[5]; k[5] += i[5])
							{
								for (k[4] = k[5]; k[4] < l[4]; k[4] += i[4])
								{
									for (k[3] = k[4]; k[3] < l[3]; k[3] += i[3])
									{
										for (k[2] = k[3]; k[2] < l[2]; k[2] += i[2])
										{
											for (k[1] = k[2]; k[1] < l[1]; k[1] += i[1])
											{
												for (k[0] = k[1]; k[0] < l[0]; k[0] += i[0])
												{
													if (kk < k[0])
													{
														t = x[kk];
														x[kk] = x[k[0]];
														x[k[0]] = t;
													}
													kk++;
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}

			level = loop;
			do
			{
				if (level >= nest)
				{
					break;
				}
				k[level] += i[level];
				level++;
			} while (k[level - 1] >= l[level - 1]);
		} while (level < nest);
	} /*if (sym[0] != 0)*/
} /*ssymrp()*/

/* rftkd.c */
/*
   Adapted from Fortran Fast Fourier Transform routines written by
   Gordon Sande, circa 1968, University of Chicago

   Translation to C by C. Bingham, University of Minnesota, July 1991
*/


/*      real fourier transform kernel driver */
/*
  NOTE: this requires scratch space 4*(pmax-1) + (pmax-1)*(pmax-1)/2
  doubles plus 2*pmax-1 (double *)'s.  Thus it may fail for large pmax.
*/

void            rftkd(long n, long factor [], long pmax, double x [],
					  long * error)
{
	long            f = 0, m = 1, p = 1;
	long            j, halfPmax;
	double        **aa, **bb;
	double       ***hx = (double ***) 0, **hscr = (double **) 0;
	double        **px = (double **) 0, *pscr = (double *) 0;
	long            nx, nscr;
	WHERE("rftkd");

	halfPmax = (pmax - 1) / 2;
	*error = 0;

	while ((p = factor[f++]) != 0)
	{
		if (p == 1)
		{
			continue;
		}
		else if (p == 2)
		{
			r2rftk(n, m, x, x + m);
		}
		else if (p == 3)
		{
			r3rftk(n, m, x, x + m, x + 2*m);
		}
		else if (p == 4)
		{
			r4rftk(n, m, x, x + m, x + 2*m, x + 3*m);
		}
		else if (p == 5)
		{
			r5rftk(n, m, x, x + m, x + 2*m, x + 3*m, x + 4*m);
		}
		else if (p == 8)
		{
			r8rftk(n, m, x, x + m, x + 2*m, x + 3*m, x + 4*m, x + 5*m, x + 6*m, x + 7*m);
		}
		else
		{ /* p != 2 && p != 3 && p != 4 && p != 5 && p != 8 */
			if (px == (double **) 0)
			{ 
				nx = pmax + 2*halfPmax;
				nscr = 4 * halfPmax + 2 * (pmax - 1) + 2 * halfPmax * halfPmax;
				if(Trash == (Symbolhandle) 0)
				{ /* allocate space if not yet done */
					Trash = GarbInstall(NTRASH);
					if(Trash == (Symbolhandle) 0 ||
					   !getScratch(hx,GHX,nx,double *) ||
					   !getScratch(hscr,GHSCR,nscr,double))
					{
						*error = OUTOFMEMORY;
						break;
					}
				}
				else
				{
					hx = (double ***) GARBAGEVALUE(Trash,GHX);
					hscr = (double **) GARBAGEVALUE(Trash,GHSCR);
				}
				
				px = *hx;
				pscr = *hscr;
				aa = px + pmax;
				bb = aa + halfPmax;
				aa[0] = pscr;
				pscr += halfPmax * halfPmax;
				bb[0] = pscr;
				pscr += halfPmax * halfPmax;
				for (j = 1; j < halfPmax; j++)
				{
					aa[j] = aa[j - 1] + halfPmax;
					bb[j] = bb[j - 1] + halfPmax;
				}
			} /* if (px == (double **) 0) */
			px[0] = x;
			for (j = 1; j < pmax; j++)
			{
				px[j] = px[j - 1] + m;
			}

			rprftk(n, m, p, px, aa, bb, pscr);
		}
		m *= p;
	} /* while ((p = factor[f++]) != 0) */
/* Trash is disposed of in qft */
} /*rftkd()*/

/* r2rftk.c */
/*
   Adapted from Fortran Fast Fourier Transform routines written by
   Gordon Sande, circa 1968, University of Chicago

   Translation to C by C. Bingham, University of Minnesota, July 1991
*/


/*      radix two real fourier transform kernel */
void            r2rftk(long n, long m, double x0 [], double x1 [])
{

	long            j, j1, k, k0, k1, mOver2, m2;
	double          angle, c, is, iu, i1, rs, ru, r1, s, twopi;
	WHERE("r2rftk");

	twopi = 8.0 * MV_PI_4;
	mOver2 = (m - 1) / 2;
	m2 = m * 2;

	for (k = 0; k < n; k += m2)
	{
		rs = x0[k] + x1[k];
		ru = x0[k] - x1[k];
		x0[k] = rs;
		x1[k] = ru;
	}

	for (j = 1; j <= mOver2; j++)
	{
		j1 = m - 2 * j;
		angle = twopi * (double) j / (double) m2;
		c = cos(angle);
		s = sin(angle);

		for (k0 = j; k0 < n; k0 += m2)
		{
			k1 = k0 + j1;
			r1 = x1[k0] * c + x1[k1] * s;
			i1 = x1[k1] * c - x1[k0] * s;
			rs = x0[k0] + r1;
			is = x0[k1] + i1;
			ru = x0[k0] - r1;
			iu = x0[k1] - i1;
			x0[k0] = rs;
			x0[k1] = ru;
			x1[k1] = is;
			x1[k0] = -iu;
		}
	}
	if (m == (m / 2) * 2)
	{
		for (k = m / 2; k < n; k += m2)
		{
			x1[k] = -x1[k];
		}
	}
	return;
} /*r2rftk()*/

/* r3rftk.c */
/*
   Adapted from Fortran Fast Fourier Transform routines written by
   Gordon Sande, circa 1968, University of Chicago

   Translation to C by C. Bingham, University of Minnesota, July 1991
*/


/*      radix three real fourier transform kernel */
void            r3rftk(long n, long m, double x0 [], double x1 [],
					   double x2 [])
{

	long            j, j1, k, k0, k1, mOver2, m3;
	double          a, angle, b, c1, c2, s1, s2, twopi;
	double          ia, ib, is, i0, i1, i2, ra, rb, rs, r0, r1, r2;
	WHERE("r3rftk");

	twopi = 8.0 * MV_PI_4;
	mOver2 = (m - 1) / 2;
	m3 = m * 3;
	a = cos(twopi / 3.0);
	b = sin(twopi / 3.0);

	for (k = 0; k < n; k += m3)
	{
		r0 = x0[k];
		rs = x1[k] + x2[k];
		ra = r0 + rs * a;
		rb = (x1[k] - x2[k]) * b;
		x0[k] = r0 + rs;
		x1[k] = ra;
		x2[k] = -rb;
	}

	for (j = 1; j <= mOver2; j++)
	{
		j1 = m - 2 * j;
		angle = twopi * (double) j / (double) m3;
		c1 = cos(angle);
		s1 = sin(angle);
		c2 = c1 * c1 - s1 * s1;
		s2 = s1 * c1 + c1 * s1;

		for (k0 = j; k0 < n; k0 += m3)
		{
			k1 = k0 + j1;
			r0 = x0[k0];
			i0 = x0[k1];
			r1 = x1[k0] * c1 + x1[k1] * s1;
			i1 = x1[k1] * c1 - x1[k0] * s1;
			r2 = x2[k0] * c2 + x2[k1] * s2;
			i2 = x2[k1] * c2 - x2[k0] * s2;
			rs = r1 + r2;
			is = i1 + i2;
			ra = r0 + rs * a;
			ia = i0 + is * a;
			rb = (r1 - r2) * b;
			ib = (i1 - i2) * b;
			x0[k0] = r0 + rs;
			x2[k1] = i0 + is;
			x1[k0] = ra + ib;
			x1[k1] = ia - rb;
			x0[k1] = ra - ib;
			x2[k0] = -(ia + rb);
		}
	}

	if (m % 2 == 0)
	{
		a = cos(twopi / 6.0);
		b = sin(twopi / 6.0);
		for (k = m / 2; k < n; k += m3)
		{
			r0 = x0[k];
			rs = x1[k] - x2[k];
			ra = r0 + rs * a;
			rb = (x1[k] + x2[k]) * b;
			x1[k] = r0 - rs;
			x0[k] = ra;
			x2[k] = -rb;
		}
	}
	return;
} /*r3rftk()*/


/* r4rftk.c */
/*
   Adapted from Fortran Fast Fourier Transform routines written by
   Gordon Sande, circa 1968, University of Chicago

   Translation to C by C. Bingham, University of Minnesota, July 1991
*/


/*      radix four real fourier transform kernel */
void            r4rftk(long n, long m, double x0 [], double x1 [],
					   double x2 [], double x3 [])
{
	long            j, j1, k, k0, k1, mOver2, m4;
	double          angle, c1, c2, c3, e, s1, s2, s3, twopi;
	double          is0, is1, iu0, iu1, i1, i2, i3, rs0, rs1, ru0,
	                ru1, r1, r2, r3;

	twopi = 8.0 * MV_PI_4;
	mOver2 = (m - 1) / 2;
	m4 = m * 4;

	for (k = 0; k < n; k += m4)
	{
		rs0 = x0[k] + x1[k];
		ru0 = x0[k] - x1[k];
		rs1 = x2[k] + x3[k];
		ru1 = x2[k] - x3[k];
		x0[k] = rs0 + rs1;
		x1[k] = ru0;
		x2[k] = rs0 - rs1;
		x3[k] = -ru1;
	}

	for (j = 1; j <= mOver2; j++)
	{
		j1 = m - 2 * j;
		angle = twopi * (double) j / (double) m4;
		c1 = cos(angle);
		s1 = sin(angle);
		c2 = c1 * c1 - s1 * s1;
		s2 = s1 * c1 + c1 * s1;
		c3 = c2 * c1 - s2 * s1;
		s3 = s2 * c1 + c2 * s1;

		for (k0 = j; k0 < n; k0 += m4)
		{
			k1 = k0 + j1;
			r1 = x2[k0] * c1 + x2[k1] * s1;
			i1 = x2[k1] * c1 - x2[k0] * s1;
			r2 = x1[k0] * c2 + x1[k1] * s2;
			i2 = x1[k1] * c2 - x1[k0] * s2;
			r3 = x3[k0] * c3 + x3[k1] * s3;
			i3 = x3[k1] * c3 - x3[k0] * s3;
			rs0 = x0[k0] + r2;
			is0 = x0[k1] + i2;
			ru0 = x0[k0] - r2;
			iu0 = x0[k1] - i2;
			rs1 = r1 + r3;
			is1 = i1 + i3;
			ru1 = r1 - r3;
			iu1 = i1 - i3;
			x0[k0] = rs0 + rs1;
			x3[k1] = is0 + is1;
			x1[k0] = ru0 + iu1;
			x2[k1] = iu0 - ru1;
			x1[k1] = rs0 - rs1;
			x2[k0] = -(is0 - is1);
			x0[k1] = ru0 - iu1;
			x3[k0] = -(iu0 + ru1);
		}
	}

	if (m % 2 == 0)
	{
		e = cos(twopi / 8.0);
		for (k = m / 2; k < n; k += m4)
		{
			rs0 = x0[k];
			ru0 = (x2[k] - x3[k]) * e;
			rs1 = (x2[k] + x3[k]) * e;
			ru1 = x1[k];
			x0[k] = rs0 + ru0;
			x1[k] = rs0 - ru0;
			x2[k] = ru1 - rs1;
			x3[k] = -ru1 - rs1;
		}
	}
	return;
} /*r4rftk()*/




/* r5rftk.c */
/*
   Adapted from Fortran Fast Fourier Transform routines written by
   Gordon Sande, circa 1968, University of Chicago

   Translation to C by C. Bingham, University of Minnesota, July 1991
*/


/*      radix five real fourier transform kernel */
void            r5rftk(long n, long m, double x0 [], double x1 [],
					   double x2 [], double x3 [], double x4 [])
{

	long            j, j1, k, k0, k1, mOver2, m5;
	double          angle, a1, a2, as, au, b1, b2, c1, c2, c3, c4,
	                s1, s2, s3, s4, twopi;
	double          r0, r1, r2, r3, r4, ra1, ra2, ras, rau, rb1, rb2,
	                rs1, rs2, rss, ru1, ru2;
	double          i0, i1, i2, i3, i4, ia1, ia2, ias, iau, ib1, ib2,
	                is1, is2, iss, iu1, iu2;
	double          twopiOv5;
	WHERE("r5rftk");

	twopi = 8.0 * MV_PI_4;
	twopiOv5 = twopi / 5.0;
	mOver2 = (m - 1) / 2;
	m5 = m * 5;
	a1 = cos(twopiOv5);
	b1 = sin(twopiOv5);
	a2 = cos(2.0 * twopiOv5);
	b2 = sin(2.0 * twopiOv5);
	as = -1.0 / 4.0;
	au = sqrt(5.0) / 4.0;

	for (k = 0; k < n; k += m5)
	{
		r0 = x0[k];
		rs1 = x1[k] + x4[k];
		ru1 = x1[k] - x4[k];
		rs2 = x2[k] + x3[k];
		ru2 = x2[k] - x3[k];
		rss = rs1 + rs2;
		ras = r0 + rss * as;
		rau = (rs1 - rs2) * au;
		ra1 = ras + rau;
		ra2 = ras - rau;
		rb1 = ru1 * b1 + ru2 * b2;
		rb2 = ru1 * b2 - ru2 * b1;
		x0[k] = r0 + rss;
		x1[k] = ra1;
		x2[k] = ra2;
		x3[k] = -rb2;
		x4[k] = -rb1;
	}

	for (j = 1; j <= mOver2; j++)
	{
		j1 = m - 2 * j;
		angle = twopi * (double) j / (double) m5;
		c1 = cos(angle);
		s1 = sin(angle);
		c2 = c1 * c1 - s1 * s1;
		s2 = s1 * c1 + c1 * s1;
		c3 = c2 * c1 - s2 * s1;
		s3 = s2 * c1 + c2 * s1;
		c4 = c2 * c2 - s2 * s2;
		s4 = s2 * c2 + c2 * s2;

		for (k0 = j; k0 < n; k0 += m5)
		{
			k1 = k0 + j1;
			r0 = x0[k0];
			i0 = x0[k1];
			r1 = x1[k0] * c1 + x1[k1] * s1;
			i1 = x1[k1] * c1 - x1[k0] * s1;
			r2 = x2[k0] * c2 + x2[k1] * s2;
			i2 = x2[k1] * c2 - x2[k0] * s2;
			r3 = x3[k0] * c3 + x3[k1] * s3;
			i3 = x3[k1] * c3 - x3[k0] * s3;
			r4 = x4[k0] * c4 + x4[k1] * s4;
			i4 = x4[k1] * c4 - x4[k0] * s4;
			rs1 = r1 + r4;
			is1 = i1 + i4;
			ru1 = r1 - r4;
			iu1 = i1 - i4;
			rs2 = r2 + r3;
			is2 = i2 + i3;
			ru2 = r2 - r3;
			iu2 = i2 - i3;
			rss = rs1 + rs2;
			iss = is1 + is2;
			ras = r0 + rss * as;
			ias = i0 + iss * as;
			rau = (rs1 - rs2) * au;
			iau = (is1 - is2) * au;
			ra1 = ras + rau;
			ia1 = ias + iau;
			ra2 = ras - rau;
			ia2 = ias - iau;
			rb1 = ru1 * b1 + ru2 * b2;
			ib1 = iu1 * b1 + iu2 * b2;
			rb2 = ru1 * b2 - ru2 * b1;
			ib2 = iu1 * b2 - iu2 * b1;
			x0[k0] = r0 + rss;
			x4[k1] = i0 + iss;
			x1[k0] = ra1 + ib1;
			x3[k1] = ia1 - rb1;
			x2[k0] = ra2 + ib2;
			x2[k1] = ia2 - rb2;
			x1[k1] = ra2 - ib2;
			x3[k0] = -(ia2 + rb2);
			x0[k1] = ra1 - ib1;
			x4[k0] = -(ia1 + rb1);
		}
	}

	if (m == m / 2 * 2)
	{
		a1 = cos(0.5 * twopiOv5);
		b1 = sin(0.5 * twopiOv5);
		a2 = cos(twopiOv5);
		b2 = sin(twopiOv5);
		for (k = m / 2; k < n; k += m5)
		{
			r0 = x0[k];
			rs1 = x1[k] - x4[k];
			ru1 = x1[k] + x4[k];
			rs2 = x2[k] - x3[k];
			ru2 = x2[k] + x3[k];
			ra1 = r0 + rs1 * a1 + rs2 * a2;
			ra2 = r0 - rs1 * a2 - rs2 * a1;
			rb1 = ru1 * b1 + ru2 * b2;
			rb2 = ru1 * b2 - ru2 * b1;
			x2[k] = r0 - rs1 + rs2;
			x0[k] = ra1;
			x1[k] = ra2;
			x3[k] = -rb2;
			x4[k] = -rb1;
		}
	}
	return;
} /*r5rftk()*/




/* r8rftk.c */
/*
   Adapted from Fortran Fast Fourier Transform routines written by
   Gordon Sande, circa 1968, University of Chicago

   Translation to C by C. Bingham, University of Minnesota, July 1991
*/


/*      radix eight real fourier transform kernel */
void            r8rftk(long n, long m, double x0 [], double x1 [],
					   double x2 [], double x3 [], double x4 [], double x5 [],
					   double x6 [], double x7 [])
{
	long            j, j1, k, k0, k1, mOver2, m8;
	double          a, angle, b, c1, c2, c3, c4, c5, c6, c7, e, s1,
	                s2, s3, s4, s5, s6, s7, t, twopi;
	double          r1, r2, r3, r4, r5, r6, r7, rs0, rs1, rs2, rs3,
	                ru0, ru1, ru2, ru3;
	double          i1, i2, i3, i4, i5, i6, i7, is0, is1, is2, is3,
	                iu0, iu1, iu2, iu3;
	double          rss0, rss1, rsu0, rsu1, rus0, rus1, ruu0, ruu1;
	double          iss0, iss1, isu0, isu1, ius0, ius1, iuu0, iuu1;

	twopi = 8.0 * MV_PI_4;
	mOver2 = (m - 1) / 2;
	m8 = m * 8;
	e = cos(twopi / 8.0);

	for (k = 0; k < n; k += m8)
	{
		rs0 = x0[k] + x1[k];
		ru0 = x0[k] - x1[k];
		rs1 = x4[k] + x5[k];
		ru1 = x4[k] - x5[k];
		rs2 = x2[k] + x3[k];
		ru2 = x2[k] - x3[k];
		rs3 = x6[k] + x7[k];
		ru3 = x6[k] - x7[k];
		rss0 = rs0 + rs2;
		rsu0 = rs0 - rs2;
		rss1 = rs1 + rs3;
		rsu1 = rs1 - rs3;
		ruu0 = ru0;
		iuu0 = -ru2;
		ruu1 = (ru1 - ru3) * e;
		iuu1 = -(ru1 + ru3) * e;
		x0[k] = rss0 + rss1;
		x1[k] = ruu0 + ruu1;
		x2[k] = rsu0;
		x3[k] = ruu0 - ruu1;
		x4[k] = rss0 - rss1;
		x5[k] = -iuu0 + iuu1;
		x6[k] = -rsu1;
		x7[k] = iuu0 + iuu1;
	}

	for (j = 1; j <= mOver2; j++)
	{
		j1 = m - 2 * j;
		angle = twopi * (double) j / (double) m8;
		c1 = cos(angle);
		s1 = sin(angle);
		c2 = c1 * c1 - s1 * s1;
		s2 = s1 * c1 + c1 * s1;
		c3 = c2 * c1 - s2 * s1;
		s3 = s2 * c1 + c2 * s1;
		c4 = c2 * c2 - s2 * s2;
		s4 = s2 * c2 + c2 * s2;
		c5 = c4 * c1 - s4 * s1;
		s5 = s4 * c1 + c4 * s1;
		c6 = c4 * c2 - s4 * s2;
		s6 = s4 * c2 + c4 * s2;
		c7 = c4 * c3 - s4 * s3;
		s7 = s4 * c3 + c4 * s3;

		for (k0 = j; k0 < n; k0 += m8)
		{
			k1 = k0 + j1;
			r1 = x4[k0] * c1 + x4[k1] * s1;
			i1 = x4[k1] * c1 - x4[k0] * s1;
			r2 = x2[k0] * c2 + x2[k1] * s2;
			i2 = x2[k1] * c2 - x2[k0] * s2;
			r3 = x6[k0] * c3 + x6[k1] * s3;
			i3 = x6[k1] * c3 - x6[k0] * s3;
			r4 = x1[k0] * c4 + x1[k1] * s4;
			i4 = x1[k1] * c4 - x1[k0] * s4;
			r5 = x5[k0] * c5 + x5[k1] * s5;
			i5 = x5[k1] * c5 - x5[k0] * s5;
			r6 = x3[k0] * c6 + x3[k1] * s6;
			i6 = x3[k1] * c6 - x3[k0] * s6;
			r7 = x7[k0] * c7 + x7[k1] * s7;
			i7 = x7[k1] * c7 - x7[k0] * s7;
			rs0 = x0[k0] + r4;
			is0 = x0[k1] + i4;
			ru0 = x0[k0] - r4;
			iu0 = x0[k1] - i4;
			rs1 = r1 + r5;
			is1 = i1 + i5;
			ru1 = r1 - r5;
			iu1 = i1 - i5;
			rs2 = r2 + r6;
			is2 = i2 + i6;
			ru2 = r2 - r6;
			iu2 = i2 - i6;
			rs3 = r3 + r7;
			is3 = i3 + i7;
			ru3 = r3 - r7;
			iu3 = i3 - i7;
			rss0 = rs0 + rs2;
			iss0 = is0 + is2;
			rsu0 = rs0 - rs2;
			isu0 = is0 - is2;
			rss1 = rs1 + rs3;
			iss1 = is1 + is3;
			rsu1 = rs1 - rs3;
			isu1 = is1 - is3;
			rus0 = ru0 - iu2;
			ius0 = iu0 + ru2;
			ruu0 = ru0 + iu2;
			iuu0 = iu0 - ru2;
			rus1 = ru1 - iu3;
			ius1 = iu1 + ru3;
			ruu1 = ru1 + iu3;
			iuu1 = iu1 - ru3;
			t = (rus1 + ius1) * e;
			ius1 = (ius1 - rus1) * e;
			rus1 = t;
			t = (ruu1 + iuu1) * e;
			iuu1 = (iuu1 - ruu1) * e;
			ruu1 = t;
			x0[k0] = rss0 + rss1;
			x7[k1] = iss0 + iss1;
			x1[k0] = ruu0 + ruu1;
			x6[k1] = iuu0 + iuu1;
			x2[k0] = rsu0 + isu1;
			x5[k1] = isu0 - rsu1;
			x3[k0] = rus0 + ius1;
			x4[k1] = ius0 - rus1;
			x3[k1] = rss0 - rss1;
			x4[k0] = -(iss0 - iss1);
			x2[k1] = ruu0 - ruu1;
			x5[k0] = -(iuu0 - iuu1);
			x1[k1] = rsu0 - isu1;
			x6[k0] = -(isu0 + rsu1);
			x0[k1] = rus0 - ius1;
			x7[k0] = -(ius0 + rus1);
		}
	}

	if (m == m / 2 * 2)
	{
		a = cos(twopi / 16.0);
		b = cos(3.0 * twopi / 16.0);
		for (k = m / 2; k < n; k += m8)
		{
			rs1 = x4[k] + x7[k];
			ru1 = x4[k] - x7[k];
			rs2 = (x2[k] + x3[k]) * e;
			ru2 = (x2[k] - x3[k]) * e;
			rs3 = x6[k] + x5[k];
			ru3 = x6[k] - x5[k];
			rss0 = x1[k] + rs2;
			rsu0 = x1[k] - rs2;
			rss1 = rs1 * b + rs3 * a;
			rsu1 = rs1 * a - rs3 * b;
			rus0 = x0[k] + ru2;
			ruu0 = x0[k] - ru2;
			rus1 = ru1 * a + ru3 * b;
			ruu1 = ru1 * b - ru3 * a;
			x0[k] = rus0 + rus1;
			x1[k] = ruu0 + ruu1;
			x2[k] = ruu0 - ruu1;
			x3[k] = rus0 - rus1;
			x4[k] = rss0 - rss1;
			x5[k] = -rsu0 - rsu1;
			x6[k] = rsu0 - rsu1;
			x7[k] = -rss0 - rss1;
		}
	}
	return;
} /*r8rftk()*/




/* rprftk.c */
/*
   Adapted from Fortran Fast Fourier Transform routines written by
   Gordon Sande, circa 1968, University of Chicago

   Translation to C by C. Bingham, University of Minnesota, July 1991
*/

/*      radix prime real fourier transform kernel */

void            rprftk(long n, long m, long p, double * x [], double * aa [],
					   double * bb [], double * pscr)
/* double aa[(pmax-1)/2][(pmax-1)/2], b[(pmax-1)/2][(pmax-1)/2] */
{

	long            j, jj, j1, k, k0, k1, mOver2, mp, p2, pm, u,
	                v;
	double          angle, is, iu, i1, i2, rs, ru, r1, r2, t, twopi,
	                xt, yt;
	double          factor;
	double         *c, *s, *ia, *ib, *ra, *rb;
	/*
		double c[pmax-1],s[pmax-1];
		double ia[(pmax-1)/2],ib[(pmax-1)/2],ra[(pmax-1)/2],rb[(pmax-1)/2];
	*/
	WHERE("rprftk");

	c = pscr;
	s = c + p - 1;
	ia = s + p - 1;
	ib = ia + (p - 1) / 2;
	ra = ib + (p - 1) / 2;
	rb = ra + (p - 1) / 2;

	twopi = 8.0 * MV_PI_4;
	mOver2 = (m - 1) / 2;

	mp = m * p;
	p2 = p / 2;
	pm = p - 1;
	factor = twopi / (double) p;
	for (u = 0; u < p2; u++)
	{
		jj = p - u - 2;
		angle = factor * (double) (u + 1);
		c[u] = cos(angle);
		s[u] = sin(angle);
		c[jj] = c[u];
		s[jj] = -s[u];
	}
	for (u = 1; u <= p2; u++)
		for (v = 1; v <= p2; v++)
		{
			jj = (u * v) % p - 1;
			aa[u - 1][v - 1] = c[jj];
			bb[u - 1][v - 1] = s[jj];
		}

	for (k = 0; k < n; k += mp)
	{
		xt = x[0][k];
		rs = x[1][k] + x[p - 1][k];
		ru = x[1][k] - x[p - 1][k];
		for (u = 0; u < p2; u++)
		{
			ra[u] = xt + rs * aa[0][u];
			rb[u] = ru * bb[0][u];
		}
		xt = xt + rs;
		for (u = 1; u < p2; u++)
		{
			jj = p - u - 2;
			rs = x[u + 1][k] + x[jj + 1][k];
			ru = x[u + 1][k] - x[jj + 1][k];
			xt += rs;
			for (v = 0; v < p2; v++)
			{
				ra[v] += rs * aa[u][v];
				rb[v] += ru * bb[u][v];
			}
		}
		x[0][k] = xt;
		for (u = 0; u < p2; u++)
		{
			jj = p - u - 2;
			x[u + 1][k] = ra[u];
			x[jj + 1][k] = -rb[u];
		}
	}

	for (j = 1; j <= mOver2; j++)
	{
		j1 = m - 2 * j;
		angle = twopi * (double) j / (double) mp;
		c[0] = cos(angle);
		s[0] = sin(angle);
		for (u = 1; u < pm; u++)
		{
			c[u] = c[u - 1] * c[0] - s[u - 1] * s[0];
			s[u] = s[u - 1] * c[0] + c[u - 1] * s[0];
		}

		for (k0 = j; k0 < n; k0 += mp)
		{
			k1 = k0 + j1;
			xt = x[0][k0];
			yt = x[0][k1];
			r1 = x[1][k0] * c[0] + x[1][k1] * s[0];
			i1 = x[1][k1] * c[0] - x[1][k0] * s[0];
			r2 = x[p - 1][k0] * c[p - 2] + x[p - 1][k1] * s[p - 2];
			i2 = x[p - 1][k1] * c[p - 2] - x[p - 1][k0] * s[p - 2];
			rs = r1 + r2;
			is = i1 + i2;
			ru = r1 - r2;
			iu = i1 - i2;
			for (u = 0; u < p2; u++)
			{
				ra[u] = xt + rs * aa[0][u];
				ia[u] = yt + is * aa[0][u];
				rb[u] = ru * bb[0][u];
				ib[u] = iu * bb[0][u];
			}
			xt = xt + rs;
			yt = yt + is;
			for (u = 1; u < p2; u++)
			{
				jj = p - u - 2;
				r1 = x[u + 1][k0] * c[u] + x[u + 1][k1] * s[u];
				i1 = x[u + 1][k1] * c[u] - x[u + 1][k0] * s[u];
				r2 = x[jj + 1][k0] * c[jj] + x[jj + 1][k1] * s[jj];
				i2 = x[jj + 1][k1] * c[jj] - x[jj + 1][k0] * s[jj];
				rs = r1 + r2;
				is = i1 + i2;
				ru = r1 - r2;
				iu = i1 - i2;
				xt = xt + rs;
				yt = yt + is;
				for (v = 0; v < p2; v++)
				{
					ra[v] += rs * aa[u][v];
					ia[v] += is * aa[u][v];
					rb[v] += ru * bb[u][v];
					ib[v] += iu * bb[u][v];
				}
			}
			x[0][k0] = xt;
			x[p - 1][k1] = yt;
			for (u = 0; u < p2; u++)
			{
				jj = p - u - 2;
				x[u + 1][k0] = ra[u] + ib[u];
				x[jj][k1] = ia[u] - rb[u];
				x[u][k1] = ra[u] - ib[u];
				x[jj + 1][k0] = -(ia[u] + rb[u]);
			}
		}
	}

	if (m % 2 == 0)
	{
		j = m / 2 + 1;
		factor = twopi / ((double) 2 * p);
		for (u = 0; u < p2; u++)
		{
			angle = factor * (double) (u + 1);
			c[u] = cos(angle);
			s[u] = sin(angle);
			for (v = 0; v < p2; v++)
			{
				t = aa[u][v] * c[u] + bb[u][v] * s[u];
				bb[u][v] = bb[u][v] * c[u] - aa[u][v] * s[u];
				aa[u][v] = t;
			}
		}
		for (k = m / 2; k < n; k += mp)
		{
			xt = x[0][k];
			rs = x[1][k] - x[p - 1][k];
			ru = x[1][k] + x[p - 1][k];
			for (u = 0; u < p2; u++)
			{
				ra[u] = xt + rs * aa[0][u];
				rb[u] = ru * bb[0][u];
			}
			for (u = 1; u < pm; u += 2)
			{
				xt += -x[u][k] + x[u + 1][k];
			}
			for (u = 1; u < p2; u++)
			{
				jj = p - u - 2;
				rs = x[u + 1][k] - x[jj + 1][k];
				ru = x[u + 1][k] + x[jj + 1][k];
				for (v = 0; v < p2; v++)
				{
					ra[v] += rs * aa[u][v];
					rb[v] += ru * bb[u][v];
				}
			}
			for (u = 0; u < p2; u++)
			{
				jj = p - u - 2;
				x[u][k] = ra[u];
				x[jj + 1][k] = -rb[u];
			}
			x[p2][k] = xt;
		}
	}
} /*rprftk()*/
