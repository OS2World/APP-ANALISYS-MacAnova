/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 4.06 or later
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
#pragma segment Kmeans
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

#include "globals.h"

/*
  990215 changed myerrorout() to putOutMsg() and putOutErrorMsg()
         changed 2 uses of putOUTSTR() to putErrorOUTSTR()
*/
#define Rands1 RANDS1  /* global long */
#define Rands2 RANDS2  /* global long */

#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

static int trwcla(double * /*data*/, double * /*weight*/,
		   double * /*classes*/, double * /*criterion*/,
		   long * /*icl*/, long /*nmax*/, long /*nmin*/, long /*nvar*/,
		   long /*ndat*/, long /*iran*/, double * /*working*/);


#if (0)
void puticl(long icl [], long ndata, long ncl, char *lab)
{
	int         i, j, k = 0;
	int         nperline = 15;
	int         nlines = (ndata-1)/nperline + 1;

	printf("Membership for %d cluster solution for %s\n",ncl, lab);
	
	for (j = 0; j < nlines; j++)
	{
		printf("%4d: ",k+1);
		for (i = 0; i < nperline && k < ndata; i++, k++)
		{
			printf("%4d",icl[k] + 1);
		}
		printf("\n");
	} /*for (j = 0; j < nlines; j++)*/
} /*puticl()*/
#endif /*0*/

enum clusterKeycodes
{
	KKMIN = 0,
	KKMAX,
	KSTART,
	KWEIGHTS,
	KQUIET,
	KSTANDARD
};

keywordList         KmeansKeys[] =
{
	InitKeyEntry("kmin",4,0,POSITIVEINT),
	InitKeyEntry("kmax",4,0,POSITIVEINT),
	InitKeyEntry("start",5,0,CHARSCALAR),
	InitKeyEntry("weights",6,0,POSITIVEVECTOR),
	InitKeyEntry("quiet",5,0,LOGICSCALAR),
	InitKeyEntry("standardize",5,0,LOGICSCALAR)
};

#define Kmin            KeyIntValue(KmeansKeys,KKMIN)
#define Kmax            KeyIntValue(KmeansKeys,KKMAX)
#define Start           KeyCharValue(KmeansKeys,KSTART)
#define Weights         KeySymhValue(KmeansKeys,KWEIGHTS)
#define Quiet           KeyLogValue(KmeansKeys,KQUIET)
#define Standardize     KeyLogValue(KmeansKeys,KSTANDARD)

enum trwclaCodes
{
	PREDEFINEDSTART = 0,
	CANDIDATEMEANS,
	OPTIMUMSTART,
	RANDOMSTART,
	NSTARTMETHODS
};

#define CODEMASK       0x00ff
#define STANDARDIZE    0x0100
#define VERBOSE        0x0200

#define StartMethod    (iop & CODEMASK)
#define Rescale        (iop & STANDARDIZE)
#define Report         (iop & VERBOSE)
#define Weighted       (weight != (double *) 0)
#define Weight(I)      ((Weighted) ? weight[I] : 1.0)

#define Qdata(I,J) data[(I) + (ndata*(J))]
#define Qave(I,J)  ave[(I) + (kmax*(J))]

static char    *Label[] =
{
	 "predefined", "by nearest candidate mean", "optimized", "random"
};

static char     *StartValues[] =
{
	"class", "mean", "optim", "random"
};

 /*
   Almost the original comments; no longer complete accruate.

   Routine to do a trace w (alias k means) cluster analysis.
   The input is the data array data(nvar,ndat), and weight(ndat),
   the mandatory vector of case wieghts.  Output consists of
   icl(ndat), the final vector of allocations of items to clusters.
   And ave(nvar,nmax), the cluster means for the different
   solutions found.  Note: Only the icl for the smallest number of
   cluster is output;if the whole range is wanted, trwcla must be
   re-entered repeatedly, preferably using the iran facility.

   nmax and min are the maximum and minimum numbers of clusters to
   be fitted.  starting from nmax cluster, the routine drops
   down one by one to end with min clusters.

   lab is a one-word character label which appears on the output, and
   iou (optionally zero) is the 'tape 7' file on which results are
   stored.  The iran controls the initial cluster allocation -
   iop  = 1 for predefined allocation. this is input in icl
		  2 for optimized allocation, found by trwcla.
		  3 for random allocation, found by trwcla.
   The routine may be entered repeatedly - usefully by putting
   different allocations into icl and setting iop=1, or by setting
   iop=3, each repetition of which will give a fresh random
   initial allocation

   961217   Code adapted from subroutine of Douglas M. Hawkins,
            doug@stat.umn.edu
 */

#if (0) /* original trwcla random number generator, for debuggine*/
#define RANMAX    48619
#define RANMULCON   211
#define RANADDCON 15073
#define RANSTART   8941

static long     Ix = RANSTART;

#define rani(DUMMY) (Ix = (RANMULCON * Ix + RANADDCON) % RANMAX)
#define ranf(DUMMY) (rani(DUMMY), (double) Ix / (double) RANMAX)

#define duni() ranf(0)
#define iuni() rani(0)
#endif

static int trwcla(double * data, double * weight,
				  double * classes, double *criterion,
				  long * icl, long kmax, long kmin, long nvar, long ndata,
				  long iop, double * working)
{
	double   *ave = working;
	double   *fn = ave + nvar*kmax;
	double   *scale = fn + kmax;
	long      isearch = 10;
	double    small = 1e-8;
#if (0)
	double    eps1 = .90, eps2 = .95; /* original values */
#else
	double    eps1 = 1.0, eps2 = 1.0;
#endif
	double    big = HUGEDBL;
	double    d, s;
	double    eps, twoeps, p1, tdm, distmn, dmin, r, t, w, crit, ocrit;
	double    cmin, rs, ss, dsto, ri, r2n, d1, d2;
	long      i, i1, j, k, k1, ncl;
	long      low, last, isto, nn, iwhat, igp, noc;
	long      icd, nearest, ialloc, jsto, ksto;
	char     *outstr;
	WHERE("trwcla");

	if (weight == (double *) 0)
	{
		eps = small;
	}
	else
	{
		eps = 0.0;
		for (i = 0;i < ndata ;i++)
		{
			eps += weight[i];
		}
		eps = small*eps/ (double) ndata;
	}
	
	twoeps = 2.0*eps;
	p1 = 1000.0*eps;

	if (Rescale)
	{
		double    wn, swn, bar, sn;
	
		for (j = 0; j < nvar; j++)
		{
			bar = Qdata(0, j);
			swn = Weight(0);
			sn = 0.0;
			for (i = 1; i < ndata; i++)
			{
				wn = Weight(i);
				d = Qdata(i,j) - bar;
				r = wn/(swn + wn);
				bar += r * d;
				sn += swn * r * d * d;
				swn += wn;
			} /*for (i = 1; i < ndata; i++)*/
			scale[j] = (sn > 0.0) ? sqrt(sn/((double) ndata - 1.0)) : 1.0;
		} /*for (j = 0; j < nvar; j++)*/
	} /*if (Rescale)*/
	else
	{
		for (j = 0; j < nvar; j++)
		{
			scale[j] = 1.0;
		}
	}
	
	if (Report)
	{
		putOutMsg("Cluster analysis by reallocation of objects using Trace W criterion");
		if (Rescale)
		{
			putOutMsg("Variables are standardized before clustering");
		}
		if (Weighted)
		{
			putOutMsg("Cases are weighted by supplied weights");
		}
		sprintf(OUTSTR, "Initial allocation is %s", Label[StartMethod]);
		putOUTSTR();
	} /*if (Report)*/
	
	/* 	do initial allocation */

	for (k = 0;k < kmax ;k++)
	{
		fn[k] = eps;
	} /*for (k = 0;k < kmax ;k++)*/

	if (StartMethod == CANDIDATEMEANS)
	{
		for (i = 0; i < ndata; i++)
		{
			dmin = big;
			for (k=0;k < kmax ;k++)
			{
				ss = 0.0;
				for (j = 0; j < nvar; j++)
				{
					d = (Qdata(i,j) - Qave(k,j))/scale[j];
					ss += d*d;
				}
				
				if (ss < dmin)
				{
					dmin = ss;
					icl[i] = k;
				}
			} /*for (k=0;k < kmax ;k++)*/
		} /*for (i = 0; i < ndata; i++)*/
	} /*if (StartMethod == CANDIDATEMEANS)*/
	else
	{
		for (j = 0;j < nvar ;j++)
		{
			for (k = 0;k < kmax ;k++)
			{
				Qave(k,j) = 0.0;
			}
		} /*for (j = 0;j < nvar ;j++)*/
	
		if (StartMethod == OPTIMUMSTART)
		{
			for (k = 0;k < kmax ;k++)
			{
				icl[k] = k;
				Qave(k,0) = Qdata(k,0);
			}
			low = kmax;
			last = MAX(low,MIN(ndata,kmax*isearch)-1);
		
			for (i = low;i <=last ;i++)
			{
				tdm = big;
				distmn = big;
				for (k=1;k < kmax ;k++)
				{
					for (k1 = 0;k1 < k ;k1++)
					{
						d = (Qave(k,0) - Qave(k1,0));
						d *= d;
						if (d < distmn)
						{
							distmn = d;
							isto = (duni() > .5) ? k1 : k;
						}
					} /*for (k1 = 0;k1 < k ;k1++)*/
				} /*for (k=1;k < kmax ;k++)*/

				for (k=0;k < kmax ;k++)
				{
					d = Qdata(i,0) - Qave(k,0);
					d *= d;
					tdm = MIN(tdm,d);
				}

				if (tdm > distmn)
				{
					Qave(isto,0) = Qdata(i,0);
				}
			} /*for (i=low;i < last ;i++)*/

			for (i=low;i < ndata ;i++)
			{
				dmin = big;
				for (k = 0;k < kmax ;k++)
				{
					d = Qdata(i,0) - Qave(k,0);
					d *= d;
					if (d < dmin)
					{
						dmin = d;
						icl[i] = k;
					}
				} /*for (k = 0;k < kmax ;k++)*/
			} /*for (i=low;i < ndata ;i++)*/
		} /*if (StartMethod == OPTIMUMSTART)*/
		else if (StartMethod == RANDOMSTART)
		{
			nn = (ndata+1)/kmax;
			iwhat = 0;
			icl[0] = iwhat;
			for (i=1;i < ndata ;i++)
			{
				if (i % nn == 0)
				{
					iwhat++;
				}
				i1 = iuni() % (i+1);
				icl[i] = icl[i1];
				icl[i1] = iwhat;
			} /*for (i=1;i < ndata ;i++)*/
		} /*if (StartMethod == OPTIMUMSTART){}else if (StartMethod == RANDOMSTART){} */
		else
		{
			for (i = 0; i < ndata; i++)
			{
				icl[i] = classes[i] - 1;
			} /*for (i = 0; i < ndata; i++)*/
		}
	} /*if (StartMethod == CANDIDATEMEANS){}else{}*/

/*
   At this point we have an initiali clustering with kmax clusters in icl[]
*/

/*
   Compute mean vectors
*/
	for (i=0;i < ndata ;i++)
	{
		icl[i] = MAX(0,MIN(kmax-1,icl[i]));
		igp = icl[i];
		w = Weight(i);
		fn[igp] += w;
		t = fn[igp];
		r = w/t;
		for (j=0;j < nvar ;j++)
		{
			Qave(igp,j) += (Qdata(i,j) - Qave(igp,j))*r;
		}
	} /*for (i=0;i < ndata ;i++)*/

	noc = 0;
	for (k = 0;k < kmax ;k++)
	{
		if (fn[k] > p1)
		{
			noc++;
		}
	} /*for (k = 0;k < kmax ;k++)*/

	if (noc*2 <= kmax)
	{
		sprintf(OUTSTR,
				"ERROR: %s() finds more than half classes are empty; cannot go on",
				FUNCNAME);
		goto errorExit;
	} /*if (noc*2 <= kmax)*/
	
	crit = 0.0;
	for (j = 0; j < nvar; j++)
	{
		s = (Rescale) ? scale[j]*scale[j] : 1.0;
		for (i = 0;i < ndata ;i++)
		{
			d = Qdata(i,j) - Qave(icl[i],j);
			crit += d*d*Weight(i)/s;
		} /*for (i = 0;i < ndata ;i++)*/
	} /*for (j = 0;j < nvar ;j++)*/

	/* 	loop */
	for (ncl = kmax;ncl >= kmin; ncl--)
	{
		if (ncl > 1)
		{
			if (Report)
			{
				outstr = OUTSTR;
				strcpy(outstr, "  k");
				outstr += strlen(outstr);
				outstr += formatChar(outstr, "Initial", DODEFAULT);
				outstr += formatChar(outstr, "Final", DODEFAULT);
				sprintf(outstr, " Reallocations");
				putOUTSTR();
			} /*if (Report)*/
		
#if (0)
			eps1 = 0.5*(1 + eps1);
			eps2 = 0.5*(1 + eps2);
#endif /*0*/
			do
			{
				/* 	reallocate loop */
				ocrit = crit;
				ialloc = 0;
				for (i = 0;i < ndata ;i++)
				{
					icd = icl[i];
					w = Weight(i);
					if (fn[icd] >= w + twoeps)
					{
						/* 	find nearest cluster */
						cmin = big;
						rs = fn[icd]/(fn[icd] - w);
						for (igp = 0;igp < ncl ;igp++)
						{
							r = rs;
							r = (igp == icd) ? rs : fn[igp]/(fn[igp] + w);
							ss = 0.0;

							for (j = 0;j < nvar ;j++)
							{
								d = (Qdata(i,j) - Qave(igp,j))/scale[j];
								ss += d*d;
							}

							ss *= r;
							if (igp == icd)
							{
								dsto = ss;
							}
							if (ss < cmin)
							{
								cmin = ss;
								nearest = igp;
							}
						} /*for (igp = 0;igp < ncl ;igp++)*/

/*
   nearest is number of nearest cluster, cmin is distance to cluster nearest
   and dsto is distance from current cluster to which case i belongs
*/
#ifdef DJGPP
	/*
		Without the following, macanodj doesn't work right
		Probably an optimization problem since it goes away
		when compiled without -O and without the fprintf()
	*/
						fprintf(stdout,"");
#endif /*DJGPP*/
						if (cmin < eps1*dsto)
						{
							double        change = 0.0;
							
							/* 	reallocate */
							igp = icd;
							icd = nearest;
							icl[i] = nearest;
							fn[nearest] += w;
							fn[igp] -= w;
							ri = fn[igp]/(fn[igp] + w);
							r2n = fn[nearest]/(fn[nearest] - w);
							for (j = 0;j < nvar ;j++)
							{
								d1 = Qdata(i,j) - Qave(igp,j);
								d2 = Qdata(i,j) - Qave(nearest,j);
								change +=
									w*(d2*d2/r2n - d1*d1/ri)/(scale[j]*scale[j]);
								t = Qave(igp,j);
								Qave(igp,j) = (fn[igp] > p1) ?
									t - (d1/fn[igp])*w : 0.0;
								Qave(nearest,j) += (d2/fn[nearest])*w;
							} /*for (j = 0;j < nvar ;j++)*/
							crit += change;
							ialloc++;
						} /*if (cmin < eps1*dsto) */
					} /*if (fn[icd] >= w + twoeps) */
				} /*for (i = 0;i < ndata ;i++)*/

				if (Report)
				{
					outstr = OUTSTR;
					sprintf(outstr, "%3ld", ncl);
					outstr += strlen(outstr);
					outstr += formatDouble(outstr, ocrit, DODEFAULT);
					outstr += formatDouble(outstr, crit, DODEFAULT);
					sprintf(outstr, "%8ld", ialloc);
					putOUTSTR();
				} /*if (Report)*/
			} while (crit > 0.0 && crit <= eps2*ocrit && ialloc > 0);
			
			for (i = 0;i < ndata ;i++)
			{
				classes[i] = (double) (icl[i] + 1);
			} /*for (i = 0;i < ndata ;i++)*/
			classes += ndata;
			*criterion++ = crit;
		
			if (ncl > kmin)
			{
				cmin = big;
				for (k = 0;k < ncl - 1 ;k++)
				{
					for (k1 = k + 1;k1 < ncl ;k1++)
					{
						ss = 0.0;
						for (j = 0;j < nvar ;j++)
						{
							d = (Qave(k1,j) - Qave(k,j))/scale[j];
							ss += fn[k]*fn[k1]/(fn[k] + fn[k1])*d*d;
						} /*for (j = 0;j < nvar ;j++)*/
						if (ss < cmin)
						{
							cmin = ss;
							jsto = k;
							ksto = k1;
						}
					} /*for (k1 = k + 1;k1 < ncl ;k1++)*/
				} /*for (k = 0;k < ncl - 1 ;k++)*/
				crit += cmin;

				if (Report)
				{
					char    *outstr = OUTSTR;
				
					sprintf(outstr,
							"Merging clusters %ld and %ld; criterion = ", jsto+1, ksto+1);
					outstr += strlen(outstr);
					outstr += formatDouble(outstr, crit, TRIMLEFT);
					putOUTSTR();
				
				} /*if (Report)*/
			
				if (ncl > 2)
				{
					/* update mean vectors */
					for (j = 0;j < nvar ;j++)
					{
						Qave(jsto,j) = (fn[jsto]*Qave(jsto,j) + fn[ksto]*Qave(ksto,j))/
							(fn[jsto] + fn[ksto]);
						for (k = ksto + 1;k < ncl ;k++)
						{
							Qave(k-1,j) = Qave(k,j);
						}
					} /*for (j = 0;j < nvar ;j++)*/

					fn[jsto] += fn[ksto];
					for (k = ksto + 1;k < ncl ;k++)
					{
						fn[k-1] = fn[k];
					} /*for (k = ksto + 1;k < ncl ;k++)*/

					for (i = 0;i < ndata ;i++)
					{
						if (icl[i] >= ksto)
						{
							if (icl[i] == ksto)
							{
								icl[i] = jsto;
							}
							else
							{
								icl[i]--;
							}
						} /*if (icl[i] >= ksto)*/
					} /*for (i = 0;i < ndata ;i++)*/
				} /*if (ncl > 2)*/
			} /*if (ncl > kmin)*/
		} /*if (ncl > 1)*/
		else
		{
			for (i = 0;i < ndata ;i++)
			{
				classes[i] = 1.0;
			} /*for (i = 0;i < ndata ;i++)*/
			*criterion++ = crit;
		}
	} /*for (ncl = kmax;ncl >= kmin && ncl > 1; ncl--)*/
	return (1);

 errorExit:
	return (0);
} /*trwcla()*/

/*
   Return 1 if the rows of ndata by nvar matrix ave are distinct
   Return 0 if any rows are identical
*/

static int distinctMeans(double ave [], long ndata, long nvar)
{
	double       *xi, *xj;
	long          i, j, k;
	long          maxk = ndata*nvar;
	
	xi = ave + 1;
	for (i = 1; i < ndata; i++, xi++)
	{
		xj = ave;
		for (j = 0; j < i; j++, xj++)
		{
			for (k = 0; k < maxk; k += ndata)
			{
				if (xi[k] != xj[k])
				{
					break;
				}
			} /*for (k = 0; k < maxk; k += ndata)*/
			if (k == maxk)
			{
				return (0);
			}
		} /*for (j = 0; j < i; j++, xj++)*/
	} /*for (i = 1; i < ndata; i++, xi++)*/
	return (1);
} /*distinctMeans()*/

/*
   return the actual number of non-empty clusters represented in classes
   newclasses[k] is set to the number of non-empty classes <= k+1
*/
static long nonEmptyClusters(double classes [], long ndata, long maxk,
							 unsigned char newclasses [])
{
	long             i, k, nclasses = 0;
	
	for (k = 0; k < maxk; k++)
	{
		newclasses[k] = 0;
	}

	for (i = 0; i < ndata; i++)
	{
		newclasses[(long) classes[i] - 1] = 1;
	}
	
	for (k = 0; k < maxk; k++)
	{
		nclasses += (long) newclasses[k];
		newclasses[k] = (unsigned char) nclasses;
	}
	return (nclasses);
} /*nonEmptyClusters()*/

/*
  Function to do k-means clustering, using code provided by Douglas Hawkins

  results <- kmeans(y,kmax:5,kmin:3,start:"random" [,quiet:T])

  results will be a structure with components 'classes' (a n by
   (kmax-kmin+1) matrix whose columns give the cluster allocation),
   and 'criterion' (a vector of length kmax-kmin+1 containing the criterion)
   If kmin is omitted, kmin is assumed the same as kmax.
   If quiet:T is not present, the value of the criterion and merge history is
   printed.

  Similarly
  results <- kmeans(y,kmax:5,kmin:2,start:"optimal")

  results <- kmeans(y,Means,kmax:5,kmin:2,start:"means")
   where Means is a matrix of trial cluster means and the initial clusters
   are cases nearest to each trial mean

  results <- kmeans(y,Classes,kmax:5,kmin:2,start:"classes")
 	where Classes is a n-vector of integers between 1 and kmax

  If keyword 'start' is not used, start:"random" will be assumed.

  By default, distances and criteria are computed using standardized data.
  This can be suppressed by standard:F

  961220 First installed
  961227 Added checks for identical starting candidate means, and empty
         initial cluster membership
*/	
enum kmeansScratch
{
	GWORKING = 0,
	GICL,
	NTRASH
};

#define MAXK    255  /* maximum number of starting clusters */

Symbolhandle kmeans(Symbolhandle list)
{
	Symbolhandle            symhData, symhIcl = (Symbolhandle) 0;
	Symbolhandle            symhMeans = (Symbolhandle) 0;
	Symbolhandle            symhClasses = (Symbolhandle) 0;
	Symbolhandle            symhCrit = (Symbolhandle) 0;
	Symbolhandle            symhKey, symh;
	Symbolhandle            result = (Symbolhandle) 0;
	double                **weightsH = (double **) 0;
	long                  **iclH = (long **) 0;
	double                **workingH = (double **) 0;
	double                 *vals, *vals1;
	long                    nKeys = NKeys(KmeansKeys);
	long                    nargs = (long) NARGS(list);
	long                    keyStart, ikey;
	long                    i, reply;
	long                    dims[2];
	long                    nrows, ncols, kmax, kmax1;
	long                    op = 0;
	char                   *keyword;
	unsigned char           work[MAXK];
	WHERE("kmeans");
	TRASH(NTRASH, errorExit);

	unsetKeyValues(KmeansKeys, nKeys);
	
	if (nargs < 2)
	{
		badNargs(FUNCNAME, -1002);
		goto errorExit;
	}
	
	symhData = COMPVALUE(list, 0);
	if (!argOK(symhData, 0, 1))
	{
		goto errorExit;
	}

	keyStart = (isKeyword(COMPVALUE(list, 1))) ? 1 : 2;
	if (TYPE(symhData) != REAL || !isMatrix(symhData, dims) ||
		dims[0] < 2)
	{
		sprintf(OUTSTR,
				"ERROR: argument 1 to %s() is not REAL matrix with at least two rows",
				FUNCNAME);
	}
	else if (anyMissing(symhData))
	{
		sprintf(OUTSTR,
				"ERROR: argument 1 to %s() has MISSING data", FUNCNAME);
	}
	else if(nargs == 2 && keyStart == 2)
	{
		sprintf(OUTSTR,
				"ERROR: %s() requires at least 1 keyword argument", FUNCNAME);
	}
	if (*OUTSTR)
	{
		goto errorExit;
	}

	nrows = dims[0];
	ncols = dims[1];

	for (ikey = keyStart; ikey < nargs; ikey++)
	{
		symhKey = COMPVALUE(list, ikey);
		keyword = isKeyword(symhKey);
		if (keyword == (char *) 0)
		{
			sprintf(OUTSTR,
					"ERROR: all arguments to %s() after arg %ld must be keywords",
					FUNCNAME, keyStart);
			goto errorExit;
		}
		
		reply = getOneKeyValue(symhKey, 0, KmeansKeys, nKeys);
		if (reply <= 0)
		{
			if (reply < 0)
			{
				badKeyword(FUNCNAME, keyword);
			}
			goto errorExit;
		} /*if (reply <= 0)*/
	} /*for (ikey = keyStart; ikey < nargs; ikey++)*/
	
	if (!charIsSet(Start))
	{
		op = RANDOMSTART;
	} /*if (!charIsSet(Start))*/
	else
	{
		for (op = 0; op < NSTARTMETHODS; op++)
		{
			if (strncmp(*Start, StartValues[op], strlen(StartValues[op])) == 0)
			{
				break;
			}
		} /*for (op = 0; op < NSTARTMETHODS; op++)*/

		if (op == NSTARTMETHODS)
		{
			sprintf(OUTSTR,
					"ERROR: illegal value \"%s\" for keyword %s",
					*Start, KeyName(KmeansKeys, KSTART));
			goto errorExit;
		}
	} /*if (!charIsSet(Start)){}else{}*/

	if (op == PREDEFINEDSTART || op == CANDIDATEMEANS)
	{ /*arg 2 should be matrix of means or vector of class membership*/
		if (keyStart == 1)
		{
			sprintf(OUTSTR,
					"ERROR: argument 2 must not be keyword with %s:%s",
					KeyName(KmeansKeys, KSTART), StartValues[op]);
			goto errorExit;
		} /*if (keyStart == 1)*/
		
		symh = COMPVALUE(list, 1);
		if (!argOK(symh, 0, 2))
		{
			goto errorExit;
		}

		if (TYPE(symh) != REAL || anyMissing(symh))
		{
			sprintf(OUTSTR,
					"ERROR: argument 2 to %s() not REAL with no MISSING values",
					FUNCNAME);
			goto errorExit;
		} /*if (TYPE(symh) != REAL || anyMissing(symh))*/

		if (op == PREDEFINEDSTART)
		{
			symhIcl = symh;
			if (!isVector(symhIcl) || symbolSize(symhIcl) != nrows)
			{
				sprintf(OUTSTR,
						"ERROR: starting class membership not REAL vector of length nrows(data)");
				goto errorExit;
			}
			vals = DATAPTR(symhIcl);
			kmax = 0;
			for (i = 0; i < nrows; i++)
			{
				if (vals[i] <= 0 || vals[i] != floor(vals[i]))
				{
					sprintf(OUTSTR,
							"ERROR: element of starting class membership not a positive integer");
					goto errorExit;
				}
				if (vals[i] > kmax)
				{
					kmax = (long) vals[i];
				}
			} /*for (i = 0; i < nrows; i++)*/

			if (longIsSet(Kmax) && kmax != Kmax)
			{
				putOutErrorMsg("WARNING: mismatch between 'kmax' and maximum starting class");
			}
			Kmax = kmax;
			if (Kmax > MAXK)
			{
				sprintf(OUTSTR,
						"ERROR: max(classes) > %ld in %s()",
						(long) MAXK, FUNCNAME);
				goto errorExit;
			}
			kmax1 = nonEmptyClusters(vals, nrows, Kmax, work);
			if (kmax1 < Kmax)
			{
				sprintf(OUTSTR,
						"WARNING: %ld starting clusters are empty; kmax reduced to %ld",
						Kmax - kmax1, kmax1);
				putErrorOUTSTR();
				Kmax = kmax1;
			}
		} /*if (op == PREDEFINEDSTART)*/
		else
		{ /*CANDIDATEMEANS*/
			symhMeans = symh;
			if (!isMatrix(symhMeans,dims) || dims[1] != ncols)
			{
				sprintf(OUTSTR,
						"ERROR: starting means not REAL matrix with ncols(data) columns");
				goto errorExit;
			}
			if (longIsSet(Kmax) && dims[0] != Kmax)
			{
				putOutErrorMsg("WARNING: mismatch between 'kmax' and number of starting means");
			}
			Kmax = dims[0];
			if (!distinctMeans(DATAPTR(symhMeans), Kmax, ncols))
			{
				putOutErrorMsg("WARNING: duplicate rows in starting matrix of means");
			}
		} /*if (op == PREDEFINEDSTART){}else{}*/
	} /*if (op == PREDEFINEDSTART || op == CANDIDATEMEANS)*/
	else if (!longIsSet(Kmax))
	{
		sprintf(OUTSTR,
				"ERROR: kmax:m required except with %s:\"%s\" or %s:\"%s\"",
				KeyName(KmeansKeys,KSTART),StartValues[PREDEFINEDSTART],
				KeyName(KmeansKeys,KSTART),StartValues[CANDIDATEMEANS]);
		goto errorExit;
	}
	
	if (!longIsSet(Kmin))
	{
		Kmin = Kmax;
	}
	else if (Kmin > Kmax)
	{
		sprintf(OUTSTR,
				"ERROR: kmin > kmax or kmin > starting number of classes");
		goto errorExit;
	}

	if (symhIsSet(Weights))
	{
		weightsH = DATA(Weights);
	}
	
	result = StrucInstall(SCRATCH, 2);
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	COMPVALUE(result, 0) = symhClasses = Makereal(nrows*(Kmax-Kmin+1));
	if (symhClasses == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	setNAME(symhClasses, "classes");

	if (Kmax != Kmin)
	{
		setNDIMS(symhClasses, 2);
		setDIM(symhClasses, 1, nrows);
		setDIM(symhClasses, 2, Kmax-Kmin+1);
	}
	if (symhIcl != (Symbolhandle) 0)
	{
		vals = DATAPTR(symhIcl);
		vals1 = DATAPTR(symhClasses);
		if (kmax == kmax1)
		{
			doubleCopy(vals, vals1, nrows);
		}
		else
		{
			/* renumber classes */
			for (i = 0; i < nrows; i++)
			{
				vals1[i] = (double) work[(int) vals[i] - 1];
			} /*for (i = 0; i < nrows; i++)*/
		}
	} /*if (symhIcl != (Symbolhandle) 0)*/
	
	COMPVALUE(result, 1) = symhCrit = Makereal(Kmax-Kmin+1);
	if (symhCrit == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	setNAME(symhCrit, "criterion");

	if (!getScratch(workingH, GWORKING, Kmax*ncols + Kmax + ncols, double) ||
		!getScratch(iclH, GICL, nrows, long))
	{
		goto errorExit;
	}

	if (symhMeans != (Symbolhandle) 0)
	{
		doubleCopy(DATAPTR(symhMeans), *workingH, Kmax*ncols);
	}
	
	
	Quiet = (logicalIsSet(Quiet)) ? Quiet : 0;
	Standardize = (logicalIsSet(Standardize)) ? Standardize : 1;

	if ((op == RANDOMSTART || op == OPTIMUMSTART) && 
		(Rands1 == 0 || Rands2 == 0))
	{
		randomSeed(!Quiet);
	}

	op |= (!Quiet) ? VERBOSE : 0;
	op |= (Standardize) ? STANDARDIZE : 0;

	if (!trwcla(DATAPTR(symhData),
				(weightsH != (double **) 0) ? *weightsH : (double *) 0,
				DATAPTR(symhClasses), DATAPTR(symhCrit), *iclH,
				Kmax, Kmin, ncols, nrows, op,  *workingH))
	{
		goto errorExit;
	}
	emptyTrash();
	return (result);

  errorExit:
	putErrorOUTSTR();
	emptyTrash();
	Removesymbol(result);
	return (0);
	
} /*kmeans()*/
