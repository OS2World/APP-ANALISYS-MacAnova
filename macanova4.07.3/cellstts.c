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
#pragma segment Cellstts
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

/*
  970820 moved getstringterm() to glmutils.c and renamed to stringToTerm()
  990226 replaced putOUTSTR() by putErrorOUTSTR()
*/
Symbolhandle    cellstats(Symbolhandle list)
{
	Symbolhandle    result, symh;
	double        **out[3];
	long            n, i, j, j2, term[MAXVARS];
	long            nvars = NVARS, ny = NY, nfactors;
	char           *cellcompname[3];
	WHERE("cellstats");
	
	*OUTSTR = '\0';
	cellcompname[0] = "mean";
	cellcompname[1] = "var";
	cellcompname[2] = "count";

	if (nvars < 1)
	{
		sprintf(OUTSTR,"ERROR: no current model for %s()", FUNCNAME);
		goto errorExit;
	}

	if (NARGS(list) != 1)
	{
		badNargs(FUNCNAME,1);
		goto errorExit;
	}

	symh = COMPVALUE(list, 0);
	if(!argOK(symh, 0,1))
	{
		goto errorExit;
	}
	if (TYPE(symh) != CHAR || !isScalar(symh))
	{
		char        what[50];
		
		sprintf(what, "ERROR: argument to %s()", FUNCNAME);
		notCharOrString(what);
		goto errorExit;
	}

	nfactors = stringToTerm(STRINGPTR(symh), term);
	if (!nfactors > 0)
	{
		/* can't find proper term */
		goto errorExit;
	}

	n = 1;
	for (i = 0; i < nvars; i++)
	{
		if (term[i] > 0)
		{
			if (NCLASSES[i] < 1)
			{
				sprintf(OUTSTR,
						"ERROR: variable %s in term for %s() is not factor",
						VARNAME(term[i]), FUNCNAME);
				goto errorExit;
			}
			term[i] = 1;
			n *= NCLASSES[i];
		}
	} /*for (i = 0; i < nvars; i++)*/

	result = RSInstall(SCRATCH, 3L, cellcompname, ny * n );

	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	for (i = 0; i < 3; i++)
	{
		symh = COMPVALUE(result, i);
		setNDIMS(symh, nfactors + ((ny > 1) ? 1 : 0));
		j2 = 1;
		for (j = 0; j < nvars; j++)
		{
			if (term[j] > 0)
			{
				setDIM(symh,j2,NCLASSES[j]);
				j2++;
			}
		} /*for (j = 0; j < nvars; j++)*/
		if (ny > 1)
		{
			setDIM(symh, nfactors + 1,ny);
		}
		
	} /*for (i = 0; i < 3; i++)*/
	
	for (i = 0; i < 3; i++)
	{
		out[i] = DATA(COMPVALUE(result,i));
	}
	(void) cellstat(Y, term, out[0], out[1], out[2], CELLSTATS);

	if (PREVMODELTYPE & FASTANOVA)	/* for fastanova output */
	{
		putOutErrorMsg("WARNING: results include estimated missing data");
	}
	
	return (result);

  errorExit:
	putErrorOUTSTR();
	
	return (0);
} /*cellstats()*/

