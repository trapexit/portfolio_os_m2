/*
	File:		utfanim.c

	Contains:	Concatenates multiple textures into an anim.

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
|||	AUTODOC -private -class tools -group m2tx -name utfanim
|||	Takes n images and concatenates them into an M2 anim file
|||	
|||	  Synopsis
|||	
|||	    utfanim <output file> <input files>
|||	
|||	  Description
|||	
|||	    This tool takes multiple UTF images and packs them into a single 
|||	    M2 animation file 
|||	
|||	  Arguments
|||	
|||	    <input files>
|||	        The input UTF textures.
|||	    <output file>
|||	        The resulting M2 anim file.
|||	
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
  printf("   Pack all the given UTF files into a single UTF file.\n");
}

#define Usage  printf("Usage: %s <Output File> <Input Files>\n",argv[0])

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

#define	ID_M2AN MAKE_ID('M','2','A','N')
#define	ID_FRM MAKE_ID('F','R','M','{')
#define	ID_SEQ MAKE_ID('S','E','Q','{')
#define	ID_ACTN MAKE_ID('A','C','T','N')

static void
IFFSwabLong(uint32* lp)
{
  register uint8 *cp = (uint8 *)lp;
  int t;

  t = cp[3]; cp[3] = cp[0]; cp[0] = t;
  t = cp[2]; cp[2] = cp[1]; cp[1] = t;
}

static void putMemShort(uint16 val)
{
	uint8 buf;
	
	buf = val >> 8;      /* UTF specifies Motorala or MSB 1st */
	qMemPutBytes((BYTE *)&buf,  1);
	buf = val & 0x00FF;
	qMemPutBytes((BYTE *)&buf,  1);
}

static void putMemLong(uint32 val)
{
	uint8 buf;
	
	buf = val >> 24;      /* UTF specifies Motorala or MSB 1st */
	qMemPutBytes((BYTE *)&buf,  1);
	buf = (val & 0x00FF0000) >> 16;
	qMemPutBytes((BYTE *)&buf,  1);
	buf = (val & 0x0000FF00) >> 8;
	qMemPutBytes((BYTE *)&buf,  1);
	buf = val & 0x000000FF;
	qMemPutBytes((BYTE *)&buf,  1);
}

static M2Err putMemFloat(gfloat size)
{
  uint32  buf;
 
  memcpy((void *)&buf, (void *)&size, 4);

#ifdef INTEL
  IFFSwabLong(&buf);
#endif
  qMemPutBytes((BYTE *)&buf,4);
  /* Fix this for Intel later */
  return(M2E_NoErr);
}

int main( int argc, char *argv[] )
{
  uint16        xSize, ySize;
  int           nTextures;
  char          fileOut[256];
  char          fileIn[256];
  M2Err         err;
  Err           result;
  uint32        format, bufSize;
  M2TX          tex;
  IFFParser     *iff;
  FILE          *fPtr;
  uint8         *buffer;

  int           i, j, argn;
 
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
  fprintf(stderr,"nTextures = %d\n", nTextures);
  
  err = M2TX_OpenFile(fileOut, &iff, TRUE, FALSE, NULL); 
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Error during opening of file \"%s\"\n", fileOut);
      return(-1);
    }
  result = PushChunk (iff, ID_M2AN, ID_FORM, IFF_SIZE_UNKNOWN_32);
  if (result < 0)
    {
      fprintf(stderr,"ERROR: in PushChunk. Abortting\n");
      return(-1);
    }

  result = PushChunk (iff, 0L, ID_FRM, IFF_SIZE_UNKNOWN_32);
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
      M2TX_Init(&tex);
      err = M2TX_ReadFile(fileIn, &tex); 
      if (err != M2E_NoErr)
	{
	  fprintf(stderr,"ERROR:Error during reading of file \"%s\"\n", fileIn);
	  return(-1);
	}
     if (i==0)
	{
	  format = M2TX_WRITE_ALL;
	  format &= ~M2TX_WRITE_TXTR;
	  format &= ~M2TX_WRITE_LOD;
	  format |= M2TX_WRITE_M2TX;
	  err = M2TX_WriteChunkData(iff, &tex, format);
	}
      err = M2TX_WriteChunkData(iff, &tex, M2TX_WRITE_LOD );
      if (err != M2E_NoErr)
	fprintf(stderr,"ERROR:%d error while adding texel data of \"%s\"\n",
		fileIn);
    }
  result = PopChunk (iff);
  if (result < 0)
    {
      fprintf(stderr,"ERROR: in PopChunk FRM. Abortting\n");
      return(-1);
    }

   
  result = PushChunk(iff, 0L, ID_SEQ, IFF_SIZE_UNKNOWN_32);
  if (result < 0)
    {
      fprintf(stderr,"ERROR: in PushChunk. Abortting\n");
      return(-1);
    }

  result = PushChunk(iff, 0L, ID_ACTN, IFF_SIZE_UNKNOWN_32);
  if (result < 0)
    {
      fprintf(stderr,"ERROR: in PushChunk. Abortting\n");
      return(-1);
    }
  
  bufSize = nTextures*sizeof(uint16) + 8;
  if (bufSize % 4)
    bufSize += 2;
  buffer = (uint8 *)calloc(bufSize, 1);
  BeginMemPutBytes((BYTE *)buffer, bufSize);
  putMemFloat(30.0);
  putMemLong(nTextures);
  for (i=0; i<nTextures; i++)
    putMemShort(i);
  EndMemPutBytes();
  if (WriteChunkBytes (iff, buffer, bufSize) != bufSize)
    {
      result = -1;
      return(result);
    }

  result = PopChunk (iff);
  if (result < 0)
    {
      fprintf(stderr,"ERROR: in PopChunk. Action. Abortting\n");
      return(-1);
    }

  result = PopChunk (iff);
  if (result < 0)
    {
      fprintf(stderr,"ERROR: in PopChunk. Seq { Abortting\n");
      return(-1);
    }

  result = PopChunk (iff);
  if (result < 0)
    {
      fprintf(stderr,"ERROR: in PopChunk. M2AN Abortting\n");
      return(-1);
    }

  result = DeleteIFFParser(iff);
  if (result<0)
    fprintf(stderr,"Error during Parser deallocation.  Who cares?\n");
  return(0);
}











