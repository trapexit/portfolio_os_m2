/*
	File:		utfaclean.c

	Contains:	Clean up color channel of matted images.

	Written by:	Todd Allendorf 

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <4>	  8/7/95	TMA		Added autodocs and more error checking.
		 <2>	 7/16/95	TMA		Removed unused variables
	To Do:
*/



/**
|||	AUTODOC -public -class tools -group m2tx -name utfaclean
|||	Sets all transparent (Alpha=0) pixels to black
|||	
|||	  Synopsis
|||	
|||	    utfsdfpage <input file> <output UTF file> 
|||	
|||	  Description
|||	
|||	    This tool takes a UTF file and scans for Alpha values equal to 0.
|||	    It then sets all such pixels to pure black.  This is useful for
|||	    processing images that are created by merging alpha channels with
|||	    color channels.  This makes sure that (if the image is later turned
|||	    into an indexed image) that there are no wasted palette entries by
|||	    making sure that all transparent values have only one color.
|||	
|||	  Arguments
|||	
|||	    <input file>
|||	        The input UTF file.
|||	    <output file>
|||	        The resulting UTF texture
|||	
|||	  See Also
|||	
|||	    utfmerge, utfmakepip, quantizer, utfquantmany
**/


#include<stdio.h>
#include "M2TXlib.h"
#include <stdlib.h>
#include <string.h>

void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   Clear color and ssb values of transparent pixels\n");
}

int main( int argc, char *argv[] )
{
  uint32        pixels, curPixel;
  M2TXRaw       raw;
  M2TX tex, newTex;
  bool          isCompressed, hasColor, hasAlpha, hasSSB;
  bool          firstLOD;
  bool          alpha = FALSE;
  bool          floyd = FALSE;
  M2TXHeader    *header, *newHeader;
  char fileIn[256];
  char fileOut[256];
  uint8         numLOD;
  int           lod;
  int argn;
  M2Err         err;

  M2TXPIP       *oldPIP;
  FILE          *fPtr;
 
#ifdef M2STANDALONE
  printf("Enter: <FileIn> <FileOut>\n");
  printf("Example: dumb.utf outfile.utf\n");
  fscanf(stdin,"%s %s",fileIn, fileOut);
#else
  /* Check for command line options. */
  if (argc < 3)
    {
      fprintf(stderr,"Usage: %s <Input File> <Output File>\n",argv[0]);
      print_description();
      return(-1);    
    }
  strcpy(fileIn, argv[1]);
  argn = 2;
  strcpy( fileOut, argv[argn] );
  
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
    return(-1);
  M2TX_GetHeader(&tex,&header);
  M2TX_GetHeader(&newTex,&newHeader);
  /*  M2TX_SetHeader(&newTex, header); */
  M2TX_Copy(&newTex, &tex);
/* Old texture header to the new texture */
  M2TXHeader_GetFIsCompressed(header, &isCompressed);
  M2TXHeader_GetFHasColor(header, &hasColor);
  M2TXHeader_GetFHasAlpha(header, &hasAlpha);
  M2TXHeader_GetFHasSSB(header, &hasSSB);
  M2TX_GetPIP(&tex,&oldPIP);
  
  M2TXHeader_GetNumLOD(header, &numLOD);
  
  firstLOD = TRUE;
  for (lod=0; lod<numLOD; lod++)
    {
      if (isCompressed)
	err = M2TX_ComprToM2TXRaw(&tex, oldPIP, lod, &raw);
      else
	err = M2TX_UncomprToM2TXRaw(&tex, oldPIP, lod, &raw);
      
      if (err != M2E_NoErr)
	{
	  fprintf(stderr,"ERROR:Error during decompression file \"%s\" lod=%d, must abort.\n", fileIn, lod);
	  return(-1);
	} 
      if (hasAlpha)
	{
	  pixels = raw.XSize*raw.YSize;
	  for (curPixel=0; curPixel<pixels; curPixel++)
	    {
	      if(raw.Alpha[curPixel]==0)
		{
		  if (hasColor)
		    {
		      raw.Red[curPixel] = raw.Green[curPixel] = 
			raw.Blue[curPixel] = 0;
		    }
		  if (hasSSB)
		    {
		      raw.SSB[curPixel] = 0;
		    }
		}
	    }
	  err = M2TXRaw_ToUncompr(&newTex, oldPIP, lod, &raw);
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:An error occured during encoding of image\"%s\" lod=%d : %d\n", fileIn, lod, err);
	      return(-1);
	    }
	}
      M2TXRaw_Free(&raw);
    }
  M2TX_WriteFile(fileOut, &newTex);

  return(0);
}






