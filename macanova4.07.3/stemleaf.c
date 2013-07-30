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
#pragma segment Stemleaf
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

/*
   960705 Keyword phrase stats:T prints min, max, quartiles
          Keyword phrase depth:F suppresses printing depth

   990226 Replaced putOUTSTR() by putErrorOUTSTR()
*/
#include "globals.h"

enum stemleafScratch
{
	GY = 0,
	NTRASH
};

Symbolhandle    stemleaf(Symbolhandle list)
{
	/* routine to do stema and leaf plot */

	Symbolhandle    symhx,symhstem, symhkey;
	long            j, nstem, tot, notmiss;	
	long            verbose = 0, xtrems = 0, width = SCREENWIDTH;
	long            nargs = NARGS(list);
	long            depth = 1;
	char           *keyword, **title = (char **) 0;
	double        **x, **y = (double **) 0;
	double          mstem;
	WHERE("stemleaf");
	TRASH(NTRASH,errorExit);
	
	OUTSTR[0] = '\0';
	nargs = NARGS(list);
	symhx = COMPVALUE(list, 0);

	if(!argOK(symhx,0,(nargs == 1) ? 0 : 1))
	{
		goto errorExit;
	}

	if (TYPE(symhx) != REAL || !isVector(symhx))
	{
		sprintf(OUTSTR,"ERROR: argument 1 to %s() must be REAL vector",
				FUNCNAME);
		goto errorExit;
	}
	
	/* scan for keywords from end of argument list */
	while(nargs > 1)
	{
		symhkey = COMPVALUE(list,nargs-1);
		if(!(keyword = isKeyword(symhkey)))
		{
			break;
		}
		
		if(strncmp(keyword,"out",3) == 0)
		{
			if (!isTorF(symhkey))
			{
				notTorF(keyword);
				goto errorExit;
			}
			else
			{
				xtrems = (DATAVALUE(symhkey,0) == 0.0);
			}
		}
		else if(strncmp(keyword,"stat",4) == 0)
		{
			if (!isTorF(symhkey))
			{
				notTorF(keyword);
				goto errorExit;
			}
			else
			{
				verbose = (DATAVALUE(symhkey,0) != 0.0) ? 1 : 0;
			}
		}
		else if(strcmp(keyword,"depth") == 0)
		{
			if (!isTorF(symhkey))
			{
				notTorF(keyword);
				goto errorExit;
			}
			else
			{
				depth = (DATAVALUE(symhkey,0) != 0.0);
			}
		}
		else if(strcmp(keyword,"title") == 0)
		{
			if (!isCharOrString(symhkey))
			{
				notCharOrString(keyword);
				goto errorExit;
			}
			else
			{
				title = STRING(symhkey);
			}
		}
		else
		{
			badKeyword(FUNCNAME, keyword);
			goto errorExit;
		}
		nargs--;
	} /* while(nargs > 1) */
	
	/* get number of stems */

	if(nargs > 2)
	{
		badNargs(FUNCNAME,-2);
		goto errorExit;
	}
	
	if (nargs == 2)
	{
		symhstem = COMPVALUE(list, 1);
		
		if(!argOK(symhstem, 0, 2))
		{
			goto errorExit;
		}
		if (!isInteger(symhstem, POSITIVEVALUE) ||
			(mstem = DATAVALUE(symhstem, 0)) < 2.0)
		{
			sprintf(OUTSTR,
					"ERROR: number of stems for %s() must be integer >= 2",
					FUNCNAME);
			goto errorExit;
		}
		nstem = (long) mstem;
	} /*if (nargs == 2)*/
	else
	{
		long        extra = ((xtrems != 0) ? 6 : 4) + verbose;

		nstem = -((SCREENHEIGHT > 10) ? SCREENHEIGHT - extra : 24 - extra);
	}
	
	tot = symbolSize(symhx);
	x = DATA(symhx);

	notmiss = 0;

	if(!getScratch(y,GY,tot,double))
	{
		return (0);
	}
	
	for (j = 0; j < tot; j++)
	{
		if (!isMissing((*x)[j]))
		{
			(*y)[notmiss++] = (*x)[j];
		}
	}

	if (notmiss < 2)
	{
		putOutErrorMsg("ERROR: at most 1 non-missing value");
		goto errorExit;
	}
	else if (notmiss < tot)
	{
		sprintf(OUTSTR, "WARNING: %ld missing values ", tot - notmiss);
		putErrorOUTSTR();
	}

	if(!doStemLeaf(y, notmiss, width, nstem, xtrems, verbose, depth, title))
	{
		goto errorExit;
	}
	emptyTrash();

	*OUTSTR = '\0';
	return (NULLSYMBOL);

  errorExit:
	putErrorOUTSTR();

	emptyTrash();

	return (0);
	
}

