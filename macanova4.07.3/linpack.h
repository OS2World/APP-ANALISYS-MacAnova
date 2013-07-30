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
   960716 Changed Double to DOUBLE (defined in blas.h)

   990302 Added prototype for dqrsl() and edited all prototypes.
*/
/* 
  define names for Linpack routines and provide prototypes
*/

#ifndef LINPPROTH__
#define LINPPROTH__

#undef UNDEFINED__

#include <math.h>
#include "blas.h"

/*
   961210 added following macros linpack routines can use to 
          check for interrupts on systems where that is not automatic
*/

/* For non-MacAnova use, change 1 to 0 */
#define CHECKNPRODUCTS 1
#if (CHECKNPRODUCTS)

#include "platform.h"
#include "handlers.h"

#ifdef NPRODUCTS /* may be defined in platform.h */
#define incAndTest(INC,LABEL) {incNproducts(INC);testNproducts(LABEL);}
#endif /*NPRODUCTS*/

#endif /*CHECKNPRODUCTS*/

#ifndef incAndTest
#define incAndTest(INC,LABEL)
#undef checkInterrupt
#define checkInterrupt(LABEL)
#endif /*incAndTest*/

/*
  Prototypes and macros for C-versions of Linpack routines
*/

#ifndef FORTRAN
#define FORTRAN /* fortran storage conventions in dqrdc */
#endif /*FORTRAN*/

#ifndef INT
#define INT long
#endif /*INT*/

#ifndef DCHDC
#define DCHDC dchdc_
#endif /*DCHDC*/

#ifndef DQRDC
#define DQRDC dqrdc_
#endif /*DQRDC*/

#ifndef DQRSL
#define DQRSL dqrsl_
#endif /*DQRSL*/

#ifndef DGECO
#define DGECO dgeco_
#endif /*DGECO*/

#ifndef DGEDI
#define DGEDI dgedi_
#endif /*DGEDI*/

#ifndef DGEFA
#define DGEFA dgefa_
#endif /*DGEFA*/

#ifndef DGESL
#define DGESL dgesl_
#endif /*DGESL*/

void DCHDC(DOUBLE * /*a*/, INT * /*plda*/, INT * /*pp*/, DOUBLE * /*work*/,
		   INT /*jpvt*/ [], INT * /*pjob*/, INT * /*info*/);

void DQRDC(DOUBLE /*x*/ [], INT * /*pldx*/, INT * /*pn*/, INT * /*pp*/,
		   DOUBLE /*qraux*/ [], INT /*jpvt*/ [], DOUBLE /*work*/ [],
		   INT * /*job*/);

void DQRSL(DOUBLE /*x*/ [], INT * /*pldx*/, INT * /*pn*/, INT * /*pk*/,
		   DOUBLE /*qraux*/ [], DOUBLE /*y*/ [], DOUBLE /*qy*/ [],
		   DOUBLE /*qty*/ [], DOUBLE /*b*/ [], DOUBLE /*rsd*/ [],
		   DOUBLE /*xb*/ [], INT * /*pjob*/, INT * /*info*/);

void DGECO(DOUBLE /*a*/ [], INT * /*plda*/, INT * /*pn*/, INT /*ipvt*/ [],
		   DOUBLE * /*rcond*/, DOUBLE /*z*/ []);
void DGEDI(DOUBLE * /*a*/, INT * /*plda*/, INT * /*pn*/, INT /*ipvt*/ [],
		   DOUBLE /*det*/ [], DOUBLE /*work*/ [], INT * /*job*/);
void DGEFA(DOUBLE * /*a*/, INT * /*plda*/, INT * /*pn*/, INT /*ipvt*/ [],
		   INT * /*info*/);
void DGESL(DOUBLE * /*a*/, INT * /*plda*/, INT * /*pn*/, INT /*ipvt*/ [],
		   DOUBLE * /*b*/, INT * /*job*/);

#endif /*LINPPROTH__*/
