/* @(#) gstate.c 96/10/04 1.79 */

#define GSTATE_PRIVATE

#include <kernel/types.h>
#include <graphics/clt/clt.h>
#include <graphics/clt/gstate.h>
#include <graphics/bitmap.h>
#include <graphics/view.h>
#include <device/te.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <kernel/mem.h>
#include <kernel/operror.h>
#include <kernel/cache.h>
#include <hardware/te.h>
#include <device/mp.h>
#include <kernel/semaphore.h>
#include <kernel/task.h>

#define DBUGZ(x) /* printf x */

#define DEBUG 		0

/* One memory block (in bytes) that M2 needs to properly
   sync because of read ahead */
#define SLOP				32


/* used for either a pause or a jump plus the extra memory
   block (32 bytes) for synching w/ M2 vertex unit */

#define kListSizeExtraPadding	(12+SLOP)

#define GS_ERRID		MakeErrId('G','S')
#define GS_OUT_OF_MEMORY	MakeErr(ER_FOLI,GS_ERRID,ER_SEVERE,\
					ER_E_APPL,ER_C_STND,ER_NoMem)


#define GS_BEGIN_FLAG        0x00000001
#define GS_CLEARFB_FLAG      0x00000002
#define GS_CLEARZB_FLAG      0x00000004
#define GS_ABORT_VBLANK_FLAG 0x00000008
#define GS_USE_LOW_LATENCY   0x00000010
#define GS_LL_START          0x00000020
#define GS_LL_IO_SENT        0x00000040
#define GS_CLONED            0x00000080


#define ByteAdvance(ptr, bytes)  *(char**)&ptr += bytes

/* Thread run for messages associated w/ sendio */

static void GS_ThreadProc(GState *gs)
{
    int newtecount;
    int result;

    if ((gs->gs_MessagePort=CreateMsgPort(NULL, 0, 0))<0) {
#ifdef BUILD_STRINGS
	printf("<GS_ThreadProc> ERROR in TE Profiling. Unable to create message port.\n");
#endif
	exit(-1);
    }

    while (1) {

	result=WaitPort(gs->gs_MessagePort, 0);


	if (result<0) {
#ifdef BUILD_STRINGS
	    printf("<GS_ThreadProc> ERROR in waitport\n");
	    PrintfSysErr(result);
#endif
	    exit(-1);
	}

	if (LockSemaphore(gs->gs_Semaphore, SEM_WAIT)<0) {
#ifdef BUILD_STRINGS
	    printf("<GS_ThreadProc> ERROR in locksemaphore\n");
#endif
	    exit(-1);
	}

	newtecount=--gs->gs_CurCount;

	if (UnlockSemaphore(gs->gs_Semaphore)<0) {
#ifdef BUILD_STRINGS
	    printf("<GS_ThreadProc> ERROR in unlocksemaphore\n");
#endif
	    exit(-1);
	}

	if (newtecount==0 && gs->gs_ProfileFunc) gs->gs_ProfileFunc(gs, -1);
    }
}

/*****************************************************************************/


static Err GS_BustedSendList(GState* g)
{
    TOUCH(g);
    return -1;
}


/*****************************************************************************/


static void GS_Init(GState *gs)
{
    gs->gs_SendList       = GS_BustedSendList;
    gs->gs_DestBuffer     = -1;
    gs->gs_ZBuffer        = -1;
    gs->gs_Speed          = TE_FULLSPEED;
    gs->gs_SetBufferIO    = -1;
    gs->gs_DisplayFrameIO = -1;
    gs->gs_SpeedIO        = -1;
    gs->gs_Semaphore	  = 0;
    gs->gs_Thread	  = 0;
    gs->gs_ProfileFunc	  = 0;
    gs->gs_CurCount 	  = 0;

    GS_SetClearValues(gs, 0.0, 0.0, 0.0, 0.0, 0.0);

#if DEBUG1
    printf("Master CPU LL stats at %x \n",&ll[0][0]);
    printf("Slave CPU LL stats at %x \n",&ll[1][0]);
#endif
}


/*****************************************************************************/


GState *GS_Create(void)
{
GState    *gs;
CacheInfo  ci;

    GetCacheInfo(&ci, sizeof(ci));

    gs = AllocMemAligned(sizeof(GState), MEMTYPE_NORMAL | MEMTYPE_FILL, ci.cinfo_DCacheLineSize);
    if (gs)
    {
        GS_Init(gs);
        gs->gs_DCacheSize     = ci.cinfo_DCacheSize;
        gs->gs_DCacheLineSize = ci.cinfo_DCacheLineSize;

        gs->gs_TEDev = OpenTEDevice();
        if (gs->gs_TEDev < 0)
        {
            FreeMem(gs, sizeof(GState));
            gs = NULL;
        }
    }

    return gs;
}


/*****************************************************************************/


Err GS_Delete(GState *gs)
{
    if (gs)
    {
	GS_FreeLists(gs);

	if (gs->gs_Flags & GS_CLONED) {
	    CloseItem(gs->gs_TEDev);
	} else {
	    GS_DisableProfiling(gs);
            CloseTEDevice(gs->gs_TEDev);
	}
	FreeMem(gs, sizeof(GState));
    }
    return 0;
}


/*****************************************************************************/


GState *GS_Clone(GState *gsrc)
{
GState *gs;
Err     result;

    gs = AllocMemAligned(sizeof(GState), MEMTYPE_NORMAL | MEMTYPE_FILL, gsrc->gs_DCacheLineSize);
    if (gs)
    {
        GS_Init(gs);

        gs->gs_DCacheLineSize = gsrc->gs_DCacheLineSize;
        gs->gs_DestBuffer     = gsrc->gs_DestBuffer;
        gs->gs_ZBuffer        = gsrc->gs_ZBuffer;
        gs->gs_SendList       = gsrc->gs_SendList;
        gs->gs_VidSignal      = gsrc->gs_VidSignal;
        gs->gs_Flags          = gsrc->gs_Flags | GS_CLONED;

        gs->gs_TEDev = OpenItem(gsrc->gs_TEDev, NULL);
        if (gs->gs_TEDev >= 0)
        {
            if (gsrc->gs_CmdLists)
                result = GS_AllocLists(gs, gsrc->gs_NumberList, gsrc->gs_ListSize);
            else
                result = 0;

            if (result >= 0)
                return gs;

            CloseItem(gs->gs_TEDev);
        }
        FreeMem(gs, sizeof(GState));
    }

    return NULL;
}


/*****************************************************************************/


Err GS_AllocLists(GState *gs, uint32 nLists, uint32 listSize)
{
Err        result;
uint32     i;
GSCmdList *cl;

    if (nLists == 0)
        return 0;

    gs->gs_CmdLists = AllocMem(nLists * sizeof(GSCmdList), MEMTYPE_FILL);
    if (gs->gs_CmdLists)
    {
        gs->gs_NumberList = nLists;
        gs->gs_SendList   = GS_SendList;
        gs->gs_ListSize   = listSize;

        result = 0;
        for (i = 0; i < nLists; i++)
        {
            cl = &gs->gs_CmdLists[i];

            cl->cl_CmdBuffer = AllocMemAligned(ALLOC_ROUND(listSize * sizeof(uint32) + kListSizeExtraPadding, gs->gs_DCacheLineSize),
                                               MEMTYPE_TRACKSIZE | MEMTYPE_NORMAL, gs->gs_DCacheLineSize);
            if (cl->cl_CmdBuffer == NULL)
            {
                result = GS_OUT_OF_MEMORY;
                break;
            }

            cl->cl_IOReq = result = CreateIOReq(NULL, 0, gs->gs_TEDev, gs->gs_MessagePort);
            if (cl->cl_IOReq < 0)
            {
                break;
            }
        }

        if (result >= 0)
        {
            gs->gs_SetBufferIO = result = CreateIOReq(NULL, 0, gs->gs_TEDev, 0);
            if (gs->gs_SetBufferIO >= 0)
            {
                gs->gs_DisplayFrameIO = result = CreateIOReq(NULL, 0, gs->gs_TEDev, 0);
                if (gs->gs_DisplayFrameIO >= 0)
                {
                    gs->gs_SpeedIO = result = CreateIOReq(NULL, 0, gs->gs_TEDev, 0);
                    if (gs->gs_SpeedIO >= 0)
                    {
                        result = GS_SetTESpeed(gs, TE_FULLSPEED);
                        if (result >= 0)
                        {
                            GS_SetList(gs, 0);
                            return 0;
                        }
                    }
                }
            }
        }

        GS_FreeLists(gs);
    }
    else
    {
        result = GS_OUT_OF_MEMORY;
    }

    return result;
}


/*****************************************************************************/


Err GS_FreeLists(GState *gs)
{
uint32     i;
GSCmdList *cl;

    if (gs->gs_CmdLists)
    {
        for (i = 0; i < gs->gs_NumberList; i++)
        {
            cl = &gs->gs_CmdLists[i];
            FreeMem(cl->cl_CmdBuffer, TRACKED_SIZE);
            DeleteIOReq(cl->cl_IOReq);
        }
        DeleteIOReq(gs->gs_DisplayFrameIO);
        DeleteIOReq(gs->gs_SetBufferIO);
        DeleteIOReq(gs->gs_SpeedIO);
        FreeMem(gs->gs_CmdLists, gs->gs_NumberList * sizeof(GSCmdList));
    }

    gs->gs_CmdLists   = NULL;
    gs->gs_NumberList = 0;
    gs->gs_SendList   = GS_BustedSendList;

    return 0;
}


/*****************************************************************************/


void GS_SetList(GState* g, uint32 l)
{
    g->gs_WhichList = l;
    g->gs_ListPtr   = g->gs_CmdLists[l].cl_CmdBuffer;
    g->gs_EndList   = g->gs_ListPtr + g->gs_ListSize;
}


/*****************************************************************************/


static Err GS_NextList(GState* g)
{
    uint32 	next = (g->gs_WhichList + 1);
    Err		err=0;

    if (next >= g->gs_NumberList)
        next = 0;

    GS_SetList(g, next);

    /* If the io requests associated with the new list is still busy, we
     * must wait for it to complete before going any further.
     */
    /* If the io requests associated with the new list is still busy, we
     * must wait for it to complete before going any further.
     */
    if (g->gs_Speed != TE_STOPPED)
    {
	if (CheckIO(g->gs_CmdLists[next].cl_IOReq)==0) {
	    err = WaitIO(g->gs_CmdLists[next].cl_IOReq);
	} else {
	    err = 0;
	}
    }
    return(err);
}

/*****************************************************************************/

static void GS_WaitTEPass(GState *g)
{
    /* Waits for the TE to get past g->gs_EndList */
    uint32 teaddr;
    uint32 endList;
#if DEBUG
    uint32	waitcount=0;
#endif

    teaddr = (uint32)GetTEReadPointer();
    endList = (((uint32)g->gs_EndList)&0x3fffffff);
    while(teaddr < endList ) {
#if DEBUG
	waitcount++;
	if (waitcount==DEBUG) {
	    printf("<GS_WaitTEPass> CPU<%d> %x endList at %x ahead of TE IRP at %x IWP at %x\n",
		   IsSlaveCPU(),g,endList, teaddr, GetTEWritePointer());
	    printf("listPtr %x endList %x realendList %x\n",g->gs_ListPtr, g->gs_EndList, g->gs_RealEndList);
	}
#endif
	teaddr = (uint32)GetTEReadPointer();
    }

}

Err GS_SendList(GState *g)
{
    IOInfo	ioInfo;
    Err 	err=0;
    uint32	flushedBytes;

    if (g->gs_Flags & GS_USE_LOW_LATENCY) {
	if (g->gs_Flags & GS_LL_START) {
	    if (err = GS_SendIO(g,1,0)) return err;
	}
	if (g->gs_LastEndList > g->gs_EndList) {
	    /* Wrap case */
	    /* We just wrapped to the top of the buffer. Must check that we aren't ahead of TE */
	    /*  Starting with
		top
		L+4K+SLOP
		<-ListPtr
		<-EndList
		. . .
		<- LastEndList IWP
		4K       TE somewhere here
		jmp
		. . .
		<-RealEndList
		*/
	    *(char**)&g->gs_EndList = ((char*)g->gs_ListPtr)+g->gs_Latency;
	    /* Spin until TE goes past where we want to write to
	       We assume that IO was sent because we got here
	     */
	    GS_WaitTEPass(g);
	    g->gs_LastEndList = g->gs_CmdLists[g->gs_WhichList].cl_CmdBuffer;

	    SetTEWritePointer(g->gs_LastEndList);
	    /*
		top <- LastEndList IWP
		??
		<-ListPtr
		4K
		<-EndList
		. . .
		4K       TE somewhere here
		jmp
		. . .
		<-RealEndList
		*/
	} else {
	    /* We are mid list just advance by 1 latency */
	    /* At this stage we have:
	       		---- <- LastEndList
				????
	       ListPtr ->
	       EndList ->
	       		. . .
	       RealEndList ->
	       */
	    flushedBytes = (((char*)g->gs_ListPtr)-((char*)g->gs_LastEndList))-(g->gs_DCacheSize+SLOP);
	    /* Either just send flushed portion or
	       flush everything and start over */
	    if (flushedBytes <= 0) {
		flushedBytes += g->gs_DCacheSize+SLOP;
		/* This is a small request send everything */
		FlushDCache(0, g->gs_LastEndList, flushedBytes);
		g->gs_LastEndList = g->gs_ListPtr;
		*(char**)&g->gs_EndList = ((char*)g->gs_ListPtr)+g->gs_Latency+g->gs_DCacheSize+SLOP;
		/* At this stage we have:
		   		----
		   		<4K+SLOP
		   ListPtr ->		 <- LastEndList
		   		L+4K+SLOP
		   EndList ->
		   		. . .
		   RealEndList ->
		   */
	    } else {
		/* We have computed more than 1 full cache so only send what's been flushed */

		ByteAdvance(g->gs_LastEndList,flushedBytes);
		*(char**)&g->gs_EndList = ((char*)g->gs_ListPtr)+g->gs_Latency;
	    }

	    if (g->gs_Flags & GS_LL_IO_SENT)
		SetTEWritePointer(g->gs_LastEndList);

	    /*
			       ----
	       		4K
	       		    <- LastEndList IWP
	       		4K+SLOP
	       ListPtr ->
	       		L
	       EndList ->
	       		. . .
	       RealEndList ->
	       */
	    if (g->gs_EndList >= g->gs_RealEndList) {
		uint32 *topList;
#if DEBUG
		uint32 waitcount=0;
#endif
		/* Wrap around to top of list if TE is running */
		while (!(g->gs_Flags & GS_LL_IO_SENT) ) {
#if DEBUG
		    waitcount++;
		    if (waitcount==DEBUG) {
			printf("<GS_SendList> CPU<%d> %x List filled, waiting for IO to start\n",IsSlaveCPU(),g);
			printf("listPtr %x endList %x realendList %x\n",g->gs_ListPtr, g->gs_EndList, g->gs_RealEndList);
		    }
#endif
		    FlushDCache(0, &g->gs_Flags, 4);
		}
#if DEBUG
		if (waitcount>=DEBUG) printf("<GS_SendList> CPU<%d> %x IO started\n",IsSlaveCPU(),g);
#endif

		topList = g->gs_CmdLists[g->gs_WhichList].cl_CmdBuffer;
		CLT_JumpAbsolute(GS_Ptr(g),topList);
		FlushDCacheAll(0);
		if (g->gs_Flags & GS_LL_IO_SENT)
		    SetTEWritePointer(g->gs_ListPtr-1);
		g->gs_ListPtr = topList;
		*(char**)&g->gs_EndList = ((char*)g->gs_ListPtr)+g->gs_Latency+g->gs_DCacheSize+SLOP;
		if (g->gs_EndList > g->gs_LastEndList) {
#ifdef BUILD_STRINGS
		    printf("<GS_SendList> CPU<%d> %x buffer too small wrapping into active area\n",IsSlaveCPU(),g);
#endif
		    exit(1);
		}
		/*
		   ListPtr ->		top
		   			L+4K+SLOP
		   EndList ->
		   			. . .
						<- LastEndList
					4K+Slop
		   			jmp    <- IWP
		   RealEndList ->
		   */

		/* Wait till TE passes where we want to write to */
		GS_WaitTEPass(g);
	    }
	}

    } else {
	uint32 *startList = g->gs_CmdLists[g->gs_WhichList].cl_CmdBuffer;

	/* high latency case */

	/* Don't send the list if it's empty */
	if (g->gs_ListPtr == startList)
	    return 0;

	CLT_Pause (&g->gs_ListPtr);		/* Put pause command at end of list */

	memset(&ioInfo, 0, sizeof(IOInfo));
	ioInfo.ioi_Command         = TE_CMD_EXECUTELIST;
	ioInfo.ioi_Send.iob_Buffer = startList;
	ioInfo.ioi_Send.iob_Len    = (uint32)g->gs_ListPtr - (uint32)startList;
	ioInfo.ioi_CmdOptions = 0;

	/* Video sync -- Wait til its safe to render to a screen */
	if ((g->gs_Flags & GS_BEGIN_FLAG) && g->gs_VidSignal) {
	    g->gs_Flags &= ~GS_BEGIN_FLAG;
	    if (g->gs_ProfileFunc) g->gs_ProfileFunc(g, 2);
	    WaitSignal( g->gs_VidSignal );
	    if (g->gs_ProfileFunc) g->gs_ProfileFunc(g, -2);
	}

	if (g->gs_Flags & GS_CLEARFB_FLAG) {
	    ioInfo.ioi_CmdOptions |= TE_CLEAR_FRAME_BUFFER;
	    g->gs_Flags &= ~GS_CLEARFB_FLAG;
	}
	if (g->gs_Flags & GS_CLEARZB_FLAG) {
	    ioInfo.ioi_CmdOptions |= TE_CLEAR_Z_BUFFER;
	    g->gs_Flags &= ~GS_CLEARZB_FLAG;
	}

	if (g->gs_Flags & GS_ABORT_VBLANK_FLAG) {
	    ioInfo.ioi_CmdOptions |= TE_ABORT_AT_VBLANK;
	}

	if (g->gs_Semaphore) {
	    int newtecount;

	    LockSemaphore(g->gs_Semaphore, SEM_WAIT);

	    newtecount=++g->gs_CurCount;

	    UnlockSemaphore(g->gs_Semaphore);

	    if (newtecount==1) g->gs_ProfileFunc(g, 1);
	}

	err = SendIO(g->gs_CmdLists[g->gs_WhichList].cl_IOReq, &ioInfo);
	g->gs_Count++;
	if (err < 0)
	    return err;

	err = GS_NextList(g);
    }

    return err;
}

/*****************************************************************************/

Err GS_SetVidSignal(GState* g, int32 signal)
{
    g->gs_VidSignal = signal;
    return 0;
}


int32 GS_GetVidSignal(GState* g)
{
    return g->gs_VidSignal;
}

/*****************************************************************************/

Err GS_BeginFrame(GState* g)
{
    g->gs_Flags |= GS_BEGIN_FLAG;
    return 0;
}

/*****************************************************************************/

Err GS_ClearFBZ(GState* g, bool clearfb, bool clearz)
{
    if (clearfb) {
	g->gs_Flags |= GS_CLEARFB_FLAG;
    }
    if (clearz) {
	g->gs_Flags |= GS_CLEARZB_FLAG;
    }
    return 0;
}

/*****************************************************************************/

Err GS_SetClearValues(GState* g, float R, float G, float B, float A, float Z)
{
    g->gs_FBInfo.tefbi_Red = R;
    g->gs_FBInfo.tefbi_Green = G;
    g->gs_FBInfo.tefbi_Blue = B;
    g->gs_FBInfo.tefbi_Alpha = A;
    g->gs_ZBInfo.tezbi_FillValue = Z;
    return 0;
}

/*****************************************************************************/

Err GS_SetAbortVblank(GState* g, bool abort)
{
    if (abort) {
	g->gs_Flags |= GS_ABORT_VBLANK_FLAG;
    } else {
	g->gs_Flags &= ~GS_ABORT_VBLANK_FLAG;
    }
    return 0;
}

/*****************************************************************************/

void GS_Reserve(GState *g, uint32 nwords)
{
    /* * * * * * *
       Should check to ensure that there's enough space, at least in dev builds
       * * * * * * */
#ifdef BUILD_STRINGS
    if (nwords > g->gs_ListSize) {
	printf("<GS_Reserve> Fatal error!! Cannot reserve more words than command list has\n");
	exit(-1);
    }
#endif
    if (g->gs_Flags & GS_USE_LOW_LATENCY) {
	if (g->gs_ListPtr+nwords >= g->gs_RealEndList) {
	    g->gs_SendList(g);
	}
    } else {
	if (g->gs_ListPtr+nwords >= g->gs_EndList) {
	    g->gs_SendList(g);
	}
    }
}

/*****************************************************************************/

static Err DispatchIO(Item ior, const IOInfo *ioInfo)
{
    if (CheckIO(ior) == 0) {
#if DEBUG
	printf("<DispatchIO> waiting for IOrequest\n");
#endif
        WaitIO(ior);
    }

    return SendIO(ior, ioInfo);
}

/*****************************************************************************/

Err GS_SetView (struct GState *g, Item viewItem)
{
	IOInfo	ioi;
	Err	err;

	memset (&ioi, 0, sizeof (ioi));
	ioi.ioi_Command	= TE_CMD_SETVIEW;
	ioi.ioi_Offset	= (uint32) viewItem;

	if ((err = DispatchIO (g->gs_DisplayFrameIO, &ioi)) >= 0)
		g->gs_View = viewItem;

	return (err);
}

/*****************************************************************************/

Err GS_SetDestBuffer(GState *g, Item bmItem)
{
IOInfo ioInfo;
Err    result;

    memset(&ioInfo, 0, sizeof(IOInfo));
    ioInfo.ioi_Command = TE_CMD_SETFRAMEBUFFER;
    ioInfo.ioi_Offset  = (uint32)bmItem;
    ioInfo.ioi_Send.iob_Buffer = &g->gs_FBInfo;
    ioInfo.ioi_Send.iob_Len = sizeof(TEFrameBufferInfo);

    result = DispatchIO(g->gs_SetBufferIO, &ioInfo);

    if (result >= 0)
        g->gs_DestBuffer = bmItem;

    return result;
}

/*****************************************************************************/

Err GS_SetZBuffer(GState* g, Item bmItem)
{
IOInfo ioInfo;
Err    result;

    memset(&ioInfo, 0, sizeof(IOInfo));
    ioInfo.ioi_Command = TE_CMD_SETZBUFFER;
    ioInfo.ioi_Offset  = (uint32)bmItem;
    ioInfo.ioi_Send.iob_Buffer = &g->gs_ZBInfo;
    ioInfo.ioi_Send.iob_Len = sizeof(TEZBufferInfo);

    result = DispatchIO(g->gs_SetBufferIO, &ioInfo);

    if (result >= 0)
        g->gs_ZBuffer = bmItem;

    return result;
}

/*****************************************************************************/

Err GS_SetTESpeed(GState *gs, uint32 speed)
{
IOInfo ioInfo;
Err    result;

    memset(&ioInfo, 0, sizeof(IOInfo));
    ioInfo.ioi_Command = TE_CMD_SPEEDCONTROL;
    ioInfo.ioi_CmdOptions = speed;
    result = DoIO(gs->gs_SpeedIO, &ioInfo);
    if (result >= 0)
    {
        gs->gs_Speed = speed;
    }

    return(result);
}

/*****************************************************************************/

Err GS_SetAbortVblankCount (struct GState *g, int32 count)
{
	IOInfo	ioi;

	memset (&ioi, 0, sizeof (ioi));
	ioi.ioi_Command	= TE_CMD_SETVBLABORTCOUNT;
	ioi.ioi_Offset	= count;

	return (DispatchIO (g->gs_DisplayFrameIO, &ioi));
}

/*****************************************************************************/

Err GS_SingleStep(GState *gs)
{
IOInfo ioInfo;
Err    result;

    memset(&ioInfo, 0, sizeof(IOInfo));
        /* First, make sure the TE has stopped */
    if (gs->gs_Speed != TE_STOPPED)
    {
        result = GS_SetTESpeed(gs, TE_STOPPED);
        if (result < 0)
        {
            return(result);
        }
    }
    ioInfo.ioi_Command = TE_CMD_STEP;
    result = DoIO(gs->gs_SpeedIO, &ioInfo);
    return(result);
}


/*****************************************************************************/

Err GS_EndFrame(GState *g)
{
IOInfo ioInfo;

    memset(&ioInfo, 0, sizeof(IOInfo));
    ioInfo.ioi_Command = TE_CMD_DISPLAYFRAMEBUFFER;
    return DispatchIO(g->gs_DisplayFrameIO, &ioInfo);
}


/*****************************************************************************/

Item GS_GetView (struct GState *g)
{
	return (g->gs_View);
}


/*****************************************************************************/

Item GS_GetDestBuffer(GState* g)
{
    return g->gs_DestBuffer;
}


/*****************************************************************************/

Item GS_GetZBuffer(GState* g)
{
    return g->gs_ZBuffer;
}

/*****************************************************************************/

Err GS_WaitIO(GState *g)
{
    uint32 i;
    Err err;

    for (i = 0; i < g->gs_NumberList; i++)
        if ((err = WaitIO(g->gs_CmdLists[i].cl_IOReq)) < 0) return err;

    return 0;
}

/*****************************************************************************/

uint32 GS_GetCmdListIndex(GState* g)
{
    return g->gs_WhichList;
}

/*****************************************************************************/

CmdListP GS_GetCurListStart(GState* g)
{
    return g->gs_CmdLists[g->gs_WhichList].cl_CmdBuffer;
}


/*****************************************************************************/

static Item AllocBitmap (int32 wide, int32 high, int32 type, Item coalign)
{
    Bitmap	*bm;
    Item	item;
    Err		err;
    char	*buf;

    item = CreateItemVA (MKNODEID (NST_GRAPHICS, GFX_BITMAP_NODE),
			 BMTAG_WIDTH, wide,
			 BMTAG_HEIGHT, high,
			 BMTAG_TYPE, type,
			 BMTAG_DISPLAYABLE, type != BMTYPE_16_ZBUFFER,
			 BMTAG_RENDERABLE, TRUE,
			 BMTAG_COALIGN, coalign,
			 TAG_END);
    if (item < 0) return item;

    bm = LookupItem (item);

    buf = AllocMemMasked (bm->bm_BufferSize,
			  bm->bm_BufMemType,
			  bm->bm_BufMemCareBits | 0x00000FFF,
			  bm->bm_BufMemStateBits);
    if (buf == NULL) {
	DeleteItem(item);
	return GS_OUT_OF_MEMORY;
    }

    err = ModifyGraphicsItemVA (item,
				BMTAG_BUFFER, buf,
				TAG_END);
    if (err < 0) {
	FreeMem(buf, bm->bm_BufferSize);
	DeleteItem(item);
	return err;
    }

    return item;
}

/*****************************************************************************/

Err GS_AllocBitmaps(Item bitmaps[], uint32 xres, uint32 yres,
			   uint32 bmType, uint32 numFb, bool useZb)
{
    int i;

    /* Returns an initialized graphics state obejct */

    if (numFb < 1)
	return ER_ParamError;

    for (i=0; i<numFb; i++) {
	bitmaps[i] = AllocBitmap (xres, yres, bmType, 0);
	if (bitmaps[i] < 0) {
	    int j = i-1;
	    while (j>=0) {
		Bitmap *bm=(Bitmap*)LookupItem(bitmaps[j]);
		FreeMem (bm->bm_Buffer, bm->bm_BufferSize);
		DeleteItem(bitmaps[j]);
		j--;
	    }
	    return bitmaps[i];
	}
    }

    if (useZb) {
	bitmaps[numFb] =
	    AllocBitmap (xres, yres, BMTYPE_16_ZBUFFER, bitmaps[0]);
	if (bitmaps[numFb] < 0) {
	    for (i=0; i<numFb; i++) {
		Bitmap *bm=(Bitmap*)LookupItem(bitmaps[i]);
		FreeMem (bm->bm_Buffer, bm->bm_BufferSize);
		DeleteItem(bitmaps[i]);
	    }
	    return bitmaps[numFb];
	}
    }

    return 0;
}

/*****************************************************************************/

Err GS_FreeBitmaps(Item bitmaps[], uint32 numBitmaps )
{
    int i;
    Err e1, err;
    Bitmap* bm;

    err = 0;
    for (i=0; i<numBitmaps; i++) {
	bm = (Bitmap*)LookupItem(bitmaps[i]);
	if (bm) {
		FreeMem(bm->bm_Buffer, bm->bm_BufferSize);
		e1 = DeleteItem(bitmaps[i]);
		if (e1<err) err=e1;
	}
    }
    return err;

}

/*****************************************************************************/

uint32 GS_GetCount(GState *gs)
{
    return(gs->gs_Count);
}

/*****************************************************************************/

Err
GS_SendIO(GState *g, bool wait, uint32 len)
{
    Err 	err;
    IOInfo 	ioInfo;
    uint32	*cmdList;
    uint32 	teaddr;
    uint32 	hwcmdlist;
    uint32	hwendlist;
#if DEBUG
    uint32	waitcount=0;
#endif

    if (!(g->gs_Flags & GS_USE_LOW_LATENCY)) {
#ifdef BUILD_STRINGS
	printf("<GS_SendIO> CPU<%d> %x SendIO called before calling GS_LowLatency()\n",IsSlaveCPU(),g);
#endif
	return -1;
    }

    /* For the Low Latency case, start the IO operation on the
       GState. This is intended for use by the master processor
       while the slave is still writing into the command list */

    if (g->gs_Flags & GS_LL_IO_SENT) {
	return -1;
    }

    if (0==CheckIO(g->gs_CmdLists[g->gs_WhichList].cl_IOReq)) {
#if DEBUG
	printf("<GS_SendIO> CPU<%d> %x Waiting for IO to complete\n",IsSlaveCPU(),g);
#endif
	WaitIO(g->gs_CmdLists[g->gs_WhichList].cl_IOReq);
    }

    cmdList = g->gs_CmdLists[g->gs_WhichList].cl_CmdBuffer;

    memset(&ioInfo, 0, sizeof(IOInfo));
    ioInfo.ioi_Command         = TE_CMD_EXECUTELIST;
    ioInfo.ioi_Send.iob_Buffer = cmdList;
    ioInfo.ioi_Send.iob_Len    = len;
    ioInfo.ioi_CmdOptions = TE_LIST_FLUSHED;

    /* Video sync -- Wait til its safe to render to a screen */
    if ((g->gs_Flags & GS_BEGIN_FLAG) && g->gs_VidSignal) {
	g->gs_Flags &= ~GS_BEGIN_FLAG;
	if (g->gs_ProfileFunc) g->gs_ProfileFunc(g, 2);
	WaitSignal( g->gs_VidSignal );
	if (g->gs_ProfileFunc) g->gs_ProfileFunc(g, -2);
    }

    if (g->gs_Flags & GS_CLEARFB_FLAG) {
	ioInfo.ioi_CmdOptions |= TE_CLEAR_FRAME_BUFFER;
	g->gs_Flags &= ~GS_CLEARFB_FLAG;
    }
    if (g->gs_Flags & GS_CLEARZB_FLAG) {
	ioInfo.ioi_CmdOptions |= TE_CLEAR_Z_BUFFER;
	g->gs_Flags &= ~GS_CLEARZB_FLAG;
    }

    if (g->gs_Flags & GS_ABORT_VBLANK_FLAG) {
	ioInfo.ioi_CmdOptions |= TE_ABORT_AT_VBLANK;
    }

    if (g->gs_Semaphore) {
	int newtecount;

	LockSemaphore(g->gs_Semaphore, SEM_WAIT);

	newtecount=++g->gs_CurCount;

	UnlockSemaphore(g->gs_Semaphore);

	if (newtecount==1) g->gs_ProfileFunc(g, 1);
    }

    err = SendIO(g->gs_CmdLists[g->gs_WhichList].cl_IOReq, &ioInfo);
    if (err != 0) {
#ifdef BUILD_STRINGS
	printf("<GS_SendIO> Error in SendIO\n");
#endif
	return err;
    }

    if (wait) {
	teaddr = (uint32)GetTEReadPointer();
	hwcmdlist = (((uint32)cmdList)&0x3fffffff);
	hwendlist = hwcmdlist + len;
	while ((teaddr < hwcmdlist) ||
	       (teaddr > hwendlist)) {
#if DEBUG
	    if (waitcount==DEBUG) {
		printf("<GS_SendIO> CPU<%d> %x Waiting for TE to be %x IRP at %x IWP at %x\n",
		       IsSlaveCPU(),g,hwcmdlist,teaddr,GetTEWritePointer());
	    }
	    waitcount++;
#endif
	    teaddr = (uint32)GetTEReadPointer();
	}

#if DEBUG
	if (waitcount>=DEBUG) printf("<GS_SendIO> CPU<%d> %x TE IRP at %x IWP at %x\n",
				     IsSlaveCPU(),g,GetTEReadPointer(),GetTEWritePointer());
#endif
    }

    g->gs_Flags |= GS_LL_IO_SENT;
    g->gs_Flags &= ~GS_LL_START;
    g->gs_Count++;

    /* Since slave CPU might be waiting for this flag to be set
       we'd better flush this line from the cache */

    FlushDCache(0, &g->gs_Flags, 4);

    return 0;
}

/*****************************************************************************/

Err
GS_SendLastList(GState *g)
{
    Err 	err;
#if DEBUG
    uint32 	waitcount=0;
#endif

    /* This is the last function called to dispatch the list */
    /* TE has already been running, place a pause instruction and advance IWP */
    /* At this stage we have:
                       ---- <- LastEndList
		         8K
	ListPtr ->
	EndList ->
                       . . .
       RealEndList ->
       */

    if (g->gs_Flags & GS_USE_LOW_LATENCY) {

	CLT_Pause(&g->gs_ListPtr);		/* Put pause command at end of list */

	FlushDCacheAll(0);

	if (g->gs_Flags & GS_LL_START) {
	    Err	err;
	    if (err = GS_SendIO(g,1,((uint32)g->gs_ListPtr) -
				((uint32)g->gs_CmdLists[g->gs_WhichList].cl_CmdBuffer)))
		return err;
	} else {
	    /* We must be slave CPU becaue we can't initiate the IO
	       so sit and wait for other CPU to do the SendIO */
	    while (!(g->gs_Flags & GS_LL_IO_SENT)) {
		/* Spin wait for IO to be sent */
#if DEBUG
		if (waitcount==DEBUG) {
		    printf("<GS_SendLastList> CPU<%d> %x Waiting for IO to start \n",IsSlaveCPU(),g);

		}
		waitcount++;
#endif
		FlushDCache(0, &g->gs_Flags, 4);
	    }
#if DEBUG
	    if (waitcount>=DEBUG) printf("<GS_SendLastList> CPU<%d> %x IO started\n",IsSlaveCPU(),g);
#endif

	}

	SetTEWritePointer(g->gs_ListPtr);

	g->gs_Flags &= ~(GS_LL_IO_SENT | GS_USE_LOW_LATENCY);

	/* Must sync for other CPU to see */
	FlushDCache(0, &g->gs_Flags, 4);

	err = GS_NextList(g);

    } else {
	/* Old High latency way */
	return GS_SendList(g);
    }

    return err;
}

/*****************************************************************************/

Err GS_LowLatency(GState* g, bool send, uint32 latency)
{
    Err err;

#if DEBUG1
    ll[IsSlaveCPU()][0]=0;
#endif

    if (g->gs_Flags & GS_USE_LOW_LATENCY) return 0;

    if (g->gs_ListPtr != g->gs_CmdLists[g->gs_WhichList].cl_CmdBuffer) {
	/* List has command is it, send them */
#if DEBUG
	printf("<GS_LowLatency> Command list in use switching\n",IsSlaveCPU(),g);
#endif
	GS_SendList(g);
    }

    if (0==CheckIO(g->gs_CmdLists[g->gs_WhichList].cl_IOReq)) {
	/* Current command list is in use switch to another one */
	if (err = GS_NextList(g))
	    return err;
    }
    g->gs_Latency = latency<<2; /* Keep latency in bytes */
    g->gs_LastEndList = g->gs_ListPtr;
    g->gs_RealEndList = g->gs_ListPtr + g->gs_ListSize;
    (*(char**)&g->gs_EndList) = ((char*)g->gs_ListPtr)+g->gs_Latency+g->gs_DCacheSize+SLOP;
    if (g->gs_EndList > g->gs_RealEndList) {
#ifdef BUILD_STRINGS
	printf("<GS_LowLatency> CPU<%d> %x Command list too small to use low latency option\n",IsSlaveCPU(),g);
#endif
	return -1;
    }

    g->gs_Flags |= GS_USE_LOW_LATENCY ;

    if (send) g->gs_Flags |= GS_LL_START;

    g->gs_Flags &= ~GS_LL_IO_SENT;

    /* ListPtr ->      top  <- LastEndList
                    L+4K+SLOP
       EndList ->
                    . . .
       RealEndList ->
	*/

    return 0;
}

/*****************************************************************************/

bool GS_IsLowLatency(GState *g)
{
    return (0 !=(g->gs_Flags & GS_USE_LOW_LATENCY));
}

/*****************************************************************************/


Err GS_EnableProfiling(GState *g, GSProfileFunc proc)
{
    uint8		Priority;


    if (g->gs_ProfileFunc || g->gs_Semaphore || g->gs_Thread) {
	GS_DisableProfiling(g);
    }

    g->gs_ProfileFunc = proc;

    if ((g->gs_Semaphore=CreateSemaphore("GState_PRofile_semaphore", 128))<0) {
#ifdef BUILD_STRINGS
	printf("<GS_EnableProfiling> Unable to create semaphore\n");
#endif
	return -1;
    }

    Priority = CURRENTTASK->t.n_Priority + 2;

    if ((g->gs_Thread=CreateThreadVA(GS_ThreadProc, "GState_Profile_thread", Priority,
				   10000, CREATETASK_TAG_ARGC, g,
				          TAG_END))<0) {
#ifdef BUILD_STRINGS
	printf("<GS_EnableProfiling> Unable to create Profiling thread\n");
#endif
	return -1;
    }

    g->gs_CurCount = 0;

    return 0;
}

/*****************************************************************************/

Err GS_DisableProfiling(GState *g)
{
    Err ret;

    g->gs_ProfileFunc = 0;

    if (g->gs_Semaphore) {
	ret = DeleteSemaphore(g->gs_Semaphore);
	if (ret < 0) return ret;
    }
    g->gs_Semaphore = 0;

    if (g->gs_Thread) {
	ret = DeleteThread(g->gs_Thread);
	if (ret < 0) return ret;
    }
    g->gs_Thread = 0;

    g->gs_CurCount = 0;

    return 0;
}


