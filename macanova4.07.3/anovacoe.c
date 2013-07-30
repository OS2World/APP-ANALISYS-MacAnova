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
#pragma segment Anovacoe
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

#define Trash  NAMEFORTRASH

extern Symbolhandle    RSInstall(), RSInstall2(), RSInstall3(), Install();
extern void            Removesymbol();

extern long            isNull();
extern void            noData(), badNargs();
extern char           *isKeyword();
extern double          cellstat();

/* 
  This routine computes the model coefficients for linear models and glms.
  It does the simple thing for balanced linear models, and does some funny
  things for unbalanced models
  
  Usage: coefs() prints linear model coefficients, secoefs() also prints their
    std errors

  	coefs() or secoefs() : Coefficients for all terms
  	coefs(Term) or secoefs(Term) : Coefficients associated with given term
	secoefs(,varno) : All coefficients, variable varno (multivariate models)
	secoefs(Term,varno) : Coefficients for Term, variable varno (multivariate
	models)

	Keyword phrases se:F or coefs:F suppress the computation of either the
	standard errors or coefficients.  To have them both F is an error.
	The default values of se and coefs are F and T on coefs() and T and T on
	secoefs().

	When no specific term is specifed, keyword phrase byterm:F on secoefs()
	changes the form of the output from a structure with one component per
	term, each with components 'coefs' and 'se' to a structure with two
	componenents 'coefs' and 'se', each with one component for each term.
	byterm is ignored if a term is specified or if only coefs or only se are
	to be returned.

	For all usages of secoefs, one can use an extra argument of the form
	error:errorTerm where errorTerm is a CHAR variable which is the name of a
	term in the model that will be used to compute the MSE needed for SE's.
	This is ignored on iterative models (poisson(), logistic(), robust(),
	for which MSE is taken as 1.

	Term and errorTerm can either be CHARACTER variables (e.g., "a.b" or 
	"ERROR1" or a term number such as 1 (for CONSTANT).

	For reasons of backwards compatibility, 'anovacoefs' is synonymous with
    'coefs'.

	950828 fixed bug causing crashes on secoefs(coefs:F) when a uninitialized
	handle was being disposed of (in doBalanced()).
	
	971126 Changed calls to appendLabels to use enum codes in labels.h

    990212 Changed most uses of putOUTSTR() to putErrorOUTSTR() and
           some use of myerrorout to putOutErrorMsg()
    990215 Remaining uses of myerrorout() changed to putOutMsg()
*/

static Symbolhandle    Trash;

enum coefsTrash
{
	GTMPY = 0,
	GTMPMEANS,
	GTMPALPHA,
	GTMPWTS,
	GCOLPTR,
	NTRASH
};


/* get coords for the jth value in table */
static void getcoords(long j, long dim, long coords[], long *offsets)
{
	long            i;
	WHERE("getcoords");
	
	for (i = dim - 1; i >= 0; i--)
	{
		coords[i] = j / offsets[i];
		j -= coords[i] * offsets[i];
	}
} /*getcoords()*/

/*
   Set dimensions appropriately for coefficients or  standard error
   symbol for a particular term. 

   Dimension for each variable in term are the number of factor levels for
   a factor or 1 for a variate.  If y is multidimensional, we need an extra
   final dimension of ny

   It returns the product of dimensions, excluding ny if multidimensional.
*/
static long setSymbolDims(Symbolhandle symh, long term[])
{
	long          i, ndims = 0, imax = 1, nvars = (long) NVARS;
	long          nclass, ny = (long) NY;
	
	setDIM(symh,1,1);

	for (i = 0; i < nvars; i++)
	{
		if (term[i] > 0)
		{
			nclass = (NCLASSES[i] > 0) ? NCLASSES[i] : 1;
			imax *= nclass;
			setDIM(symh,++ndims,nclass); /* get dimensions for results */
		}
	} /*for (i = 0; i < nvars; i++)*/
	ndims = (ndims) ? ndims : 1;
	if (ny > 1)
	{
		setDIM(symh, ++ndims, ny);
	}
	setNDIMS(symh,ndims);
	return (imax);
} /*setSymbolDims()*/

/*
   set dimensions and get space for coefficients & std errors if needed
   Returns the product of dimensions if no problems; otherwise -1
   result is either a REAL symbol (just 1 term, only coefs or ses) or
   a structure (> 1 term or both coefs and ses requested)

   On entry, space has not been allocated

   980817 changed labelling in multivariate case so that the first coordinates
          get numeric labels rather than no labels
*/

static long setSymbols(Symbolhandle result, long jterm, long term[],
					   long do_coefs,long do_ses, long byterm,
					   Symbolhandle * pCoefsSH, Symbolhandle * pSesSH)
{
	Symbolhandle         symh;
	int                  oneTerm = (jterm < 0);
	long                 ny = (long) NY, imax;
	WHERE("setSymbols");
	
	/* first set *pCoefsSH and *pSesSH */
	if (do_coefs && do_ses)
	{ /* both coefficients and standard errors */
		if (oneTerm)
		{
			*pCoefsSH = COMPVALUE(result, 0);
			*pSesSH = COMPVALUE(result, 1);
		} /*if (oneTerm)*/
		else
		{
			if (byterm)
			{
				symh = COMPVALUE(result, jterm);
				*pCoefsSH = COMPVALUE(symh, 0);
				*pSesSH = COMPVALUE(symh, 1);
			} /*if (byterm)*/
			else
			{
				symh = COMPVALUE(result,0);
				*pCoefsSH = COMPVALUE(symh, jterm);
				symh = COMPVALUE(result,1);
				*pSesSH = COMPVALUE(symh, jterm);
			} /*if (byterm){}else{}*/
		} /*if (oneTerm){}else{}*/
	} /*if (do_coefs && do_ses)*/
	else
	{ /* either coefficients or standard errors, but not both */
		if (oneTerm)
		{
			*pCoefsSH = result;
		}
		else
		{
			*pCoefsSH = COMPVALUE(result, jterm);
		}
		if (do_ses)
		{
			*pSesSH = *pCoefsSH;
			*pCoefsSH = (Symbolhandle) 0;
		}
		else
		{
			*pSesSH = (Symbolhandle) 0;
		}
	} /*if (do_coefs && do_ses){}else{}*/

	/* now set dimensions and allocate space */
		
	if (do_coefs)
	{
		imax = setSymbolDims(*pCoefsSH, term);
		TMPHANDLE = mygethandle(ny * imax * sizeof(double));
		if (TMPHANDLE == (char **) 0)
		{
			goto errorExit;
		}
		setDATA(*pCoefsSH, (double **) TMPHANDLE);
	}

	if (do_ses)
	{
		imax = setSymbolDims(*pSesSH, term);
		TMPHANDLE = mygethandle(ny * imax * sizeof(double));
		if (TMPHANDLE == (char **) 0)
		{
			goto errorExit;
		}
		setDATA(*pSesSH, (double **) TMPHANDLE);
	}

	if (ny > 1 && HASLABELS(MODELVARS[0]))
	{
		Symbolhandle symh1 = (do_coefs) ? *pCoefsSH : *pSesSH;
		long         ndims = NDIMS(symh1);
		long         dims[MAXDIMS];
		long         lengths[MAXDIMS];
		char        *labels[MAXDIMS];
		char         numericLabel[2];
		long         idim;
		char        *depvLabs[2];
		long         depvLabLengths[2];
		
		numericLabel[0] = NUMERICLABEL;
		numericLabel[1] = '\0';
		
		for (idim = 0; idim < ndims; idim++)
		{
			labels[idim] = numericLabel;
			dims[idim] = DIMVAL(symh1, idim+1);
			lengths[idim] = 2*dims[idim];
		}
		
		getMatLabels(MODELVARS[0], depvLabs, depvLabLengths);
		lengths[ndims-1] = depvLabLengths[1];
		if (do_coefs)
		{
			TMPHANDLE = createLabels(ndims, lengths);
			if (TMPHANDLE == (char **) 0 || !setLabels(*pCoefsSH, TMPHANDLE)) 
			{
				mydisphandle(TMPHANDLE);
				goto errorExit;
			}
		} /*if (do_coefs)*/

		if (do_ses)
		{
			TMPHANDLE = createLabels(ndims, lengths);
			if (TMPHANDLE == (char **) 0 || !setLabels(*pSesSH, TMPHANDLE)) 
			{
				mydisphandle(TMPHANDLE);
				goto errorExit;
			}
		} /*if (do_ses)*/
		/* Get column labels again since they may have moved */
		getMatLabels(MODELVARS[0], depvLabs, depvLabLengths);
		labels[ndims-1] = depvLabs[1];
		if (do_coefs)
		{
			for (idim = 0; idim < ndims; idim++)
			{
				appendLabels(*pCoefsSH, labels[idim], idim,
							 (idim < ndims-1) ? doExpand : dontExpand);
			}
		}
		if (do_ses)
		{
			for (idim = 0; idim < ndims; idim++)
			{
				appendLabels(*pSesSH, labels[idim], idim,
							 (idim < ndims-1) ? doExpand : dontExpand);
			}
		}
	} /*if (ny > 1 && HASLABELS(MODELVARS[0]))*/ 
	return (imax);

  errorExit:
	return (-1);
} /*setSymbols()*/

/*
	Code to work with (pseudo) balanced case.  Removed from anovacoefs()
	so as to shorten that code
	We just work through the "hand computations"
*/
static Symbolhandle doBalanced(long do_coefs, long do_ses, long byterm,
							   long icurrmin, long icurrmax,
							   long errorterm, Symbolhandle result)
{
	Symbolhandle    coefsSH, sesSH;
	long            ncells = 1;
	long            ndata = (long) NDATA, nvars = (long) NVARS;
	long            term[MAXVARS];
	long            oneTerm = (icurrmin == icurrmax);
	long            i, icurr, imax;
	double          dfe = (*DF)[errorterm], mse;
	double        **tmpyH = (double **) 0, **tmpMeansH = (double **) 0;
	double        **coefsH, **sesH;
	double         *y, *residuals, *tmpy, *ses;
	WHERE("doBalanced");
	
	ncells = 1;
	for (i = 0; i < nvars; i++)
	{
		ncells *= NCLASSES[i];
	}
	if (do_coefs)
	{
		if (!getScratch(tmpyH,GTMPY,ndata,double) ||
			!getScratch(tmpMeansH,GTMPMEANS,ncells,double))
		{
			goto errorExit;
		}
		y = *Y;
		residuals = *RESIDUALS;
		tmpy = *tmpyH;
		for (i = 0; i < ndata; i++)
		{
			tmpy[i] =  (PREVMODELTYPE & IPF) ?
				log(y[i] - residuals[i]) : y[i];
		} /*for (i = 0; i < ndata; i++)*/

		for (icurr = 0; icurr < icurrmin; icurr++)
		{/* sweep out prior stuff */
			getterm(modelterm(MODEL,icurr), term, nvars);
			/* means */
			(void) cellstat(tmpyH, term, tmpMeansH,
							(double **) 0, (double **) 0, CELLMEANSANDSWEEP);
		}
		discardScratch(tmpMeansH,GTMPMEANS,double);
	} /*if (do_coefs)*/
	
	if (do_ses > 0)
	{
		mse = (*SS)[errorterm]/dfe;
	}
	
	for (icurr = icurrmin; icurr <= icurrmax; icurr++)
	{
		getterm(modelterm(MODEL,icurr), term, nvars);
	/* set dimensions and get space for coefficients & std errors if needed */
		imax = setSymbols(result, (oneTerm) ? -1 : icurr - icurrmin, term,
				   do_coefs, do_ses, byterm, &coefsSH, &sesSH);
		if (imax < 0)
		{
			goto errorExit;
		}
		

		/* coefsSH has coefs, sesSH has se's, if requested */

		if (do_coefs)
		{
			coefsH = DATA(coefsSH);
	/* tmpyH now has residuals after sweeping out terms 0 through icurr-1 */
			(void) cellstat(tmpyH, term, coefsH, (double **) 0,
							(double **) 0, CELLMEANSANDSWEEP);
	/* tmpyH now has residuals after sweeping out terms 0 through icurr */
		} /*if (do_coefs)*/
		
		if (do_ses)
		{
			sesH = DATA(sesSH);
			ses = *sesH;
			if (do_ses > 0)
			{
				doubleFill(ses, sqrt((*DF)[icurr] * mse /NDATA), imax);
			}
			else
			{
				for (i = 0; i < imax; i++)
				{
					setMissing(ses[i]);
				}
			}
		} /*if (do_ses)*/
	} /* for (icurr = icurrmin; icurr <= icurrmax; icurr++) */

	discardScratch(tmpyH,GTMPY,double);
	return (result);

  errorExit:
	discardScratch(tmpyH,GTMPY,double);
	return (0);
} /*doBalanced()*/

/* Also defined in glmutils.c */
#define ALIASED     -1
#define PIVOTED     -2

#define TRANSPOSED   1

/*
   function to compute the coefficients for term termno of the 
   current model.  If standard errors are required, variances are
   taken from term errorterm.
*/
static int calcCoefs(long varno, long termno, long errorterm, long colno, 
					 long do_ses, Symbolhandle coefsSH, Symbolhandle sesSH)
{
	Symbolhandle     symh;
	double         **tmpWtsH = (double **) 0;
	double         **tmpAlphaH = (double **) 0;
	double          *tmpWts, *tmpAlpha;
	double          *coefs;
	double          *ses;
	double          *xtxinv, *ss;
	double           tmpWtsi, tmpWtsj;
	double           cjj, mse, scale = PREVGLMSCALE;
	double           dfe = (*DF)[errorterm];
	long           **colptrH = (long **) 0, *colptr;
	long             oldcoords[MAXVARS], newcoords[MAXVARS];
	long            *term = oldcoords;
	long             offsets[MAXVARS];
	long             dims[MAXDIMS];
	long             addin;
	long             i, j, j2, ij, ik, ikk, jk, k, k1;
	long             colptri, colptrj;
	long             df;
	long             ndims, imax;
	long             ny = (varno == 0) ? (long) NY : 1;
	long             nterms = (long) NTERMS;
	long             nvars = (long) NVARS, nregx = (long) NREGX;
	int              do_coefs = (coefsSH != (Symbolhandle) 0);
	int              useScale =
		(do_ses > 0) && (PREVGLMCONTROL & NONNORMALDIST) != 0;
	char            *termname;
	WHERE("calcCoefs");

	symh = (do_coefs) ? coefsSH : sesSH;

	getterm(modelterm(MODEL, termno), term, nvars);
	offsets[0] = 1;
	imax = 1;  /* maximum number of coefficients per variable */
	ndims = 0;
	for (j = 0; j < nvars; j++)
	{
		if (term[j] > 0)
		{
			offsets[ndims] = imax;
			dims[ndims] = DIMVAL(symh, ndims + 1);
			imax *= dims[ndims++];
		} /*if (term[j] > 0)*/
	} /*for (j = 0; j < nvars; j++)*/
	ndims = (ndims == 0) ? 1 : ndims;

	if (!getScratch(tmpWtsH,GTMPWTS,imax,double))
	{
		goto errorExit;
	}

	/*
	   get coefficients of X-variables
	*/
	df = getAlpha(termno, colno, (do_coefs) ? &tmpAlphaH : (double ***) 0,
				  &colptrH, TRANSPOSED);
	toTrash(tmpAlphaH,GTMPALPHA);
	toTrash(colptrH,GCOLPTR);
	if (df < 0)
	{
		goto errorExit;
	}

	colno += df;
	/* no more storage to be allocated; safe to dereference handles */

	/* 
	  Now rearrange and fix up marginal terms;
	  warn if pivoted term included
	  Note: (*colptrH)[0] contains ncells
	*/
	colptr = *colptrH + 1;
	/* 
	   colptr[i] has column number of i-th coefficient if it has not
	   been aliased (colptr[i] = ALIASED) or pivoted out (colptr[i] =
	   PIVOTED)
	*/
	for (j = 0; j < imax; j++)
	{
		if (colptr[j] == PIVOTED)
		{
			termname = skipStrings(*TERMNAMES, termno);
			sprintf(OUTSTR,
					"WARNING: Missing df(s) in term %s", termname);
			putErrorOUTSTR();
			if (PRINTWARNINGS)
			{
				putOutMsg("Missing effects set to zero");
			}
			break;
		} /*if (colptr[j] == PIVOTED)*/
	} /*for (j = 1; j <= imax; j++)*/

	/* start by filling in body with zeros */

	if (do_coefs)
	{
		coefs = DATAPTR(coefsSH);
		doubleFill(coefs, 0.0, imax * ny);
	}
	else
	{
		coefs = (double *) 0;
	}

	if (do_ses)
	{
		ses = DATAPTR(sesSH);
		doubleFill(ses, 0.0, imax * ny);
	}
	else
	{
		ses = (double *) 0;
	}

	/*
		Now look at each value, add it in where it is marginal, and
		subtract off where it is internal
	*/
	xtxinv = *XTXINV;
	ss = *SS;
	tmpWts = *tmpWtsH;
	if (do_coefs)
	{
		tmpAlpha = *tmpAlphaH;
	}

/*
   loop over coefficients (or vectors of coefficients if ny > 0) to be
   computed
*/
	for (j = 0; j < imax; j++)
	{
		getcoords(j, ndims, newcoords, offsets);
		for (k = 0; k < ndims; k++)
		{
			newcoords[k]++;
		}
	/*
	   loop over original coefficients to create weights in tmpWts
	   for the linear combination that will yield the j-th
	   coefficient
	*/
		for (i = 0; i < imax; i++)
		{
			/* first index changes fastest*/
			getcoords(i, ndims, oldcoords, offsets);

			/*
			   Note that old coordinates run from 0 to dims[i] - 1 and
			   new coordinates run from 1 to dims[i].

			   Include with weight +- 1 any old coefficient each of whose
			   coordinates "matches" the corresponding coordinate of the
			   new coefficient.  If there are any non-matches, the old
			   coefficient gets weight 0.

			   Cooardinates "match" if
			    (a) they are identical;
			    (b) the old coordinate is 0;
			   or
				(c) the new coordinate is at its maximum level.
			   They are included with coefficient (-1)^k where k = number
			   of coordinates for which (c) is true but not (b).

			   Note that (a) is incompatible with either (b) or (c)

			   Coordinates do not "match" if the old coordinate is not 0
			   and the new coordinate < dim[i] but is not equal to the old
			   coordinate.
			  */

			addin = 1;
			for (k = 0; k < ndims; k++)
			{
				if (oldcoords[k] > 0)
				{ /* possible non-"match" */
					/* check for max coords */
					if (newcoords[k] == dims[k])
					{ /* "match" */
						addin *= -1;
					}
					else if (newcoords[k] != oldcoords[k])
					{ /* non "match" */
						addin = 0;
						break;
					}
				} /*if (oldcoords[k] > 0)*/
			} /*for (k = 0; k < ndims; k++)*/
			tmpWts[i] = (double) addin;
		} /*for (i = 0; i < imax; i++)*/

		if (do_ses > 0)
		{
			cjj = 0.;
			for (i = 0; i < imax; i++)
			{
				colptri = colptr[i];
				if (colptri >= 0)
				{ /* not aliased or pivoted */
					tmpWtsi = tmpWts[i];
					for (j2 = 0; j2 < imax; j2++)
					{/* not aliased or pivoted */
						colptrj = colptr[j2];
						if (colptrj >= 0)
						{
							tmpWtsj = tmpWts[j2];
							ij = colptri + colptrj * nregx;			
							cjj += tmpWtsi * tmpWtsj * xtxinv[ij];
						}
					} /*for (j2 = 0; j2 < imax; j2++)*/
				} /*if (colptri >= 0)*/
			} /*for (i = 0; i < imax; i++)*/
		} /*if (do_ses > 0)*/

		/* compute linear combination  of elements in tmpAlpha */
		jk = j; /* j + k*imax */
		for (k = 0; k < ny; k++)
		{
			k1 = (varno != 0) ? varno - 1 : k;
			if (do_coefs)
			{
				ik = k1 * imax; /* i+k1*imax */
				for (i = 0; i < imax; i++)
				{
					coefs[jk] += tmpAlpha[ik] * tmpWts[i];
					ik++;
				} /*for (i = 0; i < imax; i++)*/
			} /*if (do_coefs)*/

			if (do_ses)
			{
#if (0)
				if (PREVMODELTYPE & ROBUSTREG)
				{
					ses[jk] = sqrt(fabs(cjj * RobBeta)) *
						ROBUSTVARK * CURRENTRSC;
					/* variance formula from page 40 of Huber 1977 */
				}
				else if (PREVMODELTYPE & ITERGLMS)
#endif
				if (useScale)
				{
					ses[jk] = scale*sqrt(fabs(cjj));
				}
				else
				{
					if (do_ses > 0)
					{
						ikk = k1*(nterms+1)*(ny+1) + errorterm;
						mse = ss[ikk]/dfe;
						ses[jk] = sqrt(fabs(cjj * mse));
					}
					else
					{
						setMissing(ses[jk]);
					}
				}
			} /*if (do_ses)*/
			jk += imax;
		} /*for (k = 0; k < ny; k++)*/
	} /*for (j = 0; j < imax; j++)*/

	if (do_coefs)
	{
		discardScratch(tmpAlphaH,GTMPALPHA,double);
	}
	discardScratch(tmpWtsH,GTMPWTS,double);
	discardScratch(colptrH,GCOLPTR,long);

	return (colno);
	
  errorExit:
	return (-1);
	
} /*calcCoefs()*/

/*
   960310  Modified so labels containing full term names are added
           to structure output when they will not fit on the labels
           If memory isn't available for labels, they are not added but
           it isn't an error that causes termination

   990318  Fixed bug in adding labels with byterm:F
*/
Symbolhandle    anovacoefs(Symbolhandle list)
{
	long            i, i2, colct = 0, icurr, j, k;
	long            term[MAXVARS];
	long            type, margs;
	long            do_ses, do_coefs = 1, oneTerm;
	long            icurrmin, icurrmax;
	long            nargs, varno = 0;
	long            ny = (long) NY;
	long            nterms = (long) NTERMS, errorterm = -1;
	long            nvars = (long) NVARS;
	long            ncomps;
	long            byterm = 1;
	long            useScale = 0;
	long            needlabels = 0, labelLength;
	char           *compNames[2], *keyword = (char *) 0, *glmname, *termname;
	char            compNames2[9];
	char           *compNames2P = compNames2, **compNames2H = &compNames2P;
	Symbolhandle    symh, symh2, symhkey, result = (Symbolhandle) 0;
	Symbolhandle    coefsSH = (Symbolhandle) 0, sesSH = (Symbolhandle) 0;
	double          tmp, dfe;
	WHERE("anovacoefs");

	Trash = GarbInstall(NTRASH);
	if (Trash == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	*OUTSTR = '\0';
	if (strcmp(FUNCNAME,"anovacoefs") == 0)
	{
		putOutMsg("NOTE: anovacoefs() is obsolete; use coefs() instead");
	}

	do_ses = (strcmp(FUNCNAME, "secoefs") == 0);
	margs = 6;
	
	if (PREVMODELTYPE == 0)
	{
		sprintf(OUTSTR,"ERROR: no active model");
	}
	else if (PREVMODELTYPE & LEAPS)
	{
		sprintf(OUTSTR,"ERROR: %s cannot be used after screen()",FUNCNAME);
	}
	else if (!(PREVMODELTYPE & DOCOEFS))
	{
		sprintf(OUTSTR,
				"ERROR: %s cannot be used after GLM command with coefs:F",
				FUNCNAME);
	}
	if (*OUTSTR)
	{
		goto errorExit;
	}
	
	nargs = NARGS(list);

	if (nargs > margs)
	{
		badNargs(FUNCNAME,-margs);
		goto errorExit;
	}
	
	for (i = 0; i < nargs;i++)
	{
		symhkey = COMPVALUE(list,i);
		if (i > 0 && symhkey == (Symbolhandle) 0)
		{
			(void) argOK(symhkey,0,i+1);
			goto errorExit;
		}
		
		if ((keyword = isKeyword(symhkey)))
		{
			type = TYPE(symhkey);
			if (strncmp(keyword,"err",3) == 0)
			{				
				if (!isCharOrString(symhkey) &&
					!isInteger(symhkey,POSITIVEVALUE))
				{
					sprintf(OUTSTR,
							"ERROR: %s not CHARACTER variable or string or term number",
							keyword);
					goto errorExit;
				}

				if (type == REAL)
				{
					tmp = DATAVALUE(symhkey,0);
					if (tmp > (double) (nterms+1))
					{
						sprintf(OUTSTR,
								"ERROR: %s term number is not integer between 1 and %ld",
								keyword,nterms+1);
						goto errorExit;
					}
					errorterm = (long) tmp - 1;
				}
				else
				{
					i2 = 0;
					for (errorterm = 0;errorterm <= nterms;errorterm++)
					{			/* try to match it */
						if (strcmp(*TERMNAMES + i2,STRINGPTR(symhkey)) == 0)
						{
							break;
						}
						i2 += strlen(*TERMNAMES + i2) + 1;
					}
					if (errorterm > nterms)
					{
						sprintf(OUTSTR,
								"ERROR: %s is not a term in the model %s",
								STRINGPTR(symhkey), *STRMODEL);
						goto errorExit;
					}
				}
			} /*if (strncmp(keyword,"err",3) == 0)*/
			else if (strncmp(keyword,"se",2) == 0 ||
					 strncmp(keyword,"coef",4) == 0)
			{
				if (!isTorF(symhkey))
				{
					notTorF(keyword);
					goto errorExit;
				}
				if (*keyword == 's')
				{
					do_ses = (DATAVALUE(symhkey,0) != 0.0);
				}
				else
				{
					do_coefs = (DATAVALUE(symhkey,0) != 0);
				}
			}
			else if (strcmp(keyword,"byterm") == 0)
			{
				if (!isTorF(symhkey))
				{
					notTorF(keyword);
					goto errorExit;
				}
				byterm = (DATAVALUE(symhkey,0) != 0.0);
			}
			else
			{
				badKeyword(FUNCNAME,keyword);
				goto errorExit;
			}
			setNAME(symhkey,USEDKEYWORD);
		} /*if ((keyword = isKeyword(symhkey)))*/
	} /*for (i = 0; i < nargs;i++)*/

	if (PREVMODELTYPE & (IPF | FASTANOVA))
	{
		glmname = (PREVMODELTYPE & IPF) ? "ipf" : "fastanova";
		if (!do_coefs)
		{
			sprintf(OUTSTR,
					"ERROR: coefs:F is illegal with %s() after %s()",
					FUNCNAME, glmname);
		}
		else if (do_ses)
		{
			sprintf(OUTSTR,
					"ERROR: standard errors not available after %s()",
					glmname);
		}		
		if (*OUTSTR)
		{
			goto errorExit;
		}
	} /*if (PREVMODELTYPE & (IPF | FASTANOVA))*/
	
	if (nargs > 1 && !isKeyword(symh2 = COMPVALUE(list,1)))
	{/* should be variable number */
		if (!argOK(symh2,REAL,2))
		{
			goto errorExit;
		}
		tmp = DATAVALUE(symh2,0);
		if (!isScalar(symh2) || tmp != floor(tmp) || tmp <= 0 || tmp > NY)
		{
			sprintf(OUTSTR,
					"ERROR: variable number must be positive integer <= %ld",
					ny);
		}
		if (*OUTSTR)
		{
			goto errorExit;
		}
		varno = (long) tmp;
		nargs--;
		ny = 1;
	} /*if (nargs > 1 && !isKeyword(symh2 = COMPVALUE(list,1)))*/
	
	ncomps = do_coefs + do_ses;
	if (ncomps == 0)
	{
		sprintf(OUTSTR,
				"ERROR: neither coefficients or standard errors requested");
		goto errorExit;
	}
	
	if (errorterm < 0)
	{
		errorterm = nterms;
	}
	else if (!do_ses)
	{
		sprintf(OUTSTR,
				"WARNING: keyword 'error' ignored when no standard errors requested by %s",FUNCNAME);
		putErrorOUTSTR();
	}
	
	symh = COMPVALUE(list, 0);
	/* icurrmin and icurrmax are the first and last terms for which
		we want coefficients */

	if (symh == (Symbolhandle) 0 || isKeyword(symh))
	{
		icurrmin = 0;
		icurrmax = nterms - 1;	/* do all terms */
	}
	else
	{
		type = TYPE(symh);
		if (isscratch(NAME(symh)) &&
			(!isScalar(symh) || type != REAL && type != CHAR))
		{
			sprintf(OUTSTR,
					"ERROR: 1st argument to %s() does not specify a term in %s",
					FUNCNAME, *STRMODEL);
			goto errorExit;
		}
		else if (isScalar(symh) && type == REAL)
		{
			tmp = DATAVALUE(symh,0);
			if (tmp != floor(tmp) || tmp <= 0.0 || tmp > (double) nterms)
			{
				sprintf(OUTSTR,
						"ERROR: specified term number integer between 1 and %ld",
						nterms);
				goto errorExit;
			}
			i = (long) tmp - 1;
		}
		else
		{
			termname = (isScalar(symh) && type == CHAR) ?
				STRINGPTR(symh) : NAME(symh);
			i2 = 0;
			for (i = 0; i < nterms; i++)
			{					/* try to match term */
				if (strcmp(termname, *TERMNAMES + i2) == 0)
				{
					break;
				}
				i2 += strlen(*TERMNAMES + i2) + 1;
			}
			if (i >= nterms)
			{
				sprintf(OUTSTR, "ERROR: %s is not in model %s",
						termname, *STRMODEL);
				goto errorExit;
			}
		}
		icurrmin = i;
		icurrmax = i;
	}
	oneTerm = (icurrmin == icurrmax);
	
	compNames[0] = compNames2;
	strcpy(compNames[0],(do_coefs) ? "coefs" : "se");
	if (ncomps == 2)
	{
		compNames[1] = compNames2+6;
		strcpy(compNames[1], "se");
	}
	
	dfe = (*DF)[errorterm];

	if (do_ses)
	{
		/* if this is called by the function secoefs, we want to compute se's
		for the coefs.  do_ses is a flag that says to do se's */

		useScale = (PREVGLMCONTROL & NONNORMALDIST) != 0;

		if (dfe <= 0.0 && (!useScale || PREVGLMSCALE <= 0.0))
		{
			putOutErrorMsg("WARNING: no degrees of freedom for error; standard errors set MISSING");
			do_ses = -1;
			useScale = 0;
		}
			
		if (useScale)
		{
			strcpy(OUTSTR,
				   "NOTE: standard errors assume scale parameter is ");
			formatDouble(OUTSTR + strlen(OUTSTR), PREVGLMSCALE, TRIMLEFT);
			putOUTSTR();
		} /*if (useScale)*/
	} /*if (do_ses)*/
	
	if (!oneTerm)
	{
		char        *pos = *TERMNAMES;

		for (i = 0; i < nterms; i++)
		{
			if (strlen(pos) > NAMELENGTH)
			{
				break;
			}
			pos = skipStrings(pos, 1);
		} /*for (i = 0; i < nterms; i++)*/
		if (i < nterms)
		{
			needlabels = 1;
			labelLength = skipStrings(pos, nterms - i) - *TERMNAMES;
		}
	} /*if (!oneTerm)*/

	/* Symbols for results; actual space not yet allocated */
	if (ncomps == 2)
	{
		if (oneTerm)
		{						/* single term with standard errors */
			result = RSInstall(SCRATCH, 2L, compNames, 0L);
		}
		else
		{						/* all terms with standard errors */
			if (byterm)
			{
				result = RSInstall3(SCRATCH,nterms, TERMNAMES, 
									2L, compNames, 0L);
				if (needlabels && result != (Symbolhandle) 0)
				{
					TMPHANDLE = mygethandle(labelLength);

					if (TMPHANDLE != (char **) 0 &&
						setLabels(result, TMPHANDLE))
					{
						copyStrings(*TERMNAMES, *TMPHANDLE, nterms);
					}
					else
					{
						mydisphandle(TMPHANDLE);
					}
				}
			}
			else
			{
				result = RSInstall3(SCRATCH, 2, compNames2H,
									nterms, (char **) 0, 0);
				if (result != (Symbolhandle) 0)
				{
					for (i = 0; i < 2;i++)
					{
						k=0;
						symh = COMPVALUE(result,i);
						for (j = 0; j < nterms;j++)
						{
							setNAME(COMPVALUE(symh,j), *TERMNAMES + k);
							k += strlen(*TERMNAMES + k) + 1;
						} /*for (j = 0; j < nterms;j++)*/
						if (needlabels)
						{
							TMPHANDLE = mygethandle(labelLength);
							if (TMPHANDLE != (char **) 0 &&
								setLabels(symh, TMPHANDLE))
							{
								copyStrings(*TERMNAMES, *TMPHANDLE, nterms);
							}
							else
							{
								mydisphandle(TMPHANDLE);
							}
						}
					} /*for (i = 0; i < 2;i++)*/
				} /*if (result != (Symbolhandle) 0)*/
			} /*if (byterm)*/
		} /* if (oneTerm){}else{}*/
	} /*if (ncomps == 2)*/
	else
	{
		if (oneTerm)
		{/* single term with only coefs or only standard errors */
			result = Install(SCRATCH, REAL);
		}
		else
		{/* all terms with only coefs or only standard errors */
			result = RSInstall2(SCRATCH, nterms, TERMNAMES, 0L);
			if (needlabels && result != (Symbolhandle) 0)
			{
				TMPHANDLE = mygethandle(labelLength);
				
				if (TMPHANDLE != (char **) 0 && setLabels(result, TMPHANDLE))
				{
					copyStrings(*TERMNAMES, *TMPHANDLE, nterms);
				}
				else
				{
					mydisphandle(TMPHANDLE);
				}
			}
		}
	} /*if (ncomps == 2){}else{}*/

	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	if (!(PREVGLMCONTROL & UNBALANCED))
	{/* (pseudo) balanced data */
		
		result = doBalanced(do_coefs, do_ses, byterm, icurrmin, icurrmax,
							errorterm, result);
		if (result == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		emptyTrash();
	
		return (result);
	} /*if (!(PREVGLMCONTROL & UNBALANCED))*/

	/* now the unbalanced case */

	/* find number of columns in on REGX matrix */
	colct = 0;
	for (i = 0; i < icurrmin; i++)
	{		/* for each term */
		colct += buildXmatrix(i, (double *) 0, MODELINFO);
	}

	for (icurr = icurrmin; icurr <= icurrmax; icurr++)
	{
		getterm(modelterm(MODEL, icurr), term, nvars);

		if (setSymbols(result, (oneTerm) ? -1 : icurr - icurrmin, term, 
					   do_coefs, do_ses, byterm, &coefsSH, &sesSH) < 0)
		{
			goto errorExit;
		}
		/* coefsSH and/or sesSH are already part of result */
		colct = calcCoefs(varno, icurr, errorterm, colct, do_ses,
						  coefsSH, sesSH);
		if (colct < 0)
		{
			goto errorExit;
		}
	} /*for (icurr = icurrmin; icurr <= icurrmax; icurr++)*/

	emptyTrash();
	
	return (result);
	
  errorExit:
	putErrorOUTSTR();
	
	emptyTrash();
	
	Removesymbol(result);

	return (0);
} /*anovacoefs()*/

