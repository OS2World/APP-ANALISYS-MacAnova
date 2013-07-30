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
   Function to modify a structure by replacing, adding or deleting
   a component

  changestr(struc, n, [newname:]value), 1 <= n <= ncomps(struc) + 1
	If n <= ncomps(struct), replace component n by value
	If n == ncomps(struc) + 1, add new component.  If value is given
	by a keyword phrase, the keyword names the component; otherwise
	the name will be the name of value.

  changestr(struc,compName, value)
	If compName is the name of a component, replace the component by value
	If compName does not match any component, a new component will be
	added with name compName.

  changestr(struc, -n), -n < 0
    component n will be deleted

  971118 new usage:
  changestr(struc,name:value)
	If name matches a component name, the component is given the new value
	Otherwise, a new component is added.
    Also, changed naming convention so that
	  (i)  Keyword name for value always prevails
      (ii) If no name is supplied by keyword when replacing a component,
           the previous name is retained
  990226 replaced putOUTSTR() by putErrorOUTSTR()
*/

Symbolhandle    changestr(Symbolhandle list)
{
	Symbolhandle    arg[3], symh, symh1, result = (Symbolhandle) 0;
	Symbolhandle    newComp;
	long            iarg, jcomp, ncomps, mcomps, icomp, type1;
	long            targetComp = 0;
	long            nargs = NARGS(list);
	char            compName[NAMELENGTH+1], *keyword = (char *) 0;
	char            newName[NAMELENGTH+1];
	WHERE("changestr");

	OUTSTR[0] = '\0';
	compName[0] = newName[0] = '\0';
	
	if (nargs < 2 || nargs > 3)
	{
		badNargs(FUNCNAME, (nargs < 2) ? -(1000+2) : -3);
		goto errorExit;
	}

	/* check all arguments before we begin */
	for (iarg = 0;iarg < nargs;iarg++)
	{
		if ((arg[iarg] = COMPVALUE(list, iarg)) == (Symbolhandle) 0)
		{
			noData(FUNCNAME, iarg+1);
			goto errorExit;
		}
		if (!isDefined(arg[iarg]))
		{
			undefArg(FUNCNAME, arg[iarg], iarg+1);
			goto errorExit;
		}
	} /*for (iarg = 0;iarg < nargs;iarg++)*/

	if (TYPE(arg[0]) != STRUC)
	{
		badType(FUNCNAME, STRUC, 1);
		goto errorExit;
	}

	mcomps = ncomps = NCOMPS(arg[0]);

	type1 = TYPE(arg[1]);
	if (nargs == 2)
	{
		if (!(keyword = isKeyword(arg[1])))
		{
			if (!isInteger(arg[1], NEGATIVEVALUE))
			{
				sprintf(OUTSTR,
						"ERROR: argument 2 of 2 to %s() is not a keyword or integer < 0",
						FUNCNAME);
				goto errorExit;
			}
			targetComp = (long) DATAVALUE(arg[1], 0);
		}
		else
		{
			strcpy(compName, keyword);
			strcpy(newName, keyword);
			keyword = (char *) 0;
			newComp = arg[1];
		}
	} /*if (nargs == 2)*/
	else
	{ /*nargs == 3*/
		if (!isCharOrString(arg[1]) && !isInteger(arg[1], ANYVALUE))
		{
			sprintf(OUTSTR,
					"ERROR: argument 2 of 3 to %s() not CHARACTER scalar or integer",
					FUNCNAME);
			goto errorExit;
		}

		if ((keyword = isKeyword(arg[2])))
		{
			strcpy(newName, keyword);
		}
	
		if (type1 == REAL)
		{
			targetComp = (long) DATAVALUE(arg[1], 0);
			if (targetComp <= 0)
			{
				sprintf(OUTSTR,
						"ERROR: argument 2 <= 0 illegal for %s() with 3 arguments",
						FUNCNAME);
				goto errorExit;
			}
		} /*if (type1 == REAL)*/
		else
		{ /* TYPE(arg[1]) is CHAR */
			if (strlen(STRINGPTR(arg[1])) > NAMELENGTH)
			{
				sprintf(OUTSTR, 
						"ERROR: '%s' is too long to be a component name in %s", 
						STRINGPTR(arg[1]), FUNCNAME);
				goto errorExit;
			}
			strcpy(compName, STRINGPTR(arg[1]));
		}
		newComp = arg[2];
	} /*if (nargs == 2){}else{}*/

	if (compName[0] != '\0')
	{/* either changstr(str,newname:value) or changstr(str,"newname",value)*/
		for (icomp = 0;icomp < ncomps;icomp++)
		{
			if (strcmp(compName, NAME(COMPVALUE(arg[0], icomp))) == 0)
			{
				break;
			}
		}
		targetComp = icomp + 1;
	} /*if (type1 == REAL){}else{}*/

	if (targetComp < 0)
	{ /* deleting component */
		if (-targetComp > ncomps)
		{
			sprintf(OUTSTR,
					"ERROR: %s() can't delete a non-existent component",
					FUNCNAME);
			goto errorExit;
		}
		if (ncomps == 1)
		{
			sprintf(OUTSTR, 
					"ERROR: %s() can't delete the only component of a structure",
					FUNCNAME);
			goto errorExit;
		}
		mcomps--;
	} /*if (targetComp < 0)*/
	else if (targetComp > ncomps + 1)
	{
		sprintf(OUTSTR,
				"ERROR: argument 2 to %s() > ncomps(structure) + 1", 
				FUNCNAME);
		goto errorExit;
	}
	else if (targetComp == ncomps + 1)
	{
		/* adding component */
		if (compName[0] == '\0')
		{
			/* must be changestr(str, ncomps+1, value) */
			strcpy(compName, NAME(newComp));
		}
		mcomps++;
	}
	
	if ((result = StrucInstall(SCRATCH, mcomps)) == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	if (newName[0] == '\0' && compName[0] != '\0')
	{
		strcpy(newName, compName);
	}

	for (icomp = 0, jcomp = 0;icomp < mcomps; icomp++, jcomp++)
	{
		COMPVALUE(result, icomp) = symh = Makesymbol(ASSIGNED);
		if (symh == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		
		if (icomp == targetComp - 1)
		{
			symh1 = newComp;
			if (newName[0] == '\0')
			{
				strcpy(newName, NAME(COMPVALUE(arg[0], jcomp)));
			}
		}
		else
		{
			if (-targetComp == icomp + 1)
			{ /* skip component to be deleted */
				jcomp++;
			}
			symh1 = COMPVALUE(arg[0], jcomp);
		}
		
		if (!Copy(symh1, symh))
		{
			goto errorExit;
		}
	} /*for (icomp = 0;icomp < mcomps;icomp++, jcomp++)*/

	if (targetComp > 0)
	{
		if (newName[0] == '\0')
		{
			strcpy(newName, compName);
		}
		setCompName(COMPVALUE(result, targetComp - 1), newName);
	}
	
	return (result);

  errorExit:
	putErrorOUTSTR();

	Removesymbol(result);
	return (0);
	
} /*changestr()*/
