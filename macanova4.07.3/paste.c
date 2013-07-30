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
#pragma segment Paste
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

#define CHUNK    (5*sizeof(double))
#define MAXIWIDTH 100
#define MAXFWIDTH 100
#define MAXFMTBUF  20

enum pasteKeyCodes
{
	ISEP = 1,
	IFORMAT,
	IIWIDTH,
	IAWIDTH,
	IMISS,
	IMULTILINE,
	ILINESEP,
	IJUSTIFY
};

static char       *LegalKeywords[] =
{
	"sep", "format", "intwidth", "charwidth", "missing", 
	"multiline", "linesep", "justify", (char *) 0
};

/*
   routine to create scalar CHARACTER variable representing arg in a
   form that might be pasted into a spread sheet

   In contains each row of the input as a separate line, with tab separated
   elements.  If arg is CHAR, any embedded tabs or new lines may screw
   things up.

   REALs are written in NUMBERFORMAT format.
   LOGICAL T and F are written as 1 and 0 (T and F as of 961023)

   MISSING REALs are written as MISSINGREAL
   MISSING LOGICALs are written as MISSINGLOGIC

   960313 Works with MACRO variables, so that you can assign a macro
   to CLIPBOARD

   961023 Corrected bug that screened out MACRO variables before they
   every got to multipaste().  Also changed behavior so that lineSep
   is not appended to the last line when nrows > 1.  This makes it
   uniform -- no lineSep at end of last line.  And changed it so that
   T and F are printed as T and F instead of 1 and 0.  User can
   use 1*logicVar to get old behavior

   971104 added check for value of myhandlelength()

   990213 NULL arguments now allowed.  They are essentially equivalent
          to "" except they do not expand to charwidth blanks.

   990226 Replaced putOUTSTR() by putErrorOUTSTR()
*/

#define NUMBERCHUNK    21  /*guess at bytes needed per number */

Symbolhandle multipaste(Symbolhandle arg, char *format,
					   char ** stringForMissing, char fieldSep, char lineSep)
{
	Symbolhandle      result = (Symbolhandle) 0;
	long              i, j, ij, length, spaceleft, nleft;
	long              place, islogic;
	long              nrows, ncols, ndims, dims[2];
	long              type = TYPE(arg);
	int               addNewline;
	char             *pc, *pij, *pji, *rowi;
	double            x;
	WHERE("multipaste");
	
	if (type != MACRO && type != NULLSYM)
	{		
		if (isMatrix(arg, dims))
		{
			nrows = dims[0];
			ncols = dims[1];
		} /*if (isMatrix(arg, dims))*/
		else
		{
			ndims = NDIMS(arg);
			for (i = 1; i<= ndims; i++)
			{
				/* find first dimension > 1 */
				nrows = DIMVAL(arg, i);
				if (nrows > 1)
				{
					break;
				}
			}
			ncols = symbolSize(arg)/nrows;
		} /*if (isMatrix(arg, dims)){}else{}*/
		addNewline = (nrows > 1) ? 1 : 0;
	} /*if (type != MACRO)*/
	else
	{
		ndims = ncols = nrows = 1;
		addNewline = 0;
	} /*if (type != MACRO){}else{}*/
	
	if ((result = CInstall(SCRATCH, 0)) == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	if (type == MACRO)
	{
		if ((TMPHANDLE = myduphandle(STRING(arg))) == (char **) 0)
		{
			goto errorExit;
		}
		setSTRING(result, TMPHANDLE);
	} /*if (type == MACRO)*/
	else if (type == CHAR || type == NULLSYM)
	{
		if (type == CHAR)
		{
			length = myhandlelength(STRING(arg));
			if (length < 0)
			{
				goto errorExit;
			}
		
			length += addNewline;
		}
		else
		{
			length = 1;
		}
		
		if ((TMPHANDLE = mygethandle(length)) == (char **) 0)
		{
			goto errorExit;
		}
		setSTRING(result, TMPHANDLE);

		pji = STRINGPTR(result);
		if (type == CHAR)
		{
			rowi = STRINGPTR(arg);
			for (i = 0;i < nrows; i++)
			{
				pij = rowi;
				for (j = 0; j < ncols; j++)
				{
					for (pc = pij; *pc; pc++, pji++)
					{
						*pji = *pc;
					}
					*pji++ = (j == ncols - 1) ? lineSep : fieldSep;
					if (j < ncols - 1)
					{
						pij = skipStrings(pij, nrows);
					}
				} /*for (j = 0; j < ncols; j++)*/
				if (i < nrows - 1)
				{
					rowi = skipStrings(rowi, 1);
				}
			} /*for (i = 0;i < nrows; i++)*/
#if (0) /* never add trailing lineSep */
			if (!addNewline || lineSep == '\0')
#endif /*0*/
			{ /* no trailing lineSep if only one row */
				pji--;
			}
		} /*if (type == CHAR)*/
		*pji = '\0';
	} /*if (type == MACRO){}else if (type == CHAR || type == NULLSYM)*/
	else 
	{ /* REAL or LOGIC */
		islogic = (type == LOGIC);
		nleft = nrows*ncols;
		spaceleft = nleft * ((!islogic) ? NUMBERCHUNK : 2);
		TMPHANDLE = mygethandle(spaceleft+1);
		setSTRING(result, TMPHANDLE);
		if (TMPHANDLE == (char **) 0)
		{
			goto errorExit;
		}

		place = 0;
		for (i = 0; i < nrows; i++)
		{
			for (j = 0, ij = i;j < ncols; j++, ij+= nrows)
			{
				/* pick off data values accross rows */
				x = DATAVALUE(arg, ij);
				if (isMissing(x))
				{
					strcpy(OUTSTR, *stringForMissing);
				}
				else if (islogic)
				{
					strcpy(OUTSTR, (x != 0.0) ? "T" : "F");
				}
				else
				{ /* trim off leading and trailing blanks*/
					sprintf(OUTSTR, format, (x) ? x : 0.0);
					trimBuffer(OUTSTR, TRIMLEFT | TRIMRIGHT);
				}

				length = strlen(OUTSTR);
				if (spaceleft < length + 1)
				{
					spaceleft = nleft*NUMBERCHUNK + 10;
					TMPHANDLE = mygrowhandle(TMPHANDLE, place+spaceleft+1);
					setSTRING(result, TMPHANDLE);
					if (TMPHANDLE == (char **) 0)
					{
						goto errorExit;
					}
				} /*if (spaceleft < length + 1)*/
				strcpy (*TMPHANDLE + place, OUTSTR);
				place += length;
				(*TMPHANDLE)[place++] = (j < ncols - 1) ? fieldSep : lineSep;
				nleft--;
				spaceleft -= length + 1;
			} /*for (j = 0, ij = i;j < ncols; j++, ij+= nrows)*/
		} /*for (i = 0; i < nrows; i++)*/

#if (0) /*never add trailing line separator*/
		if (nrows == 1 || lineSep == '\0')
#endif /*0*/
		{
			place--;
		}
		
		(*TMPHANDLE)[place++] = '\0';
		TMPHANDLE = mygrowhandle(TMPHANDLE, place);
		setSTRING(result, TMPHANDLE);
		if (TMPHANDLE == (char **) 0)
		{
			goto errorExit;
		}
	} /*if (type == CHAR){}else{}*/	

	setNAME(result, SCRATCH);
	setNDIMS(result, 1);
	setDIM(result,1, (lineSep == '\0') ? nrows : 1);
	*OUTSTR = '\0';
	
	return (result);

  errorExit:
	Removesymbol(result);
	*OUTSTR = '\0';
	return (0);
} /*multipaste()*/

enum
{
	LEFTJUSTIFY = 1,
	RIGHTJUSTIFY,
	CENTERJUSTIFY
};

Symbolhandle paste(Symbolhandle list)
{
	long          nargs = NARGS(list);
	Symbolhandle  symh, result = (Symbolhandle) 0;
	char        **resultString = (char **) 0;
	char         *space = " ";
	char        **separator = (char **) 0;
	char          format[MAXFMTBUF+1];
	char          intformat[MAXFMTBUF+1];
	char          missformat[MAXFMTBUF+1];
	char         *keyword, *outstr;
	char         *nameForMissing = NAMEFORMISSING;
	char        **missingCode = (char **) 0; /*handle*/
	char         *thisString;
	char          fieldsep = '\0', linesep = '\0';
	int           i, j, k;
	long          nchar = 0, nitems, nleft = CHUNK;
	long          length, place=0, needed, strlength;
	long          iwidth = -1, fwidth = -1, awidth = -1;
	long          type, key, multiline = 0;
	long          justify = 0, padleft, padright;
	long          fmt[3];
	long          nkey = 0;
	double        value;
	WHERE("paste");
	
	intformat[0] = format[0] = OUTSTR[0] = '\0';
	
	fmt[0] = -1;
	/* check all arguments first and count keys */
	for (i=0;i<nargs;i++)
	{
		if (!argOK(symh = COMPVALUE(list,i), NULLSYM, i+1))
		{
			return (0);
		}
		if ((type = TYPE(symh)) != LOGIC && type != REAL && type != CHAR &&
		   type != MACRO && type != NULLSYM)
		{
			badType(FUNCNAME, -type, i+1);
			goto errorExit;
		}
		
		if ((keyword = isKeyword(symh)))
		{
			nkey++;
			key = matchKey(keyword,LegalKeywords, (long *) 0);
			if (key == 0)
			{
				badKeyword(FUNCNAME,keyword);
				goto errorExit;
			} /*if (key == 0)*/

			if ((key == ISEP || key == IFORMAT || key == IMISS || 
				key == ILINESEP || key == IJUSTIFY) &&
			   !isCharOrString(symh))
			{
				notCharOrString(keyword);
				goto errorExit;
			}

			if (key == IJUSTIFY)
			{
				if (strlen(STRINGPTR(symh)) == 0 ||
					!strchr("rRlLcC", STRINGVALUE(symh,0)))
				{
					sprintf(OUTSTR,
							"ERROR: illegal value for %s; must be \"left\", \"right\", or \"center\"",
							keyword);
					goto errorExit;
				}
			}
			if ((key == IIWIDTH || key == IAWIDTH) &&
					!isInteger(symh,POSITIVEVALUE))
			{
				notPositiveInteger(keyword);
				goto errorExit;
			}
			if (key == IMULTILINE && !isTorF(symh))
			{
				notTorF(keyword);
				goto errorExit;
			}
			if (key == IMULTILINE)
			{
				multiline = (DATAVALUE(symh,0) != 0.0);
			}
		} /*if ((keyword = isKeyword(symh)))*/
	} /*for (i=0;i<nargs;i++)*/

	if (multiline)
	{
		keyword = LegalKeywords[IMULTILINE-1];
		if (nargs - nkey != 1)
		{
			sprintf(OUTSTR,
					"ERROR: there must be exactly 1 non-keyword argument to %s with %s:T",
					FUNCNAME, keyword);
			goto errorExit;
		}
		symh = COMPVALUE(list,0);
		type = TYPE(symh);
		if (isKeyword(symh))
		{
			sprintf(OUTSTR,
					"ERROR: first argument to %s must not be keyword with %s:T",
					FUNCNAME, keyword);
		}
		if (*OUTSTR)
		{
			goto errorExit;
		}
	} /*if (multiline)*/
	
/*
   scan trailing keywords for format info or separator; stop scanning
   when non-keyword is found or all formatting keywords and separator
   are set
*/
	for (i=nargs-1;i>=0;i--)
	{
		symh = COMPVALUE(list,i);
		if (!(keyword = isKeyword(symh)))
		{
			break;
		}
		nargs--;
		key = matchKey(keyword,LegalKeywords, (long *) 0);
		if (TYPE(symh) == CHAR)
		{
			thisString = STRINGPTR(symh);
		}
		/* trailing separators or formats become the default */
		if (key == ISEP && separator == (char **) 0)
		{
			separator = STRING(symh);
		}
		else if (key == IFORMAT && *format == '\0')
		{
			if (!setFormat(thisString,fmt))
			{
				sprintf(OUTSTR,
						"ERROR: %s is not legal form for format for %s",
						thisString,FUNCNAME);
				goto errorExit;
			}
			if (isdigit(thisString[0]))
			{
				fwidth = fmt[0];
				if (fwidth > MAXFWIDTH)
				{
					fwidth = fmt[0] = MAXFWIDTH;
				}
			}
		}
		else if (key == IIWIDTH && iwidth < 0)
		{
			if (multiline)
			{
				sprintf(OUTSTR, "WARNING: keyword %s ignored with %s:T",
						keyword, LegalKeywords[IMULTILINE-1]);
				putErrorOUTSTR();
			}
			else
			{
				iwidth = (long) DATAVALUE(symh,0) + 1000;
				if (iwidth > 1000 + MAXIWIDTH)
				{
					iwidth = 1000 + MAXIWIDTH;
				}
				sprintf(intformat,"%%%ldld", iwidth - 1000);
			}
		}
		else if (key == IAWIDTH && awidth < 0)
		{
			if (multiline)
			{
				sprintf(OUTSTR, "WARNING: keyword %s ignored with %s:T",
						keyword, LegalKeywords[IMULTILINE-1]);
				putErrorOUTSTR();
			}
			else
			{
				awidth = (long) DATAVALUE(symh,0);
			}
		}
		else if (key == IMISS && missingCode == (char **) 0)
		{
			missingCode = STRING(symh);
		}
		else if (key == ILINESEP && linesep == '\0')
		{
			if (!multiline)
			{
				sprintf(OUTSTR,
						"ERROR: '%s' not legal without %s:T",
						keyword, LegalKeywords[IMULTILINE - 1]);
				goto errorExit;
			}
			thisString = STRINGPTR(symh);
			if (strlen(thisString) != 1)
			{
				sprintf(OUTSTR,
						"ERROR: value for '%s' must be string with 1 character, e.g., \"\\n\"",
						keyword);
				goto errorExit;
			}
			linesep = thisString[0];
		}
		else if (key == IJUSTIFY && justify == 0)
		{
			if (thisString[0] == 'r' || thisString[0] == 'R')
			{
				justify = RIGHTJUSTIFY;
			}
			else if (thisString[0] == 'l' || thisString[0] == 'L')
			{
				justify = LEFTJUSTIFY;
			}
			else
			{
				justify = CENTERJUSTIFY;
			}
		}
		
		if (separator != (char **) 0 && fmt[0] > 0 &&
		   iwidth > 0 && awidth > 0 && justify != 0)
		{
			break;
		}
	} /*for (i=nargs-1;i>=0;i--)*/

	if (separator == (char **) 0)
	{
		separator = &space;
	}

	if (missingCode == (char **) 0)
	{
		missingCode = &nameForMissing;
	}

	if (justify == 0)
	{
		justify = LEFTJUSTIFY;
	}
	
	if (!multiline)
	{
		if (iwidth < 0 && fwidth >= 0)
		{
			iwidth = fwidth;
			sprintf(intformat,"%%%ldld", iwidth);
		}
	} /*if (!multiline)*/
	else 
	{
		if (strlen(*separator) != 1)
		{
			sprintf(OUTSTR,
					"ERROR: value for '%s' must be string with 1 character with %s:T",
					LegalKeywords[ISEP-1], LegalKeywords[IMULTILINE-1]);
		}
#if (0) /*multipaste() can now handle macros*/
		else if (type == MACRO)
		{
			sprintf(OUTSTR,
					"ERROR: first argument to %s must not have type %s with %s:T",
					FUNCNAME, typeName(type), LegalKeywords[IMULTILINE-1]);
		}
#endif /*0*/
		if (*OUTSTR)
		{
			goto errorExit;
		}
		fieldsep = (*separator)[0];
	} /*if (!multiline){}else{}*/
	
	if (fmt[0] < 0)
	{
		fmt[0] = PRINTFORMAT[0];
		fmt[1] = PRINTFORMAT[1];
		fmt[2] = PRINTFORMAT[2];
	}
	if (fmt[0] >= 0)
	{
		sprintf(format,"%%%ld.%ld%c",fmt[0],fmt[1],(char) fmt[2]);
	}
	if (iwidth < 0)
	{
		strcpy(intformat,"%ld");
	}
	
	if (multiline)
	{
		result = multipaste(COMPVALUE(list,0), format,
						   missingCode, fieldsep, linesep);
	} /*if (multiline)*/
	else
	{		
		result = Install(SCRATCH,CHAR);
		if (result == (Symbolhandle) 0)
		{
			goto errorExit;
		}
	
		resultString = mygethandle(nleft);
		setSTRING(result,resultString);
		if (resultString == (char **) 0)
		{
			goto errorExit;
		}
		setNDIMS(result,1);
		setDIM(result,1,1);

		**resultString = '\0';
		nchar++;
		nleft--;
		
		for (i=0;i<nargs;i++)
		{
			symh = COMPVALUE(list,i);
			type = TYPE(symh);
			
			if ((keyword = isKeyword(symh)))
			{
				if (type == CHAR)
				{
					thisString = STRINGPTR(symh);
				}
				
				key = matchKey(keyword,LegalKeywords, (long *) 0);
				if (key == ISEP)
				{
					separator = STRING(symh);
				}
				else if (key == IFORMAT)
				{
					if (!setFormat(thisString,fmt))
					{
						sprintf(OUTSTR,
								"ERROR: %s is not legal form for format for %s",
								thisString,FUNCNAME);
						goto errorExit;
					}
					if (isdigit(thisString[0]))
					{
						fwidth = fmt[0];
						if (fwidth > MAXFWIDTH)
						{
							fwidth = fmt[0] = MAXFWIDTH;
						}
						if (iwidth < 1000)
						{
							iwidth = fwidth;
							sprintf(intformat,"%%%ldld", iwidth);
						}
					}
					else
					{
						fwidth = -1;
						if (iwidth < 1000)
						{
							iwidth = -1;
						}
					}
				}
				else if (key == IIWIDTH)
				{
					iwidth = (long) DATAVALUE(symh,0) + 1000;
					if (iwidth > 1000 + MAXIWIDTH)
					{
						iwidth = 1000 + MAXIWIDTH;
					}
					sprintf(intformat,"%%%ldld", iwidth - 1000);
				}
				else if (key == IAWIDTH)
				{
					awidth = (long) DATAVALUE(symh,0);
				}
				else if (key == IMISS)
				{
					missingCode = STRING(symh);
				}
				else if (key == IJUSTIFY)
				{
					if (thisString[0] == 'r' || thisString[0] == 'R')
					{
						justify = RIGHTJUSTIFY;
					}
					else if (thisString[0] == 'l' || thisString[0] == 'L')
					{
						justify = LEFTJUSTIFY;
					}
					else
					{
						justify = CENTERJUSTIFY;
					}
				}
				sprintf(format,"%%%ld.%ld%c",fmt[0],fmt[1],(char) fmt[2]);
			} /*if ((keyword = isKeyword(symh)))*/
			else if (type != NULLSYM)
			{
				nitems = symbolSize(symh);
				place = 0;
				for (j=0;j < nitems;j++)
				{
					if (nchar > 1)
					{
						strcpy(OUTSTR,*separator);
					}
					else
					{
						*OUTSTR = '\0';
					}
				
					outstr = OUTSTR + strlen(OUTSTR);

					if ((type == REAL || type == LOGIC) &&
						(value = DATAVALUE(symh,j), isMissing(value)))
					{
						if (fwidth > 0)
						{
							sprintf(missformat,"%%%lds",fwidth);
						}
						else
						{
							strcpy(missformat,"%s");
						}
						sprintf(outstr,missformat,*missingCode);
					}
					else
					{
						switch ((int) type)
						{
						  case LOGIC:
							sprintf(outstr,"%s", (value != 0.0) ? "T" : "F");
							break;
					
						  case REAL:
							if (fabs(value) < 2147483647.5 && value == floor(value))
							{
								sprintf(outstr,intformat, (long) value);
							}
							else
							{
								sprintf(outstr,format,value);
								if (fwidth < 0)
								{ /* no width specified */
									for (k=0;outstr[k] == *space;k++)
									{
										;
									}
									if (k > 0)
									{ /* trim off leading blanks by shifting left */
										strcpy(outstr, outstr+k);
									}
									length = strlen(outstr);
									while (outstr[length-1] == *space)
									{ /* trim off trailing blanks */
										length--;
									}
									outstr[length] = '\0';
								} /*if (fwidth < 0)*/
							}
							break;

						  case CHAR:
						  case MACRO:
					/* do not copy directly to OUTSTR; may be too long */
							needed = strlength = strlen(STRINGPTR(symh)+place);
							
							padleft = padright = 0;
							if (awidth > needed)
							{
								needed = awidth;
								if (justify == LEFTJUSTIFY)
								{
									padright = awidth - strlength;
								}
								else if (justify == RIGHTJUSTIFY)
								{
									padleft = awidth - strlength;
								}
								else
								{
									padleft = (awidth - strlength)/2;
									padright = awidth - strlength - padleft;
								}
							}							
							break;
					
						} /*switch ((int) type)*/
					}

					length = strlen(OUTSTR);
					if (type == CHAR || type == MACRO)
					{
						length += needed;
					}
				
					if (length >= nleft)
					{
						nleft = CHUNK*((length - 1)/CHUNK + 1);
						resultString = mygrowhandle(resultString,nchar+nleft);
						setSTRING(result,resultString);
						if (resultString == (char **) 0)
						{
							goto errorExit;
						}
					} /*if (length >= nleft)*/
					strcat(*resultString, OUTSTR);

					if (type == CHAR || type == MACRO)
					{	/* copy directly from source */
						outstr = *resultString + strlen(*resultString);

						while (padleft-- > 0)
						{
							*outstr++ = ' ';
						}
						strcpy(outstr,STRINGPTR(symh)+place);
						outstr += strlength;
						while (padright-- > 0)
						{
							*outstr++ = ' ';
						}
						*outstr = '\0';
						place += strlength + 1;
					} /*if (type == CHAR || type == MACRO)*/
				
					nleft -= length;
					nchar += length;
					*OUTSTR = '\0';
				} /*for (j=0;j<nitems;j++)*/
				if (isscratch(NAME(symh)))
				{ /* immediately reclaim memory space */
					COMPVALUE(list, i) = (Symbolhandle) 0;
					Removesymbol(symh);
				}
			} /*if ((keyword = isKeyword(symh))){}else{}*/
		} /*for (i=0;i<nargs;i++)*/

		/* trim resultString to exact size */
		resultString = mygrowhandle(resultString, nchar);
		setSTRING(result,resultString);
		if (resultString == (char **) 0)
		{
			goto errorExit;
		}
	} /*if (multiline){}else{}*/	

	return (result);
	
  errorExit:
	putErrorOUTSTR();
	Removesymbol(result);
	return (0);
} /*paste()*/
