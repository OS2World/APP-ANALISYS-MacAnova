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
#pragma segment MakeFact
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

enum makefactorOpCodes
{
	IMATCH = 1,
	IUNIQUE
};

/*
	factor(arg)
		If arg legal for factor (all posInts) return factor equivalent
		If arg already a factor, the number of levels is recomputed from
		the actual values in arg, not taken from NCLASS(arg).
	match(arg,vals [,noMatch])
		result[i] = (arg[i] == vals[j]) ? j+1 : noMatch
		arg and vals must be both REAL or both CHAR
		vals must contain no missing values
		The default value for noMatch is length(vals) + 1
	unique(arg)
		Produces vector of unique values in arg. It is an error for arg to
		be all MISSING.  arg must be REAL or CHAR
	unique(arg,index:T) produces vector J of integers such that arg[J] is
	    the same as unique(arg)
*/

/*
	961013 Fixed bug affecting case of 1 logical scratch argument

	980420 added keyword 'index' to unique().  index:T means the
    indices of the unique elements are returned instead of the values
    so that x[unique(x,index:T)] and unique(x) are identical.

	980730 added new argument to reuseArg() so that notes will be kept.

    990226 match() and unique() now work with logical arguments
           changed most putOUTSTR() to putErrorOUTSTR()
*/

static Symbolhandle  doFactor(Symbolhandle list)
{
	Symbolhandle       result = (Symbolhandle) 0;
	Symbolhandle       symh;
	double            *in, *out;
	double             val;
	long               nargs = NARGS(list);
	long               type, type0;
	long               i, j, k, n, needed = 0,levels, nclasses = -1;
	WHERE("doFactor");
	
	*OUTSTR = '\0';

	for (i = 0; i < nargs; i++)
	{
		symh = COMPVALUE(list,i);
		if (!argOK(symh, 0, (nargs > 1) ? i + 1 : 0))
		{
			goto errorExit;
		}
		type = TYPE(symh);
		if (type != REAL && type != LOGIC)
		{
			badType(FUNCNAME,-type, (nargs > 1) ? i+1 : 0);
			goto errorExit;
		}
		if (!isVector(symh))
		{
			sprintf(OUTSTR,
					"ERROR: argument %ld to %s() is not a vector %s",i+1,
					FUNCNAME, (nargs == 1) ? "" : " or scalar");
		}
		else if (i == 0)
		{
			type0 = type;
		}
		else if (type != type0)
		{
			sprintf(OUTSTR,
					"ERROR: type of argument %ld to %s() not the same as argument 1",
					i + 1, FUNCNAME);
		}
		if (*OUTSTR)
		{
			goto errorExit;
		}
	} /*for (i = 0; i < nargs; i++)*/

	
	for (i = 0; i < nargs; i++)
	{
		symh = COMPVALUE(list, i);

		n = symbolSize(symh);
		in = DATAPTR(symh);
		for (j = 0; j < n; j++)
		{
			if (!isMissing(in[j]))
			{
				break;
			}
		} /*for (j = 0; j < n; j++)*/

		if (j < n)
		{ /* found at least 1 non-missing value */
			levels = labs(isFactor(symh));
			if (levels == 0)
			{
				sprintf(OUTSTR,
				"ERROR: argument for %s() with non-integers or nonpositive integers",
					FUNCNAME);
				goto errorExit;
			}
			
			if (levels > nclasses)
			{
				nclasses = levels;
			}
		} /*if (j < n)*/
		needed += n;
	} /*for (i = 0; i < nargs; i++)*/

	if (nclasses < 0)
	{
		sprintf(OUTSTR,
				"ERROR: no non-MISSING values in arguments to %s()",
				FUNCNAME);
	}
	else if (nclasses > MAXFACTORLEVELS)
	{
		sprintf(OUTSTR,
				"ERROR: maximum factor level > %ld is too big",
				(long) MAXFACTORLEVELS);
	}
	if (*OUTSTR)
	{
		goto errorExit;
	}
	
	if (nargs == 1)
	{
		if (isscratch(NAME(symh)))
		{/* reuse argument */
			result = reuseArg(list, 0, 1, 1);
		}
		else
		{
			result = Install(SCRATCH, REAL);
			if (result == (Symbolhandle) 0 || !Copy(symh, result))
			{
				goto errorExit;
			}
		}				
		setNAME(result, SCRATCH);
	} /*if (nargs == 1)*/
	else
	{
		result = RInstall(SCRATCH, needed);
		if (result == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		setNDIMS(result, 1);
		setDIM(result,1,needed);
	} /*if (nargs == 1){}else{}*/

	setTYPE(result, REAL);
	setNCLASS(result, nclasses);

	if (type == LOGIC || nargs > 1)
	{
		out = DATAPTR(result);
		k = 0;
		for (i = 0; i < nargs; i++)
		{
			symh = (i > 0 || COMPVALUE(list, 0) != (Symbolhandle) 0) ?
					 COMPVALUE(list, i) : result;
			n = symbolSize(symh);
			in = DATAPTR(symh);

			for (j = 0; j < n; j++)
			{
				val = in[j];
				if (isMissing(val))
				{
					setMissing(out[k]);
				}
				else
				{
					if (type == LOGIC)
					{
						val = (val != 0.0) ? 2.0 : 1.0;
					}
					out[k] = val;
				}
				k++;
			} /*for (j = 0; j < n; j++)*/
		} /*for (i = 0;i < n; i++)*/
	} /*if (type == LOGIC || nargs > 1 || !isscratch(NAME(symH)))*/

	return (result);

  errorExit:
	putErrorOUTSTR();
	return (0);
	
} /*doFactor()*/

/*
  usages:
    factor(l1, l2, ...) where l1, l2, ... are either all REAL vectors 
	of positive integers or all LOGICAL vectors.

	match(x,vec [,nomatch] [,exact:F]), x REAL or CHARACTER, vec a
	vector of the same type as x and nomatch a REAL scalar

	unique(x [,index:T]), x REAL or CHARACTER

	980730 added new argument to reuseArg() so that notes will not be kept.
*/
Symbolhandle    makefactor(Symbolhandle list)
{

	long            i, j, n, m, op;
	long            notMatched = 0, quiet = 0;
	long            nargs = NARGS(list), margs;
	long            type, cplace = 0;
	long            chOutSize = 0, chLength;
	Symbolhandle    args[3], result = (Symbolhandle) 0;
	double          val = 0.0, noMatch;
	double        **outH;
	double         *in, *out, *values;
	char          **chInH, **chOutH = (char **) 0, **chValuesH = (char **) 0;
	char           *chVal, *chVal1;
	char           *exactKey = "exact";
	int             compIndex = 0, exact = -1;
	WHERE("makefactor");

	*OUTSTR = '\0';
	if (strcmp(FUNCNAME,"factor") == 0)
	{
		return (doFactor(list));
	}

	if (strcmp(FUNCNAME,"match") == 0)
	{
		op = IMATCH;
		margs = -4;
	}
	else
	{
		op = IUNIQUE;
		margs = -2;
	}
		
	if (margs > 0 && nargs != margs || margs < 0 && nargs > labs(margs))
	{
		badNargs(FUNCNAME,margs);
		goto errorExit;
	}

	if (op == IMATCH)
	{
		char       *keyword;
		
		if (nargs < 2)
		{
			badNargs(FUNCNAME,-(1000+2));
			goto errorExit;
		}
		if (nargs > 2)
		{
			Symbolhandle     symhKey = COMPVALUE(list, nargs - 1);
			
			if ((keyword = isKeyword(symhKey)))
			{
				if (strcmp(keyword, exactKey) != 0)
				{
					badKeyword(FUNCNAME, keyword);
					goto errorExit;
				}
				if (!isTorF(symhKey))
				{
					notTorF(keyword);
					goto errorExit;
				}
				exact = (DATAVALUE(symhKey,0) != 0.0);
				nargs--;
			}
			else if (nargs > 3)
			{
				sprintf(OUTSTR,
						"ERROR: argument 4 to %s() must be '%s:F'",
						FUNCNAME, exactKey);
				goto errorExit;
			}
		} /*if (nargs > 2)*/
	} /*if (op == IMATCH)*/		
	
	for (i = 0; i < nargs;i++)
	{
		args[i] = COMPVALUE(list,i);
		if (!argOK(args[i],0,(nargs > 1) ? i+1 : 0))
		{
			goto errorExit;
		}
	} /*for (i = 0; i < nargs;i++)*/

	type = TYPE(args[0]);

	if (type != REAL && type != CHAR && type != LOGIC)
	{
		badType(FUNCNAME, -type, (op == IMATCH) ? 1 : 0);
		goto errorExit;
	}

	if (op == IMATCH)
	{
		if (TYPE(args[1]) != type)
		{
			sprintf(OUTSTR,
					"ERROR: %s arguments to %s() must have same type",
					(nargs == 2) ? "both" : "first two", FUNCNAME);
		}			
		else if (nargs == 3)
		{ /* value to return on no match */
			quiet = 1;
			if (TYPE(args[2]) != REAL || !isScalar(args[2]))
			{
				notNumberOrReal("match() argument 3");
				goto errorExit;
			}
			else
			{
				noMatch = DATAVALUE(args[2],0);
			}
		}
		else if (type != CHAR && anyMissing(args[1]))
		{
			sprintf(OUTSTR,
					"ERROR: argument 2 to %s() contains MISSING values",
					FUNCNAME);
		}
		else if (type != CHAR && !exact)
		{
			sprintf(OUTSTR,
					"ERROR: %s:F illegal on %s() with %s arguments",
					exactKey, FUNCNAME, typeName(type));
		}
		if (*OUTSTR)
		{
			goto errorExit;
		}
		exact = (exact < 0) ? 1 : exact;
	} /*if (op == IMATCH)*/
	else if (nargs > 1)
	{ /* must be unique()*/
		char         *keyword = isKeyword(args[1]);
		
		if (!keyword || strcmp(keyword, "index") != 0 ||
			!isTorF(args[1]))
		{
			sprintf(OUTSTR,
					"ERROR: argument 2 to %s() must be 'index:T'",
					FUNCNAME);
			goto errorExit;
		}
		compIndex = (DATAVALUE(args[1],0) != 0.0);
	}
	
	n = symbolSize(args[0]);

	if ((op == IMATCH && type == REAL) && isscratch(NAME(args[0])))
	{ /* reuse argument */
		result = reuseArg(list, 0, 0, 0);
		setNCLASS(result, -1);
	}
	else if (type != CHAR || compIndex || op != IUNIQUE)
	{
		result = RInstall(SCRATCH,n);
	}
	else
	{
		result = Install(SCRATCH,CHAR);
	}
	
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	else if (type == LOGIC && op == IUNIQUE && !compIndex)
	{
		setTYPE(result, LOGIC);
	}
	
	if (type != CHAR)
	{
		in = DATAPTR(args[0]);
	}
	else
	{
		chInH = STRING(args[0]);
	}
	if (TYPE(result) != CHAR)
	{
		outH = DATA(result);
		out = *outH;
	}
		
	if (op == IMATCH)
	{
		long       patternTooLong = 0;

		m = symbolSize(args[1]);
		if (!quiet)
		{
			noMatch = (double) (m+1);
		}
		
		if (type != CHAR)
		{
			values = DATAPTR(args[1]);
		}
		else
		{
			chValuesH = STRING(args[1]);
			chVal = *chInH;
		}
		
		for (i = 0; i < n;i++)
		{
			if (type != CHAR)
			{
				val = in[i];
				if (!isMissing(val))
				{
					for (j = 0;j < m;j++)
					{
						if (type == REAL && val == values[j] ||
							type == LOGIC &&
							(val != 0.0 && values[j] != 0.0 ||
							 val == 0.0 && values[j] == 0.0))
						{
							break;
						}
					} /*for (j=0;j<m;j++)*/
				} /*if (!isMissing(val))*/
				else
				{
					j = -1;
				}
			} /*if (type != CHAR)*/
			else
			{ /*CHAR*/
#define MAXPATLENGTH 500
				long      matchType = EXACTMATCH;
				char      pattern[MAXPATLENGTH + 1];

				chVal1 = *chValuesH; 
				if (!exact && !scanPat(chVal, &matchType, pattern,
									   noNameCheck, MAXPATLENGTH) &&
					matchType & PATTERNTOOLONG)
				{
					patternTooLong++;
					j = m;
				}
				else
				{
					for (j = 0;j < m;j++)
					{
						if (matchType == EXACTMATCH)
						{
							if (strcmp(chVal1,chVal) == 0)
							{
								break;
							}
						}
						else if (matchName(chVal1, matchType, pattern))
						{
							break;
						}
						chVal1 = skipStrings(chVal1, 1);
					} /*for (j=0;j<m;j++)*/
				}
				chVal = skipStrings(chVal, 1);
			} /*if (type != CHAR){}else{}*/

			if (j == m)
			{ /* not matched */
				notMatched++;
				out[i] = noMatch;
			}
			else if (j >= 0)
			{ /* matched non missing*/
				out[i] = (double) (j+1);
			}
			else
			{
				setMissing(out[i]);
			}
		} /*for (i = 0; i< n;i++)*/

		if (notMatched != 0 && !quiet)
		{
			if (patternTooLong)
			{
				sprintf(OUTSTR,
						"WARNING: %ld patterns were too long to be matched",
						patternTooLong);
				putOUTSTR();
			}
			sprintf(OUTSTR,"WARNING: %ld values not matched coded as %ld",
					notMatched, m+1);
			putErrorOUTSTR();
		}
		if (result != args[0])
		{
			setNDIMS(result,NDIMS(args[0]));
			for (i=1;i<=NDIMS(args[0]);i++)
			{
				setDIM(result,i,DIMVAL(args[0],i));
			} /*for (i=1;i<=NDIMS(args[0]);i++)*/
		}		
	} /*if (op == IMATCH)*/
	else
	{ /* find unique non-missing values */
		m = 0;
		for (i = 0; i < n;i++)
		{
			if (type != CHAR)
			{
				val = in[i];
				if (!isMissing(val))
				{
					if (!compIndex)
					{
						for (j=0;j<m;j++)
						{
							if (val == out[j])
							{
								break;
							}
						} /*for (j=0;j<m;j++)*/
					} /*if (!compIndex)*/
					else
					{
						for (j = 0; j < m;j++)
						{
							if (val == in[(long) out[j] - 1])
							{
								break;
							}
						} /*for (j=0;j<m;j++)*/
						val = (double) (i + 1);
					}
					if (j == m)
					{
						out[m++] = val;
					}
				} /*if (!isMissing(val))*/
			} /*if (type != CHAR)*/
			else
			{ /* type == CHAR */
				chVal = *chInH + cplace;
				chLength = strlen(chVal) + 1;

				if (i == 0)
				{
					if (!compIndex)
					{
						chOutSize = chLength;
						chOutH = mygethandle(chOutSize);
						setSTRING(result,chOutH);
						if (chOutH == (char **) 0)
						{
							goto errorExit;
						}
						strcpy(*chOutH,*chInH);
					} /*if (!compIndex)*/
					else
					{
						out[0] = 1.0;
					}
					m = 1;
				} /*if (i == 0)*/
				else
				{
					if (!compIndex)
					{
						chVal1 = *chOutH;
						for (j = 0; j < m;j++)
						{
							if (strcmp(chVal,chVal1) == 0)
							{
								break;
							}
							chVal1 = skipStrings(chVal1, 1);
						} /*for (j = 0; j < m;j++)*/

						if (j == m)
						{/* not matched */
							chOutH = mygrowhandle(chOutH,chOutSize + chLength);
							setSTRING(result,chOutH);
							if (chOutH == (char **) 0)
							{
								goto errorExit;
							}
							strcpy(*chOutH + chOutSize, *chInH + cplace);
							chOutSize += chLength;
							m++;
						} /*if (j == m)*/
					} /*if (!compIndex)*/
					else
					{
						char    *cval = *chInH;
						
						for (j = 0; j < m; j++)
						{
							if (strcmp(chVal, cval) == 0)
							{
								break;
							}
							if (j < m - 1)
							{
								cval = skipStrings(cval,
												   (long) (out[j+1] - out[j]));
							}
						} /*for (j = 0; j < m; j++)*/
						if (j == m)
						{
							out[m++] = (double) (i + 1);
						}
					} /*if (!compIndex){}else{}*/
				} /*if (i == 0){}else{}*/
				cplace += chLength;
			} /*if (type != CHAR){}else{}*/
		} /*for (i=0;i<n;i++)*/

		if (m == 0)
		{
			sprintf(OUTSTR,
					"ERROR: all values in argument to %s() are MISSING",
					FUNCNAME);
			goto errorExit;
		}

		if (TYPE(result) != CHAR && m < n)
		{ /* resize outH */
			outH = (double **) mygrowhandle((char **) outH, m * sizeof(double));
			setDATA(result,outH);
			if (outH == (double **) 0)
			{
				goto errorExit;
			}
		} /*if (TYPE(result) != CHAR && m < n)*/
		setNDIMS(result,1);
		setDIM(result,1,m);
	}  /*if (op == IMATCH){}else{}*/

	return (result);

  errorExit:
	putErrorOUTSTR();
	Removesymbol(result);

	return (0);
} /*makefactor()*/

