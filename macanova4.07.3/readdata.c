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

#include "globals.h"
#include "keywords.h"

#ifdef MACINTOSH
#include "macIface.h"
#endif

#ifdef WXWIN
#include "wx/wxIface.h"
#endif  /* WXWIN */

#define READDATA__
#include "matdat.h"

#ifdef CONSOLENAME
#define ISCONSOLE(NAME) (strcmp(NAME, CONSOLENAME) == 0)
#define CONSOLEOK
#else /*CONSOLENAME*/
#define ISCONSOLE(NAME)  0
#define CONSOLENAME ""
#endif /*CONSOLENAME*/

/*
  Usage of functions
	  matread(fileName, matName) [matName may be "" or missing]
      macroread(fileName, matName)
      inforead(fileName, matName)
	  vecread(fileName) or vecread(fileName, character:T)

	  fileName may be of form string:CharVec or file:CharScalar, in which
	  case it need not be first argument.
	  If matName is omitted on matread(), macroread(), or inforead(),
	  the first non-blank line in the file is assumed to be a header for
	  a macro or data set in the right format.

  In windowed versions, if fileName is "", the user selects the file using the
    standard file dialog box.

    it assumes the file starts with a matrix or macro in the right format.

  Possible other keyword phrases
   skip:"P"    P punctuation, not'+', '-', ',', '.' or '*' (e.g. '#'),
               skips any lines beginning with P (vecread only)
   stop:"S"    S punctuation, not'+', '-', ',', '.', or '*' (default '!');
               read will be terminated on first occurence of S (vecread
               only)
   go:"G"      G any character; read will be terminated by first line that
               does not have G in column 1 (vecread only); incompatible
               with stop
   quiet:T     Do not echo header lines ({mat,macro,info}read)
   quiet:F     Echo "skipped" lines (vecread)
   echo:T      Echo any lines after the header lines ({mat,macro,info}read)
               Echo any data lines (vecread)
   silent:T    Echo nothing and suppress warning messages; incompatible with
               echo:T or quiet:F
   notfoundok:T No message and NULL returned if data set or macro not found
               ({mat,macro,info}read)
   nofileok:T  No message and NULL returned if file can't be opened or is empty
   980924 changed opcode ICOMPREAD to fix bug
   981208 fixed bug that kept files from being closed
   990212 Changed most uses of putOUTSTR() to putErrorOUTSTR()
          Replaced myerrorout() with putOutErrorMsg() and putOutMsg()
*/

enum readOpCodes
{
	IVECREAD   = 0x01,
	IMATREAD   = 0x02,
	IMACROREAD = 0x04,
	IREAD      = IMATREAD | IMACROREAD,
	IINFOREAD  = 0x08,
	ICOMPREAD  = 0x10 | IREAD,
	IMACROSEARCH = 0x20 | IMACROREAD,
	IALLREADS  = IVECREAD | IMATREAD | IMACROREAD | IINFOREAD
};


enum readdataKeyCodes
{
	KFILE = 0,
	KSTRING,
	KECHO,
	KQUIET,
	KSILENT,
	KPROMPT,
	KLABELS,
	KSTOP,
	KGO,
	KSKIP,
	KCHAR,
	KBYWORDS,
	KBYCHARS,
	KBYLINES,
	KBADVAL,
	KNOTFOUNDOK,
	KNOFILEOK
};

static keywordList ReaddataKeys[] = 
{
	InitKeyEntry("file",0,IALLREADS,CHARSCALAR),
	InitKeyEntry("string",0,IALLREADS,CHARVECTOR),
	InitKeyEntry("echo",0,IALLREADS,LOGICSCALAR),
	InitKeyEntry("quiet",0,IALLREADS,LOGICSCALAR),
	InitKeyEntry("silent",4,IALLREADS,LOGICSCALAR),
	InitKeyEntry("prompt",0,IALLREADS,LOGICSCALAR),
	InitKeyEntry("labels",5,IMATREAD,SYMHVALUE),
	InitKeyEntry("stop",0,IVECREAD,CHARSCALAR),
	InitKeyEntry("go",0,IVECREAD,CHARSCALAR),
	InitKeyEntry("skip",0,IVECREAD,CHARSCALAR),
	InitKeyEntry("character",4,IVECREAD,LOGICSCALAR),
	InitKeyEntry("bywords",6,IVECREAD,LOGICSCALAR),
	InitKeyEntry("bychars",6,IVECREAD,LOGICSCALAR),
	InitKeyEntry("bylines",6,IVECREAD,LOGICSCALAR),
	InitKeyEntry("badvalue",3,IVECREAD,REALSCALAR),
	InitKeyEntry("notfoundok",0,IMATREAD|IMACROREAD|IINFOREAD,LOGICSCALAR),
	InitKeyEntry("nofileok",0,IALLREADS,LOGICSCALAR)
};

#define Keys       ReaddataKeys

#define File       (KeyCharValue(ReaddataKeys,KFILE))
#define String     (KeySymhValue(ReaddataKeys,KSTRING))
#define Echo       (KeyLogValue(ReaddataKeys,KECHO))
#define Quiet      (KeyLogValue(ReaddataKeys,KQUIET))
#define Labels     (KeySymhValue(ReaddataKeys,KLABELS))
#define Silent     (KeyLogValue(ReaddataKeys,KSILENT))
#define Prompt     (KeyLogValue(ReaddataKeys,KPROMPT))
#define Stop       (KeyCharValue(ReaddataKeys,KSTOP))
#define Go         (KeyCharValue(ReaddataKeys,KGO))
#define Skip       (KeyCharValue(ReaddataKeys,KSKIP))
#define CharRead   (KeyLogValue(ReaddataKeys,KCHAR))
#define ByWords    (KeyLogValue(ReaddataKeys,KBYWORDS))
#define ByChars    (KeyLogValue(ReaddataKeys,KBYCHARS))
#define ByLines    (KeyLogValue(ReaddataKeys,KBYLINES))
#undef  BadValue /*defined in X.h*/
#define BadValue   (KeyRealValue(ReaddataKeys,KBADVAL))
#define NotFoundOK (KeyLogValue(ReaddataKeys,KNOTFOUNDOK))
#define NoFileOK   (KeyLogValue(ReaddataKeys,KNOFILEOK))

static char     *Opnames[] = 
{
	"vecread", "matread", "macroread", "read", "inforead", "macrosearch",
	(char *) 0
};

static short     Opcodes[] =
{
	IVECREAD, IMATREAD, IMACROREAD, IREAD, IINFOREAD, IMACROSEARCH
};

#define FromString (StringToRead != (unsigned char **) 0)

/*
   Globals for communicating with datagetc() (in unxio.c or macIo.c)

   StringToRead == (unsigned char **) 0 signals read by getc() from a file;
     otherwise mygetc() returns (*StringToRead)[PlaceInString], or EOF if
     PlaceInString >= Stringlength ; PlaceInString is incremented.
     If (*StringToRead)[PlaceInString] == '\0', mygetc() returns '\n'
  LastDataWasCr is 1 if last character was CR; otherwise 0.  This allows
     correctly reading text files with CR, LF or CR/LF line separators
     files should be opened in binary mode.

   980805 Changed type of StringToRead to unsigned char **
          Changed because an octal 254 in s stopped vecread(string:s,bychars)
*/
#define DATAPROMPT "Input> "

unsigned char **StringToRead;
long            StringLength;
long            PlaceInString;
char            LastDataWasCr;
/*
  980501 DataPrompt is prompt for next line of data read interactively
  from CONSOLE
*/
char          *DataPrompt;

#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment Readdata1
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

/*
	Read next data value from *LINE, starting at *LINE + place
	*next is set to position of next character to read
	*badchar is set to '\0' if a number is successfully decoded or if
	? if found;  otherwise it is set to the first non-white bad character
	found or EOF if the scan goes off the end.

	If a non-numeric item is found, scanning continues to the next ',',
	'-', '+' or digit (or end of line) and *next is set to the offset of
	the place stopped at.  When 2 successive commas are found, *next is set	
	to the offset of the second 1;  when a successful decoding is done or '?'
	found, next is set to the offset of the first position after what was
	decoded.

*/
static double nextRealItem(long place, long *next, int *badchar)
{
	char      *cplace = *LINE + place, c, *cnext;
	double     value;
	int        ncommas = 0;
	WHERE("nextRealItem");

	*badchar = '\0';
	while (ncommas < 2 && (c = *cplace , isspace(c) || c == ','))
	{
		if (c == ',')
		{
			ncommas++;
		}
		cplace++;
	} /*while ((c = *cplace , isspace(c) || c == ','))*/

	if (ncommas > 1)
	{
		*badchar = ',';
		cplace--;
	}
	else if (c == '\0')
	{
		*badchar = EOF;
	}
	else if (c == '?')
	{
		setMissing(value);
		while ((c = *(++cplace)) && c == '?')
		{/* multiple '?' the same as 1 */
			;
		}
	} /*if (*cplace == '?')*/
	else if ((c == '.' || c == '*') &&
			 (cplace[1] == '\0' || cplace[1] == ',' || isspace(cplace[1])))
	{/* isolated '.' is MISSING (like SAS) */
		setMissing(value);
		cplace++;
	}
	else
	{
		value = mystrtod(cplace, &cnext);

		if (cnext != cplace)
		{ /*decoded number */
			cplace = cnext;
		} /*if (cnext != cplace)*/
		else
		{ /* not a number, skip past bad stuff */
			if (*cplace == '\0')
			{
				*badchar = EOF;
			} /*if (*cplace == '\0')*/
			else
			{
				c = *badchar = *cplace;
				if (c == '-' || c == '+' || c == '.')
				{
					cplace++;
				}
				
				while ((c = *cplace) != '\0')
				{
					if (isspace(c) || isdigit(c) || c == '-' || c == '+' ||
						c == '.' || c == ',')
					{
						break;
					}
					cplace++;
				} /*while ((c = *cplace) != '\0')*/
			} /*if (*cplace == '\0'){}else{}*/
		} /*if (cnext != cplace){}else{}*/
	} /*if (*cplace == '?'){}else{}*/
	*next = cplace - *LINE;
	return (value);
	
} /*nextRealItem()*/

/*
   Find the next comma- or whitespace-separated field
   If the field is null (successive commas or trailing comma)
     return a pointer to NullString and set Next so that
     (*LINE)[next] is the next character to be read
   If nothing is found, return (char *) 0;
   Otherwise, return a pointer to the start of the field and set Next
   so that the field starts at *LINE + next 
*/
static char * nextCharItem(long place, long *next)
{
	char      *cplace = *LINE + place, c;
	char      *cvalue;
	int        ncommas = 0;
	WHERE("nextCharItem");
	
	while (ncommas < 2 && (c = *cplace) != '\0' && (isspace(c) || c == ','))
	{
		cplace++;
		if (c == ',')
		{
			ncommas++;
		}
	}

	if (ncommas == 2 || ncommas == 1 && c == '\0')
	{ /* 2 commas with nothing non-white between them or trailing comma */
		*next = cplace - *LINE;
		if (ncommas == 2)
		{
			(*next)--;
		}
		cvalue = NullString;
	}
	else if (c == '\0')
	{ /* nothing found */
		cvalue = (char *) 0;
	}
	else
	{
		*next = cplace - *LINE;
		cvalue = cplace;
	}
	return (cvalue);
} /*nextCharItem()*/

static void headerError(long errorNumber, char *matName, char *fileName,
						int warningOnly)
{
	long        printLine = 0;
	char        prefix[10];

	strcpy(prefix, (warningOnly) ? "WARNING" : "ERROR");
	switch(errorNumber)
	{
	  case NOTFOUND:
		sprintf(OUTSTR, "%s: dataset or macro %s not found", prefix, matName);
		break;

	  case COMPONENTNOTFOUND:
		  {
			  int         place;
			  char        c;

			  place = strlen(matName) - 1;
			  c = matName[place];
			  matName[place] = '\0';
				   
			  sprintf(OUTSTR, "%s: component for structure %s not found",
					  prefix, matName);
			  matName[place] = c;
			  break;
		  }
	  
	  case BADHEADER:
		sprintf(OUTSTR, "%s: header for %s in wrong format", prefix, matName);
		printLine = 1;
		break;
	  case TWOFORMATS:
		sprintf(OUTSTR,
				"%s: two scanf formats found in header lines for %s",
				prefix, matName);
		printLine = 1;
		break;
	  case BADFORMAT:
	  case BADCFORMAT:
		sprintf(OUTSTR,
				"%s: something wrong with format associated with %s",
				prefix, matName);
		printLine = 1;
		break;
	  case NODATA:
		sprintf(OUTSTR, "%s: endfile hit before data or macro reached", prefix);
		break;
	  case NOFIELDS:
		sprintf(OUTSTR,
				"%s: header line for %s specifies zero cases", prefix, matName);
		printLine = 1;
		break;
	  case NOLINES:
		sprintf(OUTSTR, "%s: header line specifies 0 lines in macro %s",
				prefix, matName);
		printLine = 1;
		break;
	  default:
		sprintf(OUTSTR,
				"%s: headerError %ld for %s", prefix, errorNumber, matName);
		printLine = 1;
	} /*switch(errorNumber)*/
	putPieces(OUTSTR, " on file ", fileName, (char *) 0);
	if (printLine)
	{
		putOutMsg("Last line read was:");
		echoLINE();
	}
	*OUTSTR = '\0';
} /*headerError()*/

static void readError(int errorNumber, char *matName, char *fileName,
					  int warningOnly)
{
	char        prefix[10];
	
	strcpy(prefix, (warningOnly) ? "WARNING" : "ERROR");
	
	switch (errorNumber)
	{
	  case NODATA:
		sprintf(OUTSTR, "%s: no data read from dataset %s on file ",
				prefix, matName);
		break;
	  case NOMACRO:
		sprintf(OUTSTR, "%s: no body found for macro %s on file ",
				prefix, matName);
		break;

	  case CANTALLOC:
		sprintf(OUTSTR, "%s: memory allocation problem reading file ", prefix);
		break;
		
	  default:
		sprintf(OUTSTR,
				"%s: problem %d reading dataset or macro %s from file ",
				prefix, errorNumber, matName);
	} /*switch (errorNumber)*/
	putPieces(OUTSTR, fileName, (char *) 0, (char *) 0);
	putOutMsg("Last line read was:");
	echoLINE();
	*OUTSTR = '\0';
} /*readError()*/

static int findLine(FILE *fn, long echo, long verbose, unsigned char skipCh,
					 unsigned char goCh, int charOp, long * status)
{
	long                 length;
	int                  skipEmptyLines = (charOp == KCHAR);
	unsigned char        c;
	WHERE("findLine");
	
	do
	{
		*status = fillLINE(fn);
		length = strlen(*LINE);
		
		if (length > 0)
		{
			c = (unsigned char) (*LINE)[0];

			if (charOp != KBYCHARS && (*LINE)[length-1] == '\n')
			{
				(*LINE)[--length] = '\0';
			}
			if (echo || verbose && c == skipCh)
			{
				echoLINE();
			}
		} /*if (length > 0)*/
	} while (*status != EOF && ((skipCh != '\0' && c == skipCh) ||
								skipEmptyLines && length == 0));
	if (goCh != '\0' && (*LINE)[0] != goCh)
	{
		*status = EOF;
		length = 0;
	}

	return (*status != EOF || length > 0);
} /*findLine()*/

#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment Readdata2
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

/*
  970530 modified storage allocation.  The amount allocated each time
         is approximately 1/CHUNKFRACTION of what has already been allocated
         This was to fix very slow reading of very large files
*/

enum readChunks
{
	CHARCHUNK = 400, /*initial increment for allocating space for CHAR data */
	REALCHUNK =  50 ,/*initial increment for allocating space for REAL data */
	CHUNKFRACTION = 20
};

static long charVecRead(FILE *fn, Symbolhandle result, long verbose,
						int charOp, long echo,
						unsigned char stopCh, unsigned char goChar,
						unsigned char skipCh)
{
	char         **chdata = (char **) 0;
	long           foundStop = 0;
	long           needed, allocated = 0, used = 0, place, nItems = 0;
	char          *field, *out, c;
	long           next;
	long           status = 0;
	long           chunk = CHARCHUNK;
	long           length;
	int            byLines = (charOp == KBYLINES);
	int            byChars = (charOp == KBYCHARS);
	WHERE("charvecRead");
	
	while (status != EOF && !foundStop &&
		   findLine(fn, echo, verbose, skipCh, goChar, charOp, &status))
	{
		if ((byLines  || byChars) && (*LINE)[0] == stopCh)
		{
			break;
		}
		length = strlen(*LINE);
		needed = used + length + 1;
		if (byChars)
		{
			needed += length;
			if (length != 0 && isNewline((*LINE)[length-1]))
			{
				needed--;
			}
		} /*if (byChars)*/
		
		if (needed > allocated)
		{
			chunk = (needed < CHUNKFRACTION * CHARCHUNK) ?
				chunk : needed/CHUNKFRACTION;
			
			if (needed - used < chunk)
			{
				needed = used + chunk;
			}
			
			chdata = (allocated == 0) ?
				mygethandle(needed) : mygrowhandle(chdata, needed);
			setSTRING(result, chdata);
			if (chdata == (char **) 0)
			{
				goto errorExit;
			}
			allocated = needed;
		} /*if (needed > allocated)*/
		
		if (byLines)
		{
			used = copyStrings(*LINE, *chdata + used, 1) - *chdata;
			nItems++;
		} /*if (byLines)*/
		else if (byChars)
		{
			char      *line = *LINE;
			
			do
			{
				c = *line++;
				
				(*chdata)[used++] = c;

				if (c != '\0')
				{
					(*chdata)[used++] = '\0';
				}
				nItems++;
			} while (c != '\0' && !isNewline(c));
		} /*if (byLines) {}else if(byChars){}*/
		else
		{
			place = 0;

			while (!foundStop)
			{
				out = *chdata;
				/* there should be room to copy whatever we find to chdata*/
				field = nextCharItem(place, &next);

				if (field == (char *) 0)
				{ /* off the end of the line */
					break;
				}

				if (*field == '\0')
				{
					/* null field */
					out[used++] = '\0';
					nItems++;
					place = next;
					if ((*LINE)[next] == '\0')
					{
						break;
					}
				}
				else if (*field == stopCh)
				{ /* field starting with '!' or other stop character */
					foundStop = 1;
				}
				else
				{ /* anything else; copy up to whitespace, comma, or '!' */
					while ((c = *field) != '\0' &&
						   !isspace(c) && c != stopCh && c != ',')
					{
						out[used++] = c;
						field++;
					}
					out[used++] = '\0';
					nItems++;
					place = field - *LINE;
				}
			} /*while (!foundStop)*/
		} /*if (byLines) {}else if(byChars){}else{}*/
	} /*while (status != EOF && !foundStop && findLine(fn, echo, verbose, skipCh, goChar, byLines,&status))*/

	if (nItems == 0)
	{ /* nothing found */
		mydisphandle(chdata);
		setSTRING(result, (char **) 0);
	} /*if (nItems == 0)*/
	else
	{
		if (byChars && used > 1 &&
			(*chdata)[used-1] == '\0' && (*chdata)[used-2] == '\0')
		{
			nItems--;
			used--;
		}
		
		chdata = mygrowhandle(chdata, used);
		setSTRING(result, chdata);
		if (chdata == (char **) 0)
		{
			goto errorExit;
		}
	} /*if (nItems == 0){}else{}*/
	
	return (nItems);

  errorExit:
	return (-1);
	
} /*charVecRead()*/


static Symbolhandle doReadVec(FILE *fn, char **fnameH, long verbose, long echo,
							  long silent, int charOp,
							  double badValue, unsigned char stopCh,
							  unsigned char goCh, unsigned char skipCh)
{
	long            status, gotsome, nItems, allocated;
	double        **dataH = (double **) 0;
	double          value, tmp;
	Symbolhandle    result = (Symbolhandle) 0;
	long            warnflag = silent, foundInfinite = 0;
	long            foundBad = 0;
	long            place;
	long            chunk = REALCHUNK;
	int             badchar;
	WHERE("doReadVec");

	nItems = 0;
	if (charOp)
	{
		result = CInstall(SCRATCH, 0);
	}
	else
	{
		allocated = chunk;
		result = RInstall(SCRATCH, allocated);
	}
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	if (!charOp)
	{
		dataH = DATA(result);

		/* read until an acceptable line is found */
		gotsome = findLine(fn, echo, verbose, skipCh, goCh, 0 , &status);

		place = 0;

		while (gotsome || status != EOF)
		{
			if (allocated <= nItems)
			{/* make more space */
				chunk = (nItems < CHUNKFRACTION*REALCHUNK) ?
					chunk : nItems/CHUNKFRACTION; 
				allocated = nItems + chunk;
				dataH = (double **) mygrowhandle((char **) dataH,
												 allocated * sizeof(double));
				setDATA(result, dataH);
				if (dataH == (double **) 0)
				{
					goto errorExit;
				}
			} /*if (allocated <= nItems)*/

			while (allocated > nItems && gotsome)
			{
				value = nextRealItem(place, &place, &badchar);
				
				if (badchar != EOF)
				{
					if (badchar != 0)
					{
						if (badchar == stopCh)
						{
							status = EOF;
							gotsome = 0;
						}
						else if(realIsSet(badValue))
						{
							(*dataH)[nItems++] = badValue;
						}
						else
						{
							/* bad character */
							foundBad = 1;
						}
					} /*if (badchar != 0)*/
					else
					{
						if (!isMissing(value) &&
							(tmp = fabs(value), doubleEqualBdouble(tmp, TooBigValue)))
						{
							foundInfinite = 1;
							setMissing(value);
						}
						(*dataH)[nItems++] = value;
					} /*if (badchar != 0){}else{}*/
				} /*if (badchar != EOF)*/				

				if ((*LINE)[place] == '\0')
				{ /* nothing left in line */
					if (status != EOF)
					{ /* try to refill it */
						gotsome = findLine(fn, echo, verbose, skipCh, goCh, 0, &status);
						place = 0;
					}
					else
					{
						gotsome = 0;
					}
				} /*if ((*LINE)[place] = '\0')*/
			} /*while (allocated > nItems && gotsome)*/
		} /*while (gotsome || status != EOF)*/

		if (foundBad && !warnflag)
		{
			putPieces("WARNING: nonnumeric character(s) ignored on ",
					  *fnameH, (char *) 0, (char *) 0);
		}

		if (nItems > 0)
		{/* some data was read */
			if (foundInfinite && !silent)
			{
				putOutErrorMsg("WARNING: numbers outside representable range replaced by MISSING");
			}
			/* shrink data to its actual size */
			dataH = (double **) mygrowhandle((char **) dataH, nItems * sizeof(double));
			setDATA(result, dataH);
			if (dataH == (double **) 0)
			{
				goto errorExit;
			}
		} /*if (nItems > 0)*/
		else
		{
			mydisphandle((char **) dataH);
			setDATA(result, (double **) 0);
		}
	} /*if (!charOp)*/
	else
	{
		nItems = charVecRead(fn, result, verbose, charOp,
							 echo, stopCh, goCh, skipCh);
		if (nItems < 0)
		{
			goto errorExit;
		}
	} /*if (!charOp){}else{}*/
		
	if (nItems > 0)
	{ /* some data was read */
		setNDIMS(result, 1);
		setDIM(result, 1, nItems);
		setNCLASS(result, -1);
	} /*if (nItems > 0)*/
	else
	{
		if (!silent)
		{
			sprintf(OUTSTR,
					"WARNING: no data found by %s()", FUNCNAME);
			putErrorOUTSTR();
		}

		setSTRING(result, (char **) 0);
		setTYPE(result, NULLSYM);
		setNAME(result, NULLSCRATCH);
		setNDIMS(result,1);
		setDIM(result, 1, 0);
	} /*if (nItems > 0){}else{}*/
	
	return (result);

  errorExit:
  	Removesymbol(result);
	return (0);

} /*doReadVec()*/

/*
  970227 added new argument endMacro to calls to readHeader() and readMacro()
  970302 added capability to read structures and NULL symbols.
  970619 added capability to distinguish between macros to be expanded
         in-line and out-of-line
  990220 Added argument dollars to readHeader to pass the presence or
         absense of keyword DOLLARS on a macro
  Note: matNameH is either a real MacAnova handle, if user provided
        a name, or is an pointer to a pointer to an array of length
		MAXNAME + 1, if the user did not provide a name, or if
		doReads() is called from itself to read a structure component
		in which case *matNameH should end with '$'
*/

static Symbolhandle doReads(FILE *fp, char **matNameH, char **fnameH,
							long op, long verbose, long echo, long silent,
							long notFoundOK,
							Symbolhandle symhLabels)
{
	Symbolhandle     result = (Symbolhandle) 0, result1;
	Symbolhandle     labelsOrNotesFromFile = (Symbolhandle) 0;
	char             format[MAXFMT + 1];
	char             endMacro[ENDMACROLENGTH + 1];
	char            *matName = *matNameH;
	char            *fmt = format, **string = (char **) 0;
	double          *y;
	double           yi, missValue;
	long             nFields, nSkip;
	int              byRows, quoted, inLine = -1;
	int              labels, notes, dollars;
	long             i, size, length = 0;
	long             ndims, dims[MAXDIMS];
	long             type;
	long             reply, labelError;
	long             printHeader = verbose;
	long             dataType;
	int              nameLength = strlen(matName);
	short            badLogical = 0, foundInfinite = 0, checkMissing;
	int              nTries, nlabels;
	long             status = 1;
	unsigned long    report;
	WHERE("doReads");
	
	*OUTSTR = '\0';
	setMissing(missValue);
	endMacro[0] = '\0';

	/*
		When the user did not specify a name or when reading a structure
		component, the actual name of the object read is copied to *matNameH
		matNameH in that case points to a buffer of length at least MAXNAME + 1
	*/

	reply = readHeader(fp, dims, &nFields, &fmt, &nSkip, &byRows, &type,
					   &quoted, &inLine, &labels, &notes, &dollars,
					   &missValue, *matNameH, endMacro,
					   (!(op & IINFOREAD)) ? printHeader : -1);

	if (notFoundOK && reply == NOTFOUND)
	{
		goto returnNull;
	}
	
	if (reply != NOERROR)
	{
		if (**matNameH == '\0')
		{
			matName = "first data set";
			matNameH = &matName;
		}
		if (op != ICOMPREAD ||
			!silent && reply != NOTFOUND && reply != COMPONENTNOTFOUND)
		{
			headerError(reply, *matNameH, *fnameH, op == ICOMPREAD);
		}
		goto errorExit;
	} /*if (reply != NOERROR)*/

	dataType = type == REAL || type == LOGIC || type == CHAR;

	if (symhLabels != (Symbolhandle) 0)
	{		
		if (!dataType)
		{
			symhLabels = (Symbolhandle) 0;
			if (!silent)
			{
				sprintf(OUTSTR,
						"WARNING: labels ignored when reading %s",
						(type == MACRO) ? "macro" :
						((type == STRUC) ? "structure" : "NULL symbol"));
				putErrorOUTSTR();
			}
		}
		labels = 0; /* ignore any labels on symbol */
	} /*if (symhLabels != (Symbolhandle) 0)*/

	if (!(op & IINFOREAD))
	{
		checkMissing = !isMissing(missValue);
		
		if (type != CHAR && nFields > MAXN)
		{
			sprintf(OUTSTR,
					"ERROR: number of items specified by header is %ld > %ld",
					nFields, (long) MAXN);
			goto errorExit;
		} /*if (nFields > MAXN)*/

		if (!silent && !(op & IMATREAD || op & IMACROREAD) &&
			(op & IMATREAD && type == MACRO ||
			 op & IMACROREAD && type != MACRO))
		{
			sprintf(OUTSTR, "WARNING: found %s instead of %s",
					(op & IMATREAD) ? "macro" : "data set",
					(op & IMATREAD) ? "data set" : "macro");
			putErrorOUTSTR();
		}
	
		ndims = dims[0];
		if (dataType)
		{
			if (ndims == 1)
			{
				dims[2] = 1;
			}
		
			if (dims[1] == 0)
			{
				if (!silent)
				{
					putOutErrorMsg("WARNING: 0 lines of data in data set");
				}
				goto returnNull;
			} /*if (dims[1] == 0)*/
		
			if (*fmt == '\0' && type != CHAR)
			{
				fmt = DefaultFormat;
			}
		
			if (type == CHAR)
			{
				result = CInstall(SCRATCH, 0);
			}
			else
			{
				result = RInstall(SCRATCH, 0);
			}
			
			if (result == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			setTYPE(result, type);
			setNDIMS(result, ndims);
			size = 1;
			for (i = 1; i <= ndims; i++)
			{
				size *= dims[i];
				setDIM(result, i, dims[i]);
			} /*for (i = 1; i <= ndims; i++)*/
			
			if (symhLabels != (Symbolhandle) 0)
			{
				labelError = checkLabels(symhLabels, ndims, dims + 1);

				if (labelError != LABELSOK &&
					(!silent || labelError & LABELSERROR))
				{
					badLabels(labelError);
				}
				if (labelError & LABELSERROR ||
					!(labelError & WRONGSIZELABELS) &&
					!installLabels(symhLabels, result))
				{
					goto errorExit;
				}
			} /*if (symhLabels != (Symbolhandle) 0)*/
			
	/* on non-error, returns number of items actually read, if <= expected */
			reply = readFile(fp, nFields, fmt, nSkip, byRows, quoted,
							 result, echo, !silent);
			if (reply < 0)
			{
				readError(-reply, *matNameH, *fnameH, op == ICOMPREAD);
				goto errorExit;
			}

			if (!silent && reply != size)
			{
				sprintf(OUTSTR,
						"WARNING: %s data set incomplete; %ld items set to %s",
						typeName(type), size - reply,
						(type != CHAR) ? "MISSING" : "\"\"");
				putErrorOUTSTR();
			} /*if (!silent && reply != size)*/

			if (type != CHAR)
			{
				y = DATAPTR(result);
				for (i = 0;i < reply;i++)
				{
					if (checkMissing && y[i] == missValue)
					{
						setMissing(y[i]);
					}
					yi = y[i];
					if (!isMissing(yi))
					{
						if (type == LOGIC)
						{
							if (yi != 0.0 && yi != 1.0)
							{
								badLogical = 1;
							}
							y[i] = (double) (yi != 0.0);
						} /*if (type == LOGIC)*/
						else if ((yi = fabs(yi),
								  doubleEqualBdouble(yi, TooBigValue)))
						{ /* outside legal range */
							setMissing(y[i]);
							foundInfinite = 1;
						}
					} /*if (!isMissing(yi))*/				
				} /*for (i = 0;i < reply;i++)*/
			} /*if (type != CHAR)*/
			else
			{
				if (byRows && dims[0] > 1)
				{ /* Fix dimensions and Transpose */
					for (i = 1; i <= dims[0]; i++)
					{
						setDIM(result, i, dims[dims[0] - i + 1]);
					} /*for (i = 1; i <= dims[0]; i++)*/					

					result1 = doTranspose(result, (double *) 0, 0, &report);

					if (result1 == (Symbolhandle) 0)
					{
						goto errorExit;
					}
					else if (result1 != result)
					{
						Removesymbol(result);
						result = result1;
					}
				} /*if (byRows)*/
			} /*if (type != CHAR){}else{}*/
		} /*if (dataType)*/
		else if (type == MACRO)
		{
			char       *zeroLines = "WARNING: 0 lines in macro on file";

			labels = 0;
			if (dims[1] == 0)
			{
				if (!silent)
				{
					putOutErrorMsg(zeroLines);
				}
				goto returnNull;
			} /*if (dims[1] == 0)*/

			result = Install(SCRATCH, MACRO);
			if (result == (Symbolhandle) 0)
			{
				goto errorExit;
			}

			setScratchMacroName(result, *matNameH);
			
			if ((reply = readMacro(fp, dims[1], endMacro, result,
								   echo, !silent)) < 0)
			{
				goto errorExit;
			}
			if (reply == 0)
			{
				if (!silent)
				{
					putOutErrorMsg(zeroLines);
				}
				Removesymbol(result);
				goto returnNull;
			}
			setInline(result, inLine);
			setNDIMS(result, 1);
			setDIM(result, 1, 1);
			length = prepMacro(STRINGPTR(result), (char *) 0, dollars);
			if (length != strlen(STRINGPTR(result)) + 1)
			{
				TMPHANDLE = mygethandle(length);
				if (TMPHANDLE == (char **) 0)
				{
					goto errorExit;
				}
				
				(void) prepMacro(STRINGPTR(result), *TMPHANDLE, dollars);
				mydisphandle(STRING(result));
				setSTRING(result, TMPHANDLE);					
			} /*if (length != strlen(STRINGPTR(result)) + 1)*/			
		}
		else if (type == STRUC)
		{
			long           ncomps = dims[1], icomp;
			char           compName[MAXNAME + 1];
			char           targetName[MAXNAME + 1];
			char          *compNameP = compName;
			char         **compNameH = &compNameP;
			Symbolhandle   symhComp;
			
			result = StrucInstall(SCRATCH, ncomps);
			
			if (result == (Symbolhandle) 0)
			{
				goto errorExit;
			}

			setScratchName(result);
			matName = *matNameH;
			nameLength = strlen(matName);
			strcpy(targetName, matName);
			targetName[nameLength++] = COMPNAMESTART;
			targetName[nameLength] = '\0';

			for (icomp = 0; icomp < ncomps; icomp++)
			{
				strcpy(compName, targetName);
				symhComp = doReads(fp, compNameH, fnameH, ICOMPREAD,
								   verbose, echo, silent, 0, (Symbolhandle) 0);
				if (symhComp == (Symbolhandle) 0)
				{
					sprintf(OUTSTR,
							"ERROR: %s() can't find component %ld of structure %s",
							FUNCNAME, icomp + 1, *matNameH);
					putPieces(OUTSTR, " on file ", *fnameH, (char *) 0);
					*OUTSTR = '\0';
					goto errorExit;
				}
				Cutsymbol(symhComp);
				COMPVALUE(result, icomp) = symhComp;
				strcpy(compName, strrchr(compName, COMPNAMESTART) + 1);
				compName[NAMELENGTH] = '\0';
				setNAME(symhComp, compName);
			} /*for (icomp = 0; icomp < ncomps; icomp++)*/
		}
		else if (type == NULLSYM)
		{
			goto returnNull;
		}
		else
		{
			sprintf(OUTSTR,
					"ERROR: %s() does not know how to read type %s object",
					FUNCNAME, typeName(type));
			goto errorExit;
		}

		nlabels = 0;
		if (labels)
		{
			if (type == STRUC)
			{
				nlabels = NCOMPS(result);
			}
			else
			{
				int        i, ndims = NDIMS(result);
				
				for (i = 1; i <= ndims; i++)
				{
					nlabels += DIMVAL(result, i);
				}
			}
		} /*if (labels)*/
			
		nTries = labels + notes;
		for (i = 0; i < nTries && (labels || notes); i++)
		{
			char            labelsOrNotesName[MAXNAME + 1];
			char           *labelsOrNotesNameP = labelsOrNotesName;
			char          **labelsOrNotesNameH = &labelsOrNotesNameP;
			
			matName = *matNameH;
			nameLength = strlen(matName);

			strcpy(labelsOrNotesName, matName);
			labelsOrNotesName[nameLength++] = COMPNAMESTART;
			labelsOrNotesName[nameLength] = '\0';

			labelsOrNotesFromFile = doReads(fp, labelsOrNotesNameH,
											fnameH, ICOMPREAD,
											echo, echo, silent, 0,
											(Symbolhandle) 0);

			if (labelsOrNotesFromFile == (Symbolhandle) 0 ||
				TYPE(labelsOrNotesFromFile) != CHAR ||
				NDIMS(labelsOrNotesFromFile) != 1)
			{
				break;
			}
				
			
			if (labels && mystrncmp(labelsOrNotesName + nameLength,
									LABELSSCRATCH+2,
									strlen(LABELSSCRATCH+2)) == 0)
			{
				if (DIMVAL(labelsOrNotesFromFile, 1) != nlabels)
				{
					break;
				}
				if (!setLabels(result, STRING(labelsOrNotesFromFile)))
				{
					goto errorExit;
				}
				labels = 0;
			}
			else if (notes && mystrncmp(labelsOrNotesName + nameLength,
										NOTESSCRATCH+2,
							  			strlen(NOTESSCRATCH+2)) == 0)
			{
				if (!setNotes(result, STRING(labelsOrNotesFromFile)))
				{
					goto errorExit;
				}
				notes = 0;
			}
			else 
			{
				break;
			}
			setSTRING(labelsOrNotesFromFile, (char **) 0);
			Removesymbol(labelsOrNotesFromFile);
			labelsOrNotesFromFile = (Symbolhandle) 0;
		} /*for (i = 0; i < nTries && (labels || notes); i++)*/
		
		if (!silent)
		{
			char      *onFile = " on file ";
			
			if (labels)
			{
				sprintf(OUTSTR,
						"WARNING: %s() can't find labels for '%s'",
						FUNCNAME, *matNameH);
				putPieces(OUTSTR, onFile, *fnameH, (char *) 0);
			}
			if (notes)
			{
				sprintf(OUTSTR,
						"WARNING: %s() can't find notes for '%s'",
						FUNCNAME, *matNameH);
				putPieces(OUTSTR, onFile, *fnameH, (char *) 0);
			}
			if (badLogical)
			{
				putOutErrorMsg("WARNING: non 0 or 1 value(s) found in LOGICAL variable read; set to T");
			}
			else if (foundInfinite)
			{
				putOutErrorMsg("WARNING: numbers outside representable range replaced by MISSING");
			}
			*OUTSTR = '\0';
		} /*if (!silent)*/		
	} /*if (!(op & IINFOREAD))*/
	else
	{ /* inforead()*/
		if (verbose)
		{ /* print 1st line (name and dimensions) */
			(*LINE)[strlen(*LINE)-1] = '\0'; /* trim off '\n' */
			/* use putOutMsg to make sure line is counted */
			putOutMsg(*LINE);
		} /*if (verbose)*/

		result = CInstall(SCRATCH, 0);
		if (result == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		setDIM(result, 1, 1);
		while (status != EOF &&
			  (status = fillLINE(fp), (*LINE)[0] == COMMENTSTART))
		{
			if ((*LINE)[1] != '"')
			{ /* ignore lines starting ')"' */
				size = strlen(*LINE + 1);
				if (string == (char **) 0)
				{
					setSTRING(result, string = mygethandle(size + 1));
				}
				else
				{
					setSTRING(result,
							  string = mygrowhandle(STRING(result),
													length + size + 1));
				}
				if (string == (char **) 0)
				{
					goto errorExit;
				}
				strcpy(*string + length, *LINE + 1);
				length += size;
				if (verbose > 1 || verbose == 1 && (*LINE)[1] != COMMENTSTART)
				{
					(*LINE)[size] = '\0'; /* trim off '\n' */
					putOutMsg(*LINE);
				}
			} /*if ((*LINE)[1] != '"')*/
		} /*while (...)*/
		if (length > 0 && (*string)[length-1] == '\n')
		{
			length--;
			(*string)[length] = '\0';
			setSTRING(result, string = mygrowhandle(string, length + 1));
			if (string == (char **) 0)
			{
				goto errorExit;
			}
		} /*if (length > 0 && (*string)[length-1] == '\n')*/

		if (string == (char **) 0)
		{ /* no comment lines found */
			setSTRING(result, string = mygethandle(1));
			if (string == (char **) 0)
			{
				goto errorExit;
			}
			(*string)[0] = '\0';
		} /*if (string == (char **) 0)*/
	} /*if (!(op & IINFOREAD)){}else{}*/
	
	return (result);

  returnNull:
	result = Install(NULLSCRATCH, NULLSYM);
	return (result);
	
  errorExit:
	putErrorOUTSTR();

	Removesymbol(labelsOrNotesFromFile);
	Removesymbol(result);

	return (0);
} /*doReads()*/


/*
   Functions vecread(), matread(), macroread(), inforead()

  Routines to read data and macros in from file.
  'Read' has been replaced by synonym 'vecread'

  Usage:
	vecread(fileName)
	matread(fileName [,setName], [,quiet:T])
	macroread(fileName [,setName], [,quiet:T])
	inforead(fileName [,setName], [,quiet:T])

  where fileName and setName must be CHARACTER scalars.

  inforead() returns as a CHARACTER scalar the comment lines describing
  dataset or macro name, stripping off the leading ')' (COMMENTSTART)

  As usual on the Macintosh, fileName may be "".

  NOTE:  As of 931111, it is no longer legal on the Macintosh for fileName
  to be missing.

  950119:  If fileName is of the form string:charVec, a read will
  be simulated from the character vector, with each element starting on
  a new line.  Thus, for example, vecread(string:"1 2 3 4") is equivalent
  to run(4).

  960207 vecread(fileName, character:T) returns CHARACTER vector containing
  whitespace- or comma-separated fields read.  vecread(fileName,badvalue:val)
  returns numbers read interspersed with val for every unreadable item
  found.

  960304 On vecread() of reals, single commas are ignored, a string of k > 1
  commas will result in k - 1 bad character possibilities.

  960304 "?", "??", "???", ... are all equivalent; successive commas found
  by vecread(fileName,character:T) are interpreted as delimiting empty
  strings (""), as does a trailing comma

  960502 changed fopen() to fmyopen()
*/

#undef DEL
#define DEL    0x7f

#ifdef MACINTOSH
#define printableChar(C) (isgraph(C) || (C) > DEL)
#else /*MACINTOSH*/
#define printableChar(C) isgraph(C)
#endif /*MACINTOSH*/

#undef isascii
#define isascii(C) ((unsigned char) (C) <= 0x7f)

#define ISPUNCT(C) (ispunct(C) && (C) != '+' && (C) != '-' &&\
					(C) != '.' && (C) != '?' && (C) != ',')

#define okSkip(C) ((CharRead) ? printableChar(C) : ISPUNCT(C))

#define okStop(C) ((CharRead) ? (ispunct(C) && (C) != ',') : ISPUNCT(C))

enum consolCodes
{
	nonConsole = 0,
	interactiveConsole,
	batchConsole
};

/*
   General entry for vecread(), matread(), macroread(), and inforead()

   960503 changed macOpen() to macFindFile()

   970210 allowed keywords file and string anywhere.  If they are used, then,
   for {mat,macro,info}read(), data set or macro name, if present, may also
   be in any position.

   971030 on commands other than vecread(), quiet:F forces printing of all
   headerlines;  otherwise header lines starting '))' are not printed

   971104 added check on value of myhandlelength()

   980501 added keyword phrase prompt:F to suppress prompts when reading
          from CONSOLE
   980507 added keyword phrase go:G; vecread() will stop reading lines
          on the first line that does not start with G; cannot be used with
          stop:S
   981116 Added new function name "macrosearch";  this is not in the symbol
          table but is used by searchForMacro() in yylex.c to use
          readdata to search and load undefined macros found in a
          command line
   981120 New keyword phrase nofileok:T causes a failure to open a
          file to not be an error, returning NULL.
*/
Symbolhandle    readdata(Symbolhandle list)
{
	char           *fname;
	char            setname[MAXNAME + 1], *setnameP = setname;
	char          **fnameH, **setnameH;
	FILE           *fn = (FILE *) 0;
	Symbolhandle    arg1, arg2 = (Symbolhandle) 0, result = (Symbolhandle) 0;
	long            nargs = NARGS(list), margs;
	long            useConsole = nonConsole;
	long            keyStatus;
	long            nkeys = NKeys(ReaddataKeys);
	keywordListPtr  keys = ReaddataKeys;
	long            i;
	int             setPlace;
	unsigned long   labelError;
	char           *keyword, *whatread, *charKeyname  = KeyName(keys,KCHAR);
	unsigned char   stopChar = '\0', skipChar = '\0', goChar = '\0';
	char           *notPunctuation =
		"ERROR: value for %s not punctuation character or is '+', '-', ',', or '.'";
	char           *badCharStop =
	  "ERROR: value for '%s' is \",\" or not a punctuation character";
	char           *badCharSkip =
	  "ERROR: value for '%s' is a non-printable character";
	long            op;
	int             verbose;
	WHERE("readdata");
	
	setname[0] = OUTSTR[0] = '\0';
	
	LINE = (char **) 0;
	StringToRead = (unsigned char **) 0;
	StringLength = PlaceInString = 0;
	
	for (i = 0; Opnames[i] != (char *) 0;i++)
	{
		if (strcmp(FUNCNAME, Opnames[i]) == 0)
		{
			op = Opcodes[i];
			break;
		}
	} /*for (i = 0; Opnames[i] != (char *) 0;i++)*/

	unsetKeyValues(keys, nkeys);

	/*
	  970210 "file" and "string" now in keyword list
	  and can appear any where.  Since getAllKeyValues() moves used
	  keywords to end, any data set or macro name will end as
	  arg 1 or 2
	*/

	if (op != IMACROSEARCH)
	{
		keyStatus = getAllKeyValues(list, 0, op, keys, nkeys);
		if (keyStatus < 0)
		{
			goto errorExit;
		}

		if (charIsSet(File) && symhIsSet(String))
		{
			sprintf(OUTSTR,
					"ERROR: illegal to use both '%s' and '%s' in %s()",
					KeyName(keys,KFILE),KeyName(keys,KSTRING), FUNCNAME);
			goto errorExit;
		}
	} /*if (op != IMACROSEARCH)*/
	else
	{
		/*
		  call from yylex implicitly has
            notfoundok:T, nofileok:T, echo:F, silent:T
		*/
		setLogicalTrue(NotFoundOK);
		setLogicalTrue(NoFileOK);
		setLogicalFalse(Echo);
		setLogicalTrue(Silent);
		keyStatus = 0;
	} /*if (op != IMACROSEARCH){}else{}*/
	
	/*arg1 may not have originally been argument 1*/
	arg1 = COMPVALUE(list, 0);
	nargs -= keyStatus;

	if (symhIsSet(String))
	{
		StringToRead = (unsigned char **) STRING(String);
		StringLength = myhandlelength((char **) StringToRead);
		if (StringLength < 0)
		{
			goto errorExit;
		}
		setPlace = (nargs > 0) ? 0 : -1;
	}
	else if (charIsSet(File))
	{
#ifdef CONSOLEOK
	/*
	   useConsole == 0 if read not from CONSOLE
	   useConsole == 1 if read from CONSOLE which is interactive input
	   useConsole == 2 if read from CONSOLE while reading batch file or
	                   noninteractive (redirected) input
	*/
		if (ISCONSOLE(*File))
		{
			useConsole = (!(ISATTY & ITTYIN) || BDEPTH > 0) ?
			  batchConsole : interactiveConsole;
			fname = CONSOLENAME;
			fnameH = &fname;
		}
		else
#endif /*CONSOLEOK*/
		{
			fnameH = File;
		}
		setPlace = (nargs > 0) ? 0 : -1;
	}
	else
	{ /* neither 'file' or 'string' used; 1st arg must be file name*/
		if (arg1 == (Symbolhandle) 0)
		{
			sprintf(OUTSTR,
					"ERROR: nothing specified for %s() to read from",
					FUNCNAME);
		}
		else if (isKeyword(arg1))
		{
			sprintf(OUTSTR,
					"ERROR: file name must be arg. 1 to %s() without %s or %s",
					FUNCNAME, KeyName(keys,KSTRING),KeyName(keys,KFILE));
		}
		if (*OUTSTR)
		{
			goto errorExit;
		}
#ifdef CONSOLEOK
		if ((ISCONSOLE(NAME(arg1)) || 
			TYPE(arg1) == CHAR && isScalar(arg1) && ISCONSOLE(STRINGPTR(arg1))))
		{
			useConsole = (!(ISATTY & ITTYIN) || BDEPTH > 0) ? 
			  batchConsole : interactiveConsole;
			fname = CONSOLENAME;
			fnameH = &fname;
		}
		else
#endif /*CONSOLEOK*/
		{
			if (!isScalar(arg1) || TYPE(arg1) != CHAR)
			{
				notCharOrString("file name");
				goto errorExit;
			}
			fnameH = STRING(arg1);
		}
		nargs--; /* used up file name */
		setPlace = (nargs > 0) ? 1 : -1;
	}
	/*
	  setPlace is now set to current position in list where any data set or
	  macro name is expected or to -1 if nargs == 0
	*/
	
	
	if (!logicalIsSet(NotFoundOK))
	{
		setLogicalFalse(NotFoundOK);
	} /*if (!logicalIsSet(NotFoundOK))*/
	
	if (!logicalIsSet(NoFileOK))
	{
		setLogicalFalse(NoFileOK);
	} /*if (!logicalIsSet(NoFileOK))*/
	
	if (!logicalIsSet(Silent))
	{
		if (!NotFoundOK)
		{
			setLogicalFalse(Silent);
		}
		else
		{
			setLogicalTrue(Silent);
		}
	} /*if (!logicalIsSet(Silent))*/
	
	if (!logicalIsSet(Prompt))
	{
		if (op != IMACROSEARCH)
		{
			setLogicalTrue(Prompt);
		}
		else
		{
			setLogicalFalse(Prompt);
		}
	} /*if (!logicalIsSet(Prompt))*/
	else if (useConsole == nonConsole)
	{
		sprintf(OUTSTR,
				"WARNING: keyword %s ignored when not reading from CONSOLE",
				KeyName(keys,KPROMPT));
		putErrorOUTSTR();
		setLogicalTrue(Prompt);
	}

	if (charIsSet(Stop))
	{
		keyword = KeyName(keys,KSTOP);
		if (strlen(*Stop) != 1)
		{
			sprintf(OUTSTR,
					"ERROR: value for %s must be single character", keyword);
			goto errorExit;
		}
		stopChar = (*Stop)[0];
	} /*if (charIsSet(Stop))*/

	if (charIsSet(Go))
	{
		keyword = KeyName(keys,KGO);
		if (strlen(*Go) != 1)
		{
			sprintf(OUTSTR,
					"ERROR: value for %s must be single character", keyword);
			goto errorExit;
		}
		goChar = (*Go)[0];
	} /*if (charIsSet(Go))*/

	if (charIsSet(Skip))
	{
		keyword = KeyName(keys,KSKIP);
		if (strlen(*Skip) != 1)
		{
			sprintf(OUTSTR,
					"ERROR: value for %s must be single character", keyword);
			goto errorExit;
		}
		skipChar = (*Skip)[0];
	} /*if (charIsSet(Skip))*/

	if (symhIsSet(Labels))
	{
		labelError = checkLabels(Labels, 0, (long *) 0);
		if (labelError != LABELSOK)
		{
			badLabels(labelError);
			goto errorExit;
		}
	} /*if (symhIsSet(Labels))*/
	else
	{
		Labels = (Symbolhandle) 0;
	}
	
	/*
	  At this point,
	    nargs = number of non processed arguments
		setPlace < 0 if nargs == 0
		setPlace = 0 if nargs > 0 and 'file' or 'string' used
		setPlace = 1 if nargs > 0 and 'file' and 'string' not used
	*/
	if (op & IVECREAD)
	{
		margs = (nargs > 0) ? 1 : 0;
		if (stopChar != '\0')
		{
			if (goChar != '\0')
			{
				sprintf(OUTSTR,
						"ERROR: keywords '%s' and '%s' are incompatible on %s()",
						KeyName(keys,KSTOP), KeyName(keys,KGO), FUNCNAME);
				goto errorExit;
			}
			
			/*
			  When not reading from CONSOLE, stopping character can
			  be any non-ascii character, such as "\377", usually
			  selected to ensure the whole file will be read
			*/
			if (!okStop(stopChar) &&
				(useConsole != nonConsole || isascii(stopChar)))
			{
				sprintf(OUTSTR, (CharRead) ? badCharStop :notPunctuation,
						KeyName(keys, KSTOP));
				goto errorExit;
			}
		} /*if (stopChar != '\0')*/
		else if (goChar == '\0')
		{
			stopChar = '!';
		}
		
		if (skipChar != '\0')
		{
			if (skipChar == goChar)
			{
				sprintf(OUTSTR,
						"ERROR: both '%s' and '%s' have the same value on %s()",
						KeyName(keys, KSTOP), KeyName(keys, KGO), FUNCNAME);
				goto errorExit;
			}
			
			if (skipChar == stopChar)
			{
				if (logicalIsSet(ByLines) && ByLines)
				{
					sprintf(OUTSTR,
							"ERROR: illegal for '%s' and '%s' to be the same with %s:T",
							KeyName(keys,KSTOP), KeyName(keys,KSKIP),
							KeyName(keys,KBYLINES));
					goto errorExit;
				}
				if (!Silent)
				{
					sprintf(OUTSTR,
							"WARNING: '%s' and '%s' characters are the same",
							KeyName(keys,KSTOP),KeyName(keys,KSKIP));
					putErrorOUTSTR();
				} /*if (!Silent)*/
			} /*if (skipChar == stopChar)*/
			if (!okSkip(skipChar))
			{
				sprintf(OUTSTR, (CharRead) ? badCharSkip : notPunctuation,
						KeyName(keys, KSKIP));
				goto errorExit;
			}
		} /*if (skipChar != '\0')*/
		else if (logicalIsSet(Quiet) && !Quiet && !Silent)
		{
			sprintf(OUTSTR,
					"WARNING: '%s:F' without skip character ignored by %s()",
					KeyName(keys, KQUIET), FUNCNAME);
			putErrorOUTSTR();
		}
	} /*if (op & IVECREAD)*/
	else
	{
		margs = (nargs > 0) ? 1 : 0;
	}
	
	if (nargs > margs)
	{
		sprintf(OUTSTR,
				"ERROR: %s() has too many non-keyword arguments", FUNCNAME);
		goto errorExit;
	} /*if (nargs > margs)*/
	
	if (op & IINFOREAD || op == IREAD)
	{
		whatread = "data or macro name";
	}
	else if (op & IMACROREAD)
	{
		whatread = "macro name";
	}
	else if (op & IMATREAD)
	{
		whatread = "data set name";
	}
	
	if (!logicalIsSet(Echo))
	{
		if (!Silent)
		{
#ifdef HASCONSOLEDIALOG
	/* echo anything from CONSOLE dialog box*/
			Echo = useConsole;
#else  /*HASCONSOLEDIALOG*/
		/* echo anything from batch or non-interactive CONSOLE*/
			Echo = (useConsole == batchConsole);
#endif /*HASCONSOLEDIALOG*/
		} /*if (!Silent)*/
		else
		{
			Echo = 0;
		}
	} /*if (!logicalIsSet(Echo))*/
	else if (Echo && Silent)
	{
		sprintf(OUTSTR,
				"ERROR: %s:T and %s:T are incompatible on %s()",
				KeyName(keys, KECHO), KeyName(keys, KSILENT), FUNCNAME);
		goto errorExit;
	}
	
	/*
	   For vecread(), Quiet == 0 means to echo any skipped lines
	   for matread() and macroread(), Quiet == 0 means to echo header lines
	*/
	if (!logicalIsSet(Quiet))
	{
		if (Silent || op & IVECREAD)
		{ /* don't echo skipped lines*/
			verbose = 0;
			Quiet = 1;
		}
		else
		{
#ifdef HASCONSOLEDIALOG
			/* echo all header lines not starting with '))'*/
			verbose = 1;
#else  /*HASCONSOLEDIALOG*/
			/*
			  echo all header lines not starting '))' except
			  interactively typed ones
			*/
			verbose = (useConsole == interactiveConsole) ? 0 : 1;
#endif /*HASCONSOLEDIALOG*/
		}
	} /*if (!logicalIsSet(Quiet))*/
	else if (!Quiet && Silent)
	{
		sprintf(OUTSTR,
				"ERROR: %s:F and %s:T are incompatible on %s()",
				KeyName(keys, KQUIET), KeyName(keys, KSILENT), FUNCNAME);
		goto errorExit;
	}
	else if (!(op & IVECREAD))
	{
		verbose = (Quiet) ? 0 : 2; /* all header lines printed on quiet:F */
	}
	else
	{
		verbose = !Quiet;
	}
	
	if (logicalIsSet(CharRead) && logicalIsSet(ByWords))
	{
		if (CharRead && !ByWords || !CharRead && ByWords)
		{
			sprintf(OUTSTR,
					"ERROR: %s and %s on %s() must both be True or both be False",
					KeyName(keys,KCHAR), KeyName(keys,KBYWORDS), FUNCNAME);
			goto errorExit;
		}
		charKeyname = KeyName(keys,KBYWORDS);
	}
	else if (logicalIsSet(ByWords))
	{
		CharRead = ByWords;
 		charKeyname = KeyName(keys,KBYWORDS);
	}
	
	if ((logicalIsSet(CharRead) && CharRead ||
		 logicalIsSet(ByLines) && ByLines ||
		 logicalIsSet(ByChars) && ByChars) && 
		realIsSet(BadValue))
	{
		char      *keyname;
		
		if (logicalIsSet(CharRead) && CharRead)
		{
			keyname = charKeyname;
		}
		else if (logicalIsSet(ByLines) && ByLines)
		{
			keyname = KeyName(keys,KBYLINES);
		}
		else
		{
			keyname = KeyName(keys,KBYCHARS);
		}
		
		sprintf(OUTSTR,
				"ERROR: %s:T and %s:value cannot be be used together",
				keyname, KeyName(keys,KBADVAL));
		goto errorExit;
	}
	
	if (!logicalIsSet(ByLines))
	{
		setLogicalFalse(ByLines);
	}
	if (!logicalIsSet(ByChars))
	{
		setLogicalFalse(ByChars);
	}
	if (ByLines && ByChars)
	{
		sprintf(OUTSTR,
				"ERROR: %s:T is illegal with %s:T",
				KeyName(keys,KBYCHARS), KeyName(keys,KBYLINES));
		goto errorExit;
	}
	if (ByLines || ByChars)
	{
		if (logicalIsSet(CharRead) && !CharRead)
		{
			sprintf(OUTSTR,
					"ERROR: %s:T illegal with %s:F",
					(ByLines) ? KeyName(keys,KBYLINES) : KeyName(keys,KBYCHARS),
					charKeyname);
			goto errorExit;
		} /*if (logicalIsSet(CharRead) && !CharRead)*/
		CharRead = 1;
	} /*if (ByLines || ByChars)*/
	
	if (!logicalIsSet(CharRead))
	{
		setLogicalFalse(CharRead);
	}
	
	
	if (setPlace >= 0)
	{
		arg2 = COMPVALUE(list, setPlace);
		if (isKeyword(arg2))
		{
			if (op & IVECREAD)
			{
				badKeyword(FUNCNAME, isKeyword(arg2));
			}
			else
			{
				sprintf(OUTSTR,
						"ERROR: %s for %s() must not be given by keyword phrase",
						whatread, FUNCNAME);
			}
			
			goto errorExit;
		}
		else if (op & IVECREAD)
		{
			sprintf(OUTSTR,
					"ERROR: all non-filename arguments to %s() must be keyword phrases",
					FUNCNAME);
			goto errorExit;
		}
		
		if (!isScalar(arg2) || TYPE(arg2) != CHAR)
		{
			if (op != IMACROSEARCH)
			{
				notCharOrString(whatread);
			}
			goto errorExit;
		}
	} /*if (setPlace >= 0)*/
	else
	{
		arg2 = (Symbolhandle) 0;
	}
	
	if (!(op & IVECREAD))
	{
		if (arg2 != (Symbolhandle) 0)
		{
			setnameH = STRING(arg2);
			if (strlen(*setnameH) > MAXNAME)
			{
				sprintf(OUTSTR,
						"ERROR: data set name longer than %ld characters",
						(long) MAXNAME);
				goto errorExit;
			}
			else if ((*setnameH)[0] == '\0' || useConsole != nonConsole)
			{ /* setName is ""; seek first name */
				if (useConsole != nonConsole && !Silent)
				{
					sprintf(OUTSTR,
							"WARNING: %s ignored by %s() when reading from CONSOLE",
							whatread, FUNCNAME);
					putErrorOUTSTR();
				} /*if (useConsole != nonConsole && !Silent)*/
				/* setnameP points to a buffer of size MAXNAME + 1 */
				setnameH = &setnameP;
			}
		} /*if (arg2 != (Symbolhandle) 0)*/
		else
		{
			setnameH = &setnameP;
		}
	} /*if (!(op & IVECREAD))*/

	if (*OUTSTR)
	{
		goto errorExit;
	}

	if (!FromString && useConsole == nonConsole)
	{
		fname = *fnameH;
#ifdef HASFINDFILE
#ifdef MACINTOSH
		 /* macFindFile() default is 'TEXT' */
		fname = macFindFile(fname, "\pSpecify the data file", (STR255) 0,
							READIT, 0, (OSType *) 0, &ReadVolume);
#endif /*MACINTOSH*/

#ifdef WXWIN
		fname = wxFindFile(fname, "Specify the data file", (char *) 0);
#endif /*WXWIN*/
		if (fname == (char *) 0)
		{ /* open cancelled */
			if (!Silent)
			{
				sprintf(OUTSTR, "WARNING: %s() cancelled", FUNCNAME);
			}
			result = NULLSYMBOL;
			goto errorExit;
		}
#endif /*HASFINDFILE*/

		fname = expandFilename(fname);
		if (fname == (char *) 0 || !isfilename(fname))
		{
			goto errorExit;
		}
		fnameH = &fname;
	} /*if (!FromString && useConsole == nonConsole)*/

	LINE = mygethandle(MAXLINE+1);
	if (LINE == (char **) 0)
	{
		goto errorExit;
	}
	(*LINE)[0] = '\0';
	fn = (FILE *) 0;

	DataPrompt = (Prompt) ? DATAPROMPT : NullString;

#ifdef CONSOLEOK

	if (useConsole != nonConsole && Prompt)
	{
		if (op & IVECREAD)
		{
			char        termination[50];

			if (goChar != '\0')
			{
				sprintf(termination,
						"End by line not starting with '%c'",
						goChar);
			}
			else if (ByLines)
			{
				sprintf(termination,
						"End by line starting with '%c'", stopChar);
			}
			else
			{
				sprintf(termination,
						"End by line containing%s '%c'",
						(stopChar == skipChar) ? " but not starting with" : "",
						stopChar);
			}
			
			if (!ByLines && !ByChars)
			{
				sprintf(OUTSTR,
						"Type %s separated by spaces. %s",
						(CharRead) ? "unquoted strings" : "numbers or '?'",
						termination);
			}
			else
			{
				sprintf(OUTSTR, "Type lines.  %s", termination);
			}
		}
		else if (op & (IMATREAD | IMACROREAD))
		{
			sprintf(OUTSTR, "Type header, followed by %s",
					(strcmp(FUNCNAME, "matread") == 0) ? "data" : "macro");
		}
		else
		{ /* inforead() */
			sprintf(OUTSTR,
					"Type header, followed by comments starting with '%c'",
					COMMENTSTART);
		}
		
#ifndef HASCONSOLEDIALOG
		putOUTSTR();
#else /*HASCONSOLEDIALOG*/
		if (useConsole == interactiveConsole)
		{
			myAlert(OUTSTR);
			*OUTSTR = '\0';
		}
		else
		{
			putOUTSTR();
		}
#endif /*HASCONSOLEDIALOG*/
	} /*if (useConsole != nonConsole && Prompt)*/
	
#endif /*CONSOLEOK*/

	if (useConsole == nonConsole && !FromString)
	{
		fn = fmyopen(*fnameH, TEXTREADMODE);
		LastDataWasCr = 0;
		
		if (fn == (FILE *) 0)
		{
			sprintf(OUTSTR,
					"ERROR: %s() cannot open file %s", FUNCNAME, *fnameH);
		}
		else if (myfeof(fn))
		{/* no data */
			sprintf(OUTSTR, "ERROR: file %s is empty", *fnameH);
		}
		if (NoFileOK && *OUTSTR)
		{
			OUTSTR[0] = '\0';
			result = NULLSYMBOL;
			goto errorExit; /* not an error */
		} /*if (NoFileOK && *OUTSTR)*/
	} /*if (useConsole == nonConsole && !FromString)*/
	else if (useConsole != nonConsole)
	{ /* read from CONSOLE */
		fn = INPUTFILE[BDEPTH];
		fnameH = INPUTFILENAMES[BDEPTH];
	} /*if (useConsole == nonConsole && !FromString){}else if (useConsole != nonConsole){}*/
	else
	{
		fname = "string variable";
		fnameH = &fname;
		fn = (FILE *) 0;
	}

	if (*OUTSTR)
	{
		goto errorExit;
	}

	if (op & IVECREAD)
	{
		int    charOp;

		if (ByLines)
		{
			charOp = KBYLINES;
		}
		else if (ByChars)
		{
			charOp = KBYCHARS;
		}
		else if (CharRead)
		{
			charOp = KCHAR;
		}
		else
		{
			charOp = 0;
		}

		result = doReadVec(fn, fnameH, verbose, Echo, Silent, charOp,
						   BadValue, stopChar, goChar,
						   skipChar);
	} /*if (op & IVECREAD)*/
	else
	{
		result = doReads(fn, setnameH, fnameH, op, verbose, Echo, Silent,
						 NotFoundOK, Labels);
	} /*if (op & IVECREAD){}else{}*/

#ifdef MACINTOSH
	if (UseWindows && useConsole != nonConsole && !FromString && BDEPTH == 0)
	{
		InvalRect(&(CmdWind->portRect));
		macUpdate((WindowPtr) 0); /* update all windows */
	} /*if (UseWindows && useConsole != nonConsole && !FromString && BDEPTH == 0)*/
#endif /*MACINTOSH*/
	*OUTSTR = '\0';
/* fall through */

  errorExit:
	if (fn != STDIN && fn != (FILE *) 0 &&
		useConsole == nonConsole && !FromString)
	{
		fclose(fn);
	}
	if (op != IMACROSEARCH)
	{
		putErrorOUTSTR();
	}
	
	mydisphandle (LINE);
	LINE = (char **) 0;
	UNLOADSEG(findLine);
	return (result);
} /*readdata()*/

