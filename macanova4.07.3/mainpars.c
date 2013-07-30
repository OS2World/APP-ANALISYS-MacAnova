
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
#pragma segment Mainpars
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


/*
  Main parser for macanova by G. Oehlert, as modifed by C. Bingham
*/

/*
	Version of 920623 made many changes, here and in yylex.c and Symbol.c
	All names are returned by yylex as VAR unless they are BLTINs or MACROs
	followed by '('.  Thus the parser should never see STRUC, UNDEF, and should
	never see MACRO or BLTIN or BATCH unless followed by '('
	The number of shift/reduce conflicts has been reduced to 13

	930201 Added 'break n' functionality which also is associated with token
	BREAK, and modified yylex so that 'breakall' is equivalent to
	'break WDEPTH'.  When n is a literal positive integer, 'break n' results
	in macanova	breaking out of exactly n enclosing loops.

	930208 Added a[..][..] capability, now 17 shift/reduce errors

	930804 Added operators <-+ <-- <-*, <-/, <-^, <-%%; 23 shift/reduce errors

	931007 Added lines to make use of batch() as an expression illegal.

	931103 Added error checking for left side of <-+, <--, etc; 47 S/R errors

	931110 Communication with Ifsetup() now by return value rather than global

	931228 Found that mainpars.c as directly produced was too large for the
           Mac MPW compiler.  Accordingly, rearranged various cases and used
		   more gotos, to maximize the length of sequences of case statements
           with identical code that could be folded into one another by
           editing of mainpars.c for use on the Macintosh.  The first case
           of a set of, say 3, cases to be folded has a comment of the
		   form 'Mac: fold 3 cases'. Script mainpars.awk for awk uses this
		   to create mainpars.c.mac.new.  This also has the parser tables
		   edited out and put into file mainpars.r.mac.new in a form readable
		   by MPW resource compiler Rez.

		   Note: Some of the folded cases are not identical in mainpars.y, but
		   are translated into identical code by Yacc.  Conversely, some
		   cases that appear to be identical in mainpars.y are translated
		   by Yacc in different ways.
	
	940110 Fixed bug in byname BATCH parenlist

	940119 Found and fixed bug.  yyparse() was telling yylex() to send an
	unnecessary (and harmful) ';' after if(l){..}else{...} and after
	while(l){...} .

	940331 Added bitwise operations BITAND, BITXOR, BITOR, and BITNOT

	940411 Added line to add component ASSIGN  to definition of 'assign',
	in order to generate useful error message.
	Now 48 shift/reduce conflicts

	940630 Implemented if{...}elseif{...}...elseif{...}else{...}
	Now 49 shift/reduce

	941108 Changed precedence of '!' to be lower than EQ, NE, etc

	950207 Added type NULLSYM

	950215 Changed precedence of MATMULT, TRMATMULT, and MATMULTTR to bind
    more tightly than MULT, DIV, and MOD

	950217 New syntax element cmpstatement
	Now 48 shift/reduce

	950411 Fixed bug in elseif ({...} not skipped on F

	950413 Introduced macros SetBreakTarget, SetNextChar, SkipBracketBlock
	and SkipParenBlock for setting YYLEXMESSAGE

	950710 Added parserCopy() to fix bug in interpreting things like
    {b <- 1} + {b <- 2}

	950711 Modified binary operations to apply parserCopy to left-hand operand
	to ensure that, for example, 'b<- 2;b + (b<-3)' has value 5, not 6
	Now 67 shift reduce, all the new ones arising from things like
	expr '+' batch; see definition of badbatch

	950722 Restricted use of parserCopy to eliminate buildup of scratch
	variables.  {b<-1}+{b<-2} again will give the wrong answer.  However,
	if the {...} is really an expanded macro it will give the right answer
	This can still result in the buildup of scratch if the value of a
	macro is not assigned.  Hence, guidelines should suggest that
	macros not returning a value should end with ';;'.  Specifically
	use of parserCopy() was removed at cmpdstatement and added at
	expandedmacro.  Also, parserCopy() no longer copies null variables.

	950724 Added macrobody ('{' '#' cmpdline '}') after modifying 
	Macrosetup() to insert "{#)#macroname" before body of macro and
	changed yylex() to return '#' after an '{' starting string "{#)#"

	950725 Fixed parser error introduced by change of 950711 by eliminating
	many cases of badbatch.  Instead, each left side of a binary operation
	increases global BATCHBLOCKCOUNT by 1 and each right side decreases it.
	If yylex() would otherwise return BATCH when BATCHBLOCKCOUNT > 0, it
	uses yyerror() to print an error message and returns BADTOKEN, instead.
	The number of shift/reduce conflicts went back to 48.

	950804 Added token SEMI1 to represent ';' or newline not immediately
	followed by '}' when BRACKETLEV > 0.  If SEMI1 follows an expression
	or an assignment whose value is a scratch variable, the scratch variable
	is removed so as to control the build up of scratch variables.
	Now 49 S/R conflicts.

	950807 Added check for BLTIN and LIST to parserCopy()
	950808 Added defines for use in Assign()
	950818 Removed END as expr, added END ENDOFL as line (49 S/R conflicts)
	       Changed setupfakefor() to setupfaketokens() and use it for
           repeating while loop, too.
		   Changes made in yylex1() so that syntax elements can be used
		   as keywords and structure component names.

	951130 Added macrobody bracklist to possibilities for subscripted.
	       Now 50 shift/reduce conflicts

	960226 Added operators %/% (MATDIV) and %\% (DIVMAT) at the same level
           of precedence as %*%, %c%, and %C%

	960620 Added check to keep from printing "invisible" symbols (names
           with '_' or '@_'

	961003 Added transpexpr and removed expr   '\'' from from expr1 list
           and then allowed transpexpr bracklist as choice for subscripted
           Now 51 shift/reduce

	961102 Changed POW from left associativity to right.

	961102 Added transfers to setScrName2 at macrobody and expandedmacro
           so that scratch name would be set appropriately.

	961204 Added macro clearNproducts() where appropriate

	970305 Resetting of NLINES taken out of cleanitup

	970324 Modified parser so that '()' is equivalent to NULL except
           after function or macro names. '()' is recognized as a
           nullexpr which is shifted to an expression.

    970611 MDEPTH incremented by yylex() on seeing macro header; parser
           decrements MDEPTH after recognizing macrobody
           doParserCopy() now copies non-scratch invisible variables to
           scratch invisible variables.

    970619 parser is expected to be re-entrant (mainpars.c is post-processed
           by mainpars.sed)
           New tokens INLINEHEAD, OUTLINEHEAD, EVALHEAD signal expanded
           in-line macro, expanded out-of-line macro and evalation string.
		   Uses new global PARSERVALUE to communicate value of evaluated string

    970625 eliminated 1 case each from bracklist ('[' completelist) and
           from brack2list (LEFTBRACK2 expr).
           Now 40 shift/reduce

    970714 added check of new global EVALDEPTH in runCommand() avoid
           unloading a segment that is might be in use
           All types (including MACRO and BLTIN) now defined in globdefs.h
           with new tokens MACROTOKEN and BLTINTOKEN introduced here
           and in yylex().

    971210 changed code to process transpexpr to use runCommand
    971215 Fixed memory leak; now a scratch brack2list is deleted in
           pattern  expr brack2list
           Also added tests so that Unscratch() is not called except
           at INPUTLEVEL 0
           Also made str[[]] syntactically legal with (Symbolhandle) 0
           value so that Extract() can issue an appropriate error message

    980131 Now aborts when batch() finds an error in setting things up.
           Avoids occasional confusing error messages.

    980220 Made change to go with change in Ifsetup() so that for(i,NULL){...}
           just skips compound statement.  Ifsetup() signals NULL by
           returning -1.

    980220 Added new token NEXTREP to implmenent new syntax element 'next';
           'next' or 'next m' behave the same as 'break' or 'break n'
           except that the target loop is not terminated.
           Still 40 shift/reduce

    990204 Added PopMacroName() and modified cleanitup to clean up stack
           of macro names

VERSION: 990204   This date is used by mainpars.awk in making macMainpars.r

*/

#include "globals.h"

#ifdef MACINTOSH

#ifndef MACIFACEH__
#include "macIface.h"
#endif /*MACIFACEH__*/

#define CHECKINTERRUPT	if(INTERRUPT != INTNOTSET) YYERROR

#else /*MACINTOSH*/

#define CHECKINTERRUPT

#endif /*MACINTOSH*/

/* these must be defined identically in yylex.c */
#define SKIPBRACKETBLOCK  3
#define SKIPPARENBLOCK    4

/* these must be defined identically in Symbol.c and Subassig.c*/
#define LEFTHANDSIDE         1
#define RIGHTHANDSIDE        2
#define KEYWORDASSIGN        3

#define BlockBatch()            (BATCHBLOCKCOUNT++)
#define UnBlockBatch()          (BATCHBLOCKCOUNT--)
#define SkipBracketBlock()      (YYLEXMESSAGE = SKIPBRACKETBLOCK)
#define SkipParenBlock()        (YYLEXMESSAGE = SKIPPARENBLOCK)
#define SetNextChar(C)          (YYLEXMESSAGE = (- (long) (C)))
#define SetBreakTarget(Level)   (YYLEXMESSAGE = \
							     MAXWDEPTH + WHILEBRACKETLEV[Level] + 1)
#define SetNextTarget(Level)    SetBreakTarget(Level)
/* the following macros are referenced only by cleanitup() */
#define ClearYylexGlobals()     (BATCHBLOCKCOUNT = 0,\
								 PARSEMACROARGS = 0,\
								 ALTTOKENPTR = (int *) 0,\
                                 ALTINPUTTOKENS[0] = 0,\
								 YYLEXMESSAGE = 0,\
                                 LASTCH = '\0')
#define ClearSyntaxGlobals()    (BRACKETLEV = 0,\
								 PARENLEV = 0,\
								 MDEPTH = 0,\
								 WDEPTH = 0,\
								 IDEPTH = 0,\
								 MLEVEL = MAXMDEPTH+1)

#define YYSTYPE Symbolhandle 	/* handle to symbol */
#define YYDEBUG 1

#ifdef MACINTOSH
/*
  set to point to various yacc tables that are normally included in
  mainpars.c
*/
extern short *Pyyexca;
extern short *Pyyact;
extern short *Pyypact;
extern short *Pyypgo;
extern short *Pyyr1;
extern short *Pyyr2;
extern short *Pyychk;
extern short *Pyydef;
#endif /*MACINTOSH*/

static long         BynameType;
static long         ParseMacroArgs;
static int          Op, AssignOp;
static int          TargetLevel;

#ifdef UNDEFINED__
static WHERE("yyparse"); /*now set by make*/
#endif /*UNDEFINED__*/

long    PARSEMACROARGS = 0;  /* for communication with yylex */
long    YYLEXMESSAGE = 0;  /* for communication with yylex */
long    LASTCH = '\0';  /* for communication with yylex */
long    BATCHBLOCKCOUNT = 0; /* for communication with yylex */
int     ALTINPUTTOKENS[5*MAXWDEPTH] = {0};  /* for communication with yylex */
int    *ALTTOKENPTR = (int *) 0;  /* for communication with yylex */

/*
   End of MacAnova Code before parser instructions
*/

#ifdef __cplusplus
#  include <stdio.h>
#  include <yacc.h>
#endif	/* __cplusplus */ 
# define MACROTOKEN 257
# define BLTINTOKEN 258
# define NUMBER 259
# define VAR 260
# define END 261
# define IF 262
# define ELSE 263
# define ELSEIF 264
# define WHILE 265
# define FOR 266
# define BREAK 267
# define BREAKALL 268
# define NEXTREP 269
# define BATCH 270
# define ERROR 271
# define FATALERROR 272
# define BADTOKEN 273
# define LEFTANGLE 274
# define RIGHTANGLE 275
# define LEFTBRACK2 276
# define RIGHTBRACK2 277
# define MULT 278
# define ADD 279
# define DIV 280
# define MOD 281
# define SUB 282
# define EXP 283
# define GE 284
# define LE 285
# define GT 286
# define LT 287
# define EQ 288
# define NE 289
# define AND 290
# define OR 291
# define NOT 292
# define ENDOFL 293
# define POW 294
# define MATMULT 295
# define TRMATMULT 296
# define MATMULTTR 297
# define MATDIV 298
# define DIVMAT 299
# define BITAND 300
# define BITXOR 301
# define BITOR 302
# define BITNOT 303
# define SEMI1 304
# define INLINEHEAD 305
# define OUTLINEHEAD 306
# define EVALHEAD 307
# define ASSIGN 308
# define ASSIGN1 309
# define ASSIGNMULT 310
# define ASSIGNADD 311
# define ASSIGNDIV 312
# define ASSIGNSUB 313
# define ASSIGNPOW 314
# define ASSIGNMOD 315
# define UNARYMINUS 316
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif

/* __YYSCLASS defines the scoping/storage class for global objects
 * that are NOT renamed by the -p option.  By default these names
 * are going to be 'static' so that multi-definition errors
 * will not occur with multiple parsers.
 * If you want (unsupported) access to internal names you need
 * to define this to be null so it implies 'extern' scope.
 * This should not be used in conjunction with -p.
 */
#ifndef __YYSCLASS
# define __YYSCLASS static
#endif
#ifndef YYSTYPE
#define YYSTYPE int
#endif
YYSTYPE yylval;
__YYSCLASS YYSTYPE yyval;
typedef short yytabelem;
# define YYERRCODE 256


static int          SeenError = 0;
/*
  970619 modified output when INPUTLEVEL > 0
  990205 modified output when ACTIVEMACROS > 0
*/
void yyerror(char * s)
{
	/*  output error message */
	unsigned char   c = '\0', *inputString = *INPUTSTRING;
	int             start = 0;
	int             iswarning = (strncmp("WARNING:", s, 8) == 0);
	WHERE("yyerror");

	if (!SeenError && (PRINTWARNINGS || !iswarning))
	{
		int          length = strlen(s);
		char         msg[70];

		msg[0] = '\0';
		
		myprint(s);

		if (INPUTLEVEL > 0 || ACTIVEMACROS > 0)
		{
			int         inMacro = (ACTIVEMACROS > 0);
			int         inEvaluate =
			  (INPUTLEVEL > 0 && istempname(ThisInputname));
		
			if (inMacro)
			{
				strcpy(msg, inWhichMacro());
			}
			else if (inEvaluate)
			{
				strcpy(msg, " in evaluation string");
			}
		} /*if (INPUTLEVEL > 0 || ACTIVEMACROS > 0)*/
		
		strcat(msg, " near ");
		length += strlen(msg);
		myprint(msg);
		
		if(length + ISTRCHAR > SCREENWIDTH)
		{
			myeol();
		}
		
		if(ISTRCHAR > SCREENWIDTH && !(GUBED & 2))
		{
			start = ISTRCHAR - (SCREENWIDTH - 5);
			start = (start >= 0) ? start : 0;
			myprint(". . .");
		}

		c = inputString[ISTRCHAR];
		inputString[ISTRCHAR] = '\0';
		mymultiprint((char *) inputString + start, STDOUT, 1);
		inputString[ISTRCHAR] = c;
		SeenError = !iswarning;
	} /*if (!SeenError && (PRINTWARNINGS || !iswarning))*/
	
	if(s == OUTSTR)
	{
		OUTSTR[0] = '\0';
	}
} /*yyerror()*/

/*
   tell yylex to report either ';for()' or ';while(' to parser
*/
static int fakefortokens[] = {(int) ';', FOR, (int) '(', (int) ')', 0 };
static int fakewhiletokens[] = {(int) ';', WHILE, (int) '(', 0 };

static void setupfaketokens(int which)
{
	int    i;
	int   *tokens = (which == FOR) ? fakefortokens : fakewhiletokens;
	WHERE("setupfaketokens");
	
	for(i=0;tokens[i];i++)
	{
		ALTINPUTTOKENS[i] = tokens[i];
	 }
	ALTINPUTTOKENS[i] = 0;
	ALTTOKENPTR = ALTINPUTTOKENS;
	ISTRCHAR = WHILESTARTS[WDEPTH-1];
	LASTCH = '\0';
} /*setupfaketokens()*/

static void notfunction(Symbolhandle symh)
{
	if(isscratch(NAME(symh)))
	{
		yyerror("ERROR: problem with input");
	}
	else
	{
		sprintf(OUTSTR,"ERROR: %s is not function or macro",NAME(symh));
		yyerror(OUTSTR);
	}
}

/*
   Clean up looping related globals
*/
static void cleanLoops(long level1, long level2)
{
	int      i;
	
	for(i=level1;i<level2;i++)
	{
		WHILECONDITIONS[i] = 0;
		WHILELIMITS[i] = MAXWHILE;
		mydisphandle((char **) FORVECTORS[i]);
		FORVECTORS[i] = (double **) 0;
	}
} /*cleanLoops()*/

static Symbolhandle runCommand(Symbolhandle func, Symbolhandle list)
{
	Symbolhandle      result;
	Symbolhandle      (*f) (Symbolhandle) = FPTR(func);

	strcpy(FUNCNAME,NAME(func));
	OUTSTR[0] = '\0';
	clearNproducts();
	result = (*f)(list);

	clearNproducts();
#ifdef SEGMENTED
	if (f != elapsedTime && EVALDEPTH == 0)
	{
		UNLOADSEG(*f);
	}
#endif /*SEGMENTED*/
	Removelist(list);
	return (result);
} /*runCommand()*/

/*
   If symh is a scratch symbol, return it; otherwise return a scratch
   copy of symh
   950907 removed all external calls from initial testing
*/

#if (0)  /* on HPPA use of macro lengthened code by 4K, slowed down*/
#define parserCopy(symh) ((symh == (Symbolhandle) 0 ||\
						  isscratch(NAME(symh)) || \
						  (Type__ = TYPE(symh), Type__ == UNDEF) ||\
						  Type__ == ASSIGNED ||	Type__ == BLTIN ||\
						  DIMVAL(symh, 1) == 0 || Type__ == LIST) ?\
							symh : doParserCopy(symh))
#endif /*0*/

#ifdef parserCopy
static long Type__;
#else /*parserCopy*/
#define doParserCopy  parserCopy
#endif /*parserCopy*/
/*
  970612 now copies non-scratch invisible symbols to invisible scratch
         symbols
*/
static Symbolhandle doParserCopy(Symbolhandle symh)
{
	Symbolhandle     symhCopy;
	long             type;
	
#ifndef parserCopy
	if (symh == (Symbolhandle) 0 || isscratch(NAME(symh)) ||
		(type = TYPE(symh)) == UNDEF || type == ASSIGNED ||
		type == BLTIN || DIMVAL(symh, 1) == 0 ||
		type == LIST )
	{
		return (symh);
	}
#else /*parserCopy*/
	type = TYPE(symh);
#endif /*parserCopy*/
	symhCopy = Makesymbol(type);
	if (symhCopy != (Symbolhandle) 0 && Copy(symh, symhCopy))
	{
		setNAME(symhCopy, (invisname(NAME(symh))) ? INVISSCRATCH : SCRATCH);
		Addsymbol(symhCopy);
	} /*if (symhCopy != (Symbolhandle) 0 && Copy(symh, symhCopy))*/
	else
	{
		/* couldn't  make copy; return original */
		Delete(symhCopy);
		symhCopy = symh;
	}
	return (symhCopy);
} /*doParserCopy()*/

/*
  Called from main before every invocation of getinput.
  It needs to be here because it resets PARSEMACROARGS, ALTTOKENPTR, and
  ALTINPUTTOKENS which are defined only here and in yylex.c
  It also resets static variable SeenError
*/

/*
   950811 memoryUsage() moved to unxhandl.c and macHandle.c
   970305 cleanitup() no longer resets NLINES
   970619 resets PARSERVALUE and INPUTLEVEL
   970620 cleans out inputstrings from INPUTSTACK
   970714 pops input levels all the way down
   971125 clears new global GLMRUNNING
   990204 resets new globals ACTIVEMACROS and MACRONAMES
*/
void cleanitup(void)
{
	WHERE("cleanitup");

	if (GUBED & 4096)
	{
		memoryUsage(NAMEFORWHERE);
	}
	
	SeenError = 0;

	Unscratch(); /* clears scratch symbol table (SCRATCH, keywords, UNDEF's)*/

	RemoveUndef(); /* clears ASSIGNED && temp variables from symbol table */

	cleanAssignedStack(); /*clears stack of pending assignments*/

	INTERRUPT = INTNOTSET;

	OUTSTR[0] = '\0';

	clearNproducts();
	
	if((ISATTY & ITTYIN) && BDEPTH == 0 && MAXERRORS == 0)
	{
		NERRORS = 0;
	}

	restoreFormat(); /* restore current default format */

	ClearYylexGlobals();
	ClearSyntaxGlobals();
	PARSERVALUE = (Symbolhandle) 0;
	while (INPUTLEVEL > 0)
	{
		popInputlevel();
	}
	RUNNINGGLM = (char *) 0;
	
	/*
	  clear out stack of macro names
	*/
	while (ACTIVEMACROS > 0)
	{
		ThisMacroName[0] = '\0';
		ACTIVEMACROS--;
	}
	
	/*
	  Dispose of inputStrings in INPUTSTACK
	*/
	cleanInputlevels(1);
	cleanLoops(0, MAXWDEPTH);
#ifdef READLINE
	rlpauseCleanup();
#endif /*READLINE*/
} /*cleanitup()*/


__YYSCLASS yytabelem yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
-1, 2,
	0, 1,
	125, 1,
	-2, 0,
-1, 223,
	309, 202,
	-2, 71,
-1, 275,
	284, 0,
	285, 0,
	288, 0,
	289, 0,
	61, 0,
	60, 0,
	62, 0,
	-2, 107,
-1, 276,
	284, 0,
	285, 0,
	288, 0,
	289, 0,
	61, 0,
	60, 0,
	62, 0,
	-2, 109,
-1, 277,
	284, 0,
	285, 0,
	288, 0,
	289, 0,
	61, 0,
	60, 0,
	62, 0,
	-2, 111,
-1, 278,
	284, 0,
	285, 0,
	288, 0,
	289, 0,
	61, 0,
	60, 0,
	62, 0,
	-2, 113,
-1, 279,
	284, 0,
	285, 0,
	288, 0,
	289, 0,
	61, 0,
	60, 0,
	62, 0,
	-2, 115,
-1, 280,
	284, 0,
	285, 0,
	288, 0,
	289, 0,
	61, 0,
	60, 0,
	62, 0,
	-2, 117,
-1, 281,
	284, 0,
	285, 0,
	288, 0,
	289, 0,
	61, 0,
	60, 0,
	62, 0,
	-2, 119,
-1, 301,
	309, 203,
	-2, 72,
	};
# define YYNPROD 239
# define YYLAST 1552
__YYSCLASS yytabelem yyact[]={

    12,    30,   329,   127,   231,   229,   232,   230,   234,   233,
   300,   152,   153,   115,    81,   169,    24,   324,    24,    50,
   177,   260,    78,   130,    72,   216,    76,   334,    57,    55,
   228,    56,    31,    63,    43,   162,   164,   166,    24,   221,
   168,   292,    24,   307,   173,    54,    68,   313,    70,   312,
    24,    24,    24,   305,   311,   231,   229,   232,   230,   234,
   233,   310,    86,   249,    86,    88,   110,    88,   315,   321,
    91,    89,   323,    90,   127,    97,   239,    77,   231,   229,
   232,   230,   234,   233,   154,   155,   129,    83,   106,   103,
   108,   227,   156,   231,   229,   232,   230,   234,   233,   204,
   206,   118,   178,   121,   119,   122,   120,   124,   123,   205,
    74,   326,   147,   146,    86,   301,   296,    88,   294,    36,
   208,   209,   210,   211,   212,   213,    22,   224,   220,    35,
    19,    34,   149,   147,   146,     5,   144,   127,    48,   130,
   162,   164,   166,   226,     4,     8,   145,    47,   147,   146,
   220,   144,   112,   133,   222,   333,   254,   258,    24,    32,
   256,   238,   255,   253,    27,   226,   259,    24,    24,    24,
    45,    41,   203,   202,   240,   245,   201,   200,   199,   198,
   261,   262,   263,   264,   265,   266,   267,   268,   269,   270,
   271,   272,   273,   274,   275,   276,   277,   278,   279,   280,
   281,   282,   283,   284,   285,   286,   197,    10,   291,    26,
    86,   335,   110,    88,   176,   196,    91,    89,   325,    90,
   125,    97,   195,   297,   194,   293,   193,   192,   150,     1,
   136,   191,   190,   189,   106,   103,   108,   188,   226,   187,
   186,   185,   163,   165,   167,   184,   183,   182,    86,   181,
   180,    88,   179,    49,    91,   314,    37,   317,   318,    97,
   319,    38,   298,    79,    51,    40,    87,    64,   332,   133,
    71,    69,   306,   304,    66,    67,    73,    75,    23,    52,
    65,    58,    59,    60,    61,    62,   336,   302,    20,   207,
    53,   117,   320,   116,    80,   308,    13,    11,   112,    85,
     3,   327,   114,   328,   114,   331,   309,     2,     0,    98,
     0,     0,   109,   107,     0,     0,   104,   105,   111,   113,
    99,    82,    99,    92,    93,    94,    95,    96,   102,   101,
   100,    24,    84,   337,   330,   338,     0,   339,     0,     0,
     0,     0,    15,    46,    42,     0,    21,    16,    44,     0,
     0,    28,    29,    17,   114,    18,    25,    14,    33,    86,
    43,   110,    88,     0,     0,    91,    89,     0,    90,     0,
    97,     0,    99,    92,    93,    94,    95,    96,     0,     6,
   250,   251,   252,   106,   103,   108,     0,     0,     0,    39,
     7,   152,   153,   151,    86,   236,   110,    88,     0,     0,
    91,    89,   295,    90,     0,    97,   237,     0,     0,     0,
   118,     0,   121,   119,   122,   120,   124,   123,   106,   103,
   108,     0,     0,     0,     0,     0,     0,    86,     0,   110,
    88,     0,     0,    91,    89,     0,    90,     0,    97,   137,
     9,     0,     9,     0,     0,     0,     0,   112,     0,     0,
   114,   106,   103,   108,     0,    98,     0,     0,   109,   107,
     0,     0,   104,   105,   111,   113,     9,     0,    99,    92,
    93,    94,    95,    96,   102,   101,   100,    36,     0,     0,
     0,     0,   112,     0,    22,     0,     0,    35,   114,    34,
     0,     0,     0,    98,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     8,     0,     0,    99,    92,    93,    94,
    95,    96,     0,     0,    86,   112,   110,    88,     0,   235,
    91,    89,     0,    90,     0,    97,     0,     0,     0,     0,
     0,   148,     0,     0,     0,     0,     0,     0,   106,   103,
   108,     0,     0,     0,    86,     0,   110,    88,     0,     0,
    91,    89,     0,    90,   128,    97,     0,     0,     0,     0,
   157,   158,     0,     0,     0,     0,     0,    26,   106,   103,
   108,     0,     0,     0,   172,   126,   174,   175,   143,     0,
     0,     0,     9,     0,     0,   159,   160,   161,     0,     0,
     0,     9,     9,     9,     0,   170,   171,     0,     0,   114,
   322,     0,   112,     0,    98,     0,     0,   109,   107,     0,
     0,   104,   105,   111,   113,     0,     0,    99,    92,    93,
    94,    95,    96,   102,   101,   100,     0,     0,     0,     0,
     0,     0,   112,     0,   114,     0,     0,     0,     0,    98,
     0,     0,   109,   107,     0,     0,   104,   105,   111,   113,
     0,     0,    99,    92,    93,    94,    95,    96,   102,   101,
   100,     0,     0,     0,     0,     0,   257,   114,     0,     0,
     0,     0,    98,     0,     0,   109,   107,   242,   243,   104,
   105,   111,   113,     0,     0,    99,    92,    93,    94,    95,
    96,   102,   101,   100,     0,     0,     0,     0,     0,   241,
    15,    46,    42,     0,    21,    16,    44,     0,     0,    28,
    29,    17,     0,    18,    25,    14,    33,     0,    43,     0,
     0,     0,     0,     0,     0,     0,     0,    86,     0,   110,
    88,     0,     0,    91,    89,     0,    90,     6,    97,     0,
     0,     0,     0,     0,     0,     0,     0,    39,     7,   152,
   153,   106,   103,   108,   114,     9,     0,     0,     0,    98,
     0,     0,   109,   107,     0,     0,   104,   105,   111,   113,
     0,     0,    99,    92,    93,    94,    95,    96,   102,   101,
   100,     0,     0,     0,   114,     0,   316,     0,     0,    98,
   303,     0,   109,   107,     0,     0,   104,   105,   111,   113,
     0,     0,    99,    92,    93,    94,    95,    96,   102,   101,
   100,    36,     0,     0,     0,   112,     0,     0,    22,     0,
     0,    35,     0,    34,     0,    86,     0,   110,    88,     0,
     0,    91,    89,     0,    90,     0,    97,     8,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,   106,
   103,   108,    86,     0,   110,    88,   241,     0,    91,    89,
     0,    90,     0,    97,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,   106,   103,   108,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,   141,     0,     0,     0,     0,     0,     0,
   138,    26,     0,   140,   225,   139,     0,     0,     0,     0,
     0,     0,     0,   112,     0,   141,     0,     0,     0,   218,
     0,     0,   138,   244,     0,   140,   225,   139,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
   112,   248,     0,    86,     0,     0,    88,     0,     0,    91,
    89,     0,    90,   214,    97,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,   114,     0,     0,
     0,     0,    98,     0,     0,   109,   107,     0,     0,   104,
   105,   111,   113,   142,     0,    99,    92,    93,    94,    95,
    96,   102,   101,     0,     0,    86,     0,   110,    88,     0,
   141,    91,    89,     0,    90,   142,    97,   138,   131,     0,
   140,     0,   139,     0,     0,     0,     0,     0,     0,   106,
   103,   108,     0,     0,     0,     0,   135,     0,     0,     0,
     0,     0,     0,     0,    15,    46,    42,     0,    21,    16,
    44,     0,     0,    28,    29,    17,     0,    18,    25,    14,
    33,   141,    43,     0,     0,     0,     0,     0,   138,     0,
     0,   140,   299,   139,     0,   114,     0,     0,     0,     0,
    98,     6,     0,   109,   107,     0,     0,   104,   105,   111,
   113,    39,     7,    99,    92,    93,    94,    95,    96,   102,
   142,     0,   114,     0,     0,     0,     0,    98,     0,     0,
   109,   107,     0,     0,   104,   105,   111,   113,     0,   141,
    99,    92,    93,    94,    95,    96,   138,    46,    42,   140,
   223,   139,    44,     0,     0,    28,    29,     0,     0,     0,
     0,     0,    33,     0,    43,   289,     0,   215,     0,    46,
    42,   142,   223,     0,    44,     0,     0,    28,    29,     0,
     0,     0,     0,   219,    33,     0,    43,     0,     0,     0,
     0,   141,     0,    39,   217,     0,     0,     0,   138,   131,
     0,   140,     0,   139,     0,   246,     0,     0,     0,     0,
     0,     0,     0,   114,     0,    39,   247,   135,    98,    86,
     0,     0,    88,     0,     0,    91,    89,     0,    90,   142,
    97,    99,    92,    93,    94,    95,    96,     0,     0,     0,
     0,     0,     0,   106,   103,   108,     0,     0,     0,     0,
     0,     0,     0,     0,    46,    42,     0,    21,     0,    44,
     0,     0,    28,    29,     0,   114,     0,    25,     0,    33,
    98,    43,     0,   109,   107,     0,     0,   104,   105,   111,
     0,   142,     0,    99,    92,    93,    94,    95,    96,     0,
   132,     0,   141,     0,     0,     0,     0,     0,     0,   138,
    39,   134,   140,     0,   139,    46,    42,     0,   223,     0,
    44,   141,     0,    28,    29,     0,     0,     0,   138,     0,
    33,   140,    43,   139,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,    39,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,    46,    42,     0,    21,     0,    44,     0,
     0,    28,    29,     0,     0,     0,     0,     0,    33,     0,
    43,     0,   142,   287,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,   290,
     0,   142,     0,     0,     0,     0,     0,     0,     0,    39,
   288,     0,     0,     0,     0,    46,    42,     0,    21,     0,
    44,     0,     0,    28,    29,     0,     0,     0,     0,     0,
    33,     0,    43,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,   132,     0,     0,     0,     0,     0,     0,     0,   114,
     0,    39,   134,     0,    98,     0,     0,   109,   107,     0,
     0,   104,   105,     0,     0,     0,     0,    99,    92,    93,
    94,    95,    96,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,    46,    42,     0,    21,
     0,    44,     0,     0,    28,    29,     0,     0,     0,    25,
     0,    33,     0,    43,     0,    46,    42,     0,    21,     0,
    44,     0,     0,    28,    29,     0,     0,     0,     0,     0,
    33,     0,    43,     0,     0,     0,     0,     0,     0,     0,
     0,     0,    39,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,    39 };
__YYSCLASS yytabelem yypact[]={

   778, -3000,   778, -3000, -3000, -3000, -3000, -3000, -3000,   -40,
   -14,  -279,    28, -3000, -3000, -3000,  -280, -3000, -3000,  -207,
   -88,    46,   967,   -17,  -124,    92,    86,  -179,    92,    92,
   -17,   -17,   -17, -3000,  1229,  1229,  1229, -3000, -3000,  1248,
   -17,   -17,    92,  1248,    92,    92, -3000, -3000, -3000, -3000,
 -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000,
 -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000,
 -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000,
 -3000, -3000, -3000, -3000, -3000, -3000,  -240, -3000, -3000, -3000,
 -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000,
 -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000,
 -3000, -3000, -3000, -3000,     8, -3000,    50,    41, -3000,  1248,
  1248,  1248,  1248,  1248,  1248, -3000, -3000,   860,  -217, -3000,
   478, -3000, -3000,   102, -3000, -3000, -3000, -3000,  1128,  1248,
  1248,  1248,   444,  -232,   -17,    92,    92, -3000, -3000,   882,
   -62,   778,   778,   778, -3000, -3000, -3000, -3000, -3000, -3000,
 -3000, -3000,    26, -3000,    26, -3000,  1153, -3000,   816,  -109,
 -3000, -3000, -3000,   391, -3000, -3000, -3000, -3000,  -239,  1248,
  1248,  1248,  1248,  1248,  1248,  1248,  1248,  1248,  1248,  1248,
  1248,  1248,  1248,  1248,  1248,  1248,  1248,  1248,  1248,  1248,
  1248,  1248,  1248,  1248,  1076, -3000, -3000,  1248,   508,   508,
   508,   508,   508,   508, -3000, -3000,   -52, -3000, -3000, -3000,
   358,    72,  1018,    46,  -299, -3000,  -145, -3000, -3000, -3000,
 -3000, -3000, -3000, -3000, -3000,   -17, -3000, -3000, -3000, -3000,
 -3000,  -255, -3000, -3000, -3000,     2, -3000, -3000, -3000, -3000,
   -71,   -76,   -78,   -55,    92,   -55,   -55, -3000,   -55,   -54,
 -3000,   212,   212,    78,    26,    26,    26,    26,    26,    78,
    78,    26,   691,   789,   816,   907,   907,   907,   907,   907,
   907,   907,  1153,  1153,   959,   959,   323,   -21, -3000, -3000,
 -3000,   508, -3000, -3000,  -254, -3000, -3000,   174,    67, -3000,
  1248,   -17,  1248,  -306,  1248, -3000, -3000, -3000, -3000, -3000,
 -3000, -3000, -3000, -3000, -3000,   778, -3000, -3000, -3000, -3000,
 -3000,  -294,   -66, -3000, -3000, -3000, -3000,   508,   508, -3000,
 -3000,   508,  1248,   -55, -3000, -3000,  1248,   508, -3000,   508 };
__YYSCLASS yytabelem yypgo[]={

     0,   228,   307,   300,   144,   135,     1,   297,    32,   439,
   207,     0,   296,   293,   291,   130,   289,   288,   554,   287,
   286,   278,   273,    15,   268,    30,   531,    39,   127,   266,
   265,   261,   256,   252,   250,   249,   247,   246,   245,   241,
   240,   239,   237,   233,   232,   231,   227,   226,   224,   222,
   215,   206,   179,   178,   177,   176,   173,   172,   171,   170,
   166,   164,   163,   162,   160,   159,   157,   156,   155,   154,
    25,   118 };
__YYSCLASS yytabelem yyr1[]={

     0,     1,     1,     2,     2,     3,     3,     6,     7,     8,
     8,     4,     4,     4,     4,     4,     4,     4,     4,     4,
     4,     4,     4,     4,     4,     4,     4,     4,     4,    13,
     5,    14,     5,    16,     9,     9,    19,     9,    20,     9,
    22,     9,    24,     9,     9,     9,     9,     9,     9,     9,
     9,     9,     9,     9,    25,    25,    25,    25,    25,    25,
    10,    10,    27,    21,    21,    17,    17,    30,    31,    15,
    15,    15,    15,    15,    33,    15,    34,    15,    35,    15,
    36,    15,    37,    15,    38,    15,    39,    15,    40,    15,
    41,    15,    42,    15,    43,    15,    44,    15,    45,    15,
    46,    15,    15,    15,    15,    15,    47,    15,    48,    15,
    49,    15,    50,    15,    51,    15,    52,    15,    53,    15,
    54,    15,    55,    15,    56,    15,    57,    15,    15,    15,
    15,    60,    58,    32,    32,    32,    32,    32,    32,    11,
    11,    11,    11,    62,    11,    63,    11,    64,    11,    11,
    11,    11,    11,    11,    12,    12,    12,    12,    12,    12,
    12,    12,    12,    12,    12,    12,    12,    12,    12,    12,
    12,    12,    12,    12,    12,    12,    12,    12,    12,    12,
    12,    12,    12,    12,    65,    65,    65,    65,    65,    65,
    65,    65,    65,    65,    66,    61,    67,    68,    61,    61,
    59,    59,    28,    28,    23,    23,    69,    69,    69,    69,
    69,    69,    70,    70,    70,    70,    70,    26,    26,    26,
    26,    26,    26,    26,    26,    18,    18,    18,    18,    18,
    18,    71,    18,    29,    29,    29,    29,    29,    29 };
__YYSCLASS yytabelem yyr2[]={

     0,     3,     3,     3,     5,     3,     5,     7,     9,     9,
     9,     3,     3,     3,     5,     5,     5,     5,     5,     5,
     5,     5,     5,     5,     3,     5,     3,     3,     5,     1,
     7,     1,     7,     1,     9,     5,     1,    11,     1,    15,
     1,    11,     1,    13,     7,     7,     7,     7,     7,     7,
    11,     7,     9,     7,     3,     3,     3,     3,     3,     3,
     5,     7,     7,     5,     7,     7,     5,     5,     5,     3,
     3,     3,     5,     3,     1,     9,     1,     9,     1,     9,
     1,     9,     1,     9,     1,     9,     1,     9,     1,     9,
     1,     9,     1,     9,     1,     9,     1,     9,     1,     9,
     1,     9,     7,     5,     5,     5,     1,     9,     1,     9,
     1,     9,     1,     9,     1,     9,     1,     9,     1,     9,
     1,     9,     1,     9,     1,     9,     1,     9,     5,     3,
     3,     1,     9,     5,     7,     5,     5,     7,     7,     3,
     3,     3,     3,     1,     9,     1,     9,     1,     9,     3,
     3,     3,     5,     3,     5,     5,     5,     5,     5,     5,
     5,     5,     5,     5,     5,     5,     5,     5,     5,     5,
     5,     5,     5,     5,     5,     5,     5,     5,     5,     5,
     5,     5,     5,     5,     5,     9,     5,     5,     5,     5,
     5,     5,     5,     7,     1,     9,     1,     1,    13,     5,
     3,     5,     3,     5,     7,     5,     3,     5,     5,     7,
     7,     5,     3,     3,     3,     5,     5,     5,     7,     7,
     7,     7,     5,     5,     5,     5,     5,     7,     7,     5,
     5,     1,     8,    11,    11,     9,     7,     7,     7 };
__YYSCLASS yytabelem yychk[]={

 -3000,    -1,    -2,    -3,    -4,    -5,   293,   304,    59,    -9,
   -10,    -7,   -11,   -12,   271,   256,   261,   267,   269,   -15,
   -17,   260,    40,   -21,   -23,   270,   123,   -61,   265,   266,
    -6,    -8,   -65,   272,    45,    43,    33,   -32,   -31,   303,
   -30,   -58,   258,   274,   262,   -59,   257,    -4,    -5,   293,
    59,   304,   293,   304,    59,    43,    45,    42,   295,   296,
   297,   298,   299,    47,   281,   294,   288,   289,    60,   285,
    62,   284,    38,   290,   124,   291,    40,    91,    36,   277,
   308,   293,   293,    59,   304,   271,    36,   -29,    39,    43,
    45,    42,   295,   296,   297,   298,   299,    47,   281,   294,
   302,   301,   300,    61,   288,   289,    60,   285,    62,   284,
    38,   290,   124,   291,   276,   293,   -13,   -14,   308,   311,
   313,   310,   312,   315,   314,   308,   -18,    91,   -18,    40,
   -11,    41,   293,   -15,   304,    59,   -10,    -9,    40,    45,
    43,    33,   123,   -18,   260,   270,   258,   257,   -26,    40,
    -1,   307,   305,   306,   263,   264,   271,   -26,   -26,   -18,
   -18,   -18,   -11,   -10,   -11,   -10,   -11,   -10,   -11,   -23,
   -18,   -18,   -26,   -11,   -26,   -26,   -28,   260,   -23,   -33,
   -34,   -35,   -36,   -37,   -38,   -39,   -40,   -41,   -42,   -43,
   -44,   -45,   -46,   -47,   -48,   -49,   -50,   -51,   -52,   -53,
   -54,   -55,   -56,   -57,    91,    59,    59,   -16,   -11,   -11,
   -11,   -11,   -11,   -11,    93,   277,   -70,   304,    59,   293,
   -11,   -27,   -69,   260,   -28,    44,   -23,   308,   -25,   311,
   313,   310,   312,   315,   314,    41,   293,   304,    59,   308,
   -25,   -18,   -26,   -26,    41,   -70,   293,   304,    59,   125,
    -1,    -1,    -1,   -62,   -67,   -63,   -64,   275,   -66,   -60,
   260,   -11,   -11,   -11,   -11,   -11,   -11,   -11,   -11,   -11,
   -11,   -11,   -11,   -11,   -11,   -11,   -11,   -11,   -11,   -11,
   -11,   -11,   -11,   -11,   -11,   -11,   -11,   277,   304,    59,
   293,   -11,    93,   277,   -71,    44,    44,   -11,   -27,    44,
   309,   260,   -19,   -18,   -22,   308,   -25,    41,   293,   304,
    59,   125,   125,   125,    -6,   123,   -26,    -6,    -6,    -6,
    -8,   123,   277,    93,   271,    44,    44,   -11,   -11,   308,
   -25,   -11,   -24,   -68,    93,   277,   -20,   -11,    -6,   -11 };
__YYSCLASS yytabelem yydef[]={

     0,    -2,    -2,     2,     3,     5,    11,    12,    13,   141,
     0,     0,     0,    24,    26,    27,     0,    29,    31,   139,
   140,    71,     0,    69,     0,     0,     0,   142,     0,     0,
   149,   150,   151,   153,     0,     0,     0,    70,    73,     0,
   129,   130,     0,     0,     0,     0,   200,     4,     6,    14,
    15,    16,    17,    18,    19,   158,   159,   160,   161,   162,
   163,   164,   165,   166,   167,   168,   169,   170,   171,   172,
   173,   174,   175,   176,   177,   178,   179,   180,   181,   182,
   183,    20,    21,    22,    23,    25,     0,    66,    67,    74,
    76,    78,    80,    82,    84,    86,    88,    90,    92,    94,
    96,    98,   100,   106,   108,   110,   112,   114,   116,   118,
   120,   122,   124,   126,     0,    28,     0,     0,    33,     0,
     0,     0,     0,     0,     0,    35,   187,     0,   190,   152,
     0,    68,   133,   139,   135,   136,   157,   141,     0,     0,
     0,     0,     0,   186,    72,     0,     0,   201,    60,     0,
     0,     0,     0,     0,   143,   196,   199,   145,   147,   184,
   189,   192,   103,   154,   104,   155,   128,   156,   105,     0,
   191,   188,    63,   205,   194,   131,    65,   202,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,    30,    32,     0,    44,    45,
    46,    47,    48,    49,   225,   226,     0,   229,   230,   231,
   212,   213,   214,    -2,     0,   206,     0,    36,    53,    54,
    55,    56,    57,    58,    59,   102,   134,   137,   138,    40,
    51,   193,    61,    64,   217,     0,   222,   223,   224,     7,
     0,     0,     0,     0,     0,     0,     0,   204,     0,     0,
   203,    75,    77,    79,    81,    83,    85,    87,    89,    91,
    93,    95,    97,    99,   101,    -2,    -2,    -2,    -2,    -2,
    -2,    -2,   121,   123,   125,   127,     0,     0,   236,   237,
   238,    34,   227,   228,     0,   207,   208,   215,   216,   211,
     0,    -2,     0,   185,     0,    42,    52,   218,   219,   220,
   221,     8,     9,    10,   144,     0,   197,   146,   148,   195,
   132,     0,     0,   235,   232,   209,   210,    62,    37,    38,
    50,    41,     0,     0,   233,   234,     0,    43,   198,    39 };
typedef struct { char *t_name; int t_val; } yytoktype;
#ifndef YYDEBUG
#	define YYDEBUG	0	/* don't allow debugging */
#endif

#if YYDEBUG

__YYSCLASS yytoktype yytoks[] =
{
	"MACROTOKEN",	257,
	"BLTINTOKEN",	258,
	"NUMBER",	259,
	"VAR",	260,
	"END",	261,
	"IF",	262,
	"ELSE",	263,
	"ELSEIF",	264,
	"WHILE",	265,
	"FOR",	266,
	"BREAK",	267,
	"BREAKALL",	268,
	"NEXTREP",	269,
	"BATCH",	270,
	"ERROR",	271,
	"FATALERROR",	272,
	"BADTOKEN",	273,
	"LEFTANGLE",	274,
	"RIGHTANGLE",	275,
	"LEFTBRACK2",	276,
	"RIGHTBRACK2",	277,
	"MULT",	278,
	"ADD",	279,
	"DIV",	280,
	"MOD",	281,
	"SUB",	282,
	"EXP",	283,
	"GE",	284,
	"LE",	285,
	"GT",	286,
	"LT",	287,
	"EQ",	288,
	"NE",	289,
	"AND",	290,
	"OR",	291,
	"NOT",	292,
	"ENDOFL",	293,
	"POW",	294,
	"MATMULT",	295,
	"TRMATMULT",	296,
	"MATMULTTR",	297,
	"MATDIV",	298,
	"DIVMAT",	299,
	"BITAND",	300,
	"BITXOR",	301,
	"BITOR",	302,
	"BITNOT",	303,
	"SEMI1",	304,
	"INLINEHEAD",	305,
	"OUTLINEHEAD",	306,
	"EVALHEAD",	307,
	"ASSIGN",	308,
	"ASSIGN1",	309,
	"ASSIGNMULT",	310,
	"ASSIGNADD",	311,
	"ASSIGNDIV",	312,
	"ASSIGNSUB",	313,
	"ASSIGNPOW",	314,
	"ASSIGNMOD",	315,
	",",	44,
	"|",	124,
	"&",	38,
	"!",	33,
	"=",	61,
	"<",	60,
	">",	62,
	"+",	43,
	"-",	45,
	"*",	42,
	"/",	47,
	"UNARYMINUS",	316,
	"$",	36,
	"[",	91,
	"(",	40,
	"\'",	39,
	"-unknown-",	-1	/* ends search */
};

__YYSCLASS char * yyreds[] =
{
	"-no such reduction-",
	"cmpdline : cmpdnobreak",
	"cmpdline : cmpdbreak",
	"cmpdnobreak : line",
	"cmpdnobreak : cmpdnobreak line",
	"cmpdbreak : break",
	"cmpdbreak : cmpdnobreak break",
	"cmpdstatement : '{' cmpdline '}'",
	"evaluatedstring : '{' EVALHEAD cmpdline '}'",
	"macrobody : '{' INLINEHEAD cmpdline '}'",
	"macrobody : '{' OUTLINEHEAD cmpdline '}'",
	"line : ENDOFL",
	"line : SEMI1",
	"line : ';'",
	"line : assgn ENDOFL",
	"line : assgn ';'",
	"line : assgn SEMI1",
	"line : batch ENDOFL",
	"line : batch SEMI1",
	"line : batch ';'",
	"line : evaluatedstring ENDOFL",
	"line : expr ENDOFL",
	"line : expr ';'",
	"line : expr SEMI1",
	"line : badbatch",
	"line : expr ERROR",
	"line : ERROR",
	"line : error",
	"line : END ENDOFL",
	"break : BREAK",
	"break : BREAK ';'",
	"break : NEXTREP",
	"break : NEXTREP ';'",
	"assgn : expr1 ASSIGN",
	"assgn : expr1 ASSIGN expr",
	"assgn : component ASSIGN",
	"assgn : VAR bracklist ASSIGN",
	"assgn : VAR bracklist ASSIGN expr",
	"assgn : '(' expr ')' bracklist ASSIGN",
	"assgn : '(' expr ')' bracklist ASSIGN expr",
	"assgn : functionvalue bracklist ASSIGN",
	"assgn : functionvalue bracklist ASSIGN expr",
	"assgn : byname VAR bracklist ASSIGN",
	"assgn : byname VAR bracklist ASSIGN expr",
	"assgn : expr1 ASSIGNADD expr",
	"assgn : expr1 ASSIGNSUB expr",
	"assgn : expr1 ASSIGNMULT expr",
	"assgn : expr1 ASSIGNDIV expr",
	"assgn : expr1 ASSIGNMOD expr",
	"assgn : expr1 ASSIGNPOW expr",
	"assgn : '(' expr ')' bracklist assignop",
	"assgn : functionvalue bracklist assignop",
	"assgn : byname VAR bracklist assignop",
	"assgn : VAR bracklist assignop",
	"assignop : ASSIGNADD",
	"assignop : ASSIGNSUB",
	"assignop : ASSIGNMULT",
	"assignop : ASSIGNDIV",
	"assignop : ASSIGNMOD",
	"assignop : ASSIGNPOW",
	"batch : BATCH parenlist",
	"batch : byname BATCH parenlist",
	"keyword : compname ASSIGN1 expr",
	"functionvalue : BLTINTOKEN parenlist",
	"functionvalue : byname BLTINTOKEN parenlist",
	"component : expr '$' compname",
	"component : expr brack2list",
	"transpexpr : expr '\''",
	"nullexpr : '(' ')'",
	"expr1 : functionvalue",
	"expr1 : missingrparen",
	"expr1 : VAR",
	"expr1 : byname VAR",
	"expr1 : nullexpr",
	"expr1 : expr '+'",
	"expr1 : expr '+' expr",
	"expr1 : expr '-'",
	"expr1 : expr '-' expr",
	"expr1 : expr '*'",
	"expr1 : expr '*' expr",
	"expr1 : expr MATMULT",
	"expr1 : expr MATMULT expr",
	"expr1 : expr TRMATMULT",
	"expr1 : expr TRMATMULT expr",
	"expr1 : expr MATMULTTR",
	"expr1 : expr MATMULTTR expr",
	"expr1 : expr MATDIV",
	"expr1 : expr MATDIV expr",
	"expr1 : expr DIVMAT",
	"expr1 : expr DIVMAT expr",
	"expr1 : expr '/'",
	"expr1 : expr '/' expr",
	"expr1 : expr MOD",
	"expr1 : expr MOD expr",
	"expr1 : expr POW",
	"expr1 : expr POW expr",
	"expr1 : expr BITOR",
	"expr1 : expr BITOR expr",
	"expr1 : expr BITXOR",
	"expr1 : expr BITXOR expr",
	"expr1 : expr BITAND",
	"expr1 : expr BITAND expr",
	"expr1 : '(' expr ')'",
	"expr1 : '-' expr",
	"expr1 : '+' expr",
	"expr1 : BITNOT expr",
	"expr1 : expr '='",
	"expr1 : expr '=' expr",
	"expr1 : expr EQ",
	"expr1 : expr EQ expr",
	"expr1 : expr NE",
	"expr1 : expr NE expr",
	"expr1 : expr '<'",
	"expr1 : expr '<' expr",
	"expr1 : expr LE",
	"expr1 : expr LE expr",
	"expr1 : expr '>'",
	"expr1 : expr '>' expr",
	"expr1 : expr GE",
	"expr1 : expr GE expr",
	"expr1 : expr '&'",
	"expr1 : expr '&' expr",
	"expr1 : expr AND",
	"expr1 : expr AND expr",
	"expr1 : expr '|'",
	"expr1 : expr '|' expr",
	"expr1 : expr OR",
	"expr1 : expr OR expr",
	"expr1 : '!' expr",
	"expr1 : transpexpr",
	"expr1 : expandedmacro",
	"expandedmacro : macro parenlist",
	"expandedmacro : macro parenlist macrobody",
	"missingrparen : '(' ENDOFL",
	"missingrparen : '(' expr1 ENDOFL",
	"missingrparen : '(' SEMI1",
	"missingrparen : '(' ';'",
	"missingrparen : '(' expr1 SEMI1",
	"missingrparen : '(' expr1 ';'",
	"expr : expr1",
	"expr : component",
	"expr : assgn",
	"expr : ifstart",
	"expr : ifstart ELSE",
	"expr : ifstart ELSE cmpdstatement",
	"expr : WHILE parenlist",
	"expr : WHILE parenlist cmpdstatement",
	"expr : FOR parenlist",
	"expr : FOR parenlist cmpdstatement",
	"expr : cmpdstatement",
	"expr : macrobody",
	"expr : subscripted",
	"expr : VAR '('",
	"expr : FATALERROR",
	"badbatch : '-' batch",
	"badbatch : '+' batch",
	"badbatch : '!' batch",
	"badbatch : '(' batch",
	"badbatch : batch '+'",
	"badbatch : batch '-'",
	"badbatch : batch '*'",
	"badbatch : batch MATMULT",
	"badbatch : batch TRMATMULT",
	"badbatch : batch MATMULTTR",
	"badbatch : batch MATDIV",
	"badbatch : batch DIVMAT",
	"badbatch : batch '/'",
	"badbatch : batch MOD",
	"badbatch : batch POW",
	"badbatch : batch EQ",
	"badbatch : batch NE",
	"badbatch : batch '<'",
	"badbatch : batch LE",
	"badbatch : batch '>'",
	"badbatch : batch GE",
	"badbatch : batch '&'",
	"badbatch : batch AND",
	"badbatch : batch '|'",
	"badbatch : batch OR",
	"badbatch : batch '('",
	"badbatch : batch '['",
	"badbatch : batch '$'",
	"badbatch : batch RIGHTBRACK2",
	"badbatch : batch ASSIGN",
	"subscripted : cmpdstatement bracklist",
	"subscripted : '(' expr ')' bracklist",
	"subscripted : functionvalue bracklist",
	"subscripted : component bracklist",
	"subscripted : expandedmacro bracklist",
	"subscripted : macrobody bracklist",
	"subscripted : VAR bracklist",
	"subscripted : transpexpr bracklist",
	"subscripted : subscripted bracklist",
	"subscripted : byname VAR bracklist",
	"ifstart : IF parenlist",
	"ifstart : IF parenlist cmpdstatement",
	"ifstart : ifstart ELSEIF",
	"ifstart : ifstart ELSEIF parenlist",
	"ifstart : ifstart ELSEIF parenlist cmpdstatement",
	"ifstart : ifstart ERROR",
	"macro : MACROTOKEN",
	"macro : byname MACROTOKEN",
	"compname : VAR",
	"compname : byname VAR",
	"byname : LEFTANGLE expr RIGHTANGLE",
	"byname : LEFTANGLE expr",
	"list : ','",
	"list : expr ','",
	"list : keyword ','",
	"list : list expr ','",
	"list : list keyword ','",
	"list : list ','",
	"completelist : expr",
	"completelist : keyword",
	"completelist : list",
	"completelist : list expr",
	"completelist : list keyword",
	"parenlist : '(' ')'",
	"parenlist : '(' completelist ')'",
	"parenlist : '(' completelist ENDOFL",
	"parenlist : '(' completelist SEMI1",
	"parenlist : '(' completelist ';'",
	"parenlist : '(' ENDOFL",
	"parenlist : '(' SEMI1",
	"parenlist : '(' ';'",
	"bracklist : '[' ']'",
	"bracklist : '[' RIGHTBRACK2",
	"bracklist : '[' completelist ']'",
	"bracklist : '[' completelist RIGHTBRACK2",
	"bracklist : '[' SEMI1",
	"bracklist : '[' ';'",
	"bracklist : '[' ENDOFL",
	"bracklist : '[' ENDOFL ERROR",
	"brack2list : LEFTBRACK2 '[' expr RIGHTBRACK2 ']'",
	"brack2list : LEFTBRACK2 '[' expr RIGHTBRACK2 RIGHTBRACK2",
	"brack2list : LEFTBRACK2 '[' RIGHTBRACK2 ']'",
	"brack2list : LEFTBRACK2 '[' SEMI1",
	"brack2list : LEFTBRACK2 '[' ';'",
	"brack2list : LEFTBRACK2 '[' ENDOFL",
};
#endif /* YYDEBUG */
#define YYFLAG  (-3000)
/* @(#) $Revision: 70.7 $ */    

/*
** Skeleton parser driver for yacc output
*/

#if defined(NLS) && !defined(NL_SETN)
#include <msgbuf.h>
#endif

#ifndef nl_msg
#define nl_msg(i,s) (s)
#endif

/*
** yacc user known macros and defines
*/
#define YYERROR		goto yyerrlab

#ifndef __RUNTIME_YYMAXDEPTH
#define YYACCEPT	return(0)
#define YYABORT		return(1)
#else
#define YYACCEPT	{free_stacks(); return(0);}
#define YYABORT		{free_stacks(); return(1);}
#endif

#define YYBACKUP( newtoken, newvalue )\
{\
	if ( yychar >= 0 || ( yyr2[ yytmp ] >> 1 ) != 1 )\
	{\
		yyerror( (nl_msg(30001,"syntax error - cannot backup")) );\
		goto yyerrlab;\
	}\
	yychar = newtoken;\
	yystate = *yyps;\
	yylval = newvalue;\
	goto yynewstate;\
}
#define YYRECOVERING()	(!!yyerrflag)
#ifndef YYDEBUG
#	define YYDEBUG	1	/* make debugging available */
#endif

/*
** user known globals
*/
int yydebug;			/* set to 1 to get debugging */

/*
** driver internal defines
*/
/* define for YYFLAG now generated by yacc program. */
/*#define YYFLAG		(FLAGVAL)*/

/*
** global variables used by the parser
*/
# ifndef __RUNTIME_YYMAXDEPTH
#if (0) /* yyv and yys now are local for re-entrancy*/
__YYSCLASS YYSTYPE yyv[ YYMAXDEPTH ];	/* value stack */
__YYSCLASS int yys[ YYMAXDEPTH ];		/* state stack */
#endif /*0*/
# else
__YYSCLASS YYSTYPE *yyv;			/* pointer to malloc'ed value stack */
__YYSCLASS int *yys;			/* pointer to malloc'ed stack stack */

#if defined(__STDC__) || defined (__cplusplus)
#include <stdlib.h>
#else
	extern char *malloc();
	extern char *realloc();
	extern void free();
#endif /* __STDC__ or __cplusplus */


static int allocate_stacks(); 
static void free_stacks();
# ifndef YYINCREMENT
# define YYINCREMENT (YYMAXDEPTH/2) + 10
# endif
# endif	/* __RUNTIME_YYMAXDEPTH */
static long  yymaxdepth = YYMAXDEPTH;

#if (0) /* yypv, yyps yystate are local for re-entrancy */
__YYSCLASS YYSTYPE *yypv;			/* top of value stack */
__YYSCLASS int *yyps;			/* top of state stack */

__YYSCLASS int yystate;			/* current state */
__YYSCLASS int yytmp;			/* extra var (lasts between blocks) */

int yynerrs;			/* number of errors */
__YYSCLASS int yyerrflag;			/* error recovery flag */
int yychar;			/* current input token number */
#endif /*0*/



/*
** yyparse - return 0 if worked, 1 if syntax error not recovered from
*/
int
yyparse()
{
	register YYSTYPE *yypvt;	/* top of value stack for $vars */
	YYSTYPE *yypv;			/* top of value stack */
	int *yyps;			/* top of state stack */
	int yystate;			/* current state */
	int yytmp;			/* extra var (lasts between blocks) */
	int yynerrs;			/* number of errors */
	int yyerrflag;			/* error recovery flag */
	int yychar;			/* current input token number */
	YYSTYPE yyv[ YYMAXDEPTH ];	/* value stack */
	int yys[ YYMAXDEPTH ];		/* state stack */
#ifdef MACINTOSH
	register yytabelem *yyexca = Pyyexca;
	register yytabelem *yyact = Pyyact;
	register yytabelem *yypact = Pyypact;
	register yytabelem *yypgo = Pyypgo;
	register yytabelem *yyr1 = Pyyr1;
	register yytabelem *yyr2 = Pyyr2;
	register yytabelem *yychk = Pyychk;
	register yytabelem *yydef = Pyydef;
#endif /*MACINTOSH*/
	WHERE("yyparse");

	/*
	** Initialize externals - yyparse may be called more than once
	*/
# ifdef __RUNTIME_YYMAXDEPTH
	if (allocate_stacks()) YYABORT;
# endif
	yypv = &yyv[-1];
	yyps = &yys[-1];
	yystate = 0;
	yytmp = 0;
	yynerrs = 0;
	yyerrflag = 0;
	yychar = -1;

	goto yystack;
	{
		register YYSTYPE *yy_pv;	/* top of value stack */
		register int *yy_ps;		/* top of state stack */
		register int yy_state;		/* current state */
		register int  yy_n;		/* internal state number info */

		/*
		** get globals into registers.
		** branch to here only if YYBACKUP was called.
		*/
	yynewstate:
		yy_pv = yypv;
		yy_ps = yyps;
		yy_state = yystate;
		goto yy_newstate;

		/*
		** get globals into registers.
		** either we just started, or we just finished a reduction
		*/
	yystack:
		yy_pv = yypv;
		yy_ps = yyps;
		yy_state = yystate;

		/*
		** top of for (;;) loop while no reductions done
		*/
	yy_stack:
		/*
		** put a state and value onto the stacks
		*/
#if YYDEBUG
		/*
		** if debugging, look up token value in list of value vs.
		** name pairs.  0 and negative (-1) are special values.
		** Note: linear search is used since time is not a real
		** consideration while debugging.
		*/
		if ( yydebug )
		{
			register int yy_i;

			(sprintf(OUTSTR, "State %d, token ", yy_state ), myprint(OUTSTR));
			if ( yychar == 0 )
				(sprintf(OUTSTR, "end-of-file\n" ), myprint(OUTSTR));
			else if ( yychar < 0 )
				(sprintf(OUTSTR, "-none-\n" ), myprint(OUTSTR));
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
						break;
				}
				(sprintf(OUTSTR, "%s\n", yytoks[yy_i].t_name ), myprint(OUTSTR));
			}
		}
#endif /* YYDEBUG */
		if ( ++yy_ps >= &yys[ yymaxdepth ] )	/* room on stack? */
		{
# ifndef __RUNTIME_YYMAXDEPTH
			yyerror( (nl_msg(30002,"ERROR: Parser stack overflow; probably too deep macro recursion")) );
			YYABORT;
# else
			/* save old stack bases to recalculate pointers */
			YYSTYPE * yyv_old = yyv;
			int * yys_old = yys;
			yymaxdepth += YYINCREMENT;
			yys = (int *) realloc(yys, yymaxdepth * sizeof(int));
			yyv = (YYSTYPE *) realloc(yyv, yymaxdepth * sizeof(YYSTYPE));
			if (yys==0 || yyv==0) {
			    yyerror( (nl_msg(30002,"ERROR: Parser stack overflow; probably too deep macro recursion")) );
			    YYABORT;
			    }
			/* Reset pointers into stack */
			yy_ps = (yy_ps - yys_old) + yys;
			yyps = (yyps - yys_old) + yys;
			yy_pv = (yy_pv - yyv_old) + yyv;
			yypv = (yypv - yyv_old) + yyv;
# endif

		}
		*yy_ps = yy_state;
		*++yy_pv = yyval;

		/*
		** we have a new state - find out what to do
		*/
	yy_newstate:
		if ( ( yy_n = yypact[ yy_state ] ) <= YYFLAG )
			goto yydefault;		/* simple state */
#if YYDEBUG
		/*
		** if debugging, need to mark whether new token grabbed
		*/
		yytmp = yychar < 0;
#endif
		if ( ( yychar < 0 ) && ( ( yychar = yylex() ) < 0 ) )
			yychar = 0;		/* reached EOF */
#if YYDEBUG
		if ( yydebug && yytmp )
		{
			register int yy_i;

			(sprintf(OUTSTR, "Received token " ), myprint(OUTSTR));
			if ( yychar == 0 )
				(sprintf(OUTSTR, "end-of-file\n" ), myprint(OUTSTR));
			else if ( yychar < 0 )
				(sprintf(OUTSTR, "-none-\n" ), myprint(OUTSTR));
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
						break;
				}
				(sprintf(OUTSTR, "%s\n", yytoks[yy_i].t_name ), myprint(OUTSTR));
			}
		}
#endif /* YYDEBUG */
		if ( ( ( yy_n += yychar ) < 0 ) || ( yy_n >= YYLAST ) )
			goto yydefault;
		if ( yychk[ yy_n = yyact[ yy_n ] ] == yychar )	/*valid shift*/
		{
			yychar = -1;
			yyval = yylval;
			yy_state = yy_n;
			if ( yyerrflag > 0 )
				yyerrflag--;
			goto yy_stack;
		}

	yydefault:
		if ( ( yy_n = yydef[ yy_state ] ) == -2 )
		{
#if YYDEBUG
			yytmp = yychar < 0;
#endif
			if ( ( yychar < 0 ) && ( ( yychar = yylex() ) < 0 ) )
				yychar = 0;		/* reached EOF */
#if YYDEBUG
			if ( yydebug && yytmp )
			{
				register int yy_i;

				(sprintf(OUTSTR, "Received token " ), myprint(OUTSTR));
				if ( yychar == 0 )
					(sprintf(OUTSTR, "end-of-file\n" ), myprint(OUTSTR));
				else if ( yychar < 0 )
					(sprintf(OUTSTR, "-none-\n" ), myprint(OUTSTR));
				else
				{
					for ( yy_i = 0;
						yytoks[yy_i].t_val >= 0;
						yy_i++ )
					{
						if ( yytoks[yy_i].t_val
							== yychar )
						{
							break;
						}
					}
					(sprintf(OUTSTR, "%s\n", yytoks[yy_i].t_name ), myprint(OUTSTR));
				}
			}
#endif /* YYDEBUG */
			/*
			** look through exception table
			*/
			{
				register yytabelem *yyxi = yyexca;

				while ( ( *yyxi != -1 ) ||
					( yyxi[1] != yy_state ) )
				{
					yyxi += 2;
				}
				while ( ( *(yyxi += 2) >= 0 ) &&
					( *yyxi != yychar ) )
					;
				if ( ( yy_n = yyxi[1] ) < 0 )
					YYACCEPT;
			}
		}

		/*
		** check for syntax error
		*/
		if ( yy_n == 0 )	/* have an error */
		{
			/* no worry about speed here! */
			switch ( yyerrflag )
			{
			case 0:		/* new error */
				yyerror( (nl_msg(30003,"ERROR: problem with input")) );
				yynerrs++;
				goto skip_init;
			yyerrlab:
				/*
				** get globals into registers.
				** we have a user generated syntax type error
				*/
				yy_pv = yypv;
				yy_ps = yyps;
				yy_state = yystate;
				yynerrs++;
			skip_init:
			case 1:
			case 2:		/* incompletely recovered error */
					/* try again... */
				yyerrflag = 3;
				/*
				** find state where "error" is a legal
				** shift action
				*/
				while ( yy_ps >= yys )
				{
					yy_n = yypact[ *yy_ps ] + YYERRCODE;
					if ( yy_n >= 0 && yy_n < YYLAST &&
						yychk[yyact[yy_n]] == YYERRCODE)					{
						/*
						** simulate shift of "error"
						*/
						yy_state = yyact[ yy_n ];
						goto yy_stack;
					}
					/*
					** current state has no shift on
					** "error", pop stack
					*/
#if YYDEBUG
#	define _POP_ "Error recovery pops state %d, uncovers state %d\n"
					if ( yydebug )
						(sprintf(OUTSTR, _POP_, *yy_ps,
							yy_ps[-1] ), myprint(OUTSTR));
#	undef _POP_
#endif
					yy_ps--;
					yy_pv--;
				}
				/*
				** there is no state on stack with "error" as
				** a valid shift.  give up.
				*/
				YYABORT;
			case 3:		/* no shift yet; eat a token */
#if YYDEBUG
				/*
				** if debugging, look up token in list of
				** pairs.  0 and negative shouldn't occur,
				** but since timing doesn't matter when
				** debugging, it doesn't hurt to leave the
				** tests here.
				*/
				if ( yydebug )
				{
					register int yy_i;

					(sprintf(OUTSTR, "Error recovery discards " ), myprint(OUTSTR));
					if ( yychar == 0 )
						(sprintf(OUTSTR, "token end-of-file\n" ), myprint(OUTSTR));
					else if ( yychar < 0 )
						(sprintf(OUTSTR, "token -none-\n" ), myprint(OUTSTR));
					else
					{
						for ( yy_i = 0;
							yytoks[yy_i].t_val >= 0;
							yy_i++ )
						{
							if ( yytoks[yy_i].t_val
								== yychar )
							{
								break;
							}
						}
						(sprintf(OUTSTR, "token %s\n",
							yytoks[yy_i].t_name ), myprint(OUTSTR));
					}
				}
#endif /* YYDEBUG */
				if ( yychar == 0 )	/* reached EOF. quit */
					YYABORT;
				yychar = -1;
				goto yy_newstate;
			}
		}/* end if ( yy_n == 0 ) */
		/*
		** reduction by production yy_n
		** put stack tops, etc. so things right after switch
		*/
#if YYDEBUG
		/*
		** if debugging, print the string that is the user's
		** specification of the reduction which is just about
		** to be done.
		*/
		if ( yydebug )
			(sprintf(OUTSTR, "Reduce by (%d) \"%s\"\n",
				yy_n, yyreds[ yy_n ] ), myprint(OUTSTR));
#endif
		yytmp = yy_n;			/* value to switch over */
		yypvt = yy_pv;			/* $vars top of value stack */
		/*
		** Look in goto table for next state
		** Sorry about using yy_state here as temporary
		** register variable, but why not, if it works...
		** If yyr2[ yy_n ] doesn't have the low order bit
		** set, then there is no action to be done for
		** this reduction.  So, no saving & unsaving of
		** registers done.  The only difference between the
		** code just after the if and the body of the if is
		** the goto yy_stack in the body.  This way the test
		** can be made before the choice of what to do is needed.
		*/
		{
			/* length of production doubled with extra bit */
			register int yy_len = yyr2[ yy_n ];

			if ( !( yy_len & 01 ) )
			{
				yy_len >>= 1;
				yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
				yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
					*( yy_ps -= yy_len ) + 1;
				if ( yy_state >= YYLAST ||
					yychk[ yy_state =
					yyact[ yy_state ] ] != -yy_n )
				{
					yy_state = yyact[ yypgo[ yy_n ] ];
				}
				goto yy_stack;
			}
			yy_len >>= 1;
			yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
			yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
				*( yy_ps -= yy_len ) + 1;
			if ( yy_state >= YYLAST ||
				yychk[ yy_state = yyact[ yy_state ] ] != -yy_n )
			{
				yy_state = yyact[ yypgo[ yy_n ] ];
			}
		}
					/* save until reenter driver code */
		yystate = yy_state;
		yyps = yy_ps;
		yypv = yy_pv;
	}
	/*
	** code supplied by user is placed in this switch
	*/
	switch( yytmp )
	{
		
case 1:
{yyval = yypvt[-0];/*Mac: fold 6 cases*/} break;
case 2:
{yyval = yypvt[-0];} break;
case 3:
{yyval = yypvt[-0];} break;
case 4:
{yyval = yypvt[-0];} break;
case 5:
{yyval = yypvt[-0];} break;
case 6:
{yyval = yypvt[-0];} break;
case 7:
{/*yylex ensures '}' preceded by ';'*/
									yyval = yypvt[-1];
								} break;
case 8:
{
		/*
		yylex returns EVALHEAD when it sees header of string being
		evaluated.
		*/
						yyval = parserCopy(yypvt[-1]);
					} break;
case 9:
{
		/*
		yylex returns INLINEHEAD when it sees header of inline macro
		It also increments MLEVEL.  On a macro expansion, this
		occurs after the macro is expanded.  Macroset() uses
		MLEVEL as the value of '$$'
		parserCopy() ensures the result is a scratch variable
		*/
						MLEVEL--;
						PopMacroName();

						yyval = parserCopy(yypvt[-1]);
						goto setScrName2;
					} break;
case 10:
{
		/*
		yylex returns OUTLINEHEAD when it sees header of an out-of-line macro
		It also increments MDEPTH.  On a macro expansion, this
		occurs after the macro is expanded.  Macroset() uses
		MDEPTH as the value of '$$'
		parserCopy() ensures the result is a scratch variable
		*/
						MDEPTH--;
						PopMacroName();
						
						yyval = parserCopy(yypvt[-1]);
						goto setScrName2;
					} break;
case 11:
{
						yyval = NULLSYMBOL;
					  lev0Return:
						if(BRACKETLEV == 0)
						{
							YYACCEPT;
						}
					} break;
case 12:
{ /* BRACKETLEV must be > 0 */
						yyval = NULLSYMBOL;
					} break;
case 13:
{
					yyval = NULLSYMBOL;
				  lev0Unscratch:
					
					if(BRACKETLEV == 0 && INPUTLEVEL == 0)
					{
						Unscratch();
					}
				} break;
case 14:
{
						yyval = yypvt[-1];
						goto lev0Return;
					} break;
case 15:
{
						yyval = yypvt[-1];
						goto lev0Unscratch;
					} break;
case 16:
{ /* BRACKETLEV must be > 0 */
						yyval = NULLSYMBOL;
						if (yypvt[-1] != (Symbolhandle) 0 && isscratch(NAME(yypvt[-1])))
						{
							Removesymbol(yypvt[-1]);
						}
					} break;
case 17:
{goto dobatch;/*Mac: fold 3 cases*/} break;
case 18:
{goto dobatch;} break;
case 19:
{
					  dobatch:
						if (runCommand(Lookup("batch"), yypvt[-1]) == (Symbolhandle) 0)
						{
							YYABORT;
						}
						else
						{
							YYACCEPT; /* terminate parse */
						}
					} break;
case 20:
{
									PARSERVALUE = yyval = yypvt[-1];
									YYACCEPT;
								} break;
case 21:
{
						yyval = yypvt[-1];
						
						if(BRACKETLEV == 0)
						{
							if (yypvt[-1] != (Symbolhandle) 0 &&
								!invisname(NAME(yypvt[-1])) && INPUTLEVEL == 0)
							{
								prexpr(yypvt[-1]);
							}
							YYACCEPT;
						}
					} break;
case 22:
{
						yyval = yypvt[-1];
						if(BRACKETLEV == 0)
						{
							if (yypvt[-1] != (Symbolhandle) 0 && !invisname(NAME(yypvt[-1])))
							{
								prexpr(yypvt[-1]);
							}
							if (INPUTLEVEL == 0)
							{
								Unscratch();
							}
						}
					} break;
case 23:
{ /* BRACKETLEV must be > 0 */
						yyval = NULLSYMBOL;
						if(yypvt[-1] != (Symbolhandle) 0 && isscratch(NAME(yypvt[-1])))
						{
							Removesymbol(yypvt[-1]);
						}
					} break;
case 24:
{
						yyerror("ERROR: illegal use of batch()");
						Removelist(yypvt[-0]);
		  				YYABORT; /* terminate parse */
					} break;
case 25:
{yyval = (Symbolhandle) 0;YYABORT;/*Mac: fold 3 cases*/} break;
case 26:
{yyval = (Symbolhandle) 0;YYABORT;} break;
case 27:
{yyval = (Symbolhandle) 0;YYABORT;} break;
case 28:
{ return(END); } break;
case 29:
{  /* break, break n, breakall */
					if(yypvt[-0] == (Symbolhandle) 0)
					{
						YYABORT;
					}
					TargetLevel = WDEPTH - (int) DATAVALUE(yypvt[-0],0);
					Removesymbol(yypvt[-0]);
					if(TargetLevel < 0)
					{
						YYABORT;
					}
					SetBreakTarget(TargetLevel);
					cleanLoops(TargetLevel,WDEPTH);
				} break;
case 30:
{
					yyval = NULLSYMBOL;
				} break;
case 31:
{
					if(yypvt[-0] == (Symbolhandle) 0)
					{
						YYABORT;
					}
					TargetLevel = WDEPTH - (int) DATAVALUE(yypvt[-0],0);
					Removesymbol(yypvt[-0]);
					if(TargetLevel < 0)
					{
						YYABORT;
					}
					SetNextTarget(TargetLevel);
					cleanLoops(TargetLevel + 1,WDEPTH);
				} break;
case 32:
{
					yyval = NULLSYMBOL;
				} break;
case 33:
{
							yyval = Assign(yypvt[-1],(Symbolhandle) 0,LEFTHANDSIDE);
							if(yyval == (Symbolhandle) 0)
							{
								YYABORT;
							}
						} break;
case 34:
{
							yyval = Assign(yypvt[-3],yypvt[-0],RIGHTHANDSIDE);
						  checkResult:
							if(yyval == (Symbolhandle) 0)
							{
								YYABORT;
							}
						} break;
case 35:
{
							yyerror("ERROR: illegal assignment to structure component");
							YYABORT;
						} break;
case 36:
{
							  subassigncheck2:
								yyval = Subassign(yypvt[-2],yypvt[-1],(Symbolhandle) 0,
											   LEFTHANDSIDE);
								if(yyval == (Symbolhandle) 0)
								{
									YYABORT;
								}
							} break;
case 37:
{
							  subassign5:
								yyval = Subassign(yypvt[-4],yypvt[-3],yypvt[-0],RIGHTHANDSIDE);
								Removelist(yypvt[-3]);
								if(yyval == (Symbolhandle) 0)
								{
									YYABORT;
								}
							} break;
case 38:
{
								yyval = Subassign(yypvt[-3],yypvt[-1],(Symbolhandle) 0,
											   LEFTHANDSIDE);
								if(yyval == (Symbolhandle) 0)
								{
									YYABORT;
								}
							} break;
case 39:
{
								yyval = Subassign(yypvt[-5],yypvt[-3],yypvt[-0],RIGHTHANDSIDE);
								Removelist(yypvt[-3]);
								if(yyval == (Symbolhandle) 0)
								{
									YYABORT;
								}
							} break;
case 40:
{
								goto subassigncheck2;
							} break;
case 41:
{
								goto subassign5;
							} break;
case 42:
{
							  subassigncheck3:
								yyval = Subassign(yypvt[-3],yypvt[-1],(Symbolhandle) 0,LEFTHANDSIDE);
								if(yyval == (Symbolhandle) 0)
								{
									YYABORT;
								}
							} break;
case 43:
{
							  subassign6:
								yyval = Subassign(yypvt[-5],yypvt[-3],yypvt[-0],RIGHTHANDSIDE);
								Removelist(yypvt[-3]);
								if(yyval == (Symbolhandle) 0)
								{
									YYABORT;
								}
							} break;
case 44:
{
								Op = ADD;
							  doassignop:
								yyval = Assign(yypvt[-2],(Symbolhandle) 0,-Op);
								if(yyval == (Symbolhandle) 0)
								{
									YYABORT;
								}
								yyval = Arith(yypvt[-2], yypvt[-0], Op);
								UNLOADSEG(Arith);
								if(yyval != (Symbolhandle) 0)
								{
									yyval = Assign(yypvt[-2],yyval,RIGHTHANDSIDE);
								}
								goto checkResult;
							} break;
case 45:
{
								Op = SUB;
								goto doassignop;
							} break;
case 46:
{
								Op = MULT;
								goto doassignop;
							} break;
case 47:
{
								Op = DIV ;
								goto doassignop;
							} break;
case 48:
{
								Op = MOD;
								goto doassignop;
							} break;
case 49:
{
								Op = EXP;
								goto doassignop;
							} break;
case 50:
{ /* Mac: fold 4 cases */
									AssignOp = LONGDATAVALUE(yypvt[-0],1);
									goto assignoperror;
								} break;
case 51:
{
									AssignOp = LONGDATAVALUE(yypvt[-0],1);
									goto assignoperror;
								} break;
case 52:
{
									AssignOp = LONGDATAVALUE(yypvt[-0],1);
									goto assignoperror;
								} break;
case 53:
{
									AssignOp = LONGDATAVALUE(yypvt[-0],1);
								  assignoperror:
									sprintf(OUTSTR,
											"ERROR: illegal left side of %s",
											opName(AssignOp));
									yyerror(OUTSTR);
									YYABORT;
								} break;
case 54:
{
	 						Op = ADD;
							AssignOp = ASSIGNADD;
						  setassignOp:
							{
								Symbolhandle symhOp = LongInstall(SCRATCH, 2);
								if(symhOp == (Symbolhandle) 0)
								{
									YYABORT;
								}
								LONGDATAVALUE(symhOp,0) = (long) Op;
								LONGDATAVALUE(symhOp,1) = (long) AssignOp;
								yyval = symhOp;
							}
						} break;
case 55:
{Op = SUB;AssignOp = ASSIGNSUB;goto setassignOp;} break;
case 56:
{Op = MULT;AssignOp = ASSIGNMULT;goto setassignOp;} break;
case 57:
{Op = DIV;AssignOp = ASSIGNDIV;goto setassignOp;} break;
case 58:
{Op = MOD;AssignOp = ASSIGNMOD;goto setassignOp;} break;
case 59:
{Op = EXP;AssignOp = ASSIGNPOW;goto setassignOp;} break;
case 60:
{
							yyval = yypvt[-0];
						} break;
case 61:
{
									yyval = yypvt[-0];
				 				} break;
case 62:
{
								yyval = Assign(yypvt[-2],yypvt[-0], KEYWORDASSIGN);

								goto checkResult;
							} break;
case 63:
{
										yyval = runCommand(yypvt[-1], yypvt[-0]);
									  setScrName1:
										OUTSTR[0] = '\0';
										if(yyval == (Symbolhandle) 0)
										{
											YYABORT;
										}
										setScratchName(yyval);
										CHECKINTERRUPT;
									} break;
case 64:
{
										yyval = runCommand(yypvt[-2],yypvt[-0]);
										goto setScrName1;
									} break;
case 65:
{
									yyval = Extract(yypvt[-2],yypvt[-0],1);
									
								  setScrName2:
									OUTSTR[0] = '\0';
									if(yyval == (Symbolhandle) 0)
									{
										YYABORT;
									}
									setScratchName(yyval);
								} break;
case 66:
{
							yyval = Extract(yypvt[-1],yypvt[-0],0);
							if (yypvt[-0] != (Symbolhandle) 0 && isscratch(NAME(yypvt[-0])))
							{
								Removesymbol(yypvt[-0]);
							}
							goto setScrName2;
						} break;
case 67:
{
							Symbolhandle list = Addlist( Makelist(), yypvt[-1] );
							FunctionSymbol   prime, *primeP = &prime;
							Symbolhandle     primeH = (Symbolhandle) &primeP;
							
							if(list == (Symbolhandle) 0)
							{
								YYABORT;
							}
							markFakeSymbol(primeH);
							setNDIMS(primeH, 1);
							setDIM(primeH, 1,1);
							setPREV(primeH, (Symbolhandle) 0);
							setNEXT(primeH, (Symbolhandle) 0);
							setTYPE(primeH, BLTIN);
							setFPTR(primeH, transpose);
							setNAME(primeH, "prime");
							yyval = runCommand(primeH, list);
							goto setScrName1;
				  		} break;
case 68:
{yyval = NULLSYMBOL;} break;
case 69:
{yyval = yypvt[-0];} break;
case 70:
{
						YYABORT;
					} break;
case 71:
{yyval = yypvt[-0];} break;
case 72:
{yyval = yypvt[-1];} break;
case 73:
{yyval = yypvt[-0];} break;
case 74:
{
						  copy1AndCheck:
							BlockBatch();
							yypvt[-1] = parserCopy(yypvt[-1]);
							if (yypvt[-1] == (Symbolhandle) 0) YYABORT;
						} break;
case 75:
{
							Op = ADD;
						  arith:
							UnBlockBatch();
							clearNproducts();
							yyval = Arith(yypvt[-3],yypvt[-0],Op);
							/* Arith() removes SCRATCH unconsumed args */
							UNLOADSEG(Arith);
							goto setScrName1;
						 } break;
case 76:
{goto copy1AndCheck;} break;
case 77:
{ Op = SUB;goto arith; } break;
case 78:
{goto copy1AndCheck;} break;
case 79:
{ Op = MULT;goto arith; } break;
case 80:
{goto copy1AndCheck;} break;
case 81:
{ Op = MATMULT;goto arith; } break;
case 82:
{goto copy1AndCheck;} break;
case 83:
{ Op = TRMATMULT;goto arith; } break;
case 84:
{goto copy1AndCheck;} break;
case 85:
{ Op = MATMULTTR;goto arith; } break;
case 86:
{goto copy1AndCheck;} break;
case 87:
{ Op = MATDIV;goto arith; } break;
case 88:
{goto copy1AndCheck;} break;
case 89:
{ Op = DIVMAT;goto arith; } break;
case 90:
{goto copy1AndCheck;} break;
case 91:
{ Op = DIV;goto arith; } break;
case 92:
{goto copy1AndCheck;} break;
case 93:
{ Op = MOD;goto arith; } break;
case 94:
{goto copy1AndCheck;} break;
case 95:
{ Op = EXP;goto arith; } break;
case 96:
{goto copy1AndCheck;} break;
case 97:
{ Op = BITOR;goto arith; } break;
case 98:
{goto copy1AndCheck;} break;
case 99:
{ Op = BITXOR;goto arith; } break;
case 100:
{goto copy1AndCheck;} break;
case 101:
{ Op = BITAND;goto arith; } break;
case 102:
{yyval = parserCopy(yypvt[-1]);} break;
case 103:
{
									Op = '-';
								  unary:
									yyval = Unary(yypvt[-0],Op);
									/*Unary() reuses space if arg is SCRATCH */
									UNLOADSEG(Unary);
									goto setScrName1;
								} break;
case 104:
{
									Op = '+';
									goto unary;
								} break;
case 105:
{
									Op = BITNOT;
									goto unary;
								} break;
case 106:
{goto copy1AndCheck;} break;
case 107:
{
						Op = EQ;
					  logic:
						UnBlockBatch();
						clearNproducts();
						yyval = Logic(yypvt[-3],yypvt[-0],Op);
						/*Logic() removes unconsumed scratch args */
						UNLOADSEG(Logic);
						goto setScrName1;
					} break;
case 108:
{goto copy1AndCheck;} break;
case 109:
{ Op = EQ; goto logic; } break;
case 110:
{goto copy1AndCheck;} break;
case 111:
{ Op = NE; goto logic; } break;
case 112:
{goto copy1AndCheck;} break;
case 113:
{ Op = LT; goto logic; } break;
case 114:
{goto copy1AndCheck;} break;
case 115:
{ Op = LE; goto logic; } break;
case 116:
{goto copy1AndCheck;} break;
case 117:
{ Op = GT; goto logic; } break;
case 118:
{goto copy1AndCheck;} break;
case 119:
{ Op = GE; goto logic; } break;
case 120:
{goto copy1AndCheck;} break;
case 121:
{ Op = AND; goto logic; } break;
case 122:
{goto copy1AndCheck;} break;
case 123:
{ Op = AND; goto logic; } break;
case 124:
{goto copy1AndCheck;} break;
case 125:
{ Op = OR; goto logic; } break;
case 126:
{goto copy1AndCheck;} break;
case 127:
{ Op = OR; goto logic; } break;
case 128:
{
							yyval = Logic(yypvt[-0],yypvt[-0],NOT);
							UNLOADSEG(Unary);
							goto setScrName1;
						} break;
case 129:
{ yyval = yypvt[-0]; } break;
case 130:
{yyval = yypvt[-0]; goto setScrName2;} break;
case 131:
{
							long reply = Macrosetup(yypvt[-1],yypvt[-0]);

							Removelist(yypvt[-0]);
							if(!reply)
							{
								YYABORT;
							}
						} break;
case 132:
{
							yyval = yypvt[-0];
						} break;
case 133:
{goto missingRparen; /*Mac: fold 6 cases*/} break;
case 134:
{goto missingRparen;} break;
case 135:
{goto missingRparen;} break;
case 136:
{goto missingRparen;} break;
case 137:
{goto missingRparen;} break;
case 138:
{
								  missingRparen:
									yyval = 0;
									yyerror("ERROR: missing ')'");
									YYABORT;
								} break;
case 139:
{yyval = yypvt[-0];/*Mac: fold 3 cases*/} break;
case 140:
{yyval = yypvt[-0];} break;
case 141:
{yyval = yypvt[-0];} break;
case 142:
{
					IDEPTH--;
					yyval = yypvt[-0];
				} break;
case 143:
{
						if(IFCONDITIONS[IDEPTH-1] > 0)
						{
							 yyval = yypvt[-1];
							 SkipBracketBlock();
						}
						CHECKINTERRUPT;
						if(!Ifsetup((Symbolhandle) 0, ELSE))
						{
							yyval = 0;
							YYABORT;
						}
					} break;
case 144:
{
						if(IFCONDITIONS[IDEPTH-1] == 0)
						{
							 yyval = yypvt[-0];
						}
						IDEPTH--;
					} break;
case 145:
{
							int    ifOK;

							CHECKINTERRUPT;
							ifOK = Ifsetup(yypvt[-0], WHILE);
							Removelist(yypvt[-0]);
							if(!ifOK)
							{
								yyval = 0;
								YYABORT;
							}
							if(WHILECONDITIONS[WDEPTH-1] == 0)
							{
								SkipBracketBlock();
							}
							else if(WHILELIMITS[WDEPTH-1] <= 0)
							{ /* too many reps */
								YYABORT ;
							}
						} break;
case 146:
{ /* post WHILE checking */
							if(WHILECONDITIONS[WDEPTH-1])
							{
								setupfaketokens(WHILE);
								if(yypvt[-0] != (Symbolhandle) 0 && BRACKETLEV > 0 &&
									 isscratch(NAME(yypvt[-0])))
								{
									Removesymbol(yypvt[-0]);
									yypvt[-0] = NULLSYMBOL;
								}
							}
							else
							{
								WHILELIMITS[WDEPTH-1] = MAXWHILE;
							}
							WDEPTH--;
							yyval = yypvt[-0];
						} break;
case 147:
{
							int   ifOK;

							CHECKINTERRUPT;
							ifOK = Ifsetup(yypvt[-0], FOR);
							Removelist(yypvt[-0]);
							if(!ifOK)
							{
								yyval = 0;
								YYABORT;
							}
							if (ifOK < 0)
							{
								SkipBracketBlock();
							}
						 } break;
case 148:
{
							if(WHILECONDITIONS[WDEPTH-1] &&
								 --WHILELIMITS[WDEPTH-1] > 0)
							{
								setupfaketokens(FOR);
								if(yypvt[-0] != (Symbolhandle) 0 && BRACKETLEV > 0 &&
									 isscratch(NAME(yypvt[-0])))
								{
									Removesymbol(yypvt[-0]);
									yypvt[-0] = NULLSYMBOL;
								}
							}
							else
							{
								WHILELIMITS[WDEPTH-1] = MAXWHILE;
							}
							WDEPTH--;
							yyval = yypvt[-0];
						} break;
case 149:
{yyval = yypvt[-0];/*Mac: fold 3 cases*/} break;
case 150:
{yyval = yypvt[-0];} break;
case 151:
{yyval = yypvt[-0];} break;
case 152:
{
						notfunction(yypvt[-1]);
						yyval = 0;
						YYABORT;
					} break;
case 153:
{
						return (FATALERROR);
					} break;
case 154:
{yyval = yypvt[-0];/*Mac: fold 4 cases*/} break;
case 155:
{yyval = yypvt[-0];} break;
case 156:
{yyval = yypvt[-0];} break;
case 157:
{yyval = yypvt[-0];} break;
case 158:
{yyval = yypvt[-1];/*Mac: fold 26 cases*/} break;
case 159:
{yyval = yypvt[-1];} break;
case 160:
{yyval = yypvt[-1];} break;
case 161:
{yyval = yypvt[-1];} break;
case 162:
{yyval = yypvt[-1];} break;
case 163:
{yyval = yypvt[-1];} break;
case 164:
{yyval = yypvt[-1];} break;
case 165:
{yyval = yypvt[-1];} break;
case 166:
{yyval = yypvt[-1];} break;
case 167:
{yyval = yypvt[-1];} break;
case 168:
{yyval = yypvt[-1];} break;
case 169:
{yyval = yypvt[-1];} break;
case 170:
{yyval = yypvt[-1];} break;
case 171:
{yyval = yypvt[-1];} break;
case 172:
{yyval = yypvt[-1];} break;
case 173:
{yyval = yypvt[-1];} break;
case 174:
{yyval = yypvt[-1];} break;
case 175:
{yyval = yypvt[-1];} break;
case 176:
{yyval = yypvt[-1];} break;
case 177:
{yyval = yypvt[-1];} break;
case 178:
{yyval = yypvt[-1];} break;
case 179:
{yyval = yypvt[-1];} break;
case 180:
{yyval = yypvt[-1];} break;
case 181:
{yyval = yypvt[-1];} break;
case 182:
{yyval = yypvt[-1];} break;
case 183:
{yyval = yypvt[-1];} break;
case 184:
{
									yyval = Element(yypvt[-1],yypvt[-0]);
									Removelist(yypvt[-0]);
									goto setScrName2;
								} break;
case 185:
{
								  bracklist4:
									yyval = Element(yypvt[-2],yypvt[-0]);
									Removelist(yypvt[-0]);
									goto setScrName2;
								} break;
case 186:
{ goto bracklist2; /*Mac: fold 6 cases */} break;
case 187:
{ goto bracklist2; } break;
case 188:
{ goto bracklist2; } break;
case 189:
{ goto bracklist2; } break;
case 190:
{ goto bracklist2; } break;
case 191:
{ goto bracklist2; } break;
case 192:
{
							  bracklist2:
								yyval = Element(yypvt[-1],yypvt[-0]);
								Removelist(yypvt[-0]);
								goto setScrName2;
							} break;
case 193:
{
								yyval = Element(yypvt[-2],yypvt[-0]);
								Removelist(yypvt[-0]);
								goto setScrName2;
							} break;
case 194:
{
							int    ifOK;

							CHECKINTERRUPT;

							ifOK = Ifsetup(yypvt[-0], IF);
							Removelist(yypvt[-0]);
							if(!ifOK)
							{
								yyval = 0;
								YYABORT;
							}
							if(IFCONDITIONS[IDEPTH-1] == 0)
							{
								SkipBracketBlock();
							}
						} break;
case 195:
{ /* post IF checking */
							yyval = (IFCONDITIONS[IDEPTH-1] ) ? yypvt[-0] : NULLSYMBOL;
						} break;
case 196:
{
							if(IFCONDITIONS[IDEPTH-1] > 0)
							{
								SkipParenBlock();
							}
						} break;
case 197:
{
							int    ifOK;

							if(IFCONDITIONS[IDEPTH-1] > 0)
							{
								SkipBracketBlock();
								Removelist(yypvt[-0]);
								yypvt[-0] = (Symbolhandle) 0;
							}
							CHECKINTERRUPT;

							ifOK = Ifsetup(yypvt[-0], ELSEIF);

							if(yypvt[-0] != (Symbolhandle) 0)
							{
								Removelist(yypvt[-0]);
							}
							if(!ifOK)
							{
								yyval = 0;
								YYABORT;
							}
							if(IFCONDITIONS[IDEPTH-1] == 0)
							{
								SkipBracketBlock();
							}
						} break;
case 198:
{
							if(yypvt[-2] == (Symbolhandle) 0)
							{
								yyval = yypvt[-5];
							}
							else if(IFCONDITIONS[IDEPTH-1] > 0)
							{
								yyval = yypvt[-0];
							}
							else
							{
								yyval = NULLSYMBOL;
							}
						} break;
case 199:
{ yyval = 0 ; YYABORT;} break;
case 200:
{yyval = yypvt[-0];} break;
case 201:
{
								yyval = yypvt[-1];
								PARSEMACROARGS = ParseMacroArgs;
							} break;
case 202:
{yyval = yypvt[-0];} break;
case 203:
{yyval = yypvt[-1];} break;
case 204:
{
						yyval = Byname(yypvt[-1],&BynameType,&ParseMacroArgs);

						if(yyval == (Symbolhandle) 0)
						{
							YYABORT;
						}
						else
						{
							SetNextChar(BynameType);
						}
					} break;
case 205:
{
							yyval = 0;
							yyerror("ERROR: missing '>>'");
							YYABORT;
						} break;
case 206:
{
							  growMake:
								yyval = Growlist(Makelist());
								goto checkResult;
							} break;
case 207:
{goto growAddMake;/*Mac: fold 2 cases*/} break;
case 208:
{
							  growAddMake:
								yyval = Growlist(Addlist(Makelist(),yypvt[-1]));
								goto checkResult;
							} break;
case 209:
{goto growAdd; /*Mac: fold 2 cases*/} break;
case 210:
{
							  growAdd:
								yyval = Growlist(Addlist(yypvt[-2],yypvt[-1]));
								goto checkResult;
							} break;
case 211:
{
								yyval = Growlist(yypvt[-1]);
								goto checkResult;
							} break;
case 212:
{goto addMake; /*Mac: fold 2 cases*/} break;
case 213:
{
							  addMake:
								yyval = Addlist( Makelist(), yypvt[-0] );
								goto checkResult;
							} break;
case 214:
{
								yyval = yypvt[-0];
								goto checkResult;
							} break;
case 215:
{goto addlist;/*Mac: fold 2 cases*/} break;
case 216:
{
							  addlist:
								yyval = Addlist( yypvt[-1], yypvt[-0]);
								goto checkResult;
							} break;
case 217:
{
									  makelist:
										yyval = Makelist();
										goto checkResult;
									} break;
case 218:
{
									yyval = yypvt[-1];
								} break;
case 219:
{goto missingRparen; /*Mac: fold 6 cases*/} break;
case 220:
{goto missingRparen;} break;
case 221:
{goto missingRparen;} break;
case 222:
{goto missingRparen;} break;
case 223:
{goto missingRparen;} break;
case 224:
{goto missingRparen;} break;
case 225:
{goto makelist;/*Mac: fold 2 cases*/} break;
case 226:
{goto makelist;} break;
case 227:
{ yyval = yypvt[-1]; goto checkResult;/*Mac: fold 2 cases*/} break;
case 228:
{ yyval = yypvt[-1]; goto checkResult;} break;
case 229:
{goto missingRbracket;/*Mac: fold 3 cases*/} break;
case 230:
{goto missingRbracket;} break;
case 231:
{
					  missingRbracket:
						yyerror("ERROR: missing ']'");
						yyval = 0;
						YYABORT;
					} break;
case 233:
{ yyval = yypvt[-2]; goto checkResult;/*Mac: fold 2 cases*/} break;
case 234:
{ yyval = yypvt[-2]; goto checkResult;} break;
case 235:
{ yyval = (Symbolhandle) 0; } break;
case 236:
{goto missingRbrack2; /*Mac: fold 3 cases*/} break;
case 237:
{goto missingRbrack2;} break;
case 238:
{
								  missingRbrack2:
									yyerror("ERROR: missing ']]'");
									yyval = 0;
									YYABORT;
								} break;
	}
	goto yystack;		/* reset registers in driver code */
}

# ifdef __RUNTIME_YYMAXDEPTH

static int allocate_stacks() {
	/* allocate the yys and yyv stacks */
	yys = (int *) malloc(yymaxdepth * sizeof(int));
	yyv = (YYSTYPE *) malloc(yymaxdepth * sizeof(YYSTYPE));

	if (yys==0 || yyv==0) {
	   yyerror( (nl_msg(30004,"unable to allocate space for yacc stacks")) );
	   return(1);
	   }
	else return(0);

}


static void free_stacks() {
	if (yys!=0) free((char *) yys);
	if (yyv!=0) free((char *) yyv);
}

# endif  /* defined(__RUNTIME_YYMAXDEPTH) */

