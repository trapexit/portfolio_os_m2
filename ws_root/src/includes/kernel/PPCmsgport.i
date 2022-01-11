#ifndef __KERNEL_PPCMSGPORT_I
#define __KERNEL_PPCMSGPORT_I


/******************************************************************************
**
**  @(#) PPCmsgport.i 96/04/24 1.12
**
******************************************************************************/


#ifndef __KERNEL_PPCITEM_I
#include <kernel/PPCitem.i>
#endif

#ifndef __KERNEL_PPCLIST_I
#include <kernel/PPCList.i>
#endif

    .struct	sgPort
mp		.byte	ItemNode	// ItemNode mp
mp_Signal	.long	1		// uint32 mp_Signal	- what Owner needs to wake up
mp_Msgs		.byte	List		// List mp_Msgs		- Messages waiting for Owner
mp_UserData	.long	1		// void *mp_UserData	- User data pointer
    .ends

//  MsgPort flags

#define MSGPORT_SIGNAL_ALLOCATED	1

#define CREATEPORT_TAG_SIGNAL	TAG_ITEM_LAST+1	// use this signal
#define CREATEPORT_TAG_USERDATA	TAG_ITEM_LAST+2	// set MsgPort UserData pointer

    .struct	Message
msg		.byte	ItemNode	// ItemNode msg
msg_ReplyPort	.long	1		// Item msg_ReplyPort
msg_Result	.long	1		// uint32 msg_Result	- result from ReplyMsg
msg_DataPtr	.long	1		// void *msg_DataPtr	- ptr to beginning of data
msg_DataSize	.long	1		// int32 msg_DataSize	- size of data field
msg_Holder	.long	1		// Item msg_MsgPort	- MsgPort currently queued on
msg_DataPtrSize	.long	1		// uint32 msg_DataPtrSize - size of allocated data area
msg_SigItem	.long	1		// Item msg_SigItem	- Designated Signal Receiver
msg_UserData	.long	1
    .ends

//  specify a different size in the CreateItem call to get
//  pass by value message

#define MESSAGE_SENT		0x1	// msg sent and not replied
#define MESSAGE_REPLIED		0x2	// msg replied and not removed
#define MESSAGE_SMALL		0x4	// this is really a small value msg
#define MESSAGE_PASS_BY_VALUE	0x8	// copy data to msg buffer

#define CREATEMSG_TAG_REPLYPORT		TAG_ITEM_LAST+1
#define CREATEMSG_TAG_MSG_IS_SMALL	TAG_ITEM_LAST+2
#define CREATEMSG_TAG_DATA_SIZE		TAG_ITEM_LAST+3	// data area for pass by value


#endif /* __KERNEL_PPCMSGPORT_I */
