/* @(#) devices.c 96/06/30 1.52 */

/*#define DEBUG*/

#include <kernel/types.h>
#include <kernel/list.h>
#include <kernel/listmacros.h>
#include <kernel/item.h>
#include <kernel/device.h>
#include <kernel/driver.h>
#include <kernel/ddfnode.h>
#include <kernel/kernel.h>
#include <kernel/kernelnodes.h>
#include <kernel/io.h>
#include <kernel/task.h>
#include <kernel/usermodeservices.h>
#include <kernel/mem.h>
#include <kernel/operror.h>
#include <kernel/semaphore.h>
#include <stdio.h>
#include <string.h>
#include <kernel/internalf.h>

#define NO_DDF ((DDFNode*)1)

#ifdef DEBUG
#define DBUG(x)	printf x
#undef DEBUG_SAMESTACK
#else
#define DBUG(x)
#undef DEBUG_SAMESTACK
#endif

#define DBUG0(x) printf x

Semaphore *_DevSemaphore;

extern Item internalFindTask(char *);

/*****************************************************************************/


static int32 icd_c(Device *dev, void *p, uint32 tag, uint32 arg)
{
    TOUCH(p);

    DBUG(("icd_c dev=%lx tag=%lx arg=%lx\n",dev,tag,arg));
    switch (tag)
    {
	case CREATEDEVICE_TAG_DRVR:
	{
	    Driver *drvr = (Driver *)CheckItem((Item)arg,KERNELNODE,DRIVERNODE);
	    if (!drvr)
		return BADTAGVAL;
	    dev->dev_Driver = drvr;
	    break;
	}
	case CREATEDEVICE_TAG_LOWDEVICE:
	    if (arg && CheckItem((Item)arg,KERNELNODE,DEVICENODE) == NULL)
		return BADTAGVAL;
	    dev->dev_LowerDevice = (Item)arg;
	    break;
	case CREATEDEVICE_TAG_HWRESOURCE:
	    dev->dev_HWResource = (HardwareID)arg;
	    break;
	case CREATEDEVICE_TAG_DDFNODE:
	    dev->dev_DDFNode = (DDFNode *)arg;
	    break;
	default:
		return BADTAG;
    }
    return 0;
}

DDFNode* FindDDF(char const* name)
{
    DDFNode *ddf;
    DDFNode *r= NULL;

    StartScanProtectedList(&KB_FIELD(kb_DDFs), ddf, DDFNode)
    {
	if(!strcasecmp(ddf->ddf_n.n_Name, name))
	{
	    r= ddf;
	    break;
	}
    }
    EndScanProtectedList(&KB_FIELD(kb_DDFs));
    return r;
}

Item internalCreateDevice(Device *dev, TagArg *a)
{
    Item ret;
    Driver *drv;

    DBUG(("CreateDevice(%x)\n", dev));

    ret = TagProcessor(dev,a,icd_c,0);
    if (ret < 0)
	return ret;
    drv = dev->dev_Driver;
    if (drv == NULL)
    {
	DBUG(("no driver for device\n"));
	return BADTAG;
    }

    if (drv->drv_DeviceDataSize > 0)
    {
	dev->dev_DriverData = SuperAllocMem(drv->drv_DeviceDataSize,
					MEMTYPE_NORMAL | MEMTYPE_FILL);
	if (dev->dev_DriverData == NULL)
	    return NOMEM;
    }

    ret = OpenItem(dev->dev_Driver->drv.n_Item, 0);
    DBUG(("Open driver err %x\n", ret));
    if (ret < 0)
	return ret;

    if (drv->drv_CreateDev)
    {
        ret = (*dev->dev_Driver->drv_CreateDev)(dev);
	DBUG(("CreateDevice(%x) driver err %x\n", dev, ret));
        if (ret < 0)
	{
	    CloseItem(dev->dev_Driver->drv.n_Item);
	    return ret;
	}
    }

    PrepList(&dev->dev_IOReqs);
    internalLockSemaphore(_DevSemaphore,SEM_WAIT);
    InsertNodeFromTail(&dev->dev_Driver->drv_Devices, (Node *)dev);
    internalUnlockSemaphore(_DevSemaphore);

    return dev->dev.n_Item;
}

Err internalDeleteDevice(Device *dev, Task *t)
{
	Err	err;
	Driver *drv = dev->dev_Driver;

	DBUG(("DeleteDevice(%x) item %x\n", dev, dev->dev.n_Item));
        if (dev->dev.n_OpenCount && ((t->t_Flags & TASK_EXITING) == 0))
        {
            return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_ItemStillOpened);
        }

	if (drv->drv_DeleteDev)
	{
	    err = (*drv->drv_DeleteDev)(dev);
	    if (err < 0)
		return err;
	}

	if (drv->drv_DeviceDataSize > 0 && dev->dev_DriverData != NULL)
		SuperFreeMem(dev->dev_DriverData, drv->drv_DeviceDataSize);

	internalLockSemaphore(_DevSemaphore, SEM_WAIT);
	REMOVENODE((Node *)dev);
	internalUnlockSemaphore(_DevSemaphore);

	err = internalCloseItem(dev->dev_Driver->drv.n_Item, t);
	return err;
}


/*****************************************************************************/


Item internalOpenDevice(Device *dev, void *a, Task *t)
{
Item ret;
Item ret2;

    DBUG(("OpenDevice(%x) %s, item %x, %d opens\n",
	dev, dev->dev.n_Name, dev->dev.n_Item, dev->dev.n_OpenCount));
    TOUCH(a);
    if (dev->dev_Driver->drv_OpenDev)
    {
        ret = (*dev->dev_Driver->drv_OpenDev)(dev, t);
	DBUG(("OpenDevice: %s returns %x\n", dev->dev.n_Name, ret));
	if (ret < 0)
	    return ret;
    } else
    {
        DBUG(("OpenDevice: '%s' has null dev_Open pointer\n", dev->dev.n_Name));
	ret = dev->dev.n_Item;
    }

    if (dev->dev_LowerDevice > 0)
    {
	DBUG(("OpenLower %x\n", dev->dev_LowerDevice));
	ret2 = internalOpenItem(dev->dev_LowerDevice, 0, t);
	DBUG(("OpenDevice lower ret %lx\n", ret2));
	if (ret2 < 0)
	    return ret2;
    }

    DBUG(("OpenDevice ret %lx\n", ret));
    return ret;
}

static bool
SameStack(const DeviceStack *ds, const DeviceStackNode *dsn, const Device *dev)
{
	bool same;

	for (;;)
	{
		/* See if the current level is the same. */
		if (dsn->dsn_IsHW)
			same = (dsn->dsn_HWResource == dev->dev_HWResource);
		else
			same = (dsn->dsn_DDFNode == dev->dev_DDFNode);
		if (!same)
		{
#ifdef DEBUG_SAMESTACK
printf("SameStack: DIFFERENT: ds has ");
if (dsn->dsn_IsHW) printf("HW(%x) ", dsn->dsn_HWResource);
else printf("<%s> ", dsn->dsn_DDFNode->ddf_n.n_Name);
printf("dev has ");
if (dsn->dsn_IsHW) printf("HW(%x)\n", dev->dev_HWResource);
else printf("<%s>\n", dev->dev_Driver == NULL ? "no driver" : dev->dev_Driver->drv_DDF == NULL ? "no ddf" : dev->dev_Driver->drv_DDF->ddf_n.n_Name);
#endif
			return FALSE;
		}
#ifdef DEBUG_SAMESTACK
printf("SameStack: ok: ds has ");
if (dsn->dsn_IsHW) printf("HW(%x) ", dsn->dsn_HWResource);
else printf("%s ", dsn->dsn_DDFNode->ddf_n.n_Name);
printf("dev has ");
if (dsn->dsn_IsHW) printf("HW(%x)\n", dev->dev_HWResource);
else printf("%s\n", dev->dev_Driver->drv_DDF->ddf_n.n_Name);
#endif
		/* Step down to next level. */
		dsn = (DeviceStackNode *) NextNode(dsn);
		if (dev->dev_LowerDevice == 0)
		{
			/*
			 * No next node in Device stack.
			 * If Device has a HWResource, make sure the
			 * dsn has a HWResource node that matches.
			 */
			if (dev->dev_HWResource == 0)
			{
#ifdef DEBUG_SAMESTACK
printf("SameStack: no HW, ret %x\n", !IsNode(&ds->ds_Stack, dsn));
#endif
				return !IsNode(&ds->ds_Stack, dsn);
			}
			if (!IsNode(&ds->ds_Stack, dsn))
			{
#ifdef DEBUG_SAMESTACK
printf("SameStack: dev HW %x, no ds HW; ret FALSE\n", dev->dev_HWResource);
#endif
				return FALSE;
			}
#ifdef DEBUG_SAMESTACK
printf("SameStack: dev HW %x, ret %x\n", dev->dev_HWResource,
dsn->dsn_IsHW && dsn->dsn_HWResource == dev->dev_HWResource);
#endif
			return dsn->dsn_IsHW &&
				dsn->dsn_HWResource == dev->dev_HWResource;
		}
		if (!IsNode(&ds->ds_Stack, dsn))
		{
			/*
			 * No next node in dsn stack, but there is a
			 * next node in Device stack.  Not the same.
			 */
#ifdef DEBUG_SAMESTACK
printf("SameStack: ret FALSE\n");
#endif
			return FALSE;
		}
		dev = (Device *) LookupItem(dev->dev_LowerDevice);
	}
}

/**
|||	AUTODOC -public -class Kernel -group Devices -name DeviceStackIsIdentical
|||	Compare a DeviceStack to an open device stack.
|||
|||	  Synopsis
|||
|||	    bool DeviceStackIsIdentical(DeviceStack *ds, Item devItem);
|||
|||	  Description
|||
|||	    Compares the specified DeviceStack to the specified item,
|||	    which should be an open device stack, returned from
|||	    OpenDeviceStack().
|||
|||	  Arguments
|||
|||	    ds
|||	        Pointer to the DeviceStack to be compared.
|||	    devItem
|||	        Item number of the open device stack to be compared.
|||
|||	  Return Value
|||
|||	    Returns TRUE if the two device stacks are identical
|||	    (from top to bottom).
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V28.
|||
|||	  Associated Files
|||
|||	    <kernel/device.h>
|||
|||	  See Also
|||
|||	    CreateDeviceStackList(), OpenDeviceStack()
|||
**/

bool
DeviceStackIsIdentical(const DeviceStack *ds, Item devItem)
{
	Device *dev;

	dev = (Device *) CheckItem(devItem, KERNELNODE,DEVICENODE);
	if (dev == NULL)
		return FALSE;
	return SameStack(ds, (DeviceStackNode *) FirstNode(&ds->ds_Stack), dev);
}

/**
|||	AUTODOC -public -class Kernel -group Devices -name OpenDeviceStack
|||	Opens a stack of devices.
|||
|||	  Synopsis
|||
|||	    Item OpenDeviceStack(DeviceStack *ds);
|||
|||	  Description
|||
|||	    Opens a stack of devices.  This stack is normally one
|||	    node from a list of DeviceStacks returned from
|||	    CreateDeviceStackList.
|||
|||	  Arguments
|||
|||	    ds
|||	        Pointer to the DeviceStack to be opened.
|||
|||	  Return Value
|||
|||	    Returns the item number of the opened device stack, or
|||	    an error code if there was an error in opening the device
|||	    stack.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V28.
|||
|||	  Associated Files
|||
|||	    <kernel/device.h>
|||
|||	  See Also
|||
|||	    CloseDeviceStack(), CreateDeviceStackList()
|||
**/

Item
OpenDeviceStack(const DeviceStack *ds)
{
	DeviceStackNode *dsn;
	Driver *drv;
	Device *dev;
	Item driver;
	Item device;
	Item lowDevice;
	Item err;
	HardwareID hw;

	if (IsEmptyList(&ds->ds_Stack))
		return MAKEKERR(ER_SEVER,ER_C_STND,ER_ParamError);

#ifdef DEBUG
	printf("OpenDeviceStack: ");
	PrintDeviceStack(ds);
	printf("\n");
#endif

	/*
	 * Find the bottom-most driver on the stack for which a Device
	 * is to be created.  If there are no reopeners in the stack,
	 * this is the real bottom of the stack.  If there is a mulitplexor
	 * which is already open and has an identical stack below it,
	 * just reopen the reopener and create Devices only for Drivers
	 * above the reopener.  But if REOPEN_LOWER is set, reopen
	 * the device BELOW the reopener rather than the reopener
	 * itself, and create Devices for the reopener and Drivers above it.
	 */
	ScanList(&ds->ds_Stack, dsn, DeviceStackNode)
	{
		if (dsn->dsn_IsHW)
			/* Hit bottom. */
			break;
		if ((dsn->dsn_DDFNode->ddf_Flags &
				(DDFF_REOPEN_ME | DDFF_REOPEN_LOWER)) == 0)
			/* Not a reopener; keep looking. */
			continue;
		DBUG(("DDF %x: %s is reopener\n", dsn->dsn_DDFNode,
			dsn->dsn_DDFNode->ddf_n.n_Name));
		drv = (Driver *)  FindNamedNode(&KB_FIELD(kb_Drivers),
					dsn->dsn_DDFNode->ddf_Driver);
		if (drv == NULL)
			/* Reopener driver is not already open. */
			continue;
		/*
		 * See if any of this Driver's Devices have an identical
		 * stack to the one we're trying to open.
		 */
		ScanList(&drv->drv_Devices, dev, Device)
		{
			if (SameStack(ds, dsn, dev))
			{
				/* Found one. */
				DBUG(("Found reopener %s open %d times\n",
					dsn->dsn_DDFNode->ddf_n.n_Name,
					dev->dev.n_OpenCount));
				if (dsn->dsn_DDFNode->ddf_Flags &
					DDFF_REOPEN_ME)
				{
					/*
					 * "Reopen-me" device.
					 * Start creating Devices at the one
					 * above this one.
					 */
					lowDevice = dev->dev.n_Item;
					dsn = (DeviceStackNode *) PrevNode(dsn);
					if (!IsNodeB(&ds->ds_Stack, (Node *) dsn))
					{
						DBUG(("reopener at top\n"));
						return OpenItem(lowDevice, 0);
					}
				} else /* DDFF_REOPEN_BELOW */
				{
					/*
					 * "Reopen-below" device.
					 * Start creating Devices at this one.
					 */
					if (dev->dev_LowerDevice > 0)
						lowDevice = dev->dev_LowerDevice;
					else
						lowDevice = 0;
				}
				hw = 0;
				goto DoCreates;
			}
		}
	}
	/*
	 * Didn't find an appropriate reopener.
	 * Start creating Devices from the bottom of the stack.
	 */
	dsn = (DeviceStackNode *) LastNode(&ds->ds_Stack);
	if (dsn->dsn_IsHW)
	{
		hw = dsn->dsn_HWResource;
		dsn = (DeviceStackNode *) PrevNode(dsn);
		if (!IsNodeB(&ds->ds_Stack, dsn))
			return MAKEKERR(ER_SEVER,ER_C_STND,ER_ParamError);
	} else
	{
		hw = 0;
	}
	lowDevice = (Item)0;

	/*
	 * Scan stack backwards (bottom to top).
	 * Open the Driver for each node, then create a Device for it.
	 */
DoCreates:
	device = -1;
	for ( ;  IsNodeB(&ds->ds_Stack, (Node *) dsn);
		dsn = (DeviceStackNode *) PrevNode(dsn))
	{
		if (dsn->dsn_IsHW)
		{
			hw = dsn->dsn_HWResource;
			continue;
		}
		DBUG(("Opening driver %s\n", dsn->dsn_DDFNode->ddf_Driver));
		err = FindAndOpenItemVA(MKNODEID(KERNELNODE,DRIVERNODE),
				TAG_ITEM_NAME, dsn->dsn_DDFNode->ddf_Driver,
				TAG_END);
		DBUG(("ODS: FindAndOpen driver %s got %x\n",
			dsn->dsn_DDFNode->ddf_n.n_Name, err));
		if (err < 0)
			goto Error;
		driver = err;

		DBUG(("Creating device %s\n", dsn->dsn_DDFNode->ddf_n.n_Name));
		err = CreateItemVA(MKNODEID(KERNELNODE,DEVICENODE),
				TAG_ITEM_NAME, dsn->dsn_DDFNode->ddf_n.n_Name,
				CREATEDEVICE_TAG_DRVR, driver,
				CREATEDEVICE_TAG_LOWDEVICE, lowDevice,
				CREATEDEVICE_TAG_HWRESOURCE, hw,
				CREATEDEVICE_TAG_DDFNODE, dsn->dsn_DDFNode,
				TAG_END);
		DBUG(("ODS: Create device %s got %x\n",
			dsn->dsn_DDFNode->ddf_n.n_Name, err));
		CloseItem(driver);
		if (err < 0)
			goto Error;
		device = err;
		lowDevice = device;
		hw = 0;
	}

	/* Transfer ownership of each device item to the operator. */
	dev = (Device *) LookupItem(device);
	for (;;)
	{
		(void) SetItemOwner(dev->dev.n_Item, KB_FIELD(kb_OperatorTask));
		if (dev->dev_LowerDevice <= 0)
			break;
		dev = (Device *) LookupItem(dev->dev_LowerDevice);
	}

	return OpenItem(device, 0);

Error:
	/*
	 * Free up the partial device stack on error.
	 * Walk back down the device stack.
	 * Delete each device item and close each driver item.
	 */
	DBUG(("ODS: cleanup, err %x\n", err));
	while (device > 0)
	{
		dev = (Device *) LookupItem(device);
		device = dev->dev_LowerDevice;
		DeleteItem(dev->dev.n_Item);
		CloseItem(dev->dev_Driver->drv.n_Item);
	}
	return err;
}

/**
|||	AUTODOC -public -class Kernel -group Devices -name CloseDeviceStack
|||	Closes a stack of devices.
|||
|||	  Synopsis
|||
|||	    Err CloseDeviceStack(Item devItem);
|||
|||	  Description
|||
|||	    Closes a stack of devices previously opened by OpenDeviceStack.
|||
|||	  Arguments
|||
|||	    devItem
|||	        Item number of the DeviceStack to be closed.
|||
|||	  Return Value
|||
|||	    Returns zero or a negative error code for failure.  Possible
|||	    error codes currently include:
|||
|||	    TBD
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V28.
|||
|||	  Associated Files
|||
|||	    <kernel/device.h>
|||
|||	  See Also
|||
|||	    OpenDeviceStack(), CreateDeviceStackList()
|||
**/

Err
CloseDeviceStack(Item devItem)
{
    return CloseItem(devItem);
}

/*****************************************************************************/


static void DeleteDeviceIOReqs(Task *task, Device *dev)
{
Item   *ip;
int32   i;
int32   resCnt;
Item    it;
uint32  cnt;
IOReq  *ior;

    /* first determine if the current task has more than one open on the
     * given device
     */

    ip  = task->t_ResourceTable;
    ip += task->t_ResourceCnt; /* go to last entry */
    cnt = 0;

    for (i = 0; i < task->t_ResourceCnt; i++)
    {
        it = *--ip;
        if (it == (dev->dev.n_Item | ITEM_WAS_OPENED))
            cnt++;
    }

    /* If there's only one open, it means the task is in the process of
     * closing the device for the last time. We must therefore throw away
     * any IOReqs for that device that the task might still have.
     */

    if (cnt == 1)
    {
        do
        {
            resCnt = task->t_ResourceCnt;
            for (i = resCnt - 1; i >= 0; i--)
            {
                it = task->t_ResourceTable[i];
                if (it >= 0)
                {
                    ior = (IOReq *)CheckItem(it,KERNELNODE,IOREQNODE);
                    if (ior && (ior->io_Dev == dev))
                    {
#ifdef BUILD_STRINGS
                        if ((task->t_Flags & TASK_EXITING) == 0)
                        {
                            if (task->t_ThreadTask)
                                printf("WARNING: thread");
                            else
                                printf("WARNING: task");

                            printf(" '%s' is closing the '%s' device w/o having deleted ioreq $%06x\n",task->t.n_Name,dev->dev.n_Name,it);
                            if ((ior->io.n_Flags & NODE_NAMEVALID) && (ior->io.n_Name))
                                printf("         (name '%s')\n",ior->io.n_Name);
                        }
#endif
                        internalDeleteItem(it);
                    }
                }
            }
        }
        while (resCnt != task->t_ResourceCnt);
    }
}


/*****************************************************************************/


Err internalCloseDevice(Device *dev, Task *t)
{
	Err err;

	TOUCH(t);
	DBUG(("CloseDevice(%x) %s, item %x, %d opens\n",
		dev, dev->dev.n_Name, dev->dev.n_Item, dev->dev.n_OpenCount));
	if (dev->dev_Driver->drv_CloseDev)
	{
		err = (*dev->dev_Driver->drv_CloseDev)(dev, t);
		if (err < 0)
			return err;
	}

	if (dev->dev_LowerDevice > 0)
	{
		DBUG(("CloseLower %x\n", dev->dev_LowerDevice));
		err = internalCloseItem(dev->dev_LowerDevice, t);
		if (err < 0)
		{
			/* FIXME: Hmmm, the device's close vector has already
			 *        been called when we get here. Should we
			 *        call the open vector again?
			 */
			return err;
		}
	}

        DeleteDeviceIOReqs(t, dev);

        if (dev->dev.n_OpenCount == 0)
        {
/*if (dev->dev.n_Owner != t->t.n_Item) */
		DBUG(("Deleting closed dev %x\n", dev->dev.n_Item));
		return internalDeleteItem(dev->dev.n_Item);
        }

        return 0;
}

Err SetDeviceOwner(Device *dev, Item newOwner)
{
	Err err;
	uint32 oldPriv;
	Driver *drv = dev->dev_Driver;

	/*
	 * The Driver item has an open associated with this Device item.
	 * This is the open due to the OpenItem() call in CreateDevice.
	 * Transfer this open of the Driver to the Operator.
	 */
	oldPriv = PromotePriv(CURRENTTASK);
	err = OpenItemAsTask(drv->drv.n_Item, NULL, newOwner);
	DemotePriv(CURRENTTASK, oldPriv);
	if (err < 0)
		return err;
	err = CloseItem(drv->drv.n_Item);

	if (drv->drv_ChangeDeviceOwner)
	{
		oldPriv = PromotePriv(CURRENTTASK);
		err = (*(drv->drv_ChangeDeviceOwner))(dev, newOwner);
		DemotePriv(CURRENTTASK, oldPriv);
	}
	return err;
}
