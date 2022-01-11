/* @(#) mchan.c 96/09/06 1.10 */

/*#define DEBUG*/

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/device.h>
#include <kernel/dipir.h>
#include <kernel/driver.h>
#include <kernel/io.h>
#include <kernel/mem.h>
#include <kernel/operror.h>
#include <kernel/debug.h>
#include <kernel/kernel.h>
#include <kernel/super.h>
#include <hardware/microcard.h>
#include <device/microslot.h>
#include <loader/loader3do.h>
#include <string.h>

/* Microcard commands */
#define	SCC_READCONFIG	0x01
#define	SCC_READ_LEN	6

#ifndef BUILD_STRINGS
#undef DEBUG
#endif

#ifdef DEBUG
#define DBUG(x) printf x
#else
#define DBUG(x)
#endif

typedef struct CardState
{
	uint32		cs_Slot;
	Item		cs_SlotItem;
	uint32		cs_Size;
	uint32		cs_Start;
	bool		cs_UseDownload;
} CardState;

typedef struct IOReqAux
{
    MicroSlotSeq	iora_Seq[5];
    uchar		iora_ReadCmd[SCC_READ_LEN];
    uint32		iora_Overhead;
} IOReqAux;



static Item
DeviceInit(Device* dev)
{
	Item slotdev;
	HWResource hwr;
	CardState *cs;
	IOReq *slotior;
	Item slotiorItem;
	Err err;
	MicroSlotStatus stat;

	DBUG(("mchan: DeviceInit\n"));
	err = FindHWResource(dev->dev_HWResource, &hwr);
	if (err < 0)
		return err;
	if (hwr.hwr_Channel != CHANNEL_MICROCARD)
		return MAKEKERR(ER_SEVER,ER_C_STND,ER_NoHardware);

	/* Open the MicroSlot device. */
	slotdev = OpenSlotDevice(dev->dev_HWResource);
	if (slotdev < 0)
	{
		DBUG(("Cannot open microslot device %x\n", slotdev));
		return slotdev;
	}

	cs = dev->dev_DriverData;
	cs->cs_Slot = hwr.hwr_Slot;
	cs->cs_SlotItem = slotdev;
	cs->cs_Size = hwr.hwr_ROMSize;
	cs->cs_Start = hwr.hwr_ROMUserStart;
	cs->cs_UseDownload = FALSE;

	/* Send CMD_STATUS to the slot device. */
	slotiorItem = CreateIOReq(0, 0, cs->cs_SlotItem, 0);
	if (slotiorItem < 0)
	{
		CloseItem(slotdev);
		return slotiorItem;
	}
	slotior = LookupItem(slotiorItem);
	slotior->io_Info.ioi_Command = CMD_STATUS;
	slotior->io_Info.ioi_Recv.iob_Buffer = &stat;
	slotior->io_Info.ioi_Recv.iob_Len = sizeof(stat);
	slotior->io_Info.ioi_Send.iob_Len = 0;
	err = SuperInternalDoIO(slotior);
	DeleteItem(slotiorItem);
	if (err < 0)
	{
		CloseItem(slotdev);
		return err;
	}
	if (stat.us_CardDownloadable)
		cs->cs_UseDownload = TRUE;
	return dev->dev.n_Item;
}

static Err
DeviceDelete(Device *dev)
{
	CardState *cs;
	uint8 oldPriv;

	cs = dev->dev_DriverData;
	oldPriv = PromotePriv(CURRENTTASK);
	CloseItemAsTask(cs->cs_SlotItem, dev->dev.n_Owner);
	DemotePriv(CURRENTTASK, oldPriv);
	return 0;
}

static Err
ChangeDeviceOwner(Device *dev, Item newOwner)
{
	CardState* cs;
	Err err;

	/* Transfer the "openness" of the slot device to the new owner. */
	cs = dev->dev_DriverData;
	err = OpenItemAsTask(cs->cs_SlotItem, NULL, newOwner);
	if (err < 0)
		return err;
	CloseItem(cs->cs_SlotItem);
	return 0;
}

static Item
CreateIO(IOReq *ior)
{
	IOReq* slotior;
	Item slotiorItem;
	IOReqAux *aux;
	CardState *cs;

	DBUG(("mchan: CreateIO\n"));
	cs = ior->io_Dev->dev_DriverData;
	aux = SuperAllocMem(sizeof(IOReqAux), MEMTYPE_NORMAL);
	if (aux == NULL)
		return NOMEM;

	slotiorItem = CreateIOReq(0, 0, cs->cs_SlotItem, 0);
	if (slotiorItem < 0)
	{
		DBUG(("Cannot create microslot IOReq %x\n", slotiorItem));
		SuperFreeMem(aux, sizeof(IOReqAux));
		return slotiorItem;
	}
	slotior = (IOReq *) LookupItem(slotiorItem);
	if (slotior == NULL)
	{
		DBUG(("Cannot lookup microslot IOReq\n"));
		SuperFreeMem(aux, sizeof(IOReqAux));
		DeleteItem(slotiorItem);
		return MakeKErr(ER_SEVERE,ER_C_STND,ER_SoftErr);
	}
	ior->io_Extension[0]= (int32) slotior;
	ior->io_Extension[1]= (int32) aux;
	return 0;
}

static Err
ChangeIOReqOwner(IOReq *ior, Item newOwner)
{
	IOReq *slotior;

	DBUG(("mchan: chown\n"));
	slotior = (IOReq *) ior->io_Extension[0];
	if (slotior == NULL)
		return BADITEM;
	return SuperSetItemOwner(slotior->io.n_Item, newOwner);
}

static void
DeleteIO(IOReq* ior)
{
	IOReq *slotior;
	IOReqAux *aux;

	DBUG(("mchan: DeleteIO\n"));
	slotior = (IOReq *) ior->io_Extension[0];
	aux = (IOReqAux*) ior->io_Extension[1];

	if (slotior != NULL)
		DeleteItem(slotior->io.n_Item);
	if (aux != NULL)
	SuperFreeMem(aux, sizeof(IOReqAux));
}

static Item
DeviceOpen(Device *dev, Task *t)
{
	CardState *cs = dev->dev_DriverData;
	Err err;

	DBUG(("mchan: DeviceOpen\n"));
	err = SuperInternalOpenItem(cs->cs_SlotItem, 0, t);
	if (err < 0)
		return err;
	return dev->dev.n_Item;
}

static Err
DeviceClose(Device *dev, Task *t)
{
	CardState *cs = dev->dev_DriverData;

	DBUG(("mchan: DeviceClose\n"));
	SuperInternalCloseItem(cs->cs_SlotItem, t);
	return 0;
}

static void
FinishCmdStatus(IOReq *ior)
{
	CardState *cs;
	DeviceStatus *stat;

	cs = ior->io_Dev->dev_DriverData;
	stat = ior->io_Info.ioi_Recv.iob_Buffer;
	stat->ds_DriverIdentity    = DI_RAM;
	stat->ds_FamilyCode        = DS_DEVTYPE_OTHER;
	stat->ds_MaximumStatusSize = sizeof(DeviceStatus);
	stat->ds_DeviceBlockSize   = 1;
	stat->ds_DeviceBlockCount  = cs->cs_Size;
	stat->ds_DeviceBlockStart  = cs->cs_Start;
	stat->ds_DeviceUsageFlags  |= DS_USAGE_FILESYSTEM | DS_USAGE_READONLY | DS_USAGE_TRUEROM;
	ior->io_Actual = sizeof(DeviceStatus);
}

static IOReq *
IOCallBack(IOReq *slotior)
{
	IOReq *ior;
	IOReqAux *aux;

	ior = (IOReq *) slotior->io_Info.ioi_UserData;
	aux = (IOReqAux *) ior->io_Extension[1];
	ior->io_Error = slotior->io_Error;
	ior->io_Actual= slotior->io_Actual - aux->iora_Overhead;
	if (ior->io_Info.ioi_Command == CMD_STATUS)
		FinishCmdStatus(ior);
	SuperCompleteIO(ior);
	return NULL;
}

static Err
CmdStatus(IOReq* ior)
{
	IOReq *slotior;
	IOReqAux *aux;

	DBUG(("mchan: CmdStatus\n"));
	slotior = (IOReq *) ior->io_Extension[0];
	aux = (IOReqAux *) ior->io_Extension[1];
	slotior->io_Info = ior->io_Info;
	slotior->io_CallBack = IOCallBack;
	slotior->io_Info.ioi_UserData = ior;
	aux->iora_Overhead = 0;
	/* Send it to the slot driver. */
	ior->io_Flags &= ~IO_QUICK;
	SuperInternalSendIO(slotior);
	return 0;
}

static Err
CmdRead(IOReq *ior)
{
	uint32 offset;
	uint32 len;
	IOReq *slotior;
	MicroSlotSeq *pseq;
	uint8 *pcmd;
	IOReqAux *aux = (IOReqAux *) ior->io_Extension[1];
	CardState* cs = ior->io_Dev->dev_DriverData;

	offset = ior->io_Info.ioi_Offset;
	len = ior->io_Info.ioi_Recv.iob_Len;
	DBUG(("mchan: CmdRead(%x,%x)\n", offset, len));
	if (offset < 0 ||
	    len < 0 ||
	    offset + len > cs->cs_Size)
	{
		DBUG(("mchan: illegal offset/size\n"));
		ior->io_Error = BADIOARG;
		return 1;
	}

	if (offset >= 0x1000000)
	{
		DBUG(("mchan: offset too big\n"));
		ior->io_Error = BADIOARG;
		return 1;
	}

	if (ior->io_Info.ioi_Recv.iob_Len > 0x10000)
	{
		/* FIXME */
		DBUG(("mchan: len too big\n"));
		ior->io_Error = BADIOARG;
		return 1;
	}

	/* Initialize the I/O request for the MicroSlot. */
	slotior = (IOReq *) ior->io_Extension[0];

	/* Send the "ReadFromROM" command. */
	pseq = aux->iora_Seq;
	pcmd = aux->iora_ReadCmd;
	if (cs->cs_UseDownload)
	{
		/* Send a DOWNLOAD cmd, followed by 5-byte address/length. */
		pseq->uss_Cmd = USSCMD_STARTDOWNLOAD;
		pseq++;
	} else
	{
		/* Send a READ cmd, followed by 5-byte address/length. */
		*pcmd++ = SCC_READCONFIG;
	}
	*pcmd++ = offset >> 16;
	*pcmd++ = offset >> 8;
	*pcmd++ = offset;
	*pcmd++ = (len - 1) >> 8;
	*pcmd++ = (len - 1);

	pseq->uss_Cmd = USSCMD_WRITE;
	pseq->uss_Buffer = (void*) &aux->iora_ReadCmd;
	pseq->uss_Len = pcmd - aux->iora_ReadCmd;
	pseq++;
	aux->iora_Overhead = SCC_READ_LEN;

	pseq->uss_Cmd = USSCMD_READ;
	pseq->uss_Buffer = ior->io_Info.ioi_Recv.iob_Buffer;
	pseq->uss_Len = ior->io_Info.ioi_Recv.iob_Len;
	pseq++;

	pseq->uss_Cmd = 0;
	pseq++;

	slotior->io_Info.ioi_Command = USLOTCMD_SEQ;
	slotior->io_Info.ioi_Send.iob_Buffer = &aux->iora_Seq;
	slotior->io_Info.ioi_Send.iob_Len =
		(pseq - aux->iora_Seq) * sizeof(MicroSlotSeq);
	slotior->io_Info.ioi_Recv.iob_Len = 0;

	slotior->io_Info.ioi_UserData = ior;
	slotior->io_CallBack = IOCallBack;
	ior->io_Flags &= ~IO_QUICK;
	SuperInternalSendIO(slotior);
	return 0;
}


static void
MChanAbortIO(IOReq* ior)
{
	/* Tell the slot driver to abort it. */
	SuperInternalAbortIO((IOReq*)(ior->io_Extension[0]));
}

static const DriverCmdTable CmdTable[] =
{
	{ CMD_STATUS,	CmdStatus },
	{ CMD_BLOCKREAD,	CmdRead },
};
#define NUM_CMDS (sizeof(CmdTable) / sizeof(CmdTable[0]))


int32 MChanDriverMain(void)
{
	return CreateItemVA(MKNODEID(KERNELNODE,DRIVERNODE),
		TAG_ITEM_NAME,			"mchan",
		CREATEDRIVER_TAG_CMDTABLE,	CmdTable,
		CREATEDRIVER_TAG_NUMCMDS,	NUM_CMDS,
		CREATEDRIVER_TAG_CREATEDEV,	DeviceInit,
		CREATEDRIVER_TAG_DELETEDEV,	DeviceDelete,
		CREATEDRIVER_TAG_CHOWN_DEV,	ChangeDeviceOwner,
		CREATEDRIVER_TAG_ABORTIO,	MChanAbortIO,
		CREATEDRIVER_TAG_OPENDEV,	DeviceOpen,
		CREATEDRIVER_TAG_CLOSEDEV,	DeviceClose,
		CREATEDRIVER_TAG_CRIO,		CreateIO,
		CREATEDRIVER_TAG_CHOWN_IO,	ChangeIOReqOwner,
		CREATEDRIVER_TAG_DLIO,		DeleteIO,
		CREATEDRIVER_TAG_DEVICEDATASIZE,sizeof(CardState),
		CREATEDRIVER_TAG_MODULE,	FindCurrentModule(),
		TAG_END);
}
