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
   This file defines a number of macros any of which might otherwise
   defined in a Makefile, being specific to the compilation date and the
   installation configuration of MacAnova and its auxilliary files.  Either
   this file or Makefile can be customized for a particular installation.

    VERSION              Version number (e.g., 4.03)
    MAKE_DATE            Date of compilation or make, to be kept up to date
    PROCESSOR_COMPILER   Name with system type and compiler abbreviation
    TODAY                Combines two preceding for startup message
   
   Particularly on Unix the following paths should be customized
    DATAPATHNAME         Path to default data file
    MACROPATHNAME        Path to default macro file
    HELPPATHNAME         Path to standard help file

   For most installations, these defaults should be fine
    DATAFILENAME         Name of default data file
    MACROFILENAME        Name of default macro file
    HELPNAME             Name of standard help file (*not* HELPFILENAME)

  960314  Bumped version number to 4.00
  960505  Modified definitions of DATAFILENAME, MACROFILENAME, etc.
  960507  Bumped version number to 4.01
  960612  Bumped version number to 4.02
  961226  Bumped version number to 4.03
  970313  Bumped version number to 4.04
  970619  Bumped version number to 4.05
  971112  Bumped version number to 4.06
  980716  Bumped version number to 4.07
  990324  Added define ARIMAMACROFILENAME
*/

#ifndef _VERSIONH_

#define _VERSIONH_

/*
   If neither MAKE_DATE or TODAY are defined in Makefile, the following
   date should be changed whenever MacAnova is re-made
*/

#ifndef MAKE_DATE
#define MAKE_DATE    "03/25/99" /* Day of Make */
#endif /*MAKE_DATE*/

#ifndef VERSION
#define VERSION "4.07"     		/* version of MacAnova */
#endif /*VERSION*/

#ifndef PROCESSOR_COMPILER
#ifdef MW_CW
#ifndef powerc
#if (__option(code68881)) /*#ifdef  __MC68881__ did not work, being defined when it shouldn't be*/
#define NEEDS68881
#define PROCESSOR_COMPILER "Macintosh with Co-Processor [CW]"
#else /*__option(code68881)*/
#define PROCESSOR_COMPILER "Macintosh without Co-Processor [CW]"
#endif /*__option(code68881)*/
#else /*powerc*/
#define PROCESSOR_COMPILER "Power Macintosh [CW]"
#endif /*powerc*/
#endif /*MP_CW*/

#ifdef MPW
#ifdef mc68881
#define NEEDS68881
#define PROCESSOR_COMPILER "Macintosh with Co-Processor [MPWC]"
#else /*mc68881*/
#define PROCESSOR_COMPILER "Macintosh without Co-Processor [MPWC]"
#endif /*mc68881*/
#endif /*MPW*/

#ifdef BCPP
#define PROCESSOR_COMPILER "DOS [BCPP4]"
#endif /*BCPP*/

#ifdef DJGPP
#define PROCESSOR_COMPILER "DOS [DJGPP]"
#endif /*DJGPP*/

#ifdef WXWINMSW
#define PROCESSOR_COMPILER "Win32s [BCPP5.0]"
#endif /*WXWINMSW*/

#ifdef WXWINMOTIF
#define PROCESSOR_COMPILER "Motif [gcc]"
#endif /*WXWINMOTIF*/

#endif /*PROCESSOR_COMPILER*/

#ifndef TODAY /* Caution: This doesn't work with MPW C in Version 3.2.3 MPW */
#ifndef VMS	/* I. Pardowitz, TODAY-generation here not working under VMS */
#define	TODAY "of " MAKE_DATE " (" PROCESSOR_COMPILER ")"
#else /*VMS*/
#define TODAY MAKE_DATE
#endif /*VMS */
#endif /*TODAY*/

/*
   On Unix and perhaps other platforms, DATAPATHNAME should probably be
   defined in Makefile, or an additional entry can be put here
*/

/* 960503 changed value from ":" or "./" or ".\\" */
#ifndef DATAPATHNAME
#define DATAPATHNAME ""
#endif /*DATAPATHNAME*/

#ifndef DATAFILENAME
#if defined(MPW) || defined(MW_CW)
#define DATAFILENAME "MacAnova.dat"
#elif defined(BCPP) || defined(DJGPP) || defined(WXWINMSW)
#define DATAFILENAME "MACANOVA.DAT"
#else /*MPW || MW_CW*/
#define DATAFILENAME "macanova.dat"
#endif /*MPW || MW_CW*/
#endif /*DATAFILENAME*/

#ifndef MACROPATHNAME
#define MACROPATHNAME DATAPATHNAME
#endif /*MACROPATHNAME*/

#ifndef MACROFILENAME
#if defined(MPW) || defined(MW_CW)
#define MACROFILENAME "MacAnova.mac"
#elif defined(BCPP) || defined(DJGPP) || defined(WXWINMSW)
#define MACROFILENAME "MACANOVA.MAC"
#else /*MPW || MW_CW*/
#define MACROFILENAME "macanova.mac"
#endif /*MPW || MW_CW*/
#endif /*MACROFILENAME*/

#ifndef TSMACROFILENAME
#if defined(MPW) || defined(MW_CW)
#define TSMACROFILENAME "Tser.mac"
#elif defined(BCPP) || defined(DJGPP)
#define TSMACROFILENAME "TSER.MAC"
#else /*MPW || MW_CW*/
#define TSMACROFILENAME "tser.mac"
#endif /*MPW || MW_CW*/
#endif /*TSMACROFILENAME*/

#ifndef DESIGNMACROFILENAME
#if defined(MPW) || defined(MW_CW)
#define DESIGNMACROFILENAME "Design.mac"
#elif defined(BCPP) || defined(DJGPP)
#define DESIGNMACROFILENAME "DESIGN.MAC"
#else /*MPW || MW_CW*/
#define DESIGNMACROFILENAME "design.mac"
#endif /*MPW || MW_CW*/
#endif /*DESIGNMACROFILENAME*/

#ifndef ARIMAMACROFILENAME
#if defined(MPW) || defined(MW_CW)
#define ARIMAMACROFILENAME "Arima.mac"
#elif defined(BCPP) || defined(DJGPP)
#define ARIMAMACROFILENAME "ARIMA.MAC"
#else /*MPW || MW_CW*/
#define ARIMAMACROFILENAME "arima.mac"
#endif /*MPW || MW_CW*/
#endif /*ARIMAMACROFILENAME*/

/*
   On Unix and perhaps other platforms, HELPPATHNAME should probably be
   defined in Makefile, or an additional entry can be put here
*/

/* changed definition to "" instead of ":", "./" or ".\\" */
#ifndef HELPPATHNAME
#define HELPPATHNAME ""
#endif /*HELPPATHNAME*/

#ifndef HELPNAME
#if defined(MPW) || defined(MW_CW)
#define HELPNAME "MacAnova.hlp"
#elif defined(BCPP) || defined(DJGPP) || defined(WXWINMSW)
#define HELPNAME "MACANOVA.HLP"
#else /*MPW || MW_CW*/
#define HELPNAME "macanova.hlp"
#endif /*MPW || MW_CW*/
#endif /*HELPNAME*/

#endif /* _VERSIONH_ */
