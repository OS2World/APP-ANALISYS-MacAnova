/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 4.05 or later
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



#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment Startmsg
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

#include "version.h"
#include "globals.h"

#ifdef MACINTOSH
#include "macIface.h"
#endif /*MACINTOSH*/

/*
   951113  Changed to do centering using centerBuffer().
*/
char       *Welcome[] = 
{
	(char *) 0, /* "M A C A N O V A   x.xx" */
	"An Interactive Program for Statistical Analysis and Matrix Algebra",
	"For information on major features, type 'help(macanova)'",
	"For information on linear models and GLM's, type 'help(glm)'",
	"For latest information on changes, type 'help(news)'",
#if defined (MACINTOSH)
	"For information on Macintosh version, type 'help(macintosh)'",
#elif defined (BCPP) || defined (DJGPP)
	"For information on DOS versions, type 'help(dos_windows)'",
#elif defined (WXWINMSW)
	"For information on Windows version, type 'help(wx)'",
#elif defined (WXWINMOTIF)
	"For information on Motif version, type 'help(wx)'",
#elif defined (UNIX)
	"For information on Unix version, type 'help(unix)'",
#endif

#ifdef TODAY
	(char *) 0, /* "Version of xx/xx/xx" */
#endif /*TODAY*/

#ifdef COPYRIGHT1
	(char *) 0,
#endif /*COPYRIGHT1*/

#ifdef COPYRIGHT2
	(char *) 0,
#endif /*COPYRIGHT2*/

#ifdef COPYRIGHT3
	(char *) 0,
#endif /*COPYRIGHT3*/

#ifdef COPYRIGHT4
	(char *) 0,
#endif /*COPYRIGHT4*/

#ifdef MACANOVA2AVAIL
	(char *) 0,
#endif /*MACANOVA2AVAIL*/

#ifdef MACANOVA31AVAIL
	(char *) 0,
#endif /*MACANOVA31AVAIL*/

#ifdef BACKUPS
	(char *) 0,
	(char *) 0,
#endif /*BACKUPS*/
	(char *) 0
};

#define MAXLENGTHMSG  80
#define MAXMARGIN     20

static char    Version[MAXLENGTHMSG+1] = ""; 

#ifdef TODAY
static char    Today[MAXLENGTHMSG+1] = ""; 
#endif /*TODAY*/

#ifdef COPYRIGHT1
static char    Copyright1[MAXLENGTHMSG+1];
#endif /*COPYRIGHT1*/
#ifdef COPYRIGHT2
static char    Copyright2[MAXLENGTHMSG+1];
#endif /*COPYRIGHT2*/
#ifdef COPYRIGHT3
static char    Copyright3[MAXLENGTHMSG+1];
#endif /*COPYRIGHT3*/
#ifdef COPYRIGHT4
static char    Copyright4[MAXLENGTHMSG+1];
#endif /*COPYRIGHT4*/

#ifdef MACANOVA2AVAIL
static char    Macanova2Msg[MAXLENGTHMSG+1] = "";
#endif /*MACANOVA2AVAIL*/

#ifdef MACANOVA31AVAIL
static char    Macanova31Msg[MAXLENGTHMSG+1] = "";
#endif /*MACANOVA31AVAIL*/

#ifdef BACKUPS
static char    BackupMsg1[MAXLENGTHMSG+1] = "";
static char    BackupMsg2[MAXLENGTHMSG+1] = "";
#endif /*BACKUPS*/

/*
   Concatenate head and tail in msgs and return msgs
*/
static char *makeMsgLine(char *head, char *tail, char *msgs)
{
	int       i;

	for (i = 0; i < MAXLENGTHMSG && *head != '\0';i++)
	{
		msgs[i] = *head++;
	} /*for (i = 0; i < MAXLENGTHMSG && *head != '\0';i++)*/

	while (i < MAXLENGTHMSG && *tail != '\0')
	{
		msgs[i++] = *tail++;
	}
	msgs[i] = '\0';
	return (msgs);
} /*makeMsgLine()*/

void startupMsg(long quiet)
{
 	char		  **message;
	int             length = SCREENWIDTH;
	WHERE("startupMsg");
	
	message = Welcome;
#ifdef VERSION
	*message++ = makeMsgLine("M A C A N O V A   ", VERSION, Version);
#else /*VERSION*/
	*message++ = makeMsgLine("M A C A N O V A","", Version);
#endif /*VERSION*/

	while(*message != (char *) 0)
	{
		message++;
	}

#ifdef TODAY
	*message++ = makeMsgLine("Version ",TODAY, Today);
#endif /*TODAY*/

#ifdef COPYRIGHT1
	*message++ = makeMsgLine(COPYRIGHT1, "", Copyright1);
#endif /*COPYRIGHT1*/
#ifdef COPYRIGHT2
	*message++ = makeMsgLine(COPYRIGHT2, "", Copyright2);
#endif /*COPYRIGHT2*/
#ifdef COPYRIGHT3
	*message++ = makeMsgLine(COPYRIGHT3, "", Copyright3);
#endif /*COPYRIGHT3*/
#ifdef COPYRIGHT4
	*message++ = makeMsgLine(COPYRIGHT4, "", Copyright4);
#endif /*COPYRIGHT4*/

#ifdef MACANOVA2AVAIL
	*message++ = makeMsgLine("MacAnova 2.4x may be accessed as ",
							MACANOVA2AVAIL, Macanova2Msg);
#endif /*MACANOVA2AVAIL*/

#ifdef MACANOVA31AVAIL
	*message++ = makeMsgLine("MacAnova 3.1x may be accessed as ",
							MACANOVA31AVAIL, Macanova31Msg);
#endif /*MACANOVA31AVAIL*/

#ifdef BACKUPS
	*message++ = makeMsgLine("Past versions of MacAnova 4.0x are ","",
						  BackupMsg1);

	*message++ = makeMsgLine(BACKUPS,"",BackupMsg2);
#endif /*BACKUPS*/
	*message = (char *) 0;

	if (!quiet)
	{
#ifdef MACINTOSH
		DoApple(-aboutmacanova);
#endif /*MACINTOSH*/
		message = Welcome;
		length =  (length >= BUFFERLENGTH) ? BUFFERLENGTH-1 : length;

		while(*message != (char *) 0)
		{
			(void) centerBuffer(OUTSTR, *message, length);
			putOUTSTR();
			if(message++ == Welcome)
			{					/* extra line after first */
				myeol();
			}
		} /*while(*message != (char *) 0)*/
	} /*if (!quiet)*/	
} /*startupMsg()*/


