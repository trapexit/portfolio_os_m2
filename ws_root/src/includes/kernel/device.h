#ifndef __KERNEL_DEVICE_H
#define __KERNEL_DEVICE_H

/******************************************************************************
**
**  @(#) device.h 96/07/22 1.39
**
**  Kernel device management definitions
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_ITEM_H
#include <kernel/item.h>
#endif

#ifndef __KERNEL_NODES_H
#include <kernel/nodes.h>
#endif

#ifndef __KERNEL_LIST_H
#include <kernel/list.h>
#endif

#ifndef __KERNEL_KERNELNODES_H
#include <kernel/kernelnodes.h>
#endif

#ifndef EXTERNAL_RELEASE
struct IOReq;
#endif

#ifndef EXTERNAL_RELEASE
struct Task;
#endif

typedef struct Device
{
        OpeningItemNode	dev;
#ifndef EXTERNAL_RELEASE
        uint32		dev_rsvd[3];	/* for expansion of the public part */
        struct Driver *	dev_Driver;	/* Pointer to the Driver */
	void *		dev_DriverData;	/* For use by device driver */
        List		dev_IOReqs;	/* List of this device's IOReqs */
	Item		dev_LowerDevice; /* Next device in stack */
	HardwareID	dev_HWResource;	/* Hardware, if low-level device */
	void *		dev_DDFNode;	/* Pointer to DDFNode */

        void     *dev_DemandLoad;	/* for demand-loader */
#endif /* EXTERNAL_RELEASE */
} Device;

/* Bits in n_Flags of the Device item node. */
#define	DEVF_OFFLINE	0x1		/* Device is offline */

#ifndef EXTERNAL_RELEASE

enum device_tags
{
	CREATEDEVICE_TAG_DRVR = TAG_ITEM_LAST+1,
	CREATEDEVICE_TAG_LOWDEVICE,
	CREATEDEVICE_TAG_HWRESOURCE,
	CREATEDEVICE_TAG_DDFNODE,
};

#endif /* EXTERNAL_RELEASE */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef EXTERNAL_RELEASE

extern Item CreateDevice(const char *name, uint8 pri, Item driver);
extern int MatchDeviceName(const char *name, const char *fullname, uint32 part);
#define DEVNAME_TYPE	0
#define DEVNAME_VERSION	1
#define DEVNAME_MFG	2

#define DeleteDevice(x)	DeleteItem(x)

#endif /* EXTERNAL_RELEASE */

/*
 * DDFQuery is used to search for devices which satisfy a set of conditions.
 */
typedef enum { 
	DDF_EQ, DDF_GT, DDF_LT, DDF_GE, DDF_LE, DDF_NE,
	DDF_DEFINED, DDF_NOTDEFINED
} DDFOp;

typedef struct DDFQuery 
{
	char*	ddfq_Name;		/* Name of capability */
	DDFOp	ddfq_Operator;		/* Comparison operator */
	union {				/* Value to be compared */
	  int32	  ddfq_Int;
	  char*	  ddfq_String;
	}	ddfq_Value;
	uint32	ddfq_Flags;		/* See below */
} DDFQuery;

/* Bits in ddfq_Flags */
#define	DDF_INT		0x00000001	/* ddfq_Value is (int32) */
#define	DDF_STRING	0x00000002	/* ddfq_Value is (char *) */

/*
 * DeviceStackNode is an entry in a DeviceStack.
 * Each DeviceStackNode points to either a DDFNode (if the DeviceStackNode
 * represents a high-level device) or a HWResource (if the DeviceStackNode
 * represents a low-level device).
 */
typedef struct DeviceStackNode
{
	Node		dsn_n;		/* To link into a DeviceStack */
	union {
	  const struct DDFNode *dsnu_DDFNode;	/* Ptr to DDFNode */
	  HardwareID	  dsnu_HWResource; /* Ptr to HWResource */
	}		dsn_u;
	bool		dsn_IsHW;	/* HWResource or DDFNode? */
} DeviceStackNode;

#define	dsn_DDFNode	dsn_u.dsnu_DDFNode
#define	dsn_HWResource	dsn_u.dsnu_HWResource

/*
 * DeviceStack is a stack of (unopened) devices.
 * Each device in the stack is represented by a DeviceStackNode.
 */
typedef struct DeviceStack
{
	Node		ds_n;		/* To link to a DeviceStackList */
	List		ds_Stack;	/* Stack of DeviceStackNodes */
} DeviceStack;

/*
 * DeviceListNode is a node in a list.
 * Such a list is returned from CreateDeviceList().
 */
typedef struct DeviceListNode
{
	Node		dln_n;		/* To link into a List */
	struct DDFNode * dln_DDFNode;	/* Ptr to DDFNode */
} DeviceListNode;

struct HWResource;

/* Preferred functions for finding and opening devices. */
extern Err	CreateDeviceStackList(List **pList, const DDFQuery query[]);
extern Err	CreateDeviceStackListVA(List **pList, ...);
extern void	DeleteDeviceStackList(List *l);
extern Item	OpenDeviceStack(const DeviceStack *ds);
extern Err	CloseDeviceStack(Item devItem);

extern Err	NextHWResource(struct HWResource *hwr);

#ifndef EXTERNAL_RELEASE

/* For use by ROM apps, to open the CD-ROM device. */
extern Item	OpenRomAppMedia(void);

/* Lower-level functions for finding and opening devices. */
extern Item	OpenNamedDeviceStack(const char *stackName);
extern Item	NamedDeviceStackMounted(const char *stackName);
extern bool	DeviceStackIsIdentical(const DeviceStack *ds, Item devItem);
extern Err	CreateDeviceList(List **pList, const DDFQuery query[]);
extern void	DeleteDeviceList(List* l);
extern DeviceStack *CreateDeviceStack(void);
extern void	DeleteDeviceStack(DeviceStack *ds);
extern Err	AppendDeviceStackDDF(DeviceStack *ds, const struct DDFNode *ddf);
extern DeviceStack *CopyDeviceStack(DeviceStack *ds);
extern Err	AppendDeviceStackHW(DeviceStack *ds, HardwareID hw);
extern struct DDFNode *FindDDF(const char *name);
extern bool	DDFCanManage(const struct DDFNode *ddf, const char *hwName);

#endif /* EXTERNAL_RELEASE */

#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif	/* __KERNEL_DEVICE_H */
