/*
	File:		utfpipaccum.c

	Contains:	Find a common pip across several files.

	Written by:	Todd Allendorf 

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

	To Do:
*/


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
  printf("   UTF PIP ACCUMULATE\n");
}


int main( int argc, char *argv[] )
{
  M2TX tex;
  char fileOut[256];
  FILE *fPtr;
  M2Err err;
  uint32 i, j, val, newColors;
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
	  for (j=0; j<numColors; j++)
	    {
	      for (i=0; i<newColors; i++)
		{
		  if(pip->PIPData[j] == newPIP.PIPData[i])
		    break;
		}
	      if (i>= newColors)
		{
		  if (newColors >= 256)
		    {
		      fprintf(stderr,"ERROR:Too many accumulated colors.\n");
		      return(-1);
		    }
		  newPIP.PIPData[newColors] = pip->PIPData[j];
		  fprintf(stderr,"newPIP %d:%x\n",newColors, 
			  newPIP.PIPData[newColors]);
		  newColors++;
		  M2TXPIP_SetNumColors(&newPIP, newColors);
		}
	    }
	}
      argn++;
    }
  M2TX_SetPIP(&tex, &newPIP);
  M2TX_WriteFile(fileOut, &tex);
  return(0);
}


  







