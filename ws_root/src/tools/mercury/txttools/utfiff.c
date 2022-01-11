/*
	File:		utfiff.c

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

 #include "M2TXlib.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>

 int main( int argc, char *argv[] )
 {
   M2TX tex;
   M2TXHeader *header, *newHeader;
   char fileIn[256];
   char fileOut[256];
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
       printf("Usage: %s <Input File> <Output File>\n",argv[0]);
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
       printf("Can't open file \"%s\" \n",fileIn);
       return(-1);
     }
   else 
     fclose(fPtr);		
   M2TX_Init(&tex);		/* Initialize a texture */
   
   err = M2TX_ReadFile(fileIn,&tex);
   if (err != M2E_NoErr)
     {
       printf("Error during read\n");
       return(-1);
     }
   M2TX_WriteFile(fileOut,&tex);    	/* Write it to disk */
   return(0);
}
