/******************************************************************************
**
**  @(#) dspp_remap.c 95/09/22 1.12
**  $Id: dspp_remap.c,v 1.19 1995/03/16 22:17:34 peabody Exp phil $
**
**  ASIC-independent portion of DSPP absolute address remapping support.
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
**  950130 WJB  Created.
**  950131 WJB  Added SiliconVersion test to enable remapping to bulldog.
**  950301 WJB  Added dspp_modes.h.
**  950310 WJB  Now using dspp_remap_internal.h.
**  950310 WJB  Added use of DSPP_REMAP_ENABLE.
**  950313 WJB  Commonized a bunch of stuff.
**  950313 WJB  Cleaned up.
**  950314 WJB  Replaced DSPP_REMAP_ENABLE with DSPP_PROCABS_MODE.
**  950315 WJB  Added dsppWhetherRemapTemplate().
**  950315 WJB  Tidied up slightly.
**  950315 WJB  Added DSPPTemplate arg to dsppRemapAddress().
**  950315 WJB  Added local opcode table for new dspnGetInstructionInfo() API.
**  950316 WJB  Moved dsppRemapAbsoluteAddresses() mode-specific behavior table to dspp_remap.h.
**  950316 WJB  Replaced EZMemAlloc/Free() w/ SuperMemAlloc/Free() + MEMTYPE_TRACKSIZE.
**  950412 WJB  Commented out a couple of consts to avoid diab compiler bug.
**  950425 WJB  Took out workarounds for diab const initialization bug.
**  950531 WJB  Replaced byte-array relocation marker system with bit-array system.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**
******************************************************************************/

#include <dspptouch/dspp_instructions.h>    /* DSPP code parsing support */
#include <dspptouch/dspp_touch.h>           /* Read/Write16() */
#include <kernel/bitarray.h>

#include "audio_folio_modes.h"      /* AF_ mode defines */
#include "audio_internal.h"         /* Read16() */
#include "dspp.h"                   /* DSPP structures */
#include "dspp_remap_internal.h"    /* self */
#include "ezmem_tools.h"


#if DSPP_PROCABS_MODE  /* { */

/* -------------------- Conditional */

    /* debugging */
#define DEBUG_Remap 1       /* function entry and remap hits */     /* !!! turn off before release */
#define DEBUG_Reloc 0       /* relocation info */


/* -------------------- Local functions */

    /* relocation stuff */
static Err CreateRelocMarkers (uint32 **resultRelocMarkers, const DSPPTemplate *);
#define DeleteRelocMarkers(relocmarkers) SuperMemFree (relocmarkers, -1)
#define IsWordMarked(relocmarkers,codeindex) IsBitSet (relocmarkers, codeindex)


/* -------------------- dsppRemapAbsoluteAddresses() */

static Err dsppProcessAbsolutes (DSPPTemplate *);

#if DSPP_PROCABS_MODE == DSPP_PROCABS_MODE_REMAP

    static Err dsppRemapAbsolute (const DSPPTemplate *, uint16 *codeptr);
    #define dsppProcessAbsolute dsppRemapAbsolute

#elif DSPP_PROCABS_MODE == DSPP_PROCABS_MODE_TRAP

    static Err dsppTrapAbsolute (const DSPPTemplate *, uint16 *codeptr);
    #define dsppProcessAbsolute dsppTrapAbsolute

#endif


/******************************************************************
**
**  Remap absolute addresses in DSPP code for DSPPTemplate from
**  instrument's ASIC address space to native space for whichever
**  version of the audio folio this is.
**
**  Does nothing for trivial cases (i.e. native dsp instruments
**  and system instruments).
**
**  The method of finding absolutes involves disassembling the code
**  to locate all addresses, and avoiding those that are to be
**  relocated.
**
**  This function has mode-dependent behavior. See dspp_remap.h for
**  current behavior.
**
**  @@@ Only supports single code hunk, which must be of type
**      DCOD_RUN_DSPP. Look for direct refs to dtmp_Codes[0]
**      and usage of dsppGetCodeHunkImage (dtmp->dtmp_Codes,0).
**
**  @@@ On failure, the DSPPTemplate's code may have become corrupted.
**
******************************************************************/

Err dsppRemapAbsoluteAddresses (DSPPTemplate *dtmp)
{
        /* ignore privileged templates (e.g. head, tail) */
    if (dtmp->dtmp_Header.dhdr_Flags & DHDR_F_PRIVILEGED) return 0;

  #if DSPP_PROCABS_MODE == DSPP_PROCABS_MODE_REMAP
        /* see if target ASIC remapper needs to remap this template */
    if (!dsppWhetherRemapTemplate (dtmp)) return 0;
  #endif

        /* otherwise, process absolutes */
    return dsppProcessAbsolutes (dtmp);
}


/******************************************************************
**  Remap all absolutes in place in template's code
******************************************************************/

static Err dsppProcessAbsolutes (DSPPTemplate *dtmp)
{
    uint32 *relocmarkers = NULL;
    Err errcode = 0;

  #if DEBUG_Remap
    PRT(("dsppProcessAbsolutes[%s]:", DSPP_PROCABS_MODE == DSPP_PROCABS_MODE_REMAP ? "Remap" : "Trap"));
    PRT((" dtmp: $%08lx  code: $%08lx, $%04lx words", dtmp, dsppGetCodeHunkImage (dtmp->dtmp_Codes,0), dtmp->dtmp_Codes[0].dcod_Size));
    PRT((" Silicon: %d\n", dtmp->dtmp_Header.dhdr_SiliconVersion));
  #endif

        /* locate code words that are to be relocated */
    if ((errcode = CreateRelocMarkers (&relocmarkers, dtmp)) < 0) goto clean;

  #if DEBUG_Reloc
    {
        const int32 size = GetMemTrackSize (relocmarkers) * 8;  /* @@@ assumes that mem was allocated with MEMTYPE_TRACKSIZE, also a bit of a hack */
        int32 i;
        int hits = 0;

        DumpBitRange (relocmarkers, 0, size-1, "  relocmarkers");
        PRT(("  relocations:"));
        for (i=0; i<size; i++) if (IsWordMarked(relocmarkers,i)) {
            if (hits && !(hits % 8)) PRT(("\n              "));
            PRT((" $%03lx", i));
            hits++;
        }
        PRT(("\n"));
    }
  #endif

        /* do remapping given knowledge of relocation words */
    {
        uint16 * const codeimage = (uint16 *)dsppGetCodeHunkImage (dtmp->dtmp_Codes,0);
        const uint32 codesize = dtmp->dtmp_Codes[0].dcod_Size;
        uint32 codeindex;

        for (codeindex = 0; codeindex < codesize;) {
            static const DSPNTypeMask opcodeTable[] = {
                    /* these are the required opcodes for simply parsing DSPP code */
                    /* they also happen to be the minimum set of instructions required for remapping */
                { DSPN_OPCODE_ARITH,        DSPN_OPCODEMASK_ARITH },
                { DSPN_OPCODE_MOVEREG,      DSPN_OPCODEMASK_MOVEREG },
                { DSPN_OPCODE_MOVEADDR,     DSPN_OPCODEMASK_MOVEADDR },
            };
            const DSPNInstructionInfo instructionInfo = dspnGetInstructionInfo (opcodeTable, sizeof opcodeTable / sizeof opcodeTable[0], Read16 (&codeimage[codeindex]));

                /* remap address in opcode, if there is one */
            if (!IsWordMarked (relocmarkers,codeindex)) switch (instructionInfo.dspnii_Type) {
                case DSPN_OPCODE_MOVEADDR:
                    if ((errcode = dsppProcessAbsolute (dtmp, &codeimage[codeindex])) < 0) goto clean;
                    break;
            }
            codeindex++;

                /* process operand words */
            {
                int32 opindex;

                for (opindex=0; opindex < instructionInfo.dspnii_NumOperands;) {
                        /* premature end of code hunk? */
                    if (codeindex >= codesize) {
                        errcode = AF_ERR_BAD_DSP_CODE;
                        goto clean;
                    }

                        /* remap address in operand word */
                    {
                        const DSPNInstructionInfo operandWordInfo = dspnGetOperandWordInfo (Read16 (&codeimage[codeindex]));

                        if (!IsWordMarked (relocmarkers,codeindex)) switch (operandWordInfo.dspnii_Type) {
                            case DSPN_OPERAND_ADDR:
                                if ((errcode = dsppProcessAbsolute (dtmp, &codeimage[codeindex])) < 0) goto clean;
                                break;
                        }
                        opindex += operandWordInfo.dspnii_NumOperands;
                    }
                    codeindex++;
                }
            }
        }
    }

clean:
    DeleteRelocMarkers (relocmarkers);
    return errcode;
}


#if DSPP_PROCABS_MODE == DSPP_PROCABS_MODE_REMAP /* { */

/******************************************************************
**  Remap an address in place in pointed to code word
******************************************************************/

static Err dsppRemapAbsolute (const DSPPTemplate *dtmp, uint16 *codeptr)
{
    const uint32 codeword = Read16 (codeptr);
    const int32 fromaddr  = codeword & DSPN_ADDR_MASK;
    const int32 toaddr    = dsppRemapAddress (dtmp, fromaddr);

    if (toaddr < 0) return toaddr;

  #if DEBUG_Remap
    PRT(("  $%08lx: Remapping $%03lx -> $%03lx\n", codeptr, fromaddr, toaddr));
  #endif
    Write16 (codeptr, codeword & ~DSPN_ADDR_MASK | toaddr);     /* @@@ assumes toaddr has no set bits outside of DSPN_ADDR_MASK */

    return 0;
}

#endif   /* } */
#if DSPP_PROCABS_MODE == DSPP_PROCABS_MODE_TRAP /* { */

/******************************************************************
**
**  Trap absolute - simply return error code.
**
******************************************************************/

static Err dsppTrapAbsolute (const DSPPTemplate *dtmp, uint16 *codeptr)
{
    const uint32 codeword = Read16 (codeptr);
    const int32 fromaddr  = codeword & DSPN_ADDR_MASK;

    ERR(("dsppTrapAbsolute: Invalid use of DSP absolute address $%03lx.\n", fromaddr));
    return AF_ERR_BAD_DSP_ABSOLUTE;
}

#endif   /* } */


/* -------------------- Relocation Marker Stuff */
/* @@@ could extend this system to support multiple code hunks merely by making an array of relocmarker arrays */

#define RelocMarkerSize(nthings) (((nthings) + BITSPERUINT32 - 1) / BITSPERUINT32 * sizeof (uint32))

static void MarkRelocWord (uint32 *relocmarkers, int32 offset, int32 value);

static Err CreateRelocMarkers (uint32 **resultRelocMarkers, const DSPPTemplate *dtmp)
{
    const uint32 codesize = dtmp->dtmp_Codes[0].dcod_Size;
    uint32 *relocmarkers = NULL;
    Err errcode = 0;

  #if DEBUG_Reloc
    PRT(("CreateRelocMarkers() codesize=%ld relocmarkersize=%ld\n", codesize, RelocMarkerSize(codesize)));
  #endif

        /* initialize result */
    *resultRelocMarkers = NULL;

        /* allocate RelocMarkers */
    if ((relocmarkers = (uint32 *)SuperMemAlloc (RelocMarkerSize(codesize), MEMTYPE_ANY | MEMTYPE_TRACKSIZE | MEMTYPE_FILL)) == NULL) {
        errcode = AF_ERR_NOMEM;
        goto clean;
    }

        /*
            Call dsppRelocate() on each relocation record to find out where relocations will occur.
            Doesn't actually perform relocations on code.
        */
    {
        int32 i;

        for (i=0; i<dtmp->dtmp_NumRelocations; i++)
        {
            dsppRelocate (dtmp, &dtmp->dtmp_Relocations[i], 0, (DSPPFixupFunction)MarkRelocWord, relocmarkers);
        }
    }

        /* set result on success */
    *resultRelocMarkers = relocmarkers;

    return 0;

clean:
    DeleteRelocMarkers (relocmarkers);
    return errcode;
}

static void MarkRelocWord (uint32 *relocmarkers, uint16 offset, uint16 value)
{
    SetBitRange (relocmarkers, offset, offset);
}


#endif   /* } DSPP_PROCABS_MODE */
