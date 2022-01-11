/*
 *	@(#) ddd.cd.c 96/06/10 1.10
 *	Copyright 1995, The 3DO Company
 */

#include "kernel/types.h"
#include "dipir.h"
#include "notsysrom.h"
#include "ddd.cd.h"

extern const DipirRoutines *dipr;

/**
|||	AUTODOC -private -class Dipir -group DDD -name DDDCMD_CD_GETINFO
|||	Get CD-specific info from a device.
|||
|||	  Synopsis
|||
|||	    int32 xxx_CDGetInfo(DDDFile *fd, DiscInfo *di);
|||
|||	  Description
|||
|||	    Gets CD-specific information from a CD-ROM device.
|||	    Returns zero on success, or a negative number if an 
|||	    error occurs.
|||
|||	  Arguments
|||
|||	    fd
|||	        Pointer to a DDDFile structure for the device.
|||
|||	    di
|||	        Pointer to a DiscInfo structure into which the
|||	        disc information is placed.
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
|||	    DDDCMD_OPEN(@), DDDCMD_GETINFO(@)
|||
**/

	int32
DeviceGetCDInfo(DDDFile *fd, DiscInfo *di)
{
	DeviceGetCDInfoFunction *func;

	func = (DeviceGetCDInfoFunction *)
		FindDDDFunction(fd->fd_DDD, DDDCMD_CD_GETINFO);
	if (func == NULL)
		return -1;
	return (*func)(fd, di);
}

/**
|||	AUTODOC -private -class Dipir -group DDD -name DDDCMD_CD_GETFLAGS
|||	Get CD-specific flags from a device.
|||
|||	  Synopsis
|||
|||	    int32 xxx_CDGetFlags(DDDFile *fd, uint32 *pFlags);
|||
|||	  Description
|||
|||	    Gets CD-specific flags from a CD-ROM device.
|||	    Returns zero on success, or a negative number if an 
|||	    error occurs.
|||
|||	  Arguments
|||
|||	    fd
|||	        Pointer to a DDDFile structure for the device.
|||
|||	    pFlags
|||	        Pointer to a word into which the CD flags are placed.
|||	        Bits in the flag word are:
|||
|||	    CD_MULTISESSION
|||	        This is a multi-session disc.
|||	    CD_WRITABLE
|||	        This is a writable disc.
|||	    CD_NOT_3DO
|||	        This is not a 3DO-format disc.
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
|||	    DDDCMD_OPEN(@), DDDCMD_GETINFO(@), DDDCMD_CD_GETINFO(@)
|||
**/

	int32
DeviceGetCDFlags(DDDFile *fd, uint32 *pFlags)
{
	DeviceGetCDFlagsFunction *func;

	func = (DeviceGetCDFlagsFunction *) 
		FindDDDFunction(fd->fd_DDD, DDDCMD_CD_GETFLAGS);
	if (func == NULL)
		return -1;
	return (*func)(fd, pFlags);
}

/**
|||	AUTODOC -private -class Dipir -group DDD -name DDDCMD_CD_GETTOC
|||	Get table of contents from a CD device.
|||
|||	  Synopsis
|||
|||	    int32 xxx_CDGetTOC(DDDFile *fd, uint32 track, TOCInfo *ti);
|||
|||	  Description
|||
|||	    Reads the table of contents from a CD-ROM device.
|||	    Returns zero on success, or a negative number if an 
|||	    error occurs.
|||
|||	  Arguments
|||
|||	    fd
|||	        Pointer to a DDDFile structure for the device.
|||
|||	    track
|||	        Track number whose table of contents is to be read.
|||
|||	    ti
|||	        Pointer to a TOCInfo structure into which the table
|||	        of contents is to be placed.
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
|||	    DDDCMD_OPEN(@), DDDCMD_GETINFO(@), DDDCMD_CD_GETINFO(@)
|||
**/

	int32
DeviceGetCDTOC(DDDFile *fd, uint32 track, TOCInfo *ti)
{
	DeviceGetCDTOCFunction *func;

	func = (DeviceGetCDTOCFunction *) 
		FindDDDFunction(fd->fd_DDD, DDDCMD_CD_GETTOC);
	if (func == NULL)
		return -1;
	return (*func)(fd, track, ti);
}

/**
|||	AUTODOC -private -class Dipir -group DDD -name DDDCMD_CD_GETLASTECC
|||	Get info about the last ECC error from a CD device.
|||
|||	  Synopsis
|||
|||	    int32 xxx_CDGetLastECC(DDDFile *fd, uint32 *pECC);
|||
|||	  Description
|||
|||	    Returns in the word pointed to by pECC the ECC error
|||	    code from the last ECC error which occured on the device.
|||
|||	  Arguments
|||
|||	    fd
|||	        Pointer to a DDDFile structure for the device.
|||
|||	    pECC
|||	        Pointer to a word into which the ECC error code is placed.
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

	int32 
DeviceGetLastECC(DDDFile *fd, uint32 *pECC)
{
	DeviceGetLastECCFunction *func;

	func = (DeviceGetLastECCFunction *) 
		FindDDDFunction(fd->fd_DDD, DDDCMD_CD_GETLASTECC);
	if (func == NULL)
		return -1;
	return (*func)(fd, pECC);
}

/**
|||	AUTODOC -private -class Dipir -group DDD -name DDDCMD_CD_GETWDATA
|||	Read WData from a CD device.
|||
|||	  Synopsis
|||
|||	    int32 xxx_CDGetWData(DDDFile *fd, uint32 block, uint32 speed, uint32 bufSize, uint32 totalSize, int32 (*Callback)(DDDFile *fd, void *callbackArg, void *buf, uint32 bufSize), void *callbackArg);
|||
|||	  Description
|||
|||	    Reads WData from the specified CD device.  Each buffer
|||	    (bufSize bytes) is passed to the callback function to
|||	    be processed.  The function double-buffers the WData 
|||	    and continues reading the next buffer of data while the 
|||	    callback function processes the previous buffer.  The
|||	    callback function must complete before the next buffer
|||	    is completely read.
|||
|||	  Arguments
|||
|||	    fd
|||	        Pointer to a DDDFile structure for the device.
|||
|||	    block
|||	        Block number to start reading.  If block is zero,
|||	        data is read at the current position of the read head.
|||
|||	    speed
|||	        Speed at which to read the data.  Must be 1, 2 or 4
|||	        (for single-speed, double-speed or quad-speed).
|||
|||	    bufSize
|||	        Amount of data to be passed to the callback function.
|||
|||	    totalSize
|||	        Total amount of data to be read.  totalSize must be
|||	        an integral multiple of bufSize.
|||
|||	    Callback
|||	        Pointer to the callback function.
|||
|||	    callbackArg
|||	        Value to be passed as the second argument to the
|||	        callback function.
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

	int32
DeviceGetWData(DDDFile *fd, uint32 block, uint32 speed, 
	uint32 bufSize, uint32 totalSize,
	int32 (*Callback)(DDDFile *fd, void *callbackArg, void *buf, uint32 bufSize),
	void *callbackArg)
{
	DeviceGetWDataFunction *func;

	func = (DeviceGetWDataFunction *) 
		FindDDDFunction(fd->fd_DDD, DDDCMD_CD_GETWDATA);
	if (func == NULL)
		return -1;
	return (*func)(fd, block, speed, bufSize, totalSize, 
				Callback, callbackArg);
}

/**
|||	AUTODOC -private -class Dipir -group DDD -name DDDCMD_CD_FIRMCMD
|||	Issue a firmware command to a CD device.
|||
|||	  Synopsis
|||
|||	    int32 xxx_CDGetFirmCmd(DDDFile *fd, uint8 *cmd, uint32 cmdLen, uint8 *resp, uint32 respLen, uint32 timeout);
|||
|||	  Description
|||
|||	    Sends a low-level firmware command to the CD device.
|||
|||	  Arguments
|||
|||	    fd
|||	        Pointer to a DDDFile structure for the device.
|||
|||	    cmd
|||	        Pointer to the command bytes to send.
|||
|||	    cmdLen
|||	        Number of bytes to send.
|||
|||	    resp
|||	        Pointer to a buffer which receives the response
|||	        from the command.
|||
|||	    respLen
|||	        Size of the resp buffer.
|||
|||	    timeout
|||	        Number of milliseconds to wait for a response.
|||
|||	  Return Value
|||
|||	    Returns 0 on success, or a negative number on error.
|||
|||	  Implementation
|||
|||	    Dipir Device Driver command.
|||
|||	  Note
|||
|||	    This command is for future flexibility.  This function
|||	    should be used carefully.  Calling this function from
|||	    a device-dipir will constrain the ability to change the
|||	    CD-ROM hardware in the future.
|||
|||	  See Also
|||
|||	    DDDCMD_OPEN(@)
|||
**/

	int32
DeviceCDFirmCmd(DDDFile *fd, uint8 *cmd, uint32 cmdLen,
	uint8 *resp, uint32 respLen, uint32 timeout)
{
	DeviceCDFirmCmdFunction *func;

	func = (DeviceCDFirmCmdFunction *)
		FindDDDFunction(fd->fd_DDD, DDDCMD_CD_FIRMCMD);
	if (func == NULL)
		return -1;
	return (*func)(fd, cmd, cmdLen, resp, respLen, timeout);
}
