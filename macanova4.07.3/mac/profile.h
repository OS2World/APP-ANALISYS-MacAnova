/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 3.36 or later
*(C)*
*(C)* Copyright (c) 1994 by Gary Oehlert and Christopher Bingham
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

#ifndef PROFILEH__
#define PROFILEH__
#define COMMA ,
#ifdef EXTERN
#undef EXTERN
#endif /*EXTERN*/

#ifdef _MAIN_
#define EXTERN
#define INIT(VAL) = VAL
#define INITDIM(N) N
#define INITARRAY(VALS) = { VALS }
#else
#define EXTERN extern
#define INIT(VAL)
#define INITDIM(N)
#define INITARRAY(VALS)
#endif /*_MAIN_*/

#include <perf.h>

#define PROFILEOFF       1
#define PROFILEON        2

#define TIMERCOUNT       10
#define CODEBUCKETSIZE   8
#define DOROM            true
#define DONTDOROM        false
#define DOAPPCODE        true
#define ROMID            0
#define ROMNAME          ""
#define DORAM            true
#define DONTDORAM        false

#define PERFORMOUT       "\pPerform.out"
#define DOHISTOGRAM      true
#define RPTFILECOLUMNS   80

EXTERN TP2PerfGlobals    ThePGlobals INIT(nil);
EXTERN short             ProfileStatus INIT(0);

#endif /*PROFILEH__*/
