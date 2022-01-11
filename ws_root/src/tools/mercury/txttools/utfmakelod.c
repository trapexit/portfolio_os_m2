/*
	File:		utfmakelod.c

	Contains:	Take a utf and creates (or deletes) LODs with the appropriate filters	

	Written by:	Todd Allendorf 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <3>	 7/15/95	TMA		Autodocs updated.
		 <2>	 5/16/95	TMA		Autodocs added.
	To Do:
*/

/**
|||	AUTODOC -public -class tools -group m2tx -name utfmakelod
|||	Creates or deletes LODs of a UTF file.
|||	
|||	  Synopsis
|||	
|||	    utfmakelod <input file> <new # LODs> [options] <output file>
|||	
|||	  Description
|||	
|||	    This tool takes a UTF file, and either creates or deletes levels
|||	    of detail until the output file has the specified number of levels of 
|||	    detail.  If new LODs are required, they are made by resampling the finest
|||	    level of detail.
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
|||	    sampling (complexity = 1).  Using other filters would introduce new colors
|||	    may not be in the PIP.  If you want to resample with other filters, use a
|||	    literal image, then color reduce it to the desired bitdepth.
|||	    
|||	  See Also
|||	
|||	    utfmipcat, utffit
**/

#include "M2TXlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   UTF Make Levels of Detail\n");
  printf("   -fc 1-6\tColor Filter Complexity. 1=Point Sampling (default = 5 (sinc))\n");
  printf("   -fa 1-6\tAlpha Filter Complexity. 1=Point Sampling (default = 1)\n");
  printf("   -fs 1-6\tSSB Filter Complexity. 1=Point Sampling (default = 1)\n");
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
  bool hasPIP, isCompressed, isLiteral, hasColor, hasAlpha, hasSSB;
  int nnLOD;
  uint8 numLOD, newLODs,i;
  uint16 xSize, ySize;
  uint32 lodXSize, lodYSize, cmpSize;
  FILE *fPtr;
  M2Err err;
  long *curFilter;
  long filterValue;
  long colorFilter = M2SAMPLE_LANCZS3;
  long indexFilter = M2SAMPLE_POINT;
  long alphaFilter = M2SAMPLE_POINT;
  long ssbFilter = M2SAMPLE_POINT;
  int argn;
  long divisor=1;
  
#ifdef M2STANDALONE	
  printf("Enter: <FileIn> <new # LODs> <FileOut>\n");
  printf("Example: dumb.utf 5 dumb.cmp.utf\n");
  fscanf(stdin,"%s %d %s",fileIn, &nnLOD, fileOut);
  
#else
  /* Check for command line options. */
  if (argc < 4)
    {
      fprintf(stderr,"Usage: %s <Input File> <new # LODs> [-fc 1-6] [-fa 1-6] [-fs 1-6] <Output File>\n",argv[0]);
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
      /* No output file specified. */
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
  M2TX_Init(&tex);			/* Initialize a texture */
  M2TX_Init(&newTex);			/* Initialize a texture */
  
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
      M2TX_SetPIP(&newTex, oldPIP);
    }
  if (isCompressed)
    {
      M2TX_GetDCI(&tex,&dci);
      M2TX_SetDCI(&newTex, dci);
    }
  
  M2TXHeader_GetNumLOD(header, &numLOD);
  if (numLOD == 0)
    {
      fprintf(stderr, "Attempted to makelod on texture with no texel data! ");
      fprintf(stderr, "Quitting...\n");
      return(-1);
    }
  err = M2TXHeader_GetMinXSize(header, &xSize);
  err = M2TXHeader_GetMinYSize(header, &ySize);
  
  err = M2TXHeader_SetNumLOD(newHeader, newLODs);
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Bad number of levels of detail, aborting!\n");
      return(-1);
    }
  
  if (newLODs>numLOD)
    {
      if ((xSize<<(numLOD-1))%(1<<(newLODs-1)))
	{
	  fprintf(stderr,"ERROR:XSize=%d not divisible by %d (needed for %d levels of detail).\n",
		 xSize<<(numLOD-1), 1<<(newLODs-1), newLODs);
	  return(-1);
	}
      err = M2TXHeader_SetMinXSize(newHeader, (xSize>>(newLODs-numLOD)));
    }
  else
    err = M2TXHeader_SetMinXSize(newHeader, (xSize<<(numLOD-newLODs)));
  
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Bad xSize, aborting!\n");
      return(-1);
    }
  if (newLODs>numLOD)
    {
      if ((ySize<<(numLOD-1))%(1<<(newLODs-1)))
	{
	  fprintf(stderr,"ERROR:YSize=%d not divisible by %d(needed for %d levels of detail).\n",
		 ySize<<(numLOD-1), 1<<(newLODs-1), newLODs);
	  return(-1);
	}
      err = M2TXHeader_SetMinYSize(newHeader, (ySize>>(newLODs-numLOD)));
    }
  else
    err = M2TXHeader_SetMinYSize(newHeader, (ySize<<(numLOD-newLODs)));
  
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Bad ySize, aborting!\n");
      return(-1);
    }

  if (isCompressed)
    {
      if (isLiteral)
	err = M2TX_ComprToM2TXRaw(&tex, oldPIP, 0, &raw);
      else
	err = M2TX_ComprToM2TXIndex(&tex, oldPIP,0, &index);
    }
  else
    {
      if (isLiteral)
	err = M2TX_UncomprToM2TXRaw(&tex, oldPIP, 0, &raw);
      else
	err = M2TX_UncomprToM2TXIndex(&tex, oldPIP, 0 , &index);
    }
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Raw creation failed!\n");
      return(-1);
    }

  for (i=numLOD; i<newLODs; i++)
    {
      lodXSize = xSize >> (i-numLOD+1);
      lodYSize = ySize >> (i-numLOD+1);
      fprintf(stderr, "LOD Size: x,y = %d, %d\n", lodXSize, lodYSize);
      if (isLiteral)
	{
	  err = M2TXRaw_Init(&rawLOD, lodXSize, lodYSize, hasColor, hasAlpha,
			     hasSSB);
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:Raw Allcation failed!\n");
	      return(-1);
	    }
	}
      else
	{
	  err = M2TXIndex_Init(&indexLOD, lodXSize, lodYSize, hasColor, 
			       hasAlpha, hasSSB);
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR: Index Allcation failed!\n");
	      return(-1);
	    }
	}
      
      if (isLiteral)
	{
	  if (hasColor)
	    {
	      err = M2TXRaw_LODCreate(&raw, colorFilter, M2Channel_Colors, &rawLOD);
	      if (err != M2E_NoErr)
		{
		  fprintf(stderr,"ERROR: Color Resize failed!\n");
		  return(-1);
		}
	    }
	  if (hasAlpha)
	    {
	      err = M2TXRaw_LODCreate(&raw, alphaFilter, M2Channel_Alpha, &rawLOD);
	      if (err != M2E_NoErr)
		{
		  fprintf(stderr,"ERROR:Alpha Resize failed!\n");
		  return(-1);
		}
	    }
	  if (hasSSB)
	    {
	      err = M2TXRaw_LODCreate(&raw, ssbFilter, M2Channel_SSB, &rawLOD);
	      if (err != M2E_NoErr)
		{
		  fprintf(stderr,"ERROR: SSB Resize failed!\n");
		  return(-1);
		}
	    }
	}
      else
	{
	  if (hasColor)
	    {
	      err = M2TXIndex_LODCreate(&index, indexFilter, M2Channel_Index, &indexLOD);
	      if (err != M2E_NoErr)
		{
		  fprintf(stderr,"ERROR: Color Resize failed!\n");
		  return(-1);
		}
	    }
	  if (hasAlpha)
	    {
	      err = M2TXIndex_LODCreate(&index, alphaFilter, M2Channel_Alpha, &indexLOD);
	      if (err != M2E_NoErr)
		{
		  fprintf(stderr,"ERROR: Alpha Resize failed!\n");
		  return(-1);
		}
	    }
	  if (hasSSB)
	    {
	      err = M2TXIndex_LODCreate(&index, ssbFilter, M2Channel_SSB, &indexLOD);
	      if (err != M2E_NoErr)
		{
		  fprintf(stderr,"ERROR: SSB Resize failed!\n");
		  return(-1);
		}
	    }
	}
      
      if (isLiteral)
	{
	  err = M2TXRaw_ToUncompr(&newTex, oldPIP,i, &rawLOD);
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR: during conversion, must abort.\n");
	      return(-1);
	    }				
	  M2TXRaw_Free(&rawLOD);
	}
      else
	{
	  err = M2TXIndex_ToUncompr(&newTex, oldPIP,i, &indexLOD);
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR: during conversion, must abort.\n");
	      return(-1);
	    }				
	  M2TXIndex_Free(&indexLOD);
	}
      
      if (isCompressed)
	{
	  M2TXHeader_SetFIsCompressed(newHeader, FALSE); /* LOD comes in uncompressed */
	  err = M2TX_Compress(&newTex, i, dci, oldPIP, M2CMP_Auto|M2CMP_CustomDCI|M2CMP_CustomPIP, &cmpSize, &texel); 
	  if (err != M2E_NoErr)
	    return(-1);
	  /* Free up the old LOD ptr */
	  M2TXHeader_FreeLODPtr(newHeader, i);	
	  /* Replace it with the new */
	  M2TXHeader_SetLODPtr(newHeader, i, cmpSize, texel);
	  /* LOD is now compressed */ 
	  M2TXHeader_SetFIsCompressed(newHeader, TRUE);
	}
    }
  
  if (isLiteral)
    M2TXRaw_Free(&raw);
  else
    M2TXIndex_Free(&index);
	
  M2TX_WriteFile(fileOut,&newTex);  		/* Write it to disk */
  M2TXHeader_FreeLODPtrs(newHeader);
  
  return(0);
  
}
