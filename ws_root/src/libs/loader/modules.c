/* @(#) modules.c 96/10/05 1.59 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <loader/loader3do.h>
#include <loader/elf.h>
#include <loader/elf_3do.h>
#include <kernel/internalf.h>
#include <kernel/super.h>
#include <kernel/kernel.h>
#include <kernel/monitor.h>
#include <kernel/usermodeservices.h>
#include <file/filefunctions.h>
#include <stdio.h>
#include <string.h>
#include "daemon.h"
#include "modules.h"


/*****************************************************************************/


#define TRACE(x) /*printf x*/


/*****************************************************************************/


static int32 TagCallBack(Module *m, void *dummy, uint32 tag, uint32 arg)
{
    TOUCH(dummy);

    switch (tag)
    {
        case CREATEMODULE_TAG_LOADERINFO: m->li = (LoaderInfo *)arg;
                                          break;

        default                         : return BADTAG;
    }

    return 0;
}


/*****************************************************************************/


Item internalCreateModule(Module *m, TagArg *args)
{
Err            result;
_3DOBinHeader *binHdr;

    TRACE(("CreateModule: entering with module %x, current task %s\n", m, CURRENTTASK ? CURRENTTASK->t.n_Name : "NULL"));

    if (CURRENTTASK)
    {
        if (!IsPriv(CURRENTTASK))
            return LOADER_ERR_BADPRIV;
    }

    result = TagProcessor(m, args, TagCallBack, NULL);
    if (result >= 0)
    {
        binHdr          = m->li->header3DO;
        m->n.n_Version  = binHdr->_3DO_Item.n_Version;
        m->n.n_Revision = binHdr->_3DO_Item.n_Revision;

        if (binHdr->_3DO_Flags & _3DO_PRIVILEGE)
            m->n.n_ItemFlags |= ITEMNODE_PRIVILEGED;

        AddTail(&KB_FIELD(kb_Modules), (Node *) m);

        if (binHdr->_3DO_Flags & _3DO_SHOWINFO)
            internalPrint3DOHeader(m->n.n_Item, NULL, NULL);

#ifdef BUILD_DEBUGGER
	{
	    uint32         oldints;

	    oldints = Disable();
            Dbgr_ModuleCreated(m);
	    Enable(oldints);
	}
#endif

        result = m->n.n_Item;
    }

    TRACE(("CreateModule: returning %d\n", m->n.n_Item));

    return result;
}


/*****************************************************************************/


Err internalDeleteModule(Module *m, Task *t)
{
    TOUCH(t);

    TRACE(("DeleteModule: entering with module %s, task %s\n", m->n.n_Name, t->t.n_Name));

    RemNode((Node *)m);

#ifdef BUILD_DEBUGGER
    {
	uint32         oldints;

	oldints = Disable();
	Dbgr_ModuleDeleted(m);
	Enable(oldints);
    }
#endif

    return 0;
}


/*****************************************************************************/


Item internalOpenModule(Module *m, void *a, Task *t)
{
Err result;

    TOUCH(a);

    TRACE(("OpenModule: module %s, open count %d, task %s\n", m->n.n_Name, m->n.n_OpenCount, t ? t->t.n_Name : "NULL"));

    if (CURRENTTASK)
    {
        if (!IsPriv(CURRENTTASK))
            return LOADER_ERR_BADPRIV;

        result = 0;
        if (m->li->dataBase)
        {
            /* A task that opens an unsigned module gets write access to its
             * data section. The task also gets write permission if it is
             * privileged, or if the module being opened is the one being used
             * for the Task.
             */
            if (((m->li->header3DO->_3DO_Flags & _3DO_SIGNED) == 0)
             || (t->t.n_ItemFlags & ITEMNODE_PRIVILEGED)
             || (t->t_Module == m->n.n_Item))
            {
                TRACE(("OpenModule: grating write access to range [%x..%x] to task %s\n",
                       m->li->dataBase, (uint32)m->li->dataBase + m->li->dataLength + m->li->bssLength - 1,
                       t->t.n_Name));

                result = externalControlMem(m->li->dataBase, m->li->dataLength + m->li->bssLength,
                                            MEMC_OKWRITE, t->t.n_Item);
            }
        }

        if (result >= 0)
            result = m->n.n_Item;
    }
    else
    {
        /* the operator can just slide on by... */
        result = m->n.n_Item;
    }

    TRACE(("OpenModule: module %s, returning %d\n", m->n.n_Name, result));

    return result;
}


/*****************************************************************************/


Err internalCloseModule(Module *m, Task *t)
{
int32   i;
uint32  cnt;
Node   *n;
Task   *task;
Err     result;

    TRACE(("CloseModule: entering with module %s, task %s\n", m->n.n_Name, t->t.n_Name));

    if (!IsPriv(CURRENTTASK))
    {
        if ((CURRENTTASK->t_Flags & TASK_EXITING) == 0)
            return LOADER_ERR_BADPRIV;
    }

    result = 0;

    if (m->li->dataBase)
    {
        if ((m->li->header3DO->_3DO_Flags & _3DO_SIGNED) == 0)
        {
            /* see how many times the task family has got this module opened */
            cnt = 0;
            ScanList(&KB_FIELD(kb_Tasks), n, Node)
            {
                task = Task_Addr(n);
                if (IsSameTaskFamily(task,t))
                {
                    for (i = 0; i < task->t_ResourceCnt; i++)
                    {
                        if (task->t_ResourceTable[i] == (m->n.n_Item | ITEM_WAS_OPENED))
                            cnt++;
                    }
                }
            }

            /* If there's only one open, it means the task family is in the process
             * of closing the module for the last time. We must therefore remove the
             * family's write access to the module's memory.
             */
            if (cnt == 1)
            {
                TRACE(("CloseModule: revoking write access to range [%x..%x] from task %s\n",
                       m->li->dataBase, (uint32)m->li->dataBase + m->li->dataLength + m->li->bssLength - 1,
                       t->t.n_Name));

                result = externalControlMem(m->li->dataBase, m->li->dataLength + m->li->bssLength,
                                            MEMC_NOWRITE, t->t.n_Item);
            }
        }
    }

    if (result >= 0)
    {
        if (m->n.n_OpenCount == 0)
        {
            /* trigger an expunge of unused modules */
            result = PingDaemon();
        }
    }

    return result;
}
