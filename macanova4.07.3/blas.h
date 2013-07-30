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

/*
   960716 Changed types Long and Double to long and double
*/
/* 
  define names for blas routines and provide prototypes
*/


#ifndef BLASPROTH__
#define BLASPROTH__

#include <math.h>
#include "typedefs.h"

#undef UNDEFINED__

#ifndef INT
#define INT long
#endif /*INT*/

#ifndef DOUBLE
#define DOUBLE double
#endif /*DOUBLE*/

#ifndef DAXPY
#define DAXPY daxpy_
#endif /*DAXPY*/

#ifndef DDOT
#define DDOT ddot_
#endif /*DDOT*/

#ifndef DASUM
#define DASUM dasum_
#endif /*DASUM*/

#ifndef DCOPY
#define DCOPY dcopy_
#endif /*DCOPY*/

#ifndef DNRM2
#define DNRM2 dnrm2_
#endif /*DNRM2*/

#ifndef DROT
#define DROT drot_
#endif /*DROT*/

#ifndef DROTG
#define DROTG drotg_
#endif /*DROTG*/

#ifndef DROTM
#define DROTM drotm_
#endif /*DROTM*/

#ifndef DROTMG
#define DROTMG drotmg_
#endif /*DROTMG*/

#ifndef DSCAL
#define DSCAL dscal_
#endif /*DSCAL*/

#ifndef DSWAP
#define DSWAP dswap_
#endif /*DSWAP*/

#ifndef IDAMAX
#define IDAMAX idamax_
#endif /*IDAMAX*/

/*
   Note that the calling sequences are Fortran compatible and should be
   interchangeable with the Fortran versions, at least when Fortran is
   callable from C.
*/

#ifndef NOPROTOTYPES

/* blas.c */
void DAXPY(INT */*pn*/, DOUBLE */*pa*/, DOUBLE */*x*/, INT */*pincx*/,
		   DOUBLE */*y*/, INT */*pincy*/);
DOUBLE DDOT(INT */*pn*/, DOUBLE */*x*/, INT */*pincx*/, DOUBLE */*y*/,
			INT */*pincy*/);
DOUBLE DASUM(INT */*pn*/, DOUBLE /*dx*/[], INT */*pincx*/);
void DCOPY(INT */*pn*/, DOUBLE /*dx*/[], INT */*pincx*/, DOUBLE /*dy*/[],
		   INT */*pincy*/);
DOUBLE DNRM2(INT */*pn*/, DOUBLE /*dx*/[], INT */*pincx*/);
void DROT(INT */*pn*/, DOUBLE /*dx*/[], INT */*pincx*/, DOUBLE /*dy*/[],
		  INT */*pincy*/, DOUBLE */*pdc*/, DOUBLE */*pds*/);
void DROTG(DOUBLE */*da*/, DOUBLE */*db*/, DOUBLE */*dc*/, DOUBLE */*ds*/);
void DROTM(INT */*pn*/, DOUBLE /*dx*/[], INT */*pincx*/, DOUBLE /*dy*/[],
		   INT */*pincy*/, DOUBLE /*dparam*/[5]);
void DROTMG(DOUBLE /*dd1*/, DOUBLE /*dd2*/, DOUBLE /*dx1*/, DOUBLE /*dy1*/,
			DOUBLE /*dparam*/[5]);
void DSCAL(INT */*pn*/, DOUBLE */*pda*/, DOUBLE /*dx*/[], INT */*pincx*/);
void DSWAP(INT */*pn*/, DOUBLE /*dx*/[], INT */*pincx*/, DOUBLE /*dy*/[],
		   INT */*pincy*/);
INT IDAMAX(INT */*pn*/, DOUBLE /*dx*/[], INT */*pincx*/);

#else /*NOPROTOTYPES*/

/* blas.c */
void DAXPY(/*INT *pn, DOUBLE *pa, DOUBLE *x, INT *pincx, DOUBLE *y,
			 INT *pincy*/);
DOUBLE DDOT(/*INT *pn, DOUBLE *x, INT *pincx, DOUBLE *y, INT *pincy*/);
DOUBLE DASUM(/*INT *pn, DOUBLE dx[], INT *pincx*/);
void DCOPY(/*INT *pn, DOUBLE dx[], INT *pincx, DOUBLE dy[], INT *pincy*/);
DOUBLE DNRM2(/*INT *pn, DOUBLE dx[], INT *pincx*/);
void DROT(/*INT *pn, DOUBLE dx[], INT *pincx, DOUBLE dy[], INT *pincy,
			DOUBLE *pdc, DOUBLE *pds*/);
void DROTG(/*DOUBLE *da, DOUBLE *db, DOUBLE *dc, DOUBLE *ds*/);
void DROTM(/*INT *pn, DOUBLE dx[], INT *pincx, DOUBLE dy[], INT *pincy,
			 DOUBLE dparam[5]*/);
void DROTMG(/*DOUBLE dd1, DOUBLE dd2, DOUBLE dx1, DOUBLE dy1,
			  DOUBLE dparam[5]*/);
void DSCAL(/*INT *pn, DOUBLE *pda, DOUBLE dx[], INT *pincx*/);
void DSWAP(/*INT *pn, DOUBLE dx[], INT *pincx, DOUBLE dy[], INT *pincy*/);
INT IDAMAX(/*INT *pn, DOUBLE dx[], INT *pincx*/);
#endif /*NOPROTOTYPES*/
#endif /*BLASPROTH__*/
