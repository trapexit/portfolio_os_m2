/* @(#) super_glue.s 96/08/23 1.65 */

#include <hardware/PPCMacroequ.i>
#include <kernel/PPCfolio.i>
#include <kernel/PPCitem.i>


/*****************************************************************************/


	DECFN	SuperInternalPutMsg
	b	superinternalSendMsg

	DECFN	SuperInternalSendIO
	b	internalSendIO

	DECFN	SuperCompleteIO
	b	internalCompleteIO

	DECFN	SuperInternalAbortIO
	b	internalAbortIO

	DECFN	SuperInternalSignal
	b	internalSignal

	DECFN	SuperInternalUnlockSemaphore
	b	internalUnlockSemaphore

	DECFN	SuperInternalLockSemaphore
	b	internalLockSemaphore

	DECFN	SuperInternalDeleteItem
	b	internalDeleteItem

	DECFN	SuperInternalFreeSignal
	b	internalFreeSignal

	DECFN	SuperInvalidateDCache
	b	externalInvalidateDCache

	DECFN	SuperReportEvent
	b	internalReportEvent

	DECFN	SuperInternalWaitPort
	b	internalWaitPort

	DECFN	SuperInternalWaitIO
	b	internalWaitIO

	DECFN	SuperInternalDoIO
	b	internalDoIO

	DECFN	SuperInternalInsertMemLockHandler
	b	internalInsertMemLockHandler

	DECFN	SuperInternalRemoveMemLockHandler
	b	internalRemoveMemLockHandler

	DECFN	SuperInternalReplyMsg
	b	internalReplyMsg

	DECFN	SuperInternalOpenItem
	b	internalOpenItem

	DECFN	SuperInternalCloseItem
	b	internalCloseItem

	DECFN	SuperAllocNode
	b	AllocateNode

	DECFN	SuperFreeNode
	b	FreeNode

	DECFN	SuperInternalCreateFirq
	b	internalCreateFirq

	DECFN	SuperInternalCreateTimer
	b	internalCreateTimer


/*****************************************************************************/


	DECFN	SuperCreateItem
	b	internalCreateItem

	DECFN	SuperWaitIO
	b	externalWaitIO

	DECFN	SuperWaitPort
	b	externalWaitPort

	DECFN	SuperDoIO
	b	externalDoIO

	DECFN	SuperSetItemOwner
	b	externalSetItemOwner

	DECFN	SuperSendIO
	b	externalSendIO

	DECFN	SuperFreeSignal
	b	externalFreeSignal

	DECFN	SuperAllocSignal
	b	externalAllocSignal

	DECFN	SuperControlMem
	b	externalControlMem

	DECFN	SuperGetMsg
	b	externalGetMsg

	DECFN	SuperReplyMsg
	b	externalReplyMsg

	DECFN	SuperSendMsg
	b	externalSendMsg

	DECFN	SuperSendSmallMsg
	b	externalSendMsg

	DECFN	SuperGetThisMsg
	b	externalGetThisMsg

	DECFN	Superkprintf
	b	printf

	DECFN	SuperCloseItem
	b	externalCloseItem

	DECFN	SuperLockSemaphore
	b	externalLockSemaphore

	DECFN	SuperUnlockSemaphore
	b	externalUnlockSemaphore

	DECFN	SuperOpenItem
	b	externalOpenItem

	DECFN	SuperFindItem
	b	internalFindItem

	DECFN	SuperDeleteItem
	b	externalDeleteItem

	DECFN	SuperWaitSignal
	b	internalWait

	DECFN	SuperCreateSizedItem
	b	internalCreateSizedItem

	DECFN	SuperInternalFindAndOpenItem
	b	internalFindAndOpenItem

	DECFN	SuperInternalCreateTaskVA
	b	internalCreateTaskVA

	DECFN	SuperInternalCreateSemaphore
	b	internalCreateSemaphore

	DECFN	SuperInternalCreateErrorText
	b	internalCreateErrorText

	DECFN	SuperLogEvent
#ifdef BUILD_LUMBERJACK
	b	internalLogEvent
#else
	li	r3,-1
	blr
#endif
