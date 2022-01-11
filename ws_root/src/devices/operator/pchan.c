/*
 * @(#) pchan.c 96/09/06 1.16
 *
 * High-performance device driver which handles PCMCIA ROM (and system ROM).
 * It is functionally equivalent to the chandevice driver for these
 * devices, but uses the PCMCIA driver to do its operations via
 * high-speed DMA.
 */

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/driver.h>
#include <kernel/device.h>
#include <kernel/kernelnodes.h>
#include <kernel/interrupts.h>
#include <kernel/list.h>
#include <kernel/listmacros.h>
#include <kernel/debug.h>
#include <kernel/kernel.h>
#include <kernel/operror.h>
#include <kernel/super.h>
#include <kernel/mem.h>
#include <kernel/sysinfo.h>
#include <hardware/cde.h>
#include <hardware/visa.h>
#include <dipir/hwresource.h>
#include <dipir/hw.ram.h>
#include <loader/loader3do.h>
#include <string.h>

/*#define DEBUG */

#ifdef DEBUG
#define	DBUG(x) printf x
#else
#define	DBUG(x)
#endif


typedef struct PState
{
	Item		mu_SlotDev;
	uint32		mu_Start;
	uint32		mu_Size;
} PState;

/*
 * Use the io_Extension[0] field of an IOReq to point to the
 * slot driver's IOReq which is associated with it.
 */
#define	io_SlotIOReq	io_Extension[0]

/*****************************************************************************
  Abort an IO operation.
*/
static void
DeviceAbortIO(IOReq *ior)
{
	/* Tell the slot driver to abort it. */
	SuperInternalAbortIO((IOReq*)ior->io_SlotIOReq);
}

/*****************************************************************************
  Patch up a DeviceStatus structure from the slot device
  to have the right information about our device.
*/
static void
FinishCmdStatus(IOReq *ior)
{
	DeviceStatus *stat;
	PState *mu;

	mu = ior->io_Dev->dev_DriverData;
	stat = ior->io_Info.ioi_Recv.iob_Buffer;
	stat->ds_DriverIdentity    = DI_RAM;
	stat->ds_FamilyCode        = DS_DEVTYPE_OTHER;
	stat->ds_MaximumStatusSize = sizeof(DeviceStatus);
	stat->ds_DeviceBlockSize   = 1;
	stat->ds_DeviceBlockCount  = mu->mu_Size;
	stat->ds_DeviceBlockStart  = mu->mu_Start;
	stat->ds_DeviceUsageFlags  |= DS_USAGE_FILESYSTEM | DS_USAGE_READONLY | DS_USAGE_TRUEROM;
	ior->io_Actual = sizeof(DeviceStatus);
}

/*****************************************************************************
  Callback on I/O completion.
*/
static IOReq *
IOCallBack(IOReq *slotior)
{
	IOReq *ior;

	ior = (IOReq *) slotior->io_Info.ioi_UserData;
	ior->io_Error = slotior->io_Error;
	ior->io_Actual = slotior->io_Actual;
	switch (ior->io_Info.ioi_Command)
	{
	case CMD_STATUS:
		FinishCmdStatus(ior);
		break;
	}
	SuperCompleteIO(ior);
	return NULL;
}

/*****************************************************************************
  Schedule a device command.
  Commands are basically just passed down to the PCMCIA slot driver.
*/
static int32
CmdSched(IOReq *ior)
{
	IOReq *slotior;
	PState *mu;

	mu = ior->io_Dev->dev_DriverData;
	/* Check arguments. */
	switch (ior->io_Info.ioi_Command)
	{
	case CMD_BLOCKREAD:
		{
			uint32 offset = ior->io_Info.ioi_Offset;
			uint32 len = ior->io_Info.ioi_Recv.iob_Len;
			if (offset >= VISA_PCMCIA_ADDR)
			{
				ior->io_Error = BADIOARG;
				return 1;
			}
			if (offset < 0 ||
			    len < 0 ||
			    offset + len > mu->mu_Size)
			{
				ior->io_Error = BADIOARG;
				return 1;
			}
			break;
		}
	}

	ior->io_Flags &= ~IO_QUICK;
	/* Construct the IOReq for the slot driver. */
	slotior = (IOReq *) ior->io_SlotIOReq;
	slotior->io_Info = ior->io_Info;
	slotior->io_CallBack = IOCallBack;
	slotior->io_Info.ioi_UserData = ior;
	/* Send it to the slot driver. */
	SuperInternalSendIO(slotior);
	return 0;
}


/*****************************************************************************
*/
static Err
CreateIO(IOReq *ior)
{
	PState *mu;
	Item slotItem;
	IOReq *slotior;

	mu = ior->io_Dev->dev_DriverData;
	slotItem = CreateIOReq(0, 0, mu->mu_SlotDev, 0);
	if (slotItem < 0)
	{
		DBUG(("pchan.CreateIO: cannot create slot IOR %x\n", slotItem));
		return slotItem;
	}
	slotior = (IOReq *) LookupItem(slotItem);
	if (slotior == NULL)
	{
		DBUG(("pchan.CreateIO: cannot lookup slot IOR %x\n", slotItem));
		return MakeKErr(ER_SEVERE,ER_C_STND,ER_SoftErr);
	}
	ior->io_SlotIOReq = (int32) slotior;
	return 0;
}

/*****************************************************************************
*/
static void
DeleteIO(IOReq *ior)
{
	IOReq * slotior;

	slotior = (IOReq *) ior->io_SlotIOReq;
	if (slotior != NULL)
		DeleteItem(slotior->io.n_Item);
}

/*****************************************************************************
*/
static Err
ChangeIOReqOwner(IOReq *ior, Item newOwner)
{
	IOReq *slotior;

	slotior = (IOReq *) ior->io_SlotIOReq;
	if (slotior == NULL)
		return BADITEM;
	return SuperSetItemOwner(slotior->io.n_Item, newOwner);
}

/*****************************************************************************
*/
static Item
DeviceOpen(Device *dev, Task *t)
{
	PState *mu;
	Item err;

	mu = dev->dev_DriverData;
	err = SuperInternalOpenItem(mu->mu_SlotDev, 0, t);
	if (err < 0)
	{
		return err;
	}
	return dev->dev.n_Item;
}

/*****************************************************************************
*/
static Err
DeviceClose(Device *dev, Task *t)
{
	PState *mu;

	mu = dev->dev_DriverData;
	SuperInternalCloseItem(mu->mu_SlotDev, t);
	return 0;
}

/*****************************************************************************
*/
static Item
DeviceInit(Device *dev)
{
	HWResource hwr;
	Item slotdev;
	PState *mu;
	Err err;

	err = FindHWResource(dev->dev_HWResource, &hwr);
	DBUG(("pchan: find hw %x ret %x\n", dev->dev_HWResource, err));
	if (err < 0)
		return err;
	if (hwr.hwr_Channel != CHANNEL_PCMCIA)
		return MAKEKERR(ER_SEVER,ER_C_STND,ER_NoHardware);
	if (hwr.hwr_Slot == 0x7F)
		return MAKEKERR(ER_SEVER,ER_C_STND,ER_NoHardware);

	slotdev = OpenSlotDevice(dev->dev_HWResource);
	DBUG(("pchan: OpenSlotDev hw %x ret %x\n", dev->dev_HWResource, slotdev));
	if (slotdev < 0)
		return slotdev;

	DBUG(("pchan dev %s: start %x, size %x\n",
			hwr.hwr_Name,
			hwr.hwr_ROMUserStart, hwr.hwr_ROMSize));
	mu = dev->dev_DriverData;
	mu->mu_SlotDev = slotdev;
	mu->mu_Start = hwr.hwr_ROMUserStart;
	mu->mu_Size = hwr.hwr_ROMSize;
	return dev->dev.n_Item;
}

/*****************************************************************************
*/

static Err
DeviceDelete(Device *dev)
{
    PState* mu;
    uint8 oldPriv;

    mu = dev->dev_DriverData;
    oldPriv = PromotePriv(CURRENTTASK);
    CloseItemAsTask(mu->mu_SlotDev, dev->dev.n_Owner);
    DemotePriv(CURRENTTASK, oldPriv);
    return 0;
}

/*****************************************************************************
*/

static Err
ChangeDeviceOwner(Device *dev, Item newOwner)
{
	PState* mu;
	Err err;

	/* Transfer the "openness" of the slot device to the new owner. */
	mu = dev->dev_DriverData;
	err = OpenItemAsTask(mu->mu_SlotDev, NULL, newOwner);
	if (err < 0)
		return err;
	CloseItem(mu->mu_SlotDev);
	return 0;
}

/*****************************************************************************
*/
static DriverCmdTable CmdTable[] =
{
	CMD_BLOCKREAD,	CmdSched,
	CMD_STATUS,	CmdSched,
	CMD_GETICON,	CmdSched,
};
#define	NUM_CMDS	(sizeof(CmdTable) / sizeof(CmdTable[0]))


/*****************************************************************************
*/
Item
PChanDriverMain(void)
{
	return CreateItemVA(MKNODEID(KERNELNODE,DRIVERNODE),
		TAG_ITEM_NAME,			"pchan",
		CREATEDRIVER_TAG_CMDTABLE,	CmdTable,
		CREATEDRIVER_TAG_NUMCMDS,	NUM_CMDS,
		CREATEDRIVER_TAG_ABORTIO,	DeviceAbortIO,
		CREATEDRIVER_TAG_CREATEDEV,	DeviceInit,
		CREATEDRIVER_TAG_DELETEDEV,	DeviceDelete,
		CREATEDRIVER_TAG_CHOWN_DEV,	ChangeDeviceOwner,
		CREATEDRIVER_TAG_OPENDEV,	DeviceOpen,
		CREATEDRIVER_TAG_CLOSEDEV,	DeviceClose,
		CREATEDRIVER_TAG_CRIO,		CreateIO,
		CREATEDRIVER_TAG_DLIO,		DeleteIO,
		CREATEDRIVER_TAG_CHOWN_IO,	ChangeIOReqOwner,
		CREATEDRIVER_TAG_DEVICEDATASIZE,sizeof(PState),
		CREATEDRIVER_TAG_MODULE,	FindCurrentModule(),
		TAG_END);
}

