/*
	File:		utfpipcat.c

	Contains:     Concatenate several smaller PIPs into a single PIP.

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
|||	AUTODOC -public -class tools -group m2tx -name utfpipcat
|||	Concatenate multiple PIPs into a single PIP file
|||	
|||	  Synopsis
|||	
|||	    utfpipcat <pip output file> <input files>
|||	
|||	  Description
|||	
|||	    This tool takes PIPs from UTF files, and concatenates them into
|||	    a single PIP.  For instance 4 64-color PIPs could be concatenated
|||	    into a single PIP with 256 entries.
|||	
|||	  Arguments
|||	
|||	    <input files>
|||	        The input UTF files with PIPs.
|||	    <output file>
|||	        The output UTF file with a combination PIP.
|||	
|||	
|||	  Caveats
|||	
|||	    UTF files PIPs have a maximum of 256 entries.
|||	    
|||	  See Also
|||	
|||	    utfpage
**/

#include "M2TXlib.h"
#include "M2TXattr.h"
typedef int32 Err;
#include "clt.h"
#include "clttxdblend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   UTF PIP Concatenate\n");
}


int main( int argc, char *argv[] )
{
  M2TX tex;
  char fileOut[256];
  M2Err err;
  uint32 i, newColors;
  int argn;
  int16 numColors;
  bool flag;
  M2TXPIP newPIP, *pip;
  M2TXHeader *header;

  if (argc < 4)
    {
      fprintf(stderr,"Usage: %s <Output File> <PIP file1> <PIP file2..\n",argv[0]);
      print_description();
      return(-1);	
    }	
  else
    strcpy(fileOut, argv[1]);
  
  argn = 2;
  newColors = 0;
  M2TXPIP_SetNumColors(&newPIP, 0);
  while (argn < argc)
    {
      M2TX_Init(&tex);		/* Initialize a texture */
      
      err = M2TX_ReadFileNoLODs(argv[argn],&tex);
      if (err != M2E_NoErr)
	{
	  fprintf(stderr,"ERROR:Error during read.  Aborting\n");
	  return(-1);
	}
      M2TX_GetHeader(&tex, &header);
      M2TXHeader_GetFHasPIP(header, &flag);
      if (flag)
	{
	  M2TX_GetPIP(&tex, &pip);
	  M2TXPIP_GetNumColors(pip, &numColors);
	  if (numColors+newColors > 256)
	    {
	      fprintf(stderr,"ERROR:Number of accumulated colors exceeds 256:%d.\n",numColors+newColors);
	      return(-1);
	    }
	  
	  for (i=0; i<numColors; i++)
	    {
	      newPIP.PIPData[newColors] = pip->PIPData[i];
	      /*  fprintf(stderr,"newPIP %d:%x\n",newColors, 
		  newPIP.PIPData[newColors]); */
	      newColors++;
	    }
	  M2TXPIP_SetNumColors(&newPIP, newColors);
	}
      argn++;
    }
  M2TX_SetPIP(&tex, &newPIP);
  M2TX_WriteFile(fileOut, &tex);
  return(0);
}


  







