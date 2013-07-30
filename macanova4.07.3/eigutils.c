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
#pragma segment Eigutils
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#ifndef NOSTDLIBH
#ifdef MPW
#include <StdLib.h>
#else /*MPW*/
#include <stdlib.h>
#endif /*MPW*/
#endif /*NOSTDLIBH*/

#include "globals.h" /* among other things this brings in math.h */

#define svd eispsvd  /* svd() is invoked as eispsvd() by MacAnova*/

/*
   961204 The following will be defined in MacAnova.  They are
   included here to make easier re-use of these C versions of
   eispack routines
*/

#ifndef incNproducts
#define incNproducts(INC)
#endif /*incNproducts*/

#ifndef testNproducts
#define testNproducts(LABEL)
#endif /*testNproducts*/

/* 
  Translations of various Eispack subroutines for computing eigen values
  and vectors and singular values and vectors
  Translations to C by C. Bingham, kb@stat.umn.edu

  Except when an argument is modified (e.g., ierr), all scalars are passed
  by value.  There are a few other deviations from the original Eispack
  specifications.  For example, except in tridib(), eigenvalues are returned
  decreasing rather than increasing order, and, in svd(), singular values
  and vectors are rearranged in decreasing order of singular values.  The
  originally specified order can be obtained by changing macro COMPARE

  When FORTRAN storage mode is used (by columns), pointers are used in loops
  when traversing down a column of a matrix, but not for traversing
  across a row.
*/

#define FORTRAN  /* assume Fortran storage ordering */

#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define SIGN(a,b) (((b) > 0) ? fabs((a))  :  -fabs((a)))

#define DESCENDING /* remove to have ascending singular values, eigenvalues */
/* this doesn't affect behavior of tridib()*/
#ifdef DESCENDING
#define COMPARE(A, B) ((A) > (B))
#else /*DESCENDING*/
#define COMPARE(A, B) ((A) < (B))
#endif /*DESCENDING*/

#ifdef FORTRAN
#define USEPTRS /* use pointers when helpful */
#define Q2a(I,J) a[(I) + nm*(J)]
#define Q2b(I,J) b[(I) + nm*(J)]
#define Q2z(I,J) z[(I) + nm*(J)]

#else /*FORTRAN*/
#undef USEPTRS /*code using pointers is wrong for C storage mode*/
#define Q2a(I,J) a[(J) + nm*(I)]
#define Q2b(I,J) b[(J) + nm*(I)]
#define Q2z(I,J) z[(J) + nm*(I)]
#endif /*FORTRAN*/

extern double pythag(double /*a*/, double /*b*/);

static void     rebak(long , long, double *, double *, long, double *);
static void     reduc(long, long, double *, double *, double *, long *);
void            rsg(long, long, double *, double *, double *, long,
					double *, double *, double *, long *);
void tql2(long /*nm*/, long /*n*/, double */*d*/, double */*e*/,
		  double */*z*/, long */*ierr*/);
void tqlrat(long /*n*/, double */*d*/, double */*e2*/, long */*ierr*/);
void tred1(long /*nm*/, long /*n*/, double */*a*/, double */*d*/,
		   double */*e*/, double */*e2*/);
void tred2(long /*nm*/, long /*n*/, double */*a*/, double */*d*/,
		   double */*e*/, double */*z*/);
void svd(long /*nm*/, long /*m*/, long /*n*/, double * /*a*/, double * /*w*/,
		 long /*matu*/, double * /*u*/, long /*matv*/, double * /*v*/,
		 double * /*rv1*/, long * /*ierr*/);
void tridib(long /*n*/, double * /*eps1*/, double * /*d*/, double * /*e*/,
			double * /*e2*/, double * /*lb*/,double * /*ub*/, long /*m11*/,
			long /*m*/, double * /*w*/, long * /*ind*/, long * /*ierr*/,
			double * /*rv4*/, double * /*rv5*/);
void tinvit(long /*nm*/, long /*n*/, double * /*d*/, double * /*e*/,
			double * /*e2*/, long /*m*/, double * /*w*/,long * /*ind*/,
			double * /*z*/, long * /*ierr*/, double * /*rv1*/,
			double * /*rv2*/, double * /*rv3*/,double * /*rv4*/,
			double * /*rv6*/);

/*
	C. Bingham, kb@umnstat.stst.umn.edu  891213
	From Eispack
	Minor modifications and neatening up 950519
*/

#if (0)
/* moved to mathutil.c */
/*      estimate unit roundoff in quantities of size x. */


/*
	This program should function properly on all systems
      satisfying the following two assumptions,
         1.  The base used in representing floating point
             numbers is not a power of three.
         2.  The quantity  a  in statement 10 is represented to
             the accuracy used in floating point variables
             that are stored in memory.
      The label 'outfox' and the 'goto outfox' are intended to
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

static double          epslon(double x)
{
	double          a, b, c, eps;

	a = 4.0 / 3.0;
/* use of label designed to out-fox optimizing compiler; see above */
  outfox:
	b = a - 1.0;
	c = b + b + b;
	eps = fabs(c - 1.0);
	if (eps == 0)
	{
		goto outfox;
	}
	return (eps * fabs(x));
} /*epslon()*/
#endif /*0*/

/*
	From Eispack library
	Translated to C by C. Bingham, kb@umnstat.stst.umn.edu  891213
*/

/* translated from rebak.f */
/*
	From Eispack Library
	Compile with -DFORTRAN for Fortran storage format
	C. Bingham, kb@umnstat.stst.umn.edu  891215
*/


/*
      This subroutine is a translation of the Algol procedure rebaka,
      Num. Math. 11, 99-110(1968) by Martin and Wilkinson.
      Handbook for Auto. Comp., Vol.Ii-Linear Algebra, 303-314(1971).


      This subroutine forms the eigenvectors of a generalized
      symmetric eigensystem by back transforming those of the
      derived symmetric matrix determined by  reduc.


      On input


         nm must be set to the row dimension (column dimension ifndef
		   FORTRAN) of dimensional array parameters as declared in the
		   calling program dimension statement.


         n is the order of the matrix system.


         b contains information about the similarity transformation
           (cholesky decomposition) used in the reduction by  reduc
           in its strict lower triangle.


         dl contains further information about the transformation.


         m is the number of eigenvectors to be back transformed.


         z contains the eigenvectors to be back transformed
           in its first m columns.


      On output


         z contains the transformed eigenvectors
           in its first m columns.


      Questions and comments should be directed to Burton S. Garbow,
      Mathematics and Computer Science Div, Argonne National Laboratory


      This version dated August 1983.


      ------------------------------------------------------------------
*/

static void            rebak(long nm, long  n, double *b, double *dl, long  m,
							 double *z)
/*double b[n][nm],dl[n],z[n][nm];*/
{
	long            i, j, k;
	double          x;
#ifdef USEPTRS
	double         *zj, *bi;
#endif /*USEPTRS*/

#ifdef USEPTRS
	zj = z;
#endif /*USEPTRS*/
	for (j = 0; j < m; j++)
	{
/*      .......... for i=n step -1 until 1 do -- .......... */
#ifdef USEPTRS
		bi = b + (n-1)*nm;
#endif /*USEPTRS*/
		for (i = n - 1; i >= 0; i--)
		{
#ifdef USEPTRS
			x = zj[i];
#else /*USEPTRS*/
			x = Q2z(i, j);
#endif /*USEPTRS*/
			for (k = i + 1; k < n; k++)
			{
#ifdef USEPTRS
				x -= bi[k] * zj[k];
#else /*USEPTRS*/
				x -= Q2b(k, i) * Q2z(k, j);
#endif /*USEPTRS*/
			}
#ifdef USEPTRS
			zj[i] = x / dl[i];
			bi -= nm;
#else /*USEPTRS*/
			Q2z(i, j) = x / dl[i];
#endif /*USEPTRS*/
		} /*for (i = n - 1; i >= 0; i--)*/
		incNproducts(n*(n-1)/2);
		testNproducts(errorExit);
#ifdef USEPTRS
		zj += nm;
#endif /*USEPTRS*/
	} /*for (j = 0; j < m; j++)*/

/*fall through*/
  errorExit:
	;
} /*rebak()*/

/* translated from reduc.f */
/*
	From Eispack Library

	C. Bingham, kb@umnstat.stst.umn.edu  891215
	Compile with -DFORTRAN for fortran storage format
*/

/*
      this subroutine is a translation of the algol procedure reduc1,
      num. math. 11, 99-110(1968) by martin and wilkinson.
      handbook for auto. comp., vol.ii-linear algebra, 303-314(1971).


      This subroutine reduces the generalized symmetric eigenproblem
      ax=(hlambda)bx, where b is positive definite, to the standard
      symmetric eigenproblem using the Cholesky factorization of b.


      On input


         nm must be set to the row dimension (column dimension ifndef
		   FORTRAN) of two-dimensional array parameters as declared in the
		   calling program dimension statement.


         n is the order of the matrices a and b.  if the cholesky
           factor l of b is already available, n should be prefixed
           with a minus sign.


         a and b contain the real symmetric input matrices.  only the
           full upper triangles of the matrices need be supplied.  if
           n is negative, the strict lower triangle of b contains,
           instead, the strict lower triangle of its cholesky factor l.


         dl contains, if n is negative, the diagonal elements of l.


      On output


         a contains in its full lower triangle the full lower triangle
           of the symmetric matrix derived from the reduction to the
           standard form.  The strict upper triangle of a is unaltered.


         b contains in its strict lower triangle the strict lower
           triangle of its Cholesky factor l.  The full upper
           triangle of b is unaltered.


         dl contains the diagonal elements of l.


         ierr is set to
           zero       for normal return,
           7*n+1      if b is not positive definite.


      Questions and comments should be directed to Burton S. Garbow,
      Mathematics and Computer Science Div, Argonne National Laboratory


      This version dated August 1983.


      ------------------------------------------------------------------
*/

static void            reduc(long nm, long n, double *a, double *b, double *dl,
							 long *ierr)
/*
double a[n][nm],b[n][nm],dl[n]; for C storage mode
double a(nm,n), b(nm,n),dl(n) for Fortran storage mode
*/
{
	long            i, j, k, nn;
	double          x, y = 0.0;
#ifdef USEPTRS
	double         *aj, *ai, *bi;
#endif /*USEPTRS*/

	*ierr = 0;
	nn = labs(n);
	if (n >= 0)
	{
/*      .......... form l in the arrays b and dl .......... */
#ifdef USEPTRS
		bi = b;
#endif /*USEPTRS*/
		for (i = 0; i < n; i++)
		{
			for (j = i; j < n; j++)
			{
				x = Q2b(i, j);
				for (k = 0; k < i; k++)
				{
					x -= Q2b(i, k) * Q2b(j, k);
				} /*for (k = 0; k < i; k++)*/
				if (j != i)
				{
#ifdef USEPTRS
					bi[j] = x / y;
#else /*USEPTRS*/
					Q2b(j, i) = x / y;
#endif /*USEPTRS*/
				} /*if (j != i)*/
				else
				{
					if (x <= 0.0)
					{
/*      .......... set error -- b is not positive definite .......... */
						*ierr = 7 * n + 1;
						return;
					}
					y = sqrt(x);
					dl[i] = y;
				} /*if (j != i){}else*/
			} /*for (j = i; j < n; j++)*/
			incNproducts(i*(n-i));
			testNproducts(errorExit);
#ifdef USEPTRS
			bi += nm;
#endif /*USEPTRS*/
		} /*for (i = 0; i < n; i++)*/
	} /*if (n >= 0)*/

/*
      .......... form the transpose of the upper triangle of inv(l)*a
                 in the lower triangle of the array a ..........
*/
#ifdef USEPTRS
	ai = a;
#endif /*USEPTRS*/
	for (i = 0; i < nn; i++)
	{
		y = dl[i];
		for (j = i; j < nn; j++)
		{
			x = Q2a(i, j);
			for (k = 0; k < i; k++)
			{
				x -= Q2b(i, k) * Q2a(j, k);
			} /*for (k = 0; k < i; k++)*/
#ifdef USEPTRS
			ai[j] = x / y;
#else /*USEPTRS*/
			Q2a(j, i) = x / y;
#endif /*USEPTRS*/
		} /*for (j = i; j < nn; j++)*/
		incNproducts(i*(nn-i));
		testNproducts(errorExit);
#ifdef USEPTRS
		ai += nm;
#endif /*USEPTRS*/
	} /*for (i = 0; i < nn; i++)*/

/*      .......... pre-multiply by inv(l) and overwrite .......... */
#ifdef USEPTRS
	aj = a;
#endif /*USEPTRS*/
	for (j = 0; j < nn; j++)
	{
		for (i = j; i < nn; i++)
		{
#ifdef USEPTRS
			x = aj[i];
#else /*USEPTRS*/
			x = Q2a(i, j);
#endif /*USEPTRS*/
			for (k = j; k < i; k++)
			{
#ifdef USEPTRS
				x -= aj[k] * Q2b(i, k);
#else /*USEPTRS*/
				x -= Q2a(k, j) * Q2b(i, k);
#endif /*USEPTRS*/
			} /*for (k = j; k < i; k++)*/

			for (k = 0; k < j; k++)
			{
				x -= Q2a(j, k) * Q2b(i, k);
			} /*for (k = 0; k < j; k++)*/
			incNproducts(i);
#ifdef USEPTRS
			aj[i] = x / dl[i];
#else /*USEPTRS*/
			Q2a(i, j) = x / dl[i];
#endif /*USEPTRS*/
		} /*for (i = j; i < nn; i++)*/
		testNproducts(errorExit);
#ifdef USEPTRS
		aj += nm;
#endif /*USEPTRS*/
	} /*for (j = 0; j < nn; j++)*/
	/*fall through*/

  errorExit:
	;

} /*reduc()*/

/* translated from rsg.f */
/*
	From Eispack Library
    Translated to C by C. Bingham, kb@umnstat.stat.umn.edu 891215
*/


/*
      This subroutine calls the recommended sequence of
      subroutines from the eigensystem subroutine package (eispack)
      to find the eigenvalues and eigenvectors (if desired)
      for the real symmetric generalized eigenproblem  ax = (lambda)bx.


      On input


         nm must be set to the row dimension (column dimension ifndef
		 FORTRAN) of two-dimensional array parameters as declared in the
		 calling program dimension statement.


         n  is the order of the matrices  a  and  b.


         a  contains a real symmetric matrix.


         b  contains a positive definite real symmetric matrix.


         matz  is an integer variable set equal to zero if
         only eigenvalues are desired.  Otherwise it is set to
         any non-zero integer for both eigenvalues and eigenvectors.


      On output


         w  contains the eigenvalues in ascending order
		 (changed to descending by C. Bingham, see macro COMPARE)


         z  contains the eigenvectors if matz is not zero.


         ierr  is a pointer to an integer output variable set equal to an error
            completion code described in the documentation for tqlrat
            and tql2.  the normal completion code is zero.


         fv1  and  fv2  are temporary storage arrays.


      Questions and comments should be directed to Burton S. Garbow,
      Mathematics and Computer Science Div, Argonne National Laboratory


      This version dated August 1983.


      ------------------------------------------------------------------
*/

void            rsg(long nm, long n, double *a, double *b, double *w,
					 long matz, double *z, double *fv1, double *fv2,
					 long *ierr)
/*
double a[n][nm],b[n][nm],w[n],z[n][nm],fv1[n],fv2[n]; (C-storage mode)
double a(nm,n),b(nm,n),w(n),z(nm,n),fv1(n),fv2(n)  (Fortran storage mode)
*/
{
	if (n > nm)
	{
		*ierr = 10 * n;
	} /*if (n > nm)*/
	else
	{
		reduc(nm, n, a, b, fv2, ierr);
		testNproducts(errorExit);

		if (*ierr == 0)
		{
			if (matz == 0)
			{
/*      .......... find eigenvalues only .......... */
				tred1(nm, n, a, w, fv1, fv2);
				testNproducts(errorExit);
				tqlrat(n, w, fv2, ierr);
				testNproducts(errorExit);
			} /*if (matz == 0)*/
			else
			{
/*      .......... find both eigenvalues and eigenvectors .......... */
				tred2(nm, n, a, w, fv1, z);
				testNproducts(errorExit);
				tql2(nm, n, w, fv1, z, ierr);
				testNproducts(errorExit);
				if (*ierr == 0)
				{
					rebak(nm, n, b, fv2, n, z);
					testNproducts(errorExit);
				}
			} /*if (matz == 0){}else{}*/
		} /*if (*ierr == 0)*/
	} /*if (n > nm){}else{}*/
	/*fall through*/
  errorExit:
	;

} /*rsg()*/


/* translated from tql2.f */

/*
	From Eispack Library

	Translated to C by C. Bingham,  kb@umnstat.stat.umn.edu  891215
	Compile with -DFORTRAN for fortran storage format
*/

/*
      This subroutine is a translation of the Algol procedure tql2,
      Num. Math. 11, 293-306(1968) by Bowdler, Martin, Reinsch, and
      Wilkinson.
      Handbook for Auto. Comp., Vol.Ii-Linear Algebra, 227-240(1971).


      This subroutine finds the eigenvalues and eigenvectors
      of a symmetric tridiagonal matrix by the ql method.
      The eigenvectors of a full symmetric matrix can also
      be found if  tred2  has been used to reduce this
      full matrix to tridiagonal form.


      On input


         nm must be set to the row dimension (column dimension ifndef
		   FORTRAN) of two-dimensional array parameters as declared in the
		   calling program dimension statement.


         n is the order of the matrix.


         d contains the diagonal elements of the input matrix.


         e contains the subdiagonal elements of the input matrix
           in its last n-1 positions.  e[0] is arbitrary.


         z contains the transformation matrix produced in the
           reduction by  tred2, if performed.  If the eigenvectors
           of the tridiagonal matrix are desired, z must contain
           the identity matrix.


       On output


         d contains the eigenvalues in ascending order.  If an
           error exit is made, the eigenvalues are correct but
           unordered for indices 1,2,...,ierr-1.
           [Eigenvalues in descending order in C-version]

         e has been destroyed.


         z contains orthonormal eigenvectors of the symmetric
           tridiagonal (or full) matrix.  If an error exit is made,
           z contains the eigenvectors associated with the stored
           eigenvalues.


         ierr is a pointer to an integer error number.  *ierr is set to
           zero       for normal return,
           j          if the j-th eigenvalue has not been
                      determined after 30 iterations.


      Calls pythag for dsqrt(a*a + b*b) .


      Questions and comments should be directed to Burton S. Garbow,
      Mathematics and Computer Science Div, Argonne National Laboratory


      This version dated August 1983.


      ------------------------------------------------------------------
*/

void            tql2(long nm, long n, double *d, double *e, double *z,
					 long *ierr)
/*
double d[n],e[n],z[n][nm]; (C storage mode)
double d(n),e(n),z(nm,n) (Fortran storage mode)
*/
{
	long            i, j, k, l, m, ii, l1, l2, i1;
	double          c, c2, c3, dl1, el1, f, g, h, p, r, s, s2;
	double          tst1, tst2;
#ifdef USEPTRS
	double         *zi, *zi1;
#endif /*USEPTRS*/

	*ierr = 0;
	if (n != 1)
	{
		for (i = 1; i < n; i++)
		{
			e[i - 1] = e[i];
		}
		f = 0.0;
		tst1 = 0.0;
		e[n - 1] = 0.0;

		for (l = 0; l < n; l++)
		{
			j = 0;
			h = fabs(d[l]) + fabs(e[l]);
			if (tst1 < h)
			{
				tst1 = h;
			}
/*      .......... look for small sub-diagonal element .......... */
			for (m = l; m < n; m++)
			{
				tst2 = tst1 + fabs(e[m]);
				if (tst2 == tst1)
				{
					break;
				}
/*      .......... e(n-1) is always zero, so there is no exit */
/*                  through the bottom of the loop........... */
			} /*for (m = l; m < n; m++)*/

			if (m != l)
			{
				do
				{
					if (j == 30)
					{
						goto errorExit;
					}
					j++;
/*      .......... form shift .......... */
					l1 = l + 1;
					l2 = l1 + 1;
					g = d[l];
					p = (d[l1] - g) / (2.0 * e[l]);
					r = pythag(p, 1.0);
					d[l] = e[l] / (p + SIGN(r, p));
					d[l1] = e[l] * (p + SIGN(r, p));
					dl1 = d[l1];
					h = g - d[l];
					for (i = l2; i < n; i++)
					{
						d[i] -= h;
					}
					f += h;
/*      .......... ql transformation .......... */
					p = d[m];
					c = 1.0;
					c2 = c;
					el1 = e[l1];
					s = 0.0;
/*      .......... for i=m-1 step -1 until l do -- .......... */
#ifdef USEPTRS
					zi = z + (m - 1)*nm; /*Q2z(,i)*/
					zi1 = z + m*nm; /*Q2z(,i+1)*/
#endif /*USEPTRS*/
					for (i = m - 1; i >= l; i--)
					{
						c3 = c2;
						c2 = c;
						s2 = s;
						g = c * e[i];
						h = c * p;
						r = pythag(p, e[i]);
						e[i + 1] = s * r;
						s = e[i] / r;
						c = p / r;
						p = c * d[i] - s * g;
						d[i + 1] = h + s * (c * g + s * d[i]);
/*      .......... form vector .......... */
						for (k = 0; k < n; k++)
						{
#ifdef USEPTRS
							h = zi1[k];
							zi1[k] = s * zi[k] + c * h;
							zi[k] = c * zi[k] - s * h;
#else /*USEPTRS*/
							h = Q2z(k, i + 1);
							Q2z(k, i + 1) = s * Q2z(k, i) + c * h;
							Q2z(k, i) = c * Q2z(k, i) - s * h;
#endif /*USEPTRS*/
						} /*for (k = 0; k < n; k++)*/
#ifdef USEPTRS
						zi1 -= nm;
						zi -= nm;
#endif /*USEPTRS*/
					} /*for (i = m - 1; i >= l; i--)*/
					p = -s * s2 * c3 * el1 * e[l] / dl1;
					e[l] = s * p;
					d[l] = c * p;
					tst2 = tst1 + fabs(e[l]);
					incNproducts(28 + (m-l)*(34+4*n));
					testNproducts(errorExit);
				} while (tst2 > tst1);
			} /*if (m != l)*/
			d[l] += f;
		} /*for (l = 0; l < n; l++)*/

/*      .......... order eigenvalues and eigenvectors .......... */
#ifdef USEPTRS
		zi = z;
#endif /*USEPTRS*/
		for (ii = 1; ii < n; ii++)
		{
			i = ii - 1;
			i1 = i;
			p = d[i];
			for (j = ii; j < n; j++)
			{
				if (COMPARE(d[j], p))
				{
					i1 = j;
					p = d[j];
				}
			} /*for (j = ii; j < n; j++)*/
			if (i1 != i)
			{
				d[i1] = d[i];
				d[i] = p;
#ifdef USEPTRS
				zi1 = z + i1*nm; /*Q2z(,i1)*/
#endif /*USEPTRS*/
				for (j = 0; j < n; j++)
				{
#ifdef USEPTRS
					p = zi[j];
					zi[j] = zi1[j];
					zi1[j] = p;
#else /*USEPTRS*/
					p = Q2z(j, i);
					Q2z(j, i) = Q2z(j, i1);
					Q2z(j, i1) = p;
#endif /*USEPTRS*/
				}
			} /*if (i1 != i)*/
#ifdef USEPTRS
			zi += nm;
#endif /*USEPTRS*/
		} /*for (ii = 1; ii < n; ii++)*/
	} /*if (n != 1)*/
	return;

/*      .......... set error -- no convergence to an */
/*                 eigenvalue after 30 iterations .......... */
  errorExit:
	*ierr = l;
} /*tql2()*/

/* translated from tqlrat.f */
/*
	From Eispack Library

	Translated to C by C. Bingham,  kb@umnstat.stat.umn.edu  891215
*/

/*
      This subroutine is a translation of the Algol procedure tqlrat,
      Algorithm 464, Comm. Acm 16, 689(1973) by Reinsch.


      This subroutine finds the eigenvalues of a symmetric
      tridiagonal matrix by the rational ql method.


      On input


         n is the order of the matrix.


         d contains the diagonal elements of the input matrix.


         e2 contains the squares of the subdiagonal elements of the
           input matrix in its last n-1 positions.  e2(1) is arbitrary.


       On output


         d contains the eigenvalues in ascending order.  If an
           error exit is made, the eigenvalues are correct and
           ordered for indices 1,2,...ierr-1, but may not be
           the smallest eigenvalues.


         e2 has been destroyed.


         ierr is a pointer to an integer error number.  *ierr is set to
           zero       for normal return,
           j          if the j-th eigenvalue has not been
                      determined after 30 iterations.


      Calls pythag for dsqrt(a*a + b*b) .


      Questions and comments should be directed to Burton S. Garbow,
      Mathematics and Computer Science Div, Argonne National Laboratory


      This version dated August 1983.


      ------------------------------------------------------------------
*/

void            tqlrat(long n, double *d, double *e2, long *ierr)
/*double d[n],e2[n];*/
{
	long            i, j, l, m, l1;
	double          b, c, f, g, h, p, r, s, t;

	*ierr = 0;
	if (n != 1)
	{
		for (i = 1; i < n; i++)
		{
			e2[i - 1] = e2[i];
		}
		f = 0.0;
		t = 0.0;
		e2[n - 1] = 0.0;

		for (l = 0; l < n; l++)
		{
			j = 0;
			h = fabs(d[l]) + sqrt(e2[l]);
			if (t <= h)
			{
				t = h;
				b = epslon(t);
				c = b * b;
			} /*if (t <= h)*/
/*      .......... look for small squared sub-diagonal element .......... */
			for (m = l; m < n; m++)
			{
				if (e2[m] <= c)
				{
					break;
				}
			} /*for (m = l; m < n; m++)*/

			if (m != l)
			{
				do
				{
					if (j == 30)
					{
						goto errorExit;
					}
					j++;
/*      .......... form shift .......... */
					l1 = l + 1;
					s = sqrt(e2[l]);
					g = d[l];
					p = (d[l1] - g) / (2.0 * s);
					r = pythag(p, 1.0);
					d[l] = s / (p + SIGN(r, p));
					h = g - d[l];
					for (i = l1; i < n; i++)
					{
						d[i] -= h;
					}
					f += h;
/*      .......... rational ql transformation .......... */
					g = d[m];
					if (g == 0.0)
					{
						g = b;
					}
					h = g;
					s = 0.0;
/*      .......... for i=m-1 step -1 until l do -- .......... */
					for (i = m - 1; i >= l; i--)
					{
						p = g * h;
						r = p + e2[i];
						e2[i + 1] = s * r;
						s = e2[i] / r;
						d[i + 1] = h + s * (h + d[i]);
						g = d[i] - e2[i] / g;
						if (g == 0.0)
						{
							g = b;
						}
						h = g * p / r;
					} /*for (i = m - 1; i >= l; i--)*/
					e2[l] = s * g;
					d[l] = h;
/*      .......... guard against underflow in convergence test .......... */
					if (h == 0.0)
					{
						break;
					}
					if (fabs(e2[l]) <= fabs(c / h))
					{
						break;
					}
					e2[l] = h * e2[l];
					incNproducts(40 + 7*(m-l));
					testNproducts(errorExit);
				} while (e2[l] != 0.0);
			} /*if (m != l)*/
			p = d[l] + f;
/*      .......... order eigenvalues .......... */
/*      .......... for i=l step -1 until 2 do -- .......... */
			for (i = l; i >= 1; i--)
			{
				if (COMPARE(d[i - 1], p))
				{
					break;
				}
				d[i] = d[i - 1];
			} /*for (i = l; i >= 1; i--)*/
			d[i] = p;
		} /*for (l = 0; l < n; l++)*/
	} /*if (n != 1)*/
	return;
	
  errorExit:
/*      .......... set error -- no convergence to an */
/*                 eigenvalue after 30 iterations .......... */
	*ierr = l;
} /*tqlrat()*/

/* translated from tred1.f */

/*
	From Eispack Library

	Translated to C by C. Bingham,  kb@umnstat.stat.umn.edu  891215
	Compile with -DFORTRAN for fortran storage format
*/


/*
      This subroutine is a translation of the Algol procedure tred1,
      Num. Math. 11, 181-195(1968) by Martin, Reinsch, and Wilkinson.
      Handbook for Auto. Comp., Vol.Ii-Linear Algebra, 212-226(1971).


      This subroutine reduces a real symmetric matrix
      to a symmetric tridiagonal matrix using
      orthogonal similarity transformations.


      On input


         nm must be set to the row dimension (column dimension ifndef
		   FORTRAN) of two-dimensional array parameters as declared in the
		   calling program dimension statement.


         n is the order of the matrix.


         a contains the real symmetric input matrix.  Only the
           lower triangle of the matrix need be supplied.


      On output


         a contains information about the orthogonal trans-
           formations used in the reduction in its strict lower
           triangle.  The full upper triangle of a is unaltered.


         d contains the diagonal elements of the tridiagonal matrix.


         e contains the subdiagonal elements of the tridiagonal
           matrix in its last n-1 positions.  e(1) is set to zero.


         e2 contains the squares of the corresponding elements of e.
           e2 may coincide with e if the squares are not needed.


      Questions and comments should be directed to Burton S. Garbow,
      Mathematics and Computer Science Div, Argonne National Laboratory


      This version dated August 1983.


      ------------------------------------------------------------------
*/

void            tred1(long nm, long n, double *a, double *d, double *e,
					  double *e2)
/*double a[n][nm],d[n],e[n],e2[n];*/
{
	long            i, j, k, l;
	double          f, g, h, scale;
#ifdef USEPTRS
	double         *aj;
#endif /*USEPTRS*/

	for (i = 0; i < n; i++)
	{
		d[i] = Q2a(n - 1, i);
		Q2a(n - 1, i) = Q2a(i, i);
	} /*for (i = 0; i < n; i++)*/

/*      .......... for i=n step -1 until 1 do -- .......... */
	for (i = n - 1; i >= 0; i--)
	{
		l = i - 1;
		h = 0.0;
		scale = 0.0;
		if (l >= 0)
		{
/*      .......... scale row (algol tol then not needed) .......... */
			for (k = 0; k <= l; k++)
			{
				scale += fabs(d[k]);
			}
			
			if (scale == 0.0)
			{
				for (j = 0; j <= l; j++)
				{
					d[j] = Q2a(l, j);
					Q2a(l, j) = Q2a(i, j);
					Q2a(i, j) = 0.0;
				}
			} /*if (scale == 0.0)*/
			else
			{
				for (k = 0; k <= l; k++)
				{
					d[k] /= scale;
					h += d[k] * d[k];
				}
				e2[i] = scale * scale * h;
				f = d[l];
				g = -SIGN(sqrt(h), f);
				e[i] = scale * g;
				h -= f * g;
				d[l] = f - g;
				if (l != 0)
				{
/*      .......... form a*u .......... */
					for (j = 0; j <= l; j++)
					{
						e[j] = 0.0;
					}

#ifdef USEPTRS
					aj = a;
#endif /*USEPTRS*/
					for (j = 0; j <= l; j++)
					{
						f = d[j];
#ifdef USEPTRS
						g = e[j] + aj[j] * f;
#else /*USEPTRS*/
						g = e[j] + Q2a(j, j) * f;
#endif /*USEPTRS*/
						for (k = j+1; k <= l; k++)
						{
#ifdef USEPTRS
							g += aj[k] * d[k];
							e[k] += aj[k] * f;
#else /*USEPTRS*/
							g += Q2a(k, j) * d[k];
							e[k] += Q2a(k, j) * f;
#endif /*USEPTRS*/
						} /*for (k = j+1; k <= l; k++)*/
						e[j] = g;
#ifdef USEPTRS
						aj += nm;
#endif /*USEPTRS*/
					} /*for (j = 0; j <= l; j++)*/
/*      .......... form p .......... */
					f = 0.0;
					for (j = 0; j <= l; j++)
					{
						e[j] /= h;
						f += e[j] * d[j];
					}
					h = f / (h + h);
/*      .......... form q .......... */
					for (j = 0; j <= l; j++)
					{
						e[j] -= h * d[j];
					}
/*      .......... form reduced a .......... */
#ifdef USEPTRS
					aj = a;
#endif /*USEPTRS*/
					for (j = 0; j <= l; j++)
					{
						f = d[j];
						g = e[j];
						for (k = j; k <= l; k++)
						{
#ifdef USEPTRS
							aj[k] -= f * e[k] + g * d[k];
#else /*USEPTRS*/
							Q2a(k, j) -= f * e[k] + g * d[k];
#endif /*USEPTRS*/
						} /*for (k = j; k <= l; k++)*/
#ifdef USEPTRS
						aj += nm;
#endif /*USEPTRS*/
					} /*for (j = 0; j <= l; j++)*/
				} /*if (l != 0)*/

				for (j = 0; j <= l; j++)
				{
					f = d[j];
					d[j] = Q2a(l, j);
					Q2a(l, j) = Q2a(i, j);
					Q2a(i, j) = f * scale;
				} /*for (j = 0; j <= l; j++)*/
				continue;
			} /*if (scale == 0.0){}else{}*/
		} /*if (l >= 0)*/
		e[i] = 0.0;
		e2[i] = 0.0;
		incNproducts(14+(l+1)*(6+2*l));
		testNproducts(errorExit);
	} /*for (i = n - 1; i >= 0; i--)*/
	/*fall through*/

  errorExit:
	;
} /*tred1()*/

/* translated from tred2.f */

/*
	From Eispack Library

	Translated to C by C. Bingham,  kb@umnstat.stat.umn.edu  891215
	Compile with -DFORTRAN for fortran storage format
*/


/*
      This subroutine is a translation of the Algol procedure tred2,
      Num. Math. 11, 181-195(1968) by Martin, Reinsch, and Wilkinson.
      Handbook for Auto. Comp., Vol.Ii-Linear Algebra, 212-226(1971).


      This subroutine reduces a real symmetric matrix to a
      symmetric tridiagonal matrix using and accumulating
      orthogonal similarity transformations.


      On input


         nm must be set to the row dimension (column dimension ifndef
		   FORTRAN) of two-dimensional array parameters as declared in the
		   calling program dimension statement.


         n is the order of the matrix.


         a contains the real symmetric input matrix.  Only the
           lower triangle of the matrix need be supplied.


      On output


         d contains the diagonal elements of the tridiagonal matrix.


         e contains the subdiagonal elements of the tridiagonal
           matrix in its last n-1 positions.  e(1) is set to zero.


         z contains the orthogonal transformation matrix
           produced in the reduction.


         a and z may coincide.  If distinct, a is unaltered.


      Questions and comments should be directed to Burton S. Garbow,
      Mathematics and Computer Science Div, Argonne National Laboratory


      This version dated August 1983.


      ------------------------------------------------------------------
*/

void            tred2(long nm, long n, double *a, double *d, double *e,
					  double *z)
/*double a[n][nm],d[n],e[n],z[n][nm];*/
{
	long            i, j, k, l;
	double          f, g, h, hh, scale;
#ifdef USEPTRS
	double         *zi, *zj, *zl, *ai;
#endif /*USEPTRS*/

#ifdef USEPTRS
	ai = a;
	zi = z;
#endif /*USEPTRS*/
	for (i = 0; i < n; i++)
	{
		for (j = i; j < n; j++)
		{
#ifdef USEPTRS
			zi[j] = ai[j];
#else /*USEPTRS*/
			Q2z(j, i) = Q2a(j, i);
#endif /*USEPTRS*/
		}
#ifdef USEPTRS
		d[i] = ai[n - 1];
		ai += nm;
		zi += nm;
#else /*USEPTRS*/
		d[i] = Q2a(n - 1, i);
#endif /*USEPTRS*/
	} /*for (i = 0; i < n; i++)*/

/*      .......... for i=n step -1 until 2 do -- .......... */
#ifdef USEPTRS
	zi = z + (n - 1)*nm;
#endif /*USEPTRS*/
	for (i = n - 1; i >= 1; i--)
	{
		l = i - 1;
		h = 0.0;
		scale = 0.0;
		if (l >= 1)
		{
/*      .......... scale row (algol tol then not needed) .......... */
			for (k = 0; k <= l; k++)
			{
				scale += fabs(d[k]);
			}
			if (scale != 0.0)
			{
				for (k = 0; k <= l; k++)
				{
					d[k] /= scale;
					h += d[k] * d[k];
				}
				f = d[l];
				g = -SIGN(sqrt(h), f);
				e[i] = scale * g;
				h -= f * g;
				d[l] = f - g;
/*      .......... form a*u .......... */
				for (j = 0; j <= l; j++)
				{
					e[j] = 0.0;
				}
#ifdef USEPTRS
				zj = z;
#endif /*USEPTRS*/
				for (j = 0; j <= l; j++)
				{
					f = d[j];
#ifdef USEPTRS
					zi[j] = f;
					g = e[j] + zj[j] * f;
#else /*USEPTRS*/
					Q2z(j, i) = f;
					g = e[j] + Q2z(j, j) * f;
#endif /*USEPTRS*/
					for (k = j+1; k <= l; k++)
					{
#ifdef USEPTRS
						g += zj[k] * d[k];
						e[k] += zj[k] * f;
#else /*USEPTRS*/
						g += Q2z(k, j) * d[k];
						e[k] += Q2z(k, j) * f;
#endif /*USEPTRS*/
					} /*for (k = j+1; k <= l; k++)*/
					e[j] = g;
#ifdef USEPTRS
					zj += nm;
#endif /*USEPTRS*/
					incNproducts(l-j+1);
					testNproducts(errorExit);
				} /*for (j = 0; j <= l; j++)*/

/*      .......... form p .......... */
				f = 0.0;
				for (j = 0; j <= l; j++)
				{
					e[j] /= h;
					f += e[j] * d[j];
				}

				hh = f / (h + h);
/*      .......... form q .......... */
				for (j = 0; j <= l; j++)
				{
					e[j] -= hh * d[j];
				}
/*      .......... form reduced a .......... */
#ifdef USEPTRS
				zj = z; /*Q2z(,j)*/
#endif /*USEPTRS*/
				for (j = 0; j <= l; j++)
				{
					f = d[j];
					g = e[j];
					for (k = j; k <= l; k++)
					{
#ifdef USEPTRS
						zj[k] -= f * e[k] + g * d[k];
#else /*USEPTRS*/
						Q2z(k, j) -= f * e[k] + g * d[k];
#endif /*USEPTRS*/
					}
#ifdef USEPTRS
					d[j] = zj[l];
					zj[i] = 0.0;
					zj += nm;
#else /*USEPTRS*/
					d[j] = Q2z(l, j);
					Q2z(i, j) = 0.0;
#endif /*USEPTRS*/
					incNproducts(2*i);
					testNproducts(errorExit);
				} /*for (j = 0; j <= l; j++)*/
				d[i] = h;
#ifdef USEPTRS
				zi -= nm;
#endif /*USEPTRS*/
				continue;
			} /*if (scale != 0.0)*/
		} /*if (l >= 1)*/
		e[i] = d[l];
#ifdef USEPTRS
		for (j = 0; j <= l; j++)
		{
			d[j] = Q2z(l, j);
			Q2z(i, j) = 0.0;
			zi[j] = 0.0;
		} /*for (j = 0; j <= l; j++)*/
#else /*USEPTRS*/
		for (j = 0; j <= l; j++)
		{
			d[j] = Q2z(l, j);
			Q2z(i, j) = 0.0;
			Q2z(j, i) = 0.0;
		} /*for (j = 0; j <= l; j++)*/
#endif /*USEPTRS*/
		d[i] = h;
#ifdef USEPTRS
		zi -= nm;
#endif /*USEPTRS*/
	} /*for (i = n - 1; i >= 1; i--)*/

/*      .......... accumulation of transformation matrices .......... */
#ifdef USEPTRS
	zl = z; /*Q2z(,l)*/
	zi = z + nm;
#endif /*USEPTRS*/
	for (i = 1; i < n; i++)
	{
		l = i - 1;
#ifdef USEPTRS
		zl[n - 1] = zl[l];
		zl[l] = 1.0;
#else /*USEPTRS*/
		Q2z(n - 1, l) = Q2z(l, l);
		Q2z(l, l) = 1.0;
#endif /*USEPTRS*/
		h = d[i];
		if (h != 0.0)
		{
			for (k = 0; k <= l; k++)
			{
#ifdef USEPTRS
				d[k] = zi[k] / h;
#else /*USEPTRS*/
				d[k] = Q2z(k, i) / h;
#endif /*USEPTRS*/
			} /*for (k = 0; k <= l; k++)*/

#ifdef USEPTRS
			zj = z; /*Q2z(,j)*/
#endif /*USEPTRS*/
			for (j = 0; j <= l; j++)
			{
				g = 0.0;
				for (k = 0; k <= l; k++)
				{
#ifdef USEPTRS
					g += zi[k] * zj[k];
#else /*USEPTRS*/
					g += Q2z(k, i) * Q2z(k, j);
#endif /*USEPTRS*/
				}
				for (k = 0; k <= l; k++)
				{
#ifdef USEPTRS
					zj[k] -= g * d[k];
#else /*USEPTRS*/
					Q2z(k, j) -= g * d[k];
#endif /*USEPTRS*/
				} /*for (k = 0; k <= l; k++)*/
#ifdef USEPTRS
				zj += nm;
#endif /*USEPTRS*/
				incNproducts(2*i);
				testNproducts(errorExit);
			} /*for (j = 0; j <= l; j++)*/
		} /*if (h != 0.0)*/

#ifdef USEPTRS
		for (k = 0; k <= l; k++)
		{
			zi[k] = 0.0;
		}
		zl += nm;
		zi += nm;
#else /*USEPTRS*/
		for (k = 0; k <= l; k++)
		{
			Q2z(k, i) = 0.0;
		}
#endif /*USEPTRS*/
	} /*for (i = 1; i < n; i++)*/

	for (i = 0; i < n; i++)
	{
		d[i] = Q2z(n - 1, i);
		Q2z(n - 1, i) = 0.0;
	} /*for (i = 0; i < n; i++)*/

	Q2z(n - 1, n - 1) = 1.0;

	e[0] = 0.0;

/* fall through*/
  errorExit:
	;

} /*tred2()*/


#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment Svd
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#ifdef FORTRAN
#define Q2u(I,J) u[(I) + nm*(J)]
#define Q2v(I,J) v[(I) + n*(J)]
#else
#define Q2u(I,J) u[(J) + nm*(I)]
#define Q2v(I,J) v[(J) + n*(I)]
#endif

/*
      This subroutine is a translation of the Algol procedure svd,
      Num. Math. 14, 403-420(1970) by Golub and Reinsch.
      Handbook for Auto. Comp., Vol Ii-Linear Algebra, 134-151(1971).

      This subroutine determines the singular value decomposition
           t
      a=usv  of a real m by n rectangular matrix.  Householder
      bidiagonalization and a variant of the QR algorithm are used.

      On input

         nm must be set to the row dimension of two-dimensional
           array parameters as declared in the calling program
           dimension statement.  Note that nm must be at least
           as large as the maximum of m and n.
           [in this version nm is the row dimension of a and u, but not v;
            v is assumed to have row dimension n]

         m is the number of rows of a (and u).

         n is the number of columns of a (and u) and the order of v.

         a contains the rectangular input matrix to be decomposed.

         matu should be set to .true. if the u matrix in the
           decomposition is desired, and to .false. otherwise.

         matv should be set to .true. if the v matrix in the
           decomposition is desired, and to .false. otherwise.

      On output

         a is unaltered (unless overwritten by u or v).

         w contains the n (non-negative) singular values of a (the
           diagonal elements of s).  They are unordered.  If an
           error exit is made, the singular values should be correct
           for indices ierr+1,ierr+2,...,n.
           [In this C version, the singular values are reordered in
            decreasing order and the left & right singular vectors
            correspondingly rearranged]

         u contains the matrix u (orthogonal column vectors) of the
           decomposition if matu has been set to .true.  Otherwise
           u is used as a temporary array.  u may coincide with a.
           If an error exit is made, the columns of u corresponding
           to indices of correct singular values should be correct.

         v contains the matrix v (orthogonal) of the decomposition if
           matv has been set to .true.  Otherwise v is not referenced.
           v may also coincide with a if u is not needed.  If an error
           exit is made, the columns of v corresponding to indices of
           correct singular values should be correct.

         ierr is set to
           zero       for normal return,
           k          if the k-th singular value has not been
                      determined after 30 iterations.

         rv1 is a temporary storage array.

      Calls pythag for  sqrt(a*a + b*b).

      Questions and comments should be directed to Burton S. Garbow,
      Mathematics and Computer Science Div, Argonne National Laboratory

      This version dated August 1983.

      ------------------------------------------------------------------
*/

/*
  Modified so that v is assumed to have to have row dimension n rather than
  nm.
  Note that nm is used by macro Q2u(I,J).
*/

void svd(long nm, long m, long n, double *a, double *w,
			 long matu, double *u, long matv, double *v, double *rv1,
			 long *ierr)
/* a(nm,n), w(n), u(nm,n), v(nm,n), rv1(n)  in original Fortran */
{
	int ii, i, j, k, l, i1, k1, l1, mn, its;
	double c, f, g, h, s, x, y, z, tst1, tst2, scale;
#ifdef USEPTRS
	double       *aj;                      /*Q2a(,j)*/
	double       *ui, *uj, *uk, *ui1, *ul1;/*Q2u(,i),Q2u(,j), etc.*/
	double       *vi, *vj, *vk, *vi1;      /*Q2v(,i),Q2v(,j), etc.*/
#endif /*USEPTRS*/
	
#ifdef USEPTRS
	uj = u;
	aj = a;
	for (j = 0;j < n ;j++)
	{
		for (i = 0;i < m ;i++)
		{
			uj[i] = aj[i];
		} /*for (i = 0;i < m ;i++)*/
		uj += nm;
		aj += nm;
	} /*for (j = 0;j < n ;j++)*/
#else /*USEPTRS*/
	for (i = 0;i < m ;i++)
	{
		for (j = 0;j < n ;j++)
		{
			Q2u(i,j) = Q2a(i,j);
		}
	} /*for (i = 0;i < m ;i++)*/
#endif /*USEPTRS*/

/*      .......... Householder reduction to bidiagonal form .......... */
	g = 0.0;
	scale = 0.0;
	x = 0.0;

#ifdef USEPTRS
	ui  = u; /*Q2u(,i)*/
#endif /*USEPTRS*/
	for (i = 0;i < n ;i++)
	{
		l = i + 1;
		rv1[i] = scale*g;
		g = 0.0;
		s = 0.0;
		scale = 0.0;
		if (i < m) 
		{
			for (k = i;k < m ;k++)
			{
#ifdef USEPTRS
				scale += fabs(ui[k]);
#else /*USEPTRS*/
				scale += fabs(Q2u(k,i));
#endif /*USEPTRS*/
			} /*for (k = i;k < m ;k++)*/

			if (scale != 0.0) 
			{
				for (k = i;k < m ;k++)
				{
#ifdef USEPTRS
					ui[k] /= scale;
					s += ui[k]*ui[k];
#else /*USEPTRS*/
					Q2u(k,i) /= scale;
					s += Q2u(k,i)*Q2u(k,i);
#endif /*USEPTRS*/
				} /*for (k = i;k < m ;k++)*/
				incNproducts(2*(m-i));
#ifdef USEPTRS
				f = ui[i];
#else /*USEPTRS*/
				f = Q2u(i,i);
#endif /*USEPTRS*/
				g = -SIGN(sqrt(s),f);
				h = f*g - s;
#ifdef USEPTRS
				ui[i] = f - g;
				uj = u + l*nm;
#else /*USEPTRS*/
				Q2u(i,i) = f - g;
#endif /*USEPTRS*/
				for (j = l;j < n ;j++)
				{
					s = 0.0;
					for (k = i;k < m ;k++)
					{
#ifdef USEPTRS
						s += ui[k] * uj[k];
#else /*USEPTRS*/
						s += Q2u(k,i)*Q2u(k,j);
#endif /*USEPTRS*/
					} /*for (k = i;k < m ;k++)*/
					f = s/h;

					for (k = i;k < m ;k++)
					{
#ifdef USEPTRS
						uj[k] += f*ui[k];
#else /*USEPTRS*/
						Q2u(k,j) += f*Q2u(k,i);
#endif /*USEPTRS*/
					} /*for (k = i;k < m ;k++)*/
#ifdef USEPTRS
					uj += nm;
#endif /*USEPTRS*/
				} /*for (j = l;j < n ;j++)*/

				for (k = i;k < m ;k++)
				{
#ifdef USEPTRS
					ui[k] *= scale;
#else /*USEPTRS*/
					Q2u(k,i) *= scale;
#endif /*USEPTRS*/
				} /*for (k = i;k < m ;k++)*/
				incNproducts((2*(n-l)+1)*(m-i));
				testNproducts(errorExit);
			} /*if (scale != 0.0) */
		} /*if (i < m) */

		w[i] = scale*g;
		g = 0.0;
		s = 0.0;
		scale = 0.0;
		if (i < m && i != n - 1) 
		{
			for (k = l;k < n ;k++)
			{
				scale += fabs(Q2u(i,k));
			} /*for (k = l;k < n ;k++)*/

			if (scale != 0.0) 
			{
				for (k = l;k < n ;k++)
				{
					Q2u(i,k) /= scale;
					s += Q2u(i,k)*Q2u(i,k);
				} /*for (k = l;k < n ;k++)*/

				f = Q2u(i,l);
				g = -SIGN(sqrt(s),f);
				h = f*g - s;
				Q2u(i,l) = f - g;

				for (k = l;k < n ;k++)
				{
					rv1[k] = Q2u(i,k)/h;
				} /*for (k = l;k < n ;k++)*/
				for (j = l;j < m ;j++)
				{
					s = 0.0;

					for (k = l;k < n ;k++)
					{
						s += Q2u(j,k)*Q2u(i,k);
					} /*for (k = l;k < n ;k++)*/
					
					for (k = l;k < n ;k++)
					{
						Q2u(j,k) += s*rv1[k];
					} /*for (k = l;k < n ;k++)*/
				} /*for (j = l;j < m ;j++)*/

				for (k = l;k < n ;k++)
				{
					Q2u(i,k) *= scale;
				} /*for (k = l;k < n ;k++)*/
				incNproducts((4+2*(m-l))*(n-l));
				testNproducts(errorExit);
			} /*if (scale != 0.0) */
		} /*if (i < m && i != n - 1) */

		x = MAX(x,fabs(w[i])+fabs(rv1[i]));
#ifdef USEPTRS
		ui += nm;
#endif /*USEPTRS*/
	} /*for (i = 0;i < n ;i++)*/

/*      .......... accumulation of right-hand transformations .......... */
	if (matv)
	{
/*      .......... for i=n step -1 until 1 do -- .......... */
#ifdef USEPTRS
		vi = v + (n - 1)*n;
#endif /*USEPTRS*/
		for (i = n - 1;i >= 0;i--)
		{
			l = i + 1;
			if (i != n - 1)
			{
				if (g != 0.0) 
				{
					for (j = l;j < n ;j++)
					{
/*      .......... double division avoids possible underflow .......... */
#ifdef USEPTRS
						vi[j] = (Q2u(i,j)/Q2u(i,l))/g;
#else /*USEPTRS*/
						Q2v(j,i) = (Q2u(i,j)/Q2u(i,l))/g;
#endif /*USEPTRS*/
					} /*for (j = l;j < n ;j++)*/
#ifdef USEPTRS
					vj = v + l*n; /*Q2v(,j)*/
#endif /*USEPTRS*/
					for (j = l;j < n ;j++)
					{ 
						s = 0.0;

						for (k = l;k < n ;k++)
						{
#ifdef USEPTRS
							s += Q2u(i,k)*vj[k];
#else /*USEPTRS*/
							s += Q2u(i,k)*Q2v(k,j);
#endif /*USEPTRS*/						
						}
						for (k = l;k < n ;k++)
						{
#ifdef USEPTRS
							vj[k] += s*vi[k];
#else /*USEPTRS*/
							Q2v(k,j) += s*Q2v(k,i);
#endif /*USEPTRS*/						
						}
#ifdef USEPTRS
						vj += n;
#endif /*USEPTRS*/
					} /*for (j = l;j < n ;j++)*/
					incNproducts(2*(n-l+2)*(n-l));
				} /*if (g != 0.0) */

				for (j = l;j < n ;j++)
				{
					Q2v(i,j) = 0.0;
#ifdef USEPTRS
					vi[j] = 0.0;
#else /*USEPTRS*/
					Q2v(j,i) = 0.0;
#endif /*USEPTRS*/
				}
			} /*if (i != n - 1) */

#ifdef USEPTRS
			vi[i] = 1.0;
			vi -= n;
#else /*USEPTRS*/
			Q2v(i,i) = 1.0;
#endif /*USEPTRS*/
			g = rv1[i];
		} /*for (i = n - 1;i >= 0;i--)*/
	} /*if (matv)*/
	
/*      .......... accumulation of left-hand transformations .......... */
	if (matu) 
	{
/*      ..........for i = min(m,n) step -1 until 1 do -- .......... */
		mn = (m < n) ? m : n;
#ifdef USEPTRS
		ui = u + (mn - 1)*nm;
#endif /*USEPTRS*/
		for (i = mn - 1;i >= 0;i--)
		{
			l = i + 1;
			g = w[i];
			for (j = l;j < n ;j++)
			{
				Q2u(i,j) = 0.0;
			}
			
			if (g == 0.0)
			{
				for (j = i;j < m ;j++) 
				{
#ifdef USEPTRS
					ui[j] = 0.0;
#else /*USEPTRS*/
					Q2u(j,i) = 0.0;
#endif /*USEPTRS*/
				}
			} /*if (g == 0.0)*/
			else 
			{
#ifdef USEPTRS
				uj = u + l*nm;
#endif /*USEPTRS*/
				for (j = l;j < n ;j++)
				{ 
					s = 0.0;

					for (k = l;k < m ;k++)
					{
#ifdef USEPTRS
						s += ui[k]*uj[k];
#else /*USEPTRS*/
						s += Q2u(k,i)*Q2u(k,j);
#endif /*USEPTRS*/
					} /*for (k = l;k < m ;k++)*/
/*      .......... double division avoids possible underflow .......... */
#ifdef USEPTRS
					f = (s/ui[i])/g;
#else /*USEPTRS*/
					f = (s/Q2u(i,i))/g;
#endif /*USEPTRS*/

					for (k = i;k < m ;k++)
					{
#ifdef USEPTRS
						uj[k] += f*ui[k];
#else /*USEPTRS*/
						Q2u(k,j) += f*Q2u(k,i);
#endif /*USEPTRS*/
					} /*for (k = i;k < m ;k++)*/
#ifdef USEPTRS
					uj += nm;
#endif /*USEPTRS*/
				} /*for (j = l;j < n ;j++)*/

				for (j = i;j < m ;j++)
				{
#ifdef USEPTRS
					ui[j] /= g;
#else /*USEPTRS*/
					Q2u(j,i) /= g;
#endif /*USEPTRS*/
				}
				incNproducts((n-l)*(m-l+m-i)+m-i);
				testNproducts(errorExit);
			} /*if (g == 0.0){}else{}*/
#ifdef USEPTRS
			ui[i] += 1.0;
			ui -= nm;
#else /*USEPTRS*/
			Q2u(i,i) += 1.0;
#endif /*USEPTRS*/
		} /*for (i = mn - 1;i >= 0;i--)*/
	} /*if (matu) */
/*      .......... diagonalization of the bidiagonal form .......... */
	tst1 = x;
/*      .......... for k=n step -1 until 1 do -- .......... */
#ifdef USEPTRS
	vk = v + (n - 1) * n;
#endif /*USEPTRS*/
	for (k = n - 1;k >= 0;k--)
	{
		k1 = k - 1;
		its = 0;
		while (1)
		{/*      .......... test for splitting. */
/*                 for l=k step -1 until 1 do -- .......... */
			for (l = k;l >= 0;l--)
			{
				l1 = l - 1;
				tst2 = tst1 + fabs(rv1[l]);
				if (tst2 == tst1)
				{
					goto LAB10;
				}
/*      .......... rv1[0] is always zero, so there is no exit */
/*                 through the bottom of the loop .......... */
				tst2 = tst1 + fabs(w[l1]);
				if (tst2 == tst1)
				{
					break;
				}
			} /*for (l = k;l >= 0;l--)*/

/*      .......... cancellation of rv1[l] if l greater than 1 .......... */
			c = 0.0;
			s = 1.0;
#ifdef USEPTRS
			ui = u + l*nm;
#endif /*USEPTRS*/
			for (i = l;i <= k ;i++)
			{
				f = s*rv1[i];
				rv1[i] = c*rv1[i];
				tst2 = tst1 + fabs(f);
				if (tst2 == tst1)
				{
					break;
				}
				g = w[i];
				h = pythag(f,g);
				w[i] = h;
				c = g/h;
				s = -f/h;
				if (matu)
				{
#ifdef USEPTRS
					ul1 = u + l1*nm;
#endif /*USEPTRS*/
					for (j = 0;j < m ;j++)
					{
#ifdef USEPTRS
						y = ul1[j];
						z = ui[j];
						ul1[j] = y*c + z*s;
						ui[j] = -y*s + z*c;
#else /*USEPTRS*/
						y = Q2u(j,l1);
						z = Q2u(j,i);
						Q2u(j,l1) = y*c + z*s;
						Q2u(j,i) = -y*s + z*c;
#endif /*USEPTRS*/
					} /*for (j = 0;j < m ;j++)*/
				} /*if (matu)*/
#ifdef USEPTRS
				ui += nm;
#endif /*USEPTRS*/
			} /*for (i = l;i <= k ;i++)*/
			incNproducts((28 + ((matu) ? 4*m : 0))*(k-l+1));
			testNproducts(errorExit);
/*      .......... test for convergence .......... */
		  LAB10:
			z = w[k];
			if (l == k)
			{
				break;
			}
/*      .......... shift from bottom 2 by 2 minor .......... */
			if (its == 30)
			{
				goto errorExit;
			}
			its++;
			x = w[l];
			y = w[k1];
			g = rv1[k1];
			h = rv1[k];
			f = 0.5*(((g + z)/h)*((g - z)/y) + y/h - h/y);
			g = pythag(f,1.0);
			f = x - (z/x)*z + (h/x)*(y/(f + SIGN(g,f)) - h);
/*      .......... next qr transformation .......... */
			c = 1.0;
			s = 1.0;

			
#ifdef USEPTRS
			ui1 = u + l*nm;
			vi1 = v + l*n;
#endif /*USEPTRS*/
			for (i1 = l;i1 <= k1 ;i1++)
			{
				i = i1 + 1;
#ifdef USEPTRS
				ui = ui1 + nm;
				vi = vi1 + n;
#endif /*USEPTRS*/
				g = rv1[i];
				y = w[i];
				h = s*g;
				g = c*g;
				z = pythag(f,h);
				rv1[i1] = z;
				c = f/z;
				s = h/z;
				f = x*c + g*s;
				g =  -x*s + g*c;
				h = y*s;
				y = y*c;
				if (matv)
				{
					for (j = 0;j < n ;j++)
					{
#ifdef USEPTRS
						x = vi1[j];
						z = vi[j];
						vi1[j] = x*c + z*s;
						vi[j] = -x*s + z*c;
#else /*USEPTRS*/
						x = Q2v(j,i1);
						z = Q2v(j,i);
						Q2v(j,i1) = x*c + z*s;
						Q2v(j,i) = -x*s + z*c;
#endif /*USEPTRS*/
					} /*for (j = 0;j < n ;j++)*/
				} /*if (matv)*/
				
				z = pythag(f,h);
				w[i1] = z;
/*      .......... rotation can be arbitrary if z is zero .......... */
				if (z != 0.0) 
				{
					c = f/z;
					s = h/z;
				} /*if (z != 0.0) */
				f = c*g + s*y;
				x = -s*g + c*y;
				if (matu)
				{
					for (j = 0;j < m ;j++)
					{
#ifdef USEPTRS
						y = ui1[j];
						z = ui[j];
						ui1[j] = y*c + z*s;
						ui[j] = -y*s + z*c;
#else /*USEPTRS*/
						y = Q2u(j,i1);
						z = Q2u(j,i);
						Q2u(j,i1) = y*c + z*s;
						Q2u(j,i) = -y*s + z*c;
#endif /*USEPTRS*/
					} /*for (j = 0;j < m ;j++)*/
				} /*if (matu)*/
#ifdef USEPTRS
				ui1 += nm;
				vi1 += n;
#endif /*USEPTRS*/
			}/*for (i1 = l;i1 <= k1 ;i1++)*/

			rv1[l] = 0.0;
			rv1[k] = f;
			w[k] = x;
			incNproducts(35 + (k1-l+1)*(64 + ((matv) ? n*4 : 0)));
			testNproducts(errorExit);
		} /*while (1)*/
/*      .......... convergence .......... */
		if (z < 0.0) 
		{
/*      .......... w[k] is made non-negative .......... */
			w[k] = -z;
			if (matv)
			{
				for (j = 0;j < n ;j++) 
				{
#ifdef USEPTRS
					vk[j] = -vk[j];
#else /*USEPTRS*/
					Q2v(j,k) = -Q2v(j,k);
#endif /*USEPTRS*/
				} /*for (j = 0;j < n ;j++) */
			} /*if (matv)*/
		} /*if (z < 0.0) */
#ifdef USEPTRS
		vk -= n;
#endif /*USEPTRS*/
	} /*for (k = n - 1;k >= 0;k--)*/

/*      .......... order singular values and singular vectors (added by kb) */
#ifdef USEPTRS
	ui = u;
	vi = v;
#endif /*USEPTRS*/
	for (ii = 1; ii < n; ii++)
	{
		i = ii - 1;
		k = i;
		c = w[i];
		for (j = ii; j < n; j++)
		{
			if (COMPARE(w[j], c))
/*			if (w[j] > c)*/
			{
				k = j;
				c = w[j];
			}
		} /*for (j = ii; j < n; j++)*/

		if (k != i)
		{
			w[k] = w[i];
			w[i] = c;
			if (matu)
			{
#ifdef USEPTRS
				uk = u + k*nm;
#endif /*USEPTRS*/
				for (j = 0; j < m; j++)
				{
#ifdef USEPTRS
					c = ui[j];
					ui[j] = uk[j];
					uk[j] = c;
#else /*USEPTRS*/
					c = Q2u(j, i);
					Q2u(j, i) = Q2u(j, k);
					Q2u(j, k) = c;
#endif /*USEPTRS*/
				} /*for (j = 0; j < m; j++)*/
			} /*if (matu)*/
			
			if (matv)
			{
#ifdef USEPTRS
				vk = v + k * n;
#endif /*USEPTRS*/
				for (j = 0; j < n; j++)
				{
#ifdef USEPTRS
					c = vi[j];
					vi[j] = vk[j];
					vk[j] = c;
#else /*USEPTRS*/
					c = Q2v(j, i);
					Q2v(j, i) = Q2v(j, k);
					Q2v(j, k) = c;
#endif /*USEPTRS*/
				} /*for (j = 0; j < n; j++)*/
			} /*if (matv)*/
			
		} /*if (k != i)*/
#ifdef USEPTRS
		ui += nm;
		vi += n;
#endif /*USEPTRS*/
	} /*for (ii = 1; ii < n; ii++)*/

	k = 0;
	/* fall through */
	
/*
      .......... set error -- no convergence to a
                 singular value after 30 iterations ..........
*/
  errorExit:
	*ierr = k;
	
} /*svd()*/


#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment Trideige
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


/* Routines used by trideigen() */

/*
	Translated to C by C. Bingham (kb@stat.umn.edu) 950516
*/

/*
 ***Begin prologue  tridib
 ***Date written   760101   (yymmdd)
 ***Revision date  830518   (yymmdd)
 ***Category No.  D4A5,D4C2A
 ***Keywords  eigenvalues,eigenvectors,eispack
 ***Author  Smith, B. T., et al.
 ***Purpose  computes eigenvalues of symmetric tridiagonal matrix
             given interval using Sturm sequencing.
 ***Description

      This subroutine is a translation of the ALGOL procedure bisect,
      Num. Math. 9, 386-393(1967) by Barth, Martin, and Wilkinson.
      Handbook for Auto. Comp., Vol.II-Linear Algebra, 249-256(1971).

      This subroutine finds those eigenvalues of a tridiagonal
      symmetric matrix between specified boundary indices,
      using bisection.

      On input

         n is the order of the matrix.

         eps1 is an absolute error tolerance for the computed
           eigenvalues.  if the input eps1 is non-positive,
           it is reset for each submatrix to a default value,
           namely, minus the product of the relative machine
           precision and the 1-norm of the submatrix.

         d contains the diagonal elements of the input matrix.

         e contains the subdiagonal elements of the input matrix
           in its last n-1 positions.  e(1) is arbitrary.

         e2 contains the squares of the corresponding elements of e.
           e2(1) is arbitrary.

         m11 specifies the lower boundary index for the desired
           eigenvalues.

         m specifies the number of eigenvalues desired.  the upper
           boundary index m22 is then obtained as m22=m11+m-1.

      on output

         eps1 is unaltered unless it has been reset to its
           (last) default value.

         d and e are unaltered.

         elements of e2, corresponding to elements of e regarded
           as negligible, have been replaced by zero causing the
           matrix to split into a direct sum of submatrices.
           e2(1) is also set to zero.

         lb and ub define an interval containing exactly the desired
           eigenvalues.

         w contains, in its first m positions, the eigenvalues
           between indices m11 and m22 in ascending order.

         ind contains in its first m positions the submatrix indices
           associated with the corresponding eigenvalues in w --
           1 for eigenvalues belonging to the first submatrix from
           the top, 2 for those belonging to the second submatrix, etc.

         ierr is set to
           zero       for normal return,
           3*n+1      if multiple eigenvalues at index m11 make
                      unique selection impossible,
           3*n+2      if multiple eigenvalues at index m22 make
                      unique selection impossible.

         rv4 and rv5 are temporary storage arrays.

      Note that subroutine tql1, imtql1, or tqlrat is generally faster
      than tridib, if more than n/4 eigenvalues are to be found.

      Questions and comments should be directed to B. S. Garbow,
      Applied Mathematics Division, Argonne National Laboratory
      ------------------------------------------------------------------
 ***References  B. T. Smith, J. M. Boyle, J. J. Dongarra, B. S. Garbow,
                  Y. Ikebe, V. C. Klema, C. B. Moler, *Matrix Eigen-
                  System Routines - EISPACK Guide*, Springer-Verlag,
                  1976.
 ***routines called  (none)
 ***end prologue  tridib
*/

/*
  Comments on C-version:
   All indices now have 0 origin.  This includes i, j, p, q, r, s, m1, m2

   Also argument m11 is assumed relative to 0 origin

   Many of the original line numbers are present in comments of the form
   #nnn at the end of a line

*/

void tridib(long n, double *eps1, double *d, double *e, double *e2,
			double *lb,double *ub, long m11, long m, double *w,
			long *ind, long *ierr, double *rv4, double *rv5)
/*double d[n],e[n],e2[n],w[m],rv4[n],rv5[n];*/
/*int    ind[m]*/
{
	long            i, j, k, l, p, q, r, s, m1, m2, m22, tag, isturm = 0;
	int            breaker = 0, continuer = 0;
	double         u, v, t1, t2, xu, x0, x1, s1, s2, kluge;
	static double  machep = 1.0;

	/* ***first executable statement  tridib */
	if (machep == 1.0) 
	{
/*
   Note: kluge is used to avoid problem of register arithmetic have more
   significant digits that double can hold, as on Macintosh
*/
#ifdef DJGPP
		machep = epslon(1.0);
#else /*DJGPP*/
		do
		{
			machep *= 0.5; /*#5*/
			kluge = 1.0 + machep;
		} while (kluge > 1.0);
#endif /*DJGPP*/
		machep *= machep;
	} /*if (machep == 1.0) */
	*ierr = 0; /*#10*/
	tag = 0;
	xu = d[0];
	x0 = d[0];
	u = 0.0;

/*
      .......... Look for small sub-diagonal entries and determine an
                 interval containing all the eigenvalues ..........
*/
	for (i=0;i < n ;i++)
	{
		x1 = u;
		u = (i == n - 1) ? 0.0 : fabs(e[i + 1]);
		xu = MIN(d[i] - (x1 + u), xu);
		x0 = MAX(d[i] + (x1 + u), x0);
		if (i > 0) 
		{
			s1 = fabs(d[i]) + fabs(d[i - 1]);
			s2 = s1 + fabs(e[i]);
			if (s2 > s1)
			{
				continue;
			}
		} /*if (i > 0) */
		e2[i] = 0.0; /*#20*/
	} /*for (i=0;i < n ;i++)*/

	x1 = MAX(fabs(xu), fabs(x0))*machep*(double) n;
	xu -= x1;
	t1 = xu;
	x0 += x1;
	t2 = x0;
/*
      .......... Determine an interval containing exactly
                 the desired eigenvalues ..........
*/
	p = 0;
	q = n-1;
	m1 = m11 - 1;
	if (m1 >= 0) 
	{
		isturm = 1;
		goto errorCheck;
	}

	while (1)
	{
		m22 = m1 + m; /*#75*/
		if (m22 != n - 1) 
		{
			x0 = t2;
			isturm = 2;
			goto errorCheck;
		} /*if (m22 != n) */

		while (2)
		{
			q = -1; /*#90*/
			r = -1;
			while (3)
			{
/*
      .......... Establish and process next submatrix, refining
                 interval by the Gerschgorin bounds ..........
*/
				if (r == m-1) /*#100*/
				{
					goto exitPoint;
				}
				tag++;
				p = q + 1;
				xu = d[p];
				x0 = d[p];
				u = 0.0;

				for (q=p;q < n ;q++)
				{
					x1 = u;
					u = 0.0;
					v = 0.0;
					if (q < n - 1) 
					{
						u = fabs(e[q + 1]);
						v = e2[q + 1];
					}
					xu = MIN(d[q] - (x1 + u), xu); /*#110*/
					x0 = MAX(d[q] + (x1 + u), x0);
					if (v == 0.0)
					{
						break;
					}
				} /*for (q=p;q < n ;q++)*/ /*#120*/

				x1 = MAX(fabs(xu), fabs(x0))*machep; /*#140*/
				if (*eps1 <= 0.0)
				{
					*eps1 = -x1;
				}
				if (p != q) 
				{
					x1 *= (double) (q - p + 1); /*#180*/
					*lb = MAX(t1, xu - x1);
					*ub = MIN(t2, x0 + x1);
					x1 = *lb;
					isturm = 3;

					while (4)
					{
/*      .......... In-line procedure for Sturm sequence .......... */
						s = p - 1; /*#320*/
						u = 1.0;

						for (i=p;i <= q ;i++)
						{
							if (u != 0.0)
							{
								v = e2[i]/u; /*#325*/
							} /*if (u != 0.0)*/
							else 
							{
								v = fabs(e[i])/machep;
								if (e2[i] == 0.0)
								{
									v = 0.0;
								}
							} /*if (u != 0.0){}else{}*/
							u = d[i] - x1 - v; /*#330*/
							if (u < 0.0)
							{
								s++;
							}
						} /*for (i=p;i <= q ;i++)*/ /*#340*/

						switch(isturm)
						{
						  case 1:
							if (s == m1) /*#60*/
							{
								breaker = 4;
								break;
							} /*if (s == m1)*/
							else
							{
								if (s < m1)
								{
									xu = x1; /*#65*/
								}
								else
								{
									x0 = x1; /*#70*/
								}
								goto errorCheck;
							} /*if (s == m1){}else{}*/ 

						  case 2:
							if (s == m22) /*#80*/
							{
								breaker = 3;
								break;
							}
							else
							{
								if (s < m22)
								{
									xu = x1; /*#65*/
								}
								else
								{
									x0 = x1; /*#70*/
								}
								goto errorCheck;
							}

						  case 3:
							m1 = s + 1; /*#200*/
							x1 = *ub;
							isturm = 4;
							continue;

						  case 4:
							m2 = s; /*#220*/
							if (m1 > m2)
							{
								if (q >= n - 1) /*#940*/
								{
									goto exitPoint;
								}
								continuer = 1;
								breaker = 2;
								break;
							}
/* .......... Find roots by bisection .......... */
							x0 = *ub;
							isturm = 5;

							for (i=m1;i <= m2 ;i++)
							{
								rv5[i] = *ub;
								rv4[i] = *lb;
							} /*#240*/
/*
      .......... Loop for k-th eigenvalue
                 for k=m2 step -1 until m1 do --
                 (-do- not used to legalize -computed go to-) ..........
*/
							k = m2;
							break;
							
						  case 5:
/*      .......... Refine intervals .......... */
							if (s >= k) /*#360*/
							{
								x0 = x1; /*#400*/
							} /*if (s >= k)*/
							else 
							{
								xu = x1;
								if (s < m1)
								{
									rv4[m1] = x1;
								} /*if (s < m1)*/
								else 
								{
									rv4[s + 1] = x1; /*#380*/
									if (rv5[s] > x1)
									{
										rv5[s] = x1;
									}
								} /*if (s < m1){}else{}*/
							} /*if (s >= k){}else{}*/
							goto bisection;
						} /*switch(isturm)*/
						if (breaker-- > 1)
						{
							break;
						}

						while (5)
						{
							xu = *lb; /*#250*/
/*      .......... For i=k step -1 until m1 do -- .......... */
							for (i=k;i >= m1;i--)
							{
								if (xu < rv4[i])
								{
									xu = rv4[i];
									break;
								}
							} /*#260*/		
							if (x0 > rv5[k]) /*#280*/
							{
								x0 = rv5[k];
							}

/*      .......... Next bisection step .......... */
						  bisection: /*#300*/
							x1 = (xu + x0)*0.5;
							s1 = fabs(xu) + fabs(x0) + fabs(*eps1);
#ifdef DJGPP
			/* use epslon() to outfox compiler so convergence is noticed */
							if (fabs(x0 - xu)/2.0 > epslon(s1))
							{
								continuer = 1;
								breaker = 1;
								break;
							}
#else /*DJGPP*/
							s2 = s1 + fabs(x0 - xu)/2.0;
							if (s2 != s1)
							{
								continuer = 1;
								breaker = 1;
								break;
							}
#endif /*DJGPP*/

/*      .......... k-th eigenvalue found .......... */
							rv5[k] = x1; /*#420*/
							k--;
							if (k < m1)
							{
								breaker = 2;
								break;
							}
						} /*while (5)*/

						if (breaker-- > 1)
						{
							break;
						}
						if (continuer > 0)
						{
							continuer = 0;
							continue;
						}

					  errorCheck: /*#50*/
						v = x1;
						x1 = xu + (x0 - xu)*0.5;
						if (x1 == v)
						{
/*
      .......... Set error -- interval cannot be found containing
                 exactly the desired eigenvalues ..........
*/
							*ierr = 3*n + isturm; /*#980*/
							goto exitPoint;
						}
					} /*while (4)*/
					if (breaker-- > 1)
					{
						break;
					}
					if (continuer > 0)
					{
						continuer = 0;
						continue;
					}
				} /*if (p != q) */
				else 
				{
/*      .......... Check for isolated root within interval .......... */
					if (t1 > d[p] || d[p] >= t2)
					{
						if (q >= n - 1) /*#940*/
						{
							goto exitPoint;
						}
						continue;
					}
					m1 = p;
					m2 = p;
					rv5[p] = d[p];
				} /*if (p != q){}else{}*/
/*
      .......... Order eigenvalues tagged with their
                 submatrix associations ..........
*/
				s = r; /*#900*/
				r += m2 - m1 + 1;
				j = 0;
				k = m1;
				for (l=0;l <= r ;l++)
				{
					if (j <= s) 
					{
						if (k > m2)
						{
							break;
						}
						if (rv5[k] >= w[l]) 
						{
							j++; /*#915*/
							continue;
						} /*if (rv5[k] >= w[l]) */
						else
						{
							for (i=l+s-j;i >= l;i--)
							{
								w[i + 1] = w[i];
								ind[i + 1] = ind[i];
							} /*#905*/
						} /*if (rv5[k] >= w[l]){}else{} */
					} /*if (j <= s) */

					w[l] = rv5[k]; /*#910*/
					ind[l] = tag;
					k++;
				} /*for (l=0;l <= r ;l++)*/ /*#920*/

				if (q >= n - 1) /*#940*/
				{
					goto exitPoint;
				}
			} /*while (3)*/
			if (breaker-- > 1)
			{
				break;
			}
			if (continuer > 0)
			{
				continuer = 0;
				continue;
			}
			t2 = x1; /*#85*/
		} /*while (2)*/

		if (breaker-- > 1)
		{
			break;
		}

		xu = x1; /*#73*/
		t1 = x1;
	} /*while (1)*/

  exitPoint:
	*lb = t1; /*#1001*/
	*ub = t2;
} /*tridib()*/


#define MAXIT 5

/*
 ***begin prologue  tinvit
 ***date written   760101   (yymmdd)
 ***revision date  830518   (yymmdd)
 ***category no.  d4c3
 ***keywords  eigenvalues,eigenvectors,eispack
 ***author  smith, b. t., et al.
 ***purpose  eigenvectors of symmetric tridiagonal matrix corresponding
             to some specified eigenvalues, using inverse iteration
 ***description

      This subroutine is a translation of the inverse iteration tech-
      nique in the ALGOL procedure tristurm by Peters and Wilkinson.
      Handbook for Auto. Comp., Vol.II-Linear Algebra, 418-439(1971).

      This subroutine finds those eigenvectors of a tridiagonal
      symmetric matrix corresponding to specified eigenvalues,
      using inverse iteration.

      On input

         nm must be set to the row dimension of two-dimensional
           array parameters as declared in the calling program
           dimension statement.

         n is the order of the matrix.

         d contains the diagonal elements of the input matrix.

         e contains the subdiagonal elements of the input matrix
           in its last n-1 positions.  e(1) is arbitrary.

         e2 contains the squares of the corresponding elements of e,
           with zeros corresponding to negligible elements of e.
           e(i) is considered negligible if it is not larger than
           the product of the relative machine precision and the sum
           of the magnitudes of d(i) and d(i-1).  e2(1) must contain
           0.0e0 if the eigenvalues are in ascending order, or 2.0e0
           if the eigenvalues are in descending order.  if  bisect,
           tridib, or  imtqlv  has been used to find the eigenvalues,
           their output e2 array is exactly what is expected here.

         m is the number of specified eigenvalues.

         w contains the m eigenvalues in ascending or descending order.

         ind contains in its first m positions the submatrix indices
           associated with the corresponding eigenvalues in w --
           1 for eigenvalues belonging to the first submatrix from
           the top, 2 for those belonging to the second submatrix, etc.

      On output

        ** all input arrays are unaltered.**

         z contains the associated set of orthonormal eigenvectors.
           any vector which fails to converge is set to zero.

         ierr is set to
           zero       for normal return,
           -r         if the eigenvector corresponding to the r-th
                      eigenvalue fails to converge in 5 iterations.

         rv1, rv2, rv3, rv4, and rv6 are temporary storage arrays.

      Questions and comments should be directed to B. S. Garbow,
      Applied Mathematics Division, Argonne National Laboratory
      ------------------------------------------------------------------
 ***References  B. T. Smith, J. M. Boyle, J. J. Dongarra, B. S. Garbow,
                  Y. Ikebe, V. C. Klema, C. B. Moler, *Matrix Eigen-
                  System Routines - EISPACK Guide*, Springer-Verlag,
                  1976.
 ***Routines called  (none)
 ***end prologue  tinvit
*/
/*
  Comments on C-version:
   All indices now have 0 origin.  This includes i, j, p, q, r, s, jj

   In case of error on the r-th root, *ierr is set to -(r+1), r = 0,...,m-1

   Many of the original line numbers are present in comments of the form
   #nnn at the end of a line

*/

void tinvit(long nm, long n, double *d, double *e, double *e2,
			long m, double *w,long *ind, double *z, long *ierr,
			double *rv1, double *rv2, double *rv3,double *rv4, double *rv6)
{
	/*
	   long ind[m];
	   double d[n], e[n], e2[n], w[m], z[nm][m];
	   double rv1[n], rv2[n], rv3[n], rv4[n], rv6[n];
	   */

	long           i, j, p, q, r, s, jj, its, tag, group = 0;
	double        u, v, uk, xu, x0, x1, eps2, eps3, eps4, norm, order;
	double        kluge;
#ifdef USEPTRS
	double       *zj, *zr;
#endif /*USEPTRS*/

	/* ***first executable statement  tinvit */
	*ierr = 0;

	if (m != 0)
	{
		tag = 0;
		order = 1.0 - e2[0];
		q = -1;
		do /*  while(q < n - 1);*/
		{
/*      .......... establish and process next submatrix .......... */
			p = q + 1; /*#100*/

			for (q=p;q < n - 1 && e2[q+1] != 0.0 ;q++)
			{
				;
			} /*for (q=p;q < n - 1 && e2[q] != 0.0 ;q++)*/ /*#120*/
/*      .......... find vectors by inverse iteration .......... */
			tag++; /*#140*/
			s = -1;

			for (r=0;r < m ;r++)
			{
				if (ind[r] == tag)
				{
					its = 1;
					x1 = w[r];
					if (s < 0)
					{
/*      .......... check for isolated root .......... */
						xu = 1.0;
						if (p == q)
						{
							rv6[p] = 1.0;
							goto setZir;
						} /*if (p == q) */
						norm = fabs(d[p]); /*#490*/

						for (i=p+1;i <= q ;i++)
						{
							norm = MAX(norm, fabs(d[i]) + fabs(e[i]));
						} /*for (i=p+1;i <= q ;i++)*/ /*#500*/
/*
  .......... eps2 is the criterion for grouping,
			 eps3 replaces zero pivots and equal
			 roots are modified by eps3,
			 eps4 is taken very small to avoid overflow ..........
*/
						eps2 = .001*norm;
#ifdef DJGPP
						eps3 = epslon(norm);
#else /*DJGPP*/
						eps3 = norm;
						do
						{
							eps3 *= 0.5; /*#502*/
							kluge = norm + eps3;
						} while (kluge > norm);
#endif /*DJGPP*/
						uk = sqrt((double)(q - p + 5));
						eps3 = uk*eps3;
						eps4 = uk*eps3;
						uk = eps4/uk;
						s = p;
						group = 0; /*#505*/
					} /*if (s < 0) */
					else
					{
/*      .......... look for close or coincident roots .......... */
						
						if (fabs(x1 - x0) < eps2) /*#510*/
						{
							group++;
							if (order*(x1 - x0) <= 0.0)
							{
								x1 = x0 + order*eps3;
							}
						} /*if (fabs(x1 - x0) < eps2) */
						else
						{
							group = 0; /*#505*/
						}
					} /*if (s < 0) {}else{}*/
/*
      .......... elimination with interchanges and
                 initialization of vector ..........
*/
					v = 0.0; /*#520*/

					for (i=p;i <= q ;i++)
					{
						rv6[i] = uk;
						if (i > p)
						{
							if (fabs(e[i]) < fabs(u))
							{
								xu = e[i]/u; /*#540*/
								rv4[i] = xu;
								rv1[i - 1] = u;
								rv2[i - 1] = v;
								rv3[i - 1] = 0.0;
							} /*if (fabs(e[i]) < fabs(u)) */
							else
							{
/*
      .......... warning -- a divide check may occur here if
                 e2 array has not been specified correctly ..........
*/
								xu = u/e[i];
								rv4[i] = xu;
								rv1[i - 1] = e[i];
								rv2[i - 1] = d[i] - x1;
								rv3[i - 1] = 0.0;
								if (i != q)
								{
									rv3[i - 1] = e[i + 1];
								}
								u = v - xu*rv2[i - 1];
								v = -xu*rv3[i - 1];
								continue;
							} /*if (fabs(e[i]) < fabs(u)){}else{} */
						} /*if (i > p)*/
						u = d[i] - x1 - xu*v; /*#560*/
						if (i != q)
						{
							v = e[i + 1];
						}
					} /*for (i=p;i <= q ;i++)*/ /*#580*/

					if (u == 0.0)
					{
						u = eps3;
					}
					rv1[q] = u;
					rv2[q] = 0.0;
					rv3[q] = 0.0;

					while (1)
					{
/*
      .......... back substitution
                 for i=q step -1 until p do -- ..........
*/

						for (i=q; i >= p; i--) /*#600*/
						{
							rv6[i] = (rv6[i] - u*rv2[i] - v*rv3[i])/rv1[i];
							v = u;
							u = rv6[i];
						} /*for (i=q; i >= p; i--)*/ /*#620*/
						incNproducts(5*(q-p+1));
/*
      .......... orthogonalize with respect to previous
                 members of group ..........
*/
						j = r;

						for (jj=0;jj < group ;jj++)
						{
							do
							{
								j--; /*#630*/
							} while (ind[j] != tag);
							xu = 0.0;

#ifdef USEPTRS
							zj = z + j*nm; /*zj[i] is Q2z(i,j)*/
							for (i=p;i <= q ;i++)
							{
								xu += rv6[i]*zj[i];
							} /*for (i=p;i <= q ;i++)*/ /*#640*/

							for (i=p;i <= q ;i++)
							{
								rv6[i] -= xu*zj[i];
							} /*for (i=p;i <= q ;i++)*/
#else /*USEPTRS*/
							for (i=p;i <= q ;i++)
							{
								xu += rv6[i]*Q2z(i,j);
							} /*for (i=p;i <= q ;i++)*/ /*#640*/

							for (i=p;i <= q ;i++)
							{
								rv6[i] -= xu*Q2z(i,j);
							} /*for (i=p;i <= q ;i++)*/
#endif /*USEPTRS*/
							incNproducts(2*(q-p+1));
						} /*for (jj=0;jj < group ;jj++)*/ /*#680*/

						norm = 0.0; /*#700*/

						for (i=p;i <= q ;i++)
						{
							norm += fabs(rv6[i]);
						} /*for (i=p;i <= q ;i++)*/ /*#720*/

						if (norm >= 1.0)
						{
							break;
						} /*if (norm >= 1.0)*/

/*      .......... forward substitution .......... */
						if (its == MAXIT)
						{
/*      .......... set error -- non-converged eigenvector .......... */
							*ierr = -(r+1); /*#830*/
							xu = 0.0;
						    break;
						} /*if (its == MAXIT)*/

						if (norm != 0.0)
						{
							xu = eps4/norm; /*#740*/

							for (i=p;i <= q ;i++)
							{
								rv6[i] = rv6[i]*xu;
							} /*#760*/
							incNproducts(q-p+1);
						} /*if (norm != 0.0) */
						else
						{
							rv6[s] = eps4;
							s++;
							if (s > q)
							{
								s = p;
							}
						} /*if (norm != 0.0) {}else{}*/
/*
      .......... elimination operations on next vector
                 iterate ..........
*/
						for (i=p+1;i <= q ;i++) /*#780*/
						{
							u = rv6[i];
/*
      .......... if rv1[i-1] .eq. e(i), a row interchange
                 was performed earlier in the
                 triangularization process ..........
*/
							if (rv1[i - 1] == e[i])
							{
								u = rv6[i - 1];
								rv6[i - 1] = rv6[i];
							}
							rv6[i] = u - rv4[i]*rv6[i - 1]; /*#800*/
						} /*for (i=p;i <= q ;i++)*/ /*#820*/
						incNproducts(q-p+1);
						if (*ierr)
						{
							break;
						}
						testNproducts(errorExit);
						its++;
					} /*while (1)*/

					if (*ierr == 0)
					{
/*
      .......... normalize so that sum of squares is
                 1 and expand to full order ..........
*/

						u = 0.0; /*#840*/

						for (i=p;i <= q ;i++)
						{
							u += rv6[i]*rv6[i];
						} /*for (i=p;i <= q ;i++)*/ /*#860*/
						incNproducts(q-p+1);
						xu = 1.0/sqrt(u);
					} /*if (*ierr == 0)*/

				  setZir: /*#870*/
#ifdef USEPTRS
					zr = z + r*nm;
					for (i=0;i < n ;i++)
					{
						zr[i] = 0.0;
					} /*for (i=0;i < n ;i++)*/ /*#880*/

					for (i=p;i <= q ;i++)
					{
						zr[i] = rv6[i]*xu;
					} /*for (i=p;i <= q ;i++)*/ /*#900*/
#else /*USEPTRS*/
					for (i=0;i < n ;i++)
					{
						Q2z(i,r) = 0.0;
					} /*for (i=0;i < n ;i++)*/ /*#880*/

					for (i=p;i <= q ;i++)
					{
						Q2z(i,r) = rv6[i]*xu;
					} /*for (i=p;i <= q ;i++)*/ /*#900*/
#endif /*USEPTRS*/
					incNproducts(q-p+1);
					x0 = x1;
				} /*if (ind[r] == tag) */
			} /*for (r=0;r < m ;r++)*/ /*#920*/
			testNproducts(errorExit);
		} while(q < n - 1);
	} /*if (m != 0)*/

/* fall through */

  errorExit:
	;

} /*tinvit()*/
