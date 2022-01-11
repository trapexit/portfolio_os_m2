/******************************************************************************
**
**  @(#) dsblockfile.c 96/02/26 1.11
**
******************************************************************************/

#include <strings.h>			/* memset() */
#include <file/filefunctions.h>
#include <kernel/debug.h>		/* CHECK_NEG(), ERROR_RESULT_STATUS() */
#include <kernel/io.h>
#include <kernel/kernelnodes.h>

#include <streaming/dsblockfile.h>


/******************************************************************************
|||	AUTODOC -public -class Streaming -group DSBlockFile -name --DSBlockFile-Overview--
|||	Overview of the DSBlockFile module.
|||	
|||	  Background
|||	
|||	    DSBlockFile provides a slightly higher-level API to block-oriented file
|||	    I/O (higher level than making the system calls directly). Its main
|||	    advantage is hiding some details of the device's API such as dealing
|||	    with IOInfo structures and translating byte numbers to block numbers.
|||	    (But you still have to use byte numbers that are integral multiples of
|||	    the device's block size. If you want arbitrary byte I/O, use the RawFile
|||	    procedures.) DSBlockFile is light weight--it doesn't even allocate
|||	    memory.
|||	    
|||	    DSBlockFile is used by the DataStreamer's DataAcq module.
|||	
|||	  Usage Overview
|||	
|||	    Setup: Allocate a BlockFile structure and call OpenBlockFile() to fill
|||	    it in and open the device. Then call CreateBlockFileIOReq() one or more
|||	    times to create IOReq Items. Call GetBlockFileSize() to get the file's
|||	    size and GetBlockFileBlockSize() to get the device's block size.
|||	    
|||	    I/O: Call AsynchReadBlockFile() to issue each I/O request. This
|||	    procedure takes byte count and byte offset arguments, but both arguments
|||	    must be integral multiples of the device's block size. Do not hardwire
|||	    the block size into your program. For example the "/remote" device has a
|||	    settable block size.
|||	    
|||	    I/O completion: Use the Item's message or signal completion notification,
|||	    or WaitReadDoneBlockFile() or WaitIO(), or CheckIO() to tell when each
|||	    I/O request completes. Don't access the data buffer until the I/O
|||	    operation has completed. Don't assume that I/O operations complete in
|||	    the order they were submitted. Look at the completed IOReq Item's
|||	    io_Error field for I/O completion status and its io_Actual field for the
|||	    actual number of bytes read.
|||	    
|||	    Cleanup: Call DeleteIOReq() to delete each IOReq Item created for this
|||	    BlockFile and then call CloseBlockFile() to close the device and clean
|||	    up the BlockFile structure. Or if the thread is about to exit, let the
|||	    OS automatically reclaim the Device Item and its IOReq Items when the
|||	    thread exits.
|||	    
|||	  Associated Files
|||	
|||	    <streaming/dsblockfile.h>, libdsutils.a
|||	
*******************************************************************************/


/******************************************************************************
|||	AUTODOC -public -class Streaming -group DSBlockFile -name CreateBlockFileIOReq
|||	Creates an I/O request Item for a BlockFile.
|||	
|||	  Synopsis
|||	
|||	    Item CreateBlockFileIOReq(Item deviceItem, Item iodoneReplyPort)
|||	
|||	  Description
|||	
|||	    Creates an I/O request Item, given the device Item from an open
|||	    BlockFile. This I/O request Item can be set to give its I/O completion
|||	    notifications by message or by signal.
|||	
|||	  Arguments
|||	
|||	    deviceItem
|||	        The open file's device Item from the BlockFile.
|||	
|||	    iodoneReplyPort
|||	        The Item number of a message reply port, or 0 to get I/O completion
|||	        notification via SIGF_IODONE signal.
|||	
|||	  Return Value
|||	
|||	    The new IOReq's Item number, or a negative Err code.
|||	
|||	  Assumes
|||	
|||	    The BlockFile has been opened (via OpenBlockFile()).
|||	
|||	  Implementation
|||	
|||	    Streaming library call.
|||	
|||	  Associated Files
|||	
|||	    <streaming/dsblockfile.h>, libdsutils.a
|||	
|||	  See Also
|||	
|||	    OpenBlockFile(), CloseBlockFile(), AsynchReadBlockFile().
|||	
*******************************************************************************/
Item CreateBlockFileIOReq(Item deviceItem, Item iodoneReplyPort)
	{
	return CreateIOReq(NULL, 0, deviceItem, iodoneReplyPort);
	}


/******************************************************************************
|||	AUTODOC -public -class Streaming -group DSBlockFile -name OpenBlockFile
|||	Opens a BlockFile.
|||	
|||	  Synopsis
|||	
|||	    Err OpenBlockFile(char *name, BlockFilePtr bf)
|||	
|||	  Description
|||	
|||	    Opens a BlockFile. The client allocates and frees the BlockFile
|||	    structure. OpenBlockFile() fills it in and CloseBlockFile() cleans
|||	    it up.
|||	
|||	  Arguments
|||	
|||	    name
|||	        The file name to open.
|||	
|||	    bf
|||	        A pointer to the BlockFile structure to fill in.
|||	
|||	  Return Value
|||	
|||	    Zero for success, or a negative Portfolio Err code.
|||	
|||	  Implementation
|||	
|||	    Streaming library call.
|||	
|||	  Associated Files
|||	
|||	    <streaming/dsblockfile.h>, libdsutils.a
|||	
|||	  See Also
|||	
|||	    CloseBlockFile(), AsynchReadBlockFile().
|||	
*******************************************************************************/
Err OpenBlockFile(char *name, BlockFilePtr bf)
	{
	IOInfo		Info;
	Item		ioreqItem;
	Err			status;

	status = bf->fDevice = OpenFile(name);
	if ( status < 0 )
		{
		ERROR_RESULT_STATUS("OpenBlockFile OpenFile", status);
		return status;
		}

	ioreqItem = CreateBlockFileIOReq(bf->fDevice, 0);
	if ( ioreqItem < 0 )
		{
		CloseBlockFile(bf);
		ERROR_RESULT_STATUS("OpenBlockFile CreateBlockFileIOReq", status);
		return ioreqItem;
		}

	memset(&Info, 0, sizeof(Info));
	Info.ioi_Command = CMD_STATUS;
	Info.ioi_Flags = IO_QUICK;
	Info.ioi_Recv.iob_Buffer = &(bf->fStatus);
	Info.ioi_Recv.iob_Len = sizeof(bf->fStatus);

	status = DoIO(ioreqItem, &Info);
	CHECK_NEG("OpenBlockFile DoIO", status);

	DeleteItem(ioreqItem);

	return status;
	}


/******************************************************************************
|||	AUTODOC -public -class Streaming -group DSBlockFile -name CloseBlockFile
|||	Closes a BlockFile.
|||	
|||	  Synopsis
|||	
|||	    void CloseBlockFile(BlockFilePtr bf)
|||	
|||	  Description
|||	
|||	    Closes a BlockFile. The client allocates and frees the BlockFile
|||	    structure. OpenBlockFile() fills it in and CloseBlockFile() cleans
|||	    it up.
|||	    
|||	    Before calling CloseBlockFile() you should call DeleteIOReq() to
|||	    delete each IOReq Item created for this BlockFile. But if the thread is
|||	    about to exit, you don't need to do either. The OS will automatically
|||	    reclaim the Device Item and its IOReq Items when the thread exits.
|||	
|||	  Arguments
|||	
|||	    bf
|||	        A pointer to the BlockFile structure to close.
|||	
|||	  Implementation
|||	
|||	    Streaming library call.
|||	
|||	  Associated Files
|||	
|||	    <streaming/dsblockfile.h>, libdsutils.a
|||	
|||	  See Also
|||	
|||	    OpenBlockFile(), AsynchReadBlockFile().
|||	
*******************************************************************************/
void CloseBlockFile(BlockFilePtr bf)
	{
	CloseFile(bf->fDevice);
	bf->fDevice = -1;
	}


/******************************************************************************
|||	AUTODOC -public -class Streaming -group DSBlockFile -name GetBlockFileSize
|||	Returns the size, in bytes, of an open BlockFile.
|||	
|||	  Synopsis
|||	
|||	    uint32 GetBlockFileSize(BlockFilePtr bf)
|||	
|||	  Description
|||	
|||	    Returns the size, in bytes, of an open BlockFile.
|||	
|||	  Arguments
|||	
|||	    bf
|||	        A pointer to the open BlockFile structure.
|||	
|||	  Return Value
|||	
|||	    The size, in bytes, of the open BlockFile.
|||	
|||	  Assumes
|||	
|||	    The BlockFile has been opened (via OpenBlockFile()).
|||	
|||	  Implementation
|||	
|||	    Streaming library call.
|||	
|||	  Associated Files
|||	
|||	    <streaming/dsblockfile.h>, libdsutils.a
|||	
|||	  See Also
|||	
|||	    OpenBlockFile().
|||	
*******************************************************************************/
uint32 GetBlockFileSize(BlockFilePtr bf)
	{
	return (int32)bf->fStatus.fs_ByteCount;
	}


/******************************************************************************
|||	AUTODOC -public -class Streaming -group DSBlockFile -name GetBlockFileBlockSize
|||	Returns the device block size, in bytes, of an open BlockFile.
|||	
|||	  Synopsis
|||	
|||	    uint32 GetBlockFileBlockSize(BlockFilePtr bf)
|||	
|||	  Description
|||	
|||	    Returns the device block size, in bytes, of an open BlockFile. You
|||	    should not hardwire this size into your programs. E.g. the /remote
|||	    device can be set to various block sizes.
|||	
|||	  Arguments
|||	
|||	    bf
|||	        A pointer to the open BlockFile structure.
|||	
|||	  Return Value
|||	
|||	    The block size, in bytes, of the open BlockFile's device. All I/O
|||	    operations must be done in integral blocks.
|||	
|||	  Assumes
|||	
|||	    The BlockFile has been opened (via OpenBlockFile()).
|||	
|||	  Implementation
|||	
|||	    Streaming library call.
|||	
|||	  Associated Files
|||	
|||	    <streaming/dsblockfile.h>, libdsutils.a
|||	
|||	  See Also
|||	
|||	    OpenBlockFile().
|||	
*******************************************************************************/
uint32 GetBlockFileBlockSize(BlockFilePtr bf)
	{
	return bf->fStatus.fs.ds_DeviceBlockSize;
	}


/******************************************************************************
|||	AUTODOC -public -class Streaming -group DSBlockFile -name AsynchReadBlockFile
|||	Issues an asynchronous read operation on an open BlockFile.
|||	
|||	  Synopsis
|||	
|||	    Err AsynchReadBlockFile(BlockFilePtr bf, Item ioreqItem,
|||	        void *buffer, uint32 count, uint32 offset)
|||	
|||	  Description
|||	
|||	    Issues an asynchronous read operation on an open BlockFile. Note that
|||	    the count and offset are expressed in bytes but MUST be multiples of
|||	    the device's block size.
|||	    
|||	    You can queue up multiple I/O operations using multiple ioReq Items.
|||	    
|||	    You will receive a message or a signal when each I/O request is
|||	    finished. See CreateBlockFileIOReq() for details. You can also check
|||	    to see if an I/O request is done by calling CheckIO(), or wait until it
|||	    is done by calling WaitReadDoneBlockFile() or call WaitIO() directly.
|||	    
|||	    Don't access the data buffer until the I/O operation has completed.
|||	    When the I/O operation completes, look at the IOReq Item's io_Error
|||	    field for I/O completion status, and look at its io_Actual field for
|||	    the actual number of bytes read.
|||	
|||	  Arguments
|||	
|||	    bf
|||	        A pointer to the open BlockFile structure.
|||	
|||	    ioreqItem
|||	        An IOReq Item created by CreateBlockFileIOReq() for this BlockFile.
|||	
|||	    buffer
|||	        The address of the data buffer to read into.
|||	
|||	    count
|||	        The number of bytes to read. This MUST be a multiple of the device's
|||	        block size. (Cf. GetBlockFileBlockSize().)
|||	
|||	    offset
|||	        The offset (or position) into the file to read. This MUST be a
|||	        multiple of the device's block size. (Cf. GetBlockFileBlockSize().)
|||	
|||	  Return Value
|||	
|||	    Zero for success, or a negative Portfolio Err code from SendIO().
|||	
|||	  Assumes
|||	
|||	    The BlockFile has been opened (via OpenBlockFile()).
|||	
|||	  Implementation
|||	
|||	    Streaming library call.
|||	
|||	  Associated Files
|||	
|||	    <streaming/dsblockfile.h>, libdsutils.a
|||	
|||	  See Also
|||	
|||	    OpenBlockFile(), CreateBlockFileIOReq().
|||	
*******************************************************************************/
Err AsynchReadBlockFile(BlockFilePtr bf, Item ioreqItem, void *buffer,
		uint32 count, uint32 offset)
	{
	IOInfo                  Info;

    memset(&Info, 0, sizeof(Info));
	Info.ioi_Command = CMD_BLOCKREAD;
	Info.ioi_Recv.iob_Buffer = buffer;
	Info.ioi_Recv.iob_Len = count;
	Info.ioi_Offset = offset / bf->fStatus.fs.ds_DeviceBlockSize;

	return SendIO(ioreqItem, &Info);
	}


/******************************************************************************
|||	AUTODOC -public -class Streaming -group DSBlockFile -name WaitReadDoneBlockFile
|||	Waits until an I/O request has completed.
|||	
|||	  Synopsis
|||	
|||	    Err WaitReadDoneBlockFile(Item ioreqItem)
|||	
|||	  Description
|||	
|||	    Waits until an I/O request (issued by AsynchReadBlockFile() on an open
|||	    BlockFile) has completed. This simply calls WaitIO(), which you could
|||	    call directly.
|||	
|||	  Arguments
|||	
|||	    ioreqItem
|||	        The IOReq Item.
|||	
|||	  Return Value
|||	
|||	    Zero for success, or a negative Portfolio Err code from WaitIO().
|||	
|||	  Implementation
|||	
|||	    Streaming library call.
|||	
|||	  Associated Files
|||	
|||	    <streaming/dsblockfile.h>, libdsutils.a
|||	
|||	  See Also
|||	
|||	    OpenBlockFile(), CreateBlockFileIOReq(), AsynchReadBlockFile().
|||	
*******************************************************************************/
Err WaitReadDoneBlockFile(Item ioreqItem)
	{
	return WaitIO(ioreqItem);
	}
