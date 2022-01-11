/*
	File:		utffit.c

	Contains:	Take a utf and resizes it and creates (if necessary)
	                LODs with the appropriate filters to fit in 16K	

	Written by:	Todd Allendorf 

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <6>	10/16/95	TMA		Fixed bug in which LODSize computation could get into an
									infinite loop.
		 <4>	 7/15/95	TMA		Nothing special.
		 <3>	 5/31/95	TMA		Removed unused variables.
		 <2>	 5/16/95	TMA		Autodocs added. -depth option added. utffit no longer resizes
									images that are smaller than 16K.
	To Do:
*/


/**
|||	AUTODOC -public -class tools -group m2tx -name utffit
|||	Resize a texture so that all the levels of detail combined are <= 16K.
|||	
|||	  Synopsis
|||	
|||	    utffit <input file> <new # LODs> [options] <output file>
|||	
|||	  Description
|||	
|||	    This tool takes a UTF file and resize all the levels of detail so that
|||	    their memory footprint is less than 16K.  If there are fewer levels of 
|||	    detail than are ask for, the finest level of detail is resampled using
|||	    an optional filter.  If more levels of detail exist than are asked for,
|||	    they will be ignored.
|||	
|||	  Arguments
|||	
|||	    <input file>
|||	        The input UTF texture.
|||	    <new # LODs>
|||	        The number of levels of detail in the output UTF texture.
|||	    <output file>
|||	        The resulting UTF texture.
|||	
|||	  Options
|||	
|||	    -depth <1-32>
|||	        Tell the program to compute memory usage as if the image were of the
|||	        specified depth instead of the images actual depth.
|||	    -xpow
|||	        Lock the width to a power of two
|||	    -ypow
|||	        Lock the height to a power of two.
|||	    -p
|||	        Use a percent of TRAM
|||	    -fc
|||	        Color channel filter complexity.
|||	    -fa
|||	        Alpha channel filter complexity.
|||	    -fs
|||	        SSB channel filter complexity.
|||	
|||	    The filter complexity means the following: 1=point sampling, 2=averaged,
|||	    3=weighted sampling, 4=sinc filter, 5=Lancz s3 filter, 6=Michell filter
|||	
|||	  Caveats
|||	
|||	    Indexed UTF files cannot be resampled with any other filter than point
|||	    sampling (complexity = 0).  Using other filters would introduce new colors
|||	    may not be in the PIP.  If you want to resample with other filters, use a
|||	    literal image, then color reduce it to the desired bitdepth.
|||	
|||	  Examples
|||	
|||	    utffit check.64x64.utf 4 -depth 6 check.out.utf
|||	    
|||	    Resize check.64x64.utf so that it's top four levels of detail fit into
|||	    16K (of TRAM).  Pretend that the texture has a 6 bits per texel depth.
|||	    If the image was originally twenty-four bits per texel, it could only be
|||	    one-fourth the area of a six bit per texel UTF.  This texture won't work
|||	    right (it will be greater 16K) until it is color reduced to less than or
|||	    equal to six bits per texel.
|||	    
|||	  See Also
|||	
|||	    utfmakelod, utfresize
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
  printf("   UTF Fit Levels of Detail into TRAM.\n");
  printf("   -depth 1-32\tAct as if the final texel depth were depth.\n");
  printf("   -p %\tPercent of TRAM to use.(default = 100)\n");
  printf("   -xpow\tLock x dimension to a power of two.\n");
  printf("   -ypow\tLock y dimension to a power of two.\n");
  printf("   -fc 1-6\tColor Filter Complexity. 1=Point Sampling (default = 5)\n");
  printf("   -fa 1-6\tAlpha Filter Complexity. 1=Point Sampling (default = 1)\n");
  printf("   -fs 1-6\tSSB Filter Complexity. 1=Point Sampling (default = 1)\n");
}

long ComputeLODSize(int numLOD, double depth, int xSize, int ySize)
{

  long lod, size, totalSize;
  
  
  totalSize = 0;
  for (lod=0; lod < numLOD; lod++)
    {
      size = (long)ceil(xSize*ySize*depth);
      if (size%32)
	size = (size/32)*32+32;
      totalSize += size;
      xSize = xSize>>1;
      ySize = ySize>>1;
    }
  return(totalSize);
}

int main( int argc, char *argv[] )
{
  M2TX tex, newTex;
  M2TXDCI *dci;
  M2TXRaw raw, rawLOD;
  M2TXTex texel;
  M2TXIndex index, indexLOD;
  M2TXHeader *header, *newHeader;
  M2TXPIP *oldPIP;
  char fileIn[256];
  char fileOut[256];
  bool hasPIP, isCompressed, isLiteral, hasColor, hasAlpha, hasSSB, aspectLock;
  bool skip, done;
  bool xPow = FALSE;
  bool yPow = FALSE;
  int nnLOD;
  long bufSize = 16384*8;
  uint8 numLOD, newLODs, aDepth, cDepth, ssbDepth;
  uint16 divisibleBy;
  bool override = FALSE;
  uint8 lod, lastLOD;
  uint16 xSize, ySize;
  uint32 lodXSize, lodYSize, cmpSize, curXSize, curYSize;
  FILE *fPtr;
  M2Err err;
  double ratio, lodSize, xPixels, yPixels, aspect, newAspect;
  double origXPixels, origYPixels;
  double percentage = 100.00;
  long *curFilter;
  double tmpDepth=8;
  double pixelDepth;
  long filterValue;
  int yExp, xExp;
  double fractional;
  long colorFilter = M2SAMPLE_LANCZS3;
  long indexFilter = M2SAMPLE_POINT;
  long alphaFilter = M2SAMPLE_POINT;
  long ssbFilter = M2SAMPLE_POINT;
  int argn;
  long bits;
  
  aspectLock = FALSE;
#ifdef M2STANDALONE	
  printf("Enter: <FileIn> <new # LODs> <FileOut>\n");
  printf("Example: dumb.utf 5 dumb.cmp.utf\n");
  fscanf(stdin,"%s %d %s",fileIn, &nnLOD, fileOut);
  
#else
  /* Check for command line options. */
  if (argc < 4) 
    {
      fprintf(stderr,"Usage: %s <Input File> <new # LODs> [-depth 1-32] [-fc 0-6] [-fa 0-6] [-fs 0-6] [-xpow] [-ypow] <Output File>\n",argv[0]);
      print_description();
      return(-1);    
    }
  strcpy(fileIn, argv[1]);
  nnLOD = strtol(argv[2], NULL, 10);
  if((nnLOD<1) || (nnLOD>11))
    {
      fprintf(stderr,"ERROR:Bad # of LODs specified.  Allowed range is 1 to 11, not %d\n",
	      nnLOD);
      return(-1);
    }
  argn = 3;
  while ( argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0' )
    {
      skip = FALSE;
      if ( strcmp( argv[argn], "-fc")==0 )
        {
	  ++argn;
	  curFilter = &colorFilter;
        }
      else if ( strcmp( argv[argn], "-fa")==0 )
        {
	  ++argn;
	  curFilter = &alphaFilter;
        }
      else if ( strcmp( argv[argn], "-depth")==0 )
        {
	  ++argn;
	  override = TRUE;
	  tmpDepth = atof(argv[argn]);
	  argn++;
	  skip = TRUE;
        }
      else if ( strcmp( argv[argn], "-xpow")==0 )
        {
	  ++argn;
	  xPow = TRUE;
	  skip = TRUE;
        }
      else if ( strcmp( argv[argn], "-ypow")==0 )
        {
	  ++argn;
	  yPow = TRUE;
	  skip = TRUE;
        }
      else if ( strcmp( argv[argn], "-fs")==0 )
        {
	  ++argn;
	  curFilter = &ssbFilter;
        }
      else if ( strcmp( argv[argn], "-p")==0 )
        {
	  ++argn;
	  percentage = atof(argv[argn]);
	  argn++;
	  skip = TRUE;
        }
      else
	{
	  fprintf(stderr,"Usage: %s <Input File> <new # LODs> [-depth 1-24] [-fc 0-6] [-fa 0-6] [-fs 0-6]  [-xpow] [-ypow] <Output File>\n",argv[0]);
	  return(-1);
	}
      if (!skip)
	{
      filterValue = strtol(argv[argn], NULL, 10);
      argn++;
      
      if ((filterValue > 6)||(filterValue<1))
	{
	      fprintf(stderr,"Usage: %s <Input File> <new # LODs> [-depth 1-24] [-fc 0-6] [-fa 0-6] [-fs 0-6] [-xpow] [-ypow] <Output File>\n",argv[0]);
	  return(-1);
	}
      else
	{
	  *curFilter = (1<<(filterValue));
	}	
    }
    }
  
  if ( argn != argc )
    {
      /* Open the input file. */
      strcpy( fileOut, argv[argn] );
    }
  else
    {
      /* No input file specified. */
      fprintf(stderr,"Usage: %s <Input File> <new # LODs> [-depth 1-24] [-fc 0-6] [-fa 0-6] [-fs 0-6] [-xpow] [-ypow] <Output File>\n",argv[0]);
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

  curXSize = xSize * (1<<(numLOD-1));
  curYSize = ySize * (1<<(numLOD-1));
 
  newLODs = nnLOD;
  err = M2TXHeader_SetNumLOD(newHeader, newLODs);
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Bad number of levels of detail, aborting!\n");
      return(-1);
    }
  
  ratio = 1.0;
  for (lod=1; lod<newLODs; lod++)
    ratio += pow(.25, lod);
  divisibleBy = 1<<(newLODs-1);
  if (isLiteral)
    pixelDepth = aDepth + 3*cDepth + ssbDepth;
  else
    pixelDepth = aDepth + cDepth + ssbDepth;

  if (override)   /* To allow for color reduction after the fitting */
    {
      pixelDepth = tmpDepth;
    }

  bufSize = (((long)floor(bufSize*percentage*.01))/32)*32;
  fprintf(stderr,"Buffer Size = %d bits %d bytes\n", bufSize,
	  bufSize/8);
  aspect = (double)xSize/(double)ySize;
  lodSize = bufSize / ratio;
  
  if ((xPow) && (!yPow))
    {
      origXPixels = xPixels = floor(sqrt(lodSize/(double)pixelDepth)*sqrt(aspect));
      
      if (xPixels >= curXSize)
	xPixels = curXSize;
      
      fractional = frexp (xPixels, &xExp);
      if (fractional > 0.00001)
	xPixels = 1 << (xExp);

      if (xPixels>origXPixels)
	{
	  if (fabs(xPixels-origXPixels) > fabs(origXPixels - (1<<xExp-1)))
	      xPixels = 1<< (xExp-1);
	}

      yPixels = xPixels / aspect;
      
      if ((int)yPixels % divisibleBy)
	{
	  done = FALSE;
	  do
	    {
	      yPixels = divisibleBy*((int)yPixels/divisibleBy);
	      bits = ComputeLODSize(newLODs, pixelDepth, xPixels, 
				    yPixels);
	      if (bits>bufSize)
		yPixels -= divisibleBy;
	      else
		done = TRUE;
	    } while (!done);
	}
    }
  else
    {
      origYPixels = yPixels = floor(sqrt(lodSize/(double)pixelDepth)/sqrt(aspect));
      if (yPixels >= curYSize)
	yPixels = curYSize;

      if (yPow)
	{
	  fractional = frexp (yPixels, &yExp);
	  yPixels = 1 << (yExp);

	  if (yPixels>origYPixels)
	    {
	      if (fabs(yPixels-origYPixels) > fabs(origYPixels - (1<<yExp-1)))
		yPixels = 1<< (yExp-1);
	    }
	}
      else 
	{
	  if ((int)(yPixels) % divisibleBy)
	    {
	      yPixels = divisibleBy*((int)yPixels/divisibleBy);
	    }
	}
      xPixels = yPixels * aspect;
      if (xPow)
	{
	  fractional = frexp (xPixels, &xExp);
	  done = FALSE;	  
	  do
	    {
	      xPixels = 1 << (xExp);
	      bits = ComputeLODSize(newLODs, pixelDepth, xPixels, 
				    yPixels);
	      if (bits>bufSize)
		xExp--;
	      else
		done = TRUE;
	    } while (!done);
	}
      else if ((int)xPixels % divisibleBy)
	{
	  do
	    {
	      if (((int)xPixels)%divisibleBy)
		xPixels = divisibleBy*(((int)xPixels/divisibleBy)+1);
	      bits = ComputeLODSize(newLODs, pixelDepth, xPixels, 
				    yPixels);
	      if (bits>bufSize)
		xPixels -= divisibleBy;
	      else
		done = TRUE;
	    } while (!done);
	} 
    }

  if ((yPixels <1.0) || (xPixels<1.0))
    {
      fprintf(stderr,"ERROR: Can't scale down %dX%d image to be divisible by %d (%d LODs)\n",
	      curXSize, curYSize, divisibleBy, newLODs);
      return(-1);
    }

  newAspect = xPixels/yPixels;
  printf("XPixels = %f, YPixels = %f, Old Aspect =%f New Aspect =%f\n",
	 xPixels, yPixels, aspect, newAspect);
  
  lodXSize = (int)xPixels;
  lodYSize = (int)yPixels;

  err = M2TXHeader_SetMinXSize(newHeader, lodXSize>>(newLODs-1));
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Bad xSize, aborting!\n");
      return(-1);
    }
  
  err = M2TXHeader_SetMinYSize(newHeader, lodYSize>>(newLODs-1));  
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Bad ySize, aborting!\n");
      return(-1);
    }
  
  for (lod=0; lod<newLODs; lod++)
    {
      if (lod < numLOD)
	lastLOD = lod;
      else
	lastLOD = numLOD-1;
      if (isCompressed)
	{
	  if (isLiteral)
	    err = M2TX_ComprToM2TXRaw(&tex, oldPIP, lastLOD, &raw);
	  else
	    err = M2TX_ComprToM2TXIndex(&tex, oldPIP,lastLOD, &index);
	}
      else
	{
	  if (isLiteral)
	    err = M2TX_UncomprToM2TXRaw(&tex, oldPIP, lastLOD, &raw);
	  else
	    err = M2TX_UncomprToM2TXIndex(&tex, oldPIP, lastLOD , &index);
	}
      if (err != M2E_NoErr)
	{
	  fprintf(stderr,"ERROR:Raw creation failed!\n");
	  return(-1);
	}
      
      if (isLiteral)
	{
	  if ((raw.XSize == lodXSize) && (raw.YSize == lodYSize))
	    {
	      rawLOD.Red = raw.Red;
	      rawLOD.Green = raw.Green;
	      rawLOD.Blue = raw.Blue;
	      rawLOD.Alpha = raw.Alpha;
	      rawLOD.SSB = raw.SSB;
	      rawLOD.HasColor = raw.HasColor;
	      rawLOD.HasAlpha = raw.HasAlpha;
	      rawLOD.HasSSB = raw.HasSSB;
	      rawLOD.XSize = raw.XSize;
	      rawLOD.YSize = raw.YSize;
	    }
	  else
	    {
	      err = M2TXRaw_Init(&rawLOD, lodXSize, lodYSize, hasColor, hasAlpha, 
				 hasSSB);
	      if (err != M2E_NoErr)
		{
		  fprintf(stderr,"ERROR:Allcation failed!\n");
		  return(-1);
		}
	    }
	}
      else
	{
	  if ((index.XSize == lodXSize) && (index.YSize == lodYSize))
	    {
	      indexLOD.Index = index.Index;
	      indexLOD.Alpha = index.Alpha;
	      indexLOD.SSB = index.SSB;
	      indexLOD.HasColor = index.HasColor;
	      indexLOD.HasAlpha = index.HasAlpha;
	      indexLOD.HasSSB = index.HasSSB;
	      indexLOD.XSize = index.XSize;
	      indexLOD.YSize = index.YSize;
	    }
	  else
	    {
	      err = M2TXIndex_Init(&indexLOD, lodXSize, lodYSize, hasColor, 
				   hasAlpha, hasSSB);
	      if (err != M2E_NoErr)
		{
		  fprintf(stderr,"ERROR:Allcation failed!\n");
		  return(-1);
		}
	    }
	}
      
      if (isLiteral)
	{	
	  if ((raw.XSize != lodXSize) || (raw.YSize != lodYSize))
	    {
	      if (hasColor)
		{
		  err = M2TXRaw_LODCreate(&raw, colorFilter, M2Channel_Colors,
					  &rawLOD);
		  if (err != M2E_NoErr)
		    {
		      fprintf(stderr,"ERROR:Resize failed!\n");
		      return(-1);
		    }
		}
	      if (hasAlpha)
		{
		  err = M2TXRaw_LODCreate(&raw, alphaFilter, M2Channel_Alpha, 
					  &rawLOD);
		  if (err != M2E_NoErr)
		    {
		      fprintf(stderr,"ERROR:Resize failed!\n");
		      return(-1);
		    }
		}
	      if (hasSSB)
		{
		  err = M2TXRaw_LODCreate(&raw, ssbFilter, M2Channel_SSB, &rawLOD);
		  if (err != M2E_NoErr)
		    {
		      fprintf(stderr,"ERROR:Resize failed!\n");
		      return(-1);
		    }
		}
	    }
	}
      else
	{
	  if ((index.XSize != lodXSize) || (index.YSize != lodYSize))
	    {
	      if (hasColor)
		{
		  err = M2TXIndex_LODCreate(&index, indexFilter, M2Channel_Index, 
					    &indexLOD);
		  if (err != M2E_NoErr)
		    {
		      fprintf(stderr,"ERROR:Resize failed!\n");
		      return(-1);
		    }
		}
	      if (hasAlpha)
		{
		  err = M2TXIndex_LODCreate(&index, alphaFilter, M2Channel_Alpha,
					    &indexLOD);
		  if (err != M2E_NoErr)
		    {
		      fprintf(stderr,"ERROR:Resize failed!\n");
		      return(-1);
		    }
		}
	      if (hasSSB)
		{
		  err = M2TXIndex_LODCreate(&index, ssbFilter, M2Channel_SSB, 
					    &indexLOD);
		  if (err != M2E_NoErr)
		    {
		      fprintf(stderr,"ERROR:Resize failed!\n");
		      return(-1);
		    }
		}
	    }
	}
      
      if (isLiteral)
	{
	  err = M2TXRaw_ToUncompr(&newTex, oldPIP, lod, &rawLOD);
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:Error during conversion, must abort.\n");
	      return(-1);
	    }
	  if ((raw.XSize != lodXSize) || (raw.YSize != lodYSize))
	    M2TXRaw_Free(&rawLOD);
	}
      else
	{
	  err = M2TXIndex_ToUncompr(&newTex, oldPIP, lod, &indexLOD);
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:Error during conversion, must abort.\n");
	      return(-1);
	    }				
	  if ((index.XSize != lodXSize) || (index.YSize != lodYSize))
	    M2TXIndex_Free(&indexLOD);
	}
      
      if (isCompressed)
	{   /* LOD comes in uncompressed */
	  M2TXHeader_SetFIsCompressed(newHeader, FALSE); 
	  err = M2TX_Compress(&newTex, lod, dci, oldPIP, 
			      M2CMP_Auto|M2CMP_CustomDCI|M2CMP_CustomPIP, 
			      &cmpSize, &texel); 
	  if (err != M2E_NoErr)
	    return(-1);
	  M2TXHeader_FreeLODPtr(newHeader, lod);  /* Free up the old LOD ptr */
	  M2TXHeader_SetLODPtr(newHeader, lod, cmpSize, texel);
	  /* Replace it with the new */
	  M2TXHeader_SetFIsCompressed(newHeader, TRUE);
	  /* LOD is now compressed */
	}    

      if (isLiteral)
	M2TXRaw_Free(&raw);
      else
	M2TXIndex_Free(&index);
      
      lodXSize = lodXSize >> 1;
      lodYSize = lodYSize >> 1;
    }
  
  M2TX_WriteFile(fileOut,&newTex);    	/* Write it to disk */
  M2TXHeader_FreeLODPtrs(newHeader);
  
  return(0);
  
}
