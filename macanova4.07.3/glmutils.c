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
#pragma segment Glmutils
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

/*
	971020 Changed segment from Glm to Glmutils to avoid jump table overflow
	       added an UNLOADSEG(getterm) in glm.c to make sure it is
		   unloaded when other glm stuff is
    990212 Changed putOUTSTR() to putErrorOUTSTR()
*/
#include "globals.h"
#include "blas.h"

extern modelInfoPointer MreaderModelInfoPointer; /* defined in mreader.y */

/*
  Below are public functions related to glm

  long glmInit(void)
      Allocate glm-related globals (called by initiali.c)
  void getterm(long n, long term[], nvars)
	  Break out bits of jterm into vector term[]
  long setterm(long term[], nvars, nclasses)
	  Compute number of cells in marginal specified term and nclasses
	  and set term[i] < 0 for any variate (nclasses[i] < 0) in term
	  (term[i] != 0).
  long stringToTerm(char * string, long term [])
      Scan term name in string, setting term appropriately
  void effectiveTerm(long term[], long *effTerm, long nvars)
	  Set effTerm to reflect variates or the factors with level > 1 in term[]
	  If nvars == 0 it sets effTerm to 0
  long inEffectiveTerm(long term,long effTerm) (may be implemented as macro)
	  Check whether previous term prevterm is in current effective term
	  effTerm
  long inTerm(long jvar, modelPointer term) (may be implemented as macro)
	  Check whether model variable jvar is in term; jvar counts from 0
  long nvInTerm(modelPointer term)
	  Count the number of variables and factors in term.
  long moreInTerm(long jvar, modelPointer term) (may be implemented as macro)
	  Check whether there is a variable kvar > jvar in term, where
	  jvar is a variable in term;  jvar counts from 0
  long modeltermEqual(modelPointer term1,modelPointer term2)
	  Check two model terms for equality; return 1 if equal, 0 otherwise
  void modeltermAssign(modelPointer from, modelPointer to)
	  Copy contents of *from to *to
  char **setStrmodel(char **modelString, long clean)
	  Add '1+' to start of r.h.s. of modelString if not there, returning
	  handle for possibly modified model.  If clean != 0 also strip
	  '1[ \t]\+' from modelString if there, thereby possibly modifying
	  an argument.
  long inEarlierTerm(long termi, long term[], modelInfoPointer info)
      Returns 1 if vector term specifies a combination of factors
	  that are contained in terms 0, 1, ..., termi-1 of info->model;
	  otherwise it returns 0.	  
  long getAlpha(long termi, long colstart, double ***alphaH, long ***colptrH,
		long transposed)
	  Extract alphas associated with term termi from REGCOEF, returning
	  results in handle *alphaH to which is allocated ncells*ny doubles,
	  where ncells is the product of the dimensions of the factors in term
	  termi.  (**colptr)[i] is ALIASED if the i-th column in the term is
	  intrinsically aliassed, PIVOTED if it has been swept out, and the
	  column number, otherwise.  It seems it will only make sense to call
	  getAlpha() after fitting a glm; hence it uses the global information
	  rather than obtaining it through its argument list.
	  If transposed != 0, the array of coefficients for the term is
	  returned in transposed form (needed by anovacoefs()), otherwise
	  it is in untransposed form (needed by modelinfo()).
  void compLincom(double *xrow, double *lincom, double *seEst, double *sePred)
	  Compute linear combinations of regression coefficients with weights from
	  xrow (length nregx), putting results in lincom (length NY), and standard
	  errors in seEst (length ny).  Also, if sePred != (double *) 0, put
	  prediction standard errors in sePred (length ny)
  void deLink(double * predVec, double * seVec, double * binomn,
				 long incN, long nrows)
	  Transform predVec back to scale of response, and apply Jacobian to
	  standard errors if seVec != (double *) 0;  multiply precVec[l] and
	  seVec[l] by binomn[l * incN] if binomn != (double *) 0
  long margSS(void)
      Computes marginall SS from coefficients and XTXINV
  void seqSS(void)
      Computes sequential SS from REGSS
  void compDF(void)
      Computes actual DF from preliminary DF and aliasing info in REGSS
  long nAliased(void)
      Returns the number of aliased (pivoted) X-Variables
  void xtxinvInit(void)
	  Initialize upper triangle of (*XTXINV) to identity
  long Xsetup(void)
	  Routine to initialize DF, NREGX, and allocate storage for REGX, XTXINV,
	  REGCOEF, & REGSS, and compute the matrix of X-variables.
  long buildXmatrix(termi, regx, info)
	  If regx != 0 set up columns of x matrix corresponding to the termi-th
	  term in the model specified by info; otherwise just count cols needed.
	  Return number of columns needed or generated.  This is the maximum number
	  of degrees of freedom associated with the term (actual d.f. may be less)
	  depending on confounding
  long buildXrow(double * x, double * regx, modelInfoPointer info)
      Translates information in x to a vector whose elements
	  constitute elements of the X-matrix, using the information pointed
	  by info.
	  The value returned is the total number of degrees of freedom, not taking
	  into account any aliasing.
  long checkVariables(modelInfoPointer info)
	  Check to see that all variables are REAL and that their lengths match.
	  also check that dependent variable is a vector or matrix and that each
	  r.h.s variable is a vector.  If variates can ever be matrices, this
	  will need to be modified. If all o.k. return the number of cases (ndata);
	  otherwise return -1
  long countMissing(modelInfoPointer info, double ***misswts)
	  Return the number of cases that have missing values in either the
	  dependent variable or any of the model variables.
	  If misswts != (double ***) 0 and there are missing values, allocate a
	  handle for the missing weights and set (**misswts)[i] to 0 or 1
	  depending on whether case i contains missing values.  If there are
	  no missing values no handle is allocated.
  char *distName(unsigned long control)
      Return pointer to distribution name whose bit is set in control.  For
	  example, distName(POISSONDIST) has value "poisson".  If no distribution
      bit has been set, it returns "unknown".
  char *linkName(unsigned long control)
      Return pointer to link name whose bit is set in control.  For
	  example, linkName(LOGITLINK) has value "logit".  If no link
      bit has been set, it returns "unknown".
  long distControl(char *name)
      Return long with bit set corresponding to to the distribution whose
	  name is given.  For example, distControl("poisson") returns POISSONDIST
  long linkControl(char *name)
      Return long with bit set corresponding to to the link whose
	  name is given.  For example, distControl("logit") returns LOGITLINK

*/


#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment Initialize
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


typedef struct glmCode
{
	char           *name;
	unsigned long   code;
	int             length; /* != 0 means exact min partial match */
} glmCode, *glmCodePtr;

static glmCode        DistCodes[] =
{
	{"normal",        NORMALDIST,  4},
	{"gaussian",      NORMALDIST,  5},
	{"poisson",       POISSONDIST, 4},
	{"binomial",      BINOMDIST,   5},
	{"gamma",         GAMMADIST,   3},
	{"unknown",       0,           0}
};

static glmCode        LinkCodes[] =
{
	{"identity",      IDENTLINK,   5},
	{"logit",         LOGITLINK,   0},
	{"log",           LOGLINK,     0},
	{"probit",        PROBITLINK,  0},
	{"sqrt",          SQRTLINK,    2},
	{"reciprocal",    RECIPLINK,   5},
	{"power",         POWERLINK,   3},
	{"cloglog",       CLOGLOGLINK, 2},
	{"exponential",   EXPLINK,     3},
	{"unknown",       0,           0}
};


/*
   Allocate glm related globals
   This is called only from initiali.c
 */
long glmInit(void)
{
	char        *tempPointer;
	long         length;
	long         i;

/*
   Most of the following are also intialized in glm.h
   They are initialized here in anticipation of reorganizing globals
*/
	NVARS = -1;	/* number of variables in model */
	NFACTORS = -1; /* number of factors in model */
	NDATA = 0;	/* number of data values */
	NOTMISSING = 0;	/* number of nonmissing data */
	NTERMS = 0;	/* number of terms in model */
	RESIDUALS = (double **) 0;	/* handle to residuals */
	WTDRESIDUALS = (double **) 0;	/* handle to residuals */
	HII = (double **) 0;	/* handle to leverages */
/* Note: Y should always be identical to DATA(MODELVARS[0]) */
	Y  = (double **) 0;		/* handle to y data */
	NY = 0;		/* number of Y's (for manova) */
	SS = (double **) 0;	/* handle for model term sums of squares */
	DF = (double **) 0;		/* handle for model term dfs */
	MISSWTS = (double **) 0;	/* handle for missing value weights */
	CASEWTS = (double **) 0;
	/* handle for user weights in linear models */
	ITERWTS = (double **) 0;	/* handle for iterative weights */
	ALLWTS = (double **) 0;	/* combined weights */
	LOGITN = (double **) 0;	/* counts for logistic N */
	GLMOFFSET = (double **) 0; 	/* offsets in a glm model */
	STRMODEL = (char **) 0;	/* string form of inputted model */
	NERRORTERMS = 0;	/* number of error terms */
	DEPVNAME = (char **) 0;    /* name of dependent variable */
	TERMNAMES = (char **) 0;	/* names of terms */
	REGX = (double **) 0;	/* x matrix for regression */
	REGX2 = (double **) 0;	/* x matrix for regression */
	XTXINV = (double **) 0;	/* xtxinv matrix for regression */
	REGCOEF = (double **) 0;	/* regression coefficients */
	REGSS = (double **) 0;	/* sequential ss for regression */

	MODELTYPE = (long) NOMODEL;	/* indicator for model type */
	PREVMODELTYPE = (long) NOMODEL; /* previous model type */

	length = (MAXVARS+1) * (NAMELENGTH+1) * sizeof(char);
	tempPointer = mygetpointer(length);
	if(tempPointer == (char *) 0)
	{
		return (0);
	}
	VARNAMES = (char (*)[NAMELENGTH+1]) tempPointer;
	for(i=0;i<length;i++)
	{
		*tempPointer++ = '\0';
	}
	length = MAXVARS * sizeof(double **);
	tempPointer = mygetpointer(length);
	if(tempPointer == (char *) 0)
	{
		return (0);
	}
	X = (double ***) tempPointer;
	for(i=0;i<length;i++)
	{
		*tempPointer++ = '\0';
	}

	length = sizeof(modelInfo);
	tempPointer = mygetpointer(length);
	if(tempPointer == (char *) 0)
	{
		return (0);
	}
	MODELINFO = (modelInfoPointer) tempPointer;
	for(i=0;i<length;i++)
	{
		*tempPointer++ = '\0';
	}
	ERRORTERMS = MODELINFO->errorterms;

	tempPointer = mygetpointer(length);
	if(tempPointer == (char *) 0)
	{
		return (0);
	}
	MreaderModelInfoPointer = (modelInfoPointer) tempPointer;
	for(i=0;i<length;i++)
	{
		*tempPointer++ = '\0';
	}

	return (1);
} /*glmInit()*/


#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment Glmutils
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/



/*
  Break out bits of jterm into vector term[]
*/

void getterm(modelPointer jterm, long term [], long nvars)
{
	termWord        thisword = (*jterm)[0];
	long            i = 0, j;
	long            limit = VARSPERWORD;
	WHERE("getterm");

	for(j=0; j < nvars; j++)
	{
		if(j == limit)
		{
			thisword = (*jterm)[++i];
			limit += VARSPERWORD;
		}
		term[j] = thisword & 1L;
		thisword >>= 1;
	}
} /*getterm()*/

/*
  Compute number of cells in marginal specified term and nclasses
  and set term[i] < 0 for any variate (nclasses[i] < 0) in term (term[i] != 0).
*/

long setterm(long term[], long nvars, long nclasses[])
{
	long         i;
	long         ncells = 1;

	for(i=0;i<nvars;i++)
	{
		if(term[i] != 0)
		{
			if(nclasses[i] > 0)
			{
				ncells *= nclasses[i];
			}
			else
			{
				term[i] = -1;
			}
		}
	}
	return (ncells);
} /*setterm()*/

/*
  970820 Moved here from cellstts.c
         Modified to recognize {...} expressions as variable names
         also swapped return codes; 0 now means error
		 The number of components in term is returned.
         Also, term[k] is set to the number of the variable in the
		 term; thus if the model is "y=a+b",
		 stringToTerm("b.a", term) sets term to (2, 1)
*/
long stringToTerm(char *string, long term[])
{

	long            j, nvars = NVARS, length;
	long            jvar = 0;
	char            name[BUFFERLENGTH+1];
	char           *expression = "{...} expression";
	WHERE("stringToTerm");
	
	for (j = 0; j < nvars; j++)
	{
		term[j] = 0;
	}

	name[0] = '\0';
	while (*string)
	{
		while (isspace(*string))
		{
			string++;
		}
		jvar++;
		if (*string != '{')
		{
			length = 0;
			while (isnamechar(*string) && length <= BUFFERLENGTH)
			{
				name[length++] = *string++;
			}
			name[length] = '\0';
			if (length > NAMELENGTH)
			{
				sprintf(OUTSTR,
						"ERROR: variable name %s is too long", name);
				goto errorExit;
			}
		} /*if (*string != '{')*/
		else
		{
			long      level = 1, place = 0;
			
			string++;
			while (isspace(*string))
			{
				string++;
			}
			(void) findBracket(0, '{', (unsigned char *) string,
							   &level, &place, (long *) 0);
			if (level > 0)
			{
				sprintf(OUTSTR,
						"ERROR: No closing '}' in %s", expression);
				goto errorExit;
			}
			length = place - 1;
			while (length > 0 &&
				   (isspace(string[length-1]) || string[length-1] == ';'))
			{
				length--;
			}
			if (length == 0)
			{
				sprintf(OUTSTR, "ERROR: Empty %s", expression);
			}
			else if (length > BUFFERLENGTH)
			{
				sprintf(OUTSTR, "ERROR: %s is too long", expression);
			}
			if (*OUTSTR)
			{
				goto errorExit;
			}
			strncpy(name, string, length);
			name[length] = '\0';
			string += place;
		} /*if (*string != '{'){}else{}*/
		
		for (j = 1; j <= nvars; j++)
		{
			if (strcmp(name, VARNAME(j)) == 0)
			{
				if (term[j - 1])
				{
					sprintf(OUTSTR,
							"ERROR: duplicate model variable %s in term", name);
					goto errorExit;
				}
				term[j - 1] = jvar;
				break;
			}
		} /*for (j = 1; j <= nvars; j++)*/
		if (j > nvars)
		{
			sprintf(OUTSTR,
					"ERROR: variable %s in term is not in model", name);
			goto errorExit;
		}
		while (isspace(*string))
		{
			string++;
		}
		if (*string)
		{
			if (*string != '.')
			{
				sprintf(OUTSTR,
						"ERROR: illegal character or improper term specification");
				goto errorExit;
			}
			string++;
		}
	} /*while (*string)*/
	return (jvar);

  errorExit:
	putErrorOUTSTR();
	return (0);
	
} /*stringToTerm()*/

void stepGlmOdometer(long term[], long nclasses[], long nvars, long reverse)
{
	long          i;
	
	if (reverse)
	{
		for(i=nvars-1;i>=0;i--)
		{
			if(term[i] > 0)
			{/* it's a factor, so step it */
				term[i]++;
				if(term[i] <= nclasses[i])
				{
					break;
				}
				term[i] = 1;
			}
		} /*for(i=nvars-1;i>=0;i--)*/
	} /*if (reverse)*/
	else
	{
		for(i=0;i < nvars;i++)
		{
			if(term[i] > 0)
			{/* it's a factor, so step it */
				term[i]++;
				if(term[i] <= nclasses[i])
				{
					break;
				}
				term[i] = 1;
			}
		} /*for(i=nvars-1;i>=0;i--)*/
	} /*if (reverse){}else{}*/
} /*stepGlmOdometer()*/
/*
  Set effTerm to reflect variates or the factors with level > 1 in term[]
  If nvars == 0 it sets effTerm to 0
*/
void effectiveTerm(long term[], modelPointer effTerm, long nvars)
{
	register termWord    thisword = 0;
	register long        i = 0, j, bit = 1;
	long                 limit = VARSPERWORD;

	zeroTerm(effTerm);

	for (j = 0; j < nvars; j++)
	{
		if(j == limit)
		{
			(*effTerm) [i++] = thisword;
			limit += VARSPERWORD;
			thisword = 0;
			bit = 1;
		}
		if (term[j] < 0 || term[j] > 1)
		{	/* live var or non-1 factor */
			thisword |= bit;
		}
		bit <<= 1;
	}
	(*effTerm)[i] = thisword;
} /*effectiveTerm()*/


/*
  Check whether previous term prevterm is in current effective term effTerm
*/
long inEffectiveTerm(modelPointer prevterm,modelPointer effterm)
{
	register int     i;
	termWord         prev, eff;

	if(modeltermEqual(prevterm, effterm))
	{
		return (1);
	}
	
	for(i=0;i<WORDSPERTERM;i++)
	{
		prev = (*prevterm)[i];
		eff = (*effterm)[i];
		if(prev|eff && ((prev & eff) ^ eff))
		{
			return (0);
		}
	}
	return (1);
} /*inEffectiveTerm()*/

/*
  Check whether model variable jvar is in term
  jvar counts from 0
*/
long inTerm(long jvar, modelPointer term)
{
	long       i = jvar/VARSPERWORD;
	
	jvar -= i*VARSPERWORD;
	return ((((*term)[i] >> jvar) & 1L) != 0);
} /*inTerm()*/

/*
   Count the number of variables and factors in term
*/
long nvInTerm(modelPointer term)
{
	long       i, nv = 0;
	termWord   piece;
	
	for(i=0;i<WORDSPERTERM;i++)
	{
		for(piece = (*term)[i];piece != 0;piece >>= 1)
		{
			nv += (piece & 1L);
		}
	} /*for(i=0;i<WORDSPERTERM;i++)*/
	return (nv);
} /*nvInTerm()*/

/*
  Check whether there is a later variable than jvar in term, where jvar is in
  term;  jvar counts from 0
*/
long moreInTerm(long jvar, modelPointer term, long nvars)
{
	long           i, imax;
		
	if(nvars <= VARSPERWORD)
	{
		return ((((*term)[0] >> jvar) ^ 1) != 0);
	}

	i = jvar/VARSPERWORD;
	jvar %= VARSPERWORD;
	if((((*term)[i] >> jvar) ^ 1) != 0)
	{
		return (1);
	}
	
	imax = (nvars-1)/VARSPERWORD;
	for(++i;i <= imax; i++)
	{
		if((*term)[i] != (termWord) 0)
		{
			return (1);
		}
	}
	return (0);
} /*moreInTerm((*/

/*
  Check two model terms for equality; return 1 if equal, 0 otherwise
*/
long modeltermEqual(modelPointer term1,modelPointer term2)
{
	register long   i;

	for(i=0;i<WORDSPERTERM;i++)
	{
		if((*term1)[i] != (*term2)[i])
		{
			return (0);
		}
	}
	return (1);
} /*modeltermEqual()*/

/*
  Copy contents of *from to *to
*/
void        modeltermAssign(modelPointer from, modelPointer to)
{
	register long    i;

	for(i=0;i<WORDSPERTERM;i++)
	{
		(*to)[i] = (*from)[i];
	}
} /*modeltermAssign()*/


/*
  Add '1+' to start of r.h.s. of modelString if not there, returning
  pointer to possibly modified model.  If clean != 0 also strip
  '1[ \t]\+' from modelString if there, thereby possibly modifying
  an argument.

  980325 fixed bug so that setStrmodel copies a l.h.s {...} expression
         without scanning it
*/

char **setStrmodel(char **modelString, long clean)
{
	char          **strmodel;
	long            i, i1, j;
	char            c;
	
	strmodel = mygethandle(strlen(*modelString) + 3);

	if (strmodel != (char **) 0)
	{
		i = j = 0;

		/* copy left hand side of model in *modelString to *strmodel */
		while (1)
		{
			c = (*modelString)[i];
			if (c == '=' || c == '\0')
			{
				break;
			}
			if (c == '{')
			{
				long      level = 1, place = i + 1, length;
				
				c = findBracket(0, '{', (unsigned char *) *modelString,
								&level, &place, (long *) 0);
				length = place - i;
				strncpy(*strmodel + j, (*modelString) + i, length);
				j += length;
				i += length;
				if (c == '\0')
				{
					break;
				}
			} /*if (c == '{')*/
			else
			{
				(*strmodel)[j++] = c;
				i++;
			} /*if (c == '{'){}else{}*/
		} /*while (1)*/
		
		(*strmodel)[j++] = c; /* '=' or '\0' */
	
		if(c != '\0')
		{
			for(i++;(c = (*modelString)[i]) == ' ' || c == '\t';i++)
			{/* copy whitespace after '=' */
				(*strmodel)[j++] = c;
			}
			
			if(c != '1')
			{ /* insert '1+' */
				(*strmodel)[j++] = '1';
				(*strmodel)[j++] = '+';
			}
			/* copy remainder of model */
			strcpy(*strmodel + j, *modelString + i);

			if(c == '1' && clean)
			{/* if '1[ \t]*+' in string, delete it */
				for(i1 = i+1;
					(c = (*modelString)[i1]) == ' ' || c == '\t';i1++)
				{/* skip whitespace after '1' */
					;
				}
				if(c == '+')
				{/* squeeze out '1[ \t]*\+' in modelString */
					strcpy(*modelString+i,*modelString+i1+1);
				}
			} /*if(c == '1' && clean)*/
		} /*if(c != '\0')*/

		/* shrink strmodel to actual size */
		strmodel = mygrowhandle(strmodel,strlen(*strmodel)+1);
	}
	
	return (strmodel);
} /*setStrmodel()*/

long inEarlierTerm(long termi, long term[], modelInfoPointer info)
{
	modelType         trialterm;
	modelPointer      prevterm;
	long              j, k;
	long              term2[MAXVARS];
	long              nvars = info->nvars;
	long             *nclasses = info->nclasses;
	modelHandle       model = info->model;
	int               anyFactors = 0;
	WHERE("inEarlierTerm");
	
	for (j = 0; j < nvars; j++)
	{
		if (nclasses[j] < 0)
		{
			anyFactors = 1;
			break;
		}
	} /*for (j = 0; j < nvars; j++)*/
	
	effectiveTerm(term, (modelPointer) trialterm, nvars);
	/*  trialterm is now effective term for this potential column */

	for (k = 0; k < termi; k++)
	{
		prevterm = modelterm(model,k);
		if (inEffectiveTerm(prevterm,(modelPointer) trialterm))
		{
			/*
			   prevterm apparently in trialterm.
			*/
			if (!anyFactors)
			{
				break;
			}
			/*
			   Make sure there
			   are no variates in prevterm that are not in trialterm
			*/
			getterm(prevterm, term2, nvars);
			for (j = 0; j < nvars; j++)
			{
				if (term[j] >= 0 && term2[j] > 0 && nclasses[j] < 0)
				{
					break;
				}
				/* break out if variate in earlier term not here */
			} /*for (j = 0; j < nvars; j++)*/
			if (j == nvars)
			{ /* prevterm really in trial term*/
				break;
			}
		} /* if (inEffectiveTerm(prevterm,trialterm)) */
	} /* for (k = 0; k < termi; k++) */
	return (k != termi);

} /*inEarlierTerm()*/


/*
  Extract alphas associated with term termi from REGCOEF, returning results
  in alpha;   alpha is allocated with length ncells*ny, where ncells
  is the product of the dimensions of the factors in term termi.
  *colptr is allocated space for ncells+1 longs.  colptr[0] is set to ncells
  colptr[i+1]  is -1 if the  i-th column in the term is intrinsically
  aliassed, -1 if it has been swept out, and the column number, otherwise.
  It seems it will only make sense to call getAlpha() after fitting a glm;
  hence it uses the global information rather than obtaining it through
  its argument list.
  If alphH is NULL, the alpha's are not computed, but colptrH is.
*/
enum getAlphaScratch
{
	GTMPALPHA = 0,
	GTMPCOLPTR,
	GETALPHANTRASH
};

/* Also defined in anovacoe.c and varnames.c*/
#define ALIASED     -1  /* column is intrinsically aliased */
#define PIVOTED     -2  /* column is aliased, but not intrinsically */

long getAlpha(long termi, long colstart, double ***alphaH, long ***colptrH,
			  long transposed)
{
	long            i2, offset, j, k, term[MAXVARS];
	modelPointer    thisterm;
	long            itmp1, itmp2;
	long            nvars = (long) NVARS, ny = (long) NY, nregx = (long) NREGX;
	long            combo, ncells, imax, colno = colstart;
	long            do_coefs = (alphaH != (double ***) 0);
	double         *regss = *REGSS, *regcoef = *REGCOEF;
	double        **tmpalpha = (double **) 0, *alpha;
	long          **tmpcolptr = (long **) 0, *colptr;
	WHERE("getAlpha");
	TRASH(GETALPHANTRASH,errorExit);
	
	thisterm = modelterm(MODEL,termi);
	getterm(thisterm,term, nvars);
	imax = ncells = setterm(term,nvars, NCLASSES);
	if((do_coefs && !getScratch(tmpalpha,GTMPALPHA,imax*ny,double)) ||
	   !getScratch(tmpcolptr,GTMPCOLPTR,imax+1,long))
	{
		goto errorExit;
	}
	
	if(do_coefs)
	{
		*alphaH = tmpalpha;
		alpha = *tmpalpha;
	}
	
	*colptrH = tmpcolptr;
	colptr = *tmpcolptr;
	*colptr++ = ncells;

	/* 
	   loop over all coefficients in term.  Check to make sure they
	   are not aliased,  colptr[i] is set to ALIASED for intrinsic aliasing
	   or PIVOTED for swept out.
	   If ALIASED, ignore
	   If PIVOTED, then set matching cells of alpha to 0
	   Otherwise copy the coefficient (or coefficient vector, if ny > 0)
	   to alpha
   */
	for(combo = 0;combo < ncells;combo++)
	{
		/* check to see if each combination is already in through
		   lower order terms */
		if (transposed)
		{
			offset = 0;
			k = 1;
			for (j = 0; j < nvars; j++)
			{
				if (term[j] > 0)
				{
					offset += (term[j] - 1) * k;
					k *= NCLASSES[j];
				}
			}
		} /*if (transposed)*/
		else
		{
			offset = combo;
		}
		
		
		/*
		  offset is position in table of coefficients, traversed in
		  transposed order if transposed != 0 (expected by anovacoefs(),
		  non-transposed order otherwise (expected by modelinfo()).
		*/
		if (inEarlierTerm(termi, term, MODELINFO))
		{	/* set flag for intrinsic aliasing */
			colptr[offset] = ALIASED;
		} /*if (inEarlierTerm(termi, term, MODELINFO))*/
		else if (regss[colno] < -0.5)
		{	/* set flag for pivoted out */
			colptr[offset] = PIVOTED;
			colno++;
		} /*if (regss[colno] < -0.5)*/
		else
		{/* if to here, we have a term that is not obviously pivoted out */
			if(do_coefs)
			{
				itmp1 = offset; /* offset + i2*imax */
				itmp2 = colno;	/* colno + i2*nregx */
				for (i2 = 0; i2 < ny; i2++)
				{
					alpha[itmp1] = regcoef[itmp2];
					itmp1 += imax;
					itmp2 += nregx;
				}
			}				
			colptr[offset] = colno;
			colno++;
		}
		if (colptr[offset] < 0 && do_coefs)
		{
			itmp1 = offset; /* offset + i2 * imax */
			for (i2 = 0; i2 < ny; i2++)
			{
				alpha[itmp1] =  0.0;
				itmp1 += imax;
			} /*for (i2 = 0; i2 < ny; i2++)*/
		} /*if (colptr[offset] < 0 && do_coefs)*/ 

		/* step "odometer" */
		stepGlmOdometer(term, NCLASSES, nvars, transposed);
	} /*for(combo = 0;combo < ncells;combo++)*/
	if(do_coefs)
	{
		unTrash(GTMPALPHA);
	}
	unTrash(GTMPCOLPTR);
	emptyTrash();
	
	return (colno - colstart);

  errorExit:
	emptyTrash();
	if(do_coefs)
	{
		*alphaH = (double **) 0;
	}
	*colptrH = (long **) 0;
	return (-1);
} /*getAlpha()*/

/*
   compute sum(beta[i]*xrow[i]) standard errors
*/
void compLincom(double *xrow, double *lincom, double *seEst, double *sePred)
{
	long            ny = (long) NY, nregx = (long) NREGX;
	long            nterms = (long) NTERMS;
	long            dferror = (long) (*DF)[nterms];
	long            i, j, k, l, ll;
	long            incll;
	int             useScale =
		(PREVGLMCONTROL & NONNORMALDIST) != 0 && PREVGLMSCALE > 0;
	double         *regss = *REGSS, *xtxinvj;
	double         *regcoef = *REGCOEF, *sserror = regss + nregx;
	double          pred, var, rmse;
	WHERE("compLincom");
	
	if (lincom != (double *) 0)
	{
		for (k = 0; k < ny; k++)
		{
			pred = 0;
			for (j = 0; j < nregx; j++)
			{
				if (regss[j] > -.5)
				{
					pred += regcoef[j] * xrow[j];
				}
			}
			lincom[k] = pred;
			regcoef += nregx;
		} /*for (k = 0; k < ny; k++)*/
	} /*if (lincom != (double *) 0)*/
	
	if (seEst != (double *) 0 || sePred != (double *) 0)
	{
		if (dferror > 0 || useScale)
		{
			var = 0;
			incll = (nregx + 1)*(ny + 1);
			xtxinvj = *XTXINV;

			for (j = 0; j < nregx; j++)
			{
				if (regss[j] > -.5)
				{
					for (i = 0; i < nregx; i++)
					{
						var += xrow[i]*xtxinvj[i]*xrow[j];
					}
				} /*if (regss[j] > -.5)*/
				xtxinvj += nregx;				
			} /*for (j = 0; j < nregx; j++)*/
			ll = 0;
			for (l = 0; l < ny; l++)
			{
				rmse = (!useScale) ? sqrt(sserror[ll]/dferror) : PREVGLMSCALE;
				if (seEst != (double *) 0)
				{
					seEst[l] = rmse * sqrt(var);
				}
				if (sePred != (double *) 0)
				{
					sePred[l] = rmse*sqrt(1.0 + var);
				}
				ll += incll;
			} /*for (l = 0; l < ny; l++)*/
		} /*if (dferror > 0 || useScale)*/
		else
		{
			for (l = 0; l < ny; l++)
			{
				if (seEst != (double *) 0)
				{
					setMissing(seEst[l]);
				}
				if (sePred != (double *) 0)
				{
					setMissing(sePred[l]);
				}
			} /*for (l = 0; l < ny; l++)*/
		}
	} /*if (seEst != (double *) 0)*/
} /*compLincom()*/

#define normDensity(Z)    0.3989422804014326779*exp(-.5*(Z)*(Z))

void deLink(double * predVec, double * seVec, double * binomn,
				 long incN, long nrows)
{
	double       *predVec1, *seVec1;
	long          i, j, k;
	long          ny = (long) NY;
	long          nvals = nrows*ny;
		
	if (PREVGLMCONTROL & LOGLINK)
	{
		for (i = 0; i < nvals; i++)
		{
			predVec[i] = exp(predVec[i]);
			if (seVec != (double *) 0)
			{
				seVec[i] *= predVec[i];
			}
		} /*for (i = 0; i < nvals; i++)*/
	}
	else if (PREVGLMCONTROL & LOGITLINK)
	{
		for (i = 0; i < nvals; i++)
		{
			predVec[i] = exp(predVec[i]) / (1.0 + exp(predVec[i]));
			if (seVec != (double *) 0)
			{
				seVec[i] *= predVec[i]*(1.0 - predVec[i]);
			}
		} /*for (i = 0; i < nvals; i++)*/
	}
	else if (PREVGLMCONTROL & PROBITLINK)
	{
		for (i = 0; i < nvals; i++)
		{
			if (seVec != (double *) 0)
			{
				seVec[i] *= normDensity(predVec[i]);
			}
			predVec[i] = Cnor(predVec[i]);
		} /*for (i = 0; i < nvals; i++)*/
	}

	if (binomn != (double *) 0)
	{
		predVec1 = predVec;
		if (seVec != (double *) 0)
		{
			seVec1 = seVec;
		}

		for (k = 0; k < ny; k++)
		{
			j = 0;
			for (i = 0; i < nrows; i++)
			{
				predVec1[i] *= binomn[j];
				if (seVec != (double *) 0)
				{
					seVec1[i] *= binomn[j];
				}
				j += incN;
			} /*for (i = 0; i < nrows; i++)*/
			predVec1 += nrows;
			if (seVec != (double *) 0)
			{
				seVec1 += nrows;
			}
		} /*for (k = 0; k < ny; k++)*/
	} /*if (binomn != (double *) 0)*/
} /*deLink()*/

/*
   workhorse for computing marginal SS
   icol      column number of first X-variable in term
   maxdf     number of X-variables in term, some possibly aliased
   ncols     number of non-aliased X-variables in term
   colnos    colnos[i], column numbers of non-aliased X-variables
             relative to start of term
   cp        workspace to which the relevant segments of *XTXINV
             and coefficients are to copied and swept
*/
static long compMarg(long icol, long maxdf, long ncols, long *colnos,
					  double *cp)
{
	long           nregx = (long) NREGX;
	long           ny = (long) NY;
	long           cpdim = maxdf + ny;
	double        *xtxinv = *XTXINV + icol*(nregx+1);
	double        *coefs = *REGCOEF + icol;
	double        *diag = cp + cpdim*cpdim;
	double        *cpj;
	long           i, j, k, ij1, ij2, ik1, ik2, ki;
	long           jcol;
	long           inc1 = 1;
	WHERE("compMarg");

	ij1 = 0;
	ij2 = 0;
	for (i = 0; i < maxdf; i++)
	{ /* copy part of xtxinv */
		doubleCopy(xtxinv + ij1, cp + ij2, maxdf);
		diag[i] = cp[ij2+i];
		ij1 += nregx;
		ij2 += cpdim;
	} /*for (i = 0; i < maxdf; i++)*/

	ik1 = 0;
	ik2 = ij2;
	ki = maxdf;
	/* copy coefficients */
	for (k = 0; k < ny; k++)
	{
		DCOPY(&maxdf, coefs + ik1, &inc1, cp + ik2, &inc1);
		DCOPY(&maxdf, coefs + ik1, &inc1, cp + ki, &cpdim);
		ik1 += nregx;
		ik2 += cpdim;
		ki++;
	} /*for (k = 0; k < ny; k++)*/

	ij1 = maxdf*(cpdim + 1);
	if (ny == 0)
	{
		cp[ij1] = 0.0;
	}
	else
	{
		for (k = 0; k < ny; k++)
		{
			doubleFill(cp + ij1, 0.0, ny);
			ij1 += cpdim;
		} /*for (k = 0; k < ny; k++)*/
	}
	
	/* sweep right here */
	for (jcol = 0; jcol < ncols; jcol++)
	{
		k = colnos[jcol];
		if (!doSwp(cp, cpdim, cpdim, k, diag[k], 0))
		{
			goto errorExit;
		}
	} /*for (jcol = 0; jcol < ncols; jcol++)*/

	/* change sign of part not swept on*/
	cpj = cp + (cpdim+1)*maxdf;
	if (ny == 1)
	{
		cpj[0] = -cpj[0];
	}
	else
	{
		for (j = 0; j < ny; j++)
		{
			for (i = 0; i < ny;i++)
			{
				cpj[i] = -cpj[i];
			}
			cpj += cpdim;
		} /*for (j = 0; j < ny; j++)*/
	}
	return (1);
	
  errorExit:
	strcpy(OUTSTR, "ERROR: singularity found; cannot compute SS");
	putErrorOUTSTR();
	return (0);
	
} /*compMarg()*/

/*
   Controls computation of marginal SS
   Note: compDF() must not be called before margSS().
*/

enum margSSScratch
{
	GCOLNOS = 0,
	GCP,
	MARGSSNTRASH
};

long margSS(void)
{
	long      nterms = (long) NTERMS;
	long      ny = (long) NY;
	long      iterm, termstart, varno;
	long      dfi, maxdfi, needed = 0;
	long      cpdim;
	long    **colnosH, *colnos;
	long      inc1 = 1, inc2;
	long      j, k;
	double  **cpH, *cp;
	double   *df = *DF, *ss, *regss;
	double   *cpij, *ssij;
	WHERE("margSS");
	TRASH(MARGSSNTRASH, errorExit);
	
	for (iterm = 0; iterm < nterms; iterm++)
	{
		needed = (df[iterm] > needed) ? df[iterm] : needed;
	} /*for (iterm = 0, iterm < nterms; iterm++)*/
	
	if (!getScratch(colnosH, GCOLNOS, needed, long) ||
		!getScratch(cpH, GCP, (needed + ny) * (needed + ny) + needed, double))
	{
		goto errorExit;
	}
	colnos = *colnosH;
	cp = *cpH;
	df = *DF;
	regss = *REGSS;

	/* last term should already be ok*/
	termstart = 0;
	maxdfi = 0;
	for (iterm = 0; iterm < nterms; iterm++)
	{
		termstart += maxdfi;
		varno = termstart;
		dfi = 0;
		maxdfi = df[iterm];
		for (j = 0; j < maxdfi; j++)
		{/* go through the cols for each term */
			if (regss[varno++] > -0.5)
			{ /* columns to sweep on*/
				colnos[dfi++] = j;
			}
		} /*for (j = 0; j < maxdfi; j++)*/

		if (!compMarg(termstart, maxdfi, dfi, colnos, cp))
		{
			goto errorExit;
		}
		
		/* copy results to SS */
		ss = *SS + iterm;
		cpdim = maxdfi + ny;
		cpij = cp + maxdfi*(cpdim + 1);
		if (ny == 1)
		{
			*ss = *cpij;
		}
		else
		{
			inc2 = nterms + 1;
			ssij = ss;
			for (k = 0; k < ny; k++)
			{
				DCOPY(&ny, cpij, &inc1, ssij, &inc2);
				cpij += cpdim;
				ssij += inc2*ny;
			}
		}
	} /*for (iterm = 0; iterm < nterms; iterm++)*/

	emptyTrash();
	return (1);

  errorExit:
	emptyTrash();
	return (0);

} /*margSS()*/


/*
   Compute sequential sums of squares by combining REGSS into SS
   Note: compDF() must not be called before seqSS().
*/
void seqSS(void)
{	
	double       *ss = *SS, *regss = *REGSS, *df = *DF;
	long          icurr, iterm;
	long          dfi, maxdfi;
	long          j, ik1, ik1a, k1, k2, ik1k2, ik1k2a;
	long          nregx = (long) NREGX, nterms = (long) NTERMS;
	long          ny = (long) NY;
	long          step1, step2, step3, step4;
	
	step1 = nterms + 1;
	step2 = ny * step1;
	step3 = nregx + 1;
	step4 = ny * step3;
	
	icurr = 0;

	doubleFill(ss, 0.0, (nterms+1)*ny*ny); /* zero *SS */
	for (iterm = 0; iterm < nterms; iterm++)
	{
		dfi = 0;
		maxdfi = df[iterm];
		for (j = 0; j < maxdfi; j++)
		{						/* go through the cols for each term */
			if (regss[icurr] > -0.5)
			{
				dfi++;
				if(ny == 1)
				{
					ss[iterm] += regss[icurr];
				} /*if(ny == 1)*/
				else
				{
					ik1 = iterm; /* iterm + k1*step1 */
					ik1a = icurr; /* icurr + k1 * (nregx + 1) */
					for (k1 = 0; k1 < ny; k1++)
					{
						ik1k2 = ik1; /* iterm + k1 * step1 + k2 * step2 */
						ik1k2a = ik1a; /* icurr + k1 * (nregx + 1) + k2 * (ny * (nregx + 1)) */
						for (k2 = 0; k2 < ny; k2++)
						{
							ss[ik1k2] += regss[ik1k2a];
							/*
							   ss[iterm + k1 * step1 + k2 * step2] +=
								   regss[icurr + k1 * (nregx + 1) +
									k2 * (ny * (nregx + 1))];
							*/
							ik1k2 += step2;
							ik1k2a += step4;
						} /*for (k2 = 0; k2 < ny; k2++)*/
						ik1 += step1;
						ik1a += step3;
					} /*for (k1 = 0; k1 < ny; k1++)*/
				} /*if(ny == 1){}else{}*/
			} /*if (regss[icurr] > -0.5)*/
			icurr++;
		} /*for (j = 0; j < maxdfi; j++)*/
	} /*for (iterm = 0; iterm < nterms; iterm++)*/
} /*seqSS()*/

/*
   Compute DF by adding up non-aliased DF in each term.  It is assumed
   that DF already contains the maximum possible number per term.
*/
void compDF(void)
{	
	double       *regss = *REGSS, *df = *DF;
	long          errordf = NOTMISSING;
	long          icurr, iterm;
	long          dfi, maxdfi;
	long          j;
	long          nterms = (long) NTERMS;
	
	icurr = 0;

	for (iterm = 0; iterm < nterms; iterm++)
	{
		dfi = 0;
		maxdfi = df[iterm];
		for (j = 0; j < maxdfi; j++)
		{						/* go through the cols for each term */
			if (regss[icurr] > -0.5)
			{
				dfi++;
			} /*if (regss[icurr] > -0.5)*/
			icurr++;
		} /*for (j = 0; j < maxdfi; j++)*/
		errordf -= dfi;
		df[iterm] = (double) dfi;
	} /*for (iterm = 0; iterm < nterms; iterm++)*/

	df[nterms] = (double) errordf;
} /*seqDF()*/

/*
   count the number of aliased X-Variables

   980206 modified so that it always returns 0 after ipf() or a balanced
          anova()
*/

long nAliased(void)
{
	long        count = 0;
	
	if (!(PREVMODELTYPE & (IPF | ANOVA)) || (PREVGLMCONTROL & UNBALANCED))
	{
		long          icol, nregx = (long) NREGX;
		double       *regss = *REGSS;

		for (icol = 0; icol < nregx; icol++)
		{
			if (regss[icol] < -0.5)
			{
				count++;
			}
		}
	}
	
	return (count);
} /*nAliased()*/

/*
  Routine to initialize DF, NREGX, and allocate storage for REGX, XTXINV,
  REGCOEF, & REGSS, and compute the matrix of X-variables
  Called only by glm().

  Xsetup() != 0 if and only if storage was successfully assigned
*/

long             Xsetup(void)
{
	long            i;
	long            nterms = (long) NTERMS, ndata = (long) NDATA;
	long            nregx, ny = (long) NY;
	WHERE("Xsetup");
	
	/* first compute maximum possible df's */
	/* sum will be NREGX */

	nregx = 0;
	for (i = 0; i < nterms; i++)
	{			/* for each term */
		(*DF)[i] = (double) buildXmatrix(i, (double *) 0, MODELINFO);
		nregx += (long) (*DF)[i];
	}
	NREGX = (double) nregx;
	
	/* now get space */
	REGX = (double **) mygethandle((nregx * ndata) * sizeof(double));
	if (REGX != (double **) 0)
	{
		REGSS = 
		  (double **) mygethandle(((nregx + 1) * ny * ny) * sizeof(double));
	}
	
	if (REGX == (double **) 0 || REGSS == (double **) 0)
	{
		return (0);
	}

	if (MODELTYPE & DOCOEFS)
	{
		REGCOEF = (double **) mygethandle((nregx * ny) * sizeof(double));
		XTXINV = (double **) mygethandle((nregx * nregx) * sizeof(double));
		if(REGCOEF == (double **) 0 || XTXINV == (double **) 0)
		{
			return (0);
		}
	}
	
	/* now setup x */

	nregx = 0;
	for (i = 0; i < nterms; i++)
	{			/* for each term */
		nregx += buildXmatrix(i, *REGX + nregx*ndata, MODELINFO);
	}

	return (1);

} /*Xsetup()*/

#ifdef UNDEFINED__ /* see new code below */
/* Initialize upper triangle of (*XTXINV) to identity */
void xtxinvInit(void)
{
	long         i, j, ij;
	long         nregx = (long) NREGX;
	double      *xtxinv = *XTXINV;

	for (i = 0; i < nregx; i++)
	{
		ij = i*(nregx + 1); /* i + j*nregx */
		xtxinv[ij] = 1.0; /* xtx(i, i) = 1.0 */
		for (j = i + 1; j < nregx; j++)
		{
			ij += nregx;
			xtxinv[ij] = 0.0;
		}
	}
}
#else /*UNDEFINED__*/
/* Initialize all of XTXINV using memset */
void xtxinvInit(void)
{
	long         i;
	long         nregx = (long) NREGX;
	double      *xtxinv = *XTXINV;

	(void) memset((void *) xtxinv, 0, (size_t) nregx*nregx*sizeof(double));
	for (i = 0; i < nregx; i++)
	{
		*xtxinv = 1.0;
		xtxinv += nregx + 1;
	}
}
#endif /*UNDEFINED__*/

/*
  If regx != 0 set up columns of x matrix corresponding to the termi-th term
  in the model specified by info; otherwise just count cols needed.
  Return number of columns needed or generated.  This is the maximum number
  of degrees of freedom associated with the term (actual d.f. may be less)
*/

long buildXmatrix(long termi, double * regx, modelInfoPointer info)
{
	Symbolhandle    symh;
	long            i, j, level, term[MAXVARS];
	modelHandle     model = info->model;
	modelPointer    thisterm;
	long            ndata;
	long            nvars = info->nvars;
	double         *xvar, **x[MAXVARS];
	long            combo, ncells, ncols = 0;
	long           *nclasses;
	long            dim[2];
	WHERE("buildXmatrix");

	if (regx != (double *) 0)
	{
		(void) isMatrix(info->modelvars[0],dim);
		ndata = dim[0];

		for(j=0;j<nvars;j++)
		{
			symh = info->modelvars[j+1];
			x[j] = DATA(symh);
		}
	} /*if (regx != (double *) 0)*/
	
	nclasses = info->nclasses;
	
	thisterm = modelterm(model,termi);

	getterm(thisterm,term,nvars);

	ncells = setterm(term, nvars, nclasses);

	/* 
	   Loop to select the non-intrinsically aliased X-variables
	   and construct them.  There is potentially one for each
	   combination of indices in the term.
	   Traverses combinations with last subscript changing fastest
	*/
	for(combo = 0;combo < ncells;combo++)
	{
		/* check to see if each combination is already in through
		   lower order terms */
		if (!inEarlierTerm(termi, term, info))
		{
			/* if here, effective term needs to be entered */
			if(regx != (double *) 0)
			{/* set columns */
				for (i = 0; i < ndata; i++)
				{
					regx[i] = 1.0;
				}

				if(!modeltermEqual(thisterm,(modelPointer) NULLTERM))
				{
					for (j = 0; j < nvars; j++)
					{
						if(term[j] != 0)
						{/* variable in the term */
							xvar = *x[j];
							if (term[j] < 0)
							{	/* live variate */
								for (i = 0; i < ndata; i++)
								{
									if (!isMissing(xvar[i]))
									{
										regx[i] *= xvar[i];
									}
									else
									{
										regx[i] = 0.0;
									}
								} /*for (i = 0; i < ndata; i++)*/
							} /*if (term[j] < 0)*/
							else if (term[j] > 1)
							{	/* live factor */
								for (i = 0; i < ndata; i++)
								{
									if (!isMissing(xvar[i]))
									{
										level = (long) xvar[i];	/* category */
										if (level == nclasses[j])
										{ /* last group */
											regx[i] *= -1.;
										}
										else if (level != term[j] - 1)
										{
											regx[i] = 0.;
										}
									} /*if (!isMissing(xvar[i]))*/
									else
									{
										regx[i] = 0.;
									}
								} /*for (i = 0; i < ndata; i++)*/
							} /*if (term[j] < 0){}else if (term[j] > 1){}*/
						} /*if(term[j] != 0)*/
					}/*for (j = 0; j < nvars; j++)*/
				} /*if(!modeltermEqual(thisterm,(modelPointer) NULLTERM))*/
				regx += ndata;
			} /* if(regx != (double *) 0)*/
			ncols++;
		} /*if (!inEarlierTerm(termi, term, info))*/
		
		/* step odometer */
		stepGlmOdometer(term, nclasses, nvars, 1);
	}/* for(combo = 0;combo < ncells;combo++) */

	return (ncols);
} /*buildXmatrix()*/

/*
   Put in regx[] values a row of the X-matrix corresponding to factor
   and variate values in x, using the information in ino
*/
long buildXrow(double * x, double * regx, modelInfoPointer info)
{
	long            j, level, term[MAXVARS];
	modelHandle     model = info->model;
	modelPointer    thisterm;
	long            nvars = info->nvars, nterms = info->nterms;
	double          xvar;
	long            combo, ncells, ncols = 0;
	long           *nclasses = info->nclasses;
	long            termi;
	WHERE("buildXrow");

	for (j = 0; j < nvars; j++)
	{
		if (isMissing(x[j]))
		{
			setMissing(regx[0]);
			return (0);
		}
	} /*for (j = 0; j < nvars; j++)*/
	
	for (termi = 0; termi < nterms; termi++)
	{
		thisterm = modelterm(model, termi);
		getterm(thisterm, term, nvars);
		ncells = setterm(term, nvars, nclasses);
		/* 
		   Loop to select the non-intrinsically aliased X-variables
		   and construct them.  There is potentially one for each
		   combination of indices in the term.
		   Traverses combinations with last subscript changing fastest
		   */
		for(combo = 0;combo < ncells;combo++)
		{
			/* check to see if each combination is already in through
			   lower order terms */
			if (!inEarlierTerm(termi, term, info))
			{/* if here, effective term needs to be entered */
				regx[ncols] = 1.0;
				if(!modeltermEqual(thisterm,(modelPointer) NULLTERM))
				{
					for (j = 0; j < nvars; j++)
					{
						if(term[j] != 0)
						{	/* variable in the term */
							xvar = x[j];
							if (term[j] < 0)
							{ /* live variate */
								regx[ncols] *= xvar;
							} /*if (term[j] < 0)*/
							else if (term[j] > 1)
							{ /* live factor */
								level = (long) xvar;	/* category */
								if (level == nclasses[j])
								{ /* last group */
									regx[ncols] *= -1.;
								}
								else if (level != term[j] - 1)
								{
									regx[ncols] = 0.;
									break;
								}
							} /*if (term[j] < 0){}else if (term[j] > 1){}*/
						} /*if(term[j] != 0)*/
					} /*for (j = 0; j < nvars; j++)*/
				} /*if(!modeltermEqual(thisterm,(modelPointer) NULLTERM))*/
				ncols++;
			} /*if (!inEarlierTerm(termi, term, info))*/

			/* step odometer */
			stepGlmOdometer(term, nclasses, nvars, 1);
		} /* for(combo = 0;combo < ncells;combo++) */
	} /*for (termi = 0; termi < nterms; termi++)*/
	return (ncols);
} /*buildXrow()*/

/*
  Check to see that all variables are REAL and that their lengths match.
  also check that dependent variable is a vector or matrix and that each
  r.h.s variable is a vector.  If variates can ever be matrices, this
  will need to be modified. If all o.k. return the number of cases (ndata);
  otherwise return -1
*/

long checkVariables(modelInfoPointer info)
{
	Symbolhandle    symh;
	long            i, ndata = -1;
	long            dim[2];
	int             evaluated;
	char           *name;

	*OUTSTR = '\0';
	for(i=0;i<=info->nvars;i++)
	{
		symh = info->modelvars[i];
		
		evaluated = isevaluated(NAME(symh));
		name = (evaluated) ? getOneLabel(symh, 0, 0) : NAME(symh);
		if(TYPE(symh) != REAL)
		{
			sprintf(OUTSTR,"ERROR: non REAL variable %s in model", name);
		}
		else if(!isMatrix(symh,dim))
		{
			sprintf(OUTSTR,
					"ERROR: %s %s is not a vector%s",
					(i == 0) ? "dependent variable" : "variate or factor",
					name,(i == 0) ? " or matrix" : "");
		}
		else if(i > 0 && dim[1] > 1)
		{
			sprintf(OUTSTR,"ERROR: non-vector %s on r.h.s of model", name);
		}
		else if(i > 0 && dim[0] != ndata)
		{
			sprintf(OUTSTR,
					"ERROR: variable %s on r.h.s of model has length different from response",
					name);
		}
		else if(i == 0)
		{
			ndata = dim[0];
		}

		if(*OUTSTR)
		{
			goto errorExit;
		}
	} /*for(i=0;i<=nvars;i++)*/
	return (ndata);

  errorExit:
	putErrorOUTSTR();
		
	return (-1);
} /*checkVariables()*/
	
/*
  Return the number of cases that have missing values in either the
  dependent variable or any of the model variables.
  If misswts != (double ***) 0 and there are missing values, allocate a
  handle for the missing weights and set (**misswts)[i] to 0 or 1
  depending on whether case i contains missing values.  If there are
  no missing values no handle is allocated.
*/

long countMissing(modelInfoPointer info, double ***misswts)
{
	long            ndata, ny, nmissing = 0, dim[2];
	double        **y, **x[MAXVARS], *wts;
	long            i, j1, j2, ij;
	long            nvars = info->nvars;
	WHERE("countMissing");
	
	(void) isMatrix(info->modelvars[0],dim);
	ndata = dim[0];
	ny = dim[1];

	if(misswts != (double ***) 0)
	{
		*misswts = (double **) 0;
	}

	y = DATA(info->modelvars[0]);
	for(i = 0; i < nvars;i++)
	{
		x[i] = DATA(info->modelvars[i+1]);
	}
	
	for (i = 0; i < ndata; i++)
	{
		ij = i;					/* i + j1*ndata */
		for (j1 = 0; j1 < ny; j1++)
		{
			if (isMissing((*y)[ij]))
			{
				break;
			}
			ij += ndata;
		} /* for (j1 = 0; j1 < ny; j1++)*/
	
		if(j1 == ny)
		{
			for (j2 = 0; j2 < nvars; j2++)
			{
				if (isMissing((*x[j2])[i]))
				{
					break;
				}
			} /*for (j2 = 0; j2 < nvars; j2++)*/
		} /*if(j1 == ny)*/

		if(j1 < ny || j2 < nvars)
		{/* found missing */
			if(misswts != (double ***) 0)
			{
				if(nmissing == 0)
				{
					*misswts = (double **) mygethandle(ndata * sizeof(double));
					if (*misswts == (double **) 0)
					{
						goto errorExit;
					}
					wts = **misswts;
					doubleFill(wts, 1.0, ndata);
				}
				wts[i] = 0.0;
			} /*if(misswts != (double ***) 0)*/
			nmissing++;
		} /*if(j1 < ny || j2 < nvars)*/
	} /* for (i = 0; i < ndata; i++) */
	if(nmissing == ndata)
	{
		sprintf(OUTSTR,
				"ERROR: no cases without missing data");
		goto errorExit;
	} /*if(nmissing == ndata)*/
	
	return (nmissing);

  errorExit:
	mydisphandle((char **) *misswts);
	*misswts = (double **) 0;
	
	return (-1);
} /*countMissing()*/

static char    LinkName[15];
static char    DistName[15];

static char   *nameFromCode(glmCodePtr codeptr, unsigned long control)
{
	WHERE("nameFromCode");

	for (; codeptr->code != 0; codeptr++)
	{
		if (codeptr->code & control)
		{
			break;
		}
	}
	return (codeptr->name);

} /*nameFromCode()*/

static unsigned long nameToCode(glmCodePtr codeptr, char *name)
{
	char           c = name[0];
	char          *codename;
	long           length;
	WHERE("nameToCode");
	
	for (; codeptr->code != 0; codeptr++)
	{
		codename = codeptr->name;
		if (codename[0] == c)
		{ /* possible match*/
			length = codeptr->length;
			if (((length == 0) ?
				 strcmp(name, codename) : strncmp(name, codename, length)) == 0)
			{
				break;
			}
		} /*if (codeptr->name[0] == c)*/
	} /*for (; codeptr->code != 0; codeptr++)*/
	return (codeptr->code);
} /*nameToCode()*/

char     *distName(unsigned long control)
{
	char          *distname = nameFromCode(DistCodes, control);
	
	strcpy(DistName, distname);

	return (DistName);

} /*distName()*/

char     *linkName(unsigned long control)
{
	char          *linkname = nameFromCode(LinkCodes, control);
	
	strcpy(LinkName, linkname);

	return (LinkName);

} /*linkName()*/


unsigned long distControl(char *name)
{
	return (nameToCode(DistCodes, name));
} /*distControl*/

unsigned long linkControl(char *name)
{
	return (nameToCode(LinkCodes, name));
} /*linkControl*/


static char    GlmOpName[NAMELENGTH+1];

char * glmName(long op)
{
	char       *name;

	if (op & ANOVA)
	{
		name = (op & CASEWEIGHTS) ? "wtanova" : "anova";
	}
	else if (op & MANOVA)
	{
		name = (op & CASEWEIGHTS) ? "wtmanova" : "manova";
	}
	else if (op & FASTANOVA)
	{
		name = "fastanova";
	}
	else if (op & GLMREG)
	{
		name = "glmfit";
	}
	else if (op & IPF)
	{
		name = "ipf";
	}
	else if (op & LOGITREG)
	{
		name = "logistic";
	}
	else if (op & POISSONREG)
	{
		name = "poisson";
	}
	else if (op & OLSREG)
	{
		name = (op & CASEWEIGHTS) ? "wtregress" : "regress";
	}
	else if (op & ROBUSTREG)
	{
		name = "robust";
	}
	else if (op & LEAPS)
	{
		name = "screen";
	}
	else
	{
		name = "unknown";
	}
	strcpy(GlmOpName, name);
	return (GlmOpName);
} /*glmName()*/
