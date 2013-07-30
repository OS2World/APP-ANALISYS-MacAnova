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
#pragma segment Batch
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
#include <ctype.h>

#ifdef MACINTOSH
#include "macIface.h"
#endif

#ifdef WXWIN
#include "wx/wxIface.h"
#endif  /* WXWIN */


#define MAXBATCHERRORS 1  /* maximum number of errors allowed on batch */

/*
  This once was a routine to insert batch commands into the input stream of
  the parser.  Now it  switchs the input stream to an alternative file.

   960502 Changed fopen() to fmyopen(); other cosmetic changes
   970409 Initialize LASTINPUTWASCR[BDEPTH] for correct handling
          of various formats of text files
   971222 It is no longer legal to call batch() in a macro or evaluation
          string or to have anything executable follow it.  This seems
          more sensible than issuing a warning and ignoring things.
   980329 Added prompt:"prompt" which implies echo:T
   990226 replaced putOUTSTR() by putErrorOUTSTR()
*/

Symbolhandle    batch(Symbolhandle list)
{
	char           *fname, *keyword;
	unsigned char  *inputString;
	long            i, c;
	long            nargs = NARGS(list);
	long            echo; /* default value for 'echo' */
	Symbolhandle    arg1, arg2;
	Symbolhandle    symhPrompt = (Symbolhandle) 0;
	char           *usage = "usage is 'batch(fileName [,echo:F])', where fileName is CHARACTER";
	WHERE("batch");

	OUTSTR[0] = '\0';
	echo = (BDEPTH > 0) ? BATCHECHO[BDEPTH] : DEFAULTBATCHECHO;
	BDEPTH++;
	if(nargs > 2)
	{
		badNargs(FUNCNAME,-2);
		goto errorExit;
	}
	
	arg1 = COMPVALUE(list,0);

	if (!argOK(arg1, 0, (nargs > 1) ? 1 : 0))
	{
		goto errorExit;
	}
	
	if (!isCharOrString(arg1))
	{
		char       errormsg[50];
		
		sprintf(errormsg, "argument 1 (file name) to %s()", FUNCNAME);
		notCharOrString(errormsg);
		goto errorExit;
	}

	if(nargs == 2)
	{
		arg2 = COMPVALUE(list,1);
		if(!argOK(arg2, 0, 2))
		{
			goto errorExit;
		}
		keyword = isKeyword(arg2);
		if (!keyword || strcmp(keyword, "echo") == 0)
		{
			if (!isTorF(arg2))
			{
				notTorF((keyword) ? keyword : "argument 2");
				goto errorExit;
			}
			echo = (DATAVALUE(arg2,0) != 0.0) ? 1 : 0;
		}
		else if (strcmp(keyword, "prompt") == 0)
		{
			symhPrompt = arg2;
			if (!isCharOrString(symhPrompt))
			{
				notCharOrString(keyword);
				goto errorExit;
			}
			if (strlen(STRINGPTR(symhPrompt)) >= MAXPROMPT)
			{
				sprintf(OUTSTR,
						"ERROR: value of %s longer than %d characters",
						keyword, MAXPROMPT-1);
				goto errorExit;
			}
		}
		else
		{
			badKeyword(FUNCNAME, keyword);
			goto errorExit;
		}
	} /*if(nargs == 2)*/

	if(BDEPTH == MAXBDEPTH)
	{
		sprintf(OUTSTR, "ERROR: batch files nested too deep");
		goto errorExit;
	}
	
	if (WDEPTH > 0)
	{
		sprintf(OUTSTR,
				"ERROR: %s() cannot be used in a 'while' or 'for' loop",
				FUNCNAME);
		goto errorExit;
	} /*if (WDEPTH > 0)*/

	if (MDEPTH > 0 || MLEVEL > MAXMDEPTH+1 || INPUTLEVEL > 0)
	{
#if (1)
		sprintf(OUTSTR,
				"ERROR: %s() cannot be used in a macro or string to be evaluated",
				FUNCNAME);
		goto errorExit;
#else /*1*/
		sprintf(OUTSTR,
		
				"WARNING: all current macros and evaluation strings terminated by %s()",
				FUNCNAME);
		putErrorOUTSTR();
		while (INPUTLEVEL > 0)
		{
			popInputlevel();
		}
		MLEVEL = MAXMDEPTH + 1;
		MDEPTH = 0;
#endif /*1*/
	} /*if (MDEPTH > 0 || MLEVEL > MAXMDEPTH || INPUTLEVEL > 0)*/

	fname = STRINGPTR(arg1);

#ifdef HASFINDFILE
#ifdef MACINTOSH
	fname = macFindFile(fname, "\pSpecify the batchfile", (STR255) 0 , READIT,
						0, (OSType *) 0, &BatchVolume);
#endif /*MACINTOSH*/
#ifdef WXWIN
	fname = wxFindFile(fname, "Specify the batchfile", (char *) 0);
#endif /*WXWIN*/
	if(fname == (char *) 0)
	{ /* cancelled */
		sprintf(OUTSTR, "WARNING: %s() request cancelled", FUNCNAME);
		goto errorExit;
	}
#endif /*HASFINDFILE*/

	fname = expandFilename(fname);
	/*
	   At this point fname is a pointer to a static buffer, not a
	   dereferenced handle
	*/
	if (fname == (char *) 0 || !isfilename(fname))
	{
		goto errorExit;
	}

	for (i = 1;i < BDEPTH;i++)
	{
		if(strcmp(*INPUTFILENAMES[i], fname) == 0)
		{
			sprintf(OUTSTR,"ERROR: recursive use of batch()");
			goto errorExit;
		}
	} /*for (i = 1;i < BDEPTH;i++)*/


	if(INPUTSTRING != (unsigned char **) 0)
	{
		inputString = *INPUTSTRING;
		i = ISTRCHAR;
		c = inputString[i++];
		while(c != '\0')
		{ /* discard remainder of line */
			if (c != '}' && c != ')' && c != ';' && !isspace(c))
			{
				if(c != '#')
#if (1)
				{
					sprintf(OUTSTR,
							"ERROR: %s() not last executable expression in line or compound command",
							FUNCNAME);
					goto errorExit;
				}
#else /*1*/
				{
					putOutErrorMsg("WARNING: remainder of input line after batch() discarded");
				}
#endif /*1*/
				break;
			}
			c = inputString[i++];
		} /*while(c != '\0')*/
		mydisphandle ((char **) INPUTSTRING);
		INPUTSTRING = ThisInputstring = (unsigned char **) 0;
	} /*if(INPUTSTRING != (unsigned char **) 0)*/

	mydisphandle(INPUTFILENAMES[BDEPTH]);
	if ((INPUTFILENAMES[BDEPTH] = mygethandle(strlen(fname)+1)) == (char **) 0)
	{
		goto errorExit;
	}
	strcpy (*INPUTFILENAMES[BDEPTH], fname);

	INPUTFILE[BDEPTH] = fmyopen(fname, TEXTREADMODE);
	LASTINPUTWASCR[BDEPTH] = 0;
	
	if (INPUTFILE[BDEPTH] == (FILE *) 0)
	{
		mydisphandle(INPUTFILENAMES[BDEPTH]);
		INPUTFILENAMES[BDEPTH] = (char **) 0;
		sprintf(OUTSTR,"ERROR: cannot open file %s for %s input",
				fname, FUNCNAME);
		goto errorExit;
	}

	if (symhPrompt != (Symbolhandle) 0)
	{
		strcpy(DEFAULTPROMPT, STRINGPTR(symhPrompt));
		echo = 1;
	}
	else
	{
		strcpy(DEFAULTPROMPT, NullString);
	}
	strcpy(PROMPT, DEFAULTPROMPT);
	BATCHECHO[BDEPTH] = echo;
	
	MAXERRORS1 = (MAXERRORS1 > 0) ? MAXERRORS1 : MAXBATCHERRORS ;
	
	return (NULLSYMBOL);

  errorExit:
	BDEPTH--;
	putErrorOUTSTR();
	return (0);
	
} /*batch()*/
