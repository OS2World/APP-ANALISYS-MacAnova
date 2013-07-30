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
#pragma segment Cor
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/



#include "globals.h"
#include "dbug.h"
#include "blas.h"

/*
   Modified 940803 to recognize constant columns and treat them specially
   All elements of the corresponding row and column of the out put are set
   MISSING
  990226 replaced putOUTSTR() by putErrorOUTSTR()
*/
#define GMNS    0
#define GUSE    1
#define GXMAT   2
#define GROW1   3
#define NTRASH  4

static long doCor(double *x,double *r,short *use,double *means,
				  long nrows,long ncols)
{
	long             i, j, k, ncases = 0;
	long             ij, ik, jk, kj, jj, row1 = -1;
	long             foundConstant = 0;
	double           s, meanj, meank;
	double           temp, *xrow1;
	WHERE("doCor");
	
	for (j = 0; j < ncols; j++)
	{
		means[j] = 0.0;
	}
	
	for (i = 0; i < nrows; i++)
	{
		use[i] = 1;
		ij = i;
		for (j = 0; j < ncols; j++)
		{/* check i-th row for any missing values */
			if (isMissing(x[ij]))
			{
				use[i] = 0;
				break;
			}
			ij += nrows;
		}
		if (use[i])
		{
			if(row1 < 0)
			{
				row1 = i;
			}
			ncases++;
			xrow1 = x + row1 - i;
			ij = i;
			for (j = 0; j < ncols; j++)
			{
				means[j] += x[ij] - xrow1[ij];
				ij += nrows;
			} /*for (j = 0; j < ncols; j++)*/
		}
	} /*for (i = 0; i < nrows; i++)*/
	if (ncases == 0)
	{
		return (0);
	}
	
	/* compute lower triangle of mean corrected CP matrix */
	xrow1 = x + row1;
	for (j = 0; j < ncols; j++)
	{
		means[j] = meanj = means[j] / (double) ncases + xrow1[j*nrows];
		jk = j;
		for (k = 0; k <= j; k++)
		{	 /* lower triangle */
			s = 0.0;
			meank = means[k];
			ik = k*nrows;
			ij = j*nrows;
			
			for (i = 0; i < nrows; i++)
			{
				if (use[i])
				{
					s += (x[ij] - meanj) * (x[ik] - meank);
				}
				ij++;
				ik++;
			}
			r[jk] = s;
			jk += ncols;
		} /*for (k = 0; k <= j; k++)*/
	} /*for (j = 0; j < ncols; j++)*/
	
	for (j = 0; j < ncols; j++)
	{
		jj = j * (ncols + 1);
		jk = j;
		kj = jj + 1;
		if(r[jj] > 0)
		{
			temp = sqrt(r[jj]);
			for (k = 0; k < j; k++)
			{
				if(!foundConstant || !isMissing(r[jk]))
				{
					r[jk] /= temp;
				}
				
				jk += ncols;
			}
			r[jk] = 1.0;
			for (k = j + 1; k < ncols; k++)
			{
				if(!foundConstant || !isMissing(r[kj]))
				{
					r[kj] /= temp;
				}
				
				kj++;
			}
		}  /*if(r[jj] > 0)*/
		else
		{ /* set row & column corresponding to constant column to MISSING*/
			foundConstant = 1;
			for (k = 0; k <= j; k++)
			{
				setMissing(r[jk]);
				jk += ncols;
			}
			for (k = j + 1; k < ncols; k++)
			{
				setMissing(r[kj]);
				kj++;
			}
		} /*if(r[jj] > 0){}else{}*/
	} /*for (j = 0; j < ncols; j++)*/

	/* make symmetric */

	for (j = 1; j < ncols; j++)
	{
		jk = j;
		kj = ncols*j;
		for (k = 0; k < j; k++)
		{
			r[kj] = r[jk];
			jk += ncols;
			kj++;
		}
	}
	return ((foundConstant) ? -ncases : ncases);
	
}

/* compute correlation matrix */
Symbolhandle    cor(list)
Symbolhandle    list;
{
	long            ncases;
	long            i, nrows, tot, subtot;
	long            dims[2], ncols, nargs = NARGS(list);
	Symbolhandle    result = (Symbolhandle) 0, arg;
	double        **mnsH = (double **) 0;
	double        **xmatH = (double **) 0, **resmatH = (double **) 0;
	short         **useH = (short **) 0;
	double         *xmat;
	WHERE("cor");
	TRASH(NTRASH, errorExit);
	
	OUTSTR[0] = '\0';
	for(i=0;i<nargs;i++)
	{
		arg = COMPVALUE(list, i);
		if(!argOK(arg, REAL, i+1))
		{
			goto errorExit;
		}
		if(!isMatrix(arg, dims))
		{
			sprintf(OUTSTR,
					"ERROR: argument %ld to %s not a matrix or vector",
					i+1, FUNCNAME);
		}
		else if(i == 0)
		{
			nrows = dims[0];
			ncols = dims[1];
		}
		else if(dims[0] != nrows)
		{
			sprintf(OUTSTR,
					"ERROR: all arguments to %s must have same number of rows",
					FUNCNAME);
		}
		else
		{
			ncols += dims[1];
		}
		if(*OUTSTR)
		{
			goto errorExit;
		}
	}

	result = RInstall(SCRATCH, ncols*ncols);
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	setNDIMS(result, 2);
	setDIM(result, 1, ncols);
	setDIM(result, 2, ncols);

	if (nargs == 1 && HASLABELS(arg))
	{
		char        *labs[2];
		long         lengths[2], dims1[2];
		
		getMatLabels(arg, labs, lengths);
		lengths[0] = lengths[1];
		labs[0] = labs[1];
		dims1[0] = dims1[1] = ncols;
		TMPHANDLE = createLabels(2, lengths);

		if (TMPHANDLE == (char **) 0 || !setLabels(result, TMPHANDLE))
		{
			mydisphandle(TMPHANDLE);
			goto errorExit;
		}
		/* find labels again in case they've moved */
		getMatLabels(arg, labs, lengths);
		labs[0] = labs[1];
		buildLabels(TMPHANDLE, labs, dims1, 2);
	} /*if (nargs == 1 && HASLABELS(arg))*/
	
	resmatH = DATA(result);

	if(!getScratch(mnsH, GMNS, ncols, double) ||
	   !getScratch(useH, GUSE, nrows, short))
	{
		goto errorExit;
	}

	tot = nrows * ncols;
	if(nargs > 1)
	{
		if(!getScratch(xmatH, GXMAT, tot, double))
		{
			goto errorExit;
		}

		/* concatenate together all arguments */
		xmat = *xmatH;
		for (i = 0; i < nargs; i++)
		{
			/* for each component in list */

			arg = COMPVALUE(list, i);
			subtot = symbolSize(arg);
			doubleCopy(DATAPTR(arg), xmat, subtot);
			xmat += subtot;
		}
	}
	else
	{
		xmatH = DATA(arg);
	}

	ncases = doCor(*xmatH, *resmatH, *useH, *mnsH, nrows, ncols);

	if(ncases == 0)
	{
		sprintf(OUTSTR, "ERROR: all cases missing in %s", FUNCNAME);
		goto errorExit;
	}
	if (labs(ncases) < nrows)
	{
		sprintf(OUTSTR, "WARNING: %ld cases with missing values deleted in %s",
				 nrows - labs(ncases), FUNCNAME);
		putErrorOUTSTR();
	}
	if(ncases < 0)
	{
		sprintf(OUTSTR,
				"WARNING: constant data found by %s; rows and columns set MISSING",
				FUNCNAME);
		putErrorOUTSTR();
	}
	
	emptyTrash();

	return (result);

  errorExit:
	putErrorOUTSTR();
	emptyTrash();
	Removesymbol(result);
	
	return (0);
}

