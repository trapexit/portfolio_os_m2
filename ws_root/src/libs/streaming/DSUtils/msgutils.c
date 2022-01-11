/******************************************************************************
**
**  @(#) msgutils.c 96/04/08 1.16
**
******************************************************************************/

#include <kernel/debug.h>
#include <kernel/msgport.h>
#include <streaming/msgutils.h>
#include <streaming/dserror.h>


/******************************************************************************
|||	AUTODOC -public -class Streaming -group DSUtils -name NewMsgPort
|||	Creates an unnamed message port.
|||	
|||	  Synopsis
|||	
|||	    Item NewMsgPort(uint32 *signalMaskPtr)
|||	
|||	  Description
|||	
|||	    NewMsgPort() creates a message port and a signal bit for it. This
|||	    returns the message port's Item number and optionally also returns
|||	    its signal mask by storing into *signalMaskPtr. This is a convenience
|||	    routine used in the streaming library.
|||	    
|||	    Call DeleteMsgPort() to delete the message port Item and its associated
|||	    signal bit.
|||	
|||	  Arguments
|||	
|||	    signalMaskPtr
|||	        Address to store the new message port's signal mask, or NULL if
|||	        you don't need this information.
|||	
|||	  Return Value
|||	
|||	    Message port Item number or a negative error code.
|||	
|||	  Implementation
|||	
|||	    Streaming library call.
|||	
|||	  Associated Files
|||	
|||	    <streaming/msgutils.h>, libds.a
|||	
|||	  See Also
|||	
|||	    CreateMsgPort(), DeleteMsgPort()
|||	
 ******************************************************************************/
Item	NewMsgPort(uint32 *signalMaskPtr)
	{
	Item		portItem;
	
	portItem = CreateMsgPort(NULL, 0, 0);

	if ( portItem >= 0 && signalMaskPtr != NULL )
		*signalMaskPtr = MSGPORT(portItem)->mp_Signal;

	return portItem;
	}


/******************************************************************************
|||	AUTODOC -public -class Streaming -group DSUtils -name CreateMsgItem
|||	Creates a message Item.
|||	
|||	  Synopsis
|||	
|||	    Item CreateMsgItem(Item replyPort)
|||	
|||	  Description
|||	
|||	    This function creates an unnamed message Item with the default
|||	    priority. It's used by the data streaming modules as a convenience
|||	    function for CreateMsg().
|||	
|||	    You can provide an optional reply port. Any task you send the message
|||	    to will be able to reply it to you by calling ReplyMsg(). If you don't
|||	    supply a reply port (i.e. if replyPort == 0) then the act of sending
|||	    the message to another task will also transfer the ownership of the
|||	    message to the receiving task.
|||	
|||	  Arguments
|||	
|||	    replyPort
|||	        The Item number of the message port at which to receive the
|||	        reply, or 0 for no reply port.
|||	
|||	  Return Value
|||	
|||	    The item number of the message or a negative error code for
|||	    failure.
|||	
|||	  Implementation
|||	
|||	    Streaming library call.
|||	
|||	  Associated Files
|||	
|||	    <streaming/msgutils.h>, libds.a
|||	
|||	  See Also
|||	
|||	    CreateMsg(), DeleteMsg()
|||	
 ******************************************************************************/
Item	CreateMsgItem(Item replyPort)
	{
	return CreateMsg(NULL, 0, replyPort);
	}


/******************************************************************************
|||	AUTODOC -public -class Streaming -group DSUtils -name PollForMsg
|||	Retrieves the next message from the specified message port.
|||	
|||	  Synopsis
|||	
|||	    bool PollForMsg(Item msgPortItem, Item *msgItemPtr, Message* *pMsgPtr,
|||	         void* *pMsgDataPtr, Err *status)
|||	
|||	  Description
|||	
|||	    PollForMsg() calls GetMsg() to retrieve the next message waiting at
|||	    the given message port, and returns its Message address and Message
|||	    data pointer. PollForMsg() returns immediately if there is no message
|||	    waiting.
|||	    
|||	    PollForMsg() is just a convenience routine. One could directly call
|||	    msgItem = GetMsg(msgPortItem) and if ( msgItem > 0 ), get
|||	    MESSAGE(msgItem)->msg_DataPtr.
|||	
|||	  Arguments
|||	
|||	    msgPortItem
|||	        Item number of the message port to look for messages.
|||	
|||	    msgItemPtr
|||	        Address to store the received message's Item number, if any.
|||	        Pass in NULL if you don't need this information.
|||	
|||	    pMsgPtr
|||	        Address to store the received message's Message address.
|||	        Pass in NULL if you don't need this information.
|||	
|||	    pMsgDataPtr
|||	        Address to store the received message's data address.
|||	        Pass in NULL if you don't need this information.
|||	
|||	    status
|||	        Address to store the resulting status Portfolio error code.
|||	        Pass in NULL if you don't need this information.
|||	
|||	  Return Value
|||	
|||	    TRUE if a message was successfully received.
|||	    FALSE if there was no message waiting or an error occurred.
|||	
|||	  Implementation
|||	
|||	    Streaming library call.
|||	
|||	  Associated Files
|||	
|||	    <streaming/msgutils.h>, libds.a
|||	
|||	  See Also
|||	
|||	    GetMsg()
|||	
 ******************************************************************************/
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

