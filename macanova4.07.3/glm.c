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
#pragma segment Glm
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
#include "keywords.h"

/*
   950428   Changed the way keywords are parsed (added function getKeyValues)
            and struct keyinfo Keys[] and enabled keywords fstats and pvalues

   951012   Added keyword trunc to robust() and introduced globals
            RobTruncation and RobBeta, the latter being computed from the
			former.

   960104   Changed to use of getAllKeyValues() instead of having explicit loop

   990212   Changed putOUTSTR() to putErrorOUTSTR()
   990215   Changed myerrorout() to putOutMsg()
*/
/* 
  Front end for functions ('weights' is a synonym for 'wts'); most can have
  silent:T, print:F, and/or coef:T
	anova(model [,wts:vector] [,fstats:T] [,pvals:T] [,marginal:T])
	fastanova(model [,fstats:T] [,pvals:T])
	ipf(model [,inc:T] [,maxit:n] [, epsilon:value] [,pvals:T])
	logistic(model, denom [,offset:vector] [,inc:T] [,maxit:n]
				[, epsilon:value] [,pvals:T])
	manova(model [,wts:vector] [,fstats:T] [,pvals:T] [,marginal:T]) 
	poisson(model [,offset:vector] [,inc:T] [,maxit:n] [, epsilon:value]
	             [,pvals:T])
	regress(model [,wts:vector] [,fstats:T] [,pvals:T])
	robust(mode, [inc:T] [,maxit:n] [, epsilon:value] [,marginal:T])
	screen(model[,keep:charVec,print:T|F,mbest:ival,forced:vector,
		method:string,s2:posval,penalty:posval])
	wtanova(model,wts [,fstats:T] [,pvals:T] [,marginal:T])
	wtmanova(model,wts [,fstats:T] [,pvals:T] [,marginal:T])
	wtregress(model,wts [,fstats:T] [,pvals:T])
*/
typedef struct opEntry
{
	char          *name;
	short          iop;
	short          nargs;
	unsigned long  control;
} opEntry;

static opEntry GlmOps[] =
{ /* name,       iop,         nargs, control     
                                     arguments & keywords     */
	{"anova",     ANOVA,          -8, NORMALDIST | IDENTLINK},
                                   /*model,print|silent,wts,coefs,unbal,
								     fstats,pvalues,marginal*/
	{"fastanova", FASTANOVA,      -6, NORMALDIST | IDENTLINK},
									/*model,print|silent,maxiter,eps,
									  fstats,pvalues*/
	{"glmfit",    GLMREG,         -12, UNBALANCED},
									/*model,print|silent,offset,n,
									  maxiter,eps,inc,coefs,pvalues,
									  link,dist,scale*/
	{"ipf",       IPF,             -6,  POISSONDIST | LOGLINK},
									/*model,print|silent,maxiter,eps,inc,
									  pvalues*/
	{"logistic",  LOGITREG,        -8,  BINOMDIST | LOGITLINK | UNBALANCED},
									/*model,n,print|silent,offset,maxiter,
									  eps,inc,coefs,pvalues*/
	{"manova",    MANOVA,          -7,  NORMALDIST | IDENTLINK | UNBALANCED |
										MULTIVAR},
									/*model,print|silent,wts,coefs,
									  fstats,pvalues,marginal*/
	{"poisson",   POISSONREG,      -8,  POISSONDIST | LOGLINK | UNBALANCED},
									/*model,print|silent,offset,maxiter,
									  eps,inc,coefs,pvalues*/
	{"probit",    PROBITREG,       -8,  BINOMDIST | PROBITLINK | UNBALANCED},
									/*model,print|silent,offset,maxiter,
									  eps,inc,coefs,pvalues*/
	{"regress",   OLSREG,          -4,  NORMALDIST | IDENTLINK | UNBALANCED},
									/*model,print|silent,wts,pvalues*/
	{"robust",    ROBUSTREG,       -8,  IDENTLINK | UNBALANCED},
									/*model,print|silent,maxiter,eps,inc,
                                      fstats,pvals,marginal*/
	{"screen",    LEAPS,           -8,  NORMALDIST | IDENTLINK | UNBALANCED},
									/*model,print,keep,mbest,force,method,
									  s2|ssq,penalty*/
	{"wtanova",   ANOVA | CASEWEIGHTS, -7, NORMALDIST | IDENTLINK | UNBALANCED},
									/*model,wts,print|silent,coefs,
									  fstats,pvalues,marginal*/
	{"wtmanova",  MANOVA | CASEWEIGHTS, -7, NORMALDIST | IDENTLINK | UNBALANCED|
											MULTIVAR},
									/*model,wts,print|silent,coefs,
									  fstats,pvalues,marginal*/
	{"wtregress", OLSREG | CASEWEIGHTS, -4, NORMALDIST | IDENTLINK | UNBALANCED},
									/*model,wts,print|silent, pvalues*/
	{(char *) 0,  0,                0,   0}
};


static long            FactorFound = 0;

#if (0)  /*951012 moved into glm.h*/
/* default constants*/
#define FASTANOVAMAXITER  25
#define FASTANOVAEPSILON  1e-6
#define ITERGLMMAXITER    50
#define ITERGLMEPSILON    1e-6
#define IPFMAXITER        25
#define IPFEPSILON        1e-6
#endif /*(0)*/

enum glmKeyCodes
{
	KPRINT = 0,
	KSILENT,
	KCOEFS,
	KFSTATS,
	KPVALS,
	KUNBAL,
	KMAXIT,
	KEPSILON,
	KPROBLIMIT,
	KTRUNC,
	KWTS,
	KWEIGHTS,
	KOFFSETS,
	KDENOM,
	KPARAM,
	KINCR,
	KSSSP,
	KSSCP,
	KBYVAR,
	KLINK,
	KDISTR,
	KSCALE,
	KMARGINAL
};

static keywordList    GlmKeys[] =
{
	InitKeyEntry("print", 0, ALLGLMS, LOGICSCALAR),
	InitKeyEntry("silent", 0, (ALLGLMS) & ~LEAPS, LOGICSCALAR),
	InitKeyEntry("coefs", 4,
				 (ALLGLMS) & ~(LEAPS|OLSREG|FASTANOVA |IPF|ROBUSTREG),
				 LOGICSCALAR),
	InitKeyEntry("fstats", 5, FSTATSOK | ROBUSTREG, LOGICSCALAR),
	InitKeyEntry("pvals", 4, ALLGLMS & ~LEAPS, LOGICSCALAR),
	InitKeyEntry("unbalanced", 5, ANOVA | MANOVA | OLSREG, LOGICSCALAR),
	InitKeyEntry("maxit", 0, ITERGLMS | FASTANOVA | IPF, POSITIVEINT),
	InitKeyEntry("epsilon", 3, ITERGLMS | FASTANOVA | IPF, POSITIVEREAL),
	InitKeyEntry("problimit", 0, ITERGLMS | BINOMREG, POSITIVEREAL),
	InitKeyEntry("truncate", 5, ROBUSTREG, POSITIVEREAL),
	InitKeyEntry("wts", 2, ANOVA | MANOVA | OLSREG, REALVECTOR),
	InitKeyEntry("weights", 6, ANOVA | MANOVA | OLSREG, REALVECTOR),
	InitKeyEntry("offsets", 6, (ITERGLMS) & ~ROBUSTREG, REALVECTOR),
	InitKeyEntry("n", 0, BINOMREG | GLMREG, REALVECTOR),
	InitKeyEntry("parameters", 5, BINOMREG | GLMREG, REALVECTOR),
	InitKeyEntry("increment", 3, ITERGLMS | IPF, LOGICSCALAR),
	InitKeyEntry("sssp", 0, MANOVA, LOGICSCALAR),
	InitKeyEntry("sscp", 0, MANOVA, LOGICSCALAR),
	InitKeyEntry("byvariable", 5, MANOVA, LOGICSCALAR),
	InitKeyEntry("link", 0, GLMREG, CHARSCALAR),
	InitKeyEntry("distrib", 4, GLMREG, CHARSCALAR),
	InitKeyEntry("scale", 0, GLMREG | BINOMREG | POISSONREG, REALSCALAR),
	InitKeyEntry("marginal", 4,(FSTATSOK & (~FASTANOVA)) | ROBUSTREG | OLSREG,
								LOGICSCALAR)
};

/* Values in array GlmKeys can be referred to by these macros*/
#define         PrintResults KeyLogValue(GlmKeys,KPRINT)
#define         Silent       KeyLogValue(GlmKeys,KSILENT)
#define         Docoefs      KeyLogValue(GlmKeys,KCOEFS)
#define         Fstats       KeyLogValue(GlmKeys,KFSTATS)
#define         Pvals        KeyLogValue(GlmKeys,KPVALS)
#define         Unbalanced   KeyLogValue(GlmKeys,KUNBAL)
#define         Maxitval     KeyIntValue(GlmKeys,KMAXIT)
#define         Epsilon      KeyRealValue(GlmKeys,KEPSILON)
#define         Problimit    KeyRealValue(GlmKeys,KPROBLIMIT)
#define         Trunc        KeyRealValue(GlmKeys,KTRUNC)
#define         SymhWts      KeySymhValue(GlmKeys,KWTS)
#define         SymhWts1     KeySymhValue(GlmKeys,KWEIGHTS)
#define         SymhOffset   KeySymhValue(GlmKeys,KOFFSETS)
#define         SymhN        KeySymhValue(GlmKeys,KDENOM)
#define         SymhParam    KeySymhValue(GlmKeys,KPARAM)
#define         IncVal       KeyLogValue(GlmKeys,KINCR)
#define         Sssp         KeyLogValue(GlmKeys,KSSSP)
#define         Sscp         KeyLogValue(GlmKeys,KSSCP)
#define         Byvar        KeyLogValue(GlmKeys,KBYVAR)
#define         Link         KeyCharValue(GlmKeys,KLINK)
#define         Dist         KeyCharValue(GlmKeys,KDISTR)
#define         Scale        KeyRealValue(GlmKeys,KSCALE)
#define         Marg         KeyLogValue(GlmKeys,KMARGINAL)

#ifdef MACINTOSH
static void unloadGlmSegs(void)
{
#ifdef SEGMENTED
	UNLOADSEG(getmodel);
	UNLOADSEG(screen);
	UNLOADSEG(ipf);
	UNLOADSEG(funbalanova);
	UNLOADSEG(iterglm);
	UNLOADSEG(Cnor);
	UNLOADSEG(unbalanova);
	UNLOADSEG(printregress);
	UNLOADSEG(printglm);
	UNLOADSEG(gramschmidt);
	UNLOADSEG(getterm); /*in segment Glmutils*/
#endif /*SEGMENTED*/
}
#endif /*MACINTOSH*/

static int glmImplemented(unsigned long control)
{
	if (control & POISSONDIST)
	{
		if (!(control & (LOGLINK)))
		{
			goto errorExit;
		}
	}
	else if (control & BINOMDIST)
	{
		if (!(control & (LOGITLINK | PROBITLINK)))
		{
			goto errorExit;
		}
	}
	else if (control & NONNORMALDIST)
	{
		goto errorExit;
	}
	else if (control & NORMALDIST)
	{
		if (!(control & IDENTLINK))goto errorExit;
	}
	
	return (1);
	
  errorExit:
	sprintf(OUTSTR,
			"ERROR: %s(Model,%s:\"%s\",%s:\"%s\") is not implemented",
			FUNCNAME, KeyName(GlmKeys, KLINK), linkName(control),
			KeyName(GlmKeys, KDISTR), distName(control));
	putErrorOUTSTR();
	return (0);
} /*glmImplemented()*/

/*
  971104 added check for value of myhandlelength()
  971208 Slightly modified some messages (adding '()' after function names)
         and adding warning when there is a factor in a screen() model
  971230 Cleaned up handling of default values for Pvals and Fstats
  980804 Changed name of keyword 'clamp' to 'problimit'; added lower limit
         for it
  980810 Fixed bug that sometimes inappropriately attached labels to
         various side effect variables
  990318 SS always has labels.  For all functions except manova(),
         they are same as TERMNAMES.  For manova() with ny > 1, the
         labels for the first dimension are the same as TERMNAMES.
         The last two dimensions have identical labels, either the column
         labels of y if it has any, or vector("(1)","(2)", .. ),
         vector("[1]","[2]", .. ), ..., the actual bracket used being the
         current value of option 'labelstyle' 
  990319 Localized declarations of Symbolhandles for side effect variables
         Moved creation of side effect variables DEPVNAME and TERMNAMES
         from getmodel() to glm()

         On iterative GLM commands without increment:T, TERMNAMES is
         set to vector(rep("",nterms-1),"Overall model","ERROR1") and this is
         also used to label SS.  In addition, side effect variable
		 DF is of the form vector(rep(0,nterms-1),modelDf, ErrorDF) so
         that it contains the degrees of freedom for the quantities in SS.
*/
Symbolhandle    glm(Symbolhandle list)
{
	/* control  routine for glms */

	Symbolhandle    symh, symh1;
	Symbolhandle    symTERMNAMES;
	char           *keyword = (char *) 0;
	char          **tmpSTRMODEL = (char **) 0;
	char           *charValue;
	long            i, j;
	long            nargs = NARGS(list), margs;
	long            nvars, ndata, nregx, ny, nterms;
	long            mdatay, mdatax, mdatass, mdataxx;
	long            usePrevious = 0, notInstalled = 0;           
	long            prevType = 0, prevControl = 0;
	long            startKey;
	long            nkeys = NKeys(GlmKeys);
	keywordListPtr  keys = GlmKeys;
	double          prevScale;
	unsigned long   glmcontrol;
	unsigned long   linkcode, distcode;
	int             reply = ITERCONVERGED, doCleanup = 0;
	int             yHaslabels, yEvaluated;
	short           keyStatus;
	opEntry        *op;
	double         *residuals;
	char           *plural = "s";
	char           *factorIsVariable =
	  "WARNING: factor%s in model treated as variate%s by %s()";
	WHERE("glm");
	TRASH(1,errorExit); /* room for 1 scratch handle */
	
#ifdef MW_CW_PROFILER
/*	ProfilerSetStatus(1);*/
#endif /*MW_CW_PROFILER*/

	if (RUNNINGGLM)
	{
		sprintf(OUTSTR,
				"ERROR: recursive use of %s() from %s()", FUNCNAME,
				RUNNINGGLM);
		goto errorExit;
	} /*if (RUNNINGGLM)*/
	RUNNINGGLM = FUNCNAME;
	
	/* preserve previous things */
	prevType = PREVMODELTYPE;
	prevControl = PREVGLMCONTROL;
	prevScale = PREVGLMSCALE;

	MODELTYPE = NOMODEL;

	*OUTSTR = '\0';
	
	/* determine model type from FUNCNAME */
	for(op = GlmOps; op->iop != 0; op++)
	{
		if(strcmp(FUNCNAME,op->name) == 0)
		{
			MODELTYPE = op->iop;
			margs = op->nargs;
			glmcontrol = op->control;
			break;
		} /*if(strcmp(FUNCNAME,op->name) == 0)*/
	} /*for(op = GlmOps; op->iop != 0; op++)*/

	startKey = (MODELTYPE & (CASEWEIGHTS | BINOMREG) && 
				nargs > 1 && !isKeyword(COMPVALUE(list,1))) ? 2 : 1;
		
	if(nargs > labs(margs))
	{
		badNargs(FUNCNAME,margs);
		goto errorExit;
	}

	/* Initialize default keyword values */
	unsetKeyValues(keys, nkeys);

	if (!(MODELTYPE & LEAPS))
	{
		setLogicalTrue(PrintResults);
	}

	/* pick off keyword values */
	keyStatus = getAllKeyValues(list, startKey, MODELTYPE, keys, nkeys);
	if (keyStatus < 0)
	{
		goto errorExit;
	}
	nargs -= keyStatus;
	
	if ((MODELTYPE & GLMREG) && !charIsSet(Dist))
	{
		glmcontrol |= NORMALDIST;
	}
	
	if (logicalIsSet(Silent))
	{
		PrintResults = !Silent;
	}
	else
	{
		Silent = 0;
	}

	if (MODELTYPE & MANOVA)
	{
		if (logicalIsSet(Sssp) || logicalIsSet(Sscp))
		{
			keyword = KeyName(keys, KSSSP);
			if (!logicalIsSet(Sssp))
			{
				Sssp = Sscp;
				keyword = KeyName(keys, KSSCP);
			}
			
			if (!PrintResults)
			{
				if (Sssp)
				{
					sprintf(OUTSTR, "WARNING: %s:T ignored with %s:F or %s:T",
							keyword, KeyName(keys, KPRINT),
							KeyName(keys, KSILENT));
					putErrorOUTSTR();
				}
				Sssp = 0;
			}
			if (logicalIsSet(Fstats) && Fstats ||
				logicalIsSet(Pvals) && Pvals ||
				logicalIsSet(Byvar) && Byvar)
			{
				sprintf(OUTSTR,
						"WARNING: %s:T ignored with %s:T, %s:T or %s:T",
						KeyName(keys, KSSSP), KeyName(keys, KFSTATS),
						KeyName(keys, KPVALS), KeyName(keys, KBYVAR));
				putErrorOUTSTR();
				Sssp = 0;
			}
		} /*if (logicalIsSet(Sssp))*/
		else
		{
			Sssp = -1;
		}
	} /*if (MODELTYPE & MANOVA)*/

	Byvar = (logicalIsSet(Byvar)) ? Byvar : 0;

	Docoefs = (logicalIsSet(Docoefs)) ? Docoefs : 1;
	
	Docoefs = (Docoefs) ? DOCOEFS : DONTDOCOEFS;

	if (logicalIsSet(Marg) && Marg)
	{
		if (!Docoefs)
		{
			sprintf(OUTSTR,
					"ERROR: you can't use %s:F with %s:T",
					KeyName(keys, KCOEFS), KeyName(keys, KMARGINAL));
			goto errorExit;
		}
		glmcontrol |= MARGINALSS;
	} /*if (logicalIsSet(Marg) && Marg)*/
	
	if (!PrintResults && !Silent &&
		(logicalIsSet(Fstats) && Fstats || logicalIsSet(Pvals) && Pvals))
	{
		sprintf(OUTSTR,
				"WARNING: %s:T and/or %s:T ignored with %s:F or %s:T",
				KeyName(keys, KFSTATS), KeyName(keys, KPVALS),
				KeyName(keys, KPRINT), KeyName(keys, KSILENT));
		putErrorOUTSTR();
		Fstats = Pvals = 0;
	}
	if (MODELTYPE & MANOVA)
	{
		if (logicalIsSet(Fstats) && Fstats || logicalIsSet(Pvals) && Pvals ||
			Byvar)
		{
			if (logicalIsSet(Sssp) && Sssp)
			{
				sprintf(OUTSTR,
						"WARNING: %s:T ignored with %s:T, %s:T or %s:T",
				KeyName(keys, KSSSP), KeyName(keys, KFSTATS),
				KeyName(keys, KPVALS), KeyName(keys, KBYVAR));
				putErrorOUTSTR();
			}
			Sssp = 0;
		}
		else
		{
			Sssp = (logicalIsSet(Sssp)) ? Sssp : -1;
		}
	} /*if (MODELTYPE & MANOVA)*/

	if (logicalIsSet(Unbalanced))
	{
		if (MODELTYPE & ANOVA)
		{
			glmcontrol |= (Unbalanced) ? UNBALANCED : 0;
		}
		else if(!Unbalanced)
		{
			sprintf(OUTSTR,
					"ERROR: %s:F illegal on %s()", KeyName(keys, KUNBAL),
					FUNCNAME);
			goto errorExit;
		}
	} /*if (logicalIsSet(Unbalanced))*/
	
	
	if (realIsSet(Scale))
	{
		if(!isMissing(Scale) && Scale <= 0.0)
		{
			sprintf(OUTSTR,
					"ERROR: %s must be positive REAL or MISSING",
					KeyName(keys, KSCALE));
			goto errorExit;
		}
	} /*if (realIsSet(Scale))*/
	else
	{
		Scale = 1.0;
	}
	
/*
   Both "wts" and "weights" are legal to specify weights.  If both were
   used it is an error
*/
	if (symhIsSet(SymhWts) && symhIsSet(SymhWts1))
	{
		sprintf(OUTSTR,
				"ERROR: weights for %s() were specified twice; don't know which to use",
				FUNCNAME);
		goto errorExit;
	} /*if (symhIsSet(SymhWts) && symhIsSet(SymhWts1))*/
	
	SymhWts = (symhIsSet(SymhWts1)) ? SymhWts1 : SymhWts;
	if (symhIsSet(SymhWts))
	{
		MODELTYPE |= CASEWEIGHTS;
		glmcontrol |= UNBALANCED;
	} /*if (symhIsSet(SymhWts))*/
	else if(MODELTYPE & CASEWEIGHTS && nargs > 1 &&
			!isKeyword(COMPVALUE(list,1)))
	{
		SymhWts = COMPVALUE(list,1);
	}
	
	if (charIsSet(Link))
	{
		keyword = KeyName(keys, KLINK);
		charValue = *Link;
		linkcode = linkControl(charValue);
		if (linkcode > 0)
		{
			glmcontrol |= linkcode;
		}
		else
		{
			sprintf(OUTSTR,
					"ERROR: unrecognized value '%s' for keyword '%s'",
					charValue, keyword);
			goto errorExit;
		}		
	} /*if (charIsSet(Link))*/
	
	if (charIsSet(Dist))
	{
		keyword = KeyName(keys, KDISTR);
		charValue = *Dist;
		distcode = distControl(charValue);
		if (distcode > 0)
		{
			glmcontrol |= distcode;
		}
		else
		{
			sprintf(OUTSTR,
					"ERROR: unrecognized value '%s' for keyword '%s'",
					charValue, keyword);
			goto errorExit;
		}		
	} /*if (charIsSet(Dist))*/

	if (symhIsSet(SymhN) && symhIsSet(SymhParam))
	{
		sprintf(OUTSTR,
				"ERROR: illegal use both '%s' and '%s' on %s()",
				KeyName(keys,KDENOM), KeyName(keys,KPARAM),FUNCNAME);
		goto errorExit;
	} /*if (symhIsSet(SymhN) && symhIsSet(SymhParam))*/

	if (symhIsSet(SymhN) || symhIsSet(SymhParam))
	{
		keyword = (symhIsSet(SymhN)) ?
			KeyName(keys, KDENOM) : KeyName(keys, KPARAM);
		if (!(glmcontrol & BINOMDIST))
		{ /* must be glmfit()*/
			sprintf(OUTSTR,
					"ERROR: keyword '%s' illegal on %s() without %s:\"binomial\"",
					keyword, FUNCNAME, KeyName(keys,KDISTR));
			goto errorExit;
		}
			
		if (symhIsSet(SymhParam))
		{
			SymhN = SymhParam;
			SymhParam = (Symbolhandle) 0;
		}
	} /*if (symhIsSet(SymhN) || symhIsSet(SymhParam))*/

	if (MODELTYPE & BINOMREG && nargs > 1 && !symhIsSet(SymhN) &&
		!isKeyword(COMPVALUE(list, 1)))
	{
		SymhN = COMPVALUE(list, 1);
	}
	
	if (logicalIsSet(IncVal))
	{
		INCREMENTAL = IncVal;
		if (INCREMENTAL && MODELTYPE & ROBUSTREG && !Silent)
		{
			keyword = KeyName(keys, KINCR);

			sprintf(OUTSTR,
					"WARNING: '%s:T' no longer legal on %s(); ignored.",
					keyword, FUNCNAME);
			putErrorOUTSTR();
			INCREMENTAL = 0;
		} /*if (INCREMENTAL && MODELTYPE & ROBUSTREG && !Silent)*/
	} /*if (logicalIsSet(IncVal))*/
	else
	{
		INCREMENTAL = 0;
	}
	

	if (MODELTYPE & GLMREG && !(glmcontrol & ANYLINK))
	{ /* set default link */
		if (glmcontrol & BINOMDIST)
		{
			glmcontrol |= LOGITLINK;
		}
		else if (glmcontrol & POISSONDIST)
		{
			glmcontrol |= LOGLINK;
		}
		else if (glmcontrol & GAMMADIST)
		{
			glmcontrol |= RECIPLINK;
		}
		else if (glmcontrol & NORMALDIST)
		{
			glmcontrol |= IDENTLINK;
		}
	} /*if (MODELTYPE & GLMREG && !(glmcontrol & ANYLINK))*/
	
	if (MODELTYPE & GLMREG &&
		glmcontrol & NORMALDIST && glmcontrol & IDENTLINK)
	{ /* glmfit(model,link:"identity",distr:"normal") is anova(model)*/
		MODELTYPE = (MODELTYPE & ~GLMREG) | ANOVA;
	}
	
	if (realIsSet(Problimit) && !(glmcontrol & BINOMDIST))
	{
		sprintf(OUTSTR,
				"WARNING: '%s' ignored on %s() without '%s:\"binomial\"'",
				KeyName(keys, KPROBLIMIT), FUNCNAME, KeyName(keys, KDISTR));
		putErrorOUTSTR();
		unsetReal(Problimit);
	}
	
	if (!glmImplemented(glmcontrol))
	{
		goto errorExit;
	}
	
	/* get model */
	symh = COMPVALUE(list,0);
	symh1 = Lookup("STRMODEL");

	if(symh == (Symbolhandle) 0)
	{ /* argument 1 missing; use model in symbol STRMODEL, if it exists */
		if(symh1 == (Symbolhandle) 0)
		{
			sprintf(OUTSTR,
					"ERROR: no model specified for %s() and no previous model available",
					FUNCNAME);
			goto errorExit;
		}
	/* Note : PREVMODELTYPE is reset in Symbol.c if STRMODEL is assigned to*/

		/* do not recompute things in certain cases */
		if(nargs == 1 && NVARS >= 0 && SS != (double **) 0 && 
		   PREVMODELTYPE & OLSREG &&
		   MODELTYPE & ANOVA &&
		   (!logicalIsSet(Marg) ||
			(glmcontrol & MARGINALSS) == (PREVGLMCONTROL & MARGINALSS)) &&
			(!(glmcontrol & UNBALANCED) || (PREVMODELTYPE & CASEWEIGHTS)))
		{
			usePrevious = 1;
			glmcontrol = PREVGLMCONTROL;
			Marg = (glmcontrol & MARGINALSS) ? 1 : 0;
			if(PREVMODELTYPE & (CASEWEIGHTS | OLSREG))
			{
				MODELTYPE = ANOVA | (PREVMODELTYPE & (CASEWEIGHTS | DOCOEFS));
				glmcontrol |= UNBALANCED;
			}
		}
	}
	else if(TYPE(symh) != CHAR || !isScalar(symh))
	{
		sprintf(OUTSTR, "ERROR: model for %s() must be a single CHARACTER string",
				FUNCNAME);
		goto errorExit;
	}

	Marg = (logicalIsSet(Marg)) ? Marg : 0;
	
	if (realIsSet(Problimit) &&
		Problimit > 0.0001 || Problimit < 1e-15)
	{
		sprintf(OUTSTR,
				"ERROR: value for '%s' %s on %s()",
				KeyName(keys, KPROBLIMIT),
				(Problimit > 0.0001) ? "> 0.0001" : "< 1e-15",
				FUNCNAME);
		goto errorExit;
	}
		
	if (symh == (Symbolhandle) 0)
	{ /* use STRMODEL as current model */
		symh = symh1;
	}
	
	doCleanup = 1;

	if(!getScratch(tmpSTRMODEL,0,strlen(STRINGPTR(symh)) + 1,char))
	{
		goto errorExit;
	}
	strcpy(*tmpSTRMODEL,STRINGPTR(symh));

	if(!usePrevious)
	{
		glmcleanup(0);
		doCleanup = 0;
	}
	else
	{
		mydisphandle(STRMODEL);
	}
	
	if ((STRMODEL = setStrmodel(tmpSTRMODEL,1)) == (char **) 0)
	{
		doCleanup = 0;
		goto errorExit;
	}

/* *tmpSTRMODEL and *STRMODEL now contain the model without and with '1+' */

	if(!Silent && !(MODELTYPE & LEAPS))
	{
		myprint("Model used is ");
		putOutMsg(*tmpSTRMODEL);
	}
	emptyTrash(); /* get rid of tmpSTRMODEL */
	
/* Install *STRMODEL as variable STRMODEL */
	Remove("STRMODEL");
	symh = Install("STRMODEL", CHAR);
	if (symh == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	TMPHANDLE = mygethandle((long) strlen(*STRMODEL) + 1);
	setSTRING(symh,TMPHANDLE);
	if (TMPHANDLE == (char **) 0)
	{
		goto errorExit;
	}
	strcpy(STRINGPTR(symh), *STRMODEL);
	setNDIMS(symh,1);
	setDIM(symh,1,1);

	GLMCONTROL = glmcontrol;
	GLMSCALE = Scale;

	if (!usePrevious)
	{
		
		if (getmodel(list, nargs, startKey, Silent,
					 SymhWts, SymhOffset, SymhN) != 0)
		{
			goto errorExit;
		}

		/* For regression we ignore class structure */
		FactorFound = 0;
		nvars = (long) NVARS;
		for (i = 0; i < nvars; i++)
		{
			if(NCLASSES[i] > 0)
			{
				FactorFound++;
			}
			else if(MODELTYPE & FASTANOVA)
			{
				sprintf(OUTSTR,
						"ERROR: %s() cannot be used with variates in model",
						FUNCNAME);
				goto errorExit;
			}

			/* For regression we ignore class structure */
			if (MODELTYPE & OLSREG)
			{
				NCLASSES[i] = -1;
			}
		} /*for (i = 0; i < nvars; i++)*/
	} /*if (!usePrevious)*/
	
	if (MODELTYPE & LEAPS)
	{
		/* leaps does special things */
		Symbolhandle     result = screen(list, PrintResults);

		if (FactorFound)
		{
			sprintf(OUTSTR, factorIsVariable,
					(FactorFound > 1) ? plural : NullString,
					(FactorFound > 1) ? plural : NullString,
					FUNCNAME);
			putErrorOUTSTR();
		} /*if (FactorFound)*/
		
		RUNNINGGLM = (char *) 0;
		return (result);
	} /*if (MODELTYPE & LEAPS)*/

	ndata = (long) NDATA;
	ny = (long) NY;
	nvars = (long) NVARS;
	nterms = (long) NTERMS;

	if (ny == 1 && (MODELTYPE & MANOVA))
	{
		MODELTYPE &= ~MANOVA;
		MODELTYPE |= ANOVA;
		GLMCONTROL &= ~MULTIVAR;
	}
	if (nterms == 1 && !INCREMENTAL)
	{
		INCREMENTAL = 1;
	}
	if(MODELTYPE & ANOVA && !(GLMCONTROL & UNBALANCED))
	{ /* ignore coefs:F for balanced ANOVA */
		Docoefs = DOCOEFS;
	}
	MODELTYPE |= Docoefs;


	if (Marg && !(GLMCONTROL & UNBALANCED))
	{
		if (PrintResults)
		{
			sprintf(OUTSTR,
					"NOTE: %s:T ignored with balanced ANOVA",
					KeyName(keys, KMARGINAL));
			putErrorOUTSTR();
		}
		GLMCONTROL &= ~MARGINALSS;
		Marg = 0;
	} /*if (Marg && !(GLMCONTROL & UNBALANCED))*/
	/*
	   setting Fstats and Pvals is put off to here until we know whether
	   manova() should be treated as anova() since the defaults for
	   manova() are no Pvalues and no Fstats and are not changed by option
	   'fstats' or 'pvals'
	*/
	if (!logicalIsSet(Fstats))
	{
		if (!(MODELTYPE & FSTATSOK) || (MODELTYPE & MANOVA) && !Byvar)
		{
			Fstats = 0;
			if (!logicalIsSet(Pvals))
			{
				Pvals = (!(MODELTYPE & FSTATSOK)) ? PRINTPVALS : 0;
			}
		}
		else
		{
			Fstats = PRINTFSTATS;
			if (!logicalIsSet(Pvals))
			{
				Pvals = Fstats || PRINTPVALS;
			}
		 }
	} /*if (!logicalIsSet(Fstats))*/
	else if (!logicalIsSet(Pvals))
	{
		Pvals = Fstats;
	}
	
	Sssp = (MODELTYPE & MANOVA) ? Sssp : 0; 

	mdatay = ndata * ny; /* number of elements in dependent variable(s) */
	mdatass = ny * ny * (nterms + 1); /* number of elements in SS */

	for(i = 0;i<nargs;i++)
	{
		if((keyword = isKeyword(symh = COMPVALUE(list,i))) &&
		   strcmp(NAME(symh),USEDKEYWORD) != 0)
		{
			sprintf(OUTSTR,
					"ERROR: unrecognized or duplicate keyword %s in %s()",
					keyword, FUNCNAME);
			goto errorExit;
		}
	} /*for(i = 0;i<nargs;i++)*/

	if(MODELTYPE & CASEWEIGHTS && !Silent)
	{
		putOutMsg("Model fit by weighted least squares");
	}
	
	if(USEGLMOFFSET && !Silent)
	{
		putOutMsg("Vector of offsets used");
	}
	
	if(!usePrevious)
	{
		doCleanup = 1;

		/* allocate memory for GLM globals */
		SS = (double **) mygethandle(mdatass * sizeof(double));
		DF = (double **) mygethandle((nterms + 1) * sizeof(double));
		if (SS == (double **) 0 || DF == (double **) 0)
		{
			goto errorExit;
		}

		if(!(MODELTYPE & FASTANOVA))
		{
			RESIDUALS = (double **) mygethandle(mdatay * sizeof(double));
			HII = (double **) mygethandle(ndata * sizeof(double));
			if (RESIDUALS == (double **) 0 || HII == (double **) 0)
			{
				goto errorExit;
			}
			/* set residuals to Y*/
			doubleCopy(*Y, *RESIDUALS, mdatay);

#ifdef MISSINGISNAN
			/* zero y's with zero weights */
			if (MODELTYPE & MISSWEIGHTS)
			{
				long      i, j;
				double    *y = *RESIDUALS, *wts = *MISSWTS;;
				
				for (j = 0; j < ny; j++)
				{
					for (i = 0;i < ndata;i++)
					{
						if (wts[i] == 0.0)
						{
							y[i] = 0.0;
						}
					} /*for (i = 0;i < ndata;i++)*/
					y += ndata;
				} /*for (j = 0; j < ny; j++)*/
			} /*if (MODELTYPE & MISSWEIGHTS)*/			
#endif /*MISSINGISNAN*/
		} /*if(!(MODELTYPE & FASTANOVA))*/
		
		/* set up X matrix if needed */
		if (GLMCONTROL & UNBALANCED)
		{
			if (!Xsetup())
			{
				goto errorExit;
			}
			nregx = (long) NREGX;
#ifdef MISSINGISNAN
			if (MODELTYPE & MISSWEIGHTS)
			{
				double      *x = *REGX, *wts = *MISSWTS;
				long         i, j;
				
				/* fill every row of case with missing values with 0's*/
				for (j = 0; j < nregx; j++)
				{
					for (i = 0; i < ndata; i++)
					{
						if (wts[i] == 0.0)
						{
							x[i] = 0.0;
						}
					} /*for (i = 0; i < ndata; i++)*/
					x += ndata;
				} /*for (j = 0; j < nregx; j++)*/
			} /*if (MODELTYPE & MISSWEIGHTS)*/
#endif /*MISSINGISNAN*/
		} /*if (GLMCONTROL & UNBALANCED)*/
		else
		{
			nregx = 0;
			for(i=0;i<nterms;i++)
			{
				nregx += buildXmatrix(i,(double *) 0, MODELINFO);
			}
			NREGX = (double) nregx;
		} /*if (GLMCONTROL & UNBALANCED){}else{}*/

		mdatax = ndata * nregx; /* number of elements in indep. variable(s) */
		mdataxx = nregx * nregx; /* number of elements in XTXINV */

		if (MODELTYPE & (CASEWEIGHTS | ROBUSTREG) || GLMCONTROL & NONNORMALDIST)
		{
			WTDRESIDUALS = (double **) mygethandle(mdatay * sizeof(double));

			if (WTDRESIDUALS == (double **) 0)
			{
				goto errorExit;
			}
			if(!(MODELTYPE & IPF) && !(MODELTYPE & ROBUSTREG))
			{
				REGX2 = (double **) mygethandle(mdatax * sizeof(double));
				if (REGX2 == (double **) 0)
				{
					goto errorExit;
				}
				doubleCopy(*REGX, *REGX2, mdatax);
			} /*if(!(MODELTYPE & IPF))*/
		} /*if (MODELTYPE & (ITERGLMS | CASEWEIGHTS | IPF))*/

		if (MODELTYPE & (IPF | ANOVA) && !(GLMCONTROL & UNBALANCED))
		{
			Maxitval = (!longIsSet(Maxitval)) ? IPFMAXITER : Maxitval;
			Epsilon = (!realIsSet(Epsilon)) ? IPFEPSILON : Epsilon;
			if ((reply = ipf(Maxitval,Epsilon)) == ITERERROR)
			{
				goto errorExit;
			}
		} /*if (MODELTYPE & (IPF | ANOVA) && !(GLMCONTROL & UNBALANCED))*/
		else if (MODELTYPE & FASTANOVA)
		{
			Maxitval = (!longIsSet(Maxitval)) ? FASTANOVAMAXITER : Maxitval;
			Epsilon = (!realIsSet(Epsilon)) ? FASTANOVAEPSILON : Epsilon;
			
			/* 
			  NB: funbalanova changes NDATA and size of Y and RESIDUALS
			  These modified values are needed for predtable() and coef()
			  and should not be changed back.  However, ndata still reflects
			  the true length of the response vector.
			*/
			if ((reply = funbalanova(Maxitval,Epsilon)) == ITERERROR)
			{
				goto errorExit;
			}
		} /*else if (MODELTYPE & FASTANOVA)*/
		else if (MODELTYPE & ITERGLMS)
		{
			Maxitval = (!longIsSet(Maxitval)) ? ITERGLMMAXITER : Maxitval;
			Epsilon = (!realIsSet(Epsilon)) ? ITERGLMEPSILON : Epsilon;
			Problimit = (!realIsSet(Problimit)) ? BINOMPLIMIT : Problimit;
			if (MODELTYPE & ROBUSTREG)
			{
				Trunc = (!realIsSet(Trunc)) ? ROBTRUNCATION : Trunc;
			} /*if (MODELTYPE & ROBUSTREG)*/
			
			RobTruncation = Trunc;
			if ((reply = iterglm(Maxitval,Epsilon, Problimit)) == ITERERROR)
			{
				goto errorExit;
			}
		}/*else if (MODELTYPE & ITERGLMS)*/
		else
		{ /* regression, unbalanced anova, or manova() */
			if (unbalanova() < 1)
			{
				goto errorExit;
			}
		}
	} /*if(!usePrevious)*/

	checkInterrupt(errorExit);
	
	if(reply == ITERUNCONVERGED && !Silent)
	{
		sprintf(OUTSTR,
				"WARNING: %s() failed to converge fully; results may be inaccurate",
				FUNCNAME);
		putErrorOUTSTR();
	} /*if(reply == ITERUNCONVERGED)*/
	
	if(usePrevious)
	{
		/*
		  PREVMODELTYPE is changed to current model unless we are reusing
		  results of previous regression model when it is unchanged
		*/
		PREVMODELTYPE = prevType;
		PREVGLMCONTROL = prevControl;
		PREVGLMSCALE = prevScale;
	} /*if(usePrevious)*/
	else
	{
		PREVMODELTYPE = MODELTYPE;
		PREVGLMCONTROL = GLMCONTROL;
		PREVGLMSCALE = GLMSCALE;
	}
	
	yEvaluated = strcmp(NAME(MODELVARS[0]), EVALSCRATCH) == 0;
	if (!HASLABELS(MODELVARS[0]))
	{
		yHaslabels = 0;
	} /*if (!HASLABELS(MODELVARS[0]))*/
	else
	{
		yHaslabels = !yEvaluated;
		
		if (!yHaslabels && MODELTYPE & MANOVA)
		{
			char     *labels[2];
			char     *place;
			long      lablengths[2];
			
			getMatLabels(MODELVARS[0], labels, lablengths);
			place = labels[1];
			for (j = 0; j < nvars; j++)
			{
				if (*place++ != '\0')
				{
					/* at least one label that is not ""*/
					yHaslabels = 1;
					break;
				}
			} /*for (j = 0; j < nvars; j++)*/
		} /*if (!yHaslabels && MODELTYPE & MANOVA)*/
	} /*if (!HASLABELS(MODELVARS[0])){}else{}*/
	
	while(!usePrevious) /* fake while to allow exiting {...} by break*/
	{
		Symbolhandle   symSS, symDF, symRES, symWRES;
		Symbolhandle   symDEPVNAME;
		char          *rowcollabs[2], *pos;
		long           lablengths[2];
		long           termnamesLength;
		
		/* set up user accessible symbols for various quantities */

		symDEPVNAME = CInstall("DEPVNAME", strlen(*DEPVNAME) + 1);
		if(symDEPVNAME == (Symbolhandle) 0)
		{
			notInstalled = 1;
			break;
		}
		copyStrings(*DEPVNAME, STRINGPTR(symDEPVNAME), 1);
		setNDIMS(symDEPVNAME,1);
		setDIM(symDEPVNAME,1,1);

		symTERMNAMES = CInstall("TERMNAMES", 0);
		if (symTERMNAMES == (Symbolhandle) 0)
		{
			notInstalled = 1;
			break;
		}
		if (!(MODELTYPE & (ITERGLMS | IPF)) || MODELTYPE & ROBUSTREG ||
			INCREMENTAL)
		{
			TMPHANDLE = myduphandle(TERMNAMES);
			setSTRING(symTERMNAMES, TMPHANDLE);
		}
		else
		{
			long          needed = nterms - 1;
			char         *errorname = skipStrings(*TERMNAMES, nterms);
			char         *termname = "Overall model";
			
			needed += strlen(termname) + strlen(errorname) + 2;
			TMPHANDLE = mygethandle(needed);
			setSTRING(symTERMNAMES, TMPHANDLE);
			if (TMPHANDLE != (char **) 0)
			{
				char            *place1 = *TMPHANDLE;
				int              k;
				
				for (k = 0; k < nterms - 1; k++)
				{
					*place1++ = '\0';
				}
				place1 = copyStrings(termname, place1, 1);
				errorname = skipStrings(*TERMNAMES, nterms);
				copyStrings(errorname, place1, 1);
			}
		}
		
		if (TMPHANDLE == (char **) 0)
		{
			notInstalled = 1;
			break;
		}
		setNDIMS(symTERMNAMES,1);
		setDIM(symTERMNAMES,1,nterms + 1);

		symSS = RInstall("SS", mdatass);
		symDF = RInstall("DF", nterms + 1);
		symRES = RInstall("RESIDUALS", mdatay);

		if (symRES == (Symbolhandle) 0 || symDF == (Symbolhandle) 0 ||
			symSS == (Symbolhandle) 0)
		{
			notInstalled = 1;
			break;
		}

		if (WTDRESIDUALS != (double **) 0)
		{
			symWRES = RInstall("WTDRESIDUALS", mdatay);
			if (symWRES == (Symbolhandle) 0)
			{
				notInstalled = 1;
				break;
			}
		} /*if (WTDRESIDUALS != (double **) 0)*/

		termnamesLength = myhandlelength(STRING(symTERMNAMES));
				
		if (termnamesLength == CORRUPTEDHANDLE)
		{
			goto errorExit;
		}

		if (MODELTYPE & MANOVA)
		{/* modify dimensions for multivariate and set labels*/
			setNDIMS(symSS, 3);
			setDIM(symSS, 1, nterms + 1);
			setDIM(symSS, 2, ny);
			setDIM(symSS, 3, ny);

			/* put labels on symSS*/
			if (termnamesLength > 0)
			{
				long           needed = termnamesLength;

				if (yHaslabels)
				{
					getMatLabels(MODELVARS[0], rowcollabs, lablengths);
					needed += 2*lablengths[1];
				}
				else
				{
					needed += 2*expandLabels(LEFTBRACKET, ny, (char *) 0);
				}
					
				TMPHANDLE = mygethandle(needed);
				if (TMPHANDLE != (char **) 0 &&
					setLabels(symSS, TMPHANDLE))
				{
					appendLabels(symSS, STRINGPTR(symTERMNAMES), 0,
								 dontExpand);
					if (yHaslabels)
					{
						getMatLabels(MODELVARS[0], rowcollabs, lablengths);
						appendLabels(symSS, rowcollabs[1], 1, dontExpand);
						appendLabels(symSS, rowcollabs[1], 2, dontExpand);
					}
					else
					{
						appendLabels(symSS, LEFTBRACKET, 1, doExpand);
						appendLabels(symSS, LEFTBRACKET, 2, doExpand);
					}
				}
				else
				{
					mydisphandle(TMPHANDLE);
				}
			} /*if (termnamesLength > 0)*/
			
			setNDIMS(symRES,2);
			setDIM(symRES,1,ndata);
			setDIM(symRES,2,ny);
			
			if (MODELTYPE & (CASEWEIGHTS | ITERGLMS))
			{
				setNDIMS(symWRES,2);
				setDIM(symWRES,1,ndata);
				setDIM(symWRES,2,ny);
			} /*if (MODELTYPE & (CASEWEIGHTS | ITERGLMS))*/
		} /*if (MODELTYPE & MANOVA)*/
		else if (termnamesLength > 0)
		{
			TMPHANDLE = myduphandle(STRING(symTERMNAMES));
			if (TMPHANDLE != (char **) 0 && !setLabels(symSS, TMPHANDLE))
			{
				mydisphandle(TMPHANDLE);
			}
		} /*if (MODELTYPE & MANOVA){}else if (termnamesLength > 0){}*/
		
		if (yHaslabels)
		{
			/* put labels on symRES and symWRES*/
			if (ny > 1)
			{
				if (!yEvaluated)
				{
					/* use all labels */
					transferLabels(MODELVARS[0], symRES);
				}
				else
				{
					char         *labels[2];
					long          lablengths[2];
					
					getAllLabels(MODELVARS[0], labels, lablengths, (long *) 0);
					lablengths[0] = 2*ndata;
					
					TMPHANDLE = createLabels(2, lablengths);
					if (TMPHANDLE != (char **) 0 &&
						setLabels(symRES, TMPHANDLE))
					{
						char     numericLabel[2];
						
						numericLabel[0] = NUMERICLABEL;
						numericLabel[1] = '\0';
						
						appendLabels(symRES, numericLabel, 0, doExpand);
						appendLabels(symRES, getOneLabel(MODELVARS[0], 1, 0),
									 1, dontExpand);
					}
				}
			} /*if (ny > 1)*/
			else
			{
				long            lengths[2];
				char           *labels[2];
				
				getAllLabels(MODELVARS[0], labels, lengths, (long *) 0);
				TMPHANDLE = createLabels(1, lengths);

				if (TMPHANDLE != (char **) 0 && setLabels(symRES, TMPHANDLE))
				{
					appendLabels(symRES, getOneLabel(MODELVARS[0], 0, 0),
								 0, dontExpand);
				}
				else
				{
					mydisphandle(TMPHANDLE);
				}
			} /*if (ny > 1){}else{}*/
			if (WTDRESIDUALS != (double **) 0)
			{
				transferLabels(symRES, symWRES);
			}
		} /*if (yHaslabels)*/
		
		doubleCopy(*SS, DATAPTR(symSS), mdatass);

		residuals = DATAPTR(symRES);
		if (MODELTYPE & FASTANOVA)
		{
			j = 0;
			for (i = 0; i < ndata; i++)
			{ /* re-insert MISSING where squeezed out by funbalanova */
				if (!(MODELTYPE & MISSWEIGHTS) || (*MISSWTS)[i] > 0.)
				{
					residuals[i] = (*RESIDUALS)[j++];
				}
				else
				{
					setMissing(residuals[i]);
				}
			}
		} /*if (MODELTYPE & FASTANOVA)*/
		else
		{
			doubleCopy(*RESIDUALS, residuals, mdatay);
		}		

		if (WTDRESIDUALS != (double **) 0)
		{
			double     d, *wtdresiduals = DATAPTR(symWRES);

			doubleCopy(*WTDRESIDUALS, wtdresiduals, mdatay);
			if(MODELTYPE & ROBUSTREG)
			{
				double      *allwts = *ALLWTS;
				
				residuals = *RESIDUALS;
				
				for (i = 0; i < mdatay; i++)
				{
					/*
					   residuals[i] = r(i) = y[i] - fit[i]
					   wtdresiduals[i] = CURRENTRSC * psi(r(i)/CURRENTRSC)
					   Thus allwts[i] = psi(z)/z where z = r(i)/CURRENTRSC
					   This should be 1 if abs(z) <= RobTruncation and
					   RobTruncation/abs(z) for abs(z) > RobTruncation
					*/
					d = residuals[i];
					if (!isMissing(d) && d != 0.0)
					{
						allwts[i] = wtdresiduals[i]/d;
					}
				} /*for (i = 0; i < mdatay; i++)*/
			} /*if(MODELTYPE & ROBUSTREG)*/
		} /*if (MODELTYPE & (CASEWEIGHTS | ITERGLMS | IPF))*/

		if (!(MODELTYPE & (ITERGLMS | IPF)) || MODELTYPE & ROBUSTREG ||
			INCREMENTAL)
		{
			doubleCopy(*DF, DATAPTR(symDF), nterms + 1);
		}
		else
		{
			double       modelDF = 0.0;
			double      *df = DATAPTR(symDF);
			int          k;
			
			for (k = 0; k < nterms - 1; k++)
			{
				modelDF += (*DF)[k];
				df[k] = 0.0;
			}
			modelDF += (*DF)[k];
			df[k++] = modelDF;
			df[k] = (*DF)[k];
		}
		
		if (!(MODELTYPE & FASTANOVA))
		{
			Symbolhandle             symHII = RInstall("HII", ndata);

			if (symHII == (Symbolhandle) 0)
			{
				notInstalled = 1;
				break;
			}
			if (yHaslabels && !yEvaluated)
			{
				getMatLabels(MODELVARS[0], rowcollabs, lablengths);
				TMPHANDLE = mygethandle(lablengths[0]);
				if (TMPHANDLE != (char **) 0 && setLabels(symHII, TMPHANDLE))
				{
					getMatLabels(MODELVARS[0], rowcollabs, lablengths);
					appendLabels(symHII, rowcollabs[0], 0, dontExpand);
				}
				else
				{
					mydisphandle(TMPHANDLE);
				}
			} /*if (yHaslabels && !yEvaluated)*/
			doubleCopy(*HII, DATAPTR(symHII), ndata);
		} /*if (!(MODELTYPE & FASTANOVA))*/

		if (MODELTYPE & OLSREG)
		{
			Symbolhandle       symXTX, symCOEF;
			long               lablength;
			
			symXTX = RInstall("XTXINV", mdataxx);
			symCOEF = RInstall("COEF", nregx);
			if (symXTX == (Symbolhandle) 0 || symCOEF == (Symbolhandle) 0)
			{
				notInstalled = 1;
				break;
			}
			setNDIMS(symXTX,2);
			setDIM(symXTX,1,nregx);
			setDIM(symXTX,2,nregx);
			doubleCopy(*XTXINV, DATAPTR(symXTX), mdataxx);
			doubleCopy(*REGCOEF, DATAPTR(symCOEF), nregx);

			lablength = skipStrings(*TERMNAMES, nregx) - *TERMNAMES;
			
			TMPHANDLE = mygethandle(2*lablength);
			if (TMPHANDLE != (char **) 0 && setLabels(symXTX, TMPHANDLE))
			{
				pos = copyStrings(*TERMNAMES, *TMPHANDLE, nregx);
				pos = copyStrings(*TERMNAMES, pos, nregx);
			} /*if (TMPHANDLE != (char **) 0)*/
			else
			{
				mydisphandle(TMPHANDLE);
			}

			TMPHANDLE = mygethandle(lablength);
			if (TMPHANDLE != (char **) 0 && setLabels(symCOEF, TMPHANDLE))
			{
				copyStrings(*TERMNAMES, *TMPHANDLE, nregx);
			}
			else
			{
				mydisphandle(TMPHANDLE);
			}
		} /*if (MODELTYPE & OLSREG)*/
		break;
	} /*while(!usePrevious)   (fake while loop)*/
	
	if (PrintResults)
	{		
		if (MODELTYPE & OLSREG)
		{
			printregress(Pvals);
			if(FactorFound && INTERRUPT == INTNOTSET)
			{
				sprintf(OUTSTR, factorIsVariable,
						(FactorFound > 1) ? plural : NullString,
						(FactorFound > 1) ? plural : NullString,
						FUNCNAME);
						putErrorOUTSTR();
			}
		} /*if (MODELTYPE & OLSREG)*/
		else
		{
			printglm(Fstats,Pvals, Sssp, Byvar);
			if(usePrevious && FactorFound && INTERRUPT == INTNOTSET)
			{
				sprintf(OUTSTR, factorIsVariable,
						(FactorFound > 1) ? plural : NullString,
						(FactorFound > 1) ? plural : NullString,
						FUNCNAME);
				putErrorOUTSTR();
			} /*if(usePrevious && FactorFound && INTERRUPT == INTNOTSET)*/
		} /*if (MODELTYPE & OLSREG) {...}else{...} */
	} /*if(PrintResults)*/
	*OUTSTR = '\0';

	if(!PrintResults && !Silent)
	{
		putOutMsg("NOTE: Some results are in variables SS, DF, and RESIDUALS");
		if(MODELTYPE & DOCOEFS)
		{
			putOutMsg("      Use coefs(), secoefs(), or modelinfo() to retrieve other results");
		}
	} /*if(!PrintResults && !Silent)*/

	if(notInstalled && !Silent)
	{
		strcpy(OUTSTR,
			   "WARNING: not all the side effect variables could be created");
		putErrorOUTSTR();
	}
	

#ifdef MACINTOSH
	unloadGlmSegs();
#endif /*MACINTOSH*/

#ifdef MW_CW_PROFILER
/*	ProfilerSetStatus(0); */
#endif /*MW_CW_PROFILER*/

	RUNNINGGLM = (char *) 0;
	return (NULLSYMBOL);

  errorExit:
	RUNNINGGLM = (char *) 0;
	if(doCleanup)
	{
		glmcleanup(0);
	}
	
	MODELTYPE = NOMODEL;

	putErrorOUTSTR();
	emptyTrash();

#ifdef MACINTOSH
	unloadGlmSegs();
#endif /*MACINTOSH*/
	
	return (0);
	
} /*glm()*/


/* NOTE: neither glmcleanup nor clearGlobals should use OUTSTR */

void glmcleanup(int removeStrmodel)
{
	WHERE("glmcleanup");
	
	/* get rid of misc anova globals after error */

	clearGlobals();	

	Remove("RESIDUALS");
	Remove("WTDRESIDUALS");
	Remove("HII");
	Remove("SS");
	Remove("DF");
	if(removeStrmodel)
	{
		Remove("STRMODEL");
	}
	Remove("DEPVNAME");
	Remove("TERMNAMES");
	Remove("XTXINV");
	Remove("COEF");
} /*glmcleanup()*/

void clearGlobals(void)
{
	/* delete all glm related globals */
	long            i;
	long            nvars = (long) NVARS;
	WHERE("clearGlobals");
	
	PREVMODELTYPE = NOMODEL;
	USEGLMOFFSET = 0;
	NVARS = MODELINFO->nvars = -1;
	NFACTORS = MODELINFO->nfactors = -1;
	NTERMS = MODELINFO->nterms = 0;

	for(i=0;i<=MAXERRTRMS;i++)
	{
		zeroTerm(MODELINFO->errorterms[i]);
	}
/*
  Y was set to DATA(MODELVARS[0] and X[i] to DATA(MODELVARS[i+1] in
  getmodel().  However, they can be extended in funbalan.c and
  in that case should be deleted.
 */
	
	for (i = 0; i <= nvars; i++)
	{
		if(i == 0 && MODELVARS[0] != (Symbolhandle) 0)
		{
			if(Y != DATA(MODELVARS[0]))
			{
				mydisphandle((char **) Y);
			}
			Y = (double **) 0;
		}
		else if(MODELVARS[i] != (Symbolhandle) 0)
		{
			if(X[i-1] != DATA(MODELVARS[i]))
			{
				mydisphandle((char **) X[i-1]);
			}
			X[i-1] = (double **) 0;
		}
		
		Delete(MODELVARS[i]);
		MODELVARS[i] = (Symbolhandle) 0;
	} /*for (i = 0; i <= nvars; i++)*/
	
	/* Note: As of 930601, INTMODEL no longer exists */
	mydisphandle5((char **) MODEL, (char **) RESIDUALS,
				  (char **) WTDRESIDUALS, (char **) 0, (char **) 0);
	MODEL = (modelHandle) 0;
	RESIDUALS = WTDRESIDUALS = (double **) 0;

	mydisphandle5((char **) HII, (char **) SS, (char **) DF,
				  STRMODEL, TERMNAMES);
	HII = SS = DF = (double **) 0;
	TERMNAMES = STRMODEL = (char **) 0;

	mydisphandle(DEPVNAME);
	DEPVNAME = (char **) 0;
	
	mydisphandle5((char **) REGX, (char **) REGX2, (char **) XTXINV,
				  (char **) REGCOEF, (char **) REGSS);
	REGX = REGX2 = XTXINV = REGCOEF = REGSS = (double **) 0;

	mydisphandle5((char **) MISSWTS, (char **) CASEWTS, (char **) ITERWTS,
				  (char **) ALLWTS, (char **) GLMOFFSET);
	MISSWTS = CASEWTS = ITERWTS = ALLWTS = GLMOFFSET = (double **) 0;

	mydisphandle((char **) LOGITN);
	LOGITN = (double **) 0;
} /*clearGlobals()*/ 


