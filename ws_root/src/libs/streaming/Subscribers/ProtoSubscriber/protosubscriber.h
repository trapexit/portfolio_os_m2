/******************************************************************************
**
**  @(#) protosubscriber.h 95/08/15 1.7
**
******************************************************************************/
#ifndef __PROTOSUBSCRIBER_H__
#define __PROTOSUBSCRIBER_H__

#include <streaming/datastream.h>
#include <streaming/subscriberutils.h>
#include <streaming/protochannels.h>
#include <streaming/dsstreamdefs.h>


/*************/
/* Constants */
/*************/

/* Max # of streams that can use this subscriber */
#define	PR_SUBS_MAX_SUBSCRIPTIONS	4	

/* Stream version supported by this subscriber.  Used for sanity 
 * checking when a header chunk is received.
 */
#define PROTO_STREAM_VERSION		0	

/**************************************/
/* Subscriber context, one per stream */
/**************************************/

typedef struct ProtoContext {
	Item			creatorTask;		/* who to signal when we're done												 * initializing */
	uint32			creatorSignal;		/* signal to send for synchronous completion */
	int32			creatorStatus;		/* result code for creator */

	Item			threadItem;			/* subscriber thread item */

	Item			requestPort;		/* message port item for
										 * subscriber requests */
	uint32			requestPortSignal;	/* signal to detect request port messages */

	DSStreamCBPtr	streamCBPtr;		/* stream this subscriber belongs to */
			 
	ProtoChannel	channel[PR_SUBS_MAX_CHANNELS];
										/* an array of channels */

	} ProtoContext, *ProtoContextPtr;


/***********************************************/
/* Opcode values as passed in control messages */
/***********************************************/
enum ProtoControlOpcode {
		kProtoCtlOpTest = 0
		};

/**************************************/
/* A Control block 					  */
/**************************************/
typedef union ProtoCtlBlock {

		struct {					/* kProtoCtlOpTest */
			int32	channelNumber;	/* Logical channel to send value to */
			int32	aValue;			/* test value */
			} test;

	} ProtoCtlBlock, *ProtoCtlBlockPtr;


/*****************************/
/* Public routine prototypes */
/*****************************/
#ifdef __cplusplus 
extern "C" {
#endif

int32	InitProtoSubscriber( void );
int32	CloseProtoSubscriber( void );

int32	NewProtoSubscriber( ProtoContextPtr *pCtx, DSStreamCBPtr streamCBPtr, 
							 int32 deltaPriority );
int32	DisposeProtoSubscriber( ProtoContextPtr ctx );

#ifdef __cplusplus
}
#endif

#endif	/* __PROTOSUBSCRIBER_H__ */

