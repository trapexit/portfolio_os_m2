/****************************************************************************
**
**  @(#) datastream.h 96/03/04 1.30
**
*****************************************************************************/
#ifndef __STREAMING_DATASTREAM_H
#define __STREAMING_DATASTREAM_H


#ifndef __KERNEL_TYPES_H
	#include <kernel/types.h>
#endif

#ifndef __KERNEL_ITEMS_H
	#include <kernel/item.h>
#endif

#ifndef __KERNEL_IO_H
	#include <kernel/io.h>
#endif

#ifndef	__MSGUTILS_H__
	#include <streaming/msgutils.h>
#endif

#ifndef	__STREAMING_MEMPOOL_H
	#include <streaming/mempool.h>
#endif

#ifndef __STREAMING_DSSTREAMDEFS_H
	#include <streaming/dsstreamdefs.h>
#endif

#ifndef __STREAMING_DSERROR_H
	#include <streaming/dserror.h>
#endif


/***************
 * Basic types
 ***************/
typedef uint32 DSDataType;	/* a 4-character code */


/******************************************************************************
 * 		Data Stream Clock structures
 *
 * <branchNumber, streamTime> never goes backwards, even when jumping backwards
 * in the stream or jumping to another stream.
 *
 * Note that the clock tells the current presentation time for audio/video/data
 * being presented right now. Data earlier in the pipeline will have later
 * presentation time stamps.
 ******************************************************************************/
typedef struct DSClock {
	bool		running;		/* TRUE if the clock is currently running */
	uint32		branchNumber;	/* the branch (aka sequence) number */
	uint32		streamTime;		/* the current stream presentation time,
								 * = audioTime - clockOffset ***IF*** running */
	uint32		audioTime;		/* the current Audio time */
	uint32		clockOffset;	/* stream clock relative to audio clock */
	} DSClock;


/******************************************************************************
 *		Data buffer structures
 ******************************************************************************/

struct DSStreamCB;

/* This is the data buffer header structure managed by the stream parser.
 * The actual data buffer can be a different size for each stream, but it
 * must be fixed within a given stream. */
typedef struct DSDataBuf {
	struct DSDataBuf	*next;			/* for linking buffers into lists */
	int32				useCount;		/* count of subscribers using this buffer */
	uint8				*streamData;	/* address of the stream data buffer */
	uint8				*curDataPtr;	/* current posn, usually == streamData */
	} DSDataBuf, *DSDataBufPtr;


/* The following preamble is used for all types of messages sent by the
 * streamer to data acquisition and to subscribers. */
#define	DS_MSG_HEADER	\
	int32	whatToDo;		/* opcode determining msg contents */		\
	Item	msgItem;		/* message Item for sending this buffer */	\
	void	*privatePtr;	/* ptr to sender's private data */			\
	void	*link			/* user defined -- for linking msgs into lists */

typedef struct GenericMsg {
	DS_MSG_HEADER;
	} GenericMsg, *GenericMsgPtr;


/******************************************************************************
 *		Subscriber Message Interface (streamer to subscribers)
 ******************************************************************************/

/* --- Opcode values as passed in 'whatToDo' --- */
enum StreamOpcode {
	kStreamOpData		= 0,	/* here's a data chunk */
	kStreamOpGetChan	= 1,	/* get a logical channel's status */
	kStreamOpSetChan	= 2,	/* set a logical channel's status */
	kStreamOpControl	= 3,	/* perform a subscriber-defined function */
	kStreamOpSync		= 4,	/* clock stream resynched the clock */
	kStreamOpStart		= 5,	/* stream is being started */
	kStreamOpStop		= 6,	/* stream is being stopped */
	kStreamOpOpening	= 7,	/* initialize */
	kStreamOpClosing	= 8,	/* close-down and exit */
	kStreamOpEOF		= 9,	/* data EOF; no more data until branch or switch files */
	kStreamOpAbort		= 10,	/* error-case close-down and exit */
	kStreamOpBranch		= 11	/* branch in delivered data (not yet in the clock) */
	};


/* --- Messages sent from the streamer to subscribers --- */
typedef struct SubscriberMsg {
	DS_MSG_HEADER;

	union {
		struct {					/* kStreamOpData */
			void	*buffer;		/* ptr to the data chunk */
			uint32	branchNumber;	/* streamer increments this at each branch */
			} data;
	
		struct {					/* kStreamOpGetChan, kStreamOpSetChan */
			uint32	number;			/* channel number to operate on */
			uint32	status;			/* channel status (bits 31-16 are
						 			 * subscriber-defined) */
			uint32	mask;			/* determines which status bits to set */
			} channel;
	
		struct {					/* kStreamOpControl */
			int32	controlArg1;	/* subscriber-defined arg */
			void	*controlArg2;	/* subscriber-defined arg */
			} control;

		struct {					/* kStreamOpSync */
			uint32	clock;			/* current time */
			} sync;

		struct {					/* kStreamOpStart */
			uint32	options; 		/* start options (SOPT_NOFLUSH, SOPT_FLUSH) */
			} start;

		struct {					/* kStreamOpStop */
			uint32	options;		/* stop options (SOPT_NOFLUSH, SOPT_FLUSH) */
			} stop;

		struct {					/* kStreamOpBranch */
			uint32	options;		/* options: SOPT_FLUSH, SOPT_NEW_STREAM */
			uint32	branchNumber;	/* the new branch number */
			} branch;

		} msg;

	} SubscriberMsg, *SubscriberMsgPtr;


/* bits in status of 'channel' message above */
#define	CHAN_ENABLED	(1<<0)	/* R/W: %1 if channel enabled (allows data flow) */
#define	CHAN_ACTIVE		(1<<1)	/* R/O: %1 if channel data currently flowing */
#define	CHAN_EOF		(1<<2)	/* R/O: %1 if channel finished */
#define	CHAN_ABORTED	(1<<3)	/* R/O: %1 if channel aborted (error) */
#define	CHAN_SYSBITS	(0x0000FFFE)	/* Mask of reserved bits. The rest are
										 * subscriber-defined */
				/* NOTE: the least significant bit is R/W! */

/* bits in options of start and stop messages above */
#define	SOPT_NOFLUSH	(0)		/* don't flush, and no other bits on, either */
#define	SOPT_FLUSH		(1<<0)	/* %1 => flush in a start, stop, or branch request */

/* additional options bits for the branch message, above */
#define	SOPT_NEW_STREAM	(1<<1)	/* %1 => branching to a new stream */


/******************************************************************************
 * 		Data Acquisition Message Interface (streamer to data acq)
 ******************************************************************************/

/* --- Opcode values as passed in 'whatToDo' --- */
enum DataAcqOpcode {
	kAcqOpGetData		= 0,	/* fill buffer with data */
	kAcqOpGoMarker		= 1,	/* seek (typ. to a marker) in the stream */
	kAcqOpConnect		= 2,	/* stream connecting, initialize */
	kAcqOpDisconnect	= 3,	/* stream disconnecting */
	kAcqOpExit			= 4		/* release resources and exit */
	};

/* --- Messages sent from the streamer to data acquisition --- */
typedef struct DataAcqMsg {
	DS_MSG_HEADER;
	
	Item	dataAcqPort;	/* the Data Acq receiver of this msg */
	union {
		struct {						/* kAcqOpGetData */
			DSDataBufPtr	dsBufPtr;	/* ptr to DatBuf struct */
			uint32			bufferSize;	/* size of buffer in bytes */
			} data;
		
		struct {						/* kAcqOpGoMarker */
			uint32			value;		/* the branch value (its interpretation
										 * depends on the options field) */
			uint32			options;	/* branch options DataAcqMarkerOptions */
			uint32   		markerTime;	/* RETURNS: destination marker's stream
										 * time if GOMARKER_NEED_TIME_FLAG and if
										 * known, clearing GOMARKER_NEED_TIME_FLAG */
			} marker;
		
		struct {							/* kAcqOpConnect */
			struct DSStreamCB* streamCBPtr;	/* ptr to stream control block to
											 * connect to */
			uint32			bufferSize;	/* size of every buffer, in bytes */
			} connect;
		
		} msg;
	} DataAcqMsg, *DataAcqMsgPtr;

/* --- Values for DataAcqMsg.msg.marker.options --- */
enum DataAcqMarkerOptions {		/* msg.marker.value:  */
	/* pick one of these alternative branch types */
	GOMARKER_ABSOLUTE	= 0,	/* absolute destination file byte position [no marker table needed] */
	GOMARKER_FORWARD	= 1,	/* count of markers to advance from current time's marker */
	GOMARKER_BACKWARD	= 2,	/* count of markers to regress from current time's marker */
	GOMARKER_ABS_TIME	= 3,	/* absolute stream time of destination marker */
	GOMARKER_FORW_TIME	= 4,	/* stream time to add to current time for destination marker */
	GOMARKER_BACK_TIME	= 5,	/* stream time to subtract from current time for destination marker */
	GOMARKER_NAMED		= 6,	/* char* name of destination marker [UNIMPLEMENTED] */
	GOMARKER_NUMBER		= 7,	/* destination marker number */
	
	/* and optionally bitor in these flags */
	GOMARKER_NO_FLUSH_FLAG		= 1<<8,	/* Don't flush the subscribers.
		 * (Implemented by the streamer, not the data acq.) */
	GOMARKER_NEED_TIME_FLAG		= 1<<9,	/* need the destination time */
	
	/* (supporting definitions) */
	GOMARKER_BRANCH_TYPE_MASK	= 0xFF	/* bitmask to select the branch type */
	};


/******************************************************************************
 *		Data Streamer Message Interface (app to streamer)
 ******************************************************************************/

/* --- Opcode values as passed in 'whatToDo' --- */
enum DSRequestOpcode {
	kDSOpPreRollStream		= 0,	/* start filling buffers, wait for all but asyncBufferCnt of them */
	kDSOpCloseStream		= 1,	/* close the stream */
	kDSOpWaitEndOfStream	= 2,	/* register for EOS notification */
	kDSOpStartStream		= 3,	/* start data flowing */
	kDSOpStopStream			= 4,	/* stop data flowing */
	kDSOpSubscribe			= 5,	/* add/remove/replace a subscriber */
	kDSOpGoMarker			= 6,	/* seek (typ. to the marker) in the stream */
	kDSOpGetChannel			= 7,	/* get channel status */
	kDSOpSetChannel			= 8,	/* set channel status */
	kDSOpControl			= 9,	/* perform a subscriber-dependent function */
	kDSOpConnect			= 10	/* add/remove/replace a data acq thread */
	};

/* --- Messages sent from the app to the streamer --- */
typedef struct DSRequestMsg {
	DS_MSG_HEADER;

	union {
		struct {							/* kDSOpPreRollStream */
			uint32		asyncBufferCnt;		/* wait for all but this many
											 * buffers to be filled */
			} preroll;

		struct {							/* kDSOpSubscribe */
			DSDataType	dataType; 			/* 4-character code data type ID */
			Item		subscriberPort;		/* message port to send data to */
			} subscribe;

		struct {							/* kDSOpClockSync */
			DSDataType	exemptStream;		/* subscriber to NOT send sync msg to */
			uint32		nowTime;			/* time value to propagate */
			} clockSync;

		struct {							/* kDSOpGoMarker */
			uint32 		markerValue;		/* the branch value (its interpretation
											 * depends on the options field) */
			uint32		options;			/* branch options DataAcqMarkerOptions */
			} goMarker;

		struct {							/* kDSOpGetChannel */
			DSDataType	streamType;			/* which subscriber */
			uint32		channelNumber;		/* logical data channel number */
			uint32		*channelStatusPtr;	/* place to return channel status */
			} getChannel;

		struct {							/* kDSOpSetChannel */
			DSDataType	streamType;			/* which subscriber */
			uint32		channelNumber;		/* logical data channel number */
			uint32		channelStatus;		/* channel status bits to set */
			uint32		mask;				/* determines which status bits to set */
			} setChannel;

		struct {							/* kDSOpControl */
			DSDataType	streamType;			/* subscriber to control */
			int32		userDefinedOpcode;	/* subscriber-defined action code */
			void		*userDefinedArgPtr;	/* subscriber-defined argument */
			} control;

		struct {							/* kDSOpConnect */
			Item		acquirePort;		/* connect this data port to stream */
			} connect;

		struct {							/* kDSOpStartStream */
			uint32		options;			/* start options (e.g. SOPT_FLUSH) */
			} start;

		struct {							/* kDSOpStopStream */
			uint32		options;			/* stop options (e.g. SOPT_FLUSH) */
			} stop;

		} msg;

	} DSRequestMsg, *DSRequestMsgPtr;


/******************************************************************************
 *	Data Streamer private structures
 ******************************************************************************/

typedef struct DSStreamCB	DSStreamCB, *DSStreamCBPtr;


/******************************************************************************
 *	Procedural interface to the DataStream and DataAcq threads
 ******************************************************************************/

#ifdef __cplusplus 
extern "C" {
#endif


Item	NewDataStream(DSStreamCBPtr *pCtx, void *bufferListPtr,
			uint32 bufferSize, int32 deltaPriority, int32 numSubsMsgs);
Err		DisposeDataStream(Item msgItem, DSStreamCBPtr streamCBPtr);

Item	NewDataAcq(char *fileName, int32 deltaPriority);
Err		DisposeDataAcq(Item dataAcqMsgPort);

void	DSGetPresentationClock(DSStreamCBPtr streamCBPtr, DSClock *dsClock);
void	DSSetPresentationClock(DSStreamCBPtr streamCBPtr, uint32 branchNumber,
			uint32 streamTime);
bool	DSIsRunning(DSStreamCBPtr streamCBPtr);
Item	GetDataStreamMsgPort(DSStreamCBPtr streamCBPtr);


#ifdef __cplusplus
}
#endif

#endif	/* __STREAMING_DATASTREAM_H */
