#ifndef __DEVICE_MPEGVIDEO_H
#define __DEVICE_MPEGVIDEO_H


/******************************************************************************
**
**  @(#) mpegvideo.h 96/12/11 1.8
**
**  FMV audio and video device interface and definitions
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_ITEM_H
#include <kernel/item.h>
#endif

#ifndef __KERNEL_DEVICE_H
#include <kernel/device.h>
#endif

#ifndef __KERNEL_DRIVER_H
#include <kernel/driver.h>
#endif

#ifndef __KERNEL_IO_H
#include <kernel/io.h>
#endif

#ifndef __KERNEL_OPERROR_H
#include <kernel/operror.h>
#endif


#define FMV_DRIVER_NAME	      "mpegvideo"
#define FMV_VIDEO_DEVICE_NAME "mpegvideo"


/*=========================================================================
// Definition of IOReq and IOInfo IOReqOptions structure for FMV writes and
// reads.

//=========================================================================*/

typedef struct FMVIOReqOptions
{
	uint32		FMVOpt_Flags;
	uint32		FMVOpt_PTS;
	uint32		FMVOpt_PTS_Offset;
	uint32		FMVOpt_UserData;
	uint32		Reserved2;
	uint32		Reserved3;
} FMVIOReqOptions, *FMVIOReqOptionsPtr;

/*=========================================================================
// FMVValidPTS and FMVPTSHIGHBIT flags
//
// When System or Audio packet streams are inuse the PTS for the
// decompressed data can be retreived.  Contained in the first long of the
// "io_Extension" field is 32 bits of the first PTS contained within the
// data.  The second 32 bits contains the offset within the request that
// most closely correspond to the PTS.
//
// If a valid PTS was found the high bit in the "io_Flags" field is set.
// If this bit is cleared you  should not use the values in the extensions
// field.
//=========================================================================*/

#define FMVValidPTS				0x80000000
#define FMVPTSHIGHBIT			0x40000000

#define FMV_PTS_INDEX			0
#define FMV_PTS_OFFSET			1

#define FMVGetPTSValue(ior) ((ior)->io_Extension[FMV_PTS_INDEX])
#define FMVGetPTSOffset(ior) ((ior)->io_Extension[FMV_PTS_OFFSET])
#define FMVGetUserData(ior) ((ior)->io_Info.ioi_CmdOptions)

#define FMVSetPTSOffset(tsPtr, theOffset) \
	(((FMVIOReqOptions*)(tsPtr))->FMVOpt_PTS_Offset = (theOffset))
#define FMVSetPTSValue(tsPtr, thePTS) \
	(((FMVIOReqOptions*)(tsPtr))->FMVOpt_PTS = (thePTS))

/*=========================================================================
// FMV_END_OF_STREAM_FLAG & FMV_DISCONTINUITY_FLAG flag
//
// On a zero length write, the client can set the FMV_END_OF_STREAM_FLAG flag
// in the ((FMVIOReqOptions*)ioInfo->ioi_CmdOptions)->FMVOpt_Flags to
// indicate that no more writes for this stream will be forthcoming,
// and therefore to finish any reads in progress (the audio drvier fills
// the remaining reads with zeros).
//
// [TBD] On a read, we may want the driver to set the FMV_END_OF_STREAM_FLAG
// flag in ioReq->io_Flags after the
// ((FMVIOReqOptions*)ioInfo->ioi_CmdOptions)->FMVOpt_Flagsflag has been set
// on a write and there is no more data to read.
//
// The FMV_DISCONTINUITY_FLAG is used similarly to tell the driver that
// a discontinuity in the compressed data is occuring, i.e., don't throw away
// any data, but here comes a discontinuity. [TBD] We may want the driver to
// then inform the user when a read completes that corresponds to the
// discontinuity.
//
// The FMV_FLUSH_FLAG is used similarly to tell the driver that a flush of
// the compressed data is occuring, i.e., throw away anything you can to make
// the transition as soon as possible. [TBD] We may want the driver to then
// inform the user when a read completes that corresponds to the flush.
//
// FMVFlags macro takes ioInfo->ioi_CmdOptions as an argument and does the
// appropriate casting and indirection to read the flags.
//=========================================================================*/

#define FMV_END_OF_STREAM_FLAG		0x20000000
#define FMV_DISCONTINUITY_FLAG		0x10000000
#define FMV_FLUSH_FLAG				0x08000000

#define FMVFlags(opt) ( (opt) != 0 ? ((FMVIOReqOptions*)(opt))->FMVOpt_Flags : 0 )

#define	kMAX_CODEC_ARGS		20		/* The total possible arguments returned */

/*=========================================================================
// CODEC device status structure is returned when a CMD_STATUS is made.
// The CODEC device arguments follow the standard device arguments.
//=========================================================================*/

typedef struct CODECDeviceStatus
{
	DeviceStatus	codec_ds;			/* Standard device status tag args */
	TagArg			codec_TagArg[kMAX_CODEC_ARGS];
} CODECDeviceStatus, *CODECDeviceStatusPtr;

/*=========================================================================
// Definition of the Video CODEC device tag arguments.
//=========================================================================*/

enum
{
	VRESERVED0 = TAG_ITEM_LAST+1,		/* reserved, do not remove! */
	VID_CODEC_TAG_HSIZE,				/* Horizontal output image size */
	VID_CODEC_TAG_VSIZE,				/* Vertical output image size */
	VID_CODEC_TAG_DEPTH,				/* Output image depth */
	VID_CODEC_TAG_DITHER,				/* The dither method */
	VID_CODEC_TAG_STANDARD,				/* Resampling method */
	VRESERVED1,							/* reserved, do not remove! */
	VID_CODEC_TAG_PLAY,					/* Decode all pictures */
	VID_CODEC_TAG_SKIPFRAMES,			/* Skip n pictures */
	VID_CODEC_TAG_KEYFRAMES,			/* Decode only I pictures */
	VRESERVED2,							/* reserved, do not remove! */
	VRESERVED3, 						/* reserved, do not remove! */
	VID_CODEC_TAG_YUVMODE,				/* output yuv pixels */
	VID_CODEC_TAG_RGBMODE,				/* output rgb pixels */
	VID_CODEC_TAG_M2MODE				/* output m2 format pixels */
};

#define	kCODEC_SQUARE_RESAMPLE	0x00
#define	kCODEC_NTSC_RESAMPLE	0x01
#define	kCODEC_PAL_RESAMPLE		0x02

#define kCODEC_RANDOM_DITHER	0x00
#define	kCODEC_MATRIX_DITHER	0x01

/*=========================================================================
// Definition of the structures used with the commands:
//	MPEGVIDEOCMD_GETBUFFERINFO
//	MPEGVIDEOCMD_SETSTRIPBUFFER
//	MPEGVIDEOCMD_SETREFERENCEBUFFER
//	MPEGVIDEOCMD_ALLOCBUFFERS
//=========================================================================*/

typedef struct MPEGBufferStreamInfo {
	uint32	fmvWidth;
	uint32	fmvHeight;
	uint32	stillWidth;
	uint32	stillHeight;
} MPEGBufferStreamInfo;

typedef struct MPEGBufferAllocInfo {
	uint32	minSize;
	uint32	memFlags;
	uint32	memCareBits;
	uint32	memStateBits;
} MPEGBufferAllocInfo;

typedef struct MPEGBufferInfo {
	MPEGBufferAllocInfo	refBuffer;
	MPEGBufferAllocInfo	stripBuffer;
} MPEGBufferInfo;

#endif /* __DEVICE_MPEGVIDEO_H */
