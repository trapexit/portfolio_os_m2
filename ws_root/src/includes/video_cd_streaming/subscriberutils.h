/****************************************************************************
**
**  @(#) subscriberutils.h 96/03/04 1.11
**
*****************************************************************************/
#ifndef __STREAMING_SUBSCRIBERUTILS_H
#define __STREAMING_SUBSCRIBERUTILS_H


#ifndef __STREAMING_DATASTREAMLIB_H
#include <video_cd_streaming/datastreamlib.h>
#endif

#ifndef __STREAMING_DSSTREAMDEFS_H
#include <video_cd_streaming/dsstreamdefs.h>
#endif

/************************************************************/
/* General subscriber message queue structure with support	*/
/* for fast queuing of entries in a singly linked list.		*/
/************************************************************/
typedef struct SubsQueue {
	SubscriberMsgPtr	head;			/* head of message queue */
	SubscriberMsgPtr	tail;			/* tail of message queue */
	} SubsQueue, *SubsQueuePtr;


/************************************************/
/* Channel context, one per channel, per stream	*/
/************************************************/
typedef struct SubsChannel {
	uint32				status;			/* state bits */
	SubsQueue			msgQueue;		/* queue of subscriber messages */
	} SubsChannel, *SubsChannelPtr;


/*****************************/
/* Public routine prototypes */
/*****************************/
#ifdef __cplusplus
extern "C" {
#endif

bool AddDataMsgToTail(SubsQueuePtr subsQueue, SubscriberMsgPtr subMsg);
SubscriberMsgPtr GetNextDataMsg(SubsQueuePtr subsQueue);
Err ReplyToSubscriberMsg(SubscriberMsgPtr subMsgPtr, Err status);

#ifdef __cplusplus
}
#endif

#endif	/* __STREAMING_SUBSCRIBERUTILS_H */

