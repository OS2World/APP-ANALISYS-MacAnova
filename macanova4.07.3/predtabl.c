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
#pragma segment Predtabl
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

static void         buildPredvec(double * estimate, double * seEst,
								 double * sePred, double * xvals,
								 double * xrow, long * factors, long nfactors)
{
	long            i, j, k, ik;
	double          tableTerm[MAXVARS+1];
	long            nregx = (long) NREGX, nvars = (long) NVARS;
	long            ny = (long) NY;
	long            icell, nvariates = 0;
	long            tableSize = 1;
	double         *estimate1 = (double *) 0;
	double         *seEst1 = (double *) 0;
	double         *sePred1 = (double *) 0, *temp;
	int             doEstimate = (estimate != (double *) 0);
	int             doSeEst = (seEst != (double *) 0);
	int             doSePred = (sePred != (double *) 0);
	WHERE("buildPredvec");
	
	/* Initialize tableTerm */

	for (j = 0; j < nvars; j++)
	{
		if (NCLASSES[j] > 0)
		{
			tableTerm[j] = 0.0;
			for (i = 0; i < nfactors; i++)
			{
				if (factors[i] == j)
				{
					tableTerm[j] = 1.0;
					tableSize *= NCLASSES[j];
					break;
				}
			} /*for (i = 0; i < nfactors; i++)*/
		} /*if (NCLASSES[j] > 0)*/
		else
		{
			tableTerm[j] = xvals[nvariates++];
		}
	} /*for (j = 0; j < nvars; j++)*/

	if (ny > 1)
	{ /* use extra space at end of xrow */
		temp = xrow + nregx;
		
		if (doEstimate)
		{
			estimate1 = temp;
			temp += ny;
		}
		if (doSeEst)
		{
			seEst1 = temp;
			temp += ny;
		}
		if (doSePred)
		{
			sePred1 = temp;
		}
	} /*if (ny > 1)*/
	
	for (icell = 0; icell < tableSize; icell++)
	{
		buildXrow(tableTerm, xrow, MODELINFO);
		if (ny == 1)
		{
			estimate1 = estimate;
			seEst1 = seEst;
			sePred1 = sePred;
		}
		compLincom(xrow, estimate1, seEst1, sePred1);
		if (ny > 0)
		{
			ik = 0;
			for (k = 0; k < ny; k++)
			{
				if (doEstimate)
				{
					estimate[ik] = estimate1[k];
				}
				if (doSeEst)
				{
					seEst[ik] = seEst1[k];
				}
				if (doSePred)
				{
					sePred[ik] = sePred1[k];
				}
				ik += tableSize;
			} /*for (k = 0; k < ny; k++)*/
		} /*if (ny > 0)*/
		if (doEstimate)
		{
			estimate++;
		}
		if (doSeEst)
		{
			seEst++;
		}
		if (doSePred)
		{
			sePred++;
		}
		
		/* step odometer */
		for (j = 0; j < nfactors; j++)
		{
			k = factors[j];
			tableTerm[k] += 1.0;
			if (tableTerm[k] <= NCLASSES[k])
			{
				break;
			}
			tableTerm[k] = 1.0;
		} /*for (j = 0; j < nfactors; j++)*/
	} /*for (icell = 0; icell < tableSize; icell++)*/

} /*buildPredvec()*/
 

/*
   951121 New usage:  predtable(x:values) or predtable(wtmeans:T) 
*/
enum predtableCode
{
	UNWEIGHTEDMEANS,
	WEIGHTEDMEANS,
	USERVALUES
};

enum predtableScratch
{
	GXROW,
	NTRASH
};
enum predtableOps
{
	IPREDTABLE = 1,
	IGLMTABLE
};
enum predtableKeyCodes
{
	KESTIMATE = 0,
	KSEEST,
	KSEPRED,
	KN,
	KTERM,
	KX,
	KWTDMEAN,
	KWTMEAN
};

static keywordList    TableKeys[] =
{
	InitKeyEntry("estimate", 0, 0, LOGICSCALAR),
	InitKeyEntry("seest", 0, 0, LOGICSCALAR),
	InitKeyEntry("sepred", 0, 0, LOGICSCALAR),
	InitKeyEntry("n", 0, 0, REALVALUE),
	InitKeyEntry("term", 0, 0, POSITIVEINT),
	InitKeyEntry("x", 0, 0, REALVECTOR),
	InitKeyEntry("wtdmean", 0, 0, LOGICSCALAR),
	InitKeyEntry("wtmean", 0, 0, LOGICSCALAR)
};

#define         DoEstimate   KeyLogValue(TableKeys,KESTIMATE)
#define         DoSeEst      KeyLogValue(TableKeys,KSEEST)
#define         DoSePred     KeyLogValue(TableKeys,KSEPRED)
#define         SymhN        KeySymhValue(TableKeys,KN)
#define         TermNo       KeyIntValue(TableKeys,KTERM)
#define         SymhX        KeySymhValue(TableKeys,KX)
#define         UseWtdMean   KeyLogValue(TableKeys,KWTDMEAN)
#define         UseWtMean    KeyLogValue(TableKeys,KWTMEAN)


#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment Predtabl
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

/*
   functions to compute tables of expectations and standard errors
	glmtable([wtdmeans:T or x:vals, estimate:F, seEst:F, sePred:T, n:N]) or
	glmtable(Term,[wtdmeans:T or x:vals, estimate:F, seEst:F, sePred:T, n:N])
	  where vals is REAL vector and TERM is CHARACTER scalar of form
	  "A.B. ...", where A, B are factors in current GLM model, N is a positive
	  REAL scalar or vector or array of positive reals.

   951219 Added glmtable()

   981209 Added check to see the previous GLM command was a weighted
          manova() or anova() when wtdmean:T is used.
   990226 Replaced putOUTSTR() by putErrorOUTSTR()
*/

Symbolhandle    predtable(Symbolhandle list)
{
	Symbolhandle    result = (Symbolhandle) 0, symhKey;
	Symbolhandle    symhTerm = (Symbolhandle) 0;
	Symbolhandle    symhEstimate = (Symbolhandle) 0;
	Symbolhandle    symhSeEst = (Symbolhandle) 0;
	Symbolhandle    symhSePred = (Symbolhandle) 0;
	double         *x, *y, *residuals;
	double         *estimate, *seEst, *sePred;
	double        **xrowH = (double **) 0;
	double         *binomn, one = 1;
	double          xvals[MAXVARS];
	double          dferror;
	double         *misswts, *allwts, sumwts, caseweight;
	double         *ss, *hii;
	long            facVars[MAXDIMS], term[MAXVARS];
	long            i, i2, j, j2, k, incN;
	long            startKey = 0, keyStatus;
	long            ndata = (long) NDATA, ny = (long) NY;
	long            nterms = (long) NTERMS, nvars = (long) NVARS;
	long            nfactors = (long) NFACTORS;
	long            nvariates = nvars - nfactors;
	long            nregx = (long) NREGX;
	long            nargs = NARGS(list);
	long            nvals, ncells, needed, ndims;
	long            compMeans; /* default is unweighted means */
	long            nkeys = NKeys(TableKeys);
	keywordListPtr  keys = TableKeys;
	int             op = (strcmp(FUNCNAME, "predtable") == 0) ?
		IPREDTABLE : IGLMTABLE;
	int             olsmodel = (PREVMODELTYPE & (ANOVA | MANOVA | OLSREG));
	char           *keyword = (char *) 0, *termName = (char *) 0;
	char           *wtskeyword = KeyName(keys, KWTDMEAN);
	char           *diffvals =
	  "ERROR: %s and %s set differently; don't know which to use";
	long            ncomps, icomp;
	WHERE("predtable");
	TRASH(NTRASH, errorExit);
	
	*OUTSTR = '\0';

	unsetKeyValues(keys, nkeys);

	if (PREVMODELTYPE == 0)
	{
		sprintf(OUTSTR, "ERROR: no active GLM model");
	}
	else if(nfactors == 0)
	{
		sprintf(OUTSTR,
				"ERROR: %s() not legal when there are no factors in model; try regpred()",
				FUNCNAME);
	}
	else if (PREVMODELTYPE & (IPF | FASTANOVA))
	{
		sprintf(OUTSTR, "ERROR: you cannot use %s() after %s()",
				FUNCNAME, glmName(PREVMODELTYPE));
	}
	else if (!(PREVMODELTYPE & DOCOEFS))
	{
		sprintf(OUTSTR,
				"ERROR: %s() does not work after GLM command with coefs:F",
				FUNCNAME);
	}
	if (*OUTSTR)
	{
		goto errorExit;
	}
	
	symhTerm = COMPVALUE(list, 0);
	if (nargs == 1 && isNull(symhTerm))
	{ /* predtable() or predtable(NULL)*/
		nargs = 0;
		startKey = 0;
	} /*if (nargs == 1 && isNull(symhTerm))*/
	else
	{
		startKey = (isKeyword(symhTerm)) ? 0 : 1;
		if (startKey == 1 && !isNull(symhTerm))
		{
			if (TYPE(symhTerm) == CHAR && isScalar(symhTerm))
			{
				termName = STRINGPTR(symhTerm);
			}
			else if ((termName = NAME(symhTerm), isscratch(termName)))
			{
				sprintf(OUTSTR,
						"ERROR: \"%s\" is not valid term name", termName);
				goto errorExit;
			}
		} /*if (startKey == 1 && !isNull(symhTerm))*/
	} /*if (nargs == 1 && isNull(symhTerm)){}else{}*/
	
	/* Check keywords */
	for (i = startKey; i < nargs; i++)
	{
		symhKey = COMPVALUE(list, i);
		if (!argOK(symhKey, 0, (nargs == 1) ? 0 : i+1))
		{
			goto errorExit;
		}
		if (!(keyword = isKeyword(symhKey)))
		{
			sprintf(OUTSTR,
					"ERROR: all %s() arguments after the first must be keyword phrases",
					FUNCNAME);
					goto errorExit;
		}
	} /*for (i = startKey; i < nargs; i++)*/
	
	keyStatus = getAllKeyValues(list, startKey, 0, keys, nkeys);
	if (keyStatus < 0)
	{
		goto errorExit;
	}
	nargs -= keyStatus;
	
	if (nargs > startKey)
	{
		badKeyword(FUNCNAME, isKeyword(COMPVALUE(list, startKey)));
		goto errorExit;
	}
	
	if (!logicalIsSet(DoEstimate))
	{
		setLogicalTrue(DoEstimate);
	}

	if (!logicalIsSet(DoSeEst))
	{
		if (op == IPREDTABLE)
		{
			setLogicalFalse(DoSeEst);
		}
		else
		{
			setLogicalTrue(DoSeEst);
		}
	} /*if (!logicalIsSet(DoSeEst))*/

	if (!logicalIsSet(DoSePred))
	{
		setLogicalFalse(DoSePred);
	}

	if (!logicalIsSet(UseWtdMean) && !logicalIsSet(UseWtMean))
	{
		setLogicalFalse(UseWtdMean);
	}
	else if (!logicalIsSet(UseWtdMean))
	{
		UseWtdMean = UseWtMean;
		wtskeyword = KeyName(keys, KWTMEAN);
	}
	else if (logicalIsSet(UseWtMean) && UseWtdMean != UseWtMean)
	{
		sprintf(OUTSTR, diffvals,
				KeyName(keys, KWTDMEAN), KeyName(keys, KWTMEAN));
		goto errorExit;
	}

	if (symhIsSet(SymhN))
	{
		keyword = KeyName(keys,KN);
		if (!(PREVGLMCONTROL & BINOMDIST))
		{
			sprintf(OUTSTR,
					"ERROR: %s() keyword '%s' not allowed except when response is binomial",
					FUNCNAME, keyword);
		}
		else if (anyMissing(SymhN))
		{
			sprintf(OUTSTR,
					"ERROR: MISSING values not allowed in value for %s",
					keyword);
		}
		else if (doubleMin(DATAPTR(SymhN), symbolSize(SymhN)) <= 0)
		{
			sprintf(OUTSTR,
					"ERROR: all elements of %s must be positive", keyword);
		}
		if (*OUTSTR)
		{
			goto errorExit;
		}
	} /*if (symhIsSet(SymhN))*/

	if (symhIsSet(SymhX))
	{
		keyword = KeyName(keys,KX);
		if (nvariates == 0)
		{
			sprintf(OUTSTR,
					"ERROR: %s(...,%s:vals) illegal when there are no variates in model",
					FUNCNAME, keyword);
		}
		else if (symbolSize(SymhX) != nvariates)
		{
			sprintf(OUTSTR,
					"ERROR: length of value of \"%s\" does not match the number of variates",
					keyword);
		}
		if (*OUTSTR)
		{
			goto errorExit;
		}
	} /*if (symhIsSet(SymhX))*/

	if (longIsSet(TermNo))
	{
		if (termName != (char *) 0)
		{
			sprintf(OUTSTR,
					"ERROR: you can't use \"%s:termNo\"  and also supply a term name",
					keyword);
			goto errorExit;
		}
			
		if (TermNo > nterms)
		{
			sprintf(OUTSTR,
					"ERROR: %s has fewer than %ld terms",
					*STRMODEL, TermNo);
			goto errorExit;
		}
		termName = skipStrings(*TERMNAMES, TermNo - 1);
	} /*if (longIsSet(TermNo))*/

	if (UseWtdMean)
	{
		if (!olsmodel)
		{
			sprintf(OUTSTR,
					"ERROR: %s(...,%s:T) illegal after %s()",
					FUNCNAME, wtskeyword,glmName(PREVMODELTYPE));
			goto errorExit;
		}
		if (!(PREVMODELTYPE & CASEWEIGHTS))
		{
			sprintf(OUTSTR,
					"ERROR: %s(...,%s:T) illegal after unweighted %s()",
					FUNCNAME, wtskeyword, glmName(PREVMODELTYPE));
			goto errorExit;
		}
		if (symhIsSet(SymhX))
		{
			sprintf(OUTSTR,
					"ERROR: you can't use %s:T with %s:vals on %s()",
					wtskeyword, KeyName(keys,KX), FUNCNAME);
			goto errorExit;
		}
	} /*if (UseWtdMean)*/
	
	if (!olsmodel)
	{ /* not model fit by OLS */
		if (DoSePred)
		{
			sprintf(OUTSTR,
					"ERROR: %s(...,%s:T) illegal after %s()",
					FUNCNAME, KeyName(keys,KSEPRED), glmName(PREVMODELTYPE));
		}
		else if (!DoEstimate &&
				 PREVGLMCONTROL & (NONIDENTLINK | NONNORMALDIST))
		{
			sprintf(OUTSTR,
					"ERROR: %s(...,%s:F) illegal after %s()",
					FUNCNAME, KeyName(keys,KESTIMATE),glmName(PREVMODELTYPE));
		}
		
		if (*OUTSTR)
		{
			goto errorExit;
		}
	} /*if (!olsmodel)*/
	
	ncomps = DoEstimate + DoSeEst + DoSePred;
	if (ncomps == 0)
	{
		sprintf(OUTSTR,
				"ERROR: nothing requested for %s() to compute", FUNCNAME);
		goto errorExit;
	}

	if (symhIsSet(SymhX))
	{
		compMeans = USERVALUES;
		doubleCopy(DATAPTR(SymhX), xvals, nvariates);
	} /*if (symhIsSet(SymhX))*/
	else
	{
		compMeans = (UseWtdMean) ? WEIGHTEDMEANS : UNWEIGHTEDMEANS;
	}
	
	if (termName != (char *) 0)
	{
		nfactors = stringToTerm(termName, term);

		if (nfactors == 0)
		{
			goto errorExit;
		}
		
		for (j = 0; j < nvars; j++)
		{
			if (term[j] > 0)
			{
				if (NCLASSES[j] < 0)
				{
					sprintf(OUTSTR,
							"ERROR: variable %s is not a factor",
							VARNAME(j+1));
					goto errorExit;
				} /*if (NCLASSES[j] < 0)*/
				facVars[term[j] - 1] = j;
			}
		} /*for (j = 0; j < nvars; j++)*/

		if (nfactors < NFACTORS)
		{
			if (DoSePred)
			{
				sprintf(OUTSTR,
						"ERROR: %s:T illegal for %s() when computing marginal tables",
						KeyName(keys,KSEPRED), FUNCNAME);
			}
			else if (!(PREVGLMCONTROL & UNBALANCED))
			{
				sprintf(OUTSTR,
						"ERROR: %s() can't compute marginal tables after balanced ANOVA",
						FUNCNAME);
				putErrorOUTSTR();
				sprintf(OUTSTR,"       Use 'anova(model,unbalanced:T)'");
			}
			if (*OUTSTR)
			{
				goto errorExit;
			}
		} /*if (nfactors < NFACTORS)*/
	} /*if (termName != (char *) 0)*/
	else
	{
		/* find all factors in model */
		nfactors = 0;
		for (i = 0; i < nvars; i++)
		{
			if (NCLASSES[i] > 0)
			{
				facVars[nfactors++] = i;
			}
		} /*for (i = 0; i < nvars; i++)*/
	} /*if (termName != (char *) 0){}else{}*/
	
	/* compute size of table */
	ncells = 1;
	for (i = 0; i < nfactors; i++)
	{
		ncells *= NCLASSES[facVars[i]];
	} /*for (i = 0; i < nvars; i++)*/

	if (symhIsSet(SymhN))
	{ /* finish checking value for n */
		keyword = KeyName(keys,KN);
		if (!isScalar(SymhN) && symbolSize(SymhN) != ncells)
		{
			sprintf(OUTSTR,
					"ERROR: length of value for '%s' does not match number of cells",
					keyword);
		}
		else if (!isVector(SymhN))
		{
			if (NDIMS(SymhN) != nfactors)
			{
				sprintf(OUTSTR,
						"ERROR: number of dimensions of non-vector '%s' != number of factors",
						keyword);
			} /*if (NDIMS(SymhN) != nfactors)*/
			else
			{
				k = 1;
				for (j = 0; j < nvars; j++)
				{
					if (NCLASSES[j] > 0 &&
						NCLASSES[j] != DIMVAL(SymhN,k++))
					{
						sprintf(OUTSTR,
								"ERROR: dimensions of non-vector '%s' do not match dimensions of table",
								keyword);
						break;
					}
				} /*for (j = 0; j < nvars; n++)*/
			} /*if (NDIMS(SymhN) != nfactors){}else{}*/
		} 
		if (*OUTSTR)
		{
			goto errorExit;
		}
	} /*if (SymhN != (Symbolhandle) 0)*/

	if(USEGLMOFFSET)
	{
		putOutErrorMsg("WARNING: Table contains fitted deviations from offset, not fitted values");
	} /*if(USEGLMOFFSET)*/

	nvals = ncells * ny;
	result = ( ncomps == 1) ? RInstall(SCRATCH, nvals) :
		RSInstall(SCRATCH, ncomps, (char **) 0, nvals);
	
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	
	icomp = 0;
	
	ndims = nfactors + ((ny > 1) ? 1 : 0);
	if (DoEstimate)
	{
		symhEstimate = (ncomps == 1) ? result : COMPVALUE(result, icomp++);
		if (ncomps > 1)
		{
			setNAME(symhEstimate, "estimate");
		}
		setNDIMS(symhEstimate, ndims);
	}		
	if (DoSeEst)
	{
		symhSeEst = (ncomps == 1) ? result : COMPVALUE(result, icomp++);
		if (ncomps > 1)
		{
			setNAME(symhSeEst, "SEest");
		}
		setNDIMS(symhSeEst, ndims);
	}		
	if (DoSePred)
	{
		symhSePred = (ncomps == 1) ? result : COMPVALUE(result, icomp++);
		if (ncomps > 1)
		{
			setNAME(symhSePred, "SEpred");
		}
		setNDIMS(symhSePred, ndims);
	}		

	
	for (i = 0; i < nfactors; i++)
	{
		if (DoEstimate)
		{
			setDIM(symhEstimate, i+1,NCLASSES[facVars[i]]);
		}
		if (DoSeEst)
		{
			setDIM(symhSeEst, i+1,NCLASSES[facVars[i]]);
		}
		if (DoSePred)
		{
			setDIM(symhSePred, i+1,NCLASSES[facVars[i]]);
		}
	} /*for (i = 0; i < nfactors; i++)*/

	if (ny > 1)
	{
		if (DoEstimate)
		{
			setDIM(symhEstimate,ndims,ny);
		}
		if (DoSeEst)
		{
			setDIM(symhSeEst,ndims,ny);
		}
		if (DoSePred)
		{
			setDIM(symhSePred,ndims,ny);
		}
	} /*if (ny > 1)*/

	if (!(PREVGLMCONTROL & UNBALANCED))
	{/* balanced ; must be anova()*/
		ss = *SS;
		hii = *HII;
		if (DoEstimate)
		{
			y = *Y;
			estimate = DATAPTR(symhEstimate);
			residuals = *RESIDUALS;
			for (j = 0; j < ndata; j++)
			{
				j2 = 0;
				for (k = nvars; k > 0; k--)
				{
					j2 *= NCLASSES[k - 1];
					j2 += (*X[k - 1])[j] - 1;
				}
				estimate[j2] = y[j] - residuals[j];
			} /*for (j = 0; j < ndata; j++)*/
		} /*if (DoEstimate)*/
		
		dferror = (*DF)[nterms];
		if (DoSeEst)
		{
			seEst = DATAPTR(symhSeEst);
			if (dferror > 0)
			{
				doubleFill(seEst, sqrt(ss[nterms]*hii[0]/dferror), ncells);
			}
			else
			{
				for (j = 0; j < ncells; j++)
				{
					setMissing(seEst[j]);
				}
			}
		} /*if (DoSeEst)*/
		if (DoSePred)
		{
			sePred = DATAPTR(symhSePred);
			if (dferror > 0)
			{
				doubleFill(sePred, sqrt(ss[nterms]*(1.0 + hii[0])/dferror),
						   ncells);
			}
			else
			{
				for (j = 0; j < ncells; j++)
				{
					setMissing(sePred[j]);
				}
			}
		} /*if (DoSePred)*/
	} /*if (!(PREVGLMCONTROL & UNBALANCED))*/
	else
	{	/* unbalanced case */
		needed = nregx;
		if (ny > 1)
		{
			needed += ncomps*ny;
		}
		
		if (!getScratch(xrowH, GXROW, needed, double))
		{
			goto errorExit;
		}
		
		/* compute variate means */
		if (nvariates > 0 && compMeans != USERVALUES)
		{			
			misswts = (PREVMODELTYPE & MISSWEIGHTS) ? *MISSWTS : (double *) 0;
			allwts = (compMeans == WEIGHTEDMEANS) ? *ALLWTS : (double *) 0;
			i2 = 0;
			for (i = 0; i < nvars; i++)
			{
				if (NCLASSES[i] <= 0)
				{/* found a variate; put its mean in xvals[i] */
					sumwts = xvals[i2] = 0.;
					caseweight = 1.0;
					x = *X[i];
					for (j = 0; j < ndata; j++)
					{ /*compute mean of variates in complete cases*/
						if (misswts == (double *) 0 || misswts[j] != 0.0)
						{
							if (allwts != (double *) 0)
							{
								caseweight = allwts[j]*allwts[j];
							}
							sumwts += caseweight;
							xvals[i2] += caseweight * x[j];
						} /*if (misswts == (double *) 0 || misswts[j] != 0.0)*/
					} /*for (j = 0; j < ndata; j++)*/
					xvals[i2++] /=  sumwts;
				} /*if (NCLASSES[i] <= 0)*/
			} /*for (i = 0; i < nvars; i++)*/
		} /*if (nvariates > 0 && compMeans != USERVALUES)*/
		
		estimate = (DoEstimate) ? DATAPTR(symhEstimate) : (double *) 0;
		seEst = (DoSeEst > 0) ? DATAPTR(symhSeEst) : (double *) 0;
		sePred = (DoSePred > 0) ? DATAPTR(symhSePred) : (double *) 0;

		buildPredvec(estimate, seEst, sePred,
					 xvals, *xrowH, facVars, nfactors);
		
		/*
		  If not linear link, transform back to original scale
		  If standard errors are wanted use delta-method
		*/
		if (PREVGLMCONTROL & (NONIDENTLINK | NONNORMALDIST))
		{
			if (PREVGLMCONTROL & BINOMDIST)
			{
				incN = (SymhN != (Symbolhandle) 0 && !isScalar(SymhN)) ? 1 : 0;
				binomn = (SymhN != (Symbolhandle) 0) ? DATAPTR(SymhN) : &one;
			}
			else
			{
				binomn = (double *) 0;
				incN = 0;
			}
		
			deLink(estimate, seEst, binomn, incN, ncells);
		} /*if (PREVGLMCONTROL & (NONIDENTLINK | NONNORMALDIST))*/
		
	} /*if (!(PREVGLMCONTROL & UNBALANCED)){}else{}*/
	if ((PREVGLMCONTROL & NONNORMALDIST) != 0 && DoSeEst)
	{
		sprintf(OUTSTR,
				"NOTE: standard errors assume scale parameter is ");
		formatDouble(OUTSTR + strlen(OUTSTR), PREVGLMSCALE, TRIMLEFT);
		putOUTSTR();
	}
	emptyTrash();
	
	return (result);

  errorExit:
	putErrorOUTSTR();
	emptyTrash();
	return (0);
	
} /*predtable()*/

