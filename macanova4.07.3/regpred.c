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
#pragma segment Regpred
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
/*
  981202  cleaned up argument checking and removed bug causeing
          crash when called with no arguments

  990226  Replaced putOUTSTR() by putErrorOUTSTR()
*/
enum regpredScratch
{
	GXROW = 0,
	NTRASH
};

enum regpredOp
{
	IREGPRED = 1,
	IGLMPRED
};

enum regpredKeyCodes
{
	KESTIMATE = 0,
	KSEEST,
	KSEPRED,
	KN
};

static keywordList    PredKeys[] =
{
	InitKeyEntry("estimate", 0, 0, LOGICSCALAR),
	InitKeyEntry("seest", 0, 0, LOGICSCALAR),
	InitKeyEntry("sepred", 0, 0, LOGICSCALAR),
	InitKeyEntry("n", 0, 0, POSITIVEVECTOR)
};

#define         DoEstimate   KeyLogValue(PredKeys,KESTIMATE)
#define         DoSeEst      KeyLogValue(PredKeys,KSEEST)
#define         DoSePred     KeyLogValue(PredKeys,KSEPRED)
#define         SymhN        KeySymhValue(PredKeys,KN) 

Symbolhandle    regpred(Symbolhandle list)
{
	Symbolhandle    result, symhVariates = (Symbolhandle) 0;
	Symbolhandle    symhFactors = (Symbolhandle) 0;
	Symbolhandle    symhKey = (Symbolhandle) 0;
	Symbolhandle    symhEstimate = (Symbolhandle) 0;
	Symbolhandle    symhSeEst = (Symbolhandle) 0;
	Symbolhandle    symhSePred = (Symbolhandle) 0;
	double        **xrowH, *xrow, *binomn = (double *) 0;
	double         *x = (double *) 0, *factorVals = (double *) 0;
	double         *xk, *factork, *temp;
	double         *estimate = (double *) 0, *estimate1 = (double *) 0;
	double         *seEst = (double *) 0, *seEst1 = (double *) 0;
	double         *sePred = (double *) 0, *sePred1 = (double *) 0;
	double          xvals[MAXVARS];
	long            nkeys = NKeys(PredKeys);
	keywordListPtr  keys = PredKeys;
	long            keyStatus;
	long            vardims[2], factordims[2];
	long            nclasses[MAXDIMS];
	long            i, j, k, l, kl;
	long            incVars, incFactors, incN;
	long            nVariateVals = 0, nFactorVals = 0, npoints;
	long            nvars = (long) NVARS;
	long            nregx = (long) NREGX, nfactors = (long) NFACTORS;
	long            nargs = NARGS(list), ny = (long) NY;
	long            nvariates = nvars - nfactors;
	long            extra;
	long            op = (strcmp(FUNCNAME, "regpred") == 0) ?
		IREGPRED : IGLMPRED;
	long            startKey = -1, ncomps = 0, icomp = 0;
	char           *keyword;
	WHERE("regpred");
	TRASH(NTRASH, errorExit);

	unsetKeyValues(keys, nkeys);

	if (PREVMODELTYPE == NOMODEL)
	{
		sprintf(OUTSTR, "ERROR: no active GLM model");
	}
	else if (op == IREGPRED && nfactors > 0)
	{
		sprintf(OUTSTR,
				"ERROR: %s() illegal with factors in the model, try glmpred()",
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
	else if (!(PREVGLMCONTROL & UNBALANCED))
	{
		sprintf(OUTSTR,
				"ERROR: %s() does not work after balanced ANOVA", FUNCNAME);
		putErrorOUTSTR();
		sprintf(OUTSTR,"       use anova(model,unbalanced:T)");
	}
	else if (nvars == 0)
	{
		sprintf(OUTSTR,
				"ERROR: %s has no variates or factors", *STRMODEL);
	}
	if (*OUTSTR)
	{
		goto errorExit;
	}

	if (op == IREGPRED)
	{
		startKey = 1;
	}
	else
	{
		startKey = (nargs == 1 || isKeyword(COMPVALUE(list,1))) ? 1 : 2;
	}
	
	for (i = startKey; i < nargs; i++)
	{
		symhKey = COMPVALUE(list, i);
		if (!argOK(symhKey, 0, i + 1))
		{
			goto errorExit;
		}
		if (!(keyword = isKeyword(symhKey)))
		{
			sprintf(OUTSTR,
					"ERROR: all arguments to %s() after %ld%s must be keyword phrases",
					FUNCNAME, startKey, n_th(startKey));
					goto errorExit;
		}
		keyStatus = getOneKeyValue(symhKey, 0, keys, nkeys);
		if (keyStatus <= 0)
		{
			if (keyStatus < 0)
			{
				badKeyword(FUNCNAME, keyword);
			}
			goto errorExit;
		}
	} /*for (i = startKey; i < nargs; i++)*/
	
	if (nargs == 1 && COMPVALUE(list, 0) == (Symbolhandle) 0)
	{
		sprintf(OUTSTR,
				"ERROR: no values provided for %s()", FUNCNAME);
	}
	else if (op == IGLMPRED && nfactors > 0 && startKey < 2)
	{
		sprintf(OUTSTR,
				"ERROR: no factor levels provided for %s()", FUNCNAME);
	}
	if (*OUTSTR)
	{
		goto errorExit;
	}
	
	if (!logicalIsSet(DoEstimate))
	{
		setLogicalTrue(DoEstimate);
	}

	if (!logicalIsSet(DoSeEst))
	{
		setLogicalTrue(DoSeEst);
	}

	if (!logicalIsSet(DoSePred))
	{
		if (op == IREGPRED && (ANOVA | MANOVA | OLSREG) != 0)
		{
			setLogicalTrue(DoSePred);
		}
		else
		{
			setLogicalFalse(DoSePred);
		}
	}
		
	if (symhIsSet(SymhN))
	{
		keyword = KeyName(keys, KN);
		if (!(PREVGLMCONTROL & BINOMDIST))
		{
			sprintf(OUTSTR,
					"ERROR: %s() keyword '%s' not allowed except when response is binomial",
					FUNCNAME, keyword);
			goto errorExit;
		}
	} /*if (symhIsSet(SymhN))*/
	
	if ((PREVMODELTYPE & (ANOVA | MANOVA | OLSREG)) == 0)
	{
		if (DoSePred)
		{
			sprintf(OUTSTR,
					"ERROR: %s(...,%s:T) illegal after %s()",
					FUNCNAME, KeyName(keys,KSEPRED), glmName(PREVMODELTYPE));

		}
#if (0) /*I currently see no reason for disallowing estimate:F*/
		else if (!DoEstimate)
		{
			sprintf(OUTSTR,
					"ERROR: %s(...,%s:F) illegal after %s()",
					FUNCNAME, KeyName(keys,KESTIMATE), glmName(PREVMODELTYPE));
		}
#endif /*0*/
		if (*OUTSTR)
		{
			goto errorExit;
		}
	} /*if ((PREVMODELTYPE & (ANOVA | MANOVA | OLSREG)) == 0)*/
	
	
	ncomps = DoEstimate + DoSeEst + DoSePred;
	if (ncomps == 0)
	{
		sprintf(OUTSTR,
				"ERROR: nothing requested for %s() to compute", FUNCNAME);
		goto errorExit;
	}

	symhVariates = COMPVALUE(list, 0);
	if (nvariates > 0)
	{
		if(!argOK(symhVariates, 0, (nargs > 1) ? 1 : 0))
		{
			goto errorExit;
		}
		if(!isMatrix(symhVariates,vardims) || TYPE(symhVariates) != REAL)
		{
			sprintf(OUTSTR,
					"ERROR: variate values for %s() must be REAL vector or matrix",
					FUNCNAME);
		}
		else if (anyMissing(symhVariates))
		{
			sprintf(OUTSTR,
					"ERROR: variate values for %s() have MISSING values",
					FUNCNAME);
		}
		else
		{
			if(vardims[1] > 1 || nvariates == 1)
			{
				k = vardims[1];
				nVariateVals = vardims[0];
			}
			else
			{/* x is vector, NVARS > 1 */
				k = vardims[0];
				nVariateVals = 1;
			}

			if (k != nvariates)
			{
				strcpy(OUTSTR,
					   "ERROR: number of variate values does not match model");
			}
		}		
	} /*if (nvariates > 0)*/
	else if (!isNull(symhVariates))
	{
		sprintf(OUTSTR,
				"ERROR: No variates in %s and 1st argument to %s() not NULL",
				*STRMODEL, FUNCNAME);
	}
	if (*OUTSTR)
	{
		goto errorExit;
	}

	if (op == IGLMPRED && startKey > 1)
	{
		/* Check values for factors */
		symhFactors = COMPVALUE(list, 1);
		if (nfactors > 0)
		{
			if(!argOK(symhFactors, 0, 2))
			{
				goto errorExit;
			}
			if(!isMatrix(symhFactors,factordims) ||
			   TYPE(symhFactors) != REAL)
			{
				sprintf(OUTSTR,
						"ERROR: factor levels for %s() must be REAL vector or matrix",
						FUNCNAME);
			}
			else if (anyMissing(symhFactors))
			{
				sprintf(OUTSTR,
						"ERROR: factor levels for %s() have MISSING values",
						FUNCNAME);
			}
			else
			{
				if(factordims[1] > 1 || nfactors == 1)
				{
					k = factordims[1];
					nFactorVals = factordims[0];
				}
				else
				{				/* vector, NVARS > 1 */
					k = factordims[0];
					nFactorVals = 1;
				}

				if (k != nfactors)
				{
					strcpy(OUTSTR,
						   "ERROR: number of factor values does not match model");
				}
				else
				{						
					j = 0;
					for (i = 0; i < nvars; i++)
					{
						if (NCLASSES[i] > 0)
						{
							nclasses[j++] = NCLASSES[i];
						}
					} /*for (i = 0; i < nvars; i++)*/
						
					factorVals = DATAPTR(symhFactors);
					for (j = 0; j < nfactors; j++)
					{
						for (i = 0; i < nFactorVals; i++)
						{
							if (factorVals[i] != floor(factorVals[i]) ||
								factorVals[i] <= 0)
							{
								sprintf(OUTSTR,
										"ERROR: factor levels for %s() must be positive integers",
										FUNCNAME);
							}
							else if (factorVals[i] > nclasses[j])
							{
								sprintf(OUTSTR,
										"ERROR: level for factor %ld > %ld",
										j+1, nclasses[j]);
							}
							if (*OUTSTR)
							{
								goto errorExit;
							}
						}
						factorVals += nFactorVals; 
					} /*for (j = 0; j < nfactors; j++)*/
				}					
			}				
		} /*if (nfactors > 0)*/
		else if (!isNull(symhFactors))
		{
			sprintf(OUTSTR,
					"ERROR: No factors in %s and 2nd argument to %s() not NULL",
					*STRMODEL, FUNCNAME);
		}
	} /*if (op == IGLMPRED)*/

	if (*OUTSTR)
	{
		goto errorExit;
	}
	
	if (nVariateVals > 1 && nFactorVals > 1 && nVariateVals != nFactorVals)
	{
		sprintf(OUTSTR,
				"ERROR: numbers of variate value sets and factor level sets do not match");
		goto errorExit;
	}
	
	npoints = (nVariateVals > nFactorVals) ? nVariateVals : nFactorVals;

	if (symhIsSet(SymhN))
	{
		keyword = KeyName(keys,KN);
		if (!isScalar(SymhN) && symbolSize(SymhN) != npoints)
		{
			sprintf(OUTSTR,
					"ERROR: wrong length for value for '%s'", keyword);
			goto errorExit;
		}
	} /*if (symhIsSet(SymhN))*/
	
	extra = (npoints > 1 && ny > 1) ? ncomps * ny : 0;
	
	if (!getScratch(xrowH, GXROW, nregx + extra, double))
	{
		goto errorExit;
	}
	
	result = ( ncomps == 1) ? RInstall(SCRATCH, npoints*ny) :
		RSInstall(SCRATCH, ncomps, (char **) 0, npoints*ny);
	
	if(result == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	
	if (DoEstimate)
	{
		symhEstimate = (ncomps == 1) ? result : COMPVALUE(result, icomp++);
		if (ny > 1)
		{
			setNDIMS(symhEstimate, 2);
			setDIM(symhEstimate,1,npoints);
			setDIM(symhEstimate,2,ny);
		}
		if (ncomps > 1)
		{
			setNAME(symhEstimate, "estimate");
		}
		estimate = DATAPTR(symhEstimate);
	}		
	if (DoSeEst)
	{
		symhSeEst = (ncomps == 1) ? result : COMPVALUE(result, icomp++);
		if (ny > 1)
		{
			setNDIMS(symhSeEst, 2);
			setDIM(symhSeEst,1,npoints);
			setDIM(symhSeEst,2,ny);
		}
		if (ncomps > 1)
		{
			setNAME(symhSeEst, "SEest");
		}
		seEst = DATAPTR(symhSeEst);
	}		
	if (DoSePred)
	{
		symhSePred = (ncomps == 1) ? result : COMPVALUE(result, icomp++);
		if (ny > 1)
		{
			setNDIMS(symhSePred, 2);
			setDIM(symhSePred,1,npoints);
			setDIM(symhSePred,2,ny);
		}
		if (ncomps > 1)
		{
			setNAME(symhSePred, "SEpred");
		}
		sePred = DATAPTR(symhSePred);
	}		

	if (nvariates > 0)
	{
		x = DATAPTR(symhVariates);
	}
	if (nfactors > 0)
	{
		factorVals = DATAPTR(symhFactors);
	}

	if (symhIsSet(SymhN))
	{
		binomn = DATAPTR(SymhN);
		incN = isScalar(SymhN) ? 0 : 1;
	}
	
	xrow = *xrowH;
	if (extra > 0)
	{
		temp = xrow + nregx;
		if (DoEstimate)
		{
			estimate1 = temp;
			temp += ny;
		}
		if (DoSeEst)
		{
			seEst1 = temp;
			temp += ny;
		}
		if (DoSePred)
		{
			sePred1 = temp;
		}
	} /*if (extra > 0)*/
	
	incVars = (nVariateVals > 1) ? 1 : 0;
	incFactors = (nFactorVals > 1) ? 1 : 0;

	for (k = 0;k < npoints;k++)
	{
		xk = x;  /* k-th row of variates */
		factork = factorVals;  /* k-th row of factors */
		for (i = 0; i < nvars; i++)
		{
			if (NCLASSES[i] < 0)
			{
				xvals[i] = *xk;
				xk += nVariateVals;
			}
			else
			{
				xvals[i] = *factork;
				factork += nFactorVals;
			}
		} /*for (i = 0; i < nvars; i++)*/

		(void) buildXrow(xvals, xrow, MODELINFO);

		if (extra == 0)
		{
			estimate1 = estimate;
			seEst1 = seEst;
			sePred1 = sePred;
		}
		compLincom(xrow, estimate1, seEst1, sePred1);

		if (extra > 0)
		{
			kl = 0;
			for (l = 0; l < ny; l++)
			{
				if (DoEstimate)
				{
					estimate[kl] = estimate1[l];
				}
				if (DoSeEst)
				{
					seEst[kl] = seEst1[l];
				}
				if (DoSePred)
				{
					sePred[kl] = sePred1[l];
				}
				kl += npoints;
			} /*for (l = 0; l < ny; l++)*/
		} /*if (extra > 0)*/ 
		if (DoEstimate)
		{
			estimate++;
		}
		if (DoSeEst)
		{
			seEst++;
		}
		if (DoSePred)
		{
			sePred++;
		}
		x += incVars;
		factorVals += incFactors;
	} /*for (k = 0;k < npoints;k++)*/
	
	if (PREVGLMCONTROL & (NONIDENTLINK | NONNORMALDIST))
	{
		if (DoEstimate)
		{
			estimate -= npoints;
		}
		if (DoSeEst)
		{
			seEst -= npoints;
		}
		deLink(estimate, seEst, binomn, incN, npoints);
		sprintf(OUTSTR,
				"NOTE: standard errors assume scale parameter is ");
		formatDouble(OUTSTR + strlen(OUTSTR), PREVGLMSCALE, TRIMLEFT);
		putOUTSTR();
	} /*if (PREVGLMCONTROL & NONIDENTLINK)*/
	
	emptyTrash();
	return (result);

  errorExit:
	putErrorOUTSTR();
	emptyTrash();
	
	return (0);
} /*regpred()*/
