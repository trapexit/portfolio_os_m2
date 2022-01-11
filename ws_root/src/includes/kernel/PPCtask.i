#ifndef __KERNEL_PPCTASK_I
#define __KERNEL_PPCTASK_I


/******************************************************************************
**
**  @(#) PPCtask.i 96/06/10 1.35
**
******************************************************************************/


#ifndef __KERNEL_PPCNODES_I
#include <kernel/PPCnodes.i>
#endif

#ifndef __KERNEL_PPCLIST_I
#include <kernel/PPCList.i>
#endif

#ifndef __KERNEL_PPCTIME_I
#include <kernel/PPCtime.i>
#endif

#define TASK_READY   				1
#define TASK_WAITING 				2
#define TASK_RUNNING 				4

#define TASK_SINGLE_STACK		8

#define TASK_MAX_PRIORITY 			254
#define TASK_MIN_PRIORITY 			1

/* This is the stack frame that always exists at t_ssp in a TCB. This is
 * where the system call handler stores its local state. This structure
 * must be a multiple of 8 bytes in size.
 */
	.struct SuperStackFrame
ssf_BackChain	.long 1		/* EABI: always NULL			*/
ssf_CalleeLR	.long 1		/* EABI: place where callee can save LR	*/
ssf_SP		.long 1		/* original SP				*/
ssf_LR		.long 1		/* original LR				*/
ssf_SRR0	.long 1		/* original SRR0			*/
ssf_SRR1	.long 1		/* original SRR1			*/
ssf_R3		.long 1
ssf_R4		.long 1
ssf_R5		.long 1
ssf_R6		.long 1
	.ends

/* structure to hold the state of a task (not including FP regs) */
	.struct RegBlock
rb_GPRs		.long 32
rb_CR		.long 1
rb_XER		.long 1
rb_LR		.long 1
rb_CTR		.long 1
rb_PC		.long 1
rb_MSR		.long 1
	.ends

	.struct FPRegBlock
fprb_FPRs	.long 32
fprb_FPSCR	.long 1		// 32 bits for 602s
fprb_SP		.long 1
fprb_LT		.long 1
	.ends

    .struct	Task
t_t				.byte	ItemNode	// kevinh- was a ds.l
t_ThreadTask			.long	1		// Am I a thread of what task?
t_WaitBits			.long	1		// What will wake us up
t_SigBits			.long	1
t_AllocatedSigs			.long	1
t_StackBase			.long	1		// ptr to Base of stack
t_StackSize			.long	1
t_MaxUSecs			.long	1		// Equivalent in usecs
t_ElapsedTime			.byte	8
t_NumTaskLaunch			.long	1		// List of available user stacks for this process
t_Flags				.long	1		// more task specific flags
t_Module			.long	1		// The module we live within (an item #)
t_DefaultMsgPort		.long   1
t_UserData			.long	1

t_Unused0			.long	1		// 32 bits
t_FolioData			.long	1		// preallocated ptrs for the first 4 folios
t_TasksLinkNode			.byte	MinNode		// Link to the list of all tasks
t_FreeMemoryLists		.long	1		// task free memory pool
t_FreeResourceTableSlot		.long	1
t_ResourceTable			.long	1		// list of Items we need to clean up
t_ResourceCnt			.long	1		// maxumum number of slots in ResourceTable
t_ssp				.long	1		// ARM ptr to supervisor stack
t_SuperStackSize		.long	1
t_SuperStackBase		.long	1
t_ExitMessage			.long	1
t_ExitStatus			.long	1
t_Killer			.long	1
t_Quantum			.long	2
t_QuantumRemainder		.long	2
t_MessagesHeld			.byte	List		// messages gotten and not replied
t_WaitItem			.long	1		// mem we can write to (moved to MemList)
t_RegisterSave			.byte	RegBlock
t_FPRegisterSave		.byte	FPRegBlock
t_Forbid			.byte	1
t_CalledULand			.byte	1
    .ends

// The resource table has some bits packed in the upper
// bits of the Item, since Items will max out at 4096
#define ITEM_WAS_OPENED	 			0x4000
#define ITEM_NOT_AN_ITEM        		0x80000000

// predefined signals
#define SIGF_MEMGONE 	  1
#define SIGF_MEMLOW  	  2
#define SIGF_ABORT   	  4
#define SIGF_IODONE  	  8
#define SIGF_DEADTASK	  16

#endif /* __KERNEL_PPCTASK_I */
