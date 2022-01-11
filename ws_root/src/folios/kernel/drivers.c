/* @(#) drivers.c 96/08/06 1.67 */

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
#include <kernel/operror.h>
#include <kernel/super.h>
#include <kernel/sysinfo.h>
#include <loader/loader3do.h>
#include <dipir/dipirpub.h>
#include <dipir/hwresource.h>
#include <string.h>
#include <stdio.h>
#include <kernel/internalf.h>
#include <hardware/cde.h>
#include <file/filefunctions.h>


#define	NO_DDF	((DDFNode*)1)

#define	DipirRoutines \
	((PublicDipirRoutines *)(KB_FIELD(kb_DipirRoutines)))

typedef struct DriverInfo
{
	Item	(*di_CreateDrv)(Driver *drv);
} DriverInfo;

#ifdef DEBUG
#define DBUG(x)	printf x
#else
#define DBUG(x)
#endif

/*
 * Find a driver function in a driver command table.
 * The function is identified by command number;
 * a pointer to the function is returned.
 */
static DriverCmdFunction *
LookupDriverCmd(Driver *drv, uint32 cmd)
{
	DriverCmdTable *table;

	for (table = drv->drv_CmdTable;
	     table < &drv->drv_CmdTable[drv->drv_NumCommands];
	     table++)
		if (table->dc_Command == cmd)
			return table->dc_Function;

#ifdef BUILD_STRINGS
	printf("Device %s doesn't support command $%x\n", drv->drv.n_Name ? drv->drv.n_Name : "?", cmd);
#endif

	return NULL;
}

/*
 * The default IOReq dispatch function
 * for drivers that don't supply their own.
 */
static int32
defaultDispatch(IOReq *ior)
{
	DriverCmdFunction *func;

	func = LookupDriverCmd(ior->io_Dev->dev_Driver,
				ior->io_Info.ioi_Command);
	if (func == NULL)
		ior->io_Error = BADCOMMAND;
	else if ((*func)(ior) == 0)
		return 0;
	internalCompleteIO(ior);
	return 1;
}

/*
 * Tag processor for driver creation.
 */
static int32
icd_c(Driver *drv, DriverInfo *di, uint32 tag, uint32 arg)
{
    int32 size;

    DBUG(("icd_c drv=%lx tag=%lx arg=%lx\n",drv,tag,arg));
    switch( tag )
    {
	case CREATEDRIVER_TAG_CREATEDRV:
		di->di_CreateDrv = Make_Func(Item,arg);
		break;
	case CREATEDRIVER_TAG_DELETEDRV:
		drv->drv_DeleteDrv = Make_Func(Err,arg);
		break;
	case CREATEDRIVER_TAG_OPENDRV:
		drv->drv_OpenDrv = Make_Func(Item,arg);
		break;
	case CREATEDRIVER_TAG_CLOSEDRV:
		drv->drv_CloseDrv = Make_Func(Err,arg);
		break;

	case CREATEDRIVER_TAG_CREATEDEV:
		drv->drv_CreateDev = Make_Func(Item,arg);
		break;
	case CREATEDRIVER_TAG_DELETEDEV:
		drv->drv_DeleteDev = Make_Func(Err,arg);
		break;
	case CREATEDRIVER_TAG_OPENDEV:
		drv->drv_OpenDev = Make_Func(Item,arg);
		break;
	case CREATEDRIVER_TAG_CLOSEDEV:
		drv->drv_CloseDev = Make_Func(Err,arg);
		break;

	case CREATEDRIVER_TAG_CMDTABLE:
		drv->drv_CmdTable = (DriverCmdTable *)arg;
		break;
	case CREATEDRIVER_TAG_NUMCMDS:
		drv->drv_NumCommands = (uint8)arg;
		break;
	case CREATEDRIVER_TAG_ABORTIO:
		drv->drv_AbortIO = Make_Func(void,arg);
		break;
	case CREATEDRIVER_TAG_DISPATCH:
		drv->drv_DispatchIO = Make_Func(int32,arg);
		break;
	case CREATEDRIVER_TAG_CRIO:
		drv->drv_CreateIOReq = Make_Func(Err,arg);
		break;
	case CREATEDRIVER_TAG_DLIO:
		drv->drv_DeleteIOReq = Make_Func(Err,arg);
		break;
	case CREATEDRIVER_TAG_CHOWN_IO:
		drv->drv_ChangeIOReqOwner = Make_Func(Err,arg);
		break;
	case CREATEDRIVER_TAG_CHOWN_DRV:
		drv->drv_ChangeDriverOwner = Make_Func(Err,arg);
		break;
	case CREATEDRIVER_TAG_CHOWN_DEV:
		drv->drv_ChangeDeviceOwner = Make_Func(Err,arg);
		break;
	case CREATEDRIVER_TAG_RESCAN:
		drv->drv_Rescan = Make_Func(bool,arg);
		break;
	case CREATEDRIVER_TAG_NODDF:
		drv->drv_DDF = NO_DDF;
		break;
	case CREATEDRIVER_TAG_IOREQSIZE:
		size = (int32)arg;
		if (size < sizeof(IOReq))
			return BADTAGVAL;
		drv->drv_IOReqSize = size;
		break;
	case CREATEDRIVER_TAG_DEVICEDATASIZE:
		size = (int32)arg;
		drv->drv_DeviceDataSize = size;
		break;
	case CREATEDRIVER_TAG_MODULE:
		drv->drv_Module = (Item)arg;
		break;
	default:
	    return BADTAG;
    }
    return 0;
}

Item
internalCreateDriver(Driver *drv, TagArg *a)
{
	Item ret;
	DriverInfo dinfo;

	DBUG(("internalCreateDriver(%lx,%lx)\n",drv,a));

        if ((CURRENTTASK->t.n_ItemFlags & ITEMNODE_PRIVILEGED) == 0) return BADPRIV;

	memset(&dinfo, 0, sizeof(dinfo));
	ret = TagProcessor(drv, a, icd_c, &dinfo);
	if (ret < 0)
		return ret;

	/* must have a name */
	DBUG(("validate driver tag args\n"));
	if (drv->drv.n_Name == NULL ||
	    drv->drv_AbortIO == NULL ||
	    drv->drv_Module == 0)
	{
#ifdef BUILD_STRINGS
		printf("CreateDriver: no name or Abort or module %x\n", drv->drv_Module);
#endif
		return BADTAGVAL;
	}
	if (drv->drv_IOReqSize == 0)
		drv->drv_IOReqSize = sizeof(IOReq);
	if (drv->drv_DispatchIO == NULL)
	{
		/* install default dispatch routine */
		drv->drv_DispatchIO = defaultDispatch;
	}

	if (drv->drv_DDF == NO_DDF)
		drv->drv_DDF = NULL;
	else if (drv->drv_DDF == NULL)
	{
		drv->drv_DDF = FindDDF(drv->drv.n_Name);
		if (drv->drv_DDF == NULL)
		{
#ifdef BUILD_STRINGS
			printf("No DDF for %s\n", drv->drv.n_Name);
#endif
			return MakeKErr(ER_SEVERE,ER_C_STND,ER_SoftErr);
		}
	}

	PrepList(&drv->drv_Devices);

	if(drv->drv_Module >= 0)
	    {
	    ret = internalOpenItem(drv->drv_Module, 0, TASK(KB_FIELD(kb_OperatorTask)));
	    DBUG(("CreateDriver: OpenMod(%x) ret %x\n", drv->drv_Module, ret));
	    if (ret < 0)
		return ret;
	    }

	DBUG(("CreateDrv(%s)=%lx\n", drv->drv.n_Name, dinfo.di_CreateDrv));
	if (dinfo.di_CreateDrv)
		ret = (*dinfo.di_CreateDrv)(drv);
	else
		ret = drv->drv.n_Item;

	DBUG(("CreateDrv(%s) return %lx\n", drv->drv.n_Name, ret));

	/* FIXME:  why isn't this semaphore-protected? */
	if (ret >= 0)
		InsertNodeFromTail(&KB_FIELD(kb_Drivers), (Node *)drv);
	return ret;
}

Err
internalDeleteDriver(Driver *drv, Task *t)
{
	Device *dev;
	Err err;

	TOUCH(t);
	DBUG(("DeleteDriver(%x)\n", drv));

	err = 0;

	/* FIXME:  why isn't this semaphore-protected? */

	/* Delete all devices using this driver. This is not really needed,
	 * since by the time we get here we should be guaranteed that there
	 * are no devices around. But just to be on the safe side...
	 */
	while ((dev = (Device *) RemHead(&drv->drv_Devices)) != NULL)
	{
		DBUG(("Deleting device %x (%x)\n", dev, dev->dev.n_Item));
		internalDeleteItem(dev->dev.n_Item);
	}

	if (drv->drv_DeleteDrv)
	{
		err = (*drv->drv_DeleteDrv)(drv);
		DBUG(("DeleteDriver(%x) driver err %x\n", drv, err));
		if (err < 0)
			return err;
	}

	/* Remove from the kb_Drivers list */
	REMOVENODE((Node *)drv);

	if(drv->drv_Module >= 0)
	    err = internalCloseItem(drv->drv_Module, t);

	return err;
}


/*****************************************************************************/


/* Load a driver from external storage */
Item internalLoadDriver(char *name)
{
DDFNode *ddf;
Item    result;
char    pathname[FILESYSTEM_MAX_PATH_LEN];
char    *pattern;
uint32	oldPriv;
Item	module;

static const TagArg modTags[] =
{
    MODULE_TAG_MUST_BE_PRIVILEGED, (TagData)TRUE,
    TAG_END
};

    DBUG(("intLoadDriver(%s)\n", name));

    /* Find the DDF for this driver. */
    ddf = FindDDF(name);
    if (ddf == NULL)
	return MakeKErr(ER_SEVERE,ER_C_STND,ER_NotFound);
    DBUG(("intLoadDriver: ddf %x, name %s, module %s\n",
	ddf, ddf->ddf_n.n_Name, ddf->ddf_Module));

    /* Find the module executable file. */
    pattern = AllocMem(strlen(ddf->ddf_Module) + 30,
			MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE);
    if (pattern == NULL)
	return NOMEM;
    sprintf(pattern, "System.m2/Drivers/%s.driver", ddf->ddf_Module);
    oldPriv = PromotePriv(CURRENTTASK);
    result = FindFileAndIdentifyVA(pathname, sizeof(pathname), pattern,
	FILESEARCH_TAG_SEARCH_FILESYSTEMS, (TagData) DONT_SEARCH_UNBLESSED,
	FILESEARCH_TAG_NOVERSION_IS_0_0, 1,
	FILESEARCH_TAG_FIND_HIGHEST_MATCH, 1,
	TAG_END);
    DemotePriv(CURRENTTASK, oldPriv);
    FreeMem(pattern, TRACKED_SIZE);
    if (result < 0)
	return result;

    /*
     * Open the module.  Call the module's main() function, which
     * creates all the Driver items supported by the module.
     * We can then close the module immediately, since each Driver item
     * created by main has opened the module itself.
     */
    DBUG(("LoadDriver: OpenModule %s\n", pathname));
    module = OpenModule(pathname, OPENMODULE_FOR_THREAD, modTags);
    DBUG(("LoadDriver: OpenModule ret %x\n", module));
    if (module < 0)
	return module;
    oldPriv = PromotePriv(CURRENTTASK);
    result = ExecuteModule(module, 0, 0);
    DemotePriv(CURRENTTASK, oldPriv);
    DBUG(("LoadDriver: ExecModule ret %x\n", result));
    (void) CloseModule(module);
    return result;
}


/*****************************************************************************/


Item
OpenDriver(Driver *drv, void *a, Task *t)
{
	Item ret;

	TOUCH(a);
	DBUG(("OpenDriver(%x), %d opens\n", drv, drv->drv.n_OpenCount));
	if (drv->drv_OpenDrv)
	{
		ret = (*drv->drv_OpenDrv)(drv, t);
		DBUG(("OpenDriver: %s returns %x\n", drv->drv.n_Name, ret));
		if (ret < 0)
			return ret;
	} else
	{
		DBUG(("OpenDriver: '%s' has null drv_Open\n", drv->drv.n_Name));
		ret = drv->drv.n_Item;
	}
	DBUG(("OpenDriver ret %x\n", ret));
	return ret;
}

Err
CloseDriver(Driver *drv, Task *t)
{
	Err ret;

	TOUCH(t);
	DBUG(("CloseDriver(%x), %d opens\n", drv, drv->drv.n_OpenCount));
	if (drv->drv_CloseDrv)
	{
		ret = (*drv->drv_CloseDrv)(drv, t);
		if (ret < 0)
			return ret;
	}

        if (drv->drv.n_OpenCount == 0)
        {
        uint8 oldPriv;

		oldPriv = PromotePriv(CURRENTTASK);
		internalDeleteItem(drv->drv.n_Item);
		DemotePriv(CURRENTTASK, oldPriv);
        }

	return 0;
}

Err
SetDriverOwner(Driver *drv, Item newOwner)
{
	Err err;
	uint32	oldPriv;

	err = 0;
	if (drv->drv_ChangeDriverOwner)
	{
		oldPriv = PromotePriv(CURRENTTASK);
		err = (*(drv->drv_ChangeDriverOwner))(drv, newOwner);
		DemotePriv(CURRENTTASK, oldPriv);
	}
	return err;
}

/**
|||	AUTODOC -private -class Kernel -group Device-Drivers -name NextHWResource
|||	Scans the list of HWResources.
|||
|||	  Synopsis
|||
|||	    Err NextHWResource(HWResource *hwr);
|||
|||	  Description
|||
|||	    Returns the next HWResource structure in the list of all
|||	    HWResources.  The structure is stored into the HWResource
|||	    structure pointed to by hwr.
|||
|||	    To scan the complete list of HWResources, the first call
|||	    should be made with the hwr_InsertID field of the structure
|||	    pointed to by hwr set to 0.  Calling NextHWResource with
|||	    the hwr_InsertID field set to 0 will return the first
|||	    HWResource.  Subsequent calls will iterate through the
|||	    list.  The caller should not modify the contents of the
|||	    structure pointed to by hwr between calls to NextHWResource.
|||
|||	  Arguments
|||
|||	    hwr
|||	        A pointer to an HWResource structure into which
|||	        is stored the next HWResource.  If the hwr_InsertID
|||	        field is 0, the first HWResource is returned.
|||	        Otherwise, the structure is assumed to be a structure
|||	        returned from a previous call to NextHWResource, and
|||	        the subsequent HWResource structure is returned.
|||
|||	  Return Value
|||
|||	    Returns 0 if a HWResource was returned, or a negative number
|||	    if the structure passed in was the last one in the list,
|||	    or if there was some other error.
|||
|||	  Implementation
|||
|||	    Supervisor call implemented in Kernel folio V28.
|||
|||	  Associated Files
|||
|||	    <dipir/hwresouce.h>
|||
**/

Err
NextHWResource(HWResource *p_hwr)
{
	int32 r;

	r = SuperQuerySysInfo(SYSINFO_TAG_HWRESOURCE, p_hwr, sizeof(HWResource));
	if (r == SYSINFO_END)
		return MAKEKERR(ER_SEVER,ER_C_STND,ER_NoHardware);
	if (r != SYSINFO_SUCCESS)
	{
		DBUG(("NextHWResource: sysinfo error %x\n", r));
		if (r >= 0)
			r = MakeKErr(ER_SEVERE,ER_C_STND,ER_SoftErr);
		return r;
	}
	return 0;
}

/**
|||	AUTODOC -private -class Kernel -group Device-Drivers -name FindHWResource
|||	Finds a specific HWResource.
|||
|||	  Synopsis
|||
|||	    Err FindHWResource(HardwareID hwID, HWResource *hwr);
|||
|||	  Description
|||
|||	    Finds the HWResource identified by the specified ID.
|||	    If hwr is not equal to NULL, a copy of the HWResource
|||	    structure is stored in the structure pointed to by hwr.
|||
|||	  Arguments
|||
|||	    hwID
|||	        The ID of the HWResource to find.
|||	        Often, a device driver obtains this ID from the
|||	        dev_HWResource field of its Device structure.
|||	    hwr
|||	        A pointer to an HWResource structure into which
|||	        is stored the found HWResource.  If hwr is NULL,
|||	        the HWResource structure is not returned, but the
|||	        return value of the function still indicates whether
|||	        the HWResource was found.
|||
|||	  Return Value
|||
|||	    Returns 0 if the HWResource was found, or a negative number
|||	    if the HWResource could not be located.
|||
|||	  Implementation
|||
|||	    Supervisor call implemented in Kernel folio V28.
|||
|||	  Associated Files
|||
|||	    <dipir/hwresouce.h>
|||
**/

Err
FindHWResource(HardwareID hwID, HWResource *p_hwr)
{
	HWResource hwr;
	Err err;

	hwr.hwr_InsertID = 0;
	for (;;)
	{
		err = NextHWResource(&hwr);
		if (err < 0)
			return err;
		if (hwr.hwr_InsertID == hwID)
			break;
	}

	if (p_hwr != NULL)
		*p_hwr = hwr;
	return 0;
}

/**
|||	AUTODOC -private -class Kernel -group Device-Drivers -name OpenSlotDevice
|||	Opens a slot device driver.
|||
|||	  Synopsis
|||
|||	    Item OpenSlotDevice(HardwareID hwID);
|||
|||	  Description
|||
|||	    The HWResource identified by the specified hwID is
|||	    examined and a device stack is created for the slot
|||	    driver appropriate for the specified hardware device.
|||	    The slot device can later be closed by calling
|||	    CloseDeviceStack().
|||
|||	  Arguments
|||
|||	    hwID
|||	        The ID of the HWResource whose slot driver is desired.
|||
|||	  Return Value
|||
|||	    Returns the item number of the open device stack
|||	    representing the slot device, or a negative number
|||	    if there was an error.
|||
|||	  Implementation
|||
|||	    Supervisor call implemented in Kernel folio V28.
|||
|||	  Associated Files
|||
|||	    <dipir/hwresouce.h>
|||
**/

Item
OpenSlotDevice(HardwareID hwID)
{
	char *slotname;
	DeviceStack *ds;
	DeviceStack *myds;
	DeviceStackNode *dsn;
	List *list;
	Item slotdev;
	HWResource hwr;
	char namebuf[20];
	DDFQuery query[2];
	Err err;

	DBUG(("OpenSlotDevice(%x)\n", hwID));
	/* Find the appropriate channel (PCMCIA, Microslot, etc) */
	err = FindHWResource(hwID, &hwr);
	if (err < 0)
		return err;
	switch (hwr.hwr_Channel)
	{
	case CHANNEL_PCMCIA:	slotname = "pcmcia";	break;
	case CHANNEL_MICROCARD:	slotname = "microslot";	break;
	/* The default case is for future expansion. */
	default:		sprintf(namebuf, "slot%x", hwr.hwr_Channel);
				slotname = namebuf;	break;
	}

	/* Find all device stacks for the slot device driver.  */
	query[0].ddfq_Name = "slot";
	query[0].ddfq_Operator = DDF_EQ;
	query[0].ddfq_Flags = DDF_STRING;
	query[0].ddfq_Value.ddfq_String = slotname;
	query[1].ddfq_Name = NULL;
	err = CreateDeviceStackList(&list, query);
	if (err < 0)
	{
		DBUG(("OpenSlotDevice: cannot get slot stack list\n"));
		return err;
	}
	/*
	 * Slot drivers normally claim to talk to any hardware (HW=*),
	 * so we must find the device stack that talks to the specific
	 * hardware we're interested in.
	 * Check the node at the bottom of the device stack.
	 */
	myds = NULL;
	ScanList(list, ds, DeviceStack)
	{
		dsn = (DeviceStackNode *) LastNode(&ds->ds_Stack);
		if (dsn->dsn_IsHW && dsn->dsn_HWResource == hwID)
		{
			myds = ds;
			break;
		}
	}
	if (myds == NULL)
	{
		DBUG(("OpenSlotDevice: cannot find %s slot driver\n", slotname));
		DeleteDeviceStackList(list);
		return MAKEKERR(ER_SEVER,ER_C_STND,ER_NoHardware);
	}
	slotdev = OpenDeviceStack(myds);
	DeleteDeviceStackList(list);
	if (slotdev < 0)
		DBUG(("OpenSlotDevice: cannot open %s slot driver\n", slotname));
	return slotdev;
}

int32
SectorECC(uint8 *buffer)
{
	if (DipirRoutines->pdr_SectorECC == NULL)
		return 0;
	return (*DipirRoutines->pdr_SectorECC)(buffer);
}

int32
ChannelRead(HardwareID id, uint32 offset, uint32 len, void *buffer)
{
	return (*DipirRoutines->pdr_ReadChannel)(id, offset, len, buffer);
}

int32
ChannelMap(HardwareID id, uint32 offset, uint32 len, void **paddr)
{
	return (*DipirRoutines->pdr_MapChannel)(id, offset, len, *paddr);
}

int32
ChannelUnmap(HardwareID id, uint32 offset, uint32 len)
{
	return (*DipirRoutines->pdr_UnmapChannel)(id, offset, len);
}

int32
ChannelGetHWIcon(HardwareID id, void *buffer, uint32 bufLen)
{
	return (*DipirRoutines->pdr_GetHWIcon)(id, buffer, bufLen);
}

Item
internalOpenRomAppMedia(void)
{
	HardwareID romAppHW;
	List *list;
	DeviceStack *ds;
	DeviceStack *myds;
	DeviceStackNode *dsn;
	Err err;
	DDFQuery query[2];

	/* Find HardwareID of RomApp media. */
	err = SuperQuerySysInfo(SYSINFO_TAG_ROMAPPDEVICE,
				&romAppHW, sizeof(romAppHW));
	if (err < 0)
		return err;

	/* Find all device stacks that have the "romappmedia" attribute. */
	query[0].ddfq_Name = "romappmedia";
	query[0].ddfq_Operator = DDF_DEFINED;
	query[0].ddfq_Flags = 0;
	query[1].ddfq_Name = NULL;
	err = CreateDeviceStackList(&list, query);
	if (err < 0)
		return err;
	if (IsEmptyList(list))
	{
		DeleteDeviceStackList(list);
		return MAKEKERR(ER_SEVER,ER_C_STND,ER_NotFound);
	}

	/* Select the device stack that has the actual hardware at bottom. */
	myds = NULL;
	ScanList(list, ds, DeviceStack)
	{
		dsn = (DeviceStackNode *) LastNode(&ds->ds_Stack);
		if (dsn->dsn_IsHW && dsn->dsn_HWResource == romAppHW)
		{
			myds = ds;
			break;
		}
	}
	if (myds == NULL)
	{
		DeleteDeviceStackList(list);
		return MAKEKERR(ER_SEVER,ER_C_STND,ER_NotFound);
	}

	/* Open the device stack. */
	err = OpenDeviceStack(myds);
	DeleteDeviceStackList(list);
	return err;
}
