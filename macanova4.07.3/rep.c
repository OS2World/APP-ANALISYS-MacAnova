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
#pragma segment Rep
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


/*
  970730 removed extraneous externs and includes (all done through globals.h)
         removed explicit usage message
         Removed bug affecting replication of a CHARACTER argument.
  980209 Fixed bug in which rep(x,0) generated a floating point exception
         Cleaned up the code somewhat, too.
  990226 Replaced putOUTSTR() by putErrorOUTSTR()
*/

#include "globals.h"


Symbolhandle    rep(Symbolhandle list)
{
	long            type;
	long            i, j, k, tot, ctot, nrepterms, nelements;
	long            ii, incr, jlim;
	long            nargs = NARGS(list);
	char           *resultstr, *xstring;
	Symbolhandle    result = (Symbolhandle) 0, symhx, symhr;
	double         *repfac, *tmpreal, dtot;
	WHERE("rep");
	
	*OUTSTR = '\0';
	
	if(nargs != 2)
	{
		badNargs(FUNCNAME,2);
		goto errorExit;
	}
	
	symhx = COMPVALUE(list, 0);
	symhr = COMPVALUE(list, 1);
	if(!argOK(symhx, 0,1) || !argOK(symhr,REAL,2))
	{
		goto errorExit;
	}

	type = TYPE(symhx);
	if (type != REAL && type != CHAR && type != LOGIC)
	{
		badType(FUNCNAME,-type,1);
		goto errorExit;
	}
	
	nelements = symbolSize(symhx);
	nrepterms = symbolSize(symhr);

	if (!isVector(symhr) || nrepterms != 1 && nelements != nrepterms)
	{
		sprintf(OUTSTR,
				"ERROR: in %s(x,m), m must be scalar or a vector the same length as x",
				FUNCNAME);
	}
	else if (anyMissing(symhr))
	{
		sprintf(OUTSTR,
				"ERROR: in %s(x,m), m must not contain MISSING values",
				FUNCNAME);
	}
	if (*OUTSTR)
	{
		goto errorExit;
	}
	
	repfac = DATAPTR(symhr);
	dtot = 0.0;
	for (i = 0; i < nrepterms; i++)
	{
		if (repfac[i] < 0 || repfac[i] != floor(repfac[i]))
		{
			sprintf(OUTSTR,
					"ERROR: in %s(x,m), elements of m must be non-negative integers",
					FUNCNAME);
			goto errorExit;
		}
		dtot += repfac[i];
	} /*for (i = 0; i < nrepterms; i++)*/

	tot = (long) dtot;

	if (nrepterms == 1)
	{
		incr = 0;
		tot *= nelements;
	}
	else
	{
		incr = 1;
	}

	if (tot == 0)
	{
		result = NULLSYMBOL;
	} /*if (tot == 0)*/
	else
	{
		if (type == CHAR)
		{
			repfac = DATAPTR(symhr);
			xstring = STRINGPTR(symhx);
			ctot = 0;
			ii = 0;
			for (i = 0; i < nelements; i++)
			{
				ctot += (strlen(xstring) + 1) * repfac[ii];
				ii += incr;
				xstring = skipStrings(xstring, 1);
			}
		} /*if (type == CHAR)*/

		if (dtot > (double) MAXCOORD ||
			((type == CHAR) ?
			 isTooBig(ctot, 1, sizeof(char)) : isTooBig(tot, 1, sizeof(double))))
		{
			sprintf(OUTSTR,
					"ERROR: %s() would create too long a vector", FUNCNAME);
			goto errorExit;
		}
	
		result = Install(SCRATCH,type);

		if (result == (Symbolhandle) 0)
		{
			goto errorExit;
		}
			
		if (type == REAL || type == LOGIC)
		{
			TMPHANDLE = mygethandle(tot * sizeof(double));
			setDATA(result,(double **) TMPHANDLE);
			if (TMPHANDLE == (char **) 0)
			{
				goto errorExit;
			}
		} /*if (type == REAL || type == LOGIC)*/
		else
		{/* CHAR */
			TMPHANDLE = mygethandle(ctot);
			setSTRING(result,TMPHANDLE);
			if (TMPHANDLE == (char **) 0)
			{
				goto errorExit;
			}
		} /*if (type == REAL || type == LOGIC){}else{}*/
		setNDIMS(result,1);
		setDIM(result,1,tot);

		repfac = DATAPTR(symhr);

		switch ((int) type)
		{
		  case REAL:
		  case LOGIC:
			tmpreal = DATAPTR(symhx);
			k = 0;
			if(nrepterms == 1)
			{
				jlim = (long) repfac [0];
				if (nelements == 1)
				{
					double      value = tmpreal[0];
					
					for (j = 0; j < jlim; j++)
					{
						DATAVALUE(result, j) = value;
					}
				} /*if (nelements == 1)*/
				else
				{
					for(j = 0;j < jlim;j++)
					{
						doubleCopy(tmpreal, DATAPTR(result) + k, nelements);
						k += nelements;
					}
				} /*if (nelements == 1){}else{}*/				
			} /*if(nrepterms == 1)*/
			else
			{
				for (i = 0; i < nelements; i++)
				{
					jlim = (long) repfac [i];
					while (jlim-- > 0)
					{
						DATAVALUE(result,k++) = tmpreal[i];
					}
				} /*for (i = 0; i < nelements; i++)*/
			} /*if(nrepterms == 1){}else{}*/
			break;

		  case CHAR:
			xstring = STRINGPTR(symhx);
			resultstr = STRINGPTR(result);
			if (nrepterms == 1)
			{
				jlim = repfac[0];
				while (jlim-- > 0)
				{
					resultstr = copyStrings(xstring, resultstr, nelements);
				}
			} /*if (nrepterms == 1)*/
			else
			{
				for (i = 0; i < nelements; i++)
				{
					jlim = (long) repfac [i];
					while (jlim-- > 0)
					{
						resultstr = copyStrings(xstring, resultstr, 1);
					}
					xstring = skipStrings(xstring, 1);
				}
			} /*if (nrepterms == 1){}else{}*/
		} /*switch ((int) type)*/
	} /*if (tot == 0){}else{}*/
	
	return (result);

  errorExit:
	putErrorOUTSTR();

	Removesymbol(result);
	
	return (0);
	
} /*rep()*/
