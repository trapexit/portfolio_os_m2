/******************************************************************************
**
**  @(#) dspp_remap_bulldog.c 95/05/08 1.5
**  $Id: dspp_remap_bulldog.c,v 1.52 1995/03/18 01:23:24 peabody Exp phil $
**
**  DSPP remapping support for opera/anvil -> bulldog.
**  This module is a NOP unless AF_ASIC_M2 is defined in dspp.h.
**
**  By: Bill Barton
**
**  Copyright (c) 1994, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  941222 WJB  Created.
**  950106 WJB  Added basics for remapping.
**  950109 WJB  Preparing for remapping code walker.
**  950109 WJB  Added dsppRemapAnvilToBulldog().
**  950111 WJB  Moved DSPP instruction definitions to dspp_instructions.h.
**  950111 WJB  Cleaned up DSPP instruction definitions.
**  950111 WJB  Slight code cleanup.
**  950112 WJB  Publicized DSPP identification support stuff.
**  950112 WJB  Moved DSPP identification stuff to dspp_instructions.c.
**  950112 WJB  Renamed dsppRemapAnvilToBulldog() to dsppRemapHardwareReferences().
**  950113 WJB  Moved local DSPI_ defines to dspp_addresses.h.
**  950113 WJB  Moved disassembler to dspp_disasm.c.
**  950119 WJB  Renamed dsppRemapHardwareReferences() to dsppRemapAbsoluteAddresses().
**  950125 WJB  Revised dsppRemapAbsoluteAddresses() to avoid code words to be relocated.
**  950130 WJB  Renamed remapping function to dsppRemapAnvilAbsolutesToBulldog().
**  950131 WJB  Fixed some triple bangs.
**  950301 WJB  Added dspp_modes.h.
**  950303 WJB  Disabled EO_MEM remapping for now.
**  950310 WJB  Now using dspp_remap_internal.h.
**  950310 WJB  Made depend on DSPP_REMAP_ENABLE.
**              Added stub for dsppInitAbsoluteRemapper.
**  950310 WJB  Added demand-init call to dsppInitAbsoluteRemapper().
**  950310 WJB  Split single remapping table into separate global variables and hardware tables.
**              Added dynamic initialization of global variable remapping table.
**  950310 WJB  Added dsppTermAbsoluteRemapper().
**  950313 WJB  Added EO-Memory remapping.
**  950313 WJB  Slight cleanup and optimization in remapper.
**  950313 WJB  Added usage of AF_ERR_BAD_DSP_CODE and AF_ERR_BAD_DSP_ABSOLUTE.
**  950313 WJB  Commonized a bunch of stuff.
**  950314 WJB  Enabled gScratch remapping. Sorted global remapping table by address.
**  950314 WJB  Tweaked static variables names to fit naming convention.
**  950314 WJB  Replaced DSPP_REMAP_ENABLE with DSPP_PROCABS_MODE.
**  950315 WJB  Added dsppWhetherRemapTemplate().
**  950315 WJB  Added DSPPTemplate arg to dsppRemapAddress().
**  950316 WJB  Added REMAP_EOMem and REMAP_Safety local defines.
**  950317 WJB  Added DSPI_COMPAT_VAR_SAFETY.
**  950420 WJB  Renamed DSPI_ANVIL_EMUL_ things as DSPI_ things.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**
******************************************************************************/

#include <dspptouch/dspp_addresses.h>   /* DSPI_ */

#include "audio_folio_modes.h"      /* AF_ mode defines */
#include "audio_internal.h"         /* PRT() */
#include "dspp_remap_internal.h"    /* self */
#include "dspp_resources.h"         /* dsppImportResource() */


#if DSPP_PROCABS_MODE==DSPP_PROCABS_MODE_REMAP && defined(AF_ASIC_M2) /* { */

/* -------------------- Conditional */

    /* debugging */
#define DEBUG_Remap 1       /* !!! turn off before release */

    /* mode-specific behavior */
#ifdef AF_API_OPERA    /* Compatibility Mode */
    #define REMAP_EOMem     1           /* remap EO-Mem */
    #define REMAP_Safety    1           /* use "safe" memory address to deal w/ otherwise unremappable things */
#else                               /* Native Mode */
    #define REMAP_EOMem     0           /* don't remap EO-Mem, nowhere for it to go */
    #define REMAP_Safety    0           /* no safety net for unremappable things, return error instead */
#endif


/* -------------------- Remapping Table Support */

    /*
        @@@ Could have used uint16 here, but that might require bigger/slower
            code to deal w/, and it would mess up failure path handling of
            dsppImportResource()
    */
typedef struct DSPPRemapEntry {
    int32 drmp_FromAddr;
    int32 drmp_ToAddr;
} DSPPRemapEntry;

static const DSPPRemapEntry *dsppFindRemapEntry (const DSPPRemapEntry *remaptbl, uint32 remaplen, int32 fromaddr);


/* -------------------- Remapping Tables */

    /* Global Variables */
/*
    @@@ notes:
        . dspp_globalRemapTable[] and dspp_globalNames[] must have same order and length.
        . dspp_globalNames[] must match names defined in m2_head.dsp.
        . dspp_globalRemapTable[] drmp_ToAddr fields are filled out during runtime initialization.
*/
static DSPPRemapEntry dspp_globalRemapTable[] = {
    { DSPI_ANVIL_VAR_SCRATCH,       DRSC_ALLOC_INVALID },
    { DSPI_ANVIL_VAR_BENCH,         DRSC_ALLOC_INVALID },
    { DSPI_ANVIL_VAR_FRAME_COUNT,   DRSC_ALLOC_INVALID },
    { DSPI_ANVIL_VAR_MIX_LEFT,      DRSC_ALLOC_INVALID },
    { DSPI_ANVIL_VAR_MIX_RIGHT,     DRSC_ALLOC_INVALID },
};
#define DSPP_GLOBAL_REMAP_TABLE_LEN (sizeof dspp_globalRemapTable / sizeof dspp_globalRemapTable[0])

static const char *dspp_globalNames[DSPP_GLOBAL_REMAP_TABLE_LEN] = {
    "gScratch",
    "gBenchStart",
    "gFrameCount",
    "gMixLeft",
    "gMixRight",
};

    /* Hardware */
static const DSPPRemapEntry dspp_hardwareRemapTable[] = {
    { DSPI_ANVIL_NOISE, DSPI_NOISE },   /* used by all noise.dsp instruments */
    { DSPI_ANVIL_CLOCK, DSPI_CLOCK },   /* used by benchmark.dsp */
};
#define DSPP_HARDWARE_REMAP_TABLE_LEN (sizeof dspp_hardwareRemapTable / sizeof dspp_hardwareRemapTable[0])

    /* functions */
static Err dsppInitAbsoluteRemapper (void);
static void dsppTermAbsoluteRemapper (void);


/* -------------------- Init */

/******************************************************************
**
**  Initialize remapper. Requires resources to be
**  initialized and head.dsp, tail.dsp, etc. to be loaded
**  before this function is called.
**
**  Cleans up after itself on failure.
**
**  @@@ This function is called when dsppRemapAddress() is called
**      for the first time.
**
******************************************************************/

static Err dsppInitAbsoluteRemapper (void)
{
    Err errcode;
    int i;

  #if DEBUG_Remap
    PRT(("dsppInitAbsoluteRemapper() [Opera->M2]\n"));
  #endif

        /* Initialize global remap table using exported symbols */
    for (i=0; i<DSPP_GLOBAL_REMAP_TABLE_LEN; i++) {
        if ((errcode = dspp_globalRemapTable[i].drmp_ToAddr = dsppImportResource (dspp_globalNames[i])) < 0) goto clean;
    }

  #if DEBUG_Remap
    {
        int i;

        PRT(("  Globals:\n"));
        for (i=0; i<DSPP_GLOBAL_REMAP_TABLE_LEN; i++) {
            PRT(("    $%03lx -> $%03lx %s\n", dspp_globalRemapTable[i].drmp_FromAddr, dspp_globalRemapTable[i].drmp_ToAddr, dspp_globalNames[i]));
        }

        PRT(("  Hardware:\n"));
        for (i=0; i<DSPP_HARDWARE_REMAP_TABLE_LEN; i++) {
            PRT(("    $%03lx -> $%03lx\n", dspp_hardwareRemapTable[i].drmp_FromAddr, dspp_hardwareRemapTable[i].drmp_ToAddr));
        }

      #if REMAP_EOMem
        PRT(("  EO-Mem: $%03lx..$%03lx -> ", DSPI_ANVIL_EO_MEMORY_BASE, DSPI_ANVIL_EO_MEMORY_BASE + DSPI_ANVIL_EO_MEMORY_SIZE - 1));
        PRT(("$%03lx..$%03lx\n", DSPI_EO_MEMORY_BASE, DSPI_EO_MEMORY_BASE + DSPI_EO_MEMORY_SIZE - 1));
      #endif

      #if REMAP_Safety
        PRT(("  Safety: unremappable -> $%03lx\n", DSPI_COMPAT_VAR_SAFETY));
      #endif
    }
  #endif

    return 0;

clean:
    dsppTermAbsoluteRemapper();
    return errcode;
}


/******************************************************************
**
**  Terminate remapper. Cleans up any degree of success of
**  dsppInitAbsoluteRemapper() including never being called.
**  Can be called multiple times w/o harm.
**
**  @@@ At present, this function is only called when
**      dsppInitAbsoluteRemapper() fails.
**
******************************************************************/

static void dsppTermAbsoluteRemapper (void)
{
    int i;

  #if DEBUG_Remap
    PRT(("dsppTermAbsoluteRemapper() [Opera->M2]\n"));
  #endif

        /* Unimport all imported things in dspp_globalRemapTable[] */
    for (i=0; i<DSPP_GLOBAL_REMAP_TABLE_LEN; i++) {
        if (dspp_globalRemapTable[i].drmp_ToAddr >= 0) {
             dsppUnimportResource (dspp_globalNames[i]);
             dspp_globalRemapTable[i].drmp_ToAddr = DRSC_ALLOC_INVALID;
        }
    }
}


/* -------------------- Query */

/******************************************************************
**
**  Determine whether this non-priveleged template needs to be
**  remapped. Does this by checking the template's source ASIC
**  version against the folio's target ASIC version.
**
******************************************************************/

bool dsppWhetherRemapTemplate (const DSPPTemplate *dtmp)
{
    return dtmp->dtmp_Header.dhdr_SiliconVersion < DSPP_SILICON_BULLDOG;
}


/* -------------------- Lookup */

/******************************************************************
**
**  Given a 'fromaddr' return the correct 'toaddr', or error code
**  if no way to remap 'fromaddr'. Can use dtmp to figure out
**  the source ASIC version, which can be used to select the
**  address space for 'fromaddr'.
**
**  First call to this causes initialization.
**
******************************************************************/

int32 dsppRemapAddress (const DSPPTemplate *dtmp, int32 fromaddr)
{
    const DSPPRemapEntry *remapentry;

        /* first call initializes */
    {
        static bool inited;
        Err errcode;

        if (!inited) {
            if ((errcode = dsppInitAbsoluteRemapper()) < 0) return errcode;
            inited = TRUE;
        }
    }


        /* Do address remapping. These are ordered by frequency of use. */

        /* globals - directout.dsp, mixer*.dsp, etc. everyone uses this */
    if ((remapentry = dsppFindRemapEntry (dspp_globalRemapTable, DSPP_GLOBAL_REMAP_TABLE_LEN, fromaddr)) != NULL) return remapentry->drmp_ToAddr;

        /* hardware - only useful by *noise.dsp */
    if ((remapentry = dsppFindRemapEntry (dspp_hardwareRemapTable, DSPP_HARDWARE_REMAP_TABLE_LEN, fromaddr)) != NULL) return remapentry->drmp_ToAddr;

  #if REMAP_EOMem
        /* eo-mem - only useful for *duck*.dsp */
    if (fromaddr >= DSPI_ANVIL_EO_MEMORY_BASE && fromaddr <= DSPI_ANVIL_EO_MEMORY_BASE + DSPI_ANVIL_EO_MEMORY_SIZE - 1)
        return fromaddr - DSPI_ANVIL_EO_MEMORY_BASE + DSPI_EO_MEMORY_BASE;
  #endif

        /* Handle unremapped address */
  #if REMAP_Safety

        /* return safety address */
    ERR(("dsppRemapAddress[Opera->M2]: Unexpected DSP absolute address $%03lx remapped to safety address $%03lx.\n", fromaddr, DSPI_COMPAT_VAR_SAFETY));
    return DSPI_COMPAT_VAR_SAFETY;

  #else

        /* fail on any non-remapped address */
    ERR(("dsppRemapAddress[Opera->M2]: Unable to remap DSP address $%03lx.\n", fromaddr));
    return AF_ERR_BAD_DSP_ABSOLUTE;

  #endif
}


/* -------------------- Remapping Table Support */

/******************************************************************
**
**  Find DSPPRemapEntry in a table with matching drmp_FromAddr.
**
**  Returns pointer to DSPPRemapEntry on success, or NULL if not
**  found in table.
**
******************************************************************/

static const DSPPRemapEntry *dsppFindRemapEntry (const DSPPRemapEntry *remaptbl, uint32 remaplen, int32 fromaddr)
{
    for (; remaplen--; remaptbl++) {
        if (remaptbl->drmp_FromAddr == fromaddr) return remaptbl;
    }

    return NULL;
}

#endif   /* } DSPP_PROCABS_MODE==DSPP_PROCABS_MODE_REMAP && defined(AF_ASIC_M2) */
