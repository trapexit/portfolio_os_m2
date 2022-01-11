/******************************************************************************
**
**  @(#) subscriberutils.c 96/08/01 1.2
**
******************************************************************************/

#include <streaming/subscriberutils.h>
#include <kernel/debug.h>


bool AddDataMsgToTail(SubsQueuePtr queue, SubscriberMsgPtr subMsg)
	{
	bool		fQueueWasEmpty;

	subMsg->link = NULL;

	if ( queue->head != NULL )
		{
		/* Add the new message onto the end of the existing list of queued
		 * messages. */
		queue->tail->link = (void *) subMsg;
		queue->tail = subMsg;
		fQueueWasEmpty = FALSE;
		}
	else
		{
		/* Add the message to an empty queue  */
		queue->head = subMsg;
		queue->tail = subMsg;
		fQueueWasEmpty = TRUE;
		}

	return fQueueWasEmpty;
	}


SubscriberMsgPtr GetNextDataMsg(SubsQueuePtr queue)
	{
	SubscriberMsgPtr	subMsg;

	if ( (subMsg = queue->head) != NULL )
		{
		/* Advance the head pointer to point to the next entry in the list. */
		queue->head = (SubscriberMsgPtr) subMsg->link;

		/* If we are removing the tail entry, clear the tail pointer. */
		if ( queue->tail == subMsg )
			queue->tail = NULL;
		}

	return subMsg;
	}


Err ReplyToSubscriberMsg(SubscriberMsgPtr subMsgPtr, Err status)
	{
	Err		result;

#ifdef DEBUG
	if ( subMsgPtr == NULL )
		{
		PERR(("ReplyToSubscriberMsg: NULL ptr\n"));
		DebugBreakpoint();
		return kDSBadPtrErr;
		}
#endif

	result = ReplyMsg(subMsgPtr->msgItem, status, subMsgPtr,
		sizeof(SubscriberMsg));
	CHECK_NEG("ReplyToSubscriberMsg", result);
	return result;
	}
