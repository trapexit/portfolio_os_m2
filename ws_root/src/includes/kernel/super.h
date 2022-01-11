#ifndef __KERNEL_SUPER_H
#define __KERNEL_SUPER_H


/******************************************************************************
**
**  @(#) super.h 96/09/10 1.53
**
******************************************************************************/


#ifdef EXTERNAL_RELEASE
#error "This file may not be used in externally released source code or link lib"
#endif

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_TASK_H
#include <kernel/task.h>
#endif

#ifndef __KERNEL_SEMAPHORE_H
#include <kernel/tags.h>
#endif

#ifndef __KERNEL_IO_H
#include <kernel/io.h>
#endif

#ifndef __KERNEL_MSGPORT_H
#include <kernel/msgport.h>
#endif

#ifndef	__DIPIR_HWRESOURCE_H
#include <dipir/hwresource.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern uint32           Disable (void);
extern void             Enable (uint32);

/* This macro increments the forbid nest count. When the forbid count is
 * greater than 0, then preemptive task switching is not performed. To undo
 * the effect of this call, use Permit(). Note that Forbid()/Permit() can
 * safely be nested.
 */
#define Forbid() CURRENTTASK->t_Forbid++
extern void Permit(void);

extern Item             SuperCreateItem (int32 ctype, TagArg *tags);
extern Item             SuperDeleteItem (Item);
extern Item             SuperCloseItem (Item);
extern Item             SuperOpenItem (Item founditem, void *args);
extern Item             SuperFindItem (int32 ctype, TagArg *tp);
extern Item             SuperFindNamedItem (int32 ctype, char *name);
extern Item             SuperCreateSizedItem (int32 ctype, void *p, int32 size);

/* delete an item no matter who owns it */
extern Item             SuperInternalDeleteItem(Item);

/* close an item on behalf of another task */
extern Item		SuperInternalCloseItem(Item item, struct Task *t);

/* open an item on behalf of another task */
extern Item		SuperInternalOpenItem(Item it, void *args, struct Task *t);
extern Item		SuperInternalFindAndOpenItem(int32 cntype, TagArg *tags, Task *t);

extern Item             SuperInternalCreateFirq(struct FirqNode *, TagArg *);
extern uint32           SuperFirqInterruptControl(int32, int32);
extern Item             SuperInternalCreateTaskVA(Task *, uint32 tag1, ...);
extern Item             SuperInternalCreateTimer(struct Timer *, TagArg *);

extern Item		SuperOpenRomAppMedia (void);
extern                  Superkprintf (const char *fmt,...);

extern int32            SuperWaitSignal (int32);
extern int32            SuperAllocSignal (int32);
extern Err              SuperFreeSignal (int32);
extern Err              SuperInternalFreeSignal (int32 sigs, struct Task *t);

extern int32            SuperWaitIO (Item IOReq);
extern Err              SuperDoIO (Item IOReq, struct IOInfo *ioi);
extern int32            SuperInternalSignal (struct Task *t, uint32 bits);

extern int32            SuperLockSemaphore (Item s, uint32 wait);
extern int32            SuperUnlockSemaphore (Item s);

extern int32            SuperInternalLockSemaphore (struct Semaphore * s, uint32 wait);
extern int32            SuperInternalUnlockSemaphore (struct Semaphore * s);

extern Err              SuperSendMsg (Item mp, Item msg, void *dataptr, int32 datasize);
extern Err              SuperSendSmallMsg (Item mp, Item msg, uint32 val1, uint32 val2);

extern Item             SuperGetMsg (Item mp);
extern Item             SuperWaitPort (Item mp, Item msg);
extern Item             SuperInternalWaitPort (struct MsgPort *mp, struct Msg *msg);
extern Err              SuperReplyMsg (Item msg, int32 result, void *dataPtr, int32 dataSize);
extern Err              SuperInternalReplyMsg(struct Message *msg, int32 result, void *dataptr, int32 datasize);

extern int32            SuperReportEvent (void *eventFrame);

extern int32            SuperInternalPutMsg (struct MsgPort *, struct Message *, void *dataptr, int32 datasize);
extern int32            SuperInternalSendIO (struct IOReq *);
extern Err              SuperInternalDoIO (struct IOReq *);
extern Err              SuperInternalWaitIO( struct IOReq *);
extern void             SuperCompleteIO (struct IOReq *);
extern void             SuperInternalAbortIO (struct IOReq *);

#define SuperCreateFIRQ(n,p,c,u)       CreateFIRQ((n), (p), (c), (u))
#define SuperCreateIOReq(n,p,d,m)      CreateIOReq((n), (p), (d), (m))
#define SuperCreateMsgPort(n,p,s)      CreateMsgPort((n), (p), (s))
#define SuperCreateMsg(n,p,m)          CreateMsg((n), (p), (m))
#define SuperCreateSmallMsg(n,p,m)     CreateSmallMsg((n), (p), (m))
#define SuperCreateBufferdMsg(n,p,m,s) CreateBufferedMsg((n), (p), (m), (s))
#define SuperCreateSemaphore(n,p)      CreateSemaphore((n), (p))

extern void            *SuperSetFunction (Item folio, int32 vnum, int32 vtype, void *newfunc);

extern Err              SuperSetItemOwner (Item i, Item newOwnerTask);

extern uint32		SuperQuerySysInfo(uint32 tag, void *info, size_t infosize);
extern uint32		SuperSetSysInfo(uint32 tag, void *info, size_t infosize);

extern int32            TagProcessor (void *n, TagArg *tagpt, int32 (*cb) (), void *dataP);
extern int32            TagProcessorNoAlloc(void *n, TagArg *tagpt, int32 (*cb) (), void *dataP);

typedef	Err	(*CallBackProcPtr)();

extern Err              CallBackSuper (CallBackProcPtr proc, uint32 arg1, uint32 arg2, uint32 arg3);

extern int32		ChannelRead(HardwareID id, uint32 offset, uint32 len, void *buffer);
extern int32		ChannelMap(HardwareID id, uint32 offset, uint32 len, void **paddr);
extern int32		ChannelUnmap(HardwareID id, uint32 offset, uint32 len);
extern int32		ChannelGetHWIcon(HardwareID id, void *buffer, uint32 bufLen);

extern Err              RegisterDuck (void (*code) (), uint32 param);
extern Err              RegisterRecover (void (*code) (), uint32 param);
extern Err              UnregisterDuck (void (*code) ());
extern Err              UnregisterRecover (void (*code) ());
extern void             RegisterReportEvent(int32 (*newHook) (void *event));
extern void             TriggerDeviceRescan (void);
extern Err              ScanForDDFToken (void *np, char const *name, struct DDFTokenSeq *rhstokens);
#define ScanForDDFNeed(ddf,needname,needrhstokens) \
	ScanForDDFToken((ddf)->ddf_Needs, (needname), (needrhstokens))
#define ScanForDDFProvide(ddf,provname,provrhstokens) \
	ScanForDDFToken((ddf)->ddf_Provides, (provname), (provrhstokens))
extern Err NextDDFToken(struct DDFTokenSeq *seq, struct DDFToken *token, bool peek);
#define GetDDFToken(seq,tokptr) NextDDFToken((seq), (tokptr), FALSE)
#define PeekDDFToken(seq,tokptr) NextDDFToken((seq), (tokptr), TRUE)
extern bool SatisfiesNeed(struct DDFNode* ddf, struct DDFTokenSeq* needs);

/* CD-style ECC correction for drivers that need it. */
extern int32 SectorECC(uint8 *buffer);

extern Err FindHWResource(HardwareID hwID, struct HWResource *hwr);

extern Item OpenSlotDevice(HardwareID hwID);

extern Err CreateMemMapRecord(struct List *list, const struct IOReq *ior);
extern Err DeleteMemMapRecord(struct List *list, const struct IOReq *ior);
extern Err DeleteAllMemMapRecords(struct List *list);
extern int32 BytesMemMapped(struct List *list);

extern	void		SuperBeginNoReboot(void);
extern	void		SuperEndNoReboot(void);
extern	void		SuperExpectDataDisc(void);
extern	void		SuperNoExpectDataDisc(void);

extern void *SuperAllocNode(const void *, uint8);
extern void SuperFreeNode(const void *,void *);

extern Item SuperInternalCreateSemaphore(struct Semaphore *,TagArg *);
extern Item SuperInternalCreateErrorText(struct ErrorText *r, TagArg *ta);

Err SuperLogEvent(const char *eventDescription);


void SuperSlaveRequest(enum SlaveActions action, uint32 arg1, uint32 arg2, uint32 arg3);
Err InstallMasterHandler(enum MasterActions action, void *handler, void *handlerData);

void SuperSetSystemTimeTT(struct TimerTicks *tt);


#ifdef __cplusplus
}
#endif

#define SuperinternalSignal	SuperInternalSignal
#define SuperinternalAbortIO	SuperInternalAbortIO
#define SuperinternalSendIO	SuperInternalSendIO
#define SuperDeleteMsg(x)	SuperDeleteItem(x)
#define SuperDeleteMsgPort(x)	SuperDeleteItem(x)
#define SuperFindMsgPort(n)     SuperFindNamedItem(MKNODEID(KERNELNODE,MSGPORTNODE),(n))
#define SuperDeleteSemaphore(s)	SuperDeleteItem(s)
#define SuperFindSemaphore(n)   SuperFindNamedItem(MKNODEID(KERNELNODE,SEMAPHORENODE),(n))
#define	SuperDeleteFIRQ(x)	SuperDeleteItem(x)


/*****************************************************************************/


#ifdef __DCC__
#pragma no_side_effects SuperFindItem, SuperFindNamedItem
#endif


/*****************************************************************************/


#endif /* __KERNEL_SUPER_H */
