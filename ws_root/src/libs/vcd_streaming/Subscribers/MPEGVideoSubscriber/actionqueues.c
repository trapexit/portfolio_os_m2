/******************************************************************************
**
**  @(#) actionqueues.c 96/06/24 1.2
**
******************************************************************************/

#include <string.h>
#include <stdlib.h>

#include <kernel/types.h>
#include <kernel/debug.h>

#include <video_cd_streaming/datastream.h>
#include <streaming/subscribertraceutils.h>
#include "mpegvideochannels.h"


/*******************************************************************************************
 * Initialize an ActionQueue
 *******************************************************************************************/
void InitActionQueue(ActionQueuePtr theQueue)
{
	theQueue->actionPool		= 0;
	theQueue->head				= NULL;
	theQueue->tail				= NULL;
}
	
/*******************************************************************************************
 * Add an action to our action queue
 *******************************************************************************************/
void AddActionToTail(ActionQueuePtr queue, ActionPtr action)
{

	if( queue->head != NULL ) {
		queue->tail->link = (void *)action;
		queue->tail = action;
		action->link = NULL;			/* make sure the tails is pointing to NULL */
	} else {
		action->link = NULL;
		queue->head = action;
		queue->tail = action;
	}
}

/*******************************************************************************************
 * Remove an action from the head of the queue of actions.
 *******************************************************************************************/
ActionPtr GetNextAction(ActionQueuePtr queue)
{
	ActionPtr	action;

	if ( (action = queue->head) != NULL ) {
		queue->head = (ActionPtr)action->link;
		if ( queue->head == action )	/* this means we reached end of the queue */
			queue->tail = NULL;	
	}
	
	return action;
}

/***********************************************************************
 * Clear a action queue for reuse and return it to the ActionQueue's pool.
 ***********************************************************************/
void ClearAndReturnAction(ActionQueuePtr queue, ActionPtr actionPtr)
{
	ReturnPoolMem(queue->actionPool, actionPtr);
}

/***********************************************************************
 * Clear a action queue for reuse and return it to the ActionQueue's pool.
 ***********************************************************************/
void ClearAndReturnAllActions(ActionQueuePtr queue)
{
	ActionPtr	action;
	
	if( queue )
		while( action = GetNextAction(queue) )
			ReturnPoolMem(queue->actionPool, action);
}

