/*
	File:		quanttopip.c

	Contains:	An example of using M2TXlib's quantizing functions.

	Written by:	Todd Allendorf 

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

	To Do:
*/

/**
|||	AUTODOC -public -class tools -group m2tx -name quanttopip
|||	Quantize an image with the PIP of another image
|||	
|||	  Synopsis
|||	
|||	    quanttopip <input file> <reference image> [options] <output file>
|||	
|||	  Description
|||	
|||	    This tool takes a UTF file and a "reference file", and quantizes
|||	    the color channel of the image with the PIP of the reference image.
|||	    Dithering can be performed during quantization and the PIP may be
|||	    excluded from the file output.  The file image will be an indexed
|||	    image with the same color depth as the reference image.
|||	
|||	  Arguments
|||	
|||	    <input file>
|||	        The input UTF texture.
|||	    <reference image file>
|||	        The image whose PIP is used for color quantization
|||	    <output file>
|||	        The resulting UTF texture.
|||	
|||	  Options
|||	
|||	    -floyd
|||	        Perform Floyd-Steinberg dithering during color reduction
|||	    -nopip
|||	        Suppress writing the PIP during the final output.
|||	    -alpha
|||	        Reduce the values of the alpha channel instead of the color channel
|||	
|||	  See Also
|||	
|||	    utflitdown, utfmakepip, quantizer
**/


#include<stdio.h>
#include "M2TXlib.h"
#include <stdlib.h>
#include <string.h>

void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   Remap all the levels of details with the reference image's PIP\n");
  printf("   -nopip \tDon't write out the PIP in the output image\n");
  printf("   -floyd \tUse Floyd-Steinberg dithering on the result\n");
  printf("   -alpha \tQuantize the alpha instead of the alpha channel channel\n");
}

#define Usage  printf("Usage: %s <Input File> <Reference File> [-nopip] [-floyd] <Output File>\n",argv[0])

void M2TXRaw_MapPreserve();

M2Err make_legal(M2TXHeader *header)
{
  uint8 aDepth, cDepth, ssbDepth;
  bool isLiteral, hasSSB, hasColor, hasAlpha;
  bool needSSB = FALSE;

  M2TXHeader_GetFIsLiteral(header, &isLiteral);
  M2TXHeader_GetFHasSSB(header, &hasSSB);
  if (hasSSB)
    ssbDepth = 1;
  else
    ssbDepth = 0;
  M2TXHeader_GetFHasAlpha(header, &hasAlpha);
  if (hasAlpha)
    M2TXHeader_GetADepth(header, &aDepth);	
  else
    aDepth = 0;
  M2TXHeader_GetFHasColor(header, &hasColor);
  if (hasColor)
    M2TXHeader_GetCDepth(header, &cDepth);	
  else
    cDepth = 0;

  while ((!(M2TX_IsLegal(cDepth, aDepth, ssbDepth, isLiteral))) && (cDepth<9))
    cDepth++;
  if (cDepth>8)
    {
      cDepth = 8;
      if (!hasSSB)
	{
	  if (M2TX_IsLegal(cDepth,aDepth, 1, isLiteral))
	    needSSB = TRUE;
	  else
	    {
	      fprintf(stderr,"ERROR:Can't find the right color, alpha, ssb combo\n");
	      return(M2E_Range);
	    }
	}	
      else
	{
	  fprintf(stderr,"ERROR:Can't find the right color, alpha, ssb combo\n");
	  return(M2E_Range);
	}
    }
  M2TXHeader_SetCDepth(header,cDepth);
  if (needSSB)
    M2TXHeader_SetFHasSSB(header, TRUE);
}

int main( int argc, char *argv[] )
{
  M2TXRaw       raw, rawAlpha;
  M2TX          tex, newTex, refTex;
  bool          isCompressed, hasColor;
  bool          noPIP = FALSE;
  bool          floyd = FALSE;
  bool          hasPIP;
  bool          alpha = FALSE;
  M2TXHeader    *header, *newHeader, *refHeader;
  char          fileIn[256];
  char          refFile[256];
  char          fileOut[256];
  uint8         numLOD, lod, cDepth, ssb, opac, red, grn, blu;
  int           numColors, i;
  M2TXColor     color;
  int argn;
  M2Err         err;
  uint32        curPixel, pixels;
  M2TXPIP       *oldPIP, *refPIP, alphaPIP;
  FILE          *fPtr;

 
#ifdef M2STANDALONE
  printf("Enter: <FileIn> <numColors> <FileOut>\n");
  printf("Example: dumb.utf ref.utf dumb.indexed.utf\n");
  fscanf(stdin,"%s %s %s",fileIn, refFile, fileOut);
#else
  /* Check for command line options. */
  if (argc < 4)
    {
      print_description();
      return(-1);    
    }
  strcpy(fileIn, argv[1]);
  strcpy(refFile, argv[2]);
  argn = 3;
  while ( argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0' )
    {
      if ( strcmp( argv[argn], "-nopip")==0 )
        {
	  ++argn;
	  noPIP = TRUE;
        }      
      else if ( strcmp( argv[argn], "-alpha")==0 )
        {
	  ++argn;
	  alpha = TRUE;
        }
      else if ( strcmp( argv[argn], "-floyd")==0 )
        {
	  ++argn;
	  floyd = TRUE;
        }
      else
	{
	  Usage;
	  return(-1);
	}
    }
  if ( argn != argc )
    {
      /* Open the output file. */
      strcpy( fileOut, argv[argn] );
    }
  else
    {      /* No output file specified. */
      Usage;
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
  M2TX_Init(&refTex);			/* Initialize a texture */
  
  err = M2TX_ReadFile(fileIn,&tex);
  if (err != M2E_NoErr)
    return(-1);
  err = M2TX_ReadFileNoLODs(refFile,&refTex);
  if (err != M2E_NoErr)
    return(-1);
  M2TX_GetHeader(&tex,&header);
  M2TX_GetHeader(&refTex,&refHeader);
  /*  M2TX_SetHeader(&newTex, header);  */
  M2TX_Copy(&newTex, &tex);
/* Old texture header to the new texture */

  M2TX_GetHeader(&newTex,&newHeader);
  M2TXHeader_GetFHasPIP(refHeader, &hasPIP);
  if (!hasPIP)
    {
      fprintf(stderr,"ERROR:  Reference image \"%s\" does not have a PIP.\n", refFile);
      return(-1);
    }

  M2TXHeader_GetFIsCompressed(header, &isCompressed);
  M2TXHeader_GetFHasColor(header, &hasColor);
  M2TXHeader_GetCDepth(refHeader, &cDepth);
  M2TX_GetPIP(&tex,&oldPIP);
  M2TX_GetPIP(&refTex, &refPIP);
  if (!alpha)
    {
      M2TX_SetPIP(&newTex, refPIP);               /* Use the ref's PIP */
      M2TXHeader_SetFIsLiteral(newHeader, FALSE); /* New image is an indexed image */
      M2TXHeader_SetCDepth(newHeader, cDepth);    /* With the same color Depth */
  
      if (!noPIP)                                /* Write out the PIP? */
	M2TXHeader_SetFHasPIP(newHeader, TRUE); 
    }
  M2TXHeader_SetFIsCompressed(newHeader, FALSE);

  M2TXHeader_GetNumLOD(header, &numLOD);

  numColors = refPIP->NumColors;

  if (alpha) 
    {
      M2TXHeader_SetFHasAlpha(newHeader, TRUE);
      M2TXHeader_SetADepth(newHeader, 7);
      M2TXHeader_SetFHasSSB(newHeader, TRUE);
      for (i=0; i<numColors; i++)
	{
	  M2TXPIP_GetColor(refPIP, i, &color);
	  M2TXColor_Decode(color, &ssb, &opac, &red, &grn, &blu);
	  color = M2TXColor_Create(0, 0, opac, opac, opac);
	  M2TXPIP_SetColor(&alphaPIP, i, color);
	}
      M2TXPIP_SetNumColors(&alphaPIP, numColors);
    }
  make_legal(newHeader);
  M2TXRaw_MapPreserve();

  for (lod=0; lod<numLOD; lod++)
    {
      if (isCompressed)
	err = M2TX_ComprToM2TXRaw(&tex, oldPIP, lod, &raw);
      else
	err = M2TX_UncomprToM2TXRaw(&tex, oldPIP, lod, &raw);
      
      if (err != M2E_NoErr)
	{
	  fprintf(stderr,"ERROR:Error during decompression, must abort.\n");
	  return(-1);
	} 
      
      if (alpha)
	{
	  if (raw.Alpha == NULL)
	    {
	      fprintf(stderr,"ERROR:No alpha channel to process.\n");
	      fprintf(stderr,"If alpha values are in the PIP, use utfmakesame to create an image with a literal alpha channel.\n");
	      M2TXRaw_Free(&raw);
	      continue;
	    }
	  M2TXRaw_Init(&rawAlpha, raw.XSize, raw.YSize, TRUE, FALSE, FALSE);
	  pixels = raw.XSize * raw.YSize;	  
	  for (curPixel=0; curPixel<pixels; curPixel++)
	    {
	      rawAlpha.Red[curPixel] = rawAlpha.Green[curPixel] 
		= rawAlpha.Blue[curPixel] = raw.Alpha[curPixel];
	    }
	  err = M2TXRaw_MapToPIP(&rawAlpha, &alphaPIP, numColors,
				 floyd);
	      	      
	  /* err = M2TXRawAlpha_MapToPIP(&raw, newPIP, numColors,
	     floyd);
	     */
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:An error occured during ditherof \"%s\" lod=%d:%d\n", fileIn, lod, err);	
	      return(-1);
	    }
	  for (curPixel=0; curPixel<pixels; curPixel++)
	    {
	      raw.Alpha[curPixel] = rawAlpha.Red[curPixel]; 
	    }
	  M2TXRaw_Free(&rawAlpha);
	  err = M2TXRaw_ToUncompr(&newTex, oldPIP, lod, &raw);
	}
      else
	{
	  if (hasColor)
	    {
	      /* Map the raw image to the reference PIP */
	      err = M2TXRaw_MapToPIP (&raw, refPIP, numColors, floyd);
	      if (err != M2E_NoErr)
		{
		  fprintf(stderr,"ERROR:An error occured during dither=%d\n",err);
		  return(-1);
		}
	    }
	  /* Write out the new quantized texture, changing nothing else */
	  err = M2TXRaw_ToUncompr(&newTex, refPIP, lod, &raw);
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:An error occured during encoding=%d\n",err);
	      return(-1);
	    }
	}
      M2TXRaw_Free(&raw);
    }
  M2TX_WriteFile(fileOut, &newTex);
  return(0);
}
