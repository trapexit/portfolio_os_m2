/******************************************************************************
**
**  @(#) dspp_disasm.c 96/01/16 1.19
**  $Id: dspp_disasm.c,v 1.9 1995/03/16 01:25:42 peabody Exp phil $
**
**  DSPP Disassembler.
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
**  950113 WJB  Extracted from dspp_bulldog_remap.c.
**  950123 WJB  Added description text to dspnDisassemble().
**  950307 WJB  Added argument decoding for RBASE instructions.
**  950315 WJB  Added local opcode table for new dspnGetInstructionInfo() API.
**  950315 WJB  Made mne_Type signed.
**  950412 WJB  Working around a diab const initialization bug (see diab compiler bug).
**  950412 WJB  Fixed up error code usage.
**  950413 WJB  Added more detail to DSPN_OPCODE_BULLDOG_RBASE printout.
**  950424 WJB  Took out workarounds for diab const initialization bug.
**  950503 WJB  Now using dspnUnpackImmediate().
**  950726 WJB  Improved ARITH instruction output.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**
******************************************************************************/

#include <dspptouch/dspp_instructions.h>
#include <stdio.h>                          /* sprintf() */

#include "dspptouch_internal.h"             /* PRT() */


/* -------------------- Opcode table */

static const DSPNTypeMask dsm_opcodeTable[] = {
    { DSPN_OPCODE_ARITH,            DSPN_OPCODEMASK_ARITH },
    { DSPN_OPCODE_MOVEREG,          DSPN_OPCODEMASK_MOVEREG },
    { DSPN_OPCODE_MOVEADDR,         DSPN_OPCODEMASK_MOVEADDR },

    { DSPN_OPCODE_JUMP,             DSPN_OPCODEMASK_BRANCH },
    { DSPN_OPCODE_JSR,              DSPN_OPCODEMASK_BRANCH },
    { DSPN_OPCODE_BFM,              DSPN_OPCODEMASK_BRANCH },
    { DSPN_OPCODE_BVS,              DSPN_OPCODEMASK_BRANCH },
    { DSPN_OPCODE_BMI,              DSPN_OPCODEMASK_BRANCH },
    { DSPN_OPCODE_BRB,              DSPN_OPCODEMASK_BRANCH },
    { DSPN_OPCODE_BEQ,              DSPN_OPCODEMASK_BRANCH },
    { DSPN_OPCODE_BCS,              DSPN_OPCODEMASK_BRANCH },
    { DSPN_OPCODE_BOP,              DSPN_OPCODEMASK_BRANCH },
    { DSPN_OPCODE_BVC,              DSPN_OPCODEMASK_BRANCH },
    { DSPN_OPCODE_BPL,              DSPN_OPCODEMASK_BRANCH },
    { DSPN_OPCODE_BRP,              DSPN_OPCODEMASK_BRANCH },
    { DSPN_OPCODE_BNE,              DSPN_OPCODEMASK_BRANCH },
    { DSPN_OPCODE_BCC,              DSPN_OPCODEMASK_BRANCH },
    { DSPN_OPCODE_BLT,              DSPN_OPCODEMASK_BRANCH },
    { DSPN_OPCODE_BLE,              DSPN_OPCODEMASK_BRANCH },
    { DSPN_OPCODE_BGE,              DSPN_OPCODEMASK_BRANCH },
    { DSPN_OPCODE_BGT,              DSPN_OPCODEMASK_BRANCH },
    { DSPN_OPCODE_BHI,              DSPN_OPCODEMASK_BRANCH },
    { DSPN_OPCODE_BLS,              DSPN_OPCODEMASK_BRANCH },
    { DSPN_OPCODE_BXS,              DSPN_OPCODEMASK_BRANCH },
    { DSPN_OPCODE_BXC,              DSPN_OPCODEMASK_BRANCH },

    { DSPN_OPCODE_BULLDOG_RBASE,    DSPN_OPCODEMASK_CTRL },

    { DSPN_OPCODE_NOP,              DSPN_OPCODEMASK_SUPER_SPECIAL },
    { DSPN_OPCODE_BAC,              DSPN_OPCODEMASK_SUPER_SPECIAL },
    { DSPN_OPCODE_ANVIL_RBASE,      DSPN_OPCODEMASK_SUPER_SPECIAL },
    { DSPN_OPCODE_ANVIL_RMAP,       DSPN_OPCODEMASK_SUPER_SPECIAL },
    { DSPN_OPCODE_RTS,              DSPN_OPCODEMASK_SUPER_SPECIAL },
    { DSPN_OPCODE_OP_MASK,          DSPN_OPCODEMASK_SUPER_SPECIAL },
    { DSPN_OPCODE_SLEEP,            DSPN_OPCODEMASK_SUPER_SPECIAL },
};
#define DSM_OPCODE_TABLE_LENGTH (sizeof dsm_opcodeTable / sizeof dsm_opcodeTable[0])


/* -------------------- DSPP Disassembler */

typedef struct DSMTypeMnemonic {
    int32       mne_Type;
    const char *mne_Mnemonic;
} DSMTypeMnemonic;

    /* print an opcode */
static void dsmPrintOpcode (uint32 codeaddr, const DSPNInstructionInfo instructionInfo);

    /* print an operand word */
static void dsmPrintOperandWord (const DSPNInstructionInfo operandWordInfo);
static void dsmPrintReg (uint32 codeword, uint32 regfieldnum, bool writeback);
static void dsmPrintAddr (uint32 codeword, bool writeback);
static void dsmPrintOperand (const char *opstr, bool indirect, bool writeback);
static const char *dsmGetMnemonic (const DSMTypeMnemonic *tbl, uint32 tbllen, int32 type);

int32 dspnDisassemble (const uint16 *codeImage, uint32 codeBase, int32 codeSize, const char *descfmt, ...)
{
    const uint16 * codeptr = codeImage;
    const uint16 * const codeend = codeImage + codeSize;

    PRT(("\n"));

    if (descfmt) {
        char buf[128];
        va_list vargs;

        va_start (vargs, descfmt);

        vsprintf (buf, descfmt, vargs);
        PRT(("%s: ", buf));

        va_end (vargs);
    }

    PRT(("dspp disasm: base=$%03lx size=$%03lx words\n", codeBase, codeSize));

    while (codeptr < codeend) {
        const DSPNInstructionInfo instructionInfo = dspnGetInstructionInfo (dsm_opcodeTable, DSM_OPCODE_TABLE_LENGTH, *codeptr );

            /* print opcode */
        dsmPrintOpcode (codeptr - codeImage + codeBase, instructionInfo);
        codeptr++;

            /* print operands */
        {
            int32 opindex;

            for (opindex=0; opindex < instructionInfo.dspnii_NumOperands;) {
                if (codeptr >= codeend) return -1;

                    /* print operand word */
                {
                    const DSPNInstructionInfo operandWordInfo = dspnGetOperandWordInfo ( *codeptr );

                    dsmPrintOperandWord (operandWordInfo);
                    opindex += operandWordInfo.dspnii_NumOperands;
                }

                codeptr++;
            }
        }
    }

    return 0;
}

static void dsmPrintOpcode (uint32 codeaddr, const DSPNInstructionInfo instructionInfo)
{
    static const DSMTypeMnemonic opcodeMnemonics[] = {
        { DSPN_OPCODE_ANVIL_RBASE,      "RBASE[Opera]" },
        { DSPN_OPCODE_ANVIL_RMAP,       "RMAP[Opera]" },
        { DSPN_OPCODE_ARITH,            "ARITH"   },
        { DSPN_OPCODE_BAC,              "BAC"     },
        { DSPN_OPCODE_BCC,              "BCC"     },
        { DSPN_OPCODE_BCS,              "BCS"     },
        { DSPN_OPCODE_BEQ,              "BEQ"     },
        { DSPN_OPCODE_BFM,              "BFM"     },
        { DSPN_OPCODE_BGE,              "BGE"     },
        { DSPN_OPCODE_BGT,              "BGT"     },
        { DSPN_OPCODE_BHI,              "BHI"     },
        { DSPN_OPCODE_BLE,              "BLE"     },
        { DSPN_OPCODE_BLS,              "BLS"     },
        { DSPN_OPCODE_BLT,              "BLT"     },
        { DSPN_OPCODE_BMI,              "BMI"     },
        { DSPN_OPCODE_BNE,              "BNE"     },
        { DSPN_OPCODE_BOP,              "BOP"     },
        { DSPN_OPCODE_BPL,              "BPL"     },
        { DSPN_OPCODE_BRB,              "BRB"     },
        { DSPN_OPCODE_BRP,              "BRP"     },
        { DSPN_OPCODE_BULLDOG_RBASE,    "RBASE[M2]" },
        { DSPN_OPCODE_BVC,              "BVC"     },
        { DSPN_OPCODE_BVS,              "BVS"     },
        { DSPN_OPCODE_BXC,              "BXC"     },
        { DSPN_OPCODE_BXS,              "BXS"     },
        { DSPN_OPCODE_JSR,              "JSR"     },
        { DSPN_OPCODE_JUMP,             "JUMP"    },
        { DSPN_OPCODE_MOVEADDR,         "MOVE"    },
        { DSPN_OPCODE_MOVEREG,          "MOVE"    },
        { DSPN_OPCODE_NOP,              "NOP"     },
        { DSPN_OPCODE_OP_MASK,          "OP_MASK" },
        { DSPN_OPCODE_RTS,              "RTS"     },
        { DSPN_OPCODE_SLEEP,            "SLEEP"   },
    };
    const char * const mnemonic = dsmGetMnemonic (opcodeMnemonics, sizeof opcodeMnemonics / sizeof opcodeMnemonics[0], instructionInfo.dspnii_Type);

    PRT(("  0x%04lx: 0x%04lx  %s ", codeaddr, instructionInfo.dspnii_CodeWord, mnemonic ? mnemonic : "(unknown)"));

    switch (instructionInfo.dspnii_Type) {
        case DSPN_OPCODE_ARITH:
            PRT(("(%ld ops): ", instructionInfo.dspnii_NumOperands));
            {
                const uint8 muxa = (instructionInfo.dspnii_CodeWord & DSPN_ARITH_MUX_A_MASK) >> DSPN_ARITH_MUX_A_SHIFT;
                const uint8 muxb = (instructionInfo.dspnii_CodeWord & DSPN_ARITH_MUX_B_MASK) >> DSPN_ARITH_MUX_B_SHIFT;
                const uint8 alu  = (instructionInfo.dspnii_CodeWord & DSPN_ARITH_ALU_MASK) >> DSPN_ARITH_ALU_SHIFT;
                const uint8 bs   = instructionInfo.dspnii_CodeWord & DSPN_ARITH_BS_MASK;

                    /* @@@ not a complete set, but adequate */
                if (muxa != DSPN_ARITH_MUX_MULT || muxb != DSPN_ARITH_MUX_MULT) {
                    static const char * const opdesc[] = { "Op1", "Op2", "Op3", "Op4", "Op5", "Op6" };
                    const bool use_mult = (muxa == DSPN_ARITH_MUX_MULT || muxb == DSPN_ARITH_MUX_MULT);
                    const uint8 nmult_ops = use_mult ? ( (instructionInfo.dspnii_CodeWord & DSPN_ARITH_F_MULT_SELECT) ? 2 : 1 )
                                                     : 0;
                    const uint8 nalu_ops  = (muxa == DSPN_ARITH_MUX_OP2 || muxb == DSPN_ARITH_MUX_OP2) ? 2
                                          : (muxa == DSPN_ARITH_MUX_OP1 || muxb == DSPN_ARITH_MUX_OP1) ? 1
                                          : 0;
                    const uint8 nbs_ops   = (bs == DSPN_ARITH_BS_SPECIAL) ? 1 : 0;
                    const char * const mult_desc = nmult_ops == 2 ? "Op1 Op2"
                                                 : nmult_ops == 1 ? "Op1 ACCUME"
                                                 : "";
                    const char *alu_opa_desc, *alu_opb_desc;

                    switch (muxa) {
                        default:
                        case DSPN_ARITH_MUX_ACCUM:  alu_opa_desc = "ACCUME";              break;
                        case DSPN_ARITH_MUX_OP1:    alu_opa_desc = opdesc[nmult_ops];     break;
                        case DSPN_ARITH_MUX_OP2:    alu_opa_desc = opdesc[nmult_ops+1];   break;
                        case DSPN_ARITH_MUX_MULT:   alu_opa_desc = mult_desc;             break;
                    }
                    switch (muxb) {
                        default:
                        case DSPN_ARITH_MUX_ACCUM:  alu_opb_desc = "ACCUME";              break;
                        case DSPN_ARITH_MUX_OP1:    alu_opb_desc = opdesc[nmult_ops];     break;
                        case DSPN_ARITH_MUX_OP2:    alu_opb_desc = opdesc[nmult_ops+1];   break;
                        case DSPN_ARITH_MUX_MULT:   alu_opb_desc = mult_desc;             break;
                    }

                    if (instructionInfo.dspnii_NumOperands > (nmult_ops + nalu_ops + nbs_ops)) {
                        PRT(("%s _= ", opdesc[instructionInfo.dspnii_NumOperands-1]));
                    }

                    switch (alu) {
                        case DSPN_ARITH_ALU_TRA:
                        case DSPN_ARITH_ALU_ADDC:
                        case DSPN_ARITH_ALU_SUBB:
                        case DSPN_ARITH_ALU_INC:
                        case DSPN_ARITH_ALU_DEC:
                        case DSPN_ARITH_ALU_TRL:
                        case DSPN_ARITH_ALU_NOT:
                            PRT(("%s ", alu_opa_desc));
                            break;

                        case DSPN_ARITH_ALU_NEG:
                            PRT(("%s ", alu_opb_desc));
                            break;

                        default:
                            PRT(("%s %s ", alu_opa_desc, alu_opb_desc));
                            break;
                    }

                    switch (bs) {
                        case DSPN_ARITH_BS_NONE:
                            break;

                        case DSPN_ARITH_BS_SPECIAL:
                            PRT(("%s _<< ", opdesc[nmult_ops + nalu_ops]));
                            break;

                        default:
                            {
                                static const char * const bsdesc[] = {
                                    NULL,       "1 _<<'",   "2 _<<'",   "3 _<<'",
                                    "4 _<<'",   "5 _<<'",   "8 _<<'",   "_CLIP",
                                    NULL,       "16 _>>'",  "8 _>>'",   "5 _>>'",
                                    "4 _>>'",   "3 _>>'",   "2 _>>'",   "1 _>>'",
                                };

                                PRT(("%s ", bsdesc[bs]));
                            }
                            break;
                    }

                    if (use_mult) {
                        static const char * const aludesc[] = {
                            "???",  "NEG",  "+",    "+C",
                            "-",    "-B",   "++",   "--",
                            "",     "NOT",  "AND",  "NAND",
                            "OR",   "NOR",  "XOR",  "XNOR",
                        };
                        PRT(("_*%s ", aludesc[alu]));
                    }
                    else {
                        static const char * const aludesc[] = {
                            "TRA",  "NEG",  "+",    "+C",
                            "-",    "-B",   "++",   "--",
                            "TRL",  "NOT",  "AND",  "NAND",
                            "OR",   "NOR",  "XOR",  "XNOR",
                        };
                        PRT(("_%s ", aludesc[alu]));
                    }
                }
                else {
                    PRT(("??? "));
                }
            }
            break;

        case DSPN_OPCODE_ANVIL_RBASE:
            PRT(("0x%02x ", instructionInfo.dspnii_CodeWord & DSPN_ANVIL_RBASE_MASK));
            break;

        case DSPN_OPCODE_BULLDOG_RBASE:
            {
                const char *rbaseselect = "";

                switch (instructionInfo.dspnii_CodeWord & DSPN_BULLDOG_RBASE_SELECT_MASK) {
                    case DSPN_BULLDOG_RBASE_SELECT_0:   rbaseselect = "R0-R15"; break;
                    case DSPN_BULLDOG_RBASE_SELECT_4:   rbaseselect = "R4-R7"; break;
                    case DSPN_BULLDOG_RBASE_SELECT_8:   rbaseselect = "R8-R15"; break;
                    case DSPN_BULLDOG_RBASE_SELECT_12:  rbaseselect = "R12-R15"; break;
                }

                PRT(("%s,0x%04x ", rbaseselect, instructionInfo.dspnii_CodeWord & DSPN_BULLDOG_RBASE_ADDR_MASK));
            }
            break;

        case DSPN_OPCODE_MOVEADDR:
            dsmPrintAddr (instructionInfo.dspnii_CodeWord, FALSE);
            break;

        case DSPN_OPCODE_MOVEREG:
            dsmPrintReg (instructionInfo.dspnii_CodeWord, 0, FALSE);
            break;

        case DSPN_OPCODE_JUMP:
        case DSPN_OPCODE_JSR:
        case DSPN_OPCODE_BFM:
        case DSPN_OPCODE_BVS:
        case DSPN_OPCODE_BMI:
        case DSPN_OPCODE_BRB:
        case DSPN_OPCODE_BEQ:
        case DSPN_OPCODE_BCS:
        case DSPN_OPCODE_BOP:
        case DSPN_OPCODE_BVC:
        case DSPN_OPCODE_BPL:
        case DSPN_OPCODE_BRP:
        case DSPN_OPCODE_BNE:
        case DSPN_OPCODE_BCC:
        case DSPN_OPCODE_BLT:
        case DSPN_OPCODE_BLE:
        case DSPN_OPCODE_BGE:
        case DSPN_OPCODE_BGT:
        case DSPN_OPCODE_BHI:
        case DSPN_OPCODE_BLS:
        case DSPN_OPCODE_BXS:
        case DSPN_OPCODE_BXC:
            PRT(("0x%04lx ", instructionInfo.dspnii_CodeWord & DSPN_BRANCH_ADDR_MASK));
            break;
    }
    PRT(("\n"));
}

static void dsmPrintOperandWord (const DSPNInstructionInfo operandWordInfo)
{
    PRT(("          0x%04lx  ", operandWordInfo.dspnii_CodeWord));

    switch (operandWordInfo.dspnii_Type) {
        case DSPN_OPERAND_IMMED:
            PRT(("IMMED 0x%04lx", dspnUnpackImmediate (operandWordInfo.dspnii_CodeWord)));
            break;

        case DSPN_OPERAND_ADDR:
            PRT(("ADDR "));
            dsmPrintAddr (operandWordInfo.dspnii_CodeWord, (operandWordInfo.dspnii_CodeWord & DSPN_ADDR_F_WRITE_BACK) != 0);
            break;

        case DSPN_OPERAND_2_REG:
            dsmPrintReg (operandWordInfo.dspnii_CodeWord, 1, (operandWordInfo.dspnii_CodeWord & DSPN_REG2_F_WRITE_BACK) != 0);
        case DSPN_OPERAND_1_REG:
            dsmPrintReg (operandWordInfo.dspnii_CodeWord, 0, (operandWordInfo.dspnii_CodeWord & DSPN_REG1_F_WRITE_BACK) != 0);
            break;

        case DSPN_OPERAND_3_REG:
            dsmPrintReg (operandWordInfo.dspnii_CodeWord, 2, FALSE);
            dsmPrintReg (operandWordInfo.dspnii_CodeWord, 1, FALSE);
            dsmPrintReg (operandWordInfo.dspnii_CodeWord, 0, FALSE);
            break;
    }
    PRT(("\n"));
}

static void dsmPrintReg (uint32 codeword, uint32 regfieldnum, bool writeback)
{
    char b[16];
    const uint32 regfield = (codeword & DSPN_REGFIELD_MASK(regfieldnum)) >> DSPN_REGFIELD_SHIFT(regfieldnum);

    sprintf (b, "R%ld", regfield & DSPN_REG_MASK);
    dsmPrintOperand (b, (regfield & DSPN_REG_F_IND) != 0, writeback);
}

static void dsmPrintAddr (uint32 codeword, bool writeback)
{
    char b[16];

    sprintf (b, "0x%04lx", codeword & DSPN_ADDR_MASK);
    dsmPrintOperand (b, (codeword & DSPN_ADDR_F_IND) != 0, writeback);
}

static void dsmPrintOperand (const char *opstr, bool indirect, bool writeback)
{
    PRT((indirect ? "[%s%s] " : "%s%s ", writeback ? "%" : "", opstr));
}

static const char *dsmGetMnemonic (const DSMTypeMnemonic *tbl, uint32 tbllen, int32 type)
{
    for (; tbllen--; tbl++) {
        if (tbl->mne_Type == type) return tbl->mne_Mnemonic;
    }
    return NULL;
}
