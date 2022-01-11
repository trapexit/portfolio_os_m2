/* @(#) MPEGVideoBuffers.c 96/12/11 1.1 */
/* file: MPEGVideoBuffers.c */
/* routines to manage Strip and Reference buffers */
/* 12/06/96 Ian Lepore */
/* The 3DO Company Copyright © 1996 */

#include <string.h>
#include <kernel/types.h>
#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <kernel/super.h>
#include <kernel/debug.h>
#include <kernel/operror.h>
#include <device/mpegvideo.h>
#include "MPEGVideoBuffers.h"
#include "mpVideoDriver.h"

#ifdef DEBUG_PRINT_BUFALLOC
  #define DBGBUF(a)		PRNT(a)
#else
  #define DBGBUF(a)		(void)0
#endif

StripBuffer		gStripBuffer;   /* Global strip buffer. Shared by all thread/devices. */
static uint32	gPageSize;		/* this never changes; get it once and cache it locally */

static int32 ValidateSBAddrSize(void *address, int32 size)
{
	int32	sbSizeCode;
	
	switch (size)
	{
		case  16 * 1024:
			sbSizeCode = STRIPBUFFERSIZE16K;
			break;
		case  32 * 1024:	
			sbSizeCode = STRIPBUFFERSIZE32K;
			break;
		case  64 * 1024:	
			sbSizeCode = STRIPBUFFERSIZE64K;
			break;
		case 128 * 1024:	
			sbSizeCode = STRIPBUFFERSIZE128K;
			break;
		default:
			PERR(("Invalid strip buffer size\n"));
			return PARAMERROR;
	} 
	
	if (((uint32)address & (size-1)) != 0) 
	{
		PERR(("Strip buffer address improperly aligned\n"));
		return PARAMERROR;
	}
	
	return sbSizeCode;
}

Err CalcBufferInfo(MPEGBufferInfo *bi, int32 fmvWidth, int32 fmvHeight, int32 stillWidth)
{
	uint32			refBytes;
	uint32			stripBytes;
	
	if (gPageSize == 0) 
		gPageSize = GetPageSize(MEMTYPE_NORMAL);
	
	if (fmvWidth <= 0 || fmvHeight <= 0) 
	{
		fmvWidth  = 0;
		refBytes  = 0;
	}
	else
	{
		fmvWidth	= ALLOC_ROUND(fmvWidth,  MACROBLOCKWIDTH);
		fmvHeight	= ALLOC_ROUND(fmvHeight, MACROBLOCKHEIGHT);
		refBytes	= 2 * ALLOC_ROUND(((fmvWidth * fmvHeight * 3) / 2), gPageSize);
	}
	
	if (stillWidth < fmvWidth)
		stillWidth = fmvWidth;
	else
		stillWidth = ALLOC_ROUND(stillWidth, MACROBLOCKWIDTH);	/* FIXME: is this needed for a stripbuffer? */
		
	if (stillWidth == 0) 	/* yes, we can actually get a width of zero here:				*/
	{						/* client can use CMD_ALLOC with both fmvWidth and stillWidth	*/
		stripBytes	= 0;	/* zero to ask us to discard our buffers and reallocate them	*/
	}						/* later based on the next sequence header we see.				*/
	else if (stillWidth <= 341)
	{
		stripBytes 	= 16 * 1024;
	}
	else if (stillWidth <= 682)
	{
		stripBytes	= 32 * 1024;
	}
	else if (stillWidth <= 1365)
	{
		stripBytes	= 64 * 1024;
	}
	else if (stillWidth <= 2730)
	{
		stripBytes	= 128 * 1024;
	}
	else
	{
		PERR(("Invalid strip width (buffer would exceed max of 128k)\n"));
		return PARAMERROR;
	} 
	
	bi->refBuffer.minSize		 = refBytes;
	bi->refBuffer.memFlags		 = MEMTYPE_NORMAL;
	bi->refBuffer.memCareBits	 = gPageSize - 1;
	bi->refBuffer.memStateBits	 = 0;
	
	bi->stripBuffer.minSize		 = stripBytes;
	bi->stripBuffer.memFlags	 = MEMTYPE_NORMAL;
	bi->stripBuffer.memCareBits	 = stripBytes - 1;
	bi->stripBuffer.memStateBits = 0;

	return 0;
}

void UnUseStripBuffer(tVideoDeviceContext *theUnit)
{
	memset(&theUnit->myStripBuffer, 0, sizeof(theUnit->myStripBuffer));

	theUnit->stripBuffer = NULL;
	
	theUnit->lastBufferWidth  = 0;	/* disable fast-out logic for AllocDecodeBuffers() so that */
	theUnit->lastBufferHeight = 0; 	/* the next sequence hdr triggers a full recalc. */
}

Err UseStripBuffer(tVideoDeviceContext *theUnit, void *address, uint32 size)
{
	int32	sbSizeCode;
	
	if (address == NULL)
	{
		theUnit->stripBuffer = &gStripBuffer;
	}
	else
	{
		if ((sbSizeCode = ValidateSBAddrSize(address, size)) < 0) 
			return sbSizeCode;	/* PARAMERROR, bogus buffer address (alignment) or size */
	
		theUnit->myStripBuffer.sizeCode		= sbSizeCode;
		theUnit->myStripBuffer.sizeInBytes	= size;
		theUnit->myStripBuffer.address 		= address; 
		theUnit->stripBuffer				= &theUnit->myStripBuffer;
	}
	
	return 0;
}

void FreeGlobalStripBuffer(void)
{
	if (gStripBuffer.address) 
	{
		DBGBUF(("Free global strip buffer @%08p size %d\n", gStripBuffer.address, gStripBuffer.sizeInBytes));
		SuperFreeMem(gStripBuffer.address, gStripBuffer.sizeInBytes);
	}
	
	memset(&gStripBuffer, 0, sizeof(StripBuffer));
}

Err AllocGlobalStripBuffer(tVideoDeviceContext* theUnit)
{
	/* Allocate it if that hasn't been done already */
	
	if (gStripBuffer.address == NULL)
	{
		gStripBuffer.sizeCode	 = DEFAULT_SBSIZECODE;
		gStripBuffer.sizeInBytes = DEFAULT_STRIPBUFFERSIZE;
		gStripBuffer.address = SuperAllocMemAligned(gStripBuffer.sizeInBytes, MEMTYPE_NORMAL, gStripBuffer.sizeInBytes);
		if (gStripBuffer.address == NULL) {
			PERR(("couldn't allocate global strip buffer\n"));
			return NOMEM;
		}
	
		DBGBUF(("Alloc global strip buffer @%08p size %d\n", gStripBuffer.address, gStripBuffer.sizeInBytes));
	}
	
	return UseStripBuffer(theUnit, NULL, 0);
}

void FreeStripBuffer(tVideoDeviceContext *theUnit)
{
	if (theUnit->myStripBuffer.address)
	{
		DBGBUF(("Free local strip buffer @%08p size %d\n", theUnit->myStripBuffer.address, theUnit->myStripBuffer.sizeInBytes));
		SuperFreeMem(theUnit->myStripBuffer.address, theUnit->myStripBuffer.sizeInBytes);
	}
	UnUseStripBuffer(theUnit);	
}

Err AllocStripBuffer(tVideoDeviceContext *theUnit, MPEGBufferInfo *bi)
{
	void *	address;
	
	FreeStripBuffer(theUnit);
	
	if (bi->stripBuffer.minSize == DEFAULT_STRIPBUFFERSIZE) 
	{
		DBGBUF(("Alloc local strip buffer will use global buffer\n"));
		return AllocGlobalStripBuffer(theUnit);
	}
	
	address = SuperAllocMemMasked(bi->stripBuffer.minSize, 
				bi->stripBuffer.memFlags, 
				bi->stripBuffer.memCareBits,
				bi->stripBuffer.memStateBits);

	if (address == NULL) 
	{
		PERR(("couldn't allocate local strip buffer of %d bytes\n", bi->stripBuffer.minSize));
		return NOMEM;
	}
	
	DBGBUF(("Alloc local strip buffer @%08p size %d\n", address, bi->stripBuffer.minSize));

	return UseStripBuffer(theUnit, address, bi->stripBuffer.minSize);
}

void UnUseReferenceBuffers(tVideoDeviceContext* theUnit)
{
	theUnit->refPic[0]			= NULL;
	theUnit->refPic[1]			= NULL;
	theUnit->refPicSize			= 0;
	theUnit->lastBufferWidth	= 0;	/* disable fast-out logic for AllocDecodeBuffers() so that */
	theUnit->lastBufferHeight	= 0; 	/* the next sequence hdr triggers a full recalc. */
}

Err UseReferenceBuffers(tVideoDeviceContext* theUnit, void *address, uint32 size)
{
	if (gPageSize == 0) 
		gPageSize = GetPageSize(MEMTYPE_NORMAL);

	if (((uint32)address & (gPageSize-1)) != 0 || (size & (gPageSize-1)) != 0)
	{
		PERR(("Reference buffer address and/or size not page-aligned\n"));
		return PARAMERROR;
	}

	theUnit->refPic[0]	= (uint32 *)address;
	theUnit->refPic[1] 	= (uint32 *)((char *)address + (size / 2));
	theUnit->refPicSize	= size;
	
	return 0;
}

void FreeReferenceBuffers(tVideoDeviceContext* theUnit)
{
	if (theUnit->refPicSize != 0)
	{
		DBGBUF(("Free reference buffers @%08p size %d\n", theUnit->refPic[0], theUnit->refPicSize));
		SuperFreeMem( theUnit->refPic[0], theUnit->refPicSize );
	}
	UnUseReferenceBuffers(theUnit);
}

Err AllocReferenceBuffers(tVideoDeviceContext *theUnit, MPEGBufferInfo *bi)
{
	void *	address;
	
	FreeReferenceBuffers(theUnit);
	
	address = SuperAllocMemMasked(bi->refBuffer.minSize,
							bi->refBuffer.memFlags,
							bi->refBuffer.memCareBits,
							bi->refBuffer.memStateBits);
							
	if (address == NULL) 
	{
		PERR(("couldn't allocate reference buffers totalling %d bytes\n", bi->refBuffer.minSize));
		return NOMEM;
	}
	
	DBGBUF(("Alloc reference buffers @%08p size %d\n", address, bi->refBuffer.minSize));
	
	return UseReferenceBuffers(theUnit, address, bi->refBuffer.minSize);
}

Err AllocDecodeBuffers(tVideoDeviceContext* theUnit, MPEGStreamInfo *si)
{
	Err				err;
	uint32			width;
	uint32			height;
	int32			stripBytes;
	MPEGBufferInfo	bi;

	width  = SEQ_WIDTH(si);
	height = SEQ_HEIGHT(si);

	if (width == theUnit->lastBufferWidth && height == theUnit->lastBufferHeight)
		return 0;	/* fast out: no change since we last set up the buffers */

	err = CalcBufferInfo(&bi, width, height, width);
	if (err < 0)
		return err;

	stripBytes = (theUnit->stripBuffer == NULL) ? 0 : theUnit->stripBuffer->sizeInBytes;

	if (stripBytes < bi.stripBuffer.minSize)
	{
		if (theUnit->vdcFlags & VDCFLAG_SB_CLIENT_CONTROL)
		{
			PERR(("Client-controlled strip buffer size %d is too small for current requirement of %d bytes\n", stripBytes, bi.stripBuffer.minSize));
			return NOMEM;
		}
		else 
		{
			DBGBUF(("Reallocating strip buffer from: %d to: %d bytes\n", stripBytes, bi.stripBuffer.minSize));
			err = AllocStripBuffer(theUnit, &bi);
			if (err < 0) 
				return err;
		}
	}
	
	if (theUnit->playMode == MPEGMODEIFRAMESEARCH)
		return 0;	/* don't do anything with reference buffers if in iframe mode */
	
	if (theUnit->refPicSize != bi.refBuffer.minSize)
	{
		if (theUnit->vdcFlags & VDCFLAG_RB_CLIENT_CONTROL) 
		{
			if (theUnit->refPicSize < bi.refBuffer.minSize)
			{
				PERR(("Client-controlled reference buffer size %d is too small for current requirement of %d total bytes\n", theUnit->refPicSize, bi.refBuffer.minSize));
				return NOMEM;
			}
			else
			{
				DBGBUF(("Client controlled buffer of %d bytes is being used; really only need %d\n", theUnit->refPicSize, bi.refBuffer.minSize));
			}
		}
		else
		{
			DBGBUF(("Reallocating reference buffers from: %d to: %d total bytes\n", theUnit->refPicSize, bi.refBuffer.minSize));
			err = AllocReferenceBuffers(theUnit, &bi);
			if (err < 0)
				return err;
		}
	}

	theUnit->lastBufferWidth  = width;	/* strip and reference buffers now correctly set up for current	*/
	theUnit->lastBufferHeight = height;	/* sizes, remember sizes for fast-out on subsequent seq. hdrs.	*/
	
	return 0;
}

void FreeDecodeBuffers(tVideoDeviceContext* theUnit)
{
	FreeReferenceBuffers(theUnit);
	FreeStripBuffer(theUnit);
}
