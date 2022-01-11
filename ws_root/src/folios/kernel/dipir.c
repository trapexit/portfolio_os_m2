/* @(#) dipir.c 96/08/06 1.14 */

/*#define DEBUG*/

#include <kernel/types.h>
#include <kernel/protlist.h>
#include <kernel/listmacros.h>
#include <kernel/item.h>
#include <kernel/semaphore.h>
#include <kernel/kernel.h>
#include <kernel/kernelnodes.h>
#include <kernel/io.h>
#include <kernel/task.h>
#include <kernel/super.h>
#include <kernel/mem.h>
#include <kernel/ddfnode.h>
#include <kernel/operror.h>
#include <stdio.h>
#include <string.h>
#include <kernel/internalf.h>
#include <kernel/dipir.h>
#include <file/filefunctions.h>
#include <misc/event.h>

#ifndef BUILD_STRINGS
#undef DEBUG
#endif

#ifdef DEBUG
#define DBUG(x) printf x
#else
#define DBUG(x)
#define PrintfSysErr(err)
#endif



/* Default Rescan function for drivers that don't supply their own. */
static bool defaultRescan(Device *dev)
{
    if (dev->dev_HWResource == 0)
	return 0;
    if (FindHWResource(dev->dev_HWResource, NULL) >= 0)
	return 0;
    DBUG(("Device %s (hw %x) is offline\n",
	dev->dev.n_Name, dev->dev_HWResource));
    dev->dev.n_Flags |= DEVF_OFFLINE;
    return 1;
}

/* The steps are given after this helper function. */
static void rescanAndRecurse(Driver* drv)
{
    DDFLink* link;
    Driver* parent;
    Device* dev;
    bool (*rescanFunction)(Device *dev);

    DBUG(("Rescanning device `%s'...\n", drv->drv.n_Name));
    if (drv->drv_Rescan == NULL)
	rescanFunction = defaultRescan;
    else
	rescanFunction = drv->drv_Rescan;
    /* Re-scan every device using this driver. */
    ScanList(&drv->drv_Devices, dev, Device)
    {
        if ((*rescanFunction)(dev) == 0)
	    continue;
	/* For each of its parents, re-scan them also. */
	if (drv->drv_DDF == NULL)
	    continue;
	ScanList(&drv->drv_DDF->ddf_Parents, link, DDFLink)
	{
	    parent = (Driver *)
		FindNamedNode(&KB_FIELD(kb_Drivers), link->ddfl_Link->ddf_n.n_Name);
	    if (parent)
		rescanAndRecurse(parent);
        }
    }
}

/* This must be called by CallBackSuper() since it hits hardware. */
void ReinitDevices(uint32 noFS)
{
    CallBackNode* cbn;
    Driver* drv;
    Err err;

    struct {
	EventFrameHeader     efh;
    } event;


    DBUG(("ReinitDevices:\n"));

    DBUG(("Duck!\n"));
    StartScanProtectedList(&KB_FIELD(kb_DuckFuncs), cbn, CallBackNode)
    {
	(*cbn->cb_code)(cbn->cb_param);
    }
    EndScanProtectedList(&KB_FIELD(kb_DuckFuncs));

	/* 
	 * SoftReset() to make Dipir rebuild the HWResource list.
	 */
    DBUG(("invoking SoftReset...\n"));
    SoftReset();

	/* 
	 * For each low-level device instance, tell it to rescan.  It will
	 * perform the following steps:
	 * 
	 * For each recognized HWResource in the HWResource list with a new
	 * InsertID, add a new unit.
	 * 
	 * For each unit whose InsertID no longer exists in the HWResource list,
	 * remove it.
	 */
    DBUG(("Rescanning devices...\n"));
    /* FIXME lock DevSemaphore */
    ScanList(&KB_FIELD(kb_Drivers), drv, Driver)
    {
	if(drv->drv_DDF && ddf_is_llvl(drv->drv_DDF))
	{
	    /* Tell the device to re-scan. */
	    /* WARNING:  devices may neither delete themselves or other */
	    /* devices nor create other devices in their rescan() functions. */
	    /* If they attempt to do so, semaphore deadlock will occur. */
	    rescanAndRecurse(drv);
	}
    }
    /* FIXME unlock DevSemaphore */

    if (!noFS)
    {
        /*
         * Recheck all mounted filesystems to make sure their underlying
         * device stacks are still active.
         */
        RecheckAllFileSystems();

	/* 
	 * Kick the filesystem into mounting and scanning for DDFs
	 */
        PlugNPlay();
    }

    DBUG(("Missed me!\n"));
    StartScanProtectedList(&KB_FIELD(kb_RecoverFuncs), cbn, CallBackNode)
    {
	(*cbn->cb_code)(cbn->cb_param);
    }
    EndScanProtectedList(&KB_FIELD(kb_RecoverFuncs));

    /*
     * Send DeviceChanged event.
     */
    memset(&event, 0, sizeof(event));
    event.efh.ef_ByteCount = sizeof(event);
    event.efh.ef_EventNumber = EVENTNUM_DeviceChanged;
    err = SuperReportEvent(&event);
#ifdef BUILD_STRINGS
    if (err < 0)
        printf("Cannot send DeviceChanged event, err %x\n", err);
#else
    TOUCH(err);
#endif

    DBUG(("Done rescanning.\n"));
}

/**
|||	AUTODOC -private -class Kernel -group Configuration -name TriggerDeviceRescan
|||	Trigger a hardware re-scan.
|||
|||	  Synopsis
|||
|||	    void TriggerDeviceRescan(void);
|||
|||	  Description
|||
|||	    This is a supervisor-only call.
|||
|||	    This function sends a signal to the Operator task to perform
|||	    processing to handle system hardware configuration changes.
|||	    This processing includes calling all registered "Duck"
|||	    functions, invoking soft reset to invoke Dipir, calling all
|||	    low-level device re-scan functions, handling FS changes, and
|||	    finally calling all registered "Recover" functions.
|||
|||	    This function does not perform the re-scanning but merely
|||	    signals the Operator task to do so.  Thus, this function may
|||	    be called in an interrupt context where performing the
|||	    re-scanning process is desired but prohibited.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V28.
|||
|||	  Associated Files
|||
|||	    <kernel/super.h>
|||
|||	  See Also
|||
|||	    RegisterDuck(), RegisterRecover(),
|||	    UnregisterDuck(), UnregisterRecover()
|||
**/

void TriggerDeviceRescan(void)
{
    /* If the operator is ready, just signal him to do the rescan.
     * Otherwise, do it ourselves. */
    if (KB_FIELD(kb_Flags) & KB_OPERATOR_UP)
        externalSignal(KB_FIELD(kb_OperatorTask), RESCAN_SIGNAL);
    else
	ReinitDevices(1);
}

static Err regfunc(void (*code) (), uint32 param, ProtectedList* l)
{
    CallBackNode* cbn;

    cbn= SuperAllocMem(sizeof(CallBackNode), MEMTYPE_ANY);
    if(!cbn) return NOMEM;
    memset(cbn, 0, sizeof(CallBackNode));
    cbn->cb_code= code;
    cbn->cb_param= param;
    ModifyProtectedListWith(AddTail((List*)l, (Node*)cbn), l);
    return 0;
}

/**
|||	AUTODOC -private -class Kernel -group Configuration -name RegisterDuck
|||	Register a "Duck" function.
|||
|||	  Synopsis
|||
|||	    Err RegisterDuck(void (*code) (), uint32 param);
|||
|||	  Description
|||
|||	    This is a super vector, callable only by privileged tasks,
|||	    that may be called in user mode.  This function registers
|||	    a "Duck" function that gets called just before the Operator
|||	    invokes soft reset during system hardware configuration
|||	    change processing.
|||
|||	  Arguments
|||
|||	    code
|||	        The pointer to the function, which must take a single
|||	        32-bit argument, to be invoked.
|||
|||	    param
|||	        The single argument to the function when invoked.
|||
|||	  Return Value
|||
|||	    Returns zero or a negative error code for failure.  Possible
|||	    error codes currently include:
|||
|||	    NOMEM
|||	        There was not enough memory to complete the operation.
|||
|||	    BADPRIV
|||	        The calling task is not privileged.
|||
|||	  Notes
|||
|||	    A "Duck" function may not change the "Duck" function list by
|||	    registering or unregistering itself or any other "Duck" function.
|||	    It may change the "Recover" function list.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V28.
|||
|||	  Associated Files
|||
|||	    <kernel/super.h>
|||
|||	  See Also
|||
|||	    TriggerDeviceRescan(), RegisterRecover(),
|||	    UnregisterDuck(), UnregisterRecover()
|||
**/

Err RegisterDuck(void (*code) (), uint32 param)
{
    if(!(CURRENTTASK->t.n_ItemFlags & ITEMNODE_PRIVILEGED)) return BADPRIV;
    return IsSuper()
	? regfunc(code, param, &KB_FIELD(kb_DuckFuncs))
	: CallBackSuper(regfunc, (uint32)code, param, (uint32)&KB_FIELD(kb_DuckFuncs));
}

/**
|||	AUTODOC -private -class Kernel -group Configuration -name RegisterRecover
|||	Register a "Recover" function.
|||
|||	  Synopsis
|||
|||	    Err RegisterRecover(void (*code) (), uint32 param);
|||
|||	  Description
|||
|||	    This is a super vector, callable only by privileged tasks,
|||	    that may be called in user mode.  This function registers
|||	    a "Recover" function that gets called just before the Operator
|||	    finishes system hardware configuration change processing.
|||
|||	  Arguments
|||
|||	    code
|||	        The pointer to the function, which must take a single
|||	        32-bit argument, to be invoked.
|||
|||	    param
|||	        The single argument to the function when invoked.
|||
|||	  Return Value
|||
|||	    Returns zero or a negative error code for failure.  Possible
|||	    error codes currently include:
|||
|||	    NOMEM
|||	        There was not enough memory to complete the operation.
|||
|||	    BADPRIV
|||	        The calling task is not privileged.
|||
|||	  Notes
|||
|||	    A "Recover" function may not change the "Recover" function list
|||	    by registering or unregistering itself or any other "Recover"
|||	    function.  It may change the "Duck" function list.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V28.
|||
|||	  Associated Files
|||
|||	    <kernel/super.h>
|||
|||	  See Also
|||
|||	    TriggerDeviceRescan(), RegisterDuck(),
|||	    UnregisterDuck(), UnregisterRecover()
|||
**/

Err RegisterRecover(void (*code) (), uint32 param)
{
    if(!(CURRENTTASK->t.n_ItemFlags & ITEMNODE_PRIVILEGED)) return BADPRIV;
    return IsSuper()
	? regfunc(code, param, &KB_FIELD(kb_RecoverFuncs))
	: CallBackSuper(regfunc, (uint32)code, param, (uint32)&KB_FIELD(kb_RecoverFuncs));
}

static Err unregfunc(void (*code) (), ProtectedList* l)
{
    CallBackNode* cbn;
    Err ret= MakeKErr(ER_WARN, ER_C_STND, ER_NotFound);

    ModifyProtectedListWith(
	ScanList(l, cbn, CallBackNode)
	    {
		if(cbn->cb_code == code)
		{
		    RemNode((Node*)cbn);
		    SuperFreeMem(cbn, sizeof(CallBackNode));
		    ret= 0;
		    break;
		}
	    },
	l);
    return ret;
}

/**
|||	AUTODOC -private -class Kernel -group Configuration -name UnregisterDuck
|||	Un-register a "Duck" function.
|||
|||	  Synopsis
|||
|||	    Err UnregisterDuck(void (*code) ());
|||
|||	  Description
|||
|||	    This is a super vector, callable only by privileged tasks,
|||	    that may be called in user mode.  This function removes the
|||	    "Duck" function from the "Duck" function list.
|||
|||	  Arguments
|||
|||	    code
|||	        The pointer to the function to be removed.
|||
|||	  Return Value
|||
|||	    Returns zero or a negative error code for failure.  Possible
|||	    error codes currently include:
|||
|||	    ER_NotFound
|||	        The function was not found in the "Duck" function list.
|||
|||	    BADPRIV
|||	        The calling task is not privileged.
|||
|||	  Notes
|||
|||	    A "Duck" function may not change the "Duck" function list by
|||	    registering or unregistering itself or any other "Duck" function.
|||	    It may change the "Recover" function list.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V28.
|||
|||	  Associated Files
|||
|||	    <kernel/super.h>
|||
|||	  See Also
|||
|||	    TriggerDeviceRescan(), RegisterDuck(),
|||	    RegisterRecover(), UnregisterRecover()
|||
**/

Err UnregisterDuck(void (*code) ())
{
    if(!(CURRENTTASK->t.n_ItemFlags & ITEMNODE_PRIVILEGED)) return BADPRIV;
    return IsSuper()
	? unregfunc(code, &KB_FIELD(kb_DuckFuncs))
	: CallBackSuper(unregfunc, (uint32)code, (uint32)&KB_FIELD(kb_DuckFuncs), 0);
}

/**
|||	AUTODOC -private -class Kernel -group Configuration -name UnregisterRecover
|||	Un-register a "Recover" function.
|||
|||	  Synopsis
|||
|||	    Err UnregisterRecover(void (*code) ());
|||
|||	  Description
|||
|||	    This is a super vector, callable only by privileged tasks,
|||	    that may be called in user mode.  This function removes the
|||	    "Recover" function from the "Recover" function list.
|||
|||	  Arguments
|||
|||	    code
|||	        The pointer to the function to be removed.
|||
|||	  Return Value
|||
|||	    Returns zero or a negative error code for failure.  Possible
|||	    error codes currently include:
|||
|||	    ER_NotFound
|||	        The function was not found in the "Recover" function list.
|||
|||	    BADPRIV
|||	        The calling task is not privileged.
|||
|||	  Notes
|||
|||	    A "Recover" function may not change the "Recover" function list
|||	    by registering or unregistering itself or any other "Recover"
|||	    function.  It may change the "Duck" function list.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V28.
|||
|||	  Associated Files
|||
|||	    <kernel/super.h>
|||
|||	  See Also
|||
|||	    TriggerDeviceRescan(), RegisterDuck(),
|||	    RegisterRecover(), UnregisterDuck()
|||
**/

Err UnregisterRecover(void (*code) ())
{
    if(!(CURRENTTASK->t.n_ItemFlags & ITEMNODE_PRIVILEGED)) return BADPRIV;
    return IsSuper()
	? unregfunc(code, &KB_FIELD(kb_RecoverFuncs))
	: CallBackSuper(regfunc, (uint32)code, (uint32)&KB_FIELD(kb_RecoverFuncs), 0);
}


