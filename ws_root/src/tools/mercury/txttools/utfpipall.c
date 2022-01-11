/*
	File:		utfpipall.c

	Contains:	Change individual setttings in the PIP

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


#include "M2TXlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main( int argc, char *argv[] )
{
  M2TX tex, newTex;
  M2TXHeader *header, *newHeader;
  M2TXPIP *oldPIP, *newPIP;
  M2TXColor color;
  char fileIn[256];
  char fileOut[256];
  bool modAlpha, modSSB, hasPIP;
  bool outFile;
  uint8 newAlpha, changeAlpha, newSSB, changeSSB, colIndex;
  uint8 oldSSB, oldAlpha, oldRed, oldGreen, oldBlue, cDepth;
  FILE *fPtr;
  int index, alpha, ssb;
  M2Err err;
  int argn;

#ifdef M2STANDALONE
  
  printf("Enter: <FileIn> <FileOut>\n");
  printf("Example: dumb.utf 0 dumb.out.utf\n");
  fscanf(stdin,"%s %d %s",fileIn, &index, fileOut);
#else

  /* Check for command line options. */
  if (argc < 3)
    {
      printf("Usage: %s <Input File> [-ssb 0-1 0-1] [-alpha 0-255 0-255] [Output File]\n",argv[0]);
      return(-1);    
    }
  strcpy(fileIn, argv[1]);

  argn = 2;
  while ( argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0' )
    {
      if ( strcmp( argv[argn], "-alpha")==0 )
        {
	  ++argn;
	  modAlpha = TRUE;
	  changeAlpha  = (uint8)strtol(argv[argn], NULL, 10);
	  argn++;
	  alpha = strtol(argv[argn], NULL, 10);
	  if ((alpha > 255)||(alpha<0))
	    {
	      printf("Usage: %s <Input File> [-ssb 0-1 0-1] [-alpha 0-255 0-255] <Output File>\n",argv[0]);
	      return(-1);
	    }
	  else
	    newAlpha = (uint8)alpha;
        }
      else if ( strcmp( argv[argn], "-ssb")==0 )
        {
	  ++argn;
	  modSSB = TRUE;
	  changeSSB = (uint8)strtol(argv[argn], NULL, 10);
	  argn++;
	  ssb = strtol(argv[argn], NULL, 10);
	  if ((ssb >1)||(ssb<0))
	    {
	      printf("Usage: %s <Input File> [-ssb 0-1 0-1] [-alpha 0-255 0-255] <Output File>\n",argv[0]);
	      return(-1);
	    }
	  else
	    newSSB = (uint8)ssb;
        }
      else
	{
	  printf("Usage: %s <Input File> <PIP Index> [-ssb 0-1 0-1] [-alpha 0-255 0-255] <Output File>\n",argv[0]);
	  return(-1);
	}
      argn++;
    }
  
  if ( argn != argc )
    {
      /* Get the output file. */
      strcpy( fileOut, argv[argn] );
      outFile = TRUE;
   }
  else
    {
      /* No output file specified. */
      outFile = FALSE;
    }
#endif
  
  fPtr = fopen(fileIn, "r");
  if (fPtr == NULL)
    {
      printf("Can't open file \"%s\" \n",fileIn);
      return(-1);
    }
  else 
    fclose(fPtr);		
  
  M2TX_Init(&tex);			/* Initialize a texture */
  M2TX_Init(&newTex);			/* Initialize a texture */

  err = M2TX_ReadFile(fileIn,&tex);
  if (err != M2E_NoErr)
    return(-1);
		
  M2TX_GetHeader(&tex,&header);
  M2TX_GetHeader(&newTex,&newHeader);
  M2TX_SetHeader(&newTex, header);
  M2TXHeader_GetFHasPIP(header, &hasPIP);
  M2TXHeader_GetCDepth(header, &cDepth);
  if (hasPIP)
    {
      M2TX_GetPIP(&tex,&oldPIP);
      M2TX_SetPIP(&newTex, oldPIP);
      M2TX_GetPIP(&newTex,&newPIP);
    }
  else
    {
      printf("No PIP to modify.  Quitting!\n");
      return(-1);
    }

  for (colIndex = 0; colIndex < (1<<cDepth); colIndex++)
    {
      M2TXPIP_GetColor(newPIP, colIndex, &color);
      M2TXColor_Decode(color, &oldSSB, &oldAlpha, &oldRed, &oldGreen, &oldBlue);
      printf("Color %d: SSB=%d Alpha=%d Red=%d Green=%d Blue=%d\n", colIndex,
	     oldSSB, oldAlpha, oldRed, oldGreen, oldBlue);
      if (modAlpha)
	if (oldAlpha == changeAlpha)
	  oldAlpha = newAlpha;
      if (modSSB)
	if (oldSSB == changeSSB)
	  oldSSB = newSSB;
      color = M2TXColor_Create(oldSSB, oldAlpha, oldRed, oldGreen, oldBlue);  
      M2TXPIP_SetColor(newPIP, colIndex, color);
      M2TXColor_Decode(color, &oldSSB, &oldAlpha, &oldRed, &oldGreen, &oldBlue);
      printf("New Color %d: SSB=%d Alpha=%d Red=%d Green=%d Blue=%d\n", 
	     colIndex, oldSSB, oldAlpha, oldRed, oldGreen, oldBlue);
    }
  if (outFile)
    M2TX_WriteFile(fileOut,&newTex);    	/* Write it to disk */
  M2TXHeader_FreeLODPtrs(newHeader);
  
}

