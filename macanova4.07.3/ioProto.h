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

#ifndef IOPROTOH__
#define IOPROTOH__
/* prototypes for i/o related functions, excluding those with machine
   specific version (see unxProto.h, macProto.h)

   950927 Deleted prototypes for isAllWhite and readLine (now static functions
          in readFile.c)
   970227 Modified prototypes for readHeader() and readMacro() to include
          new argument macroEnd
   970618 Modified prototype for readHeader() to include argument inLine
   980715 Changed calling sequence of saveHandle(), saveString(), saveLong(),
          and saveDouble() and added saveSymbolInfo() and saveName()
   980716 Changed calling sequence of saveHandle() again and added
          saveRawHandle()
   980718 Moved prototypes of functions (except save() and restore() in
          save.c and restore.c to new header file mvsave.h
   980723 changed calling squences for readFile(), readMacro(), and
          readHeader()
   990220 added int * dollars to prototype for readHeader()
*/


#include "typedefs.h"

#ifdef MW_CW
#define startupMsg startupmsg  /* GWO to avoid codewarrior enum problem in dialogs.h */
#endif

#ifndef NOPROTOTYPES
/*
  980718 Prototypes of saveName(), saveRawHandle(), saveHandle(),
         saveString(), saveLong(), saveDouble() and saveSymbolInfo()
         moved to mvsave.h
         Also prototypes of restoreLong(), restoreName(), restoreString()
         restoreNString(), restoreDouble(), skipRestoreVectore() and
         getRestoreItem() moved to mvsave.h
*/

/* batch.c */
Symbolhandle batch(Symbolhandle /*list*/);
/* help.c */
Symbolhandle help(Symbolhandle /*list*/);
Symbolhandle macrousage(Symbolhandle /*list*/);
/* print.c */
Symbolhandle print(Symbolhandle /*list*/);
Symbolhandle putascii(Symbolhandle /*list*/);
/* readdata.c */
Symbolhandle readdata(Symbolhandle /*list*/);
#ifdef UNDEFINED__
Symbolhandle doReadVec(FILE */*fn*/, char */*fname*/, long /*echo*/);
Symbolhandle doReadMat(FILE */*fp*/, char */*matName*/, char */*fileName*/, long /*verbose*/, long /*echo*/);
Symbolhandle doReadMacro(FILE */*fp*/, char */*matName*/, char */*fileName*/, long /*verbose*/, long /*echo*/);
Symbolhandle doReadInfo(FILE */*fp*/, char */*matName*/, char */*fileName*/, long /*verbose*/, long /*echo*/);
Symbolhandle doReads(FILE */*fp*/, char */*matName*/, char */*fileName*/, long /*op*/, long /*verbose*/, long /*echo*/);
double nextItem(long /*place*/, long */*next*/);
void headerError(long /*errorNumber*/, char */*matName*/, char */*fileName*/);
void readError(int /*errorNumber*/, char */*matName*/, char */*fileName*/);
#endif /*UNDEFINED__*/
/* readFile.c */
long readFile(FILE */*file*/, int /*nFields*/, char */*fmt*/,
			 int /*nSkip*/, int /*byRows*/, int /*quoted*/,
			 Symbolhandle /*symhy*/, int/*echo*/, int /*warning*/);
long readMacro(FILE */*file*/, long /*nlines*/, char * /*macroEnd*/,
			  Symbolhandle /*symhtext*/, int/*echo*/, int /*warning*/);
/* readHead.c */
long readHeader(FILE * /*file*/, long /*dims*/[], long * /*nFields*/,
				char ** /*fmt*/, long * /*nSkip*/, int * /*byRows*/,
				long * /*type*/, int * /*quoted*/, int * /*inLine*/,
				int * /*labels*/, int * /*notes*/, int * /*dollars*/,
				double * /*missValue*/, char * /*matName*/,
				char * /*macroEnd*/, int /*verbose*/);
/* restore.c */
Symbolhandle restore(Symbolhandle /*list*/);
/* save.c */
Symbolhandle save(Symbolhandle /*list*/);
/* spool.c */
Symbolhandle spool(Symbolhandle /*list*/);
/* startMsg.c */
void startupMsg(long);
#else /*NOPROTOTYPES*/
/* batch.c */
Symbolhandle batch(/*Symbolhandle list*/);
/* help.c */
Symbolhandle help(/*Symbolhandle list*/);
/* print.c */
Symbolhandle print(/*Symbolhandle list*/);
Symbolhandle putascii(/*Symbolhandle list*/);
/* readdata.c */
Symbolhandle readdata(/*Symbolhandle list*/);
Symbolhandle doReadVec(/*FILE *fn, char *fname, long echo*/);
Symbolhandle doReadMat(/*FILE *fp, char *matName, char *fileName, long verbose, long echo*/);
Symbolhandle doReadMacro(/*FILE *fp, char *matName, char *fileName, long verbose, long echo*/);
Symbolhandle doReads(/*FILE *fp, char *matName, char *fileName, long objectType, long verbose, long echo*/);
double nextItem(/*long place, long *next*/);
void headerError(/*long errorNumber, char *matName, char *fileName*/);
void readError(/*int errorNumber, char *matName, char *fileName*/);
/* readFile.c*/
long readFile(/*FILE *file, int nFields, char *fmt,
				int nSkip, int byRows, int quoted,
				Symbolhandle symhy, intecho, int warning*/);
long readMacro(/*FILE *file, long nlines, char * macroEnd,
				 Symbolhandle symhtext, int echo, int warning*/);
/* readHead.c */
long readHeader(/*FILE * file, long dims[], int * nFields,
				char ** fmt, long * nSkip, int * byRows,
				long * type, int * quoted, int * inLine,
				int * labels, int * notes, int * dollars,
				double * missValue, char * matName,
				char * macroEnd, int verbose*/);
/* restore.c */
Symbolhandle restore(/*Symbolhandle list*/);
/* save.c */
Symbolhandle save(/*Symbolhandle list*/);
/* spool.c */
Symbolhandle spool(/*Symbolhandle list*/);
/* startMsg.c */
void startupMsg(/*void*/);
#endif /*NOPROTOTYPES*/

#endif /*IOPROTOH__*/
