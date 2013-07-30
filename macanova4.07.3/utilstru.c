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
#pragma segment Utils
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
#include "mainpars.h"
/*
  Various utilities for working with structures.  Some were formerly in 
  utils.c

  960424 Excised fossil unused code that did not use doRecurCheck1() and
  doRecurCheck2().
*/

/*
  doRecur2()
  Handle recursive part of Symbolhandle returning functions that accept
  two structures as arguments.
  Note that to recur on arg1, the LEFTRECUR bit of control must be set and
  to recur on arg2, the RIGHTRECUR bit must be set.  This allows doRecur2
  to be used with functions such as rational() that may have several
  arguments that are not structures as well as one that is.  The non-structure
  arguments can be combined into a temporary STRUC variable and passed
  as arg2 to doRecur2.
  Note: result is in symbol table; this ensures removal in case of an interrupt
  Function doit() must also return symbol in symbol table.

  If both args are structures, they must have same number of components
*/

Symbolhandle doRecur2(Symbolhandle (*doit) (Symbolhandle, Symbolhandle, double *, unsigned long, unsigned long *),
					  Symbolhandle arg1,Symbolhandle arg2,
					  double *params, unsigned long control,
					  unsigned long *status)
{
	Symbolhandle     result = (Symbolhandle) 0, symh;
	long            i;
	long            leftrecur = (isStruc(arg1) && (control & LEFTRECUR));
	long            rightrecur = (isStruc(arg2) && (control & RIGHTRECUR));
	WHERE("doRecur2");

	if(leftrecur || rightrecur)
	{
		long     ncomps = (leftrecur) ? NCOMPS(arg1) : NCOMPS(arg2);
		
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
			result = StrucInstall(SCRATCH,ncomps); /*put in symbol table */
			if(result == (Symbolhandle) 0)
			{
				goto errorExit;
			}
		}
			
		for(i=0;i<ncomps;i++)
		{
			symh = doRecur2(doit,
							(leftrecur) ?  COMPVALUE(arg1,i) : arg1,
							(rightrecur) ? COMPVALUE(arg2,i) : arg2,
							params,control,status);
			if(symh == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			if(!(control & (REUSELEFT | REUSERIGHT)))
			{
				Cutsymbol(symh); /* remove component from symbol table */
			}
			
			COMPVALUE(result,i) = symh;
			setCompName(symh,
						(leftrecur) ?
						NAME(COMPVALUE(arg1,i)) : NAME(COMPVALUE(arg2,i)));
		} /*for(i=0;i<ncomps;i++)*/
	} /*if(leftrecur || rightrecur)*/
	else
	{
		result = doit(arg1, arg2, params, control, status);
	}
	
	return (result);

  errorExit:
	Removesymbol(result);
	return (0);
} /*doRecur2()*/

/*
  doRecur1()
  Handle recursive part of functions that accept one structure as
  argument

  There is no need to set the LEFTRECUR bit in control and it is not checked,
  since it does not make sense for doRecur1 to be called with a STRUC argument
  unless recursion is desired.

  Note: result is in symbol table; this ensures removal in case of an interrupt
  Function doit() must also return symbol in symbol table.
*/

Symbolhandle doRecur1(Symbolhandle (*doit) (Symbolhandle, double *,
											unsigned long, unsigned long *),
					  Symbolhandle arg1, double *params,
					  unsigned long control,unsigned long *status)
{
	Symbolhandle    result = (Symbolhandle) 0, symh;
	long            i;
	long            leftrecur = isStruc(arg1) ;
	WHERE("doRecur1");

	if(leftrecur)
	{
		long     ncomps = NCOMPS(arg1);
		
		if(control & REUSELEFT)
		{
			result = arg1;
		}
		else
		{
			result = StrucInstall(SCRATCH,ncomps); /* in symbol table */
			if(result == (Symbolhandle) 0)
			{
				goto errorExit;
			}
		}
		
		for(i=0;i<ncomps;i++)
		{
			symh = doRecur1(doit,COMPVALUE(arg1,i),params,control,status);
			if(symh == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			if(!(control & REUSELEFT))
			{
				Cutsymbol(symh); /* remove component from symbol table */
			}
			
			COMPVALUE(result,i) = symh;
			setCompName(symh,NAME(COMPVALUE(arg1,i)));
		} /*for(i=0;i<ncomps;i++)*/
	} /*if(leftrecur)*/
	else
	{
		result = doit(arg1, params, control, status);
	}
	
	return (result);

  errorExit:
	Removesymbol(result);
	return (0);
} /*doRecur1()*/

/*
   doRecurCheck2 each element of two structures
   If they have different structures, ignoring names, it returns 0;
   otherwise, it compares corresponding non-structure components using
   function checkit().  If checkit() returns 0 for any component, then
   so does doRecurCheck2().
*/

long doRecurCheck2(long (*checkit) (Symbolhandle, Symbolhandle, double *, unsigned long, unsigned long *),
					  Symbolhandle arg1,Symbolhandle arg2,
					  double *params, unsigned long control,
					  unsigned long *status)
{
	long            i, ncomps, check;
	long            recurboth = control & LEFTRECUR && control & RIGHTRECUR;
	long            leftrecur = (isStruc(arg1) && (control & LEFTRECUR));
	long            rightrecur = (isStruc(arg2) && (control & RIGHTRECUR));
	WHERE("doRecurCheck2");

	if(leftrecur || rightrecur)
	{ /* recursion & at least one is a structure */
		if (recurboth && !(isStruc(arg1) && isStruc(arg2)))
		{
			*status |= BADSTRUC;
			return (0);
		}
		
		ncomps = (leftrecur) ? NCOMPS(arg1) : NCOMPS(arg2);
		
		if (recurboth && ncomps != NCOMPS(arg2))
		{
			*status |= BADSTRUC;
			return (0);
		}
		
		for(i=0;i<ncomps;i++)
		{
			check = doRecurCheck2(checkit,
								  (leftrecur) ?  COMPVALUE(arg1,i) : arg1,
								  (rightrecur) ? COMPVALUE(arg2,i) : arg2,
								  params,control,status);
			if(!check)
			{
				break;
			}
		} /*for(i=0;i<ncomps;i++)*/
	} /*if(leftrecur || rightrecur)*/
	else
	{
		check = checkit(arg1, arg2, params, control, status);
	}
	return (check);
} /*doRecurCheck2()*/

long doRecurCheck1(long (*checkit) (Symbolhandle, double *,
									unsigned long, unsigned long *),
				   Symbolhandle arg1, double *params,
				   unsigned long control,unsigned long *status)
{
	long            i, check;
	long            leftrecur = isStruc(arg1) ;
	WHERE("doRecurCheck1");

	if(leftrecur)
	{
		long     ncomps = NCOMPS(arg1);
		
		for(i=0;i<ncomps;i++)
		{
			check = doRecurCheck1(checkit,COMPVALUE(arg1,i),
								  params,control,status);
			if (!check)
			{
				break;
			}
		} /*for(i=0;i<ncomps;i++)*/
	} /*if(leftrecur)*/
	else
	{
		check = checkit(arg1, params, control, status);
	}
	return (check);
} /*doRecurCheck1()*/

/*
  Test whether all components of a structure satisfy the isMatrix() criterion
  A null component is o.k.
*/

static long argIsMatrix(Symbolhandle arg1, double *params,
						unsigned long control, unsigned long *status)
{
	return (isMatrix(arg1, (long *) 0));
} /*argIsMatrix()*/

long strucIsMatrix(Symbolhandle arg)
{
	unsigned long    status = 0;
	unsigned long    control = LEFTRECUR;
	
	return (doRecurCheck1(argIsMatrix, arg, (double *) 0, control, &status));
} /*strucIsMatrix()*/


/*
  Test whether all components of a structure satisfy the isVector() criterion
  A null component is o.k.
*/

static long argIsVector(Symbolhandle arg1, double *params,
						unsigned long control, unsigned long *status)
{
	return (isVector(arg1));
}

long strucIsVector(Symbolhandle arg)
{
	unsigned long    status = 0;
	unsigned long    control = LEFTRECUR;
	
	return (doRecurCheck1(argIsVector, arg, (double *) 0, control, &status));
} /*strucIsVector()*/

/*
  Determine all the  types of components of a structure, ORing them together
  in the return value.  Ignore null components unless all components are
  null, inwhich case, return NULLTYPE1.
*/

long getStrucTypes(Symbolhandle symh)
{
	long        type = 0, type1;
	long        i, ncomps;
	WHERE("getStrucTypes");
	
	if(isNull(symh))
	{
		type = NULLTYPE1;
	}
	else if(isStruc(symh))
	{
		ncomps = NCOMPS(symh);
		for(i=0;i<ncomps;i++)
		{
			type1 = getStrucTypes(COMPVALUE(symh,i));
			if(type1 != NULLTYPE1)
			{
				type |= type1;
			}
		}
		if(type == 0)
		{ /* all components were NULLTYPE1 */
			type = NULLTYPE1;
		}
	}
	else
	{
		type = TYPE(symh);
		if(type == REAL)
		{
			type = REALTYPE1;
		}
		else if(type == LOGIC)
		{
			type = LOGICTYPE1;
		}
		else if(type == CHAR)
		{
			type = CHARTYPE1;
		}
		else if(type == PLOTINFO)
		{
			type = PLOTINFOTYPE1;
		}
		else if(type == LIST)
		{
			type = LISTTYPE1;
		}
		else if(type == NULLSYM)
		{
			type = NULLTYPE1;
		}
		else
		{
			type = UNKNOWNTYPE1;
		}
	}
	
	return (type);
} /*getStrucTypes()*/

/*
  getSingleType() scans a structure.  If all non-STRUC, non-null components
  have the same type, it returns that type.  If all components are null,
  it returns NULLSYM.  Otherwise it returns 0.
*/
long getSingleType(Symbolhandle symh)
{
	long         type = getStrucTypes(symh);
	WHERE("getSingleType");
	
	if(type == REALTYPE1)
	{
		type = REAL;
	}
	else if(type == LOGICTYPE1)
	{
		type = LOGIC;
	}
	else if(type == CHARTYPE1)
	{
		type = CHAR;
	}
	else if(type == STRUCTYPE1)
	{
		type = STRUC;
	}
	else if(type == PLOTINFOTYPE1)
	{
		type = PLOTINFO;
	}
	else if(type == LISTTYPE1)
	{
		type = LIST;
	}
	else if(type == NULLTYPE1)
	{
		type = NULLSYM;
	}
	else
	{
		type = 0;
	}
	
	return (type);
} /*getSingleType()*/

/*
   Scan a structure for the presence of null (null handle or 0 length)
   symbols.  If any is found, return 1, else return 0
*/
static long argIsNotNull(Symbolhandle arg1, double *params,
						unsigned long control, unsigned long *status)
{
	return (!isNull(arg1));
} /*argIsNotNull()*/

long strucAnyNull(Symbolhandle arg)
{
	unsigned long    status = 0;
	unsigned long    control = LEFTRECUR;
	
	return (!doRecurCheck1(argIsNotNull, arg, (double *) 0, control, &status));
} /*strucAnyNull()*/

static long argNotType(Symbolhandle arg1, double *dtype,
					   unsigned long control, unsigned long *status)
{
	return (arg1 == (Symbolhandle) 0 || TYPE(arg1) != (long) dtype[0]);
} /*argNotType()*/

/*
	Scan a structure for the presence of an component of a specific type.
	If any is found, return 1, else return 0
*/
long strucAnyType(Symbolhandle arg, long type)
{
	unsigned long    status = 0;
	unsigned long    control = LEFTRECUR;
	double           dtype[1];
	
	dtype[0] = (double) type;
	return (!doRecurCheck1(argNotType, arg, dtype, control, &status));

} /*strucAnyNull()*/

/*
   Compute sum of symbolSize(comp) for all non STRUC components of argument
*/

long strucSymbolSize(Symbolhandle arg)
{
	long       size = 0;
	long       i, ncomps;
	int        type;
	
	if(!isNull(arg))
	{
		type = TYPE(arg);
		if(type == STRUC || type == LIST)
		{
			ncomps = NCOMPS(arg);
			for(i=0;i<ncomps;i++)
			{
				size += strucSymbolSize(COMPVALUE(arg,i));
			}
		}
		else
		{
			size = symbolSize(arg);
		}
	} /*if(!isNull(arg))*/
	return (size);
} /*strucSymbolSize()*/

/* 
   Compute number of bytes in all non-STRUC components of a structure
   Currently, if a component has type PLOTINFO, a full count of bytes is
   not given.  This does not matter at present (931208) since strucSize()
   is used only when all the components of its argument have types
   REAL, LOGIC or CHAR.

   Note:  Unlike SizeofSymbol(), strucSize() does not count "overhead" bytes
   like sizeof(Symbol), but only content bytes.
*/

long strucSize(Symbolhandle arg)
{
	long        i, ncomps;
	long        size = 0;
	int         type;
	WHERE("strucSize");
	
	if(!isNull(arg))
	{
		type = TYPE(arg);
		if(type == STRUC || type == LIST)
		{
			ncomps = NCOMPS(arg);
			for(i=0;i<ncomps;i++)
			{
				long        size1 = strucSize(COMPVALUE(arg,i));
				
				if (size1 < 0)
				{
					size = size1;
					break;
				}
				
				size += size1;
			} /*for(i=0;i<ncomps;i++)*/
		} /*if(type == STRUC || type == LIST)*/
		else
		{
			switch (TYPE(arg))
			{
			  case LOGIC:
			  case REAL:
				TMPHANDLE = (char **) DATA(arg);
				break;
			  case CHAR:
			  case MACRO:
				TMPHANDLE = STRING(arg);
				break;
			  case PLOTINFO:
				TMPHANDLE = (char **) GRAPH(arg);
				break;
			}
			size = myhandlelength(TMPHANDLE);
		} /*if(type == STRUC || type == LIST){}else{}*/
	} /*if(!isNull(arg))*/
	return (size);
} /*strucSize()*/
	
/*
   strucAnyMissing() returns 1 if its argument contains missing values, or
   any of its components do.  The second argument should normally be
   (REALTYPE1 | LOGICTYPE1) or (REALTYPE1 | LOGICTYPE1 | CHARTYPE1)
*/
static long argHasNoMissing(Symbolhandle arg1, double *params,
						unsigned long control, unsigned long *status)
{
	long      reply = 1;
	
	if (!isNull(arg1) &&
		(TYPE(arg1) == CHAR && control & CHARTYPE1) ||
		(TYPE(arg1) == REAL && control & REALTYPE1) ||
		(TYPE(arg1) == LOGIC && control & LOGICTYPE1))
	{
		reply = !anyMissing(arg1);
	}
	return (reply);
} /*argIsMissing()*/

long strucAnyMissing(Symbolhandle arg, unsigned long control)
{
	unsigned long    status = 0;

	control |= LEFTRECUR;
	
	return (!doRecurCheck1(argHasNoMissing, arg, (double *) 0, control,
						   &status));
} /*strucAnyMissing()*/

/*
  dimcmp() returns 0 if and only if the dimensions of arg1 and arg2 match,
  with the level of strictness required specified by control.
  
  op = (control & OPMASK) specifies form of check on dimensions

  op == 0 no check on dimensions
  op == STRICT exact match of dimensions
  op == NEARSTRICT exact match except for extra trailing dimensions of 1
  op == MATMULT compatible for %*%
  op == TRMATMULT compatible for %c%
  op == MATMULTTR compatible for %C%
  Otherwise (binary op token), compatible for binary elementwise operation

  Two null components always match.

  980806 When checking dimensions for matrix multiplication, the
         error message is put in OUTSTR but is not printed
*/

long dimcmp(Symbolhandle arg1, Symbolhandle arg2,
			unsigned long control)
{
	long     ndims1, ndims2, lastDim;
	long     dim1[MAXDIMS], dim2[MAXDIMS];
	long     i, maxNdims, minNdims;
	int      op = control & OPMASK;
	long     null1 = isNull(arg1), null2 = isNull(arg2);
	WHERE("dimcmp");
	
	if(op == 0)
	{
		return (0);
	}
	
	if(null1 || null2)
	{ /* o.k. if both are null */
		return ((!null1 || !null2) ? BADDIMS : 0);
	}
	
	ndims1 = NDIMS(arg1);
	ndims2 = NDIMS(arg2);
	switch(op)
	{
	  case STRICT:
		/* extra trailing dimensions of 1 not allowed */
		if(ndims1 != ndims2)
		{
			return (BADDIMS);
		}
		for(i=1;i<=ndims1;i++)
		{
			if(DIMVAL(arg1,i) != DIMVAL(arg2,i))
			{
				return (BADDIMS);
			}
		} /*for(i=1;i<=ndims1;i++)*/
		break;
		
	  case NEARSTRICT:
		/* extra trailing dimensions of 1 are allowed */
		minNdims = (ndims1 < ndims2) ? ndims1 : ndims2;
		maxNdims = (ndims1 > ndims2) ? ndims1 : ndims2;
		for(i=1;i<=maxNdims;i++)
		{
			if(i <= minNdims && DIMVAL(arg1,i) != DIMVAL(arg2,i) ||
			   i > ndims1 && DIMVAL(arg2,i) > 1 ||
			   i > ndims2 && DIMVAL(arg1,i) > 1)
			{
				return(BADDIMS);
			}
		} /*for(i=1;i<=maxNdims;i++)*/
		break;

	  case MATMULT:
	  case TRMATMULT:
	  case MATMULTTR:
		/* check for compatibility for matrix multiplication */
		if(!isMatrix(arg1,dim1) || !isMatrix(arg2,dim2))
		{
#if (1)  /* leave in OUTSTR for caller (checkOperands() in Lang.c) to use*/
			strcpy(OUTSTR, "ERROR: matrix multiplication with non-matrices");
#else /*1*/
			putOutErrorMsg("ERROR: matrix multiplication with non-matrices");
#endif /*1*/
			return (BADDIMS);
		}
		if(anyMissing(arg1) || anyMissing(arg2))
		{
			return (HASMISSING);
		}
		
		if(op == MATMULT && dim1[1] != dim2[0] ||
		   op == TRMATMULT && dim1[0] != dim2[0] ||
		   op == MATMULTTR && dim1[1] != dim2[1])
		{
			sprintf(OUTSTR,
				"ERROR: Dimension mismatch: %ld by %ld %s %ld by %ld",
				dim1[0], dim1[1], opName(op) , dim2[0], dim2[1]
			);
#if (0) /* leave in OUTSTR for caller (checkOperands() in Lang.c) to use*/
			putOUTSTR();
#endif /*0*/
			return (BADDIMS);
		}

		break;
		
	  default:
		/* must be elementwise operation */
		if(!isScalar(arg1) && !isScalar(arg2))
		{ /* only mismatch possible is when neither are scalars */
			maxNdims = (ndims1 > ndims2) ? ndims1 : ndims2;
			for(i=0;i<maxNdims;i++)
			{ /* copy dimensions and pad with 1's if necessary */
				if(i<ndims1)
				{
					dim1[i] = DIMVAL(arg1,i+1);
				}
				else
				{
					dim1[i] = 1;
				}
				if(i < ndims2)
				{
					dim2[i] = DIMVAL(arg2,i+1);
				}
				else
				{
					dim2[i] = 1;
				}
			} /* for(i=0;i<maxNdims;i++)*/
			lastDim = maxNdims-1;
			if(dim1[0] != dim2[0] && dim1[0] != 1 && dim2[0] != 1 ||
			   lastDim > 0 && dim1[lastDim] != dim2[lastDim] &&
			   dim1[lastDim] != 1 && dim2[lastDim] != 1)
			{  /* first and last dimensions > 1 must match */
				return (BADDIMS);
			}
			for(i=1;i<lastDim;i++)
			{/* middle dimensions must match exactly */
				if(dim1[i] != dim2[i])
				{
					return (BADDIMS);
				}
			} /*for(i=1;i<lastDim;i++)*/
		} /*if(!isScalar(arg1) && !isScalar(arg2))*/
		
		break;
	} /*switch(op)*/
	return (0);
} /*dimcmp()*/

/*
  treecmp() returns zero if and only if str1 and str2 have same tree structure,
  specifically
  (1)  both are null, or
  (2)  both are non-structures of the same type, or
  (3)  both are structures with the same number of components, and
       corresponding components have the same tree structure
  Argument control is of the form leftrecur+rightrecur+op
  leftrecur == 0 specifies left is not structure & no left recurrence
      should occur
  rightrecur == 0 specifies right is not structure & no right recurrence
      should occur
  op specifies form of check on dimensions
  op == 0 no check on dimensions
  op == STRICT exact match of dimensions
  op == NEARSTRICT exact match except for extra trailing dimensions of 1
  op == MATMULT compatible for %*%
  op == TRMATMULT compatible for %c%
  op == MATMULTTR compatible for %C%
  Otherwise, compatible for binary elementwise operation
  If there is a mismatch in tree structure, treecmp returns BADSTRUC
  If there is a mismatch in dimensions, treecmp returns BADDIMS
*/


static long argsCmp(Symbolhandle arg1, Symbolhandle arg2, double *params,
						unsigned long control, unsigned long *status)
{
	long         op = control & OPMASK;
	long         null1 = isNull(arg1), null2 = isNull(arg2);
	
	if (null1 || null2)
	{
		if (!(null1 && null2))
		{
			*status |= BADSTRUC;
		}
	}
	else if (op != 0)
	{
		*status |= dimcmp(arg1, arg2, control);
	}

	return (*status == 0);
	
} /*argsCmp()*/

long treecmp(Symbolhandle arg1, Symbolhandle arg2, unsigned long control)
{
	unsigned long        status = 0;
	long                 reply;
	WHERE("treecmp");
	
	reply = doRecurCheck2(argsCmp, arg1, arg2, (double *) 0, control, &status);
	return ((reply != 0) ? 0 : status);
} /*treecmp()*/

