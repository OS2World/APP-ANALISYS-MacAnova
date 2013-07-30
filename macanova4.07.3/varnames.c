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
#pragma segment GetNames
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
#include "keywords.h"

/*
  names <- varnames([model]), where model is a CHARACTER variable (default
  STRMODEL) returns a CHARACTER vector with the names of the dependent
  variable and the factors and  variates in the model.  No syntax checking
  is done except names a checked for length.  Thus varnames("y a b a.b.c") and
  varnames("y=a+b+a.b.c") all return cat("y","a","b","c").
*/

#define   TAB       '\t'
#define   NL        10
#define   CR        13
#define   SP        ' '
static long         getVariableNames(char *model,
									 char names[MAXVARS+1][NAMELENGTH+1])
{ 
	char            c;
	char            token[2*NAMELENGTH+1];
	long            length, nfactors = 0, i;
	WHERE("getVariableNames");

	while (1)
	{
		while ((c = *model) != '\0' && c != '#' && !isnamestart(c) &&
			  c != TEMPPREFIX)
		{ /* look for start of name */
			model++;
		}
		if (c == '\0' || c == '#')
		{
			break;
		}
		token[0] = c;
		
		for (length=1;length<2*NAMELENGTH;length++)
		{
			if (!isnamechar(model[length]))
			{
				break;
			}
			token[length] = model[length];
		}
		token[length] = '\0';
		if (length > NAMELENGTH)
		{
			sprintf(OUTSTR,
					"ERROR: name %s in model for %s() is longer than %ld characters",
					token, FUNCNAME, (long) NAMELENGTH);
			goto errorExit;
		}
		if (strcmp(token,"E") == 0)
		{ /* allow for error terms */
			for (i=1;(c = model[i]) != '\0';i++)
			{ /* skip white space looking for '(' */
				if (c != SP && c != TAB && c != NL && c != CR)
				{
					break;
				}
			}
		}
		else
		{
			c = '\0';
		}
		if (c != '(')
		{ /* not error term */
			for (i=0;i<nfactors;i++)
			{ /* have we seen name before? */
				if (strlen(names[i]) == length && strcmp(token,names[i]) == 0)
				{ /* yes */
					break;
				}
			}
			if (i == nfactors)
			{					/* new name */
				if (nfactors > MAXVARS)
				{
					sprintf(OUTSTR,
							"ERROR: more than %ld names in model for %s()",
							(long) MAXVARS, FUNCNAME);
					goto errorExit;
				}
				strcpy(names[nfactors],token);
				nfactors++;
			}
		}
		model += length;
	} /* while (1) */

	return (nfactors);
	
  errorExit:
	putErrorOUTSTR();
	return (-1);
	
} /*getVariableNames()*/


Symbolhandle    varnames(Symbolhandle list)
{
	long            nargs = NARGS(list);
	Symbolhandle    symh;
	Symbolhandle    result = (Symbolhandle) 0;
	char            names[MAXVARS+1][NAMELENGTH+1];
	char           *model, *place;
	long            nvars = (long) NVARS;
	long            i, nfactors = 0, needed = 0;
	WHERE("factornames");
	
	*OUTSTR = '\0';
	if (nargs > 1)
	{
		badNargs(FUNCNAME,-1);
		goto errorExit;
	}
	symh = COMPVALUE(list,0);
	if (symh != (Symbolhandle) 0)
	{
		nvars = -1;
		if (!isCharOrString(symh))
		{
			char        outstr[30];
			
			sprintf(outstr, "argument to %s()", FUNCNAME);
			notCharOrString(outstr);
			goto errorExit;
		}
		model = STRINGPTR(symh);
	}
	else if (nvars >= 0)
	{
		nfactors = nvars+1;
		for (i=0;i<nfactors;i++)
		{
			strcpy(names[i],VARNAMES[i]);
		}
	}
	else if (isCharOrString(symh = Lookup("STRMODEL")))
	{
		model = STRINGPTR(symh);
		nvars = -1;
	}
	else
	{
		sprintf(OUTSTR,
				"ERROR: no argument for %s() and no current model",
				FUNCNAME);
		goto errorExit;
	}
	if (nvars < 0 && (nfactors = getVariableNames(model,names)) < 0)
	{
		goto errorExit;
	}

	if (nfactors == 0)
	{
		sprintf(OUTSTR,"ERROR: no legal variate or factor names in %s",model);
		goto errorExit;
	}
	
	for (i = 0; i < nfactors;i++)
	{
		int       evaluated = nvars >= 0 && isevaluated(names[i]);
		
		needed += strlen((!evaluated) ?
						 names[i] : getOneLabel(MODELVARS[i], 0, 0)) + 1;
	}
	
	result = CInstall(SCRATCH, needed);
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	
	place = STRINGPTR(result);
	for (i = 0; i < nfactors;i++)
	{
		int       evaluated = (nvars >= 0) && isevaluated(names[i]);
		
		strcpy(place,
			   (!evaluated) ? names[i] : getOneLabel(MODELVARS[i], 0, 0));
		place = skipStrings(place, 1);
	} /*for (i = 0; i < nfactors;i++)*/
	Setdims(result, 1, &nfactors);
	
	return (result);
	
  errorExit:
	putErrorOUTSTR();
	Removesymbol(result);
	
	return(0);
	
} /*varnames()*/

enum modelVarCodes
{
	
	YONLY        =  0,
	XONLY        = -1,
	ALL          = -2,
	VARIATESONLY = -3,
	FACTORSONLY  = -4,
	HASCONST     = -5
};

enum modelVarsKeyCodes
{
	KALL = 0,
	KY,
	KX,
	KNX,
	KVARIATES,
	KNVARIATES,
	KFACTORS,
	KNFACTORS,
	KHASCONST
};

static keywordList        ModelvarsKeys[] =
{
	InitKeyEntry("all", 0, 0, LOGICSCALAR), 
	InitKeyEntry("y", 0, 0, LOGICSCALAR), 
	InitKeyEntry("x", 0, 0, LOGICSCALAR), 
	InitKeyEntry("nx", 0, 0, LOGICSCALAR), 
	InitKeyEntry("variates", 7, 0, LOGICSCALAR), 
	InitKeyEntry("nvariates", 8, 0, LOGICSCALAR), 
	InitKeyEntry("factors", 6, 0, LOGICSCALAR),
	InitKeyEntry("nfactors", 7, 0, LOGICSCALAR),
	InitKeyEntry("hasconst", 7, 0, LOGICSCALAR)
};

#define GetAll       KeyLogValue(ModelvarsKeys, KALL)
#define GetY         KeyLogValue(ModelvarsKeys, KY)
#define GetX         KeyLogValue(ModelvarsKeys, KX)
#define GetNX        KeyLogValue(ModelvarsKeys, KNX)
#define GetVariates  KeyLogValue(ModelvarsKeys, KVARIATES)
#define GetNVariates KeyLogValue(ModelvarsKeys, KNVARIATES)
#define GetFactors   KeyLogValue(ModelvarsKeys, KFACTORS)
#define GetNFactors  KeyLogValue(ModelvarsKeys, KNFACTORS)
#define GetHasConst  KeyLogValue(ModelvarsKeys, KHASCONST)
/*
   951222   No longer an error if nothing to return; returns NULL
   970822   Changed call to initmodelpars() to reflect a value is now returned
   971113   Added keywords nx, nfactors, nvariates to return the total
            number of r.h.s variables, the number of factors, and the number
            of variables, respectively in the model
   971114   Added keyword phrase hasconst:T
   
*/
 
Symbolhandle    modelvars(Symbolhandle list)
{
	Symbolhandle        result = (Symbolhandle) 0, symh;
	Symbolhandle        symhModel = (Symbolhandle) 0;
	modelInfoPointer    info = (modelInfoPointer) 0;
	long                nargs = NARGS(list), nvars = (long) NVARS;
	long                mvars, ndata, nfactors, nvariates, nmissing;
	long                varnos[MAXVARS+1], maxvarnos = 0;
	long               *nclasses;
	long                length, i, j;
	long                useCurrentModel = 0;
	long                nkeys = NKeys(ModelvarsKeys), keyStatus;
	int                 countOnly = 0;
	keywordListPtr      keys = ModelvarsKeys;
	double             *x, *y;
	char              **strmodel = (char **) 0;
	char              **strmodel1 = (char **) 0;
	char               *keyword = (char *) 0;
	WHERE("modelvar");
	TRASH(1,errorExit);
	
	if (nargs > 2)
	{
		badNargs(FUNCNAME,-3);
		goto errorExit;
	}
	
	symh = COMPVALUE(list,0);
	if (!argOK(symh,0,1))
	{
		goto errorExit;
	}

	if (nargs == 2)
	{
		symhModel = COMPVALUE(list, 1);
		if (isKeyword(symhModel))
		{
			sprintf(OUTSTR,
					"ERROR: argument 2 to %s() must not be keyword phrase",
					FUNCNAME);
			goto errorExit;
		} /*if (isKeyword(symhModel))*/
		if (symhModel != (Symbolhandle) 0)
		{
			if (!isCharOrString(symhModel) ||
				strlen(STRINGPTR(symhModel)) == 0)
			{
				sprintf(OUTSTR,
						"ERROR: model for %s() must be non-null string or CHARACTER variable",
						FUNCNAME);
				goto errorExit;
			}
		} /*if (symhModel != (Symbolhandle) 0)*/
		strmodel = STRING(symhModel);
	} /*if (nargs == 2)*/
	
	if ((keyword = isKeyword(symh)))
	{
		mvars = -1;

		GetAll = 0;
		GetY = 0;
		GetX = 0;
		GetVariates = 0;
		GetFactors = 0;
		GetNX = 0;
		GetNVariates = 0;
		GetNFactors = 0;
		GetHasConst = 0;

		for (i = 0; i < nkeys; i++)
		{
			keyStatus = getKeyValues(list, 0, 0, keys + i);
			if (keyStatus == 0)
			{
				goto errorExit;
			}
			if (keyStatus > 0)
			{
				break;
			}
		} /*for (i = 0; i < nkeys; i++)*/
		if (i == nkeys)
		{
			badKeyword(FUNCNAME, keyword);
			goto errorExit;
		}
		
		if (GetAll)
		{
			varnos[0] = ALL;
		}
		else if (GetY)
		{
			varnos[0] = YONLY;
		}
		else if (GetX || GetNX)
		{
			varnos[0] = XONLY;
			countOnly = GetNX;
		}
		else if (GetVariates || GetNVariates)
		{
			varnos[0] = VARIATESONLY;
			countOnly = GetNVariates;
		}
		else if (GetFactors || GetNFactors)
		{
			varnos[0] = FACTORSONLY;
			countOnly = GetNFactors;
		}
		else if (GetHasConst)
		{
			varnos[0] = HASCONST;
		}
		else 
		{
			keyword = KeyName(keys, i);
			sprintf(OUTSTR, "ERROR: value for %s() keyword %s must be T",
					FUNCNAME, keyword);
			goto errorExit;
		}
	} /*if ((keyword = isKeyword(symh)))*/
	else
	{
		if (TYPE(symh) != REAL || !isVector(symh))
		{
			sprintf(OUTSTR,
					"ERROR: argument 1 to %s() must be a REAL scalar or vector",
					FUNCNAME);
			goto errorExit;
		}
		mvars = symbolSize(symh);
		if (mvars > MAXVARS+1)
		{
			sprintf(OUTSTR,
					"ERROR: argument 1 to %s() has length greater than %ld",
					FUNCNAME, (long) MAXVARS+1);
			goto errorExit;
		}
	
		for (j=0;j<mvars;j++)
		{
			varnos[j] = (long) DATAVALUE(symh,j);
			if (DATAVALUE(symh,j) != (double) varnos[j] || varnos[j] < 0)
			{
				sprintf(OUTSTR,
						"ERROR: elements of argument 1 to %s() must be non-negative integers",
						FUNCNAME);
				goto errorExit;
			}
			if (varnos[j] > maxvarnos)
			{
				maxvarnos = varnos[j];
			}
		} /*for (j=0;j<mvars;j++)*/
	} /*if ((keyword = isKeyword(symh))){...}else{...}*/	

	if (nargs == 1)
	{ /* no model specified since the model must be argument 2 */
		if (nvars >= 0)
		{ /* active model exists */
			if (maxvarnos > nvars)
			{
				sprintf(OUTSTR,
						"ERROR: fewer than %ld variables in model %s for %s()",
						maxvarnos, *STRMODEL, FUNCNAME);
			}
			else if (PREVMODELTYPE & FASTANOVA)
			{
				sprintf(OUTSTR,
						"ERROR: %s(varno) not valid after fastanova(); try %s(varno,STRMODEL)",
						FUNCNAME, FUNCNAME);
			}
			if (*OUTSTR)
			{
				goto errorExit;
			}
			useCurrentModel = 1;
		} /*if (nvars >= 0)*/
		else if (STRMODEL != (char **) 0)
		{
			strmodel = STRMODEL;
		}
		else
		{ /* use model in variable STRMODEL if available */
			symhModel = Lookup("STRMODEL");
			if (symhModel == (Symbolhandle) 0)
			{
				sprintf(OUTSTR,
						"ERROR: no active model and variable STRMODEL does not exist");
			}
			else if (!isCharOrString(symhModel) ||
					strlen(STRINGPTR(symhModel)) == 0)
			{
				sprintf(OUTSTR,
						"ERROR: STRMODEL is not CHARACTER scalar or is null string");
			}
			if (*OUTSTR)
			{
				goto errorExit;
			}
			strmodel = STRING(symhModel);
		}
	} /*if (nargs == 1)*/

	if (!useCurrentModel)
	{
		if ((strmodel1 = setStrmodel(strmodel,0)) == (char **) 0)
		{
			goto errorExit;
		}
		toTrash(strmodel1,0);
		
		if (!initmodelparse(strmodel1) || modelparse() != 0)
		{ /* parse it */
			goto errorExit;
		}
		info = getModelInfo(); /* retrieve model information */
		if ((ndata = checkVariables(info)) < 0)
		{
			goto errorExit;
		}
		nmissing = countMissing(info, (double ***) 0);
		if (nmissing < 0)
		{
			goto errorExit;
		}
	} /*if (!useCurrentModel)*/
	else
	{
		strmodel = STRMODEL;
		info = MODELINFO;
		ndata = (long) NDATA;
	}

	nvars = info->nvars;
	nfactors = info->nfactors;
	nvariates = nvars - nfactors;
	nclasses = info->nclasses;

	if (countOnly || GetHasConst)
	{
		long        count;
		
		result = RInstall(SCRATCH, 1);
		if (result == (Symbolhandle) 0)
		{
			goto errorExit;
		}
		if (varnos[0] == VARIATESONLY)
		{
			count = nvariates;
		}
		else if (varnos[0] == FACTORSONLY)
		{
			count = nfactors;
		}
		else if (varnos[0] == XONLY)
		{
			count = nvars;
		}
		else
		{
			long       i;
			
			setTYPE(result, LOGIC);

			count = 0;
			for (i = 0; i < info->nterms; i++)
			{
				if (nvInTerm(modelterm(info->model, i)) == 0)
				{
					count = 1;
					break;
				}
			} /*for (i = 0; i < nterms; i++)*/
		}
		
		DATAVALUE(result, 0) = (double) count;
	} /*if (countOnly)*/
	else
	{
		if (mvars < 0)
		{
			if (varnos[0] < YONLY)
			{
				if (varnos[0] == ALL || varnos[0] == XONLY)
				{ /* all:T or x:T */
					mvars = (varnos[0] == XONLY) ? nvars : nvars + 1;
					i = (varnos[0] == XONLY) ? 1 : 0;
					for (j=0;j<mvars;j++)
					{
						varnos[j] = j + i;
					}
				}
				else if (varnos[0] == VARIATESONLY)
				{ /* variates:T */
					if (nvariates > 0)
					{
						mvars = 0;
						for (i=0;i<nvars;i++)
						{
							if (nclasses[i] < 0)
							{
								varnos[mvars++] = i + 1;
							}
						} /*for (i=0;i<nvars;i++)*/
					} /*if (nvariates > 0)*/
					else
					{
						mvars = 0;
					}
				}
				else
				{ /* factors:T */
					if (nfactors > 0)
					{
						mvars = 0;
						for (i=0;i<nvars;i++)
						{
							if (nclasses[i] > 0)
							{
								varnos[mvars++] = i + 1;
							}
						} /*for (i=0;i<nvars;i++)*/
					} /*if (nfactors > 0)*/
					else
					{
						mvars = 0;
					}
				}
			} /*if (varnos[0] < YONLY)*/
			else
			{ /* must be YONLY */
				mvars = 1;
			}
		} /*if (mvars < 0)*/

		length = 0;
		for (j=0;j<mvars;j++)
		{
			if (varnos[j] > nvars)
			{
				sprintf(OUTSTR,
						"ERROR: model for %s() does not contain %ld variates or factors",
						FUNCNAME, varnos[j]);
				goto errorExit;
			}
			length += symbolSize(info->modelvars[varnos[j]]);
		} /*for (j=0;j<mvars;j++)*/
	
		result = (mvars > 0) ?
		  RInstall(SCRATCH,length) : Install(SCRATCH, NULLSYM);
		if (result == (Symbolhandle) 0)
		{
			goto errorExit;
		}

		if (mvars > 0)
		{
			if (length > ndata)
			{
				setNDIMS(result,2);
				setDIM(result,1,ndata);
				setDIM(result,2,length/ndata);
			}
			else
			{
				setNCLASS(result,(varnos[0] == 0) ?
						  NCLASS(info->modelvars[0]) : nclasses[varnos[0]-1]);
			}
	
			y = DATAPTR(result);
			for (j=0;j<mvars;j++)
			{
				symh = info->modelvars[varnos[j]];
				length = symbolSize(symh);
				x = DATAPTR(symh);
				for (i=0;i<length;i++)
				{
					*y++ = *x++;
				}
			} /*for (j=0;j<mvars;j++)*/
		} /*if (mvars > 0)*/	
	} /*if (countOnly){}else{}*/
	
	return (result);
	
  errorExit:
	putErrorOUTSTR();
	Removesymbol(result);
	emptyTrash();
	
	return (0);
} /* modelvars()*/

enum modelinfoCodes
{
	KMISSING = 0,
	KNOMODELOK,
	KXVARS,
	KYVAR,
	KXTXINV,
	KCOEFS,
	KCOLCOUNT,
	KWEIGHTS,
	KPARAM,
	KSTRMODEL,
	KBITMODEL,
	KLINK,
	KDISTRIB,
	KTERMNAMES,
	KSCALE,
	KSIGMAHAT,
	KALIASED,
	KEVERYTHING
};
static keywordList        ModelinfoKeys[] =
{
	InitKeyEntry("missing",4,0,REALSCALAR),
	InitKeyEntry("nomodelok",7,0,LOGICSCALAR),
	InitKeyEntry("xvars",4,0,LOGICSCALAR),
	InitKeyEntry("y",0,0,LOGICSCALAR),
	InitKeyEntry("xtxinv",0,0,LOGICSCALAR),
	InitKeyEntry("coefs",4,0,LOGICSCALAR),
	InitKeyEntry("colcounts",4,0,LOGICSCALAR),
	InitKeyEntry("weights",6,0,LOGICSCALAR),
	InitKeyEntry("parameters",5,0,LOGICSCALAR),
	InitKeyEntry("strmodel",4,0,LOGICSCALAR),
	InitKeyEntry("bitmodel",4,0,LOGICSCALAR),
	InitKeyEntry("link",0,0,LOGICSCALAR),
	InitKeyEntry("distrib",4,0,LOGICSCALAR),
	InitKeyEntry("termnames",4,0,LOGICSCALAR),
	InitKeyEntry("scale",0,0,LOGICSCALAR),
	InitKeyEntry("sigmahat",5,0,LOGICSCALAR),
	InitKeyEntry("aliased",5,0,LOGICSCALAR),
	InitKeyEntry("all",0,0,LOGICSCALAR)
}; /*keywordList ModelinfoKeys[]*/

#define MissingVal    (KeyRealValue(ModelinfoKeys,KMISSING))
#define NoModelOK     (KeyLogValue(ModelinfoKeys,KNOMODELOK))
#define GetXvars      (KeyLogValue(ModelinfoKeys,KXVARS))
#define GetYvar       (KeyLogValue(ModelinfoKeys,KYVAR))
#define GetXtxinv     (KeyLogValue(ModelinfoKeys,KXTXINV))
#define GetCoefs      (KeyLogValue(ModelinfoKeys,KCOEFS))
#define GetColcount   (KeyLogValue(ModelinfoKeys,KCOLCOUNT))
#define GetWeights    (KeyLogValue(ModelinfoKeys,KWEIGHTS))
#define GetParam      (KeyLogValue(ModelinfoKeys,KPARAM))
#define GetStrmodel   (KeyLogValue(ModelinfoKeys,KSTRMODEL))
#define GetBitmodel   (KeyLogValue(ModelinfoKeys,KBITMODEL))
#define GetLink       (KeyLogValue(ModelinfoKeys,KLINK))
#define GetDistrib    (KeyLogValue(ModelinfoKeys,KDISTRIB))
#define GetTermnames  (KeyLogValue(ModelinfoKeys,KTERMNAMES))
#define GetScale      (KeyLogValue(ModelinfoKeys,KSCALE))
#define GetSigmahat   (KeyLogValue(ModelinfoKeys,KSIGMAHAT))
#define GetAliased    (KeyLogValue(ModelinfoKeys,KALIASED))
#define GetEverything (KeyLogValue(ModelinfoKeys,KEVERYTHING))

enum scratchVars
{
	GSTRMODEL1,
	GMISSWTS,
	GTMPALPHA,
	GCOLPTR,
	NTRASH
};

/* also defined in glmutils.c and anovacoe.c */
#define ALIASED     -1  /* column is intrinsically aliased */
#define PIVOTED     -2  /* column is aliased, but not intrinsically */
#define UNTRANSPOSED 0
/*
  xvariables() returns the X-variables (not the model variables) associated
  with the active model, or specified by STRMODEL if there is no active model
  xvariables(strmodel) returns the X-variables associated with strmodel, a
  CHARACTER variable that is parsed just as in the glm functions.

  modelinfo(xvars:T) is equivalent to xvariables()
  modelinfo(coefs:T) returns the coefficient vector (matrix) if after manova()
  associated with the design matrix.
  modelinfo(xtxinv:T) return (X'X)^-1 associated with the design matrix
  modelinfo(colcount:T) returns a vector containing the number of X-variables
  associated with each term
  modelinfo(weights:T) returns the vector of weights associated with the
  computation.
  modelinfo(param:T) returns the vector *LOGITN
  modelinfo(strmodel:T) returns just the model provided or STRMODEL
  modelinfo(bitmodel:T) returns a vector of length NTERMS (NVAR==1)
    or a nterms by (2 or 3) matrix (depending on NVAR) of the bit
    representation of the model
  modelinfo(all:T) is equivalent to modelinfo(xvars:T,coefs:T,xtxinv:T,
	colcount:T,weights:T,strmodel:T,bitmodel:T)
  If more than one keyword is specified a structure is returned with
  components, xvariables, coefs, xtxinv, colcounts, and/or weights

  980129 added keyword phrase nomodelok:T to modelinfo();  if present and
         there is no active model, NULL is returned instead of an error
         being signalled.
  990226 changed putOUTSTR() to putErrorOUTSTR()
*/

#define SETBYALL      2
#define MAKEVALUENULL 3

#ifdef MW_CW_New
#pragma global_optimizer on
#endif /*MW_CW_New*/

/* entry for xvariables() and modelinfo()*/
Symbolhandle xvariables(Symbolhandle list)
{
	Symbolhandle     arg, modelarg = (Symbolhandle) 0;
	Symbolhandle     result = (Symbolhandle) 0;
	modelInfoPointer info;
	Symbolhandle     symhxvars = (Symbolhandle) 0;
	Symbolhandle     symhyvar = (Symbolhandle) 0;
	Symbolhandle     symhcoefs = (Symbolhandle) 0;
	Symbolhandle     symhscale = (Symbolhandle) 0;
	Symbolhandle     symhsigmahat = (Symbolhandle) 0;
	Symbolhandle     symhxtxinv = (Symbolhandle) 0;
	Symbolhandle     symhcolcounts = (Symbolhandle) 0;
	Symbolhandle     symhwts = (Symbolhandle) 0;
	Symbolhandle     symhparam = (Symbolhandle) 0;
	Symbolhandle     symhstrmodel = (Symbolhandle) 0;
	Symbolhandle     symhbitmodel = (Symbolhandle) 0;
	Symbolhandle     symhlink = (Symbolhandle) 0;
	Symbolhandle     symhdistrib = (Symbolhandle) 0;
	Symbolhandle     symhtermnames = (Symbolhandle) 0;
	Symbolhandle     symhaliased = (Symbolhandle) 0;
	long             makextxinv = 0, makecoefs = 0, makexvars, makedfs = 0;
	long             makeyvar = 0, makewts = 0, makeparam = 0;
	long             makestrmodel = 0, makebitmodel = 0, maketermnames = 0;
	long             makescale = 0, makesigmahat = 0, makealiased = 0;
	long             makelink = 0, makedistrib = 0;
	long             nargs = NARGS(list);
	long             nvars = (long) NVARS, nregx;
	long             ndata, ny = (long) NY;
	long             nterms;
	long             ncomps = 1, icomps = 0, ncols = 0;
	long             nmissing = 0;
	long             colno = 0, i, j, iterm, itermj;
	long             nkeys = NKeys(ModelinfoKeys);
	keywordListPtr   keys = ModelinfoKeys;
	long             startKey, keyStatus;
	double         **coefs = (double **) 0;
	double          *regss;
	double         **regx = (double **) 0, *xij;
	double         **misswts = (double **) 0;
	double          *wts, dferror, *sserror;
	double          *bitmodel;
	double           missval = 0;
	long             xvarsonly, aliased = 0;
	char            *keyword;
	char           **strmodel = (char **) 0, **strmodel1 = (char **) 0;
	char            *notavailable = "WARNING: %s not available; set to NULL";
	char             modelfor[30];
	WHERE("xvariables");
	TRASH(NTRASH,errorExit);
	
	xvarsonly = (strcmp(FUNCNAME,"xvariables") == 0);
	sprintf(modelfor, "model for %s()", FUNCNAME);

	if (xvarsonly)
	{
		if (nargs > 2)
		{
			badNargs(FUNCNAME,-2);
			goto errorExit;
		}

		modelarg = COMPVALUE(list, 0);
		keyword = isKeyword(COMPVALUE(list, nargs - 1));
		if (nargs == 2)
		{			
			if (!argOK(modelarg, 0, 1))
			{
				goto errorExit;
			}
			else if (isKeyword(modelarg))
			{
				sprintf(OUTSTR,
						"ERROR: only one keyword phrase allowed on %s()",
						FUNCNAME);
				goto errorExit;
			}
		} /*if (nargs == 2)*/
		else if (keyword != (char *) 0)
		{
			modelarg = (Symbolhandle) 0;
		}
		
		if (keyword != (char *) 0 && strncmp(keyword,"miss", 4) != 0 ||
			nargs == 2 && !keyword)
		{
			sprintf(OUTSTR,
					"ERROR: last argument to %s() must be 'missing:RealVal'",
					FUNCNAME);
			goto errorExit;
		}
		makexvars = 1;
	} /*if (xvarsonly)*/
	else
	{ /*modelinfo()*/
		if (PREVMODELTYPE & (FASTANOVA | IPF))
		{
			sprintf(OUTSTR,
					"ERROR: %s() can't be used after %s()",
					FUNCNAME, glmName(PREVMODELTYPE));
			goto errorExit;
		}
		keyword = (char *) 0;
		makexvars = 0;
	}
	
	if (!xvarsonly || nargs == 2 || keyword != (char *) 0)
	{		
		unsetKeyValues(keys, nkeys);

		startKey = (modelarg != (Symbolhandle) 0) ? 1 : 0;
		keyStatus = getAllKeyValues(list, startKey, 0, keys, nkeys);
		
		if (keyStatus < 0)
		{
			goto errorExit;
		}
		else if (!xvarsonly && keyStatus == 0)
		{
			goto noRequest;
		}
		
		nargs -= keyStatus;
		
		for (i = startKey;i<nargs;i++)
		{
			arg = COMPVALUE(list,i);
			if (!argOK(arg,0, (nargs > 1) ? i+1 : 0))
			{
				goto errorExit;
			}
			if (!(keyword = isKeyword(arg)))
			{
				sprintf(OUTSTR,
						"ERROR: all arguments to %s() must be keyword phrases",
						FUNCNAME);
				goto errorExit;
			}
			else if (strcmp(NAME(arg), USEDKEYWORD) != 0)
			{
				badKeyword(FUNCNAME, keyword);
				goto errorExit;
			}
		} /*for (i = startKey;i<nargs;i++)*/

		if (realIsSet(MissingVal))
		{
			if (isMissing(MissingVal))
			{
				setMissing(missval);
			}
			else
			{
				missval = MissingVal;
			}
		} /*if (realIsSet(MissingVal))*/
		

		if (logicalIsSet(NoModelOK))
		{
			if (NoModelOK && nvars < 0)
			{
				result = NULLSYMBOL;
				goto normalExit;
			}
			if (keyStatus == 1)
			{
				goto noRequest;
			}
		} /*if (logicalIsSet(NoModelOK))*/
		
		if (logicalIsSet(GetEverything))
		{
			if (!GetEverything)
			{
				sprintf(OUTSTR,
						"ERROR: value for %s() keyword %s must be T",
						FUNCNAME, KeyName(keys,KEVERYTHING));
				goto errorExit;
			}
			makexvars = makeyvar = makextxinv = makecoefs =
				makedfs = makewts = makeparam = makestrmodel = makebitmodel =
				makelink = makedistrib = maketermnames = makescale =
				makesigmahat = makealiased = SETBYALL;
		} /*if (logicalIsSet(GetEverything))*/

		makexvars = (logicalIsSet(GetXvars)) ? GetXvars : makexvars;
		makeyvar = (logicalIsSet(GetYvar)) ? GetYvar : makeyvar;
		makextxinv = (logicalIsSet(GetXtxinv)) ? GetXtxinv : makextxinv;
		makecoefs = (logicalIsSet(GetCoefs)) ? GetCoefs : makecoefs;
		makedfs = (logicalIsSet(GetColcount)) ? GetColcount : makedfs;
		makewts = (logicalIsSet(GetWeights)) ? GetWeights : makewts;
		makeparam = (logicalIsSet(GetParam)) ? GetParam : makeparam;
		makestrmodel = (logicalIsSet(GetStrmodel)) ?
			GetStrmodel : makestrmodel;
		makebitmodel = (logicalIsSet(GetBitmodel)) ?
			GetBitmodel : makebitmodel;
		makelink = (logicalIsSet(GetLink)) ? GetLink : makelink;
		makedistrib = (logicalIsSet(GetDistrib)) ? GetDistrib : makedistrib;
		maketermnames = (logicalIsSet(GetTermnames)) ?
			GetTermnames : maketermnames;
		makescale = (logicalIsSet(GetScale)) ? GetScale : makescale;
		makesigmahat = (logicalIsSet(GetSigmahat)) ?
			GetSigmahat : makesigmahat;
		makealiased = (logicalIsSet(GetAliased)) ? GetAliased : makealiased;
	} /*if (!xvarsonly || nvars == 2 || keyword != (char *) 0)*/

	if (!xvarsonly)
	{ /* modelinfo() call */

		ncomps = (makexvars != 0) + (makeyvar != 0) + (makextxinv != 0) +
			(makecoefs != 0) + (makedfs != 0) + (makewts != 0) +
			(makeparam != 0) + (makestrmodel != 0) + (makebitmodel != 0) +
			(makelink != 0) + (makedistrib != 0)+ (maketermnames != 0) +
			(makescale != 0) + (makesigmahat != 0) + (makealiased != 0);

		if (ncomps == 0)
		{
			goto noRequest;
		}

		if (nvars < 0)
		{
			sprintf(OUTSTR, "ERROR: no active %s", modelfor);
			goto errorExit;
		}
		
		xvarsonly = (ncomps == 1 && makexvars);
		
		if (PREVMODELTYPE & ANOVA && !(PREVGLMCONTROL & UNBALANCED) ||
				 XTXINV == (double **) 0)
		{
			if (makecoefs)
			{
				if (makecoefs != SETBYALL)
				{
					sprintf(OUTSTR,notavailable,KeyName(keys,KCOEFS));
					putErrorOUTSTR();
				}
				makecoefs = MAKEVALUENULL;
			} /*if (makecoefs)*/
			if (makextxinv)
			{
				if (makextxinv != SETBYALL)
				{
					sprintf(OUTSTR,notavailable,KeyName(keys,KXTXINV));
					putErrorOUTSTR();
				}
				makextxinv = MAKEVALUENULL;
			} /*if (makextxinv)*/
		}
		
		if (makeparam && LOGITN == (double **) 0)
		{
			if (makeparam != SETBYALL)
			{
				sprintf(OUTSTR, notavailable, KeyName(keys,KPARAM));
				putErrorOUTSTR();
			}
			makeparam = MAKEVALUENULL;
		} /*if (makeparam && LOGITN == (double **) 0)*/
		
		if (makesigmahat && !(PREVMODELTYPE & ROBUSTREG))
		{
			if (makesigmahat != SETBYALL)
			{
				sprintf(OUTSTR,notavailable,KeyName(keys,KSIGMAHAT));
				putErrorOUTSTR();
			}
			makesigmahat = MAKEVALUENULL;
		} /*if (makesigmahat && !(PREVMODELTYPE & ROBUSTREG))*/
	} /*if (!xvarsonly)*/

	if (xvarsonly)
	{ /* xvariables(), no keywords except 'missing' allowed */
		if (modelarg != (Symbolhandle) 0)
		{
			/* argument must be a model */
			if (!isCharOrString(modelarg))
			{
				notCharOrString(modelfor);
				goto errorExit;
			}
			strmodel = STRING(modelarg);
		} /*if (modelarg != (Symbolhandle) 0)*/
		else if (nvars < 0)
		{ /* no model specified and no active model */
			modelarg = Lookup("STRMODEL");
			if (modelarg == (Symbolhandle) 0)
			{
				sprintf(OUTSTR,
						"ERROR: no active %s and STRMODEL is not defined",
						modelfor);
			}
			else if (!isCharOrString(modelarg))
			{
				sprintf(OUTSTR,
						"ERROR: STRMODEL is not scalar CHARACTER variable");
			}
			else
			{
				strmodel = STRING(modelarg);
			}
		}
		else if (PREVMODELTYPE & FASTANOVA)
		{
			strmodel = STRMODEL;
		}
		if (*OUTSTR)
		{
			goto errorExit;
		}
	} /*if (xvarsonly)*/
	
	if (strmodel != (char **) 0)
	{ /* we need to parse the model and generate X-variables */
		if ((strmodel1 = setStrmodel(strmodel,0)) == (char **) 0)
		{
			goto errorExit;
		}
		toTrash(strmodel1,GSTRMODEL1);
		
		initmodelparse(strmodel1);

		if (modelparse() != 0)
		{							/* parse it */
			goto errorExit;
		}
		info = getModelInfo(); /* retrieve model information */
		if ((ndata = checkVariables(info)) < 0)
		{
			goto errorExit;
		}
		nmissing = countMissing(info, &misswts);
		if (nmissing < 0)
		{ /* error */
			goto errorExit;
		}
		toTrash(misswts,GMISSWTS);

		nvars = info->nvars;
		nterms = info->nterms;
		
		nregx = 0;
		for (iterm = 0;iterm < nterms;iterm++)
		{
			nregx += buildXmatrix(iterm, (double *) 0, info);
		}
	} /*if (strmodel != (char **) 0)*/
	else
	{ /* use information from current model */
		info = MODELINFO;
		strmodel = STRMODEL;
		misswts = MISSWTS;
		ndata = (long) NDATA;
		nregx = (long) NREGX;
		nterms = (long) NTERMS;
		nmissing = ndata - (long) NOTMISSING;
	} /*if (strmodel != (char **) 0){...}else{...}*/

	discardScratch(strmodel1,GSTRMODEL1,char);

	if (ncomps == 1)
	{
		result = (makestrmodel || maketermnames || makelink || makedistrib) ?
			Install(SCRATCH, CHAR) : RInstall(SCRATCH,0);
	}
	else
	{
		result  = StrucInstall(SCRATCH, ncomps);
	}
	
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	
	if (makexvars)
	{
		if (ncomps == 1)
		{
			symhxvars = result;
		}
		else
		{
			symhxvars = COMPVALUE(result,icomps++) = Makereal(0);
			if (symhxvars == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			setNAME(symhxvars,KeyName(keys,KXVARS));
		}
		if (makexvars == MAKEVALUENULL)
		{
			setTYPE(symhxvars, NULLSYM);
		}
		else
		{
			TMPHANDLE = mygethandle(ndata*nregx*sizeof(double));
			setDATA(symhxvars,(double **) TMPHANDLE);
			if (TMPHANDLE == (char **) 0)
			{
				goto errorExit;
			}
		
			regx = DATA(symhxvars);

			setDIM(symhxvars,1,ndata);
			if (nregx > 1)
			{
				setNDIMS(symhxvars ,2);
				setDIM(symhxvars ,1,ndata);
				setDIM(symhxvars ,2,nregx);
			}	
			for (iterm = 0;iterm < nterms;iterm++)
			{
				colno += buildXmatrix(iterm, *regx + colno*ndata, info);
			}

			if (misswts != (double **) 0)
			{
				if (!isMissing(missval))
				{
					sprintf(OUTSTR,
							"WARNING: X-variables for cases with missing data set to %g",
							missval);
				}
				else
				{
					sprintf(OUTSTR,
							"WARNING: X-variables for cases with missing data set to MISSING");
				}
				putErrorOUTSTR();
			
				for (i=0;i<ndata;i++)
				{
					if ((*misswts)[i] == 0.0)
					{
						xij = *regx + i;
						for (j=0;j<nregx;j++)
						{
							if (isMissing(missval))
							{
								setMissing(*xij);
							}
							else
							{
								*xij = missval;
							}
							xij += ndata;
						}
					}
				} /*for (i=0;i<ndata;i++)*/

				if (misswts != MISSWTS)
				{
					discardScratch(misswts,GMISSWTS,double);
				}
			} /*if (misswts != (double **) 0)*/
		}		
	} /*if (makexvars)*/

	if (makeyvar)
	{
		if (ncomps == 1)
		{
			symhyvar = result;
		}
		else
		{
			symhyvar = COMPVALUE(result,icomps++) = Makereal(0);
			if (symhyvar == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			setNAME(symhyvar,KeyName(keys,KYVAR));
		}
		if (makeyvar == MAKEVALUENULL)
		{
			setTYPE(symhyvar, NULLSYM);
		}
		else
		{
			TMPHANDLE = myduphandle((char **) Y);
			setDATA(symhyvar, (double **) TMPHANDLE);
			if (TMPHANDLE == (char **) 0)
			{
				goto errorExit;
			}

			setNDIMS(symhyvar, (ny == 1) ? 1 : 2);
			setDIM(symhyvar, 1, ndata);
			if (ny > 1)
			{
				setDIM(symhyvar, 2, ny);
			}
		}
	}/*if (makeyvar)*/
	
	if (makeparam)
	{
		if (ncomps == 1)
		{
			symhparam = result;
		}
		else
		{
			symhparam = COMPVALUE(result,icomps++) = Makereal(0);
			if (symhparam == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			setNAME(symhparam,KeyName(keys,KPARAM));
		}
		if (makeparam == MAKEVALUENULL)
		{
			setTYPE(symhparam, NULLSYM);
		}
		else
		{
			TMPHANDLE = myduphandle((char **) LOGITN);
			setDATA(symhparam,(double **) TMPHANDLE);
		
			if (TMPHANDLE == (char **) 0)
			{
				goto errorExit;
			}
			setDIM(symhparam,1,ndata);
		}
	} /*if (makeparam)*/

	if (makextxinv)
	{
		if (ncomps == 1)
		{
			symhxtxinv = result;
		}
		else
		{
			symhxtxinv = COMPVALUE(result,icomps++) = Makereal(0);
			if (symhxtxinv == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			setNAME(symhxtxinv,KeyName(keys,KXTXINV));
		}
		if (makextxinv == MAKEVALUENULL)
		{
			setTYPE(symhxtxinv, NULLSYM);
		}
		else
		{
			TMPHANDLE = myduphandle((char **) XTXINV);
			setDATA(symhxtxinv,(double **) TMPHANDLE);
			if (TMPHANDLE == (char **) 0)
			{
				goto errorExit;
			}

			setNDIMS(symhxtxinv, 2);
			setDIM(symhxtxinv, 1, nregx);
			setDIM(symhxtxinv, 2, nregx);
		}
	} /*if (makextxinv)*/

	if (makecoefs)
	{
		if (ncomps == 1)
		{
			symhcoefs = result;
		}
		else
		{
			symhcoefs = COMPVALUE(result,icomps++) = Makereal(0);
			if (symhcoefs == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			setNAME(symhcoefs,KeyName(keys,KCOEFS));
		}
		if (makecoefs == MAKEVALUENULL)
		{
			setTYPE(symhcoefs, NULLSYM);
		}
		else
		{
			TMPHANDLE = mygethandle(ny * nregx * sizeof(double));
			setDATA(symhcoefs, (double **) TMPHANDLE);
			if (TMPHANDLE == (char **) 0)
			{
				goto errorExit;
			}

			coefs = DATA(symhcoefs);
			setNDIMS(symhcoefs, (ny > 1) ? 2 : 1);
			setDIM(symhcoefs, 1, nregx);
			if (ny > 0)
			{
				setDIM(symhcoefs, 2, ny);
			}
		
			/*
			   951030 Modified code to directly copy coefficients rather
			   than "pick them off'
			   */
			doubleCopy(*REGCOEF, *coefs, ny*nregx);
			for (j = 0; j < nregx; j ++)
			{
				if ((*REGSS)[j] < -.5)
				{
					aliased++;
				}
			}
			if (aliased > 0)
			{
				sprintf(OUTSTR,
						"WARNING: coefficient%s of %ld aliased X-variable%s set to 0",
						(aliased*ny > 1) ? "s" : "", aliased,
						(aliased > 1) ? "s" : "");
				putErrorOUTSTR();
			} /*if (aliased > 0)*/
		}
	} /*if (makecoefs)*/

	if (makealiased)
	{
		if (ncomps == 1)
		{
			symhaliased = result;
		}
		else
		{
			symhaliased = COMPVALUE(result,icomps++) = Makereal(0);
			if (symhaliased == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			setNAME(symhaliased,KeyName(keys,KALIASED));
		}
		if (makealiased == MAKEVALUENULL)
		{
			setTYPE(symhaliased, NULLSYM);
		}
		else
		{
			TMPHANDLE = mygethandle(nregx * sizeof(double));
			setDATA(symhaliased, (double **) TMPHANDLE);
			if (TMPHANDLE == (char **) 0)
			{
				goto errorExit;
			}

			setTYPE(symhaliased, LOGIC);
			setNDIMS(symhaliased, 1);
			setDIM(symhaliased, 1, nregx);
			regss = *REGSS;
			for (i = 0; i < nregx; i++)
			{
				DATAVALUE(symhaliased, i) =
					(regss[i] < -0.5) ? 1.0 : 0.0;
			} /*for (i = 0; i < nregx; i++)*/
		}		
	} /*if (makealiased)*/
		
	if (makescale)
	{
		if (ncomps == 1)
		{
			symhscale = result;
		}
		else
		{
			symhscale = COMPVALUE(result,icomps++) = Makereal(0);
			if (symhscale == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			setNAME(symhscale,KeyName(keys,KSCALE));
		}
		if (makescale == MAKEVALUENULL)
		{
			setTYPE(symhscale, NULLSYM);
		}
		else
		{
			TMPHANDLE = mygethandle(ny * sizeof(double));
			setDATA(symhscale, (double **) TMPHANDLE);
			if (TMPHANDLE == (char **) 0)
			{
				goto errorExit;
			}

			setNDIMS(symhscale, 1);
			setDIM(symhscale, 1, ny);
			dferror = (*DF)[nterms];
			sserror = *SS + nterms;
			if (PREVGLMCONTROL & NONNORMALDIST)
			{
				DATAVALUE(symhscale, 0) = PREVGLMSCALE;
			}
			else if (dferror != 0)
			{
				for (i = 0; i < ny; i++)
				{
					DATAVALUE(symhscale, i) = sqrt(*sserror/dferror);
					sserror += (nterms+1)*(ny + 1);
				} /*for (i = 0; i < ny; i++)*/
			}
			else
			{
				for (i = 0; i < ny; i++)
				{
					setMissing(DATAVALUE(symhscale, i));
				} /*for (i = 0; i < ny; i++)*/
			}
		}
	} /*if (makescale)*/

	if (makedfs)
	{
		if (ncomps == 1)
		{
			symhcolcounts = result;
		}
		else
		{
			symhcolcounts = COMPVALUE(result,icomps++) = Makereal(0);
			if (symhcolcounts == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			setNAME(symhcolcounts,KeyName(keys,KCOLCOUNT));
		}
		if (makedfs == MAKEVALUENULL)
		{
			setTYPE(symhcolcounts, NULLSYM);
		}
		else
		{
			TMPHANDLE = mygethandle(nterms * sizeof(double));
			setDATA(symhcolcounts,(double **) TMPHANDLE);
			if (TMPHANDLE == (char **) 0)
			{
				goto errorExit;
			}
		
			setDIM(symhcolcounts,1,nterms);
			for (iterm = 0;iterm < nterms;iterm++)
			{
				DATAVALUE(symhcolcounts,iterm) =
					(double) buildXmatrix(iterm, (double *) 0, info);
			} /*for (iterm = 0;iterm < nterms;iterm++)*/
		}		
	} /*if (makedfs)*/

	if (makewts)
	{
		if (ncomps == 1)
		{
			symhwts = result;
		}
		else
		{
			symhwts = COMPVALUE(result,icomps++) = Makereal(0);
			if (symhwts == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			setNAME(symhwts,KeyName(keys,KWEIGHTS));
		}
		if (makewts == MAKEVALUENULL)
		{
			setTYPE(symhwts, NULLSYM);
		}
		else
		{
			if (PREVMODELTYPE & USEWEIGHTS)
			{
				TMPHANDLE = myduphandle((char **) ALLWTS);
			}
			else
			{
				TMPHANDLE = mygethandle(ndata * sizeof(double));
			}
			setDATA(symhwts,(double **) TMPHANDLE);
		
			if (TMPHANDLE == (char **) 0)
			{
				goto errorExit;
			}
			wts = DATAPTR(symhwts);
			for (i=0;i<ndata;i++)
			{
				wts[i] = (PREVMODELTYPE & USEWEIGHTS) ? wts[i]*wts[i] : 1.0;
			}		
			setDIM(symhwts,1,ndata);
		}
		
	} /*if (makewts)*/

	if (makestrmodel)
	{
		if (ncomps == 1)
		{
			symhstrmodel = result;
		}
		else
		{
			symhstrmodel = COMPVALUE(result,icomps++) = Makesymbol(CHAR);
			if (symhstrmodel == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			setNAME(symhstrmodel,KeyName(keys,KSTRMODEL));
		}
		TMPHANDLE = myduphandle(STRMODEL);
		setSTRING(symhstrmodel, TMPHANDLE);
		if (TMPHANDLE == (char **) 0)
		{
			goto errorExit;
		}
		setNDIMS(symhstrmodel, 1);
		setDIM(symhstrmodel, 1, 1);
	} /*if (makestrmodel)*/

	if (makebitmodel)
	{
		if (ncomps == 1)
		{
			symhbitmodel = result;
		}
		else
		{
			symhbitmodel = COMPVALUE(result,icomps++) = Makereal(0);
			if (symhbitmodel == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			setNAME(symhbitmodel,KeyName(keys,KBITMODEL));
		}
		ncols = (nvars <= 32) ? 1 : ((nvars <= 64) ? 2 : 3);
		TMPHANDLE = mygethandle(nterms * ncols * sizeof(double));
		setDATA(symhbitmodel, (double **) TMPHANDLE);
		if (TMPHANDLE == (char **) 0)
		{
			goto errorExit;
		}

		setNDIMS(symhbitmodel, (ncols == 1) ? 1 : 2);
		setDIM(symhbitmodel, 1, nterms);
		if (ncols > 1)
		{
			setDIM(symhbitmodel, 2, ncols);
		}
		
		bitmodel = DATAPTR(symhbitmodel);
		for (iterm = 0;iterm < nterms;iterm++)
		{
			itermj = iterm; /* iterm + j * nterms */
			for (j=0;j<ncols;j++)
			{
				bitmodel[itermj] = (double) (*info->model)[iterm][j];
				itermj += nterms;
			}
		} /*for (iterm = 0;iterm < nterms;iterm++)*/
	} /*if (makebitmodel)*/

	if (makelink)
	{
		if (ncomps == 1)
		{
			symhlink = result;
		}
		else
		{
			symhlink = COMPVALUE(result,icomps++) = Makesymbol(CHAR);
			if (symhlink == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			setNAME(symhlink,KeyName(keys,KLINK));
		}
		if (makelink == MAKEVALUENULL)
		{
			setTYPE(symhlink, NULLSYM);
		}
		else
		{
			char    *linkname = linkName(PREVGLMCONTROL);
		
			TMPHANDLE = mygethandle(strlen(linkname) + 1);
			setSTRING(symhlink, TMPHANDLE);
			if (TMPHANDLE == (char **) 0)
			{
				goto errorExit;
			}
			strcpy(*TMPHANDLE, linkname);

			setNDIMS(symhlink, 1);
			setDIM(symhlink, 1, 1);
		}
		
	} /*if (makelink)*/

	if (makedistrib)
	{
		if (ncomps == 1)
		{
			symhdistrib = result;
		}
		else
		{
			symhdistrib = COMPVALUE(result,icomps++) = Makesymbol(CHAR);
			if (symhdistrib == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			setNAME(symhdistrib,KeyName(keys,KDISTRIB));
		}
		if (makedistrib == MAKEVALUENULL)
		{
			setTYPE(symhdistrib, NULLSYM);
		}
		else
		{
			char    *distribname = distName(PREVGLMCONTROL);
		
			TMPHANDLE = mygethandle(strlen(distribname) + 1);
			setSTRING(symhdistrib, TMPHANDLE);
			if (TMPHANDLE == (char **) 0)
			{
				goto errorExit;
			}
			strcpy(*TMPHANDLE, distribname);
			setNDIMS(symhdistrib, 1);
			setDIM(symhdistrib, 1, 1);
		}
		
	} /*if (makedistrib)*/

	if (maketermnames)
	{
		if (ncomps == 1)
		{
			symhtermnames = result;
		}
		else
		{
			symhtermnames = COMPVALUE(result,icomps++) = Makesymbol(CHAR);
			if (symhtermnames == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			setNAME(symhtermnames,KeyName(keys,KTERMNAMES));
		}
		TMPHANDLE = myduphandle(TERMNAMES);
		setSTRING(symhtermnames, TMPHANDLE);
		if (TMPHANDLE == (char **) 0)
		{
			goto errorExit;
		}
		setNDIMS(symhtermnames, 1);
		setDIM(symhtermnames, 1, nterms+1);
	} /*if (maketermnames)*/

	if (makesigmahat)
	{
		if (ncomps == 1)
		{
			symhsigmahat = result;
		}
		else
		{
			symhsigmahat = COMPVALUE(result,icomps++) = Makereal(0);
			if (symhsigmahat == (Symbolhandle) 0)
			{
				goto errorExit;
			}
			setNAME(symhsigmahat,KeyName(keys,KSIGMAHAT));
		}
		if (makesigmahat == MAKEVALUENULL)
		{
			setTYPE(symhsigmahat, NULLSYM);
		}
		else
		{
			TMPHANDLE =  mygethandle(sizeof(double));
			setDATA(symhsigmahat, (double **) TMPHANDLE);
			if (TMPHANDLE == (char **) 0)
			{
				goto errorExit;
			}

			setNDIMS(symhsigmahat, 1);
			setDIM(symhsigmahat, 1, 1);
			DATAVALUE(symhsigmahat, 0) = CURRENTRSC;
		}		
	} /*if (makesigmahat)*/

  normalExit:
	emptyTrash();
	
	return (result);
	
  noRequest:
	sprintf(OUTSTR,
			"ERROR: nothing specified for %s() to return",
			FUNCNAME);
	/* fall through*/

  errorExit:
	putErrorOUTSTR();
	emptyTrash();
	
	return (0);
} /*xvariables()*/
#ifdef MW_CW_New
#pragma global_optimizer off
#endif /*MW_CW_New*/

enum xvarsrowsScratch
{
	GXROW = 0,
	NTRASHxvarsrows
};
Symbolhandle xvarsrows(Symbolhandle list)
{
	Symbolhandle    result, symhVariates = (Symbolhandle) 0;
	Symbolhandle    symhFactors = (Symbolhandle) 0;
	double        **xrowH, *xrow;
	double         *x = (double *) 0, *factorVals = (double *) 0;
	double         *xk, *factork, *rowx, *rowxk;
	double          xvals[MAXVARS];
	long            vardims[2], factordims[2];
	long            nclasses[MAXDIMS];
	long            i, j, k;
	long            incVars, incFactors;
	long            nVariateVals = 0, nFactorVals = 0, npoints;
	long            nvars = (long) NVARS;
	long            nregx = (long) NREGX, nfactors = (long) NFACTORS;
	long            nargs = NARGS(list);
	long            nvariates = nvars - nfactors;
	WHERE("xvarsrows");
	TRASH(NTRASHxvarsrows, errorExit);
	
	if (PREVMODELTYPE == NOMODEL)
	{
		sprintf(OUTSTR, "ERROR: no active GLM model");
	}
	else if (nvars == 0)
	{
		sprintf(OUTSTR,
				"ERROR: %s has no variates or factors", *STRMODEL);
	}
	if (*OUTSTR)
	{
		goto errorExit;
	}
	if (nargs > 2)
	{
		badNargs(FUNCNAME, -2);
		goto errorExit;
	}
	
	symhVariates = COMPVALUE(list, 0);
	if (nvariates > 0)
	{
		if (!argOK(symhVariates, 0, (nargs > 1) ? 1 : 0))
		{
			goto errorExit;
		}
		if (!isMatrix(symhVariates,vardims) || TYPE(symhVariates) != REAL)
		{
			sprintf(OUTSTR,
					"ERROR: variate values for %s() must be REAL vector or matrix",
					FUNCNAME);
		}
		else if (anyMissing(symhVariates))
		{
			sprintf(OUTSTR,
					"ERROR: variate values for %s() have MISSING values",
					FUNCNAME);
		}
		else
		{
			if (vardims[1] > 1 || nvariates == 1)
			{
				k = vardims[1];
				nVariateVals = vardims[0];
			}
			else
			{/* x is vector, NVARS > 1 */
				k = vardims[0];
				nVariateVals = 1;
			}

			if (k != nvariates)
			{
				strcpy(OUTSTR,
					   "ERROR: number of variate values does not match model");
			}
		}		
	} /*if (nvariates > 0)*/
	else if (!isNull(symhVariates))
	{
		sprintf(OUTSTR,
				"ERROR: No variates in %s and argument 1 to %s() not NULL",
				*STRMODEL, FUNCNAME);
	}
	if (*OUTSTR)
	{
		goto errorExit;
	}

	if (nfactors > 0)
	{
		/* Check values for factors */
		if (nargs == 1)
		{
			sprintf(OUTSTR,
					"ERROR:  There are factors in %s but no factor levels provided",
					*STRMODEL);
		} /*if (nargs == 1)*/
		else
		{			
			symhFactors = COMPVALUE(list, 1);
			if (nfactors > 0)
			{
				if (!argOK(symhFactors, 0, 2))
				{
					goto errorExit;
				}
				if (!isMatrix(symhFactors,factordims) ||
				   TYPE(symhFactors) != REAL)
				{
					sprintf(OUTSTR,
							"ERROR: factor levels for %s() must be REAL vector or matrix",
							FUNCNAME);
				}
				else if (anyMissing(symhFactors))
				{
					sprintf(OUTSTR,
							"ERROR: factor levels for %s() have MISSING values",
							FUNCNAME);
				}
				else
				{
					if (factordims[1] > 1 || nfactors == 1)
					{
						k = factordims[1];
						nFactorVals = factordims[0];
					}
					else
					{/* vector, NVARS > 1 */
						k = factordims[0];
						nFactorVals = 1;
					}

					if (k != nfactors)
					{
						strcpy(OUTSTR,
							   "ERROR: number of factor values does not match model");
					}
					else
					{						
						j = 0;
						for (i = 0; i < nvars; i++)
						{
							if (NCLASSES[i] > 0)
							{
								nclasses[j++] = NCLASSES[i];
							}
						} /*for (i = 0; i < nvars; i++)*/
						
						factorVals = DATAPTR(symhFactors);
						for (j = 0; j < nfactors; j++)
						{
							for (i = 0; i < nFactorVals; i++)
							{
								if (factorVals[i] != floor(factorVals[i]) ||
									factorVals[i] <= 0)
								{
									sprintf(OUTSTR,
											"ERROR: factor levels for %s() must be positive integers",
											FUNCNAME);
								}
								else if (factorVals[i] > nclasses[j])
								{
									sprintf(OUTSTR,
											"ERROR: level for factor %ld > %ld",
											j+1, nclasses[j]);
								}
								if (*OUTSTR)
								{
									goto errorExit;
								}
							}
							factorVals += nFactorVals; 
						} /*for (j = 0; j < nfactors; j++)*/
					}					
				}				
			} /*if (nfactors > 0)*/
			else if (!isNull(symhFactors))
			{
				sprintf(OUTSTR,
						"ERROR: No factors in %s and argument 2 to %s() not NULL",
						*STRMODEL, FUNCNAME);
			}
		} /*if (nargs == 1){}else{}*/
	} /*if (nfactors > 0 || nargs == 2)*/

	if (*OUTSTR)
	{
		goto errorExit;
	}
	
	if (nVariateVals > 1 && nFactorVals > 1 && nVariateVals != nFactorVals)
	{
		sprintf(OUTSTR,
				"ERROR: numbers of variate value sets and factor level sets do not match");
		goto errorExit;
	}
	
	npoints = (nVariateVals > nFactorVals) ? nVariateVals : nFactorVals;

	if (npoints > 1 && !getScratch(xrowH, GXROW, nregx, double))
	{
		goto errorExit;
	}

	result = RInstall(SCRATCH, npoints*nregx);
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	if (npoints == 1)
	{
		xrowH = DATA(result);
	}

	setNDIMS(result, 2);
	setDIM(result, 1, npoints);
	setDIM(result, 2, nregx);

	if (nvariates > 0)
	{
		x = DATAPTR(symhVariates);
	}
	if (nfactors > 0)
	{
		factorVals = DATAPTR(symhFactors);
	}

	xrow = *xrowH;
	rowx = DATAPTR(result);
	
	incVars = (nVariateVals > 1) ? 1 : 0;
	incFactors = (nFactorVals > 1) ? 1 : 0;

	for (k = 0;k < npoints;k++)
	{
		xk = x;  /* k-th row of variates */
		factork = factorVals;  /* k-th row of factors */
		rowxk = rowx;
		for (i = 0; i < nvars; i++)
		{
			if (NCLASSES[i] < 0)
			{
				xvals[i] = *xk;
				xk += nVariateVals;
			}
			else
			{
				xvals[i] = *factork;
				factork += nFactorVals;
			}
		} /*for (i = 0; i < nvars; i++)*/

		(void) buildXrow(xvals, xrow, MODELINFO);

		if (npoints > 1)
		{
			for (i = 0; i < nregx; i++)
			{
				*rowxk = xrow[i];
				rowxk += npoints;
			}
		}
		x += incVars;
		factorVals += incFactors;
		rowx++;
	} /*for (k = 0;k < npoints;k++)*/
	
	emptyTrash();
	return (result);

  errorExit:
	putErrorOUTSTR();
	emptyTrash();
	
	return (0);
} /*xvarsrows()*/

