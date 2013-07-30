/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 4.07 or later
*(C)*
*(C)* Copyright (c) 1998 by Gary Oehlert and Christopher Bingham
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

#ifndef MVHELPH__
#define MVHELPH__

/*
	980722 created by moving stuff related to help from globkb.h
	       in preparation for improving help.  It was known for
           a few days as helpkb.h
*/

#undef EXTERN
#undef INIT
#undef INITDIM
#undef INITARRAY
#undef COMMA

#ifdef HELP__
#define EXTERN
#define INIT(VAL) = VAL
#define INITDIM(N) N
#define INITARRAY(VALS) = { VALS }
#else /*HELP__*/
#define EXTERN extern
#define INIT(VAL)
#define INITDIM(N)
#define INITARRAY(VALS)
#endif /*HELP__*/

#define COMMA ,

#ifndef HELPNAME
#if defined(MSDOS)
#define HELPNAME "MACANOVA.HLP"
#elif defined(MACINTOSH)
#define HELPNAME "MacAnova.hlp"
#else
#define HELPNAME "macanova.hlp"
#endif
#endif /*HELPNAME*/

#define                   MARKER '='
#define NMARKERS          4   /* help topics start with NMARKERS MARKER's */
/* Both of the following strings should be NMARKERS long */
#define MARKERSTRING      "===="
#define USAGESTRING       "%%%%"
#define KEYSTRING         "????"
#define STARTKEYS         '#'
#define SEPKEYS           ','
#define PUTUSAGE          1
#define PUTHELP           0
#define EOFSTRINGLENGTH   7
#define EOFSTRING         "_E_O_F_"
#define EOFSTRINGSTART    '_'

enum helpDefines
{
	NTOPICS         =  250, /* initial number of topics */
	HELPROOM        = 2000, /* initial space for list of topics */
	KEYROOM         =  400,  /* initial space for list of keys */
	KEYLENGTH       =   50,
	MAXKEYS         =   32, /* maximum number of keys allowed per file */
	TOPICNAMELENGTH =   20,
	TOPICCHUNK      =   10,
	HELPCHUNK       = (7*TOPICCHUNK),
	MAXHELPFILES    =    2, /* maximum number of active help files */
	HELPBUFFERLENGTH = 150
};

/*
   Brief help message produced by 'help', 'help()', and selecting
   Help on a menu.  MacAnova no longer lists all topics on 'help()'.
   For that the user can use 'help("*")'
*/
/*
	MPW 3.2 pre-processor chokes on using INITARRAY() to initialize HelpMsg[]
	So we don't use it at all
*/
#ifdef HELP__
char                  *HelpMsg[] =
{
	"Type 'help(foo)' for help on topic foo",
	"Type 'usage(foo)' for very brief information on topic foo",
	"Type 'help(\"*\")' for a list of all topics",
	"Type 'help(key:\"?\")' for a list of keys to topics",
	"Type 'help(help)' for more information about help().",
	"Type 'help(usage)' for more information about usage().",
	"Some general topics are",
	" arithmetic   files        launching    models       syntax",
	" assignment   glm          logic        notes        time_series",
	" clipboard    graphs       macanova     NULL         variables",
	" comments     graph_files  macros       number       vectors",
	" complex      graph_keys   macro_files  options      workspace",
	" customize    graph_ticks  macro_syntax quitting",
	" data_files   keywords     matrices     structures",
	" design       labels       memory       subscripts",
	(char *) 0
};
#else /*HELP__*/
extern char                  *HelpMsg[];
#endif /*HELP__*/

#if (0)
typedef struct helpInfo
{
	char               *helpFileName;
	FILE               *helpFile;
	char              **topics;
	short             **topicOffsets;      /* offsets in topics */
	unsigned long     **keyTable;          /*Help key info */
	char              **keyList;           /* list of help keys */
	short               nHelpTopics;       /* < 0 means initHelp not called */
	short               nKeys;
	short               keyOffsets[MAXKEYS];
#ifdef USEFSEEK
	long              **fileOffsets = (long **) 0; /* positions in file */
	long                lineOffset = 0;
#endif /*USEFSEEK*/
	char              **buffer;
	long                latestNews;
	long                oldestNews;
	short               bufferFull;
	short               currentTopic;
	short               eofHit;
	short               lastWasCr;
	int                 damaged;
} helpInfo, *helpInfoPtr, **helpInfoHandle;

#ifdef HELP__
helpInfo                  HelpInfoBlocks[MAXHELPFILES];
helpInfoPtr               HelpInfo[MAXHELPFILES];
short                     CurrentHelp = -1;
short                     AlternateHelp = -1;
#endif /*HELP__*/

#define CurrentHelpInfo   HelpInfo[CurrentHelp]
#define AlternateHelpInfo HelpInfo[AlternateHelp]

#define HelpFileName      CurrentHelpInfo.helpFileName
#define HelpFile          CurrentHelpInfo.helpFile
#define Topics            CurrentHelpInfo.topics
#define TopicOffsets      CurrentHelpInfo.topicOffsets
#define KeyTable          CurrentHelpInfo.keyTable
#define KeyList           CurrentHelpInfo.keyList
#define NHelpTopics       CurrentHelpInfo.nHelpTopics
#define NKeys             CurrentHelpInfo.nKeys
#define KeyOffsets        CurrentHelpInfo.keyOffsets

#ifdef USEFSEEK
#define FileOffsets       CurrentHelpInfo.fileOffsets
#define LineOffset        CurrentHelpInfo.lineOffset
#endif /*USEFSEEK*/

#define Buffer            CurrentHelpInfo.buffer
#define LatestNews        CurrentHelpInfo.latestNews
#define OldestNews        CurrentHelpInfo.oldestNews
#define BufferFull        CurrentHelpInfo.bufferFull
#define CurrentTopic      CurrentHelpInfo.currentTopic
#define HelpEofHit        CurrentHelpInfo.eofHit
#define LastHelpWasCr     CurrentHelpInfo.lastWasCr
#define Damaged           CurrentHelpInfo.damaged

#define AltHelpFileName   AlternateHelpInfo.helpFileName
#define AltHelpFile       AlternateHelpInfo.helpFile

#else /*1*/ /* preceding replaced the following */
#ifdef HELP__
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
#endif /*HELP__*/
#endif /*0*/

/*
	Offsets in file are coded as offset + fileNumber*FILEOFFSETFACTOR
	so actual offset is (*FileOffsets)[i] & FILEOFFSETMASK

	The following allows for helpfiles up to 16777215 bytes
*/
#define FILEOFFSETSHIFT     24
#define FILEOFFSETFACTOR    0x0001000000   /* 1 << FILEOFFSETSHIFT */
#define FILEOFFSETMASK      0x0000ffffff

#define CREATION    870000  /*MacAnova started in 1987*/
#define CENTURY    1000000  /*100 years*/
#define DATEDIGITS       8  /*strlen("19971231")*/

#undef	EXTERN
#undef	INIT
#undef	INITDIM
#undef	INITARRAY
#undef  COMMA

#endif /*MVHELPH__*/
