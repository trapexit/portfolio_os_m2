/* @(#) dumplumberjackbuffer.c 96/04/19 1.14 */

#include <kernel/types.h>
#include <kernel/lumberjack.h>
#include <kernel/time.h>
#include <stdio.h>

/**
|||	AUTODOC -public -class Kernel -group Lumberjack -name DumpLumberjackBuffer
|||	Parse and display the contents of a Lumberjack buffer.
|||
|||	  Synopsis
|||
|||	    void DumpLumberjackBuffer(const char *banner,
|||	                              const LumberjackBuffer *lb);
|||
|||	  Description
|||
|||	    Lumberjack, the Portfolio logging service, maintains a list of
|||	    buffers into which it logs events during system execution.
|||
|||	    This function takes a pointer to a LumberjackBuffer and decodes
|||	    the information it contains, and displays the results to the
|||	    debugging terminal. You obtain a pointer to a LumberjackBuffer
|||	    by caling ObtainLumberjackBuffer().
|||
|||	  Arguments
|||
|||	    banner
|||	        A string to display before dumping the contents of the
|||	        buffer. This may be NULL in which case no banner string will
|||	        be printed.
|||
|||	    lb
|||	        The buffer to dump the contents of, as obtained from
|||	        ObtainLumberjackBuffer(). This may be NULL, in which case
|||	        this function does nothing.
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
|||	    <kernel/lumberjack.h>, libc.a
|||
|||	  See Also
|||
|||	    ObtainLumberjackBuffer(), ReleaseLumberjackBuffer(),
|||	    ControlLumberjack(), LogEvent()
|||
**/

static const char * const logTypeNames[] =
{
    NULL,
    "*** OVERFLOW, Events Lost",
    "User Log Entry",
    "Task Created",
    "Task Died",
    "Task Became Ready",
    "Task Became Running",
    "Task Entered To Supervisor Mode",
    "Task Returned To User Mode",
    "Task Disabled Task Switching",
    "Task Enabled Task Switching",
    "Task Changed Priority",
    "Dispatching interrupt handler",
    "Done servicing interrupt",
    "Signals Sent",
    "Waiting For Signals",
    "Message Sent",
    "Message Gotten",
    "Message Replied",
    "Waiting For Messages",
    "Semaphore Locked",
    "Semaphore Unlocked",
    "Semaphore Locking Attempt Failed Because Already Locked",
    "Waiting For A Semaphore",
    "Allocated Pages Of Memory",
    "Freed Pages Of Memory",
    "Gave Pages Of Memory",
    "Item Created",
    "Item Deleted",
    "Item Opened",
    "Item Closed",
    "Item Changed Owner",
    "IOReq Started",
    "IOReq Completed",
    "IOReq Aborted"
};

#define STR(offset) (offset ? ((char *)((uint32)lb->lb_BufferData + (offset)))\
                            : "<null>")


void DumpLumberjackBuffer(const char *banner, const LumberjackBuffer *lb)
{
LogEventHeader            *leh;
LogEventBufferOverflow    *bufferOverflow;
LogEventUser              *user;
LogEventTaskCreated       *taskCreated;
LogEventTaskDied          *taskDied;
LogEventTaskReady         *taskReady;
LogEventTaskRunning       *taskRunning;
LogEventTaskForbid        *taskForbid;
LogEventTaskPermit        *taskPermit;
LogEventTaskPriority      *taskPriority;
LogEventInterruptStart    *interruptStart;
LogEventSignalSent        *signalSent;
LogEventSignalWait        *signalWait;
LogEventMessageSent       *messageSent;
LogEventMessageGotten     *messageGotten;
LogEventMessageReplied    *messageReplied;
LogEventMessageWait       *messageWait;
LogEventSemaphoreLocked   *semaphoreLocked;
LogEventSemaphoreUnlocked *semaphoreUnlocked;
LogEventSemaphoreFailed   *semaphoreFailed;
LogEventSemaphoreWait     *semaphoreWait;
LogEventPagesAllocated    *pagesAllocated;
LogEventPagesFreed        *pagesFreed;
LogEventPagesGiven        *pagesGiven;
LogEventItemCreated       *itemCreated;
LogEventItemDeleted       *itemDeleted;
LogEventItemOpened        *itemOpened;
LogEventItemClosed        *itemClosed;
LogEventItemChangedOwner  *itemChangedOwner;
LogEventIOReqStarted      *ioreqStarted;
LogEventIOReqCompleted    *ioreqCompleted;
LogEventIOReqAborted      *ioreqAborted;
TimeVal                    tv;

    if (!lb)
        return;

    if (banner)
        printf("%s\n",banner);

    leh = (LogEventHeader *)lb->lb_BufferData;
    while (leh->leh_Type != LOG_TYPE_BUFFER_END)
    {
        /* Unions? We don't need no stinkin' unions! */
        bufferOverflow     = (LogEventBufferOverflow    *)leh;
        user               = (LogEventUser              *)leh;
        taskCreated        = (LogEventTaskCreated       *)leh;
        taskDied           = (LogEventTaskDied          *)leh;
        taskReady          = (LogEventTaskReady         *)leh;
        taskRunning        = (LogEventTaskRunning       *)leh;
        taskForbid         = (LogEventTaskForbid        *)leh;
        taskPermit         = (LogEventTaskPermit        *)leh;
        taskPriority       = (LogEventTaskPriority      *)leh;
        interruptStart     = (LogEventInterruptStart    *)leh;
        signalSent         = (LogEventSignalSent        *)leh;
        signalWait         = (LogEventSignalWait        *)leh;
        messageSent        = (LogEventMessageSent       *)leh;
        messageGotten      = (LogEventMessageGotten     *)leh;
        messageReplied     = (LogEventMessageReplied    *)leh;
        messageWait        = (LogEventMessageWait       *)leh;
        semaphoreLocked	   = (LogEventSemaphoreLocked   *)leh;
        semaphoreUnlocked  = (LogEventSemaphoreUnlocked *)leh;
        semaphoreFailed	   = (LogEventSemaphoreFailed   *)leh;
        semaphoreWait	   = (LogEventSemaphoreWait     *)leh;
        pagesAllocated	   = (LogEventPagesAllocated    *)leh;
        pagesFreed	   = (LogEventPagesFreed        *)leh;
        pagesGiven	   = (LogEventPagesGiven        *)leh;
        itemCreated        = (LogEventItemCreated       *)leh;
        itemDeleted        = (LogEventItemDeleted       *)leh;
        itemOpened         = (LogEventItemOpened        *)leh;
        itemClosed         = (LogEventItemClosed        *)leh;
        itemChangedOwner   = (LogEventItemChangedOwner  *)leh;
        ioreqStarted       = (LogEventIOReqStarted      *)leh;
        ioreqCompleted     = (LogEventIOReqCompleted    *)leh;
        ioreqAborted       = (LogEventIOReqAborted      *)leh;

        printf("\n-------------\nEvent Type    : ");
        if (leh->leh_Type < sizeof(logTypeNames) / sizeof(logTypeNames[0]))
            printf("%s\n",logTypeNames[leh->leh_Type]);
        else
            printf("<%d>\n",leh->leh_Type);

        ConvertTimerTicksToTimeVal(&leh->leh_TimeStamp,&tv);
        printf("Event Time    : %d.%06d seconds\n",tv.tv_Seconds,tv.tv_Microseconds);

        if (leh->leh_TaskItem == -1)
        {
            printf("Current Task  : <idle loop>\n");
        }
        else
        {
            printf("Current Task  : %s (item $%05x)\n",STR(leh->leh_TaskName),leh->leh_TaskItem);
        }

        switch (leh->leh_Type)
        {
            case LOG_TYPE_BUFFER_OVERFLOW   : ConvertTimerTicksToTimeVal(&bufferOverflow->lebo_OverflowRecovery,&tv);
                                              printf("Recovery Time : %d.%06d seconds\n",tv.tv_Seconds,tv.tv_Microseconds);
                                              break;

            case LOG_TYPE_USER              : printf("Description   : %s\n",STR(user->leu_Description));
                                              break;

            case LOG_TYPE_TASK_CREATED      : printf("Task          : %s (item $%05x)\n",STR(taskCreated->letc_TaskName),taskCreated->letc_TaskItem);
                                              printf("Task Addr     : $%08x\n",taskCreated->letc_TaskAddr);
                                              printf("Stack Size    : %d ($%x)\n",taskCreated->letc_StackSize,taskCreated->letc_StackSize);
                                              printf("Task.t.n_Flags: $%02x\n",taskCreated->letc_NodeFlags);
                                              printf("Task.t_Flags  : $%02x\n",taskCreated->letc_TaskFlags);
                                              printf("Priority      : %d ($02x)\n",taskCreated->letc_Priority);
                                              break;

            case LOG_TYPE_TASK_DIED         : printf("Killer        : %s (item $%05x)\n",STR(taskDied->letd_KillerName),taskDied->letd_KillerItem);
                                              printf("Exit Status   : %d ($%x)\n",taskDied->letd_ExitStatus);
                                              break;

            case LOG_TYPE_TASK_READY        : printf("Task          : %s (item $%05x)\n",STR(taskReady->letr_TaskName),taskReady->letr_TaskItem);
                                              break;

            case LOG_TYPE_TASK_RUNNING      : printf("Task          : %s (item $%05x)\n",STR(taskRunning->letr_TaskName),taskRunning->letr_TaskItem);
                                              break;

            case LOG_TYPE_TASK_SUPERVISOR   :
            case LOG_TYPE_TASK_USER         : break;

            case LOG_TYPE_TASK_FORBID       : printf("New Nest Count: %d\n",taskForbid->letf_NestCount);
                                              break;

            case LOG_TYPE_TASK_PERMIT       : printf("New Nest Count: %d\n",taskPermit->letp_NestCount);
                                              break;

            case LOG_TYPE_TASK_PRIORITY     : printf("Task          : %s (item $%05x)\n",STR(taskPriority->letp_TaskName),taskPriority->letp_TaskItem);
                                              printf("Old Priority  : %u\n",taskPriority->letp_OldPriority);
                                              printf("New Priority  : %u\n",taskPriority->letp_NewPriority);
                                              break;

            case LOG_TYPE_INTERRUPT_START   : printf("ID            : %d ($%x)\n",interruptStart->leis_ID,interruptStart->leis_ID);
                                              printf("Description   : %s\n",STR(interruptStart->leis_Description));
                                              break;

            case LOG_TYPE_INTERRUPT_DONE    : break;

            case LOG_TYPE_SIGNAL_SENT       : printf("To Task       : %s (item $%05x)\n",STR(signalSent->less_TaskName),signalSent->less_TaskItem);
                                              printf("Signals Sent  : $%08x\n",signalSent->less_Signals);
                                              break;

            case LOG_TYPE_SIGNAL_WAIT       : printf("Received Sigs : $%08x\n",signalWait->lesw_CurrentSignals);
                                              printf("Waited Sigs   : $%08x\n",signalWait->lesw_WaitedSignals);
                                              break;

            case LOG_TYPE_MESSAGE_SENT      : printf("Message Item  : $%05x\n",messageSent->lems_MessageItem);
                                              printf("To Port Item  : $%05x\n",messageSent->lems_PortItem);
                                              printf("To Port Name  : %s\n",STR(messageSent->lems_PortName));
                                              printf("msg_DataPtr   : %d ($%x)\n",messageSent->lems_DataPtr,messageSent->lems_DataPtr);
                                              printf("msg_DataSize  : %d ($%x)\n",messageSent->lems_DataSize,messageSent->lems_DataSize);
                                              break;

            case LOG_TYPE_MESSAGE_GOTTEN    : printf("Message Item  : $%05x\n",messageGotten->lemg_MessageItem);
                                              printf("From Port Item: $%05x\n",messageGotten->lemg_PortItem);
                                              printf("From Port Name: %s\n",STR(messageGotten->lemg_PortName));
                                              break;

            case LOG_TYPE_MESSAGE_REPLIED   : printf("Message Item  : $%05x\n",messageReplied->lemr_MessageItem);
                                              printf("To Port Item  : $%05x\n",messageReplied->lemr_ReplyPortItem);
                                              printf("To Port Name  : %s\n",STR(messageReplied->lemr_ReplyPortName));
                                              printf("msg_DataPtr   : %d ($%x)\n",messageReplied->lemr_DataPtr,messageReplied->lemr_DataPtr);
                                              printf("msg_DataSize  : %d ($%x)\n",messageReplied->lemr_DataSize,messageReplied->lemr_DataSize);
                                              printf("msg_Result    : %d ($%x)\n",messageReplied->lemr_Result,messageReplied->lemr_Result);
                                              break;

            case LOG_TYPE_MESSAGE_WAIT      : printf("Message Item  : $%05x\n",messageWait->lemw_MessageItem);
                                              printf("On Port Item  : $%05x\n",messageWait->lemw_PortItem);
                                              printf("On Port Name  : %s\n",STR(messageWait->lemw_PortName));
                                              break;

            case LOG_TYPE_SEMAPHORE_LOCKED  : printf("Semaphore Item: $%05x\n",semaphoreLocked->lesl_SemaphoreItem);
                                              printf("Semaphore Name: %s\n",STR(semaphoreLocked->lesl_SemaphoreName));
                                              printf("New Nest Count: %d\n",semaphoreLocked->lesl_NewNestCount);
                                              printf("Lock Mode     : %s\n",(semaphoreLocked->lesl_ReadMode ? "Shared" : "Exclusive"));
                                              break;

            case LOG_TYPE_SEMAPHORE_UNLOCKED: printf("Semaphore Item: $%05x\n",semaphoreUnlocked->lesu_SemaphoreItem);
                                              printf("Semaphore Name: %s\n",STR(semaphoreUnlocked->lesu_SemaphoreName));
                                              printf("New Nest Count: %d\n",semaphoreUnlocked->lesu_NewNestCount);
                                              break;

            case LOG_TYPE_SEMAPHORE_FAILED  : printf("Semaphore Item: $%05x\n",semaphoreFailed->lesf_SemaphoreItem);
                                              printf("Semaphore Name: %s\n",STR(semaphoreFailed->lesf_SemaphoreName));
                                              printf("Nest Count    : %d\n",semaphoreFailed->lesf_NestCount);
                                              printf("Locker Task   : %s (item $%05x)\n",STR(semaphoreFailed->lesf_LockerName),semaphoreFailed->lesf_LockerItem);
                                              break;

            case LOG_TYPE_SEMAPHORE_WAIT    : printf("Semaphore Item: $%05x\n",semaphoreWait->lesw_SemaphoreItem);
                                              printf("Semaphore Name: %s\n",STR(semaphoreWait->lesw_SemaphoreName));
                                              printf("Nest Count    : %d\n",semaphoreWait->lesw_NestCount);
                                              printf("Locker Task   : %s (item $%05x)\n",STR(semaphoreWait->lesw_LockerName),semaphoreWait->lesw_LockerItem);
                                              break;

            case LOG_TYPE_PAGES_ALLOCATED   : printf("Address       : $%08x\n",pagesAllocated->lepa_Address);
                                              printf("Size In Bytes : %d ($%x)\n",pagesAllocated->lepa_NumBytes,pagesAllocated->lepa_NumBytes);
                                              printf("Memory List   : %s\n",pagesAllocated->lepa_SupervisorMemory ? "Supervisor" : "User");
                                              break;

            case LOG_TYPE_PAGES_FREED       : printf("Address       : $%08x\n",pagesFreed->lepf_Address);
                                              printf("Size In Bytes : %d ($%x)\n",pagesFreed->lepf_NumBytes,pagesFreed->lepf_NumBytes);
                                              break;

            case LOG_TYPE_PAGES_GIVEN       : printf("Address       : $%08x\n",pagesGiven->lepg_Address);
                                              printf("Size In Bytes : %d ($%x)\n",pagesGiven->lepg_NumBytes,pagesFreed->lepf_NumBytes);
                                              printf("Given To Task : %s (item $%05x)\n",STR(pagesGiven->lepg_RecipientTaskName),pagesGiven->lepg_RecipientTaskItem);
                                              break;

            case LOG_TYPE_ITEM_CREATED      : printf("Item Number   : $%05x\n",itemCreated->leic_Item);
                                              printf("Item Name     : %s\n",STR(itemCreated->leic_ItemName));
                                              printf("SubsysType    : %d\n",itemCreated->leic_ItemSubsysType);
                                              printf("Type          : %d\n",itemCreated->leic_ItemType);
                                              break;

            case LOG_TYPE_ITEM_DELETED      : printf("Item Number   : $%05x\n",itemDeleted->leid_Item);
                                              printf("Item Name     : %s\n",STR(itemDeleted->leid_ItemName));
                                              printf("SubsysType    : %d\n",itemDeleted->leid_ItemSubsysType);
                                              printf("Type          : %d\n",itemDeleted->leid_ItemType);
                                              break;

            case LOG_TYPE_ITEM_OPENED       : printf("Item Number   : $%05x\n",itemOpened->leio_Item);
                                              printf("Item Name     : %s\n",STR(itemOpened->leio_ItemName));
                                              printf("SubsysType    : %d\n",itemOpened->leio_ItemSubsysType);
                                              printf("Type          : %d\n",itemOpened->leio_ItemType);
                                              break;

            case LOG_TYPE_ITEM_CLOSED       : printf("Item Number   : $%05x\n",itemClosed->leic_Item);
                                              printf("Item Name     : %s\n",STR(itemClosed->leic_ItemName));
                                              printf("SubsysType    : %d\n",itemClosed->leic_ItemSubsysType);
                                              printf("Type          : %d\n",itemClosed->leic_ItemType);
                                              break;

            case LOG_TYPE_ITEM_CHANGEDOWNER : printf("Item Number   : $%05x\n",itemChangedOwner->leico_Item);
                                              printf("Item Name     : %s\n",STR(itemChangedOwner->leico_ItemName));
                                              printf("SubsysType    : %d\n",itemChangedOwner->leico_ItemSubsysType);
                                              printf("Type          : %d\n",itemChangedOwner->leico_ItemType);
                                              printf("New Owner     : %s (item $%05)\n",STR(itemChangedOwner->leico_NewOwnerName),itemChangedOwner->leico_NewOwner);
                                              break;

            case LOG_TYPE_IOREQ_STARTED     : printf("Item Number   : $%05x\n",ioreqStarted->leios_Item);
                                              printf("Item Name     : %s\n",STR(ioreqStarted->leios_ItemName));
                                              printf("Device Name   : %s\n",STR(ioreqStarted->leios_DeviceName));
                                              printf("IO Command    : $%x\n",ioreqStarted->leios_Command);
                                              break;

            case LOG_TYPE_IOREQ_COMPLETED   : printf("Item Number   : $%05x\n",ioreqCompleted->leioc_Item);
                                              printf("Item Name     : %s\n",STR(ioreqCompleted->leioc_ItemName));
                                              printf("Device Name   : %s\n",STR(ioreqCompleted->leioc_DeviceName));
                                              printf("IO Command    : $%x\n",ioreqCompleted->leioc_Command);
                                              printf("IO Error      : %d\n",ioreqCompleted->leioc_Error);
                                              break;

            case LOG_TYPE_IOREQ_ABORTED     : printf("Item Number   : $%05x\n",ioreqAborted->leioa_Item);
                                              printf("Item Name     : %s\n",STR(ioreqAborted->leioa_ItemName));
                                              printf("Device Name   : %s\n",STR(ioreqAborted->leioa_DeviceName));
                                              printf("IO Command    : $%x\n",ioreqAborted->leioa_Command);
                                              break;
        }

        leh = (LogEventHeader *)((uint32)leh + leh->leh_NextEvent);
    }
}
