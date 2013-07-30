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
#pragma segment Tserops
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

/*
  990226 replaced putOUTSTR() by putErrorOUTSTR()
*/

#include "globals.h"
#include "keywords.h"
#include "tser.h"

typedef struct opEntry
{
	char *name;
	long iop;
	long nargs;
} opEntry;
/*
	0 < nargs < 10  means function expects exactly nargs nonkeyword arguments
	nargs < 0 means functions expects uptto abs(nargs) args, no keywords
	nargs = nkey*10 + nonkey means functions expects nonkeyword arguments
	   and up to nkey keyword phrases.
    On hpolar() and cpolar() unwind:F can be specified by simply F as arg 2
	On convolve(), reverse:T can be specified by simply T as arg 3
*/

static opEntry Ops[] =
{ /* name        iop      nargs */
	{"hconj",    IHCONJ,     1}, /* hconj(hx) */
	{"cconj",    ICCONJ,     1}, /* cconj(cx) */
	{"hreal",    IHREAL,     1}, /* hreal(hx) */
	{"creal",    ICREAL,     1}, /* creal(cx) */
	{"himag",    IHIMAG,     1}, /* himag(hx) */
	{"cimag",    ICIMAG,     1}, /* cimag(cx) */
	{"hcopyc",   ICTOH,      1}, /* hcopyc(hx) (obsolete)*/
	{"ccopyh",   IHTOC,      1}, /* ccopyh(cx) (obsolete) */
	{"htoc",     IHTOC,      1}, /* htoc(hx) */
	{"ctoh",     ICTOH,      1}, /* ctoh(cx) */
	{"hprdh",    IHPRDH,    -2}, /* hprdh(hx1 [,hx2]) */
	{"hprdhj",   IHPRDHJ,   -2}, /* hprdhj(hx1 [,hx2]) */
	{"cprdc",    ICPRDC,    -2}, /* cprdc(cx1 [,cx2]) */
	{"cprdcj",   ICPRDCJ,   -2}, /* cprdcj(cx1 [,cx2]) */
	{"cmplx",    ICMPLX,    -2}, /* cmplx(rx1[, rx2]) */
	{"hpolar",   IHPOLAR,   21}, /* hpolar(hx [,unwind:F] [,crit:val]) */
	{"cpolar",   ICPOLAR,   21}, /* cpolar(cx [,unwind:F] [,crit:val]) */
	{"hrect",    IHRECT,     1}, /* hrect(hx) */
	{"crect",    ICRECT,     1}, /* crect(cx) */
	{"unwind",   IUNWIND,    11}, /* unwind(theta [,crit:val]) */
	{"convolve", ICONVOLVE,  22}, /* convolve(coefs,rx[,reverse:T][,decimate:T]) */
	{"rotate",   IROTATE,    2}, /* rotate(rx,lag) */
	{"reverse",  IREVERSE,   1}, /* reverse(rx) */
	{"padto",    IPADTO,     2}, /* padto(rx,nrows) */
	{(char *) 0, 0,          0}
};

#define CRITERION .75 /* default for keyword 'crit' */


enum Tser
{
	KUNWIND = 0,
	KCRITERION,
	KREVERSE,
	KDECIMATE
};

static keywordList TserKeys[] = 
{
	InitKeyEntry("unwind",0,IUNWIND | ICPOLAR | IHPOLAR,LOGICSCALAR),
	InitKeyEntry("criterion",4,IUNWIND | ICPOLAR | IHPOLAR,REALSCALAR),
	InitKeyEntry("reverse",0,ICONVOLVE,LOGICSCALAR),
	InitKeyEntry("decimate",3,ICONVOLVE,POSITIVEINT)
};

#define Unwind    KeyLogValue(TserKeys,KUNWIND)
#define Criterion KeyRealValue(TserKeys,KCRITERION)
#define Reverse   KeyLogValue(TserKeys,KREVERSE)
#define Decimate  KeyIntValue(TserKeys,KDECIMATE)

/*
   960904 fixed bug in cmplx() that did not allow cmplx(re) as equivalent to
   cmplx(re,0*re)

   980730 added new argument to reuseArg() so that notes will not be kept.
   981213 dropped unused 5th argument (ncolsOut) to qonerg()
*/

Symbolhandle tserops(Symbolhandle list)
{
	Symbolhandle      arg[2], result = (Symbolhandle) 0, symhKey;
	opEntry          *op;
	long              nargs = NARGS(list);
	long              margs = 1, nonKeyArgs, nkeys = 0, startKeys, type;
	long              mkeys = NKeys(TserKeys);
	keywordListPtr    keys = TserKeys;
	long              logicOK;
	long              iop = 0, i;
	long              dims[2][2];
	long              ncoefs, nrowsIn1, ncolsIn1, ncolsIn2, ncolsOut, nrowsOut;
	long              krot, keyStatus;
	long              xprd, rect, polar;
	double            value = 0.0;
	double           *presult, *parg0, *parg1;
	long              reuseArg1, reuseArg2;
	char             *programProblem =
	  "ERROR:****Programming problem %d for %s()";

	WHERE("tserops");
	
	*OUTSTR = '\0';
	arg[1] = (Symbolhandle) 0;

	/* set defaults */
	Criterion = CRITERION;
	Decimate = 1;
	Reverse = 0;
	Unwind = 1;

	for (op = Ops; op->iop != 0 && strcmp(op->name,FUNCNAME) != 0; op++)
	{
		;
	}
	
	if (op->iop == 0)
	{ /* should not occur unless programming error */
		sprintf(OUTSTR,"ERROR: (internal) unrecognized command %s()",
				FUNCNAME);
		goto errorExit;
	}

	iop = op->iop;
	xprd = iop & IPRD;
	polar = iop & (IPOLAR | IUNWIND);
	rect = iop & IRECT;

	margs = op->nargs;
	nonKeyArgs = labs(margs);
/*
  nonKeyArgs is expected number of nonKeyword arguments
  nkeys is the maximum number of keyword arguments
  margs is the negative of the maximum number of arguments.
*/
	if (margs > 10)
	{
		nonKeyArgs = margs % 10;
		nkeys = margs / 10;
		margs = -(nonKeyArgs + nkeys);
	}

	
/* logicOK != 0 means 1st argument can be LOGICAL */
	logicOK = iop & (IROTATE | IREVERSE);
	
	if (nargs > labs(margs) || margs > 0 && nargs != margs)
	{
		badNargs(FUNCNAME,margs);
		goto errorExit;
	}
	
	/* Check non keyword arguments */
	for (i = 0;i < nonKeyArgs && i < nargs; i++)
	{
		if (!argOK(arg[i] = COMPVALUE(list,i),0,i+1))
		{
			goto errorExit;
		}
		
		if (!logicOK && strucAnyMissing(arg[i], REALTYPE1))
		{
			sprintf(OUTSTR,
					"ERROR: MISSING values not allowed in arguments to %s()",
					FUNCNAME);
		}
		else if (isKeyword(arg[i]))
		{
			sprintf(OUTSTR,
					"ERROR: argument %ld to %s() cannot be keyword phrase",
					i+1, FUNCNAME);
		}
		if (*OUTSTR)
		{
			goto errorExit;
		}
		type = TYPE(arg[i]);
		if (type != REAL && (!logicOK || type != LOGIC))
		{
			badType(FUNCNAME, -type, i+1);
			goto errorExit;
		}
	} /*for (i = 0;i < nonKeyArgs && i < nargs; i++)*/

	startKeys = nonKeyArgs;

/*	All operations except convolve() may have matrix for arg 1 */
	if (iop & ICONVOLVE && !isVector(arg[0]))
	{
		sprintf(OUTSTR,"ERROR: first argument to %s() is not vector",
				FUNCNAME);
	} /*if (iop & ICONVOLVE && !isVector(arg[0]))*/
	else if (!isMatrix(arg[0],dims[0]))
	{
		sprintf(OUTSTR,
				"ERROR: first argument to %s() is not a vector or matrix",
				FUNCNAME);
	}
	if (*OUTSTR)
	{
		goto errorExit;
	} /*if (*OUTSTR)*/

/*
  hpolar(hx,F) is equivalent to hpolar(hx,unwind:F)
  cpolar(hx,F) is equivalent to cpolar(hx,unwind:F)
  convolve(a,x,T) is equivalent to convolve(a,x,reverse:T)
*/
	if (nargs > nonKeyArgs && !isKeyword(symhKey = COMPVALUE(list, i)))
	{
		/* hpolar(), cpolar() and convolve() allow you to not use keyword */
		if (!(iop & (IPOLAR | ICONVOLVE)))
		{
			sprintf(OUTSTR,
					"ERROR: too many non-keyword phrase arguments to %s()",
					FUNCNAME);
			goto errorExit;
		}
		if (!isTorF(symhKey))
		{
			sprintf(OUTSTR,
					"ERROR: argument %ld to %s() not T or F", i + 1, FUNCNAME);
			goto errorExit;
		}
		value = DATAVALUE(symhKey,0);
		if (iop & IPOLAR)
		{
			Unwind = (value != 0.0);
		}
		else
		{
			Reverse = (value != 0.0);
		}
		startKeys++;
	} /*if (nargs > i && !isKeyword(symhKey = COMPVALUE(list, i)))*/

	if (xprd && nargs == 1)
	{ /* [ch]prd[ch]j?(x) equivalent to [ch]prd[ch]j?(x,x)*/
		arg[1] = arg[0];
		dims[1][0] = dims[0][0];
		dims[1][1] = dims[0][1];
	} /*if (xprd && nargs == 1)*/
	
	if (nonKeyArgs == 2 && nargs > 1 && !isKeyword(arg[1]))
	{
		if (iop & (IROTATE | IPADTO))
		{ /* these require scalar arg 2 */
			value = DATAVALUE(arg[1], 0);
			if (!isScalar(arg[1]) || isMissing(value) ||
				value != floor(value) || (iop & IPADTO) && value <= 0.0)
			{
				sprintf(OUTSTR,
						"ERROR: second argument to %s() is not single integer%s",
						FUNCNAME, (iop & IPADTO) ? " > 0" : "");			
			}
			krot = (long) value;
		}
		else if (!isMatrix(arg[1], dims[1]))
		{
			sprintf(OUTSTR,
					"ERROR: second argument to %s() is not vector or matrix",
					FUNCNAME);
		}
	} /*if (nonKeyArgs == 2)*/

	if (*OUTSTR)
	{
		goto errorExit;
	}
	
	/* check keywords*/
	/* pick off keyword values */
	keyStatus = getAllKeyValues(list, startKeys, iop, keys, mkeys);
	if (keyStatus < 0)
	{
		goto errorExit;
	}
	nargs -= keyStatus;
	
	if (Criterion < 0.5 || Criterion > 1.0)
	{
		sprintf(OUTSTR,
				"ERROR: value for %s() must be between 0.5 and 1.0",
				KeyName(keys,KCRITERION));
		goto errorExit;
	}
	
	/* Might it be possible to reuse scratch arguments; further checks later*/
	reuseArg1 = (iop != IPADTO && iop != IROTATE && iop != ICONVOLVE &&
				 isscratch(NAME(arg[0])));
	reuseArg2 = (xprd && nargs > 1 && isscratch(NAME(arg[1])));

	*OUTSTR = '\0';

/* Determine size of nrowsOut by ncolsOut output matrix */

	nrowsOut = nrowsIn1 = dims[0][0];
	ncolsIn1 = dims[0][1];
	
	if (nonKeyArgs == 1 || polar)
	{/*
	   hconj(), cconj(), hreal(), creal(), himag(), cimag(), htoc(), ctoh(),
	   hrect(), crect(), hpolar(), cpolar(), unwind()
	 */
		if (iop & (ICCONJ | ICRECT | ICPOLAR))
		{
			ncolsOut = ncolsIn1 + (ncolsIn1 & 1);
		}
		else if (iop & (ICREAL | ICIMAG | ICTOH))
		{
			ncolsOut = (ncolsIn1 + 1)/2;
		}
		else if (iop & IHTOC)
		{
			ncolsOut = 2*ncolsIn1;
		}
		else
		{ /* hconj, hreal, himag, hrect, unwind, reverse */
			ncolsOut = ncolsIn1;
		}
	} /*if (margs == 1 || polar*/
	else if (xprd)
	{/* hprdh(), hprdhj(), cprdc(), cprdcj()*/
		ncolsIn2 = dims[1][1];
		if (dims[1][0] != nrowsIn1)
		{
			sprintf(OUTSTR,
					"ERROR: arguments to %s() have different numbers of rows",
					FUNCNAME);
		}
		else
		{
			if (iop & (ICPRDC | ICPRDCJ))
			{
				if (ncolsIn1 > 2 && ncolsIn2 > 2 && labs(ncolsIn1-ncolsIn2) > 1)
				{
					sprintf(OUTSTR,
							"ERROR: number columns of arguments to %s() do not match",
							FUNCNAME);
				}
				else
				{
					ncolsOut = (ncolsIn1 > ncolsIn2) ? ncolsIn1 : ncolsIn2;
					ncolsOut += ncolsOut & 1; /* increase to even */
				}
			} /*if (iop & (ICPRDC | ICPRDCJ))*/
			else
			{/* hprdh or hprdhj */
				if (ncolsIn1 > 1 && ncolsIn2 > 1 && ncolsIn1 != ncolsIn2)
				{
					sprintf(OUTSTR,
							"ERROR: arguments to %s() have different numbers of columns",
							FUNCNAME);
				}
				else
				{
					ncolsOut = (ncolsIn1 > ncolsIn2) ? ncolsIn1 : ncolsIn2;
				}
			}  /*if (iop & (ICPRDC | ICPRDCJ)){}else{}*/
		} /*if (dims[1][0] != nrowsIn1)*/
	}
	/* margs == 2: cmplx(), convolve(), rotate(), padto() */
	else if (iop & ICMPLX)
	{
		if (nargs > 1 && (dims[1][0] != nrowsIn1 || dims[1][1] != ncolsIn1))
		{
			sprintf(OUTSTR,
					"ERROR: arguments to %s() not the same size and shape",
					FUNCNAME);
		}
		else
		{
			ncolsOut = 2*ncolsIn1;
		}
	}
	else if (iop & ICONVOLVE)
	{
		ncoefs = nrowsIn1;
		nrowsIn1 = dims[1][0];
		nrowsOut = (nrowsIn1 - 1)/Decimate + 1;
		ncolsOut = ncolsIn1 = dims[1][1];
	}
	else if (iop & (IROTATE | IPADTO))
	{ /* padto(), rotate() */
		ncolsOut = ncolsIn1;
		nrowsOut = (iop == IROTATE) ? nrowsIn1 : krot;
	}
	else
	{ /* should never happen */
		sprintf(OUTSTR, programProblem, 1, FUNCNAME);
	}
	if (*OUTSTR)
	{
		goto errorExit;
	}

	if (reuseArg1 && nrowsOut == nrowsIn1 && ncolsOut == ncolsIn1)
	{
		reuseArg2 = 0;
	}
	else
	{
		reuseArg1 = 0;
	}
	if (reuseArg1 ||
		reuseArg2 && (nrowsOut != nrowsIn1 || ncolsOut != ncolsIn2))
	{
		reuseArg2 = 0;
	}
	
	if (reuseArg1)
	{
		result = reuseArg(list, 0, 0, 0);
	}
	else if (reuseArg2)
	{
		result = reuseArg(list, 1, 0, 0);
	}
	else
	{
		if (isTooBig(nrowsOut, ncolsOut, sizeof(double)))
		{
			resultTooBig(FUNCNAME);
			goto errorExit;
		}
		
		result = RInstall(SCRATCH,nrowsOut*ncolsOut);
		if (result == (Symbolhandle) 0)
		{
			goto errorExit;
		}
	}
	
	setNCLASS(result, -1);

	if (ncolsOut > 1)
	{
		setNDIMS(result,2);
		setDIM(result,2,ncolsOut);
	}
	setDIM(result,1,nrowsOut);

	if (logicOK)
	{
		setTYPE(result, TYPE(arg[0]));
	}

	presult = DATAPTR(result);
	parg0 = DATAPTR(arg[0]);
	if (arg[1] != (Symbolhandle) 0)
	{
		parg1 = DATAPTR(arg[1]);
	}
	
	if (margs == 1 && !rect)
	{
		/* 981213 eliminated unused 5th argument (ncolsOut) to qonerg */
		qonerg(parg0, presult, nrowsIn1, ncolsIn1, iop);
	}
	else if (xprd || iop == ICMPLX)
	{
		qtworg(parg0, (iop == ICMPLX && nargs == 1) ? (double *) 0 : parg1,
			   presult, nrowsIn1, ncolsIn1, ncolsIn2, iop);
	}
	else if (polar || rect)
	{
		qrect(parg0, presult, nrowsIn1, ncolsIn1, Unwind, Criterion, iop);
	}
	else if (iop == IPADTO)
	{
		qpadto(parg0, presult, nrowsIn1, ncolsIn1, nrowsOut);
	}
	else if (iop == IROTATE)
	{
		qrota(parg0, presult, nrowsIn1, ncolsIn1, krot);
	}
	else if (iop == ICONVOLVE)
	{
		qconv(parg0, parg1, presult, 
			  ncoefs, nrowsIn1, ncolsIn1, Decimate, Reverse);
	}
	else	
	{ /* should not happen */
		sprintf(OUTSTR, programProblem, 2, FUNCNAME);
		goto errorExit;
	}
	if (interrupted(DEFAULTTICKS) != INTNOTSET)
	{
		goto errorExit;
	}
	
	return (result);

  errorExit:
	putErrorOUTSTR();
	Removesymbol(result);
	return(0);
	
} /*tserops()*/
