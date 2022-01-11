/******************************************************************************
**
**  @(#) joinsubscriber.h 95/09/20 1.7
**
**  NOTE: This "Join Subscriber" is out-of-date. But rather than update it,
**        we'll probably replace it with a "Data Subscriber".
**
******************************************************************************/
#ifndef __JOINSUBSCRIBER_H__
#define __JOINSUBSCRIBER_H__

#include <streaming/dsstreamdefs.h>			/* for SUBS_CHUNK_COMMON constants */
#include <streaming/datastreamlib.h>
#include <streaming/subscriberutils.h>

#include "joinstreamdefs.h"


/**********************/
/* Internal constants */
/**********************/

/* max # of streams that can use this subscriber */
#define	DATA_MAX_SUBSCRIPTIONS		2

/* max # of data elements per subscriber */
#define	DATA_MAX_ELEMENTS			64

/* max # of different data types supported */
#define	MAX_PORTS					8

/* flag bits for the join element message */ 
#define JF_RELEASEMEMORY		0x0000	/* the default */
#define JF_RELEASERESOURCESONLY	0x0001	/* release everything but the
										 * data block */


/************************************************/
/* Channel context, one per channel, per stream */
/************************************************/

typedef struct JoinElementMsg {
	DS_MSG_HEADER;
	int32	dataType;	/* 4 character data type (e.g. ANIM) */
	int32	dataChannel; /* entities within dataTypes are identified by
						  * channel */
	void*	dataPtr;	/* pointer to local mem containing assembled data */
	int32	dataSize;	/* size of final assembled data */
	int32	dataOffset;	/* current offset into the dataPtr. Put next
						 * chunk here. */
	int32	dataTime;	/* the stream time contained in the first join chunk */
	int32	flags;
	} JoinElementMsg, *JoinElementMsgPtr;


/**************************************/
/* Subscriber context, one per stream */
/**************************************/

typedef struct JoinContext {
	Item		creatorTask;		/* who to signal when we're done initializing */
	uint32		creatorSignal;		/* signal to send for synchronous completion */
	int32		creatorStatus;		/* result code for creator */

	Item		threadItem;			/* subscriber thread item */

	Item		requestPort;		/* message port item for subscriber requests */
	uint32		requestPortSignal;	/* signal to detect request port messages */

	MemPoolPtr	joinElemMsgPool;	/* Pool of msgs for sending data
									 * elements to display task */

	Item		portListSem;		/* semaphore to arbitrate access to
									 * the dataPort info */

	Item	dataPort[MAX_PORTS];	/* message port item to send data elements */
	DSDataType dataType[MAX_PORTS]; /* message type for this port */
	Boolean	userPort[MAX_PORTS];	/* true if client allocated the port
									 * for InitJoinPort */
	int32		numPorts;			/* number of ports currently in use */
	
	Item		replyPort;			/* message port item for subscriber requests */
	uint32		replyPortSignal;	/* signal to detect request port messages */

	int32		localTimeOrigin;	/* local version of the time */

	JoinElementMsgPtr dataMsgHead;	/* ptr to a list of data that is not yet full */

	Boolean		streamStopped;		/* TRUE if kOpStopStream has been sent */

	JoinElementMsgPtr JoinElemMsgWaitingPtr;
									/* Unfinish message */
	SubscriberMsgPtr psubMsgWaiting; /* Info to finish the unfinished message */
	Boolean 	bRequestSignal;		/* Boolean indicating a request
									 * needs to be processed */

	} JoinContext, *JoinContextPtr;

/*****************************/
/* Public routine prototypes */
/*****************************/
#ifdef __cplusplus 
extern "C" {
#endif

	/* Subscriber one-time init/shutdown routines */

int32	InitJoinSubscriber( void );
int32	CloseJoinSubscriber( void );

	/* New/dispose subscriber instance */

int32	NewJoinSubscriber( JoinContextPtr *pCtx, int32 priority );
int32	DisposeJoinSubscriber( JoinContextPtr ctx );

	/* Data port create/destroy */

int32	InitJoinPort( JoinContextPtr ctx, Item *joinPort, int32 dataType );
void	DestroyJoinPort( JoinContextPtr ctx, DSDataType dataType);

	/* Get/release data element */

JoinElementMsgPtr GetJoinElement( JoinContextPtr ctx, Item joinPort );
void ReleaseJoinElement( JoinContextPtr ctx, JoinElementMsgPtr joinElemPtr );
void ReleaseJoinElementResources( JoinContextPtr ctx,
								  JoinElementMsgPtr joinElemPtr );


	/* Miscellaneous */

int32	FlushJoinSubscriber( JoinContextPtr ctx );

#ifdef __cplusplus
}
#endif

#endif	/* __JOINSUBSCRIBER_H__ */
