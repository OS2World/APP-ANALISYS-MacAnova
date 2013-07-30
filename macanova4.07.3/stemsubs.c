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
#pragma segment Stemleaf
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"

#ifndef USEPOW
#undef pow
#define pow    intpow   /*pow used only with integer powers*/
#endif /*USEPOW*/

/*
  This file contains C programs translated and adapted from Fortran code from	 the book ABCs of EDA by David Hoaglin and Paul Velleman, 1981, Duxbury

  The code was somewhat restructured and simplified by C. Bingham,
  kb@umnstat.stat.umn.edu

  Adapted for use with MacAnova 921125
  functions in this file adapted from original stemsubs.c are
  	stmnlf(), depthp(), outlyp(), sltitl(), stemp()
  Other related functions adapted from those file utilities.c are
	cinit(), intfn(), rprnt(), putchr(), putnum(), wdthof(), yinfo(),
	nposw()
*/

/*
	Various utility functions used by both box plot and stem leaf routines
	900114 Converted to C by c. bingham
    960705 Modified to allow optional printing of depth
    990215 changed myerrorout() to putOutMsg()
*/

#define MAX(a, b) (((a) > (b)) ? (a) : (b) )
#define MIN(a, b) (((a) < (b)) ? (a) : (b) )

#undef  TRUE
#undef  FALSE
#define TRUE 1
#define FALSE 0

static  int        GutterPos, DepthOffset;

#define DEPTHOFFSET 6  /*determines distance of depth to left of gutter*/
#define GUTTERPOS   5

/* from common area /chrbuf/ */

#define Trash       NAMEFORTRASH
enum scratchCodes
{
	GP = 0,
	GISCRATCH,
	NSCRATCH
};

static Symbolhandle Trash;

static long         Pmin; /* first column position */
static long         Pmax; /* last column position */
static long         Outptr, Maxptr;
static char       **P = (char **) 0;

/* replace stuff in numbrs.h */
#ifndef MAXINT
#define MAXINT   2147483647
#endif /*MAXINT*/
#define STEMFUZZ     1e-10

#define Blank    ' '

#define NN 4
static double Nicnos[NN] = {1.0, 2.0, 5.0, 10.0};

static void clearP(void)
{
	char       *p = *P;
	long        i;
	
	for (i = 0;i < Pmax ;i++)
	{
		p[i] = Blank;
	}
	p[Pmax] = '\0';
} /*clearP()*/

static long cinit(long width, long depth)
{
	DepthOffset = (!depth) ? 0 : DEPTHOFFSET;
	GutterPos = GUTTERPOS + DepthOffset ;

	Maxptr = Outptr = Pmin = 1;
	Pmax = width;
	if (Pmax - Pmin < GutterPos + 7)
	{
		Pmax = Pmin + GutterPos + 7;
	}
	
	if (!getScratch(P, GP, Pmax+1, char))
	{
		return (0);
	}
	clearP();
	return (1);

} /*cinit()*/

static long intfn(double x)
{
	long      result;

	if (fabs(x) <= MAXINT)
	{
		result = (long) ((1.0 + STEMFUZZ)*x);
	}
	else
	{
		result = (x > 0.0) ? MAXINT : -MAXINT;
	}
	return (result);
	
} /*intfn()*/


/* output line in *P and reinitialize */
static void rprnt(void)
{
	char         *p = *P;

	p[Maxptr] = '\0';
	putOutMsg(p);

	Maxptr = Outptr = Pmin;
	
	clearP();
} /*rprnt()*/


static void putchr(long posn, char chr)
{
	WHERE("putchr");
	
	if (posn != 0)
	{
		Outptr = MAX(Pmin, posn);
	}
	Outptr = MIN(Outptr, Pmax);
	(*P)[Outptr-1] = chr;
	Maxptr = MAX(Maxptr, Outptr);
	Outptr++;
} /*putchr()*/

static void putstr(long posn, char * str)
{
	char         *s = str;
	
	if (posn != 0)
	{
		putchr(posn, *s++);
	}
	while (*s != '\0')
	{
		putchr(0, *s++);
	}
} /*putstr()*/

		
static void putnum(long posn, long n, long width)
{
	long        ip, numlen;
	char        buffer[50];
	WHERE("putnum");
	
	sprintf(buffer, "%ld", n);
	numlen = strlen(buffer);
	ip = ((posn != 0) ? posn : Outptr) + width - numlen;
	
	putstr(ip, buffer);
} /*putnum()*/

static long wdthof(long val)
{
	long       absval = labs(val), numlen;

	numlen = (val >= 0) ? 1 : 2;
	while (absval > 0)
	{
		absval /= 10;
		numlen++;
	}
	
	return numlen;
} /*wdthof()*/


#if (1)
/*
   960705 modified yinfo() to makeuse of boxprep()
*/
static void yinfo(double *y, long n, double *med, double *hl, double *hh, 
		   double *adjl, double *adjh, 
		   long *iadjl, long *iadjh)
{
	double       fivenum[5];
	long         nout[4];

	boxprep(y, n, fivenum, nout);

	*iadjl = nout[0] + nout[1];
	*iadjh = n - 1 - nout[2] - nout[3];
	*adjl = fivenum[0]; /* lowest value within inner fences */
	*hl = fivenum[1];
	*med = fivenum[2];
	*hh = fivenum[3];
	*adjh = fivenum[4]; /* highest value within inner fences */
} /*yinfo()*/
#else  /* former code */
static void yinfo(double *y, long n, double *med, double *hl, double *hh, 
		   double *adjl, double *adjh, 
		   long *iadjl, long *iadjh)
{
	double       hfence, lfence, step;
	long         j, k;

	sortquick(y, n);

	k = n;
	j = (k/2);
	*med = (y[j] + y[n - 1 - j])/2.0;
	k = (k + 1)/2;
	j = (k/2);
	*hl = (y[j]+y[k-1-j])/2.0;
	*hh = (y[n - k + j] + y[n - 1 - j])/2.0;
	step = (*hh - *hl)*1.5;
	hfence = *hh + step;
	lfence = *hl - step;
	for (*iadjl = 0;y[*iadjl] < lfence; (*iadjl)++)
	{
		;
	}
	
	for (*iadjh = n-1;y[*iadjh] > hfence; (*iadjh)--)
	{
		;
	}
	
	*adjl = y[*iadjl]; /* lowest value within inner fences */
	*adjh = y[*iadjh];  /* highest value within inner fences */
} /*yinfo()*/
#endif

static long nposw(double *high, double *low,
				  long maxp, long mzero, 
				  long *ptotl, double *fract, 
				  double *unit, double *npw)
{
	long         i;
	double       lo = *low , hi = *high;
	long         jlo, jhi, trialjlo, trialjhi;
	long         trialstems;
	double       trialunit, trialfract, trialnpw, aprxw;
	WHERE("nposw");

	*ptotl = -1;

	aprxw = (hi - lo)/(double) maxp;
	trialunit = pow(10.0, floor(log10(aprxw)));
	trialfract = aprxw/(trialunit);
	for (i = 0;i < NN && Nicnos[i] < trialfract;i++)
	{
		;
	} /*for (i = 0;i < NN ;i++)*/

	while (1)
	{
		trialfract = Nicnos[i];
		trialnpw = trialfract*trialunit;
		trialjlo = intfn(lo/trialnpw);
		trialjhi = intfn(hi/trialnpw);
		trialstems = trialjhi - trialjlo + 1;
		if (*high > trialjhi*trialnpw + .99999*trialnpw)
		{
			trialstems++;
		}
		if (mzero && (hi*lo < 0.0 || hi == 0.0))
		{
			trialstems++;
		}

		if (trialstems <= maxp)
		{
/*
  We are trying to find the largest value for trialstems that does not
  exceed maxp, not just the first.
*/
			if (trialstems <= *ptotl)
			{
				break;
			}
			*ptotl = trialstems;
			*npw = trialnpw;
			*unit = trialunit;
			*fract = trialfract;
			jlo = trialjlo;
			jhi = trialjhi;
			if (trialstems == maxp)
			{
				break;
			}
		} /*if (trialstems <= maxp)*/

		i++;
		
		if (i >= NN)
		{
			i = 0;
			trialunit *= 10.0;
		}
	} /* while (1) */

	if (*fract == 1.0)
	{
		*fract = 10.0;
		*unit /= 10.0;
	} /*if (*fract == 1.0)*/
	
	*low = *npw*jlo;
	if (lo < 0)
	{
		*low -= *npw;
	}

	*high = *npw*jhi;
	if (hi > 0)
	{
		*high += *npw;
	}

	return (1);
} /*nposw()*/


static void depthp(double *w, long *iw, long n, 
				   long *pt1, long *pt2, long cut, long iadjh,  long hi, 
				   long doDepth, long *rank, long *medyet, long slbrk)
{
	long         lefcnt, ptz, depth, nwid, ptx;
	long         cutreached = FALSE;
	WHERE("depthp");
	
	ptx = *pt1;
	for (*pt1 = ptx;*pt1 <= iadjh ;(*pt1)++)
	{
		if (iw[*pt1] > cut || (cut >= 0 && iw[*pt1] == cut))
		{
			cutreached = TRUE;
			break;
		}
	} /*for (*pt1 = ptx;*pt1 <= iadjh ;(*pt1)++)*/

	if (cutreached)
	{
		if (/* hi>0 && */ cut == 0)
		{
			for (ptz = *pt1;ptz < n && w[ptz] < 0.0;ptz++)
			{
				;
			} /*for (ptz = *pt1;ptz < n && w[ptz] < 0.0;ptz++)*/
			*pt1 = ptz;
	
			for (ptz = *pt1;ptz < n && w[ptz] <= 0.0;ptz++)
			{
				;
			} /*for (ptz = *pt1;ptz < n && w[ptz] <= 0.0;ptz++)*/
			/* now ptz-*pt1 is number of exact zeros */
			*pt1 += intfn((double)(ptz - *pt1)/2.0 );
		}/*if (cut == 0)*/
	} /*if (cutreached)*/
	else
	{
		*pt1 = iadjh + 1;
	}

	if (doDepth)
	{
		lefcnt = *pt1 - *pt2;
		*rank = *rank + lefcnt;
		if (*medyet)
		{
			depth = n - (*rank - lefcnt);
		} /*if (*medyet)*/
		else
		{
			if ((double) *rank == .5*(double) n)
			{
				*medyet = TRUE;
			}
			else if ((double) *rank >= .5*(double)(n+1))
			{
				*medyet = TRUE;
				nwid = wdthof(lefcnt);
				putchr(slbrk - DepthOffset - nwid - 1 , '(');
				putnum(0, lefcnt, nwid);
				putchr(0, ')');
				return;
			}
			depth = *rank;
		} /*if (*medyet){}else{}*/
		nwid = wdthof(depth);
		putnum(slbrk - DepthOffset - nwid, depth, nwid);
	} /*if (doDepth)*/	
} /*depthp()*/

static void outlyp(long *iw, long from, long to, long hiend, long slbrk)
{
	long         opos, nwid, lhmax, i;
	WHERE("outlyp");
	
	opos = slbrk - DepthOffset;
	if (!hiend)
	{
		putstr(opos - 3, "Low ");
	}
	else
	{
		putstr(opos - 4, "High ");
	}
	nwid = MAX(wdthof(iw[from]), wdthof(iw[to]));
	lhmax = Pmax-nwid-2;

	for (i=from;i<=to ;i++)
	{
		putnum(0, iw[i], nwid);
		if (i < to)
		{
			putchr(0, ',');
			putchr(0, ' ');
		} /*if (i < to)*/
		
		if (Outptr >= lhmax || i == to)
		{
			rprnt();
			putchr(opos, ' ');
		} /*if (Outptr >= lhmax || i == to)*/
	} /*for (i=from;i<=to ;i++)*/
} /*outlyp()*/

static void sltitl(double unit, long linwid)
{
	long         iexpt;
	char        *outstr = OUTSTR;
	WHERE("sltitl");
	
	outstr += indentBuffer(outstr, GutterPos - 2);
	outstr += formatChar(outstr, (linwid == 10) ? " 1|1 " : "1*|1 ", CHARASIS);
	outstr += formatChar(outstr, "represents ", CHARASIS);
	iexpt = intfn(log10(unit));
	if (iexpt > 8 || iexpt == -1)
	{
		sprintf(outstr, "%g", 11.*unit);
	}
	else if (iexpt  > 0)
	{
		sprintf(outstr, "%ld", 11 * intfn(unit));
	}
	else
	{
		sprintf(outstr, "%.2g", 11*pow(10.0, (double) iexpt));
	}
	outstr += strlen(outstr);
	sprintf(outstr, "  Leaf digit unit = %g", unit);
	putOUTSTR();
} /*sltitl()*/

static void stemp(long stem, long linwid, long negnow, long slbrk)
{
	long         nstem, lefdig, nwid, opos;
	char        *ch5stm = "*tfs.";
	WHERE("stemp");
	
	nstem = stem/10;
	lefdig = labs(stem-nstem*10);
	nwid = wdthof(nstem);
	opos = (linwid == 10) ? slbrk-2 : slbrk-3;
	if (nstem == 0)
	{
		putchr(opos, (negnow) ? '-' : '+');
	}
	
	opos = (linwid == 10) ? slbrk - nwid : slbrk-nwid-1;
	putnum(opos, nstem, nwid);

	if (linwid == 2)
	{
		putchr(0, ch5stm[lefdig/2]);
	}
	else if (linwid == 5)
	{
		putchr(0, (lefdig < 5) ? '*' : '.');
	}
	else if (linwid != 10)
	{
		putOutMsg("stemleaf error 12");
	}
	putchr(slbrk, '|');
} /*stemp()*/



static long stmnlf(double *y, long n, long *iw,
				   long maxlines, long xtrems, long verbose, long depth)
{
	double        med, hl, hh, adjl, adjh, unit, fract, npw;
	long          i, slbrk, pltwid, rank, iadjl, iadjh, nlins;
	long          nlmax, linwid;
	long          low, hi, cut, stem, pt1, pt2, j, spacnt, leaf;
	long          negnow, medyet;
	WHERE("stmnlf");

	slbrk = Pmin + GutterPos;
	pltwid = Pmax - slbrk - 2;

	yinfo(y, n, &med, &hl, &hh, &adjl, &adjh, &iadjl, &iadjh);

	if (adjh <= adjl || xtrems)
	{
		iadjl = 0;
		iadjh = n - 1;
		adjl = y[iadjl];
		adjh = y[iadjh];
	} /*if (adjh <= adjl || xtrems)*/

	if (verbose)
	{
		char     *outstr = OUTSTR;
		
		sprintf(outstr, "n=%ld, Min=", n);
		outstr += strlen(outstr);
		outstr += formatDouble(outstr, y[0], TRIMLEFT);
		outstr += formatChar(outstr, ", Q1=", CHARASIS);
		outstr += formatDouble(outstr, hl, TRIMLEFT);
		outstr += formatChar(outstr, ", M=", CHARASIS);
		outstr += formatDouble(outstr, med, TRIMLEFT);
		outstr += formatChar(outstr, ", Q3=", CHARASIS);
		outstr += formatDouble(outstr, hh, TRIMLEFT);
		outstr += formatChar(outstr, ", Max=", CHARASIS);
		outstr += formatDouble(outstr, y[n-1], TRIMLEFT);
		putOUTSTR();
	} /*if (verbose)*/

	/* maxlines < 0 => -maxlines was default and was not reset */
	nlmax = (maxlines < 0) ?
		intfn(10.0*log10((double) (iadjh - iadjl + 1))) : maxlines;
	nlmax = MIN(nlmax, labs(maxlines));
	if (adjh <= adjl)
	{
		adjh = adjl + 1.0;
		nlmax = 1;
	}
	if (!nposw(&adjh, &adjl, nlmax, TRUE, &nlins, &fract, &unit, &npw))
	{
		return (0);
	}

	if (!xtrems)
	{
		while (iadjl >= 0 && y[iadjl] >= adjl)
		{
			iadjl--;
		}
		iadjl++;
		while (iadjh < n && y[iadjh] <= adjh)
		{
			iadjh++;
		}
		iadjh--;
	} /*if (!xtrems)*/

	for (i=0;i<n ;i++)
	{
		iw[i] = intfn(y[i]/unit);
	} /*for (i=0;i<n ;i++)*/

	if (fract != 10.0)
	{
		for (i=iadjl;i<=iadjh ;i++)
		{
			if (iw[i] % 10 != 0)
			{
				break;
			}
		} /*for (i=iadjl;i<=iadjh ;i++)*/
		if (i > iadjh)
		{
			fract = 10.0;
			npw = fract*unit;
			nlins = intfn(adjh/npw) - intfn(adjl/npw) + 1;
			if (adjh*adjl<0.0 || adjh == 0.0)
			{
				nlins = nlins + 1;
			}
		} /*if (i > iadjh)*/
	} /*if (fract != 10.0)*/
	low = iw[iadjl];
	hi = iw[iadjh];
	linwid = intfn(fract);
	rank = iadjl;
	if (rank > 0)
	{
		outlyp(iw, 0, rank - 1, FALSE, slbrk);
	}
	cut = linwid * (int) floor((1.0 + STEMFUZZ)*(double) low/(double) linwid);
	if (adjl >= 0)
	{
		negnow = FALSE;
		stem = cut - linwid;
	}
	else
	{
		cut = (cut == 0) ? cut - linwid : cut;
		negnow = TRUE;
		stem = cut;
	}

	medyet = FALSE;
	pt1 = iadjl;
	pt2 = pt1;

	for (j=0;j < nlins && pt1 <= iadjh;j++)
	{
		cut += linwid;
		if (stem != 0 || !negnow)
		{
			stem += linwid;
		}
		else
		{
			negnow = FALSE;
		}
		spacnt = 0;

		depthp(y, iw, n, &pt1, &pt2, cut, iadjh, hi, depth, &rank, &medyet,
			   slbrk);
		
		stemp(stem, linwid, negnow, slbrk);

		while (pt2 < pt1)
		{
			leaf = labs(iw[pt2] - (stem/10)*10);
			putnum(0, leaf, 1);
			if (++spacnt >= pltwid)
			{
				break;
			}
			pt2++;
		} /*while (pt2 < pt1)*/

		if (pt2 < pt1)
		{ /* overflow */
			putchr(0, '*');
			pt2 = pt1;
		}
		rprnt();
	} /*for (j=0;j < nlins && pt1 <= iadjh;j++)*/

	if (pt1 < n)
	{
		outlyp(iw, pt1, n - 1, TRUE, slbrk);
	}
	else
	{
		myeol();
	}
	
	sltitl(unit, linwid);
	return (1);
} /*stmnlf()*/

long doStemLeaf(double ** data, long n, long width,
				long maxlines, long xtrems, long verbose, long depth,
				char **title)
{
	long         **iscratch = (long **) 0;
	WHERE("doStemLeaf");
	
	P = (char **) 0;
	if ((Trash = GarbInstall(2)) == (Symbolhandle) 0)
	{
		goto errorExit;
	}
	
	if (!cinit(width, depth))
	{
		goto errorExit;;
	}
	
	if (!getScratch(iscratch, GISCRATCH, n, long))
	{
		goto errorExit;
	}

	if (title != (char **) 0)
	{
		putOutMsg(*title);
	}
	
	if (!stmnlf(*data, n, *iscratch, maxlines, xtrems, verbose, depth))
	{
		goto errorExit;
	}
	emptyTrash();
	P = (char **) 0;
	
	return (1);
	
  errorExit:
	putOUTSTR();

	emptyTrash();
	P = (char **) 0;
	
	return (0);
	
} /*doStemLeaf()*/

