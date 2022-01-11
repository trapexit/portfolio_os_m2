#ifndef __KERNEL_TASK_H
#define __KERNEL_TASK_H


/******************************************************************************
**
**  @(#) task.h 96/11/19 1.76
**
**  Kernel task management definitions
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

#ifndef __KERNEL_ITEM_H
#include <kernel/item.h>
#endif

#ifndef __KERNEL_LIST_H
#include <kernel/list.h>
#endif

#ifndef __KERNEL_TIME_H
#include <kernel/time.h>
#endif

#ifndef	EXTERNAL_RELEASE

#ifndef __LOADER_HEADER3DO_H
#include <loader/header3do.h>
#endif
#endif


/*****************************************************************************/


/* t.n_Flags */
#define TASK_READY           0x01   /* task in readyq waiting for cpu      */
#define TASK_WAITING         0x02   /* task in waitq waiting for a signal  */
#define TASK_RUNNING         0x04   /* task has cpu                        */
#define TASK_QUANTUM_EXPIRED 0x08   /* task is running on borrowed time... */

/* t_Flags */
#define TASK_DATADISCOK        0x00000001  /* This task tolerates data disks */
#define TASK_FREE_STACK        0x00000002  /* Kernel must free thread's stack */
#define	TASK_EXITING           0x00000004  /* The task is now exiting */
#define TASK_SINGLE_STACK      0x00000008  /* The task has no supervisor stack */
#define TASK_SUPERVISOR_ONLY   0x00000010  /* The task only runs in super mode */
#define TASK_PCMCIA_PERM       0x00000020  /* Permission to access PCMCIA space */
#define TASK_EXCEPTION         0x00000100  /* the task triggered an exception   */
#define TASK_SUSPENDED         0x00000200  /* the task has been suspended */

/* priorities go from 1 to 254 for privileged tasks */
#define PRIVTASK_MIN_PRIORITY 1
#define PRIVTASK_MAX_PRIORITY 254

/* priorities go from 10 to 199 for non-privileged tasks */
#define TASK_MIN_PRIORITY 10
#define TASK_MAX_PRIORITY 199

/* time quanta stuff */
#define	DEFAULT_QUANTA_USECS   15000	/* default task quanta in microsecs */
#define	MIN_QUANTA_USECS       5000	/* minimum quanta in microsecs	    */
#define	MAX_QUANTA_USECS       1000000	/* maximum quanta in microsecs	    */


/*****************************************************************************/

#ifndef EXTERNAL_RELEASE
/* This is the stack frame that always exists at t_ssp in a TCB. This is
 * where the system call handler stores its local state. This structure
 * must be a multiple of 8 bytes in size.
 */
typedef struct SuperStackFrame
{
    uint32 ssf_BackChain;	/* EABI: always NULL			*/
    uint32 ssf_CalleeLR;	/* EABI: place where callee can save LR	*/
    uint32 ssf_SP;		/* original SP				*/
    uint32 ssf_LR;		/* original LR				*/
    uint32 ssf_SRR0;		/* original SRR0			*/
    uint32 ssf_SRR1;		/* original SRR1			*/
    uint32 ssf_R3;
    uint32 ssf_R4;
    uint32 ssf_R5;
    uint32 ssf_R6;
} SuperStackFrame;
#endif

/* a task's CPU register state */
typedef struct RegBlock
{
    uint32 rb_GPRs[32];
    uint32 rb_CR;
    uint32 rb_XER;
    uint32 rb_LR;
    uint32 rb_CTR;
    uint32 rb_PC;
    uint32 rb_MSR;
} RegBlock;

/* a task's FP register state */
typedef struct FPRegBlock
{
    uint32 fprb_FPRs[32];		/* register contents	*/
    uint32 fprb_FPSCR;			/* fp status - 32 bits  */
    uint32 fprb_SP;
    uint32 fprb_LT;			/* SP/LT bits		*/
} FPRegBlock;

/* types of exceptions that can be handled by user-code */
#define USEREXC_TRAP          0x00000001
#define USEREXC_FP_INVALID_OP 0x00000002
#define USEREXC_FP_OVERFLOW   0x00000004
#define USEREXC_FP_UNDERFLOW  0x00000008
#define USEREXC_FP_ZERODIVIDE 0x00000010
#define USEREXC_FP_INEXACT    0x00000020

/* context information available after an exception occurs */
typedef struct UserExceptionContext
{
    Item        uec_Task;       /* task that caused the exception  */
    uint32      uec_Trigger;    /* exception(s) that occured       */
    RegBlock   *uec_RegBlock;   /* CPU registers at exception time */
    FPRegBlock *uec_FPRegBlock; /* FP registers at exception time  */
} UserExceptionContext;


/*****************************************************************************/


#ifndef EXTERNAL_RELEASE
/* The resource table has some bits packed in the upper
 * bits of the Item, since Items will max out at 16384.
 */
#define ITEM_WAS_OPENED	0x4000
#define ITEM_NOT_AN_ITEM	0x80000000
#endif /* EXTERNAL_RELEASE */

typedef struct Task
{
    ItemNode     t;
    struct Task *t_ThreadTask;      /* I am a thread of what task?  */
    uint32       t_WaitBits;        /* signals being waited for     */
    uint32       t_SigBits;         /* signals received             */
    uint32       t_AllocatedSigs;   /* signals allocated            */
    uint32      *t_StackBase;       /* base of stack                */
    int32        t_StackSize;       /* size of stack                */
    uint32       t_MaxUSecs;        /* quantum length in usecs      */
    TimerTicks   t_ElapsedTime;     /* time spent running this task */
    uint32       t_NumTaskLaunch;   /* # times launched this task   */
    uint32       t_Flags;           /* task flags                   */
    Item         t_Module;          /* the module we live within    */
    Item         t_DefaultMsgPort;  /* default task msgport         */
    void        *t_UserData;        /* user-private data            */
#ifndef EXTERNAL_RELEASE

    uint32       t_Unused0;

    /* per-folio data */
    void       **t_FolioData;       /* preallocated ptrs for first 4 folios */

    MinNode      t_TasksLinkNode;   /* Link to the list of all tasks */

    struct PagePool *t_PagePool;    /* task's free memory pool      */

    /* resource tracking */
    int32        t_FreeResourceTableSlot;  /* available slot in resource table */
    Item        *t_ResourceTable;          /* list of Items we need to clean up */
    int32        t_ResourceCnt;                /* maximum number of slots in ResourceTable */

    /* supervisor stack */
    uint32      *t_ssp;
    int32        t_SuperStackSize;
    uint32      *t_SuperStackBase;

    Item         t_ExitMessage;     /* Exit status message to be sent to parent */
    uint32       t_ExitStatus;      /* Status to return to parent on exit */
    struct Task *t_Killer;  /* the killer task */

    TimerTicks   t_Quantum;
    TimerTicks   t_QuantumRemainder;

    /* messages gotten and not replied */
    List         t_MessagesHeld;

    /* item this task is waiting for */
    Item         t_WaitItem;

    /* context save area */
    RegBlock     t_RegisterSave;    /* general register save */
    FPRegBlock   t_FPRegisterSave;

    uint8        t_Forbid;
    uint8        t_CalledULand;
    uint8        t_Unused[2];

    /* user exception stuff */
    uint32               t_CapturedExceptions;   /* exceptions we capture       */
    Item                 t_UserExceptionMsg;     /* message to use on exception */
    UserExceptionContext t_UserExceptionContext; /* context for exception       */
#endif
} Task, *TaskP;

/* convenient way to access a Task structure starting from its item number */
#define TASK(taskItem)     ((Task *)LookupItem(taskItem))
#define THREAD(threadItem) ((Task *)LookupItem(threadItem))


/*****************************************************************************/


/* predefined signals */
#define SIGF_ABORT	4              /* for internal use           */
#define SIGF_IODONE	8              /* IO operation completed     */
#define SIGF_DEADTASK	16             /* child task/thread has died */
#define SIGF_ONESHOT	32             /* for internal use           */

#define SIGF_RESERVED   0x800000ff


/*****************************************************************************/


/* Tag Args when creating a task/thread */
enum task_tags
{
    CREATETASK_TAG_MODULE=TAG_ITEM_LAST+1, /* A code module item             */
    CREATETASK_TAG_PC,                  /* where to start executing          */
    CREATETASK_TAG_SP,                  /* stack, make this a thread         */
    CREATETASK_TAG_STACKSIZE,           /* size of stack                     */
    CREATETASK_TAG_ARGC,                /* argc for new context              */
    CREATETASK_TAG_ARGP,                /* argv for new context              */
    CREATETASK_TAG_CMDSTR,              /* ptr to command-line               */
    CREATETASK_TAG_FREESTACK,           /* kernel should free thread's stack */
    CREATETASK_TAG_MAXQ,                /* time quantum, in usecs            */
    CREATETASK_TAG_MSGFROMCHILD,        /* send status msg to parent on exit */
    CREATETASK_TAG_USERDATA,            /* value to put in t_UserData        */
    CREATETASK_TAG_USEREXCHANDLER,      /* pointer to user exception handler */
    CREATETASK_TAG_DEFAULTMSGPORT,      /* create a default message port     */
    CREATETASK_TAG_THREAD,              /* make this a thread                */
    CREATETASK_TAG_REGBLOCK,            /* initial contents of GP registers  */
    CREATETASK_TAG_FPREGBLOCK           /* initial contents of FP registers  */
#ifndef EXTERNAL_RELEASE
    ,
    CREATETASK_TAG_PRIVILEGED = CREATETASK_TAG_FPREGBLOCK+25, /* make task privileged */
    CREATETASK_TAG_SINGLE_STACK,    /* no seperate supervisor stack needed */
    CREATETASK_TAG_SUPERVISOR_MODE  /* start task up in supervisor mode    */
#endif
};


/*****************************************************************************/


#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */


/* to start a function as a thread */
Item CreateThread(void (*code)(), const char *name, uint8 pri, int32 stackSize, const TagArg *tags);
Item CreateThreadVA(void (*code)(), const char *name, uint8 pri, int32 stackSize, uint32 tag, ...);

/* to start a module as a thread */
Item CreateModuleThread(Item module, const char *name, const TagArg *tags);
Item CreateModuleThreadVA(Item module, const char *name, uint32 tag, ...);

/* to start a module as a full-fledged task */
Item CreateTask(Item module, const char *name, const TagArg *tags);
Item CreateTaskVA(Item module, const char *name, uint32 tag, ...);

/* signal management */
int32 AllocSignal(int32 sigMask);
Err   FreeSignal(int32 sigMask);
int32 WaitSignal(int32 sigMask);
Err   SendSignal(Item task, int32 sigMask);

/* user exception handling support */
Err RegisterUserExceptionHandler(Item task, Item port);
Err ControlUserExceptions(uint32 exceptions, bool captured);
Err CompleteUserException(Item task, const RegBlock *rb, const FPRegBlock *fprb);

/* obscure task kinda things */
void Yield(void);
Err  SetExitStatus(int32 status);
void InvalidateFPState(void);
void BeginNoReboot(void);
void EndNoReboot(void);
void ExpectDataDisc(void);
void NoExpectDataDisc(void);
Err  IncreaseResourceTable(uint32 numSlots);

#ifndef	EXTERNAL_RELEASE
void Print3DOHeader(Item module, char *whatstr, char *copystr);
void Murder(Task *t,Task *killer);
void PrintTask(const char *label, const Task *t);
#endif

#ifdef  __cplusplus
}
#endif  /* __cplusplus */


#define DeleteThread(t)        DeleteItem(t)
#define DeleteModuleThread(t)  DeleteItem(t)
#define DeleteTask(t)          DeleteItem(t)

#define	GetTaskSignals(t)      ((t)->t_SigBits)
#define FindTask(n)            FindNamedItem(MKNODEID(KERNELNODE,TASKNODE),(n))

#ifndef KERNEL
#define CURRENTTASKITEM        FindItem(MKNODEID(KERNELNODE,TASKNODE), (TagArg *)NULL)
#define CURRENTTASK            TASK(CURRENTTASKITEM)
#endif

#define GetCurrentSignals()    GetTaskSignals(CURRENTTASK)
#define ClearCurrentSignals(s) ((GetCurrentSignals() & (s)) ? WaitSignal(GetCurrentSignals() & (s)) : 0)

#ifndef EXTERNAL_RELEASE
/* get the address of a Task from the embedded t_TasksLinkNode address */
#define Task_Addr(a)	(Task *)((uint32)(a) - Offset(Task *, t_TasksLinkNode))

/* are the two arguments part of the same task family? */
#define IsSameTaskFamily(t1,t2) (((t1)->t_PagePool == (t2)->t_PagePool) ? TRUE : FALSE)

/* must be used with interrupts disabled */
#define ClearSignals(t,s) ((t)->t_SigBits &= ~(s))
#define SetSignals(t,s)   ((t)->t_SigBits |= (s))


/* These macros let you make the current task privileged and restore it to its
 * previous state. These are used like:
 *
 * {
 * uint8 oldPriv;
 *
 *     oldPriv = PromotePriv(CURRENTTASK);
 *     // current task is now privileged
 *
 *     DemotePriv(CURRENTTASK,oldPriv);
 *     // current task is now back to its original privilege state
 * }
 */
#define PromotePriv(task) ((task)->t.n_ItemFlags & ITEMNODE_PRIVILEGED ?\
                           ITEMNODE_PRIVILEGED :\
                           ((task)->t.n_ItemFlags |= ITEMNODE_PRIVILEGED, 0))
#define DemotePriv(task,oldPriv) {(task)->t.n_ItemFlags &= (~ITEMNODE_PRIVILEGED);\
                                 (task)->t.n_ItemFlags |= oldPriv;}

#define IsPriv(task) ((task)->t.n_ItemFlags & ITEMNODE_PRIVILEGED ? TRUE : FALSE)

/*
   In supervisor code we frequently need to find the operator task.  To
   avoid a LookupItem, we cache a pointer to the oper task and the associated
   item number.
*/

extern Item	OperTaskItem;
extern Task *	OperTask;

#endif /* EXTERNAL_RELEASE */

/*****************************************************************************/


#endif /* __KERNEL_TASK_H */
