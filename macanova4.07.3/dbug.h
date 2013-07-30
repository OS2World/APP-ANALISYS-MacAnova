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

#ifndef DBUGH__
#define DBUGH__

#define DBOUTLENGTH 512

#include <stdio.h>

#ifdef MPW
#include <strings.h>
#include <Types.h>
#endif /*MPW*/

/*
  980803 added declaration of myAlert() and made ALERT and ALERT1
  equivalent to PRINT and PRINT1 on non-windowed versions
*/

#ifndef NAMEFORWHERE
#define NAMEFORWHERE where__
#endif /*NAMEFORWHERE*/

#ifndef WHERE
/* NB: macro WHERE should be followed by ';' as last element in declarations */
#define WHERE(name) char *NAMEFORWHERE = name
#endif /*WHERE*/

#ifdef NDEBUG
#  define assert(ignore) ((void) 0)
#  define PRINT(FMT,A,B,C,D) 
#  define PRINTMORE(FMT,A,B,C,D)
#  define PRINTVEC(FMT,X,N)
#  define PLACE(LOC)
#  define BEFORE(LOC)
#  define AFTER(LOC)
#  define PRINT1(I,FMT,A,B,C,D) 
#  define PRINTMORE1(I,FMT,A,B,C,D) 
#  define FPRINT(FMT,A,B,C,D)
#  define FPRINT1(I,FMT,A,B,C,D)
#  define ALERT(FMT,A,B,C,D)
#  define ALERT1(I,FMT,A,B,C,D) 
#  define DEBUGGER(FMT,A,B,C,D)
#  define DEBUGGER1(I,FMT,A,B,C,D)

#else /*NDEBUG*/
/* debugging output */
#ifndef PRINTPAUSE
#  define PRINT(FMT,A,B,C,D) ((void) sprintf(DBOUT,"%s: ",NAMEFORWHERE),\
							myprint(DBOUT), \
							(void) sprintf(DBOUT,(FMT),(A),(B),(C),(D)),\
							myprint(DBOUT), 0)
#else /*PRINTPAUSE*/
#  define PRINT(FMT,A,B,C,D) ((void) sprintf(DBOUT,"%s: ",NAMEFORWHERE),\
							myprint(DBOUT), \
							(void) sprintf(DBOUT,(FMT),(A),(B),(C),(D)),\
							myprint(DBOUT), mypause(), 0)
#endif /*PRINTPAUSE*/
#  define PRINTMORE(FMT,A,B,C,D) ((void) sprintf(DBOUT,(FMT),(A),(B),(C),(D)),\
							myprint(DBOUT), 0)

#  define PRINTVEC(NAME,FMT,X,N) \
	{\
		int i__;\
		PRINTMORE("%s =",NAME,0,0,0);\
		DBOUT[0] = ' ';\
		for (i__=0;i__ < (N);i__++)\
		{\
			sprintf(DBOUT+1,FMT,(X)[i__]);\
			myprint(DBOUT);\
		}\
		myeol();\
	}
/* debugging output to file */
#  define FPRINT(FMT,A,B,C,D) (\
        ((DBFILE) ? fprintf(DBFILE,"%s: ",NAMEFORWHERE) : 0),\
		((DBFILE) ? fprintf(DBFILE,(FMT),(A),(B),(C),(D)) : 0),\
		((DBFILE) ? fflush(DBFILE) : 0))

#if defined(MACINTOSH) || defined(WXWIN)
void myAlert(char * /*msgs*/);
#  define ALERT(FMT,A,B,C,D) ((void) sprintf(DBOUT,"%s: ",NAMEFORWHERE),\
				(void) sprintf(DBOUT+strlen(DBOUT),(FMT),(A),(B),(C),(D)),\
				myAlert(DBOUT), 0)
#else /*MACINTOSH || WXWIN*/
#  define ALERT(FMT,A,B,C,D) PRINT(FMT,A,B,C,D)
#endif /*MACINTOSH || WXWIN*/

#ifdef MACINTOSH
#  define DEBUGGER(FMT,A,B,C,D) ((void) sprintf(DBOUT,"%s: ",NAMEFORWHERE),\
				(void) sprintf(DBOUT+strlen(DBOUT),(FMT),(A),(B),(C),(D)),\
				CtoPstr(DBOUT), DebugStr((StringPtr) DBOUT), 0)

#else /*MACINTOSH || WXWIN*/
#  define DEBUGGER(FMT,A,B,C,D)
#endif /*MACINTOSH*/

/* conditional debugging output depending on value of global GUBED */
#  define PRINT1(I,FMT,A,B,C,D) \
		((GUBED & (I) ) ? PRINT((FMT),(A),(B),(C),(D)) : 0)
#  define PRINTMORE1(I,FMT,A,B,C,D) \
		((GUBED & (I) ) ? PRINTMORE((FMT),(A),(B),(C),(D)) : 0)
#  define FPRINT1(I,FMT,A,B,C,D) \
		((GUBED & (I) ) ? FPRINT((FMT),(A),(B),(C),(D)) : 0)

#  define ALERT1(I,FMT,A,B,C,D) \
	((GUBED & (I) ) ? ALERT((FMT),(A),(B),(C),(D)) : 0)

#ifdef MACINTOSH
#  define DEBUGGER1(I,FMT,A,B,C,D) \
	((GUBED & (I) ) ? DEBUGGER((FMT),(A),(B),(C),(D)) : 0)
#else /*MACINTOSH*/
#  define DEBUGGER1(I,FMT,A,B,C,D)
#endif /*MACINTOSH*/

#ifdef MPW
#  define assert(expression) \
	if (!(expression)) { \
		fprintf(stderr, "File %s; Line %d ## Assertion failed: " #expression "\n", \
				__FILE__, __LINE__); \
		abort(); \
	} /* from MPW {Cincludes}Assert.h */
#else /*MPW*/
#include <assert.h>
#endif /*MPW*/
#endif /*NDEBUG*/

#define PLACE(LOC) PRINT("Place %g\n",(double) (LOC),0,0,0)
#define BEFORE(LOC) PRINT("Before %s\n",(LOC),0,0,0)
#define AFTER(LOC) PRINT("After %s\n",(LOC),0,0,0)

/* To make it easier to get debugging output of (char *)'s */
#define SAFESTRING(S) ((S) ? (S) : "(nil)")

#ifdef MAIN__
char         DBOUT[DBOUTLENGTH];
FILE        *DBFILE = (FILE *) 0;
#else
extern char  DBOUT[DBOUTLENGTH];
extern FILE *DBFILE;
#endif /*MAIN__*/

#endif /*DBUGH__*/
