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
#pragma segment Initialize
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
#include <stdlib.h>

/*
   Initialize pre-defined macros
   971108 added new versions of resid and yhat which label rows and columns
   971111 added new pre-defined macro regcoefs
   980102 Modified makecols to allow final argument nomissing:T
   980710 Replaced anytrue and alltrue with versions that use evaluate
   980727 Added hasnotes
   981120 Added nofileok:T to macroread calls in getmacros
   981121 Deleted all use of "ERROR: " in macro calls to error()
   990305 Text of macros included from "iniMacro.h"
*/

#ifdef MACINTOSH
#include "macIface.h"

#define MACROTYPE   'wstr'
#define ENDNAME     "_END_"
#define IDBASE      10000
#define NAMEPREFIX  "_MV_"
#endif /*MACINTOSH*/

#if 0
   In the following quoted comments are deliminated by /@..@/.

   This file must be in a particular format to allow creation of a MPW rez
   file using script iniMacro.awk .  If additional macros are added they
   should follow this format.

   The script copies the copyright information and exctracts the values of
   macros MACROTYPE, ENDNAME, IDBASE and NAMEPREFIX (see above) and then
   ignores everything up through the line '/@STARTMACROS@/'.  It ignores all
   other  preprocessor commands.

   The quoted name of each macro followed by a comma with no intervening
   whitespace must be the second field of any line line whose first field
   is '/@MNAME@/'.

   The quoted text of a macro must start with a line whose first field is
   '/@MTEXT@/'. All lines except the last should end in \

   The final line of a macro must terminate with '", /@MEND@/', where '"'
   is the terminating quote for the macro and there must be no intervening
   whitespace before the comma.  For a one line macro this would be on
   the same line with '/@MTEXT@/'

   The script skips any lines bracketed by lines whose first fields are
   '/@NOTFORMACSTART@/' and '/@NOTFORMACEND@/'.

   ('@' is used here instead of '#' because the MPW C preprocessor got
   confused otherwise.)

   Preprocessor lines are not copied

   Scanning terminates when the line containing '/@ENDMACROS@/' is found

   961104 Slightly modified many macros, replacing $1, $2, ... by $01, $02
   which will mean less memory needed for expansion and less scanning by
   yylex when expanded.

   980730 Changed type of component 'text' to char * from unsigned char *
#endif /*if 0*/

#ifndef MACINTOSH /* macros are loaded from resources on Mac */
static struct
{
	char           *name;
	char           *text;
}               macros[] =
{
/*
  990305 text of macros moved to new header file iniMacro.h
*/
#include "iniMacro.h"
};
#endif /*MACINTOSH*/

/*
	Install text in symbol table as MACRO name
	There are length characters in text which may not be null-terminated
*/
static long installMacro(char * text, char *name, int length)
{
	Symbolhandle       symh;
	
	symh = Install(name, MACRO);

	TMPHANDLE = mygethandle(length + 1);
	if(symh == (Symbolhandle) 0 || TMPHANDLE == (char **) 0)
	{
		goto errorExit;
	}
	setSTRING(symh,TMPHANDLE);
	strncpy(*TMPHANDLE, text, length);
	(*TMPHANDLE)[length] = '\0';

	setNDIMS(symh,1);
	setDIM(symh,1,1);
	
	return (1);

  errorExit:
	return (0);
} /*installMacro()*/

/*
	Initialize pre-defined macros
	On Macintosh they are loaded from resources of type 'wstr' with id's
	IDBASE+1, IDBASE+2, ...,  The resource name is the macro name, prefixed
	by NAMEPREFIX.  The last resource has name NAMEPREFIXENDNAME ("_MV__END_")
	and is not loaded.
	If resedit is used to edit macros or add macros, it would be best if no
	gaps were left in the id numbers.
*/

long iniMacros(void)
{
#ifndef MACINTOSH
	int        i;

	for (i = 0; macros[i].name; i++)
	{
		if (!installMacro(macros[i].text, macros[i].name,
						  strlen(macros[i].text)))
		{
			goto errorExit;
		}
	}
#else /*MACINTOSH*/
	Integer          id = IDBASE, id1;
	Integer          i, count = CountResources(MACROTYPE);
	char            *prefix = NAMEPREFIX;
	Integer          prefixLength = strlen(prefix);
	Str255           resName;
	char            *name = (char *) resName;
	ResType          type;
    unsigned char  **text;
	
	/*
	   examine all resources of the wanted type
	   If the name starts with prefix, install it as long as it is not
	   the name of the dummy last macro
	*/
	for(i=1; i<= count; i++)
	{
		text = (unsigned char **) GetIndResource(MACROTYPE, i);
		if (ResError() != noErr)
		{
			break;
		}
		GetResInfo((Handle) text, &id1, &type, resName);
		if (ResError() != noErr)
		{
			goto errorExit;
		}
		PtoCstr((unsigned char *) name);
		if (strncmp(prefix, name, prefixLength) == 0 &&
			strcmp(name + prefixLength, ENDNAME) != 0)
		{
			HLock((Handle) text);
		/* first two characters of *text give number of characters */
			if (!installMacro((char *) (*text) + 2, name + prefixLength,
							  (*text)[0] * 256 + (*text)[1]))
			{
				goto errorExit;
			}
			HUnlock((Handle) text);
		}
		ReleaseResource((Handle) text);
	} /*while(1)*/
#endif /*MACINTOSH*/
	return (1);

  errorExit:
	return (0);
} /*iniMacros()*/ 
