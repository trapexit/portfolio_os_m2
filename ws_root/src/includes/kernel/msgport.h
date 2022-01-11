#ifndef __KERNEL_MSGPORT_H
#define __KERNEL_MSGPORT_H


/******************************************************************************
**
**  @(#) msgport.h 96/01/02 1.18
**
**  Kernel messaging system management
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_NODES_H
#include <kernel/nodes.h>
#endif

#ifndef __KERNEL_KERNELNODES_H
#include <kernel/kernelnodes.h>
#endif

#ifndef __KERNEL_LIST_H
#include <kernel/list.h>
#endif

#ifndef __KERNEL_ITEM_H
#include <kernel/item.h>
#endif


/*****************************************************************************/


typedef struct MsgPort
{
    ItemNode  mp;
    int32     mp_Signal;     /* signal the owner needs to wake up */
    List      mp_Msgs;       /* messages received and not gotten  */
    void     *mp_UserData;   /* user-private data                 */
} MsgPort;

/* mp.n_Flags */
#define MSGPORT_SIGNAL_ALLOCATED 1 /* signal auto-allocated when port created */

enum msgport_tags
{
    CREATEPORT_TAG_SIGNAL = TAG_ITEM_LAST+1,  /* use this signal             */
    CREATEPORT_TAG_USERDATA                   /* value to put in mp_UserData */
};

/* convenient way to access a MsgPort structure starting from its item number */
#define MSGPORT(msgPortItem) ((MsgPort *)LookupItem(msgPortItem))


/*****************************************************************************/


typedef struct Message
{
    ItemNode    msg;
    Item        msg_ReplyPort;  /* port to reply to, or 0 for none     */
    int32       msg_Result;	/* result from ReplyMsg()              */

    union
    {
        void   *msg_DataPtr;      /* ptr to beginning of data            */
        uint32  msg_Val1;         /* or val1 for small messages          */
    } msg_Data1;

    union
    {
        int32   msg_DataSize;     /* size of data field                  */
        uint32  msg_Val2;         /* or val2 for small messages          */
    } msg_Data2;

    Item        msg_Holder;	  /* port or task currently queued on    */
    uint32      msg_DataPtrSize;  /* size of allocated data area         */
    void       *msg_UserData;     /* user-private data                   */
} Message;

#define msg_DataPtr  msg_Data1.msg_DataPtr
#define msg_Val1     msg_Data1.msg_Val1
#define msg_DataSize msg_Data2.msg_DataSize
#define msg_Val2     msg_Data2.msg_Val2

#ifndef EXTERNAL_RELEASE
#define Msg Message
#endif /* EXTERNAL_RELEASE */

/* msg.n_Flags */
#define MESSAGE_SENT          0x01   /* msg sent and not replied         */
#define MESSAGE_REPLIED       0x02   /* msg replied and not gotten       */
#define MESSAGE_SMALL         0x04   /* this is really a small value msg */
#define MESSAGE_PASS_BY_VALUE 0x08   /* copy data to msg buffer          */

enum message_tags
{
    CREATEMSG_TAG_REPLYPORT=TAG_ITEM_LAST+1, /* reply port for message          */
    CREATEMSG_TAG_MSG_IS_SMALL,              /* create a small message          */
    CREATEMSG_TAG_DATA_SIZE,                 /* max data size for pass by value */
    CREATEMSG_TAG_USERDATA                   /* value to put in msg_UserData    */
};

/* convenient way to access a Message structure starting from its item number */
#define MESSAGE(msgItem) ((Message *)LookupItem(msgItem))


/*****************************************************************************/


#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */


extern Item CreateMsgPort(const char *name, uint8 pri, int32 sigMask);
extern Item CreateUniqueMsgPort(const char *name, uint8 pri, int32 sigMask);

extern Item CreateMsg(const char *name, uint8 pri, Item msgPort);
extern Item CreateSmallMsg(const char *name, uint8 pri, Item msgPort);
extern Item CreateBufferedMsg(const char *name, uint8 pri, Item msgPort, int32 dataSize);

extern Err  SendMsg(Item msgPort, Item msg, const void *dataPtr, int32 dataSize);
extern Err  SendSmallMsg(Item msgPort, Item msg, uint32 val1, uint32 val2);
extern Item GetMsg(Item msgPort);
extern Item GetThisMsg(Item msg);
extern Item WaitPort(Item msgPort, Item msg);
extern Err  ReplyMsg(Item msg, int32 result, const void *dataPtr, int32 dataSize);
extern Err  ReplySmallMsg(Item msg, int32 result, uint32 val1, uint32 val2);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#define DeleteMsgPort(msgPort) DeleteItem((msgPort))
#define DeleteMsg(msg)	       DeleteItem((msg))
#define FindMsgPort(name)      FindNamedItem(MKNODEID(KERNELNODE,MSGPORTNODE),(name))


/*****************************************************************************/


#endif	/* __KERNEL_MSGPORT_H */
