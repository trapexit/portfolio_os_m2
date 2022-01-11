/*
	File:		utfsplit.c

	Contains:	Extracts all the UTF textures contained in a UTF file..

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
|||	AUTODOC -public -class tools -group m2tx -name utfsplit
|||	Takes a UTF file and creates separate UTF textures
|||	
|||	  Synopsis
|||	
|||	    utfsplit <input file> <output name>
|||	
|||	  Description
|||	
|||	    This tool takes a UTF file and extracts the individual 
|||	    UTF textures. 
|||	
|||	  Arguments
|||	
|||	    <input file>
|||	        The input UTF file.
|||	    <output name>
|||	        Base name for the output UTF textures.
|||	
|||	
|||	  See Also
|||	
|||	    utfpage, utfcat
|||	    
**/


#include<stdio.h>

/* Huh!? */
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
  printf("   Split a UTF IFF CAT file into individual UTF textures.\n");
}

#define Usage  printf("Usage: %s <Input File> <Output name>\n",argv[0])

#define MAX_TEXTURES 256

Err TXTR_GetNext(IFFParser *iff, M2TX *tex, bool *foundM2);
Err TXTR_SetupFrame(IFFParser *iff, bool inLOD);

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
  int           texIndex;
  char          fileOut[256];
  char          fileIn[256];
  char          outName[256];
  M2Err         err;
  Err           result;
  bool          foundM2;

  M2TX          tex;
  IFFParser     *iff;

  int           argn;
 
  /* Check for command line options. */
  if (argc < 3)
    {
      print_description();
      Usage;
      return(-1);    
    }
  
  strcpy(fileIn, argv[1]);
  strcpy(fileOut, argv[2]);

  argn = 2;
  texIndex = 0;
  
  err = M2TX_OpenFile(fileIn, &iff, FALSE, FALSE, NULL); 
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Can't open file \"%s\"\n",fileOut);
      return(-1);
    }

  result = TXTR_SetupFrame(iff, TRUE);
  if (result<0)
    {
      fprintf(stderr,"ERROR:Can't setup the frame.\n",fileOut);
      return(-1);
    }

  result = 0;
  while(result >= 0)
    {
      M2TX_Init(&tex);
      result = TXTR_GetNext(iff, &tex, &foundM2);
      if (foundM2 && (result>=0))
	{
	  sprintf(outName, "%s.%d.utf",fileOut, texIndex);
	  err = M2TX_WriteFile(outName, &tex);
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:Error %d occured during write of \"%s\"\n", err, outName);
	      return(-1);
	    }
	  texIndex++;
	}
    }

  result = DeleteIFFParser(iff);
  if (result<0)
    fprintf(stderr,"Error during Parser deallocation.  Who cares?\n");
  return(0);
}



