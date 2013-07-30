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
#pragma segment Print
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
#include "plot.h"
#include "matdat.h"

#ifdef MACINTOSH
#include "macIface.h"
#endif /*MACINTOSH*/

#ifdef WXWIN
#include "wx/wxIface.h"
#endif  /* WXWIN */

/*
  930301
  myprint(), myeol(), myerrorout(), myfeof(), and putOUTSTR() moved here from
  unxio.c and macio.c

  Name of file changed from commonou.c to commonIo.c
  getinput() moved here from main.c or macInput.c

  970129 modified treatment of lines starting with '!'
  
  970618 added pushInputlevel() and popOutputlevel()

  980723 added arguments labels and notes to matWrite() and notes to
         macWrite(), and implemented writing labels and/or notes
         as symbols.

  990210 Added putErrorOUTSTR()
  990212 Added putOutErrorMsg()
  990215 Changed uses of myerrorout() to putOutMsg()
 */

#ifdef MACINTOSH
#define PRINTABLE(C)  ((C) >= 0x20 && (C) != 0x7f || (C) == '\t')
#else /*MACINTOSH*/
#define PRINTABLE(C)  ((C) >= 0x20 && (C) < 0x7f || (C) == '\t')
#endif /*MACINTOSH*/

#if (0)
/* taken from globkb.h, here for reference */
EXTERN long    		SCREENWIDTH INIT(80);   /* columns on screen */
EXTERN long         FIELDWIDTH INIT(DEFAULTPRINTDEC+7+1);
	/* width of output field */
EXTERN double       MAXFIXED; /* maximum size for fixed format number */
EXTERN double       MINFIXED; /* minimum size for fixed format */
EXTERN long         FIELDWIDTH INIT(DEFAULTPRINTDEC+7+1);  /* width of output field */
EXTERN char         FIXEDFMT[20]; /* e.g., " %12.5f"*/
EXTERN char         FLOATFMT[20]; /* e.g., " %12.5g"*/
EXTERN char         DATAFMT[20]; /* one of FIXEDFMT or FLOATFMT */
EXTERN char         MISSINGFMT[10]; /* for MISSING, e.g., " %12s" */
EXTERN long         BEFOREDEC INIT(DEFAULTPRINTDEC+7);   /* e.g. 12 in 12.5g */
EXTERN long         AFTERDEC INIT(DEFAULTPRINTDEC);
	/* # after dp in format, e.g., 5 in 12.5g */
EXTERN char         FMTTYPE INIT('g');
	/* output format type, e.g., 'g' in 12.5g */
#endif /*0*/

#ifndef INDENTSTEP
#define INDENTSTEP  2
#endif /*INDENTSTEP*/

static long         IndentLevel;
static char        *Nameformissing = NAMEFORMISSING;
static char       **MissingCode;
static short        LengthMissing;

/*
   Compute the number of fields on each line
*/
static int compFieldsPerLine(long type, long labelWidth)
{
	long          width;
	
	if (type == LOGIC)
	{
		/* use width no wider than LengthMissing */
		width = (FIELDWIDTH <= LengthMissing) ? FIELDWIDTH : LengthMissing + 1;
	} /*if (type == LOGIC)*/
	else if (type == REAL)
	{
		width = FIELDWIDTH;
	} /*if (type == LOGIC){}else if (type == REAL){}*/
	else
	{
		width = SCREENWIDTH;
	}

	return ((SCREENWIDTH >= labelWidth + width) ?
			(SCREENWIDTH - labelWidth)/width : 1);
} /*compFieldsPerLine()*/

/*
   Print a single, non-structure symbol.  If labels != 0, label each row
   with indices of first element in row
   960102 Minor change to make sure at least one label starts in column 1

   970310 functions for printing labels moved to labutils.c, along with
   a new function calcRowLabelWidths().  In the process changed the name
   of putLabels() to putRowLabels().

   971216 Modified call to do_whole_graph() in printSymbol()

*/

static void printSymbol(Symbolhandle symh, FILE *fp, long labels)
{
	Symbolhandle    symhTmp = (Symbolhandle) 0;
	long            lastdim, j, k, start, last = 0, chstart = 0, chlast = 0;
	long            outPlace, idim;
	long            type;
	long            labelWidth, fieldsPerLine, fieldWidth = FIELDWIDTH;
	long            ndims = NDIMS(symh), nRows, iRow;
	int             hasLabels = HASLABELS(symh) || USECOLLABS;
	int             charVar = (TYPE(symh) == CHAR);
	long            ndims1 = (charVar && hasLabels) ? ndims : ndims - 1;
	long            factors[MAXDIMS], coord[MAXDIMS];
	char          **ch;
	char           *dataFmt, missingFmt[10];
	char           *dimLabels[MAXDIMS];
	char          **pDimLabels = (hasLabels) ? dimLabels : (char **) 0;
	char           *name = NAME(symh);
	long            dimLabelsWidth[MAXDIMS];
	unsigned char   c;
	double        **db;
	double          tmp;
	WHERE("printSymbol");

	lastdim = DIMVAL(symh,ndims);
	nRows  = symbolSize(symh)/lastdim;
	type = TYPE(symh);

	if (type == LONG)
	{
		/*Coerce value to double*/
		symhTmp = RInstall(SCRATCH, 0);
		if (symhTmp == (Symbolhandle) 0 ||
			!CopyLongToDouble(symh, symhTmp))
		{
			Removesymbol(symhTmp);
			return;
		}
		setNAME(symhTmp, SCRATCH);
		symh = symhTmp; 
		type = REAL;
	} /*if (type == LONG)*/

	if (type == LOGIC)
	{
		/* use width no wider than LengthMissing */
		fieldWidth = (FIELDWIDTH <= LengthMissing) ?
			FIELDWIDTH : LengthMissing + 1;
	} /*if (type == LOGIC)*/

	/* labels != 0 unless user has used labels:F as argument */
	if (labels)
	{
		/*
		  ndims1 is the number of labels to the left
		*/
		labelWidth = prepPrintLabels(symh, dimLabels, dimLabelsWidth);
	} /*if (labels)*/
	else
	{
		labelWidth = 0;
	} /*if (labels){}else{}*/
	

	if (type == LOGIC)
	{
		/* use width no wider than LengthMissing */
		sprintf(missingFmt," %%-%lds", fieldWidth - 1);
		dataFmt = missingFmt;
	} /*if (type == LOGIC)*/
	else if (type == REAL)
	{
		dataFmt = DATAFMT;
		strcpy(missingFmt,MISSINGFMT);
	} /*if (type == LOGIC){}else if (type == REAL){}*/
	
	fieldsPerLine = compFieldsPerLine(type, labelWidth);
	
	/*
	   see if we can trim off a space on the left and perhaps
	   squeeze in an extra field on a line
	*/

	if (!hasLabels && labels && fieldsPerLine > 1 &&
		nDigits(fieldsPerLine*((lastdim - 1)/fieldsPerLine) + 1) < 
		nDigits(lastdim))
	{
		labelWidth--;
		j = compFieldsPerLine(type, labelWidth);
		if (nDigits(j*((lastdim - 1)/j) + 1) < nDigits(lastdim))
		{
			fieldsPerLine = j;
		}
	}
	
	/* initialize "odometer" */
	factors[0] = 1;
	if (ndims > 1)
	{
		for (idim = 0; idim < ndims-1; idim++)
		{
			coord[idim] = 1;
			factors[idim+1] = factors[idim]*DIMVAL(symh,idim+1);
		} /*for (idim = 0; idim < ndims-1; idim++)*/
	} /*if (ndims > 1)*/
	else
	{
		coord[0] = 1;
	}
	
	if (labels && hasLabels && ndims1 < ndims &&
		(dimLabelsWidth[ndims1] > 0 || dimLabels[ndims1] == (char *) 0))
	{
		putColumnLabels(fp, DIMVAL(symh, ndims),
						dimLabels[ndims-1],
						labelWidth, fieldWidth, fieldsPerLine, type);
	} /*if (hasLabels && type != CHAR)*/
	
	for (iRow = 0;iRow < nRows;iRow++)
	{
		start = 0;
		for (idim = 0;idim < ndims-1;idim++)
		{
			start += (coord[idim]-1)*factors[idim];
		} /*for (idim = 0;idim < ndims-1;idim++)*/
		
		switch ((int) type)
		{
		  case CHAR:
		  case MACRO:
			ch = STRING(symh);
			if (start < last)
			{
				last = chlast = 0;
			}
			chstart = skipStrings(*ch + chlast,start - last) - *ch;
			last = start;
			chlast = chstart;
			for (coord[ndims - 1] = 1; coord[ndims - 1] <= lastdim;
				 coord[ndims - 1]++)
			{
				/* put out coord label */
				if (labels)
				{
					putRowLabels(fp, ndims, coord, labelWidth,
							  pDimLabels, dimLabelsWidth, 1);
				} /*if (labels)*/
				
				OUTSTR[0] = '"';
				outPlace = 1;
				do /* while (c != '\0')*/
				{
					c = (unsigned char) (*ch)[chstart++];
					if (c == '\0')
					{ /* terminate with quote */
						OUTSTR[outPlace++] = '"';
						OUTSTR[outPlace] = '\0';
						fmyprint(OUTSTR,fp);
					}
					else
					{
						if (c == '\\' || c == '"')
						{ /* escape '\' && '"' */
							OUTSTR[outPlace++] = '\\';
						}
						if (c != '\n')
						{ /* put character in buffer */
							if (!PRINTABLE(c))
							{ /* output excaped octal */
								outPlace += escapedOctal(c,
										(unsigned char *) (OUTSTR + outPlace));
							}
							else
							{
								OUTSTR[outPlace++] = c;
							}
						} /*if (c != '\n')*/

						if (c == '\n' || outPlace > BUFFERLENGTH-5)
						{
							OUTSTR[outPlace] = '\0';
							fmyprint(OUTSTR,fp);
							outPlace = 0;
							if (c == '\n')
							{
								fmyeol(fp);
								if (INTERRUPT != INTNOTSET)
								{
									break;
								}
							}						
						} /*if (c == '\n' || outPlace > BUFFERLENGTH-5)*/
					} /*if (c == '\0'){...}else{...}*/
				} while (c != '\0');
				fmyeol(fp);
				checkInterrupt(interruptExit);
				if (coord[ndims-1] < lastdim)
				{
					chstart = skipStrings(*ch + chstart,nRows-1) - *ch;
				}
			} /* for (coord[ndims - 1] = 1; 
					coord[ndims - 1] <= lastdim;coord[ndims - 1]++) */
			*OUTSTR = '\0';
			break;

		  case LOGIC:
		  case REAL:
			db = DATA(symh);
			k =start;
			coord[ndims - 1] = 1;
			while (coord[ndims - 1] <= lastdim)
			{
				if (labels)
				{/* put out coord label */
					putRowLabels(fp, ndims, coord, labelWidth,
								 pDimLabels, dimLabelsWidth, 0);
				} /*if (labels)*/				

				j = 0;
				while (coord[ndims-1] <= lastdim && j < fieldsPerLine)
				{
					tmp = (*db)[k];
					if (type == LOGIC)
					{
						if (isMissing(tmp))
						{
							sprintf(OUTSTR, missingFmt, *MissingCode);
						}
						else if (tmp == 1.0)
						{
							sprintf(OUTSTR, dataFmt, "T");
						}
						else if (tmp == 0.0)
						{
							sprintf(OUTSTR, dataFmt, "F");
						}
						else
						{
							sprintf(OUTSTR, missingFmt, "BADVALUE");
						}
					} /*if (type == LOGIC)*/
					else
					{
						if (isMissing(tmp))
						{
							sprintf(OUTSTR, missingFmt, *MissingCode);
						}
						else
						{
							/* make sure even -0 prints as 0 */
							sprintf(OUTSTR, dataFmt, (tmp != 0.0) ? tmp : 0.0);
						}
					} /*if (type == LOGIC){...}else{...}*/
					fmyprint(OUTSTR,fp);
					coord[ndims-1]++;
					j++;
					k += nRows;
				} /*while (coord[ndims-1] <=lastdim && j < fieldsPerLine)*/
				fmyeol(fp);
				checkInterrupt(interruptExit);
			} /*while (coord[ndims - 1] <= lastdim)*/
			break;

		  case PLOTINFO:
			{
				plotKeyValues       keyValues;
				
				unsetPlotKeys(&keyValues, 1);
				keyValues.pause = 1;
				keyValues.landscape = 0;
				keyValues.screendump = (char *) 0;

				do_whole_graph(GRAPH(symh), fp, DUMBTERM, FINDEMPTYWINDOW,
							   &keyValues);
			}		  
			break;
			
		} /* switch(type) */

		/* step odometer */
		stepOdometer(coord, &DIMVAL(symh, 1), ndims - 1, 1, 1);
	} /*for (iRow=0;iRow<nRows;iRow++)*/

	if (symhTmp != (Symbolhandle) 0)
	{
		Removesymbol(symhTmp);
	}
	
  interruptExit:
	*OUTSTR = '\0';
} /*printSymbol()*/

/*
   Private function to print a single symbol on fp.  If labels != 0,
   label each row with indices first element in row.  If symh is a STRUC
   or LIST, doprexpr() calls itself recursively with each component;
   otherwise it calls printSymbol()
   970627 Modified doprexpr() so it can print LONG symbols by creating a temporary
          REAL symbol
*/

static void doprexpr(Symbolhandle symh, FILE *fp, long labels)
{
	long            i, j, tmpdim, type;
	Symbolhandle    component;
	char           *name;
	WHERE("doprexpr");
	
	if (isNull(symh))
	{
		fmyprint("(NULL)", fp);
		fmyeol(fp);
		
		return;
	}

	if (!isDefined(symh))
	{
		fmyprint("UNDEFINED",fp);
		fmyeol(fp);
		return;
	}
	if (DIMVAL(symh,1) == 0)
	{
		return;
	}
	
	type = TYPE(symh);
	if (type == BLTIN)
	{
		fmyprint("Built-in function cannot be printed",fp);
		fmyeol(fp);
		return;
	}

		
	SETUPINT(interruptExit);

	IndentLevel += INDENTSTEP;

	switch ((int) type)
	{
	  case LIST:
	  case STRUC:
		tmpdim = DIMVAL(symh,1);
		for (i = 0; i < tmpdim; i++)
		{
			component = COMPVALUE(symh, i);
			if (component != (Symbolhandle) 0)
			{
				if (HASLABELS(symh))
				{
					name = getOneLabel(symh, 0, i);
				}
				else
				{
					name = isscratch(NAME(component)) ?
					  NAME(component) + 2 : NAME(component);
				}
				
				for (j=0;j < IndentLevel;j++)
				{
					OUTSTR[j] = ' ';
				}
				
				sprintf(OUTSTR+IndentLevel, "%s%s",
						(HASLABELS(symh)) ? NullString : "component: ", name);
				fmyprint(OUTSTR,fp);
				fmyeol(fp);
				checkInterrupt(interruptExit);
				doprexpr(component,fp,labels);
			} /*if (component != (Symbolhandle) 0)*/
			else
			{
				fmyprint("component: (no data)", fp);
				fmyeol(fp);
			}
			
			checkInterrupt(interruptExit);
		} /*for (i = 0; i < tmpdim; i++)*/
		break;

	  case MACRO:
	  case CHAR:
	  case LOGIC:
	  case REAL:
	  case PLOTINFO:
	  case LONG:
		printSymbol(symh, fp,labels);
	} /*switch ((int) TYPE(symh))*/

	/* fall through */
  interruptExit:
	IndentLevel -= INDENTSTEP;
	return;
	
} /*doprexpr()*/

/*
   print msg on standard output with no added newline; can be used repeatedly
   followd by myeol() to produce a complex line of output.
*/
void myprint(char * msg)
{
	IndentLevel = 0;
	fmyprint(msg, STDOUT);

} /*myprint()*/

/*
  Output a possible multiline string, using putOutMsg() for each line
  so that lines will be counted

  If it is not empty, make sure it ends with newline when addNL != 0
  970519 Fixed so that msg is not tampered with;  previously newlines were
  temporarily changed to '\0' and then changed back.
  
  990207 New argument addNL to control whether a new line will be added
*/

void mymultiprint(char *msg, FILE * fp, int addNL)
{
	int              addEol;
	
	if(fp == STDOUT)
	{
		register int     i, place = 0;
		register char    c, *line = msg;
		register int     bufferlength = BUFFERLENGTH;
		char             outstr[BUFFERLENGTH+1];

		while (1)
		{
			addEol = addNL;
			for (i = 0; i < bufferlength && (c = *line++) != '\0'; i++)
			{
				if (!isNewline(c))
				{
					outstr [i] = c;
				}
				else
				{
					break;
				}
			} /*for (i = 0; i < bufferlength && (c = *line++) != '\0'; i++)*/

			outstr[i] = '\0';
			if (i > 0 || c != '\0')
			{
				if (!isNewline(c))
				{
					myprint(outstr);
				}
				else
				{
					if (i == 0)
					{
						fmyeol(fp);
					}
					else
					{
						putOutMsg(outstr);
					}
					addEol = 0;
				}				
			} /*if (i > 0 || c != '\0')*/
			if (c == '\0' || *line == '\0')
			{
				break;
			}
			checkInterrupt(interruptExit);
		} /*while (1)*/
	} /*if(fp == STDOUT)*/
	else
	{
		fputs(msg, fp);
		addEol = !isNewline(msg[strlen(msg)]);
	}
	if (addEol)
	{
		fmyeol(fp);
	}
	
	/* fall through */
  interruptExit:
	return;
} /*mymultiprint()*/

/*
   put end of line on standard output
*/
void myeol(void)
{
	fmyeol(STDOUT);
}

/*
   Print msg on standard output added followed by a newline.  Despite its 
   name, it is often used for non-error messages.

   Formerly called myerrorout()
*/
void putOutMsg(char * msg)
{
	if (PRINTWARNINGS || strncmp("WARNING:", msg, (int) 8) != 0)
	{
		myprint(msg);
		myeol();
	}
} /*putOutMsg()*/

void myerrorout(char * msg)
{
	putOutMsg(msg);
} /*myerrorout()*/


/*
   Function to output line with up to 4 pieces.  Particularly designed to
   use when outputting error messages concerning file names to avoid
   overflowing OUTSTR or other buffer.  If any of piece[1-4] is global
   OUTSTR, OUTSTR[0] is set to '\0'.
*/
void putPieces(char * piece1, char * piece2, char * piece3, char * piece4)
{
	char      *pieces[4];
	short      i;
	
	pieces[0] = piece1;
	pieces[1] = piece2;
	pieces[2] = piece3;
	pieces[3] = piece4;

	if (PRINTWARNINGS || strncmp(piece1, "WARNING:", (int) 8) != 0)
	{
		for (i=0; i < 4 && pieces[i] != (char *) 0; i++)
		{
			myprint(pieces[i]);
			if (pieces[i] == OUTSTR)
			{
				OUTSTR[0] = '\0';
			}
		}
		myeol();
	} /*if (PRINTWARNINGS || strncmp(piece1, "WARNING:", 8) != 0)*/
	
} /*putPieces()*/

/*
  echo contents of *LINE (which usually will terminate with '\n') to output,
  taking care that it will be counted appropriately
*/

void echoLINE(void)
{
	char      *pc = *LINE + strlen(*LINE) - 1;
	char       c = *pc;

	if (pc >= *LINE && (c == '\n' || c == '\r'))
	{
		*pc = '\0';
	} /*if (pc >= *LINE && (c == '\n' || c == '\r'))*/
	putOutMsg(*LINE);
	if (pc >= *LINE)
	{
		*pc = c;
	} /*if (pc >= *LINE)*/
} /*echoLINE()*/

/*
   Print global OUTSTR followed by newline if it is non empty, and then set
   OUTSTR[0] to '\0'
*/
void putOUTSTR(void)
{
	if (*OUTSTR)
	{
		putOutMsg(OUTSTR);
		*OUTSTR = '\0';
	}
} /*putOUTSTR()*/

/*
   Print global OUTSTR possibly followed by " in macro XXXX" followed by NL
   if OUTSTR is not empty, and then set OUTSTR[0] to '\0'
*/
void putErrorOUTSTR(void)
{
	if (*OUTSTR)
	{
		putPieces(OUTSTR, inWhichMacro(), (char *) 0, (char *) 0);
	}
} /*putErrorOUTSTR()*/

void putOutErrorMsg(char * msg)
{
	putPieces(msg, inWhichMacro(), (char *) 0, (char *) 0);
} /*putOutErrorMsg()*/

/*
   myfeof(fp) tests the end-of-file status on fp; it always returns 0 if fp
   is STDOUT
*/
int myfeof(FILE * fp)
{
	return ((fp != STDOUT) ? feof(fp) : 0);
} /*myfeof()*/

/*
   Print a single symbol with row labels on standard output.  Used in the
   parser to print the unassigned results, and in printglm().
*/
void prexpr(Symbolhandle symh)
{
	if (symh == (Symbolhandle) 0 || TYPE(symh) != NULLSYM)
	{
		IndentLevel = -INDENTSTEP;
		MissingCode = &Nameformissing;
		LengthMissing = strlen(*MissingCode);

		doprexpr(symh,STDOUT,1L);
	}
} /*prexpr()*/

/*
   public function to print a single symbol on fp.  If labels != 0,
   label each row with indices first element in row.  If missingcode is
   non-null, it specifies a string to be printed for missing values.  fprexpr
   also initializes indentation of headers
*/

void fprexpr(Symbolhandle symh, FILE * fp, long labels, char ** missingcode)
{
	IndentLevel = -INDENTSTEP;
	MissingCode = (missingcode != (char **) 0) ? missingcode : &Nameformissing;
	LengthMissing = strlen(*MissingCode);
	
	doprexpr(symh,fp,labels);
} /*fprexpr()*/

#ifndef LINEWIDTH
#define LINEWIDTH 132
#endif /*LINEWIDTH*/


#define ByLines  (charFormat == BYLINES)
#define ByFields (charFormat == BYFIELDS)
#define Quoted   (charFormat == BYQUOTEDFIELDS)

static char * quoteField(char * field, FILE *fp)
{
	char *        outstr = OUTSTR;
	long          place = 0, i;
	unsigned char c;
	
	outstr[place++] = '"';
			
	for (i = 0; (c = field[i]) != '\0'; i++)
	{
		
		if (c == '"' || c == '\\' ||  c != ' ' && c != '\n' && !isgraph(c))
		{
			outstr[place++] = '\\';
			if (c == '\r' || c == '\t')
			{
				c = (c == '\r') ? 'r' : 't';
			}
			else if (!isgraph(c))
			{
				outstr[place++] = c / 64 + '0';
				c %= 64;
				outstr[place++] = c / 8 + '0';
				c = c % 8 + '0';
			}
		} /*if (c != ' ' && c != '\n' && !isgraph(c))*/
		outstr[place++] = c;
		if (place > BUFFERLENGTH - 6)
		{
			outstr[place] = '\0';
			fmyprint(outstr, fp);
			place = 0;
		}
	} /*for (i = 0; (c = field[i]) != '\0'; i++)*/
	outstr[place++] = '"';
	outstr[place] = '\0';
	
	return (outstr);
} /*quoteField()*/

static void putComment(char *comment, long nlines, FILE * fp)
{
	char      *prefix = ") ";
	
	while (nlines-- > 0)
	{
		fmyprint(prefix, fp);
		fmyprint(comment, fp);
		fmyeol(fp);
		if (nlines > 0)
		{
			comment = skipStrings(comment, 1);
		}
	}
} /*putComment(*/

#ifndef LABELSARECHAR

/*
  function to write labels as a CHARACTER vector
  Labels are written quoted, with several per line, assuming
  a line length of 72
*/
#define LineWidthForLabels 72

static void labelsWrite(Symbolhandle symh, FILE * fp, char * name, int header)
{
	if (HASLABELS(symh) && strcmp(NAME(LABELS(symh)), LABELSSCRATCH) == 0 &&
		strlen(name) + strlen(LABELSSCRATCH) < MAXNAME)
	{
		char              **labelsHandle = LABELSHANDLE(symh);
		long                nLabels = DIMVAL(LABELS(symh), 1);
		long                i, j, place;
		int                 nperLine, maxlen = 1;
		char               *outstr;
		
		for (i = place = 0; i < nLabels; i++)
		{
			long         place1 =
			  skipStrings(*labelsHandle + place, 1) - *labelsHandle;
			
			if (place1 - place > maxlen)
			{
				maxlen = place1 - place;
			}
			place = place1;
		} /*for (i = 0; i < nLabels; i++)*/
		maxlen += 2; /*now includes ' ' and two '"' */
		
		nperLine = (LineWidthForLabels + 1)/maxlen;
		if (nperLine == 0)
		{
			nperLine = 1;
		}
		else if (nperLine > nLabels)
		{
			nperLine = nLabels;
		}
		
		if (header)
		{
			sprintf(OUTSTR,"%s$%s %5ld QUOTED COLUMNS",
					name, LABELSSCRATCH + 2, nLabels);
			fmyprint(OUTSTR,fp);
			fmyeol(fp);
		
			fmyprint(")\"", fp);
			for (i = 0; i < nperLine-1; i++)
			{
				fmyprint("%s ", fp);
			}
			fmyprint("%s\"", fp);
			fmyeol(fp);
		} /*if (header)*/
		for (i = 1, place = 0; i <= nLabels; i++)
		{
			outstr = quoteField(*labelsHandle + place, fp);
			fmyprint(outstr, fp);
			
			if (i % nperLine != 0 && i != nLabels )
			{
				for (j = strlen(outstr); j < maxlen; j++)
				{
					fmyprint(" ", fp);
				}
			}
			else
			{
				fmyeol(fp);
			}
			place = skipStrings(*labelsHandle + place, 1) - *labelsHandle;
		} /*for (i = 0; i < nLabels; i++)*/
		fmyeol(fp); /* follow vector of labels with blank line */
	}
} /*labelsWrite()*/
#endif /*LABELSARECHAR*/

#ifndef NOTESARECHAR
static void notesWrite(Symbolhandle symh, FILE * fp, char * name, int header)
{
	char            notesName[MAXNAME + 1];
	
	if (HASNOTES(symh) && strcmp(NAME(NOTES(symh)), NOTESSCRATCH) == 0 &&
		strlen(name) + strlen(NOTESSCRATCH) < MAXNAME)
	{
		sprintf(notesName, "%s$%s", name, NOTESSCRATCH + 2);
		matWrite((Symbolhandle) NOTES(symh), fp, 0.0, header, 0, 0, "",
				 BYLINES, 0, notesName, (Symbolhandle) 0);
	}
}
#endif /*NOTESARECHAR*/

/*
   matWrite() writes a REAL or LOGICAL symbol to fp, without row labels,
   with or without a header line.  Missing values are coded with
   the value of missValue, unless that is MISSING, in which case they
   are printed as -99999.9999 (value of OLDMISSING).

   Unless header == 0, a header line containing the name of the object and
   its dimensions is written to fp, possibly followed by comment lines.  In
   particular, when header != 0 and arg contains missing values, a comment
   line of the form ') MISSING value' is written where value is either the
   value specified by missValue or -99999.9999;  also, when header != 0 and
   arg has type LOGIC, a line of the form ') LOGIC' is written.

   970109 changed so that number of items per line depends on SCREENWIDTH
   970219 fixed bug that trimmed off 1st 2 name characters when in, say,
          matprint(filename,henry:x,name:"Henry Aldrich")
   970228 modified so that arg can be STRUCTURE
*/
void matWrite(Symbolhandle arg, FILE * fp, double missValue,
			  int header, int labels, int notes, char *separator,
			  int charFormat, int oldStyle, char *name,
			  Symbolhandle symhComment)
{
	char          *dataFmt = DATAFMT, *missingFmt = DATAFMT;
	char          *ch, *ch1, *outstr, nextc;
	long           m, n, i, j, k, l, size, length;
	long           ndims;
	long           nperline;
	long           foundMissing = 0;
	long           incs[MAXDIMS];
	long           ii[MAXDIMS];
	long           place[MAXDIMS];
	long           type = TYPE(arg);
	long           last = 0, cplace = 0;
	double         y;
	double        *yptr;
	char          *cptr;
	char           fieldFmt[20];
	unsigned char  c;
	WHERE("matWrite");

	if (!header)
	{
		notes = (notes > 0);
		labels = (labels > 0);
	}
	labels = labels && type != MACRO && HASLABELS(arg);
	notes = notes && HASNOTES(arg);

	ndims = NDIMS(arg);

	if (isscratch(name) || iskeyname(name))
	{
		name += 2;
	}
	
	if (type == NULLSYM || type == PLOTINFO)
	{
		sprintf(OUTSTR, "%s     0  %s", name, NULLNAME);
		fmyprint(OUTSTR, fp);
		fmyeol(fp);
		if (type == PLOTINFO)
		{
			sprintf(OUTSTR,
					"%c original type %s written as type %s",
					COMMENTSTART, typeName(type), typeName(NULLSYM));
			fmyprint(OUTSTR, fp);
			fmyeol(fp);
		} /*if (type == PLOTINFO)*/
		fmyeol(fp);
	}
	else if (type == MACRO)
	{
		macWrite(arg, fp, header, notes, oldStyle, name, symhComment);
	}
	else if (type != STRUC)
	{
		size = symbolSize(arg);
		m = (ndims > 1) ? DIMVAL(arg,ndims) : 1;
		if (isMissing(missValue))
		{
			missValue = OLDMISSING;
		}
	
		if (type != CHAR && (foundMissing = anyMissing(arg)) &&
			floor(missValue) != missValue)
		{
			missingFmt = (missValue == OLDMISSING) ? " %.4f" : " %.17g";
		} /*if (type != CHAR && (foundMissing = anyMissing(arg)))*/

		if (type == CHAR && charFormat == 0)
		{
			charFormat = BYFIELDS; /* default if possible */
			cptr = STRINGPTR(arg);
			for (i = 0; i < size; i++)
			{
				for (j = 0; (c = cptr[j]) != '\0'; j++)
				{
					if (c == ' ' || c == '\n')
					{
						charFormat = (*separator == '\0') ?
							BYLINES : BYQUOTEDFIELDS;
					}
					else if (!isgraph(c))
					{
						charFormat = BYQUOTEDFIELDS;
					}
					if (Quoted)
					{
						break;
					}
				} /*for (j = 0; (c = cptr[j]) != '\0'; j++)*/
				if (Quoted)
				{
					break;
				}
				if (i < size - 1)
				{
					cptr = skipStrings(cptr, 1);
				}
			} /*for (i = 0; i < size; i++)*/
		} /*if (type == CHAR && charFormat == 0)*/
	
		if (type != CHAR || !ByLines)
		{
			nperline = (SCREENWIDTH-1)/FIELDWIDTH;
			if (nperline > m || *separator != '\0')
			{
				nperline = m;
			}
		} /*if (type != CHAR || !ByLines)*/
		else
		{
			nperline = 1;
		}
		if (type == CHAR && ByFields)
		{
			sprintf(fieldFmt, "%%-%lds ", (FIELDWIDTH > 1) ? FIELDWIDTH - 1 : 1);
		}
	
		if (header)
		{
			/* write header */
			sprintf(OUTSTR,"%-12s",name);
			fmyprint(OUTSTR,fp);
			for (i = 1;i <= ndims;i++)
			{
				sprintf(OUTSTR," %5ld",DIMVAL(arg,i));
				fmyprint(OUTSTR,fp);
			}
			if (type == CHAR)
			{
				sprintf(OUTSTR, " %s", (Quoted) ? "QUOTED" : "CHARACTER");
				fmyprint(OUTSTR, fp);
			}
			else if (type == LOGIC)
			{
				fmyprint(" LOGICAL", fp);
			}
#ifndef LABELSARECHAR
			if (labels)
			{
				fmyprint(" LABELS", fp);
			}			
#endif /*LABELSARECHAR*/
#ifndef NOTESARECHAR
			if (notes)
			{
				fmyprint(" NOTES", fp);
			}			
#endif /*NOTESARECHAR*/
			fmyeol(fp);
			if (symhComment != (Symbolhandle) 0)
			{
				putComment(STRINGPTR(symhComment), symbolSize(symhComment), fp);
			} /*if (symhComment != (Symbolhandle) 0)*/		
			checkInterrupt(interruptExit);
	
			if (foundMissing)
			{
				fmyprint(") MISSING:",fp);
				sprintf(OUTSTR,missingFmt,missValue);
				fmyprint(OUTSTR,fp);
				fmyeol(fp);
				checkInterrupt(interruptExit);
			} /*if (foundMissing)*/
			if (TYPE(arg) == LOGIC)
			{
				fmyprint(") LOGICAL",fp);
				fmyeol(fp);
				checkInterrupt(interruptExit);
			} /*if (TYPE(arg) == LOGIC)*/
		
			if (type != CHAR || !ByLines)
			{
				fmyprint((type != CHAR) ? ")\"%lf" : ")\"%s", fp);
			
				for (i=1;i<nperline;i++)
				{
					fmyprint((type != CHAR) ? " %lf" : " %s",fp);
				}
				fmyprint("\"",fp);
				fmyeol(fp);
				checkInterrupt(interruptExit);
			} /*if (type != CHAR || !ByLines)*/		
		} /*if (header)*/

		incs[0] = size;
		for (j = 1;j<=ndims;j++)
		{
			ii[j-1] = 0;
			place[j-1] = 0;
			incs[j] = incs[j-1]/DIMVAL(arg,ndims-j+1);
		} /*for (j = 1;j<=ndims;j++)*/

		if (type != CHAR)
		{
			yptr = DATAPTR(arg);
		} 
		else
		{
			cptr = STRINGPTR(arg);
		}
	
		/* print out in transposed order */

		n = 0;  /* number of items already printed */
		l = 0;  /* position in array of element being printed */
		k = 0;  /* position in output line */
		while (n < size)
		{
			if (type != CHAR)
			{
				y = yptr[l];
				if (!isMissing(y))
				{
					/* make sure even -0 prints as 0 */
					sprintf(OUTSTR, dataFmt, (y != 0.0) ? y : 0.0);
				}
				else
				{
					sprintf(OUTSTR,missingFmt,missValue);
				}
				outstr = OUTSTR;
			} /*if (type != CHAR)*/ 
			else
			{
				/* first find the string */
				if (l < last)
				{
					last = cplace = 0;
				}
			
				cplace = skipStrings(cptr + cplace, l - last) - cptr;
				last = l;
				outstr = cptr + cplace;
				if (ByLines)
				{
					while (*outstr != '\0')
					{
						ch1 = strchr(outstr, '\n');
						if (ch1 != (char *) 0)
						{
							nextc = ch1[1];
							ch1[0] = '\\';
							ch1[1] = '\0';
						}
						fmyprint(outstr, fp);
						if (ch1 != (char *) 0)
						{ /* '\n' found */
							fmyeol(fp);
							ch1[0] = '\n';
							ch1[1] = nextc;
							outstr = ch1 + 1;
						}
						else
						{ /* position at next '\0' */
							outstr += strlen(outstr);
						}
					} /* while (*outstr != '\0')*/
				} /*if (ByLines)*/
				else if (ByFields)
				{
					while (strlen(outstr) > BUFFERLENGTH - 1)
					{
						strncpy(OUTSTR, outstr, BUFFERLENGTH - 1);
						OUTSTR[BUFFERLENGTH-1] = '\0';
						fmyprint(OUTSTR, fp);
						outstr += BUFFERLENGTH - 1;
					} /*while (strlen(outstr) > BUFFERLENGTH - 1)*/
					sprintf(OUTSTR, fieldFmt, outstr);
					outstr = OUTSTR;
				}
				else if (Quoted)
				{
					outstr = quoteField(outstr, fp);
					length = strlen(outstr);
					if (length >= FIELDWIDTH)
					{
						outstr[length] = ' ';
						i = length + 1;
					}
					else
					{
						for (i = strlen(outstr); i < FIELDWIDTH; i++)
						{
							outstr[i] = ' ';
						}
					}				
					outstr[i] = '\0';
				}
			} /*if (type != CHAR){}else{}*/
			
			if (!ByLines)
			{				
				/* when separator not null, trim leading and trailing blanks */
				if (*separator && (type != CHAR || !ByLines))
				{
					for (ch = outstr;*ch == ' ';ch++)
					{
						;
					}
					for (ch1 = ch + strlen(ch) - 1;*ch1 == ' ';ch1--)
					{
						;
					}
					*(++ch1) = '\0';
				}
				else
				{
					ch = outstr;
				}
				fmyprint(ch,fp);
			} /*if (!ByLines)*/
		
			n++;
			k++;
			if (n % m == 0 || k % nperline == 0 || ByLines)
			{
				fmyeol(fp);
				checkInterrupt(interruptExit);
				k = 0;
			}
			else if (*separator)
			{
				fmyprint(separator,fp);
			}
			/* the following is essentially equivalent to ndims nested for loops */
			for (j = 1; j <= ndims; j++)
			{
				if (ii[j-1] == 0)
				{
					place[j-1] = l;
				}
				if ((ii[j-1] += incs[j]) < incs[j-1])
				{
					l += incs[j];
					break;
				}
				ii[j-1] = 0;
				l = place[j-1];
			} /*for (j = 1; j <= ndims; j++)*/
		} /*while (n<size)*/
		fmyeol(fp); /* add extra blank line at end */
	} /*if (TYPE(arg) != STRUC)*/
	else
	{
		Symbolhandle   symhComp;
		long           icomp, ncomps = NCOMPS(arg);
		long           comptype;
		char           dollar[2], compName[MAXNAME + 1];
		char          *outstr;
		long           place;

		dollar[0] = COMPNAMESTART;
		dollar[1] = '\0';
		compName[MAXNAME] = '\0';

		sprintf(OUTSTR, "%s %5ld  STRUC", name, ncomps);
		fmyprint(OUTSTR, fp);
		fmyeol(fp);

		sprintf(OUTSTR, "%c Structure with components:", COMMENTSTART);
		fmyprint(OUTSTR, fp);
		fmyeol(fp);
	
		for (icomp = 0; icomp < ncomps; icomp++)
		{
			if (icomp % 5 == 0)
			{
				outstr = OUTSTR;
				*outstr++ = COMMENTSTART;
			}
			sprintf(outstr, " %-*s", NAMELENGTH, NAME(COMPVALUE(arg, icomp)));
			
			outstr += strlen(outstr);

			if ((icomp + 1) % 5 == 0 || icomp == ncomps - 1)
			{
				*outstr = '\0';
				fmyprint(OUTSTR, fp);
				fmyeol(fp);
			}
		} /*for (icomp = 0; icomp < ncomps; icomp++)*/
		fmyeol(fp);

		strcpy(compName, name);
		place = strlen(compName);
		if (place < MAXNAME - 1)
		{
			strcpy(compName + place++, dollar);
		}
		
		for (icomp = 0; icomp < ncomps; icomp++)
		{
			symhComp = COMPVALUE(arg, icomp);
			comptype = TYPE(symhComp);

			if (place < MAXNAME)
			{
				strncpy(compName + place, NAME(symhComp), MAXNAME - place);
			}

			if (comptype != MACRO)
			{
				matWrite(symhComp, fp, missValue, header, labels, notes,
						 separator, charFormat, oldStyle, compName,
						 (Symbolhandle) 0);
			}
			else
			{
				macWrite(symhComp, fp, header, notes, oldStyle, compName,
						 (Symbolhandle) 0);
			}
		} /*for (icomp = 0; icomp < ncomps; icomp++)*/
	} /*if (TYPE(arg) != STRUC){}else{}*/
	
#ifndef LABELSARECHAR
	if (labels)
	{
		labelsWrite(arg, fp, name, header);
	}
#endif /*LABELSARECHAR*/
#ifndef NOTESARECHAR
	if (notes)
	{
		notesWrite(arg, fp, name, header);
	}
#endif /*NOTESARECHAR*/
	/* fall through */
  interruptExit:
	*OUTSTR = '\0';
} /*matWrite()*/

/*
   macWrite() writes MACRO symbol arg to file fp.  If header != 0, a
   header line with the macro name and the number of lines in the macro
   is printed first so that the macro can be read back in by macroread()
   970219 fixed bug that trimmed off 1st 2 name characters when in, say,
          macrowrite(filename,henry:mymacro,name:"Henry Aldrich")
   970517 fixed bug introduced by using mymultiprint() in loop;  this inserted
          newline every 500 characters.  Now use one call to mymultiprint.
   971101 added oldStyle to argument.  If != 0, old style header with
          line count will be written.  Otherwise, new style header with
          %name% after last line will be written
   980723 Added argument notes to control whether notes will be written
*/

void macWrite(Symbolhandle arg, FILE * fp, int header, int notes,
			  int oldStyle, char * name, Symbolhandle symhComment)
{
	long           m, i;
	long           c, place = 0;
	long           type = MACRO;
	WHERE("macWrite");
	
	m = 1;
	i = 0;
	while ((c = STRINGVALUE(arg,i)))
	{
		if (c == '\n' || c == '\r')
		{
			m++;
		}
		i++;
	} /*while ((c = STRINGVALUE(arg,i)))*/
	
	if (header)
	{
		char        countBuffer[10];
		
		if (oldStyle)
		{
			sprintf(countBuffer, "%5ld", m);
		}
		else
		{
			countBuffer[0] = '\0';
		}
		
		/* write header */
		if (isscratch(name) || iskeyname(name))
		{
			name += 2;
		}
		sprintf(OUTSTR, "%-12s %s %s%s", name, countBuffer, typeName(type),
				(!isInline(arg)) ? " OUTOFLINE" : NullString);
#ifndef NOTESARECHAR
		if (notes && HASNOTES(arg))
		{
			strcat(OUTSTR, " NOTES");
		}
#endif /*NOTESARECHAR*/
		fmyprint(OUTSTR, fp);
		fmyeol(fp);
		checkInterrupt(interruptExit);
		if (symhComment != (Symbolhandle) 0)
		{
			putComment(STRINGPTR(symhComment), symbolSize(symhComment), fp);
		} /*if (symhComment != (Symbolhandle) 0)*/		
	} /*if (header)*/

	mymultiprint(STRINGPTR(arg),fp, 1);

	if (header && !oldStyle)
	{
		sprintf(OUTSTR, "%%%s%%", name);
		fmyprint(OUTSTR, fp);
		fmyeol(fp);
	} /*if (header && !oldStyle)*/

	fmyeol(fp); /* follow macro with blank line */
	/* fall through*/

  interruptExit:
	*OUTSTR = '\0';

} /*macWrite()*/

/*
  Routines associated with reading a line of data
  Moved here from macIo.c unxIo.c and wxIo.cc 980511
*/
/*
   Globals used to communicate with datagetc(); they are defined and
     initialized in readdata.c

   StringToRead == (unsigned char **) 0 signals read by getc() from a file;
     otherwise datagetc() returns (*StringToRead)[PlaceInString], or EOF if
     PlaceInString >= Stringlength ; PlaceInString is incremented.
     If (*StringToRead)[PlaceInString] == '\0', datagetc() returns '\n'
   980508 ENDOFSTRING now returned on hitting '\0'

   980805 Changed type of StringToRead to unsigned char **
          Changed value of ENDOFSTRING from -2 to 256
          Modified code in datagetc() to reflect changes.
          Changed because an octal 254 in s stopped vecread(string:s,bychars)
*/
extern unsigned char   **StringToRead;
extern long              StringLength;
extern long              PlaceInString;

/* Flag for whether last data character read was CR*/
extern char          LastDataWasCr;

/* 980501 Prompt for additional lines when reading from console*/
extern char         *DataPrompt;

#define FromString (StringToRead != (unsigned char **) 0)

#ifdef MSDOS
#define MSDOSEOF   26 /* ^Z */
#endif /*MSDOS*/


/*
  970404 modified mygetc() so that CR, LF and CR/LF are all read as a single
         LF.  Probably only makes sense for DOS, but what the heck?
  970409 renamed mygetc() to datagetc()
*/
#define ENDOFSTRING (256)

/*
   Read one character from fp (or from string pointed to by StringToRead) into
   buffer *LINE
   Only references to datagetc() are in fileToLINE()
*/
static int datagetc(FILE * fp)
{
	int              c;
	unsigned int     c1 	;
	
	if (!FromString)
	{
#ifdef READLINE
		c = my_rl_getc(fp);
		c = (c == NL && LastDataWasCr) ? my_rl_getc(fp) : c;
#else /*READLINE*/
		c = getc(fp);
		c = (c == NL && LastDataWasCr) ? getc(fp) : c;
#endif /*READLINE*/
		LastDataWasCr = (c == CR);
		c = (LastDataWasCr) ? NL : c;
	
#ifdef MSDOS
		c = (c == MSDOSEOF) ? EOF : c;
#endif /*MSDOS*/

		return (c);
	} /*if(!FromString)*/

	if (PlaceInString >= StringLength)
	{
		return (EOF);
	}
	c1 = (*StringToRead)[PlaceInString++];
		
	if (PlaceInString == StringLength - 1 && isNewline(c1))
	{ /*make sure we return EOF next time*/
		PlaceInString++;
	}
	if (c1 == '\0')
	{
		c1 = ENDOFSTRING;
	}
	return ((int) c1);
} /*datagetc()*/

/*
   fileToLine(fp) reads a line, including the terminating '\n', from fp into
   *LINE.. If the line is longer than MAXLINE, it reads MAXLINE-1
   characters, adds a '\n'  and skips to just past the next '\n'.

   If an EOF is encountered during the read, it returns EOF; otherwise
   it returns 1.  This does not mean that no characters were read, since
   the file might not be terminated by '\n'.

   The calling function can determine how many characters were read by
   strlen(*LINE).
*/
#ifdef READLINE
extern char     *Readline_prompt; /*defined in unxIo.c*/
#endif /*READLINE*/
static int fileToLINE(FILE * fp)
{
	int         c;
	long        n = 0, nc = MAXLINE;
	char       *buffer = *LINE;
#ifdef READLINE
	char       *savePrompt = (char *) 0;
#endif /*READLINE*/
	WHERE("fileToLINE");

#ifdef READLINE
	if(!FromString && fp == stdin && ISATTY == (ITTYIN | ITTYOUT))
	{
		savePrompt = Readline_prompt;
		Readline_prompt = DataPrompt;
	}
#endif /*READLINE*/

	*buffer = '\0';
	c = datagetc(fp);
	if (c != EOF)
	{
		if (c != ENDOFSTRING)
		{
			do
			{
				*buffer++ = c;
				nc--;
				n++;
			} while(nc > 0 && !isNewline(c) &&
					(c = datagetc(fp)) != EOF && c != ENDOFSTRING);
		} /*if (c != ENDOFSTRING)*/

		if (c != ENDOFSTRING)
		{
			if (c != EOF)
			{
				buffer--;
			}
			*buffer++ = '\n';
		} /*if (c != ENDOFSTRING)*/
		
		*buffer = '\0';

		if (nc == 0)
		{
			/*buffer overflowed; skip remainder of the line*/
			while (!isNewline(c) && c != ENDOFSTRING && c != EOF && n++)
			{
				c = datagetc(fp);
			}
		}
	} /*if (c != EOF)*/

#ifdef READLINE
	if(savePrompt != (char *) 0)
	{
		Readline_prompt = savePrompt;
	}
#endif /*READLINE*/

#ifdef ACTIVEUPDATE
	if (fp == stdin && ISATTY == (ITTYIN | ITTYOUT))
	{
		saveForUpdate(*LINE);
	}
#endif /*ACTIVEUPDATE*/

	if(n > MAXLINE)
	{
		sprintf(OUTSTR,
				"WARNING: line longer than %ld characters; last %ld discarded",
				(long) MAXLINE, n - MAXLINE);
		putOUTSTR();
	}
	return ((c == EOF) ? EOF : 1);
} /*fileToLINE()*/

/*
  Fill global LINE with line from FILE *fn
  On Dos and Unix, fn should always be non-null.
  On Mac, STDIN is (FILE *) 0, and LINE is filled from dialog window when 
  fn == STDIN
*/

int fillLINE(FILE * fn)
{
	if(fn != (FILE *) 0 || FromString)
	{
		return (fileToLINE(fn));
	}
	else
	{
#ifndef HASCONSOLEDIALOG
 /* should not happen on UNIX (on windowed systems this means keyboard) */
		(*LINE)[0] = '\0';
		return 0;
#else
		return (fillFromDialog());
#endif
	}
} /*fillLINE()*/

/*
   Routines associated with reading a line of input
*/

static char      **LastInputString = (char **) 0;
extern char       *StdinName; /*defined in initiali.c */
/*
  970404 modified so that CR, CR/LF and LF are all read as single LF
         Really only needed for MSDOS but what the heck?
*/

#define    LastInputWasCr (LASTINPUTWASCR[BDEPTH])

static int inputgetc(FILE * fp)
{
	int           c;

#ifdef READLINE
	c = my_rl_getc(fp); /* returns getc() if not using readline()*/
	c = (c == NL && LastInputWasCr) ? my_rl_getc(fp) : c;
#else /*READLINE*/
	c = getc(fp);
	c = (c == NL && LastInputWasCr) ? getc(fp) : c;
#endif /*READLINE*/

	LastInputWasCr = (c == CR);

#ifndef MACINTOSH
	c = (LastInputWasCr) ? '\n' : c;
#else
	c = (c == NL) ? '\n' : c;
#endif /*MACINTOSH*/

#ifdef MSDOS
#define MSDOSEOF    26
	c = (c == MSDOSEOF) ? EOF : c;
#endif /*MSDOS*/

	return (c);
} /*inputgetc()*/

/*
   Function to save a copy of the complete input line in *LastInputString
   and, if history is implemented and input is interactive, to add another 
   copy to the history list.
*/

static void    saveInputString(char shortHelp)
{
	long         length;
	char         cc;
	
	length = (!shortHelp) ? strlen((char *) *INPUTSTRING) : 5;
		
	if (length > 0)
	{		
		/* Note: It is not an error if LastInputString cannot be created*/
		if (!shortHelp)
		{
			LastInputString = myduphandle((char **) INPUTSTRING);
		}
		else
		{
			LastInputString = mygethandle(5);
			if (LastInputString != (char **) 0)
			{
				(*LastInputString)[0] = shortHelp;
				strcpy(*LastInputString + 1, "elp");
			}
		}
		
		if (LastInputString != (char **) 0)
		{
			if (((cc = (*LastInputString)[length-1]) == NL || cc == CR))
			{
				(*LastInputString)[--length] = '\0';
			}
			LastInputString = mygrowhandle(LastInputString, length+1);
#ifdef SAVEHISTORY
			if (LastInputString != (char **) 0 &&
				BDEPTH == 0 && INPUTFILE[0] == STDIN &&
				ISATTY == (ITTYIN | ITTYOUT) && 
				(cc = (*LastInputString)[0]) != '\0' && cc != '\n')
			{
				saveInHistory(LastInputString);
			} /*if (LastInputString != (char **) 0)*/
#endif /*SAVEHISTORY*/
		} /*if (LastInputString != (char **) 0)*/
	} /*if (length > 0)*/
} /*saveInputString()*/


/*
   Routine to get a "line" of input from stream and put it in a place where
   yylex() can read it.

   "line" is general, because we have to worry about carry
   across literal quotes and compound lines

   950702 Modified to eliminate look ahead and added ifdef's to make it
   valid for both Macintosh and non Macintosh
   
   950703 Small modification to GETC to allow use of readline line editing
   library

   970201 Minor modification of treatment of back slashes in lines starting
   with '!'.  They

   970617 Updates Thisinputstring (INPUTSTRINGS[INPUTLEVEL]) whenever
   INPUTSTRING is assigned or grown.

   970924 Modified getinput() and saveInputString() so that a line matching
   ^[hH]elp$ is saved correctly.
*/

/*
	950703 getinput() moved here from macMain.c and main.c
*/
#define BS    8
#define VT   11
#define FF   12
#define DEL  127

#define IPlace      iPlace__
#define CC          cc__
#define Done        done__

#ifdef SCROLLABLEWINDOW
#define NeedSpace (fromFile && spaceleft < 5)
#define GotChar (Done != EOF)
#else /*SCROLLABLEWINDOW*/
#define NeedSpace (spaceleft < 5)
#define GotChar (!Done && (c >= ' ' || OKCONTROL(cc)))
/* ignore most controls */
#define OKCONTROL(C) (C == EOF || C == BS || C != VT && C != FF && isspace(C))
#endif /*SCROLLABLEWINDOW*/

#ifdef RECOGNIZEBANG
#define LineComplete (foundbang || !afterBslash && !inquotes && bracklvl <= 0)
#else /*RECOGNIZEBANG*/
#define LineComplete (!afterBslash && !inquotes && bracklvl <= 0)
#endif /*RECOGNIZEBANG*/

/*
	Macro GETC should return character and set flag Done as follows:
	If reading from file, Done is 0 unless EOF in which case it is EOF;
	otherwise (input from window), Done is 0 unless running off end of
	window in which case 1
*/
#ifdef WXWIN
extern int UseWindows;
extern int GetC(long iPlace, int *done);
extern void GetRunningWindowParms(long *CmdStrPos, long *TextLength);
#define GETC(F) (CC = ((fromFile) ? inputgetc(F) : GetC(IPlace++,&Done)),\
                 Done = ((fromFile) ? ((CC == EOF) ? EOF : 0) : Done),\
                 CC  )

#elif defined(MACINTOSH)
#define Text       (*TextHandle(teCmd))

#define GETC(F) (CC = ((fromFile) ? inputgetc(F) : \
				Text[IPlace++]), Done = ((IPlace >= TextLength(teCmd)) ? 1 :\
				((fromFile && CC == EOF) ? EOF : 0)), CC)
#else /*MACINTOSH*/
#define GETC(F) (CC = inputgetc(F), Done = (CC == EOF) ? EOF : 0, CC)
#endif /*MACINTOSH*/

#define STEPSPACE()     (inputString[++spaceused] = '\0', --spaceleft)
#define BACKSTEPSPACE() (inputString[--spaceused] = '\0', ++spaceleft)

#define CHUNK (10*sizeof(double)) /* number of bytes to increment INPUTSTRING */

/*
  Basic function for getting a line of input into INPUTSTRING
  Return codes
   -2    Out of memory fatal error (windowed versions)
   -1    End of input file (Macintosh non-interactive mode)
    0    Out of memory fatal error (non-windowed versions)
    1    Normal return, input line is in INPUTSTRING
    2    Same as 1, except there are extra characters (windowed versions)
    3    Unable to allocate INPUTSTRING (windowed version)
*/
int getinput(void)
{
	Symbolhandle    lastline = (Symbolhandle) 0;
	unsigned int    c;
	int             first = 1;
	int             escaped, afterBslash;
	int             cc;
	int             done__, cc__; /*used in GETC*/
	int             spaceused, spaceleft, inquotes, bracklvl;
	int             foundComment;
	char           *helpCall = "help(\"__teaser__\")";
	char            shortHelp = '\0';
	char           *lastlineName = "LASTLINE";
	unsigned char  *inputString;
	FILE           *inputFile;
#ifdef MACINTOSH
	Integer         iPlace__; /*used in GETC*/
	Integer         extra = 0, ichar, fromFile, len;
#endif /*MACINTOSH*/

#ifdef WXWIN
	long            iPlace__;
	long            extra = 0, ichar, fromFile, len;
	long			cmdStrPos, textLength;
#endif /* WXWIN */
	long            linestart = 0;
#ifdef ACTIVEUPDATE
	char           *emptyline = "\n";
#endif /*ACTIVEUPDATE*/
	int             foundbang = 0;
	WHERE("getinput");

	inputFile = INPUTFILE[BDEPTH];

#ifdef WXWIN
	fromFile = (inputFile != STDIN);

	if (!fromFile)
	{
		GetC(-1,&Done); /* reset the c++/wxwin input buffering */
		GetRunningWindowParms(&cmdStrPos, &textLength);
		len = textLength - cmdStrPos;
		if (len < strlen(helpCall) + 1)
		{
			len = strlen(helpCall) + 1;
		}
		spaceleft = len+1;
		IPlace = cmdStrPos;
	} /*if (!fromFile)*/
	else
	{
		spaceleft = CHUNK;
		IPlace = -1;
	} /*if (!fromFile){}else{}*/

#elif defined(MACINTOSH)
	fromFile = (inputFile != STDIN);
	Sleep = (BATCHECHO[BDEPTH]) ?  CmdSleep : BatchSleep;

	if (!fromFile)
	{
		len = TextLength(teCmd) - CmdStrPos;
		if (len < strlen(helpCall) + 1)
		{
			len = strlen(helpCall) + 1;
		}
		spaceleft = len+1;
		IPlace = CmdStrPos;
	} /*if (!fromFile)*/
	else
	{
		spaceleft = CHUNK;
		IPlace = -1;
	} /*if (!fromFile){}else{}*/
#else /*MACINTOSH*/
	spaceleft = CHUNK;
#endif /*MACINTOSH*/
	spaceused = 0;

	mydisphandle((char **) INPUTSTRING);

	ThisInputstring = INPUTSTRING = (unsigned char **) 0;
	
	if (LastInputString != (char **) 0 && (strlen(*LastInputString) > 1 ||
	   **LastInputString != '\0' && **LastInputString != '\n'))
	{ /* don't discard LASTLINE after empty command */
		if ((lastline = Lookup(lastlineName)) != (Symbolhandle) 0)
		{ /* re-use symbol */
			DeleteContents(lastline);
			setNAME(lastline,lastlineName);
			setTYPE(lastline, MACRO);
		} /*if ((lastline = Lookup(lastlineName)) != (Symbolhandle) 0)*/
		else
		{
			lastline = Install(lastlineName, MACRO);
		}
		if (lastline != (Symbolhandle) 0)
		{
			setSTRING(lastline,LastInputString);
			setNDIMS(lastline,1);
			setDIM(lastline,1,1);
		}
	}
	if (lastline == (Symbolhandle) 0)
	{
		mydisphandle(LastInputString);
	} /*if (lastline == (Symbolhandle) 0)*/
	
	LastInputString = (char **) 0;
	ThisInputstring = INPUTSTRING = (unsigned char **) mygethandle(spaceleft);
	if (INPUTSTRING == (unsigned char **) 0)
	{
#ifdef SCROLLABLEWINDOW
		if (UseWindows)
		{
			myprint("EMERGENCY SPACE CLEANING... try deleting variables");
			glmcleanup(1);
			return (3);
		}
#endif /*SCROLLABLEWINDOW*/
		goto outofmemory;
	} /*if (INPUTSTRING == (unsigned char **) 0)*/

	inputString = *INPUTSTRING;
	inputString[0] = (unsigned char) '\0';

	ISTRCHAR = 0;
	inquotes = 0;
	bracklvl = 0;
	afterBslash = escaped = foundComment = 0;

	Done = 0;
/*
	Following loop is only done once if at prompt level; it leaves the first
	character of the line to be scanned in c and cc
*/
	do
	{ /* skip completely blank lines on batch file */
		c = (unsigned int) (cc = GETC(inputFile));
	} while (BDEPTH > 0 && (cc == NL || cc == CR));

#ifdef RECOGNIZEBANG
	if (!Done)
	{
		foundbang = (cc == BANG);
	}
#endif /*RECOGNIZEBANG*/

/*
		loop exited by goto normalExit by EOL outside of {...} or "..."
		or at end of loop by EOF.
*/
	while (!Done)
	{
		if (NeedSpace)
		{/* make  more space */
			spaceleft = CHUNK;
			ThisInputstring = INPUTSTRING =
				(unsigned char **) mygrowhandle((char **)INPUTSTRING,
												(long) spaceused + spaceleft);
			if (INPUTSTRING == (unsigned char **) 0)
			{ /*Give up*/
				goto outofmemory;
			}
			inputString = *INPUTSTRING;
			/* should have a panic space saver here */
		} /*if (NeedSpace)*/

		if (first)
		{
			first = 0;
		}/*if (first)*/
		else
		{
			c = (unsigned int) (cc = GETC(inputFile));
		} /*if (first)*/

		if (GotChar)
		{
			inputString[spaceused] = (unsigned char) c;
			STEPSPACE();
			if (cc == '\\' && !foundbang)
			{
				escaped = !escaped;
				if (!inquotes)
				{
					afterBslash = 1;
				}
			} /*if (cc == '\\')*/
#ifdef RECOGNIZEBANG
			else if (foundbang && cc != '\n')
			{ /* accept character */
				afterBslash = (c == '\\');
			}
#endif /*RECOGNIZEBANG*/
			else if (cc == '"')
			{
				if (!(foundComment || escaped))
				{
					inquotes = !inquotes;
				}
				afterBslash = escaped = 0;
			}
			else
			{/* character not backslash or '"'*/
				/*
				  character not backslash or '"'
				  if foundband != 0, character must be '\n'
				*/
				escaped = 0;
				switch (cc)
				{
				  case '{':
				  case '}':
					if (!inquotes && !foundComment)
					{
						bracklvl += (cc == '{') ? 1 : -1;
					}
					break;

				  case NL:
				  case CR:
					/* Note character already added to line */
					foundComment = 0;
					if (BATCHECHO[BDEPTH])
					{
						mymultiprint((char *) inputString + linestart, STDOUT, 1);
					}
					else if (BDEPTH == 0 && SPOOLFILE != (FILE *) 0)
					{
						fprintf(SPOOLFILE, "%s", inputString + linestart);
					}

					if (BDEPTH == 0 && !BATCHECHO[0])
					{
#ifndef SCROLLABLEWINDOW
						if (NLINES < SCREENHEIGHT - 2)/* don't force pause*/
#endif /*SCROLLABLEWINDOW*/
						{
							incrementNLINES("\n");
						}
#ifdef ACTIVEUPDATE
						/* save line for screen refresh if needed */
						inputString[spaceused-1] = '\0';
						saveForUpdate((char *) inputString + linestart);
						saveForUpdate(emptyline);
						inputString[spaceused-1] = c;
#endif /*ACTIVEUPDATE*/
					} /*if (BDEPTH == 0 && !BATCHECHO[0])*/
					linestart = spaceused;

#ifdef RECOGNIZEBANG
					if (foundbang && afterBslash)
					{/* last character was a backslash */
						afterBslash = 0;
					/* delete backslash before new line */
						BACKSTEPSPACE();
						inputString[spaceused-1] = cc;
#ifndef SCROLLABLEWINDOW
						putprompt(MOREPROMPT);
#endif /*SCROLLABLEWINDOW*/
						break; /*from switch(){}*/
					} /*if (foundbang && afterBslash)*/
#endif /*RECOGNIZEBANG*/

					if (LineComplete)
					{/* command line is complete */
#ifdef WXWIN
						if (!fromFile &&
						   (extra = textLength - 1 - IPlace) > 0)
						{
							if (extra >= spaceleft)
							{
								spaceleft = extra+1;
								ThisInputstring = INPUTSTRING =
									(unsigned char **)
									mygrowhandle((char **)INPUTSTRING,
												 (long) spaceused + spaceleft);
								if (INPUTSTRING == (unsigned char **) 0)
								{
									goto outofmemory;
								} /*if (INPUTSTRING == (unsigned char **) 0)*/
								inputString = *INPUTSTRING;
								for (ichar=0;ichar<extra;ichar++)
								{
									inputString[spaceused++] = GETC(0);
								} /*for (ichar=0;ichar<extra;ichar++)*/
							} /*if (extra >= spaceleft)*/
						}

#elif defined(MACINTOSH)
						if (!fromFile &&
						   (extra = TextLength(teCmd) - 1 - IPlace) > 0)
						{
							if (extra >= spaceleft)
							{
								spaceleft = extra+1;
								ThisInputstring = INPUTSTRING =
									(unsigned char **)
									mygrowhandle((char **)INPUTSTRING,
												 (long) spaceused + spaceleft);
								if (INPUTSTRING == (unsigned char **) 0)
								{
									goto outofmemory;
								} /*if (INPUTSTRING == (unsigned char **) 0)*/
								inputString = *INPUTSTRING;
								for (ichar=0;ichar<extra;ichar++)
								{
									inputString[spaceused++] = Text[IPlace++];
								} /*for (ichar=0;ichar<extra;ichar++)*/
							} /*if (extra >= spaceleft)*/
						}
#endif /*MACINTOSH*/
						inputString[spaceused] = '\0';

						/* treat naive call for help as just that */
						if (spaceused == 5 &&
							(inputString[0] == 'h' || inputString[0] == 'H') &&
							strncmp((char *) inputString + 1, "elp", 3) == 0)
						{
							shortHelp = inputString[0];
							strcpy((char *) inputString, helpCall);
							spaceused = strlen(helpCall);
							inputString[spaceused] = '\0';
						}

						goto normalExit;
					} /*if (LineComplete)*/
					
					if (BDEPTH == 0 && !BATCHECHO[0])
					{
#ifdef MACINTOSH
/*						putprompt(MOREPROMPT);*/
						/* play it safe in case MOREPROMPT is implemented */
						if (strncmp(MOREPROMPT,
								   Text+IPlace, strlen(MOREPROMPT)) == 0)
						{
							IPlace += strlen(MOREPROMPT);
						}
#elif defined(WXWIN)
						; 
#else /*MACINTOSH*/
						putprompt(MOREPROMPT);
#endif /*MACINTOSH*/
					}
					break;

				  case BS:
				  case DEL:
					if (BACKSTEPSPACE() > 0)
					{
						BACKSTEPSPACE();
					}
					break;

				  case '#':
					if (!inquotes)
					{
						foundComment = 1;
					}
					break;

				  default:
					;
				} /*switch (cc)*/
				afterBslash = 0;
			}
		} /*if (GotChar)*/
	} /*while (!Done)*/

	/* you get here only if EOF has been found */
	inputString[spaceused] = '\0';
	LastInputWasCr = 0;

#ifndef SCROLLABLEWINDOW
	if (BDEPTH == 0 && spaceused == 0)
	{ /* fake quit if endfile at start of line */
		strcpy((char *) inputString,"quit\n");
		spaceused = strlen((char *) inputString);
		SCREENHEIGHT = 0;
		myeol();
		putOutMsg("Normal termination by end of file on input");
	} /*if (BDEPTH == 0 && spaceused == 0)*/
#endif /*SCROLLABLEWINDOW*/

	if (BDEPTH > 0)
	{
		if (BATCHECHO[BDEPTH])
		{
			if (spaceused == 0)
			{
				putPieces("(end of file on ",
						  (char *) *INPUTFILENAMES[BDEPTH], ")", (char *) 0);
			}
			else if (Done == EOF)
			{
				mymultiprint((char *) *INPUTSTRING, STDOUT, 1);
			}
		} /*if (BATCHECHO[BDEPTH])*/

		if (spaceused == 0)
		{ /* immediate end of file on read */
			closeBatch(0);
#ifdef SCROLLABLEWINDOW
			if (BDEPTH == 0 && UseWindows)
			{
				BDEPTH = -1; /* signal not to set UndoStatus to OUTPUTUNDO */
			} /*if (BDEPTH == 0)*/
#endif /*SCROLLABLEWINDOW*/
		} /*if (Done == EOF)*/
	} /*if (BDEPTH > 0)*/
#ifdef MACINTOSH
	else if (fromFile && inputString[0] == '\0')
	{ /* must be non-interactive mode */
		myprint("End of file on ");
		putOutMsg(*INPUTFILENAMES[0]);
		fclose(INPUTFILE[0]);
		INPUTFILE[0] = STDIN;
		mydisphandle(INPUTFILENAMES[0]);
		INPUTFILENAMES[0] = &StdinName;
		return (-1);
	} /*if (BDEPTH > 0){}else if (fromFile && inputString[0] == '\0'){}*/
#endif /*MACINTOSH*/

  normalExit:
#if (0) /*set 0 to 1 for debugging*/
	PRINT1(16,"*INPUTSTRING = '%s'\n", *INPUTSTRING,0,0,0);
#endif
	saveInputString(shortHelp);
#ifdef SCROLLABLEWINDOW
	return ((extra > 0 ) ? 2 : 1);
#else /*SCROLLABLEWINDOW*/
	return (1);
#endif /*SCROLLABLEWINDOW*/
  outofmemory:
	putOutMsg("REALLY out of space, sorry");
	FatalError = 1;
#ifdef SCROLLABLEWINDOW
	return (-2);
#else /*SCROLLABLEWINDOW*/
	return (0);
#endif /*SCROLLABLEWINDOW*/

} /*getinput()*/

/*
  970618 pushInputlevel() increments INPUTLEVEL and updates contents
		 of INPUTSTACK
		 popInputlevel() decrements INPUTLEVEL and updates globals like
		 ISTRCHAR, INPUTSTRING, BRACKETLEV, ...
  970820 popInputlevel() clears ThisScratchHandle before decrementing
*/

static int    MaxInputlevel = 0; /* maximum attained input level */

int pushInputlevel(char *name, long start, unsigned char ** inputstring)
{
	WHERE("pushInputlevel");

	if (INPUTLEVEL == MAXMDEPTH)
	{
		sprintf(OUTSTR,
				"ERROR: more than %ld nested macros or evaluated strings",
				(long) MAXMDEPTH);
		putOUTSTR();
		return (0);
	}
	ThisInputstart = start;
	ThisIstrchar = ISTRCHAR;
	ThisBracketlev = BRACKETLEV;
	ThisParenlev = PARENLEV;
	INPUTLEVEL++;
	ThisWdepth = WDEPTH;
	MaxInputlevel = (MaxInputlevel < INPUTLEVEL) ? INPUTLEVEL : MaxInputlevel;
	INPUTSTRING = ThisInputstring = inputstring;
	ISTRCHAR = ThisIstrchar = 0;
	
	strcpy(ThisInputname, name);
	
	return (1);
} /*pushInputlevel()*/

void popInputlevel(void)
{
	WHERE("popInputlevel");

	clearSCRATCHHANDLE();
	INPUTLEVEL--;
	BRACKETLEV = ThisBracketlev;
	PARENLEV = ThisParenlev;
	ISTRCHAR = ThisIstrchar;
	INPUTSTRING = ThisInputstring;
#if (0) /*set 0 to 1 for debugging*/
	PRINT1(16,"ThisInputname = %s, *INPUTSTRING = '%s', ISTRCHAR = %d, INPUTLEVEL = %d\n",ThisInputname,*INPUTSTRING, ISTRCHAR, INPUTLEVEL);
#endif /*0*/
} /*popInputlevel()*/

void cleanInputlevels(int all)
{
	int           currentLevel = (all) ? 0 : INPUTLEVEL;
	WHERE("cleanInputlevels");

	for (INPUTLEVEL = MaxInputlevel; INPUTLEVEL > currentLevel; INPUTLEVEL--)
	{
		mydisphandle((char **) ThisInputstring);
		ThisInputstring = (unsigned char **) 0;
	}
	INPUTSTRING = ThisInputstring;
	MaxInputlevel = currentLevel;
} /*cleanInputlevels()*/
