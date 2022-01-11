/******************************************************************************
**
**  @(#) dspp_relocator.c 96/02/20 1.22
**
**  DSP code relocator.
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
**  950501 WJB  Split out of dspp_loader.c.
**  950501 WJB  Moved dsppRelocateInstrument() from dspp_loader.c.
**  950502 WJB  Added dsppGetResourceRelocatationValue().
**  950503 WJB  Now using dsppPackImmediate().
**  950811 WJB  Removed calls to Read/Write16().
**              Published dsppFixupInstrumentCode().
**              Added support for replacing DRSC_TYPE_KNOB resources w/ a constant.
**  950814 WJB  Added dsppValidateRelocation(). Removed validation from dsppRelocate().
**  950814 WJB  Enhanced dsppValidateRelocation().
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/

#include <dspptouch/dspp_instructions.h>

#include "audio_folio_modes.h"
#include "audio_internal.h"
#include "dspp.h"
#include "dspp_resources.h"     /* RBASE relocation info, dsppGetResourceAttribute() */


/* -------------------- Debug */

#define DEBUG_Validate  0           /* debug dsppValidateRelocation() */
#define DEBUG_Relocate  0           /* debug dsppRelocate() */
#define DBUG(x)         /* PRT(x) */

#if DEBUG_Validate
#undef ERRDBUG
#define ERRDBUG(x) PRT(x)
#endif


/* -------------------- DSPPRelocateInstrument() */

/*****************************************************************
**
**  Do relocation of an instrument in preparation to download
**  it to the DSP. Fixes up instrument's copy of the DSP code
**  to contain addresses computed from allocated DSP resources.
**
**  Inputs
**
**      dins - DSPPInstrument to relocate. All resources must have
**             been successfully allocated.
**
**      Codes - Writable copy of the template's code ready for
**              inline relocation.
**
**  Results
**
**      0 on success, Err code on failure.
**
**  Caveats
**
**      Only supports one code hunk. See dsppRelocate() for complete
**      list of other caveats.
**
*****************************************************************/

Err dsppRelocateInstrument (DSPPInstrument *dins, DSPPCodeHeader *Codes)
{
    const DSPPTemplate * const dtmp = dins->dins_Template;
    uint16 * const codeimage = (uint16 *)dsppGetCodeHunkImage (Codes, 0); /* @@@ assumes single code hunk */
    const DSPPRelocation *drlc = dtmp->dtmp_Relocations;
    int32 numRelocs = dtmp->dtmp_NumRelocations;
    Err errcode;

    for (; numRelocs--; drlc++) {
        const DSPPResource * const drsc = &dins->dins_Resources[drlc->drlc_RsrcIndex];
        int32 rsrcvalue;

        if ((errcode = rsrcvalue = dsppGetResourceRelocationValue (drsc, drlc)) < 0) return errcode;

        dsppRelocate (dtmp, drlc, rsrcvalue, (DSPPFixupFunction)dsppFixupCodeImage, codeimage);
    }

    return 0;
}

/*****************************************************************/
/**
|||	AUTODOC -private -class audio -group DSPPTemplate -name dsppFixupCodeImage
|||	General instrument code image fixup function for relocation.
|||
|||	  Synopsis
|||
|||	    void dsppFixupCodeImage (uint16 *codeImage, uint16 offset, uint16 value)
|||
|||	  Description
|||
|||	    General instrument code image fixup function for relocation. Writes a
|||	    relocated value to the pointed to code image.
|||
|||	    Fits the DSPPFixupFunction typedef.
|||
|||	  Arguments
|||
|||	    codeImage
|||	        Pointer to (partial) code hunk image to fixup.
|||
|||	    offset
|||	        Index of word relative to codeImage to fixup.
|||
|||	    value
|||	        Value to write at codeImage[offset].
|||
|||	  Implementation
|||
|||	    Folio call implemented in audio folio V30.
|||
|||	  Associated Files
|||
|||	    <audio/dspp_template.h>, System.m2/Modules/audio
|||
|||	  See Also
|||
|||	    dsppRelocate()
**/
void dsppFixupCodeImage (uint16 *codeImage, uint16 offset, uint16 value)
{
    codeImage[offset] = value;
  #if DEBUG_Relocate
    PRT(("dsppFixupCode (0x%x, 0x%x, 0x%04x)\n", codeImage, offset, value));
  #endif
}


/*****************************************************************
**
**  Return resource value suitable for passing to dsppRelocate().
**
**  Does some special processing of the allocated resource value
**  to prepare it for placement in code:
**      DRSC_TYPE_KNOB
**          Full ADDR operand for knob address.
**
**      DRSC_TYPE_INPUT
**          IMMED operand of 0.
**
**      DRSC_TYPE_TRIGGER
**          computes IMMED operand for mask of trigger bit
**          (bit number stored in resource).
**
**      Others
**          Just the 10 bits of an address operand.
**
**  Inputs
**
**      drsc - allocated DSPPResource to use as value for relocation.
**
**      drlc - DSPPRelocation record to process.
**
**  Results
**
**      Resource value (>=0) on success, Err code on failure.
**
*****************************************************************/

int32 dsppGetResourceRelocationValue (const DSPPResource *drsc, const DSPPRelocation *drlc)
{
    switch (drsc->drsc_Type) {
        case DRSC_TYPE_KNOB:
            return DSPN_OPERAND_ADDR | (drsc->drsc_Allocated + drlc->drlc_Part);

        case DRSC_TYPE_INPUT:
            return dspnPackImmediate (0);

        case DRSC_TYPE_TRIGGER:
            return dspnPackImmediate ((uint16)1 << drsc->drsc_Allocated);

        default:
            return dsppGetResourceAttribute (drsc, drlc->drlc_Part);
    }
}


/* -------------------- relocator */

typedef struct RelocMasks {
    uint16 rm_FieldMask;
    uint16 rm_LinkMask;
} RelocMasks;

static bool IsRsrcTypeRelocatable (uint8 rsrcType);
static RelocMasks GetRelocMasks (const DSPPResource *);

/*****************************************************************
**
**  Validate relocation record and links in template code image.
**
**  Inputs
**
**      dtmp
**          DSPPTemplate to test
**
**      drlc
**          DSPPRelocation record from dtmp to test
**
**  Results
**
**      Returns >= 0 on success, Err code on failure.
**
**  See Also
**
**      dsppRelocate().
**
**  @@@ Caveats
**
**      . doesn't support multiple code hunks.
**      . assumes code hunk being worked on is of DCOD_RUN_DSPP type.
**
*****************************************************************/

Err dsppValidateRelocation (const DSPPTemplate *dtmp, const DSPPRelocation *drlc)
{
  #if DEBUG_Validate
    printf ("dsppValidateRelocation: ");
    dsppDumpRelocation (drlc);
  #endif

        /* code hunk in range? */
        /* @@@ multiple code hunks are not supported at this time. */
    if (drlc->drlc_CodeHunk != 0) {
        ERRDBUG(("dsppValidateRelocation: drlc_CodeHunk=%d out of range\n", drlc->drlc_CodeHunk));
        return AF_ERR_RELOCATION;
    }

        /* resource index in range? */
    if (drlc->drlc_RsrcIndex >= dtmp->dtmp_NumResources) {
        ERRDBUG(("dsppValidateRelocation: drlc_RsrcIndex=%d out of range\n", drlc->drlc_RsrcIndex));
        return AF_ERR_RELOCATION;
    }

        /* check against resource */
    {
        const DSPPResource * const drsc = &dtmp->dtmp_Resources[drlc->drlc_RsrcIndex];

            /* check resource type */
        if (!IsRsrcTypeRelocatable (drsc->drsc_Type)) {
            ERRDBUG(("dsppValidateRelocation: Invalid rsrc type for relocation = 0x%x\n", drsc->drsc_Type));
            return AF_ERR_RELOCATION;
        }

            /* check part # in range, not for FIFOs !!! are there others? */
        if ((drsc->drsc_Type != DRSC_TYPE_IN_FIFO) &&
            (drsc->drsc_Type != DRSC_TYPE_OUT_FIFO) &&
            (drlc->drlc_Part >= drsc->drsc_Many) ) {
            ERRDBUG(("dsppValidateRelocation: drlc_Part=0x%x out of range \n", drlc->drlc_Part));
            return AF_ERR_RELOCATION;
        }

            /* check relocation links */
        {
            const uint16 *const codeimage = (uint16 *)dsppGetCodeHunkImage (dtmp->dtmp_Codes, 0 /* @@@ drlc->drlc_CodeHunk */ );
            const uint16 codesize         = dtmp->dtmp_Codes [0 /* @@@ drlc->drlc_CodeHunk */ ].dcod_Size;
            const RelocMasks relocMasks   = GetRelocMasks (drsc);
            int32 offset;
            uint16 next;

                /* Scan list and set each entry until 0 link encountered */
            offset = drlc->drlc_CodeOffset;
            do
            {
                    /* bounds check offset */
                if ((offset < 0) || (offset >= codesize))
                {
                    ERRDBUG(("dsppValidateRelocation: Offset outside code bounds=0x%x\n", offset));
                    return AF_ERR_RELOCATION;
                }

                next = (codeimage[offset] & relocMasks.rm_LinkMask);    /* get offset to next location to fixup */
                offset += next;                                         /* Links in code are relative to location. */
            } while (next != 0);
        }
    }

    return 0;
}

/*
    Returns TRUE if resource type can be relocated, FALSE otherwise.
*/
static bool IsRsrcTypeRelocatable (uint8 rsrcType)
{
    return rsrcType != DRSC_TYPE_TICKS;
}


/*****************************************************************/
/**
|||	AUTODOC -private -class audio -group DSPPTemplate -name dsppRelocate
|||	Perform relocation for a DSPPRelocation record.
|||
|||	  Synopsis
|||
|||	    void dsppRelocate (const DSPPTemplate *dtmp, const DSPPRelocation *drlc,
|||	                       uint16 value,
|||	                       DSPPFixupFunction fixupFunc, void *fixupData)
|||
|||	  Description
|||
|||	    Modify code to perform relocation or to change operands. Use info in drlc
|||	    structure to control how modification is made. Allows specific modifications
|||	    to fields in operand or opcode.
|||
|||	  Arguments
|||
|||	    dtmp
|||	        DSPPTemplate for instrument being relocated.
|||
|||	    drlc
|||	        DSPPRelocation record to process.
|||
|||	    value
|||	        Code word to store (e.g. operand, immediate).
|||
|||	    fixupFunc, fixupData
|||	        Client supplied function and data to perform fixup (see below for
|||	        details). This function may do an actual write to instrument code or can
|||	        be used for any other process (e.g. gathering information about which
|||	        words contain relocations).
|||
|||	  Implementation
|||
|||	    Folio call implemented in audio folio V30.
|||
|||	  Callback Synopsis -preformatted
|||
|||	    typedef void (*DSPPFixupFunction) (void *fixupData, int32 offset, int32 value)
|||
|||	  Callback Arguments
|||
|||	    fixupData
|||	        Client data passed to dsppRelocate().
|||
|||	    offset
|||	        Word offset in code hunk (@@@ assumes there's only one code hunk)
|||
|||	    value
|||	        To be placed into this location. This value has been cooked and
|||	        suitably overlayed with the source code image.
|||
|||	  Caveats
|||
|||	    Doesn't support multiple code hunks. The previous version more or less did,
|||	    but not all the clients of this function actually called this function
|||	    correctly in order to make use of multiple code hunks.
|||
|||	    Assumes code hunk being worked on is of DCOD_RUN_DSPP type.
|||
|||	  Associated Files
|||
|||	    <audio/dspp_template.h>, System.m2/Modules/audio
|||
|||	  See Also
|||
|||	    dsppFixupCodeImage()
**/
/*
**  See Also
**
**      dsppValidateRelocation, dsppRelocateInstrument(), other
**      clients of dsppRelocate().
*/

void dsppRelocate (const DSPPTemplate *dtmp, const DSPPRelocation *drlc, uint16 value, DSPPFixupFunction fixupFunc, void *fixupData)
{
    const uint16 *const codeimage = (uint16 *)dsppGetCodeHunkImage (dtmp->dtmp_Codes, 0 /* @@@ drlc->drlc_CodeHunk */ );
    const RelocMasks relocMasks   = GetRelocMasks (&dtmp->dtmp_Resources[drlc->drlc_RsrcIndex]);

    DBUG(("DSPPRelocate( 0x%x, 0x%x ", drlc, value));
    DBUG((", 0x%x, 0x%x)\n", fixupFunc, fixupData));
    DBUG(("dtmp->dtmp_Codes = 0x%x, size = 0x%x\n", dtmp->dtmp_Codes, dtmp->dtmp_Codes->dcod_Size ));

        /* Mask off relevant part of value */
    value &= relocMasks.rm_FieldMask;

  #if DEBUG_Relocate
    printf ("dsppRelocate: ");
    dsppDumpRelocation (drlc);
    printf ("    linkmask=$%04lx fieldmask=$%04lx value=$%04lx '%s'\n", relocMasks.rm_LinkMask, relocMasks.rm_FieldMask, value, dsppGetTemplateRsrcName (dtmp,drlc->drlc_RsrcIndex));
  #endif

    {
        uint16 offset, next, pad;

            /* Scan list and set each entry until 0 link encountered */
        offset = drlc->drlc_CodeOffset;
        do
        {
            pad = codeimage[offset];                /* Get sized value from code image, place in pad. */
            next = (pad & relocMasks.rm_LinkMask);  /* get offset to next location to fixup */
            fixupFunc (fixupData, offset, (pad & ~relocMasks.rm_FieldMask) | value);

            offset += next;                         /* Links in code are relative to location. */
        } while (next != 0);
    }
}

/*
    Generate mask pair for walking relocation links and doing fixups.
*/
static RelocMasks GetRelocMasks (const DSPPResource *drsc)
{
    RelocMasks relocMasks;

        /* Generate masks based on resource type. */
        /* @@@ these guys must completely initialize relocMasks because it has no other initializer */
    switch (drsc->drsc_Type) {
        case DRSC_TYPE_KNOB:
        case DRSC_TYPE_INPUT:
        case DRSC_TYPE_TRIGGER:
            relocMasks.rm_FieldMask = ~0;
            relocMasks.rm_LinkMask  = DSPN_ADDR_MASK;
            break;

        case DRSC_TYPE_RBASE:
            relocMasks.rm_FieldMask = DSPN_BULLDOG_RBASE_ADDR_MASK;
            relocMasks.rm_LinkMask  = 0;
            break;

        default:
            relocMasks.rm_FieldMask = relocMasks.rm_LinkMask = DSPN_ADDR_MASK;
            break;
    }

    return relocMasks;
}


/* -------------------- old method */

#if 0       /* { @@@ 950118: method used before absolute address remapping.
                           note that handles multiple code hunks and almost handles 32-bit indexing */

#define GETPAD(indx) if (if32) { pad = longs[indx]; } \
            else { pad = Read16(&shorts[indx]); }

int32 DSPPRelocate( DSPPRelocation *drlc, int32 Value,
    DSPPCodeHeader *Codes, void (*PutFunc)())
{
    uint32  *longs, pad, old, mask, next;
    int32 if32, CodeSize, shft;
    uchar *CodePtr;
    uint16 *shorts;
    int32 Offset, Result = 0;


TRACEE(TRACE_INT,TRACE_OFX,("DSPPRelocate( 0x%x, 0x%x ", drlc, Value));
TRACEE(TRACE_INT,TRACE_OFX,(", 0x%x, 0x%x)\n", Codes, PutFunc));

/* Get pointer to code fragment needed for this relocation. */
    CodePtr = dsppGetCodeHunkImage(Codes, drlc->drlc_CodeHunk);
    shorts = (uint16 *) CodePtr;
    longs = (uint32 *) CodePtr;

    CodeSize = Codes[drlc->drlc_CodeHunk].dcod_Size;
TRACEB(TRACE_INT,TRACE_OFX,("DSPPRelocate: CodeSize = 0x%x\n", CodeSize));

    if32 = (int32) drlc->drlc_Flags & DRLC_32;

/* Calculate shift and mask based on description of field to be modified. */
    shft = drlc->drlc_Bit;
    mask = ((uint32)(0xFFFFFFFF >> (32L-(int32)drlc->drlc_Width))) << shft;

TRACEB(TRACE_INT,TRACE_OFX,("DSPPRelocate: if32 = %d, shft = %d, mask = $%x\n",
         if32, shft, mask));

/* Check mode of relocation.  This mode is used for relocating branches. */
    if (drlc->drlc_Flags & DRLC_ADD)
    {
        if (drlc->drlc_Flags & DRLC_LIST)
        {
             Result = AF_ERR_RELOCATION;  /* Can't do both. */
             goto error;
        }
/* Get word from target to be relocated as either 16 or 32 bit */
        Offset = drlc->drlc_CodeOffset;
        if ((Offset < 0) || (Offset >= CodeSize))
        {
            ERRDBUG(("DSPPRelocate: Offset outside code bounds = 0x%x\n", Offset));
            Result = AF_ERR_RELOCATION;
            goto error;
        }
        else
        {
            GETPAD(Offset);   /* Get sized value from code image, place in pad. */
            old = pad & mask;
            /* Build new code by adding shifted value. */
            pad = (pad & ~mask) | ((Value << shft) + old);
            PutFunc(pad, Offset, CodePtr);
        }
    }
    else
    { /* set directly */
        if (drlc->drlc_Flags & DRLC_LIST)
        {
/* Scan list and set each entry until 0 link encountered */
            Offset = drlc->drlc_CodeOffset;
            do
            {
                if ((Offset < 0) || (Offset >= CodeSize))
                {
                    ERRDBUG(("DSPPRelocate: Offset outside code bounds = 0x%x\n", Offset));
                    Result = AF_ERR_RELOCATION;
                    goto error;
                }
                else
                {
                    GETPAD(Offset);
TRACEB(TRACE_INT,TRACE_OFX,("DSPPRelocate: LIST Code[$%x] = $%x\n", Offset, pad))
                    next = (pad & mask) >> shft;   /* get link */
                    pad = (pad & ~mask) | (Value << shft);
                    PutFunc(pad, Offset, CodePtr);
                }
                Offset = next;
            } while (next != 0);
        }
        else
        {
            Offset = drlc->drlc_CodeOffset;
            if ((Offset < 0) || (Offset >= CodeSize))
            {
                ERRDBUG(("DSPPRelocate: Offset outside code bounds = 0x%x\n", Offset));
                Result = AF_ERR_RELOCATION;
            }
            else
            {
                GETPAD(Offset);
TRACEB(TRACE_INT,TRACE_OFX,("DSPPRelocate: Code[$%x] = $%x\n", Offset, pad))
                pad = (pad & ~mask) | (Value << shft);
                PutFunc(pad, Offset, CodePtr);
            }
        }
    }
error:
TRACER(TRACE_INT,TRACE_OFX,("DSPPRelocate: returns %x\n", Result))
    return Result;
}

#undef GETPAD

#endif      /* } old method */
