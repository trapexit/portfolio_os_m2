/*
	File:		utfunpip.c

	Contains:	Takes any indexed utf file and creates literal channels from the PIP

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
|||	AUTODOC -public -class tools -group m2tx -name utfunpip
|||	Takes an indexed UTF file and tries to create literal channels
|||	
|||	  Synopsis
|||	
|||	    utfunpip <input file> [options] <output file>
|||	
|||	  Description
|||	
|||	    This tool takes an indexed UTF file and tries to create literal channels
|||	    (i.e. color, alpha, ssb) in the output image.  Existing literal
|||	    channels will be overwritten if they already exist in the image
|||	
|||	  Arguments
|||	
|||	    <input file>
|||	        The indexed input UTF texture.
|||	    <output file>
|||	        The resulting UTF texture.
|||	
|||	  Options
|||	
|||	    -alpha
|||	        Take the PIP alpha values and create a literal alpha channel.
|||	    -ssb
|||	        Take the PIP ssb values and create a literal alpha channel.
|||	    -color
|||	        Take the PIP color values and create a literal color channel.
|||	
**/

#include "M2TXlib.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

M2Err make_legal(M2TXHeader *header)
{
  uint8 aDepth, cDepth, ssbDepth;
  bool isLiteral, hasSSB, hasColor, hasAlpha;
  bool needSSB = FALSE;

  M2TXHeader_GetFIsLiteral(header, &isLiteral);
  M2TXHeader_GetFHasSSB(header, &hasSSB);
  if (hasSSB)
    ssbDepth = 1;
  else
    ssbDepth = 0;
  M2TXHeader_GetFHasAlpha(header, &hasAlpha);
  if (hasAlpha)
    M2TXHeader_GetADepth(header, &aDepth);	
  else
    aDepth = 0;
  M2TXHeader_GetFHasColor(header, &hasColor);
  if (hasColor)
    M2TXHeader_GetCDepth(header, &cDepth);	
  else
    cDepth = 0;

  while ((!(M2TX_IsLegal(cDepth, aDepth, ssbDepth, isLiteral))) && (cDepth<9))
    cDepth++;
  if (cDepth>8)
    {
      cDepth = 8;
      if (!hasSSB)
	{
	  if (M2TX_IsLegal(cDepth,aDepth, 1, isLiteral))
	    needSSB = TRUE;
	  else
	    {
	      fprintf(stderr,"WARNING:Can't find the right color, alpha, ssb combo\n");
	      return(M2E_Range);
	    }
	}	
      else
	{
	  fprintf(stderr,"WARNING:Can't find the right color, alpha, ssb combo\n");
	  return(M2E_Range);
	}
    }
  M2TXHeader_SetCDepth(header,cDepth);
  if (needSSB)
    M2TXHeader_SetFHasSSB(header, TRUE);
}

void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   Create literal channels from PIP values\n");
  printf("   -color\tMake a literal color channel from the PIP values\n");
  printf("   -alpha\tMake a literal alpha channel from the PIP values\n");
  printf("   -ssb  \tMake a literal ssb channel from the PIP values\n");
}

#define Usage printf("Usage: %s <Input File> [-color] [-alpha] [-ssb] <Output File>\n",argv[0]);

int main( int argc, char *argv[] )
{
  M2TX tex, newTex;
  M2TXIndex index;
  M2TXHeader *header, *newHeader;
  M2TXPIP *oldPIP;
  char fileIn[256];
  char fileOut[256];
  bool hasPIP, isCompressed, isLiteral, hasAlpha, hasSSB;
  bool matchColor = FALSE;
  bool matchAlpha = FALSE;
  bool matchSSB = FALSE;
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
	  matchAlpha = TRUE;
        }
      else if ( strcmp( argv[argn], "-ssb")==0 )
        {
	  ++argn;
	  matchSSB = TRUE;
        }
      else if ( strcmp( argv[argn], "-color")==0 )
        {
	  ++argn;
	  matchColor = TRUE;
        }
      else
	{
	  Usage;
	  return(-1);
	}
    }
  
  if ( argn != argc )
    {      /* Open the output file. */
      strcpy( fileOut, argv[argn] );      
    }
  else
    {      /* No output file specified. */
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
  M2TX_Init(&newTex);			/* Initialize a texture */
  
  err = M2TX_ReadFile(fileIn,&tex);
  if (err != M2E_NoErr)
    return(-1);
  M2TX_GetHeader(&tex,&header);

  M2TX_Copy(&newTex, &tex);
  /*  M2TX_SetHeader(&newTex, header); */

  M2TX_GetHeader(&newTex,&newHeader);
  M2TXHeader_GetFIsCompressed(header, &isCompressed);
  M2TXHeader_GetFIsLiteral(header, &isLiteral);
  if (isLiteral)
    {
      fprintf(stderr,"ERROR:This is a literal image.\n");
      return(-1);
    }
  M2TXHeader_GetFHasPIP(header, &hasPIP);
  if (!hasPIP)
    {
      fprintf(stderr,"ERROR:No PIP to work with!\n");
      return(-1);
    }
  M2TX_GetPIP(&tex,&oldPIP);
  M2TXHeader_GetNumLOD(header, &numLOD);
  
  if (matchAlpha)
    {
      M2TXHeader_SetFHasAlpha(newHeader, TRUE);
      M2TXHeader_SetADepth(newHeader, 7);
    }
  if (matchSSB)
    M2TXHeader_SetFHasSSB(newHeader, TRUE);
  if (matchColor)
    {
      cDepth = 8;
      M2TXHeader_SetCDepth(newHeader, cDepth);
      M2TXHeader_SetFIsLiteral(newHeader, TRUE);
      M2TXHeader_SetFHasPIP(newHeader, FALSE);
    }
  else
    M2TXHeader_SetFIsLiteral(newHeader, FALSE);
  
  M2TXHeader_GetFHasSSB(newHeader, &hasSSB);
  if (hasSSB)
    ssbDepth = 1;
  else
    ssbDepth = 0;
  M2TXHeader_GetFHasAlpha(newHeader, &hasAlpha);
  if (hasAlpha)
    M2TXHeader_GetADepth(newHeader, &aDepth);	
  else
    aDepth = 0;
  
  M2TXHeader_SetFIsCompressed(newHeader, FALSE);
  err = make_legal(newHeader);
  fprintf(stderr,"WARNING:Output won't be usable on M2.\n");  

  for (i=0; i<numLOD; i++)
    {
      if (isCompressed)
	err = M2TX_ComprToM2TXIndex(&tex, oldPIP, i, &index);
      else
	err = M2TX_UncomprToM2TXIndex(&tex, oldPIP, i, &index);
      if (err != M2E_NoErr)
	{
	  fprintf(stderr,"ERROR:Error during decompression, must abort.\n");
	  return(-1);
	}				

      if ((matchAlpha) && (hasAlpha))
	{
	  free(index.Alpha);
	  index.Alpha = NULL;
	}
      if ((matchSSB) && (hasSSB))
	{
	  free(index.SSB);
	  index.SSB = NULL;
	}      
      err = M2TXIndex_ToUncompr(&newTex, oldPIP, i, &index);
      if (err != M2E_NoErr)
	{
	  fprintf(stderr,"ERROR:Error during conversion, must abort.\n");
	  return(-1);
	}
      M2TXIndex_Free(&index);
      M2TXHeader_FreeLODPtr(header,i);
    }

  M2TX_WriteFile(fileOut,&newTex);    		/* Write it to disk */
  M2TX_Free(&newTex, TRUE, TRUE, TRUE, TRUE);
  
  return(0);
}
