/*
	File:		utfunpage.c

	Contains:     Takes all the textures in a utf page and writes them into separate utf files.

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
|||	AUTODOC -public -class tools -group m2tx -name utfunpage
|||	Takes a UTF texture and decomposes any texture pages into separate textures.
|||	
|||	  Synopsis
|||	
|||	    utfunpage <input file> <output base name>
|||	
|||	  Description
|||	
|||	    This tool takes in a UTF texture and looks for texture pages.
|||	    If it finds a texture page, it writes out the individual textures in 
|||	    the page as separarte UTF files
|||	
|||	  Arguments
|||	
|||	    <input file>
|||	        The input UTF texture.
|||	    <output base name>
|||	        The prefix of the UTF output textures' file names.
|||	
|||	
|||	  See Also
|||	
|||	    utfpage
**/


#include<stdio.h>

/* Huh!? */
#ifdef applec
#include "M2TXlib.h"
#include "ifflib.h"
#else
#include "ifflib.h"
#include "M2TXlib.h"
#endif

#include <stdlib.h>
#include <string.h>

void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   Decompose a UTF texture page into separate textures.\n");
}

#define Usage  printf("Usage: %s <Input File> <Output base name>\n",argv[0])

#define MAX_TEXTURES 256

Err TXTR_GetNext(IFFParser *iff, M2TX *tex, bool *foundM2);
Err TXTR_SetupFrame(IFFParser *iff, bool inLOD);

M2Err M2TX_OpenFile(char *fileName, IFFParser **iff, bool writeMode, bool isMac, void *spec);


long padPixels(int Pixels, int depth)
{
  long bits, remainder;

  bits = Pixels * depth;
  remainder = bits % 32;
  if (remainder)
    bits += (32-remainder);
  return(bits>>3);
}


int main( int argc, char *argv[] )
{
  uint16        xSize, ySize;
  M2TX          baseTex, newTex;
  int           nTextures;
  bool          hasPIP, hasColor, hasAlpha, hasSSB;
  bool          isCompressed;
  bool          noPIP = FALSE;
  bool          floyd = FALSE;
  bool          foundM2, isLiteral;
  M2TXHeader    *header, *newHeader;
  char          fileOut[256];
  char          baseName[256];
  char          fileIn[256];
  uint8         numLOD, cDepth, aDepth, ssbDepth;
  int argn;
  M2Err         err;

  M2TXPIP       *oldPIP;
  M2TXPIP       *newPIP;
  int           totalBytes, lodPixels, texelDepth;
  M2TXTex       bigPtr, lodPtr;
  M2TXFormat    baseFormat;
  int16         numColors;
  M2TXPgHeader  *pgArray;
  IFFParser     *iff;
  Err           result;

  int           lodLength, i,j, offset, texIndex;
 
  /* Check for command line options. */
  if (argc < 3)
    {
      print_description();
      Usage;
      return(-1);    
    }
  
  strcpy(fileIn, argv[1]);

  argn = 2;
  while ( argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0' )
    {
	  Usage;
	  return(-1);
    }

  strcpy(baseName, argv[argn]);

  err = M2TX_OpenFile(fileIn, &iff, FALSE, FALSE, NULL); 
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Can't open file \"%s\"\n",fileOut);
      return(-1);
    }

  result = TXTR_SetupFrame(iff, TRUE);
  if (result<0)
    {
      fprintf(stderr,"ERROR:Can't setup the frame.\n",fileOut);
      return(-1);
    }

  result = 0;
  texIndex = 0;
  while(result >= 0)
    {
      M2TX_Init(&newTex);
      M2TX_Init(&baseTex);
      result = TXTR_GetNext(iff, &baseTex, &foundM2);
      if (foundM2 && (result>=0))
	{
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"Error during reading of file \"%s\"\n", fileIn);
	      return(-1);
	    }

	  M2TX_GetHeader(&baseTex,&header);
	  M2TXHeader_GetFHasPIP(header, &hasPIP);
	  M2TXHeader_GetTexFormat(header, &baseFormat);
	  M2TX_SetHeader(&newTex, header); 
	  M2TX_GetHeader(&newTex, &newHeader);
	  M2TX_GetPIP(&newTex, &newPIP);
	  M2TX_GetPIP(&baseTex, &oldPIP);
	  M2TXPIP_GetNumColors(oldPIP, &numColors);
	  newTex.Page.NumTex = 0;
	  
	  nTextures = baseTex.Page.NumTex;
	  pgArray = baseTex.Page.PgData;
	  
	  for (i=0; i<nTextures; i++)
	    {
	      /* Take care of the Header */
	      M2TXHeader_SetMinXSize(newHeader, pgArray[i].MinXSize);
	      M2TXHeader_SetMinYSize(newHeader, pgArray[i].MinYSize);
	      M2TXHeader_SetNumLOD(newHeader, pgArray[i].NumLOD);
	      if (pgArray[i].PgFlags & M2PG_FLAGS_HasTexFormat)
		{
		  M2TXHeader_SetTexFormat(newHeader, pgArray[i].TexFormat);
		}
	      else
		M2TXHeader_SetTexFormat(newHeader, baseFormat);

	      if (pgArray[i].PgFlags & M2PG_FLAGS_IsCompressed)
		{
		  isCompressed = TRUE;
		  M2TXHeader_SetFIsCompressed(newHeader, TRUE);
		  for (j=0; j<4; j++)
		    {
		      newTex.DCI.TexelFormat[j] = pgArray[i].TexelFormat[j];
		      newTex.DCI.TxExpColorConst[j] = pgArray[i].TxExpColorConst[j];		      
		    }
		}
	      else
		{
		  isCompressed = FALSE;
		  M2TXHeader_SetFIsCompressed(newHeader, FALSE);
		}
	      /* The Load Rects */
	      if (pgArray[i].PgFlags & M2PG_FLAGS_XWrapMode)
		newTex.LoadRects.LRData[0].XWrapMode = 1;
	      else
		newTex.LoadRects.LRData[0].XWrapMode = 0;
	      if (pgArray[i].PgFlags & M2PG_FLAGS_YWrapMode)
		newTex.LoadRects.LRData[0].YWrapMode = 1;
	      else
		newTex.LoadRects.LRData[0].YWrapMode = 0;
	      
	      /* The PIP */
	      if (hasPIP)
		{
		  offset = pgArray[i].PIPIndexOffset;
		  M2TXPIP_SetNumColors(newPIP, numColors-offset);
		  for (j=offset; j<numColors; j++)
		    newPIP->PIPData[j-offset] = oldPIP->PIPData[j];
		}
	      
	      /* The LODs */
	      M2TXHeader_GetFHasColor(newHeader, &hasColor);
	      if (hasColor)
		M2TXHeader_GetCDepth(newHeader, &cDepth);
	      else
		cDepth = 0;
	      M2TXHeader_GetFHasAlpha(newHeader, &hasAlpha);
	      if (hasAlpha)
		M2TXHeader_GetADepth(newHeader, &aDepth);
	      else
		aDepth = 0;
	      M2TXHeader_GetFHasSSB(newHeader, &hasSSB);
	      if (hasSSB)
		ssbDepth = 1;
	      else 
		ssbDepth = 0;
	      M2TXHeader_GetFIsLiteral(newHeader, &isLiteral);
	      if (isLiteral)
		texelDepth = 3*cDepth + aDepth + ssbDepth;
	      else
		texelDepth = cDepth + aDepth + ssbDepth;
			      
	      totalBytes = pgArray[i].Offset; 
	      bigPtr = header->LODDataPtr[0] + totalBytes;
	      numLOD = pgArray[i].NumLOD;
	      xSize = (pgArray[i].MinXSize<<(numLOD-1));
	      ySize = (pgArray[i].MinYSize<<(numLOD-1));
	      
	      for (j=0; j<numLOD; j++)
		{
		  if (isCompressed)
		    {
		      lodLength = pgArray[i].LODLength[j];
		    }
		  else
		    {
		      lodPixels = xSize*ySize;
		      lodLength = padPixels(lodPixels, texelDepth);
		      xSize = xSize>>1;
		      ySize = ySize>>1;
		    }
		  lodPtr = (M2TXTex)calloc(lodLength,1);
		  memcpy(lodPtr, bigPtr, lodLength);
		  newHeader->LODDataPtr[j] = lodPtr;
		  newHeader->LODDataLength[j] = lodLength;
		  if (lodPtr == NULL)
		    {
		      fprintf(stderr,"ERROR:Out of Memory\n");
		      return(-1);
		    }
		  bigPtr += lodLength;
		}
	      sprintf(fileOut, "%s.%d.%d.utf",baseName,texIndex,i);
	      M2TX_WriteFile(fileOut, &newTex);
	      M2TXHeader_FreeLODPtrs(newHeader);
	    }
	  texIndex++;
	}
    }
  result = DeleteIFFParser(iff);
  if (result<0)
    fprintf(stderr,"Error during Parser deallocation.  Who cares?\n");
  
  M2TXHeader_FreeLODPtrs(header);
  return(0);
}
