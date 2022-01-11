/******************************************************************************
**
**  @(#) protochannels.h 95/09/13 1.3
**
******************************************************************************/
#ifndef __PROTOCHANNELS_H__
#define __PROTOCHANNELS_H__

#include <kernel/types.h>
#include <streaming/mempool.h>
#include <streaming/subscriberutils.h>

#include "protostreamdefs.h"


/* Max number of logical channels per subscription */
#define	PR_SUBS_MAX_CHANNELS		8		

/* Defines for dealing with channel status bits.
 *
 * Channel Enabled 	  - means that a channel is prepared to queue data msgs.
 *				 	    Channel 0 defaults to enabled, but all other channels
 *					    must be explicitly set with a DoSetChan() call.
 *
 * Channel Active  	  - means that a "stream started" msg has been received.
 *
 * Channel Initalized - means that a header chunk has arrived for this channel.
 */

/* Define a channel status flag to indicate wheather a channel
 * has been initalized.  
 * Bits 0-15 are reserved by the streamer; bit 0 (CHAN_ENABLED) is
 * R/W by the user with DoSetChan(), but bits 1-15 are R/O to the user.  
 * See datastream.h.
 */
#define	PROTO_CHAN_INITALIZED		(1<<16)				

/* Define a mask for subscriber reserved R/O bits.
 * Make sure that the user can't set/unset these bit(s)
 * using DoSetChan().  This mask is ORed with CHAN_SYSBITS
 * in the subscriber's implementation of DoSetChan().
 */
#define PROTO_CHAN_SUBSBITS			(0x10000)


#define IsProtoChanEnabled(x)		((x)->status & CHAN_ENABLED)		
#define IsProtoChanActive(x)		((x)->status & CHAN_ACTIVE)		
#define IsProtoChanInitalized(x)	((x)->status & PROTO_CHAN_INITALIZED)

/****************************************************************/
/* Subscriber logical channel, SA_SUBS_MAX_CHANNELS per context */
/****************************************************************/

typedef struct ProtoChannel {
	uint32			status;				/* state bits */
	SubsQueue		dataMsgQueue;		/* queued data chunks */
	} ProtoChannel, *ProtoChannelPtr;

/****************************************************************
 * We need the following forward references because the routines 
 * we prototype here will be called with pointers to structs 
 * that will be defined in protosubscriber.h 
 ****************************************************************/

struct ProtoContext;

/**********************************************/
/* Public function prototypes for this module */
/**********************************************/

#ifdef __cplusplus 
extern "C" {
#endif

int32  	InitProtoChannel( struct ProtoContext* ctx,
						  struct ProtoHeaderChunk* headerPtr );
int32	StartProtoChannel( struct ProtoContext* ctx, int32 channelNumber );
int32	StopProtoChannel( struct ProtoContext* ctx, int32 channelNumber );
int32	FlushProtoChannel( struct ProtoContext* ctx, int32 channelNumber );
int32	CloseProtoChannel( struct ProtoContext* ctx, int32 channelNumber );

int32	ProcessNewProtoDataChunk( struct ProtoContext* ctx,
								  SubscriberMsgPtr subMsg );

#ifdef __cplusplus
} 
#endif

#endif	/* __PROTOCHANNELS_H__ */
