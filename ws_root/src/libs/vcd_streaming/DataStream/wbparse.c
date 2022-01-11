/**************************************************************************************
 *  @(#) wbparse.c 96/11/26 1.5
 *
 *	File:			WBParse.c
 *
 *	Contains:		Parser for WhiteBook (VideoCD), a special case of ISO 11172-1
 *					MPEG-1 system streams.
 *
 *	Written by:		Jerry H. Morrison
 *	Modifyed by:		ARAKAWA Kay @ Matsushita Electric Industrial Co., Ltd.
 *
 *	Copyright (c)© 1994,1995, an unpublished work by The 3DO Company. All rights
 *	reserved. This material contains confidential information that is the property of
 *	The 3DO Company. Any unauthorized duplication, disclosure or use is prohibited.
 *
 * History:
 *  11/08/96    Ian		Added parsing of high-res stills.  ParerParmBlock.processStillImageType
 *						tells us which ones to process and which to discard.  (Used to always discard
 *						the high-res.)  Also new validations for high-res still sequence hdr values.
 *						New routine WBParse_SetStillImageType() so the outside world can control which
 *						type we process and which we discard.
 *	5/09/96		HPP		added WBParseInitialize to initialize the ParserParamBlock
 *	4/24/96		HPP		added static ParserParmBlock(old WBParseArgs) to our WBParser module.
 *	4/24/96		HPP		removed the reference to ParserParamBlock from status structure
 *	4/24/96		HPP		replaced the token passing with array of DSStreamSegments
 *	4/19/95		AK		Modify to call ACTION_AT_END when ParseErrorCount > 3
 *	4/03/95		AK		move the lines to ignore audio pack tail(20Byte)
 *	3/29/95		AK		ignore audio pack tail(20Byte)
 *	3/27/95		AK		trigger bit bug fix
 *	3/27/95		AK		check trigger bit
 *	3/14/95		AK		Add streamID == STREAMID_VIDEO_NORMAL_STILL
 *	1/19/95		jhm		Add WBParseIsEmpty.
 *	10/13/94	jhm		Adopt print.h .
 *	9/29/94		jhm		Rule out bum combinations of video parameters.
 *	9/23/94		jhm		Rule out audio headers with illegal-for-Whitebook parameters.
 *						Rule out bum video parameters.
 *	9/22/94		jhm		Scan into the first audio packet, if needed, to find a seq hdr. This
 *						is because the streamer doesn't always get the audio hdr before it
 *						declares the video is primed and it's ok to seek to a "start" point.
 *	9/16/94		jhm		If the first video data is all zeros, keep seeking a sequence hdr.
 *	9/14/94		jhm		If the first audio/video data doesn't start with the sequence hdr,
 *						use default parameters rather than discarding packets--supressing
 *						the audio/video--until one does.
 *	8/8/94		jhm		Shorter audio and video params printf.
 *	7/5/94		jhm		Used a PTS_TEST_OFFSET experiment to test PTS wraparound.
 *	6/28/94		jhm		Added PTS_TEST_OFFSET code (shut off) for debugging the 9-minute sync bug.
 *	6/22/94		jhm		Improve parse error messages.
 *	5/27/94		jhm		Scan past extra zeros when seeking Video Sequence Hdr.
 *	5/18/94		jhm		Added DO_OPTIONAL_DATA_CHECKS to turn off optional data checks
 *						(not many turned out to be optional). Don't miss ISO_11172_END_CODE
 *						if it barely fits at the end of the block. Minor optimizations.
 *	5/18/94		jhm		Added ERROR_VERBOSITY_LEVEL. Make videoFrameRateName et al static.
 *	5/12/94		jhm		Replaced PacketPart with Boolean DSStreamToken.packetStart .
 *						Use the union PacketStartCodes in the MPEG-1 PacketHeader.
 *						Check the VideoSequenceHeader marker bit. Trace picture width
 *						as well as size. Use symbolic const MAX_PACKET_STUFFING_BYTES.
 *	5/10/94		JHM		Changed WBParserArgs.seekingMPEGEndCode to .cntMPEGEndCodes .
 *	5/10/94		JHM		Another data error test: Audio packets should not have DTSs.
 *						Experimentally found a more efficient way to test marker bits.
 *	4/29/94		JHM		Moved MPEG structure definitions to MPEG.h.
 *	4/25/94		JHM		Watch for ISO_11172_END_CODE. Report it via seekingMPEGEndCode.
 *	4/22/94		SEH		Show block number in error message. Code for verifying test data.
 *	4/19/94		JHM		Print videoSeqHdr.bit_rate in bit/sec.
 *	3/31/94		JHM		firstSCR is now an int64.
 *  3/23/94		JHM		Print videoSeqHdr.bit_rate. Add another error check TBD.
 *						Don't care about defined(DEVELOPMENT) anymore.
 *  3/22/94		JHM		Added error checks for even buffer address and size, and for
 *						a packet that extends beyond the disc block. Added TBDs where
 *						we could add other error checks, but I doubt they'd catch many
 *						data errors and they might rule out some future disc variants.
 *  3/21/94		JHM		Make NoteParseError increment status->cntParseErrors.
 *  2/18/94		JHM		Add WBParserArgs.seekingVideoParams and tri-state semantics for
 *						WBParserArgs.seekingXxx.
 *  2/17/94		JHM		Read audio mode & emphasis from the stream. Symbolic debugging
 *						trace of audio & video parameters.
 *  2/16/94		JHM		Read video frame rate and vertical picture size from the
 *						stream. Handoff initial SCR to the streamer. Add WBParserArgs.
 *	2/11/94		JHM		Optimized ActionAtEnd -> ACTION_AT_END.
 *	2/10/94		JHM		Initial checkout & bug fixes.
 *  2/07/94		JHM		Initial version
 *
 *******************************************************************************************
 * This parser is implemented as a simple Finite State Machine (FSM).
 * It keeps its status (e.g. current parse position in the buffer) in a
 * ParserStatus struct which gets passed to the action routines.
 *
 * The current FSM state is just a pointer to an action proc. WBParse() repeatedly
 * calls the current action proc, which is responsible for:
 *   checking for the end of the input buffer (or Packet) before examining more input,
 *   looking for start codes, stream IDs, tag bits, marker bits, stuffing bytes, etc.,
 *   picking up PTSs, SCRs, and other information from the input buffer,
 *   locating Packets and packet data bytes in the buffer,
 *   perform side effects on the parser's status struct, and
 *   choosing and returning the next FSM state.
 *
 * This parser can be simple and fast because, for WhiteBook, it can assume the buffer
 * contains a whole number of MPEG Packs, so it can process a buffer and return
 * without keeping intermediate state between calls.
 *
 * [TBD] Decide how to report parse error information.
 *   Stash it in memory for examining via the debugger? Cf. NoteParseError().
 *
 *******************************************************************************************/

#include <kernel/types.h>
#include <string.h>						/* for memcpy */

#include <kernel/debug.h> 				/* for print macros: PERR, ERROR_RESULT_STATUS */
#include <video_cd_streaming/mpeg.h>
#include "wbparse.h"

/*********************************************************************
 * Debugging stuff
 *********************************************************************
 */

#define TRACE_STREAM_PARAMETERS	(1 && DEBUG)

#if TRACE_STREAM_PARAMETERS
static const char	*videoFrameRateName[16] =
	{"forbidden (!)", "23.976", "24 (!)", "25", "29.97", "30 (!)",
	"50 (!)", "59.94 (!)", "60 (!)", "reserved9 (!)", "reserved10 (!)",
	"reserved11 (!)", "reserved12 (!)", "reserved13 (!)", "reserved14 (!)",
	"reserved15 (!)"};
static const char	*audioModeName[4] =
	{"stereo", "joint_stereo", "dual_channel", "single_channel"};
static const char	*audioEmphasisModeName[4] =
	{"no", "50/15 usec", "reserved (!)", "CCITT J.17 (!)"};
#endif

/*********************************************************************
 * Local types and constants
 *********************************************************************
 */

#define DO_OPTIONAL_DATA_CHECKS			0

#define ERROR_VERBOSITY_LEVEL			0	/* <HPP> used to be 2 */

#if DEBUG && 0	/* DEBUGGING experiment: add a constant to all PTSs */
	#define PTS_TEST_OFFSET		(MPEG_CLOCK_TICKS_PER_SECOND * (60 * 7))
	#define PTS_TEST_OFFSET		((1UL << 31) - (MPEG_CLOCK_TICKS_PER_SECOND * 60) + (1UL << 31))	/* 60 secs shy of 32-bit PTS wraparound */
#endif

/* Alas, C can't handle this recursive definition. We can't declare ActionProcPtr
 * as returning an ActionProcPtr. So we need some weird hackery and many type casts
 * in this file.
 * typedef ActionProcPtr (*ActionProcPtr)(ParserStatusPtr status);
 * We can't even declare ActionProcPtr as returning a (void *), because C won't cast a
 * (void *) to a function pointer. The only thing C will cast into a function pointer
 * is another function pointer. Thus an ActionProc must return a function pointer,
 * which we can cast into an ActionProcPtr. 
 * typedef void			*(*VoidFunctionPtr)(ParserStatusPtr status);
 * typedef VoidFunctionPtr	(*ActionProcPtr)(ParserStatusPtr status);
 */

typedef struct ParserStatus	ParserStatus, *ParserStatusPtr;
 
typedef void			*(*VoidFunctionPtr)(ParserStatusPtr status);
typedef VoidFunctionPtr	(*ActionProcPtr)(ParserStatusPtr status);

struct ParserStatus {
	const char			*dataPtr;		/* current parse position in the buffer */
	const char			*bufferPtr;		/* pointer to the buffer's start */
	const char			*endPtr;		/* just beyond the buffer's end */
	ActionProcPtr		statePtr;		/* current FSM state */
	PacketHeader		packetHeader;	/* we read Packet headers into here */
	const char			*packetAddr;	/* points to a Packet start code in the buffer */
	const char			*packetEndAddr;	/* points just beyond a Packet in the buffer */
	uint32				*numOfWBChunks;	/* current number of whitebook chunks */
	DSWBChunkPtr		chunkArray;		/* array of DSWBChunk */
	int32				result;			/* WBParse result */
	int32				pts;			/* current 33-bit PTS, or -1 <HPP> changed to 32bit*/
	int8				ptsValid;		/* true if pts value are valid or not */
};

/* Parser error codes */
#define ERR_MARKER_BITS_OFF											-1
#define ERR_EXPECTED_PACK_START_CODE								-2
#define ERR_EXPECTED_PACKET_START_CODE								-3
#define ERR_EXPECTED_PACKET_STREAM_ID								-4
#define ERR_PACKET_EXTENDS_BEYOND_BUFFER							-5
#define ERR_STD_BUFFER_INFO_EXTENDS_BEYOND_PACKET					-6
#define ERR_PTS_INFO_EXTENDS_BEYOND_PACKET							-7
#define ERR_UNDEFINED_PTS_DTS_TAG									-8
#define ERR_EXPECTED_TO_BE_INSIDE_A_PACKET							-9
#define ERR_STUFFING_EXTENDS_BEYOND_PACKET							-10
#define ERR_ODD_ADDRESS_OR_SIZE_DATA								-11
#define ERR_PACKET_EXTENDS_BEYOND_BLOCK								-12
#define	ERR_AUDIO_PACKET_HAS_DTS									-13
#define LAST_PARSER_ERROR_CODE					ERR_AUDIO_PACKET_HAS_DTS

#if (ERROR_VERBOSITY_LEVEL > 0) && DEBUG
static const char	*parseErrorMsgs[-LAST_PARSER_ERROR_CODE] =
	{"marker bits off",
	"expected a pack start code",
	"expected a packet start code",
	"expected a packet stream ID",
	"packet extends beyond buffer",
	"STD buffer info extends beyond packet",
	"PTS info extends beyond packet",
	"undefined PTS/DTS tag",
	"expected to be inside a packet",
	"stuffing extends beyond packet",
	"odd address or size data",
	"packet extends beyond the block",
	"audio packet has a DTS"};
#endif

typedef struct ParseErrorSnapshot {
	int32			errorCode;
	const char		*dataPtr;
	ActionProcPtr	statePtr;
} ParseErrorSnapshot, *ParseErrorSnapshotPtr;

/*********************************************************************
 * File-local variables
 *********************************************************************/

static ParseErrorSnapshot	parseErrorSnapshot;
static ParserParamBlock	 	parserPBlock;

/*********************************************************************
 * Forward procedure declarations
 *********************************************************************/

static ActionProcPtr ActionSkipToNextBlock(ParserStatusPtr status);
static ActionProcPtr ActionCheckPackStart(ParserStatusPtr status);
static ActionProcPtr ActionPackets(ParserStatusPtr status);
static ActionProcPtr ActionSkipToNextPacket(ParserStatusPtr status);
static ActionProcPtr ActionPacketInfo(ParserStatusPtr status);
static ActionProcPtr ActionPacketData(ParserStatusPtr status);

#define ACTION_AT_END	((ActionProcPtr)NULL)

static int32 SubHeader; /* added by <AK> to check trigger bit & duplicate still image */
/*********************************************************************
 * Local procedures and macros
 *********************************************************************/

#if DEBUG
static uint32 	ownCtCalls	= 0;
#endif
int ParseErrorCount; /*<AK>bugs?*/
/*********************************************************************
 * Note a parse error. This currently reports the error via kprintf and by stashing
 * the info in parseErrorSnapshot.
 * [TBD] Keep an array of ParseErrorSnapshots in RAM?
 * This is not an action routine, but it does return the next action routine to
 * use: ActionSkipToNextBlock.
 *********************************************************************/
static ActionProcPtr NoteParseError(ParserStatusPtr status, int errorCode) {
	parserPBlock.cntParseErrors++;
	
	parseErrorSnapshot.errorCode = errorCode;
	parseErrorSnapshot.dataPtr = status->dataPtr;
	parseErrorSnapshot.statePtr = status->statePtr;

	/* verbosity level 0: no kprintfs.
	 * verbosity level 1: print most messages, filter out some msgs (obsolete feature).
	 * verbosity level 2: print all messages. */
#if (ERROR_VERBOSITY_LEVEL > 0) && DEBUG
  #if ERROR_VERBOSITY_LEVEL <= 1
	/* if ( errorCode != some_error_code_to_filter_out... ) */
  #endif
		PERR(("WBParse[%d] error: %s\n", ownCtCalls,
			parseErrorMsgs[-1 - errorCode]));
#endif	/* (ERROR_VERBOSITY_LEVEL > 0) && DEBUG */
#define PARSE_ERROR_THRE 1
	if (ParseErrorCount++ > PARSE_ERROR_THRE) return ACTION_AT_END;/*<AK>bugs?*/
	return (ActionProcPtr)&ActionSkipToNextBlock;
}

/*********************************************************************
 * Compute the address of the next block in the buffer.
 *********************************************************************/
#define NEXT_BLOCK_ADDR(status)	((status)->bufferPtr +	\
	(((uint32)((status)->dataPtr - (status)->bufferPtr)	\
	/ BYTES_PER_WHITEBOOK_BLOCK + 1) * BYTES_PER_WHITEBOOK_BLOCK))


/*********************************************************************
 * Inform the client that we found an ISO_11172_END_CODE.
 * RETURNS the next ActionProcPtr, which is &ActionSkipToNextBlock.
 *********************************************************************/
static ActionProcPtr NoteEndCode(ParserStatusPtr status) {
	TOUCH(status);
	++parserPBlock.cntMPEGEndCodes;
	return (ActionProcPtr)&ActionSkipToNextBlock;	
}


/*********************************************************************
 * Scan from *firstByte to *lastByte, inclusive, looking for the first non-zero byte.
 * This is useful as part of a scan for an MPEG start code.
 *
 * RETURNS: The address just past the last zero data byte. The result is in the
 * range [firstByte .. lastByte + 1]. If firstByte > lastByte (an empty range),
 * this returns lastByte + 1.
 *
 * IMPLEMENTATION: This uses a word-at-a-time scan and the old sentinal trick
 *    to speed the bulk of the search. This could be even faster via move-multiple
 *    and loop unrolling.
 **********************************************************************/

static char *ScanPastZeros(char *firstByte, char *lastByte) {
	register uint32	*wordPtr;
	register uint32	word;
	register char	*bytePtr;
	char			savedByte;
	
	/* Check for an erroneous range, so we don't search unbounded */
	if (firstByte > lastByte)
		return lastByte + 1;
	
	/* Store a non-zero sentinal byte to stop the search */
	savedByte = *lastByte;
	*lastByte = 0xFF;
	
	/* Scan for a non-zero byte, up to the first mod 4 boundary */
	for (bytePtr = firstByte; (uint32)bytePtr & 3; ++bytePtr)
		if (*bytePtr != 0)
			goto Eureka;
	
	/* The main loop: a word-at-a-time scan for a non-zero byte */
	wordPtr = (uint32 *)bytePtr;
	do {} while ((word = *wordPtr++) == 0);
		
	/* Backup to find the first non-zero byte in the first non-zero word:
	 * Set bytePtr to just beyond the last word picked up (word), then back
	 * it up once for each tail-end non-zero byte in word, at least once. */
	for (bytePtr = (char *)wordPtr;  word != 0;  word >>= 8, --bytePtr);
	
Eureka:	/* bytePtr now points to the first non-zero byte found */

	/* Restore the sentinal byte, and compensate if that changes the answer */
	*lastByte = savedByte;
	if (bytePtr == lastByte && savedByte == 0)
		++bytePtr;
		
	return bytePtr;
}


/*********************************************************************
 * Return TRUE iff the argument (which must be of type ) looks like a valid
 * MPEG-1 audio header that's legal in ***Whitebook***. Something that looks
 * like a header with non-Whitebook parameters is probably not a header at all,
 * or a smashed one. As it is, we can be fooled by audio data that isn't a header
 * at all. So it seems more prudent to reject the entire sequence header and
 * look for another--and maybe end up using default parameters--than to accept
 * a dubious one and overrule the illegal parameters in it.
 *
 * IMPLEMENTATION: This is written the easy, modifiable way. It could be faster:
 *    Take the 32 bit word, mask off the variable bits, and check the fixed bits.
 **********************************************************************/
#define IS_VALID_AUDIO_HEADER(hdr)	(  (hdr).syncword == MPEG_AUDIO_SYNCWORD		\
									&& (hdr).id == ID_ISO_11172_3_AUDIO				\
									&& (hdr).layer == audioLayer_II					\
									&& (hdr).sampling_frequency == aFreq_44100_Hz	\
									&& (hdr).mode != aMode_single_channel			\
									&& ((hdr).emphasis == aEmphasisMode_none		\
										|| (hdr).emphasis == aEmphasisMode_50_15) )


/*********************************************************************
 * Scan from [*firstByte .. *endByte), inclusive of firstByte and exclusive of endByte,
 * looking for a possible Audio header.
 * An audio header can appear at any byte address in the data.
 *
 * RETURNS: The address of what we hope is a real Audio header, else NULL.
 *
 * SIDE EFFECT: If this finds an audio header, it copies the header into *audioHdr.
 *
 * IMPLEMENTATION: This could be faster if it used an inner loop with a sentinal to
 * search for an 0xFF byte, and an outer loop to check those potential headers. But
 * we shouldn't have to search all that far. Hopefully most of the time we'll find the
 * audio header right up front in the very first audio packet. */
static const char *ScanForAudioHeader(const char *firstByte, const char *endByte,
		AudioHeader *audioHdr) {
	register const char	*bytePtr, *lastHdrBytePtr;
	AudioHeader			localAudioHdr;	/* a word-aligned copy */
	
	/* Figure the last possible place where an audio header could begin in the data. */
	lastHdrBytePtr = endByte - AUDIO_HEADER_CONTENT_SIZE;
	
	/* Scan for a possible audio header.
	 * If the first byte is correct, copy the rest to align it and check it. */
	for (bytePtr = firstByte; bytePtr <= lastHdrBytePtr; ++bytePtr)
		if ( *bytePtr == MPEG_AUDIO_SYNCWORD_BYTE_1
			&& (memcpy((char *)&localAudioHdr, bytePtr, AUDIO_HEADER_CONTENT_SIZE),
					IS_VALID_AUDIO_HEADER(localAudioHdr)) )
			{
			/* Eureka. */
			*audioHdr = localAudioHdr;
			return bytePtr;
			}
	
	/* There are no audio headers in this data. */
	return NULL;
}


/*********************************************************************
 * Action procedures
 *********************************************************************
 */

#if 0 && DEBUG	/* experiment: which generates the best code for testing marker bits? */

/* Is there a way to code it so the compiled code will test multiple marker bits en
 * mass with bitand? Considering the number of steps required to generate a long
 * constant, would a mass bitand be better anyway? */

/* Test1 generated 56 bytes of object code, using ARMCC 1.6.1v8 with -gf -Otime */
static ActionProcPtr Test1(PackHeaderPtr ph) {
	if (!ph->mb0 || !ph->mb1 || !ph->mb2 || !ph->mb3 || !ph->mb4)
		return ACTION_AT_END;
	return (ActionProcPtr)&ActionSkipToNextBlock;
}

/* Test2 generated 100 bytes of object code, using ARMCC 1.6.1v8 with -gf -Otime */
static ActionProcPtr Test2(PackHeaderPtr ph) {
	if (!ph->mb0 | !ph->mb1 | !ph->mb2 | !ph->mb3 | !ph->mb4)
		return ACTION_AT_END;
	return (ActionProcPtr)&ActionSkipToNextBlock;
}

/* Test3 generated 44 bytes of object code, using ARMCC 1.6.1v8 with -gf -Otime */
static ActionProcPtr Test3(PackHeaderPtr ph) {
	if (!(ph->mb0 & ph->mb1 & ph->mb2 & ph->mb3 & ph->mb4))
		return ACTION_AT_END;
	return (ActionProcPtr)&ActionSkipToNextBlock;
}

/* Test4 generated 56 bytes of object code, using ARMCC 1.6.1v8 with -gf -Otime */
static ActionProcPtr Test4(PackHeaderPtr ph) {
	if (!(ph->mb0 && ph->mb1 && ph->mb2 && ph->mb3 && ph->mb4))
		return ACTION_AT_END;
	return (ActionProcPtr)&ActionSkipToNextBlock;
}

#endif


/*********************************************************************
 * Skip to next packet and drop into ActionCheckPackStart.
 */
static ActionProcPtr ActionSkipToNextBlock(ParserStatusPtr status) {
	status->dataPtr = NEXT_BLOCK_ADDR(status);
	return (ActionProcPtr)&ActionCheckPackStart;
}

#define SEH_DEBUG 0&&DEBUG

/*********************************************************************
 * Check for a Pack start. This is the parser's initial state.
 */
static ActionProcPtr ActionCheckPackStart(ParserStatusPtr status) {
	const PackHeader	*ph;
	
	/* skip to next long-word boundary */
	status->dataPtr = (const char*)(((uint32)status->dataPtr + 3) & ~3);

	if (status->dataPtr + PACK_HEADER_CONTENT_SIZE > status->endPtr)
		return ACTION_AT_END;
	
	ph = (PackHeaderPtr)status->dataPtr;
	if (ph->packStartCode == PACK_START_CODE) {
#if READ_TRIGGER_BIT && 1		
		if ( ( (SubHeader=*(int32 *)(status->dataPtr+4))
					&TRIGGER_BIT_MASK ) == TRIGGER_BIT ) {
			PERR(("t!\n"));
		}
#endif		
	    status->dataPtr += PACK_HEADER_CONTENT_SIZE;
		if (ph->b0010 != 0x2 ||
			!(ph->mb0 & ph->mb1 & ph->mb2 & ph->mb3 & ph->mb4))
			return NoteParseError(status, ERR_MARKER_BITS_OFF);
		
		if (parserPBlock.seekingAnSCR == seeking_info) {
			/* assemble & emit the first system clock reference (SCR) */
			parserPBlock.firstSCR = ((uint32)ph->scr31_30 << 30	|
									 (uint32)ph->scr29_15 << 15	|
									 (uint32)ph->scr14_7  <<  7	|
									 (uint32)ph->scr6_0); /* Bit 32 is ignored | ((uint32)ph->scr32_32 << 32); */
			parserPBlock.seekingAnSCR = seeking_foundInfo;
		}
		/* [TBD] #if DO_OPTIONAL_DATA_CHECKS, check the SCR (In Whitebook, it
		 * should be the previous block's SCR + 1200 and ph->muxRate (In
		 * Whitebook, it should be 3528). */
		
		return (ActionProcPtr)&ActionPackets;
	}
	
	if (ph->packStartCode == 0)
		{
#if DEBUG
		static int	marginCnt;
		++marginCnt;
#endif
		return (ActionProcPtr)&ActionSkipToNextBlock;	/* FrontMargin or RearMargin */
		}
	
	/* NOTE: It takes an "audio end-code" AND a "video end-code" to indicate
	 * end-of-stream in Whitebook spec, not per MPEG-1 spec. */
	if (ph->packStartCode == ISO_11172_END_CODE)
		return NoteEndCode(status);
	/*950612<AK>NOTE: If Channel Number in SubHeader is 0, the Sector is Padding data in WB spec.*/
#if READ_TRIGGER_BIT
	if ( (ph->h.subHeader&0xff0000)==0 ) {
		return (ActionProcPtr)&ActionSkipToNextBlock;	/* Padding data */
	}
#endif
	return NoteParseError(status, ERR_EXPECTED_PACK_START_CODE);
}


/*********************************************************************
 * Process a Packet. The parser loops around to this state to process more.
 */
static ActionProcPtr ActionPackets(ParserStatusPtr status) {
	register const char	*dataPtr = status->dataPtr;	/* [TBD] Does this help generate better code? */
	PacketHeader	*ph;
	uint32			startCodePrefix;
	const char		*nextBlockAddr;
	PacketStartCodes	possibleEndCode;

	/* If there aren't enough bytes left for a PacketHeader, test for an ISO 11172
	 * end-code. We normally catch end-codes below. The test here handles the case
	 * that can't be tested below, and the test below saves the extra steps here
	 * most of the time. */
	if (dataPtr + PACKET_HEADER_CONTENT_SIZE > status->endPtr) {
	
		/* NOTE: It takes an "audio end-code" AND a "video end-code" to indicate
		 * end-of-stream in Whitebook spec, not per MPEG-1 spec. */
		if (dataPtr + sizeof(PacketStartCodes) <= status->endPtr) {
			memcpy((char *)&possibleEndCode, dataPtr, sizeof(PacketStartCodes));
			if (possibleEndCode.packetStartCode32 == ISO_11172_END_CODE)
				return NoteEndCode(status);
		}
		
		/* [TBD] #if DO_OPTIONAL_DATA_CHECKS, raise an error if the bytes between
		 * *dataPtr and *status->endPtr aren't all zeros. */
		
		return ACTION_AT_END;
		}
	ph = &status->packetHeader;
	memcpy((char *)ph, dataPtr, PACKET_HEADER_CONTENT_SIZE); /* copy the data
			in order to work with a long-word aligned struct */
			#if ARA_DEBUG
			PERR(("[%08x]%x",ph->start.packetStartCode32,ph->start.codes.streamID));{static int i;if(i++>20)i=0,PERR(("\n"));}/*arac.ins*/
			#endif
	if ((startCodePrefix = ph->start.codes.packetStartCodePrefix)
			== PACKET_START_CODE_PREFIX24) {
		uint32	streamID = ph->start.codes.streamID;
		
		if (streamID == STREAMID_PACK_START_CODE)
			return (ActionProcPtr)&ActionCheckPackStart;
		
		/* NOTE: It takes an "audio end-code" AND a "video end-code" to indicate
		 * end-of-stream in Whitebook spec, not per MPEG-1 spec. */
		if (streamID == STREAMID_ISO_11172_END_CODE)
			return NoteEndCode(status);
		
		if (streamID < STREAMID_SYSTEM_HEADER)
			return NoteParseError(status, ERR_EXPECTED_PACKET_STREAM_ID);
		
		status->packetAddr = dataPtr;
		nextBlockAddr = NEXT_BLOCK_ADDR(status);
		status->dataPtr = dataPtr += PACKET_HEADER_CONTENT_SIZE;
		status->packetEndAddr = dataPtr + ph->packetLength;

		if (status->packetEndAddr > nextBlockAddr)
			return NoteParseError(status, ERR_PACKET_EXTENDS_BEYOND_BLOCK);
		
		if (status->packetEndAddr > status->endPtr)
			return NoteParseError(status, ERR_PACKET_EXTENDS_BEYOND_BUFFER);
		
		if (streamID == STREAMID_AUDIO0
		    || streamID == STREAMID_VIDEO0
		    || streamID == parserPBlock.processStillImageType)
		{
			/* Found a data Packet that we care about */
			
			/* First, skip the stuffing bytes, 0 to MAX_PACKET_STUFFING_BYTES of them. */
			/* Temporarily store a sentinal value in the buffer. This saves scanning */
			/* time (testing dataPtr against packetEndAddr in the loop) if the */
			/* average number of stuffing bytes is >= 1. */
			register char	*shouldntBeStuffing =
				(char *)dataPtr + MAX_PACKET_STUFFING_BYTES;
			char	savedChar;
			
			if (shouldntBeStuffing >= status->packetEndAddr)
				shouldntBeStuffing = (char *)status->packetEndAddr - 1;
				/* must be at least 1 byte left in the Packet (null PTS/DTS tag) */
			
			savedChar = *shouldntBeStuffing;
			*shouldntBeStuffing = 0;				/* use a sentinal to stop the loop */
			do {} while (*dataPtr++ == STUFFING_BYTE);
			status->dataPtr = --dataPtr;
			*shouldntBeStuffing = savedChar;		/* restore the saved char */
			
			if (dataPtr == shouldntBeStuffing && savedChar == STUFFING_BYTE)
				return NoteParseError(status, ERR_STUFFING_EXTENDS_BEYOND_PACKET);
			
			/* The next action routines will extract Packet info and data bytes */
			return (ActionProcPtr)&ActionPacketInfo;
		}

		return (ActionProcPtr)&ActionSkipToNextPacket;	/* skip any SystemHeader or
			* any other Packet (reserved, private, padding, or other audio or video
			* stream ID).
			* We COULD consider some of these as errors, but we're skipping to the
			* next packet anyway, which usually entails skipping to the next block,
			* so the net effect is similar.
			* [TBD] Error-check the contents of SystemHeader packets? */
	}
	
	if (startCodePrefix == 0)
		return (ActionProcPtr)&ActionSkipToNextBlock;	/* 0 padding at end of Pack */
	
	return NoteParseError(status, ERR_EXPECTED_PACKET_START_CODE);
}


/*********************************************************************
 * Skip to the next Packet. This works anywhere within a Packet.
 * ASSUMES: We already checked if the Packet extends beyond the buffer.
 */
static ActionProcPtr ActionSkipToNextPacket(ParserStatusPtr status) {
	if (status->packetEndAddr == NULL)
		return NoteParseError(status, ERR_EXPECTED_TO_BE_INSIDE_A_PACKET);
	
	status->dataPtr = status->packetEndAddr;

	status->packetAddr = status->packetEndAddr = NULL;
#if READ_TRIGGER_BIT
		{
		    int32 gap;
		    if ( ( gap = (status->dataPtr-status->bufferPtr)%BYTES_PER_WHITEBOOK_BLOCK )==0 ){
		        if ( (*(int32 *)(status->dataPtr+4)&TRIGGER_BIT_MASK) == TRIGGER_BIT ) PERR(("T!\n"));/*bugs*/
		        #define ISO_1172_END_CODE_UPPER_BYTE (0x01)
		        #define AUDIO_PADDING_BYTE (20)
		        /*STREAMID_ISO_11172_END_CODE == 0xb9    cf. mpeg.h*/
			    return (ActionProcPtr)&ActionCheckPackStart;		    
		    } else if ( gap==(BYTES_PER_WHITEBOOK_BLOCK-AUDIO_PADDING_BYTE) && 
				status->packetHeader.start.codes.streamID == STREAMID_AUDIO0 ) {
		        status->packetEndAddr += AUDIO_PADDING_BYTE;		            
			    return (ActionProcPtr)&ActionCheckPackStart;		    
		    } 
		    #if 0
			else if ( gap == 2332 ) {
		        status->dataPtr += (4);
		        return (ActionProcPtr)&ActionCheckPackStart;		        
		    }
		    #endif
		    else 
		    {
		        ;
		    }
		}
#endif		
	return (ActionProcPtr)&ActionPackets;
}


/*********************************************************************
 * Process the optional System info in the Packet.
 * ASSUMES: We already checked that the Packet doesn't extend beyond the buffer.
 * NOTE: This has to examine some tags before knowing how many bytes to move forward
 *   in the data, so it goes ahead and copies from the buffer, tests the tags, and
 *   then checks if the variable-length structs actually fit within the buffer. This
 *   is a simple approach, but it does mean it's possible to peek a few bytes beyond
 *   the buffer's end. But the Packet length would have to be wacky to do that, and
 *   peeking beyond the buffer end shouldn't cause any problems. */
static ActionProcPtr ActionPacketInfo(ParserStatusPtr status) {
	STDBufferInfo	stdBufferInfo;
	PTS_Info		ptsInfo;
	uint32			ptsInfoSize;	/* # bytes in this Packet's PTS info variant */

	if (status->packetEndAddr == NULL)
		return NoteParseError(status, ERR_EXPECTED_TO_BE_INSIDE_A_PACKET);
	
	/* Process the optional STDBufferInfo. */
	memcpy((char *)&stdBufferInfo, status->dataPtr, STD_BUFFER_INFO_CONTENT_SIZE);
		/* Copy the data to ensure that it's long-word aligned. */
		/* Check the length after we find out if it's present. */
	
	if (stdBufferInfo.b01 == 0x1) {
		if (status->dataPtr + STD_BUFFER_INFO_CONTENT_SIZE > status->packetEndAddr)
			return NoteParseError(status, ERR_STD_BUFFER_INFO_EXTENDS_BEYOND_PACKET);
		status->dataPtr += STD_BUFFER_INFO_CONTENT_SIZE;
		/* [TBD] #if DO_OPTIONAL_DATA_CHECKS, check stdBufferInfo.stdBufferScale,
		 * .stdBufferSize against Whitebook requirements. */
	}
	/* else this Packet doesn't have STDBufferInfo */
	
	/* Process the optional PTS/PTS+DTS info (a union with 3 variants). */
	memcpy((char *)&ptsInfo, status->dataPtr, sizeof(PTS_Info));
		/* Copy the data to ensure that it's long-word aligned. */
		/* Check the length after we find out which variant it is. */
	
	status->ptsValid = FALSE;		/* <HPP> initially assume no PTS: pts->hi = pts->lo = (uint32)-1; */
	if (ptsInfo.pts.tag0x2 == 0x2) {
		if (!(ptsInfo.pts.mb0 & ptsInfo.pts.mb1 & ptsInfo.pts.mb2))
			return NoteParseError(status, ERR_MARKER_BITS_OFF);
		status->ptsValid = TRUE;	/* <HPP> indicate we got a valid pts */
		status->pts = ((uint32)ptsInfo.pts.pts31_30 << 30	| 	/* <HPP> Bit32 is ignored ((uint32)ptsInfo.pts.pts32_32 << 32)	| */
					   (uint32)ptsInfo.pts.pts29_15 << 15	|
					   (uint32)ptsInfo.pts.pts14_7  <<  7	|
					   (uint32)ptsInfo.pts.pts6_0);
		ptsInfoSize = PTS_INFO_PTS_CONTENT_SIZE;
#ifdef PTS_TEST_OFFSET				/* offset the pts variable and the packet's PTS */
		status->pts += PTS_TEST_OFFSET;	/* don't bother with any carry to pts->hi */
		ptsInfo.pts.pts31_30 = (status->pts && 0x0FFFFFFFF) >> 30;
		ptsInfo.pts.pts29_15 = (status->pts && 0x0FFFFFFFF) >> 15;
		ptsInfo.pts.pts14_7  = (status->pts && 0x0FFFFFFFF) >> 7;
		ptsInfo.pts.pts6_0   = (status->pts && 0x0FFFFFFFF);
		memcpy((char *)status->dataPtr, (char *)&ptsInfo, PTS_INFO_PTS_CONTENT_SIZE);
#endif
	}
	else if (ptsInfo.pts_dts.tag0x3 == 0x3) {
#if DO_OPTIONAL_DATA_CHECKS
		if (!(ptsInfo.pts_dts.mb0 & ptsInfo.pts_dts.mb1 &
				ptsInfo.pts_dts.mb2 & ptsInfo.pts_dts.mb3 &
				ptsInfo.pts_dts.mb4 & ptsInfo.pts_dts.mb5))
			return NoteParseError(status, ERR_MARKER_BITS_OFF);
		if ( (status->packetHeader.start.codes.streamID >= STREAMID_AUDIO0) &&
			 (status->packetHeader.start.codes.streamID <= STREAMID_AUDIO_LAST) )
			return NoteParseError(status, ERR_AUDIO_PACKET_HAS_DTS);
#endif
		status->ptsValid = TRUE;	/* <HPP> indicate we got a valid pts value */
		status->pts = ((uint32)ptsInfo.pts_dts.pts31_30 << 30	| /* <HPP> ignore bit32 ((uint32)ptsInfo.pts_dts.pts32_32) << 32	| */
				(uint32)ptsInfo.pts_dts.pts29_15  << 15	|
				(uint32)ptsInfo.pts_dts.pts14_7	  <<  7	|
				(uint32)ptsInfo.pts_dts.pts6_0);
		ptsInfoSize = PTS_INFO_PTS_DTS_CONTENT_SIZE;
		/* [TBD] Check the DTS for a reasonable value, to help find data errors */
#ifdef PTS_TEST_OFFSET
		status->pts += PTS_TEST_OFFSET;	/* don't bother with any carry to pts->hi */
		#if 0
		pts->lo += PTS_TEST_OFFSET;	/* don't bother with any carry to pts->hi */
		/* We don't need to change the PTS in the data because any packet with a DTS
		 * must be a video PTS, where the pts variable is all that matters. */
		#endif
#endif
	}
	else if (ptsInfo.other.tag0x0F == 0xF)
		ptsInfoSize = PTS_INFO_OTHER_CONTENT_SIZE;
	else
		return NoteParseError(status, ERR_UNDEFINED_PTS_DTS_TAG);
	
	if (status->dataPtr + ptsInfoSize > status->packetEndAddr)
		return NoteParseError(status, ERR_PTS_INFO_EXTENDS_BEYOND_PACKET);
	
	status->dataPtr += ptsInfoSize;
	
	return (ActionProcPtr)&ActionPacketData;
}


/*********************************************************************
 * Process the actual Packet data: Enqueue an audio or video DS stream token.
 * If requested, pull audio & video stream parameters from the data.
 * ASSUMES: We already checked that the Packet doesn't extend beyond the buffer.
 */
static ActionProcPtr ActionPacketData(ParserStatusPtr status) {
	int32				packetDataSize;
	const char			*buffer;
	int32				bufferSize;
	AudioHeader			audioHdr;
	uint32				wbchunkNumber;

	if (status->packetEndAddr == NULL)
		return NoteParseError(status, ERR_EXPECTED_TO_BE_INSIDE_A_PACKET);
	
	/* Locate the address of the data buffer to send to the subscriber. */
	/* For audio, it's packet_data. For video, it's also the packet_data. */
	/* Dive into the first audio & video packet(s) for parameters, if requested. */
	packetDataSize = status->packetEndAddr - (char *)status->dataPtr;
	if (status->packetHeader.start.codes.streamID == STREAMID_AUDIO0) {
		buffer = status->dataPtr;			/* <WHSU> buffer = status->packetAddr */
		bufferSize = status->packetEndAddr - buffer;
		
		/* If we're seeking audio parameters, try to get them from a valid sequence
		 * header, HOPEFULLY at the start of the data.
		 * NOTE: If we must scan thru the data, we could be fooled by non-header data! */
		if (parserPBlock.seekingAudioParameters == seeking_info) {
			
			if ( ScanForAudioHeader(status->dataPtr, status->packetEndAddr, &audioHdr) )
				{
				parserPBlock.audioMode = audioHdr.mode;
				parserPBlock.audioEmphasis = audioHdr.emphasis;

#if TRACE_STREAM_PARAMETERS
				PRNT(("WBParse: Audio %s, %s emphasis\n",
					audioModeName[parserPBlock.audioMode],
					audioEmphasisModeName[parserPBlock.audioEmphasis]));
#endif
				}
			else
				{
				/* Didn't find a valid audio sequence header in the data.
				 * Use default parameters and get on with playback. */
				parserPBlock.audioMode = aMode_stereo;
				parserPBlock.audioEmphasis = aEmphasisMode_none;

				PERR(("WBParse: first audio packet didn't have "
					"a valid sequence header; assuming stereo, no emphasis\n"));
				}
			
			parserPBlock.seekingAudioParameters = seeking_foundInfo;
		}
	}
	else {	/* streamID must be STREAMID_VIDEO0 */
		buffer = status->dataPtr;
		bufferSize = packetDataSize;
		
		/* If we're seeking video parameters, try to get them from a valid sequence
		 * header at the start of the data. */
		if (parserPBlock.seekingVideoParams == seeking_info) {
			char				*nonZeroPtr;
			int32				nonZeroOffset;
			
			/* Look for a valid video sequence header after optional extra zero-bytes.
			 * Requirements: at least 2 zero bytes, and enough bytes left for a video
			 * sequence header, and the right start code. Also check the marker bit. */
			nonZeroPtr = ScanPastZeros((char *)buffer, (char *)buffer + bufferSize - 1);
			nonZeroOffset = nonZeroPtr - buffer;
			if ( (nonZeroOffset >= 2)
				&& (nonZeroOffset + (VIDEO_SEQUENCE_HEADER_CONTENT_SIZE - 2) <= bufferSize)
					/* now copy the data to word-align it: */
				&& (memcpy((char *)&parserPBlock.videoSeqHdr, nonZeroPtr - 2,
						VIDEO_SEQUENCE_HEADER_CONTENT_SIZE),
					parserPBlock.videoSeqHdr.sequenceHeaderCode == MPEG_VIDEO_SEQUENCE_HEADER_CODE) )
				{
				if (!parserPBlock.videoSeqHdr.mb0)
					return NoteParseError(status, ERR_MARKER_BITS_OFF);
				
				parserPBlock.videoFrameRate = parserPBlock.videoSeqHdr.picture_rate;
				parserPBlock.videoVerticalSize = parserPBlock.videoSeqHdr.vertical_size;
				parserPBlock.seekingVideoParams = seeking_foundInfo;

#if TRACE_STREAM_PARAMETERS
				PRNT(("WBParse: Video %ldw x %ldh @ %s Hz, %ld bit/sec%s\n",
					parserPBlock.videoSeqHdr.horizontal_size,
					parserPBlock.videoVerticalSize,
					videoFrameRateName[parserPBlock.videoFrameRate],
					parserPBlock.videoSeqHdr.bit_rate * 400,
					(parserPBlock.videoSeqHdr.bit_rate == 0x3FFFF) ? " (=> variable)" : ""));
#endif

				/* Restrict the video seq parameters to Whitebook-legal combinations:
				 *    352w x 240h @ 29.97 Hz  (NTSC video),
				 *    352w x 240h @ 23.976 Hz (NTSC film)
				 *    352w x 288h @ 25 Hz     (PAL video). 
				 *	  704w x 480h             (NTSC high-res still).
				 *    704h x 576h             (PAL high-res still).
				 */

				if (status->packetHeader.start.codes.streamID == STREAMID_VIDEO_HIGH_STILL) 
					{
						if (parserPBlock.videoVerticalSize != 480 && parserPBlock.videoVerticalSize != 576)
						 	{
							if ( parserPBlock.videoFrameRate == vfr_29_97)
								{
								parserPBlock.videoVerticalSize = 480;
								PERR(("WBParse: bum video vertical size; using 480 based on frame rate\n"));
								}
							else if (parserPBlock.videoFrameRate == vfr_25 )
								{
								parserPBlock.videoVerticalSize = 576;
								PERR(("WBParse: bum video vertical size; using 576 based on frame rate\n"));
								}
							else
								{
								parserPBlock.videoVerticalSize = 480;
								parserPBlock.videoFrameRate = vfr_29_97;	/* a guess */
								PERR(("WBParse: bum video vertical size and frame rate; guessing 480 @ 29.97 Hz\n"));
								}
							}
					}
				else if ( parserPBlock.videoVerticalSize == 240 )
					{	/* Good height. Use it to validate the frame rate. */
					if ( parserPBlock.videoFrameRate != vfr_29_97
						&& parserPBlock.videoFrameRate != vfr_23_976 )
						{
						parserPBlock.videoFrameRate = vfr_29_97;	/* a guess */
						PERR(("WBParse: bum video frame rate; using 29.97 Hz\n"));
						}
					}
				else if ( parserPBlock.videoVerticalSize == 288 )
					{	/* Good height. Use it to validate the frame rate. */
					if ( parserPBlock.videoFrameRate != vfr_25 )
						{
						parserPBlock.videoFrameRate = vfr_25;
						PERR(("WBParse: bum video frame rate; using 25 Hz\n"));
						}
					}
				else if ( parserPBlock.videoFrameRate != vfr_23_976
						&& parserPBlock.videoFrameRate != vfr_25
						&& parserPBlock.videoFrameRate != vfr_29_97 )
					{	/* Bum height and frame rate. */
					parserPBlock.videoFrameRate = vfr_29_97;		/* a guess */
					parserPBlock.videoVerticalSize = 240;
					PERR(("WBParse: bum video parms; using 240h @ 29.97 Hz\n"));
					}
				else
					{	/* Bum height. Good frame rate. Use it to derive a height. */
					parserPBlock.videoVerticalSize =
						(parserPBlock.videoFrameRate == vfr_25) ? 288 : 240;
					PERR(("WBParse: bum video height; using %dh\n",
						parserPBlock.videoVerticalSize));
					}
				}
			else if ( nonZeroOffset == bufferSize )
				{
				/* The video packet data is all zeros. Keep seeking video params.
				 * NOTE: We could skip the data (don't enqueue a token) if we were
				 *    100% sure that the last 2 zeros aren't the first two bytes of
				 *    a start code. They shouldn't be, but it's not worth assuming. */
				}
			else
				{
				/* Didn't find a video sequence header at the first non-zero data bytes.
				 * Use default values and get on with playback.  Since we have no idea
				 * whether we've got NTSC or PAL source at this point, we'll assume PAL-
				 * sized source.  This will cause the video subscriber to set the MPEG
				 * decoder device to 288 or 576, and then if we have NTSC-size source
				 * we're fine, the buffers just won't get completely filled.  The VideoCD
				 * app will know for sure what the source data looks like, and will scale
				 * things accordingly.  (If we assumed NTSC sizes then actually get PAL-size
				 * data, the bottom of the PAL images will get truncated.)
				 * BTW: all this stuff really happens.  When the VideoCD app starts a stream
				 * at some non-zero offset, we don't get to see the real stream header.
				 */
				if (status->packetHeader.start.codes.streamID == STREAMID_VIDEO_HIGH_STILL) 
					{
					parserPBlock.videoVerticalSize = 576;
					} 
				else 
					{
					parserPBlock.videoVerticalSize = 288;
					}
				
				parserPBlock.videoFrameRate = vfr_29_97;
				parserPBlock.seekingVideoParams = seeking_foundInfo;

				PERR(("WBParse: first video packet didn't start with "
					"a valid sequence header; assuming vSize=%d @ 29.97 Hz\n", parserPBlock.videoVerticalSize));
				}
		}
	}

	/* Check for even buffer address and size (we need it only for video) */
	if( status->packetHeader.start.codes.streamID != STREAMID_AUDIO0 )
		if ( ((uint32)buffer & 1) | (bufferSize & 1) )
			return NoteParseError(status, ERR_ODD_ADDRESS_OR_SIZE_DATA);

	/* Here we are going to set our whitebook stream chunk */
	
	wbchunkNumber = *status->numOfWBChunks;
	
	status->chunkArray[wbchunkNumber].buffer = buffer;
	status->chunkArray[wbchunkNumber].bufferSize = bufferSize;
	status->chunkArray[wbchunkNumber].pts = status->pts;
	status->chunkArray[wbchunkNumber].ptsValid = status->ptsValid;
	status->chunkArray[wbchunkNumber].chunkType = 
		(status->packetHeader.start.codes.streamID == STREAMID_AUDIO0) ? MPAU_CHUNK_TYPE : MPVD_CHUNK_TYPE;
	
	#if READ_TRIGGER_BIT 
	if (status->packetHeader.start.codes.streamID == STREAMID_VIDEO_NORMAL_STILL)
	{
		SubHeader |= 0xe00; /* what does this do? why is it here?  should HIGH_STILL participate in this? */
	}
	status->chunkArray[wbchunkNumber].packetStart = (uint32)(0x000000ff&(SubHeader>>8));
	#else
	status->chunkArray[wbchunkNumber].packetStart = TRUE;/* always TRUE in WhiteBook VideoCD */
	#endif
	
	wbchunkNumber = (wbchunkNumber + 1) % MAX_WB_CHUNKS;
	*status->numOfWBChunks = wbchunkNumber;
	
	return (ActionProcPtr)&ActionSkipToNextPacket;
}


/*********************************************************************
 * Public WhiteBook parser functions
 *********************************************************************
 */

/* Parse a WhiteBook (VideoCD) MPEG System Stream.
 *
 * INPUTS:
 *   buffer				-- buffer data address. ASSUMES long-word aligned.
 *   bufferSize			-- buffer size. ASSUMES a whole number of WhiteBook disk blocks.
 *	 wbChunkArray		-- array to be filled by the parser
 *	 numOfWBChunks		-- number of white book chunks
 *
 * OUTPUTS:
 *	 wbChunkArray		-- fills this array with information about each audio/video whitebook
 *						   chunk array
 *	 numOfWBChunks		-- returns number of white book chunks filled
 *
 * SIDE EFFECTS:
 *   Changes some fields of *parserPBPtr.
 *
 * ASSUMES: Assumes there is more than enought elements in the whitebook chunk array. 
 *
 * ASSUMES: When asked to find video parameters, the first video packet data will begin
 *   with a video sequence header.
 *
 * ASSUMES: When asked to find audio parameters, the first audio packet data will begin
 *   with an audio syncword.
 *
 * NOTE: Audio tokens will describe the entire MPEG audio Packets, but Video
 *   tokens will just describe the MPEG video packet data bytes.
 *
 * NOTE: Although this treats the buffer as read-only, it may temporarily stuff in
 *   sentinal values for fast scanning.
 */
#if DEBUG
static const char *prevBfr = NULL;
#endif

int32 WBParse_Data(const char* buffer, int32 bufferSize, DSWBChunkPtr chunksAddr, uint32 *numOfWBChunks)
{
	ActionProcPtr	statePtr;		/* local copy to save fetching from status.statePtr */
	ParserStatus	status;
	ParseErrorCount = 0; /*<AK>bugs?*/	
	/* Initialize the parser */
	status.dataPtr = status.bufferPtr = buffer;
	status.endPtr = buffer + bufferSize;
	status.statePtr = statePtr = (ActionProcPtr)&ActionCheckPackStart;
	/* status.packetHeader = ... */
	status.packetAddr = status.packetEndAddr = NULL;
	status.ptsValid = FALSE;		/* <HPP> indicate an invalid pts */
	status.pts = 0;					/* <HPP> old one was status.pts.lo = status.pts.hi = (int32)-1; */
	
	*numOfWBChunks = 0;				/* <HPP> always make sure we are starting at 0 */
	status.numOfWBChunks = numOfWBChunks;/*<HPP> reference for our local status */
	status.chunkArray = chunksAddr;	/* <HPP> reference to our whitebook chunk array */
	
	status.result = 0;				/* status result of 0 is SUCCESS. Returning anything
									   else will result in the stream aborting */
	
	/* Run the parser FSM */
	do
		status.statePtr = statePtr = (ActionProcPtr)(*statePtr)(&status);
	while (statePtr != ACTION_AT_END);
	
#if DEBUG
	ownCtCalls++;
	prevBfr = buffer;
#endif
	return status.result;
}

/* Scan some blocks of a WhiteBook (VideoCD) MPEG System Stream to see if they're "empty".
 * This is a simple, quick test, useful for discarding WhiteBook "Front Margin" data
 * while prerolling.
 *
 * RESULT: TRUE => the data is logically "empty", that is, WBParse would return 0 tokens
 * and 0 errors. FALSE => it's worth parsing this data, although the full parse might return
 * nothing or just parse errors.
 *
 * INPUTS:
 *   buffer				-- buffer data address. ASSUMES long-word aligned.
 *   bufferSize			-- buffer size. ASSUMES a whole number of WhiteBook disk blocks.
 */
Boolean WBParse_IsEmpty(const char* buffer, int32 bufferSize)
	{
	const char			*dataPtr, *endPtr;
	
	if ( (uint32)buffer & 3 )
		return FALSE;	/* an error: misaligned buffer */

	endPtr = buffer + bufferSize;
	
	for ( dataPtr = buffer; dataPtr + PACK_HEADER_CONTENT_SIZE <= endPtr;
			dataPtr += BYTES_PER_WHITEBOOK_BLOCK )
		{
		if ( ((PackHeaderPtr)dataPtr)->packStartCode != 0 )
			return FALSE;
		}

	return TRUE;
	}

/*******************************************************************************************
 * Initialize the WBParamBlock and any other static or global variables.
 *******************************************************************************************/
void WBParse_Initialize(int32 lowOrHighResStillType)
{
	memset( &parserPBlock, 0, sizeof(ParserParamBlock) );
	parserPBlock.processStillImageType = lowOrHighResStillType;

}

/*******************************************************************************************
 * get/set state of the parser
 *******************************************************************************************/
VideoSequenceHeaderPtr WBParse_GetVideoSequenceHeader()
{
	return &parserPBlock.videoSeqHdr;
}

SeekState WBParse_GetSCRSeekState()
{
	return parserPBlock.seekingAnSCR;
}
void WBParse_SetSCRSeekState(SeekState state)
{
	parserPBlock.seekingAnSCR = state;
}

int32 WBParse_GetFirstSCR()
{
	return parserPBlock.firstSCR;
}

SeekState WBParse_GetVideoSeekState()
{
	return parserPBlock.seekingVideoParams;
}
void WBParse_SetVideoSeekState(SeekState state)
{
	parserPBlock.seekingVideoParams = state;
}

uint8 WBParse_GetVideoFrameRate()
{
	return parserPBlock.videoFrameRate;
}

uint32 WBParse_GetVideoVerticalSize()
{
	return parserPBlock.videoVerticalSize;
}

SeekState WBParse_GetAudioSeekState()
{
	return parserPBlock.seekingAudioParameters;
}
void WBParse_SetAudioSeekState(SeekState state)
{
	parserPBlock.seekingAudioParameters = state;
}

uint8 WBParse_GetAudioMode()
{
	return parserPBlock.audioMode;
}

uint8 WBParse_GetAudioEmphasis()
{
	return parserPBlock.audioEmphasis;
}

int32 WBParse_GetMPEGEndCodes()
{
	return parserPBlock.cntMPEGEndCodes;
}

int32 WBParse_GetParserErrors()
{
	return parserPBlock.cntParseErrors;
}
