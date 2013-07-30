/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 4.00 or later
*(C)*
*(C)* Copyright (c) 1996 by Gary Oehlert and Christopher Bingham
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
   Program to change all the occurrencs of
      printf(fmt, ...)
   in mainpars.c to
     (sprintf(OUTSTR, fmt, ...), myprint(OUTSTR))
*/

#include <stdio.h>
#include <ctype.h>

#define SHIFT (++argv, --argc)

void doit(FILE * fp)
{
	int       c, nparen = 0;
	int       inComment = 0, inQuotes = 0, changing = 0;
	char      lastc = '\0';
	char      buffer[1000], *pc, c1;
	
	while (fgets(buffer, 1000, fp) != (char *) 0)
	{
		lastc = '\0';
		for (pc = buffer; *pc != '\0'; pc++)
		{
			c = *pc;
			
			if (inQuotes && c == '"' && lastc != '\\')
			{
				inQuotes = 0;
				lastc = c;
			}
			else if (inComment && c == '/' && lastc == '*')
			{
				inComment = 0;
				lastc = '\0';
			}
			else if (!inComment && c == '*' && lastc == '/')
			{
				inComment = 1;
				lastc = '\0';
			}
			else if (!inQuotes && c == '"')
			{
				inQuotes = 1;
				lastc = c;
			}
			else if (!changing && c == 'p' && strncmp(pc+1,"rintf", 5) == 0 &&
					 !isalpha(pc[-1]) && !isdigit(pc[-1]) && pc[-1] != '_' &&
					 (c1 = pc[6], !isalpha(c1)) && !isdigit(c1) && c1 != '_')
			{
				int       j;
				
				pc += 6;
				changing = 1;
				nparen = (c1 == '(') ? 1 : 0;
				printf("(sprintf(OUTSTR,");
				c = lastc = '\0';
			}
			else if (changing && (c == '(' || c == ')'))
			{
				nparen += (c == '(') ? 1 : -1;
				if (nparen == 0)
				{
					printf("), myprint(OUTSTR))");
					changing = 0;
					c = lastc = '\0';
				}
				else
				{
					lastc = c;
				}
			}
			
			if (c != '\0')
			{
				putchar(c);
			}
		} /*for (pc = buffer; *pc != '\0' && *pc != '\n'; pc++)*/
	} /*while (fgets(buffer, 1000, fp) != (char *) 0)*/
	
}

int main(int argc, char **argv)
{
	FILE     *fp = stdin;
	char     *fileName = "standard input";
	
	if (SHIFT > 0)
	{
		fileName = *argv;
		fp = fopen(fileName, "r");
		if (fp == (FILE *) 0)
		{
			fprintf(stderr, "cannot open file %s for reading\n",
					fileName);
			return (1);
		}
	} /*if (SHIFT > 0)*/
	doit(fp);
	if (fp != stdin)
	{
		fclose(fp);
	}
} /*main()*/
