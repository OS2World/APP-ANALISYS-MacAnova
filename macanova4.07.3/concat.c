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
#pragma segment Concat
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

/*
  971104 added check for value of myhandlelength()
  990212 changed putOUTSTR() to putErrorOUTSTR()
*/

static long doConcat(Symbolhandle result, Symbolhandle symh,long place)
{
	int              type = TYPE(result);
	long             i, ncomps;
	WHERE("doConcat");
	
	if (!isNull(symh))
	{
		if (TYPE(symh) == STRUC || TYPE(symh) == LIST)
		{
			ncomps = NCOMPS(symh);
			for (i = 0;i < ncomps; i++)
			{
				place = doConcat(result, COMPVALUE(symh,i) ,place);
				if (place == CORRUPTEDHANDLE)
				{
					goto errorExit;
				}
			}
		} /*if (TYPE(symh) == STRUC || TYPE(symh) == LIST)*/
		else
		{			
			if (type == REAL || type == LOGIC)
			{
				register double *x = DATAPTR(symh);
				register double *y = DATAPTR(result) + place;
				register long    n = symbolSize(symh);

				place += n;
		
				while(n-- > 0)
				{
					*y++ = *x++;
				}
#if (USENOMISSING)
				if (!NOMISSING(symh))
				{
					clearNOMISSING(result);
				}
#endif /*USENOMISSING*/
			} /*if(type == REAL || type == LOGIC)*/
			else
			{/* type == CHAR */
				register char   *cx;
				register char   *cy;
				register long    nc = myhandlelength(STRING(symh));

				if (nc == CORRUPTEDHANDLE)
				{
					goto errorExit;
				}
				cx = STRINGPTR(symh);
				cy = STRINGPTR(result) + place;
				
				place += nc;

				while(nc-- > 0)
				{
					*cy++ = *cx++;
				}
			} /*if(type == REAL || type == LOGIC){}else{}*/
		} /*if(TYPE(symh) == STRUC || TYPE(symh) == LIST){}else{}*/
	} /*if(!isNull(symh))*/
	
	return (place);

  errorExit:
	return (CORRUPTEDHANDLE);
} /*doConcat()*/

enum catScratch
{
	GLABELS = 0,
	NTRASH
};

/*
  concatenate the elements in list, collapsing all components of structure
  into a single vector
  All elements being concatenated must have the same type, except that
  NULL arguments are ignored.  Having a missing argument (vector(1,,3)) is
  an error.

  970801 When all arguments are vectors and have labels, the output is
  labelled with the concatenation of all the row labels.

  970914 Fixed bug so that vector(a,labels:NULL) actually removes labels

  980617 If all REAL or LOGICAL arguments, or REAL or LOGICAL components of
         structure arguments have NOMISSING bit set, so does the result

  980724 Added keyword phrase notes:charvec
*/

Symbolhandle    concat(Symbolhandle list)
{
	Symbolhandle    symhLabels = (Symbolhandle) 0, symhKey;
	Symbolhandle    symhNotes = (Symbolhandle) 0;
	Symbolhandle    symhTmp = (Symbolhandle) 0;
	long            type, type1;
	long            iarg, tot, ctot;
	long            nargs = NARGS(list), nargs1 = nargs;
	long            length, silent = -1;
	int             foundData = 0, foundNull = 0, noLabels = 0, noNotes = 0;
	int             allVectorsWithLabels = 1;
	char           *name, *keyword, **labelsH = (char **) 0;
	Symbolhandle    result = (Symbolhandle) 0, symh;
	WHERE("concat");
	TRASH(NTRASH, errorExit);

	*OUTSTR = '\0';

	for (iarg = 0;iarg < nargs;iarg++)
	{
		symh = COMPVALUE(list, iarg);

		if (isKeyword(symh))
		{
			break;
		}
		
		if (!argOK(symh, NULLSYM,(nargs > 1) ? iarg+1 : 0))
		{ /* empty args not allowed, but type NULLSYM are */
			goto errorExit;
		}

		if ((type1 = TYPE(symh)) == NULLSYM)
		{ /* ignore explicit null symbols */
			continue;
		}
		
		if (type1 != REAL && type1 != CHAR && type1 != LOGIC && 
		   type1 != STRUC)
		{
			badType(FUNCNAME,-type1,(nargs > 1) ? iarg+1 : 0);
			goto errorExit;
		}
		
		allVectorsWithLabels = allVectorsWithLabels &&
			isVector(symh) && HASLABELS(symh);
		if (type1 == STRUC)
		{
			type1 = getSingleType(symh);

			if (type1 == 0)
			{
				sprintf(OUTSTR,
						"ERROR: not all components of a structure argument to %s have same type",
						FUNCNAME);
				goto errorExit;
			}
			if (type1 == NULLSYM)
			{/* All components are null; ignore */
				continue;
			}
			
			if(type1 > 0 && type1 != REAL && type1 != LOGIC && type1 != CHAR)
			{
				sprintf(OUTSTR,
						"ERROR: structure components of type %s are illegal for %s()",
						typeName(type1), FUNCNAME);
				goto errorExit;
			}
			foundNull = foundNull || strucAnyNull(symh);
		} /*if(type1 == STRUC)*/

		if (type1 > 0)
		{			
			/*
			  If symh is a structure, at this point we know that
			  all components have type type1, REAL, CHAR, or LOGIC
			  */
			if (!foundData)
			{
				foundData = 1;
				type = type1;
			} /*if (!foundData)*/
			else
			{
				if(type1 != type)
				{
					sprintf(OUTSTR,
							"ERROR: not all arguments to %s() have the same type",
							FUNCNAME);
					goto errorExit;
				}
			} /*if(!foundData){}else{}*/
		} /*if(type1 > 0)*/
	} /*for (iarg = 0;iarg < nargs;iarg++)*/
	
	nargs1 = iarg; /* number of arguments before first keyword phrase*/
	if (nargs1 == 0)
	{
		sprintf(OUTSTR,
				"ERROR: no data found by %s() before the first keyword phrase",
				FUNCNAME);
		goto errorExit;
	} /*if (nargs1 == 0)*/
	
	/*
	   In the following loop we trim off keywords from the tail of
	   list, adjusting NARGS(list) (set by setNCOMPS(list)), so that
	   we can use strucSymbolSize() to compute the total number of
	   elements in the data arguments.  We immediately delete the
	   symbol if any for "silent", and save the "labels" symbol in 
	   symhLabels to be discarded later.

	   Normally, the only reason to zero out an element of the list is
	   when we reuse the argument as a result.
	 */
	while (nargs > nargs1)
	{
		symhKey = COMPVALUE(list, nargs - 1);
		if (!(keyword = isKeyword(symhKey)))
		{
			sprintf(OUTSTR,
					"ERROR: non-keyword argument found by %s() after first keyword phrase",
					FUNCNAME);
			goto errorExit;
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
		else if (strcmp(keyword, "silent") == 0)
		{
			if (silent >= 0)
			{
				goto duplicateKey;
			}
			if (!isTorF(symhKey))
			{
				notTorF(keyword);
				goto errorExit;
			}
			silent = (DATAVALUE(symhKey, 0) != 0.0);
			Removesymbol(symhKey);
		}
		else
		{
			badKeyword(FUNCNAME, keyword);
			goto errorExit;
		}
		nargs--;
		COMPVALUE(list, nargs) = (Symbolhandle) 0;
		setNCOMPS(list, nargs);
	} /*while (nargs > nargs1)*/
			
	silent = (silent < 0) ? 0 : silent;

	if (foundData)
	{
		tot = strucSymbolSize(list); /* total space needed */
		if (symhLabels != (Symbolhandle) 0)
		{ /* keyword 'labels' used */
			if (TYPE(symhLabels) == NULLSYM)
			{
				noLabels = 1;
			} /*if (TYPE(symhLabels) == NULLSYM)*/
			else
			{
				long            ndims = 1;
				long            dims[1];
				unsigned long   labelError;
				Symbolhandle    symhTmp = symhLabels;

				dims[0] = tot;
		
				labelError = checkLabels(symhLabels, ndims, dims);
		
				if (labelError & LABELSERROR || !silent)
				{
					badLabels(labelError);
					if (labelError & LABELSERROR)
					{
						goto errorExit;
					}
				} /*if (labelError & LABELSERROR || !silent)*/

				if (!(labelError & WRONGSIZELABELS))
				{
					if (TYPE(symhLabels) == STRUC)
					{
						symhLabels = COMPVALUE(symhLabels, 0);
					}
			
					if (isScalar(symhLabels))
					{
						length = expandLabels(STRINGPTR(symhLabels),
											  tot, (char *) 0);

						if (!getScratch(labelsH, GLABELS, length, char))
						{
							symhLabels = symhTmp;
							goto errorExit;
						}
						expandLabels(STRINGPTR(symhLabels), tot, *labelsH);
					}
					else
					{
						labelsH = STRING(symhLabels);
						setSTRING(symhLabels, (char **) 0);
						toTrash(labelsH, GLABELS);
					}
					/* at this point labelsH is in the trash*/
				} /*if (!(labelError & WRONGSIZELABELS))*/
				symhLabels = symhTmp;
			} /*if (TYPE(symhLabels) == NULLSYM){}else{}*/		
			Removesymbol(symhLabels);
			symhLabels = (Symbolhandle) 0;
		} /*if (symhLabels != (Symbolhandle) 0)*/
	
		/* At this point, use of 'labels' signaled by labelsH != 0 */

		if (nargs == 1 && TYPE(COMPVALUE(list, 0)) != STRUC)
		{
			int         useOldLabels =
			  labelsH == (char **) 0 && !noLabels && isVector(symh);
			int         useOldNotes = 
			  symhNotes == (Symbolhandle) 0 && !noNotes && isVector(symh);

			symh = COMPVALUE(list, 0);
			if (isscratch(NAME(symh)))
			{
				/* reuse argument */
				result = reuseArg(list, 0, useOldLabels, useOldNotes);
			} /*if (isscratch(NAME(symh)))*/
			else
			{
				result = Install(SCRATCH, TYPE(symh));
				if (result == (Symbolhandle) 0 || !Copy(symh, result))
				{
					goto errorExit;
				}
				setNAME(result, SCRATCH);
				if (!useOldLabels)
				{
					clearLabels(result);
				}
				if (!useOldNotes)
				{
					clearNotes(result);
				}
			} /*if (isscratch(NAME(symh))){}else{}*/	
			setNCLASS(result, -1);

			if (HASLABELS(result))
			{
				/* 
				   Reuse labels
				   labelsH must be 0 and isVector(symh) must be true
			    */
				if (NDIMS(result) > 1)
				{/* not true vector; rebuild labels and save for later */
					long       lengths[2], dims[2];
					char      *rowcollabs[2];

					(void) isMatrix(result, dims);
					getMatLabels(result, rowcollabs, lengths);
					labelsH = createLabels(1, lengths);
					toTrash(labelsH, GLABELS);
					getMatLabels(result, rowcollabs, lengths);
					buildLabels(labelsH, rowcollabs, dims, 1);
				} /*if (NDIMS(symh) > 1)*/
				else
				{/* true vector, save labels for later */
					labelsH = LABELSHANDLE(result);
					toTrash(labelsH, GLABELS);
				}
				setLABELSHANDLE(result, (char **) 0);
			} /*if (HASLABELS(result))*/
		
			Setdims(result, 1, &tot);

			if(tot == 1)
			{
				if(type == CHAR)
				{
					name = STRINGSCRATCH;
				}
				else if(type == REAL)
				{
					name = NUMSCRATCH;
				}
				else
				{
					name = LOGICSCRATCH;
				}
			} /*if(tot == 1)*/
			else
			{
				name = VECSCRATCH;
			}
			setNAME(result, name);
		} /*if (nargs == 1 && TYPE(symh) != STRUC)*/
		else
		{
			if (type == REAL || type == LOGIC)
			{
				result = RInstall(SCRATCH,tot);
				if (result == (Symbolhandle) 0)
				{
					goto errorExit;
				}
				setTYPE(result,type);
#if (USENOMISSING)
				/*
				  NOMISSING bit will be cleared if any piece of result
				  has clear NOMISSING bit
				*/
				setNOMISSING(result);
#endif /*USENOMISSING*/
			} /*if (type == REAL || type == LOGIC)*/
			else
			{					/* type == CHAR */
				ctot = strucSize(list);
				if (ctot < 0)
				{
					goto errorExit;
				}

				result = CInstall(SCRATCH,ctot);
				if(result == (Symbolhandle) 0)
				{
					goto errorExit;
				}
				Setdims(result, 1, &tot);
			} /*if (type == REAL || type == LOGIC){}else{}*/
			if (doConcat(result,list,0) == CORRUPTEDHANDLE)
			{
				goto errorExit;
			}
		} /*if (nargs == 1 && TYPE(symh) != STRUC){}else{}*/

		if (labelsH != (char **) 0)
		{
			unTrash(GLABELS);

			if (!setLabels(result, labelsH))
			{
				toTrash(labelsH, GLABELS);
				goto errorExit;
			}
		} /*if (labelsH != (char **) 0)*/
		else if (!noLabels && allVectorsWithLabels)
		{
			long         needed = 0;
			long         lengths[2];
			char        *labels[2];
			
			for (iarg = 0; iarg < nargs; iarg++)
			{
				symh = COMPVALUE(list, iarg);
				if (!isNull(symh))
				{
					getMatLabels(symh, labels, lengths);
					needed += lengths[0];
				}
			}

			TMPHANDLE = createLabels(1, &needed);
			if (TMPHANDLE == (char **) 0 || !setLabels(result,TMPHANDLE))
			{
				mydisphandle(TMPHANDLE);
				goto errorExit;
			}
			
			if (HASLABELS(result))
			{
				char         *pos = getOneLabel(result, 0, 0);

				for (iarg = 0; iarg < nargs; iarg++)
				{
					symh = COMPVALUE(list, iarg);
					if (!isNull(symh))
					{
						getMatLabels(symh, labels, lengths);
						pos = copyStrings(labels[0], pos, DIMVAL(symh, 1));
					}
				} /*for (iarg = 0; iarg < nargs; iarg++)*/
			} /*if (HASLABELS(result))*/ 
		}
		if (symhNotes != (Symbolhandle) 0)
		{
			if (setNotes(result, STRING(symhNotes)))
			{
				setSTRING(symhNotes, (char **) 0);
				Removesymbol(symhNotes);
				symhNotes = (Symbolhandle) 0;
			}
			else
			{
				sprintf(OUTSTR, "WARNING: notes not set in %s()", FUNCNAME);
				putErrorOUTSTR();
			}
		} /*if (symhNotes != (Symbolhandle) 0)*/
	} /*if (foundData)*/
	else
	{
		result = NULLSYMBOL;
	}

	emptyTrash();
	
	return (result);

  duplicateKey:
	sprintf(OUTSTR,
			"ERROR: keyword '%s' for %s() used more than once",
			keyword, FUNCNAME);
	/* fall through*/

  errorExit:
	putErrorOUTSTR();

	Removesymbol(result);
	Removesymbol(symhLabels);
	Removesymbol(symhNotes);
	emptyTrash();
	
	return (0);
	
} /*concat()*/
