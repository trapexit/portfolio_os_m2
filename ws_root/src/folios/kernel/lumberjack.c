/* @(#) lumberjack.c 96/08/07 1.28 */

#ifdef BUILD_LUMBERJACK


/*****************************************************************************/


#include <kernel/types.h>
#include <kernel/lumberjack.h>
#include <kernel/operror.h>
#include <kernel/mem.h>
#include <kernel/nodes.h>
#include <kernel/list.h>
#include <kernel/listmacros.h>
#include <kernel/kernel.h>
#include <kernel/internalf.h>
#include <kernel/bitarray.h>
#include <string.h>


/*****************************************************************************/


#define BUFFER_SIZE (256*1024)
#define NUM_BUFFERS 3


/*****************************************************************************/


typedef struct
{
    MinNode  bw;
    Task    *bw_Task;
} BufferWaiter;


/*****************************************************************************/


static List              emptyBuffers    = PREPLIST(emptyBuffers);
static List              fullBuffers     = PREPLIST(fullBuffers);
static List              obtainedBuffers = PREPLIST(obtainedBuffers);
static List              waiters         = PREPLIST(waiters);
static LumberjackBuffer *activeBuffer    = NULL;
static bool              overflow        = FALSE;
static bool              created         = FALSE;
static uint32            eventsLogged    = 0;
static TimerTicks        overflowStartTime;
static LogEventHeader   *nextEvent;


/*****************************************************************************/


/**
|||	AUTODOC -class Kernel -group Lumberjack -name CreateLumberjack
|||	Initializes Lumberjack, the Portfolio logging service.
|||
|||	  Synopsis
|||
|||	    Err CreateLumberjack(const TagArg *tags);
|||
|||	  Description
|||
|||	    Lumberjack is used to log system events to aid during
|||	    debugging. The kernel uses Lumberjack to store a large amount
|||	    of information about what is currently happening in the system.
|||
|||	    The information logged by the kernel includes all task switches,
|||	    interrupts, signals or messages being sent, semaphores being
|||	    locked and unlocked, pages of memory being allocated or freed,
|||	    and more. You can also add your own events to the log using
|||	    the LogEvent() function.
|||
|||	    When you call this function, a list of buffers is allocated
|||	    that currently total up to a little over 768K of memory. These
|||	    buffers are used by Lumberjack to store the log entries. To
|||	    start logging information in these buffers, you must call
|||	    ControlLumberjack().
|||
|||	    As buffers get filled up, Lumberjack puts the buffers into a list
|||	    of filled buffers. You can get the oldest buffer on this list
|||	    by calling ObtainLumberjackBuffer(). Once you have the buffer,
|||	    you can parse the log entries yourself to extract information, or
|||	    you can call DumpLumberjackBuffer() to parse and output the
|||	    contents of the buffer to the debugging terminal.
|||
|||	  Arguments
|||
|||	    tags
|||	        This is reserved for future use and must currently always be
|||	        NULL.
|||
|||	  Return Value
|||
|||	    >= 0 for success, or a negative error code for failure. The
|||	    likely cause of failure is a lack of memory to allocate the
|||	    needed buffer space.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V27.
|||
|||	  Notes
|||
|||	    ... and I'm ok.
|||
|||	  Associated Files
|||
|||	    <kernel/lumberjack.h>
|||
|||	  See Also
|||
|||	    ObtainLumberjackBuffer(), ReleaseLumberjackBuffer(),
|||	    DumpLumberjackBuffer(), DeleteLumberjack(), LogEvent(),
|||	    WaitLumberjackBuffer(), ControlLumberjack()
|||
**/

Err internalCreateLumberjack(const TagArg *tags)
{
uint32            i;
LumberjackBuffer *lb;

    if (tags)
    {
        /* no tags are currently supported */
	return BADTAG;
    }

    if (created)
    {
        /* Lumberjack has already been created */
        return 1;
    }

    /* allocate the logging buffers */
    for (i = 0; i < NUM_BUFFERS; i++)
    {
        lb = (LumberjackBuffer *)SuperAllocMem(sizeof(LumberjackBuffer) + BUFFER_SIZE,
                                               MEMTYPE_ANY);
        if (!lb)
        {
            internalDeleteLumberjack();
            return NOMEM;
        }

        lb->lb_BufferData = &lb[1];
        ADDTAIL(&emptyBuffers,(Node *)lb);
    }

    /* Set things up so we're ready for action */
    nextEvent    = NULL;
    eventsLogged = 0;
    created      = TRUE;

    KB_FIELD(kb_CPUFlags) |= KB_LOGGING;

    return 0;
}


/*****************************************************************************/


/**
|||	AUTODOC -class Kernel -group Lumberjack -name DeleteLumberjack
|||	Disables Lumberjack, the Portfolio logging service.
|||
|||	  Synopsis
|||
|||	    Err DeleteLumberjack(void);
|||
|||	  Description
|||
|||	    This function stops Lumberjack and releases all buffer space
|||	    allocated by CreateLumberjack(). This causes any currently logged
|||	    events to be discarded. See ControlLumberjack() for a
|||	    way to stop logging without discarding the buffers.
|||
|||	  Return Value
|||
|||	    >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V27.
|||
|||	  Notes
|||
|||	    ... and I'm ok.
|||
|||	  Associated Files
|||
|||	    <kernel/lumberjack.h>
|||
|||	  See Also
|||
|||	    ObtainLumberjackBuffer(), ReleaseLumberjackBuffer(),
|||	    DumpLumberjackBuffer(), CreateLumberjack(), LogEvent(),
|||	    WaitLumberjackBuffer(), ControlLumberjack()
|||
**/

Err internalDeleteLumberjack(void)
{
LumberjackBuffer *lb;

    KB_FIELD(kb_CPUFlags) &= ~(KB_LOGGING);

    /* Disable logging */
    eventsLogged = 0;
    created      = FALSE;
    nextEvent    = NULL;

    /* Release the current buffer */
    SuperFreeMem(activeBuffer,sizeof(LumberjackBuffer) + BUFFER_SIZE);

    /* Release any empty buffers */
    while (lb = (LumberjackBuffer *)RemHead(&emptyBuffers))
        SuperFreeMem(lb,sizeof(LumberjackBuffer) + BUFFER_SIZE);

    /* Release any filled buffers */
    while (lb = (LumberjackBuffer *)RemHead(&fullBuffers))
        SuperFreeMem(lb,sizeof(LumberjackBuffer) + BUFFER_SIZE);

    /* Release any buffers the user got */
    while (lb = (LumberjackBuffer *)RemHead(&obtainedBuffers))
        SuperFreeMem(lb,sizeof(LumberjackBuffer) + BUFFER_SIZE);

    return 0;
}


/*****************************************************************************/


/**
|||	AUTODOC -class Kernel -group Lumberjack -name ControlLumberjack
|||	Determine which event types Lumberjack logs.
|||
|||	  Synopsis
|||
|||	    Err ControlLumberjack(uint32 eventsLogged);
|||
|||	  Description
|||
|||	    This function lets you control which events are logged
|||	    by Lumberjack. After you call CreateLumberjack(), you must
|||	    call this function in order to get logging to actually
|||	    happen.
|||
|||	  Arguments
|||
|||	    eventsLogged
|||	        This is a bit mask that specifies which types of events
|||	        should be logged by Lumberjack. See the LOGF_CONTROL_XXX
|||	        constants in <kernel/lumberjack.h> for the available
|||	        types.
|||
|||	        If you supply 0 for this argument, all logging will be
|||	        stopped.
|||
|||	  Return Value
|||
|||	    >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V27.
|||
|||	  Notes
|||
|||	    ... and I'm ok.
|||
|||	  Associated Files
|||
|||	    <kernel/lumberjack.h>
|||
|||	  See Also
|||
|||	    ObtainLumberjackBuffer(), ReleaseLumberjackBuffer(),
|||	    DumpLumberjackBuffer(), CreateLumberjack(), LogEvent(),
|||	    WaitLumberjackBuffer()
|||
**/

Err internalControlLumberjack(uint32 flags)
{
    if (!created)
        return NOTENABLED;

    eventsLogged = flags;

    return 0;
}


/*****************************************************************************/


static void CompleteActiveBuffer(void)
{
BufferWaiter *waiter;

    nextEvent->leh_Type      = LOG_TYPE_BUFFER_END;
    nextEvent->leh_NextEvent = 0;
    nextEvent->leh_TaskName  = 0;
    nextEvent->leh_TaskItem  = -1;
    nextEvent->leh_Reserved  = 0;
    SampleSystemTimeTT(&nextEvent->leh_TimeStamp);
    activeBuffer->lb_BufferSize = (uint32)nextEvent - (uint32)activeBuffer->lb_BufferData + sizeof(LogEventBufferEnd);
    AddTail(&fullBuffers,(Node *)activeBuffer);
    activeBuffer = NULL;
    nextEvent    = NULL;

    while (waiter = (BufferWaiter *)RemHead(&waiters))
        internalSignal(waiter->bw_Task, SIGF_ONESHOT);
}


/*****************************************************************************/


static Err Log(void *data, uint32 dataSize, uint32 stringWords)
{
uint32   oldints;
char    *ptr;
uint32   offset;
uint32   i;
char   **field;
uint32   stringSpace;

    oldints = Disable();

    if (nextEvent == NULL)
    {
        activeBuffer = (LumberjackBuffer *)RemHead(&emptyBuffers);
        if (!activeBuffer)
        {
            if (!overflow)
            {
                /* first time we overflow, record when this happened */
                overflow = TRUE;
                SampleSystemTimeTT(&overflowStartTime);
            }
            Enable(oldints);
            return LOGOVERFLOW;
        }
        nextEvent = activeBuffer->lb_BufferData;
    }

    /* initialize the standard LogEventHeader */
    memcpy(nextEvent,data,dataSize);
    nextEvent->leh_TaskItem = CURRENTTASKITEM;

    if (CURRENTTASK)
        nextEvent->leh_TaskName = (uint32)CURRENTTASK->t.n_Name;
    else
        nextEvent->leh_TaskName = 0;

    nextEvent->leh_Reserved = 0;
    SampleSystemTimeTT(&nextEvent->leh_TimeStamp);

    /* account for the LogEventHeader size, and include the current task name string */
    stringWords = (stringWords >> 6) | 0x40000000;

    /* The stringWords variable contains a 1 bit for every word of data that
     * is a string pointer. The high bit corresponds to word 0, the low bit
     * corresponds to word 31.
     *
     * Strings are stored after the current event structure.
     *
     * We must change the string pointer in the data structure to be
     * an offset within the current buffer where the string can be found.
     */

    stringSpace = 0;
    ptr         = (char *)((uint32)nextEvent + dataSize);
    while (stringWords)
    {
        i            = FindMSB(stringWords);
        stringWords &= ~(1 << i);    /* clear the bit we pinged on */
        offset       = (31-i)*4;
        field        = (char **)((uint32)nextEvent + offset);

        if (*field)
        {
            i = 0;
            while (TRUE)
            {
                ptr[i] = (*field)[i];
                if (ptr[i] == 0)
                    break;

                i++;
            }
            i++;
            stringSpace += i;
            *field       = (char *)((uint32)ptr - (uint32)activeBuffer->lb_BufferData);
            ptr          = &ptr[i];
        }
    }
    nextEvent->leh_NextEvent = (dataSize + stringSpace + 3) & 0xfffffffc;

    /* prepare for the next event */

    nextEvent = (LogEventHeader *)((uint32)nextEvent + nextEvent->leh_NextEvent);
    if ((uint32)nextEvent - (uint32)activeBuffer->lb_BufferData >= BUFFER_SIZE - 1024)
    {
        /* If there's less than 1024 bytes left in the buffer, we consider
         * it full.
         */
        CompleteActiveBuffer();
    }

    Enable(oldints);

    return 0;
}


/*****************************************************************************/


/**
|||	AUTODOC -class Kernel -group Lumberjack -name ObtainLumberjackBuffer
|||	Obtain a pointer to a Lumberjack buffer which is currently full.
|||
|||	  Synopsis
|||
|||	    LumberjackBuffer *ObtainLumberjackBuffer(void);
|||
|||	  Description
|||
|||	    Lumberjack, the Portfolio logging service, maintains a list of
|||	    buffers into which it logs events during system execution.
|||
|||	    This function returns a pointer to a logging buffer which is
|||	    currently full of logged events. Once you have obtained such a
|||	    buffer, you can parse its contents to extract useful information,
|||	    or just call DumpLumberjackBuffer() to display the buffer contents
|||	    to the debugging terminal.
|||
|||	    Since Lumberjack maintains a list of buffers, when one buffer
|||	    fills up, it simply gets a new buffer and starts logging events
|||	    there. This scheme lets you call ObtainLumberjackBuffer() to get
|||	    and parse a log buffer, while events are still being logged to a
|||	    different buffer.
|||
|||	    When you're done with a log buffer, you should return it to
|||	    Lumberjack using the ReleaseLumberjackBuffer() function. If the
|||	    buffer is not returned, it's possible Lumberjack will become
|||	    unable to log events because it has no buffer space left.
|||
|||	    Before you start obtaining buffers, it is generally a good idea
|||	    to first disable event logging using ControlLumberjack(0).
|||	    Otherwise, new events will continuously be logged while you're
|||	    dumping them or processing them, and you'll never see the end of
|||	    it.
|||
|||	    The LumberjackBuffer.lb_BufferData field points to the log entries
|||	    for the buffer. Each log entry starts with a LogEntryHeader
|||	    structure followed by a variable amount of data. The number of
|||	    bytes before the next entry is stored in the
|||	    LogEntryHeader.leh_NextEvent field. By adding this value to the
|||	    current LogEntryHeader address, you get to the next LogEntryHeader
|||	    structure in the buffer, The LogEntryHeader.leh_Type field
|||	    indicates what type of event this is. If the type is
|||	    LOG_TYPE_BUFFER_END, it means you have reached the last event in
|||	    the buffer.
|||
|||	  Return Value
|||
|||	    This function returns a pointer to a LumberjackBuffer structure
|||	    that Lumberjack has filled up. NULL is returned if there are
|||	    currently no filled buffers.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V27.
|||
|||	  Notes
|||
|||	    ... and I'm ok.
|||
|||	  Associated Files
|||
|||	    <kernel/lumberjack.h>
|||
|||	  See Also
|||
|||	    ReleaseLumberjackBuffer(), DumpLumberjackBuffer(),
|||	    CreateLumberjack(), DeleteLumberjack(), LogEvent(),
|||	    WaitLumberjackBuffer(), ControlLumberjack()
|||
**/

LumberjackBuffer *internalObtainLumberjackBuffer(void)
{
LumberjackBuffer *lb;
uint32            oldints;

    oldints = Disable();
    lb = (LumberjackBuffer *)RemHead(&fullBuffers);

    if (!lb)
    {
        if (eventsLogged == 0)
        {
            /* If we're not logging anything and we couldn't find a buffer
             * to return, grab the current buffer and return it.
             */
            if (nextEvent)
            {
                CompleteActiveBuffer();
                lb = (LumberjackBuffer *)RemHead(&fullBuffers);
            }
        }
    }

    Enable(oldints);

    if (lb)
    {
        /* keep track of what the user has */
        ADDTAIL(&obtainedBuffers,(Node *)lb);
    }

    return lb;
}


/*****************************************************************************/


/**
|||	AUTODOC -class Kernel -group Lumberjack -name WaitLumberjackBuffer
|||	Waits until a Lumberjack buffer is available.
|||
|||	  Synopsis
|||
|||	    Err WaitLumberjackBuffer(void);
|||
|||	  Description
|||
|||	    Lumberjack, the Portfolio logging service, maintains a list of
|||	    buffers into which it logs events during system execution.
|||
|||	    This function waits for a logging buffer to be full. Once this
|||	    function returns, you can call ObtainLumberjackBuffer() to get a
|||	    pointer to the buffer full of logged events. Once you have obtained
|||	    such a buffer, you can parse its contents to extract useful
|||	    information, or just call DumpLumberjackBuffer() to display the
|||	    buffer contents to the debugging terminal.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V32.
|||
|||	  Notes
|||
|||	    ... and I'm ok.
|||
|||	  Associated Files
|||
|||	    <kernel/lumberjack.h>
|||
|||	  See Also
|||
|||	    ObtainLumberjackBuffer(), ReleaseLumberjackBuffer(),
|||	    DumpLumberjackBuffer(),
|||	    CreateLumberjack(), DeleteLumberjack(), LogEvent(),
|||	    ControlLumberjack()
|||
**/

Err internalWaitLumberjackBuffer(void)
{
int32        sigs;
uint32       oldints;
BufferWaiter waiter;

    waiter.bw_Task = CURRENTTASK;
    ClearSignals(CURRENTTASK, SIGF_ONESHOT);
    oldints = Disable();
    AddTail(&waiters, (Node *)&waiter);

    sigs = internalWait(SIGF_ONESHOT);
    if ((sigs & SIGF_ONESHOT) == 0)
        RemNode((Node *)&waiter);

    Enable(oldints);

    if (sigs & SIGF_ABORT)
        return ABORTED;

    return 0;
}


/*****************************************************************************/


/**
|||	AUTODOC -class Kernel -group Lumberjack -name ReleaseLumberjackBuffer
|||	Return a logging buffer to the Lumberjack system so it can be reused.
|||
|||	  Synopsis
|||
|||	    void ReleaseLumberjackBuffer(LumberjackBuffer *lb);
|||
|||	  Description
|||
|||	    Lumberjack, the Portfolio logging service, maintains a list of
|||	    buffers into which it logs events during system execution.
|||	    You can obtain pointers to full log buffers in order to
|||	    display their contents using the ObtainLumberjackBuffer() folio
|||	    call. Once you are done processing the contents of a buffer, you
|||	    call ReleaseLumberjackBuffer() to return the buffer to Lumberjack
|||	    so it can be reused.
|||
|||	  Arguments
|||
|||	    lb
|||	        The log buffer to return to Lumberjack, as obtained
|||	        from ObtainLumberjackBuffer(). May be NULL in which case this
|||	        function does nothing.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V27.
|||
|||	  Notes
|||
|||	    ... and I'm ok.
|||
|||	  Associated Files
|||
|||	    <kernel/lumberjack.h>
|||
|||	  See Also
|||
|||	    ObtainLumberjackBuffer(), DumpLumberjackBuffer(), CreateLumberjack(),
|||	    DeleteLumberjack(), LogEvent(), ControlLumberjack()
|||	    WaitLumberjackBuffer()
|||
**/

void internalReleaseLumberjackBuffer(LumberjackBuffer *lb)
{
uint32            oldints;
bool              found;
LumberjackBuffer *buf;

    found = FALSE;
    ScanList(&obtainedBuffers, buf, LumberjackBuffer)
    {
        if (buf == lb)
        {
            found = TRUE;
            break;
        }
    }

    if (!found)
        return;

    /* remove the buffer from the obtained list */
    REMOVENODE((Node *)lb);

    oldints = Disable();
    ADDTAIL(&emptyBuffers,(Node *)lb);

    if (overflow)
    {
        /* We were in overflow state. Record this fact as the first event
         * in the buffer.
         */
        overflow                 = FALSE;
        activeBuffer             = lb;
        nextEvent                = activeBuffer->lb_BufferData;
        nextEvent->leh_Type      = LOG_TYPE_BUFFER_OVERFLOW;
        nextEvent->leh_NextEvent = sizeof(LogEventBufferOverflow);
        nextEvent->leh_TimeStamp = overflowStartTime;
        nextEvent->leh_TaskItem  = -1;
        nextEvent->leh_TaskName  = 0;
        nextEvent->leh_Reserved  = 0;
        SampleSystemTimeTT(&((LogEventBufferOverflow *)nextEvent)->lebo_OverflowRecovery);

        nextEvent = (LogEventHeader *)((uint32)nextEvent + nextEvent->leh_NextEvent);
    }

    Enable(oldints);
}


/*****************************************************************************/


/**
|||	AUTODOC -class Kernel -group Lumberjack -name LogEvent
|||	Add a custom event to the Lumberjack logs.
|||
|||	  Synopsis
|||
|||	    Err LogEvent(const char *eventDescription);
|||
|||	  Description
|||
|||	    Lumberjack is used to log system events to aid during
|||	    debugging. The kernel stores a large amount of information
|||	    about what is currently happening in the system into
|||	    Lumberjack. This function lets you add your own events
|||	    to the logs, which can be useful to track high level events
|||	    within your own code.
|||
|||	  Arguments
|||
|||	    eventDescription
|||	        This details your log entry. The event will be time stamped,
|||	        marked with some default information such as the current
|||	        task name, and inserted into the Lumberjack logs. This string
|||	        gets copied, so the original can be discarded after the call
|||	        returns.
|||
|||	  Return Value
|||
|||	    >= 0 for success, or a negative error code for failure. The
|||	    likely cause of failure is because you didn't call
|||	    CreateLumberjack() to activate Lumberjack, or there is no more
|||	    buffer space left to store the log entry. You can free up some
|||	    buffer space by calling ObtainLumberjackBuffer() followed by
|||	    ReleaseLumberjackBuffer().
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V27.
|||
|||	  Notes
|||
|||	    ... and I'm ok.
|||
|||	  Associated Files
|||
|||	    <kernel/lumberjack.h>
|||
|||	  See Also
|||
|||	    ObtainLumberjackBuffer(), ReleaseLumberjackBuffer(),
|||	    DumpLumberjackBuffer(), CreateLumberjack(), DeleteLumberjack(),
|||	    WaitLumberjackBuffer(), ControlLumberjack()
|||
**/

Err internalLogEvent(const char *eventDescription)
{
    if (eventsLogged & LOGF_CONTROL_USER)
    {
    LogEventUser l;

        l.leu_Header.leh_Type = LOG_TYPE_USER;
        l.leu_Description     = (uint32)eventDescription;
        return Log(&l,sizeof(l),0x80000000);
    }

    return 0;
}


/*****************************************************************************/


void LogTaskCreated(const Task *t)
{
    if (eventsLogged & LOGF_CONTROL_TASKS)
    {
    LogEventTaskCreated l;

        l.letc_Header.leh_Type = LOG_TYPE_TASK_CREATED;
        l.letc_TaskItem        = t->t.n_Item;
        l.letc_TaskAddr        = (void *)t;
        l.letc_TaskName        = (uint32)t->t.n_Name;
        l.letc_StackSize       = t->t_StackSize;
        l.letc_NodeFlags       = t->t.n_Flags;
        l.letc_TaskFlags       = t->t_Flags;
        l.letc_Priority        = t->t.n_Priority;
        Log(&l,sizeof(l),0x20000000);
    }
}


/*****************************************************************************/


void LogTaskDied(const Task *t)
{
    if (eventsLogged & LOGF_CONTROL_TASKS)
    {
    LogEventTaskDied l;

        l.letd_Header.leh_Type = LOG_TYPE_TASK_DIED;
        l.letd_KillerItem      = t->t_Killer->t.n_Item;
        l.letd_KillerName      = (uint32)t->t_Killer->t.n_Name;
        l.letd_ExitStatus      = t->t_ExitStatus;
        Log(&l,sizeof(l),0x40000000);
    }
}


/*****************************************************************************/


void LogTaskReady(const Task *t)
{
    if (eventsLogged & LOGF_CONTROL_TASKS)
    {
    LogEventTaskReady l;

        l.letr_Header.leh_Type = LOG_TYPE_TASK_READY;
        l.letr_TaskItem        = t->t.n_Item;
        l.letr_TaskName        = (uint32)t->t.n_Name;
        Log(&l,sizeof(l),0x40000000);
    }
}


/*****************************************************************************/


void LogTaskRunning(const Task *t)
{
    if (eventsLogged & LOGF_CONTROL_TASKS)
    {
    LogEventTaskRunning l;

        l.letr_Header.leh_Type = LOG_TYPE_TASK_RUNNING;

        if (t)
        {
            l.letr_TaskItem    = t->t.n_Item;
            l.letr_TaskName    = (uint32)t->t.n_Name;
        }
        else
        {
            l.letr_TaskItem    = -1;
            l.letr_TaskName    = 0;
        }

        Log(&l,sizeof(l),0x40000000);
    }
}


/*****************************************************************************/


void LogTaskSupervisor(void)
{
    if (eventsLogged & LOGF_CONTROL_TASKS)
    {
    LogEventTaskSupervisor l;

        l.lets_Header.leh_Type = LOG_TYPE_TASK_SUPERVISOR;
        Log(&l,sizeof(l),0);
    }
}


/*****************************************************************************/


void LogTaskUser(void)
{
    if (eventsLogged & LOGF_CONTROL_TASKS)
    {
    LogEventTaskUser l;

        l.letu_Header.leh_Type = LOG_TYPE_TASK_USER;
        Log(&l,sizeof(l),0);
    }
}


/*****************************************************************************/


void LogTaskForbid(void)
{
    if (eventsLogged & LOGF_CONTROL_TASKS)
    {
    LogEventTaskForbid l;

        l.letf_Header.leh_Type = LOG_TYPE_TASK_FORBID;
        l.letf_NestCount       = CURRENTTASK->t_Forbid;
        Log(&l,sizeof(l),0);
    }
}


/*****************************************************************************/


void LogTaskPermit(void)
{
    if (eventsLogged & LOGF_CONTROL_TASKS)
    {
    LogEventTaskPermit l;

        l.letp_Header.leh_Type = LOG_TYPE_TASK_PERMIT;
        l.letp_NestCount       = CURRENTTASK->t_Forbid;
        Log(&l,sizeof(l),0);
    }
}


/*****************************************************************************/


void LogTaskPriority(const Task *t, uint8 oldPri)
{
    if (eventsLogged & LOGF_CONTROL_TASKS)
    {
    LogEventTaskPriority l;

        l.letp_Header.leh_Type = LOG_TYPE_TASK_PRIORITY;
        l.letp_TaskItem        = t->t.n_Item;
        l.letp_TaskName        = (uint32)t->t.n_Name;
        l.letp_OldPriority     = oldPri;
        l.letp_NewPriority     = t->t.n_Priority;
        Log(&l,sizeof(l),0x40000000);
    }
}


/*****************************************************************************/


void LogInterruptStart(uint32 id, const char *comment)
{
    if (eventsLogged & LOGF_CONTROL_INTERRUPTS)
    {
    LogEventInterruptStart l;

        l.leis_Header.leh_Type = LOG_TYPE_INTERRUPT_START;
        l.leis_ID              = id;
        l.leis_Description     = (uint32)comment;
        Log(&l,sizeof(l),0x40000000);
    }
}


/*****************************************************************************/


void LogInterruptDone(void)
{
    if (eventsLogged & LOGF_CONTROL_INTERRUPTS)
    {
    LogEventInterruptDone l;

        l.leid_Header.leh_Type = LOG_TYPE_INTERRUPT_DONE;
        Log(&l,sizeof(l),0);
    }
}


/*****************************************************************************/


void LogSignalSent(const Task *to, int32 signals)
{
    if (eventsLogged & LOGF_CONTROL_SIGNALS)
    {
    LogEventSignalSent l;

        l.less_Header.leh_Type = LOG_TYPE_SIGNAL_SENT;
        l.less_TaskItem        = to->t.n_Item;
        l.less_TaskName        = (uint32)to->t.n_Name;
        l.less_Signals         = signals;
        Log(&l,sizeof(l),0x40000000);
    }
}


/*****************************************************************************/


void LogSignalWait(int32 waitedSignals, int32 currentSignals)
{
    if (eventsLogged & LOGF_CONTROL_SIGNALS)
    {
    LogEventSignalWait l;

        l.lesw_Header.leh_Type = LOG_TYPE_SIGNAL_WAIT;
        l.lesw_WaitedSignals   = waitedSignals;
        l.lesw_CurrentSignals  = currentSignals;
        Log(&l,sizeof(l),0);
    }
}


/*****************************************************************************/


void LogMessageSent(const Message *msg, const MsgPort *toPort, const void *dataptr, uint32 datasize)
{
    if (eventsLogged & LOGF_CONTROL_MESSAGES)
    {
    LogEventMessageSent l;

        l.lems_Header.leh_Type = LOG_TYPE_MESSAGE_SENT;
        l.lems_MessageItem     = msg->msg.n_Item;
        l.lems_PortItem        = toPort->mp.n_Item;
        l.lems_PortName        = (uint32)toPort->mp.n_Name;
        l.lems_DataPtr         = (uint32)dataptr;
        l.lems_DataSize        = datasize;
        Log(&l,sizeof(l),0x20000000);
    }
}


/*****************************************************************************/


void LogMessageGotten(const Message *msg, const MsgPort *fromPort)
{
    if (eventsLogged & LOGF_CONTROL_MESSAGES)
    {
    LogEventMessageGotten l;

        l.lemg_Header.leh_Type = LOG_TYPE_MESSAGE_GOTTEN;
        l.lemg_MessageItem     = msg->msg.n_Item;
        l.lemg_PortItem        = fromPort->mp.n_Item;
        l.lemg_PortName        = (uint32)fromPort->mp.n_Name;
        Log(&l,sizeof(l),0x20000000);
    }
}


/*****************************************************************************/


void LogMessageReplied(const Message *msg, const void *dataptr, uint32 datasize)
{
    if (eventsLogged & LOGF_CONTROL_MESSAGES)
    {
    LogEventMessageReplied  l;
    MsgPort                *port;

        port                   = (MsgPort *)LookupItem(msg->msg_ReplyPort);
        l.lemr_Header.leh_Type = LOG_TYPE_MESSAGE_REPLIED;
        l.lemr_MessageItem     = msg->msg.n_Item;
        l.lemr_ReplyPortItem   = port->mp.n_Item;
        l.lemr_ReplyPortName   = (uint32)port->mp.n_Name;
        l.lemr_DataPtr         = (uint32)dataptr;
        l.lemr_DataSize        = datasize;
        l.lemr_Result          = msg->msg_Result;
        Log(&l,sizeof(l),0x20000000);
    }
}


/*****************************************************************************/


void LogMessageWait(const Message *msg, const MsgPort *port)
{
    if (eventsLogged & LOGF_CONTROL_MESSAGES)
    {
    LogEventMessageWait l;

        l.lemw_Header.leh_Type = LOG_TYPE_MESSAGE_WAIT;
        if (msg)
            l.lemw_MessageItem = msg->msg.n_Item;
        else
            l.lemw_MessageItem = 0;

        l.lemw_PortItem        = port->mp.n_Item;
        l.lemw_PortName        = (uint32)port->mp.n_Name;
        Log(&l,sizeof(l),0x20000000);
    }
}


/*****************************************************************************/


void LogSemaphoreLocked(const Semaphore *sem, bool readMode)
{
    if (eventsLogged & LOGF_CONTROL_SEMAPHORES)
    {
    LogEventSemaphoreLocked l;

        l.lesl_Header.leh_Type = LOG_TYPE_SEMAPHORE_LOCKED;
        l.lesl_SemaphoreItem   = sem->s.n_Item;
        l.lesl_SemaphoreName   = (uint32)sem->s.n_Name;
        l.lesl_NewNestCount    = sem->sem_NestCnt;
        l.lesl_ReadMode        = readMode;
        Log(&l,sizeof(l),0x40000000);
    }
}


/*****************************************************************************/


void LogSemaphoreUnlocked(const Semaphore *sem)
{
    if (eventsLogged & LOGF_CONTROL_SEMAPHORES)
    {
    LogEventSemaphoreUnlocked l;

        l.lesu_Header.leh_Type = LOG_TYPE_SEMAPHORE_UNLOCKED;
        l.lesu_SemaphoreItem   = sem->s.n_Item;
        l.lesu_SemaphoreName   = (uint32)sem->s.n_Name;
        l.lesu_NewNestCount    = sem->sem_NestCnt;
        Log(&l,sizeof(l),0x40000000);
    }
}


/*****************************************************************************/


void LogSemaphoreFailed(const Semaphore *sem)
{
    if (eventsLogged & LOGF_CONTROL_SEMAPHORES)
    {
    LogEventSemaphoreFailed l;

        l.lesf_Header.leh_Type = LOG_TYPE_SEMAPHORE_FAILED;
        l.lesf_SemaphoreItem   = sem->s.n_Item;
        l.lesf_SemaphoreName   = (uint32)sem->s.n_Name;
        l.lesf_NestCount       = sem->sem_NestCnt;
        l.lesf_LockerItem      = sem->sem_Locker->t.n_Item;
        l.lesf_LockerName      = (uint32)sem->sem_Locker->t.n_Name;
        Log(&l,sizeof(l),0x48000000);
    }
}


/*****************************************************************************/


void LogSemaphoreWait(const Semaphore *sem)
{
    if (eventsLogged & LOGF_CONTROL_SEMAPHORES)
    {
    LogEventSemaphoreWait l;

        l.lesw_Header.leh_Type = LOG_TYPE_SEMAPHORE_WAIT;
        l.lesw_SemaphoreItem   = sem->s.n_Item;
        l.lesw_SemaphoreName   = (uint32)sem->s.n_Name;
        l.lesw_NestCount       = sem->sem_NestCnt;
        l.lesw_LockerItem      = sem->sem_Locker->t.n_Item;
        l.lesw_LockerName      = (uint32)sem->sem_Locker->t.n_Name;
        Log(&l,sizeof(l),0x48000000);
    }
}


/*****************************************************************************/


void LogPagesAllocated(const void *addr, uint32 numBytes, bool superMem)
{
    if (eventsLogged & LOGF_CONTROL_PAGES)
    {
    LogEventPagesAllocated l;

        l.lepa_Header.leh_Type  = LOG_TYPE_PAGES_ALLOCATED;
        l.lepa_Address          = (void *)addr;
        l.lepa_NumBytes         = numBytes;
        l.lepa_SupervisorMemory = superMem;
        Log(&l,sizeof(l),0);
    }
}


/*****************************************************************************/


void LogPagesFreed(const void *addr, uint32 numBytes)
{
    if (eventsLogged & LOGF_CONTROL_PAGES)
    {
    LogEventPagesFreed l;

        l.lepf_Header.leh_Type = LOG_TYPE_PAGES_FREED;
        l.lepf_Address         = (void *)addr;
        l.lepf_NumBytes        = numBytes;
        Log(&l,sizeof(l),0);
    }
}


/*****************************************************************************/


void LogPagesGiven(const void *addr, uint32 numBytes, Task *toTask)
{
    if (eventsLogged & LOGF_CONTROL_PAGES)
    {
    LogEventPagesGiven l;

        l.lepg_Header.leh_Type   = LOG_TYPE_PAGES_GIVEN;
        l.lepg_Address           = (void *)addr;
        l.lepg_NumBytes          = numBytes;
        l.lepg_RecipientTaskItem = toTask->t.n_Item;
        l.lepg_RecipientTaskName = (uint32)toTask->t.n_Name;
        Log(&l,sizeof(l),0x10000000);
    }
}


/*****************************************************************************/


void LogItemCreated(const ItemNode *it)
{
    if (eventsLogged & LOGF_CONTROL_ITEMS)
    {
    LogEventItemCreated l;

        l.leic_Header.leh_Type = LOG_TYPE_ITEM_CREATED;
        l.leic_Item            = it->n_Item;
        l.leic_ItemName        = (uint32)it->n_Name;
        l.leic_ItemType        = it->n_Type;
        l.leic_ItemSubsysType  = it->n_SubsysType;
        Log(&l,sizeof(l),0x40000000);
    }
}


/*****************************************************************************/


void LogItemDeleted(const ItemNode *it)
{
    if (eventsLogged & LOGF_CONTROL_ITEMS)
    {
    LogEventItemDeleted l;

        l.leid_Header.leh_Type = LOG_TYPE_ITEM_DELETED;
        l.leid_Item            = it->n_Item;
        l.leid_ItemName        = (uint32)it->n_Name;
        l.leid_ItemType        = it->n_Type;
        l.leid_ItemSubsysType  = it->n_SubsysType;
        Log(&l,sizeof(l),0x40000000);
    }
}


/*****************************************************************************/


void LogItemOpened(const ItemNode *it)
{
    if (eventsLogged & LOGF_CONTROL_ITEMS)
    {
    LogEventItemOpened l;

        l.leio_Header.leh_Type = LOG_TYPE_ITEM_OPENED;
        l.leio_Item            = it->n_Item;
        l.leio_ItemName        = (uint32)it->n_Name;
        l.leio_ItemType        = it->n_Type;
        l.leio_ItemSubsysType  = it->n_SubsysType;
        Log(&l,sizeof(l),0x40000000);
    }
}


/*****************************************************************************/


void LogItemClosed(const ItemNode *it)
{
    if (eventsLogged & LOGF_CONTROL_ITEMS)
    {
    LogEventItemClosed l;

        l.leic_Header.leh_Type = LOG_TYPE_ITEM_CLOSED;
        l.leic_Item            = it->n_Item;
        l.leic_ItemName        = (uint32)it->n_Name;
        l.leic_ItemType        = it->n_Type;
        l.leic_ItemSubsysType  = it->n_SubsysType;
        Log(&l,sizeof(l),0x40000000);
    }
}


/*****************************************************************************/


void LogItemChangedOwner(const ItemNode *it, const Task *newOwner)
{
    TOUCH(newOwner);

    if (eventsLogged & LOGF_CONTROL_ITEMS)
    {
    LogEventItemChangedOwner l;

        l.leico_Header.leh_Type = LOG_TYPE_ITEM_CHANGEDOWNER;
        l.leico_Item            = it->n_Item;
        l.leico_ItemName        = (uint32)it->n_Name;
        l.leico_ItemType        = it->n_Type;
        l.leico_ItemSubsysType  = it->n_SubsysType;
        l.leico_NewOwner        = it->n_Owner;
        l.leico_NewOwnerName    = (uint32)((ItemNode *)LookupItem(it->n_Owner))->n_Name;
        Log(&l,sizeof(l),0x48000000);
    }
}


/*****************************************************************************/


void LogIOReqStarted(const IOReq *ior)
{
    if (eventsLogged & LOGF_CONTROL_IOREQS)
    {
    LogEventIOReqStarted l;

        l.leios_Header.leh_Type = LOG_TYPE_IOREQ_STARTED;
        l.leios_Item            = ior->io.n_Item;
        l.leios_ItemName        = (uint32)ior->io.n_Name;
        l.leios_DeviceName      = (uint32)ior->io_Dev->dev.n_Name;
        l.leios_Command         = ior->io_Info.ioi_Command;
        Log(&l,sizeof(l),0x60000000);
    }
}


/*****************************************************************************/


void LogIOReqCompleted(const IOReq *ior)
{
    if (eventsLogged & LOGF_CONTROL_IOREQS)
    {
    LogEventIOReqCompleted l;

        l.leioc_Header.leh_Type = LOG_TYPE_IOREQ_COMPLETED;
        l.leioc_Item            = ior->io.n_Item;
        l.leioc_ItemName        = (uint32)ior->io.n_Name;
        l.leioc_DeviceName      = (uint32)ior->io_Dev->dev.n_Name;
        l.leioc_Command         = ior->io_Info.ioi_Command;
        l.leioc_Error           = ior->io_Error;
        Log(&l,sizeof(l),0x60000000);
    }
}


/*****************************************************************************/


void LogIOReqAborted(const IOReq *ior)
{
    if (eventsLogged & LOGF_CONTROL_IOREQS)
    {
    LogEventIOReqAborted l;

        l.leioa_Header.leh_Type = LOG_TYPE_IOREQ_ABORTED;
        l.leioa_Item            = ior->io.n_Item;
        l.leioa_ItemName        = (uint32)ior->io.n_Name;
        l.leioa_DeviceName      = (uint32)ior->io_Dev->dev.n_Name;
        l.leioa_Command         = ior->io_Info.ioi_Command;
        Log(&l,sizeof(l),0x60000000);
    }
}


/*****************************************************************************/

#else
extern	int	foo;	/* Make compiler happy */
#endif /* LUMBERJACK */
