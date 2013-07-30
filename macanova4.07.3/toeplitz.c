/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 4.00 or later
*(C)*
*(C)* Copyright (c) 1996 by Gary Oehlert and Christopher Bingham
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
#pragma segment Toeplitz
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

#include "globals.h"

/*
  990226 replaced putOUTSTR() by putErrorOUTSTR()
*/
Symbolhandle toeplitz(Symbolhandle list)
{
	Symbolhandle          arg;
	Symbolhandle          result = (Symbolhandle) 0;
	double               *r, *vi, *vj, *vij, *vji;
	double                ri;
	long                  n, i, j, inc;
	arg = COMPVALUE(list,0);
	
	if(NARGS(list) > 1 || arg == (Symbolhandle) 0)
	{
		badNargs(FUNCNAME,1);
		goto errorExit;
	}
	if(!argOK(arg,0,0))
	{
		goto errorExit;
	}
	if(TYPE(arg) != REAL || !isVector(arg))
	{
		sprintf(OUTSTR,
				"ERROR: argument to %s must be REAL vector",FUNCNAME);
		goto errorExit;
	}
	n = symbolSize(arg);
	if (isTooBig(n, n, sizeof(double)))
	{
		resultTooBig(FUNCNAME);
		goto errorExit;
	}
	
	result = RInstall(SCRATCH,n*n);
	if(result == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	setNDIMS(result,2);
	setDIM(result,1,n);
	setDIM(result,2,n);
	
	r = DATAPTR(arg);
	vi = vj = DATAPTR(result);
	inc = n + 1;
	for(i=0;i<n;i++)
	{
		ri = r[i];
		vij = vi;
		vji = vj;
		for(j=i;j<n;j++)
		{
			*vij = ri;
			vij += inc;
			if(i > 0)
			{
				*vji = ri;
				vji += inc;
			}
		}
		vi++;
		vj += n;
	}
	return (result);
	
  errorExit:
	putErrorOUTSTR();
	return (0);
	
} /*toeplitz()*/
