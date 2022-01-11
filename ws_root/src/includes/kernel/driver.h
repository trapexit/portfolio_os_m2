#ifndef __KERNEL_DRIVER_H
#define __KERNEL_DRIVER_H


/******************************************************************************
**
**  @(#) driver.h 96/02/27 1.27
**
**  Kernel driver management definitions
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_NODES_H
#include <kernel/nodes.h>
#endif

#ifndef __KERNEL_MSGPORT_H
#include <kernel/msgport.h>
#endif


/*****************************************************************************/


typedef struct DeviceStatus
{
    uint8   ds_DriverIdentity;
    uint8   ds_DriverStatusVersion;
    uint8   ds_FamilyCode;
    uint8   ds_Reserved0;
    uint32  ds_MaximumStatusSize;
    uint32  ds_DeviceBlockSize;
    uint32  ds_DeviceBlockCount;
    uint32  ds_DeviceBlockStart;
    uint32  ds_DeviceFlagWord;
    uint32  ds_DeviceUsageFlags;
    uint32  ds_DeviceLastErrorCode;
    uint32  ds_DeviceMediaChangeCntr;
    uint32  ds_Reserved1;
} DeviceStatus;

/* Values for ds_DriverIdentity */
#define DI_OTHER        0x00
#define DI_TIMER        0x01
#define DI_RAM          0x02
#define DI_CONTROLPORT  0x03
#define DI_FILE         0x04
#define DI_M2CD_CDROM   0x05

/* Values for ds_FamilyCode */
#define DS_DEVTYPE_OTHER 0
#define DS_DEVTYPE_CDROM 1
#define DS_DEVTYPE_FILE  3

/* Values for ds_DeviceUsageFlags. This field contains device
 * characteristic/usage information.
 *
 * DS_USAGE_FILESYSTEM: The device can support a filesystem of some sort,
 *                      and the File Folio may attempt filesystem-mount
 *                      operations.
 *
 * DS_USAGE_PRIVACCESS: The device is for use by 3DO privileged code only.
 *
 * DS_USAGE_READONLY: The device currently cannot be written to.
 *
 * DS_USAGE_OFFLINE: The device cannot be accessed because it (or its
 *                   media) is currently off-line.
 *
 * DS_USAGE_TRUEROM: The device is a true, honest-to-Babbage ROM, which cannot
 *                   possibly be written to under any circumstances.
 *
 * DS_USAGE_STATIC_MAPPABLE: The device can be mapped into memory and accessed
 *                           via the CPU.  The mapping is "static" (the entire
 *                           device will be mapped in, and once mapped its
 *                           address cannot change).
 *
 * DS_USAGE_DYNAMIC_MAPPABLE: The device can be mapped into memory and accessed
 *                            via the CPU.  The mapping is "dynamic": there may
 *                            be restrictions on how much of the device can be
 *                            mapped in at any one time, and the mapping may
 *                            change from time to time.
 *
 * DS_USAGE_UNSIGNED_OK: The removable media in this device has been
 *                       authorized by DIPIR to run unsigned executables.
 *
 * DS_USAGE_EXTERNAL: The device, or its media, can be accessed from outside
 *                    of the 3DO box.
 *
 * DS_USAGE_REMOVABLE: The media in the device can be physically removed or
 *                     ejected.
 */
#define DS_USAGE_FILESYSTEM        0x80000000
#define DS_USAGE_PRIVACCESS        0x40000000
#define DS_USAGE_READONLY          0x20000000
#define DS_USAGE_OFFLINE           0x10000000
#define DS_USAGE_TRUEROM           0x08000000
#define DS_USAGE_STATIC_MAPPABLE   0x04000000
#define DS_USAGE_DYNAMIC_MAPPABLE  0x02000000
#define DS_USAGE_UNSIGNED_OK       0x01000000
#define DS_USAGE_EXTERNAL          0x00800000
#define DS_USAGE_REMOVABLE         0x00400000
#define DS_USAGE_HOSTFS            0x00200000


/*****************************************************************************/


/* Data structures for managing devices which are, or which can be mapped
 * into memory.  Relevant iff the DS_USAGE_mumble_MAPPABLE bit is set in the
 * ds_DeviceUsageFlags word.
 */

/* Bits in mmdi_Flags, mmr_Flags, mrr_Flags */
#define	MM_MAPPABLE	0x00000001	/* Device is mappable     */
#define	MM_READABLE	0x00000002	/* Mappable for reading   */
#define	MM_WRITABLE	0x00000004	/* Mappable for writing   */
#define	MM_EXECUTABLE	0x00000008	/* Mappable for executing */
#define	MM_EXCLUSIVE	0x00000010	/* Exclusive access       */

/* Info about device's mapping capabilities. */
typedef struct MemMappableDeviceInfo
{
    uint32 mmdi_Flags;             /* What does the device support? */
    uint8  mmdi_SpeedPenaltyRatio; /* Speed of access to device     */
    uint8  mmdi_rfu1[3];
    uint32 mmdi_MaxMappableBlocks; /* Max mappable at one time      */
    uint32 mmdi_CurBlocksMapped;   /* Currently mapped              */
    uint32 mmdi_Permissions;       /* What is device allowed to do? */
} MemMappableDeviceInfo;

/* Bits in mmdi_Permissions */
#define	XIP_OK		0x00000001

/* Request to map part of a device. */
typedef struct MapRangeRequest
{
    uint32 mrr_Flags;             /* Request to map how? */
    uint32 mrr_BytesToMap;        /* How much to map     */
} MapRangeRequest;

/* Response to MapRangeRequest. */
typedef struct MapRangeResponse
{
    uint32  mrr_Flags;            /* How was the device mapped? */
    void   *mrr_MappedArea;       /* Address it was mapped at   */
} MapRangeResponse;


#ifndef EXTERNAL_RELEASE
/*****************************************************************************/

/* A driver command table is an array of pairs
 *  (command number, function pointer).
 */
typedef int32 DriverCmdFunction(struct IOReq *ior);

typedef struct DriverCmdTable
{
    uint32             dc_Command;
    DriverCmdFunction *dc_Function;
} DriverCmdTable;

typedef struct Driver
{
    OpeningItemNode drv;

    uint32	drv_DeviceDataSize;
    Err		(*drv_DeleteDrv)(struct Driver *drv);
    Item	(*drv_OpenDrv)(struct Driver *drv, struct Task *t);
    Err		(*drv_CloseDrv)(struct Driver *drv, struct Task *t);
    Err		(*drv_ChangeDriverOwner)(struct Driver *drv, Item newOwner);

    Item	(*drv_CreateDev)(struct Device *dev);
    Err		(*drv_DeleteDev)(struct Device *dev);
    Item	(*drv_OpenDev)(struct Device *dev, struct Task *t);
    Err		(*drv_CloseDev)(struct Device *dev, struct Task *t);
    Err		(*drv_ChangeDeviceOwner)(struct Device *dev, Item newOwner);

    DriverCmdTable * drv_CmdTable;
    uint32	drv_NumCommands;
    int32	(*drv_DispatchIO)(struct IOReq *ior);
    void	(*drv_AbortIO)(struct IOReq *ior);
    Item	(*drv_CreateIOReq)(struct IOReq *ior);
    Err		(*drv_DeleteIOReq)(struct IOReq *ior);
    Err		(*drv_ChangeIOReqOwner)(struct IOReq *ior, Item newOwner);
    bool	(*drv_Rescan)(struct Device *dev);

    struct DDFNode *drv_DDF;
    List	drv_Devices;
    uint32	drv_IOReqSize;
    Item	drv_Module;
} Driver;


enum driver_tags
{
    CREATEDRIVER_TAG_ABORTIO = TAG_ITEM_LAST+1,
    CREATEDRIVER_TAG_NUMCMDS,	/* size of command table */
    CREATEDRIVER_TAG_CMDTABLE,	/* driver command table */
    CREATEDRIVER_TAG_CREATEDRV,	/* create driver */
    CREATEDRIVER_TAG_DISPATCH,	/* command dispatch function */
    CREATEDRIVER_TAG_CRIO,          /* create IOReq */
    CREATEDRIVER_TAG_DLIO,          /* delete IOReq */
    CREATEDRIVER_TAG_OPENDEV,       /* open device */
    CREATEDRIVER_TAG_CLOSEDEV,      /* close device */
    CREATEDRIVER_TAG_CREATEDEV,     /* create device */
    CREATEDRIVER_TAG_DELETEDEV,     /* delete device */
    CREATEDRIVER_TAG_CHOWN_IO,      /* change owner of IOReq */
    CREATEDRIVER_TAG_RESCAN,        /* rescan for hot insert/remove */
    CREATEDRIVER_TAG_NODDF,         /* driver has no DDF */
    CREATEDRIVER_TAG_IOREQSIZE,     /* size of IOReqs */
    CREATEDRIVER_TAG_OPENDRV,       /* open driver */
    CREATEDRIVER_TAG_CLOSEDRV,      /* close driver */
    CREATEDRIVER_TAG_DELETEDRV,     /* delete driver */
    CREATEDRIVER_TAG_DEVICEDATASIZE, /* size of dev_DriverData */
    CREATEDRIVER_TAG_MODULE,        /* module of driver */
    CREATEDRIVER_TAG_CHOWN_DRV,     /* change owner of driver */
    CREATEDRIVER_TAG_CHOWN_DEV,     /* change owner of device */
};

#define	CREATEDRIVER_TAG_CHANGEOWNER CREATEDRIVER_TAG_CHOWN_IO


/*****************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif


int32 SoftReset(void);


#ifdef	__cplusplus
}
#endif	/* __cplusplus */

#endif /* EXTERNAL_RELEASE */
/*****************************************************************************/


#endif	/* __KERNEL_DRIVER_H */
