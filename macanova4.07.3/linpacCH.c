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
#pragma segment LinpakCH
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

/* 961210 Added code to allow checks for interrupt (see linpack.h) */

/*
	Initial translation to C from Ratfor by ratfor2c
*/

#include "linpack.h"
#include "blas.h"

/*
	***BEGIN PROLOGUE  DCHDC
	***DATE WRITTEN   790319   (YYMMDD)
	***REVISION DATE  820801   (YYMMDD)
	***TRANSLATED TO C 930629 (C. Bingham)
	***CATEGORY NO.  D2B1B
	***KEYWORDS  CHOLESKY DECOMPOSITION,DOUBLE PRECISION,LINEAR ALGEBRA,
	             LINPACK,MATRIX,POSITIVE DEFINITE
	***AUTHOR  DONGARRA, J., (ANL)
	           STEWART, G. W., (U. OF MARYLAND)
	***PURPOSE  Computes the Cholesky decomposition of a POSITIVE DEFINITE
	            matrix.  A pivoting option allows the user to estimate the
	            condition of a positive definite matrix or determine the
	            rank of a POSITIVE SEMIDEFINITE matrix.
	***DESCRIPTION

	     DCHDC computes the Cholesky decomposition of a positive definite
	     matrix.  A pivoting option allows the user to estimate the
	     condition of a positive definite matrix or determine the rank
	     of a positive semidefinite matrix.

	     On Entry

	         A      DOUBLE PRECISION(LDA,P).
	                A contains the matrix whose decomposition is to
	                be computed.  Only the upper half of A need be stored.
	                The lower part of the array A is not referenced.

	         LDA    INTEGER.
	                LDA is the leading dimension of the array A.

	         P      INTEGER.
	                P is the order of the matrix.

	         WORK   DOUBLE PRECISION.
	                WORK is a work array.

	         JPVT   INTEGER(P).
	                JPVT contains integers that control the selection
	                of the pivot elements, if pivoting has been requested.
	                Each diagonal element A(K,K)
	                is placed in one of three classes according to the
	                value of JPVT(K).

	                   If JPVT(K) .GT. 0, then X(K) is an initial
	                                      element.

	                   If JPVT(K) .EQ. 0, then X(K) is a free element.

	                   If JPVT(K) .LT. 0, then X(K) is a final element.

	                Before the decomposition is computed, initial elements
	                are moved by symmetric row and column interchanges to
	                the beginning of the array A and final
	                elements to the end.  Both initial and final elements
	                are frozen in place during the computation and only
	                free elements are moved.  At the K-th stage of the
	                reduction, if A(K,K) is occupied by a free element
	                it is interchanged with the largest free element
	                A(L,L) with L .GE. K.  JPVT is not referenced if
	                JOB .EQ. 0.

	        JOB     INTEGER.
	                JOB is an integer that initiates column pivoting.
	                If JOB .EQ. 0, no pivoting is done.
	                If JOB .NE. 0, pivoting is done.

	     On Return

	         A      A contains in its upper half the Cholesky factor
	                of the matrix A as it has been permuted by pivoting.

	         JPVT   JPVT(J) contains the index of the diagonal element
	                of a that was moved into the J-th position,
	                provided pivoting was requested.
	                Note for C: jpvt(j) contains index + 1

	         INFO   contains the index of the last positive diagonal
	                element of the Cholesky factor.
	                Note for C: info contains index + 1

	     For positive definite matrices INFO = P is the normal return.
	     For pivoting with positive semidefinite matrices INFO will
	     in general be less than P.  However, INFO may be greater than
	     the rank of A, since rounding error can cause an otherwise zero
	     element to be positive.  Indefinite systems will always cause
	     INFO to be less than P.

	     LINPACK.  This version dated 03/19/79 .
	     J.Dongarra J. and Stewart G. W., Argonne National Laboratory and
	     University of Maryland.


	     BLAS DAXPY,DSWAP
	     Fortran DSQRT
	***REFERENCES  DONGARRA J.J., BUNCH J.R., MOLER C.B., STEWART G.W.,
	                 *LINPACK USERS  GUIDE*, SIAM, 1979.
	***ROUTINES CALLED  DAXPY,DSWAP
	***END PROLOGUE  DCHDC
*/

void DCHDC(double *a, INT *plda, INT *pp, double *work, INT jpvt[],
		   INT *pjob, INT *info)
/*double a[lda][1], work[1];*/
{
	INT         pu, pl, j, k, l, maxl, jtemp;
	INT         inc = 1;
	double      temp;
	double      maxdia;
	double     *ak, *apl, *akk, *aj, *apu, *all, *amaxl;
	INT         swapk, negk, length;
	INT         lda = *plda, p = *pp, job = *pjob;

	/* ***first executable statement  dchdc */
	pl = 0;
	pu = -1;
	*info = p;

	if (job != 0)
	{
		/*         pivoting has been requested. rearrange the */
		/*         the elements according to jpvt. */

		ak = a;
		apl = a + pl*lda;
		for(k = 0;k < p ;k++)
		{
			akk = ak + k;
			swapk = jpvt[k] > 0;
			negk = jpvt[k] < 0;
			jpvt[k] = k+1;
			if (negk)
			{
				jpvt[k] = -jpvt[k];
			}

			if (swapk)
			{
				if (k != pl)
				{
					DSWAP(&pl, ak, &inc, apl, &inc);
					temp = *akk;
					*akk = apl[pl];
					apl[pl] = temp;
					aj = apl + lda;
					for(j = pl+1;j < p ;j++)
					{
						if (j < k)
						{
							temp = aj[pl];
							aj[pl] = ak[j];
							ak[j] = temp;
						}
						else if (j != k)
						{
							temp = aj[k];
							aj[k] = aj[pl];
							aj[pl] = temp;
						}
						aj += lda;
					} /*for(j = pl+1;j < p ;j++)*/

					jpvt[k] = jpvt[pl];
					jpvt[pl] = k + 1;
				} /*if (k != pl) */
				pl++;
				apl += lda;
			} /*if (swapk) */
			ak += lda;
		} /*for(k = 0;k < p ;k++)*/

		pu = p - 1;
		apu = ak = a + (p-1)*lda;

		for(k = p-1;k>=pl;k--)
		{
			akk = ak + k;
			if (jpvt[k] < 0)
			{
				jpvt[k] = -jpvt[k];
				if (pu != k)
				{
					DSWAP(&k, ak, &inc, apu, &inc);
					temp = *akk;
					*akk = apu[pu];
					apu[pu] = temp;
					aj = ak + lda;
					for(j = k+1;j < p ;j++)
					{
						if (j < pu)
						{
							temp = aj[k];
							aj[k] = apu[j];
							apu[j] = temp;
						}
						else if (j != pu)
						{
							temp = aj[k];
							aj[k] = aj[pu];
							aj[pu] = temp;
						}
						aj += lda;
					} /*for(j = k+1;j < p ;j++)*/

					jtemp = jpvt[k];
					jpvt[k] = jpvt[pu];
					jpvt[pu] = jtemp;
				} /*if (pu != k) */
				pu--;
				apu -= lda;
			} /*if (jpvt[k] < 0) */
			ak -= lda;
		} /*for(k = p-1;k>=pl;k--)*/
	} /*if (job != 0)*/

	ak = a;
	for(k = 0;k < p ;k++)
	{
		/*         reduction loop. */

		akk = ak + k;
		maxdia = *akk;
		maxl = k;

		/*         determine the pivot element. */

		if (k >= pl && k < pu)
		{
			all = akk + lda + 1;
			for(l = k+1;l <= pu ;l++)
			{
				if (*all > maxdia)
				{
					maxdia = *all;
					maxl = l;
				}
				all += lda + 1;
			}
		}

		/*         quit if the pivot element is not positive. */

		if (maxdia <= 0.0e0)
		{
			*info = k;
			break;
		}

		if (k != maxl)
		{
			amaxl = a + maxl*lda;
			/*            start the pivoting and update jpvt. */

			DSWAP(&k, ak, &inc, amaxl, &inc);
			amaxl[maxl] = *akk;
			*akk = maxdia;
			jtemp = jpvt[maxl];
			jpvt[maxl] = jpvt[k];
			jpvt[k] = jtemp;
		} /*if (k != maxl) */

		/*         reduction step. pivoting is contained across the rows. */

		work[k] = sqrt(*akk);
		*akk = work[k];
		aj = ak + lda;
		amaxl = a + maxl*lda;
		for(j = k+1;j < p ;j++)
		{
			if (k != maxl)
			{
				temp = aj[k];
				if (j < maxl)
				{
					aj[k] = amaxl[j];
					amaxl[j] = temp;
				}
				else if (j != maxl)
				{
					aj[k] = aj[maxl];
					aj[maxl] = temp;
				}
			} /*if (k != maxl)*/

			aj[k] /= work[k];
			work[j] = aj[k];
			temp = -aj[k];
			length = j - k;
			DAXPY(&length, &temp, work + k + 1, &inc, aj + k + 1, &inc);
			aj += lda;
		} /*for(j = k+1;j < p ;j++)*/
		incAndTest((p-k+3)*(p-k)/2,errorExit);
		ak += lda;
	}/*for(k = 0;k < p ;k++)*/
	/* fall through*/

  errorExit:
	;

} /*dchdc()*/



