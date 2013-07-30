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

#ifndef TRANSSKELPROTOH__
#define TRANSSKELPROTOH__

/* TransSkel.c */
void SkelInit(void);
void SkelMain(void);
Boolean SkelGetNextEvent(Integer eventMask, EventRecord *theEvent, LongInt sleep);
void SkelChkOneMaskedEvent(Integer eventMask, LongInt sleep);
void SkelChkOneEvent(void);
int SkelWhoa(void);
void SkelClobber(void);
void SkelMenu(MenuHandle /*theMenu*/, ProcPtr /*pSelect*/, ProcPtr /*pClobber*/);
void SkelRmveMenu(MenuHandle /*theMenu*/);
void SkelApple(StringPtr /*aboutTitle*/, ProcPtr /*aboutProc*/);
void SkelWindow(WindowPtr /*theWind*/, ProcPtr /*pMouse*/, ProcPtr /*pKey*/, ProcPtr /*pUpdate*/, ProcPtr /*pActivate*/, ProcPtr /*pClose*/, ProcPtr /*pClobber*/, ProcPtr /*pIdle*/, Boolean /*frontOnly*/);
void SkelRmveWind(WindowPtr /*theWind*/);
void SkelGrowBounds(WindowPtr /*theWind*/, Integer /*hLo*/, Integer /*vLo*/, Integer /*hHi*/, Integer /*vHi*/);
void SkelBackground(ProcPtr /*p*/);

#endif /*TRANSSKELPROTOH__*/
