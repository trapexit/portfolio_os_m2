/*
	File:		utfmakepip.c

	Contains:	Takes any utf file and tries to make it into an indexed file

	Written by:	Todd Allendorf 

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <3>	 7/15/95	TMA		Minor bug fix.
		 <2>	 5/16/95	TMA		Autodocs added.
	To Do:
*/


/**
|||	AUTODOC -public -class tools -group m2tx -name utfmakepip
|||	Takes a UTF file and tries to make it into an indexed file
|||	
|||	  Synopsis
|||	
|||	    utfmakepip <input file> [options] <output file>
|||	
|||	  Description
|||	
|||	    This tool takes a UTF file and tries to find a PIP that will allow it to
|||	    be written out as an indexed UTF file.  The user has the option of trying
|||	    include the alpha and SSB value in the PIP as well as the color information.
|||	
|||	  Arguments
|||	
|||	    <input file>
|||	        The input UTF texture.
|||	    <output file>
|||	        The resulting UTF texture.
|||	
|||	  Options
|||	
|||	    -matchalpha
|||	        Take the literal alpha values and try to put them in the PIP as well.
|||	    -matchssb
|||	        Take the literal ssb values and try to put them in the PIP as well.
|||	
**/

#include "M2TXlib.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   Make a PIP for the image\n");
  printf("   -matchalpha\tMove literal alpha values into the PIP\n");
  printf("   -matchssb\tMove literal ssb values into the PIP\n");
}

int main( int argc, char *argv[] )
{
  M2TX tex, newTex;
  M2TXRaw raw;
  M2TXHeader *header, *newHeader;
  M2TXPIP *oldPIP, newPIP;
  char fileIn[256];
  char fileOut[256];
  bool hasPIP, isCompressed, isLiteral, hasAlpha, hasSSB;
  bool matchColor = TRUE;
  bool matchAlpha = FALSE;
  bool matchSSB = FALSE;
  bool needSSB = FALSE;
  uint8 numLOD, i, cDepth, aDepth, ssbDepth;
  int argn;
  FILE *fPtr;
  M2Err err;
  
#ifdef M2STANDALONE
  printf("Enter: <FileIn> <FileOut>\n");
  printf("Example: dumb.utf dumb.indexed.utf\n");
  fscanf(stdin,"%s %s",fileIn, fileOut);
#else
  /* Check for command line options. */
  if (argc < 3)
    {
      fprintf(stderr,"Usage: %s <Input File> [-matchalpha] [-matchssb] <Output File>\n",argv[0]);
      print_description();
      return(-1);    
    }
  strcpy(fileIn, argv[1]);

  argn = 2;
  while ( argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0' )
    {
      if ( strcmp( argv[argn], "-matchalpha")==0 )
        {
	  ++argn;
	  matchAlpha = TRUE;
        }
      else if ( strcmp( argv[argn], "-matchssb")==0 )
        {
	  ++argn;
	  matchSSB = TRUE;
        }
      else
	{
	  fprintf(stderr,"Usage: %s <Input File> [-matchalpha] [-matchssb] <Output File>\n",argv[0]);
	  return(-1);
	}
    }
  
  if ( argn != argc )
    {
      /* Open the output file. */
      strcpy( fileOut, argv[argn] );      
    }
  else
    {
      /* No output file specified. */
      fprintf(stderr,"Usage: %s <Input File> [-matchalpha] [-matchssb] <Output File>\n",argv[0]);
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
  M2TX_Init(&newTex);			/* Initialize a texture */
  
  err = M2TX_ReadFile(fileIn,&tex);
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR: Bad file \"%s\" \n", fileIn);
      return(-1);
    }
  M2TX_GetHeader(&tex,&header);

  M2TX_Copy(&newTex, &tex);
  /*  M2TX_SetHeader(&newTex, header); */

  M2TX_GetHeader(&newTex,&newHeader);
  M2TXHeader_GetFIsCompressed(header, &isCompressed);
  M2TXHeader_GetNumLOD(header, &numLOD);
  M2TXHeader_GetFHasPIP(header, &hasPIP);
  M2TX_GetPIP(&tex,&oldPIP);

  for (i=0; i<numLOD; i++)
    {
      if (isCompressed)
	err = M2TX_ComprToM2TXRaw(&tex, oldPIP, i, &raw);
      else
	err = M2TX_UncomprToM2TXRaw(&tex, oldPIP, i, &raw);
      
      if (err != M2E_NoErr)
	{
	  fprintf(stderr,"ERROR:Error during decompression, must abort.\n");
	  return(-1);
	}				

      if (i==0)
	M2TXPIP_SetNumColors(&newPIP,0);
      err = M2TXRaw_FindPIP(&raw, &newPIP, &cDepth, matchColor, 
			    matchAlpha, matchSSB);
      if (err == M2E_Range)   /* Range Error, must be true color */
	{
	  fprintf(stderr,"ERROR:Too many color combinations to fit into a pip.\n");
	  return(-1);
	}
      M2TXRaw_Free(&raw);
    }

  M2TX_SetPIP(&newTex, &newPIP);

  for (i=0; i<numLOD; i++)
    {
      if (isCompressed)
	err = M2TX_ComprToM2TXRaw(&tex, oldPIP, i, &raw);
      else
	err = M2TX_UncomprToM2TXRaw(&tex, oldPIP, i, &raw);
      
      if (err != M2E_NoErr)
	{
	  fprintf(stderr,"ERROR:Error during decompression, must abort.\n");
	  return(-1);
	}
      
      err = M2TXRaw_ToUncompr(&newTex, &newPIP, i, &raw);
      if (err != M2E_NoErr)
	{
	  fprintf(stderr,"ERROR:Error during conversion, must abort.\n");
	  return(-1);
	}
      M2TXRaw_Free(&raw);
      M2TXHeader_FreeLODPtr(header,i);
    }
		
  M2TX_WriteFile(fileOut,&newTex);    		/* Write it to disk */
  M2TXHeader_FreeLODPtrs(newHeader);
  
  return(0);
}
