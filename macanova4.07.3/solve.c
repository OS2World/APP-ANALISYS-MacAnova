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
#pragma segment Solve
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

#include "globals.h"
#include "mainpars.h"
#include "blas.h"   /* prototypes and macros for names */
#include "linpack.h"   /* prototypes and macros for names */

/* 
  Routine to compute solve(a), solve(a,b), and det(a), where a is square
*/

enum funcCodes
{
	IDET = 0,
	ISOLVE,
	IRSOLVE,
	IINVERSE
};

enum scratchCodes
{
	GIPVT = 0,
	GSCRATCH,
	NTRASH
};

/*
   Routine to implement solve(a), solve(a,b) (a %\% b),
   rsolve(a,b) (b %/% a), det(a), det(x,mantexp:T)
   960320 If a has labels, solve(a) has same labels)
   960329 If a and b have labels, solve(a,b) has labels
      structure(getlabels(a,1),getlabels(b,2) and rsolve(a,b) has
	  labels structure(getlabels(b,1),getlabels(a,2))	  
   971126 Modified to use append codes in labels.h
   980819 added keyword phrase mantexp:T to det().
   981227 added keyword phrase singok:T to solve().  When present,
          solve(a) and solve(a,x) return NULLSYMBOL when a is singular
          Fixed bug for det(a,mantexp:F) which produced a vector
          instead of a scalar.
          Added keyword phrase 'quiet:T' to det() to suppress warning
          message when argument is singular
          det(a) and det(a,mantexp:T) always return 0 when a is singular
*/
Symbolhandle solve(Symbolhandle list)
{
	Symbolhandle     result = (Symbolhandle) 0;
	Symbolhandle     arg1, arg2;
	Symbolhandle     symhKey = (Symbolhandle) 0;
	double         **scratchH;
	long           **ipvtH;
	long             op;
	long             nargs = NARGS(list);
	long             margs = 1, dim[2], nrows, ncols;
	long             j;
	long             needed;
	long             length, inc1 = 1;
	long            *ipvt;
	long             job;
	int              mantexp = 0, matop;
	int              singOK = 0, quiet = 0;
	double          *scratch, *a, *work, *x, *b, *tmprow;
	double           rcond, determ[2]; 
	char            *keyword, *leftRight;
	WHERE("solver");
	TRASH(NTRASH,errorExit);

	*OUTSTR = '\0';

	matop = FUNCNAME[0] == '%';

	if(strcmp(FUNCNAME,"det") == 0)
	{
		op = IDET;
		margs = -3;
	}
	else if(strcmp(FUNCNAME,"solve") == 0 ||
			strcmp(FUNCNAME, opName(DIVMAT)) == 0)
	{
		if (!matop && nargs > 1 &&
			(keyword = isKeyword(COMPVALUE(list, nargs - 1))))
		{
			if (strcmp(keyword, "singok") != 0)
			{
				badKeyword(FUNCNAME, keyword);
				goto errorExit;
			}
			symhKey = COMPVALUE(list, nargs - 1);
			if (!isTorF(symhKey))
			{
				notTorF(keyword);
				goto errorExit;
			}
			singOK = (DATAVALUE(symhKey, 0) != 0.0);
			nargs--;
		}
		op = (nargs == 1) ? IINVERSE : ISOLVE;
		margs = (op == IINVERSE) ? 1 : -2;
	}
	else
	{
		op = IRSOLVE;
		margs = 2;
	}

	if (matop)
	{
		leftRight = (op == ISOLVE) ? "left" : "right";
	}

	if (margs > 0 && nargs != margs || margs < 0 && nargs > -margs)
	{
		badNargs(FUNCNAME,margs);
		goto errorExit;
	}

	arg1 = COMPVALUE(list,0);
	if(!argOK(arg1,REAL,(nargs > 1) ? 1 : 0))
	{
		goto errorExit;
	}

	if (!isMatrix(arg1, dim) || dim[0] != dim[1])
	{
		if (!matop)
		{
			sprintf(OUTSTR,
					"ERROR: %sargument to %s() not square matrix",
					(nargs > 1) ? "1st " : NullString, FUNCNAME);
		}
		else
		{
			sprintf(OUTSTR,
					"ERROR: %s operand of %s is not square matrix",
					leftRight, FUNCNAME);
		}
	} /*if (!isMatrix(arg1, dim) || dim[0] != dim[1])*/
	else if(anyMissing(arg1) && !matop)
	{
		sprintf(OUTSTR,"ERROR: missing values in 1st argument to %s()",
				FUNCNAME);
	}
	if(*OUTSTR)
	{
		goto errorExit;
	}

	nrows = dim[0];

	if(op == IDET)
	{
		for (j = 1; j < nargs; j++)
		{
			char             *mantexpKey = "mantexp";
			char             *quietKey = "quiet";

			symhKey = COMPVALUE(list, j);
			keyword = isKeyword(symhKey);

			if (keyword == (char *) 0)
			{
				sprintf(OUTSTR,
						"ERROR: argument %ld to %s() must be '%s:T' or '%s:T'",
						j + 1, FUNCNAME, mantexpKey, quietKey);
				goto errorExit;
			}
			
			if (strcmp(keyword, mantexpKey) != 0 &&
				strcmp(keyword, quietKey) != 0)
			{
				badKeyword(FUNCNAME, keyword);
				goto errorExit;
			}
			
			if (!isTorF(symhKey))
			{
				notTorF(keyword);
				goto errorExit;
			}
			if (keyword[0] == mantexpKey[0])
			{
				mantexp = (DATAVALUE(symhKey, 0) != 0.0);
			}
			else
			{
				quiet = (DATAVALUE(symhKey, 0) != 0.0);
			}
		} /*for (j = 1; j < nargs; j++)*/
		needed = (mantexp) ? 2 : 1;
		ncols = 1;
	} /*if(op == IDET)*/
	else
	{
		if(op == ISOLVE || op == IRSOLVE)
		{
			arg2 = COMPVALUE(list,1);
			if(!argOK(arg2, REAL, 2))
			{
				goto errorExit;
			}
			if (!isMatrix(arg2,dim) ||
				nrows != ((op == ISOLVE) ? dim[0] : dim[1]))
			{
				if (matop)
				{
					sprintf(OUTSTR,
							"ERROR: dimensions do not match for %s",
							FUNCNAME);
				}
				else
				{
					sprintf(OUTSTR,
							"ERROR: 2nd argument to %s() not matrix with same number of %s as 1st",
							FUNCNAME, (op == ISOLVE) ? "rows" : "cols");
				}
			}
			else if (matop && (anyMissing(arg1) || anyMissing(arg2)))
			{
				sprintf(OUTSTR,
						"ERROR: missing values in operand of %s", FUNCNAME);
			}
			else if(anyMissing(arg2))
			{
				sprintf(OUTSTR,"ERROR: missing values in 2nd argument to %s()",
						FUNCNAME);
			}
			if(*OUTSTR)
			{
				goto errorExit;
			}
			ncols = (op == ISOLVE) ? dim[1] : dim[0];
		} /*if(op == ISOLVE || op == IRSOLVE)*/
		else
		{
			ncols = nrows;
		}
		needed = nrows * ncols;
	} /*if(op == IDET){}else{}*/

	result = RInstall(SCRATCH, needed);
	if(result == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	if (op != IDET)
	{
		setNDIMS(result,2);
		setDIM(result,1, (op != IRSOLVE) ? nrows : ncols);
		setDIM(result,2, (op != IRSOLVE) ? ncols : nrows);

		if (HASLABELS(arg1))
		{
			char      *labs1[2], *labs2[2];
			long       lengths1[2], lengths2[2];

			getMatLabels(arg1, labs1, lengths1);

			if (op == IINVERSE)
			{
				long          tmp = lengths1[0];

				lengths1[0] = lengths1[1];
				lengths1[1] = tmp;

				TMPHANDLE = createLabels(2, lengths1);
				if (TMPHANDLE == (char **) 0 || !setLabels(result, TMPHANDLE))
				{
					mydisphandle(TMPHANDLE);
					goto errorExit;
				}
				/* Swap labels */
				appendLabels(result, labs1[1], 0, dontExpand);
				appendLabels(result, labs1[0], 1, dontExpand);
			} /*if (op == IINVERSE)*/
			else
			{
				char        numericLabel[2];
				char       *tmpLab;

				if (HASLABELS(arg2))
				{
					getMatLabels(arg2, labs2, lengths2);
				}
				else
				{
					numericLabel[0] = NUMERICLABEL;
					numericLabel[1] = '\0';
					labs2[0] = labs2[1] = numericLabel;
					lengths2[0] = lengths2[1] = 2*ncols;
				}

				if (op == ISOLVE)
				{ /* a %\% b*/
					lengths1[1] = lengths2[1];
				}
				else
				{ /* a %/% b*/
					lengths1[0] = lengths2[0];
				}

				TMPHANDLE = createLabels(2, lengths1);
				if (TMPHANDLE == (char **) 0 || !setLabels(result, TMPHANDLE))
				{
					mydisphandle(TMPHANDLE);
					goto errorExit;
				}
				getMatLabels(arg1, labs1, lengths1);
				if (HASLABELS(arg2))
				{
					getMatLabels(arg2, labs2, lengths2);
				}

				tmpLab = (op == ISOLVE) ? labs1[1] : labs2[0];
				appendLabels(result, tmpLab, 0,
							 (tmpLab == numericLabel) ? doExpand : dontExpand);
				tmpLab = (op == ISOLVE) ? labs2[1] : labs1[0];
				appendLabels(result, tmpLab, 1,
							 (tmpLab == numericLabel) ? doExpand : dontExpand);
			} /*if (op == IINVERSE)else{}*/
		} /*if (HASLABELS(arg1))*/	
	} /*if (op != IDET)*/

	needed = (op == IINVERSE) ? nrows : nrows * (nrows + 1);

	if(!getScratch(ipvtH,GIPVT,nrows,long) ||
	   !getScratch(scratchH,GSCRATCH,needed,double))
	{
		goto errorExit;
	}

	ipvt = *ipvtH;
	scratch = *scratchH;
	work = scratch;
	scratch += nrows;

	a = (op == IINVERSE) ? DATAPTR(result) : scratch;

	x = DATAPTR(arg1);

	if (op != IRSOLVE)
	{
		length = nrows * nrows;
		DCOPY(&length, x, &inc1, a, &inc1);
	} /*if (op != IRSOLVE)*/
	else
	{ /* copy transpose(arg1) */
		for (j = 0; j < nrows; j++)
		{
			DCOPY(&nrows, x + j, &nrows, a + j*nrows, &inc1);
		}
	} /*if (op != IRSOLVE){}else{}*/


	DGECO(a, &nrows, &nrows, ipvt, &rcond, work);
	if(interrupted(DEFAULTTICKS) != INTNOTSET)
	{
		goto errorExit;
	}

	if((1.0 + rcond) == 1.0)
	{
		char        *howSing = (rcond) ? "apparently " : NullString;

		if (singOK)
		{
			Removesymbol(result);
			result = NULLSYMBOL;
			goto normalExit;
		}

		if (!quiet)
		{
			if (!matop)
			{
				sprintf(OUTSTR,
						"%s: argument to %s() is %ssingular",
						(op == IDET) ? "WARNING" : "ERROR",
						FUNCNAME, howSing);
			} /*if (!matop)*/
			else
			{
				sprintf(OUTSTR,
						"ERROR: %s operand to %s is %ssingular",
						leftRight, FUNCNAME, howSing);
			} /*if (!matop){}else{}*/		
			putErrorOUTSTR();
		} /*if (!quiet)*/

		if(op != IDET)
		{
			goto errorExit;
		}
		else
		{
			rcond = 0.0;
			DATAVALUE(result,0) = 0.0;
			if (mantexp)
			{
				DATAVALUE(result, 1) = 0.0;
			}
		}
	} /*if((1.0 + rcond) == 1.0)*/

	if(op == IINVERSE || op == IDET && rcond != 0.0)
	{
		job = (op == IDET) ? 10 : 1;
		DGEDI(a, &nrows, &nrows, ipvt, determ, work, &job);
		if(interrupted(DEFAULTTICKS) != INTNOTSET)
		{
			goto errorExit;
		}
		if(op == IDET)
		{
			if (!mantexp)
			{
				DATAVALUE(result,0) = determ[0] * intpow(10.0, determ[1]);
			}
			else
			{
				DATAVALUE(result, 0) = determ[0];
				DATAVALUE(result, 1) = (determ[0] != 0.0) ? determ[1] : 0.0;
			}
		} /*if(op == IDET)*/		
	} /*if(op == IINVERSE || op == IDET && rcond != 0.0)*/
	else if(op == ISOLVE || op == IRSOLVE)
	{
		b = DATAPTR(arg2);
		x = DATAPTR(result);
		/* solve column by column (row by row for rsolve() )*/
		for (j = 0;j < ncols;j++)
		{
			tmprow = (op == ISOLVE) ? x : work;
			DCOPY(&nrows, b, (op == ISOLVE) ? &inc1 : &ncols, tmprow, &inc1);
			job = 0;
			DGESL(a, &nrows, &nrows, ipvt, tmprow, &job);
			if(interrupted(DEFAULTTICKS) != INTNOTSET)
			{
				goto errorExit;
			}
			if (op == ISOLVE)
			{
				x += nrows;
				b += nrows;
			}
			else
			{ /* put in row of output */
				DCOPY(&nrows, tmprow, &inc1, x, &ncols);
				x++;
				b++;
			}
		} /*for(j=0;j<ncols;j++)*/
	}

  normalExit:
	emptyTrash();

#ifdef SEGMENTED
	UNLOADSEG(DDOT);
	UNLOADSEG(DGECO);
#endif /*SEGMENTED*/

#if defined(HASNAN) || defined(HASINFINITY)
	if (result != NULLSYMBOL && anyNaN(result))
	{
		sprintf(OUTSTR,
				"WARNING: elements undefined due to overflow in result of %s() set to MISSING",
				FUNCNAME);
		putErrorOUTSTR();
	}
#endif /*HASNAN || HASINFINITY*/

	return(result);

  errorExit:
	putErrorOUTSTR();
	emptyTrash();
	Removesymbol(result);

#ifdef SEGMENTED
	UNLOADSEG(DDOT);
	UNLOADSEG(DGECO);
#endif /*SEGMENTED*/

	return (0);
} /*solve()*/
