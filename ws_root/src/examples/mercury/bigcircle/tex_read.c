/*	
 *	
 *	Read a texture file into a texture structure
 *		
 */

#ifdef MACINTOSH
#include <stdio.h>
#include <stdlib.h>
#include <:kernel:mem.h>
#include <:kernel:types.h>
#include <:graphics:clt:clt.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <kernel/mem.h>
#include <kernel/types.h>
#include <graphics/clt/clt.h>
#endif

#include "bsdf_read.h"

#define TEX_PAGE_BUF_SIZE 32

#if 0
/* These are the formats of the CLT Snippets stored in PCLT */
/* If you can't understand the format, refer to the CLT documentation */

/* NOTE: The format of these can be changed, but the order of the first five entries,
   should not be changed because some versions of Mercury patch these locations at run-time
   (so that the user doesn't have to patch the Snippets if only the texture address is 
   changing in something like a texture animation) and will definately crash if the order
   is changed.
   */
/* Texture Address goes in to TextureLoadList[3]  */
/* Texture Bytes goes in to TextureLoadList[5]  */
/* For an uncompressed texture load */
uint32 TextureLoadList[] = 
{
  CLT_WriteRegistersHeader(DCNTL, 1),
  CLT_Bits(DCNTL, SYNC, 1),
  CLT_WriteRegistersHeader(TXTLDSRCADDR, 1),
  0,			/* stick texture address here, 3 */
  CLT_WriteRegistersHeader(TXTCOUNT, 1),
  0,			/* stick true count of texture bytes here, 5 */
  CLT_WriteRegistersHeader(TXTLODBASE0, 1),
  0,
  CLT_WriteRegistersHeader(TXTLDCNTL, 1),	
  CLT_SetConst(TXTLDCNTL, LOADMODE, MMDMA),  /* Uncompressed textures are DMA loaded */
  CLT_WriteRegistersHeader(DCNTL, 1),
  CLT_Bits(DCNTL, TLD, 1)
  
/* Compressed Texture Loads and/or Optional Page Global attributes follow */
}

/* For a compressed Texture Load */
/* 
   NOTE: If a texture contains both uncompressed and compressed textures, a single
   texture load for the uncompressed textures will be performed first followed by all the 
   compressed texture loads (one LOD at a time).
   In such a case, the first two entries (that perform the SYNC) are unnecessary
   */
uint32 TextureLoadList[] = 
{
  CLT_WriteRegistersHeader(DCNTL, 1), 
  CLT_Bits(DCNTL, SYNC, 1),
  CLT_WriteRegistersHeader(TXTLDSRCADDR, 1),
  0,			/* stick texture address (of the first LOD) here, 3 */
  CLT_WriteRegistersHeader(TXTCOUNT, 1),
  0,			/* stick TEXEL count  here, 5 */
  CLT_WriteRegistersHeader(TXTLODBASE0, 1),
  0,                    /* stick TRAM destination here */
  CLT_WriteRegistersHeader(TXTLDCNTL, 1),
  /* Compressed = Texture Loader = slower */
  CLT_SetConst(TXTLDCNTL, LOADMODE, TEXTURE) | CLT_Bits(TXTLDCNTL,COMPRESSED,1),
  CLT_WriteRegistersHeader(TXTSRCTYPE01, 7),
  0,                   /* DCI Texel Formats 0 and 1 go here */
  0,                   /* DCI Texel Formats 2 and 3 go here */
  0,                   /* Texture's Expansion Texel Format goes here */
  0,                   /* DCI Color Constant 0 */
  0,                   /* DCI Color Constant 1 */
  0,                   /* DCI Color Constant 2 */
  0,                   /* DCI Color Constant 3 */
  CLT_WriteRegistersHeader(DCNTL, 1),
  CLT_Bits(DCNTL, TLD, 1)

  /* Each additional LOD for the sub-texture will have the following CLT commands */

  CLT_WriteRegistersHeader(TXTLDSRCADDR, 1),
  0,			/* stick texture address (of the next LOD) here */
  CLT_WriteRegistersHeader(TXTCOUNT, 1),
  0,			/* stick TEXEL count  here */
  CLT_WriteRegistersHeader(TXTLODBASE0, 1),
  0,                    /* stick TRAM destination here */
  CLT_WriteRegistersHeader(DCNTL, 1),
  CLT_Bits(DCNTL, TLD, 1)
  
}

/* To select a texture */
uint32 TextureSelectHeader[3] = 
{
  CLT_WriteRegistersHeader(DCNTL, 1),
  CLT_Bits(DCNTL, SYNC, 1),
  CLT_WriteRegistersHeader(TXTUVMAX, 2),
  UVMAX values,                                        /* 3 */
  UVMASK values,                                       /* 4 */
  CLT_WriteRegistersHeader(TXTADDRCNTL,1),
  CLT_Bits(TXTADDRCNTL, TEXTUREENABLE, 1) |
  CLT_SetConst(TXTADDRCNTL, MINFILTER, BILINEAR) |
  CLT_SetConst(TXTADDRCNTL, INTERFILTER, BILINEAR) |
  CLT_SetConst(TXTADDRCNTL, MAGFILTER, BILINEAR) |
  CLT_Bits(TXTADDRCNTL, LODMAX, 3),                    /* 6,  LODs are numbered 0-3  */
  CLT_WriteRegistersHeader(TXTLODBASE0,4),
  /* Offsets into TRAM for each LOD.  Maximum of four LODs selectable at a time */
  0,                                                   /* 8-11 */ 
  0,
  0,
  0
  /* Optional Texture Blend and Destination Blend settings are appended here */

}

/* Need to be either select or in the load snippet  */

uint32 BogusCLT[] =
{
  CLT_WriteRegistersHeader(TXTPIPCNTL,1),
  CLA_TXTPIPCNTL(CLT_Const(TXTPIPCNTL,PIPSSBSELECT,PIP),
		 CLT_Const(TXTPIPCNTL,PIPALPHASELECT,PIP),
		 CLT_Const(TXTPIPCNTL,PIPCOLORSELECT,PIP),
		 0),
  
  CLT_WriteRegistersHeader(TXTEXPTYPE, 1),
  CLT_Bits(TXTEXPTYPE, HASCOLOR, hasColor) |
  CLT_Bits(TXTEXPTYPE, CDEPTH, cDepth) |
  CLT_Bits(TXTEXPTYPE, ADEPTH, aDepth) |
  CLT_Bits(TXTEXPTYPE, HASALPHA, hasAlpha) |
  CLT_Bits(TXTEXPTYPE, HASSSB, hasSSB) |
  CLT_Bits(TXTEXPTYPE, ISLITERAL, isLiteral)
}

/* To load a PIP */
/* PIP Address goes in to PIPLoadList[1]  */
/* PIP Bytes goes in to PIPLoadList[3]  */
uint32 PIPLoadList[] = 
{
  CLT_WriteRegistersHeader(TXTLDSRCADDR, 1),
	0,				/* stick PIP address here: offset 1 */
  CLT_WriteRegistersHeader(TXTCOUNT, 1),
  0,				/* stick true count of PIP bytes here: offset 3*/
  CLT_WriteRegistersHeader(TXTLDDSTBASE, 1),
  0x46000,		         /* duplicated magic constant */
  CLT_WriteRegistersHeader(TXTLDCNTL, 1),
  CLT_SetConst(TXTLDCNTL, LOADMODE, PIP),
  CLT_WriteRegistersHeader(DCNTL, 1),
  CLT_Bits(DCNTL, TLD, 1)
}

#endif

#define align(x) ((((uint32)x)+0x03UL)&(~0x03UL))		  
#define bittobytes(x) (((((uint32)x)+0x07UL)&(~0x07UL))>>3)

/*
** Read a texture file containing muliple "load rect" chunks
** in it. A texture page contains ( this is not the final spec for
** texture page !! ) multiple load rects one for each sub-texture
** in the texture page. The whole texture will fit into a 26K TRAM.
** This routine will do the right thing if the texture is single
** texture page.
**
** This routine reads CLT snippets stored in chunks in the the new UTF
** textures.
*/
void 
ReadInTextureFile(
		  PodTexture **tex_pages,
		  uint32     *num_pages,
		  char *filename,
		  AllocProcPtr memcall
		  )
{
  RawFile*	   rawfile;
  FileInfo	   info;
  int              curPage=0;
  uint32	   formSize, filesize, bytes_read;
  uint32           *ploadbuf,*pread, totalBytes, bytesparsed, *p32;
  uint32           minXSize, minYSize;
  uint32	   nTex, i;
  uint32	   nLOD;
  uint32           chunkSize, pageCLTOff, pipCLTOff, lastOff;
  uint16           *p16;
  uint8		   *p8;		
  TpageSnippets    *ptemplateN;

  PodTexture       *page, *temp;
  MSnippet         *texRefCLT = NULL;
  CltSnippet       pageLoadCLT;
  CltSnippet       pipLoadCLT;
  CltSnippet       *pLoadSnippets;

  bool             foundPCLT, foundUV;
  float            *tempUV;
  uint32           numCompressed, dataSize, *patchOffset, version;


  if (filename == NULL)
    return;
  if (OpenRawFile(&rawfile,filename,FILEOPEN_READ) < 0)
    {
      printf("ERROR: Couldn't open texture file.\n");
      exit(1);
    }
  
  GetRawFileInfo( rawfile, &info, sizeof(info) );
  filesize = info.fi_ByteCount;

  ploadbuf = (uint32 *) (*memcall)(filesize,MEMTYPE_ANY);

  if (ploadbuf == NULL) {
    printf("WARNING: Can't allocate buffer for texture file: Out of memory\n");
    exit(1);
  }

  bytes_read = ReadRawFile(rawfile, ploadbuf, filesize);

  if (bytes_read < filesize) 
    {
      printf("WARNING: Unexpected end of file.\n");
      exit(1);
    }

  CloseRawFile(rawfile);

  *tex_pages = NULL;

  pread = (uint32 *)((long)ploadbuf);
  dataSize = *(pread+1);
  if (dataSize > filesize)
    {
      printf("WARNING:Data size of file exceeds actual file size. Possible corrupt texture.\n");
      exit(1);
    }
  else    /* Because of padding and other things, file may be longer than data */
    {
      filesize = dataSize + 8;  /*Account for IFF Form Type and data size */
    }
  
  if(*pread == 'CAT ')
    {
      pread += 4;
      totalBytes = 16;  /*Account for up to first FORM's first chunk */
      temp = *tex_pages = (PodTexture *)(*memcall)(sizeof(PodTexture)*TEX_PAGE_BUF_SIZE, MEMTYPE_ANY);
      if (temp == NULL)
	{
	  printf("WARNING: Can't allocate %d PodTexture: Out of memory\n", TEX_PAGE_BUF_SIZE);
	  exit(1);
	}
    }
  else if (*pread == 'FORM')
    {
      pread ++;
      totalBytes = 4;  /*Account for up to first FORM's first chunk */
      temp = *tex_pages = (PodTexture *)(*memcall)(sizeof(PodTexture),MEMTYPE_ANY);
      if (temp == NULL)
	{
	  printf("WARNING: Can't allocate %d PodTexture: Out of memory\n", 1);
	  exit(1);
	}
    }
  else
    {
      printf("WARNING:Not a supported IFF Type. Must be FORM or CAT.\n");
      exit(1);
    }
  
  do {
    if (curPage >= TEX_PAGE_BUF_SIZE)
      {
	printf("ERROR:Number of pages exceeds buffer size %d\n", 
	       TEX_PAGE_BUF_SIZE);
	exit(1);
      }
    formSize = *pread;
    totalBytes += 8;  /*Account for up to the FORM's first chunk */
    
    page = &(temp[curPage]);
    
    ptemplateN = (memcall)(sizeof(TpageSnippets),MEMTYPE_ANY);
    ptemplateN->loadcount = 2;	/* not actually used */
    pLoadSnippets = (memcall)(2*sizeof(CltSnippet),MEMTYPE_ANY);
    
    page->ptexture = NULL;
    page->ppip = NULL;
    page->texturebytes = 0;
    page->pipbytes = 0;
    foundPCLT = FALSE;
    foundUV = FALSE;
    numCompressed = 0;
    
    bytesparsed = 8;   /* Skip over FORM type, TXTR */
    do
      {
	pread = (uint32 *)((long)ploadbuf + totalBytes);
	
	if(*pread == 'M2TD')
	  {
	    page->ptexture = (uint32 *)(pread + 4);
	    page->texturebytes = *(pread + 1) - 8;
	    /* A page is treated as a single LOD with multiple sub-textures inside */
	  }
	else if(*pread == 'M2PI')
	  {
	    page->ppip = (uint32 *) (pread + 4);	/* skip over start field */
	    page->pipbytes = *(pread + 1) - 8;
	    /* Could put in support for offset loading */
	  }
	else if((*pread  == 'M2PG') && (!foundUV))
	  {
	    nTex = *(pread + 2);
	    version = *(pread +3);
	    /*   printf("M2PG version = %d\n", version); */
	    if (version<1)
	      {
		texRefCLT = (memcall)(sizeof(MSnippet)*nTex,MEMTYPE_ANY);
		p32 = pread;    /*Start of the Chunk Data */
		p32 += 6;
		for (i=0; i<nTex; i++)
		  {
		    p16 = (uint16*)p32;
		    minXSize = *p16;
		    p16++;
		    minYSize = *p16;
		    p16++; p16++;
		    p8 = (uint8*)p16;
		    nLOD = *p8;
		    texRefCLT[i].uscale = (minXSize<<(nLOD-1));
		    texRefCLT[i].vscale = (minYSize<<(nLOD-1));
		    p32 += 5;
		  }
		foundUV = TRUE;
	      }
	  }
	else if(*pread  == 'PCLT')
	  {
	    p32 = pread;
	    p32++;
	    chunkSize = *p32;
	    p32++;
	    nTex = *p32;
	    if (nTex > 0)
	      {
		p32++;	            /*Start of the Chunk Data */
		version = *p32;
		/* printf("PCLT version = %d\n",version); */
		p32++;
		if (version>0)      /* New for 3.0 */
		  {
		    numCompressed = *p32;
		    p32++;
		    texRefCLT = (memcall)(sizeof(MSnippet)*nTex,MEMTYPE_ANY);
		  }
		pageCLTOff = *p32;
		p32++;
		pipCLTOff = *p32;
		pLoadSnippets[0].size = pageLoadCLT.size = (pipCLTOff-pageCLTOff)/4;
		pLoadSnippets[0].data = pageLoadCLT.data = pread+2+(pageCLTOff/4);
		p32++;
		lastOff = *p32;
		p32++;
		pLoadSnippets[1].size = pipLoadCLT.size = (lastOff-pipCLTOff)/4;
		pLoadSnippets[1].data = pipLoadCLT.data = pread+2+(pipCLTOff/4);
		for (i=1; i<nTex; i++)
		  {
		    texRefCLT[i-1].snippet.size = ((*p32)-lastOff)/4;
		    texRefCLT[i-1].snippet.data = pread+2+lastOff/4;
		    lastOff = *p32;
		    p32++;
		  }
		texRefCLT[i-1].snippet.size = (chunkSize-lastOff)/4;
		texRefCLT[i-1].snippet.data = pread+2+(lastOff/4);

		if (version > 0)    /* New for 3.0 */
		  {
		    foundUV = TRUE;
		    tempUV = (float *)p32; 
		    for (i=0; i<nTex; i++)
		      {
			texRefCLT[i].uscale = *tempUV;
			tempUV++; p32++;
			texRefCLT[i].vscale = *tempUV;
			tempUV++; p32++;
		      }
		    if (numCompressed>0)
		      patchOffset = p32;
		    p32++;
		    for (i=0; i<numCompressed; i++)
		      p32++;
		  }

		ptemplateN->pselectsnippets = texRefCLT; 
		ptemplateN->ploadsnippets = pLoadSnippets;
		foundPCLT = TRUE;
	      }
	  }
	bytesparsed += *(pread + 1) + 8;
        totalBytes += *(pread + 1) + 8;
      }
    while(bytesparsed < formSize);
    totalBytes += 4;  /* Form Overhead, So it points to address of the next */
    pread = (uint32 *)((long)ploadbuf + totalBytes);

    if(!foundPCLT)
      {
	printf("ERROR: couldn't find necessary PCLT data.\n");
	exit(1);
      }
    if(!foundUV)
      {
	printf("ERROR: couldn't find necessary UV data (either in version 1.0 PCLT or version <1 M2PG.\n");
	exit(1);
      }
    curPage++;
  
    page->proutine = &M_LoadPodTexture;
    page->ptpagesnippets = ptemplateN;
    page->texturecount = 16384;

    pLoadSnippets[0].data[3] = (uint32)page->ptexture;
    pLoadSnippets[1].data[1] = (uint32)page->ppip;
    pLoadSnippets[1].data[3] = page->pipbytes;

    /* Patch the source addresses of the compressed LODs */
    for (i=0; i<numCompressed; i++)
      {
	p32 = patchOffset;
	if (p32[i]>5)  /* First LOD doesn't need to be offset, it's at start of block */
	  pLoadSnippets[0].data[p32[i]] += (uint32)page->ptexture;
      }
  }
  while (totalBytes< filesize);
  
  *num_pages = curPage;
}
	
