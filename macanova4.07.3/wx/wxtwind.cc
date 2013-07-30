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
  970912 made changes implementing various keyboard translation
  980401 replaced call to XtRemoveCallbacks() with call to
         XtRemoveAllCallbacks() so that unmodified WX code can be used
         Previous code can be activated by defining USEXtRemoveCallbacks
  980406 In motif version added calls to Refresh() to redraw window whenever
         the screen is being changed with a nontrivial selection;  there
         was a problem with failure to update when the selection ended
         off the visible part of the window.
  981110 Changed MacAnovaTextWindow:OnChar() to correctly process delete
         key in Windows 95 (it worked in Windows 3.1);  this involves doing
         something different depending on system
  981112 Since the Windows 95 delete processing code seems to work in
         Windows 3.1, it is now used regardless of system.
  990215 replaced myerrorout() by putOutErrorMsg() and putOUTSTR() by
         putErrorOUTSTR()
*/
#ifdef __GNUG__
#pragma implementation
#endif

// For compilers that support precompilation, includes "wx.h".
#include "wx_prec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx.h"
#endif

#include <math.h>
#ifdef wx_motif
#include <Xm/Text.h>
#include <Xm/Xm.h>
#include <Xm/CutPaste.h>
#include <X11/keysym.h>
#endif  // wx_motif

extern "C" {
#include "../globals.h"
#include "wxIface.h"
} /*extern "C" */


#ifdef wx_motif
#ifdef HASCLIPBOARD
#include "wx_clipb.h"
#endif /*HASCLIPBOARD*/
#endif  // wx_motif

#ifdef wx_msw
#include "wx_clipb.h"
#endif //wx_msw

#include <ctype.h>
#include "wx_timer.h"
#include "wx_mf.h"
#include "wxmain.h"

#if defined(wx_msw) && !defined(__WATCOMC__)
#include "mmsystem.h"
#endif /*wx_msw && !__WATCOMC__*/

#ifdef wx_x
#include "aiai.xbm"
#include "fload.xbm"
#endif /*wx_x*/


#ifdef wx_msw
#include <sys/types.h>
#include <sys/stat.h>
/*
  Code  adapted from src/msw/wx_text.cc
  970127 modified to enforce limit on amount of text loaded into window
         and to check for buffer overrun
  980306 further modified to match wx168 and change malloc to new
*/
#define HEDGE 2000     //extra space in buffer

Bool MacAnovaTextWindow::LoadFile(char *file)
{
	if (!file || !FileExists(file))
    {
		return FALSE;
    }

	if (file_name)
    {
		delete[] file_name;
	}
	file_name = copystring(file);

	Clear();

// ITA change - ios::nocreate is not defined in MSL, or ANSI draft standard
// for that matter
#if !defined(__MWERKS__)
	ifstream input( file, ios::nocreate | ios::in);
#else
	ifstream input;
	input.open( file, ios::in);
#endif

	if (!input.bad())
	{
		// Previously a SETSEL/REPLACESEL call-pair were done to insert
		// line by line into the control. Apart from being very slow this
		// was limited to 32K of text by the external interface presenting
		// positions as signed shorts. Now load in one chunk...
		// Note use of 'farmalloc' as in Borland 3.1 'size_t' is 16-bits...

		struct stat stat_buf;
		if (stat(file, &stat_buf) < 0)
        {
        	return FALSE;
        }
      // This may need to be a bigger buffer than the file size suggests,
      // if it's a UNIX file. Give it an extra 2000 just in case.
		long      buffersize = stat_buf.st_size + 1 + HEDGE;
#if (0)
		char     *tmp_buffer = (char*)farmalloc((size_t)(buffersize));
#else
		char     *tmp_buffer = new char[buffersize];
#endif /*0*/
		long      no_lines = 0;
		long      pos = 0;
         //following are special to MacAnova
        long      limit = CMDWINDOWLIMIT - 1000;
        long      truncated = 0;

		while (!input.eof() && input.peek() != EOF)
		{
			input.getline(wxBuffer, 500);
			int      len = strlen(wxBuffer);
            // 970127 check for buffer overrun (special to MacAnova)
            if (pos + len + 2 > buffersize)
            {
            	sprintf(OUTSTR,
            			"WARNING: more than %d lines in file; truncated",
                        HEDGE);
            	myAlert(OUTSTR);
            	*OUTSTR = '\0';
            	break;
            }
			wxBuffer[len] = 13; //CR
			wxBuffer[len+1] = 10; //LF
			wxBuffer[len+2] = 0;
			strcpy(tmp_buffer+pos, wxBuffer);
			pos += strlen(wxBuffer);
			no_lines++;
		} /*while (!input.eof() && input.peek() != EOF)*/

		// special to Macanova
        if (pos > limit)
		{
            truncated = pos - limit;
            if (tmp_buffer[truncated] == '\r')
            {
            	truncated++;
            }
            if (tmp_buffer[truncated] == '\n')
        	{
            	truncated++;
            }
            sprintf(OUTSTR,
            		"WARNING: file too long; only last %ld characters loaded",
                    pos - truncated);
            myAlert(OUTSTR);
			putErrorOUTSTR();
        } /*if (pos > limit)*/
				
#if (0)
		SendMessage((HWND)ms_handle, WM_SETTEXT, 0,
        			(LPARAM) (tmp_buffer + truncated));
#endif /*0*/
		SetWindowText((HWND)ms_handle, tmp_buffer + truncated);
		SendMessage((HWND)ms_handle, EM_SETMODIFY, FALSE, 0L);
#if (0)
		farfree(tmp_buffer);
#else
		delete [] tmp_buffer;
#endif /*0*/
		return TRUE;
	}
	return FALSE;
} /*MacAnovaTextWindow::LoadFile*/
#else
Bool MacAnovaTextWindow::LoadFile(char *file)
{
	return (wxTextWindow::LoadFile(file));
}
#endif /*wx_msw*/

#ifdef wx_msw
Bool MacAnovaTextWindow::SaveFile(char *file)
{
	if (!file)
    {
    	file = file_name;
    }
  	if (!file)
    {
    	return FALSE;
    }
	if (file_name)
    {
    	delete[] file_name;
    }
	file_name = copystring(file);

	ofstream output(file);

    // This will only save 64K max
    unsigned long nbytes = SendMessage(ms_handle, WM_GETTEXTLENGTH, 0, 0);
#if (0)
    char *tmp_buffer = (char*)farmalloc((size_t)(nbytes+1));
#else
	char *tmp_buffer = new char[nbytes+1];
#endif /*0*/
    SendMessage(ms_handle, WM_GETTEXT, (WPARAM)(nbytes+1), (LPARAM)tmp_buffer);
    tmp_buffer[nbytes] = '\0';
    char *pstr = tmp_buffer;

    if (!output.bad())
    {
		// Convert \r\n to just \n
		while (*pstr)
		{
			if (*pstr != '\r')
            {
				output << *pstr;
            }
			pstr++;
		}
    }

#if (0)
    farfree(tmp_buffer);
#else
    delete [] tmp_buffer;
#endif /*0*/
    SendMessage((HWND)ms_handle, EM_SETMODIFY, FALSE, 0L);

    return TRUE;
} /*MacAnovaTextWindow::SaveFile()*/
#else
Bool MacAnovaTextWindow::SaveFile(char *file)
{
	return (wxTextWindow::SaveFile(file));
}
#endif /*wx_msw*/

extern "C" {
#ifdef wx_motif

void MacAnovaTextWindowModifyProc (Widget w, XtPointer clientData,
							  XmTextVerifyCallbackStruct *cbs)
{
	long            startPos, endPos, textLength;
	long            displacedLength, change;
	WHERE("MacAnovaTextWindowModifyProc");

#if (0)
	if (!wxWidgetHashTable->Get ((long) w))
	{
		return;
	}
#endif /*0*/
	wxTextWindow           *tw = (wxTextWindow *) clientData;
	MacAnovaTextWindow     *mtw = (MacAnovaTextWindow *) clientData;
	
	// If we're already within an OnChar, return: probably
	// a programmatic insertion.
	if (tw->tempCallbackStruct)
	{
		return;
	}
	
	if (mtw->ControlActionStatus)
	{   // we got here from a call that we made in code,
		// so just let event pass through
		return;
	}
	if (BaseFrame->Running)
	{   // if we did type ahead, beep and swallow the key, otherwise we got
		// here from a call that we made in code, so just let event pass through
		if (cbs->event)
		{
			mybeep(1);
			cbs->doit = 0;
		}
		return;
	} /*if (BaseFrame->Running)*/
	
	startPos = cbs->startPos;
	endPos = cbs->endPos;
	textLength = cbs->text->length;
	
	if (!cbs->event)
	{ // no event, so we probably got here with a BTransfer (middle button)
		// fix up the positions here, rather than in Paste, since no corresponding
		// possibilities in MSW
		displacedLength = endPos - startPos;
		change = textLength - displacedLength;
		// need to worry about overfilling a window here

		mtw->clearUndoInfo();
		mtw->OldCmdStrPos = mtw->CmdStrPos;
		mtw->PasteLength = textLength;
		mtw->UndoStatus = PASTEUNDO;
		if (displacedLength > 0)
        {
			mtw->ToUndoBuffer();		// save what we're pasting over
		}
		/* only reset CmdStrPos if selection before or including CmdStrPos */
		if (startPos < mtw->CmdStrPos)
        {
			if (endPos <= mtw->CmdStrPos)
			{					/* entire selection <= mtw->CmdStrPos */
				mtw->CmdStrPos += change;
			}
			else
			{/* startPos < mtw->CmdStrPos && endPos > mtw->CmdStrPos */
				mtw->CmdStrPos = startPos;
			}
		} /*if (startPos < mtw->CmdStrPos)*/
		mtw->UndoPlace = startPos;
		// The following doesn't seem to work 
		mtw->SetSelection(startPos+change,startPos+change);
		mtw->SetInsertionPoint(startPos+change);
		((MacAnovaTextFrame *) (mtw->GetParent()))->ResetEditMenu(mtw->UndoStatus);
		return;
	} /*if (!cbs->event)*/
	else
	{
		long       timestamp;
		
		switch (cbs->event->type)
		{
		  case KeyPress:
			timestamp = cbs->event->xkey.time;
			break;
			
		  case ButtonPress:
			timestamp = cbs->event->xbutton.time;
			break;
			
		  default:
			timestamp = mtw->timeStamp;
			break;
		} /*switch (cbs->event->type)*/
		mtw->timeStamp = timestamp;
	} /*if (!cbs->event){}else{}*/
	
	// Check for a backspace
	if (startPos == (cbs->currInsert - 1))
	{
		tw->tempCallbackStruct = (XtPointer)cbs;

		wxKeyEvent event (wxEVENT_TYPE_CHAR);
		event.keyCode = WXK_DELETE;
		event.eventObject = tw;

		// Only if wxTextWindow::OnChar is called
		// will this be set to True (and the character
		// passed through)
		cbs->doit = False;
		
		tw->GetEventHandler()->OnChar(event);
		
		tw->tempCallbackStruct = NULL;

		return;
	} /*if (startPos == (cbs->currInsert - 1))*/

	// Pasting operation: let it through without calling OnChar
	if (textLength > 1)
	{
		return;
	}

	// Something other than text
	if (cbs->text->ptr == NULL)
	{
		return;
	}

	tw->tempCallbackStruct = (XtPointer)cbs;

	wxKeyEvent event (wxEVENT_TYPE_CHAR);

	event.keyCode = (cbs->text->ptr[0] == 10 ? 13 : cbs->text->ptr[0]);
	event.eventObject = tw;

	/*
	  Only if wxTextWindow::OnChar() is called will this be set to True (and
	  the character passed through)
	  */
	cbs->doit = False;

	tw->GetEventHandler()->OnChar(event);

	tw->tempCallbackStruct = NULL;

} /*MacAnovaTextWindowModifyProc()*/
#endif /*wx_motif*/

/*
  970912 implemented the following changes for Motif version
   Ctrl I  (but not TAB)       Interrupt
   Ctrl Alt Q                  Fast quit
   Alt Keypad Enter            Execute
   Ctrl Enter                  Execute
   Shift Enter                 Execute
   Alt Keypad Up Arrow         Up History
   Ctrl Keypad Up Arrow        Up History
   Alt Keypad Down Arrow       Down History
   Ctrl Keypad Down Arrow      Down History

  971017 implemented additional changes for Motif version to mimic Macintosh
   Ctrl /                      Copy To End
   Ctrl \                      Execute

  970915 implemented the following changes for Windows version
   Ctrl I  (but not TAB)       Interrupt
   Ctrl Shift Q                Fast quit
   Shift Enter                 Execute
   Ctrl Enter                  Execute
   Ctrl Up Arrow               Up History
   Ctrl Keypad Up Arrow        Up History
   Ctrl Down Arrow             Down History
   Ctrl Keypad Down Arrow      Down History
*/

extern wxHashTable *wxWidgetHashTable;

#ifdef wx_motif
void DoKeyInterrupt (Widget w, XEvent *event, String *params,
					 Cardinal *num_params)
{
	BaseFrame->KeyInterrupt();
}
#endif /*wx_motif*/

#ifdef wx_motif
static void DoExecute (Widget w, XEvent *event, String *params,
					   Cardinal *num_params)
{
	MacAnovaTextWindow *mtw;
	MacAnovaTextFrame     *parent;

	mtw = (MacAnovaTextWindow *) wxWidgetHashTable->Get((long)w);
	parent = (MacAnovaTextFrame *) mtw->GetParent();
	parent->OnMenuCommand(MACANOVA_EDIT_EXECUTE);
} /*DoExecute()*/

static void DoCopyToEnd (Widget w, XEvent *event, String *params,
						 Cardinal *num_params)
{
	MacAnovaTextWindow *mtw;
	MacAnovaTextFrame     *parent;

	mtw = (MacAnovaTextWindow *) wxWidgetHashTable->Get((long)w);
	parent = (MacAnovaTextFrame *) mtw->GetParent();
	parent->OnMenuCommand(MACANOVA_EDIT_COPYTOEND);
} /*DoCopyToEnd()*/
#endif /*wx_motif*/

#ifdef wx_motif
void DoFastQuit (Widget w, XEvent *event, String *params,
					Cardinal *num_params)
{
	SaveOnQuit = FALSE;
	BaseFrame->DoQuit(CANCANCEL);
} /*DoFastQuit()*/

static void DoFastClose (Widget w, XEvent *event, String *params,
					Cardinal *num_params)
{
	MacAnovaTextWindow    *mtw;
	MacAnovaTextFrame     *parent;

	mtw = (MacAnovaTextWindow *) wxWidgetHashTable->Get((long)w);
	parent = (MacAnovaTextFrame *) mtw->GetParent();
	SaveOnClose = FALSE;
	parent->OnClose();
} /*DoFastClose()*/

#endif /*wx_motif*/

#ifdef wx_motif
static void DoHistoryUp (Widget w, XEvent *event, String *params,
					Cardinal *num_params)
{
	MacAnovaTextWindow *mtw;

	mtw = (MacAnovaTextWindow *) wxWidgetHashTable->Get((long)w);
	mtw->UpDownHistory(MACANOVA_EDIT_UPHISTORY);

}
#endif /*wx_motif*/

#ifdef wx_motif
static void DoHistoryDown (Widget w, XEvent *event, String *params,
					Cardinal *num_params)
{
	MacAnovaTextWindow *mtw;

	mtw = (MacAnovaTextWindow *) wxWidgetHashTable->Get((long)w);
	mtw->UpDownHistory(MACANOVA_EDIT_DOWNHISTORY);

} /*DoHistoryDown()*/
#endif /*wx_motif*/

#ifdef wx_motif
void DoFindGraph (Widget w, XEvent *event, String *params,
				  Cardinal *num_params)
{
	MacAnovaTextWindow *mtw;
	XID                 keysym;
	Modifiers           modReturn;
	int                 windno = 0;
	WHERE("DoFindGraph");
	
	XtTranslateKey(event->xkey.display,event->xkey.keycode, (Modifiers) 0,
				   & modReturn, &keysym);
	switch (keysym)
	{
	  case XK_1:
	  case XK_F1:
		windno = 1;
		break;

	  case XK_2:
	  case XK_F2:
		windno = 2;
		break;

	  case XK_3:
	  case XK_F3:
		windno = 3;
		break;
		
	  case XK_4:
	  case XK_F4:
		windno = 4;
		break;

	  case XK_5:
	  case XK_F5:
		windno = 5;
		break;
		
	  case XK_6:
	  case XK_F6:
		windno = 6;
		break;
		
	  case XK_7:
	  case XK_F7:
		windno = 7;
		break;
		
	  case XK_8:
	  case XK_F8:
		windno = 8;
		break;
	} /*switch (keysym)*/

	BaseFrame->FindGraphFrame(windno-1);

} /*DoFindGraph()*/
#endif /*wx_motif*/

} /*extern "C" */

#ifdef wx_motif
/*
  When USEXtRemoveCallbacks is defined, the code is a little safer but 
  a change in the WX source to remove the static declaration for
  wxTextWindowModifyProc is required.  If it is not defined,
  the WX source can be used as is.
*/
#ifdef USEXtRemoveCallbacks
extern void
wxTextWindowModifyProc (Widget w, XtPointer clientData,
						XmTextVerifyCallbackStruct *cbs);

#endif //(USEXtRemoveCallbacks)
#endif // wx_motif

MacAnovaTextWindow::MacAnovaTextWindow(wxFrame *frame, int x=-1, int y=-1,
	int width=-1, int height=-1, long style=0):
	wxTextWindow(frame, x, y, width, height, style)
{

	Nahead = 0;
	UndoBuffer = new char[UNDOBUFFERINITIALALLOCATION];
	UndoBufferAllocation = UNDOBUFFERINITIALALLOCATION;
	clearUndoInfo();
	LastTypingPos = 0;

#ifdef wx_motif
	Widget          textWidget = (Widget) handle;
	timeStamp = 0;

	static char MacAnovaTranslationTable[] =
	  "<Key>KP_Enter: DoExecute()\n\
Shift <Key>Return: DoExecute()\n\
Ctrl <Key>Return: DoExecute()\n\
Ctrl <Key>\\\\: DoExecute()\n\
Ctrl <Key>/: DoCopyToEnd()\n\
Mod1 <Key>KP_8: DoHistoryUp()\n\
Mod1 <Key>KP_2: DoHistoryDown()\n\
Ctrl <Key>KP_8: DoHistoryUp()\n\
Ctrl <Key>KP_2: DoHistoryDown()\n\
Ctrl Mod1 <Key>q: DoFastQuit()\n\
Ctrl Mod1 <Key>w: DoFastClose()\n\
Ctrl <Key>1: DoFindGraph()\n\
Ctrl <Key>2: DoFindGraph()\n\
Ctrl <Key>3: DoFindGraph()\n\
Ctrl <Key>4: DoFindGraph()\n\
Ctrl <Key>5: DoFindGraph()\n\
Ctrl <Key>6: DoFindGraph()\n\
Ctrl <Key>7: DoFindGraph()\n\
Ctrl <Key>8: DoFindGraph()\n\
Ctrl <Key>F1: DoFindGraph()\n\
Ctrl <Key>F2: DoFindGraph()\n\
Ctrl <Key>F3: DoFindGraph()\n\
Ctrl <Key>F4: DoFindGraph()\n\
Ctrl <Key>F5: DoFindGraph()\n\
Ctrl <Key>F6: DoFindGraph()\n\
Ctrl <Key>F7: DoFindGraph()\n\
Ctrl <Key>F8: DoFindGraph()\n\
Ctrl <Key>i: DoKeyInterrupt()";

	static XtActionsRec actions[] =
	{
		{"DoExecute", (XtActionProc) DoExecute},
		{"DoCopyToEnd", (XtActionProc) DoCopyToEnd},
		{"DoKeyInterrupt", (XtActionProc) DoKeyInterrupt},
		{"DoFastQuit", (XtActionProc) DoFastQuit},
		{"DoFastClose", (XtActionProc) DoFastClose},
		{"DoHistoryUp", (XtActionProc) DoHistoryUp},
		{"DoHistoryDown", (XtActionProc) DoHistoryDown},
		{"DoFindGraph", (XtActionProc) DoFindGraph}
	};
	XtTranslations MacAnovaTranslations;

	XtAppAddActions (wxTheApp->appContext, actions,
					 sizeof(actions)/sizeof(XtActionsRec));
	MacAnovaTranslations = XtParseTranslationTable(MacAnovaTranslationTable);
	XtOverrideTranslations(textWidget, MacAnovaTranslations);
	XtFree ((char *) MacAnovaTranslations);

#ifdef USEXtRemoveCallbacks
	XtRemoveCallback(textWidget, XmNmodifyVerifyCallback,
					 (XtCallbackProc)wxTextWindowModifyProc,
					 (XtPointer) this);
#else /*USEXtRemoveCallbacks*/
	/* this is a "dangerous" new attempt to make things work without
	   having to modify the wxWin source code.  The old version was
	   safe, but required that wxTextWindowModifyProc be visible,
	   when wxWin defines it as static by default.
   	*/
	XtRemoveAllCallbacks(textWidget, XmNmodifyVerifyCallback);
#endif /*USEXtRemoveCallbacks*/

	XtAddCallback(textWidget, XmNmodifyVerifyCallback,
				  (XtCallbackProc)MacAnovaTextWindowModifyProc,
				  (XtPointer) this);

	// suppress bell sounding on un-processed character
	Arg     args[1];
	XtSetArg(args[0], XmNverifyBell, FALSE);
	XtSetValues(textWidget, args, 1);

#endif // wx_motif
	ControlActionStatus = 0;
} /*MacAnovaTextWindow::MacAnovaTextWindow()*/

MacAnovaTextWindow::~MacAnovaTextWindow(void)
{
	delete [] UndoBuffer;
}


void MacAnovaTextWindow::Undo(void)
{
	long       scrapLength,cmdStrPos,selStart,selEnd,location,length,change;
	WHERE("MacAnovaTextWindow::Undo");
	
	ControlActionStatus = 1;
#if (0)
	(void) wxMessageBox("starting twindow::undo", "About MacAnova 4.0",
					   wxOK|wxCENTRE);
	sprintf(dump,"UndoStatus %ld ",UndoStatus);
	(void) wxMessageBox(dump, "About MacAnova 4.0", wxOK|wxCENTRE);
#endif /*0*/
	switch (UndoStatus)
	{
	  case PASTEUNDO:
		/* undo result of previous paste*/
		SetSelection(UndoPlace, UndoPlace+PasteLength);
#if (0)
		sprintf(dump,"UndoPlace %ld PasteLength %ld",UndoPlace,PasteLength);
		(void) wxMessageBox(dump, "About MacAnova 4.0", wxOK|wxCENTRE);
#endif /*0*/
		wxTextWindow::Cut();
#if (0)
		(void) wxMessageBox("did it cut out the paste?", "About MacAnova 4.0",
						   wxOK|wxCENTRE);
#endif /*0*/
		/* restore what it replaced */
		scrapLength = FromUndoBuffer();
#if (0)
		(void) wxMessageBox("is it ok now?", "About MacAnova 4.0",
							wxOK|wxCENTRE);
#endif /*0*/
		break;
		
	  case DELETEUNDO:
	  case DELETEREDO:
		/* Must have been deletion or replacement that enabled undo */
		location = UndoPlace;
		length = LastTypingPos - UndoPlace;
		cmdStrPos = CmdStrPos;
		scrapLength = FromUndoBuffer(); /* restore what was deleted*/

		GetSelectionPosition(&selStart,&selEnd);
		if (selStart < cmdStrPos)
		{
			CmdStrPos = cmdStrPos + scrapLength - length;
		}
		if (LastChar != WXK_BACK)
		{/* was replacement of selection by typing, so remove it */
			SetSelection(selEnd, selEnd+length);
			ToUndoBuffer();
			UndoPlace = location;
			LastTypingPos = location + scrapLength;
			wxTextWindow::Cut();
			SetSelection(selStart, selEnd);
			UndoStatus = (UndoStatus == DELETEREDO) ? DELETEUNDO : DELETEREDO;
		} /*if (LastChar != WXK_BACK)*/
		break;
		
	  case CUTUNDO:
		/* undo previous cut */
		location = UndoPlace;
		change = strlen(UndoBuffer);
#if (0)
		sprintf(dump,"UndoPlace %ld scraplen %ld",UndoPlace,change);
		(void) wxMessageBox(dump, "About MacAnova 4.0", wxOK|wxCENTRE);
#endif /*0*/
		scrapLength = FromUndoBuffer();
#if (0) /*CmdStrPos already restored by FromUndoBuffer*/
		if (location < CmdStrPos)
		{
			CmdStrPos += change;
		}
#endif /*0*/
		clearUndoInfo();			/* can't undo*/
		SetSelection(location, location + change);
		break;
		
	  case OUTPUTUNDO:
		/*
		   Undo output from most recent command.  LastLength is length,
		   including final newline, just before DoStats() and LastCmdStrPos
		   is the value of CmdStrPos at that time.  We cut all the output,
		   plus the final newline.
		   */
		SetSelection(LastLength-EOL_LENGTH, GetLastPosition());
		ToUndoBuffer(); /* save stuff on local scrap */
		Remove(LastLength-EOL_LENGTH, GetLastPosition());
		CmdStrPos = LastCmdStrPos;
		LastLength = LastCmdStrPos = -1;
		LastChar = WXK_BACK;
		UndoStatus = OUTPUTREDO;	/* can redo */
#if (0)
		CmdInsertLine = GetCmdIL();
#endif /*0*/
		break;
		
	  case OUTPUTREDO:
		/* redo output just undone */
		length = GetLastPosition();
		location = CmdStrPos;
		scrapLength = FromUndoBuffer(); /*get previous output from local scrap*/
		SetSelection(GetLastPosition(), GetLastPosition());
		CmdStrPos = GetLastPosition();
		clearUndoInfo();
		LastCmdStrPos = location;
		LastLength = length + EOL_LENGTH;
		UndoStatus = OUTPUTUNDO;
#if (0)
		CmdInsertLine = GetCmdIL();
#endif /*0*/
		break;
		
	  case TYPINGUNDO:
		if (LastTypingPos > UndoPlace)
		{
			location = UndoPlace;
			length = LastTypingPos - location;
			UndoStatus = TYPINGREDO;
			OldCmdStrPos = CmdStrPos;
			SetSelection(UndoPlace, LastTypingPos);
			ToUndoBuffer();
			if (location < CmdStrPos)
			{
				CmdStrPos -= length;
			}
			wxTextWindow::Cut();
			SetSelection(location, location);
		}
		break;
		
	  case TYPINGREDO:
		selStart = UndoPlace;
		SetSelection(selStart,selStart);
		length = FromUndoBuffer();
		UndoPlace = selStart;
		LastTypingPos = UndoPlace + length;
		UndoStatus = TYPINGUNDO;
	} /*switch (UndoStatus)*/
	ControlActionStatus = 0;
#ifdef wx_motif
	Refresh();
#endif /*wx_motif*/
} /*MacAnovaTextWindow::Undo()*/

void MacAnovaTextWindow::Cut(void)
{
	long           selStart,selEnd;
	
#if (0)
	(void) wxMessageBox("greetings from MacAnovaTextWindow::Cut",
						"About MacAnova 4.0", wxOK|wxCENTRE);
#endif /*0*/
	GetSelectionPosition(&selStart,&selEnd);
	if (selStart != selEnd)
 	{
		ControlActionStatus = 1;
		clearUndoInfo();
		OldCmdStrPos = CmdStrPos;
		UndoPlace = selStart;
#if (0)
		sprintf(dump,"CSP %ld UP %ld sstrt %ld send %ld", CmdStrPos,
				UndoPlace, selStart, selEnd);
		(void) wxMessageBox(dump, "About MacAnova 4.0", wxOK|wxCENTRE);
#endif /*0*/
		if (UndoPlace < CmdStrPos)
		{/* only need change if before command line */
			if ( selEnd <= CmdStrPos)
			{
				CmdStrPos -= selEnd - selStart;
			}
			else
			{
				CmdStrPos = selStart;
			}
		} /* if (UndoPlace < CmdStrPos) */
#if (0)
		sprintf(dump,"CSP %ld UP %ld sstrt %ld send %ld", CmdStrPos, UndoPlace,
				selStart, selEnd);
		(void) wxMessageBox(dump, "About MacAnova 4.0", wxOK|wxCENTRE);
#endif /*0*/
		ToUndoBuffer();
		wxTextWindow::Cut();
#if (0)
		sprintf(dump,"CSP %ld UP %ld sstrt %ld send %ld", CmdStrPos, UndoPlace,
				selStart, selEnd);
		(void) wxMessageBox(dump, "About MacAnova 4.0", wxOK|wxCENTRE);
#endif /*0*/
		CmdInsertLine = GetInsertionLine();
		UndoStatus = CUTUNDO;
		LastChar = WXK_BACK;
		ControlActionStatus = 0;
#ifdef wx_motif
		Refresh();
#endif /*wx_motif*/
	} /*if (selStart != selEnd)*/
} /*MacAnovaTextWindow::Cut()*/

void MacAnovaTextWindow::Copy(void)
{
	long            selStart,selEnd;
	
	GetSelectionPosition(&selStart,&selEnd);
	if (selStart != selEnd)
 	{
		CmdInsertLine = GetInsertionLine();
	}
	if (UndoStatus == CUTUNDO)
	{
		clearUndoInfo();
	}
	wxTextWindow::Copy();
} /*MacAnovaTextWindow::Copy()*/

int MacAnovaTextWindow::CheckForError(void)
{
#ifdef wx_msw
	return(0);
#else //wx_msw
	return(0);
#endif  //wx_msw
}

void MacAnovaTextWindow::Paste(void)
{
	long         selStart,selEnd,clipLength;
	long         location, displacedLength, change;
	WHERE("MacAnovaTextWindow::Paste");

	GetSelectionPosition(&selStart,&selEnd);
	if (selStart == selEnd)
	{
		selStart = selEnd = GetInsertionPoint();
	}
	
	clipLength=GetClipLength();
	if (clipLength > 0)
	{
		ControlActionStatus = 1;
		location = selStart;
		displacedLength = selEnd - selStart;
		change = clipLength - displacedLength;

		// need to worry about overfilling a window here
		clearUndoInfo();
		OldCmdStrPos = CmdStrPos;
		PasteLength = clipLength;
		UndoStatus = PASTEUNDO;
		if (displacedLength > 0)
		{
			ToUndoBuffer();				// save what we're pasting over
		}
		/* only reset CmdStrPos if selection before or including CmdStrPos */
#if (0)
		sprintf(dump,
				"old selstart %ld selend %ld cliplength %ld Cmdstrpos %ld",
				selStart, selEnd,clipLength,CmdStrPos);
		(void) wxMessageBox(dump, "About MacAnova 4.0", wxOK|wxCENTRE);
#endif /*0*/

		if (selStart < CmdStrPos)
		{
			if (selEnd <= CmdStrPos)
			{/* entire selection <= CmdStrPos */
				CmdStrPos += change;
			}
			else
			{/* selStart < CmdStrPos && selEnd > CmdStrPos */
				CmdStrPos = selStart;
			}
		} /*if (selStart < CmdStrPos)*/
#if (0)
		sprintf(dump,
				"new selstart %ld selend %ld cliplength %ld Cmdstrpos %ld",
				selStart, selEnd,clipLength,CmdStrPos);
		(void) wxMessageBox(dump, "About MacAnova 4.0", wxOK|wxCENTRE);
#endif /*0*/
		wxTextWindow::Paste();
		UndoPlace = selStart;
		ControlActionStatus = 0;
#ifdef wx_motif
		Refresh();
#endif /*wx_motif*/
	} /*if (clipLength > 0)*/
} /*MacAnovaTextWindow::Paste()*/

#ifdef wx_msw
/*
  Replacement for Remove() that should not affect the clipboard
  */
void MacAnovaTextWindow::Remove(long from, long to)
{
	Synch();

	HWND hWnd = GetHWND();
	long fromChar = from;
	long toChar = to;
	  
    // Clear all selected text
	SendMessage(hWnd, EM_SETSEL, fromChar, toChar);
	SendMessage(hWnd, WM_CLEAR, (WPARAM)0, (LPARAM)0);
} /*MacAnovaTextWindow::Remove()*/
#endif /*wx_msw*/

void MacAnovaTextWindow::GetSelectionPosition(long *from, long *to)
{
#ifdef wx_motif
	Widget                      textWidget = (Widget) handle;
	XmTextVerifyCallbackStruct *cbs =
		(XmTextVerifyCallbackStruct *) tempCallbackStruct;
	if (cbs)
	{
		*from = cbs->startPos;
		*to = cbs->endPos;
	}
	else
	{
		XmTextGetSelectionPosition (textWidget, (XmTextPosition *) from,
									(XmTextPosition *) to);
	}
#endif // wx_motif

#ifdef wx_xview
#endif // wx_xview

#ifdef wx_msw
	HWND hWnd = GetHWND();
	// this is just for WIN32
	SendMessage(hWnd, EM_GETSEL, (WPARAM) from, (LPARAM) to);
#endif // wx_msw
} /*MacAnovaTextWindow::GetSelectionPosition()*/



long MacAnovaTextWindow::GetClipLength(void)
{
#ifdef wx_motif
	unsigned long         length;
	int                   result;
	Widget                textWidget = (Widget) handle;

	result = XmClipboardInquireLength(XtDisplay(textWidget),
									  XtWindow(textWidget),
									  "STRING",&length);

	return(result == 1 ? length : 0);
#endif //wx_motif
#ifdef wx_xview
#endif //wx_xview
#ifdef wx_msw
	char                 *clipString;
	long                  length;

	wxOpenClipboard();
	clipString = (char *) wxGetClipboardData(wxCF_TEXT);

	if (clipString)
	{
		length=strlen(clipString);
		delete [] clipString;
	}
	else
	{
		length = 0;
	}
	wxCloseClipboard();
	return(length);
#endif //wx_msw
} /*MacAnovaTextWindow::GetClipLength()*/

char * MacAnovaTextWindow::GetSelection(long from = -1, long to = -1)
{
	// return value should be deleted with delete []
	long           i;
	char          *selection;
#if (0)
	char          *xselection;
#endif /*(0)*/
	
	if (from < 0 || to < 0)
	{
		// defaults to get the selection on screen
		GetSelectionPosition ( &from,  &to);
	}
	else
	{// can specify what to get
		i = GetLastPosition();
		from = (from < 0 ? 0 : from);
		from = (from > i ? i : from);
		to = (to < 0 ? 0 : to);
		to = (to > i ? i : to);
	}

	if (from == to)
	{
		return((char *) 0);
	}
	selection = new char[to-from+3];

#ifdef wx_motif
	Widget textWidget = (Widget) handle;
#if (0)
	xselection = XmTextGetSelection(textWidget);
	sprintf(selection,"%s",xselection);
	XtFree(xselection);
#endif /*0*/
	XmTextGetSubstring(textWidget,from,to-from,to-from+3,selection);
#endif //wx_motif
#ifdef wx_xview
#endif //wx_xview
#ifdef wx_msw
	// God help us.  Windows makes us read it in line by line
	long            fromLine, toLine, fromLineChar, toLineChar, fillingLine;
	long            fillingLineChar, fillingLineLength, spaceUsed = 0;
	char            *lineChars;

	PositionToXY(from,&fromLineChar, &fromLine);
	PositionToXY(to,&toLineChar, &toLine);
	for (fillingLine = fromLine; fillingLine <= toLine; fillingLine++)
	{
		fillingLineLength = GetLineLength(fillingLine);
		lineChars = new char[fillingLineLength+4];
		GetLineText(fillingLine,lineChars,fillingLineLength);
		lineChars[fillingLineLength] = '\r';
		lineChars[fillingLineLength+1] = '\n';
		lineChars[fillingLineLength+2] = 0;
		fillingLineLength += 2;
		fillingLineChar = (fillingLine == fromLine ? fromLineChar : 0);
		while (fillingLineChar < fillingLineLength && spaceUsed < to-from)
		{
			selection[spaceUsed++] = lineChars[fillingLineChar++];
		}
		delete [] lineChars;
	} /*for (fillingLine = fromLine; fillingLine <= toLine; fillingLine++)*/

	selection[to-from] = 0;
#endif //wx_msw

	return(selection);

} /*MacAnovaTextWindow::GetSelection()*/


/*
   Return filename part of path
*/

#ifdef MSDOS
/*  '/' equivalent to '\' on MSDOS*/
#define isSlash(C) ((C) == DIRSEPARATOR[0] || (C) == '/')
#define isPath(N) (strchr(N, '/') || strchr(N, '\\') || strchr(N, ':'))
#else /*MSDOS*/
#define isSlash(C) ((C) == DIRSEPARATOR[0])
#define isPath(N) strchr(N, DIRSEPARATOR[0])
#endif /*MSDOS*/

static char * pathTail(char *filename)
{
	char     *tail;

	for (tail = filename + strlen(filename) - 1;tail >= filename;tail--)
	{
#ifndef MSDOS
		if (isSlash(*tail))
		{
			break;
		}
#else /*MSDOS*/
		if (isSlash(*tail) || *tail == ':')
		{
			break;
		}
#endif /*MSDOS*/
	} /*for (tail = filename + strlen(filename) - 1;tail >= filename;tail--)*/
	tail++;
	return (tail);
} /*pathTail()*/

/*
   If argument prompt is NULL, use global PROMPT (BDEPTH == 0)
   or batch file name (BDEPTH > 0), preceded by EOL.  This should only
   happen at first prompt for a command line.
   If prompt != NULL (probably "more> "), print it with no preceding EOL

   970306 when bdepth == 0 NLINES initialized here rather than in cleanitup()
   */

void MacAnovaTextWindow::PutPrompt(char * prompt)
{
	char           *prmpt;
	int             scrollbackto = -1;
	WHERE("MacAnovaTextWindow::PutPrompt");

	/* write out the prompt */
	prmpt = (prompt == (char *) 0) ? PROMPT : prompt;

	ControlActionStatus = 1;
	SetInsertionPoint(GetLastPosition());
	if (BDEPTH == 0)
	{
		if (prompt == (char *) 0)
		{
#if (0)
			WriteText(EOL_STRING);
#endif /*0*/
			myeol();
		}

		if (CURRENTSCROLLBACK && NLINES >= SCREENHEIGHT)
		{
			scrollbackto = LastCmdStrPos;
		} /*if (CURRENTSCROLLBACK)*/

		NLINES = 0;
		CURRENTSCROLLBACK = SCROLLBACK; /* reset to default */

		myprint(prmpt);
		fmyflush(STDOUT);
		CmdStrPos = GetLastPosition();
	} /*if (BDEPTH == 0)*/
	else if (BATCHECHO[BDEPTH])
	{
		if (prompt == (char *) 0)
		{
			myeol();
		}
		if (prmpt[0] != '\0')
		{
			myprint(prmpt);
		}
		else
		{
			myprint(pathTail(*INPUTFILENAMES[BDEPTH]));
			myprint("> ");
		}
	} /*if (BDEPTH == 0){}else if (BATCHECHO[BDEPTH]){}*/
	
	SetInsertionPointEnd();

	if (scrollbackto >= 0)
	{
		ShowPosition(scrollbackto);
	}

	ControlActionStatus = 0;

} /*putprompt()*/


#ifdef wx_msw
#undef isascii
#define isascii(c) (((unsigned char) (c)) < 256 ? 1 : 0)
#endif

// Gets the character typed, and,  in Motif, kludges for backspacing/delete
/*
  970430 modified so that it returns WXK_DELETE when delete key is
         hit and WXK_BACK on backspace.  In both cases,
         cbs->event->xkey.keycode is set to indicate backspace was
		 pressed.
*/

long MacAnovaTextWindow::KludgeForBackSpaceChar(wxKeyEvent& event)
{
#ifdef wx_motif
#if (0)
	Widget                      textWidget = (Widget) handle;
#endif /*(0)*/
	static KeyCode              backspace_code = 0;
	static KeyCode              delete_code = 0;
	XmTextVerifyCallbackStruct *cbs =
		(XmTextVerifyCallbackStruct *) tempCallbackStruct;
	WHERE("MacAnovaTextWindow::KludgeForBackSpaceChar");

	if (!backspace_code)
	{
		backspace_code =
			XKeysymToKeycode(cbs->event->xkey.display, XK_BackSpace);
		delete_code =
			XKeysymToKeycode(cbs->event->xkey.display, XK_Delete);
	} /*if (!backspace_code)*/
	
	if (cbs->event->xkey.keycode == delete_code)
	{
		event.keyCode = WXK_DELETE;
	}
	else if (cbs->event->xkey.keycode == backspace_code ||
			 event.keyCode == WXK_DELETE)
	{
		event.keyCode = WXK_BACK;
	}

	if (event.keyCode == WXK_BACK || event.keyCode == WXK_DELETE)
	{
		if (cbs->event->xkey.keycode == delete_code)
		{
			cbs->event->xkey.keycode = backspace_code;
		}
		
		cbs->text->length = 0;

		if (event.keyCode == WXK_DELETE && cbs->startPos == cbs->endPos - 1)
		{// deleting will be forward
			cbs->startPos++;
			cbs->endPos++;
		}
		if (cbs->startPos == cbs->endPos)
		{
			cbs->startPos = cbs->endPos-1;
		}
	} /*if (event.keyCode == WXK_BACK || event.keyCode == WXK_DELETE)*/
	return(event.keyCode);
#endif // wx_motif

#ifdef wx_xview
#endif // wx_xview

#ifdef wx_msw
	return(event.keyCode);
#endif //wx_msw
} /*MacAnovaTextWindow::KludgeForBackSpaceChar()*/

/*
	970423 Fixed Windows version bug in the way Delete was handled
	970424 Fixed Windows version bug in backspace in position 0 in window
    970501 Delete key in both Motif and Windows deletes forward
	981110 Changed code processing delete key so that it works in
	       Windows 95 (it worked in Windows 3.1).  It does different
           different things depending on operating system.  It might
           be a good idea to see if new code works in Windows 3.1
    981112 After seeing new code works in Windows 3.1, old code has been
           disabled.
*/

void MacAnovaTextWindow::OnChar(wxKeyEvent& event)
{

	long       ch;
#if (0)
	char      *TextContents;
#endif /*0*/
	Bool       moveBack;
	long       selStart, selEnd;
	WHERE("MacAnovaTextWindow::OnChar");

	if (ControlActionStatus)
    {
/*
   if ControlActionStatus, then the OnChar came from some roundabout
   call from elsewhere.  Send the keystroke, but don't update anything else,
   because it is done elsewhere
*/
		wxTextWindow::OnChar(event);
		return;
	} /*if (ControlActionStatus)*/

#ifdef wx_msw  /*Control I does Interrupt*/
	if (GetKeyState(VK_CONTROL) & 0x80)
	{
		if (event.keyCode == 0x09)
		{
			BaseFrame->KeyInterrupt();
			return;
		}
		if (event.keyCode == 'p' || event.keyCode == 0x10)
		{
			MacAnovaTextFrame     *parent = (MacAnovaTextFrame *) GetParent();
			parent->OnMenuCommand(MACANOVA_FILE_PRINTSELECTION);
			return;
		}
	} /*if (GetKeyState(VK_CONTROL) & 0x80)*/
#endif /*wx_msw*/
	if (BaseFrame->Running)
	{/* currently running a command */
		long        ch = event.keyCode;

		if (Nahead < NAHEAD)
		{/* save info for later processing */
			typeAhead[Nahead++] = ch;
		}
		return;
	} /*if (BaseFrameRunning)*/
#ifdef wx_motif
//	timeStamp = event.timeStamp;
#endif // wx_motif

	ch = KludgeForBackSpaceChar(event);

	GetSelectionPosition(&selStart, &selEnd);

#ifdef wx_msw
/*
	970915 Shift Return and Control Return now same as F6
		   Control Up arrow and Control Keypad Up Arrow same as F7
		   Control Down arrow and Control Keypad Down Arrow same as F8
		   Control F1 through Control F8 switch to graph window 1 through 8
*/
#ifndef WXK_NEWLINE
#define WXK_NEWLINE   10 /* Control Enter seems to have keycode 10 */
#endif /*WXK_NEWLINE*/

	if (ch == WXK_RETURN && GetKeyState(VK_SHIFT) & 0x80 ||
		ch == WXK_NEWLINE && GetKeyState(VK_CONTROL) & 0x80)
	{
		Execute();
		return;
	}
	if (GetKeyState(VK_CONTROL) & 0x80)
	{
		if (ch == WXK_UP || ch == WXK_NUMPAD8)
		{
			UpDownHistory(MACANOVA_EDIT_UPHISTORY);
			return;
		}
		if (ch == WXK_DOWN || ch == WXK_NUMPAD2)
		{
			UpDownHistory(MACANOVA_EDIT_DOWNHISTORY);
			return;
		}
		if (ch >= WXK_F1 && ch < WXK_F1 + NGRAPHS)
		{
			BaseFrame->FindGraphFrame(ch - WXK_F1);
			return;
		}
	} /*if (GetKeyState(VK_CONTROL) & 0x80)*/

	if (ch > 0xff)
	{
	/*
		Might add code to handle special keys
		Home      = WXK_HOME  = 0x013a
		End       = WXK_END   = 0x0139
		Page Up   = WXK_PRIOR = 0x0137
		Page Down = WXK_NEXT  = 0x0138
		F1        = WXK_F1    = 0x0155
		F2        = WXK_F2    = 0x0156
		...
		F12       = WXK_F12   = 0x0160
		For now, just hand them off for processing; they don't change window
	*/
		wxTextWindow::OnChar(event);
		return;
	} /*if (ch > 0xff)*/

	if (event.keyCode == WXK_DELETE)
	{
		/*
			On Delete, Windows moves selection point forward and then
			dispatches backspace so we need do nothing here except
			send the Delete.  We'll be back in a moment with a BS
			and can then adjust CmdStrPos, etc.
			Do nothing if just before CmdStrPos
			Note:  Windows 95 does things differently.  We don't
			get back here and MacAnova doesn't update things like
			CmdStrPos; so we do something else.
            981112 Since the Windows 95 way of doing things seems to work
                   in Windows 3.1, Windows 3.1 code was disabled.
		*/
		if (selStart == selEnd &&
        	(selStart == CmdStrPos - 1 || selEnd == GetLastPosition()))
        {
        	return;
        }
#if (0) /* revised Windows 95 code seems to work fine with Windows 3.1 */
		if (ThisMachine == mvWin32s)
		{ /*Windows 3.1*/
			wxTextWindow::OnChar(event);
			return;
		}
#endif /*0*/
	} /*if (event.keyCode == WXK_DELETE)*/
#endif /*wx_msw*/

	moveBack = (ch == WXK_BACK || ch == WXK_DELETE);

#if defined(wx_motif)
	// kludge to get delete key to delete forward
	if (ch == WXK_DELETE && selEnd == selStart + 1)
	{
		Remove(selStart, selEnd);
		clearUndoInfo();
		ch = 0;
	}
#endif /*wx_motif*/

	if (selStart == selEnd)
	{
		selStart = selEnd = GetInsertionPoint();
#ifdef wx_msw
        if (ch == WXK_DELETE)
        {
			long        xPos1, xPos2, yPos1, yPos2;

		/*
        	The idea is to step forward 1 position and change Delete to
            Backspace, but stepping forward 2 positions at the end of the
            line so as to get paste CR/LF
        */

        	selStart++;
            selEnd++;
          	/* See if we are at end of line (next position has new yPos */
			PositionToXY(selStart, &xPos1, &yPos1);
			PositionToXY(selStart+1, &xPos2, &yPos2);
			/* At end of line we need to skip extra character */
			if (yPos2 > yPos1)
            { /* skip NL */
				selStart++;
				selEnd++;
            }

            SetSelection(selStart,selEnd);
            SetInsertionPoint(selStart);
        }
#endif /*wx_msw*/
	} /*if (selStart == selEnd)*/

	if (ch == WXK_DELETE)
	{
		ch = event.keyCode = WXK_BACK;
	}

#ifdef wx_motif
	if (moveBack && selStart + 1 == selEnd)
	{
		// motif selStart on simple backspace is before the character to be
		// erased, so change it to be in conformance with other models
		selStart = selEnd;
	} /*if (moveBack && selStart + 1 == selEnd)*/
#endif							// wx_motif

	if (ch == WXK_BACK && selStart == selEnd &&
		(selStart == 0 || selStart == CmdStrPos))
	{
		/*
		  at start of window or at CmdStrPos with nothing selected
		  Ignore BS
		*/
		return;
	}

	/*
	   If we get here, then key will change command window
	*/

#if (0)
	moveBack = (back_del &&
				( (selStart < selEnd) ||
				 (selStart == selEnd && selStart != CmdStrPos)  ) );
#endif /*0*/
	Bool      nullSelection = selStart == selEnd;

	if (!nullSelection)
	{/* what ever the key it will clear or replace something */
		clearUndoInfo();
	} /*if (!nullSelection)*/

	if (UndoStatus != TYPINGUNDO && UndoStatus != DELETEUNDO )
	{
		clearUndoInfo(); /* forget undo information */
		UndoPlace = selStart;
		if (selStart == selEnd)
		{
			UndoStatus = TYPINGUNDO;
		}
	} /*if (UndoStatus != TYPINGUNDO && UndoStatus != DELETEUNDO )*/

	if (UndoStatus == TYPINGUNDO || UndoStatus == DELETEUNDO)
	{
		if (selStart != LastTypingPos)
		{/* we've moved! */
			clearUndoInfo();
			UndoStatus = TYPINGUNDO;
			UndoPlace = selStart;
		}
		if (moveBack && selStart <= UndoPlace)
		{
			clearUndoInfo();
		}
	} /*if (UndoStatus == TYPINGUNDO || UndoStatus == DELETEUNDO)*/
	

	OldCmdStrPos = CmdStrPos;
#ifdef wx_msw //980212
	long           newlength = GetLastPosition() - (selEnd - selStart);

	if (!moveBack)
	{
		newlength++;
		if ( ch == CR)
		{ // add extra char for CR/LF
			newlength++;
		}
	}
	else if (nullSelection)
	{
		long xPos,yPos;
		PositionToXY(selStart,&xPos,&yPos);
		if ( xPos == 0)
		{
			// if at head of line, take off another for CR/LF cruft
			newlength--;
		} /*if ( xPos == 0)*/
		newlength--;
	}
#endif /*wx_msw*/

	/* first reset CmdStrPos */
	if (selStart < CmdStrPos)
	{/*only adjust CmdStrPos if before it */
		if (selEnd <= CmdStrPos)
		{
			CmdStrPos -= selEnd - selStart;

			if (!moveBack)
			{
				CmdStrPos++;
#ifdef wx_msw
				if ( ch == CR)
				{
					// add extra char for CR/LF cruft
					CmdStrPos++;
				}
#endif //wx_msw				
			}
			else if (selStart == selEnd)
			{
#ifdef wx_msw
				long       xPos, yPos;

				PositionToXY(selStart,&xPos,&yPos);
				if ( xPos == 0)
				{
					// if at head of line, take off another for CR/LF cruft
					CmdStrPos--;
				} /*if ( xPos == 0)*/
#endif //wx_msw
				CmdStrPos--;
			}
		} /*if (selEnd <= CmdStrPos)*/
		else
		{/* selstart < CmdStrPos && selEnd > CmdStrPos */
			CmdStrPos = selStart;
		}
	} /*if (selStart < CmdStrPos)*/
	
	if (!nullSelection)
	{
		ToUndoBuffer();
#if (0)
		Remove(selStart,selEnd);  /* wxwin removes anyway */
#endif /*0*/
//		selEnd = selStart;
		UndoStatus = DELETEUNDO;	/* deletion or replacement can be undone */
	} /*if (!nullSelection)*/
	
	// we may want to modify here so that highlighting on parenthesis
	// matching doesn't look bad, but I couldn't get it right
	if (ch != 5 && ch != 0)
	{
		wxTextWindow::OnChar(event);
#if defined(wx_msw) && 0 /*need to follow up on this test*/
		if (GetLastPosition() != newlength)
		{
			ALERT("newlength = %ld, GetLastPosition() = %ld",newlength,GetLastPosition(),0,0);
		}
#endif // 

#ifdef wx_motif
		if (!nullSelection)
		{
			Refresh();
		}
#endif // wx_motif
	}
	LastTypingPos = GetInsertionPoint();

#ifdef wx_motif
	if (moveBack)
	{
		LastTypingPos = (selStart == selEnd) ? selEnd - 1 : selStart;
	}
	else
	{
		LastTypingPos = (selStart == selEnd) ? selEnd + 1 : selStart + 1;
	}
#endif // wx_motif
	
	if (ch == WXK_RETURN)
	{
		CmdInsertLine++;
	}
	else if (moveBack)
	{
		CmdInsertLine = GetInsertionLine();
	}

	if (ch == ')' || ch == '}' || ch == ']')
	{
		matchParen(ch);
	}
	if (moveBack && selStart != UndoPlace)
	{
#if (0)
		TextContents = GetContents();
		ch = TextContents[selStart];
		delete [] TextContents;
#else /*0*/ 
		ch = '0'; //all we really need is something that is not a backspace
#endif /*0*/
	}
	LastChar = ch;
	((MacAnovaTextFrame *) GetParent())->ResetEditMenu(UndoStatus);
} /*MacAnovaTextWindow::OnChar()*/

#undef LF
#undef CR
#define LF            10
#define CR            13

void MacAnovaTextWindow::DumpText(void)
{
	long           textLength ;
	char          *Text;
	char           dump[80];
	textLength = GetLastPosition();
	
	Text = GetContents(); // do we need to catch exceptions here?
	sprintf(dump,"CSP %ld OCSP %ld LCSP %ld \n LL %ld textL %ld",
			CmdStrPos,OldCmdStrPos, LastCmdStrPos,LastLength,textLength);
#if (0)
	wxMessageBox(dump, "about");
#endif /*0*/
	for (int i = CmdStrPos;i<textLength;i++)
	{
		sprintf(dump,"textLength %ld CmdStrPos %ld char %d %.2x",
				textLength,CmdStrPos,i,(unsigned int) Text[i]);
#if (0)
		(void) wxMessageBox(dump, "About MacAnova 4.0", wxOK|wxCENTRE);
#endif /*0*/
	}
	delete [] Text;
} /*MacAnovaTextWindow::DumpText()*/

void MacAnovaTextWindow::Execute(void)
{
	long       selStart, selEnd;

	GetSelectionPosition(&selStart,&selEnd);
	
	if (selStart < CmdStrPos && selStart < selEnd)
	{
		CopyToEnd();
	}
	selStart = selEnd = GetLastPosition();
	SetInsertionPointEnd();
	CmdInsertLine = GetInsertionLine();
		
#if (0) // previous code using clipboard
	if (selStart < CmdStrPos && selStart < selEnd)
	{/* copy selection to end before adding Return & leave it on clipboard */
		Copy();
		SetInsertionPointEnd();
		Paste();
	}
#endif /*0*/
	WriteText(EOL_STRING);		// add the return manually
	clearUndoInfo();
} /*MacAnovaTextWindow::Execute()*/

void MacAnovaTextWindow::CopyToEnd(void)
{
	long       selStart, selEnd;
	char      *line;
	
	GetSelectionPosition(&selStart,&selEnd);
	if (selStart < selEnd)
	{/* copy selection to end */
		line = GetSelection(selStart, selEnd);
		if (line != (char *) 0)
		{
#ifdef wx_msw
			stripCR(line);
#endif /*wx_msw*/
			SetInsertionPoint(GetLastPosition());
			WriteText(line);
			delete [] line;
			SetInsertionPoint(GetLastPosition());
			CmdInsertLine = GetInsertionLine();
		}
	}
#if (0) // previous code using clipboard
	if (selStart < selEnd)
	{/* copy selection to end & leave it on clipboard */
		Copy();
		SetInsertionPointEnd();
		SetSelection(GetLastPosition(), GetLastPosition());
		Paste();
	}
#endif /*0*/
} /*MacAnovaTextWindow::CopyToEnd()*/

#ifdef SAVEHISTORY
void MacAnovaTextWindow::setCurrentCommand(char *line)
{
#if (0)
	Replace(CmdStrPos, GetLastPosition(), line);
#else
	if (CmdStrPos < GetLastPosition())
	{
		Remove(CmdStrPos, GetLastPosition());
	}
	SetInsertionPoint(GetLastPosition());
	WriteText(line);
#endif /*0*/
//	SetSelection(GetLastPosition(), GetLastPosition());
	SetInsertionPoint(GetLastPosition());
} /*MacAnovaTextWindow::setCurrentCommand()*/

void MacAnovaTextWindow::UpDownHistory(int id)
{
	char      *line = recallHistory(id);

	if (line != (char *) 0)
	{
		setCurrentCommand(line);
	}
} /*void MacAnovaTextWindow::UpDownHistory()*/

#endif /*SAVEHISTORY*/

int MacAnovaTextWindow::GetLineText(long lineNo, char *buf, int bufLen)
{
#ifdef WXWINMSW
	Synch();
	{
		HWND         hWnd = GetHWND();

		*(WORD *)buf = bufLen;
		int          noChars =
			(int) SendMessage(hWnd, EM_GETLINE, (WPARAM)lineNo, (LPARAM)buf);
		buf[noChars] = 0;
		return noChars;
	}
#else
	return(wxTextWindow::GetLineText(lineNo,buf));
#endif // WXWINMSW
} /*MacAnovaTextWindow::GetLineText()*/

/*
  970203 modified so that there is no checking for quotes or brackets
         on commands starting with '!' for which TRUE is returned only if
         the command terminates with '\n' not preceded by '\\'
*/

Bool MacAnovaTextWindow::inputReady(void)
{
	int            ichar, inquotes, bracklvl, foundComment;
	long           textLength ;
	char           c, *Text = (char *) 0;
#ifdef RECOGNIZEBANG
	int            foundbang;
#endif /*RECOGNIZEBANG*/

	if (INPUTFILE[BDEPTH] !=	/*STDIN*/ (FILE *) 0)
	{/* always ready from not getting input from the command window */
		goto ready;
	}
	
#ifdef wx_msw
	Synch();
#endif //wx_msw
	textLength = GetLastPosition();
	if (textLength == CmdStrPos)
	{ 
		return(FALSE);
	}
	/*do we need to catch exceptions here?*/
	Text = GetSelection(CmdStrPos,textLength);
	if (Text == (char *) 0)
	{
		/*shouldn't happen but probably does*/
		myAlert("A problem internal to MacAnova has been detected\nIt's not your fault.\n\nContinue after the prompt");
		if(!BaseFrame->Running)
		{
			BaseFrame->RunningWindow->clearUndoInfo();
		}
		myeol();
		myeol();
		putOutErrorMsg("***** Attempting to fix problem internal to MacAnova *****");
		
		INTERRUPT = INTSET;			/* set interrupt flag */
		BaseFrame -> GraphicsPause = 0;
		goto notReady;
	}
	
#if (0)
	char         dump[80];
	sprintf(dump,"Text %08x length %ld first 4 chars %.2x %.2x %.2x %.2x",
			(unsigned long) Text, textLength, (unsigned ) Text[0],
			(unsigned )Text[1],(unsigned )Text[2],(unsigned )Text[3]);
	wxMessageBox(dump,"inputReady");
#endif /*0*/

#ifdef RECOGNIZEBANG
	foundbang = (Text[0] == BANG);
#endif /*RECOGNIZEBANG*/

	for (ichar = textLength-CmdStrPos-1;ichar >= 0;ichar--)
	{/* make sure line effectively terminates with '\n' */
		c = Text[ichar];
		if (c == '\n')
		{
#ifdef RECOGNIZEBANG
			if (foundbang && ichar > 0 && Text[ichar-1] == '\\')
			{
				goto notReady;
			}
#endif /*RECOGNIZEBANG*/
			break;
		}
		else if (c != ' ' && c != '\t')
		{/* still waiting for '\n'; not ready yet */
			goto notReady;
		}
	} /*for (ichar = textLength-1;ichar >= CmdStrPos;ichar--)*/
	
	
	if (c != '\n')
	{
		goto notReady;
	}
#ifdef RECOGNIZEBANG
	else if (foundbang)
	{
		goto ready;
	}
#endif /*RECOGNIZEBANG*/

	inquotes = 0;
	bracklvl = 0;
	foundComment = 0;

	ichar = 0;
	while (ichar < textLength-CmdStrPos && ichar >= 0)
	{/* now check balance of '{' and '}', keeping track of quotes */
		c = Text[ichar++];
		
		if (c == '\\')
		{
#ifdef wx_msw
			if (Text[ichar] == '\r' && Text[ichar+1] == '\n')
			{
				ichar++;
			}
#endif //wx_msw
			ichar++;
		} /*if (c == '\\')*/
		else if (!inquotes || c == '"')
		{
			switch((int) c)
			{
			  case '"':
				if (!foundComment)
				{
					inquotes = !inquotes;
				}
				break;
			  case '{':
				if (!foundComment)
				{
					bracklvl++;
				}
				break;
			  case '}':
				if (!foundComment)
				{
					bracklvl--;
				}
				break;
			  case '#':
				foundComment = 1;
				break;
			  case LF:
			  case CR:
				if ( bracklvl <= 0 )
				{
					goto ready;
				}
				foundComment = 0;
			  default:
				break;
			} /*switch((int) c)*/
		} /*if (c == '\\'){...}else if (!inquotes || c == '"'){...}*/
	} /*while (ichar < textLength && ichar >= CmdStrPos)*/
	/* if we get to here, not ready yet */
	/* Fall through */
  notReady:
	if (Text != (char *) 0)
	{
		delete [] Text;
	}
	return (FALSE);

  ready:
	if (Text != (char *) 0)
	{
		delete [] Text;
	}
	return (TRUE);
} /*inputReady()*/


#ifndef MATCHDELAY
#define MATCHDELAY     400
#endif /*MATCHDELAY*/

// Timer
class MacAnovaMatchTimer: public wxTimer
{
  public:
	Bool Finished;
	void Notify(void);
};

void MacAnovaMatchTimer::Notify(void)
{
	Finished = TRUE;
} /*MacAnovaMatchTimer::Notify()*/

#ifndef TICKMULTIPLIER
#ifdef wx_motif
#define TICKMULTIPLIER 100
#else /*wx_motif*/
#define TICKMULTIPLIER 10
#endif /*wx_motif*/
#endif /*TICKMULTIPLIER*/

extern "C" {
void waitForTicks(long wait_ticks)
{
	long         maxcount = wait_ticks*TICKMULTIPLIER;
/*
	maxcount is for safety to prevent essentially infinite loop
	which appeared on wx_msw version
*/
	(void) wxGetElapsedTime(TRUE); // Reset timer
	while (wxGetElapsedTime(FALSE) < wait_ticks && maxcount-- > 0)
	{
		;
	}
} /*waitForTicks()*/
	
#ifdef wx_msw
void stripCR(char *line)
{
	char        *to = line, *from = line, c;

	while ((c = *from++))
	{
		if (c != '\r')
		{
			*to++ = c;
		}
	} /*while ((c = *from++))*/
	*to = '\0';
} /*stripCR()*/

#ifdef HASCLIPBOARD
int myputscrap(char **textH)
{
	long        length = myhandlelength(textH);
	Bool        status;
	int         ncr = 0;
	char       *pc, *text1 = (char *) 0, *pc1;
	
	for (pc = *textH; *pc; pc++)
	{
		if (*pc == '\n')
		{
			ncr++;
		}
	}
	
	if (ncr > 0)
	{
		text1 = new char[length + ncr];
		if (text1 != (char *) 0)
		{
			for (pc = *textH, pc1 = text1; *pc; pc++, pc1++)
			{
				if (*pc == '\n')
				{
					*pc1++ = '\r';
				}
				*pc1 = *pc;
			}
			*pc1 = '\0';
			pc = text1;
		}
	}
	else
	{
		pc = *textH;
	}
	if (wxOpenClipboard())
	{
		wxEmptyClipboard();
		status = wxSetClipboardData(CF_TEXT, (wxObject *) pc, 0, 0);
		wxCloseClipboard();
	}
	else
	{
		status = FALSE;
	}
	if (text1 != (char *) 0)
	{
		delete [] text1;
	}
	return ((int) status);
} /*myputscrap()*/

char ** mygetscrap(void)
{
	char                 *clipString;
	char                **clipText;
	long                  length;

	wxOpenClipboard();
	clipString = (char *) wxGetClipboardData(wxCF_TEXT);
	wxCloseClipboard();
	
	if (clipString)
	{
		stripCR(clipString);
		length=strlen(clipString);
	}
	else
	{
		length = 0;
	}
	
	clipText = mygethandle(length+1);
	if (clipText != (char **) 0)
	{
		if (length > 0)
		{
			strcpy(*clipText, clipString);
		}		
		(*clipText)[length] = '\0';
	}
	else
	{
		putOutErrorMsg("WARNING: not enough memory to copy from Clipboard");
	}
	if (clipString != (char *) 0)
	{
		delete [] clipString;
	}
	
	return (clipText);
} /*mygetscrap()*/
#endif /*HASCLIPBOARD*/
#endif /*wx_msw*/

#ifdef wx_motif
#ifdef HASSELECTION
/*
  970122 myputselection() and mygetselection() interact with the
         selection type of clipping (property XA_PRIMARY), but
         not with the Clipboard with which Copy and Paste on the
         Edit menu ineract.
*/
int myputselection(char **textH)
{
	long        length = strlen(*textH);
	char       *text = new char[length + 1];
	int         status = (text != (char *) 0);
	long        timeStamp = BaseFrame->RunningWindow->timeStamp;
	
	WHERE("myputselection");

	if (wxTheClipboard == (wxClipboard *) 0)
	{
		wxInitClipboard();
	}
#if (0)
	ALERT("length = %d, status = %d, timeStamp = %d",length,status, timeStamp,0);
#endif /*0*/
	if (status)
	{
		strcpy(text, *textH);
		wxTheClipboard->SetClipboardString(text, timeStamp);
	}

	return (status);
} /*myputselection()*/

char ** mygetselection(void)
{
	char                 *clipString;
	char                **clipText = (char **) 0;
	long                  length;
	long                  timeStamp = BaseFrame->RunningWindow->timeStamp;
	WHERE("mygetselection");

	if (wxTheClipboard == (wxClipboard *) 0)
	{
		wxInitClipboard();
	}
	clipString = wxTheClipboard->GetClipboardString(timeStamp);
#if (0)
	ALERT("timeStamp = %d, clipString = '%s'",
		timeStamp,(clipString) ? clipString : "nil",0,0);
#endif /*0*/
	if (clipString != (char *) 0)
	{
		length = strlen(clipString) + 1;
		clipText = mygethandle(length);
		if (clipText != (char **) 0)
		{
			strcpy(*clipText, clipString);
		}
		delete [] clipString;
	} /*if (clipString != (char *) 0)*/
	
	return (clipText);
} /*mygetselection()*/
#endif /*HASSELECTION*/

#if (0) /* for debugging */
void dumpLine(char * line, int length)
{
	char        *outstr = OUTSTR;

	while (length-- > 0)
	{
		sprintf(outstr,"%02x", (int) *line++);
		outstr += strlen(outstr);
	}
	myAlert(OUTSTR);
	*OUTSTR = '\0';
} /*dumpLine()*/

#endif /*0*/

#ifdef HASCLIPBOARD

static XmString makeClipLabel(char *name)
{
	return (XmStringCreateLtoR(name,
							   (XmStringCharSet) XmFONTLIST_DEFAULT_TAG));
} /*makeClipLabel()*/

int myputscrap(char **textH)
{
	long               length = strlen(*textH);
	long               item_id = 0;
	long               data_id = 0;
	int                status = 1;
	MacAnovaTextWindow *mtw = BaseFrame->RunningWindow;
	Widget             textWidget = (Widget) mtw->handle;
	char              *clipString;
	XmString           clipLabel;

	if (length > 0)
	{
		clipLabel = makeClipLabel("MACANOVA");
		if (clipLabel == (XmString) 0)
		{
			goto errorExit;
		}
		
	
		clipString = new char[length+1];
		if (clipString == (char *) 0)
		{
			goto errorExit;
		}

		strcpy(clipString, *textH);
		
		for (status = 0; status != ClipboardSuccess ; )
		{
			status = XmClipboardStartCopy(XtDisplay(textWidget),
										  XtWindow(textWidget),
										  clipLabel,
										  mtw->timeStamp,
										  textWidget,
										  NULL, &item_id);
		} /*for (status = 0;status != ClipboardSuccess ; )*/
		XmStringFree(clipLabel);
		for (status = 0;status != ClipboardSuccess ; )
		{
			status = XmClipboardCopy(XtDisplay(textWidget),
									 XtWindow(textWidget),
									 item_id,
									 "STRING",
									 (XtPointer) clipString,
									 length + 1,
									 0, &data_id);
		} /*for (status = 0;status != ClipboardSuccess ; )*/
	
		delete [] clipString;
	
		for (status = 0;status != ClipboardSuccess ; )
		{
			status = XmClipboardEndCopy(XtDisplay(textWidget),
										XtWindow(textWidget),
										item_id);
		} /*for (status = 0;status != ClipboardSuccess ; )*/
	} /*if (status)*/
	
	return (status);

  errorExit:
	return (0);
	
} /*myputscrap()*/

char ** mygetscrap(void)
{
	unsigned long      item_id = 0;
	int                data_id = 0;
	int                status;
	unsigned long      length, outlength = 0;
	long               private_id = 0;
	MacAnovaTextWindow *mtw = BaseFrame->RunningWindow;
	Widget             textWidget = (Widget) mtw->handle;
	char              *clipString;
	char             **textH;
	WHERE("mygetscrap");

	/*
	  length of data on clipboard may or may not include a terminating
	  '\0'.  Hence we have to assume it does not and allocate an
	  extra byte; but then we have to compare the length with
	  strlen(*textH) and resize textH if necessary
	*/
	for (status = 0; status != ClipboardSuccess ; )
	{
		status = XmClipboardInquireLength(XtDisplay(textWidget),
										  XtWindow(textWidget),
										  "STRING", &length);
		if (status == ClipboardNoData)
		{
			length = 0;
			break;
		}
	} /*for (status = 0; status != ClipboardSuccess ; )*/
	textH = mygethandle(length + 1);
	if (textH != (char **) 0)
	{
		(*textH)[length] = '\0';
		if (length > 0)
		{
			status = XmClipboardRetrieve(XtDisplay(textWidget),
										 XtWindow(textWidget),
										 "STRING", *textH, length + 1,
										 &outlength, &private_id);
			if (status != ClipboardSuccess)
			{
				mydisphandle(textH);
				textH = (char **) 0;
			}
			else
			{
				unsigned long     length1 = strlen(*textH);
				
				if (length1 != length)
				{
					textH = mygrowhandle(textH, length1 + 1);
				}
			}
		} /*if (length > 0)*/
	} /*if (textH != (char **) 0)*/
	
	return (textH);
} /*mygetscrap()*/

#endif /*HASCLIPBOARD*/
#endif /*wx_motif*/
} /* extern "C" */


static char     *LParens = "({[", *RParens = ")}]";

void MacAnovaTextWindow::matchParen(char cRight)
{
	char            cLeft, c, *Text;
	long            selStart,place,limit,count;
	
	The_timer->Stop(); // stop the main timer during paren match

	selStart = GetInsertionPoint();
	place = selStart - 1;
	limit = (selStart >= CmdStrPos) ? CmdStrPos : 0;
#ifdef wx_msw
	count = 0;
#else //wx_msw
	count = 1;
#endif //wx_msw
	Text = GetContents();
	
	cLeft = LParens[strchr(RParens, cRight) - RParens];
	
	do
	{
		c = Text[place--];
#if (0)
		sprintf(dump,"Matchparen trying character %c at position %d",c,
				place+1);
		(void) wxMessageBox(dump, "About MacAnova 4.0", wxOK|wxCENTRE);
#endif /*0*/
		if (strchr(RParens,c) != (char *) 0)
		{
			count++;
		}
		else if (strchr(LParens,c) != (char *) 0)
		{
			count--;
		}
	} while (place >= limit && count > 0);
	// 970320 fixed profound memory leak here
	delete [] Text;
	place++;
	if (count == 0 && c == cLeft)
	{
#if (0)
		sprintf(dump,"Matched at place %d",place);
		(void) wxMessageBox(dump, "About MacAnova 4.0", wxOK|wxCENTRE);
#endif /*0*/
#ifdef wx_motif
		XmTextSetHighlight((Widget) handle, place,place+1, XmHIGHLIGHT_SELECTED);
#else /*wx_motif*/
		SetSelection(place,place+1);
#endif /*wx_motif*/
		if (myApp.Pending())
		{
			myApp.Dispatch();
		}
		waitForTicks(MATCHDELAY);
	}
	else
	{
		mybeep(1);
	}
#ifdef wx_motif
	XmTextSetHighlight((Widget) handle, place,place+1, XmHIGHLIGHT_NORMAL);
#else /*wx_motif*/
	SetSelection(selStart, selStart);
#endif /*wx_motif*/
   The_timer->Start(MACANOVATIMERINCREMENT);
} /*MacAnovaTextWindow::matchParen()*/


void MacAnovaTextWindow::clearUndoInfo(void)
{
	UndoStatus = CANTUNDO;
	PasteLength = -1;
	UndoPlace = -1;
	LastTypingPos = -1;
	LastCmdStrPos = -1;
	LastLength = -1;
	OldCmdStrPos = -1;
	UndoBuffer[0] = 0;
} /*MacAnovaTextWindow::clearUndoInfo()*/

void MacAnovaTextWindow::ToUndoBuffer(void)
{
	long          selStart,selEnd;
	char         *TextContents;
	
	GetSelectionPosition(&selStart,&selEnd);
	if ( selEnd - selStart > UndoBufferAllocation - 2)
	{
		long       allocation = 2*(selEnd - selStart);
		
		allocation = (allocation < UNDOBUFFERINITIALALLOCATION) ?
			UNDOBUFFERINITIALALLOCATION : allocation;
		if (UndoBuffer != (char *) 0)
		{
			delete [] UndoBuffer;
		}
		
		UndoBuffer = new char[allocation];
		UndoBufferAllocation = (UndoBuffer  != (char *) 0) ? allocation : 0;
	}
	if (selStart != selEnd && UndoBuffer != (char *) 0) 
 	{
		TextContents = GetSelection(); 
		if (TextContents != (char *) 0)
		{
			strcpy(UndoBuffer,TextContents);
			delete [] TextContents;
		}
		else
		{
			UndoBuffer[0] = '\0';
		}
#if (0)
		sprintf(dump,"we put '%s' in undoBuffer",UndoBuffer);
		(void) wxMessageBox(dump, "About MacAnova 4.0", wxOK|wxCENTRE);
#endif /*0*/
	}
	UndoPlace = selStart;
} /*MacAnovaTextWindow::ToUndoBuffer()*/

long MacAnovaTextWindow::FromUndoBuffer(void)
{
	long        scrapLength;
	
#if (0)
	char        dump[80];
	sprintf(dump,"undobuffer contains '%s'",UndoBuffer);
	(void) wxMessageBox(dump, "About MacAnova 4.0", wxOK|wxCENTRE);
#endif /*0*/
	if (UndoPlace != -1)
	{
		scrapLength = strlen(UndoBuffer);
#if (0)
		sprintf(dump,"undoplace %ld scrplen %ld",UndoPlace,scrapLength);
		(void) wxMessageBox(dump, "About MacAnova 4.0", wxOK|wxCENTRE);
		int         i;
		for (i=0;i<scrapLength;i++)
		{
			sprintf(dump,"UndoBuffer[%d] is %x",i,UndoBuffer[i]);
			(void) wxMessageBox(dump, "About MacAnova 4.0", wxOK|wxCENTRE);
		}
#endif /*0*/
#ifdef wx_msw
		Synch();
		HWND hWnd = GetHWND();
		SendMessage(hWnd, EM_SETSEL, (WPARAM) UndoPlace, (LPARAM) UndoPlace);
		SendMessage(hWnd, EM_REPLACESEL, 0, (LPARAM) UndoBuffer);
#else //wx_msw
		Replace(UndoPlace,UndoPlace,UndoBuffer);
		
#endif // wx_msw
#if (0)
		(void) wxMessageBox("should have put buffer back",
							"About MacAnova 4.0", wxOK|wxCENTRE);
#endif /*0*/
		SetSelection(UndoPlace,UndoPlace+scrapLength);
#if (0)
		(void) wxMessageBox("should have set selection", "About MacAnova 4.0",
							wxOK|wxCENTRE);
#endif /*0*/
	}
	else
	{
		scrapLength = 0;
	}
	if (OldCmdStrPos >= 0)
	{
		CmdStrPos = OldCmdStrPos;
	}
	clearUndoInfo(); /* can't redo */
	
	return (scrapLength);
	
} /*MacAnovaTextWindow::FromUndoBuffer()*/

