/* @(#) exceptions.c 96/11/07 1.59 */

#include <kernel/types.h>
#include <kernel/task.h>
#include <kernel/kernel.h>
#include <kernel/panic.h>
#include <kernel/operror.h>
#include <kernel/internalf.h>
#include <loader/loader3do.h>
#include <kernel/monitor.h>
#include <hardware/PPC.h>
#include <hardware/PPCasm.h>
#include <string.h>
#include <stdio.h>


/*****************************************************************************/


#ifdef DEBUG
#define DBUG(x)     printf x
#else
#define DBUG(x)
#endif


/*****************************************************************************/


#define DEBUG_HARD_BREAKPOINT 0x00000000
#define DEBUG_SOFT_BREAKPOINT 0x00000001

void SuperMasterRequest(MasterActions action, uint32 arg1, uint32 arg2, uint32 arg3);


/*****************************************************************************/


Err externalRegisterUserExceptionHandler(Item task, Item port)
{
char   msgName[75];
Task  *t;
Item   msg;
TagArg tags[4];

    t = (Task *)CheckItem(task, KERNELNODE, TASKNODE);
    if (t == NULL)
        return BADITEM;

    if (IsPriv(t) && !IsPriv(CURRENTTASK))
    {
        /* can't have a non-privileged task handling a privileged one */
        return BADPRIV;
    }

    msg = t->t_UserExceptionMsg;
    if (msg >= 0)
    {
        /* if a handler was previously installed, nuke it */
        t->t_UserExceptionMsg = -1;
        internalDeleteMsg(MESSAGE(msg), t);
    }

    if (port == 0)
    {
        /* caller just wanted to remove the handler */
        return 0;
    }

    sprintf(msgName, "%.50s Exception Notification", t->t.n_Name);

    tags[0].ta_Tag = TAG_ITEM_PRI;
    tags[0].ta_Arg = (void *)t->t.n_Priority;
    tags[1].ta_Tag = CREATEMSG_TAG_REPLYPORT;
    tags[1].ta_Arg = (void *)port;
    tags[2].ta_Tag = TAG_ITEM_NAME;
    tags[2].ta_Arg = (void *)msgName;
    tags[3].ta_Tag = TAG_END;
    msg = externalCreateMsg((Msg *)-1, tags);
    if (msg < 0)
        return msg;

    t->t_UserExceptionMsg = msg;
    return 0;
}


/*****************************************************************************/


Err externalControlUserExceptions(uint32 exceptions, bool captured)
{
Task   *ct;
uint32  fpscr;
uint32  oldints;

    if (exceptions & ~(USEREXC_TRAP |
                       USEREXC_FP_INVALID_OP |
                       USEREXC_FP_OVERFLOW |
                       USEREXC_FP_UNDERFLOW |
                       USEREXC_FP_ZERODIVIDE |
                       USEREXC_FP_INEXACT))
    {
        /* illegal bit specified */
        return BADEXCEPTIONNUM;
    }

    ct = CURRENTTASK;
    if (captured)
        ct->t_CapturedExceptions |= exceptions;
    else
        ct->t_CapturedExceptions &= ~exceptions;

    if (ct->t_CapturedExceptions & (USEREXC_FP_INVALID_OP |
                                    USEREXC_FP_OVERFLOW |
                                    USEREXC_FP_UNDERFLOW |
                                    USEREXC_FP_ZERODIVIDE |
                                    USEREXC_FP_INEXACT))
    {
        /* trap on FP operations */
        _mtmsr(_mfmsr() | MSR_FE0 | MSR_FE1);
    }
    else
    {
        /* don't trap on FP operations */
        _mtmsr(_mfmsr() & ~(MSR_FE0 | MSR_FE1));
    }

    oldints = Disable();
    if (KB_FIELD(kb_FPOwner) == ct)
    {
        /* FP state is loaded in the FPU, save it out to memory */
        SaveFPRegBlock(&ct->t_FPRegisterSave);

        /* cause the FP state to be reloaded from the TCB the next time this task
         * does an FP operation.
         */
        KB_FIELD(kb_FPOwner) = NULL;
        _mtmsr(_mfmsr() & (~MSR_FP));
    }
    Enable(oldints);

    fpscr  = ct->t_FPRegisterSave.fprb_FPSCR;
    fpscr &= ~(FPSCR_VE |
               FPSCR_OE |
               FPSCR_UE |
               FPSCR_ZE |
               FPSCR_XE);

    if (ct->t_CapturedExceptions & USEREXC_FP_INVALID_OP)
        fpscr |= FPSCR_VE;

    if (ct->t_CapturedExceptions & USEREXC_FP_OVERFLOW)
        fpscr |= FPSCR_OE;

    if (ct->t_CapturedExceptions & USEREXC_FP_UNDERFLOW)
        fpscr |= FPSCR_UE;

    if (ct->t_CapturedExceptions & USEREXC_FP_ZERODIVIDE)
        fpscr |= FPSCR_ZE;

    if (ct->t_CapturedExceptions & USEREXC_FP_INEXACT)
        fpscr |= FPSCR_XE;

    /* new FPSCR bits for this task */
    ct->t_FPRegisterSave.fprb_FPSCR = fpscr;

    return 0;
}


/*****************************************************************************/


Err externalCompleteUserException(Item task, const RegBlock *rb, const FPRegBlock *fprb)
{
Task   *ct;
Task   *t;
uint32  msr;
uint32  oldints;

    t = (Task *)CheckItem(task, KERNELNODE, TASKNODE);
    if (t == NULL)
        return BADITEM;

    if ((t->t_Flags & TASK_EXCEPTION) == 0)
    {
        /* The task is not currently processing an exception */
        return BADITEM;
    }

    ct = CURRENTTASK;

    if (IsPriv(t) && !IsPriv(ct))
    {
        /* can't have a non-privileged task handling a privileged one */
        return BADPRIV;
    }

    if (rb && !IsMemReadable(rb, sizeof(RegBlock)))
        return BADPTR;

    if (fprb && !IsMemReadable(fprb, sizeof(FPRegBlock)))
        return BADPTR;

    /* done with the exception processing */
    t->t_Flags &= (~TASK_EXCEPTION);

    /* only allow register modification if the task was in user-mode when it
     * triggered the exception, or if the caller is privileged.
     */
    if ((t->t_RegisterSave.rb_MSR & MSR_PR) || IsPriv(ct))
    {
        /* Give the task the register set that the handler specified, except for
         * the MSR bits. Letting user-code muck with the MSR bits would let them
         * enter supervisor mode trivially.
         */
        if (rb)
        {
            msr               = t->t_RegisterSave.rb_MSR;
            t->t_RegisterSave = *rb;

            if (!IsPriv(ct))
                t->t_RegisterSave.rb_MSR = msr;
        }
        else
        {
            /* skip over offending instruction */
            t->t_RegisterSave.rb_PC += 4;
        }

        if (fprb)
        {
            /* handler supplied replacement register set */
            t->t_FPRegisterSave = *fprb;
        }
        else
        {
            /* handler didn't supply new registers. Just clear the FP exception
             * bits from FPSCR
             */
            t->t_FPRegisterSave.fprb_FPSCR &= ~(FPSCR_FX |
                                                FPSCR_FEX |
                                                FPSCR_VX |
                                                FPSCR_OX |
                                                FPSCR_UX |
                                                FPSCR_ZX |
                                                FPSCR_XX |
                                                FPSCR_VXNAN |
                                                FPSCR_VXISI |
                                                FPSCR_VXIDI |
                                                FPSCR_VXZDZ |
                                                FPSCR_VXIMZ |
                                                FPSCR_VXVC |
                                                FPSCR_VXSOFT |
                                                FPSCR_VXSQRT |
                                                FPSCR_VXCVI);
        }
    }

    /* schedule the task for execution */
    oldints = Disable();
    ResumeTask(t);
    Enable(oldints);

    return 0;
}


/*****************************************************************************/


/* If this function returns, it means that the exception is not being
 * serviced by a user-handler and should therefore be handled by the kernel
 * itself.
 */
static void HandleUserExceptions(uint32 exceptions)
{
Task    *ct;
Message *msg;

    ct = CURRENTTASK;

    if ((ct->t_CapturedExceptions & exceptions) == exceptions)
    {
        /* all of the exceptions are being captured */
        ct->t_UserExceptionContext.uec_Task       = ct->t.n_Item;
        ct->t_UserExceptionContext.uec_Trigger    = exceptions;
        ct->t_UserExceptionContext.uec_RegBlock   = &ct->t_RegisterSave;
        ct->t_UserExceptionContext.uec_FPRegBlock = &ct->t_FPRegisterSave;

        /* inform the handler of the situation */
        msg = MESSAGE(ct->t_UserExceptionMsg);
        if (msg)
        {
            if (internalReplyMsg(msg, 0, &ct->t_UserExceptionContext, sizeof(UserExceptionContext)) >= 0)
            {
                ct->t_Flags |= TASK_EXCEPTION;

                /* suspend the offending task until the handler kicks it */
                SuspendTask(ct);

                /* find someone else to run */
                ScheduleNewTask();

                /* execution never comes back here */
            }
        }
    }

    /* exception not handled by a user-mode handler... */
}


/*****************************************************************************/


#ifndef BUILD_DEBUGGER
static void Reboot(void)
{
    _esa();                 /* make sure we're super                 */
    Disable();              /* keep those pesky interrupts away      */
    _mttcr(0x3c000000);     /* load up watchdog for 0.25sec interval */
}
#endif


/*****************************************************************************/


void OSPanic(const char *msg, uint32 panicerr, char *file, uint32 line)
{
Task *ct;

    if (file)
    {
        if (msg)
            printf("\n### %s Panic in file %s, line %d\n",msg,file,line);
        else
            printf("\n### Panic in file %s, line %d\n",file,line);
    }

    printf("### ");
    PrintfSysErr(panicerr);

    ct = CURRENTTASK;
    if (ct)
        printf("### Current task is '%s'\n", ct->t.n_Name);
    else
        printf("### No current task\n");

#ifndef BUILD_DEBUGGER
    Reboot();
#else
    printf("Entering infinite loop...\n");
    while (1);
#endif
}


/*****************************************************************************/


void HandleSlaveException(CrashCauses cause)
{
uint32 addr;
uint32 faultAddr;
uint32 srr1;
uint32 fpscr;

    addr = _mfsrr0();
    srr1 = _mfsrr1();

    _mtmsr(_mfmsr() | MSR_FP);
    SaveFPRegBlock(&KB_FIELD(kb_SlaveState)->ss_FPRegSave);

    if (cause == CRASH_ProgramException)
    {
        if (srr1 & SRR1_FPEXC)
        {
            _ClrFPExc();
        }
    }

    if (cause != CRASH_TraceException)
    {
        if ((srr1 & SRR1_ILLINS) && (*(uint32 *)addr == DEBUG_HARD_BREAKPOINT))
        {
            printf("\n### Breakpoint on slave CPU\n");
        }
        else if ((srr1 & SRR1_ILLINS) && (*(uint32 *)addr == DEBUG_SOFT_BREAKPOINT))
        {
            printf("\n### DebugBreakpoint() from slave CPU\n");
        }
        else
        {
            printf("\n### %s\n", crashNames[cause]);
            switch (cause)
            {
                case CRASH_DataAccess      : faultAddr = _mfdar();
                                             if (faultAddr == 0)
                                                 faultAddr = _mfdmiss();

                                             if (_mfdsisr() & 0x02000000)
                                                 printf("### illegal attempt to write location 0x%x\n", faultAddr);
                                             else
                                                 printf("### illegal attempt to read location 0x%x\n", faultAddr);
                                             break;

                case CRASH_ProgramException: if (srr1 & SRR1_ILLINS)
                                             {
                                                 printf("### attempt to execute an illegal instruction at location 0x%x\n", addr);
                                             }
                                             else if (srr1 & SRR1_PRIVINS)
                                             {
                                                 printf("### attempt to execute a privileged instruction at location 0x%x\n", addr);
                                             }
                                             else if (srr1 & SRR1_FPINS)
                                             {
                                                 printf("### unsupported floating-point emulation trap\n");
                                             }
                                             else if (srr1 & SRR1_FPEXC)
                                             {
                                                 printf("### ");
                                                 fpscr = KB_FIELD(kb_SlaveState)->ss_FPRegSave.fprb_FPSCR;

                                                 if (fpscr & FPSCR_VX)
                                                     printf("Invalid FP op, ");

                                                 if (fpscr & FPSCR_OX)
                                                     printf("FP overflow, ");

                                                 if (fpscr & FPSCR_UX)
                                                     printf("FP underflow, ");

                                                 if (fpscr & FPSCR_ZX)
                                                     printf("FP divide by zero, ");

                                                 if (fpscr & FPSCR_XX)
                                                     printf("FP inexact result, ");

                                                 printf("FPSCR 0x%08x\n", fpscr);
                                             }
                                             break;
            }
        }

#ifdef BUILD_DEBUGGER
        printf("### Slave CPU execution context\n");
#endif
    }

#ifdef BUILD_DEBUGGER
    FlushDCacheAll(0);
    SuperMasterRequest(MASTER_MONITOR, cause, addr, 0);
    FlushDCacheAll(0);
    RestoreFPRegBlock(&KB_FIELD(kb_SlaveState)->ss_FPRegSave);
#else
    internalSlaveExit(-1);
#endif
}


/*****************************************************************************/


void HandleMasterException(CrashCauses cause)
{
Task   *ct;
uint32  addr;
uint32  faultAddr;
uint32  srr1;
Module *module;
bool    found;
uint32  exceptions;
uint32  fpscr;
#ifdef BUILD_DEBUGGER
bool    breakpoint;
#endif

    ct         = CURRENTTASK;
    addr       = _mfsrr0();
    srr1       = _mfsrr1();
#ifdef BUILD_DEBUGGER
    breakpoint = FALSE;
#endif

    if (ct)
    {
        if (KB_FIELD(kb_FPOwner) == ct)
        {
            /* save the FP state into the TCB */
            _mtmsr(_mfmsr() | MSR_FP);
            SaveFPRegBlock(&ct->t_FPRegisterSave);

            /* cause the FP state to be reloaded from the TCB the next time this task
             * does an FP operation.
             */
            KB_FIELD(kb_FPOwner) = NULL;
            ct->t_RegisterSave.rb_MSR &= (~MSR_FP);
        }
    }

    if (cause == CRASH_ProgramException)
    {
        if (ct)
        {
            if (srr1 & SRR1_TRAP)
                HandleUserExceptions(USEREXC_TRAP);

            if (srr1 & SRR1_FPEXC)
            {
                _ClrFPExc();

                if ((srr1 & SRR1_FPINS) == 0)
                {
                    exceptions = 0;
                    fpscr      = ct->t_FPRegisterSave.fprb_FPSCR;

                    if ((fpscr & FPSCR_VX) && (fpscr & FPSCR_VE))
                        exceptions |= USEREXC_FP_INVALID_OP;

                    if ((fpscr & FPSCR_OX) && (fpscr & FPSCR_OE))
                        exceptions |= USEREXC_FP_OVERFLOW;

                    if ((fpscr & FPSCR_UX) && (fpscr & FPSCR_UE))
                        exceptions |= USEREXC_FP_UNDERFLOW;

                    if ((fpscr & FPSCR_ZX) && (fpscr & FPSCR_ZE))
                        exceptions |= USEREXC_FP_ZERODIVIDE;

                    if ((fpscr & FPSCR_XX) && (fpscr & FPSCR_XE))
                        exceptions |= USEREXC_FP_INEXACT;

                    if (exceptions)
                        HandleUserExceptions(exceptions);
                }
            }
        }
    }

    if (cause != CRASH_TraceException)
    {
        if ((srr1 & SRR1_ILLINS) && (*(uint32 *)addr == DEBUG_HARD_BREAKPOINT))
        {
            printf("\n### Breakpoint in task '%s'\n", ct->t.n_Name);
#ifdef BUILD_DEBUGGER
            breakpoint = TRUE;
#endif
        }
        else if ((srr1 & SRR1_ILLINS) && (*(uint32 *)addr == DEBUG_SOFT_BREAKPOINT))
        {
            printf("\n### DebugBreakpoint() from task '%s'\n", ct->t.n_Name);
#ifdef BUILD_DEBUGGER
            breakpoint = TRUE;
#endif
        }
        else
        {
            printf("\n### %s\n", crashNames[cause]);
            switch (cause)
            {
                case CRASH_DataAccess      : faultAddr = _mfdar();
                                             if (faultAddr == 0)
                                                 faultAddr = _mfdmiss();

                                             if (_mfdsisr() & 0x02000000)
                                                 printf("### illegal attempt to write location 0x%x\n", faultAddr);
                                             else
                                                 printf("### illegal attempt to read location 0x%x\n", faultAddr);
                                             break;

                case CRASH_ProgramException: if (srr1 & SRR1_ILLINS)
                                             {
                                                 printf("### attempt to execute an illegal instruction at location 0x%x\n", addr);
                                             }
                                             else if (srr1 & SRR1_PRIVINS)
                                             {
                                                 printf("### attempt to execute a privileged instruction at location 0x%x\n", addr);
                                             }
                                             else if (srr1 & SRR1_FPINS)
                                             {
                                                 printf("### unsupported floating-point emulation trap\n");
                                             }
                                             else if (srr1 & SRR1_FPEXC)
                                             {
                                                 printf("### ");
                                                 if (ct)
                                                     fpscr = ct->t_FPRegisterSave.fprb_FPSCR;
                                                 else
                                                     fpscr = 0;  /* FIXME: should read FPSCR directly from CPU */

                                                 if (fpscr & FPSCR_VX)
                                                     printf("Invalid FP op, ");

                                                 if (fpscr & FPSCR_OX)
                                                     printf("FP overflow, ");

                                                 if (fpscr & FPSCR_UX)
                                                     printf("FP underflow, ");

                                                 if (fpscr & FPSCR_ZX)
                                                     printf("FP divide by zero, ");

                                                 if (fpscr & FPSCR_XX)
                                                     printf("FP inexact result, ");

                                                 printf("FPSCR 0x%08x\n", fpscr);
                                             }
                                             break;
            }
        }

        found = FALSE;
        ScanList(&KB_FIELD(kb_Modules), module, Module)
        {
            if ((addr >= (uint32)module->li->codeBase)
             && (addr < (uint32)module->li->codeBase + module->li->codeLength))
            {
                found = TRUE;
                break;
            }
        }

        if (found)
            printf("### PC is in module '%s', offset 0x%x\n", module->n.n_Name, addr - (uint32)module->li->codeBase);
        else
            printf("### PC is not in a known code module!\n");

#ifdef BUILD_DEBUGGER
        if (ct)
        {
            if (breakpoint == FALSE)
                DumpTask("\nTask Causing Exception:", ct);
        }
        else
        {
            printf("### No active task on this CPU\n");
        }
#else
        if (ct)
            printf("### Current task is '%s'\n", ct->t.n_Name);
        else
            printf("### No active task on this CPU\n");
#endif
    }

#ifdef BUILD_DEBUGGER
    Dbgr_TaskCrashed(CURRENTTASK, cause, addr);
#else
    Reboot();
#endif
}


/*****************************************************************************/


void HandleException(CrashCauses cause)
{
    if (_mfsr14())
        HandleSlaveException(cause);
    else
        HandleMasterException(cause);
}
