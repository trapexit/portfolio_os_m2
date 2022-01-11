#ifndef __HARDWARE_DEBUGGER_H
#define __HARDWARE_DEBUGGER_H


/******************************************************************************
**
**  @(#) debugger.h 96/09/10 1.21
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_IO_H
#include <kernel/io.h>
#endif

#ifndef __HARDWARE_BRIDGIT_H
#include <hardware/bridgit.h>
#endif


/*****************************************************************************/


/* The debugger communicates with the Portfolio OS through a range of
 * shared memory which is divided up into a pair of communication
 * buffers. There is an input buffer and an output buffer.
 *
 * When Portfolio wants to talk to the debugger, it enters information in
 * the output buffer. When the debugger communicates with Portfolio, it puts
 * the data in the input buffer.
 *
 * When putting data into a buffer, one of 256 different units can be
 * specified. These units are used to multiplex the comm buffers amongst
 * multiple different uses. The unit number determines the interpretation of
 * the rest of the data within a comm buffer.
 *
 * The low-level communication with the debugger is done with the host device.
 * The sole purpose in life of this device is to coordinate access to the
 * shared buffer. Each unit of the host device corresponds to a unit of the
 * comm buffer.
 *
 * Client code can talk directly to the host device, or can interface to the
 * higher-level devices such as the hostfs, hostcd, or hostconsole devices.
 * These devices provide interface and sementic support to access different
 * units of the comm buffer.
 *
 * In addition to all the stuff discusssed above, there is also a separate
 * shared memory buffer which is used for character output to the debugging
 * terminal. The OS piles up a series of characters in this buffer, and then
 * triggers the Mac to pick the characters up using the DEBUGGER_SENDCHARS()
 * macro.
 */


/*****************************************************************************/


typedef struct DebuggerInfo	/* dbg_Version = 2 */
{
	uint32	dbg_Magic;  	/* Contains DEBUGGER_INFO_MAGIC */
	uint32	dbg_Version;	/* Version of this DebuggerInfo structure. */
	uint32	dbg_OSVersion;	/* Version/revision of the OS */
	uint32	dbg_OSTime;	/* Build date/time of the OS */
	void *	dbg_CommInPtr;	/* Address of comm. input buffer */
	uint32	dbg_CommInSize;	/* Size of comm. input buffer */
	void *	dbg_CommOutPtr;	/* Address of comm. output buffer */
	uint32	dbg_CommOutSize; /* Size of comm. output buffer */
	void *	dbg_PrintfPtr;	/* Address of printf buffer */
	uint32	dbg_PrintfSize;	/* Size of printf buffer */
	void *	dbg_MonInPtr;	/* Address of monitor input buffer */
	uint32	dbg_MonInSize;	/* Size of monitor input buffer */
	void *	dbg_MonOutPtr;	/* Address of monitor output buffer */
	uint32	dbg_MonOutSize; /* Size of monitor output buffer */
	void *	dbg_ResetVector; /* Addr of reset vector */
	uint32	dbg_DBVersion;	/* +DBG+ Version/revision of the debugger */
	uint16	dbg_HostType;	/* +DBG+ Type of host machine */
	uint16	dbg_DBType;	/* +DBG+ Which debugger is running */
	uint32	dbg_Flags;	/* Flags; see below */
} DebuggerInfo;

/* Value in dbg_Magic */
#define	DEBUGGER_INFO_MAGIC	0xE36CA3C8

/* Values in dbg_HostType */
#define	HT_MAC		0x0001	/* Macintosh */
#define	HT_PC		0x0002	/* IBM-compatible PC */
#define	HT_UNIX		0x0003	/* Unix */

/* Values in dbg_DBType */
#define	DBG_3DODEBUG	0x0001	/* 3DODebug */
#define	DBG_COMM3DO	0x0002	/* Comm3DO */

/* Bits in dbg_Flags */
#define	DBG_DEVELOPER	0x00000001  /* Use developer mode */


/*****************************************************************************/


/* types of units currently defined for the communication buffers */
typedef enum HostUnits
{
    HOST_UNUSED0_UNIT,
    HOST_UNUSED1_UNIT,

    HOST_FS_UNIT,
    HOST_LUMBERJACK_UNIT,
    HOST_COMM3DO_UNIT,
    HOST_CD_UNIT,
    HOST_CONSOLE_UNIT
} HostUnits;


/*****************************************************************************/


#define DATASIZE 26

/* This is the base format of a communication buffer. All of the unit-specific
 * data is represented within the hp_Data field of these packets.
 */
typedef struct HostPacket
{
    bool   hp_Busy;
    uint8  hp_Unit;
    uint8  hp_Data[DATASIZE];
    void  *hp_UserData;
} HostPacket;

/*
 * Offsets of debugger comm. buffers.
 * These are all offsets from DEBUGGER_REGION.
 */
#define	DEBUG_OUT_OFFSET	0x50400   /* Monitor output buffer */
#define	DEBUG_IN_OFFSET		0x50800   /* Monitor input buffer */
#define HOST_SENDCHARS_OFFSET	0x80F80   /* Character output buffer */
#define HOST_RECV_OFFSET	0x80FC0   /* Comm output buffer */
#define HOST_SEND_OFFSET	0x80FE0   /* Comm input buffer */


/*****************************************************************************/


/* for HOST_FS_UNIT */

/* from portfolio to host */
typedef struct HostFSReq
{
    bool      hfs_Busy;
    uint8     hfs_Unit;
    uint8     hfs_Command;
    uint8     hfs_Flags;
    IOBuf     hfs_Send;
    IOBuf     hfs_Recv;
    int32     hfs_Offset;
    void     *hfs_ReferenceToken;
    void     *hfs_UserData;
} HostFSReq;

/* from host to portfolio */
typedef struct HostFSReply
{
    bool    hfsr_Busy;
    uint8   hfsr_Unit;
    uint8   hfsr_Command;
    uint8   hfsr_Flags;
    int32   hfsr_Error;
    int32   hfsr_Actual;
    void   *hfsr_ReferenceToken;
    uint32  hfsr_Pad[3];
    void   *hfsr_UserData;
} HostFSReply;

typedef enum HostFSCmds
{
    HOSTFS_REMOTECMD_MOUNTFS,
    HOSTFS_REMOTECMD_OPENENTRY,
    HOSTFS_REMOTECMD_CLOSEENTRY,
    HOSTFS_REMOTECMD_CREATEFILE,
    HOSTFS_REMOTECMD_CREATEDIR,
    HOSTFS_REMOTECMD_DELETEENTRY,
    HOSTFS_REMOTECMD_READENTRY,
    HOSTFS_REMOTECMD_READDIR,
    HOSTFS_REMOTECMD_ALLOCBLOCKS,
    HOSTFS_REMOTECMD_BLOCKREAD,
    HOSTFS_REMOTECMD_BLOCKWRITE,
    HOSTFS_REMOTECMD_STATUS,
    HOSTFS_REMOTECMD_FSSTAT,
    HOSTFS_REMOTECMD_SETEOF,
    HOSTFS_REMOTECMD_SETTYPE,
    HOSTFS_REMOTECMD_SETVERSION,
    HOSTFS_REMOTECMD_DISMOUNTFS,
    HOSTFS_REMOTECMD_RENAMEENTRY,
    HOSTFS_REMOTECMD_SETDATE
} HostFSCmds;


/*****************************************************************************/


/* for HOST_CD_UNIT */

/* from portfolio to host */
typedef struct HostCDReq
{
    bool      hcd_Busy;
    uint8     hcd_Unit;
    uint8     hcd_Command;
    uint8     hcd_Flags;
    IOBuf     hcd_Send;
    IOBuf     hcd_Recv;
    int32     hcd_Offset;
    void     *hcd_ReferenceToken;
    void     *hcd_UserData;
} HostCDReq;

/* from host to portfolio */
typedef struct HostCDReply
{
    bool    hcdr_Busy;
    uint8   hcdr_Unit;
    uint8   hcdr_Command;
    uint8   hcdr_Flags;
    int32   hcdr_Error;
    int32   hcdr_Actual;
    uint32  hcdr_BlockSize;
    void   *hcdr_ReferenceToken;
    uint32  hcdr_Pad[2];
    void   *hcdr_UserData;
} HostCDReply;

typedef enum HostCDCmds
{
    HOSTCD_REMOTECMD_MOUNT,
    HOSTCD_REMOTECMD_BLOCKREAD,
    HOSTCD_REMOTECMD_DISMOUNT
} HostCDCmds;


/*****************************************************************************/


/* for HOST_CONSOLE_UNIT */

/* from portfolio to host */
typedef struct HostConsoleReq
{
    bool      hcon_Busy;
    uint8     hcon_Unit;
    uint8     hcon_Command;
    uint8     hcon_Flags;
    IOBuf     hcon_Send;
    IOBuf     hcon_Recv;
    uint32    hcon_Pad[2];
    void     *hcon_UserData;
} HostConsoleReq;

/* from host to portfolio */
typedef struct HostConsoleReply
{
    bool    hconr_Busy;
    uint8   hconr_Unit;
    uint8   hconr_Command;
    uint8   hconr_Flags;
    int32   hconr_Error;
    int32   hconr_Actual;
    uint32  hconr_Pad[4];
    void   *hconr_UserData;
} HostConsoleReply;

typedef enum HostConsoleCmds
{
    HOSTCONSOLE_REMOTECMD_GETCMDLINE
} HostConsoleCmds;


/*****************************************************************************/


/* tell the debugger something is waiting for it */
#define DEBUGGER_CHANNEL_FULL()   BRIDGIT_WRITE(BRIDGIT_BASE,BR_MISC_REG,0x00818000)

/* tell the debugger we processed the input buffer is free for use */
#define DEBUGGER_CHANNEL_EMPTY()  BRIDGIT_WRITE(BRIDGIT_BASE,BR_MISC_REG,0x00838000)

/* see if the debugger has emptied the output buffer */
#define DEBUGGER_ACK()           (BRIDGIT_READ(BRIDGIT_BASE,BR_MISC_REG) == 0)

/* tell the debugger characters await it */
#define DEBUGGER_SENDCHARS(num)   BRIDGIT_WRITE(BRIDGIT_BASE,BR_MISC_REG,((uint32)(num) << 16) | 0x8000)


/* WARNING: DEBUGGER_CHANNEL_FULL(), DEBUGGER_CHANNEL_EMPTY(), and
 *          DEBUGGER_SENDCHARS() can only be used if DEBUGGER_ACK() returns TRUE.
 *          Otherwise, the mailbox register is in use and its contents would
 *          be overwritten.
 *
 *          Also, all uses of these macros must be done with interrupts
 *          turned off.
 */


/*****************************************************************************/


#endif /* __HARDWARE_DEBUGGER_H */
