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
#pragma segment Lang
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


/*
   The code in this file implements many of the elements of MacAnova as a
   language, specifically the following:
	 matrix multiplication operators (%*%, %c%, %C%)
     binary arithmetic operators (+, 1, *, ., ^ or **, and %%)
	 binary comparison operators (==, !=, <, <=, >, >=)
	 binary logical operators (&&, ||),
	 binary bit combination operators (%&, %|, %^)
	 unary arithmetic operators - and +
	 unary logical operator !
	 unary bit operation (%!)
	 extraction from arrays using subscripts (assignment is in Subassig.c)

   Some functions having to do with subscripts are also used in Subassig.c
   when assigning to elements of an array.
*/

#include "globals.h"
#include "mainpars.h"
#include "blas.h"

#define MAX(A,B) (((A) > (B) ) ? (A) : (B))
#define illegalBitValue(X) ((X) < 0 || (X) != floor(X) || (X) > MAXBITVALUE)

/* 980313 removed some unnecessary externs */

/* Prototypes for functions in this file */

static Symbolhandle doMatMult(Symbolhandle, Symbolhandle, unsigned long,
							  unsigned long *);
static Symbolhandle doBinary(Symbolhandle, Symbolhandle, unsigned long, 
							 unsigned long *);
static Symbolhandle doUnary(Symbolhandle, unsigned long, unsigned long *);

Symbolhandle        Unary(Symbolhandle, int);
Symbolhandle        Arith(Symbolhandle, Symbolhandle, int);
Symbolhandle        Logic(Symbolhandle, Symbolhandle, int);

static Symbolhandle elementMatrix(Symbolhandle, Symbolhandle);
static char       **extractIt(Symbolhandle, Symbolhandle, long [], char ***,
							  int *);

Symbolhandle        Element(Symbolhandle, Symbolhandle);
long                buildSubscript(Symbolhandle, long, 	long, long *, int *);
long                subscriptLength(Symbolhandle, long);

/*
   950911   simplified check of binary operations in case neither argument
            is a structure
*/

static long equalDims(Symbolhandle arg1, Symbolhandle arg2)
{
	long         i, ndims1 = NDIMS(arg1), ndims2 = NDIMS(arg2);
	long         ok = (ndims1 == ndims2);
	
	for (i = 1; ok && i <= ndims1 ;i++)
	{
		ok = DIMVAL(arg1,i) == DIMVAL(arg2,i);
	}
	return (ok);
} /*equalDims()*/

/*
  Check operands of binary operatons for type and compatibility.  They
  are already known to be non-null
*/

static long checkStruc(Symbolhandle arg, char * whichSide, int op,
				 short arithOp, short logOp, short compareOp, short bitOp,
				 short matmult)
{
	long            structype = getSingleType(arg);

	if(structype == 0)
	{
		sprintf(OUTSTR,
				"ERROR: structure components of %s operand to %s are not all of same type",
				whichSide, opName(op));
		putErrorOUTSTR();
		goto errorExit;
	}

	if(((arithOp || bitOp) && structype != REAL &&
		!matmult && structype != LOGIC) ||
	   (logOp && structype != LOGIC) ||
	   (compareOp && structype != REAL && structype != CHAR &&
		structype != LOGIC))
	{
		sprintf(OUTSTR,
				"ERROR: structure components of %s operand to %s are",
				whichSide, opName(op));
		putErrorOUTSTR();

		if(logOp || arithOp && matmult)
		{/* only one type allowed */
			sprintf(OUTSTR, "not all %ss",(logOp) ? "LOGICAL" : "REAL");
		}
		else if(arithOp || bitOp)
		{/* REAL and LOGICAL allowed */
			sprintf(OUTSTR, "not all REAL or LOGICAL");
		}
		else
		{/* compareOP */
				sprintf(OUTSTR, "not all REAL, CHARACTER, or LOGICAL");
		}
		goto errorExit;
	}
	return (structype);
	
  errorExit:
	return (0);
} /*checkStruc()*/

/*
  980806 Modified handling of error messages on dimension mismatch in
         matrix multiplication to reflect that dimcmp() returns
         error message in OUTSTR instead of printing it
*/
static unsigned long checkOperands(Symbolhandle arg1, Symbolhandle arg2,
								   int op)
{
	long            type1 = TYPE(arg1), type2 = TYPE(arg2);
	long            structype1 = type1, structype2 = type2;
	char           *symptom = (char *) 0, *opType;
	unsigned long   control = (unsigned long) op;
	long            reply;
	short           matmult = 0, logOp = 0, arithOp;
	short           compareOp, bitOp = 0;
	short           nStruc =  (type1 == STRUC) + (type2 == STRUC);
	WHERE("checkOperands");
	
	OUTSTR[0] = '\0';
	compareOp = (op == EQ || op == NE || op == LT || op == LE ||
				 op == GT || op == GE);
	if (!compareOp)
	{
		logOp = (op == AND || op == OR);
		if (!logOp)
		{
			matmult = (op == MATMULT || op == TRMATMULT || op == MATMULTTR);
			if (!matmult)
			{
				bitOp = (op == BITAND || op == BITOR || op == BITXOR);
			}
		} /*if (!logOp)*/
	} /*if (!compareOp)*/
	arithOp = !(logOp || compareOp || bitOp);

	if(nStruc < 2)
	{ /* at least one non-STRUC*/
		if(type1 == UNDEF || type1 == ASSIGNED ||
		   type2 == UNDEF || type2 == ASSIGNED)
		{
			symptom = "undefined";
		}
		else if (type1 == NULLSYM || type2 == NULLSYM)
		{
			symptom = "null";
		}
		else if(matmult)
		{
			if(type1 != REAL && type1 != STRUC ||
			   type2 != REAL && type2 != STRUC)
			{
				symptom = "non-numeric";
			}
		}
		else if(logOp)
		{
			if(type1 != LOGIC && type1 != STRUC ||
			   type2 != LOGIC && type2 != STRUC)
			{
				symptom = "non-logical";
			}
		}
		else if(arithOp || bitOp)
		{
			if((type1 != REAL && type1 != LOGIC && type1 != STRUC) || 
			   (type2 != REAL && type2 != LOGIC && type2 != STRUC))
			{
				symptom = "non-numeric and non-logical";
			}
		}
		else
		{/*comparOp*/
			if(type1 != REAL && type1 != CHAR && type1 != LOGIC &&
			   type1 != STRUC ||
			   type2 != REAL && type2 != CHAR && type2 != LOGIC &&
			   type2 != STRUC)
			{
				symptom = "non REAL, CHARACTER, or LOGICAL";
			}
		}

		if (symptom)
		{
			if(matmult)
			{
				opType = "matrix multiplication";
			}
			else if(arithOp)
			{
				opType = "arithmetic";
			}
			else if(bitOp)
			{
				opType = "bit operation";
			}
			else if(logOp)
			{
				opType = "logical operation";
			}
			else
			{
				opType = "comparison";
			}

			sprintf(OUTSTR,"ERROR: %s with %s operand",opType,symptom);
			putOUTSTR();
			sprintf(OUTSTR,"%s %s %s",
					typeName(type1),opName(op),typeName(type2));
			goto yyerrorMsg;
		} /*if(symptom)*/
	} /*if(nStruc < 2)*/

	if (nStruc != 0)
	{
		if(type1 == STRUC)
		{
			structype1 = checkStruc(arg1, "left", op, 
									arithOp, logOp, compareOp, bitOp, matmult);
			if (structype1 == 0)
			{
				goto yyerrorMsg;
			}
			control |= LEFTRECUR;
		} /*if(type1 == STRUC)*/

		if(type2 == STRUC)
		{
			structype2 = checkStruc(arg2, "right", op,
									arithOp, logOp, compareOp, bitOp, matmult);
			if (structype2 == 0)
			{
				goto yyerrorMsg;
			}
			control |= RIGHTRECUR;
		} /*if(type2 == STRUC)*/
		if(type1 == STRUC)
		{
			type1 = structype1;
		}
		if(type2 == STRUC)
		{
			type2 = structype2;
		}
	} /*if (nStruc != 0)*/
	
	if(compareOp && type1 != type2 && (type1 == CHAR || type2 == CHAR))
	{
		putOutErrorMsg("ERROR: CHARACTER value compared with non-CHARACTER value");
		sprintf(OUTSTR,"%s %s %s",
				typeName(type1),opName(op),typeName(type2));
		goto yyerrorMsg;
	}
	
	/*
	   check dimensions and tree structure for compatibility
	   If neither argument is a structure and one is a scalar, no
	   further checking is needed except for matrix multiplication
	*/

	
	if (nStruc > 0)
	{
		reply = treecmp(arg1,arg2,control);
	}
	else if (matmult || !isScalar(arg1) && !isScalar(arg2))
	{
		reply = dimcmp(arg1, arg2, control);
	}
	else
	{
		reply = 0;
	}
	
	if (reply == 0)
	{
		return (control);
	}
	
	if(reply & BADDIMS)
	{
		char  *somewhere = (nStruc) ? " somewhere in structure" : "";

		if (!matmult)
		{
			sprintf(OUTSTR,
					"ERROR: Dimension mismatch%s for %s",
					somewhere, opName(op));
		}
		else
		{
			strcat(OUTSTR, somewhere);
		}
	}
	else if(reply & BADSTRUC)
	{
		sprintf(OUTSTR,
				"ERROR: structure shapes do not match for %s", opName(op));
	}
	else if(reply & HASMISSING)
	{
		sprintf(OUTSTR,"ERROR: MISSING values not allowed for %s",opName(op));
	}
	/* fall through*/

  yyerrorMsg:
	yyerror(OUTSTR);

	return (0);
	
} /*checkOperands()*/

/*
   961127 Added check to make sure interrupted() is called about every
   NPRODUCTS products
   980313 add code to set or clear NOMISSING bit in result
   980813 when only 1 is labelled, the result is labelled, using
          rep("A",m) for the missing labels.
*/

static Symbolhandle doMatMult(Symbolhandle arg1, Symbolhandle arg2,
							  unsigned long control, unsigned long *status)
{
	long            i, j, ij;
	long            dim[2][2];
	long            ikincr, kjincr, ik, kj, incri, incrj;
	long            ikstart, kjstart;
	long            limi, limj, limk, limij[2];
	long            ncomps;
	long            leftrecur = isStruc(arg1);
	long            rightrecur = isStruc(arg2);
	int             op = control & OPMASK;
	int             missingSet = 0;
	Symbolhandle    left, right, symh;
	Symbolhandle    result = (Symbolhandle) 0;
	double          y;
	double         *a1, *a2, *a3;
	WHERE("doMatMult");

	clearNproducts();

/*  note, we already know arg1 and arg2 are either both non-null
	and compatible or both null */	

	if(leftrecur || rightrecur)
	{
		ncomps = (leftrecur) ? NCOMPS(arg1) : NCOMPS(arg2);
		result = StrucInstall(SCRATCH,ncomps);
		if(result == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		
		for(i=0;i<ncomps;i++)
		{
			left = (leftrecur) ? COMPVALUE(arg1,i) : arg1;
			right = (rightrecur) ? COMPVALUE(arg2,i) : arg2;
			symh = doMatMult(left,right,control,status);
			if(symh == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			Cutsymbol(symh);
			setCompName(symh,(leftrecur) ? NAME(left) : NAME(right));
			COMPVALUE(result,i) = symh;
		}
		return (result);
	} /*if(leftrecur || rightrecur)*/
	else if(!isNull(arg1))
	{
		(void) isMatrix(arg1,dim[0]);
		(void) isMatrix(arg2,dim[1]);

		if(op == MATMULT)
		{
			limi = dim[0][0];
			limj = dim[1][1];
			ikincr = dim[0][0];
			kjincr = 1;
			incri = 1;
			incrj = dim[1][0];
			limk = dim[0][1];
		}
		else if(op == TRMATMULT)
		{
			limi = dim[0][1];
			limj = dim[1][1];
			ikincr = 1;
			kjincr = 1;
			incri = dim[0][0];
			incrj = dim[1][0];
			limk = dim[0][0];
		}
		else					/* MATMULTTR */
		{
			limi = dim[0][0];
			limj = dim[1][0];
			ikincr = dim[0][0];
			kjincr = dim[1][0];
			incri = 1;
			incrj = 1;
			limk = dim[0][1];
			if(limk != dim[1][1])
			{
 /* this should never happen now, since dimensions are pre-checked*/
				limk = -1;
			}
		}	
	} /*if(leftrecur || rightrecur){}else if(!isNull(arg1)){}*/
	else
	{
		limi = limj = 0;
	}
	
	result = (limi != 0) ?
		RInstall(SCRATCH,limi * limj) : Install(NULLSCRATCH, NULLSYM);
	
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	if(limi > 0)
	{
		limij[0] = limi;
		limij[1] = limj;
		
		Setdims(result, 2, limij);
	
		if (HASLABELS(arg1)  || HASLABELS(arg2))
		{
			char           *rowcollabs1[2], *rowcollabs2[2];
			long            lablengths1[2], lablengths2[2];
			long            newLablengths[2];
			long            k1, k2;

			k1 = (op == MATMULT || op == MATMULTTR) ? 0 : 1;
			k2 = (op == MATMULT || op == TRMATMULT) ? 1 : 0;
			if (HASLABELS(arg1))
			{
				getMatLabels(arg1, rowcollabs1, lablengths1);
			}
			if (HASLABELS(arg2))
			{
				getMatLabels(arg2, rowcollabs2, lablengths2);
			}

			newLablengths[0] = (HASLABELS(arg1)) ? lablengths1[k1] :2*limi;
			newLablengths[1] = (HASLABELS(arg2)) ? lablengths2[k2] :2*limj;
			TMPHANDLE = createLabels(2, newLablengths);
			
			if (TMPHANDLE != (char **) 0 && setLabels(result, TMPHANDLE))
			{
				char      numericLabel[2];
				
				numericLabel[0] = NUMERICLABEL;
				numericLabel[1] = '\0';

				if (HASLABELS(arg1))
				{
					getMatLabels(arg1, rowcollabs1, lablengths1);
					appendLabels(result, rowcollabs1[k1], 0, dontExpand);
				}
				else
				{
					appendLabels(result, numericLabel, 0, doExpand);
				}
				if (HASLABELS(arg2))
				{
					getMatLabels(arg2, rowcollabs2, lablengths2);
					appendLabels(result, rowcollabs2[k2], 1, dontExpand);
				}
				else
				{
					appendLabels(result, numericLabel, 1, doExpand);
				}
			} /*if (TMPHANDLE != (char **) 0 && setLabels(result, TMPHANDLE))*/
			else
			{
				mydisphandle(TMPHANDLE);
			}
		} /*if (HASLABELS(arg1)  || HASLABELS(arg2))*/
		
			
		a1 = DATAPTR(arg1);
		a2 = DATAPTR(arg2);
		a3 = DATAPTR(result);

		ikstart = 0;
		for (i = 0; i < limi; i++)
		{
			kjstart = 0;
			ij = i;
			for (j = 0; j < limj; j++)
			{

				ik = ikstart;
				kj = kjstart;
				kjstart += incrj;
			
/* blas ddot should save a little on %c% because kjincr and ikincr are both 1*/

				y = DDOT(&limk,a1+ik,&ikincr,a2+kj,&kjincr);
				incNproducts(limk);
		/* every so often check for interrupts */
				testNproducts(errorExit);

#ifdef HASINFINITY
				if (isInfinite(y))
				{
					setMissing(y);
					missingSet = 1;
					*status |= FOUNDOVERFLOW;
				}
#endif /*HASINFINITY*/
				a3[ij] = y;
				ij += limi;
			} /*for (j = 0; j < limj; j++)*/
			ikstart += incri;
		} /*for (i = 0; i < limi; i++)*/
	} /*if(limi > 0)*/
	
#if (USENOMISSING)
	if (missingSet)
	{
		clearNOMISSING(result);
	}
	else
	{
		setNOMISSING(result);
	}
#endif /*USENOMISSING*/
	return (result);

  errorExit:
	Removesymbol(result);
	
	return (0);
} /*doMatMult()*/

/*
    950922  dereferenced destination and source handles to pointers
    980313 add code to set or clear NOMISSING bit in result
*/
static Symbolhandle doUnary(Symbolhandle arg,
							unsigned long control, unsigned long *status)
{
	long            i, ndims, tot1;
	double          x;
	double         *dest, *source;
	long            ncomps;
	long            recur = isStruc(arg);
	int             op = control & OPMASK;
	int             missingSet = 0;
	Symbolhandle    left, symh;
	Symbolhandle    result = (Symbolhandle) 0;
	WHERE("doUnary");

/*  note, we already know arg non-null */	
	if(recur)
	{
		ncomps = NCOMPS(arg);
		if(control & REUSELEFT)
		{
			result = arg;
		}
		else
		{
			result = StrucInstall(SCRATCH,ncomps);
			if(result == (Symbolhandle) 0 || !transferLabels(arg, result))
			{
				goto errorExit;
			}
		}		

		for (i=0;i<ncomps;i++)
		{
			left = (recur) ? COMPVALUE(arg,i) : arg;
			if(!isNull(left))
			{
				symh = doUnary(left,control,status);
			}
			else if(left == (Symbolhandle) 0)
			{
				symh = RInstall(SCRATCH,0);
			}
			else
			{
				symh = left;
			}
			
			if(symh == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			Cutsymbol(symh);
			if(left != (Symbolhandle) 0)
			{
				setCompName(symh,NAME(left));
			}
			else
			{
				setCompName(symh,"NULL");
			}
			COMPVALUE(result,i) = symh;
		} /*for (i=0;i<ncomps;i++)*/
		return (result);
	} /*if(recur)*/

	/* Not a structure */
	/* Find total number of data values */
	tot1 = symbolSize(arg);
	ndims = NDIMS(arg);

	/* set up result structure */
	if(control & REUSELEFT)
	{ /* reuse argument if arg is scratch or unary plus */
		 result = arg;
	}
	else
	{
		result = RInstall(SCRATCH,tot1);
		Setdims(result, ndims, &DIMVAL(arg, 1));
		if (result == (Symbolhandle) 0 || !transferLabels(arg, result))
		{
			goto errorExit;
		}
	}
	if(op == NOT)
	{
		setTYPE(result,LOGIC);
	}
	else
	{
		setTYPE(result,REAL);
	}

	source = DATAPTR(arg);
	dest = DATAPTR(result);
	for (i = 0; i < tot1; i++)
	{
		x = source[i];
		if(isMissing(x))
		{
			*status |= FOUNDMISSING;
			missingSet = 1;
		}
		else if(op == NOT)
		{
			x = (x == 0.0) ? 1.0 : 0.0;
		}
		else if(op == '-')
		{ /* unary minus */
			x = -x;
		}
#ifdef BITOPS
		else if(op == BITNOT)
		{ /* bit reversal */
			if(illegalBitValue(x))
			{
				*status |= FOUNDDOMERR;
				setMissing(x);
				missingSet = 1;
			}
			else
			{
				x = (double) ~ (unsigned long) x;
			}
		}
#endif /*BITOPS*/
		/* do nothing if unary plus or MISSING */
		dest[i] = x;
	} /*for (i = 0; i < tot1; i++)*/

#if (USENOMISSING)
	if (missingSet)
	{
		clearNOMISSING(result);
	}
	else
	{
		setNOMISSING(result);
	}
#endif /*USENOMISSING*/

	return (result);

  errorExit:
	Removesymbol(result);
	
	return (0);
	
} /*doUnary()*/

#define Ndims1 dim1[0]
#define Ndims2 dim2[0]
/*
	970225 fixed bug on 0^x
	970602 fixed bug in comparison of CHARACTER arrays
    980313 add code to set or clear NOMISSING bit in result
    980807 fixed minor bug in transferring labels.
	981216 fixed bug in setting dimension in an expression like
           d OP d[,j] or d[,j] OP d, when d has labels.
	990322 fixed bug in labelling operations involving a scalar.  Now
	       (i) when both are scalars, the labels and dimensions come from
	       the operand with more dimensions
	       (ii) when only one is a scalar, the labels and dimensions come from
	       the other operand.
*/

static Symbolhandle doBinary(Symbolhandle arg1, Symbolhandle arg2,
							 unsigned long control, unsigned long *status)
{
	Symbolhandle    result = (Symbolhandle) 0;
	Symbolhandle    left, right, symh;
	long            icomp, ncomps;
	long            leftrecur = isStruc(arg1);
	long            rightrecur = isStruc(arg2);
	int             missingSet = 0;
	WHERE("doBinary");	

/*
   Note: we already know arg1 and arg2 are either both non-null and
   compatible or both null
*/	
	if (leftrecur || rightrecur)
	{ /* arg1 or arg2 (or both) is structure*/
		ncomps = (leftrecur) ? NCOMPS(arg1) : NCOMPS(arg2);
		if(control & REUSELEFT)
		{
			result = arg1;
		}
		else if(control & REUSERIGHT)
		{
			result = arg2;
		}
		else
		{
			result = StrucInstall(SCRATCH,ncomps);
			if (result == (Symbolhandle) 0 ||
				!transferLabels((leftrecur) ? arg1 : arg2, result))
			{
				goto errorExit;
			}
		}
		
		for (icomp=0;icomp<ncomps;icomp++)
		{
			left = (leftrecur) ? COMPVALUE(arg1,icomp) : arg1;
			right = (rightrecur) ? COMPVALUE(arg2,icomp) : arg2;
			symh = doBinary(left,right,control,status);
			if(symh == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			if(!(control & (REUSELEFT | REUSERIGHT)))
			{
				Cutsymbol(symh);
			}
			
			setCompName(symh,(leftrecur) ? NAME(left) : NAME(right));
			
			COMPVALUE(result,icomp) = symh;
		} /*for (icomp=0;icomp<ncomps;icomp++)*/

		return (result);
	} /*if (leftrecur || rightrecur)*/

	/*
	   Neither argument is structure
	   Note: if either argument is Null, both must be
	*/
	/* make up output structure */

	if(control & REUSELEFT)
	{
		result = arg1;
	}
	else if(control & REUSERIGHT)
	{
		result = arg2;
	}
	else
	{
		result = (isNull(arg1)) ?
			Install(NULLSCRATCH,NULLSYM) : RInstall(SCRATCH,0);
		if (result == (Symbolhandle) 0)
		{
			goto errorExit;
		}
	}

	if (!isNull(arg1))
	{
		long        maxtot, middle1, middle2, middle;
		long        dim1[MAXDIMS+1], dim2[MAXDIMS+1], dim[MAXDIMS+1];
		long        ndims, klim;
		long        i, j, k, jk1,jk2, ijk1,ijk2, ijk;
		long        incijk1, incijk2, incjk1, incjk2, inck1, inck2;
		long        scalar1 = isScalar(arg1);
		long        typeChar = (TYPE(arg1) == CHAR  || TYPE(arg1) == MACRO);
		double      yijk, xijk1 , xijk2;
		char       *sijk1, *sijk2, *sjk1, *sjk2;
		char       *s1, *s2;
		double     *x1, *x2, *dest;
		long        nrows;
		int         op, dimsSet;
		short       miss1, miss2, missarg = 0;
		short       arithOp, logOp, compareOp;
		short       addOp, subOp, multOp, divOp;
		short       modOp, expOp;
		short       eqOp, neOp, gtOp, geOp, ltOp, leOp;
		short       andOp, orOp;
#ifdef HUGE_VAL
		double      hugeval = HUGE_VAL;
#endif /*HUGE_VAL*/
#ifdef BITOPS
		short       bitandOp, bitxorOp, bitorOp;
#endif /*BITOPS*/

	/* Copy dimensions into scratch & compute total size */
		getDims(dim1, arg1);
		getDims(dim2, arg2);
		
		if (HASLABELS(arg1) || HASLABELS(arg2))
		{
			int      ok = 1;

			dimsSet = 1;

			if (equalDims(arg1,arg2))
			{
				Setdims(result, Ndims2, dim2 + 1);
				ok = transferLabels((HASLABELS(arg1)) ? arg1 : arg2, result);
			}
			else if (isScalar(arg1) && isScalar(arg2))
			{
				if (Ndims2 > Ndims1)
				{
					Setdims(result, Ndims2, dim2 + 1);
					ok = transferLabels(arg2, result);
				}
				else
				{
					Setdims(result, Ndims1, dim1 + 1);
					ok = transferLabels(arg1, result);
				}
			}
			else if (isScalar(arg1))
			{ /*scalar op arg2*/
				Setdims(result, Ndims2, dim2 + 1);
				ok = transferLabels(arg2, result);
			}
			else if (isScalar(arg2))
			{ /*arg1 op scalar*/
				Setdims(result, Ndims1, dim1 + 1);
				ok = transferLabels(arg1, result);
			}
			else if (Ndims1 <= 2 && Ndims2 <= 2)
			{
				if (dim1[1] == dim2[1])
				{ /*nrows1 == nrows2*/
					if ((Ndims1 == 1 || dim1[2] == 1) && Ndims2 == 2)
					{ /* vector op matrix */
						Setdims(result, Ndims2, dim2 + 1);
						ok = transferLabels(arg2, result);
					}
					else if (Ndims1 == 2 && (Ndims2 == 1 || dim2[2] == 1))
					{ /* matrix op vector */
						Setdims(result, Ndims1, dim1 + 1);
						ok = transferLabels(arg1, result);
					}
				}
				else if (Ndims1 == 2 && Ndims2 == 2 && dim1[2] == dim2[2])
				{ /*ncols1 == ncols2*/
					if (dim1[1] == 1)
					{ /*rowVector op matrix */
						Setdims(result, Ndims2, dim2 + 1);
						ok = transferLabels(arg2, result);
					}
					else if (dim2[1] == 1)
					{ /*matrix op rowVector */
						Setdims(result, Ndims1, dim1 + 1);
						ok = transferLabels(arg1, result);
					}
				}
				else
				{
					dimsSet = 0;
				}
			}
			else
			{
				clearLABELS(result);
			}
			if (!ok)
			{
				goto errorExit;
			}
		} /*if (HASLABELS(arg1) || HASLABELS(arg2))*/
		else
		{
			dimsSet = 0;
		}
		
		ndims = MAX(Ndims1,Ndims2);

		/* fill out short dimension with 1's */
		for (i = Ndims1 + 1; i <= ndims;i++)
		{
			dim1[i] = 1;
		}
	
		for (i = Ndims2 + 1;i <= ndims;i++)
		{
			dim2[i] = 1;
		}
	
		Ndims1 = Ndims2 = ndims;
	
		middle1 = middle2 = 1;	/* product of dimensions excluding 1st and last */

		/* note: dimensions are known to match */
		for (i = 2;i < ndims;i++)
		{/* middle dimensions must match exactly unless at least 1 arg scalar*/
			dim[i] = (!scalar1) ? dim1[i] : dim2[i];
			middle1 *= dim1[i];
			middle2 *= dim2[i];
		}

		nrows = dim[1] = MAX(dim1[1],dim2[1]);
		if(ndims > 1)
		{
			dim[ndims] = MAX(dim1[ndims],dim2[ndims]);
			middle = MAX(middle1,middle2);
			klim = dim[ndims];
			maxtot = nrows * middle * dim[ndims];
		}
		else
		{
			klim = middle = 1;
			maxtot = nrows;
		}

		/* Finish building result*/
		if (DATA(result) == (double **) 0)
		{
			TMPHANDLE = mygethandle(maxtot*sizeof(double));
			setDATA(result, (double **) TMPHANDLE);
			if (TMPHANDLE == (char **) 0)
			{
				goto errorExit;
			}
		} /*if (DATA(result) == (double **) 0)*/		

		/* ensure result is not a factor */
		
		setNCLASS(result, -1);
		if (!dimsSet)
		{
			Setdims(result, ndims, dim + 1);
		}
		
		incijk1 = (dim1[1] == 1) ? 0 : 1;
		incijk2 = (dim2[1] == 1) ? 0 : 1;

		inck1 = (dim1[ndims] == 1) ? 0 : dim1[1]*middle1;
		inck2 = (dim2[ndims] == 1) ? 0 : dim2[1]*middle2;

		incjk1 = (middle1 == 1) ? 0 : dim1[1];
		incjk2 = (middle2 == 1) ? 0 : dim2[1];

		op = control & OPMASK;
		andOp = orOp = eqOp = neOp = gtOp = geOp = ltOp = leOp = 0;
		addOp = subOp = multOp = divOp = modOp = expOp = 0;
#ifdef BITOPS
		bitandOp = bitxorOp = bitorOp = 0;
#endif /*BITOPS*/
		arithOp = ((addOp = (op == ADD)) ||
				   (subOp = (op == SUB)) ||
				   (multOp = (op == MULT)) ||
				   (divOp = (op == DIV)) ||
				   (modOp = (op == MOD)) ||
				   (expOp = (op == EXP)));

		compareOp = !arithOp &&
			((eqOp = (op == EQ)) ||
			 (neOp = (op == NE)) ||
			 (gtOp = (op == GT)) ||
			 (geOp = (op == GE)) ||
			 (ltOp = (op == LT)) ||
			 (leOp = (op == LE)));

		logOp = !arithOp && !compareOp &&
			((andOp = (op == AND)) ||
			 (orOp = (op == OR)));
		
#ifdef BITOPS
		!arithOp && !compareOp && !logOp &&
			((bitandOp = (op == BITAND)) ||
			 (bitxorOp = (op == BITXOR)) ||
			 (bitorOp = (op == BITOR)));
#endif /*BITOPS*/
	
		setTYPE(result, (logOp || compareOp) ? LOGIC : REAL);
		dest = DATAPTR(result);

		if(typeChar)
		{
			s1 = STRINGPTR(arg1);
			s2 = STRINGPTR(arg2);
		}
		else
		{
			x1 = DATAPTR(arg1);
			x2 = DATAPTR(arg2);
		}
		
		ijk = 0;
		for (k=0;k<klim;k++)
		{
		
			jk1 = k*inck1;
			jk2 = k*inck2;
			if(typeChar)
			{
				sjk1 = skipStrings(s1,jk1);
				sjk2 = skipStrings(s2,jk2);
			}
		
			for (j=0;j<middle;j++)
			{
				if(typeChar)
				{
					sijk1 = sjk1;
					sijk2 = sjk2;
				}
				else
				{
					ijk1 = jk1;
					ijk2 = jk2;
				}
				for (i=0;i<nrows;i++)
				{
					if(!typeChar)
					{
						xijk1 = x1[ijk1];
						xijk2 = x2[ijk2];
						miss1 = isMissing(xijk1);
						miss2 = isMissing(xijk2);
						missarg = miss1 || miss2;
						if (missarg)
						{
							if(!eqOp && !neOp)
							{
								setMissing(yijk);
								*status |= FOUNDMISSING;
								missingSet = 1;
							}
							else
							{ /* on == and != no warning, check sameness */
								if(miss1 && miss2)
								{ /* both missing */
									yijk = (eqOp) ? 1.0 : 0.0;
								}
								else
								{ /* just one missing */
									yijk = (neOp) ? 1.0 : 0.0;
								}
							}
						} /*if (isMissing(xijk1) || isMissing(xijk2))*/
						else if(arithOp)
						{
							if(addOp)
							{
								yijk = xijk1 + xijk2;
							}
							else if(subOp)
							{
								yijk = xijk1 - xijk2;
							}
							else if(multOp)
							{
								yijk = xijk1 * xijk2;
							}
							else if(divOp)
							{
								if (xijk2 == 0.0)
								{
									if(xijk1 == 0.0)
									{ /* 0/0 is 0 */
										yijk = 0.0;
									}
									else
									{
										*status |= FOUNDZERODIV;
										setMissing(yijk);
										missingSet = 1;
									}
								}
								else
								{
									yijk = xijk1 / xijk2;
								}
							}
							else if(modOp)
							{
								if (xijk2 == 0.0)
								{
									*status |= FOUNDZERODIV;
									setMissing(yijk);
									missingSet = 1;
								}
								else
								{
									yijk = floor(xijk1 / xijk2);
									yijk = xijk1 - xijk2*yijk;
								}
							}
							else if(expOp)
							{
								if (xijk2 == 0.)
								{ /* x^0 is 1, including 0^0 */
									yijk = 1.0;
								}
								else if (xijk1 == 0.)
								{ /* 0^p is 0, p > 0 or MISSING, p < 0 */
									if(xijk2 > 0)
									{
										yijk = 0;
									}
									else
									{
										setMissing(yijk);
										missingSet = 1;
									}
								}
								else
#ifdef USEPOW /* if defined, system pow() is efficient on integer powers */
								{ /* x != 0, p != 0 */
									yijk = pow(fabs(xijk1), xijk2);
									if(xijk1 < 0)
									{
										if (xijk2 != floor(xijk2))
										{ /* x < 0, p not integer */
											yijk = -yijk;
											*status |= FOUNDNEG;
										}
										else if((long) xijk2 % 2 != 0)
										{ /* x < 0, odd integer power */
											yijk = -yijk;
										}
									}	
								} /* if ... else */
#else  /*USEPOW*/
								if(xijk2 == floor(xijk2))
								{/* x != 0, p an integer */
									yijk = intpow(xijk1,xijk2);
								} /*if(xijk2 == floor(xijk2))*/
								else
								{/* x != 0, p not integer */
									yijk = pow(fabs(xijk1), xijk2);
									if(xijk1 < 0)
									{
										yijk = -yijk;
										*status |= FOUNDNEG;
									}
								} /*if(xijk2 == floor(xijk2)){}else{}*/
#endif /*USEPOW*/
#ifdef HUGE_VAL
								if (!missarg && !isMissing(yijk) &&
									(yijk == hugeval || yijk == -hugeval))
								{
									*status |= FOUNDOVERFLOW;
									setMissing(yijk);
									missingSet = 1;
								}
#endif /*HUGE_VAL*/
							}
#ifdef HASINFINITY
							if (!missarg && !isMissing(yijk) && isInfinite(yijk))
							{
								*status |= FOUNDOVERFLOW;
								setMissing(yijk);
								missingSet = 1;
							}
#endif /*HASINFINITY*/
						} /* if(arithOp) */
						else if(compareOp)
						{/* comparison operations */
							double      difference = xijk1 - xijk2;
							
							if(eqOp)
							{
								yijk = (difference == 0.0) ? 1.0 : 0.0;
							}
							else if(neOp)
							{
								yijk = (difference != 0.0) ? 1.0 : 0.0;
							}
							else if(gtOp)
							{
								yijk = (difference > 0.0) ? 1.0 : 0.0;
							}
							else if(geOp)
							{
								yijk = (difference >= 0.0) ? 1.0 : 0.0;
							}
							else if(ltOp)
							{
								yijk = (difference < 0.0) ? 1.0 : 0.0;
							}
							else if(leOp)
							{
								yijk = (difference <= 0.0) ? 1.0 : 0.0;
							}
						}
						else if (logOp)
						{
							if(andOp)
							{
								yijk = ((xijk1 != 0.0) && (xijk2 != 0.0)) ?
									1.0 : 0.0;
							}
							else if(orOp)
							{
								yijk = ((xijk1 != 0.0) || (xijk2 != 0.0)) ?
									1.0 : 0.0;
							} /* if ... else ... else ... */
						}  /* if(arithOp){}else if... else if(logOp){}*/
#ifdef BITOPS
						else
						{ /* bit operations */
							if (illegalBitValue(xijk1) ||
								illegalBitValue(xijk2))
							{
								*status |= FOUNDDOMERR;
								setMissing(yijk);
								missingSet = 1;
							}
							else if(bitandOp)
							{
								yijk = (double) ((unsigned long) xijk1 &
												 (unsigned long) xijk2);
							}
							else if(bitxorOp)
							{
								yijk = (double) ((unsigned long) xijk1 ^
												 (unsigned long) xijk2);
							}
							else if(bitorOp)
							{
								yijk = (double) ((unsigned long) xijk1 |
												 (unsigned long) xijk2);
							}
						} /* if(arithOp){}else if(logOp){}else{}*/
#endif /*BITOPS*/
					} /* if(!typeChar) */
					else
					{ /* typeChar; all comparison operations */
						int      difference = strcmp(sijk1, sijk2);

						if(eqOp)
						{
							yijk = (difference == 0) ? 1.0 : 0.0;
						}
						else if(neOp)
						{
							yijk = (difference != 0) ? 1.0 : 0.0;
						}
						else if(gtOp)
						{
							yijk = (difference > 0) ? 1.0 : 0.0;
						}
						else if(geOp)
						{
							yijk = (difference >= 0) ? 1.0 : 0.0;
						}
						else if(ltOp)
						{
							yijk = (difference < 0) ? 1.0 : 0.0;
						}
						else if(leOp)
						{
							yijk = (difference <= 0) ? 1.0 : 0.0;
						}
					} /* if(!typeChar){}else{} */
					dest[ijk++] = yijk;

					if(i < nrows - 1)
					{
						if(incijk1)
						{
							if(typeChar)
							{
								sijk1 = skipStrings(sijk1,incijk1);
							}
							else
							{
								ijk1++;
							}	
						}
						if(incijk2)
						{
							if(typeChar)
							{
								sijk2 = skipStrings(sijk2,incijk2);
							}
							else
							{
								ijk2++;
							}
						} /*if(incijk2)*/
					} /*if(i < nrows - 1)*/
				} /* for (i=0;i<nrows;i++) */

				if(j < middle-1)
				{
					if(incjk1)
					{
						if(typeChar)
						{
							sjk1 = skipStrings(sjk1,incjk1);
						}
						else
						{
							jk1 += incjk1;
						}
					}
					if(incjk2)
					{
						if(typeChar)
						{
							sjk2 = skipStrings(sjk2,incjk2);
						}
						else
						{
							jk2 += incjk2;
						}
					} /*if(incjk2)*/
				} /* if(j < middle-1) */
			} /* for (j=0;j<middle;j++) */
		} /* for (k=0;k<klim;k++) */
	} /*if(maxtot > 0)*/

#if (USENOMISSING)
	if (missingSet)
	{
		clearNOMISSING(result);
	}
	else
	{
		setNOMISSING(result);
	}
#endif /*USENOMISSING*/

	return (result);

  errorExit:
	Removesymbol(result);

	return (0);
	
} /*doBinary()*/

#undef Ndims1
#undef Ndims2

Symbolhandle    Unary(Symbolhandle arg, int op)
{
	/* unary minus or plus or bitwise complement for a real data set*/
	Symbolhandle    result = (Symbolhandle) 0;
	char           *symptom = (char *) 0;
	long            type;
	unsigned long   control = (long) op, status = 0;

	OUTSTR[0] = '\0';
	
	if(isNull(arg))
	{
		symptom = "missing or null";
	}
	else if((type = TYPE(arg)) == UNDEF || type == ASSIGNED)
	{
		symptom = "undefined argument";
	}
	else if(type != REAL && type != LOGIC)
	{
		if(type != STRUC)
		{
			symptom = "non REAL and non LOGICAL argument";
		}
		else if(!isReal(arg) && !isLogic(arg))
		{
			symptom = "structure with non REAL and non LOGICAL component";
		}
		else
		{
			control |= LEFTRECUR;
		}
	}
	if(symptom)
	{
		sprintf(OUTSTR,"ERROR: %s with %s",
				(op != BITNOT) ? "arithmetic" : "bit operation", symptom);
		goto yyerrorMsg;
	} /*if(symptom)*/

	if(isscratch(NAME(arg)))
	{
		control |= REUSELEFT;
	}
	
	result = doUnary(arg, control, &status);
/*
	Note: doUnary overwrites a SCRATCH arg and returns it as result.
	Hence it should not be deleted here, even if SCRATCH.
*/
	if(status & FOUNDMISSING)
	{
		sprintf(OUTSTR,"WARNING: %s%s with MISSING value(s)",
				(op == BITNOT) ? "" : "unary ",opName(op));
		putErrorOUTSTR();
	} /*if(status & FOUNDMISSING)*/
#ifdef BITOPS
	if(status & FOUNDDOMERR)
	{
		sprintf(OUTSTR,
				"WARNING: bit operation with illegal value(s); operation is %s",
				opName(op));
		putErrorOUTSTR();
	} /*if(status & FOUNDDOMERR)*/
#endif /*BITOPS*/
	return (result);

  yyerrorMsg:
	yyerror(OUTSTR);

	return (0);
} /*Unary()*/

/*
  Function to handle matrix multiplication, elementwise binary, and
  arithmetic operations.

  It has been modified (by kb) to allow more cases than just combination
  of identical dimensioned arrays, or an array with a scalar.
  1.  the dimensions of the array with fewest dimensions is augmented with 1's
      to the number of dimensions of the other array.
  2.  If the first dimension of an array is 1, it is treated as if it had the
      same first dimension as the other array, with each row (trailing slab)
      identical
  3.  If the last dimension of an array is 1, it is treated as if it had the
      same last dimension of the other array, with each column (trailing slab)
      identical.
  4.  If all dimensions of an array are 1, it is treated as a scalar, except
      the dimensionality of the result will be at least as long as the
      dimensionality of the array.
  5.  arg1 and/or arg2 may be structures.  If they both are, then their
      structure patterns must be identical and corresponding components must
      be compatible as in 1-4.  If only one is a structure, all its components
      must be compatible with the other argument as in 1-4.
*/

Symbolhandle    Arith(Symbolhandle arg1, Symbolhandle arg2, int op)
{
	Symbolhandle    result = (Symbolhandle) 0;
	unsigned long   control, control1, status = 0;
	int             bitop = (op == BITAND || op == BITOR || op == BITXOR);
	WHERE("Arith");

	OUTSTR[0] = '\0';
	
	if (isNull(arg1) && TYPE(arg1) != NULLSYM ||
		isNull(arg2) && TYPE(arg2) != NULLSYM)
	{
		sprintf(OUTSTR,
				"ERROR: %s with null argument; operation is %s",
				(!bitop) ? "arithmetic" : "bit operation",
				opName(op));
		goto yyerrorMsg;
	}
	
	if (op == MATDIV || op == DIVMAT)
	{
		Symbolhandle       a1 = (op == MATDIV) ? arg2 : arg1;
		Symbolhandle       a2 = (op == MATDIV) ? arg1 : arg2;
		Symbolhandle       list = Growlist(Addlist(Makelist(), a1));

		if (list != (Symbolhandle) 0)
		{		
			list = Addlist(list, a2);
			if (list != (Symbolhandle) 0)
			{
				strcpy(FUNCNAME, opName(op));
				result = solve(list);
				Removelist(list);
			} /*if (list != (Symbolhandle) 0)*/			
		} /*if (list != (Symbolhandle) 0)*/
		return (result);
	} /*if (op == MATDIV || op == DIVMAT)*/
	
	control = checkOperands(arg1, arg2, op);

	if(control != 0)
	{
		if(op == MATMULT || op == TRMATMULT || op == MATMULTTR)
		{
			result = doMatMult(arg1,arg2,control, &status);
		} /*if(op == MATMULT || op == TRMATMULT || op == MATMULTTR)*/
		else
		{
			control1 = (control & ~(OPMASK)) | NEARSTRICT;
			if(isscratch(NAME(arg1)) &&
			   (isScalar(arg2) || 
				(isStruc(arg1) || !isStruc(arg2)) &&
				 treecmp(arg1, arg2, control1) == 0))
			{
				control |= REUSELEFT;
			}
			else if(isscratch(NAME(arg2)) &&
			   (isScalar(arg1) || 
				(isStruc(arg2) || !isStruc(arg1)) &&
				 treecmp(arg1, arg2, control1) == 0))
			{
				control |= REUSERIGHT;
			}

			result = doBinary(arg1, arg2,control,&status);

			if(status & FOUNDMISSING)
			{
				sprintf(OUTSTR,
						"WARNING: %s with missing value(s); operation is %s",
						(!bitop) ? "arithmetic" : "bit operation",
						opName(op));
				putErrorOUTSTR();
			} /*if(status & FOUNDMISSING)*/

#ifdef BITOPS
			if(status & FOUNDDOMERR)
			{
				sprintf(OUTSTR,
						"WARNING: bit operation with illegal value(s); operation is %s",
						opName(op));
				putErrorOUTSTR();
			}	/*if(status & FOUNDDOMERR)*/
#endif /*BITOPS*/

			if(status & FOUNDZERODIV)
			{
				putOutErrorMsg("WARNING: Zero divide set to MISSING");
			}					/*if(status & FOUNDZERODIV)*/

			if(status & FOUNDNEG)
			{
				putOutErrorMsg("WARNING: fractional power of negative value");
			}					/*if(status & FOUNDNEG)*/
		}/*if(op == MATMULT || op == TRMATMULT || op == MATMULTTR){}else{}*/
#ifdef HASINFINITY
		if(status & FOUNDOVERFLOW)
		{
			sprintf(OUTSTR,
					"WARNING: result of arithmetic too large, set to MISSING; operation is %s",
					opName(op));
			putErrorOUTSTR();
		} /*if(status & FOUNDOVERFLOW)*/
#endif /*HASINFINITY*/

	} /*if(control != 0)*/
/* fall through */	
  yyerrorMsg:
	if(*OUTSTR)
	{
		yyerror(OUTSTR);
	}
	
 /* remove unconsumed SCRATCH arguments */
	 if(arg1 != (Symbolhandle) 0 &&
		isscratch(NAME(arg1)) && !(control & REUSELEFT))
	 {
		 Removesymbol(arg1);
	 }
	 if(arg2 != (Symbolhandle) 0 &&
		isscratch(NAME(arg2)) && !(control & REUSERIGHT))
	 {
		 Removesymbol(arg2);
	 }
	
	 return (result);
} /*Arith()*/

/*
  routine to do logical operations on arrays and structures of arrays
  operations are 
	  AND, OR, NOT (require LOGICAL operands)
	  EQ, NE, LT, LE, GT, GE (require REAL or CHAR operands)
*/

Symbolhandle    Logic(Symbolhandle arg1, Symbolhandle arg2, int op)
{
	Symbolhandle  result = (Symbolhandle) 0;
	unsigned long control = 0, status = 0, control1;
	long          type = 0, logOp;
	WHERE("Logic");

	OUTSTR[0] = '\0';
	logOp = (op == AND || op == OR || op == NOT);

	if (isNull(arg1) && TYPE(arg1) != NULLSYM
		|| op != NOT && isNull(arg2) && TYPE(arg2) != NULLSYM)
	{
		sprintf(OUTSTR,"ERROR: %s with null argument; operation = %s",
				(logOp) ? "logical operation" : "comparison",opName(op));
		goto yyerrorMsg;
	}
	
	type = TYPE(arg1);
	if(op != NOT)
	{
		control = checkOperands(arg1, arg2, op);
		if(!control)
		{
			goto errorExit;
		}
		control1 = (control & ~(OPMASK)) | NEARSTRICT;
		if(isscratch(NAME(arg1)) && getSingleType(arg1) != CHAR &&
		   (isScalar(arg2) || 
			(isStruc(arg1) || !isStruc(arg2)) &&
			treecmp(arg1, arg2, control1) == 0))
		{
				control |= REUSELEFT;
		}
		else if(isscratch(NAME(arg2)) && getSingleType(arg2) != CHAR &&
		   (isScalar(arg1) || 
			(isStruc(arg2) || !isStruc(arg1)) &&
			treecmp(arg1, arg2, control1) == 0))
		{
			control |= REUSERIGHT;
		}
		result = doBinary(arg1, arg2, control, &status);
	} /*if(op != NOT)*/
	else
	{
		if(type == STRUC)
		{
			if(!isLogic(arg1))
			{
				sprintf(OUTSTR,
						"ERROR: structure components for operand of %s are not all LOGICAL",
						opName(op));
				goto yyerrorMsg;
			}
		}
		else if(type != LOGIC)
		{
			sprintf(OUTSTR,
					"ERROR: nonlogical operand for logical operator");
			yyerror(OUTSTR);
			sprintf(OUTSTR,"%s(%s)",opName(op),typeName(type));
			goto errorExit;
		}
		control = op;
		if(isscratch(NAME(arg1)))
		{
			control |= REUSELEFT;
		}
		result = doUnary(arg1,control, &status);
	} /*if(op != NOT){}else{}*/
	
	if(status & FOUNDMISSING)
	{
		sprintf(OUTSTR,
				"WARNING: %s with missing value(s)",
				(logOp) ? "logical operation" : "comparison");
	} /*if(status & FOUNDMISSING)*/
	
  yyerrorMsg:
	if(*OUTSTR)
	{
		yyerror(OUTSTR);
	}
	/* fall through */
  errorExit:
	putErrorOUTSTR();
	
 /* remove unconsumed SCRATCH arguments */
	 if(arg1 != (Symbolhandle) 0 &&
		isscratch(NAME(arg1)) && !(control & REUSELEFT))
	 {
		 Removesymbol(arg1);
	 }
	 if(op != NOT && arg2 != (Symbolhandle) 0 &&
		isscratch(NAME(arg2)) && !(control & REUSERIGHT))
	 {
		 Removesymbol(arg2);
	 }
	
	return (result);

} /*Logic()*/


static Symbolhandle elementMatrix(Symbolhandle arg,
								  Symbolhandle subscriptsymh)
{
	long            n, m, i, j, ndims, type = TYPE(arg);
	long            k, klast = 0, l, needed;
	long            subscript;
	long            foundBadSubscript = 0;
	long            factors[MAXDIMS], dims[MAXDIMS+1];
	char           *stringx, *stringy;
	double         *x, *y, *subscr;
	Symbolhandle    result = (Symbolhandle) 0;
	WHERE("elementMatrix");
	
	ndims = NDIMS(subscriptsymh);
	n = DIMVAL(subscriptsymh,ndims);
	m = symbolSize(subscriptsymh)/n;

	getDims(dims, arg);
	factors[0] = 1;
	for (j = 1; j <= n; j++)
	{
		factors[j] = dims[j]*factors[j-1];
	}
	
	if (type == CHAR)
	{ /* find how much space is needed */
		needed = 0;
		stringx = STRINGPTR(arg);
		subscr = DATAPTR(subscriptsymh);
		for (i=0;i<m;i++)
		{
			for (j = 1, l = i, k = 0; j<=n; j++, l += m)
			{
				subscript = (long) subscr[l] - 1;
				if(subscript < 0 || subscript >= dims[j])
				{
					k = klast;
					break;
				}
				k += factors[j-1]*subscript;
			} /*for (j = 1, l = i, k = 0; j<=n; j++, l += m)*/
			if (k < klast)
			{
				stringx = STRINGPTR(arg);
				klast = 0;
			}
			if (j <= n)
			{ /* room for null string */
				needed++;
			}
			else
			{
				stringx = skipStrings(stringx, k - klast);
				needed += strlen(stringx) + 1;
				klast = k;
			}
		} /*for (i=0;i<m;i++)*/
		result = CInstall(SCRATCH, needed);
	} /*if (type == CHAR)*/
	else
	{
		result = RInstall(SCRATCH,m);
	}
	
	if(result == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	setTYPE(result,type);
	Setdims(result, ndims - 1, &DIMVAL(subscriptsymh,1));

	/* dereference handles */
	if (type == CHAR)
	{
		stringx = STRINGPTR(arg);
		stringy = STRINGPTR(result);
		klast = 0;
	} /*if (type == CHAR)*/
	else
	{
		x = DATAPTR(arg);
		y = DATAPTR(result);
	} /*if (type == CHAR){}else{}*/

	subscr = DATAPTR(subscriptsymh);
	
	for (i = 0; i<m; i++)
	{
		for (j=1, l = i, k = 0; j<=n; j++, l += m)
		{
			subscript = (long) subscr[l] - 1;
			if(subscript < 0 || subscript >= dims[j])
			{
				foundBadSubscript = 1;
				k = klast;
				break;
			}
			k += factors[j-1]*subscript;
		} /*for (j=1, l = i, k = 0; j<=n; j++, l += m)*/

		if (type == CHAR)
		{
			if (k < klast)
			{
				klast = 0;
				stringx = STRINGPTR(arg);
			} /*if (k < klast)*/
			
			if(j <= n)
			{
				*stringy++ = '\0';
			}
			else
			{
				if (k > klast)
				{
					stringx = skipStrings(stringx, k - klast);
					klast = k;
				}
				stringy = copyStrings(stringx, stringy, 1);
			}
		} /*if (type == CHAR)*/
		else
		{
			if(j <= n)
			{
				setMissing(y[i]);
			}
			else
			{
				y[i] = x[k];
			}
		} /*if (type == CHAR)	{}else{}*/
	} /*for (i = 0; i<m; i++)*/
	if(foundBadSubscript)
	{
		putOutErrorMsg("WARNING: illegal subscripts found.  Result set to MISSING");
	}

	return(result);

  errorExit:
	Removesymbol(result);

	return (0);
	
} /*elementMatrix()*/

#define GALLSUBS    0
#define GCHORDH     1  /* slot for cH or dH */
#define NTRASH      2
/*
   960309 Modified to extract labels
   980312 New argument int * rowsOnly; *rowsOnly = 1 if subscripts select
          all of every dimension except the first; otherwise *rowsOnly = 0
*/
static char   **extractIt(Symbolhandle arg, Symbolhandle list,
						  long resdim[], char ***labels, int * rowsOnly)
{
	long       *subvals[MAXDIMS];	
	long      **allsubs = (long **) 0;
	long        ii[MAXDIMS];
	double    **dH = (double **) 0;
	char      **cH = (char **) 0;
	double     *dP, *y;
	char       *cP;
	char       *chTail, *chPlace;
	long        lastTail, lTail, lLast = 0;
	long        maxSubscr, needed;
	long        labelPlace = 0;
	long        m ,n;
	long        i, j, k, l;
	long        type = TYPE(arg), ndims = NDIMS(arg);
	long        ncomps = NCOMPS(list);
	long        tmplong;
	int         completeDim;
	WHERE("extractIt");
	TRASH(NTRASH,errorExit);
	
	*labels = (char **) 0;
	for (i=0;i<ncomps;i++)
	{
		subvals[i] = (long *) 0;
		resdim[i] = 0;
	} /*for (i=0;i<ncomps;i++)*/

	while (ncomps > ndims && COMPVALUE(list,ncomps-1) == (Symbolhandle) 0)
	{ /* ignore trailing empty subscripts */
		ncomps--;
	}
	
	needed = 0;
	/*
	   set resdim[i] to upper bound to dimension i+1 of result and
	   compute maximum space required to save vectors for each subscript
	   */
	for (i=0;i<ncomps;i++)
	{
		if(ncomps == 1)
		{
			maxSubscr = symbolSize(arg);
		}
		else
		{
			maxSubscr = (i < ndims) ? DIMVAL(arg,i+1) : 1;
		}
		resdim[i] = subscriptLength(COMPVALUE(list,i),maxSubscr);
		if(resdim[i] == 0)
		{
			goto errorExit;
		}
		needed += resdim[i];
	} /*for (i=0;i<ncomps;i++)*/

	if(!getScratch(allsubs,GALLSUBS,needed,long))
	{
		goto errorExit;
	}
	*rowsOnly = 1;
	
	m = n = 1;
	subvals[0] = *allsubs;
	for (i = 0;i < ncomps;i++)
	{
		if(ncomps == 1)
		{
			maxSubscr = symbolSize(arg);
			*rowsOnly = (maxSubscr == DIMVAL(arg, 1));
		}
		else
		{
			maxSubscr = (i < ndims) ? DIMVAL(arg,i+1) : 1;
		}
		resdim[i] = tmplong = buildSubscript(COMPVALUE(list, i),
											 maxSubscr,resdim[i],
											 subvals[i], &completeDim);
		if (i > 0 && !completeDim)
		{
			*rowsOnly = 0;
		}
		
		/*
		   resdim[i] should be actual i+1'st dimension of output
		   and subvals[i][] should contain the 0-base subscripts
		*/
		if(tmplong < 0)
		{
			goto errorExit;
		}
		
		if (tmplong == 0)
		{
			if (*labels != (char **) 0)
			{
				mydisphandle(*labels);
				*labels = (char **) 0;
			}
			m = 0;
			break;
		} /*if (tmplong == 0)*/
		
		if (HASLABELS(arg) && (ncomps > 1 || ndims == 1))
		{
			long    labelNeeded[1];

			/* Compute how much space is needed for labels */
			labelNeeded[0] = 0;
			for (j = 0; j < tmplong; j++)
			{
				labelNeeded[0] += strlen(getOneLabel(arg, i, subvals[i][j])) + 1;
			} /*for (j = 0; j < tmplong; j++)*/
			if (*labels == (char **) 0)
			{
				*labels = createLabels(1, labelNeeded);
			}
			else
			{
				*labels = growLabels(*labels, labelNeeded[0]);
			}
			
			if (*labels == (char **) 0)
			{
				goto errorExit;
			}
			for (j = 0; j < tmplong; j++)
			{
				labelPlace = copyStrings(getOneLabel(arg, i, subvals[i][j]),
										  **labels + labelPlace, 1) - **labels;
			} /*for (j = 0; j < tmplong; j++)*/
		} /*if (HASLABELS(arg) && (ncomps > 1 || ndims == 1))*/
		
		if(i > 0)
		{
			for (j=0;j<tmplong;j++)
			{
				(subvals[i])[j] *= n;
			}
		} /*if(i > 0)*/
		m *= tmplong;
		n *= maxSubscr;
		if (i < ncomps - 1)
		{
			subvals[i+1] = subvals[i] + resdim[i];
		}
	} /*for (i=0;i<ncomps;i++)*/

/* m is number of elements in output, n is number of elements in input */
	if (m == 0)
	{  /* all F's or -run(dimensionLength)*/
		type = CHAR;
		cH = (char **) -1;
	}
	else
	{
		if(type == CHAR)
		{
			/* determine how much room we need for CHARACTER result */
			lastTail = 0;
			chPlace = chTail = STRINGPTR(arg);
			for (i=0;i<ncomps;i++)
			{
				ii[i] = 0;
			} /*for (i=0;i<ncomps;i++)*/
	
			/* compute space needed */
			needed = 0;
			lTail = -1;
			for (k = 0;k < m;k++)
			{ /* loop over destination elements */
				l = (subvals[0])[ii[0]];
				if(lTail < 0)
				{ /* lTail < 0 signals change in ii[j], j > 0 */
					lTail = 0;
					for (j=1;j<ncomps;j++)
					{
						lTail += (subvals[j])[ii[j]];
					}
	
					if(lTail < lastTail)
					{
						lastTail = 0;
						chTail = STRINGPTR(arg);
					}
					chTail = skipStrings(chTail,lTail-lastTail);
					lastTail = lTail;
				} /*if(lTail < 0)*/
				l += lTail;
				if(l < lLast)
				{
					lLast = lTail;
					chPlace = chTail;
				}
				chPlace = skipStrings(chPlace,l-lLast);
				lLast = l;
	
				needed += strlen(chPlace) + 1;
	
				for (j=0;j<ncomps;j++)
				{ /* step odometer */
					if(++ii[j] < resdim[j])
					{
						break;
					}
					lTail = -1;
					ii[j] = 0;
				} /*for (j=0;j<ncomps;j++)*/
			} /*for (k = 0;k < m;k++)*/
			if (!getScratch(cH, GCHORDH, needed, char))
			{
				goto errorExit;
			}
			cP = *cH;
			lastTail = 0;
			chTail = STRINGPTR(arg);
		} /*if(type == CHAR)*/
		else
		{
			if(!getScratch(dH, GCHORDH, m, double))
			{
				goto errorExit;
			}
			dP = *dH;
			y = DATAPTR(arg);
		} /*if(type == CHAR){}else{}*/
		
		/* we have allocated space so we need to dereference allsubs again */
		subvals[0] = *allsubs;
		for (i=0;i<ncomps;i++)
		{
			ii[i] = 0;
			if (i < ncomps - 1)
			{
				subvals[i+1] = subvals[i] + resdim[i];
			}
		} /*for (i=0;i<ncomps;i++)*/
		
		lTail = -1;
		for (k = 0;k < m;k++)
		{ /* loop over destination elements */
			l = (subvals[0])[ii[0]];
			if(lTail < 0)
			{ /* lTail < 0 signals change in ii[j], j > 0 */
				lTail = 0;
				for (j=1;j<ncomps;j++)
				{
					lTail += (subvals[j])[ii[j]];
				}
				/* sequence number of subscripted element is l + ltail */
				if(type == CHAR)
				{
					if(lTail < lastTail)
					{
						lastTail = 0;
						chTail = STRINGPTR(arg);
					}
					chTail = skipStrings(chTail,lTail-lastTail);
					lastTail = lTail;
				} /*if(type == CHAR)*/
			} /*if(lTail < 0)*/
	
			l += lTail;
			if(type == CHAR)
			{
				if(l < lLast)
				{ /* gotta back up*/
					lLast = lTail;
					chPlace = chTail;
				}
				chPlace = skipStrings(chPlace,l-lLast);
				lLast = l;
				cP = copyStrings(chPlace, cP, 1);
			} /*if(type == CHAR)*/
			else
			{
				dP[k] = y[l];
			} /*if(type == CHAR){}else{}*/
	
			for (j=0;j<ncomps;j++)
			{ /* step odometer */
				if(++ii[j] < resdim[j])
				{
					break;
				}
				lTail = -1;
				ii[j] = 0;
			} /*for (j=0;j<ncomps;j++)*/
		} /*for (k = 0;k < m;k++)*/
	
		unTrash(GCHORDH);
	}
	emptyTrash();

	return ((type == CHAR) ? cH : (char **) dH);

  errorExit:
	emptyTrash();

	return (0);
	
} /*extractIt()*/

/*
  980218  Modified behavior of subscripts to a variable
          If any subscripts is NULL, return NULL
          If a subscript for dimension j is -run(DIMVAL(arg1,i)), return NULL

  980310  A subscripted factor remains a factor if the number of subscripts
          matches the dimension.  The number of levels (NCLASS) is not changed.
  980810  Fixed bug - setLabels() was called before Setdims() was called
*/
Symbolhandle    Element(Symbolhandle arg1, Symbolhandle list)
{

	/* routine to extract elements of arrays */

	Symbolhandle    subscriptsymh, result = (Symbolhandle) 0;
	long            argType, subscrType;
	long            ndims, ncomps = NCOMPS(list), resdim[MAXDIMS];
	long            i;
	int             matrixSubscript = 0;
	int             foundNullSub = 0, rowsOnly = 1;
	char          **ch, **labels = (char **) 0;
	char           *badsubscripts = "ERROR: subscripts used with %s%s";
	WHERE("Element");

	OUTSTR[0] = '\0';
	if (!isFakeSymbol(arg1) && !myvalidhandle((char **) arg1) ||
		TYPE(arg1) == LIST)
	{
		sprintf(OUTSTR, badsubscripts, "damaged (deleted?) variable",
				NullString);
	}
	else if (isNull(arg1))
	{
		sprintf(OUTSTR, badsubscripts, "null variable", NullString);
	}
	else if(!isDefined(arg1))
	{
		sprintf(OUTSTR, badsubscripts, "undefined variable ", NAME(arg1));
	}
	else if ((argType = TYPE(arg1)) == BLTIN)
	{
		sprintf(OUTSTR,
				"ERROR: subscripts illegal with function name");
	}
	else if(ncomps > MAXDIMS)
	{
		sprintf(OUTSTR,"ERROR: more than %ld subscripts",(long) MAXDIMS);
	}
	else if (argType == STRUC)
	{
		return (Extract(arg1,list,0));
	}
	else if (argType != REAL && argType != LOGIC && argType != CHAR)
	{	/* subscripts are not of proper type */
			sprintf(OUTSTR,"ERROR: subscripts illegal with type %s variable",
					typeName(argType));
	}
	if(*OUTSTR)
	{
		goto yyerrorMsg;
	}
	
	ndims = NDIMS(arg1);
	
	if (ncomps != 1 && ncomps < ndims)
	{
		sprintf(OUTSTR,"ERROR: too few subscripts");
		goto yyerrorMsg;
	}
	
	subscriptsymh = COMPVALUE(list,0);
	
	if(!isNull(subscriptsymh) && isDefined(subscriptsymh) &&
	   ncomps == 1 &&  !isVector(subscriptsymh))
	{/* single subscript is matrix or array */
		if(DIMVAL(subscriptsymh,NDIMS(subscriptsymh)) != ndims)
		{
			sprintf(OUTSTR,
					"ERROR: last dimension of subscript array of wrong size");
			goto yyerrorMsg;
		}
		matrixSubscript = 1;
	}
	
	for (i = 0; i < ncomps; i++)
	{
		subscriptsymh = COMPVALUE(list, i);
		if (subscriptsymh != (Symbolhandle) 0)
		{/* non-blank subscript */
			if (TYPE(subscriptsymh) == NULLSYM)
			{
				foundNullSub = 1;
			}
			else if(isNull(subscriptsymh))
			{
				sprintf(OUTSTR,"ERROR: null subscript");
			}
			else if(!isDefined(subscriptsymh))
			{
				sprintf(OUTSTR,"ERROR: undefined subscript %s", 
						NAME(subscriptsymh));
			}
			else if ((subscrType = TYPE(subscriptsymh)) != LOGIC &&
					 subscrType != REAL)
			{
				sprintf(OUTSTR,"ERROR: subscript not REAL or LOGICAL");
			}
			else if(anyMissing(subscriptsymh))
			{
				sprintf(OUTSTR,"ERROR: MISSING values not allowed as subscripts");
			}
			else if(matrixSubscript)
			{
				break;
			}
			else if(!isVector(subscriptsymh))
			{
				sprintf(OUTSTR,"ERROR: array used as marginal subscript");
			}
			if(*OUTSTR)
			{
				goto yyerrorMsg;
			}
		} /*if (subscriptsymh != (Symbolhandle) 0)*/
	} /*for (i = 0; i < ncomps; i++)*/

	if (ncomps == 1 && COMPVALUE(list, 0) == (Symbolhandle) 0)
	{ /* single empty subscript: simply copy entire array */
		if(!isscratch(NAME(arg1)))
		{
			result = Install(SCRATCH, (int) TYPE(arg1));
			if (result == (Symbolhandle) 0 || Copy(arg1, result) == 0)
			{
				goto errorExit;
			}
		}
		else
		{ /* already SCRATCH, so reuse it */
			result = arg1;
			arg1 = (Symbolhandle) 0;
		}
		setNAME(result,SCRATCH);
	}
	else if(matrixSubscript) /* matrix of subscripts */
	{		
		result = elementMatrix(arg1,subscriptsymh);
	}
	else
	{
		if (foundNullSub)
		{
			/* if any subscript is NULL, return NULL */
			result = Install(NULLSCRATCH, NULLSYM);
			if (result == (Symbolhandle) 0)
			{
				goto errorExit;
			}
		} /*if (foundNullSub)*/
		else
		{
			/* get handle to extracted data */
			ch = extractIt(arg1, list, resdim, &labels, &rowsOnly);

			if (ch != (char **) 0)
			{
				if (ch == (char **) -1)
				{ /* all F subscripts; return NULL*/
					result = Install(NULLSCRATCH, NULLSYM);
					if (result == (Symbolhandle) 0)
					{
						goto errorExit;
					}
				} /*if (ch == (char **) -1)*/
				else
				{
					result = Install(SCRATCH, (int) argType);
					if (result == (Symbolhandle) 0)
					{
						mydisphandle(ch);
					} /*if (result == (Symbolhandle) 0)*/
					else
					{
						if (argType == CHAR)
						{
							setSTRING(result,ch);
						}
						else
						{
							setDATA(result,(double **) ch);
							setNCLASS(result, (rowsOnly) ? NCLASS(arg1) : -1);
						}
	
						ndims = 0;
						for (i = 0;i < ncomps;i++)
						{
							if(resdim[i] == 0)
							{
								break;
							}
							ndims++;
						}
						Setdims(result, ndims, resdim);
						if (labels != (char **) 0)
						{
							if (!setLabels(result, labels))
							{
								goto errorExit;
							}
							labels = (char **) 0;
						}
#if (USENOMISSING)
						if (NOMISSING(arg1))
						{
							setNOMISSING(result);
						}
#endif /*USENOMISSING*/

					} /*if (result == (Symbolhandle) 0){}else{}*/
				} /*if (ch == (char **) -1){}else{}*/
			} /*if (ch != (char **) 0)*/
		} /*if (foundNullSub){}else{}*/
	}
	if(arg1 != (Symbolhandle) 0 && isscratch(NAME(arg1)))
	{
		Removesymbol(arg1);
	}
	
	mydisphandle(labels);
	OUTSTR[0] = '\0';
	return (result);

  yyerrorMsg:
	yyerror(OUTSTR);
	/* fall through */

  errorExit:
	putErrorOUTSTR();
	mydisphandle(labels);
	Removesymbol(result);

	return (0);
} /*Element()*/

/* compute maximum space needed for scratch for subscript */

long subscriptLength(Symbolhandle subscriptsymh, long maxsubscr)
{
	long         nitems;
	long         subscriptSign;
	WHERE("subScriptLength");
	
	if (subscriptsymh == (Symbolhandle) 0)
	{
		/* use everything for empty list */
		nitems = maxsubscr;
	}
	else if (TYPE(subscriptsymh) == LOGIC)
	{
		nitems = symbolSize(subscriptsymh);
		if(nitems != maxsubscr)
		{
			sprintf(OUTSTR,
					"ERROR: length of LOGICAL subscript vector must match dimension");
			goto yyerrorMsg;
		}
	}
	else
	{
		/* numeric list */
		nitems = symbolSize(subscriptsymh);
		subscriptSign = (DATAVALUE(subscriptsymh,0) >= 0) ? 1 : -1;

		if(subscriptSign < 0)
		{
			if(nitems > maxsubscr)
			{
				sprintf(OUTSTR,"ERROR: too many negative subscripts");
				goto yyerrorMsg;
			}
			else	
			{
				nitems = maxsubscr;
			}
		} /*if(subscriptSign < 0)*/
	}
	return (nitems);

  yyerrorMsg:
	yyerror(OUTSTR);

	return (0);
} /*subscriptLength()*/

/*
  fill vector subValues with subscripts (origin 0) associated with
  subscriptsymh maxsubscr is the maximum permissible value (origin 1) and
  the length of the subscript vector is returned.

  980312 added argument int * completeDim; *completeDim = 1 if the
         subscript is equivalent to an empty subscript (all F's, run(1,dim))
*/

long buildSubscript(Symbolhandle subscriptsymh, long maxsubscr,
					long nitems, long *subValues, int * completeDim)
{
	long         i, j, itmp, mitems, length = nitems;
	long         subscriptSign;
	double       tmp;
	int          complete;
	WHERE("buildSubscript");
	
	complete = 1;
	if (subscriptsymh == (Symbolhandle) 0)
	{
		/* use everything for empty list */
		for (i = 0; i < length; i++)
		{
			subValues[i] = i;
		}
	}
	else if (TYPE(subscriptsymh) == LOGIC)
	{
		/* convert logical to an integer list */
		j = 0;
		for (i = 0; i < length; i++)
		{
			if (DATAVALUE(subscriptsymh,i) != 0.0)
			{
				if (i >= maxsubscr)
				{
					sprintf(OUTSTR,"ERROR: subscript out of range");
					goto yyerrorMsg;
				}
				subValues[j++] = i;
			}
		} /*for (i = 0; i < length; i++)*/
#if (0) /*all F no longer an error */
		if (j == 0)
		{
			sprintf(OUTSTR,"ERROR: all F logical subscript");
			goto yyerrorMsg;
		} /*if (j == 0)*/
#endif
		nitems = j; /* number of items selected */
		if (nitems < length)
		{
			complete = 0;
		}
	}
	else
	{
		/* numeric list */
		subscriptSign = (DATAVALUE(subscriptsymh,0) >= 0) ? 1 : -1;

		mitems = length;
		length = symbolSize(subscriptsymh);
		if(subscriptSign < 0)	
		{
			for (i=0;i<mitems;i++)
			{
				subValues[i] = 0;
			}
			complete = 0;
		} /*if(subscriptSign < 0)	*/

		for (i = 0; i < length; i++)
		{
			tmp = DATAVALUE(subscriptsymh,i);
			if (tmp != floor(tmp))
			{
				sprintf(OUTSTR,"ERROR: noninteger subscript");
				goto yyerrorMsg;
			}
			if((itmp = (long) tmp) == 0 || labs(itmp) > maxsubscr)
			{
				sprintf(OUTSTR,"ERROR: subscript out of range");
				goto yyerrorMsg;
			}
			if(itmp*subscriptSign < 0)
			{
				sprintf(OUTSTR,"ERROR: can't mix positive and negative subscripts");
				goto yyerrorMsg;
			}
			if(subscriptSign > 0)
			{
				subValues[i] = itmp-1;
				if (itmp != i + 1)
				{
					complete = 0;
				}
			}
			else
			{
				if(++subValues[(-itmp)-1] > 1)
				{
					sprintf(OUTSTR,"ERROR: duplicate negative subscripts");
					goto yyerrorMsg;
				}
			}
		} /* for (i=0;i<length;i++) */

		if(subscriptSign < 0)
		{
			j = 0;
			for (i = 0;i<mitems;i++)
			{
				if(subValues[i] == 0)
				{
					subValues[j++] = i;
				}
			}
			nitems = j;
		} /*if(subscriptSign < 0)*/
	}

	if (completeDim != (int *) 0)
	{
		*completeDim = complete;
	}
	
	return (nitems);
	
  yyerrorMsg:
	yyerror(OUTSTR);
	
	return (-1);
	
} /*buildSubscript()*/
