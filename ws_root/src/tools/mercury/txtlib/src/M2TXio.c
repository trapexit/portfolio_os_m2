/*
	File:		M2TXio.c

	Contains:	M2 Texture Library, i/o functions 

	Written by:	Todd Allendorf 

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		<9+>	 9/20/95	TMA		Fix null dereferencing bug in M2TX_CloneTXTR, M2TX_CopyTXTR.
		<8+>	  8/4/95	TMA		Fixed compiler warnings.
		 <8>	  8/4/95	TMA		Updated M2TX_(Clone|Copy|Write)TXTR to match the new data
									formats
		 <7>	 7/15/95	TMA		Autodocs updated.
		 <5>	 7/11/95	TMA		Change Mac I/O functions to match new prototypes.
		 <3>	 7/11/95	TMA		Removed mallocs.  Change old format functions names to end with
									"Old".
		 <2>	 5/16/95	TMA		Autodocs added.
		 <4>	 3/31/95	TMA		Added M2TX_WriteTXTR function. Fixed pointer arithmetric in
									M2TX_CloneTXTR and M2TX_CopyTXTR.
		 <3>	 3/26/95	TMA		Added Mac FS and memory I/O calls
		<1+>	 1/16/95	TMA		Added error checking

	To Do:
*/

#ifdef applec

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SEEK_SET
#define SEEK_SET 0
#endif


#include "M2TXTypes.h"
#include "M2TXLibrary.h"
#include "M2TXio.h"

typedef int32   Err;
typedef signed char int8;
#include "clt.h"
#include "clttxdblend.h"

#include "M2TXHeader.h"
#include "qstream.h"
#include "qmem.h"

#include "M2TXinternal.h"
#include "M2TXattr.h"



M2Err   M2TX_SetDefaultLoadRect(M2TX *tex);

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

/**
|||	AUTODOC -private -class tools -group m2txlib -name M2TX_CloneTXTR
|||	Copy a UTF textures pointers into an M2TX structure.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_CloneTXTR(char *texPtr, M2TX *tex);
|||	
|||	  Description
|||	
|||	    This functions takes a UTF structure in memory and copies the 
|||	    information into an M2TX structure for manipulation by the library.  
|||	    Only the data pointers to the levels of detail are copies, not the data
|||	    itself.
|||	
|||	  Caveats
|||	
|||	    If some other program frees up the levels of detail, the cloned texture
|||	    may no longer be valid.  Similarly, if user of the library frees up a 
|||	    level of details, the pointers in the original UTF texture in memory 
|||	    will no longer be valid.
|||	
|||	  Arguments
|||	
|||	    tex
|||	        The input M2TX texture.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXio.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_CopyTXTR()
**/

M2Err M2TX_CloneTXTR(TexBlend *texBlend, M2TX *tex)
{
	int i;
  	CltTxAttribute txAttr;
  	CltDblAttribute dblAttr;
  	uint32 value;
	uint8 numLOD;
	int numColors;
	uint32 size;
  TexData *texData;
  M2TXPIP *pip;
  PipTable *pipTable;
  M2TXColor *pipData;
  Tx_Data *texPtr;
  M2TXRect rect;
  Tx_LoadControlBlock *lcb;

  M2TX_Init(tex);
  
  texData = texBlend->m_Tex;
  if (texData != NULL)
  {
	  texPtr = &(texData->m_Tex);
  	if (texPtr != NULL)
   	 {
		tex->Header.Flags = texPtr->flags;
 	 	tex->Header.MinXSize = (uint16)(0x0000FFFF & texPtr->minX);
  		tex->Header.MinYSize = (uint16)(0x0000FFFF & texPtr->minY);
  		tex->Header.TexFormat = (uint16)(0x0000FFFF & texPtr->expansionFormat);
  		tex->Header.NumLOD = texPtr->maxLOD;
  		M2TXHeader_GetNumLOD(&(tex->Header), &numLOD);

	  	if (texPtr->dci != NULL)
   	 	{	
   		   for (i=0; i<4; i++)
			tex->DCI.TexelFormat[i] = texPtr->dci->texelFormat[i];
   	 	  for (i=0; i<4; i++)
			tex->DCI.TxExpColorConst[i] = texPtr->dci->expColor[i];
   		 }
 	 
 	 	for(i=0; i < numLOD; i++)
   		{
	   	   	M2TXHeader_SetLODPtr(&(tex->Header),i, 
				   texPtr->texelData[i].texelDataSize, 
				   texPtr->texelData[i].texelData);
    	}
    }
  }
  pipTable = texBlend->m_PIP;
  if (pipTable != NULL)
    {
      M2TX_GetPIP(tex, &pip);
      M2TXHeader_SetFHasPIP(&(tex->Header), TRUE);
      numColors =  Pip_GetSize(pipTable);
      M2TXPIP_SetNumColors(pip, numColors);
      M2TXPIP_SetIndexOffset(pip, Pip_GetIndex(pipTable));
      pipData = (M2TXColor *)Pip_GetData(pipTable);
      for (i=0; i<numColors; i++)
		tex->PIP.PIPData[i] = pipData[i];	
    }

  /* Handle the Load Rect */  
  lcb = (Tx_LoadControlBlock *)texBlend->m_LCB;
  if (lcb != NULL)
    {
      rect = tex->LoadRects.LRData[0];
      rect.XWrapMode = lcb->XWrap;
      rect.YWrapMode = lcb->YWrap;
      rect.FirstLOD = lcb->firstLOD;
      rect.NLOD = lcb->numLOD;
      rect.XSize = lcb->XSize;
      rect.YSize = lcb->YSize;
      rect.XOff = lcb->XOffset;
      rect.YOff = lcb->YOffset;
    }

  /* Handle the TAB */
  size = texBlend->m_TAB.size;
  if (size != 0)
    {
      txAttr=TXA_MinFilter;
      while (txAttr!=TXA_NoMore)
	{
	  if ((CLT_GetTxAttribute(&(texBlend->m_TAB), txAttr, &value))==0)
	    M2TXTA_SetAttr(&(tex->TexAttr), txAttr, value);
	 txAttr++;
	}
   }

  /* Handle the DAB */
  size = texBlend->m_DAB.size;
  if (size != 0)
  {
      dblAttr=DBLA_EnableAttrs;
      while (dblAttr!=DBLA_NoMore)
	{
	  if ((CLT_GetDblAttribute(&(texBlend->m_DAB), dblAttr, &value))==0)
	    M2TXDB_SetAttr(&(tex->DBl), dblAttr, value);
	 dblAttr++;
	}
  }
 return (M2E_NoErr);						
}

/**
|||	AUTODOC -private -class tools -group m2txlib -name M2TX_CopyTXTR
|||	Copy a UTF texture in memory to an M2TX structure.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_CopyTXTR(char *texPtr, M2TX *tex)
|||	
|||	  Description
|||	
|||	    This functions takes a UTF structure in memory and copies the
|||	    information into an M2TX structure for manipulation by the library.  A 
|||	    local copy of all the data is made and can be manipulated freely.
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXio.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_CloneTXTR()
**/

M2Err M2TX_CopyTXTR(TexBlend *texBlend, M2TX *tex)
{
	int i;
	uint8 numLOD;
	int numColors;
	uint32 size;
   	CltTxAttribute txAttr;
  	CltDblAttribute dblAttr;
  	uint32 value;
	M2TXTex texelPtr;
  TexData *texData;
  M2TXPIP *pip;
  PipTable *pipTable;
  M2TXColor *pipData;
  Tx_Data *texPtr;
  M2Err myErr;
  M2TXRect rect;
  Tx_LoadControlBlock *lcb;
  
  
  M2TX_Init(tex);
  
  texData = texBlend->m_Tex;
  if (texData != NULL)
 {
  texPtr = &(texData->m_Tex);
 
  if (texPtr != NULL)
    {
  tex->Header.Flags = texPtr->flags;
  tex->Header.MinXSize = (uint16)(0x0000FFFF & texPtr->minX);
  tex->Header.MinYSize = (uint16)(0x0000FFFF & texPtr->minY);
  tex->Header.TexFormat = (uint16)(0x0000FFFF & texPtr->expansionFormat);
  tex->Header.NumLOD = texPtr->maxLOD;
  M2TXHeader_GetNumLOD(&(tex->Header), &numLOD);

  if (texPtr->dci != NULL)
    {	
      for (i=0; i<4; i++)
	tex->DCI.TexelFormat[i] = texPtr->dci->texelFormat[i];
      for (i=0; i<4; i++)
	tex->DCI.TxExpColorConst[i] = texPtr->dci->expColor[i];
    }
  
  for(i=0; i < numLOD; i++)
    {
      size = texPtr->texelData[i].texelDataSize;
      texelPtr = (M2TXTex)qMemNewPtr(size);   
      if (texelPtr == NULL)
	return (M2E_NoMem);			 
      BeginMemGetBytes((BYTE *)(texPtr->texelData[i].texelData));
      qMemGetBytes((BYTE *)texelPtr, size);
      myErr = M2TXHeader_SetLODPtr(&(tex->Header), i, size, texelPtr);
    }
  EndMemGetBytes();

    }
  }
  pipTable = texBlend->m_PIP;
  if (pipTable != NULL)
    {
      M2TX_GetPIP(tex, &pip);
      M2TXHeader_SetFHasPIP(&(tex->Header), TRUE);
      numColors =  Pip_GetSize(pipTable);
      M2TXPIP_SetNumColors(pip, numColors);
      M2TXPIP_SetIndexOffset(pip, Pip_GetIndex(pipTable));
      pipData = (M2TXColor *)Pip_GetData(pipTable);
      for (i=0; i<numColors; i++)
		tex->PIP.PIPData[i] = pipData[i];	
    }


  /* Handle the Load Rects - we can't, I think*/
  lcb = (Tx_LoadControlBlock *)texBlend->m_LCB;
  if (lcb != NULL)
    {
      rect = tex->LoadRects.LRData[0];
      rect.XWrapMode = lcb->XWrap;
      rect.YWrapMode = lcb->YWrap;
      rect.FirstLOD = lcb->firstLOD;
      rect.NLOD = lcb->numLOD;
      rect.XSize = lcb->XSize;
      rect.YSize = lcb->YSize;
      rect.XOff = lcb->XOffset;
      rect.YOff = lcb->YOffset;
    }

  /* Handle the TAB */
  size = texBlend->m_TAB.size;
  if (size != 0)
    {
      txAttr=TXA_MinFilter;
      while (txAttr!=TXA_NoMore)
		{
		  if ((CLT_GetTxAttribute(&(texBlend->m_TAB), txAttr, &value))==0)
		    M2TXTA_SetAttr(&(tex->TexAttr), txAttr, value);
		txAttr++;
		}
    }
 
  /* Handle the DAB */
  size = texBlend->m_DAB.size;
  if (size != 0)
    {
      dblAttr=DBLA_EnableAttrs;
      while (dblAttr!=DBLA_NoMore)
		{
		  if ((CLT_GetDblAttribute(&(texBlend->m_DAB), dblAttr, &value))==0)
		    M2TXDB_SetAttr(&(tex->DBl), dblAttr, value);
	   	  dblAttr++;
		}
    }
	return (M2E_NoErr);						
}

/**
|||	Autodoc -private -class tools -group m2txlib -name M2TX_ReadFileOld
|||	Read an old UTF file from disk into an M2TX structure.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_ReadFileOld(char *fileName, M2TX *tex);
|||	
|||	  Description
|||	
|||	    This function reads a UTF texture into an M2TX structure. The texture is
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
|||	    <M2TXio.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_WriteMacFileOld()
**/

M2Err M2TX_ReadFileOld(char *fileName, M2TX *tex)
{
	uint32 WrapperChunkType = 'TXTR';
	uint32 HeaderChunkType = 'M2TX';
	uint32 PIPChunkType = 'M2PI';
	uint32 DCIChunkType = 'M2CI';
	uint32 TexelChunkType = 'M2TD';

	FILE *texFPtr;
	int i;
	uint8 numLOD, cDepth;
	int numColors;
	uint32 offset, nextOff, size;
	bool flag;
	M2TXTex texelPtr;
	
	texFPtr = qOpenReadFile(fileName);
	if (texFPtr == NULL)
		return (M2E_NoFile);
	qReadByteStream(texFPtr,(BYTE *)&(tex->Wrapper.Signature), 4);
	if (tex->Wrapper.Signature != WrapperChunkType)
		return (M2E_BadFile);
	tex->Wrapper.FileSize = readLong(texFPtr);
	qReadByteStream(texFPtr,(BYTE *)&(tex->Header.Signature), 4);
	if (tex->Header.Signature != HeaderChunkType)
		return (M2E_NoFile);
	tex->Header.Size = readLong(texFPtr);
	tex->Header.Version = readLong(texFPtr);
	tex->Header.Flags = readLong(texFPtr);
	tex->Header.MinXSize = readShort(texFPtr);
	tex->Header.MinYSize = readShort(texFPtr);
	tex->Header.TexFormat = readShort(texFPtr);
	qReadByteStream(texFPtr,(BYTE *)&(tex->Header.NumLOD), 1);
	qReadByteStream(texFPtr,(BYTE *)&(tex->Header.Border), 1);
	tex->Header.DCIOffset = readLong(texFPtr);
	tex->Header.PIPOffset = readLong(texFPtr);
	M2TXHeader_GetNumLOD(&(tex->Header), &numLOD);
	for(i=0; i < numLOD; i++)
	{
		tex->Header.LODDataOffset[i] = readLong(texFPtr);
	}
	M2TXHeader_GetFHasColorConst(&(tex->Header), &flag);
	if (flag)
	{
		tex->Header.ColorConst[0] = readLong(texFPtr);
		tex->Header.ColorConst[1] = readLong(texFPtr);
	}
	M2TXHeader_GetFHasPIP(&(tex->Header), &flag);
	if (flag)
	{
		M2TXHeader_GetPIPOffset(&(tex->Header), &offset);
		fseek(texFPtr, offset, SEEK_SET);
		qReadByteStream(texFPtr,(BYTE *)&(tex->PIP.Signature), 4);
		tex->PIP.Size = readLong(texFPtr);
		M2TXHeader_GetCDepth(&(tex->Header),&cDepth);
		numColors = 1 << cDepth;
		if (cDepth>0)
			for (i=0; i<numColors; i++)
				tex->PIP.PIPData[i] = readLong(texFPtr);	
	}
	M2TXHeader_GetDCIOffset(&(tex->Header), &offset);
	if (offset != 0)
	{	
		fseek(texFPtr, offset, SEEK_SET);
		qReadByteStream(texFPtr,(BYTE *)&(tex->DCI.Signature), 4);
		tex->DCI.Size = readLong(texFPtr);
		for (i=0; i<4; i++)
			tex->DCI.TexelFormat[i] = readShort(texFPtr);
		for (i=0; i<4; i++)
			tex->DCI.TxExpColorConst[i] = readLong(texFPtr);
	}
	M2TXHeader_GetLODOffset(&(tex->Header), 0, &offset);
	fseek(texFPtr, offset-8, SEEK_SET);
	qReadByteStream(texFPtr,(BYTE *)&(tex->Texel.Signature), 4);
	if (tex->Texel.Signature != TexelChunkType)
		return (M2E_BadFile);
	tex->Texel.Size = readLong(texFPtr);
	for(i=0; i < numLOD; i++)
	{
		M2TXHeader_GetLODOffset(&(tex->Header), i, &offset);
		if (i< (numLOD-1))
			M2TXHeader_GetLODOffset(&(tex->Header), i+1, &nextOff);		
		else
			nextOff = tex->Wrapper.FileSize;
		size = nextOff-offset;
		texelPtr = (M2TXTex)qMemNewPtr(size);   
		if (texelPtr == NULL)
			return (M2E_NoMem);				
		fseek(texFPtr, offset, SEEK_SET);
		qReadByteStream(texFPtr,(BYTE *)texelPtr, size);
		M2TXHeader_SetLODPtr(&(tex->Header),i, size, texelPtr);
	}
	qCloseReadFile(texFPtr);
	return (M2E_NoErr);						
}

#ifdef applec

/**
|||	AUTODOC -private -class tools -group m2txlib -name M2TX_ReadMacFileOld
|||	Reads an Old UTF texture from a Mac file.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_ReadMacFileOld(const FSSpec *spec, M2TX *tex)
|||	
|||	  Description
|||	
|||	    This function reads a UTF texture into an M2TX structure. The texture is
|||	    read from a disk file specified by the FSSpec.  This is only available 
|||	    on the Macintosh implementation of the library.
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	    spec
|||	        The FSSpec of a mac file whose data fork has the old UTF file.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXio.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_WriteMacFileOld()
**/

M2Err M2TX_ReadMacFileOld(const FSSpec *spec, M2TX *tex)
{
	uint32 WrapperChunkType = 'TXTR';
	uint32 HeaderChunkType = 'M2TX';
	uint32 PIPChunkType = 'M2PI';
	uint32 DCIChunkType = 'M2CI';
	uint32 TexelChunkType = 'M2TD';
	int i;
	uint8 numLOD, cDepth;
	int numColors;
	uint32 offset, nextOff, size;
	bool flag;
	M2TXTex texelPtr;
	short refNum;
	long count, lSize;
	OSErr macErr;

	
	macErr = FSpOpenDF(spec, 'r', &refNum);	
	if (macErr != noErr)
		return (M2E_NoFile);

	count = 4;		
	FSRead(refNum,&count,&(tex->Wrapper.Signature));
	if (tex->Wrapper.Signature != WrapperChunkType)
		return (M2E_BadFile);
	FSRead(refNum,&count,&(tex->Wrapper.FileSize));
	FSRead(refNum,&count,&(tex->Header.Signature));
	if (tex->Header.Signature != HeaderChunkType)
		return (M2E_NoFile);
	FSRead(refNum,&count,&(tex->Header.Size));
	FSRead(refNum,&count,&(tex->Header.Version));
	FSRead(refNum,&count,&(tex->Header.Flags));
	count = 2;
	FSRead(refNum,&count,&(tex->Header.MinXSize));
	FSRead(refNum,&count,&(tex->Header.MinYSize));
	FSRead(refNum,&count,&(tex->Header.TexFormat));
	count = 1;
	FSRead(refNum,&count,&(tex->Header.NumLOD));
	FSRead(refNum,&count,&(tex->Header.Border));
	count = 4;
	FSRead(refNum,&count,&(tex->Header.DCIOffset));
	FSRead(refNum,&count,&(tex->Header.PIPOffset));
	M2TXHeader_GetNumLOD(&(tex->Header), &numLOD);
	for(i=0; i < numLOD; i++)
	{
		FSRead(refNum,&count,&(tex->Header.LODDataOffset[i]));
	}
	M2TXHeader_GetFHasColorConst(&(tex->Header), &flag);
	if (flag)
	{
		FSRead(refNum,&count,&(tex->Header.ColorConst[0]));
		FSRead(refNum,&count,&(tex->Header.ColorConst[1]));
	}
	M2TXHeader_GetFHasPIP(&(tex->Header), &flag);
	if (flag)
	{
		M2TXHeader_GetPIPOffset(&(tex->Header), &offset);
		SetFPos(refNum, fsFromStart, offset);
		FSRead(refNum,&count,&(tex->PIP.Signature));
		FSRead(refNum,&count,&(tex->PIP.Size));
		M2TXHeader_GetCDepth(&(tex->Header),&cDepth);
		numColors = 1 << cDepth;
		if (cDepth>0)
			for (i=0; i<numColors; i++)
				FSRead(refNum,&count,&(tex->PIP.PIPData[i]));	
	}
	M2TXHeader_GetDCIOffset(&(tex->Header), &offset);
	if (offset != 0)
	{	
		SetFPos(refNum, fsFromStart, offset);
		FSRead(refNum,&count,&(tex->DCI.Signature));
		FSRead(refNum,&count,&(tex->DCI.Size));
		count = 2;
		for (i=0; i<4; i++)
			FSRead(refNum,&count,&(tex->DCI.TexelFormat[i]));
		count = 4;
		for (i=0; i<4; i++)
			FSRead(refNum,&count,&(tex->DCI.TxExpColorConst[i]));
	}
	M2TXHeader_GetLODOffset(&(tex->Header), 0, &offset);
	SetFPos(refNum, fsFromStart, offset-8);
	FSRead(refNum,&count,&(tex->Texel.Signature));
	if (tex->Texel.Signature != TexelChunkType)
		return (M2E_BadFile);
	FSRead(refNum,&count,&(tex->Texel.Size));
	for(i=0; i < numLOD; i++)
	{
		M2TXHeader_GetLODOffset(&(tex->Header), i, &offset);
		if (i< (numLOD-1))
			M2TXHeader_GetLODOffset(&(tex->Header), i+1, &nextOff);		
		else
			nextOff = tex->Wrapper.FileSize;
		size = nextOff-offset;
		texelPtr = (M2TXTex)qMemNewPtr(size);   
		if (texelPtr == NULL)
			return (M2E_NoMem);		
		SetFPos(refNum, fsFromStart, offset);
		lSize = size;
		FSRead(refNum,&lSize,texelPtr);
		M2TXHeader_SetLODPtr(&(tex->Header),i, size, texelPtr);
	}
	FSClose(refNum);
	return (M2E_NoErr);						
}
#endif

/**
|||	AUTODOC -private -class tools -group m2txlib -name M2TX_ReadFileNoLODsOld
|||	Reads an old UTF file into an M2TX structure except for the levels of detail.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_ReadFileNoLODsOld(char *fileName, M2TX *tex);
|||	
|||	  Description
|||	
|||	    This function reads a UTF texture into an M2TX structure. The texture is
|||	    read from a disk file specified by the file name.  All the data is read
|||	    with the exception of the levels of detail (the actual pixel data).
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXio.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_ReadFileOld()
**/

M2Err M2TX_ReadFileNoLODsOld(char *fileName, M2TX *tex)
{
	uint32 WrapperChunkType = 'TXTR';
	uint32 HeaderChunkType = 'M2TX';
	uint32 PIPChunkType = 'M2PI';
	uint32 DCIChunkType = 'M2CI';
	uint32 TexelChunkType = 'M2TD';

	FILE *texFPtr;
	int i;
	uint8 numLOD, cDepth;
	int numColors;
	uint32 offset;
	bool flag;
	
	texFPtr = qOpenReadFile(fileName);
	if (texFPtr == NULL)
		return (M2E_NoFile);
	qReadByteStream(texFPtr,(BYTE *)&(tex->Wrapper.Signature), 4);
	if (tex->Wrapper.Signature != WrapperChunkType)
		return (M2E_BadFile);
	tex->Wrapper.FileSize = readLong(texFPtr);
	qReadByteStream(texFPtr,(BYTE *)&(tex->Header.Signature), 4);
	if (tex->Header.Signature != HeaderChunkType)
		return (M2E_BadFile);
	tex->Header.Size = readLong(texFPtr);
	tex->Header.Version = readLong(texFPtr);
	tex->Header.Flags = readLong(texFPtr);
	tex->Header.MinXSize = readShort(texFPtr);
	tex->Header.MinYSize = readShort(texFPtr);
	tex->Header.TexFormat = readShort(texFPtr);
	qReadByteStream(texFPtr,(BYTE *)&(tex->Header.NumLOD), 1);
	qReadByteStream(texFPtr,(BYTE *)&(tex->Header.Border), 1);
	tex->Header.DCIOffset = readLong(texFPtr);
	tex->Header.PIPOffset = readLong(texFPtr);
	M2TXHeader_GetNumLOD(&(tex->Header), &numLOD);
	for(i=0; i < numLOD; i++)
	{
		tex->Header.LODDataOffset[i] = readLong(texFPtr);
	}
	M2TXHeader_GetFHasColorConst(&(tex->Header), &flag);
	if (flag)
	{
		tex->Header.ColorConst[0] = readLong(texFPtr);
		tex->Header.ColorConst[1] = readLong(texFPtr);
	}
	M2TXHeader_GetFHasPIP(&(tex->Header), &flag);
	if (flag)
	{
		M2TXHeader_GetPIPOffset(&(tex->Header), &offset);
		fseek(texFPtr, offset, SEEK_SET);
		qReadByteStream(texFPtr,(BYTE *)&(tex->PIP.Signature), 4);
		tex->PIP.Size = readLong(texFPtr);
		M2TXHeader_GetCDepth(&(tex->Header),&cDepth);
		numColors = 1 << cDepth;
		if (cDepth>0)
			for (i=0; i<numColors; i++)
				tex->PIP.PIPData[i] = readLong(texFPtr);
	}
	M2TXHeader_GetDCIOffset(&(tex->Header), &offset);
	if (offset != 0)
	{	
		fseek(texFPtr, offset, SEEK_SET);
		qReadByteStream(texFPtr,(BYTE *)&(tex->DCI.Signature), 4);
		tex->DCI.Size = readLong(texFPtr);
		for (i=0; i<4; i++)
			tex->DCI.TexelFormat[i] = readShort(texFPtr);
		for (i=0; i<4; i++)
			tex->DCI.TxExpColorConst[i] = readLong(texFPtr);
	}
	M2TXHeader_GetLODOffset(&(tex->Header), 0, &offset);
	fseek(texFPtr, offset-8, SEEK_SET);
	qReadByteStream(texFPtr,(BYTE *)&(tex->Texel.Signature), 4);
	if (tex->Texel.Signature != TexelChunkType)
		return (M2E_BadFile);
	tex->Texel.Size = readLong(texFPtr);
	qCloseReadFile(texFPtr);
	return (M2E_NoErr);						
}


static M2Err Header_ComputeSize(M2TXHeader *header)
{
	uint8 numLOD;
	bool flag;
	M2TXHeader_GetNumLOD(header, &numLOD);

	M2TXHeader_GetFHasColorConst(header, &flag);
	if (flag)
		header->Size = 40 + numLOD*4;
	else
		header->Size = 32 + numLOD*4;		
	return(M2E_NoErr);
}

static M2Err PIP_ComputeSize(M2TX *tex)
{
	uint8 cDepth;
	int numColors;
	
	M2TXHeader_GetCDepth(&(tex->Header),&cDepth);
	numColors = 1 << cDepth;
	if (cDepth>0)
		tex->PIP.Size = 8 +numColors*4;
	else
		tex->PIP.Size = 8;
	return(M2E_NoErr);
}

static M2Err DCI_ComputeSize(M2TX *tex)
{	
	tex->DCI.Size = 32;
	return(M2E_NoErr);
}

static M2Err Texel_ComputeSize(M2TX *tex)
{	
	uint8 numLOD,i;
	uint32	length;
	
	M2TXHeader_GetNumLOD(&(tex->Header), &numLOD);
	length=0;
	
	for(i=0; i < numLOD; i++)
		length += tex->Header.LODDataLength[i];
	tex->Texel.Size = 8 +length;
	return(M2E_NoErr);
}

static M2Err Header_ComputePIPOffset(M2TXHeader *header)
{
	bool flag;
	
	M2TXHeader_GetFHasPIP(header, &flag);
	if (flag)
		header->PIPOffset=header->Size+8;
	else
		header->PIPOffset=0;
		
	return(M2E_NoErr);
}

static M2Err Header_ComputeDCIOffset(M2TX *tex)
{
	bool flag;
	
	M2TXHeader_GetFIsCompressed(&(tex->Header), &flag);  /* 1.06 Change this! It's wrong! */
	if (flag)
	{	
		if (tex->Header.PIPOffset==0)
			tex->Header.DCIOffset=tex->Header.Size+8;
		else
			tex->Header.DCIOffset=tex->Header.PIPOffset+tex->PIP.Size;
	}
	else
		tex->Header.DCIOffset=0;

	return(M2E_NoErr);
}

static M2Err Header_ComputeLODOffset(M2TX *tex)
{
	uint32 startOff;
	uint8 numLOD,i;
	
	if (tex->Header.DCIOffset==0)
	{
		if (tex->Header.PIPOffset==0)
			startOff = tex->Header.Size+8;
		else 
			startOff = tex->Header.PIPOffset+tex->PIP.Size;
	}
	else
		startOff = tex->Header.DCIOffset + tex->DCI.Size;

	M2TXHeader_GetNumLOD(&(tex->Header), &numLOD);
	
	startOff +=8;
	for(i=0; i < numLOD; i++)
	{
		tex->Header.LODDataOffset[i] = startOff;
		startOff += tex->Header.LODDataLength[i];
	}

	return(M2E_NoErr);
}

static M2Err Wrapper_ComputeFileSize(M2TX *tex)
{

	tex->Wrapper.FileSize = tex->Header.LODDataOffset[0] + tex->Texel.Size - 8;

	return(M2E_NoErr);
}

static M2Err M2TX_ComputeSizesAndOffsets(M2TX *tex)
{
	Header_ComputeSize(&(tex->Header));
	PIP_ComputeSize(tex);
	DCI_ComputeSize(tex);
	Texel_ComputeSize(tex);
	Header_ComputePIPOffset(&(tex->Header));
	Header_ComputeDCIOffset(tex);
	Header_ComputeLODOffset(tex);
	Wrapper_ComputeFileSize(tex);
	return (M2E_NoErr);
}

#ifdef applec

/**
|||	AUTODOC -private -class tools -group m2txlib -name M2TX_WriteMacFileOld
|||	Writes a UTF to a Mac file.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_WriteMacFileOld(const FSSpec *spec, M2TX *tex);
|||	
|||	  Description
|||	
|||	    This functions writes a UTF texture from an M2TX structure. The 
|||	    texture is written into the file specified by the FSSpec.  This is 
|||	    only available on the Macintosh implementation of the library.
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXio.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_ReadMacFileOld()
**/

M2Err M2TX_WriteMacFileOld(const FSSpec *spec, M2TX *tex)
{
	int i;
	uint8 numLOD, cDepth;
	int numColors;
	uint32 offset, nextOff, size2,size;
	bool flag;
	OSErr macErr;
	M2TXTex texelPtr;
	short refNum;
	long count, lSize;

/*
	FILE *texFPtr;
	char *dirPtr;
	char tempName[1024];
	char dirName[1024];
	StringPtr dirString;
	uint8 strLen;
	char tmpPath[1024] = "";
	char fullPath[1024] = "";
	long parID;
*/	
	M2TX_ComputeSizesAndOffsets(tex);
	macErr = FSpCreate(spec,'Rean', '3DOt', smSystemScript);

/*	parID = spec->parID;
	do
	{
		GetDirName(spec->vRefNum,parID,(StringPtr)tempName);
		GetParentID(spec->vRefNum, parID, NULL,&parID);
		strLen = tempName[0];
		dirPtr = (char *)&(tempName[1]);
		strncpy(dirName, dirPtr, strLen);
		strcat(dirName,":");
		strcat(dirName,fullPath);
		strcpy(fullPath, dirName);
	} while (parID != fsRtDirID);
	
	strLen = spec->name[0];
	dirPtr = (char *)&(spec->name[1]);
	strncpy(dirName, dirPtr, strLen);
	strcat(fullPath,dirName);
*/
	macErr = FSpOpenDF(spec, 'w', &refNum);	
	count = 4;
	FSWrite(refNum,&count,&(tex->Wrapper.Signature));
	FSWrite(refNum,&count,&(tex->Wrapper.FileSize));
	FSWrite(refNum,&count,&(tex->Header.Signature));
	FSWrite(refNum,&count,&(tex->Header.Size));
	FSWrite(refNum,&count,(BYTE *)&(tex->Header.Version));
	FSWrite(refNum,&count,&(tex->Header.Flags));
	count = 2;
	FSWrite(refNum,&count,&(tex->Header.MinXSize));
	FSWrite(refNum,&count,&(tex->Header.MinYSize));
	FSWrite(refNum,&count,&(tex->Header.TexFormat));
	count = 1;
	FSWrite(refNum,&count,&(tex->Header.NumLOD));
	FSWrite(refNum,&count,&(tex->Header.Border));
	count = 4;
	FSWrite(refNum,&count,&(tex->Header.DCIOffset));
	FSWrite(refNum,&count,&(tex->Header.PIPOffset));
	M2TXHeader_GetNumLOD(&(tex->Header), &numLOD);
	for(i=0; i < numLOD; i++)
	{
		FSWrite(refNum,&count,&(tex->Header.LODDataOffset[i]));
	}
	M2TXHeader_GetFHasColorConst(&(tex->Header), &flag);
	if (flag)
	{
		FSWrite(refNum,&count,&(tex->Header.ColorConst[0]));
		FSWrite(refNum,&count,&(tex->Header.ColorConst[1]));
	}
	M2TXHeader_GetFHasPIP(&(tex->Header), &flag);
	if (flag)
	{
		M2TXHeader_GetPIPOffset(&(tex->Header), &offset);
		SetFPos(refNum, fsFromStart, offset);
		FSWrite(refNum,&count,&(tex->PIP.Signature));
		FSWrite(refNum,&count,&(tex->PIP.Size));
		M2TXHeader_GetCDepth(&(tex->Header),&cDepth);
		numColors = 1 << cDepth;
		if (cDepth>0)
			for (i=0; i<numColors; i++)
				FSWrite(refNum,&count,&(tex->PIP.PIPData[i]));	
	}
	M2TXHeader_GetDCIOffset(&(tex->Header), &offset);
	if (offset != 0)
	{	
		SetFPos(refNum, fsFromStart, offset);
		FSWrite(refNum,&count,&(tex->DCI.Signature));
		FSWrite(refNum,&count,&(tex->DCI.Size));
		count = 2;
		for (i=0; i<4; i++)
			FSWrite(refNum,&count,&(tex->DCI.TexelFormat[i]));
		count = 4;
		for (i=0; i<4; i++)
			FSWrite(refNum,&count,&(tex->DCI.TxExpColorConst[i]));
	}
	M2TXHeader_GetLODOffset(&(tex->Header), 0, &offset);
	SetFPos(refNum, fsFromStart, offset-8);
	FSWrite(refNum,&count,&(tex->Texel.Signature));
	FSWrite(refNum,&count,&(tex->Texel.Size));
	for(i=0; i < numLOD; i++)
	{
		M2TXHeader_GetLODOffset(&(tex->Header), i, &offset);
		if (i< (numLOD-1))
			M2TXHeader_GetLODOffset(&(tex->Header), i+1, &nextOff);		
		else
			nextOff = tex->Wrapper.FileSize;
		size = nextOff-offset;
		M2TXHeader_GetLODPtr(&(tex->Header),i, &size2, &texelPtr);
		SetFPos(refNum, fsFromStart,offset);
		lSize = size;
		FSWrite(refNum,&lSize,texelPtr);
	}
	FSClose(refNum);
	return (M2E_NoErr);
}

#endif

/**
|||	AUTODOC -private -class tools -group m2txlib -name M2TX_WriteFileOld
|||	Write an old UTF to disk from an M2TX structure.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_WriteFileOld(char *filename, M2TX *tex);
|||	
|||	  Description
|||	
|||	    This functions writes a UTF texture from an M2TX structure. The texture
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
|||	    <M2TXio.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_ReadFileOLD()
**/

M2Err M2TX_WriteFileOld(char *fileName, M2TX *tex)
{
	FILE *texFPtr;
	int i;
	uint8 numLOD, cDepth;
	int numColors;
	uint32 offset, nextOff, size, size2;
	bool flag;
	M2TXTex texelPtr;
	M2TX_ComputeSizesAndOffsets(tex);
	texFPtr = qOpenWriteFile(fileName);
	if (texFPtr == NULL)							
		return (M2E_BadFile);				

	qWriteByteStream(texFPtr,(BYTE *)&(tex->Wrapper.Signature), 4);
	writeLong(texFPtr,tex->Wrapper.FileSize);
	qWriteByteStream(texFPtr,(BYTE *)&(tex->Header.Signature), 4);
	writeLong(texFPtr,tex->Header.Size);
	writeLong(texFPtr,tex->Header.Version);
	writeLong(texFPtr,tex->Header.Flags);
	writeShort(texFPtr,tex->Header.MinXSize);
	writeShort(texFPtr,tex->Header.MinYSize);
	writeShort(texFPtr,tex->Header.TexFormat);
	qWriteByteStream(texFPtr,(BYTE *)&(tex->Header.NumLOD), 1);
	qWriteByteStream(texFPtr,(BYTE *)&(tex->Header.Border), 1);
	writeLong(texFPtr,tex->Header.DCIOffset);
	writeLong(texFPtr,tex->Header.PIPOffset);
	M2TXHeader_GetNumLOD(&(tex->Header), &numLOD);
	for(i=0; i < numLOD; i++)
	{
		writeLong(texFPtr,tex->Header.LODDataOffset[i]);
	}
	M2TXHeader_GetFHasColorConst(&(tex->Header), &flag);
	if (flag)
	{
		writeLong(texFPtr,tex->Header.ColorConst[0]);
		writeLong(texFPtr,tex->Header.ColorConst[1]);
	}
	M2TXHeader_GetFHasPIP(&(tex->Header), &flag);
	if (flag)
	{
		M2TXHeader_GetPIPOffset(&(tex->Header), &offset);
		fseek(texFPtr, offset, SEEK_SET);
		qWriteByteStream(texFPtr,(BYTE *)&(tex->PIP.Signature), 4);
		writeLong(texFPtr,tex->PIP.Size);
		M2TXHeader_GetCDepth(&(tex->Header),&cDepth);
		numColors = 1 << cDepth;
		if (cDepth>0)
			for (i=0; i<numColors; i++)
				writeLong(texFPtr,tex->PIP.PIPData[i]);	
	}
	M2TXHeader_GetDCIOffset(&(tex->Header), &offset);
	if (offset != 0)
	{	
		fseek(texFPtr, offset, SEEK_SET);
		qWriteByteStream(texFPtr,(BYTE *)&(tex->DCI.Signature), 4);
		writeLong(texFPtr,tex->DCI.Size);
		for (i=0; i<4; i++)
			writeShort(texFPtr,tex->DCI.TexelFormat[i]);
		for (i=0; i<4; i++)
			writeLong(texFPtr,tex->DCI.TxExpColorConst[i]);
	}
	M2TXHeader_GetLODOffset(&(tex->Header), 0, &offset);
	fseek(texFPtr, offset-8, SEEK_SET);
	qWriteByteStream(texFPtr,(BYTE *)&(tex->Texel.Signature), 4);
	writeLong(texFPtr,tex->Texel.Size);
	for(i=0; i < numLOD; i++)
	{
		M2TXHeader_GetLODOffset(&(tex->Header), i, &offset);
		if (i< (numLOD-1))
			M2TXHeader_GetLODOffset(&(tex->Header), i+1, &nextOff);		
		else
			nextOff = tex->Wrapper.FileSize;
		size = nextOff-offset;
		M2TXHeader_GetLODPtr(&(tex->Header),i, &size2, &texelPtr);
		fseek(texFPtr, offset, SEEK_SET);
		qWriteByteStream(texFPtr,(BYTE *)texelPtr, size);
	}
	qCloseWriteFile(texFPtr);
	return (M2E_NoErr);
}


/**
|||	AUTODOC -private -class tools -group m2txlib -name M2TX_WriteTXTR
|||	Write a M2TX structure to a UTF structure in memory.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_WriteTXTR(char **TXTRPtr, M2TX *tex)
|||	
|||	  Description
|||	
|||	    This functions takes an M2TX structure and writes a UTF structure into 
|||	    memory and returns a pointer to the newly allocated block of memory.
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXio.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_CopyTXTR(), M2TX_CloneTXTR()
**/

M2Err M2TX_WriteTXTR(Texture *texture, M2TX *tex)
{
  int i;
  uint8 numLOD, cDepth, aDepth, bitDepth;
  uint32 offset, totalSize, length;
  bool flag;
  TexData *texData;
  char *texelData;
  Tx_Data *texPtr;
  M2TXHeader *header;

  M2TX_GetHeader(tex, &header);
  M2TXHeader_GetCDepth(header, &cDepth);
  M2TXHeader_GetADepth(header, &aDepth);
  M2TXHeader_GetFHasSSB(header, &flag);
  if (flag)
    bitDepth = cDepth + aDepth + 1;
  else
    bitDepth = cDepth + aDepth;


  texData = texture;
  texPtr = &(texData->m_Tex);
  texPtr->flags = tex->Header.Flags;
  texPtr->minX = (uint32)(0x0000FFFF & tex->Header.MinXSize);
  texPtr->minY = (uint32)(0x0000FFFF & tex->Header.MinYSize);
  texPtr->expansionFormat = (uint32)(0x0000FFFF & tex->Header.TexFormat);
  numLOD = texPtr->maxLOD = tex->Header.NumLOD;
  texPtr->bitsPerPixel = bitDepth;

  if (texPtr->dci != NULL)
    {
      for (i=0; i<4; i++)
	texPtr->dci->texelFormat[i] = tex->DCI.TexelFormat[i];
      for (i=0; i<4; i++)
	texPtr->dci->expColor[i] = tex->DCI.TxExpColorConst[i];
    }
  texPtr->texelData = (Tx_LOD *)qMemNewPtr(numLOD*sizeof(Tx_LOD));
  totalSize = 0;
  for (i=0; i<numLOD; i++)
    {
      totalSize += tex->Header.LODDataLength[i];
    }
  texelData =  (char *)qMemNewPtr(totalSize);
  offset = 0;
  for(i=0; i < numLOD; i++)
    {
      texPtr->texelData[i].texelData = texelData + offset; 
      length = texPtr->texelData[i].texelDataSize = tex->Header.LODDataLength[i]; 
      memcpy(texPtr->texelData[i].texelData, tex->Header.LODDataPtr[i], length);
      offset += length;
    } 
	return (M2E_NoErr);
}

M2Err M2TXRaw_WriteFile(char *fileName, M2TXRaw *tex)
{
	FILE *texFPtr;
	uint32 i, pixels;

	texFPtr = qOpenWriteFile(fileName);
	if (texFPtr == NULL)							
		return (M2E_BadFile);				
	pixels = tex->XSize * tex->YSize;
	for (i=0; i<pixels; i++)
	{
		qWriteByteStream(texFPtr,(BYTE *)&(tex->Red[i]),1);
		qWriteByteStream(texFPtr,(BYTE *)&(tex->Green[i]),1);
		qWriteByteStream(texFPtr,(BYTE *)&(tex->Blue[i]),1);
	}
	qCloseWriteFile(texFPtr);
	return (M2E_NoErr);
}

M2Err M2TXIndex_WriteFile(char *fileName, M2TXIndex *tex)  
{
	FILE *texFPtr;
	uint32 i, pixels;

	texFPtr = qOpenWriteFile(fileName);
	if (texFPtr == NULL)							
		return (M2E_BadFile);				
	pixels = tex->XSize * tex->YSize;
	for (i=0; i<pixels; i++)
	{
		qWriteByteStream(texFPtr,(BYTE *)&(tex->Index[i]),1);
	}
	qCloseWriteFile(texFPtr);
	return (M2E_NoErr);
}

M2Err M2TXPIP_WriteFile(char *fileName, uint16 entries, M2TXPIP *pip)	
{
	FILE *texFPtr;
	uint32 i;

	texFPtr = qOpenWriteFile(fileName);
	if (texFPtr == NULL)							
		return (M2E_BadFile);				
	for (i=0; i<entries; i++)
	{
		qWriteByteStream(texFPtr,(BYTE *)&(pip->PIPData[i]),4);
	}
	qCloseWriteFile(texFPtr);
	return (M2E_NoErr);
}

M2Err M2TXRaw_WriteChannels(char *fileName, M2TXRaw *tex, uint16 channels) 
{
	FILE *texFPtr;
	uint32 i, pixels;

	texFPtr = qOpenWriteFile(fileName);
	if (texFPtr == NULL)							
		return (M2E_BadFile);				
	pixels = tex->XSize * tex->YSize;
	for (i=0; i<pixels; i++)
	{
		if (channels&M2Channel_Red)
			qWriteByteStream(texFPtr,(BYTE *)&(tex->Red[i]),1);
		if (channels&M2Channel_Green)
			qWriteByteStream(texFPtr,(BYTE *)&(tex->Green[i]),1);
		if (channels&M2Channel_Blue)
			qWriteByteStream(texFPtr,(BYTE *)&(tex->Blue[i]),1);
		if (channels&M2Channel_Alpha)
			qWriteByteStream(texFPtr,(BYTE *)&(tex->Alpha[i]),1);
		if (channels&M2Channel_SSB)
			qWriteByteStream(texFPtr,(BYTE *)&(tex->SSB[i]),1);
	}
	qCloseWriteFile(texFPtr);
	return (M2E_NoErr);
}

M2Err M2TXRaw_ReadChannels(char *fileName, M2TXRaw *tex, uint16 channels) 
{
	FILE *texFPtr;
	uint32 i, pixels;

	texFPtr = qOpenReadFile(fileName);
	if (texFPtr == NULL)							
		return (M2E_BadFile);				
	pixels = tex->XSize * tex->YSize;
	for (i=0; i<pixels; i++)
	{
		if (channels&M2Channel_Red)
			qReadByteStream(texFPtr,(BYTE *)&(tex->Red[i]),1);
		if (channels&M2Channel_Green)
			qReadByteStream(texFPtr,(BYTE *)&(tex->Green[i]),1);
		if (channels&M2Channel_Blue)
			qReadByteStream(texFPtr,(BYTE *)&(tex->Blue[i]),1);
		if (channels&M2Channel_Alpha)
			qReadByteStream(texFPtr,(BYTE *)&(tex->Alpha[i]),1);
		else
			tex->Alpha[i]=0xFF;
		if (channels&M2Channel_SSB)
			qReadByteStream(texFPtr,(BYTE *)&(tex->SSB[i]),1);
	}
	qCloseReadFile(texFPtr);
	return (M2E_NoErr);
}
