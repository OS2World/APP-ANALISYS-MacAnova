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
#pragma segment List
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

/*
  990215 changed myerrorout() to putOutMsg()
  990226 replaced putOUTSTR() by putErrorOUTSTR()
*/

#include "globals.h"

static long            NObjects; /*number of items to list*/

#define Trash  NAMEFORTRASH
static Symbolhandle    Trash; /* will contain handles of items to list */

/*
  return 1 if name1 > name2, -1 if name1 < name2, 0 if name1 == name2
  when name1 is folded to lower case letters.  name2 is assumed already in
  lower case
*/
static short compareNames(char *name1, char *name2)
{
	register char           c1, c2, *nm1 = name1, *nm2 = name2;
	
	do
	{
		c1 = *nm1++;
		c2 = *nm2++;
		if (isupper(c1))
		{
			c1 = tolower(c1);
		}
		
		if (c1 != c2)
		{
			return( (c1 > c2) ? 1 : -1);
		}
	} while (c1 != '\0');

	return (0);
} /*compareNames()*/

static char     Target[NAMELENGTH+1];
static long     MatchType;

/* 980521 moved definition of match codes to globdefs.h */

enum listTypeCodes
{
	IREALS   = 0x01,
	ICHARS   = 0x02,
	ILOGICS  = 0x04,
	IMACROS  = 0x08,
	ISTRUCS  = 0x10,
	IGRAPHS  = 0x20,
	IFACTORS = 0x40,
	INULLS   = 0x80,
	IALL     = IREALS|ICHARS|ILOGICS|IMACROS|ISTRUCS|IGRAPHS|IFACTORS|INULLS
};

static char        *TypeKeys[] = 
{
	"real", "char", "logi", "macr", "stru", "grap",
	"fact", "null", "all", (char *) 0
};
static short        KeyCodes[] = 
{
	IREALS, ICHARS, ILOGICS, IMACROS, ISTRUCS, IGRAPHS,
	IFACTORS, INULLS, IALL
};

/* 970513 made v a de-referenced pointer rather than a handle */
static void sortObjects(void)
{
	long            n = NObjects;
	Symbolhandle   *v = (Symbolhandle *) *GARBAGEVALUE(Trash,0);
	Symbolhandle    z;
	char            pivot[NAMELENGTH+1], *pc1, *pc2;
	long            i, j, p, lv[20], uv[20];

	lv[0] = 0;
	uv[0] = n - 1;
	p = 0;
	while (p > -1)
	{
		if (lv[p] >= uv[p])
		{
			p = p - 1;
		} /*if (lv[p] >= uv[p])*/
		else
		{
			i = lv[p] - 1;
			j = uv[p];
			pc1 = NAME(v[j]);
			pc2 = pivot;
			while (*pc1)
			{
				*pc2 = *pc1++;
				if (isupper(*pc2))
				{
					*pc2 = tolower(*pc2);
				}
				pc2++;
			}
			*pc2 = '\0';
			while (i < j)
			{
				i = i + 1;
				while (compareNames(NAME(v[i]),pivot) < 0)
				{
					i = i + 1;
				}
				j = j - 1;
				while (j > i)
				{
					if (compareNames(NAME(v[j]), pivot) <= 0)
					{
						break;
					}
					j = j - 1;
				}
				if (i < j)
				{
					z = v[i];
					v[i] = v[j];
					v[j] = z;
				}
			}
			j = uv[p];
			z = v[i];
			v[i] = v[j];
			v[j] = z;
			if (i - lv[p] < uv[p] - i)
			{
				lv[p + 1] = lv[p];
				uv[p + 1] = i - 1;
				lv[p] = i + 1;
			}
			else
			{
				lv[p + 1] = i + 1;
				uv[p + 1] = uv[p];
				uv[p] = i - 1;
			}
			p++;
		} /*if (lv[p] >= uv[p]){}else{}*/
	} /*while (p > -1)*/
} /*sortObjects()*/

static void addToList(Symbolhandle symh)
{
	Symbolhandle     **objects = (Symbolhandle **) GARBAGEVALUE(Trash,0);
	
	if (matchName(NAME(symh),MatchType,Target))
	{
		(*objects)[NObjects++] = symh;
	}
} /*addToList()*/

/*
  970513 Added argument fmt.
         On listbrief() fmt is (char *) 0 when object is last in the
         output line
  970617 list says whether a macro is inline or out of line
  970628 gives dimensions of LONG variables
  980512 returns size of object if brief < -1
*/
static long listObject(Symbolhandle symh,long brief, char *fmt)
{
	char            *name;
	int              type;
	long             i, nclass, size = 0;
	WHERE("listObject");
	
	name = NAME(symh);
	type = TYPE(symh);
	
	if(brief < 0)
	{ /* list()*/
		if (symh != (Symbolhandle) 0)
		{
			char        *outstr = OUTSTR;
			
			sprintf(outstr,"%-14s %s%-6s",name,
					isSpecial(symh) ? "*" : " ", typeName(type));
			outstr += strlen(outstr);
			if(brief < -1)
			{ /*size:T*/
				size = SizeofSymbol(symh);
				sprintf(outstr, " %7ld   ", size);
				outstr += strlen(outstr);
			}

			if (type == REAL || type == LOGIC || type == CHAR ||
				type == LONG || type == STRUC)
			{
				for (i = 1; i <= NDIMS(symh); i++)
				{
					sprintf(outstr, " %-5ld", DIMVAL(symh,i));
					outstr += strlen(outstr);
				}
				if ((nclass = NCLASS(symh)) > 0)
				{
					sprintf(outstr, " FACTOR with %ld levels",nclass);
					outstr += strlen(outstr);
				}
			} /*if (type == REAL||type == LOGIC||type == CHAR||type == STRUC)*/
			else if (type == MACRO)
			{
				sprintf(outstr, " (%s)",
						(NCLASS(symh) < 0) ? "in-line" : "out-of-line");
			}
			putOUTSTR();
		} /*if (symh != (Symbolhandle) 0)*/
	} /*if (brief < 0)*/
	else
	{ /* listbrief() */
		if (fmt != (char *) 0)
		{ /*not last in line */
			sprintf(OUTSTR, fmt, name);
			myprint(OUTSTR);
		}
		else
		{
			putOutMsg(name);
		}
	} /*if (brief < 0){}else{}*/
	*OUTSTR = '\0';

	return (size);
} /*listObject()*/

static char    *KeyNames[] =
{
	"keep", "size", "invis", "nrows","ncols", "ndims"
};

enum listKeyCodes
{
	KKEEP,
	KSIZE,
	KINVIS,
	KNROWS,
	KNCOLS,
	KNDIMS,
	NKEYS
};

/*
  970513 modified slightly; removed globals Fmt and NperLine (now local
         to listbrief()) and added fmt as argument to listObject().
         The net effect is to eliminate some trailing spaces on listbrief()
         output.
  980521 Added arguments to call to scanPat() indicating attempting to
         match a MacAnova type name and providing a maximum length for
		 the pattern
*/
Symbolhandle    listbrief(Symbolhandle list)
{
	Symbolhandle    symh, result = (Symbolhandle) 0;
	Symbolhandle  **objects;
	long            type, i, j, badarg = 0, startArgs = 0;
	long            ikey, keyindex;
	long            brief = (strcmp(FUNCNAME,"listbrief") == 0) ? 1 : -1;
	long            foundUndef = 0;
	long            foundMissing = 0;
	char           *keyword, *string;
	char            foundKeyword = 0;
	char            fmt[10];
	long            keyValue, nargs = NARGS(list);
	long            op = 0, keep = 0, invis = 0;
	long            nosort = 0;
	long            maxNobj = Nsymbols - Nfunctions;
	long            nRows = -1, nCols = -1, nDims = -1;
	long            needed, nlines;
	long            nPerLine;
	WHERE("list");
	
	*OUTSTR = '\0';
	Trash = GarbInstall(1);
	if (Trash == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	
	/* 
	  Unadvertised feature:  Allow last argument 'nosort:T' to suppress sort
	*/

	if ((symh = COMPVALUE(list,nargs-1)) != (Symbolhandle) 0 &&
	   (keyword = isKeyword(symh)) && strcmp(keyword,"nosort") == 0 &&
	   isTorF(symh))
	{
		nosort = (DATAVALUE(symh,0) != 0.0) ? 1 : 0;
		if (nargs == 1)
		{
			Removesymbol(symh);
			COMPVALUE(list,0) = (Symbolhandle) 0;
		}
		else
		{
			nargs--;
		}
	}
	
	if (GUBED & 2)
	{
		maxNobj += Nscratch;
	}
	
	if (!getScratch(objects,0,maxNobj,Symbolhandle))
	{
		goto errorExit;
	}
	NObjects = 0;
	MatchType = ANYMATCH;
	

	if (!(GUBED & 2))
	{
		for (ikey = 0; ikey < NKEYS; ikey++)
		{			
			if ((keyindex = findKeyword(list, KeyNames[ikey], 0)) >= 0)
			{
				symh = COMPVALUE(list, keyindex);
				keyword = isKeyword(symh);
				setNAME(symh,USEDKEYWORD);
				if (ikey != KNROWS && ikey != KNCOLS && ikey != KNDIMS)
				{
					if (!isTorF(symh))
					{
						notTorF(keyword);
						goto errorExit;
					}
					keyValue = (DATAVALUE(symh,0) != 0.0);
					if (symh == COMPVALUE(list,nargs-1))
					{
						Removesymbol(symh);
						COMPVALUE(list,nargs-1) = (Symbolhandle) 0;
						nargs = (nargs > 1) ? nargs - 1 : 1;
					}
				} /*if (ikey != KNROWS && ikey != KNCOLS)*/
				else
				{
					if (!isInteger(symh, POSITIVEVALUE))
					{
						notPositiveInteger(keyword);
						goto errorExit;
					}
					keyValue = DATAVALUE(symh,0);
				} /*if (ikey != KNROWS && ikey != KNCOLS){}else{}*/
				
				if (ikey == KKEEP)
				{
					keep = keyValue;
				} /*if (ikey == KKEEP)*/
				else if (ikey == KSIZE)
				{
					if (brief > 0)
					{
						sprintf(OUTSTR,
								"WARNING: %s not legal keyword with %s(); ignored",
								keyword, FUNCNAME);
						putErrorOUTSTR();
					} /*if (brief > 0)*/
					else
					{
						brief = (keyValue) ? -2 : -1;
					}
				}
				else if (ikey == KINVIS)
				{
					invis = (keyValue) ? 1 : 0;
				}
				else if (ikey == KNROWS)
				{
					nRows = keyValue;
					foundKeyword = 1;
				}
				else if (ikey == KNCOLS)
				{
					nCols = keyValue;
					foundKeyword = 1;
				}
				else if (ikey == KNDIMS)
				{
					nDims = keyValue;
					foundKeyword = 1;
				}
			} /*if ((keyindex = findKeyword(list, KeyNames[ikey], 0)) >= 0)*/
		} /*for (ikey = 0; ikey < NKEYS; ikey++)*/
	} /*if (!(GUBED & 2))*/
	
	symh = COMPVALUE(list,0);
	if (isCharOrString(symh))
	{ /* first arg may be a pattern */
		if (isscratch(NAME(symh)) ||
		   (keyword = isKeyword(symh)) && strncmp(keyword,"pat",3) == 0)
		{
			string = STRINGPTR(symh);
			if (strlen(string) > NAMELENGTH)
			{
				sprintf(OUTSTR,"ERROR: %s is too long a pattern for %s",
						string, FUNCNAME);
				goto errorExit;
			}
			if (!scanPat(string, (long *) &MatchType, Target,
						 variableNameCheck, NAMELENGTH))
			{
				sprintf(OUTSTR,"ERROR: '%s' is not valid pattern for %s",
						string, FUNCNAME);
				goto errorExit;
			}
			
			startArgs = 1;
			symh = (Symbolhandle) 0;
		}
	} /*if (isCharOrString(symh))*/

	if (nargs == 1 && symh == (Symbolhandle) 0)
	{  /* no arguments or just pattern, list everything that matches */
		for (symh = Firstsymbol(0); symh != (Symbolhandle) 0 ;
			 symh = Nextsymbol(symh,0))
		{ /* omit builtins, undefined, temporary, and scratch variables */
			type = TYPE(symh);
			if (type != BLTIN &&
				(type != ASSIGNED && (invis ||
									  !istempname(NAME(symh)) &&
									  !invisname(NAME(symh))) || GUBED & 2))
			{
				addToList(symh);
			}
		}
		if (GUBED & 2)
		{ /* list scratch table */
			for (symh = Firstsymbol(1); symh != (Symbolhandle) 0; 
				symh = Nextsymbol(symh,1))
			{
				addToList(symh);
			}
		}
	} /*if (nargs == 1 && symh == (Symbolhandle) 0)*/
	else   /* one or more arguments  */
	{
		for (i = startArgs;i < nargs;i++)
		{
			symh = COMPVALUE(list,i);
			if (!(GUBED & 2)  && (keyword = isKeyword(symh)))
			{
				if (strcmp(keyword,USEDKEYWORD+2) != 0)
				{
					foundKeyword = 1;
					for (j=0; (string = TypeKeys[j]) != (char *) 0;j++)
					{
						if (strncmp(keyword,string,4) == 0)
						{
							if (!isTorF(symh))
							{
								notTorF(keyword);
								goto errorExit;
							}
							if (DATAVALUE(symh,0) != 0.0)
							{
								op |= KeyCodes[j];
							}
							else
							{
								op &= ~KeyCodes[j];
							}
							break;
						} /*if (strncmp(keyword,string,4) == 0)*/
					} /*for (j=0; (string = DeleteKeys[j]) != (char *) 0;j++)*/
					if (string == (char *) 0)
					{
						badKeyword(FUNCNAME, keyword);
						goto errorExit;
					}
				} /*if (strcmp(keyword,USEDKEYWORD+2) != 0)*/
			} /*if (!(GUBED&2) && (keyword = isKeyword(symh)))*/
			else if (foundKeyword)
			{
				sprintf(OUTSTR,
						"ERROR: illegal combination of keywords and variables in %s",
						FUNCNAME);
				goto errorExit;
			}
		} /*for (i = startArgs;i < nargs;i++)*/

		if (foundKeyword)
		{ /* foundKeyword not T if only found 'keep', 'size' or 'invis' */
			if (op == 0 && nRows < 0 && nCols < 0 && nDims < 0)
			{
				sprintf(OUTSTR,
						"WARNING: no object types specified for %s",FUNCNAME);
				putErrorOUTSTR();
			} /*if (op == 0 && nRows < 0 && nCols < 0 && nDims < 0)*/
			else	
			{
				long            dimensioned =
					IREALS | ICHARS | IFACTORS | ILOGICS;

				if (nRows > 0 || nCols > 0 || nDims > 0)
				{
					op = (op == 0) ? dimensioned : op & dimensioned;
				}
				
				for (symh=Firstsymbol(0);symh!= (Symbolhandle) 0;
						symh=Nextsymbol(symh,0))
				{/* omit scratch, builtins, undefined, temporary variables */
					type = TYPE(symh);
					if ((isDefined(symh) &&	(invis ||
											 !istempname(NAME(symh)) &&
											 !invisname(NAME(symh)))) ||
						GUBED & 2)
					{
						if (op & IREALS && type == REAL ||
						   op & IFACTORS && NCLASS(symh) > 0 && type == REAL ||
						   op & ICHARS && type == CHAR ||
						   op & IMACROS && type == MACRO ||
						   op & ILOGICS && type == LOGIC ||
						   op & IGRAPHS && type == PLOTINFO ||
						   op & INULLS && type == NULLSYM ||
						   op & ISTRUCS && type == STRUC)
						{
							if ((nCols < 0 ||
								 nCols == 1 && isVector(symh) ||
								 (nCols > 1 && NDIMS(symh) >= 2 &&
								  DIMVAL(symh,2) == nCols)) &&
								(nRows < 0 || DIMVAL(symh,1) == nRows) &&
								(nDims < 0 || NDIMS(symh) == nDims))
							{
								addToList(symh);
							}
						}
					}
				}
			} /*if (op == 0 && nRows < 0 && nCols < 0 && nDims < 0){}else{}*/
		} /* if (foundKeyword) */
		else
		{
			for (i = startArgs;i < nargs; i++)
			{
				symh = COMPVALUE(list,i);
				if (symh == (Symbolhandle) 0)
				{
					foundMissing = 1;
				}
				else if (GUBED & 2)
				{
					addToList(symh);
				}
				else if (isscratch(NAME(symh)))
				{
					badarg = 1;
				}
				else if (!isDefined(symh))
				{
					foundUndef = 1;
				}
				else if (!isKeyword(symh))
				{
					addToList(symh);
				}
			} /*for (i = startArgs;i < nargs; i++)*/
		} /* if (foundKeyword){}else{}*/
		
		if (badarg)
		{
			putOutErrorMsg("WARNING: arguments to list() must not be expressions or strings");
		} /*if (badarg)*/

		if (foundMissing)
		{
			sprintf(OUTSTR,
					"WARNING: missing argument(s) to %s", FUNCNAME);
			putErrorOUTSTR();
		} /*if (foundMissing)*/

		if (foundUndef)
		{
			for (i = 0;i < nargs; i++)
			{
				symh = COMPVALUE(list,i);
				if (symh != (Symbolhandle) 0 && !isDefined(symh))
				{
					sprintf(OUTSTR,"WARNING: %s is not defined",NAME(symh));
					putErrorOUTSTR();
				}
			} /*for (i = 0;i < nargs; i++)*/
		} /*if (foundUndef)*/
	} /*if (nargs == 1 && symh == (Symbolhandle) 0){}else{}*/

	if (!keep)
	{
		result = NULLSYMBOL;
	} /*if (!keep)*/

	if (NObjects != 0)
	{
		if (!nosort)
		{
			sortObjects();
		}
	
		if (!keep)
		{
			if (brief > 0)
			{
				nPerLine = SCREENWIDTH/(NAMELENGTH+1);
				if (nPerLine < 1)
				{
					nPerLine = 1;
				}
				sprintf(fmt,"%%-%lds",(long) NAMELENGTH+1);
			} /*if (brief > 0)*/

			if (brief < 0)
			{
				long     totalSize = 0;

				for (i=0;i < NObjects;i++)
				{
					totalSize += listObject((*objects)[i], brief, (char *) 0);
				}
				if (brief < -1)
				{
					sprintf(OUTSTR, "Total %-16s %7ld", "of listed",
							totalSize);
					putOUTSTR();
					sprintf(OUTSTR, "Total %-16s %7ld", "memory in use",
							CurrentMemory);
					putOUTSTR();
				}
			} /*if (brief < 0)*/
			else
			{
				nlines = (NObjects - 1)/nPerLine + 1;
				
				for (j = 0;j < nlines; j++)
				{
					for (i = j;i < NObjects;i += nlines)
					{
						(void) listObject((*objects)[i], brief,
										  (i + nlines < NObjects) ?
										  fmt : (char *) 0);
					}
				}
			} /*if (brief < 0){}else{}*/
		} /*if (!keep)*/
		else
		{
			needed = 0;
			for (i=0;i < NObjects;i++)
			{
				needed += strlen(NAME((*objects)[i])) + 1;
			}
			if ((result = Install(SCRATCH,CHAR)) == (Symbolhandle) 0 ||
			   (TMPHANDLE = mygethandle(needed)) == (char **) 0)
			{
				goto errorExit;
			}
			setSTRING(result,TMPHANDLE);
			setNDIMS(result,1);
			setDIM(result,1,NObjects);
			string = *TMPHANDLE;
			for (i = 0;i < NObjects;i++)
			{
				string = copyStrings(NAME((*objects)[i]), string, 1);
			}
		} /*if (!keep){}else{}*/
	} /*if (NObjects != 0)*/	
	else if (keep)
	{
		sprintf(OUTSTR,
				"ERROR: no variables found whose names to keep");
		goto errorExit;
	}
	
	emptyTrash();
	
	return (result);

  errorExit:
	putErrorOUTSTR();

	Removesymbol(result);

	emptyTrash();
	
	return (0);
} /*listbrief()*/


