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


/*
   This file contains only macro defines, no declarations except for
   some enums replacing defines.
   Version of 951201
   Version of 960620  SAVEBINARY7 and SAVEASCII7 defined

   970613 added MAXMDEPTH, removed MAXSTRING
   971016 added error codes for fsolve()
   980521 added codes used by scanPat() and matchName()
   980717 added additional save() and restore() defines
   980718 moved save/restore stuff to new header vmsave.h
   980801 Modified enums used by scanPat() and matchName()
*/
#ifndef GLOBDEFSH__
#define GLOBDEFSH__

/* miscellaneous globally defined macros */
#ifndef NAMEFORWHERE  /* incase dbug.h not included */
#define NAMEFORWHERE where__ 
#endif /*NAMEFORWHERE*/

#ifndef WHERE /* in case dbug.h not included */
/* NB: macro WHERE should be followed by ';' among declarations */
#define WHERE(name) char *NAMEFORWHERE = name
#endif /*WHERE*/

#ifndef HUGEDBL
#define HUGEDBL 1.79769313486e+308
#endif /*HUGEDBL*/

#ifndef HUGEINT
#define HUGEINT     2146727027   /*2^31 - 756621*/
#endif /*HUGEINT*/

 /* These are referenced in glm.c, iterglm.c, ipf.c, funbalan.c*/
enum convergenceErrors
{
	ITERERROR = 0,
	ITERCONVERGED,
	ITERUNCONVERGED
};


/* errorcodes for fsolve() in mathutil.c */
enum fsolveErrors
{
	FSOLVENOTCONVERGED = 300,
	FSOLVEBADARGUMENTS,
	FSOLVEBADGUESSES,
	FSOLVENONMONOTONIC
};

/*
  Ccdes and defines used by scanPat(), matchName() in utils.c
  help(), list(), match(), keyvalue()
*/
#define WILDCARD    '*'
#define ANYCHAR     '?'

enum matchTypeCodes
{
	STARTMATCH      = 0x01,
	ENDMATCH        = 0x02,
	MIDDLEMATCH     = (STARTMATCH | ENDMATCH),
	EXACTMATCH      = 0x04,
	FOUNDANYCHAR    = 0x08,
	ANYMATCH        = 0x10,
	PATTERNTOOLONG  = 0x20,
	MATCHTYPEOFFSET = 2*PATTERNTOOLONG
};

enum matchOpCodes
{
	noNameCheck =  0,
	topicNameCheck,
	variableNameCheck
};

#define NONCONVERGED (-16000)

/* various globally defined limits */
/* NOTE: As of 930518, MAXVARS & MAXERRTRMS are defined in glm.h */
#define MAXIDEPTH       100 /* maximum number of nested if statements*/
#define MAXWDEPTH        10 /* maximum number of nested while or for loops*/
#define MAXBDEPTH        10 /* maximum number of nested batch() commands*/
#define MAXMDEPTH        49 /* maximum number of nested macro calls */
#define MAXLEVEL         99 /* maximum permissible value for '$$' in macros*/
#define MAXLINE         500 /* maximum length of input line */
#define MAXDIMS          31 /* maximum of dimensions in symbol */
#define MAXCOORD 2000000000	/* max size in one coord direction */
#define NAMELENGTH       12 /* maximum length of symbol names */
#define BUFFERLENGTH    500 /* length of certain string buffers */

/* following is largest permissible value to bit operations */
#define MAXBITVALUE    4294967295.0 /*2^32-1 = ffffffff */

#ifndef PATHSIZE       /* length of buffers to hold complete path */
#ifdef MACINTOSH
#define PATHSIZE  255
#else /*MACINTOSH*/
#define PATHSIZE 256
#endif /*MACINTOSH*/
#endif /*PATHSIZE*/

/* Variously globally defined constants */
/* for use in getKeyword() and perhaps elsewhere */
#define OPMASK         0x00000fffUL
#define TYPE1MASK      0x000ff000UL
#define TYPE2MASK      0x0ff00000UL
#define CONTROLMASK    0xf0000000UL

#define REALTYPE1      0x00001000UL
#define LOGICTYPE1     0x00002000UL
#define CHARTYPE1      0x00004000UL
#define STRUCTYPE1     0x00008000UL
#define PLOTINFOTYPE1  0x00010000UL
#define LISTTYPE1      0x00020000UL
#define NULLTYPE1      0x00040000UL
#define UNKNOWNTYPE1   0x00080000UL

#define REALTYPE2      0x00100000UL
#define LOGICTYPE2     0x00200000UL
#define CHARTYPE2      0x00400000UL
#define STRUCTYPE2     0x00800000UL
#define PLOTINFOTYPE2  0x01000000UL
#define LISTTYPE2      0x02000000UL
#define NULLTYPE2      0x04000000UL
#define UNKNOWNTYPE2   0x08000000UL

#define SCALAR1        0x00001000UL /* total length 1 */
#define VECTOR1        0x00002000UL /* dim[1] == length */
#define MATRIX1        0x00004000UL /* no more than 2 dims > 1 */
#define ARRAY1         0x00008000UL /* no restrictions on dims */

#define SCALAR2        0x00100000UL /* total length 1 */
#define VECTOR2        0x00200000UL /* dim[1] == length */
#define MATRIX2        0x00400000UL /* no more than 2 dims > 1 */
#define ARRAY2         0x00800000UL /* no restrictions on dims */

/* miscellaneous control codes for recursion and elsewhere */
#define LEFTRECUR      0x10000000UL /*1st argument or left operand is STRUC*/
#define RIGHTRECUR     0x20000000UL /*2nd argument or right operand is STRUC*/
#define REUSELEFT      0x40000000UL /*reuse 1st argument or left operand */
#define REUSERIGHT     0x80000000UL /*reuse 2nd argument or right operand */

/* miscellaneous status codes */
#define FOUNDMISSING    0x00100000UL
#define FOUNDNONPOS     0x00200000UL
#define FOUNDNEG        0x00400000UL
#define FOUNDDOMERR     0x00800000UL
#define FOUNDOVERFLOW   0x01000000UL
#define FOUNDZERODIV    0x02000000UL

/* Various op codes and return values */
/* op codes for dimcmp() */
#define STRICT          0x00000001UL
#define NEARSTRICT      0x00000002UL
#define HCONCAT         0x00000004UL
#define VCONCAT         0x00000008UL

/* opcodes for isInteger */
#define ANYVALUE          0
#define NEGATIVEVALUE     1
#define NONNEGATIVEVALUE  2
#define POSITIVEVALUE     3

/*return values from dimcmp() and treecmp()*/
#define HASMISSING      0x00000001UL
#define BADDIMS         0x00000002UL
#define BADSTRUC        0x00000004UL

/*return values from checkLabelKey*/
#define LABELSOK        0x00000000UL
#define TOOMANYLABELS   0x00000001UL
#define TOOFEWLABELS    0x00000002UL
#define WRONGSIZELABELS 0x00000004UL
#define LABELSERROR     0x00000008UL
#define WRONGTYPEVECTOR 0x00000018UL
#define WRONGTYPECOMP   0x00000028UL

/*
   control codes for fixupMatlabels()
*/
#define USEROWLABELS    0x00000001UL
#define USECOLLABELS    0x00000002UL
#define USEBOTHLABELS   0x00000003UL /*USEROWLABELS | USECOLLABELS*/

/*
  980718 moved stuff related to save and restore to new header mvsave.h
*/
/* Type defines from mainpars.h as of 930621 */
/* 
  They are here in case different version of yacc generates different codes
  They are needed when restoring save files from version 3.1x
*/
# define CHAR_V31     257
# define REAL_V31     258
# define BLTIN_V31    259
# define LIST_V31     260
# define STRUC_V31    261
# define MACRO_V31    262
# define UNDEF_V31    263
# define LOGIC_V31    266
# define MACRO1_V31   267
# define PLOTINFO_V31 278
# define GARB_V31     280
# define LONG_V31     281

/*
  Type defines moved out of mainpars.h (generated by yacc run on mainpars.c)
  970714.  Putting them here removes one inconvenience to be overcome
  if the version of yacc is changed since these should remain fixed.
*/
#ifdef wx_msw
/* 980729 change because CHAR and LONG are typedefs in winnt.h */
#define CHAR    MVCHAR
#define LONG    MVLONG
#endif /*wx_msw*/

enum symbolTypes
{
	CHAR     = 257,
	REAL     = 258,
	BLTIN    = 259,
	LIST     = 260,
	STRUC    = 261,
	MACRO    = 262,
	UNDEF    = 263,
	LOGIC    = 264,
	PLOTINFO = 267,
	GARB     = 268,
	LONG     = 269,
	NULLSYM  = 270,
	ASSIGNED = 283
};
#define SHORTCHAR   (CHAR | SHORTSYMBOL)
#define SHORTREAL   (REAL | SHORTSYMBOL)
#define SHORTLOGIC  (LOGIC | SHORTSYMBOL)
#define SHORTLONG   (LONG | SHORTSYMBOL)

#endif /*GLOBDEFSH__*/
