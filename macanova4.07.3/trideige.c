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
#pragma segment Trideige
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

/*
  usage:
	trieigen(diag,subdiag,[,values:F], [vectors:F])
	trieigen(diag,subdiag, end [,values:F], [vectors:F])
	trieigen(diag,subdiag,start,end,[,values:F], [vectors:F])
	where defaults for start and end are 1 and length(diag)
  Written by C. Bingham
  Version of 950524
  960903 Changed checking of some keywords to use isInteger()
  960911 Fixed bug introduced by preceding change.
  981007 Minor changes to error messages
  990226 Replaced putOUTSTR() by putErrorOUTSTR()
*/
#define TRIEIGVALUES  0x01
#define TRIEIGVECTORS 0x02

enum trideigscratch
{
	GSCRATCH = 0,
	GIND,
	NTRASH
};

enum trideigkeys
{
	KVALUES = 0,
	KVECTORS,
	NKEYS
};
static char     *LegalKeys[NKEYS] =
{
	"values",
	"vectors",
};


/*
  980303 Added check for MISSING values in the arguments 1 and 2
  980316 Added check for NaN in result
*/
Symbolhandle trideigen(Symbolhandle list)
{
	Symbolhandle          symhDiag, symhSubdiag;
	Symbolhandle          result = (Symbolhandle) 0;
	Symbolhandle          symhVals = (Symbolhandle) 0;
	Symbolhandle          symhVecs = (Symbolhandle) 0;
	Symbolhandle          symh, symhKey;
	double               *e2 = (double *) 0;
	double               *vecj, *veck;
	double              **scratchH = (double **) 0;
	double               *scratch = (double *) 0;
	double               *rv1, *rv2, *rv3, *rv4, *rv5, *rv6;
	double               *subdiag, *values;
	double                value, eps1 = 0.0, lb, ub;
	long                  ierr, m11;
	long                **indH = (long **) 0;
	long                  nargs = NARGS(list), keyStart;
	long                  i, j, k, iarg, ikey;
	long                  n, n1, m, halfm, startvec, endvec;
	long                  needed;
	long                  op = TRIEIGVALUES | TRIEIGVECTORS;
	long                  ncomps;
	int                   logValue;
	char                 *keyword;
	WHERE("trieigen");
	TRASH(NTRASH, errorExit);

	if (nargs < 2)
	{
		badNargs(FUNCNAME, -1003);
		goto errorExit;
	}

	iarg = 0;
	symhDiag = COMPVALUE(list, iarg++);
	if (!argOK(symhDiag, 0, iarg))
	{
		goto errorExit;
	}
	if (TYPE(symhDiag) != REAL || ! isVector(symhDiag))
	{
		sprintf(OUTSTR,
				"ERROR: argument %ld (diagonal) to %s() must be REAL vector",
				iarg, FUNCNAME);
		goto errorExit;
	}
	n = symbolSize(symhDiag);

	symhSubdiag = COMPVALUE(list, iarg++);
	if (!argOK(symhSubdiag, 0, iarg))
	{
		goto errorExit;
	}
	if (TYPE(symhSubdiag) != REAL || ! isVector(symhSubdiag))
	{
		sprintf(OUTSTR,
				"ERROR: argument %ld (subdiagonal) to %s() must be REAL vector",
				iarg, FUNCNAME);
		goto errorExit;
	}

	if (anyMissing(symhDiag) || anyMissing(symhSubdiag))
	{
		sprintf(OUTSTR,
				"ERROR: MISSING value in argument %ld to %s()",
				(anyMissing(symhDiag)) ? 1L : 2L, FUNCNAME);
		goto errorExit;
	} /*if (anyMissing(symhDiag)  || anyMissing(symhSubdiag))*/
	
	n1 = symbolSize(symhSubdiag);
	if (n1 != n && n1 != n - 1)
	{
		sprintf(OUTSTR,
				"ERROR: length(subdiag) != length(diag) and != length(diag) - 1");
		goto errorExit;
	} /*if (n1 != n && n1 != n - 1)*/

	for (keyStart = iarg;
		keyStart < nargs && !isKeyword(COMPVALUE(list, keyStart)); keyStart++)
	{ /* find start of keywords */
		;
	}

	if (keyStart > iarg + 2)
	{
		sprintf(OUTSTR,
			   "ERROR: too many non-keyword arguments to %s()", FUNCNAME);
		goto errorExit;
	}
	if (nargs - keyStart > 2)
	{
		sprintf(OUTSTR,
				"ERROR: too many arguments to %s()", FUNCNAME);
		goto errorExit;
	} /*if (nargs > iarg + 2)*/
	
/* set default values for starting and ending eigenvalues/vectors*/
	startvec = 0;
	endvec = n - 1;

	iarg = keyStart - 1;
	if (iarg >= 2)
	{ /*get ending index*/
		symh = COMPVALUE(list,iarg);
		if (!argOK(symh, 0, iarg+1))
		{
			goto errorExit;
		}
		if (!isInteger(symh, POSITIVEVALUE))
		{
			sprintf(OUTSTR,
					"ERROR: argument %ld to %s() not positive integer",
					iarg+1, FUNCNAME);
			goto errorExit;
		}
		endvec = (long) DATAVALUE(symh,0) - 1;
		iarg--;
	} /*if (iarg >= 2)*/

	if (iarg >= 2)
	{ /* get starting index */
		symh = COMPVALUE(list, iarg);
		if (!isInteger(symh, POSITIVEVALUE))
		{
			sprintf(OUTSTR,
					"ERROR: argument %ld to %s() not positive integer",
					iarg+1, FUNCNAME);
			goto errorExit;
		}
		startvec = (long) DATAVALUE(symh,0) - 1;
	} /*if (iarg >= 2)*/

	if (startvec > endvec || endvec > n - 1)
	{
		sprintf(OUTSTR,
				"ERROR: improper specification of eigenvalues/vectors to be computed");
		goto errorExit;
	}
	m = endvec - startvec + 1; /* number of eigenthings to compute*/

	/* check keywords*/
	for (iarg = keyStart; iarg < nargs; iarg++)
	{
		symhKey = COMPVALUE(list, iarg);
		if (!(keyword = isKeyword(symhKey)))
		{
			sprintf(OUTSTR,
					"ERROR: argument %ld of %s() expected to be %s:F or %s:F",
					iarg - 1, FUNCNAME,
					LegalKeys[KVALUES], LegalKeys[KVECTORS]);
			goto errorExit;
		}
		for (ikey = 0; ikey < NKEYS; ikey++)
		{
			if (strncmp(keyword, LegalKeys[ikey], 3) == 0)
			{
				break;
			}
		} /*for (ikey = 0; ikey < NKEYS; ikey++)*/
		if (ikey == NKEYS)
		{
			badKeyword(FUNCNAME, keyword);
			goto errorExit;
		}
		
		if (!isTorF(symhKey))
		{
			notTorF(keyword);
			goto errorExit;
		}
		logValue = (DATAVALUE(symhKey, 0) != 0.0);
		if (ikey == KVALUES)
		{
			if (!logValue)
			{
				op &= ~TRIEIGVALUES;
			}
			else
			{
				op |= TRIEIGVALUES;
			}
		} /*if (ikey == KVALUES)*/
		else if (ikey == KVECTORS)
		{
			if (!logValue)
			{
				op &= ~TRIEIGVECTORS;
			}
			else
			{
				op |= TRIEIGVECTORS;
			}
		}
	} /*for (iarg = keyStart; iarg < nargs; iarg++)*/

	if (op == 0)
	{
		sprintf(OUTSTR,
				"ERROR: neither eigenvalues or eigenvectors requested from %s()",
				FUNCNAME);
		goto errorExit;
	} /*if (op == 0)*/
	
/* compute scratch space needed */
	needed = (op & TRIEIGVECTORS) ? 6 * n : 3 * n;
	
	if (!(op & TRIEIGVALUES))
	{
		needed += m;
	}

	if (!getScratch(scratchH, GSCRATCH, needed, double) ||
		!getScratch(indH, GIND, m, long))
	{
		goto errorExit;
	}

	ncomps = 0;
	if (op & TRIEIGVALUES)
	{
		ncomps++;
		if((symhVals = RInstall(SCRATCH, m)) == (Symbolhandle) 0)
		{
			goto errorExit;
		}
	} /*if (op & TRIEIGVALUES)*/

	if (op & TRIEIGVECTORS)
	{
		ncomps++;
		if((symhVecs = RInstall(SCRATCH, m*n)) == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		setNDIMS(symhVecs, 2);
		setDIM(symhVecs,1,n);
		setDIM(symhVecs,2,m);
	} /*if (op & TRIEIGVECTORS)*/
	
	if (ncomps == 2)
	{
		result = StrucInstall(SCRATCH, 2);
		if (result == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		Cutsymbol(symhVals);
		COMPVALUE(result, 0) = symhVals;
		setNAME(symhVals, "values");
		Cutsymbol(symhVecs);
		COMPVALUE(result, 1) = symhVecs;
		setNAME(symhVecs, "vectors");
	} /*if (ncomps == 2)*/
	else
	{
		result = (op & TRIEIGVALUES) ? symhVals : symhVecs;
	} /*if (ncomps == 2){}else{}*/
/*
   It should now be safe to dereference handles
*/
	subdiag = (n1 == n) ? DATAPTR(symhSubdiag) : DATAPTR(symhSubdiag) - 1;
	
	scratch = *scratchH;
	if (!(op & TRIEIGVALUES))
	{
		values = scratch;
		scratch += m;
	}
	else
	{
		values = DATAPTR(symhVals);
	}
	
	e2 = scratch;
	scratch += n;
	rv4 = scratch;
	scratch += n;
	rv5 = rv6 = scratch;
	scratch += n;

	if (op & TRIEIGVECTORS)
	{
		rv1 = scratch;
		scratch += n;
		rv2 = scratch;
		scratch += n;
		rv3 = scratch;
		scratch += n;
	}

	for (i=1; i < n; i++)
	{
		e2[i] = subdiag[i]*subdiag[i];
	}
	
	m11 = n - m - startvec;
	halfm = m/2;

	tridib(n, &eps1, DATAPTR(symhDiag), subdiag, e2, &lb, &ub,
		   m11, m, values, *indH, &ierr, rv4, rv5);
	checkInterrupt(errorExit);

	if (ierr)
	{
		sprintf(OUTSTR,
				"ERROR: %s() cannot find eigenvalues; multiple roots?",
				FUNCNAME);
		goto errorExit;
	} /*if (ierr)*/

#ifdef HASNAN
	for (j = 0; j < m; j++)
	{
		if (isNaN(values[j]))
		{
			goto overflowExit;
		} /*if (isNaN(values[j]))*/
	} /*for (j = 0; j < m; j++)*/
#endif /*HASNAN*/

	if (op & TRIEIGVECTORS)
	{
		if (interrupted(DEFAULTTICKS))
		{
			goto errorExit;
		}
		vecj = DATAPTR(symhVecs);
		tinvit(n, n, DATAPTR(symhDiag), subdiag, e2, m, values,
			   *indH, vecj, &ierr, rv1, rv2, rv3, rv4, rv6);
		checkInterrupt(errorExit);
		if (ierr)
		{
			sprintf(OUTSTR,
					"ERROR: problem computing %ld%s requested eigenvector",
					m + ierr + 1, n_th(m + ierr + 1));
			goto errorExit;
		} /*if (ierr)*/

#ifdef HASNAN
		if (anyNaN(symhVecs))
		{
			goto overflowExit;
		}
#endif /*HASNAN*/
#if (USENOMISSING)
		setNOMISSING(symhVecs);
#endif /*USENOMISSING*/
		/* reorder eigenvectors*/
		veck = vecj + (m - 1) * n;
		for (j = 0;j < halfm; j++)
		{
			for (i = 0; i < n; i ++)
			{
				value = vecj[i];
				vecj[i] = veck[i];
				veck[i] = value;
			}
			vecj += n;
			veck -= n;
		} /*for (j = 0;j < halfm; j++)*/
	} /*if (op & TRIEIGVECTORS)*/
	/* reorder eigenvalues*/
	if (op & TRIEIGVALUES)
	{
		k = m - 1;
		for (j = 0; j < halfm; j++, k--)
		{
			value = values[j];
			values[j] = values[k];
			values[k] = value;
		} /*for (j = 0; j < halfm; j++)*/
#if (USENOMISSING)
		setNOMISSING(symhVals);
#endif /*USENOMISSING*/
	} /*if (op & TRIEIGVALUES)*/
		
	emptyTrash();
	UNLOADSEG(tridib);
	
	return (result);

#ifdef HASNAN
  overflowExit:
	sprintf(OUTSTR,
			"ERROR: overflow in in %s() computations", FUNCNAME);
	/* fall through*/
#endif /*HASNAN*/

  errorExit:
	putErrorOUTSTR();
	emptyTrash();
	UNLOADSEG(tridib);
	Removesymbol(result);
	return (0);
} /*trideigen()*/
	
