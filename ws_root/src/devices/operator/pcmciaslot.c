/* %Z pcmciaslot.c %E 1.30 */

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/driver.h>
#include <kernel/device.h>
#include <kernel/kernelnodes.h>
#include <kernel/debug.h>
#include <kernel/kernel.h>
#include <kernel/operror.h>
#include <kernel/super.h>
#include <kernel/mem.h>
#include <kernel/sysinfo.h>
#include <kernel/interrupts.h>
#include <kernel/cache.h>
#include <device/slot.h>
#include <device/dma.h>
#include <dipir/hwresource.h>
#include <hardware/PPCasm.h>
#include <hardware/cde.h>
#include <loader/loader3do.h>
#include <string.h>

#define	FIRST_SLOT		5
#define	NUM_SLOTS		3
#define	SLOT_SIZE		(64 * 1024 * 1024)

/*#define DEBUG */

#ifdef DEBUG
#define	DBUG(x) printf x
#else
#define	DBUG(x)
#endif

uint32 NsecPerTick;
Item MediaChangeFirq = -1;

#define	NsecToTicks(ns)	(((ns) + NsecPerTick - 1) / NsecPerTick)

/* Per-slot data structures. */
typedef struct SlotState
{
	uint32	ss_Slot;
	List	ss_MapRecords;
} SlotState;

/*****************************************************************************
  Hardware register addresses and bit definitions.
*/
static const uint32 _CycleTimeReg[] = {
	CDE_DEV5_CYCLE_TIME, CDE_DEV6_CYCLE_TIME, CDE_DEV7_CYCLE_TIME
};
static const uint32 _DevSetupReg[] = {
	CDE_DEV5_SETUP, CDE_DEV6_SETUP, CDE_DEV7_SETUP
};
static const uint32 _DevConfReg[] = {
	CDE_DEV5_CONF, CDE_DEV6_CONF, CDE_DEV7_CONF
};
static uint32 _SlotInterruptNumber[] = {
	INT_CDE_DEV5, INT_CDE_DEV6, INT_CDE_DEV7
};

#define CycleTimeReg(slot)	_CycleTimeReg		[(slot)-FIRST_SLOT]
#define DevSetupReg(slot)	_DevSetupReg		[(slot)-FIRST_SLOT]
#define DevConfReg(slot)	_DevConfReg		[(slot)-FIRST_SLOT]
#define	SlotInterruptNumber(slot) _SlotInterruptNumber	[(slot)-FIRST_SLOT]


/*****************************************************************************
  Is a slot on the base system?
*/
static bool
BaseSlot(Slot slot)
{
	return (slot >= FIRST_SLOT && slot < FIRST_SLOT + NUM_SLOTS);
}

/*****************************************************************************
  Find the HWResource of the card in a specified slot.
*/
static bool
HWInSlot(Slot slot, HWResource *hwr)
{
	hwr->hwr_InsertID = 0;
	while (NextHWResource(hwr) >= 0)
	{
		if (hwr->hwr_Channel == CHANNEL_PCMCIA &&
		    hwr->hwr_Slot == slot)
			/* Found it. */
			return TRUE;
	}
	return FALSE;
}

/*****************************************************************************
  How many bits do we have to right-shift the mask to get the
  lowest order 1-bit into bit position 0?
*/
static uint32
GetShift(uint32 mask)
{
	uint32 shift;

	for (shift = 0;  shift < 32;  shift++)
		if (mask & (1 << shift))
			break;
	return shift;
}

/*****************************************************************************
  Insert the value into the field specified by the mask.
*/
static int32
InsertField(uint32 *pReg, uint32 mask, uint32 value)
{
	uint32 shift = GetShift(mask);

#ifdef BUILD_PARANOIA
	if (value > (mask >> shift))
	{
		printf("PCMCIA: reg %x, mask %x, value %x too big!\n",
			*pReg, mask, value);
		return 1;
	}
#endif
	*pReg = (*pReg & ~mask) | ((value << shift) & mask);
	return 0;
}

/*****************************************************************************
  Set the low-level timing characteristics of a slot.
*/
static int32
CmdSetTiming(IOReq *ior)
{
	SlotState *ss;
	SlotTiming *st;
	uint32 setup;
	uint32 ctime;

	ss = ior->io_Dev->dev_DriverData;
	if (!BaseSlot(ss->ss_Slot))
	{
		DBUG(("PCMCIA.Timing: bad slot %d\n", ss->ss_Slot));
		ior->io_Error = BADIOARG;
		return 1;
	}
	DBUG(("Slotdev: SetTiming slot %d\n", ss->ss_Slot));

	if (ior->io_Info.ioi_Send.iob_Len < sizeof(SlotTiming))
	{
		ior->io_Error = BADIOARG;
		return 1;
	}
	st = (SlotTiming *) ior->io_Info.ioi_Send.iob_Buffer;

	setup = CDE_READ(KernelBase->kb_CDEBase, DevSetupReg(ss->ss_Slot));
	ctime = CDE_READ(KernelBase->kb_CDEBase, CycleTimeReg(ss->ss_Slot));
	DBUG(("Old: setup %x, cycle-time %x\n", setup, ctime));

	switch (st->st_DeviceWidth)
	{
	case 8:
		setup = (setup & ~CDE_DATAWIDTH) | CDE_DATAWIDTH_8;
		break;
	case 16:
		setup = (setup & ~CDE_DATAWIDTH) | CDE_DATAWIDTH_16;
		break;
	case 0:
		break;
	default:
	BadIOArg:
		ior->io_Error = BADIOARG;
		return 1;
	}
	if (st->st_ReadSetupTime != ST_NOCHANGE)
	    if (InsertField(&setup, CDE_READ_SETUP,
			NsecToTicks(st->st_ReadSetupTime)))
		goto BadIOArg;
	if (st->st_ReadHoldTime != ST_NOCHANGE)
	    if (InsertField(&setup, CDE_READ_HOLD,
			NsecToTicks(st->st_ReadHoldTime)))
		goto BadIOArg;
	if (st->st_WriteSetupTime != ST_NOCHANGE)
	    if (InsertField(&setup, CDE_WRITEN_SETUP,
			NsecToTicks(st->st_WriteSetupTime)))
		goto BadIOArg;
	if (st->st_WriteHoldTime != ST_NOCHANGE)
	    if (InsertField(&setup, CDE_WRITEN_HOLD,
			NsecToTicks(st->st_WriteHoldTime)))
		goto BadIOArg;
	if (st->st_IOReadSetupTime != ST_NOCHANGE)
	    if (InsertField(&setup, CDE_READ_SETUP_IO,
			NsecToTicks(st->st_IOReadSetupTime)))
		goto BadIOArg;
	InsertField(&setup, CDE_PAGEMODE,
		(st->st_Flags & ST_PAGE_MODE) ? 1 : 0);
	InsertField(&setup, CDE_MODEA,
		(st->st_Flags & ST_SYNCA_MODE) ? 1 : 0);
	InsertField(&setup, CDE_HIDEA,
		(st->st_Flags & ST_HIDEA_MODE) ? 1 : 0);

	if (st->st_CycleTime != ST_NOCHANGE)
	   if (InsertField(&ctime, CDE_CYCLE_TIME,
			NsecToTicks(st->st_CycleTime) + 1))
		goto BadIOArg;
	if (st->st_PageModeCycleTime != ST_NOCHANGE)
	   if (InsertField(&ctime, CDE_PAGEMODE_CYCLE_TIME,
			NsecToTicks(st->st_PageModeCycleTime) + 1))
		goto BadIOArg;

	DBUG(("New: setup %x, cycle-time %x\n", setup, ctime));
	CDE_WRITE(KernelBase->kb_CDEBase, DevSetupReg(ss->ss_Slot), setup);
	CDE_WRITE(KernelBase->kb_CDEBase, CycleTimeReg(ss->ss_Slot), ctime);
	return 1;
}

/*****************************************************************************
  Get the status of a slot.
*/
static int32
CmdStatus(IOReq *ior)
{
	SlotState *ss;
	uint32 len;
	SlotDeviceStatus stat;
	HWResource hwr;

	ss = ior->io_Dev->dev_DriverData;
	DBUG(("Slotdev: Status slot %d\n", ss->ss_Slot));
	memset(&stat, 0, sizeof(stat));
	stat.sds.ds_DriverIdentity = DI_OTHER;
	stat.sds.ds_FamilyCode = DS_DEVTYPE_OTHER;
	stat.sds.ds_MaximumStatusSize = sizeof(DeviceStatus);
	stat.sds.ds_DeviceBlockSize = 1;
	stat.sds.ds_DeviceBlockCount = 0;
	stat.sds.ds_DeviceBlockStart = 0;
	stat.sds.ds_DeviceUsageFlags = 0;
	stat.sds_IntrNum = SlotInterruptNumber(ss->ss_Slot & SLOT_BASE_MASK);

	if (!HWInSlot(ss->ss_Slot, &hwr))
	{
		/* No card in the slot. */
		stat.sds.ds_DeviceUsageFlags |= DS_USAGE_OFFLINE;
	} else
	{
		if (hwr.hwr_Perms & HWR_WRITE_PROTECT)
			stat.sds.ds_DeviceUsageFlags |= DS_USAGE_READONLY;
	}

	len = ior->io_Info.ioi_Recv.iob_Len;
	if (len > sizeof(stat))
		len = sizeof(stat);
	memcpy(ior->io_Info.ioi_Recv.iob_Buffer, &stat, len);
	ior->io_Actual = len;
	return 1;
}

/*****************************************************************************
  Handle completion of a DMA operation.
*/
void
DMAComplete(void *arg, Err err)
{
	IOReq *ior = arg;

	ior->io_Error = err;
	ior->io_Actual = (ior->io_Info.ioi_Command == CMD_BLOCKREAD) ?
			ior->io_Info.ioi_Recv.iob_Len :
			ior->io_Info.ioi_Send.iob_Len;
	SuperCompleteIO(ior);
}

/*****************************************************************************
  Do a DMA command (read or write).
*/
static int32
CmdDMA(IOReq *ior)
{
	SlotState *ss;
	HWResource hwr;
	uint32 setup;
	uint32 dmaFlags;

	ss = ior->io_Dev->dev_DriverData;
	DBUG(("PCMCIA.DMA: slot %d: offset %x, len %x\n",
		ss->ss_Slot, ior->io_Info.ioi_Offset,
		ior->io_Info.ioi_Command == CMD_BLOCKREAD ?
			ior->io_Info.ioi_Recv.iob_Len :
			ior->io_Info.ioi_Send.iob_Len));

	if (!HWInSlot(ss->ss_Slot, &hwr))
	{
		ior->io_Error = BADIOARG;
		return 1;
	}

	/* Allocate a DMA channel and schedule the operation. */
	setup = CDE_READ(KernelBase->kb_CDEBase, DevSetupReg(ss->ss_Slot));
	switch (setup & CDE_DATAWIDTH)
	{
	case CDE_DATAWIDTH_8:
		dmaFlags = DMA_8BIT;
		break;
	case CDE_DATAWIDTH_16:
		dmaFlags = DMA_16BIT;
		break;
	default:
		dmaFlags = 0;
	}

	ior->io_Flags &= ~IO_QUICK;
	switch (ior->io_Info.ioi_Command)
	{
	case CMD_BLOCKREAD:
		StartDMA(hwr.hwr_DeviceSpecific[0] + ior->io_Info.ioi_Offset,
			ior->io_Info.ioi_Recv.iob_Buffer,
			ior->io_Info.ioi_Recv.iob_Len,
			DMA_READ | dmaFlags, DMA_CHANNEL_ANY, 
			DMAComplete, (void*)ior);
		break;
	case CMD_BLOCKWRITE:
		StartDMA(hwr.hwr_DeviceSpecific[0] + ior->io_Info.ioi_Offset,
			ior->io_Info.ioi_Send.iob_Buffer,
			ior->io_Info.ioi_Send.iob_Len,
			DMA_WRITE | dmaFlags, DMA_CHANNEL_ANY, 
			DMAComplete, (void*)ior);
		break;
	}
	return 0;
}

/*****************************************************************************
*/
static int32
CmdGetMapInfo(IOReq *ior)
{
	SlotState *ss;
	int32 len;
	MemMappableDeviceInfo info;
	HWResource hwr;

	ss = ior->io_Dev->dev_DriverData;
	if (!HWInSlot(ss->ss_Slot, &hwr))
	{
		ior->io_Error = BADIOARG;
		return 1;
	}
	memset(&info, 0, sizeof(info));
	info.mmdi_Flags =
		MM_MAPPABLE | MM_READABLE | MM_EXECUTABLE | MM_EXCLUSIVE;
	if (!(hwr.hwr_Perms & HWR_WRITE_PROTECT))
		info.mmdi_Flags |= MM_WRITABLE;
	if (hwr.hwr_Perms & HWR_XIP_OK)
		info.mmdi_Permissions |= XIP_OK;
	info.mmdi_SpeedPenaltyRatio = 0;
	info.mmdi_MaxMappableBlocks = SLOT_SIZE;
	info.mmdi_CurBlocksMapped = BytesMemMapped(&ss->ss_MapRecords);

	len = ior->io_Info.ioi_Recv.iob_Len;
	if (len > sizeof(info))
		len = sizeof(info);
	memcpy(ior->io_Info.ioi_Recv.iob_Buffer, &info, len);
	ior->io_Actual = len;
	return 1;
}

/*****************************************************************************
*/
static int32
CmdMapRange(IOReq *ior)
{
	int32 len;
	SlotState *ss;
	int32 offset;
	int32 err;
	MapRangeRequest *req;
	MapRangeResponse resp;
	HWResource hwr;

	ss = ior->io_Dev->dev_DriverData;
	if (ior->io_Info.ioi_Send.iob_Len < sizeof(MapRangeRequest))
	{
		ior->io_Error = BADIOARG;
		return 1;
	}
	if (!HWInSlot(ss->ss_Slot, &hwr))
	{
		ior->io_Error = BADIOARG;
		return 1;
	}
	req = (MapRangeRequest *) ior->io_Info.ioi_Send.iob_Buffer;
	offset = ior->io_Info.ioi_Offset;
	if (offset < 0 ||
	    req->mrr_BytesToMap < 0 ||
	    offset + req->mrr_BytesToMap > SLOT_SIZE)
	{
		DBUG(("PCMCIA.Map: bad range\n"));
		ior->io_Error = BADIOARG;
		return 1;
	}

	err = CreateMemMapRecord(&ss->ss_MapRecords, ior);
	if (err < 0)
	{
		DBUG(("PCMCIA.Map: CreateMMR failed\n"));
		ior->io_Error = err;
		return 1;
	}
	memset(&resp, 0, sizeof(resp));
	resp.mrr_Flags = MM_MAPPABLE | MM_READABLE | MM_EXECUTABLE | MM_EXCLUSIVE;
	if (!(hwr.hwr_Perms & HWR_WRITE_PROTECT))
		resp.mrr_Flags |= MM_WRITABLE;
	resp.mrr_Flags &= req->mrr_Flags;
	resp.mrr_MappedArea = (uint8 *) hwr.hwr_DeviceSpecific[0] + offset;

	len = ior->io_Info.ioi_Recv.iob_Len;
	if (len > sizeof(resp))
		len = sizeof(resp);
	memcpy(ior->io_Info.ioi_Recv.iob_Buffer, &resp, len);
	ior->io_Actual = len;
	return 1;
}

/*****************************************************************************
*/
static int32
CmdUnmapRange(IOReq *ior)
{
	SlotState *ss;
	int32 err;

	ss = ior->io_Dev->dev_DriverData;
	if (ior->io_Info.ioi_Send.iob_Len < sizeof(MapRangeRequest))
	{
		ior->io_Error = BADIOARG;
		return 1;
	}

	err = DeleteMemMapRecord(&ss->ss_MapRecords, ior);
	if (err < 0)
	{
		ior->io_Error = err;
		return 1;
	}
	ior->io_Actual = 0;
	return 1;
}

/*****************************************************************************
  Abort an IO operation in progress.
*/
static void
SlotDeviceAbortIO(IOReq *ior)
{
	if (AbortDMA(DMAComplete, (void*)ior) < 0)
		return;
	ior->io_Error = ABORTED;
	SuperCompleteIO(ior);
}

/*****************************************************************************
*/
static int32
MediaChangeHandler(void)
{
	DBUG(("PCMCIA media change!\n"));

	/* if the media access was caused by a card, call TriggerDeviceRescan()
	 * to begin the Duck/SoftReset/Dipir/Recover process.
	 */
	if (CDE_READ(KernelBase->kb_CDEBase, CDE_BBLOCK) &
	   (CDE_DEV5_DIPIR | CDE_DEV6_DIPIR))
		TriggerDeviceRescan();

	ClearInterrupt(INT_CDE_MC);
	return 0;
}

/*****************************************************************************
*/
static Item
SlotDriverCreate(Driver *drv)
{
	SystemInfo si;
	Slot slot;
	Err err;

	err = SuperQuerySysInfo(SYSINFO_TAG_SYSTEM, (void *)&si, sizeof(si));
	if (err < 0)
		return err;
	NsecPerTick = 1000000000 / si.si_BusClkSpeed;

	SlotInterruptNumber(7) = si.si_Slot7Intr;

	if (si.si_SysFlags & SYSINFO_SYSF_HOT_PCMCIA)
	{
		/* PCMCIA supports hot insertion/removal. */
		MediaChangeFirq = SuperCreateFIRQ("PCMCIA media change", 1,
					MediaChangeHandler, INT_CDE_MC);
		ClearInterrupt(INT_CDE_MC);
		EnableInterrupt(INT_CDE_MC);
		for (slot = FIRST_SLOT;  slot < FIRST_SLOT + NUM_SLOTS;  slot++)
		{
			CDE_SET(KernelBase->kb_CDEBase,
				DevConfReg(slot), CDE_BIOBUS_SAFE);
		}
	}

	return drv->drv.n_Item;
}

/*****************************************************************************
*/
static Err
ChangeSlotDriverOwner(Driver *drv, Item newOwner)
{
	TOUCH(drv);
	SetItemOwner(MediaChangeFirq, newOwner);
	return 0;
}

/*****************************************************************************
*/
static Item
SlotDeviceCreate(Device *dev)
{
	HWResource hwr;
	SlotState *ss;
	Err err;

	err = FindHWResource(dev->dev_HWResource, &hwr);
	if (err < 0)
		return err;
	if (hwr.hwr_Channel != CHANNEL_PCMCIA)
		return MAKEKERR(ER_SEVER,ER_C_STND,ER_NoHardware);
	ss = dev->dev_DriverData;
	ss->ss_Slot = hwr.hwr_Slot;
	InitList(&ss->ss_MapRecords, "PCMCIA mem maps");
	dev->dev_DriverData = ss;
	return dev->dev.n_Item;
}

/*****************************************************************************
*/
static Item
SlotDeviceDelete(Device *dev)
{
	SlotState *ss;

	ss = dev->dev_DriverData;
	DeleteAllMemMapRecords(&ss->ss_MapRecords);
	return 0;
}

/*****************************************************************************
*/
static DriverCmdTable CmdTable[] =
{
	CMD_STATUS,		CmdStatus,
	CMD_BLOCKREAD,		CmdDMA,
	CMD_BLOCKWRITE,		CmdDMA,
	CMD_GETMAPINFO,		CmdGetMapInfo,
	CMD_MAPRANGE,		CmdMapRange,
	CMD_UNMAPRANGE,		CmdUnmapRange,
	SLOTCMD_SETTIMING,	CmdSetTiming,
};
#define	NUM_CMDS	(sizeof(CmdTable) / sizeof(CmdTable[0]))

/*****************************************************************************
*/
Item
PCMCIADriverMain(void)
{
	return CreateItemVA(MKNODEID(KERNELNODE,DRIVERNODE),
		TAG_ITEM_NAME,			"pcmciaslot",
		CREATEDRIVER_TAG_CMDTABLE,	CmdTable,
		CREATEDRIVER_TAG_NUMCMDS,	NUM_CMDS,
		CREATEDRIVER_TAG_CREATEDRV,	SlotDriverCreate,
		CREATEDRIVER_TAG_ABORTIO,	SlotDeviceAbortIO,
		CREATEDRIVER_TAG_CREATEDEV,	SlotDeviceCreate,
		CREATEDRIVER_TAG_DELETEDEV,	SlotDeviceDelete,
		CREATEDRIVER_TAG_DEVICEDATASIZE,sizeof(SlotState),
		CREATEDRIVER_TAG_CHOWN_DRV,	ChangeSlotDriverOwner,
		CREATEDRIVER_TAG_MODULE,	FindCurrentModule(),
		TAG_END);
}

