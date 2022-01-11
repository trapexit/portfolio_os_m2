/* @(#) openNamedDeviceStack.c 96/02/23 1.7 */

#include <kernel/types.h>
#include <varargs.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/mem.h>
#include <kernel/operror.h>
#include <kernel/list.h>
#include <kernel/device.h>
#include <kernel/ddfnode.h>
#include <kernel/devicecmd.h>
#include <dipir/hwresource.h>
#include <file/filefunctions.h>

#define	MAX_DEVICE_NAME	32

#define	DBUG(x) /* printf x */

/**
|||	AUTODOC -private -class Kernel -group Devices -name OpenNamedDeviceStack
|||	Opens a device stack by name.
|||
|||	  Synopsis
|||
|||	    Item OpenNamedDeviceStack(const char *stackName);
|||
|||	  Description
|||
|||	    This function constructs opens a device stack and returns
|||	    the item number of the open stack.  The stack is named as
|||	    a sequence of one or more device names, separated by commas,
|||	    and optionally followed by an integer which is interpreted
|||	    as a HWResource identifier.  If the stack name does not end 
|||	    with a HWResource identifier, the system attempts to find
|||	    the unique HWResource which can be put at the bottom of the
|||	    stack.  An error is returned if none is found, or if more
|||	    than one are found.
|||
|||	    The returned item number is equivalent to the item number
|||	    returned from OpenDeviceStack(), and can be used in a
|||	    similar manner.  For example, it may be passed to
|||	    CreateIOReq(), CloseDeviceStack(), etc.
|||
|||	  Arguments
|||
|||	    stackName
|||	        The name of the device stack to be opened.
|||
|||	  Return Value
|||
|||	    Returns the item number of the open device stack, or a 
|||	    negative error code for failure.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libc.a V28.
|||
|||	  Associated Files
|||
|||	    <kernel/device.h>, libc.a
|||
|||	  Note
|||
|||	    OpenNamedDeviceStack should normally not be used.
|||	    CreateDeviceStackList() and OpenDeviceStack() are the
|||	    normal functions used to open a device stack.
|||
|||	  See Also
|||
|||	    CreateDeviceStackList(), OpenDeviceStack(), CloseDeviceStack()
|||
**/

static Err
FixDeviceStackBottom(DeviceStack *ds)
{
	DeviceStackNode *dsn;
	HardwareID hwID;
	HWResource hwr;

	dsn = (DeviceStackNode *) LastNode(&ds->ds_Stack);
	if (dsn->dsn_IsHW)
	{
		/* Stack already has HWResource at the bottom. */
		return 0;
	}

	if (!ddf_is_llvl(dsn->dsn_DDFNode))
	{
		/* Bottom DDF is not low-level; don't need HW. */
		return 0;
	}

	/* Find a HWResource that the bottom DDF can manage. */
	DBUG(("ONDS: find HW for bottom\n"));
	hwID = 0;
	for (hwr.hwr_InsertID = 0;  NextHWResource(&hwr) >= 0; )
	{
		if (DDFCanManage(dsn->dsn_DDFNode, hwr.hwr_Name))
		{
			if (hwID != 0)
			{
				/* DDF can manage more than one HW. */
				DBUG(("ONDS: more than one HW\n"));
				return MakeKErr(ER_SEVERE,ER_C_STND,ER_ParamError);
			}
			hwID = hwr.hwr_InsertID;
		}
	}
	if (hwID == 0)
	{
		/* Didn't find any HWResource. */
		DBUG(("ONDS: no HW\n"));
		return MakeKErr(ER_SEVERE,ER_C_STND,ER_NoHardware);
	}
	DBUG(("ONDS: found HW %x\n", hwID));
	return AppendDeviceStackHW(ds, hwID);
}


Item
funcNamedDeviceStack(const char *stackName, Item (*func) (const DeviceStack *))
{
	DeviceStack *ds;
	DDFNode *ddf;
	HardwareID hwID;
	const char *s;
	const char *es;
	uint32 len;
	char deviceName[MAX_DEVICE_NAME];
	Err err;

	ds = CreateDeviceStack();
	if (ds == NULL)
		return NOMEM;
	for (s = stackName;  *s != '\0'; )
	{
		/* Get the next device name from the stackname. */
		for (es = s;  *es != '\0' && *es != ',';  es++)
			continue;
		len = es - s;
		strncpy(deviceName, s, len);
		deviceName[len] = '\0';
		s = es;
		if (*s == ',') s++;

		/* See if the device name is really a HWResource ID. */
		DBUG(("ONDS: check dev component <%s>\n", deviceName));
		hwID = strtol(deviceName, &es, 0);
		if (es - deviceName != len)
		{
			/* It's a device name: find the DDF. */
			ddf = FindDDF(deviceName);
			DBUG(("ONDS: ddf %x\n", ddf));
			if (ddf == NULL)
			{
				err = MakeKErr(ER_SEVERE,ER_C_STND,ER_NotFound);
				goto Error;
			}
			err = AppendDeviceStackDDF(ds, ddf);
			if (err < 0)
				goto Error;
		} else
		{
			/* It's a HWResource ID. */
			DBUG(("ONDS: HW %x\n", hwID));
			if (IsEmptyList(&ds->ds_Stack))
			{
				DBUG(("ONDS: no drivers on stack!\n"));
				err = MakeKErr(ER_SEVERE,ER_C_STND,ER_ParamError);
				goto Error;
			}
			err = AppendDeviceStackHW(ds, hwID);
			if (err < 0)
				goto Error;
		}
	}

	if (IsEmptyList(&ds->ds_Stack))
	{
		DBUG(("ONDS: empty stack\n"));
		err = MakeKErr(ER_SEVERE,ER_C_STND,ER_ParamError);
		goto Error;
	}
	/* See if we need to append a HWResource at bottom of stack. */
	err = FixDeviceStackBottom(ds);
	if (err < 0)
		return err;

	/* Now open it. */
	err = func(ds);
	DBUG(("ONDS: OpenDeviceStack ret %x\n", err));
		
Error:
	DeleteDeviceStack(ds);
	DBUG(("ONDS: Error %x\n", err));
	return err;
}


Item
OpenNamedDeviceStack(const char *stackName)
{
	return(funcNamedDeviceStack(stackName, OpenDeviceStack));
}


Item
NamedDeviceStackMounted(const char *stackName)
{
	return(funcNamedDeviceStack(stackName, FileSystemMountedOn));
}
