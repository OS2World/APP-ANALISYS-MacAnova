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
#pragma segment Cluster
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


/* based on Fortran code from statlib */

/*
   960918  Fixed bug associated with similar:s
           Changed many defines to enums
   971205  Changed spelling Euclidian to Euclidean

   990215  Changed myerrorout() to putOutMsg()
   990226  Changed two instances of putOUTSTR() to putErrorOUTSTR()
*/
#include "globals.h"
#include "keywords.h"

#ifndef HUGEDBL
#define HUGEDBL 1e38
#endif /*HUGEDBL*/

#ifndef MAXNCLUSTERS
#define MAXNCLUSTERS   50 /* maximum value for 'nclusters' */
#endif /*MAXNCLUSTERS*/

#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

#define Q2data(I,J) data[(I) + (J)*n]
#define Q2iclass(I,J) iclass[(I) + (J)*n]
#define LEV 9 /* default number of clusters */

enum clusterKeycodes
{
	KDISSIM = 0,
	KSIMILAR,
	KMETHOD,
	KDISTANCE,
	KNCLUSTERS,
	KPRINT,
	KCLASSES,
	KREORDER,
	KTREE,
	KSTANDARD,
	KKEEP
};

keywordList         ClusterKeys[] =
{
	InitKeyEntry("dissim",4,0,REALSQUAREMATRIX),
	InitKeyEntry("similar",3,0,REALSQUAREMATRIX),
	InitKeyEntry("method",4,0,CHARSCALAR),
	InitKeyEntry("distance",4,0,CHARSCALAR),
	InitKeyEntry("nclusters",6,0,POSITIVEINT),
	InitKeyEntry("print",0,0,LOGICSCALAR),
	InitKeyEntry("classes",5,0,LOGICSCALAR),
	InitKeyEntry("reorder",0,0,LOGICSCALAR),
	InitKeyEntry("tree",0,0,LOGICSCALAR),
	InitKeyEntry("standard",5,0,LOGICSCALAR),
	InitKeyEntry("keep",0,0,CHARVECTOR)
};

#define DissimKey       KeySymhValue(ClusterKeys,KDISSIM)
#define SimilarKey      KeySymhValue(ClusterKeys,KSIMILAR)
#define MethodKey       KeyCharValue(ClusterKeys,KMETHOD)
#define DistanceKey     KeyCharValue(ClusterKeys,KDISTANCE)
#define NclustersKey    KeyIntValue(ClusterKeys,KNCLUSTERS)
#define PrintKey        KeyLogValue(ClusterKeys,KPRINT)
#define ClassesKey      KeyLogValue(ClusterKeys,KCLASSES)
#define ReorderKey      KeyLogValue(ClusterKeys,KREORDER)
#define TreeKey         KeyLogValue(ClusterKeys,KTREE)
#define StandardKey     KeyLogValue(ClusterKeys,KSTANDARD)
#define KeepKey         KeySymhValue(ClusterKeys,KKEEP)

/* note: following codes - 1 are used as ndex to MethodNames */
enum methodCodes
{
	WARD = 1,
	SINGLE,
	COMPLETE,
	AVERAGE,
	MCQUITTY,
	MEDIAN,
	CENTROID
};

static char        *Methods[] = 
{
	"ward", "single", "complete", "average", "mcquitty", "median",
	"centroid", (char *) NULL
};
static long         MethodCodes[] = 
{
	WARD,SINGLE,COMPLETE,AVERAGE,MCQUITTY,MEDIAN,CENTROID, 0
};

static char        *MethodNames[] =
{
	"Ward's minimum variance",
	"Single linkage",
	"Complete linkage",
	"Average linkage",
	"McQuitty's",
	"Median (Gower's)",
	"Centroid"
};

enum distanceCodes
{
	EUCLID = 1,
	EUCLIDSQ,
	DISSIM,
	SIMILAR
};

static char          *DisMethods[] =
{
	"euclid", "euclidsq",(char *) 0
};

static long           DisMethodCodes[] =
{
	EUCLID, EUCLIDSQ, 0
};

static char          *DisMethodNames[] =
{
	"Euclidean", "Squared Euclidean", "Input dissimilarity matrix",
	"Input similarity matrix"
};

static char          *MethodName;
static char          *DisMethodName;


static long ioffset(long i, long j) /*was ioffset(n,i,j) */
{
/*
   map row i and column j of upper half diagonal symmetric matrix 
   onto vector. 
*/

	return ((j*(j-1))/2 + i);
/*	return (j+i*n-((i+1)*(i+2))/2); */

}

/*
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

   Hierarchical clustering using (user-specified) criterion.

   Parameters:

   data(n,m)         input data matrix,
   diss(len)         dissimilarities in lower half diagonal
                     storage;len = n.n-1/2,
   iopt              clustering criterion to be used,
   ia, ib, crit      history of agglomerations;dimensions
                     n, first n-1 locations only used,
                     (ia[i],ib[i]) contain the indices of the clusters
                     that were merged at step i, that is when there 
                     were n - i clusters
                     crit[i] contains the value of the criterion at that
                     stage
   membr, nn, disnn  vectors of length n, used to store
                     cluster cardinalities, current nearest
                     neighbour, and the dissimilarity assoc.
                     with the latter.
   flag              boolean indicator of agglomerable obj./
                     clusters.

   F. Murtagh, ESA/ESO/STECF, Garching, February 1986.

   (translated from Fortran by C. Bingham, kb@umnstat.stat.umn.edu)
 ------------------------------------------------------------
*/

/*
  kb added argument distopt
  distopt     Dissimilarity
  0 or 1      Euclidean distance
    2         Euclidean squared distance
   -1         m == n and upper triangle of data is dissimilarity matrix
   -2         m == n and upper triangle of data is similarity matrix
  If ddist != (double *) 0, it is assumed to be a n by n matrix and is
  filled with the distances.
*/

/*
   Distance between groups with data x1[k1], k1 = 1,n1 and x2[k2],k=1,n2,
   means xbar1 and xbar2, and "centers" C1 and C2.  Let C be "center" of
   combined group

   Single       d(1,2) = min{|x1[k1] - x2[k2]|^2,k1=1,n1;k2=1,n2}
   Complete     d(1,2) = max{|x1[k1] - x2[k2]|^2,k1=1,n1;k2=1,n2}
   Average      d(1,2) = average{|x1[k1] - x2[k2]|^2,k1=1,n1;k2=1,n2}
   Ward			C1 = xbar1, C2 = xbar2, C = (n1*C1+n2*C2)/(n1+n2)
                d(1,2) = (n1*n2/(n1+n2))|C1-C2|^2
   Centroid     C1 = xbar1, C2 = xbar2, C = (n1*C1+n2*C2)/(n1+n2)
                d(1,2) = |C1-C2|^2
   Median       C = C1 + C2
                d(1,2) = |C1-C2|^2
				McQuitty  No clear geometry, d(1,{2,3}) = (d(1,2)+d(1,3))/2
				
*/
static void hc(long n,long m,long iopt, long distopt,
			   double data[],double ddist[], double stdev[],
			   long ia[], long ib[], double crit[], double membr[],
			   long nn[],double disnn[],long flag[],double diss[])
/*
  double data[n][m],membr[n],diss[len];
  long ia[n],ib[n];
  double crit[n],disnn[n];
  int nn[n];
  int flag[n];
*/
{
	long i, j, k, jj, ij, ji;
	long ind, ind1, ind2, ind3;
	long ncl, im, jm, i2, j2;
	long standardize = (stdev != (double *) 0);
	double *y;
	double x, d3, value;
	double d, mean, dmin,s, fi;
	double d1, d2;
	double maxsimil = -HUGEDBL;
	double inf = HUGEDBL;
	WHERE("hc");
	
	/*   initializations */

	for (i = 0;i < n ;i++)
	{
		membr[i] = 1.;
		flag[i] =  1 ;
	}
	
	if (distopt == SIMILAR)
	{
		for (i = 0;i < n;i++)
		{
			for (j = i;j < n;j++)
			{
				value = Q2data(i,j);
				if (value > maxsimil)
				{
					maxsimil = value;
				}
			} /*for (j = i;j < n;j++)*/
		} /*for (i = 0;i < n;i++)*/
	} /*if (distopt == SIMILAR)*/
	
	/*   construct dissimilarity matrix */

	if (standardize)
	{
		y = data;
		
		for (j = 0;j < m;j++)
		{
			s = 0;
			mean = *y++;
			fi = 1.0;
			for (i = 1;i < n;i++)
			{
				d = *y++ - mean;
				fi++;
				mean += d/fi;
				s += (1.0 - 1.0/fi)*d*d;
			}
			stdev[j] = (s > 0) ? sqrt(s/(fi-1.0)) : 1.0;
		}
	} /*if (standardize)*/
	
	for (i = 0;i < n-1 ;i++)
	{
		for (j = i+1;j < n ;j++)
		{
			ind = ioffset(i,j);
			s = 0.0;
			switch (distopt)
			{
			  case EUCLID:
			  case EUCLIDSQ:
				for (k = 0;k < m ;k++)
				{ /* compute square Euclidean distance */
					d = Q2data(i,k) - Q2data(j,k);
					if (standardize)
					{
						d /= stdev[k];
					}
					s += d*d;
				} /*for (k = 0;k < m ;k++)*/
				break;

			  case DISSIM:
				s = Q2data(i,j);
				s *= s; /* square distances */
				break;

			  case SIMILAR:
				s = 2*(maxsimil-Q2data(i,j));
				break;
			} /*switch (distopt)*/

			if (iopt==WARD)
			{
				s /= 2.;
				/*
				  (above is done for the case of the min. var. method
				  where merging criteria are defined in terms of variances
				  rather than distances.)
				  */
			} /*if (iopt==WARD)*/
			diss[ind] = s;
		} /*for (j = i+1;j < n ;j++)*/
	} /*for (i = 0;i < n-1 ;i++)*/

	if (ddist)
	{ /* put distances in ddist for saving */
		for (i = 0;i < n;i++)
		{
			ij = i*(n+1); /* i + n*j , starting with j = i */
			ji = ij; /* n*i+j */
			ddist[ij] = 0.0; /* set diagonal to zero */
			for (j = i+1;j < n;j++)
			{
				ji++;
				ij += n;
				value = diss[ioffset(i,j)];
				if (iopt == WARD)
				{
					value *= 2.0;
				}
				ddist[ij] = ddist[ji] = sqrt(value);
			} /*for (j = i+1;j < n;j++)*/
		} /*for (i = 0;i < n;i++)*/
	} /*if (ddist)*/

	/*   carry out an agglomeration - first create list of nns */

	for (i = 0;i < n-1 ;i++)
	{
		dmin = inf;
		for (j = i+1;j < n ;j++)
		{
			ind = ioffset(i,j);
			if (diss[ind]<dmin) 
			{
				dmin = diss[ind];
				jm = j;
			}
		} /*for (j = i+1;j < n ;j++)*/
		nn[i] = jm;
		disnn[i] = dmin;
	} /*for (i = 0;i < n-1 ;i++)*/

	for (ncl = n;ncl > 1;ncl--)
	{
		/*      next, determine least diss. using list of nns */
		dmin = inf;
		for (i = 0;i < n-1;i++)
		{
			if (flag[i] && disnn[i]<dmin)
			{
				dmin = disnn[i];
				im = i;
				jm = nn[i];
			}
		} /*for (i = 0;i < n-1;i++)*/
		if (distopt == EUCLID || distopt == DISSIM)
		{
			dmin = sqrt(dmin);
		}
		else if (distopt == SIMILAR)
		{
			dmin = maxsimil - dmin/2;
		}
		
		/*   this allows an agglomeration to be carried out. */

		i2 = MIN(im,jm);
		j2 = MAX(im,jm);
		ia[n-ncl] = i2;
		ib[n-ncl] = j2;
		crit[n-ncl] = dmin;
		/*   update dissimilarities from new cluster. */

		flag[j2] =  0 ;
		dmin = inf;
		for (k = 0;k < n ;k++)
		{
			if (flag[k] && k != i2)
			{
				x = membr[i2]+membr[j2]+membr[k];
				ind1 = (i2>=k) ? ioffset(k,i2) : ioffset(i2,k);
				ind2 = (j2>=k) ? ioffset(k,j2) : ioffset(j2,k);
				ind3 = ioffset(i2,j2);
				d1 = diss[ind1];
				d2 = diss[ind2];
				d3 = diss[ind3];

				/*   ward's minimum variance method - iopt = 1. */

				switch (iopt)
				{
				  case WARD:
					d1 = (membr[i2]+membr[k])*d1+
						(membr[j2]+membr[k])*d2-membr[k]*d3;
					d1 /= x;
					break;
					
				/*   single link method - iopt = 2. */
				  case SINGLE:
					d1 = MIN(d1,d2);
					break;
					
				/*   complete link method - iopt = 3. */
				  case COMPLETE:
					d1 = MAX(d1,d2);
					break;
					
				/*   average link (or group average) method - iopt = 4. */
				  case AVERAGE:
					d1 = (membr[i2]*d1+membr[j2]*d2)/
						(membr[i2]+membr[j2]);
					break;
					
				/*   Mcquitty's method - iopt = 5. */
				  case MCQUITTY:
					d1 = 0.5*(d1+d2);
					break;
					
				/*   median (Gower's) method - iopt = 6. */
				  case MEDIAN:
					d1 = 0.5*(d1+d2)-0.25*d3;
					break;
					
				/*   centroid method - iopt = 7. */
				  case CENTROID:
					d1 = 
						(membr[i2]*d1+membr[j2]*d2-
						 membr[i2]*membr[j2]*d3/(membr[i2]+membr[j2]))/
							(membr[i2]+membr[j2]);
					break;
				} /*switch (iopt)*/
				
				if (i2<=k && d1 < dmin)
				{
					dmin = d1;
					jj = k;
				}
				diss[ind1] = d1;
			} /*if (flag[k] && k != i2)*/
		} /*for (k = 0;k < n ;k++)*/
		membr[i2] = membr[i2]+membr[j2];
		disnn[i2] = dmin;
		nn[i2] = jj;

		/*   update list of nns insofar as this is required. */

		for (i = 0;i < n-1 ;i++)
		{
			if (flag[i] && (nn[i] == i2 || nn[i] == j2))
			{
				/*         (redetermine nn of i:) */
				dmin = inf;
				for (j = i+1;j < n ;j++)
				{
					ind = ioffset(i,j);
					if (flag[j] && i != j && diss[ind]<dmin) 
					{
						dmin = diss[ind];
						jj = j;
					}
				} /*for (j = i+1;j < n ;j++)*/
				nn[i] = jj;
				
				disnn[i] = dmin;
			} /*if (flag[i] && (nn[i] == i2 || nn[i] == j2))*/
		} /*for (i = 0;i < n-1 ;i++)*/
		if (interrupted(DEFAULTTICKS) != INTNOTSET)
		{
			break;
		}
	} /*for (ncl = n;ncl > 1;ncl--)*/
} /*hc() */

/*
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

   Given a hierarchic clustering, described as a sequence of
   agglomerations, derive the assignments into clusters for the
   top lev-1 levels of the hierarchy.
   Prepare also the required data for representing the
   dendrogram of this top part of the hierarchy.

   parameters:

   ia, ib, crit: vectors of dimension n defining the agglomer-
                  ations.
   lev:          number of clusters in largest partition.
   hvals:        vector of dim. lev, used internally only.
   iclass:       array of cluster assignments;dim. n by lev.
   iorder, critval, height: 
                 vectors describing the dendrogram, all of dim. lev.

   F. Murtagh, ESA/ESO/STECF, Garching, February 1986.

   (translated with fix from Fortran by C. Bingham, kb@umnstat.stat.umn.edu)
 ---------------------------------------------------------------
*/

static void hcass(long n,long ia[],long ib[],double crit[],long 
				  lev,long iclass[],long hvals[], long iorder[],
				  double critval[], long height[], long printit,
				  long reorderit)
/*
int ia[n],ib[n],iclass[n][lev],hvals[lev],iorder[lev],height[lev];
double crit[n],critval[lev];
*/
{
	long         i, j, k;
	long         loc, level;
	long         icl, ilev, ncl, ival;
	char        *outstr;
	WHERE("hcass");
	
/*
  Pick out the clusters which the n objects belong to, at levels n-2, n-3,
  ... n-lev+1 of the hierarchy.  The clusters are identified by the lowest
  seq. no. of their members.  There are 2, 3, ... lev clusters, respectively,
  for the above levels of the hierarchy.
*/

	checkInterrupt(errorExit);
	hvals[0] = 0; /* always entire cluster has id 0 */
	hvals[1] = ib[n-2]; /* merged with 0 at last step */
	loc = 2;
	for (i = n-3;i>=n-lev;i-- )
	{
		for (j = 0;j<=loc-1 ;j++)
		{
			if (ia[i]==hvals[j])
			{
				break;
			}
		} /*for (j = 0;j<=loc-1 ;j++)*/

		if (j == loc)
		{ /* lowest cluster id that disappeared at this level */
			assert(loc < lev && loc >= 0);			
			hvals[loc] = ia[i];
			loc++;
		} /*if (j == loc)*/

		/* now look to see if the other part of this cluster disappeared */
		for (j = 0;j<=loc-1 ;j++)
		{
			if (ib[i]==hvals[j])
			{
				break;
			}
		} /*for (j = 0;j<=loc-1 ;j++)*/

		if (j == loc)
		{ /* yes, add to list of clusters */
			hvals[loc] = ib[i];
			loc++;
		}
	} /*for (i = n-3;i>=n-lev;i-- )*/

	/* save modified cluster ID numbers in iclass */

	for (level = n-lev;level<=n-2 ;level++)
	{
		for (i = 0;i < n ;i++)
		{
			icl = i;
			for (ilev = 0;ilev < level ;ilev++)
			{
				if (ib[ilev]==icl)
				{
					icl = ia[ilev];
				}
			} /*for (ilev = 0;ilev < level ;ilev++)*/
			ncl = n-level-1;
			Q2iclass(i,ncl-1) = icl;
		} /*for (i = 0;i < n ;i++)*/
	} /*for (level = n-lev;level<=n-2 ;level++)*/

	/* now recode according to index of ID in hvals[] */
	for (i = 0;i < n ;i++)
	{
		for (j = 0;j < lev-1 ;j++)
		{
			ival = Q2iclass(i,j);
			for (k = 1;k < lev ;k++)
			{
				if (ival==hvals[k])
				{
					Q2iclass(i,j) = k;
					break;
				}
			} /*for (k = 1;k < lev ;k++)*/
		} /*for (j = 0;j < lev-1 ;j++)*/
	} /*for (i = 0;i < n ;i++)*/

/*
  Determine an ordering of the lev clusters (at level lev-1) for later
  representation of the dendrogram.  These are stored in iorder.

  Determine the associated ordering of the criterion values for the vertical
  lines in the dendrogram.

  The ordinal values of these criterion values may be used in preference, and
  these are stored in height.

  Finally, note that the lev clusters are renamed so that they have
  seq. nos. 1 to lev.
*/
	iorder[0] = ia[n-2];
	iorder[1] = ib[n-2];
	critval[0] = 0.0;
	critval[1] = crit[n-2];
	height[0] = lev;
	height[1] = lev-1;
	loc = 1;
	for (i = n-3;i>=n-lev;i-- )
	{
		for (j = 0;j<=loc ;j++)
		{
			if (ia[i]==iorder[j]) 
			{
				/* shift rightwards and insert ib[i] beside iorder[j]: */
				for (k = loc+1;k>j+1;k-- )
				{
					iorder[k] = iorder[k-1];
					critval[k] = critval[k-1];
					height[k] = height[k-1];
				} /*for (k = loc+1;k>j+1;k-- )*/
				iorder[j+1] = ib[i];
				critval[j+1] = crit[i];
				height[j+1] = i-(n-lev)+1;
				loc++;
			} /*if (ia[i]==iorder[j]) */
		} /*for (j = 0;j<=loc ;j++)*/
	} /*for (i = n-3;i>=n-lev;i-- )*/
	
/* 
  kb fixed bug here; ia[j] = i was iorder[j] = i, sometimes allowing a
  match to occur for the wrong value of j.

  ia is no longer needed, so it is used as scratch
*/
  
	for (i = 0;i < lev ;i++)
	{
		for (j = 0;j < lev ;j++)
		{
			if (hvals[i]==iorder[j])
			{
				ia[j] = i;
				break;
			}
		} /*for (j = 0;j < lev ;j++)*/
	} /*for (i = 0;i < lev ;i++)*/
	
/* copy back into iorder */
	for (j = 0;j < lev;j++)
	{
		iorder[j] = ia[j];
	}

	if (printit)
	{
		putOutMsg(" Case  Number of Clusters");
		outstr = OUTSTR;
		sprintf(outstr, " No. ");
		for (i = 2;i<=lev;i++)
		{
			outstr += strlen(outstr);
			sprintf(outstr,"%3ld",i);
		}
		putOUTSTR();
	
		checkInterrupt(errorExit);

		outstr = OUTSTR;
		strcpy(outstr, " ----");
		for (i = 2;i<=lev;i++)
		{
			outstr += strlen(outstr);
			strcpy(outstr," --");
		}
		putOUTSTR();
		checkInterrupt(errorExit);

		/*
		  Note, when reorderit == 0, outer loop (k) is left after first
		  circuit and inner loop (j) is traversed for every level of i
		  When reorderit != 0, outer loop (k) is traversed once for each level
		  and the inner loop is graversed only when the last column
		  of the class table matches the cluster number in order[k]
		*/
		for (k = 0;k < lev;k++)
		{
			for (i = 0;i < n;i++)
			{
				if (!reorderit || Q2iclass(i,lev-2) == iorder[k])
				{
					outstr = OUTSTR;
					sprintf(outstr," %3ld ",i+1);
					for (j = 0;j < lev-1;j++)
					{
						outstr += strlen(outstr);
						sprintf(outstr," %2ld",Q2iclass(i,j)+1);
					} /*for (j = 0;j < lev-1;j++)*/
					putOUTSTR();
					checkInterrupt(errorExit);
				} /*if (Q2iclass(i,lev-1) == iorder[k])*/
			} /*for (i = 0;i < n;i++)*/
			if (!reorderit)
			{
				break;
			}
		} /*for (k = 0;k < lev;k++)*/
	} /* if (printit) */

	/* fall through*/
  errorExit:
	*OUTSTR = '\0';
} /*hcass()*/


/*
 +++++++++++++++++++++++++++++++++++++++++++++++++

   Construct a dendrogram of the top 8 levels of a hierarchic clustering.

   Parameters:

   iorder, height, critval: vectors of length lev
           defining the dendrogram.
           These are: the ordering of objects
           along the bottom of the dendrogram
           (iorder);the height of the vertical
           above each object, in ordinal values
           (height);and in real values (critval).

   Note: These vectors must have been set up with
         lev = 9 in the prior call to routine
         hcass. 

   F. Murtagh, ESA/ESO/STECF, Garching, Feb. 1986.

   (translated with mods from Fortran by C. Bingham, kb@umnstat.stat.umn.edu)
 -------------------------------------------------
*/

#define Q2tree(I,J) tree[(J) + (I)*(ncols+1)]

#define ROWINC 1 /* vertical spacing of tops of bars in dendrogram */
#define COLINC 3 /* horizontal spacing of vertical lines in dendrogram */

#define CBLANK  ' '
#define CUP     '|'
#define CACROSS '-'
#define CCORNER '+'

static void hcden(long lev,long iorder[], long height[],double critval[])
/*
int iorder[lev],height[lev];
double critval[lev];
*/
{ 
	long          nrows = ROWINC*lev;
	long          ncols = COLINC*lev;
	char        **treeH, *tree, *outstr;
	long          i, j, i2, ic, idum, l, ii;
	WHERE("hcden");
	TRASH(1, errorExit); /* one scratch handle */
	
	checkInterrupt(errorExit);
	if (!getScratch(treeH,0,nrows*(ncols+1),char))
	{
		goto errorExit;
	}
	
	tree = *treeH;
	for (i = 0;i < nrows ;i++)
	{
		for (j = 0;j < ncols ;j++)
		{
			*tree++ = CBLANK;
		} /*for (j = 0;j < ncols ;j++)*/
		*tree++ = '\0';
	} /*for (i = 0;i < nrows ;i++)*/
	tree = *treeH;
	
	for (l = 0;l < lev;l++)
	{
		i2 = nrows - ROWINC*height[l];
		j = l*COLINC + COLINC-1;
		for (i = nrows-1;i>=i2;i--)
		{
			Q2tree(i,j) = CUP;
		}
		Q2tree(i2,j--) = CCORNER;
		while (j >= COLINC)
		{
			if (nrows-ROWINC*height[j/COLINC] < i2)
			{
				break;
			}
			Q2tree(i2,j) = CACROSS;
			j--;
		}
		if (j >= COLINC-1)
		{
			Q2tree(i2,j) = CCORNER;
		}
	} /*for (l = 0;l < lev;l++)*/

	myeol();
	putOutMsg("    Criterion");
	ic = ROWINC-1;
	ii = 0;
	for (i = 0;i < nrows ;i++)
	{
		outstr = OUTSTR;
		if (i != ic+1)
		{
			sprintf(outstr,MISSINGFMT,"");
		}
		else
		{
			idum = lev - (ic+1)/ROWINC;
			for (l = 0;l < lev ;l++)
			{
				if (height[l] == idum)
				{
					break;
				}
			} /*for (l = 0;l < lev ;l++)*/
			idum = l;
			sprintf(outstr,DATAFMT,critval[idum]);
			ic += ROWINC;
		} /*if (i != ic+1){...}else{...}*/
		outstr += strlen(outstr);
		strcpy(outstr, tree + ii);
		putOUTSTR();
		ii += ncols+1;
		checkInterrupt(errorExit);
	} /*for (i = 0;i < nrows ;i++)*/
	outstr = OUTSTR;
	sprintf(outstr,MISSINGFMT,(FIELDWIDTH >=11) ? "Cluster No." : "Cl #");

	for (l = 0;l < lev;l++)
	{
		outstr += strlen(outstr);
		sprintf(outstr," %2ld",iorder[l]+1);
	} /*for (l = 0;l < lev;l++)*/
	putOUTSTR();

	outstr = OUTSTR;
	sprintf(outstr,MISSINGFMT,"");
	outstr += strlen(outstr);
	sprintf(outstr,"  Clusters 1 to %ld",lev);
	outstr += strlen(outstr);
	sprintf(outstr," (Top %ld levels of hierarchy).",lev-1);
	putOUTSTR();

	outstr = OUTSTR;
	sprintf(outstr,MISSINGFMT,"");
	outstr += strlen(outstr);
	sprintf(outstr,"  Clustering method: %s",MethodName);
	putOUTSTR();

	outstr = OUTSTR;
	sprintf(outstr,MISSINGFMT,"");
	outstr += strlen(outstr);
	sprintf(outstr,"  Distance: %s",DisMethodName);
	putOUTSTR();

	/* fall through */
  errorExit:
	emptyTrash();
} /*hcden()*/
	
enum clusterScratch
{
	GCRIT = 0,
	GMEMBR,
	GCRITVAL,
	GDISNN,
	GD,
	GIA,
	GIB,
	GFLAG,
	GNN,
	GHVALS,
	GIORDER,
	GHEIGHT,
	GICLASS,
	GSTDEV,
	NTRASH
};

Symbolhandle cluster(Symbolhandle list)
{
	Symbolhandle      arg1, arg2;
	Symbolhandle      result = (Symbolhandle) 0, symh;
	double          **dataH = (double **) 0, **critH = (double **) 0;
	double          **membrH = (double **) 0, **critvalH = (double **) 0;
	double          **disnnH = (double **) 0, **dH = (double **) 0;
	double          **ddistH = (double **) 0;
	double          **stdevH = (double **) 0;
	double           *critval, *class;
	long            **iaH = (long **) 0, **ibH = (long **) 0;
	long            **iclassH = (long **) 0, **hvalsH = (long **) 0;
	long            **iorderH = (long **) 0, **heightH = (long **) 0;
	long            **nnH = (long **) 0, **flagH = (long **) 0;
	long             *iclass;
	char             *keyword;
	long              m, n, i;
	long              len, nargs = NARGS(list), ncomps = 0, ncomps1 = 0;
	long              length;
	long              printTree = -1, printClasses = -1, reorderCases = -1;
	long              method = 0;
	long              dismethod = 0, standardize = 1;
	long              savedistance = 0, saveclasses = 0, savecrit = 0;
	long              lev = LEV;
	long              iarg;
	long              dim[2];
	long              keyStatus, ikey, nkeys = NKeys(ClusterKeys);
	keywordListPtr    keys = ClusterKeys;
	WHERE("cluster");
	TRASH(NTRASH,errorExit);
	
	unsetKeyValues(keys, nkeys);

	arg1 = COMPVALUE(list,0);
	if (!(keyword = isKeyword(arg1)))
	{ /* data matrix supplied */
		if (!argOK(arg1,REAL,1))
		{
			goto errorExit;
		}
		if (!isMatrix(arg1,dim) || dim[0] < 2)
		{
			sprintf(OUTSTR,
					"ERROR: first argument to %s must be matrix with at least 2 rows",
					FUNCNAME);
		}
		else if (anyMissing(arg1))
		{
			sprintf(OUTSTR,
					"ERROR: missing values not allowed in %s() data matrix",
					FUNCNAME);
		}
		else
		{	
			dataH = DATA(arg1);
		}		
	} /*if (!(keyword = isKeyword(arg1)))*/
	else
	{ /* distance or similarity matrix should have been supplied */
		keyStatus = getOneKeyValue(arg1, 0, keys, nkeys);
		if (keyStatus == 0)
		{
			goto errorExit;
		}
		
		if (keyStatus < 0 ||
			keyStatus - 1 != KDISSIM && keyStatus - 1 != KSIMILAR)
		{
			sprintf(OUTSTR,
					"ERROR: argument 1 to %s() not data matrix or %s:d or %s:s",
					FUNCNAME, KeyName(keys, KDISSIM), KeyName(keys, KSIMILAR));
		}
	} /*if (!(keyword = isKeyword(arg1))){}else{}*/ 

	if (*OUTSTR)
	{
		goto errorExit;
	}
	
	for (iarg = 1; iarg < nargs; iarg++)
	{
		arg2 = COMPVALUE(list, iarg);
		if (!(keyword = isKeyword(arg2)))
		{
			sprintf(OUTSTR,
					"ERROR: argument %ld to %s() not keyword phrase", iarg+1,
					FUNCNAME);
			goto errorExit;
		}
		
		keyStatus = getOneKeyValue(COMPVALUE(list, iarg), 0, keys, nkeys);
		if (keyStatus == 0)
		{
			goto errorExit;
		}
		else if (keyStatus < 0)
		{
			badKeyword(FUNCNAME, keyword);
			goto errorExit;
		}
	} /*for (iarg = 1; iarg < nargs; iarg++)*/
	
	if (symhIsSet(DissimKey) || symhIsSet(SimilarKey))
	{
		if (!symhIsSet(SimilarKey))
		{
			ikey = KDISSIM;
		}
		else if (!symhIsSet(DissimKey))
		{
			ikey = KSIMILAR;
		}
		else
		{
			sprintf(OUTSTR,
					"ERROR: can't use both keywords %s and %s on %s()",
					KeyName(keys, KDISSIM), KeyName(keys, KSIMILAR),
					FUNCNAME);
			goto errorExit;
		}

		arg2 = KeySymhValue(keys, ikey);
		keyword = KeyName(keys, ikey);
		(void) isMatrix(arg2, dim);

		if (logicalIsSet(StandardKey) && StandardKey)
		{
			sprintf(OUTSTR,
					"ERROR: can't use %s:T with keyword %s on %s()",
					KeyName(keys, KSTANDARD), keyword, FUNCNAME);
		}
		else if (charIsSet(DistanceKey))
		{
			sprintf(OUTSTR,
					"ERROR: can't use keyword %s with keyword %s on %s()",
					keyword, KeyName(keys, KDISTANCE), FUNCNAME);
		}
		else if (dataH != (double **) 0)
		{
			sprintf(OUTSTR,
					"ERROR: can't use keyword %s and also provide data matrix",
					keyword);
		}
		else if (dim[0] < 2)
		{
			sprintf(OUTSTR,
					"ERROR: value for '%s' not a REAL square matrix with at least 2 rows",
					keyword);
		}
		else if (anyMissing(arg2))
		{
			sprintf(OUTSTR,
					"ERROR: value for '%s' must not contain MISSING values",
					keyword);
		}
		if (*OUTSTR)
		{
			goto errorExit;
		}

		StandardKey = 0;
		
		dataH = DATA(arg2);
		dismethod = (ikey == KDISSIM) ? DISSIM : SIMILAR;
	} /*if (symhIsSet(DissimKey) || symhIsSet(SimilarKey))*/

	if (logicalIsSet(PrintKey))
	{
		printTree = printClasses = PrintKey;
	}
	printClasses = (logicalIsSet(ClassesKey)) ? ClassesKey : printClasses;

	printTree = (logicalIsSet(TreeKey)) ? TreeKey : printTree;

	reorderCases = (logicalIsSet(ReorderKey)) ?  ReorderKey : 0;

	standardize = (logicalIsSet(StandardKey)) ? StandardKey : 1;

	if (charIsSet(MethodKey))
	{
		
		if ((method = matchKey(*MethodKey,Methods,MethodCodes)) == 0)
		{
			sprintf(OUTSTR,"ERROR: unrecognized value %s for %s on %s()",
					*MethodKey, KeyName(keys, KMETHOD), FUNCNAME);
			goto errorExit;
		}
	} /*if (charIsSet(MethodKey))*/
	else
	{
		method = AVERAGE; /* default */
	}
	
	if (charIsSet(DistanceKey))
	{
		if ((dismethod = 
			matchKey(*DistanceKey,DisMethods,DisMethodCodes)) == 0)
		{
			sprintf(OUTSTR,"ERROR: unrecognized value %s for %s on %s()",
					*DistanceKey, KeyName(keys, KDISTANCE), FUNCNAME);
			goto errorExit;
		}
	} /*if (charIsSet(DistanceKey))*/
	else if (dismethod == 0)
	{
		dismethod = EUCLID;
	}
	
	if (longIsSet(NclustersKey))
	{
		if (NclustersKey < 2)
		{
			sprintf(OUTSTR,
					"ERROR: value for %s must be integer >= 2",
					KeyName(keys,KNCLUSTERS));
			goto errorExit;
		}
		lev = NclustersKey;
	} /*if (longIsSet(NclustersKey))*/

	if (symhIsSet(KeepKey))
	{
		arg2 = KeepKey;
		if ((length = symbolSize(arg2)) > 3)
		{
			sprintf(OUTSTR,
					"ERROR: value for %s must be CHARACTER vector of length at most 3",
					KeyName(keys, KKEEP));
			goto errorExit;
		}
		else
		{
			keyword = STRINGPTR(arg2);
			for (i = 0;i < length;i++)
			{
				if (strncmp(keyword,"dist",4) == 0)
				{
					savedistance = 1;
				}
				else if (strncmp(keyword,"class",5) == 0)
				{
					saveclasses = 1;
				}
				else if (strncmp(keyword,"crit",4) == 0)
				{
					savecrit = 1;
				}
				else if (strcmp(keyword,"all") == 0)
				{
					savedistance = saveclasses = savecrit = 1;
				}
				else
				{
					sprintf(OUTSTR,
							"ERROR: unrecognized value %s for 'keep' on %s()",
							keyword, FUNCNAME);
					goto errorExit;
				}
				keyword = skipStrings(keyword, 1);
			} /*for (i = 0;i < length;i++)*/
			ncomps = savecrit + savedistance + saveclasses;
			printTree = (printTree < 0) ? 0 : printTree;
			printClasses = (printClasses < 0) ? 0 : printClasses;
		}
	} /*if (symhIsSet(KeepKey))*/

	printTree = (printTree < 0) ? 1 : printTree;
	printClasses = (printClasses < 0) ? 1 : printClasses;
	reorderCases = (reorderCases < 0) ? 0 : reorderCases;
	if ((printTree || printClasses) && NclustersKey > MAXNCLUSTERS)
	{
		sprintf(OUTSTR,
				"ERROR: can't have value for %s > %ld when %s() gives printed output",
				KeyName(keys,KNCLUSTERS), (long) MAXNCLUSTERS, FUNCNAME);
		goto errorExit;
	}

	if (!printClasses && reorderCases)
	{
		sprintf(OUTSTR,
				"WARNING: %s:T ignored on %s() when classes not printed",
				KeyName(keys, KREORDER), FUNCNAME);
		putErrorOUTSTR();
		reorderCases = 0;
	} /*if (!printClasses && reorderCases)*/

	if (dataH == (double **) 0)
	{
		sprintf(OUTSTR,
				"ERROR: %s() needs data, dissimilarity, or similarity matrix",
				FUNCNAME);
		goto errorExit;
	} /*if (dataH == (double **) 0)*/
	
	MethodName = MethodNames[method-1];
	
	if (standardize)
	{
		DisMethodName = (dismethod == EUCLID) ?
			"Euclidean (standardized)" : "Squared Euclidean (standardized)";
	}
	else
	{
		DisMethodName = DisMethodNames[dismethod-1];
	}
	
	n = dim[0];
	m = dim[1];
		
	lev = MIN(lev, n);
	len = (n*(n-1))/2; /* size of lower triangle of distance matrix */

/*	 allocate working space for doubles */

	if (!getScratch(critH,GCRIT,n,double) ||
	   !getScratch(membrH,GMEMBR,n,double) ||
	   !getScratch(critvalH,GCRITVAL,n,double) ||
	   !getScratch(disnnH,GDISNN,n,double) ||
	   !getScratch(dH,GD,len,double))
	{
		goto errorExit;
	}
	if (standardize && !getScratch(stdevH,GSTDEV,m,double))
	{
		goto errorExit;
	}
	
/* allocate working space for longs */
	if (!getScratch(iaH,GIA,n,long) ||
	   !getScratch(ibH,GIB,n,long) ||
	   !getScratch(flagH,GFLAG,n,long) ||
	   !getScratch(nnH,GNN,n,long) ||
	   !getScratch(hvalsH,GHVALS,lev,long) ||
	   !getScratch(iorderH,GIORDER,lev,long) ||
	   !getScratch(heightH,GHEIGHT,lev,long) ||
	   !getScratch(iclassH,GICLASS,n*(lev-1),long))
	{
		goto errorExit;
	}

	if (ncomps > 0)
	{
		if (ncomps == 1)
		{
			result = Install(SCRATCH,REAL);
		}
		else if (ncomps > 1)
		{
			result = StrucInstall(SCRATCH,ncomps);
		}
		if (result == (Symbolhandle) 0)
		{
			goto errorExit;
		}
	} /*if (ncomps > 0)*/	

	if (savedistance)
	{
		if (ncomps > 1)
		{
			if ((symh = Makesymbol(REAL)) == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			COMPVALUE(result,ncomps1++) = symh;
			setNAME(symh,"distances");
		}
		else
		{
			symh = result;
		}
		
		if ((TMPHANDLE = mygethandle(n*n * sizeof(double))) == (char **) 0)
		{
			goto errorExit;
		}
		setDATA(symh, (double **) TMPHANDLE);
		setNDIMS(symh,2);
		setDIM(symh,1,n);
		setDIM(symh,2,n);
		ddistH = DATA(symh);
	} /*if (savedistance)*/

/*
  Do the agglomeration
  Note that membrH, nnH, disnnH, flagH are scratch and are not used later

  ia[i] and ib[i] contain the indices of the clusters that were merged at
  step i, that is when there were n - i clusters.

  critH[i] contains the value of the criterion at that stage
*/
	hc(n, m, method, dismethod, *dataH,
	   (ddistH != (double **) 0) ? *ddistH : (double *) 0,
	   (stdevH != (double **) 0) ? *stdevH : (double *) 0,
	   *iaH, *ibH, *critH, *membrH, *nnH, 
	   *disnnH, *flagH, *dH);

	checkInterrupt(errorExit);
/*
  Derive and print (if required) the assignments into clusters for the
  top lev-1 levels of the hierarchy.

  Prepare also the required data for representing the dendrogram
  of this top part of the hierarchy.

  iclassH   class number - 1 of each case at each stage
  iorderH   ordering of objects along the bottom of the dendrogram
  heightH   height of the vertical above each object, in ordinal form
 *critvalH height in real values

  hvalsH    is scratch and is not further used.
  iaH       is used as scratch and is thus destroyed
*/
	hcass(n, *iaH, *ibH, *critH, lev, *iclassH, *hvalsH,
		  *iorderH, *critvalH, *heightH,  printClasses, reorderCases);
	
	if (printClasses && printTree && SCREENHEIGHT)
	{/* force pause before dendrogram */
		NLINES = SCREENHEIGHT+1;
	}
		
/*
   Construct and print a dendrogram of the top lev levels of
   the hierarchic clustering.
*/
	if (printTree)
	{
		hcden(lev, *iorderH, *heightH, *critvalH);
		checkInterrupt(errorExit);
	} /*if (printTree)*/
	
	if (saveclasses)
	{
		symh = (ncomps > 1) ? Makereal(0) : result;
		if (symh == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		if (ncomps > 1)
		{
			COMPVALUE(result,ncomps1++) = symh;
			setNAME(symh,"classes");
		}

		len = n * (lev - 1);
		if ((TMPHANDLE = mygethandle(len * sizeof(double))) == (char **) 0)
		{
			goto errorExit;
		}
		setDATA(symh, (double **) TMPHANDLE);
		setNDIMS(symh,2);
		setDIM(symh,1,n);
		setDIM(symh,2,lev-1);

		class = DATAPTR(symh);
		iclass = *iclassH;
		for (i = 0;i < len;i++)
		{
			class[i] = (double) (iclass[i] + 1);
		}
	} /*if (saveclasses)*/

	if (savecrit)
	{
		symh = (ncomps > 1) ? Makereal(0) : result;
		if (symh == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		if (ncomps > 1)
		{
			COMPVALUE(result,ncomps1++) = symh;
			setNAME(symh,"criterion");
		}
		unTrash(GCRITVAL); /* ensure its survival */
		setDATA(symh,critvalH);

		critval = *critvalH;
		for (i = 1;i < lev;i++)
		{ /* if not SIMILAR change signs to force descending order */
			critval[i-1] = (dismethod == SIMILAR) ?
				critval[i] : -critval[i];
		} /*for (i = 1;i < lev;i++)*/

		sortquick (critval, lev-1);
		if (dismethod != SIMILAR)
		{
			for (i = 0;i < lev-1;i++)
			{ /* restore signs */
				critval[i] = -critval[i];
			}
		}				
		/* shrink critval to right size */
		critvalH = (double **) mygrowhandle((char **) critvalH,
										   (lev-1) * sizeof(double));
		setDATA(symh,critvalH);
		if (critvalH == (double **) 0)
		{
			goto errorExit;
		}
		setNDIMS(symh,1);
		setDIM(symh,1,lev-1);
	} /*if (savecrit)*/

	emptyTrash(); /* dispose of all scratch */
	
	return( (ncomps > 0) ? result : NULLSYMBOL);
	
  errorExit:
	putErrorOUTSTR();
	
	emptyTrash(); /* dispose of all scratch */

	Removesymbol(result);
	
	return (0);
} /*cluster()*/
