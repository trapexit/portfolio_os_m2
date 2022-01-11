#ifndef __WBPARSE__
#define __WBPARSE__
/*******************************************************************************************
 *  @(#) wbparse.h 96/11/26 1.2
 *
 *	File:			WBParse.h
 *
 *	Contains:		API definitions for WBParse.c
 *
 *	Written by:		Jerry H. Morrison
 *
 *	Copyright (c)© 1994,1995, an unpublished work by The 3DO Company. All rights
 *	reserved. This material contains confidential information that is the property of
 *	The 3DO Company. Any unauthorized duplication, disclosure or use is prohibited.
 *
 * History:
 *	5/08/96		hpp		Changed all the int64 pts to int32(33th bit is ignored)
 *	4/23/96		hpp		Changed the WBParse to have a call back routine
 *	4/23/96		hpp		moved the WBParserArgs to this file
 *	1/19/95		jhm		Add WBParseIsEmpty.
 *	7/11/94		jhm		Corrected #includes.
 *	4/29/94		JHM		Moved MPEG definitions off to new file MPEG.h.
 *  3/23/94		JHM		Added fields to VideoSequenceHeader.
 *  2/17/94		JHM		Added MPEG AudioHeader definitions.
 *  2/16/94		JHM		Added MPEG VideoSequenceHeader definitions and WBParserArgs.
 *  2/07/94		JHM		Initial version
 *
 *******************************************************************************************
 * Parser for WhiteBook (VideoCD) special case of ISO 11172-1 MPEG-1 system streams.
 *
 * ASSUMES (per WhiteBook VideoCD spec):
 *   * Each Pack starts on a disk block boundary (no leading zeros) and fits within the block.
 *   * We're always asked to parse a whole number of blocks.
 *   * The block size is a constant multiple of 4 (==> long-word aligned).
 *   * There are only 2 elementary streams: Video 0 (stream ID $E0) and Audio 0 (stream ID $C0).
 *     (If we encounter any other elementary streams, we'll skip over their Packets.)
 *
 * Buffer		= DiskBlock*
 * DiskBlock	= EmptyBlock | Pack [ISO_11172_END_CODE] [ZERO_BYTE^20] .
 * Pack			= PackHeader Packet* .
 * Packet		= PacketHeader [STUFFING_BYTE^(0..16) [STDBufferInfo] PTS_Info]
 *				  packet_data_byte* .
 *
 *******************************************************************************************/

#include <kernel/types.h>
#include <streaming/mempool.h>
#include <video_cd_streaming/dsstreamdefs.h>
#ifdef cplusplus
extern "C" {
#endif /* cplusplus */

/*******************************************************************************************
 *		STRUCTURES
 *******************************************************************************************/

/* The structure DSWBChunk is used to communicate between the stream data parser and
 * the streamer. The parser parses the raw buffers and returns a list of MPEG audio and
 * video tokens. These are used to send data messages to the appropriate subscribers. */

enum SeekState {
	seeking_info		= 0,				/* asks WBParse() to seek some info in the stream */
	seeking_foundInfo	= 1,				/* WBParse() indicates that it found the info in the stream */
	seeking_usedInfo	= 2					/* the client indicates that it has picked up the found info */
};

typedef uint8 SeekState;

typedef struct DSWBChunk {
	WBCHUNK_COMMON
} DSWBChunk, *DSWBChunkPtr;
	
/* Persistent input/output args to WBParse(). */

typedef struct ParserParmBlock {
		
	SeekState		seekingAnSCR;	
	/* Set this to seeking_info when starting a stream or a
	 * branch. When WBParse() finds the first Pack [maybe not in the first buffer passed
	 * to WBParse()], it will set this to seeking_foundInfo, store the SCR in firstSCR
	 * (below), and start emitting data packets. When the client picks up the data, it
	 * can set this to seeking_usedInfo to remember that it's picked up the info. */
	int32			firstSCR;		
	/* WBParse() sets this to the first SCR that
	 * it finds after being asked to find one, per seekingAnSCR. */
		
	SeekState		seekingVideoParams; 
    /* Set this to seeking_info when starting a stream.
	 * Then WBParse() will look in the first video packet for a video sequence header,
	 * read video parameters from it, store them in the following two fields, and set
	 * this to seeking_foundInfo. */
	VideoSequenceHeader	videoSeqHdr;	/* our current video sequence header */
	uint8			videoFrameRate;		/* Cf. the enum MPEGVideoFrameRate in MPEG.h */
	uint32			videoVerticalSize;	/* vertical picture size, in pixels */
	
	SeekState		seekingAudioParameters;
	/* Set this to seeking_info when starting a
	 * stream. Then WBParse() will look in the first audio packet for a syncword, read
	 * audio parameters from it, store them in the following two fields, and set
	 * this to seeking_foundInfo. */
	uint8			audioMode;			/* Cf. the enum MPEGAudioMode in MPEG.h */
	uint8			audioEmphasis;		/* Cf. the enum MPEGAudioEmphasisMode in MPEG.h */
	
	int32			cntMPEGEndCodes;	/* the parser increments this each time it finds an MPEG system end code */
	
	int32			cntParseErrors;		
	/* WBParse increments this each time it detects a parse
	 * error and skips to the next block */

	int32			processStillImageType;	
	/* STREAMID_VIDEO_NORMAL_STILL or STREAMID_VIDEO_HIGH_STILL 
	 * res images are the ones to process; discard the other type */

} ParserParamBlock, *ParserParamBlockPtr;

/*********************************************************************
 * WhiteBook parser functions
 *********************************************************************/
void		WBParse_Initialize(int32 lowOrHighResStillType);

int32		WBParse_Data(const char* buffer, int32 bufferSize, DSWBChunkPtr chunk, uint32 *numOfChunks);

Boolean 	WBParse_IsEmpty(const char* buffer, int32 bufferSize);

VideoSequenceHeaderPtr WBParse_GetVideoSequenceHeader(void);
SeekState 	WBParse_GetSCRSeekState(void);
void	 	WBParse_SetSCRSeekState(SeekState state);
int32 		WBParse_GetFirstSCR(void);
SeekState 	WBParse_GetVideoSeekState(void);
void	 	WBParse_SetVideoSeekState(SeekState state);
uint8 		WBParse_GetVideoFrameRate(void);
uint32		WBParse_GetVideoVerticalSize(void);
SeekState 	WBParse_GetAudioSeekState(void);
void	 	WBParse_SetAudioSeekState(SeekState state);
uint8 		WBParse_GetAudioMode(void);
uint8 		WBParse_GetAudioEmphasis(void);
int32 		WBParse_GetMPEGEndCodes(void);
int32 		WBParse_GetParserErrors(void);


#ifdef cplusplus
}
#endif /* cplusplus */

#endif /* __WBPARSE__ */
