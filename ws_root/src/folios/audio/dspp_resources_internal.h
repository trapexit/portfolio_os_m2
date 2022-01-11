#ifndef __DSPP_RESOURCES_INTERNAL_H
#define __DSPP_RESOURCES_INTERNAL_H


/******************************************************************************
**
**  @(#) dspp_resources_internal.h 95/07/08 1.8
**  $Id: dspp_resources_internal.h,v 1.6 1995/03/09 00:03:18 peabody Exp phil $
**
**  DSPP resource manager's internal include file.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  950209 WJB  Created.
**  950213 WJB  Modified API for dsppAlloc/FreeResource().
**  950214 WJB  Removed return value from dsppFreeResource().
**  950303 WJB  Moved dsppAlloc/FreeTicks() into mode-dependent modules.
**  950307 WJB  Added rsrcalloc arg to dsppFreeTicks().
**  950308 WJB  Added dsppCalcMaxTicks() macro.
**  950412 WJB  Added dsppBindResource() prototype.
**  950414 WJB  Added dsppBindResource() macro for pre-AF_ASIC_M2.
**  950417 WJB  Added dsppValidateResourceBinding().
**  950614 WJB  Moved dsppAlloc/FreeResource() protos to dspp_resources.h.
**  950708 WJB  Moved dsppAlloc/FreeTicks() protos to dspp_resources.h
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**
******************************************************************************/

#include "dspp_resources.h"     /* public part */


/* -------------------- Functions */

    /* asic-specific binding (M2 and beyond) */
Err dsppValidateResourceBinding (const DSPPResource *drscreq, const DSPPResource *drscbindto);
int32 dsppBindResource (const DSPPResource *drscreq, const DSPPResource *drscbindto);

    /* max ticks computation */
    /* @@@ I suspect that the -1 is a safety margin */
#define dsppCalcMaxTicks(clockfreq,framerate) ((clockfreq) * DSPR_FRAMES_PER_BATCH / (framerate) - 1)


/*****************************************************************************/

#endif /* __DSPP_RESOURCES_INTERNAL_H */
