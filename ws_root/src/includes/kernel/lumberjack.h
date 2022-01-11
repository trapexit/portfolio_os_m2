#ifndef __KERNEL_LUMBERJACK_H
#define __KERNEL_LUMBERJACK_H


/******************************************************************************
**
**  @(#) lumberjack.h 96/08/07 1.13
**
**  Definitions for Lumberjack, the Portfolio event logging service.
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_NODES_H
#include <kernel/nodes.h>
#endif

#ifndef __KERNEL_TIME_H
#include <kernel/time.h>
#endif

#ifndef __KERNEL_MSGPORT_H
#include <kernel/msgport.h>
#endif

#ifndef __KERNEL_SEMAPHORE_H
#include <kernel/semaphore.h>
#endif

#ifndef __KERNEL_IO_H
#include <kernel/io.h>
#endif


/*****************************************************************************/


/* These are the different kinds of events that Lumberjack currently logs. */
typedef enum LogTypes
{
    LOG_TYPE_BUFFER_END,               /* Marks the end of a buffer          */
    LOG_TYPE_BUFFER_OVERFLOW,          /* Events have been lost              */

    LOG_TYPE_USER,                     /* A user-generated event             */

    LOG_TYPE_TASK_CREATED,             /* A task is being created            */
    LOG_TYPE_TASK_DIED,                /* A task has died                    */
    LOG_TYPE_TASK_READY,               /* A task is becoming ready to run    */
    LOG_TYPE_TASK_RUNNING,             /* A task is starting to run          */
    LOG_TYPE_TASK_SUPERVISOR,          /* A task is entering supervisor mode */
    LOG_TYPE_TASK_USER,                /* A task is returning to user mode   */
    LOG_TYPE_TASK_FORBID,              /* A task is disabling task switching */
    LOG_TYPE_TASK_PERMIT,              /* A task is enabling task switching  */
    LOG_TYPE_TASK_PRIORITY,            /* A task is changing priority        */

    LOG_TYPE_INTERRUPT_START,          /* An interrupt is being serviced     */
    LOG_TYPE_INTERRUPT_DONE,           /* Interrupt are done being serviced  */

    LOG_TYPE_SIGNAL_SENT,              /* Signals are being sent             */
    LOG_TYPE_SIGNAL_WAIT,              /* Signals are being waited for       */

    LOG_TYPE_MESSAGE_SENT,             /* A message is being sent            */
    LOG_TYPE_MESSAGE_GOTTEN,           /* A message is being gotten          */
    LOG_TYPE_MESSAGE_REPLIED,          /* A message is being replied         */
    LOG_TYPE_MESSAGE_WAIT,             /* Messages are being waited for      */

    LOG_TYPE_SEMAPHORE_LOCKED,         /* A semaphore is being locked        */
    LOG_TYPE_SEMAPHORE_UNLOCKED,       /* A semaphore is being unlocked      */
    LOG_TYPE_SEMAPHORE_FAILED,         /* A semaphore was already locked     */
    LOG_TYPE_SEMAPHORE_WAIT,           /* A semaphore is being waited on     */

    LOG_TYPE_PAGES_ALLOCATED,          /* Memory pages are being allocated   */
    LOG_TYPE_PAGES_FREED,              /* Memory pages are being freed       */
    LOG_TYPE_PAGES_GIVEN,              /* Memory pages given to another task */

    LOG_TYPE_ITEM_CREATED,             /* An item was created                */
    LOG_TYPE_ITEM_DELETED,             /* An item was deleted                */
    LOG_TYPE_ITEM_OPENED,              /* An item was opened                 */
    LOG_TYPE_ITEM_CLOSED,              /* An item was closed                 */
    LOG_TYPE_ITEM_CHANGEDOWNER,        /* An item has a new owner            */

    LOG_TYPE_IOREQ_STARTED,            /* An IO operation was initiated      */
    LOG_TYPE_IOREQ_COMPLETED,          /* An IO operation completed          */
    LOG_TYPE_IOREQ_ABORTED             /* An IO operation was aborted        */
} LogTypes;


/*****************************************************************************/


/* These are used with ControlLumberjack() to set what event types get logged */
#define LOGF_CONTROL_USER       (1 << 0)
#define LOGF_CONTROL_TASKS      (1 << 1)
#define LOGF_CONTROL_INTERRUPTS (1 << 2)
#define LOGF_CONTROL_SIGNALS    (1 << 3)
#define LOGF_CONTROL_MESSAGES   (1 << 4)
#define LOGF_CONTROL_SEMAPHORES (1 << 5)
#define LOGF_CONTROL_PAGES      (1 << 6)
#define LOGF_CONTROL_ITEMS      (1 << 7)
#define LOGF_CONTROL_IOREQS     (1 << 8)


/*****************************************************************************/


/* A buffer used to contain logged events. The lb_BufferData field points
 * to the data area, and lb_BufferSize indicates how many bytes of useful
 * data is present in the buffer.
 */
typedef struct LumberjackBuffer
{
    MinNode lb;
    void   *lb_BufferData;
    uint32  lb_BufferSize;
} LumberjackBuffer;


/* Fields of this type indicate an offset from the beginning of the event
 * structure where a NUL-terminated string can be found.
 */
typedef uint32 LumberjackBufferOffset;


/*****************************************************************************/


/* This is the header that appears at the beginning of every logged event.
 *
 * When scanning through a log buffer, you use the lhe_NextEvent field to
 * tell you the offset of the next event in the buffer. When you find an event
 * of type LOG_TYPE_BUFFER_END, it means you have reached the last event in
 * the buffer.
 *
 * The time stamp is in internal system timer ticks. To convert these to
 * a usable form, see the kernel's ConvertTimerTicksToTimeVal() function.
 */
typedef struct LogEventHeader
{
    uint16                 leh_Type;         /* See the LogTypes definitions above */
    uint16                 leh_NextEvent;    /* # bytes until next event in buffer */
    LumberjackBufferOffset leh_TaskName;     /* Name of task involved in the event */
    Item                   leh_TaskItem;     /* Item of task involved in the event */
    TimerTicks             leh_TimeStamp;    /* When the event occured             */
    uint32                 leh_Reserved;     /* Always 0 for now                   */
} LogEventHeader;


/*****************************************************************************/


/* This section contains a definition of the different event structures. Every
 * event that is logged has its own structure to describe the event.
 */

typedef struct LogEventBufferEnd
{
    LogEventHeader lebe_Header;
} LogEventBufferEnd;

typedef struct LogEventBufferOverflow
{
    LogEventHeader lebo_Header;
    TimerTicks     lebo_OverflowRecovery;
} LogEventBufferOverflow;

typedef struct LogEventUser
{
    LogEventHeader         leu_Header;
    LumberjackBufferOffset leu_Description;       /* string supplied by user */
} LogEventUser;

typedef struct LogEventTaskCreated
{
    LogEventHeader          letc_Header;
    Item                    letc_TaskItem;
    void                   *letc_TaskAddr;
    LumberjackBufferOffset  letc_TaskName;
    uint32                  letc_StackSize;
    uint8                   letc_NodeFlags;
    uint8                   letc_TaskFlags;
    uint8                   letc_Priority;
} LogEventTaskCreated;

typedef struct LogEventTaskDied
{
    LogEventHeader         letd_Header;
    Item                   letd_KillerItem;
    LumberjackBufferOffset letd_KillerName;
    int32                  letd_ExitStatus;
} LogEventTaskDied;

typedef struct LogEventTaskReady
{
    LogEventHeader         letr_Header;
    Item                   letr_TaskItem;
    LumberjackBufferOffset letr_TaskName;
} LogEventTaskReady;

typedef struct LogEventTaskRunning
{
    LogEventHeader         letr_Header;
    Item                   letr_TaskItem;
    LumberjackBufferOffset letr_TaskName;
} LogEventTaskRunning;

typedef struct LogEventTaskSupervisor
{
    LogEventHeader lets_Header;
} LogEventTaskSupervisor;

typedef struct LogEventTaskUser
{
    LogEventHeader letu_Header;
} LogEventTaskUser;

typedef struct LogEventTaskForbid
{
    LogEventHeader letf_Header;
    uint8          letf_NestCount;
} LogEventTaskForbid;

typedef struct LogEventTaskPermit
{
    LogEventHeader letp_Header;
    uint8          letp_NestCount;
} LogEventTaskPermit;

typedef struct LogEventTaskPriority
{
    LogEventHeader         letp_Header;
    Item                   letp_TaskItem;
    LumberjackBufferOffset letp_TaskName;
    uint8                  letp_OldPriority;
    uint8                  letp_NewPriority;
} LogEventTaskPriority;

typedef struct LogEventInterruptStart
{
    LogEventHeader         leis_Header;
    uint32                 leis_ID;
    LumberjackBufferOffset leis_Description;
} LogEventInterruptStart;

typedef struct LogEventInterruptDone
{
    LogEventHeader         leid_Header;
    uint32                 leid_ID;
    LumberjackBufferOffset leid_Description;
} LogEventInterruptDone;

typedef struct LogEventSignalSent
{
    LogEventHeader         less_Header;
    Item                   less_TaskItem;
    LumberjackBufferOffset less_TaskName;
    uint32                 less_Signals;
} LogEventSignalSent;

typedef struct LogEventSignalWait
{
    LogEventHeader lesw_Header;
    int32          lesw_WaitedSignals;
    int32          lesw_CurrentSignals;
} LogEventSignalWait;

typedef struct LogEventMessageSent
{
    LogEventHeader         lems_Header;
    Item                   lems_MessageItem;
    Item                   lems_PortItem;
    LumberjackBufferOffset lems_PortName;
    uint32                 lems_DataPtr;
    uint32                 lems_DataSize;
} LogEventMessageSent;

typedef struct LogEventMessageGotten
{
    LogEventHeader         lemg_Header;
    Item                   lemg_MessageItem;
    Item                   lemg_PortItem;
    LumberjackBufferOffset lemg_PortName;
} LogEventMessageGotten;

typedef struct LogEventMessageReplied
{
    LogEventHeader         lemr_Header;
    Item                   lemr_MessageItem;
    Item                   lemr_ReplyPortItem;
    LumberjackBufferOffset lemr_ReplyPortName;
    uint32                 lemr_DataPtr;
    uint32                 lemr_DataSize;
    uint32                 lemr_Result;
} LogEventMessageReplied;

typedef struct LogEventMessageWait
{
    LogEventHeader         lemw_Header;
    Item                   lemw_MessageItem;
    Item                   lemw_PortItem;
    LumberjackBufferOffset lemw_PortName;
} LogEventMessageWait;

typedef struct LogEventSemaphoreLocked
{
    LogEventHeader         lesl_Header;
    Item                   lesl_SemaphoreItem;
    LumberjackBufferOffset lesl_SemaphoreName;
    uint32                 lesl_NewNestCount;
    bool                   lesl_ReadMode;
} LogEventSemaphoreLocked;

typedef struct LogEventSemaphoreUnlocked
{
    LogEventHeader         lesu_Header;
    Item                   lesu_SemaphoreItem;
    LumberjackBufferOffset lesu_SemaphoreName;
    uint32                 lesu_NewNestCount;
} LogEventSemaphoreUnlocked;

typedef struct LogEventSemaphoreFailed
{
    LogEventHeader         lesf_Header;
    Item                   lesf_SemaphoreItem;
    LumberjackBufferOffset lesf_SemaphoreName;
    uint32                 lesf_NestCount;
    Item                   lesf_LockerItem;
    LumberjackBufferOffset lesf_LockerName;
} LogEventSemaphoreFailed;

typedef struct LogEventSemaphoreWait
{
    LogEventHeader         lesw_Header;
    Item                   lesw_SemaphoreItem;
    LumberjackBufferOffset lesw_SemaphoreName;
    uint32                 lesw_NestCount;
    Item                   lesw_LockerItem;
    LumberjackBufferOffset lesw_LockerName;
} LogEventSemaphoreWait;

typedef struct LogEventPagesAllocated
{
    LogEventHeader  lepa_Header;
    void           *lepa_Address;
    uint32          lepa_NumBytes;
    bool            lepa_SupervisorMemory;
} LogEventPagesAllocated;

typedef struct LogEventPagesFreed
{
    LogEventHeader  lepf_Header;
    void           *lepf_Address;
    uint32          lepf_NumBytes;
} LogEventPagesFreed;

typedef struct LogEventPagesGiven
{
    LogEventHeader         lepg_Header;
    void                  *lepg_Address;
    uint32                 lepg_NumBytes;
    Item                   lepg_RecipientTaskItem;
    LumberjackBufferOffset lepg_RecipientTaskName;
} LogEventPagesGiven;

typedef struct LogEventItemCreated
{
    LogEventHeader         leic_Header;
    Item                   leic_Item;
    LumberjackBufferOffset leic_ItemName;
    uint8                  leic_ItemType;
    uint8                  leic_ItemSubsysType;
} LogEventItemCreated;

typedef struct LogEventItemDeleted
{
    LogEventHeader         leid_Header;
    Item                   leid_Item;
    LumberjackBufferOffset leid_ItemName;
    uint8                  leid_ItemType;
    uint8                  leid_ItemSubsysType;
} LogEventItemDeleted;

typedef struct LogEventItemOpened
{
    LogEventHeader         leio_Header;
    Item                   leio_Item;
    LumberjackBufferOffset leio_ItemName;
    uint8                  leio_ItemType;
    uint8                  leio_ItemSubsysType;
} LogEventItemOpened;

typedef struct LogEventItemClosed
{
    LogEventHeader         leic_Header;
    Item                   leic_Item;
    LumberjackBufferOffset leic_ItemName;
    uint8                  leic_ItemType;
    uint8                  leic_ItemSubsysType;
} LogEventItemClosed;

typedef struct LogEventItemChangedOwner
{
    LogEventHeader         leico_Header;
    Item                   leico_Item;
    LumberjackBufferOffset leico_ItemName;
    uint8                  leico_ItemType;
    uint8                  leico_ItemSubsysType;
    Item                   leico_NewOwner;
    LumberjackBufferOffset leico_NewOwnerName;
} LogEventItemChangedOwner;

typedef struct LogEventIOReqStarted
{
    LogEventHeader         leios_Header;
    Item                   leios_Item;
    LumberjackBufferOffset leios_ItemName;
    LumberjackBufferOffset leios_DeviceName;
    uint32                 leios_Command;
} LogEventIOReqStarted;

typedef struct LogEventIOReqCompleted
{
    LogEventHeader         leioc_Header;
    Item                   leioc_Item;
    LumberjackBufferOffset leioc_ItemName;
    LumberjackBufferOffset leioc_DeviceName;
    uint32                  leioc_Command;
    Err                    leioc_Error;
} LogEventIOReqCompleted;

typedef struct LogEventIOReqAborted
{
    LogEventHeader         leioa_Header;
    Item                   leioa_Item;
    LumberjackBufferOffset leioa_ItemName;
    LumberjackBufferOffset leioa_DeviceName;
    uint32                 leioa_Command;
} LogEventIOReqAborted;


/*****************************************************************************/


Err CreateLumberjack(const TagArg *tags);
Err DeleteLumberjack(void);
Err ControlLumberjack(uint32 controlFlags);
Err LogEvent(const char *eventDescription);

Err  WaitLumberjackBuffer(void);
LumberjackBuffer *ObtainLumberjackBuffer(void);
void ReleaseLumberjackBuffer(LumberjackBuffer *lb);
void DumpLumberjackBuffer(const char *banner, const LumberjackBuffer *lb);


#ifndef EXTERNAL_RELEASE
/*****************************************************************************/


#ifdef BUILD_LUMBERJACK

void LogTaskCreated(const Task *t);
void LogTaskDied(const Task *t);
void LogTaskReady(const Task *t);
void LogTaskRunning(const Task *t);
void LogTaskSupervisor(void);
void LogTaskUser(void);
void LogTaskForbid(void);
void LogTaskPermit(void);
void LogInterruptStart(uint32 id, const char *comment);
void LogInterruptDone(void);
void LogTaskPriority(const Task *t, uint8 oldPri);
void LogSignalSent(const Task *to, int32 signals);
void LogSignalWait(int32 waitedSignals, int32 currentSignals);
void LogMessageSent(const Message *msg, const MsgPort *toPort, const void *dataptr, uint32 datasize);
void LogMessageGotten(const Message *msg, const MsgPort *fromPort);
void LogMessageReplied(const Message *msg, const void *dataptr, uint32 datasize);
void LogMessageWait(const Message *msg, const MsgPort *port);
void LogSemaphoreLocked(const Semaphore *sem, bool readMode);
void LogSemaphoreUnlocked(const Semaphore *sem);
void LogSemaphoreFailed(const Semaphore *sem);
void LogSemaphoreWait(const Semaphore *sem);
void LogPagesAllocated(const void *addr, uint32 numBytes, bool superMem);
void LogPagesFreed(const void *addr, uint32 numBytes);
void LogPagesGiven(const void *addr, uint32 numBytes, Task *toTask);
void LogItemCreated(const ItemNode *it);
void LogItemDeleted(const ItemNode *it);
void LogItemOpened(const ItemNode *it);
void LogItemClosed(const ItemNode *it);
void LogItemChangedOwner(const ItemNode *it, const Task *newOwner);
void LogIOReqStarted(const IOReq *ior);
void LogIOReqCompleted(const IOReq *ior);
void LogIOReqAborted(const IOReq *ior);

#else

#define LogTaskCreated(t)
#define LogTaskDied(t)
#define LogTaskReady(t)
#define LogTaskRunning(t)
#define LogTaskSupervisor(t)
#define LogTaskUser(t)
#define LogTaskForbid(t)
#define LogTaskPermit(t)
#define LogInterruptStart(id, comment)
#define LogInterruptDone()
#define LogTaskPriority(t, oldPri)
#define LogSignalSent(to, signals)
#define LogSignalWait(waitedSignals, currentSignals)
#define LogMessageSent(msg, toPort, dataptr, datasize)
#define LogMessageGotten(msg, fromPort)
#define LogMessageReplied(msg, dataptr, datasize)
#define LogMessageWait(msg, port)
#define LogSemaphoreLocked(sem, readMode)
#define LogSemaphoreUnlocked(sem)
#define LogSemaphoreFailed(sem)
#define LogSemaphoreWait(sem)
#define LogPagesAllocated(addr, numBytes, superMem)
#define LogPagesFreed(addr, numBytes)
#define LogPagesGiven(addr, numBytes, toTask)
#define LogItemCreated(it)
#define LogItemDeleted(it)
#define LogItemOpened(it)
#define LogItemClosed(it)
#define LogItemChangedOwner(it, newOwner)
#define LogIOReqStarted(ior)
#define LogIOReqCompleted(ior)
#define LogIOReqAborted(ior)

#endif

#endif
/*****************************************************************************/


#endif /* __KERNEL_LUMBERJACK_H */
