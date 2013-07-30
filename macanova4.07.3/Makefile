# Makefile for MacAnova 4.07, version of 990305

# Copyright 1999 Christopher Bingham and Gary Oehlert
# See file COPYING for details
# This may be freely distributed as long as this notice is retained
# and file COPYING is included.

# Note: Nothing depends directly on platform.h  If that is changed, you
# must make sure things affected are re-compiled

# 940823  revised to use platform.h
# 941006  revised to cope with Epx problems
# 941205  Modified for use on HP
# 950120  Modified to compile comclipb.c
# 950310  Modified to compile select.c
# 950325  Added target tarfiles and lists of source and auxiliary files
# 950417  Added more explanatory comments and modified some macros
# 950522  Modified to compile and link trideigen()
# 950615  Modified to include macanova.nws as auxiliary file
# 950620  Deleted macro POSTSCRIPT, added defines for glm options
# 950703  Modified to reflect name change of commonou.c to commonio.c
# 950810  Add macros MONITOR, LIBS, PARSER
# 950918  Removed macros MONITOR, LIBS, PARSER
# 950920  Added macros to set startup file for readline
# 950927  Deleted references to readLine.[co], since readLine.c has
#         been incorporated into readFile.c
#         Renamed readlinr.c (djgpp version) to readline.c
# 951103  Added header keywords.h
# 951222  Added file keywords.c (stuff formerly in utils.c)
# 960116  Added file studrang.c
# 960223  Added file version.h supplying macros for initiali.c
#         and startMsg.c.  version.h is an alternate location to
#         define any of the following used in startupMsg() and/or
#         initialize()
#           VERSION, MAKE_DATE, PROCESSOR_COMPILER, DATAPATHNAME,
#           MACROPATHNAME, HELPPATHNAME, DATAFILENAME, MACROFILENAME,
#           HELPNAME
#         All may be defined in this Makefile
#         It is essential that the preprocessor macro defined by
#         make macro MACHINE be globally defined, either by
#         definition in Makefile or by a project setting when
#         compiling under an IDE such as Borland C or Code Warrior
#  960308 Added file labutils.c
#  960314 Changed to version 4.00
#  960327 mainpars.c now piped through fixmainp to change
#         printf(...)
#         to
#         (sprintf(OUTSTR,...),myprint(OUTSTR))
#  960424 Various rearrangements, new macros, mainly to make easier
#         having different versions.
#         Changed names of unxio.c and unxhandl.c to unxIo.c and
#         unxHandl.c;  set macros to subsituted dosIo.c and
#         dosHandle.c for djgpp.  As of now the dos*.c are identical
#         to unx*.c
#  960503 Changed name of macOpen.c to macFindFile.c
#  960614 Added readline 2.0 headerfiles to tar files list
#  960705 Deleted references to boxprep.c & boxprep.o (in boxplot.c)
#  960708 Added TestChapterY to testfiles to be tar'ed
#  960713 WXWin stuff merged
#  960715 Added target binarytest to check binaries are appropriate
#  960718 Macros BINARIESNEEDED and BINARYTEST added and
#         reorganized source file lists
#  960725 Added macro IFACEH and modified dependencies.  This is
#         is wxIface.h for wx version, macIface.h in mac version,etc.
#  960822 Included wxwin files that were changed in files to be
#         tarred.
#  960924 Further merging of WXWin stuff
#  961023 targets tarsrcfiles and tarauxfiles now gzip tar files
#  961120 New macros OPT and WARN, plus change in optimization level
#         to -O for Motif to overcome apparent compiler bug; WARN
#         commented out in make.env
#  961127 Made WX binaries dependent on  $(GLOBALHEADERS)
#  961210 Made linpacXX.o dependent on handlers.h
#  961216 Incorporated variants for Linux
#  961219 Added support for kmeans
#  970407 Modified Makefile.dj2 for use with DJGPP2
#  970410 DJ version directly compiles to macanodj.exe
#  970415 Modified arrangement of src files for DJGPP
#  970417 DJ version uses -lemu for floating emulation
#  970611 Added dynload.{c,o}
#  970619 Bumped version to 4.05
#  970625 Made utils.o depend on keywords.h
#  970725 Added user function files to source file list
#  971001 Added file wx/wxdialog.cc
#  971011 Added dunnett.c
#  971112 Bumped version to 4.06
#  971126 Added labels.h to source and to $(GLOBALHEADERS)
#  971210 Added comIface.c to source and FNSKB
#  980401 Revised to use make.env containing platform-specific stuff
#  980407 kmeans.o now depends on keywords.h
#  980513 moved tar file related stuff to tarfiles.mk and made
#         some cosmetic changes to this file
#  980715 save.o now depends on keywords.h
#  980716 Bumped version to 4.07
#  980717 Removed mainpars.h from macro GLOBALHEADERS and made Ifsetup.o,
#         Lang.o, Symbol.o, main.o, shell.o solve.o, utils.o, utilstru.o,
#         yylex.o, wxmain.o depend on mainpars.h
#  980718 Added new header mvsave.h to list of source and made save.o,
#         restore.o, plotutil.o, utils.o main.o, wxmain.o dependent on it.o
#  980722 Added new header mvhelp.h to list of source and made help.o
#         dependent on it
#  
#  980723 mainpars.h now depends on nothing
#  990305 new header iniMacro.h and macro file iniMacro.mac
#         iniMacro.o depends on iniMacro.h.  New shell script iniMacro.sh
#         creates iniMacro.h from iniMacro.mac.
#  990324 Bumped release number to 3
SHELL = /bin/sh

# Note: The date is now set in version.h
VERSIONNUMBER = 4.07
RELEASENUMBER = 3

INIMACROSH=iniMacro.sh # script to transform iniMacro.mac to iniMacro.h

# make.env must be a copy of one of the files in directory Makefile.dir
# It determines the target, compiler, compiler flags and load flags
# as well as make macros such as WX, IFACEH

include make.env

WXWINOBJECTS=$(WX)/wxmain.o $(WX)/wxtframe.o $(WX)/wxtwind.o $(WX)/wxgraph.o\
	$(WX)/wxhandl.o $(WX)/wxIo.o $(WX)/wxPrint.o $(WX)/wxdialog.o
WXWININCLUDES=$(WX)/wxmain.h $(IFACEH) # $(WX)/wxProto.h

MEMORYCOUNT = -DCOUNTMISSES #keep track of cache hits and misses
UNIX = $(READLINEFLAGS) -DDEFINETYPES $(MEMORYCOUNT)\
	-DUNIX $(WXWINMOTIF) # -DSETJMP

#   MAINPARSSED is a sed script used to create mainpars.c from y.tab.c
#   MAINPARSAWK is an awk script used to create macMainpars.c and
#      macMainpars.r from mainpars.c
#   MREADERSED is a sed script used to create mreader.c from y.tab.c
#   Different versions may be needed on different machines, depending
#   on the yacc and/or sed used.  In particular, to use bison instead of yacc
#   would require a major re-thinking.

MAINPARSSED = mainpars.sed.hp
MAINPARSAWK = mainpars.awk.hp
MREADERSED = mreader.sed.hp

# constants for default values of certain GLM options, used by setOptions()
GLMOPTIONS = -DDEFAULTPRINTFSTATS=0 -DDEFAULTPRINTPVALS=0

# Previous 4.0x versions, if any
OLDVERSIONS = #-D'BACKUPS="~kb/public/macanova4.0.past.1, ~kb/public/macanova4.0.past.2 ..."'

# Note that globals.h and *Proto.h are not in GLOBALHEADERS on which
# virtually everything depends.  Thus changing these does not trigger
# recompilation of anything.

# Currently, since PROTOHEADERS has a null definition, nothing depends on
# the matProto.h, unxProto.h, or ioProto.h

# All linear model related code depends on glm.h

# All plotting related code depends on plot.h

GLOBALHEADERS = Symbol.h globdefs.h globkb.h handlers.h\
	 typedefs.h labels.h

GLMHEADERS = glm.h

PLOTHEADERS = $(GLOBALHEADERS) plot.h

# unxProto.h and dosProto.h should be identical
UNXPROTO =#unxProto.h # prototypes for functions special to Unix
DOSPROTO =#dosProto.h # prototypes for functions special to DOS version

PROTOHEADERS =#matProto.h ioProto.h $(UNXPROTO) $(DOSPROTO)

# If any platform specific non-prototype headers are added, dependencies
# may need to be changed.
# As of 4/24/96, only Mac version has any special headers
# As of July 1996, wx version has special headers

UNXHEADERS =# special headers for Unix if any
DOSHEADERS =# special headers for DOS versions if any
WXHEADERS =$(WXINCLUDES)

# lists of object files; grouping has no current significance

UTILS = initiali.o iniMacro.o Symbol.o  Macroset.o yylex.o Lang.o\
	Ifsetup.o Subassig.o commonio.o comhandl.o comclipb.o utils.o keywords.o\
	labutils.o utilstru.o glmutils.o mathutil.o

PARSERS = mainpars.o mreader.o

# If there ever was logic for the division of function binaries among
# macros here, it's been forgotten

FNSUNX = unxIo.o unxHandl.o # specific for Unx
FNSDGJPP=#doprnt.o readline.o#anything special for djgpp DOS version
FNSDOS =#dosIo.o dosHandl.o #anything special for DOS versions

# The following is here just as a reminder there are special WX versions
# xxIo and xxHandl; the operative macro is $(WXWINOBJECTS)
FNSWXWIN =# $(WX)/wxIo.o $(WX)/wxhandl.o # specific to wxwin

FNS = concat.o macro.o makestr.o ipf.o  cellstat.o getmodel.o\
	makeFact.o readdata.o matrix.o gramschm.o unbalano.o printano.o term.o
FNS2 = delete.o list.o array.o print.o contrast.o anovacoe.o spool.o trans.o
FNS3 = cellstts.o glm.o printreg.o describe.o  rankquic.o sortquic.o
FNS4 = graphics.o myplot.o plotutil.o help.o sort.o split.o
FNS5 = stemleaf.o stemsubs.o ttests.o batch.o restore.o save.o boxplot.o
FNS6 = pvals.o pvalsub.o regpred.o power.o yates.o outer.o predtabl.o rep.o\
	run.o studrang.o dunnett.o
FNS7 = funbalan.o screen.o lpsforwd.o lpslandb.o lpsmisc.o cor.o
FNS8 = random.o iterglm.o iterfns.o tabs.o transpos.o eigen.o eigutils.o
FNS9 = betabase.o gammabas.o studentb.o normbase.o
FNSKB = columnop.o hconcat.o dim.o errorMsg.o swp.o bin.o readHead.o\
	readFile.o startMsg.o setOptio.o svd.o trideige.o cluster.o changest.o\
	movavg.o partacf.o toeplitz.o polyroot.o fft.o fftsubs.o tserops.o\
	tsersubs.o paste.o shell.o iswhat.o qr.o solve.o cholesky.o rational.o\
	varnames.o linpacGE.o linpacQR.o linpacCH.o blas.o rotation.o\
	select.o kmeans.o dynload.o comIface.o

ALLSUB = $(UTILS) $(FNSWXWIN) $(FNSDOS) $(FNSDJGPP)\
	$(FNS) $(FNS2) $(FNS3) $(FNS4)	$(FNS5) $(FNS6) $(FNS7) $(FNS8)\
	$(FNS9) $(FNSKB)

FNMAIN =main.o
ALLOBJ = handlers.o	$(PARSERS) $(ALLSUB)

macanovawx: $(BINARYTEST) $(WXWINOBJECTS) $(ALLOBJ) 
	$(CC) $(OPT) $(WARN) $(PROFILE) -o newmacan \
		$(WXWINOBJECTS) $(ALLOBJ) $(LDFLAGS) $(LDLIBS)
	mv newmacan macanovawx

macanova: $(BINARYTEST) $(FNMAIN) $(ALLOBJ) $(FNSUNX)
	$(CCC) $(OPT) $(WARN) $(PROFILE) $(MACHINE) -o newmacan\
		$(FNMAIN) $(FNSUNX) $(ALLOBJ) $(LIBS)
	mv newmacan macanova

binarytest:
	sh dobinarytest '$(NAMEFORBINARIES)'


# Note that mainpars.h does not depend on mainpars.y.  It needs to be
# specially remade when any changes to mainpars.y change y.tab.h

# Note that the output from yacc is filtered by sed and fixmainp.
# New versions of the sed script $(MAINPARSSED) and program fixmainp
# may be needed for different versions of yacc.  If you're porting MacAnova
# to a new platform, you should probably use the distributed version
# of mainpars.c, making sure your Make doesn't automatically invoke
# yacc when it sees that mainpars.c depends on mainpars.y.  By defining
# MAINPARSY to be empty (done in distributed *.env files), you may be able
# to avoid this.

# Leave the following uncommented out if making mainpars.c (at your own risk)
mainpars.h:
	$(YACC) -d mainpars.y
	mv y.tab.h mainpars.h
	$(SED) -f $(MAINPARSSED) y.tab.c $(DOFIXMAINPARS) > mainpars.c
	rm y.tab.c

# Previously dependence was
# mainpars.h: Symbol.h globdefs.h globkb.h handlers.h typedefs.h

# Leave the following uncommented out if making mainpars.c (at your own risk)
mainpars.c: $(MAINPARSSED) $(GLOBALHEADERS) $(MAINPARSY) $(FIXMAINPARS) $(PROTOHEADERS)
	$(YACC) -d mainpars.y
	$(SED) -f $(MAINPARSSED) y.tab.c $(DOFIXMAINPARS) > mainpars.c
	rm y.tab.c
	cmp mainpars.h y.tab.h
	rm y.tab.h

# $(FIXMAINPARS) is used to filter yacc output to produce mainpars.c.
# It will almost certainly have to be rewritten for use on systems
# with different versions of yacc.  If you are porting MacAnova to a new
# platform, it is recommended you use the distributed version of mainpars.c
# and set FIXMAINPARS and DOFIXMAINPARS so that nothing happens.

# Leave the following uncommented out if making mainpars.c (at your own risk)
#(FIXMAINPARS): $(FIXMAINPARSC)
#	$(CCC) $(CFLAGS) -o $(FIXMAINPARS) $(FIXMAINPARSC)

# macMainpars.c and macMainpars.r are for Macintosh and created from
# mainpars.c by awk script $(MAINPARSAWK).  It is strongly suggested that
# the distributed versions of macMainpars.c and macMainpars.r be used.

macMainpars.c: mainpars.c
	$(AWK) -f $(MAINPARSAWK) mainpars.c

macMainpars.r: mainpars.c
	$(AWK) -f $(MAINPARSAWK) mainpars.c

# macIniMacro.r is for Macintosh
macIniMacro.r: iniMacro.h iniMacro.awk
	$(AWK) -f iniMacro.awk iniMacro.h

# iniMacro.h contains the text of built in macros in a form used to
# initialize a C array of structures..  $(INIMACROSH) is a sh script
# that creates iniMacro.h from iniMacro.mac, a file of macros readable
# by macroread(), also containing some C comments and preprocessor commands.

iniMacro.h: iniMacro.mac
	cp iniMacro.h iniMacro.h.bak
	$(INIMACROSH) iniMacro.mac iniMacro.h

# Note that the output from yacc is filtered by sed.  New versions of
# the sed script $(MREADERSED) may be needed for different versions of
# yacc.  If you are porting MacAnova to another platform, it is recommended
# you use the distributed version of mreader.c rather than recreated it
# from mreader.y

# Leave the following uncommented out if making mreader.c (at your own risk)
mreader.c:	$(MREADERSED) $(MREADERY) $(GLMHEADERS) $(PROTOHEADERS)
	$(YACC) mreader.y
	$(SED) -f $(MREADERSED) y.tab.c > mreader.c
	rm y.tab.c

initiali.o: initiali.c version.h $(MAKEFILE) $(GLOBALHEADERS) $(IFACEH)\
	$(PROTOHEADERS)
	$(CCC) $(OPT) $(WARN) $(MACHINE) $(CFLAGS) $(PROFILE) -c $(UNIX)\
		$(DATAPATH) $(PROCESSOR)\
		$(HELPPATH) $(READLINEFLAGS) initiali.c

main.o: main.c $(GLOBALHEADERS) $(GLMHEADERS) $(PROTOHEADERS)\
	mainpars.h $(MAKEFILE)

Ifsetup.o Lang.o shell.o solve.o utilstru.o yylex.o wxmain.o: mainpars.h

$(ALLSUB): $(GLOBALHEADERS) $(PROTOHEADERS)

startMsg.o: startMsg.c $(MAKEFILE) platform.h version.h
	$(CCC) $(OPT) $(WARN) $(MACHINE) $(CFLAGS) $(PROFILE) -c startMsg.c\
		$(UNIX)	$(PROCESSOR)\
		$(OLDVERSIONS)

iniMacro.o: iniMacro.c iniMacro.h
	$(CCC) $(OPT) $(WARN) $(MACHINE) $(CFLAGS) $(PROFILE) -c $(UNIX) iniMacro.c

Symbol.o: Symbol.c $(PLOTHEADERS) $(PROTOHEADERS) mainpars.h
	$(CCC) $(OPT) $(WARN) $(MACHINE) $(CFLAGS) $(PROFILE) -c $(UNIX) Symbol.c

utils.o: utils.c keywords.h $(PLOTHEADERS) $(PROTOHEADERS) \
	mainpars.h blas.h mvsave.h
	$(CCC) $(OPT) $(WARN) $(MACHINE) $(CFLAGS) $(PROFILE) -c $(UNIX) utils.c

kmeans.o: kmeans.c keywords.h $(PLOTHEADERS) $(PROTOHEADERS)
	$(CCC) $(OPT) $(WARN) $(MACHINE) $(CFLAGS) $(PROFILE) -c $(UNIX) kmeans.c

keywords.o: keywords.c keywords.h $(GLOBALHEADERS) $(PROTOHEADERS)
	$(CCC) $(OPT) $(WARN) $(MACHINE) $(CFLAGS) $(PROFILE) -c $(UNIX) keywords.c

dynload.o: dynload.c dynload.h $(GLOBALHEADERS) $(PROTOHEADERS)

setOptions.o: setOptions.c $(GLOBALHEADERS) $(MAKEFILE)
	$(CCC) $(OPT) $(WARN) $(MACHINE) $(CFLAGS) $(PROFILE) -c $(UNIX)\
		$(GLMOPTIONS) setOptions.c

linpacCH.o linpacCE.o linpacQR.o: linpack.h blas.h typedefs.h handlers.h

blas.o: blas.h typedefs.h

eigen.o: blas.h $(GLOBALHEADERS) $(PROTOHEADERS) eigen.c

readdata.o: matdat.h keywords.h $(GLOBALHEADERS) $(IFACEH) $(PROTOHEADERS)

readHead.o readFile.o: matdat.h $(GLOBALHEADERS) $(IFACEH) $(PROTOHEADERS)

glm.o varnames.o save.o: keywords.h $(GLMHEADERS) $(GLOBALHEADERS)

anovacoe.o contrast.o funbalan.o ipf.o gramschm.o predtabl.o\
	glmutils.o getmodel.o mreader.o iterfns.o iterglm.o regpred.o\
	screen.o unbalano.o cellstts.o cellstat.o printano.o printreg.o\
	restore.o: $(GLMHEADERS) $(GLOBALHEADERS)

tserops.o: tser.h keywords.h $(GLOBALHEADERS) $(PROTOHEADERS)

fft.o fftsubs.o tsersubs.o: tser.h $(GLOBALHEADERS) $(PROTOHEADERS)

plotutil.o: keywords.h mvsave.h $(PLOTHEADERS) $(PROTOHEADERS) $(IFACEH)

myplot.o boxplot.o term.o: $(PLOTHEADERS) $(IFACEH) $(PROTOHEADERS)

graphics.o: $(PLOTHEADERS) $(PROTOHEADERS)

help.o: $(GLOBALHEADERS) $(PROTOHEADERS) mvhelp.h

batch.o comclipb.o comhandl.o commonio.o handlers.o help.o iniMacro.o\
	mainpars.o print.o setOptio.o spool.o startMsg.o :\
		 $(GLOBALHEADERS) $(IFACEH) $(PROTOHEADERS)

restore.o save.o : mvsave.h $(GLMHEADERS) $(IFACEH) $(GLOBALHEADERS) 

#special to WxWin version
$(WX)/wxmain.o: $(WX)/wxmain.cc  $(WXWININCLUDES) $(GLOBALHEADERS) mvsave.h
	$(CC) $(OPT) $(WARN) $(PROFILE) -c $(CPPFLAGS) $(MACHINE) $(UNIX) -o $@ $(WX)/wxmain.cc

$(WX)/wxtframe.o: $(WX)/wxtframe.cc $(WXWININCLUDES) $(GLOBALHEADERS)
	$(CC) $(OPT) $(WARN) $(PROFILE) -c $(CPPFLAGS) $(MACHINE) $(UNIX) -o $@ $(WX)/wxtframe.cc

$(WX)/wxtwind.o: $(WX)/wxtwind.cc $(WXWININCLUDES) $(GLOBALHEADERS)
	$(CC) $(OPT) $(WARN) $(PROFILE) -c $(CPPFLAGS) $(MACHINE) $(UNIX) -o $@ $(WX)/wxtwind.cc

$(WX)/wxgraph.o:        $(WX)/wxgraph.cc $(WXWININCLUDES) $(GLOBALHEADERS)
	$(CC) $(OPT) $(WARN) $(PROFILE) -c $(CPPFLAGS) $(MACHINE) $(UNIX) -o $@ $(WX)/wxgraph.cc

$(WX)/wxIo.o:        $(WX)/wxIo.cc $(WXWININCLUDES) $(GLOBALHEADERS)
	$(CC) $(OPT) $(WARN) $(PROFILE) -c $(CPPFLAGS) $(MACHINE) $(UNIX) -o $@ $(WX)/wxIo.cc

$(WX)/wxPrint.o:        $(WX)/wxPrint.cc $(WXWININCLUDES) $(GLOBALHEADERS)
	$(CC) $(OPT) $(WARN) $(PROFILE) -c $(CPPFLAGS) $(MACHINE) $(UNIX) -o $@ $(WX)/wxPrint.cc

$(WX)/wxdialog.o:       $(WX)/wxdialog.cc $(WXWININCLUDES)
	$(CC) $(OPT) $(WARN) $(PROFILE) -c $(CPPFLAGS) $(MACHINE) $(UNIX) -o $@ $(WX)/wxdialog.cc

$(WX)/wxhandl.o: 	     $(WX)/wxhandl.c $(GLOBALHEADERS) $(PROTOHEADERS)
	$(CCC) $(OPT) $(WARN) $(MACHINE)  $(CFLAGS) \
	$(PROFILE) -c $(UNIX) -o $(WX)/wxhandl.o $(WX)/wxhandl.c

# targets to make the library ($WXLIB) when needed.  As of 960715, only the
# motif version has been tried. 

wxmotif:
	cd $(WXDIR)/src/x; $(MAKE) -f makefile.unx motif

# special to djgpp
doprnt.o: doprnt.c
	$(CCC) $(OPT) $(WARN) $(PROFILE) $(MACHINE) -O -DJGPP -DFULLPREC\
		-DMAXFRACT=18 -c doprnt.c

readline.o: readline.c readline.h history.h
	$(CCC) $(OPT) $(WARN) $(PROFILE) $(CFLAGS) $(READLINEFLAGS)\
		$(READLINEFIX) -c readline.c

.c.o: $(GLOBALHEADERS) $(PROTOHEADERS)
	$(CCC) $(OPT) $(WARN) $(PROFILE) $(MACHINE) $(CFLAGS)\
		-c $(UNIX) $*.c

clean:
	rm -f NAMEFORBINARIES
	rm -f *.o wx/*.o

tarsrcfiles: $(MAKEFILE)
	RELEASENUMBER=$(RELEASENUMBER);export VERSIONNUMBER;\
	VERSIONNUMBER=$(VERSIONNUMBER);export RELEASENUMBER;\
	make -f tarfiles.mk tarsrcfiles

tarauxfiles: $(MAKEFILE)
	RELEASENUMBER=$(RELEASENUMBER);export VERSIONNUMBER;\
	VERSIONNUMBER=$(VERSIONNUMBER);export RELEASENUMBER;\
	make -f tarfiles.mk tarauxfiles

tardocfiles: $(MAKEFILE)
	RELEASENUMBER=$(RELEASENUMBER);export VERSIONNUMBER;\
	VERSIONNUMBER=$(VERSIONNUMBER);export RELEASENUMBER;\
	make -f tarfiles.mk tardocfiles

tarobjfiles: $(MAKEFILE)
	RELEASENUMBER=$(RELEASENUMBER);export VERSIONNUMBER;\
	VERSIONNUMBER=$(VERSIONNUMBER);export RELEASENUMBER;\
	make -f tarfiles.mk tarobjfiles

tarfiles: tarsrcfiles tarauxfiles # not including tardocfiles

