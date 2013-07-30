#
# Borland C++ IDE generated makefile
#
.AUTODEPEND


#
# Borland C++ tools
#
IMPLIB  = Implib
BCCDOS  = Bcc +BccDos.cfg 
TLINK   = TLink
TLIB    = TLib
TASM    = Tasm
#
# IDE macros
#


#
# Options
#
IDE_LFLAGSDOS =  -LD:\BC4\LIB
IDE_BFLAGS = 
LLATDOS_maov4p0dexe =  -LD:\BC45\LIB -c -Tde
RLATDOS_maov4p0dexe = 
BLATDOS_maov4p0dexe = 
CNIEAT_maov4p0dexe = -ID:\BC45\INCLUDE -DBCPP
LNIEAT_maov4p0dexe = -x
LEAT_maov4p0dexe = $(LLATDOS_maov4p0dexe)
REAT_maov4p0dexe = $(RLATDOS_maov4p0dexe)
BEAT_maov4p0dexe = $(BLATDOS_maov4p0dexe)
CLATDOS_initialidc = 
LLATDOS_initialidc = 
RLATDOS_initialidc = 
BLATDOS_initialidc = 
CEAT_initialidc = $(CEAT_maov4p0dexe) $(CLATDOS_initialidc)
CNIEAT_initialidc = -ID:\BC45\INCLUDE -DBCPP
LNIEAT_initialidc = -x
LEAT_initialidc = $(LEAT_maov4p0dexe) $(LLATDOS_initialidc)
REAT_initialidc = $(REAT_maov4p0dexe) $(RLATDOS_initialidc)
BEAT_initialidc = $(BEAT_maov4p0dexe) $(BLATDOS_initialidc)

#
# Dependency List
#
Dep_maov3p6 = \
   maov4p0.exe

maov3p6 : BccDos.cfg $(Dep_maov3p6)
  echo MakeNode 

Dep_maov4p0dexe = \
   labutils.obj\
   studrang.obj\
   keywords.obj\
   comclipb.obj\
   select.obj\
   qr.obj\
   trideige.obj\
   rotation.obj\
   toeplitz.obj\
   trans.obj\
   transpos.obj\
   tserops.obj\
   tsersubs.obj\
   ttests.obj\
   unbalano.obj\
   doshandl.obj\
   dosio.obj\
   utils.obj\
   utilstru.obj\
   varnames.obj\
   yates.obj\
   yylex.obj\
   anovacoe.obj\
   array.obj\
   batch.obj\
   betabase.obj\
   bin.obj\
   blas.obj\
   boxplot.obj\
   boxprep.obj\
   cellstat.obj\
   cellstts.obj\
   changest.obj\
   cholesky.obj\
   cluster.obj\
   columnop.obj\
   comhandl.obj\
   commonio.obj\
   concat.obj\
   contrast.obj\
   cor.obj\
   delete.obj\
   describe.obj\
   dim.obj\
   eigen.obj\
   eigutils.obj\
   errormsg.obj\
   fft.obj\
   fftsubs.obj\
   funbalan.obj\
   gammabas.obj\
   getmodel.obj\
   glm.obj\
   glmutils.obj\
   gramschm.obj\
   graphics.obj\
   handlers.obj\
   hconcat.obj\
   help.obj\
   ifsetup.obj\
   initiali.obj\
   ipf.obj\
   iswhat.obj\
   iterfns.obj\
   iterglm.obj\
   lang.obj\
   linpacch.obj\
   linpacge.obj\
   linpacqr.obj\
   list.obj\
   lpsforwd.obj\
   lpslandb.obj\
   lpsmisc.obj\
   macro.obj\
   macroset.obj\
   main.obj\
   mainpars.obj\
   makefact.obj\
   makestr.obj\
   mathutil.obj\
   matrix.obj\
   movavg.obj\
   mreader.obj\
   myplot.obj\
   normbase.obj\
   outer.obj\
   partacf.obj\
   paste.obj\
   plotutil.obj\
   polyroot.obj\
   power.obj\
   predtabl.obj\
   print.obj\
   printano.obj\
   printreg.obj\
   pvals.obj\
   pvalsub.obj\
   random.obj\
   rankquic.obj\
   rational.obj\
   readdata.obj\
   readfile.obj\
   readhead.obj\
   regpred.obj\
   rep.obj\
   restore.obj\
   run.obj\
   save.obj\
   screen.obj\
   setoptio.obj\
   shell.obj\
   solve.obj\
   sort.obj\
   sortquic.obj\
   split.obj\
   spool.obj\
   startmsg.obj\
   stemleaf.obj\
   stemsubs.obj\
   studentb.obj\
   subassig.obj\
   svd.obj\
   swp.obj\
   symbol.obj\
   tabs.obj\
   term.obj\
   inimacro.obj

maov4p0.exe : $(Dep_maov4p0dexe)
  $(TLINK)   @&&|
 /v $(IDE_LFLAGSDOS) $(LEAT_maov4p0dexe) $(LNIEAT_maov4p0dexe) +
D:\BC45\LIB\c0h.obj+
/o+ labutils.obj+
studrang.obj+
keywords.obj+
comclipb.obj+
select.obj+
qr.obj+
trideige.obj+
rotation.obj+
toeplitz.obj+
trans.obj+
transpos.obj+
tserops.obj+
tsersubs.obj+
ttests.obj+
unbalano.obj+
/o- doshandl.obj+
dosio.obj+
utils.obj+
utilstru.obj+
/o+ varnames.obj+
yates.obj+
/o- yylex.obj+
/o+ anovacoe.obj+
array.obj+
batch.obj+
betabase.obj+
bin.obj+
/o- blas.obj+
/o+ boxplot.obj+
boxprep.obj+
cellstat.obj+
cellstts.obj+
changest.obj+
cholesky.obj+
cluster.obj+
columnop.obj+
/o- comhandl.obj+
commonio.obj+
/o+ concat.obj+
contrast.obj+
cor.obj+
delete.obj+
describe.obj+
dim.obj+
eigen.obj+
eigutils.obj+
/o- errormsg.obj+
/o+ fft.obj+
fftsubs.obj+
funbalan.obj+
gammabas.obj+
getmodel.obj+
glm.obj+
glmutils.obj+
gramschm.obj+
graphics.obj+
/o- handlers.obj+
/o+ hconcat.obj+
help.obj+
/o- ifsetup.obj+
/o+ initiali.obj+
ipf.obj+
iswhat.obj+
iterfns.obj+
iterglm.obj+
/o- lang.obj+
/o+ linpacch.obj+
linpacge.obj+
linpacqr.obj+
list.obj+
lpsforwd.obj+
lpslandb.obj+
lpsmisc.obj+
macro.obj+
macroset.obj+
/o- main.obj+
mainpars.obj+
/o+ makefact.obj+
makestr.obj+
/o- mathutil.obj+
/o+ matrix.obj+
movavg.obj+
mreader.obj+
myplot.obj+
normbase.obj+
outer.obj+
partacf.obj+
paste.obj+
plotutil.obj+
polyroot.obj+
power.obj+
predtabl.obj+
/o- print.obj+
/o+ printano.obj+
printreg.obj+
pvals.obj+
pvalsub.obj+
random.obj+
rankquic.obj+
rational.obj+
readdata.obj+
readfile.obj+
readhead.obj+
regpred.obj+
rep.obj+
restore.obj+
run.obj+
save.obj+
screen.obj+
setoptio.obj+
shell.obj+
solve.obj+
sort.obj+
sortquic.obj+
split.obj+
spool.obj+
startmsg.obj+
stemleaf.obj+
stemsubs.obj+
studentb.obj+
/o- subassig.obj+
/o+ svd.obj+
swp.obj+
/o- symbol.obj+
/o+ tabs.obj+
term.obj+
inimacro.obj
$<,$*
D:\BC45\LIB\overlay.lib+
D:\BC45\LIB\graphics.lib+
D:\BC45\LIB\emu.lib+
D:\BC45\LIB\mathh.lib+
D:\BC45\LIB\ch.lib

|

labutils.obj :  labutils.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ labutils.c
|

studrang.obj :  studrang.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ studrang.c
|

keywords.obj :  keywords.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ keywords.c
|

comclipb.obj :  comclipb.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ comclipb.c
|

select.obj :  select.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ select.c
|

qr.obj :  qr.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ qr.c
|

trideige.obj :  trideige.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ trideige.c
|

rotation.obj :  rotation.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ rotation.c
|

toeplitz.obj :  toeplitz.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ toeplitz.c
|

trans.obj :  trans.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ trans.c
|

transpos.obj :  transpos.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ transpos.c
|

tserops.obj :  tserops.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ tserops.c
|

tsersubs.obj :  tsersubs.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ tsersubs.c
|

ttests.obj :  ttests.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ ttests.c
|

unbalano.obj :  unbalano.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ unbalano.c
|

doshandl.obj :  doshandl.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ doshandl.c
|

dosio.obj :  dosio.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ dosio.c
|

#
## The following section is generated by duplicate target utils.obj.
##
#utils.obj :  utils.c
#  $(BCCDOS) -P- -c @&&|
# $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ utils.c
#|
#
##
## The above section is generated by duplicate target utils.obj.
##
#
#
utilstru.obj :  utilstru.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ utilstru.c
|

varnames.obj :  varnames.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ varnames.c
|

yates.obj :  yates.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ yates.c
|

yylex.obj :  yylex.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ yylex.c
|

anovacoe.obj :  anovacoe.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ anovacoe.c
|

array.obj :  array.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ array.c
|

batch.obj :  batch.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ batch.c
|

betabase.obj :  betabase.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ betabase.c
|

bin.obj :  bin.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ bin.c
|

blas.obj :  blas.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ blas.c
|

boxplot.obj :  boxplot.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ boxplot.c
|

boxprep.obj :  boxprep.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ boxprep.c
|

cellstat.obj :  cellstat.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ cellstat.c
|

cellstts.obj :  cellstts.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ cellstts.c
|

changest.obj :  changest.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ changest.c
|

cholesky.obj :  cholesky.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ cholesky.c
|

cluster.obj :  cluster.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ cluster.c
|

columnop.obj :  columnop.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ columnop.c
|

comhandl.obj :  comhandl.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ comhandl.c
|

commonio.obj :  commonio.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ commonio.c
|

concat.obj :  concat.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ concat.c
|

contrast.obj :  contrast.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ contrast.c
|

cor.obj :  cor.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ cor.c
|

delete.obj :  delete.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ delete.c
|

describe.obj :  describe.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ describe.c
|

dim.obj :  dim.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ dim.c
|

eigen.obj :  eigen.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ eigen.c
|

eigutils.obj :  eigutils.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ eigutils.c
|

errormsg.obj :  errormsg.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ errormsg.c
|

fft.obj :  fft.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ fft.c
|

fftsubs.obj :  fftsubs.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ fftsubs.c
|

funbalan.obj :  funbalan.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ funbalan.c
|

gammabas.obj :  gammabas.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ gammabas.c
|

getmodel.obj :  getmodel.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ getmodel.c
|

glm.obj :  glm.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ glm.c
|

glmutils.obj :  glmutils.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ glmutils.c
|

gramschm.obj :  gramschm.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ gramschm.c
|

graphics.obj :  graphics.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ graphics.c
|

handlers.obj :  handlers.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ handlers.c
|

hconcat.obj :  hconcat.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ hconcat.c
|

help.obj :  help.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ help.c
|

ifsetup.obj :  ifsetup.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ ifsetup.c
|

initiali.obj :  initiali.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_initialidc) $(CNIEAT_initialidc) -o$@ initiali.c
|

ipf.obj :  ipf.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ ipf.c
|

iswhat.obj :  iswhat.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ iswhat.c
|

iterfns.obj :  iterfns.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ iterfns.c
|

iterglm.obj :  iterglm.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ iterglm.c
|

lang.obj :  lang.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ lang.c
|

linpacch.obj :  linpacch.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ linpacch.c
|

linpacge.obj :  linpacge.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ linpacge.c
|

linpacqr.obj :  linpacqr.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ linpacqr.c
|

list.obj :  list.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ list.c
|

lpsforwd.obj :  lpsforwd.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ lpsforwd.c
|

lpslandb.obj :  lpslandb.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ lpslandb.c
|

lpsmisc.obj :  lpsmisc.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ lpsmisc.c
|

macro.obj :  macro.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ macro.c
|

macroset.obj :  macroset.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ macroset.c
|

main.obj :  main.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ main.c
|

mainpars.obj :  mainpars.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ mainpars.c
|

makefact.obj :  makefact.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ makefact.c
|

makestr.obj :  makestr.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ makestr.c
|

mathutil.obj :  mathutil.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ mathutil.c
|

matrix.obj :  matrix.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ matrix.c
|

movavg.obj :  movavg.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ movavg.c
|

mreader.obj :  mreader.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ mreader.c
|

myplot.obj :  myplot.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ myplot.c
|

normbase.obj :  normbase.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ normbase.c
|

outer.obj :  outer.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ outer.c
|

partacf.obj :  partacf.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ partacf.c
|

paste.obj :  paste.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ paste.c
|

plotutil.obj :  plotutil.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ plotutil.c
|

polyroot.obj :  polyroot.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ polyroot.c
|

power.obj :  power.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ power.c
|

predtabl.obj :  predtabl.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ predtabl.c
|

print.obj :  print.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ print.c
|

printano.obj :  printano.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ printano.c
|

printreg.obj :  printreg.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ printreg.c
|

pvals.obj :  pvals.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ pvals.c
|

pvalsub.obj :  pvalsub.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ pvalsub.c
|

random.obj :  random.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ random.c
|

rankquic.obj :  rankquic.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ rankquic.c
|

rational.obj :  rational.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ rational.c
|

readdata.obj :  readdata.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ readdata.c
|

readfile.obj :  readfile.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ readfile.c
|

readhead.obj :  readhead.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ readhead.c
|

regpred.obj :  regpred.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ regpred.c
|

#
## The following section is generated by duplicate target rep.obj.
##
#rep.obj :  rep.c
#  $(BCCDOS) -P- -c @&&|
# $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ rep.c
#|
#
##
## The above section is generated by duplicate target rep.obj.
##
#
#
restore.obj :  restore.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ restore.c
|

run.obj :  run.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ run.c
|

save.obj :  save.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ save.c
|

screen.obj :  screen.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ screen.c
|

setoptio.obj :  setoptio.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ setoptio.c
|

shell.obj :  shell.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ shell.c
|

solve.obj :  solve.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ solve.c
|

sort.obj :  sort.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ sort.c
|

sortquic.obj :  sortquic.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ sortquic.c
|

split.obj :  split.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ split.c
|

spool.obj :  spool.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ spool.c
|

startmsg.obj :  startmsg.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ startmsg.c
|

stemleaf.obj :  stemleaf.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ stemleaf.c
|

stemsubs.obj :  stemsubs.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ stemsubs.c
|

studentb.obj :  studentb.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ studentb.c
|

subassig.obj :  subassig.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ subassig.c
|

svd.obj :  svd.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ svd.c
|

swp.obj :  swp.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ swp.c
|

symbol.obj :  symbol.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ symbol.c
|

tabs.obj :  tabs.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ tabs.c
|

term.obj :  term.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ term.c
|

inimacro.obj :  inimacro.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_maov4p0dexe) $(CNIEAT_maov4p0dexe) -o$@ inimacro.c
|

# Compiler configuration file
BccDos.cfg : 
   Copy &&|
-W-
-R
-v
-vi
-H
-H=maov3p5.csm
-R-
-X-
-v-
-vi-
-H-
-mh
-f
-Y
-g200
-2
-a-
-O-c
-Os
-Z-
-O-l
-O-b
-O-i
-O-v
| $@


