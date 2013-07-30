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
	970111 added initHistory()
	971210 added getSomeHistory(), setSomeHistory()
*/

#ifndef UNXPROTOH__
#define UNXPROTOH__

#ifdef READLINE
#if defined (READLINE_LIBRARY)
#  include "readline.h"
#else /*READLINE_LIBRARY*/
#  include <readline/readline.h>
#endif /*READLINE_LIBRARY*/
#endif /*READLINE*/

#ifndef NOPROTOTYPES
/* main.c */
int main(int /*argc*/,char ** /*argv*/);
void closeBatch(long /*all*/);
/* unxhandl.c */
void **myFindHandle(void ** /*h*/);
void **myNewHandle(long /*n*/, long /*keepLocked*/);
void myDisposHandle(void **/*name*/);
void **mySetHandleSize(void **/*name*/, long /*newn*/, long /*keepLocked*/);
void **myDupHandle(void **/*name*/, long /*keepLocked*/);
long myValidHandle(void ** /*h*/);
long myGetHandleSize(void **/*h*/);
void myHLock(void **/*h*/);
void myHUnlock(void **/*h*/);
void *myNewPtr(long /*n*/);
void myDisposePtr(void */*p*/);
void setWarehouseLimits(long /*standard*/);
void myInitHandles(void);
void memoryUsage(char * /*where*/);
/* unxio.c */
void saveForUpdate(char * /*msg*/);
void updateConsole(void);
void activeUpdateInit(void);
#ifdef READLINE
#ifdef READLINE11
Keymap rl_get_keymap(void);
void rl_set_keymap(Keymap /*map*/);
#endif /*READLINE11*/
void rlpauseCleanup(void);
#endif /*READLINE*/
void incrementNLINES(char * /*msg*/);
void mypause(void);
FILE *fmyopen(char * /*fileName*/, char * /*mode*/);
void fmyprint(char */*msg*/, FILE */*fp*/);
void fmyeol(FILE */*fp*/);
void fmyflush(FILE */*fp*/);
void mybeep(long /*n*/);
int fillLINE(FILE */*fn*/);
void putprompt(char */*prompt*/);
#ifdef READLINE
int my_rl_getc(FILE * /*f*/);
#endif /*READLINE*/
#ifdef SAVEHISTORY
void initHistory(long /*historyLength*/);
void saveInHistory(char ** /*lineH*/);
long getSomeHistory(long /*nlines*/, char *** /*historyHandle*/);
void setSomeHistory(long /*nlines*/, char ** /*lines*/);
#endif /*READLINE*/
char *expandFilename(char */*fname*/);
long okfilename(char */*fname*/);
long okpathname(char */*fname*/);
long isfilename(char */*fname*/);
#ifdef MSDOS
char *get_dataPath(void);
#endif /*MSDOS*/
#else /*NOPROTOTYPES*/
/* main.c */
int main(/*int argc,char ** argv*/);
void closeBatch(/*long all*/);
/* unxhandl.c */
void **myFindHandle(/*void ** h, void *** next, void *** prev*/);
void setNextEntry(/* void ** next, void ** prev*/);
void **myNewHandle(/*long n, long keepLocked*/);
void myDisposHandle(/*void **name*/);
void **mySetHandleSize(/*void **name, long newn, long keepLocked*/);
long myValidHandle(/*void ** h*/);
void **myDupHandle(/*void **name, long keepLocked*/);
long myGetHandleSize(/*void **h*/);
void HLock(/*char **ch*/);
void HUnlock(/*void **ch*/);
void *myNewPtr(/*long n*/);
void myDisposePtr(/*void *p*/);
void setWarehouseLimits(/*long standard*/);
void myInitHandles(/*void*/);
void memoryUsage(/*char * where*/);
/* unxio.c */
#ifdef ACTIVEUPDATE
void     saveForUpdate(/*char * msg*/);
void     updateConsole(/*void*/);
void     activeUpdateInit(/*void*/);
#endif /*ACTIVEUPDATE*/
#ifdef READLINE
#ifdef READLINE11
Keymap rl_get_keymap(/*void*/);
void rl_set_keymap(/*Keymap map*/);
#endif /*READLINE11*/
void rlpauseCleanup(/*void*/);
#endif /*READLINE*/
void incrementNLINES(/*char *msg*/);
void mypause(/*void*/);
FILE *fmyopen(/*char * fileName, char * mode*/);
void fmyprint(/*char *msg, FILE *fp*/);
void fmyeol(/*FILE *fp*/);
void fmyflush(/*FILE *fp*/);
void mybeep(/*long n*/);
int fillLINE(/*FILE *fn*/);
void putprompt(/*char *prompt*/);
#ifdef READLINE
#ifdef READLINE11
Keymap rl_get_keymap(/*void*/);
void rl_set_keymap(/*Keymap map*/);
#endif /*READLINE11*/
int my_rl_getc(/*FILE * f*/);
#endif /*READLINE*/
#ifdef SAVEHISTORY
void initHistory(/*long historyLength*/);
void saveInHistory(/*char ** lineH*/);
long getSomeHistory(/*long nlines, char *** historyHandle*/);
void setSomeHistory(/*long nlines, char ** lines*/);
#endif /*SAVEHISTORY*/
char *expandFilename(/*char *fname*/);
long okfilename(/*char *fname*/);
long okpathname(/*char *fname*/);
long isfilename(/*char *fname*/);
#ifdef MSDOS
char *get_dataPath(/*void*/);
#endif /*MSDOS*/
#endif /*NOPROTOTYPES*/

#endif /*UNXPROTOH__*/
