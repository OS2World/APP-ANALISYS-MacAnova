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
#pragma segment Trans
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
#define lgamma mylgamma /*960220*/

#ifndef	HUGEDBL
#define HUGEDBL 1e38
#endif /*HUGEDBL*/


/* 
  These codes should be consecutive integers in the same order as operations
  in opEntry1
*/
enum tranDispatchCodes
{
	INOTFOUND = 0,	
	ISQRT,
	IEXP,
	ILOG,
	ILOG10,
	IABS,
	ISIN,
	ICOS,
	ITAN,
	ISINH,
	ICOSH,
	ITANH,
	IATANH,
	IATAN,
	IATAN2,
	IASIN,
	IACOS,
	IHYPOT,
	ILGAMMA,
	IFLOOR,
	ICEILING,
	IROUND,
	INBITS,
	OPBASE = ISQRT /*code of first function in list*/
}; /*tranDispatchCodes*/

static double     		Angles2Radians;

static double commonLog(double x)
{
	return (log10(x));
} /*commonLog()*/

static double atanh1(double x)
{
	return(0.5*log((1 + x)/(1 - x)));
} /*atanh1()*/

/*
  970220 modified so it returns an integer when param <= 1
*/
static double roundit(double x,double param)
{
	double      value = floor(param * fabs(x) + .5) / param;
	WHERE("round");

	/* ensure value is integer when you know it should be */
	value = (param <= 1.0) ? floor(value + .01) : value;
	return ((x > 0.0) ? value : -value);
} /*roundit()*/

/*
   960216
   Following wrappers for library math functions were installed because
   of problems using function pointers to such functions when compiling
   for a 68K Mac using Codewarrior.  No such problems appeared when
   compiling for a Power Mac

   The ones for which there are not wrappers (e.g. pythag) are coded
   or wrapped elsewhere
*/

#if defined(MW_CW) && !defined(powerc)
static double mysqrt(double x)
{
	return (sqrt(x));
} /*mysqrt()*/
#undef sqrt
#define sqrt mysqrt

static double myexp(double x)
{
	return (exp(x));
} /*myexp()*/
#undef exp
#define exp myexp

static double mylog(double x)
{
	return (log(x));
} /*mylog()*/
#undef log
#define log mylog

static double myfabs(double x)
{
	return (fabs(x));
} /*myfabs()*/
#undef fabs
#define fabs myfabs

static double mysin(double x)
{
	return (sin(x));
} /*mysin()*/
#undef sin
#define sin mysin

static double mycos(double x)
{
	return (cos(x));
} /*mycos()*/
#undef cos
#define cos mycos

static double mytan(double x)
{
	return (tan(x));
} /*mytan()*/
#undef tan
#define tan mytan

static double mysinh(double x)
{
	return (sinh(x));
} /*mysinh()*/
#undef sinh 
#define sinh mysinh

static double mycosh(double x)
{
	return (cosh(x));
} /*mycosh()*/
#undef cosh 
#define cosh mycosh

static double mytanh(double x)
{
	return (tanh(x));
} /*mytanh()*/
#undef tanh 
#define tanh mytanh

static double myatan(double x)
{
	return (atan(x));
} /*myatan()*/
#undef atan 
#define atan myatan

static double myatan2(double x1, double x2)
{
	return (atan2(x1, x2));
} /*myatan2()*/
#undef atan2
#define atan2 myatan2

static double myasin(double x)
{
	return (asin(x));
} /*myasin()*/
#undef asin 
#define asin myasin

static double myacos(double x)
{
	return (acos(x));
} /*myacos()*/
#undef acos 
#define acos myacos

static double myfloor(double x)
{
	return (floor(x));
} /*myfloor()*/
#undef floor 
#define floor myfloor

static double myceil(double x)
{
	return (ceil(x));
} /*myceil()*/
#undef ceil 
#define ceil myceil

#endif /*MW_CW*/

#ifdef UNDEFINED__
static double nbits(double x)
{
	register unsigned long     ulx = (unsigned long) x;
	register int               n = 0;

	while(ulx != 0)
	{
		n += 1 & ulx;
		ulx >>= 1;
	}
	return ((double) n);
}
#else /*UNDEFINED__*/
/* slightly faster version of preceding */
static char bitcounts[] = 
{
	0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4
}; /*bitcounts[]*/

static double nbits(double x)
{
	register unsigned long     ulx;
	register char              n = '\0';
	register char             *bc = bitcounts;
	
	for(ulx = (unsigned long) x; ulx != 0; ulx >>= 4)
	{
		n += bc[ulx & 0x0f];
	}
	return ((double) n);
} /*nbits()*/
	
#endif /*UNDEFINED__*/

typedef struct opEntry1
{
	char *name;
	enum tranDispatchCodes iop;
	short nargs;
	double (*f)(double);
} opEntry1;

static opEntry1  Ops[] =
{
	{"sqrt",    ISQRT,    1,    sqrt},
	{"exp",     IEXP,     1,    exp},
	{"log",     ILOG,     1,    log},
	{"log10",   ILOG10,   1,    commonLog},
	{"abs",     IABS,     1,    fabs},
	{"sin",     ISIN,     1,    sin},
	{"cos",     ICOS,     1,    cos},
	{"tan",     ITAN,     1,    tan},
	{"sinh",    ISINH,    1,    sinh},
	{"cosh",    ICOSH,    1,    cosh},
	{"tanh",    ITANH,    1,    tanh},
	{"atanh",   IATANH,   1,    atanh1},
	{"atan",    IATAN,   -2,    atan},
	{"atan2",   IATAN2,   2,    (double (*) (double)) atan2},
	{"asin",    IASIN,    1,    asin},
	{"acos",    IACOS,    1,    acos},
	{"hypot",   IHYPOT,   2,    (double (*) (double)) pythag},
	{"lgamma",  ILGAMMA,  1,    lgamma},
	{"floor",   IFLOOR,   1,    floor},
	{"ceiling", ICEILING, 1,    ceil},
	{"round",   IROUND,  -2,    (double (*) (double)) roundit},
	{"nbits",   INBITS,   1,    nbits},
	{(char *) 0, (enum tranDispatchCodes) 0, 0,    (double (*) (double)) 0}
}; /*Ops[]*/

#define trash  NAMEFORTRASH
#ifndef MAXANGLE
#define MAXANGLE  5e6 /* radians/PI */
#endif /*MAXANGLE*/

/*
  MAXINTEGER should be largest integer such that
  MAXINTEGER and -MAXINTEGER are exactly represented
  and
  floor(MAXINTEGER - epsilon) == MAXINTEGER - 1 and
      ceiling(-MAXINTEGER + epsilon) == -MAXINTEGER  + 1, where
  epsilon is such that MAXINTEGER - epsilon != MAXINTEGER
  and -MAXINTEGER + epsilon != -MAXINTEGER
  If NOVALUES is defined (header values.h not available), MAXINTEGER should
  be defined in platform.h

  981106 Modified intialization of MAXINTEGER to correct bug in BC version
  */

#ifndef MAXINTEGER
#ifdef DSIGNIF
#define MAXINTEGER -1.0
#else /*DSIGNIF*/
#define MAXINTEGER 4503599627370496.0 /*2^52, may not be appropriate*/
#endif /*DSIGNIF*/
#endif /*MAXINTEGER*/

static double MaxAngle;
static double MaxInteger = MAXINTEGER;

static Symbolhandle doTrans(Symbolhandle arg1,Symbolhandle arg2,
							double *params, unsigned long control,
							unsigned long *status)
{
	Symbolhandle     result = (Symbolhandle) 0, tmpsymh = (Symbolhandle) 0;
	long            i, tot1;
	long            nargs = 1;
	long            iop;
	int             noGood, anyBad = 0;
	long            toAngles, fromAngles, checkInfinity;
	long            ndims;
	double        (*f1)(double), (*f2)(double,double);
	double          x1, x2, z, absz;
	char          **labelsH;
	WHERE("doTrans");
	
#ifdef DSIGNIF
	if (MaxInteger < 0)
	{
		int      n = DSIGNIF - 1;

		MaxInteger = 1.0;
		while (n-- > 0)
		{
			MaxInteger *= 2.0;
		}
	}
#endif /*DSIGNIF*/
	iop = control & OPMASK;

	f1 = Ops[iop-OPBASE].f;
	f2 = (double (*) (double, double)) f1;
	
	toAngles = (iop == IACOS) || (iop == IASIN) ||
		(iop == IATAN) || (iop == IATAN2);
	fromAngles = (iop == ICOS) || (iop == ISIN) || (iop == ITAN);
	checkInfinity = (iop == IEXP || iop == ICOSH || iop == ISINH ||
					 iop == IHYPOT);
	if(iop == IROUND)
	{
		x2 = intpow(10.0,params[0]);
		nargs = 2;
	}
	else if(iop == IATAN2 || iop == IHYPOT)
	{
		nargs = 2;
	}
	
	/* find total number of data values & copy dimensions */
	tot1 = symbolSize(arg1);

	/* set up result array */
	if(control & REUSELEFT)
	{
		result = arg1;
	}
	else if(control & REUSERIGHT)
	{
		result = arg2;
	}
	else
	{
		result = (tot1 > 0) ?
			RInstall(SCRATCH,tot1) : Install(NULLSCRATCH, NULLSYM);
	}
	
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}

	if (tot1 > 0)
	{
		if (HASLABELS(arg1))
		{
			tmpsymh = arg1;
		}
		else if (nargs == 2 && iop != IROUND && HASLABELS(arg2))
		{
			tmpsymh = arg2;
		}
		setNCLASS(result, -1);	/* make sure it's a variate */

		ndims = NDIMS(arg1);
		if(nargs == 2 && iop != IROUND && ndims > NDIMS(arg2))
		{						/* atan2 or hypot */
			ndims = NDIMS(arg2);
		}
	
		Setdims(result, ndims, &DIMVAL(arg1,1));
	
		if (tmpsymh != (Symbolhandle) 0)
		{ /*tmpsymh had labels to be used used*/
			long         lengths[MAXDIMS], dims[MAXDIMS];
			char        *labs[MAXDIMS];
			Symbolhandle trash = (Symbolhandle) 0;
			
			if (tmpsymh != result || ndims != NDIMS(tmpsymh))
			{
				for (i = 0; i < ndims; i++)
				{
					dims[i] = DIMVAL(result, i+1);
				}
				
				getAllLabels(tmpsymh, labs, lengths, (long *) 0);
				
				if ((trash = GarbInstall(1)) == (Symbolhandle) 0 ||
					(labelsH = createLabels(ndims, lengths)) == (char **) 0)
				{
					emptyTrash();
					goto errorExit;
				}
				toTrash(labelsH, 0); /*avoid memory leakage*/
				getAllLabels(tmpsymh, labs, (long *) 0, (long *) 0);
				buildLabels(labelsH, labs, dims, ndims);
				unTrash(0);
				if (!setLabels(result, labelsH))
				{
					mydisphandle(labelsH);
					goto errorExit;
				}
				emptyTrash();
			} /*if (tmpsymh != result || ndims != NDIMS(tmpsymh))*/			
		} /*if (tmpsymh != (Symbolhandle) 0)*/
		
		for (i = 0; i < tot1; i++)
		{
#ifdef MACINTOSH
			if(checktime(i) && interrupted(DEFAULTTICKS) != INTNOTSET)
			{
				goto errorExit;
			}
#endif /*MACINTOSH*/
			x1 = DATAVALUE(arg1,i);
			noGood = 0;
			if (isMissing(x1) ||
				(iop != IROUND && nargs == 2 &&
				 (x2 = DATAVALUE(arg2,i),isMissing(x2))))
			{
				*status |= FOUNDMISSING;
				noGood = 1;
			}
			else
			{
				switch (iop)
				{
				  case ISQRT:
					if (x1 < 0.0)
					{
						*status |= FOUNDNEG;
						noGood = 1;
					}
					break;

				  case ILOG:
				  case ILOG10:
				  case ILGAMMA:
					if (x1 <= 0.0)
					{
						*status |= FOUNDNONPOS;
						noGood = 1;
					}
					break;

				  case ICOS:
				  case ISIN:
				  case ITAN:
					if (fabs(x1) >= MaxAngle)
					{
						*status |= FOUNDDOMERR;
						noGood = 1;
					}
					break;

				  case IATANH:
					if(fabs(x1) >= 1.0)
					{
						*status |= FOUNDDOMERR;
						noGood = 1;
					}
					break;

				  case IASIN:
				  case IACOS:
					if (fabs(x1) > 1.0)
					{
						*status |= FOUNDDOMERR;
						noGood = 1;
					}
					break;
				
				  case INBITS:
					if(x1 < 0 || x1 > MAXBITVALUE || x1 != floor(x1))
					{
						*status |= FOUNDDOMERR;
						noGood = 1;
					}
					break;
				
				  case IFLOOR:
				  case ICEILING:
					if (x1 > MaxInteger - 1 || x1 < -MaxInteger + 1)
					{
						*status |= FOUNDDOMERR;
						noGood = 1;
					}
					break;

				  default:
					break;
				} /*switch (iop)*/
			}

			if (!noGood)
			{
				if(fromAngles)
				{				/*cos(), sin(), tan()*/
					x1 *= Angles2Radians;
				}
				z = (nargs == 1) ? (*f1)(x1) : (*f2)(x1,x2);

				if(toAngles)
				{				/*acos(), asin(), atan(), atan(2)*/
					z /= Angles2Radians;
				}
				else if (checkInfinity &&
						 (absz = fabs(z), doubleEqualBdouble(absz, TooBigValue)))
				{
					*status |= FOUNDOVERFLOW;
					noGood = 1;
				}
			} /*if (!noGood)*/
		
			if(noGood)
			{
				anyBad = 1;
				setMissing(z);
			} /*if(noGood)*/
			DATAVALUE(result,i) = z;
		} /*for (i = 0; i < tot1; i++)*/
	} /*if (tot1 > 0)*/
	
#if (USENOMISSING)
	if (anyBad)
	{
		clearNOMISSING(result);
	}
	else
	{
		setNOMISSING(result);
	}
#endif /*USENOMISSING*/

	return (result);
	
  errorExit:
	Removesymbol(result);
	return (0);
} /*doTrans()*/

/*
  971104 added check on value of myhandlelength()
  980806 changed behavior.  If an element of arg1 is "" or starts with
         "@", it is copied unchanged.  This is to make the bahavior more
         useful with labels that "expand" when printed.
*/
static Symbolhandle characterTrans(Symbolhandle arg1, Symbolhandle arg2,
								   struct opEntry1 *op)
{
	Symbolhandle        result = (Symbolhandle) 0;
	long                i, nargs = (arg2 != (Symbolhandle) 0) ? 2 : 1;
	long                ndims = NDIMS(arg1);
	long                type2, size, needed = 0, used;
	long                handleLength;
	int                 iop = op->iop;
	char               *name = op->name;
	char                digitString[30];
	char               *digitStringP = digitString;
	char              **string2H;
	char               *place1, *place2, *place;
	WHERE("characterTrans");
	
	if (nargs == 2)
	{
		type2 = TYPE(arg2);
		if (iop == IROUND)
		{
			if (!isScalar(arg2) ||
				(type2 != REAL && type2 != CHAR ||
				 type2 == REAL && isMissing(DATAVALUE(arg2, 0))))
			{
				sprintf(OUTSTR,
						"ERROR: on %s(charVar,arg2), arg2 must be REAL or CHARACTER scalar",
						FUNCNAME);
				goto errorExit;
			}
			if (type2 == REAL)
			{
				char         format[20];
				char        *fmt = DATAFMT;
				
				while (isdigit(*fmt) || isspace(*fmt) || *fmt == '%')
				{
					fmt++;
				}
				format[0] = '%';
				strcpy(format+1, fmt);
				sprintf(digitString, format, DATAVALUE(arg2, 0));
				string2H = &digitStringP;
			}
			else
			{
				string2H = STRING(arg2);
			}
		} /*if (iop == IROUND)*/
		else
		{
			if (type2 != CHAR || dimcmp(arg1, arg2, NEARSTRICT) != 0)
			{
				sprintf(OUTSTR,
						"ERROR: on %s(charVar,arg2), arg2 must be CHARACTER and match charVar",
						FUNCNAME);
				goto errorExit;
			}
			string2H = STRING(arg2);
		} /*if (iop == IROUND){}else{}*/
	} /*if (nargs == 2)*/
	
	size = symbolSize(arg1);
	handleLength = myhandlelength(STRING(arg1));
	if (handleLength < 0)
	{
		goto errorExit;
	}
	
	needed = handleLength + size*(strlen(name) + 2);
	if (nargs == 2)
	{
		if (iop == IROUND)
		{
			needed += size * (strlen(*string2H) + 1);
		} /*if (iop == IROUND)*/
		else
		{
			handleLength = myhandlelength(string2H);
			
			/*
			  Note: handleLength includes room for separating '\0', which
			  means it includes room for separating commas in output
			*/
			if (handleLength < 0)
			{
				goto errorExit;
			}
			needed += handleLength;
		} /*if (iop == IROUND){}else{}*/
	} /*if (nargs == 2)*/

	result = CInstall(SCRATCH, needed);
	if (result == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	
	if(nargs == 2 && iop != IROUND && ndims > NDIMS(arg2))
	{					/* atan2 or hypot */
		ndims = NDIMS(arg2);
	}
	
	setNDIMS(result,ndims);
	for (i = 1; i <= ndims; i++)
	{
		setDIM(result,i,DIMVAL(arg1,i));
	}
	
	place1 = STRINGPTR(arg1);
	place = STRINGPTR(result);
	if (nargs == 2)
	{
		place2 = *string2H;
	}
	
	used = 0;
	for (i = 0; i < size; i++)
	{
		if (place1[0] == '\0' || place1[0] == NUMERICLABEL ||
			strchr(Lparens + BracketIndex, place1[0]) != (char *) 0 ||
			nargs == 2 && iop != IROUND &&
			(place2[0] == '\0' || place2[0] == NUMERICLABEL ||
			strchr(Lparens + BracketIndex, place2[0]) != (char *) 0))
		{
			strcpy(place, place1);
		}
		else if (nargs == 1)
		{
			sprintf(place, "%s(%s)", name, place1);
		}
		else
		{
			sprintf(place, "%s(%s,%s)", name, place1, place2);
			if (iop != IROUND)
			{
				place2 = skipStrings(place2, 1);
			}
		}
		used += strlen(place) + 1;
		place1 = skipStrings(place1, 1);
		place = skipStrings(place, 1);
	} /*for (i = 0; i < tot; i++)*/ 

	if (used != needed)
	{
		TMPHANDLE = mygrowhandle(STRING(result), used);
		if (TMPHANDLE == (char **) 0)
		{
			goto errorExit;
		}
		setSTRING(result, TMPHANDLE);
	}
	
	return (result);
	
  errorExit:
	putErrorOUTSTR();
	return (0);
	
} /*characterTrans()*/

/*
   Function to implement MacAnova transformations of form f1(x) and f2(x,y)

   x and y must be REAL or structures of REALS or CHARACTER variables.  Except
   for round(x,p), if there is a second argument it much match the 
   first argument.

   960320 x and y can be CHARACTER variables (not structures).

   970527 value of trig functions (cos(x), sin(x), tan(x)) set to MISSING
          for |x| >= MAXANGLE*PI
   981214 added keywords 'degrees', 'cycles', 'radians' for trig functions,
          overriding option 'angles'
*/


Symbolhandle    Transform(Symbolhandle list)
{
	Symbolhandle    arg1, arg2 = (Symbolhandle) 0, result = (Symbolhandle) 0;
	long            nargs = NARGS(list), margs;
	Symbolhandle    symhKey = COMPVALUE(list, nargs - 1);
	struct opEntry1 *op;
	double          param[1];
	double          angles2cycles = 8.0*MV_PI_4;
	double          angles2degrees = MV_PI_4/45.0;
	unsigned long   status, control = 0;
	long            reply;
	int             iop;
	int             usesAngles;
	char           *keyword;
	WHERE("Transform");
	
	*OUTSTR = '\0';

	for(op = Ops;op->iop != 0;op++)
	{
		if(strcmp(FUNCNAME,op->name) == 0)
		{
			break;
		}
	}
	iop = op->iop;
	if(iop == INOTFOUND)
	{ /* should never happen */
		sprintf(OUTSTR,"ERROR: unrecognized transformation %s()", FUNCNAME);
		goto errorExit;
	}
	usesAngles = (iop == IACOS) || (iop == IASIN) || (iop == IATAN) ||
	  (iop == IATAN2) || (iop == ICOS) || (iop == ISIN) || (iop == ITAN);
	Angles2Radians = ANGLES2RADIANS;

	margs = op->nargs;

	if (nargs > 1 && (keyword = isKeyword(symhKey)))
	{
		if (!usesAngles)
		{
			badKeyword(FUNCNAME, keyword);
			goto errorExit;
		}
			
		if (strncmp(keyword, "cycle", 5) == 0)
		{
			Angles2Radians = angles2cycles;
		}
		else if (strncmp(keyword, "degree", 6) == 0)
		{
			Angles2Radians = angles2degrees;
		}
		else if (strncmp(keyword, "radian", 6) == 0)
		{
			Angles2Radians = 1.0;
		}
		else
		{
			badKeyword(FUNCNAME, keyword);
			goto errorExit;
		}
		if (!isTorF(symhKey))
		{
			notTorF(keyword);
			goto errorExit;
		}
		if (DATAVALUE(symhKey, 0) == 0.0)
		{
			sprintf(OUTSTR,
					"ERROR: value for %s() keyword %s must be T",
					FUNCNAME, keyword);
			goto errorExit;
		}
		nargs--;
	} /*if (nargs > 1 && (keyword = isKeyword(symhKey)))*/

	MaxAngle = MV_PI*MAXANGLE/Angles2Radians;

/*
  atan() and round() may have 1 or 2 arguments, hypot() and atan2()
  require 2
*/
	if(margs > 0 && nargs != margs || margs < 0 && nargs > labs(margs))
	{
		badNargs(FUNCNAME, margs);
		return (0);
	}

	arg1 = COMPVALUE(list, 0);
	if(!argOK(arg1, 0, (nargs > 1) ? 1 : 0))
	{
		goto errorExit;
	}

	if (nargs == 2)
	{
		arg2 = COMPVALUE(list,1);
		if(!argOK(arg2, 0, 2))
		{
			goto errorExit;
		}
	} /*if (nargs == 2)*/
	
	if (TYPE(arg1) == CHAR)
	{
		return (characterTrans(arg1, arg2, op));
	}
	
	if(!isReal(arg1))
	{
		sprintf(OUTSTR,
				"ERROR: argument to %s() must be REAL or structure of REALs",
				FUNCNAME);
		goto errorExit;
	}
	if(isStruc(arg1))
	{
		control |= LEFTRECUR;
	}

	if(nargs == 2)
	{ /* must be atan, atan2, hypot, or round */
		if(iop == IATAN)
		{
			iop = IATAN2;
		}
		
		if(iop != IROUND && !isReal(arg2))
		{
			sprintf(OUTSTR,
					"ERROR: argument 2 to %s() must be REAL or structure of REALs",
					FUNCNAME);
			goto errorExit;
		} /*if(iop != IROUND && !isReal(arg2))*/

		if(iop == IATAN2 || iop == IHYPOT)
		{
			if(isStruc(arg2))
			{
				control |= RIGHTRECUR;
			}
			
			reply = treecmp(arg1,arg2,control|NEARSTRICT);
			if(reply != 0)
			{
				sprintf(OUTSTR,
						"ERROR: %s of arguments to %s() do not match",
						(reply & BADSTRUC) ?
						"structure shape" : "dimensions",
						FUNCNAME);
			} /*if(reply != 0)*/			
		}
		else if(iop == IROUND)
		{/* second argument must be integer */
			if (!isInteger(arg2, ANYVALUE))
			{
				sprintf(OUTSTR,
						"ERROR: 2nd argument to %s() must be single integer",
						FUNCNAME);
			}
			else
			{
				param[0] = DATAVALUE(arg2, 0);
			}
		}
		else /* should not be reached */
		{
			sprintf(OUTSTR,"ERROR: programming error in Transform()");
		}
			
		if(*OUTSTR)
		{
			goto errorExit;
		}
	} /*if(nargs == 2)*/
	else
	{
		param[0] = 0.0;
		arg2 = (Symbolhandle) 0;
	} /*if(nargs == 2){}else{}*/

	if(isscratch(NAME(arg1)))
	{
		control |= REUSELEFT;
		COMPVALUE(list,0) = (Symbolhandle) 0;
	}
	else if((iop == IATAN2 || iop == IHYPOT) &&
			isscratch(NAME(arg2)))
	{
		control |= REUSERIGHT;
		COMPVALUE(list,1) = (Symbolhandle) 0;
	}
	
	status = 0;
	result = doRecur2(doTrans, arg1, arg2, param, control | iop, &status);

	if(status & FOUNDMISSING)
	{
		sprintf(OUTSTR,"WARNING: missing values in argument(s) to %s()",FUNCNAME);
		putErrorOUTSTR();
	}
	
	if(status & FOUNDOVERFLOW)
	{
		sprintf(OUTSTR,"WARNING: %s(x) with x too large set to MISSING",
				FUNCNAME);
		putErrorOUTSTR();
	}
	if (status & (FOUNDNONPOS | FOUNDNEG | FOUNDDOMERR))
	{
		char           *pc;
		
		sprintf(OUTSTR, "WARNING: %s(x) with ", FUNCNAME);
		pc = OUTSTR + strlen(OUTSTR);
		
		if(status & FOUNDNONPOS)
		{
			pc += formatChar(pc, "x <= 0", CHARASIS);
		}
		else if(status & FOUNDNEG)
		{
			pc += formatChar(pc, "x < 0", CHARASIS);
		}
		else if(status & FOUNDDOMERR)
		{
			if (iop == IASIN || iop == IACOS)
			{
				pc += formatChar(pc, "|x| > 1", CHARASIS);
			}
			else if (iop == IATANH)
			{
				pc += formatChar(pc, "|x| >= 1", CHARASIS);
			}
			else if (iop == ICOS || iop == ISIN || iop == ITAN)
			{
				pc += formatChar(pc, "|x| >= ", CHARASIS);
				pc += formatDouble(pc, MaxAngle, DOFLOAT | TRIMLEFT);
				pc += formatChar(pc,
								 (Angles2Radians == angles2cycles) ?
								 " cycles" :
								 ((Angles2Radians == angles2degrees) ?
								  " degrees" : " radians"), CHARASIS);
			}
			else if (iop == IFLOOR || iop == ICEILING)
			{
				sprintf(pc, "|x| >= %.17g", MaxInteger - 1);
				pc += strlen(pc);
			}
			else
			{
				pc += formatChar(pc, "x = illegal value", CHARASIS);
			}
		}
		formatChar(pc," set to MISSING", CHARASIS);
		putErrorOUTSTR();
	} /*if (status & (FOUNDNONPOS | FOUNDNEG | FOUNDDOMERR))*/	

	return (result);
	
  errorExit:
	putErrorOUTSTR();

	return (0);
} /*Transform()*/

