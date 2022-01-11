/****************************************************************************
**
**  @(#) msgutils.h 96/03/04 1.6
**
*****************************************************************************/
#ifndef	__MSGUTILS_H__
#define __MSGUTILS_H__	


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_ITEM_H
#include <kernel/item.h>
#endif

#ifndef __KERNEL_MSGPORT_H
#include <kernel/msgport.h>
#endif


/*******************************************/
/* System related message helper functions */
/*******************************************/
#ifdef __cplusplus 
extern "C" {
#endif


Item	NewMsgPort(uint32 *signalMaskPtr);

Item	CreateMsgItem(Item replyPort);

bool	PollForMsg(Item msgPortItem, Item *msgItemPtr, Message* *pMsgPtr,
			void* *pMsgDataPtr, Err *status);


#ifdef __cplusplus
}
#endif

#endif	/* __MSGUTILS_H__ */
