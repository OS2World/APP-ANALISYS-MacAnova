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
#pragma segment Hconcat
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
/*
   950201  modified to allow CHAR arguments
   990226  replaced putOUTSTR() by putErrorOUTSTR()
*/

extern long             argOK();
extern long             symbolSize();

#define IHCONCAT        1
#define IVCONCAT        2

static void doRealConcat(Symbolhandle list, Symbolhandle result, long op)
{
	Symbolhandle   symh;
	long           j, k, nargs = NARGS(list);
	double        *inmat, *outmat = DATAPTR(result), *outmatj;
	long           size;
	long           nrowsIn, nrowsOut, ncols;
	WHERE("doRealConcat");

	if (op == IVCONCAT)
	{
		nrowsOut = DIMVAL(result,1); /* number of rows in result */
		ncols = DIMVAL(result, 2);
	}
	
	for (k = 0; k < nargs; k++)
	{
		/* for each component in list */

		symh = COMPVALUE(list, k);
		if (TYPE(symh) != NULLSYM)
		{
			size = symbolSize(symh);
			inmat = DATAPTR(symh);
			if (op == IHCONCAT)
			{
				doubleCopy(inmat, outmat, size);
				outmat += size;
			} /*if (op == IHCONCAT)*/
			else
			{
				nrowsIn = size/ncols;
				outmatj = outmat;
				for (j = 0;j<ncols;j++)
				{
					doubleCopy(inmat, outmatj, nrowsIn);
					inmat += nrowsIn;
					outmatj += nrowsOut;
				} /*for (j = 0;j<nrowsIn;j++)*/
				outmat += nrowsIn;
			} /*if (op == IHCONCAT){}else{}*/
		} /*if (TYPE(symh) != NULLSYM)*/
		
	} /*for (k = 0; k < nargs; k++)*/
} /*doRealConcat()*/

static void doCharConcat(Symbolhandle list, Symbolhandle result, long op)
{
	Symbolhandle   symh;
	long           j, k, nargs = NARGS(list);
	char          *instring, *outstring = STRINGPTR(result);
	long           nrowsIn, ncols;
	WHERE("doCharConcat");
	
	if (op == IHCONCAT)
	{
		for (k = 0; k < nargs; k++)
		{
			/* for each component in list */
			symh = COMPVALUE(list, k);
			if (TYPE(symh) != NULLSYM)
			{
				outstring = copyStrings(STRINGPTR(symh), outstring,
										symbolSize(symh));
			}
		} /*for (k = 0; k < nargs; k++)*/
	} /*if (op == IHCONCAT)*/
	else
	{ /* vconcat */
		ncols = DIMVAL(result, 2);
		for (j = 0;j < ncols; j++)
		{
			for (k = 0; k < nargs; k++)
			{
				/* find column j of each component in list */
				symh = COMPVALUE(list, k);
				if (TYPE(symh) != NULLSYM)
				{
					nrowsIn = symbolSize(symh)/ncols;
					instring = skipStrings(STRINGPTR(symh), j*nrowsIn);
					outstring = copyStrings(instring, outstring, nrowsIn);
				}				
			} /*for (k = 0; k < nargs; k++)*/
		} /*for (j = 0;j < ncols; j++)*/
	} /*if (op == IHCONCAT){}else{}*/
} /*doCharConcat()*/

/*
  971104 added check for value of myhandlelength()
*/
Symbolhandle    hconcat(Symbolhandle list)
{
	long            i, length, tot, totalsize, firstNonNull;
	long            ncols, nrows, nargs = NARGS(list);
	long            type;
	long            op;
	long            dims[2];
	long            haslabels = -1;
	long            needed = 0;
	char           *matlabs[2], *pos;
	long            labsizes[2];
	long            which;
	Symbolhandle    result = (Symbolhandle) 0, symh;
	WHERE("hconcat");
	
/* concatenate argument matrices sideways (hconcat) or vertically (vconcat) */

	OUTSTR[0] = '\0';
	op = (strcmp(FUNCNAME,"hconcat") == 0) ? IHCONCAT : IVCONCAT;

	for (firstNonNull = 0; firstNonNull < nargs; firstNonNull++)
	{
		symh = COMPVALUE(list,firstNonNull);
		if (!argOK(symh, NULLSYM, (nargs > 1) ? firstNonNull+1 : 0))
		{
			goto errorExit;
		}
		
		if (TYPE(symh) != NULLSYM)
		{
			break;
		}
	} /*for (firstNonNull = 0; firstNonNull < nargs; firstNonNull++)*/
	if (firstNonNull == nargs)
	{ /* all arguments of type	NULLSYM */
		return (NULLSYMBOL);
	}
		
	symh = COMPVALUE(list,firstNonNull); /* can't have type NULLSYM */
	if (!argOK(symh, (long) 0, 1))
	{
		goto errorExit;
	}
	
	type = TYPE(symh);
	if (type != REAL && type != LOGIC && type != CHAR)
	{
		badType(FUNCNAME,-type,1);
		goto errorExit;
	}
	
	totalsize = tot = 0;

/* check all arguments before proceeding && found size of output */	
	for (i = firstNonNull; i < nargs; i++)
	{ /* for each component in list */

		symh = COMPVALUE(list, i);

		if (!argOK(symh, NULLSYM,i+1))
		{
			goto errorExit;
		}
		if (TYPE(symh) == NULLSYM)
		{
			continue;
		}
		
		haslabels = (haslabels) ? HASLABELS(symh) : 0;
		if (TYPE(symh) != type)
		{
			sprintf(OUTSTR,"ERROR: arguments to %s must all have same type",
					FUNCNAME);
		}
		else if (!isMatrix(symh,dims))
		{
			sprintf(OUTSTR,
					"ERROR: argument %ld to %s not vector or matrix",i+1,
					FUNCNAME);
		}
		else if (i == firstNonNull)
		{
			length = dims[(op == IHCONCAT) ? 0 : 1];
		}
		else if (dims[(op == IHCONCAT) ? 0 : 1] != length)
		{
			sprintf(OUTSTR,
					"ERROR: all arguments to %s must have same number of %s",
					FUNCNAME, (op == IHCONCAT) ? "rows" : "columns");
		}
		if (*OUTSTR)
		{
			goto errorExit;
		}
		
		if (type == CHAR)
		{
			long         handleLength = myhandlelength(STRING(symh));

			if (handleLength == CORRUPTEDHANDLE)
			{
				goto errorExit;
			}
			if (handleLength > 0)
			{
				totalsize += handleLength;
			}
		}
		tot += dims[0]*dims[1];
	} /*for (i = firstNonNull; i < nargs; i++)*/

	nrows = (op == IHCONCAT) ? length : tot/length;
	ncols = tot / nrows;
	
	if (nargs == 1 && isscratch(NAME(result = COMPVALUE(list,0))))
	{ /* Reuse single scratch argument; it can't be NULLSYM */
		COMPVALUE(list, 0) = (Symbolhandle) 0;
		for (i = 3; i <= NDIMS(result); i++)
		{
			setDIM(result,i,0);
		}
		setNCLASS(result, -1);
		setNDIMS(result,2);
		setDIM(result,1,nrows);
		setDIM(result,2,ncols);	/* number of columns */
		return (result);
	} /*if (nargs == 1 && isscratch(NAME(result = COMPVALUE(list,0))))*/
	else
	{
		result = (type != CHAR) ?
			RInstall(SCRATCH,tot) : CInstall(SCRATCH, totalsize);
		if (result == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		setTYPE(result,type);
	} /*if(nargs == 1 && isscratch(NAME(result = COMPVALUE(list,0)))){}else{}*/
	
	setNDIMS(result,2);
	setDIM(result,1,nrows);
	setDIM(result,2,ncols);	/* number of columns */

	if (type == CHAR)
	{
		doCharConcat(list, result, op);
	}
	else
	{
		doRealConcat(list, result, op);
	}

	which = (op == IHCONCAT) ? 0 : 1;

	if (haslabels)
	{
		for (i = firstNonNull; i < nargs; i++)
		{ /* for each component in list */

			symh = COMPVALUE(list, i);

			if (TYPE(symh) == NULLSYM)
			{
				continue;
			}
			getMatLabels(symh, matlabs, labsizes);
			if (i == firstNonNull)
			{
				needed += labsizes[0] + labsizes[1];
			}
			else
			{
				needed += labsizes[1 - which];
			}
		} /*for (i = firstNonNull; i < nargs; i++)*/
		
		TMPHANDLE = mygethandle(needed);
		if (TMPHANDLE == (char **) 0 || !setLabels(result, TMPHANDLE))
		{
			mydisphandle(TMPHANDLE);
			goto errorExit;
		}
		pos = *TMPHANDLE;

		for (i = firstNonNull; i < nargs; i++)
		{ /* for each component in list */

			symh = COMPVALUE(list, i);

			if (TYPE(symh) == NULLSYM)
			{
				continue;
			}
			getMatLabels(symh, matlabs, labsizes);

			(void) isMatrix(symh, dims);
			if (i == firstNonNull && op == IHCONCAT)
			{
				pos = copyStrings(matlabs[0], pos, nrows);
			}
			pos = copyStrings(matlabs[1-which], pos, dims[1-which]);
		} /*for (i = firstNonNull; i < nargs; i++)*/
		if (op == IVCONCAT)
		{
			symh = COMPVALUE(list, firstNonNull);
			getMatLabels(symh, matlabs, labsizes);
			pos = copyStrings(matlabs[1], pos, ncols);
		} /*if (op == IVCONCAT)*/
	} /*if (haslabels)*/
	
	return (result);

  errorExit:
	putErrorOUTSTR();
	Removesymbol(result);
	return (0);
} /*hconcat()*/
