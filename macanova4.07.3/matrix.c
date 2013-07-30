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
#pragma segment Matrix
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
/*
  usage: matrix(x [, dims] [, labels:labs , silent:T)

  Routine to take a set of data an rewrite subscripts into matrix form
  first argument is data, second argument is number of rows
  data are always entered down columns first.  If dims is missing,
  x must be a "generalized matrix" and the result is a genuine (2 dimensional)
  matrix equivalent to x.

  941027
  Added usage: matrix(matVar), with no dimensions.  This is legal only when
  ismatrix(matVar) is true in which case it strips off unnecessary dimensions
  of length 1, if necessary.

  960315
  Add keywords labels and silent

  980724 added keyword notes

  980911 Fixed bug in getting dimensions of argument 1 when it is a matrix

  990212 Changed putOUTSTR() to putErrorOUTSTR()
  
  990322 Fixed bug in setting dimensions when argument was labelled vector
*/

Symbolhandle    matrix(Symbolhandle list)
{
	Symbolhandle    result = (Symbolhandle) 0, arg1, arg2, symhKey;
	Symbolhandle    symhLabels = (Symbolhandle) 0;
	Symbolhandle    symhNotes = (Symbolhandle) 0;
	double          value;
	long            tot, type;
	long            nargs = NARGS(list), dims[2], ndims;
	long            nargs1, iarg;
	int             noLabels = 0, silent = 0;
	int             useArg1Labels, useArg1Notes;
	int             noNotes = 0;
	unsigned long   labelError = LABELSOK;
	char           *keyword;
	WHERE("matrix");
	
	*OUTSTR = '\0';

	if (nargs > 4)
	{
		badNargs(FUNCNAME, -4);
		goto errorExit;
	}

	arg1 = COMPVALUE(list, 0);
	if (!argOK(arg1, 0, (nargs == 1) ? 0 : 1))
	{
		goto errorExit;
	}
	if (isKeyword(arg1))
	{
		sprintf(OUTSTR,
				"ERROR: argument 1 to %s() must not be keyword phrase",
				FUNCNAME);
		goto errorExit;
	}
	
	type = TYPE(arg1);
	if(type != REAL && type != LOGIC && type != CHAR)
	{
		badType(FUNCNAME,-type,(nargs == 1) ? 0 : 1);
		goto errorExit;
	}
	(void) isMatrix(arg1, dims);
	/* find first keyword phrase; nargs1 = number of non-keyword args */
	for (nargs1 = 1; nargs1 < nargs && !isKeyword(COMPVALUE(list, nargs1));
		 nargs1++)
	{
		;
	}
	if (nargs1 > 2)
	{
		sprintf(OUTSTR,
				"ERROR: %s() can have at most 2 non-keyword arguments",
				FUNCNAME);
		goto errorExit;
	}

	for (iarg = nargs1; iarg < nargs; iarg++)
	{ /* parse keywords */
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
			if (!isTorF(symhKey))
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
	} /*for (iarg = nargs1; iarg < nargs; iarg++)*/

	useArg1Labels = (symhLabels == (Symbolhandle) 0) && isMatrix(arg1, (long *) 0) &&
	  HASLABELS(arg1);
	useArg1Notes = (symhNotes == (Symbolhandle) 0) && isMatrix(arg1, (long *) 0) &&
	  HASNOTES(arg1) && !noNotes;

	if (nargs1 == 1)
	{ /* matrix(matrixVar); nrows not supplied */
		if (!isMatrix(arg1, (long *) 0))
		{
			sprintf(OUTSTR,
					"ERROR: for %s(arg), arg must be an array with at most two dimensions > 1",
					FUNCNAME);
			goto errorExit;
		}
	} /*if (nargs1 == 1)*/
	else
	{ /* matrix(data,nrows) */
		arg2 = COMPVALUE(list, 1);
		if (!argOK(arg2,0,2))
		{
			goto errorExit;
		}
		if (!isInteger(arg2,POSITIVEVALUE))
		{
			notPositiveInteger("2nd argument");
			goto errorExit;
		}
		tot = symbolSize(arg1);
		value = DATAVALUE(arg2, 0);

		/*
		  Use labels and notes from argument 1 only if no new ones are
		  provided and arg1 is a matrix or generalized matrix with the
		  nrows(arg1) == value
		*/
		if (value != dims[0])
		{
			useArg1Labels = useArg1Notes = 0;
		}
		
		dims[0] = (long) value;
		dims[1] = tot / dims[0];

		if (dims[0] * dims[1] != tot)
		{
			sprintf(OUTSTR,"ERROR: number of rows must divide length of data");
			goto errorExit;
		}
	} /*if (nargs1 == 1){}else{}*/

	if (symhLabels != (Symbolhandle) 0)
	{
		/* keyword labels used, not arg 1 labels */
		if (TYPE(symhLabels) == NULLSYM)
		{ /* labels:NULL*/
			noLabels = 1;
		} /*if (TYPE(symhLabels) == NULLSYM)*/
		else
		{
			labelError = checkLabels(symhLabels, 2, dims);
			if ((labelError & LABELSERROR) ||
				labelError != LABELSOK && !silent)
			{
				badLabels(labelError);
				if (labelError & LABELSERROR)
				{
					goto errorExit;
				}
			}
			if (labelError & WRONGSIZELABELS)
			{ /* can't use labels */
				noLabels = 1;
			}
		} /*if (TYPE(symhLabels) == NULLSYM){}else{}*/		
	} /*if (symhLabels != (Symbolhandle) 0)*/
		
	ndims = NDIMS(arg1);
	if (isscratch(NAME(arg1)))
	{ /* reuse symbol, removing it from list, keeping labels */
		result = reuseArg(list, 0, useArg1Labels, useArg1Notes);
	}
	else
	{
		result = Install(SCRATCH,TYPE(arg1));
		if (result == (Symbolhandle) 0 || Copy(arg1, result) == 0)
		{
			goto errorExit;
		}
		if (!useArg1Labels)
		{
			clearLabels(result);
		}
		if (!useArg1Notes)
		{
			clearNotes(result);
		}
	}
	setNAME(result,SCRATCH);
	
	/* 
	   useArg1Labels != 0 only if keyword labels not used && arg1
	   is generalized matrix with first dimension matching row dimension
	   supplied; result should still have the labels of arg1 if any only
	   when useArg1Labels != 0
	*/
	if (!useArg1Labels)
	{
		Setdims(result, 2, dims);
		if (symhLabels != (Symbolhandle) 0 && !noLabels && 
			!installLabels(symhLabels, result))
		{
			goto errorExit;
		}
	} /*if (!useArg1Labels)*/
	else
	{
		/* 
		   We know arg1 must have satisfied isMatrix(arg1) && dimensions match
		   The original labels should still be there.
		*/
		if (ndims < 2 && !fixupMatLabels(result, USEROWLABELS))
		{
			goto errorExit;
		}
		else if (ndims > 2 && !fixupMatLabels(result, USEBOTHLABELS))
		{
			goto errorExit;
		}
		Setdims(result, 2, dims);
	} /*if (!useArg1Labels){}else{}*/

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
	
	setNCLASS(result,-1);

	return (result);

  duplicateKey:
	sprintf(OUTSTR,
			"ERROR: keyword '%s' for %s() used more than once",
			keyword, FUNCNAME);
	/* fall through*/

  errorExit:
	putErrorOUTSTR();

	Removesymbol(result);
	
	return (0);
} /*matrix()*/
