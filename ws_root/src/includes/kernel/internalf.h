#ifndef __INTERNALF_H
#define __INTERNALF_H


/******************************************************************************
**
**  @(#) internalf.h 96/11/19 1.97
**
******************************************************************************/


#ifdef EXTERNAL_RELEASE
#error "This file may not be used in externally released source code or link lib"
#endif

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_NODES_H
#include <kernel/nodes.h>
#endif

#ifndef __KERNEL_LIST_H
#include <kernel/list.h>
#endif

#ifndef __KERNEL_TASKS_H
#include <kernel/task.h>
#endif

#ifndef __KERNEL_MEM_H
#include <kernel/mem.h>
#endif

#ifndef __LOADER_HEADER3DO_H
#include <loader/header3do.h>
#endif

#ifndef __KERNEL_CACHE_H
#include <kernel/cache.h>
#endif

#ifndef __KERNEL_KERNEL_H
#include <kernel/kernel.h>
#endif

#ifndef __KERNEL_LUMBERJACK_H
#include <kernel/lumberjack.h>
#endif

#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */

extern void *AllocateNode(const struct Folio *, uint8);
extern void *AllocateSizedNode(const struct Folio *, uint8,int32);
extern void FreeNode(const struct Folio *,void *);

extern uint32 Disable(void);
extern void Enable(uint32);
extern void EnableIrq(void);
extern void DebugTrigger(void);
extern int DebugAbortTrigger(uint32 r0, uint32 r1, uint32 r2, uint32 r3);
extern void internalDebugPutChar(char ch);
extern void internalDebugPutStr(const char *str);
extern void internalDebugBreakpoint(void);
extern void dumpstate(void);
uint32 internalReadHardwareRandomNumber(void);

Err internalCreateLumberjack(const TagArg *tags);
Err internalDeleteLumberjack(void);
Err internalControlLumberjack(uint32 controlFlags);
Err internalLogEvent(const char *eventDescription);

void internalInvalidateFPState(void);

Err  internalWaitLumberjackBuffer(void);
LumberjackBuffer *internalObtainLumberjackBuffer(void);
void internalReleaseLumberjackBuffer(LumberjackBuffer *lb);

Err externalRegisterUserExceptionHandler(Item task, Item port);
Err externalControlUserExceptions(uint32 exceptions, bool captured);
Err externalCompleteUserException(Item task, const RegBlock *rb, const FPRegBlock *fprb);

Err		internalRegisterOperator(void);
void		internalFreeInitModules(void);
extern Item	internalOpenRomAppMedia(void);

extern int externalMayGetChar(void);
extern uint32 FirqInterruptControl(int32, int32);

extern void  LoadTaskRegs(struct Task *t);
extern struct Task *SaveTaskRegs(struct Task *t);
extern void ScheduleNewTask(void);
extern void NewReadyTask(struct Task *);
extern void IdleLoop(void);
extern void SleepTask(struct Task *);
extern void SuspendTask(struct Task *t);
extern void ResumeTask(struct Task *t);

extern void RemoveItem(struct Task *t, Item item);
extern void FreeItem(Item);
extern Item GetItem(void *);
extern Item AssignItem(void *, Item);
extern int32 FindItemSlot(struct Task *,Item);
extern Item NodeToItem(Node *n);
extern void DeleteSubSysItems(NodeSubsysTypes subSys);
extern Err externalIncreaseResourceTable(uint32 numSlots);

extern Err internalSetFunction(Item folio, int32 num, int32 type, void *func);

extern int32 internalAllocSignal(int32, struct Task *);
extern int32 externalAllocSignal(int32);
extern Err internalFreeSignal(int32,struct Task *);
extern Err externalFreeSignal(int32);
extern Item internalCreateSemaphore(struct Semaphore *,TagArg *);
extern int32 externalLockSemaphore(Item, int32);
extern int32 externalUnlockSemaphore(Item);
extern int32 internalLockSemaphore(struct Semaphore *, int32);
extern int32 internalUnlockSemaphore(struct Semaphore *);
extern Err internalSetSemaphoreOwner(struct Semaphore *, Item newOwner);

extern Item internalCreateItem(int32, void *);
extern Item internalCreateSizedItem(int32,void *,int32);
extern int32 internalDeleteItem(Item);

extern int32 internalCloseItemSlot(Item,struct Task *,int32 slot);
extern int32 internalCloseItem(Item,struct Task *);
extern Item internalOpenItem(Item, void *args, struct Task *);

extern Item externalOpenItem(Item, void *args);
extern int32 externalCloseItem(Item);
extern Item externalOpenItemAsTask(Item, void *args, Item);
extern int32 externalCloseItemAsTask(Item, Item);

extern Err ItemOpened(Item task, Item it);

extern Item internalCreateKernelItem(void *, uint8, void *);
extern int32 internalDeleteKernelItem(Item,struct Task *);
extern Item internalFindKernelItem(int32,TagArg *);
extern Item internalOpenKernelItem(Node *, void *, struct Task *t);
extern int32 internalCloseKernelItem(Item,struct Task *);
extern int32 internalSetPriorityKernelItem(ItemNode *,uint8,struct Task *);
extern Err internalSetOwnerKernelItem(ItemNode *,Item,struct Task *);
extern Item internalLoadKernelItem(int32 ntype, TagArg *tags);

extern int32 externalDeleteItem(Item);
extern int32 externalCloseItem(Item);

extern struct Folio *WhichFolio(int32 cntype);
extern Item internalCreateFolio(struct Folio *f,TagArg *);
extern int32 internalDeleteFolio(struct Folio *,struct Task*);

extern Item internalCreateFirq(struct FirqNode *,TagArg *);
extern int32 internalDeleteFirq(struct FirqNode *,struct Task*);

extern Item internalCreateHLInt(struct FirqNode *,TagArg *);
extern int32 internalDeleteHLInt(struct FirqNode *,struct Task*);

extern Item	internalAllocACS(char *name, int pri, int32 (*code)());
extern int32	internalPendACS(Item it);

extern int	internalRegisterPeriodicVBLACS(Item it);
extern int	internalRegisterSingleVBLACS(Item it);

extern int32 internalChangeTaskPri(struct Task *t, uint8 newpri);
extern Item internalCreateTask(struct Task *, TagArg *);
extern Item internalCreateTaskVA(Task *t, uint32 args, ...);
extern void KillSelf(void);
extern void internalSetExitStatus(int32);
extern Err externalSetExitStatus(int32);
void FreeDeadTasks(void);

extern Err CallUserExcHandler(Task *t, uint32 parm);
extern void UserExcHandlerReturn(void);
extern void UserExcHandler(void);

extern int32 internalWait(int32);

extern Err ValidateSignal(int32);
extern Err externalSignal(Item,int32);
extern Err internalSignal(struct Task *,int32);
extern void internalYield(void);
extern int32 internalKill(struct Task *, struct Task *);
extern Err internalSetTaskOwner(struct Task *, Item);
extern int32 externalSetItemPriority(Item i,uint8 pri);
extern int32 externalSetItemOwner(Item itm,Item newOwner);

extern Item internalFindItem(int32,TagArg *);
extern Item externalFindAndOpenItem(int32,TagArg *);
extern Item internalFindAndOpenItem(int32,TagArg *,Task *);

extern int32 internalDeleteSemaphore(struct Semaphore *,struct Task *);

extern char *AllocateString(const char *n);
extern char *AllocateName(const char *n);
extern void FreeString(char *n);
extern bool IsLegalName(const char *name);

/* Routines for messages and msgports */
extern Item internalCreateMsgPort(struct MsgPort *,TagArg *);
extern Item externalCreateMsg(struct Message *,TagArg *);
extern int32 internalDeleteMsgPort(struct MsgPort *,struct Task*);
extern int32 internalDeleteMsg(struct Message *,struct Task*);
extern Err internalSetMsgPortOwner(struct MsgPort *port, Item newOwner);
extern Err internalSetMsgOwner(struct Message *msg, Item newOwner);

extern Item externalGetMsg(Item mp);
extern Item externalGetThisMsg(Item msg);
extern int32 internalReplyMsg(struct Message *, int32 result, void *dataptr, int32 datasize);
extern int32 externalReplyMsg(Item Message, int32 result,
				void *dataptr, int32 datasize);
extern Item internalWaitPort(struct MsgPort *, struct Message *);
extern Item externalWaitPort(Item,Item);
extern Item oldWaitPort(Item,Item);
extern int32 externalSendMsg(Item mp,Item msg, void *dataptr, int32 datasize);
extern int32 superinternalSendMsg(struct MsgPort *msg,struct Message *mp,
				void *dataptr, int32 datasize);
extern int32 internalSendMsg(struct MsgPort *msg,struct Message *mp,
				void *dataptr, int32 datasize);
/* Routines for Devices */
extern Item internalLoadDevice(char *name);
extern Err internalUnloadDevice(struct Device *dev);
extern Item internalCreateDevice(struct Device *,TagArg *);
extern int32 internalDeleteDevice(struct Device *, struct Task *);
extern Item internalOpenDevice(struct Device *, void *a, struct Task *);
extern int32 internalCloseDevice(struct Device *, struct Task *);
extern struct DDFNode * internalFindDDF(char const* name);

extern Item internalLoadDriver(char *name);
extern Err internalUnloadDriver(struct Driver *drv);
extern Item internalCreateDriver(struct Driver *,TagArg *);
extern Err internalDeleteDriver(struct Driver *, struct Task *);
extern Item OpenDriver(struct Driver *, void *, struct Task *);
extern Err CloseDriver(struct Driver *, struct Task *);
extern Item internalCreateIOReq(struct IOReq *,TagArg *);
extern int32 internalDeleteIOReq(struct IOReq *, struct Task *);
extern Err SetIOReqOwner(struct IOReq *ior, Item newOwner);
extern Err SetDeviceOwner(struct Device *dev, Item newOwner);
extern Err SetDriverOwner(struct Driver *drv, Item newOwner);

extern Item internalCreateTimer(struct Timer *r, TagArg *ta);
extern int32 internalDeleteTimer(struct Timer *r,struct Task *);

extern Item internalCreateErrorText(struct ErrorText *r, TagArg *ta);
extern int32 internalDeleteErrorText(struct ErrorText *et,struct Task *t);
extern int32 GetSysErr(char *ubuff,int32 ubufflen,Item i);

extern int32 externalSendIO(Item, struct IOInfo *);
extern int32 internalSendIO(struct IOReq *);
extern Err internalDoIO(struct IOReq *);
extern Err externalDoIO(Item iorItem, struct IOInfo *ioInfo);
extern Err internalWaitIO(struct IOReq *);
extern int32 externalCheckIO(Item iorItem);
extern Err externalWaitIO(Item iorItem);

extern Err  internalSendDebuggedIO(struct IOReq *);
extern void CompleteDebuggedIO(struct IOReq *);
extern void CleanupDebuggedIOs(void);

extern void internalPrint3DOHeader(Item module, char *whatstr, char *copystr);

extern int32 externalAbortIO(Item);
extern void internalAbortIO(struct IOReq *);

extern void externalCompleteIO(struct IOReq *);
extern void internalCompleteIO(struct IOReq *);

extern int32 TagProcessor(void *n, TagArg *tagpt, int32 (*cb)(),void *dataP);
extern int32 TagProcessorNoAlloc(void *n, TagArg *tagpt, int32 (*cb)(),void *dataP);
extern int32 TagProcessorSearch(TagArg *ret, TagArg *tagpt, uint32 tag);

extern void InstallArmVector(int32,void (*)());

extern void OSPanic(const char *msg, uint32 err,char *file,uint32 line);

void ReplyHeldMessages(const struct Task *);

extern void SetupMMU(void);
extern void do_rfi(uint32);
extern void LoadDTLBs(uint32 *, uint8 *);
extern void LoadFence(struct Task *t);
extern void UpdateFence(void);

extern void GetIBATs(uint32 *bats);
extern void GetDBATs(uint32 *bats);
extern void GetSegRegs(uint32 *bats);

extern Err externalControlCaches(ControlCachesCmds cmd);
extern void externalInvalidateDCache(const void *start, uint32 numBytes);
extern void EnableICache(void);
extern void DisableICache(void);
extern void InvalidateICache(void);
extern void EnableDCache(void);
extern void DisableDCache(void);
extern void WriteThroughDCache(void);
extern void CopyBackDCache(void);
extern uint32 _GetCacheState(void);

extern int32 init_dev_sem(void);

extern void CopyToConst(const void *, void *);

extern void InitPagePool(PagePool *pp);
extern void *internalAllocMemPages(int32 size, uint32 flags);
extern void *externalAllocMemPages(int32 size, uint32 flags);
extern void internalFreeMemPages(void *p, int32 size);
extern void externalFreeMemPages(void *p, int32 size);
extern int32 ControlPagePool(PagePool *pp, uint8 *p, int32 size, ControlMemCmds cmd, Item taskItem);
extern int32 externalControlMem(uint8 *p, int32 size, ControlMemCmds cmd, Item it);

void *AllocFromPagePool(PagePool *pp, int32 usersize, uint32 flags, uint32 careBits, uint32 stateBits);
void FreeToPagePool(PagePool *pp, void *p, int32 size);
void *ReallocFromPagePool(PagePool *pp, void *oldMem, int32 oldSize, int32 newSize, uint32 flags);

Err externalCreateMemDebug(const TagArg *tags);
Err externalDeleteMemDebug(void);
Err externalControlMemDebug(uint32 controlFlags);
Err externalDumpMemDebug(const TagArg *tags);
Err externalSanityCheckMemDebug(const char *banner, const TagArg *tags);
Err externalRationMemDebug(const TagArg *tags);
void *externalAllocMemPagesDebug(int32 memSize, uint32 memFlags, const char *file, int32 lineNum);
void  externalFreeMemPagesDebug(void *mem, int32 memSize, const char *file, int32 lineNum);

void DeleteTaskMemDebug(const struct Task *t);

extern struct KernelBase *KernelBase;
extern struct KernelBase  KB;


extern void InitIO(void);

extern void SaveFPRegBlock(const FPRegBlock *fprb);
extern void RestoreFPRegBlock(const FPRegBlock *fprb);

extern Err internalGetPersistentMem(PersistentMemInfo *info, uint32 infoSize);

void DumpTask(const char *banner, const struct Task *t);
void DumpTaskRegs(const char *banner, const struct Task *t);
void DumpRegBlock(const char *banner, const struct RegBlock *rb);
void DumpFPRegBlock(const char *banner, const struct FPRegBlock *fprb);
void DumpSysRegs(const char *banner);

void internalSlaveExit(Err status);
void SlaveExit(Err status);


#ifdef  __cplusplus
}
#endif  /* __cplusplus */


#ifdef __DCC__
#pragma no_return IdleLoop, LoadTaskRegs, ScheduleNewTask, internalSlaveExit, SlaveExit
#pragma no_side_effects IsRamAddr, ValidateMem
#pragma no_side_effects externalCheckIO
#pragma no_side_effects ItemOpened
#endif


#endif	/* __INTERNALF_H */
