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
#pragma segment Eigen
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
#include "blas.h"

/*
  980316 removed extraneous externs; added checks for NaNs
*/

static char         *OpNames[] =
{
	"eigen", "eigenvals", "releigen", "releigenvals", "trace", (char *) 0
};

enum eigenCodes
{
  IEIGEN = 1,
  IEIGENVALS,
  IRELEIGEN,
  IRELEIGENVALS,
  ITRACE
};

enum eigenScratch
{
	GVALS = 0,
	GAA,
	GVECS = GAA,    /* vecs is same as aa */
	GBB,
	GSUBD,
	GSUB2,
	NTRASH
};
#define trash   NAMEFORTRASH

static long          OpCodes[] =
{
	IEIGEN, IEIGENVALS, IRELEIGEN, IRELEIGENVALS, ITRACE
};

Symbolhandle    eigen(Symbolhandle list)
{

	/* eigen value calculations for a real symmetric  matrix */

	Symbolhandle    result = (Symbolhandle) 0, arg1,
	                arg2, symh;
	long            margs, nrows, tot, i;
	long            dim[2];
	long            aii, ierr;
	long            valsVecs;
	int             op;
	int             missingSet = 0;
	double          sum;
	double        **axH = (double **) 0, **bxH = (double **) 0;
	double        **valsH = (double **) 0, **vecsH = (double **) 0;
	double        **subdH = (double **) 0, **sub2H = (double **) 0;
	double        **aaH = (double **) 0, **bbH = (double **) 0;
	char           *compname[2];
	WHERE("eigen");
	Symbolhandle    trash = (Symbolhandle) 0;
	
	op = matchKey(FUNCNAME,OpNames,OpCodes);
	if(op == IEIGEN || op == IEIGENVALS || op == ITRACE)
	{
		margs = 1;
	}
	else
	{
		margs = 2;
	}

	OUTSTR[0] = '\0';
	if(NARGS(list) != margs)
	{
		badNargs(FUNCNAME,margs);
		goto errorExit;
	}
	arg1 = COMPVALUE(list, 0);
	if(!argOK(arg1,REAL,1))
	{
		goto errorExit;
	}
	
	if(!isMatrix(arg1,dim) || dim[0] != dim[1])
	{
		sprintf(OUTSTR,"ERROR: argument to %s not REAL square matrix",
				FUNCNAME);
	}
	else if(anyMissing(arg1))
	{
		sprintf(OUTSTR,"ERROR: 1st argument to %s contains MISSING value(s)",
				FUNCNAME);
	}

	if(*OUTSTR)
	{
		goto errorExit;
	}
	
	nrows = dim[0];
	
	axH = DATA(arg1);
	if(op == ITRACE)
	{
		double     *ax = *axH;

		sum = 0.0;
		aii = 0;
		for (i = 0; i < nrows; i++)
		{
			sum += ax[aii];
			aii += nrows + 1;
		}
#ifdef HASINFINITY
		if (isInfinite(sum))
		{
			goto overflowExit;
		}
#endif /*HASINFINITY*/

		result = RInstall(SCRATCH, 1L);
		if (result == (Symbolhandle) 0)
		{
			return (0);
		}
		DATAVALUE(result,0) = sum;
#if (USENOMISSING)
		setNOMISSING(result);
#endif /*USENOMISSING*/		
		return (result);
	} /*if(op == ITRACE)*/

/*
  operations "eigen" "eigenvals", "releigen", "releigenvals"
*/
	if(!checkSymmetry(*axH, nrows))
	{
		sprintf(OUTSTR,
				"ERROR: 1st argument to %s must be symmetric REAL matrix",
				FUNCNAME);
	}
	else if (margs == 2)
	{
		arg2 = COMPVALUE(list, 1);
		if(!argOK(arg2,REAL,2))
		{
			goto errorExit;
		}

		bxH = DATA(arg2);
		if(anyMissing(arg2))
		{
			sprintf(OUTSTR,
					"ERROR: 2nd argument to %s contains MISSING value(s)",
					FUNCNAME);
		}
		else if(!isMatrix(arg2,dim) || dim[0] != dim[1])
		{
			sprintf(OUTSTR,
					"ERROR: second argument to %s is not REAL square matrix",
					FUNCNAME);
		}
		else if(dim[0] != nrows)
		{
			sprintf(OUTSTR,"ERROR: arguments to %s not same size",FUNCNAME);
		}
		else if(!checkSymmetry(*bxH, nrows))
		{
			sprintf(OUTSTR,"ERROR: 2nd argument to %s not symmetric",FUNCNAME);
		}
	}
	if(*OUTSTR)
	{
		goto errorExit;
	}

	compname[0] = "values";
	compname[1] = "vectors";

	tot = nrows * nrows;
	trash = GarbInstall(NTRASH);
	if (trash == (Symbolhandle) 0 ||
		!getScratch(valsH,GVALS,nrows,double) ||
		!getScratch(vecsH,GVECS,tot,double) ||
		!getScratch(subdH,GSUBD,nrows,double) ||
		!getScratch(sub2H,GSUB2,nrows,double))
	{
		goto errorExit;
	}
	aaH = vecsH; /* NOTE: aaH and vecsH coincide */

	switch (op)
	{
	  case IEIGEN:
		tred2(nrows, nrows, *axH, *valsH, *subdH, *vecsH);
		checkInterrupt(errorExit);
#ifdef HASNAN
		if (anyDoubleNaN(*valsH, nrows))
		{
			goto overflowExit;
		}
#endif /*HASNAN*/

		tql2(nrows, nrows, *valsH, *subdH, *vecsH, &ierr);
		checkInterrupt(errorExit);

		if (ierr > 0)
		{
			putOutErrorMsg("ERROR: eigen algorithm did not converge");
			goto errorExit;
		}
#ifdef HASNAN
		if (anyDoubleNaN(*vecsH, nrows))
		{
			goto overflowExit;
		}
#endif /*HASNAN*/
		result = RSInstall(SCRATCH, 2L, compname, 0L);
		if (result == (Symbolhandle) 0)
		{
			goto errorExit;
		}

		symh = COMPVALUE(result, 0);
		unTrash(GVALS);
		setDATA(symh,valsH);
		setNDIMS(symh,1);
		setDIM(symh,1,nrows);

#if (USENOMISSING)
		setNOMISSING(symh);
#endif /*USENOMISSING*/
		symh = COMPVALUE(result, 1);
		unTrash(GVECS);
		setDATA(symh,vecsH);

		setNDIMS(symh,2);
		setDIM(symh,1,nrows);
		setDIM(symh,2,nrows);
#if (USENOMISSING)
		setNOMISSING(symh);
#endif /*USENOMISSING*/

		if (HASLABELS(arg1) && !moveMatLabels(arg1, symh, USEROWLABELS))
		{
			goto errorExit;
		} /*if (HASLABELS(arg1) && !moveMatLabels(arg1, symh, USEROWLABELS))*/

		discardScratch(subdH,GSUBD,double);
		discardScratch(sub2H,GSUB2,double);
		valsH = vecsH = subdH = sub2H = (double **) 0;
		
		break;

	  case IEIGENVALS:
		doubleCopy(*axH, *aaH, tot); /* copy axH to aaH */
		
		tred1(nrows, nrows, *aaH, *valsH, *subdH, *sub2H);
		checkInterrupt(errorExit);
		tqlrat(nrows, *valsH, *sub2H, &ierr);
		checkInterrupt(errorExit);

		if (ierr > 0)
		{
			sprintf(OUTSTR,"ERROR: eigen algorithm in %s did not converge",
					FUNCNAME);
			goto errorExit;
		}
#ifdef HASNAN
		if (anyDoubleNaN(*valsH, nrows))
		{
			goto overflowExit;
		}
#endif /*HASNAN*/		
		result = RInstall(SCRATCH, 0L);
		if (result == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		unTrash(GVALS);
		setDATA(result,valsH);
		setNDIMS(result,1);
		setDIM(result,1,nrows);

		break;
		
	  default:
		/* IRELEIGEN or IRELEIGENVALS */
		valsVecs = (op == IRELEIGEN) ? 1 : 0;
		
		if(!getScratch(bbH,GBB,tot,double))
		{
			goto errorExit;
		}
		
		doubleCopy(*axH, *aaH, tot); /* copy axH to aaH */
		doubleCopy(*bxH, *bbH, tot); /* copy bxH to bbH */

/* modified by kb to allow return of vectors */
		rsg(nrows, nrows, *aaH, *bbH, *valsH, valsVecs, *vecsH, *subdH, *sub2H,
			&ierr);
		checkInterrupt(errorExit);
		if (ierr > 0)
		{
			if(ierr == 7*nrows + 1)
			{
				sprintf(OUTSTR,
						"ERROR: 2nd argument to %s not positive definite",
						FUNCNAME);
			}
			else
			{
				sprintf(OUTSTR,
						"ERROR: eigen algorithm in %s did not converge",
						FUNCNAME);
			}
			goto errorExit;
		}

#ifdef HASNAN
		if (anyDoubleNaN(*valsH, nrows) ||
			valsVecs && anyDoubleNaN(*vecsH, tot))
		{
			goto overflowExit;
		}
#endif /*HASNAN*/		
		if(valsVecs)
		{
			result = RSInstall(SCRATCH, 2L, compname, 0L);
		}
		else
		{
			result = RInstall(SCRATCH, 0L);
		}
		
		if (result == (Symbolhandle) 0)
		{
			goto errorExit;
		}

		if(valsVecs)
		{
			symh = COMPVALUE(result, 1);
			unTrash(GVECS);
			setDATA(symh,vecsH);
			setNDIMS(symh,2);
			setDIM(symh,1,nrows);
			setDIM(symh,2,nrows);
			if (HASLABELS(arg1) && !moveMatLabels(arg1, symh, USEROWLABELS))
			{
				goto errorExit;
			} /*if (HASLABELS(arg1) && !moveMatLabels(arg1, symh, USEROWLABELS))*/
#if (USENOMISSING)
			setNOMISSING(symh);
#endif /*USENOMISSING*/
			symh = COMPVALUE(result, 0);
		}
		else
		{
			symh = result;
		}
		
		unTrash(GVALS);
		setDATA(symh,valsH);
		setNDIMS(symh,1);
		setDIM(symh,1,nrows);
#if (USENOMISSING)
		setNOMISSING(symh);
#endif /*USENOMISSING*/
	} /* switch (op)*/
	
	emptyTrash();

	UNLOADSEG(rsg);

	return (result);


#ifdef HASNAN
  overflowExit:
	sprintf(OUTSTR,
			"ERROR: overflow found in %s() computations", FUNCNAME);
	/*fall through*/
#endif /*HASNAN*/

  errorExit:
	putErrorOUTSTR();
	emptyTrash();
	
	UNLOADSEG(rsg);

	return (0);
	
} /*eigen()*/




