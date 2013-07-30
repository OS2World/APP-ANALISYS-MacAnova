
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
#ifdef __cplusplus
#  include <stdio.h>
#  include <yacc.h>
#endif	/* __cplusplus */ 
# define MODELVAR 257
# define MODELEND 258
# define MODELPOWER 259
# define BADPOWER 260
# define BADTOKEN 261
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
static int yychar;
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
static YYSTYPE yylval;
__YYSCLASS YYSTYPE yyval;
typedef int yytabelem;
# define YYERRCODE 256


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
   called by modelyylex() for '1' (var == 0) or variable name (var == varno+1).
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



__YYSCLASS yytabelem yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
	};
# define YYNPROD 22
# define YYLAST 245
__YYSCLASS yytabelem yyact[]={

    30,    15,    13,    14,    17,    14,    16,    15,    13,    15,
    17,    14,    16,    14,    16,     8,     8,    27,    32,    15,
     4,     2,     5,    14,    35,     5,    18,     5,    28,    29,
     6,    37,    21,     1,    31,     0,     0,     0,     0,     0,
     0,    11,     3,     0,     9,     9,     0,     0,     0,    19,
     0,    18,     0,    18,     0,     0,     0,     0,     0,    18,
     0,    18,    19,    33,    19,    19,    19,    19,    19,     7,
     0,    18,    38,    36,     0,     0,    19,     0,    20,    33,
    36,     0,     0,    22,    23,    24,    25,    26,     0,     0,
     0,     0,     0,     0,     0,     0,     0,    34,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     5,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     5,     0,
     5,     0,     0,    12,     0,     0,     5,     0,     5,     0,
     0,     0,    10,    10,     0,     0,     5,     5,     5,     0,
     0,     0,     0,     0,     5 };
__YYSCLASS yytabelem yypact[]={

  -236, -3000,   -31, -3000, -3000, -3000,   -24,   -35,   -24,    -8,
 -3000, -3000, -3000,   -24,   -24,   -24,   -24,   -25,  -231, -3000,
   -41,  -239,   -33,   -68,   -43,   -23,   -33,   -24, -3000, -3000,
 -3000,   -17,   -15, -3000,   -43, -3000, -3000,  -239,  -234 };
__YYSCLASS yytabelem yypgo[]={

     0,    33,    69,    41,    34 };
__YYSCLASS yytabelem yyr1[]={

     0,     1,     1,     1,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     4,     4,     4,
     4,     3 };
__YYSCLASS yytabelem yyr2[]={

     0,     9,     3,     3,     7,     7,     7,     7,     7,     7,
     9,     7,     7,     9,     3,     5,     3,     3,     7,     5,
     3,     3 };
__YYSCLASS yytabelem yychk[]={

 -3000,    -1,   257,    -3,   256,   261,    61,    -2,    40,    69,
   257,    -3,   258,    43,    46,    42,    47,    45,    94,    -3,
    -2,    40,    -2,    -2,    -2,    -2,    -2,    42,   259,   260,
    41,    -4,   257,    -3,    -2,    41,    -3,    46,    -4 };
__YYSCLASS yytabelem yydef[]={

     0,    -2,     0,     2,     3,    21,     0,     0,     0,     0,
    14,    16,     1,     0,     0,     0,     0,     0,     0,    15,
     0,     0,     4,     5,     6,     7,     9,     0,    11,    12,
     8,     0,    17,    20,    10,    13,    19,     0,    18 };
typedef struct { char *t_name; int t_val; } yytoktype;
#ifndef YYDEBUG
#	define YYDEBUG	0	/* don't allow debugging */
#endif

#ifdef YYDEBUG

__YYSCLASS yytoktype yytoks[] =
{
	"MODELVAR",	257,
	"MODELEND",	258,
	"MODELPOWER",	259,
	"BADPOWER",	260,
	"BADTOKEN",	261,
	"+",	43,
	"-",	45,
	"/",	47,
	"*",	42,
	".",	46,
	"^",	94,
	"-unknown-",	-1	/* ends search */
};

__YYSCLASS char * yyreds[] =
{
	"-no such reduction-",
	"assgn : MODELVAR '=' model MODELEND",
	"assgn : lexerror",
	"assgn : error",
	"model : model '+' model",
	"model : model '.' model",
	"model : model '*' model",
	"model : model '/' model",
	"model : '(' model ')'",
	"model : model '-' model",
	"model : model '-' '*' model",
	"model : model '^' MODELPOWER",
	"model : model '^' BADPOWER",
	"model : 'E' '(' term ')'",
	"model : MODELVAR",
	"model : model lexerror",
	"model : lexerror",
	"term : MODELVAR",
	"term : MODELVAR '.' term",
	"term : term lexerror",
	"term : lexerror",
	"lexerror : BADTOKEN",
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
static int yydebug = 0;			/* set to 1 to get debugging */

/*
** driver internal defines
*/
/* define for YYFLAG now generated by yacc program. */
/*#define YYFLAG		(FLAGVAL)*/

/*
** global variables used by the parser
*/
# ifndef __RUNTIME_YYMAXDEPTH
__YYSCLASS YYSTYPE yyv[ YYMAXDEPTH ];	/* value stack */
__YYSCLASS int yys[ YYMAXDEPTH ];		/* state stack */
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

__YYSCLASS YYSTYPE *yypv;			/* top of value stack */
__YYSCLASS int *yyps;			/* top of state stack */

__YYSCLASS int yystate;			/* current state */
__YYSCLASS int yytmp;			/* extra var (lasts between blocks) */

static int yynerrs;			/* number of errors */
__YYSCLASS int yyerrflag;			/* error recovery flag */



/*
** yyparse - return 0 if worked, 1 if syntax error not recovered from
*/
static int
yyparse()
{
	register YYSTYPE *yypvt;	/* top of value stack for $vars */
	WHERE("yyparse_mreader");

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
#ifdef YYDEBUG
		/*
		** if debugging, look up token value in list of value vs.
		** name pairs.  0 and negative (-1) are special values.
		** Note: linear search is used since time is not a real
		** consideration while debugging.
		*/
		if ( yydebug )
		{
			register int yy_i;

			printf( "State %d, token ", yy_state );
			if ( yychar == 0 )
				printf( "end-of-file\n" );
			else if ( yychar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
						break;
				}
				printf( "%s\n", yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ++yy_ps >= &yys[ yymaxdepth ] )	/* room on stack? */
		{
# ifndef __RUNTIME_YYMAXDEPTH
			yyerror( (nl_msg(30002,"yacc stack overflow")) );
			YYABORT;
# else
			/* save old stack bases to recalculate pointers */
			YYSTYPE * yyv_old = yyv;
			int * yys_old = yys;
			yymaxdepth += YYINCREMENT;
			yys = (int *) realloc(yys, yymaxdepth * sizeof(int));
			yyv = (YYSTYPE *) realloc(yyv, yymaxdepth * sizeof(YYSTYPE));
			if (yys==0 || yyv==0) {
			    yyerror( (nl_msg(30002,"yacc stack overflow")) );
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
#ifdef YYDEBUG
		/*
		** if debugging, need to mark whether new token grabbed
		*/
		yytmp = yychar < 0;
#endif
		if ( ( yychar < 0 ) && ( ( yychar = modelyylex() ) < 0 ) )
			yychar = 0;		/* reached EOF */
#ifdef YYDEBUG
		if ( yydebug && yytmp )
		{
			register int yy_i;

			printf( "Received token " );
			if ( yychar == 0 )
				printf( "end-of-file\n" );
			else if ( yychar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
						break;
				}
				printf( "%s\n", yytoks[yy_i].t_name );
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
#ifdef YYDEBUG
			yytmp = yychar < 0;
#endif
			if ( ( yychar < 0 ) && ( ( yychar = modelyylex() ) < 0 ) )
				yychar = 0;		/* reached EOF */
#ifdef YYDEBUG
			if ( yydebug && yytmp )
			{
				register int yy_i;

				printf( "Received token " );
				if ( yychar == 0 )
					printf( "end-of-file\n" );
				else if ( yychar < 0 )
					printf( "-none-\n" );
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
					printf( "%s\n", yytoks[yy_i].t_name );
				}
			}
#endif /* YYDEBUG */
			/*
			** look through exception table
			*/
			{
				register int *yyxi = yyexca;

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
				yyerror( (nl_msg(30003,"syntax error")) );
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
#ifdef YYDEBUG
#	define _POP_ "Error recovery pops state %d, uncovers state %d\n"
					if ( yydebug )
						printf( _POP_, *yy_ps,
							yy_ps[-1] );
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
#ifdef YYDEBUG
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

					printf( "Error recovery discards " );
					if ( yychar == 0 )
						printf( "token end-of-file\n" );
					else if ( yychar < 0 )
						printf( "token -none-\n" );
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
						printf( "token %s\n",
							yytoks[yy_i].t_name );
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
#ifdef YYDEBUG
		/*
		** if debugging, print the string that is the user's
		** specification of the reduction which is just about
		** to be done.
		*/
		if ( yydebug )
			printf( "Reduce by (%d) \"%s\"\n",
				yy_n, yyreds[ yy_n ] );
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
{
										yyval = unique(yypvt[-1], 1);
										Info->nterms = GetLength(yyval);
							
										Info->model = GetModel(yyval);
										SetModel(yyval,0);
										RemoveModel(yyval);

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
									} break;
case 2:
{
							  errorExit:
								mydisphandle(Strmodel);
								Strmodel = (char **) 0;
								return (1);
							} break;
case 3:
{ goto errorExit; } break;
case 4:
{
								yyval = mjoin(yypvt[-2],yypvt[-0]);
							  checkit:
								if(yyval == (Model) 0)
								{
									goto errorExit;
								}
							} break;
case 5:
{ yyval = mdot(yypvt[-2],yypvt[-0]); goto checkit; } break;
case 6:
{ yyval = mstar(yypvt[-2],yypvt[-0]); goto checkit; } break;
case 7:
{ yyval = mnest(yypvt[-2],yypvt[-0]); goto checkit; } break;
case 8:
{ yyval = yypvt[-1]; goto checkit; } break;
case 9:
{ yyval = mminus(yypvt[-2],yypvt[-0]); goto checkit; } break;
case 10:
{ yyval = mmstar(yypvt[-3],yypvt[-0]); goto checkit; } break;
case 11:
{ yyval = mpower(yypvt[-2], ModelPower); goto checkit;} break;
case 12:
{
								sprintf(OUTSTR,
										"ERROR: illegal power < 1 or > %ld in model",
										(long) MAXPOWER);
								yyerror(OUTSTR);
								*OUTSTR = '\0';
								goto errorExit;
							} break;
case 13:
{ yyval = madder(yypvt[-1]); goto checkit; } break;
case 14:
{ yyval = yypvt[-0] ; goto checkit; } break;
case 15:
{ goto errorExit; } break;
case 16:
{ goto errorExit; } break;
case 17:
{ yyval = yypvt[-0]; goto checkit; } break;
case 18:
{ yyval = mdot(yypvt[-2],yypvt[-0]); goto checkit; } break;
case 19:
{ goto errorExit; } break;
case 20:
{ goto errorExit; } break;
case 21:
{ goto errorExit; } break;
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

