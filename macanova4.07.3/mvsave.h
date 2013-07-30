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
  Defines and declarations having to do with saving and restoring

  980718 Created by combining stuff from globdefs.h and globkb.h
         Also moved prototypes of functions in restore.c and save.c
         (except save() and restore()) from ioProto.h
         Left in globkb.h are declarations of RESTOREFILE and KEEPFILE
         and values of options restoredel (restore()) and history
         (save()).
*/

#ifndef MVSAVEH__
#define MVSAVEH__

#undef EXTERN
#undef INIT
#undef INITDIM
#undef INITARRAY
#undef COMMA

/* Globals will be defined in save.c, only declared elsewhere*/

#ifdef SAVE__
#define EXTERN
#define INIT(VAL) = VAL
#define INITDIM(N) N
#define INITARRAY(VALS) = { VALS }
#else
#define EXTERN extern
#define INIT(VAL)
#define INITDIM(N)
#define INITARRAY(VALS)
#endif /*SAVE__*/

#define COMMA ,

/*
  Values for ASCII, defining save operations; <=0 are binary, >0 are ascii
  SAVEBINARY1 & SAVEASCII1 refer to original binary format, with MAXVAR==31
  SAVEBINARY2 & SAVEASCII2 refer to first modified format, with MAXVAR == 31
  SAVEBINARY3 & SAVEASCII3 refer to modified format allowing MAXVAR == 96
  SAVEBINARY4 & SAVEASCII4 non-printable characters escaped on asciisave
  SAVEBINARY5 & SAVEASCII5 allow saving and restoring NULL symbols
  SAVEBINARY6 & SAVEASCII6 allow saving and restoring symbol labels
  SAVEBINARY7 & SAVEASCII7 new format identifying saved pieces with
                           keywords; PLOTINFO tick information is saved

  Types 2 and 3 differ only in the structure of linear model related globals
  MacAnova originally allowed only 31 variables in models, thus allowing each
  term to be described in a single 32 bit long, although in fact the
  information was kept in doubles.  When the number of variables allowed was
  increased, a term became expressible by an array of unsigned longs.

  The original ascii save format had long lines and imbedded '\0's terminating
  strings and macros.  Later formats have short lines and have character counts
  for each string and macro.  The only non-standard embedded characters would
  be in strings defined by escape sequences such as "\1".  Also, only
  NDIMS(symh) dimensions are put in file.

  Type 4 is the same as type 3 for save(), byt Type 4 escapes '\', '\t',
  and all non-printable characters (except '\n').

  Type 5 introduced when saving and restoring NULL symbols was implemented

  Type 6 introduced when saving and restoring symbol labels was implemented

  Type 7 was a major change; Symbols saved in a sort of keyword form

*/

EXTERN long            ASCII INIT(0); /* flag to indicate save or asciisave */

#define SPECIALPREFIX1 SCRATCHPREFIX1 /* 1st character of special save names */
#define SPECIALPREFIX2 SCRATCHPREFIX2 /* 2nd character of special save names */
#define SPECIALPREFIX3 '$'            /* 3rd character of special save names */

/* names for special objects saved by save(). Must start with SPECIALPREFIXs*/
#define LENGTHOPTIONS      12
EXTERN char             OPTIONS[LENGTHOPTIONS+1] INIT("@$$$$OPTIONS");

#define LENGTHHISTRY       12
EXTERN char             HISTRY[LENGTHHISTRY+1] INIT("@$$$$HISTORY");

#define LENGTHGLOBALS      12
EXTERN char             GLOBALS[LENGTHGLOBALS+1] INIT("@$$$$GLOBALS");

#define LENGTHSAVETIME     12
EXTERN char             SAVETIME[LENGTHSAVETIME+1] INIT("@$$$$SAVETIM");

#define LENGTHMACHINE      12
EXTERN char             MACHINE[LENGTHMACHINE+1] INIT("@$$$$MACHINE");

/*
  Test values for double precision and long byte order
  Every byte is unique and can easily be identified so the
  original order can be recovered.

  The hex digits are the bytes in memory order, retrieved by casting
  a double * or a long * to an unsigned char *.

  They include positive an negative numbers and positive and
  negative exponents.

  By having the hex representation on the saving machine and computing
  the hex represenation on the restoring machine, it should be able
  to figure out how to translate it the formats are not the same
*/

enum
{
	nSampleDoubles = 4,
	nSampleLongs = 4
};

EXTERN unsigned char     *SAMPLEDOUBLES[] INITARRAY(\
	(unsigned char *) "4.686506675954929163e+268" COMMA \
	(unsigned char *) "-4.686506675954929163e+268" COMMA \
	(unsigned char *) "2.25141103357564883e-117" COMMA \
	(unsigned char *) "-2.25141103357564883e-117");
/*
  On HP and Macinntosh these have representations (in machine byte order)
  77B6B5B4B3B2B1B0, F7B6B5B4B3B2B1B0,
  27B6B5B4B3B2B1B0, A7B6B5B4B3B2B1B0
*/

EXTERN unsigned char     *SAMPLELONGS[] INITARRAY(\
	(unsigned char *) "1936879984" COMMA (unsigned char *) "-1936879984" COMMA \
	(unsigned char *) "747474576" COMMA (unsigned char *) "-747474576");

/*
  On HP and Macintosh these have representations
  73727170, 8c8d8e90, 2c8d8e90, d3727170
*/

#define ByteSize       256
#define Byte2          ByteSize
#define Byte3          (Byte2*Byte2)

#define ItemNameLength 11

typedef long SaveItemTypes;

/*
  Values for global ASCII defining different versions of save files

  ASCII == -3 or -4 means modified binary save format with 96 variables
  ASCII == -1 or -2 means original binary save format with 31 variables
  ASCII == 1 means old asciisave format, with long lines and imbedded '\0's
  ASCII == 2 means new asciisave format, with short lines, no imbedded '\0's
  ASCII == 3 same as 2 except no superfluous dimension information, and
			 allowing 96 variables in models
  ASCII == 4 same as 3 except non-printable characters in strings escaped w '\'
  ASCII == 5 & ASCII == -5 same, except NULLs save, new MISSING value
  ASCII == 6 & ASCII == -6 same, except symbol labels are saved
  ASCII == 7 & ASCII == -7 Format for symbol header modified and completely
                           new format for PLOTINFO symbols

  It is important that SAVEBINARYi == -SAVEASCIIi
  980717 added extra codes and made them enums
  980724 added globals FULLSAVE and PARTIALSAVE
*/
enum
{
	SAVEBINARY1 =     -1,   /* MAXVAR == 31 binary save */
	SAVEBINARY2 =     -2,   /* same */
	SAVEBINARY3 =     -3,   /* MAXVAR == 96 binary save */
	SAVEBINARY4 =     -4,   /* same */
	SAVEBINARY5 =     -5,   /* same, NULLSYM allowed */
	SAVEBINARY6 =     -6,   /* same, labels saved */
	SAVEBINARY7 =     -7,   /* new plotting info saved in new format */
	SAVEBINARY8 =     -8,   /* for future changes */
	SAVEBINARY9 =     -9,   /* for future changes */
	SAVEASCII1  =      1,   /* original ascii save, MAXVAR  =  =  31 */
	SAVEASCII2  =      2,   /* revised ascii save, MAXVAR  =  =  31 */
	SAVEASCII3  =      3,   /* revised ascii save, MAXVAR  =  =  96 */
	SAVEASCII4  =      4,   /* same, except escaped nonprintables */
	SAVEASCII5  =      5,   /* same, NULLSYM allowed */
	SAVEASCII6  =      6,   /* same, labels saved */
	SAVEASCII7  =      7,   /* same, new plotting info saved */
	SAVEASCII8  =      8,   /* for future changes */
	SAVEASCII9  =      9   /* for future changes */
};

#define Binary  (ASCII < 0)

#define Pre336Save (ASCII > SAVEBINARY5 && ASCII < SAVEASCII5)
#define Pre400Save (ASCII > SAVEBINARY6 && ASCII < SAVEASCII6)
#define Pre407Save (ASCII > SAVEBINARY7 && ASCII < SAVEASCII7)
#define OldStyleGraphSave Pre407Save

EXTERN char        PadChar;  /* either ' ' (binary save) or '\n' (ascii save) */


/* starting characters on save and asciisave files */
#define CURRENTSAVEFORMATLEVEL  7

#ifdef SAVE__
/*
  Because the MPW 3.2 pre-processor chokes on using INIT() with "//"
  in the string, we don't use INIT to initialize these variables
*/
char        BINARYHEADER1[] = "////MacAnovaSaveFile////";
char       *BINARYHEADER2 = BINARYHEADER1;
char        BINARYHEADER3[] = "////MacAnovaSaveFile3///";
char       *BINARYHEADER4 = BINARYHEADER3;
char        BINARYHEADER5[] = "////MacAnovaSaveFile5///";
char        BINARYHEADER6[] = "////MacAnovaSaveFile6///";
char        BINARYHEADER7[] = "////MacAnovaSaveFile7///";

#if (CURRENTSAVEFORMATLEVEL >= 8)
char        BINARYHEADER8[] = "////MacAnovaSaveFile8///";
#if (CURRENTSAVEFORMATLEVEL >= 9)
char        BINARYHEADER9[] = "////MacAnovaSaveFile9///";
#endif /*CURRENTSAVEFORMATLEVEL>=9*/
#endif /*CURRENTSAVEFORMATLEVEL>=8*/

char        ASCIIHEADER1[] = "////MacAnovaASCIISave///";
char        ASCIIHEADER2[] = "////MacAnovaASCIISave2//";
char        ASCIIHEADER3[] = "////MacAnovaASCIISave3//";
char        ASCIIHEADER4[] = "////MacAnovaASCIISave4//";
char        ASCIIHEADER5[] = "////MacAnovaASCIISave5//";
char        ASCIIHEADER6[] = "////MacAnovaASCIISave6//";
char        ASCIIHEADER7[] = "////MacAnovaASCIISave7//";
#if (CURRENTSAVEFORMATLEVEL >= 8)
char        ASCIIHEADER8[] = "////MacAnovaASCIISave8//";
#if (CURRENTSAVEFORMATLEVEL >= 9)
char        ASCIIHEADER9[] = "////MacAnovaASCIISave9//";
#endif /*CURRENTSAVEFORMATLEVEL>=9*/
#endif /*CURRENTSAVEFORMATLEVEL>=8*/
#else /*SAVE__*/
extern char        BINARYHEADER1[];
extern char       *BINARYHEADER2;
extern char        BINARYHEADER3[];
extern char       *BINARYHEADER4;
extern char        BINARYHEADER5[];
extern char        BINARYHEADER6[];
extern char        BINARYHEADER7[];

#if (CURRENTSAVEFORMATLEVEL >= 8)
extern char        BINARYHEADER8[];
#if (CURRENTSAVEFORMATLEVEL >= 9)
extern char        BINARYHEADER9[];
#endif /*CURRENTSAVEFORMATLEVEL>=9*/
#endif /*CURRENTSAVEFORMATLEVEL>=8*/

extern char        ASCIIHEADER1[];
extern char        ASCIIHEADER2[];
extern char        ASCIIHEADER3[];
extern char        ASCIIHEADER4[];
extern char        ASCIIHEADER5[];
extern char        ASCIIHEADER6[];
extern char        ASCIIHEADER7[];
#if (CURRENTSAVEFORMATLEVEL >= 8)
extern char        ASCIIHEADER8[];
#if (CURRENTSAVEFORMATLEVEL >= 9)
extern char        ASCIIHEADER9[];
#endif /*CURRENTSAVEFORMATLEVEL>=9*/
#endif /*CURRENTSAVEFORMATLEVEL>=8*/

#endif /*SAVE__*/

EXTERN char        FULLSAVE[]     INIT("FULLSAVE");
EXTERN char        PARTIALSAVE[]  INIT("PARTIALSAVE");

/* Codes and names for information about saved symbols */
enum saveTypeShapeCodes
{
	SAVEEND        = 0x0000,
	SAVEREAL       = 0x0001,
	SAVELONG       = 0x0002,
	SAVECHAR       = 0x0004,
	SAVESYMBOL     = 0x0008,
	SAVESCALAR     = 0x0010,
	SAVEVECTOR     = 0x0020,
	SAVEREALSCALAR = SAVESCALAR | SAVEREAL,
	SAVELONGSCALAR = SAVESCALAR | SAVELONG,
	SAVECHARSCALAR = SAVESCALAR | SAVECHAR,
	SAVEREALVECTOR = SAVEVECTOR | SAVEREAL,
	SAVELONGVECTOR = SAVEVECTOR | SAVELONG,
	SAVECHARVECTOR = SAVEVECTOR | SAVECHAR,
	SAVEREALSYMBOL = SAVESYMBOL | SAVEREAL,
	SAVELONGSYMBOL = SAVESYMBOL | SAVELONG,
	SAVECHARSYMBOL = SAVESYMBOL | SAVECHAR
};

enum handletypes
{
	LONGHANDLE = SAVELONGVECTOR,
	DOUBLEHANDLE = SAVEREALVECTOR,
	CHARHANDLE = SAVECHARVECTOR
};


/* Note: names of items must not exceed ItemNameLength */

enum itemConstants
{
	ItemCodeLength =  7,
	EndItems       = 99
};


EXTERN char SymbolPrefix [] INIT("SY");

EXTERN SaveItemTypes SymbolItemTypes[] INITARRAY(\
    0x0000         /*start*/  COMMA \
	SAVELONGSCALAR /*nclass*/ COMMA \
	SAVELONGVECTOR /*dims*/   COMMA \
	SAVECHARSYMBOL /*labels*/ COMMA \
	SAVECHARSYMBOL /*notes*/);

enum symbolItemIndices
{
	SYMSTART,
	SYMNCLASS,
	SYMDIMS,
	SYMLABELS,
	SYMNOTES,
	SYMENDSYM
};

#define SYMITEMTYPE(I) SymbolItemTypes[I]
#define SYMITEMNAME(I) (encodeItem(SymbolPrefix, I,\
	((I) != EndItems) ? SYMITEMTYPE(I) : 0))
#define SYMBOLMARKER SYMITEMNAME(SYMSTART)

#undef	EXTERN
#undef	INIT
#undef	INITDIM
#undef	INITARRAY
#undef  COMMA

/*
  Prototypes of save and restore utilities, moved here from ioProto.h
*/

#ifndef NOPROTOTYPES

/*restore.c*/
long restoreLong(long */*longValue*/);
long restoreName(char */*name*/);
long restoreString(char */*string*/);
long restoreNString(long /*length*/, char */*string*/);
long restoreDouble(double */*value*/);
long skipRestoreVector(long /*itemType*/, long /*itemLength*/);
int getRestoreItem(long /*itemType*/, long * /*itemLength*/, 
				   double * /*itemDvalue*/, long * /*itemLvalue*/,
				   char * /*itemCvalue*/);
int getNextRestoreItem(char /*prefix*/ [], int * /*itemNumber*/,
					   int * /*itemType*/, long * /*itemLength*/,
					   double * /*itemDvalue*/, long * /*itemLvalue*/,
					   char * /*itemCvalue*/);

/* save.c */
int saveName(char * /*nameItem*/);
int saveRawHandle(char ** /*item*/, long /*n*/);
int saveHandle(char ** /*item*/, long /*type*/, long /*nvals*/,
			   char * /*itemName*/);
int saveString(char */*stringItem*/, char * /*itemName*/);
int saveLong(long /*longItem*/, char * /*itemName*/);
int saveDouble(double /*DoubleItem*/, char * /*itemName*/);
int saveSymbolInfo(Symbolhandle /*symh*/, char * /*name*/, long /*type*/);
int saveNamedVector(char * /*itemName*/, long /*itemType*/,
					long /*itemLength*/, long * /*itemLvector*/,
					double * /*itemDvector*/, char * /*itemCvector*/);

/* utils.c */
char * encodeItem(char * /*root*/, int /*itemNumber*/, int /*itemType*/);
int decodeItem(char * /*itemCode*/, char /*root*/ [], int * /*itemNumber*/, 
			   int * /*itemType*/);

#else /*NOPROTYPES*/

/*restore.c*/
long restoreLong(/*long *longValue*/);
long restoreName(/*char *name*/);
long restoreString(/*char *string*/);
long restoreNString(/*long length, char *string*/);
long restoreDouble(/*double *value*/);
long skipRestoreVector(/*long itemType, long itemLength*/);
int getRestoreItem(/*long * itemType, long * itemLength, 
					 double * itemDvalue, long * itemLvalue,
					 char * itemCvalue*/);
int getNextRestoreItem(/*char prefix [], int * itemNumber,
					   int * itemType, long * itemLength,
					   double * itemDvalue, long * itemLvalue,
					   char * itemCvalue*/);

/*save.c*/
int saveName(/*char * nameItem*/);
int saveRawHandle(/*char **item, long n*/);
int saveHandle(/*char **h, long type, long nvals, char * itemName*/);
int saveString(/*char *stringItem, char * itemName*/);
int saveLong(/*long longItem, char * itemName*/);
int saveDouble(/*double doubleItem, char * itemName*/);
int saveSymbolInfo(/*Symbolhandle symh, char * name, long type*/);
int saveNamedVector(/*char * itemName, long itemType, long itemLength,
					 long * itemLvector, double itemDvector,
					 char * itemCvector*/);
/* utils.c */
char * encodeItem(char * root, int itemNumber, int itemType);
int decodeItem(/*char * itemCode, char root [], int * itemNumber, 
			   int * itemType*/);
#endif /*NOPROTOTYPES*/

#endif /*MVSAVEH__*/
