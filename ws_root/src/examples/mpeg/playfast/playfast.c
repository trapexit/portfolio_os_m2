
/******************************************************************************
**
**  @(#) playfast.c 96/12/11 1.37
**
**  Quickly play an MPEG stream
**
******************************************************************************/

/**
|||	AUTODOC -class Examples -name PlayFast
|||	Demonstrates how to play an MPEG movie using the device driver interface.
|||
|||	  Synopsis
|||
|||	    PlayFast -f <filename>
|||	             [-16]
|||	             [-640]
|||	             [-l <loopCount>]
|||	             [-b]
|||
|||	  Description
|||
|||	    This program loads file in from disk and plays the whole thing
|||	    as an MPEG movie. The whole file must fit in memory in order to
|||	    be played. This provides a simple example of playing an MPEG
|||	    moving by talking to the device driver directly.
|||
|||	    As the program is running, you can use the control pad to change
|||	    certain options:
|||
|||	  -preformatted
|||
|||	      Pause        Pause/Resume movie
|||	      Stop         Stop movie
|||	      Up           Play faster
|||	      Down         Play slower
|||	      Left         Branch to start of movie
|||	      Right        Toggle I/All frame(s) mode
|||	      Left Shift   Toggle YUV/RGB view mode
|||	      Right Shift  Toggle YUV/RGB decoder mode
|||	      A            Play normal speed
|||	      C            Play fastest
|||
|||	  Arguments
|||
|||	    -f <filename>
|||	        This is a required argument that specifies the name of the
|||	        MPEG file to read in and play.
|||
|||	    [-16]
|||	        Play the movie in a 16-bit frame buffer instead of a 32-bit
|||	        frame buffer.
|||
|||	    [-640]
|||	        Play the movie in a 640x480 bitmap instead of the default
|||	        320x240 bitmap.
|||
|||	    [-l <loopcount>]
|||	        Play the movie loopcount time.
|||
|||	    [-b]
|||	        This causes the app to allocate its own mpeg decode buffers
|||	        and hand them off to the mpeg video device.  (Default is to
|||	        let the device allocate its own buffers as usual.)
|||
|||	  Associated Files
|||
|||	    playfast.c
|||
|||	  Location
|||
|||	    Examples/MPEG/PlayFast
|||
**/

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/kernelnodes.h>
#include <kernel/list.h>
#include <kernel/mem.h>
#include <kernel/nodes.h>
#include <kernel/operror.h>
#include <kernel/task.h>
#include <file/filesystem.h>
#include <file/fileio.h>
#include <graphics/graphics.h>
#include <graphics/bitmap.h>
#include <graphics/view.h>
#include <misc/event.h>
#include <device/mpegvideo.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


/*****************************************************************************/


#define ERR(x,e) {printf x; PrintfSysErr(e);}

#define NUM_BITMAPS   2
#define NUM_WRITEBUFFERS 2
#define MAXWRITESIZE  8192


/* configurable stuff */
static uint32  loopCount;
static int32   pixelMode;
static int32   vblCount;
static uint32  hSize;
static uint32  vSize;
static bool    mode640;

/* graphics and driver stuff */
static int32   renderSig;
static Item    view;
static Item    bitmaps[NUM_BITMAPS];
static Item    timerIO;
static Item    mpegVideoDevice;
static Item    videoWriteReqItem[NUM_WRITEBUFFERS];
static Item    videoReadReqItem;
static Item    videoCmdItem;
static Item		videoStripBufReqItem;
static Item		videoRefBufReqItem;
static void *	videoStripBuffer;
static int32	videoStripBufferSize;
static void *	videoRefBuffer;
static int32	videoRefBufferSize;
static bool		useAppVideoBuffers;

/*****************************************************************************/


static Err SetupGraphics(void)
{
Err            result;
uint32         i;
enum ViewType  viewType;
Bitmap        *bm;
void          *buf;

    result = OpenGraphicsFolio();
    if (result >= 0)
    {
        for (i = 0; i < NUM_BITMAPS; i++)
            bitmaps[i] = -1;

	for (i = 0; i < NUM_BITMAPS; i++)
        {
            bitmaps[i] = result = CreateItemVA(MKNODEID(NST_GRAPHICS, GFX_BITMAP_NODE),
                                          BMTAG_WIDTH,       hSize,
                                          BMTAG_HEIGHT,      vSize,
                                          BMTAG_TYPE,        ((pixelMode == 16) ? BMTYPE_16 : BMTYPE_32),
                                          BMTAG_DISPLAYABLE, TRUE,
                                          BMTAG_MPEGABLE,    TRUE,
                                          TAG_END);
            if (bitmaps[i] >= 0)
            {
                bm = (Bitmap *)LookupItem(bitmaps[i]);

                buf = AllocMemMasked(bm->bm_BufferSize,
                                     bm->bm_BufMemType | MEMTYPE_FILL,
                                     bm->bm_BufMemCareBits,
                                     bm->bm_BufMemStateBits);
                if (buf)
                {
                     result = ModifyGraphicsItemVA(bitmaps[i], BMTAG_BUFFER, buf, TAG_END);
                     if (result < 0)
                         FreeMem(buf, bm->bm_BufferSize);
                }
            }

            if (result < 0)
                break;
        }

        if (result > 0)
        {
            renderSig = AllocSignal(0);
            if (renderSig >= 0)
            {
                if (pixelMode == 16)
                {
                    if (mode640)
                        viewType = VIEWTYPE_16_640_LACE;
                    else
                        viewType = VIEWTYPE_16;
                }
                else
                {
                    if (mode640)
                        viewType = VIEWTYPE_32_640_LACE;
                    else
                        viewType = VIEWTYPE_32;
                }

                view = CreateItemVA(MKNODEID(NST_GRAPHICS, GFX_VIEW_NODE),
                                    VIEWTAG_VIEWTYPE,     viewType,
                                    VIEWTAG_PIXELWIDTH,   hSize,
                                    VIEWTAG_BITMAP,       bitmaps[0],
                                    VIEWTAG_RENDERSIGNAL, renderSig,
                                    TAG_END);
                if (view >= 0)
                {
                    result = AddViewToViewList(view, 0);
                    if (result >= 0)
                    {
                        WaitSignal(renderSig);
                        return 0;
                    }
                    DeleteItem(view);
                }
                FreeSignal(renderSig);
            }

            for (i = 0; i < NUM_BITMAPS; i++)
            {
                bm = (Bitmap *)LookupItem(bitmaps[i]);
                if (bm)
                {
                    FreeMem(bm->bm_Buffer, bm->bm_BufferSize);
                    DeleteItem(bitmaps[i]);
                }
            }
        }
        CloseGraphicsFolio();
    }

    return result;
}


/*****************************************************************************/


static void CleanupGraphics(void)
{
uint32  i;
Bitmap *bm;

    DeleteItem(view);

    for (i = 0; i < NUM_BITMAPS; i++)
    {
        bm = LookupItem(bitmaps[i]);
        if (bm)
        {
            FreeMem(bm->bm_Buffer, bm->bm_BufferSize);
            DeleteItem(bitmaps[i]);
        }
    }

    FreeSignal(renderSig);
    CloseGraphicsFolio();
}


/*****************************************************************************/


static void CleanupMPEGBuffers(void)
{
	if (videoRefBufReqItem > 0)
	{
		AbortIO(videoRefBufReqItem);
		DeleteIOReq(videoRefBufReqItem);
		videoRefBufReqItem = 0;
	}
	
	if (videoStripBufReqItem > 0)
	{
		AbortIO(videoStripBufReqItem);
		DeleteIOReq(videoStripBufReqItem);
		videoStripBufReqItem = 0;
	}
	
	if (videoRefBuffer)
	{
		FreeMem(videoRefBuffer, videoRefBufferSize);
		videoRefBuffer = NULL;
	}
		
	if (videoStripBuffer)
	{
		FreeMem(videoStripBuffer, videoStripBufferSize);
		videoRefBuffer = NULL;
	}
}


/*****************************************************************************/


static Err SetupMPEGBuffers(void)
{
Err    					result;
MPEGBufferStreamInfo	streamInfo;
MPEGBufferInfo			bufferInfo;
IOInfo					ioInfo;

	if (!useAppVideoBuffers)
		return 0;

    videoStripBufReqItem = result = CreateIOReq(0, 0, mpegVideoDevice, 0);
	if (result < 0)
		goto cleanup;
	
    videoRefBufReqItem = result = CreateIOReq(0, 0, mpegVideoDevice, 0);
	if (result < 0)
		goto cleanup;
	
	memset(&streamInfo, 0, sizeof(streamInfo));
	streamInfo.fmvWidth  	= 640;
	streamInfo.fmvHeight 	= 480;

	memset(&ioInfo, 0, sizeof(ioInfo));
	ioInfo.ioi_Command			= MPEGVIDEOCMD_GETBUFFERINFO;
	ioInfo.ioi_Send.iob_Buffer	= &streamInfo;
	ioInfo.ioi_Send.iob_Len		= sizeof(streamInfo);
	ioInfo.ioi_Recv.iob_Buffer	= &bufferInfo;
	ioInfo.ioi_Recv.iob_Len		= sizeof(bufferInfo);
	
	result = DoIO(videoStripBufReqItem, &ioInfo);
	if (result < 0)
	{
		printf("MPEGVIDEOCMD_GETBUFFERINFO returned status: ");
		PrintfSysErr(result);
		goto cleanup;
	}
	
#if 0
	printf(	"Buffer Info:\n"
			" Ref:   min %7d flags %08x care %08x state %08x\n"
			" Strip: min %7d flags %08x care %08x state %08x\n",
			bufferInfo.refBuffer.minSize, 
			bufferInfo.refBuffer.memFlags, 
			bufferInfo.refBuffer.memCareBits, 
			bufferInfo.refBuffer.memStateBits, 
			bufferInfo.stripBuffer.minSize, 
			bufferInfo.stripBuffer.memFlags, 
			bufferInfo.stripBuffer.memCareBits, 
			bufferInfo.stripBuffer.memStateBits);
#endif

	videoStripBufferSize = bufferInfo.stripBuffer.minSize;
	videoRefBufferSize	 = bufferInfo.refBuffer.minSize;

	videoRefBuffer = AllocMemMasked(
			bufferInfo.refBuffer.minSize, 
			bufferInfo.refBuffer.memFlags, 
			bufferInfo.refBuffer.memCareBits, 
			bufferInfo.refBuffer.memStateBits);
	if (videoRefBuffer == NULL)
	{
		printf("Can't allocate video ref buffer\n");
		result = NOMEM;
		goto cleanup;
	}
	
	ioInfo.ioi_Command 			= MPEGVIDEOCMD_SETREFERENCEBUFFER;
	ioInfo.ioi_Recv.iob_Buffer	= videoRefBuffer;
	ioInfo.ioi_Recv.iob_Len		= bufferInfo.refBuffer.minSize;
	result = SendIO(videoRefBufReqItem, &ioInfo);
	if (result < 0)
	{
		printf("MPEGVIDEOCMD_SETREFERENCEBUFFER returned status: ");
		PrintfSysErr(result);
		goto cleanup;
	}
	
	videoStripBuffer = AllocMemMasked(
			bufferInfo.stripBuffer.minSize, 
			bufferInfo.stripBuffer.memFlags, 
			bufferInfo.stripBuffer.memCareBits, 
			bufferInfo.stripBuffer.memStateBits);
	if (videoStripBuffer == NULL)
	{
		printf("Can't allocate video strip buffer\n");
		result = NOMEM;
		goto cleanup;
	}
	
	ioInfo.ioi_Command 			= MPEGVIDEOCMD_SETSTRIPBUFFER;
	ioInfo.ioi_Recv.iob_Buffer	= videoStripBuffer;
	ioInfo.ioi_Recv.iob_Len		= bufferInfo.stripBuffer.minSize;
	result = SendIO(videoStripBufReqItem, &ioInfo);
	if (result < 0)
	{
		printf("MPEGVIDEOCMD_SETSTRIPBUFFER returned status: ");
		PrintfSysErr(result);
		goto cleanup;
	}

	printf("MPEG video buffers all set up!\n");
	
	result = 0;
	
cleanup:

	if (result < 0)
		CleanupMPEGBuffers();
		
	return result;
}

/*****************************************************************************/


static Err SetupMPEGDriver(void)
{
List  *list;
Err    result;
uint32 i;

    result = CreateDeviceStackListVA(&list,
		"cmds", DDF_EQ, DDF_INT, 3,
			MPEGVIDEOCMD_CONTROL,
			MPEGVIDEOCMD_WRITE,
			MPEGVIDEOCMD_READ,
                 NULL);
    if (result < 0)
	return result;

    if (IsEmptyList(list))
    {
	DeleteDeviceStackList(list);
	return MAKEKERR(ER_SEVER,ER_C_STND,ER_NoHardware);
    }

    /* Take the first device. */
    mpegVideoDevice = result = OpenDeviceStack((DeviceStack *) FirstNode(list));
    DeleteDeviceStackList(list);

    if (mpegVideoDevice >= 0)
    {
        videoReadReqItem = result = CreateIOReq(0, 0, mpegVideoDevice, 0);
        if (videoReadReqItem >= 0)
        {
            videoCmdItem = result = CreateIOReq(0, 0, mpegVideoDevice, 0);
            if (videoCmdItem >= 0)
            {
                for (i = 0; i < NUM_WRITEBUFFERS; i++)
                    videoWriteReqItem[i] = -1;

                for (i = 0; i < NUM_WRITEBUFFERS; i++)
                {
                    videoWriteReqItem[i] = result = CreateIOReq(0, 0, mpegVideoDevice, 0);
                    if (videoWriteReqItem[i] < 0)
                        break;
                }

                if (result >= 0)
				{
					result = SetupMPEGBuffers();
					if (result >= 0)
						return 0;
				}

    for (i = 0; i < NUM_WRITEBUFFERS; i++)
    {
        if (videoWriteReqItem[i] >= 0)
            DeleteIOReq(videoWriteReqItem[i]);
    }
            }
            DeleteIOReq(videoReadReqItem);
        }
        CloseDeviceStack(mpegVideoDevice);
    }

    return result;
}


/*****************************************************************************/


static void CleanupMPEGDriver(void)
{
uint32 i;

    for (i = 0; i < NUM_WRITEBUFFERS; i++)
    {
        if (videoWriteReqItem[i] >= 0)
            DeleteIOReq(videoWriteReqItem[i]);
    }

    DeleteIOReq(videoReadReqItem);
    DeleteIOReq(videoCmdItem);
	CleanupMPEGBuffers();
    CloseDeviceStack(mpegVideoDevice);
}


/*****************************************************************************/


static Err ReadMPEGFile(void **buffer, uint32 *bufferSize, const char *name)
{
RawFile *file;
FileInfo info;
Err      result;
void    *buf;

    result = OpenRawFile(&file, name, FILEOPEN_READ);
    if (result >= 0)
    {
        result = GetRawFileInfo(file, &info, sizeof(info));
        if (result >= 0)
        {
            buf = AllocMem(info.fi_ByteCount, MEMTYPE_NORMAL);
            if (buf)
            {
				TimerTicks		startTime, endTime;
				TimeVal			start, end, delta;
				double			rate;

				SampleSystemTimeTT(&startTime);
                result = ReadRawFile(file, buf, info.fi_ByteCount);
				SampleSystemTimeTT(&endTime);

                if (result == info.fi_ByteCount)
                {
                    result        = 0;
                    (*buffer)     = buf;
                    (*bufferSize) = info.fi_ByteCount;

					/* Print out the I/O data rate statistics.
					 * This is important because we need to know how well the "remote"
					 * device is working. */
					ConvertTimerTicksToTimeVal(&startTime, &start);
					ConvertTimerTicksToTimeVal(&endTime, &end);
					SubTimes(&start, &end, &delta);
					rate = (double)info.fi_ByteCount /
						((double)delta.tv_sec + (double)delta.tv_usec * 1.0e-6);
					printf("\nRead %ld bytes in %ld.%06ld seconds @ %f bytes per second\n",
						  info.fi_ByteCount, delta.tv_sec, delta.tv_usec, rate);
                }
                else
                {
                    if (result >= 0)
                        result = -1;

                    FreeMem(buf, info.fi_ByteCount);
                }
            }
            else
            {
                result = NOMEM;
            }
        }
        CloseRawFile(file);
    }

    return result;
}


/*****************************************************************************/


static Err WaitVBL(int32 n)
{
IOInfo ioInfo;

    memset(&ioInfo, 0, sizeof (IOInfo));
    ioInfo.ioi_Command = TIMERCMD_DELAY_VBL;
    ioInfo.ioi_Offset  = n;

    return DoIO(timerIO, &ioInfo);
}


/*****************************************************************************/


/* Flush pending all data from the driver so that playback can be reset to a
 * different spot within the stream.
 */
static Err Flush(void)
{
int32           index;
IOInfo          ioInfo;
FMVIOReqOptions options;
Err             result;

    for (index = 0; index < NUM_WRITEBUFFERS; index++)
    {
        result = CheckIO(videoWriteReqItem[index]);
        if (result == 0)
        {
            AbortIO(videoWriteReqItem[index]);
            WaitIO(videoWriteReqItem[index]);
        }

        if (result < 0)
            return result;
    }

    memset(&options, 0, sizeof(FMVIOReqOptions));
    memset(&ioInfo, 0, sizeof(IOInfo));

    /* To flush the mpeg driver: write a zero length buffer
     * to the driver with FMV_FLUSH_FLAG set in the ioi_CmdOptions field.
     */

    ioInfo.ioi_Command    = MPEGVIDEOCMD_WRITE;
    ioInfo.ioi_CmdOptions = (int32)&options;
    options.FMVOpt_Flags  = FMV_FLUSH_FLAG;

    return DoIO(videoCmdItem, &ioInfo);
}


/*****************************************************************************/


static void PlayMovie(uint8 *buffer, int32 bufferSize)
{
enum ViewTypes      viewType;
uint32              bufferNumber;
IOInfo              videoReadInfo;
IOInfo              videoWriteInfo[NUM_WRITEBUFFERS];
IOInfo              controlIOInfo;
CODECDeviceStatus   stat;
uint32              size;
uint32              wBuffer;
Err                 result;
ControlPadEventData cped;
uint32              code;
bool                paused;
bool                rgbViewMode;
bool                rgbDecodeMode;
bool                iframeMode;
bool                continueFlag;

    rgbViewMode   = TRUE;
    rgbDecodeMode = TRUE;
    iframeMode    = FALSE;
    paused        = FALSE;
    bufferNumber  = 1;

    /* configure MPEG video device */
    memset(&controlIOInfo, 0, sizeof(IOInfo));
    memset(&stat,0,sizeof(CODECDeviceStatus));
    controlIOInfo.ioi_Command         = MPEGVIDEOCMD_CONTROL;
    controlIOInfo.ioi_Send.iob_Buffer = &stat;
    controlIOInfo.ioi_Send.iob_Len    = sizeof(CODECDeviceStatus);
    stat.codec_TagArg[0].ta_Tag      = VID_CODEC_TAG_STANDARD;
    stat.codec_TagArg[0].ta_Arg      = (void *) kCODEC_SQUARE_RESAMPLE;
    stat.codec_TagArg[1].ta_Tag      = VID_CODEC_TAG_DEPTH;
    stat.codec_TagArg[1].ta_Arg      = (void *) pixelMode;
    stat.codec_TagArg[2].ta_Tag      = VID_CODEC_TAG_HSIZE;
    stat.codec_TagArg[2].ta_Arg      = (void *) hSize;
    stat.codec_TagArg[3].ta_Tag      = VID_CODEC_TAG_VSIZE;
    stat.codec_TagArg[3].ta_Arg      = (void *) vSize;
    stat.codec_TagArg[4].ta_Tag      = VID_CODEC_TAG_M2MODE;
    stat.codec_TagArg[5].ta_Tag      = TAG_END;
    result = DoIO(videoCmdItem, &controlIOInfo);
    if (result < 0)
    {
        ERR(("DoIO() of MPEGVIDEOCMD_CONTROL failed: "), result);
        return;
    }

    memset(&videoReadInfo, 0, sizeof(IOInfo));
    videoReadInfo.ioi_Command         = MPEGVIDEOCMD_READ;
    videoReadInfo.ioi_Send.iob_Buffer = &bitmaps[bufferNumber];
    videoReadInfo.ioi_Send.iob_Len    = sizeof(Item);

    /* find the last picture start code */
    code = 0xffffffff;
    while (code != 0x00000100)
    {
        if ( --bufferSize < 0)
        {
            printf("Couldn't find the last picture's start code\n");
            return;
        }
        code >>= 8;
        code |= buffer[bufferSize] << 24;
    }

    if (bufferSize >= MAXWRITESIZE)
        size = MAXWRITESIZE;
    else
        size = bufferSize;

    for (wBuffer = 0; wBuffer < NUM_WRITEBUFFERS; wBuffer++)
    {
        memset(&videoWriteInfo[wBuffer], 0, sizeof(IOInfo));

        videoWriteInfo[wBuffer].ioi_Command         = MPEGVIDEOCMD_WRITE;
        videoWriteInfo[wBuffer].ioi_Send.iob_Buffer = &buffer[wBuffer * MAXWRITESIZE];

        if ( ((wBuffer + 1) * MAXWRITESIZE) > bufferSize)
            size = bufferSize - (wBuffer * MAXWRITESIZE);
        else
            size = MAXWRITESIZE;

        videoWriteInfo[wBuffer].ioi_Send.iob_Len = size;

        result = SendIO(videoWriteReqItem[ wBuffer ], &videoWriteInfo[wBuffer]);
        if (result < 0)
        {
            ERR(("SendIO() of write request failed: "), result);
            return;
        }
    }

    result = SendIO(videoReadReqItem, &videoReadInfo);
    if (result < 0)
    {
        ERR(("SendIO() of read request failed: "), result);
        return;
    }

    while (TRUE)
    {
        continueFlag = FALSE;
        do
        {
            result = GetControlPad(1, paused, &cped);
            if (result < 0)
            {
                ERR(("GetControlPad() failed: "), result);
                return;
            }

            if (result == 1)
            {
                if (cped.cped_ButtonBits & ControlLeftShift )
                {
                    /* toggle YUV/RGB view mode */
                    if (rgbViewMode)
                    {
                        printf("PlayFast: requesting YUV display mode\n");
                        if (pixelMode == 16)
                        {
                            if (mode640)
                                viewType = VIEWTYPE_YUV_16_640_LACE;
                            else
                                viewType = VIEWTYPE_YUV_16;
                        }
                        else
                        {
                            if (mode640)
                                viewType = VIEWTYPE_YUV_32_640_LACE;
                            else
                                viewType = VIEWTYPE_YUV_32;
                        }
                    }
                    else
                    {
                        printf("PlayFast: requesting RGB display mode\n");
                        if (pixelMode == 16)
                        {
                            if (mode640)
                                viewType = VIEWTYPE_16_640_LACE;
                            else
                                viewType = VIEWTYPE_16;
                        }
                        else
                        {
                            if (mode640)
                                viewType = VIEWTYPE_32_640_LACE;
                            else
                                viewType = VIEWTYPE_32;
                        }
                    }

                    result = ModifyGraphicsItemVA(view, VIEWTAG_VIEWTYPE, viewType, TAG_END);
                    if (result >= 0)
                        rgbViewMode = !rgbViewMode;
                }

                if (cped.cped_ButtonBits & ControlRightShift)
                {
                    /* toggle YUV/RGB decoder mode */
                    if (rgbDecodeMode)
                    {
                        printf("PlayFast: requesting YUV decode mode\n");
                        stat.codec_TagArg[0].ta_Tag = VID_CODEC_TAG_YUVMODE;
                        stat.codec_TagArg[1].ta_Tag = TAG_END;
                    }
                    else
                    {
                        printf("PlayFast: requesting RGB decode mode\n");
                        stat.codec_TagArg[0].ta_Tag = VID_CODEC_TAG_RGBMODE;
                        stat.codec_TagArg[1].ta_Tag = TAG_END;
                    }

                    result = DoIO(videoCmdItem, &controlIOInfo);
                    if (result >= 0)
                        rgbDecodeMode = !rgbDecodeMode;
                }

                if (cped.cped_ButtonBits & ControlLeft)
                {
                   result = Flush();
                   if (result < 0)
                   {
                       ERR(("Flush() failed: "), result);
                       return;
                   }
                   wBuffer      = 0;
                   bufferNumber = 0;
                }

                if (cped.cped_ButtonBits & ControlRight)
                {
                    if (iframeMode)
                    {
                        printf("PlayFast: requesting all frames\n");
                        stat.codec_TagArg[0].ta_Tag = VID_CODEC_TAG_PLAY;
                        stat.codec_TagArg[1].ta_Tag = TAG_END;
                    }
                    else
                    {
                        printf("PlayFast: requesting I-frames only\n");
                        stat.codec_TagArg[0].ta_Tag = VID_CODEC_TAG_KEYFRAMES;
                        stat.codec_TagArg[1].ta_Tag = TAG_END;
                    }

                    result = DoIO(videoCmdItem, &controlIOInfo);
                    if (result >= 0)
                        iframeMode = !iframeMode;
                }

                if (cped.cped_ButtonBits & ControlUp)
                {
                    if (!paused)
                        vblCount = (vblCount > 0) ? --vblCount : 0;
                }

                if (cped.cped_ButtonBits & ControlDown)
                {
                    if (!paused)
                        vblCount++;
                }

                if (cped.cped_ButtonBits & ControlX)
                    return;

                if (cped.cped_ButtonBits & ControlStart)
                    paused = paused ? FALSE : TRUE;

                if (cped.cped_ButtonBits & ControlA)
                {
                    if (!paused)
                        vblCount = 5;
                }

                if (cped.cped_ButtonBits & ControlB)
                {
                    if (!paused)
                        vblCount = 4;
                }

                if (cped.cped_ButtonBits & ControlC)
                {
                    if (!paused)
                        vblCount = 0;
                }
            }
        }
        while (paused);

        /* if a read request is complete, display the buffer, and ask to read
         * more.
         */
        if (CheckIO(videoReadReqItem))
        {
            result = WaitIO(videoReadReqItem);
            if ((result < 0) && (result != ABORTED))
            {
                ERR(("WaitIO() of read request failed: "), result);
                return;
            }

            /* Flip buffers */
            result = ModifyGraphicsItemVA(view, VIEWTAG_BITMAP, bitmaps[bufferNumber], TAG_END);
            if (result < 0)
            {
                ERR(("ModifyGraphicsItem() failed: "), result);
                return;
            }

            if (vblCount)
            {
                if (bufferNumber && (vblCount & 0x1))
                    result = WaitVBL(vblCount / 2 + 1);
                else
                    result = WaitVBL(vblCount / 2);

                if (result < 0)
                {
                    ERR(("WaitVBL() failed:"), result);
                }
            }
            bufferNumber ^= 1;

            videoReadInfo.ioi_Send.iob_Buffer = &bitmaps[bufferNumber];
            result = SendIO(videoReadReqItem, &videoReadInfo);
            if (result < 0)
            {
                ERR(("SendIO() of read request failed: "), result);
                return;
            }

            continueFlag = TRUE;
        }

        /* if a write request is complete, dispatch another one */
        if (CheckIO(videoWriteReqItem[wBuffer & 0x1L]))
        {
            result = WaitIO(videoWriteReqItem[wBuffer & 0x1]);
            if ((result < 0) && (result != ABORTED))
            {
                ERR(("WaitIO() of write request failed: "), result);
                return;
            }

            if ((wBuffer * MAXWRITESIZE) >= bufferSize)
            {
                if (loopCount-- <= 0)
                    break;

                wBuffer = 0;
                continue;
            }

            videoWriteInfo[wBuffer & 0x1L].ioi_Send.iob_Buffer = &buffer[wBuffer*size];

            if (((wBuffer + 1) * MAXWRITESIZE) > bufferSize)
                size = bufferSize - (wBuffer * MAXWRITESIZE);
            else
                size = MAXWRITESIZE;
            videoWriteInfo[wBuffer & 0x1L].ioi_Send.iob_Len = size;

            result = SendIO(videoWriteReqItem[wBuffer & 0x1], &videoWriteInfo[wBuffer & 0x1]);
            if (result < 0)
            {
                ERR(("SendIO() of write request failed: "), result);
                return;
            }

            wBuffer++;

            continueFlag = TRUE;
        }

        if (continueFlag)
            continue;

        WaitSignal(SIGF_IODONE);
    }
}


/*****************************************************************************/


static void PrintUsage(void)
{
    printf("Usage: PlayFast -f <filename> [ options ]\n"
           "       -16            - play in 16 bit color\n"
           "       -640           - decode in 640x480 mode\n"
           "       -l <loopCount> - repeat loopCount times\n"
		   "       -b             - use app-provide video decode buffers\n"
           "\n"
           "  Control Pad functions:\n"
           "    Pause        Pause/Resume movie\n"
           "    Stop         Stop movie\n"
           "    Up           Play faster\n"
           "    Down         Play slower\n"
           "    Left         Branch to start of movie\n"
           "    Right        Toggle I/All frame(s) mode\n"
           "    Left Shift   Toggle YUV/RGB view mode\n"
           "    Right Shift  Toggle YUV/RGB decoder mode\n"
           "    A            Play normal speed\n"
           "    C            Play fastest\n"
           );
}


/*****************************************************************************/


int main(int argc, char *argv[])
{
int32   arg;
Err     result;
char   *fileName;
void   *buffer;
uint32  bufferSize;

    /* set defaults */
    fileName  = NULL;
    pixelMode = 24;
    vblCount  = 5;
    hSize     = 320;
    vSize     = 240;
    mode640   = FALSE;
    loopCount = 1000000;

    /* process command line args */
    for (arg = 1; arg < argc; arg++)
    {
        if (strcasecmp(argv[arg], "-f") == 0)
        {
            fileName = argv[++arg];
        }
        else if (strcasecmp(argv[arg], "-vbl") == 0)
        {
            vblCount = atoi(argv[++arg]);
        }
        else if (strcasecmp(argv[arg], "-16") == 0)
        {
            pixelMode = 16;
        }
        else if (strcasecmp(argv[arg], "-640") == 0)
        {
            mode640 = TRUE;
            hSize   = 640;
            vSize   = 480;
        }
        else if (strcasecmp(argv[arg], "-l") == 0)
        {
            loopCount = atoi(argv[++arg]);
        }
        else if (strcasecmp(argv[arg], "-b") == 0)
        {
            useAppVideoBuffers = 1;
        }
        else
        {
            PrintUsage();
            return -1;
        }
    }

    if (fileName == NULL)
    {
        PrintUsage();
        return -1;
    }

    /* prepare things */
    result = InitEventUtility(1, 0, TRUE);
    if (result >= 0)
    {
        timerIO = result = CreateTimerIOReq();
        if (timerIO >= 0)
        {
            result = SetupGraphics();
            if (result >= 0)
            {
                result = SetupMPEGDriver();
                if (result >= 0)
                {
                    result = ReadMPEGFile(&buffer, &bufferSize, fileName);
                    if (result >= 0)
                    {
                        /* done setting up, play the darn thing! */
                        PlayMovie(buffer, bufferSize);

                        FreeMem(buffer, bufferSize);
                    }
                    else
                    {
                        ERR(("ReadMPEGFile() failed:"), result);
                    }
                    CleanupMPEGDriver();
                }
                else
                {
                    ERR(("SetupMPEGDriver() failed:"), result);
                }
                CleanupGraphics();
            }
            else
            {
                ERR(("SetupGraphics() failed:"), result);
            }
            DeleteTimerIOReq(timerIO);
        }
        else
        {
            ERR(("CreateTimerIOReq() failed:"), result);
        }
        KillEventUtility();
    }
    else
    {
        ERR(("InitEventUtility() failed:"), result);
    }

    return result;
}
