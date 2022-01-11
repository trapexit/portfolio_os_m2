/*
	File:		utfmodpip.c

	Contains:	Change individual setttings in the PIP

	Written by:	Todd Allendorf 

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
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
|||	AUTODOC -public -class tools -group m2tx -name utfmodpip
|||	Modify or print individual values of the PIP.
|||	
|||	  Synopsis
|||	
|||	    utfmodpip <input file> <PIP index> [options]
|||	
|||	  Description
|||	
|||	    This tool prints out the value of the PIP at the given index.  If options
|||	    are given, the PIP is modified accordingly and a new texture is written
|||	    out.
|||	
|||	  Arguments
|||	
|||	    <input file>
|||	        The input UTF texture.
|||	    <PIP index>
|||	        The color value to be viewed or modified [0-255].
|||	
|||	  Options
|||	
|||	    [output file]
|||	        The resulting UTF texture.
|||	    -ssb <0,1>
|||	        The value to set ssb to.
|||	    -alpha <0-255>
|||	        The value to set the alpha value of the PIP entry to. 
|||	        Only the upper seven bits are used.
|||	
|||	  Examples
|||	
|||	    utfmodpip input.utf 2
|||	
|||	    This will print out the values of PIP entry # 2 of input.utf.
|||	
|||	    utfmodpip input.utf 0 -ssb 1 output.utf 
|||	
|||	    This will set the SSB bit for PIP entry # 0 of input.utf and write the new
|||	    new texture to output.utf
|||	
**/

#include "M2TXlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   UTF Modify PIP entry.\n");
  printf("   -ssb 0-1\tNew ssb value for the PIP entry.\n");
  printf("   -alpha 0-255\tNew alpha value for the PIP entry.\n");
}

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
  uint8 newAlpha, newSSB, colIndex;
  uint8 oldSSB, oldAlpha, oldRed, oldGreen, oldBlue;
  FILE *fPtr;
  int index, alpha, ssb;
  M2Err err;
  int argn;

#ifdef M2STANDALONE
  
  printf("Enter: <FileIn> <index> <FileOut>\n");
  printf("Example: dumb.utf 0 dumb.out.utf\n");
  fscanf(stdin,"%s %d %s",fileIn, &index, fileOut);
#else

  modAlpha = modSSB = FALSE;

  /* Check for command line options. */
  if (argc < 3)
    {
      fprintf(stderr,"Usage: %s <Input File> <PIP Index> [-ssb 0-1] [-alpha 0-255] [Output File]\n",argv[0]);
      print_description();
      return(-1);    
    }
  strcpy(fileIn, argv[1]);
  index = strtol(argv[2], NULL, 10);
  if ((index > 255)||(index<0))
    {
      fprintf(stderr,"ERROR:Palette value out of range 0-255\n");
      fprintf(stderr,"Usage: %s <Input File> <PIP Index> [-ssb 0-1] [-alpha 0-255] <Output File>\n",argv[0]);
      return(-1);
    }

  argn = 3;
  while ( argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0' )
    {
      if ( strcmp( argv[argn], "-alpha")==0 )
        {
	  ++argn;
	  modAlpha = TRUE;
	  alpha = strtol(argv[argn], NULL, 10);
	  if ((alpha > 255)||(alpha<0))
	    {
	      fprintf(stderr,"Usage: %s <Input File> <PIP Index> [-ssb 0-1] [-alpha 0-255] <Output File>\n",argv[0]);
	      return(-1);
	    }
	  else
	    newAlpha = (uint8)alpha;
        }
      else if ( strcmp( argv[argn], "-ssb")==0 )
        {
	  ++argn;
	  modSSB = TRUE;
	  ssb = strtol(argv[argn], NULL, 10);
	  if ((ssb >1)||(ssb<0))
	    {
	      fprintf(stderr,"Usage: %s <Input File> <PIP Index> [-ssb 0-1] [-alpha 0-255] <Output File>\n",argv[0]);
	      return(-1);
	    }
	  else
	    newSSB = (uint8)ssb;
        }
      else
	{
	  fprintf(stderr,"Usage: %s <Input File> <PIP Index> [-ssb 0-1] [-alpha 0-255] <Output File>\n",argv[0]);
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
      fprintf(stderr,"ERROR:Can't open file \"%s\" \n",fileIn);
      return(-1);
    }
  else 
    fclose(fPtr);		
  
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

   M2TXHeader_GetFHasPIP(header, &hasPIP);
  if (hasPIP)
    {
      M2TX_GetPIP(&tex,&oldPIP);
      M2TX_SetPIP(&newTex, oldPIP);
      M2TX_GetPIP(&newTex,&newPIP);
    }
  else
    {
      fprintf(stderr,"ERROR:No PIP to modify.  Quitting!\n");
      return(-1);
    }

  colIndex = (uint8)index;
  M2TXPIP_GetColor(newPIP, colIndex, &color);
  M2TXColor_Decode(color, &oldSSB, &oldAlpha, &oldRed, &oldGreen, &oldBlue);
  printf("Original Color: SSB=%d Alpha=%d Red=%d Green=%d Blue=%d\n",
	 oldSSB, oldAlpha, oldRed, oldGreen, oldBlue);
  if (modAlpha)
    oldAlpha = newAlpha;
  if (modSSB)
    oldSSB = newSSB;
  color = M2TXColor_Create(oldSSB, oldAlpha, oldRed, oldGreen, oldBlue);  
  M2TXPIP_SetColor(newPIP, colIndex, color);
  M2TXColor_Decode(color, &oldSSB, &oldAlpha, &oldRed, &oldGreen, &oldBlue);
  
  if (argc > 3)
    printf("New Color: SSB=%d Alpha=%d Red=%d Green=%d Blue=%d\n",
	   oldSSB, oldAlpha, oldRed, oldGreen, oldBlue);

  if (outFile)
    M2TX_WriteFile(fileOut,&newTex);    	/* Write it to disk */
  M2TXHeader_FreeLODPtrs(newHeader);
  
  return(0);
}

