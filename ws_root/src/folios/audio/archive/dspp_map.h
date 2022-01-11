#ifndef __DSPP_MAP_H
#define __DSPP_MAP_H


/******************************************************************************
**
**  @(#) dspp_map.h 95/05/09 1.1
**
**  DSPP memory map and static variables.
**
**  dspp_addresses.h contains physical hardware addresses. This file defines
**  many constants for the audio folio's usage of DSPP data memory.
**
**  By: Bill Barton
**
**  Copyright (c) 1995, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  950509 WJB  Created.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/

#include <dspptouch/dspp_addresses.h>       /* anvil memory segment sizes */


/* -------------------- Opera/Anvil DSPP global variables */

/*
    These are reserved data memory locations used by Opera/Anvil. These
    addresses were known to Opera/Anvil DSP instruments. The functions that
    these served has been moved to dynamically allocated locations in the M2
    audio folio. To support old instruments on M2, we remap these absolute
    addresses found in Opera/Anvil instrument templates to the dynamically
    allocated ones (see dspp_remap.h).

    These are defined for all versions of the audio folio because we intend to
    continue to support old instruments in this way, even in native M2 mode.
*/

    /* I-Mem */
/* #define DSPI_ANVIL_VAR_RESERVED0 (0x100) */  /* (not used by opera/anvil user instruments) */
/* #define DSPI_ANVIL_VAR_RESERVED1 (0x101) */  /* (not used by opera/anvil user instruments) */
#define DSPI_ANVIL_VAR_SCRATCH      (0x102)     /* scratch location used by many instruments */
/* #define DSPI_ANVIL_VAR_CLOCK     (0x103) */  /* (not used by opera/anvil user instruments) */
#define DSPI_ANVIL_VAR_BENCH        (0x104)     /* DSPP clock value at start of frame */
#define DSPI_ANVIL_VAR_FRAME_COUNT  (0x105)     /* DSPP frame count (i.e. incremented once per frame) */
#define DSPI_ANVIL_VAR_MIX_LEFT     (0x106)     /* output sound accumulator (left) - ultimately written to DACs */
#define DSPI_ANVIL_VAR_MIX_RIGHT    (0x107)     /* output sound accumulator (right) - ultimately written to DACs */

    /* EO-Mem */
/* #define DSPI_ANVIL_VAR_DUCK_SHIT (0x309) */  /* duck droppings found EO-mem (used by
                                                   adpcmduck22s.dsp, but since we remap all of
                                                   EO-Mem for compatibility mode (the only
                                                   mode in which adpcmduck22s.dsp needs to work)
                                                   we don't need to remap separately) */


/* -------------------- Opera/Anvil Special EO-Memory offsets */

/*
    These special EO memory offsets are known to Opera/Anvil DSPP instruments
    and Opera applications. DSPReadEO() (part of swiTestHack()) traps these and
    calls the correct function to emulate Opera behavior. Note: these are
    offsets w/i the EO memory space, not absolute data memory addresses.

    These are only needed for Opera title support.
*/

#ifdef AF_API_OPERA

    #define DSPP_EO_BENCHMARK       (0x000)     /* Mapped to dsphGetAudioCyclesUsed() */
    #define DSPP_EO_MAX_TICKS       (0x001)     /* Always returns 0 */
    #define DSPP_EO_FRAME_COUNT     (0x002)     /* Mapped to dsphGetAudioFrameCount() */
 /* #define DSPP_EO_DUCK_SHIT       (0x009) */  /* Only used by adpcmduck22s.dsp driver, since all of
                                                   eo-mem is remapped, no need to trap this one directly. */

#endif


/* -------------------- Emulation of Opera/Anvil data memory segments (M2 data memory map) */

/*
    In order to emulate Opera/Anvil resources correctly, we dice up M2 data
    memory into memory ranges that resemble Opera/M2. This is used for
    resource management and remapping.

    Also defined here is a special static variable used by the remapper
    in case an Opera instrument uses an absolute variable not supported directly.

    This is the only data memory map use for the M2 DSPP. Because native mode
    uses dynamic data memory allocations for everything, this memory map is
    only necessary for Opera support.
*/

#ifdef AF_API_OPERA /* { */

    #ifdef AF_ASIC_M2   /* { */

        #define DSPI_EI_MEMORY_BASE     (0x000)
        #define DSPI_EI_MEMORY_SIZE     DSPI_ANVIL_EI_MEMORY_SIZE

        #define DSPI_I_MEMORY_BASE      (0x100)
        #define DSPI_I_MEMORY_SIZE      DSPI_ANVIL_I_MEMORY_SIZE

        #define DSPI_EO_MEMORY_BASE     (0x200)
        #define DSPI_EO_MEMORY_SIZE     DSPI_ANVIL_EO_MEMORY_SIZE

        #define DSPI_COMPAT_VAR_SAFETY  (0x280) /* A safe memory location to which to remap
                                                   otherwise unremappable absolute addresses. */

    #endif  /* } AF_ASIC_M2 */

#endif /* } AF_API_OPERA */


/*****************************************************************************/

#endif  /* __DSPP_MAP_H */
