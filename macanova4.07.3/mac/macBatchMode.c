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



#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment MacBatch
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


/*
	Window handlers for batch mode, modelled on those in macInput.c
	Version of 940419

	960811 modified call to startupMsg() to have argument quiet
	970609 null lines no longer passed to parser
	980516 BatchModeMain() now keeps control until timetocheck() returns non-zero
	       this yielded dramatic increase in speed.
    990215 Changed myerrorout() to putOutErrorMsg()
*/

/*
	Based on a TransSkel multiple-window demonstration: TextEdit module

	14 June 1986		Paul DuBois
*/
#ifdef PERFORMANCE
#include "profile.h"
#endif /*PERFORMANCE*/

#include "globals.h"
#include "mainpars.h"
#include "macIface.h" /* Note: macIface.h replaces MultiSkel.h */

extern int      yydebug; /* set 1 for yacc debugging output */

/*
	Window handler procedures for non-interactive mode operation
	There is a single invisible window with no purpose except to hang
	an "idle process" to using TransSkel.  In addition,
	there is a single nonmodal dialog box window with a single button
	labelled 'Quit'.  If this is clicked the user is given a chance to
	continue.
*/

void BatchModeInit(Boolean quiet)
{
	WHERE("BatchModeInit");

	SkelSetIdle((SkelIdleProcPtr) BatchModeMain);
	BATCHECHO[0] = 0;
	BatchDialog = GetNewDialog(BATCHMODE, (Ptr) nil, (WindowPtr) -1);
	if (BatchDialog == (DialogPtr) 0)
	{
		goto errorExit;
	}
	if (!SkelDialog(BatchDialog,\
				   (SkelWindEventProcPtr) BatchDialogEvent,
				   (SkelWindCloseProcPtr) nil,
				   (SkelWindClobberProcPtr) DialogClobber))
	{
		goto errorExit;
	}

	installScreenDims(80, 26);
	saveFormat();
	 /*
	 	High resolution plots illegal in
	    non-interactive mode
	 */
	DEFAULTDUMBPLOT = 1;
	startupMsg(quiet);

	UNLOADSEG(startupMsg);
	MyShowWindow((WindowPtr) BatchDialog);

	return;

  errorExit:
	myAlert("FATAL ERROR: Unable to start up MacAnova");
	FatalError = 1;

	return;
} /*BatchModeInit()*/



pascal void BatchModeMain(void)
{
	Integer         done;
	Integer         inputStatus;
	WHERE("BatchModeMain");

	done = (FatalError || INTERRUPT != INTNOTSET);
	while (!done && !timetocheck(DEFAULTTICKS))
	{
		unsigned char   c = ' '; /*not '\0'*/
		
		SetCursor(&WATCH);

		if ((inputStatus = getinput(), inputStatus) > 0)
		{ /* we have an input line or have hit EOF on batch file*/
			if (INPUTSTRING != (unsigned char **) 0 &&
			   (c = (*INPUTSTRING)[0], c) != '\0')
			{ /* able to create INPUTSTRING */
				Running = 1;
				if (inputStatus > 1)
				{
					putOutErrorMsg("WARNING: extra characters after end of line");
				}
#ifdef PERFORMANCE
				if (GUBED & 32768 && (!ProfileStatus || ProfileStatus == PROFILEOFF))
				{
					(void) PerfControl(ThePGlobals, true);
					ProfileStatus = PROFILEON;
				}
				if (!(GUBED & 32768) && ProfileStatus == PROFILEON)
				{
					(void) PerfControl(ThePGlobals, false);
					ProfileStatus = PROFILEOFF;
				}
#endif /*PERFORMANCE*/

#ifdef RECOGNIZEBANG
				if (c == BANG)
				{
					done = processBang();
				}
				else
#endif /*PROCESSBANG*/
				{
					done = DoStats(); /* go process input line */
				}
				

				FatalError = (done == FATALERROR) ? 1 : FatalError;

				done = (done == END || FatalError != 0 ||
						INTERRUPT != INTNOTSET);

#ifdef PERFORMANCE
				if (ProfileStatus == PROFILEON)
				{
					(void) PerfControl(ThePGlobals, false);
					ProfileStatus = PROFILEOFF;
				}
#endif /*PERFORMANCE*/

				cleanitup(); /* post command housekeeping*/

			} /*if (INPUTSTRING != (unsigned char **) 0)*/
			else if (INPUTSTRING == (unsigned char **) 0)
			{ /* can't go on */
				done = 1;
			}

			if (!done)
			{
				if (MAXERRORS1 && NERRORS >= MAXERRORS1)
				{
					if (BDEPTH > 0)
					{
						myprint("WARNING: too many errors on batch file ");
						putOutErrorMsg(*INPUTFILENAMES[BDEPTH]);
						closeBatch(1); /* shut down all batch files*/
					}
					if (MAXERRORS && NERRORS >= MAXERRORS)
					{
						putOutErrorMsg("ERROR: too many errors; execution terminated");
						done = FATALERROR;
						FatalError = 1;
					}
				} /*if (MAXERRORS1 && NERRORS >= MAXERRORS1)*/
			} /*if (!done)*/
		} /*if ((inputStatus = getinput()) > 0)*/
		else if (c != '\0')
		{
			done = 1;
		}
		FatalError = (inputStatus < -1) ? 1 : FatalError;
		if (FatalError != 0)
		{ /* panic quit, run out of space */
			done = 1;
		}
		if (!done)
		{
			yydebug = (GUBED & 2048) ? 1 : 0;
			putprompt((Char *) 0);
		}
	} /*if (!done)*/

	if (done)
	{
		(void) quitIt();
	}
} /*BatchModeMain()*/

pascal void BatchDialogEvent (Integer itemHit, EventRecord * theEvent)
{
	Integer          mods = theEvent->modifiers;
	LongInt          message = theEvent->message;
	unsigned char    ch = message & charCodeMask;

	if (itemHit == 0)
	{ /* must be un-processed command key */
		ch = (ch == Period || ch == 'i') ? 'I' : ch;
		if ((ch == 'q' || ch == 'Q' || ch == 'I') && !quitAlert(ch))
		{
			INTERRUPT = INTSET;
		}
	}
	else if (itemHit == ok && !quitAlert('I'))
	{
		INTERRUPT = INTSET;
	}
}


