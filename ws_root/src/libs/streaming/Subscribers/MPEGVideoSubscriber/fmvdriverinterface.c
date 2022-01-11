/******************************************************************************
**
**  @(#) fmvdriverinterface.c 96/02/21 1.16
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


/************************************************************************
|||	AUTODOC -public -class Streaming -group MPEG -name FMVGetDeviceItem
|||	Return an FMVDeviceHandle's device Item.
|||
|||	  Synopsis
|||
|||	    Item FMVGetDeviceItem(FMVDeviceHandle *anFMVDeviceHandlePtr)
|||
|||	  Description
|||
|||	    Return an FMVDeviceHandle's device Item. Use this in case you need to
|||	    operate directly on the Item. (Normally the fmvdriverinterface.h
|||	    routines will suffice and you won't need to operate on the Item
|||	    directly.)
|||
|||	  Arguments
|||
|||	    anFMVDeviceHandlePtr
|||	        A pointer to an FMVDeviceHandle structure.
|||
|||	  Implementation
|||
|||	    Macro call implemented in "fmvdriverinterface.h".
|||
|||	  Associated Files
|||
|||	    "fmvdriverinterface.h", libsubscriber.a
|||
|||	  See Also
|||
|||	    FMVOpenVideo().
|||
************************************************************************/


/************************************************************************
|||	AUTODOC -public -class Streaming -group MPEG -name FMVCreateIOReq
|||	Create an MPEG I/O request setup to signal on I/O completion.
|||
|||	  Synopsis
|||
|||	    Item FMVCreateIOReq(FMVDeviceHandle *device, uint32 completionSignal)
|||
|||	  Description
|||
|||	    This function creates an MPEG device I/O request Item. The Item will
|||	    send you a signal on I/O completion, either completionSignal or
|||	    SIGF_IODONE (if completionSignal is 0).
|||
|||	    This is a low-overhead notification mechanism. It doesn't tell you which
|||	    I/O request(s) completed, but it doesn't matter since you should process
|||	    I/O completions in request order. That is, keep a queue of your
|||	    outstanding I/O requests, and when the signal arrives, process any and
|||	    all completed I/O requests at the head of the queue.
|||
|||	  Arguments
|||
|||	    device
|||	        A pointer to an FMVDeviceHandle.
|||
|||	    completionSignal
|||	        The completion signal mask (a 1-bit mask identifying a signal number)
|||	        or 0 (to use the pre-allocated signal SIGF_IODONE).
|||
|||	  Return Value
|||
|||	    The Item number of the new I/O request or one of the
|||	    following negative error codes if an error occurs:
|||
|||	    kDSBadPtrErr (defined in dserror.h)
|||	        The device argument was NULL.
|||
|||	    ER_Kr_ItemNotOpen
|||	        The device specified by the dev argument is not open.
|||
|||	    NOMEM
|||	        There was not enough memory to complete the operation.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libsubscriber.a.
|||
|||	  Associated Files
|||
|||	    "fmvdriverinterface.h", libsubscriber.a
|||
|||	  Notes
|||
|||	    When you no longer need an I/O request, use FMVDeleteIOReq() to delete
|||	    it.
|||
|||	  See Also
|||
|||	    FMVOpenVideo(), FMVDeleteIOReq() FMVWriteBuffer(), FMVReadVideoBuffer().
|||
************************************************************************/
Item	FMVCreateIOReq(FMVDeviceHandle *device, uint32 completionSignal)
	{
	if ( device == NULL )
		return kDSBadPtrErr;

	return CreateItemVA(MKNODEID(KERNELNODE,IOREQNODE),
		CREATEIOREQ_TAG_DEVICE,		(void *)device->deviceItem,
	    CREATEIOREQ_TAG_SIGNAL,		(void *)completionSignal,
		TAG_END);
	}


/************************************************************************
|||	AUTODOC -public -class Streaming -group MPEG -name FMVDeleteIOReq
|||	Delete an MPEG I/O request.
|||
|||	  Synopsis
|||
|||	    Err FMVDeleteIOReq(Item item)
|||
|||	  Description
|||
|||	    This macro deletes an MPEG device I/O request Item created by
|||	    FMVCreateIOReq().
|||
|||	  Arguments
|||
|||	    item
|||	        The IOReq Item created by FMVCreateIOReq().
|||
|||	  Return Value
|||
|||	    0 or a negative error code.
|||
|||	  Implementation
|||
|||	    Macro call implemented in "fmvdriverinterface.h".
|||
|||	  Associated Files
|||
|||	    "fmvdriverinterface.h", libsubscriber.a
|||
|||	  See Also
|||
|||	    FMVOpenVideo(), FMVCreateIOReq().
|||
************************************************************************/


/************************************************************************
|||	AUTODOC -public -class Streaming -group MPEG -name FMVGetPTS
|||	Get the PTS data from a completed MPEG video device read request.
|||
|||	  Synopsis
|||
|||	    Err FMVGetPTS(IOReq *ioReqItemPtr, uint32 *ptsValue, uint32 *userData)
|||
|||	  Description
|||
|||	    A "PTS" is an MPEG Presentation Time Stamp. It tells when to present
|||	    the decoded video data. It's mainly useful
|||	    to synchronize the video and audio presentation.
|||
|||	    A PTS value is in 90 kHz MPEG ticks. The MPEG device returns the
|||	    low-order 32 bits of PTS, although MPEG defines it as a 33-bit field.
|||
|||	    A user data value can be passed through the decoder along with the PTS
|||	    value. For instance the Data Streamer uses it for a "branch
|||	    number" value, so <branchNumber, PTS> is monotonically increasing even
|||	    if the program jumps backwards in the stream or switches streams.
|||
|||	    FMVGetPTS() gets the PTS data from a completed MPEG video device read
|||	    request. You supply storage for the results.
|||
|||	  Arguments
|||
|||	    ioReqItemPtr
|||	        ptr to the looked-up IOReq of a completed MPEG device read request.
|||
|||	    ptsValue
|||	        Address to store the PTS value into.
|||
|||	    userData
|||	        Address to store the user data value into.
|||
|||	  Return Value
|||
|||	    0 if successful.
|||
|||	    kDSMPEGDevPTSNotValidErr if this read request didn't
|||	    emit a valid PTS.
|||
|||	    kDSMPEGDevPTSNotValidErr is an info value, not an
|||	    error condition. Once the MPEG video decoder has a valid input PTS, it
|||	    will emit a valid PTS on every decoded frame for the rest of the
|||	    contiguous sequence.
|||
|||	    In *ptsValue, the low-order 32-bits of the PTS value, or 0 if the
|||	    read request has no valid PTS value.
|||
|||	    In *userData, the user data value passed through the decoder along with
|||	    the PTS value (or unchanged if the read request has no valid PTS value).
|||	    Cf. FMVWriteBuffer().
|||
|||	  Implementation
|||
|||	    Link library call implemented in libsubscriber.a.
|||
|||	  Associated Files
|||
|||	    "fmvdriverinterface.h", libsubscriber.a
|||
|||	  See Also
|||
|||	    FMVOpenVideo(), FMVWriteBuffer(), FMVReadVideoBuffer().
|||
************************************************************************/
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


/************************************************************************
|||	AUTODOC -public -class Streaming -group MPEG -name FMVOpenVideo
|||	Open a handle to an MPEG video device channel.
|||
|||	  Synopsis
|||
|||	    Err FMVOpenVideo(FMVDeviceHandle *device, int32 bitsPerPixel,
|||	        int32 horizSize, int32 vertSize, uint8 resampleMode)
|||
|||	  Description
|||
|||	    The module "fmvdriverinterface" provides a slightly higher level
|||	    interface to the MPEG device driver. It also provides some isolation
|||	    from change, e.g. the rest of the streaming software didn't have to
|||	    change when the device was renamed from the "FMV" device to the
|||	    "MPEG" device.
|||
|||	    This function opens and initializes a handle to an MPEG driver video
|||	    channel.
|||
|||	    To close the FMVDeviceHandle, call FMVCloseDevice().
|||
|||	    See FMVSetVideoSize() for more documentation on the input parameters.
|||
|||	  Arguments
|||
|||	    device
|||	        FMVDeviceHandle* to fill in; it should already be initialized.
|||
|||	    bitsPerPixel
|||	        16 or 24.
|||
|||	    horizSize
|||	        Typically 320 (NTSC) or 384 (PAL).
|||
|||	    vertSize
|||	        Typically 240 (NTSC) or 288 (PAL).
|||
|||	    resampleMode
|||	        kCODEC_SQUARE_RESAMPLE noop resampling on M2 or Opera,
|||
|||	        kCODEC_NTSC_RESAMPLE 352 -> 320 resampling on Opera only, or
|||
|||	        kCODEC_PAL_RESAMPLE 352 -> 384 resampling on Opera only.
|||
|||	        These constants are defined in <device/mpegvideo.h>.
|||
|||	  Return Value
|||
|||	    0 if successful, a negative error code if unsuccessful.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libsubscriber.a.
|||
|||	  Associated Files
|||
|||	    "fmvdriverinterface.h", libsubscriber.a
|||
|||	  See Also
|||
|||	    FMVCloseDevice().
|||
************************************************************************/
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


/************************************************************************
|||	AUTODOC -public -class Streaming -group MPEG -name FMVCloseDevice
|||	Close an MPEG video device handle.
|||
|||	  Synopsis
|||
|||	    Err FMVCloseDevice(FMVDeviceHandle *device)
|||
|||	  Description
|||
|||	    Close an MPEG video device handle. It's ok to call this if the
|||	    device wasn't successfully opened.
|||
|||	  Arguments
|||
|||	    device
|||	        A pointer to an FMVDeviceHandle to close.
|||
|||	  Return Value
|||
|||	    kDSBadPtrErr (defined in dserror.h) if the device argument was NULL,
|||	    else the Portfolio Err code from the device CloseItem() call.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libsubscriber.a.
|||
|||	  Associated Files
|||
|||	    "fmvdriverinterface.h", libsubscriber.a
|||
|||	  See Also
|||
|||	    FMVOpenVideo().
|||
************************************************************************/
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


/************************************************************************
|||	AUTODOC -public -class Streaming -group MPEG -name FMVWriteBuffer
|||	Write a buffer of compressed data to the MPEG video device.
|||
|||	  Synopsis
|||
|||	    Err FMVWriteBuffer(FMVDeviceHandle *device, void *buffer,
|||	        int32 numBytes, Item ioReqItem, FMVIOReqOptionsPtr fmvOptionsPtr,
|||	        uint32 fmvFlags, uint32 ptsValue, uint32 userData)
|||
|||	  Description
|||
|||	    Begin an asynchronous write of a buffer of compressed data, with
|||	    optional flags and a PTS, to the MPEG video decoder device.
|||	    When the I/O completes, your task will receive a signal. The signal
|||	    bit you get is the one you passed to FMVCreateIOReq().
|||
|||	    See FMVGetPTS() for an explanation about the PTS value and userData.
|||
|||	  Arguments
|||
|||	    device
|||	        An open video FMVDeviceHandle*.
|||
|||	    buffer
|||	        Address of the buffer of compressed data to write to an MPEG device.
|||
|||	    numBytes
|||	        number of data bytes in the buffer.
|||
|||	    ioReqItem
|||	        MPEG video IOReq Item from FMVCreateIOReq().
|||
|||	    fmvOptionsPtr
|||	        ptr to FMVIOReqOptions storage. This is used to pass PTS and flags
|||	        to the MPEG decoder. Its contents on entry don't matter, but the
|||	        storage must remain accessible to the MPEG device until this I/O
|||	        request completes. NULL is ok if you have no PTS or flags to pass in.
|||
|||	    fmvFlags
|||	        MPEG driver flags such as a combination of FMVValidPTS, FMVPTSHIGHBIT,
|||	        FMV_END_OF_STREAM_FLAG, FMV_DISCONTINUITY_FLAG, and FMV_FLUSH_FLAG.
|||
|||	    ptsValue
|||	        The low order 32 PTS bits; irrelevant if !(fmvFlags & FMVValidPTS).
|||
|||	    userData
|||	        A user data value to pass through the decoder along with ptsValue.
|||
|||	  Return Value
|||
|||	    status code from SendIO().
|||
|||	  Implementation
|||
|||	    Link library call implemented in libsubscriber.a.
|||
|||	  Associated Files
|||
|||	    "fmvdriverinterface.h", libsubscriber.a
|||
|||	  See Also
|||
|||	    FMVOpenVideo(), FMVCreateIOReq(), FMVReadVideoBuffer(), FMVGetPTS().
|||
************************************************************************/
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


/************************************************************************
|||	AUTODOC -public -class Streaming -group MPEG -name FMVSetVideoMode
|||	Set MPEG video device modes.
|||
|||	  Synopsis
|||
|||	    Err  FMVSetVideoMode(FMVDeviceHandle *device, int32 mode,
|||	        int32 argument)
|||
|||	  Description
|||
|||	    Set MPEG video device modes.
|||
|||	  Arguments
|||
|||	    device
|||	        An open video FMVDeviceHandle*.
|||
|||	    mode
|||	        VID_CODEC_TAG_PLAY to cancel decode-only-I-frames mode, or
|||
|||	        VID_CODEC_TAG_SKIPFRAMES to skip <argument> number of B frames,
|||	        more precisely, to add <argument> to the skip counter (there's no
|||	        way to clear this counter short of FMVCloseVideo), or
|||
|||	        VID_CODEC_TAG_KEYFRAMES to go into decode-only-I-frames mode, or
|||
|||	        etc.
|||
|||	    argument
|||	        The argument to the mode being set, e.g. the number of additional B
|||	        frames to skip if mode == VID_CODEC_TAG_SKIPFRAMES.
|||
|||	  Return Value
|||
|||	    Error code from DoIO().
|||
|||	  Assumes
|||
|||	    The channel was opened via FMVOpenVideo().
|||
|||	  Implementation
|||
|||	    Link library call implemented in libsubscriber.a.
|||
|||	  Associated Files
|||
|||	    "fmvdriverinterface.h", libsubscriber.a
|||
|||	  See Also
|||
|||	    FMVOpenVideo(), FMVCreateIOReq().
|||
************************************************************************/
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


/************************************************************************
|||	AUTODOC -public -class Streaming -group MPEG -name FMVSetVideoSize
|||	Set the size parameters for an open MPEG video channel.
|||
|||	  Synopsis
|||
|||	    Err FMVSetVideoSize(FMVDeviceHandle *device, int32 bitsPerPixel,
|||	        int32 horizSize, int32 vertSize, uint8 resampleMode)
|||
|||	  Description
|||
|||	    Set the size parameters for an open MPEG video channel.
|||
|||	  Arguments
|||
|||	    device
|||	        An open MPEG video FMVDeviceHandle*.
|||
|||	    bitsPerPixel
|||	        16 or 24.
|||
|||	    horizSize
|||	        Typically 320 (NTSC) or 384 (PAL).
|||
|||	    vertSize
|||	        Typically 240 (NTSC) or 288 (PAL).
|||
|||	    resampleMode
|||	        kCODEC_SQUARE_RESAMPLE noop resampling on M2 or Opera,
|||
|||	        kCODEC_NTSC_RESAMPLE 352->320 resampling on Opera only, or
|||
|||	        kCODEC_PAL_RESAMPLE 352->384 resampling on Opera only.
|||
|||	        These constants are defined in <device/mpegvideo.h>.
|||
|||	  Return Value
|||
|||	    Error code from DoIO().
|||
|||	  Assumes
|||
|||	    The channel was opened via FMVOpenVideo().
|||
|||	  Implementation
|||
|||	    Link library call implemented in libsubscriber.a.
|||
|||	  Associated Files
|||
|||	    "fmvdriverinterface.h", libsubscriber.a
|||
|||	  Notes
|||
|||	     Only some combinations of picture sizes and resampling modes are
|||	     available. The M2 MPEG device does not support resampling, but
|||	     the Triangle Engine can resample the decoded video frames.
|||
|||	  See Also
|||
|||	    FMVOpenVideo(), FMVCreateIOReq().
|||
************************************************************************/
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


/************************************************************************
|||	AUTODOC -public -class Streaming -group MPEG -name FMVReadVideoBuffer
|||	Read one "frame" of decompressed MPEG video data.
|||
|||	  Synopsis
|||
|||	    Err FMVReadVideoBuffer(void *buffer, int32 numBytes, Item ioReqItem)
|||
|||	  Description
|||
|||	    This issues an asynchronous I/O request to read one "frame" of
|||	    decompressed MPEG video data from the MPEG device into the
|||	    supplied buffer. After the I/O completes, you can call FMVGetPTS() to
|||	    get the data's PTS (MPEG Presentation Timestamp).
|||
|||	    Each read request returns a separate video frame. If the buffer isn't
|||	    large enough, the driver will return part of the frame.
|||
|||	  Arguments
|||
|||	    buffer
|||	        Address of the data buffer to read decompressed MPEG data into.
|||
|||	    numBytes
|||	        number of data bytes in the buffer, must be a multiple of 2.
|||
|||	    ioReqItem
|||	        MPEG IOReq Item from FMVCreateIOReq().
|||
|||	  Return Value
|||
|||	    Error code from SendIO().
|||
|||	  Notes
|||
|||	    The MPEG devices has buffer alignment and size modulo requirements.
|||	    Also, each buffer better be large enough to hold an entire frame, where
|||	    bytes/pixel is 2 in 16 bit/pixel mode and 4 in 24 bit/pixel mode.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libsubscriber.a.
|||
|||	  Associated Files
|||
|||	    "fmvdriverinterface.h", libsubscriber.a
|||
|||	  See Also
|||
|||	    FMVOpenVideo(), FMVCreateIOReq().
|||
************************************************************************/
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
