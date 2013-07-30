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
#pragma segment Macro
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

/*
   950803   removed all reference to very obsolete type MACRO1
   951004   Added keyword dollars:T
   970415   addDollarSigns() changed to editMacro(); characters not in
            {\n, \t, [ -~] changed to escaped octal when in quotes.
   970617   Added keyword phrase inline:T or F
   971104   Added check for value of myhandlelength()
   980317   Fixed minor bug; no message printed with extra non keyword args
   980727   Added keyword 'notes'
   980730   Added new argument to reuseArg() so that notes will not be kept.
   990212   Replaced putOUTSTR() by putErrorOUTSTR()
   990220   Changed name of editMacro() to preMacro() and made it
            accessible externally
*/

/*
  prepMacro(text,newText,addDollars) scans new text, keeping track of
  whether it is scanning within quotes.

   Inside of quotes, it looks for non-printing characters (DEL= ascii 127,
   and characters with ascii codes < 32 except for \t and \n)

   Outside of quotes, if addDollars != 0, it looks for temporary variable
   names with an eye to appending '$$'; non-printing characters are not
   considered special.

  If newText != (char *) 0, text is copied to newText, possible appending
  '$$' to  temporary variable names outside of quotes and replacing non-
  printing characters in quotes by their escaped octal representation (\nnn).

  If newText == (char *) 0, only the scanning is done and prepMacro() returns
  the number of characters needed for the edited macro.

  prepMacro() is called first with newText == (char *) 0 to determine the
  amount of spaced needed, and then with newText a char vector with the
  required length

  No escaping is recognized outside of quotes
*/

#define addChar(C) if (addit) newText[needed] = C; needed++

long prepMacro(char * text, char * newText, int addDollars)
{
	int               addit = newText != (char *) 0;
	int               escaped = 0, inquotes = 0;
	long              needed = 0;
	unsigned char     c;
	WHERE("prepMacro");
	
	while ((c = (unsigned char) *text++) != '\0')
	{
		if (!inquotes)
		{
			addChar(c);
			if (c == '"')
			{
				inquotes = 1;
			}
			else if (c == TEMPPREFIX && isnamestart(*text))
			{
				while((c = *text, isnamechar(c)))
				{ /* scan rest of temporary name */
					text++;
					addChar(c);
				}
				if (addDollars && (c != '$' || text[1] != '$'))
				{ /* name not followed by '$$' */
					addChar('$');
					addChar('$');
				}
			} /*if (c == '"'){}else if (c == TEMPPREFIX && isnamestart(*text)){}*/
		} /*if (!inquotes)*/
		else
		{
			if (c == '\\')
			{
				escaped = !escaped;
				addChar(c);
			}
			else if (c == '"' && !escaped)
			{
				inquotes = 0;
				addChar(c);
			}
			else
			{
				escaped = 0;
				/* note extended characters (codes >= 128) are not escaped*/
				if (c == '\n' || c == '\t' || (c >= 32 && c < 127) || c >= 128)
				{
					addChar(c);
				}
				else
				{
					needed += escapedOctal(c,
								(addit) ? (unsigned char *) newText + needed :
										   (unsigned char *) 0);
				}
			}
		}  /*if (!inquotes){}else{}*/
	} /*while ((c = *text++) != '\0')*/
	addChar('\0');
	return (needed);
} /*prepMacro()*/


Symbolhandle    macro(Symbolhandle list)
{

	/* make a string into a MACRO (choices for which) */

	Symbolhandle    symh, symhKey, result = (Symbolhandle) 0;
	Symbolhandle    symhNotes = (Symbolhandle) 0;
	long            nargs = NARGS(list);
	long            type;
	long            length, needed;
	long            margs = 3; /* maximum number of args */
	long            i;
	int             addDollar = 0;
	int             inLine = EXPANDINLINE;
	char           *keyword;
	WHERE("makeMacro");

	if (nargs > margs)
	{
		badNargs(FUNCNAME, -margs);
		goto errorExit;
	}
	symh = COMPVALUE(list,0);

	if (!argOK(symh, 0, (nargs > 1) ? 1 : 0))
	{
		goto errorExit;
	}

	type = TYPE(symh);
	if (type != MACRO && !isCharOrString(symh))
	{
		char      outstr[30];
		
		sprintf(outstr,"argument%s to %s()",
				(nargs == 1) ? "" : " 1", FUNCNAME);
		notCharOrString(outstr);
		goto errorExit;
	} /*if (type != MACRO && !isCharOrString(symh))*/

	for (i = 1; i < nargs; i++)
	{
		symhKey = COMPVALUE(list, i);
		if (!(keyword = isKeyword(symhKey)))
		{
			sprintf(OUTSTR,
					"ERROR: argument %ld to %s() not a keyword phrase",
					i+1, FUNCNAME);
			putErrorOUTSTR();
			goto errorExit;
		}
		
		if (strncmp(keyword, "dol", 3) == 0 || strcmp(keyword, "inline") == 0)
		{
			if (!isTorF(symhKey))
			{
				notTorF(keyword);
				goto errorExit;
			}
			if (keyword[0] == 'd')
			{
				addDollar = (DATAVALUE(symhKey, 0) != 0.0);
			}
			else
			{
				inLine = (DATAVALUE(symhKey, 0) != 0.0);
			}
		} /*if (strncmp(keyword, "dol", 3) == 0)*/
		else if (strncmp(keyword, "note", 4) == 0)
		{
			if (TYPE(symhKey) != CHAR || !isVector(symhKey))
			{
				sprintf(OUTSTR,
						"ERROR: value for '%s' on %s() not CHARACTER vector",
						keyword, FUNCNAME);
				goto errorExit;
			}
			symhNotes = symhKey;
		}
		else
		{
			badKeyword(FUNCNAME, keyword);
			goto errorExit;
		}			
	} /*for (i = 1; i < nargs; i++)*/
	
	length = myhandlelength(STRING(symh));
	if (length < 0)
	{
		goto errorExit;
	}
	
	needed = prepMacro(STRINGPTR(symh), (char *) 0, addDollar);

	/* Note: if needed == length, no editing is needed */
	if (isscratch(NAME(symh)) && needed == length)
	{ /* reuse space */
		result = reuseArg(list, 0, 0, 0);
	} /*if (isscratch(NAME(symh)) && needed == length)*/
	else
	{
		result = Install(SCRATCH,MACRO);

		if (result == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		TMPHANDLE = mygethandle(needed);
		setSTRING(result, TMPHANDLE);
		if (TMPHANDLE == (char **) 0)
		{
			goto errorExit;
		}
		if (needed == length)
		{
			strcpy(*TMPHANDLE, STRINGPTR(symh));
		}
		else
		{
			(void) prepMacro(STRINGPTR(symh), *TMPHANDLE, addDollar);
		}
	} /*if (isscratch(NAME(symh)) && needed == length){}else{}*/

	setNAME(result, SCRATCH);
	setTYPE(result, MACRO);
	setNDIMS(result,1);
	setDIM(result,1,1);
	setInline(result, inLine);
	if (symhNotes != (Symbolhandle) 0)
	{
		TMPHANDLE = STRING(symhKey);
		setSTRING(symhKey, (char **) 0);
		if (!setNotes(result, TMPHANDLE))
		{
			goto errorExit;
		}
	}
	return (result);

  errorExit:
	putErrorOUTSTR();
	Removesymbol(result);
	return (0);
} /*macro()*/
