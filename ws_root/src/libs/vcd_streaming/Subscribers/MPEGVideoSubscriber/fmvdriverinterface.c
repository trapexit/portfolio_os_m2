/******************************************************************************
**
**  @(#) fmvdriverinterface.c 96/08/01 1.2
**
******************************************************************************/

#include <string.h>				/* for memset */
#include <stdlib.h>

#include <kernel/types.h>
#include <kernel/debug.h>
#include <kernel/io.h>
#include <kernel/mem.h>
#include <device/mpegvideo.h>

#include "fmvdriverinterface.h"
#include <streaming/dserror.h>


/*******************************************************************************************
 * Local functions
 *******************************************************************************************/


/*******************************************************************************************
 * Send a TagArg list of control commands to the MPEG driver.
 * ASSUMES: The device has been opened and the ioReqItem created for it.
 * RETURNS: DoIO() status result.
 *******************************************************************************************/
static Err	FMVDoControlCmd(FMVDeviceHandle *device, CODECDeviceStatus *stat)
	{
	IOInfo	ioi;				/* temp IOInfo for sending a command to the driver */

	if ( device == NULL )
		return kDSBadPtrErr;

	memset(&ioi, 0, sizeof(ioi));
	ioi.ioi_Command			= MPEGVIDEOCMD_CONTROL;
	ioi.ioi_Send.iob_Buffer	= stat;
	ioi.ioi_Send.iob_Len	= sizeof(CODECDeviceStatus);

	/* Send the command. */
	return DoIO(device->cmdIOReqItem, &ioi);
	}


/*******************************************************************************************
 * Public functions
 *******************************************************************************************/


Item	FMVCreateIOReq(FMVDeviceHandle *device, uint32 completionSignal)
	{
	if ( device == NULL )
		return kDSBadPtrErr;

	return CreateItemVA(MKNODEID(KERNELNODE,IOREQNODE),
		CREATEIOREQ_TAG_DEVICE,		(void *)device->deviceItem,
	    CREATEIOREQ_TAG_SIGNAL,		(void *)completionSignal,
		TAG_END);
	}


Err		FMVGetPTS(IOReq *ioReqItemPtr, uint32 *ptsValue, uint32 *userData)
	{
	if ( ioReqItemPtr == NULL )
		return kDSBadPtrErr;

	if ( (ioReqItemPtr->io_Flags & FMVValidPTS) != 0 )
		{
		*ptsValue  = (uint32)FMVGetPTSValue(ioReqItemPtr);
		*userData  = (uint32)FMVGetUserData(ioReqItemPtr);

		return 0;
		}

	*ptsValue = 0;
	return kDSMPEGDevPTSNotValidErr;
	}


Err		FMVOpenVideo(FMVDeviceHandle *device, int32 bitsPerPixel,
			int32 horizSize, int32 vertSize, uint8 resampleMode)
	{
	Err						status;
	int32					nextTag;
	List					*list;
	CODECDeviceStatus		stat;	/* TagArgs to set driver status */

	if ( device == NULL )
		return kDSBadPtrErr;

	device->deviceItem = device->cmdIOReqItem = -1;

	/* Check some arg values */
	if ( (bitsPerPixel != 24) && (bitsPerPixel != 16) )
		return kDSBadFrameBufferDepthErr;

	/* Open the device driver and get its Item number */
	status = CreateDeviceStackListVA(&list,
				"cmds", DDF_EQ, DDF_INT, 1, MPEGVIDEOCMD_CONTROL,
				NULL);
	if ( status < 0 )
		return status;
	if ( IsEmptyList(list) )
		{
		/* No MPEG devices found! */
		DeleteDeviceStackList(list);
		return kDSInitErr;
		}
	/* [TBD] FIXME: If there's more than one, just pick the first one? */
	device->deviceItem = status =
		OpenDeviceStack((DeviceStack*)FirstNode(list));
	DeleteDeviceStackList(list);
	if ( status < 0 )
		goto ERROR;

	/* Create an IOReq item to use for control commands to the driver */
	/* We MUST do this before calling FMVDoControlCmd() */
	device->cmdIOReqItem = status = FMVCreateIOReq(device, 0);
	if ( status < 0 )
		goto ERROR;

	/* On Opera 16-bit mode, and all M2 modes, only square resampling works. */
	/* if (bitsPerPixel == 16) */	/* <-- comment this line out for M2; uncomment for Opera */
		resampleMode = kCODEC_SQUARE_RESAMPLE;	/* resampling croaks in 16-bit DMA mode */

	/* Set the device's status */
	memset(&stat, 0, sizeof(stat));
	nextTag = 0;

	stat.codec_TagArg[nextTag].ta_Tag   = VID_CODEC_TAG_STANDARD;
	stat.codec_TagArg[nextTag++].ta_Arg = (void *)resampleMode;

	stat.codec_TagArg[nextTag].ta_Tag   = VID_CODEC_TAG_HSIZE;
	stat.codec_TagArg[nextTag++].ta_Arg = (void *)horizSize;

	stat.codec_TagArg[nextTag].ta_Tag   = VID_CODEC_TAG_VSIZE;
	stat.codec_TagArg[nextTag++].ta_Arg = (void *)vertSize;

	stat.codec_TagArg[nextTag].ta_Tag   = VID_CODEC_TAG_DEPTH;
	stat.codec_TagArg[nextTag++].ta_Arg = (void *)bitsPerPixel;

	stat.codec_TagArg[nextTag].ta_Tag   = VID_CODEC_TAG_M2MODE;

	stat.codec_TagArg[nextTag].ta_Tag   = TAG_END;

	status = FMVDoControlCmd(device, &stat);

	if ( status < 0 )
		goto ERROR;

	return status;


ERROR:
	PERR(("Error initializing MPEG Video device %ld\n", status));

	/* Cleanup */
	FMVCloseDevice(device);
	return status;
	}


Err		FMVCloseDevice(FMVDeviceHandle *device)
	{
	Err		status;

	if ( device == NULL )
		return kDSBadPtrErr;

	FMVDeleteIOReq(device->cmdIOReqItem);
	device->cmdIOReqItem = -1;

	status = CloseDeviceStack(device->deviceItem);
	device->deviceItem = -1;

	return status;
	}


Err		FMVWriteBuffer(FMVDeviceHandle *device, void *buffer,
			int32 numBytes, Item ioReqItem, FMVIOReqOptionsPtr fmvOptionsPtr,
			uint32 fmvFlags, uint32 ptsValue, uint32 userData)
	{
	IOInfo		ioi;

	TOUCH(device);			/* avert a compiler warning */

	/* setup the *fmvOptionsPtr structure (if it's not NULL) */
	if (fmvOptionsPtr != NULL)
		{
		memset(fmvOptionsPtr, 0, sizeof(FMVIOReqOptions));
		fmvOptionsPtr->FMVOpt_Flags			= fmvFlags;
		fmvOptionsPtr->FMVOpt_PTS			= ptsValue;
		fmvOptionsPtr->FMVOpt_UserData		= userData;
		}

	/* initialize the IOInfo for a write to the MPEG video device. */
	memset(&ioi, 0, sizeof(ioi));
	ioi.ioi_Command				= MPEGVIDEOCMD_WRITE;
	ioi.ioi_CmdOptions 			= (uint32)fmvOptionsPtr;
	ioi.ioi_Send.iob_Buffer 	= buffer;
	ioi.ioi_Send.iob_Len 		= numBytes /* & ~1 */;	/* Must not be odd on Opera. */

	return SendIO(ioReqItem, &ioi);
	}


Err		FMVSetVideoMode(FMVDeviceHandle *device, int32 mode, int32 argument)
	{
	CODECDeviceStatus	stat;	/* contains TagArgs for sending set-status cmds to driver */

	/* Send the command to the driver */
	memset(&stat, 0, sizeof(stat));
	stat.codec_TagArg[0].ta_Tag = mode;
	stat.codec_TagArg[0].ta_Arg = (void *)argument;
	stat.codec_TagArg[1].ta_Tag = TAG_END;
	return FMVDoControlCmd(device, &stat);
	}


Err		FMVSetVideoSize(FMVDeviceHandle *device, int32 bitsPerPixel,
			int32 horizSize, int32 vertSize, uint8 resampleMode)
	{
	int32				nextTag;
	CODECDeviceStatus	stat;	/* TagArgs for sending set-status cmds to driver */

	/* Check some arg validity */
	if ( (bitsPerPixel != 24) && (bitsPerPixel != 16) )
		return kDSBadFrameBufferDepthErr;

	/* Set the device's status */
	nextTag = 0;
	memset(&stat, 0, sizeof(stat));
	stat.codec_TagArg[nextTag].ta_Tag = VID_CODEC_TAG_STANDARD;
	if (bitsPerPixel == 16)
		resampleMode = kCODEC_SQUARE_RESAMPLE;	/* resampling croaks in 16-bit DMA mode */
	stat.codec_TagArg[nextTag++].ta_Arg = (void *)resampleMode;

	stat.codec_TagArg[nextTag].ta_Tag = VID_CODEC_TAG_HSIZE;
	stat.codec_TagArg[nextTag++].ta_Arg = (void *)horizSize;

	stat.codec_TagArg[nextTag].ta_Tag = VID_CODEC_TAG_VSIZE;
	stat.codec_TagArg[nextTag++].ta_Arg = (void *)vertSize;

	stat.codec_TagArg[nextTag].ta_Tag = VID_CODEC_TAG_DEPTH;
	stat.codec_TagArg[nextTag++].ta_Arg = (void *)bitsPerPixel;

	stat.codec_TagArg[nextTag].ta_Tag = TAG_END;

	return FMVDoControlCmd(device, &stat);
	}


#if 1	/* [TBD] Switch over to MPEGVIDEOCMD_READ */
Err		FMVReadVideoBuffer(void *buffer, int32 numBytes, Item ioReqItem)
	{
	IOInfo		ioi;

	/* initialize the IOInfo for reading frames from the MPEG video device */
	memset(&ioi, 0, sizeof(ioi));
	ioi.ioi_Command 			= CMD_READ;
	ioi.ioi_Recv.iob_Buffer 	= buffer;
	ioi.ioi_Recv.iob_Len 		= numBytes;

	return SendIO(ioReqItem, &ioi);
	}

#else

Err		FMVReadVideoBuffer(Item bitmapItem, Item ioReqItem)
	{
	IOInfo		ioi;

	/* initialize the IOInfo for reading frames from the MPEG video device */
	memset(&ioi, 0, sizeof(ioi));
	ioi.ioi_Command 			= MPEGVIDEOCMD_READ;
	ioi.ioi_Recv.iob_Buffer 	= &bitmapItem;
	ioi.ioi_Recv.iob_Len 		= sizeof(bitmapItem);

	return SendIO(ioReqItem, &ioi);
	}
#endif
