
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


/* Deleted definition of __YYSCLASS yytabelem yyexca[] */
# define YYNPROD 239
# define YYLAST 1552
/* Deleted definition of __YYSCLASS yytabelem yyact[] */
/* Deleted definition of __YYSCLASS yytabelem yypact[] */
/* Deleted definition of __YYSCLASS yytabelem yypgo[] */
/* Deleted definition of __YYSCLASS yytabelem yyr1[] */
/* Deleted definition of __YYSCLASS yytabelem yyr2[] */
/* Deleted definition of __YYSCLASS yytabelem yychk[] */
/* Deleted definition of __YYSCLASS yytabelem yydef[] */
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
		/*Folding 6 cases into 1*/
case 2:
case 3:
case 4:
case 5:
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
		/*Folding 3 cases into 1*/
case 18:
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
		/*Folding 3 cases into 1*/
case 26:
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
		/*Folding 4 cases into 1*/
case 51:
case 52:
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
		/*Folding 6 cases into 1*/
case 134:
case 135:
case 136:
case 137:
case 138:
{
								  missingRparen:
									yyval = 0;
									yyerror("ERROR: missing ')'");
									YYABORT;
								} break;
case 139:
		/*Folding 3 cases into 1*/
case 140:
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
		/*Folding 3 cases into 1*/
case 150:
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
		/*Folding 4 cases into 1*/
case 155:
case 156:
case 157:
{yyval = yypvt[-0];} break;
case 158:
		/*Folding 26 cases into 1*/
case 159:
case 160:
case 161:
case 162:
case 163:
case 164:
case 165:
case 166:
case 167:
case 168:
case 169:
case 170:
case 171:
case 172:
case 173:
case 174:
case 175:
case 176:
case 177:
case 178:
case 179:
case 180:
case 181:
case 182:
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
		/*Folding 6 cases into 1*/
case 187:
case 188:
case 189:
case 190:
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
		/*Folding 2 cases into 1*/
case 208:
{
							  growAddMake:
								yyval = Growlist(Addlist(Makelist(),yypvt[-1]));
								goto checkResult;
							} break;
case 209:
		/*Folding 2 cases into 1*/
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
		/*Folding 2 cases into 1*/
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
		/*Folding 2 cases into 1*/
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
		/*Folding 6 cases into 1*/
case 220:
case 221:
case 222:
case 223:
case 224:
{goto missingRparen;} break;
case 225:
		/*Folding 2 cases into 1*/
case 226:
{goto makelist;} break;
case 227:
		/*Folding 2 cases into 1*/
case 228:
{ yyval = yypvt[-1]; goto checkResult;} break;
case 229:
		/*Folding 3 cases into 1*/
case 230:
case 231:
{
					  missingRbracket:
						yyerror("ERROR: missing ']'");
						yyval = 0;
						YYABORT;
					} break;
case 233:
		/*Folding 2 cases into 1*/
case 234:
{ yyval = yypvt[-2]; goto checkResult;} break;
case 235:
{ yyval = (Symbolhandle) 0; } break;
case 236:
		/*Folding 3 cases into 1*/
case 237:
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

