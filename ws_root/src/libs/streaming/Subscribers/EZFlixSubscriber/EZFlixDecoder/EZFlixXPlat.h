/*
**
**	@(#) EZFlixXPlat.h 96/03/04 1.10
**

	Contains:	Cross Platform Definitions

	Written by:	Donn Denman and John R. McMullen


	To Do:
*/


#ifndef __ezflixxplat__
#define __ezflixxplat__

#ifndef makeformac

#include <kernel/types.h>
#include <streaming/datastream.h>
#include <streaming/dserror.h>

#define psErr		Err
#define psNoErr		0
#define Ptr			unsigned char*
#define Size		unsigned long
#define memFullErr	kDSNoMemErr

#else /* makeformac */

#define psErr		OSErr
#define psNoErr		noErr

#ifndef uint32
#define uint32		unsigned long
#endif
#ifndef int32
#define int32		long
#endif
#ifndef uint8
#define uint8		unsigned char
#endif
#ifndef uint16
#define uint16		unsigned short
#endif

#endif /* makeformac */

#define Byte unsigned char

/* structures used for holding rasters */
typedef union RGBTriple
{
	long whole;
	struct
	{
		unsigned char accum;
		unsigned char red;
		unsigned char green;
		unsigned char blue;
	} part;
} RGBTriple;


/* Decoder "Globals" structure */
typedef struct DecoderBuffers
{						
	/*	Component stuff */
	Ptr					buffer;			/* buffer allocated */
	unsigned long		bufferSize;		/* buffer size */
	unsigned char*		outputBuffer;	/* Y Frame output values */
	unsigned char*		uBuffer;		/* U Frame output values */ 
	unsigned char*		vBuffer;		/* V Frame output values */
	unsigned char*		yStripBuffer;	/* Single row of y values */
	unsigned long		yStripBuffSize;	/* buffer size */
	unsigned char*		uStripBuffer;	/* Single row of u values */
	unsigned char*		vStripBuffer;	/* Single row of v values */
	unsigned long		uvStripBuffSize;/* buffer size */
	long				imageWidth;		/* image width */
} DecoderBuffers;

#ifdef makeformac

#include <Components.h>

typedef struct
{						
	/*	Component stuff */
	ComponentInstance	self;					/*	self instance needed if targeted */
	short				selfResFileRefNum;
	Component			selfComponent;
	DecoderBuffers		decoderBuffers;
	long**				compressionBuffer;
	long				compressBufSize;
	RGBTriple**			contiguousPixels;
	long				destImageSize;
} PrivateGlobals;

#endif /* makeformac */

#ifdef __cplusplus
extern "C"{
#endif

void DeleteEZFlixPtr(Ptr memPtr, Size byteCount);
Ptr NewEZFlixPtr(Size byteCount);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ezflixxplat__ */
