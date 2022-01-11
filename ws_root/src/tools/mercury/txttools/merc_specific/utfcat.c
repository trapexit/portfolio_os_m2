/*
	File:		utfcat.c

	Contains:	Concatenates multiple textures into a single UTF file.

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
|||	AUTODOC -public -class tools -group m2tx -name utfcat
|||	Takes n images and concatenates them into a single IFF CAT of UTF textures
|||	
|||	  Synopsis
|||	
|||	    utfcat <output file> <input files>
|||	
|||	  Description
|||	
|||	    This tool takes multiple UTF images and packs them into a single
|||	    IFF CAT file of UTF textures 
|||	
|||	  Arguments
|||	
|||	    <input files>
|||	        The input UTF textures.
|||	    <output file>
|||	        The resulting UTF texture (in IFF CAT format).
|||	
|||	
|||	  See Also
|||	
|||	    
**/


#include<stdio.h>
#ifdef applec
#include "M2TXlib.h"
#include "ifflib.h"
#else
#include "ifflib.h"
#include "M2TXlib.h"
#endif

#include <stdlib.h>
#include <string.h>

void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   Concatenate all the given UTF files into a single IFF CAT file.\n");
}

#define Usage  printf("Usage: %s <Output File> <Input Files>\n",argv[0])

#define MAX_TEXTURES 256

M2Err M2TX_OpenFile(char *fileName, IFFParser **iff, bool writeMode, bool isMac, void *spec);
Err M2TX_WriteChunkData(IFFParser *iff, M2TX *tex, uint32 write_flags );

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
  int           nTextures;
  char          fileOut[256];
  char          fileIn[256];
  M2Err         err;
  Err           result;

  M2TX          tex;
  IFFParser     *iff;

  int           i, argn;
 
  /* Check for command line options. */
  if (argc < 3)
    {
      print_description();
      Usage;
      return(-1);    
    }
  
  strcpy(fileOut, argv[1]);

  argn = 2;
  nTextures = argc - 2;
  
  err = M2TX_OpenFile(fileOut, &iff, TRUE, FALSE, NULL); 
  result = PushChunk (iff, ID_TXTR, ID_CAT, IFF_SIZE_UNKNOWN_32);
  if (result < 0)
    {
      fprintf(stderr,"ERROR: in PushChunk. Abortting\n");
      return(-1);
    }

  for (i=0; i<nTextures; i++)
    {
      argn = i + 2;
      /* Open an input file. */
      strcpy( fileIn, argv[argn] );
      err = M2TX_ReadFile(fileIn, &tex);
      if (err != M2E_NoErr)
	{
	  fprintf(stderr,"ERROR:Error during reading of file \"%s\"\n", fileIn);
	  return(-1);
	}
      err = M2TX_WriteChunkData(iff, &tex, M2TX_WRITE_ALL );
      if (err != M2E_NoErr)
	fprintf(stderr,"ERROR:%d error while concatenating \"%s\"\n",
		fileIn);
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



