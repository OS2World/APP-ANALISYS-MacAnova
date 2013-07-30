/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 4.00 or later
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

#ifndef TSERH__
#define TSERH__

/* defines various constants for time series related functions */

#include "tserProt.h"

#define IRFT       0x00000001L
#define IHFT       0x00000002L
#define ICFT       0x00000004L

#define IHCONJ     0x00000008L
#define ICCONJ     0x00000010L

#define IHREAL     0x00000020L
#define ICREAL     0x00000040L

#define IHIMAG     0x00000080L
#define ICIMAG     0x00000100L

#define IHTOC      0x00000200L
#define ICTOH      0x00000400L

#define IHPRDH    0x00000800L
#define IHPRDHJ   0x00001000L
#define IHPRD     (IHPRDH|IHPRDHJ)

#define ICPRDC    0x00002000L
#define ICPRDCJ   0x00004000L
#define ICPRD     (ICPRDC|ICPRDCJ)
#define IPRD      (IHPRD|ICPRD)

#define ICMPLX    0x00008000L

#define IHPOLAR   0x00010000L
#define ICPOLAR   0x00020000L
#define IPOLAR    (IHPOLAR|ICPOLAR)

#define IHRECT    0x00040000L
#define ICRECT    0x00080000L
#define IRECT     (IHRECT|ICRECT)

#define IUNWIND   0x00100000L

#define ICONVOLVE 0x00200000L
#define IROTATE   0x00400000L
#define IPADTO    0x00800000L
#define IREVERSE  0x01000000L
#define NCMDS     25

#define IHERMITIAN     0
#define IREAL          1

#define IRADIANS       1
#define IDEGREES       2
#define ICYCLES        3

#undef NOERROR
#define NOERROR        0
#define TOOBIGPRIME    1
#define TOOMANYFACTORS 2
#define OUTOFMEMORY    3

#define PMAX 29  /* largest permissible prime factor */
#define NEST 20   /* maximum number of factors + 1 */

#endif /*TSERH__*/
