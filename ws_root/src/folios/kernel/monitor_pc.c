/* @(#) monitor_pc.c 96/11/26 1.22 */

#include <kernel/types.h>
#include <hardware/bridgit.h>
#include <hardware/debugger.h>
#include <hardware/PPC.h>
#include <hardware/bda.h>
#include <hardware/PPCasm.h>
#include <kernel/interrupts.h>
#include <kernel/task.h>
#include <kernel/cache.h>
#include <kernel/kernel.h>
#include <kernel/sysinfo.h>
#include <kernel/super.h>
#include <kernel/internalf.h>
#include <loader/loader3do.h>
#include <loader/elf_3do.h>
#include <kernel/monitor.h>
#include <device/dma.h>
#include <hardware/cde.h>
#include <hardware/pcdebugger.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#ifdef BUILD_PCDEBUGGER


#define IS_SLAVE_CONTEXT(t) (((int32)(t)) < 0)

static void Convert2PacketHeader(PacketHeader *hdr)
{
	PchostToken tmpdbgrequest;

	memcpy(&tmpdbgrequest,hdr,sizeof(PchostToken));
	memset(hdr,0,sizeof(PacketHeader));
	hdr->ph_PacketSize = tmpdbgrequest.len + sizeof(PacketHeader);
	hdr->ph_Flavor = tmpdbgrequest.opcode & 0x00ffffff;
	hdr->ph_Object = (void*)tmpdbgrequest.userdata[0];
}

static void Convert2PchostToken(PchostToken *dbgrequest)
{
	PacketHeader tmphdr;

	memcpy(&tmphdr,dbgrequest,sizeof(PchostToken));
	memset(dbgrequest,0,sizeof(PchostToken));
	dbgrequest->opcode = MON_TOKEN | (0x00ffffff&tmphdr.ph_Flavor);
	dbgrequest->len = tmphdr.ph_PacketSize - sizeof(PacketHeader);
	dbgrequest->userdata[0] = (uint32)tmphdr.ph_Object;
}

/*****************************************************************************/
/* just to get the number of bytes in the biggest structure */
union big
{
    GeneralRegs         b_1;
    FPRegs              b_2;
    SuperRegs           b_3;
    StartupInfo         b_4;
    TaskCreationInfo    b_5;
    TaskDeletionInfo    b_6;
    CrashInfo           b_7;
    ModuleCreationInfo  b_8;
    ModuleDependentInfo b_9;
    ACKInfo             b_10;
    MemoryRangeInfo     b_11;
};

/*Globals*/
static uint8	tmp[sizeof(union big)+sizeof(PacketHeader)];
static uint8	rtmp[sizeof(PacketHeader)*2];

static bool
DMAActive(void)
{
	if (CDE_READ(KernelBase->kb_CDEBase, DMACNTL) &
			CDE_DMA_CURR_VALID)
		return TRUE;
	return FALSE;
}

static void checkecpturnaround(void)
{
	if (PARALLEL_WINDOW_OFFSET==PCMCIA_ECPDMAREG_OFFSET)
	do
	{
		externalInvalidateDCache((void*)((PCMCIA_BASE)|(PCMCIA_ECPCNTLREG_OFFSET)), 4);
	}
 	while (((*(volatile unsigned long *)((uint32)(PCMCIA_BASE)|(PCMCIA_ECPCNTLREG_OFFSET)))&0x80808080)==0);
}

static void SendPacket(void *data, uint32 size)
{
uint8	     *outgoing=tmp;
uint32 		 i;
uint32		 offset;
	/*make it dma aligned*/
	i = ((uint32)outgoing & (0xffffffff^(sizeof(PacketHeader)-1))) + sizeof(PacketHeader);
	outgoing = (uint8*)i;
	/* *data does not have to be 32 byte aligned*/
	memcpy(outgoing,data,size);
	Convert2PchostToken((PchostToken *)outgoing);
	FlushDCache(0, outgoing,size);

	while (DMAActive()) continue; /*test to see if still active*/
	offset = PARALLEL_WINDOW_OFFSET;
	StartDMA( (uint32)(PCMCIA_BASE+offset),
	(void*)outgoing, size,DMA_WRITE|DMA_SYNC,
	PCDEVDMACHANNEL,
	NULL,
	(void*)checkecpturnaround);

}


/*****************************************************************************/


static void WaitPacket(void *data, uint32 size)
{
	PchostToken *pcrequest=(PchostToken *)data;
	PchostToken	*outgoing=(PchostToken*)rtmp;
	uint32 		 i;
	uint8 *databuf = (uint8*)data + sizeof(PchostToken);
	uint32		 offset;
	/*make it dma aligned*/
	i = ((uint32)outgoing & (0xffffffff^(sizeof(PchostToken)-1))) + sizeof(PchostToken);
	outgoing = (PchostToken*)i;
	memset(outgoing,0,sizeof(PchostToken));
	outgoing->opcode=MONNULLTOKEN;
	FlushDCache(0, outgoing,sizeof(PchostToken));
	while (DMAActive()) continue; /*test to see if still active*/
	offset = PARALLEL_WINDOW_OFFSET;
	while(1)
	{
		/*poll card int*/
		while ((CDE_READ(KernelBase->kb_CDEBase,CDE_INT_STS)&CARDINT)==0) continue;
		/*send permission packet*/
		{

			StartDMA((uint32)(PCMCIA_BASE+offset),
			(void*)outgoing, sizeof(PchostToken),
			DMA_WRITE|DMA_SYNC,
			PCDEVDMACHANNEL,
			NULL,
			(void*)checkecpturnaround);
			/* interrupt is cleared in here by pc */
			/* read PchostToken packet
			*data has to be 32 byte aligned and size has to be multiple of 32*/
			StartDMA( (uint32)(PCMCIA_BASE+offset),
			(void*)data, sizeof(PchostToken),DMA_READ|DMA_SYNC,
			PCDEVDMACHANNEL,
			NULL,
			(void*)checkecpturnaround);
		}
                ClearInterrupt(CARDINTMASK);

		/*chech to see if it is a Debugger command or not */
		if ((pcrequest->opcode&0xff000000)==DBGR_TOKEN)
		{
			/*yes convert it and return*/
			if ((size-sizeof(PchostToken))<pcrequest->len)
			{
				while(pcrequest->len>0)  /*flush dma*/
				{
					StartDMA((uint32)(PCMCIA_BASE+offset),
					(void*)databuf, sizeof(PchostToken),DMA_READ|DMA_SYNC,
					PCDEVDMACHANNEL,
					NULL,
					NULL);
					pcrequest->len -= sizeof(PchostToken);
				}
/*				printf("MONITOR: not enough buffer=%d for %d, halt!!!\n",(size-sizeof(PchostToken)),pcrequest->len);
*/
			while(1);
			}
			if (pcrequest->len>0)
				StartDMA((uint32)(PCMCIA_BASE+offset),
				(void*)databuf, pcrequest->len,DMA_READ|DMA_SYNC,
				PCDEVDMACHANNEL,
				NULL,
				NULL);
			Convert2PacketHeader((PacketHeader *)data);
			return;
		}
  		if ((pcrequest->opcode&0xff000000)==MONTOKEN)
  		{
			switch ((pcrequest->opcode&0xff00)>>8)
			{
		  	  case 'W':
					if (pcrequest->len>0)
					StartDMA( (uint32)(PCMCIA_BASE+offset),
					(void*)pcrequest->address, pcrequest->len,
					DMA_READ|DMA_SYNC,
					PCDEVDMACHANNEL,
					NULL,
					NULL);
				break;
			  case 'R':
					if (pcrequest->len>0)
					{
					if (pcrequest->address==(ubyte*)(0x00040008))
					{
					uint32 a;
					a=*((volatile uint32 *)pcrequest->address);
					*((uint32*)outgoing)=a;
					FlushDCache(0, outgoing,sizeof(PchostToken));
                                        StartDMA( (uint32)(PCMCIA_BASE+offset),
                                        (void*)outgoing, pcrequest->len,
                                        DMA_WRITE|DMA_SYNC,
                                        PCDEVDMACHANNEL,
                                        NULL,
                                        (void*)checkecpturnaround);
				        memset(outgoing,0,sizeof(PchostToken));
       					outgoing->opcode=MONNULLTOKEN;
				        FlushDCache(0, outgoing,sizeof(PchostToken));
					}
					else
					StartDMA( (uint32)(PCMCIA_BASE+offset),
					(void*)pcrequest->address, pcrequest->len,
					DMA_WRITE|DMA_SYNC,
					PCDEVDMACHANNEL,
					NULL,
					(void*)checkecpturnaround);
					}
				break;
			  default:
					goto error;
				break;
			}
		}
		else break; /*Big problem some one send illegal packet*/
	}
error:
	while(pcrequest->len>0) /*flush dma*/
	{
		StartDMA((uint32)(PCMCIA_BASE+offset),
		(void*)databuf, sizeof(PchostToken),DMA_READ|DMA_SYNC,
		PCDEVDMACHANNEL,
		NULL,
		NULL);
		pcrequest->len -= sizeof(PchostToken);
	}
/*	printf("MONITOR: bad packet=%x in monitor mode, halt!!!\n",pcrequest->opcode);
*/
	while(1);
}


/*****************************************************************************/


static void DispatchPacket(MonitorFlavors flavor, void *obj, void *data, uint32 size)
{
PacketHeader *hdr;
PacketHeader tmphdr;

    if (data == NULL)
    {
        data = &tmphdr;
        size = sizeof(PacketHeader);
    }

    hdr                = (PacketHeader *)data;
    hdr->ph_PacketSize = size;
    hdr->ph_Flavor     = flavor;
    hdr->ph_Object     = obj;
    SendPacket(hdr, size);
}


/*****************************************************************************/


static void DispatchACK(int32 code)
{
ACKInfo ack;

    ack.ack_ResultCode = code;
    DispatchPacket(MON_ACK, NULL, &ack, sizeof(ack));
}


/*****************************************************************************/


static bool ValidTask(Task *t)
{
Task *scan;
Node *n;

    if (t == NULL)
        return FALSE;

    if (t == CURRENTTASK)
        return TRUE;

    if (IS_SLAVE_CONTEXT(t))
        return TRUE;

    ScanList(&KB_FIELD(kb_TaskReadyQ), scan, Task)
    {
        if (t == scan)
            return TRUE;
    }

    /* FIXME: this list should not be scanned from interrupt code */
    ScanList(&KB_FIELD(kb_Tasks), n, Node)
    {
        scan = Task_Addr(n);
        if (t == scan)
            return TRUE;
    }

    return FALSE;
}


/*****************************************************************************/


static void MonitorLoop(void)
{
Task         *t;
uint8	     *incoming=tmp;
PacketHeader *hdr;
uint32        i;
Task         *ct;

    SuperSlaveRequest(SLAVE_SUSPEND, 0, 0, 0);

    /* make it dma aligned */
    i = ((uint32)incoming & (0xffffffff^(sizeof(PacketHeader)-1))) + sizeof(PacketHeader);
    incoming = (uint8*)i;

    ct = CURRENTTASK;
    while (TRUE)
    {
        WaitPacket(incoming, sizeof(union big));
        hdr = (PacketHeader *)incoming;
        t   = NULL;

        if ((hdr->ph_Flavor != DBGR_NOP)
         && (hdr->ph_Flavor != DBGR_ACK)
         && (hdr->ph_Flavor != DBGR_GetSuperRegs)
         && (hdr->ph_Flavor != DBGR_SetSuperRegs)
         && (hdr->ph_Flavor != DBGR_InvalidateICache)
         && (hdr->ph_Flavor != DBGR_FlushDCache))
        {
            t = (Task *)hdr->ph_Object;
            if (!ValidTask(t))
            {
                printf("MONITOR: Bad task pointer 0x%x from debugger\n", t);
                DispatchACK(-1);
                continue;
            }
        }

        if (t && !IS_SLAVE_CONTEXT(t))
        {
            if ((t->t_Flags & TASK_SUSPENDED) == 0)
            {
                if (hdr->ph_Flavor != DBGR_SuspendTask)
                {
                    printf("MONITOR: Debugger illegally sent flavor %d to unsuspended task %s\n", hdr->ph_Flavor, t->t.n_Name);
                    DispatchACK(-1);
                    continue;
                }
            }
        }

        switch (hdr->ph_Flavor)
        {
            case DBGR_NOP:
            {
                break;
            }

            case DBGR_ACK:
            {
                if (CURRENTTASK != ct)
                {
                    /* The current task has changed for some reason. The debugger
                     * might've nuked it via some packet. Call the scheduler in
                     * this case to figure out what to do
                     */
                    ScheduleNewTask();

                    /* execution never returns here */
                }

                SuperSlaveRequest(SLAVE_CONTINUE, 0, 0, 0);
                return;
            }

            case DBGR_GetGeneralRegs:
            {
            uint32       i;
            GeneralRegs  gr;
            RegBlock    *rb;

                memset(&gr, 0, sizeof(gr));

                if (IS_SLAVE_CONTEXT(t))
                    rb = &KB_FIELD(kb_SlaveState)->ss_RegSave;
                else
                    rb = &t->t_RegisterSave;

                for (i = 0; i < 32; i++)
                    gr.gr_GPRs[i] = rb->rb_GPRs[i];

                gr.gr_PC  = rb->rb_PC;
                gr.gr_XER = rb->rb_XER;
                gr.gr_CR  = rb->rb_CR;
                gr.gr_LR  = rb->rb_LR;
                gr.gr_CTR = rb->rb_CTR;
                gr.gr_MSR = rb->rb_MSR;

                DispatchPacket(MON_GeneralRegs, NULL, &gr, sizeof(gr));
                break;
            }

            case DBGR_GetFPRegs:
            {
            uint32      i;
            FPRegs      fpr;
            FPRegBlock *fprb;

                memset(&fpr, 0, sizeof(fpr));

                if (IS_SLAVE_CONTEXT(t))
                    fprb = &KB_FIELD(kb_SlaveState)->ss_FPRegSave;
                else
                    fprb = &t->t_FPRegisterSave;

                if (KB_FIELD(kb_FPOwner) == t)
                {
                    _mtmsr(_mfmsr() | MSR_FP);

                    /* if its in the HW, save it out so we can read it */
                    SaveFPRegBlock(&t->t_FPRegisterSave);

                    /* unfortunately, the save call is destructive, so restore things */
                    RestoreFPRegBlock(&t->t_FPRegisterSave);
                }

                for (i = 0; i < 32; i++)
                    fpr.fpr_FPRs[i] = fprb->fprb_FPRs[i];

                fpr.fpr_SP    = fprb->fprb_SP;
                fpr.fpr_LT    = fprb->fprb_LT;
                fpr.fpr_FPSCR = fprb->fprb_FPSCR;

                DispatchPacket(MON_FPRegs, NULL, &fpr, sizeof(fpr));
                break;
            }

            case DBGR_GetSuperRegs:
            {
            SuperRegs sr;

                memset(&sr, 0, sizeof(sr));

                sr.sr_HID0    = _mfhid0();
                sr.sr_PVR     = _mfpvr();

                sr.sr_IBAT[0] = _mfibat0();
                sr.sr_IBAT[1] = _mfibat1();
                sr.sr_IBAT[2] = _mfibat2();
                sr.sr_IBAT[3] = _mfibat3();

                sr.sr_DBAT[0] = _mfdbat0();
                sr.sr_DBAT[1] = _mfdbat1();
                sr.sr_DBAT[2] = _mfdbat2();
                sr.sr_DBAT[3] = _mfdbat3();

                sr.sr_SPRG[0] = _mfsprg0();
                sr.sr_SPRG[1] = _mfsprg1();
                sr.sr_SPRG[2] = _mfsprg2();
                sr.sr_SPRG[3] = _mfsprg3();

                sr.sr_SRR0    = _mfsrr0();
                sr.sr_SRR1    = _mfsrr1();
                sr.sr_ESASRR  = _mfesasrr();
                sr.sr_DEC     = _mfdec();
                sr.sr_DAR     = _mfdar();
                sr.sr_DSISR   = _mfdsisr();
                sr.sr_TBU     = _mftbu();
                sr.sr_TBL     = _mftbl();
                sr.sr_DMISS   = _mfdmiss();
                sr.sr_SER     = _mfser();
                sr.sr_SEBR    = _mfsebr();

                GetSegRegs(sr.sr_SGMT);

                DispatchPacket(MON_SuperRegs, NULL, &sr, sizeof(sr));
                break;
            }

            case DBGR_SetGeneralRegs:
            {
            uint32       i;
            GeneralRegs *gr;
            RegBlock    *rb;

                gr = (GeneralRegs *)hdr;

                if (IS_SLAVE_CONTEXT(t))
                    rb = &KB_FIELD(kb_SlaveState)->ss_RegSave;
                else
                    rb = &t->t_RegisterSave;

                for (i = 0; i < 32; i++)
                    rb->rb_GPRs[i] = gr->gr_GPRs[i];

                rb->rb_PC  = gr->gr_PC;
                rb->rb_XER = gr->gr_XER;
                rb->rb_CR  = gr->gr_CR;
                rb->rb_LR  = gr->gr_LR;
                rb->rb_CTR = gr->gr_CTR;
                rb->rb_MSR = gr->gr_MSR;

                DispatchACK(0);
                break;
            }

            case DBGR_SetFPRegs:
            {
            uint32      i;
            FPRegs     *fpr;
            FPRegBlock *fprb;

                fpr = (FPRegs *)hdr;

                if (IS_SLAVE_CONTEXT(t))
                    fprb = &KB_FIELD(kb_SlaveState)->ss_FPRegSave;
                else
                    fprb = &t->t_FPRegisterSave;

                for (i = 0; i < 32; i++)
                    fprb->fprb_FPRs[i] = fpr->fpr_FPRs[i];

                fprb->fprb_FPSCR = fpr->fpr_FPSCR;
                fprb->fprb_LT    = fpr->fpr_LT;
                fprb->fprb_SP    = fpr->fpr_SP;

                if (KB_FIELD(kb_FPOwner) == t)
                {
                    _mtmsr(_mfmsr() | MSR_FP);
                    RestoreFPRegBlock(&t->t_FPRegisterSave);
                }

                DispatchACK(0);
                break;
            }

            case DBGR_SetSuperRegs:
            {
            SuperRegs *sr;

                sr = (SuperRegs *)hdr;

                _mthid0(sr->sr_HID0);

                /* PPC requires that the BATs be cleared before being set */
                _mtibat0(0,0);
                _mtibat1(0,0);
                _mtibat2(0,0);
                _mtibat3(0,0);
                _mtibat0(sr->sr_IBAT[0] >> 32, sr->sr_IBAT[0]);
                _mtibat1(sr->sr_IBAT[1] >> 32, sr->sr_IBAT[1]);
                _mtibat2(sr->sr_IBAT[2] >> 32, sr->sr_IBAT[2]);
                _mtibat3(sr->sr_IBAT[3] >> 32, sr->sr_IBAT[3]);

                _mtdbat0(0,0);
                _mtdbat1(0,0);
                _mtdbat2(0,0);
                _mtdbat3(0,0);
                _mtdbat0(sr->sr_DBAT[0] >> 32, sr->sr_DBAT[0]);
                _mtdbat1(sr->sr_DBAT[1] >> 32, sr->sr_DBAT[1]);
                _mtdbat2(sr->sr_DBAT[2] >> 32, sr->sr_DBAT[2]);
                _mtdbat3(sr->sr_DBAT[3] >> 32, sr->sr_DBAT[3]);

                _mtsprg0(sr->sr_SPRG[0]);
                _mtsprg1(sr->sr_SPRG[1]);
                _mtsprg2(sr->sr_SPRG[2]);
                _mtsprg3(sr->sr_SPRG[3]);

                _mtsrr0(sr->sr_SRR0);
                _mtsrr1(sr->sr_SRR1);

                _mtesasrr(sr->sr_ESASRR);
                _mtdec(sr->sr_DEC);
                _mtdar(sr->sr_DAR);
                _mtdsisr(sr->sr_DSISR);
                _mtser(sr->sr_SER);
                _mtsebr(sr->sr_SEBR);

                /* we don't restore: TBU, TBL, DMISS, and SGMT */

                DispatchACK(0);
                break;
            }

            case DBGR_SetSingleStep:
            {
                if (IS_SLAVE_CONTEXT(t))
                    KB_FIELD(kb_SlaveState)->ss_RegSave.rb_MSR |= MSR_SE;
                else
                    t->t_RegisterSave.rb_MSR |= MSR_SE;

                DispatchACK(0);
                break;
            }

            case DBGR_SetBranchStep:
            {
                if (IS_SLAVE_CONTEXT(t))
                    KB_FIELD(kb_SlaveState)->ss_RegSave.rb_MSR |= MSR_BE;
                else
                    t->t_RegisterSave.rb_MSR |= MSR_BE;

                DispatchACK(0);
                break;
            }

            case DBGR_ClearSingleStep:
            {
                if (IS_SLAVE_CONTEXT(t))
                    KB_FIELD(kb_SlaveState)->ss_RegSave.rb_MSR &= (~MSR_SE);
                else
                    t->t_RegisterSave.rb_MSR &= (~MSR_SE);

                DispatchACK(0);
                break;
            }

            case DBGR_ClearBranchStep:
            {
                if (IS_SLAVE_CONTEXT(t))
                    KB_FIELD(kb_SlaveState)->ss_RegSave.rb_MSR &= (~MSR_BE);
                else
                    t->t_RegisterSave.rb_MSR &= (~MSR_BE);

                DispatchACK(0);
                break;
            }

            case DBGR_InvalidateICache:
            {
                InvalidateICache();
                SuperSlaveRequest(SLAVE_INVALIDATEICACHE, 0, 0, 0);
                DispatchACK(0);
                break;
            }

            case DBGR_FlushDCache:
            {
                FlushDCacheAll(0);
                DispatchACK(0);
                break;
            }

            case DBGR_SuspendTask:
            {
                if (!IS_SLAVE_CONTEXT(t))
                    SuspendTask(t);

                DispatchACK(0);
                break;
            }

            case DBGR_ResumeTask:
            {
                if (!IS_SLAVE_CONTEXT(t))
                    ResumeTask(t);

                DispatchACK(0);
                break;
            }

            case DBGR_AbortTask:
            {
                if (IS_SLAVE_CONTEXT(t))
                {
                    printf("### Killing slave execution context\n");
                    SuperSlaveRequest(SLAVE_ABORT, 0, 0, 0);
                }
                else
                {
                    printf("### Killing task '%s'\n", t->t.n_Name);
                    Murder(t, NULL);
                    NewReadyTask(t);  /* make it run... */
                }

                DispatchACK(0);
                break;
            }

            default:
            {
                printf("MONITOR: Unexpected packet flavor 0x%x from debugger\n", hdr->ph_Flavor);
                break;
            }
        }
    }
}


/*****************************************************************************/


void Dbgr_SystemStartup(void)
{
StartupInfo si;

    memset(&si, 0, sizeof(si));

    si.si_OSVersion = OS_VERSION;
    si.si_OSVersion = OS_REVISION;
    DispatchPacket(MON_SystemStartup, NULL, &si, sizeof(si));
    MonitorLoop();
}


/*****************************************************************************/


void Dbgr_MemoryRanges(void)
{
MemoryRangeInfo mri;

    memset(&mri, 0, sizeof(mri));

    strcpy(mri.mri_Name, "RAM");
    mri.mri_Type  = MRT_RAM;
    mri.mri_Start = KB_FIELD(kb_PagePool)->pp_MemRegion->mr_MemBase;
    mri.mri_End   = KB_FIELD(kb_PagePool)->pp_MemRegion->mr_MemTop;
    DispatchPacket(MON_MemoryRange, NULL, &mri, sizeof(mri));
    MonitorLoop();
}


/*****************************************************************************/


void Dbgr_TaskCreated(Task *t, LoaderInfo *li)
{
TaskCreationInfo tci;

    TOUCH(li);

    memset(&tci, 0, sizeof(tci));

    strncpy(tci.tci_Name, t->t.n_Name, sizeof(tci.tci_Name) - 1);
    tci.tci_Name[sizeof(tci.tci_Name) - 1] = 0;
    tci.tci_Creator        = CURRENTTASK;
    tci.tci_Thread         = (t->t_ThreadTask ? TRUE : FALSE);
    tci.tci_Module         = LookupItem(t->t_Module);
    tci.tci_UserStackBase  = t->t_StackBase;
    tci.tci_UserStackSize  = t->t_StackSize;
    tci.tci_SuperStackBase = t->t_SuperStackBase;
    tci.tci_SuperStackSize = t->t_SuperStackSize;
    DispatchPacket(MON_TaskCreated, t, &tci, sizeof(tci));
    MonitorLoop();
}


/*****************************************************************************/


void Dbgr_TaskDeleted(Task *t)
{
TaskDeletionInfo tdi;

    memset(&tdi, 0, sizeof(tdi));

    tdi.tdi_ExitStatus = t->t_ExitStatus;
    DispatchPacket(MON_TaskDeleted, t, &tdi, sizeof(tdi));
    MonitorLoop();
}


/*****************************************************************************/


void Dbgr_TaskCrashed(Task *t, CrashCauses cause, uint32 pc)
{
CrashInfo ci;

    memset(&ci, 0, sizeof(ci));

    ci.ci_Cause = cause;
    ci.ci_PC    = pc;
    DispatchPacket(MON_TaskCrashed, t, &ci, sizeof(ci));
    MonitorLoop();
}


/*****************************************************************************/


void Dbgr_MPIOReqCreated(IOReq *ior)
{
TaskCreationInfo tci;

    memset(&tci, 0, sizeof(tci));

    if (ior->io.n_Name!=NULL)
     strncpy(tci.tci_Name, ior->io.n_Name, sizeof(tci.tci_Name) - 1);
    tci.tci_Name[sizeof(tci.tci_Name) - 1] = 0;
    tci.tci_Creator        = CURRENTTASK;
    tci.tci_Thread         = TRUE;
    tci.tci_Module         = LookupItem(CURRENTTASK->t_Module);
    tci.tci_UserStackBase  = NULL;
    tci.tci_UserStackSize  = 0;
    tci.tci_SuperStackBase = NULL;
    tci.tci_SuperStackSize = 0;
    DispatchPacket(MON_TaskCreated, (void *)(-(int32)ior), &tci, sizeof(tci));
    MonitorLoop();
}


/*****************************************************************************/


void Dbgr_MPIOReqDeleted(IOReq *ior)
{
TaskDeletionInfo tdi;

    memset(&tdi, 0, sizeof(tdi));

    tdi.tdi_ExitStatus = 0;
    DispatchPacket(MON_TaskDeleted, (void *)(-(int32)ior), &tdi, sizeof(tdi));
    MonitorLoop();
}


/*****************************************************************************/


void Dbgr_MPIOReqCrashed(IOReq *ior, CrashCauses cause, uint32 pc)
{
CrashInfo ci;

    memset(&ci, 0, sizeof(ci));

    ci.ci_Cause = cause;
    ci.ci_PC    = pc;
    DispatchPacket(MON_TaskCrashed, (void *)(-(int32)ior), &ci, sizeof(ci));
    MonitorLoop();
}


/*****************************************************************************/


void Dbgr_ModuleCreated(Module *m)
{
ModuleCreationInfo   mci;
ModuleDependentInfo  mdi;
ELF_Imp3DO          *imports;
uint32               i;

    memset(&mci, 0, sizeof(mci));

    strncpy(mci.mci_Name, m->n.n_Name, sizeof(mci.mci_Name) - 1);
    mci.mci_CodeStart  = m->li->codeBase;
    mci.mci_CodeLength = m->li->codeLength;
    mci.mci_DataStart  = m->li->dataBase;
    mci.mci_DataLength = m->li->dataLength;
    mci.mci_BSSStart   = (void *)((uint32)m->li->dataBase + m->li->dataLength);
    mci.mci_BSSLength  = m->li->bssLength;
    mci.mci_EntryPoint = m->li->entryPoint;
    mci.mci_Version    = m->n.n_Version;
    mci.mci_Revision   = m->n.n_Revision;

    if (m->li->path)
        strncpy(mci.mci_Path, m->li->path, sizeof(mci.mci_Path) - 1);

    DispatchPacket(MON_ModuleCreated, m, &mci, sizeof(mci));
    MonitorLoop();

    imports = m->li->imports;
    if (imports)
    {
        for (i = 0; i < imports->numImports; i++)
        {
            memset(&mdi, 0, sizeof(mdi));
            strncpy(mdi.mdi_Name, (char *)&imports->imports[imports->numImports] + imports->imports[i].nameOffset, sizeof(mdi.mdi_Name) - 1);
            DispatchPacket(MON_ModuleDependent, m, &mdi, sizeof(mdi));
            MonitorLoop();
        }
    }
}


/*****************************************************************************/


void Dbgr_ModuleDeleted(Module *m)
{
    DispatchPacket(MON_ModuleDeleted, m, NULL, 0);
    MonitorLoop();
}


/*****************************************************************************/


/* This is called when the debugger triggers an interrupt on the
 * target. Tell the debugger we're here, and enter the monitor to see what
 * the debugger wants.
 */
void MonitorFirq(void)
{
    DispatchPacket(MON_Hello, NULL, NULL, 0);
    MonitorLoop();
}


#endif /* BUILD_PCDEBUGGER */
