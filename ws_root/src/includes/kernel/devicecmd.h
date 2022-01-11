#ifndef __KERNEL_DEVICECMD_H
#define __KERNEL_DEVICECMD_H


/******************************************************************************
**
**  @(#) devicecmd.h 96/12/11 1.57
**
**  Definitions of device commands.
**
******************************************************************************/

#ifndef EXTERNAL_RELEASE

/*
 * WARNING:  do not add include directives or structure definitions to this
 *	file.  Device Description source files may include this file to get the
 *	command definitions.
 */

#endif

/*****************************************************************************/


/* Commands are divided into CLASSES.
 * This division is simply for ease of managing the command name space.
 */
#define	MakeDeviceCmd(class,cmd)	(((class)<<16) | cmd)
#define	GetDeviceCmdNumber(devcmd)	((devcmd) & 0xffff)
#define	GetDeviceClass(devcmd)		((devcmd) >> 16)


/*****************************************************************************/


/* Command classes. */
#define CMDC_GENERIC     0x000  /* Generic commands      */
#define CMDC_GFX         0x002  /* Graphics commands     */
#define CMDC_FILE        0x003  /* File commands         */
#define CMDC_TIMER       0x004  /* Timer commands        */
#define CMDC_CTLPORT     0x005  /* Control port commands */
#define CMDC_DISC        0x006  /* Disc drive commands   */
#define CMDC_MPEGV       0x007  /* MPEG video commands   */
#define CMDC_PROXY       0x009  /* Proxy commands        */
#ifndef EXTERNAL_RELEASE
#define CMDC_USLOT       0x00A  /* micro-slot commands   */
#endif
#define CMDC_SLOT        0x00B  /* Slot commands         */
#define CMDC_SERIAL      0x00C  /* Serial commands       */
#define CMDC_TE          0x00D  /* Triangle engine cmds  */

#define CMDC_HOST        0x103  /* Host commands         */
#define CMDC_HOSTFS      0x104  /* HostFS commands       */
#define CMDC_HOSTCONSOLE 0x105  /* Host console commands */
#define CMDC_MP          0x106  /* MP commands           */


/*****************************************************************************/


/* Generic commands.
 * Commands that are not clearly specific to one device
 * (or a small number of devices).
 */
#ifndef EXTERNAL_RELEASE
#define CMD_WRITE		0
	/* CMD_WRITE is OBSOLETE: use CMD_xxxWRITE below */
#endif

#define CMD_READ		1
	/* CMD_READ is OBSOLETE: use CMD_xxxREAD below */

#define CMD_STATUS		2
	/* Returns DeviceStatus in the receive buffer.
	 * (It is strongly recommended that all devices
	 * implement CMD_STATUS.) */

#define CMD_GETMAPINFO		MakeDeviceCmd(CMDC_GENERIC,0xC0)
	/* Returns MemMappableDeviceInfo in the receive buffer. */

#define CMD_MAPRANGE		MakeDeviceCmd(CMDC_GENERIC,0xC1)
	/* Takes MapRangeRequest in the send buffer.
	 * Returns MapRangeResponse in the receive buffer. */

#define CMD_UNMAPRANGE		MakeDeviceCmd(CMDC_GENERIC,0xC2)
	/* Takes MapRangeRequest in the send buffer.
	 * Returns nothing. */

#define	CMD_BLOCKREAD		MakeDeviceCmd(CMDC_GENERIC,0xC3)
	/* Takes block offset in ioi_Offset.
	 * Returns data in the receive buffer. */

#define	CMD_BLOCKWRITE		MakeDeviceCmd(CMDC_GENERIC,0xC4)
	/* Takes block offset in ioi_Offset.
	 * Returns nothing. */

#define	CMD_STREAMREAD		MakeDeviceCmd(CMDC_GENERIC,0xC5)
	/* Returns data in the receive buffer. */

#define	CMD_STREAMWRITE		MakeDeviceCmd(CMDC_GENERIC,0xC6)
	/* Takes data in the send buffer.
	 * Returns nothing. */

#define	CMD_GETICON		MakeDeviceCmd(CMDC_GENERIC,0xC7)
	/* Returns device icon in the receive buffer. */

#define	CMD_PREFER_FSTYPE	MakeDeviceCmd(CMDC_GENERIC,0xC8)
	/* Returns preferred filesystem type in the receive buffer. */

#define	CMD_STREAMFLUSH		MakeDeviceCmd(CMDC_GENERIC,0xC9)
	/* Ensures data written out using CMD_STREAMWRITE actually makes it out */


/*****************************************************************************/
#ifndef EXTERNAL_RELEASE


/*
 * Graphics commands.
 */
#define	GFXCMD_PROJECTORMODULE	MakeDeviceCmd(CMDC_GFX,16384)


/*****************************************************************************/
#endif


/* Triangle engine commands. */
#define TE_CMD_EXECUTELIST		MakeDeviceCmd(CMDC_TE,3)
#define	TE_CMD_SETVIEW			MakeDeviceCmd(CMDC_TE,4)
#define TE_CMD_SETZBUFFER		MakeDeviceCmd(CMDC_TE,5)
#define TE_CMD_SETFRAMEBUFFER		MakeDeviceCmd(CMDC_TE,6)
#define TE_CMD_DISPLAYFRAMEBUFFER	MakeDeviceCmd(CMDC_TE,7)
#define TE_CMD_STEP			MakeDeviceCmd(CMDC_TE,8)
#define TE_CMD_SPEEDCONTROL		MakeDeviceCmd(CMDC_TE,9)
#define TE_CMD_SETVBLABORTCOUNT		MakeDeviceCmd(CMDC_TE,10)

#define TE_CMD_GETIDLETIME		MakeDeviceCmd(CMDC_TE,256)


/*****************************************************************************/


/* File commands.
 * Commands for dealing with files (which are pseudo-devices).
 */
#define FILECMD_READDIR      	MakeDeviceCmd(CMDC_FILE,3)
#define FILECMD_GETPATH      	MakeDeviceCmd(CMDC_FILE,4)
#define FILECMD_READENTRY    	MakeDeviceCmd(CMDC_FILE,5)
#define FILECMD_ALLOCBLOCKS  	MakeDeviceCmd(CMDC_FILE,6)
#define FILECMD_SETEOF       	MakeDeviceCmd(CMDC_FILE,7)
#define FILECMD_ADDENTRY     	MakeDeviceCmd(CMDC_FILE,8)
#define FILECMD_DELETEENTRY  	MakeDeviceCmd(CMDC_FILE,9)
#define FILECMD_SETTYPE      	MakeDeviceCmd(CMDC_FILE,10)
#define FILECMD_OPENENTRY    	MakeDeviceCmd(CMDC_FILE,11)
#define FILECMD_FSSTAT       	MakeDeviceCmd(CMDC_FILE,12)
#define FILECMD_ADDDIR       	MakeDeviceCmd(CMDC_FILE,13)
#define FILECMD_DELETEDIR    	MakeDeviceCmd(CMDC_FILE,14)
#define FILECMD_SETVERSION   	MakeDeviceCmd(CMDC_FILE,15)
#define FILECMD_SETBLOCKSIZE   	MakeDeviceCmd(CMDC_FILE,16)
#define FILECMD_SETFLAGS	MakeDeviceCmd(CMDC_FILE,17)
#define FILECMD_RENAME		MakeDeviceCmd(CMDC_FILE,18)
#define FILECMD_SETDATE   	MakeDeviceCmd(CMDC_FILE,19)


/*****************************************************************************/


/* Disc commands.
 * Commands for dealing with an M2CD drive.
 */
#define CDROMCMD_READ		MakeDeviceCmd(CMDC_DISC,3)
#define CDROMCMD_SCAN_READ	MakeDeviceCmd(CMDC_DISC,4)
#define CDROMCMD_SETDEFAULTS	MakeDeviceCmd(CMDC_DISC,5)
#define CDROMCMD_DISCDATA	MakeDeviceCmd(CMDC_DISC,6)
#define CDROMCMD_READ_SUBQ	MakeDeviceCmd(CMDC_DISC,7)
#define CDROMCMD_OPEN_DRAWER	MakeDeviceCmd(CMDC_DISC,8)
#define CDROMCMD_CLOSE_DRAWER	MakeDeviceCmd(CMDC_DISC,9)
#ifndef EXTERNAL_RELEASE
#define CDROMCMD_WOBBLE_INFO	MakeDeviceCmd(CMDC_DISC,10)
#define CDROMCMD_RESIZE_BUFFERS	MakeDeviceCmd(CMDC_DISC,11)
#define CDROMCMD_DIAG_INFO	MakeDeviceCmd(CMDC_DISC,12)
#define CDROMCMD_MONITOR	MakeDeviceCmd(CMDC_DISC,13)
#define CDROMCMD_PASSTHRU	MakeDeviceCmd(CMDC_DISC,14)
#endif /* EXTERNAL_RELEASE */


/*****************************************************************************/


/* Timer commands.
 * Commands for dealing with a timer-like device.
 */
#define TIMERCMD_GETTIME_VBL	MakeDeviceCmd(CMDC_TIMER,3)
#define TIMERCMD_SETTIME_VBL	MakeDeviceCmd(CMDC_TIMER,4)
#define TIMERCMD_DELAY_VBL	MakeDeviceCmd(CMDC_TIMER,5)
#define TIMERCMD_DELAYUNTIL_VBL	MakeDeviceCmd(CMDC_TIMER,6)
#define TIMERCMD_METRONOME_VBL	MakeDeviceCmd(CMDC_TIMER,7)
#define TIMERCMD_GETTIME_USEC	MakeDeviceCmd(CMDC_TIMER,8)
#define TIMERCMD_SETTIME_USEC	MakeDeviceCmd(CMDC_TIMER,9)
#define TIMERCMD_DELAY_USEC	MakeDeviceCmd(CMDC_TIMER,10)
#define TIMERCMD_DELAYUNTIL_USEC MakeDeviceCmd(CMDC_TIMER,11)
#define TIMERCMD_METRONOME_USEC	MakeDeviceCmd(CMDC_TIMER,12)


/*****************************************************************************/


/* Slot commands.
 * Commands for dealing with slot drivers.
 */
#define SLOTCMD_SETTIMING	MakeDeviceCmd(CMDC_SLOT,5)


/*****************************************************************************/


/* commands for dealing with MPEG Video. */
#define MPEGVIDEOCMD_CONTROL            MakeDeviceCmd(CMDC_MPEGV,3)
#define MPEGVIDEOCMD_WRITE              MakeDeviceCmd(CMDC_MPEGV,4)
#define MPEGVIDEOCMD_READ               MakeDeviceCmd(CMDC_MPEGV,5)
#define MPEGVIDEOCMD_GETBUFFERINFO      MakeDeviceCmd(CMDC_MPEGV,6)
#define MPEGVIDEOCMD_SETSTRIPBUFFER     MakeDeviceCmd(CMDC_MPEGV,7)
#define MPEGVIDEOCMD_SETREFERENCEBUFFER MakeDeviceCmd(CMDC_MPEGV,8)
#define MPEGVIDEOCMD_ALLOCBUFFERS       MakeDeviceCmd(CMDC_MPEGV,9)


/*****************************************************************************/


/* proxy device */
#define PROXY_CMD_CREATE_DEVICE    MakeDeviceCmd(CMDC_PROXY,3)
#define PROXY_CMD_DWIM             MakeDeviceCmd(CMDC_PROXY,4)
#define PROXY_CMD_CREATE_SOFTFIRQ  MakeDeviceCmd(CMDC_PROXY,5)


/*****************************************************************************/


/* host device */
#define HOST_CMD_SEND              MakeDeviceCmd(CMDC_HOST,3)
#define HOST_CMD_RECV              MakeDeviceCmd(CMDC_HOST,4)

/* hostfs device */
#define HOSTFS_CMD_MOUNTFS         MakeDeviceCmd(CMDC_HOSTFS,3)
#define HOSTFS_CMD_OPENENTRY       MakeDeviceCmd(CMDC_HOSTFS,4)
#define HOSTFS_CMD_CLOSEENTRY      MakeDeviceCmd(CMDC_HOSTFS,5)
#define HOSTFS_CMD_CREATEFILE      MakeDeviceCmd(CMDC_HOSTFS,6)
#define HOSTFS_CMD_CREATEDIR       MakeDeviceCmd(CMDC_HOSTFS,7)
#define HOSTFS_CMD_DELETEENTRY     MakeDeviceCmd(CMDC_HOSTFS,8)
#define HOSTFS_CMD_READENTRY       MakeDeviceCmd(CMDC_HOSTFS,9)
#define HOSTFS_CMD_READDIR         MakeDeviceCmd(CMDC_HOSTFS,10)
#define HOSTFS_CMD_ALLOCBLOCKS     MakeDeviceCmd(CMDC_HOSTFS,11)
#define HOSTFS_CMD_BLOCKREAD       MakeDeviceCmd(CMDC_HOSTFS,12)
#define HOSTFS_CMD_STATUS          MakeDeviceCmd(CMDC_HOSTFS,13)
#define HOSTFS_CMD_FSSTAT          MakeDeviceCmd(CMDC_HOSTFS,14)
#define HOSTFS_CMD_BLOCKWRITE      MakeDeviceCmd(CMDC_HOSTFS,15)
#define HOSTFS_CMD_SETEOF          MakeDeviceCmd(CMDC_HOSTFS,16)
#define HOSTFS_CMD_SETTYPE         MakeDeviceCmd(CMDC_HOSTFS,17)
#define HOSTFS_CMD_SETVERSION      MakeDeviceCmd(CMDC_HOSTFS,18)
#define HOSTFS_CMD_DISMOUNTFS      MakeDeviceCmd(CMDC_HOSTFS,19)
#define HOSTFS_CMD_SETBLOCKSIZE    MakeDeviceCmd(CMDC_HOSTFS,20)
#define HOSTFS_CMD_RENAMEENTRY     MakeDeviceCmd(CMDC_HOSTFS,21)
#define HOSTFS_CMD_SETDATE         MakeDeviceCmd(CMDC_HOSTFS,22)

/* hostconsole device */
#define HOSTCONSOLE_CMD_GETCMDLINE MakeDeviceCmd(CMDC_HOSTCONSOLE,3)


/*****************************************************************************/


/* serial commands */
#define SER_CMD_STATUS      MakeDeviceCmd(CMDC_SERIAL,3)
#define SER_CMD_SETCONFIG   MakeDeviceCmd(CMDC_SERIAL,4)
#define SER_CMD_GETCONFIG   MakeDeviceCmd(CMDC_SERIAL,5)
#define SER_CMD_WAITEVENT   MakeDeviceCmd(CMDC_SERIAL,6)
#define SER_CMD_SETRTS      MakeDeviceCmd(CMDC_SERIAL,7)
#define SER_CMD_SETDTR      MakeDeviceCmd(CMDC_SERIAL,8)
#define SER_CMD_SETLOOPBACK MakeDeviceCmd(CMDC_SERIAL,9)
#define SER_CMD_BREAK       MakeDeviceCmd(CMDC_SERIAL,10)


/*****************************************************************************/


#ifndef EXTERNAL_RELEASE

/* micro-slot driver */
#define USLOTCMD_RESET		MakeDeviceCmd(CMDC_USLOT,3)
#define USLOTCMD_SETCLOCK	MakeDeviceCmd(CMDC_USLOT,4)
#define USLOTCMD_SEQ		MakeDeviceCmd(CMDC_USLOT,8)

/* mp driver */
#define MP_CMD_DISPATCH	        MakeDeviceCmd(CMDC_MP,3)

/* control port driver */
#define CPORT_CMD_CONFIGURE     MakeDeviceCmd(CMDC_CTLPORT,3)
#define CPORT_CMD_READEVENT     MakeDeviceCmd(CMDC_CTLPORT,4)
#define CPORT_CMD_WRITEEVENT    MakeDeviceCmd(CMDC_CTLPORT,5)
#define CPORT_CMD_WRITE         MakeDeviceCmd(CMDC_CTLPORT,6)
#define CPORT_CMD_READ          MakeDeviceCmd(CMDC_CTLPORT,7)


/*****************************************************************************/


#endif /* EXTERNAL_RELEASE */

#endif	/* __KERNEL_DEVICECMD_H */
