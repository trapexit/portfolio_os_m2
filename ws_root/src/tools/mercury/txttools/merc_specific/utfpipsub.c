/*
	File:		utfpipsub.c

	Contains:	Change all the PIP values from one value to another

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
|||	AUTODOC -public -class tools -group m2tx -name utfpipsub
|||	Modify all PIP settings of a certain value
|||	
|||	  Synopsis
|||	
|||	    utfpipsub <input file> [options] <output file>
|||	
|||	  Description
|||	
|||	    This tool will change all PIP entries with a specified alpha or ssb,
|||	    to have a new alpha or ssb value (also specified).
|||	
|||	  Arguments
|||	
|||	    <input file>
|||	        The input UTF texture.
|||	    [output file]
|||	        The resulting UTF texture.
|||	
|||	  Options
|||	
|||	    -ssb <0-1> <0-1>
|||	        Change SSBs of the first value to second value.
|||	    -alpha <0-255> <0-255>
|||	        Change alphas of the first value to second value.
|||	        Only the upper seven bits are used.
|||	
|||	  Examples
|||	
|||	    utfpipsub input.utf -ssb 0 1
|||	
|||	    This will change ever PIP entry whose SSB value is 0, to have
|||	    an SSB value of 1
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
  printf("   UTF PIP value Substitution entry.\n");
  printf("   -ssb 0-1 0-1\tOld ssb value to change to new SSB value.\n");
  printf("   -alpha 0-255 0-255\tOld alpha value to change to the new alpha value.\n");
}

#define Usage printf("Usage: %s <Input File> [-ssb <0-1> <0-1>] [-alpha <0-255> <0-255>] <Output File>\n",argv[0])

int main( int argc, char *argv[] )
{
  M2TX tex, newTex;
  M2TXHeader *header, *newHeader;
  M2TXPIP *oldPIP, *newPIP;
  M2TXColor color;
  char fileIn[256];
  char fileOut[256];
  bool modAlpha, modSSB, hasPIP;
  uint8 newAlpha, newSSB, colIndex;
  uint8 testSSB, testAlpha;
  uint8 oldSSB, oldAlpha, oldRed, oldGreen, oldBlue;
  FILE *fPtr;
  int   alpha, ssb, i;
  M2Err err;
  int argn;

  /* Check for command line options. */
  if (argc < 6)
    {
      Usage;
      print_description();
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
	  alpha = strtol(argv[argn], NULL, 10);
	  if ((alpha > 255)||(alpha<0))
	    {
	      fprintf(stderr,"ERROR:Alpha %d out of range\n", oldAlpha);
	      Usage;
	      return(-1);
	    }
	  else
	    testAlpha = (uint8)alpha;
	  ++argn;
	  alpha = strtol(argv[argn], NULL, 10);
	  if ((alpha > 255)||(alpha<0))
	    {
	      fprintf(stderr,"ERROR:Alpha %d out of range\n", oldAlpha);
	      Usage;
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
	      fprintf(stderr,"ERROR:SSB %d out of range\n", oldAlpha);
	      Usage;
	      return(-1);
	    }
	  else
	    testSSB = (uint8)ssb;
 	  ++argn;
	  modSSB = TRUE;
	  ssb = strtol(argv[argn], NULL, 10);
	  if ((ssb >1)||(ssb<0))
	    {
	      fprintf(stderr,"ERROR:SSB %d out of range\n", oldAlpha);
	      Usage;
	      return(-1);
	    }
	  else
	    newSSB = (uint8)ssb;
        }
      else
	{
	  Usage;
	  return(-1);
	}
      argn++;
    }
  
  if ( argn != argc )
    {
      /* Get the output file. */
      strcpy( fileOut, argv[argn] );
   }
  else
    {
      fprintf(stderr,"No output file specified\n");
    }
  
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

  

  for (i=0; i<newPIP->NumColors; i++)
    {
      colIndex = (uint8)i;
      M2TXPIP_GetColor(newPIP, colIndex, &color);
      M2TXColor_Decode(color, &oldSSB, &oldAlpha, &oldRed, &oldGreen, &oldBlue);
      if (modAlpha)
	{
	  if (oldAlpha == testAlpha)
	    oldAlpha = newAlpha;
	}
      if (modSSB)
	{
	  if (oldSSB == testSSB)
	    oldSSB = newSSB;
	}
      color = M2TXColor_Create(oldSSB, oldAlpha, oldRed, oldGreen, oldBlue);  
      M2TXPIP_SetColor(newPIP, colIndex, color);
    }

  M2TX_WriteFile(fileOut,&newTex);    	/* Write it to disk */
  M2TXHeader_FreeLODPtrs(newHeader);
  
  return(0);
}

