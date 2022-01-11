/*
 *	@(#) dev.chan.c 96/01/10 1.20
 *	Copyright 1994,1995, The 3DO Company
 *
 * Dipir Device Driver (DDD) for ROMs with Channel drivers.
 * This is just a pass-thru to present a DDD interface for a
 * device that already has a Channel driver.
 */

#include "kernel/types.h"
#include "dipir.h"
#include "insysrom.h"


/*****************************************************************************
 Open a Channel device.
*/
	static int32
ChanOpen(DDDFile *fd)
{

	/* dipr = fd->fd_DipirTemp->dt_DipirRoutines; */
	/*
	 * We can set the block size to 1 for now.
	 * If the volume label indicates a different block size,
	 * we'll update it when we read the volume label (in ..)
	 */
	fd->fd_BlockSize = 1;
	if (fd->fd_HWResource->dev_Flags & HWR_SECURE_ROM)
		fd->fd_Flags |= DDD_SECURE;
	return 0;
}

/*****************************************************************************
 Close a Channel device.
*/
	static int32
ChanClose(DDDFile *fd)
{
	/* dipr = fd->fd_DipirTemp->dt_DipirRoutines; */
	TOUCH(fd);
	return 0;
}

/*****************************************************************************
 Read from a Channel device.
*/
	static void *
ChanReadAsync(DDDFile *fd, uint32 blk, uint32 nblks, void *buffer)
{
	int32 n;

	/* dipr = fd->fd_DipirTemp->dt_DipirRoutines; */
	n = (*fd->fd_HWResource->dev_ChannelDriver->cd_Read)
			(fd->fd_HWResource, blk * fd->fd_BlockSize,
			 nblks * fd->fd_BlockSize, buffer);
	if (n < 0)
		return (void *) n;
	return (void *) (n / fd->fd_BlockSize);
}

/*****************************************************************************
 Wait for a Channel device read to finish.
*/
	static int32
ChanWaitRead(DDDFile *fd, void *id)
{
	TOUCH(fd);
	return (int32) id;
}

/*****************************************************************************
 Map a Channel device into memory.
*/
	static int32
ChanMap(DDDFile *fd, uint32 offset, uint32 len, void **paddr)
{
	return (*fd->fd_HWResource->dev_ChannelDriver->cd_Map)
		(fd->fd_HWResource, offset, len, paddr);
}

/*****************************************************************************
 Unmap a Channel device.
*/
	static int32
ChanUnmap(DDDFile *fd, uint32 offset, uint32 len)
{
	return (*fd->fd_HWResource->dev_ChannelDriver->cd_Unmap)
		(fd->fd_HWResource, offset, len);
}

/*****************************************************************************
 Get info about a Channel device.
*/
	static int32
ChanGetInfo(DDDFile *fd, DeviceInfo *info)
{
	/* dipr = fd->fd_DipirTemp->dt_DipirRoutines; */
	info->di_Version = 1;
	info->di_BlockSize = fd->fd_BlockSize;
	info->di_FirstBlock = fd->fd_HWResource->dev.hwr_ROMUserStart;
	info->di_NumBlocks = fd->fd_HWResource->dev.hwr_ROMSize / fd->fd_BlockSize;
	return 0;
}

/*****************************************************************************
 What is the next block to try in a search for device label?
*/
	static int32
ChanRetryLabel(DDDFile *fd, uint32 *pState)
{
	return (*fd->fd_HWResource->dev_ChannelDriver->cd_RetryLabel)
		(fd->fd_HWResource, pState);
}

/*****************************************************************************
*/

static const DDDCmdTable ChanCmdTable[] =
{
	{ DDDCMD_OPEN,		(DDDFunction *) ChanOpen },
	{ DDDCMD_CLOSE,		(DDDFunction *) ChanClose },
	{ DDDCMD_READASYNC,	(DDDFunction *) ChanReadAsync },
	{ DDDCMD_WAITREAD,	(DDDFunction *) ChanWaitRead },
	{ DDDCMD_GETINFO,	(DDDFunction *) ChanGetInfo },
	{ DDDCMD_MAP,		(DDDFunction *) ChanMap },
	{ DDDCMD_UNMAP,		(DDDFunction *) ChanUnmap },
	{ DDDCMD_RETRYLABEL,	(DDDFunction *) ChanRetryLabel },
	{ 0 }
};

const DDD ChannelDDD = { ChanCmdTable };
