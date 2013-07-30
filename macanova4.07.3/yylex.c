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
#pragma segment Yylex
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
#include "mainpars.h"

#ifdef MACINTOSH
#include "macIface.h"
#endif /*MACINTOSH*/

#ifdef WXWIN
#include "wx/wxIface.h"
#endif /*WXWIN*/

/*
   Define any special platform dependent character codes here for use 
   by yylex()
*/

#ifdef MACINTOSH
#define LESSTHANOREQUALTO  0xb2 /* Macintosh <= */
#define MORETHANOREQUALTO  0xb3 /* Macintosh >= */
#define NOTEQUALTO         0xad /* Macintosh != */
#define LEFTSPECIALQUOTE   0xc7 /* Macintosh << */
#define RIGHTSPECIALQUOTE  0xc8 /* Macintosh >> */
#endif /*MACINTOSH*/


/*
   Size of chunks to get storage for strings and macro arguments
   It is chosen so that the first piece (almost always the only one
   needed) will be available in the "warehouse" for list handle
*/

#define STRINGCHUNK  (DEFAULTLISTLENGTH * sizeof(Symbolhandle))

/* these must be defined identically in mainpars.y */
#define SKIPBRACKETBLOCK  3
#define SKIPPARENBLOCK    4

/*
  File contains routines related to lexical scanning.  The main entrance is
  yylex().  Most of the work is done in yylex1().

  930506: Dereferenced handle INPUTSTRING to inputString = *INPUTSTRING.
  Note this must be redone after every call that might cause memory to move,
  notably any .*Install(), mygethandle(), etc.

  950803  Removed all references to very obsolete type MACRO1

  950804  Implemented new token SEMI1 to be returned when ';' or newline
  is hit, BRACKETLEV > 0, and the next non-white character is not '}'.  This
  allows parser to remove more unwanted scratch variables

  950822  Included a check for appropriate preceding character (LASTCH) on
  several elements (for, while, else, elseif, quit) so as to give more
  informative error messages.  Also LASTCH is now set when a character
  is taken from ALTINPUTTOKENS.

  950929  Modified parseMacro so that an escaped quote (\") never changes
  the inquote status.  This parallels a similar change in getinput()
  (in commonio.c).
  The general philosophy is that a macro receives as argument exactly what
  is typed in, after trimming off leading and trailing white space

  960226 Added check for tokens MATDIV (%/%) and DIVMAT (%\%)

  960726 Allow quit(T), quit(F) in addition to quit() and quit.

  960816  Fixed skipBrackets so that an escaped quote (\") does not change
          inQuote status.

  970612  MDEPTH incremented when expanded (in-line) macro header is seen.

  970623  Changes to implement (a) out-of-line macros and evaluated
          strings

  970820  Modified skipBrackets to use new utility findBracket

  980617  scanNumber() no sets NOMISSING bit

  980826  Implemented new syntax element 'next' using new token NEXTREP

  980902  Fixed minor bug: Wrong error message on encountering {stop}

  981118  Added automatic search of macro files when 'name *(' encountered,
          where name is not the name of a defined symbol.

  981231  Fixed minor bug which was seen when break n was at the end of
          a line.

  990204  Macro names are now saved on a stack by PushMacroName() as expanded
          macros are encountered          

  990212  Changed most uses of putOUTSTR() to putErrorOUTSTR()
*/

#define Yylval       yylval


Symbolhandle         Install(), RInstall(), Lookup();
extern Symbolhandle  Yylval;

extern long     PARSEMACROARGS;	/* defined in mainpars.c */
extern long     YYLEXMESSAGE ; /* defined in mainpars.c */
extern long     BATCHBLOCKCOUNT; /*defined in mainpars.c*/
extern int     *ALTTOKENPTR;   /* defined in mainpars.c */
extern int      ALTINPUTTOKENS[];   /* defined in mainpars.c */
				/* if non-zero, replace next char if not ';'*/
/* These need to be defined in Macroset.c as well*/
#define HEADLENGTH 4

extern  char INLINEMACROHEAD[];   /*"{#)#"*/
extern  char OUTLINEMACROHEAD[];  /*"{#]#"*/
extern  char EVALUATIONHEAD[];    /*"{#}#"*/

extern long     LASTCH;

/*
  skip white space, allowing for line continuations

  950804 modified so as to skip comments when ignoreComments != 0
*/

#define SKIPCOMMENTS     1
#define DONTSKIPCOMMENTS 0

/*
   skip a comment.  If the line ends with '\\' then leave ISTRCHAR so that
   '\\' will be next character read
   (*INPUTSTRING)[ISTRCHAR] should be the character after '#'
   if skipCommment runs off end of line (no new line),
   (*INPUTSTRING)[ISTRCHAR] will be trailing '\0'
*/
static unsigned int skipComment(void)
{
	register unsigned char  c, c1;
	register unsigned char *inputString = *INPUTSTRING;

	while((c = inputString[ISTRCHAR]) != '\0' && !isNewline(c))
	{
		if (c == '\\' &&
			(c1 = inputString[ISTRCHAR + 1], isNewline(c1)))
		{ /* leave positioned before backslash newline*/
			break;
		}
		ISTRCHAR++;
	}
	return (c);
} /*skipComment()*/

/*
   Skip white space (' ' or '\t'), including unquoted newlines escaped
   by backslash.
   If ignoreComments != 0, treat comments like white space
   It returns the non-white character that terminates the skip or '\n'

   Returns '\n' if rest of line is white
   970623 returns '\n' if run off end of line
*/
static unsigned int skipWhiteSpace(int skipComments)
{
	register unsigned char  c, c1;
	register unsigned char *inputString = *INPUTSTRING;
	
	while((c = inputString[ISTRCHAR]) == ' ' || c == '\t' ||
		  skipComments != DONTSKIPCOMMENTS && c == '#' ||
		  (c == '\\' &&
		   (c1  = inputString[ISTRCHAR+1], isNewline(c1))))
	{
		ISTRCHAR++;
		if (c == '\\')
		{ /* skip escaped new line */
			ISTRCHAR++;
		}
		else if (c == '#')
		{ /* skipComments must be SKIPCOMMENTS */
			(void) skipComment();
			/*
			  Should be positioned after '\n', before backslash '\n',
			  or before '\0' in case of missing '\n'
			*/
		}
	}
	return ((c != '\0') ? (unsigned int) c : '\n');
} /*skipWhiteSpace()*/

#ifdef UNDEFINED__ /*no longer used*/
/*
  Scan *INPUTSTRING for match of s, i.e., match all characters in s and
  determine that next character in *INPUTSTRING is not alpha-numeric.
  If match is found, return non-zero, otherwise zero.
*/

static long matchKeyword(char *s)
{
	unsigned char   *inputString = *INPUTSTRING + ISTRCHAR - 1;
	
	while(*s)
	{
		if (*s++ != *inputString++)
		{
			return (0);
		}
	} /*while(*s)*/

	return (!isnamechar(*inputString));

} /*matchKeyword()*/
#endif /*UNDEFINED__*/

/*
   960816 Fixed bug so that escaped quote in quotes does not change inQuotes
   970820 Rewrote so that it uses new utility findBracket()
*/
static char skipBrackets(long goalLevel, char leftbracket)
{
	char             c;
	long             level = (leftbracket == '{') ? BRACKETLEV : PARENLEV;
	WHERE("skipBrackets");

	c = findBracket(goalLevel, leftbracket, *INPUTSTRING, &level,
					&ISTRCHAR, &LASTCH);

	if (leftbracket == '{')
	{
		BRACKETLEV = level;
	}
	else
	{
		PARENLEV = level;
	}
	/*
	  return the last character read;
	  if it was '\0', ISTRCHAR still points to it
	*/
	return (c);
} /*skipBrackets()*/


/*
  YYLEXMESSAGE == SKIPBRACKETBLOCK : skip compound statements on if, elseif,
  or while with condition F

  YYLEXMESSAGE == SKIPPARENBLOCK : skip '(...)' after elseif when condition
  from preceding if or elseif is T

  YYLEXMESSAGE > MAXWDEPTH : fakeout parser to get out of breaks or breakall
       or to skip to the end of a loop on next

*/

static unsigned int processBreak(void)
{
	unsigned char         *inputString = *INPUTSTRING;
	unsigned int           c;
	long                   currentBracketLev = BRACKETLEV;
	long                   currentParenLev = PARENLEV;
	long                   wantedBracketLev, extraBrackets;
	long                   i;
	WHERE("processBreak");

	ALTTOKENPTR = ALTINPUTTOKENS;
	*ALTTOKENPTR = 0;

	if (YYLEXMESSAGE == SKIPBRACKETBLOCK)
	{
		/*
		  Just got F condition on if or while, skip to end.
		  Should be positioned immediately before '{' or erroneous
		  character.
		  */

		YYLEXMESSAGE = 0;
		if ((c = inputString[ISTRCHAR++]) != '{')
		{/* must be erroneous character; return it */
			ALTTOKENPTR = (int *) 0;
			return (c);
		}

 		BRACKETLEV++;
		/* move ISTRCHAR to point after real compound statement */
		c = skipBrackets(currentBracketLev, c);
		BRACKETLEV = currentBracketLev + 1;
		/* create empty compound statement */
		if (c == '}')
		{
			i = 0;
			ALTINPUTTOKENS[i++] = (int) ';';
			ALTINPUTTOKENS[i++] = (int) '}';
			ALTINPUTTOKENS[i] = 0;
		} /*if (c == '}')*/
		else
		{/* end of line before matching '}' */
			sprintf(OUTSTR,
					"ERROR: need matching '{' and '}' after 'if', 'while', and 'for'");
			putErrorOUTSTR();
			ALTTOKENPTR = (int *) 0;
		} /*if (c == '}'){}else{}*/
		return ((unsigned int) '{');
	} /*if (YYLEXMESSAGE == SKIPBRACKETBLOCK)*/

	if (YYLEXMESSAGE == SKIPPARENBLOCK)
	{
		/*
		  Previous T condition on elseif; skip '(...)'.
		  Should be positioned immediately before '('
		  */

		YYLEXMESSAGE = 0;
		if ((c = inputString[ISTRCHAR++]) != '(')
		{/* must be erroneous character; return it (should not happen)*/
			ALTTOKENPTR = (int *) 0;
			return (c);
		}
 		PARENLEV++;
		/* move ISTRCHAR to point after matching ')' */
		c = skipBrackets(currentParenLev, c);
		PARENLEV = currentParenLev + 1;
		/* create empty paren list */
		if (c == ')')
		{
			i = 0;
			ALTINPUTTOKENS[i++] = (int) ')';
			ALTINPUTTOKENS[i] = 0;
		}
		else
		{			/* end of line before matching ')' */
			ALTTOKENPTR = (int *) 0;
		}
		return ((unsigned int) '(');
	} /*if (YYLEXMESSAGE == SKIPPARENBLOCK)*/

	/*
	  If YYLEXMESSAGE > MAXWDEPTH it should be of form (wanted
	  bracketlevel) + MAXWDEPTH + 1  and we are just past
	  "break", "breakall" or "next".  We need to set up ALTINPUTTOKENS
	  with enough ';}' to fake out the parser to think
	  that we are getting out of any enclosing if's, as well
	  as enough ';}' to convince it that we are getting out of
	  the enclosing while (break) or all the enclosing whiles
	  (breakall)
	  In every case the character we return now will be ';'
	  */

	if (YYLEXMESSAGE > MAXWDEPTH)
	{/* must be break or breakall or next */
		wantedBracketLev = YYLEXMESSAGE - MAXWDEPTH - 1;

		YYLEXMESSAGE = 0;
		*ALTTOKENPTR++ = (int) '}';
		extraBrackets = currentBracketLev - wantedBracketLev - 1;
		for(i = 0;i< extraBrackets;i++)
		{
			*ALTTOKENPTR++ = (int) ';';
			*ALTTOKENPTR++ = (int) '}';
		}
		c = skipBrackets(wantedBracketLev, '{');

		if (c != (int) '}') /* end of line before matching '}' */
		{
			sprintf(OUTSTR,"ERROR: missing '}'");
			putErrorOUTSTR();
			ALTTOKENPTR--; /* wipe out one '}' to force error in parser */
		}
		*ALTTOKENPTR = 0;
		ALTTOKENPTR = ALTINPUTTOKENS;
		BRACKETLEV = currentBracketLev;
		return ((unsigned int) ';');
	} /*if (YYLEXMESSAGE > MAXWDEPTH)*/
	return (BADTOKEN); /* shouldn't ever happen */
} /*processBreak()*/

#define MACROPUNCT    1
#define MACROARG      2

/*
  create CHAR symbols containing macro arguments.  Called by
  yylex when PARSEMACROARGS != 0.

  If PARSEMACROARGS == MACROPUNCT, the next character is known to be
  '(', ',' , ')', or '\n'
  Otherwise, it encapsulates INPUTSTRING up to next one of these characters
  outside of quotes and at the same bracket level as an argument

  In initially collects characters in stringBuff.  If that fills up, then
  temporary storage is allocated for collection.  There should be no practical
  limit on the length of a macro argument.
*/
static unsigned int parseMacro(void)
{
	char            stringBuff[STRINGCHUNK];
	char           *tempString;
	char          **tempStringH = (char **) 0;
	unsigned char  *inputString = *INPUTSTRING;
	unsigned int    c, c1;
	long            macroParenLevel = 0, inQuotes;
	int             empty = 1, escaped = 0;
	long            place, nleft;
	WHERE("parseMacro");

	Yylval = (Symbolhandle) 0;

/*
   PARSEMACROARGS == MACROPUNCT means next character is known to be
   '(', ',', ')' or '\n'
*/
	if (PARSEMACROARGS == MACROPUNCT)
	{
		c = inputString[ISTRCHAR++];
		if (c == '(' || c == ',')
		{
			/* expect macro argument next time in */
			PARSEMACROARGS = MACROARG;
			if (c == '(')
			{
				PARENLEV++;
			}
		}
		else /* ')' or end of line */
		{
			PARSEMACROARGS = 0;
			if (c == ')')
			{
				PARENLEV--;
			}
			else
			{
				c = ENDOFL;
			}
		}
		return (c);
	} /*if (PARSEMACROARGS == MACROPUNCT)*/

	/*
	  PARSEMACROARGS must be MACROARG.
	  Convert everything up to ',' or ')' with macroParenLevel = 0 to CHAR
	  */

	/* skip leading white space, allowing for line continuations */
	c = skipWhiteSpace(DONTSKIPCOMMENTS);
	if (isNewline(c) || c == '\0')
	{
		PARSEMACROARGS = 0;
		return ((c != '\0') ? ENDOFL : c);
	}

	/* at this point c is non-white and should be the same as 
	   (*INPUTSTRING)[ISTRCHAR]
	*/
	Yylval = Install(SCRATCH, CHAR);
	if (Yylval == (Symbolhandle) 0)
	{
		return (FATALERROR);
	}

	setNDIMS(Yylval,1);
	setDIM(Yylval,1,1);

	macroParenLevel = 0;
	inQuotes = 0;
	place = 0;
	tempString = stringBuff;
	nleft = STRINGCHUNK - 1;

	/* collect a macro argument in tempString*/
	while(1)
	{
		if (nleft == 0)
		{
			/* no room; allocate overflow space */
			if (tempStringH == (char **) 0)
			{
				/* must have just filled up stringBuff */
				tempStringH = mygethandle(place + STRINGCHUNK);
				if (tempStringH != (char **) 0)
				{
					stringBuff[place] = '\0';
					strcpy(*tempStringH, stringBuff);
				}
			}
			else
			{
				tempStringH = mygrowhandle(tempStringH, place + STRINGCHUNK);
			}
			setSTRING(Yylval, tempStringH);
			if (tempStringH == (char **) 0)
			{
				c = FATALERROR;
				goto clearYylval;
			}
			nleft = STRINGCHUNK - 1;
			tempString = *tempStringH;
			inputString = *INPUTSTRING;
		} /*if (nleft == 0)*/
		
		ISTRCHAR++;
		while(!inQuotes && c == '\\' &&
			  (c1 = inputString[ISTRCHAR] , isNewline(c1)))
		{/* allow for continuation lines */
			ISTRCHAR++;
			c = inputString[ISTRCHAR++];
		}
		if (c == '\0')
		{
			ISTRCHAR--;
			break;
		}

		tempString[place++] = c;
		nleft--;
		if (c == '"')
		{
			if (!escaped)
			{
				inQuotes = !inQuotes;
			}
		}
		else if (!inQuotes &&
				 ((macroParenLevel <= 0 && (c == ',' || c == ')')) ||
				  isNewline(c)))
		{						/* found end of argument */
			/* put back character for next call */
			ISTRCHAR--;
			place--;
			nleft++;
			PARSEMACROARGS = (c == ',') ? MACROPUNCT : 0;
			break;
		}
		escaped = (c == '\\') ? !escaped : 0;		
		if (empty && !isspace(c))
		{
			empty = 0;
		}
		if (!inQuotes)
		{
			if (c == '(' || c == '[')
			{
				macroParenLevel++;
			}
			else if (c == ')' || c == ']')
			{
				macroParenLevel--;
			}
		} /*if (!inQuotes)*/
		c = inputString[ISTRCHAR];
	} /*while(1)*/
	tempString[place] = '\0';

	while(place > 0 && ((c1 = tempString[place-1]) == ' ' || c1 == '\t'))
	{ /* trim off trailing white space */
		place--;
	}

	if (c == '\0')
	{ /* run out of string before finished parsing macro args*/
		PARSEMACROARGS = 0;
		goto clearYylval;
	} /*if (c == '\0')*/

	if (place == 0 || empty)
	{
		/*
		  On empty macro argument, '()', '(,...)', '(...,,...)', or
		  '(...,)' , return null symbol, and token VAR.
		  White argument (e.g., ', ,') is also considered null
		  */
		c = VAR;
		goto clearYylval;
	} /*if (place == 0 || empty)*/
	else if (place > 0)
	{
		tempString[place++] = '\0';
		if (tempStringH == (char **) 0)
		{
			tempStringH = mygethandle(place);
			if (tempStringH != (char **) 0)
			{
				strcpy(*tempStringH, stringBuff);
			}
		}
		else
		{
			tempStringH = mygrowhandle(tempStringH, place);
		}
		setSTRING(Yylval, tempStringH);
		if (tempStringH == (char **) 0)
		{
			c = FATALERROR;
			goto clearYylval;
		}
		c = VAR; /* macro argument has been transformed to CHAR var */
	}

	return (c);

  clearYylval:
	Removesymbol(Yylval);
	Yylval = (Symbolhandle) 0;
	return (c);

} /*parseMacro()*/

/*
  980617 NOMISSING bit is set on successful scan
*/
static unsigned int scanNumber(void)
{
	unsigned char      *inputString = *INPUTSTRING, *endptr;
	double              y, absy;
	WHERE("scanNumber");
	
	y = mystrtod((char *) inputString+ISTRCHAR, (char **) &endptr);

	if (y == 0 && endptr == inputString+ISTRCHAR)
	{ /* should not happen */
		ISTRCHAR++;
		yyerror("ERROR: nonnumeric where numeric expected");
		return (ERROR);
	}

	if ((absy = fabs(y), doubleEqualBdouble(absy, TooBigValue)))
	{
		ISTRCHAR += endptr - (inputString+ISTRCHAR);
		yyerror("ERROR: value too big to be represented in computer");
		return (ERROR);
	}

	ISTRCHAR = endptr - inputString;

	Yylval = RInstall(NUMSCRATCH, (long) 1);
	if (Yylval == (Symbolhandle) 0)
	{
		return (FATALERROR);
	}
	DATAVALUE(Yylval,0) = y;
#if (USENOMISSING)
	setNOMISSING(Yylval);
#endif /*USENOMISSING*/

	return (NUMBER);
} /*scanNumber()*/

#define MAXTOKENSCAN 25
/*
   Check whether string in token is a syntax element (if, else, elseif, 
   break, breakall, for, while, end, quit, stop).

   If token is not a syntax element, 0 is returned;  otherwise the
   parsing token is returned (IF, ELSE, ELSEIF, BREAK, BREAKALL, FOR,
   WHILE, END)

   950912 replaced calls to strncmp() with character by character comparisons
*/
static int isSyntaxElement(char token [])
{
	char           c0 = token[0];
	char           c1 = token[1], c2 = token[2], c3 = token[3];
	char           c4, c5;

	switch (c0)
	{
	  case 'i':
		/* if */
		return ((c1 == 'f' && c2 == '\0') ? IF : 0);

	  case 'e':
		if (c1 == 'l')
		{ /* else and elseif*/
			if (c2 != 's' || c3 != 'e')
			{
				return (0);
			}
			c4 = token[4];
			if (c4 == '\0')
			{
				return (ELSE);
			}
			return ((c4 == 'i' && token[5] == 'f' && token[6] == '\0') ?
					ELSEIF : 0);
		} /*if (c1 == 'l')*/
		else if (c1 == 'x')
		{/* exit*/
			return ((c2 == 'i' && c3 == 't' && token[4] == '\0') ?
					END : 0);
		}
		/* end */
		return ((c1 == 'n' && c2 == 'd' && c3 == '\0') ? END : 0);

	  case 'f':
		/* for */
		return ((c1 == 'o' && c2 == 'r' && c3 == '\0') ? FOR : 0);

	  case 'w':
		/* while */
		return ((c1 == 'h' && c2 == 'i' && c3 == 'l' && token[4] == 'e' &&
				 token[5] == '\0') ? WHILE : 0);

	  case 'n':
		/* next */
		return ((c1 == 'e' && c2 == 'x' && c3 == 't') ? NEXTREP : 0);

	  case 'b':
		if (c1 == 'r')
		{ /* break and breakall*/
			if (c2 != 'e' || c3 != 'a' || token[4] != 'k')
			{
				return (0);
			}
			c5 = token[5];
			if (c5 == '\0')
			{
				return (BREAK);
			}
			return ((c5 == 'a' && token[6] == 'l' && token[7] == 'l' &&
					 token[8] == '\0') ? BREAKALL : 0);
		}

		/* bye */
		return ((c1 == 'y' && c2 == 'e' && c3 == '\0') ? END : 0);

	  case 'q':
		/* quit */
		return ((c1 == 'u' && c2 == 'i' && c3 == 't' && token[4] == '\0') ?
				END : 0);

	  case 's':
		/* stop */
		return ((c1 == 't' && c2 == 'o' && c3 == 'p' && token[4] == '\0') ?
				END : 0);
	  default:
		return (0);
	} /*switch (c)*/
} /*isSyntaxElement()*/

/*
  981117 function to search standard macro files for
         a macro named macroName.  If it finds it, its Symbolhandle
         is returned.  Otherwise (and this includes the case when an
         object that is not a macro is found), it returns 0.

		 Its behavior depends on the value of global MACROSEARCH.  If its value
         is 0, no search is done; if the value is 1 a search with a warning
         message is done; if the value is 2, a search is done but no
         warning message is given
  981121 Removed test opening of files, since readdata() was modified to
         not treat an unopenable file as an error.
*/

static Symbolhandle searchForMacro(char macroName [])
{
	Symbolhandle      macroFiles = Lookup("MACROFILES");
	Symbolhandle      macroSymbol = (Symbolhandle) 0;
	long              iFile, nFiles;
	WHERE("searchForMacro");
	
	if (MACROSEARCH != noMacroSearch)
	{
		if (!isVector(macroFiles) || TYPE(macroFiles) != CHAR)
		{
			macroFiles = Lookup("MACROFILE");
			if (!isCharOrString(macroFiles))
			{
				return ((Symbolhandle) 0);
			}
		}

		if (MACROSEARCH == verboseMacroSearch)
		{
			sprintf(OUTSTR,
					"WARNING: searching for unrecognized macro %s", macroName);
			ISTRCHAR++; /* include '(' in warning message */
			yyerror(OUTSTR);
			ISTRCHAR--;
		}
		
		/*
		  Successively try to open each file
		  */
		nFiles = symbolSize(macroFiles);
	
		for (iFile = 0; iFile < nFiles; iFile++)
		{
			char       *fileName = skipStrings(STRINGPTR(macroFiles),iFile);

			if (fileName[0] != '\0')
			{
				Symbolhandle list;
				long         typeList[2];
				char        *valueList[2];
				char        *keyList[2];
				char         thisFileName[PATHSIZE+1];
				char         thisMacroName[NAMELENGTH + 1];

				typeList[0] = typeList[1] = CHAR;

				strncpy(thisFileName, fileName, PATHSIZE);
				thisFileName[PATHSIZE] = '\0';
				valueList[0] = thisFileName;

				strncpy(thisMacroName, macroName, NAMELENGTH);
				thisMacroName[NAMELENGTH] = '\0';
				valueList[1] = thisMacroName;

				keyList[0] = keyList[1] = NullString;

				list = Buildlist(2, typeList, valueList, keyList);

				if (list != (Symbolhandle) 0)
				{
					char    funcname[NAMELENGTH + 1];
					
					strncpy(funcname, FUNCNAME, NAMELENGTH);
					funcname[NAMELENGTH] = '\0';
					strcpy(FUNCNAME, "macrosearch");
					OUTSTR[0] = '\0';
					macroSymbol = readdata(list);
					Removelist(list);
					strcpy(FUNCNAME, funcname);
					
					if (macroSymbol != (Symbolhandle) 0 &&
						TYPE(macroSymbol) == MACRO)
					{
						break;
					}
					Removesymbol(macroSymbol);
					macroSymbol = (Symbolhandle) 0;
				} /*if (list != (Symbolhandle) 0)*/
			} /*if (fileName[0] != '\0')*/
		} /*for (iFile = 0; iFile < nFiles; iFile++)*/
	} /*if (MACROSEARCH != noMacroSearch)*/
	if (macroSymbol == (Symbolhandle) 0 && MACROSEARCH == verboseMacroSearch)
	{
		sprintf(OUTSTR,
				"WARNING: macro %s not found on external macro files",
				macroName);
		putErrorOUTSTR();
	}
	
	return (macroSymbol);
} /*searchForMacro()*/

/*
  yylex1() is where all the real work of identifying tokens is done
  981231 eliminated label 'start' and several goto's, replacing them by
         a while (1) loop and continue statements
  990204 Names of expanded macros are pushed onto stack as they are encountered
*/
static unsigned int yylex1(void)
{
	unsigned char       *inputString;
	char                 stringBuff[STRINGCHUNK];
	char                *tempString;
	char               **tempStringH = (char **) 0;
	int                  nleft;
	unsigned int         c, c1, c2;
	double               y = -1.0;
	long                 i, j, nameStart,tokenLength, savedPlace;
	long                 place;
	char                 token[MAXTOKENSCAN+1];
	unsigned char       *endptr;
	int                  errorToken;
	WHERE("yylex1");

	while(1)
	{
		inputString = *INPUTSTRING;
		while ((c = (unsigned int) inputString[ISTRCHAR]) == ' ' || c == '\t')
		{
			ISTRCHAR++;				/* skip whitespace */
		}

		if (c == '\0')
		{
			int     outlinemacro = !isscratch(ThisInputname);
		
			if (INPUTLEVEL == 0)
			{
				return ((BRACKETLEV != 0) ? BADTOKEN : 0);
			}
#if (0) /* cleanitup now cleans all of them */
			mydisphandle((char **) ThisInputstring);
			ThisInputstring = (unsigned char **) 0;
#endif /*0*/

			popInputlevel();

			if (outlinemacro)
			{
				continue;
			}

#if (0)
			/*
			  971206 disabled check for new line to avoid problem; not sure why it is
			  here so I hope no problem has been introduced.
			  */
			if (isNewline((*INPUTSTRING)[ISTRCHAR]))
			{
				ISTRCHAR++;
			}
#endif /*0*/
			return (c); /* c is still '\0' which tells parser to quit */

		} /*if (c == '\0')*/
	
	
		if (c == '}' && LASTCH != ';' && LASTCH != ENDOFL)
		{/* force ';' before '}' */
			c = ';';
			return(c);
		}

		ISTRCHAR++;

		switch (c)
		{
		  case '\n':
#ifndef MPW
		  case '\r':
#endif /*MPW*/
			if (BRACKETLEV > 0)
			{
				if (LASTCH == ';' || LASTCH == ENDOFL)
				{
					continue;
				}
				return (((c = skipWhiteSpace(SKIPCOMMENTS)) != '}') ?
						SEMI1 : ENDOFL);
			} /*if (BRACKETLEV > 0)*/

			if (c = skipWhiteSpace(SKIPCOMMENTS), !isNewline(c))
			{
				yyerror("ERROR: probably too many }'s in macro prior to or");
				return (ERROR);
			}
			return(ENDOFL);

		  case ';':
			if (BRACKETLEV > 0 && (c1 = skipWhiteSpace(SKIPCOMMENTS)) != '}')
			{
				if (c1 == ';')
				{
					ISTRCHAR++;
				}
				c = SEMI1;
			} /*if (BRACKETLEV > 0 && (c1 = skipWhiteSpace(SKIPCOMMENTS)) != '}')*/
		
			return (c);

			/* simply pass through certain math symbols */
		  case '+':
		  case '-':
		  case '/':
		  case '$':
		  case ',':
		  case '\'':
			return (c);

		  case '(':
			PARENLEV++;
			return (c);
		  case ')':
			PARENLEV--;
			if (PARENLEV < 0)
			{
				yyerror("ERROR: too many )'s");
				c = ERROR;
			}
			return (c);
		  case '[':
		  case ']':
			if (inputString[ISTRCHAR] == c)
			{
				/* start of '[[' or ']]'*/
				c = (c == '[') ? LEFTBRACK2 : RIGHTBRACK2;
			}
			return (c);

		  case '*':
			/* "**" is equivalent to "^" */
			if ((unsigned int) inputString[ISTRCHAR] != '*')
			{
				return (c);
			}
			ISTRCHAR++;
			/* fall through */
		  case '^':
			return (POW);
		  case '<':
		  case '>':
			if (inputString[ISTRCHAR] == c)
			{
				if (c == '>')
				{
					ISTRCHAR++;
					return (RIGHTANGLE);
				}

				for(i=2;inputString[ISTRCHAR-1+i] == '<';i++)
				{
					;				/* count number of successive '<'s */
				}

				if ((i & 1) == 0)
				{					/* even number of '<'s */
					MACROSTART = ISTRCHAR-1;
					ISTRCHAR++;
					return (LEFTANGLE);
				}
				else
				{/* probably of form expr <<<name>> parsed as expr < <<name>> */
					return ('<');
				}

			}
			/* fall through */

		  case '=':
		  case '!':
			/* '<', '>', '='. '!' */
			if ((c2 = (unsigned int) inputString[ISTRCHAR++]) == '=')
			{
				if (c == '=')
				{
					c = EQ;
				}
				else if (c == '>')
				{
					c = GE;
				}
				else if (c == '<')
				{
					c = LE;
				}
				else
				{
					c = NE;
				}
			}
			else if (c == '<' && c2 == '-')
			{
				c = ASSIGN;
				c1 = (unsigned int) inputString[ISTRCHAR];
				if (strchr("+*-/^%",(int) c1))
				{
					c2 = (unsigned int) inputString[ISTRCHAR+1];
					if (c1 == '+' || c1 == '-')
					{ /* must have white space after <-+ and <-- */
						if (c2 == '\\')
						{
							savedPlace = ISTRCHAR++;
							(void) skipWhiteSpace(DONTSKIPCOMMENTS);
							for(i=savedPlace;i<ISTRCHAR;i++)
							{
								c2 = inputString[i];
								if (c2 == ' ' || c2 == '\t')
								{
									break;
								}
							}
							ISTRCHAR = savedPlace;
						}
						if (c2 != ' ' && c2 != '\t')
						{
							c2 = '\0';
						}					
					} /*if (c1 == '+' || c1 == '-')*/
				
					if (c2 != '\0')
					{
						ISTRCHAR++;
						switch (c1)
						{
						  case '+':
							c = ASSIGNADD;
							break;

						  case '-':
							c = ASSIGNSUB;
							break;

						  case '*':
							if (c2 == '*')
							{
								c = ASSIGNPOW;
								ISTRCHAR++;
							}
							else
							{
								c = ASSIGNMULT;
							}
							break;

						  case '/':
							c = ASSIGNDIV;
							break;

						  case '%':
							if (c2 == '%')
							{
								c = ASSIGNMOD;
								ISTRCHAR++;
							}
							else
							{
								ISTRCHAR--;
							}
							break;

						  case '^':
							c = ASSIGNPOW;
							break;
						} /*switch (c1)*/
					} /*if (c2 != '\0') */
				} /*if (strchr("+*-/^%",(int) c1))*/
			}/*else if (c == '<' && c2 == '-')*/
			else
			{
				ISTRCHAR--;
			}
			return (c);

#ifdef LESSTHANOREQUALTO
		  case LESSTHANOREQUALTO: /* special code for <= */
			return (LE);
#endif /*LESSTHANOREQUALTO*/

#ifdef MORETHANOREQUALTO
		  case MORETHANOREQUALTO: /* special code for >= */
			return (GE);
#endif /*MORETHANOREQUALTO*/

#ifdef NOTEQUALTO
		  case NOTEQUALTO: /* special code for >= */
			return (NE);
#endif /*NOTEQUALTO*/

#ifdef LEFTSPECIALQUOTE /* special code for << */
		  case LEFTSPECIALQUOTE:
			return (LEFTANGLE);
#endif /*LEFTSPECIALQUOTE*/

#ifdef RIGHTSPECIALQUOTE /* special code for >> */
		  case RIGHTSPECIALQUOTE:
			return (RIGHTANGLE);
#endif /*RIGHTSPECIALQUOTE*/

		  case ':':
			return (ASSIGN1);
		  case '&': /* '&&' equivalent to '&' and '||' equivalent to '|' */
		  case '|':
			if (inputString[ISTRCHAR] == c)
			{
				ISTRCHAR++;
				c = (c == '&') ? AND : OR;
			}
			return (c);

		  case '%':
			c2 = inputString[ISTRCHAR++];
			switch (c2)
			{
			  case '%':
				c = MOD; /* '%%' */
				break;
#ifdef BITOPS
			  case '!':
				c = BITNOT; /* '%!' */
				break;
			  case '&':
				c = BITAND; /* '%&' */
				break;
			  case '^':
				c = BITXOR; /* '%^' */
				break;
			  case '|':
				c = BITOR; /* '%|' */
				break;
#endif /*BITOPS*/

			  default:
				if (inputString[ISTRCHAR++] == '%')
				{
					switch (c2)
					{
					  case '*':
						c = MATMULT;   /* '%*%' */
						break;
					  case 'c':
						c = TRMATMULT;   /* '%c%' */
						break;
					  case 'C':
						c = MATMULTTR;   /* '%C%' */
						break;
					  case '/':
						c = MATDIV;   /* '%/%' */
						break;
					  case '\\':
						c = DIVMAT;   /* '%\\%' */
						break;
					  default:
						ISTRCHAR--;
						c = BADTOKEN;
					} /*switch (c2)*/
				} /*if (inputString[ISTRCHAR++] == '%') */
				else 
				{
					ISTRCHAR--;
					c = BADTOKEN;
				}
				break;
			} /*switch (c2)*/

			return (c);

		  case '?':					/* missing value */
			Yylval = RInstall(NUMSCRATCH, (long) 1);
			if (Yylval == (Symbolhandle) 0)
			{
				return (FATALERROR);
			}

			setNDIMS(Yylval,1);
			setDIM(Yylval,1,1);
			setMissing(DATAVALUE(Yylval,0));

			return (NUMBER);		/* missing value */

		  case '"':					/* start of string */
			inputString = *INPUTSTRING;
			Yylval = Install(STRINGSCRATCH, CHAR);
			if (Yylval == (Symbolhandle) 0)
			{
				return (FATALERROR);
			}
			setNDIMS(Yylval,1);
			setDIM(Yylval,1,1);

			place = 0;  /* position in string being collected */
			tempString = stringBuff;
			nleft = STRINGCHUNK - 1;
			/*
			  First use stringBuff as collecting place.  Only when that has
			  been filled allocate new space.
			  */
			while (1)
			{
				if (nleft == 0)
				{ /* need more room; get room for STRINGCHUNK more characters */
					if (tempStringH == (char **) 0)
					{ /* tempStringH not yet allocated */
						tempStringH = mygethandle(place + STRINGCHUNK);
						if (tempStringH != (char **) 0)
						{
							stringBuff[place] = '\0';
							strcpy(*tempStringH, stringBuff);
						}
					}
					else
					{ /* reallocate tempStringH */
						tempStringH = mygrowhandle(tempStringH,
												   place + STRINGCHUNK);
					}
					setSTRING(Yylval, tempStringH);
					if (tempStringH == (char **) 0)
					{
						errorToken = FATALERROR;
						goto errorExit;
					}
					nleft = STRINGCHUNK - 1;
					tempString = *tempStringH;
					inputString = *INPUTSTRING;
				} /*if (nleft == 0)*/
			
				c = inputString[ISTRCHAR++];
				if (c == '\0')
				{
					ISTRCHAR--;
					c = '"';
				}
				if (c == '"')
				{
					break;
				}

				if (c == '\\')
				{/* inside a string, so always accept escaped characters */
					/*
					  diad			  result
					  '\n'				NL
					  '\r'				CR
					  '\t'				TAB
					  '\ijk', i < 4		decoded ascii character
					  '\EOL'			'\EOL', where EOL is '\0'
					  '\somethingElse'	'somethingElse'
					  */
					c2 = inputString[ISTRCHAR++];
					if (c2 == '\0')
					{ /* run off end of *INPUTSTRING; fake closing quote */
						c = '"';
						ISTRCHAR--;
					}
					else if (isNewline(c2))
					{ /* '\' at end of line within string */
						ISTRCHAR--;
					}
					else if (c2 == 'n')
					{ 
						c = '\n';
					}
					else if (c2 == 'r')
					{
						c = '\r';
					}
					else if (c2 == 't')
					{
						c = '\t';
					}
					else if (isdigit(c2) && c2 < '8')
					{/* octal characters, e.g., \012 */
						savedPlace = ISTRCHAR;
						c = c2 - '0';
						for(j=1;j<3 ;j++)
						{
							c2 = inputString[ISTRCHAR++];
							if (!isdigit(c2) || c2 > '7')
							{
								break;
							}
							c = 8*c + c2 - '0';
						} /*for(j=1;j<3 ;j++)*/
						if (j < 3)
						{
							ISTRCHAR--;
						}
						if (c == '\0')
						{
							yyerror("ERROR: illegal octal null character");
							errorToken = BADTOKEN;
							goto errorExit;
						}
						else if (c > 0377)
						{ /* use only first two digits */
							c /= 8;
							ISTRCHAR--;
						}
					}
					else
					{
						c = c2;
					}
				} /*if (c == '\\')*/
				tempString[place++] = c;
				nleft--;
			} /*while (1)*/

			tempString[place++] = '\0';
			if (tempStringH == (char **) 0)
			{
				tempStringH = mygethandle(place);
				if (tempStringH != (char **) 0)
				{
					strcpy(*tempStringH, stringBuff);
				}
			}
			else
			{
				tempStringH = mygrowhandle(tempStringH, place);
			}
			setSTRING(Yylval,tempStringH);
			if (tempStringH == (char **) 0)
			{
				errorToken = FATALERROR;
				goto errorExit;
			}

			return (VAR); /* quoted string*/

		  case '{': /* start of compound statement, expanded macro or evaluation*/
			BRACKETLEV++;

			/* following assumes HEADLENGTH is 4 */
#if (HEADLENGTH != 4)
			ERROR
#endif

			  if (inputString[ISTRCHAR] == INLINEMACROHEAD[1] &&
				  inputString[ISTRCHAR+2] == INLINEMACROHEAD[3])
			  {
				  unsigned char        headType = inputString[ISTRCHAR+1];
				  int                  inlineMacro, outlineMacro, evaluation;
				  
				  inlineMacro = (headType == INLINEMACROHEAD[2]);
				  outlineMacro = (headType == OUTLINEMACROHEAD[2]);
				  evaluation = (headType == EVALUATIONHEAD[2]);

				  if (inlineMacro || outlineMacro || evaluation)
				  {				
					  char        macroName[NAMELENGTH + 1];
					  int         headToken;
					  
					  /*
						start of expanded macro signaled by "{#)#" (inline)
						or "{#]#" (out-of-line); start of evaluated string
						signalled by "{#}#"
						*/
			
					  if (ALTTOKENPTR != (int *) 0)
					  {
						  for(i = 0; ALTTOKENPTR[i] != 0; i++)
						  { /* find end of ALTTOKENPTR list */
							  ;
						  } /*for(i = 0; ALTTOKENPTR[i] != 0; i++)*/
					  } /*if (ALTTOKENPTR != (int *) 0)*/
					  else
					  {
						  ALTTOKENPTR = ALTINPUTTOKENS;
						  i = 0;
					  }

					  /* sequence '{' '#' signals macro body or evaluated string */
					  if (inlineMacro || outlineMacro)
					  {
						  unsigned char  *instring = inputString+ISTRCHAR+3;
						  int             place = 0;
						  
						  macroName[0] = '\0';

						  if (isnamestart(*instring))
						  {
							  do
							  {
								  macroName[place++] = *instring++;
							  } while (isnamechar(*instring) &&
									   place < NAMELENGTH);
							  macroName[place] = '\0';
						  }
					  }
					  if (inlineMacro)
					  {
						  headToken = INLINEHEAD;
						  /* 
							 970614 MLEVEL incremented ; it should be decremented
							 by parser
							 */
						  if (++MLEVEL > MAXLEVEL)
						  {
							  c = ERROR;
						  } /*if (MLEVEL > MAXLEVEL)*/
					  }
					  else if (outlineMacro)
					  {
						  headToken = OUTLINEHEAD;
						  /* 
							 970615 MDEPTH incremented ; it should be decremented
							 by parser
							 */
						  if (++MDEPTH > MAXMDEPTH)
						  {
							  c = ERROR;
						  } /*if (MDEPTH > MAXMDEPTH)*/
					  }
					  else
					  {
						  headToken = EVALHEAD;
					  }
				
					  if (c == ERROR)
					  {
						  sprintf(OUTSTR,
								  "ERROR: macros nested too deeply or too many active");
						  yyerror(OUTSTR);
			
					  }
					  else
					  {
						  if (!evaluation)
						  {
							  PushMacroName(macroName);
						  }
						  
						  /* set next token to be returned after '{' */
						  ALTTOKENPTR[i++] = headToken;
						  ALTTOKENPTR[i] = 0;
					  }
				  }
			  }
		
			return (c);

		  case '}':					/* end of one level of compound statement */
			BRACKETLEV--;
			if (BRACKETLEV < 0)
			{
				yyerror("ERROR: too many }'s");
				c = ERROR;
			}
			return (c);

		  case '#': /* start of comment; skip rest of line */
			c = skipComment();

			if (c != '\0')
			{ /* hit '\n' or '\r' */
				if (c == '\\')
				{ /* must have been "\\\n" */
					continue;
				}
				ISTRCHAR++;
			}

			return (ENDOFL);

		  case '\\':				/* may be continuation line */
			if (c2 = inputString[ISTRCHAR], isNewline(c2))
			{
				ISTRCHAR++;
				continue;
			}
			/* else fall through */

		  default:					/* see if number or variable name*/
			if (!isnamestart(c) && !isdigit(c) && c != '.' && c != TEMPPREFIX)
			{/*illegal character*/
				yyerror("ERROR: illegal character");
				return (ERROR);
			}

			c2 = inputString[ISTRCHAR]; /* next character*/
			if ((c == 'T' || c == 'F') && !isnamechar(c2))
			{ /* treat T and F special so they don't need to be in symbol table */
				Yylval = RInstall(LOGICSCRATCH,(long) 1);
				if (Yylval == (Symbolhandle) 0)
				{
					return (FATALERROR);
				}
				setTYPE(Yylval,LOGIC);
				DATAVALUE(Yylval,0) = (c == 'T') ? 1.0 : 0.0;
				return (VAR);
			}

			/* c is alpha or '@' or '.' or digit */
			nameStart = ISTRCHAR-1;	/* save loc of 1st char of name */
			if (isnamestart(c) || c == TEMPPREFIX)
			{/* may be keyword, name, or temporary name */
				tokenLength = 0;
				token[0] = c;
				while (isnamechar(c = inputString[ISTRCHAR]))
				{
					ISTRCHAR++;
					if (tokenLength < MAXTOKENSCAN-1)
					{
						token[++tokenLength] = c;
					}
					else
					{
						break;
					}
				} /*while (isnamechar(c = inputString[ISTRCHAR]))*/

				token[++tokenLength] = '\0';

				if (c == '.')
				{
					ISTRCHAR++;
					yyerror("ERROR:  do not use . in variable names");
					return (ERROR);
				}
				
				if (tokenLength > NAMELENGTH)
				{
					/* variable name too long */
					putPieces("ERROR: variable name too long: ", token,
							  (char *) 0, (char *) 0);
					ISTRCHAR -= tokenLength - NAMELENGTH - 1;
					return (ERROR);
				} /*if (tokenLength > NAMELENGTH)*/

				/*
				  See if it is a syntax element; "befinqws" are possible 1st
				  characters; TEMPPREFIX starts temp name.  If it is
				  followed by ':' it will be treated as a keyword; if it
				  is preceded by '$' it will be treated as a structure
				  component.
				  */
				c1 = skipWhiteSpace(SKIPCOMMENTS);
				/*
				  c1 is first non-white character after syntax elements,
				  treating comments as white
				  */
				if (token[0] != TEMPPREFIX && c1 != ':' && LASTCH != '$' &&
					strchr("befinqws",(int) token[0]) &&
					(c = isSyntaxElement(token)) != 0)
				{/* check for key words;  */
					char      lparen = '(';

					switch (c)
					{
					  case ELSE:
						lparen = '{';
						/* fall through */
					  case ELSEIF:
						if (LASTCH != '}')
						{
							sprintf(OUTSTR,
									"ERROR: %s does not follow '}'", token);
							yyerror(OUTSTR);
							return (BADTOKEN);
						}
						/* fall through */
					  case IF:
						IFBRACKETLEV[IDEPTH] = BRACKETLEV;
						if (!findLParen(c, lparen))
						{
							c = ERROR;
						}
						return (c);
					
					  case WHILE:
						WHILESTARTS[WDEPTH] = ISTRCHAR+1;
						/* fall through*/
					  case FOR:
						WHILEBRACKETLEV[WDEPTH] = BRACKETLEV;
						if (LASTCH != ';' && LASTCH != '{' &&
							LASTCH != ENDOFL && LASTCH != '\0')
						{
							sprintf(OUTSTR,
									"ERROR: illegal use of %s", token);
							yyerror(OUTSTR);
							c = BADTOKEN;
						}
						else if (!findLParen(c, lparen))
						{
							c = ERROR;
						}
						return (c);
					
					  case BREAK:
					  case NEXTREP:
						Yylval = (Symbolhandle) 0;

						if (WDEPTH <= ThisWdepth)
						{
							sprintf(OUTSTR,
									"ERROR: %s without for or while", token);
							yyerror(OUTSTR);
							
							return (BREAK);
						}
						savedPlace = ISTRCHAR;
						if (isdigit(c1) || c1 == '-' || c1 == '+')
						{ /* break n */
							y = mystrtod((char *) inputString+ISTRCHAR,
										 (char **) &endptr);
							ISTRCHAR = endptr - inputString;
							c2 = skipWhiteSpace(SKIPCOMMENTS);

							/* 
							   981231 added '\n' to list and removed ' ' and '\t'
							   from it
							   */
							if (c2 != '\n' && c2 != ';' && c2 != '}')
							{
								ISTRCHAR++;
								c = BADTOKEN;
							}
							else
							{
								ISTRCHAR = endptr - inputString;
								if (y <= 0 || y != floor(y))
								{
									sprintf(OUTSTR,
											"ERROR: %s n, with n not integer > 0",
											token);
									yyerror(OUTSTR);
									y = -1;
								}
								else if ((int) y > WDEPTH - ThisWdepth)
								{
									sprintf(OUTSTR,
											"ERROR: %s n where n > loop depth",
											token);
									yyerror(OUTSTR);
									y = -1;
								}
							}
						} /*if (isdigit(c1) || c1 == '-' || c1 == '+')*/
						else
						{ /* break or next */
							if (c1 != ' ' && c1 != '\t' && !isNewline(c1) &&
								c1 != ';' && c1 != '}')
							{
								ISTRCHAR++;
								c = BADTOKEN;
							}
							else
							{
								y = 1.0;
							}
						}

						if (y > 0.0)
						{
							Yylval = RInstall(NUMSCRATCH, (long) 1);
							if (Yylval == (Symbolhandle) 0)
							{
								c = FATALERROR;
							}
							else
							{
								DATAVALUE(Yylval,0) = y;
							}
						} /*if (y > 0.0)*/
						return (c);

					  case BREAKALL:
						if (WDEPTH <= ThisWdepth)
						{
							yyerror("ERROR: breakall without for or while");
							c = ERROR;
						} /*if (WDEPTH <= 0)*/
						else
						{
							if ((c1 = inputString[ISTRCHAR]) != ';' &&
								c1 != '}')
							{
								ISTRCHAR++;
								c = BADTOKEN;
							}
							else
							{
								y = (double) (WDEPTH - ThisWdepth) ;
								Yylval = RInstall(NUMSCRATCH, (long) 1);
								if (Yylval == (Symbolhandle) 0)
								{
									c = FATALERROR;
								}
								else
								{
									DATAVALUE(Yylval,0) = y;
									c = BREAK;
								}
							}
						} /*if (WDEPTH <= 0){}else{}*/
						return (c);

					  case END:
						if (BRACKETLEV > 0)
						{
							if (INPUTLEVEL > 0 || MDEPTH > 0 ||
								MLEVEL > MAXMDEPTH + 1)
							{
								sprintf(OUTSTR,
										"ERROR: %s is illegal in a macro or evaluated string",
										token);
							}
							else
							{
								sprintf(OUTSTR,
										"ERROR: %s must not be enclosed in {...}",
										token);
							}
						
							yyerror(OUTSTR);
							c = ERROR;
						} /*if (BRACKETLEV > 0)*/
						else if (LASTCH != ';' && LASTCH != '\0')
						{
							sprintf(OUTSTR,
									"ERROR: %s is not at start of line and does not follow ';'", token);
							yyerror(OUTSTR);
							c = BADTOKEN;
						}
						else
						{
							/*
							  960726 allow quit(T), quit(F) as will as quit()
							  T (default) and F only relevant with scrollable window
							  If F, quit is unconditional with no opportunity to save windows;
							  otherwise, a dialog box queries the user about each window
							  before quitting
							  */
#ifdef SCROLLABLEWINDOW
							SaveOnQuit = 1;
#endif /*SCROLLABLEWINDOW*/

							if (c1 == '(')
							{
								ISTRCHAR++;
								c2 = skipWhiteSpace(SKIPCOMMENTS);
								if (c2 == 'T' || c2 == 'F')
								{
#ifdef SCROLLABLEWINDOW
									SaveOnQuit = (c2 == 'T');
#endif /*SCROLLABLEWINDOW*/
									ISTRCHAR++;
									c2 = skipWhiteSpace(SKIPCOMMENTS);
								} /*if (c2 == 'T' || c2 == 'F')*/
								ISTRCHAR++;
								if (c2 != ')')
								{
									sprintf(OUTSTR,
											"ERROR: must be %s(), %s(T) or %s(F)",
											token, token, token);
									yyerror(OUTSTR);
									c = ERROR;
								} /*if (c2 != ')')*/
							} /*if (c1 == '(')*/ 

							if (c != ERROR)
							{
								c2 = skipWhiteSpace(SKIPCOMMENTS);
								if (!isNewline(c2))
								{
									sprintf(OUTSTR,
											"ERROR: '%s' not last command in line",
											token);
									yyerror(OUTSTR);
									c = ERROR;
								} /*if (!isNewline(c2))*/
							} /*if (c != ERROR)*/						
						} /*if (BRACKETLEV > 0){}else{}else{}*/ 
						return (c);
					} /*switch (c)*/
				} /*if token represents a syntax element*/

				/* must be name or temporary name but not a syntax element */

				if (token[0] == TEMPPREFIX && !isnamestart(token[1]))
				{
					ISTRCHAR++;
					if (isnamechar(token[1]))
					{
						sprintf(OUTSTR, "ERROR: illegal temporary name");
					}
					else
					{
						sprintf(OUTSTR,
								"ERROR: '%c' not followed by alphabetic character",
								TEMPPREFIX);
					}
					yyerror(OUTSTR);
					return(ERROR);
				}/*if (token[0] == TEMPPREFIX && !isnamestart(token[1]))*/

				/*
				  If token is "NULL", create NULL symbol.  Otherwise try and find it.
				  If token is the name of an ordinary symbol, Lookup() returns that symbol.
				  If token is the name of a special symbol such as CLIPBOARD, Lookup()
				  sets its current value and returns the special symbol.
				  If token is not the name of a special or ordinary symbol, Lookup() returns
				  (Symbolhandle) 0.
				  It is a fatal error if token is the name of a special symbol and
				  Lookup() returns (Symbolhandle) 0.  This cannot happen with CLIPBOARD
				  or SELECTION (on wx_motif)
				  Special symbols may not be in the Symbol table.
				  */
				if (token[0] == NULLNAME[0] && strcmp(token, NULLNAME) == 0)
				{
					Yylval = Install(NULLSCRATCH, NULLSYM);
				}
				else
				{
					Yylval = Lookup(token);
				}
			
				if (Yylval == (Symbolhandle) 0)
				{
					if (Findspecial(token) != (Symbolhandle) 0 ||
						token[0] == NULLNAME[0] &&
						strcmp(token, NULLNAME) == 0)
					{
						return (FATALERROR);
					}
					Yylval = Install(token, UNDEF);
					if (Yylval == (Symbolhandle) 0)
					{
						return (FATALERROR);
					}
					setNDIMS(Yylval,1);
				} /*if (Yylval == (Symbolhandle) 0)*/

				/*
				  920623 Return VAR for all names except BLTINs or MACROs followed by '('
				  950803 Replaced switch by simpler if statement.  Unfortunately this
				  eliminates any check for an illegal Symbol type.  This should
				  never occur :-), but if it does it should be caught elsewhere.
          
				  */
				c = TYPE(Yylval);
				if ((c == BLTIN || c == MACRO || c == UNDEF) && findLParen(0, '('))
				{
					if (c != UNDEF)
					{
						c = (c == BLTIN) ? BLTINTOKEN : MACROTOKEN;
						if (c == MACROTOKEN)
						{				/* macro invocation */
							MACROSTART = nameStart;
							PARSEMACROARGS = MACROPUNCT;
						}
						else if (tokenLength == 5 &&
								 token[0] == 'b' && strcmp(token, "batch") == 0)
						{
							if (BATCHBLOCKCOUNT <= 0)
							{			/* not on r.h.s of binarary operator*/
								return (BATCH);
							} /*if (BATCHBLOCKCOUNT <= 0)*/
							yyerror("ERROR: illegal use of batch()");
							return (BADTOKEN);
						}
						return (c);
					} /*if (c != UNDEF)*/
					else
					{
						/* 981118 code to search files automatically for macros*/
						Symbolhandle symh = searchForMacro(token);
					
						if (!isNull(symh))
						{
							Removesymbol(Yylval);
							Yylval = symh;
							Cutsymbol(Yylval);
							setNAME(Yylval, token);
							Addsymbol(Yylval);
							MACROSTART = nameStart;
							PARSEMACROARGS = MACROPUNCT;
							return (MACROTOKEN);
						} /*if (!isNull(symh))*/
					} /*if (c != UNDEF){}else{}*/
				}
				return (VAR);	/* any legal type */
			} /*if (isnamestart(c) || c == TEMPPREFIX)*/

			/* if not alpha, ought to be number unless it is of form \.[a-zA-z] */
			if (c == '.' && !isdigit(c1 = inputString[ISTRCHAR]))
			{
				ISTRCHAR++;
				if (c1 == TEMPPREFIX || isnamestart(c1) || isnamechar(c1))
				{
					yyerror("ERROR:  do not use . in variable names");
					return (ERROR);
				}
				return (BADTOKEN);
			} /*if (c == '.' && !isdigit(c1 = inputString[ISTRCHAR]))*/
			ISTRCHAR--;
			return (scanNumber());
		} /*switch (c)*/
	} /*while(1)*/
	
  errorExit:
	Removesymbol(Yylval);
	Yylval = (Symbolhandle) 0;
	return (errorToken);
		
} /*yylex1()*/

/*
  Called after token matching BLTIN or MACRO name or after IF, ELSEIF, WHILE,
  and FOR.
  If next non-white character is '(' it resets ISTRCHAR & returns 1
  Otherwise, it returns 0 and does not reset ISTRCHAR
  If op != 0 it prints an error message if '(' is not found

  also called from Symbol.c and Ifsetup.c
*/

long findLParen(int op, char paren)
{
	long           savePlace = ISTRCHAR;
	unsigned int   c;
	WHERE("findLParen");

	/* skip white space including \-protected newlines */
	c = skipWhiteSpace(DONTSKIPCOMMENTS);
	if (c != paren)
	{
		if (op != 0)
		{
			ISTRCHAR++;
			sprintf(OUTSTR,
					"ERROR: need '%c' after '%s%s'", paren, opName(op),
					(paren == '{' && op != ELSE) ? "(...)" : "");
			yyerror(OUTSTR);
		}
		ISTRCHAR = savePlace;
		return (0);
	} /*if (c != paren)*/
	return (1);
} /*findLParen(*/

int yylex(void)
{				/* lexical analyzer */

	unsigned int    c = 0;
	int             inputlevel = INPUTLEVEL; /* value at entrance */
	WHERE("yylex");

	Yylval = (Symbolhandle) 0;

	if (ALTTOKENPTR)
	{
		/*
		  first see if we have stashed stuff away that should be returned
		  before we examine *INPUTSTRING again
		  */
		c = (unsigned int) *ALTTOKENPTR++;
		if (c != 0)
		{
			LASTCH  = c;
			if (c == ';')
			{ /*do nothing*/
				;
			}
			else if (c == '(')
			{
				PARENLEV++;
			}
			else if (c == ')')
			{
				PARENLEV--;
			}
			else if (c == '}')
			{
				BRACKETLEV--;
			}
			else if (c == '{')
			{
				BRACKETLEV++;
			}
		} /*if (c != 0)*/
		else
		{
			ALTTOKENPTR = (int *) 0;
		} /*if (c != 0){}else{}*/
	} /*if (ALTTOKENPTR)*/

	if (c == 0)
	{
		/* nothing was foundin ALTTOKENPTR*/
		if (PARSEMACROARGS != 0)
		{
			c = parseMacro();
			LASTCH  = '\0';
		}
		else if (YYLEXMESSAGE > 0)
		{
			c = processBreak();
			LASTCH  = '\0';
		}
		else if (-YYLEXMESSAGE > ' ')
		{
			/*
			  -YYLEXMESSAGE is token to return next
			  After while(){} and if(){}, -YYLEXMESSAGE should be ';'
			  After <<name>> -YYLEXMESSAGE should be appropriate token
			  */
			c = -YYLEXMESSAGE;
			YYLEXMESSAGE = 0;
			LASTCH  = c;
		}
		else
		{
			/* normal scan of input stream */
			c = yylex1();
			LASTCH = (c == SEMI1) ? ';' : c;
		}
	} /*if (c == 0)*/

	/* for debugging purposes */
	if (GUBED & 1)
	{
		char        *outstr = OUTSTR;
		
		sprintf(outstr,"yylex[%d]: token = %s", inputlevel, tokenName(c));
		
		outstr += strlen(outstr);
		if (c == VAR || c == BLTINTOKEN || c == MACROTOKEN || c == UNDEF ||
			c == ASSIGNED || c == NUMBER)
		{
			if (Yylval != (Symbolhandle) 0)
			{
				char    *name = NAME(Yylval);
				
				if (c == VAR)
				{
					sprintf(outstr, ", type = %s", typeName(TYPE(Yylval)));
					if (strcmp(name,LOGICSCRATCH) == 0)
					{
						outstr += strlen(outstr);
						sprintf(outstr, ", value = %s",
								(DATAVALUE(Yylval,0) != 0.0) ? "T" : "F");
					}
				}
				else if (c == NUMBER)
				{
					if (isMissing(DATAVALUE(Yylval,0)))
					{
						sprintf(outstr,", value = MISSING");
					}
					else
					{
						sprintf(outstr,
								", value = %-12.5g",DATAVALUE(Yylval,0));
					}
				}
				if (!isscratch(NAME(Yylval)))
				{
					outstr += strlen(outstr);
					sprintf(outstr,", name = '%s'", name);
				}
			} /*if (Yylval != (Symbolhandle) 0)*/
		}
		else if (c == BREAK)
		{
			if (Yylval != (Symbolhandle) 0)
			{
				sprintf(outstr," %g", DATAVALUE(Yylval,0));
			}
			else
			{
				sprintf(outstr," ERROR");
			}
		}
		putOUTSTR();
	} /*if (GUBED & 1)*/

#ifdef YYLEXDB

	printf("token = %s\n",tokenName(c));

#endif /*YYLEXDB*/
	if (c == NUMBER)
	{
		c = VAR;
	}
	else if (c == FATALERROR)
	{
		FatalError = 1;
	}
	
	return ((c) ? c : ENDOFL);

} /*yylex()*/

