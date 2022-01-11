/*
	File:		utfflip.c

	Contains:	Take a utf and vertically flip it across all levels
	                of detail and leave everything else unchanged.

	Written by:	Todd Allendorf 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <2>	 7/16/95	TMA		Removed unneeded variables.
	To Do:
*/


/**
|||	AUTODOC -public -class tools -group m2tx -name utfflip
|||	Flip a texture on its y-axis
|||	
|||	  Synopsis
|||	
|||	    utfflip <input file> <output file>
|||	
|||	  Description
|||	
|||	    This tool takes a UTF file and vertically flips it the texel data.
|||	    Everything but the texel data is left untouched.
|||	
|||	  Arguments
|||	
|||	    <input file>
|||	        The input UTF texture.
|||	    <output file>
|||	        The resulting UTF texture.
|||	
**/

#include "M2TXlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   Vertically flip a UTF image.\n");
}

int main( int argc, char *argv[] )
{
  M2TX tex, newTex;
  M2TXDCI *dci;
  M2TXRaw raw;
  M2TXTex texel;
  M2TXIndex index;
  M2TXHeader *header, *newHeader;
  M2TXPIP *oldPIP;
  char fileIn[256];
  char fileOut[256];
  uint8 *lineBuffer, *linePtr;
  bool hasPIP, isCompressed, isLiteral, hasColor, hasAlpha, hasSSB;
  uint8 numLOD, aDepth, cDepth, ssbDepth;
  uint8 lod;
  uint16 xSize, ySize;
  uint32 lodXSize, lodYSize, cmpSize, i;
  FILE *fPtr;
  M2Err err;
  long tmpDepth=8;
  
#ifdef M2STANDALONE	
  printf("Enter: <FileIn> <FileOut>\n");
  printf("Example: dumb.utf dumb.flip.utf\n");
  fscanf(stdin,"%s %s",fileIn, fileOut);
  
#else
  /* Check for command line options. */
  if (argc != 3)
    {
      fprintf(stderr,"Usage: %s <Input File> <Output File>\n",argv[0]);
      print_description();
      return(-1);    
    }
  strcpy(fileIn, argv[1]);
  strcpy(fileOut, argv[2]);
#endif
  
  fPtr = fopen(fileIn, "r");
  if (fPtr == NULL)
    {
      fprintf(stderr,"ERROR:Can't open file \"%s\" \n",fileIn);
      return(-1);
    }
  else 
    fclose(fPtr);		
  
  M2TX_Init(&tex);				/* Initialize a texture */
  M2TX_Init(&newTex);				/* Initialize a texture */
  
  err = M2TX_ReadFile(fileIn,&tex);

  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Bad file \"%s\" \n",fileIn);
      return(-1);
    }
  M2TX_GetHeader(&tex,&header);
  M2TX_GetHeader(&newTex,&newHeader);

  M2TX_Copy(&newTex, &tex);
  /*  M2TX_SetHeader(&newTex, header); */

  M2TXHeader_GetFIsCompressed(header, &isCompressed);
  M2TXHeader_GetFIsLiteral(header, &isLiteral);
  M2TXHeader_GetFHasColor(header, &hasColor);
  if (hasColor)
    M2TXHeader_GetCDepth(header, &cDepth);
  else
    cDepth = 0;
  M2TXHeader_GetFHasAlpha(header, &hasAlpha);
  if (hasAlpha)
    M2TXHeader_GetADepth(header, &aDepth);
  else
    aDepth = 0;
  M2TXHeader_GetFHasSSB(header, &hasSSB);
  if (hasSSB)
    ssbDepth = 1;
  else
    ssbDepth = 0;
  M2TXHeader_GetFHasPIP(header, &hasPIP);
  if (hasPIP)
    {
      M2TX_GetPIP(&tex,&oldPIP);
      M2TX_SetPIP(&newTex, oldPIP);
    }
  if (isCompressed)
    {
      M2TX_GetDCI(&tex,&dci);
      M2TX_SetDCI(&newTex, dci);
    }
  
  M2TXHeader_GetNumLOD(header, &numLOD);
  err = M2TXHeader_GetMinXSize(header, &xSize);
  err = M2TXHeader_GetMinYSize(header, &ySize);

  lodXSize = xSize * (1<<(numLOD-1));
  lodYSize = ySize * (1<<(numLOD-1));
 
  for (lod=0; lod<numLOD; lod++)
    {
      lineBuffer = (uint8 *)malloc(lodXSize);
      if (lineBuffer == NULL)
	{
	  fprintf(stderr,"ERROR:Allcation failed!\n");
	  return(-1);
	}
      if (isCompressed)
	{
	  if (isLiteral)
	    err = M2TX_ComprToM2TXRaw(&tex, oldPIP, lod, &raw);
	  else
	    err = M2TX_ComprToM2TXIndex(&tex, oldPIP,lod, &index);
	}
      else
	{
	  if (isLiteral)
	    err = M2TX_UncomprToM2TXRaw(&tex, oldPIP, lod, &raw);
	  else
	    err = M2TX_UncomprToM2TXIndex(&tex, oldPIP, lod, &index);
	}
      for (i=0; i<(lodYSize/2); i++)
	{
	  if (hasColor)
	    {
	      if (isLiteral)
		{
		  linePtr = raw.Red;
		  memcpy(lineBuffer, linePtr+(i*lodXSize), lodXSize);
		  memcpy(linePtr+(i*lodXSize), linePtr+((lodYSize-1-i)*lodXSize),lodXSize);
		  memcpy(linePtr+((lodYSize-1-i)*lodXSize),lineBuffer,lodXSize);
		  linePtr = raw.Green;
		  memcpy(lineBuffer, linePtr+(i*lodXSize), lodXSize);
		  memcpy(linePtr+(i*lodXSize), linePtr+((lodYSize-1-i)*lodXSize),lodXSize);
		  memcpy(linePtr+((lodYSize-1-i)*lodXSize),lineBuffer,lodXSize);
		  linePtr = raw.Blue;
		  memcpy(lineBuffer, linePtr+(i*lodXSize), lodXSize);
		  memcpy(linePtr+(i*lodXSize), linePtr+((lodYSize-1-i)*lodXSize),lodXSize);
		  memcpy(linePtr+((lodYSize-1-i)*lodXSize),lineBuffer,lodXSize);
		}
	      else
		{
		  linePtr = index.Index;
		  memcpy(lineBuffer, linePtr+(i*lodXSize), lodXSize);
		  memcpy(linePtr+(i*lodXSize), linePtr+((lodYSize-1-i)*lodXSize),lodXSize);
		  memcpy(linePtr+((lodYSize-1-i)*lodXSize),lineBuffer,lodXSize);
		}
	    }
	  if (hasAlpha)
	    {
	      if (isLiteral)
		linePtr = raw.Alpha;
	      else 
		linePtr = index.Alpha;
		  memcpy(lineBuffer, linePtr+(i*lodXSize), lodXSize);
		  memcpy(linePtr+(i*lodXSize), linePtr+((lodYSize-1-i)*lodXSize),lodXSize);
		  memcpy(linePtr+((lodYSize-1-i)*lodXSize),lineBuffer,lodXSize);
	    }
	  if (hasSSB)
	    {
	      if (isLiteral)
		linePtr = raw.SSB;
	      else 
		linePtr = index.SSB;
		  memcpy(lineBuffer, linePtr+(i*lodXSize), lodXSize);
		  memcpy(linePtr+(i*lodXSize), linePtr+((lodYSize-1-i)*lodXSize),lodXSize);
		  memcpy(linePtr+((lodYSize-1-i)*lodXSize),lineBuffer,lodXSize);

	    }
	}
      if (isLiteral)
	{
	  err = M2TXRaw_ToUncompr(&newTex, oldPIP, lod, &raw);
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:Error during conversion, must abort.\n");
	      return(-1);
	    }				
	  M2TXRaw_Free(&raw);
	}
      else
	{
	  err = M2TXIndex_ToUncompr(&newTex, oldPIP, lod, &index);
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:Error during conversion, must abort.\n");
	      return(-1);
	    }				
	  M2TXIndex_Free(&index);
	}
      
      if (isCompressed)
	{   /* LOD comes in uncompressed */
	  M2TXHeader_SetFIsCompressed(newHeader, FALSE); 
	  err = M2TX_Compress(&newTex, lod, dci, oldPIP, 
			      M2CMP_Auto|M2CMP_CustomDCI|M2CMP_CustomPIP, 
			      &cmpSize, &texel); 
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:Error during compression, must abort.\n");
	      return(-1);
	    }
	  M2TXHeader_FreeLODPtr(newHeader, lod);  /* Free up the old LOD ptr */
	  M2TXHeader_SetLODPtr(newHeader, lod, cmpSize, texel);
	  /* Replace it with the new */
	  M2TXHeader_SetFIsCompressed(newHeader, TRUE);
	  /* LOD is now compressed */
	}    

      free(lineBuffer);
      
      lodXSize = lodXSize >> 1;
      lodYSize = lodYSize >> 1;
    }
  
  M2TX_WriteFile(fileOut,&newTex);    	/* Write it to disk */
  M2TXHeader_FreeLODPtrs(newHeader);
  
  return(0);
  
}
