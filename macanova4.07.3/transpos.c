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
#pragma segment Transpose
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

#include "globals.h"

/*
  970801 minor change:  When NDIMS(arg) == 1 and arg has a label,
         the row label of arg' is now "@" (expandable numeric) instead
         of "" (no label).  The old behavior can be gotten by
         matrix(a',labels:structure(getlabels(a),""))

  971104 added check on value of myhandlelength()

  981015 minor changes.  dim[0] is used instead of ndims until actual
         copying is done.
  990212 Changed putOUTSTR() to putErrorOUTSTR()
*/
Symbolhandle    doTranspose(Symbolhandle arg, double *params,
								   unsigned long control,
								   unsigned long *status)
{
	Symbolhandle result = (Symbolhandle) 0;
	long         type = (arg != (Symbolhandle) 0) ? TYPE(arg) : NULLSYM;
	WHERE("doTranspose");
	
	*status = 0;
	result = Install(SCRATCH,type);
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	
	
	if (type != NULLSYM)
	{
		long         size, j, rplace, aplace;
		long         kr;
		long         ndims;
		long         dim[MAXDIMS+1], newdim[MAXDIMS+1];
		long         ii[MAXDIMS+1], incs[MAXDIMS+1];
		long         place[MAXDIMS+1];
		long         handleLength;
		short        charType = (type == CHAR);
		double     **axH, **rxH;
		double      *ax, *rx;
		char       **asH, **rsH;
		char        *as, *rs;

		size = symbolSize(arg);
		getDims(dim, arg);
		ndims = dim[0];

		if (ndims == 1)
		{
			/* note: original value of ndims is still in dim[0] */
			ndims = 2;
			dim[2] = 1;
		}

		newdim[0] = ndims;
		/* reverse order of dimensions*/
		for (j = 0; j < ndims; j++)
		{
				newdim[j+1] = dim[ndims-j];
		}
		Setdims(result, newdim[0], newdim + 1);
		
		if (charType)
		{		
			asH = STRING(arg);
			handleLength = myhandlelength(asH);
			if (handleLength < 0)
			{
				goto errorExit;
			}
			TMPHANDLE = mygethandle(handleLength);
			setSTRING(result,TMPHANDLE);
			rsH = STRING(result);
		}
		else
		{
			axH = DATA(arg);
			TMPHANDLE = mygethandle(myhandlelength((char **) axH));
			setDATA(result,(double **) TMPHANDLE);
			rxH = DATA(result);
		}
		if (TMPHANDLE == (char **) 0)
		{
			goto errorExit;
		}


		if (!charType)
		{
			ax = *axH;
			rx = *rxH;
		}
		else
		{
			as = *asH;
			rs = *rsH;
		}
	
		if (HASLABELS(arg))
		{
			char          *labels[MAXDIMS];
			long           lengths[MAXDIMS];
			char           numericLabel[2];
			
			getAllLabels(arg, labels, lengths, (long *) 0);

			if (dim[0] == 1)
			{ /* make room for 1 exandable numeric row label */
				numericLabel[0] =  NUMERICLABEL;
				numericLabel[1] = '\0';
				lengths[1] = lengths[0];
				lengths[0] = 2;
			}
			else
			{
				/* swap lengths end for end */
				for (j = 0; j < ndims/2; j++)
				{
					long        tmp = lengths[j];
					
					lengths[j] = lengths[ndims-1-j];
					lengths[ndims-1-j] = tmp;
				}
			}
			
			TMPHANDLE = createLabels(ndims, lengths);
			if (TMPHANDLE == (char **) 0 || !setLabels(result, TMPHANDLE))
			{
				mydisphandle(TMPHANDLE);
				goto errorExit;
			}
			getAllLabels(arg, labels, (long *) 0, (long *) 0);
			if (dim[0] == 1)
			{ /* add a numeric row label */
				labels[1] = numericLabel;
			}
			/* append dimension labels in reverse order */
			for (j = 0; j < ndims; j++)
			{
				appendLabels(result, labels[ndims - 1 - j], j, dontExpand);
			}
		} /*if (HASLABELS(arg))*/

		incs[0] = size; 
		
		for (j = 1; j<=ndims; j++)
		{
			ii[j-1] = 0; 
			place[j-1] = 0; 
			incs[j] = incs[j-1]/newdim[j]; 
		} /*for (j = 1; j<=ndims; j++)*/
	
		/* rearrange as transpose */

		rplace = aplace = 0;
		for (kr = 0; kr < size; kr++)
		{
			if (!charType)
			{
				rx[rplace++] = ax[aplace]; 
			}
			else
			{/* string equivalent of rx[rplace++] = ax[aplace]*/
				rplace = copyStrings(as + aplace, rs + rplace, 1) - rs; 
			}
		
/* the following is essentially equivalent to ndims nested for loops */
			for (j = 1; j <= ndims; j++)
			{
				if (ii[j-1] == 0)
				{
					place[j-1] = aplace; 
				}
				if ((ii[j-1] += incs[j]) < incs[j-1])
				{
					if (!charType)
					{
						aplace += incs[j]; 
					}
					else
					{/* string equivalent of aplace += incs[j] */
						aplace = skipStrings(as + aplace, incs[j]) - as; 
					}
					break; 
				} /*if ((ii[j-1] += incs[j]) < incs[j-1])*/
				ii[j-1] = 0; 
				aplace = place[j-1]; 
			} /*for (j = 1; j <= ndims; j++)*/
		} /*for (kr = 0; kr < size; kr++)*/

#if (USENOMISSING)
		if (NOMISSING(arg))
		{
			setNOMISSING(result);
		}
#endif /*USENOMISSING*/
	} /*if (type != NULLSYM)*/	

	return(result);
	
  errorExit:
	Removesymbol(result);
	return (0);
} /*doTranspose()*/

/*
   Returns non-zero if and only if all non-structure components are non-null
   and have type REAL, LOGIC, or CHAR
*/

#define USECHECKRECUR
#ifdef USECHECKRECUR

static long argIsRealLogicOrChar(Symbolhandle arg, double *params,
								 unsigned long control, unsigned long *status)
{
	long     type = (arg != (Symbolhandle) 0) ? TYPE(arg) : NULLSYM;

	return (type == REAL || type == LOGIC || type == CHAR || type == NULLSYM);
}

static long checktypes(Symbolhandle arg)
{
	unsigned long    status = 0;
	unsigned long    control = LEFTRECUR;
	
	return (doRecurCheck1(argIsRealLogicOrChar, arg, (double *) 0, control,
						   &status));
} /*checktypes()*/


#else /*USECHECKRECUR*/
static checktypes(Symbolhandle arg)
{
	long        type;
	long        i, ncomps;
	long        answer = 0;

	if (!isNull(arg))
	{
		ncomps = NCOMPS(arg);
		
		if (isStruc(arg))
		{
			for (i=0;i<ncomps;i++)
			{
				answer = checktypes(COMPVALUE(arg,i));
				if (!answer)
				{
					break;
				}
			}
		} /*if (isStruc(arg))*/
		else if ((type = TYPE(arg)) == REAL || type == LOGIC ||
				type == NULLSYM || type == CHAR)
		{
			answer = 1;
		} /*if (isStruc(arg)){}else{}*/
	} /*if (!isNull(arg))*/

	return (answer);
	
} /*checktypes()*/
#endif /*USECHECKRECUR*/

/*
  Make a transpose of a matrix
  Reverse order of dimensions for an array
  usage:
    y <- x' or y <- t(x)
    When ndims(x) = 1, y is 1 by ndims(x) with y[1,j] = x[j]
	When ndims(x) > 1
      dim(y) = reverse(dim(x))
      y[i1,i2,...,ip] = x[ip,...,i2,i1]

  Planned, but not implemented, usage:
     y <- t(x,J) # p = ndims(x), J = vector(j1,...,jp) permutation of run(p)
     with y <- t(x,j1,j2,...) equivalent to t(x,vector(j1,j2,...)).
	 Result:
	 dim(y) = dim(x)[J]
     y[i1,...,ip] = x[J[i1],J[i2],...,J[ip]]
     so, simply t(x) is equivalent to t(x,p,p-1,...,2,1)

	 Alternative possibility:
       y[J[i1],J[i2],...,J[ip]] = x[i1,...,ip]
*/
Symbolhandle    transpose(Symbolhandle list)
{

	/* make a transpose of a matrix */

	Symbolhandle    result, arg;
	unsigned long   control = 0, status;
	
	WHERE("transpose");

	*OUTSTR = '\0';
	
	if (NARGS(list) != 1)
	{
		badNargs(FUNCNAME,1);
		goto errorExit;
	}
	
	arg = COMPVALUE(list, 0);
	if (!argOK(arg,(long) 0, 0))
	{
		goto errorExit;
	}

	if (!checktypes(arg))
	{
		if (isStruc(arg))
		{
			sprintf(OUTSTR,
					"ERROR: component of structure not REAL, LOGICAL, CHARACTER or NULL for %s()",
					FUNCNAME);
		}
		else
		{
			badType(FUNCNAME, -TYPE(arg), 0);
		}
		goto errorExit;
	} /*if (!checktypes(arg))*/

	if (isStruc(arg))
	{
		control |= LEFTRECUR;
	}

	result = doRecur1(doTranspose, arg, (double *) 0, control, &status);

	if (result == 0)
	{
		goto errorExit;
	}
	
	return (result);

  errorExit:
	putErrorOUTSTR();

	return (0);
} /*transpose()*/
