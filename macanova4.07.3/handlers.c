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
   Contains routines pertaining to interrupt handling

   961204 Rearranged with a number of changes to interrupted()
   for WXWin.

   990215 Changed myerrorout() to putOutMsg()
*/
#include "globals.h"

#ifdef MACINTOSH
#include "macIface.h"
#endif /*MACINTOSH*/

#ifdef WXWIN
#include "wx/wxIface.h"
#endif  /* WXWIN */

#ifdef MACINTOSH
/*Macintosh stuff*/

#if (0)
	/*debugging stuff*/
#define NTICKS     100
#define MAXTICKS  1000
static Symbolhandle    SymhTicks = (Symbolhandle) 0;
static long            CurrentN  = -1;
static long            Lastticks = 0;

static void recordTicks(void)
{
	/*debugging stuff*/
	if (GUBED & 8)
	{
		long            thisticks, mticks, i;
		if (CurrentN < 0)
		{
			SymhTicks = Lookup("TICKS");
			if (SymhTicks == (Symbolhandle) 0)
			{
				mticks  = NTICKS*(MAXTICKS/NTICKS);
				SymhTicks = RInstall("TICKS", mticks);
			}
			else
			{
				mticks = symbolSize(SymhTicks);
			}
			for (i = 0;i < mticks; i++)
			{
				setMissing(DATAVALUE(SymhTicks, i));
			}
			Lastticks = TickCount();
			CurrentN = 0;
		}
		else if (CurrentN < DIMVAL(SymhTicks, 1))
		{
			thisticks = TickCount();
			DATAVALUE(SymhTicks, CurrentN++) = (double) (thisticks - Lastticks);
			Lastticks = thisticks;
		}
		else
		{
			GUBED &= ~8;
			CurrentN = -1;
		}
	} /*if (GUBED & 8)*/
	else
	{
		CurrentN = -1;
	}
} /*recordTicks()*/	
#endif /*0*/

/*
	Temporary idle procedure that will be executed at most once
*/

static pascal void
stopAfterOneEvent(void)
{
	SkelStopEventLoop();
#if (0)
	recordTicks();/*DEBUGGING*/
#endif /*0*/
} /*stopAfterOneEvent()*/

/*
	Routine to check one event and route it, without looping through
	window handlers.
*/

pascal void
SkelChkOneMaskedEvent (Integer mask, LongInt waitTime)
{
	Integer         oldTSMask;
	LongInt         oldFgWaitTime, oldBgWaitTime;
	SkelIdleProcPtr oldIdle;

	oldTSMask = SkelGetEventMask(); /* and event mask */
	SkelGetWaitTimes(&oldFgWaitTime, &oldBgWaitTime);
	oldIdle = SkelGetIdle();/* get current idle proc */
	
	/* install new idle & mask */
	SkelSetEventMask(mask);
	SkelSetWaitTimes(waitTime, oldBgWaitTime);
	SkelSetIdle(stopAfterOneEvent);

	SkelEventLoop();        /* handle given event types only */

	/* restore stuff that was changed */
	SkelSetEventMask(oldTSMask);
	SkelSetWaitTimes(oldFgWaitTime, oldBgWaitTime);
	SkelSetIdle(oldIdle);
} /*SkelChkOneMaskedEvent ()*/

/*
	Interface to check for one event of any type using default wait time
	Used primarily to check for interrupts, but also allows user access to
	desk accessories, multi-finder, etc.
*/

pascal void
SkelChkOneEvent (void)
{
	LongInt       fgWaitTime;

	SkelGetWaitTimes(&fgWaitTime, (LongInt) nil);
	SkelChkOneMaskedEvent (everyEvent ^ keyUpMask, fgWaitTime);

} /*SkelChkOneEvent()*/
#endif /*MACINTOSH*/

/*
  interrupt handlers; these were in main.c

  The scheme that was tried, and remains of which are still in place, was for
  each function to call setjmp() using macro SETUPINT(label), where label
  was a label to jump to in case a longjmp() was executed; the various
  jmp_buf's that were required were in a stack.  Since termination of
  output when the screen is full by typing 'q' is handled (except on the Mac)
  by calling longjmp(), this scheme also stopped memory leaks when output
  was terminated.  Currently fmyeol() sets INTERRUPT to PRINTABORT and
  calls intRoutine() which executes the longjmp() call.

  Here are comments from handlers.h

	If SETJMP is defined, RestartBuf[] is a stack of jmp_buf's
	RestartBuf[0] is set by setjmp() near start
	RestartBuf[j] is set at "depth" j to allow catching interrupts or
	output aborts so as to be able to clean up.
	If SETJMP is not defined, RestartBuf[0] is a single jmp_buf used in
	main().

  The original purpose of the sehem was to provide a local means to clean up
  allocated storage to prevent memory "leaks".  Unfortunately, although
  it worked most of the time, occasionally it would crash with a message
  stating the jmpbuffer was bad.  Eventually, it was disabled and the
  prevention of memory leaks handled by "registering" temporary storage
  as components of symbols of type GARBAGE in the symbol table.  These
  are automatically disposed of at cleanup time after an interrupt.

  Macintosh
  An interrupt is recognized in doFile() in macMain.c where INTERRUPT is set
  to INTSET.  This can happen only in the event loop or if
  SkelChkOneEvent() is called which is what interrupted() does on the mac.
  Thus interrupt is effective only if a function explicitly calls
  interrupted(); it does not happen automatically.

  WX versions
  An interrupt is recognized in MacAnovaTextFrame::OnMenuCommand() (in
  wxtframe.cc) where INTERRUPT is set to INTSET.  This can happen only
  in the event loop or if if myYield() or wxYield() is called, which is
  what interrupted() does.  However, interrupted() calls timetocheck()
  to determine if myYield() should be called.  Every HOWOFTEN calls
  to interrupted, clock() is called and if enough time has passed
  myYield() is called.

  The better way to do it is to have global Ticks updated automatically.
  But in Motif, unless wxYield() is called, the timers stop running.
  If USETICKTIMER is defined, code for referencing Ticks is enabled.

  Unix & DOS (non-WX) versions
  An interrupt causes a transfer to intRoutine() which closes batch files,
  prints a message, sets INTERRUPT to INTSET, and calls longjmp().
  Under the scheme, this would have transferred back to the function which
  was active when the interrupt came.  Function interrupted() sets
  INTERRUPT to INTNOTSET (0) and returns INTNOTSET.
 
  Global variable INTERRUPT is set to FPESET on a floating point interrupt,
  to INTSET on any other interrupt.  The default value for interrupt is
  INTNOTSET with value zero.  At the "local" level, if INTERRUPT == INTNOTSET
  nothing needs to be done.  If INTERRUPT != INTNOTSET, then storage allocated
  needs to be released and an error exit taken.

*/

#if defined(MACINTOSH) || defined(WXWIN)

static unsigned long LastTickcount = 0;

#ifdef WXWIN
#ifdef USETICKTIMER
#define TickCount() (Ticks*MilliSecondsPerTick*60/1000)
#else /*USETICKTIMER*/
/*
   There is an unacceptable penalty in calling either time() (better) or
   mygettime() (worse) on every call.  Hence time is checked only every
   HOWOFTEN calls.
*/
#include <time.h>
#define HOWOFTEN    5 /* how frequently do we do time check */
#define SIXTIETHSPERTICK 60

#if (1)
static long     Counter = -1;
static time_t   Now = 0;

#define TickCount() \
	((Counter < 0) ? (Now = time((time_t *) 0),Ticks = Counter = 0) :\
	 (Ticks = (++Counter % HOWOFTEN == 0) ? time((time_t *) 0) - Now : Ticks),\
	  SIXTIETHSPERTICK*Ticks)

#else /*1*/
static unsigned long TickCount(void)
{
	static          long Counter = -1;
	static time_t   Now = 0;

	if (Counter < 0)
	{
		Now = time((time_t *) 0);
		Counter = 0;
	}
	if (++Counter % HOWOFTEN == 0)
	{
		Ticks = time((time_t *) 0) - Now;
	}
	return (SIXTIETHSPERTICK*Ticks);
} /*TickCount()*/
#endif

#endif /*USETICKTIMER*/
#else /*WXWIN*/
#define myYield() SkelChkOneMaskedEvent (everyEvent ^ keyUpMask, InterruptSleep)
#endif /*WXWIN*/

long timetocheck(unsigned long tickIncrement)
{
	unsigned long     tickCount;
	
	tickCount = TickCount();
	if (tickCount >= (LastTickcount + tickIncrement))
	{
		LastTickcount = tickCount;
		return(1);
	}
	return(0);
} /*timetocheck()*/

#else /*MACINTOSH || WXWIN*/

long timetocheck(unsigned long tickIncrement)
{
	return(1);
} /*timetocheck()*/
#endif /*MACINTOSH || WXWIN*/
	
/* Various versions of interrupted() */
#if defined(WXWIN) || defined(MACINTOSH)
/*
   Version of interrupted() for Macintosh and WX versions
	If Command (Ctrl) I or Command period or Interrupt on File menu,
	set INTERRUPT to INTSET (defined in handlers.h as 0x10000)
	Test of whether an interrupt has occurred should be
	  if(interrupted(DEFAULTTICKS) != INTNOTSET){...process interrupt...}
	
	The only place where INTERRUPT should be set to INTNOTSET is in
	cleanitup() in mainpars.c

	There is an unacceptable penalty for calling myYield()
	(SkelChkOneMaskedEvent() on Macintosh).  Hence the use of
	timetocheck().
*/

long interrupted(unsigned long tickIncrement)
{ /*WxWin and Macintosh version*/
#if (0)
	WHERE("interrupted");
#endif /*0*/

	if (INTERRUPT == INTNOTSET &&
		tickIncrement > 0 && timetocheck(tickIncrement))
	{
		myYield();
	}

	return (INTERRUPT);
} /*interrupted()*/

#elif defined(DJGPP)

#include <go32.h>

long interrupted(unsigned long tickIncrement)
{ /* DJGPP DOS version */
	if( _go32_was_ctrl_break_hit() )
	{
		INTERRUPT = INTNOTSET;
#ifdef SIGNALARG
		intRoutine(0);
#else /*SIGNALARG*/
		intRoutine();
#endif /*SIGNALARG*/
	}
	return (INTERRUPT);
} /*interrupted()*/

#else

long interrupted(unsigned long tickIncrement)
{ /*non-DJGPP, non-WX Unix and DOS version*/
	return (INTERRUPT);
} /*interrupted()*/

#endif
/* end of versions of interrupted()*/

#if !defined(MACINTOSH) && !defined(WXWIN)

/*
   True interrupt handling routines
*/

/* take care of floating point errors for Unix & DOS */
#ifdef SIGNALARG
void fpeRoutine(int sig)
#else /*SIGNALARG*/
void fpeRoutine(void)
#endif /*SIGNALARG*/
{
	long      screenheight = SCREENHEIGHT;
	WHERE("fpeRoutine");
	
	SCREENHEIGHT = 0; /* make sure output will not trigger new interrupt */

	putOutErrorMsg("ERROR: floating point exception");
	closeBatch(1); /* shut down all batch files */
	SCREENHEIGHT = screenheight;
	INTERRUPT = FPESET;
	
#ifdef SIGNALARG
	intRoutine(0);
#else /*SIGNALARG*/
	intRoutine();
#endif /*SIGNALARG*/
	/* no return */
} /*fpeRoutine()*/

/* take care of SIGINT for Unix & DOS */
#ifdef SIGNALARG
void intRoutine(int sig)
#else /*SIGNALARG*/
void intRoutine(void)
#endif /*SIGNALARG*/
{
	long     screenheight = SCREENHEIGHT;
	WHERE("intRoutine");
	
	SCREENHEIGHT = 0; /* make sure any output will not trigger new interrupt */

#if (0)
	if(GUBED & 65536 && INTWHERE[RDEPTH] != (char *) 0)
	{
		PRINT("INTWHERE[%ld] = %s\n", RDEPTH, INTWHERE[RDEPTH],0,0);
	}
#endif /*0*/

	if(INTERRUPT == INTNOTSET)
	{ /* actual interrupt */
		SCREENHEIGHT = 0; /* make sure output will not trigger new interrupt */
		myeol();
		myeol();
		putOutMsg("  *** INTERRUPT ***");
		myeol();
		SCREENHEIGHT = screenheight;
		INTERRUPT = INTSET;
		closeBatch(1); /* shutdown all batch files */
	}
	SCREENHEIGHT = screenheight;
	NLINES = 0;
	/* 
	  It is the responsibility of each function that sets up RestartBuf
	  to increment and decrement RDEPTH.  This should be automatic if
	  macros SETUPINT and RETURN are used.
	  [comment is a fossil]
	*/
	longjmp(RestartBuf[RDEPTH], 0);
} /*intRoutine()*/

#endif /*!MACINTOSH && !WXWIN*/

