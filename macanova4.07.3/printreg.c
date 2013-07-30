/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 4.06 or later
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


#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment Printreg
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
/*
   Function to print regression output
   This always includes a table of coefficients, standard errors and
   t-statistics and may optionally include P-values

   970820 Modified to use new macro VARNAME to obtain name of variable

   980303 Output now lists number of cases omitted because of 0 weight
          or MISSING values
*/
#define MINLABELWIDTH    8
#define notVariableName(name) (strcmp(name, "CONSTANT") == 0 || \
			(strncmp(name, "ERROR", 5) == 0 && isdigit(name[5])))
void printregress(long pvals)
{
	double         *df = *DF, *ss = *SS, *allwts;
	long            ndata = (long) NDATA, nregx = (long) NREGX;
	long            nOmitted = ndata - (long) NOTMISSING;
	long            i, modeldf = 0, errordf = (long) df[nregx];
	long            ivar, jvar;
	long            iprev = -1;
	long            nterms = (long) NTERMS;
	long            weighted = MODELTYPE & (MISSWEIGHTS | CASEWEIGHTS);
	double          dw1, dw2, tmp, sd, mse, r2, fstat = 0.0, pvalue;
	double         *residuals, *xtxinv = *XTXINV;
	double         *regcoef = *REGCOEF;
	char           *termnames = *TERMNAMES;
	char           *undefined = "undefined";
	char           *varName;
	char           *outstr;
	char            numberBuf[50];
	int             labelWidth = MINLABELWIDTH;
	WHERE("printregress");

	for (ivar = 0, jvar = 0; ivar < nterms; ivar++)
	{
		if (notVariableName(termnames))
		{
			varName = termnames;
			if (istempname(varName))
			{
				varName++;
			}
		}
		else
		{
			jvar++;
			varName = VARNAME(jvar);
			if (varName == VARNAMES[jvar] && istempname(varName))
			{
				varName++;
			}
		}

		if (strlen(varName) > labelWidth)
		{
			labelWidth = strlen(varName);
		}
		termnames = skipStrings(termnames, 1);
	} /*for (ivar = 0; ivar <= nterms; ivar++)*/

	termnames = *TERMNAMES;
	residuals = (MODELTYPE & CASEWEIGHTS) ? *WTDRESIDUALS : *RESIDUALS;

	if(weighted)
	{
		allwts = *ALLWTS;
	}

	mse = (errordf > 0) ? mse = ss[nregx] / (double) errordf : 0.0;

	r2 = 0.0;
	for (ivar = 0; ivar < nregx; ivar++)
	{
		if (df[ivar] > 0.0 && !modeltermEqual(modelterm(MODEL,ivar),
											 (modelPointer) NULLTERM))
		{
			r2 += ss[ivar];
			modeldf += (long) df[ivar];
		}
	} /*for (ivar = 0; ivar < nregx; ivar++)*/

	if(mse != 0.0 && modeldf > 0)
	{
		fstat = (r2/(double) modeldf) / mse;
	}

	r2 =  (r2 + ss[nregx] > 0) ? r2/(r2 + ss[nregx]) : -1.0;

	/* Compute Durbin-Watson statistic */
	if(mse > 0)
	{
		dw1 = 0.0;
		dw2 = 0.0;
		for (i = 0; i < ndata; i++)
		{
			if (!weighted || allwts[i] > 0.0)
			{
				tmp = residuals[i];
				dw2 += tmp*tmp;
				if(iprev >= 0)
				{
					tmp -= residuals[iprev];
					dw1 += tmp*tmp;
				}
				iprev = i;
			} /*if (!weighted || allwts[i] > 0.0)*/
		} /*for (i = 0; i < ndata; i++)*/
		if(dw2 > 0.0)
		{
			dw1 /= dw2;
		}
	} /*if(mse > 0)*/
	
	/*  print out the regression  results */

	outstr = OUTSTR;
	outstr += indentBuffer(outstr, labelWidth);
	outstr += formatChar(outstr, "Coef", NOTRIM);
	outstr += formatChar(outstr, "StdErr", NOTRIM);
	outstr += formatChar(outstr, "t", DOFIXED);
	if (pvals)
	{
		outstr += formatChar(outstr, "P-Value", NOTRIM);
	}
	putOUTSTR();

	for (ivar = 0, jvar = 0; ivar < nterms; ivar++)
	{
		if (notVariableName(termnames))
		{
			varName = termnames;
		}
		else
		{
			jvar++;
			varName = VARNAME(jvar);
			if (varName == VARNAMES[jvar] && istempname(varName))
			{
				varName++;
			}
		}
		
		if (df[ivar] > 0.0)
		{
			sd = sqrt(xtxinv[ivar * nregx + ivar] * mse);
			outstr = OUTSTR;
			sprintf(outstr, "%*s", (int) -labelWidth, varName);
			outstr += strlen(outstr);
			outstr += formatDouble(outstr, regcoef[ivar], NOTRIM);
			if(errordf > 0)
			{
				outstr += formatDouble(outstr, sd, NOTRIM);
			}
			else
			{
				outstr += formatChar(outstr, undefined, NOTRIM);
			}
			if(sd > 0.0)
			{
				outstr += formatDouble(outstr,regcoef[ivar] / sd, NOTRIM);
			}
			else
			{
				outstr += formatChar(outstr, undefined, NOTRIM);
			}
			if (pvals)
			{
				if (sd > 0)
				{
					pvalue = 2.0*(1.0 - Cstu(fabs(regcoef[ivar]/sd),
											 (double) errordf));
					outstr += formatDouble(outstr, pvalue, NOTRIM);
				}
				else
				{
					outstr += formatChar(outstr, undefined, NOTRIM);
				}
			}
		} /*if (df[ivar] > 0.0)*/
		else
		{
			sprintf(OUTSTR, "%*s  deleted", (int) -labelWidth, varName);
		}
		putOUTSTR();

		termnames = skipStrings(termnames, 1);
		checkInterrupt(interruptExit);
	} /*for (ivar = 0; ivar < nterms; ivar++)*/

	myeol();

	outstr = OUTSTR;
	sprintf(outstr, "N: %ld,",ndata);
	outstr += strlen(outstr);
	if (nOmitted > 0)
	{
		sprintf(outstr, " Omitted: %ld,", nOmitted);
		outstr += strlen(outstr);
	}
	strcpy(outstr, " MSE: ");
	outstr += strlen(outstr);

	if(errordf > 0)
	{
		outstr += formatDouble(outstr, mse, NOTRIM | TRIMLEFT);
	}
	else
	{
		outstr += formatChar(outstr, undefined, TRIMLEFT);
	}
	sprintf(outstr,", DF: %ld, R^2: ", errordf);
	outstr += strlen(outstr);
	if ( r2 >= 0.0)
	{
		outstr += formatDouble(outstr, r2, DOFIXED | TRIMLEFT);
	}
	else
	{
		outstr += formatChar(outstr, undefined, TRIMLEFT);
	}
	putOUTSTR();

	outstr = OUTSTR;
	sprintf(OUTSTR,"Regression F(%ld,%ld): ",modeldf,errordf);
	
	outstr += strlen(outstr);

	if(mse > 0 && modeldf > 0)
	{
		sprintf(numberBuf, DATAFMT, fstat);
		strcpy(outstr, fieldStart(numberBuf));
		if (pvals)
		{
			outstr += strlen(outstr);
			strcpy(outstr,", P-value: ");
			pvalue = 1 - Csne(fstat, modeldf, errordf);
			outstr += strlen(outstr);
			sprintf(numberBuf, DATAFMT, pvalue);
			strcpy(outstr, fieldStart(numberBuf));
		} /*if (pvals)*/
	} /*if(mse > 0 && modeldf > 0)*/
	else
	{
		strcpy(outstr, undefined);
	}
	outstr += strlen(outstr);
	
	strcpy(outstr, ", Durbin-Watson: ");
	outstr += strlen(outstr);
	if(mse > 0)
	{
		sprintf(numberBuf, DATAFMT, dw1);
		strcpy(outstr, fieldStart(numberBuf));
	}
	else
	{
		strcpy(outstr, undefined);
	}
	putOUTSTR();

	strcpy(OUTSTR,"To see the ANOVA table type 'anova()'");
	putOUTSTR();
	/* fall through */
  interruptExit:
	;
	if (pvals)
	{
		UNLOADSEG(Cstu);
		UNLOADSEG(Csne);
	}
} /*printregress()*/
