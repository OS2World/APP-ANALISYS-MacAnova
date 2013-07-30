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
#pragma segment Fft
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
#include "tser.h"

/* driver for fast fourier transform functions */

/*
   function to carry out fourier transforms, rft(), hft(), and cft()
   usage: ?ft(x[,divbyt:T])
       where x is a matrix.  When divByT is true the transform is divided
	   nrows(x)
   Calling sequence of working routine is of the form
       error = qft(a, b, m, n, divByT, iop)
     iop      Dims of a           Dims of b
      1	rft    m by n              m by n
      2 hft    m by n              m by n
      3 cft    m by n        m by (n even) ? n : n+1
    error
      1 : largest factor too large (>19)
   	  2 : number of factors too large (>19)
      3 : memory allocation problem

	980730 added new argument to reuseArg() so that notes will not be kept.
	990212 replaced putOUTSTR() by putErrorOUTSTR()
 */

Symbolhandle fft(Symbolhandle list)
{
	long           divByT = 0;
	Symbolhandle   x, xft = (Symbolhandle) 0, arg;
	long           nargs = NARGS(list);
	long           iop, error, i;
	long           nrows, ncols, ncolsout, dims[2];
	char          *keyword;
	char          *argName;
	WHERE("fft");
	
	*OUTSTR = '\0';
	if(strcmp(FUNCNAME,"rft") == 0)
	{
		iop = IRFT;
		argName = "rx";
	}
	else if(strcmp(FUNCNAME,"hft") == 0)
	{
		iop = IHFT;
		argName = "hx";
	}
	else
	{
		iop = ICFT;
		argName = "cx";
	}
	if(nargs > 2)
	{
		badNargs(FUNCNAME,-2);
		goto errorExit;
	}
	x = COMPVALUE(list,0);
	
	if(!argOK(x,REAL,1))
	{
		goto errorExit;
	}

	if(!isMatrix(x,dims))
	{
		sprintf(OUTSTR,"ERROR: first argument to %s must be real matrix",
				FUNCNAME);
		goto errorExit;
	}
	if (nargs == 2)
	{
		arg = COMPVALUE(list,1);
		
		if(!(keyword = isKeyword(arg)) || strcmp(keyword,"divbyt") == 0 )
		{
			if (!isTorF(arg))
			{
				char        outstr[30];
				
				sprintf(outstr, "argument 2 to %s", FUNCNAME);
				notTorF((keyword) ? keyword : outstr);
				goto errorExit;
			}
			divByT = (DATAVALUE(arg,0) != 0.0);
		}
	} /*if (nargs == 2)*/

	nrows = dims[0];
	ncolsout = ncols = dims[1];
	
	if(iop == ICFT && (ncols & 1) != 0)
	{
		ncolsout++;
	}
	if (ncolsout == ncols)
	{
		if (isscratch(NAME(x)))
		{/* re-use argument */
			xft = reuseArg(list, 0, 1, 0);
		}
		else
		{ /* duplicate argument */
			xft = Install(SCRATCH, REAL);
			if (xft == (Symbolhandle) 0 || !Copy(x, xft))
			{
				goto errorExit;
			}
			setNAME(xft, SCRATCH);
		}
		x = xft;

		if (ncolsout == 1)
		{
			clearLabels(xft);
		}
		else if (HASLABELS(xft) && !fixupMatLabels(xft, USECOLLABELS))
		{
			goto errorExit;
		}
		for (i = 1;i <= NDIMS(xft);i++)
		{
			setDIM(xft,i,0);
		}
		setNCLASS(xft, -1);
	} /*if (ncolsout == ncols)*/
	else
	{
		xft = RInstall(SCRATCH,nrows*ncolsout);
		if(xft == (Symbolhandle) 0)
		{
			goto errorExit;
		}
	} /*if (ncolsout == ncols){}else{}*/
	
	if(ncolsout > 1)
	{
		setNDIMS(xft,2);
		setDIM(xft,2,ncolsout);
	}
	else
	{
		setNDIMS(xft,1);
	}
	setDIM(xft,1,nrows);
	
    if((error = qft(DATAPTR(x), DATAPTR(xft), nrows, ncols, divByT, iop)))
	{
		*OUTSTR = '\0';
		if(error == TOOBIGPRIME)
		{
			sprintf(OUTSTR,
					"ERROR: largest prime factor > %ld in length of %s",
					(long) PMAX, FUNCNAME);
		}
		else if(error == TOOMANYFACTORS)
		{
			sprintf(OUTSTR,
					"ERROR: more than %ld prime factors in length of %s",
					(long) NEST,FUNCNAME);
		}
		goto errorExit;
	}
	
	UNLOADSEG(qft);
	
	return (xft);

  errorExit:
	putErrorOUTSTR();
	Removesymbol(xft);
	UNLOADSEG(qft);
	return (0);
} /*fft()*/
