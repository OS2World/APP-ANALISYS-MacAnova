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
#pragma segment Dim
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

#include "globals.h"

/*
  990226 Removed unnecessary externs and defines
         Replaced putOUTSTR() by putErrorOUTSTR()
*/
#define INDIMS   0
#define INROWS   1
#define INCOLS   2
#define INCOMPS  3
#define ILENGTH  4
#define IDIM     5

static long strucMaxNdims(Symbolhandle arg)
{
	long        maxNdims = 0;
	long        i, ncomps, ndims;
	
	if (!isNull(arg))
	{
		if (isStruc(arg))
		{
			ncomps = NCOMPS(arg);
			for (i=0;i<ncomps;i++)
			{
				ndims = strucMaxNdims(COMPVALUE(arg,i));
				maxNdims = (ndims > maxNdims) ? ndims : maxNdims;
			}
		} /*if (isStruc(arg))*/
		else
		{
			maxNdims = NDIMS(arg);
		} /*if (isStruc(arg)){}else{}*/
	} /*if (!isNull(arg))*/
	return (maxNdims);
} /*strucMaxNdims()*/

/*
   Handle ndims(), nrows(), ncols(), length()
*/
static Symbolhandle doDimOps(Symbolhandle arg, long op)
{
	long           i, ncomps,dim[2], val;
	Symbolhandle   result = (Symbolhandle) 0, symh = (Symbolhandle) 0;

	if (!isNull(arg))
	{
		if (isStruc(arg))
		{
			ncomps = NCOMPS(arg);
			result = StrucInstall(SCRATCH,ncomps);
			if (result == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			
			for (i=0;i<ncomps;i++)
			{
				symh = doDimOps(COMPVALUE(arg,i),op);
				if (symh == (Symbolhandle) 0)
				{
					goto errorExit;
				}
				Cutsymbol(symh);
				COMPVALUE(result,i) = symh;
				setCompName(symh,NAME(COMPVALUE(arg,i)));
			}
		} /*if (isStruc(arg))*/
		else
		{
			result = RInstall(SCRATCH,1);
			if (result == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			if (op == INROWS || op == INCOLS)
			{
				(void) isMatrix(arg,dim);
				val = dim[(op==INROWS) ? 0 : 1];
			}
			else if (op == ILENGTH)
			{
				val = symbolSize(arg);
			}
			else if (op == INDIMS)
			{
				val = NDIMS(arg);
			}
			DATAVALUE(result,0) = (double) val;
		} /*if (isStruc(arg)){}else{}*/
	} /*if (!isNull(arg))*/
	else
	{
		result = RInstall(SCRATCH, 1);
		if (result == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		DATAVALUE(result, 0) = 0.0;

	} /*if (!isNull(arg)){}else{}*/
	return (result);

  errorExit:
	Removesymbol(result);
	return (0);
} /*doDimOps()*/

/* handle dim() */
static long doDim(Symbolhandle arg,Symbolhandle result, long ndims)
{
	long           i, j, ncomps;
	char          *name;
	Symbolhandle   symh, dim, result1 = (Symbolhandle) 0;

	if (isStruc(arg))
	{
		ncomps = NCOMPS(arg);
		for (i=0;i<ndims;i++)
		{
			COMPVALUE(result,i) = Makestruc(ncomps);
			if (COMPVALUE(result,i) == (Symbolhandle) 0)
			{
				goto errorExit;
			}
		}
		
		result1 = StrucInstall(SCRATCH,ndims);
		if (result1 == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		
		for (i=0;i<ncomps;i++)
		{
			if (!doDim(COMPVALUE(arg,i),result1,ndims))
			{
				goto errorExit;
			}
			name = NAME(COMPVALUE(arg,i));
			for (j=0;j<ndims;j++)
			{
				dim = COMPVALUE(result1,j);
				COMPVALUE(result1,j) = (Symbolhandle) 0;
				setCompName(dim,name);
				symh = COMPVALUE(result,j);
				COMPVALUE(symh,i) = dim;
			} /* for (j=0;j<ndims;j++)*/
		}
		Removesymbol(result1);
		return (1);
	} /*if (isStruc(arg))*/
	
	for (i=0;i<ndims;i++)
	{
		dim = COMPVALUE(result,i) = Makereal(1);
		if (dim == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		
		DATAVALUE(dim,0) = (!isNull(arg) && i < NDIMS(arg)) ?
			DIMVAL(arg,i+1) : 0;
	}
	return (1);

  errorExit:
	Removesymbol(result1);

	return (0);
} /*doDim()*/
	
/*
  dim(x), ndims(x), nrows(), ncols(), ncomps(),  length()

*/

Symbolhandle    dim(Symbolhandle list)
{
	Symbolhandle    result = (Symbolhandle) 0, symh;
	long            ndims,op;
	long            i;
	WHERE("dim");
	
	symh = COMPVALUE(list, 0);

	if (NARGS(list) != 1 || symh == (Symbolhandle) 0)
	{
		badNargs(FUNCNAME,1);
		goto errorExit;
	}
	
	if (!argOK(symh, NULLSYM, 0))
	{
		goto errorExit;
	}

	if (TYPE(symh) == BLTIN)
	{
		badType(FUNCNAME,-BLTIN,0);
		goto errorExit;
	}

	if (strcmp("ncomps",FUNCNAME) == 0)
	{
		if (!isStruc(symh))
		{
			sprintf(OUTSTR,"ERROR: argument to %s not structure",FUNCNAME);
			goto errorExit;
		}
		result = RInstall(SCRATCH,1);
		if (result == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		DATAVALUE(result,0) = (double) NCOMPS(symh);
	}
	else if (strcmp("dim",FUNCNAME) == 0)
	{ /* dim() */
		if (!isStruc(symh))
		{
			if (isNull(symh))
			{
				ndims = 0;
				result = Install(NULLSCRATCH, NULLSYM);
			}
			else
			{
				ndims = NDIMS(symh);
				result = RInstall(SCRATCH,ndims);
			}
			
			if (result == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			
			for (i = 0;i < ndims;i++)
			{
				DATAVALUE(result,i) = (double) DIMVAL(symh,i+1);
			}
		} /*if (!isStruc(symh))*/
		else
		{ /* dim(structure) */
			ndims = strucMaxNdims(symh);
			if (ndims == 0)
			{
				if (Install(NULLSCRATCH, NULLSYM) == (Symbolhandle) 0)
				{
					goto errorExit;
				}
			} /* if (ndims == 0)*/
			else
			{
				if ((result = StrucInstall(SCRATCH,ndims)) == (Symbolhandle) 0)
				{
					goto errorExit;
				}
			
				if (!doDim(symh,result,ndims))
				{
					goto errorExit;
				}
				for (i = 0;i<ndims;i++)
				{
					sprintf(OUTSTR,"dim%ld",i+1);
					setCompName(COMPVALUE(result,i),OUTSTR);
				}
			} /* if (ndims == 0){}else{}*/
			*OUTSTR = '\0';
		} /*if (!isStruc(symh)){}else{}*/
	}
	else
	{ /* ndims(), nrows(), ncols(), length() */
		if (strcmp(FUNCNAME,"ndims") == 0)
		{
			op = INDIMS;
		}
		else if (strcmp(FUNCNAME,"nrows") == 0)
		{
			op = INROWS;
		}
		else if (strcmp(FUNCNAME,"ncols") == 0)
		{
			op = INCOLS;
		}
		else if (strcmp(FUNCNAME,"length") == 0)
		{
			op = ILENGTH;
		}
		if ((op == INROWS || op == INCOLS) && !strucIsMatrix(symh))
		{
			sprintf(OUTSTR,"ERROR: argument to %s is not a matrix%s",FUNCNAME,
					(isStruc(symh)) ? " or a structure of matrices" : "");
			goto errorExit;
		}
		result = doDimOps(symh,op);
		if (result == (Symbolhandle) 0)
		{
			goto errorExit;
		}
	}		
	
	return (result);

  errorExit:
	putErrorOUTSTR();

	return (0);
	
} /*dim()*/
  
#define IDIAG 1
#define IDMAT 2

static Symbolhandle doDiag(Symbolhandle arg, long *dims)
{
	Symbolhandle result = (Symbolhandle) 0;
	char        *cdiag, *cdest;
	double      *ddiag, *ddest;
	long         type = TYPE(arg), needed;
	long         incr, n, m, i, j;
	
	m = dims[0];
	incr = m + 1;

	n = (m < dims[1]) ? m : dims[1];
	
	if (type == CHAR)
	{
		needed = 0;
		cdiag = STRINGPTR(arg);
		for (i = 0; i < n; i++)
		{
			needed += strlen(cdiag) + 1;
			if (i < n-1)
			{
				cdiag = skipStrings(cdiag, incr);
			}
		} /*for (i = 0; i < n; i++)*/		
		result = CInstall(SCRATCH, needed);
	} /*if (type == CHAR)*/
	else
	{
		result = RInstall(SCRATCH,n);
	}
	
	if (result != (Symbolhandle) 0)
	{
		setTYPE(result,TYPE(arg));
		if (type == CHAR)
		{
			setDIM(result, 1, n);
			cdiag = STRINGPTR(arg);
			cdest = STRINGPTR(result);
			for (i = 0; i < n; i++)
			{
				cdest = copyStrings(cdiag, cdest, 1);
				if (i < n-1)
				{
					cdiag = skipStrings(cdiag, incr);
				}
			} /*for (i = 0; i < n; i++)*/
		} /*if (type == CHAR)*/
		else
		{
			ddiag = DATAPTR(arg);
			ddest = DATAPTR(result);
			for (i = 0, j = 0;i< n;i++, j += incr)
			{
				ddest[i] = ddiag[j];
			}
		} /*if (type == CHAR){}else{}*/		
	} /*if (result != (Symbolhandle) 0)*/
	
	return (result);
} /*doDiag()*/

/*
  Construct diagonal matrix
  if there is 1 argument, it is a vector or m by 1 or 1 by m matrix
  if there are 2 argument, the first is m and the second is v and
  it returns v*iden(m)
*/

static Symbolhandle doDmat(Symbolhandle arg1,Symbolhandle arg2,long *dims)
{
	Symbolhandle result;
	double      *dIn, *dOut;
	long         n, incr1, incr2, i, k, l;
	long         type;
	long         needed = 0;
	char        *cIn, *cOut;
	WHERE("doDmat");
	
	if (arg2 != (Symbolhandle) 0)
	{ /* dmat(n,value) */
		n = (long) DATAVALUE(arg1,0);
		arg1 = arg2;
		incr1 = 0;
	}
	else
	{ /* dmat(vector) */
		n = dims[0];
		incr1 = 1;
	}

	type = TYPE(arg1);

	if (isTooBig(n, n, (type == CHAR) ? 1 : sizeof(double)))
	{
		resultTooBig(FUNCNAME);
		goto errorExit;
	}
	
	if (type == CHAR)
	{
		cIn = STRINGPTR(arg1);
		needed = n*n; /* space for all terminating '\0' */
		for (i = 0; i < n; i++)
		{
			needed += strlen(cIn);
			cIn = skipStrings(cIn, incr1);
		}
		result = CInstall(SCRATCH, needed);
	} /*if (type == CHAR)*/
	else
	{
		result = RInstall(SCRATCH,n*n);
	}
	
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	
	setNDIMS(result,2);
	setDIM(result,1,n);
	setDIM(result,2,n);
	setTYPE(result,type);

	if (type == CHAR)
	{
		cIn = STRINGPTR(arg1);
		cOut = STRINGPTR(result);
		incr2 = n;
		for (i = 0; i < needed; i++)
		{ /* fill output with nulls */
			cOut[i] = '\0';
		}
		for (i = 0; i < n;i++)
		{
			cOut = copyStrings(cIn, cOut, 1);
			if (i < n-1)
			{
				cIn = skipStrings(cIn, incr1);
				cOut += incr2;
			} /*if (i < n-1)*/
		} /*for (i = 0; i < n;i++)*/
	} /*if (type == CHAR)*/ 
	else
	{
		dIn = DATAPTR(arg1);
		dOut = DATAPTR(result);
		doubleFill(dOut, 0.0, n*n);
		k = 0;
		l = 0;
		incr2 = n + 1;
		for (i = 0;i< n;i++)
		{
			dOut[l] = dIn[k];
			k += incr1;
			l += incr2;
		} /*for (i = 0;i< n;i++)*/ 
	} /*if (type == CHAR){}else{}*/
	
	return (result);

  errorExit:
	return (0);
} /*doDmat()*/

/* diag(matrix) and dmat(vector) or dmat(n,value) */

Symbolhandle    diag(Symbolhandle list)
{
	Symbolhandle    result = (Symbolhandle) 0, arg1, arg2;
	long            nargs;
	long            type, margs, dims[2];
	long            op;
	double          value;
	WHERE("diag");
	
	if (strcmp(FUNCNAME,"diag") == 0)
	{
		op = IDIAG;
		margs = 1;
	}
	else if (strcmp(FUNCNAME,"dmat") == 0)
	{
		op = IDMAT;
		margs = -2;
	}

	nargs = NARGS(list);
	if ((margs >= 0 && nargs != margs) || (margs < 0 && nargs > -margs))
	{
		badNargs(FUNCNAME,margs);
		goto errorExit;
	}

	arg1 = COMPVALUE(list, 0);
	if (!argOK(arg1, 0, 1))
	{
		goto errorExit;
	}
	
	type = TYPE(arg1);
	if (op == IDIAG || nargs == 1)
	{
		if (type != REAL && type != LOGIC && type != CHAR)
		{
			badType(FUNCNAME,-type, 0);
			goto errorExit;
		}
		if (op == IDIAG)
		{
			if (!isMatrix(arg1,dims))
			{
				sprintf(OUTSTR, "ERROR: argument to %s must be matrix",
						FUNCNAME);
				goto errorExit;
			}
		} /*if (op == IDIAG)*/
		else
		{ /* dmat(vector) */
			if (!isVector(arg1) && DIMVAL(arg1,2) != symbolSize(arg1))
			{ /* allow row vector as argument */
				sprintf(OUTSTR,
						"ERROR: single argument to %s must be vector or matrix with 1 row",
						FUNCNAME);
				goto errorExit;
			}
			arg2 = (Symbolhandle) 0;
			dims[0] = symbolSize(arg1);
		} /*if (op == IDIAG){}else{}*/
	} /*if (op == IDIAG || nargs == 1)*/ 
	else
	{ /* dmat(n, value) */
		if (type != REAL || !isScalar(arg1) ||
			(value = DATAVALUE(arg1,0)) <= 0 || value != floor(value))
		{
			sprintf(OUTSTR,
					"ERROR: 1st argument of 2 to %s must be positive integer",
					FUNCNAME);
			goto errorExit;
		}
		arg2 = COMPVALUE(list,1);
		if (!argOK(arg2, 0,  1))
		{
			goto errorExit;
		}

		type = TYPE(arg2);
		if (type != REAL && type != LOGIC && type != CHAR)
		{
			badType(FUNCNAME,-type, 2);
			goto errorExit;
		}
		if (!isScalar(arg2))
		{
			sprintf(OUTSTR,
					"ERROR: 2nd argument to %s must be scalar (length = 1)",
					FUNCNAME);
			goto errorExit;
		}
		dims[0] = 1;
	} /*if (op == IDIAG || nargs == 1){}else{}*/

	if (op == IDIAG)
	{
		result = doDiag(arg1,dims);
	}
	else if (op == IDMAT)
	{
		result = doDmat(arg1,arg2,dims);
	}
	
	return (result);

  errorExit:
	putErrorOUTSTR();
	return (0);
} /*diag()*/

		











		
		
