/* @(#) mpVideoMain.c 96/12/11 1.32 */

#include <stdlib.h>
#include <string.h>
#include <kernel/types.h>
#include <kernel/debug.h>
#include <kernel/operror.h>
#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <kernel/task.h>
#include <kernel/super.h>
#include <device/mpegvideo.h>

#include "mpVideoDriver.h"
#include "mpVideoDecode.h"

#include "M2MPEGUnit.h"

#ifdef DEBUG_PRINT_DEV
  #define DBGDEV(a)		PRNT(a)
#else
  #define DBGDEV(a) 	(void)0
#endif

int32 gDebugFlag = 0L;
Item gMainTaskItem;

#define DAEMON_STACK_SIZE	(3*1024) /* was 16k, PS display shows 1k max ever used; 3k s/b safe */
#define DAEMON_PRI			200

struct MPThreadArg {     /* Structure used to pass arguments to DaemonThread */
	int32 signalReady;
	int32 signalAbort;
	tVideoDeviceContext *theUnit;
};

static void DaemonThread(struct MPThreadArg *theArg)
{
	tVideoDeviceContext *theUnit;

	/* initialize unit context and set up signalling mechanism */
	theUnit = theArg->theUnit;
	theUnit->task = CURRENTTASK;
	theUnit->taskItem = CURRENTTASK->t.n_Item;

	DBGDEV(("Daemon Thread startup, current task = %08lx, item %ld\n",
			theUnit->task, theUnit->taskItem));

	InitList( &(theUnit->writeQueue), "MPEG Video Device Write Queue");
	InitList( &(theUnit->readQueue), "MPEG Video Device Read Queue");
	theUnit->currentWriteReq = (IOReq *) 0L;
	theUnit->currentReadReq = (IOReq *) 0L;
	theUnit->readReqOffset = 0L;

	/* normal driver signals */
	if( ((theUnit->signalWriteAvail = AllocSignal( 0 )) == 0L) ||
		((theUnit->signalWriteAbort = AllocSignal( 0 )) == 0L) ||
		((theUnit->signalReadAvail = AllocSignal( 0 )) == 0L) ||
		((theUnit->signalReadAbort = AllocSignal( 0 )) == 0L) ||
		((theUnit->signalQuit = AllocSignal( 0 )) == 0L) ||
		((theUnit->signalFlush = AllocSignal( 0 )) == 0L) )
	{
		PERR(("couldn't allocate signals\n"));
		SendSignal(gMainTaskItem, theArg->signalAbort);
		goto abort;
	}
	DBGDEV(("signalQuit       = %08lx\n",theUnit->signalQuit));
	DBGDEV(("signalFlush       = %08lx\n",theUnit->signalFlush));
	DBGDEV(("signalWriteAvail = %08lx\n",theUnit->signalWriteAvail));
	DBGDEV(("signalWriteAbort  = %08lx\n",theUnit->signalWriteAbort));
	DBGDEV(("signalReadAvail  = %08lx\n",theUnit->signalReadAvail));
	DBGDEV(("signalReadAbort = %08lx\n",theUnit->signalReadAbort));

	theUnit->vdcFlags = 0L;

	/* M2 MPEG specific signals */
	if( ((theUnit->signalStripBufferError = AllocSignal( 0 )) == 0L) ||
		((theUnit->signalEverythingDone = AllocSignal( 0 )) == 0L) ||
		((theUnit->signalOutputFormatter = AllocSignal( 0 )) == 0L) ||
		((theUnit->signalOutputDMA = AllocSignal( 0 )) == 0L) ||
		((theUnit->signalBitstreamError = AllocSignal( 0 )) == 0L) ||
		((theUnit->signalEndOfPicture = AllocSignal( 0 )) == 0L) ||
		((theUnit->signalVideoBitstreamDMA = AllocSignal( 0 )) == 0L) ||
		((theUnit->signalTimeout = AllocSignal( 0 )) == 0L) )
	{
		PERR(("couldn't allocate signals\n"));
		SendSignal(gMainTaskItem, theArg->signalAbort);
		goto abort;
	}
	DBGDEV(("signalStripBufferError =%08lx\n",theUnit->signalStripBufferError));
	DBGDEV(("signalEverythingDone   =%08lx\n",theUnit->signalEverythingDone));
	DBGDEV(("signalOutputFormatter  =%08lx\n",theUnit->signalOutputFormatter));
	DBGDEV(("signalOutputDMA        =%08lx\n",theUnit->signalOutputDMA));
	DBGDEV(("signalBitstreamError   =%08lx\n",theUnit->signalBitstreamError));
	DBGDEV(("signalEndOfPicture     =%08lx\n",theUnit->signalEndOfPicture));
	DBGDEV(("signalVideoBitstreamDMA=%08lx\n",theUnit->signalVideoBitstreamDMA));

	theUnit->signalMask = theUnit->signalStripBufferError |
		                  theUnit->signalEverythingDone |
		                  theUnit->signalOutputFormatter |
		                  theUnit->signalOutputDMA |
		                  theUnit->signalBitstreamError |
		                  theUnit->signalEndOfPicture |
		                  theUnit->signalVideoBitstreamDMA;

	theUnit->timerIOReqItem = CreateTimerIOReq();
	if( theUnit->timerIOReqItem <= 0 )
	{
		PERR(("couldn't create timer IOReq item\n"));
		PrintfSysErr( theUnit->timerIOReqItem );
		/* not fatal */
		theUnit->timerIOReqItem = (Item) 0L;
	}
	else
		theUnit->signalMask |= theUnit->signalTimeout;

	/* set up misc defaults */
	theUnit->playMode = MPEGMODEPLAY;	/* we're playing */
	theUnit->operaMode = 0;				/* default to M2 mode */
	theUnit->resampleMode = 0;			/* square pixels */
	theUnit->outputRGB = 1;				/* output in RGB */
	theUnit->depth = 16L;				/* 16 bit pixels */
    theUnit->pixelSize = 2;				/* 2 bytes per pixel */
	theUnit->skipCount = 0L;			/* don't skip any pictures */
	theUnit->nextRef = 0;
	theUnit->refPic[0] = 0;             /* don't allocate reference buffers */
	theUnit->refPic[1] = 0;				/* until client req. or video seq. hdr seen */
	theUnit->refPicFlags[0] = REFPICEMPTY;
	theUnit->refPicFlags[1] = REFPICEMPTY;
	theUnit->branchState = NO_BRANCH;

	/* set up default cropping */
	theUnit->xSize = 320;				/* 320 horizontal */
    theUnit->ySize = 240;				/* 240 vertical */
	theUnit->hStart = DEFAULT_LEFT_CROP / MACROBLOCKWIDTH;
	theUnit->vStart = DEFAULT_TOP_CROP / MACROBLOCKHEIGHT;
	theUnit->hStop = theUnit->hStart + DEFAULT_WIDTH / MACROBLOCKWIDTH - 1L;
	theUnit->vStop = theUnit->vStart + DEFAULT_HEIGHT / MACROBLOCKHEIGHT - 1L;

	/* set up default dithering */
	theUnit->dither[ 0 ] = 0xc0d12e3fL;
	theUnit->dither[ 1 ] = 0xe1d0302fL;

	/* set default DSB and alpha fill */
	theUnit->DSB = 1;
	theUnit->alphaFill = 0;

	SendSignal(gMainTaskItem, theArg->signalReady);

	mpVideoDecode(theUnit);

abort:
	DBGDEV(("MPEG DaemonThread exit\n"));
}



int32
mpNewThread(tVideoDeviceContext *theUnit)
{
	Item child;
	int32 waitResult, result = 0;
	struct MPThreadArg theArg;
	uint8 oldPriv;

	DBGDEV(("creating daemon thread\n"));
	oldPriv = PromotePriv(CURRENTTASK);

    /* set up signal so we can wait for child to become ready */
    gMainTaskItem = CURRENTTASK->t.n_Item;

    if (((theArg.signalReady = AllocSignal(0)) == 0L) ||
		((theArg.signalAbort = AllocSignal(0)) == 0L))
    {
        PERR(("Error allocating signalReady, signalAbort\n"));
		DemotePriv(CURRENTTASK, oldPriv);
        return( (int) MakeKErr(ER_SEVERE, ER_C_STND, ER_NoSignals) );
    }
	theArg.theUnit = theUnit;

	child = CreateItemVA(MKNODEID(KERNELNODE,TASKNODE),
                                TAG_ITEM_NAME,                  "MPEG Video Daemon",
                                TAG_ITEM_PRI,                   DAEMON_PRI,
                                CREATETASK_TAG_PC,              DaemonThread,
                                CREATETASK_TAG_STACKSIZE,       DAEMON_STACK_SIZE,
                                CREATETASK_TAG_SUPERVISOR_MODE, TRUE,
                                CREATETASK_TAG_THREAD,          TRUE,
                                CREATETASK_TAG_ARGC,            &theArg,
				TAG_END);

	if (child < 0)
	{
	    PERR(("Error creating daemon thread: "));
	    PrintfSysErr( child );
	    result = (int) child;
		goto done;
	}
    DBGDEV(("Waiting for daemon to start up\n"));
    waitResult = WaitSignal( theArg.signalReady | theArg.signalAbort );

	if( waitResult & theArg.signalReady )
    	DBGDEV(("Daemon says he's ready\n"));
	if( waitResult & (theArg.signalAbort | SIGF_ABORT) )
	{
		PERR(("Daemon startup error\n"));
        result = (int) MakeKErr(ER_SEVERE,ER_C_STND,ER_DeviceError);
	}

done:
   	FreeSignal(theArg.signalReady | theArg.signalAbort);
	DemotePriv(CURRENTTASK, oldPriv);

	return result;
}

int mpEntry( int32 argc, char *argv[] )
{
	Item driverItem;
	int32 arg, demandLoaded = 0;

#if 0
	if (argc == DEMANDLOAD_MAIN_CREATE)
#else
	/* Out of date header: kernel/item.h */
	if (argc == 0)
#endif
		demandLoaded = 1;
	else if( argc == DEMANDLOAD_MAIN_DELETE )
		return( 0 );

	if( !demandLoaded )
	{
		for(arg = 1; arg < argc; arg++)
		{
			if( strcasecmp(argv[ arg ],"-debug") == 0 )
			{
				gDebugFlag = 1L;
#ifdef CPU_ARM
				Debug();
#endif
			}
		}
	}

	memset(&gStripBuffer, 0, sizeof(StripBuffer));

	DBGDEV(("creating device\n"));

	driverItem = mpvCreateDevice();
	if (driverItem < 0)
	{
		PERR(("Error creating device %d\n",driverItem));
		PrintfSysErr( driverItem );
		return( (int) driverItem );
	}

	if( !demandLoaded )
		WaitSignal( 0L );

	return( (int) driverItem );
}


int	main(int32 argc, char *argv[])
{
    TOUCH( argc );
    TOUCH( argv );

	return mpEntry(argc, NULL);
}
