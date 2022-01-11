/*
	File:		utfshare.c

	Contains:	Concatenates multiple textures into a single UTF file (LIST FORM) that share properties.

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
|||	AUTODOC -class tools -group m2tx -name utfsharepip
|||	Takes n images and concatenates them into a single utf file with one common PIP
|||
|||	  Synopsis
|||
|||	    utfsharepip <output file> <share file> [options] <input files>
|||
|||	  Description
|||
|||	    This tool takes multiple UTF images and packs them into a single
|||	    UTF file with one common pip
|||
|||	  Arguments
|||
|||	    <input files>
|||	        The input UTF textures.
|||	    <share file>
|||	        The file containing the attributes to be used for all textures.
|||	    <output file>
|||	        The resulting UTF texture (in IFF LIST format).
|||
|||
|||	  Options
|||
|||	    -dci
|||	        Share the DCI chunk from the share file.
|||	    -tab
|||	        Share the TAB chunk from the share file.
|||	    -dab
|||	        Share the DAB chunk from the share file.
|||	    -lr
|||	        Share the Load Rectangles chunk from the share file.
|||	    -pip
|||	        Share the PIP chunk from the share file.
|||	    -lods
|||	        Share the levels of detail from the share file.
|||	    -header
|||	        Share the Header chunk from the share file.
|||
|||	  See Also
|||
|||
**/


#include<stdio.h>
#include "M2TXSGIdefs.h"
#include "ifflib.h"
#include "M2TXlib.h"
#include <stdlib.h>
#include <string.h>

void print_description()
{
  printf("Description:\n");
  printf("   Version 0.9\n");
  printf("   Pack all the given UTF files into a single UTF file with shared properties.\n");
  printf("   UTF Share Chunks\n");
  printf("   -pip\tShare the PIP chunk.\n");
  printf("   -lods\tShare the Levels of Detail chunk.\n");
  printf("   -dci\tShare the Data Compression Information chunk.\n");
  printf("   -lr\tShare the Load Rectangles chunk.\n");
  printf("   -tab\tShare the Texture Blend Attributes chunk.\n");
  printf("   -dab\tShare the Destination Blend Attributes chunk.\n");
  printf("   -header\tShare the header chunk.\n");
}

#define Usage  printf("Usage: %s <Output File> <share file> [options] <Input Files>\n",argv[0])

#define MAX_TEXTURES 256

M2Err M2TX_OpenFile(char *fileName, IFFParser **iff, bool writeMode, bool isMac, void *spec);


long padPixels(int Pixels, int depth)
{
  long bits, remainder;

  bits = Pixels * depth;
  remainder = bits % 32;
  if (remainder)
    bits += (32-remainder);
  fprintf(stderr,"Bytes %d ", bits>>3);
  return(bits>>3);
}

#define	ID_TXTR	MAKE_ID('T','X','T','R')

int main( int argc, char *argv[] )
{
  uint16        xSize, ySize;
  int           nTextures;
  char          fileOut[256];
  char          fileIn[256];
  char          pipFile[256];
  M2Err         err;
  Err           result;

  M2TX          tex;
  IFFParser     *iff;
  FILE          *fPtr;
  uint32        propFormat, formFormat;

  int           i, j, argn;

  /* Check for command line options. */
  if (argc < 4)
    {
      print_description();
      Usage;
      return(-1);
    }

  formFormat = (uint32) (M2TX_WRITE_ALL);
  propFormat = 0L;

  strcpy(fileOut, argv[1]);
  strcpy(pipFile, argv[2]);
  argn = 3;


  while ( argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0' )
    {
      if ( strcmp( argv[argn], "-pip")==0 )
        {
	  ++argn;
	  propFormat |= M2TX_WRITE_PIP;
	  formFormat &= ~((uint32)M2TX_WRITE_PIP);
        }
      else if ( strcmp( argv[argn], "-lods")==0 )
        {
	  ++argn;
	  propFormat |= M2TX_WRITE_LOD;
	  formFormat &= ~((uint32)M2TX_WRITE_LOD);
        }
      else if ( strcmp( argv[argn], "-dci")==0 )
        {
	  ++argn;
	  propFormat |= M2TX_WRITE_DCI;
	  formFormat &= ~((uint32)M2TX_WRITE_DCI);
        }
      else if ( strcmp( argv[argn], "-tab")==0 )
        {
	  ++argn;
	  propFormat |= M2TX_WRITE_TAB;
	  formFormat &= ~((uint32)M2TX_WRITE_TAB);
        }
      else if ( strcmp( argv[argn], "-dab")==0 )
        {
	  ++argn;
	  propFormat |= M2TX_WRITE_DAB;
	  formFormat &= ~((uint32)M2TX_WRITE_DAB);
        }
      else if ( strcmp( argv[argn], "-lr")==0 )
	{
	  ++argn;
	  propFormat |= M2TX_WRITE_LR;
	  formFormat &= ~((uint32)M2TX_WRITE_LR);
	}
      else if ( strcmp( argv[argn], "-header")==0 )
	{
	  ++argn;
	  propFormat |= M2TX_WRITE_M2TX;
	  formFormat &= ~((uint32)M2TX_WRITE_TXTR);
	  formFormat |= M2TX_WRITE_FORM;
	}
      else
	{
	  Usage;
	  return(-1);
	}
    }

  nTextures = argc - argn;
  fprintf(stderr,"nTextures = %d\n", nTextures);

  err = M2TX_OpenFile(fileOut, &iff, TRUE, FALSE, NULL);
  result = PushChunk (iff, ID_TXTR, ID_LIST, IFF_SIZE_UNKNOWN_32);
  if (result < 0)
    {
      fprintf(stderr,"ERROR: in PushChunk. Abortting\n");
      return(-1);
    }

  result = PushChunk (iff, ID_TXTR, ID_PROP, IFF_SIZE_UNKNOWN_32);
  if (result < 0)
    {
      fprintf(stderr,"ERROR: in PushChunk. Abortting\n");
      return(-1);
    }

  err = M2TX_ReadFile(pipFile, &tex);
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Error during reading share file \"%s\"\n", fileIn);
      return(-1);
    }

  err = M2TX_WriteChunkData(iff, &tex, propFormat);
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:%d error while concatenating \"%s\"\n",
	      fileIn);
      return(-1);
    }

  result = PopChunk (iff);
  if (result < 0)
    {
      fprintf(stderr,"ERROR: in PopChunk. Abortting\n");
      return(-1);
    }

  for (i=0; i<nTextures; i++)
    {
      /* Open an input file. */
      strcpy( fileIn, argv[argn] );
      err = M2TX_ReadFile(fileIn, &tex);
      if (err != M2E_NoErr)
	{
	  fprintf(stderr,"ERROR:Error during reading of file \"%s\"\n", fileIn);
	  return(-1);
	}
      err = M2TX_WriteChunkData(iff, &tex, formFormat);
      if (err != M2E_NoErr)
	fprintf(stderr,"ERROR:%d error while concatenating \"%s\"\n",
		fileIn);
      argn++;
    }

  result = PopChunk (iff);
  if (result < 0)
    {
      fprintf(stderr,"ERROR: in PopChunk. Abortting\n");
      return(-1);
    }

  result = DeleteIFFParser(iff);
  if (result<0)
    fprintf(stderr,"Error during Parser deallocation.  Who cares?\n");
  return(0);
}



