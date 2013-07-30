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
#pragma segment Keywords
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
#include "keywords.h"

/*
   This file contains functions related to process keyword phrases
   Most (all?) came from file utils.c
   951222

   990226 Replaced putOUTSTR() by putErrorOUTSTR()
*/

/* functions for identifying and decoding keyword phrases*/

long findKeyword(Symbolhandle /*list*/, char * /*word*/, long /*start*/);
long getKeyValues(Symbolhandle /*list*/, long /*startKey*/, long /*op*/,
					keywordListPtr /*keyInfo*/);
long getOneKeyValue(Symbolhandle /*symhKey*/, long /*op*/,
					keywordListPtr /*keys*/, long nkeys);
long getAllKeyValues(Symbolhandle /*list*/, long /*startKey*/, long /*op*/,
					 keywordListPtr /*keyList*/, long /*nkeys*/);
void unsetKeyValues(keywordListPtr /*keyList*/, long /*nkeys*/);
long matchKey(char * /*name*/, char * /*charvals*/[], long /*codes*/[]);

/*
  search argument list for presence of specified keyword.  Return index in list
  handle if the first strlen(word) characters of the keyword match word,
  -1 otherwise.
*/

long findKeyword(Symbolhandle list, char * word, long start)
{
	long            nargs = NARGS(list);
	long            i,length = strlen(word);
	char           *keyword;

	for (i=start;i<nargs;i++)
	{
		if ((keyword = isKeyword(COMPVALUE(list,i))) &&
		   strncmp(keyword,word,(size_t) length) == 0)
		{
			return (i);
		}
	}
	return (-1);
} /*findKeyword()*/

/*
  970625 checkArgType() (argument checking code formerly in
         checkAndGetKey()) moved to utils.c as a public function
*/

/*
   checkAndGetKey() checks the characteristics of symhKey and the
   permissible operations masked into op against the keyword
   specification in keyList.  It is assumed that the names have already
   been checked to match.  If characteristics are wrong, 0 is returned;
   otherwise 1 is returned and the value part of keyList is set.

   For any "qualified" REAL (POSITIVEINT, NONNEGATIVEINT, LONG,
   POSITIVEREAL, NONNEGATIVEREAL, NONMISSINGREAL), a MISSING
   value fails the test.

   970625 actual checking and error messages are in checkArgType()
*/
 
static long checkAndGetKey(Symbolhandle symhKey, keywordListPtr keyList,
						  long op)
{
	char        *keyword = KeyName(keyList,0);
	long         type = KeyType(keyList,0), symhType = TYPE(symhKey);
	double       value;
	WHERE("checkAndGetKey");
	
	if(op != 0 && !(op & KeyOps(keyList,0)))
	{
		badKeyword(FUNCNAME, keyword);
		goto errorExit;
	}

	if (!checkArgType(symhKey, keyword, type))
	{
		goto errorExit;
	}
	
	/* Everything is OK, so return the value */
	if (type & SCALARTYPE)
	{
		if (type & CHARVALUE)
		{
			KeyCharValue(keyList, 0) = STRING(symhKey);
		} /*if (type & CHARVALUE)*/
		else
		{
			value = DATAVALUE(symhKey, 0);
			if (type & LOGICVALUE)
			{
				KeyLogValue(keyList, 0) = (value != 0.0);
			}
			else if (type & INTEGERTYPE)
			{
				KeyIntValue(keyList, 0) = (long) value;
			}
			else
			{
				KeyRealValue(keyList, 0) = value;
			}
		} /*if (type & CHARVALUE){}else{}*/
	} /*if (type & SCALARTYPE)*/
	else
	{
		KeySymhValue(keyList, 0) = symhKey;
	}
	return (1);

  errorExit:
	putErrorOUTSTR();	
	return (0);
	
} /*checkAndGetKey()*/

/*
   Function to scan keyword arguments in list, starting with
   COMPVALUE(list,startKey) for a match with the keyword specified
   in keyList.  If characteristics match, the value of keyList
   is set appropriately and 1 is returned.  Otherwise, an
   error message is printed and 0 is returned.  If op != 0,
   op & keyList->legalOps must != 0.  The actual checking is done by
   checkAndGetKey()

   If argument argno is found to match, its name is changed to
   USEDKEYWORD and it is moved to the end of the argument list, with
   arguments argno+1 ... nargs-1 moved to positions argno, ..., nargs - 2,
   Return value:
      -1      No keyword with matching name is found
       0      Match but op & legalOps == 0 or characteristics wrong
	   1      Everything fine.
*/

long getKeyValues(Symbolhandle list, long startKey, long op,
					keywordListPtr keyList)
{
	Symbolhandle  symhKey, symhTmp;
	long          nargs = NARGS(list), i, argno;
	char         *keyword;
	WHERE("getKeyValues");
	
	for (argno = startKey; argno < nargs; argno++)
	{
		symhKey = COMPVALUE(list, argno);
		if ((keyword = isKeyword(symhKey)) &&
			((KeyLength(keyList,0)) ?
				 strncmp(keyword, KeyName(keyList,0), KeyLength(keyList,0)) :
				 strcmp(keyword, KeyName(keyList,0))) == 0)
		{
			break;
		}
	} /*for (argno = startKey; argno < nargs; argno++)*/
			
	if (argno >= nargs)
	{ /* no match in arg list */
		return (-1);
	}
	
	if (!checkAndGetKey(symhKey, keyList, op))
	{
		goto errorExit;
	}
	
	/* rotate symhKey to the end of list */
	for (i=argno;i < nargs-1;i++)
	{
		symhTmp = COMPVALUE(list, i+1);
		COMPVALUE(list, i+1) = (Symbolhandle) 0;
		COMPVALUE(list, i) = symhTmp;
	} /*for (i=argno;i < nargs-1;i++)*/
	COMPVALUE(list, nargs-1) = symhKey;

	setNAME(symhKey,USEDKEYWORD);

	return (1);

  errorExit:
	return (0);
} /*getKeyValues()*/

/*
   Function to scan keyList of length nkeys for keyword matching symhKey
   which should already have been checked to be a keyword.  If symhKey is
   found to match keyList[ikey], then if op & KeyOps(keyList,ikey) == 0 or
   if the characteristics of symhKey do not those specified by keyList[ikey],
   an error message is printed and 0 is returned.  Otherwise, the value
   component of keyList[ikey] is set.  The actual checking is done by
   checkAndGetKey()

   Return value:
      -1      No keyword with matching name is found
       0      Match but op & legalOps == 0 or characteristics wrong
    ikey+1    Everything fine.
      -2      Name is USEDKEYWORD
*/
long getOneKeyValue(Symbolhandle   symhKey, long op, keywordListPtr keyList,
					long nkeys)
{	
	long           ikey;
	char          *keyword = isKeyword(symhKey);
	
	if (strcmp(NAME(symhKey), USEDKEYWORD) == 0)
	{
		return (-2);
	}
	
	for (ikey = 0; ikey < nkeys; ikey++)
	{
		if (((KeyLength(keyList,ikey)) ?
			 strncmp(keyword, KeyName(keyList,ikey), KeyLength(keyList,ikey)) :
			 strcmp(keyword, KeyName(keyList,ikey))) == 0)
		{
			break;
		}
	}
	if (ikey == nkeys)
	{
		return (-1);
	}
	return ((checkAndGetKey(symhKey, keyList + ikey, op)) ? ikey + 1 : 0);

} /*getOneKeyValue()*/

/*
   Usage:
	m = getAllKeyValues(list, startKey, op, keyList, nkeys);

	m is number of keys found and set; m < 0 means error found
*/

long getAllKeyValues(Symbolhandle list, long startKey, long op,
					 keywordListPtr keyList, long nkeys)
{
	int        i, keyStatus, found = 0;
	
	/* pick off keyword values */
	for (i = 0; i < nkeys; i++)
	{
		keyStatus = getKeyValues(list, startKey, op, keyList + i);
		if (keyStatus == 0)
		{
			goto errorExit;
		}
		if (keyStatus > 0)
		{
			found++;
		}
	} /*for (i = 0; i < nkeys; i++)*/
	return (found);
	
  errorExit:
	return (-1);
} /*getAllKeyValues()*/

void unsetKeyValues(keywordListPtr keyList, long nkeys)
{
	long       keyindex;
	long       type;
	
	for (keyindex = 0; keyindex < nkeys; keyindex++)
	{
		type = KeyType(keyList,keyindex);
		if (type & SCALARTYPE)
		{
			if (type & LOGICVALUE)
			{
				unsetLogical(KeyLogValue(keyList, keyindex));
			}
			else if (type & INTEGERTYPE)
			{
				unsetLong(KeyIntValue(keyList, keyindex));
			}
			else if (type & REALVALUE)
			{
				unsetReal(KeyRealValue(keyList, keyindex));
			}
			else
			{ /* should be CHARVALUE */
				unsetHandle(KeyHandleValue(keyList,keyindex));
			}
		} /*if (type & SCALARTYPE)*/
		else
		{
			unsetSymh(KeySymhValue(keyList, keyindex));
		}
	} /*for (keyindex = 0; keyindex < nkeys; keyindex++)*/
} /*unsetKeyValues()*/

	
/*
  attempt to match name in charvals.  If matched, return the corresponding
  value from codes; otherwiser return 0.
  If codes is NULL, return index+1
  Useful for translating character value to defined code for use in switch
*/

long matchKey(char * name, char * charvals[], long codes [])
{
	long           i;

	for (i=0;charvals[i];i++)
	{
		if (strcmp(charvals[i],name) == 0)
		{
			return ( (codes != (long *) 0) ? codes[i] : i+1);
		}
	} /*for (i=0;charvals[i];i++)*/
	return (0);
} /*matchKey()*/

