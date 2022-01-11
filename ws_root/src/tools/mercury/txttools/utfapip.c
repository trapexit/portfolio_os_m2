/*
   File:	utfapip.c
   
   Contains:	Take a pip with color values and a PIP with alpha
   values and construct a pip all the combinations
   
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
|||	AUTODOC -public -class tools -group m2tx -name utfapip
|||	Sets all transparent (Alpha=0) pixels to black
|||	
|||	  Synopsis
|||	
|||	    utfsdfpage <color PIP> <alpha PIP> <combination PIP>
|||	
|||	  Description
|||	
|||	    This tool takes a two PIPs, uses the colors in the first PIP, and
|||	    the alpha values of the second, and creates a PIP that has all 
|||	    combinations of alpha/color plus a transparency value (R=G=B=A=0)
|||	    This is useful for animated series of images that have been quantized
|||	    to a common PIP and can alpha values (also quantized).
|||	
|||	  Arguments
|||	
|||	    <color PIP>
|||	        The UTF file which contains a color PIP.
|||	    <alpha PIP>
|||	        The UTF file which contains a color PIP.
|||	    <output file>
|||	        The resulting combination PIP
|||	
|||	  See Also
|||	
|||	    utfquantmany, quanttopip
**/



#include "M2TXlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   Create a PIP of all alpha and color/ssb combinations\n");
  printf("   <color PIP> \tThe UTF file PIP contains the color and ssb values you want to use.\n");
  printf("   <alpha PIP> \tThe UTF file PIP contains the alpha values you want to use.\n");
}

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
	      fprintf(stderr,"ERROR:Can't find the right color, alpha, ssb combo\n");
	      return(M2E_Range);
	    }
	}	
      else
	{
	  fprintf(stderr,"ERROR:Can't find the right color, alpha, ssb combo\n");
	  return(M2E_Range);
	}
    }
  M2TXHeader_SetCDepth(header,cDepth);
  if (needSSB)
    M2TXHeader_SetFHasSSB(header, TRUE);
}

#define Usage fprintf(stderr,"Usage: %s <color PIP> <alpha PIP> <Output File>\n", argv[0])

int main( int argc, char *argv[] )
{
  M2TX colTex, alphaTex, newTex;
  M2TXHeader *colHeader, *alphaHeader, *newHeader;
  M2TXPIP *colPIP, *alphaPIP, *newPIP;
  char fileCol[256];
  char fileOut[256];
  char fileAlpha[256];
  uint8 r, g, b, a, ssb, ar, ag, ab, aa, assb;
  int  pipIndex, pipCDepth, i, j;
  M2TXColor color;
  FILE *fPtr;
  bool hasPIP;
  M2Err err;
  
#ifdef M2STANDALONE	
  printf("Enter: <color PIP> <alpha PIP> <final PIP>\n");
  printf("Example: color.pip alpha.pip new.pip\n");
  fscanf(stdin,"%s %s %s",fileCol, fileAlpha, fileOut);
  
#else
  /* Check for command line options. */
  if (argc != 4)
    {
      Usage;
      print_description();
      return(-1);    
    }
  strcpy(fileCol, argv[1]);
  strcpy(fileAlpha, argv[2]);    
  strcpy(fileOut, argv[3]);
#endif
  
  fPtr = fopen(fileCol, "r");
  if (fPtr == NULL)
    {
      fprintf(stderr,"ERROR:Can't open file \"%s\" \n",fileCol);
      return(-1);
    }
  else 
    fclose(fPtr);		
  
  M2TX_Init(&colTex);			/* Initialize a texture */
  M2TX_Init(&alphaTex);		        /* Initialize a texture */
  M2TX_Init(&newTex);
  err = M2TX_ReadFileNoLODs(fileCol,&colTex);
  if (err != M2E_NoErr)
    return(-1);
  err = M2TX_ReadFileNoLODs(fileAlpha,&alphaTex);
  if (err != M2E_NoErr)
    return(-1);
  
  M2TX_GetHeader(&colTex,&colHeader);
  M2TXHeader_GetFHasPIP(colHeader, &hasPIP);
  if (!hasPIP)
    {
      fprintf(stderr,"ERROR:I need a pip in the color file \"%s\"\n",
	      fileCol);
      return(-1);
    }
  M2TX_GetHeader(&alphaTex,&alphaHeader);	
  M2TXHeader_GetFHasPIP(alphaHeader, &hasPIP);
  if (!hasPIP)
    {
      fprintf(stderr,"ERROR:I need a pip in the alpha file \"%s\"\n",
	      fileAlpha);
      return(-1);
    }

  M2TX_Copy(&newTex, &colTex);
  /*  M2TX_SetHeader(&newTex, header); */

  M2TX_GetHeader(&newTex,&newHeader);	
  M2TX_GetPIP(&colTex,&colPIP);
  M2TX_GetPIP(&alphaTex,&alphaPIP);
  M2TX_GetPIP(&newTex,&newPIP);

  pipIndex = 0;
  for(i=0; i<alphaPIP->NumColors; i++)
    {
      M2TXPIP_GetColor(alphaPIP, i, &color);
      M2TXColor_Decode(color, &assb, &aa, &ar, &ag, &ab);
      if (aa != 0)   /* Totally transparent is reserved for a single color */
	for (j=0; j<colPIP->NumColors; j++)
	  {
	    M2TXPIP_GetColor(colPIP, j, &color);
	    M2TXColor_Decode(color, &ssb, &a, &r, &g, &b);
	    color = M2TXColor_Create(ssb, aa, r, g, b);
	    M2TXPIP_SetColor(newPIP, pipIndex, color);
	    pipIndex++;
	  }
    }

  color = M2TXColor_Create(0, 0, 0, 0, 0);
  M2TXPIP_SetColor(newPIP, pipIndex, color);
  pipIndex++;

  M2TX_SetPIP(&newTex, newPIP);
  M2TXHeader_SetFHasPIP(newHeader, TRUE);	

  M2TXPIP_SetNumColors(newPIP, pipIndex);
  pipCDepth = 1;
  while ( pipIndex > (1<<pipCDepth))
    pipCDepth++;
  
  fprintf(stderr,"Number of colors=%d PIP CDepth=%d\n",pipIndex, 
	  pipCDepth);
  M2TXHeader_SetCDepth(newHeader, pipCDepth);
  make_legal(newHeader);

  M2TX_WriteFile(fileOut,&newTex);    	/* Write it to disk */
  return(0);  
}

