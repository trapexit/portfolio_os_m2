/******************************************************************************
**
**  @(#) Films.h 95/10/06 1.4
**
******************************************************************************/
/**
	File:		Films.h

	Contains:	Structures for an example film format

	Written by:	Peter Barrett

******************************************************************************/

/*========================================================================*/

#include <streaming/dsstreamdefs.h>

#define	kFillerChunkType	FILL_CHUNK_TYPE

#define	kSoundChunkType		SNDS_CHUNK_TYPE
#define	kSoundHeaderType	SHDR_CHUNK_TYPE
#define	kSoundSampleType	SSMP_CHUNK_TYPE

#define	kVideoChunkType					'FILM'
#define	kVideoHeaderType				'FHDR'
#define	kVideoFrameType					'FRME'
#define	kVideoKeyFrameType				'FRME'
#define	kVideoDifferenceFrameType		'DFRM'

/* RELATIVE_BRANCHING stuff */
#define	kDependentChunkFlag		   		0		/* indicates a chunk that does depend
										 		* on its sibling predecessors */
#define	kKeyChunkFlag					1<<0	/* indicates a chunk that does not
										 		* depend on its sibling predecessors */
#define kCompactVideoTrackType			'cvid'	/* Track type for Compact Video aka CinePak */


#define	GOTO_CHUNK_TYPE		GOTO_CHUNK_SUBTYPE	/* GOTO type for this subscriber */

#define	kLastSoundChunk		0x8000

typedef	struct FillerChunk {
	long		chunkType;		/*	'FILL'						*/
	long		chunkSize;
} FillerChunk, *FillerChunkPtr;

typedef	struct FilmHeader {
	SUBS_CHUNK_COMMON;
	long		version;		/*	0 for this version			*/
	long		cType;			/*	video compression type		*/
	long		height;			/*	Height of each frame		*/
	long		width;			/*	Width of each frame			*/
	long		scale;			/*	Timescale of Film			*/
	long		count;			/*	Number of frames			*/
} FilmHeader, *FilmHeaderPtr;


typedef	struct	FilmFrame {
	SUBS_CHUNK_COMMON;
	long		duration;		/*	Duration of this sample		*/
	long		frameSize;		/*	Number of bytes in frame	*/
/*	char		frameData[4];		compressed frame data...	*/
} FilmFrame, *FilmFramePtr;


typedef	struct StreamSoundHeader {
	SUBS_CHUNK_COMMON;
	SAudioHeaderChunk		headerChunk;	/* found in <streaming/dsstreamdefs.h> */
	SAudioSampleDescriptor	sampleDesc;		/* found in <streaming/dsstreamdefs.h> */
} StreamSoundHeader, *StreamSoundHeaderPtr;


typedef	struct StreamSoundChunk {
	SUBS_CHUNK_COMMON;
	long		actualSampleSize;/* actual number of bytes in the sample data */
/*	char		soundData[0];		sound data...				*/
} StreamSoundChunk, *StreamSoundChunkPtr;

typedef	struct CtrlMarkerChunk {
	SUBS_CHUNK_COMMON;
	long		marker;			/*	new position for stream		*/
} CtrlMarkerChunk, *CtrlMarkerChunkPtr;



/*========================================================================*/
