/*
 **
 **  @(#) EZFlixStream.h 95/10/01 1.2
 **
	File:		EZFlixStream.h

	Contains:	xxx put contents here xxx

	Written by:	Donn Denman and John R. McMullen

	To Do:
*/


#ifndef __EZFLIXSTREAM_H__
#define __EZFLIXSTREAM_H__

#include "EZFlixXPlat.h"

#ifdef makeformac
#include "subschunkcommon.h"
#else
#include <:streaming:subschunkcommon.h>
#endif

#include "EZFlixCodec.h"

/**********************/
/* Internal constants */
/**********************/

/* The following are definitions for constants that mark the beginning of
 * data chunka in a 3DO file.
 */
#define	EZFLIX_CHUNK_TYPE			CHAR4LITERAL('E','Z','F','L')	/* chunk data type for this subscriber */
#define	EZFLIX_HDR_CHUNK_TYPE		CHAR4LITERAL('F','H','D','R')	/* chunk data type for this subscriber */
#define	EZFLIX_FRME_CHUNK_TYPE		CHAR4LITERAL('F','R','M','E')	/* chunk data type for this subscriber */

#define kEZFlixTrackType				'EZFL'	/* Track type for EZFlix */
#define kEZFlixChunkType				'EZFL'	/* Stream Chunk type for Flix */
#define	kEZFlixHeaderType				'FHDR'
#define	kEZFlixFrameType				'FRME'
#define	kEZFlixKeyFrameType				'FRME'
#define	kEZFlixDifferenceFrameType		'FRME'

/**********************************************/
/* Format of a data chunk for this subscriber */
/**********************************************/

#ifndef SUBSCRIBER_HEADER

typedef struct ChannelAndFlags {
	short	chunkFlags;	/* reserved for flags	*/
	short	channel;	/* logical channel number */
} ChannelAndFlags;

typedef union ChannelInfo {
	long			channelAndFlags;
	ChannelAndFlags	shuttle;
} ChannelInfo;

#define	SUBSCRIBER_HEADER											\
	long		chunkType;		/* chunk type */					\
	long		chunkSize;		/* chunk size including header */	\
	long		time;			/* position in stream time */		\
	ChannelInfo	cInfo;			/* flags and channel number	*/		\
	long		subChunkType	/* data sub-type */

typedef struct SimpleHeader {
	SUBSCRIBER_HEADER;
} SimpleHeader;

typedef struct SimpleChunk {
	SUBS_CHUNK_COMMON;
} SimpleChunk;

#if sizeof(SimpleChunk) != sizeof(SimpleHeader)
#error "SUBSCRIBER_HEADER and SUBS_CHUNK_COMMON must match!"
#endif

#endif /* SUBSCRIBER_HEADER */

/**********************************************/
/* OPERA VERSION							  */
/**********************************************/

typedef	struct EZFlixHeader
{
	SUBSCRIBER_HEADER;
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
	uint8		unused;
} EZFlixHeader, *EZFlixHeaderPtr;

/* An EZFlix Frame */

typedef	struct	EZFlixFrame
{
	SUBSCRIBER_HEADER;
	int32		duration;			/*	Duration of this sample		*/
	int32		frameSize;			/*	Number of bytes in frame	*/
	char		frameData[0];		/*	Compressed frame data...	*/
} EZFlixFrame, *EZFlixFramePtr;

/**********************************************/
/* M2 VERSION							  */
/**********************************************/

typedef	struct EZFlixM2Header
{
	SUBSCRIBER_HEADER;
	int32		version;		/*	Version of EZFlix data			*/
	int32		cType;			/*	Video compression type			*/
	uint32		height;			/*	Height of each frame			*/
	uint32		width;			/*	Width of each frame				*/
	int32		scale;			/*	Timescale of Film				*/
	int32		count;			/*	Number of frames				*/
	/* Decompressor parameters */
	uint32		codecVersion;
	uint8		min;
	uint8		max;
	uint8		invgamma;
	uint8		unused;
} EZFlixM2Header, *EZFlixM2HeaderPtr;

/* An EZFlix Frame */

typedef	struct	EZFlixM2Frame
{
	SUBSCRIBER_HEADER;
	int32		duration;			/*	Duration of this sample		*/
	int32		frameSize;			/*	Number of bytes in frame	*/
	char		frameData[0];		/*	Compressed frame data...	*/
} EZFlixM2Frame, *EZFlixM2FramePtr;

#endif /* __EZFLIXSTREAM_H__ */
