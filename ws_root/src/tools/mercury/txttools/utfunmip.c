/*
	File:		utfunmip.c

	Contains:	Extracts the levels of detail from a concatenated image.

	Written by:	Todd Allendorf 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <3>	 7/15/95	TMA		Removed debugging information.
		 <2>	 5/31/95	TMA		Removed unused variables.
		 <1>	 5/16/95	TMA		first checked in
	To Do:
*/

/**
|||	AUTODOC -public -class tools -group m2tx -name utfunmip
|||	Extract levels of detail from a concatenated mip-map.
|||	
|||	  Synopsis
|||	
|||	    utfunmip <input file> <# of levels of detail> <output file>
|||	
|||	  Description
|||	
|||	    This tool extracts the individual levels of detail from a concatenated
|||	    mip-map file (generated with utfmipcat) and outputs a proper UTF file 
|||	    with the appropriate levels of details.  The output image will be half
|||	    the height of the input image (which should have only one level of detail).
|||	
|||	  Arguments
|||	
|||	    <input file>
|||	        The input UTF texture.
|||	    <# of levels of detail>
|||	        The number of levels of detail that are contained in the input image.
|||	    <output file>
|||	        The resulting UTF texture.
|||	
|||	  See Also
|||	
|||	    utfmipcat
**/

#include "M2TXlib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   Undo utfmipcat.\n");
}

int main( int argc, char *argv[] )
{
  M2TX tex, newTex, *appendTex;
  M2TXTex lodPtr;
  M2TXPIP *oldPIP;
  M2TXDCI *oldDCI;
  M2TXHeader *header, *newHeader, *appendHeader;
  char fileIn[256];
  int argn;
  char fileOut[256];
  int nnLOD;
  uint16 xSize, ySize;
  uint32 lodXSize, lodYSize;
  uint32 length, startPos;
  FILE *fPtr;
  uint8 numLOD, i;
  M2Err err;
  
#ifdef M2STANDALONE
  printf("Enter: <FileIn> <new # LODs> <FileOut>\n");
  printf("Example: dumb.utf 5 dumb.cmp.utf\n");
  fscanf(stdin,"%s %d %s",fileIn, &nnLOD, fileOut);
	
#else
  /* Check for command line options. */
  if (argc != 4)
    {
      fprintf(stderr,"Usage: %s <Input File> <# LODs> <Output File>\n",argv[0]);
      print_description();
      return(-1);    
    }
  strcpy(fileIn, argv[1]);
  nnLOD = strtol(argv[2], NULL, 10);
  argn = 3;
  if ( argn != argc )
    {
      /* Open the input file. */
      strcpy( fileOut, argv[argn] );
    }
  else
    {
      /* No input file specified. */
      fprintf(stderr,"Usage: %s <Input File> <# LODs> <Output File>\n",argv[0]);
      return(-1);   
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
  
  M2TX_Init(&tex);			/* Initialize a texture */
  M2TX_Init(&newTex);			/* Initialize a texture */
  err = M2TX_ReadFile(fileIn,&tex);
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Bad file \"%s\" \n",fileIn);
      return(-1);
    }
  
  M2TX_GetHeader(&tex,&header);

  M2TX_Copy(&newTex, &tex);
  /*  M2TX_SetHeader(&newTex, header); */

  M2TX_GetHeader(&newTex, &newHeader);
  M2TX_GetPIP(&tex, &oldPIP);
  M2TX_SetPIP(&newTex, oldPIP);
  M2TX_GetDCI(&tex, &oldDCI);
  M2TX_SetDCI(&newTex, oldDCI);
  
  numLOD = nnLOD;
  err = M2TXHeader_GetMinXSize(header, &xSize);
  err = M2TXHeader_GetMinYSize(header, &ySize);
  
  lodXSize = xSize >>(numLOD-1);  /* Smallest LOD size */
  lodYSize = ySize >>(numLOD);  /* Smallest LOD size */;
  
  M2TXHeader_SetNumLOD(newHeader, numLOD);
  
  M2TXHeader_SetMinXSize(newHeader, lodXSize);
  M2TXHeader_SetMinYSize(newHeader, lodYSize);
  
  startPos = 0;
  for (i=0; i<numLOD; i++)
    {
      lodXSize = xSize >>(i);  /* Smallest LOD size */
      lodYSize = ySize >>(i+1);  /* Smallest LOD size */;
      M2TX_Extract(&tex, 1, 0, 0, startPos, lodXSize, lodYSize, &appendTex);
      startPos += lodYSize;
      M2TX_GetHeader(appendTex ,&appendHeader);
      M2TXHeader_GetLODPtr(appendHeader,0,&length, &lodPtr);
      M2TXHeader_SetLODPtr(newHeader,i,length, lodPtr);
     }
  
  M2TX_WriteFile(fileOut,&newTex);    			/* Write it to disk */
  
  M2TXHeader_FreeLODPtrs(header);
  M2TXHeader_FreeLODPtrs(newHeader);
  
  return(0);
}



