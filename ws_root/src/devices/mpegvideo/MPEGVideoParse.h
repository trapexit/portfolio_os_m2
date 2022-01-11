/* @(#) MPEGVideoParse.h 96/12/11 1.16 */
/* file: mpegVideoParse.h */
/* definitions for MPEG video stream parser */
/* 3/28/94 George Mitsuoka */
/* The 3DO Company Copyright © 1994 */

#ifndef MPEGVIDEOPARSE_HEADER
#define MPEGVIDEOPARSE_HEADER

#include <kernel/types.h>

#ifndef MPEGSTREAM_HEADER
#include "MPEGStream.h"
#endif

#ifndef VIDEO_DEVICECONTEXT_HEADER
#include "videoDeviceContext.h"
#endif

typedef unsigned bits;

typedef struct {
	unsigned char *bsPtr;
	long bsLen;
	long bsDone;
	long bsBits;
	unsigned long bsData;
	long state;
	long min_intra_quant;
	long min_non_intra_quant;
	long quantizer_scale;
	long macroblock_type;
	long forward_r_size;
	long backward_r_size;
	long start_code;
	long dc_size;
	long coef_first;
	long ok_intra_matrix;
	long ok_non_intra_matrix;
	long ok_intra_quant_scale;
	long ok_non_intra_quant_scale;
	long coefindex;
	long luma_count;
	long chroma_count;
	long fvect_count;
	long bvect_count;
	long maxindex;
	long min_intra_index;
	long min_non_intra_index;
	long escape_allones;
	long escape_allzeros;
	long escape_numbits;
	long escape_fix;
	char intra_quant_thresh[64];
	char non_intra_quant_thresh[64];
	char *thresh;
} VCDOPT_INFO;

typedef
	struct
	{
		uint32 sequenceHeaderCode;
		bits horizontalSize:12, verticalSize:12,
			 pelAspectRatio:4, pictureRate:4;
		bits bitRate:18, mb1:1, vbvBufferSize:10, constrainedParametersFlag:1,
			 loadIntraQuantizerMatrix:1,loadNonIntraQuantizerMatrix:1;
	}
	sequenceHeader;

typedef	
	struct
	{
		uint32 groupStartCode;
		bits timeCode:25, closedGOP:1, brokenLink:1, unused:5;
	}
	GOPHeader;

typedef
	struct
	{
		uint32 pictureStartCode;
		bits temporalReference:10, pictureCodingType:3, vbvDelay:16,
			 extraBits:3;
	}
	pictureHeader;

typedef
	struct
	{
		streamContext streamID;				/* stream identifier */
		uint32 layer;						/* which layer we're in */
		enum
		{ start = 0, gotSeqHdr, gotPictHdr}
		parseState;							/* what we've seen */
		sequenceHeader seqHeader;			/* stream's sequence header */
		uint8 intraQuantMatrix[ 64 ];		/* intra quantizer matrix */
		uint8 nonIntraQuantMatrix[ 64 ];	/* non-intra quantizer matrix */
		uint8 quantMatrixOK;
		VCDOPT_INFO* vcdInfo;
		GOPHeader gopHeader;				/* current stream's GOP header */
		uint32 pictureNumber;				/* # of pictures we've decoded */
		uint32 refUserData;					/* last ref picture's userData */
		uint32 refPTS, refPTSValid;			/* last ref picture's pts */
		uint32 userData;					/* passed from input to output */
		uint32 pts, ptsValid;				/* current picture's pts */
		pictureHeader pictHeader;			/* picture header */
		uint8 fullPelForwardVector,forwardFCode;
		uint8 fullPelBackwardVector,backwardFCode;
		int32 flags;						/* what of above is valid */
		uint8 parseIFrameOnly;              /* If true skips B & P pictures */
	}
	MPEGStreamInfo;

	
#define MPS_FLAG_NOTHING_VALID			0L
#define MPS_FLAG_SEQHEADER_VALID		1L
#define MPS_FLAG_GOPHEADER_VALID		2L
#define MPS_FLAG_PICTHEADER_VALID		4L
#define MPS_DECODE_SLICE				7L

#define PICT_TYPE(si) si->pictHeader.pictureCodingType
#define SEQ_WIDTH(si) si->seqHeader.horizontalSize
#define SEQ_HEIGHT(si) si->seqHeader.verticalSize

/* internal functions */
int32 MPVideoParse(tVideoDeviceContext* theUnit, MPEGStreamInfo *context);
int32 MPVideoSequenceHeader( MPEGStreamInfo *context );
int32 MPSequenceHeader(tVideoDeviceContext* theUnit, MPEGStreamInfo *context );
int32 MPGroupOfPicturesHeader(tVideoDeviceContext* theUnit,
							  MPEGStreamInfo *context);
int32 MPPictureHeader(tVideoDeviceContext* theUnit, MPEGStreamInfo *context );
int32 MPSlice(tVideoDeviceContext* theUnit, MPEGStreamInfo *context);
int32 MPUserData(tVideoDeviceContext* theUnit, MPEGStreamInfo *context);
int32 MPExtensionData(tVideoDeviceContext* theUnit, MPEGStreamInfo *context);
void ResetReferenceFrames(tVideoDeviceContext *theUnit);

int32 InitVCDOptimize(VCDOPT_INFO **vcdopt, MPEGStreamInfo *streamInfo);
int32 DestroyVCDOptimize(VCDOPT_INFO *vcdopt);
void DoVCDOptimize(VCDOPT_INFO *vcdopt, MPEGStreamInfo *streamInfo,
					char *bsBuffer, long length);
#endif

