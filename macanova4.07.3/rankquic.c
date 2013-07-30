/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 4.04 or later
*(C)*
*(C)* Copyright (c) 1997 by Gary Oehlert and Christopher Bingham
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

#include "typedefs.h"
/*
   adaptation of a translation to C of ssort.f from cmlib.
   ***author  Jones, R. E., (SNLA)
            Wisniewski, J. A., (SNLA)
      Written by Rondall E. Jones
      Modified by John A. Wisniewski to use the Singleton quicksort
      algorithm.  Date 18 November 1976.

      Reference
          Singleton, R. C., Algorithm 347, An efficient algorithm for
          sorting with minimal storage, CACM,12(3),1969,185-7.

  This version omits the code for sorting only a single double array
*/
#define          GRADEONLY    0x00
#define          TIESIGNORE   0x10
#define          TIESMEAN     0x20
#define          TIESMIN      0x30

void rankquick(double * /*v*/, double * /*rank*/, double * /*scratch*/, long /*n*/, long /*op*/);
void rankquickchar(char * /*v*/[], double /*rank*/ [], double /*scratch*/ [],
			   long /*n*/,long /*op*/);

/* 
  If op == GRADEONLY, return (in rank[i] the index of the i-th order
  statistic
  Otherwise, return ranks.
   If op == TIESIGNORE, ignore ties.
   If op == TIESMEAN, compute average ranks of tied values
   If op == TIESMIN, compute minimum ranks of tied values

   960412 Added character quicksort rankquickchar()
   codes for rankquick() (for doubles) and rankquickchar() are identical,
   differing only in defines.  When computing ranks using rankquickchar(),
   ties are ignored.
*/

#if (1)
#undef  valueType
#undef  lessThan
#undef  moreThan
#define valueType double
#define lessThan(A, B) ((A) < (B))
#define moreThan(A, B) ((A) > (B))
#define  DONTCHANGEV /* v not changed */

void rankquick(valueType v [], double rank [], double scratch [],
			   long n,long op)
{
	long          il[21], iu[21];
	long          i, j, k, l, ij, m;
	long          bypass = 0;
	double        r;
	double        ty, tty;
	valueType     t;
	valueType     tt;
	valueType    *v1;
#ifdef DONTCHANGEV
	double        tiedrank;
	long          p;
	valueType     z;
#endif /*DONTCHANGEV*/

#ifdef DONTCHANGEV
	v1 = (valueType *) rank;
#else /*DONTCHANGEV*/
	if (op != GRADEONLY)
	{
		op = TIESIGNORE;
	}
	v1 = v;
#endif /*DONTCHANGEV*/
	/*
	  initialize scratch, (inverse ranking)
	  Copy v[] to rank for ordering; v[] is not changed.
	  rank[] will be reset at end to ranks or to ordering indices
	*/
	for (i = 0; i < n; i++)
	{
		scratch[i] = (double) i;
#ifdef DONTCHANGEV
		v1[i] = v[i];
#endif /*DONTCHANGEV*/
	}

	m = 0;
	i = 0;
	j = n-1;
	r = .375;

	while(1)
	{
		if (i != j)
		{
			if (r > .5898437)
			{
				r -= .21875;
			}
			else
			{
				r += 3.90625e-2;
			}
			bypass = 1; /* flag to bypass first block in next loop */
		} /*if (i != j)*/

		while(2)
		{
			/* begin again on another portion of the unsorted array */
			if (!bypass)
			{
				if (m-- == 0)
				{
					goto finishup;
				}
				i = il[m];
				j = iu[m];
			} /*if (!bypass)*/

			while (j-i >= 1 || bypass)
			{
				bypass = 0;

				k = i;
				/* 
				   select a central element of the array
				   and save it in location t
				*/
				ij = i + (long) ((j-i)*r);
				t = v1[ij];
				ty = scratch[ij];
				/* if first element of array is greater */
				/* than t, interchange with t */
				if (moreThan(v1[i], t))
				{
					v1[ij] = v1[i];
					v1[i] = t;
					t = v1[ij];
					scratch[ij] = scratch[i];
					scratch[i] = ty;
					ty = scratch[ij];
				} /*if (moreThan(v1[i], t))*/
				l = j;
				/* if last element of array is less than */
				/* t, interchange with t */
				if (lessThan(v1[j], t))
				{
					v1[ij] = v1[j];
					v1[j] = t;
					t = v1[ij];
					scratch[ij] = scratch[j];
					scratch[j] = ty;
					ty = scratch[ij];
					/* if first element of array is greater */
					/* than t, interchange with t */
					if (moreThan(v1[i], t))
					{
						v1[ij] = v1[i];
						v1[i] = t;
						t = v1[ij];
						scratch[ij] = scratch[i];
						scratch[i] = ty;
						ty = scratch[ij];
					} /*if (moreThan(v1[i], t))*/
				} /*if (lessThan(v1[j], t))*/

				while(3)
				{
					/* find an element in the second half of */
					/* the array which is smaller than t */
					l--;
					if (!moreThan(v1[l], t))
					{
						do
						{
							/* find an element in the first half of */
							/* the array which is greater than t */
							k++;
						} while(lessThan(v1[k], t));
						/* interchange these elements */
						if (k > l)
						{
							break;
						}
						tt = v1[l];
						v1[l] = v1[k];
						v1[k] = tt;
						tty = scratch[l];
						scratch[l] = scratch[k];
						scratch[k] = tty;
					} /*if (v1[l] <= t)*/
				} /*while(3)*/
				/* save upper and lower subscripts of */
				/* the array yet to be sorted */
				if (l-i <= j-k)
				{
					il[m] = k;
					iu[m] = j;
					j = l;
				}
				else
				{
					il[m] = i;
					iu[m] = l;
					i = k;
				}
				m++;
			} /*while (j-i >= 1)*/

			if (i == 0)
			{
				break;
			}
			i--;
			while(4)
			{
				i++;
				if (i == j)
				{
					break;
				}
				t = v1[i+1];
				ty = scratch[i+1];
				if (moreThan(v1[i], t))
				{
					k = i;
					do
					{
						v1[k+1] = v1[k];
						scratch[k+1] = scratch[k];
						k--;
					} while (moreThan(v1[k],t));
					v1[k+1] = t;
					scratch[k+1] = ty;
				} /*if (v1[i] > t)*/
			} /*while(4)*/
		} /*while(2)*/
	} /*while(1)*/

  finishup:
	
	/*
	  At this point, v1[] should contain ordered values of v[] and
	  scratch[i] should contain original index of v[i]
	  If DONTCHANGEV is defined, v[] contains original input.
    */
	if(op == GRADEONLY)
	{ /* grade */
		for(i = 0;i < n;i++)
		{
			rank[i] = scratch[i] + 1;
		}
	} /*if(op == GRADEONLY)*/
	else
	{/* set up real ranks */
		if(op == TIESIGNORE)
		{
			for(i = 0;i < n;i++)
			{ /* replace ordered values by ranks ignoring ties */
				rank[(long) scratch[i]] = i + 1;
			}
		} /*if(op == TIESIGNORE)*/
#ifdef DONTCHANGEV
		else
		{ /* average or take minimum of tied ranks */
			j = 0;
			while(j < n)
			{
				p = (long) scratch[j];
				z = v[p];
				tiedrank = (double) (j+1);
				for(i = j+1;i<n;i++)
				{/* find all values tied with z & compute combined rank */
					p = (long) scratch[i];
					if(v[p] == z)
					{
						if(op == TIESMEAN)
						{
							tiedrank += .5;
						}
					}
					else
					{
						break;
					}
				}
				while(j < i)
				{/* replace tied values by tiedrank */
					p = (long) scratch[j++];
					rank[p] = tiedrank;
				}
			} /*while(j < n)*/
		} /*if(op == TIESIGNORE){}else{}*/
#endif /*DONTCHANGEV*/
	} /*if(op == GRADEONLY){}else{}*/
} /*rankquick()*/

#else
/* original version */
void rankquick(double * v, double * rank, double * scratch, long n,long op)
{
	long          il[21], iu[21];
	long          i, j, k, l, ij, m;
	long          bypass = 0;
	long          p;
	double        r, t, ty, tt, tty;
	double        z, tiedrank;

	/*
	  initialize scratch, (inverse ranking)
	  Copy v[] to rank for ordering; v[] is not changed.
	  rank[] will be reset at end to ranks or to ordering indices
	*/
	for (i = 0; i < n; i++)
	{
		scratch[i] = (double) i;
		rank[i] = v[i];
	}

	m = 0;
	i = 0;
	j = n-1;
	r = .375;

	while(1)
	{
		if (i != j)
		{
			if (r > .5898437)
			{
				r -= .21875;
			}
			else
			{
				r += 3.90625e-2;
			}
			bypass = 1; /* flag to bypass first block in next loop */
		} /*if (i != j)*/

		while(2)
		{
			/* begin again on another portion of the unsorted array */
			if (!bypass)
			{
				if (m-- == 0)
				{
					goto finishup;
				}
				i = il[m];
				j = iu[m];
			} /*if (!bypass)*/

			while (j-i >= 1 || bypass)
			{
				bypass = 0;

				k = i;
				/* 
				   select a central element of the array
				   and save it in location t
				*/
				ij = i + (long) ((j-i)*r);
				t = rank[ij];
				ty = scratch[ij];
				/* if first element of array is greater */
				/* than t, interchange with t */
				if (rank[i] > t)
				{
					rank[ij] = rank[i];
					rank[i] = t;
					t = rank[ij];
					scratch[ij] = scratch[i];
					scratch[i] = ty;
					ty = scratch[ij];
				} /*if (rank[i] > t)*/
				l = j;
				/* if last element of array is less than */
				/* t, interchange with t */
				if (rank[j] < t)
				{
					rank[ij] = rank[j];
					rank[j] = t;
					t = rank[ij];
					scratch[ij] = scratch[j];
					scratch[j] = ty;
					ty = scratch[ij];
					/* if first element of array is greater */
					/* than t, interchange with t */
					if (rank[i] > t)
					{
						rank[ij] = rank[i];
						rank[i] = t;
						t = rank[ij];
						scratch[ij] = scratch[i];
						scratch[i] = ty;
						ty = scratch[ij];
					} /*if (rank[i] > t)*/
				} /*if (rank[j] < t)*/

				while(3)
				{
					/* find an element in the second half of */
					/* the array which is smaller than t */
					l--;
					if (rank[l] <= t)
					{
						do
						{
							/* find an element in the first half of */
							/* the array which is greater than t */
							k++;
						} while (!(rank[k] >= t));
						/* interchange these elements */
						if (k > l)
						{
							break;
						}
						tt = rank[l];
						rank[l] = rank[k];
						rank[k] = tt;
						tty = scratch[l];
						scratch[l] = scratch[k];
						scratch[k] = tty;
					} /*if (rank[l] <= t)*/
				} /*while(3)*/
				/* save upper and lower subscripts of */
				/* the array yet to be sorted */
				if (l-i <= j-k)
				{
					il[m] = k;
					iu[m] = j;
					j = l;
				}
				else
				{
					il[m] = i;
					iu[m] = l;
					i = k;
				}
				m++;
			} /*while (j-i >= 1)*/

			if (i == 0)
			{
				break;
			}
			i--;
			while(4)
			{
				i++;
				if (i == j)
				{
					break;
				}
				t = rank[i+1];
				ty = scratch[i+1];
				if (rank[i] > t)
				{
					k = i;
					do
					{
						rank[k+1] = rank[k];
						scratch[k+1] = scratch[k];
						k--;
					} while (!(t >= rank[k]));
					rank[k+1] = t;
					scratch[k+1] = ty;
				} /*if (rank[i] > t)*/
			} /*while(4)*/
		} /*while(2)*/
	} /*while(1)*/

  finishup:
	
	/*
	  At this point, rank[] should contain ordered values of v[] and
	  scratch[i] should contain original index of v[i]
    */
	if(op == GRADEONLY)
	{ /* grade */
		for(i = 0;i < n;i++)
		{
			rank[i] = scratch[i] + 1;
		}
	} /*if(op == GRADEONLY)*/
	else
	{/* set up real ranks */
		if(op == TIESIGNORE)
		{
			for(i = 0;i < n;i++)
			{ /* replace ordered values by ranks ignoring ties */
				rank[(long) scratch[i]] = i + 1;
			}
		} /*if(op == TIESIGNORE)*/
		else
		{ /* average or take minimum of tied ranks */
			j = 0;
			while(j < n)
			{
				p = (long) scratch[j];
				z = v[p];
				tiedrank = (double) (j+1);
				for(i = j+1;i<n;i++)
				{/* find all values tied with z & compute combined rank */
					p = (long) scratch[i];
					if(v[p] == z)
					{
						if(op == TIESMEAN)
						{
							tiedrank += .5;
						}
					}
					else
					{
						break;
					}
				}
				while(j < i)
				{/* replace tied values by tiedrank */
					p = (long) scratch[j++];
					rank[p] = tiedrank;
				}
			} /*while(j < n)*/
		} /*if(op == TIESIGNORE){}else{}*/
	} /*if(op == GRADEONLY){}else{}*/
} /*rankquick()*/
#endif

#undef  valueType
#undef  lessThan
#undef  moreThan
#define valueType char *
#define lessThan(A, B) (strcmp(A, B) < 0)
#define moreThan(A, B) (strcmp(A, B) > 0)
#undef  DONTCHANGEV /* v changed, no tied ranks computed */

int strcmp(const char *, const char *);

void rankquickchar(valueType v [], double rank [], double scratch [],
			   long n,long op)
{
	long          il[21], iu[21];
	long          i, j, k, l, ij, m;
	long          bypass = 0;
	double        r;
	double        ty, tty;
	valueType     t;
	valueType     tt;
	valueType    *v1;
#ifdef DONTCHANGEV
	double        tiedrank;
	long          p;
	valueType     z;
#endif /*DONTCHANGEV*/

#ifdef DONTCHANGEV
	v1 = (valueType *) rank;
#else /*DONTCHANGEV*/
	if (op != GRADEONLY)
	{
		op = TIESIGNORE;
	}
	v1 = v;
#endif /*DONTCHANGEV*/
	/*
	  initialize scratch, (inverse ranking)
	  Copy v[] to rank for ordering; v[] is not changed.
	  rank[] will be reset at end to ranks or to ordering indices
	*/
	for (i = 0; i < n; i++)
	{
		scratch[i] = (double) i;
#ifdef DONTCHANGEV
		v1[i] = v[i];
#endif /*DONTCHANGEV*/
	}

	m = 0;
	i = 0;
	j = n-1;
	r = .375;

	while(1)
	{
		if (i != j)
		{
			if (r > .5898437)
			{
				r -= .21875;
			}
			else
			{
				r += 3.90625e-2;
			}
			bypass = 1; /* flag to bypass first block in next loop */
		} /*if (i != j)*/

		while(2)
		{
			/* begin again on another portion of the unsorted array */
			if (!bypass)
			{
				if (m-- == 0)
				{
					goto finishup;
				}
				i = il[m];
				j = iu[m];
			} /*if (!bypass)*/

			while (j-i >= 1 || bypass)
			{
				bypass = 0;

				k = i;
				/* 
				   select a central element of the array
				   and save it in location t
				*/
				ij = i + (long) ((j-i)*r);
				t = v1[ij];
				ty = scratch[ij];
				/* if first element of array is greater */
				/* than t, interchange with t */
				if (moreThan(v1[i], t))
				{
					v1[ij] = v1[i];
					v1[i] = t;
					t = v1[ij];
					scratch[ij] = scratch[i];
					scratch[i] = ty;
					ty = scratch[ij];
				} /*if (moreThan(v1[i], t))*/
				l = j;
				/* if last element of array is less than */
				/* t, interchange with t */
				if (lessThan(v1[j], t))
				{
					v1[ij] = v1[j];
					v1[j] = t;
					t = v1[ij];
					scratch[ij] = scratch[j];
					scratch[j] = ty;
					ty = scratch[ij];
					/* if first element of array is greater */
					/* than t, interchange with t */
					if (moreThan(v1[i], t))
					{
						v1[ij] = v1[i];
						v1[i] = t;
						t = v1[ij];
						scratch[ij] = scratch[i];
						scratch[i] = ty;
						ty = scratch[ij];
					} /*if (moreThan(v1[i], t))*/
				} /*if (lessThan(v1[j], t))*/

				while(3)
				{
					/* find an element in the second half of */
					/* the array which is smaller than t */
					l--;
					if (!moreThan(v1[l], t))
					{
						do
						{
							/* find an element in the first half of */
							/* the array which is greater than t */
							k++;
						} while(lessThan(v1[k], t));
						/* interchange these elements */
						if (k > l)
						{
							break;
						}
						tt = v1[l];
						v1[l] = v1[k];
						v1[k] = tt;
						tty = scratch[l];
						scratch[l] = scratch[k];
						scratch[k] = tty;
					} /*if (v1[l] <= t)*/
				} /*while(3)*/
				/* save upper and lower subscripts of */
				/* the array yet to be sorted */
				if (l-i <= j-k)
				{
					il[m] = k;
					iu[m] = j;
					j = l;
				}
				else
				{
					il[m] = i;
					iu[m] = l;
					i = k;
				}
				m++;
			} /*while (j-i >= 1)*/

			if (i == 0)
			{
				break;
			}
			i--;
			while(4)
			{
				i++;
				if (i == j)
				{
					break;
				}
				t = v1[i+1];
				ty = scratch[i+1];
				if (moreThan(v1[i], t))
				{
					k = i;
					do
					{
						v1[k+1] = v1[k];
						scratch[k+1] = scratch[k];
						k--;
					} while (moreThan(v1[k],t));
					v1[k+1] = t;
					scratch[k+1] = ty;
				} /*if (v1[i] > t)*/
			} /*while(4)*/
		} /*while(2)*/
	} /*while(1)*/

  finishup:
	
	/*
	  At this point, v1[] should contain ordered values of v[] and
	  scratch[i] should contain original index of v[i]
	  If DONTCHANGEV is defined, v[] contains original input.
    */
	if(op == GRADEONLY)
	{ /* grade */
		for(i = 0;i < n;i++)
		{
			rank[i] = scratch[i] + 1;
		}
	} /*if(op == GRADEONLY)*/
	else
	{/* set up real ranks */
		if(op == TIESIGNORE)
		{
			for(i = 0;i < n;i++)
			{ /* replace ordered values by ranks ignoring ties */
				rank[(long) scratch[i]] = i + 1;
			}
		} /*if(op == TIESIGNORE)*/
#ifdef DONTCHANGEV
		else
		{ /* average or take minimum of tied ranks */
			j = 0;
			while(j < n)
			{
				p = (long) scratch[j];
				z = v[p];
				tiedrank = (double) (j+1);
				for(i = j+1;i<n;i++)
				{/* find all values tied with z & compute combined rank */
					p = (long) scratch[i];
					if(v[p] == z)
					{
						if(op == TIESMEAN)
						{
							tiedrank += .5;
						}
					}
					else
					{
						break;
					}
				}
				while(j < i)
				{/* replace tied values by tiedrank */
					p = (long) scratch[j++];
					rank[p] = tiedrank;
				}
			} /*while(j < n)*/
		} /*if(op == TIESIGNORE){}else{}*/
#endif /*DONTCHANGEV*/
	} /*if(op == GRADEONLY){}else{}*/
} /*rankquickchar()*/
