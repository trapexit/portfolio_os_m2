/******************************************************************************
**
**  @(#) msgutils.c 96/08/01 1.2
**
******************************************************************************/

#include <kernel/debug.h>
#include <kernel/msgport.h>
#include <streaming/msgutils.h>
#include <streaming/dserror.h>


Item	NewMsgPort(uint32 *signalMaskPtr)
	{
	Item		portItem;

	portItem = CreateMsgPort(NULL, 0, 0);

	if ( portItem >= 0 && signalMaskPtr != NULL )
		*signalMaskPtr = MSGPORT(portItem)->mp_Signal;

	return portItem;
	}


Item	CreateMsgItem(Item replyPort)
	{
	return CreateMsg(NULL, 0, replyPort);
	}


bool	PollForMsg(Item msgPortItem, Item *msgItemPtr, Message* *pMsgPtr,
					void* *pMsgDataPtr, Err *status)
	{
	Item		msgItem;
	Message		*msgPtr;

	/* Fetch the next incoming message. GetMsg() returns a positive Message
	 * Item number, 0 for no messages, or a negative Err code. */
	msgItem = GetMsg(msgPortItem);
	if ( msgItem <= 0 )			/* an error or no messages */
		{
		if ( status != NULL )
			*status = msgItem;
		return false;
		}

	/* Find the address of the message structure so we can get the message
	 * value from it. */
	msgPtr = MESSAGE(msgItem);
	if ( msgPtr == NULL )
		{
		*status = kDSBadItemErr;
		return false;
		}

	/* Optionally return:
	 *	- the message's item number,
	 *	- a pointer to the message structure,
	 *	- the message data  pointer. */
	if ( msgItemPtr != NULL )	*msgItemPtr		= msgItem;
	if ( pMsgPtr != NULL )		*pMsgPtr		= msgPtr;
	if ( pMsgDataPtr != NULL )	*pMsgDataPtr	= (void *)msgPtr->msg_DataPtr;

	/* Assert success */
	if ( status != NULL )
		*status = 0;
	return true;
	}

