%{
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

%}
/* 
  Be very careful about deleting tokens so that the values of data types
  CHAR, REAL, STRUC, MACRO, LOGIC, and PLOTINFO do not change since the
  values are used in save().  If they were changed, they would render
  save files unreadable, although the current version of asciisave files
  would remain readable

  The previous comment is no longer operative.  The current version of
  save, producing files headed "////MacAnovaSaveFilex3///", where x >= 3
  use symbolic names for variable types instead of the numerical values of
  the tokens.  The codes necessary to restore save files from previous versions
  are defined in globdefs.h (CHAR_V31, REAL_V31, etc.).

  Actually, none of the types CHAR, REAL, LIST, STRUC, UNDEF, LOGIC,
  PLOTINFO, GARB, LONG, or NUMBER is, in fact, used as a token any more
  These types might well be moved to globdefs.h, but as long as any
  types are defined here, all are.  Both MACRO and BLTIN are still tokens.

  970714 all types (including MACRO and BLTIN) now defined in globdefs.h
  with new tokens MACROTOKEN and BLTINTOKEN introduced here and in yylex().
  NUMBER is not a real token, but is a temporary token in yylex(),
  translated to VAR before return.
*/
%token MACROTOKEN BLTINTOKEN
%token NUMBER
%token VAR END
%token IF ELSE ELSEIF WHILE FOR BREAK BREAKALL NEXTREP BATCH ERROR FATALERROR
%token BADTOKEN
%token LEFTANGLE RIGHTANGLE
%token LEFTBRACK2 RIGHTBRACK2
%token MULT ADD DIV MOD SUB EXP GE LE GT LT EQ NE AND OR NOT ENDOFL POW
%token MATMULT TRMATMULT MATMULTTR MATDIV DIVMAT
%token BITAND BITXOR BITOR BITNOT
%token SEMI1
%token INLINEHEAD OUTLINEHEAD EVALHEAD
%nonassoc ASSIGN ASSIGN1
%nonassoc ASSIGNMULT ASSIGNADD ASSIGNDIV ASSIGNSUB ASSIGNPOW ASSIGNMOD
%left ','
%left BITOR
%left BITXOR
%left BITAND
%nonassoc BITNOT
%left '|' OR
%left '&' AND
%nonassoc '!'
%nonassoc GE LE GT LT EQ NE '=' '<' '>'
%left '+' '-'
%left '*' '/' MOD
%left MATMULT TRMATMULT MATMULTTR MATDIV DIVMAT
%left UNARYMINUS
%right POW
%left '$'
%left LEFTBRACK2
%left '['
%left '('
%left '\''

%%
cmpdline: cmpdnobreak   {$$ = $1;/*Mac: fold 6 cases*/}
	| cmpdbreak 		{$$ = $1;}
	;

cmpdnobreak: line		{$$ = $1;}
	| cmpdnobreak line	{$$ = $2;}
	;

cmpdbreak: break		{$$ = $1;}
	| cmpdnobreak break {$$ = $2;}
	;

cmpdstatement: '{' cmpdline '}' {/*yylex ensures '}' preceded by ';'*/
									$$ = $2;
								}
	;

evaluatedstring: '{' EVALHEAD cmpdline '}'
					{
		/*
		yylex returns EVALHEAD when it sees header of string being
		evaluated.
		*/
						$$ = parserCopy($3);
					}
	;

macrobody:	'{' INLINEHEAD cmpdline '}'
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

						$$ = parserCopy($3);
						goto setScrName2;
					}
	|	'{' OUTLINEHEAD cmpdline '}'
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
						
						$$ = parserCopy($3);
						goto setScrName2;
					}
line:	  ENDOFL	{
						$$ = NULLSYMBOL;
					  lev0Return:
						if(BRACKETLEV == 0)
						{
							YYACCEPT;
						}
					}
	| SEMI1			{ /* BRACKETLEV must be > 0 */
						$$ = NULLSYMBOL;
					}
	| ';'		{
					$$ = NULLSYMBOL;
				  lev0Unscratch:
					
					if(BRACKETLEV == 0 && INPUTLEVEL == 0)
					{
						Unscratch();
					}
				}
	| assgn ENDOFL	{
						$$ = $1;
						goto lev0Return;
					}
	| assgn ';'		{
						$$ = $1;
						goto lev0Unscratch;
					}
	| assgn SEMI1	{ /* BRACKETLEV must be > 0 */
						$$ = NULLSYMBOL;
						if ($1 != (Symbolhandle) 0 && isscratch(NAME($1)))
						{
							Removesymbol($1);
						}
					}
	| batch ENDOFL	{goto dobatch;/*Mac: fold 3 cases*/}

	| batch SEMI1	{goto dobatch;}

	| batch ';'		{
					  dobatch:
						if (runCommand(Lookup("batch"), $1) == (Symbolhandle) 0)
						{
							YYABORT;
						}
						else
						{
							YYACCEPT; /* terminate parse */
						}
					}

	| evaluatedstring ENDOFL	{
									PARSERVALUE = $$ = $1;
									YYACCEPT;
								}

	| expr ENDOFL	{
						$$ = $1;
						
						if(BRACKETLEV == 0)
						{
							if ($1 != (Symbolhandle) 0 &&
								!invisname(NAME($1)) && INPUTLEVEL == 0)
							{
								prexpr($1);
							}
							YYACCEPT;
						}
					}
	| expr ';'		{
						$$ = $1;
						if(BRACKETLEV == 0)
						{
							if ($1 != (Symbolhandle) 0 && !invisname(NAME($1)))
							{
								prexpr($1);
							}
							if (INPUTLEVEL == 0)
							{
								Unscratch();
							}
						}
					}
	| expr SEMI1	{ /* BRACKETLEV must be > 0 */
						$$ = NULLSYMBOL;
						if($1 != (Symbolhandle) 0 && isscratch(NAME($1)))
						{
							Removesymbol($1);
						}
					}
	| badbatch		{
						yyerror("ERROR: illegal use of batch()");
						Removelist($1);
		  				YYABORT; /* terminate parse */
					}
	| expr ERROR	{$$ = (Symbolhandle) 0;YYABORT;/*Mac: fold 3 cases*/}
	| ERROR			{$$ = (Symbolhandle) 0;YYABORT;}
	| error			{$$ = (Symbolhandle) 0;YYABORT;}
	| END ENDOFL	{ return(END); }
	;

break:	BREAK	{  /* break, break n, breakall */
					if($1 == (Symbolhandle) 0)
					{
						YYABORT;
					}
					TargetLevel = WDEPTH - (int) DATAVALUE($1,0);
					Removesymbol($1);
					if(TargetLevel < 0)
					{
						YYABORT;
					}
					SetBreakTarget(TargetLevel);
					cleanLoops(TargetLevel,WDEPTH);
				}
			';'  /* yylex will ensure the ';' is found */
				{
					$$ = NULLSYMBOL;
				}
	| NEXTREP	{
					if($1 == (Symbolhandle) 0)
					{
						YYABORT;
					}
					TargetLevel = WDEPTH - (int) DATAVALUE($1,0);
					Removesymbol($1);
					if(TargetLevel < 0)
					{
						YYABORT;
					}
					SetNextTarget(TargetLevel);
					cleanLoops(TargetLevel + 1,WDEPTH);
				}
			';'  /* yylex will ensure the ';' is found */
				{
					$$ = NULLSYMBOL;
				}
	;

/*
	Assignment to scratch variable (e.g., (1+3) <- a, is grammatically
	correct, but Assign() diagnoses error
*/

assgn:	expr1 ASSIGN    {
							$$ = Assign($1,(Symbolhandle) 0,LEFTHANDSIDE);
							if($$ == (Symbolhandle) 0)
							{
								YYABORT;
							}
						}
					 	expr
						{
							$$ = Assign($1,$4,RIGHTHANDSIDE);
						  checkResult:
							if($$ == (Symbolhandle) 0)
							{
								YYABORT;
							}
						}
	| component ASSIGN	{
							yyerror("ERROR: illegal assignment to structure component");
							YYABORT;
						}

	| VAR bracklist ASSIGN 	{
							  subassigncheck2:
								$$ = Subassign($1,$2,(Symbolhandle) 0,
											   LEFTHANDSIDE);
								if($$ == (Symbolhandle) 0)
								{
									YYABORT;
								}
							}
						expr
							{
							  subassign5:
								$$ = Subassign($1,$2,$5,RIGHTHANDSIDE);
								Removelist($2);
								if($$ == (Symbolhandle) 0)
								{
									YYABORT;
								}
							}
	| '(' expr ')' bracklist ASSIGN	
							{
								$$ = Subassign($2,$4,(Symbolhandle) 0,
											   LEFTHANDSIDE);
								if($$ == (Symbolhandle) 0)
								{
									YYABORT;
								}
							}
						expr
							{
								$$ = Subassign($2,$4,$7,RIGHTHANDSIDE);
								Removelist($4);
								if($$ == (Symbolhandle) 0)
								{
									YYABORT;
								}
							}
	| functionvalue bracklist ASSIGN	
							{
								goto subassigncheck2;
							}
						expr
							{
								goto subassign5;
							}
	| byname VAR bracklist ASSIGN
							{
							  subassigncheck3:
								$$ = Subassign($1,$3,(Symbolhandle) 0,LEFTHANDSIDE);
								if($$ == (Symbolhandle) 0)
								{
									YYABORT;
								}
							}
						expr
							{
							  subassign6:
								$$ = Subassign($1,$3,$6,RIGHTHANDSIDE);
								Removelist($3);
								if($$ == (Symbolhandle) 0)
								{
									YYABORT;
								}
							}

	| expr1 ASSIGNADD expr	{
								Op = ADD;
							  doassignop:
								$$ = Assign($1,(Symbolhandle) 0,-Op);
								if($$ == (Symbolhandle) 0)
								{
									YYABORT;
								}
								$$ = Arith($1, $3, Op);
								UNLOADSEG(Arith);
								if($$ != (Symbolhandle) 0)
								{
									$$ = Assign($1,$$,RIGHTHANDSIDE);
								}
								goto checkResult;
							}
	| expr1 ASSIGNSUB expr	{
								Op = SUB;
								goto doassignop;
							}
	| expr1 ASSIGNMULT expr	{
								Op = MULT;
								goto doassignop;
							}
	| expr1 ASSIGNDIV  expr	{
								Op = DIV ;
								goto doassignop;
							}
	| expr1 ASSIGNMOD expr	{
								Op = MOD;
								goto doassignop;
							}
	| expr1 ASSIGNPOW expr	{
								Op = EXP;
								goto doassignop;
							}


	| '(' expr ')' bracklist assignop
								{ /* Mac: fold 4 cases */
									AssignOp = LONGDATAVALUE($5,1);
									goto assignoperror;
								}
	| functionvalue bracklist assignop
								{
									AssignOp = LONGDATAVALUE($3,1);
									goto assignoperror;
								}
	| byname VAR bracklist assignop
								{
									AssignOp = LONGDATAVALUE($4,1);
									goto assignoperror;
								}
	| VAR bracklist assignop 	{
									AssignOp = LONGDATAVALUE($3,1);
								  assignoperror:
									sprintf(OUTSTR,
											"ERROR: illegal left side of %s",
											opName(AssignOp));
									yyerror(OUTSTR);
									YYABORT;
								}
	;


assignop:  ASSIGNADD	{
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
								$$ = symhOp;
							}
						}
	|		ASSIGNSUB   {Op = SUB;AssignOp = ASSIGNSUB;goto setassignOp;}
	|		ASSIGNMULT	{Op = MULT;AssignOp = ASSIGNMULT;goto setassignOp;}
	|		ASSIGNDIV	{Op = DIV;AssignOp = ASSIGNDIV;goto setassignOp;}
	|		ASSIGNMOD	{Op = MOD;AssignOp = ASSIGNMOD;goto setassignOp;}
	|		ASSIGNPOW	{Op = EXP;AssignOp = ASSIGNPOW;goto setassignOp;}
	;

batch:	BATCH parenlist	{
							$$ = $2;
						}
	| byname BATCH parenlist	{
									$$ = $3;
				 				}
keyword:  compname ASSIGN1 expr 	
							{
								$$ = Assign($1,$3, KEYWORDASSIGN);

								goto checkResult;
							}
	;

/* yylex will never return BLTINTOKEN unless followed by '(' */

functionvalue: BLTINTOKEN parenlist	{
										$$ = runCommand($1, $2);
									  setScrName1:
										OUTSTR[0] = '\0';
										if($$ == (Symbolhandle) 0)
										{
											YYABORT;
										}
										setScratchName($$);
										CHECKINTERRUPT;
									}

	| byname BLTINTOKEN parenlist	{
										$$ = runCommand($1,$3);
										goto setScrName1;
									}
	;

component:	expr '$' compname	{
									$$ = Extract($1,$3,1);
									
								  setScrName2:
									OUTSTR[0] = '\0';
									if($$ == (Symbolhandle) 0)
									{
										YYABORT;
									}
									setScratchName($$);
								}
	| expr brack2list	{
							$$ = Extract($1,$2,0);
							if ($2 != (Symbolhandle) 0 && isscratch(NAME($2)))
							{
								Removesymbol($2);
							}
							goto setScrName2;
						}
	;

transpexpr: expr '\''	{
							Symbolhandle list = Addlist( Makelist(), $1 );
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
							$$ = runCommand(primeH, list);
							goto setScrName1;
				  		}

nullexpr:  '(' ')'	{$$ = NULLSYMBOL;}

expr1:	functionvalue	{$$ = $1;}
	| missingrparen	{
						YYABORT;
					}

	| VAR				{$$ = $1;}
	| byname VAR		{$$ = $1;}
	| nullexpr			{$$ = $1;}

	| expr '+'			{
						  copy1AndCheck:
							BlockBatch();
							$1 = parserCopy($1);
							if ($1 == (Symbolhandle) 0) YYABORT;
						}
			expr
						{
							Op = ADD;
						  arith:
							UnBlockBatch();
							clearNproducts();
							$$ = Arith($1,$4,Op);
							/* Arith() removes SCRATCH unconsumed args */
							UNLOADSEG(Arith);
							goto setScrName1;
						 }
	| expr '-'     			{goto copy1AndCheck;}
			expr
							{ Op = SUB;goto arith; }
	| expr '*'     			{goto copy1AndCheck;}
			expr
							{ Op = MULT;goto arith; }
	| expr MATMULT     		{goto copy1AndCheck;}
			expr
							{ Op = MATMULT;goto arith; }
	| expr TRMATMULT     	{goto copy1AndCheck;}
			expr
							{ Op = TRMATMULT;goto arith; }
	| expr MATMULTTR     	{goto copy1AndCheck;}
			expr
							{ Op = MATMULTTR;goto arith; }
	| expr MATDIV     	{goto copy1AndCheck;}
			expr
							{ Op = MATDIV;goto arith; }
	| expr DIVMAT     	{goto copy1AndCheck;}
			expr
							{ Op = DIVMAT;goto arith; }
	| expr '/'     			{goto copy1AndCheck;}
			expr
							{ Op = DIV;goto arith; }
	| expr MOD     			{goto copy1AndCheck;}
			expr
							{ Op = MOD;goto arith; }
	| expr POW     			{goto copy1AndCheck;}
			expr
							{ Op = EXP;goto arith; }
	| expr BITOR     		{goto copy1AndCheck;}
			expr
							{ Op = BITOR;goto arith; }
	| expr BITXOR     		{goto copy1AndCheck;}
			expr
							{ Op = BITXOR;goto arith; }
	| expr BITAND     		{goto copy1AndCheck;}
			expr
							{ Op = BITAND;goto arith; }
	| '(' expr ')'			{$$ = parserCopy($2);}

	| '-' expr %prec UNARYMINUS	{
									Op = '-';
								  unary:
									$$ = Unary($2,Op);
									/*Unary() reuses space if arg is SCRATCH */
									UNLOADSEG(Unary);
									goto setScrName1;
								}
	| '+' expr %prec UNARYMINUS	{
									Op = '+';
									goto unary;
								}
	| BITNOT expr	 			{
									Op = BITNOT;
									goto unary;
								}
	| expr '='     	{goto copy1AndCheck;}
			expr
					{
						Op = EQ;
					  logic:
						UnBlockBatch();
						clearNproducts();
						$$ = Logic($1,$4,Op);
						/*Logic() removes unconsumed scratch args */
						UNLOADSEG(Logic);
						goto setScrName1;
					}
	| expr EQ      		{goto copy1AndCheck;}
			expr
						{ Op = EQ; goto logic; }
	| expr NE      		{goto copy1AndCheck;}
			expr
						{ Op = NE; goto logic; }
	| expr '<'     		{goto copy1AndCheck;}
			expr
						{ Op = LT; goto logic; }
	| expr LE      		{goto copy1AndCheck;}
			expr
						{ Op = LE; goto logic; }
	| expr '>'     		{goto copy1AndCheck;}
			expr
						{ Op = GT; goto logic; }
	| expr GE      		{goto copy1AndCheck;}
			expr
						{ Op = GE; goto logic; }
	| expr '&'     		{goto copy1AndCheck;}
			expr
						{ Op = AND; goto logic; }
	| expr AND     		{goto copy1AndCheck;}
			expr
						{ Op = AND; goto logic; }
	| expr '|'     		{goto copy1AndCheck;}
			expr
						{ Op = OR; goto logic; }
	| expr OR      		{goto copy1AndCheck;}
			expr
						{ Op = OR; goto logic; }
	| '!' expr			{
							$$ = Logic($2,$2,NOT);
							UNLOADSEG(Unary);
							goto setScrName1;
						}
	| transpexpr		{ $$ = $1; }

/* Macrosetup brackets expanded macro with '{' and '\n }' */
	| expandedmacro	{$$ = $1; goto setScrName2;}
	;

expandedmacro:
	 macro parenlist	{
							long reply = Macrosetup($1,$2);

							Removelist($2);
							if(!reply)
							{
								YYABORT;
							}
						}
		 macrobody  /* Macrosetup() ensures '{', and '}' && header are there */
						{
							$$ = $4;
						}
	;

missingrparen:	'('   ENDOFL	{goto missingRparen; /*Mac: fold 6 cases*/}
	|	'('  expr1  ENDOFL		{goto missingRparen;}
	|	'('  SEMI1				{goto missingRparen;}
	|	'('   ';'				{goto missingRparen;}
	|	'('  expr1 SEMI1		{goto missingRparen;}
	|	'('  expr1  ';'			{
								  missingRparen:
									$$ = 0;
									yyerror("ERROR: missing ')'");
									YYABORT;
								}
	;

expr:	expr1		{$$ = $1;/*Mac: fold 3 cases*/}
	|	component	{$$ = $1;}
	| 	assgn		{$$ = $1;}
	|	ifstart
				{
					IDEPTH--;
					$$ = $1;
				}

	| ifstart ELSE	{
						if(IFCONDITIONS[IDEPTH-1] > 0)
						{
							 $$ = $1;
							 SkipBracketBlock();
						}
						CHECKINTERRUPT;
						if(!Ifsetup((Symbolhandle) 0, ELSE))
						{
							$$ = 0;
							YYABORT;
						}
					}
		cmpdstatement
					{
						if(IFCONDITIONS[IDEPTH-1] == 0)
						{
							 $$ = $4;
						}
						IDEPTH--;
					}

	| WHILE parenlist	{
							int    ifOK;

							CHECKINTERRUPT;
							ifOK = Ifsetup($2, WHILE);
							Removelist($2);
							if(!ifOK)
							{
								$$ = 0;
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
						}
			cmpdstatement
						{ /* post WHILE checking */
							if(WHILECONDITIONS[WDEPTH-1])
							{
								setupfaketokens(WHILE);
								if($4 != (Symbolhandle) 0 && BRACKETLEV > 0 &&
									 isscratch(NAME($4)))
								{
									Removesymbol($4);
									$4 = NULLSYMBOL;
								}
							}
							else
							{
								WHILELIMITS[WDEPTH-1] = MAXWHILE;
							}
							WDEPTH--;
							$$ = $4;
						}

	| FOR parenlist		{
							int   ifOK;

							CHECKINTERRUPT;
							ifOK = Ifsetup($2, FOR);
							Removelist($2);
							if(!ifOK)
							{
								$$ = 0;
								YYABORT;
							}
							if (ifOK < 0)
							{
								SkipBracketBlock();
							}
						 }
			cmpdstatement
						{
							if(WHILECONDITIONS[WDEPTH-1] &&
								 --WHILELIMITS[WDEPTH-1] > 0)
							{
								setupfaketokens(FOR);
								if($4 != (Symbolhandle) 0 && BRACKETLEV > 0 &&
									 isscratch(NAME($4)))
								{
									Removesymbol($4);
									$4 = NULLSYMBOL;
								}
							}
							else
							{
								WHILELIMITS[WDEPTH-1] = MAXWHILE;
							}
							WDEPTH--;
							$$ = $4;
						}

	| cmpdstatement	{$$ = $1;/*Mac: fold 3 cases*/}

	| macrobody		{$$ = $1;}

	| subscripted	{$$ = $1;}

	| VAR  '('		{
						notfunction($1);
						$$ = 0;
						YYABORT;
					}

	| FATALERROR	{
						return (FATALERROR);
					}
	;


badbatch: '-' batch %prec UNARYMINUS	{$$ = $2;/*Mac: fold 4 cases*/}
	| '+' batch %prec UNARYMINUS	{$$ = $2;}
	| '!' batch			{$$ = $2;}
	| '(' batch			{$$ = $2;}
	| batch '+'			{$$ = $1;/*Mac: fold 26 cases*/}
	| batch '-'			{$$ = $1;}
	| batch '*'			{$$ = $1;}
	| batch MATMULT		{$$ = $1;}
	| batch TRMATMULT	{$$ = $1;}
	| batch MATMULTTR	{$$ = $1;}
	| batch MATDIV		{$$ = $1;}
	| batch DIVMAT		{$$ = $1;}
	| batch '/'			{$$ = $1;}
	| batch MOD			{$$ = $1;}
	| batch POW			{$$ = $1;}
	| batch EQ 			{$$ = $1;}
	| batch NE 			{$$ = $1;}
	| batch '<'			{$$ = $1;}
	| batch LE 			{$$ = $1;}
	| batch '>'			{$$ = $1;}
	| batch GE 			{$$ = $1;}
	| batch '&'			{$$ = $1;}
	| batch AND			{$$ = $1;}
	| batch '|'			{$$ = $1;}
	| batch OR 			{$$ = $1;}
	| batch '('			{$$ = $1;}
	| batch '['			{$$ = $1;}
	| batch '$'			{$$ = $1;}
	| batch RIGHTBRACK2	{$$ = $1;}
	| batch ASSIGN		{$$ = $1;}
	;

subscripted: cmpdstatement bracklist	{
									$$ = Element($1,$2);
									Removelist($2);
									goto setScrName2;
								}
	| '(' expr ')' bracklist	{
								  bracklist4:
									$$ = Element($2,$4);
									Removelist($4);
									goto setScrName2;
								}
	| functionvalue bracklist	{ goto bracklist2; /*Mac: fold 6 cases */}
	| component bracklist 		{ goto bracklist2; }
	| expandedmacro  bracklist	{ goto bracklist2; }
	| macrobody bracklist		{ goto bracklist2; }
	| VAR  bracklist 			{ goto bracklist2; }
	| transpexpr bracklist		{ goto bracklist2; }
	| subscripted bracklist	{
							  bracklist2:
								$$ = Element($1,$2);
								Removelist($2);
								goto setScrName2;
							}
	| byname VAR bracklist 	{
								$$ = Element($1,$3);
								Removelist($3);
								goto setScrName2;
							}
	;
		
ifstart: IF parenlist	{
							int    ifOK;

							CHECKINTERRUPT;

							ifOK = Ifsetup($2, IF);
							Removelist($2);
							if(!ifOK)
							{
								$$ = 0;
								YYABORT;
							}
							if(IFCONDITIONS[IDEPTH-1] == 0)
							{
								SkipBracketBlock();
							}
						}
		 cmpdstatement
		 				{ /* post IF checking */
							$$ = (IFCONDITIONS[IDEPTH-1] ) ? $4 : NULLSYMBOL;
						}
	|	ifstart ELSEIF 
						{
							if(IFCONDITIONS[IDEPTH-1] > 0)
							{
								SkipParenBlock();
							}
						}
						parenlist
						{
							int    ifOK;

							if(IFCONDITIONS[IDEPTH-1] > 0)
							{
								SkipBracketBlock();
								Removelist($4);
								$4 = (Symbolhandle) 0;
							}
							CHECKINTERRUPT;

							ifOK = Ifsetup($4, ELSEIF);

							if($4 != (Symbolhandle) 0)
							{
								Removelist($4);
							}
							if(!ifOK)
							{
								$$ = 0;
								YYABORT;
							}
							if(IFCONDITIONS[IDEPTH-1] == 0)
							{
								SkipBracketBlock();
							}
						}
		 cmpdstatement
		 				{
							if($4 == (Symbolhandle) 0)
							{
								$$ = $1;
							}
							else if(IFCONDITIONS[IDEPTH-1] > 0)
							{
								$$ = $6;
							}
							else
							{
								$$ = NULLSYMBOL;
							}
						}
	| ifstart ERROR		{ $$ = 0 ; YYABORT;}
		 ;

macro:	MACROTOKEN	{$$ = $1;}
	|   byname MACROTOKEN	{
								$$ = $1;
								PARSEMACROARGS = ParseMacroArgs;
							}
	;


/* WARNING: Do not fold next 2 cases */
compname:	VAR		{$$ = $1;}
	|	byname VAR	{$$ = $1;}
	;

byname:	LEFTANGLE expr RIGHTANGLE
					{
						$$ = Byname($2,&BynameType,&ParseMacroArgs);

						if($$ == (Symbolhandle) 0)
						{
							YYABORT;
						}
						else
						{
							SetNextChar(BynameType);
						}
					}
	|	LEFTANGLE expr {
							$$ = 0;
							yyerror("ERROR: missing '>>'");
							YYABORT;
						}
	;

/* note that a 'list' always terminates with a comma */
list:	 /* nothing */ ','	{
							  growMake:
								$$ = Growlist(Makelist());
								goto checkResult;
							}
	| expr ','	 			{goto growAddMake;/*Mac: fold 2 cases*/}
	| keyword ',' 			{
							  growAddMake:
								$$ = Growlist(Addlist(Makelist(),$1));
								goto checkResult;
							}
	| list expr ','			{goto growAdd; /*Mac: fold 2 cases*/}
	| list keyword ','		{
							  growAdd:
								$$ = Growlist(Addlist($1,$2));
								goto checkResult;
							}
	| list ',' 				{
								$$ = Growlist($1);
								goto checkResult;
							}
	;


completelist:  expr 		{goto addMake; /*Mac: fold 2 cases*/}
	|  keyword 				{
							  addMake:
								$$ = Addlist( Makelist(), $1 );
								goto checkResult;
							}
	|  list 				{
								$$ = $1;
								goto checkResult;
							}
	|  list expr 			{goto addlist;/*Mac: fold 2 cases*/}
	|  list keyword			{
							  addlist:
								$$ = Addlist( $1, $2);
								goto checkResult;
							}
	;

parenlist:	'(' /* nothing */ ')'	{
									  makelist:
										$$ = Makelist();
										goto checkResult;
									}
	|	'(' completelist ')'	{
									$$ = $2;
								}
	|	'('	completelist ENDOFL	{goto missingRparen; /*Mac: fold 6 cases*/}
	|	'('	completelist SEMI1	{goto missingRparen;}
	|	'('	completelist ';'	{goto missingRparen;}
	|	'('	ENDOFL				{goto missingRparen;}
	|	'('	SEMI1				{goto missingRparen;}
	|	'('	';'					{goto missingRparen;}
	;

bracklist:   '[' /* nothing */ ']'
				  	{goto makelist;/*Mac: fold 2 cases*/}
	|   '[' RIGHTBRACK2
				  	{goto makelist;}
	|	'[' completelist ']'
				  	{ $$ = $2; goto checkResult;/*Mac: fold 2 cases*/}
	|	'[' completelist RIGHTBRACK2
				  	{ $$ = $2; goto checkResult;}
	|	'['  SEMI1	{goto missingRbracket;/*Mac: fold 3 cases*/}
	|	'['  ';'  	{goto missingRbracket;}
	|	'['  ENDOFL {
					  missingRbracket:
						yyerror("ERROR: missing ']'");
						$$ = 0;
						YYABORT;
					}
					ERROR
	;
/*
	LEFTBRACK2 & RIGHTBRACK2 are '[' & ']' but signal that the next character
	is the same.  A subscript list can never start with LEFTBRACK2
*/
brack2list:   LEFTBRACK2 '[' expr RIGHTBRACK2 ']'
				  	{ $$ = $3; goto checkResult;/*Mac: fold 2 cases*/}
	|	LEFTBRACK2 '[' expr RIGHTBRACK2 RIGHTBRACK2
				  	{ $$ = $3; goto checkResult;}
	|	LEFTBRACK2 '[' RIGHTBRACK2 ']'
					{ $$ = (Symbolhandle) 0; }
	|	LEFTBRACK2 '[' SEMI1	{goto missingRbrack2; /*Mac: fold 3 cases*/}
	|	LEFTBRACK2 '[' ';'		{goto missingRbrack2;}
	|	LEFTBRACK2 '[' ENDOFL	{
								  missingRbrack2:
									yyerror("ERROR: missing ']]'");
									$$ = 0;
									YYABORT;
								}
	;

%%
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
		yyerror("syntax error");
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


