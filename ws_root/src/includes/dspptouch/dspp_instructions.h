#ifndef __DSPPTOUCH_DSPP_INSTRUCTIONS_H
#define __DSPPTOUCH_DSPP_INSTRUCTIONS_H


/******************************************************************************
**
**  @(#) dspp_instructions.h 95/08/09 1.10
**  $Id: dspp_instructions.h,v 1.12 1995/03/16 01:26:04 peabody Exp phil $
**
**  DSPP Instruction (machine code) Definitions and Support.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  950111 WJB  Extracted from dspp_remap_bulldog.c.
**  950111 WJB  Cleaned up DSPP instruction definitions.
**  950112 WJB  Added DSPP Instruction and Operand Identification support stuff.
**  950112 WJB  Added dspnDisassemble() prototype.
**  950113 WJB  Made dspnDisassemble() prototype depend on DSPP_DISASM.
**  950117 WJB  Added some notes.
**  950123 WJB  Added description text to dspnDisassemble().
**  950306 WJB  Added DSPN_BULLDOG_RBASE_ALIGNMENT.
**  950315 WJB  Added DSPNTypeMask table arg to dspnGetInstructionInfo().
**  950315 WJB  Made dspnii_Type signed.
**  950503 WJB  Added dspnPack/UnpackImmediate().
**  950727 WJB  Added more defines for arithmetic instruction opcode fields.
**  950727 WJB  Changed all _MASKs to be in-place rather than right justified.
**  950809 WJB  Added dsphDisassembleCodeMem().
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**
******************************************************************************/

#include <kernel/types.h>


/* -------------------- Opcodes */

    /* arithmetic opcode mask and type */
#define DSPN_OPCODEMASK_ARITH           0x8000
#define DSPN_OPCODE_ARITH               0x0000      /* NUM_OPS, MULT_SELECT, ALU_MUX A & B, ALU_OP, BS */
/* @@@ could define more actual n-structions here based on bit combinations of alu matrix and alu operation */

    /* arithmetic opcode NUM_OPS field (numeric) */
#define DSPN_ARITH_NUM_OPS_SHIFT        13
#define DSPN_ARITH_NUM_OPS_WIDTH        2
#define DSPN_ARITH_NUM_OPS_MASK         0x6000

    /* arithmetic opcode MULT_SELECT flag */
#define DSPN_ARITH_F_MULT_SELECT        0x1000

    /* arithmetic MUX selection fields */
#define DSPN_ARITH_MUX_A_MASK           0x0c00
#define DSPN_ARITH_MUX_A_SHIFT          10
#define DSPN_ARITH_MUX_B_MASK           0x0300
#define DSPN_ARITH_MUX_B_SHIFT          8

    /* arithmetic MUX selection codes */
#define DSPN_ARITH_MUX_ACCUM            0x0
#define DSPN_ARITH_MUX_OP1              0x1
#define DSPN_ARITH_MUX_OP2              0x2
#define DSPN_ARITH_MUX_MULT             0x3

    /* arithmetic operation code field */
#define DSPN_ARITH_F_LOGICAL            0x0080      /* this flag is set for all logical alu operations, clear for arithmetic operations (flag is in place, not shifted) */
#define DSPN_ARITH_ALU_MASK             0x00f0
#define DSPN_ARITH_ALU_SHIFT            4
#define DSPN_ARITH_ALU_TRA              0x0         /* arithmetic operation 0..7 */
#define DSPN_ARITH_ALU_NEG              0x1
#define DSPN_ARITH_ALU_ADD              0x2
#define DSPN_ARITH_ALU_ADDC             0x3
#define DSPN_ARITH_ALU_SUB              0x4
#define DSPN_ARITH_ALU_SUBB             0x5
#define DSPN_ARITH_ALU_INC              0x6
#define DSPN_ARITH_ALU_DEC              0x7
#define DSPN_ARITH_ALU_TRL              0x8         /* logical operations 8..f */
#define DSPN_ARITH_ALU_NOT              0x9
#define DSPN_ARITH_ALU_AND              0xa
#define DSPN_ARITH_ALU_NAND             0xb
#define DSPN_ARITH_ALU_OR               0xc
#define DSPN_ARITH_ALU_NOR              0xd
#define DSPN_ARITH_ALU_XOR              0xe
#define DSPN_ARITH_ALU_XNOR             0xf

    /* arithmetic opcode barrel shifter mask and codes */
#define DSPN_ARITH_BS_MASK              0x000f
#define DSPN_ARITH_BS_NONE              0x0000
#define DSPN_ARITH_BS_LEFT_1            0x0001
#define DSPN_ARITH_BS_LEFT_2            0x0002
#define DSPN_ARITH_BS_LEFT_3            0x0003
#define DSPN_ARITH_BS_LEFT_4            0x0004
#define DSPN_ARITH_BS_LEFT_5            0x0005
#define DSPN_ARITH_BS_LEFT_8            0x0006
#define DSPN_ARITH_BS_CLIP              0x0007
#define DSPN_ARITH_BS_SPECIAL           0x0008      /* Shift value comes from an operand
                                                       (indicates 1 more operand than described in
                                                       NUM_OPS field) */
#define DSPN_ARITH_BS_RIGHT_16          0x0009
#define DSPN_ARITH_BS_RIGHT_8           0x000a
#define DSPN_ARITH_BS_RIGHT_5           0x000b
#define DSPN_ARITH_BS_RIGHT_4           0x000c
#define DSPN_ARITH_BS_RIGHT_3           0x000d
#define DSPN_ARITH_BS_RIGHT_2           0x000e
#define DSPN_ARITH_BS_RIGHT_1           0x000f

    /* move opcode masks and types */
#define DSPN_OPCODEMASK_MOVEREG         0xfc00
#define DSPN_OPCODE_MOVEREG             0x9000      /* F_IND, REG */

#define DSPN_OPCODEMASK_MOVEADDR        0xf800
#define DSPN_OPCODE_MOVEADDR            0x9800      /* F_IND, ADDR */

    /* branch opcode mask, types and fields */
#define DSPN_OPCODEMASK_BRANCH          0xfc00
#define DSPN_OPCODE_JUMP                0x8400      /* ADDR */
#define DSPN_OPCODE_JSR                 0x8800      /* ADDR */
#define DSPN_OPCODE_BFM                 0x8c00      /* ADDR */
#define DSPN_OPCODE_BVS                 0xa400      /* ADDR */
#define DSPN_OPCODE_BMI                 0xa800      /* ADDR */
#define DSPN_OPCODE_BRB                 0xac00      /* ADDR */
#define DSPN_OPCODE_BEQ                 0xb400      /* ADDR */
#define DSPN_OPCODE_BCS                 0xb800      /* ADDR */
#define DSPN_OPCODE_BOP                 0xbc00      /* ADDR */
#define DSPN_OPCODE_BVC                 0xc400      /* ADDR */
#define DSPN_OPCODE_BPL                 0xc800      /* ADDR */
#define DSPN_OPCODE_BRP                 0xcc00      /* ADDR */
#define DSPN_OPCODE_BNE                 0xd400      /* ADDR */
#define DSPN_OPCODE_BCC                 0xd800      /* ADDR */
#define DSPN_OPCODE_BLT                 0xe000      /* ADDR */
#define DSPN_OPCODE_BLE                 0xe400      /* ADDR */
#define DSPN_OPCODE_BGE                 0xe800      /* ADDR */
#define DSPN_OPCODE_BGT                 0xec00      /* ADDR */
#define DSPN_OPCODE_BHI                 0xf000      /* ADDR */
#define DSPN_OPCODE_BLS                 0xf400      /* ADDR */
#define DSPN_OPCODE_BXS                 0xf800      /* ADDR */
#define DSPN_OPCODE_BXC                 0xfc00      /* ADDR */

#define DSPN_BRANCH_ADDR_MASK           0x03ff      /* 10-bit branch address */

    /* control opcode mask, types and fields */
#define DSPN_OPCODEMASK_CTRL            0xfc00

#define DSPN_OPCODE_BULLDOG_RBASE       0x9400      /* ADDR, SELECT */

#define DSPN_BULLDOG_RBASE_ADDR_MASK    0x03fc
#define DSPN_BULLDOG_RBASE_ALIGNMENT    4           /* # of words alignment required for RBASE */

#define DSPN_BULLDOG_RBASE_SELECT_MASK  0x0003
#define DSPN_BULLDOG_RBASE_SELECT_0     0x0000      /* set base for R0-R15 */
#define DSPN_BULLDOG_RBASE_SELECT_4     0x0001      /* set base for R4-R7 */
#define DSPN_BULLDOG_RBASE_SELECT_8     0x0002      /* set base for R8-R15 */
#define DSPN_BULLDOG_RBASE_SELECT_12    0x0003      /* set base for R12-R15 */

    /* super-special opcode mask, types and fields */
#define DSPN_OPCODEMASK_SUPER_SPECIAL   0xff80
#define DSPN_OPCODE_NOP                 0x8000
#define DSPN_OPCODE_BAC                 0x8080
#define DSPN_OPCODE_ANVIL_RBASE         0x8100      /* ANVIL_RBASE */
#define DSPN_OPCODE_ANVIL_RMAP          0x8180      /* ANVIL_RMAP */
#define DSPN_OPCODE_RTS                 0x8200
#define DSPN_OPCODE_OP_MASK             0x8280      /* !!! args? */
#define DSPN_OPCODE_SLEEP               0x8380

#define DSPN_ANVIL_RBASE_MASK           0x003f      /* @@@ this could do with some further decomposition */
/* #define dspnConvertDSPIToAnvilRBASE(dspi)   @@@ not implemented */
/* #define dspnConvertAnvilRBASEToDSPI(rbase)  @@@ not implement */

#define DSPN_ANVIL_RMAP_MASK            0x0007
/* !!! OP_MASK field */


/* -------------------- Operands */

    /* immediate operand mask, type and fields */
#define DSPN_OPERANDMASK_IMMED          0xc000
#define DSPN_OPERAND_IMMED              0xc000      /* F_JUSTIFY, VALUE */

#define DSPN_IMMED_F_JUSTIFY            0x2000
#define DSPN_IMMED_MASK                 0x1fff

    /* address operand mask, type and fields */
#define DSPN_OPERANDMASK_ADDR           0xe000
#define DSPN_OPERAND_ADDR               0x8000      /* F_WRITE_BACK, F_IND, ADDR */

#define DSPN_ADDR_F_WRITE_BACK          0x0800

    /* register operand masks, types and fields */
#define DSPN_OPERANDMASK_1_REG          0xe400
#define DSPN_OPERAND_1_REG              0xa000      /* F_WRITE_BACK, F_IND, REG */

#define DSPN_OPERANDMASK_2_REG          0xe400
#define DSPN_OPERAND_2_REG              0xa400      /* REG 2: F_WRITE_BACK, F_IND, REG;  REG 1: F_WRITE_BACK, F_IND, REG */

#define DSPN_OPERANDMASK_3_REG          0x8000
#define DSPN_OPERAND_3_REG              0x0000      /* REG 3: F_IND, REG;  REG 2: F_IND, REG;  REG 1: F_IND, REG */

#define DSPN_REG1_F_WRITE_BACK          0x0800
#define DSPN_REG2_F_WRITE_BACK          0x1000


/* -------------------- Common opcode and operand field definitions */

    /* address field (MOVE to memory opcode and address operand) */
#define DSPN_ADDR_F_IND                 0x0400      /* 1: indirect, 0: direct */
#define DSPN_ADDR_MASK                  0x03ff      /* 10-bit address */
#define DSPN_ADDR_WIDTH                 10          /* # of bits in address */

    /* register fields (MOVE to register opcode and register operands) */
#define DSPN_REGFIELD_SHIFT(nreg)       ((nreg) * DSPN_REGFIELD_WIDTH)  /* 0 is rightmost register (labeled R1 on diagram),
                                                                           2 is leftmost register (labeled R3 on diagram) */
#define DSPN_REGFIELD_WIDTH             5
#define DSPN_REGFIELD_MASK(nreg)        (0x001f << DSPN_REGFIELD_SHIFT(nreg))
#define DSPN_REG_F_IND                  0x0010
#define DSPN_REG_MASK                   0x000f


/* -------------------- DSPP Instruction and Operand identification */

typedef struct DSPNTypeMask {
    uint16 dspntm_Type;         /* type id of n-struction or operand */
    uint16 dspntm_Mask;         /* mask to apply to AND n-struction or operand with before comparing with dspntm_Type */
} DSPNTypeMask;

typedef struct DSPNInstructionInfo {
    uint32 dspnii_CodeWord;     /* opcode or operand word (simply 32-bit expansion of original 16-bit word - convenience0 */
    int32  dspnii_Type;         /* opcode or operand word type. >= 0 when successfully matched. < 0 when not found. */
    uint32 dspnii_NumOperands;  /* # of operands for instruction or contained in operand word */
} DSPNInstructionInfo;

    /* dspp_instructions.c */
DSPNInstructionInfo dspnGetInstructionInfo (const DSPNTypeMask *opcodeTable, uint32 opcodeTableLength, uint32 opcode);
DSPNInstructionInfo dspnGetOperandWordInfo (uint32 operandword);


/* -------------------- Misc functions */

    /* Immediate operands */
bool dspnCanValueBeImmediate (uint16 val);
uint16 dspnPackImmediate (uint16 val);
uint16 dspnUnpackImmediate (uint16 val);

    /* Disassembler */
int32 dspnDisassemble (const uint16 *codeImage, uint32 codeBase, int32 codeSize, const char *descfmt, ...);
void dsphDisassembleCodeMem (uint16 codeAddr, uint16 codeSize, const char *banner);


/*****************************************************************************/

#endif /* __DSPPTOUCH_DSPP_INSTRUCTIONS_H */
