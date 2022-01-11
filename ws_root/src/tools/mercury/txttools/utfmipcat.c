/*
	File:		utfmipcat.c

	Contains:	Take the levels of detail and concatenated them into a single
                        image.

	Written by:	Todd Allendorf 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <2>	 5/31/95	TMA		Removed unused variables.
		 <1>	 5/16/95	TMA		first checked in
	To Do:
*/

/**
|||	AUTODOC -public -class tools -group m2tx -name utfmipcat
|||	Concatenates the levels of detail into a single UTF image.
|||	
|||	  Synopsis
|||	
|||	    utfmipcat <input file> <# of levels of detail> [options] <output file>
|||	
|||	  Description
|||	
|||	    This tool concatenates the individual levels of detail from a UTF file
|||	    into a single image.  If the image contains fewer than the selected number
|||	    of levels of detail, the finest level of detail is resampled with the 
|||	    user selectable filter(s) to create a new one.
|||	    This tool is mainly useful for color reducing a literal image so that all
|||	    the levels of detail share the same PIP.  To do this, create a single 
|||	    UTF image with utfmipcat, convert the components of the UTF image (alpha,
|||	    color, ssb) to a another format like PPM, use a color reduction tool on
|||	    each of the elements (like ppmquant), convert the elements back to UTF
|||	    files, then re-merge the elements into a single UTF file.  utfmakepip
|||	    can then be used to place alpha and ssb values into the PIP if necessary.
|||	
|||	  Arguments
|||	
|||	    <input file>
|||	        The input UTF texture.
|||	    <# of levels of detail>
|||	        The number of levels of detail that should be concatenated.
|||	    <output file>
|||	        The resulting UTF texture.
|||	
|||	  Options
|||	
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
|||	  See Also
|||	
|||	    utfunmip, utfmerge, ppmtoutf, utftoppm
**/

#include "M2TXlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   UTF Make a single image MIP map.\n");
  printf("   -fc 1-6\tColor Filter Complexity. 1=Point Sampling (default = 5)\n");
  printf("   -fa 1-6\tAlpha Filter Complexity. 1=Point Sampling (default = 1)\n");
  printf("   -fs 1-6\tSSB Filter Complexity. 1=Point Sampling (default = 1)\n");
}

int main( int argc, char *argv[] )
{
  M2TX tex, newTex;
  M2TXRaw raw, rawLOD, rawMIP;
  M2TXHeader *header, *newHeader;
  M2TXPIP *oldPIP;
  char fileIn[256];
  char fileOut[256];
  bool hasPIP, isCompressed, isLiteral, hasColor, hasAlpha, hasSSB;
  int nnLOD;
  uint8 numLOD, newLODs,i;
  uint16 xSize, ySize;
  uint32 lodXSize, lodYSize, startPos, n,m, fineXSize, fineYSize;
  uint32 index, mipIndex;
  FILE *fPtr;
  M2Err err;
  long *curFilter;
  long filterValue;
  long colorFilter = M2SAMPLE_LANCZS3;
  long indexFilter = M2SAMPLE_POINT;
  long alphaFilter = M2SAMPLE_POINT;
  long ssbFilter = M2SAMPLE_POINT;
  int argn;
  
#ifdef M2STANDALONE	
  printf("Enter: <FileIn> <new # LODs> <FileOut>\n");
  printf("Example: dumb.utf 5 dumb.cmp.utf\n");
  fscanf(stdin,"%s %d %s",fileIn, &nnLOD, fileOut);
  
#else
  /* Check for command line options. */
  if (argc < 4)
    {
      fprintf(stderr,"Usage: %s <Input File> <new # LODs> [-fc 0-6] [-fa 0-6] [-fs 0-6] <Output File>\n",argv[0]);
      print_description();
      return(-1);    
    }
  strcpy(fileIn, argv[1]);
  nnLOD = strtol(argv[2], NULL, 10);
  argn = 3;
  while ( argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0' )
    {
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
      else if ( strcmp( argv[argn], "-fs")==0 )
        {
	  ++argn;
	  curFilter = &ssbFilter;
        }
      
      else
	{
	  fprintf(stderr,"Usage: %s <Input File> <new # LODs> [-fc 0-6] [-fa 0-6] [-fs 0-6] <Output File>\n",argv[0]);
	  return(-1);
	}
      
      filterValue = strtol(argv[argn], NULL, 10);
      argn++;
      
      if ((filterValue > 6)||(filterValue<1))
	{
	  fprintf(stderr,"Usage: %s <Input File> <new # LODs> [-fc 0-6] [-fa 0-6] [-fs 0-6] <Output File>\n",argv[0]);
	  return(-1);
	}
      else
	{
	  *curFilter = (1<<(filterValue));
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
      fprintf(stderr,"Usage: %s <Input File> [-pip] [-LODSs] <Output File>\n",argv[0]);
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
  
  newLODs = nnLOD;
  M2TX_Init(&tex);		/* Initialize a texture */
  M2TX_Init(&newTex);		/* Initialize a texture */
  
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
  M2TXHeader_GetFHasAlpha(header, &hasAlpha);
  M2TXHeader_GetFHasSSB(header, &hasSSB);
  M2TXHeader_GetFHasPIP(header, &hasPIP);
  if (hasPIP)
    {
      M2TX_GetPIP(&tex,&oldPIP);
     }
  
  M2TXHeader_GetNumLOD(header, &numLOD);
  err = M2TXHeader_GetMinXSize(header, &xSize);
  err = M2TXHeader_GetMinYSize(header, &ySize);
  
  err = M2TXHeader_SetNumLOD(newHeader, 1);
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Bad number of levels of detail, aborting!\n");
      return(-1);
    }
  
  fineXSize = xSize <<(numLOD-1);
  fineYSize = ySize <<(numLOD-1);
  err = M2TXHeader_SetMinXSize(newHeader, fineXSize);
  
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Bad xSize, aborting!\n");
      return(-1);
    }
  
  err = M2TXHeader_SetMinYSize(newHeader, 2*fineYSize);
  
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Bad ySize, aborting!\n");
      return(-1);
    }
 
  M2TXHeader_SetCDepth(newHeader,  8);
  M2TXHeader_SetFIsLiteral(newHeader, TRUE);
  M2TXHeader_SetFHasPIP(newHeader, FALSE);
  M2TXHeader_SetFIsCompressed(newHeader, FALSE);

  if (isCompressed)
    err = M2TX_ComprToM2TXRaw(&tex, oldPIP, 0, &raw);
  else
    err = M2TX_UncomprToM2TXRaw(&tex, oldPIP, 0, &raw);
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Raw creation failed!\n");
      return(-1);
    }

  M2TXRaw_Init(&rawMIP, fineXSize, 2*fineYSize, hasColor, hasAlpha, hasSSB);
  
  startPos = fineYSize;
  index = 0;
  for (n=0; n<fineYSize; n++)
    {
      mipIndex=startPos*fineXSize;
      for (m=0; m<fineXSize; m++)
	{
	  if (hasColor)
	    {
	      rawMIP.Red[mipIndex] = raw.Red[index]; 
	      rawMIP.Green[mipIndex] = raw.Green[index];
	      rawMIP.Blue[mipIndex] = raw.Blue[index];
	    }
	  if (hasAlpha)
	    {
	      rawMIP.Alpha[mipIndex] = raw.Alpha[index];
	    } 
	  if (hasSSB)
	    {
	      rawMIP.SSB[mipIndex] = raw.SSB[index]; 
	    }
	  mipIndex++;
	  index++;
	}
      startPos++;
    }
   
  startPos = 0;
  for (i=0; i<newLODs; i++)
    {
      lodXSize = fineXSize >> (i);
      lodYSize = fineYSize >> (i);
      
      if (i>(numLOD-1))
	{
	  err = M2TXRaw_Init(&rawLOD, lodXSize, lodYSize, hasColor, hasAlpha,
			     hasSSB);
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:Allcation failed!\n");
	      return(-1);
	    }
	  if (hasColor)
	    {
	      err = M2TXRaw_LODCreate(&raw, colorFilter, M2Channel_Colors, &rawLOD);
	      if (err != M2E_NoErr)
		{
		  fprintf(stderr,"ERROR:Resize failed!\n");
		  return(-1);
		}
	    }
	  if (hasAlpha)
	    {
	      err = M2TXRaw_LODCreate(&raw, alphaFilter, M2Channel_Alpha, &rawLOD);
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
      else
	{
	  if (isCompressed)
	    err = M2TX_ComprToM2TXRaw(&tex, oldPIP, i, &rawLOD);
	  else
	    err = M2TX_UncomprToM2TXRaw(&tex, oldPIP, i, &rawLOD);
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:Raw creation failed!\n");
	      return(-1);
	    }
	}
      index = 0;
      for (n=0; n<lodYSize; n++)
	{
	  mipIndex=startPos*fineXSize;
	  for (m=0; m<lodXSize; m++)
	    {
	      if (hasColor)
		{
		  rawMIP.Red[mipIndex] = rawLOD.Red[index]; 
		  rawMIP.Green[mipIndex] = rawLOD.Green[index];
		  rawMIP.Blue[mipIndex] = rawLOD.Blue[index];
		}
	      if (hasAlpha)
		{
		  rawMIP.Alpha[mipIndex] = rawLOD.Alpha[index];
		} 
	      if (hasSSB)
		{
		  rawMIP.SSB[mipIndex] = rawLOD.SSB[index]; 
		}
	      mipIndex++;
	      index++;
	    }
	  startPos++;
	}
      M2TXRaw_Free(&rawLOD);
    }
  err = M2TXRaw_ToUncompr(&newTex, NULL, 0, &rawMIP);
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Error during conversion, must abort.\n");
      return(-1);
    }				
  M2TXRaw_Free(&rawMIP);
  
  M2TXRaw_Free(&raw);
  
  M2TX_WriteFile(fileOut,&newTex);    			/* Write it to disk */
  M2TXHeader_FreeLODPtrs(newHeader);
  
  return(0);
  
}
