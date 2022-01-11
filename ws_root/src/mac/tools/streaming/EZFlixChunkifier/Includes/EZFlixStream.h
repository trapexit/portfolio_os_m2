/******************************************************************************
**
**  @(#) EZFlixStream.h 96/03/06 1.7
**
******************************************************************************/


#ifndef __EZFLIXSTREAM_H__
#define __EZFLIXSTREAM_H__

#include <streaming/dsstreamdefs.h>
#include "EZFlixXPlat.h"

/**********************/
/* Internal constants */
/**********************/

/* The following are definitions for constants that mark the beginning of
 * data chunka in a 3DO file.
 */
#define kEZFlixTrackType				'EZFL'	/* QT Track type for EZFlix */
#define	kEZFlixHeaderType				'FHDR'
#define	kEZFlixFrameType				'FRME'
#define	kEZFlixKeyFrameType				'FRME'
#define	kEZFlixDifferenceFrameType		'FRME'

/**********************************************/
/* OPERA VERSION							  */
/**********************************************/

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
	uint8		unused;
} EZFlixHeader, *EZFlixHeaderPtr;

/* An EZFlix Frame */

typedef	struct	EZFlixFrame
{
	SUBS_CHUNK_COMMON;
	int32		duration;			/*	Duration of this sample		*/
	int32		frameSize;			/*	Number of bytes in frame	*/
	char		frameData[4];		/*	Compressed frame data...	*/
} EZFlixFrame, *EZFlixFramePtr;

/**********************************************/
/* M2 VERSION							  */
/**********************************************/

typedef	struct EZFlixM2Header
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
	uint8		min;
	uint8		max;
	uint8		invgamma;
	uint8		unused;
} EZFlixM2Header, *EZFlixM2HeaderPtr;

/* An EZFlix Frame */

typedef	struct	EZFlixM2Frame
{
	SUBS_CHUNK_COMMON;
	int32		duration;			/*	Duration of this sample		*/
	int32		frameSize;			/*	Number of bytes in frame	*/
	char		frameData[4];		/*	Compressed frame data...	*/
} EZFlixM2Frame, *EZFlixM2FramePtr;

#endif /* __EZFLIXSTREAM_H_ */

