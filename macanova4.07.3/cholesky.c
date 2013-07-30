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
#pragma segment Choleski
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
#include "linpack.h"

/*
  usage:  cholesky(x), cholesky(x,pivot:T), or cholesky(x,force:vec)
*/

enum choleskyScratch
{
	GWORK = 0,
	GIPVT,
	NTRASH
};

/*
  980303 added check for MISSING values in argument to cholesky()
  980730 added new argument to reuseArg() so that notes will not be kept.
*/

Symbolhandle cholesky(Symbolhandle list)
{
	Symbolhandle            result = (Symbolhandle) 0;
	Symbolhandle            triang = (Symbolhandle) 0;
	Symbolhandle            pivot = (Symbolhandle) 0;
	Symbolhandle            arg1, arg2;
	double                **workH;
	double                 *aj = (double *) 0;
	long                  **ipvtH;
	long                    nargs = NARGS(list);
	long                    pivotit = 0, info = 0;
	long                    dim[2], ncols, nvalues, i, j;
	char                   *keyword;
	WHERE("cholesky");
	TRASH(NTRASH, errorExit);
	
	if (nargs > 2)
	{
		badNargs(FUNCNAME, -2);
		goto errorExit;
	}
	arg1 = COMPVALUE(list,0);
	if (!argOK(arg1,0,(nargs > 1) ? 1 : 0))
	{
		goto errorExit;
	}
	if (TYPE(arg1) != REAL || !isMatrix(arg1,dim) || dim[0] != dim[1])
	{
		sprintf(OUTSTR,
				"ERROR: argument%s to %s() is not square REAL matrix",
				(nargs > 1) ? " 1" : "",FUNCNAME);
	}
	else if (anyMissing(arg1))
	{
		sprintf(OUTSTR,
				"ERROR: argument to %s() must not have MISSING elements",
				FUNCNAME);
	}
	else if (!checkSymmetry(DATAPTR(arg1),ncols = dim[0]))
	{
		sprintf(OUTSTR,
				"ERROR: argument%s to %s() is not symmetric",
				(nargs > 1) ? " 1" : "",FUNCNAME);
	}
	
	if (*OUTSTR)
	{
		goto errorExit;
	}
	nvalues = ncols*ncols;
	
	if (nargs == 2)
	{
		arg2 = COMPVALUE(list,1);
		if (!argOK(arg2,0,2))
		{
			goto errorExit;
		}
		if ((keyword = isKeyword(arg2)))
		{
			if (strncmp(keyword,"piv",3) == 0)
			{
				pivotit = -1;
			}
			else if (strcmp(keyword,"force") != 0)
			{
				badKeyword(FUNCNAME,keyword);
				goto errorExit;
			}
		}
		if (pivotit || !keyword)
		{
			if (!keyword)
			{
				keyword = "2nd argument";
			}
			if (!isTorF(arg2))
			{
				notTorF(keyword);
				goto errorExit;
			}
			pivotit = (DATAVALUE(arg2,0) != 0.0);
		} /*if (pivotit || !keyword)*/
		else
		{ /* must be force */
			if (TYPE(arg2) != REAL || !isVector(arg2) ||
				symbolSize(arg2) != ncols)
			{
				sprintf(OUTSTR,
						"ERROR: value for %s must be REAL vector with length = nrows(arg1)",
						keyword);
				goto errorExit;
			}
			pivotit = 2;
		} /*if (pivotit || !keyword){}else{}*/
	} /*if (nargs == 2)*/

	if (!getScratch(workH, GWORK, ncols, double) ||
	   pivotit && !getScratch(ipvtH, GIPVT, ncols, long))
	{
		goto errorExit;
	}

	if (!isscratch(NAME(arg1)))
	{
		result = triang = RInstall(SCRATCH, nvalues);
		if (triang == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		doubleCopy(DATAPTR(arg1), DATAPTR(triang), nvalues);
	} /*if (!isscratch(NAME(arg1)))*/
	else
	{
		/* reuse argument for result if scratch */
		triang = result = reuseArg(list, 0, 0, 0);
	}		
	setNDIMS(triang,2);
	setDIM(triang,1,ncols);
	setDIM(triang,2,ncols);
	setNCLASS(triang,-1);

	if (pivotit)
	{
		result = StrucInstall(SCRATCH, 2);
		if (result == (Symbolhandle) 0)
		{
			Removesymbol(triang);
			goto errorExit;
		}
		Cutsymbol(triang);
		COMPVALUE(result,0) = triang;
		setNAME(triang,"r");
		pivot = COMPVALUE(result,1) = Makereal(ncols);
		if (pivot == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		setNAME(pivot,"pivot");
		if (pivotit == 2)
		{
			aj = DATAPTR(arg2);
		}
		
		for (i=0;i<ncols;i++)
		{
			(*ipvtH)[i] = 0	;
			if (aj != (double *) 0 && aj[i] != 0.0)
			{
				(*ipvtH)[i] = (aj[i] > 0.0) ? 1 : -1;
			}
		} /*for (i=0;i<ncols;i++)*/
	} /*if (pivotit)*/
	
	
	DCHDC(DATAPTR(triang), &ncols, &ncols, *workH,
		  (pivotit) ? *ipvtH : (long *) 0, &pivotit, &info);
	
	if (info != ncols)
	{
		sprintf(OUTSTR,
				"ERROR: argument to %s() is not positive definite", FUNCNAME);
		putOUTSTR();
		sprintf(OUTSTR,
				"       Problem found while %s %ld",
				(pivotit) ? "doing pivot number" : "pivoting column",info);
		goto errorExit;
	} /*if (info != ncols)*/
	
	aj = DATAPTR(triang);
	for (j = 0;j < ncols;j++)
	{
		if (pivotit)
		{
			DATAVALUE(pivot,j) = (double) (*ipvtH)[j];
		}
		for (i = j+1;i<ncols;i++)
		{
			aj[i] = 0.0;
		}
		aj += ncols;
	} /*for (j = 0;j < ncols;j++)*/
	
	emptyTrash();
	UNLOADSEG(DCHDC);
	
	return (result);
	
  errorExit:
	putErrorOUTSTR();
	
	emptyTrash();
	
	Removesymbol(result);
	UNLOADSEG(DCOPY);
	UNLOADSEG(DCHDC);

	return (0);
} /*cholesky()*/
