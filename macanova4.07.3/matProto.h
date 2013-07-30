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

/*
   961118 Added mygettime()
   961217 Added randomSeed()
   961218 changed name and type of uni() to long iuni() added double duni()
   961219 added kmeans()
   970107 modified do_whole_graph(), getPlotKeys() and checkPlotKeys()
   970109 added installScreenDims()
   970123 modified iniClipboard()
   970203 added processBang()
   970217 added fsolve()
   970226 changed prototype for decodeString()
   970310 added nDigits() in utils.c
          added prepPrintLabels(), putColumnLabels(), and
          putRowLabels() in labutils.c
   970610 Added dynload() and callC() in dynload.c
   970618 Added evaluate() in Macroset.c
          Added pushInputlevel() and popInputlevel() in commonio.c
          Added checkBalance() in utils.c
   970620 Added cleanInputlevels() in commonio.c
          Added mvEval() in Macroset.c
   970625 Added checkArgType() to utils.c
   970626 Added longMin(), longMax(), doubleToLong() and longToDouble()
          to utils.c
          Added asLong() to dynload.c
   970711 Added notPositiveLong(), notNonNegativeLong(), notLong()
          to errorMsg.c
   970730 Added isNumber() to utils.c
   970820 Added findBracket() to utils.c
          Added stringToTerm() to glmutils.c
   970822 Changed prototype for initmodelparse in mreader.y
   971012 Added QDunnett(), PDunnett()
   971016 Changed prototype for function argument to fsolve()
   971103 Changed prototype for matWrite()
   971211 Added getHistory()
   971212 Changed argument list to getPlotKeys(), added unsetPlotKeys()
   971215 Changed argument list to checkPlotKeys()
   971216 Changed argument list to do_whole_graph()
   980312 Added argument completeDim to buildSubscript()
   980315 Added anyNaN() and anyInfinite()
   980316 Added anyDoubleMissing(), anyDoubleNaN(), anyDoubleInfinite()
   980401 Added argument prompt to initialize()
   980423 Changed int argument type to long in Makesymbol(), 
          Makespecialsymbol() and Install()
   980511 Added fillLINE() to commonio.c
   980521 Added arguments to scanPat()
   980621 Added Setdims() to Symbol.c
   980710 Added clamp as argument to iterglm
   980714 Modified type and arguments of restoreGraph()
   980715 Modified calling sequence of saveGraph()
   980723 Modified calling sequences of matWrite() and macWrite()
   980724 Changed return value of checkArgsType() to int
   980727 Added setlabs() to labutils.c
   980730 Changed calling sequence to reuseArgs()
   981008 Added n_th() to utils.c
   990207 Added inWhichMacro() to errorMsg.c
   990207 Added argument to mymultiprint
   990210 Added putErrorOUTSTR() in commonio.c
   990215 Added putOutMsg(), putOutErrorMsg() in commonio.c
   990220 Added prepMacro() in macro() (previously was static)
*/
#ifndef MATPROTOH__
#define MATPROTOH__
#undef UNDEFINED__

#ifndef NOPROTOTYPES
#include "typedefs.h"
#include "plot.h"
#include "keywords.h"

/* macMain.c*/
void myUnloadSeg(void * /*routineAddr*/);
/* Ifsetup.c */
int Ifsetup(Symbolhandle /*list*/, long /*op*/);
/* Lang.c */
Symbolhandle Unary(Symbolhandle /*arg*/, int /*op*/);
Symbolhandle Arith(Symbolhandle /*arg1*/, Symbolhandle /*arg2*/, int /*op*/);
Symbolhandle Logic(Symbolhandle /*arg1*/, Symbolhandle /*arg2*/, int /*op*/);
#ifdef UNDEFINED__
Symbolhandle doMatMult(Symbolhandle /*arg1*/, Symbolhandle /*arg2*/, long /*control*/);
Symbolhandle doUnary(Symbolhandle /*arg*/, long /*control*/, long * /*status*/);
Symbolhandle doBinary(Symbolhandle /*arg1*/, Symbolhandle /*arg2*/, long /*control*/, long * /*status*/);
#endif /*UNDEFINED__*/
Symbolhandle Element(Symbolhandle /*arg1*/, Symbolhandle /*list*/);
#ifdef UNDEFINED__
Symbolhandle elementMatrix(Symbolhandle /*arg*/, Symbolhandle /*subscriptsymh*/);
char **extractIt(Symbolhandle /*arg*/, Symbolhandle /*list*/, long /*resdim*/[]);
#endif /*UNDEFINED__*/
long subscriptLength(Symbolhandle /*subscriptsymh*/,long /*maxsubscr*/);
long buildSubscript(Symbolhandle /*subscriptsymh*/, long /*maxsubscr*/,
					long /*nitems*/, long * /*subvals*/,
					int * /*completeDim*/);
/* Macroset.c */
long Macrosetup(Symbolhandle /*macro*/, Symbolhandle /*list*/);
Symbolhandle mvEval(char ** /*commandH*/);
Symbolhandle evaluate(Symbolhandle /*list*/);
/* Subassig.c */
Symbolhandle Subassign(Symbolhandle /*arg1*/, Symbolhandle /*list*/, Symbolhandle /*arg2*/,long /*checkArg1*/);
/* Symbol.c */
void Setdims(Symbolhandle /*symh*/, long /*ndims*/, long /*dims*/ []);
void Setname(Symbolhandle /*symh*/,char * /*name*/);
void setLabelsDim(Symbolhandle /*symh*/);
void setNotesDim(Symbolhandle /*symh*/);
int setLabels(Symbolhandle /*symh*/, char ** /*labels*/);
int setNotes(Symbolhandle /*symh*/, char ** /*notes*/);
void clearLabels(Symbolhandle /*symh*/);
void clearNotes(Symbolhandle /*symh*/);
Symbolhandle Findspecial(char * /*s*/);
Symbolhandle Lookup(char * /*s*/);
Symbolhandle Makesymbol(long /*type*/);
Symbolhandle Makespecialsymbol(long /*type*/, char * /*name*/,
					   Symbolhandle (* /*setSpecial*/)(Symbolhandle,long),
					   Symbolhandle (* /*getSpecial*/)(Symbolhandle));
Symbolhandle Makestruc(long /*ncomp*/);
Symbolhandle Makereal(long /*length*/);
Symbolhandle Makechar(long /*length*/);
Symbolhandle Makelong(long /*length*/);
void Addsymbol(Symbolhandle /*symh*/);
void Cutsymbol(Symbolhandle /*symh*/);
long Symbolinit(void);
Symbolhandle Firstsymbol(int /*scratch*/);
Symbolhandle Lastsymbol(void);
Symbolhandle Nextsymbol(Symbolhandle /*symh*/, int /*scratch */);
Symbolhandle Prevsymbol(Symbolhandle /*symh*/);
Symbolhandle Install(char * /*name*/, long /*type*/);
Symbolhandle RInstall(char * /*name*/, long /*length*/);
Symbolhandle CInstall(char * /*name*/, long /*length*/);
Symbolhandle GraphInstall(char * /*name*/);
Symbolhandle LongInstall(char * /*name*/, long /*length*/);
Symbolhandle GarbInstall(long /*length*/);
Symbolhandle StrucInstall(char * /*name*/, long /*ncomp*/);
Symbolhandle RSInstall(char * /*name*/, long /*ncomp*/, char * /*compnames*/[], long /*length*/);
Symbolhandle RSInstall2(char * /*name*/, long /*ncomp*/, char ** /*compnames*/, long /*length*/);
Symbolhandle RSInstall3(char * /*name*/, long /*ncomp*/, char ** /*compnames*/, long /*ncomp2*/, char * /*compnames2*/[], long /*length*/);
void Remove(char * /*name*/);
void Removesymbol(Symbolhandle /*symh*/);
void Removelist(Symbolhandle /*list*/);
void Delete(Symbolhandle /*symh*/);
void DeleteContents(Symbolhandle /*symh*/);
void Unscratch(void);
void CleanupList(Symbolhandle /*list*/);
void RemoveUndef(void);
void cleanAssignedStack(void);
long isPendingAssign(Symbolhandle /*arg*/);
Symbolhandle Assign(Symbolhandle /*arg1*/, Symbolhandle /*arg2*/, long /*keyword*/);
int Copy(Symbolhandle /*arg1*/, Symbolhandle /*arg2*/);
int CopyDoubleToLong(Symbolhandle /*from*/, Symbolhandle /*to*/);
int CopyLongToDouble(Symbolhandle /*from*/, Symbolhandle /*to*/);
Symbolhandle reuseArg(Symbolhandle /*list*/, long /*argno*/,
					  int /*keeplabels*/, int /*keepnotes*/);
Symbolhandle Extract(Symbolhandle /*strarg*/, Symbolhandle /*namearg*/, long /*named*/);
Symbolhandle Makelist(void);
Symbolhandle Addlist(Symbolhandle /*list*/, Symbolhandle /*arg*/);
Symbolhandle Growlist(Symbolhandle /*mylist*/);
Symbolhandle Byname(Symbolhandle /*symh*/, long * /*byNameType*/, long * /*parseMacro1*/);
void Dumpsymbols(int /* whichtable*/, char * /*name*/, long /*count*/);
Symbolhandle Buildlist(long /*nargs*/, long /*typeList*/[], char * /*valueList*/[], char * /*keyList*/[]);
long SizeofSymbol(Symbolhandle /*symh*/);
/* anovacoe.c */
Symbolhandle anovacoefs(Symbolhandle /*list*/);
/* array.c */
Symbolhandle array(Symbolhandle /*list*/);
/* betabase.c */
double macheps(void);
void betabase(double * /*x*/, double * /*a*/, double * /*b*/, long * /*gia*/, long * /*gib*/, double * /*cdf*/);
double betai(double /*x*/, double /*pin*/, double /*qin*/);
double ppbeta(double /*p*/, double /*a*/, double /*b*/, long * /*ifault*/);
double betanc(double /*x*/,double /*a*/,double /*b*/,double /*lambda*/,long * /*ifault*/);
/* bin.c */
Symbolhandle bin(Symbolhandle /*list*/);
/* boxplot.c */
void boxprep(double * /*xsc*/, long /*n*/, double /*fivenum*/[],
			 long /*nout*/[]);
Symbolhandle boxplot(Symbolhandle /*list*/);
/* cellstat.c */
double cellstat(double ** /*cury*/, long /*term*/[], double ** /*tmeans*/, double ** /*tvars*/, double ** /*tcounts*/, long /*balanova*/);
/* cellstts.c */
Symbolhandle cellstats(Symbolhandle /*list*/);
/* changest.c */
Symbolhandle changestr(Symbolhandle /*list*/);
/* cholesky.c */
Symbolhandle cholesky(Symbolhandle /*list*/);
/* cluster.c */
Symbolhandle cluster(Symbolhandle /*list*/);
/* columnop.c */
Symbolhandle columnops(Symbolhandle /*list*/);
/* comIface.c */
Symbolhandle getHistory(Symbolhandle /*list*/);
/* comclipb.c */
long iniClipboard(char * /*name*/);
/* comhandl.c */
char **mygethandle(long /*n*/);
char **mygetsafehandle(long /*n*/);
void mydisphandle(char ** /*h*/);
void mydisphandle5(char ** /*h1*/, char ** /*h2*/, char ** /*h3*/, char ** /*h4*/, char ** /*h5*/);
void mydispnhandles(char *** /*h*/, long /*n*/);
char **mygrowhandle(char ** /*h*/, long /*n*/);
char **mygrowsafehandle(char ** /*h*/, long /*n*/);
char **myduphandle(char ** /*h*/);
char **mydupsafehandle(char ** /*h*/);
long myvalidhandle(char ** /*h*/);
long myhandlelength(char ** /*h*/);
void mylockhandle(char ** /*h*/);
void myunlockhandle(char ** /*h*/);
char *mygetpointer(long /*n*/);
void myfreepointer(char * /*p*/);
/* commonio.c */
void myprint(char * /*msg*/);
void mymultiprint(char * /*msg*/, FILE* /*fp*/, int /*addNL*/);
void myeol(void);
void putOutMsg(char * /*Msg*/);
void myerrorout(char * /*msg*/);
void putPieces(char * /*piece1*/, char * /*piece2*/, char * /*piece3*/, char * /*piece4*/);
void echoLINE(void);
void putOUTSTR(void);
void putErrorOUTSTR(void);
void putOutErrorMsg(char * /*msg*/);
int  myfeof(FILE * /*fp*/);
void prexpr(Symbolhandle /*symh*/);
void fprexpr(Symbolhandle /*symh*/, FILE * /*fp*/, long /*labels*/,
			 char ** /*missingcode*/);
void matWrite(Symbolhandle /*arg*/, FILE * /*fp*/, double /*missValue*/,
			  int /*header*/, int /*labels*/, int /*notes*/,
			  char * /*separator*/, int /*charFormat*/,
			  int /*oldStyle*/, char * /*name*/,
			  Symbolhandle /*symhComment*/);
void macWrite(Symbolhandle /*arg*/, FILE * /*fp*/,
			  int /*header*/, int /*notes*/, int /*oldStyle*/,
			  char * /*name*/, Symbolhandle /*symhComment*/);
int fillLINE(FILE * /*fn*/);
int getinput(void);
int pushInputlevel(char * /*name*/, long /*start*/,
				   unsigned char ** /*inputstring*/);
void popInputlevel(void);
void cleanInputlevels(int /*all*/);
/* concat.c */
Symbolhandle concat(Symbolhandle /*arglist*/);
/* contrast.c */
Symbolhandle contrast(Symbolhandle /*list*/);
/* cor.c */
Symbolhandle cor(Symbolhandle /*list*/);
/* daxpy.c */
void daxpy_(long * /*pn*/, double * /*pa*/, double * /*x*/, long * /*pincx*/, double * /*y*/, long * /*pincy*/);
/* ddot.c */
double ddot_(long * /*pn*/, double * /*x*/, long * /*pincx*/, double * /*y*/, long * /*pincy*/);
/* delete.c */
Symbolhandle deleter(Symbolhandle /*list*/);
/* describe.c */
Symbolhandle describe(Symbolhandle /*list*/);
/* dim.c */
Symbolhandle dim(Symbolhandle /*list*/);
Symbolhandle diag(Symbolhandle /*list*/);
/* dunnett.c */
double PDunnett(double /*x*/, double /*ngroup*/, double /*groupSizes*/ [],
				double /*edf*/, long /*twosided*/, double /*epsilon*/);
double QDunnett(double /*x*/, double /*ngroup*/,
				double /*groupSizes*/ [], double /*edf*/, long /*twosided*/,
				double /*epsilon*/, long * /*error*/);
/* dynload.c */
Symbolhandle dynload(Symbolhandle /*list*/);
Symbolhandle callC(Symbolhandle /*list*/);
Symbolhandle asLong(Symbolhandle /*list*/);
/* eigen.c */
Symbolhandle eigen(Symbolhandle /*list*/);
/* eigutils.c */
void rsg(long /*nm*/, long /*n*/, double * /*a*/, double * /*b*/, double * /*w*/, long /*matz*/, double * /*z*/, double * /*fv1*/, double * /*fv2*/, long * /*ierr*/);
void tql2(long /*nm*/, long /*n*/, double * /*d*/, double * /*e*/, double * /*z*/, long * /*ierr*/);
void tqlrat(long /*n*/, double * /*d*/, double * /*e2*/, long * /*ierr*/);
void tred1(long /*nm*/, long /*n*/, double * /*a*/, double * /*d*/, double * /*e*/, double * /*e2*/);
void tred2(long /*nm*/, long /*n*/, double * /*a*/, double * /*d*/, double * /*e*/, double * /*z*/);
void eispsvd(long /*nm*/, long /*m*/, long /*n*/, double * /*a*/, double * /*w*/, long /*matu*/, double * /*u*/, long /*matv*/, double * /*v*/, double * /*rv1*/, long * /*ierr*/);
void tridib(long /*n*/, double * /*eps1*/, double * /*d*/, double * /*e*/,
			double * /*e2*/, double * /*lb*/,double * /*ub*/, long /*m11*/,
			long /*m*/, double * /*w*/, long * /*ind*/, long * /*ierr*/,
			double * /*rv4*/, double * /*rv5*/);

void tinvit(long /*nm*/, long /*n*/, double * /*d*/, double * /*e*/,
			double * /*e2*/, long /*m*/, double * /*w*/,long * /*ind*/,
			double * /*z*/, long * /*ierr*/, double * /*rv1*/,
			double * /*rv2*/, double * /*rv3*/,double * /*rv4*/,
			double * /*rv6*/);
/* errorMsg.c */
char *inWhichMacro(void);
void badType(char * /*who*/, long /*type*/, long /*argno*/);
void badDims(char * /*who*/, long /*ndims*/, long /*argno*/);
void badNargs(char * /*who*/, long /*nargs*/);
void invalidSymbol(char * /*who*/, long /*argno*/);
void noData(char * /*who*/, long /*argno*/);
void undefArg(char * /*who*/, Symbolhandle /*arg*/, long /*argno*/);
void badKeyword(char * /*who*/, char * /*keyword*/);
void badLabels(unsigned long /*whatError*/);
void notTorF(char * /*keyword*/);
void notCharOrString(char * /*keyword*/);
void notPositiveReal(char * /*keyword*/);
void notNonNegativeReal(char * /*keyword*/);
void notNumberOrReal(char * /*keyword*/);
void notPositiveInteger(char * /*keyword*/);
void notNonNegativeInteger(char * /*keyword*/);
void notInteger(char * /*keyword*/);
void notPositiveLong(char * /*keyword*/);
void notNonNegativeLong(char * /*keyword*/);
void notLong(char * /*keyword*/);
void notImplemented(char * /*who*/);
void resultTooBig(char * /*who*/);
/* fft.c */
Symbolhandle fft(Symbolhandle /*list*/);
/* funbalan.c */
long funbalanova(long /*maxiter*/, double /*epsilon*/);
/* gammabas.c */
void gammabase(double * /*x*/, double * /*a*/, double * /*p*/);
double ppgamma(double /*p*/, double /*a*/, long * /*ifault*/);
/* getmodel.c */
long getmodel(Symbolhandle /*list*/, long /*nargs*/, long /*startKey*/,
			  long /*silent*/, Symbolhandle /*symhWts*/,
			  Symbolhandle /*symhOffset*/, Symbolhandle /*symhN*/);
/* glm.c */
Symbolhandle glm(Symbolhandle /*list*/);
void glmcleanup(int /*removeStrmodel*/);
void clearGlobals(void);
/* glmutils.c */
long glmInit(void);
void getterm(modelPointer /*jterm*/, long /*term*/[], long /*nvars*/);
long setterm(long /*term*/[], long /*nvars*/, long /*nclasses*/[]);
long stringToTerm(char * /*string*/, long /*term*/ []);
void stepGlmOdometer(long /*term*/ [], long /*nclasses*/ [], long /*nvars*/,
					 long /*reverse*/);
void effectiveTerm(long /*term*/[], modelPointer /*effTerm*/, long /*nvars*/);
long inEffectiveTerm(modelPointer /*term*/,modelPointer/*effTerm*/);
long inTerm(long /*jvar*/, modelPointer /*term*/);
long nvInTerm(modelPointer /*term*/);
long moreInTerm(long /*jvar*/, modelPointer /*term*/, long /*nvars*/);
long modeltermEqual(modelPointer /*term1*/,modelPointer /*term2*/);
void modeltermAssign(modelPointer /*from*/, modelPointer /*to*/);
char **setStrmodel(char ** /*modelString*/, long /*clean*/);
long inEarlierTerm(long /*termi*/, long /*term*/[], modelInfoPointer /*info*/);
long getAlpha(long /*termi*/, long /*colstart*/, double *** /*alphaH*/,
			  long *** /*colptrH*/, long /*transposed*/);
void compLincom(double * /*xrow*/, double * /*lincom*/,
				double * /*seEst*/, double * /*sePred*/);
void deLink(double * /*predVec*/, double * /*seVec*/, double * /*binomn*/,
				 long /*incN*/, long /*nrows*/);
long margSS(void);
void seqSS(void);
void compDF(void);
long nAliased(void);
long Xsetup(void);
void xtxinvInit(void);
long buildXmatrix(long /*termi*/, double * /*regx*/,
				  modelInfoPointer /*info*/);
long buildXrow(double * /*x*/, double * /*regx*/, modelInfoPointer /*info*/);
long checkVariables(modelInfoPointer /*info*/);
long countMissing(modelInfoPointer /*info*/, double *** /*misswts*/);
char *linkName(unsigned long /*control*/);
char *distName(unsigned long /*control*/);
unsigned long linkControl(char * /*name*/);
unsigned long distControl(char * /*name*/);
char * glmName(long /*op*/);
/* gramschm.c */
void gramschmidt(long /*control*/);
/* graphics.c */
/* prototypes are in plot.h (960617) */
/* hconcat.c */
Symbolhandle hconcat(Symbolhandle /*arglist*/);
/* iniMacro.c*/
long iniMacros(void);
/* initiali.c */
void initialize(char * /*helpFileName*/,char * /*dataFileName*/,
				char * /*macroFileName*/, char * /*dataPath*/,
				char * /*macroPath*/, char * /*homePath*/,
				promptType /*prompt*/);
/* ipf.c */
long ipf(long /*maxiter*/, double /*epsilon*/);
/* iswhat.c */
Symbolhandle iswhat(Symbolhandle /*list*/);
Symbolhandle keyvalue(Symbolhandle /*list*/);
Symbolhandle nameOf(Symbolhandle /*list*/);
Symbolhandle renamer(Symbolhandle /*arglist*/);
/* iterfns.c */
void glmvar(long /*type*/);
void glmlink(long /*type*/);
double glmdev(long /*type*/);
void glmresid(void);
/* iterglm.c */
long iterglm(long /*maxiter*/, double /*epsilon*/, double /*clamp*/);
/* keywords.c */
long findKeyword(Symbolhandle /*list*/, char * /*word*/, long /*start*/);
long getKeyValues(Symbolhandle /*list*/, long /*startKey*/, long /*op*/,
					keywordListPtr /*keyInfo*/);
long getOneKeyValue(Symbolhandle /*symhKey*/, long /*op*/,
					keywordListPtr /*keys*/, long /*nkeys*/);
long getAllKeyValues(Symbolhandle /*list*/, long /*startKey*/, long /*op*/,
					 keywordListPtr /*keyList*/, long /*nkeys*/);
void unsetKeyValues(keywordListPtr /*keys*/, long /*nkeys*/);
long matchKey(char * /*name*/, char * /*charvals*/[], long /*codes*/[]);
/* kmeans.c */
Symbolhandle kmeans(Symbolhandle /*list*/);
/* labutils.c */
void getAllLabels(Symbolhandle /*arg*/, char * /*labels*/ [],
			   long /*widths*/ [], long /*lengths*/ []);
char * getOneLabel(Symbolhandle /*symh*/, long /*dimension*/, long /*index*/);
void getMatLabels(Symbolhandle /*arg*/, char * /*rowcol*/ [2],
				  long /*lengths*/ [2]);
char **createLabels(long /*ndims*/, long /*lengths*/ []);
char ** growLabels(char ** /*oldlabels*/, long /*length*/);
void buildLabels(char ** /*labelsH*/, char * /*labels*/ [], long /*dims*/ [],
				 long /*ndims*/);
long expandLabels(char * /*root*/, long /*size*/, char * /*labels*/);
long moveMatLabels(Symbolhandle /*from*/, Symbolhandle /*to*/,
				   unsigned long /*control*/);
long fixupMatLabels(Symbolhandle /*symh*/, unsigned long /*control*/);
long transferLabels(Symbolhandle /*from*/, Symbolhandle /*to*/);
void appendLabels(Symbolhandle /*symh*/, char * /*newLabels*/, long /*jdim*/,
				  long /*expand*/);
void appendNLabels(Symbolhandle /*symh*/, char * /*newLabels*/, long /*jdim*/,
				   long /*startIndex*/, long /*nlabels*/);
long installLabels(Symbolhandle /*symhLabels*/, Symbolhandle /*result*/);
unsigned long checkLabels(Symbolhandle /*symhLabels*/, long /*ndims*/, 
						  long /*dims*/ []);
long anyLabels(Symbolhandle /*symh*/);
long sizeOfLabels(Symbolhandle /*symh*/);
long sizeOfNotes(Symbolhandle /*symh*/);
long prepPrintLabels(Symbolhandle /*symh*/,
					    char * /*dimLabels*/ [], long /*dimLabelsWidth*/ []);
void putColumnLabels(FILE * /*fp*/, long /*lastdim*/, char * /*labels*/,
					 long /*labelWidth*/, long /*fieldWidth*/,
					 long /*nperLine*/, long /*type*/);
void putRowLabels(FILE * /*fp*/, long /*ndims*/, long /*coord*/ [],
				  long /*labelWidth*/, char * /*labels*/ [],
				  long * /*labelsWidth*/, long /*charData*/);
Symbolhandle getlabs(Symbolhandle /*list*/);
Symbolhandle setlabs(Symbolhandle /*list*/);
/* list.c */
Symbolhandle listbrief(Symbolhandle /*arglist*/);
/* lpsforwd.c */
long leapsforwd(long * /*longparms*/, double * /*RR*/, double * /*XI*/,
	double * /*XN*/, double * /*XM*/, double * /*DD*/, double * /*D*/,
	double * /*DT*/, double * /*CO*/, double * /*SC*/, double * /*SQ*/,
	double * /*CL*/, double * /*RM*/, long * /*INN*/, long * /*IPP*/,
	long * /*IND*/, long * /*INT*/, double * /*S2*/, double * /*TL*/,
	double * /*FD*/, long * /*leapsNE*/, long * /*IV*/);
/* lpslandb.c */
long leapslandb(double * /*XI*/, double * /*XN*/, double * /*D*/, double * /*YI*/,
	double * /*CI*/, double * /*CN*/, double * /*CO*/, double * /*CL*/,
	double * /*RM*/, long * /*IND*/, long * /*NS*/, long * /*ND*/, double * /*DF*/,
	long * /*IB*/, long * /*MB*/, long * /*IZ*/, long * /*MN*/, long * /*KY*/,
	double * /*SIG*/, double * /*PEN*/, double * /*ZIP*/, long * /*KX*/,
	long * /*KZ*/, long * /*NI*/, long * /*IV*/);
/* lpsmisc.c */
void leapspivot(long * /*N*/, long * /*ML*/, long * /*LS*/, long * /*LX*/,
	double * /*XN*/, long * /*INT*/, long * /*IND*/, double * /*CN*/,
	double * /*CO*/, double * /*DT*/, long * /*IQ*/, long * /*ND*/, long * /*NN*/,
	long * /*KY*/, long * /*LL*/, long * /*LY*/);
void leapstest(double * /*WT*/, long * /*N*/, double * /*XI*/, long * /*INT*/, double * /*D*/, double * /*DD*/, double * /*SQ*/, double * /*DT*/, long * /*ML*/, long * /*MN*/, long * /*ND*/);
void leapsqstore(double * /*RS*/, double * /*CN*/, double * /*CL*/, double * /*RM*/, long * /*MB*/, double * /*DF*/, long * /*MN*/, double * /*SIG*/, double * /*PEN*/, long * /*IB*/, long * /*NS*/);
void leapscopy(double * /*XN*/, double * /*XI*/, long * /*INT*/, long * /*IND*/, long * /*ML*/, long * /*MT*/, long * /*MP*/, long * /*ND*/, double * /*SC*/, double * /*CN*/, double * /*CI*/, long * /*IQ*/);
void leapstrans(double * /*TR*/, long * /*MR*/, double * /*RS*/, double * /*DF*/, long * /*MN*/, double * /*SIG*/, double * /*PEN*/, long * /*IB*/, long * /*MB*/);
/* macro.c */
long prepMacro(char * /*text*/, char * /*newText*/, int /*addDollars*/);
Symbolhandle macro(Symbolhandle /*list*/);
/* mainpars.c */
void yyerror(char * /*s*/);
void cleanitup(void);
int yyparse(void);
/* makefact.c */
Symbolhandle makefactor(Symbolhandle /*list*/);
/* makestr.c */
Symbolhandle makestr(Symbolhandle /*list*/);
Symbolhandle compnames(Symbolhandle /*list*/);
/* mathutil.c*/
long doSwp(double * /*cp*/,long /*m*/,long /*n*/,long /*k*/,double /*cpdiag*/,
		   long /*full*/);
double fsolve(double /*arg*/, double (* /*func*/) (double ,double [], long, long *),
			  double /*param*/ [], long /*nparam*/, double /*xmax*/,
			  double /*xmin*/, double /*eps*/, long /*itmax*/, long /*ntab*/,
			  double /*x*/ [], double /*y*/ [], double /*pr*/ [],
			  long /*report*/ []);
double mystrtod(char * /*nptr*/, char ** /*eptr*/);
double pythag(double /*a*/, double /*b*/);
double intpow(double /*x*/, double /*p*/);
double mylgamma(double /*x*/);
double          epslon(double /*x*/);
/* matrix.c */
Symbolhandle matrix(Symbolhandle /*list*/);
/* movavg.c */
Symbolhandle movavg(Symbolhandle /*list*/);
/* mreader.c */
int modelparse(void);
int initmodelparse(char ** /*model*/);
modelInfoPointer getModelInfo(void);
/* myplot.c */
Symbolhandle myplot(Symbolhandle /*list*/);
/* normbase.c */
void normbase(double * /*x*/, double * /*phi*/);
double ppnd(double /*p*/, long * /*ifault*/);
/* outer.c */
Symbolhandle outer(Symbolhandle /*list*/);
/* partacf.c */
Symbolhandle partacf(Symbolhandle /*list*/);
/* paste.c */
Symbolhandle multipaste(Symbolhandle /*arg*/, char * /*format*/,
	char ** /*stringForMissing*/, char /*fieldSep*/, char /*lineSep*/);
Symbolhandle paste(Symbolhandle /*list*/);
/* plotutil.c*/
void set_point(struct curve_points ** /*plot*/, long /*ipoint*/,
			   double /*x*/, double /*y*/, char * /*string*/,
			   unsigned char /*pointColor*/, unsigned char /*linecolor*/);
void undef_point(struct curve_points ** /*plot*/,long /*ipoint*/);
long do_whole_graph(whole_graph ** /*graph*/, FILE * /*fp*/,
					long /*termType*/, long /*windno*/,
					plotKeyValuesPtr /*keyValues*/);
void deleteGraph(struct whole_graph ** /*graph*/);
struct whole_graph ** copyGraph(struct whole_graph ** /*graph*/);
void getGraphExtremes(whole_graph ** /*graph*/, double * /*extremes*/,
					  long /*pointsOnly*/);
long getPlotKeys(Symbolhandle/*list*/,long /*startKey*/,
				 plotKeyValuesPtr /*keyValues*/);
void unsetPlotKeys(plotKeyValuesPtr /*keyValues*/, long /*all*/);
long checkPlotKeys(long /*toFile*/, plotKeyValuesPtr /*keyValues*/,
				   long * /*termType*/);
int saveGraph(Symbolhandle /*symh*/, char * /*name*/);
int restoreGraph(Symbolhandle /*symh*/);
FILE *openPlotFile(char * /*fileName*/,long /*new*/);
long sizeofPlot(whole_graph ** /*graph*/);
int screenDump(char * /*fname*/);
/* polyroot.c */
Symbolhandle polyroot(Symbolhandle /*list*/);
/* power.c */
Symbolhandle samplesize(Symbolhandle /*list*/);
Symbolhandle power1(Symbolhandle /*list*/);
double Fquant(double /*p*/, double /*n1*/, double /*n2*/);
/* predtabl.c */
Symbolhandle predtable(Symbolhandle /*list*/);
/* printano.c */
void printglm(long /*fstats*/, long /*pvals*/, long /*sssp*/, long /*byvar*/);
/* printreg.c */
void printregress(long /*pvals*/);
/* pvals.c */
Symbolhandle cumcdf(Symbolhandle /*list*/);
/* pvalsub.c */
double Cbet(double /*x*/, double /*a*/, double /*b*/);
double Qbeta(double /*p*/, double /*a*/, double /*b*/);
double Cbin(long /*x*/, long /*n*/, double /*P*/);
double Cchi(double /*x*/, double /*f*/);
double Qchi(double /*p*/, double /*f*/);
double Cgam(double /*x*/, double /*a*/, double /*b*/);
double Qgam(double /*p*/, double /*a*/, double /*b*/);
double noncenCchi(double /*x*/, double /*f*/, double /*lam*/);
double noncenQchi(double /*p*/, double /*f*/, double /*lam*/, double /*eps*/);
double Chyp(long /*x*/, long /*M*/, long /*N*/, long /*K*/);
double Cnor(double /*x*/);
double Qnor(double /*p*/);
double Cpoi(long /*x*/, double /*l*/);
double Csne(double /*x*/, double /*n1*/, double /*n2*/);
double Qsne(double /*p*/, double /*n1*/, double /*n2*/);
double noncentF(double /*x*/, double /*lam*/, double /*n1*/, double /*n2*/);
double noncentBeta(double /*x*/, double /*lam*/, double /*a*/, double /*b*/);
double Cstu(double /*x*/, double /*n*/);
double Qstu(double /*p*/, double /*n*/);
double noncenCstu(double /*x*/, double /*n*/, double /*noncen*/);
/* qr.c */
Symbolhandle qr(Symbolhandle /*list*/);
/* random.c */
void randomSeed(long /*verbose*/);
Symbolhandle rangen(Symbolhandle /*list*/);
Symbolhandle setseed(Symbolhandle /*list*/);
Symbolhandle getseed(Symbolhandle /*list*/);
long iuni(void);
double duni(void);
void vuni(long /*count*/, double * /*rvec*/);
void vnorm(long /*count*/, double * /*rvec*/);
/* rankquic.c */
void rankquick(double /*v*/ [], double * /*rank*/, double * /*scratch*/, long /*n*/, long /*op*/);
void rankquickchar(char * /*v*/[], double /*rank*/ [], double /*scratch*/ [],
			   long /*n*/,long /*op*/);
/* rational.c */
Symbolhandle rational(Symbolhandle /*list*/);
/* regpred.c */
Symbolhandle regpred(Symbolhandle /*list*/);
/* rep.c */
Symbolhandle rep(Symbolhandle /*arglist*/);
/* rotation.d */
Symbolhandle rotatefac(Symbolhandle /*list*/);
/* run.c */
Symbolhandle run(Symbolhandle /*arglist*/);
/* screen.c */
Symbolhandle screen(Symbolhandle /*list*/, long /*verbose*/);
long leapsqprint(long * /*ind*/, long * /*mn*/, double * /*rm*/, double * /*tss*/, double * /*s2*/, double * /*scale*/, double * /*df*/, long * /*it*/, long * /*ib*/, double * /*pen*/);
long leapsscreen(long * /*NV*/, long * /*IT*/, long * /*KX*/, long * /*NF*/,
	long * /*NO*/, long * /*IB*/, double * /*FD*/, long * /*MB*/, double * /*RT*/,
	long * /*ND*/, long * /*NC*/, long * /*IW*/, long * /*NW*/, double * /*RW*/,
	long * /*NR*/, double * /*TL*/, double * /*S2*/, long * /*leapsNE*/,
	long * /*IV*/);
/* select.c */
Symbolhandle selecter(Symbolhandle /*list*/);
/* setOptio.c */
void setDefaultOptions(void);
Symbolhandle setoptions(Symbolhandle /*list*/);
Symbolhandle getOpt(short * /*whichOptions*/);
void setOpt(Symbolhandle /*list*/, long /*quiet*/);
long isOption(char * /*keyword*/);
Symbolhandle elapsedTime(Symbolhandle /*list*/);
/* shell.c */
Symbolhandle shell(Symbolhandle /*list*/);
int processBang(void);
/* solve.c */
Symbolhandle solve(Symbolhandle /*list*/);
/* sort.c */
Symbolhandle sort(Symbolhandle /*list*/);
/* sortquic.c */
void sortquick(double /*v*/[], long /*n*/);
void sortquickchar(char * /*v*/ [], long /*n*/);
/* split.c */
Symbolhandle split(Symbolhandle /*list*/);
/* stemleaf.c */
Symbolhandle stemleaf(Symbolhandle /*list*/);
/* stemsubs.c */
long doStemLeaf(double ** /*data*/, long /*n*/, long /*width*/,
				long /*maxlines*/, long /*xtrems*/, long /*verbose*/,
				long /*depth*/, char ** /*title*/);
/* studentb.c */
void studentbase(double * /*x*/, double * /*df*/, double * /*cdf*/);
double ppstudent(double /*pp*/, double /*n*/, long * /*ifault*/);
/* studrang.c */
double Pstudrange(double /*x*/, double /*ngrp*/, double /*df*/,
				  double /*eps*/);
double Qstudrange(double /*p*/, double /*ngrp*/, double /*df*/,
				  double /*eps*/);
/* svd.c */
Symbolhandle svd(Symbolhandle /*list*/);
/* swp.c */
Symbolhandle swp(Symbolhandle /*arglist*/);
/* tabs.c */
Symbolhandle tabs(Symbolhandle /*list*/);
/* toeplitz.c */
Symbolhandle toeplitz(Symbolhandle /*list*/);
/* trans.c */
Symbolhandle Transform(Symbolhandle /*list*/);
/* transpos.c */
Symbolhandle    doTranspose(Symbolhandle /*arg*/, double * /*params*/,
								   unsigned long /*control*/,
								   unsigned long * /*status*/);
Symbolhandle transpose(Symbolhandle /*list*/);
/* trideige.c */
Symbolhandle trideigen(Symbolhandle /*list*/);
/* tserops.c */
Symbolhandle tserops(Symbolhandle /*list*/);
/* ttests.c */
Symbolhandle tval(Symbolhandle /*list*/);
Symbolhandle t2val(Symbolhandle /*list*/);
Symbolhandle tint(Symbolhandle /*list*/);
Symbolhandle t2int(Symbolhandle /*list*/);
/* unbalano.c */
long unbalanova(void);
/* utils.c */
long argOK(Symbolhandle /*arg*/, long /*type*/, long /*argno*/);
long isTorF(Symbolhandle /*arg*/);
long isNumber(Symbolhandle /*arg*/, int /*kind*/);
long isInteger(Symbolhandle /*arg*/, int /*kind*/);
long isCharOrString(Symbolhandle /*arg*/);
long isNull(Symbolhandle /*arg*/);
long isDefined(Symbolhandle /*arg*/);
long isAssigned(Symbolhandle /*arg*/);
long isScalar(Symbolhandle /*arg*/);
long isVector(Symbolhandle /*arg*/);
long isMatrix(Symbolhandle /*arg*/, long * /*dim*/);
long isFactor(Symbolhandle /*symh*/);
char *isKeyword(Symbolhandle /*arg*/);
int checkArgType(Symbolhandle /*arg*/, char * /*what*/,
				 long /*targetType*/);
int checkBalance(unsigned char * /*line*/, char * /*name*/);
long checkSymmetry(double * /*a*/, long /*nrows*/);
long symbolSize(Symbolhandle /*arg*/);
long isTooBig(long /*n1*/, long /*n2*/, size_t /*size*/);
void getDims(long * /*dims*/, Symbolhandle /*arg*/);
double decodeString(char * /*string*/, char * /*seps*/, long * /*error*/);
char findBracket(long /*goalLevel*/, char /*leftbracket*/, 
				 unsigned char * /*inputString*/,
				 long * /*thisLevel*/, long * /*thisPlace*/, 
				 long * /*lastchar*/);
long copyField(char * /*line*/, char * /*outstr*/);
long copyQuotedField(char * /*line*/, char * /*outstr*/, long /*commaOK*/,
					 long /*seekQuote*/, long * /*error*/);
int nDigits(long /*N*/);
long setFormat(char * /*format*/, long /*fmt*/[]);
void installFormat(long /*beforeDec*/, long /*afterDec*/, long /*fmtType*/);
void installScreenDims(long /*width*/, long /*height*/);
void saveFormat(void);
void restoreFormat(void);
int anyMissing(Symbolhandle /*a*/);
int anyNaN(Symbolhandle /*a*/);
int anyInfinite(Symbolhandle /*a*/);
int anyDoubleMissing(double * /*a*/, long /*n*/);
int anyDoubleNaN(double * /*a*/, long /*n*/);
int anyDoubleInfinite(double * /*a*/, long /*n*/);
void stepOdometer(long /*ind*/ [], long /*bounds*/ [], long /*ndims*/,
				  long /*base*/, long /*reverse*/);
void setScratchName(Symbolhandle /*symh*/);
void setScratchMacroName(Symbolhandle /*symh*/, char * /*macroName*/);
void setCompName(Symbolhandle /*symh*/, char * /*name*/);
char *typeName(long /*type*/);
char *opName(long /*op*/);
char *tokenName(long /*token*/);
char *fieldStart(char * /*stringBuf*/);
void trimBuffer(char * /*buffer*/, unsigned long /*control*/);
long indentBuffer(char * /*buffer*/, int /*nplaces*/);
long centerBuffer(char * /*buffer*/, char * /*value*/, long /*length*/);
long formatChar(char * /*buffer*/, char * /*value*/, unsigned long /*control*/);
long formatDouble(char * /*buffer*/, double /*value*/, unsigned long /*control*/);
long escapedOctal(unsigned char /*c*/, unsigned char * /*buffer*/);
char * n_th(long /*n*/);
char *skipStrings(char * /*ch*/, long /*l*/);
char *copyStrings(char * /*in*/, char * /*out*/, long /*l*/);
short scanPat(char * /*string*/,long * /*matchType*/, char * /*pattern*/,
			  int /*checkName*/, long /*maxLength*/);
short matchName(char * /*name*/, long /*matchType*/, char * /*pattern*/);
long mystrncmp(char * /*s1*/, char * /*s2*/, int /*n*/);
char *getTimeAndDate(void);
double mygettime(void);
void getElapsedTime(double /*times*/ [2]);
void symExtremes(Symbolhandle /*symh*/, double /*extremes*/[2]);
void symFill(Symbolhandle /*symh*/, double /*value*/);
void doubleFill(double * /*x*/, double /*value*/, long /*length*/);
void doubleCopy(double * /*from*/, double * /*to*/, long /*length*/);
double doubleMin(double * /*x*/, long /*n*/);
double doubleMax(double * /*x*/, long /*n*/);
long longMin(long * /*x*/, long /*n*/);
long longMax(long * /*x*/, long /*n*/);
void doubleToLong(double * /*in*/, long * /*out*/, long /*length*/);
void longToDouble(long * /*in*/, double * /*out*/, long /*length*/);
/* utilstru.c */
Symbolhandle doRecur1(Symbolhandle (* /*doit*/) (Symbolhandle, double *, unsigned long, unsigned long *), Symbolhandle /*arg1*/,double * /*params*/,unsigned long /*control*/,unsigned long * /*status*/);
Symbolhandle doRecur2(Symbolhandle (* /*doit*/) (Symbolhandle, Symbolhandle, double *,
	unsigned long, unsigned long *),Symbolhandle /*arg1*/,
	Symbolhandle /*arg2*/,double * /*params*/, unsigned long /*control*/,
	unsigned long * /*status*/);
long doRecurCheck1(long (* /*checkit*/) (Symbolhandle, double *, unsigned long, unsigned long *),
	Symbolhandle /*arg1*/,double * /*params*/,unsigned long /*control*/,unsigned long * /*status*/);
long doRecurCheck2(long (* /*checkit*/) (Symbolhandle, Symbolhandle, double *,
	unsigned long, unsigned long *),Symbolhandle /*arg1*/,
	Symbolhandle /*arg2*/,double * /*params*/, unsigned long /*control*/,
	unsigned long * /*status*/);
long strucIsMatrix(Symbolhandle /*arg*/);
long strucIsVector(Symbolhandle /*arg*/);
long getStrucTypes(Symbolhandle /*symh*/);
long getSingleType(Symbolhandle/*symh*/);
long strucAnyNull(Symbolhandle /*arg*/);
long strucAnyType(Symbolhandle /*arg*/, long /*type*/);
long strucSymbolSize(Symbolhandle /*arg*/);
long strucSize(Symbolhandle /*arg*/);
long strucAnyMissing(Symbolhandle /*arg*/, unsigned long /*control*/);
long dimcmp(Symbolhandle /*arg1*/, Symbolhandle /*arg2*/, unsigned long /*control*/);
long treecmp(Symbolhandle /*str1*/, Symbolhandle /*str2*/, unsigned long /*control*/);
/* varnames.c */
Symbolhandle varnames(Symbolhandle /*list*/);
Symbolhandle modelvars(Symbolhandle /*list*/);
Symbolhandle xvariables(Symbolhandle /*list*/);
Symbolhandle xvarsrows(Symbolhandle /*list*/);
/* yates.c */
Symbolhandle yates(Symbolhandle /*list*/);
/* yylex.c */
int yylex(void);
long findLParen(int /*op*/, char /*paren*/);
#else /*NOPROTOTYPES*/
/* macMain. */
void myUnloadSeg(/*void * routineAddr*/);
/* Ifsetup.c */
void Ifsetup(/*Symbolhandle list, long op*/);
/* Lang.c */
Symbolhandle Unary(/*Symbolhandle arg, int op*/);
Symbolhandle Arith(/*Symbolhandle arg1, Symbolhandle arg2, int op*/);
Symbolhandle Logic(/*Symbolhandle arg1, Symbolhandle arg2, int op*/);
#ifdef UNDEFINED__
Symbolhandle doMatMult(/*Symbolhandle arg1, Symbolhandle arg2, long control*/);
Symbolhandle doUnary(/*Symbolhandle arg, long control, long *status*/);
Symbolhandle doBinary(/*Symbolhandle arg1, Symbolhandle arg2, long control, long *status*/);
#endif /*UNDEFINED__*/
Symbolhandle Element(/*Symbolhandle arg1, Symbolhandle list*/);
#ifdef UNDEFINED__
Symbolhandle elementMatrix(/*Symbolhandle arg, Symbolhandle subscriptsymh*/);
char **extractIt(/*Symbolhandle arg, Symbolhandle list, long resdim[]*/);
#endif /*UNDEFINED__*/
long subscriptLength(/*Symbolhandle subscriptsymh,long maxsubscr*/);
long **buildSubscript(/*Symbolhandle subscriptsymh, long maxsubscr,
						long *nitems, int * completeDim*/);
/* Macroset.c */
long Macrosetup(/*Symbolhandle macro, Symbolhandle list*/);
Symbolhandle mvEval(/*char ** commandH*/);
Symbolhandle evaluate(/*Symbolhandle list*/);
/* Subassig.c */
Symbolhandle Subassign(/*Symbolhandle arg1, Symbolhandle list, Symbolhandle arg2*/,/*long checkArg1*/);
/* Symbol.c */
void Setdims(/*Symbolhandle symh, long ndims, long dims []*/);
void Setname(/*Symbolhandle symh,char *name*/);
void setLabelsDim(/*Symbolhandle symh*/);
void setNotesDim(/*Symbolhandle symh*/);
int setLabels(/*Symbolhandle symh, char ** labels*/);
int setNotes(/*Symbolhandle symh, char ** notes*/);
void clearLabels(/*Symbolhandle symh*/);
void clearNotes(/*Symbolhandle symh*/);
Symbolhandle Findspecial(/*char * s*/);
Symbolhandle Lookup(/*char *s*/);
Symbolhandle Makesymbol(/*long type*/);
Symbolhandle Makespecialsymbol(/*long type, char * name,
					   Symbolhandle (* setSpecial)(Symbolhandle,long),
					   Symbolhandle (* getSpecial)(Symbolhandle)*/);
Symbolhandle Makestruc(/*long ncomp*/);
Symbolhandle Makereal(/*long length*/);
Symbolhandle Makechar(/*long length*/);
Symbolhandle Makelong(/*long length*/);
void Addsymbol(/*Symbolhandle symh*/);
void Cutsymbol(/*Symbolhandle symh*/);
long Symbolinit(/*void*/);
Symbolhandle Firstsymbol(/*int scratch*/);
Symbolhandle Lastsymbol(/*void*/);
Symbolhandle Nextsymbol(/*Symbolhandle symh, int scratch*/);
Symbolhandle Prevsymbol(/*Symbolhandle symh*/);
Symbolhandle Install(/*char *name, long type*/);
Symbolhandle RInstall(/*char *name, long length*/);
Symbolhandle CInstall(/*char *name, long length*/);
Symbolhandle GraphInstall(/*char*name*/);
Symbolhandle LongInstall(/*char *name, long length*/);
Symbolhandle GarbInstall(/*long length*/);
Symbolhandle StrucInstall(/*char *name, long ncomp*/);
Symbolhandle RSInstall(/*char *name, long ncomp, char *compnames[], long length*/);
Symbolhandle RSInstall2(/*char *name, long ncomp, char **compnames, long length*/);
Symbolhandle RSInstall3(/*char *name, long ncomp, char **compnames, long ncomp2, char *compnames2[], long length*/);
void Remove(/*char *name*/);
void Removesymbol(/*Symbolhandle symh*/);
void Removelist(/*Symbolhandle list*/);
void Delete(/*Symbolhandle symh*/);
void DeleteContents(/*Symbolhandle symh*/);
void Unscratch(/*void*/);
void CleanupList(/*Symbolhandle list*/);
void RemoveUndef(/*void*/);
void cleanAssignedStack(/*void*/);
long isPendingAssign(/*Symbolhandle arg*/);
Symbolhandle Assign(/*Symbolhandle arg1, Symbolhandle arg2, long keyword*/);
int Copy(/*Symbolhandle arg1, Symbolhandle arg2*/);
int CopyDoubleToLong(/*Symbolhandle from, Symbolhandle to*/);
int CopyLongToDouble(/*Symbolhandle from, Symbolhandle to*/);
Symbolhandle reuseArg(/*Symbolhandle list, long argno, int keeplabels,
					   int keepnotes*/);
Symbolhandle Extract(/*Symbolhandle strarg, Symbolhandle namearg, long named*/);
Symbolhandle Makelist(/*void*/);
Symbolhandle Addlist(/*Symbolhandle list, Symbolhandle arg*/);
Symbolhandle Growlist(/*Symbolhandle mylist*/);
Symbolhandle Byname(/*Symbolhandle symh, long *byNameType, long *parseMacro1*/);
void Dumpsymbols(/*int whichtable, char *name, long count*/);
Symbolhandle Buildlist(/*long nargs, long typeList[], char *valueList[], char *keyList[]*/);
long SizeofSymbol(/*Symbolhandle symh*/);
/* anovacoe.c */
Symbolhandle anovacoefs(/*Symbolhandle list*/);
/* array.c */
Symbolhandle array(/*Symbolhandle list*/);
/* betabase.c */
double macheps(/*void*/);
void betabase(/*double *x, double *a, double *b, long *gia, long *gib, double *cdf*/);
double betai(/*double x, double pin, double qin*/);
double ppbeta(/*double p, double a, double b, long *ifault*/);
double betanc(/*double x,double a,double b,double lambda,long * ifault*/);
/* bin.c */
Symbolhandle bin(/*Symbolhandle list*/);
/* boxplot.c */
void boxprep(/*double *xsc, long n, double fivenum[], long nout[]*/);
Symbolhandle boxplot(/*Symbolhandle list*/);
/* cellstat.c */
double cellstat(/*double **cury, long term[], double **tmeans, double **tvars, double **tcounts, long balanova*/);
/* cellstts.c */
Symbolhandle cellstats(/*Symbolhandle list*/);
/* changest.c */
Symbolhandle changestr(/*Symbolhandle list*/);
/* cholesky.c */
Symbolhandle cholesky(/*Symbolhandle list*/);
/* cluster.c */
Symbolhandle cluster(/*Symbolhandle list*/);
/* columnop.c */
Symbolhandle columnops(/*Symbolhandle list*/);
 comIface.c */
Symbolhandle getHistory(/*Symbolhandle list*/);
/* comclipb.c */
long iniClipboard(/*char * name*/);
/* comhandl.c */
char **mygethandle(/*long n*/);
char **mygetsafehandle(/*long n*/);
void mydisphandle(/*char **h*/);
void mydisphandle5(/*char **h1, char **h2, char **h3, char **h4, char **h5*/);
void mydispnhandles(/*char ***h, long n*/);
char **mygrowhandle(/*char **h, long n*/);
char **mygrowsafehandle(/*char **h, long n*/);
char **myduphandle(/*char **h*/);
char **mydupsafehandle(/*char **h*/);
long myvalidhandle(/*char **h*/);
long myhandlelength(/*char **h*/);
/* commonio.c */
void myprint(/*char *msg*/);
void mymultiprint(/*char *msg, FILE * fp, int addNL*/);
void myeol(/*void*/);
void putOutMsg(/*char *msg*/);
void myerrorout(/*char *msg*/);
void putPieces(/*char * piece1, char * piece2, char * piece3, char * piece4*/);
void echoLINE(/*void*/);
void putOUTSTR(/*void*/);
void putErrorOUTSTR(/*void*/);
void putOutErrorMsg(/*char * msg*/);
int myeof(/*FILE *fp*/);
void prexpr(/*Symbolhandle symh*/);
void fprexpr(/*Symbolhandle symh, FILE *fp, long labels, char **missingcode*/);
void matWrite(/*Symbolhandle arg, FILE *fp, double missValue, int header,
				int labels, int notes, char *separator, int charFormat,
				char *name, Symbolhandle symhComment */);
void macWrite(/*Symbolhandle arg, FILE *fp, int header, int notes,
				int oldStyle, char *name, Symbolhandle symhComment*/);
int fillLINE(/*FILE * fn*/);
int getinput(/*void*/);
int pushInputlevel(/*char * name, long start, unsigned char ** inputstring*/);
void popInputlevel(/*void*/);
void cleanInputlevels(/*int all*/);
/* concat.c */
Symbolhandle concat(/*Symbolhandle arglist*/);
/* contrast.c */
Symbolhandle contrast(/*Symbolhandle list*/);
/* cor.c */
Symbolhandle cor(/*Symbolhandle list*/);
/* daxpy.c */
void daxpy_(/*long *pn, double *pa, double *x, long *pincx, double *y, long *pincy*/);
/* ddot.c */
double ddot_(/*long *pn, double *x, long *pincx, double *y, long *pincy*/);
/* delete.c */
Symbolhandle deleter(/*Symbolhandle list*/);
/* describe.c */
Symbolhandle describe(/*Symbolhandle list*/);
/* dim.c */
Symbolhandle dim(/*Symbolhandle list*/);
Symbolhandle diag(/*Symbolhandle list*/);
/* dunnett.c*/
double PDunnett(/*double x, double ngroup, doublegroupSizes [],
				double edf, long twosided, double epsilon*/);
double QDunnett(/*double x, double ngroup,
				double groupSizes [], double edf, long twosided,
				double epsilon, long * error*/);
/* dynload.c */
Symbolhandle callC(/*Symbolhandle list*/);
Symbolhandle dynload(/*Symbolhandle list*/);
Symbolhandle asLong(/*Symbolhandle list*/);
/* eigen.c */
Symbolhandle eigen(/*Symbolhandle list*/);
/* eigutils.c */
void tql2(/*long nm, long n, double *d, double *e, double *z, long *ierr*/);
void tqlrat(/*long n, double *d, double *e2, long *ierr*/);
void tred1(/*long nm, long n, double *a, double *d, double *e, double *e2*/);
void tred2(/*long nm, long n, double *a, double *d, double *e, double *z*/);
void eispsvd(/*long nm, long m, long n, double *a, double *w, long matu, double *u, long matv, double *v, double *rv1, long *ierr*/);
void tridib(/*long n, double * eps1, double * d, double * e,
			double * e2, double * lb,double * ub, long m11,
			long m, double * w, long * ind, long * ierr,
			double * rv4, double * rv5*/);

void tinvit(/*long nm, long n, double * d, double * e,
			double * e2, long m, double * w,long * ind,
			double * z, long * ierr, double * rv1,
			double * rv2, double * rv3,double * rv4,
			double * rv6*/);
/* errorMsg.c */
char *inWhichMacro(/*void*/);
void badType(/*char *who, long type, long argno*/);
void badDims(/*char *who, long ndims, long argno*/);
void badNargs(/*char *who, long nargs*/);
void invalidSymbol(/*char *who, long argno*/);
void noData(/*char *who, long argno*/);
void undefArg(/*char *who, Symbolhandle arg, long argno*/);
void badKeyword(/*char *who, char *keyword*/);
void badLabels(/*unsigned long whatError*/);
void notTorF(/*char *keyword*/);
void notCharOrString(/*char *keyword*/);
void notPositiveReal(/*char * keyword*/);
void notNonNegativeReal(/*char * keyword*/);
void notNumberOrReal(/*char *keyword*/);
void notPositiveInteger(/*char *keyword*/);
void notNonNegativeInteger(/*char * keyword*/);
void notInteger(/*char *keyword*/);
void notPositiveLong(/*char *keyword*/);
void notNonNegativeLong(/*char * keyword*/);
void notLong(/*char *keyword*/);
void notImplemented(/*char *who*/);
void resultTooBig(/*char * who*/);
/* fft.c */
Symbolhandle fft(/*Symbolhandle list*/);
/* funbalan.c */
long funbalanova(/*long maxiter, double epsilon*/);
/* gammabas.c */
void gammabase(/*double *x, double *a, double *p*/);
double ppgamma(/*double p, double a, long *ifault*/);
/* getmodel.c */
long getmodel(/*Symbolhandle list, long nargs, long startKey, long silent, 
				Symbolhandle symhWts, Symbolhandle symhOffset, 
				Symbolhandle symhN*/);
/* glm.c */
Symbolhandle glm(/*Symbolhandle list*/);
void glmcleanup(/*int removeStrmodel*/);
void clearGlobals(/*void*/);
/* glmutils.c */
long glmInit(/*void*/);
void getterm(/*modelPointer jterm, long term[], long nvars*/);
long setterm(/*long term[], long nvars, long nclasses[]*/);
long stringToTerm(/*char * string, long term []*/);
void stepGlmOdometer(/*long term [], long nclasses [], long nvars,
					 long reverse*/);
void effectiveTrm(/*long term[], modelPointer effTerm, long nvars*/);
long inEffectiveTerm(/*modelPointer term,modelPointereffTerm*/);
long inTerm(/*long jvar, modelPointer term*/);
long nvInTerm(/*modelPointer term*/);
long moreInTerm(/*long jvar, modelPointer term, long nvars*/);
long modeltermEqual(/*modelPointer term1,modelPointer term2*/);
void modeltermAssign(/*modelPointer from, modelPointer to*/);
char **setStrmodel(/*char **modelString, long clean*/);
long inEarlierTerm(/*long termi, long term[], modelInfoPointer info*/);
long getAlpha(/*long termi, long colstart, double ***alphaH,
				long ***colptrH, long transposed*/);
void compLincom(/*double * xrow, double * lincom,
				double * seEst, double * sePred*/);
void deLink(/*double * predVec, double * seVec, double * binomn,
				 long incN, long nrows*/);
long margSS(/*void*/);
void seqSS(/*void*/);
void compDF(/*void*/);
long nAliased(/*void*/);
void xtxinvInit(/*void*/);
long buildXmatrix(/*long termi, double *regx, modelInfoPointer info*/);
long buildXrow(/*double * x, double * regx, modelInfoPointer info*/);
long Xsetup(/*void*/);
long checkVariables(/*modelInfoPointer info*/);
long countMissing(/*modelInfoPointer info, double ***misswts*/);
char *linkName(/*unsigned long control*/);
char *distName(/*unsigned long control*/);
long linkControl(/*char * name*/);
long distControl(/*char * name*/);
char * glmName(/*long op*/);
/* gramschm.c */
void gramschmidt(/*long control*/);
/* graphics.c */
  /* prototypes are in plot.h */
/* hconcat.c */
Symbolhandle hconcat(/*Symbolhandle arglist*/);
/*iniMacro.c*/
long iniMacros(/*void*/);
/* initiali.c */
void initialize(/*char * helpFileName,char * dataFileName,
				char * macroFileName,
				char * dataPath, char * macroPath, char * homePath,
				promptType prompt*/);
/* ipf.c */
long ipf(/*long maxiter, double epsilon*/);
/* iswhat.c */
Symbolhandle iswhat(/*Symbolhandle list*/);
Symbolhandle keyvalue(/*Symbolhandle list*/);
Symbolhandle nameOf(/*Symbolhandle list*/);
Symbolhandle renamer(/*Symbolhandle arglist*/);
/* iterfns.c */
void glmvar(/*long type*/);
void glmlink(/*long type*/);
double glmdev(/*long type*/);
void glmresid(/*void*/);
/* iterglm.c */
long iterglm(/*long maxiter, double epsilon, double clamp*/);
/* keywords.c */
long findKeyword(/*Symbolhandle list, char *word, long start*/);
long getKeyValues(/*Symbolhandle list, long startKey, long op,
					keywordListPtr keyInfo*/);
long getAllKeyValues(/*Symbolhandle list, long startKey, long op,
					   keywordListPtr keyList, long nkeys*/);
long getOneKeyValue(/*Symbolhandle symhKey, long op, keywordListPtr keys,
					long nkeys*/);
void unsetKeyValues(/*keywordListPtr keys, long nkeys*/);
long matchKey(/*char *name, char *charvals[], long codes[]*/);
/* kmeans.c */
Symbolhandle kmeans(/*Symbolhandle list*/);
/* labutils.c */
void getAllLabels(/*Symbolhandle arg, char * labels [],
					long widths [], long lengths []*/);
char * getOneLabel(/*Symbolhandle symh, long dimension, long index*/);
void getMatLabels(/*Symbolhandle arg, char * rowcol [2],
				  long lengths [2]*/);
char **createLabels(/*long ndims, long lengths []*/);
char ** growLabels(/*char ** oldlabels, long length*/);
void buildLabels(/*char ** labelsH, char * labels [], long dims [],
				   long ndims*/);
long expandLabels(/*char * root, long size, char * labels*/);
long moveMatLabels(/*Symbolhandle from, Symbolhandle to,
				   unsigned long control*/);
long fixupMatLabels(/*Symbolhandle symh, unsigned long control*/);
long transferLabels(/*Symbolhandle from, Symbolhandle to*/);
void appendLabels(/*Symbolhandle symh, char * newLabels, long jdim,
					long expand*/);
void appendNLabels(/*Symbolhandle symh, char * newLabels, long jdim,
				   long startIndex, long nlabels*/);
long installLabels(/*Symbolhandle symhLabels, Symbolhandle result*/);
unsigned long checkLabels(/*Symbolhandle symhLabels, long ndims, 
						  long dims []*/);
long anyLabels(/*Symbolhandle symh*/);
long sizeOfLabels(/*Symbolhandle symh*/);
long sizeOfNotes(/*Symbolhandle symh*/);
long prepPrintLabels(/*Symbolhandle symh,
					   char * dimLabels [], long dimLabelsWidth []*/);
void putColumnLabels(/*FILE * fp, long lastdim, char * labels,
					 long labelWidth, long fieldWidth,
					 long nperLine, long type*/);
void putRowLabels(/*FILE * fp, long ndims, long coord [],
				  long labelWidth, char * labels [],
				  long * labelsWidth, long charData*/);
Symbolhandle getlabs(/*Symbolhandle list*/);
Symbolhandle setlabs(/*Symbolhandle list*/);
/* list.c */
Symbolhandle listbrief(/*Symbolhandle arglist*/);
/* lpsforwd.c */
long leapsforwd(/*long *longparms, double *RR, double *XI, double *XN,
	double *XM, double *DD, double *D, double *DT, double *CO, double *SC,
	double *SQ, double *CL, double *RM, long *INN, long *IPP, long *IND,
	long *INT, double *S2, double *TL, double *FD, long *leapsNE, long *IV*/);
/* lpslandb.c */
long leapslandb(/*double *XI, double *XN, double *D, double *YI, double *CI,
	double *CN, double *CO, double *CL, double *RM, long *IND, long *NS,
	long *ND, double *DF, long *IB, long *MB, long *IZ, long *MN, long *KY,
	double *SIG, double *PEN, double *ZIP, long *KX, long *KZ, long *NI,
	long *IV*/);
/* lpsmisc.c */
void leapspivot(/*long *N, long *ML, long *LS, long *LX, double *XN, long *INT, long *IND, double *CN, double *CO, double *DT, long *IQ, long *ND, long *NN, long *KY, long *LL, long *LY*/);
void leapstest(/*double *WT, long *N, double *XI, long *INT, double *D, double *DD, double *SQ, double *DT, long *ML, long *MN, long *ND*/);
void leapsqstore(/*double *RS, double *CN, double *CL, double *RM, long *MB, double *DF, long *MN, double *SIG, double *PEN, long *IB, long *NS*/);
void leapscopy(/*double *XN, double *XI, long *INT, long *IND, long *ML, long *MT, long *MP, long *ND, double *SC, double *CN, double *CI, long *IQ*/);
void leapstrans(/*double *TR, long *MR, double *RS, double *DF, long *MN, double *SIG, double *PEN, long *IB, long *MB*/);
/* macro.c */
long prepMacro(/*char * text, char * newText, int addDollars*/);
Symbolhandle macro(/*Symbolhandle list*/);
/* mainpars.c */
void yyerror(/*char *s*/);
void cleanitup(/*void*/);
int yyparse(/*void*/);
/* makefact.c */
Symbolhandle makefactor(/*Symbolhandle list*/);
/* makestr.c */
Symbolhandle makestr(/*Symbolhandle list*/);
Symbolhandle compnames(/*Symbolhandle list*/);
/* mathutil.c */
long doSwp(/*double * cp,long m,long n,long k,double cpdiag, long full*/);
double fsolve(/*double arg, double (* func) (double ,double [], long, long *),
			  double param [], long nparam, double xmax,
			  double xmin, double eps, long itmax, long ntab,
			  double x [], double y [], double pr [],
			  long report []*/);
double mystrtod(/*char *nptr, char **eptr*/);
double pythag(/*double a, double b*/);
double intpow(/*double x*/, /*double p*/);
#ifdef NOLGAMMA
double lgamma(/*double x*/);
#endif /*NOLGAMMA*/
double          epslon(/*double x*/);
/* matrix.c */
Symbolhandle matrix(/*Symbolhandle list*/);
/* mreader.c */
int modelparse(/*void*/);
int initmodelparse(/*char **model*/);
modelInfoPointer getModelInfo(/*void*/);
/* myplot.c */
Symbolhandle myplot(/*Symbolhandle list*/);
/* normbase.c */
void normbase(/*double *x, double *phi*/);
double ppnd(/*double p, long *ifault*/);
/* outer.c */
Symbolhandle outer(/*Symbolhandle list*/);
/* partacf.c */
Symbolhandle partacf(/*Symbolhandle list*/);
/* paste.c */
Symbolhandle multipaste(/*Symbolhandle arg, char ,*format,
	char **stringForMissing, char fieldSep, char lineSep*/);
Symbolhandle paste(/*Symbolhandle list*/);
/* plotutil.c*/
void set_point(/*struct curve_points ** plot, long ipoint,
			   double x, double y, char * string,
			   unsigned char pointColor, unsigned char linecolor*/);
void undef_point(/*struct curve_points **plot,long ipoint*/);
long do_whole_graph(/*whole_graph ** graph, FILE * fp,
					long termType, long windno,
					plotKeyValuesPtr keyValues*/);
void deleteGraph(/* struct whole_graph **graph */);
struct whole_graph ** copyGraph(/*struct whole_graph ** graph*/);
void getGraphExtremes(/*whole_graph ** graph, double * extremes,
						long pointsOnly */);
long getPlotKeys(/*Symbolhandle list,long startKey,
				 plotKeyValuesPtr keyValues*/);
void unsetPlotKeys(/*plotKeyValuesPtr keyValues, long all*/);
long checkPlotKeys(/*long toFile, plotKeyValuesPtr keyValues,
				   long * termType*/);
int saveGraph(/*Symbolhandle symh, char * name*/);
int restoreGraph(/*Symbolhandle symh*/);
FILE *openPlotFile(/*char *fileName,long new*/);
long sizeofPlot(/*whole_graph ** graph*/);
int screenDump(/*char *fname*/);
/* polyroot.c */
Symbolhandle polyroot(/*Symbolhandle list*/);
/* power.c */
Symbolhandle samplesize(/*Symbolhandle list*/);
Symbolhandle power1(/*Symbolhandle list*/);
/* predtabl.c */
Symbolhandle predtable(/*Symbolhandle list*/);
/* printano.c */
void printglm(/*long fstats, long pvals, long sssp, long byvar*/);
/* printreg.c */
void printregress(/*long pvals*/);
/* pvals.c */
Symbolhandle cumcdf(/*Symbolhandle list*/);
/* pvalsub.c */
double Cbet(/*double x, double a, double b*/);
double Qbeta(/*double p, double a, double b*/);
double Cbin(/*long x, long n, double P*/);
double Cchi(/*double x, double f*/);
double Qchi(/*double p, double f*/);
double Cgam(/*double x, double a, double b*/);
double Qgam(/*double p, double a, double b*/);
double noncenCchi(/*double x, double f, double lam*/);
double noncenQchi(/*double p, double f, double lam, double eps*/);
double Chyp(/*long x, long M, long N, long K*/);
double Cnor(/*double x*/);
double Qnor(/*double p*/);
double Cpoi(/*long x, double l*/);
double Csne(/*double x, double n1, double n2*/);
double Qsne(/*double p, double n1, double n2*/);
double noncentF(/*double x, double lam, double n1, double n2*/);
double noncentBeta(/*double x, double lam, double a, double b*/);
double Cstu(/*double x, double n*/);
double Qstu(/*double p, double n*/);
double noncenCstu(/*double x, double n, double noncen*/);
/* qr.c */
Symbolhandle qr(/*Symbolhandle list*/);
/* random.c */
void randomSeed(/*long verbose*/);
Symbolhandle rangen(/*Symbolhandle list*/);
Symbolhandle setseed(/*Symbolhandle list*/);
Symbolhandle getseed(/*Symbolhandle list*/);
long iuni(/*void*/);
double duni(/*void*/);
void vuni(/*long count, double *rvec*/);
void vnorm(/*long count, double *rvec*/);
/* rankquic.c */
void rankquick(/*double *v, double *rank, double *scratch, long n, long op*/);
void rankquickchar(/*char * v[], double rank [], double scratch [],
			   long n,long op*/);
/* rational.c */
Symbolhandle rational(/*Symbolhandle list*/);
/* regpred.c */
Symbolhandle regpred(/*Symbolhandle list*/);
/* rep.c */
Symbolhandle rep(/*Symbolhandle arglist*/);
/* rotation.d */
Symbolhandle rotatefac(/*Symbolhandle list*/);
/* run.c */
Symbolhandle run(/*Symbolhandle arglist*/);
/* screen.c */
Symbolhandle screen(/*Symbolhandle list, long verbose*/);
long leapsqprint(/*long *ind, long *mn, double *rm, double *tss, double *s2, double *scale, double *df, long *it, long *ib, double *pen*/);
long leapsscreen(/*long *NV, long *IT, long *KX, long *NF, long *NO, long *IB,
	double *FD, long *MB, double *RT, long *ND, long *NC, long *IW, long *NW,
	double *RW, long *NR, double *TL, double *S2, long *leapsNE, long *IV*/);
/* select.c */
Symbolhandle selecter(/*Symbolhandle list*/);
/* setOptio.c */
void setDefaultOptions(/*void*/);
Symbolhandle setoptions(/*Symbolhandle list*/);
Symbolhandle getOpt(/*short *whichOptions*/);
void setOpt(/*Symbolhandle list, long quiet*/);
long isOption(/*char *keyword*/);
Symbolhandle elapsedTime(/*Symbolhandle list*/);
/* shell.c */
Symbolhandle shell(/*Symbolhandle list*/);
int processBang(/*void*/);
/* solve.c */
Symbolhandle solve(/*Symbolhandle list*/);
/* sort.c */
Symbolhandle sort(/*Symbolhandle list*/);
/* sortquic.c */
void sortquick(/*double v[], long n*/);
void sortquickchar(/*char * v [], long n*/);
/* split.c */
Symbolhandle split(/*Symbolhandle list*/);
/* stemleaf.c */
Symbolhandle stemleaf(/*Symbolhandle list*/);
/* stemsubs.c */
long doStemLeaf(/*double ** data, long n, long width,
				  long maxlines, long xtrems, long verbose,
				  long depth, char ** title*/);
/* studentb.c */
void studentbase(/*double *x, double *df, double *cdf*/);
double ppstudent(/*double pp, double n, long *ifault*/);
 studrang.c 
double Pstudrange(/*double x, double ngrp, double df, double eps*/);
double Qstudrange(/*double p, double ngrp, double df, double eps*/);
/* svd.c */
Symbolhandle svd(/*Symbolhandle list*/);
/* swp.c */
Symbolhandle swp(/*Symbolhandle arglist*/);
/* tabs.c */
Symbolhandle tabs(/*Symbolhandle list*/);
/* toeplitz.c */
Symbolhandle toeplitz(/*Symbolhandle list*/);
/* trans.c */
Symbolhandle Transform(/*Symbolhandle list*/);
/* transpos.c */
Symbolhandle    doTranspose(/*Symbolhandle arg, double * params,
								   unsigned long control,
								   unsigned long * status*/);
Symbolhandle transpose(/*Symbolhandle list*/);
 trideige.c */
Symbolhandle trideigen(/*Symbolhandle list*/);
/* ttests.c */
Symbolhandle tval(/*Symbolhandle list*/);
Symbolhandle t2val(/*Symbolhandle list*/);
Symbolhandle tint(/*Symbolhandle list*/);
Symbolhandle t2int(/*Symbolhandle list*/);
/* unbalano.c */
long unbalanova(/*void*/);
/* utils.c */
long argOK(/*Symbolhandle arg, long type, long argno*/);
long isTorF(/*Symbolhandle arg*/);
long isNumber(/*Symbolhandle arg, int kind*/);
long isInteger(/*Symbolhandle arg, int kind*/);
long isCharOrString(/*Symbolhandle arg*/);
long isNull(/*Symbolhandle arg*/);
long isDefined(/*Symbolhandle arg*/);
long isAssigned(/*Symbolhandle arg*/);
long isScalar(/*Symbolhandle arg*/);
long isVector(/*Symbolhandle arg*/);
long isMatrix(/*Symbolhandle arg, long * dim*/);
long isFactor(/*Symbolhandle symh*/);
char *isKeyword(/*Symbolhandle arg*/);
int checkArgType(/*Symbolhandle arg, char * what, long targetType*/);
int checkBalance(/*unsigned char * line, char * name*/);
long checkSymmetry(/*double *a, long nrows*/);
long symbolSize(/*Symbolhandle arg*/);
long isTooBig(/*long n1, long n2, size_t size*/);
void getDims(/*long *dims, Symbolhandle arg*/);
double decodeString(/*char * string, char * seps, long *error */);
char findBracket(/*long goalLevel, char leftbracket, 
				 unsigned char * inputString,
				 long * thisLevel, long * thisPlace, 
				 long * lastchar*/);
long copyField(/*char * line, char * outstr*/);
long copyQuotedField(/*char * line, char * outstr, long commaOK,
					 long seekQuote, long * error*/);
int nDigits(/*long N*/);
long setFormat(/*char *format, long fmt[]*/);
void installFormat(/*long beforeDec, long afterDec, long fmtType*/);
void installScreenDims(/*long width, long height*/);
void saveFormat(/*void*/);
void restoreFormat(/*void*/);
int anyMissing(/*Symbolhandle a*/);
int anyNaN(/*Symbolhandle a*/);
int anyInfinite(/*Symbolhandle a*/);
int anyDoubleMissing(/*double * a, long n*/);
int anyDoubleNaN(/*double * a, long n*/);
int anyDoubleInfinite(/*double * a, long n*/);
void stepOdometer(/*long ind [], long bounds [], long ndims, long base,
				  long reverse*/);
void setScratchName(/*Symbolhandle symh*/);
void setScratchMacroName(/*Symbolhandle symh, char * macroName*/);
void setCompName(/*Symbolhandle symh, char *name*/);
char *typeName(/*long type*/);
char *opName(/*long op*/);
char *tokenName(/*long token*/);
char *fieldStart(/*char * stringBuf*/);
void trimBuffer(/*char * buffer, unsigned long control*/);
long indentBuffer(/*char * buffer, int nplaces*/);
long centerBuffer(/*char * buffer, char * value, long length*/);
long formatChar(/*char * buffer, char * value, unsigned long control*/);
long formatDouble(/*char * buffer, double value, unsigned long control*/);
long escapedOctal(/*unsigned char c, unsigned char * buffer*/);
char * n_th(/*long n*/);
char *skipStrings(/*char *ch, long l*/);
char *copyStrings(/*char *in, char *out, long l*/);
short scanPat(/*char *string,long *matchType, char *pattern, int checkName,
			   long maxLength*/);
short matchName(/*char *name, long matchType, char *pattern*/);
long mystrncmp(/*char *s1, char *s2, int n*/);
char *getTimeAndDate(/*void*/);
double mygettime(/*void*/);
void getElapsedTime(/*double times[2]*/);
void symExtremes(/*Symbolhandle symh, double vec[2] */);
void symFill(/*Symbolhandle symh, double value*/);
void doubleFill(/*double * x, double value, long length*/);
void doubleCopy(/*double * from, double * to, long length*/);
double doubleMin(/*double * x, long n*/);
double doubleMax(/*double * x, long n*/);
long longMin(/*long * x, long n*/);
long longMax(/*long * x, long n*/);
void doubleToLong(/*double * in, long * out, long length*/);
void longToDouble(/*long * in, double * out, long length*/);
/* utilstru.c */
Symbolhandle doRecur1(/*Symbolhandle (*doit) (Symbolhandle, double *, unsigned long, unsigned long *), Symbolhandle arg1, double *params,unsigned long control,unsigned long *status*/);
Symbolhandle doRecur2(/*Symbolhandle (*doit) (Symbolhandle, Symbolhandle, double *,
	unsigned long, unsigned long *), Symbolhandle arg1,Symbolhandle arg2,
	double *params,unsigned long control,unsigned long *status*/);
long strucIsMatrix(/*Symbolhandle arg*/);
long strucIsVector(/*Symbolhandle arg*/);
long getStrucTypes(/*Symbolhandle symh*/);
long getSingleType(/*Symbolhandle symh*/);
long checkType(/*Symbolhandle symh, long type*/);
long strucAnyNull(/*Symbolhandle arg*/);
long strucAnyType(/*Symbolhandle arg, long type*/);
long strucSymbolSize(/*Symbolhandle arg*/);
long strucSize(/*Symbolhandle arg*/);
long strucAnyMissing(/*Symbolhandle arg, unsigned long control*/);
long dimcmp(/*Symbolhandle arg1, Symbolhandle arg2, long control*/);
long treecmp(/*Symbolhandle str1, Symbolhandle str2, long matchDims*/);
/* varnames.c */
Symbolhandle varnames(/*Symbolhandle list*/);
Symbolhandle modelvars(/*Symbolhandle list*/);
Symbolhandle xvariables(/*Symbolhandle list*/);
Symbolhandle xvarsrows(/*Symbolhandle list*/);
/* yates.c */
Symbolhandle yates(/*Symbolhandle list*/);
/* yylex.c */
int yylex(/*void*/);
long findLParen(/*int op, char paren*/);
#endif /*NOPROTOTYPES*/

#endif /*MATPROTOH__*/
