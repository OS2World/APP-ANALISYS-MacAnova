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
#pragma segment Blas
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

#include "blas.h"

/*
   The blas routines in this file were translated to C from files
   dasum.f, daxpy.f, dcopy.f, ddot.f, dnrm2.f, drot.f, drotg.f, drotm.f,
   drotmg.f, dscal.f, dsdot.f, dswap.f, and idamax.f in cmlib

   The routines referred to in the following set of #undefs are not
   compiled for MacAnova
*/

#undef DROT     /*DROT() not needed by MacAnova*/
#undef DROTG    /*DROTG() not needed by MacAnova*/
#undef DROTM    /*DROTM() not needed by MacAnova*/
#undef DROTMG   /*DROTMG() not needed by MacAnova*/

#ifndef INT
#define INT      long
#endif /*INT*/

#ifdef DAXPY
/*====> daxpy.c <====*/
/* Translated from Fortran by C. Bingham (kb@umnstat.stat.umn.edu) */
#include "blas.h"

/*
  Overwrite double precision y with double precision a*x + y.
  for i = 0 to n-1, replace  y(ly+i*incy) with a*x(lx+i*incx) +
    y(ly+i*incy), where lx = 1 if incx .ge. 0, else lx = (-incx)*n,
  and ly is defined in a similar way using incy.
*/

void DAXPY(INT * pn, double * pa, double * x1, INT * pincx, double * y1,
		   INT * pincy)
/*double x[1],y[1],a;*/
{
	INT                  n = *pn, incx = *pincx, incy = *pincy;
	register double     *x = x1, *y = y1, a = *pa;
	register INT         i, ix, iy, m, ns;

	if(n > 0 && a != 0.0)
	{
		if (incx == incy && incx  > 0)
		{
			if (incx == 1)
			{
				/*  code for both increments equal to 1 */

				/*  clean-up loop so remaining vector length is a multiple of 4. */

				m = n % 4;
				for (i=0;i<m ;i++)
				{
					y[i] += a*x[i];
				}
				for (i=m;i<n;i+=4 )
				{
					y[i]   += a*x[i];
					y[i+1] += a*x[i+1];
					y[i+2] += a*x[i+2];
					y[i+3] += a*x[i+3];
				}
			} /*if (incx == 1)*/
			else
			{
				ns = n*incx;
				for (i=0;i<ns;i+=incx )
				{
					y[i] += a*x[i];
				}
			}
		} /*if (incx == incy && incx  > 0)*/
		else
		{
			/* code for nonequal or nonpositive increments. */

			ix = (incx >= 0) ? 0 : (-n+1)*incx;
			iy = (incy >= 0) ? 0 : (-n+1)*incy;

			for (i=0;i<n ;i++)
			{
				y[iy] += a*x[ix];
				ix += incx;
				iy += incy;
			}
		}
	} /*if(n > 0 && a != 0.0)*/
} /* DAXPY() */
#endif /*DAXPY*/

#ifdef DDOT
/*====> ddot.c <====*/

/* Translated from Fortran by C. Bingham (kb@umnstat.stat.umn.edu */

/*
  Return the dot product of double precision dx and dy.
  ddot = sum for i = 0 to n-1 of  dx(lx+i*incx) * dy(ly+i*incy)
  where lx = 0 if incx .ge. 0, else lx = (-incx)*(n-1), and ly is
  defined in a similar way using incy.
*/

#include "blas.h"

double DDOT(INT * pn, double * x1, INT * pincx, double * y1, INT * pincy)
{
	INT              n = *pn, incx = *pincx, incy = *pincy;
	register double *x = x1, *y = y1, s;
	register INT     i, ix, iy, m, ns;

	s = 0.0;
	if (n > 0)
	{
		if (incx == incy && incx > 0)
		{
			if (incx == 1)
			{
				/* code for both increments equal to 1. */

				/* clean-up loop so remaining vector length is a multiple of 5 */

				m = n % 5;
				for (i=0;i<m ;i++)
				{
					s += x[i]*y[i];
				}

				for (i=m;i<n;i+=5 )
				{
					s += x[i]*y[i] + x[i+1]*y[i+1] +
						x[i+2]*y[i+2] + x[i+3]*y[i+3] + x[i+4]*y[i+4];
				}
			} /*if (incx == 1)*/
			else
			{/* incx == incy > 1 */
				ns = n*incx;
				for (i=0;i<ns;i+=incx )
				{
					s += x[i]*y[i];
				}
			}
		} /*if (incx == incy && incx > 0)*/
		else
		{
			/* code for unequal or nonpositive increments. */

			ix = (incx >= 0) ? 0 : (-n+1)*incx;
			iy = (incy >= 0) ? 0 : (-n+1)*incy;

			for (i=0;i<n ;i++)
			{
				s += x[ix]*y[iy];
				ix += incx;
				iy += incy;
			}
		} /*if (incx == incy && incx > 0){}else{}*/
	} /*if (n > 0)*/

	return (s);
} /* DDOT() */
#endif /*DDOT*/

#ifdef DASUM
/*====> dasum.c <====*/
/* Translated from Fortran by C. Bingham (kb@umnstat.stat.umn.edu) */

#include "blas.h"

/*
  returns sum of magnitudes of double precision dx.
  dasum = sum from 0 to n-1 of dabs(dx(1+i*incx))
*/

double DASUM(INT * pn, double dx[], INT * pincx)
{
	INT        i, m, ns;
	INT        n = *pn, incx = *pincx;
	double     sum = 0.0;

	if (n > 0)
	{
		if (incx != 1)
		{/*  code for increments not equal to 1. */
			ns = n*incx;
			for (i = 0;i < ns;i += incx )
			{
				sum += fabs(dx[i]);
			}
		} /*if (incx!=1)*/
		else
		{/* code for increments equal to 1. */
	/* clean-up loop so remaining vector length is a multiple of 6. */

			m = n % 6;
			for (i = 0; i < m ;i++)
			{
				sum += fabs(dx[i]);
			}
			for (i = m; i < n;i+=6 )
			{
				sum += fabs(dx[i]) + fabs(dx[i+1]) + fabs(dx[i+2]) +
					fabs(dx[i+3]) + fabs(dx[i+4]) + fabs(dx[i+5]);
			}
		} /*if (incx!=1){}else{}*/
	} /*if (n>0)*/
	return (sum);
} /*DASUM() */
#endif /*DASUM*/

#ifdef DCOPY
/*====> dcopy.c <====*/
/* Translated from Fortran by C. Bingham (kb@umnstat.stat.umn.edu) */

/*
  Copy double precision dx to double precision dy.
  for i = 0 to n-1, copy dx(lx+i*incx) to dy(ly+i*incy),
  where lx = 1 if incx .ge. 0, else lx = (-incx)*n, and ly is
  defined in a similar way using incy.
*/

void DCOPY(INT * pn, double dx[], INT * pincx, double dy[], INT * pincy)
{
	INT        n = *pn, incx = *pincx, incy = *pincy;
	INT        i, m, ns, ix, iy;

	if (n > 0)
	{
		if (incx == incy && incx > 0)
		{
			if (incx != 1)
			{
				ns = n*incx;
				for (i = 0; i < ns;i+=incx )
				{
					dy[i] = dx[i];
				}
			} /*if (incx != 1)*/
			else
			{ /* incx == incy == 1 */
			/* clean-up loop so remaining vector length is a multiple of 7. */

				m = n % 7;
				for (i = 0; i < m ;i++)
				{
					dy[i] = dx[i];
				}
				for (i = m; i < n;i+=7 )
				{
					dy[i]   = dx[i];
					dy[i+1] = dx[i+1];
					dy[i+2] = dx[i+2];
					dy[i+3] = dx[i+3];
					dy[i+4] = dx[i+4];
					dy[i+5] = dx[i+5];
					dy[i+6] = dx[i+6];
				}
			}
		} /*if (incx == incy && incx > 0)*/
		else
		{
			/* code for unequal or nonpositive increments. */

			ix = (incx >= 0) ? 0 : (-n+1)*incx;
			iy = (incy >= 0) ? 0 : (-n+1)*incy;
			for (i = 0; i < n ;i++)
			{
				dy[iy] = dx[ix];
				ix += incx;
				iy += incy;
			}
		} /*if (incx == incy && incx > 0){}else{}*/
	} /*if (n>0)*/
} /*DCOPY()*/
#endif /*DCOPY*/

#ifdef DNRM2
/*====> dnrm2.c <====*/
/* Translated from Fortran by C. Bingham (kb@umnstat.stat.umn.edu) */

#include "blas.h"

/*
  Euclidean norm of the n-vector stored in dx() with storage increment incx.
  If  n <= 0 return with result = 0.
  If n >= 1 then incx must be >= 1

  C.L.Lawson, 1978 Jan 08

  Four phase method     using two built-in constants that are
  hopefully applicable to all machines.
	  cutlo = maximum of  dsqrt(u/eps)  over all known machines.
	  cuthi = minimum of  dsqrt(v)      over all known machines.
  where
	  eps = smallest no. such that eps + 1. .gt. 1.
	  u   = smallest positive no.   (underflow limit)
	  v   = largest  no.            (overflow  limit)

  Brief outline of algorithm..

  Phase 1    scans zero components.
	move to phase 2 when a component is nonzero and .le. cutlo
	move to phase 3 when a component is .gt. cutlo
	move to phase 4 when a component is .ge. cuthi/m
  where m = n for x() real and m = 2*n for complex.

  Values for cutlo and cuthi..
  From the environmental parameters listed in the IMSL converter
  document the limiting values are as follows..
  cutlo, s.p.   u/eps = 2**(-102) for  Honeywell.  Close seconds are
  Univac and Dec at 2**(-103)
  Thus cutlo = 2**(-51) = 4.44089e-16
  cuthi, s.p.   v = 2**127 for Univac, Honeywell, and Dec.
  Thus cuthi = 2**(63.5) = 1.30438e19
  cutlo, d.p.   u/eps = 2**(-67) for Honeywell and Dec.
  Thus cutlo = 2**(-33.5) = 8.23181e-11
  cuthi, d.p.   same as s.p.  cuthi = 1.30438e19
  data cutlo, cuthi / 8.232e-11,  1.304e19 /
  data cutlo, cuthi / 4.441e-16,  1.304e19 /
 */

#define CUTLO     8.232e-11
#define CUTHI     1.304e19

double DNRM2(INT * pn, double dx[], INT * pincx)
{
	INT          n = *pn, incx = *pincx;
	INT          nn, i;
	double       dxi, d;
	double       sum = 0.0, xmax;
	double       cutlo = CUTLO, cuthi = CUTHI;
	double       hitest;

	nn = n * incx;
	for (i = 0; i  <  nn && dx[i] == 0.0;i += incx)
	{ /* phase 1.  sum is zero */
		;
	}
	
	if(i >= nn)
	{
		return (sum);
	}
	xmax = fabs(dx[i]);
	sum = 1.0;
	i += incx;
	for (/*i as is*/;i < nn;i += incx)
	{
		/*
			phase 2.  sum is small.
			scale to avoid destructive underflow.
		*/
		dxi = fabs(dx[i]);
		if(dxi > cutlo)
		{
			break;
		}
		d = dxi/xmax;
		if(d <= 1.0)
		{
			sum += d*d;
		}
		else
		{
			sum = 1.0 + sum*(d*d);
			xmax = dxi;
		}
	} /*for (;i < nn;i += incx)*/
	if(i >= nn)
	{
		return (xmax * sqrt(sum));
	}
	/*
		prepare for phase 3.
		for real or d.p. set hitest = cuthi/n
		for complex      set hitest = cuthi/(2*n)
	*/
	sum *= xmax; /* undo scaling, enforcing (sum*xmax) * xmax computation */
	sum *= xmax;
	hitest = cuthi / (double) n;
	for (/* i as is */;i < nn; i++)
	{ /* Phase 3.  Sum is mid-range.  No scaling. */
		dxi = fabs(dx[i]);
		if(dxi < hitest)
		{
			sum += dxi*dxi;
		}
		else
		{/* prepare for phase 4. */
			xmax = dxi;
			sum /= xmax; /* rescale, enforcing (sum/xmax)/xmax computation */
			sum /= xmax;
			break;
		}
	} /*for (;i < nn; i++)*/
	if(i >= nn)
	{
		return (sqrt(sum));
	}

	for (/* i as is*/;i < nn; i += incx)
	{/* in phase 4 sum is large.  scale to avoid overflow. */
		dxi = fabs(dx[i]);
		d = dxi/xmax;
		if(d <= 1.0)
		{
			sum += d*d;
		}
		else
		{
			sum = 1.0 + sum*(d*d);
			xmax = dxi;
		}
	} /*for (;i < nn; i += incx)*/
	/* compute square root and adjust for scaling. */
	return (xmax * sqrt(sum));
} /*DNRM2()*/
#endif /*DNRM2*/

#ifdef DROT /* not used in MacAnova */
/*====> drot.c <====*/

/* Translated from Fortran by C. Bingham (kb@umnstat.stat.umn.edu) */

/*
  Multiply the 2 x 2 matrix  ( dc ds) times the 2 x n matrix (dx**t)
                             (-ds dc)                        (dy**t)
  where **t indicates transpose.    The elements of dx are in
  dx(lx+i*incx), i = 0 to n-1, where lx = 1 if incx .ge. 0, else
  lx = (-incx)*n, and similarly for dy using ly and incy.
*/


void DROT(INT * pn, double dx [], INT * pincx, double dy [], INT * pincy,
		  double * pdc, double * pds)
{
	double      dc = *pdc, ds = *pds;
	double      w,z;
	INT         n = *pn, incx = *pincx, incy = *pincy;
	INT         i, kx, ky, nsteps;

	if (n > 0 && (ds != 0.0 || dc != 1.0))
	{
		if (incx == incy && incx > 0)
		{
			nsteps = incx*n;
			for (i=0;i<nsteps;i+=incx )
			{
				w = dx[i];
				z = dy[i];
				dx[i] = dc*w+ds*z;
				dy[i] = -ds*w+dc*z;
			}
		}
		else
		{
			kx = (incx >= 0) ? 0 : (-n+1)*incx;
			ky = (incy >= 0) ? 0 : (-n+1)*incy;

			for (i=0;i<n ;i++)
			{
				w = dx[kx];
				z = dy[ky];
				dx[kx] = dc*w+ds*z;
				dy[ky] = -ds*w+dc*z;
				kx += incx;
				ky += incy;
			}
		}
	}
	return;
} /*DROT()*/
#endif /*DROT*/

#ifdef DROTG  /* not used in MacAnova */
/*====> drotg.c <====*/

/* Translated from Fortran by C. Bingham (kb@umnstat.stat.umn.edu) */

#include "blas.h"
/*
  Designed by C.L.Lawson, JPL, 1977 Sept 08

  Construct the Givens transformation

      ( dc  ds )
  g = (        ) ,    dc**2 + ds**2 = 1 ,
      (-ds  dc )

  which zeros the second entry of the 2-vector  (da,db)**t .

  The quantity r = (+/-)dsqrt(da**2 + db**2) overwrites da in
  storage.  The value of db is overwritten by a value z which
  allows dc and ds to be recovered by the following algorithm:
        if z=1  set  dc=0.e0  and  ds=1.e0
        if dabs(z) .lt. 1  set  dc=dsqrt(1-z**2)  and  ds=z
        if dabs(z) .gt. 1  set  dc=1/z  and  ds=dsqrt(1-dc**2)

  Normally, the subprogram drot(n,dx,incx,dy,incy,dc,ds) will
  next be called to apply the transformation to a 2 by n matrix.

  ------------------------------------------------------------------
*/

void DROTG(double * da, double * db, double * dc, double * ds)
{
	double    u,v,r;
	
	if (fabs(*da) > fabs(*db))
	{
	/*  *** here dabs(da) .gt. dabs(db) *** */
		u = (*da)+(*da);
		v = (*db)/u;
	/*      note that u and r have the sign of da */
		r = sqrt(.25+v*v)*u;
	
	/*      note that dc is positive */
	
		*dc = (*da)/r;
		*ds = v*((*dc)+(*dc));
		*db = *ds;
		*da = r;
	}
	else if (*db == 0.0) 	/*  *** here dabs(da) .le. dabs(db) *** */
	{
	/*  *** here da = db = 0.0 *** */
		*dc = 1.0;
		*ds = 0.0;
	}
	else
	{
		u = (*db)+(*db);
		v = (*da)/u;
	
	/*      note that u and r have the sign of db */
	/*      (r is immediately stored in da) */
	
		*da = sqrt(.25 + v*v)*u;
	
	/*      note that ds is positive */
	
		*ds = (*db)/(*da);
		*dc = v*((*ds)+(*ds));
		if (*dc == 0.0)
		{
			*db = 1.0;
		}
		else
		{
			*db = 1.0/(*dc);
		}
	}
} /*DROTG()*/
#endif /*DROTG*/

#ifdef DROTM
/* ====> drotm.c <==== */  /* not used in MacAnova */

/* Translated from Fortran by C. Bingham (kb@umnstat.stat.umn.edu) */

#include "blas.h"

/*
  Apply the modified Givens transformation, h, to the 2 by n matrix

  (dx**t) , where **t indicates transpose. The elements of dx are in
  (dy**t)

  dx(lx+i*incx), i = 0 to n-1, where lx = 1 if incx .ge. 0, else
  lx = (-incx)*n, and similarly for sy using ly and incy.

  With dparam[0]=dflag, h has one of the following forms..

    dflag=-1.0      dflag=0.0         dflag=1.0      dflag=-2.0

    (dh11  dh12)    (1.0   dh12)    (dh11  1.0 )    (1.0   0.0 )
  h=(          )    (          )    (          )    (          )
    (dh21  dh22),   (dh21  1.0 ),   (-1.0  dh22),   (0.0   1.0 ).

  See drotmg for a description of data storage in dparam.
*/

void DROTM(INT * pn, double dx [],INT * pincx, double dy [], INT * pincy,
		   double dparam [5])
{
	double      dflag,dh12,dh22,z,dh11,dh21,w;
	INT         n = *pn, incx = *pincx, incy = *pincy;
	INT         kx, ky, i, nsteps;

	dflag = dparam[0];
	if (n>0 && dflag != -2.0)
	{
		if (dflag < 0)
		{
			dh11 = dparam[1];
			dh12 = dparam[3];
			dh21 = dparam[2];
			dh22 = dparam[4];
		}
		else if (dflag == 0)
		{
			dh12 = dparam[3];
			dh21 = dparam[2];
		}
		else
		{
			dh11 = dparam[1];
			dh22 = dparam[4];
		}
		if (incx != incy || incx <= 0)
		{
			kx = (incx >= 0) ? 0 : (-n+1)*incx;
			ky = (incy >= 0) ? 0 : (-n+1)*incy;

			if (dflag < 0)
			{
				for (i=0;i<n ;i++)
				{
					w = dx[kx];
					z = dy[ky];
					dx[kx] = w*dh11+z*dh12;
					dy[ky] = w*dh21+z*dh22;
					kx += incx;
					ky += incy;
				} /*for (i=0;i<n ;i++)*/
			} /*if (dflag < 0)*/
			else if (dflag == 0)
			{
				for (i=0;i<n ;i++)
				{
					w = dx[kx];
					z = dy[ky];
					dx[kx] = w + z*dh12;
					dy[ky] = w*dh21 + z;
					kx += incx;
					ky += incy;
				} /*for (i=0;i<n ;i++)*/
			}
			else
			{
				for (i=0;i<n ;i++)
				{
					w = dx[kx];
					z = dy[ky];
					dx[kx] = w*dh11 + z;
					dy[ky] = -w + dh22*z;
					kx += incx;
					ky += incy;
				} /*for (i=0;i<n ;i++)*/
			}
		} /*if (incx != incy || incx <= 0)*/
		else
		{
			nsteps = n*incx;
			if (dflag<0)
			{
				for (i=0;i<nsteps;i+=incx )
				{
					w = dx[i];
					z = dy[i];
					dx[i] = w*dh11 + z*dh12;
					dy[i] = w*dh21 + z*dh22;
				}
			} /*if (dflag<0)*/
			else if (dflag == 0)
			{
				for (i=0;i<nsteps;i+=incx )
				{
					w = dx[i];
					z = dy[i];
					dx[i] = w + z*dh12;
					dy[i] = w*dh21 + z;
				}
			}
			else
			{
				for (i=0;i<nsteps;i+=incx )
				{
					w = dx[i];
					z = dy[i];
					dx[i] = w*dh11 + z;
					dy[i] = -w + dh22*z;
				}
			}
		} /*if (incx != incy || incx <= 0){}else{}*/
	} /*if (n>0 && dflag != -2.0)*/
} /*DROTM()*/
#endif /*DROTM*/

#ifdef DROTMG
/*====> drotmg.c <====*/ /* not used in MacAnova */

/* Translated from Fortran by C. Bingham (kb@umnstat.stat.umn.edu) */

#include "blas.h"
/*
  Construct the modified givens transformation matrix h which zeros
  the second component of the 2-vector  (dsqrt(dd1)*dx1,dsqrt(dd2)*
  dy2)**t.
  With dparam(1)=dflag, h has one of the following forms..

  dflag=-1.0      dflag=0.0         dflag=1.0      dflag=-2.0

    (dh11  dh12)    (1.0   dh12)    (dh11  1.0 )    (1.0   0.0 )
  h=(          )    (          )    (          )    (          )
    (dh21  dh22),   (dh21  1.0 ),   (-1.0  dh22),   (0.0   1.0 ).
  Locations 2-4 of dparam contain dh11, dh21, dh12, and dh22
  respectively. (values of 1.0 , -1.0 , or 0.0  implied by the
  value of dparam(1) are not stored in dparam.)

  The values of gamsq and rgamsq set in the data statement may be
  inexact.  This is ok as they are only used for testing the size
  of dd1 and dd2.  All actual scaling of data is done using gam.
*/

void DROTMG(double dd1, double dd2, double dx1, double dy1, double dparam[5])
{
	double        dh11,dh21,dh12,dh22,du;
	double        dp1, dp2, dq1, dq2;
	double        dflag,dtemp;
	double        gam = 4096.0, gamsq = 16777216.0, rgamsq = 5.9604645e-8;
	INT           igo, jgo = 1;
	INT           done;
	
	done = 1;
	while(dd1 >= 0.0)
	{ /* not a real loop; an if block that can be exited by break */
		/* case-dd1-nonnegative */
		dp2 = dd2*dy1;
		if (dp2 == 0.0)
		{
			dparam[0] = -2.0;
			return;
		}
		/* regular case */
		dp1 = dd1*dx1;
		dq2 = dp2*dy1;
		dq1 = dp1*dx1;

		if (fabs(dq1) > fabs(dq2))
		{
			dh21 = -dy1/dx1;
			dh12 = dp2/dp1;

			du = 1.0 - dh12*dh21;

			if (du>0.0)
			{
				dflag = 0.0;
				dd1 /= du;
				dd2 /= du;
				dx1 *= du;
				done = 0;
			}
		}
		else if(dq2 >= 0.0)
		{
			dflag = 1.0;
			dh11 = dp1/dp2;
			dh22 = dx1/dy1;
			du = 1.0 + dh11*dh22;
			dtemp = dd2/du;
			dd2 = dd1/du;
			dd1 = dtemp;
			dx1 = dy1*du;
			done = 0;
		}
		break;
	} /* while (not reached)*/

	if(done)
	{ /* procedure..zero-h-d-and-dx1.. */
		dflag = -1.0;
		dh11 = dh12 = dh21 = dh22 = dd1 = dd2 = dx1 = 0.0;
	}
	else
	{
		igo = 0;

		while(!done)
		{
			if(igo == 0)
			{
				jgo = 1;
			}
			else
			{
				jgo = igo;
				switch (igo)
				{
				  case 1:
					dd1 = dd1*(gam*gam);
					dx1 = dx1/gam;
					dh11 = dh11/gam;
					dh12 = dh12/gam;
					break;
		
				  case 2:
					dd1 = dd1/(gam*gam);
					dx1 = dx1*gam;
					dh11 = dh11*gam;
					dh12 = dh12*gam;
					break;
		
				  case 3:
					dd2 = dd2*gam*gam;
					dh21 = dh21/gam;
					dh22 = dh22/gam;
					break;
		
				  case 4:
					dd2 = dd2/(gam*gam);
					dh21 = dh21*gam;
					dh22 = dh22*gam;
					break;
				} /* switch */
			} /* if ... else */
			
			switch (jgo)
			{ /* procedure..scale-check */
			  case 1:
				if(dd1 != 0.0 && dd1 <= rgamsq)
				{
					igo = 1;
					break;
				}
				/* fall through */
			  case 2:
				if(dd1 >= gamsq)
				{
					igo = 2;
					break;
				}
				/* fall through */
			  case 3:
				if(dd2 != 0.0 && fabs(dd2) <= rgamsq)
				{
					igo = 3;
					break;
				}
				/* fall through */
			  case 4:
				if(fabs(dd2) >= gamsq)
				{
					igo = 4;
					break;
				}
				done = 1;
			} /* switch */
	
			if(!done && dflag>=0.0)
			{ /* procedure..fix-h.. */
				if (dflag!=0.0)
				{
					dh21 = -1.0;
					dh12 = 1.0;
				}
				else
				{
					dh11 = 1.0;
					dh22 = 1.0;
				}
				dflag = -1.0;
			} /* if */
		} /* while */
	}
	/* set dparam[] */
	if (dflag<0)
	{
		dparam[1] = dh11;
		dparam[2] = dh21;
		dparam[3] = dh12;
		dparam[4] = dh22;
	}
	else if (dflag == 0)
	{
		dparam[2] = dh21;
		dparam[3] = dh12;
	}
	else
	{
		dparam[1] = dh11;
		dparam[4] = dh22;
	}

	dparam[0] = dflag;
	return;
} /*DROTMG()*/
#endif /*DROTMG*/

#ifdef DSCAL
/*====> dscal.c <====*/
/* Translated from Fortran by C. Bingham (kb@umnstat.stat.umn.edu) */

#include "blas.h"

/*
  Replace double precision dx by double precision da*dx.
  For i = 0 to n-1, replace dx(1+i*incx) with  da * dx(1+i*incx)
*/
void DSCAL(INT * pn, double * pda, double dx[], INT * pincx)
{
	INT         n = *pn, incx = *pincx;
	INT         i, m, ns;
	double      da = *pda;

	if (n > 0)
	{
		if (incx != 1)
		{
			/* code for increments not equal to 1. */
			ns = n*incx;
			for (i=0;i<ns;i+=incx )
			{
				dx[i] *= da;
			}
		} /*if (incx!=1)*/
		else
		{
		/* code for increments equal to 1. */

		/* clean-up loop so remaining vector length is a multiple of 5. */
			m = n % 5;
			for (i=0;i<m ;i++)
			{
				dx[i] *= da;
			}
			for (i=m;i<n;i+=5 )
			{
				dx[i] *= da;
				dx[i+1] *= da;
				dx[i+2] *= da;
				dx[i+3] *= da;
				dx[i+4] *= da;
			}
		} /*if (incx != 1){}else{}*/
	} /*if (n > 0)*/
} /*dscal()*/
#endif /*DSCAL*/

#ifdef DSWAP
/*====> dswap.c <====*/
/* Translated from Fortran by C. Bingham (kb@umnstat.stat.umn.edu) */

#include "blas.h"

/*
  Interchange double precision dx and double precision dy.
  For i = 0 to n-1, interchange  dx(lx+i*incx) and dy(ly+i*incy),
  where lx = 1 if incx .ge. 0, else lx = (-incx)*n, and ly is
  defined in a similar way using incy.
*/

void DSWAP(INT * pn, double dx[], INT * pincx, double dy[], INT * pincy)
{
	double     dtemp1,dtemp2,dtemp3;
	INT        n = *pn, incx = *pincx, incy = *pincy;
	INT        i, ns, m, ix, iy;

	if (n > 0)
	{
		if (incx == incy && incx >= 1)
		{
			if (incx != 1)
			{/* code for equal, positive, nonunit increments. */
				ns = n*incx;
				for (i=0;i<ns;i+=incx )
				{
					dtemp1 = dx[i];
					dx[i] = dy[i];
					dy[i] = dtemp1;
				}
			} /*if (incx != 1)*/
			else
			{/* code for both increments equal to 1 */
			/* clean-up loop so remaining vector length is a multiple of 3*/
				m = n % 3;
				for (i=0;i<m ;i++)
				{
					dtemp1 = dx[i];
					dx[i] = dy[i];
					dy[i] = dtemp1;
				}
				for (i=m;i<n;i+=3 )
				{
					dtemp1 = dx[i];
					dtemp2 = dx[i+1];
					dtemp3 = dx[i+2];
					dx[i] = dy[i];
					dx[i+1] = dy[i+1];
					dx[i+2] = dy[i+2];
					dy[i] = dtemp1;
					dy[i+1] = dtemp2;
					dy[i+2] = dtemp3;
				}
			} /*if (incx != 1){}else{}*/
		} /*if (incx == incy && incx >= 1)*/
		else
		{/* code for unequal or nonpositive increments. */
			ix = (incx >= 0) ? 0 : (-n+1)*incx;
			iy = (incy >= 0) ? 0 : (-n+1)*incy;
			for (i=0;i<n ;i++)
			{
				dtemp1 = dx[ix];
				dx[ix] = dy[iy];
				dy[iy] = dtemp1;
				ix += incx;
				iy += incy;
			}
		} /*if (incx == incy && incx >= 1){}else{}*/
	} /*if (n>0)*/
} /*DSWAP()*/
#endif /*DSWAP*/

#ifdef IDAMAX
/*====> idamax.c <====*/
/* Translated from Fortran by C. Bingham (kb@umnstat.stat.umn.edu) */

#include "blas.h"

/*
  find smallest index of maximum magnitude of double precision dx.
  idamax =  first i, i = 1 to n, to minimize  abs(dx(1-incx+i*incx))
  Note that C version returns value from 1 to n (unless n == 0), just as
  does Fortran.
*/

INT IDAMAX(INT * pn, double dx[], INT * pincx)
{
	double     dmax, xmag;
	INT        n = *pn, incx = *pincx;
	INT        i, ii, ns, idamax;

	if (n > 0)
	{
		idamax = 0;
		if (n>1)
		{
			dmax = fabs(dx[0]);
			if (incx == 1)
			{/* code for increments equal to 1. */
				for (i = 1;i < n ;i++)
				{
					xmag = fabs(dx[i]);
					if (xmag>dmax)
					{
						idamax = i;
						dmax = xmag;
					}
				} /*for (i = 1;i < n ;i++)*/
			} /*if (incx == 1)*/
			else
			{/* code for increments not equal to 1. */
				ns = n*incx;
				ii = 0;
				for (i=incx;i<ns;i+=incx )
				{
					xmag = fabs(dx[i]);
					if (xmag > dmax)
					{
						idamax = ii;
						dmax = xmag;
					}
					ii++;
				}
			} /*if (incx == 1){}else{}*/
		} /*if (n > 1)*/
	} /*if (n > 0)*/
	else
	{
		idamax = -1;
	}
	return (idamax + 1);
} /*IDAMAX()*/
#endif /*IDAMAX*/
