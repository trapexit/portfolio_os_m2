/*
 *	@(#) hostcdcmd.c 96/08/22 1.4
 *	Copyright 1996, The 3DO Company
 *
 * Commands for the "hostcd" device.
 */

#include "kernel/types.h"
#include "hardware/PPCasm.h"
#include "hardware/debugger.h"
#include "file/directory.h"
#include "dipir.h"
#include "notsysrom.h"
#include "hostcd.h"


extern const DipirRoutines *dipr;

#define	SENDHOSTCMD(dev,cmd)	SendHostCmd(dev, (HostFSReq*)(cmd))
#define	GETHOSTREPLY(dev,reply)	GetHostReply(dev, (HostFSReply*)(reply))

/*****************************************************************************
 Dismount a host CD.
*/
	void
DismountHostCD(DipirHWResource *dev, void *refToken)
{
	HostCDReq cmd;
	HostCDReply reply;

	cmd.hcd_Command = HOSTCD_REMOTECMD_DISMOUNT;
	cmd.hcd_Unit = HOST_CD_UNIT;
	cmd.hcd_ReferenceToken = refToken;

        {
        /* FIXME: until debugger doesn't need this */
	cmd.hcd_Send.iob_Buffer = "cdrom.image";
	cmd.hcd_Send.iob_Len    = 12;
        }

	SENDHOSTCMD(dev, &cmd);
	GETHOSTREPLY(dev, &reply);
}

/*****************************************************************************
 Mount a host filesystem.
*/
	int32
MountHostCD(DipirHWResource *dev, uint32 unit, 
		void **pRefToken, uint32 *numBlocks, uint32 *blockSize)
{
	HostCDReq cmd;
	HostCDReply reply;

	cmd.hcd_Command = HOSTCD_REMOTECMD_MOUNT;
	cmd.hcd_Offset = unit;
	cmd.hcd_Unit = HOST_CD_UNIT;
	cmd.hcd_Send.iob_Buffer = NULL;
	cmd.hcd_Recv.iob_Buffer = NULL;
	SENDHOSTCMD(dev, &cmd);
	GETHOSTREPLY(dev, &reply);
	if (reply.hcdr_Error < 0)
		return reply.hcdr_Error;

        *pRefToken  = reply.hcdr_ReferenceToken;
	*numBlocks = reply.hcdr_Actual;
	*blockSize = reply.hcdr_BlockSize;

        if (*blockSize == 0)
	{
		/* FIXME: debugger doesn't implement the new protocol yet... */
		*pRefToken  = NULL;
		*numBlocks = 4096;
		*blockSize = 2048;
	}

	return 0;
}

	void *
ReadHostCDAsync(DipirHWResource *dev, void *refToken, 
		uint32 block, uint32 len, void *buffer)
{
	HostCDReq cmd;

	cmd.hcd_Command = HOSTCD_REMOTECMD_BLOCKREAD;
	cmd.hcd_Offset = block;
	cmd.hcd_ReferenceToken = refToken;
	cmd.hcd_Send.iob_Buffer = NULL;
	cmd.hcd_Recv.iob_Buffer = buffer;
	cmd.hcd_Recv.iob_Len = len;
	cmd.hcd_UserData = buffer;
	cmd.hcd_Unit = HOST_CD_UNIT;
	SENDHOSTCMD(dev, &cmd);
	return (void *) cmd.hcd_UserData;
}

	int32
WaitReadHostCD(DipirHWResource *dev, void *id)
{
	HostCDReply reply;

	if (id == 0)
	{
		EPRINTF(("HostCD: no read in progress\n"));
		return -1;
	}
	GETHOSTREPLY(dev, &reply);
	if (reply.hcdr_UserData != id)
	{
		EPRINTF(("HostCD: read sync error (%x != %x)\n",
			reply.hcdr_UserData, id));
		return -1;
	}
	if (reply.hcdr_Error < 0)
	{
		EPRINTF(("HostCD: device error %x\n", reply.hcdr_Error));
		return reply.hcdr_Error;
	}
	return reply.hcdr_Actual;
}
