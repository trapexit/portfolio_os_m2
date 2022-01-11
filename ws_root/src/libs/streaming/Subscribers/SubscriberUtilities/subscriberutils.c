/******************************************************************************
**
**  @(#) subscriberutils.c 96/04/30 1.11
**
******************************************************************************/

#include <streaming/subscriberutils.h>
#include <kernel/debug.h>


/******************************************************************************
|||	AUTODOC -private -class Streaming -group SubscriberUtils -name AddDataMsgToTail
|||	Adds a data message to the tail of a subscriber queue.
|||
|||	  Synopsis
|||
|||	    bool AddDataMsgToTail(SubsQueuePtr queue, SubscriberMsgPtr subMsg)
|||
|||	  Description
|||
|||	    Utility routine to add a data message to the tail of a subscriber
|||	    data message queue.
|||
|||	  Arguments
|||
|||	    SubsQueuePtr
|||	        Pointer to the data message queue.
|||
|||	    SubscriberMsgPtr
|||	        Pointer to the data message to enqueue.
|||
|||	  Return Value
|||
|||	    TRUE if the queue was empty, else FALSE.
|||
|||	  Implementation
|||
|||	    Streaming library call.
|||
|||	  Associated Files
|||
|||	    <streaming/subscriberutils.h>, libsubscriber.a
|||
|||	  See Also
|||
|||	    GetNextDataMsg()
|||
******************************************************************************/
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


/******************************************************************************
|||	AUTODOC -private -class Streaming -group SubscriberUtils -name GetNextDataMsg
|||	Removes a data message from the head of a subscriber queue.
|||
|||	  Synopsis
|||
|||	    bool GetNextDataMsg(SubsQueuePtr queue)
|||
|||	  Description
|||
|||	    Utility routine to remove a data message from the head of a subscriber
|||	    data message queue.
|||
|||	  Arguments
|||
|||	    SubsQueuePtr
|||	        Pointer to the data message queue.
|||
|||	  Return Value
|||
|||	    SubscriberMsgPtr
|||	        Pointer of the data message removed from the head of the queue,
|||	        or NULL if the queue is empty.
|||
|||	  Implementation
|||
|||	    Streaming library call.
|||
|||	  Associated Files
|||
|||	    <streaming/subscriberutils.h>, libsubscriber.a
|||
|||	  See Also
|||
|||	    AddDataMsgToTail()
|||
******************************************************************************/
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


/******************************************************************************
|||	AUTODOC -private -class Streaming -group SubscriberUtils -name ReplyToSubscriberMsg
|||	ReplyMsg() to a SubscriberMsg.
|||
|||	  Synopsis
|||
|||	    Err ReplyToSubscriberMsg(SubscriberMsgPtr subMsgPtr, Err status)
|||
|||	  Description
|||
|||	    This utility routine calls ReplyMsg() to return a SubscriberMsg to
|||	    the thread that sent it (which is generally the Data Stream Thread).
|||	    This utility routine shares some common code, makes the caller's job
|||	    a little easier, and ensures error checking gets done in development
|||	    versions.
|||
|||	  Arguments
|||
|||	    subMsgPtr
|||	        Pointer to the SubscriberMsg to return. (It contains the Message
|||	        Item.)
|||
|||	    status
|||	        The error code number to return as the status of this subscriber
|||	        request message.
|||
|||	  Return Value
|||
|||	    Portfolio error code from ReplyMsg().
|||
|||	  Implementation
|||
|||	    Streaming library call.
|||
|||	  Associated Files
|||
|||	    <streaming/subscriberutils.h>, libsubscriber.a
|||
******************************************************************************/
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
