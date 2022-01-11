/*
	File:		utfpipaccum.c

	Contains:	Find a common pip across several files.

	Written by:	Todd Allendorf 

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

	To Do:
*/


#include "M2TXlib.h"
#include "M2TXattr.h"
typedef int32 Err;
#include "clt.h"
#include "clttxdblend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   UTF PIP ACCUMULATE\n");
}


int main( int argc, char *argv[] )
{
  M2TX tex;
  char fileOut[256];
  FILE *fPtr;
  M2Err err;
  int argn;
  uint8 numLODs, i;
  bool flag;
  M2TXPIP newPIP, *pip;
  M2TXHeader *header;
  uint32 totalBits, bits, samples, texels, totalTexels, length, xSize, ySize;
  double average, high, low;
  
  totalBits = 0;
  if (argc < 2)
    {
      fprintf(stderr,"Usage: %s <Output File> <PIP file1> <PIP file2..\n",argv[0]);
      print_description();
      return(-1);	
    }	
  else
    strcpy(fileOut, argv[1]);
  
  argn = 1;
  while (argn < argc)
    {
      M2TX_Init(&tex);		/* Initialize a texture */
      
      err = M2TX_ReadFile(argv[argn],&tex);
      if (err != M2E_NoErr)
	{
	  fprintf(stderr,"ERROR:Error during read.  Aborting\n");
	  return(-1);
	}
      bits = 0;
      texels = 0;
      M2TX_GetHeader(&tex, &header);
      M2TXHeader_GetNumLOD(header, &numLODs);
      for(i=0; i<numLODs; i++)
	{
	  M2TXHeader_GetLODLength(header, i, &length);
	  M2TXHeader_GetLODDim(header, i, &xSize, &ySize);	  
	  texels += xSize*ySize;
	  bits += length*8;
	}
      average = (double)bits/(double)texels;
      if (argn==1)
	{
	  low = high = average;
	}
      else 
	{
	  if (average<low)
	    low = average;
	  if (average>high)
	    high = average;
	}
      totalTexels += texels;
      totalBits += bits;
      M2TX_Free(&tex, TRUE, TRUE, TRUE, TRUE);
      argn++;
    }

  average = (double)totalBits/(double)totalTexels;
  printf("Total Texels: %d \tTotal Bits: %d\n", totalTexels, totalBits); 
  printf("Bits Per Texel: \tLow= %6.4f High= %6.4f Average= %6.4f\n",
	 low, high, average); 
  return(0);
}


  
