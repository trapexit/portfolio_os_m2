/*
   File:	utfmakesame.c
   
   Contains:	Take a utf and compress it with the same dci and pip as the
   texture	
   
   Written by:	Todd Allendorf 
   
   Copyright:	© 1994 by The 3DO Company. All rights reserved.
   This material constitutes confidential and proprietary
   information of the 3DO Company and shall not be used by
   any Person or for any purpose except as expressly
   authorized in writing by the 3DO Company.
   
	Change History (most recent first):
	
		<2+>	 7/16/95	TMA		Removed unused variables.
		 <2>	 5/16/95	TMA		Autodocs added. Exact color depth match no longer required
									between UTF files as long as one is a superset of the other.
	To Do:
	*/

/**
|||	AUTODOC -public -class tools -group m2tx -name utfmakesame
|||	Take a utf and compress it with the same dci and pip as the reference UTF.
|||	
|||	  Synopsis
|||	
|||	    utfmakesame <input file> <reference file> <output file>
|||	
|||	  Description
|||	
|||	    This tool takes an input UTF file and a "reference" UTF file and tries to
|||	    compress the input file using the PIP, DCI, and Header of the reference
|||	    file.  Two indexed images which share the same colors but have different
|||	    PIPs (order is different) can be made share the same PIP with this tool.
|||	
|||	  Arguments
|||	
|||	    <input file>
|||	        The input UTF texture.
|||	    <reference file>
|||	        The UTF texture whose DCI, PIP, and Header will be used for compression.
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
  printf("   Coerce a UTF to have the same settings as another UTF\n");
  printf("   <reference file> \tThe UTF file whose DCI and/or PIP you want to use.\n");
}

#define Usage printf("Usage: %s <Input File> <Reference File> [-noheader] <Output File>\n", argv[0])

int main( int argc, char *argv[] )
{
  M2TX tex, newTex, refTex;
  M2TXDCI *dci, *refDCI;
  M2TXTex texel;
  M2TXIndex myIndex;
  M2TXRaw myRaw;
  M2TXHeader *header, *newHeader, *refHeader;
  M2TXPIP *oldPIP, *refPIP;
  char fileIn[256];
  char fileOut[256];
  char fileRef[256];
  bool hasPIP, isCompressed, isLiteral, hasSSB, hasAlpha;
  bool rHasPIP, rIsCompressed, rIsLiteral, rHasSSB, rHasAlpha;
  bool noHeader = FALSE;
  uint8 numLOD, i, cDepth, aDepth, rCDepth, rADepth;
  uint32 cmpSize;
  FILE *fPtr;
  int   argn;
  M2Err err;
  
#ifdef M2STANDALONE	
  printf("Enter: <FileIn> <File Reference> <FileOut>\n");
  printf("Example: dumb.utf old.cmp.utf dumb.cmp.utf\n");
  fscanf(stdin,"%s %s %s",fileIn, fileRef, fileOut);
  
#else
  /* Check for command line options. */

  if (argc < 4)
    {
      Usage;
      print_description();
      return(-1);    
    }

  strcpy(fileIn, argv[1]);
  strcpy(fileIn, argv[1]);
  strcpy(fileRef, argv[2]);    
  
  argn = 3;
  while ( argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0' )
    {
      if ( strcmp( argv[argn], "-noheader")==0 )
        {
	  ++argn;
	  noHeader = TRUE;
	}
      else
	{
	  Usage;
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
      Usage;
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
  M2TX_Init(&newTex);		        /* Initialize a texture */
  M2TX_Init(&refTex);
  err = M2TX_ReadFile(fileIn,&tex);
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Error reading input file \"%s\"\n", fileIn);
      return(-1);
    }
  err = M2TX_ReadFileNoLODs(fileRef,&refTex);
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Error reading reference file \"%s\"\n", fileRef);
      return(-1);
    }
  
  M2TX_GetHeader(&tex,&header);
  M2TX_GetHeader(&refTex,&refHeader);	

  M2TX_Copy(&newTex, &tex);
  /*  M2TX_SetHeader(&newTex, header); */

  M2TX_GetHeader(&newTex,&newHeader);
  M2TXHeader_GetCDepth(header, &cDepth);	
  M2TXHeader_GetCDepth(refHeader, &rCDepth);
  M2TXHeader_GetADepth(header, &aDepth);	
  M2TXHeader_GetADepth(refHeader, &rADepth);
  M2TXHeader_GetFHasAlpha(header, &hasAlpha);	
  M2TXHeader_GetFHasAlpha(refHeader, &rHasAlpha);	
  M2TXHeader_GetFHasSSB(header, &hasSSB);	
  M2TXHeader_GetFHasSSB(refHeader, &rHasSSB);	
  M2TXHeader_GetFHasPIP(header, &hasPIP);	
  M2TXHeader_GetFHasPIP(refHeader, &rHasPIP);	
  
  M2TXHeader_GetFIsCompressed(header, &isCompressed);
  M2TXHeader_GetFIsCompressed(refHeader, &rIsCompressed);
  M2TXHeader_GetFIsLiteral(header, &isLiteral);
  M2TXHeader_GetFIsLiteral(refHeader, &rIsLiteral);
  M2TX_GetPIP(&tex,&oldPIP);

  if(isLiteral == rIsLiteral)
    {
      if (!noHeader)
	{
	  if (isCompressed && (!rIsCompressed))
	    {
	      fprintf(stderr,"ERROR:The function you want is utfuncompress. Abortting\n");
	      return(-1);
	    }
	  if (cDepth > rCDepth)
	    {
	      fprintf(stderr,"ERROR:Expansion CDepth for reference must be greater or equal to input image. Abort!\n");
	      return(-1);
	    }
	  if (aDepth > rADepth)
	    {
	      fprintf(stderr,"ERROR:Expansion ADepth for reference must be greater or equal to input image. Abort!\n");
	      return(-1);
	    }
	  if (hasSSB &&  !rHasSSB)
	    {
	      fprintf(stderr,"ERROR:Expansion SSB for reference must be greater or equal to input image. Abort!\n");
	      return(-1);
	    }

	  M2TXHeader_SetCDepth(newHeader, rCDepth);
	  M2TXHeader_SetADepth(newHeader, rADepth);
	  M2TXHeader_SetFHasSSB(newHeader, rHasSSB);	
	  M2TXHeader_SetFHasAlpha(newHeader, rHasAlpha);	
	  M2TXHeader_SetFIsLiteral(newHeader, rIsLiteral);
	}
      M2TX_GetPIP(&refTex,&refPIP);
      M2TX_SetPIP(&newTex, refPIP);
      
      if (hasPIP && rHasPIP)
	{
	  err = M2TXPIP_MakeTranslation(oldPIP, refPIP, rCDepth);
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"WARNING:Error making translation table.\n"); 
	    }
	}
    }
  else if (isLiteral && (!rIsLiteral))
    {
      fprintf(stderr,"ERROR:You cannot convert Literal to Indexed with this function.\n");
      fprintf(stderr,"Use utfmakepip instead. Abort!\n");
      return(-1);
    }
  else /* !isLiteral && rIsLiteral */
    {
      M2TXHeader_SetCDepth(newHeader, rCDepth);
      M2TXHeader_SetADepth(newHeader, rADepth);
      M2TXHeader_SetFHasSSB(newHeader, rHasSSB);	
      M2TXHeader_SetFHasAlpha(newHeader, rHasAlpha);	
      M2TXHeader_SetFIsLiteral(newHeader, rIsLiteral);
      M2TX_SetPIP(&newTex, oldPIP);
      M2TX_GetPIP(&newTex,&refPIP);
    }

/*
  if ((!rIsLiteral) && (!rIsCompressed))
    {
      printf("ERROR:The function you want is utfuncompress. Abortting\n");
      return(-1);
    }
*/

  M2TXHeader_SetFHasPIP(newHeader, rHasPIP);	
  M2TXHeader_SetFIsCompressed(newHeader, rIsCompressed);     

  M2TX_GetDCI(&tex,&dci);
  M2TX_GetDCI(&refTex,&refDCI);
  M2TX_SetDCI(&newTex, refDCI);
  
  M2TXHeader_GetNumLOD(header, &numLOD);

  for (i=0; i<numLOD; i++)
    {
      if (rIsCompressed)
	{
	  err = M2TX_Compress(&tex, i, refDCI, refPIP, 
			      M2CMP_Auto|M2CMP_CustomDCI|M2CMP_CustomPIP, 
			      &cmpSize, &texel);
	  if (err != M2E_NoErr)
	    return(-1);
	  M2TXHeader_FreeLODPtr(newHeader, i);
	  /* Free up the old LOD ptr */
	  M2TXHeader_SetLODPtr(newHeader, i, cmpSize, texel);  
	  /* Replace it with the new */
	}
      else if (!isLiteral)
	{
	  if (isCompressed)
	    err = M2TX_ComprToM2TXIndex(&tex, oldPIP, i, &myIndex);
	  else
	    err = M2TX_UncomprToM2TXIndex(&tex, oldPIP, i, &myIndex);
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:Error during texel-to-index conversion.\n");
	      return(-1);
	    }
	  if (!rIsLiteral)
	    {
	      err = M2TXIndex_ReorderToPIP(&myIndex, refPIP, cDepth);  
	      if (err != M2E_NoErr)
		{
		  fprintf(stderr,"ERROR:Error during reordering of PIP.\n");
		  return(-1);
		}
	    }
	  err = M2TXIndex_ToUncompr(&newTex, refPIP, i, &myIndex);
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:Error during index-to-texel conversion.\n");
	      return(-1);
	    }
	  M2TXIndex_Free(&myIndex);
	}
      else
	{
	  if (isCompressed)
	    err = M2TX_ComprToM2TXRaw(&tex, oldPIP, i, &myRaw);
	  else
	    err = M2TX_UncomprToM2TXRaw(&tex, oldPIP, i, &myRaw);
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:Error during texel-to-index conversion.\n");
	      return(-1);
	    }
	  err = M2TXRaw_ToUncompr(&newTex, refPIP, i, &myRaw);
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:Error during raw-to-texel conversion.\n");
	      return(-1);
	    }
	  M2TXRaw_Free(&myRaw);
	}
    }
  
  /* LOD is now compressed */
  M2TX_WriteFile(fileOut,&newTex);    	/* Write it to disk */
  M2TXHeader_FreeLODPtrs(newHeader);
  
  return(0);
  
}


