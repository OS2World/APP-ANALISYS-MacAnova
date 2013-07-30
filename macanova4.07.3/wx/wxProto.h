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
  970910 added initializePSPrinter() to wxgraph.cc
  970930 deleted initializePSPrinter() from wxgraph.cc
  980511 changed fillLINE() to fileFromDialog() in conjuction of
		 move of fillLINE() to commonio.c.  fillLINE() is now defined
		 in matProto.h

*/
#ifndef WXPROTOH__
#define WXPROTOH__

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

#ifdef wx_motif
#include <Xm/Xm.h>
#endif /*wx_motif*/

/* wxIo.cc */
char *get_dataPath(void);
int fillFromDialog(void);
char *wxFindFile(char * /*fileName*/, char * /*message*/,
				 char * /*defaultname*/);
void incrementNLINES(char * /*msg*/);
FILE *fmyopen(char * /*fileName*/, char * /*mode*/);
void lineToWindow(char * /*msg*/, long /*msgLength*/);
void            fmyeol(FILE * /*pf*/);
void            putprompt(char * /*prompt*/);
void myAlert(char * /*msgs*/);
int mygetosversion(void);
void            fmyprint(char * /*string*/, FILE * /*fp*/);
#ifdef SAVEHISTORY
void initHistory(long /*historyLength*/);
void saveInHistory(char ** /*lineH*/);
char * recallHistory(int /*id*/);
long getSomeHistory(long /*nlines*/, char *** /*historyHandle*/);
void setSomeHistory(long /*nlines*/, char ** /*lines*/);
#endif /*SAVEHISTORY*/
char *expandFilename(char * /*fname*/);
long okfilename(char * /*fname*/);
long okpathname(char * /*fname*/);
long isfilename(char * /*fname*/);
void fmyflush(FILE * /*fp*/);
void mybeep(long /*n*/);

/* wxhandl.c */
long myValidHandle(void ** /*h*/);
void **myFindHandle(void ** /*h*/);
void **myNewHandle(long /*length*/, long /*keepLocked*/);
void myDisposHandle(void ** /*h*/);
void **mySetHandleSize(void ** /*h*/, long /*newn*/, long /*keepLocked*/);
void **myDupHandle(void ** /*h*/, long /*keepLocked*/);
long myGetHandleSize(void ** /*h*/);
void myHLock(void ** /*ch*/);
void myHUnlock(void ** /*ch*/);
void *myNewPtr(long /*n*/);
void myDisposePtr(void * /*p*/);
void setWarehouseLimits(long /*standard*/);
void myInitHandles(void);
void memoryUsage(char * /*NAMEFORWHERE*/);

/* wxmain.cc */
void closeBatch(long /*all*/);
void doBatch(void);
void putAboutBox(void);
void putHelpBox(void);
void putUnimplementedBox(void);
void myYield(void);
void * wxMalloc(size_t /*size*/);
void wxFree(void * /*ptr*/);

/* wxgraph.cc*/
char * findPlotFile(char * /*fileName*/, char * /*kind*/, long /*ps*/,
					long /*epsf*/, long /*newFile*/);
/* wxtframe.cc*/
/* wxtwind.cc*/
void waitForTicks(long /*wait_ticks*/);
#ifdef wx_motif
void DoKeyInterrupt (Widget /*w*/, XEvent * /*event*/, String * /*params*/,
					 Cardinal * /*num_params*/);
void DoFindGraph (Widget /*w*/, XEvent * /*event*/, String * /*params*/,
				  Cardinal * /*num_params*/);
void DoFastQuit (Widget /*w*/, XEvent * /*event*/, String * /*params*/,
				 Cardinal * /*num_params*/);
#endif /*wx_motif*/
#ifdef wx_msw
void stripCR(char * /*line*/);
#endif /*wx_msw*/
#ifdef HASCLIPBOARD
int myputscrap(char ** /*text*/);
char ** mygetscrap(void);
#endif /*HASCLIPBOARD*/
#ifdef wx_motif
int myputselection(char ** /*text*/);
char ** mygetselection(void);
#endif /*wx_motif*/
#ifdef __cplusplus
} /*extern "C"*/
#endif /*__cplusplus*/
#endif /*WXPROTOH__*/
