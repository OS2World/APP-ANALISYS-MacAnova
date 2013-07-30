/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 4.06 or later
*(C)*
*(C)* Copyright (c) 1997 by Gary Oehlert and Christopher Bingham
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
 * File:	wxIface.h
 */



/*
  Header unifying wxwin interface
  970910 added C macros to correct problems with printing graphs
         under Motif
  970917 added C macro FOCUSDELAY which determines wait before XSetInputFocus()
         in Motif version
  981218 added new global DefaultFocusDelay
*/
#ifndef WXIFACEH__
#define WXIFACEH__
#undef EXTERN
#undef COMMA
#define COMMA ,

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

#ifndef PLATFORMH__
#include "../platform.h"
#endif /* PLATFORMH__ */

#include "wxProto.h"

/*
	Globals related to non-interactive mode
	UseWindows indicates the status of non-interactive mode
	If non-zero all calles to print to windows are short circuited
	All graphics will be DUMB
	Value < 0 means during startup sequence
	Value > 0 means use windows
	Value of 0 means don't use windows
*/

EXTERN int UseWindows INIT(0);
EXTERN int SaveOnQuit INIT(TRUE);
EXTERN int SaveOnClose INIT(TRUE);

/*
	Graph windows
*/
#ifndef NGRAPHS
#define NGRAPHS          8
#endif /*NGRAPHS*/

#define NUMCANVASFRAMES  NGRAPHS

EXTERN char        *PanelTitle INIT("Panel of Graphs");

EXTERN int         GraphWindPaused INIT(FALSE);

#ifdef wx_msw
#ifndef CMDWINDOWLIMIT
/*
	threshhold for window to be full
	Experiments in Windows shows failure about 31200; 30000
	may be safe.
	971231 ran into problems with 30000 running test5.in; ok with 29500
*/
#define CMDWINDOWLIMIT   29500 /* 971231 changed from 30000 */
#endif /*CMDWINDOWLIMIT*/

#ifndef CMDWINDOWNEWSIZE
#define CMDWINDOWNEWSIZE 25000  /* target size after trimming */
#endif /*CMDWINDOWNEWSIZE*/

#endif /*wx_msw*/

#ifdef wx_motif
#ifndef CMDWINDOWLIMIT
#define CMDWINDOWLIMIT   85000  /* threshhold for window to be full */
#endif /*CMDWINDOWLIMIT*/

#ifndef CMDWINDOWNEWSIZE
#define CMDWINDOWNEWSIZE 80000  /* target size after trimming */
#endif /*CMDWINDOWNEWSIZE*/
#endif /*wx_motif*/

#define UNDOBUFFERINITIALALLOCATION   2000  /* beginning size for UndoBuffer, can be increased */

#ifndef MAXWINDOWS
#define MAXWINDOWS 8
#endif /*MAXWINDOWS*/

#define NUMTEXTFRAMES   MAXWINDOWS

#ifndef DEFAULTTEXTWINDOWWIDTH
#ifdef WXWINMOTIF
#define DEFAULTTEXTWINDOWWIDTH  920  /* 80 columns default font */
#define DEFAULTTEXTWINDOWHEIGHT 650
#else /*WXWINMOTIF*/
#define DEFAULTTEXTWINDOWWIDTH  680  /* 80 columns default font */
#define DEFAULTTEXTWINDOWHEIGHT 500  /* 25 lines default font */
#endif /*WXWINMOTIF*/
#endif /*DEFAULTTEXTWINDOWWIDTH*/

#ifndef DEFAULTCANVASWINDOWWIDTH
#ifdef WXWINMOTIF
#define DEFAULTCANVASWINDOWWIDTH  920
#define DEFAULTCANVASWINDOWHEIGHT 650
#else /*WXWINMOTIF*/
#define DEFAULTCANVASWINDOWWIDTH  680
#define DEFAULTCANVASWINDOWHEIGHT 500
#endif /*WXWINMOTIF*/
#endif /*DEFAULTCANVASWINDOWWIDTH*/

#ifndef DEFAULTTEXTFONTSIZE
#ifdef wx_motif
#define DEFAULTTEXTFONTSIZE 12
#else // wx_motif
#define DEFAULTTEXTFONTSIZE 12
#endif //wx_motif
#endif /*DEFAULTTEXTFONTSIZE*/

#ifndef DEFAULTCANVASFONTSIZE
#ifdef wx_motif
#define DEFAULTCANVASFONTSIZE 11
#else // wx_motif
#define DEFAULTCANVASFONTSIZE 12
#endif //wx_motif
#endif /*DEFAULTCANVASFONTSIZE*/

#ifndef DEFAULTPRINTERFONTSIZE
#ifdef wx_motif
#define DEFAULTPRINTERFONTSIZE 11
#else // wx_motif
#define DEFAULTPRINTERFONTSIZE 11
#endif //wx_motif
#endif /*DEFAULTPRINTERFONTSIZE*/

#ifdef wx_motif
#ifndef GRAPHLANDSCAPESCALING
#define GRAPHLANDSCAPESCALING         .8
#define GRAPHLANDSCAPETRANSLATIONX  20.0
#define GRAPHLANDSCAPETRANSLATIONY -40.0
#define GRAPHPORTRAITSCALING         .55
#define GRAPHPORTRAITTRANSLATIONX   60.0
#define GRAPHPORTRAITTRANSLATIONY  200.0
#endif /*GRAPHLANDSCAPESCALING*/
#ifndef TEXTLANDSCAPESCALING 
#define TEXTLANDSCAPESCALING         1.0
#define TEXTLANDSCAPETRANSLATIONX    0.0
#define TEXTLANDSCAPETRANSLATIONY  -32.0
#define TEXTPORTRAITSCALING          1.0
#define TEXTPORTRAITTRANSLATIONX     0.0
#define TEXTPORTRAITTRANSLATIONY     0.0
#endif /*TEXTLANDSCAPESCALING*/
#endif /*wx_motif*/

EXTERN int DefaultTextWindowWidth    INIT(DEFAULTTEXTWINDOWWIDTH);
EXTERN int DefaultTextWindowHeight   INIT(DEFAULTTEXTWINDOWHEIGHT);
EXTERN int DefaultTextFontSize       INIT(DEFAULTTEXTFONTSIZE);

EXTERN int DefaultCanvasWindowWidth  INIT(DEFAULTCANVASWINDOWWIDTH);
EXTERN int DefaultCanvasWindowHeight INIT(DEFAULTCANVASWINDOWHEIGHT);
EXTERN int DefaultCanvasFontSize     INIT(DEFAULTCANVASFONTSIZE);

EXTERN int DefaultPrinterFontSize    INIT(DEFAULTPRINTERFONTSIZE);

#ifndef MACANOVATIMERINCREMENT
#define MACANOVATIMERINCREMENT 200
#endif /*MACANOVATIMERINCREMENT*/

#ifdef wx_motif
#ifndef FOCUSDELAY
#define FOCUSDELAY   800 /*defines delay before XSetInputFocus()*/
#endif /*FOCUSDELAY*/
#else /*wx_motif*/
#define FOCUSDELAY     0
#endif /*FOCUSDELAY*/

EXTERN long DefaultFocusDelay INIT(FOCUSDELAY);
EXTERN long FocusDelay        INIT(FOCUSDELAY);

#define NAHEAD       100   /* number of typeahead characters */

enum DoQuitConsts
{
	CANCANCEL  =  0,     /* can try to cancel DoQuit */
	MUSTQUIT             /* cannot cancel DoQuit */
};

/* Referenced in CmdWindMain and LineToWindow in macIo.c */
EXTERN char         *TooManyErrors  INIT("ERROR: too many errors; execution terminated");


EXTERN int         Running INIT(0); /* !0 means under control of yyparse */

/*
	Globals and defines related to undoing and redoing
*/

/* possible values for UndoStatus */
#define DELETEUNDO    1
#define OUTPUTUNDO    2
#define PASTEUNDO     3
#define CUTUNDO       4
#define TYPINGUNDO    5
#define CANTUNDO      0
#define DELETEREDO   -1 /* not currently used */
#define OUTPUTREDO   -2
#define PASTEREDO    -3 /* not currently used */
#define CUTREDO      -4 /* not currently used */
#define TYPINGREDO   -5

/*
	When UndoStatus == OUTPUTUNDO, LastLength is the length of the buffer,
	less 1 for the trailing newline, just before a command line is executed,
	and LastCmdStrPos is the value of CmdStrPos at that time.  
*/
#undef UNDEFINED__
#if (0)
EXTERN long         LastLength INIT(-1);
EXTERN long         LastCmdStrPos INIT(-1);
EXTERN long         UndoPlace INIT(-1); /* start of change */
EXTERN long         PasteLength INIT(-1); /* length of paste */

/* OldCmdStrPos is CmdStrPos at time of a deletion, replacement, cut, or paste*/
EXTERN long         OldCmdStrPos INIT(-1);
EXTERN long         LastTypingPos INIT(-1);
#endif /*0*/

/*
****defines and enum declarations only after this point****
*/

/*
	The following taken from generic.c in Win32 Programming API Bible
*/
#if defined (wx_msw)
#define IS_WIN32 1
#else /*WIN32*/
#define IS_WIN32 0
#endif /*WIN32*/

#define IS_NT     IS_WIN32 && (int)(GetVersion() < 0x80000000)
#define IS_WIN32S IS_WIN32 && \
	 (int)(!(IS_NT) && (LOBYTE(LOWORD(GetVersion()))<4))
#define IS_WIN95  (int)(!(IS_NT) && !(IS_WIN32S)) && IS_WIN32

#undef LF
#undef CR
#define LF            10
#define CR            13

#ifdef wx_msw
#define EOL_LENGTH 2  /* you write \n but get \r\n */
#define EOL_STRING "\n"
#else // wx_msw
#define EOL_LENGTH 1
#define EOL_STRING "\n"
#endif // wx_msw


/*
	Information related to windows
*/

/* offsets for view rectangle in command windows */
#define TOPOFFSET      2
#define LEFTOFFSET     4
#define BOTTOMOFFSET   2
#define RIGHTOFFSET   19

/* offset of top of window is windno * STACKOFFSET */
#ifdef wx_msw
#define STACKOFFSET   22
#else /*wx_msw*/
#define STACKOFFSET   10
#endif /*wx_msw*/
/*
	information related to menus
*/

#undef	INIT
#undef	INITDIM
#undef	INITARRAY
#undef  COMMA
#undef  EXTERN

#define WXMAXPATHLENGTH 1024

#endif /*WXIFACEH__*/
