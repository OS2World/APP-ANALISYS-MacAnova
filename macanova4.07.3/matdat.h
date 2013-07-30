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

#ifndef MATDATH__
#define MATDATH__
#ifdef MPW
#ifdef MPW3
#pragma segment Readdata
#else
#define __SEG__ Readdata
#endif /*MPW3*/
#endif /*MPW*/

#ifdef ERRORMSGS
#define FPRINTF(A) fprintf A
#else
#define FPRINTF(A)
#endif

#define MAXFMT          MAXLINE       
#define MAXN            50            
#define MAXNAME         200
#define ENDMACROLENGTH  MAXNAME       
#define ENDMACROSTART   '%'           
#define FORMATITEMSTART '%'           
#define COMPNAMESTART   '$'

#undef SP      
#undef TAB     
#undef NL      
#undef CR      
#define SP      ' '
#define TAB     '\t'
#define NL      0x0a
#define CR      0x0d

#define COMMENTSTART ')' /* initial character on comment line */
#define FORMATSTART '(' /* initial character on Fortran format line */

#ifdef READDATA__
/* following should have exactly MAXN "%lf"'s */

static char DefaultFormat[] = "%lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf" ;
#endif /*READDATA__*/

#undef NOERROR
enum readErrorCodes
{
	NOERROR = 0,
	OPENERROR,
	NOTFOUND,
	COMPONENTNOTFOUND,
	BADHEADER,
	NODATA,
	TWOFORMATS,
	SHORTDATA,
	BADFORMAT,
	BADCFORMAT,
	NOFIELDS,
	NTOOBIG,
	CANTALLOC,
	NOLINES,
	NOMACRO
};


enum charFmtCodes
{
	BYLINES = 1,
	BYFIELDS,
	BYQUOTEDFIELDS
};

#endif /*MATDATH__*/
