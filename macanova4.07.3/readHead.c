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


#include "globals.h"
#include "matdat.h"

#if defined(MACINTOSH) && !defined(MACIFACEH__)
#include "macIface.h"
#endif /*MACINTOSH && !MACIFACEH__*/

#define MAXNF     40
#define FIELDSIZE MAXLINE


/* macro GETLINE is false if fillLINE() obtains no data */

#define GETLINE(F) (fillLINE(F), (*LINE)[0])

#define    EOMLENGTH  23
char       EndOfMacros[EOMLENGTH + 1] =  "_E_N_D_O_F_M_A_C_R_O_S_";
/*
  970228 added findNextName() to search for a candidate data set or macro
  header line, returning the data set or macro name in matName.

  980811 modified so it will look at what's in *LINE it there is
         anything.  This gets rid of the requirement that a
         blank line follow header lines for a structure.

         Also, it will only find names whose first character would
         be legal for a MacAnova variable ([a-zA-Z@_])

  980209 Added check for a line matching static string EndOfMacros which
         can serve as an EOF marker.  This is to allow help for macros
         or data to follow the macros and/or data sets without a problem
         of a line of help being incorrectly identified as a macro or data
         set header line in the wrong format

  990212 Changed putOUTSTR() to putErrorOUTSTR()

  990220 Added data set and macro header keyword ENDED which signals that
         there is a line of the form '%setname%' after the data set or macro.
         This is to used while searching for a macro or data set to
         skip past an unwanted macro or data set.

         Allowed keyword DOLLARS on the macro name line.  This effectively
         signals that the macro is to be processed by macro(string,dollars:T)
         before being installed.

  findNextName() reads until it finds a line that looks like a macro or
  data set header.
  Specifically, it searches for lines of form
    ^  *name  *macro *$
    ^  *name  *null *$
  or
    ^  *name  *[0-9][0-9]*
  where 'name' is of the form [a-zA-Z_@][^ *]* and case is ignored in
  'macro'

  That is, there are at least two fields, the second of which is 'macro',
  'null' or starts with a digit.

  The first field is returned in matName
*/

static int findNextName(FILE* file, char   *matName)
{
	char     *line;
	int       place;
	int       done = 0;
	char      c;
	char      macrotype[10];
	char      nulltype[10];
	WHERE("findNextName");

	strcpy(macrotype, typeName(MACRO));
	strcpy(nulltype, typeName(NULLSYM));
	
	do /*while (!done)*/
	{
		if ((*LINE)[0] == '\0' && fillLINE(file) == EOF)
		{
			return (0);
		}

		if ((*LINE)[0] == EndOfMacros[0] &&
			strncmp(*LINE, EndOfMacros, EOMLENGTH) == 0)
		{
			return (0);
		}
		
		for (line = *LINE; isspace(*line); line++)
		{ /* skip leading white space, if any */
			;
		}

		if (isnamestart(line[0]) || line[0] == TEMPPREFIX)
		{			
			for (place = 0; place < MAXNAME; place++)
			{
				c = line[place];
				if (c == '\0' || isspace(c))
				{
					break;
				}
				matName[place] = c;
			} /*for (place = 0; place < MAXNAME; place++)*/
			matName[place] = '\0';

			if (!isNewline(c) && c != '\0' && place < MAXNAME)
			{
				while ((c = line[place], !isNewline(c)) && isspace(c))
				{ /* skip more white space, if any*/
					place++;
				}
				done = isdigit(c) || mystrncmp(line + place, macrotype, 5) ||
				  mystrncmp(line + place, nulltype, 4);
			} /*if (!isNewline(c) && c != '\0' && place < MAXNAME)*/
		} /*if (isnamestart(line[0]) || line[0] == TEMPPREFIX)*/
		
		if (!done)
		{
			(*LINE)[0] = '\0';
		}
	} while (!done);

	return (1);
		
} /*findNextName()*/

/*
  Keep reading until a line starting %matName% is found
*/
static int skipToSetEnd(FILE *file, char * matName)
{
	int          length = strlen(matName);
	char        *line;
	WHERE("skipToSetEnd");
	
	while (1)
	{
		if ((*LINE)[0] == '\0' && fillLINE(file) == EOF)
		{
			return (0);
		}
		
		line = *LINE;

		if (line[0] == ENDMACROSTART &&
			strncmp(line+1, matName, length) == 0 &&
			(line + 1)[length] == ENDMACROSTART)
		{
			return (1);
		}
		line[0] = '\0';
	} /*while (1)*/	
} /*skipToSetEnd()*/

/*
  Find starts of space or tab separated fields in line, stopping
  with the first  '\0' or '\n' (which is replaced by '\0')
*/
static int findFieldStarts(char * line, int fields [])
{
	int        ichar, nf = 0;
	int        clast = ' ';
	
	for (ichar = 0;ichar < MAXLINE;ichar++)
	{
		int         c = line[ichar];

		if (isNewline(c) || c == '\0')
		{
			line[ichar] = '\0';			
			break;
		}
		
		if ((c != ' ' && c != '\t') && (clast == ' ' || clast == '\t'))
		{
			fields[nf++] = ichar;
			if (nf >= MAXNF)
			{
				break;
			}
		} /*if ((c != ' ' && c != '\t') && (clast == ' ' || clast == '\t'))*/
		clast = c;
	} /*for (ichar=0;ichar < MAXLINE;ichar++)*/
	return (nf);
} /*findFieldStarts()*/


/*
  if anonymousComp == 0, matchMatName() returns 1 if and only if
     target and thisName are the same length and match exactly,
	 ignoring case.
  If anonymousComp != 0, meaning target ends in '$', then matchMatName()
     returns 1 if and only the first strlen(target) characters of
	 thisName match those of target, ignoring case, and the 
	 character in thisName matching the terminating '$' in target
	 is the final '$' in thisName
*/
static int matchMatName(char * target, char * thisName, short anonymousComp)
{
	int       targetLength = strlen(target);
	int       nameLength = strlen(thisName);
	WHERE("matchMatName");

	/* case is ignored in all comparisons */

	if (!anonymousComp && targetLength != nameLength ||
		anonymousComp && nameLength < targetLength + 1 ||
		mystrncmp(target, thisName, targetLength) != 0)
	{
		return (0);
	}
	if (anonymousComp &&
		strrchr(thisName, COMPNAMESTART) - thisName != targetLength - 1)
	{
		return (0);
	}
	return (1);
} /*matchMatName()*/

/*
   function to seek data set or Macro matName on FILE * file
   Input
   	file		should be opened FILE pointer
	matName		name of object; if "", then object starts in first
				non-blank line in the file
	verbose		If > 1, echo entire header to stdout
	            If = 1, echo only header lines that don't start '))
	 			If < 0, call must be to read the header lines only
				If == 0, skip comment lines
   Output
	dims		Dimensions of object
	nFields		The maximum number of items per line
	fmt			Address of a format
	byRows		0 only if COLUMNS or COLS specified on header; otherwise 1
	type		type of object read, e.g., REAL, LOGICAL, MACRO
	missValue	Missing value code, if any.
	endMacro    String marking end of macro
  Return value	NOERROR, NOTFOUND, BADHEADER, TWOFORMATS
				On NOERROR, *LINE contains first line after header
				(verbose >= 0) or the first line of the header (verbose < 0)
    970227 added argument endMacro to allow macros to be correctly read in
           the following format
           mymacro MACRO
           ) comment lines
           arbitrary number of text lines
           . . . . .
           %mymacro
           ----
           Also, if no macro or data set name is given, the name of
           the first 1 found is returned in matName.
   971030 Modified so that header lines starting '))' are not printed unless
           verbose > 1 or < 0.
   980723 Added arguments labels and notes and changed some others to ints
   980811 End file can now immediately follow comment lines with
          NULL data or nrows == 0
          REAL can now be header keyword
          Sets (*LINE)[0] to '\0' after use
*/

static int findField(char * keyword, int fields [], int nf)
{
	int           i;
	
	for (i = 0; i < nf; i++)
	{
		if (mystrncmp(keyword, *LINE + fields[i], strlen(keyword)) == 0)
		{
			return (i);
		}
	} /*for (i = 0; i < nf; i++)*/
	return (-1);
} /*findField()*/

#define Ndims (dims[0])
#define Nrows (dims[1])
#define Ncols (dims[2])

long         readHeader(FILE * file, long dims [], long * nFields, char **fmt,
						long * nSkip, int * byRows, long * type,
						int * quoted, int * inLine, int * labels, int * notes,
						int * dollars, double * missValue, char * matName,
						char * endMacro, int verbose)
{
	char     matName1[MAXNAME+1];
	char     field[FIELDSIZE+1];
	char    *field1, *field2, *field3;
	int      fields[MAXNF];
	int      nf, nameLength = strlen(matName);
	int      cFmtFound = 0;
	int      fortranFmtFound = 0;
	int      done = 0, oldStyleMacro, newStyleMacro;
	long     decodeError;
	long     c, tot, place = 0;
	long     ifield, jfield, ichar, jchar, wLength;
	short    anonymousComp;
	short    infoOnly = (verbose < 0);
	short    allHeader, doubleParen;
	short    dataType, realType;
	short    badFormat;
	double   tmp;
	WHERE("readHeader");

	allHeader = verbose > 1;

	(*fmt)[0] = '\0';
	*nFields = 0;
	field[FIELDSIZE] = '\0';
	
	/*
	  anonymousComp != 0 implies that the matName ends in '$' which
	  means we are seeking a component with name on the file of the form
	     matName[^$][^$]*  , with matName not starting with '$'
	*/

	anonymousComp = (matName[0] != COMPNAMESTART) &&
		nameLength > 1 && (matName[nameLength - 1] == COMPNAMESTART);

	do /*while (!done) */
	{
		matName1[0] = '\0';
		if (!findNextName(file, matName1))
		{
			return (NOTFOUND);
		}

		/* find starts of space or tab separated fields on header lines */
		nf = findFieldStarts(*LINE, fields);
		field1 = *LINE + fields[0];
		field2 = *LINE + fields[1];
		field3 = (nf >= 3) ? *LINE + fields[2] : NullString;

		if (nameLength == 0 || matchMatName(matName, matName1, anonymousComp))
		{
			if (anonymousComp || nameLength == 0)
			{
				/* matName should be an array of length MAXNAME + 1 */
				strcpy(matName, matName1);
			}
			done = 1;
		}
		else if (anonymousComp)
		{
			return (NOTFOUND);
		}
		
		if (!done)
		{
			if (nf > 1 && isnamestart(field1[0]))
			{
				int     jchar;
			
				for (jchar = 1; (c = field1[jchar], isnamechar(c)) ; jchar++)
				{
					;
				}
			
				if (isspace(c) &&
					((findField("ENDED", fields, nf) >= 0 ||
					  mystrncmp(field2,"MACRO",5) == 0 &&
					  (field2[5] == '\0' || isspace(field2[5]))) &&
					 !skipToSetEnd(file, matName1)))
				{
					return (NOTFOUND);
				}
			}
			
			(*LINE)[0] = '\0';
		} /*if (!done)*/
	} while (!done);
	/*
		At this point, matName should either contain the name of the
		data set or macro that has been found
	*/
	
	if (infoOnly)
	{ /* must be inforead() */
		(*LINE)[0] = '\0';
		return (NOERROR);
	}
	
	Ndims = 0;
	tot = 1; /* product of numerical fields */
	*type = 0;
	*byRows = -1;
	*inLine = -1;
	*quoted = 0;
	*labels = *notes = -1;
	
	oldStyleMacro = (nf >= 3 && mystrncmp(typeName(MACRO), field3, 5) == 0);
	newStyleMacro = (nf >= 2 && mystrncmp(typeName(MACRO), field2, 5) == 0);
	
	if (newStyleMacro || oldStyleMacro)
	{ /* treat MACRO specially */
		int      offset = (oldStyleMacro) ? 3 : 2;
		int      nkeys = nf - offset;
		int     *keyfields = fields + offset;
		
		endMacro[0] = '\0';
		*notes = *dollars = 0;
		if (findField("IN", keyfields, nkeys) >= 0)
		{
			*inLine = 1;
			nf--;
		}
		if (findField("OUT",keyfields, nkeys) >= 0)
		{
			if (*inLine == 1)
			{ /* both INLINE and OUTOFLINE */
				return(BADHEADER);
			}
			*inLine = 0;
			nf--;
		}
		if (findField("NOTE", keyfields, nkeys) >= 0)
		{
			*notes = 1;
			nf--;
		}
		if (findField("DOL", keyfields, nkeys) >= 0)
		{
			*dollars = 1;
			nf--;
		}

		if (findField("ENDED", keyfields, nkeys) >= 0)
		{
			nf--;
		}

		if (oldStyleMacro)
		{ /* name nlines MACRO */
			if (nf > 3 || !isdigit(field2[0]))
			{
				return (BADHEADER);
			}

			tmp = decodeString(field2, (char *) 0, &decodeError);

			if (decodeError || tmp != floor(tmp) || tmp < 0)
			{
				return (BADHEADER);
			}
			Nrows = (long) tmp;
		} /*if (oldStyleMacro)*/
		else
		{ /* name MACRO */
			/* create end of macro code */
			if (nf > 2)
			{
				return (BADHEADER);
			}
			endMacro[0] = ENDMACROSTART;
			strncpy(endMacro+1, matName1, ENDMACROLENGTH - 2);
			place = strlen(endMacro);
			endMacro[place++] = ENDMACROSTART;
			endMacro[place] = '\0';
			Nrows = -1;
		} /*if (isdigit(field2[0])){}else{}*/
		Ndims = 1;
		*type = MACRO;
		if (*inLine < 0)
		{
			*inLine = EXPANDINLINE;
		}
	} /*if (nf >= 3 && mystrncmp(typeName(MACRO), field3, 5) == 0)*/
	else if (mystrncmp(typeName(NULLSYM), field2, 4) == 0 ||
			 field3 != (char *) 0 && 
			 mystrncmp(typeName(NULLSYM), field3, 4) == 0)
	{
		if (nf > 3 || nf == 3 && (field2[0] != '0' || !isspace(field2[1])) ||
			mystrncmp(typeName(NULLSYM), field2, 4) == 0 && nf > 2)
		{
			return (BADHEADER);
		}
		Nrows = 0;
		Ndims = 1;
		*type = NULLSYM;
	}
	else
	{
	/*
	  Read non-negative integer fields if any after name
	  All dimensions == 0 is legal; some 0 and some non-zero is not
	*/
		for (ifield = 1;ifield < nf;ifield++)
		{
			tmp = decodeString(*LINE + fields[ifield], (char *) 0, &decodeError);
			if (decodeError)
			{
				break;
			}
			if (tmp != floor(tmp) || tmp < 0 || tot == 0 && tmp > 0)
			{
				return (BADHEADER);
			}
			dims[ifield] = (long) tmp;
			Ndims++; /* ndims */
			tot *= dims[ifield];
		} /*for (ifield = 1;ifield < nf;ifield++)*/
	
		if (nf >= 3 && findField("STRUC", fields, nf) >= 0)
		{ /* treat STRUC specially */
			if (findField("LABELS", fields, nf) >= 0)
			{
				*labels = 1;
			}
			if (findField("NOTES", fields, nf) >= 0)
			{
				*notes = 1;
			}
			nf -= (*labels > 0) + (*notes > 0);
			if (nf > 3 || Ndims != 1 || Nrows == 0)
			{
				return (BADHEADER);		
			}
			*type = STRUC;
		} /*if (nf >= 3 && mystrncmp("STRUC", *LINE + fields[2], 5) == 0)*/
		else if (ifield == nf)
		{ /* only name and dimension info */
			*type = REAL;
		} /*if (ifield == nf)*/
		else if (ifield > 1)
		{ /* found at least 1 dimension; look for keywords */
			for (jfield = ifield;jfield < nf;jfield++)
			{
				strncpy(field, *LINE + fields[jfield], FIELDSIZE);
				field[FIELDSIZE] = '\0';
			
				if (mystrncmp("LOGIC", field, 5) == 0)
				{
					if (*type && *type != LOGIC)
					{ /* already set *type */
						return (BADHEADER);
					}
					*type = LOGIC;
				}
				else if (mystrncmp("REAL", field, 4) == 0)
				{
					if (*type && *type != REAL)
					{ /* already set *type */
						return (BADHEADER);
					}
					*type = REAL;
				}
				else if (mystrncmp("CHAR", field, 4) == 0 ||
						 mystrncmp("QUOTE", field, 5) == 0)
				{
					if (*type && *type != CHAR)
					{ /* already set *type */
						return (BADHEADER);
					}
					*type = CHAR;
					if (tolower(field[0]) == 'q')
					{
						*quoted = 1;
					}
					realType = 0;
				}
				else if (mystrncmp("COLUMNS", field, 7) == 0 ||
						 mystrncmp("COLS", field, 4) == 0)
				{
					if (*byRows >= 0)
					{
						return (BADHEADER);
					}
					*byRows = 0;
				}
				else if (mystrncmp("ROWS", field, 4) == 0)
				{
					if (*byRows >= 0)
					{
						return (BADHEADER);
					}
					*byRows = 1;
				}
				else if (mystrncmp("FORMAT", field, 6) == 0)
				{
					fortranFmtFound = 1;
				}
				else if (mystrncmp("LABELS", field, 6) == 0)
				{
					if (*labels >= 0)
					{
						return (BADHEADER);
					}
					*labels = 1;
				}
				else if (mystrncmp("NOTES", field, 5) == 0)
				{
					if (*notes >= 0)
					{
						return (BADHEADER);
					}
					*notes = 1;
				}
				else if (mystrncmp("ENDED", field, 5) != 0)
				{
					sprintf(OUTSTR,
							"WARNING: Unrecognized keyword %s on header", field);
					putErrorOUTSTR();
				}
			} /*for (jfield = ifield;jfield < nf;jfield++)*/
		} /*if (ifield == nf){}else if (ifield > 1){}*/

		*type = (*type == 0) ? REAL : *type;
	}

	*labels = (*labels > 0) ? 1 : 0;
	*notes = (*notes > 0) ? 1 : 0;

	if (Ndims == 0)
	{ /* no dimensions found */
		return (BADHEADER);
	} /*if (Ndims == 0)*/

	realType = (*type == REAL || *type == LOGIC);
	dataType = (realType || *type == CHAR);

	/*
	   ROWS, COLS, FORMAT or more than one dimension are illegal
	   except for a REAL, LOGICAL, or CHARACTER data set
	*/

	if (!dataType && (*byRows >= 0 || fortranFmtFound || Ndims > 1))
	{
		return (BADHEADER);
	}

	if (fortranFmtFound && !realType)
	{ /* 'format' illegal except for REAL or LOGICAL*/
		return (BADHEADER);
	} /*if (fortranFmtFound && !realType)*/

	*byRows = (*byRows < 0) ? 1 : *byRows;
	
	if (dataType)
	{
		if (*byRows && Nrows > 0)
		{
			*nFields = tot/Nrows;
		}
		else
		{
			*nFields = Nrows;
		}
	} /*if (dataType)*/
	
	if (verbose)
	{ /* echo header line to output */
		int      len = strlen(*LINE) - 1;

		if (isNewline((*LINE)[len]))
		{ /* trim off any '\n'*/
			(*LINE)[len] = '\0';
		}
		putOutMsg(*LINE); /* use putOutMsg to make sure line is counted */
	} /*if (verbose)*/

	*nSkip = 0;

	badFormat = (*type == CHAR) ? BADCFORMAT : BADFORMAT;
	
	/* now look for lines starting with ')' or '(' */
	done = 0;
	do /* while (!done) */
	{
		if (!GETLINE(file))
		{
			if (Nrows == 0)
			{
				break;
			}
			return (NODATA);
		}
		place = 0;
		switch ((*LINE)[0])
		{
		  case FORMATSTART: /*'('*/
/*
  If reading a matrix this is probably the start of a Fortran format, and
  presumably is the last line before data and hence the next line should be
  read in to the buffer.  If reading a macro or CHARACTER variable this is
  presumably the first line and should be left in the buffer
*/
			done = 1;
			if (realType)
			{
				/* get following line and don't print Fortran format */
				if (!GETLINE(file) && Nrows > 0)
				{
					return (NODATA);
				}
				place = 0;
			}
			break;

		  case COMMENTSTART: /*')'*/
/*
  Descriptive comment, may contain sscanf format or "MISSING   value",
  or "LOGIC" (LOGIC is now (960201) deprecated)
  If lines starts '))' don't print it unless verbose < 0 or verbose > 1
*/
			doubleParen = (*LINE)[1] == COMMENTSTART;
			if (!doubleParen)
			{
				nf = sscanf(*LINE+1, "%s %lf", field, &tmp);
				if (realType && nf == 2 && strncmp(field, "MISSING", 7) == 0)
				{/* ') MISSING value' specifies value to be translated as MISSING*/
					*missValue = tmp;
				}
				else if (realType && nf >= 1 && strncmp(field, "LOGIC", 5) == 0)
				{/* ') LOGIC' specifies data is LOGICAL */
					*type = LOGIC;
				}
				else if (dataType && (*LINE)[1] == '"')
				{/* comment starting ')"' contains sscanf format */
					if (cFmtFound)
					{
						return TWOFORMATS;
					}
	/* determine columns to ignore and number of fields per line*/
					place = 2;
					*nSkip = 0;

	/* recognize mX or mx (e.g., '7x' or '7X') as indicating columns to skip */
					while ((c = (*LINE)[place], isdigit(c)))
					{
						*nSkip = 10*(*nSkip) + (c - '0');
						place++;
					}
				
					if (*nSkip > 0)
					{
						if (c != 'x' && c != 'X')
						{
							return (badFormat);
						}
						place++;
					}
					ichar = *nFields = 0;

					while (ichar < MAXFMT) /* count "%"'s */
					{
						c = (*LINE)[place++];
						if (c == FORMATITEMSTART)
						{
							(*fmt)[ichar++] = c;
							(*nFields)++;
							if (sscanf((*LINE) + place,"%ld", &wLength) == 1)
							{
								if (wLength > MAXLINE || wLength <= 0)
								{
									return (badFormat);
								}
								while ((c = (*LINE)[place], isdigit(c)))
								{
									(*fmt)[ichar++] = c;
									place++;
								}
							} /*if (sscanf((*LINE) + place,"%ld", &wLength) == 1)*/

							for (jchar = 0;
								 jchar < 3 && isalpha(c = (*LINE)[place]);
								 jchar++, place++)
							{
								field[jchar] = tolower(c);
							}
							if (jchar == 0 || jchar > ((*type != CHAR) ? 2 : 1))
							{
								return (badFormat);
							}
							if (*type == CHAR)
							{
								if (field[0] != 's')
								{
									return (badFormat);
								}
								(*fmt)[ichar++] = 's';
							}
							else
							{
								if (jchar == 1)
								{
									field[1] = field[0];
									field[0] = 'l';
								}
								field[2] = '\0';
								if (field[0] != 'l' || !strchr("gfe", field[1]))
								{
									return (badFormat);
								}
								(*fmt)[ichar++] = 'l';
								(*fmt)[ichar++] = 'f';
							}
						} /*if (c == FORMATITEMSTART)*/
						else if (c == '"' || c == NL || c == CR || c == '\0')
						{
							break;
						}
						else if (c != ' ')
						{
							return (badFormat);
						}
						else
						{
							(*fmt)[ichar++] = c;
						}
					} /*while (ichar < MAXFMT)*/
					(*fmt)[ichar] = '\0';
					cFmtFound = 1;
				}
			} /*if (!doubleParen)*/
			
			if (verbose && (allHeader || !doubleParen))
			{
				if ((*LINE)[1] != '"' || *type == STRUC || *type == MACRO)
				{
					int           len = strlen(*LINE) - 1;

					if (isNewline((*LINE)[len]))
					{ /* trim off any '\n'*/
						(*LINE)[len] = '\0';
					}
					putOutMsg(*LINE);
				}
			} /*if (verbose && (allHeader || !doubleParen))*/
			
			break;

		  default:
			/* not a comment or fortran format. Leave in buffer */
			done = 1;
		} /*switch ((*LINE)[0])*/
	} while (!done);

/*
	If *nFields is greater than dimensions on header, assume it specifies
	correct dimension.  If it is less, this is taken to indicate multi-line
	row or column, all with same format
*/
	if (Nrows == 0 || tot != 0)
	{ /* empty data set is legal on read */
		return (NOERROR);
	}
	return NOERROR;
} /*readHeader()*/

