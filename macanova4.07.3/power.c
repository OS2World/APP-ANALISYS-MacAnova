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
#pragma segment Power
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

enum designCodes
{
	CRD = 1,
	RBD
};

/*
  samplesize(noncen,ngrp,alpha,pwr [,design:"crb"] [,maxn:n])
*/

#ifndef MAXN
#define         MAXN 256.0
#endif /*MAXN*/

/*
  970730 Added keyword maxn, changed default from 127 to 256 and
         made other cosmetic modifications
  980303 Fixed bug in error message
  980616 Fixed more bugs in error message
  990226 Replaced putOUTSTR() by putErrorOUTSTR()
*/
Symbolhandle    samplesize(Symbolhandle list)
{
	Symbolhandle    result, args[4];
	Symbolhandle    symhDesign = (Symbolhandle) 0, symhMaxn = (Symbolhandle) 0;
	Symbolhandle    symhKey;
	long            i;
	double          tmp, left, right, noncen, alpha, powr,trialPower;
	double          numDf, denomDf;
	double          ngrp;
	double          critval;
	double          maxn = MAXN;
	long            minNgrp;
	long            design = CRD;
	long            nargs = NARGS(list);
	char           *keyword, *designName = "crd";
	char           *usage = "usage: samplesize(noncen,ngrp,alpha,pwr [,design:\"rbd\"])";
	char           *badargsFmt = "%s (arg. %d to %s())";
	char           *argWhat[4];
	char            errormsg[80];
	WHERE("samplesize");

	argWhat[0] = "non-centrality";
	argWhat[1] = "number of groups";
	argWhat[2] = "significance level";
	argWhat[3] = "desired power";

	if (nargs < 4)
	{
		badNargs(FUNCNAME,-1000 - 4);
		goto errorExit;
	}
	else if (nargs > 6)
	{
		badNargs(FUNCNAME,-6);
		goto errorExit;
	}
	for (i = 0;i < 4;i++)
	{
		args[i] = COMPVALUE(list,i);
	}
	if (!isNumber(args[0], NONNEGATIVEVALUE))
	{
		sprintf(errormsg, badargsFmt, argWhat[0], 1, FUNCNAME);
		notNonNegativeReal(errormsg);
		goto errorExit;
	}

	if (!isInteger(args[1], POSITIVEVALUE))
	{
		sprintf(errormsg, badargsFmt, argWhat[1], 2, FUNCNAME);
		notPositiveInteger(errormsg);
		goto errorExit;
	}
	if (!isNumber(args[2], POSITIVEVALUE))
	{
		sprintf(errormsg, badargsFmt, argWhat[2], 3, FUNCNAME);
		notPositiveReal(errormsg);
		goto errorExit;
	}
	if (!isNumber(args[3], POSITIVEVALUE))
	{
		sprintf(errormsg, badargsFmt, argWhat[3], 4, FUNCNAME);
		notPositiveReal(errormsg);
		goto errorExit;
	}

	for (i = 4; i < nargs; i++)
	{
		symhKey = COMPVALUE(list, i);

		keyword = isKeyword(symhKey);
		if (keyword)
		{
			if (strcmp(keyword, "design") == 0)
			{
				symhDesign = symhKey;
			}
			else if (strcmp(keyword, "maxn") == 0)
			{
				symhMaxn = symhKey;
			}
			else
			{
				badKeyword(FUNCNAME, keyword);
				goto errorExit;
			}
		} /*if (keyword)*/
		else
		{
			sprintf(OUTSTR,
					"ERROR: argument %ld to %s() must be keyword phrase",
					i+1, FUNCNAME);
			goto errorExit;
		}
	} /*for (i = 4; i < nargs; i++)*/
	
	if (symhDesign != (Symbolhandle) 0)
	{
		if (!isCharOrString(symhDesign))
		{
			notCharOrString(isKeyword(symhDesign));
			goto errorExit;
		}
		designName = STRINGPTR(symhDesign);
		if (strcmp(designName,"crd") == 0 || strcmp(designName,"cr") == 0)
		{
			design = CRD;
		}
		else if (strcmp(designName,"rbd") == 0 || strcmp(designName,"rb") == 0 ||
				 strcmp(designName,"rcb") == 0)
		{
			design = RBD;
		}
		else
		{
			sprintf(OUTSTR,"ERROR: design name %s not recognized by %s()",
					designName,FUNCNAME);
			goto errorExit;
		}
	}
	if (symhMaxn != (Symbolhandle) 0)
	{
		if (!isInteger(symhMaxn, POSITIVEVALUE))
		{
			notPositiveInteger(isKeyword(symhMaxn));
			goto errorExit;
		}
		maxn = DATAVALUE(symhMaxn, 0);
	}

	minNgrp = (design == CRD) ? 1 : 2;
	if (DATAVALUE(args[1], 0) < minNgrp)
	{
		sprintf(OUTSTR, "ERROR: %s for %s() must be integer >= %ld",
				argWhat[1], FUNCNAME, minNgrp);
		goto errorExit;
	}
	
	for (i = 2; i < 3; i++)
	{
		if (DATAVALUE(args[i], 0) >= 1.0)
		{
			sprintf(OUTSTR,
					"ERROR: %s for %s() must be between 0 and 1",
					argWhat[i], FUNCNAME);
			goto errorExit;
		}
	}

	noncen = DATAVALUE(args[0], 0);
	ngrp = DATAVALUE(args[1], 0);
	alpha = DATAVALUE(args[2], 0);	
	powr = DATAVALUE(args[3], 0);

	right = 1.0;

	/* for CRD, treat problem as one sample t-test of H0: mu = 0 */
	numDf = (ngrp > 1.0) ? ngrp - 1.0 : 1.0;

	do /*while (trialPower < powr)*/
	{
		if (interrupted(DEFAULTTICKS) != INTNOTSET)
		{
			goto interruptExit;
		}
		left = right;
		if (right >= maxn)
		{
			break;
		}
		right *= 2.0;
		if (right > maxn)
		{
			right = maxn;
		}
		
		denomDf = (right - 1.0) * ((design == CRD) ? ngrp : (ngrp - 1.0));
		critval = Qsne(1 - alpha, numDf, denomDf);
		trialPower = 1.0 - noncentF(critval, right * noncen, numDf, denomDf);
	}while (trialPower < powr);


	if (trialPower > powr)
	{
		left = floor(left) + .1;

		while (right - left > 1.1)
		{
			if (interrupted(DEFAULTTICKS) != INTNOTSET)
			{
				goto interruptExit;
			}
			tmp = floor((right + left) / 2.0 + .1);
			denomDf = (tmp - 1.0) * ((design == CRD) ? ngrp : (ngrp - 1.0));
			critval = Qsne(1 - alpha, numDf, denomDf);
			trialPower = noncentF(critval,tmp * noncen, numDf, denomDf);
			if (trialPower > 1.0)
			{ /* not fully converged, ignore */
				trialPower -= floor(trialPower);
			}
			trialPower = 1.0 - trialPower;
			if (trialPower < powr)
			{
				left = tmp;
			}
			else
			{
				right = tmp;
			}
		} /*while (right - left > 1.1)*/
	} /*if (trialPower > powr)*/
	else
	{
		sprintf(OUTSTR,"WARNING: %s() truncated at %4.0f", FUNCNAME, right);
		putErrorOUTSTR();
	}

	result = RInstall(SCRATCH, 1L);

	if (result != (Symbolhandle) 0)
	{
		DATAVALUE(result,0) = right;
	}
	
	return (result);

  errorExit:
	putErrorOUTSTR();

	putOutMsg(usage);

  interruptExit:
	
	return (0);

} /*samplesize()*/

enum powerOpCode
{
	IPOWER = 1,
	IPOWER2
};

/* 
  power(noncen,ngrp,alpha,groupsize)
  power2(noncen2,ndf,alpha,ddf)
*/
/* name changed for use in MPW where pow is equivalenced to power*/
Symbolhandle    power1(Symbolhandle list)
{
	Symbolhandle    args[4], result = (Symbolhandle) 0;
	Symbolhandle    symhDesign = (Symbolhandle) 0;
	double          noncen, ngrp, alpha, groupSize, powr;
	double          Qsne(), noncentF();
	double          numDf, denomDf, critval;
	long            nargs = NARGS(list);
	long            op, design = CRD;
	long            nvals = 1, inc[4], ii[4];
	char           *keyword, *designName = "crd";
	char           *usage;
	char           *argWhat[4];
	long            i, j, length;
	WHERE("power1");
	
	*OUTSTR = '\0';
	if (strcmp(FUNCNAME,"power") == 0)
	{
		op = IPOWER;
		usage = "power(noncen,ngroups,alpha,groupsize [,design:\"rbd\"])";
		if (nargs < 4 || nargs > 5)
		{
			badNargs(FUNCNAME,(nargs < 4) ? -1004 : -5);
			goto errorExit;
		}
	}
	else
	{
		op = IPOWER2;
		usage = "power2(noncen2,numDf,alpha,denomDf)";
		if (nargs != 4)
		{
			badNargs(FUNCNAME,4);
			goto errorExit;
		}
	}

	argWhat[0] = "non-centrality";
	argWhat[1] = (op == IPOWER) ? "number of groups" : "numerator d.f.";
	argWhat[2] = "significance level";
	argWhat[3] = (op == IPOWER) ? "group size" : "denominator d.f.";
	
	for (j=0;j < 4;j++)
	{
		args[j] = COMPVALUE(list,j);
		if (!argOK(args[j], REAL, j+1))
		{
			goto errorExit;
		}

		if (!isVector(args[j]))
		{
			sprintf(OUTSTR,
					"ERROR: %s for %s() must be REAL scalar or vector",
					argWhat[j], FUNCNAME);
			goto errorExit;
		}

		if (anyMissing(args[j]))
		{
			sprintf(OUTSTR,
					"ERROR: %s for %s() contains MISSING values",
					argWhat[j], FUNCNAME);
			goto errorExit;
		} /*if (anyMissing(args[j]))*/
		
		ii[j] = 0;
		inc[j] = 0;
		length = DIMVAL(args[j],1);
		if (j == 0)
		{
			nvals = length;
		}
		else if (length > 1)
		{
			if (nvals > 1 && length != nvals)
			{
				sprintf(OUTSTR,
						"ERROR:  %s for %s() is vector with non-matching length",
						argWhat[j], FUNCNAME);
				goto errorExit;
			}
			nvals = length;
		}
		inc[j] = (length == 1) ? 0 : 1;
	} /*for (j=0;j < 4;j++)*/
	if (nargs == 5)
	{ /* must be power() */
		char      *keyName = "design";
		
		symhDesign = COMPVALUE(list, 4);
		keyword = isKeyword(symhDesign);
		if (keyword == (char *) 0)
		{
			sprintf(OUTSTR,
					"ERROR: argument 5 to %s() must be %s:\"rbd\" or %s:\"crd\"",
					FUNCNAME, keyName, keyName);
			goto errorExit;
		}
		
		if (strcmp(keyword, keyName) != 0)
		{
			badKeyword(FUNCNAME, keyword);
			goto errorExit;
		}
		
		if (!isCharOrString(symhDesign))
		{
			notCharOrString(keyword);
			goto errorExit;
		}
		designName = STRINGPTR(symhDesign);
		if (strcmp(designName,"crd") == 0 || strcmp(designName,"cr") == 0)
		{
			design = CRD;
		}
		else if (strcmp(designName,"rbd") == 0 || strcmp(designName,"rb") == 0 ||
				 strcmp(designName,"rcb") == 0)
		{
			design = RBD;
		}
		else
		{
			sprintf(OUTSTR,"ERROR: illegal value \"%s\" for keyword %s on %s()",
					designName, keyword, FUNCNAME);
			goto errorExit;
		}
	} /*if (nargs == 5)*/

	length = (inc[0] > 0) ? nvals : 1;
	for (i=0;i<length;i++)
	{
		if ((noncen = DATAVALUE(args[0],i)) < 0.0)
		{
			sprintf(OUTSTR, "ERROR: %s for %s() not all non-negative", argWhat[0], FUNCNAME);
			goto errorExit;
		}
	} /*for (i=0;i<length;i++)*/

	length = (inc[1] > 0) ? nvals : 1;
	for (i=0;i<length;i++)
	{
		numDf = DATAVALUE(args[1],i);
		if (numDf < 1.0 || numDf != floor(numDf))
		{
			sprintf(OUTSTR,
					"ERROR: %s for %s() not all positive integers",
					argWhat[1], FUNCNAME);
			goto errorExit;
		}
	} /*for (i=0;i<length;i++)*/

	length = (inc[2] > 0) ? nvals : 1;
	for (i=0;i<length;i++)
	{
		alpha = DATAVALUE(args[2],i);
		if (alpha <= 0.0 || alpha >= 1.0)
		{
			sprintf(OUTSTR,
					"ERROR: %s for %s() not all between 0 and 1",
					argWhat[2], FUNCNAME);
			goto errorExit;
		}
	} /*for (i=0;i<length;i++)*/

	length = (inc[3] > 0) ? nvals : 1;
	for (i = 0;i < length;i++)
	{
		denomDf = DATAVALUE(args[3],i);
		if (denomDf < 1.0 || denomDf != floor(denomDf) ||
			(op == IPOWER && denomDf < 2.0))
		{
			sprintf(OUTSTR,
					"ERROR: %s for %s() not all integers > %ld",
					argWhat[3], FUNCNAME, (op == IPOWER) ? 1L : 0L);
			goto errorExit;
		}
	} /*for (i=0;i<length;i++)*/

	result = RInstall(SCRATCH, nvals);

	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	
	for (i = 0;i < nvals;i++)
	{
		noncen = DATAVALUE(args[0],ii[0]);
		if (noncen == 0.0)
		{
			powr = alpha;
		}
		else
		{
			numDf = DATAVALUE(args[1],ii[1]);
			alpha = DATAVALUE(args[2],ii[2]);
			denomDf = DATAVALUE(args[3],ii[3]);
			if (op == IPOWER)
			{
				ngrp = numDf;
				groupSize = denomDf;
				numDf = (ngrp > 1.0) ? ngrp - 1.0 : 1.0;
				denomDf = (groupSize - 1.0) * 
					((design == CRD || ngrp == 1) ? ngrp : (ngrp - 1.0));
				noncen *= groupSize;
			} /*if (op == IPOWER)*/

			critval = Qsne(1 - alpha, numDf, denomDf);
			powr = noncentF(critval, noncen, numDf, denomDf);
			if (powr > 1.0)
			{
				sprintf(OUTSTR,
						"WARNING: non-central F not fully converged in %s() after %g terms",
						FUNCNAME, floor(powr));
				putErrorOUTSTR();
				powr -= floor(powr);
			}
			powr = 1.0 - powr;
		}
		DATAVALUE(result,i) = powr;
		for (j = 0; j < 4; j++)
		{
			ii[j] += inc[j];
		}
	} /*for (i = 0;i < nvals;i++)*/
	
	return (result);

  errorExit:
	putErrorOUTSTR();
	
	putOutMsg(usage);
	return (0);
} /*power1()*/

#undef UNDEFINED__
#ifdef UNDEFINED__
/* 
  This has been obsoleted by Qsne in pvalsub.c which calls ppbeta in
  ppbeta() in betabase.c
*/

double          Fquant(double p, double n1, double n2)
{
	/* find F quantile by interval bisection */

	/*  first get starting values via Cornish-Fisher approx
	    eqn 17 p. 82 of Johnson and Kotz */

	double          left, right, center, tmp, delta, sigma;
	extern double   Csne(), ppnd();
	long            fault;

	tmp = ppnd(p, &fault);
	sigma = sqrt(0.5 * (1.0 / n1 + 1.0 / n2));
	delta = 0.5 * (1. / n2 - 1. / n1);
	center = delta * (1.0 + (tmp * tmp - 1.) / 3.) +
		tmp * sigma / sqrt(1.0 - sigma * sigma);

	if (Csne(center, n1, n2) > p)
	{
		right = center;
		left = right * 0.9;
		while (Csne(left, n1, n2) > p)
		{
			left *= 0.9;
		}
	}
	else
	{
		left = center;
		right = left * 1.1;
		while (Csne(right, n1, n2) < p)
		{
			right *= 1.1;
		}
	}

	while ((right - left) * 2 / (right + left) > 0.001)
	{
		center = (left + right) / 2.;
		if (Csne(center, n1, n2) < p)
		{
			left = center;
		}
		else
		{
			right = center;
		}
	}

	return (center);
} /*Fquant()*/

/* This has been replaced by a new version in pvalsub.c calling betanc */
double          noncentF(double x, double lam, double n1, double n2)
{
	/* approximate cumulative for noncentral F using Tiku's approx
	   or Severo and Zelen approx if Tiku fails
	   p 195 of Johnson and kotz */

	double          H, K, n1p, c, b, n1pluslam, tmp;
	extern double   Csne(), Cnor();
	WHERE("noncentF");
	
	n1pluslam = n1 + lam;
	if (n2 > 2.0)
	{
		H = 2.*n1pluslam*n1pluslam*n1pluslam +
			3.*n1pluslam*(n1 + 2.*lam)*(n2 - 2.) +
			(n1 + 3.*lam)*(n2 - 2.)*(n2 - 2.);
		K = n1pluslam*n1pluslam + (n2 - 2.)*(n1 + 2*lam);

		n1p = 0.5*(n2 - 2.)*(sqrt(H*H/(H*H - 4.*K*K*K)) - 1.);
		c = (n1p/n1)*H/K/(2*n1p + n2 - 2.);
		b = -n2*(c - 1. - lam/n1)/(n2 - 2.);

		return (Csne((x - b)/c, n1p, n2));
	}

	tmp = pow(n1*x/(n1 + lam), 1./3.);
	b = sqrt((2./9.)*((n1 + 2.*lam)/(n1pluslam*n1pluslam) + tmp*tmp/n2));

	c = (1. - (2./9.)/n2)*tmp - 1. + 
		(2./9.)*(n1 + 2.*lam)/(n1pluslam*n1pluslam);
	return (Cnor(c/b));
} /*noncentF()*/
#endif /*UNDEFINED__*/
