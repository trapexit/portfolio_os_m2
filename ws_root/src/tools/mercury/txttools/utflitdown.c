/*
	File:		utflitdown.c

	Contains:	An example of using M2TXlib's literal dithering functions.

	Written by:	Todd Allendorf 

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <3>	  8/7/95	TMA		Fixed color depth checking.
		 <2>	  8/7/95	TMA		Fixed SSB bug.
	To Do:
*/

/**
|||	AUTODOC -public -class tools -group m2tx -name utflitdown
|||	Reduces bitdepth of a literal and dithers the result
|||	
|||	  Synopsis
|||	
|||	    utflitdown <input file> <new bitdepth> [options] <output file>
|||	
|||	  Description
|||	
|||	    This tool takes a UTF file, and reduces the bitdepth a literal 
|||	    color or alpha channel.  Ordered or Floyd-Steingberg dithering may
|||	    be performed at that time.
|||	
|||	  Arguments
|||	
|||	    <input file>
|||	        The input UTF texture.
|||	    <new bitdepth>
|||	        The final bitdepth of the selected component.
|||	    <output file>
|||	        The resulting UTF texture.
|||	
|||	  Options
|||	
|||	    -floyd
|||	        Perform Floyd-Steinberg dithering during depth reduction
|||	    -alpha
|||	        Reduce the depth of the alpha channel instead of the color channel
|||	    -nodither
|||	        Perform no dithering on the selected channel
|||	
|||	  Caveats
|||	
|||	    Indexed UTF files cannot have their color reduced with this method.
|||	    Also, if no dithering options are specified, ordered dithering will
|||	    be performed.  The color channel is always considered the default
|||	    channel and the bit depth is per component and therefore should 
|||	    never be greater than eight.
|||	    
|||	  See Also
|||	
|||	    quantizer
**/

#include<stdio.h>
#include "M2TXlib.h"
#include <stdlib.h>
#include <string.h>

void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   Reduce the bitdepth of a literal channel.\n");
  printf("   -alpha\tReduce the bitdepth of the literal alpha channel.\n");
  printf("   -floyd\tUse Floyd-Steinberg dithering (instead of ordered).\n");
  printf("   -nodither\tUse no dithering (instead of ordered).\n");
}

int main( int argc, char *argv[] )
{
  M2TXRaw       raw;
  M2TX tex, newTex;
  bool          isCompressed, isLiteral;
  bool          alpha = FALSE;
  bool          floyd = FALSE;
  bool          nodither = FALSE;
  M2TXHeader    *header, *newHeader;
  char fileIn[256];
  char fileOut[256];
  uint8         numLOD, i;
  int           newDepth;
  int argn;
  M2Err         err;
  M2TXPIP       *oldPIP, *newPIP;
  FILE          *fPtr;
 
#ifdef M2STANDALONE
  printf("Enter: <FileIn> <newDepth> <FileOut>\n");
  printf("Example: dumb.utf 5 dumb.indexed.utf\n");
  fscanf(stdin,"%s %d %s",fileIn, &newDepth, fileOut);
#else
  /* Check for command line options. */
  if (argc < 4)
    {
      fprintf(stderr,"Usage: %s <Input File> <Component Depth> [-alpha] [-floyd] [-nodither] <Output File>\n",argv[0]);
      print_description();
      return(-1);    
    }
  strcpy(fileIn, argv[1]);
  argn = 3;
  while ( argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0' )
    {
      if ( strcmp( argv[argn], "-alpha")==0 )
        {
	  ++argn;
	  alpha = TRUE;
        }      
      else if ( strcmp( argv[argn], "-floyd")==0 )
        {
	  ++argn;
	  floyd = TRUE;
        }
      else if ( strcmp( argv[argn], "-nodither")==0 )
        {
	  ++argn;
	  nodither = TRUE;
        }
      else
	{
	  fprintf(stderr,"Usage: %s <Input File> <Num Colors> [-alpha] [-floyd] [-nodither] <Output File>\n",argv[0]);
	  return(-1);
	}
    }
      if ( argn != argc )
	{
	  /* Open the input file. */
	  strcpy( fileOut, argv[argn] );
	}
    else
      {
        /* No input file specified. */
	  fprintf(stderr,"Usage: %s <Input File> <Num Colors> [-alpha] [-floyd] [-nodither] <Output File>\n",argv[0]);
    	return(-1);   
      }
  
  newDepth = strtol(argv[2], NULL, 10);
  strcpy( fileOut, argv[argn] );
  
#endif

  if ((!alpha) && ((newDepth != 8) && (newDepth != 0) && (newDepth != 5)))
    {
      fprintf(stderr,"ERROR:Component color depth %d is illegal, must be 0, 5, or 8\n",
	     newDepth);
      return(-1);   
    }
  if ( alpha && ((newDepth != 7) && (newDepth != 0) && (newDepth != 4)))
    {
      fprintf(stderr,"ERROR:Alpha depth %d is illegal, must be 0, 4, or 7\n",
	     newDepth);
      return(-1);   
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
  /* Old texture header to the new texture */

  M2TXHeader_GetFIsCompressed(header, &isCompressed);
  M2TX_GetPIP(&tex,&oldPIP);
  M2TX_GetPIP(&newTex, &newPIP);

  M2TXHeader_GetFIsLiteral(header, &isLiteral);
  if ((!alpha) && (!isLiteral))
    {
      fprintf(stderr,"ERROR:You can't use this function to reduce an indexed color image.\n");
      fprintf(stderr,"Use quantizer instead.\n");
      return(-1);   
    }
    
  if (alpha)
    {
      M2TXHeader_SetADepth(newHeader, newDepth);
      if (newDepth == 0)
	M2TXHeader_SetFHasAlpha(newHeader, FALSE);
      else
	M2TXHeader_SetFHasAlpha(newHeader, TRUE);
    }
  else
    {
      M2TXHeader_SetCDepth(newHeader, newDepth);
      if (newDepth == 5)
	M2TXHeader_SetFHasSSB(newHeader, TRUE);
      if (newDepth == 0)
	M2TXHeader_SetFHasColor(newHeader, FALSE);
      else
	M2TXHeader_SetFHasColor(newHeader, TRUE);
    }

  M2TXHeader_GetNumLOD(header, &numLOD);

  for (i=0; i< numLOD; i++)
    {
      if (isCompressed)
	err = M2TX_ComprToM2TXRaw(&tex, oldPIP, 0, &raw);
      else
	err = M2TX_UncomprToM2TXRaw(&tex, oldPIP, 0, &raw);
      
      if (err != M2E_NoErr)
	{
	  fprintf(stderr,"ERROR:Error during decompression, must abort.\n");
	  return(-1);
	} 

      if (!nodither)
	{
	  if (alpha)
	    {
	      if (newDepth == 4)
		{
		  err = M2TXRaw_DitherDown(&raw, FALSE, newDepth, floyd);
		  if (err != M2E_NoErr)
		    {
		      fprintf(stderr,"ERROR:An error occured during DitherDown:%d\n",err);
		      return(-1);
		    }      
		}
	    }
	  else
	    {
	      if (newDepth == 5)
		{
		  err = M2TXRaw_DitherDown(&raw, TRUE, newDepth, floyd);
		  if (err != M2E_NoErr)
		    {
		      fprintf(stderr,"ERROR:An error occured during DitherDown:%d\n",err);
		      return(-1);
		    }
		}
	    }
	}
      err = M2TXRaw_ToUncompr(&newTex, NULL, 0, &raw);
      M2TXRaw_Free(&raw);
    }
  /* Write out the new quantized texture, changing nothing else */
  M2TX_WriteFile(fileOut, &newTex);
  return(0);
}

