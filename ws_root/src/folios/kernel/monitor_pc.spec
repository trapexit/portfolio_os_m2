@(#) monitor_pc.spec 96/11/01 1.5

General
-------

The communication protocol is generally pretty simple. Everything is done
through packets. Packets all start with a packet header, followed by the
packet-specific data.

The PacketHeader structure specifies the size of the data being sent within
the packet, specifies the packet flavor, and a generic packet object pointer.
The object pointer is used by many of the packet flavors, as described below.

Every packet sent by one side is acknowledged by the other side. This
acknowlegement is done through a response packet which either contains the
information requested, or is a generic ACK packet.


Debugger Packets
----------------

These are packets sent by the debugger to the monitor. The packets can be in
any of the flavors listed below. Packets that expect a response packet other
than MON_ACK may still receive a MON_ACK packet from the monitor. This will
happen when the monitor cannot complete the required operation. No further
response packets for the current transaction are sent when a failed MON_ACK
packet is sent by the monitor.


DBGR_NOP

    Packet Data      : None
    Packet Object    : NULL
    Expected Response: None
    Description      : Do nothing.

DBGR_ACK

    Packet Data      : ACKInfo
    Packet Object    : NULL
    Expected Response: None
    Description      : Sent by the debugger in response to the following
                       packets from the monitor: MON_SystemStartup,
                       MON_TaskCreated, MON_TaskDeleted, MON_TaskCrashed,
                       MON_ModuleCreated, MON_ModuleDeleted,
                       MON_ModuleDependent, MON_Hello, MON_MemoryRange.

                       The ACKInfo structure contains a result code which is
                       set to 0 for normal completion or a negative number to
                       indicate an error.

DBGR_GetGeneralRegs

    Packet Data      : None
    Packet Object    : Task pointer
    Expected Response: MON_GeneralRegs
    Description      : Get a suspended task's registers.

DBGR_GetFPRegs

    Packet Data      : None
    Packet Object    : Task pointer
    Expected Response: MON_FPRegs
    Description      : Get a suspended task's FP registers.

DBGR_GetSuperRegs

    Packet Data      : None
    Packet Object    : NULL
    Expected Response: MON_SuperRegs
    Description      : Get supervisor registers from the monitor.

DBGR_SetGeneralRegs

    Packet Data      : GeneralRegs structure
    Packet Object    : Task pointer
    Expected Response: MON_ACK
    Description      : Set a suspended task's general registers.

DBGR_SetFPRegs

    Packet Data      : FPRegs structure
    Packet Object    : Task pointer
    Expected Response: MON_ACK
    Description      : Set a suspended task's FP registers.

DBGR_SetSuperRegs

    Packet Data      : SuperRegs structure
    Packet Object    : NULL
    Expected Response: MON_ACK
    Description      : Set the system's supervisor registers.

DBGR_SetSingleStep

    Packet Data      : None
    Packet Object    : Task pointer
    Expected Response: MON_ACK
    Description      : Put a suspended task into single-step mode.

DBGR_SetBranchStep

    Packet Data      : None
    Packet Object    : Task pointer
    Expected Response: MON_ACK
    Description      : Put a suspended task into branch-stepping mode.

DBGR_ClearSingleStep

    Packet Data      : None
    Packet Object    : Task pointer
    Expected Response: MON_ACK
    Description      : Put a suspended task out of single-stepping mode.

DBGR_ClearBranchStep

    Packet Data      : None
    Packet Object    : Task pointer
    Expected Response: MON_ACK
    Description      : Put a suspended task out of branch-stepping mode.

DBGR_InvalidateICache

    Packet Data      : None
    Packet Object    : NULL
    Expected Response: MON_ACK
    Description      : Invalidate the target's instruction cache.

DBGR_FlushDCache

    Packet Data      : None
    Packet Object    : NULL
    Expected Response: MON_ACK
    Description      : Flush the target's data cache.

DBGR_SuspendTask

    Packet Data      : None
    Packet Object    : Task pointer
    Expected Response: MON_ACK
    Description      : Suspend a running task.

DBGR_ResumeTask

    Packet Data      : None
    Packet Object    : Task pointer
    Expected Response: MON_ACK
    Description      : Restart a suspended task.

DBGR_AbortTask

    Packet Data      : None
    Packet Object    : Task pointer
    Expected Response: MON_ACK
    Description      : Terminate a previously suspended task and free its
                       resources.

Monitor Packets
---------------

These are packets sent by the monitor to the debugger. The packets can be
in any of the following flavors.

MON_NOP

    Packet Data      : None
    Packet Object    : NULL pointer
    Expected Response: DBGR_ACK
    Description      : Do nothing.

MON_SystemStartup

    Packet Data      : StartupInfo structure
    Packet Object    : NULL pointer
    Expected Response: DBGR_ACK
    Description      : Sent to the debugger on bootup to inform it of what's
                       happening.

MON_TaskCreated

    Packet Data      : TaskCreationInfo structure
    Packet Object    : Task pointer
    Expected Response: DBGR_ACK
    Description      : Inform the debugger about a new task being created.

MON_TaskDeleted

    Packet Data      : None
    Packet Object    : Task pointer
    Expected Response: DBGR_ACK
    Description      : Inform the debugger about a task being deleted from
                       the system.

MON_TaskCrashed

    Packet Data      : TaskCrashInfo
    Packet Object    : Task pointer
    Expected Response: DBGR_ACK
    Description      : Inform the debugger about a crash or breakpoint in a
                       task. The TaskCrashInfo packet holds information on the
                       cause of the crash.

MON_ModuleCreated

    Packet Data      : ModuleCreationInfo
    Packet Object    : Module pointer
    Expected Response: DBGR_ACK
    Description      : Inform the debugger that a code module has been loaded.
                       The ModuleCreationInfo packet contains information
                       about the new module.

MON_ModuleDependent

    Packet Data      : ModuleDependentInfo
    Packet Object    : Module pointer
    Expected Response: DBGR_ACK
    Description      : Inform the debugger about a module dependency. The
                       information packet specifies the name of a module of
                       which the given module is dependent. The information
                       also include the version and revision of the module.

MON_ModuleDeleted

    Packet Data      : None
    Packet Object    : Module pointer
    Expected Response: DBGR_ACK
    Description      : Inform the debugger that a module has been removed from
                       memory.

MON_ACK

    Packet Data      : ACKInfo
    Packet Object    : NULL pointer
    Expected Response: None
    Description      : Sent by the monitor in response to the following
                       packets from the debugger: DBGR_SetGeneralRegs,
                       DBGR_SetFPRegs, DBGR_SetSuperRegs, DBGR_SetSingleStep,
                       DBGR_SetBranchStep, DBGR_ClearSingleStep,
                       DBGR_ClearBranchStep, DBGR_InvalidateICache,
                       DBGR_FlushDCache, DBGR_SuspendTask, DBGR_ResumeTask,
                       DBGR_AbortTask

                       The ACKInfo structure contains a result code which is
                       set to 0 for normal completion or a negative number to
                       indicate an error.

MON_GeneralRegs

    Packet Data      : GeneralRegs structure
    Packet Object    : NULL pointer
    Expected Response: None
    Description      : Sent by the monitor in response to the debugger
                       sending a DBGR_GetGeneralRegs packet.

MON_FPRegs

    Packet Data      : FPRegs structure
    Packet Object    : NULL pointer
    Expected Response: None
    Description      : Sent by the monitor in response to the debugger
                       sending a DBGR_GetFPRegs packet.

MON_SuperRegs

    Packet Data      : SuperRegs structure
    Packet Object    : NULL pointer
    Expected Response: None
    Description      : Sent by the monitor in response to the debugger
                       sending a DBGR_GetSuperRegs packet.
MON_Hello

    Packet Data      : None
    Packet Object    : NULL pointer
    Expected Response: DBGR_ACK
    Description      : Inform the debugger that the monitor is being entered
                       as a result of an interrupt being triggered by the
                       debugger.

MON_MemoryRange

    Packet Data      : MemoryRange structure
    Packet Object    : NULL
    Expected Response: DBGR_ACK
    Description      : Inform the debugger about a memory range. The info
                       includes a name for the range and a general type
                       specification.
