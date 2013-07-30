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
#pragma segment Qr
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
#include "linpack.h"

enum triqrOpCodes
{
	ITRILOWER = 1,
	ITRIUPPER,
	ITRIUNPACK,
	ICOMPQR,
	IQR = ICOMPQR,
	IQRDECODE
};

/*
   working triupper(), trilower(), and triunpack() code for CHAR variables
*/
static long cTriangle(Symbolhandle In, Symbolhandle Out,
					 long nrowsIn, long ncolsIn, long iop, char *keyword)
{
	long           i, j;
	long           nc, incr1, incr2;
	char          *cIn;
	char          *cInj, *cOutij, *cTmp;
	char          *nullStr = "";
	long           needed = 0;
	WHERE("cTriangle");
	
	while (needed >= 0)
	{
		cIn = STRINGPTR(In);
		if (needed > 0)
		{
			needed = -1;
			cOutij = STRINGPTR(Out);
		}
	
		if (iop != ITRIUNPACK)
		{/* triupper or trilower */
			if (*keyword == '\0')
			{/* full triangle (triupper(x) or trilower (x)*/
				cInj = cIn;

				for (j = 0;j<ncolsIn;j++)
				{
					for (i=0;i<nrowsIn;i++)
					{
						cTmp = cInj;
						if (i > j && iop != ITRILOWER ||
						   i < j && iop != ITRIUPPER)
						{
							cTmp = "";
						}
						if (needed < 0)
						{
							cOutij = copyStrings(cTmp, cOutij, 1);
						} /*if (needed < 0)*/
						else
						{
							needed += strlen(cTmp) + 1;
						} /*if (needed < 0){}else{}*/
						cInj = skipStrings(cInj, 1);
					} /*for (i=0;i<nrowsIn;i++)*/
				} /*for (j = 0;j<ncolsIn;j++)*/
			} /*if (*keyword == '\0')*/
			else if (strcmp(keyword,"pack") == 0)
			{/* packed form */
				if (iop == ITRIUPPER)
				{/* triupper(x,T) */
					cInj = cIn;
					for (j=0;j<ncolsIn;j++)
					{
						for (i=0;i <= j;i++)
						{
							if (needed < 0)
							{
								cOutij = copyStrings(cInj, cOutij, 1);
							}
							else
							{
								needed += strlen(cInj) + 1;
							}
							cInj = skipStrings(cInj, 1);
						} /*for (i=0;i <= j;i++)*/
						if (j < ncolsIn-1)
						{
							cInj = skipStrings(cInj, nrowsIn - j - 1);
						}
					} /*for (j=0;j<ncolsIn;j++)*/
				} /*if (iop == ITRIUPPER)*/
				else
				{/* trilower(x,T) (packed)*/
					for (i=0;i<nrowsIn;i++)
					{
						cInj = skipStrings(cIn, i);
						for (j=0;j <= i;j++)
						{
							if (needed < 0)
							{
								cOutij = copyStrings(cInj, cOutij, 1);
							}
							else
							{
								needed += strlen(cInj) + 1;
							}
							cInj = skipStrings(cInj, nrowsIn);
						} /*for (j=0;j<=i;j++)*/
					} /*for (i=0;i<nrowsIn;i++)*/
				} /*if (iop == ITRIUPPER){}else{}*/
			} /*else if (strcmp(keyword,"pack") == 0)*/
			else if (strcmp(keyword,"square") == 0)
			{ /* make output ncols by ncols (only for triupper) */
				nc = (ncolsIn < nrowsIn) ? ncolsIn : nrowsIn;
				cInj = cIn;
				for (j = 0;j<nc;j++)
				{
					for (i=0;i<nc;i++)
					{
						cTmp = (i > j) ? nullStr : cInj;
						if (needed < 0)
						{
							cOutij = copyStrings(cTmp, cOutij, 1);
						}
						else
						{
							needed += strlen(cTmp) + 1;
						}
						cInj = skipStrings(cInj, 1);
					} /*for (i=0;i<nc;i++)*/
					cInj = skipStrings(cInj, nrowsIn - nc);
				} /*for (j = 0;j<nc;j++)*/
			} /*else if (strcmp(keyword,"square") == 0)*/
		} /*if (iop != ITRIUNPACK)*/
		else
		{/* triunpack() */
			cInj = cIn;
			if (strcmp(keyword,"lower") == 0)
			{/* make lower triangle */
				incr1 = 0;
				for (j = 0; j < nrowsIn; j++)
				{
					cInj = skipStrings(cIn, incr1);
					incr1 += j + 2;
					for (i = 0; i < nrowsIn; i++)
					{
						if (i < j)
						{
							cTmp = nullStr;
						} /*if (i < j)*/
						else
						{
							cTmp = cInj;
							if (i < nrowsIn - 1)
							{
								incr2 = i + 1;
								cInj = skipStrings(cInj, incr2);
							}
						} /*if (i < j){}else{}*/
						if (needed < 0)
						{
							cOutij = copyStrings(cTmp, cOutij, 1);
						}
						else
						{
							needed += strlen(cTmp) + 1;
						}
					} /*for (i = 0; i < nrowsIn; i++)*/
				} /*for (j = 0; j < nrowsIn, j++)*/
			} /*if (strcmp(keyword,"lower") == 0)*/
			else if (strcmp(keyword,"upper") == 0)
			{/* make upper triangular matrix */
				cInj = cIn;
				for (j=0;j<nrowsIn;j++)
				{
					for (i=0;i<nrowsIn;i++)
					{
						if (i <= j)
						{
							cTmp = cInj;
							cInj = skipStrings(cInj, 1);
						}
						else
						{
							cTmp = nullStr;
						}

						if (needed < 0)
						{
							cOutij = copyStrings(cTmp, cOutij, 1);
						}
						else
						{
							needed += strlen(cTmp) + 1;
						}
					} /*for (i=0;i<nrowsIn;i++)*/
				} /*for (j=0;j<nrowsIn;j++)*/					
			} /*else if (strcmp(keyword,"upper") == 0)*/
			else
			{/* make fully symmetric */
				incr1 = 0;
				for (j = 0; j < nrowsIn; j++)
				{
					incr1 += j;
					cInj = skipStrings(cIn, incr1);
					for (i = 0; i < nrowsIn; i++)
					{
						if (needed < 0)
						{
							cOutij = copyStrings(cInj, cOutij, 1);
						}
						else
						{
							needed += strlen(cInj) + 1;
						}
						if (i < nrowsIn - 1)
						{
							incr2 = (i < j) ? 1 : i + 1;
							cInj = skipStrings(cInj, incr2);
						}									
					} /*for (i = 0; i < nrowsIn; i++)*/
				} /*for (j = 0; j < nrowsIn, j++)*/
			} /*else fully symmetric */
		} /*if (iop != ITRIUNPACK){}else{}*/
		if (needed > 0)
		{
			TMPHANDLE = mygethandle(needed);
			if (TMPHANDLE == (char **) 0)
			{
				return (0);
			}
			setSTRING(Out, TMPHANDLE);
		} /*if (needed > 0)*/
	} /*while (needed >= 0)*/
	return (1);
} /*cTriangle()*/

/*
   working triupper(), trilower(), and triunpack() code for REAL and LOGIC
   variables
*/
static void dTriangle(Symbolhandle In, Symbolhandle Out,
					 long nrowsIn, long ncolsIn, long iop, char *keyword)
{
	long           i, j;
	long           nc, incr1, incr2;
	double        *dIn, *dOut;
	double        *dInj, *dOutij, dTmp;
	WHERE("dTriangle");
	
	dIn = DATAPTR(In);
	dOutij = dOut = DATAPTR(Out);

	if (iop != ITRIUNPACK)
	{/* triupper or trilower */
		if (*keyword == '\0')
		{/* full triangle (triupper(x) or trilower (x)*/
			dInj = dIn;

			for (j = 0;j<ncolsIn;j++)
			{
				for (i=0;i<nrowsIn;i++)
				{
					dTmp = *dInj++;
					if (i > j && iop != ITRILOWER ||
					   i < j && iop != ITRIUPPER)
					{
						dTmp = 0.0;
					}
					*dOutij++ = dTmp;
				} /*for (i=0;i<nrowsIn;i++)*/
			} /*for (j = 0;j<ncolsIn;j++)*/
		} /*if (*keyword == '\0')*/
		else if (strcmp(keyword,"pack") == 0)
		{/* packed form */
			if (iop == ITRIUPPER)
			{/* triupper(x,T) */
				dInj = dIn;
				for (j=0;j<ncolsIn;j++)
				{
					for (i=0;i<=j;i++)
					{
						*dOutij++ = *dInj++;
					} /*for (i=0;i<=j;i++)*/
					dInj += (nrowsIn - j - 1);
				} /*for (j=0;j<ncolsIn;j++)*/
			} /*if (iop == ITRIUPPER)*/
			else
			{/* trilower(x,T) (packed)*/
				for (i=0;i<nrowsIn;i++)
				{
					dInj = dIn + i;
					for (j=0;j<=i;j++)
					{
						*dOutij++ = *dInj;
						dInj += nrowsIn;
					} /*for (j=0;j<=i;j++)*/
				} /*for (i=0;i<nrowsIn;i++)*/
			} /*if (iop == ITRIUPPER){}else{}*/
		} /*else if (strcmp(keyword,"pack") == 0)*/
		else if (strcmp(keyword,"square") == 0)
		{ /* make output ncols by ncols (only for triupper) */
			nc = (ncolsIn < nrowsIn) ? ncolsIn : nrowsIn;
			dInj = dIn;
			for (j = 0;j<nc;j++)
			{
				for (i=0;i<nc;i++)
				{
					dTmp = (i > j) ? 0.0 : *dInj;
					*dOutij++ = dTmp;
					dInj++;
				} /*for (i=0;i<nc;i++)*/
				dInj += nrowsIn - nc;
			} /*for (j = 0;j<nc;j++)*/
		} /*else if (strcmp(keyword,"square") == 0)*/
	} /*if (iop != ITRIUNPACK)*/
	else
	{/* triunpack() */
		dInj = dIn;

		if (strcmp(keyword,"lower") == 0)
		{/* make lower triangle */
			for (i=0;i<nrowsIn;i++)
			{
				/* fill output row by row */
				dOutij = dOut + i;
				for (j=0;j<nrowsIn;j++)
				{
					dTmp = (j <= i) ? *dInj++ : 0.0;
					*dOutij = dTmp;
					dOutij += nrowsIn;
				} /*for (j=0;j<nrowsIn;j++)*/
			} /*for (i=0;i<nrowsIn;i++)*/
		} /*if (strcmp(keyword,"lower") == 0)*/
		else if (strcmp(keyword,"upper") == 0)
		{/* make upper triangular matrix */
			dInj = dIn;
			dOutij = dOut;
			for (j=0;j<nrowsIn;j++)
			{
				for (i=0;i<nrowsIn;i++)
				{
					if (i <= j)
					{
						dTmp = *dInj++;
					}
					else
					{
						dTmp = 0.0;
					}
					*dOutij++ = dTmp;
				} /*for (i=0;i<nrowsIn;i++)*/
			} /*for (j=0;j<nrowsIn;j++)*/
		} /*else if (strcmp(keyword,"upper") == 0)*/
		else
		{/* make fully symmetric */
			incr1 = 0;
			for (j = 0; j < nrowsIn; j++)
			{
				incr1 += j;
				dInj = dIn + incr1;
				for (i = 0; i < nrowsIn; i++)
				{
					*dOutij++ = *dInj;
					incr2 = (i < j) ? 1 : i + 1;
					dInj += incr2;
				} /*for (i = 0; i < nrowsIn; i++)*/
			} /*for (j = 0; j < nrowsIn, j++)*/
		} /*else fully symmetric */
	} /*if (iop != ITRIUNPACK){}else{}*/
} /*dTriangle()*/

static void doQr(double **hx, double **hqraux, double **hr, long **hpivot,
				 double **hwork, long nrows, long ncols)
{
	long            nvals = nrows*ncols;
	double         *x = *hx, *r = *hr;
	long            pivotit = (hpivot != (long **) 0);
	long            i;
	WHERE("doQr");
	
	doubleCopy(x, r, nvals);

	if (pivotit)
	{
		for (i=0;i<ncols;i++)
		{
			(*hpivot)[i] = 0;
		}
	} /*if (pivotit)*/
	
	DQRDC(*hr, &nrows, &nrows, &ncols, *hqraux, 
		   (pivotit) ? *hpivot : (long *) 0,
		   (pivotit) ? *hwork : (double *) 0, &pivotit);

	UNLOADSEG(DDOT);
	UNLOADSEG(DQRDC);

} /*doQr()*/

typedef struct opEntry
{
	char *name;
	short iop;
	short nargs;
} opEntry;

static opEntry Ops[] =
{ /* name        iop             nargs */
	{"trilower",  ITRILOWER,       -2}, /* trilower(a [,pack:T]) */
	{"triupper",  ITRIUPPER,       -2}, /* triupper(a [,pack:T]) */
	{"triunpack", ITRIUNPACK,      -2}, /* triunpack(a [,upper|lower:T) */
	{"qr",        IQR,             -2}, /* qr(x [,ronly]:T) */
	{"qrdecode",  IQRDECODE,      -10}, /* qrdecode()*/
	{(char *) 0,  0,                0}
};

enum qrScratch
{
	GHWORK = 0,
	GHPIVOT,
	NTRASH
};

#define QRFUZZ           1e-12 /* tolerance for deviation from integer */
/*
  980303 added check for MISSING values in argument to qr()
  981107 fixed bug affecting when nrows(x) < ncols(x);
         qraux is initialized to 0
         with ronly:T, result has dimensions  nrows(x),min(nrows(x),ncols(x))
         and is initialized to 0
*/

Symbolhandle qr(Symbolhandle list)
{
	Symbolhandle      arg[2];
	Symbolhandle      r = (Symbolhandle) 0, qraux = (Symbolhandle) 0;
	Symbolhandle      pivot = (Symbolhandle) 0;
	Symbolhandle      result = (Symbolhandle) 0;
	long            **hpivot = (long **) 0;
	double          **hwork = (double **) 0;
	long              dims[2], nrows, ncols;
	long              margs, iop, nargs = NCOMPS(list), nvals;
	long              i, realOnly, type;
	long              logValue = 0, square = 0, ronly = 0,pivotit = 0;
	char             *keyword = "", *legalKeywords[3], **kw;
	double            tmp;
	opEntry          *op;
	char             *compNames[3];
	WHERE("qr");
	TRASH(NTRASH,errorExit);
	
	*OUTSTR = '\0';
	compNames[0] = "qr";
	compNames[1] = "qraux";
	compNames[2] = "pivot";
	
	for (op=Ops;op->iop;op++)
	{
		if (strcmp(op->name,FUNCNAME) == 0)
		{
			break;
		}
	} /*for (op=Ops;op->iop;op++)*/

	iop = op->iop;

	if (iop == 0 || iop == IQRDECODE)
	{
		sprintf(OUTSTR,"ERROR: %s() not implemented",FUNCNAME);
		goto errorExit;
	} /*if (op->iop == 0)*/

	
	margs = op->nargs;
	if (margs > 0 && nargs != margs || margs < 0 && nargs > labs(margs))
	{
		badNargs(FUNCNAME,margs);
		goto errorExit;
	} /*if (margs > 0 && nargs != margs || margs < 0 && nargs > labs(margs))*/

	for (i=0;i<nargs;i++)
	{
		arg[i] = COMPVALUE(list,i);
	}
	
	realOnly = (iop == IQR);
	if (!argOK(arg[0],0 ,1))
	{
		goto errorExit;
	}
	type = TYPE(arg[0]);
	
	if (type != REAL && (realOnly || type != CHAR && type != LOGIC))
	{
		badType(FUNCNAME, -type, 1);
		goto errorExit;
	}
	
	if (!isMatrix(arg[0],dims))
	{
		sprintf(OUTSTR,"ERROR: argument 1 to %s is not %s",FUNCNAME,
				(iop != ITRIUNPACK) ? "matrix" : "vector");
		goto errorExit;
	}

	if (iop == IQR && anyMissing(arg[0]))
	{
		sprintf(OUTSTR,
				"ERROR: argument 1 to %s() must not have MISSING elements",
				FUNCNAME);
		goto errorExit;
	}
	
	nrows = dims[0];
	ncols = dims[1];
	if (iop == ITRIUNPACK)
	{
		tmp = sqrt(1.0 + 8.0*(double) nrows);
		if (!isVector(arg[0]) || floor(tmp + QRFUZZ) != ceil(tmp - QRFUZZ))
		{
			sprintf(OUTSTR,
					"ERROR: 1st argument to %s is not vector with length of form n*(n+1)/2",
					FUNCNAME);
			goto errorExit;
		}
		else
		{
			nrows = ((long) floor(tmp + QRFUZZ) - 1) / 2;
		}
	} /*if (iop == ITRIUNPACK)*/
	
	legalKeywords[0] = "";
	legalKeywords[1] = legalKeywords[2] = (char *) 0;
	switch(iop)
	{
	  case ITRIUPPER:
		legalKeywords[1] = "square";
	  case ITRILOWER:
		legalKeywords[0] = "pack";
		break;

	  case IQR:
		legalKeywords[0] = "pivot";
		legalKeywords[1] = "ronly";
		break;

	  case ITRIUNPACK:
		legalKeywords[0] = "lower";
		legalKeywords[1] = "upper";
		break;
	} /*switch(iop)*/

	if (nargs == 2)
	{
		if ((keyword = isKeyword(arg[1])))
		{
			for (kw = legalKeywords;*kw && strcmp(keyword,*kw) != 0;kw++)
			{
				;
			}
			if (*kw == (char *) 0)
			{
				badKeyword(FUNCNAME, keyword);
				goto errorExit;
			}
		}
		else
		{ /* use of keyword is optional */
			keyword = legalKeywords[0];
		}
		
		if (!argOK(arg[1],0, 2))
		{
			goto errorExit;
		}
		if (!isTorF(arg[1]))
		{
			notTorF(keyword);
			goto errorExit;
		}
		logValue = DATAVALUE(arg[1],0) != 0.0;
		
		if (iop == ITRILOWER || iop == ITRIUPPER)
		{
			square = (strcmp(keyword,"square") == 0);
			if (square && !logValue)
			{
				square = 0;
				keyword = "";
			}
			
			if (logValue && nrows != ncols && !square)
			{
				sprintf(OUTSTR,
						"ERROR: input matrix must be square when %s is T",
						keyword);
				goto errorExit;
			}
		} /*if (iop == ITRILOWER || iop == ITRIUPPER)*/
		else if (iop == IQR)
		{
			pivotit = (strcmp(keyword,"pivot") == 0);
			if (pivotit)
			{
				if (!logValue)
				{
					pivotit = 0;
				}
				else
				{
					logValue = 0;
				}
			}
			ronly = logValue;
		} /*if (iop == ITRILOWER || iop == ITRIUPPER){}else if (iop == IQR){}*/
	} /* if (nargs == 2) */
	
	/* nvals is space needed for (principal) output matrix */
	if (iop == ITRILOWER || iop == ITRIUPPER)
	{
		if (!logValue)
		{
			nvals = nrows*ncols;
		}
		else if (!square)
		{
			nvals = (nrows*(nrows+1))/2;
		}
		else
		{
			nvals = (ncols < nrows) ? ncols : nrows;
			nvals = nvals*nvals;
		}
	} /*if (iop == ITRILOWER || iop == ITRIUPPER)*/
	else if (iop == IQR)
	{
		nvals = nrows*ncols;
	} /*else if (iop == IQR)*/
	else if (iop == ITRIUNPACK)
	{
		if (!logValue && *keyword)
		{ /* swap keywords if F */
			keyword = (strcmp(keyword,legalKeywords[0]) == 0) ?
				legalKeywords[1] : legalKeywords[0];
		}
		nvals = nrows * nrows;
	} /*else if (iop == ITRIUNPACK)*/

	if (iop != IQR || ronly)
	{
		result = (type != CHAR) ? 
			RInstall(SCRATCH,nvals) : CInstall(SCRATCH, 0);
		if (result == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		setNDIMS(result, 1);
		setDIM(result, 1, nvals);
	} /*if (iop != IQR || ronly)*/
	else
	{ /* IQR && !ronly, save complete results in structure */
		result = StrucInstall(SCRATCH,(pivotit) ? 3L : 2L);
		if (result == (Symbolhandle) 0)
		{
			goto errorExit;
		}
	}
		
	if (iop == IQR)
	{
		/* create symbols r and qraux */
		r = RInstall(SCRATCH,nvals);
		qraux = RInstall(SCRATCH,ncols);
		if (r == (Symbolhandle) 0 || qraux == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		symFill(qraux, 0.0);
		if (!ronly)
		{
			Cutsymbol(r);
			COMPVALUE(result,0) = r;
			Cutsymbol(qraux);
			COMPVALUE(result,1) = qraux;
			setNAME(r,compNames[0]);
			setNAME(qraux,compNames[1]);
		}
		else
		{
			setNDIMS(result,2);
			setDIM(result,1, (nrows >= ncols) ? ncols : nrows);
			setDIM(result,2,ncols);
		}
		
		setNDIMS(r,2);
		setDIM(r,1,nrows);
		setDIM(r,2,ncols);

		if (pivotit)
		{
			if (!getScratch(hpivot,GHPIVOT,ncols,long) ||
			   !getScratch(hwork,GHWORK,ncols,double) ||
			   (pivot = RInstall(SCRATCH,ncols)) == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			if (!ronly)
			{
				Cutsymbol(pivot);
				COMPVALUE(result,2) = pivot;
				setNAME(pivot,compNames[2]);
			}
		} /*if (pivotit)*/
	} /*if (iop == IQR)*/
	else if ((iop == ITRIUPPER || iop == ITRILOWER) && (!logValue || square))
	{
		setNDIMS(result,2);
		setDIM(result,1,(square) ? (long) sqrt(.1+(double) nvals) : nrows);
		setDIM(result,2,(square) ? DIMVAL(result,1) : ncols);
	} /*else if ((iop == ITRIUPPER || iop == ITRILOWER) && (!logValue || square))*/
	else if (iop == ITRIUNPACK)
	{
		setNDIMS(result,2);
		setDIM(result,1,nrows);
		setDIM(result,2,nrows);
	} /*else if (iop == ITRIUNPACK)*/
	
	switch (iop)
	{
	  case ITRILOWER:
	  case ITRIUPPER:
	  case ITRIUNPACK:
		if (type == CHAR)
		{
			if (!cTriangle(arg[0], result, nrows, ncols, iop, keyword))
			{
				goto errorExit;
			}
		}
		else
		{
			dTriangle(arg[0], result, nrows, ncols, iop, keyword);
		}

		break;

	  case IQR:
		doQr(DATA(arg[0]),DATA(qraux),DATA(r), 
			 (pivotit) ? hpivot : (long **) 0,
			 (pivotit) ? hwork : (double **) 0, nrows, ncols);

#ifdef HASNAN
		if (anyDoubleNaN(DATAPTR(qraux), ncols) ||
			anyDoubleNaN(DATAPTR(r), nvals))
		{
			goto overflowExit;
		}
#endif /*HASNAN*/
		if (pivotit)
		{
			for (i=0;i<ncols;i++)
			{
				DATAVALUE(pivot,i) = (double) (*hpivot)[i];
			}
		} /*if (pivotit)*/
		
		if (ronly)
		{
			dTriangle(r,result, nrows,ncols,ITRIUPPER,"square");
			if (nrows < ncols)
			{
				long      squarepart = nrows*nrows;
				
				doubleCopy(DATAPTR(r) + squarepart,
						   DATAPTR(result) + squarepart, nvals - squarepart);
			}
			Removesymbol(qraux);
			Removesymbol(r);
		}
		break;
	} /*switch (iop)*/

	emptyTrash();
	return(result);
	
#ifdef HASNAN
  overflowExit:
	sprintf(OUTSTR,
			"ERROR: overflow in %s() computations", FUNCNAME);
	/*fall through*/
#endif /*HASNAN*/
  errorExit:
	putErrorOUTSTR();
	emptyTrash();
	if (iop == IQR && ronly)
	{
		Removesymbol(r);
		Removesymbol(qraux);
		if (pivotit)
		{
			Removesymbol(pivot);
		}
	}
	
	Removesymbol(result);
	
	return (0);
}



