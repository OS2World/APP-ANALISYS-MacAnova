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
#pragma segment Lang
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/

#include "globals.h"
#include "mainpars.h"

#define RUNFUZZ 1e-15

enum forErrorCodes
{
	BADNCOMPS = 1,
	BADRANGEVAL,
	BADLIMITSVAL,
	MISSVALLIMITS,
	ZEROINC,
	WRONGSIGNINC
};

static char            IndexNames[MAXWDEPTH][NAMELENGTH+1];
/*
  Setup for if, else, elseif, while, and for

  Routine to test logic and correspondingly set globals to affect course
  of parsing.  It sets globals affecting subsequent parsing.  It returns 0
  when an error is found.
 
  The first argument list is a LIST and the second is IF, ELSEIF, ELSE, WHILE
  or FOR.

  On IF, ELSEIF and WHILE, NCOMPS(list) should be 1 and COMPVALUE(list,0)
  should be a LOGICAL scalar specifying the condition.

  On ELSE, list is a null handle.

  On FOR, 2 <= NCOMPS(list) <= 4.
   NAME(COMPVALUE(list, 0)) is the name of the index variable
   If NCOMPS(list) == 2, COMPVALUE(list,1) is a REAL vector specifying
   the values of the index.
   If NCOMPS(list) == 4, COMPVALUE(list,3) must be a non-zero non-MISSING
   REAL scalar inc (default value 1)
   If NCOMPS(list) == 3 or 4, COMPVALUE(list,1) and COMPVALUE(list,2) must
   be non MISSING REAL scalars i1 and i2 and a range vector of the form
   run(i1,i2,inc) is computed and saved.

  The depth of nested for and while loops is given by WDEPTH which is
  incremented here and decremented in the parser.

  The name of the index variable used in for(index,range){} is
  IndexNames[WDEPTH-1] and the vector of values is stored in reverse
  order in FORVECTORS[WDEPTH-1].

  The depth of nested if/elseif/else statements is given by IDEPTH which is
  incremented here and decremented in the parser.

  The value of Logical in if(Logical){} or ...elseif(Logical){} is
  put in IFCONDITIONS[IDEPTH-1]

  As of 911002 only IF was processed and the input stream was modified was
  to skip ahead to be just before '{'  and to wipe out input stream on
  errors.  It was changed so that the input stream is not changed and all
  control of parsing is through globals.

   931110  Ifsetup() now returns error code rather than set Global IfsetupError
   980208  Fixed divide by zero bug in for(i,1,1){}
           Modified some of the code checking arguments to make use of
           isScalar() which incorporates a check for a 0 Symbolhandle
           Removed some unneeded externs
   980218  Corrected handling of MISSING i1, i2 and inc in for(i,i1,i2[,inc])
   980220  for(i,NULL){...} just skips compound statment and is not an error.

*/
int Ifsetup(Symbolhandle list, long op)
{
	Symbolhandle    logic, indexVal, range, start, end, inc;
	char           *opname;
	long            condition;
	long            i,i1, length, halfLength;
	long            iftype = (op == IF || op == ELSEIF);
	long            nullList = (list == (Symbolhandle) 0);
	long            ncomps = (nullList) ? 0 : NCOMPS(list);
	long            badForList;
	double          startVal, endVal, incVal = 1.0, endVal1, span, val;
	double         *forVector, *rangeVec;
	WHERE("Ifsetup");

	OUTSTR[0] = '\0';
	
	if (op != ELSE)
	{ /* IF, ELSEIF, FOR and WHILE */
		if (iftype)
		{
			opname = (op == IF) ? "if" : "elseif";
		}
		else
		{
			opname = (op == WHILE) ? "while" : "for";
		}
		
		if ((!iftype && WDEPTH >= MAXWDEPTH) ||
			(op == IF && IDEPTH >= MAXIDEPTH))
		{
			sprintf(OUTSTR,"ERROR: too many nested %s's",opname);

			goto yyerrorExit;
		}

		if (!iftype)
		{ /* for & while */
			WDEPTH++;
		}
		else if (op == IF)
		{
			IDEPTH++;
		}
	
		if (op == WHILE || (iftype && !nullList))
		{
			char       *condValue = "conditional value";
			
			if (ncomps > 1)
			{
				sprintf(OUTSTR,"ERROR: usage is %s(LOGICAL){ ... }", opname);
			}
			else if ((logic = COMPVALUE(list,0)) == (Symbolhandle) 0)
			{
				sprintf(OUTSTR,"ERROR: no %s %s", opname, condValue);
			}
			else if (!isDefined(logic))
			{
				sprintf(OUTSTR,"ERROR: %s %s is UNDEFINED", opname, condValue);
			}
			else if (TYPE(logic) != LOGIC)
			{
				sprintf(OUTSTR,"ERROR: %s %s not LOGICAL", opname, condValue);
			}
			else if (!isScalar(logic))
			{
				sprintf(OUTSTR,
						"ERROR: %s %s has length > 1", opname, condValue);
			}
			else if (isMissing(DATAVALUE(logic,0)))
			{
				sprintf(OUTSTR, "ERROR: %s %s is MISSING", opname, condValue);
			}
			if (*OUTSTR)
			{
				goto yyerrorExit;
			}
			condition = (DATAVALUE(logic,0) != 0.0);
		} /* if (op == WHILE || (iftype && !nullList) */
		else if (op == FOR)
		{ /* for */
			opname = "for";
			if (FORVECTORS[WDEPTH-1] == (double **) 0)
			{
				/* first time through at this level */
				indexVal = COMPVALUE(list,0);
				COMPVALUE(list,0) = (Symbolhandle) 0;
				/* Check index symbol */
				if (indexVal == (Symbolhandle) 0 ||
				   TYPE(indexVal) == BLTIN || isscratch(NAME(indexVal)))
				{
					sprintf(OUTSTR,
							"ERROR: %s index to %s loop",
							(indexVal==(Symbolhandle) 0) ? "missing" : "illegal",
							opname);
					goto yyerrorExit;
				}

				badForList = (ncomps == 1 || ncomps > 4) ? BADNCOMPS : 0;

				if (!badForList)
				{
					if (ncomps == 2)
					{
						range = COMPVALUE(list, 1);
						if (TYPE(range) != NULLSYM &&
							(!isVector(range) || TYPE(range) != REAL))
						{
							badForList = BADRANGEVAL;
						}
					} /*if (ncomps == 2)*/
					else
					{
						start = COMPVALUE(list, 1);
						end = COMPVALUE(list, 2);
						inc = (ncomps == 4) ?
						  COMPVALUE(list, 3) : (Symbolhandle) 0;
						if (!isScalar(start) || TYPE(start) != REAL ||
							!isScalar(end) || TYPE(end) != REAL ||
							(ncomps == 4 &&
							 (!isScalar(inc) || TYPE(inc) != REAL)))
						{
							badForList = BADLIMITSVAL;
						}
						else
						{
							startVal = DATAVALUE(start, 0);
							endVal = DATAVALUE(end, 0);
							incVal = (ncomps == 4) ? DATAVALUE(inc, 0) : 1.0;
						
							if (isMissing(startVal) || isMissing(endVal) ||
								isMissing(incVal))
							{
								badForList = MISSVALLIMITS;
							}
							else
							{
								double     howFar = endVal - startVal;
								
								if (ncomps == 3)
								{
									incVal = (howFar >= 0) ? 1.0 : -1.0;
								}
								else if (howFar != 0.0 && incVal == 0.0)
								{
									badForList = ZEROINC;
								}
								else if (incVal * howFar < 0.0)
								{
									badForList = WRONGSIGNINC;
								}
							}
						}
					} /*if (ncomps == 2){}else{}*/
				} /*if (!badForList)*/
				
				if (badForList)
				{
					char        *usage1 = "for(index,i1,i2[,inc]){...}";
					char        *usage2 = "for(index,i1,i2,inc){...}";
				
					switch (badForList)
					{
					  case BADNCOMPS:
						sprintf(OUTSTR,
							   "ERROR: usage is for(index,realVec){...} or %s", usage1);
						break;
						
					  case BADRANGEVAL:
						sprintf(OUTSTR,
								"ERROR: range must be REAL vector in for(index, range){...}");
						break;
						
					  case BADLIMITSVAL:
						sprintf(OUTSTR,
								"ERROR: i1, i2, inc must be non-MISSING REAL scalars in %s",
								usage1);
						break;

					  case MISSVALLIMITS:
						sprintf(OUTSTR,
								"ERROR: i1, i2, inc must not be MISSING in %s", 
								usage1);
						break;
						
					  case ZEROINC:
						sprintf(OUTSTR,
								"ERROR: inc must not be 0 in %s", usage2);
						break;
						
					  case WRONGSIGNINC:
						sprintf(OUTSTR,
								"ERROR: inc has wrong sign in %s", usage2);
						break;
					} /*switch (badForList)*/
					goto yyerrorExit;
				} /*if (badForList)*/
				
				incVal = (incVal) ? incVal : 1.0;

				if (ncomps == 2)
				{
					length = symbolSize(range);
				} /*if (ncomps == 2)*/
				else
				{
					span = endVal - startVal;
					length = (long) (span / incVal) + 1;

/* 
   The following is intended to insure that if endVal is essentially an exact
   multiple of by away from startVal, then the last element of the result
   will be close to endVal, even if slightly outside the interval from
   startVal to endVal; in addition, a very near zero is made exactly zero
   The behavior is supposed to be exactly that of run().
*/
					if (length > 1)
					{		
						for (endVal1 = startVal + length * incVal;
							 incVal > 0.0 && endVal1 <= endVal ||
							 incVal < 0.0 && endVal1 >= endVal ||
							 fabs((endVal1 - endVal)/span) <= RUNFUZZ;
							 endVal1 = startVal + length * incVal)
						{
							length++;
						}
					} /*if (length > 1)*/
				} /*if (ncomps == 2){}else{}*/
				
				WHILELIMITS[WDEPTH-1] = length;

				/* get temporary storage for range */
				if (length == 0)
				{
					/* must be for(i,NULL){...} */
					FORVECTORS[WDEPTH-1] = (double **) 0;
					WHILECONDITIONS[WDEPTH-1] = 0;
					/* return signal to skip compound statement*/
					return (-1);
				} /*if (length == 0)*/

				if (ncomps > 2 || !isscratch(NAME(range)))
				{
					FORVECTORS[WDEPTH-1] = 
					  (double **) mygethandle(length*sizeof(double));
					if (FORVECTORS[WDEPTH-1] == (double **) 0)
					{
						goto errorExit;
					}
					if (ncomps == 2)
					{
						rangeVec  = DATAPTR(range);
					}
				} /*if (ncomps > 2 || !isscratch(NAME(range)))*/
				else
				{ /* for(i,scratchVec){}*/
					rangeVec  = DATAPTR(range);
					FORVECTORS[WDEPTH-1] = DATA(range);
					setDATA(range, (double **) 0);
				} /*if (ncomps > 2 || !isscratch(NAME(range))){}else{}*/

				forVector = *FORVECTORS[WDEPTH-1];

				/*
				  Copy range or run(startVal,endVal, incVal) to FORVECTORS
				  in reverse order, doing it in such a way that forVector
				  can be the same as rangeVec
				  */
				if (ncomps == 2)
				{ /* copy from rangeVec to forVector in reverse order */
					halfLength = length/2;
					for (i = 0,i1 = length - 1;i < halfLength;i++, i1--)
					{
						val = rangeVec[i];
						forVector[i] = rangeVec[i1];
						forVector[i1] = val;
					}
					if (2*halfLength != length)
					{
						forVector[i] = rangeVec[i1];
					}
				} /*if (ncomps == 2)*/
				else if (length == 1)
				{
					forVector[0] = startVal;
				}
				else
				{ /* generate run(startVal,endVal,incVal)*/
					for (i = 0,i1 = length - 1;i1 >= 0;i1--, i++)
					{
						val = startVal + (double) i * incVal;
						forVector[i1] = 
						  (incVal && fabs(val/span) < RUNFUZZ) ? 0.0 : val;
					}
					if (fabs((val-endVal)/span) < RUNFUZZ)
					{
						forVector[0] = endVal;
					}
				} /*if (ncomps == 2){}else{}*/

				/* save index name */
				strncpy(IndexNames[WDEPTH-1],NAME(indexVal),NAMELENGTH);
				IndexNames[WDEPTH-1][NAMELENGTH] = '\0';
			} /* if (FORVECTORS[WDEPTH-1] == (double **) 0) */
			else
			{ /* not first time through */
				indexVal = Lookup(IndexNames[WDEPTH-1]);
			}
			/* re-use index variable if it is a REAL scalar */
			if (!isScalar(indexVal) || TYPE(indexVal) != REAL)
			{
				Removesymbol(indexVal);
				indexVal = (Symbolhandle) 0;
			}
			if (indexVal == (Symbolhandle) 0)
			{
				indexVal = RInstall(IndexNames[WDEPTH-1],1);
				if (indexVal == (Symbolhandle) 0)
				{
					goto errorExit;
				}
			}
			val = (*FORVECTORS[WDEPTH-1])[WHILELIMITS[WDEPTH-1]-1];
#if (USENOMISSING)
			if (isMissing(val))
			{
				clearNOMISSING(indexVal);
			}
			else
			{
				setNOMISSING(indexVal);
			}
#endif /*USENOMISSING*/
			DATAVALUE(indexVal,0) = val;
		}
 
		if (op == WHILE)
		{
			/* WHILESTARTS, WHILEBRACKETLEV, and IFBRACKETLEV set in yylex */
			if (--WHILELIMITS[WDEPTH-1] <= 0 && condition)
			{					/* safety precaution */
				/* Note: Parser also recognizes this condition and terminates
				   loop
				*/
				sprintf(OUTSTR,"ERROR: more than %ld repetitions of %s loop",
						(long) MAXWHILE-1,opname);
				putErrorOUTSTR();
			}	
			WHILECONDITIONS[WDEPTH-1] = condition;
		} /*if (op == WHILE)*/
		else if (op == FOR)
		{/* Note: WDEPTH & WHILELIMITS[WDEPTH-1] are decremented in mainpars */
			WHILECONDITIONS[WDEPTH-1] = 1;
			if (WHILELIMITS[WDEPTH-1] == 1)
			{ /* this is the last time */
				IndexNames[WDEPTH-1][0] = '\0';
				mydisphandle((char **) FORVECTORS[WDEPTH-1]);
				FORVECTORS[WDEPTH-1] = (double **) 0;
			}
		} /*if (op == WHILE){}else if (op == FOR)*/
		else if (!nullList) /* op == IF or op == ELSEIF */
		{
			IFCONDITIONS[IDEPTH-1] = condition;
		}
	} /* if (op != ELSE) */
	else
	{
		opname = "else";
	}
	
	/* go look for starting bracket enclosing the compound statement */
	if (!findLParen(op, '{'))
	{
		goto errorExit;
	}

	if (op == FOR)
	{
		WHILESTARTS[WDEPTH-1] = ISTRCHAR;
	}
	*OUTSTR = '\0';
	
	if (interrupted(DEFAULTTICKS) != INTNOTSET)
	{
		goto errorExit;
	}

	return (1);

  yyerrorExit:
	yyerror(OUTSTR);
	return (0);
	
  errorExit:
	putErrorOUTSTR();

	/* report error to parser */

	return (0);
	
} /*Ifsetup()*/
