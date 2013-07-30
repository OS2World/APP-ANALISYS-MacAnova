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
#include "matdat.h"

#ifdef MACINTOSH
#include "macIface.h"
#endif

#ifdef WXWIN
#include "wx/wxIface.h"
#endif /* WXWIN */

#ifdef CONSOLENAME
#define ISCONSOLE(NAME) (strcmp(NAME,CONSOLENAME) == 0)
#else /*CONSOLENAME*/
#define ISCONSOLE(NAME)  0
#endif /*CONSOLENAME*/

/*
  990212 replaced putOUTSTR() by putErrorOUTSTR()
*/
enum printOpCodes
{
	IPRINT      = 0x01,
	IWRITE      = 0x02,
	IMACRO      = 0x04,
	IMAT        = 0x08,
	IFILE       = 0x10,
	IERR        = 0x20,
	IFPRINT     = IPRINT | IFILE,
	IFWRITE     = IWRITE | IFILE,
	IMATPRINT   = IPRINT | IMAT | IFILE,
	IMATWRITE   = IWRITE | IMAT | IFILE,
	IMACROWRITE = IMACRO | IFILE,
	IERROR      = IERR | IPRINT
};


	/* formerly a routine to print a single macro */
/*
  Operations print(), write(), fprint(), fwrite(), matprint(), matwrite(),
  macrowrite(), error()

  The first argument for fprint(), fwrite(), matprint(), matwrite(),
  and macrowrite(), must be a file name.

  If the last argument is the CHAR string "new" then the file is overwritten;
  otherwise, stuff is written at the end.

  For print(), write(), fprint(), and fwrite() the remaining arguments may
  be any printable object (types REAL, LOGIC, CHAR, MACRO, STRUC, PLOTINFO).

  For matprint() and matwrite(), arguments must be REAL OR LOGIC.  LOGIC
  values T and F are translated to 1.0 and 0.0 and a comment of the form ')
  LOGICAL' is prepended.  A warning message is printed for any argument
  that is not a structure, or a REAL, LOGIC or CHARACTER variable.

  For macrowrite(), a warning message is printed for any argument that is not
  MACRO

  write(), fwrite(), and matwrite() differ from print(), fprint(), and
  matprint(), only in printing more significant digits.

  If keyword phrase file:fileName appears on print() or write(), it
  operates like fprint() or fwrite() with that fileName.  In that case,
  new:T is acceptable.

  All operations accept keywords nsig (e.g., nsig:8) and format (e.g.,
  format:"10'4f"), although these are meaningless for macrowrite.  They
  control the printing of any REAL or LOGIC objects later in the argument
  list.  These keywords may be repeated to change the significance.

  nsig:m is is equivalent to a format of %(m+7).mg  .  The last nsig or
  format after all non-keywords or unrecognized keywords is taken as a
  starting default.  Thus print(a, b, c, nsig:9) is equivalent to
  print(nsig:9, a, b c) or write(a,b,c)

  Key word new (e.g., new:T or new:F) can appear immediately after the file
  name.

  error() is identical to print() except it will cause immediate termination
  of the statement in which it occurs.  It is designed for printing fatal
  error messages in macros.  As of 990207 it also adds " in macro XXXX"
  to the macro when called from a macro, unless keyword phrase macroname:F
  is an argument
*/

typedef struct opEntry
{
	char *name;
	short iop1;
	short iop2;
} opEntry;

static opEntry Ops[] =
{ /* name          iop1       iop2 */
	{"write",      IWRITE,      IPRINT},
	{"fwrite",     IFWRITE,     IPRINT},
	{"matwrite",   IMATWRITE,   IMAT},
	{"print",      IPRINT,      IPRINT},
	{"fprint",     IFPRINT,     IPRINT},
	{"matprint",   IMATPRINT,   IMAT},
	{"macrowrite", IMACROWRITE, IMACRO},
	{"error",      IERROR,      IERR},
	{(char *) 0,   0}
};

typedef struct keylist
{
	char        *name;
	short        type;
	short        legalOps;
} keylist;

static keylist LegalKeys[] =
{
/*  name         type     legalOps*/
	{"new",       LOGIC,   IPRINT | IMAT | IMACRO},
	{"file",      CHAR,    IPRINT | IMAT | IMACRO},
	{"sep",       CHAR,    IMAT},
	{"header",    LOGIC,   IPRINT | IMAT | IMACRO},
	{"missing",   -1,      IPRINT | IMAT | IERR},
	{"nsig",      REAL,    IPRINT | IMAT | IERR},
	{"format",    CHAR,    IPRINT | IMAT | IERR},
	{"width",     REAL,    IPRINT | IMAT | IERR},
	{"height",    REAL,    IPRINT | IERR},
	{"labels",    LOGIC,   IPRINT | IMAT | IERR},
	{"notes",     LOGIC,   IMAT | IMACRO},
	{"name",      CHAR,    IPRINT | IMAT | IMACRO},
	{"quoted",    LOGIC,   IMAT},
	{"bylines",   LOGIC,   IMAT},
	{"comments",  CHAR,    IMAT | IMACRO},
	{"oldstyle",  LOGIC,   IMAT | IMACRO},
	{"macroname", LOGIC,   IERR},
	{(char *) 0,  0,       0},
};

#ifdef SEGMENTED
#if defined(MPW) || defined(MW_CW)
#pragma segment Print
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

/*
   General entry for print(), write(), fprint(), fwrite(), error(),
   matprint(), matwrite(), macrowrite()

   960502 changed fopen() to fmyopen()
   960503 changed macOpen() to macFindFile()
   960827 Illegal values for keywords now cause an error before any
          output, instead of just a warning.  Also keywords header,
		  labels, and sep can appear several times, applying to
		  later objects being printed.  Also name at end is same
		  as name at beginning, like other keywords
   960906 Added keyword comments on matwrite(), matprint() and macrowrite()
   970109 Added keyword width on print(), write(), matprint(), matwrite(),
          and error(), and keyword height on print(), write() and error()
   970302 Allow STRUC, NULLSYM, and PLOTINFO types as arguments to
          mat{print,write}() and macrowrite().  PLOTINFO variables are
		  written as NULL variables for now
   970506 Use macros TEXTWRITEMODE and TEXTAPPENDMODE for fmyopen() mode
   971101 Added keyword oldstyle to macrowrite()
   980430 Cosmetic changes; also, for example, error("N < 0") now prints 
          "ERROR: N < 0"
   980723 New keyword notes for matprint(), matwrite() and macwrite() and
          made keyword labels legal for matprint() and matwrite().
          Added notes and labels to argument list for matWrite()
          On print("CHARACTER SCALAR", header:T and/or labels:T), the
          string is enclosed in quotes and a header printed unless
          header:F
   990207 error() now adds information about which macro was active and
          recognizes new keyword phrase 'macroname:F' which suppresses the
          new information.
*/
Symbolhandle    print(Symbolhandle list)
{
	Symbolhandle    arg1 = COMPVALUE(list,0);
	Symbolhandle    argi, filearg = (Symbolhandle) 0;
	Symbolhandle    symhComment = (Symbolhandle) 0;
	Symbolhandle    result = (Symbolhandle) 0;
	FILE           *outFile = STDOUT;
	char           *fileMode = (char *) 0;
	long            nargs = NARGS(list), i, type, targetType, ifile;
	long            unusedArgs = nargs;
	long            op, keyOp;
	long            toFile = 0, useConsole = 0;
	long            fmt[3];
	int             header = -1, labels = -1, notes = -1;
	long            width = -1, height = -1;
	int             isheight;
	int             charFormat = 0;
	int             oldStyle = 0;
	int             macroname = 1;
	struct opEntry *who;
	struct keylist *key;
	char            separator[2];
	char          **missingCode = (char **) 0;
	char           *name, **nameH = (char **) 0;
	char           *keyword;
	char           *fileName = (char *) 0;
	char           *missingValueError =
		"ERROR: MISSING value not legal for %s";
	double          dsig, missValue;
	long            nsig = -1;
	WHERE("print");
	
/* save current format values in case they are changed */
	saveFormat();
	setMissing(missValue);
	SETUPINT(cleanup);
	
	*OUTSTR = '\0';
	*separator = '\0';

	for (who = Ops;who->iop1 != 0;who++)
	{
		if (strcmp(who->name,FUNCNAME) == 0)
		{
			op = who->iop1;
			keyOp = who->iop2;
			break;
		}
	} /*for (who = Ops;who->iop != 0;who++)*/

	if (!(op & IERR))
	{ /* look file file:"filename" */
		for (i = 0; i < nargs;i++)
		{
			argi = COMPVALUE(list,i);
		
			if ((keyword = isKeyword(argi)) && strcmp(keyword,"file") == 0)
			{
				if (filearg != (Symbolhandle) 0)
				{
					goto repeatedKey;
				}
				
				unusedArgs--;
				op |= IFILE;
				ifile = i + 1;
				filearg = argi;
				if (!ISCONSOLE(NAME(filearg)) && !isCharOrString(filearg))
				{
					notCharOrString(keyword);
					goto errorExit;
				}
				setNAME(argi,USEDKEYWORD);
			} /*if ((keyword = isKeyword(argi)) && strcmp(keyword,"file") == 0)*/
		} /*for (i=0;i<nargs;i++)*/
	} /*if (!(op & IERR))*/
	
	if (op & IFILE)
	{ /* operation writing to a file */
		if (filearg == (Symbolhandle) 0)
		{ /* no file:fileName ; file must be first argument */
			ifile = toFile = 1;
			filearg = arg1;
			if (!ISCONSOLE(NAME(filearg)) && !isCharOrString(filearg))
			{
				notCharOrString("file name");
				goto errorExit;
			}
			unusedArgs--;
		}
		else
		{ /* file:"fileName" was present" */
			toFile = 2;
		}
		
		useConsole = ISCONSOLE(NAME(filearg)) || ISCONSOLE(STRINGPTR(filearg));

		if (useConsole)
		{
			fileName = CONSOLENAME;
		}
	} /*if (op & IFILE)*/
	
	if (toFile)
	{ /* writing to a file */
		fileName = (useConsole) ? CONSOLENAME : STRINGPTR(filearg);
	} /*if (toFile)*/

	for (i=(toFile == 1) ? 1 : 0;i<nargs;i++)
	{ /* check arguments and keywords, saving trailing formatting info */
		int        restoreDefault = 0;

		argi = COMPVALUE(list,i);
		if (!argOK(argi, (!(op & (IMAT|IMACRO))) ? NULLSYM : 0,i+1))
		{
			goto errorExit;
		}
		type = TYPE(argi);

		if ((keyword = isKeyword(argi)))
		{
			for (key = LegalKeys;key->type != 0;key++)
			{
				if (strcmp(keyword,key->name) == 0)
				{
					break;
				}
			} /*for (key = LegalKeys;key->type != 0;key++)*/

			targetType = key->type;
			
			if (key->legalOps & keyOp)
			{
				if (strcmp(keyword, "missing") == 0)
				{
					targetType = (op & IMAT) ? REAL : CHAR;
				}
				
				if (strcmp(keyword, "comments") == 0)
				{
					if (type != targetType || !isVector(argi))
					{
						sprintf(OUTSTR,
								"ERROR: value for %s not CHARACTER vector or scalar",
								keyword);
						goto errorExit;
					}
				} /*if (strcmp(keyword, "comments") == 0)*/
				else if (!isScalar(argi) || type != targetType)
				{
					if (targetType == CHAR)
					{
						notCharOrString(keyword);
					}
					else if (targetType == LOGIC)
					{
						notTorF(keyword);
					}
					else
					{
						notNumberOrReal(keyword);
					}
					goto errorExit;
				} /*if (!isScalar(argi) || type != targetType)*/

				if (targetType == LOGIC && isMissing(DATAVALUE(argi, 0)))
				{
					notTorF(keyword);
					goto errorExit;
				}
				
				if (strcmp(keyword,"new") == 0)
				{
					if (!toFile)
					{
						sprintf(OUTSTR,
								"ERROR: keyword %s illegal unless a file is specified",
								keyword);
						goto errorExit;
					}
					if (fileMode)
					{
						goto repeatedKey;
					}
					fileMode = (DATAVALUE(argi,0) == 0.0) ?
						TEXTAPPENDMODE : TEXTWRITEMODE;
					setNAME(argi,USEDKEYWORD);
				} /*if (strcmp(keyword,"new") == 0)*/
				else if (strcmp(keyword,"sep") == 0)
				{
					if (strlen(STRINGPTR(argi)) > 1)
					{
						sprintf(OUTSTR,
								"ERROR: value for '%s' more than 1 character",
								keyword);
						goto errorExit;
					}
					strcpy(separator, STRINGPTR(argi));
				}
				else if (strcmp(keyword,"header") == 0)
				{
					header = (DATAVALUE(argi,0) == 0.0) ? 0 : 1;
				}
				else if (strcmp(keyword,"labels") == 0)
				{
					labels = (DATAVALUE(argi,0) == 0.0) ? 0 : 1;
				}
				else if (strcmp(keyword,"notes") == 0)
				{
					notes = (DATAVALUE(argi,0) == 0.0) ? 0 : 1;
				}
				else if (strcmp(keyword,"macroname") == 0)
				{
					macroname = (DATAVALUE(argi,0) == 0.0) ? 0 : 1;
				}
				else if (strcmp(keyword,"nsig") == 0)
				{
					dsig  = DATAVALUE(argi,0);
					if (isMissing(dsig))
					{
						sprintf(OUTSTR, missingValueError, keyword);
						goto errorExit;
					}
					if (dsig <= 0.0 || dsig != floor(dsig))
					{
						notPositiveInteger(keyword);
						goto errorExit;
					}
					nsig = (long) dsig;
					fmt[0] = nsig+7;
					fmt[1] = nsig;
					fmt[2] = (long) 'g';
				}
				else if (strcmp(keyword,"format") == 0)
				{
					if (!setFormat(STRINGPTR(argi),fmt))
					{
						sprintf(OUTSTR,
								"ERROR: bad format specifier '%s'",
								STRINGPTR(argi));
						goto errorExit;
					}
					nsig = fmt[1];
				}
				else if (strcmp(keyword,"missing") == 0)
				{
					if (op & IMAT)
					{
						missValue = DATAVALUE(argi,0);
						if (isMissing(missValue))
						{
							sprintf(OUTSTR, missingValueError, keyword);
							goto errorExit;
						} /*if (isMissing(missValue))*/
					}
					else
					{
						missingCode = STRING(argi);
					}
				}
				else if (strcmp(keyword,"name") == 0)
				{
					nameH = STRING(argi);
				}
				else if (strcmp(keyword, "quoted") == 0)
				{
					charFormat = (DATAVALUE(argi,0) != 0.0) ?
						BYQUOTEDFIELDS : 0;
				}
				else if (strcmp(keyword, "bylines") == 0)
				{
					charFormat = (DATAVALUE(argi,0) != 0.0) ?
						BYLINES : 0;
				}
				else if (strcmp(keyword, "comments") == 0)
				{
					symhComment = argi;
				}
				else if ((isheight = (strcmp(keyword,"height") == 0)) ||
						 strcmp(keyword,"width") == 0)
				{
					double         value = DATAVALUE(argi,0);
					long           ivalue;
					long           minvalue =
						((isheight) ? MINSCREENHEIGHT : MINSCREENWIDTH);
												   
					if (isMissing(value))
					{
						sprintf(OUTSTR, missingValueError, keyword);
						goto errorExit;
					}
					if (value <= 0.0 || value != floor(value))
					{
						notPositiveInteger(keyword);
						goto errorExit;
					}
					ivalue = (long) value;
					if (ivalue < minvalue)
					{
						sprintf(OUTSTR,
								"ERROR: value for %s is %ld < %ld; illegal",
								keyword, ivalue, minvalue);
						goto errorExit;
					}
					if (isheight)
					{
						height = ivalue;
					} /*if (isheight)*/
					else
					{
						width = ivalue;
					}
				}
				else if (strcmp(keyword, "oldstyle") == 0)
				{
					oldStyle = (DATAVALUE(argi,0) != 0.0) ? 1 : 0;
				}
				unusedArgs--;
			} /*if (key->legalOps & keyOp)*/
			else if (strcmp(keyword, USEDKEYWORD+2) != 0)
			{  /* not recognized as a legal keyword for this command */
				restoreDefault = 1;
			}
		} /*if ((keyword = isKeyword(argi)))*/
		else /* not a keyword */
		{
			restoreDefault = 1;
		}
		if (restoreDefault)
		{
			nameH = (char **) 0;
			setMissing(missValue); /* wipes out previous missValue*/
			missingCode = (char **) 0;
			nsig = -1; /*wipes out previous format or nsig keywords*/
			width = height = -1;
			charFormat = 0;
			header = labels = notes = -1; /* restore default */
			separator[0] = '\0';
			symhComment = (Symbolhandle) 0;
			oldStyle = 0;
		} /*if (restoreDefault)*/
	} /*for (i=(toFile == 1) ? 1 : 0;i<nargs;i++)*/

	if (unusedArgs == 0)
	{
		sprintf(OUTSTR,"ERROR: nothing specified to print or write");
		goto errorExit;
	} /*if (unusedArgs == 0)*/
	
	if (toFile)
	{
		if (fileMode == (char *) 0)
		{ /* default mode is append (new:T) */
			fileMode = TEXTAPPENDMODE;
		}
		
#ifdef HASFINDFILE
#ifdef MACINTOSH
		fileName = macFindFile(fileName, "\pFile for output:", "\p",WRITEIT,
							   0, (OSType *) 0, &PrintVolume);
#endif /*MACINTOSH */
#ifdef WXWIN
		fileName = wxFindFile(fileName, "File for output:", (char *) 0); 
#endif /*WXWIN*/
		if (fileName == (char *) 0)
		{
			goto cleanup;
		}
#endif /*HASFINDFILE*/
		fileName = expandFilename(fileName);
		if (fileName == (char *) 0 || !isfilename(fileName))
		{
			goto cleanup;
		}
	} /* if (toFile) */

	if (toFile && !useConsole)
	{
		outFile = fmyopen(fileName,fileMode);
		if (outFile == (FILE *) 0)
		{
			sprintf(OUTSTR,"ERROR: %s() cannot open %s for writing",
					FUNCNAME,STRINGPTR(arg1));
			putErrorOUTSTR();
			goto cleanup;
		}
#ifdef MACINTOSH
		macSetInfo(PrintVolume,fileName,'TEXT',TEXTCREATOR);
#endif /*MACINTOSH*/
	} /*if (toFile && !useConsole)*/

	if (nsig < 0)
	{ /* no format has been set */
		if (op & IWRITE)
		{
			for (i=0;i<3;i++)
			{
				fmt[i] = WRITEFORMAT[i];
			}
		}
		else
		{
			for (i=0;i<3;i++)
			{
				fmt[i] = PRINTFORMAT[i];
			}
		}
	} /*if (nsig < 0)*/

	installFormat(fmt[0],fmt[1],fmt[2]);

	/* Note: SCREENWIDTH and SCREENHEIGHT are restored by restoreFormat() */
	installScreenDims((width > 0) ? width : SCREENWIDTH,
					  (height > 0) ? height : SCREENHEIGHT);
	
	if ((op & (IWRITE|IPRINT)) != 0 && (op & IMAT) == 0 && unusedArgs == 1
	   && nameH == (char **) 0 && header <= 0 && labels <= 0 && notes <= 0)
	{
		/*
		   only one thing to write or print
		*/
		for (i = (toFile == 1) ? 1 : 0;i < nargs;i++)
		{
			argi = COMPVALUE(list,i);
			if (!argOK(argi, NULLSYM,i+1))
			{
				goto cleanup;
			}
			if ((keyword = isKeyword(argi)) && strcmp(keyword, "name") == 0 || 
				(TYPE(argi) == LOGIC && DATAVALUE(argi, i) != 0.0 &&
				 (strcmp(keyword, "labels") == 0 ||
				  strcmp(keyword, "notes") == 0)))
			{
				break;
			}
			
			if (!isKeyword(argi) && isCharOrString(argi))
			{/* if single character variable, print as is, no header */
				char      *errorStart = "ERROR: ";
				
				if ((op & IERR) && strncmp(STRINGPTR(argi), errorStart,6) != 0
					&& strncmp(STRINGPTR(argi),"WARNING:", 8) != 0)
				{
					myprint(errorStart);
				}
				if (*STRINGPTR(argi) == '\0')
				{
					fmyeol(outFile);
				}
				else
				{
					mymultiprint(STRINGPTR(argi), outFile,
						(op & IERR) == 0 || !macroname);
				}
				if (op & IERR && macroname)
				{
					if (STRINGVALUE(argi, 0))
					{
						myprint(inWhichMacro());
					}
					fmyeol(outFile);
				}				
				unusedArgs = 0;
				break;
			}
		} /*for (i = (toFile == 1) ? 1 : 0;i < nargs;i++)*/
	} /*if ((op&(IWRITE|IPRINT)) != 0 && (op&IMAT) == 0 && unusedArgs == 1 ...)*/

	for (i = (toFile == 1) ? 1 : 0; i < nargs && unusedArgs > 0; i++)
	{
		argi = COMPVALUE(list, i);
		if (!argOK(argi, NULLSYM,i+1))
		{
			goto cleanup;
		}
		type = TYPE(argi);

		/*
		   960827 Keywords should all have legal values and need not be checked
		*/
		if ((keyword = isKeyword(argi)))
		{
			for (key = LegalKeys;key->type != 0;key++)
			{
				if (strcmp(keyword,key->name) == 0)
				{
					break;
				}
			} /*for (key = LegalKeys;key->type != 0;key++)*/

			if (key->legalOps & keyOp)
			{
				if (strcmp(keyword,"format") == 0)
				{
					if (setFormat(STRINGPTR(argi),fmt))
					{
						installFormat(fmt[0],fmt[1],fmt[2]);
					}
				} /*if (strcmp(keyword,"format") == 0)*/
				else if (strcmp(keyword,"nsig") == 0)
				{
					nsig = (long) DATAVALUE(argi, 0);
					installFormat(nsig+7,nsig,(long)'g');
				}
				else if (strcmp(keyword,"missing") == 0)
				{
					if (op & IMAT)
					{
						missValue = DATAVALUE(argi,0);
					}
					else
					{
						missingCode = STRING(argi);
					}
				}
				else if (strcmp(keyword,"sep") == 0)
				{
					strcpy(separator, STRINGPTR(argi));
				}
				else if (strcmp(keyword,"header") == 0)
				{
					header = (DATAVALUE(argi,0) == 0.0) ? 0 : 1;
				}
				else if (strcmp(keyword,"labels") == 0)
				{
					labels = (DATAVALUE(argi,0) == 0.0) ? 0 : 1;
				}
				else if (strcmp(keyword,"notes") == 0)
				{
					notes = (DATAVALUE(argi,0) == 0.0) ? 0 : 1;
				}
				else if (strcmp(keyword,"name") == 0)
				{
					nameH = STRING(argi);
				}
				else if (strcmp(keyword, "quoted") == 0)
				{
					charFormat = (DATAVALUE(argi,0) != 0.0) ?
						BYQUOTEDFIELDS : 0;
				}
				else if (strcmp(keyword, "bylines") == 0)
				{
					charFormat = (DATAVALUE(argi,0) != 0.0) ?
						BYLINES : 0;
				}
				else if (strcmp(keyword, "comments") == 0)
				{
					symhComment = argi;
				}
				else if ((isheight = (strcmp(keyword,"height") == 0)) ||
						 strcmp(keyword,"width") == 0)
				{
					long      ivalue = (long) DATAVALUE(argi, 0);
					
	/* Note: SCREENWIDTH and SCREENHEIGHT are restored by restoreFormat() */
					installScreenDims((!isheight) ? ivalue : SCREENWIDTH,
									  (isheight) ? ivalue : SCREENWIDTH);
				}
				else if (strcmp(keyword, "oldstyle") == 0)
				{
					oldStyle = (DATAVALUE(argi,0) != 0.0) ? 1 : 0;
				}
				setNAME(argi,USEDKEYWORD);
			} /*if (key->legalOps & keyOp)*/			
			if (strcmp(NAME(argi), USEDKEYWORD) == 0)
			{
				argi = (Symbolhandle) 0;
			}
		} /*if ((keyword = isKeyword(argi)))*/

		if (argi != (Symbolhandle) 0)
		{
			unusedArgs--;
		}
		
		if (argi != (Symbolhandle) 0 && (op & (IMAT + IMACRO)) &&
			type == STRUC)
		{
			if (strucAnyType(argi, PLOTINFO))
			{
				sprintf(OUTSTR,
						"WARNING: %s() will write GRAPH components of structure as %s component",
						FUNCNAME, NULLNAME);
				putErrorOUTSTR();
			}
		} /*if (argi != (Symbolhandle) 0 && type == STRUC)*/

		if (argi != (Symbolhandle) 0)
		{
			name = (nameH != (char **) 0) ? *nameH : NAME(argi);
			nameH = (char **) 0; /* use name only once */
			if (op & (IMAT + IMACRO))
			{
				if (type == REAL || type == LOGIC || type == CHAR ||
					type == STRUC || type == NULLSYM || type == PLOTINFO)
				{
					if (op & IMACRO)
					{
						sprintf(OUTSTR,
								"WARNING: %s variable written by %s()",
								typeName(type), FUNCNAME);
						putErrorOUTSTR();
					}
				}
				else if (type == MACRO)
				{
					if (op & IMAT)
					{
						sprintf(OUTSTR,
								"WARNING: %s written by %s()",
								typeName(type), FUNCNAME);
						putErrorOUTSTR();
					}
				}
				else
				{ /* not REAL, LOGIC, CHAR, MACRO, STRUC or NULLSYM*/
					sprintf(OUTSTR,
							"ERROR: %s() cannot write variable of type %s",
							FUNCNAME, typeName(type));
					putErrorOUTSTR();
					goto cleanup;
				}
				/* 971103 all calls routed through matWrite()*/
				matWrite(argi, outFile, missValue,
						 (header < 0) ? (separator[0] == '\0') : header,
						 labels, notes, separator, charFormat, oldStyle,
						 name, symhComment);
						 
				symhComment = (Symbolhandle) 0;
			} /*if (op & (IMAT + IMACRO))*/
			else
			{/* not matwrite, matprint, or macrowrite */
				if (header)
				{
					if (isKeyword(argi) || isscratch(name))
					{
						name += 2;
					}
					else if (argi == NULLSYMBOL)
					{
						name = "NULL";
					}
					sprintf(OUTSTR,"%s:", name);
					fmyprint(OUTSTR,outFile);
					*OUTSTR = '\0';
					fmyeol(outFile);
				}	
				fprexpr(argi,outFile,labels, missingCode);
			} /*if (op & (IMAT + IMACRO)){...}else{...}*/
		} /*if (argi != (Symbolhandle) 0)*/

		if (INTERRUPT != INTNOTSET)
		{
			if (INTERRUPT != PRINTABORT)
			{
				goto cleanup;
			}
			break;
		}
	} /*for (i = (toFile == 1) ? 1 : 0; i < nargs; i++)*/

	result = (op != IERROR) ? NULLSYMBOL : (Symbolhandle) 0;

	goto cleanup;

  repeatedKey:
	sprintf(OUTSTR,
			"ERROR: repeated use of keyword '%s' as argument to %s()",
			keyword, FUNCNAME);
/* fall through*/
  errorExit:
	putErrorOUTSTR();

  cleanup:
	if (outFile != (FILE *) 0 && outFile != STDOUT)
	{
		fclose(outFile);
		outFile = (FILE *) 0;
	}
	restoreFormat();

	*OUTSTR = '\0';
	
	return (result);
} /*print()*/

/*
  putascii(vector), where vector[i] > 0 && vector[i] < 256
  routine to output raw ascii codes

  getascii(charVec1 [,charVec2 ...]) translates character variables to
  a vector of integer ascii codes 

  960502 changed fopen() to fmyopen()
  980310 now checks for MISSING in arguments
  980903 added code for getascii()
*/
#define GBUFFER  0

Symbolhandle    putascii(Symbolhandle list)
{

	Symbolhandle    arg1, symh;
	Symbolhandle    result = (Symbolhandle) 0;
	long            i, j, n = 0, length;
	long            nargs = NARGS(list);
	int             new = 0, toFile = 0, keep = 0;
	int             put = strcmp(FUNCNAME, "putascii") == 0;
	double          val;
	char           *keyword, *fileName = (char *) 0;
	char           *fileMode = TEXTAPPENDMODE;
	unsigned char **buffer = (unsigned char **) 0;
	FILE           *fp = STDOUT;
	WHERE("putascii");
	TRASH(1,errorExit);
	
	SETUPINT(errorExit);

	if ((arg1 = COMPVALUE(list,0)) == (Symbolhandle) 0 && nargs == 1)
	{
		nargs = 0;
	}
	
	for (i=nargs-1;i>=0;i--)
	{
		symh = COMPVALUE(list,i);
		if (!(keyword = isKeyword(symh)))
		{
			break;
		}
		if (!put)
		{ /* no keywords allowed on getascii()*/
			badKeyword(FUNCNAME, keyword);
			goto errorExit;
		}
		if (strcmp(keyword,"file") == 0)
		{
			if (!isCharOrString(symh))
			{
				notCharOrString("'file'");
				goto errorExit;
			}
			toFile = 1;
			fileName = STRINGPTR(symh);
		}
		else if (strcmp(keyword,"new") == 0 || strcmp(keyword,"keep") == 0)
		{
			int      logicalValue;
			
			if (!isTorF(symh))
			{
				notTorF(keyword);
				goto errorExit;
			}
			logicalValue = (DATAVALUE(symh, 0) != 0.0);
			
			if (keyword[0] == 'n')
			{
				new = logicalValue;
			}
			else
			{
				keep = logicalValue;
			}
		}
		else
		{
			badKeyword(FUNCNAME, keyword);
			goto errorExit;
		}
		nargs--;
	} /*for (i=nargs-1;i>=0;i--)*/

	if (new && !toFile)
	{
		sprintf(OUTSTR,
				"ERROR: 'new:T' illegal without 'file:fileName'");
	}
	else if (keep && toFile)
	{
		sprintf(OUTSTR,
				"ERROR: 'keep:T' is illegal with 'file:fileName'");
	}
	else if (nargs == 0)
	{
		sprintf(OUTSTR,
				"ERROR: no data supplied for %s()", FUNCNAME);
	}

	if (*OUTSTR)
	{
		goto errorExit;
	}
	
	for (i = 0;i < nargs;i++)
	{
		symh = COMPVALUE(list,i);
		if (!argOK(arg1,0,i+1))
		{
			goto errorExit;
		}
		if (!isVector(symh) || TYPE(symh) != ((put) ? REAL : CHAR) ||
			put && anyMissing(symh))
		{
			sprintf(OUTSTR,
					"ERROR: data arguments to %s() must be %s scalars or vectors",
					FUNCNAME, (put) ? "non-MISSING REAL" : "CHARACTER");
			goto errorExit;
		}
		n += symbolSize(symh);
	} /*for (i = 0;i < nargs;i++)*/
	
	if (toFile)
	{
		if (new)
		{ /* default mode is "a" (new:F) */
			fileMode = TEXTWRITEMODE;
		}

#ifdef HASFINDFILE
#ifdef MACINTOSH
		fileName = macFindFile(fileName, "\pFile for putascii():", "\p",
							   WRITEIT, 0, (OSType *) 0, &PrintVolume);
#endif /*MACINTOSH */

#ifdef WXWIN
		fileName = wxFindFile(fileName, "File for putascii():", (char *) 0);
#endif /* WXWIN */
		if (fileName == (char *) 0)
		{
			goto errorExit;
		}
#endif /*HASFINDFILE*/
		fileName = expandFilename(fileName);
		if (fileName == (char *) 0 || !isfilename(fileName))
		{
			goto errorExit;
		}
		fp = fmyopen(fileName,fileMode);
		if (fp == (FILE *) 0)
		{
			sprintf(OUTSTR,"ERROR: %s() cannot open file %s",
					FUNCNAME, fileName);
			goto errorExit;
		}
	} /* if (toFile) */
		
	if (put)
	{ /*putascii()*/
		if (!getScratch(buffer, GBUFFER, n+1, unsigned char))
		{
			goto errorExit;
		}

		n = 0;
		for (i = 0;i < nargs;i++)
		{
			symh = COMPVALUE(list,i);
			length = symbolSize(symh);
			for (j = 0;j < length;j++)
			{
				val = DATAVALUE(symh,j);
				if (floor(val) != val || val <= 0.0 || val > 255.0)
				{
					sprintf(OUTSTR,
							"ERROR: all values for %s() must be integers >= 1 and <= 255",
							FUNCNAME);
					goto errorExit;
				}
				(*buffer)[n++] = (unsigned char) val;
			} /*for (j = 0;j < length;j++)*/
		} /*for (i = 0;i < nargs;i++)*/
		(*buffer)[n] = '\0';

		if (!keep)
		{
			fmyprint((char *) *buffer, fp);
			fmyflush(fp);
			result = NULLSYMBOL;
		}
		else
		{
			if ((result = Install(SCRATCH,CHAR)) == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			unTrash(GBUFFER);
			setSTRING(result,(char **) buffer);
			setNDIMS(result,1);
			setDIM(result,1,1);
		}
	} /*if (put)*/
	else
	{ /*getascii()*/
		double    *data;
		long       length = 0, k;
		char      *place;
		
		for (i = 0; i < nargs; i++)
		{
			long       size;
			
			symh = COMPVALUE(list, i);
			
			place = STRINGPTR(symh);
			size = DIMVAL(symh, 1);
			for (j = 0; j < size; j++)
			{
				length += strlen(place);
				place = skipStrings(place, 1);
			}
		} /*for (i = 0; i < nargs; i++)*/

		if (length > 0)
		{
			result = RInstall(SCRATCH, length);
			if (result == (Symbolhandle) 0)
			{
				goto errorExit;
			}

			k = 0;
			data = DATAPTR(result);
			for (i = 0; i < nargs; i++)
			{
				long       size;
			
				symh = COMPVALUE(list, i);
			
				place = STRINGPTR(symh);
				size = DIMVAL(symh, 1);
				for (j = 0; j < size; j++)
				{
					unsigned char     *pc = (unsigned char *) place;
				
					while (*pc)
					{
						data[k++] = (double) *pc++;
					}
					place = skipStrings(place, 1);
				} /*for (j = 0; j < size; j++)*/
			} /*for (i = 0; i < nargs; i++)*/
		} /*if (length > 0)*/
		else
		{
			result = NULLSYMBOL;
		}
	} /*if (put){}else{}*/
	
	/* fall through */

  errorExit:
	putErrorOUTSTR();

	if (toFile && fp != STDOUT && fp != (FILE *) 0)
	{
		fclose(fp);
	}
	emptyTrash();
	
	return (result);
} /*putascii()*/
