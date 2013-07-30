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

void sortquick(double /*v*/ [], long /*n*/);
void sortquickchar(char * /*v*/ [], long /*n*/);

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

  This version is for sorting only a single double array
*/
#if (1)
#undef  valueType
#undef  lessThan
#undef  moreThan
#define valueType double
#define lessThan(A, B) ((double) (A) < (double) (B))
#define moreThan(A, B) ((double) (A) > (double) (B))

void sortquick(valueType v[], long n)
{
	long          il[21], iu[21];
	long          i, j, k, l, ij, m;
	long          bypass = 0;
	valueType     t;
	valueType     tt;
	double        r;
	
	if (n <= 1)
	{
		return;
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
					return;
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
				ij = i + (int) ((j-i)*r);
				t = v[ij];
				/*
					if first element of array is greater
					than t, interchange with t
				*/
				if (moreThan(v[i], t))
				{
					v[ij] = v[i];
					v[i] = t;
					t = v[ij];
				} /*if (moreThan(v[i] > t)*/
				l = j;
				/*
					if last element of array is less than
					t, interchange with t
				*/
				if (lessThan(v[j], t))
				{
					v[ij] = v[j];
					v[j] = t;
					t = v[ij];
					/*
					   if first element of array is greater
					   than t, interchange with t
					*/
					if (moreThan(v[i], t))
					{
						v[ij] = v[i];
						v[i] = t;
						t = v[ij];
					}
				} /*if (lessThan(v[j], t))*/

				while(3)
				{
					/*
					   find an element in the second half of
					   the array which is smaller than t
					*/
					l--;
					if (!moreThan(v[l], t))
					{
						do
						{
							/*
							   find an element in the first half of
							   the array which is greater than t
							*/
							k++;
						} while (lessThan(v[k], t));
						/* interchange these elements */
						if (k > l)
						{
							break;
						}
						tt = v[l];
						v[l] = v[k];
						v[k] = tt;
					} /*if (v[l] <= t)*/
				} /*while(3)*/
				/*
				   save upper and lower subscripts of
				   the array yet to be sorted
				*/
				if (l-i <= j-k)
				{
					il[m] = k;
					iu[m] = j;
					j = l;
				} /*if (l-i <= j-k)*/
				else
				{
					il[m] = i;
					iu[m] = l;
					i = k;
				} /*if (l-i <= j-k){}else{}*/
				m++;
			} /*while (j-i >= 1 || bypass)*/

			if (i == 0)
			{
				break;
			}

			i--;
			while (4)
			{
				i++;
				if (i == j)
				{
					break;
				}
				t = v[i+1];
				if (moreThan(v[i], t))
				{
					k = i;
					do
					{
						v[k+1] = v[k];
						k--;
					} while (moreThan(v[k], t));
					v[k+1] = t;
				} /*if (moreThan(v[i], t))*/
			} /* while(4)*/
		} /*while(2)*/
	} /*while(1)*/
} /*sortquick()*/


#else
void sortquick(double *v, long n)
{
	long          il[21], iu[21];
	long          i, j, k, l, ij, m;
	long          bypass = 0;
	double        r, t, tt;
	
	if (n <= 1)
	{
		return;
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
					return;
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
				ij = i + (int) ((j-i)*r);
				t = v[ij];
				/*
					if first element of array is greater
					than t, interchange with t
				*/
				if (v[i] > t)
				{
					v[ij] = v[i];
					v[i] = t;
					t = v[ij];
				}
				l = j;
				/*
					if last element of array is less than
					t, interchange with t
				*/
				if (v[j] < t)
				{
					v[ij] = v[j];
					v[j] = t;
					t = v[ij];
					/*
					   if first element of array is greater
					   than t, interchange with t
					*/
					if (v[i] > t)
					{
						v[ij] = v[i];
						v[i] = t;
						t = v[ij];
					}
				} /*if (v[j] < t)*/

				while(3)
				{
					/*
					   find an element in the second half of
					   the array which is smaller than t
					*/
					l--;
					if (v[l] <= t)
					{
						do
						{
							/*
							   find an element in the first half of
							   the array which is greater than t
							*/
							k++;
						} while (v[k] < t);
						/* interchange these elements */
						if (k > l)
						{
							break;
						}
						tt = v[l];
						v[l] = v[k];
						v[k] = tt;
					} /*if (v[l] <= t)*/
				} /*while(3)*/
				/*
				   save upper and lower subscripts of
				   the array yet to be sorted
				*/
				if (l-i <= j-k)
				{
					il[m] = k;
					iu[m] = j;
					j = l;
				} /*if (l-i <= j-k)*/
				else
				{
					il[m] = i;
					iu[m] = l;
					i = k;
				} /*if (l-i <= j-k){}else{}*/
				m++;
			} /*while (j-i >= 1 || bypass)*/

			if (i == 0)
			{
				break;
			}

			i--;
			while (4)
			{
				i++;
				if (i == j)
				{
					break;
				}
				t = v[i+1];
				if (v[i] > t)
				{
					k = i;
					do
					{
						v[k+1] = v[k];
						k--;
					} while (t < v[k]);
					v[k+1] = t;
				} /*if (v[i] > t)*/
			} /* while(4)*/
		} /*while(2)*/
	} /*while(1)*/
} /*sortquick()*/
#endif
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

  This version is for sorting only a single array of char *
*/

#undef  valueType
#undef  lessThan
#undef  moreThan
#define valueType char *
#define lessThan(A, B) (strcmp(A, B) < 0)
#define moreThan(A, B) (strcmp(A, B) > 0)
int strcmp(const char *, const char *);

void sortquickchar(valueType v[], long n)
{
	long          il[21], iu[21];
	long          i, j, k, l, ij, m;
	long          bypass = 0;
	valueType     t;
	valueType     tt;
	double        r;
	
	if (n <= 1)
	{
		return;
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
					return;
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
				ij = i + (int) ((j-i)*r);
				t = v[ij];
				/*
					if first element of array is greater
					than t, interchange with t
				*/
				if (moreThan(v[i], t))
				{
					v[ij] = v[i];
					v[i] = t;
					t = v[ij];
				} /*if (moreThan(v[i] > t)*/
				l = j;
				/*
					if last element of array is less than
					t, interchange with t
				*/
				if (lessThan(v[j], t))
				{
					v[ij] = v[j];
					v[j] = t;
					t = v[ij];
					/*
					   if first element of array is greater
					   than t, interchange with t
					*/
					if (moreThan(v[i], t))
					{
						v[ij] = v[i];
						v[i] = t;
						t = v[ij];
					}
				} /*if (lessThan(v[j], t))*/

				while(3)
				{
					/*
					   find an element in the second half of
					   the array which is smaller than t
					*/
					l--;
					if (!moreThan(v[l], t))
					{
						do
						{
							/*
							   find an element in the first half of
							   the array which is greater than t
							*/
							k++;
						} while (lessThan(v[k], t));
						/* interchange these elements */
						if (k > l)
						{
							break;
						}
						tt = v[l];
						v[l] = v[k];
						v[k] = tt;
					} /*if (v[l] <= t)*/
				} /*while(3)*/
				/*
				   save upper and lower subscripts of
				   the array yet to be sorted
				*/
				if (l-i <= j-k)
				{
					il[m] = k;
					iu[m] = j;
					j = l;
				} /*if (l-i <= j-k)*/
				else
				{
					il[m] = i;
					iu[m] = l;
					i = k;
				} /*if (l-i <= j-k){}else{}*/
				m++;
			} /*while (j-i >= 1 || bypass)*/

			if (i == 0)
			{
				break;
			}

			i--;
			while (4)
			{
				i++;
				if (i == j)
				{
					break;
				}
				t = v[i+1];
				if (moreThan(v[i], t))
				{
					k = i;
					do
					{
						v[k+1] = v[k];
						k--;
					} while (moreThan(v[k], t));
					v[k+1] = t;
				} /*if (moreThan(v[i], t))*/
			} /* while(4)*/
		} /*while(2)*/
	} /*while(1)*/
} /*sortquickchar()*/

