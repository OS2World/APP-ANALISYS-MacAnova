/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 4.06 or later
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


#ifndef UNIX
#define UNIX
#endif /*UNIX*/

/* Defines for generic Unix platforms. */
#include "genunix.h"


/*
  Defines for nonwindowed version compiled by cc on HP
  980714 Moved define of DISABLECONTROLY from genunix.h
*/

#define HPUX

#define _INCLUDE_POSIX_SOURCE
#define SIGNALARG /*signal handler of form void handler(int) */


#define PLATFORM       PLATFORM_HPCC
#define PLATFORM_ALT1  PLATFORM_WXHPGCC
#define PLATFORM_ALT2  PLATFORM_WXSGI
#define PLATFORM_ALT3  PLATFORM_SGI
#define PLATFORM_ALT4  PLATFORM_HPGCC


#define USEPOW /*use pow() for computing integer powers*/
#define HASDYNLOAD  /* this assumes that the dld library is available */

/*
   The following is for use when READLINE is defined to prevent ^Y from
   causing process suspension instead of a yank.  It may be unnecessary if
   the READLINE library is properly compiled.  See rltty.c
   980714 moved here from genunix.h
*/
#define DISABLECONTROLY  /*keep ^Y from causing suspension instead of yank*/

#define BDOUBLEHILOW
#define HASINFINITY
#define HASNAN


