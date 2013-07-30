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
#pragma segment Screen
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
#include <math.h>

#ifndef LOOPCHUNK
#define LOOPCHUNK   50  /* check for interrupt every LOOPCHUNK times past test*/
#endif /*LOOPCHUNK*/


#define ci (*CI)
#define cn (*CN)
#define ns (*NS)
#define nd (*ND)
#define df (*DF)
#define ib (*IB)
#define mb (*MB)
#define iz (*IZ)
#define mn (*MN)
#define ky (*KY)
#define sig (*SIG)
#define pen (*pen)
#define zip (*ZIP)
#define kx (*KX)
#define kz (*KZ)
#define iv *IV
#define xi(i,j) ( XI[(i-1) + nd * (j-1)] )
#define xn(i,j) ( XN[(i-1) + nd * (j-1)] )
#define d(i) ( D[i-1] )
#define yi(i) ( YI[i-1] )
#define co(i) ( CO[i-1] )
#define cl(i) ( CL[i-1] )
#define rm(i) ( RM[i-1] )
#define ni(i) ( NI[i-1] )
#define ind(i) ( IND[i-1] )
/*
#                           leaps and bounds
*/
/*
  990316 Converted declaration to standard C form, using Name [] to
  indicate arrays and * Name to indicate pointers to scalars
*/
long leapslandb(double XI [], double XN [], double D [], double YI [],
				double * CI, double * CN, double CO [], double CL [],
				double RM [], long IND [], long * NS, long * ND,
				double * DF, long * IB, long * MB, long * IZ, long * MN,
				long * KY, double * SIG, double * PEN, double * ZIP,
				long * KX, long * KZ, long NI [], long * IV)
{
	double          rs, bnd, tmpd;
	long            ip, l, n, ls, tmpi0, tmpi1, tmpi2, tmpi3, nn, lo,
	                mr, ld;
	long            lb, il, m, tmpi4, lc;
#ifdef MACINTOSH
	long            loopcounter = 0;
#endif /*MACINTOSH*/
	WHERE("leapslandb");

	ni(iz) = kx;
	ip = iz;
	yi(iz) = xi(ky, ky);
	if (iv != 0)
	{
		/*
		#                  initial p+1 variable regressions
		*/
		for (l = iz; l <= kx; l++)
		{
			n = ind(l);
			rs = xn(ky, ky) - xn(n, ky) * xn(n, ky) / xn(n, n);
			tmpd = cn + co(n);
			leapsqstore(&rs, &tmpd, CL, RM, MB, DF, MN, SIG, PEN, IB, NS);
		} /*for (l = iz; l <= kx; l++)*/
	} /*if (iv != 0)*/
	
	for (;;)
	{
		/*
		#                            first fathoming
		*/
		ls = ip + 1 + iv;
		for (lb = ls; lb <= kx; lb++)
		{
			tmpi0 = mn + kx - lb + iv;
			leapstrans(&bnd, &mr, YI + ip - 1, DF, &tmpi0, SIG, PEN, IB, MB);
			if (bnd < rm(mr))
			{
				break;
			}
		}
		if (bnd <= rm(mr))
		{
			ld = kx + ls - lb;
			if (ip <= iz)
			{
				/*
				#                           finish inversion
				*/
				for (l = iz; l <= kx; l++)
				{
					n = ind(l);
					xi(n, n) = -d(n);
					tmpi0 = 0;
					tmpi1 = 0;
					tmpi2 = l - 1;
					tmpi3 = 3;
					leapspivot(&n, &tmpi0, IZ, &tmpi2, XI,
							   IND, IND, CI, ZIP, D, &tmpi3,
							   ND, &nn, KY, IZ, &tmpi1);
				}
				lo = kx;
			} /*if (ip <= iz)*/
			else
			{
				/*
				#                          extend prior pivots
				*/
				if (iv != 1 && ld > ni(ip - 1))
				{
					il = ip - 2;
					for (l = iz; l <= il; l++)
					{
						if (ld > ni(l + 1))
						{
							m = ind(l);
							if (xn(m, m) < zip)
							{
								tmpi0 = 0;
								tmpi1 = l + 1;
								tmpi2 = ni(l + 1) + 1;
								tmpi3 = 3;
								tmpi4 = 0;
								leapspivot(&m, &tmpi0, &tmpi1, &ld, XN,
										   IND, IND, CN, ZIP, D, &tmpi3,
										   ND, NI + l, KY, &tmpi2, &tmpi4);
							}
							if (xn(m, m) > zip)
							{
								tmpi0 = 0;
								tmpi1 = l + 1;
								tmpi2 = ni(l + 1) + 1;
								tmpi3 = 3;
								tmpi4 = 0;
								leapspivot(&m, &tmpi0, &tmpi1, &ld, XI, IND,
										   IND, CI, ZIP, D, &tmpi3, ND,
										   NI + l, KY, &tmpi2, &tmpi4);
							} /*if (xn(m, m) > zip)*/
						} /*if (ld > ni(l + 1))*/
					} /*for (l = iz; l <= il; l++)*/
				} /*if (iv != 1 && ld > ni(ip - 1))*/
				lo = ld + iv * (kx - ld);
				tmpi0 = 0;
				tmpi1 = 0;
				tmpi2 = 0;
				tmpd = -co(n);
				leapspivot(&n, &tmpi0, &ip, &lo, XI, IND,
						   IND, CI, &tmpd, D, &tmpi1, ND,
						   NI + ip - 1, KY, &ip, &tmpi2);
			} /*if (ip <= iz){}else{}*/
			/*
			#                           bound regressions
			*/
			for (lb = ip; lb <= lo; lb++)
			{
				n = ind(lb);
				rs = xi(ky, ky) - xi(n, ky) * xi(n, ky) / xi(n, n);
				if (ld == kx)
				{
					tmpd = ci - co(n);
					tmpi0 = kx - ip + mn - 1;
					leapsqstore(&rs, &tmpd, CL, RM, MB, DF, &tmpi0,
								SIG, PEN, IB, NS);
				}
				/*
				#                          re-order variables
				*/
				m = lb;
				while (m != ip && rs > yi(m))
				{
					yi(m + 1) = yi(m);
					ind(m) = ind(m - 1);
					m--;
				}
				ind(m) = n;
				yi(m + 1) = rs;
			} /*for (lb = ip; lb <= lo; lb++)*/

			if (ls < kx)
			{
				ld = (ld < kx - 1 ? ld : kx - 1);
				/*
				#                           second fathoming
				*/
				for (lb = ls; lb <= ld; lb++)
				{
					lc = ld + ls - lb;
					tmpi0 = mn + ld - lb + iv;
					leapstrans(&bnd, &mr, YI + lc, DF, &tmpi0,
							   SIG, PEN, IB, MB);
					if (bnd < rm(mr))
					{
						break;
					}
				}
				if (bnd < rm(mr))
				{
					/*
					#                 fixed and p+1 variable regressions
					*/
					for (lb = ls; lb <= lc; lb++)
					{
						ip = lb - iv;
						n = ind(ip - 1);
						tmpi0 = 0;
						tmpi1 = lc + iv * (kz - lc) - 1;
						tmpi2 = 0;
						tmpi3 = 0;
						leapspivot(&n, &tmpi0, &ip, &tmpi1, XN, IND, IND,
								   CN, CO + n - 1, D, &tmpi2,
								   ND, NI + ip - 1, KY, &ip, &tmpi3);
						if (iv != 1)
						{
							leapsqstore(XN + ky - 1 + (ky - 1) * nd, CN, CL,
										RM, MB, DF, MN, SIG, PEN, IB, NS);
							mn++;
						} /*if (iv != 1)*/
						else
						{
							mn++;
							for (l = ip; l <= kx; l++)
							{
								m = ind(l);
								rs = xn(ky, ky) - xn(m, ky) * xn(m, ky) / xn(m, m);
								tmpd = cn + co(m);
								leapsqstore(&rs, &tmpd, CL, RM, MB, DF, MN,
											SIG, PEN, IB, NS);
							} /*for (l = ip; l <= kx; l++)*/
						} /*if (iv != 1){}else{}*/
					} /*for (lb = ls; lb <= lc; lb++)*/
				} /*if (bnd < rm(mr))*/
			} /*if (ls < kx)*/
		} /*if (bnd <= rm(mr))*/
		else
		{
			ip--;
		}
		for (;;)
		{
			/*
			#                               backtrack
			*/
			if (ip <= iz)
			{
				return (1);
			}
#ifdef MACINTOSH
			loopcounter++;
			if(interrupted(DEFAULTTICKS) != INTNOTSET)
			{
				return (0);
			}
#endif /*MACINTOSH*/
			n = ind(ip - 1);
			if (xn(n, n) < zip)
			{
				break;
			}
			tmpi0 = 0;
			tmpi1 = 0;
			tmpi2 = 0;
			leapspivot(&n, &tmpi0, &ip, NI + ip - 1, XI, IND,
					   IND, CI, CO + n - 1, D, &tmpi1,
					   ND, &nn, KY, &ip, &tmpi2);
			ip = ip - 1;
		} /*for (;;)*/
		mn--;
		tmpi0 = 0;
		tmpi1 = 0;
		tmpi2 = 0;
		tmpd = -co(n);
		leapspivot(&n, &tmpi0, &ip, NI + ip - 1, XN, IND,
				   IND, CN, &tmpd, D, &tmpi1, ND, &nn, KY, &ip, &tmpi2);
	} /*for (;;)*/
} /*leapslandb()*/
