#ifndef __GRAPHICS_CLT_GSTATE_H
#define __GRAPHICS_CLT_GSTATE_H


/******************************************************************************
**
**  @(#) gstate.h 96/10/03 1.44
**
**  Definitions and declarations for the Graphics State
**
******************************************************************************/


#ifndef __GRAPHICS_GRAPHICS_H
#include <graphics/graphics.h>
#endif

#ifndef __GRAPHICS_BITMAP_H
#include <graphics/bitmap.h>
#endif


/*****************************************************************************/

#ifndef EXTERNAL_RELEASE
#ifndef __DEVICE_TE_H
#include <device/te.h>
#endif

typedef struct
{
    Item  cl_IOReq;
    void *cl_CmdBuffer;
} GSCmdList;
#endif

typedef void (*GSProfileFunc)(struct GState*, int32 arg) ;

typedef struct GState
{
    Err      		   (*gs_SendList)(struct GState*);
    CmdListP   		   gs_EndList;     /* End of current list */
    CmdListP   		   gs_ListPtr;     /* Next insert position w/in cur list */

#ifndef EXTERNAL_RELEASE
#ifdef GSTATE_PRIVATE

    /* - - - Elements below this line MUST be accessed via functions - - - */
    /* Low Latency List Management keep this w/in first cache line */
    uint32                 *gs_RealEndList;
    uint32                 *gs_LastEndList;
    uint32                  gs_Latency;

    /* Cmd list management */
    uint32                  gs_NumberList;
    uint32                  gs_WhichList;
    GSCmdList              *gs_CmdLists;
    uint32                  gs_ListSize;
    uint16                  gs_DCacheSize;
    uint16                  gs_DCacheLineSize;

    /* Rendering and display buffer management */
    int32                   gs_VidSignal;

    Item                    gs_ZBuffer;
    Item                    gs_DestBuffer;
    Item                    gs_View;
    Item                    gs_TEDev;
    Item                    gs_SetBufferIO;
    Item                    gs_DisplayFrameIO;
    /* TE Speed and stepping control */
    uint32                  gs_Speed;
    Item                    gs_SpeedIO;
    TEFrameBufferInfo       gs_FBInfo;
    TEZBufferInfo           gs_ZBInfo;
    uint32                  gs_Count;

    /* GState state machine indicated by flag values */
    uint32                  gs_Flags;

    /* Items used in profile checking */
    Item 		    gs_Semaphore; 	/* must be locked before changing gs_Curcount*/
    int32 		    gs_CurCount; 	/* the number of io requests currently queued up*/
    Item 		    gs_Thread;
    Item 		    gs_MessagePort;
    GSProfileFunc	    gs_ProfileFunc;

#endif
#endif
} GState;

/* Gets the pointer to the current list insertion point suitable
 * for calls to CLT
 */
#define GS_Ptr(g) (& ((g)->gs_ListPtr))


/*****************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif


GState  *GS_Create(void);
GState  *GS_Clone(GState *gs);
Err      GS_Delete(GState *gs);
Err      GS_AllocLists(GState *gs, uint32 numLists, uint32 wordsPerList);
void     GS_SetList(GState *gs, uint32 idx);
Err      GS_SendList(GState *gs);
void     GS_Reserve(GState *gs, uint32 numWords);
Err      GS_SetView(GState *gs, Item viewItem);
Err      GS_SetDestBuffer(GState *gs, Item bmItem);
Err      GS_SetZBuffer(GState *gs, Item bmItem);
Item     GS_GetView(GState *gs);
Item     GS_GetDestBuffer(GState *gs);
Item     GS_GetZBuffer(GState *gs);
Err      GS_BeginFrame(GState *gs);
Err      GS_EndFrame(GState *gs);
Err      GS_SetVidSignal(GState *gs, int32 signal);
int32    GS_GetVidSignal(GState *gs);
Err      GS_WaitIO(GState *gs);
Err      GS_FreeLists(GState *gs);
uint32   GS_GetCmdListIndex(GState *gs);
CmdListP GS_GetCurListStart(GState *gs);
Err 	 GS_SetTESpeed(GState *gs, uint32 speed);
Err 	 GS_SingleStep(GState *gs);
Err 	 GS_ClearFBZ(GState *gs, bool clearfb, bool clearz);
Err 	 GS_SetClearValues(GState *gs, float r, float g, float b, float a, float z);
Err 	 GS_SetAbortVblank(GState *gs, bool abort);
Err 	 GS_SetAbortVblankCount(GState *gs, int32 count);
Err	 GS_SendLastList(GState *gs);
Err 	 GS_LowLatency(GState *gs, bool send, uint32 latency);
bool 	 GS_IsLowLatency(GState *gs);
Err 	 GS_SendIO(GState *gs, bool wait, uint32 len);
Err 	 GS_EnableProfiling(GState *g, GSProfileFunc proc);
Err 	 GS_DisableProfiling(GState *g);

/* Bitmap utilities */
Err     GS_AllocBitmaps(Item bitmaps[], uint32 xres, uint32 yres, uint32 bmType, uint32 numFrameBufs, bool useZb);
Err     GS_FreeBitmaps(Item bitmaps[], uint32 numBitmaps);

#ifndef EXTERNAL_RELEASE
uint32 GS_GetCount(GState *gs);
#endif


#ifdef __cplusplus
}
#endif


/*****************************************************************************/


#endif /* __GRAPHICS_CLT_GSTATE_H */
