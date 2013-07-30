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
#pragma segment LinpakQR
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


/*
  Translated from Fortran by C. Bingham (kb@umnstat.stat.umn.edu)
*/

/* does not require stdio.h; requires math.h (included by linpack.h)
   to declare sqrt() and fabs()
*/

/*
  961210 Added code to allow checks for interrupt (see linpack.h)
  990302 Added dqrsl()
*/

#include "linpack.h"  /* prototypes and macros for names */
#include "blas.h"  /* prototypes and macros for names */

#ifndef INT
#define INT      long
#endif /*INT*/

/*
  dqrdc uses Householder transformations to compute the qr
  factorization of an n by p matrix x.  Column pivoting
  based on the 2-norms of the reduced columns may be
  performed at the users option.

  On entry

     x       double precision(ldx,p), where ldx .ge. n.
             x contains the matrix whose decomposition is to be
             computed.

     ldx     integer.
             ldx is the leading dimension of the array x.

     n       integer.
             n is the number of rows of the matrix x.

     p       integer.
             p is the number of columns of the matrix x.

     jpvt    integer(p).
             jpvt contains integers that control the selection
             of the pivot columns.  The k-th column x(k) of x
             is placed in one of three classes according to the
             value of jpvt(k).

                If jpvt(k) .gt. 0, then x(k) is an initial
                                   column.

                If jpvt(k) .eq. 0, then x(k) is a free column.

                If jpvt(k) .lt. 0, then x(k) is a final column.

             Before the decomposition is computed, initial columns
             are moved to the beginning of the array x and final
             columns to the end.  Both initial and final columns
             are frozen in place during the computation and only
             free columns are moved.  At the k-th stage of the
             reduction, if x(k) is occupied by a free column
             it is interchanged with the free column of largest
             reduced norm.  jpvt is not referenced if
             job .eq. 0.

     work    double precision(p).
             work is a work array.  work is not referenced if
             job .eq. 0.

     job     integer.
             job is an integer that initiates column pivoting.
             If job .eq. 0, no pivoting is done.
             If job .ne. 0, pivoting is done.

  On return

     x       x contains in its upper triangle the upper
             triangular matrix r of the qr factorization.
             Below its diagonal x contains information from
             which the orthogonal part of the decomposition
             can be recovered.  Note that if pivoting has
             been requested, the decomposition is not that
             of the original matrix x but that of x
             with its columns permuted as described by jpvt.

     qraux   double precision(p).
             qraux contains further information required to recover
             the orthogonal part of the decomposition.

     jpvt    jpvt(k) contains the index of the column of the
             original matrix that has been interchanged into
             the k-th column, if pivoting was requested.

  Linpack. This version dated 08/14/78 .
  G.W. Stewart, University of Maryland, Argonne National Lab.

  References  Dongarra J.J., Bunch J.R., Moler C.B., Stewart G.W.,
                *Linpack Users  Guide*, SIAM, 1979.
*/

/*
  In this C version, with indices starting at zero, jpvt[k] contains the
  index+1 of the interchanged column, i.e., just what the Fortran version
  uses.
*/


/*
  dqrdc uses the following functions and subprograms.

  blas daxpy,ddot,dscal,dswap,dnrm2
  fortran dabs,dmax1,min0,dsqrt
*/

/*
  Whether using Fortran or C storage conventions, it is assumed that
  X(i,j) is the element in row i and column j of array x.

  If C storage conventions are used, ldx is the size of the trailing
  dimension of array x

  The Fortran calling conventions have been retained in that all
  arguments are pointers
*/

#ifdef FORTRAN  /* 1st subscript changes fastest */
#define INCI 1
#define INCJ ldx
#define PX(I,J) (x + (I) + (J)*INCJ) /* &X(I,J) */
#else           /* 2nd subscript changes fastest */
#define INCI ldx
#define INCJ 1
#define PX(I,J) (x + (I)*INCI + (J)) /* &X(I,J) */
#endif
#define X(I,J) (*PX(I,J))

void DQRDC(double x[], INT * pldx, INT * pn, INT * pp, double qraux[],
		   INT jpvt[], double work[], INT * pjob)
/* double precision x(ldx,1), qraux(1), work(1)*/
{
	/*      internal variables */

	register INT ldx = *pldx;
	INT          n = *pn, p = *pp, job = *pjob;
	INT          j, jp, l, lup, maxj, pl, pu;
	double       maxnrm, tt;
	double       nrmxl, t;
	double       temp;
	INT          negj, swapj;
	INT          length, inci = INCI;

	pl = 0;
	pu = -1;
	if (job != 0)
	{
		/*
		  Pivoting has been requested.  Rearrange the columns
		  according to jpvt.
		*/

		for (j = 0;j < p ;j++)
		{
			swapj = (jpvt[j] > 0);
			negj = (jpvt[j] < 0);
			jpvt[j] = (negj) ? -(j+1) : j+1 ;/* Fortran style index returned */
			if (swapj)
			{
				if (j != pl)
				{
					DSWAP(&n, PX(0,pl), &inci, PX(0,j), &inci);
				}
				jpvt[j] = jpvt[pl];
				jpvt[pl] = j+1;
				pl++;
			}
		} /*for (j = 0;j < p ;j++)*/
		pu = p-1;
		for (j = pu;j >= 0;j--)
		{
			if (jpvt[j] < 0)
			{
				jpvt[j] = -jpvt[j];
				if (j != pu)
				{
					DSWAP(&n, PX(0,pu), &inci, PX(0,j), &inci);
					jp = jpvt[pu];
					jpvt[pu] = jpvt[j];
					jpvt[j] = jp;
				}
				pu--;
			} /*if (jpvt[j] < 0)*/
		} /*for (j = pu;j >= 0;j--)*/
	} /* if (job != 0) */

	/* compute the norms of the free columns. */

	for (j = pl;j <= pu ;j++)
	{
		work[j] = qraux[j] = DNRM2(&n, PX(0,j), &inci);
	}
	incAndTest((pu - pl + 1)*n, errorExit);

	/* perform the Householder reduction of x. */

	lup = (n < p) ? n : p;
	for (l = 0;l < lup ;l++)
	{
		if (l < pu && l >= pl)
		{
			/*
			  locate the column of largest norm and bring it
			  into the pivot position.
			*/

			maxnrm = 0.0;
			maxj = l;
			for (j = l;j <= pu ;j++)
			{
				if (qraux[j] > maxnrm)
				{
					maxnrm = qraux[j];
					maxj = j;
				}
			}

			if (maxj != l)
			{
				DSWAP(&n, PX(0,l), &inci, PX(0,maxj), &inci);
				qraux[maxj] = qraux[l];
				work[maxj] = work[l];
				jp = jpvt[maxj];
				jpvt[maxj] = jpvt[l];
				jpvt[l] = jp;
			}
		} /*if (l < pu && l >= pl)*/
		qraux[l] = 0.0;

		if (l < n-1)
		{ /* Compute the Householder transformation for column l. */
			length = n-l;
			nrmxl = DNRM2(&length, PX(l,l), &inci);
			if (nrmxl != 0.0)
			{
				if ( X(l,l) < 0)
				{
					nrmxl = -nrmxl;
				}

				temp = 1.0/nrmxl;
				DSCAL(&length, &temp, PX(l,l), &inci);
				incAndTest(2*length, errorExit);
				X(l,l)++;

				/*
				  apply the transformation to the remaining columns,
				  updating the norms.
				*/

				for (j = l+1;j < p ;j++)
				{
					length = n-l;
					t = -DDOT(&length, PX(l,l), &inci, PX(l,j), &inci)/X(l,l);
					DAXPY(&length, &t, PX(l,l), &inci, PX(l,j), &inci);

					if (j >= pl && j <= pu && qraux[j] != 0.0)
					{
						temp = fabs(X(l,j))/qraux[j];
						tt = 1.0-temp*temp;
						tt = (tt < 0.0) ? 0.0 : tt;
						t = tt;
						temp = qraux[j]/work[j];
						tt = 1.0+0.05*tt*temp*temp;
						if (tt != 1.0)
						{
							qraux[j] *= sqrt(t);
						}
						else
						{
							length--;
							qraux[j] = DNRM2(&length, PX(l+1,j), &inci);
							work[j] = qraux[j];
						}
					} /*if (j >= pl && j <= pu && qraux[j] != 0.0)*/
					incAndTest(3*length, errorExit);
				} /*for (j = l+1;j < p ;j++)*/

				/* save the transformation. */

				qraux[l] = X(l,l);
				X(l,l) = -nrmxl;
			} /* if (nrmxl != 0.0) */
		} /* if (l != n-1)  */
	} /*for (l = 0;l < lup ;l++)*/
	/*fall through*/
  errorExit:
	;
} /*DQRDC()*/

/*

 *Date written   780814   (yymmdd)
 *Revision date  820801   (yymmdd)
 *Category no.  d9, d2a1
 *Author  Stewart,  G. W.,  (U. of Maryland)
 *Purpose  Applies the output of dqrdc to compute coordinate
         transformations,  projections,  and least squares solutions.

  dqrsl applies the output of dqrdc to compute coordinate
  transformations,  projections,  and least squares solutions.
  For k .le. min(n, p),  let xk be the matrix

         xk = (x(jpvt(1)), x(jpvt(2)),  ... ,x(jpvt(k)))

  formed from columnns jpvt(1),  ... ,jpvt(k) of the original
  n x p matrix x that was input to dqrdc (if no pivoting was
  done,  xk consists of the first k columns of x in their
  original order).  dqrdc produces a factored orthogonal matrix q
  and an upper triangular matrix r such that

           xk = q * (r)
                    (0)

  This information is contained in coded form in the arrays
  x and qraux.

  On entry

     x      double precision(ldx, p).
            x contains the output of dqrdc.

     ldx    integer.
            ldx is the leading dimension of the array x.

     n      integer.
            n is the number of rows of the matrix xk.  It must
            have the same value as n in dqrdc.

     k      integer.
            k is the number of columns of the matrix xk.  k
            must not be greater than min(n, p),  where p is the
            same as in the calling sequence to dqrdc.

     qraux  double precision(p).
            qraux contains the auxiliary output from dqrdc.

     y      double precision(n)
            y contains an n - vector that is to be manipulated
            by dqrsl.

     job    integer.
            job specifies what is to be computed.  job has
            the decimal expansion abcde,  with the following
            meaning.

                 if a != 0,  compute qy.
                 if b, c,d,  or e != 0,  compute qty.
                 if c != 0,  compute b.
                 if d != 0,  compute rsd.
                 if e != 0,  compute xb.

            Note that a request to compute b,  rsd,  or xb
            automatically triggers the computation of qty,  for
            which an array must be provided in the calling
            sequence.

  On return

     qy     double precision(n).
            qy contains q*y,  if its computation has been
            requested.

     qty    double precision(n).
            qty contains trans(q)*y,  if its computation has
            been requested.  here trans(q) is the
            transpose of the matrix q.

     b      double precision(k)
            b contains the solution of the least squares problem

                 minimize norm2(y - xk*b),

            if its computation has been requested.  (Note that
            if pivoting was requested in dqrdc,  the j - th
            component of b will be associated with column jpvt(j)
            of the original matrix x that was input into dqrdc.)

     rsd    double precision(n).
            rsd contains the least squares residual y - xk*b,
            if its computation has been requested.  rsd is
            also the orthogonal projection of y onto the
            orthogonal complement of the column space of xk.

     xb     double precision(n).
            xb contains the least squares approximation xk*b,
            if its computation has been requested.  xb is also
            the orthogonal projection of y onto the column space
            of x.

     info   integer.
            info is zero unless the computation of b has
            been requested and r is exactly singular.  In
            this case,  info is the index of the first zero
            diagonal element of r and b is left unaltered.

  The parameters qy,  qty,  b,  rsd,  and xb are not referenced
  if their computation is not requested and in this case
  can be replaced by dummy variables in the calling program.
  To save storage,  the user may in some cases use the same
  array for different parameters in the calling sequence.  A
  frequently occuring example is when one wishes to compute
  any of b,  rsd,  or xb and does not need y or qty.  In this
  case one may identify y,  qty,  and one of b,  rsd,  or xb,  while
  providing separate arrays for anything else that is to be
  computed.  Thus the calling sequence

       call dqrsl(x, ldx, n,k, qraux, y,dum, y,b, y,dum, 110, info)

  will result in the computation of b and rsd,  with rsd
  overwriting y.  More generally,  each item in the following
  list contains groups of permissible identifications for
  a single calling sequence.

       1. (y, qty, b) (rsd) (xb) (qy)

       2. (y, qty, rsd) (b) (xb) (qy)

       3. (y, qty, xb) (b) (rsd) (qy)

       4. (y, qy) (qty, b) (rsd) (xb)

       5. (y, qy) (qty, rsd) (b) (xb)

       6. (y, qy) (qty, xb) (b) (rsd)

  In any group the value returned in the array allocated to
  the group corresponds to the last member of the group.

  Linpack.  this version dated 08/14/78 .
  G. W. Stewart,  University of Maryland,  Argonne National Lab.

  dqrsl uses the following functions and subprograms.

  blas daxpy, dcopy, ddot
  Fortran dabs, min0, mod

     References  Dongarra J.J.,  Bunch J.R.,  Moler C.B.,  Stewart G.W.,
              *Linpack Users  Guide*,  SIAM,  1979.
     Routines called  daxpy, dcopy, ddot

  Translated to C by C. Bingham (kb@stat.umn.edu), 990301

  Whether using Fortran or C storage conventions, it is assumed that
  X(i,j) is the element in row i and column j of array x.

  If C storage conventions are used, ldx is the size of the trailing
  dimension of array x

  The Fortran calling conventions have been retained in that all
  arguments are pointers
*/

void DQRSL(double x [], INT * pldx, INT * pn, INT * pk, double qraux [],
		   double y [], double qy [], double qty [], double b [],
		   double rsd [], double xb [], INT * pjob, INT * info)
/*
	double x(ldx,1), qraux(1), y(1), qy(1), qty(1), b(1), rsd(1), xb(1);
*/
{
	double       t, temp;
	INT          i, j, ju;
	INT          ldx = *pldx, n = *pn, k = *pk, job = *pjob;
	short        cb, cqy, cqty, cr, cxb;
	INT          inc1 = 1;

	/* Set info flag. */
	*info = 0;

	/* Determine what is to be computed. */

	cqy = job/10000 != 0;
	cqty = (job % 10000) != 0;
	cb = (job % 1000)/100 != 0;
	cr = (job % 100)/10 != 0;
	cxb = (job % 10) != 0;

	ju = (k < n) ? k : (n - 1);

	/* Special action when n=1. */

	if (ju == 0)
	{
		if (cqy)
		{
			qy[0] = y[0];
		}
		if (cqty)
		{
			qty[0] = y[0];
		}
		if (cxb)
		{
			xb[0] = y[0];
		}
		if (cb)
		{
			if (X(0, 0) != 0.0)
			{
				b[0] = y[0]/X(0, 0);
			}
			else
			{
				*info = 1;
			}
		} /*if (cb)*/

		if (cr)
		{
			rsd[0] = 0.0;
		}
	} /*if (ju == 0) */
	else
	{
		/* Set up to compute qy or qty. */

		if (cqy)
		{
			DCOPY(&n, y, &inc1, qy, &inc1);
		}
		if (cqty)
		{
			DCOPY(&n, y, &inc1, qty, &inc1);
		}
		if (cqy)
		{
			/* Compute qy. */
			for (j = ju - 1; j >= 0; j--)
			{
				if (qraux[j] != 0.0)
				{
					long       nmj = n - j;

					temp = X(j, j);
					X(j, j) = qraux[j];
					t = -DDOT(&nmj, PX(j, j), &inc1, qy + j, &inc1)/X(j, j);
					DAXPY(&nmj, &t, PX(j, j), &inc1, qy + j, &inc1);
					X(j, j) = temp;
				} /*if (qraux[j] != 0.0) */
			} /*for (j = ju - 1; j >= 0; j--)*/
			incAndTest(ju*(2*n - ju + 1), errorExit);
		} /*if (cqy)*/

		if (cqty)
		{
			/* Compute trans(q)*y. */

			for (j = 0;j < ju ;j++)
			{
				if (qraux[j] != 0.0)
				{
					long      nmj = n - j;

					temp = X(j, j);
					X(j, j) = qraux[j];
					t = -DDOT(&nmj, PX(j, j), &inc1, qty + j, &inc1)/X(j, j);
					DAXPY(&nmj, &t, PX(j, j), &inc1, qty + j, &inc1);
					X(j, j) = temp;
				} /*if (qraux[j] != 0.0) */
			} /*for (j = 0;j < ju ;j++)*/
			incAndTest(ju*(2*n - ju + 1), errorExit);
		} /*if (cqty)*/

		/* Set up to compute b,  rsd,  or xb. */

		if (cb)
		{
			DCOPY(&k, qty, &inc1, b, &inc1);
		} /*if (cb)*/

		if (cxb)
		{
			DCOPY(&k, qty, &inc1, xb, &inc1);
			for (i = k;i < n ;i++)
			{
				xb[i] = 0.0;
			}
		} /*if (cxb)*/
		if (cr)
		{
			long     nmk = n - k;

			for (i = 0;i < k ;i++)
			{
				rsd[i] = 0.0;
			}
			if (nmk > 0)
			{
				DCOPY(&nmk, qty + k, &inc1, rsd + k, &inc1);
			}
		} /*if (cr)*/

		if (cb)
		{
			/* Compute b. */
			for (j = k - 1; j >= 0; j--)
			{
				if (X(j, j) == 0.0)
				{
					*info = j + 1;
					break;
				}
				b[j] /= X(j, j);
				if (j > 0)
				{
					t = -b[j];
					DAXPY(&j, &t, PX(0, j), &inc1, b, &inc1);
				}
			} /*for (j = k - 1; j >= 0; j--)*/
			incAndTest(k*(k-1)/2, errorExit);
		} /*if (cb) */

		if (cr || cxb)
		{
			/* Compute rsd or xb as required. */

			for (j = ju - 1; j >= 0; j--)
			{
				if (qraux[j] != 0.0)
				{
					long      nmj = n - j;

					temp = X(j, j);
					X(j, j) = qraux[j];
					if (cr)
					{
						t = -DDOT(&nmj, PX(j, j), &inc1, rsd + j, &inc1)/
						  X(j, j);
						DAXPY(&nmj, &t, PX(j, j), &inc1, rsd + j, &inc1);
					}
					if (cxb)
					{
						t = -DDOT(&nmj, PX(j, j), &inc1, xb + j, &inc1)/
						  X(j, j);
						DAXPY(&nmj, &t, PX(j, j), &inc1, xb + j, &inc1);
					}
					X(j, j) = temp;
				} /*if (qraux[j] != 0.0) */
			} /*for (j = ju - 1; j >= 0; j--)*/
		} /*if (cr || cxb)*/
	} /*if (ju == 0){}else{}*/
	/* fall through */
  errorExit:
	;
} /*DQRSL()*/


