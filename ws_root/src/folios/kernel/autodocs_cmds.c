/* @(#) autodocs_cmds.c 96/12/11 1.37 */

/**
|||	AUTODOC -class Device_Commands -group MPEG -name MPEGVIDEOCMD_WRITE
|||	Submits a buffer of MPEG video data for decoding by the MPEG video device.
|||
|||	  Description
|||
|||	    This command submits a buffer containing MPEG video data to
|||	    the MPEG video device for decoding. Presentation time stamp (PTS)
|||	    information and user data may be optionally sent with the request.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to MPEGVIDEOCMD_WRITE.
|||
|||	    ioi_CmdOptions
|||	        Optional pointer to an FMVIOReqOptions structure containing
|||	        presentation time stamp (PTS) and user information. This data
|||	        flows through the decoder and is output with the corresponding
|||	        read request.
|||
|||	    ioi_Send.iob_Buffer
|||	        Pointer to a buffer of MPEG video data.
|||
|||	    ioi_Send.iob_Len
|||	        Set to the size of the MPEG video data buffer.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/mpegvideo.h>
|||
|||	  See Also
|||
|||	    MPEGVIDEOCMD_READ(@), MPEGVIDEOCMD_CONTROL(@)
|||
**/

/**
|||	AUTODOC -class Device_Commands -group MPEG -name MPEGVIDEOCMD_READ
|||	Submits a bitmap item for the MPEG video device to place a decoded picture.
|||
|||	  Description
|||
|||	    This command submits a bitmap item to the MPEG video device. The
|||	    device then places a decoded frame of MPEG video into the bitmap's
|||	    buffer.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to MPEGVIDEOCMD_READ.
|||
|||	    ioi_Send.iob_Buffer
|||	        Pointer to a bitmap Item. The bitmap item must have been
|||	        created using BMTAG_MPEGABLE set to TRUE.
|||
|||	    ioi_Send.iob_Len
|||	        Set to sizeof( Item );
|||
|||	  When the request has completed the following IOReq fields may be set:
|||
|||	    io_Flags
|||	        The 0x80000000 bit is set if a presentation time stamp is valid for
|||	        this read request.
|||
|||	    io_Extension[ 0 ]
|||	        Contains the presentation time stamp for this read request if the
|||	        above bit is set. Can be accessed using the FMVGetPTSValue macro.
|||
|||	    io_Info.ioi_CmdOptions
|||	        Contains the user data associated with this read request. Can be
|||	        accessed using the FMVGetUserData macro.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/mpegvideo.h>, <graphics/bitmap.h>
|||
|||	  See Also
|||
|||	    MPEGVIDEOCMD_WRITE(@), MPEGVIDEOCMD_CONTROL(@)
|||
**/

/**
|||	AUTODOC -class Device_Commands -group MPEG -name MPEGVIDEOCMD_CONTROL
|||	Permits application control over MPEG video decoding.
|||
|||	  Description
|||
|||	    This command submits one or more control request to the MPEG video
|||	    device. A TagArg list is used to set various parameters and modes.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to MPEGVIDEOCMD_CONTROL
|||
|||	    ioi_Send.iob_Buffer
|||	        Pointer to a CODECDeviceStatus structure.
|||
|||	    ioi_Send.iob_Len
|||	        Set to sizeof( CODECDeviceStatus );
|||
|||	    The following control requests may be submitted in the codec_TagArg
|||	    list in the CODECDeviceStatus structure.
|||
|||	    VID_CODEC_TAG_HSIZE
|||	        This request sets the horizontal output size. The associated ta_Arg
|||	        should be set to the horizontal output size and should be a multiple
|||	        of 16.
|||
|||	    VID_CODEC_TAG_VSIZE
|||	        This request sets the veritcal output size. The associated ta_Arg
|||	        should be set to the vertical output size and should be a multiple
|||	        of 16.
|||
|||	    VID_CODEC_TAG_DEPTH
|||	        This request allows selection of 16 or 32  bit output formats.
|||	        The associated ta_Arg should be set to 16 or 32.
|||
|||	    VID_CODEC_TAG_M2MODE
|||	        This request selects M2 output modes and should ALWAYS be set for
|||	        M2 titles.
|||
|||	    VID_CODEC_TAG_KEYFRAMES
|||	        This request puts the device into I frame only mode. After this
|||	        request is sent the device will only decode and return MPEG I
|||	        pictures. Other pictures in the input bitstream are skipped.
|||	        Use VID_CODEC_TAG_PLAY to return to normal play mode.
|||
|||	    VID_CODEC_TAG_PLAY
|||	        This request puts the device into normal play mode. After this
|||	        request is sent the device will decode and return all MPEG
|||	        pictures in the input bitstream. Used to exit I frame only mode.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/mpegvideo.h>
|||
|||	  See Also
|||
|||	    MPEGVIDEOCMD_READ(@), MPEGVIDEOCMD_WRITE(@)
|||
**/

/**
|||	AUTODOC -class Device_Commands -group MPEG -name MPEGVIDEOCMD_ALLOCBUFFERS
|||	Directs the MPEG video device to pre-allocate fixed-size decode buffers.
|||
|||	  Description
|||
|||	    This command provides complete control over the memory usage 
|||	    of the MPEG video device.  When different image sizes are 
|||	    encountered, the device normally allocates bigger or smaller 
|||	    buffers as needed, possibly leading to memory fragmentation.
|||	    This command directs the driver to pre-allocate buffers at
|||	    a given size and prevents the driver from reallocating the
|||	    buffers internally when different-sized images are processed.
|||
|||	    The MPEG video device uses two types of decode buffers; a strip
|||	    buffer is used for processing all images, and a reference buffer
|||	    is used for processing moving images.  You can fill in the fields
|||	    of an MPEGBufferStreamInfo structure to indicate the largest 
|||	    still image and largest moving image you will ever send to the
|||	    device, and pass that structure to this command to have decode
|||	    buffers allocated based on those values.  If you process only
|||	    still images, set the fmvWidth and fmvHeight fields to zero.  
|||	    If you process only moving images, set the stillWidth and 
|||	    stillHeight fields to zero.
|||
|||	    Once the device has allocated buffers based on the values sent
|||	    with this command, it will not reallocate the buffers based on
|||	    changing image sizes.  If you send the device an image too big
|||	    to fit into the buffers you specified, the IOReq for the 
|||	    MPEGVIDEOCMD_WRITE that contains the image's video sequence 
|||	    header will be returned to you with a NOMEM status in the 
|||	    io_Error field to indicate that the buffers aren't big enough.  
|||	    Subsequent MPEGVIDEOCMD_WRITE IOReqs that don't contain video 
|||	    sequence headers will be flushed (returned with io_Error set 
|||	    to zero, but without processing the data).
|||
|||	    The MPEG video device does not internally allocate any buffers
|||	    until it sees the first video sequence header.  The best way to
|||	    ensure that memory fragmentation can't happen is to send this 
|||	    command immediately after opening the device, before sending 
|||	    any image data.  
|||
|||	    You can return the device to its normal internal buffer 
|||	    allocation mode by sending this command with an MPEGBufferStreamInfo
|||	    structure that has all the fields set to zero.  In this case,
|||	    any currently-allocated buffers are released before the IOReq
|||	    is returned to you as completed.  Buffers will not be reallocated 
|||	    until a new video sequence header is seen.  You can do this even
|||	    if the driver is already in its normal internal-allocation mode.
|||	    This provides you with a way to get the device to free its 
|||	    internal buffers, making the memory available to your application.
|||	    (But make sure you free such memory back to the system free pool
|||	    before sending any new image data to the device, or before sending
|||	    this command with non-zero values, so it can reallocate its buffers.)
|||
|||	    If you had previously supplied specific buffer memory to the 
|||	    device using MPEGVIDEOCMD_SETSTRIPBUFFER(@) or 
|||	    MPEGVIDEOCMD_SETREFERENCEBUFFER(@), this command will cause the 
|||	    IOReq(s) for the SET command(s) to be returned with a status 
|||	    of ABORTED in the io_Error field.  That is, this command 
|||	    overrides the prior SET command(s) and returns the prior buffer
|||	    memory to you.
|||
|||	    This command flushes the device; the device's internal state is
|||	    reset so that it will flush any incoming data until it sees the 
|||	    next video sequence header.  This prevents any attempt to use
|||	    data in a reference buffer that is no longer valid.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to MPEGVIDEOCMD_ALLOCBUFFERS.
|||
|||	    ioi_Send.iob_Buffer
|||	        Pointer to an MPEGBufferStreamInfo structure describing the
|||	        largest image sizes you intend to process.
|||
|||	    ioi_Send.iob_Len
|||	        Set to the size of the MPEGBufferStreamInfo structure.
|||
|||	    ioi_Recv.iob_Buffer
|||	        Optional pointer to an MPEGBufferInfo structure.  If you
|||	        specify this pointer the device will return information
|||	        about the sizes of the buffers it allocated internally.  If you
|||	        don't need this information, set ioi_Recv.iob_Buffer to NULL.
|||
|||	    ioi_Recv.iob_Buffer
|||	        Set to the size of the MPEGBufferInfo structure if you set
|||	        ioi_Recv.iob_Buffer.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V33.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/mpegvideo.h>
|||
|||	  See Also
|||
|||	    MPEGVIDEOCMD_GETBUFFERINFO(@), MPEGVIDEOCMD_SETSTRIPBUFFER(@),
|||	    MPEGVIDEOCMD_SETREFERENCEBUFFER(@)
**/

/**
|||	AUTODOC -class Device_Commands -group MPEG -name MPEGVIDEOCMD_GETBUFFERINFO
|||	Returns information about buffer allocation requirements.
|||
|||	  Description
|||
|||	    This command returns information about buffer allocation requirements.
|||
|||	    The MPEG video device uses two types of decode buffers; a strip
|||	    buffer is used for processing all images, and a reference buffer
|||	    is used for processing moving images.  You can fill in the fields
|||	    of an MPEGBufferStreamInfo structure to indicate the largest 
|||	    still image and largest moving image you will ever send to the
|||	    device, and pass that structure to this command to have buffer 
|||	    allocation information calculated.  Based on that information,
|||	    you can allocate buffers and hand them off to the device using the
|||	    MPEGVIDEOCMD_SETSTRIPBUFFER(@) and MPEGVIDEOCMD_SETREFERENCEBUFFER(@)
|||	    commands. 
|||
|||	    If you process only still images, set the fmvWidth and fmvHeight 
|||	    fields to zero.  If you process only moving images, set the 
|||	    stillWidth and stillHeight fields to zero.
|||
|||	    The MPEG hardware has very specific buffer alignment requirements.
|||	    When allocating buffers based on the information returned by this
|||	    command, you must use AllocMemMasked() passing the memCareBits and
|||	    memStateBits values from the returned MPEGBufferInfo structure.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to MPEGVIDEOCMD_GETBUFFERINFO.
|||
|||	    ioi_Send.iob_Buffer
|||	        Pointer to an MPEGBufferStreamInfo structure describing the
|||	        largest image sizes you intend to process.
|||
|||	    ioi_Send.iob_Len
|||	        Set to the size of the MPEGBufferStreamInfo structure.
|||
|||	    ioi_Recv.iob_Buffer
|||	        Pointer to an MPEGBufferInfo structure where the results of the
|||	        buffer allocation calculations will be stored.
|||
|||	    ioi_Recv.iob_Buffer
|||	        Set to the size of the MPEGBufferInfo structure.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V33.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/mpegvideo.h>
|||
|||	  See Also
|||
|||	    MPEGVIDEOCMD_ALLOCBUFFERS(@), MPEGVIDEOCMD_SETSTRIPBUFFER(@),
|||	    MPEGVIDEOCMD_SETREFERENCEBUFFER(@)
**/

/**
|||	AUTODOC -class Device_Commands -group MPEG -name MPEGVIDEOCMD_SETSTRIPBUFFER
|||	Provides a strip buffer for the MPEG video device to use.
|||
|||	  Description
|||
|||	    This command provides a strip buffer for the MPEG video device to use.
|||
|||	    The MPEG video device uses two types of decode buffers; a strip
|||	    buffer is used for processing all images, and a reference buffer
|||	    is used for processing moving images.  Normally, the device 
|||	    automatically allocates a strip buffer for itself.  This command
|||	    allows you to provide a buffer for the device to use.
|||
|||	    The MPEG hardware has very specific buffer alignment requirements.
|||	    Use the MPEGVIDEOCMD_GETBUFFERINFO(@) command to obtain information
|||	    about the buffer allocation requirements based on the size of the
|||	    images you'll be processing.  When allocating buffers based on the 
|||	    information returned by the MPEGVIDEOCMD_GETBUFFERINFO command, you 
|||	    must use AllocMemMasked() passing the memCareBits and memStateBits 
|||	    values from the returned MPEGBufferInfo structure.
|||
|||	    You must use SendIO() (not DoIO()) on the IOReq for this command. If
|||	    you use DoIO() or WaitIO() on this IOReq your task will block forever.
|||	    The device will retain the IOReq while it is using the buffer.  You
|||	    must not use the buffer memory yourself while the device is holding
|||	    the IOReq.  To reclaim the memory for your own use, call AbortIO()
|||	    on the IOReq.  You may reclaim the memory, use it for your own
|||	    purposes for a while (not sending any image data to the device 
|||	    while you're using the memory), then give the buffer back to the
|||	    device again using this command before sending new image data.
|||
|||	    Once you have provided the device with a strip buffer using
|||	    this command, it will not reallocate the buffer based on
|||	    changing image sizes.  If you send the device an image too big
|||	    to fit into the buffer you provided, the IOReq for the 
|||	    MPEGVIDEOCMD_WRITE that contains the image's video sequence 
|||	    header will be returned to you with a NOMEM status in the 
|||	    io_Error field to indicate that the buffer isn't big enough.  
|||	    Subsequent MPEGVIDEOCMD_WRITE IOReqs that don't contain video 
|||	    sequence headers will be flushed (returned with io_Error set 
|||	    to zero, but without processing the data).
|||
|||	    If you reclaim the buffer using AbortIO() and then send image 
|||	    data to the device, it will behave as described above for a 
|||	    too-small buffer.  The only way to get the device back into its
|||	    normal self-allocation mode is to use MPEGVIDEOCMD_ALLOCBUFFERS
|||	    with zeroes specified for stillWidth and stillHeight.
|||
|||	    If you free the buffer memory while the device is using the buffer,
|||	    the IOReq will be returned to you with io_Error set to ABORTED and
|||	    the device will behave as described above for a too-small buffer.
|||
|||	    This command flushes the device; the device's internal state is
|||	    reset so that it will flush any incoming data until it sees the 
|||	    next video sequence header.  This prevents any attempt to use
|||	    data in a buffer that is no longer valid.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to MPEGVIDEOCMD_SETSTRIPBUFFER.
|||
|||	    ioi_Recv.iob_Buffer
|||	        Pointer to a memory buffer allocated based on the values 
|||	        returned by MPEGVIDEOCMD_GETBUFFERINFO.
|||
|||	    ioi_Recv.iob_Buffer
|||	        Set to the size of the buffer.
|||
|||	  Note
|||
|||	    This command does not actually send or receive any data, it just 
|||	    provides a block of memory for the device to use as an internal
|||	    working buffer.  The buffer is provided via the ioi_Recv fields
|||	    in the IOInfo structure because this allows the system to validate
|||	    your ownership of the memory block, and also allows the system to
|||	    automatically abort the the IOReq and inform the device if you
|||	    free the memory before reclaiming the IOReq from the device.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V33.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/mpegvideo.h>
|||
|||	  See Also
|||
|||	    MPEGVIDEOCMD_ALLOCBUFFERS(@), MPEGVIDEOCMD_GETBUFFERINFO(@),
|||	    MPEGVIDEOCMD_SETREFERENCEBUFFER(@)
**/

/**
|||	AUTODOC -class Device_Commands -group MPEG -name MPEGVIDEOCMD_SETREFERENCEBUFFER
|||	Provides a reference buffer for the MPEG video device to use.
|||
|||	  Description
|||
|||	    This command provides a reference buffer for the MPEG video device to use.
|||
|||	    The MPEG video device uses two types of decode buffers; a strip
|||	    buffer is used for processing all images, and a reference buffer
|||	    is used for processing moving images.  Normally, the device 
|||	    automatically allocates a reference buffer for itself.  This command
|||	    allows you to provide a buffer for the device to use.  You do not
|||	    need to provide a reference buffer if you are processing only still
|||	    images  (I.E., if you have set the VID_CODEC_TAG_KEYFRAMES option.)
|||
|||	    The MPEG hardware has very specific buffer alignment requirements.
|||	    Use the MPEGVIDEOCMD_GETBUFFERINFO(@) command to obtain information
|||	    about the buffer allocation requirements based on the size of the
|||	    images you'll be processing.  When allocating buffers based on the 
|||	    information returned by the MPEGVIDEOCMD_GETBUFFERINFO command, you 
|||	    must use AllocMemMasked() passing the memCareBits and memStateBits 
|||	    values from the returned MPEGBufferInfo structure.
|||
|||	    You must use SendIO() (not DoIO()) on the IOReq for this command. If
|||	    you use DoIO() or WaitIO() on this IOReq your task will block forever.
|||	    The device will retain the IOReq while it is using the buffer.  You
|||	    must not use the buffer memory yourself while the device is holding
|||	    the IOReq.  To reclaim the memory for your own use, call AbortIO()
|||	    on the IOReq.  You may reclaim the memory, use it for your own
|||	    purposes for a while (not sending any image data to the device 
|||	    while you're using the memory), then give the buffer back to the
|||	    device again using this command before sending new image data.
|||
|||	    Once you have provided the device with a reference buffer using
|||	    this command, it will not reallocate the buffer based on
|||	    changing image sizes.  If you send the device an image too big
|||	    to fit into the buffer you provided, the IOReq for the 
|||	    MPEGVIDEOCMD_WRITE that contains the image's video sequence 
|||	    header will be returned to you with a NOMEM status in the 
|||	    io_Error field to indicate that the buffer isn't big enough.  
|||	    Subsequent MPEGVIDEOCMD_WRITE IOReqs that don't contain video 
|||	    sequence headers will be flushed (returned with io_Error set 
|||	    to zero, but without processing the data).
|||
|||	    If you reclaim the buffer using AbortIO() and then send image 
|||	    data to the device, it will behave as described above for a 
|||	    too-small buffer.  The only way to get the device back into its
|||	    normal self-allocation mode is to use MPEGVIDEOCMD_ALLOCBUFFERS
|||	    with zeroes specified for stillWidth and stillHeight.
|||
|||	    If you free the buffer memory while the device is using the buffer,
|||	    the IOReq will be returned to you with io_Error set to ABORTED and
|||	    the device will behave as described above for a too-small buffer.
|||
|||	    This command flushes the device; the device's internal state is
|||	    reset so that it will flush any incoming data until it sees the 
|||	    next video sequence header.  This prevents any attempt to use
|||	    data in a buffer that is no longer valid.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to MPEGVIDEOCMD_SETREFERENCEBUFFER.
|||
|||	    ioi_Recv.iob_Buffer
|||	        Pointer to a memory buffer allocated based on the values 
|||	        returned by MPEGVIDEOCMD_GETBUFFERINFO.
|||
|||	    ioi_Recv.iob_Buffer
|||	        Set to the size of the buffer.
|||
|||	  Note
|||
|||	    This command does not actually send or receive any data, it just 
|||	    provides a block of memory for the device to use as an internal
|||	    working buffer.  The buffer is provided via the ioi_Recv fields
|||	    in the IOInfo structure because this allows the system to validate
|||	    your ownership of the memory block, and also allows the system to
|||	    automatically abort the the IOReq and inform the device if you
|||	    free the memory before reclaiming the IOReq from the device.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V33.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/mpegvideo.h>
|||
|||	  See Also
|||
|||	    MPEGVIDEOCMD_ALLOCBUFFERS(@), MPEGVIDEOCMD_GETBUFFERINFO(@),
|||	    MPEGVIDEOCMD_SETSTRIPBUFFER(@)
**/

/**
|||	AUTODOC -class Device_Commands -group Generic -name CMD_STATUS
|||	Requests the DeviceStatus information for a device.
|||
|||	  Description
|||
|||	    This command causes the requested device to return its
|||	    DeviceStatus information.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to CMD_STATUS.
|||
|||	    ioi_Recv.iob_Buffer
|||	        Pointer to a DeviceStatus struct.
|||
|||	    ioi_Recv.iob_Len
|||	        Set to sizeof(DeviceStatus).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>
|||
**/

/**
|||	AUTODOC -class Device_Commands -group Generic -name CMD_BLOCKREAD
|||	Reads data from a block-oriented device.
|||
|||	  Description
|||
|||	    This command causes the requested device to return data.
|||	    The device must be "block-oriented".  This means that the
|||	    device data is divided into fixed sized "blocks".  The
|||	    size of the blocks on the device is available from the
|||	    .ds_DeviceBlockSize field of the DeviceStatus structure.
|||	    If the .ds_DeviceBlockSize field is zero, the device is
|||	    not a block-oriented device.
|||
|||	    The data on the device is addressible: each block of data
|||	    has a "block number", and each request to read data
|||	    specifies the block number of the first block of data to
|||	    be read (in ioi_Offset).  Block numbers start at zero.
|||	    However, blocks starting at zero are not necessarily
|||	    accessible; the first accessible block number is given
|||	    by the .ds_DeviceBlockStart field in the DeviceStatus
|||	    structure.  The number of blocks on the device is given
|||	    by the .ds_DeviceBlockCount field in the DeviceStatus
|||	    structure.  This block count includes all blocks, beginning
|||	    with block zero, even if some of the initial blocks are
|||	    not accessible.  Thus, the accessible block numbers are
|||	    those between .ds_DeviceBlockStart and .ds_DeviceBlockCount-1,
|||	    inclusive.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to CMD_BLOCKREAD.
|||
|||	    ioi_Recv.iob_Buffer
|||	        Pointer to a buffer where the data is to be stored.
|||
|||	    ioi_Recv.iob_Len
|||	        Set to the size of the buffer pointed to by
|||	        ioi_Recv.iob_Buffer.  No more data than this will
|||	        be stored in the receive buffer.  Less data may be
|||	        returned, if less data is available from the device,
|||	        or if the size of the buffer is not a multiple of
|||	        the device block size.  The actual amount of data
|||	        written into the receive buffer is returned in
|||	        io_Actual.
|||
|||	    ioi_Offset
|||	        Set to the block number of the first block of data
|||	        to be read from the device.
|||
|||	  Note
|||
|||	    The data returned by CMD_BLOCKREAD may not be present in physical
|||	    memory and may only appear in the CPU cache. Therefore, before
|||	    processing data read from a file using a DMA device, you must
|||	    first write back the data to main memory using WriteBackDCache()
|||	    or FlushDCacheAll().
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    CMD_BLOCKWRITE(@), CMD_STREAMREAD(@), CMD_STREAMWRITE(@),
|||	    CDROMCMD_READ(@)
**/

/**
|||	AUTODOC -class Device_Commands -group Generic -name CMD_BLOCKWRITE
|||	Writes data to a block-oriented device.
|||
|||	  Description
|||
|||	    This command sends data to be stored on the device.
|||	    The device must be "block-oriented" (see CMD_BLOCKREAD(@))
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to CMD_BLOCKWRITE.
|||
|||	    ioi_Send.iob_Buffer
|||	        Pointer to a buffer containing the data to be written.
|||
|||	    ioi_Send.iob_Len
|||	        Set to the size of the buffer pointed to by
|||	        ioi_Recv.iob_Buffer.  Less than the full amount of
|||	        data in the buffer may be written to the device, if
|||	        the device does not have the capacity to store the
|||	        full amount of data, or if the buffer is not a multiple
|||	        of the device block size.  The actual amount of data
|||	        written to the device is returned in io_Actual.
|||
|||	    ioi_Offset
|||	        Set to the block number of the first block of data
|||	        to be written to the device.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    CMD_BLOCKREAD(@), CMD_STREAMWRITE(@), CMD_STREAMREAD(@)
**/

/**
|||	AUTODOC -class Device_Commands -group Generic -name CMD_STREAMREAD
|||	Reads data from a stream-oriented device.
|||
|||	  Description
|||
|||	    This command reads data from the associated device. The data
|||	    is read sequentially, as it comes in. The command normally waits
|||	    until enough data arrives to satisfy the request completely,
|||	    although a timeout value can be supplied to cause early
|||	    termination.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to CMD_STREAMREAD.
|||
|||	    ioi_CmdOptions
|||	        The minimum number of microseconds before a timeout occurs.
|||	        If more than this amount of time passes between two bytes of
|||	        data being received, the IOReq is returned to the caller.
|||	        The number of bytes actually read is available in io_Actual as
|||	        usual. If this ioi_CmdOptions is set to 0, it means that no
|||	        timeout is desired.
|||
|||	    ioi_Recv.iob_Buffer
|||	        Pointer to a buffer where the data is to be stored.
|||
|||	    ioi_Recv.iob_Len
|||	        Set to the number of bytes to read into the buffer.
|||	        No more data than this will be stored in the receive buffer.
|||	        Less data may be returned, if less data is available from the
|||	        device or if a timeout occurs. The actual amount of data
|||	        put into the receive buffer is returned in io_Actual.
|||
|||	  Note
|||
|||	    The data returned by CMD_STREAMREAD may not be present in physical
|||	    memory and may only appear in the CPU cache. Therefore, before
|||	    processing data read from a file using a DMA device, you must
|||	    first write back the data to main memory using WriteBackDCache()
|||	    or FlushDCacheAll().
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    CMD_STREAMWRITE(@), CMD_BLOCKREAD(@), CMD_BLOCKWRITE(@)
**/

/**
|||	AUTODOC -class Device_Commands -group Generic -name CMD_STREAMWRITE
|||	Writes data to a stream-oriented device.
|||
|||	  Description
|||
|||	    This command writes data to the device in a stream-oriented manner.
|||
|||	    Note that this command may simply add data to a buffer maintained
|||	    by the driver or hardware. You can use the CMD_STREAMFLUSH(@)
|||	    command to guarantee that any buffered bytes are actually sent out.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to CMD_STREAMWRITE.
|||
|||	    ioi_Send.iob_Buffer
|||	        Pointer to a buffer containing the data to be written.
|||
|||	    ioi_Send.iob_Len
|||	        Set to the number of bytes to output from the send buffer.
|||	        As much data as possible is written out, although a device
|||	        can fill up or refuse too much data. The actual number of bytes
|||	        written is returned in io_Actual.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    CMD_STREAMFLUSH(@), CMD_STREAMREAD(@), CMD_BLOCKREAD(@),
|||	    CMD_BLOCKWRITE(@)
**/

/**
|||	AUTODOC -class Device_Commands -group Generic -name CMD_STREAMFLUSH
|||	Flushes any pending data to a stream-oriented device.
|||
|||	  Description
|||
|||	    This command flushes any pending data to the output stream. When
|||	    this command returns, any data that had been written to the stream
|||	    previously is guaranteed to have made it out.
|||
|||	    Note that this command only guarantees that write operations with
|||	    a priority >= to the priority of the CMD_STREAMFLUSH IOReq have
|||	    been flushed. It is possible for lower priority IOReq to not have
|||	    made it out. To ensure that all data is flushed, you must set the
|||	    priority of the CMD_STREAMFLUSH IOReq to 0.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to CMD_STREAMFLUSH.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V31.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    CMD_STREAMWRITE(@)
**/

/**
|||	AUTODOC -class Device_Commands -group Generic -name CMD_GETMAPINFO
|||	Returns information about the memory-mapping capabilities of a device.
|||
|||	  Description
|||
|||	    This command causes the requested device to return a
|||	    MemMappableDeviceInfo structure.  This structure describes
|||	    the capabilities of the device to map itself into memory
|||	    space, allowing software to access the device directly
|||	    via memory references.  The .mmdi_Flags field describes
|||	    the basic memory-mapping capabilities:
|||
|||	       MM_MAPPABLE
|||	          The device is mappable into memory.
|||	       MM_READABLE
|||	          The device can be read while mapped, via memory
|||	          reads.
|||	       MM_WRITABLE
|||	          The device can be written to while mapped, via
|||	          memory writes.
|||	       MM_EXECUTABLE
|||	          Code can be executed from the device while mapped.
|||	       MM_EXCLUSIVE
|||	          The device supports mapping in "exclusive" mode.
|||	          See CMD_MAPRANGE(@).
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to CMD_GETMAPINFO.
|||
|||	    ioi_Recv.iob_Buffer
|||	        Pointer to a MemMappableDeviceInfo structure.
|||
|||	    ioi_Recv.iob_Len
|||	        Set to sizeof(MemMappableDeviceInfo).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <kernel/driver.h>
|||
|||	  See Also
|||
|||	    CMD_MAPRANGE(@), CMD_UNMAPRANGE(@)
**/

/**
|||	AUTODOC -class Device_Commands -group Generic -name CMD_MAPRANGE
|||	Requests a device to map itself into memory.
|||
|||	  Description
|||
|||	    This command causes the requested device to map itself into
|||	    memory address space.  After successful completion of this
|||	    command, the device can be accessed simply by accessing
|||	    the appropriate memory addresses.  A MapRangeRequest
|||	    structure is sent to the device in the .ioi_Send buffer.
|||	    A MapRangeResponse structure is returned from the device
|||	    in the .ioi_Recv buffer.
|||
|||	    The .ioi_Offset field specifies the starting offset within
|||	    the device of the first byte to be mapped.  The
|||	    .mrr_BytesToMap field of the MemMapRequest structure
|||	    specifies the requested number of bytes to be mapped.
|||	    More than .mrr_BytesToMap bytes may be mapped, but the
|||	    caller should not depend this.  The memory address where
|||	    the device has been mapped is returned in the
|||	    .mrr_MappedArea field of the MemMapResponse structure.
|||
|||	    The bits in the .mrr_Flags field of the MemMapRequest
|||	    structure specifies how the mapped area will be used.  See
|||	    CMD_GETMAPINFO(@) for a description of the meanings of these
|||	    bits.  Only those bits which are set in the .mmdi_Flags
|||	    field of the device's MemMappableDeviceInfo structure may
|||	    be set in the .mrr_Flags field of a MemMapRequest.
|||	    If the MM_EXCLUSIVE bit is set in the MemMapRequest, the
|||	    CMD_MAPRANGE(@) command will succeed only if the device is not
|||	    currently mapped.  Furthermore, after such a mapping succeeds,
|||	    other CMD_MAPRANGE(@) commands will fail until the first mapping
|||	    is removed with a CMD_UNMAPRANGE(@) command.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to CMD_MAPRANGE
|||
|||	    ioi_Offset
|||	        Offset within the device of the first byte to be mapped.
|||
|||	    ioi_Send.iob_Buffer
|||	        Pointer to a MapRangeRequest structure.
|||
|||	    ioi_Send.iob_Len
|||	        Set to sizeof(MapRangeRequest).
|||
|||	    ioi_Recv.iob_Buffer
|||	        Pointer to a MapRangeResponse structure.
|||
|||	    ioi_Recv.iob_Len
|||	        Set to sizeof(MapRangeResponse).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <kernel/driver.h>
|||
|||	  See Also
|||
|||	    CMD_GETMAPINFO(@), CMD_UNMAPRANGE(@)
**/

/**
|||	AUTODOC -class Device_Commands -group Generic -name CMD_UNMAPRANGE
|||	Requests a device to unmap itself from memory.
|||
|||	  Description
|||
|||	    This command undoes the effect of a previous CMD_MAPRANGE(@)
|||	    command.  After successful completion of this command, a
|||	    mapping previously set up by a CMD_MAPRANGE(@) command is
|||	    no longer valid, and the memory addresses which previously
|||	    referred to the device may no longer be accessed.  A
|||	    MapRangeRequest structure is sent to the device in the
|||	    .ioi_Send buffer.
|||
|||	    The .ioi_Offset field must be identical to the .ioi_Offset
|||	    of the CMD_MAPRANGE(@) which originally set up the mapping.
|||	    The .mrr_BytesToMap field and the .mrr_Flags field of the
|||	    MemMapRequest structure must likewise be identical to the
|||	    corresponding fields of the of the MemMapRequest structure
|||	    which originally set up the mapping.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to CMD_UNMAPRANGE.
|||
|||	    ioi_Offset
|||	        Offset within the device of the first byte of the
|||	        mapping to be undone.
|||
|||	    ioi_Send.iob_Buffer
|||	        Pointer to a MapRangeRequest structure.
|||
|||	    ioi_Send.iob_Len
|||	        Set to sizeof(MapRangeRequest).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <kernel/driver.h>
|||
|||	  See Also
|||
|||	    CMD_MAPRANGE(@), CMD_GETMAPINFO(@)
**/

/**
|||	AUTODOC -class Device_Commands -group Generic -name CMD_PREFER_FSTYPE
|||	Requests the preferred filesystem type for a device.
|||
|||	  Description
|||
|||	    This command causes the requested device to return the
|||	    type of filesystem with which the device would prefer
|||	    to be formatted.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to CMD_PREFER_FSTYPE.
|||
|||	    ioi_Recv.iob_Buffer
|||	        Pointer to a uint32 which receives the filesystem type.
|||	        You can use this type when formatting the device using
|||	        FormatFileSystem().
|||
|||	    ioi_Recv.iob_Len
|||	        Set to sizeof(uint32).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <file/filesystem.h>
|||
|||	  Notes
|||
|||	    This command is meaningful only for devices which have
|||	    the DS_USAGE_FILESYSTEM bit set in the .ds_DeviceUsageFlags
|||	    file of their DeviceStatus.
|||
|||	  See Also
|||
|||	    CMD_STATUS(@)
|||
**/

/**
|||	AUTODOC -class Device_Commands -group CD-ROM -name CDROMCMD_READ
|||	Requests specific block (sector) data from the disc.
|||
|||	  Description
|||
|||	    This command causes the requested sector(s) to be returned.  This
|||	    command is similar to CMD_BLOCKREAD(@); but allows various options
|||	    to be supplied in the ioi_CmdOptions field.  The block size is
|||	    generally defined by the media present.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to CDROMCMD_READ.
|||
|||	    ioi_Recv.iob_Buffer
|||	        Pointer to a receive buffer.  Optimum performance will be
|||	        attained if the buffer is 4-byte alligned.
|||
|||	    ioi_Recv.iob_Len
|||	        The length of the receive buffer must be a multiple of the
|||	        block size.  Again, this is dependent on the current media.
|||	        For the pre-defined block lengths, see
|||	        ioi_CmdOptions.asFields.blockLength below.
|||
|||	    ioi_Offset
|||	        This is the starting address of the sector(s) you wish to
|||	        read.  It can be in either of the following formats:  1) the
|||	        absolute sector address (e.g., sector 472903); or 2) the
|||	        address of the sector in binary MSF format.
|||
|||	    ioi_CmdOptions
|||	        This field provides access to parameters which define how to
|||	        perform this read.  The options are provided as bitfields of
|||	        this 32-bit word.  Specifying a value of zero for each field
|||	        means you wish to use the current default.  The system defaults
|||	        are indicated below with '*'.  The fields are accessed by
|||	        specifying 'ioi_CmdOptions.asFields' before the following:
|||
|||	    .densityCode
|||	        Specifies what type of media you are reading; and is one
|||	        of the following:
|||
|||	      NULL
|||	          Use the current default.
|||
|||	      CDROM_DEFAULT_DENSITY*
|||	          Indicates that you wish to read Mode1, Mode2Form1, or
|||	          Mode2Form2 type sectors.  Do whatever is necessary to return
|||	          the correct data.
|||
|||	      CDROM_DATA
|||	          Indicates that you wish to read Mode1 sectors.  If a
|||	          different sector type exists an error is returned.
|||
|||	      CDROM_MODE2_XA
|||	          Indicates that you wish to read Mode2Form1 or Mode2Form2
|||	          sectors.  If a different sector type exists an error is
|||	          returned.
|||
|||	      CDROM_DIGITAL_AUDIO
|||	          Indicates that you wish to read RedBook Audio sectors data.
|||
|||
|||	    .addressFormat
|||	        Indicates what address format is specified in ioi_Offset.
|||	        Valid values are:
|||
|||	      NULL
|||	          Use the current default.
|||
|||	      CDROM_Address_Blocks*
|||	          Indicates that the sector address specified in ioi_Offset is
|||	          an absolute block/sector number.
|||
|||	      CDROM_Address_Abs_MSF
|||	          Indicates that the sector address specified in ioi_Offset is
|||	          specified in binary MSF format.  That is, a 32-bit value
|||	          (0x00MMSSFF) in which each field (M,S,F) is binary (as
|||	          opposed to Binary Coded Decimal).
|||
|||
|||	    .errorRecovery
|||	        Specifies what type of error recovery you would like performed;
|||	        and whether or not any errored sector data is returned.  Valid
|||	        values are:
|||
|||	      NULL
|||	          Use the current default.
|||
|||	      CDROM_DEFAULT_RECOVERY*
|||	          Perform ECC on any errored sector.  If ECC fails, retry the
|||	          read.  Continue for the number of retries specified by
|||	          .retryShift.  If unable to obtain the sector cleanly, return
|||	          an error.  NOTE:  Does not return the errored sector data.
|||
|||	      CDROM_CIRC_RETRIES_ONLY
|||	          Perform retries only (no ECC).  If retries fail, return an
|||	          error.  NOTE:  This does return the errored sector data.
|||
|||	      CDROM_BEST_ATTEMPT_RECOVERY
|||	          Perform ECC and retries.  If both fail, return an error AND
|||	          the errored sector.  (Similar to CDROM_DEFAULT_RECOVERY.)
|||
|||
|||	    .retryShift
|||	        Specifies the number of retries that the driver attempts for a
|||	        given sector when an error is detected.  Upon exceeding this
|||	        count, the driver returns an error.  If the .errorRecovery
|||	        field contained CDROM_BEST_ATTEMPT_RECOVERY, then the errored
|||	        sector data is returned as well.
|||
|||	        The value (n) specified indicates that you wish to perform
|||	        (2^n - 1) retries.  Valid values for n range from 0 (zero
|||	        retries) to 7 (127 retries).  The default value is 3 (7
|||	        retries).
|||
|||
|||	    .speed
|||	        Specifies the speed at which the data is to be read.  Some
|||	        mechanisms may support 4x, 6x, and 8x speeds.  Others return
|||	        an error.  Valid values are:
|||
|||	      NULL
|||	          Use the current default.
|||
|||	      CDROM_SINGLE_SPEED
|||	          Operate the mechanism at single speed.
|||
|||	      CDROM_DOUBLE_SPEED*
|||	          Operate the mechanism at double speed.
|||
|||	      CDROM_4X_SPEED
|||	          Operate the mechanism at 4x speed.
|||
|||	      CDROM_6X_SPEED
|||	          Operate the mechanism at 6x speed.
|||
|||	      CDROM_8X_SPEED
|||	          Operate the mechanism at 8x speed.
|||
|||
|||	    .pitch
|||	        Specifies the variable pitch component of the speed.  Note that
|||	        this is only valid when operating in CDROM_SINGLE_SPEED.  Also
|||	        note that some mechanisms may not contain the hardware to
|||	        support variable pitch.  In such a case NO error is returned;
|||	        and the data is read in normal pitch.  Valid values are:
|||
|||	      NULL
|||	          Use the current default.
|||
|||	      CDROM_PITCH_SLOW
|||	          Operate drive at -1% of single speed.
|||
|||	      CDROM_PITCH_NORMAL
|||	          Operate drive at single speed.
|||
|||	      CDROM_PITCH_FAST
|||	          Operate drive at +1% of single speed.
|||
|||
|||	    .blockLength
|||	        Specifies the block length that you are interested in reading
|||	        from the disc sector.  Note that this value is dependent upon
|||	        the media present.  The pre-defined values are:
|||
|||	      NULL
|||	          Use the current default.
|||
|||	      CDROM_AUDIO
|||	          Returns 2352 bytes of audio data for each sector.
|||
|||	      CDROM_AUDIO_SUBCODE
|||	          Returns 2352 bytes of (RedBook) Audio data, followed by 96
|||	          bytes of Subcode, for EACH sector.  NOTE:  The subcode
|||	          returned with each sector is only LOOSELY associated with the
|||	          sector returned.  The subcode is NOT synced up with the data
|||	          for each sector.
|||
|||	      CDROM_MODE1*
|||	          Returns 2048 bytes of Mode1 (YellowBook) sector data.
|||
|||	      CDROM_MODE2FORM1
|||	          Returns 2048 bytes of Mode2Form1 (OrangeBook) sector data.
|||
|||	      CDROM_MODE2FORM1_SUBHEADER
|||	          Returns 8 bytes of SubHeader, followed by 2048 bytes of
|||	          Mode2Form1 (OrangeBook) sector data for EACH sector.
|||
|||	      CDROM_MODE2FORM2
|||	          Returns 2324 bytes of Mode2Form2 (OrangeBook) sector data.
|||
|||	      CDROM_MODE2FORM2_SUBHEADER
|||	          Returns 8 bytes of SubHeader, followed by 2324 bytes of
|||	          Mode2Form2 (OrangeBook) sector data for EACH sector.
|||
|||	      Note that other valid block lengths are possible.  These would
|||	      rarely be used; and it is left as an experiment to the reader to
|||	      determine what they are.  A hint:  any combination of Header +
|||	      SubHeader + Data + Aux/ECC can generally be obtained per sector.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <device/cdrom.h>, <kernel/devicecmd.h>
|||
|||	  Notes
|||
|||	    Much information about the contents of sector data can be gleaned
|||	    from the RedBook, YellowBook, OrangeBook, WhiteBook, and GreenBook
|||	    CD-ROM specifications (Phillips/Sony).
|||
|||	  See Also
|||
|||	    CDROMCMD_SCAN_READ(@), CDROMCMD_SETDEFAULTS(@), CMD_BLOCKREAD(@)
**/

/**
|||	AUTODOC -class Device_Commands -group CD-ROM -name CDROMCMD_SCAN_READ
|||	Requests approximate block (sector) data from the disc.
|||
|||	  Description
|||
|||	    This command causes the drive to seek "close to" the requested
|||	    sector and begin returning data immediately.  This provides a means
|||	    of loose seeking by allowing (cd-rom) track skipping without having
|||	    to return a specific sector.  It is anticipated that this will be
|||	    used by the Audio and Video apps to improve fast-fwd/fast-rev
|||	    performance.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to CDROMCMD_SCAN_READ.
|||
|||	    ioi_Recv.iob_Buffer
|||	        Pointer to a receive buffer.  Optimum performance will be
|||	        attained if the buffer is 4-byte alligned.
|||
|||	    ioi_Recv.iob_Len
|||	        The length of the receive buffer must be a multiple of the
|||	        block size.  This is dependent on the current media.
|||	        For the pre-defined block lengths, see
|||	        ioi_CmdOptions.asFields.blockLength below.
|||
|||	    ioi_Offset
|||	        This is the address of the sector you wish to jump close to.
|||	        It can be in either of the following formats:  1) the absolute
|||	        sector address (e.g., sector 9385681); or 2) the address of the
|||	        sector in binary MSF format.
|||
|||	    ioi_CmdOptions
|||	        This field provides access to parameters which define how to
|||	        perform this read.  The options are provided as bitfields of
|||	        this 32-bit word.  Specifying a value of zero for each field
|||	        means you wish to use the current default.  The system defaults
|||	        are indicated below with '*'.  The fields are accessed by
|||	        specifying 'ioi_CmdOptions.asFields' before the following:
|||
|||	    .densityCode
|||	        Specifies what type of media you are reading; and is one
|||	        of the following:
|||
|||	      NULL
|||	          Use the current default.
|||
|||	      CDROM_DEFAULT_DENSITY*
|||	          Indicates that you wish to read Mode1, Mode2Form1, or
|||	          Mode2Form2 type sectors.  Do whatever is necessary to return
|||	          the correct data.
|||
|||	      CDROM_DATA
|||	          Indicates that you wish to read Mode1 sectors.  If a
|||	          different sector type exists an error is returned.
|||
|||	      CDROM_MODE2_XA
|||	          Indicates that you wish to read Mode2Form1 or Mode2Form2
|||	          sectors.  If a different sector type exists an error is
|||	          returned.
|||
|||	      CDROM_DIGITAL_AUDIO
|||	          Indicates that you wish to read RedBook Audio sectors data.
|||
|||
|||	    .addressFormat
|||	        Indicates what address format is specified in ioi_Offset.
|||	        Valid values are:
|||
|||	      NULL
|||	          Use the current default.
|||
|||	      CDROM_Address_Blocks*
|||	          Indicates that the sector address specified in ioi_Offset is
|||	          an absolute block/sector number.
|||
|||	      CDROM_Address_Abs_MSF
|||	          Indicates that the sector address specified in ioi_Offset is
|||	          specified in binary MSF format.  That is, a 32-bit value
|||	          (0x00MMSSFF) in which each field (M,S,F) is binary (as
|||	          opposed to Binary Coded Decimal).
|||
|||
|||	    .errorRecovery
|||	        Specifies what type of error recovery you would like performed;
|||	        and whether or not any errored sector data is returned.  Valid
|||	        values are:
|||
|||	      NULL
|||	          Use the current default.
|||
|||	      CDROM_DEFAULT_RECOVERY*
|||	          Perform ECC on any errored sector.  If ECC fails, retry the
|||	          read.  Continue for the number of retries specified by
|||	          .retryShift.  If unable to obtain the sector cleanly, return
|||	          an error.  NOTE:  Does not return the errored sector data.
|||
|||	      CDROM_CIRC_RETRIES_ONLY
|||	          Perform retries only (no ECC).  If retries fail, return an
|||	          error.  NOTE:  Does not return the errored sector data.
|||
|||	      CDROM_BEST_ATTEMPT_RECOVERY
|||	          Perform ECC and retries.  If both fail, return an error AND
|||	          the errored sector.  (Similar to CDROM_DEFAULT_RECOVERY.)
|||
|||
|||	    .retryShift
|||	        Specifies the number of retries that the driver attempts for a
|||	        given sector when an error is detected.  Upon exceeding this
|||	        count, the driver returns an error.  If the .errorRecovery
|||	        field contained CDROM_BEST_ATTEMPT_RECOVERY, then the errored
|||	        sector data is returned as well.
|||
|||	        The value (n) specified indicates that you wish to perform
|||	        (2^n - 1) retries.  Valid values for n range from 0 (zero
|||	        retries) to 7 (127 retries).  The default value is 3 (7
|||	        retries).
|||
|||
|||	    .speed
|||	        Specifies the speed at which the data is to be read.  Some
|||	        mechanisms may support 4x, 6x, and 8x speeds.  Others return
|||	        an error.  Valid values are:
|||
|||	      NULL
|||	          Use the current default.
|||
|||	      CDROM_SINGLE_SPEED
|||	          Operate the mechanism at single speed.
|||
|||	      CDROM_DOUBLE_SPEED*
|||	          Operate the mechanism at double speed.
|||
|||	      CDROM_4X_SPEED
|||	          Operate the mechanism at 4x speed.
|||
|||	      CDROM_6X_SPEED
|||	          Operate the mechanism at 6x speed.
|||
|||	      CDROM_8X_SPEED
|||	          Operate the mechanism at 8x speed.
|||
|||
|||	    .pitch
|||	        Specifies the variable pitch component of the speed.  Note that
|||	        this is only valid when operating in CDROM_SINGLE_SPEED.  Also
|||	        note that some mechanisms may not contain the hardware to
|||	        support variable pitch.  In such a case NO error is returned;
|||	        and the data is read in normal pitch.  Valid values are:
|||
|||	      NULL
|||	          Use the current default.
|||
|||	      CDROM_PITCH_SLOW
|||	          Operate drive at -1% of single speed.
|||
|||	      CDROM_PITCH_NORMAL
|||	          Operate drive at single speed.
|||
|||	      CDROM_PITCH_FAST
|||	          Operate drive at +1% of single speed.
|||
|||
|||	    .blockLength
|||	        Specifies the block length that you are interested in reading
|||	        from the disc sector.  Note that this value is dependent upon
|||	        the media present.  The pre-defined values are:
|||
|||	      NULL
|||	          Use the current default.
|||
|||	      CDROM_AUDIO
|||	          Returns 2352 bytes of audio data for each sector.
|||
|||	      CDROM_AUDIO_SUBCODE
|||	          Returns 2352 bytes of (RedBook) Audio data, followed by 96
|||	          bytes of Subcode, for EACH sector.  NOTE:  The subcode
|||	          returned with each sector is only LOOSELY associated with the
|||	          sector returned.  The subcode is NOT synced up with the data
|||	          for each sector.
|||
|||	      CDROM_MODE1*
|||	          Returns 2048 bytes of Mode1 (YellowBook) sector data.
|||
|||	      CDROM_MODE2FORM1
|||	          Returns 2048 bytes of Mode2Form1 (OrangeBook) sector data.
|||
|||	      CDROM_MODE2FORM1_SUBHEADER
|||	          Returns 8 bytes of SubHeader, followed by 2048 bytes of
|||	          Mode2Form1 (OrangeBook) sector data for EACH sector.
|||
|||	      CDROM_MODE2FORM2
|||	          Returns 2324 bytes of Mode2Form2 (OrangeBook) sector data.
|||
|||	      CDROM_MODE2FORM2_SUBHEADER
|||	          Returns 8 bytes of SubHeader, followed by 2324 bytes of
|||	          Mode2Form2 (OrangeBook) sector data for EACH sector.
|||
|||	      Note that other valid block lengths are possible.  These would
|||	      rarely be used; and it is left as an experiment to the reader to
|||	      determine what they are.  A hint:  any combination of Header +
|||	      SubHeader + Data + Aux/ECC can generally be obtained per sector.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <device/cdrom.h>, <kernel/devicecmd.h>
|||
|||	  Notes
|||
|||	    Much information about the contents of sector data can be gleaned
|||	    from the RedBook, YellowBook, OrangeBook, WhiteBook, and GreenBook
|||	    CD-ROM specifications (Phillips/Sony).
|||
|||	  See Also
|||
|||	    CDROMCMD_READ(@), CDROMCMD_SETDEFAULTS(@), CMD_BLOCKREAD(@)
**/

/**
|||	AUTODOC -class Device_Commands -group CD-ROM -name CDROMCMD_DISCDATA
|||	Requests the Disc Information (DiscID, TOC, and Session Info).
|||
|||	  Description
|||
|||	    This command causes the driver to return the DiscID, Table Of
|||	    Contents, and Session Info for the current disc.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to CDROMCMD_DISCDATA.
|||
|||	    ioi_Recv.iob_Buffer
|||	        Pointer to a CDROM_Disc_Data structure.
|||
|||	    ioi_Recv.iob_Len
|||	        Set to sizeof(CDROM_Disc_Data).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <device/cdrom.h>, <kernel/devicecmd.h>
**/

/**
|||	AUTODOC -class Device_Commands -group CD-ROM -name CDROMCMD_SETDEFAULTS
|||	Set/Get the current device defaults for read-mode IOReqs.
|||
|||	  Description
|||
|||	    This command sets the internal defaults used by the CD-ROM driver
|||	    to process CDROMCMD_READ(@) and CDROMCMD_SCAN_READ(@) IOReqs.
|||	    Each of these defaults is only used if its associated field in
|||	    ioi_CmdOptions is set to NULL.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to CDROMCMD_READ.
|||
|||	    ioi_Recv.iob_Buffer
|||	        To obtain the current defaults, set this to the address of
|||	        a CDROMCommandOptions structure.
|||
|||	    ioi_Recv.iob_Len
|||	        If obtaining the current defaults, set to
|||	        sizeof(CDROMCommandOptions).
|||
|||	    ioi_CmdOptions
|||	        This field provides is where the values for the new defaults
|||	        are specified.  The options are provided as bitfields of this
|||	        32-bit word.  Specifying a value of zero for each field means
|||	        you wish to keep the current default.  The system defaults are
|||	        indicated below with '*'.  The fields are accessed by
|||	        specifying 'ioi_CmdOptions.asFields' before the following:
|||
|||	    .densityCode
|||	        Specifies what type of media you are reading; and is one
|||	        of the following:
|||
|||	      NULL
|||	          Use the current default.
|||
|||	      CDROM_DEFAULT_DENSITY*
|||	          Indicates that you wish to read Mode1, Mode2Form1, or
|||	          Mode2Form2 type sectors.  Do whatever is necessary to return
|||	          the correct data.
|||
|||	      CDROM_DATA
|||	          Indicates that you wish to read Mode1 sectors.  If a
|||	          different sector type exists an error is returned.
|||
|||	      CDROM_MODE2_XA
|||	          Indicates that you wish to read Mode2Form1 or Mode2Form2
|||	          sectors.  If a different sector type exists an error is
|||	          returned.
|||
|||	      CDROM_DIGITAL_AUDIO
|||	          Indicates that you wish to read RedBook Audio sectors data.
|||
|||
|||	    .addressFormat
|||	        Indicates what address format is specified in ioi_Offset.
|||	        Valid values are:
|||
|||	      NULL
|||	          Use the current default.
|||
|||	      CDROM_Address_Blocks*
|||	          Indicates that the sector address specified in ioi_Offset is
|||	          an absolute block/sector number.
|||
|||	      CDROM_Address_Abs_MSF
|||	          Indicates that the sector address specified in ioi_Offset is
|||	          specified in binary MSF format.  That is, a 32-bit value
|||	          (0x00MMSSFF) in which each field (M,S,F) is binary (as
|||	          opposed to Binary Coded Decimal).
|||
|||
|||	    .errorRecovery
|||	        Specifies what type of error recovery you would like performed;
|||	        and whether or not any errored sector data is returned.  Valid
|||	        values are:
|||
|||	      NULL
|||	          Use the current default.
|||
|||	      CDROM_DEFAULT_RECOVERY*
|||	          Perform ECC on any errored sector.  If ECC fails, retry the
|||	          read.  Continue for the number of retries specified by
|||	          .retryShift.  If unable to obtain the sector cleanly, return
|||	          an error.  NOTE:  Does not return the errored sector data.
|||
|||	      CDROM_CIRC_RETRIES_ONLY
|||	          Perform retries only (no ECC).  If retries fail, return an
|||	          error.  NOTE:  Does not return the errored sector data.
|||
|||	      CDROM_BEST_ATTEMPT_RECOVERY
|||	          Perform ECC and retries.  If both fail, return an error AND
|||	          the errored sector.  (Similar to CDROM_DEFAULT_RECOVERY.)
|||
|||
|||	    .retryShift
|||	        Specifies the number of retries that the driver attempts for a
|||	        given sector when an error is detected.  Upon exceeding this
|||	        count, the driver returns an error.  If the .errorRecovery
|||	        field contained CDROM_BEST_ATTEMPT_RECOVERY, then the errored
|||	        sector data is returned as well.
|||
|||	        The value (n) specified indicates that you wish to perform
|||	        (2^n - 1) retries.  Valid values for n range from 0 (zero
|||	        retries) to 7 (127 retries).  The default value is 3 (7
|||	        retries).
|||
|||	        Note that the only way to specify ZERO retries is if the
|||	        the .errorRecovery field is non-NULL.
|||
|||
|||	    .speed
|||	        Specifies the speed at which the data is to be read.  Some
|||	        mechanisms may support 4x, 6x, and 8x speeds.  Others return
|||	        an error.  Valid values are:
|||
|||	      NULL
|||	          Use the current default.
|||
|||	      CDROM_SINGLE_SPEED
|||	          Operate the mechanism at single speed.
|||
|||	      CDROM_DOUBLE_SPEED*
|||	          Operate the mechanism at double speed.
|||
|||	      CDROM_4X_SPEED
|||	          Operate the mechanism at 4x speed.
|||
|||	      CDROM_6X_SPEED
|||	          Operate the mechanism at 6x speed.
|||
|||	      CDROM_8X_SPEED
|||	          Operate the mechanism at 8x speed.
|||
|||
|||	    .pitch
|||	        Specifies the variable pitch component of the speed.  Note that
|||	        this is only valid when operating in CDROM_SINGLE_SPEED.  Also
|||	        note that some mechanisms may not contain the hardware to
|||	        support variable pitch.  In such a case NO error is returned;
|||	        and the data is read in normal pitch.  Valid values are:
|||
|||	      NULL
|||	          Use the current default.
|||
|||	      CDROM_PITCH_SLOW
|||	          Operate drive at -1% of single speed.
|||
|||	      CDROM_PITCH_NORMAL
|||	          Operate drive at single speed.
|||
|||	      CDROM_PITCH_FAST
|||	          Operate drive at +1% of single speed.
|||
|||
|||	    .blockLength
|||	        Specifies the block length that you are interested in reading
|||	        from the disc sector.  Note that this value is dependent upon
|||	        the media present.  The pre-defined values are:
|||
|||	      NULL
|||	          Use the current default.
|||
|||	      CDROM_AUDIO
|||	          Returns 2352 bytes of audio data for each sector.
|||
|||	      CDROM_AUDIO_SUBCODE
|||	          Returns 2352 bytes of (RedBook) Audio data, followed by 96
|||	          bytes of Subcode, for EACH sector.  NOTE:  The subcode
|||	          returned with each sector is only LOOSELY associated with the
|||	          sector returned.  The subcode is NOT synced up with the data
|||	          for each sector.
|||
|||	      CDROM_MODE1*
|||	          Returns 2048 bytes of Mode1 (YellowBook) sector data.
|||
|||	      CDROM_MODE2FORM1
|||	          Returns 2048 bytes of Mode2Form1 (OrangeBook) sector data.
|||
|||	      CDROM_MODE2FORM1_SUBHEADER
|||	          Returns 8 bytes of SubHeader, followed by 2048 bytes of
|||	          Mode2Form1 (OrangeBook) sector data for EACH sector.
|||
|||	      CDROM_MODE2FORM2
|||	          Returns 2324 bytes of Mode2Form2 (OrangeBook) sector data.
|||
|||	      CDROM_MODE2FORM2_SUBHEADER
|||	          Returns 8 bytes of SubHeader, followed by 2324 bytes of
|||	          Mode2Form2 (OrangeBook) sector data for EACH sector.
|||
|||	      Note that other valid block lengths are possible.  These would
|||	      rarely be used; and it is left as an experiment to the reader to
|||	      determine what they are.  A hint:  any combination of Header +
|||	      SubHeader + Data + Aux/ECC can generally be obtained per sector.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <device/cdrom.h>, <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    CDROMCMD_READ(@), CDROMCMD_SCAN_READ(@), CMD_BLOCKREAD(@)
**/

/**
|||	AUTODOC -class Device_Commands -group CD-ROM -name CDROMCMD_READ_SUBQ
|||	Requests the QCode data for the current sector.
|||
|||	  Description
|||
|||	    This command causes the driver to return the QCode information for
|||	    the current sector.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to CDROMCMD_READ_SUBQ.
|||
|||	    ioi_Recv.iob_Buffer
|||	        Pointer to a SubQInfo structure.
|||
|||	    ioi_Recv.iob_Len
|||	        Set to sizeof(SubQInfo).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <device/cdrom.h>, <kernel/devicecmd.h>
**/

/**
|||	AUTODOC -class Device_Commands -group CD-ROM -name CDROMCMD_OPEN_DRAWER
|||	Open the CD-ROM drive's drawer.
|||
|||	  Description
|||
|||	    This command opens the CD-ROM drive drawer.  This will not have any
|||	    effect on a clamshell mechanism.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to CDROMCMD_OPEN_DRAWER.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <device/cdrom.h>, <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    CDROMCMD_CLOSE_DRAWER(@)
**/

/**
|||	AUTODOC -class Device_Commands -group CD-ROM -name CDROMCMD_CLOSE_DRAWER
|||	Close the CD-ROM drive's drawer.
|||
|||	  Description
|||
|||	    This command closes the CD-ROM drive drawer.  This will not have
|||	    any effect on a clamshell mechanism.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to CDROMCMD_CLOSE_DRAWER.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <device/cdrom.h>, <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    CDROMCMD_OPEN_DRAWER(@)
**/

/**
|||	AUTODOC -private -class Device_Commands -group CD-ROM -name CDROMCMD_RESIZE_BUFFERS
|||	Requests the CD-ROM driver to give up 32K of memory.
|||
|||	  Description
|||
|||	    This command causes the driver to jettison 32K of its internal
|||	    memory used for drive buffers.  This has the adverse effect of
|||	    potentially poorer performance when reading data from the disc.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to CDROMCMD_RESIZE_BUFFERS.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <device/m2cd.h>, <kernel/devicecmd.h>
**/

/**
|||	AUTODOC -private -class Device_Commands -group CD-ROM -name CDROMCMD_DIAG_INFO
|||	Set/Get diagnostic info from the CD-ROM driver.
|||
|||	  Description
|||
|||	    This command has two modes of operation:  1) Setting various
|||	    internal driver variables in order to alter driver behavior; and
|||	    2) Getting the contents of the driver's internal 'cdrom' struct.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to CDROMCMD_DIAG_INFO.
|||
|||	    ioi_Recv.iob_Buffer
|||	        To obtain contents of cdrom struct, set to the address of a
|||	        cdrom struct.
|||
|||	    ioi_Recv.iob_Len
|||	        To obtain contents of cdrom struct, set to sizeof(cdrom).
|||
|||	    ioi_Send.iob_Buffer
|||	        To alter the state the internal variable gPrintTheStuff, set
|||	        this to the address of a char who's value is 'p'.  To change
|||	        the value of gMaxHWM, set this to the address of a char who's
|||	        value is 'h'.
|||
|||	    ioi_Send.iob_Len
|||	        For changing gPrintTheStuff or gMaxHWM, set to 1.
|||
|||	    ioi_CmdOptions
|||	        When altering gPrintTheStuff, this value is XOR'd with the
|||	        the current value to produce the new value.  When changing
|||	        gMaxHWM, this contains the value of the new maximum High
|||	        Water Mark.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <device/m2cd.h>, <kernel/devicecmd.h>
**/

/**
|||	AUTODOC -class Device_Commands -group Timer -name TIMERCMD_GETTIME_VBL
|||	Returns the current system time in vertical blanking intervals.
|||
|||	  Description
|||
|||	    This command returns the current system time counted in VBL units.
|||	    There are 60 VBLs per second on an NTSC system and 50 VBLs per
|||	    second on a PAL system.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to TIMERCMD_GETTIME_VBL
|||
|||	    ioi_Recv.iob_Buffer
|||	        Pointer to a TimeValVBL variable where the VBL count will be
|||	        put.
|||
|||	    ioi_Recv.iob_Len
|||	        Set to sizeof(TimeValVBL).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/time.h>, <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    TIMERCMD_SETTIME_VBL(@), TIMERCMD_DELAY_VBL(@),
|||	    TIMERCMD_DELAYUNTIL_VBL(@), TIMERCMD_METRONOME_VBL(@)
**/

/**
|||	AUTODOC -class Device_Commands -group Timer -name TIMERCMD_SETTIME_VBL
|||	Sets the current system time in vertical blanking intervals.
|||
|||	  Description
|||
|||	    This command lets you set the current value of the system VBL
|||	    counter.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to TIMERCMD_SETTIME_VBL
|||
|||	    ioi_Send.iob_Buffer
|||	        Pointer to a TimeValVBL variable specifying the new VBL count.
|||
|||	    ioi_Send.iob_Len
|||	        Set to sizeof(TimeValVBL).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/time.h>, <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    TIMERCMD_GETTIME_VBL(@), TIMERCMD_DELAY_VBL(@),
|||	    TIMERCMD_DELAYUNTIL_VBL(@), TIMERCMD_METRONOME_VBL(@)
**/

/**
|||	AUTODOC -class Device_Commands -group Timer -name TIMERCMD_DELAY_VBL
|||	Waits for a fixed number of vertical blanking intervals.
|||
|||	  Description
|||
|||	    This command does nothing but wait for a specific number of
|||	    vertical blanking intervals to pass. When that number passes,
|||	    your IOReq is returned to you. This is a simple way to wait for an
|||	    amount of time to pass.
|||
|||	    There are 60 VBLs per second on NTSC systems and 50 per second
|||	    on PAL systems. Therefore, waiting for 60 VBLs will cause a 1
|||	    second delay on NTSC systems, and a 1.2 second delay on PAL
|||	    systems.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to TIMERCMD_DELAY_VBL
|||
|||	    ioi_Offset
|||	        Specifies the number of vertical blanking intervals to wait
|||	        for.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/time.h>, <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    TIMERCMD_GETTIME_VBL(@), TIMERCMD_SETTIME_VBL(@),
|||	    TIMERCMD_DELAYUNTIL_VBL(@), TIMERCMD_METRONOME_VBL(@)
**/

/**
|||	AUTODOC -class Device_Commands -group Timer -name TIMERCMD_DELAYUNTIL_VBL
|||	Waits for the system VBL count to reach a specific number.
|||
|||	  Description
|||
|||	    This command does nothing but wait for the system's vertical
|||	    blank counter to reach a specific number. When that number passes,
|||	    your IOReq is returned to you. This is a simple way to wait for a
|||	    specific time to arrive. If you ask to wait for a count that has
|||	    already passed, your IOReq is returned immediately.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to TIMERCMD_DELAYUNTIL_VBL
|||
|||	    ioi_Offset
|||	        Specifies the vertical blank count value to wait for.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/time.h>, <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    TIMERCMD_GETTIME_VBL(@), TIMERCMD_SETTIME_VBL(@),
|||	    TIMERCMD_DELAY_VBL(@), TIMERCMD_METRONOME_VBL(@)
**/

/**
|||	AUTODOC -class Device_Commands -group Timer -name TIMERCMD_METRONOME_VBL
|||	Requests to be signalled at regular intervals of time.
|||
|||	  Description
|||
|||	    This command causes a signal to be sent to your task on a regular
|||	    basis. This is very useful to deal with operations that must occur
|||	    consistently at regular time intervals. Once you have submitted
|||	    an IOReq using this command, you will start receiving signals. In
|||	    order to stop the signals, you must abort the IOReq using AbortIO()
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to TIMERCMD_METRONOME_VBL
|||
|||	    ioi_Offset
|||	        Specifies the number of vertical blanking intervals between
|||	        each signal sent to your task.
|||
|||	    ioi_CmdOptions
|||	        The signal mask specifying which signals to send to your task.
|||	        This is typically the return value of AllocSignal(0).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/time.h>, <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    TIMERCMD_GETTIME_VBL(@), TIMERCMD_SETTIME_VBL(@),
|||	    TIMERCMD_DELAYUNTIL_VBL(@), TIMERCMD_DELAY_VBL(@), AllocSignal()
**/

/**
|||	AUTODOC -class Device_Commands -group Timer -name TIMERCMD_GETTIME_USEC
|||	Returns the current system time.
|||
|||	  Description
|||
|||	    This command returns the current system time.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to TIMERCMD_GETTIME_USEC
|||
|||	    ioi_Recv.iob_Buffer
|||	        Pointer to a TimeVal structure to receive the current time.
|||
|||	    ioi_Recv.iob_Len
|||	        Set to sizeof(TimeVal).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/time.h>, <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    TIMERCMD_SETTIME_USEC(@), TIMERCMD_DELAY_USEC(@),
|||	    TIMERCMD_DELAYUNTIL_USEC(@), TIMERCMD_METRONOME_USEC(@)
**/

/**
|||	AUTODOC -class Device_Commands -group Timer -name TIMERCMD_SETTIME_USEC
|||	Sets the current system time.
|||
|||	  Description
|||
|||	    This command lets you set the current system time.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to TIMERCMD_SETTIME_USEC
|||
|||	    ioi_Send.iob_Buffer
|||	        Pointer to a TimeVal structure specifying the new system time.
|||
|||	    ioi_Send.iob_Len
|||	        Set to sizeof(TimeVal).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/time.h>, <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    TIMERCMD_GETTIME_USEC(@), TIMERCMD_DELAY_USEC(@),
|||	    TIMERCMD_DELAYUNTIL_USEC(@), TIMERCMD_METRONOME_USEC(@)
**/

/**
|||	AUTODOC -class Device_Commands -group Timer -name TIMERCMD_DELAY_USEC
|||	Waits for a fixed amount of time to pass.
|||
|||	  Description
|||
|||	    This command does nothing but wait for a specific amount of time
|||	    to pass. When that amount of time passes, your IOReq is returned to
|||	    you. This is a simple way to wait for an amount of time to pass.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to TIMERCMD_DELAY_USEC
|||
|||	    ioi_Send.iob_Buffer
|||	        Pointer to a TimeVal structure specifying the amount of time to
|||	        wait for.
|||
|||	    ioi_Send.iob_Len
|||	        Set to sizeof(TimeVal).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/time.h>, <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    TIMERCMD_GETTIME_USEC(@), TIMERCMD_SETTIME_USEC(@),
|||	    TIMERCMD_DELAYUNTIL_USEC(@), TIMERCMD_METRONOME_USEC(@)
**/

/**
|||	AUTODOC -class Device_Commands -group Timer -name TIMERCMD_DELAYUNTIL_USEC
|||	Waits for a given time.
|||
|||	  Description
|||
|||	    This command does nothing but wait for a given time to arrive.
|||	    When that time arrives, your IOReq is returned to you. If you ask
|||	    to wait for a time that has already passed, your IOReq is returned
|||	    immediately.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to TIMERCMD_DELAYUNTIL_USEC
|||
|||	    ioi_Send.iob_Buffer
|||	        Pointer to a TimeVal structure specifying the time to wait for.
|||
|||	    ioi_Send.iob_Len
|||	        Set to sizeof(TimeVal).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/time.h>, <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    TIMERCMD_GETTIME_USEC(@), TIMERCMD_SETTIME_USEC(@),
|||	    TIMERCMD_DELAY_USEC(@), TIMERCMD_METRONOME_USEC(@)
**/

/**
|||	AUTODOC -class Device_Commands -group Timer -name TIMERCMD_METRONOME_USEC
|||	Requests to be signalled at regular intervals of time.
|||
|||	  Description
|||
|||	    This command causes a signal to be sent to your task on a regular
|||	    basis. This is very useful to deal with operations that must occur
|||	    consistently at regular time intervals. Once you have submitted
|||	    an IOReq using this command, you will start receiving signals. In
|||	    order to stop the signals, you must abort the IOReq using AbortIO()
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to TIMERCMD_METRONOME_USEC
|||
|||	    ioi_Send.iob_Buffer
|||	        Pointer to a TimeVal structure that specifies the amount of
|||	        time between each signal sent to your task.
|||
|||	    ioi_Send.iob_Len
|||	        Set to sizeof(TimeVal).
|||
|||	    ioi_CmdOptions
|||	        The signal mask specifying which signals to send to your task.
|||	        This is typically the return value of AllocSignal(0).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/time.h>, <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    TIMERCMD_GETTIME_USEC(@), TIMERCMD_SETTIME_USEC(@),
|||	    TIMERCMD_DELAYUNTIL_USEC(@), TIMERCMD_DELAY_USEC(@), AllocSignal()
**/

/**
|||	AUTODOC -class Device_Commands -group File -name FILECMD_READDIR
|||	Read a directory entry by number
|||
|||	  Description
|||
|||	    This command allows the caller to scan through a list of
|||	    directory entries in the target directory. The number of a
|||	    directory entry is its physical position in the directory block
|||	    of the target directory. A directory entry is either a file
|||	    or a subdirectory. Target directory must have been previously
|||	    opened through OpenFile().
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to FILECMD_READDIR
|||
|||	    ioi_Offset
|||	        Number of the directory entry in the directory list. Entry
|||	        1 is the first entry in the directory.
|||
|||	    ioi_Recv.iob_Buffer
|||	        Pointer to the user allocated DirectoryEntry structure.
|||	        After the command is successfully completed, this buffer
|||	        contains the entry data.
|||
|||	    ioi_Recv.iob_Len
|||	        Set to sizeof(DirectoryEntry).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <file/filesystem.h>, <file/directory.h>, <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    FILECMD_GETPATH(@), FILECMD_READENTRY(@), FILECMD_SETVERSION(@),
|||	    FILECMD_ALLOCBLOCKS(@), FILECMD_SETEOF(@), FILECMD_SETDATE(@),
|||	    FILECMD_SETTYPE(@), FILECMD_FSSTAT(@), OpenFile(), CloseFile()
**/

/**
|||	AUTODOC -class Device_Commands -group File -name FILECMD_READENTRY
|||	Read a directory entry by name
|||
|||	  Description
|||
|||	    This command allows the caller to scan through a list of
|||	    directory entries looking for a specific entry. The entry is
|||	    identified by its name. A directory entry is either a file
|||	    or a subdirectory. Target directory must have been previously
|||	    opened through OpenFile().
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to FILECMD_READENTRY
|||
|||	    ioi_Send.iob_Buffer
|||	        Pointer to a null-terminated filename.
|||
|||	    ioi_Send.iob_Len
|||	        Set to strlen(filename) + 1.
|||
|||	    ioi_Recv.iob_Buffer
|||	        Pointer to the DirectoryEntry structure.
|||
|||	    ioi_Recv.iob_Len
|||	        Set to sizeof(DirectoryEntry).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <file/filesystem.h>, <file/directory.h>, <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    FILECMD_GETPATH(@), FILECMD_READDIR(@), FILECMD_SETVERSION(@),
|||	    FILECMD_ALLOCBLOCKS(@), FILECMD_SETEOF(@), FILECMD_SETDATE(@),
|||	    FILECMD_SETTYPE(@), FILECMD_FSSTAT(@),
|||	    OpenFile(), CloseFile()
**/

/**
|||	AUTODOC -class Device_Commands -group File -name FILECMD_GETPATH
|||	Get pathname of a file
|||
|||	  Description
|||
|||	    This command returns the pathname of an open file. This is
|||	    useful when the file has been opened by an alternate pathname.
|||	    Target file must have been previously opened through
|||	    OpenFile().
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to FILECMD_GETPATH
|||
|||	    ioi_Recv.iob_Buffer
|||	        Pointer to a buffer of at least FILESYSTEM_MAX_PATH_LEN bytes.
|||
|||	    ioi_Recv.iob_Len
|||	        length of the buffer.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <file/filesystem.h>, <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    FILECMD_READENTRY(@), FILECMD_READDIR(@), FILECMD_SETVERSION(@),
|||	    FILECMD_ALLOCBLOCKS(@), FILECMD_SETEOF(@), FILECMD_SETDATE(@),
|||	    FILECMD_SETTYPE(@), FILECMD_FSSTAT(@),
|||	    OpenFile(), CloseFile()
**/

/**
|||	AUTODOC -class Device_Commands -group File -name FILECMD_ALLOCBLOCKS
|||	Allocate storage space for a file
|||
|||	  Description
|||
|||	    This command allocates the specified number of blocks to an
|||	    existing file. Target File must have been previously created
|||	    through CreateFile() and opened via OpenFile(). Space
|||	    allocation is atomic, either the specified number of blocks
|||	    is allocated or non. If there is not enough space on the filesystem
|||	    to service the request, then an error is returned in io_Error
|||	    field of ioReq. Otherwise, zero is returned. Although the
|||	    filesystem makes every effort to make the allocation contiguous,
|||	    caller should make no assumption about the nature of allocation.
|||	    Space allocation may or may not be contiguous. This command may be
|||	    called multiple times to allocate more storage space for a given
|||	    file. However, it is most efficient to allocate all necessary
|||	    storage only once immediately after the file is created and opened.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to FILECMD_ALLOCBLOCKS
|||
|||	    ioi_Offset
|||	        Number of file blocks to allocate or free.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <file/filesystem.h>, <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    FILECMD_READENTRY(@), FILECMD_READDIR(@), FILECMD_SETVERSION(@),
|||	    FILECMD_GETPATH(@), FILECMD_SETEOF(@), FILECMD_SETDATE(@),
|||	    FILECMD_SETTYPE(@), FILECMD_FSSTAT(@),
|||	    OpenFile(), CloseFile(), CreateFile()
**/

/**
|||	AUTODOC -class Device_Commands -group File -name FILECMD_SETVERSION
|||	Sets revision and version of a file
|||
|||	  Description
|||
|||	    This command sets a revision and version number for specified
|||	    file. Target file must have been previously opened through
|||	    OpenFile().
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to FILECMD_SETVERSION
|||
|||	    ioi_Offset
|||	        Version and revision to set. Version is the first byte
|||	        followed by revision number.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <file/filesystem.h>, <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    FILECMD_READENTRY(@), FILECMD_READDIR(@), FILECMD_ALLOCBLOCKS(@),
|||	    FILECMD_GETPATH(@), FILECMD_SETEOF(@), FILECMD_SETDATE(@),
|||	    FILECMD_SETTYPE(@), FILECMD_FSSTAT(@),
|||	    OpenFile(), CloseFile(), CreateFile()
**/

/**
|||	AUTODOC -class Device_Commands -group File -name FILECMD_SETDATE
|||	Sets the modification date of a file or directory.
|||
|||	  Description
|||
|||	    This command sets the modification date of a file or directory.
|||	    Target file or directory must have been previously opened through
|||	    OpenFile().
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to FILECMD_SETDATE
|||
|||	    ioi_Send.iob_Buffer
|||	        Pointer to a TimeVal structure specifying the new modification
|||	        date for the file or directory. The time specified is relative
|||	        to 01-Jan-1993
|||
|||	    ioi_Send.iob_Len
|||	        Set to sizeof(TimeVal).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V33.
|||
|||	  Associated Files
|||
|||	    <file/filesystem.h>, <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    FILECMD_READENTRY(@), FILECMD_READDIR(@), FILECMD_ALLOCBLOCKS(@),
|||	    FILECMD_GETPATH(@), FILECMD_SETEOF(@), FILECMD_SETVERSION(@),
|||	    FILECMD_SETTYPE(@), FILECMD_FSSTAT(@),
|||	    OpenFile(), CloseFile(), CreateFile()
**/

/**
|||	AUTODOC -class Device_Commands -group File -name FILECMD_SETEOF
|||	Set the End Of File (EOF)
|||
|||	  Description
|||
|||	    This command sets the logical end of file for the specified file.
|||	    EOF can not extend beyond the space allocated to the
|||	    file via FILECMD_ALLOCBLOCKS, Portfolio Operating System does
|||	    not support holes in files at this time. EOF is automatically
|||	    set when the file is written passed its previous EOF. When a
|||	    file is created EOF is set to zero. It remains zero even though
|||	    storage is allocated to it. The only time EOF is changed
|||	    is either through this command or when an actual write occurs
|||	    extending the logical size of the file. Target file must have been
|||	    previously opened through OpenFile().
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to FILECMD_SETEOF
|||
|||	    ioi_Offset
|||	        New logical file size in bytes.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <file/filesystem.h>, <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    FILECMD_READENTRY(@), FILECMD_READDIR(@), FILECMD_ALLOCBLOCKS(@),
|||	    FILECMD_GETPATH(@), FILECMD_SETVERSION(@),
|||	    FILECMD_SETTYPE(@), FILECMD_FSSTAT(@),
|||	    OpenFile(), CloseFile(), CreateFile()
**/

/**
|||	AUTODOC -private -class Device_Commands -group File -name FILECMD_ADDENTRY
|||	Add a file to an existing directory
|||
|||	  Description
|||
|||	    This command is internal and for supervisor only. User must call
|||	    CreateFile() system calls to create new files.
|||
|||	  Associated Files
|||
|||	    <file/filesystem.h>, <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    FILECMD_READENTRY(@), FILECMD_READDIR(@), FILECMD_ALLOCBLOCKS(@),
|||	    FILECMD_GETPATH(@), FILECMD_SETEOF(@), FILECMD_SETDATE(@), FILECMD_SETVERSION(@),
|||	    FILECMD_DELETEENTRY(@), FILECMD_SETTYPE(@), FILECMD_FSSTAT(@),
|||	    FILECMD_ADDDIR(@), FILECMD_DELETEDIR(@), OpenFile(),
|||	    CloseFile(), CreateFile()
**/

/**
|||	AUTODOC -private -class Device_Commands -group File -name FILECMD_DELETEENTRY
|||	Delete a file from an existing directory
|||
|||	  Description
|||
|||	    This command is internal and for supervisor only. User must call
|||	    DeleteFile() system calls to delete files.
|||
|||	  Associated Files
|||
|||	    <file/filesystem.h>, <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    FILECMD_READENTRY(@), FILECMD_READDIR(@), FILECMD_ALLOCBLOCKS(@),
|||	    FILECMD_GETPATH(@), FILECMD_SETEOF(@), FILECMD_SETDATE(@), FILECMD_SETVERSION(@),
|||	    FILECMD_SETTYPE(@), FILECMD_FSSTAT(@),
|||	    OpenFile(), CloseFile(), DeleteFile()
**/

/**
|||	AUTODOC -private -class Device_Commands -group File -name FILECMD_ADDDIR
|||	Add a directory to an existing directory
|||
|||	  Description
|||
|||	    This command is internal and for supervisor only. User must call
|||	    CreateDirectory() system calls to create a new directory.
|||
|||	  Associated Files
|||
|||	    <file/filesystem.h>, <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    FILECMD_READENTRY(@), FILECMD_READDIR(@), FILECMD_ALLOCBLOCKS(@),
|||	    FILECMD_GETPATH(@), FILECMD_SETEOF(@), FILECMD_SETDATE(@), FILECMD_SETVERSION(@),
|||	    FILECMD_SETTYPE(@), FILECMD_FSSTAT(@),
|||	    OpenFile(), CloseFile(), CreateDirectory()
**/

/**
|||	AUTODOC -private -class Device_Commands -group File -name FILECMD_DELETEDIR
|||	Delete a directory from an existing directory
|||
|||	  Description
|||
|||	    This command is internal and for supervisor only. User must call
|||	    DeleteDirectory() system calls to delete an existing directory.
|||
|||	  Associated Files
|||
|||	    <file/filesystem.h>, <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    FILECMD_READENTRY(@), FILECMD_READDIR(@), FILECMD_ALLOCBLOCKS(@),
|||	    FILECMD_GETPATH(@), FILECMD_SETEOF(@), FILECMD_SETDATE(@), FILECMD_SETVERSION(@),
|||	    FILECMD_SETTYPE(@), FILECMD_FSSTAT(@),
|||	    OpenFile(), CloseFile(), DeleteDirectory()
**/

/**
|||	AUTODOC -class Device_Commands -group File -name FILECMD_SETTYPE
|||	Set file type
|||
|||	  Description
|||
|||	    This command sets the file type to specified type. Type can not
|||	    be set for directories (*dir) or system meta files. Only user
|||	    file types can be manipulated. At file creation file type is
|||	    set to NULL. Target file must have been previously opened through
|||	    OpenFile().
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to FILECMD_SETTYPE
|||
|||	    ioi_Offset
|||	        New file type.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <file/filesystem.h>, <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    FILECMD_READENTRY(@), FILECMD_READDIR(@), FILECMD_ALLOCBLOCKS(@),
|||	    FILECMD_GETPATH(@), FILECMD_SETVERSION(@),
|||	    FILECMD_SETEOF(@), FILECMD_SETDATE(@), FILECMD_FSSTAT(@),
|||	    OpenFile(), CloseFile(), CreateFile()
**/

/**
|||	AUTODOC -class Device_Commands -group File -name FILECMD_FSSTAT
|||	Get filesystem status information
|||
|||	  Description
|||
|||	    This command acquires information about a mounted filesystem.
|||	    This command can be called on any open file belonging to the
|||	    target filesystem. Typical information provided by this command
|||	    is the block size of the filesystem, its size, device information,
|||	    and the mount point of the filesystem. Not all filesystems can
|||	    provide all this information. For details see FileSystemStat
|||	    struct in filesystem.h. The fst_Bitmap field indicates about
|||	    which other fields of the structure this command was able to get
|||	    information. Since not all fields are meaningful for all of
|||	    the filesystems currently available on Portfolio. Target file
|||	    must have been previously opened through OpenFile().
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to FILECMD_FSSTAT
|||
|||	    ioi_Recv.iob_Buffer
|||	        Pointer to the user allocated FileSystemStat structure.
|||	        After the command is successfully completed, this buffer
|||	        contains the acquired data.
|||
|||	    ioi_Recv.iob_Len
|||	        Set to sizeof(FileSystemStat).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <file/filesystem.h>, <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    FILECMD_READENTRY(@), FILECMD_READDIR(@), FILECMD_ALLOCBLOCKS(@),
|||	    FILECMD_GETPATH(@), FILECMD_SETVERSION(@),
|||	    FILECMD_SETEOF(@), FILECMD_SETDATE(@), FILECMD_SETTYPE(@),
|||	    OpenFile(), CloseFile()
**/

/**
|||	AUTODOC -class Device_Commands -group Host -name HOST_CMD_SEND
|||	Sends a packet of information to a remote host development system.
|||
|||	  Description
|||
|||	    This command sends a 26 byte packet of information to the remote
|||	    debugging host.
|||
|||	    To send information over to the host, you must specify a particular
|||	    unit number. Unit numbers 0..127 are reserved for system use. You
|||	    can use units 128..255 for your own uses. By writing code on the
|||	    host system, you can read packets of information from any one of
|||	    these units. This can be very useful during development.
|||
|||	    The reserved units are used for things like host file system
|||	    interfacing, host command-line interface, and so forth. There are
|||	    higher-level APIs to access these units.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to HOST_CMD_SEND
|||
|||	    ioi_CmdOptions
|||	        Set to the unit number to use. Valid values are in the range
|||	        128..255.
|||
|||	    ioi_Send.iob_Buffer
|||	        Points to a buffer containing the data to send to the host.
|||
|||	    ioi_Send.iob_Len
|||	        Number of bytes to send to the host. This can be in the
|||	        range 1..26.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    HOST_CMD_RECV(@)
|||
**/

/**
|||	AUTODOC -class Device_Commands -group Host -name HOST_CMD_RECV
|||	Waits for a packet of information from a remote host development system.
|||
|||	  Description
|||
|||	    This command waits for a 26 byte packet of information to arrive
|||	    from the remote debugging host.
|||
|||	    To receive information from the host, you must specify a particular
|||	    unit number. Unit numbers 0..127 are reserved for system use. You
|||	    can use units 128..255 for your own uses. By writing code on the
|||	    host system, you can send packets of information on any one of
|||	    these units, and have code running on the 3DO system receive them.
|||	    This can be very useful during development.
|||
|||	    The reserved units are used for things like host file system
|||	    interfacing, host command-line interface, and so forth. There are
|||	    higher-level APIs to access these units.
|||
|||	    If there are no pending receive requests when a packet comes in
|||	    from the host, the packet gets buffered by the driver. A fixed
|||	    number of packets are buffered before overflows start occuring.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to HOST_CMD_RECV
|||
|||	    ioi_CmdOptions
|||	        Set to the unit number to use. Valid values are in the range
|||	        128..255.
|||
|||	    ioi_Recv.iob_Buffer
|||	        Points to a buffer where the packet of information can be
|||	        put.
|||
|||	    ioi_Recv.iob_Len
|||	        Number of bytes available for the packet. This can be in
|||	        the range 1..26.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    HOST_CMD_SEND(@)
|||
**/

/**
|||	AUTODOC -class Device_Commands -group HostFS -name HOSTFS_CMD_MOUNTFS
|||	Requests that a file system be mounted on a remote host.
|||
|||	  Description
|||
|||	    This command requests that the remote debugging host mount a
|||	    file system and return a reference token uniquely identifying it.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to HOSTFS_CMD_MOUNTFS
|||
|||	    ioi_Offset
|||	        File system number to mount. The host keeps mountable file
|||	        systems in a numbered list starting at 0. You can mount any
|||	        of these file systems. Asking to mount a non-existing file
|||	        system will result in an error code being returned.
|||
|||	    ioi_Recv.iob_Buffer
|||	        Points to a buffer where the Host will put the name of the
|||	        file system that was mounted. This pointer may be NULL in
|||	        which case no name is returned.
|||
|||	    ioi_Recv.iob_Len
|||	        Number of bytes available for the file system name, including
|||	        the NULL terminator.
|||
|||	  Return Value
|||
|||	    When the file system is successfully mounted, the ioi_CmdOptions
|||	    field of the IOReq item is set to a reference token which uniquely
|||	    identifies the file system to the host. You use this token
|||	    to reference objects within this file system.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    HOSTFS_CMD_OPENENTRY(@), HOSTFS_CMD_CLOSEENTRY(@),
|||	    HOSTFS_CMD_CREATEFILE(@), HOSTFS_CMD_CREATEDIR(@),
|||	    HOSTFS_CMD_DELETEENTRY(@), HOSTFS_CMD_READENTRY(@)
|||	    HOSTFS_CMD_READDIR(@), HOSTFS_CMD_ALLOCBLOCKS(@),
|||	    HOSTFS_CMD_BLOCKREAD(@), HOSTFS_CMD_STATUS(@),
|||	    HOSTFS_CMD_FSSTAT(@), HOSTFS_CMD_BLOCKWRITE(@),
|||	    HOSTFS_CMD_SETEOF(@), HOSTFS_CMD_SETTYPE(@),
|||	    HOSTFS_CMD_SETVERSION(@), HOSTFS_CMD_DISMOUNTFS(@),
|||	    HOSTFS_CMD_SETBLOCKSIZE(@),
|||
**/

/**
|||	AUTODOC -class Device_Commands -group HostFS -name HOSTFS_CMD_DISMOUNTFS
|||	Requests that a file system be dismounted.
|||
|||	  Description
|||
|||	    This command causes a remote host file system to be dismounted
|||	    and become unavailable for use.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to HOSTFS_CMD_DISMOUNTFS
|||
|||	    ioi_CmdOptions
|||	        Set to the reference token obtained when the file system
|||	        was first mounted using the HOSTFS_CMD_MOUNTFS(@) command.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    HOSTFS_CMD_OPENENTRY(@), HOSTFS_CMD_CLOSEENTRY(@),
|||	    HOSTFS_CMD_CREATEFILE(@), HOSTFS_CMD_CREATEDIR(@),
|||	    HOSTFS_CMD_DELETEENTRY(@), HOSTFS_CMD_READENTRY(@)
|||	    HOSTFS_CMD_READDIR(@), HOSTFS_CMD_ALLOCBLOCKS(@),
|||	    HOSTFS_CMD_BLOCKREAD(@), HOSTFS_CMD_STATUS(@),
|||	    HOSTFS_CMD_FSSTAT(@), HOSTFS_CMD_BLOCKWRITE(@),
|||	    HOSTFS_CMD_SETEOF(@), HOSTFS_CMD_SETTYPE(@),
|||	    HOSTFS_CMD_SETVERSION(@), HOSTFS_CMD_MOUNTFS(@),
|||	    HOSTFS_CMD_SETBLOCKSIZE(@),
|||
**/

/**
|||	AUTODOC -class Device_Commands -group HostFS -name HOSTFS_CMD_OPENENTRY
|||	Obtains a reference token for an object within a remote file system.
|||
|||	  Description
|||
|||	    This command lets you request a reference token for a named object
|||	    within a remote file system. The named object is relative to the
|||	    object to which the reference token points to. That is, if the
|||	    reference token was obtained from HOSTFS_CMD_MOUNTFS(@), then the
|||	    named object is in the root of the remote file system. If the
|||	    reference token was obtained from a previous call to
|||	    HOSTFS_CMD_OPENENTRY(@), then the name is relative to a
|||	    directory within the remote file system.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to HOSTFS_CMD_OPENENTRY
|||
|||	    ioi_Send.iob_Buffer
|||	        Pointer to the NULL-terminated name of the object to
|||	        obtain the reference token for. This name is relative to
|||	        the supplied reference token.
|||
|||	    ioi_Send.iob_Len
|||	        Number of bytes in the object name, including the
|||	        NULL terminator.
|||
|||	    ioi_CmdOptions
|||	        Reference token for the directory or file system that contains
|||	        the object to open.
|||
|||	  Return Value
|||
|||	    When the object is successfully opened, the ioi_CmdOptions
|||	    field of the IOReq item is set to a reference token which uniquely
|||	    identifies the desired object to the host. You use this token
|||	    to reference this object using other commands.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    HOSTFS_CMD_MOUNTFS(@), HOSTFS_CMD_CLOSEENTRY(@),
|||	    HOSTFS_CMD_CREATEFILE(@), HOSTFS_CMD_CREATEDIR(@),
|||	    HOSTFS_CMD_DELETEENTRY(@), HOSTFS_CMD_READENTRY(@)
|||	    HOSTFS_CMD_READDIR(@), HOSTFS_CMD_ALLOCBLOCKS(@),
|||	    HOSTFS_CMD_BLOCKREAD(@), HOSTFS_CMD_STATUS(@),
|||	    HOSTFS_CMD_FSSTAT(@), HOSTFS_CMD_BLOCKWRITE(@),
|||	    HOSTFS_CMD_SETEOF(@), HOSTFS_CMD_SETTYPE(@),
|||	    HOSTFS_CMD_SETVERSION(@), HOSTFS_CMD_DISMOUNTFS(@),
|||	    HOSTFS_CMD_SETBLOCKSIZE(@),
|||
**/

/**
|||	AUTODOC -class Device_Commands -group HostFS -name HOSTFS_CMD_CLOSEENTRY
|||	Concludes use of a reference token.
|||
|||	  Description
|||
|||	    This command is used to indicate that a given reference token is no
|||	    longer needed. The host is then free to reclaim any memory
|||	    associated with the token. Future attempts to use the token will
|||	    fail.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to HOSTFS_CMD_CLOSEENTRY
|||
|||	    ioi_CmdOptions
|||	        Reference token identifying the object to operate on,
|||	        as obtained using HOSTFS_CMD_OPENENTRY(@).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    HOSTFS_CMD_OPENENTRY(@), HOSTFS_CMD_MOUNTFS(@),
|||	    HOSTFS_CMD_CREATEFILE(@), HOSTFS_CMD_CREATEDIR(@),
|||	    HOSTFS_CMD_DELETEENTRY(@), HOSTFS_CMD_READENTRY(@)
|||	    HOSTFS_CMD_READDIR(@), HOSTFS_CMD_ALLOCBLOCKS(@),
|||	    HOSTFS_CMD_BLOCKREAD(@), HOSTFS_CMD_STATUS(@),
|||	    HOSTFS_CMD_FSSTAT(@), HOSTFS_CMD_BLOCKWRITE(@),
|||	    HOSTFS_CMD_SETEOF(@), HOSTFS_CMD_SETTYPE(@),
|||	    HOSTFS_CMD_SETVERSION(@), HOSTFS_CMD_DISMOUNTFS(@),
|||	    HOSTFS_CMD_SETBLOCKSIZE(@),
|||
**/

/**
|||	AUTODOC -class Device_Commands -group HostFS -name HOSTFS_CMD_CREATEFILE
|||	Creates a new file within an existing directory.
|||
|||	  Description
|||
|||	    This command is used to create a new file within an existing
|||	    directory on a remote host file system.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to HOSTFS_CMD_CREATEFILE
|||
|||	    ioi_Send.iob_Buffer
|||	        Points to the NULL-terminated name of the file to create.
|||
|||	    ioi_Send.iob_Len
|||	        Number of bytes in the file name, including the NULL terminator.
|||
|||	    ioi_CmdOptions
|||	        Reference token identifying the directory or file system into
|||	        which the file should be created, as obtained using
|||	        HOSTFS_CMD_MOUNTFS(@) or HOSTFS_CMD_OPENENTRY(@).
|||
|||	  Return Value
|||
|||	    When the file is successfully created, the ioi_CmdOptions
|||	    field of the IOReq item is set to a reference token which uniquely
|||	    identifies the new file to the host. You use this token
|||	    to reference this file using other commands.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    HOSTFS_CMD_OPENENTRY(@), HOSTFS_CMD_CLOSEENTRY(@),
|||	    HOSTFS_CMD_MOUNTFS(@), HOSTFS_CMD_CREATEDIR(@),
|||	    HOSTFS_CMD_DELETEENTRY(@), HOSTFS_CMD_READENTRY(@)
|||	    HOSTFS_CMD_READDIR(@), HOSTFS_CMD_ALLOCBLOCKS(@),
|||	    HOSTFS_CMD_BLOCKREAD(@), HOSTFS_CMD_STATUS(@),
|||	    HOSTFS_CMD_FSSTAT(@), HOSTFS_CMD_BLOCKWRITE(@),
|||	    HOSTFS_CMD_SETEOF(@), HOSTFS_CMD_SETTYPE(@),
|||	    HOSTFS_CMD_SETVERSION(@), HOSTFS_CMD_DISMOUNTFS(@),
|||	    HOSTFS_CMD_SETBLOCKSIZE(@),
|||
**/

/**
|||	AUTODOC -class Device_Commands -group HostFS -name HOSTFS_CMD_CREATEDIR
|||	Creates a new directory within an existing directory.
|||
|||	  Description
|||
|||	    This command is used to create a new directory within an existing
|||	    directory on a remote host file system.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to HOSTFS_CMD_CREATEDIR
|||
|||	    ioi_Send.iob_Buffer
|||	        Points to the NULL-terminated name of the directory to create,
|||
|||	    ioi_Send.iob_Len
|||	        Number of bytes in the directory name, including the NULL
|||	        terminator.
|||
|||	    ioi_CmdOptions
|||	        Reference token identifying the directory or file system into
|||	        which the directory should be created, as obtained using
|||	        HOSTFS_CMD_MOUNTFS(@) or HOSTFS_CMD_OPENENTRY(@).
|||
|||	  Return Value
|||
|||	    When the directory is successfully created, the ioi_CmdOptions
|||	    field of the IOReq item is set to a reference token which uniquely
|||	    identifies the new directory to the host. You use this token
|||	    to reference this directory using other commands.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    HOSTFS_CMD_OPENENTRY(@), HOSTFS_CMD_CLOSEENTRY(@),
|||	    HOSTFS_CMD_CREATEFILE(@), HOSTFS_CMD_MOUNTFS(@),
|||	    HOSTFS_CMD_DELETEENTRY(@), HOSTFS_CMD_READENTRY(@)
|||	    HOSTFS_CMD_READDIR(@), HOSTFS_CMD_ALLOCBLOCKS(@),
|||	    HOSTFS_CMD_BLOCKREAD(@), HOSTFS_CMD_STATUS(@),
|||	    HOSTFS_CMD_FSSTAT(@), HOSTFS_CMD_BLOCKWRITE(@),
|||	    HOSTFS_CMD_SETEOF(@), HOSTFS_CMD_SETTYPE(@),
|||	    HOSTFS_CMD_SETVERSION(@), HOSTFS_CMD_DISMOUNTFS(@),
|||	    HOSTFS_CMD_SETBLOCKSIZE(@),
|||
**/

/**
|||	AUTODOC -class Device_Commands -group HostFS -name HOSTFS_CMD_DELETEENTRY
|||	Deletes an entry within a remote file system.
|||
|||	  Description
|||
|||	    This command is used to delete an entry from a remote host file
|||	    system. The command works for both files and directories.
|||	    Directories can only be deleted if they are empty.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to HOSTFS_CMD_DELETEENTRY
|||
|||	    ioi_Send.iob_Buffer
|||	        Points to the NULL-terminated name of the object to delete,
|||
|||	    ioi_Send.iob_Len
|||	        Number of bytes in the object name, including the NULL
|||	        terminator.
|||
|||	    ioi_CmdOptions
|||	        Reference token identifying the directory containing the object
|||	        to delete, as obtained using HOSTFS_CMD_OPENENTRY(@).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    HOSTFS_CMD_OPENENTRY(@), HOSTFS_CMD_CLOSEENTRY(@),
|||	    HOSTFS_CMD_CREATEFILE(@), HOSTFS_CMD_CREATEDIR(@),
|||	    HOSTFS_CMD_MOUNTFS(@), HOSTFS_CMD_READENTRY(@)
|||	    HOSTFS_CMD_READDIR(@), HOSTFS_CMD_ALLOCBLOCKS(@),
|||	    HOSTFS_CMD_BLOCKREAD(@), HOSTFS_CMD_STATUS(@),
|||	    HOSTFS_CMD_FSSTAT(@), HOSTFS_CMD_BLOCKWRITE(@),
|||	    HOSTFS_CMD_SETEOF(@), HOSTFS_CMD_SETTYPE(@),
|||	    HOSTFS_CMD_SETVERSION(@), HOSTFS_CMD_DISMOUNTFS(@),
|||	    HOSTFS_CMD_SETBLOCKSIZE(@),
|||
**/

/**
|||	AUTODOC -class Device_Commands -group HostFS -name HOSTFS_CMD_READENTRY
|||	Obtains information about an object within a remote host file system.
|||
|||	  Description
|||
|||	    This command returns information about a particular object within
|||	    a remote host file system. The information is returned in a
|||	    DirectoryEntry structure as defined in <file/directory.h>
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to HOSTFS_CMD_READENTRY
|||
|||	    ioi_Send.iob_Buffer
|||	        Points to the NULL-terminated name of the object to get
|||	        information on.
|||
|||	    ioi_Send.iob_Len
|||	        Number of bytes in the object name, including the NULL
|||	        terminator.
|||
|||	    ioi_Recv.iob_Buffer
|||	        Points to a DirectoryEntry structure where the information
|||	        about the object will be stored.
|||
|||	    ioi_Recv.iob_Len
|||	        Set to sizeof(DirectoryEntry).
|||
|||	    ioi_CmdOptions
|||	        Reference token identifying the directory containing the object
|||	        to get information on, as obtained using HOSTFS_CMD_OPENENTRY(@).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <file/directory.h>, <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    HOSTFS_CMD_OPENENTRY(@), HOSTFS_CMD_CLOSEENTRY(@),
|||	    HOSTFS_CMD_CREATEFILE(@), HOSTFS_CMD_CREATEDIR(@),
|||	    HOSTFS_CMD_DELETEENTRY(@), HOSTFS_CMD_MOUNTFS(@)
|||	    HOSTFS_CMD_READDIR(@), HOSTFS_CMD_ALLOCBLOCKS(@),
|||	    HOSTFS_CMD_BLOCKREAD(@), HOSTFS_CMD_STATUS(@),
|||	    HOSTFS_CMD_FSSTAT(@), HOSTFS_CMD_BLOCKWRITE(@),
|||	    HOSTFS_CMD_SETEOF(@), HOSTFS_CMD_SETTYPE(@),
|||	    HOSTFS_CMD_SETVERSION(@), HOSTFS_CMD_DISMOUNTFS(@),
|||	    HOSTFS_CMD_SETBLOCKSIZE(@),
|||
**/

/**
|||	AUTODOC -class Device_Commands -group HostFS -name HOSTFS_CMD_READDIR
|||	Obtains information about an object within a remote host file system.
|||
|||	  Description
|||
|||	    This command returns information about a particular object within
|||	    a remote host file system. The information is returned in a
|||	    DirectoryEntry structure as defined in <file/directory.h>.
|||
|||	    This command is very similar to HOSTFS_CMD_READENTRY(@), except
|||	    that the object to get information on is specified by index number
|||	    within a directory, instead of by name of object within the
|||	    directory. This is the command to use when listing all files in a
|||	    directory.
|||
|||	    ioi_Command
|||	        Set to HOSTFS_CMD_READDIR
|||
|||	    ioi_Offset
|||	        The index of the object to get information on. Index values start
|||	        at 1, not 0.
|||
|||	    ioi_Recv.iob_Buffer
|||	        Points to a DirectoryEntry structure where the information
|||	        about the object will be stored.
|||
|||	    ioi_Recv.iob_Len
|||	        Set to sizeof(DirectoryEntry).
|||
|||	    ioi_CmdOptions
|||	        Reference token identifying the directory containing the object
|||	        to get information on, as obtained using HOSTFS_CMD_OPENENTRY(@).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <file/directory.h>, <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    HOSTFS_CMD_OPENENTRY(@), HOSTFS_CMD_CLOSEENTRY(@),
|||	    HOSTFS_CMD_CREATEFILE(@), HOSTFS_CMD_CREATEDIR(@),
|||	    HOSTFS_CMD_DELETEENTRY(@), HOSTFS_CMD_READENTRY(@)
|||	    HOSTFS_CMD_MOUNTFS(@), HOSTFS_CMD_ALLOCBLOCKS(@),
|||	    HOSTFS_CMD_BLOCKREAD(@), HOSTFS_CMD_STATUS(@),
|||	    HOSTFS_CMD_FSSTAT(@), HOSTFS_CMD_BLOCKWRITE(@),
|||	    HOSTFS_CMD_SETEOF(@), HOSTFS_CMD_SETTYPE(@),
|||	    HOSTFS_CMD_SETVERSION(@), HOSTFS_CMD_DISMOUNTFS(@),
|||	    HOSTFS_CMD_SETBLOCKSIZE(@),
|||
**/

/**
|||	AUTODOC -class Device_Commands -group HostFS -name HOSTFS_CMD_RENAMEENTRY
|||	Renames an object on a remote host file system.
|||
|||	  Description
|||
|||	    This command lets you rename a file or directory located on a
|||	    remote file system. You supply the new name of the object.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to HOSTFS_CMD_RENAMEENTRY
|||
|||	    ioi_Send.iob_Buffer
|||	        Points to the NULL-terminated new name of the object to delete,
|||
|||	    ioi_Send.iob_Len
|||	        Number of bytes in the new object name, including the NULL
|||	        terminator.
|||
|||	    ioi_CmdOptions
|||	        Reference token identifying the object the object being renamed,
|||	        as obtained using HOSTFS_CMD_OPENENTRY(@).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V30.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    HOSTFS_CMD_OPENENTRY(@), HOSTFS_CMD_CLOSEENTRY(@),
|||	    HOSTFS_CMD_CREATEFILE(@), HOSTFS_CMD_CREATEDIR(@),
|||	    HOSTFS_CMD_DELETEENTRY(@), HOSTFS_CMD_READENTRY(@)
|||	    HOSTFS_CMD_READDIR(@), HOSTFS_CMD_MOUNTFS(@),
|||	    HOSTFS_CMD_BLOCKREAD(@), HOSTFS_CMD_STATUS(@),
|||	    HOSTFS_CMD_FSSTAT(@), HOSTFS_CMD_BLOCKWRITE(@),
|||	    HOSTFS_CMD_SETEOF(@), HOSTFS_CMD_SETTYPE(@),
|||	    HOSTFS_CMD_SETVERSION(@), HOSTFS_CMD_DISMOUNTFS(@),
|||	    HOSTFS_CMD_SETBLOCKSIZE(@),
|||
**/

/**
|||	AUTODOC -class Device_Commands -group HostFS -name HOSTFS_CMD_ALLOCBLOCKS
|||	Controls the number of blocks allocated to a file on a remote host file
|||	system.
|||
|||	  Description
|||
|||	    This command lets you add or remove blocks from a file. The blocks
|||	    are always added to or removed from the end of the file. The
|||	    number of blocks in a file determines how much data is contained
|||	    in the file.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to HOSTFS_CMD_ALLOCBLOCKS
|||
|||	    ioi_Offset
|||	        Set to the number of blocks to add or remove from the file. A
|||	        positive value specifies blocks to add, while a negative value
|||	        specifies blocks to remove.
|||
|||	    ioi_CmdOptions
|||	        Reference token identifying the file to operate on,
|||	        as obtained using HOSTFS_CMD_OPENENTRY(@).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    HOSTFS_CMD_OPENENTRY(@), HOSTFS_CMD_CLOSEENTRY(@),
|||	    HOSTFS_CMD_CREATEFILE(@), HOSTFS_CMD_CREATEDIR(@),
|||	    HOSTFS_CMD_DELETEENTRY(@), HOSTFS_CMD_READENTRY(@)
|||	    HOSTFS_CMD_READDIR(@), HOSTFS_CMD_MOUNTFS(@),
|||	    HOSTFS_CMD_BLOCKREAD(@), HOSTFS_CMD_STATUS(@),
|||	    HOSTFS_CMD_FSSTAT(@), HOSTFS_CMD_BLOCKWRITE(@),
|||	    HOSTFS_CMD_SETEOF(@), HOSTFS_CMD_SETTYPE(@),
|||	    HOSTFS_CMD_SETVERSION(@), HOSTFS_CMD_DISMOUNTFS(@),
|||	    HOSTFS_CMD_SETBLOCKSIZE(@),
|||
**/

/**
|||	AUTODOC -class Device_Commands -group HostFS -name HOSTFS_CMD_BLOCKREAD
|||	Reads data from an opened file on a remote host file system.
|||
|||	  Description
|||
|||	    This command lets you read blocks of data from an opened file on a
|||	    remote file system.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to HOSTFS_CMD_BLOCKREAD
|||
|||	    ioi_Offset
|||	        Specifies the block number where to start reading. This value
|||	        must not be greater than the number of blocks currently in
|||	        the file.
|||
|||	    ioi_Recv.iob_Buffer
|||	        Buffer where the data should be placed.
|||
|||	    ioi_Recv.iob_Len
|||	        Number of bytes of data to read. This must be a
|||	        multiple of the file's block size. The block size can
|||	        be determined by using the HOSTFS_CMD_STATUS(@) command.
|||
|||	    ioi_CmdOptions
|||	        Reference token identifying the file to read from,
|||	        as obtained using HOSTFS_CMD_OPENENTRY(@).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    HOSTFS_CMD_OPENENTRY(@), HOSTFS_CMD_CLOSEENTRY(@),
|||	    HOSTFS_CMD_CREATEFILE(@), HOSTFS_CMD_CREATEDIR(@),
|||	    HOSTFS_CMD_DELETEENTRY(@), HOSTFS_CMD_READENTRY(@)
|||	    HOSTFS_CMD_READDIR(@), HOSTFS_CMD_ALLOCBLOCKS(@),
|||	    HOSTFS_CMD_MOUNTFS(@), HOSTFS_CMD_STATUS(@),
|||	    HOSTFS_CMD_FSSTAT(@), HOSTFS_CMD_BLOCKWRITE(@),
|||	    HOSTFS_CMD_SETEOF(@), HOSTFS_CMD_SETTYPE(@),
|||	    HOSTFS_CMD_SETVERSION(@), HOSTFS_CMD_DISMOUNTFS(@),
|||	    HOSTFS_CMD_SETBLOCKSIZE(@),
|||
**/

/**
|||	AUTODOC -class Device_Commands -group HostFS -name HOSTFS_CMD_BLOCKWRITE
|||	Writes data to a remote host file system.
|||
|||	  Description
|||
|||	    This command lets you write blocks of data to a remote host file
|||	    system.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to HOSTFS_CMD_BLOCKWRITE
|||
|||	    ioi_Offset
|||	        Specifies the block number where to start writing. This value
|||	        must not be greater than the number of blocks currently in
|||	        the file.
|||
|||	    ioi_Send.iob_Buffer
|||	        Buffer where the data to write can be found.
|||
|||	    ioi_Recv.iob_Len
|||	        Number of bytes of data to write. This must be a
|||	        multiple of the file's block size. The block size can
|||	        be determined by using the HOSTFS_CMD_STATUS(@) command.
|||
|||	    ioi_CmdOptions
|||	        Reference token identifying the file to write to,
|||	        as obtained using HOSTFS_CMD_OPENENTRY(@).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    HOSTFS_CMD_OPENENTRY(@), HOSTFS_CMD_CLOSEENTRY(@),
|||	    HOSTFS_CMD_CREATEFILE(@), HOSTFS_CMD_CREATEDIR(@),
|||	    HOSTFS_CMD_DELETEENTRY(@), HOSTFS_CMD_READENTRY(@)
|||	    HOSTFS_CMD_READDIR(@), HOSTFS_CMD_ALLOCBLOCKS(@),
|||	    HOSTFS_CMD_BLOCKREAD(@), HOSTFS_CMD_STATUS(@),
|||	    HOSTFS_CMD_FSSTAT(@), HOSTFS_CMD_MOUNTFS(@),
|||	    HOSTFS_CMD_SETEOF(@), HOSTFS_CMD_SETTYPE(@),
|||	    HOSTFS_CMD_SETVERSION(@), HOSTFS_CMD_DISMOUNTFS(@),
|||	    HOSTFS_CMD_SETBLOCKSIZE(@),
|||
**/

/**
|||	AUTODOC -class Device_Commands -group HostFS -name HOSTFS_CMD_STATUS
|||	Obtains information about an opened entry on a remote host file system.
|||
|||	  Description
|||
|||	    This command returns information about an opened entry within a
|||	    remote host file system. The information is returned in a
|||	    FileStatus structure as defined in <file/filesystem.h>
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to HOSTFS_CMD_STATUS
|||
|||	    ioi_Recv.iob_Buffer
|||	        Points to a FileStatus structure where the information on
|||	        the entry will be stored.
|||
|||	    ioi_Recv.iob_Len
|||	        Set to sizeof(FileStatus)
|||
|||	    ioi_CmdOptions
|||	        Reference token identifying the object to get information on,
|||	        as obtained using HOSTFS_CMD_OPENENTRY(@).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <file/filesystem.h>, <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    HOSTFS_CMD_OPENENTRY(@), HOSTFS_CMD_CLOSEENTRY(@),
|||	    HOSTFS_CMD_CREATEFILE(@), HOSTFS_CMD_CREATEDIR(@),
|||	    HOSTFS_CMD_DELETEENTRY(@), HOSTFS_CMD_READENTRY(@)
|||	    HOSTFS_CMD_READDIR(@), HOSTFS_CMD_ALLOCBLOCKS(@),
|||	    HOSTFS_CMD_BLOCKREAD(@), HOSTFS_CMD_MOUNTFS(@),
|||	    HOSTFS_CMD_FSSTAT(@), HOSTFS_CMD_BLOCKWRITE(@),
|||	    HOSTFS_CMD_SETEOF(@), HOSTFS_CMD_SETTYPE(@),
|||	    HOSTFS_CMD_SETVERSION(@), HOSTFS_CMD_DISMOUNTFS(@),
|||	    HOSTFS_CMD_SETBLOCKSIZE(@),
|||
**/

/**
|||	AUTODOC -class Device_Commands -group HostFS -name HOSTFS_CMD_FSSTAT
|||	Obtains information about a remote host file system.
|||
|||	  Description
|||
|||	    This command returns information about a remote host file system.
|||	    The information is returned in a FileSystemStat structure, as
|||	    defined in <file/filesystem.h>
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to HOSTFS_CMD_FSSTAT
|||
|||	    ioi_Recv.iob_Buffer
|||	        Points to a FileSystemStat structure where the information
|||	        about the remote file system will be stored.
|||
|||	    ioi_Recv.iob_Len
|||	        Set to sizeof(FileSystemStat).
|||
|||	    ioi_CmdOptions
|||	        Reference token for any object on the remote file system to
|||	        get information on.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <file/filesystem.h>, <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    HOSTFS_CMD_OPENENTRY(@), HOSTFS_CMD_CLOSEENTRY(@),
|||	    HOSTFS_CMD_CREATEFILE(@), HOSTFS_CMD_CREATEDIR(@),
|||	    HOSTFS_CMD_DELETEENTRY(@), HOSTFS_CMD_READENTRY(@)
|||	    HOSTFS_CMD_READDIR(@), HOSTFS_CMD_ALLOCBLOCKS(@),
|||	    HOSTFS_CMD_BLOCKREAD(@), HOSTFS_CMD_STATUS(@),
|||	    HOSTFS_CMD_MOUNTFS(@), HOSTFS_CMD_BLOCKWRITE(@),
|||	    HOSTFS_CMD_SETEOF(@), HOSTFS_CMD_SETTYPE(@),
|||	    HOSTFS_CMD_SETVERSION(@), HOSTFS_CMD_DISMOUNTFS(@),
|||	    HOSTFS_CMD_SETBLOCKSIZE(@),
|||
**/

/**
|||	AUTODOC -class Device_Commands -group HostFS -name HOSTFS_CMD_SETEOF
|||	Sets the logical byte count of a file on a remote host file system.
|||
|||	  Description
|||
|||	    Every file keeps a count of the number of valid bytes the file
|||	    contains. This command lets you set this count for an opened file
|||	    on a remote host file system.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to HOSTFS_CMD_SETEOF
|||
|||	    ioi_Offset
|||	        Specifies the byte count to set for the opened file.
|||
|||	    ioi_CmdOptions
|||	        Reference token identifying the file to set the size off,
|||	        as obtained using HOSTFS_CMD_OPENENTRY(@).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    HOSTFS_CMD_OPENENTRY(@), HOSTFS_CMD_CLOSEENTRY(@),
|||	    HOSTFS_CMD_CREATEFILE(@), HOSTFS_CMD_CREATEDIR(@),
|||	    HOSTFS_CMD_DELETEENTRY(@), HOSTFS_CMD_READENTRY(@)
|||	    HOSTFS_CMD_READDIR(@), HOSTFS_CMD_ALLOCBLOCKS(@),
|||	    HOSTFS_CMD_BLOCKREAD(@), HOSTFS_CMD_STATUS(@),
|||	    HOSTFS_CMD_FSSTAT(@), HOSTFS_CMD_BLOCKWRITE(@),
|||	    HOSTFS_CMD_MOUNTFS(@), HOSTFS_CMD_SETTYPE(@),
|||	    HOSTFS_CMD_SETVERSION(@), HOSTFS_CMD_DISMOUNTFS(@),
|||	    HOSTFS_CMD_SETBLOCKSIZE(@),
|||
**/

/**
|||	AUTODOC -class Device_Commands -group HostFS -name HOSTFS_CMD_SETDATE
|||	Sets the modification date of a file or directory on a remote host file
|||	system.
|||
|||	  Description
|||
|||	    This command sets the modification date of a file or directory.
|||	    This command lets you set this date for an opened file
|||	    on a remote host file system.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to HOSTFS_CMD_SETDATE
|||
|||	    ioi_Send.iob_Buffer
|||	        Pointer to a TimeVal structure specifying the new modification
|||	        date for the file or directory. The time specified is relative
|||	        to 01-Jan-1993
|||
|||	    ioi_Send.iob_Len
|||	        Set to sizeof(TimeVal).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V33.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    HOSTFS_CMD_OPENENTRY(@), HOSTFS_CMD_CLOSEENTRY(@),
|||	    HOSTFS_CMD_CREATEFILE(@), HOSTFS_CMD_CREATEDIR(@),
|||	    HOSTFS_CMD_DELETEENTRY(@), HOSTFS_CMD_READENTRY(@)
|||	    HOSTFS_CMD_READDIR(@), HOSTFS_CMD_ALLOCBLOCKS(@),
|||	    HOSTFS_CMD_BLOCKREAD(@), HOSTFS_CMD_STATUS(@),
|||	    HOSTFS_CMD_FSSTAT(@), HOSTFS_CMD_BLOCKWRITE(@),
|||	    HOSTFS_CMD_MOUNTFS(@), HOSTFS_CMD_SETTYPE(@),
|||	    HOSTFS_CMD_SETVERSION(@), HOSTFS_CMD_DISMOUNTFS(@),
|||	    HOSTFS_CMD_SETBLOCKSIZE(@), HOSTFS_CMD_SETEOF(@)
|||
**/

/**
|||	AUTODOC -class Device_Commands -group HostFS -name HOSTFS_CMD_SETTYPE
|||	Sets the modification date of a file or directory.
|||
|||	  Description
|||
|||	    This command sets the modification date of a file or directory.
|||	    Target file or directory must have been previously opened through
|||	    OpenFile().
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to HOSTFS_CMD_SETDATE
|||
|||	    ioi_Send.iob_Buffer
|||	        Pointer to a TimeVal structure specifying the new modification
|||	        date for the file or directory. The time specified is relative
|||	        to 01-Jan-1993
|||
|||	    ioi_Send.iob_Len
|||	        Set to sizeof(TimeVal).
|||
|||	    ioi_CmdOptions
|||	        Reference token identifying the object to set the type of,
|||	        as obtained using HOSTFS_CMD_OPENENTRY(@).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V33.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    HOSTFS_CMD_OPENENTRY(@), HOSTFS_CMD_CLOSEENTRY(@),
|||	    HOSTFS_CMD_CREATEFILE(@), HOSTFS_CMD_CREATEDIR(@),
|||	    HOSTFS_CMD_DELETEENTRY(@), HOSTFS_CMD_READENTRY(@),
|||	    HOSTFS_CMD_READDIR(@), HOSTFS_CMD_ALLOCBLOCKS(@),
|||	    HOSTFS_CMD_BLOCKREAD(@), HOSTFS_CMD_STATUS(@),
|||	    HOSTFS_CMD_FSSTAT(@), HOSTFS_CMD_BLOCKWRITE(@),
|||	    HOSTFS_CMD_MOUNTFS(@),
|||	    HOSTFS_CMD_SETVERSION(@), HOSTFS_CMD_DISMOUNTFS(@),
|||	    HOSTFS_CMD_SETBLOCKSIZE(@)
|||
**/

/**
|||	AUTODOC -class Device_Commands -group HostFS -name HOSTFS_CMD_SETVERSION
|||	Set the version and revision codes for an opened entry on a remote host
|||	file system.
|||
|||	  Description
|||
|||	    Every file system object has a version and revision code associated
|||	    with it. This command lets you set these values for an opened entry
|||	    on a remote host file system.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to HOSTFS_CMD_SETVERSION
|||
|||	    ioi_Offset
|||	        Specifies the version and revision code for the object.
|||	        The two values are 8 bit wide and pack like:
|||	        (version << 8) | revision.
|||
|||	    ioi_CmdOptions
|||	        Reference token identifying the object to set the version of,
|||	        as obtained using HOSTFS_CMD_OPENENTRY(@).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    HOSTFS_CMD_OPENENTRY(@), HOSTFS_CMD_CLOSEENTRY(@),
|||	    HOSTFS_CMD_CREATEFILE(@), HOSTFS_CMD_CREATEDIR(@),
|||	    HOSTFS_CMD_DELETEENTRY(@), HOSTFS_CMD_READENTRY(@)
|||	    HOSTFS_CMD_READDIR(@), HOSTFS_CMD_ALLOCBLOCKS(@),
|||	    HOSTFS_CMD_BLOCKREAD(@), HOSTFS_CMD_STATUS(@),
|||	    HOSTFS_CMD_FSSTAT(@), HOSTFS_CMD_BLOCKWRITE(@),
|||	    HOSTFS_CMD_SETEOF(@), HOSTFS_CMD_SETTYPE(@),
|||	    HOSTFS_CMD_MOUNTFS(@), HOSTFS_CMD_DISMOUNTFS(@),
|||	    HOSTFS_CMD_SETBLOCKSIZE(@),
|||
**/

/**
|||	AUTODOC -class Device_Commands -group HostFS -name HOSTFS_CMD_SETBLOCKSIZE
|||	Sets the device block size for a particular IOReq dealing with a
|||	file on a remote host file system.
|||
|||	  Description
|||
|||	    This command lets you set the block size used when reading
|||	    or writing data using the current IOReq. If this command is not
|||	    set, then the default size is 8K. The proper value to be set here
|||	    is determined by sending a HOSTFS_CMD_STATUS(@) command to the
|||	    opened entry, and extracting the block size from the FileStatus
|||	    structure.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to HOSTFS_CMD_SETBLOCKSIZE.
|||
|||	    ioi_Offset
|||	        The block size to use for this IOReq.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>
|||
|||	  See Also
|||
|||	    HOSTFS_CMD_OPENENTRY(@), HOSTFS_CMD_CLOSEENTRY(@),
|||	    HOSTFS_CMD_CREATEFILE(@), HOSTFS_CMD_CREATEDIR(@),
|||	    HOSTFS_CMD_DELETEENTRY(@), HOSTFS_CMD_READENTRY(@)
|||	    HOSTFS_CMD_READDIR(@), HOSTFS_CMD_ALLOCBLOCKS(@),
|||	    HOSTFS_CMD_BLOCKREAD(@), HOSTFS_CMD_STATUS(@),
|||	    HOSTFS_CMD_FSSTAT(@), HOSTFS_CMD_BLOCKWRITE(@),
|||	    HOSTFS_CMD_SETEOF(@), HOSTFS_CMD_SETTYPE(@),
|||	    HOSTFS_CMD_MOUNTFS(@), HOSTFS_CMD_DISMOUNTFS(@),
|||	    HOSTFS_CMD_MOUNTFS(@),
|||
**/

/**
|||	AUTODOC -class Device_Commands -group HostConsole -name HOSTCONSOLE_CMD_GETCMDLINE
|||	Gets command-line input from the remote host.
|||
|||	  Description
|||
|||	    This command lets you request that the host prompt the user for an
|||	    input string. When the input string is entered, it is returned to
|||	    you.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to HOSTCONSOLE_CMD_GETCMDLINE
|||
|||	    ioi_Send.iob_Buffer
|||	        Pointer to a NULL-terminated string that will be used to prompt
|||	        the user for input.
|||
|||	    ioi_Send.iob_Len
|||	        Length of the prompt string, including the NULL-terminator.
|||
|||	    ioi_Recv.iob_Buffer
|||	        Buffer where the NULL-terminated returned command-line will
|||	        be put.
|||
|||	    ioi_Recv.iob_Len
|||	        Maximum length of the returned command-line, including the
|||	        NULL-terminator.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>
|||
**/

/**
|||	AUTODOC -private -class Device_Commands -group uSlot -name USLOTCMD_RESET
|||	Resets a micro-slot.
|||
|||	  Description
|||
|||	    This command sends an OutReset to the specified slot.  Normally,
|||	    this causes the card in the slot to enter its power-on state.
|||	    One side effect is that the card will assert the Attention bit
|||	    in the CDE MicroSlotStatus register, thus causing a Dipir event.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to USLOTCMD_RESET.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V28.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/microslot.h>
|||
|||	  See Also
|||
|||	    USLOTCMD_SETCLOCK(@)
|||
**/

/**
|||	AUTODOC -private -class Device_Commands -group uSlot -name USLOTCMD_SETCLOCK
|||	Sets the clock rate of the micro-slot.
|||
|||	  Description
|||
|||	    This command sets the clock rate of the micro-slot hardware to
|||	    one of the specified values.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to USLOTCMD_SETCLOCK.
|||
|||	    ioi_CmdOptions
|||	        Set to one of:
|||
|||	       USLOTCLK_1MHZ
|||	          Set the clock speed to 1MHz.
|||
|||	       USLOTCLK_2MHZ
|||	          Set the clock speed to 2MHz.
|||
|||	       USLOTCLK_4MHZ
|||	          Set the clock speed to 4MHz.
|||
|||	       USLOTCLK_8MHZ
|||	          Set the clock speed to 8MHz.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V28.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/microslot.h>
|||
|||	  Notes
|||
|||	    This command affects all slots.
|||
|||	  See Also
|||
|||	    USLOTCMD_RESET(@)
|||
**/

/**
|||	AUTODOC -private -class Device_Commands -group uSlot -name USLOTCMD_WRITEV
|||	Perform a vectored-write operation.
|||
|||	  Description
|||
|||	    This command performs a vectored-write operation by interpreting
|||	    the contents of the ioi_Send buffer as a list of pointer/length
|||	    pairs terminated by a null pointer.  This allows multiple writes
|||	    to occur in an atomic fashion to the specified slot.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to USLOTCMD_WRITEV.
|||
|||	    ioi_Send.iob_Buffer
|||	        Pointer to a buffer containing pointer/length pairs.  Each
|||	        of these pointers is validated in the same way that a
|||	        normal write is.  The last pointer must be null and need not
|||	        have an associated length.
|||
|||	    ioi_Send.iob_Len
|||	        Set to the size of the buffer holding the pointer/length
|||	        pairs.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V28.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/microslot.h>
|||
|||	  Notes
|||
|||	    The total number of bytes written to the slot is returned in
|||	    io_Actual.  If the operation completes successfully, this will
|||	    equal the sum of the buffer lengths.
|||
|||	  See Also
|||
|||	    USLOTCMD_WRITETHENREAD(@)
|||
**/

/**
|||	AUTODOC -private -class Device_Commands -group uSlot -name USLOTCMD_WRITETHENREAD
|||	Perform a write-read operation.
|||
|||	  Description
|||
|||	    This command performs a write operation followed by a read
|||	    operation in an atomic fashion to the specified slot.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to USLOTCMD_WRITETHENREAD.
|||
|||	    ioi_Send.iob_Buffer
|||	        Pointer to a buffer containing the data to be written.
|||
|||	    ioi_Send.iob_Len
|||	        Set to the number of bytes to write to the slot.  This may
|||	        not be greater than the size of the send buffer.
|||
|||	    ioi_Recv.iob_Buffer
|||	        Pointer to a buffer that will accept the data from the slot.
|||
|||	    ioi_Recv.iob_Len
|||	        Set to the number of bytes to read from the slot.  This may
|||	        not be greater than the size of the receive buffer.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V28.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/microslot.h>
|||
|||	  Notes
|||
|||	    The sum of total number of bytes written to the slot and the
|||	    total number of bytes read from the slot is returned in io_Actual.
|||	    If the operation completes successfully, this will equal the sum
|||	    of the buffer lengths.  If the write part of this command does
|||	    not complete successfully, no data are read from the slot.
|||
|||	  See Also
|||
|||	    USLOTCMD_WRITEV(@), USLOTCMD_WRITETHENVERIFY(@)
|||
**/

/**
|||	AUTODOC -private -class Device_Commands -group uSlot -name USLOTCMD_WRITETHENVERIFY
|||	Perform a write-verify operation.
|||
|||	  Description
|||
|||	    This command performs a write operation followed by a verify
|||	    operation in an atomic fashion to the specified slot.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to USLOTCMD_WRITETHENVERIFY.
|||
|||	    ioi_Send.iob_Buffer
|||	        Pointer to a buffer containing the data to be verified.
|||
|||	    ioi_Send.iob_Len
|||	        Set to the number of bytes to verify.  This may not be
|||	        greater than the size of the verify buffer.
|||
|||	    ioi_CmdOptions
|||	        Pointer to a buffer containing the data to be written.
|||
|||	    ioi_Offset
|||	        Set to the number of bytes to write to the slot.  This may
|||	        not be greater than the size of the send buffer.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V29.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/microslot.h>
|||
|||	  Notes
|||
|||	    The sum of total number of bytes written to the slot and the
|||	    total number of bytes verified is returned in io_Actual.  If
|||	    the operation completes successfully, this will equal the sum
|||	    of the buffer lengths.  If the write part of this command does
|||	    not complete successfully, no data are verified.
|||
|||	  See Also
|||
|||	    USLOTCMD_WRITETHENREAD(@)
|||
**/

/**
|||	AUTODOC -private -class Device_Commands -group Slot -name SLOTCMD_SETTIMING
|||	Sets low-level timing characteristics of a slot.
|||
|||	  Description
|||
|||	    This command sets the low-level hardware timing characteristics
|||	    of a slot.  After successful completion of this command, all
|||	    future accesses to the slot will use the new timings.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to SLOTCMD_SETTIMING.
|||
|||	    ioi_Send.iob_Buffer
|||	        Pointer to a SlotTiming structure.  The fields of the
|||	        structure are initialized as follows.  If any of the
|||	        fields is set to the value ST_NOCHANGE, that particular
|||	        characteristic is left unchanged.
|||
|||	    st_DeviceWidth
|||	        Set to the width of the slot in bits (8, 16 or 32).
|||	    st_CycleTime
|||	        Set to the desired device cycle time, in nanoseconds.
|||	    st_PageModeCycleTime
|||	        Set to the desired device page-mode cycle time, in
|||	        nanoseconds.
|||	    st_ReadSetupTime
|||	        Set to the desired read setup time, in nanoseconds.
|||	    st_ReadHoldTime
|||	        Set to the desired read hold time, in nanoseconds.
|||	    st_WriteSetupTime
|||	        Set to the desired write setup time, in nanoseconds.
|||	    st_WriteHoldTime
|||	        Set to the desired write hold time, in nanoseconds.
|||	    st_IOReadSetupTime
|||	        Set to the desired I/O read setup time, in nanoseconds.
|||	    st_Flags
|||	        The ST_PAGE_MODE bit enables page-mode accesses to
|||	        the slot.  The ST_SYNCA_MODE bit enables synchronous
|||	        (MODEA) accesses to the slot.  The ST_HIDEA_MODE bit
|||	        enables address-hidden accesses to the slot.
|||
|||	    ioi_Send.iob_Len
|||	        Set to sizeof(SlotTiming).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/slot.h>
|||
**/

/**
|||	AUTODOC -class Device_Commands -group Serial -name SER_CMD_BREAK
|||	Sends a break signal over the serial line.
|||
|||	  Description
|||
|||	    This command sends a break signal (serial line held low for an
|||	    extended period). The length of the break signal can be specified
|||	    by the client.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to SER_CMD_BREAK.
|||
|||	    ioi_CmdOptions
|||	        Specifies the duration of the break signal in microseconds.
|||	        If this value is set to 0, a default is assumed.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V29.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/serial.h>
|||
|||	  See Also
|||
|||	    CMD_STREAMWRITE(@), CMD_STREAMREAD(@)
|||
**/

/**
|||	AUTODOC -class Device_Commands -group Serial -name SER_CMD_WAITEVENT
|||	Waits for serial events to occur.
|||
|||	  Description
|||
|||	    This command lets you wait for specific events to occur. You can
|||	    wait for multiple events to occur. The IOReq is returned to you
|||	    when any of the events occur, and the io_Actual field specifies
|||	    which events occured.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to SER_CMD_WAITEVENT.
|||
|||	    ioi_CmdOptions
|||	        Specifies a bitmask of the events to wait for. This mask is
|||	        constructed using the various SER_EVENT_* constants defined in
|||	        <device/serial.h>.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V29.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/serial.h>
|||
|||	  See Also
|||
|||	    SER_CMD_STATUS(@)
|||
**/

/**
|||	AUTODOC -class Device_Commands -group Serial -name SER_CMD_SETRTS
|||	Controls the state of the serial RTS line.
|||
|||	  Description
|||
|||	    This command lets you control the state of the serial RTS line. The
|||	    line can be made to go high or low.
|||
|||	    This command can only be used when the driver is configured for
|||	    no handshaking or software handshaking. The driver takes over the
|||	    RTS line when hardware handshaking is enabled.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to SER_CMD_SETRTS.
|||
|||	    ioi_CmdOptions
|||	        Set this field to 1 to set RTS or 0 to clear RTS.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V29.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/serial.h>
|||
|||	  See Also
|||
|||	    SER_CMD_STATUS(@), SER_CMD_SETDTR(@), SER_CMD_SETLOOPBACK(@)
|||
**/

/**
|||	AUTODOC -class Device_Commands -group Serial -name SER_CMD_SETDTR
|||	Controls the state of the serial DTR line.
|||
|||	  Description
|||
|||	    This command lets you control the state of the serial DTR line. The
|||	    line can be made to go high or low. When the device is first
|||	    initialized, the DTR line is automatically held high. When the
|||	    device is shutting down, DTR is automatically dropped low.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to SER_CMD_SETDTR.
|||
|||	    ioi_CmdOptions
|||	        Set this field to 1 to set DTR or 0 to clear DTR.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V29.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/serial.h>
|||
|||	  See Also
|||
|||	    SER_CMD_STATUS(@), SER_CMD_SETRTS(@)
|||
**/

/**
|||	AUTODOC -class Device_Commands -group Serial -name SER_CMD_SETLOOPBACK
|||	Controls the state of automatic serial loopback mode.
|||
|||	  Description
|||
|||	    This command lets you control serial loopback, which pumps
|||	    outgoing data back into the serial port. This is sometimes useful
|||	    for diagnostic and for developing code without having a terminal
|||	    hooked up.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to SER_CMD_SETLOOPBACK.
|||
|||	    ioi_CmdOptions
|||	        Set to 1 to turn loopback mode on or 0 to set loopback mode off.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V29.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/serial.h>
|||
|||	  See Also
|||
|||	    SER_CMD_STATUS(@), SER_CMD_SETRTS(@), SER_CMD_SETDTR(@)
|||
**/

/**
|||	AUTODOC -class Device_Commands -group Serial -name SER_CMD_SETCONFIG
|||	Sets the configuration of the serial port.
|||
|||	  Description
|||
|||	    This command lets you set various serial attributes. Sending this
|||	    command has the effect of resetting the serial port.
|||
|||	    The default configuration for a serial port is:
|||
|||	      sc_BaudRate           = 57600;
|||	      sc_Handshake          = SER_HANDSHAKE_NONE;
|||	      sc_WordLength         = SER_WORDLENGTH_8;
|||	      sc_Parity             = SER_PARITY_NONE;
|||	      sc_StopBits           = SER_STOPBITS_1;
|||	      sc_OverflowBufferSize = 0;
|||
|||	    The sc_OverflowBufferSize field lets you specify the size in bytes
|||	    of a buffer maintained by the driver to deal with read overflow
|||	    conditions. This happens when data is coming into the serial port
|||	    and there is no IOReq present to read the data. It is
|||	    more efficient to use IOReq double-buffering rather than to rely
|||	    on the driver to buffer data.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to SER_CMD_SETCONFIG.
|||
|||	    ioi_Send.iob_Buffer
|||	        Points to an initialized SerConfig structure as defined in
|||	        <device/serial.h>, specifying the new serial configuration
|||	        parameters.
|||
|||	    ioi_Send.iob_Len
|||	        Set to sizeof(SerConfig).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V29.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/serial.h>
|||
|||	  See Also
|||
|||	    SER_CMD_GETCONFIG(@)
|||
**/

/**
|||	AUTODOC -class Device_Commands -group Serial -name SER_CMD_GETCONFIG
|||	Determines the current serial port settings.
|||
|||	  Description
|||
|||	    This command returns the current serial port settings in a
|||	    SerConfig structure.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to SER_CMD_GETCONFIG.
|||
|||	    ioi_Recv.iob_Buffer
|||	        Points to a SerConfig structure as defined in <device/serial.h>,
|||	        where the current serial configuration parameters will be
|||	        stored.
|||
|||	    ioi_Recv.iob_Len
|||	        Set to sizeof(SerConfig).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V29.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/serial.h>
|||
|||	  See Also
|||
|||	    SER_CMD_SETCONFIG(@)
|||
**/

/**
|||	AUTODOC -class Device_Commands -group Serial -name SER_CMD_STATUS
|||	Gets the state of various serial attributes.
|||
|||	  Description
|||
|||	    This command returns information about the current state of the
|||	    serial port. This includes the number of errors encountered since
|||	    the last reset, as well as the state of certain serial lines.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to SER_CMD_STATUS.
|||
|||	    ioi_Recv.iob_Buffer
|||	        Points to a SerStatus structure as defined in <device/serial.h>,
|||	        where the status information will be stored.
|||
|||	    ioi_Recv.iob_Len
|||	        Set to sizeof(SerStatus).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V29.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/serial.h>
|||
|||	  See Also
|||
|||	    SER_CMD_SETRTS(@), SER_CMD_SETDTR(@), SER_CMD_SETLOOPBACK(@),
|||	    SER_CMD_WAITEVENT(@)
|||
**/

/**
|||	AUTODOC -class Device_Commands -group MP -name MP_CMD_DISPATCH
|||	Gets the slave processor to execute some code.
|||
|||	  Description
|||
|||	    This command lets you run some code on the slave processor. You
|||	    supply a pointer to the code to run, a stack for the code to use,
|||	    a parameter to pass to the function, and a place where to store a
|||	    return value.
|||
|||	    The second processor starts executing your code as soon as possible.
|||	    When execution completes, the IOReq is returned to you. Execution
|||	    on the slave completes when the slave function returns, or when the
|||	    slave function calls exit().
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to MP_CMD_DISPATCH.
|||
|||	    ioi_Send.iob_Buffer
|||	        Points to an initialized MPPacket structure. This structure
|||	        specifies the code to run, the single parameter to pass to the
|||	        function, and the stack to use for the function. Note that
|||	        the stack pointer is actually a pointer to the topmost address
|||	        of the stack plus 1.
|||
|||	    ioi_Send.iob_Len
|||	        Set to sizeof(MPPacket).
|||
|||	    ioi_Recv.iob_Buffer
|||	        Points to a variable of type Err to hold the return value from
|||	        the function being executed.
|||
|||	    ioi_Recv.iob_Len
|||	        Set to sizeof(Err).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V31.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/mp.h>
|||
**/

/* keep the compiler happy... */
extern int foo;
