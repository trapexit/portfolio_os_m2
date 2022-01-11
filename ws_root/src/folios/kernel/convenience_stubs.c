/* @(#) convenience_stubs.c 96/07/31 1.2 */

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/mem.h>
#include <kernel/kernelnodes.h>
#include <kernel/msgport.h>
#include <kernel/operror.h>
#include <kernel/semaphore.h>
#include <kernel/task.h>
#include <kernel/interrupts.h>
#include <kernel/super.h>
#include <loader/loader3do.h>
#include <file/filesystem.h>
#include <file/filesystem.h>


/*****************************************************************************/


extern Item CreateNamedItemVA(int32 ctype, const char *name, uint8 pri, uint32 extraTags, ...);


/*****************************************************************************/


Item CreateBufferedMsg(const char *name,uint8 pri, Item mp, int32 datasize)
{
    return CreateNamedItemVA(MKNODEID(KERNELNODE,MESSAGENODE),name,pri,
                             CREATEMSG_TAG_REPLYPORT, mp,
                             (datasize ? CREATEMSG_TAG_DATA_SIZE : TAG_NOP), datasize,
                             TAG_END);
}


/*****************************************************************************/


Item CreateFIRQ(const char *name, uint8 pri, int32 (*code)(void), int32 num)
{
    return CreateNamedItemVA(MKNODEID(KERNELNODE,FIRQNODE),name,pri,
				  CREATEFIRQ_TAG_CODE, code,
				  CREATEFIRQ_TAG_NUM,  num,
                                  TAG_END);
}


/*****************************************************************************/


Item CreateIOReq(const char *name,uint8 pri,Item dev, Item mp)
{
    return CreateNamedItemVA(MKNODEID(KERNELNODE,IOREQNODE),name,pri,
                             CREATEIOREQ_TAG_DEVICE,    dev,
                             (mp > 0 ? CREATEIOREQ_TAG_REPLYPORT : TAG_NOP), mp,
                             TAG_END);
}


/*****************************************************************************/


Item CreateMsg(const char *name,uint8 pri,Item mp)
{
    return CreateNamedItemVA(MKNODEID(KERNELNODE,MESSAGENODE),name,pri,
                             CREATEMSG_TAG_REPLYPORT,mp,
                             TAG_END);
}


/*****************************************************************************/


Item CreateMsgPort(const char *name,uint8 pri,int32 signal)
{
    return CreateNamedItemVA(MKNODEID(KERNELNODE,MSGPORTNODE),name,pri,
                             (signal ? CREATEPORT_TAG_SIGNAL : TAG_NOP), signal,
                             TAG_END);
}


/*****************************************************************************/


Item CreateSemaphore(const char *name,uint8 pri)
{
    return CreateNamedItemVA(MKNODEID(KERNELNODE,SEMA4NODE),name,pri,TAG_END);
}


/*****************************************************************************/


Item CreateSmallMsg(const char *name,uint8 pri,Item mp)
{
    return CreateNamedItemVA(MKNODEID(KERNELNODE,MESSAGENODE),name,pri,
                             CREATEMSG_TAG_REPLYPORT,    mp,
                             CREATEMSG_TAG_MSG_IS_SMALL, 0,
                             TAG_END);
}


/*****************************************************************************/


Item FindNamedItem(int32 cntype, const char *name)
{
   return FindItemVA(cntype, TAG_ITEM_NAME, name,
                             TAG_END);
}


/*****************************************************************************/


Item CreateThread(void (*code)(), const char *name, uint8 pri, int32 stackSize,
	          const TagArg *tags)
{
uint8 *stack;
Item   thread;

    stackSize = ALLOC_ROUND(stackSize, 8);

    stack = AllocMem(stackSize, MEMTYPE_NORMAL);
    if (!stack)
        return NOMEM;

    thread = CreateItemVA(MKNODEID(KERNELNODE,TASKNODE),
                          TAG_ITEM_NAME,                  name,
                          (pri ? TAG_ITEM_PRI : TAG_NOP), pri,
                          CREATETASK_TAG_PC,              code,
                          CREATETASK_TAG_STACKSIZE,       stackSize,
                          CREATETASK_TAG_SP,              &stack[stackSize],
                          CREATETASK_TAG_FREESTACK,       0,
                          CREATETASK_TAG_THREAD,          0,
                          TAG_JUMP,                       tags);
    if (thread < 0)
        FreeMem(stack, stackSize);

    return thread;
}


/*****************************************************************************/


Item CreateModuleThread(Item module, const char *name, const TagArg *tags)
{
uint8  *stack;
Item    thread;
int32   stackSize;
Module *m;

    m = CheckItem(module, KERNELNODE, MODULENODE);
    if (!m)
        return BADITEM;

    stackSize = m->li->header3DO->_3DO_Stack;
    if (stackSize == 0)
        return MakeKErr(ER_SEVERE, ER_C_NSTND, ER_Kr_BadStackSize);

    stackSize = ALLOC_ROUND(stackSize, 8);

    stack = AllocMem(stackSize, MEMTYPE_NORMAL);
    if (!stack)
        return NOMEM;

    thread = CreateItemVA(MKNODEID(KERNELNODE,TASKNODE),
                          TAG_ITEM_NAME,                     name,
                          CREATETASK_TAG_MODULE,             module,
                          CREATETASK_TAG_STACKSIZE,          stackSize,
                          CREATETASK_TAG_SP,                 &stack[stackSize],
                          CREATETASK_TAG_FREESTACK,          0,
                          CREATETASK_TAG_THREAD,             0,
                          TAG_JUMP,                          tags);
    if (thread < 0)
        FreeMem(stack, stackSize);

    return thread;
}


/*****************************************************************************/


Item CreateTask(Item module, const char *name, const TagArg *tags)
{
    Module *m;
    TagArg *t;
    TagArg tagarray[4];

    m = (Module *) LookupItem(module);
    if (m == NULL)
	return BADITEM;

    t = tagarray;
    t->ta_Tag = TAG_ITEM_NAME;
    t->ta_Arg = (void*) name;
    t++;
    t->ta_Tag = CREATETASK_TAG_MODULE;
    t->ta_Arg = (void*) module;
    t++;
    /* Set the task's current directory, unless _3DO_NO_CHDIR is set. */
    if ((m->li->header3DO->_3DO_Flags & _3DO_NO_CHDIR) == 0)
    {
	t->ta_Tag = (FILEFOLIO << 16) | FILETASK_TAG_CURRENTDIRECTORY;
	t->ta_Arg = (void*) m->li->directory;
	t++;
    }
    t->ta_Tag = TAG_JUMP;
    t->ta_Arg = (void*) tags;

    return CreateItem(MKNODEID(KERNELNODE,TASKNODE), tagarray);
}
