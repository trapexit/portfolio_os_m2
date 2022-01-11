/*
	File:		utfcompress.c

	Contains:	Takes any utf file and compresses it as best it can	

	Written by:	Todd Allendorf 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):
		 <4>	 7/15/95	TMA		Fixed PIP being lost during index compression (sometimes).
		 <3>	  6/9/95	TMA		Fixed args bug.
		 <2>	 5/16/95	TMA		Autodocs added.
	To Do:
*/

/**
|||	AUTODOC -public -class tools -group m2tx -name utfcompress
|||	Compresses a UTF file using the desired compression type(s).
|||	
|||	  Synopsis
|||	
|||	    utfcompress <input file> [options] <output file>
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
|||	    -lockPIP
|||	        Do multi-texel compression and run-length encoding, but keep the PIP
|||	        the same.
|||	    -best
|||	        Does all of the above and also may sort the PIP to the optimal order.
|||	
|||	  Caveats
|||	    -lockPIP and -best take almost the same amount of time and can be quite
|||	    long.  A 128x128x8 bt image can take about thirty seconds on a PowerMac 
|||	    while a 256x256x8 image can take five minutes.  Lower color depths aren't
|||	    as time intensive.  It is recommended that you compress only after the
|||	    textures have been resized and carved to their final size (to fit into
|||	    TRAM <16K) to give the best compression results. I.E. 4
|||	    16K textures will compress better than one 64K texture.
|||	    BDA has the following restrictions:
|||	    1) The BDA can only load compressed textures that are 
|||	    less than 16K in their UNCOMPRESSED form.  gcomp must be
|||	    used to slice up textures that are larger than 16K when
|||	    they are uncompressed.
|||	    2) The final version of BDA can only compress Indexed 
|||	    textures.  It will not accept literal compressed textures.
|||	    Compression type -best also reorders the PIP, so if textures are expected
|||	    share a PIP, the -lockPIP option should be used or the utfmakesame tool.
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
#include <math.h>

/*
 * Returns the total bit size of data. Depth is pixel depth, {x,y}Size is
 * Min{X,Y}Size.
 */
long ComputeM2TXSize(int numLOD, double depth, int xSize, int ySize)
{

  long lod, size, totalSize;

  totalSize = 0;
  for (lod=0; lod < numLOD; lod++)
    {
      size = (long)ceil(xSize * ySize * depth);
      if (size%32)
	size = (size/32)*32+32;
      totalSize += size;
      xSize = xSize>>1;
      ySize = ySize>>1;
    }
  return(totalSize);
}

void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   UTF Compress\n");
  printf("   -rle \tRun-length encoding only (No multi-texel compression).\n");
  printf("   -lockPIP \tMulti-texel compression w/o sorting PIP entries.\n");
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
  bool hasPIP, isLiteral, hasSSB, hasAlpha, hasColor;
  uint32 cmpSize;
  int comprType=M2CMP_Auto;
  int compression, argn;
  uint8 numLOD, lod;
  uint16 xSize, ySize;
  uint8  aDepth, cDepth, ssbDepth;
  double pixelDepth;
  long size;
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
      fprintf(stderr,"Usage: %s <Input File> [-rle|-best|-lockPIP] <Output File>\n",argv[0]);
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
	      fprintf(stderr,"Usage: %s <Input File> [-rle|-best|-lockPIP] <Output File>\n",argv[0]);
	      return(-1);
	    }	
        }
      if (argn != argc)
        {
	  strcpy(fileOut, argv[argn]);
	}
      else
       	{
	  fprintf(stderr,"Usage: %s <Input File> [-rle|-best|-lockPIP] <Output File>\n",argv[0]);
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
  /* +KRS+ */
  M2TXHeader_GetNumLOD(header, &numLOD);
  M2TXHeader_GetFIsLiteral(header, &isLiteral);
  if ( isLiteral )
    fprintf(stderr, "WARNING: Compressing literal\n");
  M2TXHeader_GetFHasColor(header, &hasColor);
  if ( hasColor )
    M2TXHeader_GetCDepth(header, &cDepth);
  else
    cDepth = 0;
  M2TXHeader_GetFHasAlpha(header, &hasAlpha);
  if ( hasAlpha )
    M2TXHeader_GetADepth(header, &aDepth);
  else
    aDepth = 0;
  M2TXHeader_GetFHasSSB(header, &hasSSB);
  if ( hasSSB )
    ssbDepth = 1;
  else
    ssbDepth = 0;

  if ( isLiteral )
    pixelDepth = aDepth + 3*cDepth + ssbDepth;
  else
    pixelDepth = aDepth + cDepth + ssbDepth;

  M2TXHeader_GetMinXSize(header, &xSize);
  M2TXHeader_GetMinYSize(header, &ySize);

  size = ComputeM2TXSize(numLOD, pixelDepth, xSize, ySize);
  size /= 8;
  if (size > 16384)
    fprintf(stderr, "WARNING: Texture uncompressed size greater than 16K\n");
  /* -KRS- */
      
  lod = 0;
  M2TX_GetDCI(&tex, &oldDCI);
  /* Auto will find the best DCI and reorder the PIP */
  err = M2TX_Compress(&tex,lod,&newDCI,&newPIP,comprType,&cmpSize,&texel);  
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:During compression, error=%d\n",err);
      return(-1);
    }
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
