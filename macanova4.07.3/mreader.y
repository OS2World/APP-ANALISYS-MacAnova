%{
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
#pragma segment Glm
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


/*
  model parser written in yacc
  Version of 940224
  yacc message
	conflicts: 7 shift/reduce
  930426 Changed yylval type to Symbolhandle, allowing retrieval of space
  in case of interrupt.
  940224 Added checks to ensure that an error term is not deleted and that an
  error term is not constant

  950510 Changed '1 << var' to '1L << var' in MakeTerm() to make fix in
  Borland DOS version for which int means short.

  960709 Added model syntax "M^N" where M is a submodel and N is a 
  positive integer.  Also corrected long standing bug in unique().

  970818 Added {...} formulae for r.h.s. variables, that is models
  like "y = x + {x^2} + {x^3}" will work.

  970822 Changed initmodelparse so that it duplicates the model and returns
  0 if it is unable to do so.
  Modified modelyylex to recognized and expand expressions of the form Pn(expr)

  970823 Changed modelyylex() to recognized and expand expressions of the
  form Cn(expr)

  971014 Fixed bug in modelyylex() so that it now gives error message when
  dependent variable expression {expr} appears on r.h.s. of models

  971124 Fixed modelyylex() so that, say, manova("{y[,run(3)]}=groups") is
  legal, {...} can be a matrix if it's to the left of '='

  971126 Modified calls to appendLabels to use codes in labels.h

  980116 Fixed memory allocation bug in insertForm()

  980120 Added check for recursive use by adding separate entry modelparse
         which calls yyparse().
         Added define #yyparse yyparse_mreader
         Concurrently modified mreader.sed, the sed script that modifies the
         yacc output.

  990207 Changed call to mymultiprint() to include new 3rd argument

  990212 Changed putOUTSTR() to putErrorOUTSTR()
*/

#include "globals.h"

#undef BADTOKEN

#define YYSTYPE Symbolhandle
#define YYDEBUG

typedef YYSTYPE      Model;

/*
  As of 930517, all the code depends on the representation of a model only
  through the following macros.  If a new representation is chosen
  only these will need to be changed
*/

#define RemoveModel(M)       Removesymbol(M)

#define GetModel(A) (modelHandle) LONGDATA((Symbolhandle) A)
#define SetModel(A,B) setLONGDATA((Symbolhandle) A,(long **) (B))

/* in following, A is a modelHandle, B is a modelPointer, I and L are longs */
/* in mreader, term numbers run from 1 to nterms */
#define GetLength(A) DIMVAL((Symbolhandle) A,2)
#define SetLength(A,L) setDIM((Symbolhandle) A,2,L)
#define TermI(A,I) modelterm(*(A),(I) - 1)  /* value is modelPointer */
#define SetTermI(A,I,B) modeltermAssign(B,(*(A) + (I) - 1)) /*(*A)[I-1] = B*/
#define ModelSize(L) ((L) * sizeof(modelType))
#define SetModelDims(A,L)	{\
								setNDIMS((Symbolhandle) A,2);\
								setDIM((Symbolhandle) A,1,WORDSPERTERM);\
								SetLength(A,L);\
							}

/* in following, A and B are modelPointers's & I is a variable number */
#define EqualTerms(A,B) modeltermEqual(A,B)
#define ContainTerm(A,B) inEffectiveTerm(A,B)
#define SetErrorTerm(A) modeltermAssign(A,(modelPointer) Info->errorterms[Info->nerrorterms-1])
#define GetErrorTerm(I) ((modelPointer) Info->errorterms[(I)-1])

#define yylex yylex_mreader
#define yyerror yyerror_mreader
#define yyparse yyparse_mreader

#define   TAB       '\t'
#define   NL        10
#define   CR        13
#define   SP        ' '

#define   MAXPOWER   MAXDIMS /* maximum value for p in term^p */

#define Info        MreaderModelInfoPointer

modelInfoPointer    Info;

/* handle of model being scanned; initialized initmodel()*/
static char       **Strmodel = (char **) 0;
/* temporary handle created and used by insertForm */
static char       **NewStrmodel = (char **) 0;
static long         Imodchar; /* position in the model */
static int          ModelPower;
static char        *MODELPARSERUNNING = (char *) 0;
static int yyparse(void); /* prototype */
%}

%token MODELVAR MODELEND MODELPOWER BADPOWER BADTOKEN
%left '+' '-'
%left '/'
%left '*'
%left '.'
%left '^'
%%
assgn: 	MODELVAR '=' model MODELEND	{
										$$ = unique($3, 1);
										Info->nterms = GetLength($$);
							
										Info->model = GetModel($$);
										SetModel($$,0);
										RemoveModel($$);

										if (Info->nterms == 0)
										{
											yyerror("ERROR: No terms in model");
											mydisphandle((char **) Info->model);
											Info->model = (modelHandle) 0;
										}
										else
										{
											/* shrink to correct size */
											Info->model = (modelHandle)
											  mygrowhandle((char **) Info->model,
														   ModelSize(Info->nterms));
										}
										mydisphandle(Strmodel);
										Strmodel = (char **) 0;
										return(Info->model == (modelHandle) 0);
									}
	| lexerror				{
							  errorExit:
								mydisphandle(Strmodel);
								Strmodel = (char **) 0;
								return (1);
							}
	| error					{ goto errorExit; }
	;

model: 	  model '+' model	{
								$$ = mjoin($1,$3);
							  checkit:
								if($$ == (Model) 0)
								{
									goto errorExit;
								}
							}
	| model '.' model		{ $$ = mdot($1,$3); goto checkit; }
	| model '*' model		{ $$ = mstar($1,$3); goto checkit; }
	| model '/' model		{ $$ = mnest($1,$3); goto checkit; }
	| '(' model ')'			{ $$ = $2; goto checkit; }
	| model '-' model		{ $$ = mminus($1,$3); goto checkit; }
	| model '-' '*' model	{ $$ = mmstar($1,$4); goto checkit; }
	| model '^' MODELPOWER	{ $$ = mpower($1, ModelPower); goto checkit;}
	| model '^' BADPOWER	{
								sprintf(OUTSTR,
										"ERROR: illegal power < 1 or > %ld in model",
										(long) MAXPOWER);
								yyerror(OUTSTR);
								*OUTSTR = '\0';
								goto errorExit;
							}
	| 'E' '(' term ')'		{ $$ = madder($3); goto checkit; }
	| MODELVAR				{ $$ = $1 ; goto checkit; }
	| model lexerror		{ goto errorExit; }
	| lexerror				{ goto errorExit; }
	;


term:	  MODELVAR			{ $$ = $1; goto checkit; }
	| MODELVAR '.' term		{ $$ = mdot($1,$3); goto checkit; }
	| term lexerror			{ goto errorExit; }
	| lexerror				{ goto errorExit; }
	;

lexerror: BADTOKEN  		{ goto errorExit; }
	;
%%
/* start of private functions */
static void yyerror(char *s)
{
	/*  output error message */
	char            c;
	WHERE("yyerror");

	if(strcmp(s,"syntax error") == 0)
	{
		myprint("ERROR: Model syntax error");
	}
	else
	{
		myprint(s);
	}

	myprint(" near ");
	c = (*Strmodel)[Imodchar];
	(*Strmodel)[Imodchar] = '\0';
	mymultiprint(*Strmodel, STDOUT, 1);
	(*Strmodel)[Imodchar] = c;
} /*yyerror()*/

static Model InstallModel(long nterms)
{
	Model             modelH = (Model) Install(SCRATCH,LONG);
	modelHandle       model;

	if(modelH != (Model) 0)
	{ /* will be structured as WORDSPERTERM by nterms matrix */
		model = (modelHandle) mygethandle(ModelSize(nterms));
		SetModel(modelH,model);
		if(model != (modelHandle) 0)
		{
			SetModelDims(modelH,nterms);
		}
		else
		{
			RemoveModel(modelH);
			modelH = (Model) 0;
		}
	} /*if(modelH != (Model) 0)*/
	return (modelH);
} /*InstallModel()*/

static modelPointer MakeTerm(long var)
{
	static modelType     term;
	long                 i;

	zeroTerm(term);

	if(var != 0)
	{
		i = (var-1) / VARSPERWORD;
		var = (var - 1) % VARSPERWORD;
		term[i] = (termWord) (1L << var);
	} /*if(var != 0)*/
	return ((modelPointer) term);
} /*MakeTerm()*/

static modelPointer   OrTerms(modelPointer termA, modelPointer termB)
{
	static modelType     termC;
	long                 i;

	for(i=0;i<WORDSPERTERM;i++)
	{
		termC[i] = (termWord) ((*termA)[i] | (*termB)[i]);
	} /*for(i=0;i<WORDSPERTERM;i++)*/
	return ((modelPointer) termC);
} /*OrTerms()*/

#ifdef UNDEFINED__  /* currently not used */
static modelPointer   AndTerms(modelPointer termA, modelPointer termB)
{
	static modelType     termC;
	long                 i;

	for(i=0;i<WORDSPERTERM;i++)
	{
		termC[i] = (termWord) ((*termA)[i] & (*termB)[i]);
	} /*for(i=0;i<WORDSPERTERM;i++)*/
	return ((modelPointer) termC);
} /*AndTerms()*/
#endif /*UNDEFINED__*/

static void modelMerge(Model modela, modelPointer pMerge)
{
	modelHandle    a = GetModel(modela);
	long           alength = GetLength(modela);
	long           iterm;

	zeroTerm(pMerge);
	
	for (iterm = 1; iterm <= alength; iterm++)
	{
		modeltermAssign(OrTerms(TermI(a,iterm), pMerge), pMerge);
	} /*for(iterm = 1; iterm <= alength; iterm++)*/
} /*modelMerge*/

static long nvInModel(Model modela)
{
	modelHandle    a = GetModel(modela);
	modelType      merge;
	modelPointer   pMerge = (modelPointer) merge;
	long           alength = GetLength(modela);

	modelMerge(modela, pMerge);

	return (nvInTerm(pMerge));
	
} /*nvInModel()*/

static Model unique(Model modela, long start)
{
	/*  remove repeated model terms from list */
	modelHandle    a = GetModel(modela);
	long           alength = GetLength(modela);
	long           i,j,k;
	WHERE("unique");

	for(i=start;i < alength; i++)
	{
		for(j=i+1;j<=alength;j++)
		{
			while (j <= alength && EqualTerms(TermI(a,i),TermI(a,j)))
			{ /* a repeat */
				for(k=j+1;k<=alength;k++)
				{
					SetTermI(a,k-1,TermI(a,k));
				} /*for(k=j+1;k<=alength;k++)*/
				alength--;
			} /*while (j <= alength && EqualTerms(TermI(a,i),TermI(a,j)))*/
		} /*for(j=i+1;j<=alength;j++)*/
	} /*for(i=1;i< alength; i++)*/
 	SetLength(modela,alength);

	return (modela);
} /*unique()*/

/*
  Routine to join (add) two models together
  preserve left precedence
*/

static Model mjoin(Model modela, Model modelb)
{
	modelHandle    a = GetModel(modela), b = GetModel(modelb), c;
	long           alength = GetLength(modela), blength = GetLength(modelb);
	long           clength = alength + blength;
	Model          modelc = InstallModel(clength);
	long           i, j, k;
	WHERE("mjoin");

	if(modelc != (Model) 0)
	{
		c = GetModel(modelc);
		k = 1;
		for(i = 1; i <= alength; i++)
		{
			SetTermI(c,k,TermI(a,i));
			k++;
		} /*for(i = 1; i <= alength; i++)*/
		for(j = 1; j <= blength; j++)
		{
			SetTermI(c,k,TermI(b,j));
			k++;
		} /*for(j = 1; j <= blength; j++)*/
	} /*if(modelc != (Model) 0)*/
	RemoveModel(modela);
	RemoveModel(modelb);

	return(modelc);
} /*mjoin()*/

/*
  Routine to nest two models together
  c = a/b
 */

static Model  mnest(Model modela, Model modelb)
{
	modelHandle     a = GetModel(modela), b = GetModel(modelb), c;
	long            alength = GetLength(modela), blength = GetLength(modelb);
	long            clength = alength + blength;
	Model           modelc = InstallModel(clength);
	modelType       merge;
	modelPointer    pMerge = (modelPointer) merge;
	long            i, j, k;

	if(modelc != (Model) 0)
	{
		modelMerge(modela, pMerge);
		c = GetModel(modelc);
		k = 1;
		for(i = 1; i <= alength; i++)
		{
			SetTermI(c,k,TermI(a,i));
			k++;
		} /*for(i = 1; i <= alength; i++)*/

		for(j = 1; j <= blength; j++)
		{
			SetTermI(c,k,OrTerms(TermI(b,j),pMerge));
			k++;
		} /*for(j = 1; j <= blength; j++)*/
	} /*if(modelc != (Model) 0)*/
	RemoveModel(modela);
	RemoveModel(modelb);

	return(modelc);
} /*mnest()*/

/*
  c = a.b
*/
static Model mdot(Model modela, Model modelb)
{
	/*  routine to dot a model with a model */
	modelHandle  a = GetModel(modela), b = GetModel(modelb), c;
	long         alength = GetLength(modela), blength = GetLength(modelb);
	long         clength = alength * blength;
	Model        modelc = InstallModel(clength);
	long         i, j, k;

	if(modelc != (Model) 0)
	{
		c = GetModel(modelc);
		k = 1;
		for(i = 1; i <= blength; i++)
		{
			for(j = 1; j <= alength; j++)
			{
				SetTermI(c,k,OrTerms(TermI(a,j),TermI(b,i)));
				k++;
			} /*for(j = 1; j <= alength; j++)*/
		} /*for(i = 1; i <= blength; i++)*/
	} /*if(modelc != (Model) 0)*/
	RemoveModel(modela);
	RemoveModel(modelb);

	return(modelc);
} /*mdot()*/

/*  c = a*b = a + b + a.b   a la GLIM  */

static Model mstar(Model modela, Model modelb)
{
	modelHandle   a = GetModel(modela), b = GetModel(modelb), c;
	long          alength = GetLength(modela), blength = GetLength(modelb);
	long          clength = alength + blength + alength * blength;
	Model         modelc = InstallModel(clength);
	long          i, j, k;

	if(modelc != (Model) 0)
	{
		c = GetModel(modelc);
		k = 1;
		for(i = 1; i <= alength; i++)
		{
			SetTermI(c,k, TermI(a,i));
			k++;
		} /*for(i = 1; i <= alength; i++)*/
		for(i = 1; i <= blength; i++)
		{
			SetTermI(c,k, TermI(b,i));
			k++;
		} /*for(i = 1; i <= blength; i++)*/
		for(i = 1; i <= blength; i++)
		{
			for(j = 1; j <= alength; j++)
			{
				SetTermI(c,k, OrTerms(TermI(a,j),TermI(b,i)));
				k++;
			}
		} /*for(i = 1; i <= blength; i++)*/
	} /*if(modelc != (Model) 0)*/
	RemoveModel(modela);
	RemoveModel(modelb);

	return(modelc);
} /*mstar()*/

/* remove from a terms containing terms in b: c = a -* b*/

static Model mmstar(Model modela, Model modelb)
{
	modelHandle    a = GetModel(modela), b = GetModel(modelb), c = a;
	Model          modelc = modela;
	modelPointer   termbj, termci;
	long           blength = GetLength(modelb), clength = GetLength(modelc);
	long           i, j, k, deleted = 0;

	for(j=1; j <= blength; j++)
	{
		termbj = TermI(b,j);
		for(i = 1; i <= clength; i++)
		{
			while(i <= clength && ContainTerm(termci = TermI(c,i),termbj))
			{ /* drop out i-th term of c */
				for(k=1;k<Info->nerrorterms;k++)
				{ /* check to see whether it is error term */
					if(EqualTerms(GetErrorTerm(k), termci))
					{
						yyerror("ERROR: attempt to remove error term from model");
						return ( (Model) 0);
					} /*if(EqualTerms(GetErrorTerm(k), termci))*/
				} /*for(k=1;k<Info->nerrorterms;k++)*/

				for( k = i; k < clength ; k++)
				{ /* squeeze term out of model */
					SetTermI(c,k,TermI(c,k+1));
					deleted++;
				} /*for( k = i; k < clength ; k++)*/
				clength--;
			}/*while(i <= clength && ContainTerm(termci = TermI(c,i),termbj))*/
		} /*for(i = 1; i <= clength; i++)*/
	} /*for(j=1; j <= blength; j++)*/
	SetLength(modelc,clength);
	RemoveModel(modelb);
	if(deleted == 0)
	{
		yyerror("ERROR: attempt to subtract term(s) not in the model");
		return ((Model) 0);
	} /*if(deleted == 0)*/

	return(modelc);
} /*mmstar()*/

/* remove terms in b from a: c = a - b */
static Model mminus(Model modela, Model modelb)
{
	modelHandle  a = GetModel(modela), b = GetModel(modelb), c = a;
	Model        modelc = modela;
	modelPointer termbj;
	long         blength = GetLength(modelb), clength = GetLength(modelc);
	long         i, j, k, deleted = 0;

	for(j=1; j <= blength; j++)
	{
		termbj = TermI(b,j);
		for(k=1;k<Info->nerrorterms;k++)
		{
			if(EqualTerms(GetErrorTerm(k), termbj))
			{
				yyerror("ERROR: attempt to remove error term from model");
				return ( (Model) 0);
			}
		} /*for(k=1;k<Info->nerrorterms;k++)*/
		
		for(i = 1; i <= clength; i++)
		{
			while( EqualTerms(TermI(c,i),termbj)  && i <= clength )
			{ /* squeeze out term i from c */
				for( k = i; k < clength ; k++)
				{
					SetTermI(c,k,TermI(c,k+1));
				}
				deleted++;
				clength--;
			} /*while( EqualTerms(TermI(c,i),termbj)  && i <= clength )*/
		} /*for(i = 1; i <= clength; i++)*/
	} /*for(j=1; j <= blength; j++)*/
	if(deleted == 0)
	{
		yyerror("ERROR: attempt to subtract term(s) not in the model");
		return ((Model) 0);
	} /*if(deleted == 0)*/
	SetLength(modelc,clength);
	RemoveModel(modelb);

	return(modelc);
} /*mminus()*/

/*
   Equivalent to modela.(1+modela)...(1+modela)  [p factors]
*/
static Model mpower(Model modela, int p)
{
	modelHandle   a = GetModel(modela), c;
	long          alength = GetLength(modela), clength = 1;
	long          needed = alength;
	long          nvars = nvInModel(modela);
	Model         modelc;
	int           i, j, k, l;
	WHERE("mpower");
	
	if (p > 1)
	{
		p = (p > nvars) ? nvars : p;
		for (i = 1; i < p; i++)
		{
			if (isTooBig(alength + 1, needed, ModelSize(1)))
			{
				putOutErrorMsg("ERROR: insufficient memory to expand model");
				return ((Model) 0);
			}
			needed *= (alength + 1);
		} /*for (i = 1; i < p; i++)*/

		modelc = InstallModel(needed);
		if(modelc != (Model) 0)
		{
			c = GetModel(modelc);
			k = 0;
			for (j = 1; j <= alength; j++)
			{
				k++;
				SetTermI(c, k, TermI(a,j));
			} /*for (j = 1; j <= alength; j++)*/
			SetLength(modelc, k);

			clength = 0;
			for (l = 1; l < p; l++)
			{
				/* delete duplicates as we go */
				modelc = unique(modelc, clength+1);
				c = GetModel(modelc);
				clength = GetLength(modelc);
				k = clength;
				for (i = 1; i <= alength; i++)
				{
					for (j = 1; j <= clength; j++)
					{
						k++;
						SetTermI(c,k,OrTerms(TermI(a,i),TermI(c,j)));
					} /*for (j = 1; j <= clength; j++)*/
				} /*for (i = 1; i <= alength; i++)*/
				SetLength(modelc, k);
			} /*for (l = 1; l < p; l++)*/
		} /*if(modelc != (Model) 0)*/
		RemoveModel(modela);
	} /*if (p > 1)*/
	else
	{
		modelc = modela;
	}
	
	return (modelc);
} /*mpower()*/

static Model madder(Model modela)
{
	/* add term to list of error terms */
	modelHandle   a = GetModel(modela);
	modelPointer  term = TermI(a,1);
	long          k;
	
	if(EqualTerms(term, (modelPointer) NULLTERM))
	{ /* E(1) is illegal */
		yyerror("ERROR: error term is CONSTANT");
		return ( (Model) 0);
	} /*if(EqualTerms(term, (modelPointer) NULLTERM))*/

	for(k=1;k<Info->nerrorterms;k++)
	{ /* make sure term isn't already an error term */
		if(EqualTerms(term, GetErrorTerm(k)))
		{
			yyerror("ERROR: duplicate error term in model");
			return ( (Model) 0);
		} /*if(EqualTerms(term, GetErrorTerm(k)))*/
	} /*for(k=1;k<Info->nerrorterms;k++)*/
	
	SetErrorTerm(TermI(a,1));
	Info->nerrorterms++;

	return (modela);
} /*madder()*/

/*
   called by yylex() for '1' (var == 0) or variable name (var == varno+1).
   Create corresponding new model with one term
*/
static long setyylval(long var)
{
	if((yylval = InstallModel(1)) == (Model) 0)
	{
		return (BADTOKEN);
	} /*if((yylval = InstallModel(1)) == (Model) 0)*/
	SetTermI(GetModel(yylval),1,MakeTerm(var));

	return (MODELVAR);
} /*setyylval()*/

/*
  op == 'P'
  Expand expression of the form Pn(expr) as
    ({expr}+{(expr)^2}+...+{(expr)^order})
  and insert it in Strmodel inplace of Pn(expr)
  (*Strmodel)[place] is the first non white character in expr
  Thus, for example, if *Strmodel is "y = x + P3(log(y)) + w", is will become
  "y = x + ({log(y)}+{(log(y))^2}+{(log(y))^3}) + w"

  op == 'C'
  Expand expression of the form Cn(expr) as
    ({cos(expr)}+{sin(expr)}+{cos(2*(expr))}+{sin(2*(expr))}+ ... +
      {cos(order*(expr))}+{sin(order*(expr))})

  At entry, (*Strmodel)[Imodchar] should be 'C' or 'P' and hence is
  the number of characters before them.

  (*Strmodel)[place] should be first character after '('

  970822 Installed

  980115 corrected bug in allocating space for expanded Pn(expr) and Cn(expr)
         Dropped argument place since it can be found from Imodchar
*/
static int insertForm(long order, int op)
{
	char     *inputString = *Strmodel + Imodchar;
	char     *newInputString;
	long      place, endplace;
	long      nparen = 1;
	long      tailLength;
	long      k, length, needed;
	char     *chOp = (op == 'P') ? "Pn" : "Cn";
	char     *cosName = "cos", *sinName = "sin";
	WHERE("insertForm");

	/* move to just after '('; we know it's there */
	while (*inputString++ != '(')
	{
		;
	}
	
	/* skip leading white space */
	while (isspace(*inputString))
	{
		inputString++;
	}
	/*
	  inputString[0] should be first non-blank character after '('
	  (*Strmodel)[place] is first character of expression in (...)
	*/
	place = endplace = inputString - *Strmodel;
	(void) findBracket(0, '(', (unsigned char *) *Strmodel,
					   &nparen, &endplace, (long *) 0);

	if (nparen > 0)
	{
		sprintf(OUTSTR, "ERROR: missing ')' in %s(...) term in model", chOp);
		return (0);
	}
	/* 
	   (*Strmodel)[endplace] should be first character after ')'
	*/

	tailLength = strlen(*Strmodel + endplace);
	length = endplace - 1 - place;
	while (length > 0 && isspace(inputString[length-1]))
	{ /*trim off white space before ')'*/
		length--;
	}
	/* 
	   length should now be the length of expression, shorn of any leading
	   or trailing white space
	*/
	if (length == 0)
	{
		sprintf(OUTSTR,
				"ERROR: no variable specified for %s(...) term in model",
				chOp);
		return (0);
	}

	/*
	  op == 'P'
	 The expanded expression will look like of
	    ({expr}+{(expr)^2}+{(expr)^3}+...+{(expr)^n})
	  In the following, 5 counts '+', '{', '}', '^', + digit

	  op == 'C'
      The expanded expression will look like
        ({cos(expr)}+{sin(expr)}+{cos(2*(expr))}+{sin(2*(expr))}+ ... +
           {cos(order*(expr))}+{sin(order*(expr))})

	  (*Strmodel)[Imodchar] should be the digit after 'P' or 'C'
	  Thus Imodchar - 1 is the number of characters up to but not including
	  'P' or 'C'

	  Initial value for needed includes 3 for '(' and ') and '\0', but
	  but not what's needed between them
	*/
	
	needed = Imodchar - 1 + tailLength + 3;

	if (op == 'P')
	{
		/* 2 is for '{' and '}', 5 is for '(', ')', '^', digit, '+'*/
		needed += order*(length + 2) + (order - 1)*5;
	}
	else
	{ /* 7 is for '{', 'cos' or 'sin', '(', ')', '}'; 5 is for digit, '*', '(', ')', '+' */
		needed += 2*(order*(length + 7) + (order - 1)*5);
	}

	if (order > 9)
	{ /* room for 2nd digits, if any */
		needed += (op == 'P') ? order - 9 : 2*(order - 9);
	}

	NewStrmodel = mygethandle(needed);

	if (NewStrmodel == (char **) 0)
	{
		return (0);
	}
	Imodchar--;
	inputString = *Strmodel + place;
	newInputString = *NewStrmodel;
	strncpy(newInputString, *Strmodel, Imodchar);
	newInputString += Imodchar;
	*newInputString++ = '(';

	if (op == 'P')
	{
		*newInputString++ = '{';
		strncpy(newInputString, inputString, length);
		newInputString += length;
		*newInputString++ = '}';
		for (k = 2; k <= order; k++)
		{
			*newInputString++ = '+';
			*newInputString++ = '{';
			*newInputString++ = '(';
			strncpy(newInputString,inputString, length);
			newInputString += length;
			*newInputString++ = ')';
			*newInputString++ = '^';
			if (k >= 10)
			{
				*newInputString++ = (char) (k/10 + '0');
			}
			*newInputString++ = (char) (k%10 + '0');
			*newInputString++ = '}';
		} /*for (k = 2; k <= order; k++)*/
	} /*if (op == 'P')*/
	else
	{
		*newInputString++ = '{';
		strcpy(newInputString, cosName);
		newInputString += 3;
		*newInputString++ = '(';
		strncpy(newInputString, inputString, length);
		newInputString += length;
		*newInputString++ = ')';
		*newInputString++ = '}';
		*newInputString++ = '+';
		*newInputString++ = '{';
		strcpy(newInputString, sinName);
		newInputString += 3;
		*newInputString++ = '(';
		strncpy(newInputString, inputString, length);
		newInputString += length;
		*newInputString++ = ')';
		*newInputString++ = '}';
		for (k = 2; k <= order; k++)
		{
			*newInputString++ = '+';
			*newInputString++ = '{';
			strcpy(newInputString, cosName);
			newInputString += 3;
			*newInputString++ = '(';
			if (k >= 10)
			{
				*newInputString++ = '0' + k/10;
			}
			*newInputString++ = '0' + k % 10;
			*newInputString++ = '*';
			*newInputString++ = '(';
			strncpy(newInputString, inputString, length);
			newInputString += length;
			*newInputString++ = ')';
			*newInputString++ = ')';
			*newInputString++ = '}';
			*newInputString++ = '+';
			*newInputString++ = '{';
			strcpy(newInputString, sinName);
			newInputString += 3;
			*newInputString++ = '(';
			if (k >= 10)
			{
				*newInputString++ = '0' + k/10;
			}
			*newInputString++ = '0' + k % 10;
			*newInputString++ = '*';
			*newInputString++ = '(';
			strncpy(newInputString, inputString, length);
			newInputString += length;
			*newInputString++ = ')';
			*newInputString++ = ')';
			*newInputString++ = '}';
		} /*for (k = 2; i <= order; k++)*/
	}
	*newInputString++ = ')';
	strcpy(newInputString, *Strmodel + endplace);

	mydisphandle(Strmodel);
	Strmodel = NewStrmodel;
	NewStrmodel = (char **) 0;
	return (1);
} /*insertForm()*/ 

/* Makefile must change references to yylex generated by yacc to modelyylex */
static int Lastc = 0;
/*
  971104 added check for value of myhandlelength()
*/
static int modelyylex(void)
{  /* lexical analyzer for model parser */

	char          *inputString = *Strmodel;
	char           token[NAMELENGTH+1];
	char          *place;
	int            c, nextc;
	long           ivar = - 1, i;
	int            evaluated = 0;
	char          *expression = "{...} expression";
	Symbolhandle   symh = (Symbolhandle) 0;
	WHERE("modelyylex");

	while (1)
	{
		c = inputString[Imodchar++];
		if (c == '\\')
		{
			c = inputString[Imodchar++];
		}
		
		nextc = inputString[Imodchar];
		
		switch(c)
		{

		  case CR:  /* treat end of lines as white space */
		  case NL:
		  case SP:
		  case TAB:
			continue;				/* skip white space */

		  case '+':
		  case '-':
		  case '/':
		  case '*':
		  case '.':
		  case '=':
		  case '(':
		  case ')':
		  case '^':
			Lastc = c;
			return(c);
			/* simply pass through model symbols */

		  case '\0':
		  case '#':
			Lastc = 0;
			return(MODELEND);			/* end of string */

			/* treat 1 specially for constant */
		  case '1':
			if (!isnamestart(c) && c != TEMPPREFIX && Lastc != '^')
			{ /* set yylval to single zero term & return MODELVAR */
				Lastc = c;
				return (setyylval(0));
			}
			/* fall through */
		  case '0':
		  case '2':
		  case '3':
		  case '4':
		  case '5':
		  case '6':
		  case '7':
		  case '8':
		  case '9':
			if (Lastc == '^')
			{
				ModelPower = 0;
				
				while (isdigit(c))
				{
					ModelPower = 10*ModelPower + (c - '0');
					c = inputString[Imodchar++];
				} /*while (isdigit(c))*/
				Imodchar--;
				nextc = inputString[Imodchar];

				if (isnamestart(nextc) || nextc == TEMPPREFIX)
				{
					goto errorExit;
				}
				
				return ((ModelPower <= 0 || ModelPower > MAXPOWER) ?
						BADPOWER : MODELPOWER);
			} /*if (Lastc == '^')*/
			return (c);
			
		  case '{':
			{
				long        start, length;
				long        nbrace = 1;
				char       *labels[MAXDIMS];

				while (isspace(inputString[Imodchar]))
				{ /* trim off leading white space*/
					Imodchar++;
				}
				start = Imodchar;
				c = findBracket(0, '{', (unsigned char *) inputString,
								 &nbrace, &Imodchar, (long *) 0);
				length = Imodchar - start - 1;

				while ((c = inputString[start + length - 1]) == ';' ||
					   isspace(c))
				{ /* trim off trailing white space && ';' */
					length--;
				} /*while (isspace(inputString[start + length - 1]))*/
				
				if (nbrace > 0)
				{
					sprintf(OUTSTR, "ERROR: missing '}' in %s", expression);
				}
				else if (length <= 0)
				{
					sprintf(OUTSTR, "ERROR: %s is empty", expression);
				}
				else if (length >= BUFFERLENGTH)
				{
					sprintf(OUTSTR, "ERROR: %s is too long", expression);
				}
				
				if (*OUTSTR)
				{
					goto errorExit;
				}
				
				if (Info->nvars > -1)
				{
					/* not y; see if expression already seen */
					for (ivar = 0;ivar <= Info->nvars;ivar++)
					{
						Symbolhandle    thisVariable = Info->modelvars[ivar];
						
						if (strcmp(NAME(thisVariable), EVALSCRATCH) == 0 &&
							HASLABELS(thisVariable))
						{
							getAllLabels(thisVariable, labels,
										 (long *) 0, (long *) 0);
							if (strncmp(inputString + start, labels[0],
										length) == 0)
							{
								break;
							}
						}
					} /*for (ivar=0;ivar <= Info->nvars;ivar++)*/

					if (ivar == 0)
					{		/* Y variable appearing in model terms */
						strcpy(OUTSTR,
							   "ERROR: dependent varable expression found on r.h.s of model");
						goto errorExit;
					} /*if (ivar == 0)*/
				} /*if (Info->nvars > -1)*/
				else
				{
					/* first variable seen; must be y*/
					ivar = 0;
				}
				
				if (ivar > MAXVARS)
				{ /* would be too many variables; don't evaluate */
					break;
				}
				
				if (ivar > Info->nvars)
				{
					/*
					  New expression not previously seen;
					  evaluate it
					*/
					long         dims[2], lengths[2], oldLengths[2];
					long         ndims;
					char        *oldLabels[2], **oldLabel = (char **) 0;
					
					if (SCRATCHHANDLE)
					{
						long      handleLength = myhandlelength(SCRATCHHANDLE);

						if (handleLength < 0)
						{
							goto errorExit;
						}
						
						if (handleLength < length + 1)
						{
							clearSCRATCHHANDLE();
						}
					} /*if (SCRATCHHANDLE)*/

					if (SCRATCHHANDLE == (char **) 0)
					{
						SCRATCHHANDLE = mygethandle(length+1);
						if (SCRATCHHANDLE == (char **) 0)
						{
							goto errorExit;
						}
						inputString = *Strmodel; /*fresh dereference*/
					} /*if (SCRATCHHANDLE == (char **) 0)*/

					strncpy(*SCRATCHHANDLE, inputString + start, length);
					(*SCRATCHHANDLE)[length] = '\0';

					symh = mvEval(SCRATCHHANDLE);
					clearSCRATCHHANDLE();
					inputString = *Strmodel; /* play it safe */

					evaluated = 1;
					if (symh == (Symbolhandle) 0)
					{
						goto errorExit;
					}
					if (!isDefined(symh))
					{
						sprintf(OUTSTR,
								"ERROR: %s is UNDEFINED", expression);
					}
					else if (TYPE(symh) != REAL ||
							 !isMatrix(symh, dims) ||
							 ivar > 0 && !isVector(symh))
					{
						sprintf(OUTSTR,
								"ERROR: %s is not a REAL %s", expression,
								(ivar == 0) ? "matrix" : "vector");
					}
					if (*OUTSTR)
					{
						goto errorExit;
					}
					
					Cutsymbol(symh);/*be on safe side*/
					setNAME(symh, EVALSCRATCH);
					Addsymbol(symh);
					if (dims[1] > 1)
					{
						ndims = 2;
						if (HASLABELS(symh))
						{
							getMatLabels(symh, oldLabels, oldLengths);
							if (oldLengths[1] == dims[1])
							{
					/* column labels must all be ""; forget row labels */
								clearLabels(symh);
							}
						}
					} /*if (dims[1] > 1)*/
					else
					{
						ndims = 1;
						dims[1] = 0;
						if (HASLABELS(symh))
						{
							clearLabels(symh);
						}
					} /*if (dims[1] > 1){}else{}*/

					for (ivar = 0; ivar <= Info->nvars; ivar++)
					{ /*make sure previous variables are still ok*/
						Symbolhandle      thisModel = Info->modelvars[ivar];
						
						if (!myvalidhandle((char **) thisModel) ||
							!myvalidhandle((char **) DATA(thisModel)))
						{
							sprintf(OUTSTR,
									"%s has damaged other variables in the model",
									expression);
							goto errorExit;
						}
					} /*for (ivar = 0; ivar <= Info->nvars; ivar++)*/
					
					/* room for expression and dims[0] '\0' */
					lengths[0] = dims[0] + length;
					
					if (dims[1] > 0 && HASLABELS(symh))
					{ /* space for old column labels */
						lengths[1] = oldLengths[1];
					} /*if (dims[1] > 0 && HASLABELS(symh))*/
					else
					{ /* space for rep("",dims[1])*/
						lengths[1] = dims[1];
					}
					
					TMPHANDLE = createLabels(ndims, lengths);

					if (TMPHANDLE == (char **) 0)
					{
						goto errorExit;
					}
					
					if (HASLABELS(symh))
					{
						getMatLabels(symh, oldLabels, oldLengths);
						/* preserve current labels */
						oldLabel = LABELSHANDLE(symh);
						setLABELSHANDLE(symh, (char **) 0);
						if (!setLabels(symh, TMPHANDLE))
						{
							mydisphandle(TMPHANDLE);
							goto errorExit;
						}
					} /*if (HASLABELS(symh))*/
					else if (!setLabels(symh, TMPHANDLE))
					{
						mydisphandle(TMPHANDLE);
						goto errorExit;
					}
					inputString = *Strmodel;
					setNDIMS(symh, ndims);
					setDIM(symh, 1, dims[0]);

					strncpy(OUTSTR, inputString + start, length);
					OUTSTR[length] = '\0';
					/* Pad with enough '\0's to make real labels */
					appendLabels(symh, OUTSTR, 0, padExpand);
					OUTSTR[0] = '\0';
					if (ndims == 2)
					{
						setDIM(symh, 2, dims[1]);
						if (oldLabel == (char **) 0)
						{
							appendLabels(symh, NullString, 1, doExpand);
						}
						else
						{
							appendLabels(symh, oldLabels[1], 1, dontExpand);
							mydisphandle(oldLabel);
						}
					} /*if (dims[1] > 0)*/
				} /*if (ivar > Info->nvars)*/

				if (Info->nvars == -1)
				{	/* new Y variable */
					Info->modelvars[0] = symh;
					Info->nvars = 0;
					return (setyylval(0));
				} /*if (Info->nvars == -1)*/
			} /*case '{'*/
			break;

		  default: /* get next variable name in token */
			/*
			  not CR, NL, SP, TAB, +, -, /, *, ., =, (, ), ^, '\0'
			  #, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, {
			*/
			Lastc = 0;
			/* treat "E\W*(" specially; it begins an error term */
			if (c  == 'E')
			{
				for (place = inputString + Imodchar; *place; place++)
				{
					if (!isspace(*place))
					{
						break;
					}
				} /*for (place = inputString + Imodchar;*place;place++)*/
				if (*place == '(')
				{
					Imodchar = place - inputString;
					return((int) 'E');
				} /*if (*place == '(')*/
			} /*if (c  == 'E')*/

			if ((c == 'P' || c == 'C') && isdigit(inputString[Imodchar]))
			{
				long        place = Imodchar;
				long        order = 0;
				
				while(isdigit(inputString[place]))
				{
					order = 10*order + inputString[place] - '0';
					place++;
				}
				if (place > Imodchar)
				{
					while (isspace(inputString[place]))
					{
						place++;
					}
					if (inputString[place] == '(')
					{
						/* must be polynomial or periodic expression*/
						if (Info->nvars == -1)
						{
							sprintf(OUTSTR,
									"ERROR: %cn(...) illegal to left of '='",
									c);
						}
						else if (order > 99)
						{
							sprintf(OUTSTR,
									"ERROR: %s term order > 99",
									(c == 'P') ? "polynomial" : "periodic");
						}
						if (*OUTSTR)
						{
							Imodchar = place + 1;
							goto errorExit;
						}
						place++;
						/* place points to character after '(' */
						
						if (!insertForm(order, c))
						{
							goto errorExit;
						}
						inputString = *Strmodel;

						/* go around for another try*/
						continue;
					} /*if (inputString[place] == '(')*/
				} /*if (place > Imodchar)*/
			} /*if (c == 'P')*/
			
			if (isnamestart(c) || c == TEMPPREFIX)
			{
				token[0] = c;
				for (i = 1; i < NAMELENGTH; i++)
				{
					c = inputString[Imodchar];
					if (!isnamechar(c))
					{
						break;
					}
					token[i] = c;
					Imodchar++;
				} /*for (i = 1; i < NAMELENGTH; i++)*/
			} /*if (isnamestart(c) || c == TEMPPREFIX)*/
			else
			{
				/* we have illegal character */
				strcpy(OUTSTR, "ERROR: illegal character in model");
				goto errorExit;
			} /*if (isnamestart(c) || c == TEMPPREFIX){}else{}*/
			token[i] = '\0';

			if (isnamechar(inputString[Imodchar]))
			{
				/* variable name too long */
				Imodchar++;
				strcpy(OUTSTR, "ERROR: variable name in model too long");
				goto errorExit;
			} /*if (isnamechar(inputString[Imodchar]))*/

			symh = Lookup(token);

			if (symh == (Symbolhandle)  0)
			{
				/* unrecognized variable */
				strcpy(OUTSTR, "ERROR: unrecognized variable in model");
				goto errorExit;
			} /*if (symh == (Symbolhandle)  0)*/

			if (Info->nvars == -1)
			{	/* new Y variable */
				Info->modelvars[0] = symh;
				Info->nvars = 0;
				return (setyylval(0));
			} /*if (Info->nvars == -1)*/

			/* check to see if it is already in model */
			for (ivar = 0;ivar <= Info->nvars;ivar++)
			{
				if (strcmp(token,NAME(Info->modelvars[ivar])) == 0)
				{
					break;
				}
			} /*for (ivar=0;ivar <= Info->nvars;ivar++)*/

			if (ivar == 0)
			{		/* Y variable appearing in model terms */
				strcpy(OUTSTR,
					   "ERROR: dependent varable found on right hand side of model");
				goto errorExit;
			} /*if (ivar == 0)*/

		} /*switch(c)*/

		if (ivar > Info->nvars)
		{ /* must be new variable */
			Info->nvars++;

			if (Info->nvars > MAXVARS)
			{	/* too many variables */
				sprintf(OUTSTR,
						"ERROR: more than %ld variables in model",
						(long) MAXVARS);
				goto errorExit;
			} /*if (Info->nvars > MAXVARS)*/
			if (NCLASS(symh) > 0)
			{
				Info->nfactors++;
			} /*if (NCLASS(symh) > 0)*/
			if (Info->nfactors > MAXDIMS)
			{	/* too many factors */
				sprintf(OUTSTR,
						"ERROR: more than %ld factors in model",
						(long) MAXDIMS);
				goto errorExit;
			} /*if (Info->nfactors > MAXDIMS)*/

			Info->modelvars[ivar] = symh;
			if (ivar > 0)
			{
				Info->nclasses[ivar-1] = NCLASS(symh);
			} /*if (ivar > 0)*/
		} /*if (ivar > Info->nvars)*/

		return (setyylval(ivar));
	} /*while (1)*/

  errorExit:
		

	if (evaluated)
	{
		Removesymbol(symh);
	}
	if (*OUTSTR)
	{
		yyerror(OUTSTR);
		*OUTSTR = '\0';
	}
	
	return (BADTOKEN);
} /*modelyylex()*/

/* start of externally accessible (public) functions */
int modelparse(void)
{
	int       reply;
	
	MODELPARSERUNNING = FUNCNAME;
	reply = yyparse();
	MODELPARSERUNNING = (char *) 0;
	return (reply);
} /*modelparse()*/

int initmodelparse(char **model)
{
	int      i;

	if (MODELPARSERUNNING)
	{
		sprintf(OUTSTR,
				"ERROR: Recursive use of model parser by %s() while %s() is running",
				FUNCNAME, MODELPARSERUNNING);
		putErrorOUTSTR();
		return (0);
	}
	
	Info->nvars = -1;
	Info->nfactors = 0;
	Info->nerrorterms = 1;
	Imodchar = 0;
	for (i = 0;i <= MAXVARS;i++)
	{
		Info->modelvars[i] = (Symbolhandle) 0;
	} /*for (i = 0;i <= MAXVARS;i++)*/
	for (i = 0;i <= MAXERRTRMS;i++)
	{
		zeroTerm(Info->errorterms[i]);
	} /*for (i = 0;i <= MAXERRTRMS;i++)*/
	mydisphandle((char **) Info->model);
	Info->model = (modelHandle) 0;
	if (Strmodel != (char **) 0)
	{
		mydisphandle(Strmodel);
	}
	if (NewStrmodel != (char **) 0)
	{
		mydisphandle(NewStrmodel);
		NewStrmodel = (char **) 0;
	}
	Strmodel = myduphandle(model);
	return (Strmodel != (char **) 0);
} /*initmodelparse()*/

modelInfoPointer getModelInfo(void)
{
	return (Info);
} /*getModelInfo()*/



