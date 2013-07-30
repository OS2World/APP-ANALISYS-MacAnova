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
#pragma segment Iswhat
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
/*
  Operations isreal(), ischar(), islogic(), isvector(), ismatrix(),
  isscalar(), isfactor(), isnull(), ismacro(), isname()

  Each returns a logical vector of length NARGS(list)

  950821 isscalar(), isvector(), and ismatrix() all recognize keyword phrases
  real:T, char:T, and logic:T.

  980317 added isarray() which also recognizes real, char and logic

  990113 OK to repeat properties (e.g., vector("real","real")) on argvalue()
         and keyvalue()

  990226 Replaced putOUTSTR() by putErrorOUTSTR()

  990325 Added implemntation of isname()
*/
/*
  isVariableName(name) returns 1 if and only if name is a legal, possibly
  temporary, MacAnova name.  "NULL", "F", and "T" are not legal

  At some point this may go into utils.c
*/
static int isVariableName(char * name)
{
	int           nameOK = 0;

	if (strlen(name) <= NAMELENGTH &&
		(strcmp(name, NULLNAME) != 0 &&
		 (name[1] != '\0' || name[0] != 'T' && name[0] != 'F')))
	{
		if (istempname(name))
		{
			name++;
		}
		nameOK = isnamestart(*name);

		while (nameOK && *(++name) != '\0')
		{
			nameOK = isnamechar(*name);
		}
	}
	return (nameOK);
} /*isVariableName()*/


/*
  doIsname() implements MacAnova function isname()

  isname(arg1, arg2, ..., argk) where each argument is either missing
  or a CHARACTER scalar, returns a LOGICAL vector of length nargs.  Element
  k of the result is T is and only if argument k is a CHARACTER scalar whose
  value is a legal MacAnova name.  Values NULL, T and F are explicitly
  checked for 

  990235
*/
static Symbolhandle doIsname(Symbolhandle list)
{
	Symbolhandle        arg, result = (Symbolhandle) 0;
	long                nargs = NARGS(list);
	int                 iarg;
	
	for (iarg = 0; iarg < nargs; iarg++)
	{
		arg = COMPVALUE(list, iarg);
		if (arg != (Symbolhandle) 0 &&
			(!isScalar(arg) || TYPE(arg) != CHAR))
		{
			sprintf(OUTSTR,
					"ERROR: argument %d to %s() is not CHARACTER scalar",
					iarg + 1, FUNCNAME);
			goto errorExit;
		}
	}
	result = RInstall(SCRATCH, nargs);
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	setTYPE(result, LOGIC);
	for (iarg = 0; iarg < nargs; iarg++)
	{
		double      value = 0.0;
		
		arg = COMPVALUE(list, iarg);
		if (arg != (Symbolhandle) 0 && isVariableName(STRINGPTR(arg)))
		{
			value = 1.0;
		}
		DATAVALUE(result, iarg) = value;
	} /*for (iarg = 0; iarg < nargs; iarg++)*/
	
	return (result);
	
  errorExit:
	putErrorOUTSTR();
	return (0);
} /*doIsname()*/

enum iswhatOps
{
	IREAL = 0,
	ILOGIC,
	ICHAR,
	NKEYS
};

static char     *TypeKeys[NKEYS] = {"real", "logic", "char"};

#define RealOK      okTypes[IREAL]
#define LogicOK     okTypes[ILOGIC]
#define CharOK      okTypes[ICHAR]

Symbolhandle iswhat(Symbolhandle list)
{
	long               nargs = NARGS(list);
	long               i, j, type, value;
	Symbolhandle       symh, result = (Symbolhandle) 0;
	Symbolhandle       symhKey;
	long               dims[2];
	char              *what = FUNCNAME + 2;
	char              *keyword;
	long               keyIndex, checkType = 0;
	short              okTypes[NKEYS];
	WHERE("iswhat");

	OUTSTR[0] = '\0';

	if (strcmp(what, "name") == 0)
	{
		return (doIsname(list));
	}
	
	if (strcmp(what, "scalar") == 0 || strcmp(what, "vector") == 0 ||
		strcmp(what, "matrix") == 0 || strcmp(what, "array") == 0)
	{ /* check for keywords */
		for (i=0; i < NKEYS; i++)
		{
			keyword = TypeKeys[i];
			okTypes[i] = -1;
			if ((keyIndex = findKeyword(list, keyword, 1)) >= 0)
			{
				nargs--;
				checkType = 1;
				symhKey = COMPVALUE(list,keyIndex);
				if (!isTorF(symhKey))
				{
					notTorF(keyword);
					goto errorExit;
				}
				okTypes[i] = (DATAVALUE(symhKey,0) != 0.0);
				setNAME(symhKey, USEDKEYWORD);
			} /*if ((keyIndex = findKeyword(list, keyword, 1)) >= 0)*/
		} /*for (i=0; i < NKEYS; i++)*/

		/*
		   All types, REAL, LOGIC, or CHARACTER are ok unless any of
		   the keywords real, logic, or char appear.  In the latter case, only
		   types that are explicitly indicated by, e.g., char:T, are ok.
		*/
		if (checkType)
		{/* some keyword found */
			for (i = 0; i < 3; i++)
			{ /* set unset types to false */
				okTypes[i] = (okTypes[i] < 0) ? 0 : okTypes[i];
			} /*for (i = 0; i < 3; i++)*/
			if (okTypes[0] + okTypes[1] + okTypes[2] == 0)
			{
				sprintf(OUTSTR,
						"ERROR: no acceptable types specified for %s",
						FUNCNAME);
				goto errorExit;
			} /*if (okTypes[0] + okTypes[1] + okTypes[2] == 0)*/
		} /*if (checkType)*/
	}

	if (nargs <= 0)
	{
		sprintf(OUTSTR,
				"ERROR: no arguments for %s() to check", FUNCNAME);
		goto errorExit;
	}
	result = RInstall(SCRATCH, nargs);
	if(result == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	setTYPE(result,LOGIC);

	nargs = NARGS(list);
	j = 0;
	for (i=0;i<nargs;i++)
	{
		symh = COMPVALUE(list,i);
		type = (symh != (Symbolhandle) 0) ? TYPE(symh) : 0;
		if (!checkType || type == 0 || strcmp(NAME(symh), USEDKEYWORD) != 0)
		{
			if (strcmp(what,"real") == 0)
			{
				value = (type == REAL);
			}
			else if (strcmp(what,"char") == 0)
			{
				value = (type == CHAR);
			}
			else if (strcmp(what,"logic") == 0)
			{
				value = (type == LOGIC);
			}
			else if (strcmp(what,"struc") == 0)
			{
				value = (type == STRUC);
			}
			else if (strcmp(what,"macro") == 0)
			{
				value = (type == MACRO);
			}
			else if (strcmp(what,"graph") == 0)
			{
				value = (type == PLOTINFO);
			}
			else if (strcmp(what,"scalar") == 0)
			{
				value = (isScalar(symh) != 0);
			}
			else if (strcmp(what,"vector") == 0)
			{
				value = (isVector(symh) != 0);
			}
			else if (strcmp(what,"matrix") == 0)
			{
				value = (isMatrix(symh,dims) != 0);
			}
			else if (strcmp(what,"array") == 0)
			{
				value = (type == CHAR || type == REAL || type == LOGIC ||
						 type == LONG);
			}
			else if (strcmp(what,"factor") == 0)
			{
				value = (isFactor(symh) > 0);
			}
			else if (strcmp(what,"null") == 0)
			{
				value = (isDefined(symh) != 0 && TYPE(symh) == NULLSYM);
			}
			else if (strcmp(what,"defined") == 0)
			{
				value = (isDefined(symh) != 0);
			}
			else
			{ /* should not happen*/
				sprintf(OUTSTR,"ERROR: ****programming error for operation %s",
						FUNCNAME);
				Delete(result);
				goto errorExit;
			}

			if (value && checkType)
			{
				if (!RealOK && type == REAL || 
					!LogicOK && type == LOGIC ||
					!CharOK && type == CHAR)
				{
					value = 0;
				}
			} /*if (value && checkType)*/
			DATAVALUE(result, j++) = (double) value;
		} /*if (!checkType||type == 0||strcmp(NAME(symh), USEDKEYWORD) != 0)*/
	} /*for (i=0;i<nargs;i++)*/

	return (result);

  errorExit:
	putErrorOUTSTR();
	return (0);
} /*iswhat()*/

/*
   Function primarily for usage in macros to retrieve keyword values

   keyvalue(name1:arg1,name2:arg2, ..., targetName,type)
     where type is "real", "logical", "character", "structure", "macro",
     or "graph" and targetName is a quoted string or CHAR scalar
   Returns NULL, if no keyword name matches name.
   Returns 0 if there is a matching keyword name but value is wrong type
   Otherwise returns the value of the matching keyword.

   keyvalue(,targetName,type) returns NULL provided type is legal

   keyvalue(structure(name1:arg,name2:arg2,...),targetName,type) is equivalent
     to keyvalue(name1:arg1,name2:arg2,...,targetName,type)
     
   Typical usage in macro expecting optional keyword phrase 'weights:w'
      @w <- keyvalue($K,"weights","real")
	  if(isnull(@w)) {@w <- rep(1,@n)}

   960916 Installed
   980602 Enhanced keyvalue().  You can have several qualifiers, for example:
          keyvalue($K,"keyname",vector("positive","integer","scalar"))
   980620 Added property "notnull".  Use this to match keyword with non-
          null value of any type.  This was added since it is impossible
          to distinguish keyvalue(a:NULL,"a") from keyvalue(b:1,"a").
   990113 It's now OK to repeat a property, e.g., vector("real","real")
          You can include multiple properties in a single element of
          the last argument, e.g., "positive integer scalar" 
   990120 You can use the following abbreviated properties for scalars
          "number"  same as "real scalar nonmissing"
          "TF"      same as "logical scalar nonmissing"
          "string"  same as "character scalar"
   990121 New type property "square" implies and is not incompatible with
          "matrix"
*/
enum
{
	TYPECODE      = 0,
	SHAPECODE,
	SIGNCODE,
	VALUECODE,
	MAXCODE,
	REALVAL       = 0,
	LOGICVAL,
	CHARVAL
};

#define PROPERTYLENGTH 20 /* must be longer than full name of any property*/

static char             *ScalarNames[] =
{
	"TF", "number", "string", (char *) 0
};

static long             ScalarCodes[] =
{
	LOGICVALUE | SCALARTYPE | NONMISSINGTYPE,
	REALVALUE  | SCALARTYPE | NONMISSINGTYPE,
	CHARVALUE  | SCALARTYPE
};

static char             *TypeNames[] =
{
	"real", "logical", "character", "macro", "graph", "notnull", (char *) 0
};
static long              TypeCodes[] =
{
	REALVALUE, LOGICVALUE, CHARVALUE, -MACRO, -PLOTINFO, -NULLSYM
};

static char             *ShapeNames[] =
{
	"scalar", "vector", "matrix", "array", "structure", "square", (char *) 0
};
static long              ShapeCodes[] =
{
	SCALARTYPE, VECTORTYPE, MATRIXTYPE, ARRAYTYPE, STRUCTURETYPE,
	MATRIXTYPE | SQUARETYPE
};

static char             *SignNames[] = 
{
	"positive", "nonnegative", (char *) 0
};
static long              SignCodes[] =
{
	POSITIVETYPE | REALVALUE | NONMISSINGTYPE,
	NONNEGATIVETYPE | REALVALUE | NONMISSINGTYPE
};

static char             *ValueNames[] = 
{
	"integer", "nonmissing", (char *) 0
};
static long              ValueCodes[] =
{
	INTEGERTYPE | REALVALUE| NONMISSINGTYPE,
	NONMISSINGTYPE
};

/*
  Match name to an element of one of TypeNames, ShapeNames, SignNames or
  ValueNames.  All of the characters name must match the first strlen(name)
  characters.
*/

static long keyDecode(char *name, long   codes[], char * words[])
{
	int          i, j, n = strlen(name), n1;
	int          minMatchLength;
	long         code;
	char        *word;

	if (mystrncmp(name, "non", 3) == 0 || mystrncmp(name, "str", 3) == 0)
	{
		minMatchLength = 4;
	}
	else
	{
		minMatchLength = 3;
	}

	for (i = 0; ScalarNames[i] != (char *) 0; i++)
	{
		word = ScalarNames[i];
		n1 = strlen(word);

		if (mystrncmp(name, word,
			(n1 < minMatchLength) ? n1 : minMatchLength) == 0)
		{
			long     typecode;

			code = ScalarCodes[i];
			
			if (codes[SHAPECODE] != 0 && codes[SHAPECODE] != SCALARTYPE)
			{
				return (-SHAPECODE - 1);
			}
			codes[SHAPECODE] = SCALARTYPE;
			
			typecode = code & (CHARVALUE | REALVALUE | LOGICVALUE);
			
			if (codes[TYPECODE] != 0 && codes[TYPECODE] != typecode)
			{
				return (-TYPECODE - 1);
			}
			codes[TYPECODE] = typecode;

			if (typecode != CHARVALUE)
			{
				codes[VALUECODE] |= NONMISSINGTYPE;
			}
			words[TYPECODE] = words[SHAPECODE] = words[VALUECODE] = word;
			
			return (code);
		} /*if (mystrncmp(name, ScalarNames[i], n) == 0)*/
	} /*for (i = 0; ScalarNames[i] != (char *) 0; i++)*/
	
	for (i = 0; TypeNames[i] != (char *) 0; i++)
	{
		word = TypeNames[i];
		n1 = strlen(word);

		if (mystrncmp(name, word,
			(n1 < minMatchLength) ? n1 : minMatchLength) == 0)
		{
			code = TypeCodes[i];
			j = TYPECODE;
			goto done;
		}
	} /*for (i = 0; TypeNames[i] != (char *) 0; i++)*/

	for (i = 0; ShapeNames[i] != (char *) 0; i++)
	{
		word = ShapeNames[i];
		n1 = strlen(word);

		if (mystrncmp(name, word,
			(n1 < minMatchLength) ? n1 : minMatchLength) == 0)
		{
			code = ShapeCodes[i];
			j = SHAPECODE;
			goto done;
		}
	} /*for (i = 0; ShapeNames[i] != (char *) 0; i++)*/

	for (i = 0; SignNames[i] != (char *) 0; i++)
	{
		word = SignNames[i];
		n1 = strlen(word);

		if (mystrncmp(name, word,
			(n1 < minMatchLength) ? n1 : minMatchLength) == 0)
		{
			code = SignCodes[i];
			j = SIGNCODE;
			goto done;
		}
	} /*for (i = 0; SignNames[i] != (char *) 0; i++)*/

	for (i = 0; ValueNames[i] != (char *) 0; i++)
	{
		word = ValueNames[i];
		n1 = strlen(word);

		if (mystrncmp(name, word,
			(n1 < minMatchLength) ? n1 : minMatchLength) == 0)
		{
			code = ValueCodes[i];
			j = VALUECODE;
			goto done;
		}
	} /*for (i = 0;  ValueNames[i] != (char *) 0; i++)*/

	return (0);

  done:
	if (codes[j] && (code & ~SQUARETYPE) != (codes[j]  & ~SQUARETYPE))
	{
		return (-j - 1);
	}
	codes[j] = code;
	words[j] = word;
	return (code);

} /*keyDecode()*/


/*
   980730 added new argument to reuseArg() so that notes will be kept.

   980801 added new function argvalue() with usage
          x <- argvalue(y, msg, properties)

*/
#define PATTERNLENGTH    20
#define KEYNAMELENGTH   100
void myAlert(char * /*msg*/);

Symbolhandle keyvalue(Symbolhandle list)
{
	Symbolhandle         result = (Symbolhandle) NULLSYMBOL;
	Symbolhandle         symhStr, component, symhName;
	Symbolhandle         symhProperties = (Symbolhandle) 0;
	Symbolhandle         symhArg = (Symbolhandle) 0;
	long                 nargs = NARGS(list), type;
	long                 ikey, nkeys = -1, i;
	long                 namePlace, propertyPlace;
	long                 code = 0;
	long                 propertyCodes[4];
	long                 argType, matchType;
	int                  strComp, keyCheck = strcmp(FUNCNAME, "keyvalue") == 0;
	int                  nProperties;
	char                 pattern[PATTERNLENGTH+1];
	char                *keyword = (char *) 0;
	char                 keyName[KEYNAMELENGTH + 1];
	char                *properties;
	char                *propertyNames[4];
	char                *cantUse =
		"ERROR: can't use property \"%s\" with property \"%s\"";
	WHERE("keyvalue");

	keyName[KEYNAMELENGTH] = '\0';

	if (nargs < 2)
	{
		badNargs(FUNCNAME, -1002);
		goto errorExit;
	}
	else if (!keyCheck && nargs > 3)
	{
		badNargs(FUNCNAME, -3);
	}

	propertyCodes[TYPECODE] = propertyCodes[SHAPECODE] =
	  propertyCodes[SIGNCODE] = propertyCodes[VALUECODE] = 0;

	propertyNames[TYPECODE] = propertyNames[SHAPECODE] =
	  propertyNames[SIGNCODE] = propertyNames[VALUECODE] = NullString;

	symhStr = COMPVALUE(list, 0);

	if (!keyCheck)
	{ /*argvalue()*/
		Symbolhandle   symhMsg = COMPVALUE(list, 1);
		char          *symptom = (char *) 0;

		if (!isCharOrString(symhMsg))
		{
			char        outstr[40];

			sprintf(outstr, "ERROR: argument 2 to %s()", FUNCNAME);
			notCharOrString(outstr);
			goto errorExit;
		}
		strncpy(keyName, STRINGPTR(symhMsg), KEYNAMELENGTH);

		symhArg = symhStr;
		/* Mimics code of argOK()*/
		if (symhArg == (Symbolhandle) 0)
		{
			symptom = "missing";
		}
		else if ((argType = TYPE(symhArg), argType == BLTIN))
		{
			symptom = "a built-in function";
		}
		else if (!myvalidhandle((char **) symhArg))
		{
			symptom = "damaged (deleted ?)";
		}
		else if (!isDefined(symhArg) || isNull(symhArg) && argType != NULLSYM)
		{
			symptom = (argType == UNDEF || argType == ASSIGNED) ?
			  "not defined" : "NULL";
		}

		if (symptom != (char *) 0)
		{
			sprintf(OUTSTR, "ERROR: %s is %s", keyName, symptom);
			goto errorExit;
		}
		namePlace = 1;
		strComp = 0;
	} /*if (!keyCheck)*/
	else
	{ /* keyvalue()*/
		strComp = !isKeyword(symhStr) && isStruc(symhStr);
		if (strComp)
		{
			nkeys = NCOMPS(symhStr);
			namePlace = 1;
		} /*if (!isKeyword(symhStr) && isStruc(symhStr))*/
		else /*if (strComp)*/
		{ /* first argument should be keyword phrase or empty argument */
			if (symhStr == (Symbolhandle) 0)
			{
				nkeys = 0;
				namePlace = 1;
			} /*if (symhStr == (Symbolhandle) 0)*/
			else
			{
				for (ikey = 0; ikey < nargs && isKeyword(COMPVALUE(list,ikey));
					 ikey++)
				{
					;
				}
				if (ikey == 0)
				{
					sprintf(OUTSTR,
							"ERROR: no keywords found by %s()", FUNCNAME);
					goto errorExit;
				}
				namePlace = nkeys = ikey;
			} /*if (symhStr == (Symbolhandle) 0){}else{}*/ 
			symhStr = list;
		} /*if (strComp){}else{}*/

		if (nargs <= namePlace)
		{
			sprintf(OUTSTR,
					"ERROR: no non-keyword or non-structure argument to %s()",
					FUNCNAME);
		}
		else if (nargs > namePlace + 2)
		{
			sprintf(OUTSTR, "ERROR: too many arguments to %s()", FUNCNAME);
		}

		if (*OUTSTR)
		{
			goto errorExit;
		}

		symhName = COMPVALUE(list, namePlace);
		if (!isCharOrString(symhName))
		{
			sprintf(OUTSTR,
					"ERROR: argument %ld to %s() is not quoted string or CHARACTER scalar",
					namePlace, FUNCNAME);
			goto errorExit;
		}
		strncpy(keyName, STRINGPTR(symhName), KEYNAMELENGTH);

		if (!scanPat(keyName,&matchType,pattern,topicNameCheck,PATTERNLENGTH) ||
			matchType == EXACTMATCH && strlen(keyName) > NAMELENGTH - 2)
		{
			sprintf(OUTSTR,
					"ERROR: %s is not a legal keyword name or pattern for %s()",
					keyName, FUNCNAME);
			goto errorExit;
		}
	} /*if (!keyCheck){}else{}*/

	propertyPlace = namePlace + 1;

	if (propertyPlace < nargs)
	{
		symhProperties = COMPVALUE(list, propertyPlace);

		if (!argOK(symhProperties, 0, propertyPlace + 1))
		{
			goto errorExit;
		}
		
		if (TYPE(symhProperties) != CHAR || !isVector(symhProperties))
		{
			sprintf(OUTSTR,
					"ERROR: argument %ld to %s() is not a CHARACTER scalar or vector",
					propertyPlace + 1, FUNCNAME);
			goto errorExit;
		}
		properties = STRINGPTR(symhProperties);
		nProperties = symbolSize(symhProperties);

		for (i = 0; i < nProperties; i++)
		{
			long        thisCode;
			long        length = strlen(properties);
			char        thisProperty[PROPERTYLENGTH];
			char       *place = properties;
			
			/*
			  Each element may encode several white space separated properties
			*/
			while (place - properties < length)
			{
				if (copyField(place, (char *) 0) > PROPERTYLENGTH)
				{ /* be careful */
					sprintf(OUTSTR,
							"ERROR: property name for %s() is too long",
							FUNCNAME);
					goto errorExit;
				}
				place += copyField(place, thisProperty);
				thisCode = keyDecode(thisProperty, propertyCodes,
									 propertyNames);
				if (thisCode > 0)
				{
					code |= thisCode;
				}
				else if (thisCode == 0)
				{
					sprintf(OUTSTR,
							"ERROR: property \"%s\" not recognized by %s()",
							thisProperty, FUNCNAME);
				}
				else if (thisCode < 0 && -thisCode <= MAXCODE)
				{
					sprintf(OUTSTR,
							cantUse, thisProperty,
							propertyNames[-thisCode - 1]);
				}
				else if ((code = thisCode, nProperties > 1))
				{
					sprintf(OUTSTR,
							"ERROR: you can't use other properties for %s() with \"%s\"",
							FUNCNAME,
							(-thisCode == MACRO) ? TypeNames[3] : TypeNames[4]);
				}
				if (*OUTSTR)
				{
					goto errorExit;
				}
			} /*while (place - properties < length)*/
			
			properties = skipStrings(properties, 1);
		} /*for (i = 0; i < nProperties; i++)*/

		if (code > 0)
		{
			if ((propertyCodes[TYPECODE] & (LOGICVALUE | CHARVALUE | STRUCTURETYPE)) && 
				(propertyCodes[SIGNCODE] & (POSITIVETYPE | NONNEGATIVETYPE)))
			{
				sprintf(OUTSTR, cantUse,
						propertyNames[TYPECODE], propertyNames[SIGNCODE]);
				goto errorExit;
			}
			if ((propertyCodes[TYPECODE] & (LOGICVALUE | CHARVALUE | STRUCTURETYPE)) && 
				(propertyCodes[VALUECODE] & INTEGERTYPE))
			{
				sprintf(OUTSTR, cantUse,
						propertyNames[TYPECODE], propertyNames[VALUECODE]);
				goto errorExit;
			}
			if (code & NONMISSINGTYPE)
			{
				if (propertyCodes[TYPECODE] & CHARVALUE)
				{
					sprintf(OUTSTR, cantUse,
							propertyNames[TYPECODE],
							(propertyCodes[VALUECODE]) ?
							propertyNames[VALUECODE] : propertyNames[SIGNCODE]);
					goto errorExit;
				}
				if (propertyCodes[SHAPECODE] & STRUCTURETYPE)
				{
					sprintf(OUTSTR, cantUse,
							propertyNames[SHAPECODE],
							(propertyCodes[VALUECODE]) ?
							propertyNames[VALUECODE] : propertyNames[SIGNCODE]);
					goto errorExit;
				}
			} /*if (code & NONMISSINGTYPE)*/
		} /*if (code > 0)*/
	} /*if (propertyPlace < nargs)*/

	if (keyCheck)
	{
		for (ikey = 0; ikey < nkeys; ikey++)
		{
			component = COMPVALUE(symhStr, ikey);
			keyword = (symhStr == list) ? isKeyword(component) : NAME(component);
			if (matchName(keyword, matchType, pattern))
			{
				strcpy(keyName, keyword);
				break;
			}
		} /*for (ikey = 0; ikey < nkeys; ikey++)*/
		if (ikey < nkeys)
		{
			symhArg = component;
			argType = TYPE(symhArg);
		}
	} /*if (keyCheck)*/
	else
	{
		ikey = 0;
	}

	if (symhArg != (Symbolhandle) 0)
	{
		if (symhProperties != (Symbolhandle) 0)
		{
			
			type = propertyCodes[TYPECODE];

			if (type < 0)
			{
				char     *preposition = (keyCheck) ? "for" : "of";
				type = -type;

				if (type == NULLSYM && isNull(symhArg))
				{/* type == NULLSYM means NULL value illegal */
					sprintf(OUTSTR, "ERROR: value %s %s is NULL",
							preposition, keyName);
				}
				else if (type != NULLSYM && argType != type)
				{
					sprintf(OUTSTR,
							"ERROR: value %s %s is not %s",
							preposition, keyName, typeName(type));
				}
				if (*OUTSTR)
				{
					goto errorExit;
				}
			} /*if (type < 0)*/
			else
			{
				char        msg[102];

				if (keyCheck)
				{
					sprintf(msg, "%s '%s'",
							(strComp) ? "structure component" : "keyword",
							keyName);
				}
				else
				{
					msg[0] = '#';
					strncpy(msg + 1, keyName, 100);
					msg[101] = '\0';
				}

				if ((code & NONMISSINGTYPE) && isStruc(symhArg))
				{
					sprintf(OUTSTR, "ERROR: %s is not %s",
							(keyCheck) ? msg : msg+1,
							(!(code & REALVALUE) && !(code & LOGICVALUE)) ?
							"REAL or LOGICAL" :
							((code & REALVALUE) ? "REAL" : "LOGICAL"));
					goto errorExit;
				}

				if (!checkArgType(symhArg, msg, code))
				{
					goto errorExit;
				}
			} /*if (type < 0){}else{}*/
		} /*if (symhProperties != (Symbolhandle) 0)*/

		if (strComp)
		{ /* must be structure arg */
			if (isscratch(NAME(symhStr)))
			{ /* reuse component */
				result = reuseArg(symhStr, ikey, 1, 1);
				COMPVALUE(list, 0) = (Symbolhandle) 0;
				Removesymbol(symhStr);
			}
			else
			{ /* make copy of component */
				result = Install(SCRATCH, type);
				if (result == (Symbolhandle) 0)
				{
					goto errorExit;
				}
				if (!Copy(symhArg, result))
				{
					Removesymbol(result);
					goto errorExit;
				}
			}
		} /*if (strComp)*/
		else
		{
			if (keyCheck || isscratch(NAME(symhArg)))
			{
				result = reuseArg(list, ikey, 1, 1);
			}
			else
			{
				result = Install(SCRATCH, argType);
				if (result == (Symbolhandle) 0)
				{
					goto errorExit;
				}
				if (!Copy(symhArg, result))
				{
					Removesymbol(result);
					goto errorExit;
				}
			}
		} /*if (symhStr != list){}else{}*/

		setNAME(result,SCRATCH);
	} /*if (symhArg != (Symbolhandle) 0)*/

	return (result);

  errorExit:
	putErrorOUTSTR();
	return (0);
} /*keyvalue()*/

/*
  vec <- nameof(arg1, arg2, ..., argk) returns vector of argument names
*/

Symbolhandle nameOf(Symbolhandle list)
{
	Symbolhandle      arg, result = (Symbolhandle) 0;
	long              nargs = NARGS(list), i;
	long              needed = 0, place = 0;
	char             *name;
	WHERE("nameOf");

	for (i = 0;i<nargs;i++)
	{
		if ((arg = COMPVALUE(list,i)) == (Symbolhandle) 0)
		{
			noData(FUNCNAME,i+1);
			goto errorExit;
		}
		name = NAME(arg);
		needed += strlen(name) + 1;
		if (isscratch(name))
		{
			needed -= 2;
		}
	}

	if ((result = Install(SCRATCH,CHAR)) == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	TMPHANDLE = mygethandle(needed);
	setSTRING(result,TMPHANDLE);
	if (TMPHANDLE == (char **) 0)
	{
		goto errorExit;
	}
	for (i = 0;i < nargs;i++)
	{
		name = NAME(COMPVALUE(list,i));
		if (isscratch(name))
		{
			name += 2;
		}
		strcpy(*TMPHANDLE + place,name);
		place += strlen(name) + 1;
	}
	setNDIMS(result,1);
	setDIM(result,1,nargs);

	return (result);

  errorExit:
	Removesymbol(result);
	return (0);

} /*nameOf()*/


/*
   usage: rename(var, Name) where Name is CHARACTER scalar or an
   undefined variable.   The name of var is changed to Name or to NAME(Name).
      or: rename(expr, Name)
   where expr is a scratch variable (function result, expression, constant)
   is equivalent either to Name <- expr or <<Name>> <- expr.

   Written by C. Bingham (kb@stat.umn.edu) 950524

   The C-function name chosen to avoid conflict with system function rename()

   990325 Fixed some minor bugs.
*/
Symbolhandle renamer(Symbolhandle list)
{
	Symbolhandle          symh, symhName;
	char                 *name, newName[NAMELENGTH + 1];
	long                  nameOK, isexpr;
	long                  nargs = NARGS(list), type;
	WHERE("rename");

	*OUTSTR = '\0';
	if (nargs != 2)
	{
		badNargs(FUNCNAME, 2);
		goto errorExit;
	}


	symh = COMPVALUE(list, 0);
	symhName = COMPVALUE(list, 1);

	if (!argOK(symh, 0, 1))
	{
		goto errorExit;
	}
	if (symhName == (Symbolhandle) 0)
	{
		sprintf(OUTSTR,
				"ERROR: argument 2 to %s() is missing", FUNCNAME);
		goto errorExit;
	}
	
	if (symh == symhName)
	{
		return (NULLSYMBOL);
	}
	
	if (TYPE(symh) == BLTIN)
	{
		sprintf(OUTSTR,
				"ERROR: %s cannot change name of built-in functions",
				FUNCNAME);
	}
	else if (isSpecial(symh))
	{
		sprintf(OUTSTR,
				"ERROR: %s cannot change name of %s", FUNCNAME, NAME(symh));
	}
	else if (isAssigned(symh))
	{
		sprintf(OUTSTR,
				"ERROR: %s cannot change name of variable being assigned to",
				FUNCNAME);
	}
	if (*OUTSTR)
	{
		goto errorExit;
	}
	isexpr = isscratch(NAME(symh));

	type = TYPE(symhName);
	if (type == UNDEF)
	{
		strcpy(newName, NAME(symhName));
		Removesymbol(symhName);
		COMPVALUE(list, 1) = (Symbolhandle) 0;
	} /*if (type == UNDEF)*/
	else
	{
		if (type != CHAR || !isScalar(symhName))
		{
			name = NAME(symhName);
			if (isscratch(name))
			{
				sprintf(OUTSTR,
						"ERROR: Name must be undefined variable or CHARACTER scalar");
			}
			else if (type == BLTIN)
			{
				sprintf(OUTSTR,
						"ERROR: %s is the name of a built-in function", name);
			}
			else
			{
				sprintf(OUTSTR,
						"ERROR: a %s with name %s already exists",
						(type == MACRO) ? "macro" : "variable", name);
			}
		} /*if (type != CHAR || !isScalar(symhName))*/
		else if (strlen(name = STRINGPTR(symhName)) > NAMELENGTH)
		{
			sprintf(OUTSTR,
					"ERROR: name to be assigned is too long; length > %ld",
					(long) NAMELENGTH);
		}
		if (*OUTSTR)
		{
			goto errorExit;
		}

		if (!isVariableName(name))
		{
			sprintf(OUTSTR,
					"ERROR: '%s' is not a legal MacAnova name",
					STRINGPTR(symhName));
			goto errorExit;
		} /*if (!nameOK)*/

		strcpy(newName, name);

		if (strcmp(newName, NAME(symh)) != 0 &&
			Lookup(newName) != (Symbolhandle) 0)
		{
			sprintf(OUTSTR,
					"ERROR: a variable with name %s already exists", newName);
			goto errorExit;
		}
	} /*if (type == UNDEF){}else{}*/

	if (isexpr)
	{
		COMPVALUE(list, 0) = (Symbolhandle) 0;
	}
	Cutsymbol(symh);
	setNAME(symh, newName);
	Addsymbol(symh);

	return (NULLSYMBOL);

  errorExit:
	putErrorOUTSTR();
	return (0);
} /*rename()*/
