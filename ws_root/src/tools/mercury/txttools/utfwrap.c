/*
	File:		utfwrap.c

	Contains:	Change wrapmode setttings

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   UTF Tile.\n");
  printf("   -xtile \tTile in the x direction.\n");
  printf("   -ytile \tTile in the y direction.\n");
  printf("   -xclamp \tClamp in the x direction.\n");
  printf("   -yclamp \tClamp in the y direction.\n");
}

#define Usage printf("Usage: %s <Input File> [-xclamp|-xtile] [-yclamp|-ytile] \n",argv[0])


int main( int argc, char *argv[] )
{
  M2TX tex;
  M2TXHeader *header;
  char fileIn[256];
  FILE *fPtr;
  M2Err err;
  int argn;

  /* Check for command line options. */
  if (argc < 3)
    {
      Usage;
      print_description();
      return(-1);    
    }

  strcpy(fileIn, argv[1]);

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
    {
      fprintf(stderr,"ERROR:Bad file \"%s\" \n",fileIn);
      return(-1);
    }


  argn = 2;
  while ((argn < argc) && argv[argn][0]=='-' && argv[argn][1] != '\0')
    {
      if ( strcmp( argv[argn], "-xtile")==0 )
        {
	  ++argn;
	  tex.LoadRects.LRData[0].XWrapMode = 1;
        }
      else  if ( strcmp( argv[argn], "-xclamp")==0 )
        {
	  ++argn;
	  tex.LoadRects.LRData[0].XWrapMode = 0;
        }
      else if ( strcmp( argv[argn], "-ytile")==0 )
        {
	  ++argn;
	  tex.LoadRects.LRData[0].YWrapMode = 1;
        }
      else  if ( strcmp( argv[argn], "-yclamp")==0 )
        {
	  ++argn;
	  tex.LoadRects.LRData[0].YWrapMode = 0;
        }
      else
	{
	  Usage;
	  return(-1);
	}
    }
    M2TX_WriteFile(fileIn,&tex);    	/* Write it to disk */ 
  return(0);
}






