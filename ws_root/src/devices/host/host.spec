/* @(#) host.spec 96/03/13 1.2 */

             Portfolio To Host Interface Specification (13-Mar-96)
             =====================================================

This document specifies the various layers of the communication protocol to
interface a 3DO development system to a foreign host. The goals behind this
specification are to:

  - Provide a solution which delivers high performance reliable
    communication between Portfolio and a remote host.

  - Decouple Portfolio from any particular host by supporting a generic
    protocol.

  - Provide an API identical to native resources to enable transparent code
    sharing.

The various layers involved in this communication protocol are depicted
below:

                    Application                     Portfolio Shell

                         |                                |
                         |                                |
                 +-------+-----------+                    |
                 |                   |                    |
                 |                   |                    |
                                                          |
              Host File          Opera File               |
               System              System                 |
                                                          |
                 |                   |                    |
                 |                   |                    |

              Host FS             Host CD           Host Console
              Device              Device               Device

                 |                   |                    |
                 |                   |                    |
                 +-------------------+--------------------+
                                     |
                                     |

                                 Host Device

                                     |
                                     |

                                  Host (Mac)


===============================================================================

1. Host File System
-------------------

The Host File System exposes an interface identical to the Acrobat File System.
The intent for this file system is to provide access to a host (non 3DO) file
system, and make it appear as close to a 3DO file system as possible.

This document doesn't describe the interface to the Host File System, as it
is intended to be identical to the standard Acrobat interface. Refer to
Acrobat documentation for more information.


2. Opera File System
--------------------

The Opera File System is a standard component of the Portfolio OS and is used
to read from a CD. It can interface to the usual cd-rom device driver, or to
to hostcd device driver.

This document doesn't describe the interface to the Opera File System.
Refer to the Opera File System documentation for more information.


3. Host Device
--------------

The Host Device provides the low-level services to interface to the remote
host. It manages a pair of shared memory buffers which are used to pass
information between Portfolio and the remote Host.

Up to 26 bytes of data can be sent to or received from the host through the
communication buffers. It is customary to embed pointers within the
communication buffers to additional shared memory areas where extra data can be
exchanged.

Each packet of information exchanged between the Host Device and the Host
contains a unit specification from 0 to 255. The unit number determines the
format and meaning of the 26 bytes of data within the buffer. You control
which unit packets of information are sent to by interfacing to the different
units of the Host Device.

This device supports the HOST_CMD_SEND and HOST_CMD_RECV commands. Refer to
the Portfolio autodocs for a description of these commands.


4. Host FS Device
-----------------

The Host FS Device responds to a set of commands designed to allow a Portfolio
file system to be emulated on top of a Host's native file system. In effect,
it provides the translation services needed to convert Portfolio-style file
operations into something that the Host can understand. This device interfaces
to the lower-level Host Device to perform the actual communication with the
remote Host.

Although it is possible for clients to connect directly to this device, it
is generally expected that only the Host File System will maintain a link to
the Host FS Device.

This device supports a wide array of commands (see <kernel/devicecmd.h> for
the many HOSTFS_CMD_* commands). Refer to the Portfolio autodocs for a
description of these commands.


5. Host CD Device
-----------------

The Host CD Device responds to the same commands that the standard cd-rom
device supports. This allows higher-level components, like the Opera File
System to interface to either device in an identical manner. This device
interfaces to the lower-level Host Device to perform the actual communication
with the remote Host.

The purpose of this device is to allow a single file on the Host to be
treated as a virtual CD-ROM from the Portfolio side.

This device supports the CMD_BLOCKREAD command. Refer to
the Portfolio autodocs for a description of this command.


6. Host Console Device
----------------------

The Host Console Device provides services to obtain command-line input
from the remote Host. A client can request a command-line, and the Host then
prompts the user to enter the text. When the text is entered, it is returned
to Portfolio. This device interfaces to the lower-level Host Device to
perform the actual communication with the remote Host.

This device supports the HOSTCONSOLE_CMD_GETCMDLINE command. Refer to the
Portfolio autodocs for a description of this command.


7. Host (Mac)
-------------

As explained above, the Host communicates with the 3DO system through shared
memory buffers. The buffers pass 26 bytes of data back and forth between the
two environments, along with 2 bytes of header, and 4 bytes of trailer
information.

Part of the header is a 1 byte unit number. The unit number determines how
the data payload in the buffer should be interpreted. The following units
are currently supported:

  HOST_FS_UNIT        - provide file system services
  HOST_CD_UNIT        - provide CD emulation services
  HOST_CONSOLE_UNIT   - provide command-line input services

The structures describing the data packets sent back and forth through the
communication buffers are defined in <hardware/debugger.h>. There are
different structures for every unit.

Below is a description of what can be sent through the different units.


7.1 Host - HOST_FS_UNIT
-----------------------

Following is a list of the various information packets that can be sent to
the HOST_FS_UNIT and the replies that the Host is expected to provide.

-----

HOSTFS_REMOTECMD_MOUNTFS
This command is used to mount a file system on the host.

   hfs_Command          Set to HOSTFS_REMOTECMD_MOUNTFS

   hfs_Offset           File system number to mount. This will start at 0 and
                        increase sequentially. The mount operation will fail
                        when the host reaches the last available FS to mount.

   hfs_Recv.iob_Buffer  Buffer where the host should put the name that the
                        file system should have. Should be NULL terminated.
                        This pointer may be NULL in which case the name is
                        not returned.

   hfs_Recv.iob_Len     Number of bytes available for the file system name,
                        including the NULL terminator.

   hfs_UserData         Arbitrary 32-bit value defined by the Host Device. This
                        value is returned with the reply packet and lets the
                        Host Device find its bearings.

The reply packet from the host contains:

   hfsr_Command         Set to HOSTFS_REMOTECMD_MOUNTFS

   hfsr_Error           Portfolio error code

   hfsr_ReferenceToken  Returned reference token for the root of the FS being
                        mounted.

   hfsr_UserData        Value supplied with hfs_UserData when initiating
                        the operation.

-----

HOSTFS_REMOTECMD_DISMOUNTFS
This command is used to dismount a file system on the host.

   hfs_Command          Set to HOSTFS_REMOTECMD_DISMOUNTFS

   hfs_ReferenceToken   Reference token for the mounted file system, as
                        obtained from HOSTFS_REMOTECMD_MOUNTFS

   hfs_UserData         Arbitrary 32-bit value defined by the Host Device. This
                        value is returned with the reply packet and lets the
                        Host Device find its bearings.

The reply packet from the host contains:

   hfsr_Command         Set to HOSTFS_REMOTECMD_DISMOUNTFS

   hfsr_Error           Portfolio error code

   hfsr_UserData        Value supplied with hfs_UserData when initiating
                        the operation.

After this command completes, the file system becomes unavailable for
use.

-----

HOSTFS_REMOTECMD_OPENENTRY
This command is used to obtain a reference token for a filesystem object.

   hfs_Command          Set to HOSTFS_REMOTECMD_OPENENTRY

   hfs_Send.iob_Buffer  Pointer to the NULL-terminated name of the object to
                        obtain the reference token for.

   hfs_Send.iob_Len     Number of bytes in the object name, including the
                        NULL terminator.

   hfs_ReferenceToken   Reference token for the containing directory.

   hfs_UserData         Arbitrary 32-bit value defined by the Host Device. This
                        value is returned with the reply packet and lets the
                        Host Device find its bearings.

The reply packet from the host contains:

   hfsr_Command         Set to HOSTFS_REMOTECMD_OPENENTRY

   hfsr_Error           Portfolio error code

   hfsr_ReferenceToken  Returned reference token

   hfsr_UserData        Value supplied with hfs_UserData when initiating
                        the operation.
----

HOSTFS_REMOTECMD_CLOSEENTRY
This command is used to indicate that a given reference token is no longer
useful, and any associated data can be discarded.

   hfs_Command          Set to HOSTFS_REMOTECMD_CLOSEENTRY

   hfs_ReferenceToken   Reference token to dispose of.

   hfs_UserData         Arbitrary 32-bit value defined by the Host Device. This
                        value is returned with the reply packet and lets the
                        Host Device find its bearings.

The reply packet from the host contains:

   hfsr_Command         Set to HOSTFS_REMOTECMD_CLOSEENTRY

   hfsr_Error           Portfolio error code

   hfsr_UserData        Value supplied with hfs_UserData when initiating
                        the operation.

-----

HOSTFS_REMOTECMD_CREATEFILE
This command is used to create a new file within an existing directory.

   hfs_Command          Set to HOSTFS_REMOTECMD_CREATEFILE

   hfs_Send.iob_Buffer  Pointer to the NULL-terminated name of the file to
                        create,

   hfs_Send.iob_Len     Number of bytes in the file name, including the
                        NULL terminator.

   hfs_ReferenceToken   Reference token for the containing directory.

   hfs_UserData         Arbitrary 32-bit value defined by the Host Device. This
                        value is returned with the reply packet and lets the
                        Host Device find its bearings.

The reply packet from the host contains:

   hfsr_Command         Set to HOSTFS_REMOTECMD_CREATEFILE

   hfsr_Error           Portfolio error code

   hfsr_ReferenceToken  Returned reference token

   hfsr_UserData        Value supplied with hfs_UserData when initiating
                        the operation.

-----

HOSTFS_REMOTECMD_CREATEDIR
This command is used to create a new directory within an existing directory.

   hfs_Command          Set to HOSTFS_REMOTECMD_CREATEDIR

   hfs_Send.iob_Buffer  Pointer to the NULL-terminated name of the directory to
                        create,

   hfs_Send.iob_Len     Number of bytes in the directory name, including the
                        NULL terminator.

   hfs_ReferenceToken   Reference token for the containing directory.

   hfs_UserData         Arbitrary 32-bit value defined by the Host Device. This
                        value is returned with the reply packet and lets the
                        Host Device find its bearings.

The reply packet from the host contains:

   hfsr_Command         Set to HOSTFS_REMOTECMD_CREATEDIR

   hfsr_Error           Portfolio error code

   hfsr_ReferenceToken  Returned reference token

   hfsr_UserData        Value supplied with hfs_UserData when initiating
                        the operation.

-----

HOSTFS_REMOTECMD_DELETEENTRY
This command is used to remove a given object from a containing directory.
This command is used for both directories and files.

   hfs_Command          Set to HOSTFS_REMOTECMD_DELETEENTRY

   hfs_Send.iob_Buffer  Pointer to the NULL-terminated name of the object to
                        delete,

   hfs_Send.iob_Len     Number of bytes in the object name, including the
                        NULL terminator.

   hfs_ReferenceToken   Reference token for the containing directory.

   hfs_UserData         Arbitrary 32-bit value defined by the Host Device. This
                        value is returned with the reply packet and lets the
                        Host Device find its bearings.

The reply packet from the host contains:

   hfsr_Command         Set to HOSTFS_REMOTECMD_DELETEENTRY

   hfsr_Error           Portfolio error code

   hfsr_UserData        Value supplied with hfs_UserData when initiating
                        the operation.

-----

HOSTFS_REMOTECMD_RENAMEENTRY
This command is used to change the name of a given object within its directory.
This command is used for both directories and files.

   hfs_Command          Set to HOSTFS_REMOTECMD_RENAMEENTRY

   hfs_Send.iob_Buffer  Pointer to the NULL-terminated new name of the object.

   hfs_Send.iob_Len     Number of bytes in the new object name, including the
                        NULL terminator.

   hfs_ReferenceToken   Reference token for the object being affected.

   hfs_UserData         Arbitrary 32-bit value defined by the Host Device. This
                        value is returned with the reply packet and lets the
                        Host Device find its bearings.

The reply packet from the host contains:

   hfsr_Command         Set to HOSTFS_REMOTECMD_RENAMEENTRY

   hfsr_Error           Portfolio error code

   hfsr_UserData        Value supplied with hfs_UserData when initiating
                        the operation.

-----

HOSTFS_REMOTECMD_READENTRY
This command is used to obtain information about a particular object within
a directory. The information is returned in a DirectoryEntry structure,
as defined in <file/directory.h>. The object is specified by name.

   hfs_Command          Set to HOSTFS_REMOTECMD_READENTRY

   hfs_Send.iob_Buffer  Pointer to the NULL-terminated name of the object to
                        get information on.

   hfs_Send.iob_Len     Number of bytes in the object name, including the
                        NULL terminator.

   hfs_Recv.iob_Buffer  Pointer to a DirectoryEntry structure where the
                        information should be stored.

   hfs_Recv.iob_Len     The number of bytes of information desired. Only
                        this number of bytes should be copied to the
                        DirectoryEntry structure.

   hfs_ReferenceToken   Reference token for containing directory.

   hfs_UserData         Arbitrary 32-bit value defined by the Host Device. This
                        value is returned with the reply packet and lets the
                        Host Device find its bearings.

The reply packet from the host contains:

   hfsr_Command         Set to HOSTFS_REMOTECMD_READENTRY

   hfsr_Error           Portfolio error code

   hfsr_Actual          Number of bytes actually read.

   hfsr_UserData        Value supplied with hfs_UserData when initiating
                        the operation.

-----

HOSTFS_REMOTECMD_READDIR
This command is used to obtain information about a particular object within
a directory. The information is returned in a DirectoryEntry structure,
as defined in <file/directory.h>. The object is specified by index number
within a given directory.

   hfs_Command          Set to HOSTFS_REMOTECMD_READDIR

   hfs_Offset           The index of the object to get information on.
                        Index values start at 1, not 0.

   hfs_Recv.iob_Buffer  Pointer to a DirectoryEntry structure where the
                        information should be stored.

   hfs_Recv.iob_Len     The number of bytes of information desired. Only
                        this number of bytes should be copied to the
                        DirectoryEntry structure.

   hfs_ReferenceToken   Reference token for containing directory.

   hfs_UserData         Arbitrary 32-bit value defined by the Host Device. This
                        value is returned with the reply packet and lets the
                        Host Device find its bearings.

The reply packet from the host contains:

   hfsr_Command         Set to HOSTFS_REMOTECMD_READDIR

   hfsr_Error           Portfolio error code

   hfsr_Actual          Number of bytes actually read.

   hfsr_UserData        Value supplied with hfs_UserData when initiating
                        the operation.

-----

HOSTFS_REMOTECMD_ALLOCBLOCKS
This command is used to control the number of blocks in a file. The number of
blocks determines the physical file size, and serves as an upper bound to the
hfs_Offset field for HOSTFS_REMOTECMD_BLOCKREAD and HOSTFS_REMOTECMD_BLOCKWRITE operations.

   hfs_Command          Set to HOSTFS_REMOTECMD_ALLOCBLOCKS

   hfs_Offset           Set to the number of blocks to add or remove from the
                        file. A positive value specifies blocks to add, while a
                        negative value specifies blocks to remove.

   hfs_ReferenceToken   Reference token for the file being affected.

   hfs_UserData         Arbitrary 32-bit value defined by the Host Device. This
                        value is returned with the reply packet and lets the
                        Host Device find its bearings.

The reply packet from the host contains:

   hfsr_Command         Set to HOSTFS_REMOTECMD_ALLOCBLOCKS

   hfsr_Error           Portfolio error code

   hfsr_UserData        Value supplied with hfs_UserData when initiating
                        the operation.

This command should fail if there is not enough room to extend the file, or
if the reference token refers to a directory.

----

HOSTFS_REMOTECMD_BLOCKREAD
This command is used to read data from an opened file. The read operation
is always performed on block boundaries.

   hfs_Command          Set to HOSTFS_REMOTECMD_BLOCKREAD

   hfs_Offset           Specifies the block number where to start reading.
                        If this value is greater or equal to the number of
                        blocks in the file, the command should fail.

   hfs_Recv.iob_Buffer  Buffer where the data should be placed.

   hfs_Recv.iob_Len     Number of bytes of data to read. This must be a
                        multiple of the file's block size, otherwise this
                        command should fail.

   hfs_ReferenceToken   Reference token for the file to read from.

   hfs_UserData         Arbitrary 32-bit value defined by the Host Device. This
                        value is returned with the reply packet and lets the
                        Host Device find its bearings.

The reply packet from the host contains:

   hfsr_Command         Set to HOSTFS_REMOTECMD_BLOCKREAD

   hfsr_Error           Portfolio error code

   hfsr_Actual          Number of bytes actually read.

   hfsr_UserData        Value supplied with hfs_UserData when initiating
                        the operation.

-----

HOSTFS_REMOTECMD_BLOCKWRITE
This command is used to write data to an opened file. The write operation
is always performed on block boundaries.

   hfs_Command          Set to HOSTFS_REMOTECMD_BLOCKWRITE

   hfs_Offset           Specifies the block number where to start writing.
                        If this value is greater or equal to the number of
                        blocks in the file, the command should fail.

   hfs_Send.iob_Buffer  Buffer where the data to write can be found.

   hfs_Send.iob_Len     Number of bytes of data to write. This must be a
                        multiple of the file's block size, otherwise this
                        command should fail.

   hfs_ReferenceToken   Reference token for the file to write to.

   hfs_UserData         Arbitrary 32-bit value defined by the Host Device. This
                        value is returned with the reply packet and lets the
                        Host Device find its bearings.

The reply packet from the host contains:

   hfsr_Command         Set to HOSTFS_REMOTECMD_BLOCKWRITE

   hfsr_Error           Portfolio error code

   hfsr_Actual          Number of bytes actually written.

   hfsr_UserData        Value supplied with hfs_UserData when initiating
                        the operation.

-----

HOSTFS_REMOTECMD_STATUS
This command is used to obtain information about a particular opened entry.

   hfs_Command          Set to HOSTFS_REMOTECMD_STATUS

   hfs_Recv.iob_Buffer  Pointer to a FileStatus structure where the
                        information should be stored.

   hfs_Recv.iob_Len     The number of bytes of information desired. Only
                        this number of bytes should be copied to the
                        FileStatus structure.

   hfs_ReferenceToken   Reference token for the entry to get information on.

   hfs_UserData         Arbitrary 32-bit value defined by the Host Device. This
                        value is returned with the reply packet and lets the
                        Host Device find its bearings.

The reply packet from the host contains:

   hfsr_Command         Set to HOSTFS_REMOTECMD_STATUS

   hfsr_Error           Portfolio error code

   hfsr_Actual          Number of bytes actually copied to input buffer.

   hfsr_UserData        Value supplied with hfs_UserData when initiating
                        the operation.

-----

HOSTFS_REMOTECMD_FSSTAT
This command is used to obtain information about the file system as a whole.

   hfs_Command          Set to HOSTFS_REMOTECMD_FSSTAT

   hfs_Send.iob_Buffer  Pointer to a FileSystemStat structure having been
                        pre-initialized by the File folio.

   hfs_Send.iob_Len     The number of bytes of information supplied by the
                        File folio.

   hfs_Recv.iob_Buffer  Pointer to a FileSystemStat structure where the
                        information should be stored.

   hfs_Recv.iob_Len     The number of bytes of information desired. Only
                        this number of bytes should be copied to the
                        FileSystemStat structure.

   hfs_ReferenceToken   Reference token for the entry to get information on.

   hfs_UserData         Arbitrary 32-bit value defined by the Host Device. This
                        value is returned with the reply packet and lets the
                        Host Device find its bearings.

The reply packet from the host contains:

   hfsr_Command         Set to HOSTFS_REMOTECMD_FSSTAT

   hfsr_Error           Portfolio error code

   hfsr_Actual          Number of bytes actually copied to input buffer.

   hfsr_UserData        Value supplied with hfs_UserData when initiating
                        the operation.

The data copied to the receive buffer is a combination of what the File
folio supplied, and what the host is supplying.

The volume creation time specified in the FileSystemStat structure is
relative to January 1st 1993, which is the Portfolio epoch. If the
creation time is actually earlier than that, pass in 0 as a creation
time.

-----

HOSTFS_REMOTECMD_SETEOF
This command is used to set the logical number of bytes within a file. This
value should be stored in the meta data for the file, and is not actually
used by the file system.

   hfs_Command          Set to HOSTFS_REMOTECMD_SETEOF

   hfs_Offset           Specifies the byte count to use as file size.

   hfs_ReferenceToken   Reference token for the file being affected.

   hfs_UserData         Arbitrary 32-bit value defined by the Host Device. This
                        value is returned with the reply packet and lets the
                        Host Device find its bearings.

The reply packet from the host contains:

   hfsr_Command         Set to HOSTFS_REMOTECMD_SETEOF

   hfsr_Error           Portfolio error code

   hfsr_UserData        Value supplied with hfs_UserData when initiating
                        the operation.

-----

HOSTFS_REMOTECMD_SETTYPE
This command is used to set the four byte file type associated with every file.
This value should be stored in the meta data for the file, and is not
actually used by the file system.

   hfs_Command          Set to HOSTFS_REMOTECMD_SETTYPE

   hfs_Offset           Four byte file type to set.

   hfs_ReferenceToken   Reference token.

   hfs_UserData         Arbitrary 32-bit value defined by the Host Device. This
                        value is returned with the reply packet and lets the
                        Host Device find its bearings.

The reply packet from the host contains:

   hfsr_Command         Set to HOSTFS_REMOTECMD_SETTYPE

   hfsr_Error           Portfolio error code

   hfsr_UserData        Value supplied with hfs_UserData when initiating
                        the operation.

-----

HOSTFS_REMOTECMD_SETVERSION
This command is used to set the version and revision values of a file. These
values should be stored in the meta data for the file, and are not actually
used by the file system.

   hfs_Command          Set to HOSTFS_REMOTECMD_SETVERSION

   hfs_Offset           Specifies the version and revision values. These are
                        two 8 bit values packed like: (version << 8) | revision.

   hfs_ReferenceToken   Reference token.

   hfs_UserData         Arbitrary 32-bit value defined by the Host Device. This
                        value is returned with the reply packet and lets the
                        Host Device find its bearings.

The reply packet from the host contains:

   hfsr_Command         Set to HOSTFS_REMOTECMD_SETVERSION

   hfsr_Error           Portfolio error code

   hfsr_UserData        Value supplied with hfs_UserData when initiating
                        the operation.


7.2 Host - HOST_CD_UNIT
-----------------------

Following is a list of the various information packets that can be sent to
the HOST_CD_UNIT and the replies that the Host is expected to provide.

-----

HOSTCD_REMOTECMD_MOUNT
This command mounts a file on the Mac as a CD-ROM emulated drive.

   hcd_Command          Set to HOSTCD_REMOTECMD_MOUNT

   hfs_Offset           Image file number to mount. This will start at 0 and
                        increase sequentially. The mount operation will fail
                        when the Host reaches the last available image to
                        mount.

   hcd_UserData         Arbitrary 32-bit value defined by the Host Device. This
                        value is returned with the reply packet and lets the
                        Host Device find its bearings.

The reply packet from the host contains:

   hcdr_Command         Set to HOSTCD_REMOTECMD_MOUNT

   hcdr_Error           Portfolio error code

   hcdr_Actual          Number of blocks in the image file mounted.

   hcdr_BlockSize       Size in bytes of a block in the image file.

   hcdr_ReferenceToken  Returned reference token uniquely identifying the
                        mounted image file.

   hcdr_UserData        Value supplied with hcd_UserData when initiating
                        the operation.

-----

HOSTCD_REMOTECMD_DISMOUNT
This command is used to dismount an image file on the host.

   hcd_Command          Set to HOSTCD_REMOTECMD_DISMOUNT

   hcd_ReferenceToken   Reference token for the mounted image file, as
                        obtained from HOSTCD_REMOTECMD_MOUNT

   hcd_UserData         Arbitrary 32-bit value defined by the Host Device. This
                        value is returned with the reply packet and lets the
                        Host Device find its bearings.

The reply packet from the host contains:

   hcdr_Command         Set to HOSTCD_REMOTECMD_DISMOUNT

   hcdr_Error           Portfolio error code

   hcdr_UserData        Value supplied with hcd_UserData when initiating
                        the operation.

After this command completes, the image file becomes unavailable for use.

-----

HOSTCD_REMOTECMD_BLOCKREAD
This command is used to read data from the image file. The read operation
is always performed on block boundaries.

   hcd_Command          Set to HOSTCD_REMOTECMD_BLOCKREAD

   hcd_Offset           Specifies the block number where to start reading.
                        If this value is greater or equal to the number of
                        blocks in the image file, the command should fail.

   hcd_ReferenceToken   Reference token for the image to read from.

   hcd_Recv.iob_Buffer  Buffer where the data should be placed.

   hcd_Recv.iob_Len     Number of bytes of data to read. This must be a
                        multiple of the block size, otherwise this
                        command should fail.

   hcd_UserData         Arbitrary 32-bit value defined by the Host Device. This
                        value is returned with the reply packet and lets the
                        Host Device find its bearings.

The reply packet from the host contains:

   hcdr_Command         Set to HOSTCD_REMOTECMD_BLOCKREAD

   hcdr_Error           Portfolio error code

   hcdr_Actual          Number of bytes actually read.

   hcdr_UserData        Value supplied with hcd_UserData when initiating
                        the operation.


7.3 Host - HOST_CONSOLE_UNIT
----------------------------

Following is a list of the various information packets that can be sent to the
HOST_CONSOLE_UNIT and the replies that the Host is expected to provide.

-----

HOSTCONSOLE_REMOTECMD_GETCMDLINE
Requests a command-line from the host.

   hcon_Command         Set to HOSTCONSOLE_REMOTECMD_GETCMDLINE

   hcon_Send.iob_Buffer Pointer to the NULL-terminated string to use to prompt
                        the user for input.

   hcon_Send.iob_Len    Number of bytes in the prompt string, including the
                        NULL-terminator.

   hcon_Recv.iob_Buffer Buffer where the resulting command-line should be put.
                        The returned command-line should be NULL-terminated.

   hcon_Recv.iob_Len    Number of bytes available in the receive buffer. No
                        more than this many bytes can be put in the buffer.

   hcon_UserData        Arbitrary 32-bit value defined by the Host Device. This
                        value is returned with the reply packet and lets the
                        Host Device find its bearings.

The reply packet from the host contains:

   hconr_Command        Set to HOSTCONSOLE_REMOTECMD_GETCMDLINE

   hconr_Error          Portfolio error code

   hconr_Actual         Number of bytes actually read.

   hconr_UserData       Value supplied with hcon_UserData when initiating
                        the operation.
