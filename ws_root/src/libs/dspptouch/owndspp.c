/******************************************************************************
**
**  @(#) owndspp.c 96/06/18 1.4
**
**  DSPP exclusive access control.
**
**  By: Bill Barton
**
**  Copyright (c) 1996, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  960603 WJB  Created.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/

#include <dspptouch/dspp_touch.h>   /* self */
#include <kernel/semaphore.h>
#include <loader/loader3do.h>       /* FindCurrentModule() */

#include "dspptouch_internal.h"

#define OWNDSPP_SEMAPHORE_NAME  "OwnDSPP"


/******************************************************************/
/**
|||	AUTODOC -private -class dspptouch -name OwnDSPP
|||	Grants exclusive access to DSPP.
|||
|||	  Synopsis
|||
|||	    Item OwnDSPP (Err busyErrCode)
|||
|||	  Description
|||
|||	    Grants exclusive access to the DSPP. This is used to arbitrate between
|||	    audio and beep folios.
|||
|||	    This currently operates by creating a unique semaphore. If this is
|||	    successful, exclusive access to the DSPP is granted to the caller. The
|||	    caller is responsible for disowning the DSPP when through with it.
|||
|||	    When built with BUILD_STRINGS defined, if someone else already owns the DSPP,
|||	    this function also prints an error message indicating who the current owner
|||	    is.
|||
|||	  Arguments
|||
|||	    busyErrCode
|||	        Err code to be returned when the exclusive access already belongs to
|||	        someone else.
|||
|||	  Return Value
|||
|||	    On success, returns non-negative Item number of the lock granted to the
|||	    caller. Pass this item to DisownDSPP() to relinquish exclusive access to the
|||	    DSPP. On failure, returns a negative error code. Returns busyErrCode when
|||	    the DSPP is already owned by another module.
|||
|||	  Implementation
|||
|||	    Library call implemented in libdspptouch.a V32.
|||
|||	  Associated Files
|||
|||	    <dspptouch/dspp_touch.h>
|||
|||	  See Also
|||
|||	    DisownDSPP()
**/

Item OwnDSPP (Err busyErrCode)
{
    Item result;

        /* create unique semaphore. Store our module item number in the
        ** semaphore's user data to help determine the owner should anyone else
        ** try to call OwnDSPP(). */
    result = CreateItemVA (MKNODEID(KERNELNODE,SEMA4NODE),
        TAG_ITEM_NAME,                OWNDSPP_SEMAPHORE_NAME,
        TAG_ITEM_UNIQUE_NAME,         TRUE,
        CREATESEMAPHORE_TAG_USERDATA, FindCurrentModule(),
        TAG_END);

        /* if a semaphore by this name already exists, translate error code */
    if (result == UNIQUEITEMEXISTS) {

        #if BUILD_STRINGS
        {
            const Semaphore * const otherSem = (Semaphore *)LookupItem (FindSemaphore (OWNDSPP_SEMAPHORE_NAME));
            const ItemNode * const otherOwnerModule = otherSem ? CheckItem ((Item)otherSem->sem_UserData, KERNELNODE, MODULENODE) : NULL;

            if (otherOwnerModule) {
                ERR(("OwnDSPP: DSPP already owned by module 0x%x ('%s').\n", otherOwnerModule->n_Item, otherOwnerModule->n_Name));
            }
            else {
                ERR(("OwnDSPP: DSPP already owned.\n"));
            }
        }
        #endif

        /* !!! attempt to expunge current owner and try again */

        return busyErrCode;
    }

    return result;
}


/******************************************************************/
/**
|||	AUTODOC -private -class dspptouch -name DisownDSPP
|||	Relinquishes exclusive access to DSPP.
|||
|||	  Synopsis
|||
|||	    void DisownDSPP (Item dsppLock)
|||
|||	  Description
|||
|||	    Relinquishes exclusive access to the DSPP granted by OwnDSPP().
|||
|||	  Arguments
|||
|||	    dsppLock
|||	        Item number returned by OwnDSPP().
|||
|||	  Implementation
|||
|||	    Macro implemented in <dspptouch/dspp_touch.h> V32.
|||
|||	  Associated Files
|||
|||	    <dspptouch/dspp_touch.h>
|||
|||	  See Also
|||
|||	    OwnDSPP()
**/
