/******************************************************************************
**
**  @(#) subscribertraceutils.h 96/04/12 1.16
**
******************************************************************************/
#ifndef __SUBSCRIBERTRACEUTILS_H__
#define __SUBSCRIBERTRACEUTILS_H__

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

/* The number of events in each trace buffer */
#define	MAX_TRACE		1000


/**********************************************************************
 * Data Structures 
 **********************************************************************/

typedef struct TraceEntry {
	int32	when;		/* time stamp */
	int32	what;		/* event code, aka event */
	int32	channel;	/* see AddTrace() */
	int32	aValue;		/* see AddTrace() */
	void*	aPtr;		/* see AddTrace() */
	} TraceEntry, *TraceEntryPtr;

typedef struct TraceBuffer {
	uint32			index;					/* index of current entry */
	TraceEntry		evList[MAX_TRACE];		/* The list of trace events */
	} TraceBuffer, *TraceBufferPtr;


/**********************************************************************
 * Trace Event IDs 
 **********************************************************************/

enum {
	/* Generic subscriber Trace Event IDs */
	kTraceInitSubscriber		= 1000,
	kTraceCloseSubscriber		= 1001,
	kTraceNewSubscriber			= 1002,
	kTraceDisposeSubscriber		= 1003,
	kTraceWaitingOnSignal		= 1004,
	kTraceGotSignal				= 1005,
	kTraceDataMsg				= 1006,
	kTraceGetChanMsg			= 1007,
	kTraceSetChanMsg			= 1008,
	kTraceControlMsg			= 1009,
	kTraceSyncMsg				= 1010,
	kTraceOpeningMsg			= 1011,
	kTraceClosingMsg			= 1012,
	kTraceStopMsg				= 1013,
	kTraceStartMsg				= 1014,
	kTraceEOFMsg				= 1015,
	kTraceAbortMsg				= 1016,
	kTraceDataSubmitted			= 1017,
	kTraceDataCompleted			= 1018,
	
	/* Generic subscriber channel Trace Event IDs */
	kTraceChannelInit			= 1019,
	kTraceChannelClose			= 1020,
	kTraceChannelStart			= 1021,
	kTraceChannelStop			= 1022,
	kTraceChannelFlush			= 1023,
	kTraceChannelNewDataArrived	= 1024,
	
	/* Additional generic subscriber Trace Event IDs */
	kFlushedDataMsg				= 1025,
	kNewHeaderMsgArrived		= 1026,
	kNewDataMsgArrived			= 1027,
	kHaltChunkArrived			= 1028,
	kRepliedToHaltChunk			= 1029,
	kFlushedBuffer				= 1030,
	kMovedBuffer				= 1031,
	kBufferCompleted			= 1032,
	kFoundBuffer				= 1033,
	kFoundWrongBuffer			= 1034,
	kTraceBranchMsg				= 1035,
	kTraceChannelBranchMsg		= 1036,
	kTraceDecodeBufferRcvd		= 1037,
	kTraceDecodeBufferSent		= 1038,

	kTracePreBranchTime			= 1039,
	kTracePreBranchTimeRunning	= 1040,
	kTraceCurrentBranchTime		= 1041,
	kTraceCurrentBranchTimeRunning	= 1042,
	kTracePostBranchTime		= 1043,
	kTracePostBranchTimeRunning	= 1044,

	kFreedBuffer				= 1045,


	/* SAudio Subscriber Trace Event IDs */
	kFreedBufferFromFolio		= 2000,
	/*							= 2001, */
	kMaskedSignalBits			= 2002,
	/*							= 2003, */
	kBeginSetChannelAmp			= 2004,
	kBeginSetChannelPan			= 2005,
	kEndSetChannelAmp			= 2006,
	kEndSetChannelPan			= 2007,
	kCurrentBufferOnStop		= 2008,
	kBufferOrphaned				= 2009,
	kLoadedTemplates			= 2010,
	kChannelMuted				= 2011,
	kChannelUnMuted				= 2012,
	kTraceInternalChannelPause	= 2013,
	kTraceInternalChannelStop	= 2014,
	kBeginPlayback				= 2015,
	/* kStartAttachment			= 2016, This has been removed and replaced by kStartSpooler */
	/* kResumeInstrument			= 2017, This has been removed and replaced by kResumeSpooler */
	kStartWFlushWhilePaused		= 2018,
	kStopWFlushWhilePaused		= 2019,
	kAttachCrossedWhileStopping	= 2020,
	kAttachCrossedWhilePausing	= 2021,
	kCurrentBufferOnPause		= 2022,
	kStartSpooler				= 2023,
	kResumeSpooler				= 2024,

/* MPEG Audio Subscriber Trace Event IDs */
	kTraceCompressedBfrMsg				= 2500,
	kTraceSendDecompressedBfrMsg		= 2501,
	kTraceReceivedDecompressedBfrMsg	= 2502,
	kTraceCloseDecoderMsg				= 2503,
	kTraceFlushReadMsg					= 2504,
	kTraceFlushWriteMsg					= 2505,
	kTraceFlushReadWriteMsg				= 2506,
	kSendDecoderRequestMsg				= 2507,
	kGotDecoderReply					= 2508,


	/* MPEG Video Subscriber Trace Event IDs */
	kRecievedFrameBufferList	= 3000,
	kRecievedFramePort			= 3001,
	
	kSubmitMPEGWriteBuffer		= 3002,
	kCompleteMPEGWriteBuffer	= 3003,

	kSubmitMPEGReadBuffer		= 3004,
	kCompleteMPEGReadBuffer		= 3005,
	
	kMPEGReadPTS				= 3006,
	kMPEGReadDeltaPTS			= 3007,
	
	kSubmitMPEGBranch			= 3008,
	
	
	/* DATA Subscriber Trace Event IDs */
	kDataTraceStartDataCopy			= 4000,
	kDataTraceContinueCopy			= 4001,
	kDataChunkPiece1Arrived			= 4002,
	kDataChunkPieceNArrived			= 4003,
	kDataEnterProcessMessageQueue	= 4004,
	kDataLeaveProcessMessageQueue	= 4005,
	};


/**********************************************************************
 * Public routine prototypes 
 **********************************************************************/

#ifdef __cplusplus 
extern "C" {
#endif

void	AddTrace(TraceBufferPtr traceBuffer, int32 event, int32 channel,
				int32 value, void *ptr);
Err		DumpRawTraceBuffer(TraceBufferPtr traceBuffer, const char *filename);
Err		DumpTraceCompletionStats(TraceBufferPtr traceBuffer,
				int32 startEventCode, int32 completionEventCode,
				const char *filename);

#ifdef __cplusplus
}
#endif

#endif	/* __SUBSCRIBERTRACEUTILS_H__ */

