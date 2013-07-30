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
#pragma segment Printano
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
/*
  990212 Replaced two uses of putOUTSTR() by putErrorOUTSTR()
*/
extern Symbolhandle    RInstall();

#define MINLABELWIDTH (NAMELENGTH+1)
#define MAXLABELWIDTH 30
#define MAXCOLUMNCOUNT 5
#define DFWIDTH        5
#define Manova         (MODELTYPE & MANOVA)

#define LABELINDENT    1 /* number of spaces to indent continued term names */

static void printTermLabel(char *termname, long width)
{
	long          i, indent = 0;
	char         *outstr;
	char         *endlabel, *pc, c;

	while (1)
	{
		outstr = OUTSTR;
		for (i = 0; i < indent; i++)
		{
			*outstr++ = ' ';
		}
		if (strlen(termname) <= width)
		{
			sprintf(outstr, "%*s", (int) -width, termname);
			break;
		} /*if (strlen(termname) <= width)*/

		pc = termname;
		endlabel = (char *) 0;
		for (i = 0; i < width; i++)
		{
			*outstr++ = c = *pc++;
			if (c == '.')
			{
				termname = pc;
				endlabel = outstr;
			}
		} /*for (i = 0; i < width; i++)*/
			
		if (endlabel == (char *) 0)
		{
			endlabel = outstr;
			termname = pc;
		}
		*endlabel = '\0';
		putOUTSTR();

		if (indent == 0)
		{
			indent = LABELINDENT;
			width -= indent;
		} /*if (indent == 0)*/
	} /*while (1)*/
} /*printTermLabel()*/

/*
   Function to print ANOVA and ANODEV tables for all GLM commands except
   regress(), wtregress(), and screen().

   arguments fstats and pvals determinine whether fstatistics and P-values
   should be printed.  For manova(), either imply that the SS/SP table
   is not printed, but instead univariate SS, MS, plus F-statistics and/or
   P-values are printed.

   sssp and byvar apply only to manova() and wtmanova().

   If set, sssp determines whether SS/SP are printed.  If not set, they are
   printed if and only if each line of a matrix will fit on the screen.  For
   the default format, this means SS/SP will be printed if and only if
   ncols(y) <= 5.

   If byvar != 0, SS/SP are not printed.  Instead ANOVA tables are printed
   for each variable separately, with F-statistics and/or P-values depending
   on the value of fstats and pvals.

   For manova() and wtmanova(), if byvar == 0 and either fstats != 0 or pvals
   != 0, SS/SP are suppressed and for each term, the corresponding rows of
   the univariate ANOVA tables are printed for each variable.  Thus the
   content of the output is the same as when byvar!=0 and fstats != 0 and/or
   pvals!=0, but it is in a different order.
*/

/*
   print out anova output
   960930 manova() output with fstats:T, pval:T, or byvar:T uses column
   labels of response, if any

   971124 fixed bug with labeled multivariate response matrix whose
   column labels were all ""
*/

void printglm(long fstats, long pvals, long sssp, long byvar)
{
	long            i, iterm, iTable, k, k2;
	long            ny = (long) NY, nterms = (long) NTERMS;
	long            ndata = (long) NDATA;
	long            printTerms;
	long            lengths[2];
	char           *rowcollabs[2], *label = NullString;
	char           *colLabels[MAXCOLUMNCOUNT + 1];
	char            buffer[50];
	char           *termnames, *termnames1;
	char           *outstr,  *undefined = "undefined";
	Symbolhandle    symh = (Symbolhandle) 0;
	double          modeldf = 0.0;
	double         *tmp, *ss, *df;
	double          f;
	double          pvalue, termms, errorms;
	long            errorTerm, useChisq = 0, marginalss = 0;
	long            labelWidth, maxLabelWidth, maxVarLabelLength = 0;
	long            tableWidth, termnameLength, varLabelLength;
	long            ssspWidth, ssspLabelWidth;
	long            nTables = 1;
	WHERE("printglm");

	if (GLMCONTROL & MARGINALSS)
	{
		marginalss = (nAliased() > 0) ? -1 : 1;
	} /*if (GLMCONTROL & MARGINALSS)*/
	
	if (GLMCONTROL & UNBALANCED || !(MODELTYPE & ANOVA))
	{
		if (!marginalss)
		{
			sprintf(OUTSTR, "WARNING: summaries are sequential");
		}
		else
		{
			sprintf(OUTSTR,
					"WARNING: SS%s are %s sums of squares%s",
					(ny==1 || byvar || fstats || pvals) ? "" : "/SP",
					(marginalss > 0) ? "Type III" : "marginal",
					(ny==1 || byvar || fstats || pvals) ? "" : " and products");
		}
		putErrorOUTSTR();
	} /*if (GLMCONTROL & UNBALANCED || !(MODELTYPE & ANOVA))*/

	rowcollabs[1] = (char *) 0;

	if (Manova)
	{
		if (!HASLABELS(MODELVARS[0]))
		{
			ssspLabelWidth = 5 + ((ny > 10) ? 2 : 0);
		} /*if (!HASLABELS(MODELVARS[0]))*/
		else
		{
			long        widths[2];
			
			getAllLabels(MODELVARS[0], rowcollabs, lengths, widths);
			if (lengths[1] == ny)
			{
				rowcollabs[1] = (char *) 0;
				ssspLabelWidth = 0;
			}
			else
			{
				lengths[0] = lengths[1];
				ssspLabelWidth = widths[1];
			}
		} /*if (!HASLABELS(MODELVARS[0])){}else{}*/
		
		ssspWidth = ny * FIELDWIDTH + ssspLabelWidth;

		if(byvar || fstats || pvals)
		{
			sssp = -1;
			if (byvar)
			{
				nTables = ny;
			}
		} /*if(byvar || fstats || pvals)*/
		else if (sssp < 0)
		{ /* ss/sp printed depending on size */
			if (ssspWidth >= SCREENWIDTH)
			{
				sssp = 0;
				sprintf(OUTSTR,
						"NOTE: SS/SP matrices suppressed because of size; use '%s(,sssp:T)'",
						FUNCNAME);
				putErrorOUTSTR();
			}
			else
			{
				sssp = 1;
			}
		} /*if(byvar || fstats || pvals){}else{}*/
	} /*if (Manova)*/
	
	if (Manova && sssp > 0)
	{
		tableWidth = DFWIDTH;
	} /*if (Manova && sssp > 0)*/
	else
	{
		tableWidth = DFWIDTH + 2 * FIELDWIDTH;
		if (pvals)
		{
			tableWidth += FIELDWIDTH;
		}
		if (fstats)
		{
			tableWidth += FIELDWIDTH;
		}
	} /*if (Manova && sssp > 0){}else{}*/
	maxLabelWidth = SCREENWIDTH - 1 - tableWidth;
	maxLabelWidth = (maxLabelWidth < MINLABELWIDTH) ?
		MINLABELWIDTH : maxLabelWidth;
	labelWidth = MINLABELWIDTH;

	termnames = *TERMNAMES;
	for (i = 0; i <= nterms; i++)
	{
		termnameLength = strlen(termnames);
		labelWidth = (termnameLength > labelWidth) ?
			termnameLength : labelWidth;
		termnames = skipStrings(termnames, 1);
	} /*for (i = 0; i <= nterms; i++)*/

	if (rowcollabs[1] != (char *) 0 && sssp < 0 && !byvar &&
		maxVarLabelLength + 1 > labelWidth)
	{		
		labelWidth = maxVarLabelLength+1;
	}
	
	labelWidth = (labelWidth > maxLabelWidth) ? maxLabelWidth : labelWidth;

	if (!Manova || sssp <= 0)
	{ /* ss/sp matrices not printed */
		tableWidth += labelWidth;
	} /*if (!Manova || !sssp)*/
	else
	{
		tableWidth = ny * FIELDWIDTH;
		while (tableWidth >= SCREENWIDTH - ssspLabelWidth)
		{
			tableWidth -= FIELDWIDTH;
		}
		if (tableWidth < FIELDWIDTH)
		{
			tableWidth = FIELDWIDTH;
		}
	} /*if (!Manova || !sssp){}else{}*/
	
	colLabels[MAXCOLUMNCOUNT] = (char *) 0;

	i = 0;
	colLabels[i++] = "DF";
	if (MODELTYPE & ROBUSTREG)
	{
		colLabels[i++] = "SS*";
		colLabels[i++] = "MS*";
		if (fstats)
		{
			colLabels[i++] = "F*";
		}
		if (pvals)
		{
			colLabels[i++] = "P-value*";
		}
	} /*if (MODELTYPE & ROBUSTREG)*/
	else if (GLMCONTROL & (POISSONDIST | BINOMDIST | GAMMADIST))
	{
		colLabels[i++] = "Deviance";
		colLabels[i++] = "MDev";
		if (pvals)
		{
			colLabels[i++] = "P-value";
			useChisq = 1;
		}
	} /*...else if (GLMCONTROL & (POISSONDIST | BINOMDIST | GAMMADIST))*/
	else if (Manova && sssp > 0)
	{ /* print ss/sp tables */
		symh = RInstall(SCRATCH,ny * ny);
		if (symh == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		setNDIMS(symh,2);
		setDIM(symh,1,ny);
		setDIM(symh,2,ny);
		if (rowcollabs[1] != (char *) 0)
		{
			TMPHANDLE = createLabels(2, lengths);
			if (TMPHANDLE != (char **) 0 && setLabels(symh, TMPHANDLE))
			{
				getAllLabels(MODELVARS[0], rowcollabs, (long *) 0, (long *) 0);
				appendLabels(symh, rowcollabs[1], 0, dontExpand);
				appendLabels(symh, rowcollabs[1], 1, dontExpand);
			}
			else
			{
				mydisphandle(TMPHANDLE);
				rowcollabs[1] = (char *) 0;
			}
		} /*if (HASLABELS(MODELVARS[0]))*/
		tmp = DATAPTR(symh);
	} /*... else if (Manova && sssp > 0)*/
	else
	{ /* anova() or manova() with sssp <= 0 */
		colLabels[i++] = "SS";
		colLabels[i++] = "MS";
		if (fstats)
		{
			colLabels[i++] = "F";
		}
		if (pvals)
		{
			colLabels[i++] = "P-value";
		}
		if (Manova && HASLABELS(MODELVARS[0]) && rowcollabs[1] != (char *) 0)
		{
			getAllLabels(MODELVARS[0], rowcollabs, (long *) 0, (long *) 0);
		}
		else
		{
			rowcollabs[1] = (char *) 0;
		}
	}

	colLabels[i] = (char *) 0;

	if (Manova && sssp >= 0)
	{
		outstr = OUTSTR;
		outstr += indentBuffer(outstr,
							   (sssp) ?
							   ssspLabelWidth : labelWidth + DFWIDTH);
		outstr += centerBuffer(outstr, "SS and SP Matrices",
							   (sssp) ? tableWidth : 34);
		putOUTSTR();
	} /*if (Manova && sssp >= 0)*/

	label = buffer;
	for (iTable = 0; iTable < nTables; iTable++)
	{		
		ss = *SS + (nterms + 1) * iTable * (ny + 1);
		df = *DF;
		termnames = *TERMNAMES;

		if (nTables > 1)
		{
			if (rowcollabs[1] != (char *) 0)
			{
				label = (iTable == 0) ? rowcollabs[1] : skipStrings(label, 1);
			}
			else
			{
				sprintf(label, "Variable %ld", iTable + 1);
			}
			centerBuffer(OUTSTR, label, tableWidth);
			putOUTSTR();
		} /*if (nTables > 1)*/
		outstr = OUTSTR;
		outstr += indentBuffer(outstr, labelWidth);
		sprintf(outstr, "%*s", (int) DFWIDTH, colLabels[0]);
		outstr += strlen(outstr);

		if (!Manova || sssp < 0)
		{
			for (i=1; colLabels[i] != (char *) 0; i++)
			{
				outstr += formatChar(outstr, colLabels[i], NOTRIM);
			} /*for (i=1; colLabels[i] != (char *) 0; i++)*/
		} /*if (!Manova || sssp < 0)*/
		putOUTSTR();
	
		checkInterrupt(errorExit);
	
		printTerms = (INCREMENTAL ||
					  !(MODELTYPE & (IPF | ITERGLMS) & ~ROBUSTREG));
		   
		for (iterm = 0; iterm <= nterms; iterm++)
		{
			modeldf += df[iterm];
			outstr = OUTSTR;
			if (Manova && sssp >= 0)
			{
				printTermLabel(termnames, labelWidth);
				outstr += strlen(outstr);
				sprintf(outstr,"%*.0f", (int) DFWIDTH, df[iterm]);
				putOUTSTR();
				if (sssp > 0)
				{
					for (k = 0; k < ny; k++)
					{
						for (k2 = 0; k2 < ny; k2++)
						{
							tmp[k + k2*ny] =
								ss[iterm + (nterms + 1)*(k + k2*ny)];
						}
					} /*for (k = 0; k < ny; k++)*/
					prexpr(symh);
				}
				else
				{
					outstr = OUTSTR;
					outstr += indentBuffer(outstr, labelWidth + DFWIDTH+1);
					sprintf(buffer, "Type 'SS[%ld,,]' to see SS/SP matrix",
							iterm + 1);
					outstr += formatChar(outstr, buffer, TRIMRIGHT);
					putOUTSTR();
				}
				checkInterrupt(errorExit);
			} /*if (Manova && sssp >= 0)*/
			/*if (Manova && !pvals && !fstats)*/
			else
			{
				if(printTerms || iterm == nterms)
				{
					printTermLabel(termnames, labelWidth);
					outstr += strlen(outstr);
					sprintf(outstr,"%*.0f", (int) DFWIDTH, df[iterm]);
					outstr += strlen(outstr);
					if (iterm < nterms && (fstats || pvals) &&
						df[iterm] > 0.0 && !useChisq)
					{			/* determine appropriate error term */
						termnames1 = termnames;
						for (errorTerm=iterm+1; errorTerm <= nterms;
							 errorTerm++)
						{
							termnames1 = skipStrings(termnames1, 1);
							if (strncmp(termnames1, "ERROR", 5) == 0)
							{
								termnames1 += 5;
								if (*termnames1 == '\0')
								{
									termnames1--;
								}
							
								while(isdigit(*termnames1))
								{
									termnames1++;
								}
								if (*termnames1 == '\0')
								{
									break;
								}
							} /*if (strncmp(termnames1, "ERROR", 5) == 0)*/
						} /*for (errorTerm=iterm+1; errorTerm <= nterms; errorTerm++)*/
						if (errorTerm > nterms)
						{
							errorTerm = nterms;
						}
					}

					if (Manova && !byvar)
					{
						putOUTSTR();
						for (k = 0; k < ny; k++)
						{
							k2 = iterm + (nterms + 1)*k*(ny + 1);
							if (df[iterm] > 0.0)
							{
								termms = ss[k2]/df[iterm];
								if (df[errorTerm] > 0)
								{
									errorms =
										(ss[errorTerm + k2 - iterm]/df[errorTerm]);
								}
								else
								{
									errorms = -1.0;
								}
								f = (errorms > 0) ? termms/errorms : -1.0;
								pvalue = (pvals && f > 0) ?
									1.0 - Csne(f,df[iterm],df[errorTerm]) : -1.0;
							} /*if (df[iterm] > 0.0)*/
							else
							{
								pvalue = f = -1.0;
							}
							outstr = OUTSTR;
							if (rowcollabs[1] != (char *) 0)
							{
								label = (k == 0) ?
									rowcollabs[1] : skipStrings(label, 1);
								sprintf(outstr, " %-s",label);
							}
							else
							{
								sprintf(outstr, " Var %2ld", k+1);
							}
							varLabelLength = strlen(outstr);
							outstr += varLabelLength;

							outstr += indentBuffer(outstr,
												   labelWidth + DFWIDTH -
												   varLabelLength) ;
							outstr += formatDouble(outstr,ss[k2], DODEFAULT);
							if (df[iterm] > 0)
							{
								outstr += formatDouble(outstr,termms, DODEFAULT);
							}
							else
							{
								outstr += formatChar(outstr,undefined, NOTRIM);
							}
							if (iterm < nterms)
							{
								if (fstats)
								{
									if (df[iterm] > 0 && f >= 0)
									{
										outstr += formatDouble(outstr,f,
															   DOFIXED);
									}
									else
									{
										outstr += formatChar(outstr, undefined,
															 NOTRIM);
									}
								} /*if (fstats)*/
								if (pvals)
								{
									if (df[iterm] > 0 && f >= 0)
									{
										outstr += formatDouble(outstr,pvalue,
															   DODEFAULT);
									}
									else
									{
										outstr += formatChar(outstr,undefined,
															 NOTRIM);
									}
								} /*if (pvals)*/
							} /*if (iterm < nterms)*/						
							putOUTSTR();
						} /*for (k = 0; k < ny; k++)*/
					} /*if (Manova && !byvar)*/
					else
					{
						outstr += formatDouble(outstr, ss[iterm], DODEFAULT);
						if (df[iterm] > 0)
						{
							termms = ss[iterm] / df[iterm];
							outstr += formatDouble(outstr,termms, DODEFAULT);
						}
						else
						{
							termms = -1.0;
							outstr += formatChar(outstr, undefined, NOTRIM);
						}
						if (iterm < nterms && (fstats || pvals) || useChisq)
						{
							if (!useChisq)
							{
								if (df[iterm] > 0.0 && df[errorTerm] > 0.0)
								{
									errorms = ss[errorTerm]/df[errorTerm];
								}
								else
								{
									errorms = -1.0;
								}
								f = (errorms > 0) ? termms/errorms : -1.0;
								if (fstats)
								{
									if (f < 0)
									{
										outstr += formatChar(outstr, undefined,
															 NOTRIM);
									}
									else
									{
										outstr += formatDouble(outstr, f,
															   DOFIXED);
									}
								} /*if (fstats)*/
							} /*if (!useChisq)*/
				
							if (pvals)
							{
								if (df[iterm] == 0 || !useChisq && f < 0.0)
								{
									outstr += formatChar(outstr, undefined,
														 NOTRIM);
								} /*if (df[iterm] == 0 || !useChisq && f < 0.0)*/
								else
								{
									if (useChisq)
									{
										pvalue = 1 - Cchi(ss[iterm], df[iterm]);
									}
									else
									{
										pvalue = 1.0 - Csne(f, df[iterm], df[errorTerm]);
									}
									outstr += formatDouble(outstr, pvalue,
														   DODEFAULT);
								} /*if (df[iterm] == 0 || !useChisq && f < 0.0){}else{}*/
							} /*if (pvals)*/
						} /*if (iterm < nterms && (fstats || pvals) || useChisq)*/
					} /*if (Manova){}else{}*/
				} /*if(printTerms || iterm == nterms)*/
				else if(iterm == nterms - 1)
				{
					printTermLabel("Overall model", labelWidth);
					outstr += strlen(outstr);
					sprintf(outstr,"%*.0f", (int) DFWIDTH, modeldf);
					outstr += strlen(outstr);
					outstr += formatDouble(outstr, ss[iterm], DODEFAULT);
					outstr += formatDouble(outstr,
										   ss[iterm] / (modeldf > 0 ? modeldf : 1),
										   DODEFAULT);
					if (pvals)
					{
						if (modeldf > 0)
						{
							pvalue = 1 - Cchi(ss[iterm], df[iterm]);
							outstr += formatDouble(outstr, pvalue, DODEFAULT);
						}
						else
						{
							outstr += formatChar(outstr, undefined, NOTRIM);
						}
					} /*if (pvals)*/
				}
				if (!Manova || sssp < 0)
				{
					putOUTSTR();
				}
				checkInterrupt(errorExit);
			} /*if (Manova && sssp >= 0){...}else{...} */
			termnames = skipStrings(termnames, 1);
		} /*for (iterm = 0; iterm <= nterms; iterm++)*/

		if(MODELTYPE & ROBUSTREG)
		{
			putOutMsg("* ANOVA is approximate and should be interpreted with caution");
			myeol();
			outstr = OUTSTR;
			strcpy(outstr, "Robust estimate of sigma: ");
			outstr += strlen(outstr);
			outstr += formatDouble(outstr, CURRENTRSC, TRIMLEFT);
			putOUTSTR();
		}
		else if (MODELTYPE & GLMREG || strcmp(FUNCNAME, "glmfit") == 0)
		{
			myeol();
			sprintf(OUTSTR,
					"Response distribution is %s, link function is %s",
					distName(GLMCONTROL), linkName(GLMCONTROL));
			putOUTSTR();
		}
	} /*for (iTable = 0; iTable < nTables; iTable++)*/
	
	if (marginalss < 0)
	{
		strcpy(OUTSTR,
			   "WARNING: aliased columns in X-variables; interpret results with caution");
		putErrorOUTSTR();
	} /*if (marginalss < 0)*/
	
	/* fall through */
	
  errorExit:
	Removesymbol(symh);

	if (pvals || fstats)
	{
		UNLOADSEG(Cchi);
		UNLOADSEG(Csne);
	}

	*OUTSTR = '\0';
	return;
} /*printglm()*/
