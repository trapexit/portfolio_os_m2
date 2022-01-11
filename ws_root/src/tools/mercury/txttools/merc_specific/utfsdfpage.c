/*
	File:		utfsdfpage.c

	Contains:	Reads an SDF file and creates a TexPaage array and Texture Page

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
|||	AUTODOC -public -class tools -group m2tx -name utfsdfpage
|||	Create a mercury texture page using only a TexArray
|||	
|||	  Synopsis
|||	
|||	    utfsdfpage <input file> [options] <output UTF file> <output SDF TexArray>
|||	
|||	  Description
|||	
|||	    This tool takes an ASCII SDF file and creates a TexPageArray file or a TexArray
|||	    with PageIndex entries (for use with gmerc) and a texture page file.  It takes every texture
|||	    in the TexArray of the ASCII SDF file and resizes it to fit into 16K.
|||	    It also does color if a bitdepth of 8 or less is specified (and if 
|||	    necessary).  Every texture will map to one Mercury texture page.
|||	    It will also produce a script file with all the necessary steps to 
|||	    produce a texture page using standalone command-line tools so they can
|||	    be modified in the future. If the textures referenced by the input SDF
|||	    file are already under 16K they will be left alone.
|||	
|||	  Arguments
|||	
|||	    <input file>
|||	        The input text file with an SDF TexArray and TexPageArray.
|||	    <output file>
|||	        The resulting UTF texture (which is a Mercury Texture Page).
|||	    <tex page file>
|||	        The ASCII SDF TexPageArray file needed by gmerc
|||	
|||	  Options
|||	
|||	    -pip pipfile
|||	        Use the PIP of this file as the PIP for all >16K textures
|||	    -lod 
|||	        Number of LODs to create for every >16K texture encountered. Default=4
|||	    -old
|||	        Output the old TexPageArray format instead of the new PageIndex format.
|||	    -floyd
|||	        Perform Floyd-Steinberg dithering during color reduction (Default Off)
|||	    -depth <Color Depth
|||	        Color bitdepth for every >16K texture encountered. (Default 8)
|||	    -s <script file> 
|||	        redirect the commands to reproduce the page from stdout to this file
|||	
|||	  Caveats
|||	
|||	    Literal Alpha and SSB channels will remain unaffected by the quantization
|||	    process.  Please be sure to remove any extraneous literal alpha and ssb
|||	    channels from the input UTF images before executing utfsdfpage.
|||	    All textures will be resized to fit into 16K.  They will use a sinc filter
|||	    for color and alpha and point sampling for ssb channels, if present.
|||	    Textures larger than 16K will have their PIPs reordered when they are resized
|||	    and then quantized.  Compressed textures will be turned into uncompressed 
|||	    textures.
|||	    Currently, textures are mapped with by their file names.  Therefore, it is
|||	    not possible to have two unique textures that share the same UTF file.  This
|||	    is a limitation of the ASCII SDF parser that will be addressed in a future release
|||	    so that multiple TexArray entries can share a single texture file (and one copy of
|||	    texel data).
|||	
|||	  See Also
|||	
|||	    utfquantmany, utfpage, utfunpage, utfsplit
**/



#include <stdio.h>

/* Huh!? */
#ifdef applec
#include "M2TXlib.h"
#include "ifflib.h"
#else
#include "ifflib.h"
#include "M2TXlib.h"
#endif

#include "M2TXattr.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

int CommentLevel = 5;
#include "qmem.h"
#include "os.h"
#include "LWSURF.h"
#include "lws.i"
#include "lws.h"
#include "SDFTexBlend.h"
#include "clt.h"
#include "clttxdblend.h"
#include  "utfpage.h"
#include  "page.h"
#include <math.h>

extern LWS *cur_lws;

#ifdef applec 
int strcasecmp(char *s, char *t);
#endif

M2Err M2TX_OpenFile(char *fileName, IFFParser **iff, bool writeMode, bool isMac, void *spec);
Err M2TX_WriteChunkData(IFFParser *iff, M2TX *tex, uint32 write_flags );

M2Err Texture_Append(M2TXHeader *header, uint32 *totalBytes, TxPage *tp, bool doCopy)
{
  M2TXPgHeader *subPgHeader;
  bool         isCompressed;
  M2TX         *subTex;
  uint16        xSize, ySize;
  uint8         numLOD;
  uint32        lodSize, i;
  M2TXHeader    *subHeader;
  M2TXTex       texPtr;
  
  texPtr = (M2TXTex)(header->LODDataPtr[0] + *totalBytes);
  subPgHeader = &(tp->PgHeader);
  subTex = tp->Tex;
  M2TX_GetHeader(subTex, &subHeader);

  /* Could be done at load time */

  M2TXHeader_GetNumLOD(subHeader, &numLOD);
  M2TXHeader_GetMinXSize(subHeader, &xSize);
  M2TXHeader_GetMinYSize(subHeader, &ySize);
  M2TXHeader_GetFIsCompressed(subHeader, &isCompressed);
  
  if (isCompressed)
    {
      fprintf(stderr, "ERROR:A compressed texture. Not Allowed.\n");
      return(M2E_BadFile);
    }
  
  /*
    if (*totalBytes == 0)
    header->TexFormat = subHeader->TexFormat;
    */
  
  subPgHeader->NumLOD = numLOD;
  subPgHeader->TexFormat = subHeader->TexFormat;
  if (subPgHeader->TexFormat != header->TexFormat)
    subPgHeader->PgFlags |= M2PG_FLAGS_HasTexFormat;
  subPgHeader->MinXSize = xSize;
  subPgHeader->MinYSize = ySize;
  subPgHeader->Offset = *totalBytes;
  if (doCopy==TRUE)
    for (i=0; i<numLOD; i++)
      {
	lodSize = subHeader->LODDataLength[i];
	*totalBytes += lodSize;
	if ((*totalBytes) > TRAM_SIZE)
	  {
	    return(M2E_Range);
	  }
	memcpy(texPtr, subHeader->LODDataPtr[i], lodSize);
	texPtr += lodSize;
      }
  return(M2E_NoErr);
}
 
void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   Take an ASCII SDF file and creates a UTF Page and an ASCII SDF TexPageArray file.\n");
  printf("   -pip pipfile  \tUse this pip as the PIP for every texture >16K.\n");
  printf("   -floyd        \tUse Floyd-Steinberg dithering for quantization >16K textures\n");
  printf("   -old          \tOutput the old TexPageArray format.\n");
  printf("   -lod <# LODs> \tNumber of LODs to produce for >16K textures. Default 4\n");
  printf("   -depth <Color Depth>\tColor bitdepth for every >16K texture encountered. Default 8\n");
  printf("   -s <script file> \tRedirect the commands to reproduce the page from stdout to this file\n");
}

#define Usage  printf("Usage: %s <Input File> [options] <Output UTF Page File> <Output SDF TexArray>\n",argv[0])


SDF_Arrange(SDFTex *sdfTex, int curTex, PageNames **myPages, int *cP, int *nP, bool pageIndexMode)
{
  PageNames *pageNames, *pgTmp, *pn;
  int i, j, k;
  M2Err err;
  int curPage, curName, nPages;
  bool done;
 
  curPage = 0;
  curName = 0;
  pageNames = *myPages;
  nPages = *nP;

  for (i=0; i<curTex; i++)
    {
      if ((sdfTex[i].FileName == NULL) || (!strcasecmp("(none).utf", sdfTex[i].FileName)))
	{
	  if (pageIndexMode==FALSE)
	    {
	      fprintf(stderr,"WARNING: You have a TexArray entry with no texture.\n");
	      fprintf(stderr,"This will not be useable unless you switch to PageIndex notation.\n");
	    }
	  else
	    {
	      fprintf(stderr,"No texture file, going in first page sub-texture %d\n", pageNames[curPage].NumNames);
	      pn = &(pageNames[0]);
	      err = PageNames_Extend(pn, 1);
	      if (err != M2E_NoErr)
		{
		  fprintf(stderr,"ERROR: Errors occured in SDF_Arrange\n");
		  return(err);
		}
	      curName = pn->NumNames;
	      pn->SubNames[curName] = NULL;
	      pn->NumNames = curName+1;
	      sdfTex[i].PageIndex[0] = curPage;
	      sdfTex[i].PageIndex[1] = curName;
	    }
	}
      else
	{
	  done = FALSE;
	  for (j=0; (j<i) && (!done); j++)
	    {
	      pn = &(pageNames[j]);
	      for (k=0; k<pn->NumNames; k++)
		{
		  if (!strcasecmp(pn->SubNames[k], sdfTex[i].FileName))
		    {
		      err = PageNames_Extend(pn, 1);
		      if (err != M2E_NoErr)
			{
			  fprintf(stderr,"ERROR: Errors occured in PageName_Entry\n");
			  return(err);
			}
		      curName = pn->NumNames;
		      pn->SubNames[curName] = (char *)calloc(strlen(sdfTex[i].FileName)+4,1);
		      if (pn->SubNames[curName] == NULL)
			return(M2E_NoMem);
		      strcpy(pn->SubNames[curName], sdfTex[i].FileName);
		      pn->NumNames = curName+1;
		      sdfTex[i].PageIndex[0] = j;
		      sdfTex[i].PageIndex[1] = curName;
		      done = TRUE;
		      break;
		    }
		}
	    }
	  if (!done)
	    {
	      pn = &(pageNames[curPage]);
	      PageNames_Init(pn);
	      sdfTex[i].PageIndex[0] = curPage;
	      err = PageNames_Extend(pn, 1);
	      if (err != M2E_NoErr)
		{
		  fprintf(stderr,"ERROR: Errors occured in PageName_Entry\n");
		  return(err);
		}
	      curName = pageNames[curPage].NumNames;
	      pn->SubNames[curName] = (char *)calloc(strlen(sdfTex[i].FileName)+4,1);
	      if (pn->SubNames[curName] == NULL)
		return(M2E_NoMem);
	      strcpy(pn->SubNames[curName], sdfTex[i].FileName);
	      pn->NumNames = curName+1;
	      sdfTex[i].PageIndex[1] = curName;
	      pageNames[curPage].NumNames = curName + 1;
	      err = PageNames_Extend(pn, 1);
	      if (err != M2E_NoErr)
		{
		  fprintf(stderr,"ERROR: Errors occured in PageName_Entry\n");
		  return(err);
		}
	      
	      curPage++;
	      curName = 0;
	      if (curPage >= (nPages-1))
		{
		  nPages += PAGE_BUF_SIZE;
		  pgTmp = qMemResizePtr(pageNames, nPages*sizeof(PageNames));
		  if (pgTmp == NULL)
		    {
		      fprintf(stderr,"ERROR:Out of memory in TexPageArray_ReadFile!\n");
		      return(M2E_NoMem);
		    }
		  pageNames = pgTmp;
		}
	    }
	}
    }
  *myPages = pageNames;
  *cP = curPage;
  *nP = nPages; 
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

long ComputeLODSize(int numLOD, double depth, int xSize, int ySize)
{

  long lod, size, totalSize;
  
  
  totalSize = 0;
  for (lod=0; lod < numLOD; lod++)
    {
      size = (long)ceil(xSize*ySize*depth);
      if (size%32)
	size = (size/32)*32+32;
      totalSize += size;
      xSize = xSize>>1;
      ySize = ySize>>1;
    }
  return(totalSize);
}


M2Err Quantize(M2TX *tex, int numColors, bool extPIP, bool floyd, uint8 cDepth, M2TX *pipTex )
{
  M2TXRaw       raw;
  M2TX          newTex;
  bool          isCompressed, hasColor, hasAlpha, hasSSB;
  bool          firstLOD, done;
  bool          alpha = FALSE;
  M2TXHeader    *header, *newHeader;
  uint8         numLOD;
  int           lod;
  M2Err         err;

  M2TXPIP       *oldPIP, *newPIP;
 

  M2TX_Init(&newTex);			/* Initialize a texture */
  
  M2TX_GetHeader(tex,&header);
  M2TX_GetHeader(&newTex,&newHeader);
  /*  M2TX_SetHeader(&newTex, header); */
  M2TX_Copy(&newTex, tex);
  /* Old texture header to the new texture */
  M2TXHeader_GetFIsCompressed(header, &isCompressed);
  M2TXHeader_GetFHasColor(header, &hasColor);
  M2TXHeader_GetFHasAlpha(header, &hasAlpha);
  M2TXHeader_GetFHasSSB(header, &hasSSB);
  M2TX_GetPIP(tex,&oldPIP);
  M2TX_GetPIP(&newTex, &newPIP);

  M2TXHeader_GetNumLOD(header, &numLOD);
  
  M2TXHeader_SetFIsLiteral(newHeader, FALSE);
  M2TXHeader_SetFHasPIP(newHeader, TRUE);
  M2TXHeader_SetCDepth(newHeader, cDepth);
  make_legal(newHeader);

  firstLOD = TRUE;
  if (!extPIP)
    {
      for (lod=0; lod<numLOD; lod++)
	{
	  if (isCompressed)
	    err = M2TX_ComprToM2TXRaw(tex, oldPIP, lod, &raw);
	  else
	    err = M2TX_UncomprToM2TXRaw(tex, oldPIP, lod, &raw);
	  
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:Error during decompression lod=%d, must abort.\n", lod);
	      return(-1);
	    } 
	  
	  if (lod == (numLOD-1))
	    done = TRUE;
	  else
	    done = FALSE;
	  
	  if (hasColor)
	    err = M2TXRaw_MultiMakePIP(&raw, newPIP, &numColors,
				       firstLOD,done);

	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:An error occured during MakePIP lod=%d:%d\n", lod, err);
	      return(err);
	    }	      
      
	  if (firstLOD)
	    firstLOD = FALSE;
	  M2TXRaw_Free(&raw);
	}
    }
  /*
    fprintf(stderr,"Number of colors used = %d\n", numColors);
    */
  M2TXPIP_SetNumColors(newPIP, numColors);

  for (lod=0; lod<numLOD; lod++)
    {
      if (isCompressed)
	err = M2TX_ComprToM2TXRaw(tex, oldPIP, lod, &raw);
      else
	err = M2TX_UncomprToM2TXRaw(tex, oldPIP, lod, &raw);
      
      if (err != M2E_NoErr)
	{
	  fprintf(stderr,"ERROR:Error during decompression of lod=%d, must abort.\n", lod);
	  return(-1);
	} 
      if (extPIP)
	err = M2TXRaw_MapToPIP (&raw, &(pipTex->PIP), numColors, floyd);
      else
	err = M2TXRaw_MapToPIP (&raw, newPIP, numColors, floyd);
      
      if (err != M2E_NoErr)
	{
	  fprintf(stderr,"ERROR:An error occured during dither of lod=%d:%d\n", lod, err);
	  return(-1);
	}
      if (extPIP)
	err = M2TXRaw_ToUncompr(&newTex, &(pipTex->PIP), lod, &raw);
      else
	err = M2TXRaw_ToUncompr(&newTex, newPIP, lod, &raw);
      if (err != M2E_NoErr)
	{
	  fprintf(stderr,"ERROR:An error occured during encoding of  lod=%d : %d\n", lod, err);
	  return(-1);
	}
      /* Write out the new quantized texture, changing nothing else */
      M2TXRaw_Free(&raw);
    }
  
  
  M2TXHeader_FreeLODPtrs(header);
  
  M2TX_Copy(tex, &newTex);
  if (extPIP)
    M2TX_SetPIP(tex, &(pipTex->PIP));
  else
    M2TX_SetPIP(tex, newPIP);
  M2TXPIP_SetNumColors(&(tex->PIP), numColors);
  return(M2E_NoErr);

}

/* Doesn't do compressed textures yet */
M2Err Fit(M2TX *tex, uint8 cDepth, bool xTile, bool yTile, int nnLOD)
{
  M2TX newTex;
  M2TXPIP *oldPIP;
  M2TXRaw raw, rawLOD;
  M2TXHeader *header, *newHeader;
  M2Err err;
  bool hasPIP, isCompressed, isLiteral, hasColor, hasAlpha, hasSSB;
  bool done;
  int yExp, xExp;
  long bufSize = 16384*8;
  long colorFilter = M2SAMPLE_LANCZS3;
  long indexFilter = M2SAMPLE_POINT;
  long alphaFilter = M2SAMPLE_POINT;
  long ssbFilter = M2SAMPLE_POINT;
  long bits;
  uint8 numLOD, newLODs, aDepth, ssbDepth, divisibleBy;
  uint8 lod, lastLOD;
  uint16 xSize, ySize;
  uint32 lodXSize, lodYSize, curXSize, curYSize;
  double ratio, lodSize, xPixels, yPixels, aspect, newAspect;
  double origXPixels, origYPixels;
  double percentage = 100.00;
  double pixelDepth;
  double fractional;

  M2TX_Init(&newTex);				/* Initialize a texture */
  M2TX_GetHeader(&newTex,&newHeader);
  M2TX_GetHeader(tex, &header);
  M2TX_Copy(&newTex, tex);

  M2TXHeader_GetFIsCompressed(header, &isCompressed);
  M2TXHeader_GetFIsLiteral(header, &isLiteral);
  M2TXHeader_GetFHasColor(header, &hasColor);
  if (hasColor)
    M2TXHeader_SetCDepth(newHeader, 8);
  else
    cDepth = 0;
  M2TXHeader_SetFIsCompressed(newHeader, FALSE);
  M2TXHeader_SetFIsLiteral(newHeader, TRUE);  /* For the Quantization step, we want literal */

  M2TXHeader_GetFHasAlpha(header, &hasAlpha);
  if (hasAlpha)
    M2TXHeader_GetADepth(header, &aDepth);
  else
    aDepth = 0;
  M2TXHeader_GetFHasSSB(header, &hasSSB);
  if (hasSSB)
    ssbDepth = 1;
  else
    ssbDepth = 0;
  M2TXHeader_GetFHasPIP(header, &hasPIP);
  if (hasPIP)
    {
      M2TX_GetPIP(tex,&oldPIP);
    }

  M2TXHeader_GetNumLOD(header, &numLOD);
  err = M2TXHeader_GetMinXSize(header, &xSize);
  err = M2TXHeader_GetMinYSize(header, &ySize);

  curXSize = xSize * (1<<(numLOD-1));
  curYSize = ySize * (1<<(numLOD-1));
 
  newLODs = nnLOD;
  err = M2TXHeader_SetNumLOD(newHeader, newLODs);
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Bad number of levels of detail, aborting!\n");
      return(M2E_Range);
    }
  
  ratio = 1.0;
  for (lod=1; lod<newLODs; lod++)
    ratio += pow(.25, lod);
  divisibleBy = 1<<(newLODs-1);

  pixelDepth = aDepth + cDepth + ssbDepth;

  bufSize = (((long)floor(bufSize*percentage*.01))/32)*32;
  /*
    fprintf(stderr,"Buffer Size = %d bits %d bytes\n", bufSize,
    bufSize/8);
    */
  aspect = (double)xSize/(double)ySize;
  lodSize = bufSize / ratio;
  
  if ((xTile) && (!yTile))
    {
      origXPixels = xPixels = floor(sqrt(lodSize/(double)pixelDepth)*sqrt(aspect));
      
      if (xPixels >= curXSize)
	xPixels = curXSize;
      
      fractional = frexp (xPixels, &xExp);
      if (fractional > 0.00001)
	xPixels = 1 << (xExp);

      if (xPixels>origXPixels)
	{
	  if (fabs(xPixels-origXPixels) > fabs(origXPixels - (1<<xExp-1)))
	    xPixels = 1<< (xExp-1);
	}

      yPixels = xPixels / aspect;
      
      if ((int)yPixels % divisibleBy)
	{
	  done = FALSE;
	  do
	    {
	      yPixels = divisibleBy*((int)yPixels/divisibleBy);
	      bits = ComputeLODSize(newLODs, pixelDepth, xPixels, 
				    yPixels);
	      if (bits>bufSize)
		yPixels -= divisibleBy;
	      else
		done = TRUE;
	    } while (!done);
	}
    }
  else
    {
      origYPixels = yPixels = floor(sqrt(lodSize/(double)pixelDepth)/sqrt(aspect));
      
      if (yPixels >= curYSize)
	yPixels = curYSize;
      
      if (yTile)
	{
	  fractional = frexp (yPixels, &yExp);
	  yPixels = 1 << (yExp);

	  if (yPixels>origYPixels)
	    {
	      if (fabs(yPixels-origYPixels) > fabs(origYPixels - (1<<yExp-1)))
		yPixels = 1<< (yExp-1);
	    }
	}
      else if ((int)(yPixels) % divisibleBy)
	yPixels = divisibleBy*((int)yPixels/divisibleBy);
      
      xPixels = yPixels * aspect;
      if (xTile)
	{
	  fractional = frexp (xPixels, &xExp);
	  done = FALSE;	  
	  do
	    {
	      xPixels = 1 << (xExp);
	      bits = ComputeLODSize(newLODs, pixelDepth, xPixels, 
				    yPixels);
	      if (bits>bufSize)
		xExp--;
	      else
		done = TRUE;
	    } while (!done);
	}
      else if ((int)xPixels % divisibleBy)
	{
	  do
	    {
	      if (((int)xPixels)%divisibleBy)
		xPixels = divisibleBy*(((int)xPixels/divisibleBy)+1);
	      bits = ComputeLODSize(newLODs, pixelDepth, xPixels, 
				    yPixels);
	      if (bits>bufSize)
		xPixels -= divisibleBy;
	      else
		done = TRUE;
	    } while (!done);
	} 
    }
  
  if ((yPixels <1.0) || (xPixels<1.0))
    {
      fprintf(stderr,"ERROR: Can't scale down %dX%d image to be divisible by %d (%d LODs)\n",
	      curXSize, curYSize, divisibleBy, newLODs);
      return(M2E_Range);
    }

  newAspect = xPixels/yPixels;
  /*
    printf("XPixels = %f, YPixels = %f, Old Aspect =%f New Aspect =%f\n",
    xPixels, yPixels, aspect, newAspect);
    */
  lodXSize = (int)xPixels;
  lodYSize = (int)yPixels;

  err = M2TXHeader_SetMinXSize(newHeader, lodXSize>>(newLODs-1));
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Bad xSize, aborting!\n");
      return(M2E_Range);
    }
  
  err = M2TXHeader_SetMinYSize(newHeader, lodYSize>>(newLODs-1));  
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Bad ySize, aborting!\n");
      return(M2E_Range);
    }
  
  for (lod=0; lod<newLODs; lod++)
    {
      if (lod < numLOD)
	lastLOD = lod;
      else
	lastLOD = numLOD-1;
      err = M2TX_UncomprToM2TXRaw(tex, oldPIP, lastLOD, &raw);
      if (err != M2E_NoErr)
	{
	  fprintf(stderr,"ERROR:Raw creation failed!\n");
	  return(err);
	}
      
      if ((raw.XSize == lodXSize) && (raw.YSize == lodYSize))
	{
	  rawLOD.Red = raw.Red;
	  rawLOD.Green = raw.Green;
	  rawLOD.Blue = raw.Blue;
	  rawLOD.Alpha = raw.Alpha;
	  rawLOD.SSB = raw.SSB;
	  rawLOD.HasColor = raw.HasColor;
	  rawLOD.HasAlpha = raw.HasAlpha;
	  rawLOD.HasSSB = raw.HasSSB;
	  rawLOD.XSize = raw.XSize;
	  rawLOD.YSize = raw.YSize;
	}
      else
	{
	  err = M2TXRaw_Init(&rawLOD, lodXSize, lodYSize, hasColor, hasAlpha, 
			     hasSSB);
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:Allcation failed!\n");
	      return(err);
	    }
	}

      if ((raw.XSize != lodXSize) || (raw.YSize != lodYSize))
	{
	  if (hasColor)
	    {
	      err = M2TXRaw_LODCreate(&raw, colorFilter, M2Channel_Colors,
				      &rawLOD);
	      if (err != M2E_NoErr)
		{
		  fprintf(stderr,"ERROR:Resize failed!\n");
		  return(err);
		}
	    }
	  if (hasAlpha)
	    {
	      err = M2TXRaw_LODCreate(&raw, alphaFilter, M2Channel_Alpha, 
				      &rawLOD);
	      if (err != M2E_NoErr)
		{
		  fprintf(stderr,"ERROR:Resize failed!\n");
		  return(err);
		}
	    }
	  if (hasSSB)
	    {
	      err = M2TXRaw_LODCreate(&raw, ssbFilter, M2Channel_SSB, &rawLOD);
	      if (err != M2E_NoErr)
		{
		  fprintf(stderr,"ERROR:Resize failed!\n");
		  return(err);
		}
	    }
	}

      err = M2TXRaw_ToUncompr(&newTex, oldPIP, lod, &rawLOD);
      if (err != M2E_NoErr)
	{
	  fprintf(stderr,"ERROR:Error during conversion, must abort.\n");
	  return(err);
	}
      if ((raw.XSize != lodXSize) || (raw.YSize != lodYSize))
	M2TXRaw_Free(&rawLOD);
      
      M2TXRaw_Free(&raw);
      
      lodXSize = lodXSize >> 1;
      lodYSize = lodYSize >> 1;
    }
  
  M2TXHeader_FreeLODPtrs(header);
  M2TX_Copy(tex, &newTex);

  return(M2E_NoErr);
}

#define	ID_TXTR	MAKE_ID('T','X','T','R')

int main( int argc, char *argv[] )
{
  ByteStream*		stream = NULL;

  M2TX          tex, subTex, pipTex;
  int           catResult;
  bool          flag, result, gotTPA, xTile, yTile;
  bool          extPIP = FALSE;
  bool          floyd = FALSE;
  bool          hasPIP, subHasPIP, hasColor;
  bool          pageIndexMode = TRUE;
  M2TXHeader    *header, *subHeader;
  char          fileIn[256];
  char          fileOut[256];
  char          pipFile[256];
  char          scriptFile[256];
  char          texFile[256];
  int           i, j, k, argn, duplicateTex;
  M2Err         err;
  FILE          *fPtr, *texFPtr;
  FILE          *scriptFPtr;
  uint32        totalBytes, attr, val, tabVal, sdfVal;
  M2TXTex       pageTexel, pagePtr;
  int nTextures = TEX_BUF_SIZE;
  TxPage        txPage, *subTxPage, *pages;
  int           curPage = 0;
  int           curTex = 0;
  M2TXPgHeader *pgData;
  CltSnippet    pipLoadCLT, *pRefSnip;
  bool       first, found, doCopy, emptyTex;
  int16      numColors;
  SDFTex     *sdfTex, *mySDFTex;
  PageNames  *pageNames;
  int        numTex, numPages;
  long       newLOD=4;
  IFFParser  *iff;
  uint8      cDepth=8;

  scriptFPtr = stdout;
#ifdef M2STANDALONE
  printf("Enter: <SDF File In> <UTF Page File Out> <SDF TexPageArray File Out>\n");
  printf("Example: page.txt page.utf\n");
  fscanf(stdin,"%s %s %s",fileIn, fileOut, texFile);
#else
  /* Check for command line options. */
  if (argc < 4)
    {
      Usage;
      print_description();
      return(-1);
    }
  strcpy(fileIn, argv[1]);
  argn = 2;
  while ( (argn < argc) && (argv[argn][0] == '-') && (argv[argn][1] != '\0') )
    {
      if ( strcmp( argv[argn], "-pip")==0 )
        {
	  ++argn;
	  extPIP = TRUE;
	  strcpy(pipFile, argv[argn]);
	  ++argn;
        }
      else if ( strcmp( argv[argn], "-floyd")==0 )
        {
	  ++argn;
	  floyd = TRUE;
        }
      else if ( strcmp( argv[argn], "-old")==0 )
        {
	  ++argn;
	  pageIndexMode = FALSE;
        }
      else if ( strcmp( argv[argn], "-s")==0 )
        {
	  ++argn;
	  strcpy(scriptFile, argv[argn]);
	  ++argn;
	  scriptFPtr = fopen(scriptFile, "w");
	  if (scriptFPtr == NULL)
	    {
	      fprintf(stderr,"ERROR:Can't open file \"%s\" \n",scriptFile);
	      return(-1);
	    }
        }
      else if ( strcmp( argv[argn], "-lod")==0 )
        {
	  ++argn;
	  newLOD = strtol(argv[argn],NULL,10);
	  ++argn;
	  if((newLOD<1) || (newLOD>11))
	    {
	      fprintf(stderr,"ERROR:Bad # of LODs specified.  Allowed range is 1 to 11, not %d\n",
		      newLOD);
	      return(-1);
	    }
        }
      else if ( strcmp( argv[argn], "-depth")==0 )
        {
	  ++argn;
	  cDepth = (uint8)strtol(argv[argn],NULL,10);
	  ++argn;
	  if((cDepth>8))
	    {
	      fprintf(stderr,"ERROR:Bad Color Depth specified for quantization. Allowed color depth range is 0 to 8, not %d\n",
		      cDepth);
	      return(-1);
	    }
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
      ++argn;
      fPtr = fopen(fileOut, "w");
      if (fPtr == NULL)
	{
	  fprintf(stderr,"ERROR:Can't open file \"%s\" for writing of UTF\n",fileOut);
	  return(-1);
	}
      fclose(fPtr);
      if ( argn != argc )
	{
	  strcpy( texFile, argv[argn] );
	  texFPtr = fopen(texFile, "w");
	  if (texFPtr == NULL)
	    {
	      fprintf(stderr,"ERROR:Can't open file \"%s\" for writing of TexPageArray\n",texFile);
	      return(-1);
	    }
	}
      else
	{      /* No tex page array file specified. */
	  Usage;
	  return(-1);   
	}
    }
  else
    {      /* No UTF Texture Page file specified. */
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
 

  err = TexPageArray_ReadFile(fileIn, &sdfTex, &numTex, &curTex, &pageNames, 
			      &numPages, &curPage, &gotTPA);
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Error during TexPageArray read of \"%s\".  Aborting\n",
	      fileIn);
      return(-1);
    }
  

  if (pageIndexMode==FALSE)
    {
      curPage = 0;
      if (curPage <1)
	{
	  fprintf(texFPtr,"SDFVersion 1.0\n\n");
	  fprintf(texFPtr,"Define TexArray \"%s\"{\n", fileIn);
	  for (i=0; i<curTex; i++)
	    {
	      SDFTex_Print(&(sdfTex[i]), texFPtr, 1);
	    }
	  fprintf(texFPtr,"}\n");
	  first = TRUE;
	  for (i=0; i<curTex; i++)
	    {
	      if (sdfTex[i].FileName != NULL)
		{
		  if (strcasecmp("(none).utf", sdfTex[i].FileName))
		    {
		      if (first)
			{
			  fprintf(texFPtr,"Define TexPageArray {\n");
			  first = FALSE;
			}
		      fprintf(texFPtr,"\t{\n\t\tfileName \"%s\"\n\t}\n",sdfTex[i].FileName);
		      curPage++;
		    }
		}
	    }
	  if (!first)
	    fprintf(texFPtr,"}\n");
	}
      fprintf(texFPtr,"\n");
      if (texFPtr != stdout)
	fclose(texFPtr);
    }


  SDF_Arrange(sdfTex, curTex, &pageNames, &curPage, &numPages, pageIndexMode); 
  if (pageIndexMode==TRUE)
    {
      if (curTex>0)
	{
	  fprintf(texFPtr,"SDFVersion 1.0\n\n");
	  fprintf(texFPtr,"Define TexArray \"%s\"{\n", fileIn);
	  for (i=0; i<curTex; i++)
	    {
	      SDFTex_Print(&(sdfTex[i]), texFPtr, 1);
	    }
	  fprintf(texFPtr,"}\n");
	}
    }
  /* Allocate a page buffer */
  pageTexel = pagePtr = (M2TXTex)calloc(TRAM_SIZE,1);
  if (extPIP == TRUE)
    {
      err = M2TX_ReadFileNoLODs(pipFile,&pipTex);
      if (err != M2E_NoErr)
	{
	  fprintf(stderr,"ERROR:Error during read of PIP File \"%s\"  Aborting\n",
		  pipFile);
	  return(-1);
	}
    }

  for (i=0; i<curPage; i++)
    {
      M2TX_Init(&tex);  /* Initialize the Page texture */
      txPage.Tex = &tex; 
      TxPage_Init(&txPage);
      hasPIP = FALSE;
      M2TX_GetHeader(&tex,&header);
      M2TXHeader_SetMinXSize(header, 128);
      M2TXHeader_SetMinYSize(header, 128);
      M2TXHeader_SetFHasColor(header, TRUE);
      M2TXHeader_SetCDepth(header, cDepth);
      M2TXHeader_SetNumLOD(header, 1);
      header->LODDataPtr[0] = pageTexel; 
      
      if (extPIP == TRUE)
	{
	  M2TXHeader_GetFHasPIP(&(pipTex.Header), &flag);
	  if (flag)
	    M2TX_SetPIP(&tex, &(pipTex.PIP));
	  else
	    {	
	      fprintf(stderr,"ERROR:Your PIP File \"%s\" has no PIP! Aborting\n",
		      pipFile);
	      return(-1);
	    }
	  hasPIP= TRUE;
	  M2TXHeader_SetFHasPIP(header, TRUE);
	}

      nTextures = tex.Page.NumTex = pageNames[i].NumNames;
      pages = (TxPage *)qMemClearPtr(nTextures, sizeof(TxPage));
      pRefSnip = (CltSnippet *)qMemClearPtr(nTextures, sizeof(CltSnippet));
      pgData = (M2TXPgHeader *)qMemClearPtr(nTextures, sizeof(M2TXPgHeader));
      totalBytes = 0;

      for (j=0; j<nTextures; j++)
	{
	  M2TX_Init(&subTex);
	  M2TX_GetHeader(&subTex, &subHeader);
	  subTxPage = &(pages[0]);
	  TxPage_Init(subTxPage);
	  /* Find the right SDFTex */
	  
	  found = FALSE;
	  k=0;
	  while ((!found) && (k<curTex))
	    {
	      if (sdfTex[k].PageIndex[0] == i) 
		{
		  if (sdfTex[k].PageIndex[1] == j)
		    {
		      found=TRUE;
		      break;
		    }
		  else if ((sdfTex[k].PageIndex[1] == -1) && (j==0) && (nTextures==1))
		    {
		      found=TRUE;
		      break;
		    }
		}
	      k++;
	    }
	  
	  if (k>=curTex)
	    {
	      fprintf(stderr,"ERROR:can't find page %d, sub-texture %d\n",
		      i, j);
	      return(-1);
	    }
	  
	  mySDFTex = &(sdfTex[k]);
	  if ((strcasecmp("(none).utf", mySDFTex->FileName)) && (mySDFTex->FileName != NULL))
	    {
	      err = M2TX_ReadFile(mySDFTex->FileName, &subTex);
	      if (err != M2E_NoErr)
		{
		  fprintf(stderr,"ERROR:Error during read of \"%s\".  Aborting\n",
			  mySDFTex->FileName);
		  return(-1);
		}
	      M2TXHeader_GetFHasPIP(subHeader, &subHasPIP);
	      if (!hasPIP)
		{
		  if (!extPIP)
		    {
		      if (subHasPIP)
			{
			  M2TX_SetPIP(&tex, &(subTex.PIP));
			  hasPIP = TRUE;
			  M2TXHeader_SetFHasPIP(header, TRUE);
			}
		    }
		}
	    }
	  
	  subTxPage->Tex = &subTex;
	  
	  /* Resolve XTile, YTile Precedence */
	  if (subTex.LoadRects.LRData[0].XWrapMode == 1)
	    {
	      if ((mySDFTex->XTile) != FALSE)
		subTxPage->PgHeader.PgFlags |= M2PG_FLAGS_XWrapMode;
	    }
	  if (subTex.LoadRects.LRData[0].YWrapMode == 1)
	    {
	      if ((mySDFTex->YTile) != FALSE)
		subTxPage->PgHeader.PgFlags |= M2PG_FLAGS_YWrapMode;
	    }
	  /* For NULL textures, set up texture. */
	  /* For previously used textures, suppress copying */

	  doCopy = TRUE;
	  emptyTex = FALSE;
	  duplicateTex = -1;
	  if ((!strcasecmp("(none).utf", mySDFTex->FileName)) || (mySDFTex->FileName == NULL))
	    {
	      M2TXHeader_SetNumLOD(&(subTex.Header),0);
	      doCopy = FALSE;
	      emptyTex = TRUE;
	    }
	  else
	    {
	      for (k=0; k<j; j++)
		{
		  if (!strcasecmp(mySDFTex->FileName,
				  pageNames[i].SubNames[k]))
		    {
		      duplicateTex = k;
		      doCopy = FALSE;
		      break;
		    }
		}
	    }

	  if ((mySDFTex->XTile) == TRUE)
	    subTxPage->PgHeader.PgFlags |= M2PG_FLAGS_XWrapMode;
	  if ((mySDFTex->YTile) == TRUE)
	    subTxPage->PgHeader.PgFlags |= M2PG_FLAGS_YWrapMode;

	  err = Texture_Append(header, &totalBytes, subTxPage, doCopy);
	  if (err != M2E_NoErr)
	    {
	      if ((err == M2E_Range) && (doCopy))
		{
		  fprintf(scriptFPtr,"utffit %s 4 -depth %d ", mySDFTex->FileName, cDepth);
		  xTile = (subTxPage->PgHeader.PgFlags & M2PG_FLAGS_XWrapMode) ? TRUE : FALSE;
		  yTile = (subTxPage->PgHeader.PgFlags & M2PG_FLAGS_YWrapMode) ? TRUE : FALSE;
		  if (xTile)
		    fprintf(scriptFPtr,"-xpow ");
		  if (yTile)
		    fprintf(scriptFPtr,"-ypow ");
		  fprintf(scriptFPtr,"%s\n", mySDFTex->FileName, cDepth);
		  err = Fit(&subTex, cDepth, xTile, yTile, newLOD);
		  if (err != M2E_NoErr)
		    {
		      fprintf(stderr,"ERROR:Error Fitting \"%s\".  Aborting\n",
			      mySDFTex->FileName);
		      return(-1);
		    }

		  M2TX_GetHeader(&subTex, &subHeader);
		  M2TXHeader_GetFHasColor(subHeader, &hasColor);
		  if (hasColor)
		    {
		      if (extPIP)
			{
			  numColors = pipTex.PIP.NumColors;
			  fprintf(scriptFPtr,"quanttopip %s %s ",
				  mySDFTex->FileName, pipFile);
			  if (floyd)
			    fprintf(scriptFPtr,"-floyd ");
			  fprintf(scriptFPtr,"%s\n", mySDFTex->FileName);
			}
		      else 
			{
			  numColors = 1 << cDepth;
			  fprintf(scriptFPtr,"utfquantmany boguspipfile %d -pip ",
				  numColors);
			  if (floyd)
			    fprintf(scriptFPtr,"-floyd ");
			  fprintf(scriptFPtr,"%s\n", mySDFTex->FileName);
			}
		      err = Quantize(&subTex, numColors, extPIP, floyd, cDepth, &pipTex);
		      M2TX_SetPIP(&tex, &(subTex.PIP));
		      hasPIP = TRUE;
		      M2TXHeader_SetFHasPIP(header, TRUE);
	
		      if (err != M2E_NoErr)
			{
			  fprintf(stderr,"ERROR:Error Quantizing \"%s\".  Aborting\n",
				  mySDFTex->FileName);
			  return(-1);
			}
		    }
		  M2TX_CreateTAB(&subTex);
		  totalBytes = 0;
		  err = Texture_Append(header, &totalBytes, subTxPage, doCopy);
		  if (err != M2E_NoErr)
		    {
		      fprintf(stderr,"ERROR:Error appending texture \"%s\".  Aborting\n",
			      mySDFTex->FileName);
		      return(-1);
		    }
		  subTxPage->Tex = &subTex;
		}
	    }
      

	  /* Resolve TAB and DAB Precedence */
	  
	  attr=0;
	  while(attr<TXA_NoMore)
	    {
	      err = M2TXTA_GetAttr(&(subTex.TexAttr), attr, &tabVal);
	      if (err == M2E_NoErr)
		{
		  result = SDFTex_GetTAB(mySDFTex, attr, &sdfVal);
		  if (!result)  /* If not set by TexArray, okay to set */
		    SDFTex_SetTAB(mySDFTex, attr, tabVal);
		}
	      attr++;
	    }
	  
	  if (duplicateTex >= 0)
	    {  /* We probably resized this guy so, don't trust what's in the FILE TAB */
	      mySDFTex->TAB[TXA_PipAlphaSelect] = sdfTex[duplicateTex].TAB[TXA_PipAlphaSelect];
	      mySDFTex->TAB[TXA_PipSSBSelect] = sdfTex[duplicateTex].TAB[TXA_PipSSBSelect];
	      mySDFTex->TAB[TXA_PipColorSelect] = sdfTex[duplicateTex].TAB[TXA_PipColorSelect];
	    }
	  /* Convert/Combine TAB and DAB settings in SDFTex to CLT addresses */
	  attr=0;
	  
	  /*  Quick fix */
	  subTxPage->PgHeader.PgFlags |= M2PG_FLAGS_HasTexFormat;
	  subTxPage->PgHeader.TexFormat = subTex.Header.TexFormat;
	  /*  */
	  if ((mySDFTex->XTile) == TRUE)
	    subTxPage->PgHeader.PgFlags |= M2PG_FLAGS_XWrapMode;
	  if ((mySDFTex->YTile) == TRUE)
	    subTxPage->PgHeader.PgFlags |= M2PG_FLAGS_YWrapMode;
	  while(attr<TXA_NoMore)
	    {
	      result = SDFTex_GetTAB(mySDFTex, attr, &val);
	      if (result)  /* If not set by TexArray, okay to set */
		TxPage_TABInterpret(subTxPage, attr, val);
	      attr++;
	    }
	  
	  attr=0;
	  while(attr<DBLA_NoMore)
	    {
	      result = SDFTex_GetDAB(mySDFTex, attr, &val);
	      if (result)  /* If not set by TexArray, okay to set */
		TxPage_DABInterpret(subTxPage, attr, val);
	      attr++;
	    }
      
	  if (duplicateTex >= 0)
	    {  /* Warning: Absolute location in the Snippet */
	      subTxPage->PgHeader.Offset = pgData[duplicateTex].Offset;
	      subTxPage->PgHeader.NumLOD = pgData[duplicateTex].NumLOD;
	      subTxPage->PgHeader.TexFormat = pgData[duplicateTex].TexFormat;
	      subTxPage->PgHeader.MinXSize = pgData[duplicateTex].MinXSize;
	      subTxPage->PgHeader.MinYSize = pgData[duplicateTex].MinYSize;
	    }
	  if (emptyTex)
	    {
	      subTxPage->PgHeader.NumLOD = 1;
	      subTxPage->PgHeader.Offset = 0;
	    }

	  /* Create the sub-texture reference CLT */
	  PageCLT_CreateRef(subTxPage, &txPage, j);
	  memcpy(&(pgData[j]), &(subTxPage->PgHeader), sizeof(M2TXPgHeader));
	  /* Copy Texture Reference Snippet to the list for the whole page */
	  pRefSnip[j].size = subTxPage->CLT.size;
	  pRefSnip[j].allocated = subTxPage->CLT.allocated;
	  pRefSnip[j].data = subTxPage->CLT.data;
	}
      header->LODDataLength[0] = totalBytes;
      tex.Page.PgData = pgData;
      
      PageCLT_CreatePageLoad(&txPage, totalBytes);
      PageCLT_Init(&pipLoadCLT);
      PageCLT_CreatePIPLoad(&pipLoadCLT, &txPage, hasPIP);
      
      tex.PCLT.NumTex = nTextures;
      tex.PCLT.PageLoadCLT.size = txPage.CLT.size;
      tex.PCLT.PageLoadCLT.data = txPage.CLT.data;
      tex.PCLT.PageLoadCLT.allocated = txPage.CLT.allocated;
      tex.PCLT.PIPLoadCLT.size = pipLoadCLT.size;
      tex.PCLT.PIPLoadCLT.data = pipLoadCLT.data;
      tex.PCLT.PIPLoadCLT.allocated = pipLoadCLT.allocated;
      tex.PCLT.TexRefCLT = pRefSnip;
      tex.PCLT.UVScale = txPage.UVScale;
      tex.PCLT.NumCompressed = 0;
      tex.PCLT.PatchOffset = txPage.PatchOffset;

      /* IFF Cat */
      if (curPage <2)
	M2TX_WriteFile(fileOut, &tex); 
      else
	{
	  if (i==0)
	    {
	      err = M2TX_OpenFile(fileOut, &iff, TRUE, FALSE, NULL); 
	      catResult = PushChunk (iff, ID_TXTR, ID_CAT, IFF_SIZE_UNKNOWN_32);
	      if (catResult < 0)
		{
		  fprintf(stderr,"ERROR: in PushChunk. Abortting\n");
		  return(-1);
		}
	    }
	  err = M2TX_WriteChunkData(iff, &tex, M2TX_WRITE_ALL );
	  if (err != M2E_NoErr)
	    fprintf(stderr,"ERROR:%d error while concatenating \"%d\"\n",
		    i);
	}
      free(pgData);
    }
  fprintf(scriptFPtr,"utfpage %s ", texFile);
  if (extPIP)
    fprintf(scriptFPtr,"-pip %s ", pipFile);
  fprintf(scriptFPtr,"%s\n\n", fileOut);
  if (scriptFPtr != stdout)
    fclose(scriptFPtr);
  if (curPage>1)
    {
      catResult = PopChunk(iff);
      if (catResult < 0)
	{
	  fprintf(stderr,"ERROR: in PopChunk. Abortting\n");
	  return(-1);
	}
      catResult = DeleteIFFParser(iff);
      if (catResult<0)
	fprintf(stderr,"Error during Parser deallocation.  Who cares?\n");
      return(0);
    }
}
