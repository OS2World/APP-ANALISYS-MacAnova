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

/*
   Change history
   950712 started history
   950712 added usage delete(var,return:T) KB
   990112 fixed bug preventing @a <- NULL; delete(@a, return:T)
   990226 replaced putOUTSTR() by putErrorOUTSTR()
*/

#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment Delete
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
#include "dbug.h"
enum deleteKeys
{
	IREALS  = 0x0001,                                                
	ICHARS  = 0x0002,                                                
	ILOGICS = 0x0004,                                                
	IMACROS = 0x0008,                                                
	ISTRUCS = 0x0010,                                                
	IGRAPHS = 0x0020,                                                
	INULLS  = 0x0040,                                                
	IALL    = IREALS|ICHARS|ILOGICS|IMACROS|ISTRUCS|IGRAPHS|INULLS,
	IRETURN = 0x0080,
	IINVIS  = 0x0100,
	ISILENT = 0x0200
};

static char        *DeleteKeys[] = 
{
	"real", "char", "logi", "macr", "stru", "grap", "null", "all",
	"return", "invis", "silent", (char *) 0
};
static short        KeyCodes[] = 
{
	IREALS, ICHARS, ILOGICS, IMACROS, ISTRUCS, IGRAPHS, INULLS, IALL,
	IRETURN, IINVIS, ISILENT
};

/*
  990112 fixed bug preventing @a <- NULL; delete(@a, return:T)
  990204 added identification of macro
  990303 new keyword phrase invisible:T for use with return:T
*/
static Symbolhandle doDeleteReturn(Symbolhandle list, int visible)
{
	long              argno = 0, nargs = NARGS(list);
	Symbolhandle      symh = (Symbolhandle) 0;
	char             *badDeleteReturn = "ERROR: attempt to delete and return ";
	char             *name;

	for (argno = 0; argno < nargs; argno++)
	{ /* guaranteed to find a non-keyword argument */
		symh = COMPVALUE(list,argno);
		if (!isKeyword(symh))
		{
			break;
		}
	} /*for (argno = 0; argno < nargs; argno++)*/

	name = (symh != (Symbolhandle) 0) ? NAME(symh) : NullString;
	
	if (symh == (Symbolhandle) 0 || TYPE(symh) != NULLSYM && isNull(symh))
	{
		sprintf(OUTSTR, "%smissing argument%s", badDeleteReturn,
				inWhichMacro());
	}
	else if (!isDefined(symh))
	{
		sprintf(OUTSTR, "%sundefined argument %s%s", badDeleteReturn, name,
				inWhichMacro());
	}
	else if (TYPE(symh) == BLTIN)
	{
		sprintf(OUTSTR, "%sbuilt-in function %s()%s", badDeleteReturn, name,
				inWhichMacro());
	}
	else if (isAssigned(symh))
	{
		sprintf(OUTSTR,
				"%svariable %s to the left of '<-'%s", badDeleteReturn, name,
				inWhichMacro());
	}
	
	if (*OUTSTR)
	{
		goto errorExit;
	}
	COMPVALUE(list, argno) = (Symbolhandle) 0;
	if (!isscratch(name))
	{
		Cutsymbol(symh);
		setNAME(symh, (visible) ? SCRATCH : INVISSCRATCH);
		Addsymbol(symh);
	} /*if (!isscratch(name))*/
	
	return (symh);
	
  errorExit:
	putErrorOUTSTR();
	return (0);
	
} /*doDeleteReturn()*/

Symbolhandle    deleter(Symbolhandle list)
{
	/*  routine to delete a list of variables */

	Symbolhandle    symh, Install();
	Symbolhandle    next;
	long            i, j, nargs = NARGS(list);
	long            type;
	long            foundKey = 0, foundNonKey = 0;
	long            op = 0;
	long            ndeleted = 0;
	char           *keyword;
	int             silent = 0;
	WHERE("deleter");
	
	*OUTSTR = '\0';
	if (nargs == 1 && COMPVALUE(list, 0) == (Symbolhandle) 0)
	{
		nargs = 0;
	}
	
	for(i = 0; i < nargs; i++)
	{
		symh = COMPVALUE(list,i);
		if ((keyword = isKeyword(symh)))
		{
			int       logicVal;

			for(j=0;DeleteKeys[j] != (char *) 0;j++)
			{
				if (strncmp(keyword,DeleteKeys[j],4) == 0)
				{
					break;
				}
			} /*for(j=0;DeleteKeys[j] != (char *) 0;j++)*/

			if (DeleteKeys[j] == (char *) 0)
			{
				badKeyword(FUNCNAME,keyword);
				goto errorExit;
			}

			if (!isTorF(symh))
			{
				notTorF(keyword);
				goto errorExit;
			}

			logicVal = (DATAVALUE(symh,0) != 0.0);
			if (logicVal)
			{
				op |= KeyCodes[j];
			}
			else
			{
				op &= ~KeyCodes[j];
			}
			if (KeyCodes[j] == ISILENT)
			{
				if (i < nargs - 1)
				{
					sprintf(OUTSTR,
							"ERROR: keyword %s must be last argument on %s()",
							keyword, FUNCNAME);
					goto errorExit;
				}
				silent = logicVal;
				nargs--;
			}
			else if (KeyCodes[j] == IRETURN && !logicVal)
			{
				sprintf(OUTSTR,
						"ERROR: return:F is illegal on %s()", FUNCNAME);
				goto errorExit;
			}
			
			if (!(KeyCodes[j] & (IRETURN | IINVIS | ISILENT)))
			{
				foundKey++;
			}
		} /*if ((keyword = isKeyword(symh)))*/
		else
		{
			foundNonKey++;
		} /*if ((keyword = isKeyword(symh))){}else if (foundKey){}*/
	} /*for(i = 0; i < nargs; i++)*/

	if (op & (IRETURN | IINVIS))
	{
		int         visible = !(op & IINVIS);
		
		if (foundKey != 0)
		{
			sprintf(OUTSTR,
					"ERROR: only keywords 'silent' and 'invisible' can be used with return:T");
		}
		else if (!(op & IRETURN))
		{
			sprintf(OUTSTR,
					"ERROR: 'invisible:T can be used only with 'return:T'");
		}
		else if (foundNonKey != 1)
		{
			sprintf(OUTSTR,
					"ERROR: you can delete only 1 variable when using return:T");
		}
		if (*OUTSTR)
		{
			goto errorExit;
		}
		return (doDeleteReturn(list, visible));

	} /*if (op & (IRETURN | IINVIS))*/

	if (foundKey && foundNonKey)
	{
		
		sprintf(OUTSTR,
				"ERROR: illegal to have both keywords and variables as %s arguments",
				FUNCNAME);
		goto errorExit;
	} /*if (foundKey && foundNonKey > 0)*/
	
	if (nargs == 0)
	{
		if (!silent)
		{
			putOutErrorMsg("WARNING: nothing specified to delete");
		}
	}
	else if (!foundKey)
	{
		if (!silent)
		{
			for (i = 0; i < nargs; i++)
			{
				if (COMPVALUE(list, i) == (Symbolhandle) 0)
				{
					sprintf(OUTSTR,
							"WARNING: missing argument to %s()", FUNCNAME);
					putErrorOUTSTR();
				} /*if (COMPVALUE(list, i) == (Symbolhandle) 0)*/
			} /*for (i = 0; i < nargs; i++)*/
		} /*if (!silent)*/		

		for (i = 0; i < nargs; i++)
		{
			symh = COMPVALUE(list, i);
			if (!isKeyword(symh) && symh != (Symbolhandle) 0)
			{
				int          duplicated = 0;
				
				COMPVALUE(list,i) = (Symbolhandle) 0;
				if (myvalidhandle((char **) symh))
				{
					duplicated = 0;
					for(j = i+1;j < nargs;j++)
					{
						if (COMPVALUE(list,j) == symh)
						{
							duplicated = 1;
							if (!silent)
							{
								sprintf(OUTSTR,
										"WARNING: duplicate variable in list");
								putErrorOUTSTR();
							}
							break;
						}
					} /*for(j = i+1;j < nargs;j++)*/
				} /*if (myvalidhandle((char **) symh))*/				

				if (!duplicated)
				{ /* not duplicated later in list so delete it if possible*/
					if (!myvalidhandle((char **) symh) ||
						!isNull(symh) && !isDefined(symh))
					{
						if (isFakeSymbol(symh))
						{
							sprintf(OUTSTR,
									"WARNING: attempt to delete built-in function %s()",
									NAME(symh));
						}
						else
						{
							sprintf(OUTSTR,
									"WARNING: attempt to delete undefined variable %s",
									(myvalidhandle((char **) symh)) ?
									NAME(symh) :"");
						}
					}
					else if (isNull(symh) && TYPE(symh) != NULLSYM)
					{
						sprintf(OUTSTR,"WARNING: null argument to %s",
								FUNCNAME);
					}
					else if (isscratch(NAME(symh)))
					{
						sprintf(OUTSTR,
								"WARNING: attempt to delete string, expression, or function result");
					}
					else if ((keyword = isKeyword(symh)))
					{
						badKeyword(FUNCNAME, keyword);
					}
					else if (isAssigned(symh))
					{
						sprintf(OUTSTR,
								"WARNING: attempt to delete variable being assigned a value; ignored");
					}
					else
					{
						Removesymbol(symh);
					}
				} /*if (*OUTSTR == '\0')*/
				if (!silent)
				{
					putErrorOUTSTR();
				}
			} /*if (symh != (Symbolhandle) 0)*/
		} /*for (i = 0; i < nargs; i++)*/
	} /*if (!foundKey)*/
	else
	{
		if (op != 0)
		{
			symh = Firstsymbol(0);
			while (symh != (Symbolhandle) 0)
			{					/* loop through regular symbol table*/
				next = Nextsymbol(symh,0);
				type  = TYPE(symh);
				if (type == REAL && op & IREALS ||
				   type == CHAR && op & ICHARS ||
				   type == LOGIC && op & ILOGICS ||
				   type == MACRO && op & IMACROS ||
				   type == STRUC && op & ISTRUCS ||
				   type == CHAR && op & ICHARS ||
				   type == NULLSYM && op & INULLS ||
				   type == PLOTINFO && op & IGRAPHS)
				{
					ndeleted++;
					Removesymbol(symh);
				}
				symh = next;
			} /*while (symh != (Symbolhandle) 0)*/
			if (!silent && ndeleted == 0)
			{
				putOutErrorMsg("WARNING: nothing deleted");
			}
		} /*if (op != 0)*/
	} /*if (!foundKey){}else{}*/
	
	
	return (NULLSYMBOL);

  errorExit:
	putErrorOUTSTR();
	
	return (0);
	
}
