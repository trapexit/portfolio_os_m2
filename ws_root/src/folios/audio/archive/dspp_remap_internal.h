#ifndef __DSPP_REMAP_INTERNAL_H
#define __DSPP_REMAP_INTERNAL_H


/******************************************************************************
**
**  @(#) dspp_remap_internal.h 95/05/09 1.3
**  $Id: dspp_remap_internal.h,v 1.6 1995/03/15 23:23:38 peabody Exp phil $
**
**  Hardware remapping support (internal). Internal to dspp_remap.c and its
**  kin. No other files should include this.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  950310 WJB  Moved contents of dspp_remap.h to here.
**  950313 WJB  Commonized a bunch of stuff.
**  950314 WJB  Replaced DSPP_REMAP_ENABLE with DSPP_PROCABS_MODE.
**  950315 WJB  Added dsppWhetherRemapTemplate().
**  950315 WJB  Added DSPPTemplate arg to dsppRemapAddress().
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**
******************************************************************************/

#include <kernel/types.h>

#include "dspp_map.h"       /* DSPI_ANVIL_VAR_, anvil memory segment emulation (convenience) */
#include "dspp_remap.h"     /* public version of self (convenience) */


/* -------------------- Functions */

#if DSPP_PROCABS_MODE == DSPP_PROCABS_MODE_REMAP

    /* target asic-dependent functions */
bool dsppWhetherRemapTemplate (const DSPPTemplate *);
int32 dsppRemapAddress (const DSPPTemplate *, int32 fromaddr);

#endif


/*****************************************************************************/

#endif /* __DSPP_REMAP_INTERNAL_H */
