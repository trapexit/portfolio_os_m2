/* @(#) mpVideoDriver.c 96/12/11 1.31 */
/*	file: mpVideoDriver.c
	M2 MPEG Video Driver
	2/28/95 George Mitsuoka
	The 3DO Company Copyright (C) 1995 */

#define MPVIDEODRIVER_C

#include <string.h>
#include <kernel/debug.h>
#include <kernel/io.h>
#include <kernel/mem.h>
#include <kernel/super.h>
#include <graphics/bitmap.h>
#include <graphics/graphics.h>
#include <loader/loader3do.h>
#include "M2MPEGUnit.h"
#include "mpVideoDriver.h"
#include "MPEGVideoBuffers.h"

#ifdef DEBUG_PRINT_IORW
  #define DBGIO(a)		PRNT(a)
#else
  #define DBGIO(a)		(void)0
#endif

#ifdef DEBUG_PRINT_IOCNTL
  #define DBGCNTL(a)	PRNT(a)
#else
  #define DBGCNTL(a)	(void)0
#endif

#ifdef DEBUG_PRINT_DEV
  #define DBGDEV(a)		PRNT(a)
#else
  #define DBGDEV(a) 	(void)0
#endif

#define INTERNAL_MPEG_DRIVERNAME		"mpegvideo"
#define INTERNAL_MPEG_DEVICENAME		"mpeg video device"
#define INTERNAL_MPEG_DRIVERIDENTITY	DI_OTHER
#define INTERNAL_MPEG_MAXSTATUSSIZE	    sizeof( DeviceStatus )
#define INTERNAL_MPEG_DEVICEFLAGWORD	DS_DEVTYPE_OTHER
#define INTERNAL_MPEG_DRIVERPRI			150
#define INTERNAL_MPEG_DEVICEPRI			150
#define INTERNAL_MPEG_COMMAND1NAME		"control command"
#define INTERNAL_MPEG_COMMAND2NAME		"write command"
#define INTERNAL_MPEG_COMMAND3NAME		"read command"

static void	mpvDriverAbortIO( IOReq *ior );
static Item	mpvDriverInit( Driver *drv );
static Err mpvDriverDelete(Driver* drv);
static Item mpvDriverClose(Driver* drv, Task* task);
static Item	mpvDeviceInit( Device *dev );
static Err mpvDeviceDelete( Device *dev );
static Item	mpvDeviceOpen(Device *dev, Task* task);
static Err mpvDeviceClose(Device *dev, Task* task);
static int32 mpvCmdWrite( IOReq *ior );
static int32 mpvCmdRead( IOReq *ior );
static int32 mpvOldCmdRead(IOReq* ior);
static int32 mpvCmdStatus( IOReq *ior );
static int32 mpvCmdControl( IOReq *ior );
static int32 mpvCmdGetBufferInfo( IOReq *ior );
static int32 mpvCmdSetStripBuffer( IOReq *ior );
static int32 mpvCmdSetReferenceBuffer( IOReq *ior );
static int32 mpvCmdAllocBuffers( IOReq *ior );
static void StartWrite(tVideoDeviceContext *theUnit);
static void StartRead(tVideoDeviceContext *theUnit);
static void mpvQueueReq(IOReq *ior);
static int32 FreeDeviceResources(Device* dev);

/*	there is one driver commands table per driver */

static DriverCmdTable CmdTable[] =
{
	CMD_STATUS,						mpvCmdStatus,
	MPEGVIDEOCMD_CONTROL,			mpvCmdControl,
	MPEGVIDEOCMD_WRITE,				mpvCmdWrite,
	MPEGVIDEOCMD_READ,				mpvCmdRead,
	CMD_READ,						mpvOldCmdRead,  /* used by MPEGVideoSubscriber. */
	MPEGVIDEOCMD_GETBUFFERINFO,		mpvCmdGetBufferInfo,
	MPEGVIDEOCMD_SETSTRIPBUFFER,	mpvCmdSetStripBuffer,
	MPEGVIDEOCMD_SETREFERENCEBUFFER,mpvCmdSetReferenceBuffer,
	MPEGVIDEOCMD_ALLOCBUFFERS,		mpvCmdAllocBuffers,
};

static void mpvQueueReq( IOReq *ior )
{
	int32 interrupts;
	tVideoDeviceContext *theUnit;

	theUnit = VDEVCONTEXT(ior->io_Dev);

	/* add request to the appropriate queue */
	/* support obsolete CMD_WRITE for now */
	if( (ior->io_Info.ioi_Command == MPEGVIDEOCMD_WRITE) ||
		(ior->io_Info.ioi_Command == CMD_WRITE) )
	{
		/* disable interrupts while queueing the request */
		interrupts = Disable();
		AddTail( &(theUnit->writeQueue), (Node *) ior );
/*		DBGIO(("mpvQueueReq: Add Write Q=%d\n", GetNodeCount(&(theUnit->writeQueue)))); */
		Enable( interrupts );
	}
	/* support obsolete CMD_READ for now */
	else if( (ior->io_Info.ioi_Command == MPEGVIDEOCMD_READ) ||
			 (ior->io_Info.ioi_Command == CMD_READ) )
	{
		/* disable interrupts while queueing the request */
		interrupts = Disable();
		AddTail( &(theUnit->readQueue), (Node *) ior );
/*		DBGIO(("mpvQueueReq: Add Read Q=%d\n", GetNodeCount(&(theUnit->readQueue)))); */
		Enable( interrupts );
	}
}

static void StartWrite(tVideoDeviceContext *theUnit)
{
	/* signal the decoder task that a write buffer is available */
    SuperInternalSignal(theUnit->task, theUnit->signalWriteAvail);
}

static void StartRead(tVideoDeviceContext *theUnit)
{
	/* signal the decoder task that a read buffer is available */
	SuperInternalSignal( theUnit->task, theUnit->signalReadAvail );
}

/*	int32 MPVRead(tVideoDeviceContext* theUnit, uint8 **buf, int32 *len,
				   uint32 *pts, uint32 userData)

		MPVRead returns in buf a pointer to a buffer containing
		raw MPEG video data. The byte length of the buffer is returned
		in len. The required unit is passed to the decode
		routine when invoked. The return value from MPVRead can be
		a system error code (< 0), zero for normal completion,
		MPVPTSVALID, indicating that the pts and userData values have been set,
		or MPVFLUSHWRITE, indicating that the decode routine should
		complete its current write buffer */

int32 MPVRead(tVideoDeviceContext *theUnit, uint8 **buf, int32 *len, uint32 *pts, uint32 *userData)
{
	int32 waitResult;
	IOBuf *theWriteBuf;
	FMVIOReqOptions *optionPtr;

	DBGIO(("MPVRead ... "));

	/* is the queue empty? */
	while( IsEmptyList( &theUnit->writeQueue ) )
	{
		DBGIO(("waiting\n"));
		/* wait for a write request to be queued */
		waitResult = WaitSignal(theUnit->signalWriteAvail |
								theUnit->signalReadAbort |
								theUnit->signalFlush |
								theUnit->signalQuit);
		DBGIO(("MPVRead (after waiting) ... "));
		if ((waitResult & SIGF_ABORT) || (waitResult == ABORTED) ||	(waitResult & theUnit->signalQuit))
		{
			/* ack! this should never happen! */
			*len = 0L;
			DBGIO(("return ABORTED.\n"));
			return( ABORTED );
		}
		if( waitResult & theUnit->signalFlush )
		{
			/* we need to flush the driver (EG, buffers changed by client) */
			*len = 0L;
			DBGIO(("got Flush signal, return MPVFLUSHDRIVER\n"));
			return( MPVFLUSHDRIVER );
		}
		if( waitResult & theUnit->signalReadAbort )
		{
			/* the current read IO req was aborted while we were waiting for a write req */
			*len = 0L;
			DBGIO(("return MPVFLUSHWRITE\n"));
			return( MPVFLUSHWRITE );	/* a read req is a decoder write */
		}
	}
	/* get next IOReq from the queue */
	theUnit->currentWriteReq = (IOReq *) RemHead( &theUnit->writeQueue );
/*	DBGIO(("Rem: Write Q=%d ", GetNodeCount(&theUnit->writeQueue))); */
	DBGIO(("got ior %08p ", theUnit->currentWriteReq));
	
	/* set the buf and len return values */
	theWriteBuf = &(theUnit->currentWriteReq->io_Info.ioi_Send);
	*buf = (uint8 *) theWriteBuf->iob_Buffer;
	*len = theWriteBuf->iob_Len;

	optionPtr = (FMVIOReqOptions *) theUnit->currentWriteReq->io_Info.ioi_CmdOptions;
	if (optionPtr)
	{
		/* check for valid pts */
		if (optionPtr->FMVOpt_Flags & FMVValidPTS)	{
			*pts = optionPtr->FMVOpt_PTS;
			*userData = optionPtr->FMVOpt_UserData;
			DBGIO(("return MPVPTSVALID\n"));
			return( MPVPTSVALID );

		} else if (optionPtr->FMVOpt_Flags & FMV_FLUSH_FLAG) {
			/* Flush flag is set. Flush the driver. */
			DBGIO(("got FMV_FLUSH_FLAG, return MPVFLUSHDRIVER\n"));
			MPVCompleteRead(theUnit, 0);
			return MPVFLUSHDRIVER;
		} else if (optionPtr->FMVOpt_Flags & FMV_END_OF_STREAM_FLAG) {
			/* Driver has received a complete picture. Don't wait for
			 * another picture. Complete the IOs.
			 */
			/*******************************************************/
			/* 911. WARNNING: This is a hack! NOT TESTED COMPLETELY.
			/* ONLY TESTED FOR STILL IMAGES. If the last picture is an
			/* I frame...!! What happens to the PTS.
            /********************************************************/
			DBGIO(("got FMV_END_OF_STREAM_FLAG, return MPVFLUSHDRIVER\n"));
			MPVCompleteRead(theUnit, 0);
			MPVCompleteWrite(theUnit, 0, 0, 0, 0);
			return MPVFLUSHDRIVER;
		}
	}

	DBGIO(("return 0\n"));

	return 0L;
}

/*	note that a 'write' from the decoder goes out to a 'read' IOReq */

int32 MPVNextWriteBuffer(tVideoDeviceContext *theUnit, uint32 **buf, uint32 *len)
{
	int32 waitResult;
	IOBuf *theReadBuf;

	DBGIO(("MPVNextWriteBuffer ..."));

	while( 1 )
	{
		/* no current read request? */
		if( theUnit->currentReadReq == (IOReq *) 0L )
		{
			while( IsEmptyList( &theUnit->readQueue ) )
			{
				/* queue empty, wait for next read request to be queued */
				DBGIO(("waiting\n"));
				waitResult = WaitSignal(theUnit->signalReadAvail |
										theUnit->signalWriteAbort |
										theUnit->signalFlush |
										theUnit->signalQuit);
				DBGIO(("MPVNextWriteBuffer (after waiting) ..."));
				if ((waitResult & SIGF_ABORT) || waitResult == ABORTED || (waitResult & theUnit->signalQuit))
				{
					/* ack! this should never happen! */
					DBGIO(("return ABORTED\n"));
					return( ABORTED );
				}
				if( waitResult & theUnit->signalFlush )
				{
					/* we need to flush the driver (EG, buffers changed by client) */
					*len = 0L;
					DBGIO(("got Flush signal, return MPVFLUSHDRIVER\n"));
					return( MPVFLUSHDRIVER );
				}
				if( waitResult & theUnit->signalWriteAbort )
				{
					/* the current write IO req was aborted while we were waiting for a read req */
					DBGIO(("return MPVFLUSHREAD\n"));
					return( MPVFLUSHREAD );		/* our write req is a decoder read */
				}
			}
			/* get next IOReq from the queue */
			theUnit->currentReadReq = (IOReq *) RemHead( &theUnit->readQueue );
/*			DBGIO(("Rem: Read Q=%d ", GetNodeCount(&theUnit->readQueue)));	*/
			DBGIO(("ior %08p  ",theUnit->currentReadReq));
			theUnit->readReqOffset = 0L;
		}

		/* support obsolete CMD_READ for now */

		if( theUnit->currentReadReq->io_Info.ioi_Command == CMD_READ )
		{
			theReadBuf = &(theUnit->currentReadReq->io_Info.ioi_Recv);

			*buf = (uint32 *) theReadBuf->iob_Buffer;
			*len = theReadBuf->iob_Len;
			break;
		}
		else
		{
			IOReq *ior;
			Item bmItem;
			Bitmap *bm;

			/* rather than a buffer, we should have been passed a Bitmap Item */
			ior = theUnit->currentReadReq;
			if( (uint32) ior->io_Info.ioi_Send.iob_Buffer & 0x3L )
				goto failRetry;
			if( ior->io_Info.ioi_Send.iob_Len != sizeof( Item ) )
				goto failRetry;

			bmItem = *((Item *) (ior->io_Info.ioi_Send.iob_Buffer));

			if( (bm = CheckItem( bmItem,NST_GRAPHICS,GFX_BITMAP_NODE )) == NULL)
				goto failRetry;
			if( (theUnit->depth == 16) &&
				( (bm->bm_Type != BMTYPE_16) &&
				  (bm->bm_Type != BMTYPE_16_ZBUFFER) ) )
				goto failRetry;
			if( (theUnit->depth == 32) && (bm->bm_Type != BMTYPE_32) )
				goto failRetry;
			if( (theUnit->xSize != bm->bm_Width) ||
				(theUnit->ySize != bm->bm_Height) )
				goto failRetry;
			*buf = bm->bm_Buffer;
			*len = bm->bm_BufferSize;
			break;
		failRetry:
			PERR(("Bad Bitmap Item in read IOReq\n"));
			ior->io_Error = BADIOARG;
			CompleteIO( ior );
		}
	}

	DBGIO(("return 0\n"));

	return( 0L );
}

int32 MPVCompleteRead(tVideoDeviceContext *theUnit, int32 status)
{
	IOReq *ior;

	DBGIO(("MPVCompleteRead "));

	/* is there a current write request? */
	if( (ior = theUnit->currentWriteReq) != (IOReq *) 0L )
	{
		/* was it aborted? */
		if( theUnit->vdcFlags & VDCFLAG_ABORT_WRITE )
		{
			ior->io_Error  = ABORTED;
			ior->io_Actual = 0L;
			theUnit->vdcFlags ^= VDCFLAG_ABORT_WRITE;
		}
		else
		{
			ior->io_Error  = status;
			ior->io_Actual = ior->io_Info.ioi_Send.iob_Len;
		}
		theUnit->currentWriteReq = (IOReq *) 0L;
		DBGIO(("ior %08p io_Error %d io_Actual %d\n", ior, ior->io_Error, ior->io_Actual));
		CompleteIO( ior );
	}
	else
	{
		DBGIO(("no ioreq to complete\n"));
	}

	return( 0L );
}

int32 MPVCompleteWrite(tVideoDeviceContext* theUnit, int32 status, int32 ptsValid, uint32 pts, uint32 userData)
{
	IOReq *ior;

	DBGIO(("MPVCompleteWrite "));

	/* is there a current read request? */
	if( (ior = theUnit->currentReadReq) != (IOReq *) 0L )
	{
		/* was it aborted? */
		if( theUnit->vdcFlags & VDCFLAG_ABORT_READ )
		{
			ior->io_Error  = ABORTED;
			ior->io_Actual = 0L;
			theUnit->vdcFlags ^= VDCFLAG_ABORT_READ;
		}
		else
		{
			ior->io_Error  = status;
			ior->io_Actual = ior->io_Info.ioi_Recv.iob_Len;
		}
		if( ptsValid )
		{
			ior->io_Extension[ 0 ] = pts;
			ior->io_Extension[ 1 ] = 0L;
			ior->io_Flags |= FMVValidPTS;
			ior->io_Info.ioi_CmdOptions = userData;
		}
		else
		{
			ior->io_Flags &= ~FMVValidPTS;
		}
		theUnit->currentReadReq = (IOReq *) 0L;
		DBGIO(("ior %08p io_Error %d io_Actual %d\n", ior, ior->io_Error, ior->io_Actual));
		CompleteIO( ior );
	}
	else
	{
		DBGIO(("no ioreq to complete\n"));
	}

	return( 0L );
}

static void mpvDriverAbortIO(IOReq *ior)
{
	tVideoDeviceContext *theUnit;

	/* interrupts are disabled by the system prior to entry */
	DBGIO(("mpvDriverAbortIO ior %08p ", ior));

    /* verify valid unit number */
    /* change this to support multiple devices */
	ior->io_Error = ABORTED;

	theUnit = VDEVCONTEXT(ior->io_Dev);

	if( ior == theUnit->currentWriteReq )
	{
		DBGIO(("== current write\n"));
		theUnit->vdcFlags |= VDCFLAG_ABORT_WRITE;
		SuperInternalSignal( theUnit->task, theUnit->signalWriteAbort );
	}
	else if( ior == theUnit->currentReadReq )
	{
		DBGIO(("== current read\n"));
		theUnit->vdcFlags |= VDCFLAG_ABORT_READ;
		SuperInternalSignal( theUnit->task, theUnit->signalReadAbort );
	}
	else if (ior == theUnit->clientSBReq)
	{
		DBGIO(("== client strip buffer\n"));
		UnUseStripBuffer(theUnit);
		theUnit->clientSBReq = NULL;
		SuperInternalSignal( theUnit->task, theUnit->signalFlush );
		SuperCompleteIO(ior);
	}
	else if (ior == theUnit->clientRBReq)
	{
		DBGIO(("== client reference buffer\n"));
		UnUseReferenceBuffers(theUnit);
		theUnit->clientRBReq = NULL;
		SuperInternalSignal( theUnit->task, theUnit->signalFlush );
		SuperCompleteIO(ior);
	}
	else
	{
		DBGIO(("== queue'd node\n"));
		RemNode( (Node *) ior );
		ior->io_Actual = 0L;
		SuperCompleteIO( ior );
	}
}

/*	DriverInit gets called once when the driver is created
 *	it returns the driver item or a negative value on error
 */
static Item mpvDriverInit(Driver *drv)
{
Err result;

	DBGDEV(("%s: DriverInit\n",INTERNAL_MPEG_DRIVERNAME));

	result = M2MPEGInit();
	if (result < 0)
	    return result;

	return( drv->drv.n_Item );
}

static Err mpvDriverChangeOwner(Driver *drv, Item newOwner)
{
	TOUCH( drv );
	return M2MPEGChangeOwner( newOwner );
}

static Err mpvDriverDelete(Driver* drv)
{
	TOUCH(drv);
	FreeGlobalStripBuffer();
	return M2MPEGDelete();
}

static Item mpvDriverClose(Driver* drv, Task* task)
{
	TOUCH(task);
	return drv->drv.n_Item;
}

Item mpvCreateDevice(void)
{
	Item drvrItem;

	/*	create the driver */
	drvrItem = CreateItemVA(MKNODEID(KERNELNODE,DRIVERNODE),
		TAG_ITEM_NAME,			INTERNAL_MPEG_DRIVERNAME,
		TAG_ITEM_PRI,			INTERNAL_MPEG_DRIVERPRI,
		CREATEDRIVER_TAG_CMDTABLE,	CmdTable,
		CREATEDRIVER_TAG_NUMCMDS,	(sizeof(CmdTable)/sizeof(CmdTable[0])),
		CREATEDRIVER_TAG_CREATEDRV,	mpvDriverInit,
		CREATEDRIVER_TAG_CHOWN_DRV,	mpvDriverChangeOwner,
		CREATEDRIVER_TAG_CREATEDEV,	mpvDeviceInit,
		CREATEDRIVER_TAG_DELETEDEV,	mpvDeviceDelete,
		CREATEDRIVER_TAG_OPENDEV,	mpvDeviceOpen,
		CREATEDRIVER_TAG_CLOSEDEV,	mpvDeviceClose,
		CREATEDRIVER_TAG_ABORTIO,	mpvDriverAbortIO,
		CREATEDRIVER_TAG_DEVICEDATASIZE,sizeof(tVideoDeviceContext),
		CREATEDRIVER_TAG_CLOSEDRV,	mpvDriverClose,
		CREATEDRIVER_TAG_DELETEDRV,	mpvDriverDelete,
		CREATEDRIVER_TAG_MODULE,	FindCurrentModule(),
		TAG_END);

	DBGDEV(("%s: Creating driver returns drvrItem=%d\n", INTERNAL_MPEG_DRIVERNAME,drvrItem));

	return drvrItem;
}

/*	Each DevInit gets called once when the device is created
 *	and returns the device item or a negative value on error
 */
static Item mpvDeviceInit( Device *dev )
{
	DBGDEV(("%s: DevInit\n",INTERNAL_MPEG_DEVICENAME));
	if (!dev->dev_DriverData)
	{
		PERR(("mpvDeviceInit: What happened to my dev_DriverData.\n"));
		return(MakeKErr(ER_SEVERE,ER_C_STND,ER_DeviceError));
	}
	if (mpNewThread(VDEVCONTEXT(dev)))
		return(MakeKErr(ER_SEVERE,ER_C_STND,ER_DeviceError));

	return( dev->dev.n_Item );
}

/*    Each DevDelete gets called once when the device is deleted */
static Err mpvDeviceDelete( Device *dev )
{
	DBGDEV(("%s: DevDelete\n",INTERNAL_MPEG_DEVICENAME));
	return FreeDeviceResources(dev);
}

/*	each DevOpen gets called once each time the device is opened
 *	and returns the device item or a negative value on error
 */
static Item mpvDeviceOpen(Device *dev, Task* task)
{
	DBGDEV(("%s: DevOpen %ld\n",INTERNAL_MPEG_DEVICENAME,dev->dev.n_OpenCount));
	TOUCH(task);
	return( dev->dev.n_Item );
}

/* each DevClose gets called once each time the device is closed */
static Err mpvDeviceClose(Device *dev, Task* task)
{
	TOUCH(task);
	DBGDEV(("%s: DevClose %ld\n",INTERNAL_MPEG_DEVICENAME,dev->dev.n_OpenCount));
	return dev->dev.n_Item;
}

static int32 CheckOptions( IOReq *ior )
{
	FMVIOReqOptions *optionsPtr;

	optionsPtr = (FMVIOReqOptions *) ior->io_Info.ioi_CmdOptions;

	/* is there a FMVIOReqOptions struct? */
	if( !optionsPtr )
		return( 0L );

	/* verify alignment */
	if( (uint32) optionsPtr & 0x3L )
	{
		PERR(("CmdOptions not long aligned\n"));
		return( BADIOARG );
	}
	/* verify addresses */
	if( !IsMemReadable( optionsPtr, sizeof(FMVIOReqOptions) ) )
	{
		PERR(("CmdOptions doesn't point to FMVIOReqOptions\n"));
		return( BADIOARG );
	}
	/* verify reserved fields */
	if( optionsPtr->Reserved2 | optionsPtr->Reserved3 )
	{
		PERR(("FMVIOReqOptions reserved fields non-zero\n"));
		return( BADIOARG );
	}
	return( 0L );
}

static int32 mpvCmdWrite( IOReq *ior )
{
	DBGIO(("mpvCmdWrite ior %08p\n",INTERNAL_MPEG_DEVICENAME, INTERNAL_MPEG_COMMAND2NAME, ior));

	if( CheckOptions( ior ) )
	{
		ior->io_Error = BADIOARG;
		return( 1L );				/* synchronous, don't call completeio */
	}
	ior->io_Flags &= ~IO_QUICK;		/* asynchronous, clear IO_QUICK */
	mpvQueueReq( ior );
	StartWrite(VDEVCONTEXT(ior->io_Dev));

	return( 0L );
}

static int32 mpvCmdRead( IOReq *ior )
{
	DBGIO(("mpvCmdRead ior %08p\n",INTERNAL_MPEG_DEVICENAME,INTERNAL_MPEG_COMMAND3NAME, ior));

	if( CheckOptions( ior ) )
	{
		ior->io_Error = BADIOARG;
		return( 1L );				/* synchronous, don't call completeio */
	}
	ior->io_Flags &= ~IO_QUICK;		/* asynchronous, clear IO_QUICK */
	mpvQueueReq( ior );
	StartRead(VDEVCONTEXT(ior->io_Dev));

	return( 0L );
}

/*	driver specific commands begin here */
static int32 mpvCmdControl( IOReq *ior )
{
	tVideoDeviceContext savedContext, *theUnit;
	CODECDeviceStatus *cmdList;
	TagArgP commands;
	int     i;

	DBGCNTL(("mpvCmdControl "));

	theUnit = VDEVCONTEXT(ior->io_Dev);

	cmdList = (CODECDeviceStatus *) ior->io_Info.ioi_Send.iob_Buffer;

	/* null pointer? */
	if( !cmdList )
	{
		ior->io_Error = BADIOARG;
		goto abort;
	}
	/* verify alignment */
	if( (uint32) cmdList & 0x3L )
	{
		PERR(("CmdControl buffer not long aligned\n"));
		ior->io_Error = BADIOARG;
		goto abort;
	}
	/* verify addresses */
	if( !IsMemReadable( cmdList, sizeof( CODECDeviceStatus ) ) )
	{
		PERR(("CmdControl buffer doesn't point to CODECDeviceStatus\n"));
		ior->io_Error = BADIOARG;
		goto abort;
	}
	commands = cmdList->codec_TagArg;

	/* save our context in case one of the commands is bad */
	savedContext = *theUnit;

	/* loop through commands */
	for( i = 0; i < kMAX_CODEC_ARGS; i++ )
	{
		if( commands[i].ta_Tag == TAG_END )
			break;

		switch ( commands[i].ta_Tag )
		{
			case VID_CODEC_TAG_HSIZE:
				savedContext.xSize = (uint32) commands[i].ta_Arg;
				DBGCNTL(("HSIZE = %d ", savedContext.xSize));
				break;
			case VID_CODEC_TAG_VSIZE:
				savedContext.ySize = (uint32) commands[i].ta_Arg;
				DBGCNTL(("VSIZE = %d ", savedContext.ySize));
				break;
			case VID_CODEC_TAG_DEPTH:
				savedContext.depth = (int32) commands[i].ta_Arg;
				if( savedContext.depth < 0 )
				{
					PERR(("Obsolete depth specification (%ld)\n",
							savedContext.depth));
					PERR(("    use VID_CODEC_TAG_M2MODE\n"));
					savedContext.depth = -savedContext.depth;
					savedContext.operaMode = 0;
				}
				DBGCNTL(("DEPTH = %d ", savedContext.depth));
				break;
			case VID_CODEC_TAG_M2MODE:
				savedContext.operaMode = 0;
				DBGCNTL(("M2MODE "));
				break;
			case VID_CODEC_TAG_YUVMODE:
				savedContext.outputRGB = 0;
				DBGCNTL(("YUVMODE  "));
				break;
			case VID_CODEC_TAG_RGBMODE:
				savedContext.outputRGB = 1;
				DBGCNTL(("RBGMODE "));
				break;
			case VID_CODEC_TAG_DITHER:
				PERR(("WARNING: VID_CODEC_TAG_DITHER ignored\n"));
				break;
			case VID_CODEC_TAG_STANDARD:
				savedContext.resampleMode = (unsigned) commands[i].ta_Arg;
				DBGCNTL(("RESAMPLE = %d ", savedContext.resampleMode));
				break;
			case VID_CODEC_TAG_PLAY:
				savedContext.playMode = (unsigned) MPEGMODEPLAY;
				DBGCNTL(("MODE = FMV "));
				break;
			case VID_CODEC_TAG_SKIPFRAMES:
				savedContext.skipCount = (int32) commands[i].ta_Arg;
				DBGCNTL(("SKIPFRAMES = %d ", savedContext.skipCount));
				break;
			case VID_CODEC_TAG_KEYFRAMES:
				savedContext.playMode = (unsigned) MPEGMODEIFRAMESEARCH;
				DBGCNTL(("MODE = IFRAMES "));
				break;
			default:
				PERR(("CmdControl: unrecognized command %ld\n",
					  commands[i].ta_Tag));
				ior->io_Error = BADIOARG;
				goto abort;
		}
	}
	
	DBGCNTL(("\n"));
	
	/* finished reading through command list, validate args */
	if( M2MPGValidateParameters( &savedContext ) < 0L )
	{
		ior->io_Error = BADIOARG;
		goto abort;
	}
	/* requested settings are okay, copy them in */
	*theUnit = savedContext;
abort:
	return( 1L );					/* synchronous, don't call completeio */
}

static int32 mpvOldCmdRead( IOReq *ior )
{
#if 0
	uint32 *dst = (uint32 *)ior->io_Info.ioi_Recv.iob_Buffer;
	int32 len = ior->io_Info.ioi_Recv.iob_Len;

	DBGIO(("mpvOldCmdRead ior %08p\n", ior));
#endif
	return mpvCmdRead(ior);
}

/* CmdStatus is boilerplate */
static int32 mpvCmdStatus( IOReq *ior )
{
	DeviceStatus *dst = (DeviceStatus *) ior->io_Info.ioi_Recv.iob_Buffer;
	DeviceStatus mystat;
	int32 len = ior->io_Info.ioi_Recv.iob_Len;

	DBGIO(("mpvCmdStatus dst=%08lx len=%d\n", dst, len));
	if( len < 8 )
		goto abort;

	memset( &mystat,0,sizeof( DeviceStatus ) );

	/*	check driver.h to see if there are more appropriate
		values for your driver */
	mystat.ds_DriverIdentity = INTERNAL_MPEG_DRIVERIDENTITY;
	mystat.ds_MaximumStatusSize = INTERNAL_MPEG_MAXSTATUSSIZE;
	mystat.ds_DeviceFlagWord = INTERNAL_MPEG_DEVICEFLAGWORD;

	if( len > sizeof( DeviceStatus ) )
		len = sizeof( DeviceStatus );
	memcpy( dst,&mystat,len );
	ior->io_Actual = len;

	return( 1 );

abort:
	ior->io_Error = BADIOARG;
	return( 1 );
}

static int32 mpvCmdGetBufferInfo( IOReq *ior )
{
	MPEGBufferStreamInfo *	bsi;
	MPEGBufferInfo *		bi;
	
	DBGCNTL(("mpvCmdGetBufferInfo\n"));
	
	bsi = (MPEGBufferStreamInfo *) ior->io_Info.ioi_Send.iob_Buffer;
	bi	= (MPEGBufferInfo *) ior->io_Info.ioi_Recv.iob_Buffer;
	
	if (ior->io_Info.ioi_Send.iob_Len != sizeof(MPEGBufferStreamInfo) ||
		!IsMemReadable(bsi, sizeof(MPEGBufferStreamInfo)))
	{
		PERR(("CmdGetBufferInfo send buffer doesn't point to MPEGBufferStreamInfo\n"));
		ior->io_Error = BADIOARG;
		goto abort;
	}
	
	if (ior->io_Info.ioi_Recv.iob_Len != sizeof(MPEGBufferInfo))
	{ /* system has already verified IsMemWritable for receive buffer */
		PERR(("CmdGetBufferInfo receive buffer doesn't point to MPEGBufferInfo\n"));
		ior->io_Error = BADIOARG;
		goto abort;
	}
	
	ior->io_Error  = CalcBufferInfo(bi, bsi->fmvWidth, bsi->fmvHeight, bsi->stillWidth);
	ior->io_Actual = (ior->io_Error < 0) ? 0 : sizeof(MPEGBufferInfo);
		
abort:

	return 1;
}

static int32 mpvCmdSetStripBuffer( IOReq *ior )
{
	int32	interrupts;
	tVideoDeviceContext *	theUnit;
	
	DBGCNTL(("mpvCmdSetStripBuffer\n"));
	
	theUnit = VDEVCONTEXT(ior->io_Dev);

	interrupts = Disable();

	if (theUnit->clientSBReq)
	{
		mpvDriverAbortIO(theUnit->clientSBReq);
	}
	else
	{
		FreeStripBuffer(theUnit);
		SuperInternalSignal( theUnit->task, theUnit->signalFlush );
	}

	Enable(interrupts);

	if (ior->io_Info.ioi_Recv.iob_Buffer == NULL)
	{
		PERR(("Invalid NULL buffer iob_Recv buffer pointer for MPEGVIDEOCMD_SETSTRIPBUFFER\n"));
		ior->io_Error = BADPTR;
		goto abort;
	}
	
	if (ior->io_Info.ioi_Recv.iob_Len == 0)
	{
		PERR(("Invalid buffer length of zero for MPEGVIDEOCMD_SETSTRIPBUFFER\n"));
		ior->io_Error = BADPTR;
		goto abort;
	}
	
	ior->io_Error = UseStripBuffer(theUnit, ior->io_Info.ioi_Recv.iob_Buffer, ior->io_Info.ioi_Recv.iob_Len);
	if (ior->io_Error < 0)
		goto abort;
		
	theUnit->clientSBReq = ior;
	theUnit->vdcFlags |= VDCFLAG_SB_CLIENT_CONTROL;
	
	return 0;
		
abort:

	return 1;
}

static int32 mpvCmdSetReferenceBuffer( IOReq *ior )
{
	int32	 interrupts;
	tVideoDeviceContext *	theUnit;
	
	DBGCNTL(("mpvCmdSetReferenceBuffer\n"));
	
	theUnit = VDEVCONTEXT(ior->io_Dev);

	interrupts = Disable();
	
	if (theUnit->clientRBReq)
	{
		mpvDriverAbortIO(theUnit->clientRBReq);
	}
	else
	{
		FreeReferenceBuffers(theUnit);
		SuperInternalSignal( theUnit->task, theUnit->signalFlush );
	}

	Enable(interrupts);

	if (ior->io_Info.ioi_Recv.iob_Buffer == NULL)
	{
		PERR(("Invalid NULL buffer iob_Recv buffer pointer for MPEGVIDEOCMD_SETREFERENCEBUFFER\n"));
		ior->io_Error = BADPTR;
		goto abort;
	}
	
	if (ior->io_Info.ioi_Recv.iob_Len == 0)
	{
		PERR(("Invalid buffer length of zero for MPEGVIDEOCMD_SETREFERENCEBUFFER\n"));
		ior->io_Error = BADPTR;
		goto abort;
	}
	
	ior->io_Error = UseReferenceBuffers(theUnit, ior->io_Info.ioi_Recv.iob_Buffer, ior->io_Info.ioi_Recv.iob_Len);
	if (ior->io_Error < 0)
		goto abort;
		
	theUnit->clientRBReq = ior;
	theUnit->vdcFlags |= VDCFLAG_RB_CLIENT_CONTROL;
	
	return 0;
		
abort:

	return 1;
}

static int32 mpvCmdAllocBuffers( IOReq *ior )
{
	Err						err;
	int32					interrupts;
	MPEGBufferStreamInfo *	bsi;
	MPEGBufferInfo 			bi;
	tVideoDeviceContext *	theUnit;
	
	DBGCNTL(("mpvCmdAllocBuffers\n"));
	
	theUnit = VDEVCONTEXT(ior->io_Dev);

	bsi = (MPEGBufferStreamInfo *) ior->io_Info.ioi_Send.iob_Buffer;
	
	if (ior->io_Info.ioi_Send.iob_Len != sizeof(MPEGBufferStreamInfo) ||
		!IsMemReadable(bsi, sizeof(MPEGBufferStreamInfo)))
	{
		PERR(("CmdAllocBuffers send buffer doesn't point to MPEGBufferStreamInfo\n"));
		ior->io_Error = BADIOARG;
		goto abort;
	}
	
	err = CalcBufferInfo(&bi, bsi->fmvWidth, bsi->fmvHeight, bsi->stillWidth);
	if (err < 0) 
	{
		/* PERR already done by calc routine */
		ior->io_Error = err;
		goto abort;
	}

	interrupts = Disable();
	
	if (theUnit->clientSBReq)
	{
		mpvDriverAbortIO(theUnit->clientSBReq);
	}
	else
	{
		FreeStripBuffer(theUnit);
		SuperInternalSignal( theUnit->task, theUnit->signalFlush );
	}
	
	if (theUnit->clientRBReq)
	{
		mpvDriverAbortIO(theUnit->clientRBReq);
	}
	else
	{
		FreeReferenceBuffers(theUnit);
		SuperInternalSignal( theUnit->task, theUnit->signalFlush );
	}
	
	Enable(interrupts);

	theUnit->vdcFlags &= ~(VDCFLAG_SB_CLIENT_CONTROL|VDCFLAG_RB_CLIENT_CONTROL);

	if (bi.stripBuffer.minSize != 0) 
	{
		err = AllocStripBuffer(theUnit, &bi);
		if (err < 0)
		{
			ior->io_Error = err;
			goto abort;
		}
		theUnit->vdcFlags |= VDCFLAG_SB_CLIENT_CONTROL;
	}
	
	if (bi.refBuffer.minSize != 0)
	{
		err = AllocReferenceBuffers(theUnit, &bi);
		if (err < 0)
		{
			ior->io_Error = err;
			goto abort;
		}
		theUnit->vdcFlags |= VDCFLAG_RB_CLIENT_CONTROL;
	}

	/* returned info is an optional feature, provided if the
	 * receive buffer is non-NULL and right-sized...
	 */

	if (ior->io_Info.ioi_Recv.iob_Buffer != NULL &&
		ior->io_Info.ioi_Recv.iob_Len == sizeof(MPEGBufferInfo))
	{ /* system has already verified IsMemWritable for non-NULL receive buffer */
		memcpy(ior->io_Info.ioi_Recv.iob_Buffer, &bi, sizeof(MPEGBufferInfo));
		ior->io_Actual = sizeof(MPEGBufferInfo);
	}
	
		
abort:

	return 1;
}

static int32 FreeDeviceResources(Device* dev)
{
	Err err = 0;
	tVideoDeviceContext* theUnit = VDEVCONTEXT(dev);

	if (!theUnit)
		return 0;

	FreeDecodeBuffers(theUnit);

	/* Check if the driver's thread is still around. If yes kill it. */
	if (CheckItem(theUnit->taskItem, KERNELNODE, TASKNODE))
		SuperInternalSignal(theUnit->task, theUnit->signalQuit);

	return err;
}
