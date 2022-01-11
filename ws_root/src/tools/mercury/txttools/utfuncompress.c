/*
	File:		utfuncompress.c

	Contains:	Takes any utf file and uncompress it

	Written by:	Todd Allendorf 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <2>	 5/16/95	TMA		Autodocs added.
	To Do:
*/

/**
|||	AUTODOC -public -class tools -group m2tx -name utfuncompress
|||	Takes a UTF file and uncompresses it.
|||	
|||	  Synopsis
|||	
|||	    utfuncompress <input file> <output file>
|||	
|||	  Description
|||	
|||	    This tool takes a UTF file and uncompresses it, if necessary, before 
|||	    writing it out to disk.
|||	
|||	  Arguments
|||	
|||	    <input file>
|||	        The input UTF texture.
|||	    <output file>
|||	        The resulting UTF texture.
|||	
|||	  See Also
|||	
|||	    utfcompress
**/


#include "M2TXlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   UTF Uncompress\n");
}

int main( int argc, char *argv[] )
{
  M2TX tex, newTex;
  M2TXRaw raw;
  M2TXIndex index;
  M2TXHeader *header, *newHeader;
  M2TXPIP *oldPIP;
  char fileIn[256];
  char fileOut[256];
  bool hasPIP, isCompressed, isLiteral;
  uint8 numLOD,i;
  FILE *fPtr;
  M2Err err;
  
#ifdef M2STANDALONE
  printf("Enter: <FileIn> <FileOut>\n");
  printf("Example: dumb.utf dumb.cmp.utf\n");
  fscanf(stdin,"%s %s",fileIn, fileOut);
#else
  /* Check for command line options. */
  if (argc != 3)
    {
      fprintf(stderr,"Usage: %s <Input File> <Output File>\n",argv[0]);
      print_description();
      return(-1);	
    }	
  else
    {
      strcpy(fileIn, argv[1]);
      strcpy(fileOut, argv[2]);
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
  M2TX_Init(&newTex);		/* Initialize a texture */
  
  err = M2TX_ReadFile(fileIn,&tex);
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Bad File.\n");
      return(-1);
    }
  M2TX_GetPIP(&tex,&oldPIP);
  M2TX_GetHeader(&tex,&header);
  M2TX_GetHeader(&newTex,&newHeader);

  M2TX_Copy(&newTex, &tex);
  /*  M2TX_SetHeader(&newTex, header); */

  M2TXHeader_GetFIsCompressed(header, &isCompressed);
  M2TXHeader_SetFIsCompressed(newHeader, FALSE);
  M2TXHeader_GetFIsLiteral(header, &isLiteral);
  M2TXHeader_GetFHasPIP(header, &hasPIP);
  M2TX_GetPIP(&tex,&oldPIP);
  M2TX_SetPIP(&newTex, oldPIP);
  M2TXHeader_GetNumLOD(header, &numLOD);
  for (i=0; i<numLOD; i++)
    {
      if (isCompressed)
	{
	  if (isLiteral)
	    err = M2TX_ComprToM2TXRaw(&tex, oldPIP, i, &raw);
	  else
	    err = M2TX_ComprToM2TXIndex(&tex, oldPIP, i , &index);
	}
      else
	{
	  if (isLiteral)
	    err = M2TX_UncomprToM2TXRaw(&tex, oldPIP, i, &raw);
	  else
	    err = M2TX_UncomprToM2TXIndex(&tex, oldPIP, i , &index);
	}
      
      if (err != M2E_NoErr)
	{
	  fprintf(stderr,"ERROR:Error during decompression, must abort.\n");
	  return(-1);
	}				
      
      if (isLiteral)
	{
	  err = M2TXRaw_ToUncompr(&newTex, oldPIP, i, &raw);
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:Error during conversion, must abort.\n");
	      return(-1);
	    }				
	  M2TXRaw_Free(&raw);
	}
      else
	{
	  err = M2TXIndex_ToUncompr(&newTex, oldPIP, i, &index);
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:Error during conversion, must abort.\n");
	      return(-1);
	    }				
	  M2TXIndex_Free(&index);
	}
      M2TXHeader_FreeLODPtr(header,i);
    }
  
  M2TX_WriteFile(fileOut,&newTex);    	/* Write it to disk */
  M2TXHeader_FreeLODPtrs(newHeader);
  
  return(0);
}
