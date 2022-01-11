#ifndef __DSPP_REMAP_H
#define __DSPP_REMAP_H


/******************************************************************************
**
**  @(#) dspp_remap.h 95/08/07 1.4
**  $Id: dspp_remap.h,v 1.12 1995/03/16 21:56:08 peabody Exp phil $
**
**  DSPP absolute address remapping support (public).
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  950112 WJB  Created.
**  950119 WJB  Renamed dsppRemapHardwareReferences() to dsppRemapAbsoluteAddresses().
**  950125 WJB  Changed arg list for dsppRemapAbsoluteAddresses().
**  950130 WJB  Moved dsppRemapAbsoluteAddresses() proto to dspp.h.
**              Added dsppRemapAnvilAbsolutesToBulldog() proto.
**  950301 WJB  Added dspp_modes.h.
**  950310 WJB  Moved contents to dspp_remap_internal.h.
**  950310 WJB  Added dsppRemapAbsoluteAddresses() prototype.
**  950310 WJB  Added DSPP_REMAP_ENABLE and dsppInitAbsoluteRemapper().
**  950310 WJB  Removed dsppInitAbsoluteRemapper().
**  950314 WJB  Replaced DSPP_REMAP_ENABLE with DSPP_PROCABS_MODE.
**  950316 WJB  Set all DSPP_MODE_M2_ASIC builds to use DSPP_PROCABS_MODE_REMAP.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**
******************************************************************************/

#include <kernel/types.h>

#include "dspp.h"           /* DSPP structures */
#include "audio_folio_modes.h"


/* -------------------- Absolute Address Processing Mode */

/******************************************************************
**
**  Mode-specific behavior.
**
**  folio type                  dev     rom
**  -----------------------     ---     ---
**  opera native                off     off
**  opera->m2 compatibility     remap   remap
**  m2 native                   remap   remap
**
******************************************************************/

    /* Possible values for DSPP_PROCABS_MODE */
#define DSPP_PROCABS_MODE_OFF   0   /* No processing. Turns dsppRemapAbsoluteAddresses()
                                       into a macro that returns 0. */

#define DSPP_PROCABS_MODE_REMAP 1   /* Remap. dsppRemapAbsoluteAddresses() does in-place
                                       remapping of all remappable absolutes in template's
                                       code image. Specific handling of unremappable
                                       addresses is left up to the asic-specific code. */

#define DSPP_PROCABS_MODE_TRAP  2   /* Trap. dsppRemapAbsoluteAddresses() returns an
                                       error for any absolute address encountered in
                                       templates's code image. */


    /* default setting for DSPP_PROCABS_MODE if not set above */
#ifndef DSPP_PROCABS_MODE
    #define DSPP_PROCABS_MODE   DSPP_PROCABS_MODE_OFF
#endif


/* -------------------- Functions */

    /* remapping */
#if DSPP_PROCABS_MODE
  Err dsppRemapAbsoluteAddresses (DSPPTemplate *);
#else
  #define dsppRemapAbsoluteAddresses(dtmp) (0)
#endif


/*****************************************************************************/

#endif /* __DSPP_REMAP_H */
