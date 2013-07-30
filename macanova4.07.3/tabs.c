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
#pragma segment Tabs
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

#define coord(I,J) (classVars[J][I])

enum tabCodes
{
	IMEAN  = 0x01,
	IVAR   = 0x02,
	ICOUNT = 0x04
};

enum doitCodes
{
	JMEAN  = 0,
	JVAR,
	JCOUNT
};


/*
  Modified by kb to allow for multivariate data

  971212 changed argument list to celltabs to avoid need for global static
         variables
         tabs(,a,b,...,count:T) is now ok
*/

static int      celltabs(Symbolhandle list, long nvars,
						 double **tmeans, double **tvars, double **tcounts,
						 unsigned long control)
{
	Symbolhandle    symh;
	long            i, j, k, ncells, icell, level;
	long            ivar;
	long            term[MAXDIMS];
	long            needVars = (control & IVAR);
	long            needMeans = (control & (IMEAN | IVAR));
	long            length;
	double         *classVars[MAXDIMS];
	long            nclass[MAXDIMS], types[MAXDIMS];
	short           nmissing = 0;
	double          d,yy;
	double         *y;
	long            ndata, ny, dim[2];
	int             foundOverflow = 0;
	double         *vars = (double *) 0, *means = (double *) 0, *counts;

	symh = COMPVALUE(list, 0);

	if (symh == (Symbolhandle) 0 || isNull(symh))
	{
		y = (double *) 0;
		ndata = symbolSize(COMPVALUE(list, 1));
		ny = 1;
	}
	else
	{
		(void) isMatrix(symh, dim);
		y = DATAPTR(symh);
		ndata = dim[0];
		ny = dim[1];
	}

	ncells = 1;
	for (i = 0; i < nvars; i++)
	{
		symh = COMPVALUE(list, i+1);

		term[i] = ncells;
		nclass[i] = labs(isFactor(symh));
		classVars[i] = DATAPTR(symh);
		types[i] = TYPE(symh);
		ncells *= (long) nclass[i];
	} /*for (i = 0; i < nvars; i++)*/
	length = ncells * ny;
	/*
	  ncells is the number of means to be computed
	  term contains the offsets for indexing into array of means
	*/

	counts = *tcounts;
	doubleFill(counts, 0.0, length);
	if (needVars)
	{
		vars = *tvars;
		doubleFill(vars, 0.0, length);
	}
	if (needMeans)
	{
		means = *tmeans;
		doubleFill(means, 0.0, length);
	}
	
	for (ivar = 0;ivar < ny;ivar++)
	{
		for (i = 0; i < ndata; i++)
		{
			yy = (y != (double *) 0) ? y[i] : 1.0;
			if (!isMissing(yy))
			{
				for (j = 0; j < nvars; j++)
				{
					if (isMissing(coord(i,j)))
					{
						nmissing++;
						break;
					}
				}
				if (j == nvars)
				{
					icell = 0;
					for (j = 0; j < nvars; j++)
					{
						level = (types[j] == REAL) ? (long) coord(i,j) :
							((coord(i,j) != 0.0)) ? 2 : 1;
						icell += term[j] * (level-1);	/* find cell number */
					}
					counts[icell]++; /* increment count */
					if (needMeans)
					{
						means[icell] += yy; /* increment sum  */
					}
				} /*if (j == nvars)*/
			} /*if (!isMissing(yy))*/
		} /*for (i = 0; i < ndata; i++)*/

		if (needMeans)
		{
			for (icell = 0; icell < ncells; icell++)
			{
#ifdef HASINFINITY
				if (isInfinite(means[icell]))
				{
					setMissing(means[icell]);
					foundOverflow = 1;
				}
				else
#endif /*HASINFINITY*/
				if (counts[icell] > 0.0)
				{
					means[icell] /= counts[icell];
				}
			}
		} /*if (needMeans)*/
		
		if (needVars)
		{
			for (i = 0; i < ndata; i++)
			{
				yy = y[i];
				if (!isMissing(yy))
				{
					if (nmissing)
					{
						for (j = 0; j < nvars; j++)
						{
							if (isMissing(coord(i,j)))
							{
								nmissing--;
								break;
							}
						} /*for (j = 0; j < nvars; j++)*/
					} /*if (nmissing)*/
					else
					{
						j = nvars;
					}
					
					if (j == nvars)
					{
						icell = 0;
						for (j = 0; j < nvars; j++)
						{
							k = (types[j] == REAL) ? ((long) coord(i,j) - 1) :
							((coord(i,j) != 0.0)) ? 1 : 0;
							icell += term[j] * k; /* find cell number */
						}
#ifdef HASINFINITY
						if (!isMissing(means[icell]))
#endif /*HASINFINITY*/
						{
							d = yy - means[icell];
							vars[icell] += d * d; /* get variance*/
						}
					} /*if (j == nvars)*/
				} /*if (!isMissing(yy))*/
			} /*for (i = 0; i < ndata; i++)*/

			for (icell = 0; icell < ncells; icell++)
			{
#ifdef HASINFINITY
				if (isMissing(means[icell]) || isInfinite(vars[icell]))
				{
					setMissing(vars[icell]);
					foundOverflow = 1;
				}
				else
#endif /*HASINFINITY*/
				if (counts[icell] > 1.0)
				{
					vars[icell] /= (counts[icell] - 1.);
				}
			} /*for (icell = 0; icell < ncells; icell++)*/
		} /*if (needVars)*/
		counts += ncells;
		means += ncells;
		vars += ncells;
		y += ndata;
	} /*for (ivar = 0;ivar < ny;ivar++)*/
	return (!foundOverflow);
} /*celltabs()*/

enum tabScratch
{
	GMEANS = 0,
	GVARS,
	GCOUNTS,
	NTRASH
};


/*
  modified by kb to allow for the variable tabulated being a matrix

  961114 keyword 'n' made a synonym for keyword 'count'  Component
  of result is still named 'count'
  
  980818 Fixed bug (control not set properly on tabs(NULL,a,b,...) and
         set dimensions of result with Setdims()
  990226 Replaced putOUTSTR() by putErrorOUTSTR()
*/

Symbolhandle    tabs(Symbolhandle list)
{
	Symbolhandle    result = (Symbolhandle) 0, symh;
	double        **y, **out[3];
	long            dim[2], ny, ndata;
	long            ncells, i, k;
	long            ncomps = 0, nvars;
	long            type;
	long            nargs = NARGS(list);
	long            nclass[MAXDIMS+1];
	short           all = 1;
	short           doit[3];
	unsigned long   control = ICOUNT | IMEAN | IVAR, which, which1;
	char            *compname[3], *keyword;
	WHERE("tabs");
	TRASH(NTRASH, errorExit);
	
	compname[0] = "mean";
	compname[1] = "var";
	compname[2] = "count";

	for (i=0;i<3;i++)
	{
		out[i] = (double **) 0;
		doit[i] = -1;
	}
	
	*OUTSTR = '\0';

	while (nargs > 0)
	{
		if ((symh = COMPVALUE(list,nargs-1)) == (Symbolhandle) 0 ||
			(keyword = isKeyword(symh)) == (char *) 0)
		{
			break;
		}
		if (all)
		{
			all = 0;
			control = 0;
			for (i=0;i<3;i++)
			{
				doit[i] = 0;
			}
		}
		if (strncmp(keyword, "mean", 4) == 0)
		{
			which = IMEAN;
			which1 = JMEAN;
		}
		else if (strncmp(keyword, "var",3)  == 0)
		{
			which = IVAR;
			which1 = JVAR;
		}
		else if (strncmp(keyword, "count", 5) == 0 ||
				 strcmp(keyword, "n") == 0)
		{
			which = ICOUNT;
			which1 = JCOUNT;
		}
		else
		{
			badKeyword(FUNCNAME, keyword);
			goto errorExit;
		}
		if (!isTorF(symh))
		{
			notTorF(keyword);
			goto errorExit;
		}
		if (DATAVALUE(symh,0) != 0.0)
		{
			control |= which;
			doit[which1] = 1;
		}
		else
		{
			control &= ~which;
			doit[which1] = 0;
		}
		nargs--;
	} /*while (nargs > 0)*/
	
	ncomps = doit[JMEAN] + doit[JVAR] + doit[JCOUNT];
	if(ncomps == 0)
	{
		sprintf(OUTSTR, "ERROR: nothing specified to compute");
		goto errorExit;
	}
	if (nargs < 2)
	{
		badNargs(FUNCNAME,-1002);
		return(0);
	}
	
	symh = COMPVALUE(list, 0);
	if (symh == (Symbolhandle) 0 || isNull(symh))
	{
		if (ncomps > 1 || !doit[JCOUNT])
		{
			sprintf(OUTSTR,
					"ERROR: you must provide y for %s() when computing means or variances",
					FUNCNAME);
			goto errorExit;
		}
		ncomps = 1;
		doit[JMEAN] = doit[JVAR] = 0;
		doit[JCOUNT] = 1;
		control = ICOUNT;
		ndata = -1;
		ny = 1;
	}
	else if (!argOK(symh,REAL,0))
	{
		goto errorExit;
	}
	else if (!isMatrix(symh,dim))
	{
		sprintf(OUTSTR,
				"ERROR: variable tabulated must be REAL vector or matrix");
		goto errorExit;
	}
	else
	{
		y = DATA(symh);
		ndata = dim[0];
		ny = dim[1];
		ncomps = labs(ncomps);
	}
	
	nvars = nargs - 1;
	if (nvars > MAXDIMS)
	{
		sprintf(OUTSTR,"ERROR: more than %ld factors in %s()",(long) MAXDIMS,
				FUNCNAME);
		goto errorExit;
	}
	
	ncells = 1;
	for (i = 0; i < nvars; i++)
	{
		symh = COMPVALUE(list, (i + 1));
		if (!argOK(symh,0,i+2))
		{
			goto errorExit;

		}
		type = TYPE(symh);
		if (type != REAL && type != LOGIC)
		{
			badType(FUNCNAME,-type,i+2);
			goto errorExit;
		}
		
		if (ndata < 0)
		{
			ndata = symbolSize(symh);
		}
		
		if ((nclass[i] = labs(isFactor(symh))) == 0 ||
		   symbolSize(symh) != ndata)
		{
			sprintf(OUTSTR,
					"ERROR: class variables must be factors with same length as tabulated variable");
			goto errorExit;
		}
		ncells *= nclass[i];
	} /*for (i = 0; i < nvars; i++)*/

	if ((control & (IMEAN | IVAR)) &&
	   !getScratch(out[0], GMEANS, ncells*ny, double) ||
	   (control & IVAR) &&
	   !getScratch(out[1], GVARS, ncells*ny, double) ||
	   !getScratch(out[2], GCOUNTS, ncells*ny, double))
	{
		goto errorExit;
	}
	
	result = (ncomps == 1) ? RInstall(SCRATCH, 0) :
		RSInstall(SCRATCH, ncomps, (char **) 0, 0);
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	
	if (ncomps == 1)
	{
		for (i=0;i<3;i++)
		{
			if (doit[i])
			{
				unTrash(i);
				setDATA(result, out[i]);
				break;
			}
		}
	} /*if (ncomps == 1)*/
	else
	{
		k = 0;
		for (i=0;i<3;i++)
		{
			if (doit[i])
			{
				symh = COMPVALUE(result,k++);
				unTrash(i);
				setDATA(symh, out[i]);
				setNAME(symh, compname[i]);
			} /*if (doit[i])*/
		} /*for (i=0;i<3;i++)*/
	} /*ifncomps == 1){}else{}*/

	nclass[nvars] = ny;
	for (i=0;i < ncomps;i++)
	{
		symh = (ncomps == 1) ? result : COMPVALUE(result, i);
		Setdims(symh, nvars + ((ny > 1) ? 1 : 0), nclass);
	} /*for (i=0;i<ncomps;i++)*/
	
	if (!celltabs(list, nvars, out[0], out[1], out[2], control))
	{
		sprintf(OUTSTR,
				"WARNING: overflow in %s() computations; set to MISSING",
				FUNCNAME);
		putErrorOUTSTR();
	}
	
	
	emptyTrash();

	return (result);

  errorExit:
	putErrorOUTSTR();
	emptyTrash();
	Removesymbol(result);
	
	return(0);
	
} /*Symbolhandle tabs()*/


