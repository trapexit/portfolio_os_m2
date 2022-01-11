/******************************************************************************
**
**  @(#) MPEGVideoChunkifier.c 96/09/04 1.5
**
******************************************************************************/

/**
|||	AUTODOC -public -class Streaming_Tools -group MPEG -name MPEGVideoChunkifier
|||	Chunkifies an MPEG-1 Video elementary stream.
|||	
|||	  Format -preformatted
|||	
|||	    MPEGVideoChunkifier (-prompt) | 
|||	        (-i <infile> -o <outfile> -d <time> -s <time>) [options]
|||	
|||	  Description
|||	
|||	    Converts an MPEG-1 Video "elementary stream" (an ISO standard MPEG-1
|||	    video-only compressed stream) to a chunkified MPEG-1 video format for
|||	    the data stream Weaver.
|||	
|||	    As an alternative to the command-line interface, a prompt mode is
|||	    available by typing 'MPEGVideoChunkifier -prompt'. Typing the
|||	    command with no arguments prints out usage, help and examples.
|||	
|||	    For the -d and -s parameters, time is specified in M2 audio ticks,
|||	    that is, (44100/184) or approximately 239.674 ticks per second. So
|||	    24 frames/second translates to approximately 10 audio ticks/second,
|||	    and 30 frames/second translates to approximately 8 audio ticks/second.
|||	
|||	  Arguments
|||	
|||	    -prompt
|||	        Puts MPEGVideoChunkifier into user-prompt mode. In this mode,
|||	        the chunkifier will prompt for all its input parameters.
|||	
|||	    -i <infile>
|||	        File name of the input MPEG-1 Video elementary stream.
|||	
|||	    -o <outfile>
|||	        File name of the output MPEG-1 Video chunkified stream.
|||	
|||	    -d <time>
|||	        Delay in audio ticks between stream time (aka "decoding time"
|||	        or "delivery time") and presentation time.
|||	
|||	    -s <time>
|||	        Presentation time in audio ticks to display the first frame.
|||	        [If you want to set the video start time, you MUST do it with
|||	        this -s switch. It won't work to do it with the start
|||	        parameter of the weaver's "file" command.]
|||	
|||	    -c <logical-channel-number>
|||	        Logical channel number for the data subscription (usually 0).
|||	
|||	    -p
|||	        Progress report (or "verbose") mode. A very useful feature
|||	        because it prints out the stream time (or "decoding" time),
|||	        the presentation time, and the (I, B, or P) frame type
|||	        of each video frame. Note that decoding times should be
|||	        used for branching in order to branch to where the data is.
|||	
|||	    -fps <framerate>
|||	        Floating point frame rate in fps. Use this argument ONLY if you
|||	        need to override the "picture_rate" parameter in the
|||	        video sequence header of the input MPEG Video elementary stream.
|||	
|||	    -refmax <maximum-reference-frame-distance>
|||	        Maximum reference frame distance. This is a workaround to override
|||	        bum temporal reference numbers in the input file. Use this ONLY if
|||	        you're sure you need it!
|||	
|||	  Caveats
|||	
|||	        MPEGVideoChunkifier does not support ISO MPEG-1 Systems streams,
|||	        which is an ISO-defined format for multiplexed MPEG-1 Video and
|||	        MPEG-1 Audio streams. If you wish to chunkify the MPEG-1 Video part
|||	        of an MPEG-1 Systems stream, you must use first demultiplex out
|||	        the video. There are a number of public-domain MPEG-1 Systems
|||	        "splitter" tools which will do this.
|||	
|||	  Examples
|||	
|||	    MPEGVideoChunkifier -prompt
|||	
|||	    MPEGVideoChunkifier -i mymovie.mpg -o mymovie.MPVD -d 10 -s 10
**/


#include <CursorCtl.h>
#include <stdio.h>
#include <string.h>
#include <Memory.h>
#include <stdlib.h>
#include <ctype.h>

#ifndef __MPEGUTILS__
#include "MPEGUtils.h"
#endif

#ifndef __TMPEGSTREAMINFO__
#include "MPEGStreamInfo.h"
#endif

/***********/
/* Defines */
/***********/

#undef DEBUG
#define VERSION 1					/* data format version number */
#define START_CODE_LENGTH 3
#define VIDEO_CODE_LENGTH 4
//#define DPRNT(x) printf x
#define DPRNT(x)
enum{
	GOT_NOTHING,
	GOT_SEQUENCE_HEADER,
	GOT_GOP_HEADER,
	GOT_PICTURE
};

#define NON_RESTRICTING_REF_FRAME_MAX		1024	/* 1024 is larger than MPEG allows */

typedef struct TheSequenceHeader {	// Structure to hold the last parsed
	Ptr ptr;						// MPEG sequence header.
	int size;
};

typedef struct TheGOPHeader {	// Structure to hold the last parsed
	Ptr ptr;					// MPEG GOP header.	
	int size;
};

class TheDataAccessor {			// Class to hold file access information.
public:							// Not the best example of OOP. But better
	TheDataAccessor();			// than what we had before.
	~TheDataAccessor();
	int GetMoreData();
	void AdvanceDataAccessor(int bytes);
	char* GetNextStartCode(const char* startCodeStr, int size);

	FILE *inFile;
	FILE *outFile;
	char* inBuffer;
	char* CurrentPtr;
	char* outBuffer;
	uint32 inBufSize;
	uint32 inBufBytesParsed;
	uint32 outBufSize;
};

typedef struct {		/* parameters supplied by the user */
	unsigned long channel;	/* logical channel number */
	long videoM;
	long videoDTSdelay;		/* video DTS delay (in 90 KHz clock ticks) */
	long videoPTSinit;		/* the initial PTS value (in 90 KHz clock ticks) */
	int refFrameMax;		/* maximum distance between reference frames */
} inparams;

typedef struct {		/* calculated parameters from the user supplied ones */
	double videoframerate;
	long videoPTS;
	long videoDTS;
	double videoPTSconst;	/* MPEG ticks/frame */
} calcparams;

/***********/
/* Globals */
/***********/

static char MPEGStartCode[START_CODE_LENGTH] = {0x00, 0x00, 0x01};
static char PictureStartCode[VIDEO_CODE_LENGTH] = {0x00, 0x00, 0x01, 0x00};
static char SequenceHeaderCode[VIDEO_CODE_LENGTH] = {0x00, 0x00, 0x01, 0xB3};
static char SequenceEndCode[VIDEO_CODE_LENGTH] = {0x00, 0x00, 0x01, 0xB7};
static char GroupStartCode[VIDEO_CODE_LENGTH] = {0x00, 0x00, 0x01, 0xB8};
static long videoframecount = 0;
static inparams UserParams; 	/* user supplied parameters */
static calcparams CalcParams;	/* calculated from the user params */
static int ProgressReport;
static TMPEGStreamInfo *gStreamInfo  = 0;
static double gFramesPerSecond = 0.0;
static TheSequenceHeader gCurrentSequenceHeader;
static TheGOPHeader gCurrentGOPHeader;
static TheDataAccessor* gDataAccessor = 0;
static const double MPEG_CLOCK = 90000.0;
static const double AUDIO_TICK_RATE = 44100.0 / 184.0;	/* 239.674 */
static const double MPEG_TICKS_PER_AUDIO_TICK = MPEG_CLOCK / AUDIO_TICK_RATE;	/* 375.510 */
static const double AUDIO_TICKS_PER_MPEG_TICK = AUDIO_TICK_RATE / MPEG_CLOCK;	/* 0.00266304 */
static const int INPUT_BUFFER_SIZE = 20 * 1024;
static const int OUTPUT_BUFFER_SIZE = 100 * 1024;

/*************************/
/* Function Declarations */
/*************************/

static int GetOptions(int argc, char** argv);
static void PrintUsage();
static void PrintInfo();
static int IsInteger(const char* theStr);
static unsigned short GetPictureTemporalRef(const char* ptr);
static long CalculateParams();
static int PadDataChunks(FILE* theFile, unsigned int bytes);
static int ParseSequenceHeader();
static int ParseGOPHeader();
static int ParsePictureChunk(int state);
static int WritePictureChunk();
static void InitializeMac(void);
static int ReadUserParams(inparams *UserParams);
static long Parse(inparams *UserParams, calcparams *CalcParams);
static long OutputVideoFrameChunk(inparams *UserParams, calcparams *CalcParams, long frameType);
static long GetPictureType(char *ptr);
static long OutputVideoHeaderChunk(inparams *UserParams);

/****************************** Function Definitions ************************/

int
main(int argc, char** argv)
{
	int status = 0;
	
	InitializeMac();
	gDataAccessor = new TheDataAccessor;

	if ((status = GetOptions(argc, argv)) != 0) {
		if (status == 1)
			PrintUsage();
		delete gDataAccessor;
		exit(status);
	}

	if (CalculateParams() == 0) {
		if (ProgressReport && UserParams.refFrameMax < NON_RESTRICTING_REF_FRAME_MAX )
			printf("Reference Frame Max = %ld\n", UserParams.refFrameMax);
		if (ProgressReport && UserParams.channel != 0 )
			printf("Logical channel number = %ld\n", UserParams.channel);

		gStreamInfo = new TMPEGStreamInfo;
		
		if (Parse(&UserParams, &CalcParams))
			printf("Error processing stream!\n");
	}

	delete gDataAccessor;
	delete gStreamInfo;
	
	if (ProgressReport)
		printf("Processing done!\n");

	return 0;
}

long
Parse(inparams *UserParams, calcparams *CalcParams)
{
	char *AUptr = NULL;
	int state = GOT_NOTHING;
	long frameType;
	int done = 0;
	
	gDataAccessor->GetMoreData();

	/* Output video header chunk */
	if (OutputVideoHeaderChunk(UserParams) != 0)
	{
		printf("OutputVideoHeaderChunk Failed!\n");
		return -1;
	}
	
	do {
		AUptr = gDataAccessor->GetNextStartCode(MPEGStartCode, START_CODE_LENGTH);
		if (!AUptr)
			return 0;
		
		switch ((unsigned char) AUptr[3]) {
		case 0x00:
			DPRNT(("PictureStartCode\n"));			
			frameType = GetPictureType(AUptr);
			gStreamInfo->TemporalRef(GetPictureTemporalRef(AUptr));
			gStreamInfo->PictureType((int) frameType);
			if (ParsePictureChunk(state))
				return -1;
			if (OutputVideoFrameChunk(UserParams, CalcParams, frameType))
				return -1;
			gDataAccessor->outBufSize = 0;
			state = GOT_PICTURE;
			break;

		case 0xB3:
			DPRNT(("SequenceHeaderCode\n"));
			if (ParseSequenceHeader()) {
				printf("ParseSequenceHeader Failed.\n");
				return -1;
			}
			state = GOT_SEQUENCE_HEADER;
			continue;
			break;

		case 0xB7:
			DPRNT(("SequenceEndCode\n"));
			done = 1;
			break;

		case 0xB8:
			DPRNT(("GroupStartCode\n"));
			if (ParseGOPHeader()) {
				printf("ParseGOPHeader Failed.\n");
				return -1;
			}
			state = GOT_GOP_HEADER;
			continue;
			break;
		default:
			printf("Got: 0x%.2x%.2x%.2x  %.2x!\n", AUptr[0], AUptr[1], AUptr[2], AUptr[3]);
			return -1;
			break;
		}
	} while (!done);

	return(0);
}

static int
ParseSequenceHeader()
{
	char *gopPtr = NULL;

	do {
		gopPtr = gDataAccessor->GetNextStartCode(GroupStartCode,
												 VIDEO_CODE_LENGTH);
		if (gopPtr)
			break;
		if (gDataAccessor->GetMoreData() == 0) {
			printf("Couldn't find GOP start code.\n");
			return -1;
		}
	} while (!gopPtr);
	
	int seqHeaderSize = gopPtr - gDataAccessor->CurrentPtr;

	if (gCurrentSequenceHeader.ptr) {
		if (seqHeaderSize != gCurrentSequenceHeader.size)
			SetPtrSize(gCurrentSequenceHeader.ptr, seqHeaderSize);
	} else
		gCurrentSequenceHeader.ptr = NewPtr(seqHeaderSize);
	
	if (!gCurrentSequenceHeader.ptr) {
		printf("Out Of Memory. Could alloc. gCurrentSequenceHeader\n");
		return -1;
	}
	
	gCurrentSequenceHeader.size = seqHeaderSize;
	memcpy(gCurrentSequenceHeader.ptr, gDataAccessor->CurrentPtr, seqHeaderSize);
	gDataAccessor->AdvanceDataAccessor(seqHeaderSize);
	
	return 0;
}

static int
ParseGOPHeader()
{
	char *picPtr = NULL;

	do {
		picPtr = gDataAccessor->GetNextStartCode(PictureStartCode,
												 VIDEO_CODE_LENGTH);
		if (picPtr)
			break;
		if (gDataAccessor->GetMoreData() == 0) {
			printf("Could not find Picture header!\n");
			return -1;
		}
	} while (!picPtr);

	
	int gopSize = picPtr - gDataAccessor->CurrentPtr;

	if (gCurrentGOPHeader.ptr) {
		if (gopSize != gCurrentGOPHeader.size)
			SetPtrSize(gCurrentGOPHeader.ptr, gopSize);
	} else
		gCurrentGOPHeader.ptr = NewPtr(gopSize);
	
	if (!gCurrentGOPHeader.ptr) {
		printf("Out Of Memory. Could alloc. gCurrentGOPHeader\n");
		return -1;
	}
	
	gCurrentGOPHeader.size = gopSize;
	memcpy(gCurrentGOPHeader.ptr, gDataAccessor->CurrentPtr, gopSize);
	gDataAccessor->AdvanceDataAccessor(gopSize);
	
	return 0;
}

static int
ParsePictureChunk(int state)
{
	if (state == GOT_GOP_HEADER) {
		/* Add Sequence header & GOP header first */
		memcpy(gDataAccessor->outBuffer, gCurrentSequenceHeader.ptr,
			   gCurrentSequenceHeader.size);
		gDataAccessor->outBufSize = gCurrentSequenceHeader.size;
		memcpy(gDataAccessor->outBuffer + gDataAccessor->outBufSize, gCurrentGOPHeader.ptr,
			   gCurrentGOPHeader.size);
		gDataAccessor->outBufSize += gCurrentGOPHeader.size;
	}

	/* Copy picture start code into output buffer. */
	memcpy(gDataAccessor->outBuffer + gDataAccessor->outBufSize,
		   gDataAccessor->CurrentPtr, VIDEO_CODE_LENGTH);
	gDataAccessor->outBufSize += VIDEO_CODE_LENGTH;
	gDataAccessor->AdvanceDataAccessor(VIDEO_CODE_LENGTH);
	
	char* tempPtr = NULL;
	int done = 0;
	int size = 0;
	
	DPRNT(("Slice: "));
	do {
		tempPtr = gDataAccessor->GetNextStartCode(MPEGStartCode, START_CODE_LENGTH);

		if (tempPtr == NULL) {
			size = gDataAccessor->inBufSize - gDataAccessor->inBufBytesParsed;
			memcpy(gDataAccessor->outBuffer + gDataAccessor->outBufSize, gDataAccessor->CurrentPtr, size);
			gDataAccessor->outBufSize += size;
			gDataAccessor->AdvanceDataAccessor(size);
			if (gDataAccessor->GetMoreData() == 0) {
				printf("Not a complete picture.\n");
				return -1;
			}
		} else if ((uchar)tempPtr[2] == 1 &&
				    ((uchar)tempPtr[3] >= 1 && (uchar)tempPtr[3] <= 0xAF)) {
			DPRNT((" %X", tempPtr[3]));
			size = tempPtr - gDataAccessor->CurrentPtr + START_CODE_LENGTH;
			memcpy(gDataAccessor->outBuffer + gDataAccessor->outBufSize, gDataAccessor->CurrentPtr, size);
			gDataAccessor->outBufSize += size;
			gDataAccessor->AdvanceDataAccessor(size);
		} else {
			DPRNT((" Complete picture.\n"));
			size = tempPtr - gDataAccessor->CurrentPtr;
			memcpy(gDataAccessor->outBuffer + gDataAccessor->outBufSize, gDataAccessor->CurrentPtr, size);
			gDataAccessor->outBufSize += size;
			gDataAccessor->AdvanceDataAccessor(size);
			done = 1;
		}
	} while (!done);
	DPRNT(("\n"));

	return 0;
}

/* Write some data bytes to the output file and check for success. */
static int FWrite(const void *bfr, size_t bytecount, FILE *file)
	{
	long byteswritten = fwrite(bfr, 1, bytecount, file);
	if ( byteswritten != bytecount )
		{
		printf("File output error!\n");
		return -1;
		}
	return 0;
	}


/******************************************************************************
Notes on timestamps for MPEG-Video Chunkifier, G. Wallace, 9/21/95

These notes are to explain the dts & pts formulae in OutputVideoFrameChunk().

(The unit of time here is a video frame period.
"t-ref" means the "temporal reference" field in the MPEG Video picture header.
"count" is the frame count.)

Example 1: stream with no pts delay, starts with B frames

type:	I	B	B	P	B	B	B	P	B	B;	I	B	B	...
count:	0	1	2	3	4	5	6	7	8	9	10	11	12
pts:	2	0	1	6	3	4	5	9	7	8	12	10	11
t-ref:	2	0	1	6	3	4	5	9	7	8	2	0	1

But if dts=count, this pattern violates pts >= dts.  Thus we
have to delay the presentation by 1 frame:
dts:	0	1	2	3	4	5	6	7	8	9	10	11	12
pts:	3	1	2	7	4	5	6	10	8	9	13	11	12

This implies the formulae:
				dts = count;
for B:			pts = dts;
for ref:		pts = dts + dist-to-next-ref.

Alternatively, if dts need not always be increasing, could have:
dts:	0	0	1	2	3	4	5	6	7	8	9	10	11
pts:	2	0	1	6	3	4	5	9	7	8	12	10	11


Example 2: stream with no pts delay, which starts with I frame

type:	I	P	B	B	B	P	B	B;	I	B	B	P	B	B  ...
count:	0	1	2	3	4	5	6	7	8	9	10	11	12	13
pts:	0	4	1	2	3	7	5	6	10	8	9	13	11	12
t-ref:	0	4	1	2	3	7	5	6	2	0	1	5	3	4

Again if dts=count, this pattern violates pts >= dts, so should be:
dts:	0	1	2	3	4	5	6	7	8	9	10	11	12	13
pts:	1	5	2	3	4	8	6	7	11	9	10	14	12	13

This also implies the formulae:
				dts = count;
for B:			pts = dts;
for ref:		pts = dts + dist-to-next-ref.

Next, generalize the forumlae to include delay (i.e. amount by which PTS is
delayed from DTS):

		dts = count;
B:		pts = dts + delay;
ref:	pts = dts + delay + dist-to-next-ref.

Finally, to fully generalize the formulae, note that dts must be increased for
the case of a large pts-start-time. Specifically, dts must be increased by the
difference between the minimum starting pts and the user-specified starting
pts -- once it's been validated! -- minus the delay:

		dts = count + (pts-start-spec'd - pts-start-min - delay)

so the first pts = dts + delay = pts-start-spec'd - pts-start-min.

Notice from the above two examples that minimum first pts is always 1 (if
delay is zero).  Thus we have as the final formulae:

		dts = count + (pts-start - delay - 1);
B:		pts = dts + delay;
ref:	pts = dts + delay + dist-to-next-ref.

One observation: if you have a no-B-frame stream, so that encoded order and
presentation order are identical, and (assuming delay=0) you want to make
starting presentation-time=0, you can't. The min starting presentation time
will still be 1.  This might seem like a bug, but it's really not, because
this chunkifier has no way of determining the global fact that there are no
B-frames in the stream.  Even if the first two frames are non-B -- or even if
the first N frames are non-B for that matter -- there could always be a
B-frame later in the stream, and then the min starting presentation-time of 1
would be necessary.
******************************************************************************/

long
OutputVideoFrameChunk(inparams *UserParams, calcparams *CalcParams, long frametype)
{
	MPEGVideoFrameChunk VideoFrameChunk;
	int	readRefFrameDist = 0, refFrameDist = 0;
	
	/* Compute DTS and PTS */
	// From Notes: dts = count + PTSinit - DTSdelay - 1

	double DTS = (double) (videoframecount * CalcParams->videoPTSconst) +
		  (double) (UserParams->videoPTSinit - UserParams->videoDTSdelay -
		   		   (1 * CalcParams->videoPTSconst));

	CalcParams->videoDTS = (long)(DTS + 0.5);

	if (frametype == BFRAME)
		// pts = dts + delay;
		CalcParams->videoPTS = CalcParams->videoDTS + UserParams->videoDTSdelay;
	else
		// pts = dts + delay + dist-to-next-ref.
		{
		refFrameDist = readRefFrameDist = gStreamInfo->ReferenceFramesDist();
		if ( refFrameDist > UserParams->refFrameMax )
			refFrameDist = UserParams->refFrameMax;
		CalcParams->videoPTS = CalcParams->videoDTS + UserParams->videoDTSdelay +
							   refFrameDist * CalcParams->videoPTSconst;
		}
	videoframecount++;

	//gStreamInfo->Dump();
	gStreamInfo->IncrementFrameCount();
	gStreamInfo->IncrementGOPFrameCount();
	
	/* Output a chunk header */
	DTS = DTS * AUDIO_TICKS_PER_MPEG_TICK;

	VideoFrameChunk.chunkType = MPVD_CHUNK_TYPE;
	VideoFrameChunk.chunkSize = offsetof(MPEGVideoFrameChunk, compressedVideo) +
								gDataAccessor->outBufSize;
	VideoFrameChunk.time = (unsigned long)(DTS + 0.5);
	VideoFrameChunk.channel = UserParams->channel;
	VideoFrameChunk.subChunkType = MVDAT_CHUNK_SUBTYPE;
	VideoFrameChunk.pts = (unsigned long)CalcParams->videoPTS;
	
	if (ProgressReport)
		printf("%6ld:    %c    %5ld    %5ld    %5ld    %9ld",
			videoframecount-1,
			frametype == IFRAME ? 'I' :
			frametype == PFRAME ? 'P' :
			frametype == BFRAME ? 'B' : '-',
			VideoFrameChunk.chunkSize,
			VideoFrameChunk.time,
			(long)(VideoFrameChunk.pts * AUDIO_TICKS_PER_MPEG_TICK + 0.5),
			VideoFrameChunk.pts);
	if (readRefFrameDist != refFrameDist)
		printf("    Note: Clipped the reference frame distance from %ld to %ld!\n",
				readRefFrameDist, refFrameDist);
	else if (ProgressReport)
		printf("\n");

	SpinCursor(2);

	/* Output the chunk */
#if 0
	outbits(VideoFrameChunk.chunkType, 0, NULL, 32, 31, gDataAccessor->outFile);
	outbits(VideoFrameChunk.chunkSize, 0, NULL, 32, 31, gDataAccessor->outFile);
	outbits(VideoFrameChunk.time, 0, NULL, 32, 31, gDataAccessor->outFile);
	outbits(VideoFrameChunk.channel, 0, NULL, 32, 31, gDataAccessor->outFile);
	outbits(VideoFrameChunk.subChunkType, 0, NULL, 32, 31, gDataAccessor->outFile);
	outbits(VideoFrameChunk.pts, 0, NULL, 32, 31, gDataAccessor->outFile);
#else
	if ( FWrite(&VideoFrameChunk, offsetof(MPEGVideoFrameChunk, compressedVideo),
			gDataAccessor->outFile) < 0 )
		return(-1);
#endif
	
	if ( FWrite(gDataAccessor->outBuffer, gDataAccessor->outBufSize,
			gDataAccessor->outFile) < 0 )
		return(-1);

	if (VideoFrameChunk.chunkSize % 4)
		return PadDataChunks(gDataAccessor->outFile, 
							 (unsigned int) (4 - (VideoFrameChunk.chunkSize % 4)));

	return 0;
}

long
OutputVideoHeaderChunk(inparams *UserParams)
{
	MPEGVideoHeaderChunk VideoHeaderChunk;

	char *tempPtr = gDataAccessor->GetNextStartCode(SequenceHeaderCode, VIDEO_CODE_LENGTH);

	if (tempPtr == NULL) {
		printf("sequence_header_code missing!\n");
		return -1;
	}
	gDataAccessor->AdvanceDataAccessor(tempPtr - gDataAccessor->inBuffer);
	
	/* Compute DTS */
	double DTS = (double)(UserParams->videoPTSinit - UserParams->videoDTSdelay);

	/* Output chunk header */
	DTS = DTS * AUDIO_TICKS_PER_MPEG_TICK;

	VideoHeaderChunk.chunkType = MPVD_CHUNK_TYPE;
	VideoHeaderChunk.chunkSize = sizeof(MPEGVideoHeaderChunk);
	VideoHeaderChunk.time = DTS;
	VideoHeaderChunk.channel = UserParams->channel;
	VideoHeaderChunk.subChunkType = MVHDR_CHUNK_SUBTYPE;
	VideoHeaderChunk.version = VERSION;
	VideoHeaderChunk.maxPictureArea = 0;
	VideoHeaderChunk.framePeriod = MPEG_CLOCK / CalcParams.videoframerate + 0.5; /* MPEG ticks/frame */
	
	if (ProgressReport)
		/* Print the field titles for upcoming print out lines */
		printf("\n Frame   Frame  Frame    Decode   Presentation"
			   "\n Number  Type   Size      Time    Time     MPEG Time"
			   "\n-------  -----  -----    -----    -----    ---------\n");
	
#if 0
	outbits(VideoHeaderChunk.chunkType, 0, NULL, 32, 31, gDataAccessor->outFile);
	outbits(VideoHeaderChunk.chunkSize, 0, NULL, 32, 31, gDataAccessor->outFile);
	outbits(VideoHeaderChunk.time, 0, NULL, 32, 31, gDataAccessor->outFile);
	outbits(VideoHeaderChunk.channel, 0, NULL, 32, 31, gDataAccessor->outFile);
	outbits(VideoHeaderChunk.subChunkType, 0, NULL, 32, 31, gDataAccessor->outFile);
	outbits(VideoHeaderChunk.version, 0, NULL, 32, 31, gDataAccessor->outFile);
	outbits(VideoHeaderChunk.maxPictureArea, 0, NULL, 32, 31, gDataAccessor->outFile);
	outbits(VideoHeaderChunk.framePeriod, 0, NULL, 32, 31, gDataAccessor->outFile);
#else
	if ( FWrite(&VideoHeaderChunk, sizeof(VideoHeaderChunk), gDataAccessor->outFile) < 0 )
		return(-1);
#endif
	
	if (VideoHeaderChunk.chunkSize % 4)
		return PadDataChunks(gDataAccessor->outFile,
							 (unsigned int) (4 - (VideoHeaderChunk.chunkSize % 4)));

	return 0;
}

//=========================== TheDataAccessor Class =========================

TheDataAccessor::TheDataAccessor()
{
	inFile = 0;
	outFile = 0;
	inBuffer = 0;
	CurrentPtr = 0;
	outBuffer = 0;
	inBufSize = 0;
	inBufBytesParsed = 0;
	outBufSize = 0;

	if ((inBuffer = NewPtr(INPUT_BUFFER_SIZE)) == NULL) {
		printf("Out of memory. Could not alloc. input buffer\n");
		return;
		// 911 return -1;
	}
	CurrentPtr = inBuffer;
	
	if ((outBuffer = NewPtr(OUTPUT_BUFFER_SIZE)) == NULL) {
		printf("Out of memory. Could not alloc. input buffer\n");
		DisposePtr(inBuffer);
		return;
		// 911 return -1;
	}
}

TheDataAccessor::~TheDataAccessor()
{
	if (inFile)
		fclose(inFile);
	if (outFile)
		fclose(outFile);
	if (inBuffer)
		DisposePtr(inBuffer);
	if (outBuffer)
		DisposePtr(outBuffer);
}

void
TheDataAccessor::AdvanceDataAccessor(int bytes)
{
	inBufBytesParsed += bytes;
	CurrentPtr += bytes;
}

int
TheDataAccessor::GetMoreData()
{
	static int firstTime = 1;
	int bytesRead = 0;
	
	if (firstTime) {
		firstTime = 0;
		bytesRead = fread(inBuffer, 1, INPUT_BUFFER_SIZE, inFile);
		inBufBytesParsed = 0;
		inBufSize = bytesRead;
		CurrentPtr = gDataAccessor->inBuffer;
		return inBufSize;
	}
	
	if (gDataAccessor->inBufBytesParsed == 0)
		return inBufSize;
	int bytesLeft = inBufSize - inBufBytesParsed;

	if (bytesLeft)
		memcpy(inBuffer, CurrentPtr, bytesLeft);
	
	bytesRead = fread(inBuffer + bytesLeft, 1,
					  INPUT_BUFFER_SIZE-bytesLeft, inFile);
					  
	inBufBytesParsed = 0;
	inBufSize = bytesLeft + bytesRead;
	CurrentPtr = gDataAccessor->inBuffer;
	return inBufSize;
}

char*
TheDataAccessor::GetNextStartCode(const char* startCodeStr, int size)
{
	char* tempPtr = NULL;
	
	if ((inBufSize - inBufBytesParsed) <= 4) {
		if (GetMoreData() == 0)
			return 0;
	}
	
	tempPtr = strstrn(CurrentPtr, inBufSize - inBufBytesParsed,
				      (char *) startCodeStr, size);

	/* If start code happens to be in the last 4 bytes or less of the
	 * buffer GetMoreData. Dangerous boundary condition.
	 */
	if ((inBuffer + inBufSize - tempPtr) <= sizeof(uint32)) {
		GetMoreData();
		tempPtr = strstrn(CurrentPtr, inBufSize - inBufBytesParsed,
						  (char *) startCodeStr, size);
	}
	
	return tempPtr;
}

/********************************************************/
/* Identify MPEG picture type from a picture_start_code */
/********************************************************/

long
GetPictureType(char *ptr) /* pointer to picture start code */
{
	/* ptr point to a PICTURE START CODE */
	long picttype;
	char tmp;

	/* the picture type should be found as follows */
	/* (32bits for start code ) (10 bits for temporal_reference) 
	(3 bits for picture type ) */

	/* WARNING THIS CODE ASSUMES 8 BIT BYTES..., of if longer chars, 
	that only bits 0..7 have data. */
	tmp = ptr[5] & 0x38;	/* 00111000, mask out unwanted bits */
	picttype = tmp;
	return(picttype);
}

static unsigned short
GetPictureTemporalRef(const char* ptr)
{
	/* ptr point to a PICTURE START CODE */
	/* (32bits for start code ) (10 bits for temporal_reference) */

	unsigned short temp_ref = 0;
	
	temp_ref = *(((short *) ptr) + 2);
	temp_ref = temp_ref & 0xFFC0;
	temp_ref = temp_ref >> 6;
	
	return temp_ref;
}


static int
PadDataChunks(FILE* theFile, unsigned int bytes)
{
	// Pads 'theFile' with 'bytes' number of zeros. This function
	// is used to ensure that MPEG Video/Audio chunks are four bytes
	// aligned.
	
	if (theFile && bytes) {
		static const char zeros[4] = {0, 0, 0, 0};
		
		return FWrite(zeros, bytes, theFile);
	}
	return 0;
}

static long
CalculateParams()
{
	long bytesread;
	unsigned long index;
	char *inputbuf, *startptr;
	static const double picture_rate[8] = { 23.976,
											24.0,
											25.0,
											29.97,
											30.0,
											50.0,
											59.94,
											60.0 };
	
	/* Identify video frame rate */
	inputbuf = NewPtr(INPUT_BUFFER_SIZE);
	if (!inputbuf)
		return 1;
	
	bytesread = fread(inputbuf, 1, INPUT_BUFFER_SIZE, gDataAccessor->inFile);
	rewind(gDataAccessor->inFile);
	startptr = strstrn(inputbuf, bytesread, SequenceHeaderCode, VIDEO_CODE_LENGTH);
	if (startptr == NULL) {
		printf("sequence_header_code missing!\n");
		return(-1);
	}
	
	// If user has not specified fps translate the frame rate code in the input file.
	if ( gFramesPerSecond <= 0.0 ) {
		index = ((VideoSequenceHeader *)startptr)->picture_rate;
		if ((index == 0) || (index > 8)) {
			printf("Illegal picture rate code in the input file!\n");
			return(-1);
		}
		gFramesPerSecond = picture_rate[index - 1];
	}
	CalcParams.videoframerate = gFramesPerSecond;

	if (ProgressReport)
		printf("Video frame rate = %f frames/second\n", CalcParams.videoframerate);

	/* Constant used for the computation of video PTS:
	 * 90000 MPEG ticks/sec / videoframerate frames/sec = videoPTSconst MPEG ticks/frame */
	CalcParams.videoPTSconst = MPEG_CLOCK / CalcParams.videoframerate;
	
	DisposePtr(inputbuf);
	return(0);
}

static int
IsInteger(const char* theStr)
{
	int i = 0;
	
	if (!theStr || strlen(theStr) == 0)
		return 0;
	
	while (theStr[i]) {
		if (!isdigit(theStr[i]))
			return 0;
		i++;
	}
	return 1;
}
	
void
InitializeMac()
{
	InitCursorCtl(NULL);
}

static int
GetOptions(int argc, char **argv)
{
	int i = 0;
	int err = 0;
	char theInput[64];
	char theOutput[64];
	char theDelay[20];
	char thePresent[20];
	char theFps[20];
	char theRefMax[20];
	char theChannel[20];
	
	if (argc == 1) {
		PrintInfo();
		return 1;
	}

	/* Interactive mode. Pompt user for inputs */
	if (argc == 2 && !strcmp(argv[1], "-prompt")) {
		
		if ((err = ReadUserParams(&UserParams)) != 0)
			return err;
		else
			return 0;
	}

	/* Process command line options. */
	theInput[0] = 0;
	theOutput[0] = 0;
	theDelay[0] = 0;
	thePresent[0] = 0;
	theFps[0] = 0;
	theRefMax[0] = 0;
	theChannel[0] = 0;
	ProgressReport = false;
	
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-i")) {
			/* Input filename */
			if (argv[i + 1])
				strcpy(theInput, argv[i + 1]);
			i++;
		}
		
		else if (!strcmp(argv[i], "-o")) {
			/* Output filename */
			if (argv[i + 1])
				strcpy(theOutput, argv[i + 1]);
			i++;
		}

		else if (!strcmp(argv[i], "-d")) {
			/* Delay time between stream time and presentation time. */
			if (argv[i + 1])
				strcpy(theDelay, argv[i + 1]);
			i++;
		}
		
		else if (!strcmp(argv[i], "-s")) {
			/* Presentation time of first displayed frame. */
			if (argv[i + 1])
				strcpy(thePresent, argv[i + 1]);
			i++;
		}
		
		else if (!strcmp(argv[i], "-c")) {
			/* Logical channel number. */
			if (argv[i + 1])
				strcpy(theChannel, argv[i + 1]);
			i++;
		}
		
		else if (!strcmp(argv[i], "-p")) {
			/* Progress Report. */
			ProgressReport = true;
		}
		
		else if (!strcmp(argv[i], "-fps")) {
			/* Frame per second for video. */
			if (argv[i + 1])
				strcpy(theFps, argv[i + 1]);
			i++;
		}
		
		else if (!strcmp(argv[i], "-refmax")) {
			/* Maximum reference frame distance, overrides the file to workaround bum
			 * temporal reference numbers. */
			if (argv[i + 1])
				strcpy(theRefMax, argv[i + 1]);
			i++;
		}
		
		else
			return 1;
	}
	
	// Validate options
	if (!strlen(theInput) || !strlen(theOutput))
		return 1;
	
	if (!IsInteger(theDelay) || !IsInteger(thePresent))
		return 1;
	
	gFramesPerSecond = 0.0;
	sscanf(theFps, "%lf", &gFramesPerSecond);
	
	if ((gDataAccessor->inFile = fopen(theInput,"rb")) == NULL) {
		printf("Input MPEG video file could not be found!    %s\n", theInput);
		return 2;
	}
	
	if ((gDataAccessor->outFile = fopen(theOutput,"wb")) == NULL) {
		printf("Output file could not be opened!    %s\n", theOutput);
		return 2;
	}
	
	UserParams.channel = 0;
	if ( strlen(theChannel) )
		if ( IsInteger(theChannel) )
			UserParams.channel = atol(theChannel);
		else
			return 1;

	UserParams.videoDTSdelay = atol(theDelay) * MPEG_TICKS_PER_AUDIO_TICK + 0.5;
	UserParams.videoPTSinit = atol(thePresent) * MPEG_TICKS_PER_AUDIO_TICK + 0.5;
	
	UserParams.refFrameMax = NON_RESTRICTING_REF_FRAME_MAX;
	if ( IsInteger(theRefMax) )
		UserParams.refFrameMax = atol(theRefMax);

	return 0;
}

int
ReadUserParams(inparams *UserParams)
{
	char inputfilename[80], outputfilename[80];
	long parm;
	
	printf("Enter the MPEG video input filename: \n");
	scanf("%s", inputfilename);
	if ((gDataAccessor->inFile = fopen(inputfilename,"rb")) == NULL) {
		printf("Input MPEG video file could not be found!\n");
		return(2);
	}
	
	printf("Enter the output (chunkified MPEG video) filename:\n");
	scanf("%s", outputfilename);
	if ((gDataAccessor->outFile = fopen(outputfilename,"wb")) == NULL) {
		printf("Output file could not be opened!\n");
		return(2);
	}
	
	printf("Enter the logical channel number (usually 0):\n");
	scanf("%ld", &parm);
	UserParams->channel = (parm >= 0) ? parm : 0;

	printf("Enter the delay between stream time and presentation time (in audio ticks):\n");
	scanf("%ld", &parm);
	/* Convert to 90 KHZ clock */
	UserParams->videoDTSdelay = parm * MPEG_TICKS_PER_AUDIO_TICK + 0.5;
	
	printf("Enter the presentation time to display the first frame (in audio ticks):\n");
	scanf("%ld", &parm);
	/* Convert to 90 KHZ clock */
	UserParams->videoPTSinit = parm * MPEG_TICKS_PER_AUDIO_TICK + 0.5;

	ProgressReport = true;

	printf("Enter the frame/second to play the frames (floating point; type 0 to use the value inside the file):\n");
	scanf("%lf", &gFramesPerSecond);
	
	printf("Enter the maximum reference frame distance, e.g. 3 (type 0 unless you're sure you want to override the file):\n");
	UserParams->refFrameMax = NON_RESTRICTING_REF_FRAME_MAX;
	scanf("%ld", &UserParams->refFrameMax);
	if ( UserParams->refFrameMax <= 0 )
		UserParams->refFrameMax = NON_RESTRICTING_REF_FRAME_MAX;

	return(0);
}

static void
PrintInfo()
{
	printf("\n\nMPEGVideoChunkifier  Tool,  Version 3.1 © 1995-1996 The 3DO Company.");
	printf("\nConverts a MPEG-1 video elementary stream to a 3DO chunkified data stream.");
}

static void
PrintUsage()
{
	printf("\n\nUsage:");
	printf("\n     MPEGVideoChunkifier (-prompt) |");
	printf("\n                    (-i infile -o outfile -d time -s time) |");
	printf("\nOptions:");
	printf("\n    -prompt             # Interactive mode.");
	printf("\n    -i filename         # Input filename.");
	printf("\n    -o filename         # Output filename.");
	printf("\n    -d time             # Delay time between stream time and presentation time (in audio ticks).");
	printf("\n    -s time             # Presentation time to display the first frame (in audio ticks).");
	printf("\n    -c channel          # Logical channel number (optional, usually 0).");
	printf("\n    -p                  # Progress report.");
	printf("\n    -fps rate           # Input file frame rate (floating point frames/second). Optional."
		   "\n                        # Use this ONLY if you need to overrride the file's video sequence header.");
	printf("\n    -refmax max         # Maximum ref frame distance. Use this ONLY if you need to"
		   "\n                        # override bum temporal reference numbers in the input file.");

	printf("\n\nExamples:");
	printf("\n    MPEGVideoChunkifier -prompt");
	printf("\n    MPEGVideoChunkifier -i mymovie.mpg -o mymovie.chk -d 10 -s 10");

	printf("\n\n       - There are approximately 239.674 audio ticks per second.");
	printf("\n         Hence for a 24 fps movie, each frame is ~10 audio ticks.");
	printf("\n       - To overwrite the frame rate specified in the input file's");
	printf("\n         video sequence header, use the -fps option.");
}
