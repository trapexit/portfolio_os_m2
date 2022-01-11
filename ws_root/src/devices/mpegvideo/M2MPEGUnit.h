/* @(#) M2MPEGUnit.h 96/12/11 1.11 */
/* file: M2MPEGUnit.h */
/* register definitions for the 3DO M2 MPEG Unit */
/* 10/10/94 George Mitsuoka */
/* The 3DO Company Copyright © 1994 */

/*	these definitions are taken from the 3DO M2 Specification,
	Chapter 5 (MPEG Unit) */

#ifndef M2MPEGUNIT_HEADER
#define M2MPEGUNIT_HEADER

#include <kernel/types.h>

#ifndef MPEGVIDEOPARSE_HEADER
#include "MPEGVideoParse.h"
#endif

#ifndef VIDEO_DEVICECONTEXT_HEADER
#include "videoDeviceContext.h"
#endif

typedef
	union
	{
		uint32 i;
		struct
		{
			unsigned version:16, revision:16;
		}
		f;
	}
	TDeviceIDReg;

typedef
	union
	{
		uint32 i;
		struct
		{
			unsigned reserved0:19,
					 vofRdEnable:1, vofWrEnable:1, vofReset_n:1,
					 vodEnable:1, vodReset_n:1,
					 motEnable:1, motReset_n:1,
					 mvdReset_n:1,
					 parserStep:1, parserEnable:1, parserReset_n:1,
					 vbdEnable:1, vbdReset_n:1;
		}
		f;
	}
	TConfigReg;

typedef
	union
	{
		uint32 i;
		struct
		{
			unsigned reserved0:25,
					 stripBufferError:1, everythingDone:1,
					 outputFormatter:1, outputDMA:1, bitstreamError:1,
					 endOfPicture:1, videoBitstreamDMA:1;
		}
		f;
	}
	TInterruptReg;

typedef
	struct
	{
		unsigned reserved0:7, address:25;
		unsigned reserved1:15, length:17;
	}
	TDMAReg;

typedef
	union
	{
		uint32 i;
		struct
		{
			unsigned reserved0:15,
					 vbdSnoopEnable:1, bufferByteCount:16;
		}
		f;
	}
	TBufferStatusReg;

typedef
	union
	{
		uint32 i;
		struct
		{
			unsigned b0002:3, fullPelBackwardVector:1, b04:1, backwardRSize:3,
					 b0810:3, fullPelForwardVector:1, b12:1, forwardRSize:3,
					 b16:1, priorityMode:3, b2028:9, pictureCodingType:3;
		}
		f;
	}
	TParserConfigReg;

typedef
	union
	{
		uint32 i;
		struct
		{
			unsigned b00:1, mbRow:7, b08:1,
					 bitstreamError:1, errorState:6, evalBits:16;
		}
		f;
	}
	TParserStatus0Reg;

typedef
	union
	{
		uint32 i;
		struct
		{
			unsigned b0002:3, blockNumber:3, macroBlockType:5, numBitsValid:5,
					 lastStartCode:8, b24:1, mbCol:7;
		}
		f;
	}
	TParserStatus1Reg;

typedef
	union
	{
		uint32 i;
		struct
		{
			unsigned b0015:16, mbHeight:8, mbWidth:8;
		}
		f;
	}
	TImageSizeReg;

typedef
	union
	{
		uint32 i;
		struct
		{
			unsigned reserved0:7, address:25;
		}
		f;
	}
	TRefBufAddrReg;

typedef
	union
	{
		uint32 i;
		struct
		{
			unsigned b0006:7,outputBufferAddress:13, b2027:8,
					 fullFrameStripBuffer:1, stripBufferEnable:1,
					 stripBufferSize:2;
		}
		f;
	}
	TOutputDMAConfigReg;

#define STRIPBUFFERSIZE16K	0
#define STRIPBUFFERSIZE32K	1
#define STRIPBUFFERSIZE64K	2
#define STRIPBUFFERSIZE128K	3

typedef
	union
	{
		uint32 i;
		struct
		{
			unsigned b0002:3, vofSnoopEnable:1, b0406:3, ditherMode:1,
					 b0809:2, operaFormat:1, format:1, b12:1, resamplerMode:3,
					 b1618:3, enableCSC:1, bit2022:3, rowChunks:9;
		}
		f;
	}
	TOutputFormatConfigReg;

typedef
	union
	{
		uint32 i;
		struct
		{
			unsigned b00:1,hStart:7,b08:1,vStart:7,b16:1,hStop:7,
					 b24:1,vStop:7;
		}
		f;
	}
	TOutputFormatCropReg;

typedef
	union
	{
		uint32 i;
		struct
		{
			unsigned b0023:24, DSB:1, alphaFillValue:7;
		}
		f;
	}
	TOutputFormatAlphaFillReg;

#define MPG_REG_DEVICEID			0x0000L
#define MPG_REG_DRIVERTYPE			0x0058L
#define MPG_REG_CONFIG				0x0004L
#define MPG_REG_INTENABLE			0x0008L
#define MPG_REG_INTSTATUS			0x000cL
#define MPG_REG_VBSCURADDR			0x0010L
#define MPG_REG_VBSCURLEN			0x0014L
#define MPG_REG_VBSNEXTADDR			0x0018L
#define MPG_REG_VBSNEXTLEN			0x001cL
#define MPG_REG_VBSCONFIG			0x0020L
#define MPG_REG_PARSERCONFIG		0x0024L
#define MPG_REG_PARSERSTATUS0		0x0028L
#define MPG_REG_PARSERSTATUS1		0x005cL
#define MPG_REG_IMAGESIZE			0x002cL
#define MPG_REG_REVPREDBUF			0x0054L
#define MPG_REG_FWDPREDBUF			0x0030L
#define MPG_REG_OUTDMACONFIG		0x0034L
#define MPG_REG_FORMATCONFIG		0x0038L
#define MPG_REG_FORMATSIZE			0x0050L
#define MPG_REG_FORMATCROP			0x0040L
#define MPG_REG_FORMATINPUT			0x003cL
#define MPG_REG_FORMATOUTPUT		0x0044L
#define MPG_REG_UPPERDITHERMTX		0x0048L
#define MPG_REG_LOWERDITHERMTX		0x004cL
#define MPG_REG_LFSR				0x0050L
#define MPG_REG_FORMATALPHAFILL		0x0060L
#define MPG_REG_INTRAQUANTTABLE		0x0200L
#define MPG_REG_NONINTRAQUANTTABLE	0x0240L

int32 M2MPEGInit( void );
int32 M2MPEGDelete( void );
int32 M2MPEGChangeOwner( Item newOwner );
Err M2MPGValidateParameters( tVideoDeviceContext *parameters );
Err M2MPGDumpState( void );
Err M2MPGStartSlice(tVideoDeviceContext* theUnit, MPEGStreamInfo *si,
					uint8 *vbsBuf, int32 vbsLen);
Err M2MPGEndSlice( void );
Err M2MPGContinue( uint8 *vbsBuf, int32 vbsLen );
Err M2MPGEndDMA( uint8 *buf, int32 *count, uint8 **addr );

#define M2MPGVBSBUFFERSIZE		64

#define M2MPGPASS1				0x00000000L
#define DRIVERTYPE_MPEG1		0x00000000L

#endif

