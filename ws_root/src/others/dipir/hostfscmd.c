#include "kernel/types.h"
#include "hardware/PPCasm.h"
#include "hardware/debugger.h"
#include "file/directory.h"
#include "dipir.h"
#include "notsysrom.h"
#include "hostfs.h"


extern const DipirRoutines *dipr;

/*****************************************************************************
 Dismount a host filesystem.
*/
	void
DismountHostFS(DipirHWResource *dev, RefToken refToken)
{
	HostFSReq cmd;
	HostFSReply reply;

	cmd.hfs_Command = HOSTFS_REMOTECMD_DISMOUNTFS;
	cmd.hfs_ReferenceToken = refToken;
	cmd.hfs_Send.iob_Buffer = NULL;
	cmd.hfs_Recv.iob_Buffer = NULL;
	cmd.hfs_Unit = HOST_FS_UNIT;
	SendHostCmd(dev, &cmd);
	GetHostReply(dev, &reply);
}

/*****************************************************************************
 Mount a host filesystem.
*/
	int32
MountHostFS(DipirHWResource *dev, char *fsName, RefToken *pRefToken)
{
	char *name;
	uint32 fsOffset;
	HostFSReq cmd;
	HostFSReply reply;

	name = DipirAlloc(FILESYSTEM_MAX_NAME_LEN, ALLOC_DMA);
	if (name == NULL)
		return -1;
	fsOffset = 0;
	for (;;)
	{
		cmd.hfs_Command = HOSTFS_REMOTECMD_MOUNTFS;
		cmd.hfs_Offset = fsOffset++; 
		cmd.hfs_Send.iob_Buffer = NULL; 
		cmd.hfs_Recv.iob_Buffer = name; 
		cmd.hfs_Recv.iob_Len = FILESYSTEM_MAX_NAME_LEN;
		cmd.hfs_Unit = HOST_FS_UNIT;
		SendHostCmd(dev, &cmd);
		GetHostReply(dev, &reply);
		if (reply.hfsr_Error < 0)
		{
			DipirFree(name);
			return reply.hfsr_Error;
		}
		if (strcmp(name, fsName) == 0)
			break;
		DismountHostFS(dev, reply.hfsr_ReferenceToken);
	}
	DipirFree(name);
	*pRefToken = reply.hfsr_ReferenceToken;
	return 0;
}

/*****************************************************************************
 Close the currently selected host file.
*/
	void
CloseHostFSFile(DipirHWResource *dev, RefToken fileToken)
{
	HostFSReq cmd;
	HostFSReply reply;

	if (fileToken == NULL_TOKEN)
		return;
	cmd.hfs_Command = HOSTFS_REMOTECMD_CLOSEENTRY;
	cmd.hfs_ReferenceToken = fileToken;
	cmd.hfs_Send.iob_Buffer = NULL;
	cmd.hfs_Recv.iob_Buffer = NULL;
	cmd.hfs_Unit = HOST_FS_UNIT;
	SendHostCmd(dev, &cmd);
	GetHostReply(dev, &reply);
}

/*****************************************************************************
*/
	int32
OpenHostFSFile(DipirHWResource *dev, RefToken dirToken, char *filename, 
	RefToken *pRefToken, uint32 *pSize, uint32 *pBlockSize)
{
	char *filenameCopy;
	uint32 filenameLen;
	DirectoryEntry *dir;
	HostFSReq cmd;
	HostFSReply reply;

	dir = DipirAlloc(sizeof(DirectoryEntry), ALLOC_DMA); 
	if (dir == NULL)
		return -1;
	filenameLen = strlen(filename) + 1;
	filenameCopy = DipirAlloc(filenameLen, ALLOC_DMA);
	if (filenameCopy == NULL)
	{
		DipirFree(dir);
		return -1;
	}
	strcpy(filenameCopy, filename);

	cmd.hfs_Command = HOSTFS_REMOTECMD_READENTRY;
	cmd.hfs_Send.iob_Buffer = filenameCopy;
	cmd.hfs_Send.iob_Len = filenameLen;
	cmd.hfs_ReferenceToken = dirToken;
	cmd.hfs_Recv.iob_Buffer = dir;
	cmd.hfs_Recv.iob_Len = sizeof(DirectoryEntry);
	cmd.hfs_Unit = HOST_FS_UNIT;
	SendHostCmd(dev, &cmd);
	GetHostReply(dev, &reply);
	if (reply.hfsr_Error < 0)
		goto Error;

	cmd.hfs_Command = HOSTFS_REMOTECMD_OPENENTRY, 
	cmd.hfs_Recv.iob_Buffer = NULL;
	cmd.hfs_Unit = HOST_FS_UNIT;
	SendHostCmd(dev, &cmd);
	GetHostReply(dev, &reply);
	if (reply.hfsr_Error < 0)
		goto Error;

	if (pSize != NULL)
		*pSize = dir->de_ByteCount;
	if (pBlockSize != NULL)
		*pBlockSize = dir->de_BlockSize;
	DipirFree(filenameCopy);
	DipirFree(dir);
	*pRefToken = reply.hfsr_ReferenceToken;
	return 0;
Error:
	DipirFree(filenameCopy);
	DipirFree(dir);
	return reply.hfsr_Error;
}

	void *
ReadHostFSFileAsync(DipirHWResource *dev, RefToken file, 
		uint32 block, uint32 len, void *buffer)
{
	HostFSReq cmd;

	cmd.hfs_Command = HOSTFS_REMOTECMD_BLOCKREAD;
	cmd.hfs_ReferenceToken = file;
	cmd.hfs_Offset = block;
	cmd.hfs_Send.iob_Buffer = NULL;
	cmd.hfs_Recv.iob_Buffer = buffer;
	cmd.hfs_Recv.iob_Len = len;
	cmd.hfs_UserData = buffer;
	cmd.hfs_Unit = HOST_FS_UNIT;
	SendHostCmd(dev, &cmd);
	return (void *) cmd.hfs_UserData;
}

	int32
WaitReadHostFSFile(DipirHWResource *dev, void *id)
{
	HostFSReply reply;

	if (id == 0)
	{
		EPRINTF(("Host: no read in progress\n"));
		return -1;
	}
	GetHostReply(dev, &reply);
	if (reply.hfsr_UserData != id)
	{
		EPRINTF(("Host: read sync error (%x != %x)\n",
			reply.hfsr_UserData, id));
		return -1;
	}
	if (reply.hfsr_Error < 0)
	{
		EPRINTF(("Host: device error %x\n", reply.hfsr_Error));
		return reply.hfsr_Error;
	}
	return reply.hfsr_Actual;
}
