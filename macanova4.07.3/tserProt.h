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

#ifndef TSERPROTH__
#define TSERPROTH__
/* Prototypes for FFT functions and other time series related functions */
/*
   960716 changed types Long, Double to long and double
   981213 dropped unused 5th argument n1 to qonerg
*/

/* qft.c */
long qft(double */*a*/, double */*b*/, long /*m*/, long /*n*/, long /*divByT*/, long /*iop*/);
/* realft.c */
long realft(long /*pts*/, double /*x*/[]);
/* onecft.c */
long onecft(long /*n*/, double /*x*/[], double /*y*/[]);
/* onehft.c */
long onehft(long /*n*/, double /*x*/[]);
/* siprp.c */
void siprp(long /*pts*/, long /*sym*/[], long /*pSym*/, long /*unSym*/[], double /*x*/[]);
/* srfp.c */
long srfp(long /*pts*/, long */*pmax*/, long /*twoGrp*/, long /*factor*/[], long /*sym*/[], long */*pSym*/, long /*unSym*/[]);
/* ssymrp.c */
void ssymrp(long /*pts*/, long /*sym*/[], double /*x*/[]);
/* rftkd.c */
void rftkd(long /*n*/, long /*factor*/[], long /*pmax*/, double /*x*/[], long * /*error*/);
/* r2rftk.c */
void r2rftk(long /*n*/, long /*m*/, double /*x0*/[], double /*x1*/[]);
/* r3rftk.c */
void r3rftk(long /*n*/, long /*m*/, double /*x0*/[], double /*x1*/[], double /*x2*/[]);
/* r4rftk.c */
void r4rftk(long /*n*/, long /*m*/, double /*x0*/[], double /*x1*/[], double /*x2*/[], double /*x3*/[]);
/* r5rftk.c */
void r5rftk(long /*n*/, long /*m*/, double /*x0*/[], double /*x1*/[], double /*x2*/[], double /*x3*/[], double /*x4*/[]);
/* r8rftk.c */
void r8rftk(long /*n*/, long /*m*/, double /*x0*/[], double /*x1*/[], double /*x2*/[], double /*x3*/[], double /*x4*/[], double /*x5*/[], double /*x6*/[], double /*x7*/[]);
/* rprftk.c */
void rprftk(long /*n*/, long /*m*/, long /*p*/, double */*x*/[], double */*aa*/[], double */*bb*/[], double */*pscr*/);

/* tserops.c */
Symbolhandle doTserOps(Symbolhandle /*list*/);

/* tsersubs.c */
void qonerg(double */*a*/, double */*b*/, long /*m*/, long /*n*/,
			long /*iop*/);
void qtworg(double */*a*/, double */*b*/, double */*c*/, long /*m*/,
			long /*n1*/, long /*n2*/, long /*iop*/);
void qconj(double */*a*/, double */*b*/, long /*m*/, long /*n*/, long /*iop*/);
void qxprd(double */*a*/, double */*b*/, double */*c*/, long /*m*/,
		   long /*n1*/, long /*n2*/, long /*iop*/);
void qconv(double */*a*/, double */*b*/, double */*c*/, long /*m1*/,
		   long /*m2*/, long /*n*/, long /*decimate*/, long /*reverse*/);
void qreal(double */*a*/, double */*b*/, long /*m*/, long /*n*/, long /*iop*/);
void qcmplx(double */*a*/, double */*b*/, double */*c*/, long /*m*/,
			long /*n*/);
void qrect(double */*a*/, double */*b*/, long /*m*/, long /*n*/,
		   long /*unwind*/, double /*crit*/, long /*iop*/);
void qhcopy(double */*a*/, double */*b*/, long /*m*/, long /*n*/,
			long /*iop*/);
void qpadto(double */*in*/, double */*out*/, long /*m1*/, long /*n*/,
			long /*m2*/);
void qrota(double */*a*/, double */*b*/, long /*m*/, long /*n*/,
		   long /*krot*/);
void qreverse(double */*a*/, double */*b*/, long /*m*/, long /*n*/);
void qunwnd(double */*phase*/, long /*m*/, double /*oneCycle*/,
			double /*crit*/, long /*iop*/);

#endif /*TSERPROTH__*/
