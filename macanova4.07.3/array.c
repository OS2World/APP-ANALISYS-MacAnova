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
#pragma segment Array
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

/*
  usage: array(x, dims [,labels:labs] [, notes:charVec])
     or: array(x,n1,n2, . . . [,labels:labs] [, notes:charVec])
  routine to take a set of data an rewrite subscripts into array form
  first argument is data, second argument is vector of dimensions
  data are always entered with leftmost dimension varying fastest
  labs is a CHARACTER vector or structure with CHARACTER vector components

  980723 added keyword 'notes'

  990212 changed putOUTSTR() to putErrorOUTSTR()
  990215 changed myerrorout() to putOutMsg()
*/
Symbolhandle    array(Symbolhandle list)
{

	Symbolhandle    result = (Symbolhandle) 0, arg1, arg2, symhKey;
	Symbolhandle    symhLabels = (Symbolhandle) 0;
	Symbolhandle    symhNotes = (Symbolhandle) 0;
	long            tot1, tot2, i, j, iarg;
	long            ndims1, ndims2, dimi, nargs = NARGS(list);
	long            nargs1 = nargs;
	long            dim[MAXDIMS];
	int             noLabels = 0, noNotes = 0, silent = 0;
	int             reuseArg1Labels, reuseArg1Notes;
	unsigned long   labelError = LABELSOK;
	char           *usage = "usage: array(x,dimVec) or array(x,n1,n2,...)";
	char           *keyword = (char *) 0;
	double          val;
	WHERE("array");
	
	*OUTSTR = '\0';
	
	arg1 = COMPVALUE(list, 0);
	if(!argOK(arg1,0,1))
	{
		goto errorExit;
	}
	if (isKeyword(arg1))
	{
		sprintf(OUTSTR,
				"ERROR: argument 1 to %s must not be a keyword phrase",
				FUNCNAME);
		goto errorExit;
	}
	
	if(TYPE(arg1) != REAL && TYPE(arg1) != LOGIC && TYPE(arg1) != CHAR)
	{
		badType(FUNCNAME,-TYPE(arg1),1);
		goto errorExit;
	}
		
	for (nargs1 = 1; nargs1 < nargs && !isKeyword(COMPVALUE(list, nargs1));
		 nargs1++)
	{
		;
	}

	/* nargs1 is the number of non-keyword arguments */
	for (iarg = nargs1; iarg < nargs; iarg++)
	{
		symhKey = COMPVALUE(list, iarg);
		if (!(keyword = isKeyword(symhKey)))
		{
			sprintf(OUTSTR,
					"ERROR: non-keyword argument must not follow keyword on %s()",
					FUNCNAME);
			goto errorExit;
		}
		if (strcmp(keyword, "silent") == 0)
		{
			if (!isScalar(symhKey) || TYPE(symhKey) != LOGIC)
			{
				notTorF(keyword);
				goto errorExit;
			}
			silent = (DATAVALUE(symhKey, 0) != 0.0);
		}
		else if (strncmp(keyword, "label", 5) == 0)
		{
			if (symhLabels != (Symbolhandle) 0)
			{
				goto duplicateKey;
			}
			symhLabels = symhKey;
			if (TYPE(symhLabels) == NULLSYM)
			{
				noLabels = 1;
				symhLabels = (Symbolhandle) 0;
			}
		}
		else if (strncmp(keyword, "note", 4) == 0)
		{
			if (symhNotes != (Symbolhandle) 0)
			{
				goto duplicateKey;
			}
			symhNotes = symhKey;
			if (TYPE(symhNotes) == NULLSYM)
			{
				noNotes = 1;
				symhNotes = (Symbolhandle) 0;
			}
			else if (!checkArgType(symhNotes, keyword, CHARVECTOR))
			{
				goto errorExit;
			}
		}
		else
		{
			badKeyword(FUNCNAME, keyword);
			goto errorExit;
		}
	} /*for (iarg = 1; iarg < nargs; iarg++)*/
	
	reuseArg1Labels = !noLabels && symhLabels == (Symbolhandle) 0 &&
	  HASLABELS(arg1);

	reuseArg1Notes = !noNotes && symhNotes == (Symbolhandle) 0 &&
	  HASNOTES(arg1);

	tot1 = 1;
	ndims1 = NDIMS(arg1);
	for (i = 0; i < ndims1; i++)
	{
		dim[i] = DIMVAL(arg1, i+1);
		tot1 *= dim[i];
	}
	
	tot2 = (nargs1 == 1) ? tot1 : 1;
	ndims2 = (nargs1 == 1) ? ndims1 : 0;
	for (i = 1;i < nargs1;i++)
	{
		arg2 = COMPVALUE(list,i);
		if (!argOK(arg2,0,i+1))
		{
			goto errorExit;
		}
		if (TYPE(arg2) != REAL || !isVector(arg2) || anyMissing(arg2))
		{
			sprintf(OUTSTR,
					"ERROR: dimensions for %s() must be non-MISSING REAL scalars or vectors",
					FUNCNAME);
			goto errorExit;
		}
		dimi = symbolSize(arg2);
		for (j = 0;j < dimi;j++)
		{
			if (ndims2 >= MAXDIMS)
			{
				sprintf(OUTSTR,
						"ERROR: more than %ld dimensions specified for %s",
						(long) MAXDIMS, FUNCNAME);
				goto errorExit;
			}
			val = DATAVALUE(arg2,j);
		
			if (val!= floor(val) || val < 1.0)
			{
				sprintf(OUTSTR,
						"ERROR: dimension %ld for %s is not positive integer",
						ndims2 + 1, FUNCNAME);
				goto errorExit;
			} /*if (val!= floor(val) || val < 1.0)*/

			if (ndims2 > ndims1 || dim[ndims2] != (long) val)
			{
				reuseArg1Labels = reuseArg1Notes = 0;
			}
			dim[ndims2++] = (long) val;
			tot2 *= (long) val;
		} /*for (j = 0;j < dimi;j++)*/
	} /*for (i = 1;i < nargs1;i++)*/

	if (ndims1 < ndims2)
	{
		reuseArg1Labels = reuseArg1Notes = 0;
	}
	
	if (tot1 != tot2)
	{
		sprintf(OUTSTR,
				"ERROR: product of dimensions for %s does not match length of data",
				FUNCNAME);
		goto errorExit;
	}

	if (symhLabels != (Symbolhandle) 0)
	{		
		labelError = checkLabels(symhLabels, ndims2, dim);
		if ((labelError & LABELSERROR) ||
			labelError != LABELSOK && !silent)
		{
			badLabels(labelError);
		}
		if (labelError & (LABELSERROR | WRONGSIZELABELS))
		{
			noLabels = 1;
			symhLabels = (Symbolhandle) 0;
		}
	} /*if (symhLabels != (Symbolhandle) 0)*/

	usage = (char *) 0;

	if (isscratch(NAME(arg1)))
	{ /* don't need new symbol */
		result = reuseArg(list, 0, reuseArg1Labels, reuseArg1Notes);
	} /*if (isscratch(NAME(arg1)))*/
	else
	{
		result = Install(SCRATCH,TYPE(arg1));
		if (result == (Symbolhandle) 0 || !Copy(arg1, result))
		{
			goto errorExit;
		}
		setNAME(result,SCRATCH);
		if (!reuseArg1Labels)
		{
			clearLabels(result);
		}
		if (!reuseArg1Notes)
		{
			clearNotes(result);
		}
	} /*if (isscratch(NAME(arg1))){}else{}*/

	setNCLASS(result,-1);
	Setdims(result, ndims2, dim);

	if (symhLabels != (Symbolhandle) 0 && !installLabels(symhLabels, result))
	{
		goto errorExit;
	}
		
	if (symhNotes != (Symbolhandle) 0)
	{
		if (setNotes(result, STRING(symhNotes)))
		{
			setSTRING(symhNotes, (char **) 0);
		}
		else
		{
			sprintf(OUTSTR, "WARNING: notes not set in %s()", FUNCNAME);
			putErrorOUTSTR();
		}
	} /*if (symhNotes != (Symbolhandle) 0)*/
	
	return (result);
	
  duplicateKey:
	sprintf(OUTSTR,
			"ERROR: keyword '%s' for %s() used more than once",
			keyword, FUNCNAME);
	/* fall through*/

  errorExit:
	putErrorOUTSTR();
	if(usage)
	{
		putOutMsg(usage);
	}

	Removesymbol(result);

	return (0);
	
} /*array()*/
