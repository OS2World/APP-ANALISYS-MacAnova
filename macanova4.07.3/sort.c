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
#pragma segment Columnop
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

/*
  Replaced putOUTSTR() by putErrorOUTSTR()
*/
typedef struct opEntry
{
	char *name;
	short iop;
	short nargs;
} opEntry;

enum sortOpCodes
{
	IISMISSING = 0,
	IANYMISSING,
	IRANKITS,
	IHALFNORM,
	IRANK,
	IGRADE,
	ISORT
};

#define          UP           0x08
#define          GRADEONLY    0x00
#define          TIESIGNORE   0x10
#define          TIESMEAN     0x20
#define          TIESMIN      0x30

#define          SORTOPMASK   0x07
#define          TIEMASK      0x30

static opEntry Ops[] =
{ /* name        iop      nargs */
	{"ismissing",  IISMISSING,     1}, /* ismissing(x) */
	{"anymissing", IANYMISSING,    1}, /* anymissing(x) */
	{"rankits",    IRANKITS,      -2}, /* rankits(x [,ties:"i|a|m"]) */
	{"halfnorm",   IHALFNORM,     -2}, /* halfnorm(x [,ties:"i|a|m"]) */
	{"rank",       IRANK,         -3}, /* rank(x [,down:T] [,ties:"i|a|m"]) */
	{"grade",      IGRADE,        -2}, /* grade(x [,down:T]) */
	{"sort",       ISORT,         -2}, /* sort(x [,down:T) */
	{(char *) 0,   -1,              0}
};

enum sortScratch
{
	GX2 = 0,
	GRANKS,
	GSCR,
	GCHARSCR,
	NTRASH
};

/*
  971104 added check for value of myhandlelength()
*/
static Symbolhandle doSortOps(Symbolhandle arg1, double *params,
							  unsigned long control, unsigned long *status)
{
	Symbolhandle    result = (Symbolhandle) 0;
	double        **x2H = (double **) 0;
	double        **ranksH = (double **) 0;
	double        **scrH = (double **) 0;
	double         *x, *x2, *scr, *ranks, *answer;
	double          y, missVal = HUGEDBL;
	double          denom;
	long            i, ierr, n, n2, ndims;
	long            size = symbolSize(arg1);
	long            j;
	long            iop = control & SORTOPMASK;
	long            up = control & UP;
	long            rankOp = control & TIEMASK;
	long            ncols;
	long            ischar = (TYPE(arg1) == CHAR);
	char           *cplace, *cplace1;
	char         ***charxH = (char ***) 0;
	char          **charx = (char **) 0;
	char           *string;
	WHERE("doSortOps");
	TRASH(NTRASH,errorExit); /* allocate trash basket for scratch */

	size = symbolSize(arg1);

	if (control & REUSELEFT)
	{
		result = arg1;
	} /*if (control & REUSELEFT)*/
	else
	{
		if (iop == IANYMISSING)
		{
			result = RInstall(SCRATCH, 1);
		}
		else
		{
			if (size == 0)
			{
				result = Install(NULLSCRATCH, NULLSYM);
			}
			else if (!ischar || iop != ISORT)
			{
				result = RInstall(SCRATCH, size);
			}
			else
			{
				long      handleLength = myhandlelength(STRING(arg1));
				
				if (handleLength < 0)
				{
					goto errorExit;
				}
				
				result = CInstall(SCRATCH, handleLength);
			}
		}
		if (result == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		if (iop == IANYMISSING)
		{
			setTYPE(result, LOGIC);
			DATAVALUE(result, 0) = 0.0;
		}
	} /*if (control & REUSELEFT){}else{}*/	

	setNCLASS(result, -1);

	if (size > 0)
	{		
		if (iop != IANYMISSING)
		{
			ndims = NDIMS(arg1);
			n = DIMVAL(arg1,1);
			ncols = size/n;
			if (!(control & REUSELEFT))
			{
				setNDIMS(result,ndims);
				for (i = 1; i <= ndims; i++)
				{
					setDIM(result,i,DIMVAL(arg1,i));
				}
			}		
		} /*if (iop != IANYMISSING)*/
	
		if (iop == IISMISSING)
		{
			setTYPE(result,LOGIC);

			answer = DATAPTR(result);
			if (!ischar)
			{
				x = DATAPTR(arg1);
				for (i = 0; i < size; i++)
				{
					answer[i] = (isMissing(x[i])) ? 1.0 : 0.0;
				}
			}
			else
			{
				string = STRINGPTR(arg1);
				for (i = 0; i < size; i++)
				{
					answer[i] = (*string == '\0') ? 1.0 : 0.0;
					string += strlen(string) + 1;
				}
			}
		} /*if (iop == IISMISSING)*/
		else if (iop == IANYMISSING)
		{
			DATAVALUE(result,0) = (anyMissing(arg1)) ? 1.0 : 0.0;
		}
		else
		{/* IRANKITS, IHALFNORM, IRANK, IGRADE, ISORT */
			if (iop != ISORT)
			{
				if (!getScratch(x2H,GX2,n,double) ||
					!getScratch(scrH,GSCR,n,double) ||
					!getScratch(ranksH,GRANKS,n,double))
				{
					goto errorExit;
				}
			} /*if (iop != ISORT)*/

			if (ischar && !getScratch(charxH,GCHARSCR, n, char *))
			{
				goto errorExit;
			}

			if (iop != ISORT)
			{
				x2 = *x2H;
				scr = *scrH;
				ranks = *ranksH;
			}
			if (iop != ISORT || !ischar)
			{
				answer = DATAPTR(result);
			}
			
			if (ischar)
			{
				charx = *charxH;
				cplace = STRINGPTR(arg1);
				if (iop == ISORT)
				{
					cplace1 = STRINGPTR(result);
				}
			}
			else
			{
				/* if reusing arg1, x and answer may be the same */
				x = DATAPTR(arg1);
			}

			/*
			   Treatment of MISSING:

			   "" does not count as MISSING for CHARACTER data (only
			   on anymissing())

			   For rankits() and halfnorm(), the values computed use the
			   number of non-missing values as sample size, with MISSING
			   whereever the input is MISSING.
			   
			   For rank(), the ranks(), the rank of a MISSING value is set
			   to MISSING, the maximum rank of non-missing values being the
			   number of such values, regardless of the direction of the
			   sort.
			   
			   For sort(), MISSING values are always sorted to the end,
			   regardless of the direction of the sort.
			   
			   For grade(), if there are k MISSING values, the last k elements
			   in a column of the result are the row numbers of the MISSING
			   values; the first nrows-k values are the row numbers of the
			   order statistics of the non-missing values.  Thus if x is a
			   vector, x[grade(x,down:T)] should be the same as sort(x,down:T)
			   whether down is T or F.
			*/

			for (j = 0; j < ncols;j++)
			{
				if (!ischar)
				{
					if (iop == ISORT)
					{
						x2 = answer;
					}
					n2 = 0;
					for (i = 0; i < n; i++)
					{/* transform MISSING to HUGEDBL */
						y = x[i];
						if (isMissing(y))
						{/* should always sort to the end */
							y = missVal;
							*status |= FOUNDMISSING;
						}
						else
						{
							n2++; /* number of non-missing values */
							y = (up) ? y : -y;
						}
						x2[i] = y;
					} /*for (i = 0; i < n; i++)*/
				} /*if (!ischar)*/
				else
				{
					for (i = 0; i < n; i++)
					{
						charx[i] = cplace;
						cplace = skipStrings(cplace, 1);
					}
				} /*if (!ischar){}else{}*/
				
				/* Only sort(), rank(), and grade() can have CHARACTER arg*/
				switch (iop)
				{
				  case IRANKITS:
					rankquick(x2,ranks,scr,n,rankOp);
					denom = (double) n2 + 0.25;
					for (i = 0; i < n; i++)
					{
						if (isMissing(x[i]))
						{
							setMissing(answer[i]);
						}
						else
						{
							answer[i] = ppnd((ranks[i] - 0.375) / denom, &ierr);
						}
					} /*for (i = 0; i < n; i++)*/
					break;

				  case IHALFNORM:
					for (i = 0; i < n; i++)
					{
						if (!isMissing(x[i]))
						{
							x2[i] = fabs(x2[i]);
						}
					} /*for (i = 0; i < n; i++)*/

					rankquick(x2, ranks, scr, n, rankOp);

					denom  = 2.0*n2 + 0.5;
					for (i = 0; i < n; i++)
					{
						if (isMissing(x[i]))
						{
							setMissing(answer[i]);
						}
						else
						{
							answer[i] = ppnd(0.5 + (ranks[i] - 0.375) / denom,
											 &ierr);
						}
					} /*for (i = 0; i < n; i++)*/
					break;

				  case IRANK:
				  case IGRADE:
					if (!ischar)
					{
						rankquick(x2, ranks, scr, n, rankOp);
						if (iop == IRANK)
						{
							for (i = 0; i < n; i++)
							{
								if (isMissing(x[i]))
								{
									setMissing(answer[i]);
								}
								else
								{
									answer[i] = ranks[i];
								}
							} /*for (i = 0; i < n; i++)*/
						} /*if (iop == IRANK)*/
						else
						{/*IGRADE*/
							for (i=0;i<n;i++)
							{
								answer[i] = ranks[i];
							}
						} /*if (iop == IRANK){...}else{...}*/
					} /*if (!ischar)*/
					else
					{
						rankquickchar(charx, ranks, scr, n, rankOp);
						if (up)
						{
							for (i=0;i<n;i++)
							{
								answer[i] = ranks[i];
							}
						}
						else if (iop == IRANK)
						{
							double      dn1 = (double) n + 1.0;
							for (i=0;i<n;i++)
							{
								answer[i] = dn1 - ranks[i];
							}
						}
						else
						{
							for (i = 0; i < n; i++)
							{
								answer[i] = ranks[n-1-i];
							}
						}
					} /*if (!ischar){}else{}*/
					
					break;

				  case ISORT:
					if (!ischar)
					{
						sortquick(answer, n);
						if (!up)
						{
							for (i = 0; i < n2; i++)
							{
								answer[i] = -answer[i];
							}
						}
						for (i = n2; i < n; i++)
						{
							setMissing(answer[i]);
						}
					} /*if (!ischar)*/
					else
					{
						sortquickchar(charx, n);
						for (i = 0; i < n; i++)
						{
							cplace1 = copyStrings(charx[(up) ? i : n-1-i],
												 cplace1, 1);
						} /*for (i = 0; i < n; i++)*/
					} /*if (!ischar){}else{}*/			
					break;
				} /* switch (iop) */
				
				if (!ischar)
				{
					x += n;
				}
				if (!ischar || iop != ISORT)
				{
					answer += n;
				}
			} /*for (j = 0; j < ncols;j++)*/
		} /* if ... else */
	} /*if (size > 0)*/
			 
	emptyTrash();
	
	return (result);
	
  errorExit:
	Removesymbol(result);
	emptyTrash();
	return (0);
} /*doSortOps()*/

Symbolhandle    sort(Symbolhandle list)
{

	Symbolhandle    arg1, symhkey, result;
	long            up = 1;
	long            iop, margs, nargs = NARGS(list);
	long            missingOp, rankOp, charLegal;
	unsigned long   control = 0, status = 0;
	long            type, type1;
	long            jarg;
	long            ndims;
	char           *keyword, c;
	opEntry        *op;
	WHERE("sort");
	
	*OUTSTR = '\0';
	for (op = Ops; op->iop >= 0 && strcmp(op->name,FUNCNAME) != 0; op++)
	{
		;
	}
	
	if (op->iop < 0)
	{ /* should never occur unless programming error */
		sprintf(OUTSTR,"ERROR: unrecognized operation %s",FUNCNAME);
		goto errorExit;
	}

	margs = op->nargs;
	iop = op->iop;
	if (iop == IGRADE)
	{
		rankOp = GRADEONLY;
	}
	else if (iop == IRANKITS || iop == IHALFNORM)
	{
		rankOp = TIESIGNORE;
	}
	else
	{
		rankOp = TIESMEAN;
	}
	charLegal = !(iop == IRANKITS || iop == IHALFNORM);
	missingOp = (iop == IANYMISSING || iop == IISMISSING);
	
	if (margs > 0 && nargs != margs || margs < 0 && nargs > -margs)
	{
		badNargs(FUNCNAME,margs);
		goto errorExit;
	}
	
	arg1 = COMPVALUE(list, 0);
	if (!argOK(arg1, NULLSYM ,0))
	{
		goto errorExit;
	}
	type = TYPE(arg1);
	
	if (type == STRUC)
	{
		type1 = getStrucTypes(arg1);

		if (type1 & ~REALTYPE1)
		{
			if ((missingOp && !(type1 & ~(LOGICTYPE1 | CHARTYPE1))) ||
				!missingOp && charLegal && (type1 & ~CHARTYPE1))
			{
				sprintf(OUTSTR,
						"ERROR: argument to %s has component that is not REAL%s",
						FUNCNAME,
						(missingOp) ?
						" LOGICAL or CHARACTER" :
						((charLegal) ? " or CHACACTER" : ""));
				goto errorExit;
			}
			type1 = (type1 & CHARTYPE1) ? CHAR : LOGIC;
		} /*if (type1 & ~REALTYPE1)*/
		else
		{
			type1 = REAL;
		}
		
	} /*if (type == STRUC)*/
	else
	{
		if (!missingOp && type != REAL && (!charLegal || type != CHAR))
		{
			badType(FUNCNAME, (!charLegal) ? REAL : -type, 0);
			goto errorExit;
		}
		else if (missingOp && type != REAL && type != LOGIC && type != CHAR &&
				 type != NULLSYM)
		{
			badType(FUNCNAME,-type,0);
			goto errorExit;
		}
		type1 = type;
	} /*if (type == STRUC){}else{}*/
	
	if (type1 == CHAR && iop == IRANK)
	{
		rankOp = TIESIGNORE;
	}
	
	for (jarg = 1;jarg < nargs;jarg++)
	{ /* only here for sort(), rank(), and grade(), rankits(), halfnorm() */
		symhkey = COMPVALUE(list,jarg);
		if (!argOK(symhkey,0,jarg+1))
		{
			goto errorExit;
		}

		if ((keyword = isKeyword(symhkey)))
		{
			if ((iop == IRANK || iop == IRANKITS || iop == IHALFNORM) &&
				strcmp(keyword,"ties") == 0)
			{
				if (type1 == CHAR)
				{
					sprintf(OUTSTR,
							"WARNING: keyword %s ignored on %s() with CHARACTER data",
							keyword, FUNCNAME);
					putErrorOUTSTR();
				} /*if (type1 == CHAR)*/
				else
				{
					rankOp = -1;
					if (isCharOrString(symhkey))
					{
						c = STRINGVALUE(symhkey,0);
						if (c == 'i' || c == 'I')
						{		/* "ignore" */
							rankOp = TIESIGNORE;
						}
						else if (c == 'a' || c == 'A')
						{		/* "average" */
							rankOp = TIESMEAN;
						}
						else if (c == 'm' || c == 'M')
						{		/* "minimum" */
							rankOp = TIESMIN;
						}
					}
					if (rankOp < 0)
					{
						sprintf(OUTSTR,
								"ERROR: Value for '%s' must be \"ignore\", \"average\", or \"minimum\"",
								keyword);
						goto errorExit;
					}
				} /*if (type1 == CHAR && iop != IRANK){}else{}*/ 
			} /*if (strcmp(keyword,"ties") == 0)*/
			else if (iop != IRANKITS && iop != IHALFNORM &&
					 strcmp(keyword,"down") == 0 || strcmp(keyword,"up") == 0)
			{
				if (!isTorF(symhkey))
				{
					notTorF(keyword);
					goto errorExit;
				}
				up = (DATAVALUE(symhkey,0) != 0.0);
				if (strcmp(keyword,"down") == 0)
				{
					up = !up;
				}
			}
			else
			{
				badKeyword(FUNCNAME,keyword);
				goto errorExit;
			}
		} /*if ((keyword = isKeyword(symhkey)))*/
		else if (iop == IRANKITS || iop == IHALFNORM)
		{
			sprintf(OUTSTR,
					"ERROR: 2nd argument to %s must be 'ties:method'",
					FUNCNAME);
			goto errorExit;
		}
		else if (jarg == nargs-1)
		{
			if (!isTorF(symhkey))
			{
				char       outstr[50];
				
				sprintf(outstr, "non-keyword final argument to %s", FUNCNAME);
				notTorF(outstr);
				goto errorExit;
			}
			up = (DATAVALUE(symhkey,0) == 0.0);
		}
		else
		{
			sprintf(OUTSTR,
					"ERROR: 2nd argument out of 3 not a keyword for %s",
					FUNCNAME);
			goto errorExit;
		}
	} /*for (jarg = 1;jarg < nargs;jarg++)*/
	
	control = iop | rankOp;

	if (up)
	{
		control |= UP;
	}
	if (type1 != CHAR && isscratch(NAME(arg1)) &&
		(iop == ISORT || IISMISSING))
	{ /* only cases when we can reuse argument as result */
		control |= REUSELEFT;
		COMPVALUE(list, 0) = (Symbolhandle) 0;
	}
	
	if (iop != IISMISSING || type == STRUC)
	{
		result = doRecur1(doSortOps, arg1, (double *) 0,control, &status);
	} /*if (iop != IISMISSING || type == STRUC)*/
	else
	{ /* ismissing(x), where x is not a structure */
		long         i;
		long         size;
		double      *y;
		
		if (control & REUSELEFT)
		{
			result = arg1;
		}
		else if (type == NULLSYM)
		{
			result = Install(NULLSCRATCH, NULLSYM);
		}
		else
		{
			size = symbolSize(arg1);
			result = RInstall(SCRATCH, size);
		}
		
		if (result == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		if (type != NULLSYM)
		{
			setTYPE(result, LOGIC);
			clearLabels(result);
			if (!(control & REUSELEFT))
			{
				ndims = NDIMS(arg1);
				setNDIMS(result, ndims);
				for (i = 1; i <= ndims; i++)
				{
					setDIM(result, i, DIMVAL(arg1,i));
				}
			} /*if (!(control & REUSELEFT))*/
		
			y = DATAPTR(result);

			if (type == CHAR)
			{
				char     *place = STRINGPTR(arg1), *newplace;
			
				for (i = 0;i < size; i++)
				{
					newplace = skipStrings(place, 1);
					y[i] = (newplace - place > 1) ? 0.0 : 1.0;
					place = newplace;
				}
			} /*if (type == CHAR)*/
			else
			{
				double   *x = DATAPTR(arg1);

				for (i = 0; i < size; i++)
				{
					y[i] = (isMissing(x[i])) ? 1.0 : 0.0;
				}
			} /*if (type == CHAR){}else{}*/
		} /*if (type != NULLSYM)*/		
	} /*if (iop != IISMISSING || type == STRUC){}else{}*/
	
	if (status & FOUNDMISSING)
	{
		sprintf(OUTSTR,
				"WARNING:  MISSING values in argument to %s", FUNCNAME);
		putErrorOUTSTR();
	}

	return (result);

  errorExit:
	putErrorOUTSTR();

	return (0);
} /*sort()*/
