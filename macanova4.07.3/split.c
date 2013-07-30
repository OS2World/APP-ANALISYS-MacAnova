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
#pragma segment Makestr
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

extern Symbolhandle  GarbInstall(), Makereal();
extern void          Delete();
extern char        **mygethandle();
extern void          mydisphandle();

extern void          badNargs();
extern long          argOK(), isVector();
extern long          isFactor();

#define         ISPLITBYFACTOR 1
#define         ISPLITBYCOLS   2
#define         ISPLITBYROWS   3
/*
   Substantially rewritten 930215
  1.  split(x,Factor) now accepts an array as first argument, splitting
      it into a structure of vectors, matrices, or arrays, except that,
	  if isVector(x) is True, each component is a vector, and if isMatrix(x)
      is True, each componen is a matrix.
  2.  split(x,bycols:T) and split(x) are synonymous, splitting x into
      components based on the last subscript, except if isVector(x) is
      True when only a single vector component is created, or isMatrix(x)
      is True, when it splits on the "column" subscript.
  3.  split(x,bycols:T) splits array x by the first subscript.  If
      isVector(x) is True, each component will be a scalar, while if
      isMatrix(x) is True, each component will be a row vector.  In
      particular, after a manova(), ss <- split(SS,byrows:T) produces
      a structure whose components are p by p matrices.
  4.  Keyword phrase compnames:CharVec is recognized and allows you to
      specify component names.  If length(CharVec) == 1, it is used as
      a root to construct names, e.g., compnames:"variety" results in
      names variety1, variety2, ... .  If compnames is not explicitly
      specified, components are named row1, row2, ... for byrows:T, col1,
      col2, ... for bycols:T, factorName1, factorName2, ... if a factor
      is provided, where factorName is the name of the factor (which can
      be specified by keyword, as in variety:rep(run(4),5)).  If the 
      factor is an expression with no keyword name, "comp" is assumed.
  5.  MISSING values in x are not excluded, although "rows" corresponding
      to missing values in Factor are omitted.
      
  980807 Put in check that supplied component names do not contain
         whitespace;  Otherwise they cannot be restored after a binary save.
*/

enum splitScratch
{
	GCOUNTS = 0,
	NTRASH
};

Symbolhandle    split(Symbolhandle list)
{
	Symbolhandle    result = (Symbolhandle) 0, symh, arg1, arg2;
	long          **countsH = (long **) 0;
	double         *class, *x, *y;
	double          dclass;
	long            keyindex;
	long            lclass;
	long            nrows, nvals, nclass, ncols, size, inc;
	long            i, j, maxname;
	long            nargs = NARGS(list), ndims;
	long            dims[MAXDIMS];
	long            nmiss;
	long           *counts, thisCount;
	long            type1, type2, op = -1;
	long            nNames = 0;
	long            argsleft = nargs;
	char          **compNames = (char **) 0;
	char           *keyword, *name;
	char            namestart[NAMELENGTH+1];
	char            tempname[NAMELENGTH+1];
	WHERE("split");
	TRASH(NTRASH,errorExit);

	*OUTSTR = '\0';
	namestart[0] = '\0';
	if(nargs > 3)
	{
		badNargs(FUNCNAME,-3);
		goto errorExit;
	}
	
	if(nargs > 1)
	{
		if((keyindex = findKeyword(list,"byrow",1)) >= 0)
		{
			op = ISPLITBYROWS;
		}
		else if((keyindex = findKeyword(list,"bycol",1)) >= 0)
		{
			op = ISPLITBYCOLS;
		}
		if(keyindex >= 0)
		{
			arg2 = COMPVALUE(list,keyindex);
			keyword = isKeyword(arg2);
			if(arg2 != (Symbolhandle) 0 &&
			   (TYPE(arg2) != LOGIC || !isScalar(arg2) ||
				DATAVALUE(arg2,0) != 1.0))
			{
				sprintf(OUTSTR,
						"ERROR: value for %s must be T",keyword);
				goto errorExit;
			}
			setNAME(arg2,USEDKEYWORD);
			argsleft--;
		} /*if(keyindex >= 0)*/

		if((keyindex = findKeyword(list,"compname",1)) >= 0)
		{
			arg2 = COMPVALUE(list,keyindex);
			keyword = isKeyword(arg2);
			if(TYPE(arg2) != CHAR || !isVector(arg2))
			{
				sprintf(OUTSTR,
						"ERROR: value for %s must be CHARACTER vector or string",
						keyword);
				goto errorExit;
			}
			compNames = STRING(arg2);
			nNames = symbolSize(arg2);
			if(nNames == 1)
			{
				strncpy(namestart,*compNames,NAMELENGTH);
				namestart[NAMELENGTH] = '\0';
			}
			setNAME(arg2,USEDKEYWORD);
			argsleft--;
		} /*if((keyindex = findKeyword(list,"compname",1)) >= 0)*/
	} /*if(nargs > 1)*/

	if(argsleft > 2 || op >= 0 && argsleft > 1)
	{
		sprintf(OUTSTR,"ERROR: too many arguments to %s()",FUNCNAME);
		goto errorExit;
	}

	arg1 = COMPVALUE(list, 0);
	if (!argOK(arg1,0,(nargs == 1) ? 0 :1))
	{
		goto errorExit;
	}

	if((type1 = TYPE(arg1)) != REAL && type1 != LOGIC)
	{
		badType(FUNCNAME,-type1,1);
		goto errorExit;
	}

	if (compNames != (char **) 0)
	{
		char       *what = (char *) 0;
		
		name = *compNames;
		
		for (i = 0; i < nNames; i++)
		{
			char     *pc;
			
			for (pc = name; *pc; pc++)
			{
				if (*pc == '$' || isspace(*pc))
				{
					break;
				}
			}

			if (*pc == '\0')
			{
				if (pc - name > NAMELENGTH)
				{
					what = "is too long";
				}
				else if (pc == name)
				{
					what = "is empty";
				}
			}
			else if (*pc == '$')
			{
				what = "contains '$'";
			}
			else
			{
				what = "contains space or bad character";
			}
	
			if (what)
			{
				break;
			}				
			name = skipStrings(name, 1);
		} /*for (i = 0; i < nNames; i++)*/
		if (what)
		{
			sprintf(OUTSTR,
					"ERROR: A component name for %s() %s",
					FUNCNAME, what);
			goto errorExit;
		} /*if (i < nNames)*/
	} /*if (compNames != (char **) 0)*/
	
	nvals = symbolSize(arg1);

	if(argsleft == 2)
	{ /* must be split(x,factor) form */
		op = ISPLITBYFACTOR;
		arg2 = COMPVALUE(list, 1);
		type2 = TYPE(arg2);
		if(!argOK(arg2,0,2))
		{
			goto errorExit;
		}
		if(type2 != REAL && type2 != LOGIC)
		{
			badType(FUNCNAME,-type2,2);
			goto errorExit;
		}
		
		if(isVector(arg1))
		{
			ndims = 1;
			dims[0] = nvals;
		}
		else if(isMatrix(arg1,dims))
		{
			ndims = 2;
		}
		else
		{
			ndims = NDIMS(arg1);
			for (i = 1; i <= ndims;i++)
			{
				dims[i-1] = DIMVAL(arg1,i);
			}
		}
		nrows = dims[0];
		ncols = nvals/nrows;

		if ((nclass = labs(isFactor(arg2))) == 0)
		{
			sprintf(OUTSTR,"ERROR: second argument to %s() is not a factor",
					FUNCNAME);
		}
		else if (nrows != symbolSize(arg2))
		{
			sprintf(OUTSTR,
					"ERROR: length of factor argument to %s() does not match data",
					FUNCNAME);
		}
		if(*OUTSTR)
		{
			goto errorExit;
		}

		if(!getScratch(countsH,GCOUNTS,nclass,long))
		{
			goto errorExit;
		}

		counts = *countsH;
		for (i = 0; i < nclass; i++)
		{
			counts[i] = 0;
		}

		class = DATAPTR(arg2);
		nmiss = 0; /* missing count */
		for (i = 0; i < nrows; i++)
		{
			if(isMissing(class[i]))
			{
				nmiss++;
			}
			else
			{
				lclass = (type2 == REAL) ? ((long) class[i] - 1) :
					((class[i] != 0.0) ? 1 : 0);
				counts[lclass]++;
			}
		} /*for (i = 0; i < nrows; i++)*/

		if (nmiss > 0)
		{
			if(nmiss < nrows)
			{
				sprintf(OUTSTR,
						"WARNING: %ld missing values in factor argument to %s()",
						nmiss, FUNCNAME);
				putOUTSTR();
				putOutErrorMsg("WARNING: corresponding data deleted from output");
			}
			else
			{
				sprintf(OUTSTR,
						"ERROR: all data for factor argument to %s() are MISSING",
						FUNCNAME);
				goto errorExit;
			}
		} /*if (nmiss > 0)*/

		if(namestart[0] == '\0')
		{ /* compnames not provided; use factor name or "comp" */
			name = NAME(arg2);
			if(isscratch(name))
			{
				name = "comp";
			}
			else if(iskeyname(name))
			{
				name += 2;
			}
			else if(istempname(name))
			{
				name++;
			}
			strcpy(namestart,name);
		} /*if(namestart[0] == '\0')*/
	} /*if(nargs == 2)*/

	if(op < 0)
	{
		op = ISPLITBYCOLS;
	}
	
	if(op != ISPLITBYFACTOR)
	{
		if(isVector(arg1))
		{
			ndims = 1;
			nclass = (op == ISPLITBYCOLS) ? 1 : nvals;
			dims[0] = (op == ISPLITBYCOLS) ? nvals : 1;
		}
		else if(isMatrix(arg1,dims))
		{
			ndims = (op == ISPLITBYCOLS) ? 1 : 2;
			nclass = (op == ISPLITBYCOLS) ? dims[1] : dims[0];
			dims[0] = (op == ISPLITBYCOLS) ? dims[0] : 1;
		}
		else
		{
			ndims = NDIMS(arg1)-1;
			if(op == ISPLITBYCOLS)
			{
				nclass = DIMVAL(arg1,ndims+1);
				for(i=1;i<=ndims;i++)
				{
					dims[i-1] = DIMVAL(arg1,i);
				}
			}
			else
			{
				nclass = DIMVAL(arg1,1);
				for(i=1;i<=ndims;i++)
				{
					dims[i-1] = DIMVAL(arg1,i+1);
				}
			}
		}

		size = nvals/nclass;

		if(namestart[0] == '\0')
		{ /* compnames not provided */
			if(op == ISPLITBYCOLS)
			{
				strcpy(namestart,"col");
			}
			else
			{ /* must be ISPLITBYROWS */
				strcpy(namestart,"row");
			}
		} /*if(namestart[0] == '\0')*/
	} /*if(op != ISPLITBYFACTOR)*/
	
	if(nNames > 1 && nNames != nclass)
	{
		putOutErrorMsg("ERROR: number of component names does not match number of components");
		goto errorExit;
	}

	if(nNames == 1 && nclass > 1)
	{
		nNames = 0;
	}
	
	result = StrucInstall(SCRATCH,nclass);

	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	maxname = NAMELENGTH - nDigits(nclass);
	namestart[maxname] = '\0';

	for (j = 0; j < nclass; j++)
	{ /* set up output structure */
		if (op == ISPLITBYFACTOR)
		{
			thisCount =  (*countsH)[j];
			size = thisCount*ncols;
		}
		
		symh = COMPVALUE(result,j) = (size > 0) ?
			Makereal(size) : Makesymbol(NULLSYM);

		if (symh == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		if (size > 0)
		{
			setTYPE(symh, type1);
		}
		
		if(nNames == 0)
		{
			sprintf(tempname, "%s%ld", namestart,j + 1);
		}
		else
		{
			name = skipStrings(*compNames,j);
			strncpy(tempname,name,NAMELENGTH);
			tempname[NAMELENGTH] = '\0';
		}
		setNAME(symh, tempname);
		if (op != ISPLITBYFACTOR || thisCount > 0)
		{
			setNDIMS(symh,ndims);
			setDIM(symh,1,(op == ISPLITBYFACTOR) ? thisCount : dims[0]);
			for(i=2;i<=ndims;i++)
			{
				setDIM(symh,i,dims[i-1]);
			}
		}
		else
		{
			sprintf(OUTSTR,
					"WARNING: component %ld of %s() output is empty",
					j+1,FUNCNAME);
			putErrorOUTSTR();
		}
	} /*for (j = 0; j < nclass; j++)*/

	if(op == ISPLITBYCOLS)
	{
		x = DATAPTR(arg1);
		for(lclass=0;lclass<nclass;lclass++)
		{
			y = DATAPTR(COMPVALUE(result,lclass));
			doubleCopy(x, y, size);
			x += size;
		}
	}
	else if(op == ISPLITBYROWS)
	{
		for(lclass=0;lclass<nclass;lclass++)
		{
			y = DATAPTR(COMPVALUE(result,lclass));
			x = DATAPTR(arg1) + lclass;
			for(j=0;j<size;j++)
			{
				y[j] = *x;
				x += nclass;
			}
		} /*for(lclass=0;lclass<nclass;lclass++)*/
	}
	else
	{ /* split by factor */
		counts = *countsH;
		class = DATAPTR(arg2);
		for(lclass=0;lclass<nclass;lclass++)
		{
			counts[lclass] = 0;
		}
		
		for(i=0;i<nrows;i++)
		{
			dclass = class[i];
			if(!isMissing(dclass))
			{
				lclass = (type2 == REAL) ? ((long) dclass - 1) :
					((dclass != 0.0) ? 1 : 0);
				symh = COMPVALUE(result,lclass);
				x = DATAPTR(arg1) + i;
				y = DATAPTR(symh) + (counts[lclass]++);
				*y = *x;
				inc = DIMVAL(symh,1);
				for(j=1;j<ncols;j++)
				{
					x += nrows;
					y += inc;
					*y = *x;
				}
			} /*if(!isMissing(dclass))*/ 
		} /*for(i=0;i<nrows;i++)*/
	}
	emptyTrash();

	return (result);

  errorExit:
	putErrorOUTSTR();

	emptyTrash();
	Removesymbol(result);

	return (0);
} /*split()*/
