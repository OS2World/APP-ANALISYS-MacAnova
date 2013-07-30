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
#pragma segment Lang
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

#ifndef Newline
#ifdef MACINTOSH
#define Newline 13
#else /*MACINTOSH*/
#define Newline 10
#endif /*MACINTOSH*/
#endif /*Newline*/

/*
  Modified by kb to add expanded macro to INPUTSTRING instead of starting
  fresh.  This was necessitated by the introduction of while loops for
  which it is normally necessary to return to the start of a loop.  A while
  loop containing a macro would crash.

  '$1', '$2', ... are replaced by the text of argument 1, argument 2, ...
  '$0'  is equivalent to '$1,$2,...,$nargs'
  '$N'  is replaced by the number of arguments to the macro
  '$A'  is equivalent to 'vector("$1","$2",...)'
  '$K'  is like '$0' except only keyword arguments are included
  '$k'  is like '$N' except only keyword arguments are counted
  '$V'  is like '$0' except only non-keyword arguments are included
  '$v'  is like '$N' except only non-keyword arguments are counted
  '$S'  is replaced by the name of the macro
  '$$'  is replaced by the current value of static variable level which is
		incremented mod 100
  A '$' preceded by '\' is not interpreted specially
  All these constructs except $A are recognized insided quotes as well as
  outside.

  Macrosetup returns 0 on error, 1 on normal return

  950807  Fixed bug so that an escaped quote would not toggle inquotes
  950927  Finished (:-)) fixing previous bug, taking into account the
  possibility that an escaped quote might occur outside a quote.

  970420 Replaced a strncpy() which was sometimes called with 0 third argument
  with an explicit loop because it might be causing problem with macros
  on CGI version

  970606 Renamed previous Macroset() to expandit() with added arguments
  nameForS, head, tail and expand.  New Macroset() calles expandit with
  arguments NAME(macrosymh), appropriate head and tail and with expand == 1.
  In preparation for some sort of expand() function, used like a macro
  call but returning the text of the expanded macro.

  970611 Removed incrementing of MDEPTH; now done in yyparse()
  970612 Moved definition of MACROSTART to globkb.h
  970617 Added check for balanced '(', '{' and '"' to expandit()
  971120 Changed expansion of $A from cat("$1",...) to vector("$1",...)
  990212 Changed putOUTSTR() to putErrorOUTSTR()
*/

/* size of increment for increasing length of INPUTSTRING */
#define CHUNK 64

/*
	Following added 970615, replacing definition of MACROHEADSTART as "{#)#"
	INLINEMACROHEAD and OUTLINEMACROHEAD referenced as externs in
	yylex.c

	970618 added EVALUATIONHEAD; changed MACROHEADLENGTH to HEADLENGTH 
	971104 added check for the value of myhandlelength()
	971126 Rename doExpand() expandit() to avoid conflict with a constant
*/

#define HEADLENGTH 4

char    INLINEMACROHEAD[HEADLENGTH + 1] =
{
	'{', '#', ')',  '#', '\0'
};

char    OUTLINEMACROHEAD[HEADLENGTH + 1] =
{
	'{', '#', ']',  '#', '\0'
};

char    EVALUATIONHEAD[HEADLENGTH + 1] =
{
	'{', '#', '}',  '#', '\0'
};

/* maximum no. characters inserted after text, including '\0' ("}" or "\n}") */
#define TAIL   3

#define SPACEUSED(I) (SpaceUsed += (I), SpaceLeft -= (I))

#define InputString    (*INPUTSTRING)

#define Trash          NAMEFORTRASH
static Symbolhandle    Trash;

#define TempStringH    STRING(Trash)
static unsigned char  *TempString;
static long            SpaceUsed, SpaceLeft;
static long            CurrentLength;

enum expandOpCodes
{
	inlineMacro = 0,
	outoflineMacro,
	expandedString
};

static int moreSpace(long needed)
{
	if (needed > 0)
	{
		if(CurrentLength < SpaceUsed)
		{ /* should never happen */
			fprintf(stderr,"PANIC: CurrentLength = %ld, SpaceUsed = %ld\n",
					CurrentLength, SpaceUsed);
		}
		TMPHANDLE = (CurrentLength == 0) ?
			mygethandle(needed) : mygrowhandle(TempStringH, CurrentLength+needed);
		setSTRING(Trash, TMPHANDLE);

		if (TMPHANDLE == (char **) 0)
		{
			return (0);
		}
	} /*if (needed > 0)*/

	CurrentLength += needed;
	SpaceLeft = CurrentLength - SpaceUsed;
	TempString = (unsigned char *) *TempStringH;
	
	return (1);
} /*moreSpace()*/

static long countEscapes(register char *s)
{
	register long   nq = 0;
	register char   c;

	while((c = *s++) != '\0')
	{
		if(c == '"' || c == '\\')
		{
			nq++;
		}
	}
	return (nq);
} /*countEscapes()*/

/*
   Copy string s2 to s1, inserting '\' before every '"' and '\'
   Returns the number of characters added to s1
*/

static long strEscapeCpy(register unsigned char *s1, char *s2)
{
	register unsigned char     c;
	unsigned char             *start = s1;

	while((c = *s2++) != '\0')
	{
		if(c == '"' || c == '\\')
		{
			*s1++ = '\\';
		}
		*s1++ = c;
	}
	*s1 = '\0';
	return (s1 - start);
} /*strEscapeCpy()*/

/* check to see whether argument is a keyword phrase */
static long iskeyarg(char *s)
{
	char     c;

	if (!istempname(s) && !iskeystart(*s))
	{
		return (0);
	} /*if (!istempname(s) && !iskeystart(*s))*/

	while((c = *(++s), isnamechar(c) || isspace(c)))
	{
		;
	} /*while((c = *(++s), isnamechar(c)))*/
	return (c == ':');
} /*iskeyarg()*/

static long expandit(Symbolhandle macrosymh, Symbolhandle list,
						  char *nameForS, char * head, char * tail, int op)
{
	/* routine to insert macro code into the input stream of the parser */

	char            c, lastc, nextc = '\0', *name;
	char            level[2];
	unsigned char **macroText;
	char           *argList;
	long            i, j, argno, macstrchar;
	long            needed, length;
	long            argListSize = 0, varListSize = 0, keyListSize = 0;
	long            inquotes, iskey, copyit, escaped = 0;
	long            nargs = NARGS(list), nKey = 0, nVar = 0, ncopied;
	int             nullForMissing;
	int             outofline = (op == outoflineMacro || op == expandedString);
	int             isMacro = (op == outoflineMacro || op == inlineMacro);
	int             mlevel = (op == inlineMacro) ? MLEVEL : MDEPTH;
	Symbolhandle    symh;
	WHERE("expandit");

	OUTSTR[0] = '\0';
	level[0] = '\0';
	if(nargs == 1 && COMPVALUE(list,0) == (Symbolhandle) 0)
	{
		nargs = 0;
	}

	macroText = (unsigned char **) STRING(macrosymh);

	/* this should be a valid handle because macro() should enforce it */

	needed = strlen(head) + 1; /*includes trailing '\0'*/

	/* MACROSTART set by yylex() to mark the insertion point of the macro */
	if (!outofline)
	{
		needed += MACROSTART;
	}

	needed += strlen((char *) *macroText) + strlen(tail);

	/* compute best guess at space needed */
	for (i = 0;i<nargs;i++)
	{
		/* Note all non-null arguments should be guaranteed to have type CHAR*/
		if((symh = COMPVALUE(list,i)) != (Symbolhandle) 0)
		{
			/* assume each arg used at least once */
			argList = STRINGPTR(symh);
			length = strlen(argList);
			needed += length; /* assumes each argument used at least once */
			argListSize += length + 1;
			iskey = iskeyarg(argList);

			if(iskey)
			{
				keyListSize += length+1;
				nKey++;
			}
			else
			{
				varListSize += length+1;
				nVar++;
			}
		} /*if((symh = COMPVALUE(list,i)) != (Symbolhandle) 0)*/
		else
		{
			nVar++;
			varListSize++;
		}
	} /*for (i = 0;i<nargs;i++)*/

	SpaceUsed = 0;
	if (outofline && NextInputstring != (unsigned char **) 0)
	{
		/* re-use next inputstring on the stack if it's big enough */
		CurrentLength = myhandlelength((char **) NextInputstring);
		if (CurrentLength < 0)
		{
			return (0);
		}
		
		if (needed > CurrentLength)
		{
			mydisphandle((char **) NextInputstring);
			CurrentLength = 0;
		}
		else
		{
			(*NextInputstring)[0] = '\0';
			/* TempStringH = (char **) NextInputstring; */
			setSTRING(Trash, (char **) NextInputstring);
			needed = 0;
		}
		NextInputstring = (unsigned char **) 0;
	}
	else
	{
		CurrentLength = 0;
	}

	if(!moreSpace(needed))
	{
		goto outOfMemory;
	}

	if (!outofline)
	{
		/* begin by duplicating INPUTSTRING up to start of macro */
		for (i = 0; i < MACROSTART; i++)
		{
			TempString[i] = InputString[i];
		}
		SPACEUSED(MACROSTART);
	} /*if (!outofline)*/

/*
   The macro text is wrapped with a leading '{#)macroNname\n' (in head)
   and a trailing '\n}'
*/
	strcpy((char *) TempString + SpaceUsed, head);
	SPACEUSED(strlen(head));

	/*
	  Now copy macroText into TempString with substitutions.

	  TempString should have space for the complete text of the macro plus
	  one copy of each argument.  Thus we only need to check for space needed
	  when expanding something.

	  Spaces and tabs outside of quotes are not copied, nor are trailing
	  comments starting with '#'
	*/

	macstrchar = 0;
	inquotes = 0;
	lastc = '\0';

	while ((c = (*macroText)[macstrchar++]))
	{
		needed = 1;
		if(!inquotes)
		{
			if (c == '#')
			{					/* don't copy comment */
				while ((c = (*macroText)[macstrchar++]) && c != Newline)
				{/* skip to end of line or end of string */
					;
				}

				if(c == '\0')
				{
					break;
				}
			}
			else if ((c == ' ' || c == '\t') &&
					 (lastc == ' ' || lastc == '\t'))
			{/* don't copy extra white space outside of quotes */
				continue;
			}
		} /*if(!inquotes)*/

		nextc = (*macroText)[macstrchar];
		if (SpaceLeft < needed + TAIL && !moreSpace(CHUNK))
		{
			goto outOfMemory;
		}
		/*
		  At this point, lastc is last character copied or '\0';
		  c is current character, not yet copied;
		  and nextc is the next character to be examined
		*/

		switch (c)
		{
		  case '$':
			/* possible start of item to expand */
			escaped = 0;
			if(lastc == '\\')
			{ /* treat '\$' as non-expanding */
				TempString[SpaceUsed-1] = c;
			}
			else if (isdigit(nextc))
			{/* insert argument into (*INPUTSTRING) */
				argno = 0;
				while ((c = (*macroText)[macstrchar], isdigit(c)))
				{
					argno = 10*argno + (c - '0');
					macstrchar++;
				}
				nullForMissing = argno > 0 && nextc == '0';
				/* insert argument, if any, or entire list for '$0' */
				if (nargs < argno ||
					(symh = COMPVALUE(list, argno - 1)) == (Symbolhandle) 0)
				{ /* argument is missing */
					if(!inquotes)
					{ /* message will be printed if argument missing */
						if (!nullForMissing)
						{
							sprintf(OUTSTR,
									"error(\"ERROR: Argument %ld to macro %s missing\");",
									argno,nameForS);
						}
						else
						{
							sprintf(OUTSTR,"NULL");
						}
					}
					else
					{ /* no expansion if missing argument is in quotes */
						*OUTSTR = '\0';
					}
					name = OUTSTR;
				}
				else if(argno > 0)
				{
					name = STRINGPTR(symh);
				}

				needed = (argno > 0) ? strlen(name) : argListSize;
				if(inquotes)
				{ /* needed to count the number of '"' && '\' */
					if(argno > 0)
					{
						needed += countEscapes(name);
					} /*if(argno > 0)*/
					else
					{
						for (j=0;j<nargs;j++)
						{
							symh = COMPVALUE(list,j);
							if(symh != (Symbolhandle) 0)
							{							
								needed += countEscapes(STRINGPTR(symh));
							}
						}
					} /*if(argno > 0){}else{}*/
				} /*if(inquotes)*/

				if (SpaceLeft < needed + TAIL && !moreSpace(needed + CHUNK))
				{
					goto outOfMemory;
				}
				if(argno > 0)
				{ /* copy argument */
					if(!inquotes)
					{
						strcpy((char *) TempString + SpaceUsed, name);
						length = strlen(name);
					}
					else
					{
						length = strEscapeCpy(TempString + SpaceUsed,name);
					}
					SPACEUSED(length);
				} /*if(argno > 0)*/
				else
				{/* copy entire argument list, separated by commas */
					for (j=0;j<nargs;j++)
					{
						symh = COMPVALUE(list,j);
						if(symh != (Symbolhandle) 0)
						{
							argList = STRINGPTR(symh);
							if(!inquotes)
							{
								strcpy((char *) TempString + SpaceUsed,
									   argList);
								length = strlen(argList);
							}
							else
							{
								length = strEscapeCpy(TempString + SpaceUsed,
												   argList);
							}
							SPACEUSED(length);
						}
						if(j < nargs-1)
						{
							TempString[SpaceUsed] = ',';
							SPACEUSED(1);
						}
					} /*for (j=0;j<nargs;j++)*/
				} /*if(argno > 0){}else{}*/ 
				c = '\0';
			} /* else if (isdigit(nextc))*/
			else if(!isnamechar(lastc) &&
					!isnamechar((*macroText)[macstrchar+1]) &&
					(strchr("KkNSVv",nextc) ||
					 !inquotes && nextc == 'A'))
			{ /* Process $N, $A, $K, $k, $V, $v, $S */
				macstrchar++;
				switch (nextc)
				{
				  case 'N':
				  case 'k':
				  case 'v':
					 /* replace '$N' by nargs, '$k' by nKey, '$v' by nVar */
					sprintf(OUTSTR,"%ld",
							(nextc == 'N') ? nargs :
								((nextc == 'k') ? nKey :nVar));
					needed = strlen(OUTSTR);
					if(SpaceLeft < needed + TAIL && !moreSpace(needed+CHUNK))
					{
						goto outOfMemory;
					}
					strcpy((char *) TempString+SpaceUsed,OUTSTR);
					SPACEUSED(needed);
					break;

				  case 'S':
					/* replace $S by nameForS */
					needed = strlen(nameForS);
					if(SpaceLeft < needed + TAIL && !moreSpace(needed+CHUNK))
					{
						goto outOfMemory;
					}
					strcpy((char *) TempString+SpaceUsed,nameForS);
					SPACEUSED(needed);
					break;

				  case 'A':
					/*
					  replace '$A' by vector("arg1","arg2",...,"argn")
					*/
					needed = argListSize + 2*nargs + 5 + 2*(nargs == 0);
					for (j=0;j<nargs;j++)
					{
						symh = COMPVALUE(list,j);
						if(symh != (Symbolhandle) 0)
						{							
							needed += countEscapes(STRINGPTR(symh));
						}
					} /*for (j=0;j<nargs;j++)*/

					if(SpaceLeft < needed + TAIL && !moreSpace(needed+CHUNK))
					{
						goto outOfMemory;
					}
					strcpy((char *) TempString+SpaceUsed,"vector(\"");
					SPACEUSED(8);
					for (j=0;j<nargs;j++)
					{
						symh = COMPVALUE(list,j);
						if(symh != (Symbolhandle) 0)
						{							
							length = strEscapeCpy(TempString + SpaceUsed,
											   STRINGPTR(symh));
							SPACEUSED(length);
						}
						if(j < nargs-1)
						{
							strcpy((char *) TempString + SpaceUsed,"\",\"");
							SPACEUSED(3);
						}
					} /*for (j=0;j<nargs;j++)*/ /* for j */
					strcpy((char *) TempString + SpaceUsed,"\")");
					SPACEUSED(2);
					break;

				  case 'K':
				  case 'V':
					needed = (nextc == 'K') ? keyListSize : varListSize;
					if(inquotes)
					{
						for (j=0;j < nargs;j++)
						{
							symh = COMPVALUE(list,j);
							if(symh != (Symbolhandle) 0)
							{							
								argList = STRINGPTR(symh);
								iskey = iskeyarg(argList);
								if(nextc == 'K' && iskey ||
								   nextc == 'V' && !iskey)
								{
									needed += countEscapes(STRINGPTR(symh));
								}
							} /*if(symh != (Symbolhandle) 0)*/
						} /*for (j=0;j<nargs;j++)*/
					} /*if(inquotes)*/					

					if (SpaceLeft < needed + TAIL &&
						!moreSpace(needed + CHUNK))
					{
						goto outOfMemory;
					}

					copyit = 0;
					ncopied = 0;
					for (j = 0;j < nargs;j++)
					{
						symh = COMPVALUE(list,j);
						if(symh != (Symbolhandle) 0)
						{
							argList = STRINGPTR(symh);
							iskey = iskeyarg(argList);
							copyit = (nextc == 'K' && iskey) ||
								(nextc == 'V' && !iskey);
							if(copyit)
							{
								if(inquotes)
								{
									length = strEscapeCpy(TempString+SpaceUsed,
														  argList);
								}
								else
								{
									strcpy((char *) TempString + SpaceUsed,
										   argList);
									length = strlen(argList);
								}

								SPACEUSED(length);
								if(++ncopied < ((nextc == 'K') ? nKey : nVar))
								{
									TempString[SpaceUsed] = ',';
									SPACEUSED(1);
								}
							} /*if(copyit)*/							
						} /*if(symh != (Symbolhandle) 0)*/
						else if (nextc == 'V' && ++ncopied < nVar)
						{
							TempString[SpaceUsed] = ',';
							SPACEUSED(1);
						}
					} /*for (j=0;j<nargs;j++)*/
					break;

				} /*switch (nextc)*/
				c = '\0';
			}
			else if(nextc == '$' && !isdigit((*macroText)[macstrchar+1]))
			{ /* '$$' not followed by digit ('$$1' treated like '$arg1') */
				macstrchar++;
				if(level[0] == '\0')
				{/* increment value to be substituted just once per expansion*/
					level[0] = mlevel / 10 + '0';
					level[1] = mlevel % 10 + '0';
				}

				needed = 2;
				if(SpaceLeft < needed + TAIL && !moreSpace(needed+CHUNK))
				{
					goto outOfMemory;
				}
				TempString[SpaceUsed] = level[0];
				TempString[SpaceUsed+1] = level[1];
				SPACEUSED(2);
			}
			else
			{/* not a macro substitution */
				goto saveChar;
			}
			break;

		  case '\\':
			if (inquotes)
			{
				escaped = !escaped;
			}
			goto saveChar;

		  case '"':
			if (!inquotes)
			{
				inquotes = 1;
			}
			else if (!escaped)
			{
				inquotes = 0;
			}
			escaped = 0;

			goto saveChar;

		  default:				/* anything else */
			escaped = 0;

		  saveChar:
			TempString[SpaceUsed] = c;
			SPACEUSED(1);
		} /*switch (c)*/
		lastc = c;
		TempString[SpaceUsed] = '\0';
	}/* while ((c = (*macroText)[macstrchar++])) */

	/* text of macro has now been expanded and copied*/

	needed = strlen(tail) + 1;
	if (SpaceLeft < needed && !moreSpace(needed))
	{
		goto outOfMemory;
	}
	strcpy((char *) TempString + SpaceUsed, tail);
	SPACEUSED(needed - 1);

	TempString[SpaceUsed] = '\0';

	if (!outofline)
	{
		/* now copy the rest of the INPUTSTRING into the new input stream */
		length = strlen((char *) InputString+ISTRCHAR);
		needed = length + 1;
		if (SpaceLeft < needed && !moreSpace(needed-SpaceLeft))
		{
			goto outOfMemory;
		}

		if (length > 0)
		{
			strcpy((char *) TempString + SpaceUsed,
				   (char *) InputString + ISTRCHAR);
			SPACEUSED(length);
		} /*if (length > 0)*/	
	} /*if (!outofline)*/

	/* now close it off and replace INPUTSTRING */
	TempString[SpaceUsed] = '\0';

	*OUTSTR = '\0';

	return (1);

  outOfMemory:
	return (0);
} /*expandit()*/

/*
	NCLASS(macrosymh) < 0 <==> inline expansion (classic case)
	NCLASS(macrosymh) == 0 <==> out-of-line expansion
*/


long Macrosetup(Symbolhandle macrosymh, Symbolhandle list)
{
	char        macroName[NAMELENGTH + 1];
	char        macroHead[HEADLENGTH+NAMELENGTH+2];
	char        macroTail[TAIL+1];
	char        c;
	long        i, length;
	int         inLine = isInline(macrosymh);
	int         macro = (TYPE(macrosymh) == MACRO);
	int         op = (inLine) ? inlineMacro : outoflineMacro;
	WHERE("Macrosetup");

	Trash = Install(SCRATCH,CHAR);

	if (Trash == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	if (!checkBalance((unsigned char *) STRINGPTR(macrosymh), NAME(macrosymh)))
	{
		goto errorExit;
	}

	if (!inLine && INPUTLEVEL == MAXMDEPTH)
	{
		sprintf(OUTSTR,
				"ERROR: more than %ld nested macros or evaluated strings",
				(long) MAXMDEPTH + 1);
		putErrorOUTSTR();
		goto errorExit;
	}

	strcpy(macroName, NAME(macrosymh));
	strcpy(macroHead, (inLine) ? INLINEMACROHEAD : OUTLINEMACROHEAD);
	strcat(macroHead, macroName);
	strcat(macroHead, "\n");

	i = 0;
	length = strlen(STRINGPTR(macrosymh));
	if (length == 0 ||
		(c = STRINGVALUE(macrosymh, length-1) , c != '\n' && c != '\r'))
	{
		macroTail[i++] = '\n';
	}
	macroTail[i++] = '}';
	macroTail[i] = '\0';

	if (!expandit(macrosymh, list, macroName, macroHead, macroTail, op))
	{
		goto errorExit;
	}

	if (inLine)
	{
		mydisphandle((char **) ThisInputstring);
		INPUTSTRING = ThisInputstring = (unsigned char **) TempStringH;
		ISTRCHAR = ThisIstrchar = MACROSTART;
	}
	else if (!pushInputlevel(macroName, MACROSTART,
							 (unsigned char **) TempStringH))
	{
		goto errorExit;
	}

	setSTRING(Trash, (char **) 0);

	Removesymbol(Trash);

	Trash = (Symbolhandle) 0;
	return (1);

  errorExit:
	*OUTSTR = '\0';
	Removesymbol(Trash);
	Trash = (Symbolhandle) 0;
	return (0);
} /*Macrosetup()*/


extern Symbolhandle  PARSERVALUE; /* defined in mainpars.c */

Symbolhandle mvEval(char ** commandH)
{
	Symbolhandle     result = (Symbolhandle) 0;
	unsigned char   *inputstring;
	long             currentEvalDepth = EVALDEPTH;
	long             currentInputLevel = INPUTLEVEL;
	long             length;
	char            *command = *commandH;
	WHERE("mvEval");

	length = strlen(command);
	if (length > 0)
	{
		long             needed, handleLength;
		int              needNL;

		if (!checkBalance((unsigned char *) command, (char *) 0))
		{
			goto errorExit;
		}

		if (INPUTLEVEL == MAXMDEPTH)
		{
			sprintf(OUTSTR,
					"ERROR: more than %ld nested macros or evaluated strings",
					(long) MAXMDEPTH);
			putErrorOUTSTR();
			goto errorExit;
		}
		/*
		  allocate space for line plus EVALUATIONHEAD, '\n' and '}'
		*/
		
		needNL = (!isNewline(command[length-1])) ? 1 : 0;
		needed = HEADLENGTH + 1 + length + needNL + 2;

		handleLength = myhandlelength((char **) NextInputstring);
		if (handleLength == CORRUPTEDHANDLE)
		{
			goto errorExit;
		}
		
		if (handleLength >= needed)
		{
			TMPHANDLE = (char **) NextInputstring;
			NextInputstring = (unsigned char **) 0;
		}
		else
		{
			mydisphandle((char **) NextInputstring);
			NextInputstring = (unsigned char **) 0;
			TMPHANDLE = mygethandle(needed);
			if (TMPHANDLE == (char **) 0)
			{
				goto errorExit;
			}
		}
		(*TMPHANDLE)[0] = '\0';

		if (!pushInputlevel(EVALNAME, 0, (unsigned char **) TMPHANDLE))
		{
			goto errorExit;
		}
		
		inputstring = *INPUTSTRING;
		strcpy((char *) inputstring, EVALUATIONHEAD);
		inputstring += HEADLENGTH;
		*inputstring++ = '\n';
		strcpy((char *) inputstring, *commandH);
		inputstring += strlen((char *) inputstring);
		if (!isNewline(*(inputstring - 1)))
		{
			*inputstring++ = '\n';
		}
		*inputstring++ = '}';
		*inputstring = '\0';

		PARSERVALUE = NULLSYMBOL;
		BRACKETLEV = 0;
		PARENLEV = 0;
		EVALDEPTH++;
	
		if (yyparse() != 0 || PARSERVALUE == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		result = PARSERVALUE;
		/* parser should have run PARSERVALUE through parsercopy()*/
	}
	else
	{
		result = NULLSYMBOL;
	}
	PARSERVALUE = (Symbolhandle) 0;
	
	EVALDEPTH = currentEvalDepth;

	return (result);

  errorExit:
	EVALDEPTH = currentEvalDepth;
	while (INPUTLEVEL > currentInputLevel)
	{
		popInputlevel();
	}

	return ((Symbolhandle) 0);
} /*mvEval()*/

Symbolhandle evaluate(Symbolhandle list)
{
	long             nargs = NARGS(list);
	Symbolhandle     arg;
	Symbolhandle     result = (Symbolhandle) 0;
	WHERE("evaluate");

	if (nargs != 1 || COMPVALUE(list, 0) == (Symbolhandle) 0)
	{
		badNargs(FUNCNAME, 1);
		goto errorExit;
	}

	arg = COMPVALUE(list, 0);
	if (!isCharOrString(arg))
	{
		char     msg[50];

		sprintf(msg, "argument 1 to %s", FUNCNAME);
		notCharOrString(msg);
		goto errorExit;
	}

	if (strlen(STRINGPTR(arg)) == 0)
	{
		sprintf(OUTSTR,
				"WARNING: argument to %s() is null string (\"\")",
				FUNCNAME);
		putErrorOUTSTR();
		result = NULLSYMBOL;
	} /*if (strlen(STRINGPTR(arg)) == 0)*/
	
	result = mvEval(STRING(arg));

	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	
	return (result);

  errorExit:
	while(INPUTLEVEL > 0)
	{
		popInputlevel();
	}
	return ((Symbolhandle) 0);
} /*evaluate()*/
