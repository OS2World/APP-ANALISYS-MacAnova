/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 4.02 or later
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

#ifndef HANDLERSH__
#define HANDLERSH__

#ifndef EXTERN
#ifdef MAIN__
#define EXTERN
#define INIT(VAL) = VAL
#define INITDIM(N) N
#define INITARRAY(VALS) = { VALS }
#else
#define EXTERN extern
#define INIT(VAL)
#define INITDIM(N)
#define INITARRAY(VALS)
#endif /*MAIN__*/
#define COMMA ,
#endif /*EXTERN*/

/*
   various things related to interrupt handling
   961209 Moved globals and macros related to checking for interrupts
          from globkb.h to here
*/

#define NOCHECK        0
#define ONETICK        1
#define TWOTICKS       2
#define THREETICKS     3
#define FOURTICKS      4
#define TENTICKS      10
#define TWENTYTICKS   20

#ifdef MACINTOSH

#ifndef FGINTERRUPTINTERVAL
#define FGINTERRUPTINTERVAL 60L  /* minimum ticks between calls to SkelChkOneMaskedEvent()*/
#endif /*BACKGROUNDSLEEP*/

#ifndef BGINTERRUPTINTERVAL
#define BGINTERRUPTINTERVAL 5L 
#endif /*BGINTERRUPTINTERVAL*/

EXTERN unsigned long   InterruptInterval   INIT(FGINTERRUPTINTERVAL); 
EXTERN unsigned long   FgInterruptInterval INIT(FGINTERRUPTINTERVAL); 
EXTERN unsigned long   BgInterruptInterval INIT(BGINTERRUPTINTERVAL); 

#define DEFAULTTICKS   InterruptInterval
#define ALTERNATETICKS InterruptInterval
#else /*MACINTOSH*/
EXTERN unsigned long DEFAULTTICKS INIT(TENTICKS);
EXTERN unsigned long ALTERNATETICKS INIT(TENTICKS);
#endif /*MACINTOSH*/

#ifndef NOPROTOTYPES
#ifdef SIGNALARG
void fpeRoutine(int);
void intRoutine(int);
#else /*SIGNALARG*/
void fpeRoutine(void);
void intRoutine(void);
#endif /*SIGNALARG*/

long interrupted(unsigned long);
long timetocheck(unsigned long);
#else /*NOPROTOTYPES*/
#ifdef SIGNALARG
void fpeRoutine(/*int*/);
void intRoutine(/*int*/);
#else /*SIGNALARG*/
void fpeRoutine(/*void*/);
void intRoutine(/*void*/);
#endif /*SIGNALARG*/

long interrupted(/*void*/);
long timetocheck(/*void*/);
#endif /*NOPROTOTYPES*/

/*
	The following are fossils
*/
#define SETUPINT0(LABEL) INTERRUPT = INTNOTSET; *OUTSTR = '\0'

#define RETURN0 return

#define CATCHINT0
#define IGNOREINT0

#ifdef SETJMP
/* 
  LABEL should be label to transfer to on interrupt or aborted output
*/
#define CATCHINT signal(SIGINT,intRoutine)
#define IGNOREINT signal(SIGINT,SIG_IGN)
#define SETUPINT(LABEL)	if(RDEPTH < MAXRDEPTH)\
						{\
							INTWHERE[RDEPTH+1] = NAMEFORWHERE;\
							if(setjmp(RestartBuf[++RDEPTH]))\
							{\
								*OUTSTR = '\0';\
								goto LABEL;\
							}\
						}\
						INTERRUPT = INTNOTSET;\
						*OUTSTR = '\0'


#define	RETURN RDEPTH -= (RDEPTH > 0) ? 1 : 0; return

#else /*SETJMP*/
#define CATCHINT0
#define IGNOREINT0
#define SETUPINT(LABEL) SETUPINT0(LABEL)
#define RETURN RETURN0
#endif /*SETJMP*/

#undef RETURN /* make sure it's not defined; no longer needed */

#ifndef MAXRDEPTH
#ifdef SETJMP
#define MAXRDEPTH       15
#else /*SETJMP*/
#define MAXRDEPTH        1
#endif /*SETJMP*/
#endif /*MAXRDEPTH*/

/*
   940210 changed names of these constants.  First 3 were NOTLOCAL, LOCALINT,
   and LOCALFPE
*/

#define INTNOTSET       0x00000  /* no interrupt in progress */
#define INTSET          0x10000  /* interrupt occurred */
#define FPESET          0x20000  /* FP error occurred */
#define PRINTABORT      0x40000  /* printing has been aborted */

#ifdef MACINTOSH
#ifndef   VALUECHUNK
#define   VALUECHUNK  50 /* check for interrupt every VALUECHUNK answers */
#endif /*VALUECHUNK*/
#define  checktime(I)  (((I) + 1) % VALUECHUNK == 0)
#endif /*MACINTOSH*/

/* global variables related to interrupt processing */
EXTERN long     INTERRUPT INIT(INTNOTSET);
EXTERN long     RDEPTH INIT(0); /* stack pointer for RestartBuf */
EXTERN char    *INTWHERE[INITDIM(MAXRDEPTH+1)]; /* global used in debugging */

#define checkInterrupt(LABEL)\
	if (INTERRUPT != INTNOTSET)\
	{\
		goto LABEL;\
	}

#ifdef NPRODUCTS
/*
   some operations call interrupted() about every MAXPRODUCTS products
   or quotients
*/
EXTERN long            Nproducts INIT(0);
EXTERN long            MAXPRODUCTS INIT(NPRODUCTS);

#define clearNproducts()  (Nproducts = 0)
#define incNproducts(INC) (Nproducts += (INC))
#define testNproducts(LABEL) \
	if (Nproducts > MAXPRODUCTS)\
	{\
		if (interrupted(DEFAULTTICKS) != INTNOTSET)\
		{\
			goto LABEL;\
		}\
		clearNproducts();\
	}

#else /*NPRODUCTS*/
#define clearNproducts()
#define incNproducts(INC)
#define testNproducts(LABEL)
#endif /*NPRODUCTS*/

#include <setjmp.h>
#include <signal.h>

/*
If SETJMP is defined, RestartBuf[] is a stack of jmp_buf's
RestartBuf[0] is set by setjmp() near start
RestartBuf[j] is set at "depth" j to allow catching interrupts or
output aborts so as to be able to clean up.
If SETJMP is not defined, RestartBuf[0] is a single jmp_buf used in
main().
*/

#ifndef CPROTO
EXTERN jmp_buf  RestartBuf[INITDIM(MAXRDEPTH+1)];
#else
EXTERN int      RestartBuf[INITDIM(MAXRDEPTH+1)][_JBLEN];
#endif /*CPROTO*/


#undef	EXTERN
#undef	INIT
#undef	INITDIM
#undef	INITARRAY
#undef  COMMA

#endif /*HANDLERSH__*/
