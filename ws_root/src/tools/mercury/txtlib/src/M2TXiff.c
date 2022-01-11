/*
	File:		M2TXiff.c

	Contains:	M2 Texture Library, iff i/o functions 

	Written by:	Todd Allendorf 

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

	   <15+>	12/17/95	TMA		Fixed compile errors.
		<15>	12/17/95	TMA		Correct Mac File creation bug which prevents overwriting of
									existing files.
	   <12+>	 9/19/95	TMA		Changed #include ":misc:iff.h" to ifflib.h
	   <11+>	 9/18/95	TMA		Got rid of compiler warning.
		<11>	 9/18/95	TMA		Made IFF writing per-chunk selectable.
		<10>	 9/18/95	TMA		Made Mac file creation set the proper file type.
		 <9>	  9/6/95	TMA		Keep M2TX_WriteToIFF from closing file when exitting.
		 <8>	  9/6/95	TMA		Added M2TX_WriteToIFF call. Added reading of color constants
									from Command Lists.
		 <7>	  8/7/95	TMA		Fix compiler warnings.  Added support for new header flags to
									control output of different chunks (TAB, DAB, LR, DCI).
		 <6>	  8/4/95	TMA		Fixed output problems of TAB, DAB, and LoadRects. Input of these
									fields is now preserved.
		 <5>	 7/25/95	TMA		Fix NumColors one too great bug.
		 <4>	 7/15/95	TMA		Autodocs updated and changes to insure only 32 bit IFF's a
									written out (no 64 bit chunks).
		<1+>	 7/11/95	TMA		Fixed includes for brain-dead Macs.
	To Do:
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __MWERKS__
#ifndef applec
#define applec
#endif
#endif


#ifdef applec

#ifndef dupFNErr
#define dupFNErr  -48
#endif

#ifdef __MWERKS__
#pragma only_std_keywords off
#endif

#ifndef __SCRIPT__
#include <Script.h>
#endif

#ifndef __FILES__
#include <Files.h>
#endif

#ifdef __MWERKS__
#pragma only_std_keywords reset
#pragma ANSI_strict reset
#endif

#endif

#ifndef SEEK_SET
#define SEEK_SET 0
#endif

#include "ifflib.h"

#include "M2TXTypes.h"
#include "M2TXiff.h"
#include "M2TXHeader.h"
#include "M2TXLibrary.h"
#include "qstream.h"
#include "qmem.h"

#include "clt.h"
#include "clttxdblend.h"

#include "M2TXattr.h"
#include "M2TXinternal.h"


Err M2TX_WriteToIFF(IFFParser *iff, M2TX *tex, bool outLOD);
M2Err M2TX_OpenFile(char *fileName, IFFParser **iff, bool writeMode, bool isMac, void *spec);

M2Err M2TX_SetDefaultLoadRect(M2TX *tex);
M2Err M2TX_CreateTAB(M2TX *tex);

static uint16 readShort(FILE *theFile)
{
  uint8 buf[2];
  
  qReadByteStream(theFile, (BYTE *)buf,  2);
  return( (buf[0] << 8) + (buf[1] <<0)); 
}

static uint32 readLong(FILE *theFile)
{
  uint8 buf[4];
  
  qReadByteStream(theFile, (BYTE *)buf, 4);
  return (((uint32) buf[0]) << 24) + (((uint32) buf[1]) << 16)
    + (((uint32) buf[2]) << 8) + buf[3];
}

static void writeShort(FILE *theFile, uint16 value)
{
  uint8 buf[2];
  
  buf[0] = (uint8)(value >> 8);
  buf[1] = (uint8)(value & 0x00FF);
  qWriteByteStream(theFile, (BYTE *)buf,  2); 
}

static void writeLong(FILE *theFile, uint32 value)
{
  uint8 buf[4];
  

  buf[0] = (uint8)(value >> 24);
  buf[1] = (uint8)((value & 0x00FF0000) >> 16);
  buf[2] = (uint8)((value & 0x0000FF00) >> 8);
  buf[3] = (uint8)(value & 0x000000FF);
  qWriteByteStream(theFile, (BYTE *)buf,  4); 
}

static uint16 getMemShort()
{
  uint8 buf[2];

	qMemGetBytes((BYTE *)buf,  2);
  	return( (buf[0] << 8) + (buf[1] <<0)); 
}

static uint32 getMemLong()
{
  uint8 buf[4];

	qMemGetBytes((BYTE *)buf,  4);
	return (((uint32) buf[0]) << 24) + (((uint32) buf[1]) << 16)
    + (((uint32) buf[2]) << 8) + buf[3];
}

static uint32 getMemFloat()
{
  uint8 *buf, temp;
  float val;
  
  qMemGetBytes((BYTE *)&val,  4);	
#ifdef INTEL
  buf = (uint8 *)&val; 
  temp = buf[0]; buf[0]= buf[3]; buf[3]=temp;
  temp = buf[1]; buf[1]= buf[2]; buf[2]=temp;
#endif
  return (val);
}

static void putMemShort(uint16 val)
{
	uint8 buf;
	
	buf = val >> 8;      /* UTF specifies Motorala or MSB 1st */
	qMemPutBytes((BYTE *)&buf,  1);
	buf = val & 0x00FF;
	qMemPutBytes((BYTE *)&buf,  1);
}

static void putMemLong(uint32 val)
{
	uint8 buf;
	
	buf = val >> 24;      /* UTF specifies Motorala or MSB 1st */
	qMemPutBytes((BYTE *)&buf,  1);
	buf = (val & 0x00FF0000) >> 16;
	qMemPutBytes((BYTE *)&buf,  1);
	buf = (val & 0x0000FF00) >> 8;
	qMemPutBytes((BYTE *)&buf,  1);
	buf = val & 0x000000FF;
	qMemPutBytes((BYTE *)&buf,  1);
}

static void putMemFloat(float val)
{
	uint8 *buf, temp;
	
	buf = (uint8 *)&val;
#ifdef INTEL
	temp = buf[0]; buf[0]= buf[3]; buf[3]=temp;
	temp = buf[1]; buf[1]= buf[2]; buf[2]=temp;
#endif
	qMemPutBytes((BYTE *)buf,  4);
}

static M2Err PIP_ComputeSize(M2TX *tex)
{
  uint8 cDepth;
  int16 numColors;
  

  M2TXPIP_GetNumColors(&(tex->PIP), &numColors);
  if (numColors <= 0)
    {
      M2TXHeader_GetCDepth(&(tex->Header),&cDepth);
      numColors = 1 << cDepth;
      if (cDepth>0)
	tex->PIP.Size = 8 +numColors*4;
      else
	tex->PIP.Size = 8;
    }
  else
    tex->PIP.Size = 8 +numColors*4;

  return(M2E_NoErr);
}

static M2Err Texel_ComputeSizeOffsets(M2TX *tex)
{	
  uint8 numLOD,i;
  uint32	length, initialOff;
  
  M2TXHeader_GetNumLOD(&(tex->Header), &numLOD);
  length=0;
  initialOff = 4 + 4*numLOD;
  
  for(i=0; i < numLOD; i++)
    {
      if (i==0)
	tex->Header.LODDataOffset[i] = initialOff;
      else
	tex->Header.LODDataOffset[i] = length + initialOff;
      length += tex->Header.LODDataLength[i];
    }
  tex->Texel.Size = initialOff + length;
  return(M2E_NoErr);
}

static M2Err PgCLT_ComputeOffsets(PgCLT *pclt)
{	
  uint32  numTex,i;
  uint32	length, initialOff;
  
  numTex = pclt->NumTex;
  length=0;
  /*
  initialOff = 16 + 4*numTex;
  */
  initialOff = 16 + 4*numTex + 8*numTex + 4 + 4*pclt->NumCompressed;
 
  pclt->PageCLTOff = initialOff;
  initialOff += pclt->PageLoadCLT.size*4;
  pclt->PIPCLTOff = initialOff;
  initialOff += pclt->PIPLoadCLT.size*4;

  if (pclt->TexRefCLT == NULL)
    return(M2E_BadPtr);

  pclt->TexCLTOff = (uint32 *)qMemClearPtr(numTex, sizeof(uint32));
  for(i=0; i < numTex; i++)
    {
      pclt->TexCLTOff[i] = initialOff;
      initialOff += pclt->TexRefCLT[i].size*4;
    }
  return(M2E_NoErr);
}

#define	ID_TXTR	 MAKE_ID('T','X','T','R')
#define	ID_M2TX	 MAKE_ID('M','2','T','X')
#define	ID_M2PI	 MAKE_ID('M','2','P','I')
#define	ID_M2CI	 MAKE_ID('M','2','C','I')
#define	ID_M2TD	 MAKE_ID('M','2','T','D')
#define	ID_M2TA	 MAKE_ID('M','2','T','A')
#define	ID_M2LR	 MAKE_ID('M','2','L','R')
#define	ID_M2DB MAKE_ID('M','2','D','B')
#define	ID_M2PG MAKE_ID('M','2','P','G')
#define	ID_PCLT MAKE_ID('P','C','L','T')

static IFFTypeID utfprops[] =
 {
ID_TXTR, ID_M2TX,
ID_TXTR, ID_M2PI,
ID_TXTR, ID_M2CI,
ID_TXTR, ID_M2TD,
ID_TXTR, ID_M2TA,
ID_TXTR, ID_M2LR,
ID_TXTR, ID_M2DB,
ID_TXTR, ID_M2PG,
ID_TXTR, ID_PCLT,
0
};

static IFFTypeID utfnoLODsprops[] =
 {
ID_TXTR, ID_M2TX,
ID_TXTR, ID_M2PI,
ID_TXTR, ID_M2CI,
ID_TXTR, ID_M2TA,
ID_TXTR, ID_M2LR,
ID_TXTR, ID_M2DB,
ID_TXTR, ID_M2PG,
ID_TXTR, ID_PCLT,
0
};

static
Err ProcessTXTR(IFFParser *iff, M2TX *tex)
{
  PropChunk	*pc;

  uint32        temp32, dataSize, i, j, dataOffset, nextOff;
  uint16        temp16;
  uint8         temp8, numLOD;
  uint32        numLR, numPg, version;
  M2TXTex       texelPtr;
  M2Err         myErr;
  bool          empty = TRUE;
  
/*   tex = (M2TX *) FindFrame (iff, ID_TXTR);
*/  
  if (pc = FindPropChunk (iff, ID_TXTR, ID_M2TX))
    {
      if (pc -> pc_DataSize < 16)
	  return (IFF_ERR_MANGLED);
      tex->Header.Size = pc->pc_DataSize;
      BeginMemGetBytes((BYTE *)pc->pc_Data);
      temp32 = getMemLong();   /* Reserved field */
      tex->Header.Flags = getMemLong();
      tex->Header.MinXSize = getMemShort();
      tex->Header.MinYSize = getMemShort();
      tex->Header.TexFormat = getMemShort();
      qMemGetBytes((BYTE *)&(tex->Header.NumLOD), 1);
      qMemGetBytes((BYTE *)&(tex->Header.Border), 1);
      qMemGetBytes((BYTE *)&temp8, 1);      /* Reserved field #2 */
      EndMemGetBytes();
      empty=FALSE;
    }
  
  if (pc = FindPropChunk (iff, ID_TXTR, ID_M2PI))
    {
      M2TXHeader_SetFHasPIP(&(tex->Header), TRUE);
      dataSize = pc->pc_DataSize;
      if (dataSize < 4)
	{
	  return (IFF_ERR_MANGLED);
	}
      BeginMemGetBytes((BYTE *)pc->pc_Data);
      tex->PIP.Size = dataSize;
      tex->PIP.IndexOffset = getMemLong();
      temp32 = getMemLong();   /* Reserved field */
      dataSize -= 8;
      i=0;
      for (i=0; dataSize > 3; i++, dataSize -=4)
	tex->PIP.PIPData[i] = getMemLong();
      tex->PIP.NumColors = i;
      EndMemGetBytes();  
      empty=FALSE;
    }
  else
    {
      M2TXHeader_SetFHasPIP(&(tex->Header), FALSE);
      tex->PIP.NumColors = -1;
    }
  
  if (pc = FindPropChunk (iff, ID_TXTR, ID_M2CI))
    {
      dataSize = pc->pc_DataSize;
      if (dataSize < 24)
	{
	  return (IFF_ERR_MANGLED);
	}
      tex->DCI.Size = dataSize;
      BeginMemGetBytes((BYTE *)pc->pc_Data);
      for(i=0; i<4; i++)
	tex->DCI.TexelFormat[i] = getMemShort();   /* Reserved field */
      for(i=0; i<4; i++)
	tex->DCI.TxExpColorConst[i] = getMemLong();   /* Reserved field */
      EndMemGetBytes();  
      empty=FALSE;
    }
	  
  if (pc = FindPropChunk (iff, ID_TXTR, ID_M2TD))
    {
      BeginMemGetBytes((BYTE *)pc->pc_Data);
      dataSize = pc->pc_DataSize;
      if (dataSize < 4)
	{
	  return (IFF_ERR_MANGLED);
	}
      tex->Texel.Size = dataSize;
      numLOD = temp16 = getMemShort();   /* NumLOD */
      if (tex->Header.NumLOD != numLOD)
	{
	  fprintf(stderr, "Warning. Number of levels of detail in header=%d,\n",
		 tex->Header.NumLOD);
	  fprintf(stderr,"but Number of levels of detail in texel data = %d\n",
		 numLOD);
	}
      temp16 = getMemShort();   /* Reserved field */
      
      for(i=0; i < numLOD; i++)
	{
	  tex->Header.LODDataOffset[i] = getMemLong();   /* Reserved field */
	}
      for(i=0; i < numLOD; i++)
	{      
	  dataOffset = tex->Header.LODDataOffset[i];
	  if (i<(numLOD-1))
	    nextOff = tex->Header.LODDataOffset[i+1];
	  else
	    nextOff = pc->pc_DataSize;
	  dataSize = nextOff-dataOffset;
	  texelPtr = (M2TXTex)qMemNewPtr(dataSize);
	  qMemGetBytes((BYTE *)texelPtr, dataSize);
	  M2TXHeader_SetLODPtr(&(tex->Header),i, dataSize, texelPtr);
	}	
      EndMemGetBytes();
      empty=FALSE;
    }
  
  if (pc = FindPropChunk (iff, ID_TXTR, ID_M2TA))
    {
      dataSize = pc->pc_DataSize;
      tex->TexAttr.Size=dataSize;
      if (dataSize < 4)
	{
	  return (IFF_ERR_MANGLED);
	}
      BeginMemGetBytes((BYTE *)pc->pc_Data);
      temp32 = getMemLong();   /* Reserved field */
      dataSize -= 4;
      i=0;
      tex->TexAttr.Tx_Attr = qMemNewPtr(dataSize);
      for (i=0; dataSize > 3; i++, dataSize -=4)
	tex->TexAttr.Tx_Attr[i] = getMemLong();
      EndMemGetBytes();  

      myErr = M2TXTA_GetAttr(&(tex->TexAttr), TXA_PipConstSSB0, &temp32);
      if (myErr == M2E_NoErr)
	{
	  M2TXHeader_SetFHasColorConst(&(tex->Header), TRUE); 
	  M2TXHeader_SetColorConst(&(tex->Header), 0, (M2TXColor)temp32);
	}
      myErr = M2TXTA_GetAttr(&(tex->TexAttr), TXA_PipConstSSB1, &temp32);
      if (myErr == M2E_NoErr)
	{
	  M2TXHeader_SetFHasColorConst(&(tex->Header), TRUE); 
	  M2TXHeader_SetColorConst(&(tex->Header), 1, (M2TXColor)temp32);
	}
      empty=FALSE;
    }
  else
    {
      tex->TexAttr.Reserved = 0;
      tex->TexAttr.Size = 4;
      tex->TexAttr.Tx_Attr = NULL;
    }
  
  if (pc = FindPropChunk (iff, ID_TXTR, ID_M2DB))
    {
      dataSize = pc->pc_DataSize;
      tex->DBl.Size=dataSize;
      if (dataSize < 4)
	{
	  return (IFF_ERR_MANGLED);
	}
      BeginMemGetBytes((BYTE *)pc->pc_Data);
      temp32 = getMemLong();   /* Reserved field */
      dataSize -= 4;
      i=0;
      tex->DBl.DBl_Attr = qMemNewPtr(dataSize);
      for (i=0; dataSize > 3; i++, dataSize -=4)
	tex->DBl.DBl_Attr[i] = getMemLong();
      EndMemGetBytes();
      empty=FALSE;
    }
  else
    {
      tex->DBl.Size = 0;
      tex->DBl.Reserved = 0;
      tex->DBl.DBl_Attr = NULL;
    }
  
  if (pc = FindPropChunk (iff, ID_TXTR, ID_M2LR))
    {
      dataSize = pc->pc_DataSize;
      tex->LoadRects.Size=dataSize;
      if (dataSize < 8)
	{
	  return (IFF_ERR_MANGLED);
	}
      BeginMemGetBytes((BYTE *)pc->pc_Data);
      numLR = tex->LoadRects.NumLoadRects = getMemLong(); 
      temp32 = getMemLong();   /* Reserved field */
      dataSize -= 8;
      i=0;
  
      tex->LoadRects.LRData = qMemNewPtr(numLR*sizeof(M2TXLR));
      for (i=0; i<numLR; i++)
	{
	  tex->LoadRects.LRData[i].XWrapMode = getMemShort();
	  tex->LoadRects.LRData[i].YWrapMode = getMemShort();
	  tex->LoadRects.LRData[i].FirstLOD = getMemShort();
	  tex->LoadRects.LRData[i].NLOD = getMemShort();
	  tex->LoadRects.LRData[i].XOff = getMemShort();
	  tex->LoadRects.LRData[i].YOff = getMemShort();
	  tex->LoadRects.LRData[i].XSize = getMemShort();
	  tex->LoadRects.LRData[i].YSize = getMemShort();
	}
      EndMemGetBytes();  
      empty=FALSE;
    }
  else
    {
      tex->LoadRects.Size=8;
    }
  
  if (pc = FindPropChunk (iff, ID_TXTR, ID_M2PG))
    {
      dataSize = pc->pc_DataSize;
      tex->Page.Size=dataSize;
      if (dataSize < 8)
	{
	  return (IFF_ERR_MANGLED);
	}
      BeginMemGetBytes((BYTE *)pc->pc_Data);
      numPg = tex->Page.NumTex = getMemLong();
      tex->Page.Version = getMemLong();   /* Version field */
      dataSize -= 8;
      i=0;
  
      tex->Page.PgData = qMemNewPtr(numPg*sizeof(M2TXPgHeader));
      for (i=0; i<numPg; i++)
	{
	  tex->Page.PgData[i].Offset = getMemLong();
	  tex->Page.PgData[i].PgFlags = getMemLong();
	  tex->Page.PgData[i].MinXSize = getMemShort();
	  tex->Page.PgData[i].MinYSize = getMemShort();
	  tex->Page.PgData[i].TexFormat = getMemShort();
	  qMemGetBytes((BYTE *)&(tex->Page.PgData[i].NumLOD), 1);
	  qMemGetBytes((BYTE *)&(tex->Page.PgData[i].PIPIndexOffset), 1);
	  /* For < Mercury 3.0 */
	  if ( tex->Page.Version <1)
	    {
	      qMemGetBytes((BYTE *)&(tex->Page.PgData[i].PgPIPIndex), 1);
	      qMemGetBytes((BYTE *)&(tex->Page.PgData[i].PgTABIndex), 1);
	      qMemGetBytes((BYTE *)&(tex->Page.PgData[i].PgDABIndex), 1);
	      qMemGetBytes((BYTE *)&(tex->Page.PgData[i].PgLRIndex), 1);
	    }
	  else
	    {
	      if (tex->Page.PgData[i].PgFlags & M2PG_FLAGS_IsCompressed)
		{
		  for (j=0; j<4; j++)
		    tex->Page.PgData[i].TexelFormat[j] = getMemShort();
		  for (j=0; j<4; j++)
		    tex->Page.PgData[i].TxExpColorConst[j] = getMemLong();
		  for (j=0; j<tex->Page.PgData[i].NumLOD; j++)
		    tex->Page.PgData[i].LODLength[j] = getMemLong();
		}
	    }
	}
      EndMemGetBytes();  
      empty=FALSE;
    }
  else
    {
      tex->Page.Size = 8;
      tex->Page.NumTex = 0;
      tex->Page.PgData = NULL;
    }

  if (pc = FindPropChunk (iff, ID_TXTR, ID_PCLT))
    {
      dataSize = pc->pc_DataSize;
      tex->PCLT.Size=dataSize;
      if (dataSize < 16)
	{
	  return (IFF_ERR_MANGLED);
	}
      BeginMemGetBytes((BYTE *)pc->pc_Data);
      numPg = tex->PCLT.NumTex = getMemLong();
      version = getMemLong();   /* Reserved field */
      if (version > 0)
	tex->PCLT.NumCompressed = getMemLong();
      dataOffset = tex->PCLT.PageCLTOff = getMemLong();
      nextOff = tex->PCLT.PIPCLTOff = getMemLong();
      if (numPg>0)
	{
	  tex->PCLT.TexCLTOff = (uint32 *)qMemNewPtr(numPg*sizeof(uint32));
	  tex->PCLT.TexRefCLT = (CltSnippet *)qMemNewPtr(numPg*sizeof(CltSnippet));
	}
      else
	{
	  tex->PCLT.TexCLTOff = NULL;
	  tex->PCLT.TexRefCLT = NULL;
	}
      for (i=0; i<numPg; i++)
	tex->PCLT.TexCLTOff[i] = getMemLong();
      if (version>0)
	{
	  tex->PCLT.UVScale = (float *)qMemNewPtr(2*numPg*sizeof(float));
	  for (i=0; i<numPg; i++)
	    {
	      tex->PCLT.UVScale[2*i] = getMemFloat();
	      tex->PCLT.UVScale[2*i+1] = getMemFloat();
	    }
	  tex->PCLT.PatchOffset = (uint32 *)qMemNewPtr(2*numPg*sizeof(uint32));
	  for (i=0; i<tex->PCLT.NumCompressed; i++)
	    {
	      tex->PCLT.PatchOffset[i] = getMemLong();
	    }
	}
      else
	{
	  tex->PCLT.NumCompressed = 0;
	  tex->PCLT.UVScale = NULL;
	  tex->PCLT.PatchOffset = NULL;
	}

      temp32 = nextOff-dataOffset;
      tex->PCLT.PageLoadCLT.data = (uint32 *)qMemNewPtr(temp32);
      tex->PCLT.PageLoadCLT.size = temp32/4;
      tex->PCLT.PageLoadCLT.allocated = temp32/4;
      for (i=0; i<(temp32/4); i++)
	tex->PCLT.PageLoadCLT.data[i] = getMemLong();

      dataOffset = nextOff;
      if (numPg<1)
	nextOff = dataSize;
      else
	nextOff = tex->PCLT.TexCLTOff[0];
      temp32 = nextOff-dataOffset;
      tex->PCLT.PIPLoadCLT.data = (uint32 *)qMemNewPtr(temp32);
      tex->PCLT.PIPLoadCLT.size = temp32/4;
      tex->PCLT.PIPLoadCLT.allocated = temp32/4;
      for (i=0; i<(temp32/4); i++)
	tex->PCLT.PIPLoadCLT.data[i] = getMemLong();

      dataOffset = nextOff;
      for (j=0; j<numPg; j++)
	{
	  if (j<(numPg-1))
	    nextOff = tex->PCLT.TexCLTOff[j+1];
	  else
	    nextOff = dataSize;
	  temp32 = nextOff-dataOffset;
	  tex->PCLT.TexRefCLT[j].data = (uint32 *)qMemNewPtr(temp32);
	  tex->PCLT.TexRefCLT[j].size = temp32/4;
	  tex->PCLT.TexRefCLT[j].allocated = temp32/4;
	  for (i=0; i<(temp32/4); i++)
	    tex->PCLT.TexRefCLT[j].data[i] = getMemLong();
	  dataOffset = nextOff;
	}      
      EndMemGetBytes();  
      empty=FALSE;
    }
  else
    {
      tex->PCLT.NumTex = 0;
      tex->PCLT.NumCompressed = 0;
      tex->PCLT.TexCLTOff = NULL;
      tex->PCLT.TexRefCLT = NULL;
      tex->PCLT.UVScale = NULL;
      tex->PCLT.PatchOffset = NULL;
      tex->PCLT.PageLoadCLT.size = 0;
      tex->PCLT.PIPLoadCLT.size = 0;
    }

  if (empty==TRUE)
    return(IFF_ERR_MANGLED);
  return(0);
}

#define WRITE_BUFFER_SIZE 1048
Err M2TX_WriteChunkData(IFFParser *iff, M2TX *tex, uint32 write_flags )
{
  Err		result;
  M2Err         returnErr;
  M2TXRect      *rect;
  M2TXPgHeader  *page;
  PgCLT         *pclt;
  uint8 buffer[WRITE_BUFFER_SIZE];
  uint8 numLOD;
  uint8 temp8;
  uint16 temp16;
  bool flag;
  uint32 dataSize, i, j, numLR, numPg;
  bool outTXTR = ( write_flags & M2TX_WRITE_TXTR )? TRUE : FALSE;
  bool outM2TX = ( write_flags & M2TX_WRITE_M2TX )? TRUE : FALSE;
  bool outFORM = ( write_flags & M2TX_WRITE_FORM )? TRUE : FALSE;
  bool outLOD = ( write_flags & M2TX_WRITE_LOD )? TRUE : FALSE;
  bool outPIP = ( write_flags & M2TX_WRITE_PIP )? TRUE : FALSE;
  bool outTAB = ( write_flags & M2TX_WRITE_TAB )? TRUE : FALSE;
  bool outDAB = ( write_flags & M2TX_WRITE_DAB )? TRUE : FALSE;
  bool outLR =  ( write_flags & M2TX_WRITE_LR )? TRUE : FALSE;
  bool outDCI =  ( write_flags & M2TX_WRITE_DCI )? TRUE : FALSE;
  bool outPG =  ( write_flags & M2TX_WRITE_M2PG )? TRUE : FALSE;
 bool outPCLT =  ( write_flags & M2TX_WRITE_PCLT )? TRUE : FALSE;
  
 if (iff == NULL)
   {
     result = -1;
     return(result);
   }
  
 /* write texture header */
 if (outTXTR)
   {  
     result = PushChunk (iff, ID_TXTR, ID_FORM, IFF_SIZE_UNKNOWN_32);
     if (result < 0)
       {
	 return(result);
       }
	
     result = PushChunk (iff, 0L, ID_M2TX,  16);
     if (result < 0)
       {
	 return(result);
       }
	  
     BeginMemPutBytes((BYTE *)buffer, 16);
     putMemLong(0L);   /* Reserved field */
     putMemLong(tex->Header.Flags & 0x0001); 
     /* The output file only knows about the first flag */
     putMemShort(tex->Header.MinXSize);
     putMemShort(tex->Header.MinYSize);
     putMemShort(tex->Header.TexFormat);
	
     if (outLOD)
       numLOD = tex->Header.NumLOD;
     else
       numLOD = 0;
	
     qMemPutBytes((BYTE *)&numLOD, 1);
     temp8 = 0;
     qMemPutBytes((BYTE *)&temp8, 1);
     EndMemPutBytes();
     if (WriteChunk (iff, buffer, 16L) != 16)
       {
	 result = -1;
	 return(result);
       }
	
     result = PopChunk (iff);
     if (result < 0)
       return(result);
   }
 else if (outM2TX)
   {
     result = PushChunk (iff, 0L, ID_M2TX,  16);
     if (result < 0)
       {
	 return(result);
       }
	  
     BeginMemPutBytes((BYTE *)buffer, 16);
     putMemLong(0L);   /* Reserved field */
     putMemLong(tex->Header.Flags & 0x0001); 
     /* The output file only knows about the first flag */
     putMemShort(tex->Header.MinXSize);
     putMemShort(tex->Header.MinYSize);
     putMemShort(tex->Header.TexFormat);
	
      numLOD = tex->Header.NumLOD;
	
     qMemPutBytes((BYTE *)&numLOD, 1);
     temp8 = 0;
     qMemPutBytes((BYTE *)&temp8, 1);
     EndMemPutBytes();
     if (WriteChunk (iff, buffer, 16L) != 16)
       {
	 result = -1;
	 return(result);
       }
	
     result = PopChunk (iff);
     if (result < 0)
       return(result);
   }
 else if (outFORM)
   {

     result = PushChunk (iff, ID_TXTR, ID_FORM, IFF_SIZE_UNKNOWN_32);
     if (result < 0)
       {
	 return(result);
       }
   }

  M2TXHeader_GetFHasPIP(&(tex->Header), &flag);
  if (flag && outPIP)
    {
      result = PushChunk (iff, 0L, ID_M2PI,  IFF_SIZE_UNKNOWN_32);
      if (result < 0)
	return(result);
      PIP_ComputeSize(tex);
      BeginMemPutBytes((BYTE *)buffer, tex->PIP.Size);
      putMemLong(tex->PIP.IndexOffset);
      putMemLong(0L);                    /* Reserved */
      dataSize = tex->PIP.Size-8;
      for (i=0; dataSize > 3; i++, dataSize -=4)
	putMemLong(tex->PIP.PIPData[i]);
      EndMemPutBytes();
      if (WriteChunk (iff, buffer, tex->PIP.Size) != tex->PIP.Size)
	{
	  result=-1;
	  return(result);
	}
      result = PopChunk (iff);
      if (result < 0)
	return(result);
    }

  M2TXHeader_GetFIsCompressed(&(tex->Header), &flag);
  if (flag && outDCI)
    {
      M2TXHeader_GetFHasNoDCI(&(tex->Header), &flag);
      if (!flag && outDCI)
	{
      result = PushChunk (iff, 0L, ID_M2CI,  24);
      if (result < 0)
	return(result);

      BeginMemPutBytes((BYTE *)buffer, 24);
      for (i=0; i<4; i++)
	putMemShort(tex->DCI.TexelFormat[i]);
      for (i=0; i<4; i++)
	putMemLong(tex->DCI.TxExpColorConst[i]);
      EndMemPutBytes();
      if (WriteChunk (iff, buffer, 24L) != 24)
	{
	  result = -1;
	  return(result);
	}
      result = PopChunk (iff);
      if (result < 0)
	return(result);
    }
    }
  

  M2TXHeader_GetFHasNoTAB(&(tex->Header), &flag);
  if (!flag && outTAB)
    {
      returnErr = M2TX_CreateTAB(tex);
      if (returnErr == M2E_NoErr)
	{
	  result = PushChunk (iff, 0L, ID_M2TA, IFF_SIZE_UNKNOWN_32);
	  if (result < 0)
	    {
	      result = -1;
	      return(result);
	    }
	  BeginMemPutBytes((BYTE *)buffer, 4);
	  putMemLong(0);   /* Reserved field */
	  EndMemPutBytes();
	  if (WriteChunk (iff, buffer, 4L) != 4)
	    {
	      result = -1;
	      return(result);
	    }
	  dataSize = tex->TexAttr.Size - 4;
	  if (dataSize < WRITE_BUFFER_SIZE)
	    {
	      BeginMemPutBytes((BYTE *)buffer, dataSize);
	      for (i=0; i<dataSize/4; i++)
		putMemLong(tex->TexAttr.Tx_Attr[i]);   /* Reserved field */
	      EndMemPutBytes();
	      if (WriteChunk (iff, buffer, dataSize) != dataSize)
		{
		  result = -1;
		  return(result);
		}
	    }
	  else 
	    return(-1);
	  result = PopChunk (iff);
	  if (result < 0)
	    return(result);
	}
    }
  
  M2TXHeader_GetFHasNoDAB(&(tex->Header), &flag);
  if (!flag && outDAB)
    {
      if (tex->DBl.DBl_Attr != NULL)
	{
	  result = PushChunk (iff, 0L, ID_M2DB, IFF_SIZE_UNKNOWN_32);
	  if (result < 0)
	    {
	      result = -1;
	      return(result);
	    }
	  BeginMemPutBytes((BYTE *)buffer, 4);
	  putMemLong(0);   /* Reserved field */
	  EndMemPutBytes();
	  if (WriteChunk (iff, buffer, 4L) != 4)
	    {
	      result = -1;
	      return(result);
	    }
	  dataSize = tex->DBl.Size - 4;
	  if (dataSize < WRITE_BUFFER_SIZE)
	    {
	      BeginMemPutBytes((BYTE *)buffer, dataSize);
	      for (i=0; i<dataSize/4; i++)
		putMemLong(tex->DBl.DBl_Attr[i]);   /* Reserved field */
	      EndMemPutBytes();
	      if (WriteChunk (iff, buffer, dataSize) != dataSize)
		{
		  result = -1;
		  return(result);
		}
	    }
	  else 
	    return(-1);
	  result = PopChunk (iff);
	  if (result < 0)
	    return(result); 
	}
    }
  
  M2TXHeader_GetFHasNoLR(&(tex->Header), &flag);
  if (!flag && outLR)
    {
      returnErr = M2TX_SetDefaultLoadRect(tex);
      rect = tex->LoadRects.LRData;
      if ((returnErr == M2E_NoErr) && (rect != NULL))
	{
	  numLR = tex->LoadRects.NumLoadRects; 
	  if ((numLR > 1) || (rect->XWrapMode ==  TX_WrapModeTile) ||
	      (rect->YWrapMode == TX_WrapModeTile))
	    {
	      result = PushChunk (iff, 0L, ID_M2LR, IFF_SIZE_UNKNOWN_32);
	      if (result < 0)
		{
		  result = -1;
		  return(result);
		}
	      BeginMemPutBytes((BYTE *)buffer, 8);
	      putMemLong(numLR);
	      putMemLong(0);   /* Reserved field */
	      EndMemPutBytes();
	      if (WriteChunk (iff, buffer, 8L) != 8)
		{
		  result = -1;
		  return(result);
		}
	      dataSize = sizeof(M2TXRect);
	      for (i=0; i<numLR; i++, rect++)
		{
		  BeginMemPutBytes((BYTE *)buffer, dataSize);
		  putMemShort(rect->XWrapMode);
		  putMemShort(rect->YWrapMode);
		  putMemShort(rect->FirstLOD);	
		  putMemShort(rect->NLOD);	
		  putMemShort(rect->XOff);	
		  putMemShort(rect->YOff);	
		  putMemShort(rect->XSize);	
		  putMemShort(rect->YSize);	
		  EndMemPutBytes();
		  if (WriteChunk (iff, buffer, dataSize) != dataSize)
		    {
		      result = -1;
		      return(result);
		    }
		}
	      result = PopChunk (iff);
	      if (result < 0)
		return(result);      
	    }
	}
    }
  

  numPg = tex->PCLT.NumTex;
  if ((numPg>0) && outPCLT)
    {
      pclt = &(tex->PCLT);
      result = PushChunk (iff, 0L, ID_PCLT, IFF_SIZE_UNKNOWN_32);
      if (result < 0)
	{
	  result = -1;
	  return(result);
	}

      returnErr = PgCLT_ComputeOffsets(pclt);

      BeginMemPutBytes((BYTE *)buffer, 20);
      putMemLong(numPg);
      putMemLong(1);   /* Version field */
      /* New for Mercury 3.0 */
      putMemLong(pclt->NumCompressed);
      putMemLong(pclt->PageCLTOff);
      putMemLong(pclt->PIPCLTOff);
      EndMemPutBytes();
      if (WriteChunk (iff, buffer, 20L) != 20)
	{
	  result = -1;
	  return(result);
	}


      for (i=0; i<numPg; i++)
	{
	  BeginMemPutBytes((BYTE *)buffer, 4);
	  putMemLong(pclt->TexCLTOff[i]);
	  if (WriteChunk (iff, buffer, 4L) != 4)
	    {
	      result = -1;
	      return(result);
	    }
	  EndMemPutBytes();
	}

      /* New for Mercury 3.0 */
      for (i=0; i<numPg; i++)
	{
	  BeginMemPutBytes((BYTE *)buffer, 8);
	  putMemFloat(pclt->UVScale[i*2]);
	  putMemFloat(pclt->UVScale[i*2+1]);
	  if (WriteChunk (iff, buffer, 8L) != 8)
	    {
	      result = -1;
	      return(result);
	    }
	  EndMemPutBytes();
	}
      
      /* New for Mercury 3.0 */
      for (i=0; i<pclt->NumCompressed; i++)
	{
	  BeginMemPutBytes((BYTE *)buffer, 4);
	  putMemLong(pclt->PatchOffset[i]);
	  if (WriteChunk (iff, buffer, 4L) != 4)
	    {
	      result = -1;
	      return(result);
	    }
	  EndMemPutBytes();
	}

      dataSize = pclt->PageLoadCLT.size*4;
      if (dataSize > WRITE_BUFFER_SIZE)
	{
	  fprintf(stderr,"ERROR:CLTSnippet size %d too large for me!\n",
		  dataSize);
	  return(M2E_NoMem);
	}
      BeginMemPutBytes((BYTE *)buffer, dataSize);
      for (i=0; i<dataSize/4; i++)
	putMemLong(pclt->PageLoadCLT.data[i]);
      if (WriteChunk (iff, buffer, dataSize) != dataSize)
	{
	  result = -1;
	  return(result);
	}
      EndMemPutBytes();

      dataSize = pclt->PIPLoadCLT.size*4;
      if (dataSize > WRITE_BUFFER_SIZE)
	{
	  fprintf(stderr,"ERROR:CLTSnippet size %d too large for me!\n",
		  dataSize);
	  return(M2E_NoMem);
	}
      BeginMemPutBytes((BYTE *)buffer, dataSize);
      for (i=0; i<dataSize/4; i++)
	putMemLong(pclt->PIPLoadCLT.data[i]);
      if (WriteChunk (iff, buffer, dataSize) != dataSize)
	{
	  result = -1;
	  return(result);
	}
      EndMemPutBytes();

      for (j=0; j<numPg; j++)
	{
	  dataSize = pclt->TexRefCLT[j].size*4;
	  if (dataSize > WRITE_BUFFER_SIZE)
	    {
	      fprintf(stderr,"ERROR:CLTSnippet size %d too large for me!\n",
		      dataSize);
	      return(M2E_NoMem);
	    }
	  BeginMemPutBytes((BYTE *)buffer, dataSize);
	  for (i=0; i<dataSize/4; i++)
	    putMemLong(pclt->TexRefCLT[j].data[i]);
	  if (WriteChunk (iff, buffer, dataSize) != dataSize)
	    {
	      result = -1;
	      return(result);
	    }
	  EndMemPutBytes();
	}

      result = PopChunk (iff);
      if (result < 0)
	return(result);      
    }

  numPg = tex->Page.NumTex;
  if ((numPg>0) && outPG)
    {
      page = tex->Page.PgData;
      if ((page != NULL))
	{
	  numPg = tex->Page.NumTex; 
	  result = PushChunk (iff, 0L, ID_M2PG, IFF_SIZE_UNKNOWN_32);
	  if (result < 0)
	    {
	      result = -1;
	      return(result);
	    }
	  BeginMemPutBytes((BYTE *)buffer, 8);
	  putMemLong(numPg);
	  putMemLong(1);   /* Version field, For 3.0 == 1 */
	  EndMemPutBytes();
	  if (WriteChunk (iff, buffer, 8L) != 8)
	    {
	      result = -1;
	      return(result);
	    }
	  for (i=0; i<numPg; i++, page++)
	    {
	      dataSize = 16;
	      if (page->PgFlags & M2PG_FLAGS_IsCompressed)
		dataSize = dataSize + 24 + 4*page->NumLOD;     
	      BeginMemPutBytes((BYTE *)buffer, dataSize);
	      putMemLong(page->Offset);
	      putMemLong(page->PgFlags);
	      putMemShort(page->MinXSize);	
	      putMemShort(page->MinYSize);	
	      putMemShort(page->TexFormat);
	      numLOD = page->NumLOD;
	      qMemPutBytes((BYTE *)&numLOD, 1);
	      temp8 = page->PIPIndexOffset;
	      qMemPutBytes((BYTE *)&temp8, 1);
	      /*  For version 0 which we don't write out anymore
		  temp8 = page->PgPIPIndex;
		  qMemPutBytes((BYTE *)&temp8, 1);
		  temp8 = page->PgTABIndex;
		  qMemPutBytes((BYTE *)&temp8, 1);
		  temp8 = page->PgDABIndex;
		  qMemPutBytes((BYTE *)&temp8, 1);
		  temp8 = page->PgLRIndex;
		  qMemPutBytes((BYTE *)&temp8, 1);
		  */
	      if (page->PgFlags & M2PG_FLAGS_IsCompressed)
		{		  
		  for (j=0; j<4; j++)
		    putMemShort(page->TexelFormat[j]);
		  for (j=0; j<4; j++)
		    putMemLong(page->TxExpColorConst[j]);
		  for(j=0; j<page->NumLOD; j++)
		    putMemLong(page->LODLength[j]);
		}
	      if (WriteChunk (iff, buffer, dataSize) != dataSize)
		{
		  result = -1;
		  return(result);
		}
	    }
	  result = PopChunk (iff);
	  if (result < 0)
	    return(result);      
	}
    }

  if(outLOD) 
    {
      result = PushChunk (iff, 0L, ID_M2TD,  IFF_SIZE_UNKNOWN_32);
      if (result < 0)
	{
	  result = -1;
	  return(result);
	}
      BeginMemPutBytes((BYTE *)buffer, 64);
      numLOD = temp16 = tex->Header.NumLOD;
      putMemShort(temp16); 
      putMemShort(0);   /* Reserved field */
      Texel_ComputeSizeOffsets(tex);
      for (i=0; i<numLOD; i++)
	putMemLong(tex->Header.LODDataOffset[i]);
      EndMemPutBytes();
      dataSize = 4 + 4 * numLOD;
      if (WriteChunk (iff, buffer, dataSize) != dataSize)
	{
	  result = -1;
	  return(result);
	}
      for (i=0; i<numLOD; i++)
	{
	  dataSize = tex->Header.LODDataLength[i];
	  if (WriteChunk(iff,tex->Header.LODDataPtr[i], dataSize) != dataSize)
	    {
	      result = -1;
	    return(result);
	    }
	}
      result = PopChunk (iff);
      if (result < 0)
	return(result);      

    }
  if (outTXTR || outFORM) result = PopChunk (iff);
  
  return (result);
  
}

Err M2TX_WriteToIFF(IFFParser *iff, M2TX *tex, bool outLOD )
{
  if (outLOD)
	return(M2TX_WriteChunkData(iff, tex, M2TX_WRITE_ALL ));
  else
    return(M2TX_WriteChunkData(iff, tex,  
			((uint32)(M2TX_WRITE_ALL))& 
			(~((uint32)(M2TX_WRITE_LOD)))));
}



M2Err M2TX_OpenFile(char *fileName, IFFParser **iff, bool writeMode, bool isMac, void *spec)
{
  Err       result;
  TagArg    tags[2];
  M2Err     returnErr = M2E_NoErr;

#ifdef applec
  if (isMac)
    {
      tags[0].ta_Tag = IFF_TAG_FILE_MAC;
      tags[0].ta_Arg = spec;
      tags[1].ta_Tag = TAG_END;
    }
  else
    {
#endif
      tags[0].ta_Tag = IFF_TAG_FILE;
      tags[0].ta_Arg = fileName;
      tags[1].ta_Tag = TAG_END;      
#ifdef applec
    }
#endif

  result = CreateIFFParser (iff, writeMode, tags);

  if (result >= 0)
    return(returnErr);
  else    
    switch(result)
      {
      case IFF_ERR_NOMEM: 
	returnErr = M2E_NoMem;
	break;
      case IFF_ERR_BADTAG:
	returnErr = M2E_Range;
	break;
      case IFF_ERR_NOTIFF:
      case -1:
	returnErr = M2E_BadFile;
	break;
      case IFF_ERR_BADPTR:
      case IFF_ERR_BADLOCATION:
      default:
	returnErr = M2E_BadPtr;
	break;
      }  
  return(returnErr);
}

M2Err M2TX_WriteIFF(M2TX *tex, bool outLOD, char *fileName, 
		    bool isMac, void *spec)
{ 
  IFFParser	*iff = NULL;
  Err		result;
  TagArg        tags[2];
  M2Err         returnErr = M2E_NoErr;
  

  returnErr = M2TX_OpenFile(fileName, &iff, TRUE, isMac, spec); 

  if (returnErr != M2E_NoErr)
    return(returnErr);

  result = M2TX_WriteToIFF(iff, tex, outLOD);
  
  if (result < 0)
    {
      switch(result)
	{
	case IFF_ERR_NOMEM: 
	  returnErr = M2E_NoMem;
	  break;
	case IFF_ERR_BADTAG:
	  returnErr = M2E_Range;
	  break;
	case IFF_ERR_NOTIFF:
	case -1:
	  returnErr = M2E_BadFile;
	  break;
	case IFF_ERR_BADPTR:
	case IFF_ERR_BADLOCATION:
	default:
	  returnErr = M2E_BadPtr;
	  break;
	}  
    }
  if (iff)
    {
      result = DeleteIFFParser(iff);
      if ((result<0) && (returnErr != M2E_NoErr))
	returnErr = M2E_Limit;
    }    
  return(returnErr);
}

static Err stopOnExit(IFFParser *iff, void *userData)
{
  return (IFF_PARSE_EOC);
}


Err TXTR_SetupFrame(IFFParser *iff, bool inLOD)
{
  Err result;

  /*  result = MakeFrame (iff, ID_TXTR, CreateUTFframe, FreeUTFframe); */
  result = InstallExitHandler (iff, ID_TXTR, ID_FORM,
			       IFF_CIL_TOP, stopOnExit, iff);

  if (result<0)
    return(result);
  
  /*
   * Declare property, collection and stop chunks.
   * (You still have to do this because YOU handle the data.)
   */
  
  if (inLOD)
    result = RegisterPropChunks (iff, utfprops);
  else
    result = RegisterPropChunks (iff, utfnoLODsprops);

  /*
    result = RegisterStopChunks (iff, utfstops);
    if (result<0)
    goto err;
    */
  return(result);
}

Err TXTR_GetNext(IFFParser *iff, M2TX *tex, bool *foundM2)
{
  Err result;
  ContextNode	*top;

  result = 0;
  while (result>=0)
    {
      /*
       * ParseIFF() when the frame is filled.  In any case, we 
       * should nab a frame for this context (even if
       * it's really NULL).
       */
      
      result = ParseIFF (iff, IFF_PARSE_SCAN);
      
      if (result == IFF_PARSE_EOC)
	{
	  top = GetCurrentContext (iff);
	  /* if (GetParentContext(top)) */
	  if (top)
	    {
	      /* if (GetParentContext(top) -> cn_ID == ID_FORM)  */
	      if (top -> cn_ID == ID_FORM)  
		{
		  result = ProcessTXTR(iff, tex);
		  if (result <0)
		    {
		      *foundM2 = FALSE;
		      fprintf(stderr, "Possible bad IFF in file form, going to the next one\n");
		    }
		  else
		    {
		      tex->Wrapper.FileSize = top->cn_Size;
		      *foundM2 = TRUE;
		      return(result);
		    }
		}
	    }
	  result = 0;
	  continue;
	}
      /*
       * Other errors are real and possibily nasty errors.
       */
      if ((result<0)&&(result != IFF_PARSE_EOF))
	{
	  return(result);
	}
      else if (result == IFF_PARSE_EOF)
	{
	  break;
	}
    }
  return(result);
}

M2Err M2TX_ReadIFF(M2TX *tex, bool inLOD, char *fileName, bool isMac, 
		   void *spec)
{
  IFFParser	*iff;
  M2Err         returnErr = M2E_NoErr;
  Err		result;
  bool		foundM2 = FALSE;
  TagArg        tags[2];
  
  /*
   * Create IFF file and open a DOS stream on it.
   */

  returnErr = M2TX_OpenFile(fileName, &iff, FALSE, isMac, spec); 

  if (returnErr != M2E_NoErr)
    return(returnErr);


  result = TXTR_SetupFrame(iff, inLOD);
  if (result>=0)
    {
      /*
       * Declare "frame" (entry and exit) handlers for the UTF format.
       */
      
      result = TXTR_GetNext(iff, tex, &foundM2);
    }

  if (result<0)
    {
      if (result == IFF_PARSE_EOF) 
	{
	  if (foundM2)
	    returnErr = M2E_NoErr;
	  else
	    returnErr = M2E_BadFile;
	} 
      else
	switch(result)
	  {
	  case IFF_ERR_NOMEM: 
	    returnErr = M2E_NoMem;
	    break;
	  case IFF_ERR_BADTAG:
	    returnErr = M2E_Range;
	    break;
	  case IFF_ERR_MANGLED:
	  case IFF_ERR_NOTIFF:
	    returnErr = M2E_BadFile;
	    break;
	  case IFF_ERR_BADPTR:
	  case IFF_ERR_BADLOCATION:
	  default:
	    returnErr = M2E_BadPtr;
	    break;
	  }
    }
  if (iff)
    { 
      result = DeleteIFFParser(iff);
      if ((result<0) && (returnErr != M2E_NoErr))
	returnErr = M2E_Limit;
    }      
  return(returnErr);
}


/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TX_ReadFile
|||	Read an IFF UTF file from disk into an M2TX structure.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_ReadFile(char *fileName, M2TX *tex);
|||	
|||	  Description
|||	
|||	    This function reads an IFF UTF texture into an M2TX structure. The texture is
|||	    read from a disk file specified by the file name.
|||	
|||	  Arguments
|||	    
|||	      tex
|||	          The input M2TX texture.
|||	    fileName
|||	        The name of the file to open.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXiff.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_WriteMacFile()
**/

M2Err M2TX_ReadFile(char *fileName, M2TX *tex)
{
  FILE *texFPtr;
  uint32 temp32;
  uint32 WrapperChunkType = 'TXTR';

  texFPtr = qOpenReadFile(fileName);
  if (texFPtr == NULL)
    return (M2E_NoFile);
  qReadByteStream(texFPtr,(BYTE *)&temp32, 4);
  qCloseReadFile(texFPtr);
  if (temp32 == WrapperChunkType)
    return(M2TX_ReadFileOld(fileName, tex));
  else
    return(M2TX_ReadIFF(tex, TRUE, fileName, FALSE, NULL));
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TX_WriteFile
|||	Write an IFF UTF to disk from an M2TX structure.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_WriteFile(char *fileName, M2TX *tex);
|||	
|||	  Description
|||	
|||	    This function writes an IFF UTF texture from an M2TX structure. The texture
|||	    is written into the file specified by the file name.
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	    fileName
|||	        The name of the file to open.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXiff.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_ReadFile()
**/

M2Err M2TX_WriteFile(char *fileName, M2TX *tex)
{
#ifdef applec
  Str255  appleName;
  FSSpec  outFile;
  FSSpec  *writeFile = &outFile;
  OSErr   macErr;

  appleName[0] = strlen(fileName);
  strcpy((char *)&(appleName[1]), fileName);
  macErr = FSMakeFSSpec(0, 0, appleName, writeFile);
  macErr = FSpCreate(writeFile,'Rean', '3DOt', smSystemScript);
   if (macErr == dupFNErr)
    {
      ;
    }
   else if (macErr != noErr)
     return (M2E_NoFile);
  return(M2TX_WriteIFF(tex, TRUE, NULL, TRUE, writeFile));
#else
  return(M2TX_WriteIFF(tex, TRUE, fileName, FALSE, NULL));
#endif
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TX_ReadFileNoLODs
|||	Read an IFF UTF file into an M2TX structure except for the levels of detail.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_ReadFileNoLODs(char *fileName, M2TX *tex);
|||	
|||	  Description
|||	
|||	    This function reads an IFF UTF texture into an M2TX structure. The texture is
|||	    read from a disk file specified by the file name.  All the data is read
|||	    with the exception of the levels of detail (the actual pixel data).
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	    fileName
|||	        The name of the file to open.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXiff.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_ReadFile()
**/

M2Err M2TX_ReadFileNoLODs(char *fileName, M2TX *tex)
{
  FILE *texFPtr;
  uint32 temp32;
  uint32 WrapperChunkType = 'TXTR';
  
  texFPtr = qOpenReadFile(fileName);
  if (texFPtr == NULL)
    return (M2E_NoFile);
  qReadByteStream(texFPtr,(BYTE *)&temp32, 4);
  qCloseReadFile(texFPtr);
  if (temp32 == WrapperChunkType)
    return(M2TX_ReadFileNoLODsOld(fileName, tex));
  else
    return(M2TX_ReadIFF(tex, FALSE, fileName, FALSE, NULL));
}


#ifdef applec

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TX_ReadMacFile
|||	Read an IFF UTF texture from a Mac file.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_ReadMacFile(const FSSpec *spec, M2TX *tex)
|||	
|||	  Description
|||	
|||	    This function reads an IFF UTF texture into an M2TX structure. The texture is
|||	    read from a disk file specified by the FSSpec.  This function is only available 
|||	    on the Macintosh implementation of the library.
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	    spec
|||	        The FSSpec of a mac file whose data fork has the UTF file.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXiff.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_WriteMacFile()
**/

M2Err M2TX_ReadMacFile(const FSSpec *spec, M2TX *tex)
{
  uint32 temp32;
  int32 count;
  short refNum;
  uint32 WrapperChunkType = 'TXTR';
  OSErr macErr;

  macErr = FSpOpenDF(spec, 'r', &refNum);	
  if (macErr != noErr)
    return (M2E_NoFile);
  count = 4;		
  FSRead(refNum,&count,&temp32);
  FSClose(refNum);
    if (temp32 == WrapperChunkType)
      return(M2TX_ReadMacFileOld(spec, tex));
    else
      return(M2TX_ReadIFF(tex, TRUE, NULL, TRUE, (void *)spec));  
}
#endif


#ifdef applec

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TX_WriteMacFile
|||	Write an IFF UTF to a Mac file.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_WriteMacFile(const FSSpec *spec, M2TX *tex);
|||	
|||	  Description
|||	
|||	    This functions writes an IFF UTF texture from an M2TX structure. The 
|||	    texture is written into the file specified by the FSSpec.  This is 
|||	    only available on the Macintosh implementation of the library.
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	    spec
|||	        The FSSpec of a mac file whose data fork has the UTF file.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXiff.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_ReadMacFile()
**/

M2Err M2TX_WriteMacFile(const FSSpec *spec, M2TX *tex)
{
  OSErr macErr;
 
  macErr = FSpCreate(spec,'Rean', '3DOt', smSystemScript);
  if (macErr == dupFNErr)
    {
      macErr = FSpOpenDF(spec, fsWrPerm, 0);
      if (macErr != noErr)
        return (M2E_NoFile);
    }
  else if (macErr != noErr)
    return (M2E_NoFile);
 
  return(M2TX_WriteIFF(tex, TRUE, NULL, TRUE, (void *)spec));
}

#endif

#if 0

#define	ID_M2AN	 MAKE_ID('M','2','A','N')
#define	ID_FRM MAKE_ID('F','R','M','{')
#define	ID_SEQ MAKE_ID('S','E','Q','{')
#define	ID_ACTN MAKE_ID('A','C','T','N')

static IFFTypeID animprops[] =
 {
   ID_M2AN, ID_ACTN,
   ID_M2AN, ID_M2TX,
   ID_M2AN, ID_M2PI,
   ID_M2AN, ID_M2CI,
   ID_M2AN, ID_M2TD,
   ID_M2AN, ID_M2TA,
   ID_M2AN, ID_M2LR,
   ID_M2AN, ID_M2DB,
   ID_M2AN, ID_M2PG,
0
};




#endif
