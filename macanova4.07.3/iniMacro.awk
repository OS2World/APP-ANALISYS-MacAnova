# usage:  awk -f iniMacro.awk iniMacro.h
# This produces file macIniMacro.r, used  used in making MacAnova under MPW
# on the Macintosh.  Rez creates resources form macIniMacro.r which are
# used to initialize pre-defined macros.
#
# 980710 now copies update history
BEGIN	{
			rFile = "macIniMacro.r"
			ndelete = 0
			attributes = ", nonpurgeable"
			copyright = 0
			found = 0
			intext = 0
			history = 0
		}
/^\*\(C\)\*/	{
				if(!copyright) 	print "/*" > rFile
				copyright = 1
			}
/INIMACROHISTORY/	{
						print "\n/*" > rFile
						history = 1
						next
					}
history == 1	{
					print > rFile
					if ($1 == "*/") {history = 0}
				}
copyright == 1	{
					print > rFile
					if($0 ~ /^[ 	]*\*\//) copyright = 0
				}
/^#define.*IDBASE/		{code = $3;next}
/^#define.*MACROTYPE/	{type = $3;next}
/^#define.*ENDNAME/		{lastname = substr($3,2,length($3)-2);next}
/^#define.*NAMEPREFIX/	{
	prefix = substr($3,2,length($3)-2)
	code1 = (code - code%100)/100
	print "\n/* Resource version of macros in iniMacro.c */" > rFile
	print "" > rFile
	print "/*" > rFile
	print "Each resource must be of the form\n" > rFile
	printf "resource %s (%dxx, \"%sMacroname\"%s)\n", type, code1, prefix, attributes > rFile
	print "{" > rFile
	print "\t\"first line of macro text\\n\"" > rFile
	print "\t\"second line of macro text\\n\"" > rFile
	print "\t. . . . ." > rFile
	print "\t\"last line of macro text\\n\"" > rFile
	print "};" > rFile
	print " */" > rFile
	print "" > rFile
	print "" > rFile
	next
}
$1 ~ /\/\*STARTMACROS\*\//	{
	found = 1
	next
}
/^#/	{next}
found == 0	{next}
$1 ~ /\/\*NOTFORMACSTART\*\//	{skip=1;next}
$1 ~ /\/\*NOTFORMACEND\*\//	{skip=0;next}
skip != 0	{next}
$1 ~ /\/\*MNAME\*\//	{
	code++
	name = substr($2,2,length($2)-3)
	printf "resource %s (%d, \"%s%s\"%s)", type, code, prefix, name, attributes > rFile
	$1 = ""; $2 = ""
	print > rFile
	print "{" > rFile
}
$1 ~ /\/\*MTEXT\*\//	{
	out = substr($0,11)
	if($NF !~ /\/\*MEND\*\//){
		if (out ~ /^.*\\$/) out = substr(out,1,length(out)-1)"\""
		intext = 1
		printf "\t%s\n", out > rFile
	}
	else {
		out = substr(out,1,length(out)-10)
		printf "\t%s\n", out > rFile
		print "};" > rFile
		print "" > rFile
	}
	next
}
$NF ~ /\/\*MEND\*\//	{
	out = $0
	out = "\""substr(out,1,length(out)-10)
	printf "\t%s\n", out > rFile
	print "};" > rFile
	print "" > rFile
	intext = 0
	next
}
intext != 0	{
	out = "\""substr($0,1,length($0)-1)"\""
	printf "\t%s\n", out > rFile
	next
}	
$1 ~ /\/\*ENDMACROS\*\/$/	{
	code++
	printf "resource %s (%d, \"%s%s\"%s)\n{\n", type, code, prefix, lastname, attributes > rFile
	print "\t\"\"" > rFile
	print "};" > rFile
	exit
}

END	{printf "%s is the new file for Macintosh\n", rFile}
	
	

