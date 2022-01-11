/* @(#) nulldrvr.c 96/05/17 1.9 */

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/driver.h>
#include <kernel/device.h>
#include <kernel/debug.h>
#include <kernel/operror.h>
#include <kernel/super.h>
#include <loader/loader3do.h>
#include <string.h>
#include <stdio.h>

#define DBUG(x) /* printf x */

int32 DoIt(IOReq *ior)
{
	TOUCH(ior);
	return 1;
}

void NullFunc(IOReq *ior)
{
	TOUCH(ior);
}


static const DriverCmdTable CmdTable[] =
{
	CMD_STATUS,	    DoIt,
};

Item CreateDev(Device *dev)
{
	return dev->dev.n_Item;
}

int32 main(void)
{
	return CreateItemVA(MKNODEID(KERNELNODE,DRIVERNODE),
		TAG_ITEM_NAME,              "null",
		TAG_ITEM_PRI,               1,
		CREATEDRIVER_TAG_CMDTABLE,  CmdTable,
		CREATEDRIVER_TAG_NUMCMDS,   sizeof(CmdTable) / sizeof(DriverCmdTable),
		CREATEDRIVER_TAG_CREATEDEV, CreateDev,
		CREATEDRIVER_TAG_ABORTIO,   NullFunc,
		CREATEDRIVER_TAG_MODULE,    FindCurrentModule(),
		TAG_END);
}
