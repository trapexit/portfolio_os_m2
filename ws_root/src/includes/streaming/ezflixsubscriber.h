/******************************************************************************
**
**  @(#) ezflixsubscriber.h 96/03/04 1.6
**
*******************************************************************************/
#ifndef __EZFLIXSUBSCRIBER_H__
#define __EZFLIXSUBSCRIBER_H__

#ifndef	__GRAPHICS_GRAPHICS_H
#include <graphics/graphics.h>
#endif

#ifndef __STREAMING_DSSTREAMDEFS_H
#include <streaming/dsstreamdefs.h>
#endif

#ifndef __STREAMING_DATASTREAM_H
#include <streaming/datastream.h>
#endif


/******************************/
/* Public types and constants */
/******************************/

typedef struct
{
	int32	rowBytes;
	int32	baseAddr;
	ubyte*	movieBuffer;

	int32	width;
	int32	height;
	int32	xPos;
	int32	yPos;
	
} EZFlixImageDesc, *EZFlixImageDescPtr;


/**************************************
 * Filed (stream chunk) data formats.
 * [TBD] Move these to dsstreamdefs.h
 **************************************/

typedef	struct EZFlixHeader
{
	SUBS_CHUNK_COMMON;
	int32		version;		/*	Version of EZFlix data			*/
	int32		cType;			/*	Video compression type			*/
	uint32		height;			/*	Height of each frame			*/
	uint32		width;			/*	Width of each frame				*/
	int32		scale;			/*	Timescale of Film				*/
	int32		count;			/*	Number of frames				*/
	/* Decompressor parameters */
	uint32		codecVersion;
	uint8		yquant[4];
	uint8		uvquant[4];
	uint8		min;
	uint8		max;
	uint8		invgamma;
} EZFlixHeader, *EZFlixHeaderPtr;

typedef	struct	EZFlixFrame
{
	SUBS_CHUNK_COMMON;
	int32		duration;			/*	Duration of this sample		*/
	int32		frameSize;			/*	Number of bytes in frame	*/
	char		frameData[4];		/*	Compressed frame data...	*/
} EZFlixFrame, *EZFlixFramePtr;


/**************************************/
/* Subscriber context, one per stream */
/**************************************/

typedef struct EZFlixContext EZFlixContext, *EZFlixContextPtr;


/*********************************/
/* EZFlix channel context record */
/*********************************/

typedef struct EZFlixRec EZFlixRec, *EZFlixRecPtr;


/*****************************/
/* Public routine prototypes */
/*****************************/

#ifdef __cplusplus 
extern "C" {
#endif


Item	NewEZFlixSubscriber(DSStreamCBPtr streamCBPtr, int32 deltaPriority,
			Item msgItem, uint32 numChannels, EZFlixContextPtr *pContextPtr);

int32	EZFlixDuration(EZFlixRecPtr recPtr);
int32	EZFlixCurrTime(EZFlixRecPtr recPtr);

Err		InitEZFlixStreamState(DSStreamCBPtr streamCBPtr,
							  EZFlixContextPtr ctx, 
							  EZFlixRecPtr *pRecPtr, 
							  uint32 channel, 
							  bool flushOnSync);
Err		DestroyEZFlixStreamState(EZFlixContextPtr ctx, EZFlixRecPtr recPtr);
bool	DrawEZFlixToBuffer(EZFlixContextPtr ctx, EZFlixRecPtr recPtr, Bitmap *bitmap);
void	FlushEZFlixChannel(EZFlixContextPtr ctx, EZFlixRecPtr recPtr);


#ifdef __cplusplus
}
#endif

#endif	/* __EZFLIXSUBSCRIBER_H__ */
