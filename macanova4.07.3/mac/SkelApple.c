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
 * SkelApple.c - TransSkel 3.0 Apple menu handler
 */

#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment TransSkel
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

#include	"TransSkel.h"

/*
 * Default Apple menu handler support variables
 *
 * appleID is set to skelAppleMenuID if SkelApple() is called and
 * becomes the id of the Apple menu.
 *
 * appleSelect is the procedure to execute if there is a non-DA
 * selection from the Apple menu.
 *
 * appleApplItems is true if the application installs one or more
 * items of its own at the top of the Apple menu.
 */

static MenuHandle appleMenu;
static short    appleID = 0;
static SkelMenuSelectProcPtr appleSelect = nil;
static Boolean  appleApplItems = false;


/*
 * Apple menu handler routines
 *
 * If the application installed its own items into the menu and the
 * selection was one of them, pass the item number to the selection
 * procedure.  Otherwise the item is a desk accessory. The port is
 * saved and restored because it is not always preserved correctly
 * across the call to OpenDeskAcc() (IM I-440).
 *
 * DoAppleClobber() disposes of the Apple menu and resets the other
 * Apple menu handler variables.
 */


static pascal void
DoAppleItem(short item)
{
	GrafPtr         curPort;
	Str255          str;
	Handle          h;
	short           i;

	/*
	 * If there are application items, determine if item is one of them.
	 * Can tell by tracking backward through the items; if the gray line
	 * separating application items and DA's if not found before top of
	 * menu is reached, then its an application item, else a DA.  This
	 * strategy requires that there be no "-" items in the items string
	 * passed to SkelApple().
	 */
	if (appleApplItems)
	{
		for (i = item; i > 0; i--)	/* look from current to item 1 */
		{
			GetItem(appleMenu, i, str);
			if (str[0] == 1 && str[1] == '-')
			{
				break;
			}
		}
		if (i == 0)	/* reached top without seeing line */
		{
			if (appleSelect != nil)	/* call select proc if there is one */
			{
				(*appleSelect) (item);
			}
			return;
		}
	}

	/* either no application items or selection isn't one of them */
	GetPort(&curPort);
	GetItem(appleMenu, item, str);	/* get DA name */
	SetResLoad(false);
	h = GetNamedResource('DRVR', str);
	SetResLoad(true);
	if (h != (Handle) nil)
	{
		ResrvMem(SizeResource(h) + 0x1000);
		(void) OpenDeskAcc(str);	/* open it */
	}
	SetPort(curPort);
}


static pascal void
DoAppleClobber(MenuHandle menu)
{
	DisposeMenu(menu);	/* menu will be == appleMenu here */
	appleID = 0;
	appleApplItems = false;
	appleSelect = nil;
}


/*
 * Install a handler for the Apple menu.
 *
 * SkelApple() is called if TransSkel is supposed to handle the apple
 * menu itself.
 *
 * items contains the title of any items the application
 * wants to install at the top of the menu.  If items is empty or nil, then
 * only desk accessories are put into the menu.  If not empty, then the items
 * are installed at the top, followed by a gray line, then the desk
 * accessories.
 *
 * doSelect is the procedure to be called when a non-DA selection is
 * made and is passed the item number.  If doSelect is nil, the selection
 * is ignored.
 *
 * SkelApple() does not cause the menubar to be drawn, so if the Apple
 * menu is the only menu, DrawMenuBar() must be called afterward.
 *
 * No value is returned, unlike SkelMenu().  It is assumed that
 * SkelApple() will be called so early in the application that the call
 * to SkelMenu() is virtually certain to succeed.  If it doesn't,
 * presumably there's little hope for the application anyway.
 */

pascal void
SkelApple(StringPtr items, SkelMenuSelectProcPtr doSelect)
{
	appleID = skelAppleMenuID;
	/* 024 = apple character */
	appleMenu = NewMenu(appleID, (StringPtr) "\p\024");
	if (items != (StringPtr) nil && items[0] > 0)
	{
		/* application has own items in menu */
		appleApplItems = true;
		/* add items */
		AppendMenu(appleMenu, items);
		/* add gray line */
		AppendMenu(appleMenu, (StringPtr) "\p(-");
		appleSelect = doSelect;
	}
	AddResMenu(appleMenu, 'DRVR');	/* add desk accessories */
	(void) SkelMenu(appleMenu, DoAppleItem, DoAppleClobber, false, false);
}

/*
	added by C. Bingham, 940606
*/

pascal MenuHandle SkelGetAppleMenu(void)
{
	return (appleMenu);
}
