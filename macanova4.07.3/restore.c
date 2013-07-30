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
#pragma segment Save
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
#include "mvsave.h"

#ifdef MACINTOSH
#include "macIface.h"
#endif /*MACINTOSH*/

#ifdef WXWIN
#include "wx/wxIface.h"
#endif /*WXWIN*/

#include <ctype.h>

/*
  replaced putOUTSTR() by putErrorOUTSTR() in most places
  replaced myerrorout() by putOutErrorMsg() and putOutMsg()
*/

#define CHUNK         (5*sizeof(double)) /* units to ask for more room */
#define CR            13
#define LF            10
#define TIMELENGTH    30 /* length of buffer for time information */

#define HEADERLENGTH  24 /* strlen("////MacAnovaSaveFile////") */

static char         **Home = (char **) 0; /* parking place for HOME */

static char           LastRestoreWasCr = 0;

#undef CR
#undef NL
#define CR   13
#define NL   10

static int restoregetc(FILE * fp)
{
	int           c;

	c = fgetc(fp);
	if (!Binary)
	{
		c = (c == NL && LastRestoreWasCr) ? fgetc(fp) : c;
		LastRestoreWasCr = (c == CR);
#ifndef MACINTOSH
		c = (LastRestoreWasCr) ? '\n' : c;
#else
		c = (c == NL) ? '\n' : c;
#endif /*MACINTOSH*/

#ifdef MSDOS
#define MSDOSEOF    26
	c = (c == MSDOSEOF) ? EOF : c;
#endif /*MSDOS*/
	} /*if (!Binary)*/
	
	return (c);
} /*restoregetc()*/

/*
  Test for a name match.  If a match is found, the data part of symh
  is copied to pointer dPointer and 1 is returned.  Otherwise 0 is returned.
*/
static long symbol2Double(double *dPointer,char *name,long size,
						  Symbolhandle symh)
{
	WHERE("symbol2Double");

	if (strcmp(NAME(symh), name) != 0)
	{
		return (0);
	}
	size = (size <= DIMVAL(symh, 1)) ? size : DIMVAL(symh, 1);
	doubleCopy(DATAPTR(symh), dPointer, size);
	return (1);
} /*symbol2Double()*/

/*
  Test for a name match.  If a match is found, the data part of symh
  is copied to pointer lPointer and 1 is returned.  Otherwise 0 is returned.
*/
static long symbol2Long(long *lPointer,char *name,long size, Symbolhandle symh)
{
	long                i;
	WHERE("symbol2Long");

	if(strcmp(NAME(symh),name) != 0)
	{
		return (0);
	}
	size = (size <= DIMVAL(symh,1)) ? size : DIMVAL(symh,1);
	for (i = 0;i < size;i++)
	{
		*lPointer++ = LONGDATAVALUE(symh,i);
	}
	return (1);
} /*symbol2Long()*/

/*
  Test for a name match.  If a match is found, the current handle is
  disposed of, it is reset to DATA(symh), DATA(symh) is cleared, and 1
  is returned.  Otherwise 0 is returned.
*/

static long symbol2Handle(double ***handle,char *name,Symbolhandle symh)
{
	if (strcmp(NAME(symh),name) != 0)
	{
		return (0);
	}

	if (*handle != (double **) 0)
	{
		mydisphandle((char **) *handle);
	}

	*handle = DATA(symh);
	setDATA(symh, (double **) 0);
	return (1);
} /*symbol2Handle()*/

/*
  980717 New function to search for SYMBOLMARKER (%#$SYMBOL$#%)
         put before each symbol name.  Someday this may facilitate
         recovery from restore errors.
*/
static int getSymbolName(char name [])
{
	if (!Pre407Save && (!restoreName(name) || strcmp(name, SYMBOLMARKER) != 0))
	{
		return (0);
	}
	
	if (!restoreName(name) || strlen(name) > NAMELENGTH)
	{
		return (0);
	}
	
	return (1);
} /*getSymbolName()*/

/*
  restoreSymbolLabels() used for restores from files in Pre407Save formats 
*/
static int restoreSymbolLabels(Symbolhandle symh, long labelsLength)
{
	long          i, j, ndims = NDIMS(symh);
	long          labelsActual = 0;
	char         *labels;
	WHERE("restoreSymbolLabels");
	
	TMPHANDLE = mygethandle(labelsLength);
	if (TMPHANDLE == (char **) 0 || !setLabels(symh, TMPHANDLE))
	{
		mydisphandle(TMPHANDLE);
		return (0);
	}

	labels = *TMPHANDLE;
	for (i = 1; i <= ndims; i++)
	{
		for (j = 0; j < DIMVAL(symh, i); j++)
		{
			if (!restoreString(labels))
			{
				return (0);
			}
			labelsActual += strlen(labels) + 1;
			labels = skipStrings(labels,1);
		} /*for (j = 0; j < DIMVAL(symh, i); j++)*/				
	} /*for (i = 1; i <= ndims; i++)*/

	if (labelsActual != labelsLength)
	{
		return (0);
	}

	return (1);
} /*restoreSymbolLabels()*/

/*
  Read symbol from file.  It is immediately put in symbol table and returned
  as an installed symbol.  This means a structure component must be cut from
  the symbol table before being attached.

  980717 symbol name is now read by getSymbolName()
  980718 symbol labels are now read by restoreSymbolLabels()
*/
static Symbolhandle restoresym(void)
{
	long            i, j, tot, spaceleft, ch;
	long            type, dimi, nclass, length, nchar = 0;
	long            ndims = -1, mdims;
	long            labelsLength = -1;
	Symbolhandle    symh = (Symbolhandle) 0, component;
	char          **chardata = (char **) 0, name[NAMELENGTH+1];
	char            itemCvalue[MAXCHARITEMSIZE + 1];
	char           *typename = itemCvalue;
	long          **longdata;
	double        **data;
	whole_graph   **graph = (whole_graph **) 0;
	WHERE("restoresym");

	/* get symbol name */
	if (!getSymbolName(itemCvalue) || strlen(itemCvalue) > NAMELENGTH+1)
	{
		goto errorExit;
	}
	strcpy(name, itemCvalue);

	/* get symbol type */
	if (ASCII == SAVEBINARY1 || ASCII == SAVEASCII1)
	{ /* numeric type of non-ascii and old ascii format */
		if (!restoreLong(&type))
		{
			goto errorExit;
		}
		/* make sure codes are restore correctly even if future codes change */
		switch (type)
		{
		  case CHAR_V31:
			type = CHAR;
			break;
		  case REAL_V31:
			type = REAL;
			break;
		  case LOGIC_V31:
			type = LOGIC;
			break;
		  case STRUC_V31:
			type = STRUC;
			break;
		  case MACRO_V31:
			type = MACRO;
			break;
		  case PLOTINFO_V31:
			type = PLOTINFO;
			break;
		  case LONG_V31:
			type = LONG;
			break;
		  default:
			;
		} /*switch (type)*/
	} /*if(ASCII == SAVEBINARY1 || ASCII == SAVEASCII1)*/
	else
	{
		if (!restoreName(typename))
		{/* name-specified type for new ascii formats and new binary format */
			goto errorExit;
		}
		if (strncmp(typename, "SHORT", 5) == 0)
		{
			typename += 5;
			type = SHORTSYMBOL;
		}
		else
		{
			type = 0;
		}
		
		if(strcmp(typename,"REAL") == 0)
		{
			type |= REAL;
		}
		else if(strcmp(typename,"LOGIC") == 0)
		{
			type |= LOGIC;
		}
		else if(strcmp(typename,"LONG") == 0)
		{
			type |= LONG;
		}
		else if(strcmp(typename,"CHAR") == 0)
		{
			type |= CHAR;
		}
		else if(strcmp(typename,"STRUC") == 0)
		{
			type |= STRUC;
		}
		else if(strcmp(typename,"MACRO") == 0)
		{
			type |= MACRO;
		}
		else if(strcmp(typename,"GRAPH") == 0)
		{
			type |= PLOTINFO;
		}
		else if(strcmp(typename,"NULL") == 0)
		{
			type |= NULLSYM;
		}
		else
		{ /* not recognized */
			goto errorExit;
		}
	}
	
	symh = Install(name, type); /* immediately add to symbol table */

	if (symh == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	if (Pre407Save)
	{
		/* get NCLASS */
		if (!restoreLong(&nclass))
		{
			goto errorExit;
		}
		setNCLASS(symh,nclass);

		/* get NDIMS */
		if (!restoreLong(&ndims))
		{
			goto errorExit;
		}
		setNDIMS(symh,ndims);

		/* get dimensions */
		mdims = (ASCII >= SAVEASCII3 || ASCII <= SAVEBINARY3) ? ndims : MAXDIMS;
		for (i = 1; i <= mdims; i++)
		{
			if (!restoreLong(&dimi))
			{
				goto errorExit;
			}
			if(i <= ndims)
			{
				setDIM(symh,i,dimi);
			}
		} /*for (i = 1; i <= mdims; i++)*/

		if (!Pre400Save)
		{
			if (!restoreLong(&labelsLength) ||
				labelsLength > 0 && !restoreSymbolLabels(symh, labelsLength))
			{
				goto errorExit;
			}
		}
	} /*if (Pre407Save)*/
	else
	{
		double        itemDvalue;
		long          itemLvalue, itemLength;
		long          nlabels = -1;
		int           itemNumber, itemType;
		char          itemCvalue[MAXCHARITEMSIZE + 1];
#ifndef LABELSARECHAR
		Symbolhandle  labelsSymh = (Symbolhandle) 0;
#endif /*LABELSARECHAR*/
#ifndef NOTESARECHAR
		Symbolhandle  notesSymh = (Symbolhandle) 0;
#endif /*NOTESARECHAR*/

		/* Look for Symbol items until itemNumber == EndItems */
		while (1)
		{
			if (!getNextRestoreItem(SymbolPrefix, &itemNumber, &itemType,
									&itemLength, &itemDvalue, &itemLvalue,
									itemCvalue))
			{
				goto errorExit;
			}

			if (itemNumber == EndItems)
			{
				break;
			}
			
			if (itemNumber < SYMENDSYM)
			{ /* known item */
				if (itemType != SYMITEMTYPE(itemNumber))
				{
					goto errorExit;
				}

				switch (itemNumber)
				{
				  case SYMNCLASS:
					setNCLASS(symh, itemLvalue);
					break;

				  case SYMDIMS:
					ndims = itemLength;
					setNDIMS(symh, ndims);
					nlabels = 0;

					if (ISSHORT(symh) && ndims != 1)
					{
						goto errorExit;
					}
					
					for (i = 1; i <= ndims; i++)
					{
						long         dimi;

						if (!restoreLong(&dimi))
						{
							goto errorExit;
						}
						setDIM(symh, i, dimi);
						nlabels += dimi;
					} /*for (i = 1; i <= ndims; i++)*/
					break;

#ifdef LABELSARECHAR
				  case SYMLABLEN:
					labelsLength = itemLvalue;
					if (labelsLength < 0)
					{
						goto errorExit;
					}
					break;

				  case SYMLABELS:
					if (labelsLength <= 0 || nlabels != itemLength ||
						!restoreSymbolLabels(symh, labelsLength))
					{
						goto errorExit;
					}
					break;
#else /*LABELSARECHAR*/
				  case SYMLABELS:
					labelsSymh = restoresym();

					if (labelsSymh == (Symbolhandle) 0 ||
						NDIMS(labelsSymh) != 1 ||
						DIMVAL(labelsSymh, 1) != nlabels ||
						!setLabels(symh, STRING(labelsSymh)))
					{
						Removesymbol(labelsSymh);
						goto errorExit;
					}
					setSTRING(labelsSymh, (char **) 0);
					Removesymbol(labelsSymh);
					break;
					
#endif /*LABELSARECHAR*/

#ifndef NOTESARECHAR
				  case SYMNOTES:
					notesSymh = restoresym();

					if (notesSymh == (Symbolhandle) 0 ||
						NDIMS(notesSymh) != 1 ||
						!setNotes(symh, STRING(notesSymh)))
					{
						Removesymbol(notesSymh);
						goto errorExit;
					}
					setSTRING(notesSymh, (char **) 0);
					Removesymbol(notesSymh);
					break;
					
#endif /*NOTESARECHAR*/
				} /*switch (itemNumber)*/
			} /*if (itemNumber < SYMENDSYM)*/
			else if (itemType & SAVEVECTOR)
			{ /*unrecognized vector type ; skip value*/
				if (!skipRestoreVector(itemType, itemLength))
				{
					goto errorExit;
				}
			} /*else if (itemType & SAVEVECTOR)*/
		} /*while (1)*/ 
	} /*if (Pre407Save){}else{}*/
	
	if (DIMVAL(symh, 1) == 0)
	{
		setTYPE(symh, NULLSYM | ((ISSHORT(symh)) ? SHORTSYMBOL : 0));
	} /*if (DIMVAL(symh, 1) == 0)*/
	else
	{
		tot = symbolSize(symh);
		switch  (type & TYPEMASK)
		{
		  case STRUC:
			TMPHANDLE = mygethandle((long) tot * sizeof(Symbolhandle));
			setCOMP(symh,(Symbolhandle**) TMPHANDLE);
			if (COMP(symh) == (Symbolhandle**) 0)
			{
				goto errorExit;
			}
			for (i = 0; i < tot; i++)
			{
				COMPVALUE(symh, i) = (Symbolhandle) 0;
			}
			for (i = 0; i < tot; i++)
			{
				component = restoresym();
				Cutsymbol(component);
				COMPVALUE(symh, i) = component;
				if (component == (Symbolhandle) 0)
				{
					goto errorExit;
				}
			}
			break;

		  case MACRO:
		  case CHAR:
			if(Binary || ASCII == SAVEASCII1)
			{
				chardata = mygethandle((long) CHUNK);
				setSTRING(symh,chardata);
				spaceleft = CHUNK;
				if (chardata == (char **) 0)
				{
					goto errorExit;
				}
				j = 0;
				i = 0;
				while (i < tot && (ch = restoregetc(RESTOREFILE)) != EOF)
				{
					if (spaceleft < 2)
					{
						chardata = mygrowhandle((char **) chardata,
												(long) j + CHUNK);
						setSTRING(symh,chardata);
						spaceleft = CHUNK;
						if (chardata == (char **) 0)
						{
							goto errorExit;
						}
					}
					(*chardata)[j++] = (char) ch;
					spaceleft--;
					if (ch == '\0')
					{
						i++;
					}
				} /*while (i < tot && (ch = restoregetc(RESTOREFILE)) != EOF)*/
			} /*if(Binary || ASCII == SAVEASCII1)*/
			else
			{					/* ASCII > SAVEASCII1 */
				for(i=0;i<tot;i++)
				{
					if(!restoreLong(&length))
					{
						goto errorExit;
					}
					if(i == 0)
					{
						setSTRING(symh,mygethandle(length + 1));
					}
					else
					{
						setSTRING(symh,
								  mygrowhandle(chardata, nchar + length + 1));
					}
					chardata = STRING(symh);
					if(chardata == (char **) 0)
					{
						goto errorExit;
					}
					if (!restoreNString(length,*chardata + nchar))
					{
						goto errorExit;
					}
					nchar += length + 1;
				}
			}

			/* if necessary, shrink to exact size */
			if((Binary || ASCII == SAVEASCII1) && spaceleft > 0)
			{
				setSTRING(symh,(char **) 0);
				chardata = mygrowhandle(chardata, j);
				setSTRING(symh,chardata);
				if(chardata == (char **) 0)
				{
					goto errorExit;
				}
			}
			break;

		  case PLOTINFO:
#if (1) /* 980714 new calling sequence for restoreGraph()*/
			if (!restoreGraph(symh))
			{
				goto errorExit;
			}
#else /*1*/
			graph = restoreGraph(); /* restoreGraph() is in plotutil.c */
			if(graph == (whole_graph **) 0)
			{
				goto errorExit;
			}
			setGRAPH(symh, graph);
#endif /*1*/
			break;

		  case REAL:
		  case LOGIC:

			if(tot > 0)
			{/* allow for possibility of null symbol on save file */
				/* This shouldn't happen for type 5 files or later */
				data = (double **) mygethandle(tot * sizeof(double));
				setDATA(symh,data);
				if (data == (double **) 0)
				{
					goto errorExit;
				}
				if (!Binary)
				{				/* ascii file */
					for (i = 0; i < tot; i++)
					{
						if(!restoreDouble(*data + i))
						{
							goto errorExit;
						}
					}
				} /*if (!Binary)*/
				else
				{				/* binary file */
					if(fread((void *) *data, sizeof(double),
							 (size_t) tot, RESTOREFILE) != tot)
					{
						goto errorExit;
					}
					if (ASCII > SAVEBINARY5)
					{
						double    *x = *data;

						for (i = 0; i < tot; i++)
						{
							if (x[i] == OLDMISSING)
							{
								setMissing(x[i]);
							}
						} /*for (i = 0; i < tot; i++)*/
					} /*if (ASCII > SAVEBINARY5)*/
				} /*if (!Binary){}else{}*/
			} /*if(tot > 0)*/

			break;

		  case LONG:
			longdata = (long **) mygethandle((long) tot * sizeof(long));
			setLONGDATA(symh,longdata);
			if (longdata == (long **) 0)
			{
				goto errorExit;
			}

			if (!Binary)
			{					/* ascii file */
				for (i = 0; i < tot; i++)
				{
					if(!restoreLong(*longdata + i))
					{
						goto errorExit;
					}
				}
			}
			else
			{					/* binary file */
				if (fread((void *) *longdata, sizeof(long), (size_t) tot,
						  RESTOREFILE) != tot)
				{
					goto errorExit;
				}
			}
			break;
		} /*switch  (type & TYPEMASK)*/
	} /*if (DIMVAL(symh, 1) == 0){}else{}*/

	return (symh);

  errorExit:
	Removesymbol(symh);

	return (0);

} /*restoresym()*/

/* bit definitions used in previous versions */
#define BALANOVA     0x002L  /* 2 balanced anova */
#define UNBALANO     0x010L  /* 16 unbalanced anova */
#define OLDOLSREG    0x020L  /* 32 standard regression */
#define OLDROBUSTREG 0x040L  /* 64 robust regression */
/*
  971104 added check on value of myhandlelength()
*/
static void restoreGlobals(Symbolhandle globals)
{
	long                icomp, i, j, n, k, nvars;
	Symbolhandle        symh, symh1;
	double              dTemp[MAXVARS];
	double            **model = (double **) 0;
	double              errorterms[MAXERRTRMS + 1];
	unsigned long       tempModelType = 0;
	unsigned long       tempGlmControl = 0;
	long                maxvars = MAXVARS;
	long                neweststyle =
	  (ASCII <= SAVEBINARY5 || ASCII >= SAVEASCII5);
	WHERE("restoreGlobals");

	nvars = -1;
	for(icomp=0;icomp<NCOMPS(globals);icomp++)
	{
		symh = COMPVALUE(globals,icomp);

		if(symbol2Double(&NVARS,"NVARS",1,symh))
		{
			nvars = MODELINFO->nvars = NVARS;
		}
		else if(symbol2Long(&maxvars,"MAXVARS",1,symh))
		{
			if(maxvars > MAXVARS)
			{
				sprintf(OUTSTR,
						"ERROR: Model information saved by version allowing %ld variables in models",
						maxvars);
				putOUTSTR();
				sprintf(OUTSTR,
						"       This version allows only %ld.  Model information not restored",
						(long) MAXVARS);
				putErrorOUTSTR();
				NVARS = MODELINFO->nvars = -1;
				break;
			}
		}
		else if(symbol2Double(&NFACTORS,"NFACTORS",1,symh))
		{
			MODELINFO->nfactors = NFACTORS;
		}
		else if(symbol2Double(&NDATA,"NDATA",1,symh))
			;
		else if(symbol2Double(&NOTMISSING,"NOTMISSING",1,symh))
			;
		else if(symbol2Double(&NTERMS,"NTERMS",1,symh))
		{
			MODELINFO->nterms = NTERMS;
		}
		else if(symbol2Double(&NREGX,"NREGX",1,symh))
			;
		else if(symbol2Double(&NY,"NY",1,symh))
			;
		else if(symbol2Double(&NERRORTERMS,"NERRORTERMS",1,symh))
		{
			MODELINFO->nerrorterms = NERRORTERMS;
		}
		else if(symbol2Double(&PREVGLMSCALE,"PREVGLMSCALE",1,symh))
			;
		else if(symbol2Double(dTemp,"PREVMODELTYP",1,symh))
		{
			tempModelType = (long) dTemp[0];
			if (!neweststyle)
			{
				if (tempModelType & OLDROBUSTREG)
				{
					putOutErrorMsg("WARNING: cannot restore information from robust() in previous version");
					NVARS = MODELINFO->nvars = -1;
					break;
				} /*if (tempModelType & OLDROBUSTREG)*/
				
				tempGlmControl = UNBALANCED;
				if (tempModelType & LEAPS)
				{
					tempGlmControl |= NORMALDIST | IDENTLINK;
				}
				else if (tempModelType & BALANOVA)
				{
					tempModelType &= ~BALANOVA;
					tempModelType |= ANOVA;
					tempGlmControl &= ~UNBALANCED;
					tempGlmControl |= (NORMALDIST|IDENTLINK);
				}
				else if (tempModelType & FASTANOVA)
				{
					tempGlmControl |= NORMALDIST | IDENTLINK;
				}
				else if (tempModelType & MANOVA)
				{
					tempGlmControl |= (NORMALDIST||IDENTLINK|MULTIVAR);
				}
				else if (tempModelType & UNBALANO)
				{
					tempModelType &= ~UNBALANO;
					tempModelType |= ANOVA;
					tempGlmControl |= (NORMALDIST|IDENTLINK);
				}
				else if (tempModelType & OLDOLSREG)
				{
					tempModelType &= ~OLDOLSREG;
					tempModelType |= OLSREG;
					tempGlmControl |= NORMALDIST | IDENTLINK;
				}
				else if (tempModelType & LOGITREG)
				{
					tempGlmControl |= BINOMDIST | LOGITLINK;
				}
				else if (tempModelType & POISSONREG)
				{
					tempGlmControl |= POISSONDIST | LOGLINK;
				}
			}
		}
		else if(symbol2Double(dTemp,"PREVGLMCONTR",1,symh))
		{
			tempGlmControl = (long) dTemp[0];
		}
		else if(symbol2Double(dTemp,"INCREMENTAL",1,symh))
		{
			INCREMENTAL = (long) dTemp[0];
		}
		else if(symbol2Double(dTemp,"USEGLMOFFSET",1,symh))
		{
			USEGLMOFFSET = (long) dTemp[0];
		}
		else if(symbol2Double(dTemp,"NCLASSES",MAXVARS,symh))
		{
			for(j=0;j<maxvars;j++)
			{
				NCLASSES[j] = (j < nvars) ? (long) dTemp[j] : 0;
			}
		}
		else if(symbol2Double(errorterms,"ERRORTERMS",MAXERRTRMS+1,symh))
		{
			for(j=0;j<=MAXERRTRMS;j++)
			{
				zeroTerm(MODELINFO->errorterms[j]);
				MODELINFO->errorterms[j][0] = (long) errorterms[j];
			}
		}
		else if(symbol2Long((long *) MODELINFO->errorterms,"ERRORTERMS1",
							(WORDSPERTERM)*(MAXERRTRMS+1),symh))
			;
		else if(symbol2Handle((double ***) &DEPVNAME,"DEPVNAME",symh))
			;
		else if(strcmp(NAME(symh),"VARNAMES") == 0)
		{
			n = (DIMVAL(symh,1) <= maxvars+1) ? DIMVAL(symh,1) : maxvars+1;
			k = 0;
			for(j=0;j<=maxvars;j++)
			{
				if(j < n)
				{
					strcpy(VARNAMES[j],STRINGPTR(symh) + k);
					k += strlen(VARNAMES[j]) + 1;
				}
				else
				{
					VARNAMES[j][0] = '\0';
				}
			}
		}
		else if(symbol2Handle(&model,"MODEL",symh))
		{
			long      length = myhandlelength((char **) model);

			if (length > 0)
			{
				length /= sizeof(double);
				TMPHANDLE = mygethandle(length * sizeof(modelType));
			}
			else
			{
				TMPHANDLE = (char **) 0;
			} 
		
			if(TMPHANDLE == (char **) 0)
			{
				NVARS = MODELINFO->nvars = -1;
				putOutMsg("       Model information not restored");
				break;
			}
			MODEL = (modelHandle) TMPHANDLE;
			for(i=0;i<length;i++)
			{
				zeroTerm((*MODEL)[i]);
				(*MODEL)[i][0] = (termWord) (*model)[i];
			}
		}
		else if(symbol2Handle((double ***) &MODEL,"MODEL1",symh))
		{
			;
		}
		/*
		  Under current version, Y should always be the same as
		  DATA(MODELVARS[0]) so we no longer restore it;  however, we
		  do recognize it
		*/
		else if(strcmp(NAME(symh),"Y") == 0)
			;
		else if(symbol2Handle(&RESIDUALS,"RESIDUALS",symh))
			;
		else if(symbol2Handle(&WTDRESIDUALS,"WTDRESIDUALS",symh))
			;
		else if(symbol2Handle(&GLMOFFSET,"GLMOFFSET",symh))
			;
		else if(symbol2Handle(&HII,"HII",symh))
			;
		else if(symbol2Handle(&SS,"SS",symh))
			;
		else if(symbol2Handle(&DF,"DF",symh))
			;
		else if(symbol2Handle(&MISSWTS,"MISSWTS",symh))
			;
		else if(symbol2Handle(&CASEWTS,"CASEWTS",symh))
			;
		else if(symbol2Handle(&ITERWTS,"ITERWTS",symh))
			;
		else if(symbol2Handle(&ALLWTS,"ALLWTS",symh))
			;
		else if(symbol2Handle(&LOGITN,"LOGITN",symh))
			;
		else if(strcmp(NAME(symh),"STRMODEL") == 0)
		{
			if(STRMODEL != (char **) 0)
			{
				mydisphandle(STRMODEL);
			}
			STRMODEL = STRING(symh);
			setSTRING(symh,(char **) 0);
		}
		else if(strcmp(NAME(symh),"TERMNAMES") == 0)
		{
			if(TERMNAMES != (char **) 0)
			{
				mydisphandle(TERMNAMES);
			}
			TERMNAMES = STRING(symh);
			setSTRING(symh,(char **) 0);
		}
		else if(symbol2Handle(&REGX,"REGX",symh))
			;
		else if(symbol2Handle(&REGX2,"REGX2",symh))
			;
		else if(symbol2Handle(&XTXINV,"XTXINV",symh))
			;
		else if(symbol2Handle(&REGCOEF,"REGCOEF",symh))
			;
		else if(symbol2Handle(&REGSS,"REGSS",symh))
			;

		/*
		  Under current version, X[i] should always be the same as
		  DATA(MODELVARS[i+1]) so we no longer restore it.  We do
		  recognize it, however
		*/
		else if(strcmp(NAME(symh),"X") == 0)
			;
		else if(strcmp(NAME(symh),"MODELVARS") == 0)
		{
			n = NCOMPS(symh);
			for(j=0;j<=maxvars;j++)
			{
				symh1 = (j < n) ? COMPVALUE(symh,j) : (Symbolhandle) 0;
				MODELVARS[j] = symh1;
				if(j < n)
				{
					COMPVALUE(symh,j) = (Symbolhandle) 0;
					if(j==0)
					{
						Y = DATA(MODELVARS[0]);
					}
					else
					{
						X[j-1] = DATA(MODELVARS[j]);
					}
				} /*if(j < n)*/
			} /*for(j=0;j<=maxvars;j++)*/
		}
		else
		{
			putOutErrorMsg("WARNING: unrecognized global variable on file. Globals not restored");
			NVARS = MODELINFO->nvars = -1;
			break;
		}
	} /*for(icomp=0;icomp<NCOMPS(globals);icomp++)*/
	if (NVARS >= 0)
	{
		PREVMODELTYPE = tempModelType;
		PREVGLMCONTROL = tempGlmControl;
	}
} /*restoreGlobals()*/

#ifdef PLATFORM
/*
	980403 added checks for PLATFORM_ALT5 through PLATFORM_ALT8
*/
enum machineStatusConstants
{
	okPlatform,
	badPlatform,
	badGlm
};

/*
  980728  changed so that LIMITS is expected to be a CHARACTER vector
          when !Pre407
*/
#define NLIMITS  4

static long checkMachine(Symbolhandle globals)
{
	long                icomp;
	Symbolhandle        symh;
	long                limitsType = (Pre407Save) ? LONG : CHAR;
	long                retvalue = okPlatform;
	WHERE("checkMachine");

	for (icomp = 0;icomp < NCOMPS(globals);icomp++)
	{
		symh = COMPVALUE(globals,icomp);

		if (strcmp(NAME(symh), "LIMITS") == 0 && TYPE(symh) == limitsType &&
			isVector(symh) && DIMVAL(symh, 1) == NLIMITS)
		{
			long       limits[NLIMITS];
			long       maxdims, maxvars, wordsperterm, varsperword;

			if (Pre407Save)
			{
				symbol2Long(limits, "LIMITS", NLIMITS, symh);
			} /*if (Pre407Save)*/
			else
			{
				int        i;
				char      *place = STRINGPTR(symh);
				
				for (i = 0; i < NLIMITS; i++)
				{
					if (sscanf(place, "%ld", limits + i) != 1)
					{
						sprintf(OUTSTR,
								"ERROR: information on limits unreadable");
						goto errorExit;
					}
					place = skipStrings(place, 1);
				} /*for (i = 0; i < NLIMITS; i++)*/
			} /*if (Pre407Save){}else{}*/

			maxdims = limits[0];
			maxvars = limits[1];
			wordsperterm = limits[2];
			varsperword = limits[3];

			if (maxdims != MAXDIMS)
			{
				sprintf(OUTSTR,
						"ERROR: file was saved by incompatible version of MacAnova");
				goto errorExit;
			}
			else if (wordsperterm != WORDSPERTERM ||
					 varsperword != VARSPERWORD)
			{
				retvalue = badGlm;
			}
		}
		else if (strcmp(NAME(symh), "PLATFORM") == 0 && TYPE(symh) == CHAR &&
				 isScalar(symh))
		{
			char        *platform = STRINGPTR(symh);
			
			if (strcmp(platform,PLATFORM) != 0
#ifdef PLATFORM_ALT1
				&& strcmp(platform, PLATFORM_ALT1) != 0
#endif /*PLATFORM_ALT1*/
#ifdef PLATFORM_ALT2
				&& strcmp(platform, PLATFORM_ALT2) != 0
#endif /*PLATFORM_ALT2*/
#ifdef PLATFORM_ALT3
				&& strcmp(platform, PLATFORM_ALT3) != 0
#endif /*PLATFORM_ALT3*/
#ifdef PLATFORM_ALT4
				&& strcmp(platform, PLATFORM_ALT4) != 0
#endif /*PLATFORM_ALT4*/
#ifdef PLATFORM_ALT5
				&& strcmp(platform, PLATFORM_ALT5) != 0
#endif /*PLATFORM_ALT5*/
#ifdef PLATFORM_ALT6
				&& strcmp(platform, PLATFORM_ALT6) != 0
#endif /*PLATFORM_ALT6*/
#ifdef PLATFORM_ALT7
				&& strcmp(platform, PLATFORM_ALT7) != 0
#endif /*PLATFORM_ALT7*/
#ifdef PLATFORM_ALT8
				&& strcmp(platform, PLATFORM_ALT8) != 0
#endif /*PLATFORM_ALT8*/
				)
			{
				if (Binary)
				{
					sprintf(OUTSTR,
						"ERROR: Binary file saved by version of Macanova compiled for %s",
							platform);
					goto errorExit;
				}
				sprintf(OUTSTR,
					"WARNING: File saved by version of Macanova compiled for %s",
						platform);
				putErrorOUTSTR();
			} /*if (strcmp(platform,PLATFORM) != 0)*/
		}
	} /*for (icomp = 0;icomp < NCOMPS(globals);icomp++)*/
	return (retvalue);

  errorExit:
	putErrorOUTSTR();
	putOutMsg("       Restore aborted");

	return (badPlatform);
} /*checkMachine()*/
#endif /*PLATFORM*/

/* Modified from listObject in list.c */
static void listRestoredSymbol(Symbolhandle symh)
{
	char            *name;
	int              type;
	long             i, nclass;
	WHERE("listRestoredSymbol");

	name = NAME(symh);
	type = TYPE(symh);

	sprintf(OUTSTR,"Restoring %-15s %-6s",name,typeName(type));
	myprint(OUTSTR);

	if (type == REAL || type == LOGIC || type == CHAR || type == STRUC)
	{
		for (i = 1; i <= NDIMS(symh); i++)
		{
			sprintf(OUTSTR, " %-5ld", DIMVAL(symh,i));
			myprint(OUTSTR);
		}
		if ((nclass = NCLASS(symh)) > 0)
		{
			sprintf(OUTSTR, " FACTOR with %ld levels",nclass);
			myprint(OUTSTR);
		}
	}
	myeol();
	*OUTSTR = '\0';
} /*listRestoredSymbol()*/

/*
   Usage: restore(fileName [,delete:T] [,list:T])
	delete:T suppresses deletion of symbols that are not being replaced
	list:T lists symbols as they are restored.  Primarily for debugging
	purposes

	960305 Modified so that original contents of Symbol "HOME" are saved
	are restored so that restoring of a save file made on a different
	system or with a different home directory will not change HOME.

	960419 Modified so that default value of delete is taken from global
	DEFAULTRESTOREDEL which is settable by setoptions.

	960502 Changed fopen() to fmyopen()
	960503 Changed macOpen() to macFindFile()
	970409 Modified so that a restore of an ascii file will work whether the
           files line separators are CR, LF or CR/LF.  New function
           restoregetc() to handle reading.
	970506 Used TEXTREADMODE and BINARYREADMODE to symbolize modes for fmyopen()
	980623 Recognizes symbol HISTRY and resets the history
	980715 Uses ASCIIHEADER7 and BINARYHEADER7 by default
	980821 When in batch mode, restore() does not change current prompt
	990325 On Macintosh, 'ttro' is now an acceptable type for a restore file
*/

Symbolhandle    restore(Symbolhandle list)
{
	Symbolhandle    arg1, argKey, symh, next, symh1, home;
	char            instuff[HEADERLENGTH+1];
	long            i;
	long            nargs = NARGS(list);
	int             deleteCurrent = DEFAULTRESTOREDEL;
	int             forceDelete = 0;
	int             restoreHistory = 1;
	int             restoreGlobs = 1, listRestored = 0;
	int             fullSave;
	char           *keyword;
	char            whenSaved[TIMELENGTH];
	char           *fname, name[NAMELENGTH+1];
#ifdef MACINTOSH
	OSType          types[4];
#endif
	WHERE("restore");

	*OUTSTR = '\0';
	RESTOREFILE = (FILE *) 0;
	whenSaved[0] = '\0';

	if (Home != (char **) 0)
	{
		mydisphandle(Home);
		Home = (char **) 0;
	}
	
	arg1 = COMPVALUE(list, 0);

	if(nargs > 3)
	{
		badNargs(FUNCNAME,-3);
		goto errorExit;
	}

	if (!argOK(arg1, 0, 0))
	{
		goto errorExit;
	}
	if (!isCharOrString(arg1))
	{
		sprintf(OUTSTR,
				"ERROR: file name must be quoted string or CHARACTER scalar");
		goto errorExit;
	}
	for (i = 1; i < nargs; i++)
	{ /* must be either delete:(T|F) or simply T or F */
		long        logValue;
		
		argKey = COMPVALUE(list,i);
		if (!argOK(argKey, 0, i+1))
		{
			goto errorExit;
		}
		if ((keyword = isKeyword(argKey)))
		{
			if (strncmp(keyword, "del", 3) != 0 &&
#ifdef SAVEHISTORY
				strncmp(keyword, "his", 3) != 0 &&
#endif /*SAVEHISTORY*/
				strncmp(keyword, "lis", 3) != 0)
			{
				badKeyword(FUNCNAME, keyword);
				goto errorExit;
			}
		}
		if (!isTorF(argKey))
		{
			notTorF((keyword) ? keyword : (char *) 0);
			goto errorExit;
		}
		
		logValue = (DATAVALUE(argKey, 0) != 0.0);
		if (!keyword || keyword[0] == 'd')
		{
			forceDelete = deleteCurrent = logValue;
		}
		else if (keyword[0] == 'l')
		{
			listRestored = logValue;
		}
#ifdef SAVEHISTORY
		else if (keyword[0] == 'h')
		{
			restoreHistory = logValue;
		}
#endif /*SAVEHISTORY*/
	} /*for (i = 1; i < nargs; i++)*/

#ifdef SAVEHISTORY
	if (!(ISATTY & ITTYIN))
	{
		restoreHistory = 0;
	}
#endif /*SAVEHISTORY*/

	fname = STRINGPTR(arg1);

#ifdef HASFINDFILE
#ifdef MACINTOSH
	types[0] = KEEPFILETYPE;
	types[1] = ASCIIKEEPFILETYPE;
	types[2] = 'TEXT'; /* allow for type having been changed */
	types[3] = 'ttro';
	fname = macFindFile(fname, "\pSpecify the restorefile", (STR255) 0, READIT,
						4, types, &RestoreVolume);
#endif /*MACINTOSH*/

#ifdef WXWIN
	fname = wxFindFile(fname, "Specify the restorefile",  (char *) 0);
#endif /*WXWIN*/
	if (fname == (char *) 0)
	{
		sprintf(OUTSTR, "WARNING: %s() cancelled", FUNCNAME);
		goto errorExit;
	}
#endif /*HASFINDFILE*/

	fname = expandFilename(fname);
	if (fname == (char *) 0 || !isfilename(fname))
	{
		goto errorExit;
	}

	KEEPFILE = (FILE *) 0;
	mydisphandle(KEEPFILENAME);
	
	/* save the restore file name as save file name */
	if ((KEEPFILENAME = mygethandle(strlen(fname)+1)) != (char **) 0)
	{
		strcpy(*KEEPFILENAME, fname);
#ifdef MACINTOSH
		KeepVolume = RestoreVolume;
#endif /*MACINTOSH*/
	}

	RESTOREFILE = fmyopen(fname, BINARYREADMODE);
	LastRestoreWasCr = 0;

	if (RESTOREFILE == (FILE *) 0 )
	{
		sprintf(OUTSTR,"ERROR: cannot open file %s for restore", fname);
		goto errorExit;
	}

	PadChar = ' ';
	if (!restoreNString(HEADERLENGTH, instuff))
	{
		sprintf(OUTSTR,
				"ERROR: Does not appear to be save file;header = %s",instuff);
		goto errorExit;
	}

	if (strcmp(instuff, BINARYHEADER1) == 0)
	{
		ASCII = SAVEBINARY1;
	}
	else if (strcmp(instuff, BINARYHEADER4) == 0)
	{
		ASCII = SAVEBINARY4;
	}
	else if (strcmp(instuff, BINARYHEADER5) == 0)
	{
		ASCII = SAVEBINARY5;
	}
	else if (strcmp(instuff, BINARYHEADER6) == 0)
	{
		ASCII = SAVEBINARY6;
	}
	else if (strcmp(instuff, BINARYHEADER7) == 0)
	{
		ASCII = SAVEBINARY7;
	}
	else if (strcmp(instuff, ASCIIHEADER1) == 0)
	{
		ASCII = SAVEASCII1;
	}
	else if (strcmp(instuff, ASCIIHEADER2) == 0)
	{
		ASCII = SAVEASCII2;
	}
	else if (strcmp(instuff, ASCIIHEADER3) == 0)
	{
		ASCII = SAVEASCII3;
	}
	else if (strcmp(instuff, ASCIIHEADER4) == 0)
	{
		ASCII = SAVEASCII4;
	}
	else if (strcmp(instuff, ASCIIHEADER5) == 0)
	{
		ASCII = SAVEASCII5;
	}
	else if (strcmp(instuff, ASCIIHEADER6) == 0)
	{
		ASCII = SAVEASCII6;
	}
	else if (strcmp(instuff, ASCIIHEADER7) == 0)
	{
		ASCII = SAVEASCII7;
	}
	else if (strncmp(instuff, ASCIIHEADER4, HEADERLENGTH-3) == 0 ||
			 strncmp(instuff, BINARYHEADER4, HEADERLENGTH-4) == 0)
	{
		sprintf(OUTSTR,
				"ERROR: MacAnova save file cannot be restored by this version of MacAnova");
		goto errorExit;
	}
	else
	{
		sprintf(OUTSTR,"ERROR: file is not a MacAnova save file");
		goto errorExit;
	}

	if (!Binary && ASCII != SAVEASCII1)
	{
		/* close file and reopen in text mode */
		fclose(RESTOREFILE);
		RESTOREFILE = fmyopen(fname, TEXTREADMODE);
		LastRestoreWasCr = 0;
		/* skip identifying header */
		if(!restoreNString(HEADERLENGTH,instuff))
		{
			goto errorExit;
		}
		PadChar = '\n';
	} /*if (!Binary && ASCII != SAVEASCII1)*/

	if (!Pre407Save)
	{
		if (!restoreName(instuff))
		{
			goto errorExit;
		}
		if (strcmp(instuff, FULLSAVE) == 0)
		{
			fullSave = 1;
		}
		else if (strcmp(instuff, PARTIALSAVE) == 0)
		{
			fullSave = 0;
			deleteCurrent = forceDelete;
		}
		else
		{
			goto incompleteRestore;
		}
	}
	else
	{
		fullSave = 1;
	}
	
	sprintf(OUTSTR, "Restoring %sworkspace from file ",
			(fullSave) ? NullString : "partial ");
	myprint(OUTSTR);
	putOutMsg(fname);
	*OUTSTR = '\0';

	if (ASCII == SAVEASCII1)
	{
		sprintf(OUTSTR,
				"WARNING: MacAnova 2.4 or earlier asciisave file ; may include old style macros");
		putErrorOUTSTR();
	}

	/* delete current workspace */
	home = Lookup("HOME");
	if (isCharOrString(home))
	{ /* save copy of contents of symbol HOME */
		Home = myduphandle(STRING(home));
		if (Home == (char **) 0)
		{
			goto errorExit;
		}
	} /*if (isCharOrString(home))*/
	
	if (deleteCurrent)
	{
		for (i = 0;i < nargs;i++)
		{/* first remove items in the argument list */
			Removesymbol(COMPVALUE(list,i));
			COMPVALUE(list,i) = (Symbolhandle) 0;
		}

		symh = Firstsymbol(0);
		while (symh != (Symbolhandle) 0)
		{ /* clear regular symbol table*/
			next = Nextsymbol(symh,0);
			Removesymbol(symh); /* will not remove builtins */
			symh = next;
		}
		symh = Firstsymbol(1);
		while (symh != (Symbolhandle) 0)
		{ /* clear scratch, UNDEF, and keyword symbol table */
			if(symh == list)
			{/* do not remove list; that is done in mainpars.y */
				symh = Nextsymbol(symh,1);
			}
			else
			{
				next = Nextsymbol(symh,1);
				Removesymbol(symh); /* will not remove builtins */
				symh = next;
			}
		}
		clearGlobals();
	} /*if (deleteCurrent)*/

/* Make sure correct info is in VERSION */
	if (VERSION_ID != (char **) 0)
	{
		Remove("VERSION");
		symh = Install("VERSION", CHAR);
		TMPHANDLE = myduphandle(VERSION_ID);
		if(symh == (Symbolhandle) 0 || TMPHANDLE == (char **) 0)
		{
			goto errorExit;
		}
		setSTRING(symh, TMPHANDLE);
		setNDIMS(symh,1);
		setDIM(symh,1,1);
	}  /*if (deleteCurrent){}else{}*/

	while (1)
	{
		int            ch;

		while ((ch = restoregetc(RESTOREFILE)) == ' ' || ch == PadChar)
		{
			;
		}
		if (ch == EOF)
		{
			break;
		}
		ungetc((char) ch, RESTOREFILE);

		symh = restoresym(); /* is already in Symbol table */
		if (symh == (Symbolhandle) 0)
		{
			fclose(RESTOREFILE);
			goto incompleteRestore;
		}

		strcpy(name,NAME(symh));
		if(strcmp(name,"T") == 0 || strcmp(name,"F") == 0 ||
		   strcmp(name,"HELP") == 0 || strcmp(name,"help") == 0 ||
		   strcmp(name,"VERSION") == 0)
		{ /* all but the last might be saved by MacAnova2.4 */
			Removesymbol(symh);
		}
		else if (strcmp(name,OPTIONS) == 0)
		{
			promptType       currentPrompt;
#if (0) /* no longer any comment on unrecognized options*/
			long             nOptions = NCOMPS(symh);
			
			for (i=0;i<nOptions;i++)
			{
				keyword = NAME(COMPVALUE(symh,i));
				if(!isOption(keyword))
				{
					break;
				}
			}
			if (i < nOptions)
			{
				sprintf(OUTSTR,
						"WARNING: unrecognized option name %s found during restore",
						keyword);
				putErrorOUTSTR();
			}
#endif /*0*/
			if (listRestored)
			{
				putOutMsg("Restoring option values");
			}

			strcpy(currentPrompt, PROMPT);

			/* arg2 == 1 means don't complain about unrecognized options*/
			setOpt(symh, 1);

			/* if reading batch file, don't change the current prompt */
			if (BDEPTH > 0)
			{
				strcpy(PROMPT, currentPrompt);
			}
			Removesymbol(symh);
		}
#ifdef PLATFORM
		else if(strcmp(name,MACHINE) == 0)
		{
			int       platformStatus = checkMachine(symh);

			Removesymbol(symh);
			if (platformStatus == badPlatform)
			{
				goto errorExit;
			}
			if (platformStatus == badGlm)
			{
				restoreGlobs = 0;
			}
		}
#endif /*PLATFORM*/
		else if(strcmp(name, GLOBALS) == 0)
		{
			if(restoreGlobs && !deleteCurrent)
			{
				clearGlobals();
			}

			if(restoreGlobs)
			{
				restoreGlobals(symh);
				if (listRestored)
				{
					putOutMsg("Restoring private glm values");
				}

				if(NVARS < 0)
				{
					clearGlobals();
				}
			}
			else if(PRINTWARNINGS)
			{
				putOutErrorMsg("WARNING: incompatible internal representation of model on restore file");
				putOutMsg("         Model information not restored");
			}
			Removesymbol(symh);
		}
		else if(strcmp(name, SAVETIME) == 0)
		{
			strncpy(whenSaved,STRINGPTR(symh),TIMELENGTH-1);
			whenSaved[TIMELENGTH-1] = '\0';
			Removesymbol(symh);
		}
#ifdef SAVEHISTORY
		else if (strcmp(name, HISTRY) == 0)
		{
			if (restoreHistory)
			{
				if (listRestored)
				{
					putOutMsg("Restoring history of commands");
				}
				setSomeHistory(DIMVAL(symh, 1), STRING(symh));
			}
			Removesymbol(symh);
		}
#endif /*SAVEHISTORY*/
		else
		{ /* ordinary symbol */
			if(!deleteCurrent)
			{ /* delete symbol if it exists */
				Cutsymbol(symh); /* make sure it won't be found by Lookup */
				symh1 = Lookup(name);
				Addsymbol(symh); /* put back in symbol table */
				Removesymbol(symh1);
			}
			/* Make sure HOME doesn't get changed */
			if (Home != (char **) 0 && strcmp(name, "HOME") == 0 &&
				isCharOrString(symh))
			{
				mydisphandle(STRING(symh));
				setSTRING(symh, Home);
				Home = (char **) 0;
			}			

			if (listRestored)
			{
				listRestoredSymbol(symh);
			}
		}
	} /* while (1) */
	fclose(RESTOREFILE);
	if (*whenSaved)
	{
		sprintf(OUTSTR, "Workspace saved %s", whenSaved);
		putOUTSTR();
	}

	if (Home != (char **) 0 && Lookup("HOME") == (Symbolhandle) 0)
	{
		symh = CInstall("HOME", 0);
		if (symh != (Symbolhandle) 0)
		{
			setSTRING(symh, Home);
			setNDIMS(symh, 1);
			setDIM(symh, 1, 1);
			Home = (char **) 0;
		}
	} /*if (Home != (char **) 0 && Lookup("HOME") == (Symbolhandle) 0)*/
	
	mydisphandle(Home);
	Home = (char **) 0;
#ifdef MACINTOSH
	UNLOADSEG(restoreGraph);
#endif /*MACINTOSH*/
	return (NULLSYMBOL);

  incompleteRestore:
	strcpy(OUTSTR, "ERROR: incomplete restore");

  errorExit:
	putErrorOUTSTR();
	mydisphandle(Home);
	Home = (char **) 0;
#ifdef MACINTOSH
	UNLOADSEG(restoreGraph);
#endif /*MACINTOSH*/
	
	return (0);

} /*restore()*/


/*
  scans for integer followed by a PadChar; if the PadChar is not present, it is
  an error (return 0)
*/

long restoreLong(long * longValue)
{
	long          c;
	long          sign = 1, value = 0;

/*
	while((c = restoregetc(RESTOREFILE)) == ' ' || c == PadChar)
	{
		;
	}
*/
	c = restoregetc(RESTOREFILE);

	if(c == EOF || (!isdigit(c) && c != '-'))
	{
		return (0);
	}
	if(c == '-')
	{
		sign = -1;
	}
	else
	{
		value = c - '0';
	}
	while(isdigit((char) (c = restoregetc(RESTOREFILE))))
	{
		value = 10*value + (c - '0');
	}
	*longValue = sign*value;
	return (c == PadChar);
} /*restoreLong()*/

long restoreName(char * name)
{
	int         i, c;

/*
	while((c = restoregetc(RESTOREFILE)) == ' ' || c == PadChar)
	{
		;
	}
	if(c == EOF)
	{
		return (0);
	}
	*name++ = c;
*/
	for(i=0;i<NAMELENGTH;i++)
	{
		if((c = restoregetc(RESTOREFILE)) == EOF)
		{
			return (0);
		}

		if(c == PadChar)
		{
			break;
		}
		*name++ = (char) c;
	} /* for i */
	*name++ = '\0';
	return (i < NAMELENGTH || restoregetc(RESTOREFILE) == PadChar);
} /*restoreName()*/

/*
  restoreString() should be used only when one is sure the string to be
  read will fit in string[].  Otherwise, use restoreNString()
*/
long restoreString(char * string)
{
	long        length;

	*string = '\0';
	if(!restoreLong(&length) || !restoreNString(length,string))
	{
		return (0);
	}

	return (1);

} /*restoreString()*/

long restoreNString(long length, char * string)
{
	int            ch, nc, s;

	while(length-- > 0)
	{
		if((ch = restoregetc(RESTOREFILE)) == EOF)
		{
			return (0);
		}
		if(ASCII > SAVEASCII3 && ch == '\\')
		{
			if((ch = restoregetc(RESTOREFILE)) == EOF)
			{
				return (0);
			}
			if(ch == 't')
			{
				ch = '\t';
			}
			else if(isdigit(ch) && ch <= '3')
			{
				s = ch - '0';
				for(nc = 1; nc < 3; nc++)
				{
					if((ch = restoregetc(RESTOREFILE)) == EOF ||
					   !isdigit(ch) || ch > '7')
					{
						goto errorExit;
					}
					s = 8*s + (ch - '0');
				}
				ch = s;
			}
		} /*if(ASCII > SAVEASCII3 && ch == '\\')*/
		*string++ = ch;
	} /*while(length-- > 0)*/
	*string = '\0';

	return (restoregetc(RESTOREFILE) == PadChar);

  errorExit:
	return (0);
} /*restoreNString()*/

long restoreDouble(double * value) 
{
	int         c, length = 0;
	char       *end;
	char        buffer[100];
	WHERE("restoreDouble");

	while((c = restoregetc(RESTOREFILE)) == ' ' || c == PadChar)
	{
		;
	}
	if(c == EOF)
	{
		return (0);
	}
	buffer[0] = c;

	length = 1;
	while((c = restoregetc(RESTOREFILE)) != EOF && c != PadChar)
	{
		buffer[length++] = c;
	}

	if(c == EOF)
	{
		return (0);
	}
	buffer[length] = '\0';
	if (ASCII >= SAVEASCII5 && buffer[0] == '?')
	{
		setMissing(*value);
		end = buffer + length;
	}
	else
	{
		*value = mystrtod(buffer, &end);
		if (ASCII < SAVEASCII5 && *value == OLDMISSING)
		{
			setMissing(*value);
		}
	}

	return (end - buffer == length);
} /*restoreDouble()*/

/*
   skipRestoreVector() skips vector of indicated type.
*/
long skipRestoreVector(long itemType, long itemLength)
{
	double         itemDvalue;
	long           j, itemLvalue;
	char           itemCvalue[MAXCHARITEMSIZE + 1];

	for (j = 0; j < itemLength; j++)
	{
		if (itemType & SAVEREAL)
		{
			if (!restoreDouble(&itemDvalue))
			{
				return (0);
			}
		}
		else if (itemType & SAVELONG)
		{
			if (!restoreLong(&itemLvalue))
			{
				restore (0);
			}
		}
		else if (itemType & SAVECHAR)
		{
			if (!restoreString(itemCvalue) ||
				strlen(itemCvalue) > MAXCHARITEMSIZE)
			{
				return (0);
			}
		}
		else
		{
			return (0);
		}
	} /*for (j = 0; j < itemLength; j++)*/
	return (1);
} /*skipRestoreVector()*/

/*
  Get one item in ascii form.
  If the item is a scalar (itemType & SAVESCALAR != 0), its value is
  returned in itemDvalue, itemLvalue or itemCvalue.  It the item is a vector
  (itemType & SAVEVECTOR != 0), its length is returned in itemLength
  but no values are returned.  This can be called after reading a
  named restore item.
*/
int getRestoreItem(long itemType, long * itemLength, 
				   double * itemDvalue, long * itemLvalue, char * itemCvalue)
{
	WHERE("getRestoreItem");
	
	if (itemType & SAVEVECTOR)
	{				
		if (!restoreLong(itemLength) && *itemLength < 1)
		{
			return (0);
		}
	}
	else if (itemType & SAVESCALAR)
	{
		if (itemType & SAVEREAL)
		{
			if (!restoreDouble(itemDvalue))
			{
				return (0);
			}
		}
		else if (itemType & SAVELONG)
		{
			if (!restoreLong(itemLvalue))
			{
				return (0);
			}
		}
		else if (itemType & SAVECHAR)
		{
			if (!restoreString(itemCvalue) ||
				strlen(itemCvalue) > MAXCHARITEMSIZE)
			{
				return (0);
			}
		}
		else
		{
			return (0);
		}
	} /*else if (itemType & SAVESCALAR)*/
	else if (!(itemType & SAVESYMBOL))
	{
		return (0);
	}
	return (1);
} /*getRestoreItem()*/

int getNextRestoreItem(char prefix [], int * itemNumber, int * itemType,
					   long * itemLength, double * itemDvalue,
					   long * itemLvalue, char * itemCvalue)
{
	char          itemName[MAXCHARITEMSIZE + 1];
	char          thisPrefix[3];
	WHERE("getNextRestoreItem");

	if (!restoreName(itemName) ||
		!decodeItem(itemName, thisPrefix, itemNumber, itemType) ||
		strcmp(prefix, thisPrefix) != 0)
	{
		return (0);
	}

	return (*itemNumber == EndItems ||
			getRestoreItem(*itemType, itemLength, itemDvalue,
						   itemLvalue, itemCvalue));

} /*getNextRestoreItem()*/
