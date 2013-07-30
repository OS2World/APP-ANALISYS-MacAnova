/*
*(C)* The file is part of the source distribution of MacAnova
*(C)* version 4.06 or later
*(C)*
*(C)* Copyright (c) 1997 by Gary Oehlert and Christopher Bingham
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
  971126 new file
*/

#ifndef LABELSH__

#define LABELSH__

/*
  Prototypes (also in matProto.h)
*/

void getAllLabels(Symbolhandle /*arg*/, char * /*labels*/ [],
			   long /*lengths*/ [], long /*widths*/ []);
char * getOneLabel(Symbolhandle /*symh*/, long /*dimension*/, long /*index*/);
void getMatLabels(Symbolhandle /*arg*/, char * /*rowcol*/ [2],
				  long /*lengths*/ [2]);
char **createLabels(long /*ndims*/, long /*lengths*/ []);
void buildLabels(char ** /*labelsH*/, char * /*labels*/ [], long /*dims*/ [],
				 long /*ndims*/);
char ** growLabels(char ** /*oldlabels*/, long /*length*/);
long expandLabels(char * /*root*/, long /*n*/, char * /*labels*/);
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
long prepPrintLabels(Symbolhandle /*symh*/,
					    char * /*dimLabels*/ [], long /*dimLabelsWidth*/ []);
void putColumnLabels(FILE * /*fp*/, long /*lastdim*/, char * /*labels*/,
					 long /*labelWidth*/, long /*fieldWidth*/,
					 long /*nperLine*/, long /*type*/);
void putRowLabels(FILE * /*fp*/, long /*ndims*/, long /*coord*/ [],
				  long /*labelWidth*/, char * /*labels*/ [],
				  long * /*labelsWidth*/, long /*charData*/);

enum expandCodes
{
	dontExpand = 0,
	doExpand,
	padExpand
};

#endif /*LABELSH__*/
