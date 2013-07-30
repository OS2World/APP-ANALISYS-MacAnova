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
#pragma segment Batch
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

#ifdef MACINTOSH
#include "macIface.h"
#endif /*MACINTOSH*/

#ifdef WXWIN
#include "wx/wxIface.h"
#endif /*WXWIN*/


/*
  Routine to spool output to a file or suspend spooling
  Modified by kb to accept second logical argment:
  	spool(fileName [,new:T])
  If new:T is present, the file is opened with fopen(fileName,"w") instead of
  fopen(fileName,"a")
    spool()  toggles spooling on and off once started.
	spool(,new:T) restarts spooling on last specified file.

  SPOOLFILE should be non-zero if and only if spooling is in effect.
  SpoolFileName should be non-zero if and only if spooling is in effect
  or the last attempt to spool was successful.

  960502 Changed fopen() to fmyopen()
  960503 Changed macOpen() to macFindFile()
  960719 spool() now prints starting message after spool("")
         Changed a number of output places to use putPieces()
  970506 Use TEXTWRITEMODE and TEXTAPPENDMODE as fmyopen() mode
  990226 Replaced putOUTSTR() by putErrorOUTSTR() and myerrorout() by
         putOutErrorMsg()
*/

Symbolhandle    spool(Symbolhandle list)
{
	Symbolhandle    arg1, arg2;
	FILE           *spoolFile;
	char          **spoolFileName;
	long            rewindit = 0;
	long            doSpool = 0;
	long            nargs = NARGS(list);
	char           *keyword;
	char           *fileName;
	WHERE("spool");
	
#ifdef MACINTOSH
	if (!UseWindows)
	{ /*  */
		sprintf(OUTSTR,"WARNING: %s() disabled in non-interactive mode",
				FUNCNAME);
		goto errorExit;
	}
#endif /*MACINTOSH*/
	
	*OUTSTR = '\0';

	if(nargs > 2)
	{
		badNargs(FUNCNAME,-2);
		goto errorExit;
	}

	arg1 = COMPVALUE(list,0);
	if(arg1 != (Symbolhandle) 0)
	{
		if (TYPE(arg1) != CHAR || !isScalar(arg1))
		{
			notCharOrString("filename");
			goto errorExit;
		}
		fileName = STRINGPTR(arg1);
	} /*if(arg1 != (Symbolhandle) 0)*/		
	
	/* 2 arguments */
	if (nargs == 2)
	{ /* check to see if we need to rewind */
		arg2 = COMPVALUE(list,1);
		if((keyword = isKeyword(arg2)) && strcmp(keyword,"new") != 0)
		{ /* only legal keyword is "new" */
			badKeyword(FUNCNAME, keyword);
			goto errorExit;
		}
		if(arg2 == (Symbolhandle) 0 || TYPE(arg2) != LOGIC ||
		   !isScalar(arg2))
		{
			notTorF(keyword);
			goto errorExit;
		}
		
		rewindit = (DATAVALUE(arg2,0) != 0.0);
	} /*if (nargs == 2)*/
	
/* now see about file name */
	if (arg1 == (Symbolhandle) 0)
	{ /* none given */
		/* no argument, so stop or restart (new:T) spooling */
		if (SPOOLFILE != (FILE *) 0)
		{
			if(rewindit)
			{
				putPieces("Restart spooling on ", *SPOOLFILENAME,
						  (char *) 0, (char *) 0);
				doSpool = 1;
			}
			else
			{
				putPieces("Spooling on ", *SPOOLFILENAME," suspended", (char *) 0);
				doSpool = 0;
			}
			fclose(SPOOLFILE);
			SPOOLFILE = (FILE *) 0;
		}
		else if(SPOOLFILENAME == (char **) 0)
		{
			putOutErrorMsg("WARNING: not spooling at present");
			doSpool = 0;
		}
		else
		{
			spoolFileName = SPOOLFILENAME;
			SPOOLFILENAME = (char **) 0;
			putPieces((rewindit) ? "Restart" : "Resume", " spooling on ",
					  *spoolFileName, (char *) 0);
			doSpool = 1;
		}
	} /*if (arg1 == (Symbolhandle) 0)*/
	else if(SPOOLFILENAME == (char **) 0 || SPOOLFILE == (FILE *) 0 ||
			strcmp(*SPOOLFILENAME,fileName) != 0 || rewindit)
	{ /* new filename provided */
		if(SPOOLFILENAME != (char **) 0)
		{
			mydisphandle(SPOOLFILENAME);
			SPOOLFILENAME = (char **) 0;
		}
#ifdef HASFINDFILE
#ifdef MACINTOSH
		fileName = macFindFile(fileName, "\pSave spooled output as:", "\p",
							   WRITEIT, 0, (OSType *) 0, &SpoolVolume);
#endif /*MACINTOSH*/

#ifdef WXWIN
		fileName = wxFindFile(fileName, "Save spooled output as:", (char *) 0);
#endif /*WXWIN*/
		if (fileName == (char *) 0)
		{
			strcpy(OUTSTR,"WARNING: spooling request cancelled");
			goto errorExit;
		}
#endif /*HASFINDFILE*/
		fileName = expandFilename(fileName);
		if (fileName == (char *) 0 || !isfilename(fileName))
		{
			goto errorExit;
		}
		if((spoolFileName = mygethandle(strlen(fileName) + 1)) ==
		   (char **) 0)
		{
			goto errorExit;
		}
		strcpy(*spoolFileName,fileName);
		doSpool = 1;
	}
	else
	{
		putPieces("WARNING: already spooling on file ", *SPOOLFILENAME,
				  (char *) 0, (char *) 0);
		doSpool = 0;
	}
	
	if (doSpool)
	{
		if (SPOOLFILE != (FILE *) 0)
		{
			fclose(SPOOLFILE);
			SPOOLFILE = (FILE *) 0;
		}
		spoolFile = fmyopen(*spoolFileName, (rewindit) ?
							TEXTWRITEMODE : TEXTAPPENDMODE);
		if (spoolFile == (FILE *) 0)
		{
			putPieces("ERROR: cannot open file ", *spoolFileName,
					  " for spooling", (char *) 0);
			mydisphandle(spoolFileName);
			goto errorExit;
		}

#ifdef HASFINDFILE
		if (arg1 != (Symbolhandle) 0 && STRINGVALUE(arg1, 0) == '\0')
		{
			/* Because SPOOLFILE not yet set this should not be spooled */
			putPieces("Spooling started on file ", *spoolFileName,
					  (char *) 0, (char *) 0);
		}
#endif /*HASFINDFILE*/

		SPOOLFILE = spoolFile;
		SPOOLFILENAME = spoolFileName;

#ifdef MACINTOSH
		macSetInfo(SpoolVolume, *SPOOLFILENAME, 'TEXT', TEXTCREATOR);
#endif /*MACINTOSH*/
	} /*if (doSpool)*/

#ifdef MACINTOSH
	setCmdFileItems(FrontWindow() == CmdWind);
#endif /*MACINTOSH*/

	return (NULLSYMBOL);

  errorExit:
	putErrorOUTSTR();
	return (0);
	
} /*spool()*/
