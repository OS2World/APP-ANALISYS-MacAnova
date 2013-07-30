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
 * Given a rectangle, determine the following values:
 * - Which device contains more of the rectangle than any other
 * - The device rectangle (this includes the menu bar area if the device
 * is the main device)
 * - Whether or not the device is the main device
 *
 * These values are stuffed into the arguments, which are passed as
 * variable addresses.  If you're not interested in a particular value,
 * pass nil for the corresponding argument.
 *
 * The return value if true if the rectangle overlaps some device,
 * false if it lies outside all devices.  If the rectangle overlaps no
 * device, non-nil arguments are filled in with the main device, the main
 * device rect, and true, respectively.  This is useful, e.g., for callers
 * that may want to reposition a window if its content rectangle isn't
 * visible on some monitor.
 *
 * The returned device value will be nil on systems that don't have GDevices
 * (i.e.,, that don't support Color QuickDraw), even if the function result
 * is true.
 *
 * References: TN TB 30.
 */

#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment TransSkel
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

#include	"TransSkel.h"

pascal Boolean
SkelGetRectDevice (Rect *rp, GDHandle *rGD, Rect *devRect, Boolean *isMain)
{
	GDHandle	gd, curGD;
	Rect		gdRect, curRect, iSectRect;
	long		maxArea, area;
	Boolean		main = false;
	Boolean		result;
		
	gd = (GDHandle) nil;				/* no device for rectangle known yet */

	if (!SkelQuery (skelQHasColorQD))
	{
		/*
		 * No Color QuickDraw implies only one screen, which is therefore
		 * the main device.  Test rectangle against full screen, setting
		 * result true if they intersect.
		 */
#if defined( MPW) || defined(MW_CW)
		GrafPtr    screenPort;

		GetWMgrPort (&screenPort);
		gdRect = screenPort->portRect;
#else /*MPW*/
		gdRect = screenBits.bounds;
#endif/*MPW*/
		main = true;
		result = SectRect (rp, &gdRect, &iSectRect);
	}
	else
	{
		/* determine device having maximal overlap with r */

		maxArea = 0;
		for (curGD = GetDeviceList (); curGD != (GDHandle) nil; curGD = GetNextDevice (curGD))
		{
			/* only consider active screen devices */
			if (!TestDeviceAttribute (curGD, screenDevice)
				|| !TestDeviceAttribute (curGD, screenActive))
			{
				continue;
			}
			curRect = (**curGD).gdRect;
			if (!SectRect (rp, &curRect, &iSectRect))
			{
				continue;
			}
			area = (long) (iSectRect.right - iSectRect.left)
					* (long) (iSectRect.bottom - iSectRect.top);
			if (maxArea < area)
			{
				maxArea = area;
				gd = curGD;
				gdRect = curRect;
				result = true;	/* rectangle overlaps some device */
			}
		}
		if (gd == (GDHandle) nil)	/* rectangle overlaps no device, use main */
		{
			gd = GetMainDevice ();
			gdRect = (**gd).gdRect;
			result = false;
		}
		main = (gd == GetMainDevice ());
	}

	/* fill in non-nil arguments */

	if (rGD != (GDHandle *) nil)
	{
		*rGD = gd;
	}
	if (devRect != (Rect *) nil)
	{
		*devRect = gdRect;
	}
	if (isMain != (Boolean *) nil)
	{
		*isMain = main;
	}

	return (result);
}
