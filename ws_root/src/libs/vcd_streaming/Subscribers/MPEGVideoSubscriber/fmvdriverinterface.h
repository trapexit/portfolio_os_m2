/****************************************************************************
**
**  @(#) fmvdriverinterface.h 96/06/05 1.1
**
*****************************************************************************/
#ifndef __STREAMING_FMVDRIVERINTERFACE_H
#define __STREAMING_FMVDRIVERINTERFACE_H


#ifndef __KERNEL_TYPES_H
	#include <kernel/types.h>
#endif

#ifndef __KERNEL_ITEM_H
	#include <kernel/item.h>		/* for DeleteItem() */
#endif

#include <device/mpegvideo.h>		/* FMVIOReqOptionsPtr. (See also <misc/mpeg.h>) */


/*****************************************************
 * A "handle" onto the FMV Device driver, created by this module.
 * The client must allocate one of these for each open device Item it wants
 * on an MPEG FMV video decoder device.
 *****************************************************/
typedef struct FMVDeviceHandle {
	Item		deviceItem;
	Item		cmdIOReqItem;
	} FMVDeviceHandle;


/*****************************************************
 * USAGE SUMMARY
 * Setup: Allocate an FMVDeviceHandle, call FMVOpenVideo() to open the
 *   device, call FMVCreateIOReq() to create I/O Req Items, and call
 *   FMVSetVideoSize() to adjust settings.
 * Playback: Call FMVWriteBuffer() to issue asynchronous writes of compressed
 *   data, call FMVReadVideoBuffer() to issue asynchronous reads of decompressed
 *   data, and call FMVGetPTS() to get the presentation timestamp (PTS) of each
 *   decompressed frame. Call FMVSetVideoMode() to change playback modes.
 * Cleanup: Call FMVCloseDevice() to close the device.
 *
 * NOTE: The MPEG video driver and this interface use 32-bit PTSs. They ignore
 * MPEG's 33rd bit.
 *****************************************************/
#ifdef __cplusplus
extern "C" {
#endif


	/* Return an FMVDeviceHandle's Device Item. */
#define FMVGetDeviceItem(anFMVDeviceHandlePtr)	\
			((Item)(anFMVDeviceHandlePtr)->deviceItem)

Item	FMVCreateIOReq(FMVDeviceHandle *device, uint32 completionSignal);

#define FMVDeleteIOReq(item)	(DeleteItem(item))

Err		FMVGetPTS(IOReq *ioReqItemPtr, uint32 *ptsValue, uint32 *userData);

Err		FMVOpenVideo(FMVDeviceHandle *device, int32 bitsPerPixel, int32 horizSize,
			int32 vertSize, uint8 resampleMode);

Err		FMVCloseDevice(FMVDeviceHandle *device);

Err		FMVWriteBuffer(FMVDeviceHandle *device, void *buffer,
			int32 numBytes, Item ioReqItem, FMVIOReqOptionsPtr fmvOptionsPtr,
			uint32 fmvFlags, uint32 ptsValue, uint32 userData);

Err		FMVSetVideoMode(FMVDeviceHandle *device, int32 mode, int32 argument);

Err		FMVSetVideoSize(FMVDeviceHandle *device, int32 bitsPerPixel, int32 horizSize,
			int32 vertSize, uint8 resampleMode);

Err		FMVReadVideoBuffer(void *buffer, int32 numBytes, Item ioReqItem);


#ifdef __cplusplus
}
#endif

#endif	/* __STREAMING_FMVDRIVERINTERFACE_H */
