/*
	File:		utfatmod.c

	Contains:	Modify TAB and DAB settings.

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
|||	AUTODOC -public -class tools -group m2tx -name utfatmod
|||	Modify TAB and DAB settings of a UTF image
|||	
|||	  Synopsis
|||	
|||	    utfatmod <input file> <mod file> [options] <output file>
|||	
|||	  Description
|||	
|||	    This tool takes a UTF file and a "modification file", and
|||	    outputs a UTF file that has the TAB and DAB modified according
|||	    to the texture file (which is in the format of the TexBlend 
|||	    data structure of the ASCII SDF format).
|||	
|||	  Arguments
|||	
|||	    <input file>
|||	        The input UTF texture.
|||	    <modification file>
|||	        The ASCII SDF (partial) file with the TexBlend modifications
|||	    <output file>
|||	        The resulting UTF texture.
|||	
|||	  Options
|||	
|||	  See Also
|||	
|||	    utfbatchmod
**/


#include<stdio.h>
#include "M2TXlib.h"
#include "M2TXattr.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
typedef int32 Err;
typedef float gfloat;
#include "os.h"
#include "LWSURF.h"
#include "lws.i"
#include "lws.h"
#include "SDFTexBlend.h"
#include "clt.h"
#include "clttxdblend.h"


void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   Modifies TAB and DAB settings based on the TexBlend file\n");
}

#define Usage  printf("Usage: %s <Input File> <Modification File> <Output File>\n",argv[0])

extern LWS *cur_lws;
M2Err Texture_Entry(int tokenType, SDFTex *tb);

M2Err Texture_ReadModFile(char *fileIn, SDFTex *texOut)
{
  LWS lws;
  ByteStream*		stream = NULL;
  int  tokenType = T_RBRACE;
  
  strcpy(lws.FileName, fileIn);
  stream = K9_OpenByteStream(lws.FileName, Stream_Read, 0);
  if (stream == NULL)
    {
      return (M2E_BadFile);
    }
  lws.Stream = stream;
  lws.Parent = cur_lws;
  cur_lws = &lws;

  SDFTex_Init(texOut);
  token[0]='#';
  Texture_Entry(tokenType, texOut);

  return(M2E_NoErr);
}


int main( int argc, char *argv[] )
{

  ByteStream*		stream = NULL;

  M2TX          tex;
  bool          noPIP = FALSE;
  bool          floyd = FALSE;
  bool          result;
  M2TXHeader    *header;
  char          fileIn[256];
  char          refFile[256];
  char          fileOut[256];
  int argn;
  M2Err         err;
  FILE          *fPtr;
  SDFTex        sdfTex;
  uint32        attr, sdfVal;

 
#ifdef M2STANDALONE
  printf("Enter: <FileIn> <Modification File> <FileOut>\n");
  printf("Example: dumb.utf TexBlend.sdf dumb.mod.utf\n");
  fscanf(stdin,"%s %s %s",fileIn, refFile, fileOut);
#else
  /* Check for command line options. */
  if (argc < 4)
    {
      Usage;
      print_description();
      return(-1);
    }
  strcpy(fileIn, argv[1]);
  strcpy(refFile, argv[2]);
  argn = 3;
  while ( argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0' )
    {
      Usage;
      return(-1);
    }
  if ( argn != argc )
    {
      /* Open the output file. */
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
  
  err = M2TX_ReadFile(fileIn,&tex);
  if (err != M2E_NoErr)
    return(-1);
  M2TX_GetHeader(&tex,&header);

  Texture_ReadModFile(refFile, &sdfTex);
		  
  attr=0;
  while(attr<TXA_NoMore)
    {
      result = SDFTex_GetTAB(&sdfTex, attr, &sdfVal);
      if (result)
	M2TXTA_SetAttr(&(tex.TexAttr), attr, sdfVal);
      attr++;
    }

  attr=0;
  while(attr<DBLA_NoMore)
    {
      result = SDFTex_GetDAB(&sdfTex, attr, &sdfVal);
      if (result)
	M2TXDB_SetAttr(&(tex.DBl), attr, sdfVal);
      attr++;
    }
  
  M2TX_WriteFile(fileOut, &tex);
  return(0);
}

