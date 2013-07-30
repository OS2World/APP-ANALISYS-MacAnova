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

#ifndef TYPEDEFSH__
#define TYPEDEFSH__

#if defined(BCPP) || defined(WIN31)
#define HuGe huge
#else
#define HuGe
#endif /* BCPP || WIN31 */

typedef void                Void;
typedef Void HuGe          *VoidP;
typedef VoidP              *VoidH;
typedef double              Double;
typedef Double HuGe        *DoubleP;
typedef DoubleP            *DoubleH;
typedef long                Long;
typedef Long HuGe          *LongP;
typedef LongP              *LongH;
typedef unsigned long       uLong;
typedef uLong HuGe         *uLongP;
typedef uLongP             *uLongH;
typedef char                Char;
typedef Char HuGe          *CharP;
typedef CharP              *CharH;
typedef unsigned char       uChar;
typedef uChar HuGe         *uCharP;
typedef uCharP             *uCharH;
typedef int                 Int;
typedef Int HuGe           *IntP;
typedef IntP               *IntH;
typedef short               Short;
typedef Short HuGe         *ShortP;
typedef ShortP             *ShortH;

#undef UNDEFINED__
#ifdef UNDEFINED__
#ifndef DEFINETYPES
...
#else /*DEFINETYPES*/     /* use #define instead of typedef */
#if defined(BCPP) || defined(WIN31)
#define Void                void huge
#define Double              double huge
#define Long                long huge
#define uLong               unsigned long huge
#define Char                char huge
#define uChar               unsigned char huge
#define Int                 int        
#define Short               short
#else /*BCPP || WIN31*/
#define Void                void
#define Double              double     
#define Long                long       
#define uLong               unsigned long
#define Char                char       
#define uChar               unsigned char
#define Int                 int        
#define Short               short      
#endif /*BCPP || WIN31*/
#endif /*DEFINETYPES*/
#endif /*UNDEFINED__*/

#endif /*TYPEDEFSH__*/
