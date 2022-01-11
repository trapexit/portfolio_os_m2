/******************************************************************************
**
**  @(#) dspn_info.c 95/12/12 1.11
**  $Id: dspp_instructions.c,v 1.14 1995/03/16 01:26:04 peabody Exp phil $
**
**  DSPP Instruction (machine code) Support.
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
**  950112 WJB  Extracted from dspp_bulldog_remap.c.
**  950112 WJB  Cleaned up slightly.
**  950301 WJB  Replaced dspp.h with dspp_modes.h.
**  950308 WJB  Turned off extra instruction lookup table entries when just remapping.
**  950315 WJB  Removed #ifdefs around code.
**  950315 WJB  Added DSPNTypeMask table arg to dspnGetInstructionInfo().
**  950315 WJB  Made dspnii_Type signed.
**  950503 WJB  Added dspnPack/UnpackImmediate().
**  950803 WJB  Moved dspnPack/UnpackImmediate() to separate modules.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**
******************************************************************************/

#include <dspptouch/dspp_instructions.h>
#include <kernel/types.h>
#include <string.h>                         /* memset() */

#include "dspptouch_internal.h"


/* -------------------- Local functions */

    /* table entry for identifying DSPP opcodes or operands */
static int32 dspnGetType (const DSPNTypeMask *tbl, uint32 tbllen, uint32 code);


/* -------------------- Instruction identification (given opcode) */

static int32 dspnGetInstructionOperandCount (uint32 opcode, uint32 instructiontype);

DSPNInstructionInfo dspnGetInstructionInfo (const DSPNTypeMask *opcodeTable, uint32 opcodeTableLength, uint32 opcode)
{
    DSPNInstructionInfo info;

    memset (&info, 0, sizeof info);
    info.dspnii_CodeWord    = opcode;
    info.dspnii_Type        = dspnGetType (opcodeTable, opcodeTableLength, opcode);
    info.dspnii_NumOperands = dspnGetInstructionOperandCount (opcode, info.dspnii_Type);

    return info;
}

static int32 dspnGetInstructionOperandCount (uint32 opcode, uint32 instructiontype)
{
    switch (instructiontype) {
        case DSPN_OPCODE_ARITH:
            /* arithmetic instructions require parsing in order to determine how many operands they have */
            {
                int32 numops;

                    /* first get NUM_OPS field from n-struction word (this includes result) */
                numops = (opcode & DSPN_ARITH_NUM_OPS_MASK) >> DSPN_ARITH_NUM_OPS_SHIFT;

                    /* if this 0, check to see if multiply requires 2 ops. if so, there are really 4 operands, not 0 */
                if (!numops && (opcode & DSPN_ARITH_F_MULT_SELECT)) numops = 4;

                    /* check barrel shifter mask for mode that gets shift amount from an operand */
                if ((opcode & DSPN_ARITH_BS_MASK) == DSPN_ARITH_BS_SPECIAL) numops++;

                return numops;
            }

        case DSPN_OPCODE_MOVEADDR:
        case DSPN_OPCODE_MOVEREG:
            return 1;   /* all moves have precisely 1 operand after the n-struction */

        default:
            return 0;   /* all others have no operands after the n-struction */
    }
}


/* -------------------- Operand word identification */

static int32 dspnGetOperandCountFromOperandWordType (uint32 operandwordtype);

DSPNInstructionInfo dspnGetOperandWordInfo (uint32 operandword)
{
    static const DSPNTypeMask operandtable[] = {
        { DSPN_OPERAND_IMMED, DSPN_OPERANDMASK_IMMED },
        { DSPN_OPERAND_ADDR,  DSPN_OPERANDMASK_ADDR },
        { DSPN_OPERAND_1_REG, DSPN_OPERANDMASK_1_REG },
        { DSPN_OPERAND_2_REG, DSPN_OPERANDMASK_2_REG },
        { DSPN_OPERAND_3_REG, DSPN_OPERANDMASK_3_REG },
    };
    DSPNInstructionInfo info;

    memset (&info, 0, sizeof info);
    info.dspnii_CodeWord    = operandword;
    info.dspnii_Type        = dspnGetType (operandtable, sizeof operandtable / sizeof operandtable[0], operandword);
    info.dspnii_NumOperands = dspnGetOperandCountFromOperandWordType (info.dspnii_Type);

    return info;
}

static int32 dspnGetOperandCountFromOperandWordType (uint32 operandwordtype)
{
    switch (operandwordtype) {
        case DSPN_OPERAND_2_REG: return 2;
        case DSPN_OPERAND_3_REG: return 3;
        default:            return 1;
    }
}


/* -------------------- Type/Mask lookup */

/* returns value < 0 if not found */
static int32 dspnGetType (const DSPNTypeMask *tbl, uint32 tbllen, uint32 codeword)
{
    for (; tbllen--; tbl++) {
        const uint32 testtype = tbl->dspntm_Type;
        const uint32 testmask = tbl->dspntm_Mask;

        if ((codeword & testmask) == testtype) return testtype;
    }
    return -1;
}
