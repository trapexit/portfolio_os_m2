/* @(#) chan.c 96/09/06 1.34 */

/*
 * Channel device driver.
 * This driver talks to the dipir-level Channel Drivers to access a device ROM.
 */

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
#include <dipir/hwresource.h>
#include <loader/loader3do.h>
#include <hardware/PPCasm.h>
#include <string.h>

/*
 * Define PCHANDEVICE if the pchan device exists to handle
 * system ROM and PCMCIA ROMs.
 * In that case, chandevice won't try to handle them.
 */
#define PCHANDEVICE 1

/*
 * Define MCHANDEVICE if the mchan device exists to handle
 * Microcard ROMs.
 * In that case, chandevice won't try to handle them.
 */
#define	MCHANDEVICE 1

/*#define DEBUG */

#ifdef DEBUG
#define	DBUG(x) printf x
#else
#define	DBUG(x)
#endif

#ifdef BUILD_STRINGS
#define	ERROR(x) printf x
#else
#define	ERROR(x)
#endif

typedef struct ChanState
{
	HardwareID	cu_ID;		/* ID of ROM-level channel driver */
	uint32		cu_Start;
	uint32		cu_Size;
	uint32		cu_Permissions;
	List		cu_MapRecords;
} ChanState;


/*****************************************************************************
*/
static void
ChanDeviceAbortIO(IOReq *ior)
{
	TOUCH(ior);
	return;
}

/*****************************************************************************
*/
static int32
CmdBlockRead(IOReq *ior)
{
	ChanState *cu;
	int32 offset;
	int32 len;
	int32 r;

	offset = ior->io_Info.ioi_Offset;
	len = ior->io_Info.ioi_Recv.iob_Len;
	cu = ior->io_Dev->dev_DriverData;
	DBUG(("Chandev: Read: offset %x, len %x\n", offset, len));

	if (offset < 0 || len < 0 || offset + len > cu->cu_Size)
	{
		DBUG(("ChanRead:BADIOARG\n"));
		ior->io_Error = BADIOARG;
		return 1;
	}
	r = ChannelRead(cu->cu_ID, offset, len,
			ior->io_Info.ioi_Recv.iob_Buffer);
	if (r < 0)
		ior->io_Error = MakeKErr(ER_SEVER,ER_C_STND,ER_DeviceError);
	else
		ior->io_Actual = r;
	return 1;
}

/*****************************************************************************
*/
static int32
CmdStatus(IOReq *ior)
{
	ChanState *cu;
	uint32 len;
	DeviceStatus stat;

	cu = ior->io_Dev->dev_DriverData;
	DBUG(("Chandev: Status\n"));
	memset(&stat, 0, sizeof(stat));
	stat.ds_DriverIdentity = DI_RAM;
	stat.ds_FamilyCode = DS_DEVTYPE_OTHER;
	stat.ds_MaximumStatusSize = sizeof(DeviceStatus);
	stat.ds_DeviceBlockSize = 1;
	stat.ds_DeviceBlockCount = cu->cu_Size;
	stat.ds_DeviceBlockStart = cu->cu_Start;
	stat.ds_DeviceUsageFlags =
	    DS_USAGE_FILESYSTEM | DS_USAGE_READONLY | DS_USAGE_STATIC_MAPPABLE | DS_USAGE_TRUEROM;
	if (cu->cu_Permissions & HWR_UNSIGNED_OK)
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
	ChanState *cu;
	MemMappableDeviceInfo info;

	DBUG(("Chandev: get map info\n"));
	cu = ior->io_Dev->dev_DriverData;
	memset(&info, 0, sizeof(info));
	info.mmdi_Flags =
		MM_MAPPABLE | MM_READABLE | MM_EXECUTABLE | MM_EXCLUSIVE;
	if (cu->cu_Permissions & HWR_XIP_OK)
		info.mmdi_Permissions |= XIP_OK;
	info.mmdi_SpeedPenaltyRatio = 0;
	info.mmdi_MaxMappableBlocks = cu->cu_Size;
	info.mmdi_CurBlocksMapped = BytesMemMapped(&cu->cu_MapRecords);

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
	ChanState *cu;
	int32 offset;
	int32 err;
	void *addr;
	MapRangeRequest *req;
	MapRangeResponse resp;

	if (ior->io_Info.ioi_Send.iob_Len < sizeof(MapRangeRequest))
	{
		ior->io_Error = BADIOARG;
		return 1;
	}

	cu = ior->io_Dev->dev_DriverData;
	req = (MapRangeRequest *) ior->io_Info.ioi_Send.iob_Buffer;
	offset = ior->io_Info.ioi_Offset;
	DBUG(("Chandev: Map: offset %x, size %x\n", offset, req->mrr_BytesToMap));

	if (offset < 0 ||
	    req->mrr_BytesToMap < 0 ||
	    offset + req->mrr_BytesToMap > cu->cu_Size)
	{
		DBUG(("Chandev: bad range\n"));
		ior->io_Error = BADIOARG;
		return 1;
	}

	if (ChannelMap(cu->cu_ID, offset, req->mrr_BytesToMap, &addr) < 0)
	{
		DBUG(("Chandev: channel driver failed\n"));
		ior->io_Error = MakeKErr(ER_SEVER,ER_C_STND,ER_DeviceError);
		return 1;
	}
	err = CreateMemMapRecord(&cu->cu_MapRecords, ior);
	if (err < 0)
	{
		DBUG(("Chandev: CreateMMR failed\n"));
		(void) ChannelUnmap(cu->cu_ID, offset, req->mrr_BytesToMap);
		ior->io_Error = err;
		return 1;
	}
	memset(&resp, 0, sizeof(resp));
	resp.mrr_Flags = MM_MAPPABLE | MM_READABLE | MM_EXECUTABLE | MM_EXCLUSIVE;
	resp.mrr_Flags &= req->mrr_Flags;
	resp.mrr_MappedArea = addr;

	len = ior->io_Info.ioi_Recv.iob_Len;
	if (len > sizeof(resp))
		len = sizeof(resp);
	memcpy(ior->io_Info.ioi_Recv.iob_Buffer, &resp, len);
	ior->io_Actual = len;
	DBUG(("Chandev: maprange ok\n"));
	return 1;
}

/*****************************************************************************
*/
static int32
CmdUnmapRange(IOReq *ior)
{
	int32 err;
	ChanState *cu;
	uint32 offset;
	MapRangeRequest *req;

	if (ior->io_Info.ioi_Send.iob_Len < sizeof(MapRangeRequest))
	{
		ior->io_Error = BADIOARG;
		return 1;
	}

	cu = ior->io_Dev->dev_DriverData;
	req = (MapRangeRequest *) ior->io_Info.ioi_Send.iob_Buffer;
	offset = ior->io_Info.ioi_Offset;
	DBUG(("Chandev: Unmap: offset %x, size %x\n", offset, req->mrr_BytesToMap));

	err = DeleteMemMapRecord(&cu->cu_MapRecords, ior);
	if (err < 0)
	{
		ior->io_Error = err;
		return 1;
	}
	if (ChannelUnmap(cu->cu_ID, offset, req->mrr_BytesToMap) < 0)
	{
		ior->io_Error = MakeKErr(ER_SEVER,ER_C_STND,ER_DeviceError);
		return 1;
	}
	ior->io_Actual = 0;
	return 1;
}

/*****************************************************************************
*/
static Item
ChanDeviceCreate(Device *dev)
{
	HWResource hwr;
	ChanState *cu;
	Err err;

	err = FindHWResource(dev->dev_HWResource, &hwr);
	if (err < 0)
		return err;
	if (hwr.hwr_ROMSize == 0)
		return MAKEKERR(ER_SEVER,ER_C_STND,ER_NoHardware);
#ifdef PCHANDEVICE
	if (hwr.hwr_Channel == CHANNEL_SYSMEM ||
	    hwr.hwr_Channel == CHANNEL_PCMCIA)
		/* pchandevice does these. */
		return MAKEKERR(ER_SEVER,ER_C_STND,ER_NoHardware);
#endif
#ifdef MCHANDEVICE
	if (hwr.hwr_Channel == CHANNEL_MICROCARD)
		/* mchandevice does these. */
		return MAKEKERR(ER_SEVER,ER_C_STND,ER_NoHardware);
#endif
	DBUG(("Chan dev %s\n", hwr.hwr_Name));
	cu = dev->dev_DriverData;
	cu->cu_ID = hwr.hwr_InsertID;
	cu->cu_Size = hwr.hwr_ROMSize;
	cu->cu_Start = hwr.hwr_ROMUserStart;
	cu->cu_Permissions = hwr.hwr_Perms;
	InitList(&cu->cu_MapRecords, "Chandev mem maps");
	return dev->dev.n_Item;
}

/*****************************************************************************
*/
static Err
ChanDeviceDelete(Device *dev)
{
	ChanState *cu;

	cu = dev->dev_DriverData;
	DeleteAllMemMapRecords(&cu->cu_MapRecords);
	return 0;
}

/*****************************************************************************
*/
static DriverCmdTable CmdTable[] =
{
	CMD_BLOCKREAD,	CmdBlockRead,
	CMD_STATUS,	CmdStatus,
	CMD_GETMAPINFO, CmdGetMapInfo,
	CMD_MAPRANGE,   CmdMapRange,
	CMD_UNMAPRANGE, CmdUnmapRange,
};
#define	NUM_CMDS	(sizeof(CmdTable) / sizeof(CmdTable[0]))

/*****************************************************************************
*/
Item
ChanDriverMain(void)
{
	return CreateItemVA(MKNODEID(KERNELNODE,DRIVERNODE),
		TAG_ITEM_NAME,			"chan",
		CREATEDRIVER_TAG_CMDTABLE,	CmdTable,
		CREATEDRIVER_TAG_NUMCMDS,	NUM_CMDS,
		CREATEDRIVER_TAG_ABORTIO,	ChanDeviceAbortIO,
		CREATEDRIVER_TAG_CREATEDEV,	ChanDeviceCreate,
		CREATEDRIVER_TAG_DELETEDEV,	ChanDeviceDelete,
		CREATEDRIVER_TAG_DEVICEDATASIZE,sizeof(ChanState),
		CREATEDRIVER_TAG_MODULE,	FindCurrentModule(),
		TAG_END);
}

