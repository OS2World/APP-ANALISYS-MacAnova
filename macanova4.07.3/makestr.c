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
#pragma segment Makestr
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

/* 
  structure(arg1,...[,compnames:charvec,silent:T,label:labels])
  make a new structure from items in list
  components can be assigned names by keywords, e.g., a:cat(10,20)
  If an argument has a temporary name starting with TEMPPREFIX ('@'), it
  is stripped it off to make component name

  strconcat(arg1,...[,compnames:charvec,silent:T,label:labels]) is like
  makestr, except if argj is itself a structure, the top level components
  of argj are top level components of the result.  This is particularly
  useful for merging structures or for adding several components to a
  structure.

  After all the components supplied, both accept keyword arguments of
  the form compnames:CharVec1, labels:CharVec2, silent:T

  silent:T suppresses any WARNING messages

  compnames supplies names for the top level components.  The names are
  limited to 12 characters.

  labels can be used to supply component labels of arbitrary length.

  If CharVec1 is a CHARACTER scalar, say "var", it is taken as a root for
  contructing names "var1", "var2", ... .  Otherwise length(CharVec1) must
  match the number of components.

  If CharVec2 is NULL, the output will have no labels.

  If CharVec2 is a CHARACTER scalar, say "var", it is taken as a root for
  contructing names "var1", "var2", ... .  Otherwise length(CharVec2) must
  match the number of components.

  Any undefined arguments (including assigned to) or functions are turned
  into NULL components with the same name, with a warning message unless
  silent:T is an argument.

  970206 Added keyword silent, allowed builtin functions as arguments,
  preserved names of UNDEF and ASSIGNED arguments.

  970324 A defined argument that is NULL no longer generates a warning
  970627 Specific check for LONG arguments
  980730 added new argument to reuseArg() so that notes will be kept.
         added keyword 'notes'
  980807 added check to see if component names  contain white
         space or are too long
  990212 Changed putOUTSTR() to putErrorOUTSTR()
*/

#define trash NAMEFORTRASH
enum makestrScratch
{
	GLABELS = 0,
	NTRASH
};

static Symbolhandle setComp(Symbolhandle list, Symbolhandle result,
							long iarg, long icomp)
{
	Symbolhandle       arg = COMPVALUE(list, iarg);
	Symbolhandle       symh;
	WHERE("setComp");
	
	if (!isDefined(arg))
	{
		if (arg != (Symbolhandle) 0)
		{
			/* type UNDEF or ASSIGNED */
			if (TYPE(arg) != ASSIGNED)
			{
				symh = reuseArg(list, iarg, 1, 1);
				Cutsymbol(symh);
				COMPVALUE(result, icomp) = symh;
				setTYPE(symh, NULLSYM);
			} /*if (TYPE(arg) != ASSIGNED)*/
			else
			{
				symh = COMPVALUE(result, icomp) = Makesymbol(NULLSYM);
				if (symh == (Symbolhandle) 0)
				{
					goto errorExit;
				}
				setNAME(symh, NAME(arg));
			} /*if (TYPE(arg) != ASSIGNED){}else{}*/
		} /*if (arg != (Symbolhandle) 0)*/
		else
		{/* empty argument */
			symh = COMPVALUE(result, icomp) = Makesymbol(NULLSYM);
			if (symh == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			setNAME(symh, NULLNAME);
		}				
	} /*if (!isDefined(arg))*/
	else if (TYPE(arg) == BLTIN)
	{
		symh = COMPVALUE(result, icomp) = Makesymbol(NULLSYM);
		if (symh == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		setNAME(symh, NAME(arg));
	}
	else if(!isKeyword(arg) && !isscratch(NAME(arg)))
	{ /* make copy */
		symh = COMPVALUE(result, icomp) = Makesymbol(REAL);
		if(symh == (Symbolhandle) 0 || !Copy(arg,symh))
		{
			goto errorExit;
		}
	}
	else
	{ /* keyword or scratch: move symbol to result */
		symh = reuseArg(list, iarg, 1, 1);
		Cutsymbol(symh);
		COMPVALUE(result, icomp) = symh;
	}
	/* fall through*/

  errorExit:
	return (symh);

} /*setComp()*/


Symbolhandle    makestr(Symbolhandle list)
{
	Symbolhandle    result = (Symbolhandle) 0, arg, component, symh;
	Symbolhandle    symhKey, arg1 = COMPVALUE(list, 0);
	Symbolhandle    symhCompnames = (Symbolhandle) 0;
	Symbolhandle    symhLabels = (Symbolhandle) 0;
	Symbolhandle    symhNotes = (Symbolhandle) 0;
	long            strconcat = (strcmp(FUNCNAME,"strconcat") == 0);
	long            jcomp, iarg, icomp, nNames = 0, maxname, type;
	long            nargs = NARGS(list), ncomps = 0, ncomps1;
	long            dims[1];
	int             foundEmpty = 0, foundLong = 0, foundBltin = 0;
	int             silent = 0;
	int             noLabels = 0, noNotes = 0;
	int             useArg1Labels, useArg1Notes;
	char           *keyword, *nameptr, **labelsH;
	char            name[NAMELENGTH+1], namestart[NAMELENGTH];
	char           *plural = "s";
	Symbolhandle    trash = (Symbolhandle) 0;
	WHERE("makestr");
	
	*OUTSTR = '\0';
	
	/*
	  trim off but save compname:names, label:labels, silent:T
	  from list if they are there
	*/
	while (nargs > 1 && (keyword = isKeyword(COMPVALUE(list,nargs-1))))
	{
		symhKey = COMPVALUE(list,nargs-1);
		if (strncmp(keyword, "compname", 8) == 0)
		{
			char       *what = (char *) 0;

			symhCompnames = symhKey;
			if(TYPE(symhCompnames) != CHAR || !isVector(symhCompnames))
			{
				sprintf(OUTSTR,
						"ERROR: keyword %s for %s() must be a CHARACTER vector or scalar",
						keyword, FUNCNAME);
				goto errorExit;
			}
			nNames = symbolSize(symhCompnames);
			nameptr = STRINGPTR(symhCompnames);
			
			for (icomp = 0; icomp < nNames; icomp++)
			{
				char     *pc;
				
				for (pc = nameptr; *pc; pc++)
				{
					if (*pc == '$' || isspace(*pc))
					{
						break;
					}
				}

				if (*pc == '\0')
				{
					if (pc - nameptr > NAMELENGTH)
					{
						what = "is too long";
					}
					else if (pc == nameptr)
					{
						what = "is empty";
					}
				}
				else if (*pc == '$')
				{
					what = "contains '$'";
				}
				else
				{
					what = "contains space or bad character";
				}
		
				if (what)
				{
					break;
				}				
				nameptr = skipStrings(nameptr, 1);
			} /*(icomp = 0; icomp < nNames; icomp++)*/
			if (what)
			{
				sprintf(OUTSTR,
						"ERROR: A component name for %s() %s",
						FUNCNAME, what);
				goto errorExit;
			} /*if (i < nNames)*/
		}
		else if (strncmp(keyword, "label", 5) == 0)
		{
			symhLabels = symhKey;
			if (TYPE(symhLabels) == NULLSYM)
			{
				noLabels = 1;
				symhLabels = (Symbolhandle) 0;
			} /*if (TYPE(symhLabels) == NULLSYM)*/
		}
		else if (strncmp(keyword, "note", 4) == 0)
		{
			symhNotes = symhKey;
			if (TYPE(symhNotes) == NULLSYM)
			{
				noNotes = 1;
				symhNotes = (Symbolhandle) 0;
			} /*if (TYPE(symhNotes) == NULLSYM)*/
		}
		else if (strcmp(keyword, "silent") == 0)
		{
			if (!isTorF(symhKey))
			{
				notTorF(keyword);
				goto errorExit;
			}
			silent = (DATAVALUE(symhKey, 0) != 0);
		}
		else
		{
			break;
		}
		nargs--;
	} /*while (nargs > 1 && (keyword = isKeyword(COMPVALUE(list,nargs-1))))*/
	
	for (iarg = 0; iarg < nargs; iarg++)
	{
		arg = COMPVALUE(list, iarg);
		type = (arg != (Symbolhandle) 0) ? TYPE(arg) : 0;
		
		if (type == LONG)
		{
			sprintf(OUTSTR,
					"ERROR: argument %ld to %s() has illegal type LONG",
					iarg+1, FUNCNAME);
			goto errorExit;
		}
		ncomps++;
		
		if (type == BLTIN)
		{
			/* will be replaced by NULLSYM symbol of same name*/
			foundBltin++;
		}
		else if (!isDefined(arg))
		{
			/* 
			   Missing or undefined arguments in list will be replaced
			   by a NULLSYM symbol
			*/
			foundEmpty++;
		} /*else if (!isDefined(arg))*/
		else if(strconcat)
		{ /* count the number of output components for strconcat() */
			ncomps += ((isStruc(arg)) ? NCOMPS(arg) - 1: 0);
		}
	} /*for (iarg = 0; iarg < nargs; iarg++)*/

	if(nNames > 1 && nNames != ncomps)
	{
		sprintf(OUTSTR,
				"ERROR: incorrect number of component names for %s()",
				FUNCNAME);
		goto errorExit;
	} /*if(nNames > 1 && nNames != ncomps)*/
	
	if (!silent)
	{
		if(foundEmpty > 0)
		{
			sprintf(OUTSTR,
					"WARNING: undefined or empty argument%s to %s()",
					(foundEmpty > 1) ? plural : NullString, FUNCNAME);
			putErrorOUTSTR();
		} /*if(foundEmpty > 0)*/
		if (foundBltin)
		{
			sprintf(OUTSTR,
					"WARNING: function name%s used as argument%s to %s()",
					(foundBltin > 1) ? plural : NullString,
					(foundBltin > 1) ? plural : NullString, FUNCNAME);
			putErrorOUTSTR();
		} /*if (foundBltin)*/
	} /*if (!silent)*/

	if (strconcat && isStruc(arg) && nargs == 1)
	{
		useArg1Labels = symhLabels == (Symbolhandle) 0 && HASLABELS(arg1);
		useArg1Notes = !noNotes && symhNotes == (Symbolhandle) 0 &&
		  HASNOTES(arg1);
	}
	else
	{
		useArg1Labels = useArg1Notes = 0;
	}
	
	if (symhLabels != (Symbolhandle) 0)
	{
		unsigned long     labelError;
		
		dims[0] = ncomps;
		labelError = checkLabels(symhLabels, 1, dims);
		if (labelError & LABELSERROR || !silent)
		{
			badLabels(labelError);
			if (labelError & LABELSERROR)
			{
				goto errorExit;
			}
		} /*if (labelError & LABELSERROR || !silent)*/

		if (!(labelError & WRONGSIZELABELS))
		{
			if ((trash = GarbInstall(NTRASH)) == (Symbolhandle) 0)
			{
				goto errorExit;
			}

			if (TYPE(symhLabels) == STRUC)
			{
				symhLabels = COMPVALUE(symhLabels, 0);
			}

			if (isScalar(symhLabels))
			{
				long length = expandLabels(STRINGPTR(symhLabels),
										   ncomps, (char *) 0);
				if (!getScratch(labelsH, GLABELS, length, char))
				{
					goto errorExit;
				}
				expandLabels(STRINGPTR(symhLabels), ncomps, *labelsH);
			}
			else
			{
				labelsH = STRING(symhLabels);
				setSTRING(symhLabels, (char **) 0);
				toTrash(labelsH, GLABELS);
			}
		} /*if (!(labelError & WRONGSIZELABELS))*/
		else
		{
			symhLabels = (Symbolhandle) 0;
		}
	} /*if (symhLabels != (Symbolhandle) 0)*/
		
	result = StrucInstall(SCRATCH, ncomps);

	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	if(nNames == 1 && ncomps > 1)
	{
		strncpy(namestart, STRINGPTR(symhCompnames), NAMELENGTH);
		if(namestart[0] == '\0')
		{
			strcpy(namestart, "component");
		}
		maxname = NAMELENGTH - nDigits(ncomps);
		namestart[maxname] = '\0';
	}
	else
	{
		namestart[0] = '\0';
	}

	if (symhLabels != (Symbolhandle) 0)
	{
		unTrash(GLABELS);
		if (!setLabels(result, labelsH))
		{
			toTrash(labelsH, GLABELS);
			goto errorExit;
		}
	}
	else if (useArg1Labels)
	{
		/* Must be strconcat(str), str with no labels */
		transferLabels(arg1, result);
	}
	
	if (symhNotes != (Symbolhandle) 0 || useArg1Notes)
	{
		if (symhNotes != (Symbolhandle) 0)
		{
			TMPHANDLE = STRING(symhNotes);
			setSTRING(symhNotes, (char **) 0);
		}
		else
		{
			TMPHANDLE = myduphandle(NOTESHANDLE(arg1));
		}
		if (!setNotes(result, TMPHANDLE))
		{
			mydisphandle(TMPHANDLE);
			goto errorExit;
		}
	} /*if (symhNotes != (Symbolhandle) 0 || useArg1Notes)*/

	if (!strconcat)
	{ /* structure() */
		for (iarg = 0;iarg < nargs;iarg++)
		{
			symh = setComp(list, result, iarg, iarg);
			if (symh == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			setCompName(symh, NAME(symh));
		} /*for (iarg = 0;iarg < nargs;iarg++)*/
	} /* if(!strconcat) */
	else
	{ /* strconcat() */
		ncomps = 0;
		for (iarg = 0;iarg < nargs;iarg++)
		{
			arg = COMPVALUE(list,iarg);

			if (isStruc(arg))
			{
				int    reuse = (isscratch(NAME(arg)) || isKeyword(arg));

				ncomps1 = NCOMPS(arg);
				for (jcomp = 0;jcomp < ncomps1; jcomp++)
				{
					component = COMPVALUE(arg,jcomp);
					if(reuse)
					{
						symh = reuseArg(arg, jcomp, 1, 1);
						if(symh == (Symbolhandle) 0)
						{ /* make sure symh is not null handle */
							symh = Makesymbol(NULLSYM);
							if(symh == (Symbolhandle) 0)
							{
								goto errorExit;
							}
						}
						COMPVALUE(result,ncomps++) = symh;
					}
					else
					{
						if (component != (Symbolhandle) 0)
						{
							symh = COMPVALUE(result,ncomps++) =
								Makesymbol(REAL);
							if(symh == (Symbolhandle) 0 || !Copy(component,symh))
							{
								goto errorExit;
							}
						} /*if (component != (Symbolhandle) 0)*/
						else
						{
							symh = COMPVALUE(result, ncomps++) =
								Makesymbol(NULLSYM);
							if (symh == (Symbolhandle) 0)
							{
								goto errorExit;
							}
						} /*if (component != (Symbolhandle) 0){}else{}*/
					}					
					setCompName(symh,NAME(symh));
				} /*for(jcomp=0;jcomp<ncomps1;jcomp++)*/
			} /* if(isStruc(arg)) */
			else
			{
				symh = setComp(list, result, iarg, ncomps++);
				if (symh == (Symbolhandle) 0)
				{
					goto errorExit;
				}
				setCompName(symh,NAME(symh));
			} /*if(isStruc(arg)) {...} else {...} */
		}/* for(iarg=0;iarg<nargs;iarg++)*/
	} /* if(!strconcat){...}else{...} */ 
	
	if (symhCompnames != (Symbolhandle) 0)
	{
		nameptr = STRINGPTR(symhCompnames);
		for (icomp = 0;icomp<ncomps;icomp++)
		{
			if(namestart[0] != '\0')
			{
				sprintf(name,"%s%ld",namestart,icomp+1);
			}
			else
			{
				if(strlen(nameptr) > NAMELENGTH)
				{
					foundLong++;
				}
				strncpy(name,nameptr,NAMELENGTH);
				name[NAMELENGTH] = '\0';
				nameptr = skipStrings(nameptr,1);
			}
			
			if(name[0] != '\0')
			{
				setCompName(COMPVALUE(result,icomp),name);
			}
		} /*for (icomp = 0;icomp<ncomps;icomp++)*/
		if(foundLong && !silent)
		{
			sprintf(OUTSTR,
					"WARNING: name%s longer than %ld characters truncated by %s()",
					(foundLong > 1) ? plural : NullString,
					(long) NAMELENGTH, FUNCNAME);
			putErrorOUTSTR();
		} /*if(foundLong && !silent)*/		
	} /*if(symhCompnames != (Symbolhandle) 0)*/
	
	return (result);

  errorExit:
	putErrorOUTSTR();
	Removesymbol(result);
	emptyTrash();
	return (0);
	
} /*makestr()*/

Symbolhandle compnames(Symbolhandle list)
{
	Symbolhandle        result = (Symbolhandle) 0, arg;
	char               *names;
	long                i, needed = 0, ncomps;
	
	if(NARGS(list) > 1)
	{
		badNargs(FUNCNAME,1);
		goto errorExit;
	}
	if(!argOK((arg = COMPVALUE(list,0)),STRUC,0))
	{
		goto errorExit;
	}
	ncomps = NCOMPS(arg);

	for(i=0;i<ncomps;i++)
	{
		needed += strlen(NAME(COMPVALUE(arg,i))) + 1;
	}
	result = CInstall(SCRATCH, needed);
	if(result == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	
	names = STRINGPTR(result);

	for(i=0;i<ncomps;i++)
	{
		strcpy(names, NAME(COMPVALUE(arg,i)));
		names = skipStrings(names, 1);
	}
	setNDIMS(result,1);
	setDIM(result,1,ncomps);

	return (result);
	
  errorExit:
	Removesymbol(result);
	
	return (0);
} /*compnames()*/

