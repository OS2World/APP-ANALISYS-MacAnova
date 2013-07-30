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

#include <math.h>
#include "globals.h"

#define n (*N)
#define ml (*ML)
#define ls (*LS)
#define lx (*LX)
#define cn (*CN)
#define co (*CO)
#define iq (*IQ)
#define nd (*ND)
#define nn (*NN)
#define ky (*KY)
#define ll (*LL)
#define ly (*LY)
#define xn(i,j) ( XN[(i-1)+ *ND * (j-1)] )
#define dt(i) ( DT[i-1] )
#define ind(i) ( IND[i-1] )
#define myint(i) ( INT[i-1] )
/*
#                             partial sweep
*/
/*
  990316 Converted declaration to standard C form, using Name [] to
  indicate arrays and * Name to indicate pointers to scalars
*/
void leapspivot(long * N, long * ML, long * LS, long * LX, double XN [],
				long INT [], long IND [], double * CN, double * CO,
				double DT [], long * IQ, long * ND, long * NN, long * KY,
				long * LL, long * LY)
/*
dimension xn(nd,nd),dt(nd),ind(nd),int(nd)
*/
{
	double          b;
	long            j, l, k, m;
	nn = lx;
	if (iq != 3)
	{
		cn = cn + co;
		xn(n, n) = -xn(n, n);
		xn(ky, ky) = xn(ky, ky) + xn(n, ky) * xn(n, ky) / xn(n, n);
	} /*if (iq != 3)*/
	if (ml != 0)
	{
		/*
		#                            gauss-jordan
		*/
		for (l = 1; l <= ml; l++)
		{
			j = myint(l);
			b = xn(n, j) / xn(n, n);
			if (iq == 0)
			{
				xn(j, j) = xn(j, j) + b * xn(n, j);
			}
			if (iq == 1)
			{
				xn(j, j) = dt(l);
			}
			for (m = ll; m <= ly; m++)
			{
				k = ind(m);
				xn(j, k) = xn(j, k) + b * xn(n, k);
				xn(k, j) = xn(j, k);
			} /*for (m = ll; m <= ly; m++)*/
		} /*for (l = 1; l <= ml; l++)*/
	} /*if (ml != 0)*/

	if (ll <= lx)
	{
		/*
		#                                gauss
		*/
		for (l = ll; l <= lx; l++)
		{
			j = ind(l);
			b = xn(n, j) / xn(n, n);
			for (m = ls; m <= l; m++)
			{
				k = ind(m);
				xn(j, k) = xn(j, k) + b * xn(n, k);
				xn(k, j) = xn(j, k);
			} /*for (m = ls; m <= l; m++)*/
			xn(j, ky) = xn(j, ky) + b * xn(n, ky);
			xn(ky, j) = xn(j, ky);
		} /*for (l = ll; l <= lx; l++)*/
	} /*if (ll <= lx)*/
} /*leapspivot()*/


#define wt (*WT)
#define mn (*MN)
#define xi(i,j) ( XI[ (i-1) + nd * (j-1) ] )
#define d(i) ( D[i-1] )
#define dd(i) ( DD[i-1] )
#define sq(i) ( SQ[i-1] )
/*
#                           check tolerances
*/
/*
  990316 Converted declaration to standard C form, using Name [] to
  indicate arrays and * Name to indicate pointers to scalars
*/
void leapstest(double * WT, long * N, double XI [], long INT [], double D [],
			   double DD [], double SQ [], double DT [], long * ML,
			   long * MN, long * ND)
{
	long            j, l;

	d(n) = xi(n, n);
	wt = d(n) / sq(n);
	dd(mn) = wt * d(n);
	myint(mn) = n;
	if (ml != 0)
	{
		for (l = 1; l <= ml; l++)
		{
			j = myint(l);
			/*
			#*****the next two lines of code are modified for cdc (?)
			*/
			dt(l) = xi(j, j);
			if (xi(n, n) != 0.)
			{
				dt(l) = dt(l) - xi(n, j) * xi(n, j) / xi(n, n);
			}
			if (-dd(l) / dt(l) < wt)
			{
				wt = -dd(l) / dt(l);
			}
		} /*for (l = 1; l <= ml; l++)*/
	} /*if (ml != 0)*/
} /*leapstest()*/


#define rs (*RS)
#define cl(i) ( CL[i-1] )

#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment Screen
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#define rm(i) ( RM[i-1] )
#define mb (*MB)
#define df (*DF)
#define sig (*SIG)
#define pen (*PEN)
#define ib (*IB)
#define ns (*NS)
/*
#                   labels and saves best regressions
*/
/*
  990316 Converted declaration to standard C form, using Name [] to
  indicate arrays and * Name to indicate pointers to scalars
*/
void leapsqstore(double * RS, double * CN, double CL [], double RM [],
				 long * MB, double * DF, long * MN, double * SIG, double * PEN,
				 long * IB, long * NS)
{
	double          tr;
	long            l, m, mr;

	leapstrans(&tr, &mr, RS, DF, MN, SIG, PEN, IB, MB);
	if (rm(mr) > tr)
	{
		for (m = 1; m <= mb; m++)
		{
			l = mr - mb + m;
			if (cl(l) == cn)
			{
				return;
			}
		} /*for (m = 1; m <= mb; m++)*/
		if (mb != 1)
		{
			for (m = 2; m <= mb; m++)
			{
				if (tr >= rm(l - 1))
				{
					break;
				} /*if (tr >= rm(l - 1))*/
				rm(l) = rm(l - 1);
				cl(l) = cl(l - 1);
				l--;
			} /*for (m = 2; m <= mb; m++)*/
		} /*if (mb != 1)*/

		rm(l) = tr;
		cl(l) = cn;
	} /*if (rm(mr) > tr)*/
} /*leapsqstore()*/


#define mt (*MT)
#define mp (*MP)
#define ci (*CI)
#define sc(i) ( SC[i-1] )
/*
#                              matrix copy
*/
/*
  990316 Converted declaration to standard C form, using Name [] to
  indicate arrays and * Name to indicate pointers to scalars
*/
void leapscopy(double XN [], double XI [], long INT [], long IND [],
			   long * ML, long * MT, long * MP, long * ND, double SC [],
			   double * CN, double * CI, long * IQ)
{
	long            j, k, l, m;
	for (l = 1; l <= mp; l++)
	{
		if (l > ml)
		{
			m = mt + l - ml - 1;
			myint(l) = ind(m);
		}
		k = myint(l);
		for (m = 1; m <= l; m++)
		{
			j = myint(m);
			if (iq == 0)
			{
				xn(j, k) = xi(j, k);
			}
			if (iq == 1)
			{
				xn(j, k) = xi(j, k) / (sc(j) * sc(k));
			}
			xn(k, j) = xn(j, k);
		} /*for (m = 1; m <= l; m++)*/
	} /*for (l = 1; l <= mp; l++)*/
	cn = ci;
} /*leapscopy()*/


#undef myint

#define tr (*TR)
#define mr (*MR)
/*
#             transform of criterion-smaller must be better
*/
/*
  990316 Converted declaration to standard C form, using Name [] to
  indicate arrays and * Name to indicate pointers to scalars
*/
void leapstrans(double * TR, long * MR, double * RS, double * DF, long * MN,
				double * SIG, double * PEN, long * IB, long * MB)
{
	mr = mb;

	switch ((int) ib)
	{
	  case 1:
		tr = rs;
		mr = mn * mb;
		break;
	  case 2:
		tr = rs / (df - (double) mn);
		break;
	  case 3:
		tr = rs + pen * (mn) * sig;
		break;
	} /*switch ((int) ib)*/
} /*leapstrans()*/
