/* @(#) memmaprec.c 96/08/16 1.8 */

#include <kernel/types.h>
#include <kernel/list.h>
#include <kernel/listmacros.h>
#include <kernel/mem.h>
#include <kernel/operror.h>
#include <kernel/io.h>
#include <kernel/driver.h>
#include <kernel/super.h>
#include <kernel/memmaprec.h>

#define	OVERLAP(mr1,mr2) \
	((mr1)->mr_Offset + (mr1)->mr_Size >= (mr2)->mr_Offset && \
	 (mr2)->mr_Offset + (mr2)->mr_Size >= (mr1)->mr_Offset)

/**
|||	AUTODOC -private -class Kernel -group Device-Drivers -name CreateMemMapRecord
|||	Creates a record of a memory-mapping.
|||
|||	  Synopsis
|||
|||	    Err CreateMemMapRecord(List *list, const IOReq *ior);
|||
|||	  Description
|||
|||	    This function creates a record of the memory-mapping
|||	    described by the IOReq and stores the record in the specified
|||	    list.
|||
|||	  Arguments
|||
|||	    list
|||	        A pointer to a list in which to store the record.
|||	    ior
|||	        A pointer to an IOReq which describes the mapping.
|||	        Normally, this IOReq would have its ioi_Command field
|||	        set to CMD_MAPRANGE(@).
|||
|||	  Return Value
|||
|||	    Returns 0, or a negative number for failure.
|||
|||	  Implementation
|||
|||	    Supervisor call implemented in Kernel folio V28.
|||
|||	  Associated Files
|||
|||	    <kernel/memmaprec.h> <kernel/io.h>
|||
|||	  See Also
|||
|||	    DeleteMemMapRecord(), DeleteAllMemMapRecords(), BytesMemMapped(),
|||	    CMD_MAPRANGE(@), CMD_UNMAPRANGE(@)
|||
**/

	Err
CreateMemMapRecord(List *list, const IOReq *ior)
{
	MemMapRecord *mr;
	MemMapRecord *newmr;
	MapRangeRequest *req;

	req = (MapRangeRequest *) ior->io_Info.ioi_Send.iob_Buffer;
	newmr = SuperAllocMem(sizeof(MemMapRecord), MEMTYPE_ANY);
	if (newmr == NULL)
		return NOMEM;
	newmr->mr_Offset = ior->io_Info.ioi_Offset;
	newmr->mr_Size = req->mrr_BytesToMap;
	newmr->mr_Flags = req->mrr_Flags;
	/*
	 * Make sure that if we overlap another mapping,
	 * both mappings are non-EXCLUSIVE.
	 */
	SCANLIST(list, mr, MemMapRecord)
	{
		if (!OVERLAP(mr, newmr))
			continue;
		if ((mr->mr_Flags & MM_EXCLUSIVE) ||
		    (newmr->mr_Flags & MM_EXCLUSIVE))
		{
			SuperFreeMem(newmr, sizeof(MemMapRecord));
			return NOTOWNER;
		}
	}
	ADDTAIL(list, (Node *)newmr);
	return 0;
}

/**
|||	AUTODOC -private -class Kernel -group Device-Drivers -name DeleteMemMapRecord
|||	Deletes a record of a memory-mapping.
|||
|||	  Synopsis
|||
|||	    Err DeleteMemMapRecord(List *list, const IOReq *ior);
|||
|||	  Description
|||
|||	    This function deletes a record of the memory-mapping
|||	    previously created by CreateMemMapRecord(), and removes
|||	    it from the list.
|||
|||	  Arguments
|||
|||	    list
|||	        A pointer to a list in which the record is stored.
|||	    ior
|||	        A pointer to an IOReq which describes the mapping
|||	        to be deleted.  Normally, this IOReq would have its
|||	        ioi_Command field set to CMD_UNMAPRANGE(@).
|||
|||	  Return Value
|||
|||	    Returns 0, or a negative number for failure.
|||
|||	  Implementation
|||
|||	    Supervisor call implemented in Kernel folio V28.
|||
|||	  Associated Files
|||
|||	    <kernel/memmaprec.h> <kernel/io.h>
|||
|||	  See Also
|||
|||	    CreateMemMapRecord(), DeleteAllMemMapRecords(),
|||	    CMD_MAPRANGE(@), CMD_UNMAPRANGE(@)
|||
**/

	int32
DeleteMemMapRecord(List *list, const IOReq *ior)
{
	MemMapRecord *mr;
	MapRangeRequest *req;

	req = (MapRangeRequest *) ior->io_Info.ioi_Send.iob_Buffer;
	SCANLIST(list, mr, MemMapRecord)
	{
		if (mr->mr_Offset == ior->io_Info.ioi_Offset &&
		    mr->mr_Size == req->mrr_BytesToMap)
		{
			REMOVENODE((Node*)mr);
			SuperFreeMem(mr, sizeof(MemMapRecord));
			return 0;
		}
	}
	return BADIOARG;
}

/**
|||	AUTODOC -private -class Kernel -group Device-Drivers -name DeleteAllMemMapRecords
|||	Deletes all records of a memory-mapping in a list.
|||
|||	  Synopsis
|||
|||	    Err DeleteAllMemMapRecords(List *list);
|||
|||	  Description
|||
|||	    This function deletes all memory-mapping records stored
|||	    in the specified list.
|||
|||	  Arguments
|||
|||	    list
|||	        A pointer to a list in which the records are stored.
|||
|||	  Return Value
|||
|||	    Returns 0, or a negative number for failure.
|||
|||	  Implementation
|||
|||	    Supervisor call implemented in Kernel folio V28.
|||
|||	  Associated Files
|||
|||	    <kernel/memmaprec.h> <kernel/io.h>
|||
|||	  See Also
|||
|||	    CreateMemMapRecord(), DeleteMemMapRecord(),
|||	    CMD_MAPRANGE(@), CMD_UNMAPRANGE(@)
|||
**/

	int32
DeleteAllMemMapRecords(List *list)
{
	MemMapRecord *mr;

	for (;;)
	{
		mr = (MemMapRecord *) FIRSTNODE(list);
		if (!ISNODE(list, mr))
			break;
		REMOVENODE((Node*)mr);
		SuperFreeMem(mr, sizeof(MemMapRecord));
	}
	return 0;
}

/**
|||	AUTODOC -private -class Kernel -group Device-Drivers -name BytesMemMapped
|||	Return total number of bytes recorded in a memory-mapping list.
|||
|||	  Synopsis
|||
|||	    int32 BytesMemMapped(List *list);
|||
|||	  Description
|||
|||	    This function scans the list of memory-mapping records stored
|||	    in the specified list and returns the total number of bytes
|||	    mapped by all the records in the list.
|||
|||	  Arguments
|||
|||	    list
|||	        A pointer to a list in which the records are stored.
|||
|||	  Return Value
|||
|||	    Returns number of bytes mapped, or a negative number for failure.
|||
|||	  Implementation
|||
|||	    Supervisor call implemented in Kernel folio V28.
|||
|||	  Associated Files
|||
|||	    <kernel/memmaprec.h> <kernel/io.h>
|||
|||	  See Also
|||
|||	    CreateMemMapRecord(), DeleteMemMapRecord(),
|||	    CMD_MAPRANGE(@), CMD_UNMAPRANGE(@)
|||
**/

	int32
BytesMemMapped(List *list)
{
	MemMapRecord *mr;
	int32 total;

	total = 0;
	SCANLIST(list, mr, MemMapRecord)
	{
		total += mr->mr_Size;
	}
	return total;
}

