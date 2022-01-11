/*
 *	@(#) ddd.c 96/06/10 1.21
 *	Copyright 1995, The 3DO Company
 *
 * Interface functions to call into a DDD (dipir device driver).
 */

#include "kernel/types.h"
#include "dipir.h"
#include "insysrom.h"

extern int32 ReadLabelStuff(DDDFile *fd);
extern void FreeLabelStuff(DDDFile *fd);

typedef int32 OpenDeviceFunction(DDDFile *fd);
typedef int32 CloseDeviceFunction(DDDFile *fd);
typedef void *ReadAsyncFunction(DDDFile *fd, uint32 blk, uint32 nblks, void *buffer);
typedef int32 WaitReadFunction(DDDFile *fd, void *id);
typedef int32 EjectDeviceFunction(DDDFile *fd);
typedef int32 GetDeviceInfoFunction(DDDFile *fd, DeviceInfo *info);
typedef int32 MapDeviceFunction(DDDFile *fd, uint32 offset, uint32 len, void **paddr);
typedef int32 UnmapDeviceFunction(DDDFile *fd, uint32 offset, uint32 len);
typedef int32 RetryLabelDeviceFunction(DDDFile *fd, uint32 *pState);


/*****************************************************************************
 Find a function in a DDD command table.
*/
	DDDFunction *
FindDDDFunction(DDD *ddd, uint32 cmd)
{
	DDDCmdTable *p;

	for (p = ddd->ddd_CmdTable;  p->dddcmd_Cmd != 0;  p++)
		if (p->dddcmd_Cmd == cmd)
			return p->dddcmd_Func;
	return NULL;
}

/*****************************************************************************
 Does a DDD support a specified command?
*/
	Boolean
DriverSupports(DDDFile *fd, uint32 cmd)
{
	return (FindDDDFunction(fd->fd_DDD, cmd) != NULL);
}

/**
|||	AUTODOC -private -class Dipir -group DDD -name DDDCMD_OPEN
|||	Opens a Dipir Device Driver.
|||
|||	  Synopsis
|||
|||	    int32 xxx_Open(DDDFile *fd);
|||
|||	  Description
|||
|||	    This function opens a Dipir Device Driver.  It returns
|||	    zero on success, or a negative number if an error occurs.
|||	    It may also return the special value DDD_OPEN_IGNORE to
|||	    indicate that the device should be ignored by dipir.
|||
|||	  Arguments
|||
|||	    fd
|||	        Pointer to a DDDFile structure.  The structure is
|||	        partially initialized at the time the Open function
|||	        is called.  Only the fd_HWResource, fd->fd_DDD, and
|||	        fd->fd_DipirTemp fields are valid.  If the open
|||	        succeeds, the Open function should initialize the
|||	        fd_BlockSize field.
|||
|||	  Return Value
|||
|||	    Returns 0 on success, or a negative number on error,
|||	    or DDD_OPEN_IGNORE if the device is to be ignored.
|||
|||	  Implementation
|||
|||	    Dipir Device Driver command.
|||
|||	  See Also
|||
|||	    DDDCMD_CLOSE(@)
|||
**/

/*****************************************************************************
 Open a DDD.
*/
	DDDFile *
OpenDipirDevice(DipirHWResource *dev, DDD *ddd)
{
	DDDFile *fd;
	OpenDeviceFunction *func;
	int32 ret;

	func = (OpenDeviceFunction *)
		FindDDDFunction(ddd, DDDCMD_OPEN);
	if (func == NULL)
		return NULL;
	fd = DipirAlloc(sizeof(DDDFile), 0);
	if (fd == NULL)
		return NULL;
	memset(fd, 0, sizeof(DDDFile));
	fd->fd_Version = 1;
	fd->fd_HWResource = dev;
	fd->fd_DDD = ddd;
	fd->fd_DipirTemp = dtmp;
	ret = (*func)(fd);
	if (ret == DDD_OPEN_IGNORE)
	{
		DipirFree(fd);
		return IGNORE_DEVICE;
	}
	if (ret < 0)
	{
		DipirFree(fd);
		return NULL;
	}
	(void) ReadLabelStuff(fd);
	return fd;
}

/**
|||	AUTODOC -private -class Dipir -group DDD -name DDDCMD_CLOSE
|||	Closes a Dipir Device Driver.
|||
|||	  Synopsis
|||
|||	    int32 xxx_Close(DDDFile *fd);
|||
|||	  Description
|||
|||	    This function closes a Dipir Device Driver which was
|||	    previously opened via DDDCMD_OPEN(@).  It returns
|||	    zero on success, or a negative number if an error occurs.
|||
|||	  Arguments
|||
|||	    fd
|||	        Pointer to the DDDFile structure for the DDD to
|||	        be closed.
|||
|||	  Return Value
|||
|||	    Returns 0 on success, or a negative number on error.
|||
|||	  Implementation
|||
|||	    Dipir Device Driver command.
|||
|||	  See Also
|||
|||	    DDDCMD_OPEN(@)
|||
**/

/*****************************************************************************
 Close a DDD.
*/
	int32
CloseDipirDevice(DDDFile *fd)
{
	CloseDeviceFunction *func;
	int32 ret;

	FreeLabelStuff(fd);
	func = (CloseDeviceFunction *)
		FindDDDFunction(fd->fd_DDD, DDDCMD_CLOSE);
	if (func == NULL)
		return -1;
	ret = (*func)(fd);
	DipirFree(fd);
	return ret;
}

/**
|||	AUTODOC -private -class Dipir -group DDD -name DDDCMD_READASYNC
|||	Read data from a Dipir Device Driver.
|||
|||	  Synopsis
|||
|||	    void * xxx_ReadAsync(DDDFile *fd, uint32 blk, uint32 nblks, void *buffer);
|||
|||	  Description
|||
|||	    This function initiates a read of data from a Dipir Device 
|||	    Driver.  The function may return before the data has been
|||	    read.  The interface DDDCMD_WAITREAD(@) can be used to 
|||	    wait until the data is completely read.
|||
|||	  Arguments
|||
|||	    fd
|||	        Pointer to the DDDFile structure for the DDD from
|||	        which to read data.
|||
|||	    blk
|||	        First block number to read.  The size of the blocks
|||	        on the device is given by fd->fd_BlockSize.
|||
|||	    nblks
|||	        Number of blocks to read.
|||
|||	    buffer
|||	        Buffer into which the data should be placed.
|||
|||	  Return Value
|||
|||	    Returns an identifier which can be passed to the
|||	    DDDCMD_WAITREAD function.  If an error occurs, returns
|||	    the value (void*)(-1).
|||
|||	  Implementation
|||
|||	    Dipir Device Driver command.
|||
|||	  See Also
|||
|||	    DDDCMD_WAITREAD(@)
|||
**/

/*****************************************************************************
 Initiate a read from a device.
*/
	void *
ReadAsync(DDDFile *fd, uint32 blk, uint32 nblks, void *buffer)
{
	ReadAsyncFunction *func;

	func = (ReadAsyncFunction *) 
		FindDDDFunction(fd->fd_DDD, DDDCMD_READASYNC);
	if (func == NULL)
		return 0;
	return (*func)(fd, blk, nblks, buffer);
}

/**
|||	AUTODOC -private -class Dipir -group DDD -name DDDCMD_WAITREAD
|||	Wait for data to be read from a Dipir Device Driver.
|||
|||	  Synopsis
|||
|||	    void * xxx_WaitRead(DDDFile *fd, void *id);
|||
|||	  Description
|||
|||	    Wait for data to be read from a Dipir Device Driver.
|||	    After a read is initiated via DDDCMD_READASYNC(@),
|||	    the caller may use DDDCMD_WAITREAD to wait until
|||	    the data has been completely read into memory.
|||
|||	  Arguments
|||
|||	    fd
|||	        Pointer to the DDDFile structure for the DDD from
|||	        which to read data.
|||
|||	    id
|||	        A value returned from a previous call to
|||	        DDDCMD_READASYNC(@).
|||
|||	  Return Value
|||
|||	    Returns the number of blocks actually read.
|||
|||	  Implementation
|||
|||	    Dipir Device Driver command.
|||
|||	  See Also
|||
|||	    DDDCMD_READASYNC(@)
|||
**/

/*****************************************************************************
 Wait for a read previously initiated by ReadAsync to finish.
*/
	int32
WaitRead(DDDFile *fd, void *id)
{
	WaitReadFunction *func;

	func = (WaitReadFunction *)
		FindDDDFunction(fd->fd_DDD, DDDCMD_WAITREAD);
	if (func == NULL)
		return -1;
	return (*func)(fd, id);
}

/*****************************************************************************
 Read from a device (synchronously).
*/
	int32
ReadSync(DDDFile *fd, uint32 blk, uint32 nblks, void *buffer)
{
	void *id;

	id = ReadAsync(fd, blk, nblks, buffer);
	return WaitRead(fd, id);
}

/**
|||	AUTODOC -private -class Dipir -group DDD -name DDDCMD_EJECT
|||	"Eject" a device.
|||
|||	  Synopsis
|||
|||	     int32 xxx_Eject(DDDFile *fd);
|||
|||	  Description
|||
|||	    "Eject" a device.  The actual result of this call
|||	    depends on the type of device, but it is intended to
|||	    ensure that no further data can be read from the device.
|||
|||	  Arguments
|||
|||	    fd
|||	        Pointer to the DDDFile structure for the device
|||	        to eject.
|||
|||	  Return Value
|||
|||	    Returns 0 on success, or a negative value on failure.
|||
|||	  Implementation
|||
|||	    Dipir Device Driver command.
|||
|||	  See Also
|||
|||	    DDDCMD_OPEN(@)
|||
**/

/*****************************************************************************
 Eject a device.
*/
	int32
EjectDevice(DDDFile *fd)
{
	EjectDeviceFunction *func;

	func = (EjectDeviceFunction *)
		FindDDDFunction(fd->fd_DDD, DDDCMD_EJECT);
	if (func == NULL)
		return -1;
	return (*func)(fd);
}

/**
|||	AUTODOC -private -class Dipir -group DDD -name DDDCMD_GETINFO
|||	Get information about a device.
|||
|||	  Synopsis
|||
|||	    void * xxx_GetInfo(DDDFile *fd, DeviceInfo *info);
|||
|||	  Description
|||
|||	    Fills in the supplied DeviceInfo structure with
|||	    information about the device.
|||
|||	  Arguments
|||
|||	    fd
|||	        Pointer to the DDDFile structure for the device.
|||
|||	    info
|||	        Pointer to a DeviceInfo structure into which to
|||	        put the information about the device.
|||
|||	  Return Value
|||
|||	    Returns 0 on success, or a negative value on failure.
|||
|||	  Implementation
|||
|||	    Dipir Device Driver command.
|||
|||	  See Also
|||
|||	    DDDCMD_OPEN(@)
|||
**/

/*****************************************************************************
 Get info about a device.
*/
	int32
GetDeviceInfo(DDDFile *fd, DeviceInfo *info)
{
	GetDeviceInfoFunction *func;

	func = (GetDeviceInfoFunction *)
		FindDDDFunction(fd->fd_DDD, DDDCMD_GETINFO);
	if (func == NULL)
		return -1;
	return (*func)(fd, info);
}

/**
|||	AUTODOC -private -class Dipir -group DDD -name DDDCMD_MAP
|||	Memory-map a device.
|||
|||	  Synopsis
|||
|||	    int32 xxx_Map(DDDFile *fd, uint32 blk, uint32 nblks, void **paddr);
|||
|||	  Description
|||
|||	    Map a device into memory, if possible.  Returns the address
|||	    at which the device was mapped in the paddr argument.
|||
|||	  Arguments
|||
|||	    fd
|||	        Pointer to the DDDFile structure for the device to map.
|||
|||	    blk
|||	        First block number to map.
|||
|||	    nblks
|||	        Number of blocks to map.
|||
|||	    paddr
|||	        Pointer to a variable into which is placed the address 
|||	        of the memory region at which the device was mapped.
|||
|||	  Return Value
|||
|||	    Returns the number of blocks actually mapped, or a negative
|||	    number if an error occured or if the device does not
|||	    support memory mapping.
|||
|||	  Implementation
|||
|||	    Dipir Device Driver command.
|||
|||	  See Also
|||
|||	    DDDCMD_UNMAP(@)
|||
**/

/*****************************************************************************
 Map a device into memory (if possible).
*/
	int32
MapDevice(DDDFile *fd, uint32 offset, uint32 len, void **paddr)
{
	MapDeviceFunction *func;

	func = (MapDeviceFunction *)
		FindDDDFunction(fd->fd_DDD, DDDCMD_MAP);
	if (func == NULL)
		return -1;
	return (*func)(fd, offset, len, paddr);
}

/**
|||	AUTODOC -private -class Dipir -group DDD -name DDDCMD_UNMAP
|||	Un-memory-map a device.
|||
|||	  Synopsis
|||
|||	    int32 xxx_Unmap(DDDFile *fd, uint32 blk, uint32 nblks);
|||
|||	  Description
|||
|||	    Unmap a device from memory.  Undoes a previous call to
|||	    DDDCMD_MAP(@).  The blk and nblks arguments must be
|||	    identical to the corresponding arguments which were
|||	    previously passed into the DDDCMD_MAP(@) function.
|||
|||	  Arguments
|||
|||	    fd
|||	        Pointer to the DDDFile structure for the device to unmap.
|||
|||	    blk
|||	        First block number to unmap.
|||
|||	    nblks
|||	        Number of blocks to unmap.
|||
|||	  Return Value
|||
|||	    Returns 0 on success, or a negative number on error.
|||
|||	  Implementation
|||
|||	    Dipir Device Driver command.
|||
|||	  See Also
|||
|||	    DDDCMD_MAP(@)
|||
**/

/*****************************************************************************
 Unmap a device from memory.
*/
	int32
UnmapDevice(DDDFile *fd, uint32 offset, uint32 len)
{
	UnmapDeviceFunction *func;

	func = (UnmapDeviceFunction *)
		FindDDDFunction(fd->fd_DDD, DDDCMD_UNMAP);
	if (func == NULL)
		return -1;
	return (*func)(fd, offset, len);
}

/**
|||	AUTODOC -private -class Dipir -group DDD -name DDDCMD_RETRYLABEL
|||	Suggest possible locations for a device label.
|||
|||	  Synopsis
|||
|||	    int32 xxx_RetryLabel(DDDFile *fd, uint32 *pState);
|||
|||	  Description
|||
|||	    If *pState is zero, returns the block number of the 
|||	    block on the device most likely to contain a device label.
|||	    Subsequent calls will return other block numbers where
|||	    a label might reside.  Returns a negative number when
|||	    there are no more candidates.
|||
|||	    The function may return a negative number on the first
|||	    call.  This indicates that the only possible label block
|||	    is di_FirstBlock in the device's DeviceInfo structure.
|||
|||	  Arguments
|||
|||	    fd
|||	        Pointer to the DDDFile structure for the device.
|||
|||	    pState
|||	        Pointer to an integer used by the function to keep
|||	        track of where it is in the list of candidate blocks.
|||	        The caller should set the word pointed to by pState
|||	        to zero prior to the first call, and should not 
|||	        modify it afterwards.
|||
|||	  Return Value
|||
|||	    Returns the block number of the next possible label block,
|||	    or -1 if there are no more possible label blocks.
|||
|||	  Implementation
|||
|||	    Dipir Device Driver command.
|||
|||	  See Also
|||
|||	    DDDCMD_OPEN(@)
|||
**/

/*****************************************************************************
 What is the next block in search for device label?
*/
	int32
RetryLabelDevice(DDDFile *fd, uint32 *pState)
{
	RetryLabelDeviceFunction *func;

	func = (RetryLabelDeviceFunction *)
		FindDDDFunction(fd->fd_DDD, DDDCMD_RETRYLABEL);
	if (func == NULL)
		return -1;
	return (*func)(fd, pState);
}

