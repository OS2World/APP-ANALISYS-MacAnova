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

enum opCodes
{
	OPN     = 0x0001,
	OPMIN   = 0x0002,
	OPQ1    = 0x0004,
	OPMED   = 0x0008,
	OPQ3    = 0x0010,
	OPMAX   = 0x0020,
	OPMEAN  = 0x0040,
	OPVAR   = 0x0080,
	OPSTDEV = 0x0100,
	OPM2    = 0x0200,
	OPM3    = 0x0400,
	OPM4    = 0x0800,
	OPG1    = 0x1000,
	OPG2    = 0x2000,
	DOMEAN  = OPMEAN | OPVAR | OPSTDEV | OPM2 | OPM3 | OPM4 | OPG1 | OPG2,
	DOS2    = OPVAR | OPSTDEV | OPM2 | OPG1 | OPG2,
	DOS3    = OPM3 | OPG1,
	DOS4    = OPM4 | OPG2,
	DOSORT  = OPQ1 | OPMED | OPQ3
};

enum opindex
{
	JN = 0,
	JMIN,
	JQ1,
	JMEDIAN,
	JQ3,
	JMAX,
	JMEAN,
	JVAR,
	BASICSTATS,
	JSTDEV = BASICSTATS,
	JM2,
	JM3,
	JM4,
	JG1,
	JG2,
	NSTATS
};

static char          *Descompname[] = 
{
	"n", "min", "q1", "median", "q3", "max", "mean", "var",
	"stddev", "m2", "m3", "m4", "g1", "g2"
};

#define MeanVal answers[0]
#define VarVal  answers[1]
#define MinVal  answers[2]
#define MaxVal  answers[3]
#define SDVal   answers[4]
#define M2Val   answers[5]
#define M3Val   answers[6]
#define M4Val   answers[7]
#define G1Val   answers[8]
#define G2Val   answers[9]

#define NANSWERS    10

/* Compute statistics that don't need sorting,
   used whenever q1, median, q3 not wanted so as to avoid need for
   allocating scratch space

   990225 Fixed bug affecting stddev when all values are MISSING
   990226 replaced putOUTSTR() by putErrorOUTSTR()

*/
static long compStats(double *x, long op, long nrows, double answers[NANSWERS],
					  long *status)
{
	long       i, n = 0;
	double     y;
	double     s1 = 0.0, s2 = 0.0, s3 = 0.0, s4 = 0.0;
	
	MinVal = HUGEDBL;
	MaxVal = -MinVal;
			
	for (i = 0;i < nrows; i++)
	{
		y = x[i];
		if (!isMissing(y))
		{
			n++;
			if (op & DOMEAN)
			{
				s1 += y;
			}
			if (op & OPMIN && y < MinVal)
			{
				MinVal = y;
			}
			if (op & OPMAX && y > MaxVal)
			{
				MaxVal = y;
			}
		} /*if (!isMissing(y))*/
	} /*for (i = 0;i < nrows; i++)*/

	if (n < nrows)
	{
		*status |= FOUNDMISSING;
	}
	if (n == 0)
	{
		setMissing(MeanVal);
		setMissing(VarVal);
		setMissing(MinVal);
		setMissing(MaxVal);
		setMissing(SDVal);
		setMissing(M2Val);
		setMissing(M3Val);
		setMissing(M4Val);
		setMissing(G1Val);
		setMissing(G2Val);
	} /*if (n == 0)*/
	else if (op & DOMEAN)
	{
		double         fn = (double) n;
		
#ifdef HASINFINITY
		if (isInfinite(s1))
		{
			setMissing(MeanVal);
			setMissing(VarVal);
			setMissing(G1Val);
			setMissing(G2Val);
			*status |= FOUNDOVERFLOW;
			goto done;
		} /*if (isInfinite(MeanVal))*/
#endif /*HASINFINITY*/

		MeanVal = s1 / fn;

		if (op & ~OPMEAN)
		{
			if (n == 1)
			{
				VarVal = SDVal = G1Val = G2Val = 0.0;
			}
			else
			{
				int       ngt2 = (n > 2);
				int       ngt3 = (n > 3);
				
				for (i = 0;i < nrows; i++)
				{
					y = x[i];
					if (!isMissing(y))
					{
						double       yy;

						y -= MeanVal;

						yy = y*y;
						
						if (op & DOS2)
						{
							s2 += yy;
						}
						
						if (op & DOS3)
						{
							s3 += y*yy;
						}
						if (op & DOS4)
						{
							s4 += yy*yy;
						}
					} /*if (!isMissing(y))*/
				} /*for (i = 0;i < nrows; i++)*/

#ifdef HASINFINITY
				if (op & DOS2 && isInfinite(s2))
				{
					setMissing(VarVal);
					setMissing(SDVal);
					setMissing(M2Val);
					setMissing(M3Val);
					setMissing(M4Val);
					setMissing(G1Val);
					setMissing(G2Val);
					*status |= FOUNDOVERFLOW;
					goto done;
				} /*if (op & DOS2 && isInfinite(s2))*/

				if (op & DOS3 && isInfinite(s3))
				{
					setMissing(G1Val);
					setMissing(M3Val);
					*status |= FOUNDOVERFLOW;
					goto done;
				} /*if (op & DOS3 && isInfinite(s3))*/

				if (op & DOS4 && isInfinite(s4))
				{
					setMissing(G2Val);
					setMissing(M4Val);
					*status |= FOUNDOVERFLOW;
					goto done;
				} /*if (op & DOS4 && isInfinite(s4))*/
#endif /*HASINFINITY*/

				VarVal = s2 / (fn - 1.0);
				SDVal = sqrt(VarVal);
				M2Val = s2 / fn;
				M3Val = s3 / fn;
				M4Val = s4 / fn;
				
				if (ngt2 && (op & OPG1))
				{
					G1Val = (fn*s3/((fn-1.0)*(fn-2.0)))/(SDVal*VarVal);
				}
				else
				{
					G1Val = 0.0;
				}
				
				if (ngt3 && (op & OPG2))
				{
					G2Val = ((fn*(fn+1.0)*s4 -
							  3*(fn-1.0)*s2*s2)/((fn-1.0)*(fn-2.0)*(fn-3.0)))/
					  (VarVal*VarVal);
				}
				else
				{
					G2Val = 0.0;
				}
			} /*if (n > 1)*/
		} /*if (op & ~OPMEAN)*/
	} /*if (n == 0){} else if (op & DOMEAN){}*/

  done:
	
	return (n);		
} /*compStats()*/

static short doDescribe(Symbolhandle arg, Symbolhandle result, long op,
						long *status)
{
	Symbolhandle    symh, result1 = (Symbolhandle) 0;
	Symbolhandle    components[NSTATS], component;
	double         *x, *xscratch = (double *) 0;
	long            n, nmiss, nrows, ncols, i, j, k, l, jj;
	long            ndims, dims[MAXDIMS+1];
	long            nstats = NCOMPS(result);
	long            compno;
	long            varcomp = -1;
#ifdef HASINFINITY
	long            foundOverflow = 0;
#endif /*HASINFINITY*/
	WHERE("doDescribe");

	if (isStruc(arg))
	{
		long            icomp, ncomps = NCOMPS(arg);
		Symbolhandle    component1 = (Symbolhandle) 0;
		
		for (i=0;i<nstats;i++)
		{
			components[i] = COMPVALUE(result, i) = Makestruc(ncomps);
			if (components[i] == (Symbolhandle) 0)
			{
				goto errorExit;
			}
		} /*for (i=0;i<nstats;i++)*/
		
		result1 = StrucInstall(SCRATCH, nstats);
		if (result1 == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		
		for (icomp=0;icomp<ncomps;icomp++)
		{
			symh = COMPVALUE(arg, icomp);
			if (doDescribe(symh, result1, op, status) == 0)
			{
				goto errorExit;
			}
			
			for (i = 0;i < nstats;i++)
			{
				component1 = COMPVALUE(result1, i);
				COMPVALUE(result1, i) = (Symbolhandle) 0;
				COMPVALUE(components[i], icomp) = component1;
				setCompName(component1, NAME(symh));
			}
		} /* for (icomp=0;i<ncomps;i++) */
		Removesymbol(result1);
		return (1);
	} /* if (isStruc(arg)) */

	ncols = 1;
	ndims = NDIMS(arg);
	if (ndims <= 1)
	{
		dims[0] = 1;
	}
	else
	{
		ndims--;
		for (i = 0;i < ndims;i++)
		{
			dims[i] = DIMVAL(arg, i+2);
			ncols *= dims[i];
		}
	}
	
	nrows = DIMVAL(arg, 1);

	for (i = 0;i < nstats;i++)
	{
		components[i] = COMPVALUE(result,i) = Makereal(ncols);
		if (components[i] == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		setNDIMS(components[i], ndims);
		for (j = 0;j < ndims; j++)
		{
			setDIM(components[i], j+1, dims[j]);
		}
	} /*for (i = 0;i < nstats;i++)*/

	if (nrows > 0)
	{
		Symbolhandle    scratchH = (Symbolhandle) 0;
		double          answers[NANSWERS];

		if (op & DOSORT)
		{
			scratchH = RInstall(SCRATCH, nrows);

			if (scratchH == (Symbolhandle) 0)
			{
				goto errorExit;;
			}
			xscratch = DATAPTR(scratchH);
		} /*if (op & DOSORT)*/
		x = DATAPTR(arg);

		l = 0;
		for (jj = 0;jj < ncols;jj++)
		{
			compno = 0;
			if (op & DOSORT)
			{
				nmiss = 0;		/* missing count */
				for (i = 0; i < nrows; i++)
				{
					if (!isMissing(x[l]))
					{
						xscratch[i - nmiss] = x[l];
					}
					else
					{
						nmiss++;
					}
					l++;
				} /*for (i = 0; i < nrows; i++)*/

				if (nmiss > 0)
				{
					*status |= FOUNDMISSING;
				}

				n = nrows - nmiss;
				if (op & OPN)
				{
					DATAVALUE(components[compno++], jj) = (double) n;
				}
			
				if (n > 0)
				{
					sortquick(xscratch, n);
				
					if (op & OPMIN)
					{
						component = components[compno++];
						DATAVALUE(component, jj) = xscratch[0];
					}
					if (op & OPQ1)
					{
						j = (n - 1)/4;
						k = (n + 1)/4;
						DATAVALUE(components[compno++], jj) =
							(xscratch[j] + xscratch[k]) / 2.0;
					}
					if (op & OPMED)
					{
						j = (n - 1) / 2;
						k = n / 2;
						DATAVALUE(components[compno++], jj) =
							(xscratch[j] + xscratch[k]) / 2.0;
					}
					if (op & OPQ3)
					{
						j = n - 1 - (n - 1)/4;
						k = n - 1 - (n + 1)/4;
						DATAVALUE(components[compno++], jj) =
							(xscratch[j] + xscratch[k]) / 2.0;
					} /*if (op & OPQ3)*/

					if (op & OPMAX)
					{
						component = components[compno++];
						DATAVALUE(component, jj) = xscratch[n-1];
					} /*if (op & OPMAX)*/
					
					if (op & DOMEAN)
					{
						double        s1 = 0.0, s2 = 0.0, s3 = 0.0, s4 = 0.0;
						double        mean, fn = (double) n;

						for (i = 0; i < n; i++)
						{
							s1 += xscratch[i];
						}
#ifdef HASINFINITY
						if ((foundOverflow = isInfinite(s1)))
						{
							setMissing(mean);
						}
						else
#endif /*HASINFINITY*/
						{
							mean = s1/fn;
						}
						
						if (op & OPMEAN)
						{
							DATAVALUE(components[compno++], jj) = mean;
						}

						if (op & ~DOMEAN)
						{
							double           var = 0.0, stdev;
							double           g1, g2, m2, m3, m4;
#ifdef HASINFINITY
							if (!foundOverflow)
							{
#endif /*HASINFINITY*/

								for (i = 0; i < n; i++)
								{
									double        res, resq;
									
									res = xscratch[i] - mean;
									resq = res * res;
									if (op & DOS2)
									{
										s2 += resq;
									}
									
									if (op & DOS3)
									{
										s3 += res*resq;
									}
									if (op & DOS4)
									{
										s4 += resq*resq;
									}
								} /*for (i = 0; i < n; i++)*/
#ifdef HASINFINITY
								if (isInfinite(s2))
								{
									setMissing(s2);
									setMissing(s3);
									setMissing(s4);
									foundOverflow = 1;
								}
								if (!isMissing(s2))
								{
#endif /*HASINFINITY*/
									var = (n > 1) ? s2 / (fn - 1.0) : 0.0;
#ifdef HASINFINITY
								}
							} /*if (!foundOverflow)*/
							else
							{
								*status |= FOUNDOVERFLOW;
								setMissing(var);
								setMissing(stdev);
								setMissing(m2);
								setMissing(m3);
								setMissing(m4);
								setMissing(g1);
								setMissing(g2);
							}
#endif /*HASINFINITY*/						
							if (op & OPVAR)
							{
								DATAVALUE(components[compno++], jj) = var;
							}
							if (op & OPSTDEV)
							{
								if (!isMissing(var))
								{
									stdev = sqrt(var);
								}
								else
								{
									setMissing(stdev);
								}
								DATAVALUE(components[compno++], jj) = stdev;
							}
							if (op & OPM2)
							{
								m2 = (!isMissing(s2)) ? s2/fn : s2;
								DATAVALUE(components[compno++], jj) = m2;
							}
							if (op & OPM3)
							{
								m3 = (!isMissing(s3)) ? s3/fn : s3;
								DATAVALUE(components[compno++], jj) = m3;
							}
							if (op & OPM4)
							{
								m4 = (!isMissing(s4)) ? s4/fn : s4;
								DATAVALUE(components[compno++], jj) = m4;
							}

								
							if (op & OPG1)
							{
								if (n <= 3)
								{
									g1 = 0.0;
								}
#ifdef HASINFINITY
								if (isInfinite(s3))
								{
									setMissing(g1);
								}
#endif /*HASINFINITY*/
								if (!isMissing(g1))
								{
									g1 = (fn*s3/((fn-1.0)*(fn-2.0)))/
									  (stdev*var);
								}
								DATAVALUE(components[compno++], jj) = g1;
							} /*if (op & OPG1)*/
							if (op & OPG2)
							{
								if (n <= 3)
								{
									g2 = 0.0;
								}
#ifdef HASINFINITY
								if (isInfinite(s4))
								{
									setMissing(g2);
								}
#endif /*HASINFINITY*/
								if (!isMissing(g2))
								{
									g2 = ((fn*(fn+1.0)*s4 -
											  3*(fn-1.0)*s2*s2)/((fn-1.0)*(fn-2.0)*(fn-3.0)))/
									  (var*var);
								}
								DATAVALUE(components[compno++], jj) = g2;
							} /*if (op & OPG2)*/
						} /*if (op & (OPVAR | OPSTDEV | OPG1 | OPG2))*/
					} /*if (op & DOMEAN)*/
				} /*if (n > 0)*/
				else
				{/* n == 0 */
					for (i = compno;i < nstats;i++)
					{
						setMissing(DATAVALUE(components[i], jj));
					}
				}
			} /*if (op & DOSORT)*/
			else
			{
				n = compStats(x, op, nrows, answers, status);
				if (op & OPN)
				{
					DATAVALUE(components[compno++], jj) = (double) n;
				}
				if (op & OPMIN)
				{
					DATAVALUE(components[compno++], jj) = MinVal;
				}
				if (op & OPMAX)
				{
					DATAVALUE(components[compno++], jj) = MaxVal;
				}
				if (op & OPMEAN)
				{
					DATAVALUE(components[compno++], jj) = MeanVal;
				}
				if (op & OPVAR)
				{
					DATAVALUE(components[compno++], jj) = VarVal;
				}
				if (op & OPSTDEV)
				{
					DATAVALUE(components[compno++], jj) = SDVal;
				}
				if (op & OPM2)
				{
					DATAVALUE(components[compno++], jj) = M2Val;
				}
				if (op & OPM3)
				{
					DATAVALUE(components[compno++], jj) = M3Val;
				}
				if (op & OPM4)
				{
					DATAVALUE(components[compno++], jj) = M4Val;
				}
				if (op & OPG1)
				{
					DATAVALUE(components[compno++], jj) = G1Val;
				}
				if (op & OPG2)
				{
					DATAVALUE(components[compno++], jj) = G2Val;
				}
				x += nrows;
			} /*if (op & DOSORT){}else{}*/
		} /* for (jj = 0;jj < ncols;jj++) */

		if (op & DOSORT)
		{
			Removesymbol(scratchH);
		}
	} /* if (nrows > 0) */
	else
	{/* component is empty */
		for (i = 0;i < nstats;i++)
		{
			symh = components[i];
			setMissing(DATAVALUE(symh, 0));
			setNDIMS(symh, 1);
			setDIM(symh, 1, 1);
		}
	}
	
	return (1);

  errorExit:
	Removesymbol(result1);

	return (0);
} /*doDescribe()*/	

/*
   Function to compute variable statistics, returning a structure
   describe(x) returns n, min, q1, median, q3, max, mean, var
   describe(x,mean:T,var:T), say, computes only mean and var
   describe(x,all:T,mean:F,var:F), say, computes all except mean and var
   
   If x is matrix or array, describe computes the statistics over the
   first dimensions for all combinations of later dimensions.

   If only one statistic is requested, describe() returns a REAL variable,
   not a structure.

   If x is a structure, each component of describe(x) is a structure whose
   components are the result of computing the each statistic for each 
   component of x.

   950823 added keyword all

   990225 Fixed bug affecting stddev when all values are MISSING
*/

Symbolhandle    describe(Symbolhandle list)
{
	Symbolhandle    result = (Symbolhandle) 0, symh, symhkey;
	long            i, j, nstats, nargs = NARGS(list);
	long            status = 0, op, stats[NSTATS];
	char           *keyword;
	WHERE("describe");

	*OUTSTR = '\0';
	
	symh = COMPVALUE(list, 0);
	if (!argOK(symh, 0, 0))
	{
		goto errorExit;
	}
	if (!isReal(symh))
	{
		sprintf(OUTSTR,
				"ERROR: argument%s to %s must be REAL or structure of REALs",
				(nargs == 1) ? "" : " 1", FUNCNAME);
		goto errorExit;
	}
	
	for (j = 0; j < NSTATS; j++)
	{
		stats[j] = (nargs == 1 && j < BASICSTATS) ? 1 : -1;
	}

	for (i = 1;i<nargs;i++)
	{
		symhkey = COMPVALUE(list,i);
		if (!argOK(symhkey,0,i+1))
		{
			goto errorExit;
		}

		if (!(keyword = isKeyword(symhkey)))
		{
			sprintf(OUTSTR,
					"ERROR: all but first argument to %s() must be keyword phrases",
					FUNCNAME);
			goto errorExit;
		}

		if (strcmp(keyword, "all") == 0)
		{
			if (!isTorF(symhkey))
			{
				notTorF(keyword);
				goto errorExit;
			}
			if (DATAVALUE(symhkey, 0) != 0.0)
			{
				for (j = 0; j < NSTATS; j++)
				{
					if (stats[j] < 0)
					{
						stats[j] = 1;
					}
				} /*for (j = 0; j < NSTATS; j++)*/
			} /*if (DATAVALUE(symhkey, 0) != 0.0)*/ 
		} /*if (strcmp(keyword, "all") == 0)*/
		else
		{
			if (strcmp(keyword, "q2") == 0)
			{ /* alternate keyword for median */
				j = JMEDIAN;
			} /*if (strcmp(keyword, "q2") == 0)*/
			else if (strcmp(keyword, "m1") == 0)
			{
				j = JMEAN;
			}
			else
			{
				for (j=0; j<NSTATS; j++)
				{
					if (strncmp(keyword,Descompname[j],
								strlen(Descompname[j])) == 0)
					{
						break;
					}
				} /*for (j=0; j<NSTATS; j++)*/
				if (j == NSTATS)
				{
					badKeyword(FUNCNAME,keyword);
					goto errorExit;
				}
			} /*if (strcmp(keyword, "q2") == 0){}else{}*/			
			if (!isTorF(symhkey))
			{
				notTorF(keyword);
				goto errorExit;
			}
			stats[j] = (DATAVALUE(symhkey,0) != 0.0);
		} /*if (strcmp(keyword, "all") == 0)else{}*/ 
	} /*for (i=1;i<nargs;i++)*/

	op = 0;
	nstats = 0;
	for (j = 0; j < NSTATS; j++)
	{
		if (stats[j] > 0)
		{
			op |= (1L << j);
			stats[nstats++] = j;
		}
	} /*for (j = 0; j < NSTATS; j++)*/

	if (op == 0)
	{
		sprintf(OUTSTR,
				"ERROR: no statistics specified for %s()", FUNCNAME);
		goto errorExit;
	} /*if (op == 0)*/

	if ((result = StrucInstall(SCRATCH, nstats)) == (Symbolhandle) 0 ||
		doDescribe(symh, result, op, &status) == 0)
	{
		goto errorExit;
	}
	
	if (nstats > 1)
	{
		for (i=0;i<nstats;i++)
		{
			setNAME(COMPVALUE(result, i), Descompname[stats[i]]);
		}
	} /*if (nstats > 1)*/
	else
	{
		symh = COMPVALUE(result,0);
		COMPVALUE(result, 0) = (Symbolhandle) 0;
		setNAME(symh, SCRATCH);
		Addsymbol(symh);
		Removesymbol(result);
		result = symh;
	} /*if (nstats > 1){}else{}*/
	
	if (status & FOUNDMISSING)
	{
		putOutErrorMsg("WARNING: missing values in input to describe");
		putOutErrorMsg("WARNING: output reflects non-missing data only");
	} /*if (status & FOUNDMISSING)*/	

	if (status & FOUNDOVERFLOW)
	{
		putOutErrorMsg("WARNING: overflow while computing mean or variance; result(s) set MISSING");
	}
	return (result);
	
  errorExit:
	putErrorOUTSTR();

	Removesymbol(result);
	
	return (0);
} /*describe()*/
