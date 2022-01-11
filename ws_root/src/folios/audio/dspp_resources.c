/******************************************************************************
**
**  @(#) dspp_resources.c 96/07/02 1.47
**  $Id: dspp_resources.c,v 1.54 1995/03/10 22:58:53 peabody Exp phil $
**
**  DSPP resource manager (asic-independent part)
**
**  By: Phil Burk and Bill Barton
**
**  Copyright (c) 1995, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  950203 WJB  Extracted from dspp_loader.c and dspp_instr.c.
**  950207 WJB  Revised function names.
**  950207 WJB  Changed some functions to return Err.
**              Made some args const.
**  950207 WJB  Added usage of DSPP_TYPE_MASK. Fixed some triple bangs.
**  950207 WJB  Fixed DSPGetTotalRsrcUsed() DRSC_TICKS to use
**              DSPPData.dspp_TicksPerFrame instead of constant.
**  950208 WJB  Added usage of DRSC_TYPE_MASK to dsppGetResourceAttribute().
**  950209 WJB  Renamed dsppAlloc/FreeResource() to dsppOpen/CloseResource().
**  950210 WJB  Moved anvil-specific stuff to dspp_resources_anvil.c.
**  950213 WJB  Moved I-mem init to anvil-specific code.
**  950213 WJB  Modified API for dsppAlloc/FreeResource().
**  950213 WJB  Commonized ADC open code - dsppTestAudioInput().
**  950213 WJB  Revised API to dsppImportResource().
**  950213 WJB  Revised API to and optimized dsppAllocTicks().
**  950213 WJB  Cleaned up dsppFreeTicks().
**  950213 WJB  Revised dsppFreeTicks() API.
**  950213 WJB  Fixed incorrect allocation amount in dsppExportResource().
**  950214 WJB  Cleaned up dsppCloseResource().
**  950214 WJB  Made dins_Template trap be PARANOID code. Tweaked debug.
**  950214 WJB  Cleaned up dsppOpenResource() (no object code change).
**  950214 WJB  Revised dsppOpenResource() var names (no object code change).
**  950215 WJB  Restructured a bit to resolve some failure path problems.
**  950215 WJB  Relaxed dsppUnimportResource() to ignore the condition of resource not found.
**  950215 WJB  Optimized dsppOpenResource().
**  950216 WJB  Optimized dsppCloseResource().
**  950216 WJB  Added dsppOpen/CloseInstrumentResources().
**              Hid dsppOpen/CloseResource().
**  950216 WJB  Optimized dsppOpenIntrumentResources().
**  950216 WJB  First step in restructuring allocation loop in dsppOpenInstrumentResources().
**  950217 WJB  Added dsppExport/UnexportInstrumentResources().
**  950217 WJB  Moved drsc_Allocated check from dsppUnexportResource() to dsppUnexportInstrumentResources().
**  950217 WJB  Added DRSC_IMPORT trap in label relocator.
**  950217 WJB  Cleaned up dsppSumResourceReferences().
**  950217 WJB  Updated internal docs.
**  950217 WJB  Made dsppCloseInstrumentResources() return void.
**              Removed Export/Unexport from dsppOpen/CloseInstrumentResources().
**  950217 WJB  Added an error message in dsppUnexportInstrumentResources().
**  950217 WJB  Replaced EZMemAlloc/Free() w/ SuperMemAlloc/Free().
**  950224 WJB  Changed int32 drsc_Type into 4 uint8 fields.
**  950224 WJB  Replaced DSPPExternal with DSPPExportedResource.
**              Added dins_ExportedResources[] to DSPPInstrument.
**              Retired drsc_References.
**              Renamed dspp_ExternalList to dspp_ExportedResourceList.
**  950303 WJB  Added dsppIsValidResourceType().
**              Removed DRSC_ type validation in dsppOpenResource().
**  950303 WJB  Tweaked debug.
**  950303 WJB  Moved dsppAlloc/FreeTicks() into mode-dependent modules.
**  950307 WJB  Added rsrcalloc arg to dsppFreeTicks().
**  950310 WJB  Publicized dsppImport/UnimportResource().
**  950412 WJB  Added support for DRSC_F_BIND.
**  950417 WJB  Added dsppValidateTemplateResources().
**              Privatized dsppIsValidResourceType().
**  950417 WJB  Added call to dsppValidateResourceBinding().
**              Added validation debugging.
**  950425 WJB  Moved dspp_ExportedResourceList here.
**  950428 WJB  Added support for DRCS_HW_CPU_INT.
**  950428 WJB  Renamed DSPI_CPU_INT0 to DSPI_CPU_INT.
**  950809 WJB  Tidied up after resource type changes.
**  950809 WJB  Added dsppIsValidResourceInitFlags() (currently commented out).
**  950809 WJB  Removed support for old-style exported labels.
**  950823 WJB  Added validate trap for drsc_Many == 0.
**  950823 WJB  Removed support for pre-allocated resources (drsc_Many==0) and old-style exported label fixup.
**  950922 WJB  Now only permits privileged instruments to export symbols.
**  960105 WJB  Added GetAudioResourceInfo().
**  960108 WJB  Added dsppGetTemplateResourceUsage().
**  960115 WJB  Added dsppGetInstrumentPortInfo() and dsppGetRsrcPortType().
**  960520 PLB  Enable dsppIsValidResourceInitFlags() now that assembler is fixed.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/

#include <kernel/list.h>                /* INITLIST() */
#include <kernel/kernel.h>

#include "audio_internal.h"             /* TRACE, PRT() */
#include "dspp_resources_internal.h"    /* self */


/* -------------------- Debugging */

#define DEBUG_Resource  0               /* main debugging */
#define DEBUG_Export    0               /* debug Export/Unepoxrt() */
#define DEBUG_Validate  0               /* turn on debugging during validation */

#if DEBUG_Resource
  #define DBUG(x)      PRT(x)
  #define DBUGRSRC(x)  PRT(x)
#else
  #define DBUG(x)
  #define DBUGRSRC(x)
#endif

#if DEBUG_Resource || DEBUG_Validate
  #undef ERRDBUG
  #define ERRDBUG(x)   PRT(x)
#endif


/* -------------------- Local data */

    /* List of system-wide DSPPExportedResources. Can search by name. */
static List dspp_exportedResourceList = INITLIST (dspp_exportedResourceList, "DSPP Exported Resources");


/* -------------------- Local functions */

static Err dsppOpenResource (DSPPInstrument *, int32 indx);
static void dsppCloseResource (DSPPInstrument *, int32 indx);

static Err dsppTestAudioInput (void);

static Err dsppExportResource (DSPPExportedResource *, DSPPInstrument *, int32 rsrcindex);
static void dsppUnexportResource (DSPPExportedResource *);
#if DEBUG_Export
  static void dsppDumpExportList (const char *desc);
#endif


/* -------------------- Template Resource Validation */

static bool dsppIsValidResourceType (uint8 rsrcType);
static bool dsppIsValidResourceInitFlags (const DSPPResource *);

/******************************************************************
**
**  Perform validation pass on DSPPTemplate's DSPPResource array.
**      . Check for valid resource type.
**      . Make sure all DRSC_F_BIND rsrc[indx].drsc_BoundToRsrc < indx
**        (bound resource must be allocated first).
**      . Make sure all DRSC_F_BIND BoundTo resources are marked
**        with DRSC_F_BOUND_TO.
**      . Call mode-specific validation code.
**
**  Inputs
**
**      dtmp - reads these fields:
**          . dtmp_NumResources
**          . dtmp_Resources
**
**  Results
**
**      Returns 0 on success or Err code on failure.
**
******************************************************************/

Err dsppValidateTemplateResources (const DSPPTemplate *dtmp)
{
    int32 i;
    Err errcode;

  #if DEBUG_Validate
    PRT(("\ndsppValidateTemplateResources: dtmp=0x%08lx functionid=%d %d resource(s)\n", dtmp, dtmp->dtmp_Header.dhdr_FunctionID, dtmp->dtmp_NumResources));
  #endif

    for (i=0; i<dtmp->dtmp_NumResources; i++) {
        const DSPPResource * const drsc = &dtmp->dtmp_Resources[i];

      #if DEBUG_Validate
        dsppDumpResource (dtmp,i);
      #endif

            /* Validate resource type */
        if (!dsppIsValidResourceType (drsc->drsc_Type)) {
            ERRDBUG(("dsppValidateTemplateResources: Invalid resource type = 0x%02x\n", drsc->drsc_Type));
            return AF_ERR_BADRSRCTYPE;
        }

            /* Validate usage of DRSC_F_INIT_AT flags */
        if (!dsppIsValidResourceInitFlags (drsc)) {
            ERRDBUG(("dsppValidateTemplateResources: Invalid usage of init flags: type=0x%02x flags=0x%02x\n", drsc->drsc_Type, drsc->drsc_Flags));
            return AF_ERR_BADOFX;
        }

            /* DRSC_F_EXPORT requires a privileged template */
        if (drsc->drsc_Flags & DRSC_F_EXPORT && !(dtmp->dtmp_Header.dhdr_Flags & DHDR_F_PRIVILEGED)) {
            ERRDBUG(("dsppValidateTemplateResources: Export requires privilege\n"));
            return AF_ERR_BADOFX;
        }

            /* make sure drsc_Many is valid (exception: bound code resources have many==0 - !!! perhaps change assembler to write 1 for this case) */
        if (drsc->drsc_Many <= 0 && !(drsc->drsc_Type == DRSC_TYPE_CODE && drsc->drsc_Flags & DRSC_F_BIND)) {
            ERRDBUG(("dsppValidateTemplateResources: Invalid resource allocation: many=0x%04x\n", drsc->drsc_Many));
            return AF_ERR_BADOFX;
        }

            /* Do more checking for DRSC_F_BIND resources */
        if (drsc->drsc_Flags & DRSC_F_BIND) {
            const DSPPResource * const drsc_bind_to = &dtmp->dtmp_Resources[drsc->drsc_BindToRsrcIndex];

                /* Make sure resource to which this one is bound, precedes this one */
            if (drsc->drsc_BindToRsrcIndex < 0 || drsc->drsc_BindToRsrcIndex >= i) {
                ERRDBUG(("dsppValidateTemplateResources: Binding out index of range: %d->%d\n", i, drsc->drsc_BindToRsrcIndex));
                return AF_ERR_BAD_RSRC_BINDING;
            }

                /* Make sure bound-to resource has DRSC_F_BOUND_TO set. */
            if (!(drsc_bind_to->drsc_Flags & DRSC_F_BOUND_TO)) {
                ERRDBUG(("dsppValidateTemplateResources: Bound-to resource not marked w/ DRSC_F_BOUND_TO: %d->%d\n", i, drsc->drsc_BindToRsrcIndex));
                return AF_ERR_BAD_RSRC_BINDING;
            }

                /* Call API-specific validator */
            if ((errcode = dsppValidateResourceBinding (drsc, drsc_bind_to)) < 0) {
                ERRDBUG(("dsppValidateTemplateResources: Invalid binding: %d->%d\n", i, drsc->drsc_BindToRsrcIndex));
                return errcode;
            }
        }
    }

    return 0;
}

/******************************************************************
**  Return TRUE if resource type is valid.
******************************************************************/
static bool dsppIsValidResourceType (uint8 rsrctype)
{
    return rsrctype < DSPP_NUM_RSRC_TYPES;
}

/******************************************************************
**  Return TRUE if initializer flags are valid
**  !!! same rules apply to DINIs, probably ought to generalize this a bit.
******************************************************************/
static bool dsppIsValidResourceInitFlags (const DSPPResource *drsc)
{
    if (drsc->drsc_Flags & (DRSC_F_INIT_AT_ALLOC | DRSC_F_INIT_AT_START)) {

            /* Don't permit imported resources to be initialized */
        if (drsc->drsc_Flags & DRSC_F_IMPORT) return FALSE;

            /* enforce init rules for each resource type */
        switch (drsc->drsc_Type) {
                /* these can be init'd at alloc or start */
            case DRSC_TYPE_VARIABLE:
            case DRSC_TYPE_OUTPUT:
            case DRSC_TYPE_RBASE:
                break;

                /* these can be init'd at alloc only */
            case DRSC_TYPE_KNOB:
                if (drsc->drsc_Flags & DRSC_F_INIT_AT_START) return FALSE;
                break;

                /* all the others aren't allowed to be initialized */
            default:
                return FALSE;
        }
    }

    return TRUE;
}


/* -------------------- Open/Close resource set for instrument */

/******************************************************************
**
**  Open all DSPP resources for a DSPPInstrument.
**      . Allocates DSPPResource table for instrument
**      . Opens (allocates or imports) all resources
**
**  Cleans up after itself on failure.
**
**  Inputs
**
**      dins - reads these fields:
**          . dins_Template
**          . dins_RateShift
**
**  Results
**
**      Returns 0 on success or Err code on failure.
**
**      Modifies dins fields:
**          . dins_Resources - allocated table on success, NULL on failure.
**          . dins_NumResources - number of resources, 0 on failure.
**          . dins_ExecFrame
**          . dins_EntryPoint
**          . dins_DSPPCodeSize
**
******************************************************************/

Err dsppOpenInstrumentResources (DSPPInstrument *dins)
{
    const int32 numResources = dins->dins_Template->dtmp_NumResources;
    Err errcode;
    int32 i;

  #if DEBUG_Resource
    PRT(("\ndsppOpenInstrumentResources() dins=0x%08lx %ld resource(s)\n", dins, numResources));
  #endif

        /* alloc/init resource table */
    dins->dins_NumResources = numResources;
    if ((dins->dins_Resources = (DSPPResource *)SuperAllocMem (numResources * sizeof (DSPPResource), MEMTYPE_ANY | MEMTYPE_FILL)) == NULL) {
        errcode = AF_ERR_NOMEM;
        goto clean;
    }
    for (i=0; i<numResources; i++) {
        dins->dins_Resources[i].drsc_Allocated = DRSC_ALLOC_INVALID;
    }

        /* Open all resources (assumes dins_Resources table init'd above) */
    for (i=0; i<numResources; i++) {
        if ((errcode = dsppOpenResource (dins,i)) < 0) goto clean;
    }

  #if DEBUG_Export
    {
        int32 i;

            /* dump export list if this instrument imported something. */
        for (i=0; i<dins->dins_NumResources; i++) if (dins->dins_Resources[i].drsc_Flags & DRSC_F_IMPORT) {
            dsppDumpExportList ("dsppOpenInstrumentResources");
            break;
        }
    }
  #endif

    return 0;

clean:
  #if DEBUG_Resource
    PRT(("dsppOpenInstrumentResources() error: 0x%08lx\n", errcode));
  #endif

    dsppCloseInstrumentResources (dins);
    return errcode;
}


/******************************************************************
**
**  Close all DSPP resources for a DSPPInstrument.
**      . Opens (deallocates or unimports) all resources
**      . Frees DSPPResource table
**
**  Inputs
**
**      dins - reads these fields:
**          . dins_Template
**          . dins_Resources - tolerates NULL pointer and partially filled out table.
**          . dins_NumResources - tolerates 0.
**          . dins_ExecFrame
**          . dins_RateShift
**
**  Results
**
**      Modifies dins fields:
**          . dins_Resources - sets to NULL
**          . dins_NumResources - sets to 0
**
******************************************************************/

void dsppCloseInstrumentResources (DSPPInstrument *dins)
{
  #if DEBUG_Resource
    PRT(("\ndsppCloseInstrumentResources() dins=0x%08lx %ld resource(s)\n", dins, dins->dins_NumResources));
  #endif

    if (dins->dins_Resources) {
        int32 i;

            /* Close resources */
        for (i=0; i<dins->dins_NumResources; i++) {
            TRACEB(TRACE_INT,TRACE_OFX,("dsppCloseInstrumentResources: rsrc[%d]\n", i));
            dsppCloseResource (dins, i);
        }

      #if DEBUG_Export
        {
            int32 i;

                /* dump export list if this instrument imported something. */
            for (i=0; i<dins->dins_NumResources; i++) if (dins->dins_Resources[i].drsc_Flags & DRSC_F_IMPORT) {
                dsppDumpExportList ("dsppCloseInstrumentResources");
                break;
            }
        }
      #endif

            /* Free table */
        SuperFreeMem (dins->dins_Resources, dins->dins_NumResources * sizeof (DSPPResource));
        dins->dins_Resources = NULL;
        dins->dins_NumResources = 0;
    }

}


/* -------------------- Export/Unexport resource set for instrument */

/******************************************************************
**
**  Export all DRSC_F_EXPORT resources for a DSPPInstrument.
**  Allocates and fills in dins_ExportedResources[] array if there
**  are any resources to export. Otherwise, this function does
**  nothing.
**
**  Cleans up after itself on failure.
**
**  Inputs
**
**      dins - reads these fields:
**          . dins_Resources
**          . dins_NumResources
**
**  Results
**
**      Returns 0 on success or Err code on failure.
**
**      Modifies dins fields:
**          . dins_NumExportedResources - number of exported resources (0 is valid and not an indication of failure)
**          . dins_ExportedResources - allocated array of DSPPExportedResource's, or NULL if dins_NumExportedResources==0.
**
**      Modifies dspp_exportedResourceList.
**
******************************************************************/

Err dsppExportInstrumentResources (DSPPInstrument *dins)
{
    Err errcode;
    int32 i;
    int32 numExportees;

        /* count number of resources to export */
    numExportees = 0;
    for (i=0; i<dins->dins_NumResources; i++) {
        if (dins->dins_Resources[i].drsc_Flags & DRSC_F_EXPORT) numExportees++;
    }

  #if DEBUG_Export
    PRT(("\ndsppExportInstrumentResources() dins=0x%08lx %ld exportee(s)\n", dins, numExportees));
  #endif

        /* build table (don't allocate a table if no exportees) */
    if (numExportees) {

            /* allocate dins_ExportedResources[] */
        dins->dins_NumExportedResources = numExportees;
        if ((dins->dins_ExportedResources = (DSPPExportedResource *)SuperAllocMem (numExportees * sizeof (DSPPExportedResource), MEMTYPE_ANY | MEMTYPE_FILL)) == NULL) {
            errcode = AF_ERR_NOMEM;
            goto clean;
        }

            /* export resources, fill out dins_ExportedResources[] */
        {
            DSPPExportedResource *dexp = dins->dins_ExportedResources;  /* pointer to next DSPPExportedResource to fill out */

            for (i=0; i<dins->dins_NumResources; i++) {
                if (dins->dins_Resources[i].drsc_Flags & DRSC_F_EXPORT) {
                    if ((errcode = dsppExportResource (dexp++, dins, i)) < 0) goto clean;
                }
            }
        }

      #if DEBUG_Export
        dsppDumpExportList ("dsppExportInstrumentResources");
      #endif
    }

    return 0;

clean:
  #if DEBUG_Export
    PRT(("dsppExportInstrumentResources() error: 0x%08lx\n", errcode));
    dsppDumpExportList ("dsppExportInstrumentResources");
  #endif

    dsppUnexportInstrumentResources (dins);
    return errcode;
}


/******************************************************************
**
**  Unexport all DRSC_F_EXPORT resources for a DSPPInstrument.
**  Frees dins_ExportedResources[] array if present. First
**  verifies that the any exported resources are not in use. If any
**  are still in use, returns an error code w/o unexporting anything.
**
**  Inputs
**
**      dins - reads these fields:
**          . dins_NumExportedResources - can be 0.
**          . dins_ExportedResources - can be NULL.
**
**  Results
**
**      Returns 0 on success or Err code on failure (a resource
**      is still in use). !!! may become void.
**
**      Modifies dins fields:
**          . dins_NumExportedResources - set to 0.
**          . dins_ExportedResources - set to NULL.
**
**      Modifies dspp_exportedResourceList.
**
******************************************************************/

Err dsppUnexportInstrumentResources (DSPPInstrument *dins)
{
#if DEBUG_Export
	PRT(("\ndsppUnexportInstrumentResources() dins=0x%08lx %ld exportee(s)\n", dins, dins->dins_NumExportedResources));
#endif

	if (dins->dins_ExportedResources)
	{
		int32 i;

/* Unexport all exported resources */
		for (i=0; i<dins->dins_NumExportedResources; i++)
		{
			dsppUnexportResource (&dins->dins_ExportedResources[i]);
		}

/* Free dins_ExportedResources[] array */
		SuperFreeMem (dins->dins_ExportedResources, dins->dins_NumExportedResources * sizeof (DSPPExportedResource));
		dins->dins_ExportedResources = NULL;
		dins->dins_NumExportedResources = 0;

#if DEBUG_Export
	dsppDumpExportList ("dsppUnexportInstrumentResources");
#endif
	}

	return 0;
}


/* -------------------- Open/Close Single Resource */

/******************************************************************
**
**  Open a DSPP resources for a given DSPPResource (extracted
**  from DSPPInstrument). Either imports or allocates depending
**  on DSPPResource.
**
**  Inputs
**
**      dins - reads these fields:
**          . dins_Template
**          . dins_Resources[indx] - assumes that entry has been
**            precleared.
**
**      indx - index into dins_Resources[] table to allocate.
**
**  Results
**
**      Returns 0 on success or Err code on failure.
**
**      Fills in dins_Resources[] entry on success, otherwise it is
**      left untouched.
**
**      Depending on the resource being allocated, can touch:
**          . dins_ExecFrame
**          . dins_EntryPoint
**          . dins_DSPPCodeSize
**
******************************************************************/

static Err dsppOpenResource (DSPPInstrument *dins, int32 indx)
{
    const DSPPResource * const drscreq = &dins->dins_Template->dtmp_Resources[indx];
    DSPPResource * const drscalloc = &dins->dins_Resources[indx];
    int32 rsrcalloc;

  #if DEBUG_Resource
    printf ("dsppOpenResource: ");
    dsppDumpResource (dins->dins_Template, indx);
  #endif

    TRACEB(TRACE_INT,TRACE_OFX|TRACE_HACK,("dsppOpenResource: RsrcName = %s\n", DSPPGetRsrcName(dins, indx) ));

        /* Imported it from another instrument? */
        /* @@@ drsc_Many is a don't care field for imported resources - be sure to ignore it */
    if (drscreq->drsc_Flags & DRSC_F_IMPORT) {
        TRACEB(TRACE_INT,TRACE_OFX,("dsppOpenResource: Imported resource!\n"));
        rsrcalloc = dsppImportResource (DSPPGetRsrcName (dins, indx));
    }
        /* Bind it to another resource in this instrument? */
    else if (drscreq->drsc_Flags & DRSC_F_BIND) {
        rsrcalloc = dsppBindResource (drscreq, &dins->dins_Resources[drscreq->drsc_BindToRsrcIndex]);
    }
        /* Allocate it */
        /* @@@ can't tolerate drsc_Many == 0, expects caller to validate */
    else {
        TRACEB(TRACE_INT,TRACE_OFX,("dsppOpenResource: Type = %d, Many = %d\n", drscreq->drsc_Type, drscreq->drsc_Many ));

        switch (drscreq->drsc_Type) {
            case DRSC_TYPE_TICKS:
                rsrcalloc = dsppAllocTicks (dins->dins_RateShift, drscreq->drsc_Many);
                break;

            case DRSC_TYPE_CODE:
                /* !!! this is kind of an icky special case. should probably make entry location be done at a higher level after all resources are opened */
                if ((rsrcalloc = dsppAllocResource (drscreq->drsc_Type, drscreq->drsc_Many)) >= 0) {
                    dins->dins_EntryPoint   = rsrcalloc;
                    dins->dins_DSPPCodeSize = drscreq->drsc_Many;
                }
                break;

            case DRSC_TYPE_INPUT:
                rsrcalloc = 0;    /* Inputs are either immediates, or connected so they don't need data memory. */
                break;

/* !!! Is there a security problem if a bogus dsp instrument
** allocates a huge number of parts and attempts to init via DINI?
*/
            case DRSC_TYPE_HW_DAC_OUTPUT:
                rsrcalloc = DSPI_OUTPUT0;
                break;

            case DRSC_TYPE_HW_OUTPUT_STATUS:
                rsrcalloc = DSPI_OUTPUT_STATUS;
                break;

            case DRSC_TYPE_HW_OUTPUT_CONTROL:
                rsrcalloc = DSPI_OUTPUT_CONTROL;
                break;

            case DRSC_TYPE_HW_ADC_INPUT:
                if ((rsrcalloc = dsppTestAudioInput()) >= 0) rsrcalloc = DSPI_INPUT0;
                break;

            case DRSC_TYPE_HW_INPUT_STATUS:
                rsrcalloc = DSPI_INPUT_STATUS;
                break;

            case DRSC_TYPE_HW_INPUT_CONTROL:
                rsrcalloc = DSPI_INPUT_CONTROL;
                break;

            case DRSC_TYPE_HW_NOISE:
                rsrcalloc = DSPI_NOISE;
                break;

            case DRSC_TYPE_HW_CPU_INT:
                rsrcalloc = DSPI_CPU_INT;
                break;

            case DRSC_TYPE_HW_CLOCK:
                rsrcalloc = DSPI_CLOCK;
                break;

            default:
                rsrcalloc = dsppAllocResource (drscreq->drsc_Type, drscreq->drsc_Many);
                break;
        }
    }

        /* trap failure from above */
    if (rsrcalloc < 0) return rsrcalloc;

        /* post results to drscalloc */
    drscalloc->drsc_Allocated       = rsrcalloc;
    drscalloc->drsc_SubType         = drscreq->drsc_SubType;            /* !!! redundant: this will go away */
    drscalloc->drsc_Flags           = drscreq->drsc_Flags;              /* !!! redundant: this will go away */
    drscalloc->drsc_Type            = drscreq->drsc_Type;               /* !!! redundant: this will go away */
    drscalloc->drsc_Many            = drscreq->drsc_Many;               /* !!! redundant: this will go away */
    drscalloc->drsc_BindToRsrcIndex = drscreq->drsc_BindToRsrcIndex;    /* !!! redundant: this will go away */
    drscalloc->drsc_Default         = drscreq->drsc_Default;            /* !!! redundant: this will go away */

  #if DEBUG_Resource
    printf ("dsppOpenResource: allocated=0x%04x (if datamem then at 0x%08x) \n",
        drscalloc->drsc_Allocated, (DSPX_DATA_MEMORY + drscalloc->drsc_Allocated) );
  #endif
    TRACEB(TRACE_INT,TRACE_HACK,("dsppOpenResource: Type = 0x%x, Allocated = 0x%x\n", drscalloc->drsc_Type, drscalloc->drsc_Allocated));
    DBUG2(("Resource: DSPI[0x%x] at 0x%x is '%s'\n", drscalloc->drsc_Allocated,
        &DSPX_DATA_MEMORY[drscalloc->drsc_Allocated], DSPPGetRsrcName(dins,indx) ));

    return 0;
}


/******************************************************************
**
**  Close a DSPP resource opened by dsppOpenResource(). Either
**  frees the resource or drops access to an imported shared resource.
**
**      dins:
**          . dins_Resources[indx] - can be unopened. It is
**            assumed that this resource is not still on the
**            dspp_exportedResourceList.
**          . dins_ExecFrame - can be unset.
**
**      indx - index into dins_Resources[] table to allocate.
**
**  Results
**
**      Clears dins_Resources[indx].
**
******************************************************************/

static void dsppCloseResource (DSPPInstrument *dins, int32 indx)
{
    DSPPResource * const drscalloc = &dins->dins_Resources[indx];

  #if DEBUG_Resource
    printf ("dsppCloseResource: ");
    dsppDumpResource (dins->dins_Template, indx);
    printf ("dsppCloseResource: alloc=0x%04x\n", drscalloc->drsc_Allocated);
  #endif

        /* do nothing if resource not opened */
    if (drscalloc->drsc_Allocated >= 0) {

            /* Imported? */
        if (drscalloc->drsc_Flags & DRSC_F_IMPORT) {
            TRACEB(TRACE_INT,TRACE_OFX,("dsppCloseResource: Imported resource! \n"));
            dsppUnimportResource (DSPPGetRsrcName (dins, indx));
        }
            /* Bound? - do nothing */
        else if (drscalloc->drsc_Flags & DRSC_F_BIND) {
        }
            /* Must be allocated, free it */
            /* @@@ can't tolerate drsc_Many == 0, expects caller to validate */
        else {
            switch (drscalloc->drsc_Type) {
                case DRSC_TYPE_TICKS:
                    dsppFreeTicks (dins->dins_RateShift, drscalloc->drsc_Many);
                    break;

                    /* We didn't allocate these from table so we shouldn't free them either. 950116 */
                case DRSC_TYPE_INPUT:
                case DRSC_TYPE_HW_DAC_OUTPUT:
                case DRSC_TYPE_HW_OUTPUT_STATUS:
                case DRSC_TYPE_HW_OUTPUT_CONTROL:
                case DRSC_TYPE_HW_ADC_INPUT:
                case DRSC_TYPE_HW_INPUT_STATUS:
                case DRSC_TYPE_HW_INPUT_CONTROL:
                case DRSC_TYPE_HW_NOISE:
                case DRSC_TYPE_HW_CPU_INT:
                case DRSC_TYPE_HW_CLOCK:
                    break;

                default:
                    dsppFreeResource (drscalloc->drsc_Type, drscalloc->drsc_Allocated, drscalloc->drsc_Many);
                    break;
            }
        }

            /* Clear drscalloc */
        memset (drscalloc, 0, sizeof *drscalloc);     /* !!! somewhat redundant */
        drscalloc->drsc_Allocated = DRSC_ALLOC_INVALID;
    }
}


/* -------------------- Audio Input */

/******************************************************************
**  See if Audio Input is enabled. Returns 0 if enabled, error
**  code otherwise.
******************************************************************/

static Err dsppTestAudioInput (void)
{
	const AudioFolioTaskData * const aftd = GetAudioFolioTaskData(CURRENTTASK);

	if (!aftd || (aftd->aftd_InputEnables <= 0)) {
		ERR((
			"CreateInstrument: Cannot create an instrument which uses Audio Input\n"
			"                  without first calling EnableAudioInput().\n"));
		return AF_ERR_NORSRC;   /* !!! "not enabled" or "no license" error would be better (maybe base on ER_NoHardware?) */
	}

	return 0;
}


/* -------------------- Import/Export Single Resource */

/******************************************************************
**
**  Gain access to a previously exported shared resource.
**
**  Returns shared resource (>=0) or Err code (<0).
**
******************************************************************/

int32 dsppImportResource (const char *name)
{
	DSPPExportedResource * const dexp = (DSPPExportedResource *)FindNamedNode (&dspp_exportedResourceList, name);

#if DEBUG_Export
	PRT(("dsppImportResource() dexp=0x%08lx '%s'\n", dexp, name));
#endif

	if (!dexp)
	{
		ERR(("dsppImportResource: Couldn't find external '%s'\n", name));
		return AF_ERR_EXTERNALREF;
	}

/* return resource allocation from owner */
	return dexp->dexp_Owner->dins_Resources [ dexp->dexp_ResourceIndex ] . drsc_Allocated;
}


/******************************************************************
**  Release a resource imported by dsppImportResource().
******************************************************************/

void dsppUnimportResource (const char *name)
{
	TOUCH(name);
}


/******************************************************************
**
**  Export a sharable resource for other instruments to link to
**  by adding to global list.
**
**  Inputs
**
**      dexp - pointer to DSPPExportedResource to fill out.
**             Assumes that caller has pre-initialized dexp (filled w/ 0).
**
**      dins - DSPPInstrument that owns the resource to be exported.
**
**      rsrcindex - index in instrument's DSPPResources[] table to be
**                  exported.
**
**  Results
**
**      Returns 0 on success, error code on failure.
**
**      Fills out dexp.
**
**      Touches dspp_exportedResourceList.
**
******************************************************************/

static Err dsppExportResource (DSPPExportedResource *dexp, DSPPInstrument *dins, int32 rsrcindex)
{
  #if DEBUG_Export
	PRT(("dsppExportResource: dexp=0x%08lx dins=0x%08lx", dexp, dins));
	PRT((" rsrc[%ld] '%s'\n", rsrcindex, DSPPGetRsrcName (dins, rsrcindex)));
  #endif

		/* init DSPPExportedResource */
	dexp->dexp_Node.n_Name   = (char *)DSPPGetRsrcName (dins, rsrcindex);
	dexp->dexp_Owner         = dins;
	dexp->dexp_ResourceIndex = rsrcindex;

/* Make sure name that we are exporting is unique. */
#ifdef BUILD_PARANOIA
	{
		DSPPExportedResource *other_dexp;
		other_dexp = (DSPPExportedResource *)FindNamedNode (&dspp_exportedResourceList, dexp->dexp_Node.n_Name);
		DBUG(("dsppExportResource: check for unique name = %s\n", dexp->dexp_Node.n_Name));
		if( other_dexp != NULL )
		{
			ERR(("dsppExportResource: exported resource already exists = %s\n", dexp->dexp_Node.n_Name ));
			return AF_ERR_NAME_NOT_UNIQUE;
		}
	}
#endif

		/* add to dspp_ExportedResourceList */
	AddTail (&dspp_exportedResourceList, (Node *)dexp);

	return 0;
}


/******************************************************************
**
**  Remove a sharable resource previously exported by
**  dsppExportResource().
**
**  Inputs
**
**      dexp - The exported resource record to remove. Must
**             not be NULL. Assumed to have no outstanding
**             references on it.
**
**             Tolerates dexp not being on the exported list
**             (i.e. dexp_Node.n_Next == NULL). Does nothing
**             in this case.
**
**  Results
**
**      Clears dexp->dexp_Node.n_Next.
**
**      Touches dspp_exportedResourceList.
**
******************************************************************/

static void dsppUnexportResource (DSPPExportedResource *dexp)
{
  #if DEBUG_Export
	PRT(("dsppUnexportResource: dexp=0x%08lx dins=0x%08lx", dexp, dexp->dexp_Owner));
	PRT((" rsrc[%ld] '%s' %s\n", dexp->dexp_ResourceIndex, dexp->dexp_Node.n_Name ? dexp->dexp_Node.n_Name : "<NULL>", dexp->dexp_Node.n_Next ? "" : "(not in list)"));
  #endif

		/* remove it - do nothing if not in list (RemoveAndMarkNode() take care of this) */
	RemoveAndMarkNode ((Node *)dexp);
}

#if DEBUG_Export

/******************************************************************
** debug - dump dspp_exportedResourceList
******************************************************************/

static void dsppDumpExportList (const char *desc)
{
    const DSPPExportedResource *dexp;

    if (desc) PRT(("%s: ", desc));
    PRT(("exported list\n"));

    SCANLIST (&dspp_exportedResourceList, dexp, DSPPExportedResource) {
        const DSPPResource * const drsc = &dexp->dexp_Owner->dins_Resources [ dexp->dexp_ResourceIndex ];

        PRT(("  dins=0x%08lx rsrc[%ld]", dexp->dexp_Owner, dexp->dexp_ResourceIndex));
        PRT((" flags=0x%02lx type=0x%02lx many=0x%04lx", drsc->drsc_Flags, drsc->drsc_Type, drsc->drsc_Many));
        PRT((" alloc=0x%04lx '%s'\n", drsc->drsc_Allocated, dexp->dexp_Node.n_Name));
    }
}

#endif  /* DEBUG_Export */


/* -------------------- Resource consumption / availability queries */

/*****************************************************************/
/**
|||	AUTODOC -public -class audio -group Miscellaneous -name GetAudioResourceInfo
|||	Get information about available audio resources.
|||
|||	  Synopsis
|||
|||	    Err GetAudioResourceInfo (AudioResourceInfo *info, uint32 infoSize,
|||	                              uint32 rsrcType)
|||
|||	  Description
|||
|||	    This function returns availability information of a specified audio
|||	    resource. The information is returned in an AudioResourceInfo structure.
|||
|||	    The AudioResourceInfo structure contains the following fields:
|||
|||	    rinfo_Total
|||	        Indicates the total amount of the resource in the system.
|||
|||	    rinfo_Free
|||	        Indicates the amount of the resource currently available in the system.
|||
|||	    rinfo_LargestFreeSpan
|||	        Indicates the size of the largest contiguous block of the resource. This
|||	        only applies to memory resources (AF_RESOURCE_TYPE_CODE_MEM and
|||	        AF_RESOURCE_TYPE_DATA_MEM). This field is set to 0 for the others.
|||
|||	  Arguments
|||
|||	    info
|||	        A pointer to an AudioResourceInfo structure where the information will
|||	        be stored.
|||
|||	    infoSize
|||	        The size in bytes of the AudioResourceInfo structure.
|||
|||	    rsrcType
|||	        The resource type to query. This must be one of the AF_RESOURCE_TYPE_
|||	        values defined in <audio/audio.h>. See below.
|||
|||	  Resource Types
|||
|||	    AF_RESOURCE_TYPE_TICKS
|||	        Number of DSP ticks / batch (8 frame set) available.
|||
|||	    AF_RESOURCE_TYPE_CODE_MEM
|||	        Words of DSP code memory available.
|||
|||	    AF_RESOURCE_TYPE_DATA_MEM
|||	        Words of DSP data memory available.
|||
|||	    AF_RESOURCE_TYPE_FIFOS
|||	        Number of FIFOs available.
|||
|||	    AF_RESOURCE_TYPE_TRIGGERS
|||	        Number of Triggers available.
|||
|||	  Return Value
|||
|||	    Non-negative if successful or an error code (a negative value) if an error
|||	    occurs. On failure, the contents of info are left unchanged.
|||
|||	  Implementation
|||
|||	    Folio call implemented in audio folio V30.
|||
|||	  Caveats
|||
|||	    The information returned by GetAudioResourceInfo() is inherently flawed,
|||	    since you are existing in a multitasking environment. Resources can be
|||	    allocated or freed asynchronous to the operation of the task calling
|||	    GetAudioResourceInfo().
|||
|||	    Tick amounts returned by this function are in ticks per batch (8 frame set).
|||	    Tick amounts returned by GetInstrumentResourceInfo() are in ticks per frame
|||	    regardless of the instrument's AF_TAG_CALCRATE_DIVIDE setting. So an
|||	    instrument created with full rate execution will reduce the available ticks
|||	    reported by GetAudioResourceInfo() by eight times the amount returned by
|||	    GetInstrumentResourceInfo().
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>
|||
|||	  See Also
|||
|||	    GetInstrumentResourceInfo()
**/
/*
    This function is hardware/api specific, so there's one per
    dspp_resources_xxx.c, but the documentation and syntax is the same for all
    versions.
*/


/******************************************************************
**
**  Totals up resource usage of a specified type for the specified
**  DSPPTemplate. Doesn't include resources used by subroutines.
**
**  Inputs
**      dtmp
**          DSPPTemplate to scan
**
**      rsrcType
**          AF_RESOURCE_TYPE_. Assumed to be valid.
**
**  Results
**      Total resource usage (>=0).
**
******************************************************************/

int32 dsppGetTemplateResourceUsage (const DSPPTemplate *dtmp, int32 rsrcType)
{
	/*
	**  Lookup table to match DRSC_TYPE_ to AF_RESOURCE_TYPE_.
	**      . AF_RESOURCE_TYPE_NONE means no match.
	**      . anything past end also is no match (e.g. all those hardware resources)
	**
	**  @@@ assumes that 8 bits is big enough for AF_RESOURCE_TYPE_,
	**      and that AF_RESOURCE_TYPE_MANY is < 256
	*/
	#define AF_RESOURCE_TYPE_NONE   AF_RESOURCE_TYPE_MANY
	static const uint8 matchRsrcTypes[] = { /* @@@ depends on DRSC_TYPE_ order */
		AF_RESOURCE_TYPE_CODE_MEM,  /* DRSC_TYPE_CODE */
		AF_RESOURCE_TYPE_DATA_MEM,  /* DRSC_TYPE_KNOB */
		AF_RESOURCE_TYPE_DATA_MEM,  /* DRSC_TYPE_VARIABLE */
		AF_RESOURCE_TYPE_NONE,      /* DRSC_TYPE_INPUT */
		AF_RESOURCE_TYPE_DATA_MEM,  /* DRSC_TYPE_OUTPUT */
		AF_RESOURCE_TYPE_FIFOS,     /* DRSC_TYPE_IN_FIFO */
		AF_RESOURCE_TYPE_FIFOS,     /* DRSC_TYPE_OUT_FIFO */
		AF_RESOURCE_TYPE_TICKS,     /* DRSC_TYPE_TICKS */
		AF_RESOURCE_TYPE_DATA_MEM,  /* DRSC_TYPE_RBASE */
		AF_RESOURCE_TYPE_TRIGGERS,  /* DRSC_TYPE_TRIGGER */
	};
	#undef AF_RESOURCE_TYPE_NONE
	int32 sum = 0;
	int32 i;

	for (i=0; i<dtmp->dtmp_NumResources; i++) {
		const DSPPResource * const drsc = &dtmp->dtmp_Resources[i];

		/*
		**  Avoid:
		**      . imported resources - they don't actually belong to this instrument
		**      . bound resources - avoid counting them twice)
		*/
		if (!(drsc->drsc_Flags & (DRSC_F_IMPORT | DRSC_F_BIND)) &&
			drsc->drsc_Type < sizeof matchRsrcTypes / sizeof matchRsrcTypes[0] &&
			matchRsrcTypes[drsc->drsc_Type] == rsrcType) {

			sum += drsc->drsc_Many;
		}
	}

	return sum;
}


/******************************************************************/
/*
	Get InstrumentPortInfo from DSPPResource.

	Arguments
		resultInfo, resultInfoSize,
			InstrumentPortInfo to write results into.

		dtmp
			DSPPTemplate containing resource to query. Assumed to be valid.

		rsrcIndex
			index of resource in DSPPTemplate to query. Assumed to be a valid
			port resource.

	Results
		Fills out resultInfo
*/
void dsppGetInstrumentPortInfo (InstrumentPortInfo *resultInfo, uint32 resultInfoSize, const DSPPTemplate *dtmp, int32 rsrcIndex)
{
	const DSPPResource * const drsc = &dtmp->dtmp_Resources[rsrcIndex];
	InstrumentPortInfo info;

		/* init info */
	memset (&info, 0, sizeof info);

		/* get common stuff */
	strcpy (info.pinfo_Name, dsppGetTemplateRsrcName (dtmp, rsrcIndex));    /* @@@ assumed to be big enough */
	info.pinfo_Type = dsppGetRsrcPortType (drsc->drsc_Type);                /* @@@ assumed to succeed */

		/* get port-type specific stuff */
	switch (info.pinfo_Type) {
		case AF_PORT_TYPE_INPUT:
		case AF_PORT_TYPE_OUTPUT:
		case AF_PORT_TYPE_KNOB:
			info.pinfo_SignalType = drsc->drsc_SubType;
			info.pinfo_NumParts   = drsc->drsc_Many;
			break;
	}

		/* copy to client's buffer */
	memset (resultInfo, 0, resultInfoSize);
	memcpy (resultInfo, &info, MIN (resultInfoSize, sizeof info));
}


/******************************************************************/
/*
	Get InstrumentPortInfo from envelope resource.

	Arguments
		resultInfo, resultInfoSize,
			InstrumentPortInfo to write results into.

		dtmp
			DSPPTemplate containing resource to query. Assumed to be valid.

		deri
			DSPPEnvHookRsrcInfo containing envelope resource indeces. Assumed to be valid.

	Results
		Fills out resultInfo
*/
void dsppGetInstrumentEnvPortInfo (InstrumentPortInfo *resultInfo, uint32 resultInfoSize, const DSPPTemplate *dtmp, const DSPPEnvHookRsrcInfo *deri)
{
	const DSPPResource * const drsc = &dtmp->dtmp_Resources[deri->deri_RequestRsrcIndex];
	const char * const rsrcName = dsppGetTemplateRsrcName (dtmp, deri->deri_RequestRsrcIndex);
	InstrumentPortInfo info;

		/* init info */
	memset (&info, 0, sizeof info);

		/* get info name w/o suffix */
		/* @@@ assumed to be big enough and pre-cleared */
	strncpy (info.pinfo_Name, rsrcName, strlen(rsrcName)-(sizeof ENV_SUFFIX_REQUEST-1));

		/* get other info */
	info.pinfo_Type       = AF_PORT_TYPE_ENVELOPE;
	info.pinfo_SignalType = drsc->drsc_SubType;

		/* copy to client's buffer */
	memset (resultInfo, 0, resultInfoSize);
	memcpy (resultInfo, &info, MIN (resultInfoSize, sizeof info));
}


/******************************************************************/
/*
	Tests whether resource index is the signature resource of an envelope.
	Only one of the 4 envelope resources is considered to be the signature.
	(current .request)

	Arguments
		resultderi
			DSPPEnvHookRsrcInfo to write results into.

		dtmp
			DSPPTemplate containing resource to test.

		rsrcIndex
			Resource index to test. Assumed to be in range for template.

	Results
		TRUE if this is the .request of an envelope, and all the rest of the resources exist.
		In this case, resultderi is filled out.
		FALSE if not an envelope resource. resultderi isn't written to.
*/
bool dsppIsEnvPortRsrc (DSPPEnvHookRsrcInfo *resultderi, const DSPPTemplate *dtmp, int32 rsrcIndex)
{
	if (dtmp->dtmp_Resources[rsrcIndex].drsc_Type == ENV_DRSC_TYPE_REQUEST) {
		const char * const rsrcName = dsppGetTemplateRsrcName (dtmp, rsrcIndex);
		const int32 rsrcNameLen = strlen(rsrcName);
		char envHookName[ENV_MAX_NAME_LENGTH+1];

			/* see if the resource name has the right suffix */
		if (rsrcNameLen >= sizeof ENV_SUFFIX_REQUEST-1 &&
			!strcasecmp (rsrcName+rsrcNameLen-(sizeof ENV_SUFFIX_REQUEST-1), ENV_SUFFIX_REQUEST))
		{
				/* see if the rest of the resources exist */
			strncpy (envHookName, rsrcName, rsrcNameLen-(sizeof ENV_SUFFIX_REQUEST-1));
			envHookName[rsrcNameLen-(sizeof ENV_SUFFIX_REQUEST-1)] = '\0';

			if (dsppFindEnvHookResources (resultderi, sizeof *resultderi, dtmp, envHookName) >= 0) return TRUE;
		}
	}
	return FALSE;
}


/******************************************************************/
/*
	Lookup AF_PORT_TYPE_ which corresponds to a DRSC_TYPE_

	Arguments
		rsrcType
			DRSC_TYPE_ to lookup.

	Results
		Valid AF_PORT_TYPE_ of resource type, or AF_ERR_BADRSRCTYPE if resource
		is not a port.
*/
int32 dsppGetRsrcPortType (uint8 rsrcType)
{
	/*
	**  Translate DRSC_TYPE_ to AF_PORT_TYPE_.
	**      . AF_PORT_TYPE_NONE means resource type is not a port.
	**      . anything past end also is not a port (e.g., all those hardware resources)
	**
	**  @@@ assumes that 8 bits is big enough for AF_PORT_TYPE_,
	**      and that AF_PORT_TYPE_MANY is < 256
	*/
	#define AF_PORT_TYPE_NONE   AF_PORT_TYPE_MANY
	static const uint8 rsrcPortTypes[] = { /* @@@ depends on DRSC_TYPE_ order */
		AF_PORT_TYPE_NONE,      /* DRSC_TYPE_CODE */
		AF_PORT_TYPE_KNOB,      /* DRSC_TYPE_KNOB */
		AF_PORT_TYPE_NONE,      /* DRSC_TYPE_VARIABLE */
		AF_PORT_TYPE_INPUT,     /* DRSC_TYPE_INPUT */
		AF_PORT_TYPE_OUTPUT,    /* DRSC_TYPE_OUTPUT */
		AF_PORT_TYPE_IN_FIFO,   /* DRSC_TYPE_IN_FIFO */
		AF_PORT_TYPE_OUT_FIFO,  /* DRSC_TYPE_OUT_FIFO */
		AF_PORT_TYPE_NONE,      /* DRSC_TYPE_TICKS */
		AF_PORT_TYPE_NONE,      /* DRSC_TYPE_RBASE */
		AF_PORT_TYPE_TRIGGER,   /* DRSC_TYPE_TRIGGER */
	};
	#undef AF_PORT_TYPE_NONE
	uint8 portType;

	return
		rsrcType < sizeof rsrcPortTypes / sizeof rsrcPortTypes[0] &&
		(portType = rsrcPortTypes[rsrcType]) <= AF_PORT_TYPE_MAX
			? portType
			: AF_ERR_BADRSRCTYPE;
}
