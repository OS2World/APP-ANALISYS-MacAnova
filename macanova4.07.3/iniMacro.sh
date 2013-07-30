#! /bin/sh

# sh script to creat iniMacro.h from macro file iniMacro.mac

# So that iniMacro.h can be used as input to iniMacro.awk in creating
# macIniMacro.r, iniMacro.mac must include certain comments and
# C preprocessor directives.

# The macros themselves are in the standard form readable by macroread.

# Everything through a line containing Start_Of_Macros is copied
# unchanged.

# Comment lines (and any other lines starting with ')') are deleted.

# Each macro name is enclosed in quotes, preceded by /*MNAME*/ and
# followed by a comma.

# The text of each macro is preceded by /*MTEXT*/, enclosed in quotes,
# quotes and backslashes are escaped, tabs are changed to '\t', and
# lines are ended with '\n\'.  The text is followed by a comma and /*mend*/

# In the macros in iniMacro.mac themselves, any tabs in quoted strings
# should be designated by '\t' and there should be no lines starting
# with ')' except for comment lines between a name line and macro text.

# Version 990308

# usage: iniMacro.sh iniMacro.mac [out.h]  (default for out.h is iniMacro.h)
 
OUTFILE=$2
if [ "$OUTFILE" = "" ]
then
	OUTFILE=iniMacro.h
fi


sed -e '1,/Start_Of_Macros/b'\
	-e 's/\\/\\\\/g' \
	-e 's/"/\\"/g' \
	-e 's/	/\\t/g' \
	-e '/^)/d' $1 |\
awk 'BEGIN	{
				outfile ="'"$OUTFILE"'"
				startMacro = "/*STARTMACROS*/"
				inMacro = 0
			}
inMacro == 0 && /^[a-zA-Z][a-zA-Z0-0_]* *[mM][aA][cC][rR][oO] */	{
			if (startMacro != ""){
				print startMacro > outfile
				startMacro = ""
			}
			inMacro = 1
			name = $1
			nLines = 0
			printf("\n/*MNAME*/ \"%s\",\n/*MTEXT*/ \"", name) > outfile
			next
		}
inMacro == 0	{print > outfile; next}
/^%[a-zA-Z][a-zA-Z0-9]*%/	{
								inMacro = 0
								printf("\", /*MEND*/\n") > outfile
								next
							}
inMacro != 0	{
					if (nLines > 0)printf("\\n\\\n") > outfile
					nLines++
					printf("%s",$0) > outfile
					next
				}
END	{
		print("/*ENDMACROS*/")  > outfile
		print("\t(char *) 0, (unsigned char *) 0") > outfile
		printf ("New version of %s created\n", outfile)
	}'

