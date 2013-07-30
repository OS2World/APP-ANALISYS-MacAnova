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
#pragma segment Labutils
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

#include "globals.h"
#include "keywords.h"

#ifdef MACINTOSH
#include "macIface.h"
#endif /*MACINTOSH*/

/*
   Functions useful in working with symbol labels
   960307 New file
   960308 Added MacAnova function getlabels (getlabs())
   970310 New functions prepPrintLabels(), putColumnLabels(),
          putRowLabels() related to printing (moved from commonio.c)
   970313 Added code to handle response to new options 'labelstyle'
          and 'labelabove'
   971126 Modified appendLabels() to recognize more values for argument 3
   980720 Many changes reflecting new format of labels
   980726 Added code for new functions setlabels() and setnotes()
   990226 Replaced putOUTSTR() by putErrorOUTSTR()
*/
void getAllLabels(Symbolhandle /*arg*/, char * /*labels*/ [],
			   long /*lengths*/ [], long /*widths*/ []);
char * getOneLabel(Symbolhandle /*symh*/, long /*dimension*/, long /*index*/);
void getMatLabels(Symbolhandle /*arg*/, char * /*rowcol*/ [2],
				  long /*lengths*/ [2]);
char **createLabels(long /*ndims*/, long /*lengths*/ []);
void buildLabels(char ** /*labelsH*/, char * /*labels*/ [], long /*dims*/ [],
				 long /*ndims*/);
char ** growLabels(char ** /*oldlabels*/, long /*length*/);
long expandLabels(char * /*root*/, long /*n*/, char * /*labels*/);
long moveMatLabels(Symbolhandle /*from*/, Symbolhandle /*to*/,
				   unsigned long /*control*/);
long fixupMatLabels(Symbolhandle /*symh*/, unsigned long /*control*/);
long transferLabels(Symbolhandle /*from*/, Symbolhandle /*to*/);
void appendLabels(Symbolhandle /*symh*/, char * /*newLabels*/, long /*jdim*/,
				  long /*expand*/);
void appendNLabels(Symbolhandle /*symh*/, char * /*newLabels*/, long /*jdim*/,
				   long /*startIndex*/, long /*nlabels*/);
long installLabels(Symbolhandle /*symhLabels*/, Symbolhandle /*result*/);
unsigned long checkLabels(Symbolhandle /*symhLabels*/, long /*ndims*/,
						  long /*dims*/ []);
long anyLabels(Symbolhandle /*symh*/);
long sizeOfLabels(Symbolhandle /*symh*/);
long prepPrintLabels(Symbolhandle /*symh*/,
					    char * /*dimLabels*/ [], long /*dimLabelsWidth*/ []);
void putColumnLabels(FILE * /*fp*/, long /*lastdim*/, char * /*labels*/,
					 long /*labelWidth*/, long /*fieldWidth*/,
					 long /*nperLine*/, long /*type*/);
void putRowLabels(FILE * /*fp*/, long /*ndims*/, long /*coord*/ [],
				  long /*labelWidth*/, char * /*labels*/ [],
				  long * /*labelsWidth*/, long /*charData*/);

static char    *TempLabels[MAXDIMS];
/*FuseLabels[i] != 0 <==> TempLabel[i] to be fused with TempLabels[i-1] */
static char     FuseLabels[MAXDIMS+1];

static char     NumericLabel[] = {NUMERICLABEL, '\0'};

/*
   Return pointers to the labels for each dimension of arg in array
   labels.  If there are no labels or NDIMS(arg) == 0, labels[0] is
   set to 0.  Normally labels will be an array of size MAXDIMS

   If lengths != (long *) 0, lengths[i] is set to the number of
   characters in the dimension i labels.

   If widths != (long *) 0, widths[i] is set to the maximum number of
   characters in any dimension i label.

   If both lengths and widths are (long *) 0 the labels for the last
   dimension need not be complete for this to work as intended.  Thus
   if you are creating identical labels for both dimensions of a 
   square matrix, you can build up the labels for the first dimension using
   appendNLabels, say, and then get pointers to that vector of
   labels and the vector for the unconstructed set of labels for the
   second dimensions using getAllLabels() and then use copyStrings() to
   duplicate the first vector into the secont.

   WARNING:  note that this implicitly dereferences a handle.  If any
   storage is allocated, expandLabels must be called again to insure
   the pointers are still up to date.
*/

void getAllLabels(Symbolhandle arg, char *  labels [], long lengths [],
				  long widths [])
{
	long           dimi, ndims = NDIMS(arg);
	long           i, j;
	int            setLengths = (lengths != (long *) 0);
	int            setWidths = (widths != (long *) 0);
	char          *place;
	WHERE("getAllLabels");

	for (i = 0; i == 0 || i < ndims; i++)
	{
		if (setLengths)
		{
			lengths[i] = 0;
		}
		if (setWidths)
		{
			widths[i] = 0;
		}
	} /*for (i = 0; i < ndims; i++)*/

	if (ndims == 0 || !HASLABELS(arg))
	{
		/* no labels at all ; all pointers set to 0 */
		for (i = 0; i == 0 || i < ndims; i++)
		{
			labels[i] = (char *) 0;
		} /*for (i = 0; i < ndims; i++)*/
	} /*if (ndims == 0 || !HASLABELS(arg))*/
	else
	{ /*ndims > 0 && HASLABELS(arg)*/
		place = getOneLabel(arg, 0, 0);

		for (i = 0; i < ndims; i++)
		{
			labels[i] = place;
			dimi = DIMVAL(arg, i+1);

			if (setWidths || setLengths)
			{
				int        foundNonTemp = 0;
				long       length = 0;
				char      *firstLabel = place;

				for (j = 0; j < dimi; j++)
				{
					int          width = strlen(place);

					foundNonTemp = foundNonTemp || 
						place[0] != NUMERICLABEL ||
						length != width * j ||
						strcmp(place, firstLabel) != 0;
					
					if (setWidths && width > widths[i])
					{
						widths[i] = width;
					}
					length += width;
					place = skipStrings(place, 1);
				} /*for (j = 0; j < dimi; j++)*/

				if (setLengths)
				{
					lengths[i] = length + dimi;
				}
				
				if (setWidths && !foundNonTemp)
				{
					widths[i] = 0;
				}
			} /*if (setWidths || setLengths)*/
			else if (i < ndims - 1)
			{
				place = skipStrings(place, dimi);
			}
		} /*for (i = 0; i < ndims; i++)*/
	} /*if (ndims == 0 || !HASLABELS(arg)){}else{}*/
} /*getAllLabels()*/

/*
   Return a pointer to the index-th label for coordinate dimension.
   Both index and dimension are 0-based

   getOneLabel(symh, dim, 0) returns pointer to
   the labels for coordinate dim.
*/
char * getOneLabel(Symbolhandle symh, long dimension, long index)
{
	char        *label;
	
	if (HASLABELS(symh))
	{
		long       i;

		label = LABELSPTR(symh);
		for (i = 1; i <= dimension; i++)
		{
			label = skipStrings(label, DIMVAL(symh, i));
		}
		if (index > 0)
		{
			label = skipStrings(label, index);
		}
	} /*if (HASLABELS(symh))*/
	else
	{
		label = (char *) 0;
	}
	return (label);
} /*getOneLabel()*/

/*
   Get row and column labels of matrix or generalized matrix.
   lengths[0] and lengths[1] are the number of bytes in each set of labels

   970801 fixed bug.  When NDIMS(arg) == 1, lengths[1] was being
   set to 1 (appropriate for null label "") instead of 2 (for
   numerical label "@")
*/

void getMatLabels(Symbolhandle arg, char * rowcol [2], long lengths [2])
{
	char        *labels[MAXDIMS];
	long         labelLengths[MAXDIMS];
	long         i, ndims = NDIMS(arg);
	long         dim1, dim2; /*indices of row and col dimensions*/
	WHERE("getMatLabels");

	rowcol[0] = rowcol[1] = (char *) 0;
	lengths[0] = lengths[1] = 0;
	
	if (isMatrix(arg, (long *) 0) && HASLABELS(arg))
	{
		getAllLabels(arg, labels, labelLengths, (long *) 0);
		if (ndims <= 2 || isVector(arg))
		{
			dim1 = 0;
			dim2 = 1;
		} /*if (ndims <= 2 || isVector(arg))*/
		else 
		{ /* either non-scalar row vector or 2 dimensions > 1 */
			dim1 = dim2 = -1;
			for (i = 1; i <= ndims; i++)
			{
				if (DIMVAL(arg, i) > 1)
				{
					if (dim1 < 0)
					{
						dim1 = i - 1;
					}
					else
					{
						dim2 = i - 1;
						break;
					}
				} /*if (DIMVAL(arg, i) > 1)*/
			} /*for (i = 1; i <= ndims; i++)*/

			if (dim2 < 0)
			{ /* must be row vector*/
				dim2 = dim1;
				dim1 = 0;
			}
		} /*if (ndims <= 2 || isVector(arg)){}else{}*/

		/*
		  dim1 and dim2 such that nrows = DIMVAL(arg,dim1+1) and
		  ncols = DIMVAL(arg,dim2+1), except that dim2 = 1
		  when ndims = 1 and DIMVAL(arg,2) = 0 
		*/
		rowcol[0] = labels[dim1];
		rowcol[1] = (ndims > 1) ? labels[dim2] : NumericLabel;
		lengths[0] = labelLengths[dim1];
		lengths[1] = (ndims > 1) ? labelLengths[dim2] : 2;
	} /*if (!isMatrix(arg, dims) && HASLABELS(arg))*/	
} /*getMatLabels()*/

/*
   createLabels(ndims, lengths) creates a handle big enough to hold
   n dims vectors of labels, with vector i of lengths[i], including nulls
*/
char **createLabels(long ndims, long lengths [])
{
	long      tot = 0, i;
	
	for (i = 0; i < ndims; i++)
	{
		tot += lengths[i];
	}
	return ((tot > 0) ? mygethandle(tot) : (char **) 0);
} /*createLabels()*/

/*
   growLabels(labelsH, length) grows a handle currently holding labels to a
   size sufficient to hold an additional dimension of labels of size
   length, including nulls; it returns the new handle.  If length < 0, it
   shrinks the label handle by length.
*/
char ** growLabels(char ** labelsH, long length)
{
	long      size = myhandlelength(labelsH);
	
	if (size > 0)
	{
		size += length;
		labelsH = (size > 0) ? mygrowhandle(labelsH, size) : (char **) 0;
	} /*if (labelsH != (char **) 0)*/
	else
	{
		labelsH = (char **) 0;
	}
	
	return (labelsH);
} /*growLabels()*/

/*
   Fill handle to be used for labels with vectors of labels in labels
*/
void buildLabels(char **labelsH, char * labels [], long dims [], long ndims)
{
	long        i;
	char       *pos;
	
	if (labelsH != (char **) 0)
	{
		pos = *labelsH;
		
		for (i = 0; i < ndims; i++)
		{
			pos = copyStrings(labels[i], pos, dims[i]);
		}
	} /*if (labelsH != (char **) 0)*/
} /*buildLabels*/

/*
   expandLabels() is used to build labels of the form "X1", "X2",...
   "Xn" where "X" is the value of CHARACTER scalar symh and n is the
   of labels wanted.

   It needs to be called twice; once to find the space needed, and then
   with an allocated vector of the right size to contain the labels.

   If "X" is "#", generated labels are "1","2", ... .

   If "X" is "", all generated labels are ""

   If "X" is "@.*", all generated labels are "X"

   expandLabels(STRINGPTR(symh), n, (char *) 0) returns the amount of space
   needed for the expanded labels.

   expandlabels(STRINGPTR(symh), n, labels) puts the expanded labels in labels
   which must have sufficient room.  It returns the actual space used.
   
   960307 New
   960310 Changed behavior on null labels so that the user has to
   explicitly ask for numerical labels by "#" to get them.  This also
   means the user can explicitly ask for null labels by ""
   970311 Scalars of form of "@.*" are just replicated, not expanded.
   980807 Fixed bug in computing length when n = a and labels in not
          a bracket
  */

long expandLabels(char * root, long n, char *labels)
{	
	long         i, jdigit , pow10;
	char        *place;
	char         pound = Lparens[PoundIndex];
	long         needed, nroot = strlen(root);
	int          numbersOnly;

	numbersOnly = (nroot == 1 &&
				   strchr(Lparens, (int) root[0]) != (char *) 0);

	if (labels == (char *) 0)
	{ /* compute how much room is needed */
		if (nroot == 0 || root[0] == NUMERICLABEL)
		{ /*all labels to be "" or duplicates of root*/
			needed = n * (nroot+1);
		} /*if (nroot == 0 || root[0] == NUMERICLABEL)*/
		else if (n == 1 && !numbersOnly)
		{
			/* non bracketed length 1; don't expand */
			needed = nroot + 1;
		}
		else
		{
			/*
			   compute space for vector("root1","root2",...,"rootNnnn")
			   or vector("1","2",...,"nnnn")
			*/

			if (numbersOnly)
			{
				/* space for '\0' or '[', ']' and '\0' */
				needed = (root[0] == pound) ? n : 3*n;
			}
			else
			{
				/* space for root and '\0' */
				needed = n * (nroot + 1);
			}

			/* compute space needed for expanded digits */
			pow10 = 1;
			for (jdigit = 1; ;jdigit++)
			{
				if (n/pow10 >= 10)
				{
					needed += 9*pow10*jdigit;
					pow10 *= 10;
				}
				else
				{
					needed += (n - pow10 + 1)*jdigit;
					break;
				}
			} /*for (jdigit = 1; ;jdigit++)*/
		} /*if (nroot == 0 || root[0] == NUMERICLABEL){}else{}*/
	} /*if (labels == (char *) 0)*/
	else if (nroot == 0)
	{ /*all labels ""*/
		for (i = 0; i < n; i++)
		{
			labels[i] = '\0';
		}
		needed = n;
	}
	else if (root[0] == NUMERICLABEL)
	{
		/* all labels the same as root */
		place = labels;
		for (i = 0; i < n; i++)
		{
			strcpy(place, root);
			place += nroot + 1;
		} /*for (i = 0; i < n; i++)*/
		needed = place - labels;
	}
	else if (n == 1 && !numbersOnly)
	{
		/* non bracketed length 1; don't expand */
		strcpy(labels, root);
		needed = nroot + 1;
	}
	else	
	{
		char      *head = root;
		char       tail[2];

		/*
		  numbersOnly != 0 if strlen(root) == 1 and root[0] is '#' or one
		  of '[', '(', '{', '<', '/', or '\\'
		*/
		if (numbersOnly && root[0] != pound)
		{
			tail[0] = Rparens[strchr(Lparens,head[0]) - Lparens];
			tail[1] = '\0';
			numbersOnly = 0;
		}
		else
		{
			tail[0] = '\0';
		}
		if (numbersOnly)
		{
			/* root must be "#" so make head and tail both be null */
			head = tail;
		}
		place = labels;
		for (i = 1; i <= n; i++)
		{
			sprintf(place, "%s%ld%s", head, i, tail);
			place = skipStrings(place, 1);
		}
		needed = place - labels;
	}
	return (needed);
			
} /*expandLabels()*/ 

/*
  980807 Fixed bug so that it always clears labels from to unless
         they are the same handle as LABELS(from)
  990322 Fixed bug that occurred when ndims(from) != ndims(to).  It
         assumes min(ndims(from),ndims(to)) dimensions are identical
         and when ndims(to) > ndims(from), the extra dimensions are
         all 1.
*/
long transferLabels(Symbolhandle from, Symbolhandle to)
{
	int       ok = 1;
	WHERE("transferLabels");
	
	if (LABELS(from) != LABELS(to))
	{
		clearLABELS(to);
		if (HASLABELS(from))
		{
			int        ndims1 = NDIMS(from), ndims2 = NDIMS(to);
			long       lengths[MAXDIMS];
			int        i;
			char      *labels[MAXDIMS];

			if (ndims1 == ndims2)
			{
				TMPHANDLE = myduphandle(LABELSHANDLE(from));
			}
			else
			{
				long       needed = 0;
				
				getAllLabels(from, labels, lengths, (long *) 0);

				for (i = 0; i < ndims2; i++)
				{
					needed += (i < ndims1) ? lengths[i] : 2;
				}
				TMPHANDLE = mygethandle(needed);
			}
			ok = (TMPHANDLE != (char **) 0) && setLabels(to, TMPHANDLE);
			if (!ok)
			{
				mydisphandle(TMPHANDLE);
			}
			else if (ndims1 != ndims2)
			{
				char      *place = *TMPHANDLE;
				char       numericLabel[2];
				
				numericLabel[0] = NUMERICLABEL;
				numericLabel[1] = '\0';

				getAllLabels(from, labels, lengths, (long *) 0);
				for (i = 0; i < ndims2; i++)
				{
					if (i < ndims1)
					{
						place = copyStrings(labels[i], place, DIMVAL(from, i+1));
					}
					else
					{
						place = copyStrings(numericLabel, place, 1);
					}
				} /*for (i = 0; i < ndims2; i++)*/
			} /*if (ndims1 != ndims2)*/				
		} /*if (HASLABELS(from))*/
	} /*if (LABELS(from) != LABELS(to))*/
	
	return (ok);
} /*transferLabels()*/

/*
   Routine to copy 1 or 2 labels from generalized matrix from to
   matrix to whose dimensions are set
   if control & USEROWLABELS == 0, row labels are expanded from LEFTBRACKET
   if control & USECOLLABELS == 0, column labels are expanded from LEFTBRACKET
*/

long moveMatLabels(Symbolhandle from, Symbolhandle to, unsigned long control)
{
	long         lengths[2], dims[2];
	long         retVal = 0;
	int          useLabel1 = control & USEROWLABELS;
	int          useLabel2 = control & USECOLLABELS;
	char        *labs[2];
	WHERE("moveMatLabels");

	if (isMatrix(from, dims) && (NDIMS(from) > 2 || !useLabel1 || !useLabel2))
	{
		if (LABELS(from) != LABELS(to)) /* be paranoid*/
		{ 
			clearLabels(to);
		}
		
		getMatLabels(from, labs, lengths);

		if (!useLabel1)
		{
			lengths[0] = expandLabels(LEFTBRACKET, dims[0], (char *) 0);
		}
		if (!useLabel2)
		{
			lengths[1] = expandLabels(LEFTBRACKET, dims[1], (char *) 0);
		}
	
		TMPHANDLE = createLabels(2, lengths);

		setNDIMS(to, 2);
		setDIM(to, 1, dims[0]);
		setDIM(to, 2, dims[1]);

		if (TMPHANDLE == (char **) 0 || !setLabels(to, TMPHANDLE))
		{
			mydisphandle(TMPHANDLE);
			goto errorExit;
		}
		getMatLabels(from, labs, lengths);

		appendLabels(to, (useLabel1) ? labs[0] : LEFTBRACKET, 0,
					 (useLabel1) ? dontExpand : doExpand);

		appendLabels(to, (useLabel2) ? labs[1] : LEFTBRACKET, 1,
					 (useLabel2) ? dontExpand : doExpand);
		retVal = 1;
	}
	else
	{
		retVal = transferLabels(from, to);
	}
/* fall through */	

  errorExit:
	return (retVal);

} /*moveMatLabels()*/

/*
   Routine to make symh a matrix and fix up its labels to match
   if control & USEROWLABELS == 0, row labels are "@"
   if control & USECOLLABELS == 0, column labels are "@"
   
   980811 fixed nasty bug that affected fixupMatLabels when control
          was not USEBOTHLABELS; the replacement labels were LEFTBRACKET which
          required more than the space allocated on expansion.
          Also cleaned up some unnecessary trickery
*/

#define trash   NAMEFORTRASH

long fixupMatLabels(Symbolhandle symh, unsigned long control)
{
	Symbolhandle    tmpsymH = (Symbolhandle) 0;
	Symbolhandle    trash = (Symbolhandle) 0;
	long            dims[2];
	int             useLabel1 = control & USEROWLABELS;
	int             useLabel2 = control & USECOLLABELS;
	WHERE("fixupMatLabels");
	
	if (isMatrix(symh, dims) && (NDIMS(symh) > 2 || !useLabel1 || !useLabel2))
	{ /* Something needs to be done */
		long          lengths[2];
		char         *labs[2];
		char        **labelsH = (char **) 0;
		char          numericLabel[2];

		tmpsymH = Install(SCRATCH, TYPE(symh));
		if (tmpsymH == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		Setdims(tmpsymH, NDIMS(symh), &DIMVAL(symh, 1));

		trash = GarbInstall(1);
		if (trash == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		
		getMatLabels(symh, labs, lengths);
		if (!useLabel1)
		{
			lengths[0] = 2*dims[0];
		}
		if (!useLabel2)
		{
			lengths[1] = 2*dims[1];
		}
	
		TMPHANDLE = createLabels(2, lengths);
		toTrash(TMPHANDLE, 0);
		if (TMPHANDLE == (char **) 0)
		{
			goto errorExit;
		}
		getMatLabels(symh, labs, lengths);
		labelsH = LABELSHANDLE(symh); /*save old labels to restore if required*/
		setLABELSHANDLE(symh, (char **) 0);
		/*
			Put labels somewhere where macros and getMatLabels() will work
		*/
		setLabels(tmpsymH, labelsH);
		unTrash(0); /* TMPHANDLE */
		emptyTrash(); /* not needed any more*/

		Setdims(symh, 2, dims);

		if (!setLabels(symh, TMPHANDLE))
		{ /*Restore dimensions and labels and quit*/ 
			mydisphandle(TMPHANDLE);
			Setdims(symh, NDIMS(tmpsymH), &DIMVAL(tmpsymH, 1));
			setLABELS(symh, LABELS(tmpsymH));
			setLABELS(tmpsymH, (LabelsHandle) 0);
			goto errorExit;
		}

		numericLabel[0] = NUMERICLABEL;
		numericLabel[1] = '\0';

		getMatLabels(tmpsymH, labs, lengths);

		appendLabels(symh, (useLabel1) ? labs[0] : numericLabel,
			0, (useLabel1) ? dontExpand : doExpand);
		appendLabels(symh, (useLabel2) ? labs[1] : numericLabel,
			1, (useLabel2) ? dontExpand : doExpand);
		Removesymbol(tmpsymH);
	} /*if (isMatrix(symh, dims) && (NDIMS(symh) > 2 || !useLabel1 || !useLabel2))*/
	return (1);

  errorExit:
	Removesymbol(tmpsymH);
	emptyTrash();
	return (0);
} /*fixupMatLabels()*/

/*
   Routine to append a vector of labels for coordinate jdim (0 base)
   to the labels already appended to symh.

   If expand != 0, newLabels is a scalar or (char *) 0;  in the latter case
   it is interpreted as meaning ""

   If expand == doExpand, traditional expansion is done, that is "", "@", "@(",
   etc. are replicated and "root" is expanded as "root1","root2"...

   If expand == padExpand, all but the first element in the label will be ""

   It is assumed that LABELS(symh) has sufficient room for labels

   971226 Modified so that expand has more values
*/

void appendLabels(Symbolhandle symh, char *newLabels, long jdim, long expand)
{
	long           i, dimj = DIMVAL(symh, jdim + 1);
	char          *labels = getOneLabel(symh, jdim, 0);

	if (expand != dontExpand)
	{
		if (newLabels == (char *) 0)
		{
			newLabels = NullString;
		}
		if (expand == doExpand)
		{
			expandLabels(newLabels, dimj, labels);
		}
		else
		{ /*padExpand*/
			char      *place = copyStrings(newLabels, labels, 1);
			
			for (i = 1; i < dimj; i++)
			{
				*place++ = '\0';
			}
		}
	} /*if (expand != dontExpand)*/
	else
	{
		copyStrings(newLabels, labels, dimj);
	}
} /*appendLabels()*/

/*
   Append nlabels individual labels starting at index startIndex (base 0) of
   coordinate jdim (base 0)
*/
void appendNLabels(Symbolhandle symh, char *newLabels, long jdim,
				   long startIndex, long nlabels)
{
	if (HASLABELS(symh))
	{
		copyStrings(newLabels, getOneLabel(symh, jdim, startIndex), nlabels);
	}
} /*appendNLabels */

/*
   Install labels specified in symhLabels in result.  It is assumed that
   symhLabels has been checked for appropriateness and that the dimensions
   of result have already been set.
*/

long installLabels(Symbolhandle symhLabels, Symbolhandle result)
{
	Symbolhandle   symhLabel;
	long           needed = 0, nlabels, ncomps, ndims = NDIMS(result);
	long           i;
	WHERE("installLabels");
	
	/* We ignore any extra label components */
	ncomps = (TYPE(symhLabels) == STRUC) ? NCOMPS(symhLabels) : 1;
	nlabels = (ncomps < ndims) ? ncomps : ndims;
	/* 
	   nlabels is the number of coordinate labels supplied we will use
	*/
	for (i = 0; i < nlabels; i++)
	{
		symhLabel = (TYPE(symhLabels) == STRUC) ?
			COMPVALUE(symhLabels, i) : symhLabels;
		if (isScalar(symhLabel))
		{
			needed += expandLabels(STRINGPTR(symhLabel), DIMVAL(result, i+1),
								   (char *) 0);
		}
		else
		{
			long       handleLength = myhandlelength(STRING(symhLabel));

			if (handleLength < 0)
			{
				goto errorExit;
			}
			
			needed += handleLength;
		}
	} /*for (i = 0; i < nlabels; i++)*/

	for (i = nlabels ; i < ndims; i++)
	{ /* fill out missing labels with "@" */
		needed += 2*DIMVAL(result, i + 1);
	} /*for (i = nlabels ; i < ndims; i++)*/

	TMPHANDLE = mygethandle(needed);

	if (TMPHANDLE == (char **) 0 || !setLabels(result, TMPHANDLE))
	{
		mydisphandle(TMPHANDLE);
		goto errorExit;
	}
	
	for (i = 0; i < nlabels; i++)
	{
		symhLabel = (TYPE(symhLabels) == STRUC) ?
					 COMPVALUE(symhLabels, i) : symhLabels;
		appendLabels(result, STRINGPTR(symhLabel), i,
					 (isScalar(symhLabel)) ? doExpand : dontExpand);
	} /*for (i = 0; i < nlabels; i++)*/
	for (i = nlabels; i < ndims; i++)
	{
		appendLabels(result, NumericLabel, i, doExpand);
	} /*for (i = nlabels; i < ndims; i++)*/
	return (1);
	
  errorExit:
	return (0);
} /*installLabels()*/

/*
   Routine to check value of labels:value for appropriateness
   if symLabels is not a structure with CHARACTER vector components
   or a CHARACTER vector, WRONGTYPEVECTOR or WRONGTYPECOMP is returned

   if ndims == 0, only types are checked.  dims is not referenced.

   If the labels are of the wrong sizes, WRONGSIZELABELS is returned; value
   will be ignored

   If there are too many labels but the first ndims are ok, TOOMANYLABELS
   is returned; extra ones will be ignored

   If there are too few labels, but the ones there are ok, TOOFEWLABELS is
   returned; missing ones will be assumed to be ""

   If more than one non-fatal error is found, the error codes are or'ed

   Otherwise, LABELSOK is returned
*/
unsigned long checkLabels(Symbolhandle symhLabels, long ndims, long dims [])
{
	Symbolhandle    symhLabel;
	long            i, retVal, nlabels, ncomps;
	int             checkTypesOnly = (ndims == 0);

	if (TYPE(symhLabels) != STRUC)
	{
		ncomps = 0;
		nlabels = 1;
		retVal = (ndims > 1) ? TOOFEWLABELS : LABELSOK;
	} /*if (TYPE(symhLabels) != STRUC)*/
	else
	{
		ncomps = NCOMPS(symhLabels);
		if (checkTypesOnly || ncomps == ndims)
		{
			retVal = LABELSOK;
			nlabels = ncomps;
		}
		else if (ncomps > ndims)
		{
			retVal = TOOMANYLABELS;
			nlabels = ndims;
		}
		else
		{
			retVal = TOOFEWLABELS;
			nlabels = ncomps;
		}
	} /*if (TYPE(symhLabels) != STRUC){}else{}*/
	
	for (i = 0; i < nlabels; i++)
	{
		symhLabel = (ncomps == 0) ?
			symhLabels : COMPVALUE(symhLabels, i);
		
		if (isNull(symhLabel) || TYPE(symhLabel) != CHAR ||
			!isVector(symhLabel))
		{
			retVal |= (ncomps == 0) ? WRONGTYPEVECTOR : WRONGTYPECOMP;
			goto errorExit;
		}

		if (!checkTypesOnly &&
			!isScalar(symhLabel) && symbolSize(symhLabel) != dims[i])
		{
			retVal |= WRONGSIZELABELS;
		}
	} /*for (i = 0; i < nlabels; i++)*/
	
	return (retVal);
	
  errorExit:
	retVal |= LABELSERROR;
	return (retVal);
} /*checkLabels()*/

/*
   anyLabels(symh) returns 1 if and only if symh or any component
   has labels
*/
long anyLabels(Symbolhandle symh)
{
	if (symh != (Symbolhandle) 0)
	{
		if (HASLABELS(symh))
		{
			return (1);
		}
		else if (TYPE(symh) == STRUC)
		{
			long      i, ncomps = NCOMPS(symh);

			for (i = 0; i < ncomps; i++)
			{
				if (anyLabels(COMPVALUE(symh, i)))
				{
					return (1);
				}
			}
		}
	} /*if (symh != (Symbolhandle) 0)*/
	return (0);
} /*anyLabels()*/

long sizeOfLabels(Symbolhandle symh)
{
	if (!HASLABELS(symh))
	{
		return (0);
	}
#ifdef LABELSARECHAR
	return (myhandlelength(LABELSHANDLE(symh)));
#else /*LABELSARECHAR*/
	return (SizeofSymbol((Symbolhandle) LABELS(symh)));
#endif /*LABELSARECHAR*/
} /*sizeOfLabels()*/

long sizeOfNotes(Symbolhandle symh)
{
	if (!HASNOTES(symh))
	{
		return (0);
	}
#ifdef NOTESARECHAR
	return (myhandlelength(NOTESHANDLE(symh)));
#else /*NOTESARECHAR*/
	return (SizeofSymbol((Symbolhandle) NOTES(symh)));
#endif /*NOTESARECHAR*/
} /*sizeOfNotes()*/

/*
  Functions related to printing labels; called from printSymbol() in
  commonio.c
*/


/*
  calculate the widths of each of the "row" labels of symh.

  If TYPE(symh) == CHAR or if symh has no labels, these are the first
  NDIMS(symh) labels; otherwise they are the first NDIMS(symh) - 1 labels.

  When symh has labels:
   If labels for dimension i are all "", dimLabelsWidth[i] is set to 0

   If labels for dimension i are all identical of the form "@.*",
   TempLabels[i] is set to dimLabels[i] and dimLabelsWidth is calculated
   appropriately.  If the label is "@#", "@[", ... it will be
   expanded when printed similarly to "#", "[", ... except that
   successive identical labels like "@[" or "@(" will be "fused".
   "@" behaves like "@(".

   Otherwise, TempLabels[i] is set to (char *) 0 and dimLabelsWidth[i]
   is set to the maximum size of any label in dimLabels[i]
  When symh does not have labels
   dimLabelsWidth[0] is ndigits + 2, and dimLabelswidth[i] is ndigits + 1,
   i > 0, where ndigits is the maximum digits needed for the dimension.

  The value returned is the total width of the labels, including spaces
  between successive labels if needed.
*/
long prepPrintLabels(Symbolhandle symh,
					   char * dimLabels [], long dimLabelsWidth [])
{
	int          charVar = (TYPE(symh) == CHAR);
	long         labelWidth = 0;
	long         ndims = NDIMS(symh);
	long		 ndims1 = (charVar) ? ndims : ndims - 1;
	long         idim;
	char         pound = Lparens[PoundIndex];
	WHERE("prepPrintLabels");
	
	for (idim = 0; idim < MAXDIMS; idim++)
	{
		TempLabels[idim] = (char *) 0;
		FuseLabels[idim] = (char) 0;
	}

	/* compute the total width of actual labels, excepting those
	   specified by rep("",n) or rep("@.*", n)
	*/

	if (!HASLABELS(symh) && !USECOLLABS)
	{ /* default: all numerical labels of form (3), (1,2), (1,2,3) ... */
		labelWidth = ndims + 1; /* space for '(', ')' and ',', if needed*/
		for (idim = 1;idim <= ndims;idim++)
		{
			labelWidth += nDigits(DIMVAL(symh,idim));
		}
	} /*if (!HASLABELS(symh))*/
	else
	{
		/*
		  getAllLabels() sets dimLabels[i] to a pointer to the label for
		  dimension i+1 and dimLabelsWidth[i] to the maximum width
		  of that label, except dimLabelsWidth[i] = 0 if the the label
		  is rep("",dimi) or rep("@.*", dimi), that is a null label
		  or a label expandable at print time
		*/
		
		if (HASLABELS(symh))
		{
			getAllLabels(symh, dimLabels, (long *) 0, dimLabelsWidth);
		}
		else
		{
			for (idim = 0; idim < ndims; idim++)
			{
				dimLabelsWidth[idim] = 0;
				dimLabels[idim] = NumericLabel;
			}
		}
		
		for (idim = 0; idim < ndims1; idim++)
		{
			if (dimLabelsWidth[idim] == 0 &&
				dimLabels[idim][0] == NUMERICLABEL)
			{
				/* Temporary expandable label of form rep("@.*",n)*/
				int         extra = 0; /*width in addition to digits */
				int         length = strlen(dimLabels[idim]);
				char       *head = LEFTBRACKET;
				/*
					save label pointer for reference by putColumnLabels()
					and putRowLabels()
				*/
				TempLabels[idim] = dimLabels[idim];

				/* now calculate width needed for label, not counting spaces*/
				dimLabelsWidth[idim] = nDigits(DIMVAL(symh,idim+1));
				
				if (length == 1 || length == 2 &&
					(head = strchr(Lparens, dimLabels[idim][1])))
				{
					/*
						length == 1 means "@" which is equivalent to "@("
					*/
					if (length == 2 && *head == pound)
					{ /*"@#", will be printed 1 2 4 3 with extra == 0*/
						/* trailing space except for last */
						if (idim < ndims1 - 1)
						{
							labelWidth++;
						}
					} /*if (*head == pound)*/
					else
					{ /*"@" or "@[", "@(", ...   bracketed labels*/
						extra++; /*leading bracket or ','*/
						FuseLabels[idim + 1] = 
							(idim < ndims1 - 1) &&
							dimLabelsWidth[idim+1] == 0 &&
							strcmp(dimLabels[idim], dimLabels[idim+1]) == 0;
						if (!FuseLabels[idim + 1])
						{
							extra++; /*trailing bracket*/
							if (idim < ndims1 - 1)
							{
								labelWidth++; /* trailing space*/
							}
						}
					}/*if (length == 2 && *head == pound){}else{}*/
				}
				else
				{
					extra = length - 1;
					if (idim < ndims1 - 1)
					{
						labelWidth++; /* trailing space*/
					}
				}
				dimLabelsWidth[idim] += extra;
			} /*if (dimLabelsWidth[idim] == 0 && dimLabels[idim][0] == NUMERICLABEL)*/
			else if (dimLabelsWidth[idim] > 0 && idim < ndims1 - 1)
			{ /* ordinary label */
				labelWidth++; /* trailing space*/
			}
			labelWidth += dimLabelsWidth[idim];
		} /*for (idim = 0; idim < ndims1; idim++)*/

		/* do something for column label */
		if (!charVar && dimLabelsWidth[ndims1] == 0 &&
			dimLabels[ndims1][0] == NUMERICLABEL)
		{
			/* don't calculate dimLabelsWidth[ndims1] */
			TempLabels[ndims1] = dimLabels[ndims1];
			dimLabels[ndims1] = (char *) 0;
		}
	} /*if (!HASLABELS(symh)){}else{}*/
	return (labelWidth);
} /*prepPrintLabels()*/

/*
  print labels for the last dimension across the top of the output
*/

void putColumnLabels(FILE * fp, long lastdim, char * labels, long labelWidth,
					 long fieldWidth, long nperLine,
					 long type)
{
	char         *outstr, *place = labels;
	char          buffer[250], *pbuffer = buffer;
	int           leftAdjust = (type == CHAR || type == LOGIC) ? 1 : 0;
	int           numLabel, bracketLabel;
	long          length;
	char         *fmt = (leftAdjust) ? "%-*s" : "%*s";
	char          tempTail[2], tempHead[2];
	char          pound = Lparens[PoundIndex];
	char         *head = tempHead, *tail = tempTail;
	long          j;     
	long          ndims;
	WHERE("putColumnLabels");
	
	tempTail[1] = tempHead[1] = '\0';
	
	if (labels == (char *) 0)
	{
		/*
			expandable label; find original in TempLabels[]
		*/

		for (ndims = MAXDIMS; ndims > 0 && TempLabels[ndims-1] == (char *) 0;
			 ndims--)
		{
			;
		}
		labels = TempLabels[ndims-1];
	} /*if (labels == (char *) 0)*/
	else
	{
		ndims = 0;
	}
	length = strlen(labels);
	labelWidth += leftAdjust;

	numLabel = (ndims > 0);
	bracketLabel = numLabel &&
		(length == 1 || length == 2 && strchr(Lparens, labels[1]));

	if (bracketLabel)
	{
		if (length == 1)
		{ /* "@", treat as "@(" */
			tempHead[0] = LEFTBRACKET[0];
			tempTail[0] = RIGHTBRACKET[0];
		}
		else if(length == 2 && labels[1] == pound)
		{ /*"@#" */
			tempHead[0] = tempTail[0] = '\0';
		}
		else
		{ /* "@[", "@(", .... */
			tempHead[0] = labels[1];
			tempTail[0] = Rparens[strchr(Lparens, labels[1]) - Lparens];
		}
	} /*if (bracketLabel)*/
	else if (numLabel)
	{ /* other label expandable at print time */
		head = labels + 1;
		tail = NullString;
	}
		
	place = labels;
	outstr = OUTSTR;
	
	for (j = 1; j <= lastdim; j++)
	{
		/* add leading spaces at start of line */
		if ((nperLine == 1 || (j % nperLine == 1)) &&
			labelWidth > leftAdjust)
		{
			sprintf(outstr, "%*s", (int) labelWidth, NullString);
			outstr += labelWidth;
		}
		if (numLabel)
		{
			/* last dimension was label to be expanded with index number */
			sprintf(buffer, "%s%ld%s", head, j, tail);
		}
		else
		{
			pbuffer = place;
			if (j < lastdim)
			{
				place = skipStrings(place, 1);
			}
		}
		
		sprintf(outstr, fmt, (int) fieldWidth, pbuffer);
		outstr += strlen(outstr);

		if (j % nperLine == 0 && j < lastdim)
		{
			fmyprint(OUTSTR, fp);
			fmyeol(fp);
			outstr = OUTSTR;
		}
	} /*for (j = 1; j <= lastdim; j++)*/
	fmyprint(OUTSTR, fp);
	fmyeol(fp);

} /*putColumnLabels()*/

/*
   Output labels at start of line.  Labels are printed if coord[ndims-1]
   == 1; otherwise labelWidth spaces are put out.

   labels == (char **) 0 => classic all numerical labels
   labelsWidth[j] > 0  && labels[j] != TempLabels[j] => regular label
   labelsWidth[j] > 0  && labels[j] == TempLabels[j] => numerical label
   labelsWidth[j] == 0                               => blank 0 width label
   Successive numerical labels are "fused" giving labels of the form,
   e.g., (1,2); isolated numeric labels have form, e.g., (2)
*/
void putRowLabels(FILE * fp, long ndims, long coord [], long labelWidth,
				  char *labels[], long *labelsWidth, long charData)
{
	char         *outstr = OUTSTR, *label;
	long          j, margin;
	long          ndims1 = (charData) ? ndims : ndims - 1;
	char         *tail, *head, pound = Lparens[PoundIndex];
	char          tempHead[2], tempTail[2];
	WHERE("putRowLabels");
	
	head = tempHead;
	tail = tempTail;
	tempHead[1] = tempTail[1] = OUTSTR[0] = '\0';

	if (ndims > 0)
	{
		if (labels == (char **) 0)
		{
			/* purely numerical labels all at left*/
			tempHead[0] = LEFTBRACKET[0];
			tempTail[0] = ',';
			for (j = 0; j < ndims; j++)
			{
				if (j == ndims - 1)
				{
					tempTail[0] = RIGHTBRACKET[0];
				}
				sprintf(outstr, "%s%ld%s", head, coord[j], tail);
				outstr += strlen(outstr);
				tempHead[0] = '\0';
			}
		} /*if (labels == (char **) 0)*/
		else if (coord[ndims-1] > 1 && !charData)
		{ /* not first line or "row", put out space */
			sprintf(outstr, "%*s", (int) labelWidth, NullString);
		}
		else
		{
			/* 1st line of "row"; put out labels */
			long         j1, k;

			for (j = 0; j < ndims1; j++)
			{
				if (labelsWidth[j] > 0)
				{
					char      buffer[250];
					char      leftBracket, rightBracket;
					int       length = strlen(labels[j]);
					int       width, bracketLabel, numLabel;

					numLabel = labels[j] == TempLabels[j];

					leftBracket = labels[j][1];
					bracketLabel = numLabel &&
						(length == 1 || length == 2 &&
						leftBracket != pound && strchr(Lparens, leftBracket));
					if (bracketLabel)
					{
						if (length == 1)
						{
							leftBracket = LEFTBRACKET[0];
							rightBracket = RIGHTBRACKET[0];
						}
						else
						{
							rightBracket =
								Rparens[strchr(Lparens, leftBracket) - Lparens];
						}
					} /*if (bracketLabel)*/
					else
					{
						leftBracket = rightBracket = '\0';
					}
					
						
					/* check to see if we should print this label */
					if (bracketLabel)
					{
						/*
						  bracketed label; bypass immediately trailing
						  identical bracketed labels, if any
						*/
						for (j1 = j+1; FuseLabels[j1]; j1++)
						{
							;
						}
					}
					else
					{
						j1 = j + 1;
					}
					
					/*
					  put out label only if this label and all following
					  are numeric or if all coordinates starting
					  with the next non-matching bracketed label (label
					  j1) are 1

					  j1 is either ndims1 (this and all following are
					  matching bracketed numeric) , j + 1, if this label
					  is not bracketed numeric, or the index of the next
					  following not matching label if this label is bracketed
					  numeric
					*/
					for (k = j1; k < ndims1; k++)
					{
						if (coord[k] > 1)
						{
							break;
						}
					} /*for (k = j1; k < ndims1; k++)*/

					width = labelsWidth[j];

					if (k >= ndims1)
					{
						/* no later coordinates > 1 => OK to print */
						if (!numLabel)
						{
							/* non expandable label */
							label = skipStrings(labels[j], coord[j] - 1);
						} /*if (!numLabel)*/
						else if (!bracketLabel)
						{
							/* "@.*" but none of "@", "@[", "@(", ... */
							head = labels[j] + 1;
							if (length == 2 && *head == pound)
							{
								head = NullString;
							}
							sprintf(buffer,"%s%ld",head,coord[j]);
							label = buffer;
						}
						else
						{
							/*
							  Must be bracketed numeric label
							  Put it in buffer, fused together with
							  immediately following identical such labels
							*/
							width = 0;
							tempHead[0] = leftBracket;
							tempTail[0] = ',';
							head = tempHead;
							tail = tempTail;
							label = buffer;
							
							for (j1 = j; j1 < ndims1 && tail[0] == ',';
								 j1++)
							{
								if (!FuseLabels[j1+1])
								{
									tail[0] = rightBracket;
								} 
								sprintf(label, "%s%ld%s", head, coord[j1], tail);
								head[0] = '\0';
								label += strlen(label);
								width += labelsWidth[j1];
							} /*for (j1 = j; j1 < ndims1 && tail == ','; j1++)*/
							j = j1 - 1;
							label = buffer;
						} /*if (labels[j] != (char *) 0){}else{}*/
					} /*if (k >= ndim1)*/
					else
					{
						/* don't put out label */
						label = NullString;
					}
					if (j < ndims1 - 1 && (k >= ndims1 || !FuseLabels[j+1]))
					{
						/* space before next label */
						width++;
					}
					sprintf(outstr, "%-*s", width, label);
					outstr += strlen(outstr);
				} /*if (labelsWidth[j] > 0)*/			
			} /*for (j = 0; j < ndims1; j++)*/
		}
	} /*if (ndims > 0)*/
	margin = labelWidth - strlen(OUTSTR);

	while (margin-- > 0)
	{
		fmyprint(" ",fp);
	}

	fmyprint(OUTSTR,fp);

	if (charData)
	{
		fmyprint(" ", fp);
	}

	*OUTSTR = '\0';

} /*putLabels()*/

/*
   MacAnova functions getlabels() and getnotes()

   Usage: getlabs(a [,silent:T]) returns structure (or vector) of labels
   for all dimensions of a
          getlabs(a,dims [,silent:T]), dims a vector of positive integers
   returns a structure or vector of the labels for the specified
   dimensions of a.  If silent:T is present, no warning is printed if there
   are no labels.

         getnotes(a [,silent:T])

   961022 Forced error exit on unrecognized keyword

   980723 Added getnotes()

   980806 Added keyword phrase trim:T or F to getlabels().  If T, 
          label vectors all of whose elements are identical and either
          start with '@' and are "", will be returned as scalars.
*/

Symbolhandle getlabs(Symbolhandle list)
{
	Symbolhandle        arg, result = (Symbolhandle) 0, symh;
	Symbolhandle        symhDims = (Symbolhandle) 0;
	Symbolhandle        symhKey = (Symbolhandle) 0;
	long                nargs = NARGS(list);
	long                i, ndims, ncomps;
	long                dimi;
	long                silent = 0, trim = 1;
	int                 getLabels = strcmp(FUNCNAME, "getlabels") == 0;
	int                 margs = (getLabels) ? 4 : 2;
	char               *keyword;
	WHERE("getlabs");
	
	if (nargs > margs)
	{
		badNargs(FUNCNAME, -margs);
		goto errorExit;
	} /*if (nargs > margs)*/

	arg = COMPVALUE(list, 0);
	if (!argOK(arg, 0, (nargs == 1) ? 0 : 1))
	{
		goto errorExit;
	}

	while (nargs > 1 &&
		   (keyword = isKeyword(symhKey = COMPVALUE(list, nargs-1))))
	{
		if (strcmp(keyword, "silent") == 0)
		{
			if (!isTorF(symhKey))
			{
				notTorF(keyword);
				goto errorExit;
			}
			silent = (DATAVALUE(symhKey, 0) != 0.0);
		}
		else if (getLabels && strcmp(keyword, "trim") == 0)
		{
			if (!isTorF(symhKey))
			{
				notTorF(keyword);
				goto errorExit;
			}
			trim = (DATAVALUE(symhKey, 0) != 0.0);
		}
		else
		{
			badKeyword(FUNCNAME, keyword);
			goto errorExit;
		}
		nargs--;
	} /*while (nargs > 1 && (keyword = isKeyword(symhKey = COMPVALUE(nargs-1))))*/

	if (getLabels)
	{
		if (!HASLABELS(arg))
		{
			result = NULLSYMBOL;
			if (!silent)
			{
				sprintf(OUTSTR,
						"WARNING: argument to %s() has no labels", FUNCNAME);
				putErrorOUTSTR();
			}
		} /*if (!HASLABELS(arg))*/
		else
		{
			char               *labels[MAXDIMS];
			long                lengths[MAXDIMS];
			long                nLabels;
			ndims = NDIMS(arg);

			if (nargs == 2)
			{
				double      value;

				symhDims = COMPVALUE(list, 1);
				if (!argOK(symhDims, 0, 2))
				{
					goto errorExit;
				}

				ncomps = symbolSize(symhDims);
				if (TYPE(symhDims) == REAL && isVector(symhDims))
				{
					for (i = 0; i < ncomps; i++)
					{
						value = DATAVALUE(symhDims, i);
						if (value != floor(value) || value < 1 || value > ndims)
						{
							break;
						}
					} /*for (i = 0; i < ncomps; i++)*/				
				} /*if (TYPE(symhDims) == REAL && isVector(symhDims))*/
				else
				{
					i = -1;
				}
			
				if (i < ncomps)
				{
					sprintf(OUTSTR,
							"ERROR: arg2 to %s not vector of positive integers <= ndims(arg1)",
							FUNCNAME);
					goto errorExit;
				} /*if (i < ncomps)*/
			} /*if (nargs == 2)*/
			else
			{
				ncomps = ndims;
			} /*if (nargs == 2){}else{}*/
		
			result = (ncomps > 1) ?
			  StrucInstall(SCRATCH, ncomps) : CInstall(SCRATCH, 0);
			if (result == (Symbolhandle) 0)
			{
				goto errorExit;
			}

			getAllLabels(arg, labels, lengths, (long *) 0);

			for (i = 0; i < ncomps; i++)
			{
				long        j;

				dimi = (symhDims == (Symbolhandle) 0) ? i :
				  ((long) DATAVALUE(symhDims, i) - 1);

				nLabels = DIMVAL(arg, dimi + 1);
				
				if (ncomps == 1)
				{
					symh = result;
				}
				else
				{
					symh = Makesymbol(CHAR);
					COMPVALUE(result, i) = symh;
					if (symh == (Symbolhandle) 0)
					{
						goto errorExit;
					}
					sprintf(NAME(symh),"dim%ld",dimi + 1);
					/*
					  Get pointers to labels again since we have
					  allocated storage
					*/
					getAllLabels(arg, labels, (long *) 0, (long *) 0);
				}
			
				if (trim && (labels[dimi][0] == NUMERICLABEL ||
							 labels[dimi][0] == '\0'))
				{ /* may be label which can be trimmed */
					char        *theseLabels = labels[dimi];

					for (j = 1; j < nLabels; j++)
					{
						/* compare first label with remaining labels*/
						theseLabels = skipStrings(theseLabels, 1);
						if (strcmp(theseLabels, labels[dimi]) != 0)
						{
							break;
						}
					} /*for (j = 1; j < nLabels; j++)*/

					if (j == nLabels)
					{ /* labels are the same and are "" or start with '@'*/
						nLabels = 1;
						/* lengths[dimi] < 0 indicates trimmed */
						lengths[dimi] = -((long) strlen(labels[dimi]) + 1);
					}
				} /*if (trim && labels[dimi][0] == NUMERICLABEL)*/ 

				TMPHANDLE = mygethandle(labs(lengths[dimi]));
				setSTRING(symh, TMPHANDLE);
				if (TMPHANDLE == (char **) 0)
				{
					goto errorExit;
				}
				setNDIMS(symh, 1);
				setDIM(symh, 1, nLabels);
				/*
				  Get pointers to labels again since we have
				  allocated storage
				*/
				getAllLabels(arg, labels, (long *) 0, (long *) 0);
			} /*for (i = 0; i < ncomps; i++)*/

			for (i = 0; i < ncomps; i++)
			{
				symh = (ncomps == 1) ? result : COMPVALUE(result, i);
				dimi = (symhDims == (Symbolhandle) 0) ? i :
				  ((long) DATAVALUE(symhDims, i) - 1);
				nLabels = (lengths[dimi] > 0) ? DIMVAL(arg, dimi+1) : 1;
				copyStrings(labels[dimi], STRINGPTR(symh), nLabels);
			} /*for (i = 0; i < ndims; i++)*/
		} /*if (!HASLABELS(arg)){}else{}*/
	} /*if (getLabels)*/
	else
	{
		if (!HASNOTES(arg))
		{
			result = NULLSYMBOL;
			if (!silent)
			{
				sprintf(OUTSTR,
						"WARNING: argument to %s() has no notes", FUNCNAME);
				putErrorOUTSTR();
			}
		} /*if (!HASNOTES(arg))*/
		else
		{
			char     **notesHandle = NOTESHANDLE(arg);
			
			result = CInstall(SCRATCH, 0);
			if (result == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			setNDIMS(result, 1);
			setDIM(result, 1, DIMVAL(NOTES(arg), 1));
			if (!isscratch(NAME(arg)))
			{
				setSTRING(result, myduphandle(notesHandle));
			}
			else
			{
				setNOTESHANDLE(arg, (char **) 0);
				setSTRING(result, notesHandle);
			}
		} /*if (!HASNOTES(arg)){}else{}*/		
	} /*if (getLabels){}else{}*/

	return (result);
	
  errorExit:
	putErrorOUTSTR();
	Removesymbol(result);
	return (0);
} /*getlabs()*/

/*
  980726
  setlabels(arg, labels [, silent:T])
  attachnotes(arg, notes)
  appendnotes(arg, notes)

  arg must be an existing symbol.  For setlabels() it must be a vector,
  matrix or array or structure; for {attach,append}notes() it can also be
  a GRAPH variable or macro.

  These all return NULLSYMBOL.
    setlabels(arg, NULL) removes labels
    attachnotes(arg, NULL) removes notes
	appendnotes(arg, NULL) does nothing
*/

Symbolhandle setlabs(Symbolhandle list)
{
	Symbolhandle  target, attachment, symhTemp = (Symbolhandle) 0;
	long          nargs = NARGS(list);
	long          type1, type2;
	int           silent = 0;
	int           doLabels = strcmp(FUNCNAME, "setlabels") == 0;
	int           doNotes = !doLabels;
	int           append = (FUNCNAME[1] == 'p');
	char         *what = (doLabels) ? "labels" : "notes";
	WHERE("setlabs");

	if (nargs != 2)
	{
		int           badnargs = 0;

		if (doNotes)
		{
			badnargs = 2;
		}
		else if (nargs != 3)
		{
			badnargs = (nargs > 3) ? -3 : -1002;
		}
		if (badnargs)
		{
			badNargs(FUNCNAME, badnargs);
			goto errorExit;
		}
	} /*if (nargs != 2)*/
	
	target = COMPVALUE(list, 0);
	attachment = COMPVALUE(list, 1);

	if (!argOK(target, NULLSYM, 1) || !argOK(attachment, NULLSYM, 2))
	{
		goto errorExit;
	}
	
	type1 = TYPE(target);
	type2 = TYPE(attachment);
	
	if (type1 != REAL && type1 != LOGIC && type1 != CHAR && type1 != STRUC &&
		(doLabels || type1 != PLOTINFO && type1 != MACRO))
	{
		badType(FUNCNAME, -type1, 1);
		goto errorExit;
	}

	if (nargs == 3)
	{
		Symbolhandle  symhKey = COMPVALUE(list, 2);
		char         *keyword = isKeyword(symhKey);
		char         *keyname = "silent";
		
		if (keyword == (char *) 0 || strcmp(keyword, keyname) != 0)
		{
			sprintf(OUTSTR,
					"ERROR: argument 3 to %s() must be '%s:T'",
					FUNCNAME, keyname);
			goto errorExit;
		}
		if (!isTorF(symhKey))
		{
			notTorF(keyword);
			goto errorExit;
		}
		silent = (DATAVALUE(symhKey, 0) != 0.0);
	} /*if (nargs == 3)*/
	
	if (isSpecial(target))
	{
		sprintf(OUTSTR,
				"ERROR: you can't attach %s to special variable %s\n",
				what, NAME(target));
		goto errorExit;
	}
	
	if (isscratch(NAME(target)))
	{
		sprintf(OUTSTR,
				"ERROR: you can't attach %s to an expression or function result",
				what);
		goto errorExit;
	}
	
	if (type2 == NULLSYM)
	{
		if (doLabels)
		{
			clearLabels(target);
		}
		else if (!append)
		{
			clearNotes(target);
		}
	} /*if (type2 == NULLSYM)*/
	else if (doNotes)
	{
		long         length, nlines1, nlines2;
		char        *place;
		
		if (type2 != CHAR || !isVector(attachment))
		{
			sprintf(OUTSTR,
					"ERROR: notes to be %s by %s() must be a CHARACTER vector",
					(append) ? "added" : "attached", FUNCNAME);
			goto errorExit;
		}

		append = append && HASNOTES(target);
		
		length = myhandlelength(STRING(attachment));

		if (length < 0)
		{
			goto errorExit;
		}

		/* temporary symbol to attach notes to so as to avoid memory leaks*/
		symhTemp = Install(SCRATCH, type1);
		if (symhTemp == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		
		if (append)
		{
			long         length1 = myhandlelength(NOTESHANDLE(target));

			if (length1 < 0)
			{
				goto errorExit;
			}
			length += length1;
			nlines1 = DIMVAL(NOTES(target), 1);
		} /*if (append)*/
		else
		{
			nlines1 = 0;
		}
		nlines2 = DIMVAL(attachment, 1);
		
		if (append || !isscratch(NAME(attachment)))
		{
			/* can't reuse STRING(attachment)*/
			TMPHANDLE = mygethandle(length);
		}
		else
		{ /* reuse STRING(attachment) */
			TMPHANDLE = STRING(attachment);
			setSTRING(attachment, (char **) 0);
		}
		
		if (TMPHANDLE == (char **) 0 || !setNotes(symhTemp, TMPHANDLE))
		{
			mydisphandle(TMPHANDLE);
			goto errorExit;
		}
		
		place = *TMPHANDLE;

		if (append)
		{
			place = copyStrings(NOTESPTR(target), place, nlines1);
		}

		if (append || !isscratch(NAME(attachment)))
		{
			place = copyStrings(STRINGPTR(attachment), place, nlines2);
		}
		setDIM(NOTES(symhTemp), 1, nlines1 + nlines2);

		if (HASNOTES(target))
		{
			clearNotes(target);
		}

		TMPHANDLE = (char **) NOTES(symhTemp);
		setNOTES(symhTemp, (NotesHandle) 0);
		setNOTES(target, (NotesHandle) TMPHANDLE);
	} /*if (type2 == NULLSYM){}else if (doNotes){}*/
	else
	{
		unsigned long  labelError = checkLabels(attachment, NDIMS(target),
											&DIMVAL(target, 1));

		labelError |=  (labelError & WRONGSIZELABELS) ? LABELSERROR : 0;
		
		if (labelError & LABELSERROR || labelError != LABELSOK && !silent)
		{
			badLabels(labelError);
		}
		if (labelError & LABELSERROR)
		{
			goto errorExit;
		}
	
		symhTemp = Install(SCRATCH, type1);
		if (symhTemp == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		Setdims(symhTemp, NDIMS(target), &DIMVAL(target, 1));
		if (!installLabels(attachment, symhTemp))
		{
			goto errorExit;
		}
		if (HASLABELS(target))
		{
			clearLabels(target);
		}
		TMPHANDLE = (char **) LABELS(symhTemp);
		setLABELS(symhTemp, (LabelsHandle) 0);
		setLABELS(target, (LabelsHandle) TMPHANDLE);
	} /*if (type2 == NULLSYM){}else if (doNotes){}else{}*/

	Removesymbol(symhTemp);
	return(NULLSYMBOL);
	
  errorExit:
	putErrorOUTSTR();
	Removesymbol(symhTemp);

	return (0);
} /*setlabs()*/
