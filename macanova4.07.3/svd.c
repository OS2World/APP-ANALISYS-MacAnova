/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 4.07 or later
*(C)*
*(C)* Copyright (c) 1998 by Gary Oehlert and Christopher Bingham
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
#pragma segment Svd
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

/*
  980316 eliminated unnecessary externs, added check for overflow
  980817 Labels now propagate to output
  981112 Eliminated restriction that ncols(x) <= nrows(x).  Dimensions now
         set by Setdims()
*/

enum svdScratch
{
	GVALS = 0,
	GU,
	GV,
	GSCRATCH,
	NTRASH
};

/*
  singular value decomoposition of a real matrix

  usage:  svd(x [,left:T] [,right:T])
     or:  svd(x, all:T, [,left:F] [,right:F] [,values:F])

  If all:T is not present, values:T is implied.
  If no keywords are present, svd returns a vector of singular values in
  decreasing order.
  If only one part of the svd is requested, it is returned as a REAL
  vector (values) or matrix.  Otherwise, a structure is returned with
  components 'values', 'leftvectors', and/or 'rightvectors'
*/

Symbolhandle    svd(Symbolhandle list)
{
	Symbolhandle    result = (Symbolhandle) 0, args[4], symh;
	long            ncomps, nrows, ncols, i;
	long            dim[2], nargs = NARGS(list);
	long            ierr;
	long            values = 1, left = 0, right = 0, logicval;
	long            lengths[2];
	double        **vals = (double **) 0, **u = (double **) 0;
	double        **v = (double **) 0, **scratch = (double **) 0;
	char           *keyword;
	char           *compNames[3];
	char           *labels[2];
	char            numericLabel[2];
	WHERE("svd");
	TRASH(NTRASH,errorExit);
	
	OUTSTR[0] = '\0';

	if (nargs > 4)
	{
		badNargs(FUNCNAME,-4);
		goto errorExit;
	}
	
	for (i = 0;i < nargs;i++)
	{
		args[i] = COMPVALUE(list,i);
		if (!argOK(args[i],0, (nargs > 1) ? i+1 : 0))
		{
			goto errorExit;
		}

		if (i == 0)
		{
			if (TYPE(args[0]) != REAL || !isMatrix(args[0],dim))
			{
				sprintf(OUTSTR,"ERROR: 1st argument to %s() is not REAL matrix",
						FUNCNAME);
			}
			else if (anyMissing(args[0]))
			{
				sprintf(OUTSTR,
						"ERROR: missing values not allowed in argument to %s()",
						FUNCNAME);
			}
#if (0)  /* 981212: removed restriction that ncols < nrows */
			else if (dim[1] > dim[0])
			{
				sprintf(OUTSTR,
						"ERROR: argument to %s() has fewer rows than columns",
						FUNCNAME);
			}
#endif /*0*/
			if (*OUTSTR != '\0')
			{
				goto errorExit;
			}
		} /*if (i == 0)*/
		else
		{
			if (!(keyword = isKeyword(args[i])))
			{
				sprintf(OUTSTR,
						"ERROR: argument %ld to %s() must be a LOGICAL keyword phrase",
						i+1,FUNCNAME);
				goto errorExit;
			}
			else if (strncmp(keyword,"left", 4) != 0  &&
					strncmp(keyword,"right", 5) != 0 &&
					strncmp(keyword,"val", 3) != 0 &&
					strncmp(keyword,"all", 3) != 0)
			{
				badKeyword(FUNCNAME, keyword);
				goto errorExit;
			}
			else if (!isTorF(args[i]))
			{
				notTorF(keyword);
				goto errorExit;
			}

			logicval = (DATAVALUE(args[i],0) != 0.0) ? 1 : 0;

			if (strncmp(keyword, "all", 3) == 0)
			{
				if (!logicval)
				{
					sprintf(OUTSTR,
							"ERROR: 'all:F' is not legal for %s()", FUNCNAME);
					goto errorExit;
				}
				values = left = right = 1;
			}
			else if (strncmp(keyword,"left", 4) == 0)
			{
				left = logicval;
			}
			else if (strncmp(keyword,"right", 5) == 0)
			{
				right = logicval;
			}
			else
			{ /* must be 'values'*/
				values = logicval;
			}
		} /*if (i == 0){}else{}*/
	} /*for (i = 0;i < nargs;i++)*/

	nrows = dim[0];
	ncols = dim[1];
		
	ncomps = 0;
	if (values)
	{
		compNames[ncomps++] = "values";
	}
	if (left)
	{
		compNames[ncomps++] = "leftvectors";
	}
	if (right)
	{
		compNames[ncomps++] = "rightvectors";
	}
	if (ncomps == 0)
	{
		sprintf(OUTSTR,
				"ERROR: nothing specified for %s() to compute", FUNCNAME);
		goto errorExit;
	}

	/*
	   vals is a handle for singular values (always needed)
	   u is a handle for left singular vectors (always needed)
	   v is a handle for right singular vectors (only needed if right:T)
	   scratch is a handle for working scratch space
	*/
	if (!getScratch(vals,GVALS,ncols,double) ||
		!getScratch(u,GU,nrows*ncols,double) ||
		right && !getScratch(v,GV,ncols * ncols, double) ||
		!getScratch(scratch,GSCRATCH,ncols,double))
	{
		goto errorExit;
	}

	eispsvd(nrows,nrows,ncols,DATAPTR(args[0]),*vals, left, *u, right,
			(right) ? *v : (double *) 0, *scratch, &ierr);

	if (ierr > 0)
	{
		sprintf(OUTSTR,
				"ERROR: singular value algorithm in %s() did not converge",
				FUNCNAME);
		goto errorExit;
	}

	
	if (ncomps == 1)
	{
		if ((result = Install(SCRATCH,REAL)) == (Symbolhandle) 0)
		{
			goto errorExit;
		}
	} /*if (ncomps == 1)*/
	else
	{
		result = RSInstall(SCRATCH,ncomps,compNames,0L);
		if (result == (Symbolhandle) 0)
		{
			goto errorExit;
		}
	} /*if (ncomps == 1){}else{}*/

	if (HASLABELS(args[0]))
	{
		numericLabel[0] = NUMERICLABEL;
		numericLabel[1] = '\0';
	}
	
	for (i = 0;i < ncomps;i++)
	{
		keyword = compNames[i];
		symh = (ncomps == 1) ? result : COMPVALUE(result,i);
		
#ifdef HASNAN
		if (anyNaN(symh))
		{
			sprintf(OUTSTR,
					"ERROR: computation by %s() produced overflow", FUNCNAME);
			Removesymbol(result);
			goto errorExit;
		}
#endif /*HASNAN*/

		if (ncomps > 1)
		{
			setNAME(symh,keyword);
		}
		
		if (strcmp(keyword, "values") == 0)
		{
			unTrash(GVALS);
			setDATA(symh,vals);
			Setdims(symh, 1, &ncols);
			if (ncols > nrows)
			{
				doubleFill(*vals + nrows, 0.0, ncols - nrows);
			}
		}
		else if (strncmp(keyword, "left", 4) == 0)
		{
			unTrash(GU);
			setDATA(symh,u);
			Setdims(symh, 2, dim);
			if (ncols > nrows)
			{
				doubleFill(*u + nrows*nrows, 0.0, (ncols - nrows)*nrows);
			}
			
			if (HASLABELS(args[0]))
			{
				getMatLabels(args[0], labels, lengths);
				lengths[1] = expandLabels(DEFAULTLEFTBRACKET, ncols,
										  (char *) 0);
				TMPHANDLE = createLabels(2, lengths);
				if (TMPHANDLE == (char **) 0 || !setLabels(symh, TMPHANDLE))
				{
					goto errorExit;
				}
				getMatLabels(args[0], labels, lengths);
				appendLabels(symh, labels[0], 0, dontExpand);
				appendLabels(symh, DEFAULTLEFTBRACKET, 1, doExpand);
			}
		}
		else if (strncmp(keyword, "right", 5) == 0)
		{
			unTrash(GV);
			setDATA(symh,v);
			dim[0] = ncols;
			Setdims(symh, 2, dim);

			if (HASLABELS(args[0]))
			{
				getMatLabels(args[0], labels, lengths);
				lengths[0] = lengths[1];
				lengths[1] = expandLabels(DEFAULTLEFTBRACKET, ncols,
										  (char *) 0);
				TMPHANDLE = createLabels(2, lengths);
				if (TMPHANDLE == (char **) 0 || !setLabels(symh, TMPHANDLE))
				{
					goto errorExit;
				}
				getMatLabels(args[0], labels, lengths);
				appendLabels(symh, labels[1], 0, dontExpand);
				appendLabels(symh, DEFAULTLEFTBRACKET, 1, doExpand);
			} /*if (HASLABELS(args[0]))*/
		}
#if (USENOMISSING)
		setNOMISSING(symh);
#endif /*USENOMISSING*/
	} /*for (i = 0;i<ncomps;i++)*/	

	emptyTrash();

	return (result);

  errorExit:
	putOUTSTR();

	emptyTrash();

	return (0);
	
} /*svd()*/


