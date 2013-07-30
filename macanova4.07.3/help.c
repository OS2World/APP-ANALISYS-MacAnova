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



#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment Help
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


/*
  Functions related to help()
*/
#define HELP__  /*tells mvhelp.h to declare certain statics */
#include "globals.h"
#include "mvhelp.h" /*briefly named helpkb.h*/

#ifdef MACINTOSH
#include "macIface.h"
#ifdef MPW
#include <StdLib.h>
#endif /*MPW*/
#endif /*MACINTOSH*/

#ifdef WXWIN
#include "wx/wxIface.h"
#endif  /* WXWIN */

/*
  980722 moved a lot of stuff into header helpkb.h
*/
#define LF   10
#define CR   13

#undef NEWLINE
#ifdef MACINTOSH
#define NEWLINE   CR
#else
#define NEWLINE   LF
#endif /*MACINTOSH*/

#if (0) /* the following now in helpkb.h*/
#define MARKER '='
#define NMARKERS 4   /* help topics start with NMARKERS MARKER's */
/* Both of the following strings should be NMARKERS long */
#define MARKERSTRING "===="
#define USAGESTRING  "%%%%"
#define KEYSTRING    "????"
#define STARTKEYS    '#'
#define SEPKEYS      ','
#define PUTUSAGE      1
#define PUTHELP       0
#define EOFSTRINGLENGTH 7
#define EOFSTRING    "_E_O_F_"
#define EOFSTRINGSTART '_'

#define HELPROOM      2000 /* initial space for list of topics */
#define KEYROOM        400  /* initial space for list of keys */
#define NTOPICS        250 /* initial number of topics */
#define KEYLENGTH       50
#define TOPICNAMELENGTH 20
#define TOPICCHUNK      10
#define HELPCHUNK (7*TOPICCHUNK)
#define MAXKEYS         32 /* maximum number of keys allowed per file */

#define CREATION    870000  /*MacAnova started in 1987*/
#define CENTURY    1000000  /*100 years*/
#define DATEDIGITS       8  /*strlen("19971231")*/


static short           NHelpTopics = -1; /* < 0 means initHelp not called */
static char          **Topics = (char **) 0;
static short         **TopicOffsets = (short **) 0; /* offsets in Topics */
static unsigned long **KeyTable = (unsigned long **) 0; /*Help key info */
static char          **KeyList = (char **) 0; /* list of help keys */
static short           KeyOffsets[MAXKEYS], NKeys;
#ifdef USEFSEEK
static long          **FileOffsets = (long **) 0; /* positions in file */
static long            LineOffset = 0;
#endif /*USEFSEEK*/

static char          **Buffer = (char **) 0; /* line buffer */
static short           CurrentTopic = 0;
static short           BufferFull = 0; /* != 0 means a line is in buffer */
static char          **HelpFileName = (char **) 0;
static FILE           *HelpFile = (FILE *) 0;
static char          **AltHelpFileName = (char **) 0;
static FILE           *AltHelpFile = (FILE *) 0;
static short           HelpEofHit = 0;
static char            LastHelpWasCr = 0; /*Flag for last Character == CR*/
static int             Damaged;

static long            LatestNews = -1; /* yyyymmdd date of latest news item*/
static long            OldestNews = -1; /* yyyymmdd date of oldest news item*/
static char            NEWS[] = "news";
#endif /*0*/

#define OrigHelpFile HELPFILE
#define OrigHelpFileName HELPFILENAME

enum helpFileCodes
{
	INEWALTFILE = 1,
	IORIGFILE,
	IALTFILE
};

/*
   950919 added check that HelpFile is non-null

   950928 Added new keyword alt.  alt:T switches to most recent file
          set by file:"filename".  Added globals AltHelpFile and
		  AltHelpFileName

   970919 Added checks for too long key names to avoid crash.  A count of
          bad lines is kept in global Damaged and printed if non-zero

   971118 Help file name is part of warning message when file can't be opened

   971222 News dates should now be o.k. until 2050.  All dates in file
          < 500000 are interpreted as 20000000 + date; all dates >=
          500000 and < 1000000 are interpreted as 19000000 + date

   980521 Added arguments to scanPat() to indicate it should check for
          the pattern being a possible name and for the length of pattern
		  Moved definition of EXACTMATCH, etc. to globdefs.h

   980827 Added 'next' to ControlNames (list of syntax elements)

   981214 Modified usage output for trig functions to reflect new keyword
          phrases degrees:T, radians:T and cycles:T
   981215 usage() now uses usage info from file for transformations rather
          than generating it.  help() still prints topic transformations
          for most transformations
   990215 Changed most putOUTSTR() to putErrorOUTSTR()
          Changed myerrorout() to putOutMsg() and putOutErrorMsg()
   990325 On Macintosh, help file can have type 'ttro'
*/

static void rewindHelp(void)
{
	if (HelpFile != (FILE *) 0)
	{
		rewind(HelpFile);
	}
	
#ifdef USEFSEEK
	if (LineOffset > 0)
	{
		LineOffset = 0;
	}
#endif /*USEFSEEK*/
	BufferFull = 0;
	CurrentTopic = 0;
	HelpEofHit = 0;
	LastHelpWasCr = 0;
} /*rewindHelp()*/

#undef   CR
#undef   NL
#define  CR   13
#define  NL   10

static int helpgetc(FILE * fp)
{
	int           c;

	c = getc(fp);
	c = (c == NL && LastHelpWasCr) ? getc(fp) : c;

	LastHelpWasCr = (c == CR);

#ifndef MACINTOSH
	c = (LastHelpWasCr) ? '\n' : c;
#else
	c = (c == NL) ? '\n' : c;
#endif /*MACINTOSH*/

#ifdef MSDOS
#define MSDOSEOF    26
	c = (c == MSDOSEOF) ? EOF : c;
#endif /*MSDOS*/

	return (c);
} /*helpgetc()*/

/* 
  Read upto n characters from fp into s[].  return 0 is EOF hit before
  any characters read
  A line starting with EOFSTRING is considered to be an internal EOF marker
  Line is put in *Buffer, without trailing NEWLINE
*/
static int getHelpLine(void)
{
	int         c, i, n = BUFFERLENGTH;
	char       *buffer = *Buffer;
	WHERE("getHelpLine");
	
#ifdef USEFSEEK
	if (LineOffset >= 0)
	{ /* need file position only during the first scann */
		LineOffset = ftell(HelpFile);
		LastHelpWasCr = 0;
	}
#endif /*USEFSEEK*/
	for (i = 0;i < n;i++)
	{
		c = helpgetc(HelpFile);
		if (isNewline(c) || c == EOF)
		{
			break;
		}
		else
		{
			buffer[i] = c;
		}
	} /*for (i = 0;i < n;i++)*/

	if (buffer[0] == EOFSTRINGSTART &&
		strncmp(buffer, EOFSTRING, EOFSTRINGLENGTH) == 0)
	{
		i = 0;
		c = EOF;
	}
	buffer[i] = '\0';
	
	HelpEofHit = (c == EOF);

	if (i == 0 && HelpEofHit)
	{
		return (0);
	}
	else
	{
		return (1);
	}
} /*getHelpLine()*/

/*
  read help file until line is found starting with ====topicName
  return pointer to topicName in buffer
  return NULL on EOF
*/

static int nextTopic(char *topic)
{
	int         i;
	char        *buffer;
	WHERE("nextTopic");

	if (HelpEofHit)
	{
		rewindHelp();
	}
	
	topic[0] = '\0';
	
	while (1)
	{
		if (!BufferFull)
		{
			if (getHelpLine() == 0)
			{
				return (0);
			}
		}
		else
		{
			BufferFull = 0;
		}
		buffer = *Buffer;
		if (strncmp(buffer, MARKERSTRING, NMARKERS) == 0)
		{
			BufferFull = 1;
			buffer += NMARKERS;
			for (i = 0;i < TOPICNAMELENGTH;i++)
			{
				if (!isnamechar(buffer[i]))
				{
					break;
				}
				topic[i] = buffer[i];
			}
			topic[i] = '\0';
			BufferFull = 1;
			return (1);
		} /*if (strncmp(buffer, MARKERSTRING, NMARKERS) == 0)*/
	} /* while (1) */
} /*nextTopic()*/

static void damagedFile(void)
{
	sprintf(OUTSTR,
			"WARNING: help file \"%s\" appears to have been damaged; bad line is",
			*HelpFileName);
	putErrorOUTSTR();
	putOutMsg(*Buffer);
} /*damagedFile()*/


/*
   Function to read list of help keys from file, allocate storage, and
   save them in *KeyList.  Offsets to the keys go in array KeyOffsets
   When called, the Key markers should be in the buffer.  Comma- and/or
   newline-separated keys start on the next line, terminated by a new
   set of key markers
*/

static void initKeys(void)
{
	int      c, nleft = KEYROOM, length, place = 0, ibuf;
	int      damaged;
	char     key[KEYLENGTH + 1], *pc;
	char    *buffer;
	WHERE("initKeys");
	
	NKeys = 0;
	mydisphandle(KeyList);
	KeyList = mygethandle(nleft*sizeof(char));
	if (KeyList == (char **) 0)
	{
		goto errorExit;
	}
	
	while (getHelpLine() != 0)
	{ /* loop over lines in help file */
		if (strncmp(*Buffer, KEYSTRING, NMARKERS) == 0 ||
			strncmp(*Buffer, MARKERSTRING, NMARKERS) == 0)
		{
			if ((*Buffer)[0] == MARKER)
			{
				BufferFull = 1;
			}
			break;
		}
		
		ibuf = 0;

		do /* while (c != '\0') */
		{ /* collect keys on current line */
			key[0] = '\0';
			pc = key;
			buffer = *Buffer;
			damaged = 0;
			while ((c = buffer[ibuf++]) != SEPKEYS && c != '\0')
			{
				if (pc - key > KEYLENGTH)
				{
					damaged = 1;
				} /*if (pc - key > KEYLENGTH)*/
				else if (c != ' ' || key[0] != '\0')
				{ /* skip leading spaces*/
					*pc++ = c;
				}
			} /*while ((c = buffer[ibuf++]) != SEPKEYS && c != '\0')*/
			if (damaged)
			{
				if (!Damaged)
				{
					damagedFile();
				}
				Damaged--;
			} /*if (damaged)*/

			pc--;
			while (*pc == ' ' && pc >= key)
			{ /* trim off trailing spaces */
				pc--;
			}
			pc[1] = '\0';
			if (key[0] != '\0')
			{ /* we got a key */
				length = strlen(key);
				if (length + 1 > nleft)
				{ /* we need more room */
					nleft = KEYROOM;
					KeyList = mygrowhandle(KeyList, place + nleft);
					if (KeyList == (char **) 0)
					{
						goto errorExit;
					}
				}
				strcpy(*KeyList + place, key);
				KeyOffsets[NKeys++] = place;
				place += length + 1;
				nleft -= length+1;
				if (NKeys == MAXKEYS)
				{
					break;
				}
			} /*if (key[0] != '\0')*/
		} while (c != '\0');
	} /*while (getHelpLine() != 0)*/
	if (NKeys == 0)
	{
		mydisphandle(KeyList);
		KeyList = (char **) 0;
	}
	else if ((KeyList = mygrowhandle(KeyList, place)) == (char **) 0)
	{
		goto errorExit;
	}
	
	return;
	
  errorExit:
	mydisphandle(KeyList);
	KeyList = (char **) 0;
	NKeys = 0;
} /*initKeys()*/

/* comparison of strings ignoring case */
static long keycmp(long iKey, char * string)
{
	char     *key = *KeyList + KeyOffsets[iKey];
	int       c1, c2;
	int       i;
	int       n = strlen(key);
	WHERE("keycmp");

	for (i = 0;i < n;i++)
	{
		c1 = key[i];
		c2 = string[i];
		c1 = (isupper(c1)) ? tolower(c1) : c1;
		c2 = (isupper(c2)) ? tolower(c2) : c2;
		if (c1 != c2)
		{ /* match if run out of string before we run out of key */
			return ((c2 == '\0') ? 0 : 1);
		}
		else if (c1 == '\0')
		{
			return 0;
		}
	}
	return (0);
} /*keycmp()*/

static void listKeys(void)
{
	short       iKey;
	short       maxlength = 0, length;
	short       nperline, nlines, line;
	char       *outstr;
		
	for (iKey = 0;iKey < NKeys;iKey++)
	{
		length = strlen(*KeyList + KeyOffsets[iKey]);
		maxlength = (length > maxlength) ? length : maxlength;
	}
	maxlength++;
	nperline = SCREENWIDTH/maxlength;
	nperline = (nperline > 0) ? nperline : 1;
	nlines = (NKeys - 1)/nperline + 1;
	sprintf(OUTSTR,
		"Type 'help(key:\"heading\")', where heading is in following list:");
	putOUTSTR();
	for (line = 0;line < nlines;line++)
	{
		outstr = OUTSTR;
		for (iKey = line; iKey < NKeys; iKey += nlines)
		{
			strcpy(outstr, *KeyList + KeyOffsets[iKey]);
			length = strlen(outstr);
			outstr += length;

			if (iKey + nlines <= NKeys)
			{
				while (length++ <= maxlength)
				{
					*outstr++ = ' ';
				}
			}
			*outstr = '\0';
		} /*for (iKey = line; iKey < NKeys; iKey += nlines)*/
		putOUTSTR();
	} /*for (iKey = 0;iKey < NKeys;)*/
} /*listKeys()*/

static void setKeyTable(char *buffer, long n)
{
	unsigned long   *keyTable = *KeyTable + n - 1;
	short            i, length;
	char             key[KEYLENGTH + 1];
	char             c;
	WHERE("setKeyTable");

	*keyTable = 0;
	do /* while (c != '\0')*/
	{ /* loop over keys on line after '#'*/
		while (*buffer == ' ')
		{ /* skip spaces */
			buffer++;
		}
		length = 0;
		while ((c = *buffer++) != SEPKEYS && c != '\0')
		{
			if (length == KEYLENGTH)
			{
				if (Damaged <= 0)
				{
					damagedFile();
					Damaged = abs(Damaged);
				}
				Damaged++;
				return;
			}
			key[length++] = c;
		}
			
		while (length > 0 && key[length-1] == ' ')
		{ /* trim off trailing blanks */
			length--;
		}
		key[length] = '\0';
		
		if (key[0] != '\0')
		{
			for (i = 0;i < NKeys;i++)
			{
				if (keycmp(i, key) == 0)
				{
					*keyTable |= (1L << i);
				}
			}
		} /*if (key[0] != '\0')*/		
	} while (c != '\0');
} /*setKeyTable()*/

static long adjustDate(long date)
{
	if (date < CREATION)
	{
		date += 20*CENTURY;
	}
	else if (date < CENTURY)
	{
		date += 19*CENTURY;
	}
	return (date);
} /*adjustDate()*/

/* Called by help every time, normally does anything only the first time */
#define JUSTOPEN     1
#define OPENANDSCAN  2

/*
   960502 Changed fopen() to fmyopen()
*/
static void initHelp(int justOpen)
{
	char       topic[TOPICNAMELENGTH+1], *keystart;
	long       room = 0, spaceleft = 0, topicsleft = 0;
	long       length, needed;
	WHERE("initHelp");
	
	if (NHelpTopics < 0)
	{
		Damaged = 0;
		if (HelpFileName == (char **) 0)
		{
			HelpFileName = OrigHelpFileName;
			if (HelpFileName == (char **) 0)
			{
				return;
			}
			HelpFile = OrigHelpFile;
		} /*if (HelpFileName == (char **) 0)*/
		
		if (HelpFile == (FILE *) 0)
		{
			HelpFile = fmyopen(*HelpFileName, TEXTREADMODE);
			LastHelpWasCr = 0;
		}

		if (HelpFile == (FILE *) 0)
		{
			sprintf(OUTSTR,
					"WARNING: unable to open help file '%s'",*HelpFileName);
			putErrorOUTSTR();
			goto errorExit;
		}
		else
		{
			if (HelpFileName == AltHelpFileName)
			{
				AltHelpFile = HelpFile;
			}
			
#ifdef USEFSEEK
			LineOffset = 0;
#endif /*USEFSEEK*/
			rewindHelp();
		}
		
		if (justOpen == JUSTOPEN)
		{
			return;
		}
		
		mydisphandle5((char **) TopicOffsets, Topics, Buffer,
					  (char **) KeyTable, (char **) 0);		

		KeyTable = (unsigned long **) 0;

		TopicOffsets = (short **) mygethandle(NTOPICS * sizeof(short));
		Topics = mygethandle(HELPROOM * sizeof(char));
		Buffer = mygethandle(BUFFERLENGTH * sizeof(char));
		if (TopicOffsets == (short **) 0 ||
			Topics == (char **) 0 || Buffer == (char **) 0)
		{
			goto noroom;
		}
#ifdef USEFSEEK
		mydisphandle((char **) FileOffsets);
		FileOffsets = (long **) mygethandle(NTOPICS * sizeof(long));
		if (FileOffsets == (long **) 0)
		{
			goto noroom;
		}
#endif /*USEFSEEK*/

		while (getHelpLine() != 0)
		{
			if (strncmp(*Buffer, KEYSTRING, NMARKERS) == 0)
			{
				initKeys();
				if (NKeys > 0)
				{ /* create table of keys associated with topics*/
					KeyTable = (unsigned long **)
						mygethandle(NTOPICS*sizeof(unsigned long));
					if (KeyTable == (unsigned long **) 0)
					{
						goto noroom;
					}
				}
				break;
			} /*if (strncmp(*Buffer, KEYSTRING, NMARKERS) == 0)*/
			else if (strncmp(*Buffer, MARKERSTRING, NMARKERS) == 0)
			{
				BufferFull = 1;
				break;
			}
		} /*while (getHelpLine() != 0)*/

		topicsleft = NTOPICS;
		spaceleft = HELPROOM;
		if (!BufferFull)
		{
			(*Buffer)[0] = '\0';
		}
		
	    /* find all help topics available */
		NHelpTopics = 0;
		LatestNews = -1;
		OldestNews = 21*CENTURY;

		while (nextTopic(topic) != 0)
		{
			length = strlen(topic);
			if (length + 1 > spaceleft)
			{
				spaceleft = HELPCHUNK;
				Topics = mygrowhandle(Topics,(room + spaceleft)*sizeof(char));
				if (Topics == (char **) 0)
				{
					goto noroom;
				}
			} /*if (length + 1 > spaceleft)*/
					
			if (topicsleft <= 0)
			{
				topicsleft = TOPICCHUNK;
				needed = NHelpTopics+topicsleft;
				TopicOffsets = (short **)
					mygrowhandle((char **) TopicOffsets, needed*sizeof(short));
				if (TopicOffsets == (short **) 0)
				{
					goto noroom;
				}
#ifdef USEFSEEK
				FileOffsets = (long **)
					mygrowhandle((char **) FileOffsets, needed*sizeof(long));
				if (FileOffsets == (long **) 0)
				{
					goto noroom;
				}
#endif /*USEFSEEK*/
				if (NKeys > 0)
				{
					KeyTable = (unsigned long **)
						mygrowhandle((char **) KeyTable,
									 needed*sizeof(unsigned long));
					if (KeyTable == (unsigned long **) 0)
					{
						goto noroom;
					}
				}
				
			} /*if (topicsleft <= 0)*/
					
			strcpy((*Topics) + room, topic);
			(*TopicOffsets)[NHelpTopics] = room;
#ifdef USEFSEEK
			(*FileOffsets)[NHelpTopics] = LineOffset;
#endif /*USEFSEEK*/
			
			NHelpTopics++;
			topicsleft--;
			room += length+1;
			spaceleft -= length+1;
			if (NKeys > 0 &&
				(keystart = strchr(*Buffer, STARTKEYS)) != (char *) 0)
			{ /* retrieve keys for this entry */
				setKeyTable(keystart+1, NHelpTopics);
			}
			BufferFull = 0;
			if (strcmp(topic, NEWS) == 0)
			{
				int        ic;

				while (1)
				{
					while (getHelpLine() != 0 && !isdigit((*Buffer)[0]) &&
						   (*Buffer)[0] != MARKERSTRING[0])
					{
						;
					}
					if ((*Buffer)[0] == '\0' ||
						strncmp(*Buffer, MARKERSTRING, NMARKERS) == 0)
					{
						BufferFull = 1;
						break;
					}
					
					for (ic = 0; ic < DATEDIGITS &&
						   isdigit((*Buffer)[ic]); ic++)
					{
						;
					}
					if (ic == DATEDIGITS || ic == DATEDIGITS - 2)
					{
						long         itemDate;
						
						(*Buffer)[ic] = '\0';
						sscanf(*Buffer, "%ld", &itemDate);
						itemDate = adjustDate(itemDate);
					
						if (itemDate > LatestNews)
						{
							LatestNews = itemDate;
						}
						else if (itemDate < OldestNews)
						{
							OldestNews = itemDate;
						}
					} /*if (ic == DATEDIGITS || ic == DATEDIGITS - 2)*/
					BufferFull = 0;
				} /*while (1)*/ 
			} /*if (strcmp(topic, NEWS) == 0)*/
		} /*while (nextTopic(topic) != 0)*/

		if (Damaged)
		{
			sprintf(OUTSTR,
					"WARNING: %d bad lines were found in file \"%s\"",
					abs(Damaged), *HelpFileName);
			putErrorOUTSTR();
		} /*if (Damaged)*/
		
		rewindHelp();
#ifdef USEFSEEK
		LineOffset = -1; /* signals no need to use ftell()*/
#endif /*USEFSEEK*/
	} /*if (NHelpTopics < 0)*/
	return;
	
  noroom:
	putOutErrorMsg("WARNING: unable to allocate memory needed for help");

  errorExit:
	mydisphandle5((char **) TopicOffsets, Topics, Buffer,
				  (char **) 0, (char **) 0);
	TopicOffsets = (short **) 0;
	Topics = Buffer = (char **) 0;
#ifdef USEFSEEK
	mydisphandle((char **) FileOffsets);
	FileOffsets = (long **) 0;
#endif /*USEFSEEK*/
	NHelpTopics = -1;
	
} /*initHelp() */

/*
	990325 allow file type 'ttro' in addition to 'TEXT' on Macintosh
*/
static long       changeHelpFile(Symbolhandle symh, long action)
{
	char           *tempName;
	long            nameLength;
#ifdef MACINTOSH
	OSType          types[2];
#endif /*MACINTOSH*/
	WHERE("changeHelpFile");
	
	if (action == INEWALTFILE)
	{
		tempName = STRINGPTR(symh);
		if (strlen(tempName) >= PATHSIZE)
		{
			sprintf(OUTSTR, "ERROR: alternate help file name too long");
			goto errorExit;
		}
#ifdef HASFINDFILE
#ifdef MACINTOSH
		types[0] = 'TEXT';
		types[1] = 'ttro';
		tempName = macFindFile(tempName, "\pSpecify the Help File", (STR255) 0,
							   READIT, 2, types, &HelpVolume);
#endif /*MACINTOSH*/

#ifdef WXWIN
		tempName = wxFindFile(tempName, "Specify the Help File", (char *) 0);
#endif /* WXWIN */
		if (tempName == (char *) 0)
		{ /* cancelled */
				goto errorExit;
		}
#endif /*HASFINDFILE*/
		tempName = expandFilename(tempName);
		if (tempName == (char *) 0)
		{
			goto errorExit;
		}

		if (!isfilename(tempName))
		{
			goto errorExit;
		}

		if (HelpFileName == (char **) 0)
		{
			HelpFileName = OrigHelpFileName;
		}
		if (HelpFileName != (char **) 0 &&
			strcmp(tempName, *HelpFileName) == 0)
		{ /* file name is the same as current file; do nothing*/
			return (1);
		}
		
/*
Note: OrigHelpFile is never closed, even when we are using an alternative
	 Similarly, AltHelpFile is not closed when we switch back to
	 OrigHelpFile
*/
		if (HelpFile == OrigHelpFile)
		{
			rewindHelp();
		}
		else
		{
			if (AltHelpFile != (FILE *) 0)
			{
				fclose(AltHelpFile);
			}
			AltHelpFile = (FILE *) 0;
		}
		HelpFile = (FILE *) 0;
		NHelpTopics = -1;

		mydisphandle(AltHelpFileName);
		HelpFileName = AltHelpFileName = (char **) 0;

		nameLength = strlen(tempName);
		AltHelpFileName = mygethandle(nameLength + 1);
		if (AltHelpFileName == (char **) 0)
		{
			goto errorExit;
		}
		strcpy(*AltHelpFileName, tempName);
		HelpFileName = AltHelpFileName;
	} /*if (action == INEWALTFILE)*/
	else
	{
		if (action == IORIGFILE)
		{
			if (HelpFile != OrigHelpFile)
			{
				/* do not close AltHelpFile */
				HelpFile = OrigHelpFile;
				HelpFileName = OrigHelpFileName;
				rewindHelp();
				NHelpTopics = -1;
			} /*if (HelpFile != OrigHelpFile)*/
		} /*if (symh == symhOrig)*/
		else
		{
			if (AltHelpFile == (FILE *) 0)
			{
				sprintf(OUTSTR,
						"ERROR: no alternative help file has been previously specified");
				goto errorExit;
			}
			if (HelpFile != AltHelpFile)
			{
				HelpFile = AltHelpFile;
				HelpFileName = AltHelpFileName;
				rewindHelp();
				NHelpTopics = -1;
			} /*if (HelpFile != AltHelpFile)*/
		} /*if (symh == symhOrig){}else{}*/
	} /*if (action == INEWALTFILE){}else{}*/
	LastHelpWasCr = 0;
	
	return (1);
	
  errorExit:
	putErrorOUTSTR();
	return (0);

} /*changeHelpFile()*/

static long       fillTopiclist(char *topic, short topiclist[], long matchType,
								unsigned long keyMask, short *maxlength)
{
	short           itopic, ntopics = 0, length;
	
	*maxlength = 0;
	for (itopic = 0;itopic < NHelpTopics;itopic++)
	{ /* count topics to be listed */
		if (NKeys > 0 && (*KeyTable)[itopic] & keyMask ||
			matchName(*Topics + (*TopicOffsets)[itopic], matchType,topic))
		{
			length = strlen(*Topics + (*TopicOffsets)[itopic]);
			*maxlength = (length > *maxlength) ? length : *maxlength;
			topiclist[ntopics++] = itopic;
		}
	}
	return (ntopics);
} /*fillTopiclist()*/

static void       listTopics(char *topic, long matchType)
{
	short          nperline, itopic, nlines, ntopics = 0, line;
	short          length, topiclist[500], maxlength = 0;
	unsigned long  keyMask = 0;
	char           *name, *outstr;
	WHERE("listTopics");

	SETUPINT(errorExit);
	
	if (matchType < 0)
	{
		keyMask = (-matchType <= MAXKEYS) ? 1L << -(matchType+1) : 0xffffffff;
	}
	
	initHelp(OPENANDSCAN);
	
	if (NHelpTopics > 0)
	{
		if (matchType == ANYMATCH || matchType == -(MAXKEYS + 1))
		{
			putOutMsg("Help is available on the following topics: ");
		}
		else if (matchType < 0)
		{
			sprintf(OUTSTR,
					"The following help topics concern %s",
					*KeyList + KeyOffsets[-(matchType+1)]);
			putOUTSTR();
		}
	
		ntopics = fillTopiclist(topic, topiclist, matchType,
								keyMask, &maxlength);
		maxlength++;
		nperline = SCREENWIDTH/maxlength;
		if (nperline < 1)
		{
			nperline = 1;
		}
		nlines = (ntopics - 1)/nperline + 1;
			
		for (line = 0;line < nlines;line++)
		{
			outstr = OUTSTR;
			for (itopic = line;itopic < ntopics;itopic += nlines)
			{
				name = *Topics + (*TopicOffsets)[topiclist[itopic]];
				strcpy(outstr, name);
				length = strlen(name);
				outstr += length;
				if (itopic + nlines < ntopics)
				{
					while (length++ < maxlength)
					{
						*outstr++ = ' ';
					}
				}
				*outstr = '\0';
			} /*for (itopic = line;itopic < NHelpTopics;itopic += nlines)*/
			putOUTSTR();
			checkInterrupt(errorExit);
		} /*for (line = 0;line < nlines;line++)*/

		if (matchType == ANYMATCH && HelpFile == OrigHelpFile)
		{
			putOutMsg("For information on help, enter help(help)");
		}
		putOutMsg("For help on topic foo, enter help(foo) or help(\"foo\")");
	} /*if (NHelpTopics > 0)*/
	
	/* fall through */

  errorExit:
	return;
	
} /*listTopics()*/

static char      *ControlNames[] =
{
	"break", "breakall", "else", "elseif", "for",
	"help", "Help", "if", "while", "next",
	(char *) 0
};


/*
  Note: atan() and round() are omitted from this list because they have
  their own entries
*/

#define IANGLEVALUE  10 /*index of "acos"*/
#define IANGLEARG    12 /*index of "cos" */

static char           *TransformationNames[] = 
{
	"abs"    ,"exp"    ,"lgamma" ,"log"    ,"log10"  ,"sqrt"   ,"cosh"   ,
	"sinh"   ,"tanh"   , "atanh" , "acos"  ,"asin"   ,"cos"    ,"sin"    ,
	"tan"    , (char *) 0
};

#define isControl(NAME) matchKey(NAME, ControlNames, (long *) 0)
#define isTransformation(NAME) matchKey(NAME, TransformationNames, (long *) 0)
/*
  981215 Removed code for printing usage for transformations.  Transformation
         Usage is now taken from the help file as it is for all other topics
         Also removed 3rd argument char *topic
*/
static int putHelp(long dates[2], long usage)
{
	long         i, line = 0;
	long         foundDate = (dates[0] < 0) ? -1 : 0;
	long         foundUsage = 0;
	long         printLine = (foundDate < 0);
	long         dateSought = dates[1];
	char        *buffer = *Buffer;
	WHERE("puthelp");

	BufferFull = 0;

	/* now print it out */

	while (1)
	{
		checkInterrupt(errorExit);
		buffer[0] = '\0';
		if (getHelpLine() == 0 ||
			(buffer[0] == MARKER && buffer[1] == MARKER
			 && buffer[2] == MARKER && buffer[3] == MARKER))
		{					/* end of topic found */
			break;
		}

		if (strncmp(buffer, USAGESTRING, 4) == 0)
		{
			if (foundUsage)
			{
				if (usage == PUTUSAGE)
				{
					break;
				}
				printLine = (foundDate < 0);
			}
			else
			{
				foundUsage = 1;
				printLine = (usage == PUTUSAGE) ? 1 : 0;
			}
		} /*if (strncmp(buffer, USAGESTRING, 4) == 0)*/
		else
		{			
			if (foundDate >= 0)
			{/* topic is 'news' */
				for (i = 0;i < DATEDIGITS;i++)
				{
					if (!isdigit(buffer[i]))
					{
						break;
					}
				} /*for (i = 0;i < DATEDIGITS;i++)*/

				if (i == DATEDIGITS || i == DATEDIGITS - 2)
				{/* line starts with date */
					char      datebuf[DATEDIGITS + 1];
					long      thisDate;

					strncpy(datebuf, buffer, i);
					datebuf[i] = '\0';

					sscanf(datebuf, "%ld", &thisDate);
					thisDate = adjustDate(thisDate);

					if (foundDate && thisDate < dateSought)
					{		/* earlier than date[0] */
						break;
					}
					if (!foundDate && thisDate <= dateSought)
					{
						foundDate = 1;
						printLine = 1;
						dateSought = dates[0];
					}
				} /*if (i == DATEDIGITS)*/
			} /*if (foundDate >= 0)*/
			else if (!foundUsage && !printLine)
			{
				printLine = (line > 0);
			}

			if (printLine)
			{
				putOutMsg(buffer);
			}
		} /*if (strncmp(buffer, USAGESTRING, 4) == 0){}else{}*/
		line++;
	} /*while (1)*/
	if (*buffer)
	{
		BufferFull = 1;
	}
	
	return ((foundUsage) ? 1 : -1);

  errorExit:
	*OUTSTR = '\0';
	return (0);
} /*putHelp()*/

/*
   950907 added argument to initHelp() so that file is not scanned on
   calls of form help(file:filename) and help(orig:T).  This change means
   a startup file can set the default help file without the cost of
   scanning it before it's needed

   951212 added entry usage() and modified help file accordingly.  If
   the first line of a help entry starts with %%%%, everything up to a closing
   %%%% is considered information to be printed by usage() but not by help()
   If there is no %%%% marker, usage prints the same as help.

   For transformations in the list TransformationNames, usage() prints
   a generic message, while help() prints help for topic "transformations"

   960426 Made one of file:fileName, orig:T and alt:T legal with key:"Key"
   Also changeHelpFile() so there shouldn't be any unnecessary scans of
   the file.

   960723 Changed code so that 'help()' gives a short message but does not
   list all the topics.  'help("*")' gives all topics.  Also changed the
   "teaser" message to include 'help("*")' usage.

   970223 Fixed bug that could cause crash with too long help topics

   970808 Fixed bug so that now help(file:filename, "*") just lists topic
   names instead of all help
*/
#define MATCHTYPEOFFSET   32 /* also defined in utils.c */
Symbolhandle    help(Symbolhandle list)
{
	Symbolhandle    symh;
	Symbolhandle    symhFile = (Symbolhandle) 0, symhOrig = (Symbolhandle) 0;
	Symbolhandle    symhAlt = (Symbolhandle) 0;
	Symbolhandle    symhKey = (Symbolhandle) 0;
	long            doWhat = 
		(strcmp(FUNCNAME, "help") == 0) ? PUTHELP : PUTUSAGE;
	long            iTopic, nTopics = 0, i, karg;
	long            nActions = 0, action = 0, actionArg = -1;
	long            type;
	long            dates[2];
	char           *name;
	double          val[2], tmp;
	long            nargs = NARGS(list), start = -1, ndates;
	long            nargsOrig = nargs;
	long            matchType = 0;
	char           *argTopic, *what, *keyword = (char *) 0;
	char           *wantedTopic;
	char            target[TOPICNAMELENGTH+1], topic[2*TOPICNAMELENGTH+1];
	int             doneTransformation = 0, reply;
	int             doneNull = 0, doneLogic = 0, doneNumber = 0;
	char           *logicName = "logic", *numberName = "number";
	char           *tranName = "transformations";
#ifdef SCROLLABLEWINDOW
	Symbolhandle    symhScrollback = (Symbolhandle) 0;
	int             scrollback = CURRENTSCROLLBACK;
#endif /*SCROLLABLEWINDOW*/
	WHERE("help");
	
	/* NHelpTopics and TopicOffsets are initialized by initHelp() */
	symh = COMPVALUE(list, 0);
	if (nargs == 1 &&
		(symh == (Symbolhandle) 0 ||
		 TYPE(symh) == CHAR && strcmp(STRINGPTR(symh),"__teaser__") == 0))
	{
		for (i = 0; HelpMsg[i] != (char *) 0; i++)
		{
			putOutMsg(HelpMsg[i]);
		}
		return (NULLSYMBOL);
	}

	keyword = isKeyword(symh);
	if (keyword)
	{
		if (strncmp(keyword,"key",3) == 0)
		{
			symhKey = symh;
		}
		else if (strcmp(keyword,"file") == 0)
		{
			symhFile = symh;
			nActions++;
		}
		else if (strncmp(keyword,"orig", 4) == 0)
		{
			symhOrig = symh;
			nActions++;
		}
		else if (strncmp(keyword,"alt", 3) == 0)
		{
			symhAlt = symh;
			nActions++;
		}
	} /*if (keyword)*/
	if (nActions > 0)
	{
		actionArg = 0;
	}
	
	if (nargs > 1 && (keyword = isKeyword(symh = COMPVALUE(list,nargs-1))))
	{
		if (strncmp(keyword,"key",3) == 0)
		{
			if (symhKey != (Symbolhandle) 0)
			{
				goto duplicateKey;
			}
			nargs--;
			symhKey = symh;
		}
		else if (strcmp(keyword,"file") == 0)
		{
			if (symhFile != (Symbolhandle) 0)
			{
				goto duplicateKey;
			}
			actionArg = --nargs;
			symhFile = symh;
			nActions++;
		}
		else if (strncmp(keyword,"orig", 4) == 0)
		{
			if (symhOrig != (Symbolhandle) 0)
			{
				goto duplicateKey;
			}
			actionArg = --nargs;
			symhOrig = symh;
			nActions++;
		}
		else if (strncmp(keyword,"alt", 3) == 0)
		{
			if (symhAlt != (Symbolhandle) 0)
			{
				goto duplicateKey;
			}
			actionArg = --nargs;
			symhAlt = symh;
			nActions++;
		}
		else if (strcmp(keyword,"scrollback") == 0)
		{
#ifdef SCROLLABLEWINDOW
			symhScrollback = symh;
#else /*SCROLLABLEWINDOW*/
			sprintf(OUTSTR,
					"WARNING: %s() keyword '%s' ignored in this version",
					FUNCNAME, keyword);
			putErrorOUTSTR();
#endif /*SCROLLABLEWINDOW*/
			nargs--;
		}
	} /*if (nargs > 1 && (keyword = isKeyword(symh = COMPVALUE(list,nargs-1)))*/
	if (nActions > 1)
	{
		sprintf(OUTSTR,
				"ERROR: you cannot use more than 1 of 'file', 'orig', or 'alt' with %s()",
				FUNCNAME);
	} /*if (nActions > 1)*/
	else if (symhKey != (Symbolhandle) 0)
	{
		if (!isCharOrString(symhKey))
		{
			notCharOrString(keyword);
			goto errorExit;
		}
		if (nargsOrig > nActions + 1)
		{
			sprintf(OUTSTR,
					"ERROR: with key:\"keyWord\" you can only use file:fileName, orig:T or alt:T");
		}
	}
	
	if (*OUTSTR != '\0')
	{
		goto errorExit;
	}

#ifdef SCROLLABLEWINDOW
	if (symhScrollback != (Symbolhandle) 0)
	{
		if (!isTorF(symhScrollback))
		{
			notTorF(isKeyword(symhScrollback));
			goto errorExit;
		}
		scrollback = DATAVALUE(symhScrollback, 0) != 0.0;
	} /*if (symhScrollback != (Symbolhandle) 0)*/
#endif /*SCROLLABLEWINDOW*/
	
	if (nActions > 0)
	{ /* one of file:"filename", alt:T, or orig:T */
		if (symhFile != (Symbolhandle) 0)
		{ /* file:"filename", sets new help file as alternate*/
			symh = symhFile;
			keyword = isKeyword(symh);
			action = INEWALTFILE;
			if (!isCharOrString(symh))
			{
#if defined(MACINTOSH) || defined(WXWIN)
				sprintf(OUTSTR,
						"ERROR: usage is %s(%s:\"fileName\") or %s(%s:\"\")",
						FUNCNAME, keyword, FUNCNAME, keyword);
#else /*MACINTOSH || WXWIN*/
				sprintf(OUTSTR,
						"ERROR: usage is %s(%s:\"fileName\")",
						FUNCNAME, keyword);
#endif /*MACINTOSH || WXWIN*/
				goto errorExit;
			} /*if (!isCharOrString(symh))*/
		} /*if (symhFile != (Symbolhandle) 0)*/
		else
		{
			if (symhAlt != (Symbolhandle) 0)
			{ /* alt:T means use file most recent file set by file:fileName	*/
				action = IALTFILE;
				symh = symhAlt;
			}
			else
			{ /* orig:T means use help file set at startup */
				action = IORIGFILE;
				symh = symhOrig;
			}
			keyword = isKeyword(symh);
			if (!isTorF(symh))
			{
				notTorF(keyword);
				goto errorExit;
			}
			if (DATAVALUE(symh, 0) == 0.0)
			{
				sprintf(OUTSTR,
						"ERROR: value for %s must be T", keyword);
				goto errorExit;
			}
		} /*if (symhFile != (Symbolhandle) 0){}else{}*/

		if (!changeHelpFile(symh, action))
		{
			goto errorExit;
		}
		setNAME(symh, USEDKEYWORD);

		if (nargsOrig == 1)
		{ /* open Help file but don't read it */
			initHelp(JUSTOPEN);
			if (HelpFile == (FILE *) 0)
			{
				goto notAvailable;
			}
			goto normalExit;
		}
	} /*if (nActions > 0)*/	

	initHelp(OPENANDSCAN);

	if (HelpFileName == (char **) 0)
	{
		goto errorExit;
	}
	if (NHelpTopics <= 0)
	{
		goto notAvailable;
	}
	
	if (symhKey != (Symbolhandle) 0)
	{
		setNAME(symh, USEDKEYWORD);
		if (NKeys <= 0)
		{
			sprintf(OUTSTR,
					"ERROR: no keys used in help file");
			goto errorExit;
		}
			
		if (strcmp("all", STRINGPTR(symhKey)) == 0)
		{						/* list all topics */
			matchType = -(MAXKEYS + 1);
			i = 0;
		}
		else if (STRINGVALUE(symhKey, 0) == '?')
		{
			i = NKeys + 1;
		}
		else
		{
			for (i = 0;i < NKeys;i++)
			{
				if (keycmp(i, STRINGPTR(symhKey)) == 0)
				{
					matchType = -(i + 1);
					break;
				}
			} /*for (i = 0;i < NKeys;i++)*/
		}
			
		if (i >= NKeys)
		{/* list keys */
			if (i == NKeys)
			{
				putPieces("ERROR: Key ", STRINGPTR(symhKey),
						  " does not match any key in the help file",
						  (char *) 0);
			}
			listKeys();
			if (i == NKeys)
			{
				goto errorExit;
			}
			goto normalExit;
		} /*if (i >= NKeys)*/
	} /*if (symhKey != (Symbolhandle) 0)*/
	
	symh = COMPVALUE(list, (actionArg == 0 && nargs == 2) ? 1 : 0);

	if ((nargs == 1 || nargs == 2 && actionArg >= 0) &&
		matchType == 0 && !isKeyword(symh) &&
		isCharOrString(symh) && isscratch(NAME(symh)) &&
		strlen(name = STRINGPTR(symh)) <= NAMELENGTH)
	{ /* if single quoted character string as argument*/
		if (!isControl(name))
		{/* not syntax element such as 'break', or 'help' */
			if (!scanPat(name,&matchType,target, topicNameCheck,
						 TOPICNAMELENGTH))
			{
				sprintf(OUTSTR,
						"ERROR: improper %s() topic '%s'",
						FUNCNAME, STRINGPTR(symh));
				goto errorExit;
			}

			for (i = 0;i < NHelpTopics;i++)
			{/* count the number of matches */
				name = *Topics + (*TopicOffsets)[i];
				if (matchName(name,matchType,target))
				{
					nTopics++;
				}
			}
		} /*if (!isControl(name))*/
		else
		{
			matchType = EXACTMATCH;
			nTopics = 1;
		}
	} /* if (single quoted character string as argument)*/

	if (nTopics > 1 || matchType < 0)
	{ /* 1 argument and more than one topic matched, list them */
		listTopics(target, matchType);
		checkInterrupt(errorExit);
	} /*if (nTopics > 1 || matchType < 0)*/
	else
	{ /* otherwise, print help for each argument*/
		for (karg = 0; karg < nargs; karg++)
		{
			dates[0] = dates[1] = -1;
			ndates = 0;
			symh = COMPVALUE(list,karg);
			if (symh == (Symbolhandle) 0)
			{
				putOutErrorMsg("WARNING: empty argument to help()");
				continue;
			}
			name = NAME(symh);
			if (strcmp(name, USEDKEYWORD) == 0)
			{
				continue;
			}
			type = TYPE(symh);
			matchType = 0;

			if ((keyword = isKeyword(symh)) && strcmp(keyword,NEWS) == 0)
			{
				/*
				  news:yymmdd
				  news:vector(yymmdd1, yymmdd2)
				  news:yyyymmdd
				  news:vector(yyyymmdd1, yyyymmdd2)
				  Immediately translate a date of the form yymmdd to yyyymmdd
				*/
				ndates = symbolSize(symh);
				if (type == REAL && isVector(symh) && ndates <= 2)
				{
					val[0] = DATAVALUE(symh, 0);
					val[1] = (ndates == 2) ?
					  DATAVALUE(symh, 1) : (double) LatestNews;
					for (i = 0; i < 2; i++)
					{						
						if (val[i] != floor(val[i]) || val[i] < 0)
						{
							val[0] = val[1] = -1;
							break;
						}
						val[i] = (double) adjustDate((long) val[i]);
						
						if (val[i] < OldestNews)
						{
							val[i] = OldestNews;
						}
						else if (val[i] > LatestNews)
						{
							val[i] = LatestNews;
						}
					} /*for (i = 0; i < 2; i++)*/
					
					if (val[1] < val[0])
					{
						tmp = val[0];
						val[0] = val[1];
						val[1] = tmp;
					}
				} /*if (type == REAL && isVector(symh) && ndates <= 2)*/
				else
				{
					val[0] = -1.0;
				}
				
				if (val[0] < 0.0)
				{
					sprintf(OUTSTR,
							"WARNING: value of '%s' not 1 or 2 dates of form yymmdd or yyyymmdd; ignored",
							keyword);
					putErrorOUTSTR();
				} /*if (val[0] < 0.0)*/
				for (i = 0;i < 2; i++)
				{
					dates[i] = (long) val[i];
				}
				wantedTopic = argTopic = NEWS;
			} /*if ((keyword = isKeyword(symh)) && strcmp(keyword,NEWS) == 0)*/
			else if (keyword != (char *) 0 && !GUBED)
			{
				sprintf(OUTSTR,
						"WARNING: keyword %s not recognized by %s()",
						keyword, FUNCNAME);
				putErrorOUTSTR();
				continue;
			}
			else if (isCharOrString(symh) && isscratch(name))
			{ /* quoted string */
				wantedTopic = argTopic = STRINGPTR(symh);
				if (strlen(argTopic) > TOPICNAMELENGTH)
				{
					sprintf(OUTSTR,
							"WARNING: \"%s\" is too long a %s() topic name",
							argTopic, FUNCNAME);
					putErrorOUTSTR();
					continue;
				} /*if (strlen(argTopic) > TOPICNAMELENGTH)*/

				if (!isControl(argTopic) &&
					scanPat(argTopic, &matchType, target,
							topicNameCheck, TOPICNAMELENGTH))
				{ /* nTopics < 0 means 1st arg is syntax element */
					argTopic = target;
				}
				else
				{
					matchType = EXACTMATCH;
				}
			}
			else if (isscratch(name))
			{
				if (strcmp(name, NULLSCRATCH) == 0)
				{ /* Topic NULL */
					if (doneNull)
					{
						continue;
					}
					doneNull = 1;
					wantedTopic = argTopic = NULLNAME;
				}
				else if (strcmp(name, LOGICSCRATCH) == 0)
				{ /*help(T) or help(F)*/
					if (doneLogic)
					{
						continue;
					}
					doneLogic = 1;
					wantedTopic = (DATAVALUE(symh, 0)) ? "T" : "F";
					argTopic = logicName;
				}
				else if (strcmp(name, NUMSCRATCH) == 0)
				{ /*e.g. help(3.12) or help(?)*/
					if (doneNumber)
					{
						continue;
					}
					doneNumber = 1;
					wantedTopic = (!isMissing(DATAVALUE(symh,0))) ?
					  NUMSCRATCH + 2 : "?";
					argTopic = numberName;
				}
				else
				{
					sprintf(OUTSTR,
							"WARNING: improper argument to %s()", FUNCNAME);
					putErrorOUTSTR();
					continue;
				}
			}
			else
			{
				wantedTopic = argTopic = NAME(symh);
			}

			if (strcmp(argTopic, NEWS) == 0 && ndates == 0 && LatestNews > 0)
			{
				long      threeMonthsEarlier = (LatestNews % 10000 >= 400) ?
				  LatestNews - 300 : LatestNews - 9100;
				
				/*news with no dates supplied*/

				dates[0] = threeMonthsEarlier;
				dates[1] = LatestNews;
			} /*if (strcmp(argTopic, NEWS) == 0 && ndates == 0 && LatestNews > 0)*/

			if (start < 0)
			{
				start = karg;
			}

			if (matchType >= MATCHTYPEOFFSET)
			{
				int     nPieces = (matchType >= MATCHTYPEOFFSET) ?
				  matchType/MATCHTYPEOFFSET : 1;

				copyStrings(argTopic, topic, nPieces);
			}
			else if (doWhat != PUTUSAGE && isTransformation(argTopic))
			{
				strcpy(topic, tranName);
			}
			else
			{
				strcpy(topic, argTopic);
			}
			matchType = (matchType == 0) ? EXACTMATCH : matchType;
			if (matchType != EXACTMATCH)
			{
				rewindHelp();
			}

			for (iTopic = 0; iTopic < NHelpTopics; iTopic++)	
			{ /* look for first matching topic name */
				if (matchName(*Topics + (*TopicOffsets)[iTopic], matchType,
							  topic))
				{
					strcpy(target, *Topics + (*TopicOffsets)[iTopic]);
					break;
				}
			} /*for (iTopic = 0; iTopic < NHelpTopics; iTopic++)*/
			
			if (strcmp(target, tranName) != 0 || !doneTransformation++)
			{
				if (iTopic >= NHelpTopics)
				{
					if (type == BLTIN)
					{
						what = "function";
					}
					else if (type == MACRO)
					{
						what = "macro";
					}
					else
					{
						what = "topic";
					}
				
					sprintf(OUTSTR, "WARNING: no help on %s '%s'",
							what, wantedTopic);
					putErrorOUTSTR();
				} /*if (iTopic >= NHelpTopics)*/
				else
				{
#ifdef USEFSEEK
					fseek(HelpFile, (*FileOffsets)[iTopic], 0);
					BufferFull = 0;
					HelpEofHit = 0;
#else  /*USEFSEEK*/
					if (iTopic <= CurrentTopic)
					{
						rewindHelp();
					}
#endif /*USEFSEEK*/

					while (nextTopic(topic) != 0)
					{
						if (strcmp(topic,target) == 0)
						{ /* found wanted topic */
							if (nargs - start > 1)
							{
								if (doWhat == PUTHELP)
								{
									sprintf(OUTSTR,"%s %s:",
											"Help on", argTopic);
									putOUTSTR();
								}
								else if (karg > start)
								{ /* put out blank line before usage*/
									myeol();
								}
							} /*if (nargs - start > 1)*/
							reply = putHelp(dates, doWhat);
							if (!reply)
							{
								goto errorExit;
							}
							break;
						} /*if (strcmp(topic,target) == 0)*/
						BufferFull = 0;
					} /*while (nextTopic(topic) != 0)*/

					if (topic[0] == '\0')
					{
						rewindHelp();
					}
					else
					{
						CurrentTopic = iTopic;
					}
				}/*if (iTopic >= NHelpTopics){}else{}*/
			} /*if (strcmp(target, tranName) != 0 || !doneTransformation++)*/
		} /*for (karg = start;karg < nargs;karg++)*/
	} /*if (nTopics > 1 || matchType < 0){}else{}*/

  normalExit:

#ifdef SCROLLABLEWINDOW
  	CURRENTSCROLLBACK = scrollback;
#endif /*SCROLLABLEWINDOW*/

	return (NULLSYMBOL);

  notAvailable:
	sprintf(OUTSTR,"WARNING: Help is not available");
	goto errorExit;

  duplicateKey:
	sprintf(OUTSTR, "ERROR: duplicate use of %s in %s()", keyword, FUNCNAME);
	goto errorExit;

  errorExit:
	putErrorOUTSTR();

	return (0);
} /*help()*/


static void putComments(Symbolhandle macroSymh, int printName)
{
	int        foundComment;
	char      *outstr = OUTSTR, c;
	char      *name = NAME(macroSymh), *text = STRINGPTR(macroSymh);

	if (isscratch(name))
	{
		name += 2;
	}
	
	if (printName)
	{
		sprintf(OUTSTR, "%s:", name);
		putOUTSTR();
	}
	foundComment = 0;
	while (*text != '\0')
	{
		/* print to output all lines in macro starting with '#' */
		if (*text != '#')
		{/* skip lines that do not start with '#' */
			while (*text != '\n' && *text != '\0')
			{
				text++;
			}
		} /*if (*text != '#')*/
		else
		{
			foundComment = 1;
			outstr = OUTSTR;
			*outstr++ = ' ';
			while (*text != '\n' && *text != '\0')
			{
				if (outstr - OUTSTR >= BUFFERLENGTH - NAMELENGTH - 1)
				{
					*outstr = '\0';
					myprint(OUTSTR);
					*OUTSTR = '\0';
					outstr = OUTSTR;
				}
				c = *text++;
				if (c == '$' && text[0] == 'S' && !isnamechar(text[1]))
				{ /* expand $S */
					text++;
					strcpy(outstr, name);
					outstr += strlen(outstr);
				}
				else
				{
					*outstr++ = c;
				}
			} /*while (*text != '\n' && *text != '\0')*/
			*outstr = '\0';
			putOUTSTR();
		} /*if (*text != '#'){}else{}*/
		if (*text == '\n')
		{
			text++;
		}
	} /*while (*text != '\0')*/

	if (!foundComment)
	{
		putOutMsg(" (no comment lines found)");
	}
	*OUTSTR = '\0';
} /*putComments()*/

/*
   Function to list comments in macros

   usage:  macrousage(macro1 [, macro2 ...])
           macrousage(charVec)

   981203 Fix of minor bug in argument checking
*/

Symbolhandle macrousage(Symbolhandle list)
{
	Symbolhandle       arg;
	long               nargs = NARGS(list), n;
	long               i, type;
	char              *name;
	WHERE("macrousage");

	*OUTSTR = '\0';
	
	for (i = 0; i < nargs; i++)
	{
		arg = COMPVALUE(list, i);
		if (!argOK(arg, 0, (nargs > 1) ? i + 1 : 0))
		{
			goto errorExit;
		}
		type = TYPE(arg);
		
		if (nargs > 1 && type == CHAR)
		{
			sprintf(OUTSTR,
					"ERROR: when %s() has 2 or more arguments, all must be macros",
					FUNCNAME);
			goto errorExit;
		}
		if (type != MACRO && (i > 0 || type != CHAR))
		{
			badType(FUNCNAME, (i == 0 && nargs == 1) ? -type : MACRO,
					(nargs == 1) ? 0 : i + 1);
			goto errorExit;
		}
	} /*for (i = 0; i < nargs; i++)*/
	
	if (type == CHAR)
	{
		if (!isVector(arg))
		{
			sprintf(OUTSTR,
					"ERROR: CHARACTER argument to %s() must be quoted string or vector",
					FUNCNAME);
			goto errorExit;
		} /*if (!isVector(arg))*/
		n = symbolSize(arg);
		name = STRINGPTR(arg);
		for (i = 0; i < n; i++)
		{
			if (strlen(name) > NAMELENGTH)
			{
				sprintf(OUTSTR,
						"WARNING: '%s' has more than %ld characters; ignored",
						name, (long) NAMELENGTH);
			}
			else if (*name == '\0')
			{
				sprintf(OUTSTR,
						"WARNING: empty string in argument to %s()",FUNCNAME);
			}
			else
			{
				arg = Lookup(name);
				if (arg == (Symbolhandle) 0)
				{
					sprintf(OUTSTR, "WARNING: %s is not defined", name);
				}
				else if (TYPE(arg) != MACRO)
				{
					sprintf(OUTSTR, "WARNING: %s is not a MACRO", name);
				}
				else
				{
					putComments(arg, n > 1);
				}
			}
			putOUTSTR();
			name = skipStrings(name, 1);
		} /*for (i = 0; i < n; i++)*/
	} /*if (type == CHAR)*/
	else
	{
		for (i = 0; i < nargs; i++)
		{
			arg = COMPVALUE(list, i);
			putComments(arg, nargs > 1);
		} /*for (i = 0; i < nargs; i++)*/		
	} /*if (type == CHAR){}else{}*/
	
				
	return (NULLSYMBOL);
	
  errorExit:
	putErrorOUTSTR();
	return (0);
} /* macrousage()*/
