/******************************************************************************
**
**  @(#) dspp_resources_bulldog.c 96/10/16 1.36
**  $Id: dspp_resources_bulldog.c,v 1.18 1995/03/18 02:19:55 peabody Exp phil $
**
**  DSPP resource manager - Bulldog version.
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
**  950209 WJB  Created.
**  950223 WJB  Hacked in an opera-like allocator to assist in getting simulator working.
**  950301 WJB  Added dspp_modes.h.
**  950303 WJB  Added more realistic M2 resource allocator.
**  950303 WJB  Added stubs for dsppAlloc/FreeTicks().
**  950306 WJB  Added RBASE8 allocation.
**  950307 WJB  Added rsrcalloc arg to dsppFreeTicks().
**  950307 WJB  Added M2 tick allocator.
**  950308 WJB  Added local tick allocator.
**  950317 WJB  Added dsppGrabSystemReservedResources() stub.
**  950411 WJB  Added support for DRSC_TYPE_RBASE.
**  950412 WJB  Added dsppBindResource() stub.
**  950413 WJB  Added support for DRSC_FIFO_OSC in dsppGetResourceAttribute().
**              Implemented dsppBindResource().
**  950417 WJB  Added dsppValidateResourceBinding().
**  950501 WJB  Added DRSC_TRIGGER support. Tweaked comments.
**  950501 WJB  Added special DRSC_TRIGGER handling to dsppGetResourceAttribute().
**  950502 WJB  Moved special DRSC_TRIGGER handling out of dsppGetResourceAttribute().
**  950508 WJB  Added usage of DSPR_NUM_TRIGGERS.
**  950601 WJB  Adapted to bit-array version of table allocator.
**  950614 WJB  Added dsppAllocResourceHere().
**  950626 PLB  Add Attribute to Allocated for memory arrays in dsppGetResourceAttribute().
**  950628 PLB  dsppAllocTicks() and dsppFreeTicks() take RateShift instead of dins
**  950809 WJB  Tidied up after resource type changes.
**  951211 PLB  Removed AF_ASIC_M2 and AF_API_M2 references.
**  951222 WJB  Detect BDA 1.1, set trigger and FIFO limits lower to BDA 1.1 limits.
**  960105 WJB  Added GetAudioResourceInfo().
**  961006 PLB  Use system clock rate from ROM for tick calculation.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/

#include <dspptouch/dspp_instructions.h>    /* DSPN_ */

#include "audio_folio_modes.h"          /* AF_ mode defines */
#include "audio_internal.h"             /* PRT() */
#include "dspp_resources_internal.h"    /* self */
#include <kernel/sysinfo.h>             /* SYSINFO_TAG_SYSTEM */
#include "table_alloc.h"


/* -------------------- Debugging */

#define DEBUG_Resource  0
#define DEBUG_Allocator 0
#define DEBUG_Tick      0
#define DEBUG_Bind      0               /* debug binding */

#if DEBUG_Resource
  #undef ERRDBUG
  #define ERRDBUG(x)   PRT(x)
#endif

#if DEBUG_Allocator
  #include "table_alloc_debug.h"        /* DumpTableAllocator() */
#endif


/* -------------------- Allocator tables */

    /* Memory, FIFO, and Trigger Resource tables */
static TableAllocData dspp_CodeMemoryTable [TableAllocDataSize(DSPI_CODE_MEMORY_SIZE)];
static TableAllocData dspp_DataMemoryTable [TableAllocDataSize(DSPI_DATA_MEMORY_SIZE)];
static TableAllocData dspp_FIFOTable       [TableAllocDataSize(DSPI_MAX_DMA_CHANNELS)];
static TableAllocData dspp_TriggerTable    [TableAllocDataSize(DSPR_MAX_TRIGGERS)];

                                                /* Size                   Offset                 Table */
static TableAllocator dspp_CodeMemoryAllocator = { DSPI_CODE_MEMORY_SIZE, DSPI_CODE_MEMORY_BASE, dspp_CodeMemoryTable };
static TableAllocator dspp_DataMemoryAllocator = { DSPI_DATA_MEMORY_SIZE, DSPI_DATA_MEMORY_BASE, dspp_DataMemoryTable };
static TableAllocator dspp_FIFOAllocator       = { DSPI_MAX_DMA_CHANNELS, 0,                     dspp_FIFOTable };
static TableAllocator dspp_TriggerAllocator    = { DSPR_MAX_TRIGGERS,     0,                     dspp_TriggerTable };

    /* Tick resource */
static struct {
    int32 dtic_MaxTicks;        /* Maximum ticks / batch */
    int32 dtic_AvailTicks;      /* Currently available ticks / batch */
} dspp_TickAllocator;


/* -------------------- Local functions */

static TableAllocator *dsppGetResourceAllocator (int32 rsrctype);


/* -------------------- Init */

/******************************************************************
**
**  Initialize DSPP resource allocation during audio folio init.
**  Called by DSPP_Init().
**
**  @@@ Can only be called once. Takes advantage of initial state
**      of TableAllocators.
**
**  @@@ depends on gIfBDA_1_1 being set prior to this function being
**      called.
**
**  Note: this is called directly by clients - there is one of these
**  for each AF_API_* value.
**
******************************************************************/

void dsppInitResources (void)
{
  #if DEBUG_Resource
    PRT(("dsppInitResources() [M2]\n"));
  #endif

  #if DEBUG_Resource
    PRT(("  code: size=$%04lx offset=$%04lx\n", dspp_CodeMemoryAllocator.tall_Size, dspp_CodeMemoryAllocator.tall_Base));
    PRT(("  data: size=$%04lx offset=$%04lx\n", dspp_DataMemoryAllocator.tall_Size, dspp_DataMemoryAllocator.tall_Base));
    PRT(("  fifo: size=$%04lx offset=$%04lx\n", dspp_FIFOAllocator.tall_Size,       dspp_FIFOAllocator.tall_Base));
    PRT(("  trig: size=$%04lx offset=$%04lx\n", dspp_TriggerAllocator.tall_Size,    dspp_TriggerAllocator.tall_Base));
  #endif

        /* Init Tick Resource */
    dsppAdjustAvailTicks (MAX_DEFAULT_SAMPLERATE);      /* @@@ perhaps should use dspp_SampleRate? (adds order dependency, though) */
}


/* -------------------- Low level resource Alloc/Free for M2 */

/******************************************************************
**
**  Allocate a DSPP resource - M2 version.
**
**  Supports:
**      DRSC_TYPE_CODE
**      DRSC_TYPE_KNOB
**      DRSC_TYPE_VARIABLE
**      DRSC_TYPE_OUTPUT
**      DRSC_TYPE_IN_FIFO
**      DRSC_TYPE_OUT_FIFO
**      DRSC_TYPE_RBASE
**      DRSC_TYPE_TRIGGER
**
**  All others cause AF_ERR_NORSRC to be returned.
**
**  Does special case handling:
**      . DRSC_TYPE_CODE - allocates from Code memory.
**      . DRSC_TYPE_KNOB, DRSC_TYPE_VARIABLE, DRSC_TYPE_OUTPUT - allocates from Data memory.
**      . DRSC_TYPE_IN_FIFO, DRSC_TYPE_OUT_FIFO - allocates from FIFOs.
**      . DRSC_TYPE_RBASE - allocates requested # of words with 4-word alignment from Data memory.
**
**  Returns allocated resource (>=0) or error code on failure (<0)
**
******************************************************************/

int32 dsppAllocResource (int32 rsrctype, int32 rsrcamount)
{
    TableAllocator * const allocator = dsppGetResourceAllocator (rsrctype);
    int32 rsrcalloc;

  #if DEBUG_Resource
    PRT(("dsppAllocResource[M2]: type=$%02lx amt=$%04lx allocator=$%08lx\n", rsrctype, rsrcamount, allocator));
  #endif

    if (!allocator) return AF_ERR_BADRSRCTYPE;

  #if DEBUG_Resource
    PRT(("  avail=$%04lx largest=$%04lx\n", TotalAvailThings(allocator), LargestAvailThings(allocator)));
  #endif

    switch (rsrctype) {
        case DRSC_TYPE_RBASE:        /* only thing special about this one is that it requires RBASE alignment */
            rsrcalloc = AllocAlignedThings (allocator, rsrcamount, DSPN_BULLDOG_RBASE_ALIGNMENT);
            break;

        default:
            rsrcalloc = AllocThings (allocator, rsrcamount);
            break;
    }

    return rsrcalloc >= 0 ? rsrcalloc : AF_ERR_NORSRC;
}


/******************************************************************
**
**  Allocate a DSPP resource at a specific address - M2 version.
**
**  Supports:
**      DRSC_TYPE_CODE
**      DRSC_TYPE_KNOB
**      DRSC_TYPE_VARIABLE
**      DRSC_TYPE_OUTPUT
**      DRSC_TYPE_IN_FIFO
**      DRSC_TYPE_OUT_FIFO
**      DRSC_TYPE_RBASE
**      DRSC_TYPE_TRIGGER
**
**  All others cause AF_ERR_NORSRC to be returned.
**
**  Does special case handling:
**      . DRSC_TYPE_CODE - allocates from Code memory.
**      . DRSC_TYPE_KNOB, DRSC_TYPE_VARIABLE, DRSC_TYPE_OUTPUT, DRSC_TYPE_RBASE - allocates from Data memory.
**      . DRSC_TYPE_IN_FIFO, DRSC_TYPE_OUT_FIFO - allocates from FIFOs.
**
**  Caveats
**      . Assumes caller knows proper alignment of things (e.g. DRSC_TYPE_RBASE)
**
**  Returns requested resource (rsrcalloc) (>=0) if available or
**  error code on failure (<0)
**
******************************************************************/

int32 dsppAllocResourceHere (int32 rsrctype, int32 rsrcalloc, int32 rsrcamount)
{
    TableAllocator * const allocator = dsppGetResourceAllocator (rsrctype);

  #if DEBUG_Resource
    PRT(("dsppAllocResourceHere[M2]: type=$%02lx amt=$%04lx allocator=$%08lx alloc=$%04lx\n", rsrctype, rsrcamount, allocator, rsrcalloc));
  #endif

    if (!allocator) return AF_ERR_BADRSRCTYPE;

  #if DEBUG_Resource
    PRT(("  avail=$%04lx largest=$%04lx\n", TotalAvailThings(allocator), LargestAvailThings(allocator)));
  #endif

    return AllocTheseThings (allocator, rsrcalloc, rsrcamount) ? rsrcalloc : AF_ERR_NORSRC;
}


/******************************************************************
**
**  Free DSPP resource allocated by dsppAllocResource() - M2 version.
**
**  Supports:
**      DRSC_TYPE_CODE
**      DRSC_TYPE_KNOB
**      DRSC_TYPE_VARIABLE
**      DRSC_TYPE_OUTPUT
**      DRSC_TYPE_IN_FIFO
**      DRSC_TYPE_OUT_FIFO
**      DRSC_TYPE_RBASE
**      DRSC_TYPE_TRIGGER
**
**  Does special case handling:
**      . DRSC_TYPE_CODE - frees to Code memory.
**      . DRSC_TYPE_KNOB, DRSC_TYPE_VARIABLE, DRSC_TYPE_OUTPUT, DRSC_TYPE_RBASE - frees to Data memory.
**      . DRSC_TYPE_IN_FIFO, DRSC_TYPE_OUT_FIFO - frees to FIFOs.
**
**  Tolerates negative rsrcalloc (as returned from dsppAllocResource()
**  when allocation fails).
**
******************************************************************/

void dsppFreeResource (int32 rsrctype, int32 rsrcalloc, int32 rsrcamount)
{
    TableAllocator * const allocator = dsppGetResourceAllocator (rsrctype);

  #if DEBUG_Resource
    PRT(("dsppFreeResource[M2]: type=$%02lx amt=$%04lx allocator=$%08lx", rsrctype, rsrcamount, allocator));
    PRT((" alloc=$%04lx\n", rsrcalloc));
  #endif

    if (allocator && rsrcalloc >= 0) {
        FreeThings (allocator, rsrcalloc, rsrcamount);
    }

  #if DEBUG_Resource
    if (allocator) PRT(("  avail=$%04lx largest=$%04lx\n", TotalAvailThings(allocator), LargestAvailThings(allocator)));
  #endif
}


/* -------------------- Tick Allocator for M2 */

/*****************************************************************
**
**  Adjust available ticks based on a new DAC frame rate.
**
**  Inputs
**
**      maxFramerate - New maximum audio DAC rate in Hz.
**
**  Results
**
**      Returns 0 on success, Err code on failure (too many
**      ticks in use to increase frame rate)
**
**      Modifies dspp_TickAllocator
**
**  Note: this is called directly by clients - there is one of these
**  for each AF_API_* value.
**
*****************************************************************/

Err dsppAdjustAvailTicks (float32 maxFramerate)
{
    int32 NewMaxTicks;
    int32 LessMaxTicks;  /* amount we're taking out of the avail tick pool (<0 if we're adding to the pool) */
    SystemInfo SI;
    uint32 clockSpeed;
    int32 Result;
    
/* Use actual system clock speed obtained from ROM. */
    Result = SuperQuerySysInfo( SYSINFO_TAG_SYSTEM, &SI , sizeof(SI) );
    if( Result == SYSINFO_SUCCESS )
    {
        clockSpeed = SI.si_CPUClkSpeed;
    }
    else
    {
 /* FIXME - we could error out but this change was made late and I wanted to reduce risk. */
        clockSpeed = DSPP_BULLDOG_CLOCK_RATE;
    }
    	
    NewMaxTicks  = dsppCalcMaxTicks (clockSpeed, maxFramerate);
    LessMaxTicks = dspp_TickAllocator.dtic_MaxTicks - NewMaxTicks;

  #if DEBUG_Tick
    PRT(("dsppAdjustAvailTicks[M2]: maxFramerate=%g maxticks=%ld->%ld", maxFramerate, dspp_TickAllocator.dtic_MaxTicks, NewMaxTicks));
    PRT((" less=%ld avail=%ld\n", LessMaxTicks, dspp_TickAllocator.dtic_AvailTicks));
  #endif

        /* Can we remove LessMaxTicks from avail ticks pool */
    if (dspp_TickAllocator.dtic_AvailTicks < LessMaxTicks) return AF_ERR_NORSRC;

        /* set new max */
    dspp_TickAllocator.dtic_MaxTicks = NewMaxTicks;

        /* reduce avail ticks by LessMaxTicks */
    dspp_TickAllocator.dtic_AvailTicks -= LessMaxTicks;

  #if DEBUG_Tick
    PRT(("  new avail=%ld\n", dspp_TickAllocator.dtic_AvailTicks));
  #endif

    return 0;
}


/*****************************************************************
**
**  Allocate ticks based on RateShift of code to be executed.
**  Returns number ticks allocated from batch
**  (i.e. 8 * frame rate) or error code.
**
**  Inputs
**
**      RateShift - 1,2, or 8 based on execution rate of code
**
**      rsrcamount - number of ticks to allocate at nominal rate.
**
**  Results
**
**      Returns # of ticks allocated from batch (>=0) or Err code (<0).
**
*****************************************************************/

int32 dsppAllocTicks ( int32 RateShift, int32 rsrcamount)
{
    const int32 tickfactor = (int32)DSPR_FRAMES_PER_BATCH >> RateShift;  /* # of ticks to alloc from each batch */
    const int32 totalticks = rsrcamount * tickfactor;                    /* total # of ticks to allocate */

  #if DEBUG_Tick
    PRT(("dsppAllocTicks[M2]: rateshift=%ld amt=%ld avail=%ld\n", RateShift, rsrcamount, dspp_TickAllocator.dtic_AvailTicks));
  #endif

  #ifdef PARANOID
    if (!tickfactor) {
        ERR(("dsppAllocTicks[M2]: Bad RateShift=%ld\n", RateShift));
        return AF_ERR_BADOFX;
    }
  #endif

        /* enough batch-ticks available? */
    if (totalticks > dspp_TickAllocator.dtic_AvailTicks) return AF_ERR_NORSRC;

        /* allocate from pool. */
    dspp_TickAllocator.dtic_AvailTicks -= totalticks;

  #if DEBUG_Tick
    PRT(("  result=%ld avail=%ld total=%ld\n", totalticks, dspp_TickAllocator.dtic_AvailTicks, dspp_TickAllocator.dtic_MaxTicks));
  #endif

    return totalticks;
}


/******************************************************************
**
**  Free ticks allocated by dsppAllocTicks().
**
**  Inputs
**
**      RateShift - 1,2, or 8 based on execution rate of code
**
**      rsrcamount - number of ticks to allocate at nominal rate.
**
******************************************************************/

void dsppFreeTicks ( int32 RateShift, int32 rsrcamount)
{
    const int32 tickfactor = (int32)DSPR_FRAMES_PER_BATCH >> RateShift;  /* # of ticks to alloc from each batch */
    const int32 totalticks = rsrcamount * tickfactor;                    /* total # of ticks to allocate */

  #if DEBUG_Tick
    PRT(("dsppFreeTicks[M2]: rateshift=%ld amt=%ld \n", RateShift, rsrcamount));
    PRT(("  avail=%ld->", dspp_TickAllocator.dtic_AvailTicks));
  #endif

/* Free back to main pool. */
    dspp_TickAllocator.dtic_AvailTicks += totalticks;

  #if DEBUG_Tick
    PRT(("%ld\n", dspp_TickAllocator.dtic_AvailTicks));
  #endif
}


/* -------------------- Resource binding */

/******************************************************************
**
**  The following binding permutations are supported:
**      code mem -> code mem + offset
**      variable -> variable or rbase + offset
**      output -> output or rbase + offset
**      knob -> knob or rbase + offset
**      rbase -> fifo (in or out) + attribute
**
******************************************************************/

/******************************************************************
**
**  Perform API-specific validation on resource binding
**      . Legality of specific type -> type resource binding
**      . Make sure that bound to resource has more of whatever than
**        is requested in binding resource (make sure that offset and
**        offset + length are in bounds)
**
**  Inputs
**
**      drsc - DSPPResource to validate.
**      drsc_bind_to - DSPPResource to bind to.
**
**  Results
**
**      Returns 0 on success or Err code on failure.
**
******************************************************************/

Err dsppValidateResourceBinding (const DSPPResource *drsc, const DSPPResource *drsc_bind_to)
{
    switch (drsc->drsc_Type) {
        case DRSC_TYPE_CODE:
                    /* check type and bounds */
            if (drsc_bind_to->drsc_Type == DRSC_TYPE_CODE &&
                drsc->drsc_Allocated >= 0 && drsc->drsc_Allocated + drsc->drsc_Many <= drsc_bind_to->drsc_Many) return 0;
            break;

    /* !!! optimize these: */
        case DRSC_TYPE_VARIABLE:
            switch (drsc_bind_to->drsc_Type) {
                case DRSC_TYPE_VARIABLE:
                case DRSC_TYPE_RBASE:
                        /* bounds check against bound-to resource */
                    if (drsc->drsc_Allocated >= 0 && drsc->drsc_Allocated + drsc->drsc_Many <= drsc_bind_to->drsc_Many) return 0;
                    break;
            }
            break;

        case DRSC_TYPE_OUTPUT:
            switch (drsc_bind_to->drsc_Type) {
                case DRSC_TYPE_OUTPUT:
                case DRSC_TYPE_RBASE:
                        /* bounds check against bound-to resource */
                    if (drsc->drsc_Allocated >= 0 && drsc->drsc_Allocated + drsc->drsc_Many <= drsc_bind_to->drsc_Many) return 0;
                    break;
            }
            break;

        case DRSC_TYPE_KNOB:
            switch (drsc_bind_to->drsc_Type) {
                case DRSC_TYPE_KNOB:
                case DRSC_TYPE_RBASE:
                        /* bounds check against bound-to resource */
                    if (drsc->drsc_Allocated >= 0 && drsc->drsc_Allocated + drsc->drsc_Many <= drsc_bind_to->drsc_Many) return 0;
                    break;
            }
            break;

        case DRSC_TYPE_RBASE:
            switch (drsc_bind_to->drsc_Type) {
                case DRSC_TYPE_IN_FIFO:
                case DRSC_TYPE_OUT_FIFO:
                    /* Just check type. Attribute (stored in drsc_Allocated) is checked at allocation time. */
                    return 0;
            }
            break;
    }

        /* anything else is an error */
    return AF_ERR_BAD_RSRC_BINDING;
}


/******************************************************************
**
**  Perform resource binding - M2 version.
**
**  Assumes that invalid bindings have been trapped by validation
**  function (above).
**
**  Returns bind resource value (a data memory pointer for M2) (>=0)
**  or error code on failure (<0).
**
******************************************************************/

int32 dsppBindResource (const DSPPResource *drscreq, const DSPPResource *drscboundto)
{
  #if DEBUG_Bind
    PRT(("dsppBindResource[M2]: bind type=$%02x many=$%04lx -> type=$%02x many=$%04x alloc=$%04lx +$%04x\n",
        drscreq->drsc_Type, drscreq->drsc_Many,
        drscboundto->drsc_Type, drscboundto->drsc_Many, drscboundto->drsc_Allocated,
        drscreq->drsc_Allocated));
  #endif

        /* @@@ Doesn't do any real validation of inter-binding types here, expects validator to do that.
               This code would support many more combinations than the validator currently permits. */
    switch (drscboundto->drsc_Type) {
        case DRSC_TYPE_IN_FIFO:
        case DRSC_TYPE_OUT_FIFO:
            return dsppGetResourceAttribute (drscboundto, drscreq->drsc_Allocated);

        default:
            return drscboundto->drsc_Allocated  /* bound-to's allocation */
                 + drscreq->drsc_Allocated;     /* + offset stored in this one's pre-allocation field */
    }
}


/* -------------------- Allocation support */

/******************************************************************
**
**  Get allocator for a given of DRSC_ type.
**
**  Supports:
**      DRSC_TYPE_CODE
**      DRSC_TYPE_KNOB
**      DRSC_TYPE_VARIABLE
**      DRSC_TYPE_OUTPUT
**      DRSC_TYPE_IN_FIFO
**      DRSC_TYPE_OUT_FIFO
**      DRSC_TYPE_RBASE
**      DRSC_TYPE_TRIGGER
**
**  Returns pointer to a TableAllocator on success, or NULL on
**  failure.
**
******************************************************************/

static TableAllocator *dsppGetResourceAllocator (int32 rsrctype)
{
    switch (rsrctype) {
        case DRSC_TYPE_CODE:
            return &dspp_CodeMemoryAllocator;

        case DRSC_TYPE_VARIABLE:
        case DRSC_TYPE_KNOB:
        case DRSC_TYPE_OUTPUT:
        case DRSC_TYPE_RBASE:
            return &dspp_DataMemoryAllocator;

        case DRSC_TYPE_IN_FIFO:
        case DRSC_TYPE_OUT_FIFO:
            return &dspp_FIFOAllocator;

        case DRSC_TYPE_TRIGGER:
            return &dspp_TriggerAllocator;

        default:
            return NULL;
    }
}

/* -------------------- Query */

/******************************************************************
**
**  Return actual DSPI address, or other value suitable for relocation,
**  for an allocated resource. This is handy for resource that
**  consist of multiple registers (e.g. FIFOs).
**
**  Inputs
**
**      drsc        Allocated DSPPResource to process.
**
**      Attribute   Hardware independent identifier for the
**                  member of the multi-register resource to return
**                  (e.g. DRSC_FIFO_ values), or 0.
**
**  Results
**
**      DRSC_*_FIFO     address of FIFO register specified by Attribute.
**
**      Other           contents of drsc_Allocated.
**
**      Returns error code on failure.
**
******************************************************************/

int32 dsppGetResourceAttribute (const DSPPResource *drsc, int32 Attribute)
{
    switch (drsc->drsc_Type) {
        case DRSC_TYPE_IN_FIFO:
        case DRSC_TYPE_OUT_FIFO:
            switch(Attribute) {
                case DRSC_FIFO_NORMAL:
                    return DSPI_FIFO_DATA(drsc->drsc_Allocated);        /* Read/Write and bump */

                case DRSC_FIFO_STATUS:
                    return DSPI_FIFO_CONTROL(drsc->drsc_Allocated);     /* Status */

                case DRSC_FIFO_READ:
                    return DSPI_FIFO_CURRENT(drsc->drsc_Allocated);     /* Read w/o bump */

                case DRSC_FIFO_OSC:
                    return DSPI_FIFO_OSC(drsc->drsc_Allocated);         /* Base of osc register set */

                default:
                    return AF_ERR_RSRCATTR;
            }

        default:
            return (drsc->drsc_Allocated + Attribute);  /* For memory arrays, etc. */
    }
}


/* -------------------- Resource availability */

static void dsppGetAvailAndLargest (AudioResourceInfo *, const TableAllocator *);
static void dsppGetAvail (AudioResourceInfo *, const TableAllocator *);


/******************************************************************
**
**  @@@ autodoc in dspp_resources.c
**
**  Note: this is called directly by clients - there is one of these
**  for each AF_API_* value.
**
******************************************************************/

Err GetAudioResourceInfo (AudioResourceInfo *resultInfo, uint32 resultInfoSize, uint32 rsrcType)
{
    AudioResourceInfo info;

    memset (&info, 0, sizeof info);

  #if DEBUG_Allocator
    switch (rsrcType) {
        case AF_RESOURCE_TYPE_CODE_MEM: DumpTableAllocator (&dspp_CodeMemoryAllocator, "Code Mem"); break;
        case AF_RESOURCE_TYPE_DATA_MEM: DumpTableAllocator (&dspp_DataMemoryAllocator, "Data Mem"); break;
        case AF_RESOURCE_TYPE_FIFOS:    DumpTableAllocator (&dspp_FIFOAllocator,       "FIFOs");    break;
        case AF_RESOURCE_TYPE_TRIGGERS: DumpTableAllocator (&dspp_TriggerAllocator,    "Triggers"); break;
    }
  #endif

    switch (rsrcType) {
        case AF_RESOURCE_TYPE_TICKS:
            info.rinfo_Total = dspp_TickAllocator.dtic_MaxTicks;
            info.rinfo_Free  = dspp_TickAllocator.dtic_AvailTicks;
            break;

        case AF_RESOURCE_TYPE_CODE_MEM:
            dsppGetAvailAndLargest (&info, &dspp_CodeMemoryAllocator);
            break;

        case AF_RESOURCE_TYPE_DATA_MEM:
            dsppGetAvailAndLargest (&info, &dspp_DataMemoryAllocator);
            break;

        case AF_RESOURCE_TYPE_FIFOS:
            dsppGetAvail (&info, &dspp_FIFOAllocator);
            break;

        case AF_RESOURCE_TYPE_TRIGGERS:
            dsppGetAvail (&info, &dspp_TriggerAllocator);
            break;

        default:
            return AF_ERR_BADRSRCTYPE;
    }

        /* success: copy to client's buffer */
    memset (resultInfo, 0, resultInfoSize);
    memcpy (resultInfo, &info, MIN (resultInfoSize, sizeof info));

    return 0;
}

static void dsppGetAvailAndLargest (AudioResourceInfo *info, const TableAllocator *allocator)
{
    info->rinfo_Total = allocator->tall_Size;
    info->rinfo_Free  = TotalAvailThings (allocator);
    info->rinfo_LargestFreeSpan = LargestAvailThings (allocator);
}

static void dsppGetAvail (AudioResourceInfo *info, const TableAllocator *allocator)
{
    info->rinfo_Total = allocator->tall_Size;
    info->rinfo_Free  = TotalAvailThings (allocator);
}
