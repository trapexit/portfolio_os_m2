#ifndef __DRIVER_H
#define __DRIVER_H


/******************************************************************************
**
**  @(#) driver.h 96/10/02 1.6
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_TIMER_H
#include <kernel/timer.h>
#endif

#ifndef __GRAPHICS_BITMAP_H
#include <graphics/bitmap.h>
#endif

#ifndef __GRAPHICS_VIEW_H
#include <graphics/view.h>
#endif


/*****************************************************************************/


typedef struct
{
    uint32 v_X;
    uint32 v_Y;
    uint32 v_Red;
    uint32 v_Green;
    uint32 v_Blue;
    uint32 v_Alpha;
} VertexRGBAW;

typedef struct
{
    uint32      cl_Commands0[15];
    VertexRGBAW cl_TopLeft;
    VertexRGBAW cl_BottomLeft;
    VertexRGBAW cl_TopRight;
    VertexRGBAW cl_BottomRight;
    uint32      cl_Commands1[3];
    uint32      cl_JumpAddr;
    uint32      cl_Commands2[1];
} ClearFBCmdList;

typedef struct
{
    uint32      cl_Commands0[15];
    VertexRGBAW cl_TopLeft;
    VertexRGBAW cl_TopRight;
    VertexRGBAW cl_BottomLeft;
    VertexRGBAW cl_BottomRight;
    uint32      cl_Commands1[4];
} ClearZBCmdList;


/*****************************************************************************/


typedef struct DeviceState DeviceState;

/* a single one of these for global state info */
typedef struct
{
    Driver      *ds_Driver;
    IOReq       *ds_CurrentIOReq;
    DeviceState *ds_Device;
    Bitmap      *ds_FrameBuffer;
    Bitmap      *ds_ZBuffer;
    List         ds_PendingIOReqs;

    List         ds_DeviceStates;
    uint32       ds_NumDevices;

    TimerTicks   ds_TimeoutTime;
    Timer       *ds_TimeoutTimer;
    Timer       *ds_SpeedBumpTimer;

#ifdef BUILD_DEBUGGER
    TimerTicks   ds_IdleStartTime;
    TimerTicks   ds_TotalIdleTime;
    bool         ds_Idling;
#endif

    Item         ds_FirqVBL;
    Item         ds_FirqCLIP;
    Item         ds_FirqIMINST;
    Item         ds_FirqDFINST;
    Item         ds_FirqTEGEN;
} DriverState;

#define DRIVERSTATE(ior) ((DriverState *)ior->io_Extension[1])


/*****************************************************************************/


typedef enum
{
    DEVCTRL_NORMAL,
    DEVCTRL_WAIT_OFFSCREEN,
    DEVCTRL_WAIT_DISPLAYED,
    DEVCTRL_STEPPING,
    DEVCTRL_SPEEDBUMP
} DeviceControl;

/* one of these for each TE Device structure */
typedef struct DeviceState
{
    MinNode           ds;
    DriverState      *ds_DriverState;

    Bitmap           *ds_FrameBuffer;
    Bitmap           *ds_ZBuffer;
    View             *ds_View;
    TEFrameBufferInfo ds_FBInfo;
    TEZBufferInfo     ds_ZBInfo;

    DeviceControl     ds_Control;
    List              ds_Waiters;         /* all types of ioreqs */
    List              ds_DisplayWaiters;  /* TE_CMD_DISPLAYFRAMEBUFFER ioreqs */
    TimerTicks        ds_SpeedBumpTime;
    bool              ds_FullStop;
    bool              ds_PauseInstruction;
    IOReq            *ds_SteppingIO;
    int16             ds_VBLAbortCount;
    int16             ds_VBLAbortCountStart;

    ClearFBCmdList    ds_ClearFBCmdList;
    ClearZBCmdList    ds_ClearZBCmdList;

    /* places where to save state */
    struct TEContext *ds_TEContext;
};

#define DEVICESTATE(ior) ((DeviceState *)ior->io_Extension[0])


/*****************************************************************************/


/* the extended IOReq used by this driver */
typedef struct
{
    IOReq   teio;

    /* used for SetFrameBuffer and SetZBuffer. This is because the conversion
     * to float must be done within the dispatch function and not in the
     * interrupt handler.
     */
    float32 teio_Width;
    float32 teio_Height;
} TEIOReq;

#define TEIO(ior) ((TEIOReq *)ior)


/*****************************************************************************/


#endif /* __DRIVER_H */
