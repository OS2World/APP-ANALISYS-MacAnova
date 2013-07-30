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
#pragma segment Rotfac
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
#include "blas.h"

/*
  Code acquired from Doug Hawkins, University of Minnesota, 12/93

  Comments from Fortran original
	Routine to do a varimax rotation on the real array aload(nv,nnf).
	If nnf is positive, the routine feels free to reverse and reorder
	the factors;this is suppressed in nnf is entered as the negative
	of its actual value.  This suppression is desirable when doing
	a q-mode analysis.

  Translated to C by C. Bingham with the following modifications
     Argument nnf is now nf and is assumed positive and additional argument
     fnorm replaces the local variable fnorm.  If fnorm == (double *) 0,
	 reversing and reordering the factors is suppressed

	 Also argument itmax has been added to control the maximum number of
	 iterations and argument eps provides a convergence limit (originally
	 hard wired as 1e-4).
	 
	 Information on the iteration is printed if verbose != 0

	 varmx() returns a non-zero value if and only if fewer than itmax
	 iterations are required.

	 960919 Modified code to make it easier to add new rotation methods.
            except for slgihtly different error messages, it should have no
            effect on what it does.
     980303 Changed check before computation of rotaton angle to avoid
            atan2 domain error
*/

static long varmx(double *aload,long nv, long nf, double *fnorm,
		   long itmax, double eps, long verbose)
/*float aload[nv][1];*/
{
	double         crit, fcrit, fnv = (double) nv;
	double         t, sq, a, b, d, c2, s2;
	double        *aloadj, *aloadk;
	double         denominator, numerator, angl, ocrit, trot, c, s, ss;
	double         eps1 = eps, eps2 = eps;
	long           inoim = 0, ict = 0, irot = 0;
	long           i, j, k, iflip, nf1 = nf - 1;
	WHERE("varmx");
	
	aloadj = aload;
	crit = 0.0;
	for(j=0;j < nf ;j++)
	{
		s2 = 0.0;
		for(i=0;i < nv ;i++)
		{
			sq = aloadj[i]*aloadj[i];
			s2 += sq;
			crit += sq*sq;
		}
		crit -= s2*s2/fnv;
		aloadj += nv;
	} /*for(j=0;j < nf ;j++)*/

	fcrit = crit;
	do /*while (inoim < 2 && ict < itmax && iflip);*/
	{
		iflip = 0;
		aloadj = aload;
		for(j=0;j < nf1 ;j++)
		{
			aloadk = aloadj + nv;
			for(k=j+1;k < nf ;k++)
			{
				a = b = c = d = 0.0;
				for(i=0;i < nv ;i++)
				{
					c2 = aloadj[i]*aloadj[i] - aloadk[i]*aloadk[i];
					s2 = 2.0*aloadj[i]*aloadk[i];
					a += c2;
					b += s2;
					c += c2*c2 - s2*s2;
					d += c2*s2;
				} /*for(i=0;i < nv ;i++)*/

				denominator = fnv*c + b*b - a*a;
				numerator = 2.0*(fnv*d - a*b);
				if (fabs(numerator) > eps1*fabs(denominator))
				{
					iflip = 1;
					irot++;
					angl = 0.25*atan2(numerator,denominator);
					
					c = cos(angl);
					s = sin(angl);
					for(i=0;i < nv ;i++)
					{
						t = c*aloadj[i] + s*aloadk[i];
						aloadk[i] = -s*aloadj[i] + c*aloadk[i];
						aloadj[i] = t;
					} /*for(i=0;i < nv ;i++)*/
				} /*if (fabs(numerator) >= eps1*fabs(denominator))*/
				aloadk += nv;
			} /*for(k=j+1;k < nf ;k++)*/
			aloadj += nv;
		} /*for(j=0;j < nf1 ;j++)*/
		ict++;

		ocrit = crit;
		crit = 0.0;
		aloadj = aload;
		for(j=0;j < nf ;j++)
		{
			s2 = 0.0;
			for(i=0;i < nv ;i++)
			{
				sq = aloadj[i]*aloadj[i];
				s2 += sq;
				crit += sq*sq;
			} /*for(i=0;i < nv ;i++)*/
			crit -= s2*s2/fnv;
			aloadj += nv;
		} /*for(j=0;j < nf ;j++)*/
        
		trot = (crit > 0.0) ? (crit-ocrit)/crit : 0.0;
		inoim++;
		if (trot > eps2)
		{
			inoim = 0;
		}
	} while (inoim < 2 && ict < itmax && iflip);

	if (fnorm != (double *) 0)
	{
		aloadj = aload;
		for(j=0;j < nf ;j++)
		{
			s = ss = 0.0;
			for(i=0;i < nv ;i++)
			{
				s += aloadj[i];
				ss += aloadj[i]*aloadj[i];
			} /*for(i=0;i < nv ;i++)*/
			fnorm[j] = ss;
			if (s <= 0.0)
			{
				for(i=0;i < nv ;i++)
				{
					aloadj[i] = -aloadj[i];
				}
			} /*if (s <= 0.0)*/

			aloadk = aload;
			for(k=0;k < j ;k++)
			{
				if (fnorm[k] < fnorm[j])
				{
					t = fnorm[k];
					fnorm[k] = fnorm[j];
					fnorm[j] = t;
					for(i=0;i < nv ;i++)
					{
						t = aloadj[i];
						aloadj[i] = aloadk[i];
						aloadk[i] = t;
					} /*for(i=0;i < nv ;i++)*/
				} /*if (fnorm[k] < fnorm[j])*/
				aloadk += nv;
			} /*for(k=0;k < j ;k++)*/
			aloadj += nv;
		} /*for(j=0;j < nf ;j++)*/
	} /*if (fnorm != (double *) 0)*/
	if(verbose)
	{
		char   *outstr = OUTSTR;
		
		outstr += formatChar(outstr, "Varimax starting criterion = ",
							 CHARASIS);
		outstr += formatDouble(outstr, fcrit, DODEFAULT | TRIMLEFT);
		outstr += formatChar(outstr, ", final criterion = ", CHARASIS);
		outstr += formatDouble(outstr, crit, DODEFAULT | TRIMLEFT);
		putOUTSTR();
		sprintf(OUTSTR,
				"%ld iterations and %ld rotations", ict, irot);
		putOUTSTR();
	} /*if(verbose)*/	
	return (ict < itmax);
} /*varmx()*/

/*
   function to perform factor rotation.
   At present (940905) it can do only varimax rotation
   usage:
   	rotatefac(loadings [,method:"varimax"] 
		[,verbose:T] [,reorder:F [,maxiter:m] [,eps:value])
   The default values for verbose and reorder are F and T
   The default values for maxiter and eps are 100 and 1e-5
*/
#ifndef ROTMAXITER
#define ROTMAXITER  100
#endif /*ROTMAXITER*/

#ifndef ROTEPSILON
#define ROTEPSILON 1e-5
#endif /*ROTEPSILON*/

enum rotationScratch
{
	GFNORM = 0,
	GLABELS,
	NTRASH
};

enum rotationMethods
{
	IVARIMAX = 0,
	IQUARTIMAX,
	NMETHODS,
	NOTAVAILABLE = IQUARTIMAX /* first non-implmented recognized method*/
};

typedef struct methodName
{
	char      *name;
	short      length;
} methodName;

static methodName     Methods[NMETHODS] =
{
	{"varimax",    3},
	{"quartimax",  5}
};

static long doRotation(long imethod, double *aload, long nv, long nf,
					 double *fnorm, long itmax, double eps, long verbose)
{
	long      retval = -1;

	if (imethod == IVARIMAX)
	{
		retval = varmx(aload, nv, nf, fnorm, itmax, eps, verbose);
	}
	return (retval);
} /*doRotation()*/

/*
  980303 added check for MISSING values

  980730 added new argument to reuseArg() so that notes will not be kept.

  990226 Replaced most use of putOUTSTR() by putErrorOUTSTR()
*/

Symbolhandle rotatefac(Symbolhandle list)
{
	Symbolhandle     loadings, result = (Symbolhandle) 0;
	Symbolhandle     keySymh;
	long             verbose = 0, reorder = 0, maxiter = ROTMAXITER;
	long             converged;
	long             nargs = NARGS(list);
	double           eps = ROTEPSILON, value;
	double         **fnorm = (double **) 0;
	char            *keyword, *method;
	long             i, imethod = IVARIMAX;
	long             nv, nf, dims[2];
	WHERE("rotatefac");
	TRASH(NTRASH, errorExit);
	
	OUTSTR[0] = '\0';
	
	loadings = COMPVALUE(list,0);
	if (!argOK(loadings, 0, (nargs > 1) ? 1 : 0))
	{
		goto errorExit;
	}
	if (TYPE(loadings) != REAL || !isMatrix(loadings, dims) ||
		anyMissing(loadings))
	{
	sprintf(OUTSTR,
			"ERROR: argument 1 to %s() not a REAL matrix with no MISSING elements",
			FUNCNAME);
	}
	else if(dims[1] > dims[0])
	{
		sprintf(OUTSTR, "ERROR: argument 1 to %s() has more columns than rows",
				FUNCNAME);
	}
	if (*OUTSTR)
	{
		goto errorExit;
	}
	
	nv = dims[0];
	nf = dims[1];
	
	for (i = 1; i < nargs;i++)
	{
		keySymh = COMPVALUE(list,i);
		if (!argOK(keySymh, 0, i+1))
		{
			goto errorExit;
		}
		if (!(keyword = isKeyword(keySymh)))
		{
			sprintf(OUTSTR,
					"ERROR: all except 1st argument to %s() must be keyword phrases",
					FUNCNAME);
			goto errorExit;
		}
		if (strncmp(keyword, "verb", 4) == 0 ||
			strncmp(keyword, "reord", 5) == 0)
		{
			if (!isTorF(keySymh))
			{
				notTorF(keyword);
				goto errorExit;
			}
			value = DATAVALUE(keySymh,0);
			if (keyword[0] == 'v')
			{
				verbose = (value != 0.0);
			}
			else
			{
				reorder = (value != 0.0);
			}
		}
		else if(strncmp(keyword, "maxit", 5) == 0)
		{
			if (!isInteger(keySymh, POSITIVEVALUE))
			{
				notPositiveInteger(keyword);
				goto errorExit;
			}
			maxiter = (long) DATAVALUE(keySymh, 0);
        }
		else if (strncmp(keyword, "eps", 3) == 0)
		{
      		if (!isNumber(keySymh, POSITIVEVALUE))
         	{
				notPositiveReal(keyword);
				goto errorExit;
			}
			eps = DATAVALUE(keySymh, 0);
		}
		else if(strncmp(keyword, "meth", 4) == 0)
		{
			if (!isCharOrString(keySymh))
			{
				notCharOrString(keyword);
				goto errorExit;
			}
			method = STRINGPTR(keySymh);
			for (imethod = 0; imethod < NMETHODS; imethod++)
			{
				if (strncmp(method, Methods[imethod].name,
							 Methods[imethod].length) == 0)
				{
					break;
				}
			} /*for (imethod = 0; imethod < NMETHODS; imethod++)*/
			
			if (imethod == NMETHODS)
			{
				sprintf(OUTSTR,
						"ERROR: %s is not a recognized rotation method for %s()",
						method, FUNCNAME);
			}
			else if (imethod >= NOTAVAILABLE)
			{
				sprintf(OUTSTR,
						"ERROR: %s rotation is not yet implemented for %s()",
						Methods[imethod].name, FUNCNAME);
			}
			if (*OUTSTR)
			{
				goto errorExit;
			}
		}
		else
		{
			badKeyword(FUNCNAME, keyword);
			goto errorExit;
		}
	} /*for (i = 1; i < nargs;i++)*/
	
	if (reorder && !getScratch(fnorm, GFNORM, nf, double))
	{
		goto errorExit;
	}

	if (isscratch(NAME(loadings)))
	{ /* reuse argument */
		result = reuseArg(list, 0, 1, 0);
	}
	else
	{
		result = Install(SCRATCH, REAL);
		if (result == (Symbolhandle) 0 || !Copy(loadings, result))
		{
			Removesymbol(result);
			goto errorExit;
		}
		setNAME(result, SCRATCH);
		clearNotes(result);
	}

	if (HASLABELS(result))
	{
		if (!fixupMatLabels(result, USEROWLABELS))
		{
			goto errorExit;
		}
	} /*if (HASLABELS(result))*/ 
	else
	{
		setNDIMS(result, 2);
		setDIM(result, 1, nv);
		setDIM(result, 2, nf);
	}
	
	setNCLASS(result, -1);
	
	converged = doRotation(imethod, DATAPTR(result), nv, nf,
						   (fnorm != (double **) 0) ? *fnorm : (double *) 0,
						   maxiter, eps, verbose);
	
	if (!converged)
	{
		sprintf(OUTSTR,
				"WARNING: %s rotation did not converge in %ld iterations",
				Methods[imethod].name, maxiter);
		putErrorOUTSTR();
	}
	
	emptyTrash();
	return (result);

  errorExit:
	emptyTrash();
	putErrorOUTSTR();
	return ((Symbolhandle) 0);
	
} /*rotatefac()*/


