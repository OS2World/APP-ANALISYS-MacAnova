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
#pragma segment Utils
#endif /*MPW||MW_CW*/
#endif /*SEGMENTED*/


#include "globals.h"
#include "macIface.h" /* defines SFReply Reply as global*/

/*
   960502 added parameters fileName and vRefNum
          Moved check for fileName[0] == '\0' here
		  readWrite should be either READIT or WRITEIT

   960503 changed name of macOpen() to macFindFile(); changed name
          of file to macFindFile.c
   960530 If actual file name is supplied and volume != (Integer *) 0,
          and *volume is set to Id of current directory.
          
   960820 Fixed minor bug; with fileName[0] != '\0', *volume was ignored

   990325 On reading on Macintosh, when ntypes is 0, 'ttro' is an additional
          default type
*/
static char       FileName[64]; /* macFindFile() returns address of FileName */

char *macFindFile(char *fileName, Str255 message, Str255 defaultname,
			  Integer readWrite, Integer ntypes, OSType * types, 
			  Integer * volume) 
{
	SFTypeList      myTypes;
	Point           p;
	Integer         numTypes;
	Integer         vRefNum = (volume != (Integer *) 0) ? *volume : 0;
	LongInt         dirID;
	int             i;
	WHERE("macFindFile");

	if (fileName[0] == '\0')
	{
		/* Use standard file dialog box */
		SetPt(&p, 82, 90);
	
		DeactivateWindow();		/*unhighlight front window*/

		if (readWrite == WRITEIT)
		{						/* write */
			DialogEdit = true;
			NDialogButtons = 2;
			ButtonChars[0] = 's';
			ButtonChars[1] = 'n';
			SFPPutFile(p, message, defaultname, NullDialogHookPtr, &Reply,
					   putDlgID, MyDialogFilterPtr);
		} /*if (readWrite == WRITEIT)*/
		else
		{/* read */
			if(ntypes == 0)
			{
				numTypes = 0;
				myTypes[numTypes++] = 'TEXT';
				myTypes[numTypes++] = 'ttro';
				if(types != (OSType *) 0)
				{
					myTypes[numTypes++] = types[0];
				}
			}
			else
			{
				numTypes = (ntypes <= 4) ? ntypes : 4; /*play it safe*/
				for(i = 0;i < numTypes; i++)
				{
					myTypes[i] = types[i];
				}
			}
			DialogEdit = false;
			NDialogButtons = 2;
			ButtonChars[0] = 'o'; /* Open */
			ButtonChars[1] = 'n'; /* Cancel */
			SFPGetFile(p, message, NullFileFilterPtr,	
					   numTypes, myTypes, NullDialogHookPtr, &Reply,
					   getDlgID, MyDialogFilterPtr);
		} /*if (readWrite == WRITEIT){}else{}*/
		macUpdate((WindowPtr) 0);
	
		if (!Reply.good)
		{ /* cancelled */
			return (0);
		}
		vRefNum = Reply.vRefNum;
		if (volume != (Integer *) 0)
		{
			*volume = vRefNum;
		}
	
		PtoCstr((unsigned char *) Reply.fName);
		strcpy(FileName, (char *) Reply.fName);
		CtoPstr((char *) Reply.fName);
		fileName = FileName;
	} /*if (fileName[0] == '\0')*/
	else if (volume != (Integer *) 0 && vRefNum == 0 &&
			 HGetVol((StringPtr) 0, &vRefNum, &dirID) == noErr)
	{
		*volume = vRefNum;
	}
 
	if (vRefNum != 0)
	{
		SetVol(0L, vRefNum);
	}
	return (fileName);

} /*macFindFile()*/

void macSetInfo(Integer vRefNum,char * fName,OSType type,OSType creator)
{
	FInfo       finder_stuff;
	Str63       fileName;

	strncpy((char *) fileName, fName, 63);
	fileName[63] = '\0';
	CtoPstr((char *) fileName);

	GetFInfo(fileName, vRefNum, &finder_stuff);
	finder_stuff.fdType = type;
	finder_stuff.fdCreator = creator;
	SetFInfo(fileName, vRefNum, &finder_stuff);
} /*macSetInfo()*/
