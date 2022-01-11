#ifndef __KERNEL_PPCKERNEL_I
#define __KERNEL_PPCKERNEL_I


/******************************************************************************
**
**  @(#) PPCkernel.i 96/11/13 1.59
**
******************************************************************************/


#ifndef __KERNEL_PPCITEM_I
#include <kernel/PPCitem.i>
#endif

#ifndef __KERNEL_PPCNODES_I
#include <kernel/PPCnodes.i>
#endif

#ifndef __KERNEL_PPCPROTLIST_I
#include <kernel/PPCprotlist.i>
#endif

#ifndef __KERNEL_PPCFOLIO_I
#include <kernel/PPCfolio.i>
#endif

#ifndef __KERNEL_PPCTASK_I
#include <kernel/PPCtask.i>
#endif


/*****************************************************************************/


	.struct SlaveState
ss_RegSave		.byte	RegBlock
ss_FPRegSave		.byte	FPRegBlock
ss_SuperStack		.byte	2048
ss_SuperSP		.long	1
ss_WritablePages	.long	1
ss_NumPages		.long	1
ss_SlaveSER		.long	1
ss_SlaveSEBR		.long	1
ss_SlaveMSR		.long	1
ss_SlaveHID0		.long	1
ss_SlaveVersion		.long	1
	.ends


#define SLAVE_NOP      		0
#define SLAVE_DISPATCH		1
#define SLAVE_ABORT		2
#define SLAVE_CONTROLICACHE	3
#define SLAVE_INVALIDATEICACHE	4
#define SLAVE_CONTROLDCACHE	5
#define SLAVE_INITMMU		6
#define SLAVE_UPDATEMMU		7
#define SLAVE_INITVERSION	8
#define SLAVE_SRESETPREP	9
#define SLAVE_SRESETRECOVER	10

#define MASTER_NOP		0
#define MASTER_SLAVECOMPLETE	1
#define MASTER_PUTSTR		2

	.struct CPUActionReq
car_Action		.long	1
car_Arg1		.long	1
car_Arg2		.long	1
car_Arg3		.long	1
	.ends


/*****************************************************************************/


	.struct KernelBaseRec		// See kernel.h for more info
kb			.byte	Folio	// Folio	kb
kb_CPUFlags		.long	1	// uint32	kb_Flags
kb_Tasks		.byte	List	// List		kb_Tasks

kb_CurrentTask		.long	1	// Task * const	kb_CurrentTask
kb_CurrentTaskItem	.long	1	// Item const	kb_CurrentTaskItem
kb_NumTaskSwitches	.long	1	// uint32	kb_NumTaskSwitches
kb_TaskReadyQ		.byte	List	// List		kb_TaskReadyQ

kb_WriteFencePtr	.long	1	// uint32	*kb_WriteFencePtr
kb_RAMBaseAddress       .long   1       // uint32       kb_RAMBaseAddress
kb_RAMEndAddress        .long   1       // uint32       kb_RAMEndAddress
kb_CurrentFenceTask	.long	1	// Task		*kb_CurrentFenceTask

kb_FolioList		.byte	List	// List		kb_FolioList
kb_Drivers		.byte	List	// List		kb_Drivers
kb_DevSemaphore		.long	1	// Item		kb_DevSemaphore
kb_DDFs			.byte	ProtectedList	// ProtectedList kb_DDFs
kb_MsgPorts		.byte	List	// List		kb_MstPorts
kb_Semaphores		.byte	List	// List		kb_Semaphores
kb_Errors		.byte	List	// List		kb_Errors
kb_ErrorSemaphore	.long	1	// Item		kb_ErrorSemaphore
kb_MemLockHandlers	.byte	List	// List		kb_MemLockHandlers
kb_MemLockSemaphore	.long	1	// Item		kb_MemLockSemaphore
kb_DeadTasks		.byte	List	// List		kb_DeadTasks
kb_DuckFuncs		.byte	ProtectedList	// ProtectedList kb_DuckFuncs
kb_RecoverFuncs		.byte	ProtectedList	// ProtectedList kb_RecoverFuncs

kb_MemRegion		.long	1	// struct MemRegion	*kb_MemRegion
kb_PagePool		.long	1	// struct PagePool	*kb_PagePool

kb_DataFolios		.long	1	// Folio	**kb_DataFolios
kb_FolioTaskDataCnt	.byte	1	// uint8	kb_FolioTaskDataCnt
kb_FolioTaskDataSize	.byte	1	// uint8	kb_FolioTaskDataSize

kb_PleaseReschedule	.byte	1	// bool		kb_PleaseReschedule
kb_MaxInterrupts	.byte	1	// uint8	kb_MaxInterrupts
kb_InterruptHandlers	.long	1	// Node		**kb_InterruptHandlers
kb_InterruptStack	.long	1	// uint32	*kb_InterruptStack

kb_ItemTable		.long	1	// ItemEntry	**kb_ItemTable
kb_MaxItem		.long	1	// int32	kb_MaxItem

kb_FPOwner		.long	1	// Task		*kb_FPOwner

kb_AppVolumeLabel	.long	1	// DiscLabel	*kb_AppVolumeLabel
kb_AppVolumeName	.byte	32	// char		kv_AppvolumeLabel[32]

kb_OperatorTask		.long	1
kb_UniqueID		.long	2	// uint32	kb_UniqueID[2]
kb_BusClock		.long	1	// uint32	kb_BusClk
kb_ShowTaskMessages	.byte	1	// bool		kb_ShowTaskMessages
kb_pad0			.byte	3	// bool		kb_pad0[3]

kb_QueryROMSysInfo	.long	1	// uint32	(*kb_QueryROMSysInfo)(unsigned long, void *, unsigned long)
kb_SetROMSysInfo	.long	1	// uint32	(*kb_SetROMSysInfo)(unsigned long, void *, unsigned long)
kb_PerformSoftReset	.long	1	// uint32	(*kb_PerformSoftReset)(void)
kb_PutC			.long	1	// void		(*kb_PutC)(char)
kb_MayGetChar		.long	1	// int		(*kb_MayGetChar)(void)
kb_DipirRoutines	.long	1	// void		*kb_DipirRoutines
kb_KernelModule		.long	1	// void		*kb_KernelModule

kb_CDEBase		.long	1	// uint32	kb_CDEBase
kb_NoReboot		.long	1	// int32	kb_NoReboot
kb_ExpectDataDisc	.long	1	// int32	kb_ExpectDataDisc
kb_DCacheSize		.long	1	// uint32	kb_DCacheSize
kb_DCacheBlockSize	.long	1	// uint32	kb_DCacheBlockSize
kb_DCacheNumBlocks	.long	1
kb_DCacheFlushData	.long	1

kb_PersistentMemStart	.long	1
kb_PersistentMemSize	.long	1

kb_ROMBaseAddress       .long   1       // uint32       kb_ROMBaseAddress
kb_ROMEndAddress        .long   1       // uint32       kb_ROMEndAddress

kb_Modules		.byte	List
kb_OSComponents		.long	1

kb_NumCPUs		.long	1
kb_MasterReq		.long	1
kb_SlaveReq		.long	1
kb_SlaveState		.long	1
kb_CurrentSlaveTask	.long	1
kb_CurrentSlaveIO	.long	1
	.ends

#define CURRENTTASK	KernelBaseRec.kb_CurrentTask
#define CURRENTTASKITEM	KernelBaseRec.kb_CurrentTaskItem

#define KB_NODBGR	0x00000001
#define KB_LOGGING	0x00000002
#define KB_MEMDEBUG	0x00000004
#define KB_SERIALPORT	0x00000008


#endif /* __KERNEL_PPCKERNEL_I */
