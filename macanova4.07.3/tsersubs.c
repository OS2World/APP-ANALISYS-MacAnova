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
#pragma segment Tsersubs
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
#include "tser.h"

/*
   This includes to following functions:
   qonerg	dispatcher for of hconj, cconj, hreal, himag, creal, cimag,
			ctoh, htoc, revers
   qtworg	dispatcher for hprdh, hprdhj, cprdc, cprdcj, cmplx
   qconj	computation of hconj, cconj
   qxprd	computation of hprdh, hprdhj, cprdc, cprdcj
   qconv    computation of convolve
   qreal	computation of creal, cimag, hreal, himag
   qcmplx	computation of cmplx
   qrect	computation of hpolar, cpolar, hrect, crect and dispatch unwind
   qhcopy	computation of ctoh, htoc
   qpadto	computation of padto
   qrota	computation of rotate
   qreverse	computation of reverse
   qunwnd   computation of unwind

*/
/* qconj.c */


/* qonerg.c */



/*
 	function to control various one-argument fft-related routines
 	iop	    op		iop	    op
 	IHCONJ hconj	IHIMAG himag
 	ICCONJ cconj	ICIMAG cimag
 	IHREAL hreal	IHTOC  htoc
 	ICREAL creal	ICTOC  ctoh

 	version of 950605
	981213 removed unused argument 5 which specified the
           number of columns in the output matrix b.
*/

void qonerg(double * a, double * b, long m, long n, /*long n1,*/long iop)
/*double a[m][n], b[m][n1];*/
{
	WHERE("qonerg");
	
	if (iop & (IHCONJ | ICCONJ))
	{
		qconj(a, b, m, n, iop);
	}
	else if (iop & (IHREAL | IHIMAG | ICREAL | ICIMAG))
	{
		qreal(a, b, m, n, iop);
	}
	else if (iop & (ICTOH | IHTOC))
	{
		qhcopy(a, b, m, n, iop);
	}
	else if (iop & IREVERSE)
	{
		qreverse(a, b, m, n);
	}
} /*qonerg()*/

/* qtworg.c */

/*
 	function to control various two-argument fft-related routines
 	iop      op    iop	    op
 	IHPRDH  hprdh  ICPRDCJ cprdcj
 	IHPRDHJ hprdhj ICMPLX  cmplx
 	ICPRCJ  cprdc

 	version of 950605
*/

void                 qxprd(), qcplx();

void qtworg(double * a, double * b, double * c, long m, long n1,
			long n2, long iop)
/*double a[m][n1], b[m][n2], c[m][n1]; */
{
	if (iop & IPRD)
	{
		qxprd(a, b, c, m, n1, n2, iop);
	}
	else if (iop & ICMPLX)
	{
		qcmplx(a, b, c, m, n1);
	}
} /*qtworg()*/

/*
  function to compute complex conjugate of hermitian or complex
  matrix according as iop = IHCONJ or ICCONJ.

*/

void            qconj(double * a, double * b, long m, long n, long iop)
{
	double         *ajre, *ajim;
	double         *bjre, *bjim;
	double         *aj, *bj;
	long            i, ir, j, m1, nn;
	long            different = (a != b);
	
	if (iop & ICCONJ)
	{
		ajre = a;
		bjre = b;
		nn = n / 2;
		for (j = 0; j < nn; j++)
		{
			ajim = ajre + m;
			bjim = bjre + m;
			if (different)
			{
				for (i = 0; i < m; i++)
				{
					bjre[i] = ajre[i];
				}
			}
			for (i = 0; i < m; i++)
			{
				bjim[i] = -ajim[i];
			}
			ajre += 2*m;
			bjre += 2*m;
		} /*for (j = 0; j < nn; j++)*/

		if (n % 2 != 0)
		{ /* missing imaginary part for last column; treat as if zero */
			bjim = bjre + m;
			for (i = 0; i < m; i++)
			{ /* must be different */
				bjre[i] = ajre[i];
				bjim[i] = 0.0;
			}
		} /*if (n % 2 != 0)*/
	} /*if (iop & ICCONJ)*/
	else
	{/* hconj */
		m1 = m / 2;
		
		aj = a;
		bj = b;
		for (j = 0; j < n; j++)
		{
			if (different)
			{
				for (i = 0; i <= m1; i++)
				{
					bj[i] = aj[i];
				}
			}
			for(ir = m-1;ir > m1; ir--)
			{
				bj[ir] = -aj[ir];
			}
			aj += m;
			bj += m;
		} /*for (j = 0; j < n; j++)*/
	} /*if (iop & ICCONJ){}else{}*/
	return;
} /*qconj()*/

/* qxprd.c */

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

/*
    function to carry out elementwise multiplication of
    complex and hermitian matrices. iop == IHPRDHJ + 0,1,2,3 means
    hprdh, hprdhj, cprdc, cprdcj.

	981106 fixed bug related to cprdc() and cprdcj() when one or both
           arguments have an odd number of columns
*/
void            qxprd(double * a, double * b, double * c, long m, long n1,
					  long n2, long iop)
{
	long            m1, m2, i, j, ir, nn, iwhich, twom;
	long            n, inc1, inc2, n1odd, n2odd;
	long            odd;
	double          amult, t1, t2;
	double         *aj, *bj, *cj;
	double         *ajre, *bjre, *cjre;
	double         *ajim, *bjim, *cjim;
	double          temp;
	WHERE("qxprd");
	
	odd = m & 1;
	amult = (iop & (IHPRDHJ | ICPRDCJ)) ? -1.0 : 1.0;
	
	if (iop & (IHPRDH | IHPRDHJ))
	{/* hprdh (amult == 1), hprdhj (amult == -1) */
		inc1 = (n1 == 1) ?  0 : m;
		inc2 = (n2 == 1) ?  0 : m;
		m1 = m / 2;
		m2 = (m - 1) / 2;
		n = MAX(n1, n2);

		aj = a;
		bj = b;
		cj = c;
		for (j = 0; j < n; j++)
		{
			if (interrupted(DEFAULTTICKS) != INTNOTSET)
			{
				return;
			}
			cj[0] = aj[0] * bj[0];
			for (i = 1, ir = m-1; i <= m2; i++, ir--)
			{
				temp = aj[i] * bj[i] - amult * aj[ir] * bj[ir];
				cj[ir] = amult * aj[i] * bj[ir] + aj[ir] * bj[i];
				cj[i] = temp; /*insurance in case storage re-used*/
			}
			if (!odd)
			{
				cj[m1] = aj[m1] * bj[m1];
			}
			aj += inc1;
			bj += inc2;
			cj += m;
		} /*for (j = 0; j < n1; j++)*/
	} /*if (iop & (IHPRDH | IHPRDHJ))*/
	else
	{/* cprdc (amult == 1), cprdcj (amult == -1) */
		
		n1odd = (n1 & 1) != 0;
		n2odd = (n2 & 1) != 0;
		nn = MAX(n1, n2);
		if((n1odd || n2odd) && (nn & 1) == 0)
		{
			nn--;
		}
		nn /= 2;
		
		twom = 2*m;
		inc1 = (n1 <= 2) ? 0 : twom;
		inc2 = (n2 <= 2) ? 0 : twom;
		ajre = a;
		bjre = b;
		cjre = c;
		for (j = 0; j < nn; j++)
		{
			if (interrupted(DEFAULTTICKS) != INTNOTSET)
			{
				return;
			}

			ajim = ajre + m;
			bjim = bjre + m;
			cjim = cjre + m;

			for (i = 0; i < m; i++)
			{
				temp = ajre[i] * bjre[i] - amult * ajim[i] * bjim[i];
				cjim[i] = amult * ajre[i] * bjim[i] + ajim[i] * bjre[i];
				cjre[i] = temp; /*insurance in case storage re-used*/
			}
			ajre += inc1;
			bjre += inc2;
			cjre += twom;
		} /*for (j = 0; j < nn; j++)*/

		if (interrupted(DEFAULTTICKS) != INTNOTSET)
		{
			return;
		}
		if (n1odd || n2odd)
		{
			/*
			  iwhich =1, 2, or 3 according as a is short and b ok,
			  a is ok and b short, or both a and b are short.
			  */
			ajim = ajre + m;
			bjim = bjre + m;
			cjim = cjre + m;
			iwhich = (n1odd && !n2odd) ? 1 : ((!n1odd && n2odd) ? 2 : 3);
			t1 = t2 = 0.0;
			for (i = 0; i < m; i++)
			{
				if (iwhich == 1)
				{
					t2 = bjim[i];
				}
				else if (iwhich == 2)
				{
					t1 = ajim[i];
				}

				temp = ajre[i] * bjre[i];
				cjim[i] = amult * ajre[i] * t2 + t1 * bjre[i];
				cjre[i] = temp; /*insurance in case storage re-used*/
			} /*for (i = 0; i < m; i++)*/
		} /*if (n1odd || n2odd)*/
	} /*if (iop & (IHPRDH | IHPRDHJ)){}else{}*/
} /*qxprd()*/

/* qconv.c */


/*
  function to perform circular convolution of a with columns of b.
  reverse == 0    ordinary convolution
  reverse != 0    sums of lagged products
*/

void qconv(double * a, double * b, double * c, long m1, long m2, long n,
		   long decimate, long reverse)
/*double a[m1], b[m2][n], c[m2][n];*/
{
	long           inck, incl, is, it, j, k, l;
	long           start, end;
	long           nrowsOut = (m2 - 1)/decimate + 1;
	double        *cj, *bj;
	double         s;
	WHERE("qconv");
	
	if(!reverse)
	{ /* regular convolution, subscripts move in opposite directions */
		inck = -1;
		end = -1;
		start = m2-1;
	}
	else
	{ /* reverse convolution, subscripts move in same direction */
		inck = 1;
		start = 0;
		end = m2;
	}
	incl = decimate * inck;
	bj = b;
	cj = c;
	
	for(j=0;j<n;j++)
	{
		l = 0;
		for(it=0;it<nrowsOut;it++)
		{
#ifdef MACINTOSH
			if(checktime(it) && interrupted(DEFAULTTICKS) != INTNOTSET)
			{
				return;
			}
#endif /*MACINTOSH*/
			k = l;
			s = 0.0;
			for(is=0;is<m1;is++)
			{
				s += a[is] * bj[k];
				k += inck;
				if(k == end)
				{
					k = start;
				}
			} /*for(is=0;is<m1;is++)*/
			cj[it] = s;
			l -= incl;
			if(it==0 && reverse)
			{
				l += m2;
			}
		} /*for(it=0;it<nrowsOut;it++)*/
		bj += m2;
		cj += nrowsOut;
	} /*for(j=0;j<n;j++)*/
} /*qconv()*/

/* qreal.c */


/*
  function to take real and imaginary parts of hermitian and
  complex matrices. iop = IHREAL, IHIMAG, ICREAL, ICIMAG mean
  hreal,himag,creal,cimag
*/

void qreal(double * a, double * b, long m, long n, long iop)
/*double a[m][n], b[m][n];*/
{
	long            m1, m2, i, j, ir, nn;
	double         *aj, *bj;
	long            nOdd, mOdd, realOp;

	realOp = iop & (IHREAL | ICREAL);

	if (iop & (ICREAL | ICIMAG))
	{
		nOdd = n & 1;
		aj = (realOp) ? a : a + m;
		bj = b;
		nn = n/2;
		if(realOp && nOdd)
		{
			nn++;
		}
		
		for(j=0;j<nn ;j++)
		{ 
			if (interrupted(DEFAULTTICKS) != INTNOTSET)
			{
				return;
			}
			for(i=0;i<m;i++)
			{
				bj[i] = aj[i];
			}
			aj += 2*m;
			bj += m;
		} /*for(j=0;j<nn ;j++)*/
		if (!realOp && nOdd) 
		{ /* fill out last column with 0's */
			for(i=0;i<m;i++)
			{
				bj[i] = 0.0;
			}
		}
	} /*if (iop & (ICREAL | ICIMAG))*/
	else 
	{ /* hreal, himag */
		m1 = m/2;
		m2 = (m+1)/2;
		mOdd = m & 1;
		aj = a;
		bj = b;
		for(j=0;j<n ;j++)
		{		
			if (interrupted(DEFAULTTICKS) != INTNOTSET)
			{
				return;
			}
			bj[0] = (realOp) ? aj[0] : 0.0;
			if(!mOdd)
			{
				bj[m1] = (realOp) ? aj[m1] : 0.0;
			}
			for(i=1, ir=m-1;i<m2;i++, ir--)
			{
				bj[i] = (realOp) ? aj[i] : aj[ir];
				bj[ir] = (realOp) ? bj[i] : -bj[i];
			}
			aj += m;
			bj += m;
		} /*for(j=0;j<n ;j++)*/
	} /*if (iop & (ICREAL | ICIMAG)){}else{}*/
} /*qreal()*/

/* qcmplx.c */


/*
  function to create complex matrix from real and imaginary
  parts in a and b.
*/

void qcmplx(double * a, double * b, double * c, long m, long n)
/*double a[m][n], b[m][n], c[m][n];*/
{
	long        i, j;
	double     *aj = a, *bj = b, *cjre, *cjim;
	long        breal = (b == (double *) 0);
	
	
	cjre = c;
	for(j=0;j<n;j++)
	{
		cjim = cjre + m;
		for(i=0;i<m;i++)
		{
			cjre[i] = aj[i];
			cjim[i] = (breal) ? 0.0 : bj[i];
		} /*for(i=0;i<m;i++)*/
		aj += m;
		bj += m;
		cjre += 2*m;
	} /*for(j=0;j<n;j++)*/
} /*qcmplx()*/

/* qrect.c */

/*
    subroutine to convert hermitian and complex series from
    rectangular (normal) form to polar and vice versa,
    and "unwind" phase functions.
      iop   operation    units  angles
      IHPOLAR hpolar        1    radians
      IHRECT  hrect         2    degrees
      ICPOLAR cpolar        3    cycles
      ICRECT  crect
      IUNWIND unwind
  if cpolar or crect b is m by 2*((n+1)/2)
*/
/* 
  MacAnova note: There is no argument 'units' since that is taken from global
  ANGLES2RADIANS
*/
/*
  980310 fixed bug; when ncols of imaginary argument was odd,
         it was not doing the right thing with the last column of
         of the output
*/
void qrect(double * a, double * b, long m, long n,
		   long unwind, double crit, long iop)
/*double a[m][n], b[m][n], crit;*/
{
	long             opPolar;
	long             m1, m2, i, ir, j, nn;
	long             mOdd, nOdd;
	double           factor, twopi, fullcycle, halfcycle, s, t;
	double          *aj, *bj;
	double          *ajre, *ajim;
	double          *bjre, *bjim;
	WHERE("qrect");

	opPolar = (iop == IHPOLAR||iop == ICPOLAR);
	nOdd = n & 1;
	mOdd = m & 1;

	twopi = 8.0*MV_PI_4;
	factor = ANGLES2RADIANS;
	fullcycle = twopi/factor;
	
	if (iop & (IHRECT| IHPOLAR))
	{
		m1 = m/2;
		m2 = (m+1)/2;
		aj = a;
		bj = b;
		for(j=0;j<n ;j++)
		{
			if (interrupted(DEFAULTTICKS) != INTNOTSET)
			{
				return;
			}
			bj[0] = aj[0];
			if(!mOdd)
			{
				bj[m1] = aj[m1];
			}
			
			for(i=1, ir = m-1;i<m2;i++, ir--)
			{
				s = aj[i];
				t = aj[ir];
				if(!opPolar)
				{
					t *= factor;
					bj[i] = s*cos(t);
					bj[ir] = s*sin(t);
				}
				else
				{
					bj[i] = pythag(s, t);
					bj[ir] = atan2(t, s)/factor;
				}
			} /*for(i=1, ir = m-1;i<m2;i++, ir--)*/
			if (unwind && opPolar)
			{
				qunwnd(bj, m, fullcycle, crit, IHERMITIAN);
			}
			aj += m;
			bj += m;
		} /*for(j=0;j<n ;j++)*/
	} /*if (iop & (IHRECT| IHPOLAR))*/
	else if (iop & (ICRECT| ICPOLAR))
	{
		/* ****cpolar, crect */
		nn = n/2;
		ajre = a;
		bjre = b;
		for(j=0;j<nn ;j++)
		{
			if (interrupted(DEFAULTTICKS) != INTNOTSET)
			{
				return;
			}
			ajim = ajre + m;
			bjim = bjre + m;
			for(i=0;i<m ;i++)
			{
				s = ajre[i];
				t = ajim[i];
				if (!opPolar)
				{
					t *= factor;
					bjre[i] = cos(t)*s;
					bjim[i] = sin(t)*s;
				}
				else
				{
					bjre[i] = pythag(s, t);
					bjim[i] = atan2(t, s)/factor;
				}
			} /*for(i=0;i<m ;i++)*/
			if (unwind && opPolar)
			{
				qunwnd(bjim, m, fullcycle, crit, IREAL);
			}
			ajre += 2*m;
			bjre += 2*m;
		} /*for(j=0;j<nn ;j++)*/

		if (nOdd)
		{
			if (interrupted(DEFAULTTICKS) != INTNOTSET)
			{
				return;
			}
			/* ****add final column if n is not even. */
			halfcycle = 0.5*fullcycle;
			bjim = bjre + m;
			for(i = 0;i<m;i++)
			{
				if(!opPolar)
				{
					bjre[i] = ajre[i];
					bjim[i] = 0.0;
				}
				else
				{
					s = ajre[i];
					bjre[i] = fabs(s);
					bjim[i] = (s >= 0.0) ? 0.0 : halfcycle;
				}
			} /*for(i = 0;i<m;i++)*/
		} /*if (nOdd)*/
	}
	else
	{ /* unwind */
		aj = a;
		bj = b;
		for(j = 0;j < n;j++)
		{
			if (interrupted(DEFAULTTICKS) != INTNOTSET)
			{
				return;
			}
			if(b != a)
			{
				for(i=0;i<m;i++)
				{
					bj[i] = aj[i];
				}
			}
			qunwnd(bj, m, fullcycle, crit, IREAL);
			aj += m;
			bj += m;
		} /*for(j = 0;j < n;j++)*/
	}
} /*qrect()*/

/* qhcopy.c */


/*
  function to convert from hermitian to full complex and
  vice versa.  iop = ICTOH, IHTOC means ctoh, htoc
*/

void qhcopy(double * a, double * b, long m, long n, long iop)
/*double a[m][n], b[m][n];*/
{
	long              m1, m2, j, i, ir, nn;
	long              mOdd, nOdd;
	double           *aj, *bj;
	double           *ajre, *ajim, *bjre, *bjim;
	
	mOdd = m & 1;
	m1 = m/2;
	m2 = (m+1)/2;
	if (iop & IHTOC)
	{
		/* **** htoc */
		
		aj = a;
		bjre = b;
		for(j=0;j<n ;j++)
		{
			bjim = bjre + m;
			bjre[0] = aj[0];
			bjim[0] = 0.0;
			
			if(!mOdd)
			{
				bjre[m1] = aj[m1];
				bjim[m1] = 0.0;
			}
			
			for(i = 1, ir = m-1;i<m2;i++, ir--)
			{
				bjre[ir] = bjre[i] = aj[i];
				bjim[ir] = -(bjim[i] = aj[ir]);
			}
			aj += m;
			bjre += 2*m;
		} /*for(j=0;j<n ;j++)*/
	} /*if (iop & IHTOC)*/
	else
	{ /* ctoh */
		nn = n/2;
		nOdd = n & 1;
		ajre = a;
		bj = b;
		for(j=0;j<nn ;j++)
		{
			ajim = ajre + m;
			bj[0] = ajre[0];
			
			if(!mOdd)
			{
				bj[m1] = ajre[m1];
			}
			
			for(i = 1, ir = m-1;i < m2;i++, ir--)
			{ /* hermitian symmetrization of output matrix */
				bj[i] = 0.5*(ajre[i] + ajre[ir]);
				bj[ir] = 0.5*(ajim[i] - ajim[ir]);
			}
			ajre += 2*m;
			bj += m;
		} /*for(j=0;j<nn ;j++)*/
		if (nOdd)
		{ /* a has odd number of columns */
			bj[0] = ajre[0];
			if(!mOdd)
			{
				bj[m1] = ajre[m1];
			}
			for(i = 1, ir = m-1;i < m2; i++, ir--)
			{
				bj[i] = 0.5*(ajre[i] + ajre[ir]);
				bj[ir] = 0.0;
			} /*for(i = 1;i < m2; i++)*/
		} /*if (nOdd)*/
	} /*if (iop & IHTOC){}else{}*/
} /*qhcopy()*/

/* qpadto.c */


/*
  function to add rows zeros to in or delete rows from in so as to make
  out m2 by n.
  Adapted from Fortran 920420
  Version of 940915
 */

void qpadto(double * in, double * out, long m1, long n, long m2)
/*double in[m1][n], out[m2][n];*/
{
	long             i, j;
	double          *inj = in;
	double          *outj = out;

	for(j = 0;j < n; j++)
	{
		for(i=0;i<m2 ;i++)
		{
			outj[i] = (i < m1) ? inj[i] : 0.0;
		}
		inj += m1;
		outj += m2;
	} /*for(j = 0;j < n; j++)*/
} /*qpadto()*/

/* qrota.c */



/*
  subroutine to 'rotate' columns of a into b, offsetting
  them circularly by krot.
*/
void qrota(double * a, double * b, long m, long n, long krot)
/*double a[m][n], b[m][n];*/
{
	long        i, is, j;
	double     *aj = a, *bj = b;
	WHERE("qrota");
	
	krot %= m;

	if (krot < 0)
	{
		krot += m;
	}
	
	for(j=0;j<n;j++)
	{
		if (interrupted(DEFAULTTICKS) != INTNOTSET)
		{
			return;
		}
		for(i = 0, is = krot;i< m; i++)
		{
			bj[is++] = aj[i];
			if(is == m)
			{
				is = 0;
			}
		} /*for(i = 0, is = krot;i< m; i++)*/
		aj += m;
		bj += m;
	} /*for(j=0;j<n;j++)*/
} /*qrota()*/

/*
  function to reverse the rows of a into b.
*/
void qreverse(double * a, double * b, long m, long n)
{
	long        i, ir, j, m2 = m/2, odd = (2*m2 != m);
	double     *aj = a, *bj = b;
	double      temp;
	WHERE("qreverse");
	
	for(j=0;j<n;j++)
	{
		if (interrupted(DEFAULTTICKS) != INTNOTSET)
		{
			return;
		}
		for(i = 0, ir = m-1;i< m2; i++, ir--)
		{ /* coded so it is safe for a to be the same as b*/
			temp = aj[i];
			bj[i] = aj[ir];
			bj[ir] = temp;
		} /*for(i = 0;i< m2; i++)*/
		if(odd)
		{
			bj[m2] = aj[m2];
		}
		aj += m;
		bj += m;
	} /*for(j=0;j<n;j++)*/
} /*qreverse()*/

/* qunwnd.c */


/*
  Function to 'unwind' phase functions.
   iop    meaning
    1     phase is to be real (e.g., imag part of cpolar output)
    2     phase is hermitian (hpolar output), and only 2nd half is modified.
   if the phase changes by more than +-crit cycles (crit should be
   between 0.5 and 1.0), it is assumed to be 'wrapping around'
   and a multiple of twopi  (2*pi, 360, or 1 depending on units)
   is added or subtracted, in an attempt to eliminate meaningless
   jumps in phase functions.
*/

void qunwnd(double * phase, long m, double oneCycle, double crit, long iop)
/*double phase[m], oneCycle, crit;*/
{
	long            i, k, inc, m2;
	double          phlast, diffph;
	WHERE("qunwnd");

	crit = oneCycle*crit;

	if (iop==IREAL) 
	{
		/* ****initialize for iop eq 1 (full real form) */
		phlast = phase[0];
		inc = 1;
		k = 0;
		m2 = m;
	}
	else 
	{
		/* ****initialize for iop eq 2 (hermitian form) */
		phlast = (phase[0] >= 0) ? 0.0 : oneCycle/2.0;
		inc = -1;
		k = m;
		m2 = (m+1)/2;
	}
	for(i=1;i<m2 ;i++)
	{
		k += inc;
		diffph = phase[k] - phlast;
		phlast = phase[k];
		while(diffph > crit)
		{
			diffph -= oneCycle;
			phase[k] -= oneCycle;
		}
		while (diffph < -crit)
		{
			diffph += oneCycle;
			phase[k] += oneCycle;
		}
		phlast = phase[k];
	} /*for(i=1;i<m ;i++)*/
} /*qunwnd()*/

