
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
	Contains the text of MacAnova pre-defined macros.  If this file
	is iniMacro.mac, they are in the standard form readable
	by macro read, interspersed with C comments and pre-processor
	directives.  If this file is iniMacro.h, they are as quoted strings
	in the format needed for inclusion in iniMacro.c and for processing
	by script iniMacro.awk

	iniMacro.h is produced from iniMacro.mac by
	  sh iniMacro.sh iniMacro.mac iniMacro.h

    990305 File created with comment lines and preprocessor directives
           from iniMacro.c.
    990309 Minor modification of several macros and addition of comments
	       Deleted fcolplot and frowplot
    990324 Fixed bug in rowplot
*/

#if 0
   In the following quoted comments are deliminated by /@..@/.

   This file must be in a particular format to allow creation of a MPW rez
   file using script iniMacro.awk .  If additional macros are added they
   should follow this format.

   The script copies the copyright information and exctracts the values of
   macros MACROTYPE, ENDNAME, IDBASE and NAMEPREFIX (see above) and then
   ignores everything up through the line '/@STARTMACROS@/'.  It ignores all
   other  preprocessor commands.

   The quoted name of each macro followed by a comma with no intervening
   whitespace must be the second field of any line line whose first field
   is '/@MNAME@/'.

   The quoted text of a macro must start with a line whose first field is
   '/@MTEXT@/'. All lines except the last should end in \

   The final line of a macro must terminate with '", /@MEND@/', where '"'
   is the terminating quote for the macro and there must be no intervening
   whitespace before the comma.  For a one line macro this would be on
   the same line with '/@MTEXT@/'

   The script skips any lines bracketed by lines whose first fields are
   '/@NOTFORMACSTART@/' and '/@NOTFORMACEND@/'.

   ('@' is used here instead of '#' because the MPW C preprocessor got
   confused otherwise.)

   Preprocessor lines are not copied

   Scanning terminates when the line containing '/@ENDMACROS@/' is found

   961104 Slightly modified many macros, replacing $1, $2, ... by $01, $02
   which will mean less memory needed for expansion and less scanning by
   yylex when expanded.

   980730 Changed type of component 'text' to char * from unsigned char *
#endif /*if 0*/

/*
   INIMACROHISTORY  (recognized by iniMacro.awk in creating macInimacro.r)
   Initialize pre-defined macros
   971108 added new versions of resid and yhat which label rows and columns
   971111 added new pre-defined macro regcoefs
   980102 Modified makecols to allow final argument nomissing:T
   980710 Replaced anytrue and alltrue with versions that use evaluate
   980727 Added hasnotes
   981120 Added nofileok:T to macroread calls in getmacros
   981121 Deleted all use of "ERROR: " in macro calls to error()
*/

#ifdef MACINTOSH
#include "macIface.h"

#define MACROTYPE   'wstr'
#define ENDNAME     "_END_"
#define IDBASE      10000
#define NAMEPREFIX  "_MV_"
#endif /*MACINTOSH*/

/* Start_Of_Macros */

/*STARTMACROS*/

/*MNAME*/ "redo",
/*MTEXT*/ "REDO <- macro({if($N==0){LASTLINE}else{$01}})\n\
REDO()", /*MEND*/


/*MNAME*/ "readcols",
/*MTEXT*/ "# $S(filename,name1,...,namek [,echo:T or F]), only filename quoted\n\
@nv$$ <- $v-1\n\
if (!isnull(keyvalue($K,\"string\",\"char\")) ||\\\n\
\t!isnull(keyvalue($K,\"file\",\"char\"))){\n\
\t@nv$$ <-+ 1\n\
}else{\n\
\t@file$$ <- $01\n\
\tif(!ischar(@file$$) || !isscalar(@file$$)){\n\
\t\terror(\"$1 not quoted string or CHARACTER scalar\")\n\
\t}else{delete(@file$$)}\n\
}\n\
if(@nv$$ < 1){\n\
\terror(\"too few arguments for $S\")\n\
}\n\
@names$$ <- if(@nv$$ == 1){\n\
\tif(ischar($02)){$2}else{\"$2\"}\n\
}else{$A[1+run(@nv$$)]}\n\
@n$$ <- length(@names$$)\n\
@data$$ <- \\\n\
 if(@nv$$==$v){vecread($K)}elseif($k>0){vecread($01,$K)}else{vecread($01)}\n\
@data$$ <- matrix(@data$$,@n$$)\n\
for(@i$$,run(@n$$)){\n\
\t<<@names$$[@i$$]>> <- vector(@data$$[@i$$,]);;\n\
}\n\
delete(@data$$,@names$$,@n$$,@nv$$)", /*MEND*/


/*MNAME*/ "twotailt",
/*MTEXT*/ "#$S(tval,df)\n\
@tval$$ <- $1\n\
@df$$ <- $2\n\
if(!isreal(@tval$$) || !isreal(@df$$)){\n\
\terror(\"both arguments to $S must be REAL\")\n\
}\n\
if(length(@tval$$)>1 && length(@df$$)>1 && length(@tval$$)!=length(@df$$)){\n\
\terror(\"argument lengths don't match\")\n\
}\n\
2*(1 - cumstu(abs(@tval$$),@df$$))", /*MEND*/


/*MNAME*/ "resvsrankits",
/*MTEXT*/ "#$S([jvar [,char]] [,title:\"Your title\"])\n\
if(!isdefined(DF) || !isdefined(SS) ||\\\n\
!isdefined(RESIDUALS) || !isdefined(HII)){\n\
\terror(\"Apparently no linear model has been fitted\")\n\
}\n\
@m$$ <- length(DF)\n\
if (DF[@m$$] == 0){\n\
\terror(\"no degrees of freedom for error; can't studentize\")}\n\
@w$$ <- getoptions()\n\
setoptions(warnings:F)\n\
@chars$$ <- \"*\"\n\
if($v==2){\n\
\t@chars$$ <- $02\n\
\tif(isreal(@chars$$) && isscalar(@chars$$)){\n\
\t\tif(@chars$$==0){\n\
\t\t\t@chars$$ <- run(nrows(RESIDUALS))\n\
\t\t}\n\
\t}\n\
}\n\
@i$$ <- if($v==0){1}else{$01}\n\
@r$$ <- if(isdefined(WTDRESIDUALS)){WTDRESIDUALS[,@i$$]}else{RESIDUALS[,@i$$]}\n\
@r$$ <- @r$$/sqrt((SS[@m$$,@i$$,@i$$]/DF[@m$$])*(1 - HII))\n\
chplot(Rankits:rankits(@r$$),@r$$,@chars$$,DD:0,$K,ylab:\"Studentized Resids\")\n\
setoptions(@w$$)\n\
delete(@m$$,@w$$,@i$$,@r$$,@chars$$)", /*MEND*/


/*MNAME*/ "resvsyhat",
/*MTEXT*/ "#$S([jvar [,char]] [,title:\"Your title\"])\n\
if(!isdefined(DF) || !isdefined(SS) ||\\\n\
!isdefined(RESIDUALS) || !isdefined(HII)){\n\
\terror(\"Apparently no linear model has been fitted\")\n\
}\n\
@m$$ <- length(DF)\n\
if (DF[@m$$] == 0){\n\
\terror(\"no degrees of freedom for error; can't studentize\")}\n\
@w$$ <- getoptions()\n\
setoptions(warnings:F)\n\
@chars$$ <- \"*\"\n\
if($v==2){\n\
\t@chars$$ <- $02\n\
\tif(isreal(@chars$$) && isscalar(@chars$$)){\n\
\t\tif(@chars$$==0){\n\
\t\t\t@chars$$ <- run(nrows(RESIDUALS))\n\
\t\t}\n\
\t}\n\
}\n\
@i$$ <- if($v==0){1}else{$01}\n\
@r$$ <- if(isdefined(WTDRESIDUALS)){WTDRESIDUALS[,@i$$]}else{RESIDUALS[,@i$$]}\n\
@r$$ <- @r$$/sqrt((SS[@m$$,@i$$,@i$$]/DF[@m$$])*(1 - HII))\n\
chplot(Yhat:modelvars(y:T)[,@i$$]-RESIDUALS[,@i$$],@r$$,@chars$$,DD:0,$K,\\\n\
\tylab:\"Studentized Resids\")\n\
setoptions(@w$$)\n\
delete(@m$$,@w$$,@i$$,@r$$)", /*MEND*/


/*MNAME*/ "resvsindex",
/*MTEXT*/ "#$S([jvar [,char]] [,title:\"Your title\"])\n\
if(!isdefined(DF) || !isdefined(SS) ||\\\n\
!isdefined(RESIDUALS) || !isdefined(HII)){\n\
\terror(\"Apparently no linear model has been fitted\")\n\
}\n\
@m$$ <- length(DF)\n\
if (DF[@m$$] == 0){\n\
\terror(\"no degrees of freedom for error; can't studentize\")}\n\
@chars$$ <- \"*\"\n\
if($v==2){\n\
\t@chars$$ <- $02\n\
\tif(isreal(@chars$$) && isscalar(@chars$$)){\n\
\t\tif(@chars$$==0){\n\
\t\t\t@chars$$ <- run(nrows(RESIDUALS))\n\
\t\t}\n\
\t}\n\
}\n\
@w$$ <- getoptions()\n\
setoptions(warnings:F)\n\
@i$$ <- if($v==0){1}else{$01}\n\
@r$$ <- if(isdefined(WTDRESIDUALS)){WTDRESIDUALS[,@i$$]}else{RESIDUALS[,@i$$]}\n\
@r$$ <- @r$$/sqrt((SS[@m$$,@i$$,@i$$]/DF[@m$$])*(1 - HII))\n\
setoptions(@w$$)\n\
chplot(1,@r$$,@chars$$,DD:0,$K,xlab:\"Case Number\",\\\n\
ylab:\"Studentized Resids\")\n\
delete(@m$$,@w$$,@i$$,@r$$)", /*MEND*/


/*MNAME*/ "hist",
/*MTEXT*/ "# usage: $S(x), $S(x,nbins), or $S(x,binvec)\n\
if($v < 1 || $v > 2){\n\
\terror(\"usage is $S(x), $S(x,nbins) or $S(x,binvec)\")\n\
}\n\
@x$$ <- $01\n\
if(!isvector(@x$$)||!isreal(@x$$)){error(\"$1 is not a REAL vector\")}\n\
if(anymissing(@x$$)){@x$$ <- @x$$[!ismissing(@x$$)]}\n\
if($v == 2){\n\
\t@bins$$ <- $02\n\
\tif(!isvector(@bins$$)||!isreal(@bins$$)){\n\
\t\terror(\"$2 is not a REAL scalar or vector\")\n\
\t}\n\
}else{\n\
\t@bins$$ <- floor(log(length(@x$$))/log(2)) + 1\n\
}\n\
@leftin$$ <- keyvalue($K,\"leftendin\",\"logical\")\n\
@leftin$$ <- if (!isnull(@leftin$$)){@leftin$$}else{F}\n\
@nbins$$ <- if(isscalar(@bins$$)){@bins$$}else{length(@bins$$)-1}\n\
if(@nbins$$ <= 0 || @nbins$$ != floor(@nbins$$)){\n\
\terror(\"$2 is not positive integer\")\n\
}\n\
if(isscalar(@bins$$)){\n\
\t@min$$ <- 1.05*min(@x$$) - .05*max(@x$$)\n\
\t@max$$ <- 1.05*max(@x$$) - .05*min(@x$$)\n\
\t@widths$$ <- (@max$$ - @min$$)/@nbins$$\n\
\t@bins$$ <- run(@min$$,@max$$+.1*@widths$$,@widths$$)\n\
} else {\n\
\tif(min(@x$$) <= min(@bins$$) || max(@x$$) > max(@bins$$)){\n\
\t\terror(\"minimum or maximum of data outside of bins\")\n\
\t}\n\
}\n\
@widths$$ <- movavg(1,@bins$$)[-1]\n\
if(min(@widths$$) < 0){\n\
\terror(\"bin boundaries are not monotonic\")\n\
}\n\
@hts$$ <- bin(@x$$, @bins$$, leftendin:@leftin$$)$counts/@widths$$\n\
@xpts$$ <- vector(hconcat(@bins$$,@bins$$,@bins$$)')[-1]\n\
@ypts$$ <- vector(0,hconcat(@hts$$,@hts$$,rep(0,length(@hts$$)))',0)/length(@x$$)\n\
lineplot(@xpts$$,@ypts$$,$K,xlab:\"$1\",ylab:\"Density\",\\\n\
\tyaxis:F,title:\"Histogram of $1 with total area 1\")\n\
delete(@x$$,@bins$$,@nbins$$,@hts$$,@xpts$$,@ypts$$,@widths$$)", /*MEND*/


/*MNAME*/ "vboxplot",
/*MTEXT*/ "#usage: $S(x1,x2, ... [,keywords]  or $S(struct [,keywords])\n\
if ($v == 0){error(\"no data provided to $S\")}\n\
if($k>0){boxplot($V,$K,vertical:T)}else{boxplot($V,vertical:T)}", /*MEND*/


/*MNAME*/ "boxcox",
/*MTEXT*/ "# usage: $S(x,power), x a vector or matrix, power a scalar\n\
@x$$ <- argvalue($1,\"argument 1\",vector(\"real\"))\n\
@power$$ <- argvalue($2,\"power argument\",vector(\"real\",\"nonmissing\",\"scalar\"))\n\
@dims$$ <- dim(@x$$)\n\
if (sum(vector(ismissing(@x$$))) != length(@x$$)){\n\
\tif (min(@x$$[vector(!ismissing(@x$$))]) <= 0){\n\
\t\terror(\"first argument to $S has zero or negative values\")\n\
\t}\n\
\tif (ndims(@x$$) > 2){@x$$ <- matrix(vector(@x$$),@dims$$[1])}\n\
\t@w$$ <- getoptions(warnings:T)\n\
\tsetoptions(warnings:F)\n\
\t@gm$$ <- exp(describe(log(@x$$),mean:T))'\n\
\tif ( @power$$ == 0 ) {\n\
\t\t@x$$ <- @gm$$ * log(@x$$)\n\
\t} else {\n\
\t\t@x$$ <- (@x$$^@power$$ - 1)/(@power$$*@gm$$^(@power$$-1))\n\
\t}\n\
\tsetoptions(warnings:delete(@w$$,return:T))\n\
\tdelete(@gm$$)\n\
}\n\
delete(@power$$)\n\
array(delete(@x$$,return:T),delete(@dims$$,return:T))", /*MEND*/


/*MNAME*/ "makefactor",
/*MTEXT*/ "# $S(values) where values either REAL or CHAR\n\
if($v < 1 || $v > 2){\n\
\terror(\"usage is $S(vec [, sort]), sort T or F, default T\")\n\
}\n\
@values$$ <- argvalue($1, \"argument 1\", \"vector\")\n\
@sort$$ <- if($v == 1){T}else{\n\
\targvalue($2, \"argument 2\", vector(\"logical\",\"scalar\",\"nonmissing\"))\n\
}\n\
if(!@sort$$){\n\
\tfactor(match(@values$$,unique(@values$$)))\n\
}else{\n\
\tfactor(match(@values$$,unique(sort(@values$$))))\n\
}", /*MEND*/


/*MNAME*/ "resid",
/*MTEXT*/ "# $S() or $S(model) or $S(model,T), where model is a linear model\n\
if($N > 2){\n\
\terror(\"usage is $S() or $S(model) or $S(model,T)\")\n\
}\n\
@reg$$ <- if ($N<=1){F}else{\n\
\targvalue($2,\"argument 2\", vector(\"logical scalar nonmissing\"))\n\
}#T forces regress()\n\
if($N >= 1){\n\
\t@model$$ <- argvalue($1,\"argument 1\",vector(\"character\",\"scalar\"))\n\
\tif(@reg$$){regress(@model$$,silent:T)}else{manova(@model$$,silent:T)}\n\
\tdelete(@model$$)\n\
}\n\
if(!isdefined(RESIDUALS)||!isdefined(SS)||!isdefined(HII)||!isdefined(DF)){\n\
\terror(\"no active model; try $S(model)\")\n\
}\n\
@m$$ <- dim(SS)[1] # number of terms including error\n\
if (DF[@m$$] == 0){\n\
\terror(\"no degrees of freedom for error; can't studentize\")\n\
}\n\
@w$$ <- getoptions(warnings:T)\n\
setoptions(warnings:T)\n\
@r$$ <- if(isdefined(WTDRESIDUALS)){WTDRESIDUALS}else{RESIDUALS}\n\
@r$$ <- @r$$/sqrt((1-HII)*(diag(SS[@m$$,,])'/DF[@m$$]))\n\
@p$$ <- nrows(SS[1,,])\n\
if (@p$$ == 1){\n\
\t@labels$$ <- vector(\"Depvar\",\"StdResids\",\"HII\",\"Cook's D\",\\\n\
\t\t\"t-stats\")}else{\n\
 \t@dlabels$$ <- @rslabels$$ <-  @rtlabels$$ <-  @rclabels$$ <- NULL\n\
\tfor (@i$$,run(@p$$)){\n\
\t\t@dlabels$$ <- vector(@dlabels$$, paste(\"Depvar\",@i$$))\n\
\t\t@rslabels$$ <- vector(@rslabels$$, paste(\"StdResids\",@i$$))\n\
\t\t@rclabels$$ <- vector(@rclabels$$, paste(\"Cook's D\",@i$$))\n\
\t\t@rtlabels$$ <- vector(@rtlabels$$, paste(\"t-stats\",@i$$))\n\
\t}\n\
\t@labels$$ <- vector(@dlabels$$,@rslabels$$,\"HII\",@rclabels$$,@rtlabels$$)\n\
\tdelete(@dlabels$$,@rslabels$$,@rtlabels$$,@rclabels$$)\n\
}\n\
@o$$ <- matrix(vector(\\\n\
\tmodelvars(0),\\\n\
\t@r$$,\\\n\
\tHII,\\\n\
\t@r$$^2*HII/((1-HII)*sum(DF[-@m$$])),\\\n\
\t@r$$*sqrt((DF[@m$$]-1)/(DF[@m$$]-@r$$^2))),nrows(@r$$),\\\n\
\tlabels:structure(\"(\",@labels$$))\n\
setoptions(warnings:@w$$)\n\
delete(@w$$,@r$$,@m$$,@p$$,@labels$$)\n\
delete(@o$$,return:T)", /*MEND*/


/*MNAME*/ "yhat",
/*MTEXT*/ "# $S() or $S(model) or $S(model,T), where model is a linear model\n\
if($N > 2){\n\
\terror(\"usage is $S() or $S(model) or $S(model,T)\")\n\
}\n\
@reg$$ <- if($N<=1){F}else{\n\
\targvalue($2, \"argument 2\", vector(\"logical\",\"scalar\",\"nonmissing\"))\n\
}\n\
if($N >= 1){\n\
\t@model$$ <- argvalue($1, \"argument 1\",vector(\"character\",\"scalar\"))\n\
\tif(@reg$$){regress(@model$$,silent:T)}else{manova(@model$$,silent:T)}\n\
\tdelete(@model$$)\n\
}\n\
if(!isdefined(RESIDUALS) || !isdefined(SS) ||\\\n\
\t!isdefined(HII) || !isdefined(DF)){\n\
\terror(\"apparently no active linear model; try $S(model)\")\n\
}\n\
@m$$ <- dim(SS)[1]#number of terms counting error\n\
if (DF[@m$$] == 0){\n\
\terror(\"no degrees of freedom for error; can't studentize\")}\n\
@w$$ <- getoptions(warnings:T)\n\
setoptions(warnings:T)\n\
@p$$ <- nrows(SS[1,,])\n\
if (@p$$ == 1){\n\
\t@labels$$ <- vector(\"Depvar\",\"Pred\",\"Pred Resid\",\"SE Est\", \"SE Pred\")\n\
}else{\n\
\t@ylabels$$ <- @plabels$$ <-  @prlabels$$ <-  @seelabels$$ <-\\\n\
\t\t@seplabels$$ <- NULL\n\
\tfor (@i$$,run(@p$$)){\n\
\t\t@ylabels$$ <- vector(@ylabels$$, paste(\"Depvar\",@i$$))\n\
\t\t@plabels$$ <- vector(@plabels$$, paste(\"Pred\",@i$$))\n\
\t\t@prlabels$$ <- vector(@prlabels$$, paste(\"Pred Resid\",@i$$))\n\
\t\t@seelabels$$ <- vector(@seelabels$$, paste(\"SE Est\",@i$$))\n\
\t\t@seplabels$$ <- vector(@seplabels$$, paste(\"SE Pred\",@i$$))\n\
\t}\n\
\t@labels$$ <-\\\n\
\t\tvector(@ylabels$$,@plabels$$,@prlabels$$,@seelabels$$,@seplabels$$)\n\
\tdelete(@ylabels$$,@plabels$$,@prlabels$$,@seelabels$$,@seplabels$$)\n\
}\n\
@wts$$ <- if(isdefined(WTDRESIDUALS)){\n\
\tmodelinfo(weights:T)}else{rep(1,nrows(RESIDUALS))\n\
}\n\
@mse$$ <- diag(SS[@m$$,,])'/DF[@m$$]\n\
@depvar$$ <- modelvars(0)\n\
delete(@reg$$, @m$$)\n\
@o$$ <- matrix(vector(\\\n\
\t@depvar$$ ,\\\n\
\t@depvar$$-RESIDUALS,\\\n\
\tRESIDUALS/(1-HII),\\\n\
\tsqrt(HII*@mse$$/@wts$$),\\\n\
\tsqrt((1+HII/@wts$$)*@mse$$)),nrows(@depvar$$),\\\n\
\tlabels:structure(\"(\",@labels$$))\n\
setoptions(warnings:@w$$)\n\
delete(@w$$,@wts$$,@depvar$$,@mse$$, @labels$$)\n\
delete(@o$$, return:T)", /*MEND*/


/*MNAME*/ "regcoefs",
/*MTEXT*/ "# $S([pvals:T][,byvar:T]) or $S(model[,pvals:T][,byvar:T])\n\
if ($v > 1 || $k > 2){\n\
\terror(\"usage is $S([pvals:T][,byvar:T]) or $S(model[,pvals:T][,byvar:T])\")\n\
}\n\
@pvals$$ <- keyvalue($K,\"pvals\",\"logic\")\n\
@byvar$$ <- keyvalue($K,\"byvar\",\"logic\")\n\
if ($v == 1){\n\
\t@model$$ <- argvalue($1,\"model\",vector(\"character\",\"scalar\"))\n\
\tmanova(delete(@model$$, return:T),silent:T)\n\
}\n\
@p$$ <- nrows(SS[1,,])\n\
@mterms$$ <- dim(SS)[1]\n\
if (isnull(@pvals$$)){@pvals$$ <- getoptions(pvals:T)}\n\
if (isnull(@byvar$$)){\n\
\t@byvar$$ <- (@p$$ != 1)\n\
}elseif(@p$$ == 1 && @byvar$$){\n\
\tprint(\"WARNING: byvar:T ignored by $S with univariate response variable\")\n\
\t@byvar$$ <- F\n\
}\n\
@stuff$$ <- secoefs(byterm:F)\n\
if (max(vector(length(@stuff$$[1]))) > @p$$){\n\
\terror(\"$S does not allow factors in the model\")\n\
}\n\
if (isstruc(@stuff$$[1])){\n\
\t@rowlabels$$ <- compnames(@stuff$$[1])\n\
}else{\n\
\t@rowlabels$$ <- TERMNAMES[1]\n\
}\n\
@collabels$$ <- if (@p$$ == 1 || @byvar$$){\n\
\tvector(\"Coef\",\"StdErr\",\"t\")\n\
}else{\n\
\t@makelabs$$ <- macro(\"getlabels(vector(run(\\\\$1),labels:\\\\$2))\")\n\
\tvector(@makelabs$$(@p$$,\"Coef \"),\\\n\
\t@makelabs$$(@p$$,\"StdErr \"),@makelabs$$(@p$$,\"t \"))\n\
}\n\
if (DF[@mterms$$] == 0){@pvals$$ <- F}\n\
if (@pvals$$){\n\
\t@collabels$$ <- if (@p$$ == 1 || @byvar$$){\n\
\t\tvector(@collabels$$,\"P-Value\")\n\
\t}else{\n\
\t\tvector(@collabels$$,@makelabs$$(@p$$,\"P-Value \"))\n\
\t}\n\
}\n\
@coefs$$ <- vector(@stuff$$[1])\n\
@stderrs$$ <-  vector(@stuff$$[2])\n\
if (@p$$ > 1){\n\
\t@coefs$$ <- matrix(@coefs$$,@p$$)'\n\
\t@stderrs$$ <- matrix(@stderrs$$,@p$$)'\n\
}\n\
@tstats$$ <- if (DF[@mterms$$] > 0){@coefs$$/@stderrs$$}else{\n\
\t@pvals$$ <- F\n\
\tarray(rep(?,(@mterms$$-1)*@p$$),dim(@coefs$$))\n\
}\n\
if (!@byvar$$ || @p$$ == 1){\n\
\t@o$$ <- hconcat(@coefs$$,@stderrs$$,@tstats$$)\n\
\tif (@pvals$$){\n\
\t\t@o$$ <- hconcat(@o$$,2*(1-cumstu(abs(@tstats$$),DF[@mterms$$])))\n\
\t}\n\
\t@o$$ <- matrix(@o$$,labels:structure(@rowlabels$$,@collabels$$))\n\
}else{\n\
\t@o$$ <- split(run(@p$$)')\n\
\tfor (@i$$,run(@p$$)){\n\
\t\t@oi$$ <- hconcat(@coefs$$[,@i$$],@stderrs$$[,@i$$],@tstats$$[,@i$$])\n\
\t\tif (@pvals$$){\n\
\t\t\t@oi$$ <- hconcat(@oi$$,\\\n\
\t\t\t\t2*(1-cumstu(abs(@tstats$$[,@i$$]),DF[@mterms$$])))\n\
\t\t}\n\
\t\t@o$$ <- changestr(@o$$, @i$$, matrix(@oi$$,labels:structure(@rowlabels$$,@collabels$$)))\n\
\t}\n\
\tdelete(@oi$$,@i$$)\n\
 \t@o$$ <- strconcat(@o$$,compnames:\"Variable_\")\n\
}\n\
delete(@stuff$$,@p$$,@mterms$$,@pvals$$,@byvar$$,@coefs$$,@stderrs$$,@tstats$$)\n\
delete(@o$$,return:T)", /*MEND*/


/*MNAME*/ "anovapred",
/*MTEXT*/ "# $S(a,b,...) , where a, b, ..., are all the factors in the model\n\
if($v == 0){\n\
\terror(\"usage is $S(a,b,...) where a,b,... are all the factors in model\")\n\
}\n\
if(!isdefined(DEPVNAME) || !isdefined(RESIDUALS)||\\\n\
\t !isdefined(HII) || !isdefined(DF)){\n\
\terror(\"$S: apparently no active linear model\")\n\
}\n\
@error$$ <- dim(SS)[1]\n\
@mse$$ <- diag(SS[@error$$,,])'/DF[@error$$]\n\
structure(estimate:tabs(<<DEPVNAME>>-RESIDUALS,$V,mean:T),\\\n\
SEest:sqrt(tabs(@mse$$*HII,$V,mean:T)),\\\n\
SEpred:sqrt(tabs(@mse$$*(1+HII),$V,mean:T)))", /*MEND*/


/*MNAME*/ "makecols",
/*MTEXT*/ "# $S(y,name1,name2,...,namek [,nomissing:T]), matrix y, unquoted names name1,... \n\
if($v < 2 || $k > 1){\n\
\terror(\"usage: $S(y,name1,name2,... [,nomissing:T]\")\n\
}\n\
@data$$ <- matrix(argvalue($1,\"argument 1\",vector(\"real\",\"matrix\")))\n\
@strip$$ <- keyvalue($K,\"nomissing\",\"logic\")\n\
if(isnull(@strip$$)){@strip$$ <- F}\n\
@names$$ <- if($v == 2){\n\
\tif(ischar($02)){$02}else{\"$2\"}\n\
}else{$A[run(2,$v)]}\n\
@n$$ <- min(length(@names$$),ncols(@data$$))\n\
for(@i$$,run(@n$$)){\n\
\t<<@names$$[@i$$]>> <- vector(@data$$[,@i$$])\n\
\tif(@strip$$){\n\
\t\tif (anymissing(<<@names$$[@i$$]>>)){\n\
\t\t\t@I$$ <- !ismissing(<<@names$$[@i$$]>>)\n\
\t\t\t<<@names$$[@i$$]>> <- <<@names$$[@i$$]>>[@I$$]\n\
\t\t}\n\
\t}\n\
}\n\
NULL", /*MEND*/


/*MNAME*/ "colplot",
/*MTEXT*/ "#$S(y [,title:\\\"Title of your choice\\\")\n\
if($N<1){\n\
\terror(\"$S needs at least 1 argument\")\n\
}\n\
chplot(1,argvalue($1,\"argument 1\",vector(\"real\",\"matrix\")),\\\n\
\tlines:T,$K,xlab:\"Row Number\")", /*MEND*/


/*MNAME*/ "rowplot",
/*MTEXT*/ "#$S(y [,title:\\\"Title of your choice\\\"])\n\
if($v<1){\n\
\terror(\"$S needs at least 1 non keyword argument\")\n\
}\n\
chplot(1,argvalue($1,\"argument 1\",vector(\"real\",\"matrix\"))',\\\n\
\tlines:T,$K,xlab:\"Column Number\")", /*MEND*/


/*MNAME*/ "regs",
/*MTEXT*/ "#$S(x,y),matrix or vector y, matrix x\n\
@Xvars$$ <- $1\n\
@Y <- $2\n\
if(!ismatrix(@Xvars$$)||!ismatrix(@Y)){\n\
\terror(\"usage: $S(x,y), matrix x, vector or matrix y\")\n\
}\n\
@p$$ <- length(@Xvars$$)/dim(@Xvars$$)[1]\n\
@X1 <- @Xvars$$[,1]\n\
STRMODEL <- \"@Y=@X1\"\n\
if(@p$$>1){\n\
\tfor(@i$$,run(2,@p$$)){\n\
\t\t<<paste(\"@X\",@i$$,sep:\"\")>> <- @Xvars$$[,@i$$]\n\
\t\tSTRMODEL <- paste(STRMODEL,\"+@X\",@i$$,sep:\"\")\n\
\t}\n\
}\n\
if(length(@Y)==dim(@Y)[1]){\n\
\tregress()\n\
}else{\n\
\tmanova()\n\
\tprint(\"NOTE: use secoefs() to get coefficients and standard errors.\")\n\
}", /*MEND*/


/*MNAME*/ "breakif",
/*MTEXT*/ "if($1){\n\
\t@depth$$ <- if($N==1){1}else{$02}\n\
\t@break$$ <- macro(paste(\"delete(@depth$$);break \",@depth$$,\";\"))\n\
\t@break$$()\n\
}", /*MEND*/


/*MNAME*/ "alltrue",
/*MTEXT*/ "# usage: $S(logic1,logic2,...,logicm) computes logic1 && ... && logicm\n\
if($N == 0 || $k > 0)\\\n\
\t{error(\"'$S' must at least 1 argument with no keywords\")}\n\
@args$$ <- $A\n\
for(@i$$,1,$N){\n\
\t@ans$$ <- evaluate(@args$$[@i$$])\n\
\tif (!isscalar(@ans$$,logic:T)){\n\
\t\terror(paste(\"argument\",@i$$,\"to $S is not LOGICAL scalar\"))\n\
\t}\n\
\tif(!@ans$$){break}\n\
}\n\
delete(@i$$,@args$$)\n\
delete(@ans$$,return:T)", /*MEND*/


/*MNAME*/ "anytrue",
/*MTEXT*/ "# usage: $S(logic1,logic2,...,logicm) computes logic1 || ... || logicm\n\
if($N == 0 || $k > 0)\\\n\
\t{error(\"'$S' must at least 1 argument with no keywords\")}\n\
@args$$ <- $A\n\
for(@i$$,1,$N){\n\
\t@ans$$ <- evaluate(@args$$[@i$$])\n\
\tif (!isscalar(@ans$$,logic:T)){\n\
\t\terror(paste(\"argument\",@i$$,\"to $S is not LOGICAL scalar\"))\n\
\t}\n\
\tif(@ans$$){break}\n\
}\n\
delete(@i$$,@args$$)\n\
delete(@ans$$,return:T)", /*MEND*/

/*NOTFORMACSTART*/ /*macros up to NOTFORMACEND are not defined on Macintosh*/
/*  970201 interact:T added to calls to shell() in edit and more*/
#ifdef DEFINEEDIT
#if defined(DJGPP) || defined(wx_msw)


/*MNAME*/ "edit",
/*MTEXT*/ "# $S(realVar), $S(macro), $S(), or $S(0)\n\
@editor$$ <- \"edit\" #change for different default editor\n\
@tmpfile$$ <- \"\\\\tmp\\\\mv\" #change for different temp name start\n\
@delete$$ <- \"erase\"   #change for different delete file command\n\
if(isdefined(EDITOR) && ischar(EDITOR) && isscalar(EDITOR)){\n\
\t@editor$$ <- EDITOR\n\
}\n\
@arg$$ <- if($N == 0){\n\
\tmacro(\"====> Replace this line with lines of your macro <====\")\n\
}else{$01}\n\
if($N > 2 || (!ismacro(@arg$$) && !isreal(@arg$$))){\n\
\terror(\"usage: $S(realVar), $S(macro), $S(), or $S(0)\")\n\
}\n\
@save$$ <- if($N == 2){$02}else{F}\n\
if(!islogic(@save$$) || !isscalar(@save$$)){\n\
\terror(\"2nd argument to $S must be T or F\")\n\
}\n\
@tmpfile$$ <- paste(@tmpfile$$,round(100000*runi(1)),sep:\"\")\n\
if(ismacro(@arg$$)){\n\
\tprint(file:@tmpfile$$,paste(vector(\"macro_to_edit MACRO\",\\\n\
\t\tpaste(@arg$$),\\\n\
\t\t\"%macro_to_edit%\\n\"),multiline:T,linesep:\"\\n\"))\n\
}else{\n\
\t@vector$$ <- isscalar(@arg$$) && @arg$$[1] == 0\n\
\tif(!@vector$$){\n\
\t\tmatwrite(@tmpfile$$,edited:@arg$$,new:T)\n\
\t}\n\
}\n\
shell(paste(@editor$$,@tmpfile$$),interact:T)\n\
if(ismacro(@arg$$)){\n\
\t@arg$$ <- macroread(@tmpfile$$,quiet:T)\n\
}else{\n\
\tif(!@vector$$){\n\
\t\t@arg$$ <- matread(@tmpfile$$,quiet:T)\n\
\t}else{\n\
\t\t@arg$$ <- vecread(@tmpfile$$,quiet:T)\n\
\t}\n\
\tdelete(@vector$$)\n\
}\n\
shell(paste(@delete$$,@tmpfile$$),interact:T)\n\
delete(@delete$$, @tmpfile$$, @editor$$)\n\
if(@save$$){\n\
\t$01 <- @arg$$\n\
\tdelete(@arg$$)\n\
}else{\n\
\t@arg$$\n\
}", /*MEND*/

#elif defined(UNIX) /* DJGPP || wx_msw */


/*MNAME*/ "edit",
/*MTEXT*/ "# $S(realVar), $S(macro), $S(), or $S(0)\n\
@editor$$ <- \"vi\"\n\
@tmpfile$$ <- \"/tmp/macanova.\"\n\
@delete$$ <- \"rm\"\n\
if(isdefined(EDITOR) && ischar(EDITOR) && isscalar(EDITOR)){\n\
\t@editor$$ <- EDITOR\n\
}\n\
@arg$$ <- if($N == 0){\n\
\tmacro(\"====> Replace this line with lines of your macro <====\")\n\
}else{$01}\n\
if($N > 2 || (!ismacro(@arg$$) && !isreal(@arg$$))){\n\
\terror(\"usage: $S(realVar), $S(macro), $S(), or $S(0)\")\n\
}\n\
@save$$ <- if($N == 2){$02}else{F}\n\
if(!islogic(@save$$) || !isscalar(@save$$)){\n\
\terror(\"2nd argument to $S must be T or F\")\n\
}\n\
@tmpfile$$ <- paste(@tmpfile$$,round(100000*runi(1)),sep:\"\")\n\
if(ismacro(@arg$$)){\n\
\tprint(file:@tmpfile$$,paste(vector(\"macro_to_edit MACRO\",\\\n\
\t\tpaste(@arg$$),\\\n\
\t\t\"%macro_to_edit%\\n\"),multiline:T,linesep:\"\\n\"))\n\
}else{\n\
\t@vector$$ <- isscalar(@arg$$) && @arg$$[1] == 0\n\
\tif(!@vector$$){\n\
\t\tmatwrite(@tmpfile$$,edited:@arg$$,new:T)\n\
\t}\n\
}\n\
shell(paste(@editor$$,@tmpfile$$),interact:T)\n\
if(ismacro(@arg$$)){\n\
\t@arg$$ <- macroread(@tmpfile$$,quiet:T)\n\
}else{\n\
\tif(!@vector$$){\n\
\t\t@arg$$ <- matread(@tmpfile$$,quiet:T)\n\
\t}else{\n\
\t\t@arg$$ <- vecread(@tmpfile$$,quiet:T)\n\
\t}\n\
\tdelete(@vector$$)\n\
}\n\
shell(paste(@delete$$,@tmpfile$$),interact:T)\n\
delete(@delete$$, @tmpfile$$, @editor$$)\n\
if(@save$$){\n\
\t$01 <- @arg$$\n\
\tdelete(@arg$$)\n\
}else{\n\
\t@arg$$\n\
}", /*MEND*/

#endif /*DJGPP || wx_msw*/
#endif /*DEFINEEDIT*/

#ifdef DEFINEMORE
#ifdef UNIX

/*MNAME*/ "more",
/*MTEXT*/ "# $S(var) or $S(macro)\n\
if($v != 1){\n\
\terror(\"usage: $S(x [,keywords])\")\n\
}\n\
@x$$ <- $V\n\
if(!ismacro(@x$$) && !isreal(@x$$) && !ischar(@x$$) && !islogic(@x$$)){\n\
\terror(\"$V is not a macro or a REAL, LOGICAL, or CHARACTER variable\")\n\
}\n\
@tmpfile$$ <- paste(\"/tmp/macanova.\",round(100000*runi(1)),sep:\"\")\n\
@pager$$ <- if(isdefined(PAGER) && ischar(PAGER) && isscalar(PAGER)){\n\
\tPAGER\n\
}else{\n\
\t\"more\"\n\
}\n\
if(ismacro(@x$$)){\n\
\tmacrowrite(@tmpfile$$,name:\"$V\",@x$$,new:T)\n\
}elseif($k == 0){\n\
\tprint(file:@tmpfile$$,name:\"$V\",@x$$,new:T)\n\
}else{\n\
\tprint(file:@tmpfile$$,name:\"$V\",@x$$,$K,new:T)\n\
}\n\
shell(paste(@pager$$,@tmpfile$$, \"; rm\", @tmpfile$$),interact:T)\n\
delete(@x$$, @tmpfile$$, @pager$$)\n\
NULL", /*MEND*/
#endif /*UNIX*/

#endif /*DEFINEMORE*/
/*NOTFORMACEND*/


/*MNAME*/ "console",
/*MTEXT*/ "# usage: y <- $S() reads from keyboard or following lines in batch file\n\
if($k==0){vecread(CONSOLE)}else{vecread(CONSOLE,$K)}", /*MEND*/


/*MNAME*/ "fromclip",
/*MTEXT*/ "# usage: x <- $S([char:T])  or  x <- $S(ncols[,char:T])\n\
if($v==0){\n\
\tif ($k > 0){vecread(string:CLIPBOARD,$K)}else{\n\
\t\tvecread(string:CLIPBOARD)}\n\
}else{\n\
\t@n$$ <- $01\n\
\tif(if(!isscalar(@n$$)||!isreal(@n$$)){T}\\\n\
\t\t\telseif(@n$$<0||@n$$!=floor(@n$$)){T}else{F}){\n\
\t\terror(\"$1 not positive integer\")\n\
\t}\n\
\tif ($k > 0){matrix(vecread(string:CLIPBOARD,$K),@n$$)}else{\n\
\t\tmatrix(vecread(string:CLIPBOARD),@n$$)}'\n\
}", /*MEND*/


/*MNAME*/ "toclip",
/*MTEXT*/ "#usage: $S(x)  or (e.g.)  $S(x,format:\"6.3f\",missing:\"NA\",linesep:\";\")\n\
if($v != 1){error(\"usage is $S(x [,keyword phrases])\")}\n\
CLIPBOARD <- if($k == 0){$01}else{\n\
\tpaste($01,multiline:T,sep:\"\\t\",$K,\\\n\
\t\tformat:\".17g\",missing:\"?\",linesep:\"\\n\")\n\
};;", /*MEND*/


/*MNAME*/ "enter",
/*MTEXT*/ "# usage: x <- $S(3.1 2.75 3.12 4.5) (no commas needed)\n\
vecread(string:\"$0\")", /*MEND*/


/*MNAME*/ "enterchars",
/*MTEXT*/ "# usage: x <- $S(weight age) (use no quotes, commas)\n\
vecread(string:\"$0\",char:T)", /*MEND*/


/*MNAME*/ "model",
/*MTEXT*/ "#$S(y=a+b), e.g., sets STRMODEL to \"y=a+b\"\n\
STRMODEL <- \"$1\";;", /*MEND*/


/*MNAME*/ "haslabels",
/*MTEXT*/ "!isnull(getlabels($1,1,silent:T))", /*MEND*/


/*MNAME*/ "hasnotes",
/*MTEXT*/ "!isnull(getnotes($1,silent:T))", /*MEND*/

/* 981121 added keyword phrase nofile:T to calls to macroread()*/

/*MNAME*/ "getmacros",
/*MTEXT*/ "#$S(macro1,macro2,...[,quiet:T]) retrieves macros from MACROFILES\n\
if($v==0){error(\"must specify at least one macro name\")}\n\
if(!isdefined(MACROFILES) && (!isdefined(MACROFILE))){\n\
\terror(\"neither MACROFILES nor MACROFILE exists\")\n\
}\n\
@files$$ <- if(isdefined(MACROFILES)){MACROFILES}else{MACROFILE}\n\
@nf$$ <- length(@files$$)\n\
@args$$ <- $A\n\
for(@i$$,run($v)){\n\
\t@macname$$ <- @args$$[@i$$]\n\
\tfor (@j$$,run(@nf$$)){\n\
\t\t<<@macname$$>> <- if($k==0){\n\
\t\t\tmacroread(@files$$[@j$$],@macname$$,silent:F,nofileok:T,notfoundok:T)\n\
\t\t}else{\n\
\t\t\tmacroread(@files$$[@j$$],@macname$$,silent:F,nofileok:T,notfoundok:T,$K)\n\
\t\t}\n\
\t\tif (!isnull(<<@macname$$>>)){break}\n\
\t}\n\
\tif (isnull(<<@macname$$>>)){\n\
\t\tprint(paste(\"WARNING: macro\",@macname$$,\"not found\"))\n\
\t\tdelete(<<@macname$$>>)\n\
\t}\n\
}\n\
delete(@macname$$,@files$$,@i$$,@j$$,@nf$$,@args$$)", /*MEND*/


/*MNAME*/ "addmacrofile",
/*MTEXT*/ "# $S(fileName) or $S(fileName,atEnd), atEnd T or F\n\
@newfiles$$ <- $1\n\
@atend$$ <- if($v > 1){$02}else{F}\n\
if (!ischar(@newfiles$$) || !isvector(@newfiles$$)){\n\
\terror(\"$1 not quoted string or CHARACTER scalar or vector\")\n\
}\n\
if (!islogic(@atend$$) || !isscalar(@atend$$)){\n\
\terror(\"$1 not T or F\")\n\
}\n\
MACROFILES <- if (!isdefined(MACROFILES)){\n\
\t@newfiles$$\n\
}elseif(@atend$$){\n\
\tvector(MACROFILES,@newfiles$$)\n\
}else{\n\
\tvector(@newfiles$$,MACROFILES)\n\
}\n\
delete(@newfiles$$,@atend$$)", /*MEND*/


/*MNAME*/ "adddatapath",
/*MTEXT*/ "# $S(fileName) or $S(fileName,atEnd), atEnd T or F\n\
@newpaths$$ <- $1\n\
@atend$$ <- if($v > 1){$2}else{F}\n\
if (!ischar(@newpaths$$) || !isvector(@newpaths$$)){\n\
\terror(\"$1 not quoted string or CHARACTER scalar or vector\")\n\
}\n\
if (!islogic(@atend$$) || !isscalar(@atend$$)){\n\
\terror(\"$1 not T or F\")\n\
}\n\
DATAPATHS <- if (!isdefined(DATAPATHS)){\n\
\t@newpaths$$\n\
}elseif(@atend$$){\n\
\tvector(DATAPATHS,@newpaths$$)\n\
}else{\n\
\tvector(@newpaths$$,DATAPATHS)\n\
}\n\
delete(@newpaths$$,@atend$$)", /*MEND*/


/*MNAME*/ "getdata",
/*MTEXT*/ "# y <- $S(datasetName [,quiet:T]) retrieves data set from DATAFILE\n\
if($v>1){\n\
\terror(\"too many arguments for $S\")\n\
}\n\
if($v==1){\n\
\tif($k==0){matread(DATAFILE,\"$1\")}else{matread(DATAFILE,\"$1\",$K)}\n\
}else{\n\
\tif($k==0){matread(DATAFILE)}else{matread(DATAFILE,$K)}\n\
}", /*MEND*/

/*NOTFORMACSTART*/ /*macros up to NOTFORMACEND are not defined on Macintosh*/
#ifdef TEK
/* on some terminals, in particular Versaterm, puts in Tektronix 40xx mode */

/*MNAME*/ "tek",
/*MTEXT*/ "putascii(vector(29,27,56))", /*MEND*/

/* puts xterm in Tek4014 mode */

/*MNAME*/ "tekx",
/*MTEXT*/ "putascii(vector(27,91,63,51,56,104,27,56))", /*MEND*/

/* on some terminals, in particular Versaterm, puts in VT100 mode */

/*MNAME*/ "vt",
/*MTEXT*/ "putascii(vector(27,50))", /*MEND*/

/* puts xterm in vt100 mode */

/*MNAME*/ "vtx",
/*MTEXT*/ "putascii(vector(27,3))", /*MEND*/

#endif /*TEK*/
/*NOTFORMACEND*/
/*ENDMACROS*/
	(char *) 0, (unsigned char *) 0
