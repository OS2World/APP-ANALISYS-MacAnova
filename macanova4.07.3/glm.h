/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 4.07 or later
*(C)*
*(C)* Copyright (c) 1998 by Gary Oehlert and Christopher Bingham
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

#ifndef GLMH__
#define GLMH__
/*
  Defines and global declarations related to glm computations
  930811 Modified so that large globals such as MODELINFO are allocated
	dynamically rather than taking up part of the global segment;  this was
	because the Mac version ran over 32K globals
  Appropriate changes where made in the glm modules.

  970819 added macro VARNAME
  980710 add macro BinomClamp
  980804 Renamed BINOMCLAMP and BinomClamp to BINOMPLIMIT and BinomPlimit
*/

#include "typedefs.h"

#undef EXTERN
#undef INIT
#undef INITDIM
#undef INITARRAY
#undef COMMA

#ifdef MAIN__
#define EXTERN
#define INIT(VAL) = VAL
#define INITDIM(N) N
#define INITARRAY(VALS) = { VALS }
#else
#define EXTERN extern
#define INIT(VAL)
#define INITDIM(N)
#define INITARRAY(VALS)
#endif /*MAIN__*/

#define COMMA ,

/* Limits (formerly in globdefs.h) */
#ifndef WORDSPERTERM
#define WORDSPERTERM  3       /* number of words in a term */
#endif /*WORDSPERTERM*/

#ifndef VARSPERWORD
#define VARSPERWORD  32     /* number of variables per word */
#endif /*VARSPERWORD*/

#define OLDMAXVARS   31
#define MAXFACTORLEVELS 32767 /* maximum number of levels in a factor */
/*
   Options related to GLMS
   Their default values can be changed in Makefile if desired.
*/

#ifndef DEFAULTPRINTFSTATS
#define DEFAULTPRINTFSTATS   0
#endif /*DEFAULTPRINTFSTATS*/

#ifndef DEFAULTPRINTPVALS
#define DEFAULTPRINTPVALS    0
#endif /*DEFAULTPRINTPVALS*/

EXTERN long       PRINTFSTATS INIT(DEFAULTPRINTFSTATS);
EXTERN long       PRINTPVALS  INIT(DEFAULTPRINTPVALS);

/*
   Constants and globals affecting algorithms
*/

#ifndef REGTOLVAL
#define REGTOLVAL     1.0e-12  /* used in singularity check in gramschmidt */
#endif /*REGTOLVAL*/
EXTERN double  REGTOL INIT(REGTOLVAL); /* made a global 951012*/

/* maximum number of iterations and tolerance for iterative functions */
#define FASTANOVAMAXITER  25
#define FASTANOVAEPSILON  1e-6
#define ITERGLMMAXITER    50
#define ITERGLMEPSILON    1e-6
#define IPFMAXITER        25
#define IPFEPSILON        1e-6

/* Global related to iterated glm computational algorithms */
EXTERN double    GlmGlobals[INITDIM(10)];

/* Constants and macros related to robust()*/
#define   ROBTRUNCATION     .75 /*default value Huber truncation point */

#define   CURRENTRSC      GlmGlobals[0] /*current robust scale factor*/
#define   RobTruncation   GlmGlobals[1] /* Huber truncation point */
/* RobBeta = cumchi(c*c,3) + 2*c*c*(1-cumnor(c)), c = RobTruncation*/
#define   RobBeta         GlmGlobals[2]
#define   ROBUSTVARK      GlmGlobals[3] /*K from page 40 of Huber 1977 */
#define   REGDIM          GlmGlobals[4] /* degrees of freedom in model */
#define   GLMSCALE        GlmGlobals[0]
#define   GLMPOWER        GlmGlobals[1]

/* constant related to logistic() and probit()*/
#define   BINOMPLIMIT      1e-8
#define   BinomPlimit      GlmGlobals[2]
#define   HitPlimit        GlmGlobals[3]

#if     WORDSPERTERM == 1
#define MAXVARS    VARSPERWORD		/* max number of variables in model */
#else
#define MAXVARS    (WORDSPERTERM*VARSPERWORD)		/* max number of variables in model */
#endif  /*WORDSPERTERM == 1 */

#define MAXERRTRMS 10		/* max number of error terms */

/* stuff relating to representation of models */
typedef unsigned long termWord;
typedef termWord      modelType[WORDSPERTERM];
typedef modelType    *modelPointer, **modelHandle;

typedef struct       modelInfo
{
	long             nvars;
	long             nfactors;
	long             nterms;
	long             nerrorterms;
	modelHandle      model;
	modelType        errorterms[MAXERRTRMS+1];
	Symbolhandle     modelvars[MAXVARS+1];
	long             nclasses[MAXVARS];
} modelInfo, *modelInfoPointer, **modelInfoHandle;

#define modelterm(M,I) (((modelPointer) *(M)) + (I)) /* type modelPointer */
#define zeroTerm(T)  modeltermAssign((modelPointer) NULLTERM, (modelPointer) T)


/* constants related to glm's */

/* GLM Codes used in MODELTYPE and PREVMODELTYPE*/
/* special cases, different algorithms */
#define NOMODEL      0x0000UL  /* 0 no current model */
#define LEAPS        0x0001UL  /* 1 screening regression*/
#define ANOVA        0x0002UL  /* 2 anova, balanced is special case */
#define FASTANOVA    0x0004UL  /* 4 iterative fast anova via EM */

/* single pass gram schmidt models */
#define MANOVA       0x0008UL  /* 8 manova */
#define OLSREG       0x0010UL  /* 16 standard regression */

/* multipass iterative gram schmidt models */
#define ROBUSTREG    0x0020UL  /* 32 robust regression */
#define GLMREG       0x0040UL  /* 64 general glm regression */
#define LOGITREG     0x0080UL  /* 128 logistic regression */
#define POISSONREG   0x0100UL  /* 256 poisson regression */
#define PROBITREG    0x0200UL  /* 512 probit regression */
#define ITERGLMS     0x03e0UL  /* ROBUSTREG|GLMREG|LOGITREG|POISSONREG|PROBITREG */
#define BINOMREG     0x0280UL  /* LOGITREG|PROBITREG*/

/* weight indicators */
#define CASEWEIGHTS  0x0400UL  /* 1024 user specified case weights */
#define MISSWEIGHTS  0x0800UL  /* 2048 missing values, so weights needed */
#define USEWEIGHTS   0x0fe0UL  /* ITERGLMS | CASEWEIGHTS | MISSWEIGHTS */

#define IPF          0x1000UL  /* 4096 iterative proportional fitting */

#define FSTATSOK     (ANOVA | FASTANOVA | MANOVA)
#define ALLGLMS      (FSTATSOK | OLSREG | ITERGLMS | LEAPS | IPF)

/* GLM Codes used in GLMCONTROL and PREVGLMCONTROL*/
 /* response distribution codes */
#define NORMALDIST    0x000001UL /* normal distribution */
#define BINOMDIST     0x000002UL /* binomial distribution */
#define POISSONDIST   0x000004UL /* poisson distribution */
#define GAMMADIST     0x000008UL /* gamma distribution */
#define NONNORMALDIST 0x00000eUL /* BINOMDIST | POISSONDIST | GAMMADIST */
#define ANYDIST       0x00000fUL /* NONNORMALDIST | NORMALDIST */
 /* link specification codes */
#define IDENTLINK     0x000100UL /* identity link */
#define LOGITLINK     0x000200UL /* logit link */
#define LOGLINK       0x000400UL /* natural log link */
#define PROBITLINK    0x000800UL /* invnor link */
#define SQRTLINK      0x001000UL /* square root link */
#define RECIPLINK     0x002000UL /* reciprocal link */
#define POWERLINK     0x004000UL /* power link */
#define CLOGLOGLINK   0x008000UL /* complementary log-log link */
#define EXPLINK       0x010000UL /* exponential link */
#define NONIDENTLINK  0x01fe00UL /* LOGITLINK | LOGLINK | ... | EXPLINK */
#define ANYLINK       0x01ff00UL /* NONIDENTLINK | IDENTLINK */
 /* other codes for GLMCONTROL */
#define UNBALANCED    0x100000UL /* use gramschmidt */
#define MULTIVAR      0x200000UL /* multivariate response */
#define MARGINALSS    0x400000UL /* marginal (type III) SS */

/* op codes for cellstat() */
#define CELLSTATS         1
#define CELLSUMS          2
#define CELLCOUNTS        3
#define CELLMEANSANDSWEEP 4

/* opcodes for gramschmidt() and/or itergs() */
#define DONTNOTIFY   0x000000
#define DONTDOSWEEP  0x000000
#define DONTDOCOEFS  0x000000
#define DONTDORINV   0x000000
#define DONTDOSS     0x000000

#define DONOTIFY     0x010000  /* warn if singular column found */
#define DOSWEEP      0x020000  /* actually do orthogonalization */
#define DOCOEFS      0x040000  /* compute coefficients */
#define DORINV       0x080000  /* compute R^(-1) in QR decomposition */
#define DOSS         0x100000  /* compute SS or SSCP */

/* opcodes for iterfns.c functions */
#define INITIAL      1L
#define STEP         2L
#define FINAL        4L

/* global variables related to doing glm */

#if    WORDSPERTERM == 1
EXTERN modelType       NULLTERM INITARRAY(0);
#elif  WORDSPERTERM == 2
EXTERN modelType       NULLTERM INITARRAY(0 COMMA 0);
#else  /*WORDSPERTERM == 3*/
EXTERN modelType       NULLTERM INITARRAY(0 COMMA 0 COMMA 0);
#endif /*WORDSPERTERM == 3*/

/* MODELINFO is initialized in glmInit() */
EXTERN modelInfoPointer MODELINFO; /* previously global space was allocated */

#if (0) /* INTMODEL no longer referenced */
EXTERN modelHandle     INTMODEL INIT((modelHandle) 0);	/*  like MODEL except integers */
#endif /*0*/

/* VARNAMES is initialized in glmInit() */
/* matrix of variable names in model*/
EXTERN char           (*VARNAMES)[NAMELENGTH+1];
#if (0)   /* previously global space was allocated */
EXTERN char            VARNAMES[INITDIM(MAXVARS+1)][NAMELENGTH+1];
#endif /*0*/

/* find pointer to name or to expression computing variable */
#define VARNAME(I) ((strcmp(VARNAMES[I], EVALSCRATCH) != 0) ? \
					VARNAMES[I] : LABELSPTR(MODELVARS[I]))
#if (0) /* MODEL now a macro */
EXTERN modelHandle     MODEL INIT((modelHandle) 0);
#endif /*0*/

#define MODEL          MODELINFO->model
	/* handle to modelTypes indicating which order to enter terms in model */

EXTERN long            INCREMENTAL INIT(0);/*  incremental deviance flag */
EXTERN long	           USEGLMOFFSET INIT(0); /* flag for using glm offsets */

/* handles to variables used in model, 0 is y */
#define MODELVARS      MODELINFO->modelvars

#if (0)
EXTERN Symbolhandle    MODELVARS[INITDIM(MAXVARS+1)];
#endif /*0*/

EXTERN char           *RUNNINGGLM INIT(0); /*set != 0 at start of glm*/
EXTERN double          NVARS INIT(-1);	/* number of variables in model */
EXTERN double          NFACTORS INIT(-1); /* number of factors in model */
EXTERN double          NDATA INIT(0);	/* number of data values */
EXTERN double          NOTMISSING INIT(0);	/* number of nonmissing data */
EXTERN double          NTERMS INIT(0);	/* number of terms in model */

#define NCLASSES       (MODELINFO->nclasses)

#if (0)
EXTERN long            NCLASSES[INITDIM(MAXVARS)];
#endif /*0*/
	/* number of classes in each model variable */

EXTERN double          NREGX; /* number of columns in regression matrix */
EXTERN double        **RESIDUALS INIT(0);	/* handle to residuals */
EXTERN double        **WTDRESIDUALS INIT(0);	/* handle to residuals */
EXTERN double        **HII INIT(0);	/* handle to leverages */
/* Note: Y should always be identical to DATA(MODELVARS[0]) */
EXTERN double        **Y INIT(0);		/* handle to y data */
EXTERN double          NY INIT(0);		/* number of Y's (for manova) */
/* Note: X[i] should always be identical to DATA(MODELVARS[i+1]) */
EXTERN double       ***X; /*initialized in glmInit*/
#if (0)
EXTERN double        **X[INITDIM(MAXVARS)];	/* array of handles to x columns */
#endif /* 0 */
EXTERN double        **SS INIT(0);	/* handle for model term sums of squares */
EXTERN double        **DF INIT(0);		/* handle for model term dfs */
EXTERN double        **MISSWTS INIT(0);	/* handle for missing value weights */
EXTERN double        **CASEWTS INIT(0);
	/* handle for user weights in linear models */
EXTERN double        **ITERWTS INIT(0);	/* handle for iterative weights */
EXTERN double        **ALLWTS INIT(0);	/* combined weights */
EXTERN double        **LOGITN INIT(0);	/* counts for logistic N */
EXTERN double	     **GLMOFFSET INIT(0); 	/* offsets in a glm model */
EXTERN long            MODELTYPE INIT(NOMODEL);	/* indicator for model type */
EXTERN long            PREVMODELTYPE INIT(NOMODEL); /* previous model type */
EXTERN unsigned long   GLMCONTROL  INIT(0);
EXTERN unsigned long   PREVGLMCONTROL  INIT(0);
EXTERN double          PREVGLMSCALE    INIT(1.0);
/*
   Bits in MODELTYPE and PREVMODELTYPE
				0  means no current model (NOMODEL)
				1  means leaps screening (LEAPS)
				2  means anova (ANOVA)
				4  means fast anova (FASTANOVA) 
				8  means manova (MANOVA)
			   16  means standard regression (OLSREG)
               32  means robust regression (ROBUSTREG)
			   64  means general glm regression (GLMREG)
			  128  means logistic regression (LOGITREG)
			  256  means Poisson regression (POISSONREG)
              512  means Probit regression (PROBITREG)
			 1024  means case weights (CASEWEIGHTS)
			 2048  means missing value weights (MISSWEIGHTS)
*/

EXTERN char          **STRMODEL INIT(0);	/* string form of inputted model */

/* ERRORTERMS is initialized in glmInit() */
EXTERN modelPointer    ERRORTERMS INIT((modelPointer) 0);

#if (0)
EXTERN modelPointer    ERRORTERMS INIT((modelPointer) MODELINFO.errorterms);
#endif /*0*/

		/* terms used as errors */
EXTERN double          NERRORTERMS;	/* number of error terms */
EXTERN char          **DEPVNAME INIT(0);    /* name of dependent variable */
EXTERN char          **TERMNAMES INIT(0);	/* names of terms */
EXTERN double        **REGX INIT(0);	/* x matrix for regression */
EXTERN double        **REGX2 INIT(0);	/* x matrix for regression */
EXTERN double        **XTXINV INIT(0);	/* xtxinv matrix for regression */
EXTERN double        **REGCOEF INIT(0);	/* regression coefficients */
EXTERN double        **REGSS INIT(0);	/* sequential ss for regression */

#endif /*GLMH_*/
