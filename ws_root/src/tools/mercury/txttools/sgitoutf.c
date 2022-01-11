/*
	File:		sgitoutf.c

	Contains:	Converts an SGI image file to a UTF file

	Written by:	Todd Allendorf 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <2>	10/16/95	TMA		Autodoc fix.
	To Do:
*/


/**
|||	AUTODOC -public -class tools -group m2tx -name sgitoutf
|||	Convert an SGI image file into a UTF file.
|||	
|||	  Synopsis
|||	
|||	    sgitoutf <input file> <output file>
|||	
|||	  Description
|||	
|||	    This tool converts all the components of an SGI image file into a UTF file. 
|||	    The color channel(s) get mapped to color channels of a UTF file.
|||	    If an alpha channel is presentit is mapped to the alpha channel in
|||	    the UTF file. 
|||	
|||	  Arguments
|||	
|||	    <input file>
|||	        The input SGI texture.
|||	
|||	  See Also
|||	
|||	    ppmtoutf, psdtoutf
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "M2TXlib.h"
#include "qstream.h"

#define IMAGIC 	0732
#define BPPMASK			0x00ff
#define ITYPE_VERBATIM		0x0000
#define ITYPE_RLE		0x0100
#define ISRLE(type)		(((type) & 0xff00) == ITYPE_RLE)
#define ISVERBATIM(type)	(((type) & 0xff00) == ITYPE_VERBATIM)
#define BPP(type)		((type) & BPPMASK)
#define RLE(bpp)		(ITYPE_RLE | (bpp))
#define VERBATIM(bpp)		(ITYPE_VERBATIM | (bpp))

#define RINTLUM (79)
#define GINTLUM (156)
#define BINTLUM (21)
#define ILUM(r,g,b)     ((int)(RINTLUM*(r)+GINTLUM*(g)+BINTLUM*(b))>>8)

#define TAGLEN	(5)

#ifndef SEEK_SET
#define SEEK_SET 0
#endif


/* byte order independent read/write of shorts and longs. */

static uint16 getShort(FILE *theFile)
{
  uint8 buf[2];
  
  qReadByteStream(theFile, (BYTE *)buf,  2);
  return( (buf[0] << 8) + (buf[1] <<0)); 
}

static uint32 getLong(FILE *theFile)
{
  uint8 buf[4];
  
  qReadByteStream(theFile, (BYTE *)buf, 4);
  return (((uint32) buf[0]) << 24) + (((uint32) buf[1]) << 16)
    + (((uint32) buf[2]) << 8) + buf[3];
}

static M2Err readTab(FILE *theFile, uint32 *tab, int len)
{
  while (len>0) 
    {
      *tab++ = getLong(theFile);
      len -= 4;
    }
    return(M2E_NoErr);
}

static void expandRow(unsigned char *oPtr, unsigned char *iPtr)
{
  unsigned char pixel, count;
  int lineCount = 0;

  while (1)
    {
      pixel = *iPtr++;
      if (!(count = (pixel & 0x7f)))
	return;
      lineCount += count;
      if (pixel & 0x80)
	{
	  while (count > 0)
	    {
	      *oPtr++ = *iPtr++;
	      count--;
	    }	
	}
      else
	{
	  pixel = *iPtr++;
	  while (count > 0)
	    {
	      *oPtr++ = pixel;
	      count--;
	    }
	}
    }
}

static M2Err rgbImageLoad(char *fname, M2TX *tex, uint8 numLODs)
{
  int16 imageIMagic, imageType, imageDim, xSize, ySize, zSize;
  int32  rleBufLen, y, z;
  uint32 *startTab, *lengthTab, tabLen;
  unsigned char *dataPtr, *rleData, *base;
  char *fileName;
  int trunc;
  long fileSize;
  bool rle, badOrder;
  int bpp, cur;
  uint16 i;
  FILE *theFile;
  M2TXIndex index, lodI1;
  M2TXPIP *pip;
  M2Err err;
  M2TXColor color;
  M2TXHeader *header;
  M2TXRaw lod1;
  M2TXRaw raw;
  uint8 aDepth, cDepth, ssbDepth = 0;
  bool isLiteral, hasAlpha, hasColor, hasSSB = FALSE;
 
  fileName = fname;
  theFile = (FILE *)qOpenReadFile(fname);
  if(theFile ==NULL)
    {
      /* GLIB_ERROR(("Can not open image file.\n")); */
      return (M2E_BadFile);
    }
  
  trunc = 0;
  
  fileSize = qGetFileSize(theFile);
  
  /* read header information from stream */
  
  imageIMagic = getShort(theFile);
  imageType = getShort(theFile);
  imageDim = getShort(theFile);
  xSize = getShort(theFile);
  ySize = getShort(theFile);
  zSize = getShort(theFile);
  
  if (zSize==1)
    err = M2TXIndex_Alloc(&index, xSize, ySize);
  else 
    err = M2TXRaw_Alloc(&raw, xSize, ySize);

	if (err != M2E_NoErr)
		return (err);

  /*  if (K9_ErrorOnStream(stream)) {
      qCloseReadFile(theFile);
      GLIB_WARNING(("error in header info: %s", fname));
      return NULL;
      }  
      */
  
  if (imageIMagic != IMAGIC)
    {
      qCloseReadFile(theFile);
      /*     GLIB_WARNING(("bad magic number: %s", fname)); */
      return (M2E_BadFile);
    }
  
  rle = ISRLE(imageType);
  bpp = BPP(imageType);   
  if (bpp != 1)
    return (M2E_Range);
  
  if (rle)
    {          
      rleBufLen = 2 * xSize + 10;
      tabLen = ySize * zSize * sizeof(uint32);
      
      startTab = (uint32 *) malloc(tabLen);
      lengthTab = (uint32 *) malloc(tabLen);
      rleData = (unsigned char *) malloc(rleBufLen);
      
      if ((startTab==NULL) || (lengthTab==NULL) || (rleData == NULL))
	return (M2E_NoMem);
      /*  GLIB_ERROR(("out of memory in gdev_rgb_image_load()")); */
      
      fseek(theFile, 512, SEEK_SET);
      err = readTab(theFile, startTab, tabLen);
      err += readTab(theFile, lengthTab, tabLen);
      
      if (err != M2E_NoErr)
	{
	  /* loaderr = "error reading scanline tables"; */
	  free(startTab);
	  free(lengthTab);
	  free(rleData);
	  return (M2E_BadFile);
	}
      /* check data order */
      cur = 0;
      badOrder = 0;
      for (y = ySize-1; (y >= 0) && !badOrder; y--)
	{
	  for (z = 0; z < zSize && !badOrder; z++) 
	    {
	      if (startTab[y + z * ySize] < cur)
		badOrder = 1;
	      else
		cur = startTab[y + z * ySize];
	    }
	}
      
      fseek(theFile, 512 + 2 * tabLen, SEEK_SET);
      cur = 512 + 2 * tabLen;
      
      if (badOrder)
	{
	  for (z = 0; z < zSize; z++)
	    {
	      if (zSize == 1)
		base = index.Index;
	      else
		switch (z)
		  {
		  case 0:
		    base = raw.Red;
		    break;
		  case 1:
		    base = raw.Green;
		    break;
		  case 2:
		    base = raw.Blue;
		    break;
		  case 3:
		    base = raw.Alpha;
		    break;
		  }
	      dataPtr = base;
	      for (y = ySize-1; (y >=0); y--)
		{
		  if (cur != startTab[y+z*ySize])
		    {
		      fseek(theFile, startTab[y+z*ySize],SEEK_SET);
		      cur = startTab[y+z*ySize];
		    }
		  if (lengthTab[y+z*ySize] > rleBufLen)
		    {
		      /* GLIB_ERROR(("rgbimageload() - rlebuf too small")); */
		      err = M2E_Range;
		    }
		  qReadByteStream(theFile,rleData,lengthTab[y+z*ySize]);
		  cur += lengthTab[y+z*ySize];
		  expandRow(dataPtr, rleData);
		  dataPtr += xSize;
		}
	    }
	}
      else
	{
	  for (y = ySize-1; y >= 0; y--)
	    {
	      for (z = 0; z < zSize; z++)
		{
		  if (zSize == 1)
		    base = index.Index;
		  else
		    switch (z)
		      {
		      case 0:
			base = raw.Red;
			break;
		      case 1:
			base = raw.Green;
			break;
		      case 2:
			base = raw.Blue;
			break;
		      case 3:
			base = raw.Alpha;
			break;
		      }
		  dataPtr = base + (ySize-y-1)*xSize;
		  if (cur != startTab[y+z*ySize])
		    {
		      fseek(theFile, startTab[y+z*ySize], SEEK_SET);
		      cur = startTab[y+z*ySize];
		    }
		  qReadByteStream(theFile,rleData,lengthTab[y+z*ySize]);
		  cur += lengthTab[y+z*ySize];
		  expandRow(dataPtr, rleData);
		}
	    }
	}

      free(startTab);
      free(lengthTab);
      free(rleData);
    }
  /* end of RLE case */ 
  else 
    {		/* not RLE */
      err =fseek(theFile, 512, SEEK_SET);    
      for (z = 0; z<zSize; z++) 
	{
	  if (zSize == 1)
	    base = index.Index;
	  else
	    switch (z)
	      {
	      case 0:
		base = raw.Red;
		break;
	      case 1:
		base = raw.Green;
		break;
	      case 2:
		base = raw.Blue;
		break;
	      case 3:
		base = raw.Alpha;
		break;
	      }
	  dataPtr = base + (ySize-1)*xSize;
	  for (y = ySize-1; y >= 0; y--)
	    {
	      qReadByteStream(theFile, dataPtr, xSize);
	      dataPtr -= xSize;
	    }
	}
    }
  if (err == M2E_BadFile)
    {
      qCloseReadFile(theFile);
      /*   GLIB_WARNING(("%s %s", loaderr, fname));
       */
      return (err);
    }
  
  if (err==M2E_Range)
    trunc = 1;				/* probably truncated file */
  
  qCloseReadFile(theFile);
  
  /* got the raw image data convert it to K9 Image format */
  
  cDepth = 8;
  ssbDepth = 0;
  aDepth = 0;
  hasColor = TRUE;

  M2TX_GetHeader(tex,&header);
  M2TXHeader_SetMinXSize(header,xSize);
  M2TXHeader_SetMinYSize(header,ySize);  
  M2TXHeader_SetFIsCompressed(header,FALSE);
    
  if (xSize % (1<<(numLODs-1)))
    return (M2E_Range);
  if (ySize % (1<<(numLODs-1)))
    return (M2E_Range);
  
  M2TXHeader_SetNumLOD(header,numLODs);
  M2TXHeader_SetMinXSize(header, xSize>>(numLODs-1));
  M2TXHeader_SetMinYSize(header, ySize>>(numLODs-1));
  
  if (zSize>1)
    {
      isLiteral = TRUE;
      if (zSize>3)
	{
	  hasAlpha = TRUE;
	  aDepth = 7;
	}
      
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
      
      if (cDepth > 8)
	{
	  fprintf(stderr,"ERROR:Can't find a valid UTF texture format for this file\n");
	  if (isLiteral)
	    M2TXRaw_Free(&raw);
	  else
	    M2TXIndex_Free(&index);
	  return (M2E_BadFile);
	}
      
      M2TXHeader_SetFIsLiteral(header,isLiteral);      
      M2TXHeader_SetCDepth(header,cDepth);
      M2TXHeader_SetADepth(header,aDepth);
      M2TXHeader_SetFHasSSB(header, hasSSB);
      M2TXHeader_SetFHasAlpha(header,hasAlpha);
      M2TXHeader_SetFHasColor(header,hasColor);
      
      M2TXRaw_ToUncompr(tex, NULL, 0, &raw);
      for (i=1; i<numLODs; i++)
	{
	  xSize = xSize >> 1;
	  ySize = ySize >> 1;
	  M2TXRaw_Alloc(&lod1, xSize, ySize);
	  err = M2TXRaw_LODCreate(&raw, M2SAMPLE_AVERAGE, M2Channel_Colors, &lod1);
	  if (zSize>3)
	    {
	      err = M2TXRaw_LODCreate(&raw, M2SAMPLE_POINT, M2Channel_Alpha, &lod1);		
	    }		
	  M2TXRaw_ToUncompr(tex, NULL, i, &lod1); 
	  M2TXRaw_Free(&lod1);
	}
      M2TXRaw_Free(&raw);
    }
  else
    {
      isLiteral = FALSE;
      M2TXHeader_SetFHasPIP(header,TRUE);
      M2TX_GetPIP(tex,&pip);
      for (i=0; i<256; i++)
	{
	  color = M2TXColor_Create(0,0xFF,i,i,i);
	  M2TXPIP_SetColor(pip,i,color);
	}
      M2TXPIP_SetNumColors(pip, 256);
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
      if (cDepth > 8)
	{
	  fprintf(stderr,"ERROR:Can't find a valid UTF texture format for this file\n");
	  if (isLiteral)
	    M2TXRaw_Free(&raw);
	  else
	    M2TXIndex_Free(&index);
	  return (M2E_BadFile);
	}
      
      M2TXHeader_SetFIsLiteral(header,isLiteral);      
      M2TXHeader_SetCDepth(header,cDepth);
      M2TXHeader_SetADepth(header,aDepth);
      M2TXHeader_SetFHasSSB(header, hasSSB);
      M2TXHeader_SetFHasAlpha(header,hasAlpha);
      M2TXHeader_SetFHasColor(header,hasColor);
      
      M2TXIndex_ToUncompr(tex, pip, 0, &index);
      for (i=1; i<numLODs; i++)
	{
	  xSize = xSize >> 1;
	  ySize = ySize >> 1;
	  M2TXIndex_Alloc(&lodI1, xSize, ySize);
	  err = M2TXIndex_LODCreate(&index, M2SAMPLE_POINT, M2Channel_Index, &lodI1);
	  M2TXIndex_ToUncompr(tex, pip, i, &lodI1); 
	  M2TXIndex_Free(&lodI1);
	}
      M2TXIndex_Free(&index);
    }

  return (M2E_NoErr);
}

void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   SGI image files to UTF\n");
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
  err =  rgbImageLoad(fileIn, &tex, numLODs);
  if (err != M2E_NoErr)
    {
      if (err == M2E_Range)
	fprintf(stderr,"ERROR:Range error\n");
      else
	fprintf(stderr,"ERROR:An error occured during conversion.\n");
      return(-1);
    }
  M2TX_WriteFile(fileOut, &tex);
  M2TX_GetHeader(&tex,&header);
  M2TXHeader_FreeLODPtrs(header);
  return(0);
}




