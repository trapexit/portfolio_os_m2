/* @(#) MPEGStream.h 96/12/11 1.12 */
/* file: MPEGStream.h */
/* mpeg stream io routine prototypes */
/* 8/14/93 George Mitsuoka */
/* The 3DO Company Copyright © 1993 */

#ifndef MPEGSTREAM_HEADER
#define MPEGSTREAM_HEADER

#include <stdio.h>
#include <kernel/types.h>

#ifndef VIDEO_DEVICECONTEXT_HEADER
#include "videoDeviceContext.h"
#endif

#define LOOKAHEADSIZE 	80

typedef
	struct
	{
		int32 OSStreamID;
		int32 lookAheadValid;
		int32 offset;
		uint8 lookAhead[ LOOKAHEADSIZE ];
		uint8 *bufferStart,*buffer,*bufferEnd;
		int32 length;
		uint32 DMAState;
		char  *fileName;
		FILE  *slicesFile, *commandsFile;
		uint32 flags, PTS, userData;
		uint32 lastFlags, lastPTS, lastUserData;
	}
	streamContext;

#define SC_FLAG_PTS_VALID		(1L)

#define DMAStateNoDMA			0L		/* no DMA in progress */
#define DMAStateLookAheadDMA	1L		/* DMA from the lookahead buffer */
#define DMAStateBufferDMA		2L		/* DMA from the normal buffer */
	
#define START_CODE_PREFIX			0x100L
#define PICTURE_START_CODE			0x100L
#define SLICE_START_CODE_MIN		0x101L
#define SLICE_START_CODE_MAX		0x1afL
#define USER_DATA_START_CODE		0x1b2L
#define SEQUENCE_HEADER_CODE		0x1b3L
#define SEQUENCE_ERROR_CODE			0x1b4L
#define EXTENSION_START_CODE		0x1b5L
#define SEQUENCE_END_CODE			0x1b7L
#define GROUP_START_CODE			0x1b8L
#define SYSTEM_START_CODE_MIN		0x1b9L
#define SYSTEM_START_CODE_MAX		0x1ffL

int32 NextStartCode(tVideoDeviceContext* theUnit, streamContext *streamID);
int32 FlushBytes(tVideoDeviceContext* theUnit, streamContext *streamID,
				 uint8 count); /* Flushes upto 256 bytes
														   from the stream. */
int32 MPCurrentPTS( streamContext *s, uint32 *thePTS, uint32 *userData );
int32 MPOpen(tVideoDeviceContext* theUnit, streamContext *streamID);
int32 MPRead(tVideoDeviceContext* theUnit, streamContext *streamID,
			 uint8 *dest, int32 length);
int32 MPLook( streamContext *streamID, uint8 *dest, int32 length );
int32 MPNextDMA(tVideoDeviceContext* theUnit, streamContext *streamID,
				uint8 **addr, int32 *length);
int32 MPEndDMA(tVideoDeviceContext* theUnit, streamContext *s,
			   uint8 *nextBytes, int32 len, uint8 *addr);
int32 MPDumpSlices(tVideoDeviceContext* theUnit, streamContext *streamID);

#endif

