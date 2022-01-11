/*
	File:		psdtoutf.c

	Contains:	Converts a Photoshop to a UTF file

	Written by:	Todd Allendorf 

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <2>	 7/15/95	TMA		Changed 0,0 to be the UPPER left corner of the image. Added
									autodocs.
		 <1>	 5/31/95	TMA		first checked in
	To Do:
*/


/**
|||	AUTODOC -public -class tools -group m2tx -name psdtoutf
|||	Convert a Photoshop 3.0 file into a UTF file.
|||	
|||	  Synopsis
|||	
|||	    psdtoutf <input file> <output file>
|||	
|||	  Description
|||	
|||	    This tool converts all the components of a Photoshop 3.0 file into a UTF file. 
|||	    The first color channel(s) get mapped to a color channels of a UTF file.
|||	    If additional channels are present, the next one is mapped into a
|||	    alpha channel in the UTF file.  If yet another channel is present,
|||	    it is convertd into an SSB channel in the UTF file.
|||	
|||	  Arguments
|||	
|||	    <input file>
|||	        The input Photoshop 3.0 texture.
|||	    <output file>
|||	        The resulting UTF texture
|||	
|||	  See Also
|||	
|||	    ppmtoutf, tifftoutf
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "M2TXlib.h"
#include "qmem.h"

/* byte order independent read/write of shorts and longs. */

static uint16 getShort(FILE *fPtr)
{
  uint8 buf[2];
  long count;
  
  count = 2;
  count = fread(buf, 1, count, fPtr);
  return( (buf[0] << 8) + (buf[1] <<0)); 
}

static uint32 getLong(FILE *fPtr)
{
  uint8 buf[4];
  long count;
  
  count = 4;
  count = fread(buf, 1, count, fPtr);
  return (((uint32) buf[0]) << 24) + (((uint32) buf[1]) << 16)
    + (((uint32) buf[2]) << 8) + buf[3];
}

/*
static uint8 oneExtend(uint8 depth, uint8 run)
{
	uint8 temp;
	
	temp = 0xFF << depth;
	if (0x01 & run)
		return ( (run << (8-depth)) + (temp >> depth));
	else 
		return (run << (8-depth));
}
*/

static void ExpandRow(uint8 *rawPtr, uint32 xSize, uint8 depth, bool extend, 
		      uint8  *rleBuffer, bool isSSB)
{
  MemQB_Vars;
  uint32 i, lineCount;
  uint32 runLength;
  signed char control;
  uint8 run;
  
  
  /*  BeginMemGetBits (rleBuffer); */
  if (((int)rleBuffer)%4)
      fprintf(stderr,"rleBuffer not quad-byte aligned =%x. Probably bad data\n",
	      rleBuffer);
  BeginMemQGetBits((uint32 *)rleBuffer);

  lineCount=0;
  while (xSize > lineCount)
    {
      /*      control = qMemGetBits(8);  */    /* Get the control byte */
      MemQGetBits(8); control = QBResult;
      if (control<0)
	{
	  runLength = -control +1;
	  /* run = qMemGetBits(depth); */     /* Get the control byte */
	  MemQGetBits(depth); run = QBResult;
	  if (extend)
	    run = oneExtend(depth,run);
	  if (isSSB==TRUE)
	    run = (run>127) ? 0xFF : 0;
	  for (i=0; i<runLength; i++)
	    rawPtr[lineCount++] = run;
	}
      else
	{
	  runLength = control + 1;
	  for (i=0; i<runLength; i++)
	    {
	      /* run = qMemGetBits(depth); */     /* Get the control byte */
	      MemQGetBits(depth); run = QBResult;
	      if (extend)
		run = oneExtend(depth,run);
	      if (isSSB==TRUE)
		run = (run>127) ? 0xFF : 0;
	      rawPtr[lineCount++] = run;
	    }	
	}
    }
  /*  EndMemGetBits(); */
  EndMemQGetBits();
}

static M2Err psdImageLoad(FILE *fPtr, M2TX *tex, uint8 numLODs)
{
  int16 imageVer, channels, depth, mode, compression;
  uint32  rleBufLen, imageSig, xSize, ySize, tmpSize;
  char  reserved[6];
  unsigned char  *rleData, *tmpBuffer;
  int numColors;
  uint16 *scanLBC;    /* Scanline byte counts */
  bool rle, hasSSB, hasColor, hasAlpha, isLiteral, origHasColor, origHasAlpha, origHasSSB;
  uint32 i,j,k, curPixel;
  uint8 cDepth, aDepth, ssbDepth;
  M2TXRaw lod1;
  M2TXRaw raw;
  M2TXIndex index, lodI1;
  M2TXPIP pip;
  M2Err err;
  M2TXColor color;
  M2TXHeader *header;
  uint32 AdobeSig = '8BPS';
  long count;
  uint8 *curChannel;
  bool extendBits, isSSB;
  uint8 run, red, green, blue, alpha ,ssb;
 
  /* read header information from stream */
  
  count = 4;
  count = fread(&imageSig, 1, count, fPtr);
  if (imageSig != AdobeSig)
  {
  	fclose(fPtr);
  	return (M2E_BadFile);
  }
  imageVer = getShort(fPtr);
  if (imageVer != 1)
  {
  	fclose(fPtr);
  	return (M2E_BadFile);
  }
  count = 6;
  count = fread(reserved, 1, count, fPtr);
  channels = getShort(fPtr);
  ySize = getLong(fPtr);
  xSize = getLong(fPtr);
  depth = getShort(fPtr);
  mode = getShort(fPtr);
  
  aDepth = cDepth = ssbDepth = 0;
  hasAlpha = hasSSB = FALSE;
  hasColor = TRUE;

	if ((mode ==0) || (mode==2))
		extendBits = FALSE;
  	else
  		extendBits = TRUE;

  switch (mode)
  {
 	case 0:
 	case 1:
 	case 2:
 	case 8:
 	{
  		isLiteral = FALSE;
 		if (channels>1)
			hasAlpha = TRUE;
		if (channels>2)
			hasSSB = TRUE;
 		break;
  	}
	case 3:
	{
		isLiteral = TRUE;
		if (channels>3)
			hasAlpha = TRUE;
		if (channels>4)
			hasSSB = TRUE;
		break;
	}
	case 7:
	{
		if (channels<3)
		{
			isLiteral = FALSE;
			if (channels==2)
				hasAlpha = TRUE;    /* Assume an indexed and an Alpha channel */
		}
		else
		{ 
			isLiteral = TRUE;
			if (channels>3)
				hasAlpha = TRUE;
			if (channels>4)
				hasSSB = TRUE;
		}
		break;
	}
 	case 4:
 	case 9:
  	default:
 	{
	   	fclose(fPtr);
	   	fprintf(stderr,"ERROR:Unsupported Photoshop image type.  CMYK and Lab Color are not supported.\n");
  		return (M2E_BadFile);
  		break;
 	}
  }

  if (!isLiteral)
    {
      err = M2TXIndex_Init(&index, xSize, ySize, hasColor, hasAlpha, hasSSB);
    }
  else 
    {
      err = M2TXRaw_Init(&raw, xSize, ySize, hasColor, hasAlpha, hasSSB);
    }
  if (err != M2E_NoErr)
    return (err);
  
  tmpSize = getLong(fPtr);                 
  if (tmpSize >0)
    {
      tmpBuffer = (unsigned char *)malloc(tmpSize);
      if (tmpBuffer == NULL)
	{
	  fclose(fPtr);
	  if (isLiteral)
	    M2TXRaw_Free(&raw);
	  else
	    M2TXIndex_Free(&index);		
	  return (M2E_NoMem);
	}
      count = tmpSize;
      count = fread(tmpBuffer, 1, count, fPtr);
      if (mode == 2)  /* Indexed colors only have a PIP */
	{
	  numColors = tmpSize/3;
	  M2TXPIP_SetNumColors(&pip, numColors);
	  for (i=0; i<numColors; i++)
	    {
	      color = M2TXColor_Create(0,0xFF,tmpBuffer[i],tmpBuffer[i],tmpBuffer[i]);
	      M2TXPIP_SetColor(&pip,i,color);  	
	    }
	  for(i=0; i<numColors; i++)
	    {
	      err = M2TXPIP_GetColor(&pip, i, &color);
	      M2TXColor_Decode(color, &ssb, &alpha, &red, &green, &blue);
	      green = tmpBuffer[numColors+i];
	      color = M2TXColor_Create(0,0xFF,red,green,blue);
	      M2TXPIP_SetColor(&pip,i,color);  	
	    }
	  for (i=0; i<numColors; i++)
	    {
	      err = M2TXPIP_GetColor(&pip, i, &color);
	      M2TXColor_Decode(color, &ssb, &alpha, &red, &green, &blue);
	      blue = tmpBuffer[2*numColors+i];
	      color = M2TXColor_Create(0,0xFF,red,green,blue);
	      M2TXPIP_SetColor(&pip,i,color);  	
	    }
	}
      free(tmpBuffer); 
    }                          
  tmpSize = getLong(fPtr);
  if (tmpSize >0)
    {
      tmpBuffer = (unsigned char *)malloc(tmpSize);
      if (tmpBuffer == NULL)
  	{
	  fclose(fPtr);
	  if (isLiteral)
	    M2TXRaw_Free(&raw);
	  else
	    M2TXIndex_Free(&index);		
	  return (M2E_NoMem);
	}
      count = tmpSize;
      count = fread(tmpBuffer, 1, count, fPtr);
      free(tmpBuffer);
    }
  tmpSize = getLong(fPtr);
  if (tmpSize >0)
    {	
      tmpBuffer = (unsigned char *)malloc(tmpSize);
      if (tmpBuffer == NULL)
  	{
	  fclose(fPtr);
	  if (isLiteral)
	    M2TXRaw_Free(&raw);
	  else
	    M2TXIndex_Free(&index);		
	  return (M2E_NoMem);
	}
      count = tmpSize;
      count = fread(tmpBuffer, 1, count, fPtr);
  	free(tmpBuffer);
    }
  compression = getShort(fPtr);                /* Get compression types */
  
  if (compression == 0)
    rle = FALSE;
  else 
    rle = TRUE;
  
  if (rle)
    {          
      rleBufLen = 2 * xSize + 10;
      rleData = (uint8 *)malloc(rleBufLen);
      scanLBC = (uint16 *)malloc(ySize * channels*2);
      if ((scanLBC == NULL) || (rleData==NULL))
 	{
	  fclose(fPtr);
	  if (isLiteral)
	    M2TXRaw_Free(&raw);
	  else
	    M2TXIndex_Free(&index);		
	  return (M2E_NoMem);
  	}
      for (i=0; i<ySize*channels; i++)
		scanLBC[i] = getShort(fPtr);
	for (i=0; i<channels; i++)
	{	
	  isSSB = FALSE;
		if (i>4)
			break;
		if (isLiteral)
		{
			switch(i)
			{
				case 0:
					curChannel =  raw.Red;
					break;
				case 1:
					curChannel =  raw.Green;  
				 	break;
			 	case 2:
					curChannel =  raw.Blue;  
					break;
			 	case 3:
					curChannel =  raw.Alpha;  
					break;
			 	case 4:
					curChannel =  raw.SSB;  
					isSSB = TRUE;
					break;
				default:
					curChannel =  raw.Red;  
					break;
			}
		}
		else
		{
			switch(i)
			{
				case 0:
					curChannel =  index.Index;  
				 	break;
			 	case 1:
					curChannel =  index.Alpha;  
					break;
			 	case 2:
					curChannel =  index.SSB;  
					break;
				default:
					curChannel = index.Index;
					break;
			}		
		}
		for (j=0; j<ySize; j++)
		{
			count = scanLBC[i*ySize+j];
			count = fread(rleData, 1, count, fPtr);
			ExpandRow(curChannel, xSize, depth, extendBits, rleData, isSSB);
			curChannel += xSize;
		}
			
	}
	  free(rleData);
	  free(scanLBC);
	}
  /* end of RLE case */ 
  else 
    {
    	tmpSize = channels*xSize*ySize*depth/8;
    	tmpSize +=16;
	tmpBuffer = (unsigned char *)malloc(tmpSize);
	if (tmpBuffer == NULL)
	  {
	    fclose(fPtr);
	    if (isLiteral)
	      M2TXRaw_Free(&raw);
	    else
	      M2TXIndex_Free(&index);		
	    return (M2E_NoMem);
	  }
	count = tmpSize;
	count = fread(tmpBuffer, 1, count, fPtr);
	BeginMemGetBits (tmpBuffer);

    	for (i=0; i<channels; i++)
	  {	
	    isSSB = FALSE;
	    if (i>4)
	      break;
	    if (isLiteral)
			{
			  switch(i)
			    {
			    case 0:
			      curChannel =  raw.Red;  
			      break;
			    case 1:
			      curChannel =  raw.Green;  
			      break;
			    case 2:
			      curChannel =  raw.Blue;  
			      break;
			    case 3:
			      curChannel =  raw.Alpha;  
			      break;
			    case 4:
			      curChannel =  raw.SSB;
			      isSSB = TRUE;
			      break;
			    default:
			      curChannel =  raw.Red;  
			      break;
			    }
			}
	    else
	      {
		switch(i)
		  {
		  case 0:
		    curChannel =  index.Index;  
		    break;
		  case 1:
		    curChannel =  index.Alpha;  
		    break;
		  case 2:
		    curChannel =  index.SSB;  
		    isSSB = TRUE;
		    break;
		  default:
		    curChannel = index.Index;
		    break;
		  }		
	      }
	    curPixel = 0;
	    for (j=0; j<ySize; j++)
	      {
		for (k=0; k<xSize; k++)
		  {
		    run = qMemGetBits(depth);      /* Get the control byte */
		    if (extendBits)
		      run = oneExtend(depth,run);
		    if (isSSB==TRUE)
		      run = (run>127) ? 0xFF : 0 ;
		    curChannel[curPixel] = run;
		    curPixel++;
		  }	
	      }
	  }
    	
    	free(tmpBuffer);    	
	fclose(fPtr);
	
      }
  
  
  /* got the raw image data convert it to K9 Image format */
  
  M2TX_GetHeader(tex,&header);
  if (isLiteral)
    {	
      if (depth <= 5) 
	cDepth = 5;
      else
	cDepth = 8; 
    }
  else
    {
      if ((mode != 1) && (mode != 7))    /* Greyscale and Duotone are not literal but they are eight bit */
	cDepth = depth;
      else
	cDepth = 8;
    }
  if (hasAlpha)
    {
      if (depth <= 4) 
	aDepth = 4;
      else
	aDepth = 7;
    }
  
  if (hasSSB)
    ssbDepth = 1;
  else
    ssbDepth = 0;
  
  M2TXHeader_SetFIsLiteral(header, isLiteral);
  M2TXHeader_SetFHasSSB(header, hasSSB);  
  M2TXHeader_SetFHasPIP(header,!isLiteral);
  
  M2TXHeader_SetFIsCompressed(header,FALSE);
  M2TXHeader_SetFHasColor(header,hasColor);
  
  if (xSize % (1<<(numLODs-1)))
    return (M2E_Range);
  if (ySize % (1<<(numLODs-1)))
    return (M2E_Range);
  
  M2TXHeader_SetNumLOD(header,numLODs);
  M2TXHeader_SetMinXSize(header, xSize>>(numLODs-1));
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Bad xSize, aborting!\n");
      return(err);
    }
  M2TXHeader_SetMinYSize(header, ySize>>(numLODs-1));
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Bad xSize, aborting!\n");
      return(err);
    }

  origHasColor = hasColor;
  origHasAlpha = hasAlpha;
  origHasSSB = hasSSB;
  
  if (isLiteral)
    {
      if (!(M2TX_IsLegal(cDepth, aDepth, ssbDepth, TRUE)))
	{
	  if (hasAlpha)    /* Literal formats, not many choices */
	    {
	      cDepth = 8;
	      hasColor = TRUE;
	      aDepth = 7;
	      hasAlpha = TRUE;
	      hasSSB = TRUE;
	    }
	  else
	    {
	      if (hasSSB)
		{
		  cDepth = 8;
		  hasColor = TRUE;
		  aDepth = 7;
		  hasAlpha = TRUE;
		}
	      else if (cDepth==5)
		hasSSB=TRUE;			
	    }
	}
	}
  else	
    {
      while (!(M2TX_IsLegal(cDepth, aDepth, ssbDepth, FALSE)))
      	{
	  if (cDepth > 8)
	    break;
	  if (!hasSSB)
	    {
	      if (M2TX_IsLegal(cDepth, aDepth, 1, FALSE))
		{
		  hasSSB = TRUE;
		  ssbDepth = 1;
		  break;
		}
	    }
	  if ((hasAlpha) && (aDepth==4))
	    {
	      if (M2TX_IsLegal(cDepth, 7, ssbDepth, FALSE))
		{
		  aDepth = 7;
		  break;
		}      		
	    }
	  cDepth++;
      	}
    }
  if (cDepth > 8)
    {
      fprintf(stderr,"ERROR:Can't find a valid UTF texture format for this file\n");
      if (isLiteral)
	M2TXRaw_Free(&raw);
      else
	M2TXIndex_Free(&index);		
      return (M2E_BadFile);
    }
  
  M2TXHeader_SetCDepth(header,cDepth);
  M2TXHeader_SetADepth(header,aDepth);
  M2TXHeader_SetFHasSSB(header, hasSSB);
  M2TXHeader_SetFHasAlpha(header,hasAlpha);
  M2TXHeader_SetFHasColor(header,hasColor);

  if (!isLiteral)
    {
      if ((mode == 1) || (mode == 8))
	{
	  for (i=0; i<256; i++)
	    {
	      color = M2TXColor_Create(0,0xFF,i,i,i);
	      M2TXPIP_SetColor(&pip,i,color);
	    }	
	  M2TXPIP_SetNumColors(&pip, 256);
	}
      else if (mode == 0)
	{
	  color = M2TXColor_Create(0,0xFF,0x00,0x00,0x00);
	  M2TXPIP_SetColor(&pip,0,color);
	  color = M2TXColor_Create(0,0xFF,0xFF,0xFF,0xFF);
	  M2TXPIP_SetColor(&pip,1,color);
	  M2TXPIP_SetNumColors(&pip, 2);
	}
      M2TX_SetPIP(tex,&pip);
    }
  
  if (!isLiteral )
    {
      M2TXIndex_ToUncompr(tex, &pip, 0, &index);
    }
  else
    {
      M2TXRaw_ToUncompr(tex, NULL, 0, &raw);
    }
  for (i=1; i<numLODs; i++)
    {
      xSize = xSize >> 1;
      ySize = ySize >> 1;
      if (isLiteral)
	M2TXRaw_Init(&lod1, xSize, ySize, hasColor, hasAlpha, hasSSB);
      else
	M2TXIndex_Init(&lodI1, xSize, ySize, hasColor, hasAlpha, hasSSB);
      
      if (origHasColor)
	{
	  if (!isLiteral)
	    err = M2TXIndex_LODCreate(&index, M2SAMPLE_POINT, M2Channel_Index, &lodI1);
	  else
	    err = M2TXRaw_LODCreate(&raw, M2SAMPLE_LANCZS3, M2Channel_Colors, &lod1);
	}
      if (origHasAlpha)
	{
	  if (!isLiteral)
	    err = M2TXRaw_LODCreate(&raw, M2SAMPLE_LANCZS3, M2Channel_Alpha, &lod1);
	  else
	    err = M2TXIndex_LODCreate(&index, M2SAMPLE_LANCZS3, M2Channel_Alpha, &lodI1);
	}	
      if (origHasSSB)
	{
	  if (!isLiteral)
	    err = M2TXRaw_LODCreate(&raw, M2SAMPLE_POINT, M2Channel_SSB, &lod1);
	  else
	    err = M2TXIndex_LODCreate(&index, M2SAMPLE_POINT, M2Channel_SSB, &lodI1);
	}	
      if (isLiteral)		
	{
	  M2TXRaw_ToUncompr(tex, NULL, i, &lod1); 
/*	  M2TXRaw_Free(&lod1); */
	}
      else
	{
	  M2TXIndex_ToUncompr(tex, &pip, i, &lodI1); 
/*	  M2TXIndex_Free(&lodI1); */
	}
    }
/*
  if (isLiteral)
    M2TXRaw_Free(&raw);
  else
    M2TXIndex_Free(&index); 
*/
  
  return (M2E_NoErr);
}

void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   Photoshop 2.5 and 3.0 to UTF\n");
}

int main( int argc, char *argv[] )
{
  static M2TX tex;
  M2Err err;
  M2TXHeader *header;
  uint8 numLODs;
  FILE *fPtr;
  char fileIn[256];
  char fileOut[256];
  
#ifdef M2STANDALONE
  printf("Enter: <FileIn> <FileOut>\n");
  printf("Example: dumb.utf dumb.cmp.utf\n");
  fscanf(stdin,"%s %s",fileIn, fileOut);
#else
  /* Check for command line options. */
  if (argc != 3)
    {
      fprintf(stderr,"Usage: %s <Input File> <Output File>\n",argv[0]);
      print_description();
      return(-1);	
    }	
  else
    {
      strcpy(fileIn, argv[1]);
      strcpy(fileOut, argv[2]);
    }
#endif
  
  fPtr = fopen(fileIn, "r");
  if (fPtr == NULL)
    {
      fprintf(stderr,"ERROR:Can't open file \"%s\" \n",fileIn);
      return(-1);
    }
  
  numLODs = 1;
  M2TX_Init(&tex);
  err =  psdImageLoad(fPtr, &tex, numLODs);
  if (err != M2E_NoErr)
    {
      if (err == M2E_Range)
	fprintf(stderr,"ERROR:Range error\n");
      else
	{
	  fprintf(stderr,"ERROR:Had trouble reading the file.\n");
	  fprintf(stderr,"See if data fork was corrupted.\n");
	}
      return(-1);
    }
  M2TX_WriteFile(fileOut, &tex);
  M2TX_GetHeader(&tex,&header);
  M2TXHeader_FreeLODPtrs(header);
  return (0);
}
