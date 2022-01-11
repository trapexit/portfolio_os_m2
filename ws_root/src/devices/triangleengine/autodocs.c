/* @(#) autodocs.c 96/11/14 1.7 */

/**
|||	AUTODOC -class Device_Commands -group TriangleEngine -name TE_CMD_GETIDLETIME
|||	Obtains the total amount of time the triangle engine has been idle.
|||
|||	  Description
|||
|||	    This command lets you query the driver to see how long the
|||	    triangle engine hardware has been idle. This can be useful when
|||	    tuning rendering performance.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to TE_CMD_GETIDLETIME.
|||
|||	    ioi_Recv.iob_Buffer
|||	        Pointer to the TimerTicks structure to receive the amount of
|||	        idle time.
|||
|||	    ioi_Send.iob_Len
|||	        Set to the sizeof(TimerTicks).
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V31.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/te.h>
|||
|||	  See Also
|||
|||	    TE_CMD_SETFRAMEBUFFER(@), TE_CMD_SETZBUFFER(@),
|||	    TE_CMD_SETVBLABORTCOUNT(@), TE_CMD_DISPLAYFRAMEBUFFER(@),
|||	    TE_CMD_STEP(@), TE_CMD_SPEEDCONTROL(@), TE_CMD_EXECUTELIST(@)
|||
**/

/**
|||	AUTODOC -class Device_Commands -group TriangleEngine -name TE_CMD_EXECUTELIST
|||	Submits a command list to be executed by the triangle engine.
|||
|||	  Description
|||
|||	    This command lets you dispatch a command list to be executed by
|||	    the triangle engine. The triangle engine is directed to render
|||	    into the current frame buffer and Z buffer, as set using the
|||	    TE_CMD_SETFRAMEBUFFER(@) and TE_CMD_SETZBUFFER(@) commands.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to TE_CMD_EXECUTELIST.
|||
|||	    ioi_Send.iob_Buffer
|||	        Pointer to the list of triangle engine commands to execute.
|||
|||	    ioi_Send.iob_Len
|||	        Set to the number of bytes in the command list to execute.
|||
|||	    ioi_CmdOptions
|||	        Contains a set of bit flags that provide certain controls over
|||	        the rendering operation. See below.
|||
|||	  Options
|||
|||	    These are the option flags that can be specified in the
|||	    ioi_CmdOptions field:
|||
|||	    TE_CLEAR_FRAME_BUFFER
|||	        Before executing the supplied command list, the driver will
|||	        insert its own short command list to clear the current frame
|||	        buffer to the currently defined R, G, B, and A values as
|||	        specified with TE_CMD_SETFRAMEBUFFER(@).
|||
|||	    TE_CLEAR_Z_BUFFER
|||	        Before executing the supplied command list, the driver will
|||	        insert its own short command list to clear the current Z
|||	        buffer to the currently defined Z value as
|||	        specified with TE_CMD_SETZBUFFER(@).
|||
|||	    TE_WAIT_UNTIL_OFFSCREEN
|||	        When this option is set, execution of this and subsequent
|||	        command lists will be deferred until the target frame buffer
|||	        is marked as being offscreen by the graphics folio. This
|||	        guarantees that no rendering is performed to a frame buffer
|||	        being displayed to the user.
|||
|||	    TE_ABORT_AT_VBLANK
|||	        When this option is set, it causes the driver to stop the
|||	        triangle engine if a vertical blank interrupt occurs while
|||	        this IOReq's command list is being executed. This lets you
|||	        schedule optional rendering operations, which proceed for as
|||	        long as possible until the frame needs to be displayed to the
|||	        user.
|||
|||	        Only the current command is affected by this feature (that
|||	        is, the abort does not cascade through the pending IOReqs
|||	        until the next TE_CMD_DISPLAYFRAMEBUFFER).
|||
|||	        If/when the abort occurs, the state of the triangle engine
|||	        after the abort is guaranteed to be random.
|||
|||	    TE_LIST_FLUSHED
|||	        Specifies that the supplied command list is already guaranteed
|||	        to be in main memory where the triangle engine can access it.
|||	        When this bit is not set, the driver takes action to flush
|||	        the CPU data cache to guarantee the command list hits main
|||	        memory.
|||
|||	  Return Value
|||
|||	    Upon completion, this command returns some status information in
|||	    the io_Actual field of the IOReq. See the TE_STATUS_* bits defined
|||	    in <device/te.h> for details.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V31.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/te.h>
|||
|||	  See Also
|||
|||	    TE_CMD_SETFRAMEBUFFER(@), TE_CMD_SETZBUFFER(@),
|||	    TE_CMD_SETVBLABORTCOUNT(@), TE_CMD_DISPLAYFRAMEBUFFER(@),
|||	    TE_CMD_STEP(@), TE_CMD_SPEEDCONTROL(@), TE_CMD_GETIDLETIME(@)
|||
**/

/**
|||	AUTODOC -class Device_Commands -group TriangleEngine -name TE_CMD_SETFRAMEBUFFER
|||	Sets the frame buffer for the triangle engine to render to.
|||
|||	  Description
|||
|||	    This command determines which frame buffer the triangle engine
|||	    should be rendering to for subsequent command lists.
|||
|||	    This command queues along with the other commands such as
|||	    TE_CMD_EXECUTELIST(@). As a result, the command affects the frame
|||	    buffer used for command lists submitted after this command is sent
|||	    to the driver. So rendering operations already in the driver's
|||	    queue are not affected.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to TE_CMD_SETFRAMEBUFFER.
|||
|||	    ioi_Offset
|||	        Set to the item number of the bitmap to use as a frame buffer.
|||	        If this value is set to -1, then the triangle engine's
|||	        rendering is suppressed.
|||
|||	    ioi_Send.iob_Buffer
|||	        Pointer to an optional TEFrameBufferInfo structure which
|||	        specifies some additional information about the frame buffer.
|||	        This information currently specifies the background clear
|||	        color to be used when the TE_CLEAR_FRAME_BUFFER option is
|||	        used with the TE_CMD_EXECUTELIST(@) command. If this pointer
|||	        is NULL, the current values for the frame buffer info are
|||	        not changed.
|||
|||	    ioi_Send.iob_Len
|||	        Set to sizeof(TEFrameBufferInfo), or 0 if no frame buffer info
|||	        structure is being supplied.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V31.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/te.h>
|||
|||	  See Also
|||
|||	    TE_CMD_EXECUTELIST(@), TE_CMD_SETZBUFFER(@), TE_CMD_GETIDLETIME(@)
|||	    TE_CMD_DISPLAYFRAMEBUFFER(@), TE_CMD_STEP(@), TE_CMD_SPEEDCONTROL(@)
|||
**/

/**
|||	AUTODOC -class Device_Commands -group TriangleEngine -name TE_CMD_SETZBUFFER
|||	Sets the Z buffer for the triangle engine to use.
|||
|||	  Description
|||
|||	    This command determines which Z buffer the triangle engine should
|||	    be using for subsequent command lists.
|||
|||	    This command queues along with the other commands such as
|||	    TE_CMD_EXECUTELIST(@). As a result, the command affects the Z
|||	    buffer used for command lists submitted after this command is sent
|||	    to the driver. So rendering operations already in the driver's
|||	    queue are not affected.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to TE_CMD_SETZBUFFER.
|||
|||	    ioi_Offset
|||	        Set to the item number of the bitmap to use as a Z buffer.
|||	        If this value is set to -1, then the triangle engine does not
|||	        perform any Z buffering.
|||
|||	    ioi_Send.iob_Buffer
|||	        Pointer to an optional TEZBufferInfo structure which
|||	        specifies some additional information about the Z buffer.
|||	        This information currently specifies the value to be used
|||	        when the TE_CLEAR_Z_BUFFER option is used with the
|||	        TE_CMD_EXECUTELIST(@) command. If this pointer is NULL, the
|||	        current values for the Z buffer info are not changed.
|||
|||	    ioi_Send.iob_Len
|||	        Set to sizeof(TEZBufferInfo), or 0 if no Z buffer info
|||	        structure is being supplied.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V31.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/te.h>
|||
|||	  See Also
|||
|||	    TE_CMD_SETFRAMEBUFFER(@), TE_CMD_EXECUTELIST(@),, TE_CMD_GETIDLETIME(@)
|||	    TE_CMD_DISPLAYFRAMEBUFFER(@), TE_CMD_STEP(@), TE_CMD_SPEEDCONTROL(@)
|||
**/

/**
|||	AUTODOC -class Device_Commands -group TriangleEngine -name TE_CMD_SETVIEW
|||	Sets the View to which rendered Bitmaps should be attached.
|||
|||	  Description
|||
|||	    This command establishes the View Item to which Bitmaps are to
|||	    be attached when rendering is complete.  The Bitmap to be
|||	    attached is set using TE_CMD_SETFRAMEBUFFER(@).
|||
|||	    The Bitmap will be attached to the specified View when the
|||	    command TE_CMD_DISPLAYFRAMEBUFFER(@) is serviced.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to TE_CMD_SETVIEW.
|||
|||	    ioi_Offset
|||	        Set to the item number of the View to which rendered Bitmaps
|||	        are to be attached.  If this value is set to -1, no View
|||	        will be used.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V32.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/te.h>
|||
|||	  See Also
|||
|||	    TE_CMD_SETFRAMEBUFFER(@), TE_CMD_EXECUTELIST(@), TE_CMD_GETIDLETIME(@)
|||	    TE_CMD_DISPLAYFRAMEBUFFER(@), TE_CMD_STEP(@), TE_CMD_SPEEDCONTROL(@)
|||
**/

/**
|||	AUTODOC -class Device_Commands -group TriangleEngine -name TE_CMD_SETVBLABORTCOUNT
|||	Sets the number of vertical blanks to ignore before aborting command
|||	lists.
|||
|||	  Description
|||
|||	    This command establishes the number of vertical blanks to allow
|||	    to transpire before aborting a triangle engine command list
|||	    using the optional TE_ABORT_AT_VBLANK feature.
|||
|||	    When a command list is submitted for execution via
|||	    TE_CMD_EXECUTELIST(@), the client may optionally request that
|||	    the command list be aborted at the next vertical blank by
|||	    setting TE_ABORT_AT_VBLANK in the ioi_CmdOptions field.  This is
|||	    to support applications that desire a fixed frame rate, and want
|||	    to fill the frame with "optional" imagery after the required
|||	    imagery has been drawn.
|||
|||	    For commands utilizing this feature, this command exists to
|||	    specify the quantity of vertical blanks to ignore before
|||	    aborting the command list.  This is so applications may run at a
|||	    fixed frame rate that is an integral quotient of the video
|||	    refresh rate.
|||
|||	    The number of VBlanks to ignore is placed in the ioi_Offset
|||	    field of the IOInfo structure.  Supplying negative values will
|||	    generate an error.
|||
|||	    For each opened device, the default for this value is zero (i.e.
|||	    ignore no VBlanks; abort on next occurring one).  Properly used,
|||	    this would yield a frame rate of 60 Hz on an NTSC display.  If
|||	    it were set to one (1), a single VBlank would be ignored before
|||	    aborting on the second, yielding a frame rate of 30 Hz.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to TE_CMD_SETVBLABORTCOUNT.
|||
|||	    ioi_Offset
|||	        Set to the quantity of VBlanks to ignore before aborting a
|||	        command list with the TE_ABORT_AT_VBLANK feature enabled.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V33.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/te.h>
|||
|||	  See Also
|||
|||	    TE_CMD_EXECUTELIST(@), TE_CMD_STEP(@), TE_CMD_SPEEDCONTROL(@),
|||	    TE_CMD_GETIDLETIME(@)
|||
**/

/**
|||	AUTODOC -class Device_Commands -group TriangleEngine -name TE_CMD_DISPLAYFRAMEBUFFER
|||	Indicates that rendering to the current frame buffer is over and that
|||	the associated bitmap should be displayed to the user.
|||
|||	  Description
|||
|||	    This command serves as a marker to tell the triangle engine driver
|||	    that rendering to the current frame buffer is complete, and the
|||	    triangle engine driver should coordinate with the graphics folio
|||	    to get the bitmap attached to a View, which ultimately displays it
|||	    to the user.
|||
|||	    This command queues along with the other commands such as
|||	    TE_CMD_EXECUTELIST(@). As a result, the command waits for previous
|||	    rendering operations to complete before attaching the bitmap
|||	    to the View.  The View to which the completed Bitmaps should be
|||	    attached is specified using the command TE_CMD_SETVIEW(@).
|||
|||	    The IOReq used to issue this command is returned to you only when
|||	    the graphics folio indicates that the bitmap has begun to be
|||	    swept by the video beam and is considered as being "displayed".
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to TE_CMD_DISPLAYFRAMEBUFFER.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V31.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/te.h>
|||
|||	  See Also
|||
|||	    TE_CMD_SETFRAMEBUFFER(@), TE_CMD_SETVIEW(@),
|||	    TE_CMD_SETZBUFFER(@), TE_CMD_EXECUTELIST(@), TE_CMD_STEP(@),
|||	    TE_CMD_SPEEDCONTROL(@), TE_CMD_GETIDLETIME(@)
|||
**/

/**
|||	AUTODOC -class Device_Commands -group TriangleEngine -name TE_CMD_STEP
|||	Causes the triangle engine to execute one instruction.
|||
|||	  Description
|||
|||	    This command lets you advance the triangle engine by a single
|||	    triangle engine command within the current command list.
|||
|||	    You use TE_CMD_STEP when the triangle engine is currently paused.
|||	    The triangle engine will pause as a result of encountering a
|||	    PAUSE or INT instruction in the command list, and by using the
|||	    TE_CMD_SPEEDCONTROL(@) command to cause the triangle engine to
|||	    execute slowly.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to TE_CMD_STEP.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V31.
|||
|||	  Caveat
|||
|||	    Do not use TE_CMD_STEP if an IOReq has already been
|||	    dispatched with the  TE_CLEAR_Z_BUFFER CmdOption flag
|||	    set. This will cause a Triangle Engine timeout.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/te.h>
|||
|||	  See Also
|||
|||	    TE_CMD_SETFRAMEBUFFER(@), TE_CMD_SETZBUFFER(@),
|||	    TE_CMD_EXECUTELIST(@), TE_CMD_DISPLAYFRAMEBUFFER(@),
|||	    TE_CMD_SPEEDCONTROL(@), TE_CMD_GETIDLETIME(@)
|||
**/

/**
|||	AUTODOC -class Device_Commands -group TriangleEngine -name TE_CMD_SPEEDCONTROL
|||	Controls the execution speed of the triangle engine.
|||
|||	  Description
|||
|||	    This command is controls the speed at which the triangle engine
|||	    processes command lists. You specify the number of microseconds
|||	    between each TE command.
|||
|||	    Setting a value of 0 for the delay indicates that the triangle
|||	    engine should run at full speed. Setting a delay value of
|||	    0xffffffff means that the triangle engine should be stoppped
|||	    completely. When it is stopped, you can use the TE_CMD_STEP(@)
|||	    command to single-step the triangle engine.
|||
|||	    This command is intended to help in debugging triangle engine
|||	    command lists. You can execute command lists at a slow rate, which
|||	    lets you see the rendering occur slowly on the screen. This lets
|||	    you easily detect subtle rendering errors such as rendering over
|||	    the same area multiple times needlessly.
|||
|||	    Whenever a PAUSE or INT instruction is encountered in a command
|||	    list, the speed control value is automatically set to 0xffffffff,
|||	    which means the triangle engine stops. You can then single-step
|||	    from that point on, or resume execution at a slower rate by
|||	    using TE_CMD_SPEEDCONTROL(@) to set a new delay value.
|||
|||	  IOInfo
|||
|||	    ioi_Command
|||	        Set to TE_CMD_SPEEDCONTROL.
|||
|||	    ioi_CmdOptions
|||	        The number of microseconds of delay to insert between each
|||	        instruction executed by the triangle engine. Note that there
|||	        is a minimum overhead involved in single-stepping the
|||	        triangle, probably in the neighborhood of 10-20 microseconds.
|||
|||	  Implementation
|||
|||	    Command defined in Portfolio V31.
|||
|||	  Associated Files
|||
|||	    <kernel/devicecmd.h>, <device/te.h>
|||
|||	  See Also
|||
|||	    TE_CMD_SETFRAMEBUFFER(@), TE_CMD_SETZBUFFER(@), TE_CMD_GETIDLETIME(@),
|||	    TE_CMD_DISPLAYFRAMEBUFFER(@), TE_CMD_EXECUTELIST(@), TE_CMD_STEP(@)
|||
**/

/* keep the compiler happy... */
extern int foo;
