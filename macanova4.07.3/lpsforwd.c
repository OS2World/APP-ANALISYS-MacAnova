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

/*
  990215 changed myerrorout() to putOutMsg() and putOUTSTR() to
         putErrorOUTSTR()
*/
#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment Screen
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

#define ns (*NS)
#define nd (*ND)
#define kx (*KX)
#define no (*NO)
#define it (*IT)
#define nf (*NF)
#define ib (*IB)
#define mb (*MB)
#define s2 (*S2)
#define tl (*TL)
#define nv (*NV)
#define fd (*FD)
#define ne (*leapsNE)
#define iv (*IV)

#define rr(i,j) ( RR[(i-1) + nd*(j-1)] )
#define xi(i,j) ( XI[(i-1) + nd*(j-1)] )
#define xn(i,j) ( XN[(i-1) + nd*(j-1)] )
#define xm(i)   ( XM[(i-1)] )
#define dd(i)   ( DD[(i-1)] )
#define d(i)    ( D[(i-1)] )
#define dt(i)   ( DT[(i-1)] )
#define co(i)   ( CO[(i-1)] )
#define sc(i)   ( SC[(i-1)] )
#define sq(i)   ( SQ[(i-1)] )
#define cl(i)   ( CL[(i-1)] )
#define rm(i)   ( RM[(i-1)] )
#define inn(i)  ( INN[(i-1)] )
#define ind(i)  ( IND[(i-1)] )
#define myint(i)  ( INT[(i-1)] )
#define ipp(i)  ( IPP[(i-1)] )

/* converted fortran, send proper pointer for array element in function call */

/*
#                      bounds from forward pivots
*/
/*
  990316 converted declaration to standard C, using [] to indicate
         arrays rather than pointers
*/
long leapsforwd(long longparms [], double RR [], double XI [], double XN [],
				double XM [], double DD [], double D [], double DT [],
				double CO [], double SC [], double SQ [], double CL [],
				double RM [], long INN [], long IPP [], long IND [],
				long INT [], double * S2, double * TL, double * FD,
				long * leapsNE,  long * IV)
{
	double          ci, cn, zip, tol, tal, tul, rs, sig, bit, df, pen,
	                wt, bnd, wst;
	long            nbit, l, mr, n, nn, ky, tmpi, tmpi0, tmpi1, tmpi2;
	long            kz, m, mn, ls, mh, mt, nb, j, lb;
	long           *NS, *ND, *KX, *NO, *IT, *NF, *IB, *MB, *NV;
	double          tn, dx, sel, di;
	double          tmpd, tmpd2;
	WHERE("leapsforwd");
	
	NS = longparms;
	ND = longparms + 1;
	KX = longparms + 2;
	NO = longparms + 3;
	IT = longparms + 4;
	NF = longparms + 5;
	IB = longparms + 6;
	MB = longparms + 7;
	NV = longparms + 8;

	bit = 2.0;
	nbit = 1;
	while (bit != bit + 1.0)
	{
		nbit++;
		bit += bit;
	}
#if (0) /* test moved to screen() 990310 */
	if (nv > nbit)
	{
		ne = 1;
		putOutErrorMsg("ERROR:  too many variables for screen");
		return (1);
	}
#endif /*0*/
	for (l = 1; l <= nv; l++)
	{
		ind(l) = l;
		myint(l) = 0;
#ifdef USEPOW
		co(l) = pow((double) 2.0, (double) (nv - l));
#else /*USEPOW*/
		co(l) = intpow((double) 2.0, (double) (nv - l));
#endif /*USEPOW*/
	}

	zip = 0.0;
	tmpi0 = 0;
	tmpi1 = 0;
	leapscopy(RR, RR, IND, IND, NV, &tmpi0, NV, ND, SC, &cn, &zip, &tmpi1);
	/*
	#                          check subscript list
	*/
	kz = kx + 1;
	for (l = 1; l <= kz; l++)
	{
		m = inn(l);
		ind(l) = m;
		if (m < 1 || m > nv || myint(m) != 0)
		{
			ne = 2;
			return (1);
		}
		if (rr(m, m) <= zip)
		{
			ne = 3;
			return (1);
		}
		tn = rr(m, m);
		/*
		  scale by powers of 16
		*/
		tmpd = log(tn) / 2.0 / log(16.);
		tmpd = (tmpd >= 0 ? floor(tmpd) : -floor(fabs(tmpd)));
#ifdef USEPOW
		sc(m) = pow(16.0, tmpd);
#else /*USEPOW*/
		sc(m) = intpow(16.0, tmpd);
#endif /*USEPOW*/
		sq(m) = rr(m, m) / (sc(m) * sc(m));
		myint(m) = 1;
	}
	dx = zip;
	pen = fd;
	if (pen <= zip && ib == 3)
	{
		pen = 2.0;
	}
	df = no - (it < 1 ? it : 1);
	sel = 1.0;
	ky = ind(kz);
	/*
	#                            set tolerances
	*/
	tul = sq(ky) / pow(2.0, (double) nbit / 2.);
	tol = (double) (kx * kx) / pow(2.0, (double) nbit / 2.);
	tal = tl;
	if (tal < tol || tal >= 1.0)
	{
		tal = tol;
	}
	tmpi0 = 1;
	tmpi1 = 1;
	leapscopy(XN, RR, IND, IND, &kz, &tmpi0, &kz, ND, SC, &cn, &zip, &tmpi1);
	/*
	#          forced variables and mean square residual for c(p)
	*/
	for (l = 1; l <= kx; l++)
	{
		sig = s2 / (sc(ky) * sc(ky));
		if (l == nf + 1)
		{
			tmpi0 = 0;
			tmpi1 = 1;
			leapscopy(XI, XN, IND, IND, &kz, &tmpi1, &kz, ND, SC, &ci, &cn, &tmpi0);
		}
		if (l > nf && (/*ib != 3 ||*/ sig > zip))
		{ /*compute s2 for all methods if not specified (kb change) */
			break;
		}
		n = ind(l);
		ipp(l) = l + 1;
		tmpi = l - 1;
		leapstest(&wt, &n, XN, INT, D, DD, SQ, DT, &tmpi, &l, ND);
		if (wt < tal && l <= nf)
		{
			ne = 4;
			return (1);
		}
		if (wt < tol)
		{
			ne = 5;
			return (1);
		}
		tmpi = l - 1;
		tmpi0 = l + 1;
		tmpi1 = 1;
		tmpi2 = l + 1;
		leapspivot(&n, &tmpi, &tmpi0, KX, XN, INT, IND, &cn, CO + n - 1,
				   DT, &tmpi1, ND, &nn, &ky, &tmpi2, KX);
		sig = xn(ky, ky) / (df - (double) kx);
	} /*for (l = 1; l <= kx; l++)*/
	if(s2 <= 0.0)
	{
		s2 = sig * (sc(ky) * sc(ky));
	}
	
	/*
	#                load transform storage with worst value
	*/
	tmpd = sq(ky) + sq(ky);
	tmpd2 = sig + sig;
	leapstrans(&wst, &mr, &tmpd, &df, KX, &tmpd2, &pen, IB, MB);
	for (l = 1; l <= ns; l++)
	{
		cl(l) = zip;
		rm(l) = wst;
	}
	mn = nf + 1;
	ipp(mn) = mn;
	for (;;)
	{
		/*
		#                           forward algorithm
		*/
		ls = ipp(mn);
		mh = mn;
		mt = mn;
		if (ls < kx - 2)
		{
			tmpi = mn - 1;
			tmpi2 = mn + kz - ls;
			tmpi0 = 0;
			leapscopy(XN, XI, INT, IND, &tmpi, &ls, &tmpi2, ND, SC, &cn, &ci, &tmpi0);
		} /*if (ls < kx - 2)*/
		for (lb = ls; lb <= kx; lb++)
		{
			mn = mh + lb - ls;
			ipp(mn) = lb + 1;
			/*
			#                             select pivot
			*/
			nb = lb;
			for (l = lb; l <= kx; l++)
			{
				j = ind(l);
				if (mn == 1)
				{
					dd(l) = -xi(j, ky) * xi(j, ky) / xi(j, j);
				}
				else if (mn > 1)
				{
					dd(l) = xi(j, j) / sq(j);
				}
				if (dd(l) < dd(nb))
				{
					nb = l;
				}
			}
			n = ind(nb);
			ind(nb) = ind(lb);
			ind(lb) = n;
			tmpi = mn - 1;
			leapstest(&wt, &n, XI, INT, D, DD, SQ, DT, &tmpi, &mn, ND);
			if (wt < tol)
			{
				break;
			}
			/*
			#                       avoid pivot for last x
			*/
			if (lb < kx)
			{
				tmpi = mn - 1;
				tmpi1 = 1;
				tmpi2 = lb + 1;
				tmpi0 = lb + 1;
				leapspivot(&n, &tmpi, &tmpi0, KX, XI, INT, IND, &ci, CO + n - 1, DT, &tmpi1, ND, &nn, &ky, &tmpi2, KX);
				rs = xi(ky, ky);
			}
			else
			{
				rs = xi(ky, ky) - xi(n, ky) * xi(n, ky) / xi(n, n);
				ci = ci + co(n);
			}
			if (wt >= tal)
			{
				if (rs < tul)
				{
					ne = 6;
					return (1);
				}
				mt = mn + 1;
				leapsqstore(&rs, &ci, CL, RM, MB, &df, &mn, &sig, &pen, IB, NS);
			}
			if (lb == kx)
			{
				ci -= co(n);
			}
		} /*for (lb = ls; lb <= kx; lb++)*/
		if (mt <= mn)
		{
			sel = tal;
		}
		if (ipp(mn) > kx)
		{
			if (wt >= tal && ls < kx - 2)
			{
				tmpi0 = 0;
				tmpi1 = 1;
				tmpi = 0;
				leapspivot(&n, &tmpi0, &kz, KX, XI, INT, IND, &ci, CO + n - 1, DT, &tmpi1, ND, &nn, &ky, &kz, &tmpi);
				/*
				#                        begin branch and bound
				*/
				mn = mh;
				if(!leapslandb(XI, XN, D, DD, &ci, &cn, CO, CL, RM, IND, NS,
						   ND, &df, IB, MB, &ls, &mn, &ky, &sig, &pen, &zip,
							   KX, &kz, INT, IV))
				{
					goto errorExit;
				}
				
				di = (rs - xi(ky, ky)) / sq(ky);
				dx = (dx > fabs(di) ? dx : fabs(di));
				tmpi = mn - 1;
				tmpi1 = mn + kz - ls;
				tmpi0 = 0;
				leapscopy(XI, XN, INT, IND, &tmpi, &ls, &tmpi1, ND, SC,
						  &ci, &cn, &tmpi0);
			} /*if (wt >= tal && ls < kx - 2)*/
			for (;;)
			{
				/*
				#                               backtrack
				*/
				if (mn == 1)
				{
					goto breakout;
				}
				mn = mn - 1;
				n = myint(mn);
				tmpi = mn - 1;
				tmpi0 = 0;
				tmpd = -co(n);
				leapspivot(&n, &tmpi, IPP + mn - 1, KX, XI, INT, IND, &ci,
						   &tmpd, DT, &tmpi0, ND, &nn, &ky, IPP + mn - 1, KX);
				if (mn > nf && mn <= mt)
				{
					if (wt < tol || mn < mh)
					{
						break;
					}
					leapstrans(&bnd, &mr, &rs, &df, &mn, &sig, &pen, IB, MB);
					if (bnd < rm(mr))
					{
						break;
					}
				} /*if (mn > nf && mn <= mt)*/
			} /*for (;;)*/
		} /*if (ipp(mn) > kx)*/
	} /*for (;;)*/
	/*
	#                                output
	*/
  breakout:
	
	for (lb = 1; lb <= ns; lb++)
	{
		if (cl(lb) != zip)
		{
			/*
			#                             decode labels
			*/
			m = 0;
			/*			for (l = 1; l <= nv; l++)*/
			for (l = 1; l <= nv; l++)
			{
				if (cl(lb) >= co(l))
				{
					m++;
					ind(m) = l;
					cl(lb) = cl(lb) - co(l);
				}
			} /*for (l = 1; l <= nv; l++)*/
			ind(m + 1) = ky;
			if(!leapsqprint(IND, &m, RM + lb - 1, SQ + ky - 1, &sig,
							SC + ky - 1, &df, IT, IB, &pen))
			{
				goto errorExit;
			}
			
		}
	}
	
	if (sel < 1.0 && PRINTWARNINGS)
	{
		sprintf(OUTSTR,
				"WARNING: subsets with tolerances less than %11.4g have been",
				sel);
		putErrorOUTSTR();
		putOutMsg("encountered and omitted");
	}
	di = (sq(ky) - xi(ky, ky)) / sq(ky);
	dx = (dx > fabs(di) ? dx : fabs(di));

	if (dx > tol)
	{
		sprintf(OUTSTR,
				"WARNING: largest discrepancy observed for r^2 is %g", dx);
		putErrorOUTSTR();
	}
	return (1);

  errorExit:
	return (0);
} /*leapsforwd()*/
