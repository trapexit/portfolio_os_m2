/* @(#) driver.c 96/10/03 1.55 */

#include <kernel/types.h>
#include <kernel/operror.h>
#include <kernel/super.h>
#include <kernel/mem.h>
#include <kernel/tags.h>
#include <kernel/io.h>
#include <kernel/task.h>
#include <kernel/cache.h>
#include <kernel/driver.h>
#include <kernel/device.h>
#include <kernel/interrupts.h>
#include <kernel/time.h>
#include <kernel/timer.h>
#include <kernel/kernel.h>
#include <kernel/debug.h>
#include <hardware/te.h>
#include <graphics/bitmap.h>
#include <graphics/view.h>
#include <graphics/gfx_pvt.h>
#include <graphics/clt/cltmacros.h>
#include <device/te.h>
#include <loader/loader3do.h>
#include <string.h>
#include <stdio.h>
#include "driver.h"
#include "hw.h"
#include "context.h"


/*****************************************************************************/


#define TRACE(x) /* printf x */

#ifdef BUILD_STRINGS
#define INFO(x) printf x
#else
#define INFO(x)
#endif


/*****************************************************************************/


/* FIXME: stubs until graphics implements these for real */
Err SuperModifyGraphicsItem(Item it, const TagArg *tags);
Err SuperSetOnScreenCallBack(Item it, void *a, void *b) {TOUCH(it); TOUCH(a); TOUCH(b); return 0;}
Err SuperSetOffScreenCallBack(Item it, void *a, void *b) {TOUCH(it); TOUCH(a); TOUCH(b); return 0;}
Err SuperSetBitmapCallBack(void *a, void *b) {TOUCH(a); TOUCH(b); return 0;}

Err SuperSetViewBitmap(Item view, Item bitmap)
{
TagArg tags[2];

    tags[0].ta_Tag = VIEWTAG_BITMAP;
    tags[0].ta_Arg = (void *)bitmap;
    tags[1].ta_Tag = TAG_END;
    return SuperModifyGraphicsItem(view, tags);
}

#define BM_OFFSCREEN 1
#define BM_DISPLAYED 2
#define BM_ATTACHED  4


/*****************************************************************************/


static const ClearFBCmdList clearFBCmdList =
{
    CLT_WriteRegistersHeader(DCNTL, 1),
      CLT_Bits(DCNTL, SYNC, 1),

    CLT_ClearRegistersHeader(TXTADDRCNTL, 1),
      FV_TXTADDRCNTL_TEXTUREENABLE_MASK,

    CLT_ClearRegistersHeader(TXTTABCNTL, 1),
      FV_TXTTABCNTL_COLOROUT_MASK |
      FV_TXTTABCNTL_ALPHAOUT_MASK,

    CLT_SetRegistersHeader(TXTTABCNTL, 1),
      CLT_Const(TXTTABCNTL,COLOROUT,PRIMCOLOR) |
      CLT_Const(TXTTABCNTL,ALPHAOUT,PRIMALPHA),

    CLT_SetRegistersHeader(DBUSERCONTROL, 1),
      CLT_Mask(DBUSERCONTROL, DESTOUTMASK),

    CLT_ClearRegistersHeader(DBUSERCONTROL, 1),
      CLT_Mask(DBUSERCONTROL, BLENDEN) |
      CLT_Mask(DBUSERCONTROL, SRCEN)   |
      CLT_Mask(DBUSERCONTROL, ZBUFFEN) |
      CLT_Mask(DBUSERCONTROL, ZOUTEN),

    CLT_WriteRegistersHeader(DBDISCARDCONTROL, 1),
      CLT_Bits(DBDISCARDCONTROL, ZCLIPPED, 0) |
      CLT_Bits(DBDISCARDCONTROL, SSB0,     0) |
      CLT_Bits(DBDISCARDCONTROL, RGB0,     0) |
      CLT_Bits(DBDISCARDCONTROL, ALPHA0,   0),

    RC_WRITE_VERTEX |
      CLT_Bits(TRIANGLE, NEW,         1)        |
      CLT_Bits(TRIANGLE, STRIPFAN,    RC_STRIP) |
      CLT_Bits(TRIANGLE, PERSPECTIVE, 0)        |
      CLT_Bits(TRIANGLE, TEXTURE,     0)        |
      CLT_Bits(TRIANGLE, SHADING,     1)        |
      CLT_Bits(TRIANGLE, COUNT,       3),

    {0},
    {0},
    {0},
    {0},

    CLT_WriteRegistersHeader(DCNTL, 1),
      CLT_Bits(DCNTL, SYNC, 1),

    CLT_WriteRegistersHeader(DCNTLDATA, 2),
      0,
      CLT_Bits(DCNTL, JA, 1)
};

static const ClearZBCmdList clearZBCmdList =
{
    CLT_WriteRegistersHeader(DCNTL, 1),
      CLT_Bits(DCNTL, SYNC, 1),

    CLT_ClearRegistersHeader(TXTADDRCNTL, 1),
      FV_TXTADDRCNTL_TEXTUREENABLE_MASK,

    CLT_ClearRegistersHeader(TXTTABCNTL, 1),
      FV_TXTTABCNTL_COLOROUT_MASK |
      FV_TXTTABCNTL_ALPHAOUT_MASK,

    CLT_SetRegistersHeader(TXTTABCNTL, 1),
      CLT_Const(TXTTABCNTL,COLOROUT,PRIMCOLOR) |
      CLT_Const(TXTTABCNTL,ALPHAOUT,PRIMALPHA),

    CLT_SetRegistersHeader(DBUSERCONTROL, 1),
      CLT_Mask(DBUSERCONTROL, DESTOUTMASK),

    CLT_ClearRegistersHeader(DBUSERCONTROL, 1),
      CLT_Mask(DBUSERCONTROL, BLENDEN) |
      CLT_Mask(DBUSERCONTROL, SRCEN)   |
      CLT_Mask(DBUSERCONTROL, ZBUFFEN) |
      CLT_Mask(DBUSERCONTROL, ZOUTEN),

    CLT_WriteRegistersHeader(DBDISCARDCONTROL, 1),
      CLT_Bits(DBDISCARDCONTROL, ZCLIPPED, 0) |
      CLT_Bits(DBDISCARDCONTROL, SSB0,     0) |
      CLT_Bits(DBDISCARDCONTROL, RGB0,     0) |
      CLT_Bits(DBDISCARDCONTROL, ALPHA0,   0),

    RC_WRITE_VERTEX |
      CLT_Bits(TRIANGLE, NEW,         1)        |
      CLT_Bits(TRIANGLE, STRIPFAN,    RC_STRIP) |
      CLT_Bits(TRIANGLE, PERSPECTIVE, 0)        |
      CLT_Bits(TRIANGLE, TEXTURE,     0)        |
      CLT_Bits(TRIANGLE, SHADING,     1)        |
      CLT_Bits(TRIANGLE, COUNT,       3),

    {0},
    {0},
    {0},
    {0},

    CLT_WriteRegistersHeader(DCNTL, 1),
      CLT_Bits(DCNTL, SYNC, 1),

    CLT_WriteRegistersHeader(DCNTL, 1),
      CLT_Bits(DCNTL, PSE, 1),
};


/*****************************************************************************/


static DriverState driverState;


/*****************************************************************************/


static void OnScreenHandler(struct View *v, DriverState *ds);
static void OffScreenHandler(struct View *v, DriverState *ds);

typedef	void (*gfxcallback)(struct View *, void *);


/*****************************************************************************/


static void StartCmdList(IOReq *ior)
{
DriverState *ds;
DeviceState *devState;
uint32       cmdList;
uint32       endList;

    ds       = DRIVERSTATE(ior);
    devState = DEVICESTATE(ior);

    cmdList = (uint32)ior->io_Info.ioi_Send.iob_Buffer;
    endList = cmdList+ior->io_Info.ioi_Send.iob_Len;

    if ((ior->io_Info.ioi_CmdOptions & TE_CLEAR_Z_BUFFER) && ds->ds_ZBuffer)
    {
        /* need to clear the Z buffer before proceeding further */
        SetTEFrameBuffer(ds->ds_ZBuffer);
        SetTEZBuffer(NULL);
        cmdList = (uint32)&devState->ds_ClearZBCmdList;
    }
    else if ((ior->io_Info.ioi_CmdOptions & TE_CLEAR_FRAME_BUFFER) && ds->ds_FrameBuffer)
    {
        /* need to clear the frame buffer before proceeding */
        devState->ds_ClearFBCmdList.cl_JumpAddr = cmdList;
        FlushDCache(0, &devState->ds_ClearFBCmdList, sizeof(ClearFBCmdList));
        cmdList = (uint32)&devState->ds_ClearFBCmdList;
    }

    /* load the start address */
    WriteRegister(TE_IMMINSTR_DATA_ADDR, cmdList);

    /* Set the IWP to point past the end of the buffer */
    WriteRegister(TE_INSTR_WRITE_PTR_ADDR, endList);

    if (devState->ds_Control == DEVCTRL_NORMAL)
    {
        /* execute until pause instruction */
        WriteRegister(TE_IMMINSTR_CNTRL_ADDR, TE_IMMINSTR_START |
                                              TE_IMMINSTR_INTERRUPT);
        /* setup the one second timeout */
        (* ds->ds_TimeoutTimer->tm_Load)(ds->ds_TimeoutTimer, &ds->ds_TimeoutTime);
    }
    else
    {
        /* execute the next command only */
        WriteRegister(TE_IMMINSTR_CNTRL_ADDR, TE_IMMINSTR_STEP |
                                              TE_IMMINSTR_START |
                                              TE_IMMINSTR_INTERRUPT);

        if (!devState->ds_FullStop)
            (* ds->ds_SpeedBumpTimer->tm_Load)(ds->ds_SpeedBumpTimer, &devState->ds_SpeedBumpTime);
    }
}


/*****************************************************************************/


static void UpdateClearFBCmdList(DeviceState *devState, IOReq *ior)
{
uint32  width;
uint32  height;
uint32  red;
uint32  green;
uint32  blue;
uint32  alpha;

    width  = *(uint32 *)&TEIO(ior)->teio_Width;
    height = *(uint32 *)&TEIO(ior)->teio_Height;
    red    = *(uint32 *)&devState->ds_FBInfo.tefbi_Red;
    green  = *(uint32 *)&devState->ds_FBInfo.tefbi_Green;
    blue   = *(uint32 *)&devState->ds_FBInfo.tefbi_Blue;
    alpha  = *(uint32 *)&devState->ds_FBInfo.tefbi_Alpha;

    devState->ds_ClearFBCmdList.cl_TopLeft.v_Red       = red;
    devState->ds_ClearFBCmdList.cl_TopLeft.v_Green     = green;
    devState->ds_ClearFBCmdList.cl_TopLeft.v_Blue      = blue;
    devState->ds_ClearFBCmdList.cl_TopLeft.v_Alpha     = alpha;

    devState->ds_ClearFBCmdList.cl_BottomLeft.v_Y      = height;
    devState->ds_ClearFBCmdList.cl_BottomLeft.v_Red    = red;
    devState->ds_ClearFBCmdList.cl_BottomLeft.v_Green  = green;
    devState->ds_ClearFBCmdList.cl_BottomLeft.v_Blue   = blue;
    devState->ds_ClearFBCmdList.cl_BottomLeft.v_Alpha  = alpha;

    devState->ds_ClearFBCmdList.cl_TopRight.v_X        = width;
    devState->ds_ClearFBCmdList.cl_TopRight.v_Red      = red;
    devState->ds_ClearFBCmdList.cl_TopRight.v_Green    = green;
    devState->ds_ClearFBCmdList.cl_TopRight.v_Blue     = blue;
    devState->ds_ClearFBCmdList.cl_TopRight.v_Alpha    = alpha;

    devState->ds_ClearFBCmdList.cl_BottomRight.v_X     = width;
    devState->ds_ClearFBCmdList.cl_BottomRight.v_Y     = height;
    devState->ds_ClearFBCmdList.cl_BottomRight.v_Red   = red;
    devState->ds_ClearFBCmdList.cl_BottomRight.v_Green = green;
    devState->ds_ClearFBCmdList.cl_BottomRight.v_Blue  = blue;
    devState->ds_ClearFBCmdList.cl_BottomRight.v_Alpha = alpha;
}


/*****************************************************************************/


static void UpdateClearZBCmdList(DeviceState *devState, IOReq *ior)
{
uint32 width;
uint32 height;
uint32 zfill;

    width  = *(uint32 *)&TEIO(ior)->teio_Width;
    height = *(uint32 *)&TEIO(ior)->teio_Height;
    zfill  = *(uint32 *)&devState->ds_ZBInfo.tezbi_FillValue;

    devState->ds_ClearZBCmdList.cl_TopLeft.v_Red       = zfill;
    devState->ds_ClearZBCmdList.cl_TopLeft.v_Green     = zfill;
    devState->ds_ClearZBCmdList.cl_TopLeft.v_Blue      = zfill;

    devState->ds_ClearZBCmdList.cl_BottomLeft.v_Y      = height;
    devState->ds_ClearZBCmdList.cl_BottomLeft.v_Red    = zfill;
    devState->ds_ClearZBCmdList.cl_BottomLeft.v_Green  = zfill;
    devState->ds_ClearZBCmdList.cl_BottomLeft.v_Blue   = zfill;

    devState->ds_ClearZBCmdList.cl_TopRight.v_X        = width;
    devState->ds_ClearZBCmdList.cl_TopRight.v_Red      = zfill;
    devState->ds_ClearZBCmdList.cl_TopRight.v_Green    = zfill;
    devState->ds_ClearZBCmdList.cl_TopRight.v_Blue     = zfill;

    devState->ds_ClearZBCmdList.cl_BottomRight.v_X     = width;
    devState->ds_ClearZBCmdList.cl_BottomRight.v_Y     = height;
    devState->ds_ClearZBCmdList.cl_BottomRight.v_Red   = zfill;
    devState->ds_ClearZBCmdList.cl_BottomRight.v_Green = zfill;
    devState->ds_ClearZBCmdList.cl_BottomRight.v_Blue  = zfill;
    devState->ds_ClearZBCmdList.cl_BottomRight.v_Alpha = zfill;

    FlushDCache(0, &devState->ds_ClearZBCmdList, sizeof(ClearZBCmdList));
}


/*****************************************************************************/


/*
 * !!! Manipulates shared data structures.
 * !!! MUST be executed with interrupts disabled.
 */
static void StartIO(DriverState *ds, IOReq *ior)
{
DeviceState       *devState;
TEFrameBufferInfo *fbi;
TEZBufferInfo     *zbi;
uint32             size;

    TRACE(("StartIO: entering with ior 0x%x\n", ior));

    while (TRUE)
    {
        if (ior == NULL)
        {
            ior = (IOReq *)RemHead(&ds->ds_PendingIOReqs);
            if (ior == NULL)
            {
                /* nothing left to do */
                ds->ds_CurrentIOReq = NULL;

                /* disable time out */
                (* ds->ds_TimeoutTimer->tm_Unload)(ds->ds_TimeoutTimer);
                (* ds->ds_SpeedBumpTimer->tm_Unload)(ds->ds_SpeedBumpTimer);
                return;
            }
        }

        devState = DEVICESTATE(ior);

        if ((devState->ds_Control == DEVCTRL_WAIT_OFFSCREEN)
         || (devState->ds_Control == DEVCTRL_WAIT_DISPLAYED))
        {
            /* this device has suspended normal processing for now... */
            AddTail(&devState->ds_Waiters, (Node *)ior);
            ior = NULL;
        }
        else if (ior->io_Info.ioi_Command == TE_CMD_EXECUTELIST)
        {
            if (ior->io_Info.ioi_CmdOptions & TE_WAIT_UNTIL_OFFSCREEN)
            {
                if (devState->ds_View)
                {
                    /*
                     * The target Bitmap/View must be offscreen before
                     * rendering proceeds.
                     */
                    if (devState->ds_View->v_ViewFlags & VIEWF_PENDINGSIG_REND)
                    {
                        /* the target bitmap is still on screen, wait for it */
                        SuperInternalSetRendCallBack (devState->ds_View, (gfxcallback) OffScreenHandler, ds);
                        devState->ds_Control = DEVCTRL_WAIT_OFFSCREEN;
                        continue;	/*  Stack ior to ds_Waiters  */
                    }
                }
            }

            /* check if this IOReq originated from a different device */
            if (ds->ds_Device != devState)
            {
                if (ds->ds_Device)
                    SaveTEContext(ds->ds_Device);

                RestoreTEContext(devState);
                ds->ds_Device      = devState;
                ds->ds_FrameBuffer = NULL;
                ds->ds_ZBuffer     = NULL;
            }

            /* check if the frame buffer has changed */
            if (devState->ds_FrameBuffer != ds->ds_FrameBuffer)
            {
                ds->ds_FrameBuffer = devState->ds_FrameBuffer;
                SetTEFrameBuffer(ds->ds_FrameBuffer);
            }

            /* check if the Z buffer has changed */
            if (devState->ds_ZBuffer != ds->ds_ZBuffer)
            {
                ds->ds_ZBuffer = devState->ds_ZBuffer;
                SetTEZBuffer(ds->ds_ZBuffer);
            }

#ifdef BUILD_DEBUGGER
            if (ds->ds_Idling)
            {
            TimerTicks resultTT, endTT;

                ds->ds_Idling = FALSE;
                SampleSystemTimeTT(&endTT);
                SubTimerTicks(&ds->ds_IdleStartTime, &endTT, &resultTT);
                AddTimerTicks(&ds->ds_TotalIdleTime, &resultTT, &ds->ds_TotalIdleTime);
            }
#endif

            ClearTEStatus();
            ds->ds_CurrentIOReq = ior;
            devState->ds_VBLAbortCount = devState->ds_VBLAbortCountStart;
            StartCmdList(ior);
            return;
        }
        else if (ior->io_Info.ioi_Command == TE_CMD_SETFRAMEBUFFER)
        {
            devState->ds_FrameBuffer = (Bitmap *)ior->io_Info.ioi_Offset;

            fbi = ior->io_Info.ioi_Send.iob_Buffer;
            if (fbi)
            {
                size = ior->io_Info.ioi_Send.iob_Len;
                if (size > sizeof(TEFrameBufferInfo))
                    size = sizeof(TEFrameBufferInfo);

                memcpy(&devState->ds_FBInfo, fbi, size);
            }

            if (devState->ds_FrameBuffer)
                UpdateClearFBCmdList(devState, ior);

            SuperCompleteIO(ior);
            ior = NULL;
        }
        else if (ior->io_Info.ioi_Command == TE_CMD_DISPLAYFRAMEBUFFER)
        {
            if (devState->ds_FrameBuffer == NULL)
            {
                ior->io_Error = TE_ERR_NOFRAMEBUFFER;
            }
            else if (!devState->ds_View)
            {
                ior->io_Error = TE_ERR_NOVIEW;
            }
            else
            {
                /*
                 * Wait until previous View state has become visible.
                 */
                if (devState->ds_View->v_ViewFlags & VIEWF_PENDINGSIG_DISP)
                {
                    SuperInternalSetDispCallBack (devState->ds_View, (gfxcallback) OnScreenHandler, ds);
                    devState->ds_Control = DEVCTRL_WAIT_DISPLAYED;
                    continue;	/*  Stack ior to ds_Waiters  */
                }
                ior->io_Error = SuperSetViewBitmap(devState->ds_View->v.n_Item,
                                                   devState->ds_FrameBuffer->bm.n_Item);
            }

            SuperCompleteIO(ior);
            ior = NULL;
        }
        else if (ior->io_Info.ioi_Command == TE_CMD_SETZBUFFER)
        {
            devState->ds_ZBuffer = (Bitmap *)ior->io_Info.ioi_Offset;

            zbi = ior->io_Info.ioi_Send.iob_Buffer;
            if (zbi)
            {
                size = ior->io_Info.ioi_Send.iob_Len;
                if (size > sizeof(TEZBufferInfo))
                    size = sizeof(TEZBufferInfo);

                memcpy(&devState->ds_ZBInfo, zbi, size);
            }

            if (devState->ds_ZBuffer)
                UpdateClearZBCmdList(devState, ior);

            SuperCompleteIO(ior);
            ior = NULL;
        }
        else if (ior->io_Info.ioi_Command == TE_CMD_SETVIEW)
        {
            if (devState->ds_View) {
                /*  Remove previous callbacks.  */
                SuperInternalSetRendCallBack (devState->ds_View, NULL, NULL);
                SuperInternalSetDispCallBack (devState->ds_View, NULL, NULL);
            }

            if (devState->ds_View = (View *) ior->io_Info.ioi_Offset) {
                /*
                 * Install callbacks; will get scheduled with next call to
                 * ModifyGraphicsItem().
                 */
                SuperInternalSetRendCallBack (devState->ds_View, (gfxcallback) OffScreenHandler, NULL);
                SuperInternalSetDispCallBack (devState->ds_View, (gfxcallback) OnScreenHandler, NULL);
            }
            SuperCompleteIO (ior);
            ior = NULL;
        } else if (ior->io_Info.ioi_Command == TE_CMD_SETVBLABORTCOUNT)
        {
            if (ior->io_Info.ioi_Offset < 0)
                ior->io_Error = TE_ERR_BADIOARG;
            else
                devState->ds_VBLAbortCountStart = ior->io_Info.ioi_Offset;

            SuperCompleteIO (ior);
            ior = NULL;
        }
    }
}


/*****************************************************************************/


static void ResetDriver(DriverState *ds)
{
    TRACE(("ResetDriver: entering\n"));

    InitTE();

    /* make sure things get reloaded properly */
    ds->ds_FrameBuffer = NULL;
    ds->ds_ZBuffer     = NULL;
    ds->ds_Device      = NULL;
}


/*****************************************************************************/

bool TEPaused(void)
{
uint32      *irp;

    irp = (uint32 *)(0x40000000 + ReadRegister(TE_INSTR_READ_PTR_ADDR));

    return((irp[0] == CLT_WriteRegistersHeader(DCNTL, 1)) &&
           (irp[1] == CLT_Bits(DCNTL, PSE, 1)));
}

/*****************************************************************************/

static void AbortTEIO(IOReq *ior)
{
DriverState *ds;
DeviceState *devState;
IOReq       *n;

    TRACE(("AbortTEIO: entering with ior 0x%x\n", ior));

    ds       = DRIVERSTATE(ior);
    devState = DEVICESTATE(ior);

    if (ior == ds->ds_CurrentIOReq)
    {
        /* stop everything and reset to a known state */
        ResetDriver(ds);
        ds->ds_CurrentIOReq = NULL;
        (* ds->ds_TimeoutTimer->tm_Unload)(ds->ds_TimeoutTimer);
        (* ds->ds_SpeedBumpTimer->tm_Unload)(ds->ds_SpeedBumpTimer);
    }
    else
    {
        /* the IOReq is either in the driver's pending queue, in the
         * device's wait queue, or in the driver's displaywait queue
         */
        if (ior == (IOReq *)FirstNode(&devState->ds_Waiters))
        {
            /* This is the IO that has blocked this device. Since the IO is
             * going away, restore the device to the normal state and requeue
             * all the waiting IOs into the driver's pending queue
             */
            while (n = (IOReq *)RemHead(&devState->ds_Waiters))
                InsertNodeFromTail(&ds->ds_PendingIOReqs, (Node *)n);

            devState->ds_Control = DEVCTRL_NORMAL;
        }

        /* remove it from whatever queue it's on */
        RemNode((Node *)ior);
    }

    ior->io_Error = TE_ERR_ABORTED;
    SuperCompleteIO(ior);

    if (ds->ds_CurrentIOReq == NULL)
        StartIO(ds, NULL);
}


/*****************************************************************************/


/* some rendering has been clipped by the window clipper */
static int32 CLIP_IntHandler(FirqNode *fn)
{
    TRACE(("CLIP_IntHandler: entering\n"));

    TOUCH(fn);

    ClearTEInterrupts(TE_INTRSTAT_WINCLIP |
                      TE_INTRSTAT_FBCLIP);

    return 0;
}


/*****************************************************************************/


/* an immediate command has completed */
static int32 IMINST_IntHandler(FirqNode *fn)
{
DriverState *ds;
DeviceState *devState;

    TRACE(("IMINST_IntHandler: entering\n"));

    ds       = (DriverState *)fn->firq_Data;
    devState = DEVICESTATE(ds->ds_CurrentIOReq);

    ClearRegister(TE_INTERRUPT_STATUS_ADDR, TE_INTRSTAT_IMMINSTR);

    if ((devState->ds_Control == DEVCTRL_NORMAL)
      || ((devState->ds_Control == DEVCTRL_SPEEDBUMP) && (devState->ds_PauseInstruction)))
    {
        if (devState->ds_Control == DEVCTRL_SPEEDBUMP)
        {
            devState->ds_PauseInstruction = FALSE;
            (* ds->ds_SpeedBumpTimer->tm_Unload)(ds->ds_SpeedBumpTimer);
        }

        if (ds->ds_CurrentIOReq->io_Info.ioi_CmdOptions & TE_CLEAR_Z_BUFFER)
        {
            /* this IO just finished clearing the Z buffer, switch to the normal
             * frame buffer and run the user's command list.
             */
            ds->ds_CurrentIOReq->io_Info.ioi_CmdOptions &= (~TE_CLEAR_Z_BUFFER);
            SetTEFrameBuffer(ds->ds_FrameBuffer);
            SetTEZBuffer(ds->ds_ZBuffer);
            StartCmdList(ds->ds_CurrentIOReq);
        }
        else
        {
            SuperCompleteIO(ds->ds_CurrentIOReq);
            StartIO(ds, NULL);

#ifdef BUILD_DEBUGGER
            if (ds->ds_CurrentIOReq == NULL)
            {
                ds->ds_Idling = TRUE;
                SampleSystemTimeTT(&ds->ds_IdleStartTime);
            }
#endif
        }
    }
    else if (devState->ds_Control == DEVCTRL_STEPPING)
    {
        SuperCompleteIO(devState->ds_SteppingIO);
        devState->ds_Control = DEVCTRL_SPEEDBUMP;
    }
    else if (devState->ds_Control == DEVCTRL_SPEEDBUMP)
    {
        /* nothing to do but wait for the speed bump time out */
    }

    return 0;
}


/*****************************************************************************/


/* a command list has executed an INT instruction */
static int32 DFINST_IntHandler(FirqNode *fn)
{
DriverState *ds;
DeviceState *devState;

    TRACE(("DFINST_IntHandler: entering\n"));

    ds       = (DriverState *)fn->firq_Data;
    devState = DEVICESTATE(ds->ds_CurrentIOReq);

    devState->ds_FullStop = TRUE;
    devState->ds_Control  = DEVCTRL_SPEEDBUMP;

    ClearRegister(TE_INTERRUPT_STATUS_ADDR, TE_INTRSTAT_DEFINSTR |
                                            TE_INTRSTAT_DEFINSTR_VECTOR);

    return 0;
}


/*****************************************************************************/

/* general TE interrupt */
static int32 TEGEN_IntHandler(FirqNode *fn)
{
DriverState *ds;
uint32       status;

    TRACE(("TEGEN_IntHandler: entering\n"));

    ds = (DriverState *)fn->firq_Data;

    status = ReadRegister(TE_INTERRUPT_STATUS_ADDR);
    if (status & (TE_INTRSTAT_SUPERVISOR |
                  TE_INTRSTAT_ILLINSTR |
                  TE_INTRSTAT_SPLINSTR))
    {
        INFO(("### Illegal triangle engine command\n"));
        INFO(("### Instruction Read Pointer  0x%08x\n", *(vuint32*)TE_INSTR_READ_PTR_ADDR));
        INFO(("### Aborting IOReq and reinitializing hardware\n"));
        ResetDriver(ds);

        ds->ds_CurrentIOReq->io_Error = TE_ERR_ILLEGALINSTR;
        SuperCompleteIO(ds->ds_CurrentIOReq);
        StartIO(ds, NULL);
    }
    else
    {
        /* FIXME: handle the following events
         *
         *        TE_INTRSTAT_ALUSTAT
         *        TE_INTRSTAT_ZFUNC
         *        TE_INTRSTAT_ANYRENDER
         */
        ClearTEInterrupts(TE_INTRSTAT_ALUSTAT |
                          TE_INTRSTAT_ZFUNC   |
                          TE_INTRSTAT_ANYRENDER);
    }

    return 0;
}


/*****************************************************************************/


static int32 VBL_IntHandler(FirqNode *fn)
{
DriverState *ds;
DeviceState *devState;
IOReq       *ior;

    ds  = (DriverState *)fn->firq_Data;
    ior = ds->ds_CurrentIOReq;

    if (ior)
    {
        if (ior->io_Info.ioi_Command == TE_CMD_EXECUTELIST)
        {
            if (ior->io_Info.ioi_CmdOptions & TE_ABORT_AT_VBLANK)
            {
                devState = DEVICESTATE(ior);
                if (--devState->ds_VBLAbortCount < 0) {
                    ResetDriver(ds);
                    SuperCompleteIO(ds->ds_CurrentIOReq);
                    StartIO(ds, NULL);
                }
            }
        }
    }

    return 0;
}


/*****************************************************************************/


static void OnScreenHandler(struct View *v, DriverState *ds)
{
IOReq       *ior;
DeviceState *devState;
uint32       oldints;

    TRACE(("OnScreenHandler: entering with ds 0x%x\n", ds));

    if (!ds)
        return;

    oldints = Disable();

    ScanList(&ds->ds_DeviceStates, devState, DeviceState)
    {
        if (devState->ds_Control == DEVCTRL_WAIT_DISPLAYED)
        {
            if (devState->ds_View == v)
            {
                while (ior = (IOReq *)RemHead(&devState->ds_Waiters))
                    InsertNodeFromTail(&ds->ds_PendingIOReqs, (Node *)ior);

                devState->ds_Control = DEVCTRL_NORMAL;
            }
        }
    }

    if (ds->ds_CurrentIOReq == NULL)
        StartIO(ds, NULL);

    Enable(oldints);
}


/*****************************************************************************/


static void OffScreenHandler(struct View *v, DriverState *ds)
{
IOReq       *ior;
DeviceState *devState;
uint32       oldints;

    TRACE(("OffScreenHandler: entering with ds 0x%x\n", ds));

    if (!ds)
        return;

    oldints = Disable();

    ScanList(&ds->ds_DeviceStates, devState, DeviceState)
    {
        if (devState->ds_Control == DEVCTRL_WAIT_OFFSCREEN)
        {
            if (devState->ds_View == v)
            {
                while (ior = (IOReq *)RemHead(&devState->ds_Waiters))
                    InsertNodeFromTail(&ds->ds_PendingIOReqs, (Node *)ior);

                devState->ds_Control = DEVCTRL_NORMAL;
            }
        }
    }

    if (ds->ds_CurrentIOReq == NULL)
        StartIO(ds, NULL);

    Enable(oldints);
}


/*****************************************************************************/


static void BitmapHandler(Item it, DriverState *ds)
{
DeviceState *devState;
int32        oldints;
Bitmap      *bitmap;

    TRACE(("BitmapHandler: entering with it 0x%x, ds 0x%x\n", it, ds));

    oldints = Disable();

    bitmap = (Bitmap *)LookupItem(it);
    ScanList(&ds->ds_DeviceStates, devState, DeviceState)
    {
        if (devState->ds_FrameBuffer == bitmap)
            devState->ds_FrameBuffer = NULL;

        if (devState->ds_ZBuffer == bitmap)
            devState->ds_ZBuffer = NULL;
    }

    Enable(oldints);
}


/*****************************************************************************/


static int32 TimeoutHandler(Timer *timer)
{
DriverState *ds;
DeviceState *devState;

    TRACE(("TimeoutHandler: entering\n"));

    ds = timer->tm_UserData;
    if (ds->ds_CurrentIOReq)
    {
        devState = DEVICESTATE(ds->ds_CurrentIOReq);
        if (devState->ds_Control == DEVCTRL_NORMAL)
        {
            INFO(("### Triangle engine timeout\n"));
            INFO(("### Instruction Read Pointer  0x%08x\n", *(vuint32*)TE_INSTR_READ_PTR_ADDR));
            INFO(("### Aborting IOReq and reinitializing hardware\n"));
            ResetDriver(ds);

            ds->ds_CurrentIOReq->io_Error = TE_ERR_TIMEOUT;
            SuperCompleteIO(ds->ds_CurrentIOReq);
            StartIO(ds, NULL);
        }
    }

    return 0;
}


/*****************************************************************************/


static int32 SpeedBumpHandler(Timer *timer)
{
DriverState *ds;
DeviceState *devState;

    TRACE(("SpeedBumpHandler: entering\n"));

    ds       = timer->tm_UserData;
    devState = DEVICESTATE(ds->ds_CurrentIOReq);

    if (devState->ds_Control == DEVCTRL_SPEEDBUMP)
    {
        if (!devState->ds_FullStop)
            (* ds->ds_SpeedBumpTimer->tm_Load)(ds->ds_SpeedBumpTimer, &devState->ds_SpeedBumpTime);

        devState->ds_PauseInstruction = TEPaused();

        WriteRegister(TE_IMMINSTR_CNTRL_ADDR, TE_IMMINSTR_STEP |
                                              TE_IMMINSTR_CONTINUE |
                                              TE_IMMINSTR_INTERRUPT);
    }

    return 0;
}


/*****************************************************************************/


static int32 DispatchTEIO(IOReq *ior)
{
uint32       oldints;
DriverState *ds;

    ds = DRIVERSTATE(ior);

    ior->io_Flags &= ~IO_QUICK;

    oldints = Disable();

    if (ds->ds_CurrentIOReq == NULL)
        StartIO(ds, ior);
    else
        InsertNodeFromTail(&ds->ds_PendingIOReqs, (Node *)ior);

    Enable(oldints);

    return 0;
}


/*****************************************************************************/


static int32 CmdExecuteList(IOReq *ior)
{
    TRACE(("CmdExecuteList: entering with ior 0x%x\n", ior));

    if (ior->io_Info.ioi_Send.iob_Buffer == NULL)
    {
        ior->io_Error = TE_ERR_NOLIST;
        return 1;
    }

    if (ior->io_Info.ioi_CmdOptions & ~(TE_CLEAR_FRAME_BUFFER |
                                        TE_CLEAR_Z_BUFFER |
                                        TE_WAIT_UNTIL_OFFSCREEN |
                                        TE_ABORT_AT_VBLANK |
                                        TE_LIST_FLUSHED))
    {
        ior->io_Error = TE_ERR_BADOPTIONS;
        return 1;
    }

    if ((ior->io_Info.ioi_CmdOptions & TE_LIST_FLUSHED) == 0)
    {
        /* get the command-list, textures, PIPs, and anything else to memory */
        FlushDCacheAll(0);
    }

    return DispatchTEIO(ior);
}


/*****************************************************************************/


static int32
CmdSetView (
IOReq	*ior
)
{
	View	*v;
	Item	vi;

	vi = (Item) ior->io_Info.ioi_Offset;
	if (vi >= 0) {
		if (!(v = (View *) CheckItem
				    (vi, NST_GRAPHICS, GFX_VIEW_NODE)))
		{
			ior->io_Error = TE_ERR_BADITEM;
			return (1);
		}
	} else
		v = NULL;

	ior->io_Info.ioi_Offset = (uint32) v;

	return (DispatchTEIO (ior));
}


/*****************************************************************************/


static int32 CmdSetFrameBuffer(IOReq *ior)
{
Bitmap *bitmap;
Item    frameBuffer;

    TRACE(("CmdSetFrameBuffer: entering with ior 0x%x\n", ior));

    frameBuffer = (Item)ior->io_Info.ioi_Offset;
    if (frameBuffer != -1)
    {
	bitmap = (Bitmap *)CheckItem(frameBuffer, NST_GRAPHICS, GFX_BITMAP_NODE);
	if (bitmap == NULL)
        {
            ior->io_Error = TE_ERR_BADITEM;
            return 1;
        }

        if (((bitmap->bm_Type != BMTYPE_16)
          && (bitmap->bm_Type != BMTYPE_16_ZBUFFER)
          && (bitmap->bm_Type != BMTYPE_32))
         || (bitmap->bm_Buffer == NULL)
         || (bitmap->bm_BufferSize == 0)
         || ((bitmap->bm_Flags & BMF_RENDERABLE) == 0))
        {
            ior->io_Error = TE_ERR_BADBITMAP;
            return 1;
        }

        /* must convert to FP format here and not from int handler */
        TEIO(ior)->teio_Width  = (float32)bitmap->bm_Width;
        TEIO(ior)->teio_Height = (float32)bitmap->bm_Height;
    }
    else
    {
        bitmap = NULL;
    }
    ior->io_Info.ioi_Offset = (uint32)bitmap;

    return DispatchTEIO(ior);
}


/*****************************************************************************/


static int32 CmdSetZBuffer(IOReq *ior)
{
Bitmap *bitmap;
Item    zBuffer;

    TRACE(("CmdSetZBuffer: entering with ior 0x%x\n", ior));

    zBuffer = (Item)ior->io_Info.ioi_Offset;
    if (zBuffer != -1)
    {
	bitmap = (Bitmap *)CheckItem(zBuffer, NST_GRAPHICS, GFX_BITMAP_NODE);
	if (bitmap == NULL)
        {
            ior->io_Error = TE_ERR_BADITEM;
            return 1;
        }

        if ((bitmap->bm_Type != BMTYPE_16_ZBUFFER)
         || (bitmap->bm_Buffer == NULL)
         || (bitmap->bm_BufferSize == 0)
         || ((bitmap->bm_Flags & BMF_RENDERABLE) == 0))
        {
            ior->io_Error = TE_ERR_BADBITMAP;
            return 1;
        }

        /* must convert to FP format here and not from int handler */
        TEIO(ior)->teio_Width  = (float32)bitmap->bm_Width;
        TEIO(ior)->teio_Height = (float32)bitmap->bm_Height;
    }
    else
    {
        bitmap = NULL;
    }
    ior->io_Info.ioi_Offset = (uint32)bitmap;

    return DispatchTEIO(ior);
}


/*****************************************************************************/


static int32 CmdDisplayFrameBuffer(IOReq *ior)
{
    TRACE(("CmdDisplayFrameBuffer: entering with ior 0x%x\n", ior));
    return DispatchTEIO(ior);
}


/*****************************************************************************/

/* A SpeedBump value of 3 or less will hang the system (Reason TBD),
 * so we'll set a lowest bound of 5 here for good measure.
 */
#define SMALLEST_SPEEDBUMP 5

static int32 CmdSpeedControl(IOReq *ior)
{
DriverState *ds;
DeviceState *devState;
TimeVal      tv;
uint32       oldints;
uint32 speedBump;

    TRACE(("CmdSpeedControl: entering with ior 0x%x\n", ior));

    ds       = DRIVERSTATE(ior);
    devState = DEVICESTATE(ior);

    oldints = Disable();
    if (ior->io_Info.ioi_CmdOptions == 0)
    {
        if (devState == ds->ds_Device)
        {
            (* ds->ds_SpeedBumpTimer->tm_Unload)(ds->ds_SpeedBumpTimer);

            if (devState->ds_Control == DEVCTRL_SPEEDBUMP)
            {
                devState->ds_Control = DEVCTRL_NORMAL;
                WriteRegister(TE_IMMINSTR_CNTRL_ADDR, TE_IMMINSTR_CONTINUE |
                                                      TE_IMMINSTR_INTERRUPT);
            }
        }
    }
    else
    {
        speedBump = ior->io_Info.ioi_CmdOptions;
        if (speedBump < SMALLEST_SPEEDBUMP)
        {
            speedBump = SMALLEST_SPEEDBUMP;
        }

        if (speedBump == 0xffffffff)
        {
            devState->ds_FullStop = TRUE;
        }
        else
        {
            tv.tv_Seconds      = 0;
            tv.tv_Microseconds = speedBump;
            ConvertTimeValToTimerTicks(&tv, &devState->ds_SpeedBumpTime);
        }

        if (devState->ds_Control == DEVCTRL_NORMAL)
        {
                /* Is this device the one that is currently using the TE, and
                 * is it executing anything?
                 */
            if ((devState->ds_DriverState->ds_Device == devState) &&
                (devState->ds_DriverState->ds_CurrentIOReq))
            {
                    /* If the TE is executing a list, then start the timer and stop
                     * the current instruction.
                     * When the time expires, the SpeedBumpHandler() will try
                     * to execute the next instruction in the list. Therefore, we don't
                     * want to start the timer now if there are no more TE instructions
                     * to execute else the TE will execute a garbage instruction.
                     */
                devState->ds_FullStop = FALSE;
                (* ds->ds_SpeedBumpTimer->tm_Load)(ds->ds_SpeedBumpTimer, &devState->ds_SpeedBumpTime);
                    /* stop the TE */
                WriteRegister(TE_IMMINSTR_CNTRL_ADDR, TE_IMMINSTR_STOP_CRNTINSTR |
                              TE_IMMINSTR_INTERRUPT);
            }
        }
        devState->ds_Control = DEVCTRL_SPEEDBUMP;
    }
    Enable(oldints);

    return 1;
}


/*****************************************************************************/


static int32 CmdStep(IOReq *ior)
{
DriverState *ds;
DeviceState *devState;
uint32       oldints;

    TRACE(("CmdStep: entering with ior 0x%x\n", ior));

    ds       = DRIVERSTATE(ior);
    devState = DEVICESTATE(ior);

    if (devState->ds_Control != DEVCTRL_SPEEDBUMP)
    {
        ior->io_Error = TE_ERR_NOTPAUSED;
        return 1;
    }

    oldints = Disable();

    ior->io_Flags           &= ~IO_QUICK;
    devState->ds_SteppingIO  = ior;
    devState->ds_Control     = DEVCTRL_STEPPING;

    if (devState == ds->ds_Device)
    {
        /* make it go one instruction */
        WriteRegister(TE_IMMINSTR_CNTRL_ADDR, TE_IMMINSTR_STEP |
                                              TE_IMMINSTR_CONTINUE |
                                              TE_IMMINSTR_INTERRUPT);
    }

    Enable(oldints);

    return 0;
}


/*****************************************************************************/


static int32
CmdSetVBLAbortCount (IOReq *ior)
{
	if (ior->io_Info.ioi_Offset < 0) {
		ior->io_Error = TE_ERR_BADIOARG;
		return (1);
	}
	return (DispatchTEIO (ior));
}


/*****************************************************************************/


#ifdef BUILD_DEBUGGER
static int32 CmdGetIdleTime(IOReq *ior)
{
uint32       oldints;
TimerTicks  *tt;
TimerTicks   endTT;
DriverState *ds;

    TRACE(("CmdGetIdleTime: entering with ior 0x%x\n", ior));

    if (ior->io_Info.ioi_Recv.iob_Len != sizeof(TimerTicks))
        return BADIOARG;

    ds = DRIVERSTATE(ior);

    tt = ior->io_Info.ioi_Recv.iob_Buffer;
    oldints = Disable();
    *tt = ds->ds_TotalIdleTime;

    if (ds->ds_Idling)
    {
        SampleSystemTimeTT(&endTT);
        SubTimerTicks(&ds->ds_IdleStartTime, &endTT, &endTT);
        AddTimerTicks(tt, &endTT, tt);
    }

    Enable(oldints);

    return 1;
}
#endif


/*****************************************************************************/


static Item CreateTEIO(IOReq *ior)
{
DriverState *ds;

    TRACE(("CreateTEIO: entering with ior %x\n", ior));

    ds = &driverState;

    ior->io_Extension[0] = (uint32)ior->io_Dev->dev_DriverData;
    ior->io_Extension[1] = (uint32)ds;
    return ior->io.n_Item;
}


/*****************************************************************************/


static Err CreateTEDevice(Device *dev)
{
DriverState *ds;
DeviceState *devState;
DeviceState *otherDevState;
Err          result;
uint32       oldints;

    TRACE(("CreateTEDevice: entering, task %s\n", CURRENTTASK->t.n_Name));

    ds       = &driverState;
    devState = (DeviceState *)dev->dev_DriverData;

    PrepList(&devState->ds_Waiters);
    PrepList(&devState->ds_DisplayWaiters);
    devState->ds_Control                = DEVCTRL_NORMAL;
    devState->ds_DriverState            = ds;
    devState->ds_FBInfo.tefbi_Red       = 0.0;
    devState->ds_FBInfo.tefbi_Green     = 0.0;
    devState->ds_FBInfo.tefbi_Blue      = 0.0;
    devState->ds_FBInfo.tefbi_Alpha     = 0.0;
    devState->ds_ZBInfo.tezbi_FillValue = 0.0;
    devState->ds_ClearFBCmdList         = clearFBCmdList;
    devState->ds_ClearZBCmdList         = clearZBCmdList;

    ds->ds_NumDevices++;
    if (ds->ds_NumDevices > 1)
    {
        /* if there's more than 1 device active, all DeviceState structures
         * must have suitable TE state save areas allocated for them.
         */

        result = AllocTEContext(devState);
        if (result < 0)
            return result;

        if (ds->ds_NumDevices == 2)
        {
            /* since there was previously only one device active, that device
             * didn't have a context save area. So we must now allocate one for
             * it.
             */
            otherDevState = (DeviceState *)FirstNode(&ds->ds_DeviceStates);

            result = AllocTEContext(otherDevState);
            if (result < 0)
            {
                FreeTEContext(devState);
                return result;
            }
        }
    }

    oldints = Disable();
    AddTail(&ds->ds_DeviceStates, (Node *)devState);
    Enable(oldints);

    return dev->dev.n_Item;
}


/*****************************************************************************/


static Err DeleteTEDevice(Device *dev)
{
DriverState *ds;
DeviceState *devState;
DeviceState *otherDevState;
uint32       oldints;

    TRACE(("DeleteTEDevice: entering\n"));

    ds       = &driverState;
    devState = (DeviceState *)dev->dev_DriverData;

    oldints = Disable();

    /* remove from the driver's list */
    RemNode((Node *)devState);

    /* if this device is currently loaded in the HW, remove it from there */
    if (ds->ds_Device == devState)
        ResetDriver(ds);

    ds->ds_NumDevices--;
    if (ds->ds_NumDevices == 1)
    {
        otherDevState = (DeviceState *)FirstNode(&ds->ds_DeviceStates);

        /* if there's only one device left, nuke its state save
         * area since it'll never get used anymore.
         */
        if (ds->ds_Device == NULL)
        {
            /* since we're about to nuke this state save area, we must load
             * it in the HW so we won't lose the state.
             */
            RestoreTEContext(otherDevState);
            ds->ds_Device = otherDevState;
        }
    }
    else
    {
        otherDevState = NULL;
    }

    Enable(oldints);

    if (otherDevState)
        FreeTEContext(otherDevState);

    FreeTEContext(devState);

    return 0;
}


/*****************************************************************************/


static Item CreateTEFIRQ(DriverState *ds, int32 (*func)(FirqNode *fn), uint32 intNum)
{
    return CreateItemVA(MKNODEID(KERNELNODE, FIRQNODE),
                        TAG_ITEM_NAME,       "TE",
                        CREATEFIRQ_TAG_NUM,  intNum,
                        CREATEFIRQ_TAG_CODE, func,
                        CREATEFIRQ_TAG_DATA, ds,
                        TAG_END);
}


/*****************************************************************************/


static Item CreateTEDriver(Driver *drv)
{
int32        oldints;
TimeVal      tv;
Err          result;
DriverState *ds;
Item         timer;

    TRACE(("CreateTEDriver: entering\n"));

    ds = &driverState;

    memset(ds, 0, sizeof(DriverState));

    oldints = Disable();
    ResetDriver(ds);
    Enable(oldints);

    tv.tv_Seconds      = 1;
    tv.tv_Microseconds = 0;
    ConvertTimeValToTimerTicks(&tv, &ds->ds_TimeoutTime);

    timer = result = CreateItemVA(MKNODEID(KERNELNODE, TIMERNODE),
	                          CREATETIMER_TAG_HNDLR,    TimeoutHandler,
                                  CREATETIMER_TAG_USERDATA, ds,
                                  TAG_END);
    if (timer >= 0)
    {
        ds->ds_TimeoutTimer = (Timer *)LookupItem(timer);

        timer = result = CreateItemVA(MKNODEID(KERNELNODE, TIMERNODE),
                                      CREATETIMER_TAG_HNDLR,    SpeedBumpHandler,
                                      CREATETIMER_TAG_USERDATA, ds,
                                      TAG_END);
        if (timer >= 0)
        {
            ds->ds_SpeedBumpTimer = (Timer *)LookupItem(timer);

            ds->ds_FirqCLIP = result = CreateTEFIRQ(ds, CLIP_IntHandler, INT_BDA_CLIP);
            if (ds->ds_FirqCLIP >= 0)
            {
                ds->ds_FirqIMINST = result = CreateTEFIRQ(ds, IMINST_IntHandler, INT_BDA_IMINST);
                if (ds->ds_FirqIMINST >= 0)
                {
                    ds->ds_FirqDFINST = result = CreateTEFIRQ(ds, DFINST_IntHandler, INT_BDA_DFINST);
                    if (ds->ds_FirqDFINST >= 0)
                    {
                        ds->ds_FirqTEGEN = result = CreateTEFIRQ(ds, TEGEN_IntHandler, INT_BDA_TEGEN);
                        if (ds->ds_FirqTEGEN >= 0)
                        {
                            ds->ds_FirqVBL = result = CreateTEFIRQ(ds, VBL_IntHandler, INT_BDA_V1);
                            if (ds->ds_FirqVBL >= 0)
                            {
                                ds->ds_Driver = drv;
                                PrepList(&ds->ds_PendingIOReqs);
                                PrepList(&ds->ds_DeviceStates);

                                result = SuperSetBitmapCallBack(BitmapHandler, ds);
                                if (result >= 0)
                                {
#if 0
                                    EnableInterrupt(INT_BDA_CLIP);
#endif
                                    EnableInterrupt(INT_BDA_IMINST);
                                    EnableInterrupt(INT_BDA_DFINST);
                                    EnableInterrupt(INT_BDA_TEGEN);
                                    EnableInterrupt(INT_BDA_V1);

                                    return drv->drv.n_Item;
                                }
                                DeleteFIRQ(ds->ds_FirqVBL);
                            }
                            DeleteFIRQ(ds->ds_FirqTEGEN);
                        }
                        DeleteFIRQ(ds->ds_FirqDFINST);
                    }
                    DeleteFIRQ(ds->ds_FirqIMINST);
                }
                DeleteFIRQ(ds->ds_FirqCLIP);
            }
            DeleteItem(ds->ds_SpeedBumpTimer->tm.n_Item);
        }
        DeleteItem(ds->ds_TimeoutTimer->tm.n_Item);
    }

    return result;
}


/*****************************************************************************/


static Err DeleteTEDriver(Driver *drv)
{
DriverState *ds;

    TRACE(("DeleteTEDriver: entering\n"));

    TOUCH(drv);

    ds = &driverState;

    SuperSetBitmapCallBack(NULL, NULL);
    DeleteItem(ds->ds_TimeoutTimer->tm.n_Item);
    DeleteItem(ds->ds_SpeedBumpTimer->tm.n_Item);
    DeleteFIRQ(ds->ds_FirqCLIP);
    DeleteFIRQ(ds->ds_FirqIMINST);
    DeleteFIRQ(ds->ds_FirqDFINST);
    DeleteFIRQ(ds->ds_FirqTEGEN);
    DeleteFIRQ(ds->ds_FirqVBL);

    return 0;
}


/*****************************************************************************/


static Err ChangeOwnerTEDriver(Driver *drv, Item newOwner)
{
DriverState *ds;

    TRACE(("ChangeOwnerTEDriver: entering\n"));

    TOUCH(drv);

    ds = &driverState;

    SetItemOwner(ds->ds_TimeoutTimer->tm.n_Item,  newOwner);
    SetItemOwner(ds->ds_SpeedBumpTimer->tm.n_Item, newOwner);
    SetItemOwner(ds->ds_FirqCLIP,   newOwner);
    SetItemOwner(ds->ds_FirqIMINST, newOwner);
    SetItemOwner(ds->ds_FirqDFINST, newOwner);
    SetItemOwner(ds->ds_FirqTEGEN,  newOwner);
    SetItemOwner(ds->ds_FirqVBL,    newOwner);
    return 0;
}


/*****************************************************************************/


static const DriverCmdTable cmdTable[] =
{
    {TE_CMD_EXECUTELIST,        CmdExecuteList},
    {TE_CMD_SETVIEW,            CmdSetView},
    {TE_CMD_SETZBUFFER,         CmdSetZBuffer},
    {TE_CMD_SETFRAMEBUFFER,     CmdSetFrameBuffer},
    {TE_CMD_DISPLAYFRAMEBUFFER, CmdDisplayFrameBuffer},
#ifdef BUILD_DEBUGGER
    {TE_CMD_GETIDLETIME,        CmdGetIdleTime},
#endif
    {TE_CMD_STEP,               CmdStep},
    {TE_CMD_SETVBLABORTCOUNT,	CmdSetVBLAbortCount},
    {TE_CMD_SPEEDCONTROL,       CmdSpeedControl},
};


int main(void)
{
    return CreateItemVA(MKNODEID (KERNELNODE, DRIVERNODE),
		TAG_ITEM_NAME,		         "triangleengine",
		CREATEDRIVER_TAG_CMDTABLE,       cmdTable,
		CREATEDRIVER_TAG_NUMCMDS,        sizeof(cmdTable) / sizeof(cmdTable[0]),
		CREATEDRIVER_TAG_ABORTIO,        AbortTEIO,
		CREATEDRIVER_TAG_CRIO,           CreateTEIO,
		CREATEDRIVER_TAG_IOREQSIZE,      sizeof(TEIOReq),
		CREATEDRIVER_TAG_CREATEDRV,      CreateTEDriver,
		CREATEDRIVER_TAG_DELETEDRV,      DeleteTEDriver,
		CREATEDRIVER_TAG_CHOWN_DRV,      ChangeOwnerTEDriver,
		CREATEDRIVER_TAG_CREATEDEV,      CreateTEDevice,
		CREATEDRIVER_TAG_DELETEDEV,      DeleteTEDevice,
		CREATEDRIVER_TAG_DEVICEDATASIZE, sizeof(DeviceState),
		CREATEDRIVER_TAG_MODULE,         FindCurrentModule(),
		TAG_END);
}
