/*
	File:		utfaddlod.c

	Contains:	Takes one file and appends the lods of another onto it

	Written by:	Todd Allendorf 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		<4+>	 7/16/95	TMA		Remove unused variables.
		 <4>	 7/15/95	TMA		Autodocs updated.
		 <3>	  6/9/95	TMA		Fix size bugs.
		 <2>	 5/16/95	TMA		Added autodocs.
	To Do:
*/


/**
|||	AUTODOC -public -class tools -group m2tx -name utfaddlod
|||	Appends the LODs of one UTF file onto another.
|||	
|||	  Synopsis
|||	
|||	    utfaddlod <input file> <append file> <output file>
|||	
|||	  Description
|||	
|||	    This tool appends the levels of detail of one UTF file onto the end of  
|||	    another, producing a new UTF file.  
|||	
|||	  Caveats
|||	
|||	    The files must be the same type (color depth, alpha depth, ssb component,
|||	    compression, pip, etc.) as it makes no attempt to resolve any differences.
|||	
|||	  Arguments
|||	
|||	    <input file>
|||	        The input UTF texture.
|||	    <append file>
|||	        The UTF texture with LODs to append onto the end of the input file.
|||	    <output file>
|||	        The resulting UTF texture.
|||	
|||	  Examples
|||	
|||	    utfaddlod check.64x64.utf check.32x32.utf check.2lod.utf
|||	
|||	    Assume check.64x64.utf has a single level of detail as does check.32x32.utf
|||	    The resulting file is one with two levels of detail.  You can append as 
|||	    many levels of detail (up to the UTF limit of 11) as long as the last 
|||	    level of detail of the first file is twice the X and Y dimensions of the
|||	    first level of detail of the appended file.
|||	
|||	  See Also
|||	
|||	    utfmakelod, utffit
**/


#include "M2TXlib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   Append one UTF's Levels of Detail after another's.\n");
}

int main( int argc, char *argv[] )
{
	M2TX tex, newTex, appendTex;
	M2TXTex lodPtr;
	M2TXPIP *oldPIP;
	M2TXDCI *oldDCI;
	M2TXHeader *header, *newHeader, *appendHeader;
	char fileIn[256];
	char fileAppend[256];
	char fileOut[256];
	uint16 xSize, ySize, aXSize, aYSize, apXSize, apYSize;
	uint16 lodXSize, lodYSize;
	uint32 length;
	FILE *fPtr;
	uint8 numLOD, nALOD, i;
	M2Err err;
		
#ifdef M2STANDALONE
	printf("Enter: <FileIn> <AppendFile> <FileOut>\n");
	printf("Example: dumb.1_4.utf dumb.5_8.utf dumb.1_8.utf\n");
	fscanf(stdin,"%s %s %s",fileIn, fileAppend, fileOut);

#else
    /* Check for command line options. */
	if (argc != 4)
	{
	  fprintf(stderr,"Usage: %s <Input File> <Append File> <Output File>\n",argv[0]);
	  print_description();
	  return(-1);	
	}	
	else
	{
        strcpy(fileIn, argv[1]);
        strcpy(fileAppend, argv[2]);
        strcpy(fileOut, argv[3]);
	}
#endif

	fPtr = fopen(fileIn, "r");
	if (fPtr == NULL)
	{
		fprintf(stderr,"ERROR:Can't open file \"%s\" \n",fileIn);
		return(-1);
	}
	else 
		fclose(fPtr);
	fPtr = fopen(fileAppend, "r");
	if (fPtr == NULL)
	{
		fprintf(stderr,"ERROR:Can't open file \"%s\" \n",fileAppend);
		return(-1);
	}
	else 
		fclose(fPtr);

	M2TX_Init(&tex);						/* Initialize a texture */
	M2TX_Init(&newTex);						/* Initialize a texture */
	M2TX_Init(&appendTex);						/* Initialize a texture */
	err = M2TX_ReadFile(fileIn,&tex);
	if (err != M2E_NoErr)
	  {
	    fprintf(stderr,"ERROR:Bad File \"%s\"\n",fileIn);
	    return(-1);
	  }
	err = M2TX_ReadFile(fileAppend,&appendTex);
	if (err != M2E_NoErr)
	  {
	    fprintf(stderr,"ERROR:Bad File \"%s\"\n",fileAppend);
	    return(-1);
	  }
	M2TX_GetHeader(&tex,&header);
	M2TX_GetHeader(&newTex,&newHeader);
	M2TX_GetHeader(&appendTex,&appendHeader);

	M2TX_Copy(&newTex, &tex);
	/*	M2TX_SetHeader(&newTex, header); */

	M2TX_GetPIP(&tex,&oldPIP);
	M2TX_SetPIP(&newTex, oldPIP);
	M2TX_GetDCI(&tex,&oldDCI);
	M2TX_SetDCI(&newTex, oldDCI);

	M2TXHeader_GetNumLOD(header, &numLOD);
	err = M2TXHeader_GetMinXSize(header, &xSize);
	err = M2TXHeader_GetMinYSize(header, &ySize);
	
	lodXSize = xSize;  /* Smallest LOD size */
	lodYSize = ySize;
	
	M2TXHeader_GetNumLOD(appendHeader, &nALOD);
	err = M2TXHeader_GetMinXSize(appendHeader, &aXSize);
	err = M2TXHeader_GetMinYSize(appendHeader, &aYSize);

	apXSize = aXSize <<nALOD;
	apYSize = aYSize <<nALOD;
	
	if ((lodXSize != apXSize) || (lodYSize != apYSize))
	{
		fprintf(stderr,"ERROR:Can't append, incorrect image size\n");
		fprintf(stderr,"Expected XSize=%d YSize=%d, got XSize=%d YSize=%d\n",
				xSize>>1, ySize>>1, apXSize<<(nALOD-1), apYSize<<(nALOD-1));
		return(-1);
	}
	M2TXHeader_SetNumLOD(newHeader, nALOD+numLOD);
	M2TXHeader_SetMinXSize(newHeader, aXSize);
	M2TXHeader_SetMinYSize(newHeader, aYSize);

	for (i=0; i<nALOD; i++)
	{
	  M2TXHeader_GetLODPtr(appendHeader,i,&length, &lodPtr);
	  M2TXHeader_SetLODPtr(newHeader,i+numLOD,length, lodPtr);
	}
		
	M2TX_WriteFile(fileOut,&newTex);    			/* Write it to disk */
	M2TXHeader_FreeLODPtrs(newHeader);

	return(0);
}
