/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 4.06 or later
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
#pragma segment Cellstat
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

#define x(i,j) (*X[j])[i]	/* i is case, j is variable */

static long cellNumber(long caseno, long term[])
{
	long        cellno = 0, j, level;
	long        nvars = NVARS;
	
	for (j = 0; j < nvars; j++)
	{
		level = x(caseno, j);
		cellno += term[j] * (level - 1);	/* find cell number */
	} /*for (j = 0; j < nvars; j++)*/
	return (cellno);
} /*cellNumber()*/

/*  routine to compute cell by cell stats for a particular model term
    used for description or sweeping

	curyy     handle to response vector
	X         array of handles to class variables
	NVARS     number of variables (columns of x)
	NCLASSES  number of classes in each variable
	term      particular model term to fit    term is 1 or 0 as a particular
	          variable is included or excluded from the term
              CAUTION: term is modified and must be reinitialized
	tmeans    handle for vector of means
	tvars     handle for vector of variances
	tcounts   handle for vector of counts
	op        opcode
	  CELLSUMS   compute cell sums only.  It works even if cell sizes
	      differ, but assumes that there is no missing DATA.  It is currently
		  used only by ipf().
	  CELLMEANSANDSWEEP compute cell means, sweep out from y, and return RSS
	      It requires equal cell sizes
	  CELLCOUNTS compute cell counts only; it requires a model be defined and
	      omits all cases for which there are any MISSING values.
	  CELLSTATS  compute means, variances, counts.  Does not require equal
	      cell sizes but requires a model be defined and omits all cases for
		  which there are any MISSING values.  It is used only in cellstats()
	NOTE: tvars not referenced if op != CELLSTATS
	      tmeans is not reference if op == CELLCOUNTS
	NOTE: CELLMEANSANDSWEEP and CELLSUMS share the code for computing cell
		  sums; otherwise the operations are done by different code.
*/


double          cellstat(double ** cury, long term [], double ** tmeans,
						 double ** tvars, double ** tcounts, long op)
{
	long            i, j, k, icellk, ik, ncells, icell;
	long            ndata = (long) NDATA, nvars = (long) NVARS, ny = (long) NY;
	double          comct, curss = 0.0, d, sumabs;
	double         *means, *counts, *vars;
	double         *y = *cury, *misswts;
	WHERE("cellstat");
	
	if(tvars != (double **) 0)
	{
		vars  = *tvars;
	}
	if(tmeans != (double **) 0)
	{
		means  = *tmeans;
	}
	if(tcounts != (double **) 0)
	{
		counts  = *tcounts;
	}
	if(MODELTYPE & MISSWEIGHTS)
	{
		misswts = *MISSWTS;
	}

	ncells = 1;
	for (j = 0; j < nvars; j++)
	{
		term[j] *= ncells;
		if (term[j] > 0)
		{
			ncells *= NCLASSES[j];
		}
	} /*for (j = 0; j < nvars; j++)*/

	/*
	   ncells contains the number of means to be computed
	   term now contains the offset for indexing into array of means
   */

	if (op == CELLMEANSANDSWEEP || op == CELLSUMS)
	{/* just do for balanced anova */
		doubleFill(means, 0.0, ncells);

		sumabs = 0;
		for (i = 0; i < ndata; i++)
		{
			icell = cellNumber(i, term);
			means[icell] += y[i];	/* increment sum  */
			sumabs += fabs(y[i]);
		} /*for (i = 0; i < ndata; i++)*/
		/* at this point, means[] contains the cell totals */

		if (op == CELLMEANSANDSWEEP)
		{
			/*
			   Compute the cell means, subtract them from all data in
			   each cell, and compute the sum of squares of deviations.
			   This code assumes balance among the cells
			*/
			comct = NDATA / (double) ncells;
			for (icell = 0; icell < ncells; icell++)
			{ /* compute means */
				if (fabs(means[icell]) <= 1e-15*sumabs)
				{
					means[icell] = 0.0;
				}
				else
				{
					means[icell] /= comct;
				}
			} /*for (icell = 0; icell < ncells; icell++)*/
	
			for (i = 0; i < ndata; i++)
			{ /* sweep out current model from y annd compute RSS */
				icell = cellNumber(i, term);
				y[i] -= means[icell];
				curss += y[i] * y[i];
			} /*for (i = 0; i < ndata; i++)*/
		} /*if (op == CELLMEANSANDSWEEP)*/
	} /*if (op == CELLMEANSANDSWEEP || op == CELLSUMS)*/
	else if (op == CELLCOUNTS)
	{
		/* just get counts in each cell */
		doubleFill(counts, 0.0, ncells);
		for (i = 0; i < ndata; i++)
		{
			if (!(MODELTYPE & MISSWEIGHTS) || misswts[i] != 0.0)
			{
				icell = cellNumber(i, term);
				counts[icell]++; /* increment count */
			} /*if (!(MODELTYPE & MISSWEIGHTS) || misswts[i] != 0.0)*/		
		} /*for (i = 0; i < ndata; i++)*/
	} /*else if (op == CELLCOUNTS)*/
	else
	{ /* CELLSTATS */
		/* compute cell counts, means and variances */
		doubleFill(vars, 0.0, ncells * ny);
		doubleFill(means, 0.0, ncells * ny);
		doubleFill(counts, 0.0, ncells * ny);
	
		for (i = 0; i < ndata; i++)
		{
			if (!(MODELTYPE & MISSWEIGHTS) || (*MISSWTS)[i] != 0.0)
			{	
				icellk = cellNumber(i, term);  /* cellNo + k*ncells */
				ik = i; /* i + k * ndata */
				for (k = 0; k < ny; k++)
				{
					counts[icellk]++; /* increment count */
					means[icellk] += y[ik]; /* increment sum  */
					icellk += ncells;
					ik += ndata;
				} /*for (k = 0; k < ny; k++)*/
			} /*if (!(MODELTYPE & MISSWEIGHTS) || (*MISSWTS)[i] != 0.0)*/	
		} /*for (i = 0; i < ndata; i++)*/
	
		for (icell = 0; icell < ncells; icell++)
		{
			icellk = icell; /* icell + k * ncells */
			for (k = 0; k < ny; k++)
			{
				if (counts[icellk] > 0.0)
				{
					means[icellk] /= counts[icellk];
				}
				icellk += ncells;
			}
		} /*for (icell = 0; icell < ncells; icell++)*/
	
		for (i = 0; i < ndata; i++)
		{
			if (!(MODELTYPE & MISSWEIGHTS) || (*MISSWTS)[i] != 0.0)
			{	
				icellk = cellNumber(i, term);  /* cellNo + k * ncells */
				ik = i; /* i + k*ndata */
				for (k = 0; k < ny; k++)
				{/* get variance*/
					d = y[ik] - means[icellk];
					vars[icellk] += d*d;
					icellk += ncells;
					ik += ndata;
				} /*for (k = 0; k < ny; k++)*/
			} /*if (!(MODELTYPE & MISSWEIGHTS) || (*MISSWTS)[i] != 0.0)*/
		} /*for (i = 0; i < ndata; i++)*/
	
		for (icell = 0; icell < ncells; icell++)
		{
			icellk = icell; /* icell + k * ncells */
			for (k = 0; k < ny; k++)
			{
				if (counts[icellk] > 1.0)
				{
					vars[icellk] /= counts[icellk] - 1.0;
				}
				icellk += ncells;
			} /*for (k = 0; k < ny; k++)*/
		} /*for (icell = 0; icell < ncells; icell++)*/
	} /*else if (op == CELLSTATS)*/
	return (curss);
} /*cellstat()*/
