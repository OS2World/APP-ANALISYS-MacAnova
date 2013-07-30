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
 * Get the device containing most of a window's content rectangle and the
 * largest usable rectangle on that device.  If the device is the main
 * device, the rectangle is adjusted not to contain the menu bar area.
 *
 * The result is used by TransSkel for window zooming.  Normally, the
 * caller adjusts the top of the rectangle to account for the title bar
 * height, then insets it by a few pixels in order to leave a little
 * space around the window edges.  SkelGetWindowDevice() itself does
 * not account for the title bar height.  That responsibility is
 * left with the caller, which can call SkelGetWindTitleHeight() to
 * find this out.
 *
 * Returns true if the window overlaps some device in its current
 * position.  False can be returned, for instance, if an application
 * saves document window positions and a document is saved while
 * positioned on a second monitor, then opened on a system that doesn't
 * have a second monitor.
 *
 * The returned device value will be nil on systems that don't have GDevices
 * (i.e.,, that don't support Color QuickDraw), even if the function result
 * is true.
 *
 * If the window does not overlap any device, the device and devRect arguments
 * are filled in with the values for the main device.  The rectangle can
 * be used to position the window so it can be made visible.
 *
 * If the caller is not interested in the device or rectangle, nil
 * can be passed instead of the address of a variable.
 *
 * References: TN TB 30, HIN 6, HIN 7.
 */

#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment TransSkel
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

#include	"TransSkel.h"

pascal Boolean
SkelGetWindowDevice (WindowPtr wind, GDHandle *gd, Rect *devRect)
{
	Rect	r;
	Boolean	isMain;
	Boolean	result;

	/* get window content rect in global coordinates */

	SkelGetWindContentRect (wind, &r);
	result = SkelGetRectDevice (&r, gd, devRect, &isMain);
	if (isMain && devRect != (Rect *) nil)
	{
		devRect->top += SkelQuery (skelQMBarHeight);
	}
	return (result);
}
