/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 4.07 or later
*(C)*
*(C)* Copyright (c) 1998 by Gary Oehlert and Christopher Bingham
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
#pragma segment Utils
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

/*
   Contains various routines for emitting error messages
   960711 Added resultTooBig()

   970711 Added notPositiveLong(), notNonNegativeLong(), notLong()

   980727 Modified badLabels()

   990207 Added inWhichMacro()
   
   990211 Changed putOUTSTR() to putErrorOUTSTR()
*/

#include "globals.h"

/*
  Function to return the phrase " in macro XXXXX" for use in error
  messages

  Added 990207
*/
#define INMACROTMPLENGTH  (NAMELENGTH + 11) /* strlen(" in macro ") + 1 = 11*/
static char        InMacroTmp[INMACROTMPLENGTH];

char *inWhichMacro(void)
{
	if (ACTIVEMACROS <= 0)
	{
		InMacroTmp[0] = '\0';
	}
	else
	{
		sprintf(InMacroTmp, " in macro %s", ThisMacroName);
	}
	return ((char *) InMacroTmp);
} /*inWhichMacro()*/

static void emitMsg(char * who, long argno, char *msg, char * name)
{
	char       *name1 = (name != (char *) 0) ? name : "";
	
	if(argno != 0)
	{
		sprintf(OUTSTR,"ERROR: argument %ld%s to %s()%s",
				labs(argno), name1, who, msg);
	}
	else if(argno == 0)
	{
		sprintf(OUTSTR,"ERROR: argument%s to %s()%s", name1, who, msg);
	}
	putErrorOUTSTR();
} /*emitMsg()*/

void badType(char * who, long type, long argno)
{
	char         msg[100];
	
	if(type > 0)
	{
		sprintf(msg, " must have type %s", typeName(type));
	}
	else if(type == -UNDEF || type == -ASSIGNED)
	{
		strcpy(msg, " is not defined");
	}
	else		
	{
		sprintf(msg," must not have type %s", typeName(-type));
	}
	emitMsg(who, argno, msg, (char *) 0);
} /*badType()*/

void badDims(char * who, long ndims, long argno)
{
	char      msg[100];

	if (ndims > 0)
	{
		sprintf(msg, "does not have %ld dimensions", ndims);
	}
	else if (ndims < 0)
	{
		sprintf(msg, "has more than %ld dimensions", -ndims);
	}
	else
	{
		sprintf(msg, "has the wrong number of dimensions");
	}
	emitMsg(who, argno, msg, (char *) 0);
} /*badDims()*/

/*
  Emit error message relative to the wrong number of arguments
  nargs         Meaning
  0             No arguments provided; at least 1 expected
  k > 0         Exactly k arguments required; actual ~= k
  k == -1000    Argument found but none allowed
  k <  -1000    At least -k - 1000 arguments required
  -1000 < k < 0 At most -k arguments allowed

  981228 fixed minor bug involving plurals
*/

void badNargs(char *who, long nargs)
{
	char    *plural = "s";
	
	if(nargs == 0)
	{
		sprintf(OUTSTR,"ERROR: no argument(s) given for %s()",who);
	}
	else if(nargs > 0)
	{
		sprintf(OUTSTR,"ERROR: %s() requires %ld argument%s",who, nargs,
				(nargs > 1) ? plural : NullString);
	}
	else if(nargs > -1000)
	{
		nargs = -nargs;
		sprintf(OUTSTR,"ERROR: %s() may have at most %ld argument%s",who,
				nargs, (nargs > 1) ? plural : NullString);
	}
	else if(nargs == -1000)
	{
		sprintf(OUTSTR,"ERROR: %s() should have no arguments", FUNCNAME);
	}
	else
	{
		nargs = -nargs - 1000;
		sprintf(OUTSTR,"ERROR: %s() must have at least %ld argument%s",who,
				nargs, (nargs > 1) ? plural : NullString);
	}
		
	putErrorOUTSTR();
} /*badNargs()*/

void invalidSymbol(char * who, long argno)
{
	emitMsg(who, argno, " damaged (deleted ?)", (char *) 0);
} /*invalidSymbol()*/

void noData(char * who, long argno)
{
	emitMsg(who, argno, " is missing", (char *) 0);
} /*noData()*/

/*
   should be called only when arg != (Symbolhandle) 0.  None the less
   we play it safe

   980420 no longer gives name of NULLSYMBOL when that is the offending symbol
*/
void undefArg(char * who, Symbolhandle arg, long argno)
{
	char        name[NAMELENGTH+4];
	char       *symptom;
	
	if (arg == (Symbolhandle) 0)
	{
		noData(who, argno);
	} /*if (arg == (Symbolhandle) 0)*/
	else
	{
		symptom = (TYPE(arg) == UNDEF || TYPE(arg) == ASSIGNED) ?
		  " is not defined" : " is NULL";
		if (!isscratch(NAME(arg)) && arg != NULLSYMBOL)
		{
			sprintf(name," (%s)", NAME(arg));
		}
		else
		{
			name[0] = '\0';
		}
		
		emitMsg(who, argno, symptom, name);
	} /*if (arg == (Symbolhandle) 0){}else{}*/

} /*undefArg()*/

void badKeyword(char * who, char * keyword)
{
	sprintf(OUTSTR,"ERROR: %s is not valid keyword for %s()",
			keyword, who);
	putErrorOUTSTR();
}

/*
  980803 changed "must be" in error message to "is not"
*/
static void badScalar(char *keyword, char *what)
{
	sprintf(OUTSTR,
			"ERROR: value%s%s is not %s",
			(keyword) ? " for " : "", (keyword) ? keyword : "",
			what);
	putErrorOUTSTR();
} /*badScalar()*/

/*
  980727 modified so that "ERROR: " message is printed whenever
         whatError & LABELSERROR != 0
*/
void badLabels(unsigned long whatError)
{
	int       labelsError = (whatError & LABELSERROR);
	char     *prefix = (labelsError) ? "ERROR: " : "WARNING: ";
	char     *msg, *tag;
	
	if (whatError != LABELSOK)
	{
		if (whatError & WRONGSIZELABELS)
		{
			msg = "%ssizes of labels do not match dimensions on %s()%s";
			tag = (labelsError) ? NullString : "; ignored";
		}
		else if (whatError & TOOMANYLABELS)
		{
			msg = "%sextra vectors of labels supplied to %s()%s";
			tag = (labelsError) ? NullString : " are ignored";
		}
		else if (whatError & TOOFEWLABELS)
		{
			msg = "%stoo few vectors of labels supplied to %s()%s";
			tag = (labelsError) ?
			  NullString : "; missing assumed \"@\"";
		}
		else if (whatError == WRONGTYPEVECTOR)
		{
			msg = "%stype of non-structure value for 'labels' on %s() not CHARACTER%s";
			tag = NullString;
		}
		else if (whatError == WRONGTYPECOMP)
		{
			msg = "%sa component of 'labels' on %s() is not CHARACTER vector%s";
			tag = NullString;
		}
		else
		{
			msg = "%sproblem with value for 'labels' in %s()%s";
			tag = NullString;
		}
		sprintf(OUTSTR, msg, prefix, FUNCNAME, tag);
		putErrorOUTSTR();
	} /*if (whatError != LABELSOK)*/
	
} /*badLabels()*/

void notTorF(char *keyword)
{
	badScalar(keyword,"T or F");
}

void notCharOrString(char *keyword)
{
	badScalar(keyword, "quoted string or CHARACTER scalar");
}

void notPositiveReal(char *keyword)
{
	badScalar(keyword, "positive REAL");
}

void notNonNegativeReal(char *keyword)
{
	badScalar(keyword, "non-negative REAL");
}

void notNumberOrReal(char *keyword)
{
	badScalar(keyword, "number or REAL scalar");
}

void notPositiveInteger(char *keyword)
{
	badScalar(keyword, "positive REAL integer");
}

void notNonNegativeInteger(char *keyword)
{
	badScalar(keyword, "non-negative REAL integer");
}

void notInteger(char *keyword)
{
	badScalar(keyword, "a REAL integer");
}

void notPositiveLong(char *keyword)
{
	badScalar(keyword, "positive LONG integer");
}

void notNonNegativeLong(char *keyword)
{
	badScalar(keyword, "non-negative LONG integer");
}

void notLong(char *keyword)
{
	badScalar(keyword, "a LONG integer");
}

void notImplemented(char *who)
{
	sprintf(OUTSTR,"ERROR: '%s()' is not implemented in this version",who);
	putErrorOUTSTR();
}

void resultTooBig(char *who)
{
	sprintf(OUTSTR, "ERROR: too large result to be returned by %s()", who);
	putErrorOUTSTR();
} /*resultTooBig()*/
