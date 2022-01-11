/*
	File:		utfpage.c

	Contains:	Build pages from individual textures

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
|||	AUTODOC -public -class tools -group m2tx -name utfpage
|||	Create a mercury texture page using a TexArray and TexPageArray
|||	
|||	  Synopsis
|||	
|||	    utfpage <input file> [options] <output file>
|||	
|||	  Description
|||	
|||	    This tool takes a text file with an SDF TexArray and a TexPageArray,
|||	    and outputs a UTF CAT file that has the all the textures specified 
|||	    in the input file packed into Mercury pages.  Each page will have a 
|||	    PCLT chunk that contains data for CltSnippets to 1) Load the texture
|||	    page 2) Load the PIP, if any  3) Select individual sub-textures within
|||	    the page.
|||	    If only one texture page is specified in the input file, the output 
|||	    will be a standard UTF IFF FORM file.  If more than one texture page 
|||	    is specified, the output will be an UTF IFF CAT file.
|||	
|||	  Arguments
|||	
|||	    <input file>
|||	        The input text file with an SDF TexArray and TexPageArray.
|||	    <output file>
|||	        The resulting UTF texture (which is a Mercury Texture Page).
|||	
|||	  Options
|||	
|||	    -pip pipfile
|||	        Use the PIP within the pipfile rather than the first textures PIP
|||	    -superpip
|||	        For each page, concatenate each images PIP into a "Super PIP" for each page.
|||	    -t TexArrayFile
|||	        Output the TexArray in PageIndex notation to the specified file.
|||	
|||	  Caveats
|||	
|||	    The pip of the first texture of each page is used as the PIP for the
|||	    whole page unless a pipfile is specified.  All the images are assumed
|||	    to share the same PIP (with the same ordering) and that the total 
|||	    uncompressed size of any texture page does not exceed 16K.
|||	    The -superpip option assumes that the total number of colors
|||	    in all PIPs of the sub-textures of a page do not exceed 256.
|||	    -superpip merely concatenates the individual texture's PIPs,
|||	    there is no sharing of colors between textures.
|||	
|||	  See Also
|||	
|||	    utfquantmany, utfpipcat, utfunpage, utfsplit
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
#include <math.h>
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
#include "page.h"

extern LWS *cur_lws;

#ifdef applec 
 int strcasecmp(char *s, char *t);
#endif

M2Err M2TX_OpenFile(char *fileName, IFFParser **iff, bool writeMode, bool isMac, void *spec);
Err M2TX_WriteChunkData(IFFParser *iff, M2TX *tex, uint32 write_flags );

#define EXPANSIONFORMATMASK	0x00001FFF




PageCLT_CreatePageLoadAppend(M2TXHeader *header, TxPage *txPage, TxPage *subTxPage, uint32 *totalSBytes, 
			     uint32 *totalDBytes, int *curComprTex, bool doCopy)
{
  uint32     size, regVal, texels, i, lod, lodSize;
  M2TXHeader *subHeader;
  CltSnippet *snip;
  bool       isCompressed;
  uint8      numLOD;
  uint16     minXSize, minYSize;
  M2TXDCI    *dci;
  M2TXPgHeader *subPgHeader;
  M2TXTex       texPtr;
  
  texPtr = (M2TXTex)(header->LODDataPtr[0] + *totalSBytes);

  subHeader = &(subTxPage->Tex->Header);
  subPgHeader = &(subTxPage->PgHeader);
  M2TX_GetDCI(subTxPage->Tex, &dci);
  M2TXHeader_GetFIsCompressed(subHeader, &isCompressed);
  M2TXHeader_GetNumLOD(subHeader, &numLOD);
  M2TXHeader_GetMinXSize(subHeader, &minXSize);
  M2TXHeader_GetMinYSize(subHeader, &minYSize);

  if (doCopy==TRUE)
    {
      if (*totalSBytes == 0)
	{
	  /* Create Page Load CLT */
	  PageCLT_Init(&(txPage->CLT));
	  snip = &(txPage->CLT);
	  
	  /* Header that does the SYNC */
	  snip->data[0] = CLT_WriteRegistersHeader(DCNTL, 1);
	  snip->data[1] = CLT_Bits(DCNTL, SYNC, 1);
	  txPage->CLT.size = 2;
	}
      else
	snip = &(txPage->CLT);

      size = snip->size;

      PageCLT_Extend(&(txPage->CLT), 2);
      snip->data[size] = CLT_WriteRegistersHeader(TXTLDSRCADDR,1); size++;
      snip->data[size] = *totalSBytes; 
      txPage->PatchOffset[*curComprTex] = size;
      *curComprTex = *curComprTex +1;
      size++;  /* Offset from start of memory block */
      snip->size = size;

      texels = (minXSize*minYSize<<(numLOD-1))<<(numLOD-1);
      PageCLT_Extend(&(txPage->CLT), 2);
      snip->data[size] = CLT_WriteRegistersHeader(TXTCOUNT,1); size++;  /* Texels to decompress */
      snip->data[size] = texels; size++;
      snip->size = size;

  
      if ( texels > 0)
	{
	  PageCLT_Extend(&(txPage->CLT), 2);
	  snip->data[size] = CLT_WriteRegistersHeader(TXTLODBASE0,1); size++;
	  snip->data[size] = *totalDBytes; size++;    /* Destination of the Load in TRAM */
	  snip->size = size;

	  PageCLT_Extend(&(txPage->CLT), 12);
	  snip->data[size] = CLT_WriteRegistersHeader(TXTLDCNTL,1); size++;
	  regVal = CLT_SetConst(TXTLDCNTL, LOADMODE, TEXTURE) |
	    CLT_Bits(TXTLDCNTL, COMPRESSED, 1);
	  snip->data[size] = regVal; size++;
    
	  snip->data[size] = CLT_WriteRegistersHeader(TXTSRCTYPE01, 7); size++;
	  regVal = (((uint32)(dci->TexelFormat[1]) & EXPANSIONFORMATMASK)<<16) +
	    (((uint32)(dci->TexelFormat[0])) & EXPANSIONFORMATMASK);
	  snip->data[size] = regVal; size++;
	  regVal = (((uint32)(dci->TexelFormat[3]) & EXPANSIONFORMATMASK)<<16) +
	    (((uint32)(dci->TexelFormat[2])) & EXPANSIONFORMATMASK);
	  snip->data[size] = regVal; size++;
	  snip->data[size] = ((uint32)subHeader->TexFormat) & EXPANSIONFORMATMASK; size++;
	  snip->data[size] = dci->TxExpColorConst[0]; size++;
	  snip->data[size] = dci->TxExpColorConst[1]; size++;
	  snip->data[size] = dci->TxExpColorConst[2]; size++;
	  snip->data[size] = dci->TxExpColorConst[3]; size++;

	  snip->data[size] = CLT_WriteRegistersHeader(DCNTL, 1); size++;
	  snip->data[size] = CLT_Bits(DCNTL, TLD, 1); size++;
	  snip->size = size;

	  lodSize =  subHeader->LODDataLength[0];
	  memcpy(texPtr, subHeader->LODDataPtr[0], lodSize);
	  texPtr += lodSize;
	}


      for (i=0; i<4; i++)
      {
	subPgHeader->TexelFormat[i] = dci->TexelFormat[i];
	subPgHeader->TxExpColorConst[i] = dci->TxExpColorConst[i];
      }
    subPgHeader->LODLength[0] = lodSize;

    if (doCopy == TRUE)
      {
	*totalSBytes += lodSize;
	*totalDBytes += M2TXHeader_ComputeLODSize(subHeader, 0);
	for (lod=1; (lod<numLOD); lod++)
	  {
	    /* Must SYNC between each load */
	    PageCLT_Extend(&(txPage->CLT), 2);
	    snip->data[size] = CLT_WriteRegistersHeader(DCNTL, 1); size++;
	    snip->data[size] = CLT_Bits(DCNTL, SYNC, 1); size++;
	    txPage->CLT.size = size;

	    PageCLT_Extend(&(txPage->CLT), 2);
	    snip->data[size] = CLT_WriteRegistersHeader(TXTLDSRCADDR,1); size++;
	    snip->data[size] = *totalSBytes;   
	    txPage->PatchOffset[*curComprTex] = size;
	    *curComprTex = *curComprTex +1;
	    size++;  /* Offset from start of memory block */
	    snip->size = size;
	    
	    texels = (minXSize*minYSize<<(numLOD-lod-1))<<(numLOD-lod-1);
	    PageCLT_Extend(&(txPage->CLT), 6);
	    snip->data[size] = CLT_WriteRegistersHeader(TXTCOUNT,1); size++;  /* Texels to decompress */
	    snip->data[size] = texels; size++;
	    snip->size = size;
	    snip->data[size] = CLT_WriteRegistersHeader(TXTLODBASE0,1); size++;
	    snip->data[size] = *totalDBytes; size++;
	    snip->size = size;
	    snip->data[size] = CLT_WriteRegistersHeader(DCNTL, 1); size++;
	    snip->data[size] = CLT_Bits(DCNTL, TLD, 1); size++;
	    snip->size = size;

	    lodSize = subHeader->LODDataLength[lod];
	    memcpy(texPtr, subHeader->LODDataPtr[lod], lodSize );
	    subPgHeader->LODLength[lod]=lodSize;
	    *totalSBytes = *totalSBytes + lodSize;
	    *totalDBytes = *totalDBytes + M2TXHeader_ComputeLODSize(subHeader, lod);
	    texPtr += lodSize;
	  }
      }    
    }
  else
    {
	for (lod=1; (lod<numLOD); lod++)
	  {
	    lodSize = subHeader->LODDataLength[lod];
	    subPgHeader->LODLength[lod]=lodSize;
	  }

    }

}

M2Err Texture_Append(M2TXHeader *header, TxPage *sTP, TxPage *texPage, 
		     uint32 *totalSBytes, uint32 *totalDBytes, int *curComprTex, bool doCopy)
{
  M2TXPgHeader *subPgHeader;
  bool         isCompressed;
  M2TX         *subTex;
  uint16        xSize, ySize;
  uint8         numLOD;
  uint32        lodSize, i, totalBytes;
  M2TXHeader    *subHeader;
  M2TXTex       texPtr;
  
  texPtr = (M2TXTex)(header->LODDataPtr[0] + *totalSBytes);
  subPgHeader = &(sTP->PgHeader);
  subTex = sTP->Tex;
  M2TX_GetHeader(subTex, &subHeader);

  /* Could be done at load time */
  M2TXHeader_GetNumLOD(subHeader, &numLOD);
  M2TXHeader_GetMinXSize(subHeader, &xSize);
  M2TXHeader_GetMinYSize(subHeader, &ySize);
  M2TXHeader_GetFIsCompressed(subHeader, &isCompressed);

  subPgHeader->TexFormat = subHeader->TexFormat;
  if (subPgHeader->TexFormat != header->TexFormat)
    subPgHeader->PgFlags |= M2PG_FLAGS_HasTexFormat;
  subPgHeader->NumLOD = numLOD;
  subPgHeader->MinXSize = xSize;
  subPgHeader->MinYSize = ySize;
  subPgHeader->Offset = *totalSBytes;
  if (isCompressed)
    {
      subPgHeader->PgFlags |= M2PG_FLAGS_IsCompressed;
       PageCLT_CreatePageLoadAppend(header, texPage, sTP, totalSBytes, totalDBytes, 
				   curComprTex, doCopy);
    }
  else
    {
      if (doCopy==TRUE)
	for (i=0; i<numLOD; i++)
	  {
	    /*      lodSize = subHeader->LODDataLength[i]; */
	    lodSize = M2TXHeader_ComputeLODSize(subHeader, i);
	    totalBytes = lodSize + *totalDBytes;
	    if (totalBytes > TRAM_SIZE)
	      {
		fprintf(stderr,"ERROR:Exceeding %d bytes available in TRAM on LOD %d, %d bytes\n",
			TRAM_SIZE, i,totalBytes);
		return(M2E_Range);
	      }
	    memcpy(texPtr, subHeader->LODDataPtr[i], lodSize);
	    *totalDBytes = *totalDBytes + lodSize;
	    *totalSBytes = *totalSBytes + lodSize;
	    
	    texPtr += lodSize;
	  }
    }
  return(M2E_NoErr);
}
 
void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   Takes in page format file and creates a UTF Page.\n");
  printf("   -pip pipfile  \tUse this pip as the page's pip.\n");
  printf("   -superpip  \tConcatenate individual textures into a single page palette.\n");
  printf("   -t TexArrayFile  \tWrite out the TexArray in PageIndex notation.\n");
}

#define Usage  printf("Usage: %s <Input File> [options] <Output File>\n",argv[0])



#define	ID_TXTR	MAKE_ID('T','X','T','R')

M2Err SDF_Assign(SDFTex *sdfTex, int curTex, PageNames **myPages, int *cP, int *nP, bool gotTPA)
{
  PageNames *pageNames, *pn;
  int i, j, n;
  M2Err err;
  int curPage, numNames;
 
  if (gotTPA)
    {
      pageNames = *myPages;
      curPage = *cP;
      for (i=0; i<curPage; i++)
	{
	  pn = &(pageNames[i]);
	  numNames = pn->NumNames;
	  for (j=0; j<numNames; j++)
	    {
	      n = 0;
	      while ( n<curTex)
		{
		  if (sdfTex[n].FileName != NULL)
		    if (strcasecmp(sdfTex[n].FileName, pageNames[i].SubNames[j])==0)
		      break;
		  n++;
		}
	      if (n>=curTex)
		{
		  fprintf(stderr,"ERROR:Can't find TexArray entry for \"%s\"\n",
			  pageNames[i].SubNames[j]);
		  return(M2E_Range);
		}
	      sdfTex[n].PageIndex[0] = i;
	      sdfTex[n].PageIndex[1] = j;
	    }
	}
    }
  else
    {
      curPage = -1;
      for (i=0; i<curTex; i++)
	{
	  if (sdfTex[i].PageIndex[0] == -1)
	    fprintf(stderr,"WARNING: TexArray entry %d has no PageIndex\n",i);
	  else
	    if (sdfTex[i].PageIndex[0] > curPage)
	      curPage = sdfTex[i].PageIndex[0];
	}
      if (curPage==-1)
	{
	  fprintf(stderr,"ERROR: You must either supply a PageIndex for each TexArray entry\n");
	  fprintf(stderr,"or a TexPageArray.  This file has neither.  No page can be constructed\n");
	}
      else
	{
	  curPage++;
	  if (((*nP)>0) && (*myPages!=NULL))
	    qMemReleasePtr(*myPages);
	  pageNames = (PageNames *)qMemClearPtr(curPage, sizeof(PageNames));
	  if (pageNames == NULL)
	    {
	      fprintf(stderr,"ERROR:Out of memory in TexPageArray_ReadFile!\n");
	      return(M2E_NoMem);
	    }
	  
	  for (i=0; i<curPage; i++)
	    {
	      numNames = 0;
	      pn = &(pageNames[i]);
	      PageNames_Init(pn);
	      for (j=0; j<curTex; j++)
		{
		  if (sdfTex[j].PageIndex[0] == i)
		    {
		      if (sdfTex[j].PageIndex[1]>numNames)
			numNames = sdfTex[j].PageIndex[1];
		      if (sdfTex[j].PageIndex[1]==-1)
			sdfTex[j].PageIndex[1]=0;
		    }
		}
	      err = PageNames_Extend(pn, numNames+1);
	      if (err != M2E_NoErr)
		{
		  fprintf(stderr,"ERROR: Errors occured in SDF_Assign\n");
		  return(err);
		}
	      pn->NumNames = numNames+1;
	      for (j=0; j<curTex; j++)
		{
		  if (sdfTex[j].PageIndex[0] == i)
		    {
		      if (sdfTex[j].FileName == NULL)
			pn->SubNames[sdfTex[j].PageIndex[1]] = NULL;
		      else
			{
			  pn->SubNames[sdfTex[j].PageIndex[1]] = (char *)calloc(strlen(sdfTex[j].FileName)+4,1);
			  if (pn->SubNames[sdfTex[j].PageIndex[1]] == NULL)
			    {
			      fprintf(stderr,"Out of Memory in SDF_Assign\n");
			      return(M2E_NoMem);
			    }
			  strcpy(pn->SubNames[sdfTex[j].PageIndex[1]], sdfTex[j].FileName);
			}
		    }
		}
	    }
	}
      *myPages = pageNames;
      *cP = curPage;
      *nP = curPage;
    }
  
  return(M2E_NoErr);

}

int main( int argc, char *argv[] )
{
  ByteStream*		stream = NULL;

  M2TX          tex, subTex, pipTex;
  M2TXPIP       *pagePIP, *subPIP;
  int           catResult, processedTex, duplicateTex;
  bool          flag, result, gotTPA, outputMode, found, emptyTex, doCopy;
  bool          extPIP = FALSE;
  bool          superPIP = FALSE;
  bool          floyd = FALSE;
  bool          hasPIP, subHasPIP, compressedPass, uncompressedPass, isCompressed;
  M2TXHeader    *header, *subHeader;
  char          fileIn[256];
  char          fileOut[256];
  char          pipFile[256];
  char          texFile[256];
  int           i, j, k, argn;
  M2Err         err;
  FILE          *fPtr, *texFPtr;
  uint32        totalSBytes, totalDBytes, attr, val, tabVal, sdfVal;
  M2TXTex       pageTexel, pagePtr;
  int nTextures = 0;
  TxPage        txPage, *subTxPage, *pages;
  int           curPage = 0;
  int           curTex = 0;
  M2TXPgHeader *pgData;
  CltSnippet    pipLoadCLT, *pRefSnip;
  
  int16      numColors;
  int        newColors, indexOffset;
  SDFTex     *sdfTex, *mySDFTex;
  PageNames  *pageNames;
  int        numTex, numPages;
  int        curComprTex;
  IFFParser     *iff;
  uint8      cDepth;

  outputMode = FALSE;
#ifdef M2STANDALONE
  printf("Enter: <FileIn> <FileOut>\n");
  printf("Example: page.txt page.utf\n");
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
      if ( strcmp( argv[argn], "-pip")==0 )
        {
	  ++argn;
	  extPIP = TRUE;
	  if (superPIP == TRUE)
	    {
	      fprintf(stderr,"ERROR:-pip and -superpip are mutually exclusive options.\n");
	      return(-1);
	    }
	  strcpy(pipFile, argv[argn]);
	  ++argn;
        }
      else if ( strcmp( argv[argn], "-t")==0 )
        {
	  ++argn;
	  strcpy(texFile, argv[argn]);
	  ++argn;
	  outputMode = TRUE;
	  texFPtr = fopen(texFile, "w");
	  if (texFPtr == NULL)
	    {
	      fprintf(stderr,"ERROR:Can't open file \"%s\" \n",texFile);
	      return(-1);
	    }
        }
      else if ( strcmp( argv[argn], "-superpip")==0 )
        {
	  ++argn;
	  superPIP = TRUE;
	  if (extPIP == TRUE)
	    {
	      fprintf(stderr,"ERROR:-pip and -superpip are mutually exclusive options.\n");
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
 
  
  err = TexPageArray_ReadFile(fileIn, &sdfTex, &numTex, &curTex, &pageNames, 
			      &numPages, &curPage, &gotTPA);
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Error during TexPageArray read of \"%s\".  Aborting\n",
	      fileIn);
      return(-1);
    }

  SDF_Assign(sdfTex, curTex, &pageNames, &curPage, &numPages, gotTPA); 
  if (outputMode==TRUE)
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
      M2TXHeader_SetNumLOD(header, 1);
      header->LODDataPtr[0] = pageTexel; 
      if (superPIP == TRUE)
	{
	  hasPIP = TRUE;
	  M2TXHeader_SetFHasPIP(header, TRUE);
	  M2TX_GetPIP(&tex, &pagePIP);
	  newColors=0;
	}
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
      totalDBytes = totalSBytes = 0;

      /* Two passes are made through the list, one for uncompressed textures
	 and one for compressed textures.
	 Uncompressed textures are put at the start of the Texture page and loaded
	 with a single MMDMA load.
	 Compressed textures must be loaded one LOD at a time by the texture loader
	 (and consequently has a HUGE performance hit) and are done after loading all
	 the uncompressed textures in the page
	 */
      compressedPass = FALSE;
      uncompressedPass = FALSE;
      processedTex = 0;
      curComprTex = 0;
      do 
	{
	  if (uncompressedPass)
	    compressedPass = TRUE;
	  for (j=0; j<nTextures; j++)
	    {
	      M2TX_Init(&subTex);
	      M2TX_GetHeader(&subTex, &subHeader);
	      subTxPage = &(pages[j]);
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
	      
	      
	      doCopy = TRUE;
	      emptyTex = FALSE;
	      duplicateTex = -1;
	      if (mySDFTex->FileName == NULL)
		{
		  M2TXHeader_SetNumLOD(&(subTex.Header),0);
		  emptyTex = TRUE;
		  isCompressed = FALSE;
		  doCopy = FALSE;
		}
	      else if (strcasecmp("(none).utf", mySDFTex->FileName))
		{
		  err = M2TX_ReadFile(mySDFTex->FileName, &subTex);
		  if (err != M2E_NoErr)
		    {
		      fprintf(stderr,"ERROR:Error during read of \"%s\".  Aborting\n",
			      mySDFTex->FileName);
		      return(-1);
		    }
		  else if (j==0)
		    {
		      M2TXHeader_GetCDepth(subHeader, &cDepth);
		      M2TXHeader_SetCDepth(header, cDepth);
		    }
		  for (k=0; k<j; k++)
		    {
		      if (!strcasecmp(mySDFTex->FileName,
				      pageNames[i].SubNames[k]))
			{
			  duplicateTex = k;
			  doCopy = FALSE;
			  break;
			}
		    }
		  M2TXHeader_GetFIsCompressed(subHeader, &isCompressed);
		}
	      else 
		{
		  M2TXHeader_SetNumLOD(&(subTex.Header),0);
		  doCopy = FALSE;
		  emptyTex = TRUE;
		}

	      if (isCompressed == compressedPass)
		{
		  if (!emptyTex)
		    {
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
		      else if ((superPIP && subHasPIP) && doCopy)
			{
			  indexOffset = newColors;
			  M2TX_GetPIP(&subTex, &subPIP);
			  M2TXPIP_GetNumColors(subPIP, &numColors);
			  if (numColors+newColors > 256)
			    {
			      fprintf(stderr,"ERROR:Number of accumulated PIP colors exceeds 256:%d as of sub-texture %d in page %d.\n",
				      numColors+newColors, j, i);
			      return(-1);
			    }
			  
			  for (k=0; k<numColors; k++)
			    {
			      pagePIP->PIPData[newColors] = subPIP->PIPData[k];
			      /*  fprintf(stderr,"newPIP %d:%x\n",newColors, 
				  newPIP.PIPData[newColors]); */
			      newColors++;
			    }
			  M2TXPIP_SetNumColors(pagePIP, newColors);
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
		  if (superPIP == TRUE)
		    {
		      fprintf(stderr,"Page:%d Texture:%d PIPOffset=%d\n",
			      i,j, indexOffset);
		      SDFTex_SetTAB(mySDFTex, TXA_PipIndexOffset, indexOffset);
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
		  
		  /* Convert/Combine TAB and DAB settings in SDFTex to CLT addresses */
		  attr=0;
		  
		  SDFTex_GetTAB(mySDFTex, TXA_PipIndexOffset, &val);
		  /*  Quick fix */
		  subTxPage->PgHeader.PgFlags |= M2PG_FLAGS_HasTexFormat;
		  subTxPage->PgHeader.TexFormat = subTex.Header.TexFormat;
		  /*  */
		  subTxPage->PgHeader.PIPIndexOffset = val;
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


		  /* For NULL textures, set up texture. */
		  /* For previously used textures, suppress copying */
		  

		      err = Texture_Append(header, subTxPage, &txPage, &totalSBytes, &totalDBytes, 
					   &curComprTex, doCopy);
		      if (err != M2E_NoErr)
			{
			  fprintf(stderr,"ERROR:Error appending texture \"%s\".  Aborting\n",
			      mySDFTex->FileName);
			  return(-1);
			}

		  /* Create the sub-texture reference CLT */

		  if (duplicateTex >= 0)
		    {  /* Warning: Absolute location in the Snippet */
		      subTxPage->PgHeader.Offset = pgData[duplicateTex].Offset;
		      if (superPIP == TRUE)
			subTxPage->PgHeader.PIPIndexOffset = pgData[duplicateTex].PIPIndexOffset;
		    }
		  if (emptyTex)
		    {
		      subTxPage->PgHeader.NumLOD = 0;
		      subTxPage->PgHeader.Offset = 0;
		    }

		  PageCLT_CreateRef(subTxPage, &txPage, j);
		  memcpy(&(pgData[j]), &(subTxPage->PgHeader), sizeof(M2TXPgHeader));
		  processedTex++;
		  /* Copy Texture Reference Snippet to the list for the whole page */
		  pRefSnip[j].size = subTxPage->CLT.size;
		  pRefSnip[j].allocated = subTxPage->CLT.allocated;
		  pRefSnip[j].data = subTxPage->CLT.data;
		}
	    }
	  if (!uncompressedPass)
	    {
	      uncompressedPass = TRUE;
	      PageCLT_CreatePageLoad(&txPage, totalSBytes);
	    }
	  /* Check to see if compressed pass is necessary */
	  if (processedTex == nTextures)
	    compressedPass = TRUE;
	  
	} while (!compressedPass);

      header->LODDataLength[0] = totalSBytes;
      tex.Page.PgData = pgData;

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
      tex.PCLT.NumCompressed = curComprTex;
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
		  fprintf(stderr,"ERROR: in PushChunk. Abortting.\n");
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
  if (curPage>1)
    {
      catResult = PopChunk (iff);
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
    return(0);
}
