/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 4.02 or later
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
#pragma segment LinpakGE
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

/* 961210 Added code to allow checks for interrupt; see linpack.h*/

/*
Translation to C of Linpack double precision routines dgeco(), dgedi(),
dgefa(), and dgesl() for computing and using LU factorizations of double
precision matrices

DGECO.........Compute  LU  factorization  of  general  double  precision
              matrix and estimate its condition.
DGEDI.........Uses LU factorization of general double  precision  matrix
              to compute its determinant and/or inverse.
DGEFA.........Compute  LU  factorization  of  general  double  precision
              matrix.
DGESL.........Uses LU factorization of general double  precision  matrix
              to solve systems.
The Fortran storage ordering has been preserved, that is matrices are
stored column by column, with elements of each column in successive double
words.

BLAS routines are used, using the Fortran calling sequences, that is all
arguments are pointers.  The actual names used are defined by macros in
blas.h which in the present version defines them as daxpy_, ddot_,
etc.

Similarly, the calling sequences of the Linpack routines are Fortran style,
using pointers with names defined by macros in linpack.h

These routines should work identically to the Fortran versions, except the
indices returned in ipvt[] by DGECO() and DGEFA() and expected in ipvt[] by
DGEDI() and DGESL() count from 0.

The translation was accomplished as follows: Fortran was converted to
structured Ratfor by struct;  Ratfor was partially translated to C by
sed script ratfor2C; this was followed by hand editing, changing subscripts
to run from 0, modifying loops, etc., accompanied by line by line comparison
with original Fortran and testing.

*/

/*====> dgeco.c <====*/

#include "blas.h"
#include "linpack.h"

#if (0)

/* blas prototypes as in blas.h */
void   DAXPY(INT *, double *, double *, INT *, double *, INT *);
double DDOT(INT *, double *, INT *, double *, INT *);
double DASUM(INT *, double *, INT *);
void   DSCAL(INT *, double *, double *, INT *);
double DSWAP(INT *, double *, INT *, double *, INT *);
INT    IDAMAX(INT *, double *, INT *);

/* linpack prototypes as in linpack.h */
void DGECO(double a[], INT *plda, INT *pn, INT ipvt[],
		   double *rcond, double z[]);
void DGEDI(double *a, INT *plda, INT *pn, INT ipvt[], double det[],
		   double work[], INT *pjob);
void DGEFA(double *a, INT *plda, INT *pn, INT ipvt[], INT *info);
void DGESL(double *a, INT *plda, INT *pn, INT ipvt[], double *b, INT *pjob);
#endif

/*
	***BEGIN PROLOGUE  DGECO
	***DATE WRITTEN   780814   (YYMMDD)
	***REVISION DATE  820801   (YYMMDD)
	***Translated to C 930627
	***CATEGORY NO.  D2A1
	***KEYWORDS  CONDITION,DOUBLE PRECISION,FACTOR,LINEAR ALGEBRA,LINPACK,
	             MATRIX
	***AUTHOR  MOLER, C. B., (U. OF NEW MEXICO)
	***PURPOSE  Factors a double precision matrix by Gaussian elimination
	            and estimates the condition of the matrix.
	***DESCRIPTION
	
	     DGECO factors a double precision matrix by Gaussian elimination
	     and estimates the condition of the matrix.
	
	     If  RCOND  is not needed, DGEFA is slightly faster.
	     To solve  A*X = B , follow DGECO by DGESL.
	     To compute  INVERSE(A)*C , follow DGECO by DGESL.
	     To compute  DETERMINANT(A) , follow DGECO by DGEDI.
	     To compute  INVERSE(A) , follow DGECO by DGEDI.
	
	     On Entry
	
	        A       DOUBLE PRECISION(LDA, N)
	                the matrix to be factored.
	
	        LDA     INTEGER
	                the leading dimension of the array  A .
	
	        N       INTEGER
	                the order of the matrix  A .
	
	     On Return
	
	        A       an upper triangular matrix and the multipliers
	                which were used to obtain it.
	                The factorization can be written  A = L*U  where
	                L  is a product of permutation and unit lower
	                triangular matrices and  U  is upper triangular.
	
	        IPVT    INTEGER(N)
	                an INTEGER vector of pivot indices.
	
	        RCOND   DOUBLE PRECISION
	                an estimate of the reciprocal condition of  A .
	                For the system  A*X = B , relative perturbations
	                in  A  and  B  of size  EPSILON  may cause
	                relative perturbations in  X  of size  EPSILON/RCOND .
	                If  RCOND  is so small that the logical expression
	                           1.0 + RCOND .EQ. 1.0
	                is true, then  A  may be singular to working
	                precision.  In particular,  RCOND  is zero  if
	                exact singularity is detected or the estimate
	                underflows.
	
	        Z       DOUBLE PRECISION(N)
	                a work vector whose contents are usually unimportant.
	                If  A  is close to a singular matrix, then  Z  is
	                an approximate null vector in the sense that
	                NORM(A*Z) = RCOND*NORM(A)*NORM(Z) .
	
	     LINPACK.  This version dated 08/14/78 .
	     Cleve Moler, University of New Mexico, Argonne National Lab.
	
	     Subroutines and Functions
	
	     LINPACK DGEFA
	     BLAS DAXPY,DDOT,DSCAL,DASUM
	     Fortran DABS,DMAX1,DSIGN
	***References  Dongarra J.J., BUNCH J.R., Moler C.B., Stewart G.W.,
	                 *Linpack Users  Guide*, SIAM, 1979.
	***Routines Called  DASUM,DAXPY,DDOT,DGEFA,DSCAL
	***END PROLOGUE  DGECO
*/

void DGECO(double a[], INT *plda, INT *pn, INT ipvt[],
		   double *rcond, double z[])
/*
double a[lda][1],z[1];
*/
{
	double         ek, t, wk, wkm;
	double         anorm, s, sm, ynorm;
	double        *aj, *ak, *akk, *akj;
	double         temp;
	INT            lda = *plda, n = *pn;
	INT            info, j, k, l;
	INT            inc = 1, length;

	/* 	compute 1-norm of a */

	/* ***first executable statement  dgeco */
	anorm = 0.0;
	aj = a;
	for(j = 0;j < n ;j++)
	{
		temp  = DASUM(&n, aj, &inc);
		if(temp > anorm)
		{
			anorm = temp;
		}
		aj += lda;
	} /*for(j = 0;j < n ;j++)*/

	/* 	factor */

	DGEFA(a, &lda, &n, ipvt, &info);
	checkInterrupt(errorExit);
/*
	rcond = 1/(norm(a)*(estimate of norm(inverse(a)))) .
	estimate = norm(z)/norm(y) where  a*z = y  and  trans(a)*y = e .
	trans(a)  is the transpose of a .  the components of  e  are
	chosen to cause maximum local growth in the elements of w  where
	trans(u)*w = e .  the vectors are frequently rescaled to avoid
	overflow.
*/
	/* 	solve trans(u)*w = e */

	ek = 1.0;
	for(j = 0;j < n ;j++)
	{
		z[j] = 0.0;
	}
	akk = a;
	for(k = 0;k < n ;k++)
	{
		if (z[k] != 0.0)
		{
			ek = (-z[k] > 0) ? fabs(ek) : -fabs(ek);
		}
		if (fabs(ek - z[k]) > fabs(*akk))
		{
			s = fabs(*akk)/fabs(ek - z[k]);
			DSCAL(&n, &s, z, &inc);
			ek *= s;
		}
		wk = ek - z[k];
		wkm = -ek - z[k];
		s = fabs(wk);
		sm = fabs(wkm);
		if (*akk == 0.0)
		{
			wk = 1.0;
			wkm = 1.0;
		}
		else
		{
			wk /= *akk;
			wkm /= *akk;
		}

		if(k < n-1)
		{
			akj = akk + lda;
			for(j = k+1;j < n ;j++)
			{
				sm += fabs(z[j] + wkm*(*akj));
				z[j] += wk*(*akj);
				s += fabs(z[j]);
				akj += lda;
			} /*for(j = k+1;j < n ;j++)*/

			if (s < sm)
			{
				t = wkm - wk;
				wk = wkm;
				akj = akk + lda;
				for(j = k+1;j < n ;j++)
				{
					z[j] += t*(*akj);
					akj += lda;
				}
			} /*if (s < sm)*/
		} /*if(k < n-1)*/
		incAndTest(3*(n-k-1),errorExit);
		z[k] = wk;
		akk += lda + 1;
	} /*for(k = 0;k < n ;k++)*/
	
	s = 1.0/DASUM(&n, z, &inc);
	DSCAL(&n, &s, z, &inc);

	/* 	solve trans(l)*y = w */

	ak = a + (n - 1)*lda;
	for(k = n-1;k >= 0;k--)
	{
		length = n - k - 1;
		if (length > 0)
		{
			z[k] += DDOT(&length, ak + k + 1, &inc, z + k + 1, &inc);
		}
		if (fabs(z[k]) > 1.0)
		{
			s = 1.0/fabs(z[k]);
			DSCAL(&n, &s, z, &inc);
		}
		incAndTest(length + n, errorExit);
		l = ipvt[k];
		t = z[l];
		z[l] = z[k];
		z[k] = t;
		ak -= lda;
	} /*for(k = n-1;k >= 0;k--)*/
	s = 1.0/DASUM(&n, z, &inc);
	DSCAL(&n, &s, z, &inc);

	ynorm = 1.0;

	/* 	solve l*v = y */

	ak = a;
	for(k = 0;k < n ;k++)
	{
		l = ipvt[k];
		t = z[l];
		z[l] = z[k];
		z[k] = t;
		length = n - k - 1;
		if (length > 0)
		{
			DAXPY(&length, &t, ak + k + 1, &inc, z + k + 1, &inc);
		}
		if (fabs(z[k]) > 1.0)
		{
			s = 1.0/fabs(z[k]);
			DSCAL(&n, &s, z, &inc);
			ynorm *= s;
		}
		incAndTest(length+n, errorExit);
		ak += lda;
	} /*for(k = 0;k < n ;k++)*/

	s = 1.0/DASUM(&n, z, &inc);
	DSCAL(&n, &s, z, &inc);
	ynorm *= s;

	/* 	solve  u*z = v */

	ak = a + (n - 1)*lda;
	for(k = n-1;k >= 0;k--)
	{
		akk = ak + k;
		if (fabs(z[k]) > fabs(*akk))
		{
			s = fabs(*akk)/fabs(z[k]);
			DSCAL(&n, &s, z, &inc);
			ynorm *= s;
		}
		if (*akk != 0.0)
		{
			z[k] /= *akk;
		}
		else
		{
			z[k] = 1.0;
		}
		t = -z[k];
		DAXPY(&k, &t, ak, &inc, z, &inc);
		incAndTest(n+k,errorExit);
		ak -= lda;
	} /*for(k = n-1;k >= 0;k--)*/

	/* 	make znorm = 1.0 */
	s = 1.0/DASUM(&n, z, &inc);
	DSCAL(&n, &s, z, &inc);
	ynorm *= s;

	*rcond = (anorm != 0.0) ? ynorm/anorm : 0.0;
	/* fall through*/
  errorExit:
	;

} /*DGECO()*/

/*====> dgedi.c <====*/

#include "blas.h"
#include "linpack.h"

/*
	***BEGIN PROLOGUE  DGEDI
	***DATE WRITTEN   780814   (YYMMDD)
	***REVISION DATE  820801   (YYMMDD)
	***Translated to C 930625
	***CATEGORY NO.  D3A1,D2A1
	***KEYWORDS  DETERMINANT,DOUBLE PRECISION,FACTOR,INVERSE,
	             LINEAR ALGEBRA,LINPACK,MATRIX
	***AUTHOR  MOLER, C. B., (U. OF NEW MEXICO)
	***PURPOSE  Computes the determinant and inverse of a matrix using
	            factors computed by DGECO or DGEFA.
	***DESCRIPTION
	
	     DGEDI computes the determinant and inverse of a matrix
	     using the factors computed by DGECO or DGEFA.
	
	     On Entry
	
	        A       DOUBLE PRECISION(LDA, N)
	                the output from DGECO or DGEFA.
	
	        LDA     INTEGER
	                the leading dimension of the array  A .
	
	        N       INTEGER
	                the order of the matrix  A .
	
	        IPVT    INTEGER(N)
	                the pivot vector from DGECO or DGEFA.
	
	        WORK    DOUBLE PRECISION(N)
	                work vector.  Contents destroyed.
	
	        JOB     INTEGER
	                = 11   both determinant and inverse.
	                = 01   inverse only.
	                = 10   determinant only.
	
	     On Return
	
	        A       inverse of original matrix if requested.
	                Otherwise unchanged.
	
	        DET     DOUBLE PRECISION(2)
	                determinant of original matrix if requested.
	                Otherwise not referenced.
	                Determinant = DET(1) * 10.0**DET(2)
	                with  1.0 .LE. DABS(DET(1)) .LT. 10.0
	                or  DET(1) .EQ. 0.0 .
	
	     Error Condition
	
	        A division by zero will occur if the input factor contains
	        a zero on the diagonal and the inverse is requested.
	        It will not occur if the subroutines are called correctly
	        and if DGECO has set RCOND .GT. 0.0 or DGEFA has set
	        INFO .EQ. 0 .
	
	     LINPACK.  This version dated 08/14/78 .
	     Cleve Moler, University of New Mexico, Argonne National Lab.
	
	     Subroutines and Functions
	
	     BLAS DAXPY,DSCAL,DSWAP
	     Fortran DABS,MOD
	***References  Dongarra J.J., Bunch J.R., Moler C.B., Stewart G.W.,
	                 *Linpack Users  Guide*, SIAM, 1979.
	***ROUTINES CALLED  DAXPY,DSCAL,DSWAP
	***END PROLOGUE  DGEDI
*/

void DGEDI(double *a, INT *plda, INT *pn, INT ipvt[], double det[],
		   double work[], INT *pjob)
/*double     a[lda][1],det[2],work[1];*/
{

	double         t;
	double        *aj, *ak, *aii, *akk, *akj;
	INT            lda = *plda, n = *pn, job = *pjob;
	INT            i, j, k, l;
	INT            inc = 1, length;

	/* 	compute determinant */
	
	/* ***first executable statement  dgedi */
	if (job/10 != 0)
	{
		det[0] = 1.0;
		det[1] = 0.0;
		aii = a;
		for(i = 0;i < n ;i++)
		{
			if (ipvt[i] != i)
			{
				det[0] = -det[0];
			}
			det[0] *= *aii;
	/* 	...exit */
			if (det[0] == 0.0)
			{
				break;
			}
			while (fabs(det[0]) < 1.0)
			{
				det[0] *= 10.0;
				det[1]--;
			}
			while (fabs(det[0]) >= 10.0)
			{
				det[0] /= 10.0;
				det[1]++;
			}
			aii += lda + 1;
		} /*for(i = 0;i < n ;i++)*/
	} /*if (job/10 != 0)*/
	
	/* 	compute inverse(u) */
	
	if (job % 10 != 0)
	{
		ak = a;
		for(k = 0;k < n ;k++)
		{
			akk = ak + k;
			*akk = 1.0/(*akk);
			t = -(*akk);
			DSCAL(&k, &t, ak, &inc);
			aj = ak + lda;
			akj = akk + lda;
			length = k + 1;
			for(j = k+1;j < n ;j++)
			{
				t = *akj;
				*akj = 0.0;
				DAXPY(&length, &t, ak, &inc, aj, &inc);
				aj += lda;
				akj += lda;
			}
			ak += lda;
		} /*for(k = 0;k < n ;k++)*/
		incAndTest(n*(n+1), errorExit);
		
	/* 	form inverse(u)*inverse(l) */
	
		ak = a + (n - 2)*lda;
		for(k = n-2;k >= 0;k--)
		{
			for(i = k+1;i < n ;i++)
			{
				work[i] = ak[i];
				ak[i] = 0.0;
			}
			aj = ak + lda;
			for(j = k+1;j < n ;j++)
			{
				t = work[j];
				DAXPY(&n, &t, aj, &inc, ak, &inc);
				aj += lda;
			}
			incAndTest((n-k-1)*n, errorExit);
			l = ipvt[k];
			if (l != k)
			{
				DSWAP(&n, ak, &inc, a + l*lda, &inc);
			}
			ak -= lda;
		} /*for(k = n-2;k >= 0;k--)*/
	} /*if (job % 10 != 0)*/
	/*fall through*/
  errorExit:
	;
} /*DGEDI()*/



/*====> dgefa.c <====*/

#include "blas.h"
#include "linpack.h"

/*
	***BEGIN PROLOGUE  DGEFA
	***DATE WRITTEN   780814   (YYMMDD)
	***REVISION DATE  820801   (YYMMDD)
	***Translated to C 930627
	***CATEGORY NO.  D2A1
	***KEYWORDS  DOUBLE PRECISION,FACTOR,LINEAR ALGEBRA,LINPACK,MATRIX
	***AUTHOR  MOLER, C. B., (U. OF NEW MEXICO)
	***PURPOSE  Factors a double precision matrix by Gaussian elimination.
	***DESCRIPTION
	
	     DGEFA factors a double precision matrix by Gaussian elimination.
	
	     DGEFA is usually called by DGECO, but it can be called
	     directly with a saving in time if  RCOND  is not needed.
	     (Time for DGECO) = (1 + 9/N)*(Time for DGEFA) .
	
	     On Entry
	
	        A       DOUBLE PRECISION(LDA, N)
	                the matrix to be factored.
	
	        LDA     INTEGER
	                the leading dimension of the array  A .
	
	        N       INTEGER
	                the order of the matrix  A .
	
	     On Return
	
	        A       an upper triangular matrix and the multipliers
	                which were used to obtain it.
	                The factorization can be written  A = L*U  where
	                L  is a product of permutation and unit lower
	                triangular matrices and  U  is upper triangular.
	
	        IPVT    INTEGER(N)
	                an integer vector of pivot indices.
	
	        INFO    INTEGER
	                = 0  normal value.
	                = K  if  U(K,K) .EQ. 0.0 .  This is not an error
	                     condition for this subroutine, but it does
	                     indicate that DGESL or DGEDI will divide by zero
	                     if called.  Use  RCOND  in DGECO for a reliable
	                     indication of singularity.
	
	     LINPACK.  This version dated 08/14/78 .
	     Cleve Moler, University of New Mexico, Argonne National Lab.
	
	     Subroutines and Functions
	
	     BLAS DAXPY,DSCAL,IDAMAX
	***References  Dongarra J.J., Bunch J.R., Moler C.B., Stewart G.W.,
	                 *Linpack Users  Guide*, SIAM, 1979.
	***ROUTINES CALLED  DAXPY,DSCAL,IDAMAX
	***END PROLOGUE  DGEFA
*/

/*
  C note: *info is set to k + 1 if U[k][k] == 0 (using above notation)
*/

void DGEFA(double *a, INT *plda, INT *pn, INT ipvt[], INT *info)
/*double a[lda][1];*/
{
	double         t;
	double        *akk, *ak, *aj;
	INT            lda = *plda, n = *pn;
	INT            j,k,l;
	INT            inc = 1, length;

	/* 	  gaussian elimination with partial pivoting */
	
	/* ***first executable statement  dgefa */
	*info = 0;

	ak = a;
	for(k = 0;k < n - 1 ;k++)
	{
/* 		  find l = pivot index */
		akk = ak + k;
		length = n - k;
		l = IDAMAX(&length, akk, &inc) + k - 1;
		ipvt[k] = l;

/* 	     zero pivot implies this column already triangularized */

		if (ak[l] == 0.0)
		{
			*info = k+1;
		} /*if (ak[l] == 0.0)*/
		else
		{

/* 	        interchange if necessary */

			if (l != k)
			{
				t = ak[l];
				ak[l] = *akk;
				*akk = t;
			}

/* 	        compute multipliers */

			t = -1.0/(*akk);
			length = n - k - 1;
			DSCAL(&length, &t, ak + k + 1, &inc);

/* 	        row elimination with column indexing */

			aj = ak + lda;
			for(j = k+1;j < n ;j++)
			{
				t = aj[l];
				if (l != k)
				{
					aj[l] = aj[k];
					aj[k] = t;
				}
				DAXPY(&length, &t, ak + k + 1, &inc, aj + k + 1, &inc);
				aj += lda;
			} /*for(j = k+1;j < n ;j++)*/
			incAndTest((n-k)*length, errorExit);
		} /*if (ak[l] == 0.0){...}else{...}*/
		ak += lda;
	} /*for(k = 0;k < n - 1 ;k++)*/

	ipvt[n - 1] = n - 1;
	if (ak[n-1] == 0.0)
	{
		*info = n;
	}
	/*fall through*/
  errorExit:
	;
} /*DGEFA()*/

/*====> dgesl.c <====*/

#include "blas.h"
#include "linpack.h"

/*
	***BEGIN PROLOGUE  DGESL
	***DATE WRITTEN   780814   (YYMMDD)
	***REVISION DATE  820801   (YYMMDD)
	***CATEGORY NO.  D2A1
	***KEYWORDS  DOUBLE PRECISION,LINEAR ALGEBRA,LINPACK,MATRIX,SOLVE
	***AUTHOR  MOLER, C. B., (U. OF NEW MEXICO)
	***PURPOSE  Solves the double precision system  A*X=B or  TRANS(A)*X=B
	            using the factors computed by DGECO or DGEFA.
	***DESCRIPTION
	
	     DGESL solves the double precision system
	     A * X = B  or  TRANS(A) * X = B
	     using the factors computed by DGECO or DGEFA.
	
	     On Entry
	
	        A       DOUBLE PRECISION(LDA, N)
	                the output from DGECO or DGEFA.
	
	        LDA     INTEGER
	                the leading dimension of the array  A .
	
	        N       INTEGER
	                the order of the matrix  A .
	
	        IPVT    INTEGER(N)
	                the pivot vector from DGECO or DGEFA.
	
	        B       DOUBLE PRECISION(N)
	                the right hand side vector.
	
	        JOB     INTEGER
	                = 0         to solve  A*X = B ,
	                = nonzero   to solve  TRANS(A)*X = B  where
	                            TRANS(A)  is the transpose.
	
	     On Return
	
	        B       the solution vector  X .
	
	     Error Condition
	
	        A division by zero will occur if the input factor contains a
	        zero on the diagonal.  Technically this indicates singularity
	        but it is often caused by improper arguments or improper
	        setting of LDA .  It will not occur if the subroutines are
	        called correctly and if DGECO has set RCOND .GT. 0.0
	        or DGEFA has set INFO .EQ. 0 .
	
	     To compute  INVERSE(A) * C  where  C  is a matrix
	     with  P  columns
	           CALL DGECO(A,LDA,N,IPVT,RCOND,Z)
	           IF (RCOND is too small) GO TO ...
	           DO 10 J = 1, P
	              CALL DGESL(A,LDA,N,IPVT,C(1,J),0)
	        10 CONTINUE
	
	     LINPACK.  This version dated 08/14/78 .
	     Cleve Moler, University of New Mexico, Argonne National Lab.
	
	     Subroutines and Functions
	
	     BLAS DAXPY,DDOT
	***References  Dongarra J.J., Bunch J.R., Moler C.B., Stewart G.W.,
	                 *Linpack Users  Guide*, SIAM, 1979.
	***ROUTINES CALLED  DAXPY,DDOT
	***END PROLOGUE  DGESL
*/

void DGESL(double *a, INT *plda, INT *pn, INT ipvt[], double *b, INT *pjob)
/*double a[lda][1],b[1];*/
{

	double      t;
	double     *ak;
	INT         lda = *plda, n = *pn, job = *pjob;
	INT         k, l;
	INT         inc = 1, length;
	/* ***first executable statement  dgesl */

	if (job != 0)
	{

	/* 		  job = nonzero, solve  trans(a) * x = b */
	/* 		  first solve  trans(u)*y = b */

		ak = a;
		for(k = 0;k < n ;k++)
		{
			t = DDOT(&k, ak, &inc, b, &inc);
			b[k] = (b[k] - t)/ak[k];
			ak += lda;
		}
		
	/* 		  now solve trans(l)*x = y */

		ak = a + (n - 2)*lda;
		for(k = n-2;k >= 0;k--)
		{
			length = n - k - 1;
			b[k] += DDOT(&length, ak + k + 1, &inc, b + k + 1, &inc);
			l = ipvt[k];
			if (l != k)
			{
				t = b[l];
				b[l] = b[k];
				b[k] = t;
			}
		}
		incAndTest(n*(n-1), errorExit);
	} /*if (job != 0)*/
	else
	{

	/* 		  job = 0 , solve  a * x = b */
	/* 		  first solve  l*y = b */

		ak = a;
		for(k = 0;k < n-1 ;k++)
		{
			l = ipvt[k];
			t = b[l];
			if (l != k)
			{
				b[l] = b[k];
				b[k] = t;
			}
			length = n - k - 1;
			DAXPY(&length, &t, ak + k + 1, &inc, b + k+1, &inc);
			ak += lda;
		}

	/* 		  now solve  u*x = y */

		ak = a + (n - 1)*lda;
		for(k = n-1;k >= 0;k--)
		{
			b[k] /= ak[k];
			t = -b[k];
			DAXPY(&k, &t, ak, &inc, b, &inc);
			ak -= lda;
		}
		incAndTest(n*(n-1), errorExit);
	} /*if (job != 0){}else{}*/
  errorExit:
	;
} /*DGESL()*/

