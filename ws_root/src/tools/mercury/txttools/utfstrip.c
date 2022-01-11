/*
	File:		utfstrip.c

	Contains:	Takes any utf file and strips out indicated chunks

	Written by:	Todd Allendorf 

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <3>	  8/7/95	TMA		Added functionality for DCI, TAB, DAB, and LR removal.
		 <2>	 5/16/95	TMA		Autodocs added.
	To Do:
*/



/**
|||	AUTODOC -public -class tools -group m2tx -name utfstrip
|||	Removes the specified chunks of the input file.
|||	
|||	  Synopsis
|||	
|||	    utfstrip <input file> [options] <output file>
|||	
|||	  Description
|||	
|||	    This tool takes a UTF file and removes the specified chunks before 
|||	    writing the file out to disk.
|||	    This is useful for optimizing space on images that share the same PIP and
|||	    needs only to be loaded once.
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
|||	    -dci
|||	        Removes the DCI chunk from the input file.
|||	    -tab
|||	        Removes the TAB chunk from the input file.
|||	    -dab
|||	        Removes the DAB chunk from the input file.
|||	    -lr
|||	        Removes the Load Rectangles chunk from the input file.
|||	    -pip
|||	        Removes the PIP chunk from the input file.
|||	    -lods
|||	        Removes the levels of detail from the input file.
|||	    -alpha
|||	        Removes the literal alpha channel from the input file.
|||	    -ssb
|||	        Removes the literal ssb channel from the input file.
|||	    -color
|||	        Removes the color channel from the input file.
|||	
|||	  See Also
|||	
|||	    utfmakesame
**/
/* Huh!? */
#ifdef applec
#include "M2TXlib.h"
#include "ifflib.h"
#else
#include "ifflib.h"
#include "M2TXlib.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   UTF Strip Chunks\n");
  printf("   -pip  \tRemove the PIP chunk.\n");
  printf("   -lods \tRemove the Levels of Detail chunk.\n");
  printf("   -dci  \tRemove the Data Compression Information chunk.\n");
  printf("   -lr   \tRemove the Load Rectangles chunk.\n");
  printf("   -tab  \tRemove the Texture Blend Attributes chunk.\n");
  printf("   -dab   \tRemove the Destination Blend Attributes chunk.\n");
  printf("   -alpha \tRemove the literal alpha channel.\n");
  printf("   -ssb   \tRemove the literal ssb channel.\n");
  printf("   -color \tRemove the color channel.\n");
}

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
	      fprintf(stderr,"WARNING:Can't find the right color, alpha, ssb combo\n");
	      return(M2E_Range);
	    }
	}	
      else
	{
	  fprintf(stderr,"WARNING:Can't find the right color, alpha, ssb combo\n");
	  return(M2E_Range);
	}
    }
  M2TXHeader_SetCDepth(header,cDepth);
  if (needSSB)
    M2TXHeader_SetFHasSSB(header, TRUE);
  return(M2E_NoErr);
}


M2Err M2TX_OpenFile(char *fileName, IFFParser **iff, bool writeMode, bool isMac, void *spec);
Err M2TX_WriteChunkData(IFFParser *iff, M2TX *tex, uint32 write_flags );

#define	ID_TXTR	MAKE_ID('T','X','T','R')


#define Usage printf("Usage: %s <Input File> [-pip] [-lods] [-dci] [-lr] [-tab] [-dab] [-alpha] [-color] [-ssb] <Output File>\n",argv[0]);

int main( int argc, char *argv[] )
{
  M2TX tex, newTex;
  M2TXHeader *header;
  M2TXPIP    *pip;
  M2TXRaw    raw;
  uint8      lod, numLOD;
  char fileIn[256];
#ifdef M2STANDALONE
  char stripField[256];
#endif
  char fileOut[256];
  bool isCompressed;
  bool stripAlpha = FALSE;
  bool stripColor = FALSE;
  bool stripSSB  = FALSE;
  FILE *fPtr;
  M2Err err;
  IFFParser     *iffOut;
  int argn;
  uint32       formFormat;

  formFormat = (uint32) (M2TX_WRITE_ALL);

#ifdef M2STANDALONE
  printf("Enter: <FileIn> <FileOut>\n");
  printf("Example: dumb.utf dumb.cmp.utf\n");
  fscanf(stdin,"%s %s",fileIn, fileOut);
  printf("Enter a chunk to remove. Either \"pip\" or \"lods\".\n");
  printf ("Enter \"done\" when finished.\n");
  do
    {
      fscanf(stdin,"%s",stripField);
      if (strcmp(stripField,"pip")==0)
	formFormat &= ~((uint32)M2TX_WRITE_PIP);
      else if (strcmp(stripField,"lods")==0)
	formFormat &= ~((uint32)M2TX_WRITE_LOD);
      else if (strcmp(stripField,"dci")==0)
	formFormat &= ~((uint32)M2TX_WRITE_DCI);
      else if (strcmp(stripField,"tab")==0)
	formFormat &= ~((uint32)M2TX_WRITE_TAB);
      else if (strcmp(stripField,"dab")==0)
	formFormat &= ~((uint32)M2TX_WRITE_DAB);
      else if (strcmp(stripField,"lr")==0)
	formFormat &= ~((uint32)M2TX_WRITE_LR);
      else if (strcmp(stripField,"header")==0)
	{
	  formFormat &= ~((uint32)M2TX_WRITE_TXTR);
	  formFormat |= M2TX_WRITE_FORM;
	}
      else if (strcmp(stripField,"alpha")==0)
	stripAlpha = TRUE;
      else if (strcmp(stripField,"ssb")==0)
	stripSSB = TRUE;
      else if (strcmp(stripField,"color")==0)
	stripColor = TRUE;
    }
  while (strcmp(stripField,"done")!=0);
  
#else
  /* Check for command line options. */
  if (argc < 4)
    {
      Usage;
      print_description();
      return(-1);    
    }
  strcpy(fileIn, argv[1]);
  
  argn = 2;
  while ( argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0' )
    {
      if ( strcmp( argv[argn], "-pip")==0 )
        {
	  ++argn;
	  formFormat &= ~((uint32)M2TX_WRITE_PIP);
        }
      else if ( strcmp( argv[argn], "-lods")==0 )
        {
	  ++argn;
	  formFormat &= ~((uint32)M2TX_WRITE_LOD);
        }
      else if ( strcmp( argv[argn], "-dci")==0 )
        {
	  ++argn;
	  formFormat &= ~((uint32)M2TX_WRITE_DCI);
        }
      else if ( strcmp( argv[argn], "-tab")==0 )
        {
	  ++argn;
	  formFormat &= ~((uint32)M2TX_WRITE_TAB);
        }
      else if ( strcmp( argv[argn], "-dab")==0 )
        {
	  ++argn;
	  formFormat &= ~((uint32)M2TX_WRITE_DAB);
        }
        else if ( strcmp( argv[argn], "-lr")==0 )
	  {
	    ++argn;
	    formFormat &= ~((uint32)M2TX_WRITE_LR);
	  }
        else if ( strcmp( argv[argn], "-header")==0 )
	  {
	    ++argn;
	    formFormat &= ~((uint32)M2TX_WRITE_TXTR);
	    formFormat |= M2TX_WRITE_FORM;
	  }
        else if ( strcmp( argv[argn], "-alpha")==0 )
	  {
	    ++argn;
	    stripAlpha = TRUE;
	  }
        else if ( strcmp( argv[argn], "-ssb")==0 )
	  {
	    ++argn;
	    stripSSB = TRUE;
	  }
        else if ( strcmp( argv[argn], "-color")==0 )
	  {
	    ++argn;
	    stripColor = TRUE;
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
    {
      /* No output file specified. */
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
  
  M2TX_Init(&tex);		/* Initialize a texture */  

  err = M2TX_ReadFile(fileIn, &tex);
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Error during read of file \"%s\"\n",fileIn);
      return(-1);
    }

  err = M2TX_OpenFile(fileOut, &iffOut, TRUE, FALSE, NULL); 
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Can't open file \"%s\"\n",fileOut);
      return(-1);
    }

  M2TX_GetHeader(&tex,&header);

  if (stripAlpha || stripColor || stripSSB)
    {
      M2TX_Copy(&newTex, &tex);
      
      if (stripAlpha)
	{
	  M2TXHeader_SetFHasAlpha(&(newTex.Header), FALSE);
	  M2TXHeader_SetADepth(&(newTex.Header), 0);
	}
      if (stripColor)
	{
	  M2TXHeader_SetFHasColor(&(newTex.Header), FALSE);
	  M2TXHeader_SetCDepth(&(newTex.Header), 0);
	}
      if (stripSSB)
	M2TXHeader_SetFHasSSB(&(newTex.Header), FALSE);

      M2TX_GetPIP(&tex, &pip);
      M2TXHeader_GetNumLOD(header, &numLOD);
      M2TXHeader_GetFIsCompressed(header, &isCompressed);
      err = make_legal(&(newTex.Header));
      if (err != M2E_NoErr)
	fprintf(stderr, "WARNING:Output won't be useable on M2.\n");

      for (lod=0; lod<numLOD; lod++)
	{
	  if (isCompressed)
	    err = M2TX_ComprToM2TXRaw(&tex, pip, lod, &raw);
	  else
	    err = M2TX_UncomprToM2TXRaw(&tex, pip, lod, &raw);
      
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:Error during decompression file \"%s\" lod=%d, must abort.\n", fileIn, lod);
	      return(-1);
	    } 

	  err = M2TXRaw_ToUncompr(&newTex, pip, lod, &raw);
	  /* Write out the new quantized texture, changing nothing else */
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:An error occured during encoding of image\"%s\" lod=%d : %d\n", fileIn, lod, err);
	      return(-1);
	    }
	  M2TXRaw_Free(&raw);
	}
      M2TX_Copy(&tex, &newTex);
      M2TX_Free(&newTex, TRUE, TRUE, TRUE, TRUE);
    }
  err = M2TX_WriteChunkData(iffOut, &tex, formFormat);
  if (err != M2E_NoErr)
    fprintf(stderr,"ERROR:%d error while writing chunk data.\n");

  M2TX_Free(&tex, TRUE, TRUE, TRUE, TRUE);
  
  return(0);
}









