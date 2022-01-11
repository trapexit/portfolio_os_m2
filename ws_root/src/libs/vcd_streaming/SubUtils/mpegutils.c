/******************************************************************************
**
**  @(#) mpegutils.c 96/06/05 1.2
**
******************************************************************************/

#include <string.h>
#include <stdlib.h>				
#include <stdio.h>

#include <kernel/types.h>
#include <kernel/debug.h>
#include <kernel/mem.h>
#include <audio/audio.h>
#include <video_cd_streaming/mpeg.h>

#include <video_cd_streaming/mpegutils.h>

/* The following table converts the ISO 11172 4 bit picture rate code into 90kHz ticks
 * per picture delta PTS values. We only use this to dead-reckon picture PTSs. Hopefully,
 * every decoded picture will have a valid PTS so the MPEG standard frame rate field won't
 * really matter. In that case, we can support any frame rate that up to the max speed
 * the hardware can decode. */
#define ISO11172_VSHRateTableSize			16
#define DEFAULT_FRAME_RATE					3750	/* 24 pictures per second */
#define CUSTOM_FRAME_RATE					6000	/* 15 pictures per second */
#define DEFAULT_VERTICAL_SIZE				240		/* default vertical size */
#define DEFAULT_HORIZONTAL_SIZE				320		/* default horizontal size */
#define DEFAULT_ASPECT_RATIO				1		/* default aspect ratio */
#define DEFAULT_VBV_BUFFER_SIZE				4096	/* default buffer size */

static const uint32 VSHRateTable[ISO11172_VSHRateTableSize] =
{
	CUSTOM_FRAME_RATE,		/* unknown, maybe custom; provide a working value */
	3754,					/* 23.976 pictures per second */
	3750,					/* 24 pictures per second */
	3600,					/* 25 pictures per second */
	3003,					/* 29.97 pictures per second */
	3000,					/* 30 pictures per second */
	1800,					/* 50 pictures per second */
	1502,					/* 59.94 pictures per second */
	1500,					/* 60 pictures per second */
	
	/* The remaining MPEG picture rate codes are reserved.
	 * Provide a helpful default frame rate value to use for those picture rate codes. */
	DEFAULT_FRAME_RATE, DEFAULT_FRAME_RATE, DEFAULT_FRAME_RATE, DEFAULT_FRAME_RATE,
	DEFAULT_FRAME_RATE, DEFAULT_FRAME_RATE, DEFAULT_FRAME_RATE
};

#if defined(DEBUG) || (PRINT_LEVEL >= 2)
static const char *const	videoFrameRateName[16] =
	{
	"unknown/custom",
	"23.976",
	"24",
	"25",
	"29.97",
	"30",
	"50",
	"59.94",
	"60",
	"reserved", "reserved", "reserved", "reserved", "reserved", "reserved", "reserved"
	};
#endif

/*==========================================================================================
  ==========================================================================================
					Routines for Working with MPEG Video Sequence Headers
  ==========================================================================================
  ==========================================================================================*/
 
/*******************************************************************************************
 * Extract and return the frame rate from an MPEG video sequence header.
 *******************************************************************************************/
uint32 VideoSequenceHeader_GetFrameRate(VideoSequenceHeaderPtr seqHeader)
{
	/* Check that we have a valid header */
	if ( seqHeader->sequenceHeaderCode != MPEG_VIDEO_SEQUENCE_HEADER_CODE ) {
		PERR(("MPEG VideoSeqHdr has a bum header code. Assuming %d pictures/sec.\n",
			DEFAULT_FRAME_RATE));
		return DEFAULT_FRAME_RATE;
	}
	
	/* Print some useful debugging info. */
	PRNT(("  MPEG VideoSeqHdr %dh x %dv @ %s Hz %ld bit/sec\n",
		seqHeader->horizontal_size,
		seqHeader->vertical_size,
		videoFrameRateName[seqHeader->picture_rate],
		seqHeader->bit_rate * 400));
	
	return VSHRateTable[seqHeader->picture_rate];
}

/*******************************************************************************************
 * Extract and return the vertical size from the video sequence header
 *******************************************************************************************/
uint32 VideoSequenceHeader_GetVerticalSizes(VideoSequenceHeaderPtr seqHeader)
{
	/* Check that we have a valid header */
	if ( seqHeader->sequenceHeaderCode != MPEG_VIDEO_SEQUENCE_HEADER_CODE ) {
		return DEFAULT_VERTICAL_SIZE;
	}
	return seqHeader->vertical_size;
}

/*******************************************************************************************
 * Extract and return the horizontal size from the video sequence header
 *******************************************************************************************/
uint32 VideoSequenceHeader_GetHorizontalSizes(VideoSequenceHeaderPtr seqHeader)
{
	/* Check that we have a valid header */
	if ( seqHeader->sequenceHeaderCode != MPEG_VIDEO_SEQUENCE_HEADER_CODE ) {
		return DEFAULT_HORIZONTAL_SIZE;
	}
	return seqHeader->horizontal_size;
}

/*******************************************************************************************
 * Extract and return the aspect ratio
 *******************************************************************************************/
uint32 VideoSequenceHeader_GetAspectRatio(VideoSequenceHeaderPtr seqHeader)
{
	/* Check that we have a valid header */
	if ( seqHeader->sequenceHeaderCode != MPEG_VIDEO_SEQUENCE_HEADER_CODE ) {
		return DEFAULT_ASPECT_RATIO;
	}
	return seqHeader->pel_aspect_ratio;
}

/*******************************************************************************************
 * Extract and return the aspect ratio
 *******************************************************************************************/
uint32 VideoSequenceHeader_GetBufferSize(VideoSequenceHeaderPtr seqHeader)
{
	/* Check that we have a valid header */
	if ( seqHeader->sequenceHeaderCode != MPEG_VIDEO_SEQUENCE_HEADER_CODE ) {
		return DEFAULT_VBV_BUFFER_SIZE;
	}
	return seqHeader->vbv_buffer_size;
}


/*==========================================================================================
  ==========================================================================================
						Routines for Dealing with MPEG Timestamp Conversions
  ==========================================================================================
  ==========================================================================================*/
#define AUDIO_TICKS_PER_SEC_FLOAT		(44100.0 / 184.0)	/* about 239.673913. Assumes 44.1 kHz audio */

#define MPEG_TICKS_PER_AUDIO_TICK_FLOAT	\
	(MPEG_CLOCK_TICKS_PER_SECOND / AUDIO_TICKS_PER_SEC_FLOAT)
#define MPEG_TICKS_PER_AUDIO_TICK	\
	((int32)(MPEG_TICKS_PER_AUDIO_TICK_FLOAT + 0.5))	/* nearest integer value */

#define TWO_TO_THE_16_FLOAT	65536.0
#define TWO_TO_THE_32_FLOAT	(TWO_TO_THE_16_FLOAT * TWO_TO_THE_16_FLOAT)

/*******************************************************************************************
 * Convert *UNSIGNED* MPEG timestamp ticks to audio clock ticks (e.g. DS stream
 * clock values), assuming they have the same origin. This rounds the remainder.
 * The input domain is [0 .. 2^32) so the output range is [0 .. 11,437,685] ticks
 * == [0 .. 13.2560719] hours.
 *
 * WARNING: This produces bogus results for negative signed values.
 *
 * IMPLEMENTATION: This uses a 64 bit multiply because it's faster than division.
 *******************************************************************************************/
#define AUDIO_TICKS_PER_MPEG_TICK_FRAC32	\
	((uint32)(TWO_TO_THE_32_FLOAT /	MPEG_TICKS_PER_AUDIO_TICK_FLOAT + 0.5))
#define ONE_HALF_FRAC32						(1UL << 31)

uint32 MPEGTimestampToAudioTicks(MPEGTimestamp mpegTimestamp)
{
	uint64	prod;
	
	prod = (uint64)mpegTimestamp * AUDIO_TICKS_PER_MPEG_TICK_FRAC32 + ONE_HALF_FRAC32;
	return (uint32)(prod >> 32);
}

/*******************************************************************************************
 * Convert a *SIGNED* delta of MPEG timestamp ticks to SIGNED audio clock tick values,
 * assuming they have the same origin. This rounds the remainder.
 * The input domain is [-2^31 .. 2^31) so the output range is [-5,718,843 .. 5,718,842] ticks
 * == +/- 6.628036152 hours.
 *
 * WARNING: This produces bogus results for large unsigned values, which do appear in MPEG.
 *
 * IMPLEMENTATION: This uses a 64 bit multiply because it's faster than division.
 *******************************************************************************************/
int32 MPEGDeltaTimestampToAudioTicks(MPEGTimestamp mpegTimestamp)
{
	int64	prod;
	
	prod = (int64)mpegTimestamp * AUDIO_TICKS_PER_MPEG_TICK_FRAC32 + ONE_HALF_FRAC32;
	return (int32)(prod >> 32);
}

/*******************************************************************************************
 * Convert *UNSIGNED* audio clock ticks (e.g. DS stream clock values) to MPEG timestamp
 * ticks (90 kHz mod 2^32), assuming they have the same origin. For the ordinary audio clock
 * (239.673913 Hz), this computes audioTicks * 375.5102041 mod 2^32. The wraparound happens
 * at 13.2560719 hours.
 *
 * ASSUMES: This implementation assumes the audio clock is 239.673913 Hz (44.1 kHz / 184).
 *******************************************************************************************/
#define TWO_TO_THE_23						(1 << 23)
#define MPEG_TICKS_PER_AUDIO_TICK_FRAC_9_23	\
	((uint32)(MPEG_TICKS_PER_AUDIO_TICK_FLOAT * TWO_TO_THE_23 + 0.5))
#define ONE_HALF_FRAC23						(1UL << 22)

uint32 AudioTicksToMPEGTimestamp(uint32 audioTicks)
{
	uint64	prod;
	
	prod = (uint64)audioTicks * MPEG_TICKS_PER_AUDIO_TICK_FRAC_9_23 + ONE_HALF_FRAC23;
	return (uint32)(prod >> 23);
}


