/* @(#) mpVideoDriver.h 96/12/11 1.15 */

#ifndef MPVIDEODRIVER_HEADER
#define MPVIDEODRIVER_HEADER

#include <device/mpegvideo.h>
#include "videoDeviceContext.h"

/* definitions required for proper driver registration */

#define DRIVERNAME	FMV_VIDEO_DEVICE_NAME
#define DEVICENAME	FMV_VIDEO_DEVICE_NAME
#define DRIVERIDENTITY		DI_OTHER
#define MAXIMUMSTATUSSIZE	sizeof( DeviceStatus )
#define DEVICEFLAGWORD		DS_DEVTYPE_OTHER
#define DRIVERPRI	150
#define DEVICEPRI	150

#define VDEVCONTEXT(dev)  ((tVideoDeviceContext *)((dev)->dev_DriverData))
/* possible return values */
enum {
	MPVPTSVALID = 1,
	MPVFLUSHREAD,
	MPVFLUSHWRITE,
	MPVFLUSHDRIVER    /* Special commands sent to driver using write(eg. reset)*/
};

/* MPEG definitions */
#define MAXVIDEOFRAMESIZE   	(4*1152)
#define DEFAULT_LEFT_CROP		(0L)
#define DEFAULT_TOP_CROP		(0L)
#define DEFAULT_WIDTH			352L
#define DEFAULT_HEIGHT			288L
#define MACROBLOCKWIDTH			16L
#define MACROBLOCKHEIGHT		16L
#define MPEGMODEPLAY			((unsigned) 0L)
#define MPEGMODEIFRAMESEARCH 	((unsigned) 1L)

/* function protypes */

Item mpvCreateDevice( void );
int32 mpNewThread(tVideoDeviceContext *theUnit);


/* decoder callback routines */
int32 MPVRead(tVideoDeviceContext* theUnit, uint8 **buf, int32 *len,
			  uint32 *pts, uint32 *userData);
int32 MPVCompleteRead(tVideoDeviceContext* theUnit, int32 status);
int32 MPVNextWriteBuffer(tVideoDeviceContext* theUnit, uint32 **buf,
						 uint32 *len);
int32 MPVCompleteWrite(tVideoDeviceContext *theUnit, int32 status, int32 flags, 
						uint32 pts, uint32 userData);

#endif




