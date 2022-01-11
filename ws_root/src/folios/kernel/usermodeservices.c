/* @(#) usermodeservices.c 96/05/23 1.23 */

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/msgport.h>
#include <kernel/kernel.h>
#include <kernel/operror.h>
#include <kernel/usermodeservices.h>
#include <kernel/internalf.h>
#include <kernel/super.h>
#include <string.h>
#include <stdio.h>


/*****************************************************************************/


#define DBUG(x)	/* printf x */
#define DBUGIS(x) /* printf x */


/*****************************************************************************/


Err CallAsItemServer(void *func, void *arg, bool privileged)
{
Item    msgItem;
Msg    *msg;
Item    replyPort;
TagArg  tags[3];
Err     result;
Item    serverPort;
char   *portName;

    DBUGIS(("CallAsItemServer: func = 0x%x, priv = %d\n", func, privileged ));
    if (privileged)
    {
        portName = "PrivilegedItemServer";
    }
    else
    {
        portName = "ItemServer";
    }

    DBUGIS(("CallAsItemServer: portName = %s\n", portName ));

    serverPort = result = FindMsgPort(portName);
    DBUGIS(("CallAsItemServer: serverPort = 0x%x\n", serverPort ));
    if (serverPort >= 0)
    {
        replyPort = result = SuperCreateItem(MKNODEID(KERNELNODE,MSGPORTNODE),NULL);
        if (replyPort >= 0)
        {
            tags[0].ta_Tag = CREATEMSG_TAG_REPLYPORT;
            tags[0].ta_Arg = (void *)replyPort;
            tags[1].ta_Tag = CREATEMSG_TAG_MSG_IS_SMALL;
            tags[2].ta_Tag = TAG_END;
            msgItem = result = CreateItem(MKNODEID(KERNELNODE,MESSAGENODE),tags);
            if (msgItem >= 0)
            {
                msg = (Msg *)LookupItem(msgItem);

                /* tell the recipient that it really came from the kernel... */
                msg->msg.n_ItemFlags |= ITEMNODE_PRIVILEGED;

                result = SendSmallMsg(serverPort, msgItem, (uint32)func, (uint32)arg);
                if (result >= 0)
                {
                    result = WaitPort(replyPort, msgItem);
                    if (result >= 0)
                        result = msg->msg_Result;
                }
                DeleteItem(msgItem);
            }
            DeleteItem(replyPort);
        }
    }

    DBUGIS(("CallAsItemServer returns %x\n", result));

    return (result);
}
