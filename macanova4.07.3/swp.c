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
#pragma segment Swp
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

enum swpScratch
{
	GCOLSH = 0,
	GDIAGSH,
	NTRASH
};

#define Trash NAMEFORTRASH

static Symbolhandle   Trash;

/*
   951101  doSwp() moved to mathutils.c
   960605 Redid bcprd() so that 
      bcprd(x1,x2,...,xk) is equivalent to bcprd(hconcat(x1,..,xk))
   971126 Modified to use append codes in labels.h
   981007 Minor changes to error messages
   981227 Keyword phrase quiet:T added to swp(), cleaned fossil
          code from swpIt()
   981229 Keyword phrase keepswept:T added to swp()
*/

#define FULLSWP     1
#define MAXDIAG    40   /* if nswp > MAXDIAG, get scratch space */
#define SINGULAR    0
#define NONSINGULAR 1

/*
  981228 added quiet, keepswept and diagsH to argument list
         deleted obsolete code
*/
static long swpIt(double **yH, double **colsH, double **diagsH,
				  long nRows, long nCols, long nswp, int quiet, int keepswept)
{
	double          *diags = *diagsH, *y = *yH, *cols = *colsH;
	long             i, iswp, minRowCol = (nRows < nCols) ? nRows : nCols;
	long             retValue = 0;
	WHERE("swpIt");
	
	*OUTSTR = '\0';
	
	if (isMissing(diags[0]))
	{
		for (i = 0;i < minRowCol;i++)
		{ /* save diagonals */
			diags[i] = y[i*(nRows+1)];
		}
	}

	for (i = 0;i < nswp;i++)
	{
		iswp = (long) cols[i] - 1;
		if (doSwp(*yH, nRows, nCols, iswp, diags[iswp], FULLSWP) == SINGULAR)
		{
			if (keepswept)
			{
				cols[i] = -cols[i];
			}
			if(!quiet)
			{
				sprintf(OUTSTR, 
						"WARNING: tolerance failure pivoting column %ld; not swept",
						iswp+1);
				putErrorOUTSTR();
			}
		} /*if (doSwp(*yH, nRows, nCols, iswp, diags[iswp], FULLSWP) == SINGULAR)*/
	} /*for (i = 0;i < nswp;i++)*/
	retValue = 1;

/* fall through */

  errorExit:
	putErrorOUTSTR();
	return (retValue);
} /*swpIt()*/

static Symbolhandle bcprd(Symbolhandle list)
{
	Symbolhandle      symh, result = (Symbolhandle) 0;
	Symbolhandle      argi, argj;
	double           *xbar, *xi, *xj;
	double           *cp, *cpij;
	double            meani, meanj;
	long              nargs = NARGS(list);
	long              ncolsi, ncolsj;
	long              totCols = 0;
	long              nrows, n2;
	long              dims[2];
	long              iarg, jarg, ivar, jvar, kvari, kvarj;
	long              ki, kj;
	long              i;
	long              jmin;
	long              lengths[2], lengthsi[2];
	int               foundOverflow = 0;
	char             *labs[2], *constant = "CONSTANT";
	WHERE("bcprd");
	
	lengths[0] = strlen(constant) + 1;
	lengths[1] = 0;
	*OUTSTR = '\0';
	
	for (iarg = 0; iarg < nargs; iarg++)
	{
		symh = COMPVALUE(list, iarg);
		if (!argOK(symh, 0, (nargs > 1) ? iarg + 1 : 0))
		{
			goto errorExit;
		}
		if (TYPE(symh) != REAL || !isMatrix(symh, dims))
		{
			sprintf(OUTSTR,
					"ERROR: argument %ld to %s() is not a REAL matrix",
					iarg + 1, FUNCNAME);
			goto errorExit;
		}
		if (anyMissing(symh))
		{
			sprintf(OUTSTR,
					"ERROR: argument %ld to %s() contains MISSING values",
					iarg + 1, FUNCNAME);
			goto errorExit;
		}
		
		if (iarg == 0)
		{
			nrows = dims[0];
		}
		else if (dims[0] != nrows)
		{
			sprintf(OUTSTR,
					"ERROR: all arguments to %s() must have the same number of rows",
					FUNCNAME);
			goto errorExit;
		}
		totCols += dims[1];
		if (lengths[0] >= 0)
		{
			if (HASLABELS(symh))
			{
				getMatLabels(symh, labs, lengthsi);
				lengths[0] += lengthsi[1];
			} /*if (HASLABELS(symh))*/
			else
			{
				lengths[0] = -1;
			}
		} /*if (lengths[1] >= 0)*/
	} /*for (iarg = 0; iarg < nargs; iarg < nargs)*/

	n2 = totCols + 1;

	result = RInstall(SCRATCH, n2*n2);
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	setNDIMS(result,2);
	setDIM(result, 1, n2);
	setDIM(result, 2, n2);

	if (lengths[0] > 0)
	{
		/* all arguments have labels */
		lengths[1] = lengths[0];

		TMPHANDLE = createLabels(2, lengths);
		if (TMPHANDLE == (char **) 0 || !setLabels(result, TMPHANDLE))
		{
			mydisphandle(TMPHANDLE);
			goto errorExit;
		}
		kvari = 0;
		appendNLabels(result, constant, 0, kvari++, 1);
		for (iarg = 0; iarg < nargs; iarg++)
		{
			symh = COMPVALUE(list, iarg);
			getMatLabels(symh, labs, lengthsi);
			(void) isMatrix(symh, dims);
			appendNLabels(result, labs[1], 0, kvari, dims[1]);
			kvari += dims[1];
		} /*for (iarg = 0; iarg < nargs; iarg++)*/		
		appendLabels(result, getOneLabel(result, 0, 0), 1, dontExpand);
	} /*if (iarg == nargs)*/		

	cp = DATAPTR(result);
	xbar = cp + 1;
	doubleFill(cp, 0.0, n2*n2);
	cp[0] = 1./(double) nrows;

	kvari = 0;
	for (iarg = 0; iarg < nargs ; iarg++)
	{
		argi = COMPVALUE(list, iarg);
		(void) isMatrix(argi, dims);
		xi = DATAPTR(argi);
		for (ivar = 0; ivar < dims[1]; ivar++)
		{
			for (i = 0; i < nrows; i++)
			{
				xbar[kvari] += xi[i];
			}
			xbar[kvari] /= (double) nrows;
			cp[(kvari + 1)*n2] = -xbar[kvari];
			kvari++;
			xi += nrows;
		} /*for (ivar = 0; ivar < dims[1]; ivar++)*/
	} /*for (iarg = 0; iarg < nargs ; iarg++)*/

	kvari = 0;

	for (iarg = 0; iarg < nargs ; iarg++)
	{
		argi = COMPVALUE(list, iarg);
		(void) isMatrix(argi, dims);
		ncolsi = dims[1];
		kvarj = kvari;
		for (jarg = iarg; jarg < nargs; jarg++)
		{
			argj = COMPVALUE(list, jarg);
			(void) isMatrix(argj, dims);
			ncolsj = dims[1];
			xi = DATAPTR(argi);
			ki = kvari + 1;
			for (ivar = 0; ivar < ncolsi; ivar++)
			{
				meani = xbar[ki-1];
				jmin = (jarg > iarg) ? 0 : ivar;
				kj = kvarj + jmin + 1;
				xj = DATAPTR(argj) + jmin*nrows;
				for (jvar = jmin; jvar < ncolsj; jvar++)
				{
					meanj = xbar[kj-1];
					cpij = cp + ki*n2 + kj;
					for (i = 0; i < nrows; i++)
					{
						*cpij += (xi[i] - meani)*(xj[i] - meanj);
					}
					cp[ki + kj*n2] = *cpij;
					xj += nrows;
					kj++;
				} /*for (jvar = 0; jvar < nmax; jvar++)*/
				xi += nrows;
				ki++;
			} /*for (ivar = 0; ivar < ncolsi; ivar++)*/
			kvarj += ncolsj;
		} /*for (jarg = iarg; jarg < nargs; jarg++)*/ 
		kvari += ncolsi;
	} /*for (iarg = 0; iarg < nargs ; iarg++)*/
	
#ifdef HASINFINITY
	cpij = cp + n2;
	for (kj = 1; kj < n2; kj++)
	{
		for (ki = 1; ki < n2; ki++)
		{
			if (isInfinite(cpij[ki]))
			{
				foundOverflow = 1;
				setMissing(cpij[ki]);
			}
		}
		cpij += n2;
	} /*for (kj = 1; kj < n2; kj++)*/	

	if (foundOverflow)
	{
		sprintf(OUTSTR,
				"WARNING: overflow in %s(); result(s) set to MISSING",
				FUNCNAME);
		putErrorOUTSTR();
	}
#endif /*HASINFINITY*/
	return (result);

  errorExit:
	putErrorOUTSTR();
	Removesymbol(result);

	return (0);
} /*bcprd()*/

/* 
   usage:
	   bcprd(x)
	   swp(x,cols) or swp(x,n1,n2,...)

  931103 modified  to accept swp(a, vec1, vec2, ...) with same meaning as
  swp(a,cat(vec1,vec2,...))
  960320 Output of swp(x,cols) takes labels from x
  960327 Output of bcprd(x) takes labels from getlabels(x,2)
  981228 Added keyword phrase 'quiet:T' to swp()
*/

Symbolhandle    swp(Symbolhandle list)
{
	Symbolhandle        arg1, argi, result = (Symbolhandle) 0;
	Symbolhandle        symhMatrix = (Symbolhandle) 0;
	Symbolhandle        symhSweptCols = (Symbolhandle) 0;
	long                nargs = NARGS(list);
	long                nRows, nCols, nswp = 0, i, iarg;
	long                dim[2], minRowCol;
	long                length;
	double              colNo, *coli, *colsP;
	double            **y = (double **) 0, **colsH = (double **) 0;
	double            **diagsH = (double **) 0, *diags = (double *) 0;
	double              diag[MAXDIAG];
	int                 quiet = -1, keepswept = 0;
	char               *keyword;
	WHERE("swp");
	
	if (strcmp(FUNCNAME, "bcprd") == 0)
	{
		return (bcprd(list));
	}
	
	if ((Trash = GarbInstall(NTRASH)) == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	
	OUTSTR[0] = '\0';

	arg1 = COMPVALUE(list,0);
	if (!argOK(arg1, 0, 1))
	{
		goto errorExit;
	}
	if (TYPE(arg1) != REAL || !isMatrix(arg1, dim))
	{
		sprintf(OUTSTR,"ERROR: 1st argument to %s() must be REAL matrix",
				FUNCNAME);
	}
	else if (anyMissing(arg1))
	{
		sprintf(OUTSTR,"ERROR: missing values in 1st argument to %s()",
				FUNCNAME);
	}
	if (*OUTSTR)
	{
		goto errorExit;
	}

	nRows = dim[0];
	nCols = dim[1];
	minRowCol = (nRows < nCols) ? nRows : nCols;
		
	while (nargs > 2)
	{
		Symbolhandle         symhKey = COMPVALUE(list, nargs - 1);
		
		keyword = isKeyword(symhKey);
		if (keyword == (char *) 0)
		{
			break;
		}
		
		if (strcmp(keyword, "quiet") == 0 || strncmp(keyword, "keep", 4) == 0)
		{
			if (!isTorF(symhKey))
			{
				notTorF(keyword);
				goto errorExit;
			}
			if (keyword[0] == 'q')
			{
				quiet = (DATAVALUE(symhKey, 0) != 0.0);
			}
			else
			{
				keepswept = (DATAVALUE(symhKey, 0) != 0.0);	
			}
		}
		else if (strncmp(keyword, "diag", 4) == 0)
		{
			if (TYPE(symhKey) != REAL || !isVector(symhKey) ||
				anyMissing(symhKey) ||
				doubleMin(DATAPTR(symhKey), DIMVAL(symhKey, 1)) <= 0.0)
			{
				sprintf(OUTSTR,
						"ERROR: value for %s is not vector of positive numbers",
						keyword);
				goto errorExit;
			}
			if (DIMVAL(symhKey, 1) != minRowCol)
			{
				sprintf(OUTSTR,
						"ERROR: length of value of %s does not match diagonal of %s() argument 1",
						keyword, FUNCNAME);
				goto errorExit;
			}
			diagsH = DATA(symhKey);
		}
		else
		{
			badKeyword(FUNCNAME, keyword);
			goto errorExit;
		}
		nargs--;
	} /*while (nargs > 2)*/
	
	if (quiet < 0)
	{
		quiet = keepswept;
	}

	if (nargs < 2)
	{
		badNargs(FUNCNAME, -1002);
		goto errorExit;
	}
	
	for (iarg = 1;iarg < nargs;iarg++)
	{
		if (!argOK(argi = COMPVALUE(list,iarg),REAL,iarg+1))
		{
			goto errorExit;
		}
		length = symbolSize(argi);
		if (!isVector(argi) && length != DIMVAL(argi,2))
		{/* neither column or row vector */
			sprintf(OUTSTR,
					"ERROR: argument %ld to %s() not a scalar or vector",
					iarg + 1, FUNCNAME);
		}
		else if (anyMissing(argi))
		{
			sprintf(OUTSTR,"ERROR: missing value in argument %ld to %s()",
					iarg + 1, FUNCNAME);
		}
		if (*OUTSTR)
		{
			goto errorExit;
		}
		colsP = DATAPTR(argi);
		for (i = 0;i < length;i++)
		{
			colNo = colsP[i];
			if (colNo <= 0 || colNo != floor(colNo))
			{
				sprintf(OUTSTR,
						"ERROR: %ld%s argument to %s() not all positive integers",
						iarg, n_th(iarg), FUNCNAME);
			}
			else if (colNo > minRowCol)
			{
				sprintf(OUTSTR,
						"ERROR: attempt to sweep a non-existent row or column");
			}
			if (*OUTSTR)
			{
				goto errorExit;
			}
		} /*for (i = 0;i < length;i++)*/
		nswp += length;
	} /*for (iarg = 1;iarg < nargs;iarg++)*/
	
	if (diagsH == (double **) 0)
	{
		if (minRowCol <= MAXDIAG)
		{ /* don't need scratch space; use auto variable */
			diags = diag;
			diagsH = &diags; /*fake handle*/
		}
		else
		{
			if (!getScratch(diagsH,GDIAGSH,minRowCol, double))
			{
				goto errorExit;
			}
		}
		setMissing((*diagsH)[0]);
	} /*if (diagsH = (double **) 0)*/
	
	
	if (nargs == 2 && !keepswept)
	{
		colsH = DATA(COMPVALUE(list,1));
	}
	else if (!getScratch(colsH, GCOLSH, nswp, double))
	{
		goto errorExit;
	}
	else
	{
		/* copy columns to be swept to *colsH */
		colsP = *colsH;
		for (iarg = 1;iarg < nargs; iarg++)
		{
			argi = COMPVALUE(list,iarg);
			coli = DATAPTR(argi);
			length = symbolSize(argi);
			for (i = 0;i < length;i++)
			{
				colsP[i] = coli[i];
			}
			colsP += length;
		} /*for (iarg = 1;iarg < nargs; iarg++)*/
	}

	if (!keepswept && isscratch(NAME(arg1)))
	{ /* re-use symbol */
		symhMatrix = result = reuseArg(list, 0, 1, 0);
	} /*if (isscratch(NAME(arg1)))*/
	else
	{/* copy arg1 to result or to component 1 of result*/
		char            *compnames[2];

		if (!keepswept)
		{
			symhMatrix = result = Install(SCRATCH, REAL);
		}
		else
		{
			compnames[0] = "matrix";
			compnames[1] = "sweptcols";
			result = RSInstall(SCRATCH, 2, compnames, 0);
			if (result == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			symhMatrix = COMPVALUE(result, 0);
			symhSweptCols = COMPVALUE(result, 1);
		}
		if (result == (Symbolhandle) 0 || !Copy(arg1, symhMatrix))
		{
			goto errorExit;
		}
		setNAME(result, SCRATCH);
		if (keepswept)
		{
			setNAME(symhMatrix, compnames[0]);
			unTrash(GCOLSH);
			setDATA(symhSweptCols, colsH);
			Setdims(symhSweptCols, 1, &nswp);
		}
	} /*if (isscratch(NAME(arg1))){}else{}*/

	Setdims(symhMatrix, 2, dim);

	if (HASLABELS(result))
	{
		if (!fixupMatLabels(result, USEBOTHLABELS))
		{
			goto errorExit;
		}
	} /*if (HASLABELS(result))*/

	y = DATA(symhMatrix);
	if (!swpIt(y, colsH, diagsH, nRows, nCols, nswp, quiet, keepswept))
	{
		goto errorExit;
	}
	
	emptyTrash();
	
	*OUTSTR = '\0';

	return (result);

  errorExit:
	putErrorOUTSTR();
	
	emptyTrash();
	Removesymbol(result);
	
	return (0);
	
} /*swp()*/

