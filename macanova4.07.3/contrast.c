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
#pragma segment Contrast
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
#include "blas.h"

#if (0) /*951012 moved to glm.h*/
extern double          CURRENTRSC;
extern double          ROBUSTVARK; /* this is K from page 40 of Huber 1977 */
#endif /*(0)*/

#define Trash   NAMEFORTRASH
static Symbolhandle Trash = (Symbolhandle) 0; /* repository for scratch */

/*#define x(i,j) ((*REGX)[(long) (i) + (long) (j) * ndata ])*/

#define CONTRASTFUZZ     1.0e-7 /* allowed departure of sum(wts) from zero */

enum contrastScratch
{
	GGROUPCTS = 0,
	GCONVEC,
	NTRASH
};

/*
 *Concoef is the vector of coefficients provided by the user.  It has
 length Conlen

 *Convec is the vector of multipliers for the GLM coefficients

 Convars[j] is the index (first variable is 1) of the j-th factor in term
 Nconvars is the number of factors in the term
*/
static double **Convec, **Concoef;
static long     Conlen, Convars[MAXVARS], Nconvars;

/*
   Check columns to see how to build coefs for contrast
   termi is the index of the model term
   At this point it is known that Convec has a coefficient
   for each cell of the termi-th term, and that 
*/

static void buildConvec(long termi)
{
	long            add, i, cellterm[MAXVARS];
	long            conterm[MAXDIMS], nclasses[MAXDIMS];
	long            jvar, convarj;
	long            nvars = (long) NVARS;
	long            icombo, ncombos;
	long            colno = 0;
	double         *convec = *Convec, *concoef = *Concoef;

	/* put bit pattern of the term in cellterm[] */
	getterm(modelterm(MODEL,termi), cellterm, nvars);

	/*
	   Compute number of combinations and set cellterm[i] = -1 for
	   any variate
    */
	ncombos = setterm(cellterm, nvars, NCLASSES);

	for (jvar = 0; jvar < Nconvars; jvar++)
	{
		nclasses[jvar] = NCLASSES[Convars[jvar] - 1];
	} /*for (jvar = 0; jvar < Nconvars; jvar++)*/
	
	/*
	   Loop through all combinations of indices for the ncombos cells
	   for the term.  Each index runs from 1 to NCLASSES[j] and current
	   indices are in cellterm[].
	*/
	for(icombo = 0;icombo < ncombos;icombo++)
	{
/*
   Check to see if this combination is already in through lower order terms
*/
		if (!inEarlierTerm(termi, cellterm, MODELINFO))
		{/* if here, effective term needs to be entered */
			/* 
			  Now figure which terms of Concoef go into this element of Convec
			  loop through all elements of Concoef and add in appropriate
			  coefs
			*/
			for (jvar = 0; jvar < Nconvars; jvar++)
			{
				conterm[jvar] = 1;
			}
			
	/*
	   cellterm[] contains levels for current cell in the marginal associated
	   with the term

	   conterm[] contains factor levels levels associated with concoef[i]
	*/
			for (i = 0; i < Conlen; i++)
			{
				add = 1.0;
				for (jvar = 0; jvar < Nconvars; jvar++)
				{
					convarj = Convars[jvar] - 1;
					if (cellterm[convarj] > 1)
					{
						/* possible non-match ; check for maximum coords */
						if (conterm[jvar] == NCLASSES[convarj])
						{
							add *= -1.0;
						}
						else if (conterm[jvar] + 1 != cellterm[convarj])
						{
							add = 0;
							break;
						}
					} /*if (cellterm[convarj] > 1)*/
				} /*for (jvar = 0; jvar < Nconvars; jvar++)*/
				convec[colno] += add * concoef[i];

				/* step odometer */
				stepGlmOdometer(conterm, nclasses, Nconvars, 0);
			} /*for (i = 0; i < Conlen; i++)*/
			colno++;
		} /*if (!inEarlierTerm(termi, cellterm, MODELINFO))*/

		/* step odometer */
		stepGlmOdometer(cellterm, NCLASSES, nvars, 1);
	} /*for(icombo = 0;icombo < ncombos;icombo++)*/
} /*buildConvec()*/

static long getconentry(long i, long convars[], long nconvars)
{
	long            j, k, step;

	step = 0;
	for (j = nconvars - 1; j >= 0; j--)
	{
		k = convars[j] - 1;
		step = ((long) (*X[k])[i]) - 1 + step * NCLASSES[k];
	}
	return (step);
} /*getconentry()*/

static void do_balanced(Symbolhandle symhcon, Symbolhandle symhest,
						Symbolhandle symhss, Symbolhandle symhse,
						long errorterm,
						long byvar, long do_ses, long nbyvar)
{
	double             *concoef = DATAPTR(symhcon);
	double             *conest = DATAPTR(symhest);
	double             *conss = DATAPTR(symhss);
	double             *conse = DATAPTR(symhse);
	double             *sserror = *SS + errorterm;
	double              dferror = (*DF)[errorterm];
	double             *y = *Y;
	double             *byvariable;
	double              tmp, mse, con, denom, cellsize;
	long                ibyvar, i, ndata = (long) NDATA; 
	WHERE("do_balanced");

	if(byvar > 0)
	{
		byvariable = *X[byvar - 1];
	}

	if(do_ses)
	{
		mse = *sserror/dferror;
	}

	cellsize = NDATA / (double) (Conlen * nbyvar);
	denom = 0.0;
	for (i = 0; i < Conlen; i++)
	{
		denom += concoef[i]*concoef[i];
	} /*for (i = 0; i < Conlen; i++)*/
	denom *= cellsize;

	for (ibyvar = 0; ibyvar < nbyvar; ibyvar++)
	{
		con = 0.;
		for (i = 0; i < ndata; i++)
		{
			if (byvar < 1 || (ibyvar + 1) == (long) byvariable[i] )
			{
				tmp = concoef[getconentry(i, Convars, Nconvars)];
				con += y[i] * tmp;
			}
		} /*for (i = 0; i < ndata; i++)*/
		conest[ibyvar] = con / cellsize;
		conss[ibyvar] = con * con / denom;
		if(do_ses)
		{
			conse[ibyvar] = sqrt(mse*denom)/cellsize;
		}
		else
		{
			setMissing(conse[ibyvar]);
		}
	} /*for (ibyvar = 0; ibyvar < nbyvar; ibyvar++)*/
} /*do_balanced()*/

/*
   Routine to compute contrast SS for a given contrast
   960829 Fixed bug in use of PREVGLMSCALE.  It was being treated as if it
   were like a variance rather than a standard deviation.

   970421 Removed limitation that factors in a term had to be in the
          original order of factors in the model.  Thus contrast("c.b",v)
          is legal after anova("y=b*c") or anova("y=a+b+c")
   970820 modified to use utility stringToTerm which recognizes
          term names containing {...} expressions
   990215 changed myerrorout() to putOutMsg() and putOutErrorMsg()
   990226 replaced putOUTSTR() by putErrorOUTSTR()
*/

Symbolhandle    contrast(Symbolhandle list)
{

	Symbolhandle    symhname, symhcon, symhby, symhkey;
	Symbolhandle    symhest = (Symbolhandle) 0, symhss = (Symbolhandle) 0;
	Symbolhandle    symhse = (Symbolhandle) 0;
	Symbolhandle    result = (Symbolhandle) 0;
	double         *conss, *conse, *conest;
	long            contermvec[MAXVARS];
	modelType       conterm;
	long            nbyvar, byvar, firstcol, lastcol, termi, type;
	long            i, j, j1, j2, k, inck, ij, ik, jk, kk, kkinc, j1j2;
	long            k1, k2, ik1, ik2, k1k2, ik1k2;
	long            ibyvar;
	long            tmpi, dims[2];
	long            nargs = NARGS(list), nvars = (long) NVARS;
	long            ny = (long) NY, ndata = (long) NDATA, nregx = (long) NREGX;
	long            nterms = (long) NTERMS, errorterm = nterms;
	long            do_ses = 1;
	long            inc = 1; /*increment for ddot()*/
	long            useScale = PREVMODELTYPE & ITERGLMS & ~ROBUSTREG;
	double          con, denom, tmp, singchk, mse, dferror, scale;
	double          sumabscon = 0;
	double         *y, *sserror, *regcoef, *regss, *xtxinv, *wtdresiduals;
	double         *allwts, *misswts, *casewts;
	double         *convec, *conveci, *convecj, *concoef;
	double        **groupctsH, *groupcts;
	double         *byvariable, *xij;
	char           *name, *compname[3], *keyword;
	char           *iterGlm;
	char           *usage =
		"usage: contrast(term, contrast [, byvar] [,error:termName])";
	WHERE("contrast");
	
	Trash = (Symbolhandle) 0;
	compname[0] = "estimate";
	compname[1] = "ss";
	compname[2] = "se";
	OUTSTR[0] = '\0';
	
	if (PREVMODELTYPE == 0 || PREVMODELTYPE & (LEAPS + OLSREG))
	{
		sprintf(OUTSTR,"ERROR: no active non-regression GLM model");
	}
	else if (PREVMODELTYPE & (FASTANOVA | IPF))
	{
		sprintf(OUTSTR,"ERROR: %s() does not work after %s()",
				FUNCNAME, (PREVMODELTYPE & FASTANOVA) ? "fastanova" : "ipf");
	}
	else if (!(PREVMODELTYPE & DOCOEFS)  &&
			 (!(PREVMODELTYPE & ANOVA) || (PREVGLMCONTROL & UNBALANCED)))
	{
		sprintf(OUTSTR,
				"ERROR: %s() does not work after GLM command with coefs:F",
				FUNCNAME);
	}
	else if(nargs < 2)
	{
		sprintf(OUTSTR,"ERROR: %s",usage);
	}
	else if ((symhname = COMPVALUE(list, 0)) == (Symbolhandle) 0 &&
			 !argOK(symhname,0, 1) || 
			 !argOK(symhcon = COMPVALUE(list, 1),REAL, 2))
	{
		strcpy(OUTSTR,usage);
	}
	else if (isscratch(NAME(symhname)) &&
			(!isScalar(symhname) ||
			(type = TYPE(symhname)) != REAL && type != CHAR))
	{
		sprintf(OUTSTR,
				"ERROR: 1st argument not term name or number or factor in model");
		putErrorOUTSTR();
		strcpy(OUTSTR,usage);
	}
	
	if(*OUTSTR)
	{
		goto errorExit;
	}
	
	if (PREVMODELTYPE & ROBUSTREG)
	{
		iterGlm = "robust()";
	}
	else if (PREVMODELTYPE & LOGITREG)
	{
		iterGlm = "logistic()";
	}
	else if (PREVMODELTYPE & POISSONREG)
	{
		iterGlm = "poisson()";
	}
	else if (PREVMODELTYPE & PROBITREG)
	{
		iterGlm = "probit()";
	}
	else if (PREVMODELTYPE & GLMREG)
	{
		iterGlm = "glmfit()";
	}

	if(nargs > 2)
	{ /* check for error:termName */
		symhkey = COMPVALUE(list,nargs-1);
		if (symhkey != (Symbolhandle) 0 && (keyword = isKeyword(symhkey)) &&
		   strncmp(keyword,"error",5) == 0)
		{ /* error term specified for computing standard errors */
			if (PREVMODELTYPE & ITERGLMS)
			{
				sprintf(OUTSTR,
						"ERROR: %s keyword %s not legal after %s", FUNCNAME,
						keyword, iterGlm);
				goto errorExit;
			} /*if (PREVMODELTYPE & ITERGLMS)*/
			else
			{
				if(!isScalar(symhkey) ||
				   (type = TYPE(symhkey)) != CHAR && type != REAL)
				{
					sprintf(OUTSTR,
							"ERROR: value for %s not CHARACTER variable or string or term number",
							keyword);
					goto errorExit;
				}

				if (type == REAL)
				{
					tmp = DATAVALUE(symhkey,0);
					if(tmp != floor(tmp) || tmp <= 0.0 ||
					   tmp > (double) (nterms+1))
					{
						sprintf(OUTSTR,
								"ERROR: %s error term number must be integer between 1 and %ld",
								FUNCNAME, nterms+1);
						goto errorExit;
					}
					errorterm = (long) tmp - 1;
				} /*if (type == REAL)*/
				else					
				{
					name = *TERMNAMES;
					for (errorterm = 0;errorterm <= nterms;errorterm++)
					{
						if(strcmp(name,STRINGPTR(symhkey)) == 0)
						{
							break;
						}
						name = skipStrings(name, 1);
					} /*for (errorterm = 0;errorterm <= nterms;errorterm++)*/

					if(errorterm > nterms)
					{
						sprintf(OUTSTR,
								"ERROR: %s is not a term in the model %s",
								STRINGPTR(symhkey),*STRMODEL);
						goto errorExit;
					}
				} /*if (type == REAL){}else{}*/
			} /*if (PREVMODELTYPE & ITERGLMS){}else{}*/
			nargs--;			
		}
	} /*if(nargs > 2)*/
	
	type = TYPE(symhname);
	if (isScalar(symhname) && (type == REAL || type == CHAR))
	{
		if(type == REAL)
		{
			tmp = DATAVALUE(symhname,0);
			if(tmp != floor(tmp) || tmp <= 0.0 || tmp > (double) nterms)
			{
				sprintf(OUTSTR,
						"ERROR: term number for %s must be integer between 1 and %ld",
						FUNCNAME, nterms);
				goto errorExit;
			}
			name = skipStrings(*TERMNAMES, (long) tmp - 1);
		} /*if(type == REAL)*/
		else
		{
			name = STRINGPTR(symhname);
		}
	} /*if (isScalar(symhname) && (type == REAL || type == CHAR))*/
	else
	{
		name =  NAME(symhname);
	}
	
	for (j = 0; j < MAXVARS; j++)
	{
		Convars[j] = contermvec[j] = 0;
	} /*for (j = 0; j < MAXVARS; j++)*/
	
	/* at this point name is the name of the term a contrast is wanted for*/

	Nconvars = stringToTerm(name, contermvec);

	if (!Nconvars)
	{
		goto errorExit;
	}

	k = 0;
	for (j = 0; j < nvars; j++)
	{
		if (contermvec[j] > 0)
		{
			if (NCLASSES[j] < 0)
			{
				sprintf(OUTSTR, "ERROR: variable %s is not a factor",
						VARNAME(j+1));
				goto errorExit;
			}
			Convars[contermvec[j] - 1] = j + 1;
			contermvec[j] = 2;
		} /*for (j = 0; j < nvars; j++)*/
	} /*for (j = 0; j < nvars; j++)*/
	
	if (Nconvars == 1 && !isVector(symhcon) ||
		Nconvars == 2 && !isMatrix(symhcon,dims) ||
		Nconvars > 2 && NDIMS(symhcon) != Nconvars)
	{
		sprintf(OUTSTR,
				"ERROR: dimensions of contrast coefficients do not match number of factors");
		goto errorExit;
	}

	Conlen = 1;
	for (k = 0; k < Nconvars; k++)
	{
		if (DIMVAL(symhcon,k + 1) != NCLASSES[Convars[k] - 1])
		{
			sprintf(OUTSTR,	
				"ERROR: contrast coefficients do not match dimensions of term %s",
					name);
			goto errorExit;
		}
		Conlen *= NCLASSES[Convars[k] - 1];
	} /*for (k = 0; k < Nconvars; k++)*/

	con = 0.0;		/* check for adding to zero */
	concoef = DATAPTR(symhcon);
	for (i = 0; i < Conlen; i++)
	{
		con += concoef[i];
		sumabscon += fabs(concoef[i]);
	} /*for (i = 0; i < Conlen; i++)*/
	if (sumabscon == 0.0)
	{
		sprintf(OUTSTR,
				"ERROR: all contrast coefficients are 0");
	}
	else if (fabs(con) > CONTRASTFUZZ*sumabscon)
	{
		sprintf(OUTSTR,"ERROR: coefficients add to %g != 0", con);
	}
	if (*OUTSTR)
	{
		goto errorExit;
	}
	
	byvar = 0;
	nbyvar = 1;
	if (nargs > 2)
	{
		/* by variable has been specifed */
		symhby = COMPVALUE(list, 2);
		if (symhby == (Symbolhandle) 0 && !argOK(symhby, 0, 3))
		{
			goto errorExit;
		}
		else if (isScalar(symhby) && (type = TYPE(symhby)) == CHAR)
		{
			name = STRINGPTR(symhby);
		}
		else
		{
			name = NAME(symhby);
			if (isscratch(name))
			{
				sprintf(OUTSTR,
						"ERROR: byvar variable can't be expression or function result");
			}
		}
		if(*OUTSTR)
		{
			goto errorExit;
		}
		
		for (j = 1; j <= nvars; j++)
		{
			if (strcmp(name, VARNAME(j)) == 0)
			{
				byvar = j;
				break;
			}
		} /*for (j = 1; j <= nvars; j++)*/
		if (j > nvars)
		{
			sprintf(OUTSTR,"ERROR: byvar variable %s not in model", name);
		}
		else if (contermvec[j - 1] > 0)
		{
			sprintf(OUTSTR,"ERROR: byvar variable %s is in contrast term",
					name);
		}
		else if ((nbyvar = NCLASSES[j - 1]) < 1)
		{
			sprintf(OUTSTR,"ERROR: byvar variable %s is not a factor", name);
		}
		else if (PREVMODELTYPE & ITERGLMS)
		{
			sprintf(OUTSTR,
					"ERROR: cannot use a byvar variable after %s", iterGlm);
		}
		if(*OUTSTR)
		{
			goto errorExit;
		}
	} /* if(nargs > 2) */

	dferror = (*DF)[errorterm];
	
	do_ses = (dferror > 0.0 || (useScale && PREVGLMSCALE != 0.0));
	if (!do_ses)
	{
		putOutErrorMsg("WARNING: no degrees of freedom for error; standard errors set to MISSING");
	} /*if (!do_ses)*/
		
	if (useScale)
	{
		putOutMsg("NOTE: Deviances based on full model weights");
		if (do_ses)
		{
			scale = PREVGLMSCALE;
			strcpy(OUTSTR,
				   "      Standard errors assume scale parameter is ");
			formatDouble(OUTSTR + strlen(OUTSTR), scale, TRIMLEFT);
			putOUTSTR();
		}
		else
		{
			useScale = 0;
		}		
	}
	else if(PREVMODELTYPE & ROBUSTREG)
	{
		sprintf(OUTSTR,
				"NOTE: approximate ss%s based on converged full model",
				(do_ses) ? " and se" : "");
		putOUTSTR();
	}

	/* ny should be > 1 only for MANOVA */
	result = RSInstall(SCRATCH, 3L, compname, (ny == 1) ? nbyvar : 0L);
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	if(ny == 1)
	{
		symhest = COMPVALUE(result, 0);
		symhss = COMPVALUE(result, 1);
		symhse = COMPVALUE(result, 2);
	} /*if(ny == 1)*/
	else
	{
		/* Multivariate case*/
		symhest = COMPVALUE(result,0) = Makereal(nbyvar * ny);
		symhss = COMPVALUE(result,1) = Makereal(nbyvar*ny*ny);
		symhse = COMPVALUE(result,2) = Makereal(nbyvar * ny);
		if(symhest == (Symbolhandle) 0 || symhss == (Symbolhandle) 0 ||
		   symhse == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		setNAME(symhest,compname[0]);
		setNDIMS(symhest,2);
		setDIM(symhest,1,nbyvar);
		setDIM(symhest,2,ny);
		
		setNAME(symhss,compname[1]);
#if (1) /*why 3 dimensions when no by-variable? Maybe it should be changed*/
		setNDIMS(symhss,3);
		setDIM(symhss,1,nbyvar);
		setDIM(symhss,2,ny);
		setDIM(symhss,3,ny);
#else /*1*/ /* this would change it */
		if (nbyvar > 1)
		{
			setNDIMS(symhss,3);
			setDIM(symhss,1,nbyvar);
			setDIM(symhss,2,ny);
			setDIM(symhss,3,ny);
		}
		else
		{
			setNDIMS(symhss,2);
			setDIM(symhss,1,ny);
			setDIM(symhss,2,ny);
		}
#endif /*1*/
		setNAME(symhse,compname[2]);
		setNDIMS(symhse,2);
		setDIM(symhse,1,nbyvar);
		setDIM(symhse,2,ny);
	} /*if(ny == 1){...}else{...}*/

	if(PREVMODELTYPE & ITERGLMS & ~ROBUSTREG)
	{
		setNAME(symhss, "deviance");
	}
	
	if (PREVMODELTYPE & ANOVA && !(PREVGLMCONTROL & UNBALANCED))
	{			/* balanced */
		do_balanced(symhcon,symhest,symhss,symhse,
					errorterm, byvar, do_ses, nbyvar);
		return (result);
	} /*if (PREVMODELTYPE & ANOVA && !(PREVGLMCONTROL & UNBALANCED))*/

	/* unbalanced if here */
	Trash = GarbInstall(NTRASH);
	if(Trash == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	
	sserror = *SS + errorterm;
	if(do_ses && ny == 1)
	{
		mse = (useScale) ? scale*scale : *sserror/dferror;
	} /*if(do_ses && ny == 1)*/

	/* set conterm to term specified by factors in the term name*/
	zeroTerm(conterm);
	effectiveTerm(contermvec,(modelPointer) conterm,nvars);

	/* first determine if we do additive or subtractive SS */
	for (termi = 0; termi < nterms; termi++)
	{
		if(modeltermEqual(modelterm(MODEL,termi),(modelPointer) conterm))
		{
			break;
		}
	} /*for (termi = 0; termi < nterms; termi++)*/

	if (termi < nterms && nbyvar == 1)
	{			/*  conterm in model, so subtractive */
		/* approach is to build up a set of coefs for the params estimated*/
		Concoef = DATA(symhcon);

		/* make sure that the contrast conforms to anova model variable order*/
#if (0)
		/* 970421 removed test for order of Convars */
		for (i = 1; i < Nconvars; i++)
		{
			if (Convars[i] < Convars[i - 1])
			{
				putOutErrorMsg("ERROR:  contrast term variables not in anova model order");
				goto errorExit;
			}
		} /*for (i = 1; i < Nconvars; i++)*/
#endif

		/* find first and last columns of term coefficients */

		firstcol = 0;
		for (i = 0; i < termi; i++)
		{
			firstcol += buildXmatrix(i, (double *) 0, MODELINFO);
		}
		lastcol = firstcol + buildXmatrix(termi, (double *) 0, MODELINFO) - 1;

		/* get space for modified coefs */

		/* Allocate scratch storage for subtractive case */
		if (!getScratch(Convec,GCONVEC,lastcol - firstcol + 1,double))
		{
			goto errorExit;
		}

		convec = *Convec;
		regss = *REGSS;
		regcoef = *REGCOEF;
		xtxinv = *XTXINV;
		sserror = *SS + errorterm;
		conest = DATAPTR(symhest);
		conss = DATAPTR(symhss);
		conse = DATAPTR(symhse);
		
		doubleFill(convec, 0.0, lastcol - firstcol + 1);

		buildConvec(termi);

#if (0) /*change as of 951228 */
		/* check if missing (pivoted) column has nonzero coef */
		tmpi = 0;

		for (j = firstcol; j <= lastcol; j++)
		{
			if (regss[j] < -0.5 && convec[tmpi] != 0.0)
			{
				putOutErrorMsg("ERROR:  pivoted df has nonzero contrast coefficient");
				goto errorExit;
			}
			tmpi++;
		} /*for (j = firstcol; j <= lastcol; j++)*/
#endif
		/* N.B.: nbyvar is 1 here */
		denom = 0.;
		conveci = convec;
		for (j1 = firstcol; j1 <= lastcol; j1++)
		{
			if (regss[j1] >= -0.5)
			{
				/* non-aliased DF */
				j1j2 = j1 + firstcol * nregx; /* j1 + j2*nregx */
				convecj = convec;
				for (j2 = firstcol; j2 <= lastcol; j2++)
				{
					if (regss[j2] >= -.5)
					{
					
						denom += *conveci * *convecj * xtxinv[j1j2];
					}
					convecj++;
					j1j2 += nregx;
				}
			}
			conveci++;
		} /*for (j1 = firstcol; j1 <= lastcol; j1++)*/
		
		if (denom > 0.0)
		{
			if(do_ses && ny > 1 && !useScale)
			{
				kk = 0;			/* k*(nterms+1)*(ny+1) */
				kkinc = (nterms+1)*(ny+1);
			} /*if(do_ses && ny > 1 && !useScale)*/
		
			for (k = 0; k < ny; k++)
			{
				con = 0.;
				tmpi = 0;
				jk = firstcol + k*nregx; /*j + k*nregx */
				for (j = firstcol; j <= lastcol; j++)
				{
					if(regss[j] > -0.5)
					{
						con += convec[tmpi] * regcoef[jk];
					}
					tmpi++;
					jk++;
				} /*for (j = firstcol; j <= lastcol; j++)*/

				conest[k] = con;
				if(ny > 1 && !useScale)
				{
					mse = sserror[kk]/dferror;
					kk += kkinc;
				}
				if(do_ses)
				{
					conse[k] = sqrt(denom*mse);
				}
				else
				{
					setMissing(conse[k]);
				}
			} /* for (k=0;k<ny;k++) */

			k1k2 = 0;
			for (k2 = 0; k2 < ny; k2++)
			{
				for (k1 = 0; k1 < ny; k1++)
				{
					conss[k1k2++] = conest[k1] * conest[k2] / denom;
				}
			}
		} /*if (denom > 0.0)*/
		else
		{
			putOutErrorMsg("WARNING: zero df for contrast");
			setMissing(tmp);
			doubleFill(conest, tmp, ny);
			doubleFill(conse, tmp, ny);
			doubleFill(conss, tmp, ny*ny);
		} /*if (denom > 0.0){}else{}*/	
		emptyTrash();
		
		return (result);
	} /*if (termi < nterms && nbyvar == 1)*/

	/*
	   conterm not in model or byvar specified, so contrast is additive
	   byvar is illegal when PREVMODELTYPE & ITERGLMS != 0
	*/

	/* Allocate scratch storage for additive case */
	if(!getScratch(Convec,GCONVEC,ndata,double) ||
	   !getScratch(groupctsH,GGROUPCTS,Conlen,double))
	{
		goto errorExit;
	}
	convec = *Convec;
	sserror = *SS + errorterm;
	y = *Y;
	groupcts = *groupctsH;
	if(byvar > 0)
	{
		byvariable = *X[byvar - 1];
	}

	conest = DATAPTR(symhest);
	conss = DATAPTR(symhss);
	concoef = DATAPTR(symhcon);
	conse = DATAPTR(symhse);

	allwts = (PREVMODELTYPE & USEWEIGHTS) ? *ALLWTS : (double *) 0;
	misswts = (PREVMODELTYPE & MISSWEIGHTS) ? *MISSWTS : (double *) 0;
	casewts = (PREVMODELTYPE & CASEWEIGHTS) ? *CASEWTS : (double *) 0;
	wtdresiduals = (PREVMODELTYPE & ITERGLMS) ? *WTDRESIDUALS : (double *) 0;

	/* repeat for each level of by variable */
	for (ibyvar = 0; ibyvar < nbyvar; ibyvar++)
	{
		for (j = 0; j < Conlen; j++)
		{
			groupcts[j] = 0.;
		}

		/* compute cell counts for every cell */
		for (i = 0; i < ndata; i++)
		{/* for all data, even missing */
			if ((misswts == (double *) 0 || misswts[i] != 0.0) &&
				(casewts == (double *) 0 || casewts[i] != 0.0))
			{
				if (byvar < 1 || byvariable[i] == ibyvar + 1)
				{
					groupcts[getconentry(i, Convars, Nconvars)] += 1.;
				}
			}
		} /*for (i = 0; i < ndata; i++)*/

/* Rescale coefs so they apply to y and compute sum of contrast coefficients */
		sumabscon = tmp = 0.;
		for (j = 0; j < Conlen; j++)
		{
			if (groupcts[j] > 0.)
			{
				groupcts[j] = concoef[j] / groupcts[j];
				tmp += concoef[j];
				sumabscon += fabs(concoef[j]);
			}
		} /*for (j = 0; j < Conlen; j++)*/

		if (fabs(tmp) > CONTRASTFUZZ * sumabscon || sumabscon == 0)
		{		/* not a contrast or all 0 coefs at this level of by var */
			inck = nbyvar*ny;
			ik1 = ibyvar;
			for(k1 = 0;k1 < ny;k1++)
			{ /* ik1 = ibyvar + k1*nbyvar */
				setMissing(conest[ik1]);
				setMissing(conse[ik1]);
				ik1k2 = ik1;
				for(k2 = 0;k2 < ny;k2++)
				{ /* ik1k2 = ibyvar + k1*nbyvar + k2*nbyvar*ny */
					setMissing(conss[ik1k2]);
					ik1k2 += inck;
				}
				ik1 += nbyvar;
			} /*for(k1 = 0;k1 < ny;k1++)*/
		} /*if (fabs(tmp) > CONTRASTFUZZ * sumabscon)*/
		else
		{ /* apparently valid contrast (sums to zero over non-empty cells) */
			singchk = 0.0;
			for (i = 0; i < ndata; i++)
			{/* for all data, even missing */
				if (misswts == (double *) 0 || misswts[i] != 0.0)
				{
					if (byvar < 1 || byvariable[i] == ibyvar + 1)
					{
						convec[i] = groupcts[getconentry(i, Convars, Nconvars)];
						singchk += convec[i] * convec[i];
					}
					else
					{
						convec[i] = 0.;
					}
				}
				else
				{
					convec[i] = 0.0;
				}
			} /*for (i = 0; i < ndata; i++)*/

			/* now put in weights if weights are used */

			if (PREVMODELTYPE & USEWEIGHTS)
			{
				double    weight;
				
				for (i = 0; i < ndata; i++)
				{
					if(PREVMODELTYPE & ROBUSTREG)
					{
						weight = (misswts != (double *) 0) ? 
							misswts[i] : 1.0;
					}
					else
					{
						weight = allwts[i];
					}
					convec[i] *= weight;
				} /*for (i = 0; i < ndata; i++)*/
			} /*if (PREVMODELTYPE & USEWEIGHTS)*/

			/* when no byvar, sweep all columns out of Convec*/

			if (nbyvar == 1)
			{
				for (j = 0; j < nregx; j++)
				{
					xij = *REGX + j * ndata; /* *xij is x(i,j) */
					con = DDOT(&ndata, convec, &inc, xij, &inc);
					denom = DDOT(&ndata, xij, &inc, xij, &inc);
					
					if (denom > 0.0)
					{
						xij = *REGX + j * ndata; /* &x(i, j) */
						con /= -denom;
						DAXPY(&ndata, &con, xij, &inc, convec, &inc);
					} /*if (denom > 0.0)*/
				} /*for (j = 0; j < nregx; j++)*/
			} /*if (nbyvar == 1)*/

			denom = DDOT(&ndata, convec, &inc, convec, &inc);

			if (denom / singchk <= REGTOL)
			{
				putOutErrorMsg("WARNING: zero df for contrast");
				inck = nbyvar*ny;
				ik2 = ibyvar; /*ibyvar + k2 * nbyvar*/
				for(k2 = 0;k2 < ny;k2++)
				{
					ik1k2 = ik2; /* ibyvar + k2 * nbyvar + k1*nbyvar*ny */;
					setMissing(conest[ik1k2]);
					setMissing(conse[ik1k2]);
					for(k1=0;k1<ny;k1++)
					{
						setMissing(conss[ik1k2]);
						ik1k2 += inck;
					}
					ik2 += nbyvar;
				}
				continue;
			} /*if (denom / singchk <= 1.0e-15)*/

			if(ny > 1 && do_ses && !useScale)
			{
				kk = 0; /*j2*(nterms+1)*(ny+1)*/
				kkinc = (nterms+1)*(ny+1);
			}
			
			ij = ibyvar;				/* ibyvar + k * nbyvar */
			ik = 0; /* i+k*ndata*/
			for (k = 0; k < ny; k++)
			{
				con = 0.;
				for (i = 0; i < ndata; i++)
				{
					if (PREVMODELTYPE & ITERGLMS)
					{
						con += convec[i] * wtdresiduals[ik];
					}
					else if (PREVMODELTYPE & USEWEIGHTS)
					{
						if (misswts == (double *) 0 || misswts[i] > 0.0)
						{
							con += convec[i] * allwts[i] * y[ik];
						}
					}
					else
					{
						con += convec[i] * y[ik];
					}
					ik++;
				} /*for (i= 0; i < ndata; i +)*/

				/*		conest[ibyvar+k*nbyvar] = con;   */
				conest[ij] = con;
				if(ny > 1 && !useScale)
				{
					mse = sserror[kk]/dferror;
					kk += kkinc;
				}
				if(do_ses)
				{
					conse[ij] = sqrt(denom*mse);
				}
				else
				{
					setMissing(conse[ij]);
				}
				ij += nbyvar;
			} /*for (k = 0; k < ny; k++)*/

			if(ny == 1)
			{
				con = con * con / denom;
				conss[ibyvar] = con;
			} /*if(ny == 1)*/
			else
			{
				inck = nbyvar*ny;
				ik2 = ibyvar; /* ibyvar + k2*nbyvar */
				for (k2 = 0; k2 < ny; k2++)
				{
					ik1k2 = ik2; /* ibyvar + k2*nbyvar + k1*nbyvar*ny */
					ik1 = ibyvar; /* ibyvar + k1*nbyvar */
					for (k1 = 0; k1 < ny; k1++)
					{
						conss[ik1k2] = conest[ik2] * conest[ik1] / denom;
						ik1k2 += inck; 
						ik1 += nbyvar;
					} /*for (k = 0; k < ny; k1++)*/
					ik2 += nbyvar;
				} /*for (k2 = 0; k2 < ny; k2++)*/
			}  /*if(ny == 1){...}else{...}*/
		} /*if (fabs(tmp) > CONTRASTFUZZ * sumabscon){}else{}*/
	} /*for (ibyvar = 0; ibyvar < nbyvar; ibyvar++)*/

	emptyTrash();
	return (result);

  errorExit:
	putErrorOUTSTR();

	emptyTrash();
	Removesymbol(result);

	return (0);
} /*contrast()*/


