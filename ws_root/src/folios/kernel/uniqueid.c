/* @(#) uniqueid.c 96/06/26 1.16 */

#include <kernel/types.h>
#include <kernel/kernel.h>
#include <kernel/uniqueid.h>
#include <stdio.h>

/**
|||	AUTODOC -class Kernel -group Miscellaneous -name ReadUniqueID
|||	Gets the system unique id.
|||
|||	  Synopsis
|||
|||	    Err ReadUniqueID(UniqueID *id);
|||
|||	  Description
|||
|||	    3DO systems may be equipped with a unique ID chip that uniquely
|||	    identifies each machine. This function lets you read the value of
|||	    this ID chip.
|||
|||	    The unique ID code is mainly useful to networked applications. It
|||	    may also be used to determine whether a saved game is being used on
|||	    the same machine it was originally created.
|||
|||	  Arguments
|||
|||	    id
|||	        Pointer to a UniqueID structure where the system's ID is
|||	        stored.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||	    Possible error codes currently include:
|||
|||	    NOHARDWARE
|||	        The system doesn't have a unique ID code.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V30.
|||
|||	  Associated Files
|||
|||	    <kernel/uniqueid.h>
|||
**/

Err ReadUniqueID(UniqueID *id)
{
    if (!id)
	return BADPTR;

    if ((KB_FIELD(kb_Flags) & KB_UNIQUEID) == 0)
        return NOHARDWARE;

    id->u_upper = KB_FIELD(kb_UniqueID)[0];
    id->u_lower = KB_FIELD(kb_UniqueID)[1];
    return 0;
}
