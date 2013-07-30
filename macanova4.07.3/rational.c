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
#pragma segment Rational
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

/*
  990226 removed unecessary includes and replaced putOUTSTR() by
         putErrorOUTSTR()
*/
enum rationalErrors
{
	FOUNDPOLE         = 0x01,
	FOUNDINFINITE     = 0x02,
	FOUNDZEROOVERZERO = 0x04
};


/* Note: top is not referenced if p == 0, and bottom is not if q == 0 */

static Symbolhandle doRational(Symbolhandle symhx, Symbolhandle symhcoefs,
							   double *orders,
							   unsigned long control, unsigned long *status)
{
	long         i, j;
	double       topval = 1.0, bottomval = 1.0, xval, ratio;
	long         p = (long) orders[0] - 1, q = (long) orders[1] - 1;
	long         n = symbolSize(symhx);
	long         ndims = NDIMS(symhx);
	double      *x;
	double      *top = (double *) 0, *bottom = (double *) 0, *ans;
	Symbolhandle result = (Symbolhandle) 0;
	WHERE("doRational");
	
	if(control & REUSELEFT)
	{
		result = symhx;
	} /*if(control & REUSELEFT)*/
	else
	{
		result = (n > 0) ? RInstall(SCRATCH,n) : Install(NULLSCRATCH, NULLSYM);
		if(result == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		if (n > 0)
		{
			Setdims(result, ndims, &DIMVAL(symhx, 1));
		} /*if (n > 0)*/

		if (HASLABELS(symhx) && !transferLabels(symhx, result))
		{
			goto errorExit;
		}
	} /*if(control & REUSELEFT){}else{}*/

	if (n > 0)
	{
		setNCLASS(result, -1);

		x = DATAPTR(symhx);
		ans = DATAPTR(result);

		if(COMPVALUE(symhcoefs,0) != (Symbolhandle) 0)
		{
			top = DATAPTR(COMPVALUE(symhcoefs,0));
		}
		if(COMPVALUE(symhcoefs,1) != (Symbolhandle) 0)
		{
			bottom = DATAPTR(COMPVALUE(symhcoefs,1));
		}
	
		for (i = 0;i < n;i++)
		{
			xval = x[i];
			if(isMissing(xval))
			{
				setMissing(ratio);
			} /*if(isMissing(xval))*/
			else
			{
				if(p >= 0)
				{
					topval = 0.0;
					if(xval != 0.0)
					{
						for (j = p; j > 0;j--)
						{
							topval = (topval + top[j])*xval;
						}
					} /*if(xval != 0.0)*/
					topval += top[0];
#ifdef HASINFINITY
					if (isInfinite(topval))
					{
						setMissing(ratio);
						*status |= FOUNDINFINITE;
					}
#endif /*HASINFINITY*/
				} /*if(p >= 0)*/

				if (!isMissing(ratio))
				{
					if(q >= 0)
					{
						bottomval = 0.0;
						if(xval != 0.0)
						{
							for(j=q;j>0;j--)
							{
								bottomval = (bottomval + bottom[j])*xval;
							}
						} /*if(xval != 0.0)*/
				
						bottomval += bottom[0];
					} /*if(q >= 0)*/

#ifdef HASINFINITY
					if (isInfinite(bottomval))
					{
						/* finite/infinite */
						ratio = 0.0;
					}
					else
#endif /*HASINFINITY*/
					if(bottomval != 0.0)
					{ /* finite/finit*/
						double       top = fabs(topval);
						double       bot = fabs(bottomval);

						if (top > 1 && 1.0/bot < TOOBIGVALUE/top ||
							(top <= 1.0 && 
							 (bot >= 1.0 || top < bot * TOOBIGVALUE)))
						{
							ratio = topval/bottomval;
						}
						else
						{
							setMissing(ratio);
							*status = FOUNDINFINITE;
						}
					}
					else
					{
						setMissing(ratio);
						*status |= (topval != 0.0) ?
						  FOUNDPOLE : FOUNDZEROOVERZERO;
					}
				} /*if (!isMissing(ratio))*/
			} /*if(isMissing(xval)){}else{}*/
			ans[i] = ratio;
		} /*for (i = 0;i < n;i++)*/
	} /*if (n > 0)*/	

	return (result);
		
  errorExit:
	return (0);
} /*doRational()*/

/* 
  usage: rational(x,a1,a2)  where a1 and a2 are vectors
  Computes P1(x)/P2(x) where Pi(x) = 1 - sum(ai*x^run(length(ai))), i = 1,2
  If a2 is omitted, computes P1(x)
*/

Symbolhandle rational(Symbolhandle list)
{
	Symbolhandle   x, symh;
	Symbolhandle   result = (Symbolhandle) 0;
	Symbolhandle   coefs = (Symbolhandle) 0;
	Symbolhandle   arg[2];
	double              orders[2];
	long                p;
	long                nargs = NARGS(list);
	long                i;
	unsigned long       control = 0;
	unsigned long       status = 0;
	char               *warningMsg =
	  "WARNING: %s found by %s(); value(s) set to MISSING";
	WHERE("rational");
	
	*OUTSTR = '\0';
	
	if(nargs > 3)
	{
		badNargs(FUNCNAME,-3);
		goto errorExit;
	}
	
	x = COMPVALUE(list,0);
	if(!argOK(x,0,1))
	{
		goto errorExit;
	}

	if(TYPE(x) == STRUC)
	{
		control |= LEFTRECUR;

		if(getSingleType(x) != REAL)
		{
			sprintf(OUTSTR,
					"ERROR: at least one component of first argument to %s is not REAL",
					FUNCNAME);
		}
		if(*OUTSTR)
		{
			goto errorExit;
		}
	} /*if(TYPE(x) == STRUC)*/
	else if(TYPE(x) != REAL)
	{
		badType(FUNCNAME,-TYPE(x),1);
		goto errorExit;
	}

	arg[0] = arg[1] = (Symbolhandle) 0;
	orders[0] = orders[1] = 0.0;

	/* 
	   Check coefficient arguments; must be null or REAL vectors with
	   no MISSING values
	*/
	for(i=1;i<nargs;i++)
	{
		arg[i-1] = symh = COMPVALUE(list,i);
		if(symh != (Symbolhandle) 0)
		{
			if(!isDefined(symh))
			{
				undefArg(FUNCNAME,symh,i+1);
				goto errorExit;
			}
			
			if(TYPE(symh) != REAL || !isVector(symh))
			{
				sprintf(OUTSTR,
						"ERROR: coefficients for %s must be REAL vectors",
						FUNCNAME);
				goto errorExit;
			}
			if(anyMissing(symh))
			{
				sprintf(OUTSTR,
						"ERROR: MISSING values in coefficient vectors for %s",
						FUNCNAME);
				goto errorExit;
			}
		} /*if(symh != (Symbolhandle) 0)*/
	} /*for(i=1;i<nargs;i++)*/
	
	coefs = StrucInstall(SCRATCH,2);
	if(coefs == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	for(i=0;i<2;i++)
	{
		if((symh = arg[i]) != (Symbolhandle) 0)
		{
			p = symbolSize(symh);
			if(p == 1 && DATAVALUE(symh,0) == 1.0)
			{
				p = 0;
			}
			else
			{
				COMPVALUE(coefs,i) = Makesymbol(REAL);
				if(COMPVALUE(coefs,i) == (Symbolhandle) 0 ||
				   Copy(symh,COMPVALUE(coefs,i)) == 0)
				{				/* make copy of coefficients */
					goto errorExit;
				}
				setNAME(COMPVALUE(coefs, i), SCRATCH);
				clearLabels(COMPVALUE(coefs, i));
				clearNotes(COMPVALUE(coefs, i));
			}			
		} /*if((symh = arg[i]) != (Symbolhandle) 0)*/
		else
		{
			p = 0;
		}
		orders[i] = (double) p;
	}

	if(isscratch(NAME(x)))
	{
		control |= REUSELEFT;
		COMPVALUE(list,0) = (Symbolhandle) 0;
	}
	
	result = doRecur2(doRational, x, coefs, orders, control, &status);

	if(result == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	Removesymbol(coefs);

	if (status & FOUNDPOLE)
	{
		sprintf(OUTSTR, warningMsg, "zero denominator(s)", FUNCNAME);
		putErrorOUTSTR();
	}
	if (status & FOUNDZEROOVERZERO)
	{
		sprintf(OUTSTR, warningMsg, "zero/zero", FUNCNAME);
		putErrorOUTSTR();
	}
	if (status & FOUNDINFINITE)
	{
		sprintf(OUTSTR, warningMsg, "too large ratio(s)", FUNCNAME);
		putErrorOUTSTR();
	}

	return(result);
	
  errorExit:
	putErrorOUTSTR();

	Removesymbol(coefs);
	Removesymbol(result);
	return (0);
	
}

