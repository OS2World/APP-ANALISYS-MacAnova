/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 3.36 or later
*(C)*
*(C)* Copyright (c) 1994 by Gary Oehlert and Christopher Bingham
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
*(C)* Most modules in this file are Copyright (c) by Paul DuBois and have
*(C)* been placed in the public domain.  The additions and changes made by
*(C)* C. Bingham for use with MPW and interfacing with MacAnova are also being
*(C)* placed in the public domain.
*(C)* Macanova related changes are bracketed by #ifdef KB ... #endif
*(C)* Purely MPW changes are bracketed by #ifdef MPW ... #endif
*/

/*
 * SkelDoEvents (mask) - process all pending events of types indicated in mask
 * SkelDoUpdates () - process all pending update events
 *
 * These routines may be called any time subsequent to the call of SkelInit().
 */

#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment TransSkel
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

#include	"TransSkel.h"

static short    sdeMask;


/*
 * Make sure any events of proper type are processed before
 * proceeding.  I.e., wait until there are no more events of the
 * type we're waiting for, then terminate SkelDoEvents().
 */

static pascal void
CheckEvents(void)
{
	EventRecord     event;

	if (!EventAvail(sdeMask, &event))
	{
		SkelStopEventLoop();
	}
}


/*
 * Process all events of type(s) given in mask.  It is possible to call this
 * recursively.
 * Operation:
 * - Save current SkelDoEvents() mask, current TransSkel event mask, and
 * current background procedure.
 * - Install the new mask into TransSkel and save a copy in a local variable.
 * Install a new background procedure that checks whether any events of the
 * desired type(s) are available or not.
 * - Call SkelMain() to initiate an event loop.  The background task calls
 * SkelWhoa() to terminate SkelMain() when there are no more events of
 * interest available.
 * - Restore the previous background procedure and TransSkel mask, and
 * previous SkelDoEvents() mask.  The latter is necessary in case this is
 * a recursive call.
 */

pascal void
SkelDoEvents(short mask)	/* can be called recursively */
{
	short           oldSdeMask;
	short           oldTSMask;
	SkelIdleProcPtr oldIdle;

	oldIdle = SkelGetIdle();/* get current idle proc */
	oldTSMask = SkelGetEventMask();	/* and event mask */
	oldSdeMask = sdeMask;	/* and SkelDoEvents() processing types */

	SkelSetIdle(CheckEvents);	/* install new idle & mask */
	SkelSetEventMask(mask);
	sdeMask = mask;		/* <- so CheckEvents can find mask */

	SkelEventLoop();	/* handle given event types only */

	SkelSetIdle(oldIdle);	/* restore stuff that was changed */
	SkelSetEventMask(oldTSMask);
	sdeMask = oldSdeMask;
}


pascal void
SkelDoUpdates(void)
{
	SkelDoEvents(updateMask);
}
