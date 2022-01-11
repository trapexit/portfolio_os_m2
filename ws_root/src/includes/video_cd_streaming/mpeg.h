#ifndef __MPEG_H__
#define __MPEG_H__
/*******************************************************************************************
 *	File:			MPEG.h
 *
 *	Contains:		Definitions for ISO 11172 MPEG-1 and ISO 13818 MPEG-2 structures
 *
 *	Written by:		Jerry H. Morrison
 *	Modifyed by:		ARAKAWA Kay @ Matsushita Electric Industrial Co., Ltd.
 *
 *	Copyright:	© 1994 by The 3DO Company. All rights reserved.
 *				This material constitutes confidential and proprietary
 *				information of the 3DO Company and shall not be used by
 *				any Person or for any purpose except as expressly
 *				authorized in writing by the 3DO Company.
 *
 * History:
 *	3/24/95		AK		Added "#if READ_TRIGGER_BIT"
 *	3/14/95		AK		Added HIGH_STILL/NORMAL_STILL to stream ID.
 *	9/22/94		jhm		Added MPEG_AUDIO_SYNCWORD_BYTE_1.
 *	7/11/94		jhm		Corrected #includes.
 *	5/12/94		JHM		Use the union PacketStartCodes in the MPEG-1 PacketHeader
 *						as well as the MPEG-2 PES_PacketPart1. Added
 *						MAX_[PES_]PACKET_STUFFING_BYTES.
 *	5/10/94		JHM		Added STREAMID_AUDIO/VIDEO_LAST.
 *	5/5/94		JHM		Added PACKET_START_AUDIO0/VIDEO0. Bug fixes.
 *	4/29/94		JHM		Added MPEG-2 System layer structure definitions.
 *	4/29/94		JHM		Initial version
 *
 *******************************************************************************************/

/*******************************************************************************************
 * Bit field naming conventions used for structs in this file:
 *  "b0010" should contain %0010,
 *  "scr31_30" contains SCR bits [31..30],
 *  "mb0" is a marker bit #0, it should contain 1,
 *  "tag0x2" is a variant tag to test against the hex value 0x2, and
 *  "fillOutThisStruct" fills the struct to a long-word boundary, beyond the actual.
 *
 *  Struct definitions begin a new group of "unsigned" fields on 4-byte boundaries.
 *******************************************************************************************/

#ifndef __KERNEL_TYPES_H
	#include <kernel/types.h>
#endif

/*********************************************************************
 * WhiteBook File System definitions.
 *********************************************************************
*/
#define READ_TRIGGER_BIT	1
#if READ_TRIGGER_BIT
#define BYTES_PER_WHITEBOOK_BLOCK	2336 /*arac.bug?? for read subhead <-2324*/
#else
#define BYTES_PER_WHITEBOOK_BLOCK 	2324
#endif

#define TRIGGER_BIT       0x01700f 		
#define TRIGGER_BIT_MASK  0xff780f 

/*********************************************************************
 * ISO 11172-1 MPEG-1 System layer structure definitions.
 *********************************************************************
 */

#define MPEG_CLOCK_TICKS_PER_SECOND 90000L

#define PACK_START_CODE				0x000001BAL
#define PACKET_START_CODE_PREFIX24	0x000001L
#define PACKET_START_CODE_PREFIX32	0x00000100L	/* 24-bit value left-aligned in a 32-bit long-word */
#define SYSTEM_HEADER_START_CODE	0x000001BBL
#define ISO_11172_END_CODE			0x000001B9L

#define ZERO_BYTE					0
#define STUFFING_BYTE				0xFF

#define STREAMID_ISO_11172_END_CODE	0xB9	/* not a stream ID, but fits in PacketHeader.streamID */
#define STREAMID_PACK_START_CODE	0xBA	/* not a stream ID, but fits in PacketHeader.streamID */
#define STREAMID_SYSTEM_HEADER		0xBB	/* almost a stream ID; fits PacketHeader */
/* 0xBC is reserved in MPEG-1, defined for MPEG-2, below */
#define STREAMID_PRIVATE1			0xBD
#define STREAMID_PADDING			0xBE
#define STREAMID_PRIVATE2			0xBF
#define STREAMID_AUDIO0				0xC0	/* Audio streams:    $C0 + [0 .. 1F] */
#define STREAMID_AUDIO_LAST			0xDF	/*   [STREAMID_AUDIO0 .. STREAMID_AUDIO_LAST] */
#define STREAMID_VIDEO0				0xE0	/* Video streams:    $E0 + [0 .. F] */
#define STREAMID_VIDEO_HIGH_STILL				0xE2	/* Video streams:    $E0 + [0 .. F] */
#define STREAMID_VIDEO_NORMAL_STILL				0xE1	/* Video streams:    $E0 + [0 .. F] */
#define STREAMID_VIDEO_LAST			0xEF	/*   [STREAMID_VIDEO0 .. STREAMID_VIDEO_LAST] */
											/*	0xF0..FF are reserved in MPEG-1, some are defined for MPEG-2, below */

#define PACKET_START_AUDIO0			(PACKET_START_CODE_PREFIX32 | STREAMID_AUDIO0)
#define PACKET_START_VIDEO0			(PACKET_START_CODE_PREFIX32 | STREAMID_VIDEO0)

#if READ_TRIGGER_BIT
typedef	struct ExtendedPackHeader {							
	uint32	header;
	uint32	subHeader;
	uint32	subHeader2;
} ExtendedPackHeader;
#endif

typedef struct PackHeader {
#if READ_TRIGGER_BIT
    ExtendedPackHeader h;
#endif
	uint32			packStartCode;			/* == PACK_START_CODE */
	unsigned		b0010:4, scr32_32:1, scr31_30:2, mb0:1, scr29_15:15, mb1:1,
					scr14_7:8;
	unsigned		scr6_0:7, mb2:1, mb3:1, muxRate:22, mb4:1;
} PackHeader, *PackHeaderPtr;

#define PACK_HEADER_CONTENT_SIZE		(sizeof(PackHeader))


typedef union PacketStartCodes {	/* 2 views of packet_start_code_prefix, stream_id */
	uint32			packetStartCode32;
	struct {
		unsigned	packetStartCodePrefix:24, streamID:8;
	}				codes;
} PacketStartCodes;


typedef struct PacketHeader {
	PacketStartCodes	start;
	unsigned			packetLength:16, fillOutThisStruct:16;
} PacketHeader, *PacketHeaderPtr;

#define PACKET_HEADER_CONTENT_SIZE		(sizeof(PacketHeader) - 16/8)


typedef struct SystemHeaderBody {
	unsigned		headerLength:16, mb0:1, rateBound21_7:15;
	unsigned		rateBound6_0:7, mb1:1, audioBound:6, fixedFlag:1, CSPSFlag:1,
		   			systemAudioLockFlag:1, systemVideoLockFlag:1, mb2:1, videoBound:5,
		   			reservedByte:8;
} SystemHeaderBody, *SystemHeaderBodyPtr;

#define SYSTEM_HEADER_BODY_CONTENT_SIZE		(sizeof(SystemHeaderBody))


/* A data PacketHeader is followed by [0 .. MAX_PACKET_STUFFING_BYTES]
 * stuffing_byte(s) */
#define MAX_PACKET_STUFFING_BYTES		16


typedef struct StreamParams {
	unsigned		streamID:8, b11:2, stdBufferBoundScale:1, stdBufferSizeBound:13,
					fillOutThisStruct:8;
} StreamParams, *StreamParamsPtr;

#define STREAM_PARAMS_CONTENT_SIZE	(sizeof(StreamParams) - 8/8)


typedef struct STDBufferInfo {
	unsigned		b01:2, stdBufferScale:1, stdBufferSize:13, fillOutThisStruct:16;
} STDBufferInfo, *STDBufferInfoPtr;

#define STD_BUFFER_INFO_CONTENT_SIZE	(sizeof(STDBufferInfo) - 16/8)


typedef struct PTS_Info1 {
	unsigned		tag0x2:4, pts32_32:1, pts31_30:2, mb0:1, pts29_15:15, mb1:1,
					pts14_7:8;
	unsigned		pts6_0:7, mb2:1, fillOutThisStruct:24;
} PTS_Info_PTS;

#define PTS_INFO_PTS_CONTENT_SIZE		(sizeof(PTS_Info_PTS) - 24/8)

typedef struct PTS_Info2 {
	unsigned		tag0x3:4, pts32_32:1, pts31_30:2, mb0:1, pts29_15:15, mb1:1,
					pts14_7:8;
	unsigned		pts6_0:7, mb2:1, b0001:4, dts32_32:1, dts31_30:2, mb3:1,
					dts29_15:15, mb4:1;
	unsigned		dts14_0:15, mb5:1, fillOutThisStruct:16;
} PTS_Info_PTS_DTS;

#define PTS_INFO_PTS_DTS_CONTENT_SIZE	(sizeof(PTS_Info_PTS_DTS) - 16/8)

typedef struct PTS_Info3 {
	unsigned		tag0x0F:8, fillOutThisStruct:24;
} PTS_Info_Other;

#define PTS_INFO_OTHER_CONTENT_SIZE		(sizeof(PTS_Info_Other) - 24/8)

typedef union PTS_Info {
	PTS_Info_PTS		pts;
	PTS_Info_PTS_DTS	pts_dts;
	PTS_Info_Other		other;
} PTS_Info, *PTS_InfoPtr;


/*********************************************************************
 * ISO 13818-1 MPEG-2 System layer structure definitions.
 *********************************************************************
 */

#define STREAMID_PROGRAM_STREAM_MAP	0xBC
#define STREAMID_ECM				0xF0
#define STREAMID_EMM				0xF1
#define STREAMID_DSM_CC				0xF2
#define STREAMID_MHEG				0xF3	/* ISO/IEC 13522 stream, i.e. MHEG */
								/*	0xF4..FE are still reserved in MPEG-2 */
#define STREAMID_PROGRAM_STREAM_DIRECTORY	0xFF


#define PES_SCRAMBLING_UNSCRAMBLED			0
#define PES_SCRAMBLING_USER_DEFINED1		1
#define PES_SCRAMBLING_USER_DEFINED2		2
#define PES_SCRAMBLING_USER_DEFINED3		3

typedef struct PES_PacketPart1 {
	PacketStartCodes	start;
	unsigned			PES_PacketLength:16,	/* length of the rest of the packet */

	#define PES_PACKET_PART1_CONTENT_SIZE1	(sizeof(PacketStartCodes) + 16/8)
	
	/* if (streamID != STREAMID_PRIVATE2 && streamID != STREAMID_PADDING) then also: */
						b10:2,
						PES_scrambling_control:2,
						PES_priority:1,
						data_alignment_indicator:1,
						copyright:1,
						original_or_copy:1,
						PTS_DTS_flags:2,
						ESCR_flag:1,
						ES_rate_flag:1,
						DSM_trick_mode_flag:1,
						additional_copy_info_flag:1,
						PES_CRC_flag:1,
						PES_extension_flag:1;
	unsigned			PES_header_data_length:8,

	#define PES_PACKET_PART1_CONTENT_SIZE2	(PES_PACKET_PART1_CONTENT_SIZE1 + 24/8)
	
	/* if (PTS_DTS_flags == %10 || PTS_DTS_flags == %11) then also: */
						tag0:4,		/* == PTS_DTS_flags */
						pts32_32:1,
						pts31_30:2,
						mb0:1,
						pts29_15:15,
						mb1:1;
	unsigned			pts14_0:15,
						mb2:1,

	#define PES_PACKET_PART1_CONTENT_SIZE3	(PES_PACKET_PART1_CONTENT_SIZE2 + 40/8)
	
	/* if (PTS_DTS_flags == %11) then also: */
						b0001:4,
						dts32_32:1,
						dts31_30:2,
						mb3:1,
						dts29_22:8;
	unsigned			dts21_15:7,
						mb4:1,
						dts14_0:15,
						mb5:1,
						fillOutThisStruct:8;
} PES_PacketPart1, *PES_PacketPart1Ptr;

#define PES_PACKET_PART1_CONTENT_SIZE4			(sizeof(PES_PacketPart1) - 8/8)


	/* if (ESCR_flag) then also: */
typedef struct PES_PacketPart2 {
	unsigned		reserved:2,
					escr32_32:1,
					escr31_30:2,
					mb0:1,
					escr29_15:15,
					mb1:1,
					escr14_5:10;
	unsigned		escr4_0:5,
					mb2:1,
					escr_extension:9,
					mb3:1,
					fillOutThisStruct:16;
} PES_PacketPart2, *PES_PacketPart2Ptr;

#define PES_PACKET_PART2_CONTENT_SIZE		(sizeof(PES_PacketPart2) - 16/8)


	/* if (ES_rate_flag) then also: */
typedef struct PES_PacketPart3 {
	unsigned		mb0:1,
					ES_rate:22,		/* in units of 50 bytes/sec, 0 is forbidden */
					mb1:1,
					fillOutThisStruct:8;
} PES_PacketPart3, *PES_PacketPart3Ptr;

#define PES_PACKET_PART3_CONTENT_SIZE		(sizeof(PES_PacketPart3) - 8/8)


#define TRICK_MODE_FAST_FORWARD		0
#define TRICK_MODE_SLOW_MOTION		1
#define TRICK_MODE_FREEZE_FRAME		2
#define TRICK_MODE_FAST_REVERSE		3

	/* if (DSM_trick_mode_flag) then also: */
typedef union PES_PacketPart4 {
	struct {
		unsigned	trick_mode_control:3,
					field_id:2,
					intra_slice_refresh:1,
					frequency_truncation:2,
					fillOutThisStruct:24;
	}					ff,		/* trick_mode_control == TRICK_MODE_FAST_FORWARD */
						fr;		/* trick_mode_control == TRICK_MODE_FAST_REVERSE */
	struct {
		unsigned	trick_mode_control:3,
					field_rep_cntrl:5,
					fillOutThisStruct:24;
	}					sloMo;	/* trick_mode_control == TRICK_MODE_SLOW_MOTION */
	struct {
		unsigned	trick_mode_control:3,
					field_id:2,
					reserved:3,
					fillOutThisStruct:24;
	}					freeze;	/* trick_mode_control == TRICK_MODE_FREEZE_FRAME */
} PES_PacketPart4, *PES_PacketPart4Ptr;

#define PES_PACKET_PART4_CONTENT_SIZE		(sizeof(PES_PacketPart4) - 24/8)


	/* if (additional_copy_info_flag) then also: */
typedef struct PES_PacketPart5 {
	unsigned		mb0:1,
					additional_copy_info:7,
					fillOutThisStruct:24;
} PES_PacketPart5, *PES_PacketPart5Ptr;

#define PES_PACKET_PART5_CONTENT_SIZE		(sizeof(PES_PacketPart5) - 24/8)


	/* if (PES_CRC_flag) then also: */
typedef struct PES_PacketPart6 {
	unsigned		previous_PES_packet_CRC:16,
					fillOutThisStruct:16;
} PES_PacketPart6, *PES_PacketPart6Ptr;

#define PES_PACKET_PART6_CONTENT_SIZE		(sizeof(PES_PacketPart6) - 16/8)


	/* if (PES_extension_flag) then also: */
typedef struct PES_PacketPart7 {
	unsigned		PES_private_data_flag:1,
					pack_header_field_flag:1,
					program_packet_sequence_counter_flag:1,
					P_STD_buffer_flag:1,
					reserved:3,
					PES_extension_field_flag:1,
					fillOutThisStruct:24;
} PES_PacketPart7, *PES_PacketPart7Ptr;

#define PES_PACKET_PART7_CONTENT_SIZE		(sizeof(PES_PacketPart7) - 24/8)


	/* if (PES_private_data_flag) then also: */
typedef struct PES_PacketPart8 {
	char		PES_private_data[16];	/* may not contain the sequence $00 00 01 */
} PES_PacketPart8, *PES_PacketPart8Ptr;

#define PES_PACKET_PART8_CONTENT_SIZE		(sizeof(PES_PacketPart8))


	/* if (pack_header_field_flag) then also: */
typedef uint8	PES_PacketPart9_Prefix;	/* pack_field_length */
	/* ... followed by a pack_header() that's pack_field_length bytes long. The
	 * pack_header() is really for Program Streams.
	 *
	 * [TBD] Is this really a 16-bit or an 8-bit field? The ISO/IEC 13818-1: CD
	 *    spec, Dec. 1993, is inconsistent on this point! */

#define PES_PACKET_PART9_PREFIX_CONTENT_SIZE	(sizeof(PES_PacketPart9_Prefix))


	/* if (program_packet_sequence_counter_flag) then also: */
typedef struct PES_PacketPart10 {
	unsigned		mb0:1,
					program_packet_sequence_counter:7,
					mb1:1,
					original_stuff_length:7,
					fillOutThisStruct:16;
} PES_PacketPart10, *PES_PacketPart10Ptr;

#define PES_PACKET_PART10_CONTENT_SIZE		(sizeof(PES_PacketPart10) - 16/8)


	/* if (P_STD_buffer_flag) then also: */
typedef struct PES_PacketPart11 {
	unsigned		b01:2,
					P_STD_buffer_scale:1,	/* Program Stream only */
					P_STD_buffer_size:13,
					fillOutThisStruct:16;
} PES_PacketPart11, *PES_PacketPart11Ptr;

#define PES_PACKET_PART11_CONTENT_SIZE		(sizeof(PES_PacketPart11) - 16/8)


	/* if (PES_extension_field_flag) then also: */
typedef struct PES_PacketPart12 {
	unsigned		mb0:1,
					PES_extension_field_length:7,
	/* ... followed by PES_extension_field_length reserved bytes */
					fillOutThisStruct:24;
} PES_PacketPart12, *PES_PacketPart12Ptr;

#define PES_PACKET_PART12_CONTENT_SIZE		(sizeof(PES_PacketPart12) - 24/8)

/* ... followed by [0 .. MAX_PES_PACKET_STUFFING_BYTES] stuffing_byte(s) */
#define MAX_PES_PACKET_STUFFING_BYTES		32

/* The max number of non-payload bytes include the fixed header, optional fields up
 * to the max PES_header_data_length value, and the max number of stuffing bytes. */
#define MAX_PES_NON_PAYLOAD_BYTES		(PES_PACKET_PART1_CONTENT_SIZE2 + 255 + 32)

/* ... followed by N PES_packet_data_byte(s) */


/*********************************************************************
 * ISO 11172-2 MPEG-1 Video layer structure definitions.
 *********************************************************************
 */

#define MPEG_VIDEO_SEQUENCE_HEADER_CODE			0x000001B3L

enum MPEGVideoFrameRate {
	vfr_null		=  0,	/* unknown (a forbidden frame rate value) */
	vfr_23_976		=  1,	/* 23.976 Hz	*** allowed in WhiteBook VideoCD *** */
	vfr_24			=  2,	/* 24 Hz */
	vfr_25			=  3,	/* 25 Hz		*** allowed in WhiteBook VideoCD *** */
	vfr_29_97		=  4,	/* 29.97 Hz		*** allowed in WhiteBook VideoCD *** */
	vfr_30			=  5,	/* 30 Hz */
	vfr_50			=  6,	/* 50 Hz */
	vfr_59_94		=  7,	/* 59.94 Hz */
	vfr_60			=  8	/* 60 Hz */
	/* values [9 .. 15] are reserved by the MPEG spec */
};

typedef struct VideoSequenceHeader {
	uint32			sequenceHeaderCode;		/* == MPEG_VIDEO_SEQUENCE_HEADER_CODE */
	unsigned		horizontal_size:12,
					vertical_size:12,
					pel_aspect_ratio:4,
					picture_rate:4;			/* cf. MPEGVideoFrameRate */
	unsigned		bit_rate:18,
					mb0:1,
					vbv_buffer_size:10,
					constrained_parameters_flag:1,
					load_intra_quantizer_matrix:1,
					maybe_load_non_intra_quantizer_matrix:1; /* holds
						* load_non_intra_quantizer_matrix if load_intra_quantizer_matrix ==
						* 0, else holds the first bit of 64-byte intra_quantizer_matrix */
	/* more fields follow with wacky alignment */
} VideoSequenceHeader, *VideoSequenceHeaderPtr;

#define VIDEO_SEQUENCE_HEADER_CONTENT_SIZE	(sizeof(VideoSequenceHeader))


/*********************************************************************
 * ISO 11172-3 MPEG-1 Audio layer structure definitions.
 *********************************************************************
 */

#define MPEG_SAMPLES_PER_AUDIO_FRAME			1152

#define MPEG_AUDIO_SYNCWORD			0xFFF
#define MPEG_AUDIO_SYNCWORD_BYTE_1	0xFF	/* 1st byte of a Syncword, for searching */
#define ID_ISO_11172_3_AUDIO		1

enum MPEGAudioLayer {				/* Audio "Layer" is the compression complexity level */
	audioLayer_reserved	= 0,
	audioLayer_III		= 1,
	audioLayer_II		= 2,		/* Layer II	*** allowed in WhiteBook VideoCD *** */
	audioLayer_I		= 3
};

enum MPEGAudioSamplingFreq {
	aFreq_44100_Hz	= 0,		/* 44.1 kHz		*** allowed in WhiteBook VideoCD *** */
	aFreq_48000_Hz	= 1,		/* 48 kHz */
	aFreq_32000_Hz	= 2,		/* 32 kHz */
	aFreq_reserved	= 3			/* MPEG reserved value */
}; 

enum MPEGAudioMode {
	aMode_stereo				= 0,	/* */
	aMode_joint_stereo			= 1,	/* joint_stereo, intensity_stereo, or ms_stereo */
	aMode_dual_channel			= 2,	/* */
	aMode_single_channel		= 3		/*	*** not allowed in WhiteBook VideoCD *** */
};

enum MPEGAudioEmphasisMode {
	aEmphasisMode_none			= 0,	/* no emphasis			*** allowed in WhiteBook VideoCD *** */
	aEmphasisMode_50_15			= 1,	/* 50/15 microseconds	*** allowed in WhiteBook VideoCD *** */
	aEmphasisMode_reserved		= 2,	/* reserved */
	aEmphasisMode_CCITT_J_17	= 3		/* CCITT J.17 emphasis */
};

typedef struct AudioHeader {
	unsigned	syncword:12,		/* == MPEG_AUDIO_SYNCWORD */
				id:1,				/* == ID_ISO_11172_3_AUDIO */
				layer:2,			/* must be audioLayer_II in WhiteBook VideoCD */
				protection_bit:1,	/* 0 => error concealment redundancy was added to the audio bitstream */
				bitrate_index:4,	/* index into bitrate table */
				sampling_frequency:2,	/* cf. MPEGAudioSamplingFreq enum */
				padding_bit:1,		/* should be 1 (padding on) with 44.1 kHz sampling */
				private_bit:1,		/* for private use, not used by ISO */
				mode:2,				/* cf. MPEGAudioMode enum */
				mode_extension:2,	/* see the MPEG spec */
				copyright:1,		/* 1 => copyrighted audio stream */
				original_or_copy:1,	/* 0 => "the bitstream is a copy" */
				emphasis:2;			/* cf MPEGAudioEmphasisMode enum */
} AudioHeader, *AudioHeaderPtr;

#define AUDIO_HEADER_CONTENT_SIZE	(sizeof(AudioHeader))


#ifdef cplusplus
extern "C" {
#endif /* cplusplus */


/*********************************************************************
 * Functions
 *********************************************************************
 */



#ifdef cplusplus
}
#endif /* cplusplus */

#endif /* __MPEG_H__ */
