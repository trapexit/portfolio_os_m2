/*
	File:		utfacompress.c

	Contains:	Takes any utf file and compresses it as best it can taking into
	              account the alpha channel.

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
|||	AUTODOC -public -class tools -group m2tx -name utfacompress
|||	Compresses a UTF file using the desired compression type(s).
|||	
|||	  Synopsis
|||	
|||	    utfacompress <input file> [options] <output file>
|||	
|||	  Description
|||	
|||	    This tool a UTF texture file and compresses it using a scheme selected by
|||	    the user or does its best if no options are selected.  
|||	
|||	  Caveats
|||	
|||	    The files must be the same type (color depth, alpha depth, ssb component
|||	    compression, pip, etc.) as it makes no attempt to resolve any differences.
|||	
|||	  Arguments
|||	
|||	    <input file>
|||	        The input UTF texture.
|||	    <output file>
|||	        The resulting UTF texture.
|||	
|||	  Options
|||	
|||	    -rle
|||	        Just compress the texture using run-length encoding.
|||	    -lockedPIP
|||	        Do multi-texel compression and run-length encoding, but keep the PIP
|||	        the same.
|||	    -best
|||	        Does all of the above and also may sort the PIP to the optimal order.
|||	
|||	  Caveats
|||	    -lockedPIP and -best take almost the same amount of time and can be quite
|||	    long.  A 128x128x8 bt image can take about thirty seconds on a PowerMac 
|||	    while a 256x256x8 image can take five minutes.  Lower color depths aren't
|||	    as time intensive.  It is recommended that you compress only after the
|||	    textures have been resized and carved to their final size (to fit into
|||	    TRAM <16K for most textures).
|||	
|||	    Compression type -best also reorders the PIP, so if textures are expected
|||	    share a PIP, the -lockedPIP option should be used or the utfmakesame tool.
|||	    For a bunch of similar images, compression need not be done individually
|||	    on each texture.  One image can be compressed and then used as a reference
|||	    for utfmakesame.  Compression will then only take seconds and the images
|||	    will share the same format (multi-texel format and PIP) so they can be
|||	    animated easily.  Also, the PIP maybe stripped from all but one image in
|||	    a series of same-format images thus saving up to 1K for each texture.
|||	
|||	  See Also
|||	
|||	    utfmakesame, utfstrip
**/



#include "M2TXlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define NUM_ALPHAS 6
M2Err M2TX_CompressWithAlpha(M2TX *tex, uint8 lod, M2TXDCI *newDCI, M2TXPIP *newPIP, 
			     uint16 cmpType, uint32 *size, M2TXTex *newTexel)
{
  M2TX    tempTex;
  M2TXPIP *oldPIP, *usePIP;
  M2TXDCI *oldDCI, aDCI;
  M2TXRaw myRaw;
  M2TXColor color, bestColor;
  M2TXIndex myIndex, format, *cmpFormat;
  M2TXFormat txFormat, bestFormat;
  M2TXHeader	*header;
  uint8 cDepth, aDepth,  ssbDepth, depth, numLOD;
  uint8 bestAlpha, red, green, blue, alpha, ssb;
  uint32 xSize, ySize, pixels, p;
  uint32 aUsage[256], bUsage[NUM_ALPHAS], origSize, bestSize;
  uint16 minX, minY, i, j, k, best;
  M2Err err;
  bool isCompressed, isLiteral, hasColor, hasAlpha, hasSSB;
  bool texelIsTemp, rle, bestDCI, bestPIP, customPIP, customDCI, lockPIP;
	
  err = M2TX_GetHeader(tex,&header);
  err = M2TX_GetPIP(tex,&oldPIP);
  err = M2TX_GetDCI(tex,&oldDCI);
  err = M2TXHeader_GetFIsLiteral(header, &isLiteral);
  err = M2TXHeader_GetFIsCompressed(header, &isCompressed);
  err = M2TXHeader_GetMinXSize(header, &minX);
  err = M2TXHeader_GetMinYSize(header, &minY);
  err = M2TXHeader_GetNumLOD(header, &numLOD);
  err = M2TXHeader_GetFHasColor(header, &hasColor);
  err = M2TXHeader_GetFHasAlpha(header, &hasAlpha);
  err = M2TXHeader_GetFHasSSB(header, &hasSSB);
  if (hasColor)
    err = M2TXHeader_GetCDepth(header, &cDepth);
  else
    cDepth = 0;
  if (hasAlpha)
    err = M2TXHeader_GetADepth(header, &aDepth);
  else
    aDepth = 0;	
  if (hasSSB) 
    ssbDepth = 1;
  else
    ssbDepth = 0;
  if (isLiteral)
    depth = ssbDepth + aDepth + 3*cDepth;
  else
    depth = ssbDepth + aDepth + cDepth;	
  if (!isLiteral)
    if (oldPIP == NULL)
      return(M2E_BadPtr);		/* No PIP to decode the data with */

  xSize = minX << (numLOD-lod-1);
  ySize = minY << (numLOD-lod-1);
  texelIsTemp = FALSE;
	
  if (isCompressed)		/* Get it into uncompressed form for compression */
    {
      if (isLiteral)
	err = M2TX_ComprToM2TXRaw(tex, oldPIP, lod, &myRaw);
      else
	err = M2TX_ComprToM2TXIndex(tex, oldPIP, lod, &myIndex);			
    }
  else
    {
      if (isLiteral)
	err = M2TX_UncomprToM2TXRaw(tex, oldPIP, lod, &myRaw);
      else
	err = M2TX_UncomprToM2TXIndex(tex, oldPIP, lod, &myIndex);			
    }
  if (err != M2E_NoErr)
    return (err);
 				
  /*	if (24 and rle)					/* Too experimental (and probably no gain) */
  /*		try Create24_16Cmp */

  *newTexel = NULL;
  *size = 0;
  cmpFormat = NULL;
  customDCI = bestDCI = bestPIP = customPIP = rle = lockPIP = FALSE;
  if (cmpType&M2CMP_RLE)
    rle = TRUE;
  if ((cmpType&M2CMP_BestDCIPIP) && (!isLiteral))
    {
      bestDCI = TRUE;
      bestPIP = TRUE;
    }
  if (cmpType&M2CMP_CustomPIP)
    customPIP = TRUE;
  if (cmpType&M2CMP_CustomDCI)	
    customDCI = TRUE;
  if (cmpType&M2CMP_Auto)			/* 1.04 */
    {
      rle = TRUE;
      if (!isLiteral)
	{
	  bestDCI = TRUE;
	  bestPIP = TRUE;
	}
    }
  if (cmpType&M2CMP_LockPIP)
    {
      bestPIP = customPIP = FALSE;
      lockPIP = TRUE;
    }
  if (bestPIP || customPIP)
    usePIP = newPIP;
  else
    usePIP = oldPIP;
  if (((!customDCI) && (!bestDCI)) || isLiteral) 		
    { 	/* In case the user was sloppy and didn't copy the Format to the DCI, for RLE only 1.06 */
      M2TXHeader_GetTexFormat(header, &txFormat);
      M2TXHeader_GetColorConst(header, 0, &color);
      for (i=0; i<4; i++)
	{
	  M2TXDCI_SetTexFormat(newDCI,i,txFormat);
	  M2TXDCI_SetColorConst(newDCI, i, color);
	}
    }
  if (isLiteral)
    {
      return(M2E_Range);   /* Can't Compress a Literal Image anymore */
    }	
  else	
    {
      if (bestDCI && (!customDCI) && (!customPIP))
	{
	  if ((!lockPIP))  	
	    M2TXIndex_FindBestDCIPIP(&myIndex, tex, newDCI, newPIP, cDepth, aDepth, hasSSB);
	  else
	    M2TXIndex_FindBestDCI(&myIndex, tex, newDCI, cDepth, aDepth, hasSSB);
	  
	  /* Find alpha values */
	  if (aDepth > 0)
	    {
	      pixels = xSize*ySize;
	      for (p=0; p<256; p++)
		aUsage[p]=0;
	      for (p=0; p<pixels; p++)
		aUsage[myIndex.Alpha[p]]++;
	      
	  /* Get the size of the thing without alpha changes */
	      M2TXIndex_Init(&format, myIndex.XSize, myIndex.YSize, TRUE, TRUE, TRUE);
	      err = M2TXIndex_SmartFormatAssign(&myIndex, &format, hasColor, hasAlpha,
						hasSSB, newDCI, usePIP);
	      cmpFormat = &format;
	      err = M2TXIndex_Compress(&myIndex, cmpFormat, rle, newDCI, size, newTexel);
	      origSize = *size; 
	      
	      for (i=0; i<4; i++)
		{
		  M2TXDCI_GetTexFormat(newDCI,i,&txFormat);
		  M2TXDCI_GetColorConst(newDCI,i,&color);
		  M2TXDCI_SetTexFormat(&aDCI,i,txFormat);
		  M2TXDCI_SetColorConst(&aDCI,i,color);
		}
	      
	      for (i=0; i<NUM_ALPHAS; i++)     /* Check the top # alpha values used */
		{
		  bestAlpha = 0;
		  for (j=1; j<256; j++)
		    {
		      if (aUsage[j]>aUsage[bestAlpha])
			bestAlpha = j;
		    }
		  bUsage[i] = bestAlpha;
		  if (aUsage[bestAlpha] > 0)
		    {
		      aUsage[bestAlpha] = 0;
		      for (k=1; k<4; k++)
			{
			  M2TXDCI_GetTexFormat(&aDCI, k, &txFormat);
			  M2TXDCI_GetColorConst(&aDCI, k, &color);
			  aDepth = M2TXFormat_GetADepth(txFormat);
			  hasSSB = M2TXFormat_GetFHasSSB(txFormat);
			  if (!((aDepth==7)&&(hasSSB)))
			    {
			      M2TXFormat_SetADepth(&txFormat, 0);
			      
			      if (aDepth == 7)
				M2TXFormat_SetFHasSSB(&txFormat, FALSE);
			      M2TXDCI_SetTexFormat(&aDCI, k, txFormat);
			      M2TXColor_Decode(color, &ssb, &alpha, &red, &green, &blue);
			      color = M2TXColor_Create(ssb, bestAlpha, red, green, blue);
			      M2TXDCI_SetColorConst(&aDCI, k, color);
			      err = M2TXIndex_SmartFormatAssign(&myIndex, &format, hasColor,
								hasAlpha, hasSSB, &aDCI, usePIP);
			      cmpFormat = &format;
			      err = M2TXIndex_Compress(&myIndex, cmpFormat, rle, &aDCI, size, 
						       newTexel);
			      if (*size < origSize)
				{
				  origSize = *size;
				  M2TXDCI_SetTexFormat(newDCI, k, txFormat);
				  M2TXDCI_SetColorConst(newDCI, k, color);
				}
			      else
				{
				  M2TXDCI_GetTexFormat(newDCI,k,&txFormat);
				  M2TXDCI_GetColorConst(newDCI,k,&color);
				  M2TXDCI_SetTexFormat(&aDCI,k,txFormat);
				  M2TXDCI_SetColorConst(&aDCI,k,color);
				}
			    }
		     
			}
		    }
		}
	      
	      for (i=0; i<NUM_ALPHAS; i++)     /* Check the top # alpha values used */
		{
		  bestAlpha = bUsage[i];
		  M2TXDCI_GetTexFormat(&aDCI, 0, &txFormat);
		  M2TXDCI_GetColorConst(&aDCI, 0, &color);
		  bestSize = origSize;
		  best = 0;
		  for (k=1; k<4; k++)
		    {
		      aDepth = M2TXFormat_GetADepth(txFormat);
		      hasSSB = M2TXFormat_GetFHasSSB(txFormat);
		      if (!((aDepth==7)&&(hasSSB)))
			/* Can't have SSB if not in expanded */
			{
			  M2TXFormat_SetADepth(&txFormat, 0);			  
			  if (aDepth == 7)
			    M2TXFormat_SetFHasSSB(&txFormat, FALSE);
			  M2TXDCI_SetTexFormat(&aDCI, k, txFormat);
			  M2TXColor_Decode(color, &ssb, &alpha, &red, &green, &blue);
			  color = M2TXColor_Create(ssb, bestAlpha, red, green, blue);
			  M2TXDCI_SetColorConst(&aDCI, k, color);
			  err = M2TXIndex_SmartFormatAssign(&myIndex, &format, hasColor,
							    hasAlpha, hasSSB, &aDCI, usePIP);
			  cmpFormat = &format;
			  err = M2TXIndex_Compress(&myIndex, cmpFormat, rle, &aDCI, size, 
						   newTexel);
			  if (*size < bestSize)
			    {
			      bestSize = *size;
			      best = k;
			      bestColor = color;
			    }
			  M2TXDCI_GetTexFormat(newDCI,k,&txFormat);
			  M2TXDCI_GetColorConst(newDCI,k,&color);
			  M2TXDCI_SetTexFormat(&aDCI,k,txFormat);
			  M2TXDCI_SetColorConst(&aDCI,k,color);
			}
		    }
		  if (bestSize < origSize)
		    {
		      origSize = *size;
		      M2TXDCI_GetTexFormat(&aDCI, 0, &txFormat);
		      M2TXFormat_SetADepth(&txFormat, 0);			  
		      M2TXDCI_SetTexFormat(newDCI, best, txFormat);
		      M2TXDCI_SetColorConst(newDCI, best, bestColor);
		    }
		}
	    }
	}
      else if (bestPIP || customPIP)
	M2TXIndex_ReorderToPIP(&myIndex, newPIP, cDepth);		
      if (bestDCI || customDCI)
	{
	  if (usePIP == NULL)					/* 1.05 */
	    return(M2E_BadPtr);		  /* 1.05 */
	  if (aDepth == 0)
	    M2TXIndex_Init(&format, myIndex.XSize, myIndex.YSize, TRUE, TRUE, TRUE);
	  err = M2TXIndex_SmartFormatAssign(&myIndex, &format, hasColor, hasAlpha,
					    hasSSB, newDCI, usePIP);
	  if (err != M2E_NoErr)
	    return (err);
	  cmpFormat = &format;
	} 
      err = M2TXIndex_Compress(&myIndex, cmpFormat, rle, newDCI, size, newTexel);
      if (cmpFormat != NULL)
	err = M2TXIndex_Free(&format);							
      err = M2TXIndex_Free(&myIndex);							
    } 

  if (texelIsTemp)
    M2TXHeader_FreeLODPtr(&(tempTex.Header),lod);
		
  return(M2E_NoErr);
}

void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   UTF Compress\n");
  printf("   -rle \tRun-length encoding only (No multi-texel compression).\n");
  printf("   -lockedPIP \tMulti-texel compression w/o sorting PIP entries.\n");
  printf("   -best \tDo everything possible(default).\n");
}

int main( int argc, char *argv[] )
{
  M2TX tex;
  M2TXTex texel;
  M2TXHeader *header;
  M2TXPIP *oldPIP, newPIP;
  M2TXDCI *oldDCI, newDCI;
  char fileIn[256];
  char fileOut[256];
  bool hasPIP;
  uint32 cmpSize;
  int comprType=M2CMP_Auto;
  int compression, argn;
  uint8 numLOD, lod;
  FILE *fPtr;
  M2Err err;
  
#ifdef M2STANDALONE
  printf("Enter: <FileIn> <FileOut>\n");
  printf("Example: dumb.utf dumb.cmp.utf\n");
  fscanf(stdin,"%s %s",fileIn, fileOut);
#else
  /* Check for command line options. */
  if (argc < 3)
    {
      fprintf(stderr,"Usage: %s <Input File> [-rle|-best|-lockedPIP] <Output File>\n",argv[0]);
      print_description();
      return(-1);	
    }	
  else
    {
      strcpy(fileIn, argv[1]);
      argn = 2;
      while ((argn <argc) && (argv[argn][0] == '-') && (argv[argn][1] != '/0'))
        {
	  if (strcmp(argv[argn], "-rle")==0)
	    {
	      comprType = M2CMP_RLE | M2CMP_LockPIP;
	      argn++;
	    }	
	  else if (strcmp(argv[argn], "-best")==0)
	    {
	      comprType = M2CMP_Auto;
	      argn++;
	    }	
	  else if (strcmp(argv[argn], "-lockPIP")==0)
	    {
	      comprType = M2CMP_Auto | M2CMP_LockPIP;
	      argn++;
	    }
	  else
	    {
	      fprintf(stderr,"Usage: %s <Input File> [-rle|-best|-lockedPIP] <Output File>\n",argv[0]);
	      return(-1);
	    }	
        }
      if (argn != argc)
        {
	  strcpy(fileOut, argv[argn]);
	}
      else
       	{
	  fprintf(stderr,"Usage: %s <Input File> [-rle|-best|-lockedPIP] <Output File>\n",argv[0]);
	  return(-1);
       	}	
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
  
  err = M2TX_ReadFile(fileIn,&tex);
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Bad file.\n");
      return(-1);
    }
  M2TX_GetPIP(&tex,&oldPIP);
  M2TX_GetHeader(&tex,&header);
  
  lod = 0;
  M2TX_GetDCI(&tex, &oldDCI);
  /* Auto will find the best DCI and reorder the PIP */
  err = M2TX_CompressWithAlpha(&tex,lod,&newDCI,&newPIP,comprType,&cmpSize,&texel);  
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:During compression, error=%d\n",err);
      return(-1);
    }
  M2TXHeader_GetNumLOD(header, &numLOD);
  M2TXHeader_FreeLODPtr(header, lod);		/* Free up the old LOD ptr */
  M2TXHeader_SetLODPtr(header, lod, cmpSize, texel);  /* Replace it with the new */
  
  compression = comprType | M2CMP_CustomDCI;
  if (!(comprType & M2CMP_LockPIP))
    compression |= M2CMP_CustomPIP;
  for (lod=1; lod<numLOD; lod++) 
    {	/* Compress remaining LODs with the new DCI and new PIP computed by previous compress */
      err = M2TX_Compress(&tex, lod, &newDCI, &newPIP, compression, &cmpSize, &texel); 
      if (err != M2E_NoErr)
	{
	  fprintf(stderr,"ERROR:During compression, error=%d\n",err);
	  return(-1);
	}
      M2TXHeader_FreeLODPtr(header, lod);	/* Free up the old LOD ptr */
      M2TXHeader_SetLODPtr(header, lod, cmpSize, texel);  /* Replace it with the new */
    }
  
  M2TXHeader_SetFIsCompressed(header, TRUE);	 /* Update the texture flags */
  M2TXHeader_GetFHasPIP(header, &hasPIP);
  M2TX_SetDCI(&tex, &newDCI);
  if ((hasPIP)&&(!(comprType&M2CMP_LockPIP)))
    M2TX_SetPIP(&tex, &newPIP);
  M2TX_WriteFile(fileOut,&tex);    		/* Write it to disk */
  M2TXHeader_FreeLODPtrs(header);
  
  return (0);
}

