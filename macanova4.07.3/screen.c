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
#pragma segment Screen
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


/*
   driver for screen()
   951107 Removed capability of positional specification of
   mbest, forced, method, s2, and penalty.

   Redid keyword handling using a keywordList and associated macros.

   All scratch is allocated using getScratch macro instead of piggybacking
   on glm globals SS, XTXINV, and REGX

  990215 replaced myerrorout() by putOutErrorMsg() and putOutMsg()
  990226 replaced most uses of putOUTSTR() by putErrorOUTSTR()
  990310 made a number of cosmetic changes, fixed one bug, 'force' can
         be used instead of 'forced'.
*/

#include "globals.h"
#include "blas.h"

enum scratchIds
{
	GIW = 0,
	GRT,
	GRW,
	GREGX,
	NTRASH
};
/* control codes for keep */
enum screenKeepCodes
{
	KEEPP      = 0x01,
	KEEPCP     = 0x02,
	KEEPADJRSQ = 0x04,
	KEEPRSQ    = 0x08,
	KEEPMODEL  = 0x10,
	KEEPALL    = KEEPP | KEEPCP | KEEPADJRSQ | KEEPRSQ | KEEPMODEL
};

/*
   Methods codes, used in leapstrans() (in lpsmisc.c) and
   in leapsforwd() (lpsforwd.c)
   Hard coded there (variable ib) as 1, 2, 3
*/
enum methodCodes 
{
	RSQMETHOD = 1,
	ADJRSQMETHOD,
	CPMETHOD
};

static long         Keep;    /* specifies what to return if anything */
static long         Verbose; /* if 0, suppress printed output */
static long         Imodel;  /* sequence number of model */

static Symbolhandle Result;
static char         *Compnames[] = 
{
	"p","cp","adjrsq","rsq","model"
};
static long         Compops[] = 
{
	KEEPP, KEEPCP, KEEPADJRSQ, KEEPRSQ, KEEPMODEL
};

/* set up symbol for values to return */
static Symbolhandle initKeep(long keep, long imethod, long mbest, long nforced)
{
	Symbolhandle    symh, result = (Symbolhandle) 0;
	long            nmodels = 0, ncomps = 0, icomp;
	long            binomcoef;
	long            i, nvars = (long) NVARS - nforced;
	
	if(imethod == RSQMETHOD)
	{
		binomcoef = 1;

		for(i=1;i<=nvars;i++)
		{
			binomcoef = (binomcoef*(nvars-i+1))/i;
			nmodels += (binomcoef < mbest) ? binomcoef : mbest;
		}
	}
	else
	{
		nmodels = (nvars < 31) ?
			intpow(2.0,(double) nvars) - 1 : 2147483647;
		nmodels = (nmodels < mbest) ? nmodels : mbest;
	}

	ncomps = 0; /* number of items requested to keep*/
	for(icomp=0;icomp<5;icomp++)
	{
		if(keep & Compops[icomp])
		{
			ncomps++;
		}
	} /*for(icomp=0;icomp<5;icomp++)*/

	if(ncomps == 1)
	{ /* keep as vector */
		result = (!(keep & KEEPMODEL)) ?
			RInstall(SCRATCH,nmodels) : CInstall(SCRATCH,0);
		if (result == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		setNDIMS(result,1);
	} /*if(ncomps == 1)*/
	else
	{
		if ((result = StrucInstall(SCRATCH,ncomps)) == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		ncomps = 0;
		for(icomp = 0;icomp < 5;icomp++)
		{
			if(keep & Compops[icomp])
			{
				if(Compops[icomp] != KEEPMODEL)
				{
					symh = COMPVALUE(result,ncomps++) = Makereal(nmodels);
					if(symh == (Symbolhandle) 0)
					{
						goto errorExit;
					}
				} /*if(Compops[icomp] != KEEPMODEL)*/
				else
				{
					symh = COMPVALUE(result,ncomps++) = Makesymbol(CHAR);
					if(symh == (Symbolhandle) 0)
					{
						goto errorExit;
					}
					setNDIMS(symh,1);
				} /*if(Compops[icomp] != KEEPMODEL){}else{}*/
				setCompName(symh,Compnames[icomp]);
			} /*if(keep & Compops[icomp])*/
		} /*for(icomp = 0,icomp < 5;icomp++)*/
	} /*if(ncomps == 1){}else{}*/
	return (result);
	
  errorExit:
	Removesymbol(result);
	return (0);
} /*initKeep()*/

/*
  971104 added check for value of myhandlelength()
*/
static long screenDoKeep(long keep, Symbolhandle result, double p, double cp,
						 double r2, double adjr2,
						 long *ind, long nvar)
{
	Symbolhandle   symh;
	long           ncomps = (TYPE(result) == STRUC) ? NCOMPS(result) : 1;
	long           jcomp = 0, icomp, i, length;
	int            evaluated;
	char          *model;
	WHERE("screenDoKeep");
	
	for(icomp=0;icomp<5;icomp++)
	{
		if(keep & Compops[icomp])
		{
			symh = (ncomps > 1) ? COMPVALUE(result,jcomp++) : result;
			if(Compops[icomp] == KEEPP)
			{
				DATAVALUE(symh,Imodel) = p;
			}
			else if(Compops[icomp] == KEEPCP)
			{
				DATAVALUE(symh,Imodel) = cp;
			}
			else if(Compops[icomp] == KEEPRSQ)
			{
				DATAVALUE(symh,Imodel) = r2;
			}
			else if(Compops[icomp] == KEEPADJRSQ)
			{
				DATAVALUE(symh,Imodel) = adjr2;
			}
			else if(Compops[icomp] == KEEPMODEL)
			{
				length = strlen(*DEPVNAME);
				if (isevaluated(VARNAMES[0]))
				{
					length += 2;
				}
				
				for (i = 0; i < nvar; i++)
				{
					length += strlen(VARNAME(ind[i])) + 1;
					if (isevaluated(VARNAMES[ind[i]]))
					{
						length += 2;
					}
				} /*for (i = 0; i < nvar; i++)*/

				if(Imodel == 0)
				{ /* first model */
					TMPHANDLE = mygethandle(length+1);
					setSTRING(symh,TMPHANDLE);
					if(TMPHANDLE == (char **) 0)
					{
						goto errorExit;
					}
					model = *TMPHANDLE;
				} /*if(Imodel == 0)*/
				else
				{
					long      handleLength;
					
					TMPHANDLE = STRING(symh);
					handleLength = myhandlelength(TMPHANDLE);
					if (handleLength < 0)
					{
						goto errorExit;
					}
					
					TMPHANDLE = mygrowhandle(TMPHANDLE,
											 handleLength + length + 1);
					setSTRING(symh,TMPHANDLE);
					if(TMPHANDLE == (char **) 0)
					{
						goto errorExit;
					}
					model = skipStrings(*TMPHANDLE,Imodel);
				} /*if(Imodel == 0){}else{}*/
				
				evaluated = isevaluated(VARNAMES[0]);

				if (evaluated)
				{
					*model++ = '{';
				}
				strcpy(model,*DEPVNAME);
				model += strlen(model);
				if (evaluated)
				{
					*model++ = '}';
				}
				for (i = 0; i < nvar; i++)
				{
					evaluated = isevaluated(VARNAMES[ind[i]]);
					
					*model++ = (i == 0) ? '=' : '+';
					if (evaluated)
					{
						*model++ = '{';
					}
					strcpy(model, VARNAME(ind[i]));
					model += strlen(model);
					if (evaluated)
					{
						*model++ = '}';
						if (i == nvar - 1)
						{
							*model = '\0';
						}
					}
				}					
				setDIM(symh,1,Imodel+1);
			}
		} /*if(keep & Compops[icomp])*/
	} /*for(icomp=0;icomp<5;icomp++)*/
	Imodel++;
	return (1);
	
  errorExit:
	return (0);
	
} /*screenDoKeep()*/

static keywordList       ScreenKeys[] =
{
#define KMBEST    0
	InitKeyEntry("mbest",0,0,POSITIVEINT),
#define KFORCED   1
	InitKeyEntry("forced",5,0,SYMHVALUE),
#define KMETHOD   2
	InitKeyEntry("method",4,0,CHARSCALAR),
#define KS2       3
	InitKeyEntry("s2",0,0,POSITIVEREAL),
#define KSSQ      4
	InitKeyEntry("ssq",0,0,POSITIVEREAL),
#define KPENALTY  5
	InitKeyEntry("penalty",3,0,POSITIVEREAL),
#define KKEEP     6
	InitKeyEntry("keep",0,0,CHARVECTOR)
};

#define MbestKey   KeyIntValue(ScreenKeys,KMBEST)
#define ForcedKey  KeySymhValue(ScreenKeys,KFORCED)
#define MethodKey  KeyCharValue(ScreenKeys,KMETHOD)
#define S2Key      KeyRealValue(ScreenKeys,KS2)
#define SsqKey     KeyRealValue(ScreenKeys,KSSQ)
#define PenaltyKey KeyRealValue(ScreenKeys,KPENALTY)
#define KeepKey    KeySymhValue(ScreenKeys,KKEEP)

/*
   Front end for leaps and bounds screening of all possible regressions.
   The model has been decoded into the globals, and the various controlling
   keywords are in COMPVALUE(list,i),i=1,...,nargs

   951107  Removed positionally dependent arguments.   All except Model
   must be keyword phrases.

   990310 revised checking of keywords and dimensions.  Moved test
          for too many variables from leapsforwd() in lpsorwd.c to screen()
*/

Symbolhandle    screen(Symbolhandle list, long printit)
{
	/* control  routine to do leaps and bounds */

	Symbolhandle    symh;
	char           *keyword, *outstr;
	char           *tmps;
	long            nargs = NARGS(list);
	long            i, j, k, i2;
	long            nv, it, nforced, imethod, mbest;
	long            nd, nc, nw, nr, ne, iv;
	long            ij, ji;
	long            nkeep;
	long            inc1 = 1;
	long          **iwH = (long **) 0, *iw;
	long            ndata = (long) NDATA, nvars = (long) NVARS;
	long            notmissing = (long) NOTMISSING;
	long            nkeys = NKeys(ScreenKeys), keyStatus;
	long            minNdata = nvars + 2;
	keywordListPtr  keys = ScreenKeys;
	char           *keepvec;
	double          penalty, tl, s2;
	double         *misswts, *x;
	double        **regxH, *regx1, *regx2;
	double        **rtH, *rt;
	double        **rwH, *rw;
	double          tmpd, *tmpdp;
	WHERE("screen");
	TRASH(NTRASH,errorExit);
	
	*OUTSTR = '\0';
	
	Verbose = (printit != 0) ? 1 : 0;
	Keep = Imodel = 0;
	Result = (Symbolhandle) 0;

	unsetKeyValues(keys, nkeys);

	/*
	  Check for keywords
	  Positional arguments are no longer allowed before 1st keyword, if any
	*/

	for (i=1;i<nargs;i++)
	{
		symh = COMPVALUE(list, i);
		if(!(keyword = isKeyword(symh)))
		{
			sprintf(OUTSTR,
					"ERROR: all arguments to %s() except the first must be keyword phrases",
					FUNCNAME);
			goto errorExit;
		}
		keyStatus = getOneKeyValue(symh, 0, keys, nkeys);
		if (keyStatus == 0)
		{
			goto errorExit;
		}
		if (keyStatus == -1)
		{
			badKeyword(FUNCNAME, keyword);
			goto errorExit;
		}
	} /*for (i=1;i<nargs;i++)*/

	if (symhIsSet(KeepKey))
	{
		nkeep = symbolSize(KeepKey);
		keepvec = STRINGPTR(KeepKey);
		for(i=0;i<nkeep;i++)
		{
			if(mystrncmp("all",keepvec, 0) == 0)
			{
				Keep |= KEEPALL;
			}
			else if(mystrncmp("p",keepvec, 0) == 0)
			{
				Keep |= KEEPP;
			}
			else if(mystrncmp("cp",keepvec, 0) == 0)
			{
				Keep |= KEEPCP;
			}
			else if(mystrncmp("adj",keepvec,3) == 0)
			{
				Keep |= KEEPADJRSQ;
			}
			else if(mystrncmp("rsq",keepvec, 3) == 0)
			{
				Keep |= KEEPRSQ;
			}
			else if(mystrncmp("model",keepvec,0) == 0)
			{
				Keep |= KEEPMODEL;
			}
			else
			{
				sprintf(OUTSTR,
						"ERROR: %s not legal value for '%s'",
						keepvec, KeyName(keys, KKEEP));
				goto errorExit;
			}
			keepvec = skipStrings(keepvec,1);
		} /*for(i=0;i<nkeep;i++)*/
		if (printit < 0)
		{
			Verbose = 0;
		}
	} /*if (symhIsSet(KeepKey))*/

	if(!Verbose && !Keep)
	{
		sprintf(OUTSTR,
				"ERROR: print:F illegal for %s unless keyword '%s' is used",
				FUNCNAME, KeyName(keys, KKEEP));
		goto errorExit;
	} /*if(!Verbose && !Keep)*/

	if (charIsSet(MethodKey))
	{
		tmps = *MethodKey;
		if(mystrncmp(tmps,"r2",0) == 0 || mystrncmp(tmps,"r",0) == 0 ||
		   mystrncmp(tmps,"rsq",0) == 0)
		{
			imethod = RSQMETHOD;
		}
		else if(mystrncmp(tmps,"adj",3) == 0 || 
				mystrncmp(tmps,"a", 0) == 0)
		{
			imethod = ADJRSQMETHOD;
		}
		else if(mystrncmp(tmps,"cp",0) == 0 || 
				mystrncmp(tmps,"c",0) == 0)
		{
			imethod = CPMETHOD;
		}
		else
		{
			sprintf(OUTSTR,"ERROR: illegal method %s for %s", tmps, FUNCNAME);
			goto errorExit;
		}
	} /*if (charIsSet(MethodKey))*/
	else
	{
		imethod = CPMETHOD; /* default is Mallow's Cp */
	}
	
	if (realIsSet(S2Key) || realIsSet(SsqKey))
	{
		s2 = S2Key;
		if (!realIsSet(S2Key))
		{
			s2 = SsqKey;
		}
		else if (realIsSet(SsqKey))
		{
			sprintf(OUTSTR,
					"ERROR: you can't use both %s and %s; they are synonyms",
					KeyName(keys, KS2), KeyName(keys, KSSQ));
			goto errorExit;
		}
	} /*if (realIsSet(S2Key) || realIsSet(SsqKey))*/
	else
	{
		s2 = 0.0;
	}
	
	penalty = (realIsSet(PenaltyKey)) ? PenaltyKey : 2.0;
	
	mbest = (longIsSet(MbestKey)) ? MbestKey : 5;

	{
		double       bit = 2.0;
		int          nbit = 1;
		while (bit != bit + 1.0)
		{
			nbit++;
			bit += bit;
		}
		if (nvars >= nbit)
		{
			sprintf(OUTSTR,
					"ERROR: too many variables for %s()", FUNCNAME);
			goto errorExit;
		}
	}
	if (notmissing < minNdata)
	{
		sprintf(OUTSTR,
				"ERROR: too few cases for %s()", FUNCNAME);
		goto errorExit;
	}

	/* set up screen parameters */
	nv = nvars + 1;
	it = 1;
	nd = nv;
	nc = 3 * nd;
	nw = 4 * nv;
	tl = 10.;
	ne = 0;
	iv = 0;
	

	if(!getScratch(iwH,GIW,nw,long))
	{
		goto errorExit;
	}
	iw = *iwH;
	
	for (i = 0; i < nw; i++)
	{
		iw[i] = 0;
	}

	for (i = 0; i < nvars; i++)
	{
		iw[i] = i + 1;
	}
	iw[nvars] = nv;

	nforced = 0;
	if (symhIsSet(ForcedKey))
	{
		if(!isVector(ForcedKey) ||
		   (TYPE(ForcedKey) != REAL && TYPE(ForcedKey) != CHAR))
		{
			sprintf(OUTSTR,
					"ERROR: List of forced variables not REAL or CHARACTER vector");
			goto errorExit;
		}

		if (TYPE(ForcedKey) == REAL)
		{
			tmpdp = DATAPTR(ForcedKey);
			for (i = 0; i < DIMVAL(ForcedKey,1); i++)
			{
				tmpd = tmpdp[i];
				if(tmpd <= 0.0 || tmpd != floor(tmpd))
				{
					sprintf(OUTSTR,
							   "ERROR: forced variable subscripts must be positive integers");
					goto errorExit;
				}
				else if((k = (long) tmpd) > nvars)
				{
					sprintf(OUTSTR,
							"ERROR: forced variable subscript out of range");
					goto errorExit;
				}
				iw[k + nvars] = 1;
			}
		} /*if (TYPE(ForcedKey) == REAL)*/
		else
		{
			tmps = STRINGPTR(ForcedKey);
			for (i = 0; i < DIMVAL(ForcedKey,1); i++)
			{
				for (k = 1; k <= nvars; k++)
				{
					if (strcmp(tmps, VARNAME(k)) == 0)
					{
						break;
					}
				} /*for (k = 1; k <= nvars; k++)*/
				if (k < 1 || k > nvars)
				{
					sprintf(OUTSTR,"ERROR: forced variable not in model");
					goto errorExit;
				}
				iw[nvars + k] = 1;
				tmps = skipStrings(tmps, 1);
			} /*for (i = 0; i < DIMVAL(ForcedKey,1); i++)*/
		} /*if (TYPE(ForcedKey) == REAL){}else{}*/

		for (i = 0; i < nvars; i++)
		{
			if (iw[nvars + i + 1])
			{
				iw[nforced] = i + 1;
				nforced++;
			}
			else
			{
				iw[nvars - 1 - i + nforced] = i + 1;
			}
		} /*for (i = 0; i < nvars; i++)*/
		iw[nvars] = nv;
	} /*if (symhIsSet(ForcedKey))*/

	if(nforced == nvars)
	{
		sprintf(OUTSTR,
				"ERROR: all variables forced in, no screening possible");
		goto errorExit;
	}
	
	if (Keep != 0 &&
		(Result = initKeep(Keep, imethod, mbest, nforced)) == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	

	if (!getScratch(regxH, GREGX, notmissing * (nvars + 1), double))
	{
		goto errorExit;
	}
	
	misswts = (MODELTYPE & MISSWEIGHTS) ? *MISSWTS : (double *) 0;
	regx1 = *regxH;
	for (i2 = 0; i2 <= nvars; i2++)
	{
		x = (i2 < nvars) ? *X[i2] : *Y;		
		j = 0;
		for (i =0; i < ndata; i++)
		{
			if (misswts == (double *) 0 || misswts[i] != 0.0)
			{
				regx1[j++] = x[i];
			}
		} /*for (i =0; i < ndata; i++)*/
		regx1 += notmissing;
	} /*for (i2 == 0; i2 <= nvars; i2++)*/

	nr = 2 * mbest * nvars + 7 * nd;
	if (!getScratch(rwH, GRW, nr, double))
	{
		goto errorExit;
	}
	rw = *rwH;
	/* take out means */
	regx1 = *regxH;
	for (i = 0; i <= nvars; i++)
	{
		tmpd = 0.0;

		for (k = 0; k < notmissing; k++)
		{
			tmpd += regx1[k];
		}
		tmpd /= notmissing;
		rw[i] = tmpd;
		for (k = 0; k < notmissing; k++)
		{
			regx1[k] -= tmpd;
		}
		regx1 += notmissing;
	} /*for (i = 0; i <= nvars; i++)*/

	/* set up centered cross products matrix */
	i = (3 * (nvars + 1) * (nvars + 1)) * sizeof(double);
	if (!getScratch(rtH, GRT, i, double))
	{
		goto errorExit;
	}
	rt = *rtH;
	
	regx1 = *regxH;
	for (i = 0; i <= nvars; i++)
	{
		ij = i + (nvars + 1) * i; /* i + (nvars+1) * j */
		ji = ij ; /* j + (nvars+1)*i */
		regx2 = regx1;
		for (j = i; j <= nvars; j++)
		{
			rt[ji] = rt[ij] = DDOT(&notmissing, regx1, &inc1, regx2, &inc1);
			ij += nvars + 1;
			ji++;
			regx2 += notmissing;
		} /*for (j = i; j <= nvars; j++)*/
		regx1 += notmissing;
	} /*for (i = 0; i <= nvars; i++)*/

	if(interrupted(DEFAULTTICKS) != INTNOTSET)
	{
		goto errorExit;
	}

	if(Verbose)
	{
		sprintf(OUTSTR,"Model used is %s=",*DEPVNAME);
		outstr = OUTSTR + strlen(OUTSTR);
		tmps = *STRMODEL;
		/* *STRMODEL should always be of the form "depv=1+..." */
		while(*tmps++ != '+')
		{
			;
		}
		sprintf(outstr, "%s", tmps);
		putOUTSTR();

		if (nforced > 0)
		{
			iw = *iwH;
			sprintf(OUTSTR, "%ld variables were forced:  ", nforced);
			myprint(OUTSTR);
			for (i = 0; i < nforced; i++)
			{
				sprintf(OUTSTR, "%s  ", VARNAME(iw[i]));
				myprint(OUTSTR);
			}
			myeol();
		} /*if (nforced > 0)*/

		if (imethod == CPMETHOD)
		{
			strcpy(OUTSTR, "Error variance set to ");
			outstr = OUTSTR + strlen(OUTSTR);
			
			if (s2 > 0.0)
			{
				outstr += formatDouble(outstr, s2, TRIMLEFT);
			}
			else
			{
				outstr += formatChar(outstr, "full model mse", CHARASIS);
			}
			outstr += formatChar(outstr, ", penalty factor is ", CHARASIS);
			outstr += formatDouble(outstr, penalty, TRIMLEFT);
			putOUTSTR();
		} /*if (imethod == CPMETHOD)*/
	} /*if(Verbose)*/
	
	if (!leapsscreen(&nv, &it, &nvars, &nforced, &notmissing,
					 &imethod, &penalty,
					 &mbest, *rtH, &nd, &nc,
					 *iwH, &nw, *rwH, &nr, &tl, &s2, &ne, &iv))
	{
		*OUTSTR = '\0';
		goto errorExit;
	}

	*OUTSTR = '\0';
	emptyTrash();
	glmcleanup(0);

	return ((Result != (Symbolhandle) 0) ? Result : NULLSYMBOL);

  errorExit:
	putErrorOUTSTR();
	
	emptyTrash();
	Removesymbol(Result);
	Result = (Symbolhandle) 0;
	glmcleanup(0);
	return (0);
} /*screen()*/

/*
   ind[]       Vector of indices of independent followed by dependent variable
               in current model
   *mn         number of variables in model
   *rm         full model MSE
   *ib         criterion: 1: r2, 2: adjr2, 3: cp
   *pen        penalty for cp
   *tss        total sum of squares
   *df         d.f. in *tss
*/
long leapsqprint(long * ind, long * mn, double * rm, double * tss,
				 double * s2, double * scale, double * df,
				 long * it, long * ib, double * pen)
{
	long            i, p;
	double          rss, adjr2, r2, cp;
	int             starter, length;
	WHERE("leapsqprint");
	

	if(*ib == RSQMETHOD)
	{
		rss = *rm;
	}
	else if(*ib == ADJRSQMETHOD)
	{
		rss = (*df - *mn) * *rm;
	}
	else /*CPMETHOD*/
	{
		rss = *rm - *pen * *mn * *s2;
	}
	r2 = 1. - rss / *tss;
	adjr2 = 1. - *df * rss / (*tss * (*df - *mn));
	p = *mn + *it;
	cp = rss / *s2 + *pen * p - (*df + *it);

	if(Verbose)
	{		
		if (Verbose == 1)
		{
			putOutMsg("  p    C(p)  Adj R^2  R^2    Model");
			Verbose = 2;
		}
		
		sprintf(OUTSTR,"%3ld %8.3f %6.4f %6.4f",p,cp,adjr2,r2);
		myprint(OUTSTR);
	
		length = starter = strlen(OUTSTR);
	
		for (i = 0; i < *mn; i++)
		{
			sprintf(OUTSTR, " %s", VARNAME(ind[i]));
			myprint(OUTSTR);
			if(i < *mn - 1 &&
			   (length += strlen(OUTSTR)) >= SCREENWIDTH - NAMELENGTH)
			{
				myeol();
				length = starter;
				sprintf(OUTSTR,"%*s",starter," "); /* NOTE use of %*s */
				myprint(OUTSTR);
			}
		}
		myeol();
	} /*if(Verbose)*/

	if(Keep && !screenDoKeep(Keep, Result,
							 (double) p, cp, r2, adjr2, ind, *mn))
	{
		goto errorExit;
	}
	
	*OUTSTR = '\0';
	return (1);

  errorExit:
	*OUTSTR = '\0';
	return (0);
	
} /*leapsqprint()*/

/*	  
	  * ******************************************************************** *
	  *                                                                      *
	  *                   Regressions by Leaps and Bounds                    *
	  *                     G.M.Furnival and R.W.Wilson                      *
	  *                           Version 8/10/81                            *
	  *                                                                      *
	  * call screen(nv,it,kx,nf,no,ib,fd,mb,rt,nd,nc,iw,nw,rw,nr,tl,s2,ne,iv)*
	  *                                                                      *
	  *  nv = number of rows and columns in input matrix (nv.le.m where m    *
	  *       is the number of bits in the fractional part of the real       *
	  *       numbers employed in the computations)                          *
	  *  it = type of input matrix (0=products about 0, 1=products about     *
	  *       means, 2=correlation matrix)                                   *
	  *  kx = number of independent variables (kx.lt.nv)                     *
	  *  nf = number of indepent variables to be forced i.e. included in     *
	  *       every subset (nf.lt.kx)                                        *
	  *  no = number of observations (no.gt.kx when it=0, no.gt kx+1         *
	  *       otherwise)                                                     *
	  *  ib = selection criterion code (1=r**2, 2=adjusted r**2, 3=mallows   *
	  *       c(p) with frane's variable penalty)                            *
	  *  fd = variable penalty or f-to-delete for c(p). a value of 2.0       *
	  *       gives mallows original c(p). (a value of fd .le.0 is replaced  *
	  *       by 2.0)                                                        *
	  *  mb = number of best regressions desired-for each size subset when   *
	  *       ib=1, in total when ib.gt.1                                    *
	  *  rt = real two-dimensional array. first nv columns contain input     *
	  *       matrix of products or correlations. rest of array is working   *
	  *       storage.                                                       *
	  *  nd = first dimension of rt(nd.ge.nv)
	  *  nc = second dimension of rt (nc.ge.3*nd)                            *
	  *  iw = integer one-dimensional array. first kx+1 elements contain     *
	  *       subscripts of variables to be used in problem with subscripts  *
	  *       of forced variables in first nf elements, subscripts of free   *
	  *       variables in next kx-nf elements and subscript of dependent    *
	  *       variable in element kx+1. rest of array is working storage.    *
	  *  nw = dimension of iw (nw.ge.4*nd)                                   *
	  *  rw = real one dimensional array. when it=1,first nv elements must   *
	  *       contain vector of means. rest of array is working storage.     *
	  *  nr = dimension of rw (nr.ge.2*mb*kx+7*nd if ib=1, nr.ge.2*mb+7*nd   *
	  *       if ib.gt.1)                                                    *
	  *  tl = minimum allowable tolerance (tl.lt.kx*kx/2**(m/2) or.ge.1      *
	  *       reset to kx*kx/2**(m/2))                                       *
	  *  s2 = estimated variance of residuals for c(p) (value.le.0 replaced  *
	  *       by mean square residual from full model)                       *
	  *  ne = error code (1=input parameter out of bounds, 2=invalid list    *
	  *       of subscripts in iw, 3=zero or negative diagonal in input      *
	  *       matrix rt, 4=set of forced variables nearly collinear,         *
	  *       5=full model nearly collinear-s2 must be supplied for c(p),    *
	  *       6=r**2 so close to 1 (r**2.gt.1-2**(-m/2)) that selection of   *
	  *       best subsets is uncertain.                                     *
	  *  iv = version of algorithm (0=partial reordering, 2=full reordering) *
	  *                                                                      *
	  * ******************************************************************** *
	  *  changes in screen specific to multreg are indicated
	  *   by the presence of the logical word 'uofm', which is
	  *   always set to 'true'.  most code changes are in
	  *   format statements, usually e-format changes to g-format,
	  *   indicated by comment statements.
	  *************************************************************************
*/

#define nv (*NV)
#define it (*IT)
#define kx (*KX)
#define nf (*NF)
#define no (*NO)
#define ib (*IB)
#define fd (*FD)
#define mb (*MB)
#define nd (*ND)
#define nc (*NC)
#define nw (*NW)
#define nr (*NR)
#define tl (*TL)
#define s2 (*S2)
#define ne (*leapsNE)
#define iv (*IV)
#define rt(i,j) ( RT[(i-1) + nd*(j-1)] )
#define rw(i)  ( RW[i-1] )
#define iw(i) ( IW[i-1] )

/* converted fortran, send proper pointer for array element in function call */
#define prt(i,j) ( RT + (i-1) + nd*(j-1) )
#define prw(i)  ( RW + i-1 )
#define piw(i) ( IW + i-1 )
/*
  This is an interface module to set up the call to leapsforwd().  It checks
  parameters and prints certain error messages
  Three pieces of RT, nine pieces of RW and four pieces of IW are passed as
  separate arguments to leapsforwd()
*/
long leapsscreen(long * NV, long * IT, long * KX, long * NF, long * NO,
				 long * IB, double * FD, long * MB, double RT [], long * ND,
				 long * NC, long IW [], long * NW, double RW [], long * NR,
				 double * TL, double * S2, long * leapsNE, long * IV)
/* 	  dimension rt(nd,nc),iw(nw),rw(nr) */
{
	long            nrm, ns, longparms[15];
	WHERE("leapsscreen");

	ne = 0;
	ns = (ib == RSQMETHOD) ? mb * kx : mb;

	if (0 > nf ||
		nf >= kx ||
		kx >= nv ||
		nv > nd ||
		kx + (1 > it ? it : 1) >= no ||
		nc < 3 * nd ||
		nw < 4 * nd ||
		nr < 2 * ns + 7 * nd ||
		ib < RSQMETHOD || ib > CPMETHOD ||
		it < 0 || it > 2 ||
		mb <= 0 ||
		iv < 0 || iv > 1
		)
	{
		ne = 1;					/* parameter out of bounds */
	}
	else
	{
		nrm = 7 * nd + ns + 1;
		longparms[0] = ns;
		longparms[1] = *ND;
		longparms[2] = *KX;
		longparms[3] = *NO;
		longparms[4] = *IT;
		longparms[5] = *NF;
		longparms[6] = *IB;
		longparms[7] = *MB;
		longparms[8] = *NV;
		if(!leapsforwd(longparms,
					   prt(1, 1), prt(1, nd + 1), prt(1, 2 * nd + 1),
					   prw(1), prw(nd + 1),	prw(2 * nd + 1), prw(3 * nd + 1),
					   prw(4 * nd + 1), prw(5 * nd + 1), prw(6 * nd + 1),
					   prw(7 * nd + 1), prw(nrm),
					   piw(1), piw(nd + 1), piw(2 * nd + 1), piw(3 * nd + 1),
					   S2, TL, FD, leapsNE, IV))
		{
			goto errorExit;
		}
	}

	if (ne > 0)
	{
		switch ((int) ne)
		{
			/* The '(internal to screen)' messages should not occur */
		  case 1:
			putOutErrorMsg("ERROR: (internal to screen) screen input parameters out of bounds");
			break;
		  case 2:
			putOutErrorMsg("ERROR: (internal to screen) invalid list of iw subscripts");
			break;
		  case 3:
			putOutErrorMsg("ERROR: (internal to screen) negative diagonal");
			break;
		  case 4:
			putOutErrorMsg("ERROR: set of forced variables apparently collinear");
			break;
		  case 5:
			putOutErrorMsg("ERROR: full model apparently collinear");
			break;
		  case 6:
			putOutErrorMsg("ERROR: r^2 so close to 1 that subset selection is uncertain");
			break;
		  default:
			;
		} /*switch ((int) ne)*/
		goto errorExit;
	} /*if (ne > 0)*/

	return (1);
	
  errorExit:
	return (0);
} /*leapsscreen()*/
