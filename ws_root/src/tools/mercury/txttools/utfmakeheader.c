/*
	File:		utfmakeheader.c

	Contains:	Make a header for a PIP (for TEA)

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   Given a PIP create a legal header with no texel data.\n");
}

int main( int argc, char *argv[] )
{
  M2TX tex, newTex;
  M2TXHeader *header;
  M2TXPIP *oldPIP;
  char fileIn[256];
  char fileOut[256];
  bool hasPIP;
  uint8 pipCDepth;
  FILE *fPtr;
  M2Err err;
  int argn;
  int16 numColors;

#ifdef M2STANDALONE
  
  printf("Enter: <FileIn> <FileOut>\n");
  printf("Example: dumb.utf dumb.out.utf\n");
  fscanf(stdin,"%s %d %s",fileIn, &index, fileOut);
#else


  /* Check for command line options. */
  if (argc < 3)
    {
      fprintf(stderr,"Usage: %s <Input File> [Output File]\n",argv[0]);
      print_description();
      return(-1);    
    }
  strcpy(fileIn, argv[1]);
  argn = 2;
  if ( argn != argc )
    {
      /* Get the output file. */
      strcpy( fileOut, argv[argn] );
   }
  else
    {
      fprintf(stderr,"Usage: %s <Input File> [Output File]\n",argv[0]);
      print_description();
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
  
  M2TX_Init(&tex);			/* Initialize a texture */

  err = M2TX_ReadFileNoLODs(fileIn,&tex);
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Bad file \"%s\" \n",fileIn);
      return(-1);
    }
  
  M2TX_GetHeader(&tex,&header);
  M2TXHeader_GetFHasPIP(header, &hasPIP);
  if (hasPIP)
    {
      M2TX_GetPIP(&tex,&oldPIP);
      M2TXHeader_SetNumLOD(header, 0);
    }
  else
    {
      fprintf(stderr,"ERROR:No PIP to use.  Quitting!\n");
      return(-1);
    }

  pipCDepth = 1;
  M2TXPIP_GetNumColors(oldPIP, &numColors);
  M2TXHeader_SetFHasColor(header, TRUE);
  while ( numColors > (1<<pipCDepth))
    pipCDepth++;
   
  M2TXHeader_SetCDepth(header, pipCDepth);

  M2TX_WriteFile(fileOut,&tex);    	/* Write it to disk */
  
  return(0);
}

