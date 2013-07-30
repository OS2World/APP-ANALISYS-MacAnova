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
 * Determine height of a window's title bar.  This is determined as the
 * difference between the top of the window's structure and content rects.
 *
 * This function will not necessarily work for windows with strange shapes
 * or that have a title bar on the side.
 */

#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment TransSkel
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

#include	"TransSkel.h"

pascal short
SkelGetWindTitleHeight (WindowPtr w)
{
	Rect	content;
	Rect	structure;

	SkelGetWindContentRect (w, &content);
	SkelGetWindStructureRect (w, &structure);
	return (content.top - structure.top);
}
