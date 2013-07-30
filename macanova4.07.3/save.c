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

/*
  File save.c contains most functions for saving symbols.  Functions
  for saving PLOTINFO symbols are in plotutil.c

  980715 added saveSymbolInfo(), modified savesym() and rearranged
         public functions
  980718 changed save format of symbol header information
  980724 save either FULLSAVE or PARTIALSAVE after initial marker
  990212 Changed putOUTSTR() to putErrorOUTSTR() and substituted
         putOutErrorMsg() and putOutMsg() for myerrorout()
*/
#define SAVE__

#include "globals.h"
#include "mvsave.h"

#ifdef MACINTOSH
#include "macIface.h"
#ifdef MPW
#else
#include <unix.h>
#endif /*MPW*/
#endif /* MACINTOSH*/

#ifdef WXWIN
#include "wx/wxIface.h"
#endif /*WXWIN*/

#define MAXCHUNK  16000

/* Globals ASCII, KEEPFILE, and KEEPFILENAME are declared in mvsave.h */

/*
  Values for ASCII, defining save operations; <=0 are binary, >0 are ascii
  SAVEBINARY1 & SAVEASCII1 refer to original binary format, with MAXVAR==31
  SAVEBINARY2 & SAVEASCII2 refer to first modified format, with MAXVAR == 31
  SAVEBINARY3 & SAVEASCII3 refer to modified format allowing MAXVAR == 96
  SAVEBINARY4 & SAVEASCII4 same, except nonprintables escaped for asciisave
  SAVEBINARY5 & SAVEASCII5 same, except NULLs save, new MISSING value
  SAVEBINARY6 & SAVEASCII6 same, except symbol labels are saved
  SAVEBINARY7 & SAVEASCII7 Format for symbol header modified and completely
                           new format for PLOTINFO symbols

  Type 3 and later not only allows saving Global information related to 96
  variable models, but also save only the necessary dimension information,
  rather than MAXDIMS every time.

  The original ascii save format had long lines and imbedded '\0's terminating
  strings and macros.  Later formats have short lines and have character counts
  for each string and macro.  The only non-standard embedded characters would
  be in strings defined by escape sequences such as "\1".

  They are defined in mvsave.h
*/

#define DEFAULTSAVEOPTIONS  1
#define DEFAULTALL          0
		
#define NGLOBALS 37
#define NMACHINE 3 /* number of components for Globals */

#define STRINGFORMISSING  "??"

static char            StringForMissing[20];
static char           *CharFmt;
static char           *LongFmt;
static char           *DoubleFmt;

static Symbolhandle    Globals;

static Symbolhandle myInstall(char **globalHandle, size_t size)
{
	Symbolhandle   symh;
	char         **handle;
	WHERE("myInstall");
	
	if ((symh = Install(SCRATCH,REAL)) == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	if ((handle = myduphandle(globalHandle)) == (char **) 0)
	{
		goto errorExit;
	}
	setDATA(symh, (double **) handle);
	setNDIMS(symh, 1);
	/* the calling function should reset DIM 1 for type char */
	/* handle should be OK if we got here; no need to check myhandlelength() */
	setDIM(symh, 1, myhandlelength(handle) / size);
	
	return (symh);

  errorExit:
	Removesymbol(symh);
	return(0);
} /*myInstall()*/

static int       double2Symbol(double *dPointer,char *name,long length,
								long icomp)
{
	Symbolhandle   symh;
	long           i;
	
	if ((COMPVALUE(Globals,icomp) = Makereal(length)) == (Symbolhandle) 0)
	{
		return (0);
	}
	symh = COMPVALUE(Globals,icomp);
	
	setNAME(symh,name);
	for (i=0;i<length;i++)
	{
		DATAVALUE(symh,i) = dPointer[i];
	}
	return (1);
	
} /*double2Symbol()*/

static int       long2Symbol(long *lPointer,char *name,long length,
								long icomp)
{
	Symbolhandle   symh;
	long           i;
	
	if ((COMPVALUE(Globals,icomp) = Makelong(length)) == (Symbolhandle) 0)
	{
		return (0);
	}
	symh = COMPVALUE(Globals,icomp);
	
	setNAME(symh,name);
	for (i=0;i<length;i++)
	{
		LONGDATAVALUE(symh,i) = lPointer[i];
	}
	return (1);
} /*double2Symbol()*/

/* bit definitions used in previous versions */
#define BALANOVA     0x002L  /* 2 balanced anova */
#define UNBALANO     0x010L  /* 16 unbalanced anova */
#define OLDOLSREG    0x020L  /* 32 standard regression */
#define OLDROBUSTREG 0x040L  /* 64 robust regression */

static Symbolhandle bundleGlobals(void)
{
	Symbolhandle       symh,symh1;
	long               i, j, k, n, nGlobals = 0;
	long               nvars = (long) NVARS, mvars;
	long               nterms = (long) NTERMS;
	long               ltemp;
	double             dTemp;
	char             **cHandle, name[NAMELENGTH+1];
	double             nclasses[MAXVARS];
	long               maxvars = MAXVARS;
	long               newstyle = (ASCII>=SAVEASCII3 || ASCII<=SAVEBINARY3);
	long               neweststyle = !Pre336Save;
	WHERE("bundleGlobals");
	
	if ((Globals = StrucInstall(GLOBALS,NGLOBALS)) == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	
	mvars = (newstyle) ? nvars : OLDMAXVARS;

	if (!newstyle && nvars > OLDMAXVARS)
	{
		sprintf(OUTSTR,
				"WARNING: more than %ld model variables; cannot save in v2.4 or v3.1 format",
				(long) OLDMAXVARS);
		putErrorOUTSTR();
		nvars = -1;
	} /*if (!newstyle && nvars > OLDMAXVARS)*/
	
	if (nvars < 0)
	{ /* no globals; only save NVARS */
		double      nvars1 = -1.0;
		if (!double2Symbol(&nvars1,"NVARS",1,nGlobals++))
		{
			goto errorExit;
		}
	} /*if (nvars < 0)*/
	else
	{
		for (i=0;i<mvars;i++)
		{
			nclasses[i] = (double) NCLASSES[i];
		}
		
		if (!double2Symbol(&NVARS,"NVARS",1,nGlobals++))
		{
			goto errorExit;
		}
		if (newstyle && !long2Symbol(&maxvars,"MAXVARS",1,nGlobals++))
		{
			goto errorExit;
		}
		if (newstyle && !double2Symbol(&NFACTORS,"NFACTORS",1,nGlobals++))
		{
			goto errorExit;
		}
		if (!double2Symbol(&NDATA,"NDATA",1,nGlobals++))
		{
			goto errorExit;
		}
		if (!double2Symbol(&NOTMISSING,"NOTMISSING",1, nGlobals++))
		{
			goto errorExit;
		}
		if (!double2Symbol(&NTERMS,"NTERMS",1,nGlobals++))
		{
			goto errorExit;
		}
		if (!double2Symbol(&NREGX,"NREGX",1,nGlobals++))
		{
			goto errorExit;
		}
		if (!double2Symbol(&NY,"NY",1,nGlobals++))
		{
			goto errorExit;
		}
		if (!double2Symbol(&NERRORTERMS,"NERRORTERMS",1,nGlobals++))
		{
			goto errorExit;
		}
		if (neweststyle &&
		   !double2Symbol(&PREVGLMSCALE,"PREVGLMSCALE",1,nGlobals++))
		{
			goto errorExit;
		}
		ltemp = PREVMODELTYPE;
		if (!neweststyle)
		{
			if (ltemp & ANOVA)
			{
				ltemp &= ~ANOVA;
				ltemp |= (PREVGLMCONTROL & UNBALANCED) ?
					UNBALANO : BALANOVA;
			}
			else if (ltemp & OLSREG)
			{
				ltemp &= ~OLSREG;
				ltemp |= OLDOLSREG;
			}
			/* we never save robust() info for older versions */
		} /*if (!neweststyle)*/
		dTemp = (double) ltemp;
			
		/* "PREVMODELTYPE" truncated to 12 = NAMELENGTH characters */
		if (!double2Symbol(&dTemp,"PREVMODELTYP",1,nGlobals++))
		{
			goto errorExit;
		}
		if (neweststyle)
		{
			dTemp = (double) PREVGLMCONTROL;
			/* "PREVGLMCONTROL" truncated to 12 = NAMELENGTH characters */
			if (!double2Symbol(&dTemp,"PREVGLMCONTR",1,nGlobals++))
			{
				goto errorExit;
			}
		} /*if (neweststyle)*/
		
		dTemp = (double) INCREMENTAL;
		if (!double2Symbol(&dTemp,"INCREMENTAL",1,nGlobals++))
		{
			goto errorExit;
		}
		dTemp = (double) USEGLMOFFSET;
		if (!double2Symbol(&dTemp,"USEGLMOFFSET",1,nGlobals++))
		{
			goto errorExit;
		}
		if (mvars > 0 && !double2Symbol(nclasses,"NCLASSES",mvars, nGlobals++))
		{
			goto errorExit;
		}
		if (newstyle)
		{
			if (!long2Symbol((long *) MODELINFO->errorterms,"ERRORTERMS1",
							WORDSPERTERM*(MAXERRTRMS+1), nGlobals))
			{
				goto errorExit;
			}
			/* "ERRORTERMS1" indicates new style of ERRORTERMS */
			symh = COMPVALUE(Globals,nGlobals++);
			/* make it a WORDSPERTERM by (MAXERRTRMS+1) matrix */
			setNDIMS(symh,2);
			setDIM(symh,1,WORDSPERTERM);
			setDIM(symh,2,MAXERRTRMS+1);
		}
		else
		{
			double    *errorterms;
			
			symh = COMPVALUE(Globals,nGlobals++) = Makereal(MAXERRTRMS+1);
			if (symh == (Symbolhandle) 0)
			{
				goto errorExit;
			}

			errorterms = DATAPTR(symh);
			for (i=0;i<=MAXERRTRMS;i++)
			{
				*errorterms++ = (double) MODELINFO->errorterms[i][0];
			}
			setNAME(symh,"ERRORTERMS");
			/* "ERRORTERMS" indicates old style of ERRORTERMS */
		}                       
		
		symh = COMPVALUE(Globals,nGlobals++) = Makesymbol(CHAR);
		if (symh == (Symbolhandle) 0)
		{
			goto errorExit;
		}

		if ((cHandle = mygethandle((nvars+1)*(NAMELENGTH+1))) == (char **) 0)
		{
			goto errorExit;
		}
	
		setSTRING(symh,cHandle);
		setNAME(symh,"VARNAMES");
		setNDIMS(symh,1);
		setDIM(symh,1,nvars+1);
		k = 0;
		for (j=0;j<=nvars;j++)
		{
			strcpy((*cHandle) + k, VARNAMES[j]);
			k += strlen(VARNAMES[j]) + 1;
		}
		/* shrink cHandle to proper size */
		cHandle = mygrowhandle(cHandle,k);
		setSTRING(symh,cHandle);
		if (cHandle == (char **) 0)
		{
			goto errorExit;
		}

		/* now save handle stuff */
		if (MODEL != (modelHandle) 0)
		{
			if (newstyle)
			{
				if ((symh = myInstall((char **) MODEL, sizeof(termWord))) ==
				   (Symbolhandle) 0)
				{
					goto errorExit;
				}
				Cutsymbol(symh);
				COMPVALUE(Globals,nGlobals++) = symh;
				setNAME(symh,"MODEL1"); /* MODEL1 indicates new style of MODEL */
				setTYPE(symh,LONG);
				/* make it a matrix with 1 column per term */
				setNDIMS(symh,2);
				setDIM(symh,2,DIMVAL(symh,1)/WORDSPERTERM);
				setDIM(symh,1,WORDSPERTERM);
			}/*if (newstyle)*/
			else
			{
				double        *model;
				
				symh = COMPVALUE(Globals,nGlobals++) = Makereal(nterms);
				if (symh == (Symbolhandle) 0)
				{
					goto errorExit;
				}

				model = DATAPTR(symh);
				
				for (i=0;i<nterms;i++)
				{
					*model++ = (double) (*MODEL)[i][0];
				}
				setNAME(symh,"MODEL");/*"MODEL" indicates old style of MODEL*/
			}
		}
		/* 
		  Only save X and Y when saving with v31:T. It is no longer necessary
		  since now Y is always identical to DATA(MODELVARS[0]) and X[i]
		  is now always identical to DATA(MODELVARS[i+1]).  The only place
		  where this was in question was in funbalanova, where Y (and X[i])
		  were lengthened.  That code there now resets MODELVARS
			*/
		if (!newstyle && Y != (double **) 0)
		{
			if ((symh = myInstall((char **) Y, sizeof(double))) ==
			   (Symbolhandle) 0)
			{
				goto errorExit;
			}
			Cutsymbol(symh);
			COMPVALUE(Globals,nGlobals++) = symh;
			setNAME(symh,"Y");
		}

		if (RESIDUALS != (double **) 0)
		{
			if ((symh = myInstall((char **) RESIDUALS, sizeof(double))) ==
			   (Symbolhandle) 0)
			{
				goto errorExit;
			}
			Cutsymbol(symh);
			COMPVALUE(Globals,nGlobals++) = symh;
			setNAME(symh,"RESIDUALS");
		}

		if (WTDRESIDUALS != (double **) 0)
		{
			if ((symh = myInstall((char **) WTDRESIDUALS, sizeof(double))) ==
			   (Symbolhandle) 0)
			{
				goto errorExit;
			}
			Cutsymbol(symh);
			COMPVALUE(Globals,nGlobals++) = symh;
			setNAME(symh,"WTDRESIDUALS");
		}

		if (GLMOFFSET != (double **) 0)
		{
			if ((symh = myInstall((char **) GLMOFFSET, sizeof(double))) ==
			   (Symbolhandle) 0)
			{
				goto errorExit;
			}
			Cutsymbol(symh);
			COMPVALUE(Globals,nGlobals++) = symh;
			setNAME(symh,"GLMOFFSET");
		}

		if (HII != (double **) 0)
		{
			if ((symh = myInstall((char **) HII, sizeof(double))) ==
			   (Symbolhandle) 0)
			{
				goto errorExit;
			}
			Cutsymbol(symh);
			COMPVALUE(Globals,nGlobals++) = symh;
			setNAME(symh,"HII");
		}

		if (SS != (double **) 0)
		{
			if ((symh = myInstall((char **) SS, sizeof(double))) ==
			   (Symbolhandle) 0)
			{
				goto errorExit;
			}
			Cutsymbol(symh);
			COMPVALUE(Globals,nGlobals++) = symh;
			setNAME(symh,"SS");
		}
		if (DF != (double **) 0)
		{
			if ((symh = myInstall((char **) DF, sizeof(double))) ==
			   (Symbolhandle) 0)
			{
				goto errorExit;
			}
			Cutsymbol(symh);
			COMPVALUE(Globals,nGlobals++) = symh;
			setNAME(symh,"DF");
		}

		if (MISSWTS != (double **) 0)
		{
			if ((symh = myInstall((char **) MISSWTS, sizeof(double))) ==
			   (Symbolhandle) 0)
			{
				goto errorExit;
			}
			Cutsymbol(symh);
			COMPVALUE(Globals,nGlobals++) = symh;
			setNAME(symh,"MISSWTS");
		}

		if (CASEWTS != (double **) 0)
		{
			if ((symh = myInstall((char **) CASEWTS, sizeof(double))) ==
			   (Symbolhandle) 0)
			{
				goto errorExit;
			}
			Cutsymbol(symh);
			COMPVALUE(Globals,nGlobals++) = symh;
			setNAME(symh,"CASEWTS");
		}

		if (ITERWTS != (double **) 0)
		{
			if ((symh = myInstall((char **) ITERWTS, sizeof(double))) ==
			   (Symbolhandle) 0)
			{
				goto errorExit;
			}
			Cutsymbol(symh);
			COMPVALUE(Globals,nGlobals++) = symh;
			setNAME(symh,"ITERWTS");
		}

		if (ALLWTS != (double **) 0)
		{
			if ((symh = myInstall((char **) ALLWTS, sizeof(double))) ==
			   (Symbolhandle) 0)
			{
				goto errorExit;
			}
			Cutsymbol(symh);
			COMPVALUE(Globals,nGlobals++) = symh;
			setNAME(symh,"ALLWTS");
		}

		if (LOGITN != (double **) 0)
		{
			if ((symh = myInstall((char **) LOGITN, sizeof(double))) ==
			   (Symbolhandle) 0)
			{
				goto errorExit;
			}
			Cutsymbol(symh);
			COMPVALUE(Globals,nGlobals++) = symh;
			setNAME(symh,"LOGITN");
		}

		if (STRMODEL != (char **) 0)
		{
			if ((symh = myInstall(STRMODEL, sizeof(char))) ==
			   (Symbolhandle) 0)
			{
				goto errorExit;
			}
			setTYPE(symh,CHAR);
			setDIM(symh,1,1);
			Cutsymbol(symh);
			COMPVALUE(Globals,nGlobals++) = symh;
			setNAME(symh,"STRMODEL");
		}

		if (DEPVNAME != (char **) 0)
		{
			if ((symh = myInstall( DEPVNAME, sizeof(char))) ==
			   (Symbolhandle) 0)
			{
				goto errorExit;
			}
			setTYPE(symh,CHAR);
			setDIM(symh,1,1);
			Cutsymbol(symh);
			COMPVALUE(Globals,nGlobals++) = symh;
			setNAME(symh,"DEPVNAME");
		}

		if (TERMNAMES != (char **) 0)
		{
			if ((symh = myInstall( TERMNAMES, sizeof(char))) ==
			   (Symbolhandle) 0)
			{
				goto errorExit;
			}
			setTYPE(symh,CHAR);
			setDIM(symh,1,(long) NTERMS + 1);
			Cutsymbol(symh);
			COMPVALUE(Globals,nGlobals++) = symh;
			setNAME(symh,"TERMNAMES");
		}

		if (REGX != (double **) 0)
		{
			if ((symh = myInstall((char **)REGX, sizeof(double))) ==
			   (Symbolhandle) 0)
			{
				goto errorExit;
			}
			Cutsymbol(symh);
			COMPVALUE(Globals,nGlobals++) = symh;
			setNAME(symh,"REGX");
		}

		/* Note REGX2 is not saved, since it is used only for scratch */
		if (XTXINV != (double **) 0)
		{
			if ((symh = myInstall((char **)XTXINV, sizeof(double))) ==
			   (Symbolhandle) 0)
			{
				goto errorExit;
			}
			Cutsymbol(symh);
			COMPVALUE(Globals,nGlobals++) = symh;
			setNAME(symh,"XTXINV");
		}

		if (REGCOEF != (double **) 0)
		{
			if ((symh = myInstall((char **)REGCOEF, sizeof(double))) ==
			   (Symbolhandle) 0)
			{
				goto errorExit;
			}
			Cutsymbol(symh);
			COMPVALUE(Globals,nGlobals++) = symh;
			setNAME(symh,"REGCOEF");
		}

		if (REGSS != (double **) 0)
		{
			if ((symh = myInstall((char **)REGSS, sizeof(double))) ==
			   (Symbolhandle) 0)
			{
				goto errorExit;
			}
			Cutsymbol(symh);
			COMPVALUE(Globals,nGlobals++) = symh;
			setNAME(symh,"REGSS");
		}

		/* 
		  Only save X and Y when saving with v31:T. It is no longer necessary
		  since now Y is always identical to DATA(MODELVARS[0]) and X[i]
		  is now always identical to DATA(MODELVARS[i+1]).  The only place
		  where this was in question was in funbalanova, where Y (and X[i])
		  were lengthened.  That code there now resets MODELVARS
		*/
		if (!newstyle && X[0] != (double **) 0)
		{
			symh = COMPVALUE(Globals,nGlobals++) = Makestruc(MAXVARS);
			if (symh == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			setNAME(symh,"X");
			for (i=0;i<MAXVARS;i++)
			{
				if (X[i] == (double **) 0)
				{
					break;
				}
				else
				{
					n = myhandlelength((char **)X[i]);
					
					if (n < 0)
					{
						goto errorExit;
					}
					
					n /= sizeof(double);
					if ((symh1 = Makereal(n)) == (Symbolhandle) 0)
					{
						goto errorExit;
					}
					COMPVALUE(symh,i) = symh1;
					setNCOMPS(symh,i);
					sprintf(name,"X%ld",i);
					setNAME(symh1,name);
					for (j=0;j<n;j++)
					{
						DATAVALUE(symh1,j) = (*X[i])[j];
					}
				}
			} /*for (i=0;i<MAXVARS;i++)*/
		}

		if (MODELVARS[0] != (Symbolhandle) 0)
		{
			symh = COMPVALUE(Globals,nGlobals++) = Makestruc(nvars+1);
			if (symh == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			setNAME(symh,"MODELVARS");

			for (i=0;i<=nvars;i++)
			{
				COMPVALUE(symh,i) = symh1 = Makesymbol(REAL);
				if (symh1 == (Symbolhandle) 0 || Copy(MODELVARS[i],symh1) == 0)
				{
					goto errorExit;
				}
			}
		}
	} /*if (nvars < 0){}else{}*/
	
	setNCOMPS(Globals,nGlobals);
	return (Globals); /* note that Globals is in symbol table */

  errorExit:
	if (Globals != (Symbolhandle) 0)
	{
		setNCOMPS(Globals,nGlobals);
		Removesymbol(Globals);
		Globals = (Symbolhandle) 0;
	}
	
	return (0);
} /*bundleGlobals()*/               

#ifdef PLATFORM

static int saveSpecialNumbers(long * nGlobals)
{
	Symbolhandle       symh;
	unsigned char     *cptr;
	char              *hexChars = "0123456789ABCDEF";
	char               dBuffer[2*sizeof(double) + 1];
	char               lBuffer[2*sizeof(long) + 1];
	char              *place;
	double             x[nSampleDoubles];
	long               lx[nSampleLongs];
	int                nrows = (nSampleDoubles > nSampleLongs) ?
	  nSampleDoubles : nSampleLongs;
	long               length = 0;
	int                jnumber, i;
	WHERE("saveSpecialNumbers");
	
	/* 
	   Save character strings for four doubles and their hex representations
	*/
	
	for (jnumber = 0; jnumber < nSampleDoubles; jnumber++)
	{
		length += strlen((char *) SAMPLEDOUBLES[jnumber]) + 2*sizeof(double);
		sscanf((char *) SAMPLEDOUBLES[jnumber], "%lf", &x[jnumber]);
	} /*for (jnumber = 0; jnumber < nSampleDoubles; jnumber++)*/

	for (jnumber = 0; jnumber < nSampleLongs; jnumber++)
	{
		length += strlen((char *) SAMPLELONGS[jnumber]) + 2*sizeof(long);
		sscanf((char *) SAMPLELONGS[jnumber], "%ld", &lx[jnumber]);
	} /*for (jnumber = 0; jnumber < nSampleLongs; jnumber++)*/

	length += 4*nrows;
	
	symh = COMPVALUE(Globals, (*nGlobals)++) = Makesymbol(CHAR);
	if (symh == (Symbolhandle) 0)
	{
		return (0);
	}

	TMPHANDLE = mygethandle(length);
	setSTRING(symh, TMPHANDLE);
	if (TMPHANDLE == (char **) 0)
	{
		return (0);
	}
	place = *TMPHANDLE;
	
	for (jnumber = 0; jnumber < nrows; jnumber++)
	{
		if (jnumber < nSampleDoubles)
		{
			cptr = (unsigned char *) (x + jnumber);
			for (i = 0; i < sizeof(double); i++)
			{
				dBuffer[2*i] = hexChars[cptr[i]/16];
				dBuffer[2*i+1] = hexChars[cptr[i] % 16];
			}
			dBuffer[2*i] = '\0';
			place = copyStrings((char *) SAMPLEDOUBLES[jnumber], place, 1);
			place = copyStrings(dBuffer, place, 1);
		}
		else
		{
			*place++ = '\0';
			*place++ = '\0';
		}
	} /*for (jnumber = 0; jnumber < nrows; jnumber++)*/
	
	for (jnumber = 0; jnumber < nrows; jnumber++)
	{
		if (jnumber < nSampleLongs)
		{
			cptr = (unsigned char *) (lx + jnumber);;
			for (i = 0; i < sizeof(long); i++)
			{
				lBuffer[2*i] = hexChars[cptr[i]/16];
				lBuffer[2*i+1] = hexChars[cptr[i] % 16];
			}
			lBuffer[2*i] = '\0';
			place = copyStrings((char *) SAMPLELONGS[jnumber], place, 1);
			place = copyStrings(lBuffer, place, 1);
		}
		else
		{
			*place++ = '\0';
			*place++ = '\0';
		}
	} /*for (jnumber = 0; jnumber < nrows; jnumber++)*/

	setNAME(symh, "NUMBERS");
	setNDIMS(symh, 2);
	setDIM(symh, 1, 2*nrows);
	setDIM(symh, 2, 2);

	return (1);

} /*saveSpecialNumbers()*/

/*
  980728 LIMITS is now a CHARACTER vector
*/
static Symbolhandle bundleMachine(void)
{
	Symbolhandle       symh;
	long               limits[4];
	long               nGlobals = 0;
	long               i, length = 0;
	char             **cHandle, *place;
	char               cbuffer[4][20];
	WHERE("bundleMachine");
	
	if ((Globals = StrucInstall(MACHINE, NMACHINE)) == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	limits[0] = MAXDIMS;
	limits[1] = MAXVARS;
	limits[2] = WORDSPERTERM;
	limits[3] = VARSPERWORD;
	
	if (Pre407Save)
	{
		if (!long2Symbol(limits,"LIMITS",sizeof(limits)/sizeof(long),nGlobals++))
		{
			goto errorExit;
		}
	} /*if (Pre407Save)*/
	else
	{
		for (i = 0; i < 4; i++)
		{
			sprintf(cbuffer[i], "%ld", limits[i]);
			length += strlen(cbuffer[i]) + 1;
		}

		symh = COMPVALUE(Globals, nGlobals++) = Makesymbol(CHAR);
		if (symh == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		setNCOMPS(Globals, nGlobals);

		cHandle = mygethandle(length);
		setSTRING(symh, cHandle);
		if (cHandle == (char **) 0)
		{
			goto errorExit;
		}

		place = *cHandle;
		for (i = 0; i < 4; i++)
		{
			place = copyStrings(cbuffer[i], place, 1);
		}
		setNAME(symh,"LIMITS");
		setNDIMS(symh, 1);
		setDIM(symh, 1, 4);
	} /*if (Pre407Save){}else{}*/
	
	symh = COMPVALUE(Globals,nGlobals++) = Makesymbol(CHAR);
	if (symh == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	setNCOMPS(Globals, nGlobals);

	cHandle = mygethandle(strlen(PLATFORM) + 1);
	setSTRING(symh, cHandle);
	if (cHandle == (char **) 0)
	{
		goto errorExit;
	}
	
	strcpy(*cHandle, PLATFORM);
	setNAME(symh, "PLATFORM");
	setNDIMS(symh, 1);
	setDIM(symh, 1, 1);
	
	if (!Pre407Save && !saveSpecialNumbers(&nGlobals))
	{
		goto errorExit;
	}
	setNCOMPS(Globals, nGlobals);
	return (Globals); /* note that Globals is in symbol table */

  errorExit:
	if (Globals != (Symbolhandle) 0)
	{
		setNCOMPS(Globals,nGlobals);
		Removesymbol(Globals);
		Globals = (Symbolhandle) 0;
	}
	
	return (0);
} /*bundleMachine()*/
#endif /*PLATFORM*/             

static int savesym(Symbolhandle symh)
{
	long            i, ncomps;
	long            type, completetype, nvals;
	long            newstyle = (ASCII>=SAVEASCII3 || ASCII<=SAVEBINARY3);
	long            saveLabels = !Pre400Save;
	char           *name, *keyword;
	WHERE("savesym");

	if (symh == (Symbolhandle) 0)
	{
		return (1);
	}

	name = NAME(symh);
	type = TYPE(symh);
	if (Pre407Save)
	{
		completetype = type;
	}
	else
	{
		completetype = type | ((ISSHORT(symh)) ? SHORTSYMBOL : 0);
	}
	
	nvals = symbolSize(symh);

	if (isNull(symh))
	{
		completetype = (Pre336Save) ? REAL : NULLSYM;
	}
	
	/* don't save builtins, list, scratch, temps, keywords, T or F */
	if (type == BLTIN || type == UNDEF || type == ASSIGNED || type == LIST ||
		type == GARB ||
		isscratch(name) && name[2] != SPECIALPREFIX3 &&
		strcmp(name, LABELSSCRATCH) != 0 && strcmp(name, NOTESSCRATCH) != 0)
	{
		return (1);
	}

	if ((keyword = isKeyword(symh)))
	{
		/* caller should screen for keywords if they are not to be saved */
		name = keyword;
	}
	
	if (type == NULLSYM && Pre336Save)
	{ /* should not happen */
		return (1);
	}
	
	if (type == PLOTINFO)
	{
		return (saveGraph(symh, name)); /* saveGraph() is in plotutil.c */
	}
	
	/* Save symbol Header */
	if (!saveSymbolInfo(symh, name, completetype))
	{
		return (0);
	}
	
	/* Now save contents */

	switch (type)
	{
	  case NULLSYM:
		/* no contents */
		return (1);
		
	  case STRUC:
		ncomps = symbolSize(symh);
		for (i = 0; i < ncomps; i++)
		{
			if (!savesym(COMPVALUE(symh, i)))
			{
				return (0);
			}
		}
		return (1);

	  case MACRO:
	  case CHAR:
		return (saveHandle(STRING(symh), CHARHANDLE, nvals, (char *) 0));
		
	  case REAL:
	  case LOGIC:
		return (saveHandle((char **) DATA(symh), DOUBLEHANDLE, nvals, (char *) 0));

	  case LONG:
		return (saveHandle((char **) LONGDATA(symh), LONGHANDLE, nvals, (char *) 0));
	} /*switch (type)*/
	return (0);
} /*savesym()*/

#define FOUNDMISSVAL    0x01L
#define FOUNDNULLSYMH   0x02L
#define FOUNDINFINITY   0x04L

/*
   Check to see if there are any items to be saved are NULL or have NULL
   components, or if any REAL or LOGICAL data to be saved is MISSING
*/
static unsigned int anyNullOrMissing(Symbolhandle list, long saveSome)
{
	Symbolhandle        symh;
	long                i, nargs = NARGS(list);
	unsigned long       result = 0L;
	
	if (!saveSome)
	{
		for (symh = Lastsymbol(); symh != ENDPREV; symh = Prevsymbol(symh))
		{
			if (isNull(symh))
			{
				result |= FOUNDNULLSYMH;
			}
			if (strucAnyMissing(symh, REALTYPE1 | LOGICTYPE1))
			{
				result |= FOUNDMISSVAL;
			}
		}
	} /*if (!saveSome)*/
	else
	{/* partial save */
		for (i=1;i<nargs;i++)
		{
			symh = COMPVALUE(list,i);
			if (!isKeyword(symh) ||
				strcmp(NAME(symh),USEDKEYWORD) != 0 && isNull(symh))
			{
				if (isNull(symh))
				{
					result |= FOUNDNULLSYMH;
				}
				if (strucAnyMissing(symh, REALTYPE1 | LOGICTYPE1))
				{
					result |= FOUNDMISSVAL;
				}
			}
		} /*for (i=1;i<nargs;i++)*/
	} /*if (!saveSome){}else{}*/
	return (result);
} /*anyNullOrMissing()*/

/*
   check to see if there are any items with labels are to be saved
*/
static unsigned int anyWithLabels(Symbolhandle list, long saveSome)
{
	Symbolhandle        symh;
	
	if (!saveSome)
	{
		for (symh = Lastsymbol(); symh != ENDPREV; symh = Prevsymbol(symh))
		{
			if (anyLabels(symh))
			{
				return (1);
			}
		}
	} /*if (!saveSome)*/
	else
	{/* partial save */
		long                i, nargs = NARGS(list);

		for (i= 1 ; i < nargs;i++)
		{
			symh = COMPVALUE(list,i);
			if ((!isKeyword(symh) || strcmp(NAME(symh),USEDKEYWORD) != 0) &&
				anyLabels(symh))
			{
				return (1);
			}
		} /*for (i=1;i<nargs;i++)*/
	} /*if (!saveSome){}else{}*/
	return (0);
} /*anyWithLabels()*/
/*
   Check to see if PLOTINFO symbols are to be saved
*/
static unsigned int anyPlotInfo(Symbolhandle list, long saveSome)
{
	Symbolhandle        symh;
	
	if (!saveSome)
	{
		for (symh = Lastsymbol(); symh != ENDPREV; symh = Prevsymbol(symh))
		{
			if (TYPE(symh) == PLOTINFO)
			{
				return (1);
			}
		}
	} /*if (!saveSome)*/
	else
	{/* partial save */
		long                i, nargs = NARGS(list);

		for (i= 1 ; i < nargs;i++)
		{
			symh = COMPVALUE(list,i);
			if ((!isKeyword(symh) || strcmp(NAME(symh),USEDKEYWORD) != 0) &&
				TYPE(symh) == PLOTINFO)
			{
				return (1);
			}
		} /*for (i=1;i<nargs;i++)*/
	} /*if (!saveSome){}else{}*/
	return (0);
} /*anyPlotInfo()*/

/*
  Public functions for saving
  int saveName(char * nameItem)
  int saveHandle(char ** h, long type, long nvals, char * itemName)
  int saveLong(long longItem, char * itemName)
  int saveDouble(double doubleItem, char * itemName)
  int saveString(char * stringItem, char * itemName)
  int saveSymbolInfo(Symbolhandle symh, char * name, long type)
  Symbolhandle save(Symbolhandle symh)
*/
int saveName(char * nameItem)
{
	unsigned        m;
	WHERE("saveName");
	
	m = strlen(nameItem);
	if (m > 0 &&
	   fwrite((void *) nameItem, 1, m, KEEPFILE) != m ||
	   fwrite(&PadChar, 1, 1, KEEPFILE) != 1)
	{
		return(0);
	}
	return (1);
} /*saveName()*/

int saveRawHandle(char **item, long n)
{
	long          place = 0, m;

	while (n > 0)
	{
		m = (n <= MAXCHUNK) ? n : MAXCHUNK;
		if (fwrite(*item + place, 1, m, KEEPFILE) < m)
		{
			return (0);
		}
		n -= m;
		place += m;
	}
	return (1);
} /*saveRawHandle()*/

/*
  980715  Added argument itemName
          If itemName is not NULL, it is saved along with one of
          SAVEREALVECTOR, SAVECHARVECTOR, or SAVELONGVECTOR and the number
          of items in the vector and then the handle is saved
          in ascii format
*/
int saveHandle(char ** item, long type, long nvals, char * name)
{
	double            **hdouble;
	long              **hlong;
	WHERE("saveHandle");
	
	if (item != (char **) 0)
	{
		int           number, itemType;
		char          prefix[3];

		if (name && (!decodeItem(name, prefix, &number, &itemType) ||
					 itemType != type || !saveName(name) ||
					 !saveLong(nvals, (char *) 0)))
		{
			return (0);
		}
		
		/* Note: if name != 0, save in ascii format */
		if (Binary && name == (char *) 0 ||
			ASCII == SAVEASCII1 && type == CHARHANDLE)
		{ /* binary format; literal save */
			long                n;

			if (type == CHARHANDLE)
			{
				n = skipStrings(*item, nvals) - *item;
			}
			else
			{
				n = nvals *
				  ((type == DOUBLEHANDLE) ? sizeof(double) : sizeof(long));
			}

			return (saveRawHandle(item, n));
		} /*if (Binary || ASCII == SAVEASCII1 && type == CHARHANDLE)*/
		else
		{ /*ascii format */
			long                i, place;

			switch (type)
			{
			  case LONGHANDLE:
				hlong = (long **) item;

				for (i = 0;i < nvals;i++)
				{
					if (!saveLong((*hlong)[i], (char *) 0))
					{
						return (0);
					}
				}
				break;

			  case DOUBLEHANDLE:
				hdouble  = (double **) item;

				for (i = 0; i < nvals;i++)
				{
					if (!saveDouble((*hdouble)[i], (char *) 0))
					{
						return (0);
					}
				}
				break;

			  case CHARHANDLE:
				place = 0;
				for (i = 0; i < nvals; i++)
				{
					if (!saveString(*item + place, (char *) 0))
					{
						return (0);
					}
					place += strlen(*item + place) + 1;
				}
				break;

			} /*switch (type)*/
		} /*if (Binary || ASCII == SAVEASCII1 && type == CHARHANDLE){}else{}*/
	} /*if (item != (char **) 0)*/
	return (1);
} /*saveHandle()*/

int saveLong(long longItem, char * itemName)
{
	int        number, type;
	char       prefix[3];
	unsigned   m;
	int        result;
	
	if (itemName && (!decodeItem(itemName, prefix, &number, &type) ||
			type != SAVELONGSCALAR || !saveName(itemName)))
	{
		putOutErrorMsg("ERROR: Programming error in saveLong()");
		return (0);
	}

	sprintf(OUTSTR,LongFmt,longItem);
	m = strlen(OUTSTR);
	
	result = (fwrite(OUTSTR, 1, m, KEEPFILE) == m);
	*OUTSTR = '\0';
	return (result);
} /*saveLong()*/

int saveDouble(double doubleItem, char * itemName)
{
	int         number, type;
	char        prefix[3];
	unsigned    m;
	long       result;
	
	if (itemName && (!decodeItem(itemName, prefix, &number, &type) ||
			type != SAVEREALSCALAR || !saveName(itemName)))
	{
		return (0);
	}

	if (isMissing(doubleItem))
	{
		strcpy(OUTSTR, StringForMissing);
	}
	else
	{
		sprintf(OUTSTR,DoubleFmt,doubleItem);
	}
	
	m = strlen(OUTSTR);
	
	result = (fwrite(OUTSTR, 1, m, KEEPFILE) == m);
	*OUTSTR = '\0';
	return (result);
} /*saveDouble()*/

/*
  Note this definition of PRINTABLE differs slightly from a similar one
  in commonou.c by including '\n' but not '\t' as a printable character
  Also all characters > '~' are escaped, even on a Macintosh.  The intent
  is that an asciisave() file should consist completely of printable
  ascii characters <= '~', plus newlines.
*/

#define PRINTABLE(C) \
		((C) == '\n' || (C) >= 0x20 && (C) < 0x7f && (C) != '\\' )

int saveString(char * stringItem, char * itemName)
{
	long           length = strlen(stringItem);
	long           nChars;
	int            type, number;
	char           prefix[3];
	unsigned char  c, stringBuffer[BUFFERLENGTH+3];
	unsigned char *pc = (unsigned char *) stringItem;
	WHERE("saveString");

	if (itemName && (!decodeItem(itemName, prefix, &number, &type) ||
			type != SAVECHARSCALAR || !saveName(itemName)))
	{
		putOutErrorMsg("ERROR: Programming error in saveString()");
		return (0);
	}

	if (!saveLong(length, (char *) 0))
	{
		goto errorExit;
	}

	while (length > 0)
	{
		nChars = 0;
		do
		{
			c = *pc++;
			length--;
			if (ASCII <= 3 || PRINTABLE(c))
			{
				stringBuffer[nChars++] = c;
			}
			else
			{
				if (c == '\\' || c == '\t')
				{
					stringBuffer[nChars++] = '\\';
					stringBuffer[nChars++] = (c == '\\') ? c : 't';
				}
				else
				{
					nChars += escapedOctal(c, stringBuffer + nChars);
				}
			}
		} while (length > 0 && nChars < BUFFERLENGTH);
		if (fwrite((char *) stringBuffer, 1, nChars, KEEPFILE) != nChars)
		{
			goto errorExit;
		}
	} /*while (length > 0)*/

	if (fwrite(&PadChar, 1, 1, KEEPFILE) != 1)
	{
		goto errorExit;
	}

	return (1);     

  errorExit:
	return (0);
} /*saveString()*/

/*
  save shortSymbol (either labels or notes)
  Format is
    itemName
    itemType
    symbolHeader
    symbol
*/  

#ifndef LABELSARECHAR
static int saveNamedShortSymbol(char * itemName, ShortSymbolhandle symh)
{
	return (saveName(itemName) && savesym((Symbolhandle) symh));
} /*saveNamedShortSymbol()*/
#endif /*LABELSARECHAR*/

int saveNamedVector(char * itemName, long itemType, long itemLength,
					long * itemLvector, double * itemDvector,
					char * itemCvector)
{
	long          i;
	int           ok;
	int           type, number;
	char          prefix[3];
	char         *pc = itemCvector;
	WHERE("saveNamedVector");

	if (!decodeItem(itemName, prefix, &number, &type) || type != itemType)
	{
		putOutErrorMsg("ERROR: programming error in saveNamedVector()");
		return (0);
	}
	
	if (!saveName(itemName) || !saveLong(itemLength, (char *) 0))
	{
		return (0);
	}

	if ((itemType & SAVEREAL) && itemDvector == (double *) 0 ||
		(itemType & SAVELONG) && itemLvector == (long *) 0 ||
		(itemType & SAVECHAR) && itemCvector == (char  *) 0)
	{
		return (1);
	}
	
	for (i = 0; i < itemLength; i++)
	{
		if (itemType & SAVEREAL)
		{
			ok = saveDouble(itemDvector[i], (char *) 0);
		}
		else if (itemType & SAVELONG)
		{
			ok = saveLong(itemLvector[i], (char *) 0);
		}
		else if (itemType & SAVECHAR)
		{
			ok = saveString(pc, (char *) 0);
			pc = skipStrings(pc, 1);
		}
		
		if (!ok)
		{
			return (0);
		}
	} /*for (i = 0; i < itemLength; i++)*/
	return (1);
} /*saveNamedVector()*/

/*
  980715 new function to consolidate saving symbol name, type,
         dimensions, etc.
  980717 also save new marker before each symbol.  This may someday
         facilitate recovery from restore errors
*/

int saveSymbolInfo(Symbolhandle symh, char * name, long type)
{
	long            i, ndims, mdims;
	long            newstyle = (ASCII>=SAVEASCII3 || ASCII<=SAVEBINARY3);
	long            saveLabels = !Pre400Save;
	WHERE("saveSymbolInfo");

	if (!Pre407Save && !saveName(SYMBOLMARKER))
	{
		return (0);
	}

	if (!saveName(name))
	{
		return (0);
	}

	/* Save symbol type */
	if (!newstyle && ASCII != SAVEASCII2)
	{
		long      oldType;
		
		/* make sure type codes saved are right even if they change in future*/
		switch (type)
		{
		  case CHAR:
			oldType = CHAR_V31;
			break;
		  case REAL:
			oldType = REAL_V31;
			break;
		  case LOGIC:
			oldType = LOGIC_V31;
			break;
		  case STRUC:
			oldType = STRUC_V31;
			break;
		  case MACRO:
			oldType = MACRO_V31;
			break;
		  case PLOTINFO:
			oldType = PLOTINFO_V31;
			break;
		  case LONG:
			oldType = LONG_V31;
			break;

		  default:
			;
		} /*switch (type)*/

		if (!saveLong(oldType, (char *) 0))
		{
			return (0);
		}
	} /*if (!newstyle && ASCII != SAVEASCII2)*/
	else
	{
		saveName(typeName(type));
	}
	
	ndims = NDIMS(symh);
	if (Pre407Save)
	{
		/* save NCLASS(symh)*/
		if (!saveLong(NCLASS(symh), (char *) 0))
		{
			return (0);
		}

		/* Save dimension information */

		if (!saveLong(ndims, (char *) 0))
		{
			return (0);
		}
		mdims = (newstyle) ? ndims : MAXDIMS;
		for (i = 1; i <= mdims; i++)
		{
			if (!saveLong(DIMVAL(symh,i), (char *) 0))
			{
				return (0);
			}
		} /*for (i = 1; i <= mdims; i++)*/
	} /*if (Pre407Save)*/
	else
	{
		if (!saveLong(NCLASS(symh), SYMITEMNAME(SYMNCLASS)))
		{
			return (0);
		}

		if (!saveNamedVector(SYMITEMNAME(SYMDIMS), SAVELONGVECTOR, ndims,
							 &DIMVAL(symh,1), (double *) 0, (char *) 0))
		{
			return (0);
		}
	} /*if (Pre407Save){}else{}*/
	
	/*	save labels, if any */
	if (saveLabels)
	{
		long       nlabels = 0;

#ifdef LABELSARECHAR
		long       labelSize = sizeOfLabels(symh);

		if (!saveLong(labelSize,
					  (Pre407Save) ? (char *) 0 : SYMITEMNAME(SYMLABLEN)))
		{
			return (0);
		
		}
		if (labelSize > 0)
		{
			char      *labels = getOneLabel(symh, 0, 0);

			for (i = 1; i <= ndims; i++)
			{
				nlabels += DIMVAL(symh, i);
			}

			if (Pre407Save)
			{
				for (i = 0; i < nlabels; i++)
				{
					if (!saveString(labels, (char *) 0))
					{
						return (0);
					}
					labels = skipStrings(labels, 1);
				} /*for (i = 0; i < nlabels; i++)*/
			} /*if (Pre407Save)*/
			else
			{
				if (!saveNamedVector(SYMITEMNAME(SYMLABELS), SAVECHARVECTOR,
									 nlabels, (long *) 0, (double *) 0,
									 labels))
				{
					return (0);
				}
			} /*if (Pre407Save){}else{}*/
		} /*if (labelSize > 0)*/

#else /*LABELSARECHAR*/

		if (Pre407Save)
		{
			long            labelSize = 0;
			long            lengths[MAXDIMS];
			long            j;
			char           *labels[MAXDIMS];
			
			if (HASLABELS(symh))
			{
				getAllLabels(symh, labels, lengths, (long *) 0);
				for (j = 0; j < ndims; j++)
				{
					labelSize += lengths[j];
				}
			} /*if (HASLABELS(symh))*/
			if (!saveLong(labelSize, (char *) 0))
			{
				return (0);
			}
			if (labelSize > 0)
			{
				for (j = 0; j < ndims; j++)
				{
					long        dimj = DIMVAL(symh, j + 1);
					
					for (i = 1; i <= dimj; i++)
					{
						if (!saveString(labels[j], (char *) 0))
						{
							return (0);
						}
						if (i < dimj)
						{
							labels[j] = skipStrings(labels[j], 1);
						}
					} /*for (i = 1; i <= dimj; i++)*/
				} /*for (j = 0; j < ndims; j++)*/
			} /*if (labelSize > 0)*/
		} /*if (Pre407Save)*/
		else
		{
			if (HASLABELS(symh) &&
				!saveNamedShortSymbol(SYMITEMNAME(SYMLABELS), LABELS(symh)))
			{
				return (0);
			}
			if (HASNOTES(symh) &&
				!saveNamedShortSymbol(SYMITEMNAME(SYMNOTES), NOTES(symh)))
			{
				return (0);
			}
		} /*if (Pre407Save){}else{}*/
#endif /*LABELSARECHAR*/
	} /*if (saveLabels)*/

	if (!Pre407Save && !saveName(SYMITEMNAME(EndItems)))
	{
		return (0);
	}
	
	return (1);
} /*saveSymbolInfo()*/

/*
   General entry for both save() and asciisave()

   960502 Changed fopen() to fmyopen()
   960503 Changed macOpen() to macFindFile()
   960827 Changed checking of logical keywords to use isTorF()
   970506 Use BINARYWRITEMODE and TEXTWRITEMODE as fmyopen() mode
   980623 Added phrase keyword "history:T"
   980615 SAVEASCII7 and SAVEBINARY7 now defaults
   990107 Check to make sure first argument is not keyword phrase
          (unless it's file:filename)
          Also fixed bug so that format type 7 is used on
          a resave
*/
Symbolhandle    save(Symbolhandle list)
{
	Symbolhandle    symh;
	Symbolhandle    symhFile = COMPVALUE(list,0);
	Symbolhandle    symhV24 = (Symbolhandle) 0;
	Symbolhandle    symhTorF = (Symbolhandle) 0;
	Symbolhandle    symhHelp = (Symbolhandle) 0;
	long            saveAll = -DEFAULTALL;
	long            saveOptions = -DEFAULTSAVEOPTIONS;
	long            saveHistory = (ISATTY & ITTYIN) ? -DEFAULTSAVEHISTORY : 0;
	long            saveSome = 0;
	long            v31 = -1, v24 = -1, logicvalue;
	long            v335 = -1, noLabels = -1, v406 = -1;
	long            ascii = (strcmp(FUNCNAME,"asciisave") == 0) ? 1 : -1;
	long            specialOK = 0;
	long            isFilename = (symhFile != (Symbolhandle) 0);
	char           *whenSaved;
	long            i, nKeywords = 0, nargs = NARGS(list);
	char           *keyword, *chptr;
	char           *fileName, *header;
#ifdef MACINTOSH
	Str255          fName;
	OSType          fileType;
#endif
#ifdef WXWIN
	char            fName[WXMAXPATHLENGTH];
#endif /*WXWIN*/
	WHERE("save");

	OUTSTR[0] = '\0';

	for (i = (!isFilename) ? 1 : 0; i < nargs;i++)
	{ /* check for keywords */
		Symbolhandle         symhKey = COMPVALUE(list,i);
	
		if (!argOK(symhKey, NULLSYM,i+1))
		{
			goto errorExit;
		}
		
		if ((keyword = isKeyword(symhKey)))
		{
			if (i == 0 && strcmp(keyword, "file") != 0)
			{
				sprintf(OUTSTR,
						"ERROR: the only legal keyword phrase for %s() arg. 1 is 'file:fName'",
						FUNCNAME);
				goto errorExit;
			}
			
			if (strcmp(keyword, "all") == 0 ||
				strcmp(keyword, "options") == 0 ||
				strcmp(keyword, "history") == 0)
			{
				nKeywords++;

				setNAME(symhKey, USEDKEYWORD);
				if (!isTorF(symhKey))
				{
					notTorF(keyword);
					goto errorExit;
				}
#ifndef SAVEHISTORY
				if (keyword[0] == 'h')
				{
					sprintf(OUTSTR,
							"ERROR: keyword '%s' not available in this version",
							keyword);
					goto errorExit;
				}
				else
#endif /*SAVEHISTORY*/
				{
					logicvalue = (DATAVALUE(symhKey,0) != 0.0);
					if (keyword[0] == 'a')
					{
						saveOptions = saveAll = logicvalue;
					}
					else if (keyword[0] == 'o')
					{
						saveOptions = logicvalue;
					}
					else
					{
						saveHistory = logicvalue;
					}
				}
			}
			else if (strcmp(keyword,"old") == 0 ||
					 strcmp(keyword,"v24") == 0 ||
					 strcmp(keyword,"v31") == 0 ||
					 strcmp(keyword,"v335") == 0 ||
					 strcmp(keyword,"v406") == 0 ||
					 strncmp(keyword,"nolab", 5) == 0 ||
					 strcmp(keyword,"ascii") == 0)
			{/* old:T is same as v24:T */
				nKeywords++;
				setNAME(symhKey, USEDKEYWORD);
				if (!isTorF(symhKey))
				{
					notTorF(keyword);
					goto errorExit;
				} /*if (!isTorF(symhKey))*/
				else
				{
					logicvalue = (DATAVALUE(symhKey,0) != 0.0);
					if (strcmp(keyword,"old") == 0 ||
						strcmp(keyword,"v24") == 0)
					{
						v24 = logicvalue;
					}
					else if (strcmp(keyword,"v31") == 0)
					{
						v31 = logicvalue;
					}
					else if (strncmp(keyword, "v335", 4) == 0)
					{
						v335 = logicvalue;
					}
					else if (strcmp(keyword, "v406") == 0)
					{
						v406 = logicvalue;
					}
					else if (strncmp(keyword, "nolab", 5) == 0)
					{
						noLabels = logicvalue;
					}
					else
					{
						ascii = logicvalue;
					}
				} /*if (!isTorF(symhKey)){}else{}*/
			}
		} /*if ((keyword = isKeyword(symhKey)))*/
		else if (i > 0)
		{
			if (isscratch(NAME(symhKey)))
			{
				sprintf(OUTSTR,
						"ERROR: attempt to save expression or function result");
			}
			else if (TYPE(symhKey) == BLTIN)
			{
				sprintf(OUTSTR,
						"ERROR: attempt to save built-in function");
			}
			if (*OUTSTR)
			{
				goto errorExit;
			}
		} 
	} /*for (i = (!isFileName) ? 0 : 1; i < nargs;i++)*/

	saveSome = (nargs != nKeywords+1);

	if (saveSome)
	{
		saveOptions = (saveOptions < 0) ? 0 : saveOptions;
		saveHistory = (saveHistory < 0) ? 0 : saveHistory;
		saveAll = (saveAll < 0) ? 0 : saveAll;
	}
	
#ifdef SAVEHISTORY
	if (!(ISATTY & ITTYIN) && saveHistory)
	{
		sprintf(OUTSTR,
				"WARNING: 'history:T' ignored on %s() with non-interactive input",
				FUNCNAME);
		putErrorOUTSTR();
		saveHistory = 0;
	} /*if (!(ISATTY & ITTYIN) && saveHistory)*/
#endif /*SAVEHISTORY*/	

/*
	On the Mac, KeepVolume != 0 signals previous save() or asciisave()
	Elsewhere, KEEPFILENAME != 0 signals this.

	On Mac, a restore before a save sets KEEPFILENAME to the restore file
	name, but does not set KeepVolume

	960930 The above is no longer correct.
	 KEEPFILE != (FILE *) 0 && KEEPFILENAME != (char **) 0 signal a
	 previous save() or asciisave().  restore() resets KEEPFILENAME to the
	 restore file name, and zeros KEEPFILE.  On Windows and Mac versions,
	 Save Workspace menu command is equivalent to Save Workspace As unless
	 both KEEPFILE and KEEPFILENAME are non zero.  On all versions,
	 'save()' (with no file name) is an error unless both KEEPFILE and
	 KEEPFILENAME are non-zero.  The value of KEEPFILE is of course ignored
	 once it has been closed.


	If there has been a previous save() or asciisave(), a simple 'save()'
	without specifying a value for ascii, v31, v335, v24, or old, uses the
	previous value of ASCII.  Otherwise, unless v31, v335, v24, or old is
	set, it sets ASCII to the appropriate value based on previous save.
	Thus, a simple save() continues to use the previously set mode, whether
	ascii or binary, or compatible with one of the previous version.
*/

	if (!isFilename)
	{ /* no file name */
		if (KEEPFILE == (FILE *) 0 || KEEPFILENAME == (char **) 0)
		{
			sprintf(OUTSTR,"ERROR: no save file name specified");
			goto errorExit;
		}
		/* Has been previous save() or asciisave()*/

		/* Use ascii if it has been set */
		if (ascii > 0)
		{ /* asciisave() or ascii:T*/
			ASCII = (ASCII == SAVEBINARY1) ? SAVEASCII1 : ASCII;
			ASCII = (ASCII == SAVEBINARY2) ? SAVEASCII2 : ASCII;
			ASCII = (ASCII == SAVEBINARY3) ? SAVEASCII3 : ASCII;
			ASCII = (ASCII == SAVEBINARY4 || ASCII == SAVEBINARY5 ||
					 ASCII == SAVEASCII4 || ASCII == SAVEASCII5) ?
						 SAVEASCII5 : ASCII;
			ASCII = (ASCII == SAVEBINARY6 || ASCII == SAVEASCII6) ?
				SAVEASCII6 : ASCII;
			ASCII = (ASCII <= SAVEBINARY7 || ASCII >= SAVEASCII7) ?
				SAVEASCII7 : ASCII;
		} /*if (ascii > 0)*/
		else if (ascii == 0)
		{ /* ascii:F */
			ASCII = (ASCII == SAVEASCII1) ? SAVEBINARY1 : ASCII;
			ASCII = (ASCII == SAVEASCII2) ? SAVEBINARY2 : ASCII;
			ASCII = (ASCII == SAVEASCII3) ? SAVEBINARY3 : ASCII;
			ASCII = (ASCII == SAVEBINARY4 || ASCII == SAVEBINARY5 ||
					 ASCII == SAVEASCII4 || ASCII == SAVEASCII5) ?
						 SAVEBINARY5 : ASCII;
			ASCII = (ASCII <= SAVEBINARY6 || ASCII >= SAVEASCII6) ?
				SAVEBINARY6 : ASCII;
			ASCII = (ASCII <= SAVEBINARY7 || ASCII >= SAVEASCII7) ?
				SAVEBINARY7 : ASCII;
		}
		else /*save() without 'ascii'*/
		{
			ASCII = (ASCII == SAVEBINARY4 || ASCII == SAVEBINARY5) ?
						 SAVEBINARY5 : ASCII;
			ASCII = (ASCII == SAVEASCII4 || ASCII == SAVEASCII5) ?
						 SAVEASCII5 : ASCII;
			ASCII = (ASCII <= SAVEBINARY7) ? SAVEBINARY7 : ASCII;
			ASCII = (ASCII >= SAVEASCII7) ? SAVEASCII7 : ASCII;
		}			

		if (v24 < 0 && v31 < 0)
		{ /* none of v24, v31, or old were keywords */
			v24 = (ASCII == SAVEASCII1 || ASCII == SAVEBINARY1);
			v31 = (ASCII == SAVEASCII2 || ASCII == SAVEBINARY2);
		} /*if (v24 < 0 && v31 < 0)*/

		if (noLabels > 0)
		{
			ASCII = (ASCII > SAVEASCII5) ? SAVEASCII5 : ASCII;
			ASCII = (ASCII < SAVEBINARY6) ? SAVEBINARY5 : ASCII;
		}
		else if (noLabels < 0)
		{
			noLabels = (ASCII <= SAVEASCII5  && ASCII >= SAVEBINARY5) ? 1 : 0;
		}
	} /*if (!isFilename)*/
	else
	{ /* file name specified*/
#if (1) /* default save type is always SAVEASCII7 or SAVEBINARY7 */
		ASCII = (ascii > 0) ? SAVEASCII7 : SAVEBINARY7;
#else /*1*/ /*formerly reverted to 5 if no labels or plots */
		if (noLabels > 0 ||
			(!anyWithLabels(list, saveSome) && !anyPlotInfo(list, saveSome)))
		{
			ASCII = (ascii > 0) ? SAVEASCII5 : SAVEBINARY5;
		}
		else
		{
			ASCII = (ascii > 0) ? SAVEASCII7 : SAVEBINARY7;
		}
#endif /*1*/
	} /*if (!isFilename){}else{}*/

	if (saveAll && Pre336Save && PREVMODELTYPE & ROBUSTREG)
	{
		putOutErrorMsg("WARNING: cannot use all:T after robust() with v31:T or v335:T");
		saveAll = 0;
	}
	if (!Pre336Save && v335 > 0)
	{
		ASCII = (Binary) ? SAVEBINARY4 : SAVEASCII4;
	}
	else if (!saveAll && !anyNullOrMissing(list, saveSome))
	{
		ASCII = (ASCII == SAVEASCII5) ? SAVEASCII4: ASCII;
		ASCII = (ASCII == SAVEBINARY5) ? SAVEBINARY4: ASCII;
	}
	if (Pre336Save || v24 > 0 || v31 > 0)
	{
		sprintf(StringForMissing,"%.4f\n",OLDMISSING);
	}
	else
	{
		sprintf(StringForMissing,"%s\n",STRINGFORMISSING);
	}
	
	v24 = (v24 <= 0) ? 0 : 1;
	v31 = (v31 <= 0) ? 0 : 1;
	v335 = (v335 <= 0) ? 0 : 1;
	v406 = (v406 <= 0) ? 0 : 1;
	
	if (v24 + v31 + v335 + v406 > 1)
	{
		sprintf(OUTSTR,
				"ERROR: more than 1 of v31:T, old:T, v24:T, v335:T or v406:T on %s()",
				FUNCNAME);
		goto errorExit;
	} /*if (v24 + v31 + v335 + v406> 1)*/
	
	if (v24)
	{
#ifdef MACINTOSH
		if (Binary)
		{
			sprintf(OUTSTR,
					"ERROR: Cannot write version 2.4x binary save files");
			goto errorExit;
		}
#endif /*MACINTOSH*/
		if (saveAll)
		{
			putOutErrorMsg("WARNING: all:T ignored with old:T and v24:T");
			saveAll = 0;
		}
		ASCII = (Binary) ? SAVEBINARY1 : SAVEASCII1;
	} /*if (v24)*/
	else if (v31)
	{
		ASCII = (Binary) ? SAVEBINARY2 : SAVEASCII2;
	}
	else if (v406)
	{
		ASCII = (ASCII == SAVEASCII7) ? SAVEASCII6 : ASCII; 
		ASCII = (ASCII == SAVEBINARY7) ? SAVEBINARY6 : ASCII; 
	}

	if (saveSome && (saveOptions || saveHistory || saveAll))
	{
		putOutErrorMsg("WARNING: 'options', 'all' and/or 'history' ignored on partial save");
		saveHistory = saveOptions = saveAll = 0;
	} /*if (saveSome && (saveOptions >= 0 || saveHistory >= 0 || saveAll > 0))*/


	if (!isFilename)
	{ /* use previous filename */
#ifdef MACINTOSH
		SetVol(0L, KeepVolume);
#endif
#if (0) /*990107 No longer print this message*/
		sprintf(OUTSTR, "Saving on file %s",*KEEPFILENAME);
		putOUTSTR();
#endif /*0*/
	} /*if (!isFilename)*/
	else
	{ /* get new file name */
		if (TYPE(symhFile) != CHAR || !isScalar(symhFile))
		{
			notCharOrString("file name");
			goto errorExit;
		}
		fileName = STRINGPTR(symhFile);

#ifdef HASFINDFILE
		if (KEEPFILENAME != (char **) 0)
		{
			/* use previous name as default */
			strcpy((char *) fName, *KEEPFILENAME);
#ifdef MACINTOSH
			CtoPstr((char *) fName);
#endif /*MACINTOSH*/
		} /*if (KEEPFILENAME != (char **) 0)*/
		else
		{
			fName[0] = '\0';
		}
		
#ifdef MACINTOSH
		fileName = macFindFile(fileName, "\pSave workspace as:", fName, 
							   WRITEIT, 0, (OSType *) 0, &KeepVolume);
#endif /*MACINTOSH*/
#ifdef WXWIN
		fileName = wxFindFile(fileName, "Save workspace as:", fName);
#endif /*WXWIN*/
		if (fileName == (char *) 0)
		{
			sprintf(OUTSTR, "WARNING: %s() cancelled", FUNCNAME);
			putErrorOUTSTR();
			return (NULLSYMBOL);
		} /*if (fileName == (char *) 0)*/

#ifdef MACINTOSH
		if (RestoreVolume == 0)
		{
			RestoreVolume = KeepVolume;
		}
#endif /*MACINTOSH*/
#endif /*HASFINDFILE*/

		fileName = expandFilename(fileName);
		/*
		   At this point fileName is a pointer to a static buffer, not a
		   dereferenced handle
		*/
		if (fileName == (char *) 0 || !isfilename(fileName))
		{
			goto errorExit;
		}
		mydisphandle(KEEPFILENAME);
		KEEPFILENAME = mygethandle(strlen(fileName) + 1);
		if (KEEPFILENAME == (char **) 0)
		{
			goto errorExit;
		}
		strcpy(*KEEPFILENAME,fileName);
	} /*if (!isFilename)*/

#ifndef MACINTOSH
	KEEPFILE = fmyopen(*KEEPFILENAME,
					 (Binary || ASCII == SAVEASCII1) ?
					   BINARYWRITEMODE : TEXTWRITEMODE);
#else /*MACINTOSH*/
	KEEPFILE = fmyopen(*KEEPFILENAME, BINARYWRITEMODE);
#endif /*MACINTOSH*/

	if (KEEPFILE == (FILE *) 0)
	{
		sprintf(OUTSTR,"ERROR: cannot open %s for saving", *KEEPFILENAME);
		goto errorExit;
	}

	CharFmt = "%s ";
	LongFmt = "%ld ";
	DoubleFmt = "%.20g "; /* 980713 changed from "%23.15e " */
	PadChar = ' ';
	switch (ASCII)
	{
	  case SAVEBINARY1:
	  case SAVEBINARY2:
		header = BINARYHEADER1;
		break;
	
	  case SAVEBINARY3:
	  case SAVEBINARY4:
		header = BINARYHEADER3;
		break;
	
	  case SAVEBINARY5:
		header = BINARYHEADER5;
		break;
	
	  case SAVEBINARY6:
		header = BINARYHEADER6;
		break;
	
	  case SAVEBINARY7:
		header = BINARYHEADER7;
		break;
	
#if (CURRENTSAVEFORMATLEVEL >= 8)
	  case SAVEBINARY8:
		header = BINARYHEADER8;
		break;

#if (CURRENTSAVEFORMATLEVEL >= 9)
	  case SAVEBINARY9:
		header = BINARYHEADER9;
		break;

#endif /*CURRENTSAVEFORMATLEVEL >= 8*/
#endif /*CURRENTSAVEFORMATLEVEL >= 9*/

	  case SAVEASCII1:
		header = ASCIIHEADER1;
		break;

	  case SAVEASCII2:
		header = ASCIIHEADER2;
		break;

	  case SAVEASCII3:
		header = ASCIIHEADER3;
		break;

	  case SAVEASCII4:
		header = ASCIIHEADER4;
		break;

	  case SAVEASCII5:
		header = ASCIIHEADER5;
		break;

	  case SAVEASCII6:
		header = ASCIIHEADER6;
		break;

	  case SAVEASCII7:
		header = ASCIIHEADER7;
		break;

#if (CURRENTSAVEFORMATLEVEL >= 8)
	  case SAVEASCII8:
		header = ASCIIHEADER8;
		break;

#if (CURRENTSAVEFORMATLEVEL >= 9)
	  case SAVEASCII9:
		header = ASCIIHEADER9;
		break;

#endif /*CURRENTSAVEFORMATLEVEL >= 8*/
#endif /*CURRENTSAVEFORMATLEVEL >= 9*/

	} /*switch (ASCII)*/

/* NOTE, PadChar should not be changed to '\n' until after saveName(header) */
	if (!saveName(header))
	{
		goto diskFull;
	}

	if (!Binary && ASCII != SAVEASCII1)
	{
		CharFmt = "%s\n";
		LongFmt = "%ld\n";
		DoubleFmt = "%.20g\n";
		PadChar = '\n';
	}
	
	if (!Pre407Save && !saveName((saveSome) ? PARTIALSAVE : FULLSAVE))
	{
		goto diskFull;
	}

/* First things saved is time and date info */
	if (!v24)
	{ /* do not save time and date when old:T */
		whenSaved = getTimeAndDate();
		
		symh = CInstall(SAVETIME, strlen(whenSaved)+1);
		if (symh == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		setNDIMS(symh,1);
		setDIM(symh,1,1);
		strcpy(STRINGPTR(symh),whenSaved);
		if (!savesym(symh))
		{
			goto diskFull;
		}
		Removesymbol(symh);
		
#ifdef PLATFORM
/*
   Next thing saved is information about the machine and version
   implementation
*/
		if (!v31)
		{ /* do not save machine info when v31:T is used */
			if ((symh = bundleMachine()) == (Symbolhandle) 0)
			{
				goto errorExit;
			}

			if (!savesym(symh))
			{
				goto diskFull;
			}
			Removesymbol(symh);
		} /*if (!v31)*/
#endif /*PLATFORM*/
	} /*if (!v24)*/
	else
	{ /*v24:T*/
/*
   Or special things for version 2.4x such as T and F
*/
		char      *msgs = "Type Help() followed by a return to get help";
		
		symhV24 = StrucInstall(SCRATCH,2);
		if (symhV24 == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		symhTorF = COMPVALUE(symhV24,0) = Makereal(1);
		if (symhTorF == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		setTYPE(symhTorF,LOGIC);
		setNAME(symhTorF,"T");
		DATAVALUE(symhTorF,0) = 1.0;
		if (!savesym(symhTorF))
		{
			goto diskFull;
		}
		setNAME(symhTorF,"F");
		DATAVALUE(symhTorF,0) = 0.0;
		if (!savesym(symhTorF))
		{
			goto diskFull;
		}

		symhHelp = COMPVALUE(symhV24,1) = Makechar(strlen(msgs) + 1);
		if (symhHelp == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		
		strcpy(STRINGPTR(symhHelp), msgs);
		setNAME(symhHelp,"help");
		if (!savesym(symhHelp))
		{
			goto diskFull;
		}
		setNAME(symhHelp,"HELP");
		if (!savesym(symhHelp))
		{
			goto diskFull;
		}
		Removesymbol(symhV24);
		symhV24 = (Symbolhandle) 0;
	} /*if (!v24){}else{}*/

/*
   Now save some or all of variables
*/
	
	if (!saveSome)
	{
	/* save things backwards so that they will be restored in right order*/
		for (symh = Lastsymbol(); symh != ENDPREV; symh = Prevsymbol(symh))
		{
			if (!(Pre336Save && isNull(symh)) &&
				(specialOK || !isSpecial(symh)) && !savesym(symh))
			{
				goto diskFull;
			}
		} /*for (symh = Lastsymbol();symh != ENDPREV;symh = Prevsymbol(symh))*/
	} /*if (!saveSome)*/
	else
	{/* partial save */
		for (i=1;i<nargs;i++)
		{
			symh = COMPVALUE(list,i);
			if (!(keyword = isKeyword(symh)) ||
				strcmp(NAME(symh),USEDKEYWORD) != 0)
			{
				if (!(Pre336Save && isNull(symh)) &&
					(specialOK || !isSpecial(symh)) && !savesym(symh))
				{
					goto diskFull;
				}
			}
		} /*for (i=1;i<nargs;i++)*/
	} /*if (!saveSome){}else{}*/
		
#ifdef SAVEHISTORY
	if (!v24 && !v31 && !v335 && saveHistory)
	{ /* do not save history in old formats */
		char        **history;
		long          nlines;
		
		nlines = getSomeHistory(0, &history);
		if (nlines > 0)
		{
			symh = CInstall(HISTRY, 0);

			if (symh == (Symbolhandle) 0)
			{
				mydisphandle(history);
				goto errorExit;
			}
			setSTRING(symh, history);
			Setdims(symh, 1, &nlines);
			setNAME(symh, HISTRY);

			if (!savesym(symh))
			{
				goto diskFull;
			}
			Removesymbol(symh);
		}
	} /*if (!v24 && !v31 && !v335 && saveHistory)*/
#endif /*SAVEHISTORY*/

	if (!v24 && saveOptions)
	{ /* do not save options with old:T */
		symh = getOpt((short *) 0);
		if (symh == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		setNAME(symh,OPTIONS);

		if (!savesym(symh))
		{
			goto diskFull;
		}
		Removesymbol(symh);
	} /*if (!v24 && saveOptions)*/

	if (!v24 && saveAll)
	{
		if ((symh = bundleGlobals()) == (Symbolhandle) 0)
		{
			goto errorExit;
		}

		if (!savesym(symh))
		{
			goto diskFull;
		}
		Removesymbol(symh);
	} /*if (!v24 && saveAll)*/

#ifdef MACINTOSH
	fileType = (Binary) ? KEEPFILETYPE : ASCIIKEEPFILETYPE;
	macSetInfo(KeepVolume,*KEEPFILENAME,fileType,KEEPCREATOR);
#endif  /*MACINTOSH*/

	fclose(KEEPFILE);

	/*
	   960930  Don't zero KEEPFILE.  It is flag that a save has been done
	           It is zeroed by restore()
    */
	
	chptr = (!Binary) ? "ascii" : "";
	if (!saveSome)
	{
		sprintf(OUTSTR,"Workspace %ssaved on file ", chptr);
	}
	else
	{
		sprintf(OUTSTR,"Specified variables %ssaved on file ", chptr);
	}
	myprint(OUTSTR);
	*OUTSTR = '\0';
	putOutMsg(*KEEPFILENAME);
#ifdef MACINTOSH
	UNLOADSEG(saveGraph);
#endif /*MACINTOSH*/
	return (NULLSYMBOL);

  diskFull:
	sprintf(OUTSTR,
			"ERROR: Problem writing save file \"%s\".  Out of disk space?",
			*KEEPFILENAME);

  errorExit:
	putErrorOUTSTR();
	if (KEEPFILE != (FILE *) 0)
	{
		fclose(KEEPFILE);
	}
	
	if (v24)
	{
		Removesymbol(symhV24);
	}
	
#ifdef MACINTOSH
	UNLOADSEG(saveGraph);
#endif /*MACINTOSH*/
	return (0);
	
} /*save()*/



