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
#pragma segment Funbalan
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

/*
  990215 changed myerrorout() to putOutErrorMsg()
  990226 replaced some useages of putOUTSTR() by putErrorOUTSTR()
*/
#include "globals.h"

static long            getcellindex(long term[], long k)
{
	long            indices[MAXVARS], i, j;
	long            nvars = (long) NVARS;
	
	for (i = 0; i < nvars; i++)
	{
		indices[i] = k % NCLASSES[i];
		k /= NCLASSES[i];
	} /*for (i = 0; i < nvars; i++)*/
	j = 1;
	k = 0;
	for (i = 0; i < nvars; i++)
	{
		if (term[i] > 0)
		{
			k += j * indices[i];
			j *= NCLASSES[i];
		}
	} /*for (i = 0; i < nvars; i++)*/
	return (k);
} /*getcellindex()*/

/*
	Fast iterative routine for computing ss and df for unbalanced designs
	Uses EM algorithm
	
	
	RESIDUALS	handle to response vector (which will be changed)
	Y       handle to response variable (will be changed)
	X		array of handles to class variables
	NVARS	numberof variables (columns of x)
	NCLASSES	number of classes in each variable
	NDATA	number of data values (will be changed)
	MODEL	handle giving terms in order to be used in model
		term i contains the variables in the binary expansion of i
		0 is constant; 3 is AB; 5 is AC etc
	SS	handle to term sums of squares
	DF	handle of term df

	980318 At some time in the past, kb made changes in the way degrees of
	freedom were computed.  No comments or notes were made, but his
	recollection is that it was in response to incorrect computation with
	certain patterns of missing cells;  in fact, this is the only issue that
	the computation addressed.  Unfortunately, this made incorrect the
	computation of degrees of freedom in certain situations when there
	were no missing cells, such as for a model "y = a + b - 1".  Accordingly
	the old way is reinstated pending time to find improvements.  Generally,
	the problem of always correctly determining degrees of freedom for all
	terms when there are empty cells is very difficult, if not impossible.
*/

#define OLDWAY  /*causes compilation of old method */

enum fastScratch
{
	GCTS = 0,
	GMNS,
	GRESID,
	NTRASH
};


long funbalanova(long maxiter,double epsilon)
{
	Symbolhandle    symh;
	long            i, j, k, m, n, i2, termmax, ncells, npseudo, term[MAXVARS];
	long            maxcts, nEmpty = 0;
	long            nvars = (long) NVARS, ndata = (long) NDATA, ndataOrig;
	long            nterms = (long) NTERMS, notmissing = (long) NOTMISSING;
	long            returnValue = ITERERROR;
	double        **ctsH = (double **) 0, **mnsH = (double **) 0;
	double        **residH = (double **) 0;
	double        **varsH = (double **) 0; /* used only as dummy argument */
	double         *cts, *mns;
	double         *resid, *df, *ss, *y, *misswts, *xj;
	double          curss, newss, oldss, ybar;
	long            constantIsFirst = 1;
	WHERE("funbalanova");
	TRASH(NTRASH,errorExit); /* room for up to 3 scratch */
	
	epsilon *= epsilon; /* modification 950303 */
	ncells = 1;
	for (i = 0; i < nvars; i++)
	{
		ncells *= NCLASSES[i];
	}

	if(!getScratch(ctsH,GCTS,ncells,double) ||
	   !getScratch(mnsH,GMNS,ncells,double))
	{
		goto errorExit;
	}
	cts = *ctsH;
	mns = *mnsH;
	df = *DF;
	y = *Y;
	misswts = (MISSWTS != (double **) 0) ? *MISSWTS : (double *) 0;

#if (0)
	/*
	  following is essentially the original code from MacAnova2.43
	  It is the basis for the OLDWAY code below.
	*/
	{
		/* compute df's */
		int         i, i2, j, k, warn;
		
		for(i=0;i<nvars;i++) 
		{
			term[i] = 1;
		}
		for(i=0;i<nterms;i++) 
		{
			i2 = **modelterm(MODEL,i);
			df[i] = 1;
			for(k=0;k<nvars;k++) 
			{
				if( i2 & 1 ) 
				{
					df[i] *= NCLASSES[k];
				}
				i2 = i2 >> 1;
			}
		}

		/* DF now has maximum possible df, go through and take out of higher terms */

		for(i=0;i<nterms-1;i++) 
		{
			i2 = **modelterm(MODEL,i);
			for(j=i+1;j<nterms;j++) 
			{
				k = **modelterm(MODEL,j);

				if(!( ( k & i2) ^ i2 ) ) 
				{  /* i2 included in k */
					df[j] -= df[i];
				}
			}
		}
		warn = 1;
		(void) cellstat(Y, term, (double **) 0, (double **) 0, ctsH,
						CELLCOUNTS);
#if (0)
		cellstat(Y,term,mns,vars,cts,(long)2);  /* gets cell counts into counts */
#endif
		for(i=0;i<nterms;i++)
		{
			getterm(modelterm(MODEL,i),term, nvars);
			/* set up cell counter for term */
			termmax = 1;
			for(j=0;j<nvars;j++)
			{
				if(term[j] > 0)
				{
					termmax *= (long) NCLASSES[j];
				}
			}
			for(j=0;j<termmax;j++)
			{
				mns[j] = 0.;
			}
			/* go through all  previously used cells and mark locations
			   in current term counter */
			for(k=0;k<ncells;k++) 
			{
				if(cts[k] < 0) 
				{
					mns[getcellindex(term,k)] = -1.;
				}
			}
			/* go through all unused cells and see if they can be used in
			   new term.  Cumulate the number added in */
			j = 0;
			for(k=0;k<ncells;k++) 
			{
				if( cts[k] > 0 && mns[getcellindex(term,k)] == 0.) 
				{
					mns[getcellindex(term,k)] = -1;
					cts[k] *= -1.;
					j++;
				}
			}
			/* check for empty cells */
			if(j < df[i] && warn) 
			{
				warn = 0;
				putOutErrorMsg("WARNING: empty cell; anovacoefs() will not work properly");
			}
			df[i] = j;
		} /*for(i=0;i<nterms;i++)*/
	}
#endif /*0*/ /* end of original code*/

#if (0)
 /* correct way to compute maximum DF's*/
	/* compute df's */
	{
		long       nregx = 0;
		
		for (i = 0; i < nterms; i++)
		{			/* for each term */
			df[i] = buildXmatrix(i, (double *) 0, MODELINFO);
			nregx += (long) df[i];
		}
	}
#endif /*0*/

#if (0)
/*
  old way to compute max df's; didn't work in some cases
  They're not used so omit
*/
	for (i = 0; i < nterms; i++)
	{
		df[i] = 1;
		for (k = 0; k < nvars; k++)
		{
			if(inTerm(k,modelterm(MODEL,i)))
			{
				df[i] *= (double) NCLASSES[k];
			}
		}/*for (k = 0; k < nvars; k++)*/
	} /*for (i = 0; i < nterms; i++)*/

	PRINT("df[0-3] = %g %g %g %g\n",df[0],df[1],df[2],df[3]);

	/* DF now has maximum possible df, go through and take out of higher terms */

	for (i = 0; i < nterms - 1; i++)
	{
		modelPointer    iterm = modelterm(MODEL,i);	/* terms in model */

		for (j = i + 1; j < nterms; j++)
		{
			if(inEffectiveTerm(modelterm(MODEL,j),iterm))
			{	/* iterm included in in term j */
				df[j] -= df[i];
			}
		}
	} /*for (i = 0; i < nterms - 1; i++)*/

#endif /*0*/	

	/* at this point, DF has maximum possible, but there may be missing cells
	   now go through and attempt to reduce DF for missing cells.
	   The current algorithm does not work properly in certain cases, e.g.
	   the following cell pattern (O = Empty, X = not empty)
	       X   X   O   O   O
	       X   X   O   O   O
	       O   O   X   X   X
	       O   O   X   X   X
	       O   O   X   X   X

       980318 This comment specifically refers to the new algorithm which
	   has been disabled.  However, it still does not do a good job with
	   patterns of missing cells.
       
	*/
	
	/*
	  make a structure for used cells, cts tells us what has been
	  used up till now, and mns will be for the current model term
	*/

	/* compute cell counts into cts */
	for (i = 0; i < nvars; i++)
	{
		term[i] = 1;
	}

	/* Compute the number of complete data cases in each cell */
	(void) cellstat(Y, term, (double **) 0, (double **) 0, ctsH, CELLCOUNTS);

	for (k = 0; k < ncells; k++)
	{ /* count empty cells */
		if (cts[k] == 0)
		{
			nEmpty++;
		}
	}
	if (nEmpty == 0)
	{
	/* compute df's */
		long       nregx = 0;
		
		for (i = 0; i < nterms; i++)
		{			/* for each term */
			df[i] = buildXmatrix(i, (double *) 0, MODELINFO);
			nregx += (long) df[i];
		}
	} /*if (nEmpty == 0)*/
	else
	{
		if(PRINTWARNINGS)
		{
			sprintf(OUTSTR,
					"WARNING: There %s %ld empty cell%s; coefs() may give wrong answers",
					(nEmpty == 1) ? "is" : "are", nEmpty,
					(nEmpty == 1) ? "" : "s");
			putOUTSTR();
			putOutErrorMsg("         and the degrees of freedom may be in error");
		} /*if(nEmpty > 0 && PRINTWARNINGS)*/

#ifdef OLDWAY
		for (i = 0; i < nterms; i++)
		{
			long          dfi = 0.0;

			/* set term to binary expansion of i-th term */
			getterm(modelterm(MODEL,i), term, nvars);
			/* set up cell counter for term */
			termmax = 1;
			for (j = 0; j < nvars; j++)
			{
				if (term[j] > 0)
				{
					termmax *= NCLASSES[j];
				}
			} /*for (j = 0; j < nvars; j++)*/

			if (i == 0 && termmax != 1)
			{
				constantIsFirst = 0;
			}

			/* termmax is size of associated marginal table */
			doubleFill(mns, 0.0, termmax);
			/*
			  go through all  previously used cells and mark locations
			  in current term counter
			*/
			if (i > 0)
			{
				for (k = 0; k < ncells; k++)
				{
					if (cts[k] < 0)
					{
						mns[getcellindex(term, k)] = -1.0;
					}
				} /*for (k = 0; k < ncells; k++)*/
			} /*if (i > 0)*/

			/*
			  go through all "unused" cells and see if they can be used in
			  new term.  Cumulate the number added in
			  mns[m] is set to -1 if the m-th marginal cell is not empty
			*/
			for (k = 0; k < ncells; k++)
			{
				m = getcellindex(term, k);
				if (cts[k] > 0 && mns[m] == 0.0)
				{
					mns[m] = cts[k] = -1.0;
					dfi++;
				}
			} /*for (k = 0; k < ncells; k++)*/

			df[i] = dfi;
		} /*for (i = 0; i < nterms; i++)*/
#else /*OLDWAY*/
		for (i = 0; i < nterms; i++)
		{
			long        dfi = (long) df[i];

			/* set term to binary expansion of i-th term */
			getterm(modelterm(MODEL,i), term, nvars);

			/* set up cell counter for term */
			termmax = 1;
			for (j = 0; j < nvars; j++)
			{
				if (term[j] > 0)
				{
					termmax *= NCLASSES[j];
				}
			}
			if (i == 0 && termmax != 1)
			{
				constantIsFirst = 0;
			}

			/* termmax is size of associated marginal table */
			doubleFill(mns, 0.0, termmax);

			/*
			  go through all  previously used cells and mark locations
			  in current term counter
			*/
			if (i > 0)
			{
				for (k = 0; k < ncells; k++)
				{
					if (cts[k] < 0)
					{
						mns[getcellindex(term, k)] = -1.0;
					}
				} /*for (k = 0; k < ncells; k++)*/
			} /*if (i > 0)*/
			/*
			  go through all "unused" cells and see if they can be used in
			  new term.  Cumulate the number added in
			  mns[m] is set to -1 if the m-th marginal cell is not empty
			  It is set to k+1 if the cell is empty, k = index of first cell of
			  table associated with m-th marginal cell.  Cell k will be marked as
			  "used up" if m-th cell is empty.
			*/
			for (k = 0; k < ncells; k++)
			{
				if ((mns[m = getcellindex(term, k)] >= 0.0))
				{
					if (cts[k] > 0)
					{
						if(mns[m] == 0.0)
						{
							cts[k] = -1.;
						}
						mns[m] = -1;
					}
					else if(cts[k] == 0.0 && mns[m] == 0.0)
					{ /* mark this level as including empty cell */
						mns[m] = (double) (k + 1);
					}
				} /*if ((mns[m = getcellindex(term, k)] >= 0.))*/
			} /*for (k = 0; k < ncells; k++)*/

			for (j = 0; j < termmax; j++)
			{
				if((m = (long) mns[j]) > 0)
				{
					dfi--; /* deduct d.f. */
					cts[m - 1] = -1.0; /* mark cell as used */
				}
			} /*for (j = 0; j < termmax; j++)*/
			if(dfi < 0)
			{
				dfi = 0;
			}

			df[i] = dfi;
		} /*for (i = 0; i < nterms; i++)*/
#endif /*OLDWAY*/
	} /*if (nEmpty == 0){}else{}*/

	/* compute DF for error */
	df[nterms] = NOTMISSING;
	for (i = 0; i < nterms; i++)
	{
		df[nterms] -= df[i];
	}
	if (df[nterms] < 0)
	{
		df[nterms] = 0;
	}
	
	/* find out how many pseudo responses are needed and set them up */

	/* recompute cell counts into cts */
	for (i = 0; i < nvars; i++)
	{
		term[i] = 1;
	} /*for (i = 0; i < nvars; i++)*/

	(void) cellstat(Y, term, (double **) 0, (double **) 0, ctsH, CELLCOUNTS);

	maxcts = 0; /* max(cts[i],i=1,ncells) */
	for (i = 0; i < ncells; i++)
	{
		if (cts[i] > maxcts)
		{
			maxcts = cts[i];
		}
	} /*for (i = 0; i < ncells; i++)*/

	ndataOrig = ndata;
	ndata = maxcts * ncells;
	NDATA = (double) ndata;
	npseudo = ndata - notmissing;

	/*  squeeze out cases with missing data  */
	if(notmissing != ndataOrig)
	{
		k = 0;
		for (i = 0; i < ndataOrig; i++)
		{
			if (misswts[i] != 0.0)
			{ /* y and x values not missing */
				y[k++] = y[i];
			} /*if (!isMissing(y[i]))*/
		} /* for (i = 0; i < ndataOrig; i++) */

		for (j = 0;j < nvars; j++)
		{
			xj = *(X[j]);
			k = 0;
			for (i = 0; i < ndataOrig; i++)
			{
				if (misswts[i] != 0.0)
				{/* y and x values not missing */
					xj[k++] = xj[i];
				} /*if (misswts[i] != 0.0)*/
			} /* for (i = 0; i < ndataOrig; i++) */
		} /*for (j = 0;j < nvars; j++)*/		
	} /*if(notmissing != ndataOrig)*/

	 /* change length of variables */
	if (ndata != ndataOrig)
	{
		/* increase the length of variables as if it were balanced */
		Y = (double **) mygrowhandle((char **) Y,ndata * sizeof(double));

		/* Y should always be identical with DATA(MODELVARS[0]) */
		symh = MODELVARS[0];
		setDATA(symh,Y);
		if(Y == (double **) 0)
		{
			goto errorExit;
		}
		setDIM(symh,1,ndata);

		for (j = 0; j < nvars; j++)
		{
			X[j] = (double **) mygrowhandle((char **) X[j],
											ndata * sizeof(double));
			/* X[j] should always be identical with DATA(MODELVARS[j+1]) */
			symh = MODELVARS[j+1];
			setDATA(symh,X[j]);
			if(X[j] == (double **) 0)
			{
				goto errorExit;
			}
			setDIM(symh,1,ndata);
		} /*for (j = 0; j < nvars; j++)*/
		y = *Y;
	} /*if (ndata != ndataOrig)*/	

	ybar = 0.0;
	for(i=0;i<notmissing;i++)
	{
		ybar += y[i];
	}
#ifdef HASINFINITY
	if (isInfinite(ybar))
	{
		goto overflowExit;
	}
#endif /*HASINFINITY*/
	ybar /= NOTMISSING;

	if (npseudo != 0)
	{/* add pseudo responses, if any, at end */
		k = notmissing;
		for (i = 0; i < nvars; i++)
		{ /* start in cell (1,1,...,1) */
			term[i] = 1;
		}
		for (i = 0; i < ncells; i++)
		{
			i2 = maxcts - cts[i];
			for (n = 0; n < i2; n++)
			{
				y[k] = ybar;
				for (m = 0; m < nvars; m++)
				{
					(*(X[m]))[k] = term[m];
				}
				k++;
			}
			/* step factor levels */
			term[0]++;
			for (j = 1; j < nvars; j++)
			{
				if (term[j - 1] > NCLASSES[j - 1])
				{
					term[j - 1] = 1;
					term[j]++;
				}
			}
		} /*for (i = 0; i < ncells; i++)*/
	} /*if(npseudo != 0)*/
	/* now go through terms in model getting residual ss for each term
	   and sweeping out effects */

	curss = 0.0;
	if (constantIsFirst)
	{
		(*SS)[0] = (double) notmissing * ybar * ybar;
		for (i = 0; i < notmissing; i++)
		{
			y[i] -= ybar;
			curss += y[i]*y[i];
		}
		for (i = notmissing; i < ndata; i++)
		{
			y[i] = 0.0;
		}
	}
	else
	{
		for (i = 0; i < notmissing; i++)
		{
			curss += y[i] * y[i];	/*  get total SS */
		}
	}

#ifdef HASINFINITY
	if (isInfinite(curss))
	{
		goto overflowExit;
	}
#endif /*HASINFINITY*/

	if(!getScratch(residH,GRESID,ndata,double))
	{
		goto errorExit;
	}
	
	/* re-dereference handles*/
	y = *Y;
	resid = *residH;
	ss = *SS;
	df = *DF;

	for (i = constantIsFirst; i < nterms; i++)
	{
		if (df[i] == 0)
		{
			ss[i] = 0.0;
		} /*if (df[i] == 0)*/
		else
		{
			oldss = 0.0;
			doubleCopy(y, resid, ndata);
			for (k = 0; k < maxiter; k++)
			{
				if (interrupted(DEFAULTTICKS) != INTNOTSET)
				{
					goto errorExit;
				}
				for (j = 0; j <= i; j++)
				{
					/* set binary expansion in term */
					getterm(modelterm(MODEL,j), term, nvars);
					/* sweep cell means from current residuals, compute new RSS */
					newss = cellstat(residH, term, mnsH, varsH, ctsH,
									 CELLMEANSANDSWEEP);
#ifdef HASINFINITY
					if (isInfinite(newss))
					{
						goto overflowExit;
					}
#endif /*HASINFINITY*/
				}

				if (fabs(oldss - newss) <= epsilon * (oldss + newss))
				{
					break;
				}
				oldss = newss;
				if (k < maxiter - 1)
				{
					doubleCopy(y, resid, ndata - npseudo);
					/* set pseudo obs to the current fitted values */
					for(j = ndata - npseudo;j < ndata;j++)
					{
						resid[j] = y[j] - resid[j];
						y[j] = resid[j];
					}
				} /*if (k < maxiter - 1)*/
			} /*for (k = 0; k < maxiter; k++)*/

			/* set pseudo obs to the final fitted values */
			for(j = ndata - npseudo;j < ndata;j++)
			{
				y[j] -= resid[j];
			}
			if(k == maxiter)
			{
				returnValue = ITERUNCONVERGED;
			}
		
			ss[i] = curss - newss;	/* set ss for term */
			curss = newss;
		} /*if (df[i] == 0){}else{}*/
	} /*for (i = constantIsFirst; i < nterms; i++)*/

	ss[nterms] = curss;	/* error ss */

	unTrash(GRESID); /* residH is a keeper */
	RESIDUALS = residH;
	returnValue = (returnValue != 0) ? returnValue : ITERCONVERGED;
	/* fall through */

  errorExit:
	*OUTSTR = '\0';
	emptyTrash();	/* dispose of scratch space */

	return (returnValue);

#ifdef HASINFINITY
  overflowExit:
	sprintf(OUTSTR,
			"ERROR: overflow found in %s() computations", FUNCNAME);
	putErrorOUTSTR();
	emptyTrash();
	return(ITERERROR);
#endif /*HASINFINITY*/
} /*funbalanova()*/

