/* @(#) ramdevice.c 96/05/17 1.63 */

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/driver.h>
#include <kernel/device.h>
#include <kernel/kernelnodes.h>
#include <kernel/list.h>
#include <kernel/listmacros.h>
#include <kernel/debug.h>
#include <kernel/kernel.h>
#include <kernel/operror.h>
#include <kernel/super.h>
#include <kernel/mem.h>
#include <kernel/sysinfo.h>
#include <dipir/hwresource.h>
#include <dipir/hw.ram.h>
#include <loader/loader3do.h>
#include <string.h>

/* #define DEBUG */

#ifdef DEBUG
#define	DBUG(x) printf x
#else
#define	DBUG(x)
#endif

typedef struct RamState
{
	uint32		rs_Permissions;
	HWResource_RAM	rs_Ram;
	List		rs_MapRecords;
} RamState;


/*****************************************************************************
*/
static void
RamDeviceAbortIO(IOReq *ior)
{
	TOUCH(ior);
	return;
}

/*****************************************************************************
*/
static int32
CmdRead(IOReq *ior)
{
	int32    offset;
	int32    len;
	RamState *ru;

	offset = ior->io_Info.ioi_Offset;
	len = ior->io_Info.ioi_Recv.iob_Len;
	ru = ior->io_Dev->dev_DriverData;
	DBUG(("Ramdev: Read: offset %x, len %x\n", offset, len));

	if (offset < 0 || len < 0 || offset + len > ru->rs_Ram.ram_Size)
	{
		ior->io_Error = BADIOARG;
		return 1;
	}

	DBUG(("   copy from %x to %x, len %x\n",
		ru->rs_Ram.ram_Addr + offset,
		ior->io_Info.ioi_Recv.iob_Buffer, len));
        if (ru->rs_Ram.ram_Flags & RAM_BYTEACCESS)
        {
                uint32 i;
                for (i = 0;  i < len;  i++)
                        ((uint8*)(ior->io_Info.ioi_Recv.iob_Buffer))[i] =
                                ((uint8*)ru->rs_Ram.ram_Addr + offset)[i];
        } else
                memcpy(ior->io_Info.ioi_Recv.iob_Buffer,
                        (uint8*)ru->rs_Ram.ram_Addr + offset, len);
	ior->io_Actual = len;
	return 1;
}

/*****************************************************************************
*/
static int32
CmdWrite(IOReq *ior)
{
	int32    offset;
	int32    len;
	RamState *ru;

	offset = ior->io_Info.ioi_Offset;
	len = ior->io_Info.ioi_Send.iob_Len;
	ru = ior->io_Dev->dev_DriverData;
	DBUG(("Ramdev: Write: offset %x, len %x\n", offset, len));

	if (offset < 0 || len < 0 || offset + len > ru->rs_Ram.ram_Size)
	{
		ior->io_Error = BADIOARG;
		return 1;
	}

	if (ru->rs_Ram.ram_Flags & RAM_READONLY)
	{
		ior->io_Error = MakeKErr(ER_SEVERE,ER_C_STND,ER_DeviceReadonly);
		return 1;
	}

	/*
	 * Only privileged code can write to a RAM device directly.
	 * Everybody else must go through the file system.
	 *
	 * If the callback is set, then the caller has enough privilege.
	 * If the callback is not set, check to see if the owner of the
	 * I/O req is privileged.
	 */
	if (ior->io_CallBack == NULL)
	{
		Task *t;

		t = (Task *) LookupItem(ior->io.n_Owner);
		if ((t->t.n_ItemFlags & ITEMNODE_PRIVILEGED) == 0)
		{
			ior->io_Error = BADPRIV;
			return 1;
		}
	}

	DBUG(("   copy from %x to %x, len %x\n",
		ior->io_Info.ioi_Send.iob_Buffer, ru->rs_Ram.ram_Addr + offset, len));
        if (ru->rs_Ram.ram_Flags & RAM_BYTEACCESS)
        {
                uint32 i;
                for (i = 0;  i < len;  i++)
                        ((uint8*)ru->rs_Ram.ram_Addr + offset)[i] =
                                ((uint8*)(ior->io_Info.ioi_Send.iob_Buffer))[i];
        } else
                memcpy((uint8 *) ru->rs_Ram.ram_Addr + offset,
                        ior->io_Info.ioi_Send.iob_Buffer, len);
	ior->io_Actual = len;
	return 1;
}


/*****************************************************************************
*/
static int32
CmdStatus(IOReq *ior)
{
	int32         len;
	DeviceStatus  stat;
	RamState      *ru;

	ru = ior->io_Dev->dev_DriverData;
	DBUG(("Ramdev: Status\n"));
	memset(&stat, 0, sizeof(stat));
	stat.ds_DriverIdentity    = DI_RAM;
	stat.ds_FamilyCode        = DS_DEVTYPE_OTHER;
	stat.ds_MaximumStatusSize = sizeof(DeviceStatus);
	stat.ds_DeviceBlockSize   = 1;
	stat.ds_DeviceBlockCount  = ru->rs_Ram.ram_Size;
	stat.ds_DeviceBlockStart  = 0;
	stat.ds_DeviceUsageFlags  = DS_USAGE_FILESYSTEM | DS_USAGE_STATIC_MAPPABLE;
	if (ru->rs_Ram.ram_Flags & RAM_READONLY)
		stat.ds_DeviceUsageFlags |= DS_USAGE_READONLY;
	if (ru->rs_Permissions & HWR_UNSIGNED_OK)
		stat.ds_DeviceUsageFlags |= DS_USAGE_UNSIGNED_OK;

	len = ior->io_Info.ioi_Recv.iob_Len;
	if (len > sizeof(stat))
		len = sizeof(stat);
	memcpy(ior->io_Info.ioi_Recv.iob_Buffer, &stat, len);
	ior->io_Actual = len;
	return 1;
}

/*****************************************************************************
*/
static int32
CmdGetMapInfo(IOReq *ior)
{
	int32 len;
	RamState *ru;
	MemMappableDeviceInfo info;

	ru = ior->io_Dev->dev_DriverData;
	memset(&info, 0, sizeof(info));
	info.mmdi_Flags =
		MM_MAPPABLE | MM_READABLE | MM_EXECUTABLE | MM_EXCLUSIVE;
	if ((ru->rs_Ram.ram_Flags & RAM_READONLY) == 0)
		info.mmdi_Flags |= MM_WRITABLE;
	if (ru->rs_Permissions & HWR_XIP_OK)
		info.mmdi_Permissions |= XIP_OK;
	info.mmdi_SpeedPenaltyRatio = 0;
	info.mmdi_MaxMappableBlocks = ru->rs_Ram.ram_Size;
	info.mmdi_CurBlocksMapped = BytesMemMapped(&ru->rs_MapRecords);

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
	RamState *ru;
	int32 offset;
	int32 err;
	MapRangeRequest *req;
	MapRangeResponse resp;

	if (ior->io_Info.ioi_Send.iob_Len < sizeof(MapRangeRequest))
	{
		ior->io_Error = BADIOARG;
		return 1;
	}

	ru = ior->io_Dev->dev_DriverData;
	req = (MapRangeRequest *) ior->io_Info.ioi_Send.iob_Buffer;
	offset = ior->io_Info.ioi_Offset;
	DBUG(("Ramdev: Map: offset %x, size %x\n", offset, req->mrr_BytesToMap));

	if (offset < 0 ||
	    req->mrr_BytesToMap < 0 ||
	    offset + req->mrr_BytesToMap > ru->rs_Ram.ram_Size)
	{
		DBUG(("Ramdev: bad range\n"));
		ior->io_Error = BADIOARG;
		return 1;
	}

	err = CreateMemMapRecord(&ru->rs_MapRecords, ior);
	if (err < 0)
	{
		DBUG(("Ramdev: CreateMMR failed\n"));
		ior->io_Error = err;
		return 1;
	}
	memset(&resp, 0, sizeof(resp));
	resp.mrr_Flags = MM_MAPPABLE | MM_READABLE | MM_EXECUTABLE;
	if ((ru->rs_Ram.ram_Flags & RAM_READONLY) == 0)
		resp.mrr_Flags |= MM_WRITABLE;
	resp.mrr_Flags &= req->mrr_Flags;
	resp.mrr_MappedArea = (uint8 *) ru->rs_Ram.ram_Addr + offset;

	len = ior->io_Info.ioi_Recv.iob_Len;
	if (len > sizeof(resp))
		len = sizeof(resp);
	memcpy(ior->io_Info.ioi_Recv.iob_Buffer, &resp, len);
	ior->io_Actual = len;
	DBUG(("Ramdev: maprange ok\n"));
	return 1;
}

/*****************************************************************************
*/
static int32
CmdUnmapRange(IOReq *ior)
{
	int32 err;
	RamState *ru;

	ru = ior->io_Dev->dev_DriverData;
	err = DeleteMemMapRecord(&ru->rs_MapRecords, ior);
	if (err < 0)
	{
		ior->io_Error = err;
		return 1;
	}
	ior->io_Actual = 0;
	return 1;
}

/*****************************************************************************
*/
static int32
CmdPreferFSType(IOReq *ior)
{
	RamState *ru;
	uint32 fstype;

	ru = ior->io_Dev->dev_DriverData;
	fstype = ru->rs_Ram.ram_FSType;
	if (ior->io_Info.ioi_Recv.iob_Len < sizeof(fstype))
	{
		ior->io_Error = MakeKErr(ER_SEVER,ER_C_STND,ER_IOIncomplete);
		return 1;
	}
	memcpy(ior->io_Info.ioi_Recv.iob_Buffer, &fstype, sizeof(fstype));
	ior->io_Actual = sizeof(fstype);
	return 1;
}

/*****************************************************************************
*/
static Item
RamDeviceCreate(Device *dev)
{
	HWResource hwr;
	RamState *ru;
	Err err;

	err = FindHWResource(dev->dev_HWResource, &hwr);
	if (err < 0)
		return err;
	if (!MatchDeviceName("RAM", hwr.hwr_Name, DEVNAME_TYPE))
		return MAKEKERR(ER_SEVER,ER_C_STND,ER_NoHardware);

	DBUG(("Ram dev %s\n", hwr.hwr_Name));
	ru = dev->dev_DriverData;
	ru->rs_Permissions = hwr.hwr_Perms;
	InitList(&ru->rs_MapRecords, "RAM mem maps");
	memcpy(&ru->rs_Ram, &hwr.hwr_DeviceSpecific, sizeof(ru->rs_Ram));
	if (hwr.hwr_Perms & HWR_WRITE_PROTECT)
		ru->rs_Ram.ram_Flags |= RAM_READONLY;
	return dev->dev.n_Item;
}

/*****************************************************************************
*/
static Err
RamDeviceDelete(Device *dev)
{
	RamState *ru;

	ru = dev->dev_DriverData;
	DeleteAllMemMapRecords(&ru->rs_MapRecords);
	return 0;
}

/*****************************************************************************
*/
static DriverCmdTable CmdTable[] =
{
	CMD_BLOCKWRITE,	CmdWrite,
	CMD_BLOCKREAD,	CmdRead,
	CMD_STATUS,	CmdStatus,
	CMD_GETMAPINFO,	CmdGetMapInfo,
	CMD_MAPRANGE,	CmdMapRange,
	CMD_UNMAPRANGE,	CmdUnmapRange,
	CMD_PREFER_FSTYPE, CmdPreferFSType,
};


/*****************************************************************************
*/
int32
main(void)
{
	return CreateItemVA(MKNODEID(KERNELNODE,DRIVERNODE),
			TAG_ITEM_NAME,             "ram",
			TAG_ITEM_PRI,              1,
			CREATEDRIVER_TAG_CMDTABLE, CmdTable,
			CREATEDRIVER_TAG_NUMCMDS,  sizeof(CmdTable) / sizeof(CmdTable[0]),
			CREATEDRIVER_TAG_ABORTIO,  RamDeviceAbortIO,
			CREATEDRIVER_TAG_CREATEDEV,RamDeviceCreate,
			CREATEDRIVER_TAG_DELETEDEV,RamDeviceDelete,
			CREATEDRIVER_TAG_DEVICEDATASIZE,sizeof(RamState),
			CREATEDRIVER_TAG_MODULE,   FindCurrentModule(),
			TAG_END);
}
