/*
	File:		utfpopfine.c

	Contains:	Removes a user-selected number of LODs from the top of a UTF file

	Written by:	Todd Allendorf 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <3>	 7/15/95	TMA		Autodocs updated.
		 <2>	 5/31/95	TMA		Removed unused variables.
		 <1>	 5/16/95	TMA		first checked in
	To Do:
*/

/**
|||	AUTODOC -public -class tools -group m2tx -name utfpopfine
|||	Removes a specified number of levels of detail from the top of a UTF file.
|||	
|||	  Synopsis
|||	
|||	    utfpopfine <input file> <pop #> <output file>
|||	
|||	  Description
|||	
|||	    This tool pops a specified number of the finest levels of detail, 
|||	    writing out the remaining coarser levels of detail.
|||	
|||	
|||	  Arguments
|||	
|||	    <input file>
|||	        The input UTF texture.
|||	    <pop #>
|||	        The number of levels of detail to remove, starting with the finest
|||	    <output file>
|||	        The resulting UTF texture.
|||	
|||	  See Also
|||	
|||	    utfmakelod
**/


#include "M2TXlib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   UTF Pop Finest Levels of Detail\n");
}


int main( int argc, char *argv[] )
{
  M2TX tex, newTex;
  M2TXTex lodPtr;
  M2TXPIP *oldPIP;
  M2TXDCI *oldDCI;
  M2TXHeader *header, *newHeader;
  char fileIn[256];
  int argn;
  char fileOut[256];
  int nnLOD;
  uint32 length;
  FILE *fPtr;
  uint8 numLOD, curLOD, pop, i;
  M2Err err;
  
#ifdef M2STANDALONE
  printf("Enter: <FileIn> <new # LODs> <FileOut>\n");
  printf("Example: dumb.utf 5 dumb.cmp.utf\n");
  fscanf(stdin,"%s %d %s",fileIn, &nnLOD, fileOut);
	
#else
  /* Check for command line options. */
  if (argc != 4)
    {
      fprintf(stderr,"Usage: %s <Input File> <#  LODs to remove> <Output File>\n",argv[0]);
      print_description();
      return(-1);    
    }
  strcpy(fileIn, argv[1]);
  nnLOD = strtol(argv[2], NULL, 10);
  if (nnLOD<1)
    {
      fprintf(stderr,"ERROR:Invalid number:%d.  Valid values 1 to # LODs\n",nnLOD);
      return(-1);
    }
  argn = 3;
  if ( argn != argc )
    {
      /* Open the input file. */
      strcpy( fileOut, argv[argn] );
    }
  else
    {
      /* No input file specified. */
      fprintf(stderr,"Usage: %s <Input File> <# LODs> <Output File>\n",argv[0]);
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
  
  M2TX_Init(&tex);						/* Initialize a texture */
  M2TX_Init(&newTex);						/* Initialize a texture */
  err = M2TX_ReadFile(fileIn,&tex);
  if (err != M2E_NoErr)
    return(-1);
  M2TX_GetHeader(&tex,&header);

  M2TX_Copy(&newTex, &tex);
  /*  M2TX_SetHeader(&newTex, header); */

  M2TX_GetHeader(&newTex, &newHeader);
  M2TX_GetPIP(&tex, &oldPIP);
  M2TX_SetPIP(&newTex, oldPIP);
  M2TX_GetDCI(&tex, &oldDCI);
  M2TX_SetDCI(&newTex, oldDCI);
  
  pop = nnLOD;
  M2TXHeader_GetNumLOD(header, &numLOD);

  if (numLOD == 0)
    {
      fprintf(stderr, "ERROR:No more level of details to pop! Quitting...\n");
      return(-1);
    }
  else if (pop > numLOD)
    {
      fprintf(stderr,"ERROR:Invalid number:%d.  Valid values 1 to %d\n", pop, numLOD);
      return(-1);
    }
  M2TXHeader_SetNumLOD(newHeader, numLOD-pop);
  
  curLOD = 0;
  for (i=0; i<numLOD; i++)
    {
      if (i >= pop)
	{
     /*  printf("Level of detail %d goes to new level %d\n", i, curLOD); */
	  M2TXHeader_GetLODPtr(header,i,&length, &lodPtr);
	  M2TXHeader_SetLODPtr(newHeader,curLOD,length, lodPtr);
	  curLOD++;
	}
    }
  M2TX_WriteFile(fileOut,&newTex);    			/* Write it to disk */
  
  M2TXHeader_FreeLODPtrs(header);
  return(0);
}



