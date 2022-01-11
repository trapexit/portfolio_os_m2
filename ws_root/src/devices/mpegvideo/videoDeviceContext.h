/* @(#) videoDeviceContext.h 96/12/11 1.8 */
/*	file: videoDeviceContext.h
	tVideoDeviceContext definition
	4/12/94 George Mitsuoka
	The 3DO Company Copyright (C) 1995 */

#ifndef VIDEO_DEVICECONTEXT_HEADER
#define VIDEO_DEVICECONTEXT_HEADER

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/task.h>

typedef struct {
	uint32 * address;			/* aligned stripBuffer address */
	unsigned sizeInBytes:24,	/* size of aligned stripBuffer in bytes */
			 sizeCode:8;		/* size code passed to hardware */
} StripBuffer;

extern StripBuffer gStripBuffer;

typedef
    struct                          /* unit structure */
    {
		struct Task *task;			/* task to signal */
		Item taskItem;				/* task's item number */
        List writeQueue;            /* queue of write IOReqs */
        List readQueue;             /* queue of read IOReqs */
        IOReq *currentWriteReq;     /* current write request */
        IOReq *currentReadReq;      /* current read request */
		int32 readReqOffset;		/* offset into current read req buffer */
		int32 signalWriteAvail;		/* new write IOReq available */
		int32 signalReadAvail;		/* new read IOReq available */
		int32 signalReadAbort;		/* abort the current read req */
		int32 signalWriteAbort;		/* abort the current write req */
		int32 signalQuit;           /* device closed, exit thread */
		int32 signalFlush;			/* flush the device */
		uint32 vdcFlags;			/* device flags */
		uint32 branchState;         /* unit's branch state */

		/* M2 MPEG video specific stuff */
		int32 signalStripBufferError;	/* strip buffer error */
		int32 signalEverythingDone;		/* output formatter, parser done */
		int32 signalOutputFormatter;	/* output formatter done */
		int32 signalOutputDMA;			/* output DMA completed */
		int32 signalBitstreamError;		/* parser error */
		int32 signalEndOfPicture;		/* parser completed */
		int32 signalVideoBitstreamDMA;	/* bitstream buffer exhausted */
		int32 signalTimeout;			/* hardware timeout signal */
		int32 signalMask;				/* bitwise or of interrupt signals */
		Item  timerIOReqItem;			/* for timeout handling */

		uint32 *refPic[ 2 ];			/* pointers to reference pictures */
		uint32 refPicSize;				/* size in bytes of both buffers (as alloc'd) */
		uint32 refPicFlags[ 2 ];		/* state of reference pictures */
		IOReq *clientRBReq;				/* client-supplied ref buffer ioreq */
		
		StripBuffer *stripBuffer;		/* -> strip buffer to use (either local or shared/global) */
		StripBuffer  myStripBuffer;		/* local strip buffer if shared/global isn't right-sized */
		IOReq *	clientSBReq;			/* client-supplied strip buffer ioreq */
		
		uint32	lastBufferWidth;		/* last width and height AllocDecodeBuffers() used; */
		uint32	lastBufferHeight;		/* optimizes processing of subsequent sequence hdrs. */

		uint32 *destBuffer;				/* output buffer provided by app */
		uint32 destBufferSize;			/* size of the output buffer */

		/* decode parameters */
		unsigned	unused0:12,			/* align fields to nibble boundaries */
					playMode:4,			/* play or I frame search */
					operaMode:4,		/* output in opera format */
					resampleMode:4,		/* turn on resampling if available */
					outputRGB:4,		/* output in RGB */
					nextRef:4;			/* next destination reference picture */
		int32 depth;					/* output pixel depth */
		int32 pixelSize;				/* calculated from depth */
		int32 skipCount;				/* for skipping B frames */
		uint32 xSize, ySize;			/* image size */
		uint32 hStart,vStart;			/* crop rectangle upper left */
		uint32 hStop,vStop;				/* lower right, all macroblock units */
		uint32 dither[ 2 ];				/* dither matrix upper, lower */
		unsigned	unused1:20,			/* align field to nibble boundaries */
					DSB:4,				/* DSB bit */
					alphaFill:8;		/* alpha fill value */
    }
    tVideoDeviceContext;

/* Units branching states */
enum {
	NO_BRANCH = 0,           /* Playing normally. No branching yet. */
	JUST_BRANCHED,           /* Just recieved the branch notification */
	FIRST_REF_AFTER_BRANCH,  /* Recieved one I frame after branch notification.*/
};

#define VDCFLAG_ABORT_WRITE			(1 << 0)
#define VDCFLAG_ABORT_READ			(1 << 1)
#define VDCFLAG_SB_CLIENT_CONTROL	(1 << 2)	/* client controls strip buffer size */
#define VDCFLAG_RB_CLIENT_CONTROL	(1 << 3)	/* client controls reference buffer size */

#define REFPICEMPTY				0			/* reference picture is empty */
#define REFPICDECODED			1			/* picture has been decoded */

#endif

