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
#pragma segment Readdata
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


/*
   960131  added limited CHAR reading capabilit
   990212  changed putOUTSTR() to putErrorOUTSTR()
*/
#include <stdio.h>
#include "matdat.h"
#include "globals.h"

#if defined(MACINTOSH) && !defined(MACIFACEH__)
#include "macIface.h"
#endif /*MACINTOSH && !MACIFACEH__*/

#define MIN(a, b) (((a) < (b)) ? (a) : (b) )
#define GETLINE1(F) (status != EOF && (status = fillLINE(F), (*LINE)[0]))

#define CHARCHUNK  10

/*
   Check whether line has anything in it
*/

static short isAllWhite(char * line)
{
	long n = MAXLINE;

	while (*line != '\0' && n-- > 0)
	{
		if (*line != SP && *line != TAB && *line != NL && *line != CR)
		{
			return (0);
		}
		line++;
	}
	return (1);
} /*isAllWhite()*/

/*
	decode fields in LINE, returning the number of fields successfully
	decoded by sscanf
*/

static long readLine(char * fmt,long nSkip, double * x)
{
	long         nRead;
	WHERE("readLine");

#ifndef lint /* lint crashes on the following (too many args) */
	nRead = (isAllWhite(*LINE)) ? 0 : sscanf(*LINE+nSkip,fmt,
		x   ,x+ 1,x+ 2,x+ 3,x+ 4,x+ 5,x+ 6,x+ 7,x+ 8,x+ 9,
		x+10,x+11,x+12,x+13,x+14,x+15,x+16,x+17,x+18,x+19,
		x+20,x+21,x+22,x+23,x+24,x+25,x+26,x+27,x+28,x+29,
		x+30,x+31,x+32,x+33,x+34,x+35,x+36,x+37,x+38,x+39,
		x+40,x+41,x+42,x+43,x+44,x+45,x+46,x+47,x+48,x+49);
#else /*lint*/
	nRead = (isAllWhite(*LINE)) ? 0 : sscanf(*LINE+nSkip,fmt,
		x   ,x+ 1,x+ 2,x+ 3,x+ 4,x+ 5,x+ 6,x+ 7,x+ 8,x+ 9,
		x+10,x+11,x+12,x+13,x+14,x+15,x+16,x+17,x+18,x+19,
		x+20,x+21,x+22,x+23,x+24,x+25,x+26,x+27,x+28,x+29,
		x+30,x+31,x+32,x+33,x+34,x+35,x+36,x+37,x+38,x+39);
#endif /*lint*/
	return (nRead);
} /*readLine()*/


static long allocateRoom(Symbolhandle symh, long needed, long nitems,
						 long chunk)
{
	long           allocated = needed + labs(nitems)*chunk;
	WHERE("allocateRoom");
	
	TMPHANDLE = (nitems < 0) ?
		mygethandle(allocated) : mygrowhandle(STRING(symh),allocated);

	setSTRING(symh, TMPHANDLE);
	if (TMPHANDLE == (char **) 0)
	{
		allocated = 0;
	}
	return (allocated);
} /*allocateRoom()*/

#define stepUsed(N) \
		{\
			if (++used >= allocated)\
			{\
				if ((allocated = allocateRoom(symhy,used,N,CHARCHUNK)) == 0)\
					goto cantAllocate;\
				yc = STRINGPTR(symhy);\
			}\
		}

/*
  readFile() can handle arbitrary dimensioned arrays
  980723 made some arguments ints
  980811 Set (*LINE)[0] to '\0' before returning to indicate it's been used
*/

long readFile(FILE * file, int nFields, char * fmt,
			  int nSkip, int byRows, int quoted,
			  Symbolhandle symhy, int echo, int warning)
{
	double        *y;
	char          *yc, c;
	long           i, j, k, l, cplace;
	long           type = TYPE(symhy);
	long           iRowOrCol;
	long           nRead, nExpected, mExpected, nLeft;
	long           nToRead, readSome, itemsLeft;
	long           nRows, nCols;
	long           ndims, size, howmany, status = 1, stepIn, stepOut;
	long           dims[MAXDIMS];
	long           incs[MAXDIMS];
	long           ii[MAXDIMS];
	long           place[MAXDIMS];
	long           allocated = 0, needed, used, charmode = 0, length;
	long           chunk = CHARCHUNK;
	long           done = 0, foundStuff = 0;
	long           noMore = 0;
	static         double x[MAXN];
	WHERE("readFile");

	/*
	  it is assumed that first line of data is already in LINE
	  readLine scans LINE, fillLINE refills it
	  */
/* 
  mExpected is number possibly multiline 'lines' expected in the file
  nExpected is number items expected per 'line'
*/
	
	ndims = NDIMS(symhy);
	size = 1;
	for (i = 0;i < ndims;i++)
	{
		dims[i] = DIMVAL(symhy, i + 1);
		size *= dims[i];
	}

	if (type != CHAR)
	{
		TMPHANDLE = mygethandle(size * sizeof(double));
		setDATA(symhy, (double **) TMPHANDLE);
		if (TMPHANDLE == (char **) 0)
		{
			goto cantAllocate;
		}
		y = DATAPTR(symhy);
	} /*if (type != CHAR)*/
	else
	{
		allocated = allocateRoom(symhy, 0, -size, chunk);
		if (allocated == 0)
		{
			goto cantAllocate;
		}
		
		used = 0;
		if (quoted)
		{
			charmode = BYQUOTEDFIELDS;
		}
		else if (fmt[0] != '\0')
		{
			charmode = BYFIELDS;
		}
		else
		{
			charmode = BYLINES;
		}
		itemsLeft = size;
	} /*if (type != CHAR){}else{}*/

	nCols = (ndims > 1) ? dims[ndims-1] : 1;
	nRows = (ndims > 1) ? size/nCols : dims[0];
	
	nExpected = (byRows) ? nCols : nRows;
	mExpected = (byRows) ? nRows : nCols;

	if (!charmode)
	{
		if (byRows)
		{
			incs[0] = size;
			for (j = 0;j < ndims;j++)
			{
				ii[j] = 0;
				place[j] = 0;
				incs[j+1] = incs[j]/dims[ndims-j-1];
			} /*for (j = 0;j < ndims;j++)*/
		} /*if (byRows)*/
	
		for (i = 0;i < size;i++)
		{
			setMissing(y[i]);
		}
	} /*if (!charmode)*/
	
	l = howmany = 0;

	for (iRowOrCol = 0;iRowOrCol < mExpected;iRowOrCol++)
	{
		if (iRowOrCol > 0 && (noMore = !GETLINE1(file)))
		{
			break;
		}
		if (echo)
		{
			echoLINE();
		}
		nLeft = nExpected;
		readSome = 0;
		while (nLeft > 0)
		{
			nToRead = MIN(nLeft, nFields);

			if (!charmode)
			{
				nRead = readLine(fmt, nSkip, x);
			/*
			  stop reading on empty line or line with non-numeric first field
			  */
			
				if (warning && ((nRead > 0 && nRead < nToRead) ||
					(nRead <= 0 && readSome)))
				{
					sprintf(OUTSTR,
							"WARNING: unreadable or missing item in %s %ld set to MISSING",
							(byRows || ndims == 1)? "case" : "column",
							(ndims == 1) ? howmany + nRead + 1 : iRowOrCol+1);
					putErrorOUTSTR();
				}
				if (nRead <= 0)
				{
					break;
				}
				for (j = nRead;j < nToRead;j++)
				{
					setMissing(x[j]);
				}
				howmany += nToRead;
				for (k = 0; k < nToRead; k++)
				{
					if (byRows)
					{
	/*
	   Put data read in y in in transposed order
	   The following is essentially equivalent to ndims nested for loops
	*/
						y[l] = x[k];
						for (j = 0; j < ndims; j++)
						{
							if (ii[j] == 0)
							{
								place[j] = l;
							}
							if ((ii[j] += incs[j+1]) < incs[j])
							{
								l += incs[j+1];
								break;
							}
							ii[j] = 0;
							l = place[j];
						} /*for (j = 0; j < ndims; j++)*/
					} /*if (byRows)*/
					else
					{
						y[l++] = x[k];
					}
				} /*for (k = 0; k < nToRead; k++)*/
			} /*if (!charmode)*/
			else
			{ /* Reading CHARACTER variable */
				length = strlen(*LINE);
				if ((*LINE)[length-1] == '\n')
				{
					(*LINE)[--length] = '\0';
				}
				if (nSkip > 0 && length < nSkip)
				{
					(*LINE)[nSkip] = '\0';
				}
				cplace = nSkip;

				switch (charmode)
				{
				  case BYLINES:
/*
   Each line is one item.  If a line ends with backslash, it is taken to
   escape the ending newline and the item is continued to another line
*/
					for (nRead = 0;nRead < nToRead ; /*empty*/)
					{
						length = strlen(*LINE + nSkip);
						needed = used + length + 1;
						if (needed > allocated)
						{
							/*compute average size of previous items*/
							chunk = (needed - 1)/(size - itemsLeft + 1) + 1;
							allocated = allocateRoom(symhy, needed,
													 itemsLeft - 1, chunk);
							if (allocated == 0)
							{
								goto cantAllocate;
							}
						} /*if (needed > allocated)*/
						yc = STRINGPTR(symhy);
					
						strcpy(yc + used, *LINE);
						used += length + 1;
						if (yc[used-2] == '\\')
						{ /*may be escaped newline*/
							done = (length > 1 && yc[used-3] == '\\');
							yc[used-2] = (done) ? '\0' : '\n';
							used--;
						} /*if (yc[used-2] == '\\')*/
						else
						{ /* completed reading item */
							done = 1;
						}
						
						if (done)
						{
							howmany++;
							itemsLeft--;
							nRead++;
						}
						if (!done || nRead < nToRead)
						{
							if ((noMore = !GETLINE1(file)))
							{
								done = 1;
								break;
							}
							length = strlen(*LINE);
							if ((*LINE)[length-1] == '\n')
							{
								(*LINE)[--length] = '\0';
							}
							if (nSkip > 0 && length < nSkip)
							{
								(*LINE)[nSkip] = '\0';
							}
							if (echo)
							{
								echoLINE();
							}
							cplace = nSkip;
						} /*if (!done || nRead < nToRead)*/
					} /*for (nRead = 0;nRead < nToRead ; )*/
					break; /* from switch */
					
				  case BYFIELDS:
/*
   Items are white space delimited fields; nothing is escaped; only one
   line is scanned for nToRread fields
*/

					for (nRead = 0;nRead < nToRead ; nRead++)
					{
						needed = used + strlen(*LINE + cplace) + 1;
						if (needed > allocated)
						{
							/*compute average size of previous items*/
							chunk = (needed - 1)/(size - itemsLeft + 1) + 1;
							allocated = allocateRoom(symhy, needed,
													 itemsLeft - 1, chunk);
							if (allocated == 0)
							{
								goto cantAllocate;
							}
						} /*if (needed > allocated)*/
						yc = STRINGPTR(symhy);
						stepIn = copyField(*LINE + cplace, yc + used);
						stepOut = strlen(yc + used);
						
						if (stepIn == 0)
						{ /* nothing not white found*/
							break;
						}
						howmany++;
						itemsLeft--;
						cplace += stepIn;
						used += stepOut + 1;
					} /*for (nRead = 0;nRead < nToRead ; nRead++)*/

					if (nRead < nToRead)
					{
						if (warning)
						{
							sprintf(OUTSTR,
									"WARNING: missing fields in %s %ld set to \"\"",
									(byRows || ndims == 1)? "case" : "column",
									(ndims == 1) ?
									howmany + nRead + 1 : iRowOrCol+1);
							putErrorOUTSTR();
						} /*if (warning)*/
						
						needed = used + (nToRead - nRead);
						itemsLeft -= (nToRead - nRead);
						howmany += (nToRead - nRead);
						if (needed > allocated)
						{
							/*compute average size of previous items*/
							chunk = (needed - 1)/(size - itemsLeft) + 1;
							allocated = allocateRoom(symhy, needed,
													 itemsLeft, chunk);
							if (allocated == 0)
							{
								goto cantAllocate;
							}
						} /*if (needed > allocated)*/
						yc = STRINGPTR(symhy);
						while (nRead++ < nToRead)
						{
							yc[used++] = '\0';
						}
					} /*if (nRead < nToRead)*/
						
					break; /* from switch */
					
				  case BYQUOTEDFIELDS:
/*
   Each field is quoted; anything nonwhite between fields yields a warning but
   is otherwise ignored, except that there can be one comma between fields
   without triggering a warning
*/

					for (nRead = 0;nRead < nToRead ; nRead++)
					{
						int       seekQuote;
						
						seekQuote = 1;
						do
						{
							needed = used + strlen(*LINE + cplace) + 1;
							if (needed > allocated)
							{
								/*compute average size of previous items*/
								chunk = (needed - 1)/(size - itemsLeft + 1) + 1;
								allocated = allocateRoom(symhy, needed,
														 itemsLeft - 1, chunk);
								if (allocated == 0)
								{
									goto cantAllocate;
								}
								setSTRING(symhy, TMPHANDLE);
							} /*if (needed > allocated)*/
							yc = STRINGPTR(symhy);

							stepIn = copyQuotedField((*LINE) + cplace,
													 yc + used, nRead > 0,
													 seekQuote,
													 &foundStuff);
							if (stepIn == 0)
							{ /* no quote found on remainder of line */
								c = '\0';
								break;
							}
							stepOut= strlen(yc + used);
							used += stepOut;
							
							if (stepIn > 0)
							{ /* completed quoted field read */
								cplace += stepIn;
							} /*if (stepIn > 0)*/
							else
							{  /* hit end of line before closing quote */
								yc[used++] = '\n';
								seekQuote = 0;
								if ((noMore = !GETLINE1(file)))
								{
									break;
								}
								c = '\n';
								length = strlen(*LINE);
								if ((*LINE)[length-1] == '\n')
								{
									(*LINE)[--length] = '\0';
								}
								if (nSkip > 0 && length < nSkip)
								{
									(*LINE)[nSkip] = '\0';
								}
								if (echo)
								{
									echoLINE();
								}
								cplace = nSkip;
							} /*if (stepIn > 0){}else{}*/
						} while (stepIn < 0);
						yc[used++] = '\0';
						if (stepIn == 0 || noMore)
						{
							break;
						}
						howmany++;
						itemsLeft--;
					} /*for (nRead = 0;nRead < nToRead ; )*/

					if (nRead == 0)
					{
						noMore = 1;
						break;
					}
					
					if (nRead < nToRead)
					{
						if (warning)
						{
							sprintf(OUTSTR,
									"WARNING: missing fields in %s %ld set to \"\"",
									(byRows || ndims == 1)? "case" : "column",
									(ndims == 1) ?
									howmany + nRead + 1 : iRowOrCol+1);
							putErrorOUTSTR();
						} /*if (warning)*/
						
						needed = used + (nToRead - nRead);
						itemsLeft -= (nToRead - nRead);
						howmany += (nToRead - nRead);
						if (needed > allocated)
						{
							/*compute average size of previous items*/
							chunk = (needed - 1)/(size - itemsLeft) + 1;
							allocated = allocateRoom(symhy, needed,
													 itemsLeft, chunk);
							if (allocated == 0)
							{
								goto cantAllocate;
							}
						} /*if (needed > allocated)*/
						yc = STRINGPTR(symhy);
						while (nRead++ < nToRead)
						{
							yc[used++] = '\0';
						}
					} /*if (nRead < nToRead)*/
				
					break;
					
				} /*switch (charmode)*/
			} /*if (!charmode){}else{}*/
			if (noMore)
			{
				break;
			}
			
			readSome += nToRead;
			nLeft -= nToRead;

			
			if (nLeft > 0)
			{ /* get a new line */
				if ((noMore = !GETLINE1(file)))
				{
					break;
				}
				if (nSkip > 0 && length < nSkip)
				{
					(*LINE)[nSkip] = '\0';
				}
				if (echo)
				{
					echoLINE();
				}
				cplace = nSkip;

				if (echo)
				{
					echoLINE();
				}
			} /*if (nLeft > 0)*/
		} /* while (nLeft > 0) */
	/* complete partially read multiline row or column */

		if (nRead <= 0 || noMore)
		{
			break;
		}
	} /*for (iRowOrCol = 0;iRowOrCol < mExpected;iRowOrCol++)*/

	if (warning && 0 < iRowOrCol && iRowOrCol < mExpected)
	{
		sprintf(OUTSTR,
				"WARNING: %ld %s read are fewer than %ld specified on header",
				iRowOrCol, (byRows) ? "rows" : "columns", nRows);
		putErrorOUTSTR();
	} /*if (iRowOrCol < mExpected)*/

	if (charmode)
	{
		if (warning && foundStuff)
		{
			sprintf(OUTSTR,
					"WARNING: %s() found extra stuff between quoted fields",
					FUNCNAME);
			putErrorOUTSTR();
		}
		
		if (itemsLeft > 0)
		{
			needed = used + itemsLeft;
			if (needed > allocated)
			{
				allocated = allocateRoom(symhy, needed, 0, chunk);
				if (allocated == 0)
				{
					goto cantAllocate;
				}
			} /*if (needed > allocated)*/
			yc = STRINGPTR(symhy);
			while (itemsLeft-- > 0)
			{
				yc[used++] = '\0';
			}
		} /*if (itemsLeft > 0)*/ 

		if (used != allocated)
		{/* trim down to size actually used */
			TMPHANDLE = mygrowhandle(STRING(symhy), used);
			setSTRING(symhy, TMPHANDLE);
			if (TMPHANDLE == (char **) 0)
			{
				goto cantAllocate;
			}
		} /*if (charmode && used > 0)*/
	} /*if (charmode)*/	
	(*LINE)[0] = OUTSTR[0] = '\0';

	return (howmany > 0) ? howmany : -NODATA;

  cantAllocate:
  	(*LINE)[0] = '\0';
	return (-CANTALLOC);
} /* readFile()*/

#define CHUNK   (10*sizeof(double))
/*
  970227 added new argument macroEnd.  If nlines < 0, the end of the
  macro is signalled by a line matching macroEnd which is otherwise
  ignored
  971104 added check for value of myhandlelength()
  980723 made some argments ints
  980811 set (*LINE)[0] to '\0' before returning to show line has been used
*/

long readMacro(FILE * file, long nlines, char * macroEnd,
			   Symbolhandle symhtext, int echo, int warning)
{
	long          c = 0, nchar = 0, error = NOTFOUND, nMax = CHUNK;
	char        **text = (char **) 0;
	long          jline = 1, iplace = 0, status = 1;
	int           seekMacroEnd = (nlines < 0);
	int           lengthMacroEnd = strlen(macroEnd);
	WHERE("readMacro");
	
/* it is assumed that *LINE already has something in it */
	if (echo)
	{
		echoLINE();
	}
	
	text = mygethandle(nMax+1);
	setSTRING(symhtext, text);
	if (text == (char **) 0)
	{
		goto errorExit;
	}

	if (!seekMacroEnd || strncmp(*LINE, macroEnd, lengthMacroEnd) != 0)
	{
		while (seekMacroEnd || jline <= nlines)
		{
			c = (*LINE)[iplace++];
			if (c == '\0')
			{
				if (jline == nlines || !GETLINE1(file) ||
					seekMacroEnd &&
					strncmp(*LINE, macroEnd, lengthMacroEnd) == 0)
				{
					break;
				}
				jline++;

				if (echo)
				{
					echoLINE();
				}
				iplace = 0;
				c = (*LINE)[iplace++];
			} /*if (c == '\0')*/
			(*text)[nchar++] = c;
			if (nchar > nMax)
			{
				nMax += CHUNK;
				text = mygrowhandle(text, nMax+1);
				setSTRING(symhtext, text);
				if (text == (char **) 0)
				{
					goto errorExit;
				}
			}
		} /*while (seekMacroEnd || jline <= nlines)*/

		while (nchar > 0 && isNewline((*text)[nchar-1]))
		{ /* trim off final newline */
			nchar--;
		}
		(*text)[nchar] = '\0';
	} /*if (!seekMacroEnd || strncmp(*LINE, macroEnd, lengthMacroEnd) != 0)*/
	
	if (warning)
	{
		*OUTSTR = '\0';
		if (seekMacroEnd && strncmp(*LINE, macroEnd, lengthMacroEnd) != 0)
		{
			sprintf(OUTSTR,
					"WARNING: line '%s' to terminate macro not found",
					macroEnd);
		}
		else if (!seekMacroEnd && (*LINE)[0] == '\0')
		{
			sprintf(OUTSTR,
					"WARNING: only %ld lines of macro found, < %ld specified on header",
					jline, nlines);
		}
		putErrorOUTSTR();
	} /*if (warning)*/

	if (nchar > 0)
	{
		long      handleLength = myhandlelength(text);
		
		if (handleLength < 0)
		{
			goto errorExit;
		}
		
		if (handleLength > nchar + 1)
		{ /* trim handle down to correct size */
			text = mygrowhandle(text, nchar+1);
			setSTRING(symhtext, text);
			if (text == (char **) 0)
			{
				goto errorExit;
			}
		} /*if (handleLength > nchar + 1)*/
	} /*if (nchar > 0)*/
	(*LINE)[0] = '\0';
	return (nchar);
	
  errorExit:
	(*LINE)[0] = '\0';
	return (-error);

} /* readMacro() */

		
