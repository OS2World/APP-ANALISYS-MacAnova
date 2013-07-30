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
#pragma segment Run
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
#define RUNFUZZ 1e-15

/*
   MacAnova function run()
   Usage:  run(a,b,inc) returns cat(a,a+inc,a+2*inc,...,b1), b1 = a+k*inc
   where k is largest integer such that b1 >= b (b > a) or b2 <=b (b < a)
   inc != 0 and (b-a)/inc must be positive
           run(a,b) is equivalent to run(a,b,1)
           run(n) is equivalent to run(1,n,1)

   980218 corrected handling of MISSING i1, i2 or inc in run(i1, i2 [,inc])
   980618 NOMISSING bit is set in result
   990212 Changed putOUTSTR() to putErrorOUTSTR()
*/
Symbolhandle    run(Symbolhandle arglist)
{
	long            i, tot;
	long            nargs;
	Symbolhandle    result = (Symbolhandle) 0, symh[3];
	double         *tmpreal;
	double          first, last, by = 1.0, fi, last1, val, range;
	double          dtot;
	WHERE("run");
	
	/* generate a run of numbers */

	*OUTSTR = '\0';

	if ((nargs = NARGS(arglist)) > 3)
	{
		badNargs(FUNCNAME,-3);
		goto errorExit;
	}

	for (i=0;i<nargs;i++)
	{
		symh[i] = COMPVALUE(arglist,i);
		if (!argOK(symh[i], 0, (nargs > 1) ? i+1 : 0))
		{
			goto errorExit;
		}
		if (nargs > 1 && (!isScalar(symh[i]) || TYPE(symh[i]) != REAL ))
		{
			sprintf(OUTSTR,"ERROR: argument %ld to %s not REAL scalar",
					i+1, FUNCNAME);
			goto errorExit;
		}
	} /*for (i=0;i<nargs;i++)*/

	if (nargs == 1)
	{
		if (!isVector(symh[0]) || TYPE(symh[0]) != REAL ||
		   !isScalar(symh[0]) && symbolSize(symh[0]) != 3)
		{
			sprintf(OUTSTR,
					"ERROR: arg must be REAL scalar or vector of length 3 in %s(arg)",
					FUNCNAME);
			goto errorExit;
		}
		
		if (isScalar(symh[0]))
		{
			first = 1.0;
			last = DATAVALUE(symh[0],0);
			by = 1.0;
			
			if (!isMissing(last) && (last != floor(last) || last <= 0))
			{
				sprintf(OUTSTR,"ERROR: N must be positive integer in %s(N)",
						FUNCNAME);
				goto errorExit;
			}
		}
		else
		{
			first = DATAVALUE(symh[0],0);
			last = DATAVALUE(symh[0],1);
			by = DATAVALUE(symh[0],2);
		}
	} /*if (nargs == 1)*/
	else
	{
		first = DATAVALUE(symh[0],0);
		last = DATAVALUE(symh[1],0);
		if (nargs == 3)
		{
			by = DATAVALUE(symh[2],0);
		}
		else
		{
			by = 1.;
		}
	} /*if (nargs == 1){}else{}*/

	if (isMissing(first) || isMissing(last) || isMissing(by))
	{
		sprintf(OUTSTR,
				"ERROR: MISSING values illegal in arguments to %s", FUNCNAME);
		goto errorExit;
	}
	
	if (last == first)
	{
		by = 0.0;
		range = 1;
	}
	else
	{
		range = last - first;
	}
	if (nargs == 2 && range < 0)
	{
		by = -1.0;
	}
	else if (range * by < 0.0 || last != first && by == 0.0)
	{
		sprintf(OUTSTR,"ERROR: incr in %s(%s) is 0 or wrong sign", 
				FUNCNAME, (nargs> 1) ? "start,end,incr" : "cat(start,end,incr)");
		goto errorExit;
	}
	
	dtot = (by != 0.0) ? (range / by) + 1.0 : 1.0;

	tot = (long) floor(dtot);
	if (dtot > (double) MAXCOORD || isTooBig(tot, 1, sizeof(double)))
	{
		sprintf(OUTSTR,
				"ERROR: arguments to %s() specify too long a vector",
				FUNCNAME);
		goto errorExit;
	} /*if (dtot > (double) MAXCOORD || isTooBig(tot, 1, sizeof(double))*/

/* 
   The following is intended to insure that if last is essentially and exact
   multiple of by away from start, then the last element of the result
   will be close to last, even if slightly outside the interval from
   first to last
*/

	if (by != 0.0)
	{		
		for (last1 = first + tot * by;
			 by > 0.0 && last1 <= last || by < 0.0 && last1 >= last ||
			 fabs((last1 - last)/range) <= RUNFUZZ;
			 last1 = first + (++tot) * by)
		{
			;
		}
	}

	result = RInstall(SCRATCH,tot);
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	tmpreal = DATAPTR(result);

	for (i = 0, fi = 0.0; i < tot;i++,fi++)
	{
		val = first + fi * by;
		tmpreal[i] = (fabs(val/range) < RUNFUZZ) ? 0.0 : val;
	}

	if (tot > 1 && fabs((val - last)/range) <= RUNFUZZ)
	{
		tmpreal[tot-1] = last;
	}
	
#if (USENOMISSING)
	/* result can have no MISSING values */
	setNOMISSING(result);
#endif /*USENOMISSING*/
	return (result);

  errorExit:
	putErrorOUTSTR();
	
	return (0);
	
} /*run()*/
