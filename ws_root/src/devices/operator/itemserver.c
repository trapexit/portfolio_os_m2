/* @(#) itemserver.c 96/08/01 1.28 */

#include <kernel/types.h>
#include <kernel/msgport.h>
#include <kernel/mem.h>
#include <kernel/io.h>
#include <kernel/debug.h>
#include <kernel/operror.h>
#include <kernel/super.h>
#include <kernel/kernel.h>
#include <stdio.h>
#include <string.h>


/*****************************************************************************/


typedef (* CallBackFunc)(void *);

static void ItemServer(Message *msg)
{
int32        result;
CallBackFunc func;

    func = (CallBackFunc)msg->msg_Val1;
    result = (*func)((void *)msg->msg_Val2);
    ReplySmallMsg(msg->msg.n_Item, result, 0, 0);
}


/*****************************************************************************/


void ProvideItemServices(Item serverPort)
{
Item       msgItem;
Message   *msg;
Err        result;

    while (TRUE)
    {
        /* extract a request from the server port */

        msgItem = GetMsg(serverPort);
        if (msgItem <= 0)
            return;

        /* validate the message */

        msg = (Message *)LookupItem(msgItem);
        if (msg->msg.n_ItemFlags & ITEMNODE_PRIVILEGED)
        {
            result = CreateThreadVA(ItemServer, "Item Server", 0, 4096,
                          CREATETASK_TAG_ARGC, msg,
                          (CURRENTTASK->t.n_ItemFlags & ITEMNODE_PRIVILEGED ? CREATETASK_TAG_PRIVILEGED : TAG_NOP), 0,
                          (CURRENTTASK->t.n_ItemFlags & ITEMNODE_PRIVILEGED ? CREATETASK_TAG_SINGLE_STACK : TAG_NOP), 0,
                          TAG_END);
        }
        else
        {
            /* message wasn't sent by kernel, something smells fishy... */
            result = BADPRIV;
        }

        if (result < 0)
            ReplyMsg(msgItem, result, NULL, 0);
    }
}
