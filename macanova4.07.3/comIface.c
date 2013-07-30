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
#ifdef MACINTOSH
#include "macIface.h"
#endif /*MACINTOSH*/

#ifdef WXWIN
#include "wx/wxIface.h"
#endif  /* WXWIN */

/*
  Functions for access system related aspects of MacAnova
  As of 971210 only gethistory() is implemented.  This would be a place
  to put functions like setmenu(), setmenuitem(), closewindow(), newwindow()
  etc., if they existed

  990226 replaced putOUTSTR() by putErrorOUTSTR()
*/

/*
  usage:
  gethistory()    get entire history list as character VECTOR
  gethistory(n)   get most recent n commands; give warning if fewer than n
*/

#ifdef SAVEHISTORY
Symbolhandle getHistory(Symbolhandle list)
{
	Symbolhandle   result = (Symbolhandle) 0;
	Symbolhandle   arg;
	char         **commands = (char **) 0;
	long           nargs = NARGS(list);
	long           nlines, mlines;
	int            get = FUNCNAME[0] == 'g';

	if (!get)
	{ /*sethistory()*/
		if (nargs > 1 || (arg = COMPVALUE(list, 0)) == (Symbolhandle) 0)
		{
			badNargs(FUNCNAME, 1);
			goto errorExit;
		}
		if (!isVector(arg) || TYPE(arg) != CHAR)
		{
			sprintf(OUTSTR,
					"ERROR: %s() argument is not CHARACTER vector", FUNCNAME);
			goto errorExit;
		}
		nlines = symbolSize(arg);
		if (nlines > HISTORY)
		{
			sprintf(OUTSTR,
					"ERROR: attempt to restore a history of more than %ld commands",
					HISTORY);
			goto errorExit;
		} /*if (symbolSize(arg) > HISTORY)*/

		setSomeHistory(nlines, STRING(arg));

		result = NULLSYMBOL;
	} /*if (!get)*/
	else
	{ /*sethistory()*/
		if (nargs > 1)
		{
			badNargs(FUNCNAME, -1);
			goto errorExit;
		}
		arg = COMPVALUE(list, 0);
		if (arg != (Symbolhandle) 0)
		{
			if (!isInteger(arg, POSITIVEVALUE))
			{
				notPositiveInteger("argument 1");
				goto errorExit;
			}
			nlines = (long) DATAVALUE(arg, 0);
		}
		else
		{
			nlines = 0;
		}

		result = Install(SCRATCH, CHAR);
		if (result == 0)
		{
			goto errorExit;
		}
		
		mlines = getSomeHistory(nlines, &commands);

		setSTRING(result, commands);
		if (commands == (char **) 0)
		{
			Removesymbol(result);
			goto errorExit;
		}
		
		setNDIMS(result, 1);
		setDIM(result, 1, mlines);

		if (nlines > 0)
		{
			if (mlines == 1 && (*commands)[0] == '\0')
			{
				mlines = 0;
			}

			if (mlines < nlines)
			{
				sprintf(OUTSTR,
						"WARNING: %s() retrieved only %ld past command lines",
						FUNCNAME, mlines);
				putErrorOUTSTR();
			}
		} /*if (result != (Symbolhandle) 0 && nlines > 0)*/
	} /*if (!get){}else{}*/

	return (result);

  errorExit:
	putErrorOUTSTR();
	return (0);
} /*Symbolhandle getHistory()*/
#else /*SAVEHISTORY*/
Symbolhandle getHistory(Symbolhandle list)
{
	notImplemented(FUNCNAME);
	return (0);
} /*getHistory()*/
#endif /*SAVEHISTORY*/

