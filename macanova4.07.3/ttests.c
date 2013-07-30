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
#pragma segment Ttests
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"


#ifndef NOPROTOTYPES
static double tquantile(double /*p*/, double /*n*/);
static long tmeanss(double **/*x*/, long /*n*/, double */*mean*/, double */*ss*/);
#else /*NOPROTOTYPES*/
static double tquantile(/*double p, double n*/);
static long tmeanss(/*double **x, long n, double *mean, double *ss*/);
#endif /*NOPROTOTYPES*/

static double          tquantile(double p, double n)
{

	return (Qstu(p,n));

} /*tquantile()*/


static long  tmeanss(double **xH, long n, double * mean, double * ss)
{
	double         *x = *xH;
	double          d, xi;
	long            i, m = 0;

	*mean = *ss = 0.0;

	for (i = 0; i < n; i++)
	{
		xi = x[i];
		if (!isMissing(xi))
		{
			*mean += xi;
			m++;
		}
	} /*for (i = 0; i < n; i++)*/
#ifdef HASINFINITY
	if (isInfinite(*mean))
	{
		return (-1);
	}
	else
#endif /*HASINFINITY*/
	if (m > 1)
	{
		*mean /= m;
		for (i = 0; i < n; i++)
		{
			xi = x[i];
			if (!isMissing(xi))
			{
			
				d = xi - *mean;
				*ss += d*d;
			} /*if (!isMissing(xi))*/ 
		} /*for (i = 0; i < n; i++)*/
#ifdef HASINFINITY
		if (isInfinite(*ss))
		{
			return (-2);
		}
#endif /*HASINFINITY*/
	} /*if (m > 1)*/
	return (m);
} /*tmeanss()*/

enum   ttestCodes
{
	ITVAL,
	ITINT,
	IT2VAL,
	IT2INT
};


Symbolhandle    tval(Symbolhandle list)
{
	/* one sample t test of zero mean and confidence interval for mean */

	Symbolhandle    arg[2], result = (Symbolhandle) 0;
	double          mean, ss, se, level, t, df;
	int             iop, margs, nargs = NARGS(list);
	int             returnDF = 0;
	char           *keyword = (char *) 0, *compNames[2];
	long            i, n, m;
	WHERE("tint");
	
	*OUTSTR = '\0';
	compNames[0] = "t";
	compNames[1] = "df";
	
	iop = (strcmp(FUNCNAME,"tval") == 0) ? ITVAL : ITINT;
	margs = (iop == ITVAL) ? -2 : 2;

	if (margs > 0 && nargs != margs || margs < 0 && nargs > abs(margs))
	{
		badNargs(FUNCNAME,margs);
		goto errorExit;
	}
	
	for (i=0;i<nargs;i++)
	{
		arg[i] = COMPVALUE(list, i);
	
		if ((iop == ITINT || i == 0) &&
			!argOK(arg[i], REAL, (nargs != 1) ? i + 1 : 0))
		{
			goto errorExit;
		}
	
		if (i == 0 && (!isVector(arg[0]) || (n = symbolSize(arg[0])) < 2))
		{
			sprintf(OUTSTR,
					"ERROR: data for %s() must be a vector of length > 1",
					FUNCNAME);
		}
		else if (i == 1)
		{
			if (iop == ITINT)
			{
				if (!isScalar(arg[1]))
				{
					sprintf(OUTSTR,
							"ERROR: argument 2 for %s() must be a REAL scalar",FUNCNAME);
				}
			} /*if (iop == ITINT)*/
			else
			{
				if ((keyword = isKeyword(arg[1])) && strcmp(keyword,"df") != 0)
				{
					badKeyword(FUNCNAME,keyword);
					goto errorExit;
				}					
				if (!isTorF(arg[1]))
				{
					char        msg[30];
					
					if (keyword)
					{
						strcpy(msg, keyword);
					}
					else
					{
						sprintf(msg,"argument 2 for %s()", FUNCNAME);
					}
					notTorF(msg);
					goto errorExit;
				} /*if (!isTorF(arg[1]))*/

				if (!keyword)
				{
					keyword = "df";
				}
			} /*if (iop == ITINT){}else{}*/
		} /*else if (i == 1)*/
		
		if (*OUTSTR)
		{
			goto errorExit;
		}
	} /*for (i=0;i<nargs;i++)*/
	
	if (iop == ITINT)
	{		
		level = DATAVALUE(arg[1],0);
		if (isMissing(level))
		{
			sprintf(OUTSTR,
					"ERROR: value of argument 2 for %s() is MISSING", FUNCNAME);
		}
		else if (level > .9999999 || level < .0000001)
		{
			sprintf(OUTSTR,
					"ERROR: value of argument 2 for %s() must be between 0 and 1",
					FUNCNAME);
		}
		if (*OUTSTR)
		{
			goto errorExit;
		}
	} /*if (iop == ITINT)*/
	else if (nargs == 2)
	{
		returnDF = (DATAVALUE(arg[1],0) != 0.0);
	}
	
	if (returnDF)
	{
		result = RSInstall(SCRATCH, 2, compNames, 1);
	}
	else
	{
		result = RInstall(SCRATCH, (iop == ITVAL) ? 1 : 2);
	}
	
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	

	m = tmeanss(DATA(arg[0]), n, &mean, &ss);

#ifdef HASINFINITY
	if (m < 0)
	{
		sprintf(OUTSTR,
			"ERROR: too large %s computed by %s()", 
			(m == -1) ? "mean" : "standard deviation", FUNCNAME);
	}
	else
#endif /*HASINFINITY*/
	if (m < 2)
	{
		sprintf(OUTSTR,
				"ERROR: %s non-missing value%s in data for %s()",
				(m == 0) ? "no" : "only 1" , (m == 0) ? "s" : "", FUNCNAME);
	}
	else if (iop == ITVAL && ss == 0.0)
	{
		sprintf(OUTSTR,
				"ERROR: estimated standard deviation of data for %s() is zero",
				FUNCNAME);
	}
	if (*OUTSTR)
	{
		goto errorExit;
	}
	
	if (m < n)
	{
		sprintf(OUTSTR,
				"WARNING: Missing values found in data for %s()",FUNCNAME);
		putOUTSTR();
	}
	
	df = (double) (m - 1);
	se = sqrt((ss / df) / (double) m);
	if (iop == ITVAL)
	{
		t = mean / se;
		if (!returnDF)
		{
			DATAVALUE(result,0) = t;
		}
		else
		{
			DATAVALUE(COMPVALUE(result,0),0) = t;
			DATAVALUE(COMPVALUE(result,1),0) = df;
		}
	} /*if (iop == ITVAL)*/
	else
	{
		t = tquantile(1.0 - 0.5*(1.0 - level), df);
		DATAVALUE(result,0) = mean - t*se;
		DATAVALUE(result,1) = mean + t*se;
	}
	
	return (result);

  errorExit:
	putOUTSTR();

	Removesymbol(result);
	return (0);
} /*tval()*/

/* 
  t2val(x1,x2)  or t2val(x1,x2,df:T) or t2val(x1,x2,pooled:F)
  t2int(x1,x2,level) or t2int(x1,x2,level,pooled:F)

  Note: t2val(x1,x2,F) is equivalent to t2val(x1,x2,pooled:F) and
  t2int(x1,x2,level,F) is equivalent to t2int(x1,x2,level,pooled:F)

  960920 Fixed bug that caused crash if keywords were used
  980406 Fixed bug that prevented keyword 'df' from working right
*/

Symbolhandle    t2val(Symbolhandle list)
{
	/* two sample t test of zero difference and C.I. for difference*/

	Symbolhandle    arg[4], result = (Symbolhandle) 0;
	double          se, mean[2], ss[2], var, level, t, df;
	long            iop, i, n[2], m[2], nn, nargs = NARGS(list);
	long            mnNargs, mxNargs;
	char           *compNames[2], *keyword = (char *) 0;
	char           *keyNames[2];
	int             returnDF = 0, pooled = 1, logvalue = 0;
	WHERE("t2val");
	
	*OUTSTR = '\0';
	compNames[0] = "t";
	keyNames[1] = compNames[1] = "df";
	keyNames[0] = "pooled";
	
	iop = (strcmp(FUNCNAME,"t2val") == 0) ? IT2VAL : IT2INT;

	mnNargs = (iop == IT2VAL) ? 2 : 3;
	mxNargs = mnNargs + 1;

	if (nargs > mxNargs)
	{
		badNargs(FUNCNAME,-mxNargs);
		goto errorExit;
	}
	else if (nargs < mnNargs)
	{
		badNargs(FUNCNAME,-(1000+mnNargs));
		goto errorExit;
	}
	
	for (i=0;i<2;i++)
	{
		if (!argOK(arg[i] = COMPVALUE(list,i),REAL, i+1))
		{
			goto errorExit;
		}
		if (!isVector(arg[i]))
		{
			sprintf(OUTSTR,
					"ERROR: argument %ld to %s() is not a vector",
					i+1, FUNCNAME);
			goto errorExit;
		}			
		else
		{
			n[i] = symbolSize(arg[i]);
		}
	} /*for (i=0;i<2;i++)*/

	if (iop == IT2INT)
	{
		if (!argOK(arg[2] = COMPVALUE(list,2),REAL,3))
		{
			goto errorExit;
		}
		level = DATAVALUE(arg[2],0);
		if (!isScalar(arg[2]) || isMissing(level) ||
			level < .00000001 || level > .99999999)
		{
			sprintf(OUTSTR,
					"ERROR: coverage for %s() is not a scalar between 0 and 1",
					FUNCNAME);
			goto errorExit;
		}
		
		i = 3;
	}
	else
	{
		i = 2;
	}
	if (i < nargs)
	{
		arg[i] = COMPVALUE(list, i);
		if (!isTorF(arg[i]))
		{
			sprintf(OUTSTR,
					"ERROR: argument %ld for %s() must be %s", i+1, FUNCNAME,
					(iop == IT2VAL) ? "df:T or pooled:F" : "pooled:F");
			goto errorExit;
		} /*if (!isTorF(arg[i]))*/

		if ((keyword = isKeyword(arg[i])))
		{
			if (strncmp(keyword,keyNames[0],4) != 0 &&
			   (iop == IT2INT || strcmp(keyword, keyNames[1]) != 0))
			{
				badKeyword(FUNCNAME,keyword);
				goto errorExit;
			}
		}		
		else
		{
			keyword = keyNames[0];
		}
		logvalue = (DATAVALUE(arg[i],0) != 0.0);
	} /*if (i < nargs)*/

	if (keyword)
	{
		if (strncmp(keyword,keyNames[0],4) == 0)
		{
			pooled = logvalue;
			returnDF = (iop == IT2VAL && !pooled);
		}
		else
		{ /* must be "df" */
			returnDF = logvalue;
			pooled = 1;
		}
	} /*if (keyword)*/

	if (returnDF)
	{
		result = RSInstall(SCRATCH, 2, compNames, 1);
	}
	else
	{
		result = RInstall(SCRATCH, (iop == IT2VAL) ? 1 : 2);
	}

	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	nn = 0;
	var = 0.0;
	for (i=0;i<2;i++)
	{
		m[i] = tmeanss(DATA(arg[i]), n[i], mean+i, ss+i);
		if (m[i] < 1)
		{
			sprintf(OUTSTR,
					"ERROR: no non-missing values in argument %ld to %s()",
					i+1,FUNCNAME);
			goto errorExit;
		} /*if (m[i] < 1)*/

		if (m[i] < n[i])
		{
			sprintf(OUTSTR,
					"WARNING: missing values in argument %ld to %s()",
					i+1,FUNCNAME);
			putOUTSTR();
		}
		nn += m[i];
		var += ss[i]; /* pooled SS */
	} /*for (i=0;i<2;i++)*/

	*OUTSTR = '\0';
	if (nn < 3 || !pooled && (m[0] < 2 || m[1] < 2))
	{
		sprintf(OUTSTR,
				"ERROR: too few non-missing values in arguments to %s()",
				FUNCNAME);
	}
	else if (iop == IT2VAL)
	{
		if (ss[0] == 0.0 && ss[1] == 0.0)
		{
			sprintf(OUTSTR,
					"ERROR: estimated standard error of difference for %s() is zero",
					FUNCNAME);
		}
	}

	if (*OUTSTR)
	{
		goto errorExit;
	}

	if (pooled)
	{
		df = (double) (nn - 2);
		var /= df;
		se = sqrt(var * (1.0/(double) m[0] + 1.0 / (double) m[1]));
	}
	else
	{
		ss[0] /= (double) (m[0]*(m[0] - 1));
		ss[1] /= (double) (m[1]*(m[1] - 1));

		var = ss[0] + ss[1];
		se = sqrt(var);
		
		df = var*var/
			(ss[0]*ss[0]/(double)(m[0]-1) + ss[1]*ss[1]/(double)(m[1]-1));
	}
	
	if (iop == IT2VAL)
	{
		t = (mean[0] - mean[1]) / se;
		if (!returnDF)
		{
			DATAVALUE(result,0) = t;
		}
		else
		{
			DATAVALUE(COMPVALUE(result,0),0) = t;
			DATAVALUE(COMPVALUE(result,1),0) = df;
		}
	}
	else
	{
		t = tquantile(1.0 - 0.5*(1.0 - level),df);
		DATAVALUE(result,0) = mean[0] - mean[1] - t*se;
		DATAVALUE(result,1) = mean[0] - mean[1] + t*se;
	}
	*OUTSTR = '\0';
	
	return (result);

  errorExit:
	putOUTSTR();

	Removesymbol(result);
	return (0);
} /*t2val()*/


#ifdef UNDEFINED__
Symbolhandle    tint(Symbolhandle list)
{
	/* one sample t interval */

	Symbolhandle    arg[2], result = (Symbolhandle) 0;
	double          t, mean, ss, se, level;
	long            i, m, n;

	*OUTSTR = '\0';
	
	if (NARGS(list) != 2)
	{
		badNargs(FUNCNAME,2);
		goto errorExit;
	}
	
	for (i=0;i<2;i++)
	{
		if (!argOK(arg[i] = COMPVALUE(list,i),REAL,i+1))
		{
			goto errorExit;
		}
		if (i==0 && (!isVector(arg[0]) || (n = symbolSize(arg[0])) < 2) ||
		   i==1 && !isScalar(arg[1]))
		{
			sprintf(OUTSTR,
					"ERROR: %s argument to %s() must be %s",
					(i==0) ? "1st" : "2nd",FUNCNAME,
					(i==0) ? "vector of length >= 2" : "scalar");
			goto errorExit;
		}
	} /*for (i=0;i<2;i++)*/
	
	level = DATAVALUE(arg[1],0);
	if (isMissing(level))
	{
		sprintf(OUTSTR,"ERROR: level for %s() is MISSING", FUNCNAME);
	}
	else if (level > .9999999 || level < .0000001)
	{
		sprintf(OUTSTR,"ERROR: 2nd argument to %s() must be between 0 and 1",
				FUNCNAME);
	}
	if (*OUTSTR)
	{
		goto errorExit;
	}
	
	t = tquantile(1.0 - 0.5*(1.0 - level), (double) (n - 1));
	m = tmeanss(DATA(arg[0]), n, &mean, &ss);

	if (m < 2)
	{
		sprintf(OUTSTR,"ERROR: %s non-missing values in 1st argument to %s()",
				(m == 0) ? "no" : "only 1", FUNCNAME);
		goto errorExit;
	}
	if (m < n)
	{
		sprintf(OUTSTR,"WARNING: missing values in argument to %s() ignored");
		putOUTSTR();
	}
	
	se = sqrt((ss / (double) (m-1)) / (double) m);

	result = RInstall(SCRATCH,2);
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	DATAVALUE(result,0) = mean - t * se;
	DATAVALUE(result,1) = mean + t * se;

	return (result);

  errorExit:
	putOUTSTR();

	Removesymbol(result);
	return (0);
} /*tint()*/

Symbolhandle    t2int(Symbolhandle list)
{
	/* two sample t interval */

	Symbolhandle    arg[3], result = (Symbolhandle) 0;
	double          t, level, mean[2], ss, se, df, var;
	long            i, m[2], n[2], nn;

	*OUTSTR = '\0';
	
	if (NARGS(list) != 3)
	{
		badNargs(FUNCNAME,3);
		goto errorExit;
	}
	
	for (i=0;i<3;i++)
	{
		if (!argOK(arg[i] = COMPVALUE(list,i),REAL,i+1))
		{
			goto errorExit;
		}
		if (i < 2 && !isVector(arg[i]) ||  i == 2 && !isScalar(arg[2]))
		{
			sprintf(OUTSTR,
					"ERROR: argument %ld to %s() is not %s", i+1, FUNCNAME,
					(i < 2) ? "vector" : "scalar");
			goto errorExit;
		}
		if (i < 2)
		{
			n[i] = symbolSize(arg[i]);
		}
	}

	level = DATAVALUE(arg[2],0);
	if (isMissing(level))
	{
		sprintf(OUTSTR,"ERROR: value of argument 3 for %s() is MISSING",
				FUNCNAME);
	}
	else if (level > .9999999 || level < .0000001)
	{
		sprintf(OUTSTR,
				"ERROR: value of argument 3 to %s() must be between 0 and 1",
				FUNCNAME);
	}
	if (*OUTSTR)
	{
		goto errorExit;
	}
	
	result = RInstall(SCRATCH,2);
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	nn = 0;
	var = 0.0;
	for (i=0;i<2;i++)
	{
		m[i] = tmeanss(DATA(arg[i]), n[i], mean+i, &ss);
		if (m[i] < 1)
		{
			sprintf(OUTSTR,
					"ERROR: no non-missing values in argument %ld to %s()",
					i+1,FUNCNAME);
			goto errorExit;
		}
		if (m[i] < n[i])
		{
			sprintf(OUTSTR,
					"WARNING: missing values in argument %ld to %s()",
					i+1,FUNCNAME);
			putOUTSTR();
		}
		nn += m[i];
		var += ss;
	}

	if (nn < 3)
	{
		sprintf(OUTSTR,
				"ERROR: too few non missing values in arguments to %s()",
				FUNCNAME);
	}
	else if (var == 0.0)
	{
		sprintf(OUTSTR,"ERROR: pooled variance for %s() is zero",FUNCNAME);
		goto errorExit;
	}

	df = (double) (nn - 2);

	t = tquantile(1.0 - 0.5*(1.0 - level),df);
	var /= df;
	se = sqrt(var * (1.0/(double) m[0] + 1.0/(double) m[1]));
	DATAVALUE(result,0) = (mean[0] - mean[1]) - t * se;
	DATAVALUE(result,1) = (mean[0] - mean[1]) + t * se;

	return (result);

  errorExit:
	putOUTSTR();
	Removesymbol(result);
	
	return (0);
} /*t2int()*/
#endif /*UNDEFINED__*/
