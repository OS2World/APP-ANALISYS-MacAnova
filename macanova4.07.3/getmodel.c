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
#pragma segment Getmodel
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
/*
   950427	removed explicit reference to keywords and added arguments
            containing the values of keywords, offset, n, and wts

   990212   Changed most uses of putOUTSTR() to putErrorOUTSTR() and
            myerrorout() to putOutErrorMsg()
*/

#define GCTS     0
#define NTRASH   1

#define ONEWAYBALANCE       1
#define TWOWAYBALANCE       2
#define COMPLETEBALANCE     3

#if (0)
#define isVariate(I)  (NCLASSES[I] <= 1)
#else
#define isVariate(I)  (NCLASSES[I] < 1)
#endif
static int maxNvInTerm(void)
{
	long       i, nv, maxnv = 0;
	long       nterms = NTERMS;

	for (i = 0; i < nterms; i++)
	{
		
		nv = nvInTerm(modelterm(MODEL, i));
		maxnv = (nv > maxnv) ? nv : maxnv;
	}
	return (maxnv);
} /*maxNvInTerm()*/

static int anovaEffectsOnly(long level)
{
	long       nfactors = NFACTORS, nvars = NVARS;

	return ((nfactors == nvars) &&
			(nfactors == 1 || maxNvInTerm() <= level));
} /*anovaEffectsOnly*/

static int cellsEqual(double *cellCounts, long ncells)
{
	long         i;
	
	for (i = 1; i < ncells; i++)
	{
		if (cellCounts[i] != cellCounts[0])
		{
			return (0);
		}
	}
	return (1);
}

static int isBalanced(int op)
{
	long          ndata = NDATA;
	long          nvars = NVARS;
	long          i, j, k, ncells, length;
	int           balanced = 1;
	long          term[MAXVARS];
	double      **cts = (double **) 0;
	WHERE("isBalanced");
	TRASH(NTRASH,errorExit);
	
	switch (op)
	{
	  case ONEWAYBALANCE:
		/* check if it is possible main effects are balanced*/
		ncells = 0;
		for (i = 0; i < nvars; i++)
		{
			length = NCLASSES[i];
			if (ndata % length != 0)
			{
				/* This one-way marginal table not possibly balanced */
				return (0);
			}
			ncells = (length > ncells) ? length : ncells;
		} /*for (i = 0; i < nvars; i++)*/

		/*
		   Check that one-way marginals are balanced
	   */
		if(!getScratch(cts,GCTS,ncells,double))
		{
			goto errorExit;
		}

		/* check all one-way marginals for balance */

		for (i = 0; i < nvars; i++)
		{
			for(k = 0; k < nvars; k++)
			{
				term[k] = 0;
			}
			term[i] = 1;

			cellstat(Y, term, (double **) 0, (double **) 0,cts, CELLCOUNTS);
			balanced = cellsEqual(*cts, NCLASSES[i]);
			if (!balanced)
			{
				break;
			}
		} /*for (i = 0; i < nvars; i++)*/
		break;
		
	  case TWOWAYBALANCE:
		/* 
		   Check if all two-way marginal tables balanced
		*/
		ncells = 0;
		for (i = 1; i < nvars; i++)
		{
			for ( j = 0; j < i; j++)
			{
				length = NCLASSES[i] * NCLASSES[j];
				if (ndata % length != 0)
				{
				/* This two-way marginal table not possibly balanced */
					return (0);
				}
				ncells = (length > ncells) ? length : ncells;
			}
		} /*for (i = 1; i < nvars; i++)*/

		if(!getScratch(cts,GCTS,ncells,double))
		{
			goto errorExit;
		}

		/* check all two-way marginals for balance */
		for (i = 1; i < nvars; i++)
		{
			for ( j = 0; j < i; j++)
			{
				for(k = 0; k < nvars; k++)
				{
					term[k] = 0;
				}
				term[i] = term[j] = 1;

				cellstat(Y, term, (double **) 0, (double **) 0,cts,
						 CELLCOUNTS);
				balanced = cellsEqual(*cts, NCLASSES[i]*NCLASSES[j]);
				if (!balanced)
				{
					break;
				}
			} /*for ( j = 0; j < i; j++)*/
			if (!balanced)
			{
				break;
			}
		} /*for (i = 1; i < nvars; i++)*/
		break;
		
	  case COMPLETEBALANCE:
		/* check for complete balance with all variables */
		ncells = 1;
		for(i=0;i<nvars;i++)
		{
			term[i] = 1;
			ncells *= NCLASSES[i];
		}
		if (ndata < ncells || ndata % ncells != 0)
		{
			/* complete balance not possible */
			return (0);
		}
		
		if(!getScratch(cts,GCTS,ncells,double))
		{
			goto errorExit;
		}

		/* compute cell counts */
		cellstat(Y, term, (double **) 0, (double **) 0, cts, CELLCOUNTS);
		balanced = cellsEqual(*cts, ncells);
		break;
	} /*switch (op)*/
	
	discardScratch(cts,GCTS,double);
	return (balanced);

  errorExit:
	emptyTrash();
	if(MODELTYPE & ANOVA)
	{
		sprintf(OUTSTR, "Try anova(,unbalanced:T)");
	}
	else
	{
		sprintf(OUTSTR, "Try poisson()");
	}
	putOUTSTR();
	return (-1);
} /*isBalanced()*/

static long makeTermNames(long needed)
{
	long             i, k;
	long             nterms = NTERMS;
	long             nerrorterms = NERRORTERMS;
	long             nvars = NVARS;
	int              countOnly = (needed == 0);
	char            *termNames, *varName;
	modelPointer     kterm;
	WHERE("makeTermNames");
	
	if (!countOnly)
	{
		termNames = *TERMNAMES;
	}	

	for (i = 0; i < nterms; i++)
	{
		kterm = modelterm(MODEL,i);
		if (modeltermEqual(kterm,(modelPointer) NULLTERM))
		{
			varName = "CONSTANT";
			if (countOnly)
			{
				needed += strlen(varName) + 1;
			}
			else
			{
				strcpy(termNames, varName);
				termNames = skipStrings(termNames, 1);
			}			
		} /*if (modeltermEqual(kterm,(modelPointer) NULLTERM))*/
		else
		{
			/* check to see if it is an error term */
			for (k = 0; k < nerrorterms - 1; k++)
			{
				if (modeltermEqual(kterm, MODELINFO->errorterms + k))
				{
					varName = "ERROR";
					if (countOnly)
					{
						needed += strlen(varName) + ((k >= 9) ? 2 : 1) + 1;
					}
					else
					{
						strcpy(termNames, varName);
						termNames = skipStrings(termNames, 1) - 1;
						if(k >= 9)
						{
							*termNames++ = (k+1)/10 + '0';
						}
						*termNames++ = (k+1)%10 + '0';
						*termNames++  = '\0';
					}
					break;
				} /*if (modeltermEqual(kterm, MODELINFO->errorterms + k))*/
			} /*for (k = 0; k < nerrorterms - 1; k++)*/

			if (k == nerrorterms - 1)
			{/* regular (not error) term */
				for (k = 0; k < nvars; k++)
				{
					if (inTerm(k,kterm))
					{
						int      evaluated = isevaluated(VARNAMES[k+1]);
						
						varName = VARNAME(k+1);

						if (!evaluated && istempname(varName))
						{/* strip off TEMPPREFIX ('@') if temporary name */
							varName++;
						}
						if (countOnly)
						{
							needed += strlen(varName) + 1;
							if (evaluated)
							{
								needed += 2;
							}
						} /*if (countOnly)*/
						else
						{
							if (evaluated)
							{
								*termNames++ = '{';
							}
							strcpy(termNames, varName);
							termNames = skipStrings(termNames, 1) - 1;
							if (evaluated)
							{
								*termNames++ = '}';
							}
							*termNames++ = 
								(moreInTerm(k,kterm,nvars)) ? '.' : '\0';
						} /*if (countOnly){}else{}*/
					 } /*if (inTerm(k,kterm))*/
				 } /*for (k = 0; k < nvars; k++)*/
			} /*if (k == nerrorterms - 1)*/
		} /*if (modeltermEqual(kterm,(modelPointer) NULLTERM)){...}else{...}*/
	} /*for (i = 0; i < nterms; i++)*/

	varName = "ERROR";
	if (countOnly)
	{
		needed += strlen(varName) + ((nerrorterms > 9) ? 2 : 1) + 1;
	}
	else
	{
		strcpy(termNames, varName);
		termNames = skipStrings(termNames, 1) - 1;
		if(nerrorterms > 9)
		{
			*termNames++ = nerrorterms/10 + '0';
		}
		*termNames++ = nerrorterms%10 + '0';
		*termNames++ = '\0';
	} /*if (countOnly){}else{}*/
			
	return (needed);
} /*makeTermNames()*/


/*
  Procedure which gets model string and converts it to proper format
  It also checks various things for correctness.

  getmodel() returns  0 if ok 1 if error
  980303 fixed bug having to do with MISSING weights; led to crash
         in Windows version
  990309 NFACTORS and MODELINFO->nfactors set to 0 on regress() and screen()
  990319 Moved creation of side effect variables DEPVNAME and TERMNAMES to glm()
*/

long getmodel(Symbolhandle list, long nargs, long startKey, long silent,
			  Symbolhandle symhWts, Symbolhandle symhOffset,
			  Symbolhandle symhN)
{
	long            i, j, i2, inc;
	long            ny, ndata, nvars, nfactors, nterms, nerrorterms;
	long            needed, balanced;
	long            foundNonIntY = 0, foundNonIntN = 0;
	long            intercept;
	long            nmissing, nzerowt;
	long            dim[2];
	modelPointer    kterm;
	Symbolhandle    symh;
	double          tmp;
	double         *glmoffset, *casewts, *misswts, *ptmp, *factorvar;
	double          yi, logitni, *y, *logitn, maxlevel;
	long            caseweights = MODELTYPE & CASEWEIGHTS;
	char           *ipfwarning =
		"WARNING: ipf() cannot be used, poisson() substituted";
	modelInfoPointer info = (modelInfoPointer) 0;
	WHERE("getmodel");
	TRASH(NTRASH,errorExit);

	*OUTSTR = '\0';
	USEGLMOFFSET = 0;
	
	if (!initmodelparse(STRMODEL) || modelparse() != 0)
	{/* parse it */
		goto errorExit;
	}
	info = getModelInfo(); /* retrieve model information */
	if((ndata = checkVariables(info)) < 0)
	{
		goto errorExit;
	}
	if(!(GLMCONTROL & MULTIVAR) && !isVector(info->modelvars[0]))
	{
		sprintf(OUTSTR,
				"ERROR: non-vector response variable for %s()", FUNCNAME);
		goto errorExit;
	}
	
	NVARS = nvars = MODELINFO->nvars = info->nvars;
	NFACTORS = nfactors = MODELINFO->nfactors = info->nfactors;
	NTERMS = nterms = MODELINFO->nterms = info->nterms;
	
	NERRORTERMS = nerrorterms = MODELINFO->nerrorterms = info->nerrorterms;
	for(i=0;i<nerrorterms;i++)
	{
		modeltermAssign((modelPointer) info->errorterms[i],
						(modelPointer) MODELINFO->errorterms[i]);
	}
/* Note: As of 930601, INTMODEL has been eliminated */
	MODEL = info->model;
	info->model = (modelHandle) 0;
	
	if(MODELTYPE & (OLSREG|LEAPS))
	{
		NFACTORS = nfactors = MODELINFO->nfactors = 0;
		intercept = 0;
		for(j=0;j<nterms;j++)
		{
			kterm = modelterm(MODEL,j);
			if(nvInTerm(kterm) > 1)
			{
				sprintf(OUTSTR,
						"ERROR: crossed variates and/or factors are illegal for %s()",
						FUNCNAME);
				goto errorExit;
			} /*if(nvInTerm(modelterm(MODEL,j)) > 1)*/
			if(modeltermEqual(kterm,(modelPointer) NULLTERM))
			{
				intercept = 1;
			}
		} /*for(j=0;j<nterms;j++)*/
	} /*if(MODELTYPE & (OLSREG|LEAPS))*/

	if (MODELTYPE & LEAPS && !intercept)
	{
		sprintf(OUTSTR,
				"ERROR: a model without an intercept is illegal for %s()",
				FUNCNAME);
		goto errorExit;
	}

	/* set up MODELVARS */
	for (i = 0; i <= nvars; i++)
	{
		if(MODELVARS[i] != (Symbolhandle) 0)
		{
			Delete(MODELVARS[i]);
		}
		
		MODELVARS[i] = Makesymbol(REAL);
		if (MODELVARS[i] == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		
		if (!Copy(info->modelvars[i], MODELVARS[i]))
		{
			goto errorExit;
		}
		info->modelvars[i] = (Symbolhandle) 0;
		strcpy(VARNAMES[i], NAME(MODELVARS[i]));
		if(i < nvars)
		{
			NCLASSES[i] = info->nclasses[i];
		}			
		clearNotes(MODELVARS[i]);
	} /*for (i = 0; i <= nvars; i++)*/

	/* set up Y and X */
	/* response and factors are known to be matrix or vector */
	for (i = 0; i <= nvars; i++)
	{
		symh = MODELVARS[i];
		(void) isMatrix(symh,dim); /* get dimensions */
		if (HASLABELS(symh) && NDIMS(symh) != 2)
		{
			if (!fixupMatLabels(symh, USEBOTHLABELS))
			{
				goto errorExit;
			}
		} /*if (HASLABELS(symh) && NDIMS(symh) != 2)*/
		else
		{
			Setdims(symh, 2, dim);
		}
		
		if(i == 0)
		{
			Y = DATA(symh);
			ny = dim[1];
			NDATA = (double) ndata;
			NY = (double) ny;
			/* make sure dimensions are correct, always 2 */
		} /*if(i == 0)*/
		else
		{/* i > 0 */
			X[i - 1] = DATA(symh);

			if (isVariate(i-1))
			{
				GLMCONTROL |= UNBALANCED;
			} /*if (isVariate(i-1))*/
		}  /*if(i == 0){}else{}*/
		if(*OUTSTR)
		{
			goto errorExit;
		}
	} /*for (i = 0; i <= nvars; i++)*/

	/* check for crossed variates */

	for (i = 0; i < nvars-1; i++)
	{
		if (isVariate(i))
		{
			/* variable i is a variate */
			for (j = 0; j < nterms; j++)
			{					/* 
				find the terms it is in and check to see it is not crossed
				with another variate
			  */
				kterm = modelterm(MODEL,j);
				
				if(inTerm(i,kterm) && nvInTerm(kterm) > 1)
				{
					/* 
					  j-th term has variable i and there are at least 2
					  variables or factors in the term.
					  Check to see if j-th term has another variate
					*/
					for (i2 = i+1; i2 < nvars; i2++)
					{
						if (isVariate(i2) && inTerm(i2,kterm))
						{
							sprintf(OUTSTR,
									"ERROR: crossed variates in model are illegal for %s()",
									FUNCNAME);
							goto errorExit;
						} /*if (isVariate(i2) && inTerm(i2,kterm))*/
					} /*for (i2 = 0; i2 < nvars; i2++)*/
				} /*if(inTerm(i,kterm))*/
			} /*for (j = 0; j < nterms; j++)*/
		} /*if (isVariate(i))*/
	} /*for (i = 0; i < nvars; i++)*/

	/* check for case weights, i.e., user specified weights */

	if (caseweights)
	{
		if (symhWts == (Symbolhandle) 0)
		{
			sprintf(OUTSTR,
					"ERROR: no weights given for %s()",FUNCNAME);
		}
		else if (TYPE(symhWts) != REAL || !isVector(symhWts) ||
				 symbolSize(symhWts) != ndata)
		{
			sprintf(OUTSTR,
					"ERROR: weights for %s() not REAL vector with length = nrows(%s)",
					FUNCNAME, NAME(MODELVARS[0]));
		}
		else
		{
			CASEWTS = (double **) mygethandle(ndata * sizeof(double));
			if (CASEWTS == (double **) 0)
			{
				goto errorExit;
			}
			casewts = *CASEWTS;

			doubleCopy( DATAPTR(symhWts), casewts, ndata);
			for (i = 0; i < ndata; i++)
			{
				if (!isMissing(casewts[i]) && casewts[i] < 0)
				{
					sprintf(OUTSTR,"ERROR: negative case weights for %s()",
							FUNCNAME);
					break;
				}
			} /*for (i = 0; i < ndata; i++)*/
		}
		if(*OUTSTR)
		{
			goto errorExit;
		}
	} /* if(caseweights) */

	/* check for logistic N */
	if (GLMCONTROL & BINOMDIST)
	{
		if (symhN == (Symbolhandle) 0)
		{
			sprintf(OUTSTR,"ERROR: no N variable given for %s()", FUNCNAME);
		}
		else if(!isVector(symhN) || TYPE(symhN) != REAL || 
				symbolSize(symhN) != ndata && symbolSize(symhN) != 1)
		{
			sprintf(OUTSTR,
					"ERROR: %s() N not REAL scalar or vector of same length as response",
					FUNCNAME);
		}
		else
		{
			LOGITN = (double **) mygethandle(ndata * sizeof(double));
			if (LOGITN == (double **) 0)
			{
				goto errorExit;
			}
		}
		if(*OUTSTR)
		{
			goto errorExit;
		}
		inc = (symbolSize(symhN) == 1) ? 0 : 1;
		i2 = 0;
		y = *Y;
		logitn = *LOGITN;
		ptmp = DATAPTR(symhN);
		for (i = 0; i < ndata; i++)
		{
			yi = y[i];
			logitni = logitn[i] = ptmp[i2];
			if (!isMissing(yi) && !isMissing(logitni))
			{
				tmp = floor(logitni);
				if (tmp <= 0.0)
				{
					sprintf(OUTSTR,
							"ERROR:  N values must be nonnegative for %s()",
							FUNCNAME);
				}
				else if (yi < 0.0 || yi > logitni)
				{
					sprintf(OUTSTR,
							"ERROR: response value out of range for %s()",
							FUNCNAME);
				}
				if(*OUTSTR)
				{
					goto errorExit;
				}
				if (!foundNonIntY && yi != floor(yi))
				{
					foundNonIntY = 1;
				}
				if (!foundNonIntN && logitni != tmp)
				{
					foundNonIntN = 1;
				}
			} /*if (!isMissing(yi) && !isMissing(logitni))*/
			i2 += inc;
		}/* for (i = 0; i < ndata; i++) */
	} /*if (GLMCONTROL & BINOMDIST)*/
	else if (GLMCONTROL & POISSONDIST)
	{/* check for nonnegative Y in Poisson distributed glm */
		y = *Y;
		for (i = 0; i < ndata; i++)
		{
			yi = y[i];
			if (!isMissing(yi))
			{
				if (yi < 0.0)
				{
					sprintf(OUTSTR,
							"ERROR: response variable negative for %s()",
							FUNCNAME);
					goto errorExit;
				}
				if (!foundNonIntY && floor(yi) != yi)
				{
					foundNonIntY = 1;
				}
			} /*if (!isMissing(yi))*/			
		} /*for (i = 0; i < ndata; i++)*/
	} /*if (glmcontrol & POISSONDIST)*/

	if (!silent)
	{
		if (foundNonIntY)
		{
			sprintf(OUTSTR,
					"WARNING: non-integer value(s) of response variable for %s()",
					FUNCNAME);
			putErrorOUTSTR();
		}
		if (foundNonIntN)
		{
			sprintf(OUTSTR,
					"WARNING: non-integer number N of trials for %s()",
					FUNCNAME);
			putErrorOUTSTR();
		}
	} /*if (!silent)*/
	
	/* check for user defined glmoffset */

	if (symhOffset != (Symbolhandle) 0)
	{
		if (TYPE(symhOffset) != REAL || !isVector(symhOffset) ||
			symbolSize(symhOffset) != ndata)
		{
			sprintf(OUTSTR,
					"ERROR: offsets for %s() not REAL vector nrows(offset) = nrows(response)",
					FUNCNAME);
			goto errorExit;
		}

		USEGLMOFFSET = 1;
		GLMOFFSET = (double **) mygethandle(ndata * sizeof(double));
		if (GLMOFFSET == (double **) 0)
		{
			goto errorExit;
		}
		doubleCopy(DATAPTR(symhOffset), *GLMOFFSET, ndata);
	} /*if (symhOffset != (Symbolhandle) 0)*/

	/* check for missing & fill MISSWTS if any are missing*/
	nmissing = countMissing(MODELINFO, &MISSWTS);
	if(nmissing < 0)
	{
		goto errorExit;
	}

	if(caseweights || GLMCONTROL & BINOMDIST || USEGLMOFFSET)
	{
		casewts = (caseweights) ? *CASEWTS : (double *) 0;
		logitn = (GLMCONTROL & BINOMDIST) ? *LOGITN : (double *) 0;
		glmoffset = (USEGLMOFFSET) ? *GLMOFFSET : (double *) 0;
		misswts = (nmissing > 0) ? *MISSWTS : (double *) 0;
		for (i = 0; i < ndata; i++)
		{
			if (caseweights && isMissing(casewts[i]) ||
				(GLMCONTROL & BINOMDIST) && isMissing(logitn[i]) ||
				USEGLMOFFSET && isMissing(glmoffset[i]))
			{
				if(nmissing == 0)
				{
					MISSWTS = (double **) mygethandle(ndata * sizeof(double));
					if (MISSWTS == (double **) 0)
					{
						goto errorExit;
					}
					misswts = *MISSWTS;
					doubleFill(misswts, 1.0, ndata);
					casewts = (caseweights) ? *CASEWTS : (double *) 0;
					logitn = (GLMCONTROL & BINOMDIST) ? *LOGITN : (double *) 0;
					glmoffset = (USEGLMOFFSET) ? *GLMOFFSET : (double *) 0;
				} /*if(nmissing == 0)*/
				if (misswts[i] != 0.0)
				{
					misswts[i] = 0.0;
					nmissing++;
				}
			}
		} /*for (i = 0; i < ndata; i++)*/
	} /*if(caseweights || GLMCONTROL & BINOMDIST || USEGLMOFFSET)*/
	
	if(nmissing > 0)
	{
		MODELTYPE |= MISSWEIGHTS;
		GLMCONTROL |= UNBALANCED;
		misswts = *MISSWTS;
	} /*if(nmissing > 0)*/

	/* find the actual maximum factor levels included */
	if (nfactors > 0)
	{
		for (j = 0; j < nvars; j++)
		{
			if (!isVariate(j))
			{
				factorvar = *(X[j]);
				maxlevel = 0;
				for (i = 0;i < ndata; i++)
				{
					if (!isMissing(factorvar[i]) && factorvar[i] > maxlevel &&
						(nmissing == 0 || misswts[i] > 0.0))
					{
						maxlevel = factorvar[i];
					}
				} /*for (i = 0;i < ndata; i++)*/
				NCLASSES[j] = (long) maxlevel;
			} /*if (!isVariate(j))*/
		} /*for (j = 0; j < nvars; j++)*/
	} /*if (nfactors > 0)*/
	
	if (nmissing > 0 && !silent)
	{
		putOutErrorMsg("WARNING: cases with missing values deleted");
	}

	/* check for zero case weights, i.e., user specified weights */
	if (caseweights)
	{
		casewts = *CASEWTS;
		misswts = (MODELTYPE & MISSWEIGHTS) ? *MISSWTS : (double *) 0;
		nzerowt = 0;
		for (i = 0; i < ndata; i++)
		{
			if ((!(MODELTYPE & MISSWEIGHTS) || misswts[i] > 0.0) &&
				casewts[i] == 0)
			{
				nzerowt++;
			}
		}
		if (nzerowt != 0 && !silent)
		{
			putOutErrorMsg("WARNING: cases with zero weight completely removed");
		}
		nmissing += nzerowt;
	} /*if (caseweights)*/
	NOTMISSING = NDATA - nmissing;

	if(NOTMISSING == 0)
	{
		sprintf(OUTSTR,
				"ERROR: no non-missing cases with non-zero weight");
		goto errorExit;
	}
	
	/*
	   Check for possible Latin Square or other balanced main effect design
	    for which balanced computation is appropriate
	  In the future, we need code to recognize fractional factorials,
	  confounded factorials, etc
	 */

	balanced = (!(GLMCONTROL & UNBALANCED) && !(MODELTYPE & FASTANOVA) &&
				isBalanced(ONEWAYBALANCE));
	
	if (balanced && nvars > 1)
	{
		if (anovaEffectsOnly(1))
		{
			balanced = isBalanced(TWOWAYBALANCE);
		}
		else
		{
			balanced = isBalanced(COMPLETEBALANCE);
		}
		if (balanced < 0)
		{
			goto errorExit;
		}
	} /*if (balanced)*/
	
	if(!balanced)
	{
		GLMCONTROL |= UNBALANCED;
		if (MODELTYPE & IPF)
		{
			MODELTYPE &= ~IPF;
			MODELTYPE |= POISSONREG;
			if(!silent)
			{
				putOutErrorMsg(ipfwarning);
			}
		}
	} /*if(!balanced)*/

	/* check for incremental deviances specified by extra argument */
	if (MODELTYPE & (ITERGLMS | IPF) && !INCREMENTAL &&
		nargs > startKey && !isKeyword(COMPVALUE(list,nargs-1)))
	{
		INCREMENTAL = 1;
	}

	/*
	  990319 moved creation of side-effect variables DEPVNAME and
	  TERMNAMES to glm()
    */
	/* save dependent variable name */
	needed = strlen(VARNAME(0)) + 1;
	
	DEPVNAME = mygethandle(needed);
	if(DEPVNAME == (char **) 0)
	{
		goto errorExit;
	}
	/* re-dereference name after memory allocation */
	strcpy(*DEPVNAME, VARNAME(0));
	
	/* Create TERMNAMES */
	needed = makeTermNames(0); /* get space requirement */
	TERMNAMES = mygethandle(needed);
	if (TERMNAMES == (char **) 0)
	{
		goto errorExit;
	}
	
	/* now fill in TERMNAMES */

	(void) makeTermNames(needed);

	emptyTrash();
	
	return (0); /* normal (no error) return */

/* NOTE: cleanup of globals is done in glm after error return */
 errorExit:
	putErrorOUTSTR();
	emptyTrash();
	if(info != (modelInfoPointer) 0)
	{
		mydisphandle((char **) info->model);
		MODEL = info->model = (modelHandle) 0;
	}
	
	return (1);

} /*getmodel()*/
