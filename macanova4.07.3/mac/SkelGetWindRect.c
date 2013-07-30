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
 * Calculate content or structure rectangle for a window. These are
 * core routines because they are called by other core routines,
 * e.g., SkelGetWindowDevice() and SkelGetWindTitleHeight ().
 */

#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment TransSkel
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

# include	"TransSkel.h"

# define	kTranslate	0x4000


/*
 * Get content rectangle and convert it to global coordinates
 */

pascal void
SkelGetWindContentRect (WindowPtr w, Rect *rp)
{
	GrafPtr	oldPort;
	Rect	content;

	GetPort (&oldPort);
	SetPort (w);
	*rp = w->portRect;
#ifdef MPW
	LocalToGlobal ((Point *) &rp->top);
	LocalToGlobal ((Point *) &rp->bottom);
#else /*MPW*/
	LocalToGlobal (&topLeft (*rp));
	LocalToGlobal (&botRight (*rp));
#endif /*MPW*/
	SetPort (oldPort);
}


/*
 * Get structure rectangle.  This is already in global coordinates, but the
 * tricky part is that it isn't valid if the window is invisible.
 *
 * If window's visible, the structure region's valid, so get the bounding box.
 *
 * If the window's not visible, fling it out into space, make it visible, get
 * the structure region bounding box, make it invisible again and restore it to
 * its normal position.  Use ShowHide() for this since  it doesn't change the
 * window's hiliting or position in the stacking order.  The rectangle
 * calculated this way has to be moved back, too, since it's obtained when the
 * window is in flung position.
 *
 * I have seen similar code that also saves and restored the window's userState,
 * but Inside Macintosh (Toolbox Essentials, p. 4-70) explicitly states that
 * the userState isn't modified when you just move a window, so I don't see the
 * point.
 */

pascal void
SkelGetWindStructureRect (WindowPtr w, Rect *rp)
{
	Rect	content;

	if (((WindowPeek) w)->visible)
	{
		*rp = (**(* (WindowPeek) w).strucRgn).rgnBBox;
	}
	else
	{
		SkelGetWindContentRect (w, &content);				/* get upper-left coords */
		MoveWindow (w, kTranslate, content.top, false);		/* fling window */
		ShowHide (w, true);
		*rp = (**(* (WindowPeek) w).strucRgn).rgnBBox;
		ShowHide (w, false);
		MoveWindow (w, content.left, content.top, false);	/* unfling window */
		OffsetRect (rp, content.left - kTranslate, 0);		/* unfling struct rect */
	}
}
