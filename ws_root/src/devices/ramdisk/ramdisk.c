/* @(#) ramdisk.c 96/09/24 1.11 */

#include <kernel/io.h>
#include <kernel/driver.h>
#include <kernel/device.h>
#include <kernel/mem.h>
#include <kernel/debug.h>
#include <kernel/operror.h>
#include <kernel/super.h>
#include <kernel/kernel.h>
#include <loader/loader3do.h>
#include <string.h>
#include <stdio.h>

#undef DEBUG

#ifdef DEBUG
#define	DBUG(x)		printf x
#define	DumpBytes(p,n)	_DumpBytes(p,n)
#else
#define	DBUG(x)
#define	DumpBytes(p,n)
#endif

/*****************************************************************************/

uint32 ramdiskSize = 128 * 1024;
uint8 *diskData;


/*****************************************************************************/

#ifdef DEBUG
static void DumpBytes(void *buf, uint32 len)
{
	uint32 i;
	uint8 *p = buf;

	printf("Dump %x bytes (addr %x)\n", len, buf);
	for (i = 0;  i < len;  i++)
	{
		if ((i % 16) == 0)
			printf("\n%6x  ", i);
		printf("%02x ", p[i]);
		if (i > 256)
		{
			printf(" ... ");
			break;
		}
	}
	printf(" --end\n");
}
#endif /* DEBUG */

/*****************************************************************************/



static void AbortRamDiskIO(IOReq *ior)
{
    TOUCH(ior);

    /* nothing to abort, the thing always works synchronously */
}


/*****************************************************************************/


static int32 CmdStatus(IOReq *ior)
{
DeviceStatus stat;
int32        len;

    memset(&stat,0,sizeof(stat));
    stat.ds_DriverIdentity    = DI_RAM;
    stat.ds_MaximumStatusSize = sizeof(DeviceStatus);
    stat.ds_FamilyCode        = DS_DEVTYPE_OTHER;
    stat.ds_DeviceUsageFlags  = DS_USAGE_FILESYSTEM;
    stat.ds_DeviceBlockSize   = 1;
    stat.ds_DeviceBlockCount  = ramdiskSize;

    if (diskData == NULL)
	stat.ds_DeviceUsageFlags |= DS_USAGE_OFFLINE;

    len = ior->io_Info.ioi_Recv.iob_Len;
    if (len > sizeof(stat))
        len = sizeof(stat);

    memcpy(ior->io_Info.ioi_Recv.iob_Buffer,&stat,len);

    ior->io_Actual = len;

    return 1;
}


/*****************************************************************************/


#ifdef INDUCE_MEDIA_ERRORS
uint32 mrand(void)
{
	static int next = 1;
	next = next * 1103515245 + 12345;
	return next >> 4;
}
#endif


/*****************************************************************************/


static int32 CmdBlockWrite(IOReq *ior)
{
int32  offset;
int32  len;

    if (diskData == NULL)
    {
	diskData = SuperAllocMem(ramdiskSize, MEMTYPE_NORMAL|MEMTYPE_FILL);
	if (diskData == NULL)
	{
	    ior->io_Error = NOMEM;
	    return 1;
	}
#ifdef BUILD_STRINGS
	printf("ramdisk: data at %x\n", diskData);
#endif
    }

    offset = ior->io_Info.ioi_Offset;
    len    = ior->io_Info.ioi_Send.iob_Len;

#ifdef INDUCE_MEDIA_ERRORS
    {
	if (offset > 0x400 && (mrand() % 4) == 0)
	{
	    ior->io_Error = MakeErr(ER_DEVC, MakeErrId('r','d'), ER_SEVER,
				ER_E_SSTM, ER_C_STND, ER_MediaError);
	    return 1;
	}
    }
#endif

    DBUG(("ramdisk: write %x bytes at %x\n", len, offset));

    if ((offset < 0) || (len < 0) || (offset + len > ramdiskSize))
    {
        ior->io_Error = BADIOARG;
        return 1;
    }

    memcpy(&diskData[offset], ior->io_Info.ioi_Send.iob_Buffer, len);
    DumpBytes(&diskData[offset], len);

    ior->io_Actual = len;
    return 1;
}


/*****************************************************************************/


static int32 CmdBlockRead(IOReq *ior)
{
int32  offset;
int32  len;

    if (diskData == NULL)
    {
	ior->io_Error = MakeKErr(ER_SEVERE,ER_C_STND,ER_EndOfMedium);
	return 1;
    }

    offset = ior->io_Info.ioi_Offset;
    len    = ior->io_Info.ioi_Recv.iob_Len;

    DBUG(("ramdisk: read %x bytes at %x\n", len, offset));

    if ((offset < 0) || (len < 0) || (offset + len > ramdiskSize))
    {
        ior->io_Error = BADIOARG;
        return 1;
    }

    memcpy(ior->io_Info.ioi_Recv.iob_Buffer, &diskData[offset], len);
    DumpBytes(&diskData[offset], len);

    ior->io_Actual = len;
    return 1;
}


/*****************************************************************************/


static Err DeleteRamDiskDriver(Driver *drv)
{
    DBUG(("ramdisk: Delete drv %x, %d opens\n", drv, drv->drv.n_OpenCount));
    TOUCH(drv);
    SuperFreeMem(diskData, ramdiskSize);
    return 0;
}


/*****************************************************************************/


static DriverCmdTable cmdTable[] =
{
    {CMD_BLOCKWRITE, CmdBlockWrite},
    {CMD_BLOCKREAD,  CmdBlockRead},
    {CMD_STATUS,     CmdStatus},
};
#define	NUM_CMDS	(sizeof(cmdTable) / sizeof(cmdTable[0]))


int32 main(void)
{
Item driver;

    driver = CreateItemVA(MKNODEID(KERNELNODE,DRIVERNODE),
		TAG_ITEM_NAME,			"ramdisk",
		CREATEDRIVER_TAG_NUMCMDS,	NUM_CMDS,
		CREATEDRIVER_TAG_CMDTABLE,	cmdTable,
		CREATEDRIVER_TAG_ABORTIO,	AbortRamDiskIO,
		CREATEDRIVER_TAG_DELETEDRV,	DeleteRamDiskDriver,
		CREATEDRIVER_TAG_MODULE,	FindCurrentModule(),
		TAG_END);
    if (driver >= 0)
        return OpenItemAsTask(driver, NULL, KernelBase->kb_OperatorTask);

    return driver;
}
