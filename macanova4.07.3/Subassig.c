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
#pragma segment Lang
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"


/*
  Function Subassign() implements assignment to subscripts
            arg1[list] <- arg2

  Modified by kb to allow the subscripts to be a single matrix, with each
  row providing a full set of subscripts.  Unlike Element() in Lang.c, it
  requires a matrix (2 dimensional array).

  Modified by kb to allow negative subscripts, specifying positions not to
  be replaced
 
  Modified 920608 by the addition of an additonal argument.  It is now
  called twice by yypars, once with checkLhs != 0 to check the l.h.s, before
  parsing the r.h.s., and a second time with checkLhs == 0 after parsing
  the r.h.s.

  Modified 920728 so that it returns an object of the expected shape, that is
  for example  'b<- a[1,] <- 3' sets the first row of a to 3 and sets
  b to a row vector of 3's with dim(b)[2] == dim(a)[2].  Previously it
  simply returned arg1.

  950825 modified so as to recognize macros LEFTHANDSIDE and RIGHTHANDSIDE
  The last argument is LEFTHANDSIDE if only checking the lefthand side of
  the assignment.

  980219 a[..., j,...] <- NULL, a[...,j,...] <- scalar now allowed when
  (a) j is NULL
  (b) j is rep(F,n)
  (c) j is -run(n)
  where n is the length of the dimension.  In all cases, no change is made
  to a and the result returned is NULL In particular, x[x>10] <- 10 is
  allowed, even when max(x) <= 10, as also is x[x>10] <- y[x>10]
  This change is based on a suggestion of Peter Fortini
   (Peter_Fortini@ST.cytec.com)

  980223 Fixed bug so that a[] <- b is allowed if b is a scalar or has
         the same length as a.
*/

/* These must be defined identically in mainpars.y*/
#define LEFTHANDSIDE         1
#define RIGHTHANDSIDE        2

#define trash   NAMEFORTRASH

enum SubassgnScratch
{
	GITEMLISTH = 0,
	GALLSUBSH,
	GCNEWARG1H,
	GCRESULTH,
	NTRASH
};
/*
   Routine to replace elements of arrays

   961101 Changed code so that, for example,
   x[vector(1,1)] <- a, where length(a) == 2 assigns a[2] to x[1],
   for REAL, LOGICAL and CHARACTER variables.  Previously a[1] was
   assigned for CHARACTER variables.

   980312 Added argument completeDim to buildSubscript() call; not needed here
          but used by extractIt() in Lang.c
          Added awareness of NOMISSING flag.  Left hand side will have flag
          set only if both r.h.s and subscripted variable have it set.  Even
          if all MISSING values in subscripted variable are replaced by non-
          MISSING values, the flag will not be set.
   980616 Modified slightly so that a[,] <- b, a[,,] <- b, etc are treated
          the same as a[] <- b, viz., it's an error unless b is a scalar
          or length(a) = length(b)
*/
Symbolhandle    Subassign(Symbolhandle arg1, Symbolhandle list,
						  Symbolhandle arg2, long whichSide)
{
	Symbolhandle    subscrSymh, result = (Symbolhandle) 0;
	Symbolhandle    dest = (Symbolhandle) 0;
	long            i, i2, i2inc, j, k;
	long            space, resSpace, arg1tot, arg2tot, nitems, factor;
	long            jLast, length;
	long            l, lTail;
	long            maxSubscr;
	long            ii[MAXDIMS],resdim[MAXDIMS];
	long            ndims, ncomps;
	long          **itemlistH = (long **) 0;
	long          **allsubsH = (long **) 0;
	long           *subValues[MAXDIMS+1];
	long           *itemlistP;
	long            needed = 0;
	long            position;
	long            subLimit;
	long            arg1Type, arg2Type, subscrType;
	int             matrixSubscr = 0, sameSymbol = 0;
	int             foundNullSubscr = 0;
	int             rightIsNull = 0, rightIsScalar;
	char           *ch1, *ch2;
	double          nclass;
	double          y;
	double         *subscript;
	char          **cNewArg1H = (char **) 0, **cResultH = (char **) 0;
	char           *cNewArg1P, *cResultP;
	char            namearg1[NAMELENGTH+1];
	Symbolhandle    trash = (Symbolhandle) 0;
	WHERE("Subassign");
	
	*OUTSTR = '\0';
	ncomps = NCOMPS(list);
	
	/* check to make sure subscripts are of proper type and number */
	if (whichSide == LEFTHANDSIDE)
	{
		if (arg1 == (Symbolhandle) 0)
		{ /* should not happen */
			sprintf(OUTSTR,"ERROR: no variables in subscript assignment");
		}
		else if (!isFakeSymbol(arg1) && !myvalidhandle((char **) arg1) ||
				 TYPE(arg1) == LIST)
		{
			sprintf(OUTSTR,
					"ERRRO: use of subscripts with damaged (deleted?) variable");
		}
		else if (!isDefined(arg1))
		{
			sprintf(OUTSTR,
					"ERROR: use of subscripts with undefined variable %s",
					NAME(arg1));
		}
		else if ((arg1Type = TYPE(arg1)) == STRUC)
		{
			sprintf(OUTSTR,
					"ERROR: illegal to assign to a structure component");
		}
		else if (arg1Type != REAL && arg1Type != LOGIC && arg1Type != CHAR)
		{
			sprintf(OUTSTR,"ERROR: use of subscripts with %s variable",
					typeName(arg1Type));
		}
		else if (isscratch(NAME(arg1)))
		{
			sprintf(OUTSTR,
					"ERROR: assignment to subscripts of number, expression or function result");
		}
		else if (ncomps > MAXDIMS)
		{
			sprintf(OUTSTR,
					"ERROR: more than %ld subscripts",(long) MAXDIMS);
		}
		else if (ncomps != 1 && ncomps < NDIMS(arg1))
		{
			sprintf(OUTSTR, "ERROR: too few subscripts");
		}

		if (*OUTSTR)
		{
			yyerror(OUTSTR);
			arg1 = (Symbolhandle) 0;
		}
		return (arg1);
	} /*if (whichSide == LEFTHANDSIDE)*/
	
	/* whichSide must be RIGHTHANDSIDE.  Now carry out assignment */
	if (isNull(arg2) && TYPE(arg2) != NULLSYM || !isDefined(arg2))
	{
		sprintf(OUTSTR, "ERROR: replacement elements are %s",
				(isNull(arg2)) ? "null" : "undefined");
	}
	else if ((arg2Type = TYPE(arg2)) == NULLSYM)
	{
		rightIsNull = 1;
	}
	else if (arg2Type != (arg1Type = TYPE(arg1)))
	{
		sprintf(OUTSTR,"ERROR: replacement elements of wrong type");
	}

	if (*OUTSTR)
	{
		goto yyerrorMsg;
	}
	
	ndims = NDIMS(arg1);
	
	arg1tot = symbolSize(arg1);
	arg2tot = symbolSize(arg2);
	rightIsScalar = arg2tot == 1;
	for (i = 0; i < ncomps && COMPVALUE(list, i) == (Symbolhandle) 0; i++)
	{
		;
	}
	if (i == ncomps)
	{ /* all empty subscripts*/
		if (arg1tot != arg2tot && !rightIsScalar)
		{
			sprintf(OUTSTR,"ERROR: wrong number of replacement items");
			goto yyerrorMsg;
		}
		subscrSymh = (Symbolhandle) 0;
	} /*if (ncomps == 1 && COMPVALUE(list, 0) == (Symbolhandle) 0)*/
	else
	{
		for (i = 0; i < ncomps; i++)
		{
			subscrSymh = COMPVALUE(list, i);
			if (subscrSymh == (Symbolhandle) 0)
			{/* blank subscript */
				continue;
			}
			if (!isDefined(subscrSymh))
			{
				sprintf(OUTSTR,"ERROR: undefined subscript");
			}
			else if ((subscrType = TYPE(subscrSymh)) != LOGIC &&
					 subscrType != REAL && subscrType != NULLSYM)
			{
				sprintf(OUTSTR,"ERROR: subscript not REAL, LOGICAL or NULL");
			}
			else if (subscrType == NULLSYM)
			{
				if (!rightIsNull && !rightIsScalar)
				{
					sprintf(OUTSTR,
							"ERROR: assigning non-NULL non-scalar to NULL subscript");
				}
				else
				{
					foundNullSubscr = 1;
				}
			}
			else if (anyMissing(subscrSymh))
			{
				sprintf(OUTSTR,"ERROR: MISSING values not allowed as subscripts");
			}
			else if (!isVector(subscrSymh))
			{
				/* must be matrix subscript */
				if (ncomps > 1)
				{
					sprintf(OUTSTR,"ERROR: array used as marginal subscript");
				}
				else if (subscrType != REAL)
				{
					sprintf(OUTSTR,"ERROR: matrix of subscripts must be REAL");
				}
				else if (NDIMS(subscrSymh) != 2 || DIMVAL(subscrSymh,2) != ndims)
				{
					sprintf(OUTSTR,
							"ERROR: matrix of subscripts has wrong dimensions");
				}
				else if (arg2tot > 1 && arg2tot != DIMVAL(subscrSymh,1))
				{
					sprintf(OUTSTR,
							"ERROR: number of rows of matrix of subscripts != number of values");
				}
				else
				{
					matrixSubscr = 1;
				}
			} /*else if (!isVector(subscrSymh))*/
			if (*OUTSTR)
			{
				goto yyerrorMsg;
			}
		} /*for (i = 0; i < ncomps; i++)*/

		if (foundNullSubscr && (rightIsNull || rightIsScalar))
		{		
			/* no further check of subscripts */
			goto returnNull;
		} /*if (rightIsNull)*/
	} /*if (ncomps == 1 && COMPVALUE(list, 0) == (Symbolhandle) 0){}else{}*/
	
	if ((trash = GarbInstall(NTRASH)) == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	strcpy(namearg1,NAME(arg1));

	for (i = 0;i<ncomps;i++)
	{
		ii[i] = 0;
		subValues[i] = (long *) 0;
	}

	if (matrixSubscr)
	{/* matrix of subscripts */
		k = 0;
		/* check subscripts for validity */
		nitems = DIMVAL(subscrSymh,1);
		for (j=0;j<ndims;j++)
		{
			subLimit = DIMVAL(arg1,j+1);
			for (i=0;i<nitems;i++)
			{
				y = DATAVALUE(subscrSymh,k++);
				if (y <= 0 || y > subLimit)
				{
					sprintf(OUTSTR,"ERROR: subscript out of range");
				}
				else if (y != floor(y))
				{
					sprintf(OUTSTR,"ERROR: non-integral subscript");
				}
				if (*OUTSTR)
				{
					goto yyerrorMsg;
				}
			} /*for (i=0;i<nitems;i++)*/
		} /*for (j=0;j<ndims;j++)*/

		if (arg1Type == CHAR && !getScratch(itemlistH,GITEMLISTH,nitems,long))
		{
			goto errorExit;
		}

		resdim[0] = 1;
		for (j=1;j<ndims;j++)
		{
			resdim[j] = DIMVAL(arg1,j)*resdim[j-1];
		}

		result = (arg1Type == CHAR) ?
		  Install(SCRATCH,CHAR) : RInstall(SCRATCH,nitems);
		if (result == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		setTYPE(result,arg1Type);
	} /*if (matrixSubscr)*/
	else
	{ /* list of subscripts */
		if (arg1Type == REAL && NCLASS(arg1) > 0)
		{	/* destination is factor; check values for validity */
			nclass = (double) NCLASS(arg1);
			for (i=0;i<arg2tot;i++)
			{
				y = DATAVALUE(arg2,i);
				if (!isMissing(y) && (y != floor(y) || y < 1.0 || y > nclass))
				{
					sprintf(OUTSTR,
							"ERROR: attempt to assign non-integer or out of range values to a factor");
					goto yyerrorMsg;
				}
			} /*for (i=0;i<arg2tot;i++)*/
		} /*if (arg1Type == REAL && NCLASS(arg1) > 0)*/
		factor = nitems = 1;
		for (i=0;i<ncomps;i++)
		{
			subscrSymh = COMPVALUE(list,i);
			/* we know that subscrSymh cannot be NULL*/
			if(ncomps == 1)
			{
				maxSubscr = arg1tot;
			}
			else
			{
				maxSubscr = (i < ndims) ? DIMVAL(arg1,i+1) : 1;
			}
			resdim[i] = subscriptLength(subscrSymh ,maxSubscr);
			
			if (resdim[i] == 0)
			{
				goto errorExit;
			}
			needed += resdim[i];
		} /*for (i=0;i<ncomps;i++)*/

		if (!getScratch(allsubsH,GALLSUBSH,needed,long))
		{
			goto errorExit;
		}

		subValues[0] = *allsubsH;
		for (i=0;i<ncomps;i++)
		{
			subscrSymh = COMPVALUE(list,i);
			if(ncomps == 1)
			{
				maxSubscr = symbolSize(arg1);
			}
			else
			{
				maxSubscr = (i < ndims) ? DIMVAL(arg1,i+1) : 1;
			}
			resdim[i] = buildSubscript(subscrSymh, maxSubscr, resdim[i], 
									   subValues[i], (int *) 0);
			if (resdim[i] < 0)
			{
				goto errorExit;
			} /*if (resdim[i] < 0)*/
			if (resdim[i] == 0)
			{
				foundNullSubscr = 1;
			}
			
			if (!foundNullSubscr && i > 0)
			{
				for (j=0;j<resdim[i];j++)
				{
					(subValues[i])[j] *= factor;
				} /*for (j=0;j<resdim[i];j++)*/
			} /*if (i > 0)*/

			factor *= maxSubscr;
			nitems *= resdim[i];
			subValues[i+1] = subValues[i] + resdim[i];
		} /* for (i=0;i<ncomps;i++) */

		/* nitems is number of elements specified by subscript(s) */
		if (arg2tot > 1 && nitems != arg2tot)
		{
			sprintf(OUTSTR,"ERROR: wrong number of replacement items");
			goto yyerrorMsg;
		} /*if (arg2tot > 1 && nitems != arg2tot)*/

		if (foundNullSubscr || rightIsNull)
		{
			emptyTrash();
			goto returnNull;
		}
		
		if (arg1Type == CHAR)
		{
			if (!getScratch(itemlistH,GITEMLISTH,nitems,long))
			{
				goto errorExit;
			}
			result = Install(SCRATCH,CHAR);
		} /*if (arg1Type == CHAR)*/
		else
		{
			result = RInstall(SCRATCH,nitems);
		} /*if (arg1Type == CHAR){}else{}*/
		if (result == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		setTYPE(result,arg1Type);
	} /*if (matrixSubscr){}else{}*/

	/*
	  dest is symbol for the subscripted symbol
	  If arg1 and arg2 are the same and are REAL or LOGICAL,
	  then to ensure arg2 is not corrupted, arg2 is copied to dest and is
	  removed only in the case of success
	  */
	sameSymbol = (arg1 == arg2) && arg1Type != CHAR;

	if (!sameSymbol)
	{
		dest = arg1;
	} /*if (!sameSymbol)*/
	else
	{/* allows, e.g., a[run(length(a),1)] <- a to work correctly */
		dest = Install(namearg1,REAL);
		if (dest == (Symbolhandle) 0 || !Copy(arg1,dest))
		{
			goto errorExit;
		}
		Cutsymbol(arg1); /* move arg1 to Scratch symbol table */
		setNAME(arg1,SCRATCH);
		Addsymbol(arg1);
	} /*if (!sameSymbol{}else{})*/

	/* For REAL or LOGIC insert values; for CHAR fill (*itemlistH) */
	if (arg1Type == CHAR)
	{
		itemlistP = *itemlistH;
	}

	if (matrixSubscr)
	{/* matrix of subscripts */
		i2 = 0;
		i2inc = (arg2tot == 1) ? 0 : 1;
		for (i=0;i<nitems;i++)
		{
			subscript = DATAPTR(subscrSymh);
			position = (long) subscript[i] - 1;
			for (j = 1;j< ndims;j++)
			{
				subscript += nitems;
				position += resdim[j]*((long) subscript[i] - 1);
			}
			if (arg1Type == CHAR)
			{
				itemlistP[i] = position;
			}
			else
			{
				DATAVALUE(result,i) = DATAVALUE(dest,position) = DATAVALUE(arg2,i2);
				i2 += i2inc;
			}
		} /*for (i=0;i<nitems;i++)*/
	} /*if (matrixSubscr)*/
	else
	{ /* list of subscripts */
		lTail = -1;
		i2 = 0;
		i2inc = (arg2tot == 1) ? 0 : 1;
		for (i=0;i<nitems;i++)
		{
			l = (subValues[0])[ii[0]];
			if (lTail < 0)
			{
				lTail = 0;
				for (j = 1;j<ncomps;j++)
				{
					lTail += (subValues[j])[ii[j]];
				}
			}
			l += lTail;
			if (arg1Type == CHAR)
			{
				itemlistP[i] = l;
			}
			else
			{
				DATAVALUE(result,i) = DATAVALUE(dest,l) = DATAVALUE(arg2,i2);
				i2 += i2inc;
			}

			for (j=0;j<ncomps;j++)
			{ /* turn over odometer */
				if (++ii[j] < resdim[j])
				{
					break;
				}
				ii[j] = 0;
				lTail = -1;
			} /*for (j=0;j<ncomps;j++)*/
		} /*for (i=0;i<nitems;i++)*/
	} /*if (matrixSubscr){}else{}*/

	if (arg1Type == CHAR)
	{
		/*
		  At this point *itemlistH contains all the offsets in the destination
		  */
		/* compute space needed and allocate*/
		space = 0;
		ch1 = STRINGPTR(arg1);
		jLast = -1;
		resSpace = 0;
		for (i = 0; i < arg1tot; i++)
		{ /* compute space needed */
			for (j = nitems - 1;j >= 0; j--)
			{ /* see if string i is to be replaced */
				if (itemlistP[j] == i)
				{
					break;
				}
			} /*for (j = 0;j < nitems; j++)*/

			if (j < 0)
			{ /* string i not to be replaced */
				length = strlen(ch1) + 1;
				ch1 += length;
			} /*if (j < 0)*/
			else
			{ /* string i will be replaced */
				ch1 = skipStrings(ch1,1L);
				if (arg2tot == 1)
				{
					j = 0;
				}
				if (j < jLast || jLast < 0)
				{
					ch2 = STRINGPTR(arg2);
					jLast = 0;
				}

				if (j > jLast)
				{
					ch2 = skipStrings(ch2,j-jLast);
					jLast = j;
				}

				length = strlen(ch2) + 1;
				resSpace += length;
			} /*if (j < 0){}else{}*/
			space += length;
		} /*for (i = 0; i < arg1tot; i++)*/

		if (!getScratch(cNewArg1H,GCNEWARG1H,space,char) ||
			!getScratch(cResultH,GCRESULTH,resSpace,char))
		{
			goto errorExit;
		}
		cNewArg1P = *cNewArg1H;
		cResultP = *cResultH;

		ch1 = STRINGPTR(arg1);
		jLast =- 1;
		for (i = 0; i < arg1tot; i++)
		{ /* now fill it up */
			for (j = nitems-1;j >= 0; j--)
			{
				if (itemlistP[j] == i)
				{
					break;
				}
			} /*for (j = nitems-1;j >= 0; j--)*/
			if (j < 0)
			{ /* string i not to be replaced and hence not part of result*/
				cNewArg1P = copyStrings(ch1, cNewArg1P, 1);
				ch1 = skipStrings(ch1, 1);
			} /*if (j < 0)*/
			else
			{ /* string i to be replaced and new piece added to result*/
				if (arg2tot == 1)
				{
					j = 0;
				}

				ch1 = skipStrings(ch1,1L); /* skip string being replaced */
				if (j < jLast || jLast < 0)
				{
					ch2 = STRINGPTR(arg2);
					jLast = 0;
				}

				if (j > jLast)
				{
					ch2 = skipStrings(ch2,j-jLast);
					jLast = j;
				}

				cNewArg1P = copyStrings(ch2, cNewArg1P, 1);
				cResultP = copyStrings(ch2, cResultP, 1);
			} /*if (j < 0){}else{}*/
		} /*for (i = 0; i < arg1tot; i++)*/
		mydisphandle((char **) STRING(dest));
		unTrash(GCNEWARG1H);
		setSTRING(dest,cNewArg1H);
		unTrash(GCRESULTH);
		setSTRING(result,cResultH);
	} /*if (arg1Type == CHAR)*/

	if (!matrixSubscr)
	{
		setNDIMS(result,ncomps);
		for (j=0;j<ncomps;j++)
		{
			setDIM(result,j+1,resdim[j]);
		}
	}
	else
	{ /* result is vector */
		setNDIMS(result,1);
		setDIM(result,1, nitems);
	}
#if (USENOMISSING)
	if (NOMISSING(arg2))
	{
		setNOMISSING(result);
	}
	else
	{
		clearNOMISSING(result);
		clearNOMISSING(dest);
	}
#endif /*USENOMISSING*/
	
	if (sameSymbol)
	{
		Removesymbol(arg1);
	}
	else if (isscratch(NAME(arg2)))
	{
		Removesymbol(arg2);
	}

	emptyTrash();

	return (result);

  returnNull:
/*
  right side was either null or
  right side was a scalar and one or more subscripts were NULL or
  selected no elements (all F or -run(dimlength))
*/
	if (isscratch(NAME(arg2)))
	{
		/* re-use arg2 */
		result = arg2;
		if (rightIsScalar)
		{
			DeleteContents(arg2);
			setTYPE(arg2, NULLSYM);
		}
		setNAME(arg2, NULLSCRATCH);
	}
	else
	{
		result = Install(SCRATCH, NULLSYM);
		if (result == (Symbolhandle) 0)
		{
			goto errorExit;
		}
	}
	return (result);

  yyerrorMsg:
	yyerror(OUTSTR);
	/* fall through */
  errorExit:
	putOUTSTR();

	if (sameSymbol && dest != (Symbolhandle) 0)
	{
		Removesymbol(dest);
		Cutsymbol(arg1); /* preserve arg1 */
		setNAME(arg1,namearg1);
		Addsymbol(arg1);
	}
	Removesymbol(result);
	emptyTrash();
	return (0);
} /*Subassign()*/


