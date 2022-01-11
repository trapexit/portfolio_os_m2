/*  @(#) ppc_disasm.cpp 96/07/25 1.13 */


/* Copyright 1994 Free Software Foundation, Inc.
 *
 * Originally written by Ian Lance Taylor, Cygnus Support
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.

 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.

 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

 *  PROMINENT NOTICE: This file was substantially modified by Martin
 *  Taillefer, The 3DO Company, in 1995.  For more information, contact
 *  your 3DO DTS representative at 415 261 3400, or send email to
 *  'support@3do.com'.
 */


#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "ppc_disasm.h"


/*****************************************************************************/


#ifndef bool
#define bool Boolean
#endif

#undef TRUE
#undef FALSE

#define TRUE  ((bool)1)
#define FALSE ((bool)0)


typedef enum
{
	OPTYPE_MISC,
    OPTYPE_LOAD,
    OPTYPE_STORE,
    OPTYPE_BRANCH,
    OPTYPE_RFI
} OpcodeTypes;

/* Describes an individual opcode */
typedef struct
{
    /* The opcode name */
    const char   *opc_Name;

    /* The opcode itself. Those bits which will be filled in with
     * operands should be 0.
     */
    uint32        opc_Opcode;

    /* The opcode mask containing ones indicating those bits which must match
     * the opcode field, and zeroes indicating those bits which need not
     * match (and are presumably filled in by operands).
     */
    uint32        opc_Mask;

    /* general type of instruction, for use by the comment generator */
    unsigned char opc_Type;

    /* An array of operand codes. Each code is an index into the
     * operand table.  They appear in the order which the operands must
     * appear in assembly code, and are terminated by a zero.
     */
    unsigned char opc_Operands[6];
} PPCOpcode;


/* A macro to extract the major opcode from an instruction.  */
#define PPC_OP(i) (((i) >> 26) & 0x3f)


/*****************************************************************************/


/* Describes an individual operand to an instruction */
typedef struct PPCOperand
{
    /* The number of bits in the operand */
    int32 op_Bits;

    /* How far the operand is left shifted in the instruction */
    int32 op_Shift;

    /* Extraction function to extract this operand type from an instruction.
     *
     * If this field is not NULL, then simply call it with the
     * instruction value.  It will return the value of the operand.
     * *INVALID will be set to TRUE if this operand type can not
     * actually be extracted from this operand (i.e., the instruction does not
     * match).  If the operand is valid, *INVALID will not be changed.
     */
    uint32 (*op_Extract) (uint32 instruction, bool *invalid);

    /* syntax flags */
    uint32  op_Flags;
} PPCOperand;


/* Values defined for the op_Flags field of a PPCOperand structure.
 *
 * PPC_OPERAND_SIGNED
 * This operand takes signed values.
 *
 * PPC_OPERAND_SIGNOPT
 * This operand takes signed values, but also accepts a full positive
 * range of values when running in 32 bit mode.  That is, if bits is
 * 16, it takes any value from -0x8000 to 0xffff.  In 64 bit mode,
 * this flag is ignored.
 *
 * PPC_OPERAND_FAKE
 * This operand does not actually exist in the assembler input.  This
 * is used to support extended mnemonics such as mr, for which two
 * operands fields are identical.  The disassembler should call
 * the extract function, ignore the return value, and check the value
 * placed in the valid argument.
 *
 * PPC_OPERAND_PARENS
 * The next operand should be wrapped in parentheses rather than
 * separated from this one by a comma.  This is used for the load and
 * store instructions which want their operands to look like
 *     reg,displacement(reg)
 *
 * PPC_OPERAND_CR
 * This operand may use the symbolic names for the CR fields, which
 * are
 *     lt  0	gt  1	eq  2	so  3	un  3
 *     cr0 0	cr1 1	cr2 2	cr3 3
 *     cr4 4	cr5 5	cr6 6	cr7 7
 * These may be combined arithmetically, as in cr2*4+gt.
 *
 * PPC_OPERAND_GPR
 * This operand names a register.  The disassembler uses this to print
 * register names with a leading 'r'.
 *
 * PPC_OPERAND_FPR
 * This operand names a floating point register.  The disassembler
 * prints these with a leading 'f'.
 *
 * PPC_OPERAND_SPR
 * This operand names a special purpose register.  The disassembler uses
 * this to print register names when possible.
 *
 * PPC_OPERAND_RELATIVE
 * This operand is a relative branch displacement.  The disassembler
 * prints these symbolically if possible.
 *
 * PPC_OPERAND_ABSOLUTE
 * This operand is an absolute branch address.  The disassembler
 * prints these symbolically if possible.
 *
 * PPC_OPERAND_OPTIONAL
 * This operand is optional, and is zero if omitted.  This is used for
 * the optional BF and L fields in the comparison instructions.
 * The disassembler should print this operand out only if it is not zero.
 *
 * PPC_OPERAND_NEXT
 * This flag is only used with PPC_OPERAND_OPTIONAL.  If this operand
 * is omitted, then for the next operand use this operand value plus
 * 1, ignoring the next operand field for the opcode.  This wretched
 * hack is needed because the Power rotate instructions can take
 * either 4 or 5 operands.  The disassembler should print this operand
 * out regardless of the PPC_OPERAND_OPTIONAL field.
 *
 * PPC_OPERAND_NEGATIVE
 * This operand should be regarded as a negative number for the
 * purposes of overflow checking (i.e., the normal most negative
 * number is disallowed and one more than the normal most positive
 * number is allowed).  This flag will only be set for a signed
 * operand.
 *
 * PPC_OPERAND_HEX
 * This operand should be displayed in hexadecimal.
 */
#define PPC_OPERAND_SIGNED	 0x0001
#define PPC_OPERAND_SIGNOPT	 0x0002
#define PPC_OPERAND_FAKE	 0x0004
#define PPC_OPERAND_PARENS	 0x0008
#define PPC_OPERAND_CR		 0x0010
#define PPC_OPERAND_GPR		 0x0020
#define PPC_OPERAND_FPR		 0x0040
#define PPC_OPERAND_SPR		 0x0080
#define PPC_OPERAND_RELATIVE	 0x0100
#define PPC_OPERAND_ABSOLUTE	 0x0200
#define PPC_OPERAND_OPTIONAL	 0x0400
#define PPC_OPERAND_NEXT	 0x0800
#define PPC_OPERAND_NEGATIVE	 0x1000
#define PPC_OPERAND_HEX          0x2000


/*****************************************************************************/


/* A bunch of simple functions to extract complicated operands from
 * an instruction.
 */

/* The BA field in an XL form instruction when it must be the same as
 * the BT field in the same instruction.  This operand is marked FAKE.
 * The insertion function just copies the BT field into the BA field,
 * and the extraction function just checks that the fields are the
 * same.
 */
static uint32 Extract_BAT(uint32 instr, bool *invalid)
{
    if (((instr >> 21) & 0x1f) != ((instr >> 16) & 0x1f))
        *invalid = TRUE;

    return 0;
}

/* The BB field in an XL form instruction when it must be the same as
 * the BA field in the same instruction.  This operand is marked FAKE.
 * The insertion function just copies the BA field into the BB field,
 * and the extraction function just checks that the fields are the
 * same.
 */
static uint32 Extract_BBA(uint32 instr, bool *invalid)
{
    if (((instr >> 16) & 0x1f) != ((instr >> 11) & 0x1f))
        *invalid = TRUE;

    return 0;
}

/* The BD field in a B form instruction.  The lower two bits are
 * forced to zero.
 */
static uint32 Extract_BD(uint32 instr, bool *invalid)
{
    if (instr & 0x8000)
        return (instr & 0xfffc) - 0x10000;

    return (instr & 0xfffc);
}

/* The BD field in a B form instruction when the - modifier is used.
 * This modifier means that the branch is not expected to be taken.
 * We must set the y bit of the BO field to 1 if the offset is
 * negative.  When extracting, we require that the y bit be 1 and that
 * the offset be positive, since if the y bit is 0 we just want to
 * print the normal form of the instruction.
 */
static uint32 Extract_BDM(uint32 instr, bool *invalid)
{
    if (((instr & (1 << 21)) == 0) || (instr & (1 << 15) == 0))
        *invalid = TRUE;

    if (instr & 0x8000)
        return (instr & 0xfffc) - 0x10000;

    return (instr & 0xfffc);
}

/* The BD field in a B form instruction when the + modifier is used.
 * This is like BDM, above, except that the branch is expected to be
 * taken.
 */
static uint32 Extract_BDP(uint32 instr, bool *invalid)
{
    if ((((instr & (1 << 21)) == 0) || (instr & (1 << 15)) != 0))
        *invalid = TRUE;

    if (instr & 0x8000)
        return (instr & 0xfffc) - 0x10000;

    return (instr & 0xfffc);
}

/* Check for legal values of a BO field. */

static bool valid_bo(uint32 value)
{
    /* Certain encodings have bits that are required to be zero.  These
     * are (z must be zero, y may be anything):
     *   001zy
     *   011zy
     *   1z00y
     *   1z01y
     *   1z1zz
     */
    switch (value & 0x14)
    {
        default  :
        case 0   : return TRUE;
        case 0x4 : return ((value & 0x2) ? FALSE : TRUE);
        case 0x10: return ((value & 0x8) ? FALSE : TRUE);
        case 0x14: return ((value == 0x14) ? TRUE : FALSE);
    }
}

/* The BO field in a B form instruction.  Warn about attempts to set
 * the field to an illegal value.
 */
static uint32 Extract_BO(uint32 instr, bool *invalid)
{
uint32 value;

    value = (instr >> 21) & 0x1f;
    if (!valid_bo(value))
        *invalid = TRUE;

    return value;
}

/* The BO field in a B form instruction when the + or - modifier is
 * used.  This is like the BO field, but it must be even.  When
 * extracting it, we force it to be even.
 */
static uint32 Extract_BOE(uint32 instr, bool *invalid)
{
uint32 value;

    value = (instr >> 21) & 0x1f;
    if (!valid_bo(value))
        *invalid = TRUE;

    return (value & 0x1e);
}

/* The condition register number portion of the BI field in a B form
 * or XL form instruction.  This is used for the extended conditional
 * branch mnemonics, which set the lower two bits of the BI field.  It
 * is the BI field with the lower two bits ignored.
 */
static uint32 Extract_CR(uint32 instr, bool *invalid)
{
    return (instr >> 16) & 0x1c;
}

/* The DS field in a DS form instruction.  This is like D, but the
 * lower two bits are forced to zero.
 */
static uint32 Extract_DS(uint32 instr, bool *invalid)
{
    if (instr & 0x8000)
        return (instr & 0xfffc) - 0x10000;

    return (instr & 0xfffc);
}

/* The LI field in an I form instruction.  The lower two bits are
 * forced to zero.
 */
static uint32 Extract_LI(uint32 instr, bool *invalid)
{
    if (instr & 0x2000000)
        return (instr & 0x3fffffc) - 0x4000000;

    return (instr & 0x3fffffc);
}

/* The MB and ME fields in an M form instruction expressed as a single
 * operand which is itself a bitmask.  The extraction function always
 * marks it as invalid, since we never want to recognize an
 * instruction which uses a field of this type.
 */
static uint32 Extract_MBE(uint32 instr, bool *invalid)
{
uint32 value;
uint32 mb, me;
uint32 i;

    *invalid = TRUE;

    value = 0;
    mb    = (instr >> 6) & 0x1f;
    me    = (instr >> 1) & 0x1f;

    for (i = mb; i < me; i++)
        value |= 1 << (31 - i);

    return value;
}

/* The MB or ME field in an MD or MDS form instruction.  The high bit
 * is wrapped to the low end.
 */
static uint32 Extract_MB6(uint32 instr, bool *invalid)
{
    return ((instr >> 6) & 0x1f) | (instr & 0x20);
}

/* The NB field in an X form instruction.  The value 32 is stored as
 * 0.
 */
static uint32 Extract_NB(uint32 instr, bool *invalid)
{
uint32 value;

    value = (instr >> 11) & 0x1f;
    if (value == 0)
        value = 32;

    return value;
}

/* The NSI field in a D form instruction.  This is the same as the SI
 * field, only negated.  The extraction function always marks it as
 * invalid, since we never want to recognize an instruction which uses
 * a field of this type.
 */
static uint32 Extract_NSI(uint32 instr, bool *invalid)
{
    *invalid = TRUE;

    if (instr & 0x8000)
        return - ((instr & 0xffff) - 0x10000);

    return - (instr & 0xffff);
}

static uint32 Extract_RBS(uint32 instr, bool *invalid)
{
    if (((instr >> 21) & 0x1f) != ((instr >> 11) & 0x1f))
        *invalid = TRUE;

    return 0;
}

/* The SH field in an MD form instruction.  This is split.  */
static uint32 Extract_SH6(uint32 instr, bool *invalid)
{
    return ((instr >> 11) & 0x1f) | ((instr << 4) & 0x20);
}

/* The SPR or TBR field in an XFX form instruction.  This is
 * flipped -- the lower 5 bits are stored in the upper 5 and vice-versa.
 */
static uint32 Extract_SPR(uint32 instr, bool *invalid)
{
    return ((instr >> 16) & 0x1f) | ((instr >> 6) & 0x3e0);
}


/*****************************************************************************/


typedef enum OperandTypes
{
    LAST,
    BA,
    BAT,
    BB,
    BBA,
    BD,
    BDA,
    BDM,
    BDMA,
    BDP,
    BDPA,
    BF,
    OBF,
    BFA,
    BI,
    BO,
    BOE,
    BT,
    CR,
    D,
    DS,
    FLM,
    FRA,
    FRB,
    FRC,
    FRS,
    FXM,
    L,
    LI,
    LIA,
    MB,
    ME,
    MBE,
    OP_DUMMY,  /* placeholder */
    MB6,
    NB,
    NSI,
    RA,
    RAL,
    RAM,
    RAO,
    RAS,
    RB,
    RBS,
    RS,
    SH,
    SH6,
    SI,
    SISIGNOPT,
    SPR,
    SR,
    TO,
    U,
    UI,
    XUI
} OperandTypes;

#define BA_MASK		(0x1f << 16)
#define BB_MASK		(0x1f << 11)
#define BI_MASK		(0x1f << 16)
#define BO_MASK		(0x1f << 21)
#define FRA_MASK	(0x1f << 16)
#define FRB_MASK	(0x1f << 11)
#define FRC_MASK	(0x1f << 6)
#define FRT		(FRS)
#define MB_MASK		(0x1f << 6)
#define ME_MASK		(0x1f << 1)
#define ME6		(MB6)
#define MB6_MASK	(0x3f << 5)
#define RA_MASK		(0x1f << 16)
#define RB_MASK		(0x1f << 11)
#define RT		(RS)
#define RT_MASK		(0x1f << 21)
#define SH_MASK		(0x1f << 11)
#define SH6_MASK	((0x1f << 11) | (1 << 1))
#define SPR_MASK	(0x3ff << 11)
#define TBR		(SPR)
#define TBR_MASK	(0x3ff << 11)
#define TO_MASK		(0x1f << 21)

/* LAST
 * Used to indicate the end of the list of operands.
 *
 * BA
 * The BA field in an XL form instruction.
 *
 * BAT
 * The BA field in an XL form instruction when it must be the same
 * as the BT field in the same instruction.
 *
 * BB
 * The BB field in an XL form instruction.
 *
 * BBA
 * The BB field in an XL form instruction when it must be the same
 * as the BA field in the same instruction.
 *
 * BD
 * The BD field in a B form instruction.  The lower two bits are
 * forced to zero.
 *
 * BDA
 * The BD field in a B form instruction when absolute addressing is used.
 *
 * BDM
 * The BD field in a B form instruction when the - modifier is used.
 *
 * BDMA
 * The BD field in a B form instruction when the - modifier and
 * absolute addressing are used.
 *
 * BDP
 * The BD field in a B form instruction when the + modifier is used.
 *
 * BDPA
 * The BD field in a B form instruction when the + modifier and
 * absolute addressing are used.
 *
 * BF
 * The BF field in an X or XL form instruction.
 *
 * OBF
 * An optional BF field.  This is used for comparison instructions,
 * in which an omitted BF field is taken as zero.
 *
 * BFA
 * The BFA field in an X or XL form instruction.
 *
 * BI
 * The BI field in a B form or XL form instruction.
 *
 * BO
 * The BO field in a B form instruction.  Certain values are illegal.
 *
 * BOE
 * The BO field in a B form instruction when the + or - modifier is
 * used. This is like the BO field, but it must be even.
 *
 * BT
 * The BT field in an X or XL form instruction.
 *
 * CR
 * The condition register number portion of the BI field in a B form
 * or XL form instruction.  This is used for the extended conditional branch
 * mnemonics, which set the lower two bits of the BI field.  This field is
 * optional.
 *
 * D
 * The D field in a D form instruction.  This is a displacement off
 * a register, and implies that the next operand is a register in
 * parentheses.
 *
 * DS
 * The DS field in a DS form instruction.  This is like D, but the
 * lower two bits are forced to zero.
 *
 * FLM
 * The FLM field in an XFL form instruction.
 *
 * FRA
 * The FRA field in an X or A form instruction.
 *
 * FRB
 * The FRB field in an X or A form instruction.
 *
 * FRC
 * The FRC field in an A form instruction.
 *
 * FRS and FRT
 * The FRS field in an X form instruction or the FRT field in a D, X
 * or A form instruction.
 *
 * FXM
 * The FXM field in an XFX instruction.
 *
 * L
 * The L field in a D or X form instruction.
 *
 * LI
 * The LI field in an I form instruction.  The lower two bits are
 * forced to zero.
 *
 * LIA
 * The LI field in an I form instruction when used as an absolute address.
 *
 * MB
 * The MB field in an M form instruction.
 *
 * ME
 * The ME field in an M form instruction.
 *
 * MBE
 * The MB and ME fields in an M form instruction expressed a single
 * operand which is a bitmask indicating which bits to select.  This
 * is a two operand form using PPC_OPERAND_NEXT.
 *
 * MB6
 * The MB or ME field in an MD or MDS form instruction.  The high
 * bit is wrapped to the low end.
 *
 * NB
 * The NB field in an X form instruction.  The value 32 is stored as 0.
 *
 * NSI
 * The NSI field in a D form instruction.  This is the same as the
 * SI field, only negated.
 *
 * RA
 * The RA field in an D, DS, X, XO, M, or MDS form instruction.
 *
 * RAL
 * The RA field in a D or X form instruction which is an updating
 * load, which means that the RA field may not be zero and may not
 * equal the RT field.
 *
 * RAM
 * The RA field in an lmw instruction, which has special value restrictions.
 *
 * RAO
 * Same as RA, except that if the register is r0, don't print it
 *
 * RAS
 * The RA field in a D or X form instruction which is an updating
 * store or an updating floating point load, which means that the RA
 * field may not be zero.
 *
 * RB
 * The RB field in an X, XO, M, or MDS form instruction.
 *
 * RBS
 * The RB field in an X form instruction when it must be the same as
 * the RS field in the instruction.  This is used for extended
 * mnemonics like mr.
 *
 * RS
 * The RS field in a D, DS, X, XFX, XS, M, MD or MDS form
 * instruction or the RT field in a D, DS, X, XFX or XO form
 * instruction.
 *
 * SH
 * The SH field in an X or M form instruction.
 *
 * SH6
 * The SH field in an MD form instruction.  This is split.
 *
 * SI
 * The SI field in a D form instruction.
 *
 * SISIGNOPT
 * The SI field in a D form instruction when we accept a wide range
 * of positive values.
 *
 * SPR
 * The SPR or TBR field in an XFX form instruction.  This is
 * flipped -- the lower 5 bits are stored in the upper 5 and vice-versa.
 *
 * SR
 * The SR field in an X form instruction.
 *
 * TO
 * The TO field in a D or X form instruction.
 *
 * U
 * The U field in an X form instruction.
 *
 * UI
 * The UI field in a D form instruction.
 *
 * XUI
 * Same as a UI field, but should be displayed in hex.
 */

const PPCOperand ppcOperands[] =
{
  /* END      */  { 0, 0, 0, 0, },
  /* BA       */  { 5, 16, 0, PPC_OPERAND_CR },
  /* BAT      */  { 5, 16, Extract_BAT, PPC_OPERAND_FAKE },
  /* BB       */  { 5, 11, 0, PPC_OPERAND_CR },
  /* BBA      */  { 5, 11, Extract_BBA, PPC_OPERAND_FAKE },
  /* BD       */  { 16, 0, Extract_BD, PPC_OPERAND_RELATIVE | PPC_OPERAND_SIGNED },
  /* BDA      */  { 16, 0, Extract_BD, PPC_OPERAND_ABSOLUTE | PPC_OPERAND_SIGNED },
  /* BDM      */  { 16, 0, Extract_BDM, PPC_OPERAND_RELATIVE | PPC_OPERAND_SIGNED },
  /* BDMA     */  { 16, 0, Extract_BDM, PPC_OPERAND_ABSOLUTE | PPC_OPERAND_SIGNED },
  /* BDP      */  { 16, 0, Extract_BDP, PPC_OPERAND_RELATIVE | PPC_OPERAND_SIGNED },
  /* BDPA     */  { 16, 0, Extract_BDP, PPC_OPERAND_ABSOLUTE | PPC_OPERAND_SIGNED },
  /* BF       */  { 3, 23, 0, PPC_OPERAND_CR },
  /* OBF      */  { 3, 23, 0, PPC_OPERAND_CR | PPC_OPERAND_OPTIONAL },
  /* BFA      */  { 3, 18, 0, PPC_OPERAND_CR },
  /* BI       */  { 5, 16, 0, PPC_OPERAND_CR },
  /* BO       */  { 5, 21, Extract_BO, 0 },
  /* BOE      */  { 5, 21, Extract_BOE, 0 },
  /* BT       */  { 5, 21, 0, PPC_OPERAND_CR },
  /* CR       */  { 5, 16, Extract_CR, PPC_OPERAND_CR | PPC_OPERAND_OPTIONAL },
  /* D        */  { 16, 0, 0, PPC_OPERAND_PARENS | PPC_OPERAND_SIGNED },
  /* DS       */  { 16, 0, Extract_DS, PPC_OPERAND_PARENS | PPC_OPERAND_SIGNED },
  /* FLM      */  { 8, 17, 0, 0 },
  /* FRA      */  { 5, 16, 0, PPC_OPERAND_FPR },
  /* FRB      */  { 5, 11, 0, PPC_OPERAND_FPR },
  /* FRC      */  { 5, 6,  0, PPC_OPERAND_FPR },
  /* FRS      */  { 5, 21, 0, PPC_OPERAND_FPR },
  /* FXM      */  { 8, 12, 0, PPC_OPERAND_HEX },
  /* L        */  { 1, 21, 0, PPC_OPERAND_OPTIONAL },
  /* LI       */  { 26, 0, Extract_LI, PPC_OPERAND_RELATIVE | PPC_OPERAND_SIGNED },
  /* LIA      */  { 26, 0, Extract_LI, PPC_OPERAND_ABSOLUTE | PPC_OPERAND_SIGNED },
  /* MB       */  { 5,  6, 0, 0 },
  /* ME       */  { 5,  1, 0, 0 },
  /* MBE      */  { 5,  6, 0, PPC_OPERAND_OPTIONAL | PPC_OPERAND_NEXT },
  /* dummy    */  { 32, 0, Extract_MBE, 0 },
  /* MB6      */  { 6,  5, Extract_MB6, 0 },
  /* NB       */  { 6, 11, Extract_NB, 0 },
  /* NSI      */  { 16, 0, Extract_NSI, PPC_OPERAND_NEGATIVE | PPC_OPERAND_SIGNED },
  /* RA       */  { 5, 16, 0, PPC_OPERAND_GPR },
  /* RAL      */  { 5, 16, 0, PPC_OPERAND_GPR },
  /* RAM      */  { 5, 16, 0, PPC_OPERAND_GPR },
//  /* RAO      */  { 5, 16, 0, 0, PPC_OPERAND_GPR | PPC_OPERAND_OPTIONAL },	//dawn
//dawn - too many initializers
  /* RAO      */  { 5, 16, 0, PPC_OPERAND_GPR | PPC_OPERAND_OPTIONAL },	//dawn
  /* RAS      */  { 5, 16, 0, PPC_OPERAND_GPR },
  /* RB       */  { 5, 11, 0, PPC_OPERAND_GPR },
  /* RBS      */  { 5,  1, Extract_RBS, PPC_OPERAND_FAKE },
  /* RS       */  { 5, 21, 0, PPC_OPERAND_GPR },
  /* SH       */  { 5, 11, 0, 0 },
  /* SH6      */  { 6,  1, Extract_SH6, 0 },
  /* SI       */  { 16, 0, 0, PPC_OPERAND_SIGNED },
  /* SISIGNOPT*/  { 16, 0, 0, PPC_OPERAND_SIGNED | PPC_OPERAND_SIGNOPT },
  /* SPR      */  { 10,11, Extract_SPR, PPC_OPERAND_SPR },
  /* SR       */  { 4, 16, 0, 0 },
  /* TO       */  { 5, 21, 0, 0 },
  /* U        */  { 4, 12, 0, 0 },
  /* UI       */  { 16, 0, 0, 0 },
  /* XUI      */  { 16, 0, 0, PPC_OPERAND_HEX }
};


/*****************************************************************************/


/* Macros used to form opcodes.  */

/* The main opcode */
#define OP(x)		(((x) & 0x3f) << 26)
#define OP_MASK OP	(0x3f)

/* The main opcode combined with a trap code in the TO field of a D
 * form instruction. Used for extended mnemonics for the trap
 * instructions.
 */
#define OPTO(x,to)	(OP(x) | (((to) & 0x1f) << 21))
#define OPTO_MASK	(OP_MASK | TO_MASK)

/* The main opcode combined with a comparison size bit in the L field
 * of a D form or X form instruction.  Used for extended mnemonics for
 * the comparison instructions.
 */
#define OPL(x,l)	(OP(x) | (((l) & 1) << 21))
#define OPL_MASK OPL	(0x3f,1)

/* An A form instruction. */
#define A(op, xop, rc)	(OP(op) | (((xop) & 0x1f) << 1) | ((rc) & 1))
#define A_MASK 		A(0x3f, 0x1f, 1)

/* An A_MASK with the FRB field fixed.  */
#define AFRB_MASK	(A_MASK | FRB_MASK)

/* An A_MASK with the FRC field fixed.  */
#define AFRC_MASK	(A_MASK | FRC_MASK)

/* An A_MASK with the FRA and FRC fields fixed.  */
#define AFRAFRC_MASK	(A_MASK | FRA_MASK | FRC_MASK)

/* A B form instruction.  */
#define B(op, aa, lk)	(OP (op) | (((aa) & 1) << 1) | ((lk) & 1))
#define B_MASK		B(0x3f, 1, 1)

/* A B form instruction setting the BO field.  */
#define BBO(op, bo, aa, lk)	(B((op), (aa), (lk)) | (((bo) & 0x1f) << 21))
#define BBO_MASK 		BBO(0x3f, 0x1f, 1, 1)

/* A BBO_MASK with the y bit of the BO field removed.  This permits
 * matching a conditional branch regardless of the setting of the y bit.
 */
#define Y_MASK		(1 << 21)
#define BBOY_MASK	(BBO_MASK &~ Y_MASK)

/* A B form instruction setting the BO field and the condition bits of
 * the BI field.
 */
#define BBOCB(op, bo, cb, aa, lk) (BBO ((op), (bo), (aa), (lk)) | (((cb) & 0x3) << 16))
#define BBOCB_MASK 		  BBOCB(0x3f, 0x1f, 0x3, 1, 1)

/* A BBOCB_MASK with the y bit of the BO field removed.  */
#define BBOYCB_MASK	(BBOCB_MASK &~ Y_MASK)

/* A BBOYCB_MASK in which the BI field is fixed.  */
#define BBOYBI_MASK	(BBOYCB_MASK | BI_MASK)

/* The main opcode mask with the RA field clear.  */
#define DRA_MASK	(OP_MASK | RA_MASK)

/* A DS form instruction.  */
#define DSO(op, xop)	(OP (op) | ((xop) & 0x3))
#define DS_MASK 	DSO(0x3f, 3)

/* An M form instruction.  */
#define M(op, rc)	(OP (op) | ((rc) & 1))
#define M_MASK 		M(0x3f, 1)

/* An M form instruction with the ME field specified.  */
#define MME(op, me, rc) (M ((op), (rc)) | (((me) & 0x1f) << 1))

/* An M_MASK with the MB and ME fields fixed.  */
#define MMBME_MASK	(M_MASK | MB_MASK | ME_MASK)

/* An M_MASK with the SH and ME fields fixed.  */
#define MSHME_MASK	(M_MASK | SH_MASK | ME_MASK)

/* An MD form instruction.  */
#define MD(op, xop, rc) (OP(op) | (((xop) & 0x7) << 2) | ((rc) & 1))
#define MD_MASK 	MD(0x3f, 0x7, 1)

/* An MD_MASK with the MB field fixed.  */
#define MDMB_MASK	(MD_MASK | MB6_MASK)

/* An MD_MASK with the SH field fixed.  */
#define MDSH_MASK	(MD_MASK | SH6_MASK)

/* An MDS form instruction.  */
#define MDS(op, xop, rc) (OP (op) | (((xop) & 0xf) << 1) | ((rc) & 1))
#define MDS_MASK 	MDS(0x3f, 0xf, 1)

/* An MDS_MASK with the MB field fixed.  */
#define MDSMB_MASK 	(MDS_MASK | MB6_MASK)

/* An SC form instruction.  */
#define SC(op, sa, lk)	(OP (op) | (((sa) & 1) << 1) | ((lk) & 1))
#define SC_MASK 	(OP_MASK | (0x3ff << 16) | (1 << 1) | 1)

/* An X form instruction.  */
#define X(op, xop)	(OP (op) | (((xop) & 0x3ff) << 1))

/* An X form instruction with the RC bit specified.  */
#define XRC(op, xop, rc) (X ((op), (xop)) | ((rc) & 1))

/* The mask for an X form instruction.  */
#define X_MASK 		XRC(0x3f, 0x3ff, 1)

/* An X_MASK with the RA field fixed.  */
#define XRA_MASK	(X_MASK | RA_MASK)

/* An X_MASK with the RB field fixed.  */
#define XRB_MASK	(X_MASK | RB_MASK)

/* An X_MASK with the RT field fixed.  */
#define XRT_MASK	(X_MASK | RT_MASK)

/* An X_MASK with the RA and RB fields fixed.  */
#define XRARB_MASK	(X_MASK | RA_MASK | RB_MASK)

/* An X_MASK with the RT and RA fields fixed.  */
#define XRTRA_MASK	(X_MASK | RT_MASK | RA_MASK)

/* An X form comparison instruction.  */
#define XCMPL(op, xop, l) (X ((op), (xop)) | (((l) & 1) << 21))

/* The mask for an X form comparison instruction.  */
#define XCMP_MASK	(X_MASK | (1 << 22))

/* The mask for an X form comparison instruction with the L field fixed. */
#define XCMPL_MASK (XCMP_MASK | (1 << 21))

/* An X form trap instruction with the TO field specified.  */
#define XTO(op, xop, to) (X ((op), (xop)) | (((to) & 0x1f) << 21))
#define XTO_MASK	 (X_MASK | TO_MASK)

/* An XFL form instruction.  */
#define XFL(op, xop, rc) (OP (op) | (((xop) & 0x3ff) << 1) | ((rc) & 1))
#define XFL_MASK	 (XFL (0x3f, 0x3ff, 1) | (1 << 25) | (1 << 16))

/* An XL form instruction with the LK field set to 0.  */
#define XL(op, xop) (OP (op) | (((xop) & 0x3ff) << 1))

/* An XL form instruction which uses the LK field.  */
#define XLLK(op, xop, lk) (XL ((op), (xop)) | ((lk) & 1))

/* The mask for an XL form instruction.  */
#define XL_MASK XLLK(0x3f, 0x3ff, 1)

/* An XL form instruction which explicitly sets the BO field.  */
#define XLO(op, bo, xop, lk)	(XLLK ((op), (xop), (lk)) | (((bo) & 0x1f) << 21))
#define XLO_MASK 		(XL_MASK | BO_MASK)

/* An XL form instruction which explicitly sets the y bit of the BO
 * field.
 */
#define XLYLK(op, xop, y, lk)	(XLLK ((op), (xop), (lk)) | (((y) & 1) << 21))
#define XLYLK_MASK		(XL_MASK | Y_MASK)

/* An XL form instruction which sets the BO field and the condition
 * bits of the BI field.
 */
#define XLOCB(op, bo, cb, xop, lk)	(XLO ((op), (bo), (xop), (lk)) | (((cb) & 3) << 16))
#define XLOCB_MASK 			XLOCB (0x3f, 0x1f, 0x3, 0x3ff, 1)

/* An XL_MASK or XLYLK_MASK or XLOCB_MASK with the BB field fixed.  */
#define XLBB_MASK	(XL_MASK | BB_MASK)
#define XLYBB_MASK	(XLYLK_MASK | BB_MASK)
#define XLBOCBBB_MASK	(XLOCB_MASK | BB_MASK)

/* An XL_MASK with the BO and BB fields fixed.  */
#define XLBOBB_MASK 	(XL_MASK | BO_MASK | BB_MASK)

/* An XL_MASK with the BO, BI and BB fields fixed.  */
#define XLBOBIBB_MASK	(XL_MASK | BO_MASK | BI_MASK | BB_MASK)

/* An XO form instruction.  */
#define XO(op, xop, oe, rc) 	(OP(op) | (((xop) & 0x1ff) << 1) | (((oe) & 1) << 10) | ((rc) & 1))
#define XO_MASK 		XO(0x3f, 0x1ff, 1, 1)

/* An XO_MASK with the RB field fixed.  */
#define XORB_MASK	(XO_MASK | RB_MASK)

/* An XS form instruction.  */
#define XS(op, xop, rc)	(OP (op) | (((xop) & 0x1ff) << 2) | ((rc) & 1))
#define XS_MASK 	XS(0x3f, 0x1ff, 1)

/* An XFX form instruction with the SPR field filled in.  */
#define XSPR(op, xop, spr)	(X ((op), (xop)) | (((spr) & 0x1f) << 16) | (((spr) & 0x3e0) << 6))
#define XSPR_MASK		(X_MASK | SPR_MASK)

/* An XFX form instruction with the SPR field filled in.  */
#define XTBR(op, xop, tbr)	(X ((op), (xop)) | (((tbr) & 0x1f) << 16) | (((tbr) & 0x3e0) << 6))
#define XTBR_MASK		(X_MASK | TBR_MASK)

/* The BO encodings used in extended conditional branch mnemonics.  */
#define BODNZF	(0x0)
#define BODNZFP	(0x1)
#define BODZF	(0x2)
#define BODZFP	(0x3)
#define BOF	(0x4)
#define BOFP	(0x5)
#define BODNZT	(0x8)
#define BODNZTP	(0x9)
#define BODZT	(0xa)
#define BODZTP	(0xb)
#define BOT	(0xc)
#define BOTP	(0xd)
#define BODNZ	(0x10)
#define BODNZP	(0x11)
#define BODZ	(0x12)
#define BODZP	(0x13)
#define BOU	(0x14)

/* The BI condition bit encodings used in extended conditional branch
mnemonics.  */
#define CBLT	(0)
#define CBGT	(1)
#define CBEQ	(2)
#define CBSO	(3)

/* The TO encodings used in extended trap mnemonics.  */
#define TOLGT	(0x1)
#define TOLLT	(0x2)
#define TOEQ	(0x4)
#define TOLGE	(0x5)
#define TOLNL	(0x5)
#define TOLLE	(0x6)
#define TOLNG	(0x6)
#define TOGT	(0x8)
#define TOGE	(0xc)
#define TONL	(0xc)
#define TOLT	(0x10)
#define TOLE	(0x14)
#define TONG	(0x14)
#define TONE	(0x18)
#define TOU	(0x1f)

/* The opcode tables. These tables includes almost all of the extended
 * instruction mnemonics. The format of the tables is:
 *
 *     NAME OPCODE MASK TYPE FLAGS		{ OPERANDS }
 *
 * NAME is the name of the instruction.
 *
 * OPCODE is the instruction opcode.
 *
 * MASK is the opcode mask; this is used to tell the disassembler
 * which bits in the actual opcode must match OPCODE.
 *
 * TYPE indicates the general classification of this command. This is
 * used by the comment generator.
 *
 * OPERANDS is the list of operands.
 *
 * The disassembler reads the tables in order and prints the first
 * instruction which matches, so the tables are sorted to put more
 * specific instructions before more general instructions.  There is one
 * table per major opcode.
 *
 * These tables only looks good when using 8 character tabs
 */

const PPCOpcode ppcOpcodes2[] =
{
{ "tdlgti",   OPTO(2,TOLGT),		OPTO_MASK,     OPTYPE_MISC, { RA, SI,0 }},
{ "tdllti",   OPTO(2,TOLLT),		OPTO_MASK,     OPTYPE_MISC, { RA, SI,0 }},
{ "tdeqi",    OPTO(2,TOEQ),		OPTO_MASK,     OPTYPE_MISC, { RA, SI,0 }},
{ "tdlgei",   OPTO(2,TOLGE),		OPTO_MASK,     OPTYPE_MISC, { RA, SI,0 }},
{ "tdlnli",   OPTO(2,TOLNL),		OPTO_MASK,     OPTYPE_MISC, { RA, SI,0 }},
{ "tdllei",   OPTO(2,TOLLE),		OPTO_MASK,     OPTYPE_MISC, { RA, SI,0 }},
{ "tdlngi",   OPTO(2,TOLNG),		OPTO_MASK,     OPTYPE_MISC, { RA, SI,0 }},
{ "tdgti",    OPTO(2,TOGT),		OPTO_MASK,     OPTYPE_MISC, { RA, SI,0 }},
{ "tdgei",    OPTO(2,TOGE),		OPTO_MASK,     OPTYPE_MISC, { RA, SI,0 }},
{ "tdnli",    OPTO(2,TONL),		OPTO_MASK,     OPTYPE_MISC, { RA, SI,0 }},
{ "tdlti",    OPTO(2,TOLT),		OPTO_MASK,     OPTYPE_MISC, { RA, SI,0 }},
{ "tdlei",    OPTO(2,TOLE),		OPTO_MASK,     OPTYPE_MISC, { RA, SI,0 }},
{ "tdngi",    OPTO(2,TONG),		OPTO_MASK,     OPTYPE_MISC, { RA, SI,0 }},
{ "tdnei",    OPTO(2,TONE),		OPTO_MASK,     OPTYPE_MISC, { RA, SI,0 }},
{ "tdi",      OP(2),			OP_MASK,       OPTYPE_MISC, { TO, RA, SI,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes3[] =
{
{ "twlgti",   OPTO(3,TOLGT),		OPTO_MASK,     OPTYPE_MISC, { RA, SI,0 }},
{ "twllti",   OPTO(3,TOLLT),		OPTO_MASK,     OPTYPE_MISC, { RA, SI,0 }},
{ "tweqi",    OPTO(3,TOEQ),		OPTO_MASK,     OPTYPE_MISC, { RA, SI,0 }},
{ "twlgei",   OPTO(3,TOLGE),		OPTO_MASK,     OPTYPE_MISC, { RA, SI,0 }},
{ "twlnli",   OPTO(3,TOLNL),		OPTO_MASK,     OPTYPE_MISC, { RA, SI,0 }},
{ "twllei",   OPTO(3,TOLLE),		OPTO_MASK,     OPTYPE_MISC, { RA, SI,0 }},
{ "twlngi",   OPTO(3,TOLNG),		OPTO_MASK,     OPTYPE_MISC, { RA, SI,0 }},
{ "twgti",    OPTO(3,TOGT),		OPTO_MASK,     OPTYPE_MISC, { RA, SI,0 }},
{ "twgei",    OPTO(3,TOGE),		OPTO_MASK,     OPTYPE_MISC, { RA, SI,0 }},
{ "twnli",    OPTO(3,TONL),		OPTO_MASK,     OPTYPE_MISC, { RA, SI,0 }},
{ "twlti",    OPTO(3,TOLT),		OPTO_MASK,     OPTYPE_MISC, { RA, SI,0 }},
{ "twlei",    OPTO(3,TOLE),		OPTO_MASK,     OPTYPE_MISC, { RA, SI,0 }},
{ "twngi",    OPTO(3,TONG),		OPTO_MASK,     OPTYPE_MISC, { RA, SI,0 }},
{ "twnei",    OPTO(3,TONE),		OPTO_MASK,     OPTYPE_MISC, { RA, SI,0 }},
{ "twi",      OP(3),			OP_MASK,       OPTYPE_MISC, { TO, RA, SI,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes7[] =
{
{ "mulli",    OP(7),			OP_MASK,       OPTYPE_MISC, { RT, RA, SI,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes8[] =
{
{ "subfic",   OP(8),			OP_MASK,       OPTYPE_MISC, { RT, RA, SI,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes10[] =
{
{ "cmplwi",   OPL(10,0),		OPL_MASK,      OPTYPE_MISC, { OBF, RA, UI,0 }},
{ "cmpldi",   OPL(10,1),		OPL_MASK,      OPTYPE_MISC, { OBF, RA, UI,0 }},
{ "cmpli",    OP(10),			OP_MASK,       OPTYPE_MISC, { BF, L, RA, UI,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes11[] =
{
{ "cmpwi",    OPL(11,0),		OPL_MASK,      OPTYPE_MISC, { OBF, RA, SI,0 }},
{ "cmpdi",    OPL(11,1),		OPL_MASK,      OPTYPE_MISC, { OBF, RA, SI,0 }},
{ "cmpi",     OP(11),			OP_MASK,       OPTYPE_MISC, { BF, L, RA, SI,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes12[] =
{
{ "addic",    OP(12),			OP_MASK,       OPTYPE_MISC, { RT, RA, SI,0 }},
{ "subic",    OP(12),			OP_MASK,       OPTYPE_MISC, { RT, RA, NSI,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes13[] =
{
{ "addic.",   OP(13),			OP_MASK,       OPTYPE_MISC, { RT, RA, SI,0 }},
{ "subic.",   OP(13),			OP_MASK,       OPTYPE_MISC, { RT, RA, NSI,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes14[] =
{
{ "li",       OP(14),			DRA_MASK,      OPTYPE_MISC, { RT, SI,0 }},
{ "mr",       OP(14),			(OP_MASK|0xffff), OPTYPE_MISC, { RT, RA,0 }},
{ "addi",     OP(14),			OP_MASK,       OPTYPE_MISC, { RT, RA, SI,0 }},
{ "subi",     OP(14),			OP_MASK,       OPTYPE_MISC, { RT, RA, NSI,0 }},
{ "la",       OP(14),			OP_MASK,       OPTYPE_MISC, { RT, D, RA,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes15[] =
{
{ "lis",      OP(15),			DRA_MASK,      OPTYPE_MISC, { RT, XUI,0 }},
{ "addis",    OP(15),			OP_MASK,       OPTYPE_MISC, { RT,RA,SISIGNOPT,0 }},
{ "subis",    OP(15),			OP_MASK,       OPTYPE_MISC, { RT, RA, NSI,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes16[] =
{
{ "bdnz-",    BBO(16,BODNZ,0,0),	BBOYBI_MASK,   OPTYPE_BRANCH, { BDM,0 }},
{ "bdnz+",    BBO(16,BODNZ,0,0),	BBOYBI_MASK,   OPTYPE_BRANCH, { BDP,0 }},
{ "bdnz",     BBO(16,BODNZ,0,0),	BBOYBI_MASK,   OPTYPE_BRANCH, { BD,0 }},
{ "bdnzl-",   BBO(16,BODNZ,0,1),	BBOYBI_MASK,   OPTYPE_BRANCH, { BDM,0 }},
{ "bdnzl+",   BBO(16,BODNZ,0,1),	BBOYBI_MASK,   OPTYPE_BRANCH, { BDP,0 }},
{ "bdnzl",    BBO(16,BODNZ,0,1),	BBOYBI_MASK,   OPTYPE_BRANCH, { BD,0 }},
{ "bdnza-",   BBO(16,BODNZ,1,0),	BBOYBI_MASK,   OPTYPE_BRANCH, { BDMA,0 }},
{ "bdnza+",   BBO(16,BODNZ,1,0),	BBOYBI_MASK,   OPTYPE_BRANCH, { BDPA,0 }},
{ "bdnza",    BBO(16,BODNZ,1,0),	BBOYBI_MASK,   OPTYPE_BRANCH, { BDA,0 }},
{ "bdnzla-",  BBO(16,BODNZ,1,1),	BBOYBI_MASK,   OPTYPE_BRANCH, { BDMA,0 }},
{ "bdnzla+",  BBO(16,BODNZ,1,1),	BBOYBI_MASK,   OPTYPE_BRANCH, { BDPA,0 }},
{ "bdnzla",   BBO(16,BODNZ,1,1),	BBOYBI_MASK,   OPTYPE_BRANCH, { BDA,0 }},
{ "bdz-",     BBO(16,BODZ,0,0), 	BBOYBI_MASK,   OPTYPE_BRANCH, { BDM,0 }},
{ "bdz+",     BBO(16,BODZ,0,0), 	BBOYBI_MASK,   OPTYPE_BRANCH, { BDP,0 }},
{ "bdz",      BBO(16,BODZ,0,0), 	BBOYBI_MASK,   OPTYPE_BRANCH, { BD,0 }},
{ "bdzl-",    BBO(16,BODZ,0,1), 	BBOYBI_MASK,   OPTYPE_BRANCH, { BDM,0 }},
{ "bdzl+",    BBO(16,BODZ,0,1), 	BBOYBI_MASK,   OPTYPE_BRANCH, { BDP,0 }},
{ "bdzl",     BBO(16,BODZ,0,1), 	BBOYBI_MASK,   OPTYPE_BRANCH, { BD,0 }},
{ "bdza-",    BBO(16,BODZ,1,0), 	BBOYBI_MASK,   OPTYPE_BRANCH, { BDMA,0 }},
{ "bdza+",    BBO(16,BODZ,1,0), 	BBOYBI_MASK,   OPTYPE_BRANCH, { BDPA,0 }},
{ "bdza",     BBO(16,BODZ,1,0), 	BBOYBI_MASK,   OPTYPE_BRANCH, { BDA,0 }},
{ "bdzla-",   BBO(16,BODZ,1,1), 	BBOYBI_MASK,   OPTYPE_BRANCH, { BDMA,0 }},
{ "bdzla+",   BBO(16,BODZ,1,1), 	BBOYBI_MASK,   OPTYPE_BRANCH, { BDPA,0 }},
{ "bdzla",    BBO(16,BODZ,1,1), 	BBOYBI_MASK,   OPTYPE_BRANCH, { BDA,0 }},
{ "blt-",     BBOCB(16,BOT,CBLT,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDM,0 }},
{ "blt+",     BBOCB(16,BOT,CBLT,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDP,0 }},
{ "blt",      BBOCB(16,BOT,CBLT,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BD }
},
{ "bltl-",    BBOCB(16,BOT,CBLT,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDM,0 }},
{ "bltl+",    BBOCB(16,BOT,CBLT,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDP,0 }},
{ "bltl",     BBOCB(16,BOT,CBLT,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BD,0 }},
{ "blta-",    BBOCB(16,BOT,CBLT,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDMA,0 }},
{ "blta+",    BBOCB(16,BOT,CBLT,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDPA,0 }},
{ "blta",     BBOCB(16,BOT,CBLT,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDA,0 }},
{ "bltla-",   BBOCB(16,BOT,CBLT,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDMA,0 }},
{ "bltla+",   BBOCB(16,BOT,CBLT,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDPA,0 }},
{ "bltla",    BBOCB(16,BOT,CBLT,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDA,0 }},
{ "bgt-",     BBOCB(16,BOT,CBGT,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDM,0 }},
{ "bgt+",     BBOCB(16,BOT,CBGT,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDP,0 }},
{ "bgt",      BBOCB(16,BOT,CBGT,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BD,0 }},
{ "bgtl-",    BBOCB(16,BOT,CBGT,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDM,0 }},
{ "bgtl+",    BBOCB(16,BOT,CBGT,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDP,0 }},
{ "bgtl",     BBOCB(16,BOT,CBGT,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BD,0 }},
{ "bgta-",    BBOCB(16,BOT,CBGT,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDMA,0 }},
{ "bgta+",    BBOCB(16,BOT,CBGT,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDPA,0 }},
{ "bgta",     BBOCB(16,BOT,CBGT,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDA,0 }},
{ "bgtla-",   BBOCB(16,BOT,CBGT,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDMA,0 }},
{ "bgtla+",   BBOCB(16,BOT,CBGT,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDPA,0 }},
{ "bgtla",    BBOCB(16,BOT,CBGT,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDA,0 }},
{ "beq-",     BBOCB(16,BOT,CBEQ,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDM,0 }},
{ "beq+",     BBOCB(16,BOT,CBEQ,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDP,0 }},
{ "beq",      BBOCB(16,BOT,CBEQ,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BD,0 }},
{ "beql-",    BBOCB(16,BOT,CBEQ,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDM,0 }},
{ "beql+",    BBOCB(16,BOT,CBEQ,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDP,0 }},
{ "beql",     BBOCB(16,BOT,CBEQ,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BD,0 }},
{ "beqa-",    BBOCB(16,BOT,CBEQ,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDMA,0 }},
{ "beqa+",    BBOCB(16,BOT,CBEQ,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDPA,0 }},
{ "beqa",     BBOCB(16,BOT,CBEQ,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDA,0 }},
{ "beqla-",   BBOCB(16,BOT,CBEQ,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDMA,0 }},
{ "beqla+",   BBOCB(16,BOT,CBEQ,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDPA,0 }},
{ "beqla",    BBOCB(16,BOT,CBEQ,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDA,0 }},
{ "bso-",     BBOCB(16,BOT,CBSO,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDM,0 }},
{ "bso+",     BBOCB(16,BOT,CBSO,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDP,0 }},
{ "bso",      BBOCB(16,BOT,CBSO,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BD,0 }},
{ "bsol-",    BBOCB(16,BOT,CBSO,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDM,0 }},
{ "bsol+",    BBOCB(16,BOT,CBSO,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDP,0 }},
{ "bsol",     BBOCB(16,BOT,CBSO,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BD,0 }},
{ "bsoa-",    BBOCB(16,BOT,CBSO,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDMA,0 }},
{ "bsoa+",    BBOCB(16,BOT,CBSO,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDPA,0 }},
{ "bsoa",     BBOCB(16,BOT,CBSO,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDA,0 }},
{ "bsola-",   BBOCB(16,BOT,CBSO,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDMA,0 }},
{ "bsola+",   BBOCB(16,BOT,CBSO,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDPA,0 }},
{ "bsola",    BBOCB(16,BOT,CBSO,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDA,0 }},
{ "bun-",     BBOCB(16,BOT,CBSO,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDM,0 }},
{ "bun+",     BBOCB(16,BOT,CBSO,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDP,0 }},
{ "bun",      BBOCB(16,BOT,CBSO,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BD,0 }},
{ "bunl-",    BBOCB(16,BOT,CBSO,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDM,0 }},
{ "bunl+",    BBOCB(16,BOT,CBSO,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDP,0 }},
{ "bunl",     BBOCB(16,BOT,CBSO,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BD,0 }},
{ "buna-",    BBOCB(16,BOT,CBSO,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDMA,0 }},
{ "buna+",    BBOCB(16,BOT,CBSO,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDPA,0 }},
{ "buna",     BBOCB(16,BOT,CBSO,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDA,0 }},
{ "bunla-",   BBOCB(16,BOT,CBSO,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDMA,0 }},
{ "bunla+",   BBOCB(16,BOT,CBSO,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDPA,0 }},
{ "bunla",    BBOCB(16,BOT,CBSO,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDA,0 }},
{ "bge-",     BBOCB(16,BOF,CBLT,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDM,0 }},
{ "bge+",     BBOCB(16,BOF,CBLT,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDP,0 }},
{ "bge",      BBOCB(16,BOF,CBLT,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BD,0 }},
{ "bgel-",    BBOCB(16,BOF,CBLT,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDM,0 }},
{ "bgel+",    BBOCB(16,BOF,CBLT,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDP,0 }},
{ "bgel",     BBOCB(16,BOF,CBLT,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BD,0 }},
{ "bgea-",    BBOCB(16,BOF,CBLT,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDMA,0 }},
{ "bgea+",    BBOCB(16,BOF,CBLT,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDPA,0 }},
{ "bgea",     BBOCB(16,BOF,CBLT,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDA,0 }},
{ "bgela-",   BBOCB(16,BOF,CBLT,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDMA,0 }},
{ "bgela+",   BBOCB(16,BOF,CBLT,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDPA,0 }},
{ "bgela",    BBOCB(16,BOF,CBLT,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDA,0 }},
{ "bnl-",     BBOCB(16,BOF,CBLT,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDM,0 }},
{ "bnl+",     BBOCB(16,BOF,CBLT,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDP,0 }},
{ "bnl",      BBOCB(16,BOF,CBLT,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BD,0 }},
{ "bnll-",    BBOCB(16,BOF,CBLT,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDM,0 }},
{ "bnll+",    BBOCB(16,BOF,CBLT,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDP,0 }},
{ "bnll",     BBOCB(16,BOF,CBLT,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BD,0 }},
{ "bnla-",    BBOCB(16,BOF,CBLT,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDMA,0 }},
{ "bnla+",    BBOCB(16,BOF,CBLT,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDPA,0 }},
{ "bnla",     BBOCB(16,BOF,CBLT,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDA,0 }},
{ "bnlla-",   BBOCB(16,BOF,CBLT,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDMA,0 }},
{ "bnlla+",   BBOCB(16,BOF,CBLT,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDPA,0 }},
{ "bnlla",    BBOCB(16,BOF,CBLT,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDA,0 }},
{ "ble-",     BBOCB(16,BOF,CBGT,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDM,0 }},
{ "ble+",     BBOCB(16,BOF,CBGT,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDP,0 }},
{ "ble",      BBOCB(16,BOF,CBGT,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BD,0 }},
{ "blel-",    BBOCB(16,BOF,CBGT,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDM,0 }},
{ "blel+",    BBOCB(16,BOF,CBGT,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDP,0 }},
{ "blel",     BBOCB(16,BOF,CBGT,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BD,0 }},
{ "blea-",    BBOCB(16,BOF,CBGT,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDMA,0 }},
{ "blea+",    BBOCB(16,BOF,CBGT,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDPA,0 }},
{ "blea",     BBOCB(16,BOF,CBGT,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDA,0 }},
{ "blela-",   BBOCB(16,BOF,CBGT,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDMA,0 }},
{ "blela+",   BBOCB(16,BOF,CBGT,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDPA,0 }},
{ "blela",    BBOCB(16,BOF,CBGT,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDA,0 }},
{ "bng-",     BBOCB(16,BOF,CBGT,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDM,0 }},
{ "bng+",     BBOCB(16,BOF,CBGT,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDP,0 }},
{ "bng",      BBOCB(16,BOF,CBGT,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BD,0 }},
{ "bngl-",    BBOCB(16,BOF,CBGT,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDM,0 }},
{ "bngl+",    BBOCB(16,BOF,CBGT,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDP,0 }},
{ "bngl",     BBOCB(16,BOF,CBGT,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BD,0 }},
{ "bnga-",    BBOCB(16,BOF,CBGT,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDMA,0 }},
{ "bnga+",    BBOCB(16,BOF,CBGT,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDPA,0 }},
{ "bnga",     BBOCB(16,BOF,CBGT,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDA,0 }},
{ "bngla-",   BBOCB(16,BOF,CBGT,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDMA,0 }},
{ "bngla+",   BBOCB(16,BOF,CBGT,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDPA,0 }},
{ "bngla",    BBOCB(16,BOF,CBGT,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDA,0 }},
{ "bne-",     BBOCB(16,BOF,CBEQ,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDM,0 }},
{ "bne+",     BBOCB(16,BOF,CBEQ,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDP,0 }},
{ "bne",      BBOCB(16,BOF,CBEQ,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BD,0 }},
{ "bnel-",    BBOCB(16,BOF,CBEQ,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDM,0 }},
{ "bnel+",    BBOCB(16,BOF,CBEQ,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDP,0 }},
{ "bnel",     BBOCB(16,BOF,CBEQ,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BD,0 }},
{ "bnea-",    BBOCB(16,BOF,CBEQ,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDMA,0 }},
{ "bnea+",    BBOCB(16,BOF,CBEQ,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDPA,0 }},
{ "bnea",     BBOCB(16,BOF,CBEQ,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDA,0 }},
{ "bnela-",   BBOCB(16,BOF,CBEQ,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDMA,0 }},
{ "bnela+",   BBOCB(16,BOF,CBEQ,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDPA,0 }},
{ "bnela",    BBOCB(16,BOF,CBEQ,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDA,0 }},
{ "bns-",     BBOCB(16,BOF,CBSO,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDM,0 }},
{ "bns+",     BBOCB(16,BOF,CBSO,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDP,0 }},
{ "bns",      BBOCB(16,BOF,CBSO,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BD,0 }},
{ "bnsl-",    BBOCB(16,BOF,CBSO,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDM,0 }},
{ "bnsl+",    BBOCB(16,BOF,CBSO,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDP,0 }},
{ "bnsl",     BBOCB(16,BOF,CBSO,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BD,0 }},
{ "bnsa-",    BBOCB(16,BOF,CBSO,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDMA,0 }},
{ "bnsa+",    BBOCB(16,BOF,CBSO,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDPA,0 }},
{ "bnsa",     BBOCB(16,BOF,CBSO,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDA,0 }},
{ "bnsla-",   BBOCB(16,BOF,CBSO,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDMA,0 }},
{ "bnsla+",   BBOCB(16,BOF,CBSO,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDPA,0 }},
{ "bnsla",    BBOCB(16,BOF,CBSO,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDA,0 }},
{ "bnu-",     BBOCB(16,BOF,CBSO,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDM,0 }},
{ "bnu+",     BBOCB(16,BOF,CBSO,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDP,0 }},
{ "bnu",      BBOCB(16,BOF,CBSO,0,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BD,0 }},
{ "bnul-",    BBOCB(16,BOF,CBSO,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDM,0 }},
{ "bnul+",    BBOCB(16,BOF,CBSO,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDP,0 }},
{ "bnul",     BBOCB(16,BOF,CBSO,0,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BD,0 }},
{ "bnua-",    BBOCB(16,BOF,CBSO,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDMA,0 }},
{ "bnua+",    BBOCB(16,BOF,CBSO,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDPA,0 }},
{ "bnua",     BBOCB(16,BOF,CBSO,1,0),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDA,0 }},
{ "bnula-",   BBOCB(16,BOF,CBSO,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDMA,0 }},
{ "bnula+",   BBOCB(16,BOF,CBSO,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDPA,0 }},
{ "bnula",    BBOCB(16,BOF,CBSO,1,1),	BBOYCB_MASK,   OPTYPE_BRANCH, { CR, BDA,0 }},
{ "bdnzt-",   BBO(16,BODNZT,0,0),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDM,0 }},
{ "bdnzt+",   BBO(16,BODNZT,0,0),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDP,0 }},
{ "bdnzt",    BBO(16,BODNZT,0,0),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BD,0 }},
{ "bdnztl-",  BBO(16,BODNZT,0,1),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDM,0 }},
{ "bdnztl+",  BBO(16,BODNZT,0,1),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDP,0 }},
{ "bdnztl",   BBO(16,BODNZT,0,1),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BD,0 }},
{ "bdnzta-",  BBO(16,BODNZT,1,0),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDMA,0 }},
{ "bdnzta+",  BBO(16,BODNZT,1,0),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDPA,0 }},
{ "bdnzta",   BBO(16,BODNZT,1,0),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDA,0 }},
{ "bdnztla-", BBO(16,BODNZT,1,1),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDMA,0 }},
{ "bdnztla+", BBO(16,BODNZT,1,1),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDPA,0 }},
{ "bdnztla",  BBO(16,BODNZT,1,1),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDA,0 }},
{ "bdnzf-",   BBO(16,BODNZF,0,0),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDM,0 }},
{ "bdnzf+",   BBO(16,BODNZF,0,0),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDP,0 }},
{ "bdnzf",    BBO(16,BODNZF,0,0),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BD,0 }},
{ "bdnzfl-",  BBO(16,BODNZF,0,1),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDM,0 }},
{ "bdnzfl+",  BBO(16,BODNZF,0,1),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDP,0 }},
{ "bdnzfl",   BBO(16,BODNZF,0,1),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BD,0 }},
{ "bdnzfa-",  BBO(16,BODNZF,1,0),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDMA,0 }},
{ "bdnzfa+",  BBO(16,BODNZF,1,0),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDPA,0 }},
{ "bdnzfa",   BBO(16,BODNZF,1,0),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDA,0 }},
{ "bdnzfla-", BBO(16,BODNZF,1,1),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDMA,0 }},
{ "bdnzfla+", BBO(16,BODNZF,1,1),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDPA,0 }},
{ "bdnzfla",  BBO(16,BODNZF,1,1),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDA,0 }},
{ "bt-",      BBO(16,BOT,0,0),		BBOY_MASK,     OPTYPE_BRANCH, { BI, BDM,0 }},
{ "bt+",      BBO(16,BOT,0,0),		BBOY_MASK,     OPTYPE_BRANCH, { BI, BDP,0 }},
{ "bt",       BBO(16,BOT,0,0),		BBOY_MASK,     OPTYPE_BRANCH, { BI, BD,0 }},
{ "btl-",     BBO(16,BOT,0,1),		BBOY_MASK,     OPTYPE_BRANCH, { BI, BDM,0 }},
{ "btl+",     BBO(16,BOT,0,1),		BBOY_MASK,     OPTYPE_BRANCH, { BI, BDP,0 }},
{ "btl",      BBO(16,BOT,0,1),		BBOY_MASK,     OPTYPE_BRANCH, { BI, BD,0 }},
{ "bta-",     BBO(16,BOT,1,0),		BBOY_MASK,     OPTYPE_BRANCH, { BI, BDMA,0 }},
{ "bta+",     BBO(16,BOT,1,0),		BBOY_MASK,     OPTYPE_BRANCH, { BI, BDPA,0 }},
{ "bta",      BBO(16,BOT,1,0),		BBOY_MASK,     OPTYPE_BRANCH, { BI, BDA,0 }},
{ "btla-",    BBO(16,BOT,1,1),		BBOY_MASK,     OPTYPE_BRANCH, { BI, BDMA,0 }},
{ "btla+",    BBO(16,BOT,1,1),		BBOY_MASK,     OPTYPE_BRANCH, { BI, BDPA,0 }},
{ "btla",     BBO(16,BOT,1,1),		BBOY_MASK,     OPTYPE_BRANCH, { BI, BDA,0 }},
{ "bf-",      BBO(16,BOF,0,0),		BBOY_MASK,     OPTYPE_BRANCH, { BI, BDM,0 }},
{ "bf+",      BBO(16,BOF,0,0),		BBOY_MASK,     OPTYPE_BRANCH, { BI, BDP,0 }},
{ "bf",       BBO(16,BOF,0,0),		BBOY_MASK,     OPTYPE_BRANCH, { BI, BD,0 }},
{ "bfl-",     BBO(16,BOF,0,1),		BBOY_MASK,     OPTYPE_BRANCH, { BI, BDM,0 }},
{ "bfl+",     BBO(16,BOF,0,1),		BBOY_MASK,     OPTYPE_BRANCH, { BI, BDP,0 }},
{ "bfl",      BBO(16,BOF,0,1),		BBOY_MASK,     OPTYPE_BRANCH, { BI, BD,0 }},
{ "bfa-",     BBO(16,BOF,1,0),		BBOY_MASK,     OPTYPE_BRANCH, { BI, BDMA,0 }},
{ "bfa+",     BBO(16,BOF,1,0),		BBOY_MASK,     OPTYPE_BRANCH, { BI, BDPA,0 }},
{ "bfa",      BBO(16,BOF,1,0),		BBOY_MASK,     OPTYPE_BRANCH, { BI, BDA,0 }},
{ "bfla-",    BBO(16,BOF,1,1),		BBOY_MASK,     OPTYPE_BRANCH, { BI, BDMA,0 }},
{ "bfla+",    BBO(16,BOF,1,1),		BBOY_MASK,     OPTYPE_BRANCH, { BI, BDPA,0 }},
{ "bfla",     BBO(16,BOF,1,1),		BBOY_MASK,     OPTYPE_BRANCH, { BI, BDA,0 }},
{ "bdzt-",    BBO(16,BODZT,0,0),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDM,0 }},
{ "bdzt+",    BBO(16,BODZT,0,0),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDP,0 }},
{ "bdzt",     BBO(16,BODZT,0,0),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BD,0 }},
{ "bdztl-",   BBO(16,BODZT,0,1),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDM,0 }},
{ "bdztl+",   BBO(16,BODZT,0,1),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDP,0 }},
{ "bdztl",    BBO(16,BODZT,0,1),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BD,0 }},
{ "bdzta-",   BBO(16,BODZT,1,0),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDMA,0 }},
{ "bdzta+",   BBO(16,BODZT,1,0),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDPA,0 }},
{ "bdzta",    BBO(16,BODZT,1,0),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDA,0 }},
{ "bdztla-",  BBO(16,BODZT,1,1),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDMA,0 }},
{ "bdztla+",  BBO(16,BODZT,1,1),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDPA,0 }},
{ "bdztla",   BBO(16,BODZT,1,1),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDA,0 }},
{ "bdzf-",    BBO(16,BODZF,0,0),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDM,0 }},
{ "bdzf+",    BBO(16,BODZF,0,0),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDP,0 }},
{ "bdzf",     BBO(16,BODZF,0,0),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BD,0 }},
{ "bdzfl-",   BBO(16,BODZF,0,1),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDM,0 }},
{ "bdzfl+",   BBO(16,BODZF,0,1),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDP,0 }},
{ "bdzfl",    BBO(16,BODZF,0,1),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BD,0 }},
{ "bdzfa-",   BBO(16,BODZF,1,0),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDMA,0 }},
{ "bdzfa+",   BBO(16,BODZF,1,0),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDPA,0 }},
{ "bdzfa",    BBO(16,BODZF,1,0),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDA,0 }},
{ "bdzfla-",  BBO(16,BODZF,1,1),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDMA,0 }},
{ "bdzfla+",  BBO(16,BODZF,1,1),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDPA,0 }},
{ "bdzfla",   BBO(16,BODZF,1,1),	BBOY_MASK,     OPTYPE_BRANCH, { BI, BDA,0 }},
{ "bc-",      B(16,0,0),		B_MASK,        OPTYPE_BRANCH, { BOE, BI, BDM,0 }},
{ "bc+",      B(16,0,0),		B_MASK,        OPTYPE_BRANCH, { BOE, BI, BDP,0 }},
{ "bc",       B(16,0,0),		B_MASK,        OPTYPE_BRANCH, { BO, BI, BD,0 }},
{ "bcl-",     B(16,0,1),		B_MASK,        OPTYPE_BRANCH, { BOE, BI, BDM,0 }},
{ "bcl+",     B(16,0,1),		B_MASK,        OPTYPE_BRANCH, { BOE, BI, BDP,0 }},
{ "bcl",      B(16,0,1),		B_MASK,        OPTYPE_BRANCH, { BO, BI, BD,0 }},
{ "bca-",     B(16,1,0),		B_MASK,        OPTYPE_BRANCH, { BOE, BI, BDMA,0 }},
{ "bca+",     B(16,1,0),		B_MASK,        OPTYPE_BRANCH, { BOE, BI, BDPA,0 }},
{ "bca",      B(16,1,0),		B_MASK,        OPTYPE_BRANCH, { BO, BI, BDA,0 }},
{ "bcla-",    B(16,1,1),		B_MASK,        OPTYPE_BRANCH, { BOE, BI, BDMA,0 }},
{ "bcla+",    B(16,1,1),		B_MASK,        OPTYPE_BRANCH, { BOE, BI, BDPA,0 }},
{ "bcla",     B(16,1,1),		B_MASK,        OPTYPE_BRANCH, { BO, BI, BDA,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes17[] =
{
{ "sc",       SC(17,1,0),		0xffffffff,    OPTYPE_MISC, { 0,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes18[] =
{
{ "b",	      B(18,0,0),		B_MASK,        OPTYPE_BRANCH, { LI,0 }},
{ "bl",       B(18,0,1),		B_MASK,        OPTYPE_BRANCH, { LI,0 }},
{ "ba",       B(18,1,0),		B_MASK,        OPTYPE_BRANCH, { LIA,0 }},
{ "bla",      B(18,1,1),		B_MASK,        OPTYPE_BRANCH, { LIA,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes19[] =
{
{ "mcrf",     XL(19,0),			XLBB_MASK|(3<<21)|(3<<16), OPTYPE_MISC, { BF, BFA,0 }},
{ "blr",      XLO(19,BOU,16,0), 	XLBOBIBB_MASK, OPTYPE_BRANCH, { 0 }},
{ "blrl",     XLO(19,BOU,16,1), 	XLBOBIBB_MASK, OPTYPE_BRANCH, { 0 }},
{ "bdnzlr",   XLO(19,BODNZ,16,0),	XLBOBIBB_MASK, OPTYPE_BRANCH, { 0 }},
{ "bdnzlr-",  XLO(19,BODNZ,16,0),	XLBOBIBB_MASK, OPTYPE_BRANCH, { 0 }},
{ "bdnzlr+",  XLO(19,BODNZP,16,0),	XLBOBIBB_MASK, OPTYPE_BRANCH, { 0 }},
{ "bdnzlrl",  XLO(19,BODNZ,16,1),	XLBOBIBB_MASK, OPTYPE_BRANCH, { 0 }},
{ "bdnzlrl-", XLO(19,BODNZ,16,1),	XLBOBIBB_MASK, OPTYPE_BRANCH, { 0 } },
{ "bdnzlrl+", XLO(19,BODNZP,16,1),	XLBOBIBB_MASK, OPTYPE_BRANCH, { 0 } },
{ "bdzlr",    XLO(19,BODZ,16,0),	XLBOBIBB_MASK, OPTYPE_BRANCH, { 0 } },
{ "bdzlr-",   XLO(19,BODZ,16,0),	XLBOBIBB_MASK, OPTYPE_BRANCH, { 0 } },
{ "bdzlr+",   XLO(19,BODZP,16,0),	XLBOBIBB_MASK, OPTYPE_BRANCH, { 0 } },
{ "bdzlrl",   XLO(19,BODZ,16,1),	XLBOBIBB_MASK, OPTYPE_BRANCH, { 0 } },
{ "bdzlrl-",  XLO(19,BODZ,16,1),	XLBOBIBB_MASK, OPTYPE_BRANCH, { 0 } },
{ "bdzlrl+",  XLO(19,BODZP,16,1),	XLBOBIBB_MASK, OPTYPE_BRANCH, { 0 } },
{ "bltlr",    XLOCB(19,BOT,CBLT,16,0),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bltlr-",   XLOCB(19,BOT,CBLT,16,0),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bltlr+",   XLOCB(19,BOTP,CBLT,16,0), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bltlrl",   XLOCB(19,BOT,CBLT,16,1),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bltlrl-",  XLOCB(19,BOT,CBLT,16,1),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bltlrl+",  XLOCB(19,BOTP,CBLT,16,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bgtlr",    XLOCB(19,BOT,CBGT,16,0),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bgtlr-",   XLOCB(19,BOT,CBGT,16,0),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bgtlr+",   XLOCB(19,BOTP,CBGT,16,0), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bgtlrl",   XLOCB(19,BOT,CBGT,16,1),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bgtlrl-",  XLOCB(19,BOT,CBGT,16,1),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bgtlrl+",  XLOCB(19,BOTP,CBGT,16,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "beqlr",    XLOCB(19,BOT,CBEQ,16,0),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "beqlr-",   XLOCB(19,BOT,CBEQ,16,0),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "beqlr+",   XLOCB(19,BOTP,CBEQ,16,0), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "beqlrl",   XLOCB(19,BOT,CBEQ,16,1),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "beqlrl-",  XLOCB(19,BOT,CBEQ,16,1),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "beqlrl+",  XLOCB(19,BOTP,CBEQ,16,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bsolr",    XLOCB(19,BOT,CBSO,16,0),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bsolr-",   XLOCB(19,BOT,CBSO,16,0),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bsolr+",   XLOCB(19,BOTP,CBSO,16,0), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bsolrl",   XLOCB(19,BOT,CBSO,16,1),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bsolrl-",  XLOCB(19,BOT,CBSO,16,1),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bsolrl+",  XLOCB(19,BOTP,CBSO,16,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bunlr",    XLOCB(19,BOT,CBSO,16,0),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bunlr-",   XLOCB(19,BOT,CBSO,16,0),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bunlr+",   XLOCB(19,BOTP,CBSO,16,0), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bunlrl",   XLOCB(19,BOT,CBSO,16,1),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bunlrl-",  XLOCB(19,BOT,CBSO,16,1),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bunlrl+",  XLOCB(19,BOTP,CBSO,16,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bgelr",    XLOCB(19,BOF,CBLT,16,0),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bgelr-",   XLOCB(19,BOF,CBLT,16,0),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bgelr+",   XLOCB(19,BOFP,CBLT,16,0), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bgelrl",   XLOCB(19,BOF,CBLT,16,1),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bgelrl-",  XLOCB(19,BOF,CBLT,16,1),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bgelrl+",  XLOCB(19,BOFP,CBLT,16,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnllr",    XLOCB(19,BOF,CBLT,16,0),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnllr-",   XLOCB(19,BOF,CBLT,16,0),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnllr+",   XLOCB(19,BOFP,CBLT,16,0), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnllrl",   XLOCB(19,BOF,CBLT,16,1),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnllrl-",  XLOCB(19,BOF,CBLT,16,1),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnllrl+",  XLOCB(19,BOFP,CBLT,16,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "blelr",    XLOCB(19,BOF,CBGT,16,0),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "blelr-",   XLOCB(19,BOF,CBGT,16,0),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "blelr+",   XLOCB(19,BOFP,CBGT,16,0), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "blelrl",   XLOCB(19,BOF,CBGT,16,1),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "blelrl-",  XLOCB(19,BOF,CBGT,16,1),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "blelrl+",  XLOCB(19,BOFP,CBGT,16,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnglr",    XLOCB(19,BOF,CBGT,16,0),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnglr-",   XLOCB(19,BOF,CBGT,16,0),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnglr+",   XLOCB(19,BOFP,CBGT,16,0), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnglrl",   XLOCB(19,BOF,CBGT,16,1),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnglrl-",  XLOCB(19,BOF,CBGT,16,1),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnglrl+",  XLOCB(19,BOFP,CBGT,16,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnelr",    XLOCB(19,BOF,CBEQ,16,0),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnelr-",   XLOCB(19,BOF,CBEQ,16,0),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnelr+",   XLOCB(19,BOFP,CBEQ,16,0), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnelrl",   XLOCB(19,BOF,CBEQ,16,1),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnelrl-",  XLOCB(19,BOF,CBEQ,16,1),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnelrl+",  XLOCB(19,BOFP,CBEQ,16,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnslr",    XLOCB(19,BOF,CBSO,16,0),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnslr-",   XLOCB(19,BOF,CBSO,16,0),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnslr+",   XLOCB(19,BOFP,CBSO,16,0), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnslrl",   XLOCB(19,BOF,CBSO,16,1),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnslrl-",  XLOCB(19,BOF,CBSO,16,1),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnslrl+",  XLOCB(19,BOFP,CBSO,16,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnulr",    XLOCB(19,BOF,CBSO,16,0),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnulr-",   XLOCB(19,BOF,CBSO,16,0),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnulr+",   XLOCB(19,BOFP,CBSO,16,0), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnulrl",   XLOCB(19,BOF,CBSO,16,1),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnulrl-",  XLOCB(19,BOF,CBSO,16,1),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnulrl+",  XLOCB(19,BOFP,CBSO,16,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "btlr",     XLO(19,BOT,16,0), 	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "btlr-",    XLO(19,BOT,16,0), 	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "btlr+",    XLO(19,BOTP,16,0),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "btlrl",    XLO(19,BOT,16,1), 	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "btlrl-",   XLO(19,BOT,16,1), 	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "btlrl+",   XLO(19,BOTP,16,1),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bflr",     XLO(19,BOF,16,0), 	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bflr-",    XLO(19,BOF,16,0), 	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bflr+",    XLO(19,BOFP,16,0),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bflrl",    XLO(19,BOF,16,1), 	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bflrl-",   XLO(19,BOF,16,1), 	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bflrl+",   XLO(19,BOFP,16,1),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bdnztlr",  XLO(19,BODNZT,16,0),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bdnztlr-", XLO(19,BODNZT,16,0),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bdnztlr+", XLO(19,BODNZTP,16,0),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bdnztlrl", XLO(19,BODNZT,16,1),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bdnztlrl-",XLO(19,BODNZT,16,1),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bdnztlrl+",XLO(19,BODNZTP,16,1),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bdnzflr",  XLO(19,BODNZF,16,0),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bdnzflr-", XLO(19,BODNZF,16,0),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bdnzflr+", XLO(19,BODNZFP,16,0),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bdnzflrl", XLO(19,BODNZF,16,1),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bdnzflrl-",XLO(19,BODNZF,16,1),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bdnzflrl+",XLO(19,BODNZFP,16,1),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bdztlr",   XLO(19,BODZT,16,0),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bdztlr-",  XLO(19,BODZT,16,0),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bdztlr+",  XLO(19,BODZTP,16,0),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bdztlrl",  XLO(19,BODZT,16,1),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bdztlrl-", XLO(19,BODZT,16,1),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bdztlrl+", XLO(19,BODZTP,16,1),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bdzflr",   XLO(19,BODZF,16,0),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bdzflr-",  XLO(19,BODZF,16,0),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bdzflr+",  XLO(19,BODZFP,16,0),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bdzflrl",  XLO(19,BODZF,16,1),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bdzflrl-", XLO(19,BODZF,16,1),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bdzflrl+", XLO(19,BODZFP,16,1),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bclr",     XLLK(19,16,0),		XLYBB_MASK,    OPTYPE_BRANCH, { BO, BI,0 }},
{ "bclrl",    XLLK(19,16,1),		XLYBB_MASK,    OPTYPE_BRANCH, { BO, BI,0 }},
{ "bclr+",    XLYLK(19,16,1,0),		XLYBB_MASK,    OPTYPE_BRANCH, { BOE, BI,0 }},
{ "bclrl+",   XLYLK(19,16,1,1),		XLYBB_MASK,    OPTYPE_BRANCH, { BOE, BI,0 }},
{ "bclr-",    XLYLK(19,16,0,0),		XLYBB_MASK,    OPTYPE_BRANCH, { BOE, BI,0 }},
{ "bclrl-",   XLYLK(19,16,0,1),		XLYBB_MASK,    OPTYPE_BRANCH, { BOE, BI,0 }},
{ "crnot",    XL(19,33),		XL_MASK,       OPTYPE_MISC, { BT, BA, BBA,0 }},
{ "crnor",    XL(19,33),		XL_MASK,       OPTYPE_MISC, { BT, BA, BB,0 }},
{ "rfi",      XL(19,50),		0xffffffff,    OPTYPE_RFI,  { 0,0 }},
{ "crandc",   XL(19,129),		XL_MASK,       OPTYPE_MISC, { BT, BA, BB,0 }},
{ "isync",    XL(19,150),		0xffffffff,    OPTYPE_MISC, { 0,0 }},
{ "crclr",    XL(19,193),		XL_MASK,       OPTYPE_MISC, { BT, BAT, BBA,0 }},
{ "crxor",    XL(19,193),		XL_MASK,       OPTYPE_MISC, { BT, BA, BB,0 }},
{ "crnand",   XL(19,225),		XL_MASK,       OPTYPE_MISC, { BT, BA, BB,0 }},
{ "crand",    XL(19,257),		XL_MASK,       OPTYPE_MISC, { BT, BA, BB,0 }},
{ "crset",    XL(19,289),		XL_MASK,       OPTYPE_MISC, { BT, BAT, BBA,0 }},
{ "creqv",    XL(19,289),		XL_MASK,       OPTYPE_MISC, { BT, BA, BB,0 }},
{ "crorc",    XL(19,417),		XL_MASK,       OPTYPE_MISC, { BT, BA, BB,0 }},
{ "crmove",   XL(19,449),		XL_MASK,       OPTYPE_MISC, { BT, BA, BBA,0 }},
{ "cror",     XL(19,449),		XL_MASK,       OPTYPE_MISC, { BT, BA, BB,0 }},
{ "bctr",     XLO(19,BOU,528,0),	XLBOBIBB_MASK, OPTYPE_BRANCH, { 0 }},
{ "bctrl",    XLO(19,BOU,528,1),	XLBOBIBB_MASK, OPTYPE_BRANCH, { 0 }},
{ "bltctr",   XLOCB(19,BOT,CBLT,528,0),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bltctr-",  XLOCB(19,BOT,CBLT,528,0),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bltctr+",  XLOCB(19,BOTP,CBLT,528,0),XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bltctrl",  XLOCB(19,BOT,CBLT,528,1),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bltctrl-", XLOCB(19,BOT,CBLT,528,1),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bltctrl+", XLOCB(19,BOTP,CBLT,528,1),XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bgtctr",   XLOCB(19,BOT,CBGT,528,0),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bgtctr-",  XLOCB(19,BOT,CBGT,528,0),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bgtctr+",  XLOCB(19,BOTP,CBGT,528,0),XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bgtctrl",  XLOCB(19,BOT,CBGT,528,1),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bgtctrl-", XLOCB(19,BOT,CBGT,528,1),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bgtctrl+", XLOCB(19,BOTP,CBGT,528,1),XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "beqctr",   XLOCB(19,BOT,CBEQ,528,0),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "beqctr-",  XLOCB(19,BOT,CBEQ,528,0),	XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "beqctr+",  XLOCB(19,BOTP,CBEQ,528,0),XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "beqctrl",  XLOCB(19,BOT,CBEQ,528,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "beqctrl-", XLOCB(19,BOT,CBEQ,528,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "beqctrl+", XLOCB(19,BOTP,CBEQ,528,1),XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bsoctr",   XLOCB(19,BOT,CBSO,528,0), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bsoctr-",  XLOCB(19,BOT,CBSO,528,0), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bsoctr+",  XLOCB(19,BOTP,CBSO,528,0),XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bsoctrl",  XLOCB(19,BOT,CBSO,528,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bsoctrl-", XLOCB(19,BOT,CBSO,528,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bsoctrl+", XLOCB(19,BOTP,CBSO,528,1),XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bunctr",   XLOCB(19,BOT,CBSO,528,0), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bunctr-",  XLOCB(19,BOT,CBSO,528,0), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bunctr+",  XLOCB(19,BOTP,CBSO,528,0),XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bunctrl",  XLOCB(19,BOT,CBSO,528,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bunctrl-", XLOCB(19,BOT,CBSO,528,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bunctrl+", XLOCB(19,BOTP,CBSO,528,1),XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bgectr",   XLOCB(19,BOF,CBLT,528,0), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bgectr-",  XLOCB(19,BOF,CBLT,528,0), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bgectr+",  XLOCB(19,BOFP,CBLT,528,0),XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bgectrl",  XLOCB(19,BOF,CBLT,528,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bgectrl-", XLOCB(19,BOF,CBLT,528,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bgectrl+", XLOCB(19,BOFP,CBLT,528,1),XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnlctr",   XLOCB(19,BOF,CBLT,528,0), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnlctr-",  XLOCB(19,BOF,CBLT,528,0), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnlctr+",  XLOCB(19,BOFP,CBLT,528,0),XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnlctrl",  XLOCB(19,BOF,CBLT,528,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnlctrl-", XLOCB(19,BOF,CBLT,528,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnlctrl+", XLOCB(19,BOFP,CBLT,528,1),XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "blectr",   XLOCB(19,BOF,CBGT,528,0), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "blectr-",  XLOCB(19,BOF,CBGT,528,0), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "blectr+",  XLOCB(19,BOFP,CBGT,528,0),XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "blectrl",  XLOCB(19,BOF,CBGT,528,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "blectrl-", XLOCB(19,BOF,CBGT,528,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "blectrl+", XLOCB(19,BOFP,CBGT,528,1),XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bngctr",   XLOCB(19,BOF,CBGT,528,0), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bngctr-",  XLOCB(19,BOF,CBGT,528,0), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bngctr+",  XLOCB(19,BOFP,CBGT,528,0),XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bngctrl",  XLOCB(19,BOF,CBGT,528,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bngctrl-", XLOCB(19,BOF,CBGT,528,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bngctrl+", XLOCB(19,BOFP,CBGT,528,1),XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnectr",   XLOCB(19,BOF,CBEQ,528,0), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnectr-",  XLOCB(19,BOF,CBEQ,528,0), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnectr+",  XLOCB(19,BOFP,CBEQ,528,0),XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnectrl",  XLOCB(19,BOF,CBEQ,528,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnectrl-", XLOCB(19,BOF,CBEQ,528,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnectrl+", XLOCB(19,BOFP,CBEQ,528,1),XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnsctr",   XLOCB(19,BOF,CBSO,528,0), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnsctr-",  XLOCB(19,BOF,CBSO,528,0), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnsctr+",  XLOCB(19,BOFP,CBSO,528,0),XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnsctrl",  XLOCB(19,BOF,CBSO,528,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnsctrl-", XLOCB(19,BOF,CBSO,528,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnsctrl+", XLOCB(19,BOFP,CBSO,528,1),XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnuctr",   XLOCB(19,BOF,CBSO,528,0), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnuctr-",  XLOCB(19,BOF,CBSO,528,0), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnuctr+",  XLOCB(19,BOFP,CBSO,528,0),XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnuctrl",  XLOCB(19,BOF,CBSO,528,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnuctrl-", XLOCB(19,BOF,CBSO,528,1), XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "bnuctrl+", XLOCB(19,BOFP,CBSO,528,1),XLBOCBBB_MASK, OPTYPE_BRANCH, { CR,0 }},
{ "btctr",    XLO(19,BOT,528,0),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "btctr-",   XLO(19,BOT,528,0),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "btctr+",   XLO(19,BOTP,528,0),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "btctrl",   XLO(19,BOT,528,1),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "btctrl-",  XLO(19,BOT,528,1),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "btctrl+",  XLO(19,BOTP,528,1),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bfctr",    XLO(19,BOF,528,0),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bfctr-",   XLO(19,BOF,528,0),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bfctr+",   XLO(19,BOFP,528,0),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bfctrl",   XLO(19,BOF,528,1),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bfctrl-",  XLO(19,BOF,528,1),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bfctrl+",  XLO(19,BOFP,528,1),	XLBOBB_MASK,   OPTYPE_BRANCH, { BI,0 }},
{ "bcctr",    XLLK(19,528,0),		XLYBB_MASK,    OPTYPE_BRANCH, { BO, BI,0 }},
{ "bcctr-",   XLYLK(19,528,0,0),	XLYBB_MASK,    OPTYPE_BRANCH, { BOE, BI,0 }},
{ "bcctr+",   XLYLK(19,528,1,0),	XLYBB_MASK,    OPTYPE_BRANCH, { BOE, BI,0 }},
{ "bcctrl",   XLLK(19,528,1),		XLYBB_MASK,    OPTYPE_BRANCH, { BO, BI,0 }},
{ "bcctrl-",  XLYLK(19,528,0,1),	XLYBB_MASK,    OPTYPE_BRANCH, { BOE, BI,0 }},
{ "bcctrl+",  XLYLK(19,528,1,1),	XLYBB_MASK,    OPTYPE_BRANCH, { BOE, BI,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes20[] =
{
{ "rlwimi",   M(20,0),			M_MASK,        OPTYPE_MISC, { RA,RS,SH,MBE,ME,0 }},
{ "rlwimi.",  M(20,1),			M_MASK,        OPTYPE_MISC, { RA,RS,SH,MBE,ME,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes21[] =
{
{ "rotlwi",   MME(21,31,0),		MMBME_MASK,    OPTYPE_MISC, { RA, RS, SH,0 }},
{ "clrlwi",   MME(21,31,0),		MSHME_MASK,    OPTYPE_MISC, { RA, RS, MB,0 }},
{ "rlwinm",   M(21,0),			M_MASK,        OPTYPE_MISC, { RA,RS,SH,MBE,ME,0 }},
{ "rotlwi.",  MME(21,31,1),		MMBME_MASK,    OPTYPE_MISC, { RA,RS,SH,0 }},
{ "clrlwi.",  MME(21,31,1),		MSHME_MASK,    OPTYPE_MISC, { RA, RS, MB,0 }},
{ "rlwinm.",  M(21,1),			M_MASK,        OPTYPE_MISC, { RA,RS,SH,MBE,ME,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes23[] =
{
{ "rotlw",    MME(23,31,0),	MMBME_MASK,	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "rlwnm",    M(23,0),		M_MASK, 	   OPTYPE_MISC, { RA,RS,RB,MBE,ME,0 }},
{ "rotlw.",   MME(23,31,1),	MMBME_MASK,	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "rlwnm.",   M(23,1),		M_MASK, 	   OPTYPE_MISC, { RA,RS,RB,MBE,ME,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes24[] =
{
{ "nop",      OP(24),		0xffffffff,	   OPTYPE_MISC, { 0,0 }},
{ "ori",      OP(24),		OP_MASK,	   OPTYPE_MISC, { RA, RS, XUI,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes25[] =
{
{ "oris",     OP(25),		OP_MASK,	   OPTYPE_MISC, { RA, RS, XUI,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes26[] =
{
{ "xori",     OP(26),		OP_MASK,	   OPTYPE_MISC, { RA, RS, XUI,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes27[] =
{
{ "xoris",    OP(27),		OP_MASK,	   OPTYPE_MISC, { RA, RS, XUI,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes28[] =
{
{ "andi.",    OP(28),		OP_MASK,	   OPTYPE_MISC, { RA, RS, XUI,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes29[] =
{
{ "andis.",   OP(29),		OP_MASK,	   OPTYPE_MISC, { RA, RS, XUI,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes30[] =
{
{ "rotldi",   MD(30,0,0),	MDMB_MASK,	   OPTYPE_MISC, { RA, RS, SH6,0 }},
{ "clrldi",   MD(30,0,0),	MDSH_MASK,	   OPTYPE_MISC, { RA, RS, MB6,0 }},
{ "rldicl",   MD(30,0,0),	MD_MASK,	   OPTYPE_MISC, { RA, RS, SH6, MB6,0 }},
{ "rotldi.",  MD(30,0,1),	MDMB_MASK,	   OPTYPE_MISC, { RA, RS, SH6,0 }},
{ "clrldi.",  MD(30,0,1),	MDSH_MASK,	   OPTYPE_MISC, { RA, RS, MB6,0 }},
{ "rldicl.",  MD(30,0,1),	MD_MASK,	   OPTYPE_MISC, { RA, RS, SH6, MB6,0 }},
{ "rldicr",   MD(30,1,0),	MD_MASK,	   OPTYPE_MISC, { RA, RS, SH6, ME6,0 }},
{ "rldicr.",  MD(30,1,1),	MD_MASK,	   OPTYPE_MISC, { RA, RS, SH6, ME6,0 }},
{ "rldic",    MD(30,2,0),	MD_MASK,	   OPTYPE_MISC, { RA, RS, SH6, MB6,0 }},
{ "rldic.",   MD(30,2,1),	MD_MASK,	   OPTYPE_MISC, { RA, RS, SH6, MB6,0 }},
{ "rldimi",   MD(30,3,0),	MD_MASK,	   OPTYPE_MISC, { RA, RS, SH6, MB6,0 }},
{ "rldimi.",  MD(30,3,1),	MD_MASK,	   OPTYPE_MISC, { RA, RS, SH6, MB6,0 }},
{ "rotld",    MDS(30,8,0),	MDSMB_MASK,	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "rldcl",    MDS(30,8,0),	MDS_MASK,	   OPTYPE_MISC, { RA, RS, RB, MB6,0 }},
{ "rotld.",   MDS(30,8,1),	MDSMB_MASK,	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "rldcl.",   MDS(30,8,1),	MDS_MASK,	   OPTYPE_MISC, { RA, RS, RB, MB6,0 }},
{ "rldcr",    MDS(30,9,0),	MDS_MASK,	   OPTYPE_MISC, { RA, RS, RB, ME6,0 }},
{ "rldcr.",   MDS(30,9,1),	MDS_MASK,	   OPTYPE_MISC, { RA, RS, RB, ME6,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes31[] =
{
{ "cmpw",     XCMPL(31,0,0),	XCMPL_MASK,	   OPTYPE_MISC, { OBF, RA, RB,0 }},
{ "cmpd",     XCMPL(31,0,1),	XCMPL_MASK,	   OPTYPE_MISC, { OBF, RA, RB,0 }},
{ "cmp",      X(31,0),		XCMP_MASK,	   OPTYPE_MISC, { BF, L, RA, RB,0 }},
{ "twlgt",    XTO(31,4,TOLGT),	XTO_MASK,	   OPTYPE_MISC, { RA, RB,0 }},
{ "twllt",    XTO(31,4,TOLLT),	XTO_MASK,	   OPTYPE_MISC, { RA, RB,0 }},
{ "tweq",     XTO(31,4,TOEQ),	XTO_MASK,	   OPTYPE_MISC, { RA, RB,0 }},
{ "twlge",    XTO(31,4,TOLGE),	XTO_MASK,	   OPTYPE_MISC, { RA, RB,0 }},
{ "twlnl",    XTO(31,4,TOLNL),	XTO_MASK,	   OPTYPE_MISC, { RA, RB,0 }},
{ "twlle",    XTO(31,4,TOLLE),	XTO_MASK,	   OPTYPE_MISC, { RA, RB,0 }},
{ "twlng",    XTO(31,4,TOLNG),	XTO_MASK,	   OPTYPE_MISC, { RA, RB,0 }},
{ "twgt",     XTO(31,4,TOGT),	XTO_MASK,	   OPTYPE_MISC, { RA, RB,0 }},
{ "twge",     XTO(31,4,TOGE),	XTO_MASK,	   OPTYPE_MISC, { RA, RB,0 }},
{ "twnl",     XTO(31,4,TONL),	XTO_MASK,	   OPTYPE_MISC, { RA, RB,0 }},
{ "twlt",     XTO(31,4,TOLT),	XTO_MASK,	   OPTYPE_MISC, { RA, RB,0 }},
{ "twle",     XTO(31,4,TOLE),	XTO_MASK,	   OPTYPE_MISC, { RA, RB,0 }},
{ "twng",     XTO(31,4,TONG),	XTO_MASK,	   OPTYPE_MISC, { RA, RB,0 }},
{ "twne",     XTO(31,4,TONE),	XTO_MASK,	   OPTYPE_MISC, { RA, RB,0 }},
{ "trap",     XTO(31,4,TOU),	0xffffffff,	   OPTYPE_MISC, { 0,0 }},
{ "tw",       X(31,4),		X_MASK, 	   OPTYPE_MISC, { TO, RA, RB,0 }},
{ "subfc",    XO(31,8,0,0),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "subc",     XO(31,8,0,0),	XO_MASK,	   OPTYPE_MISC, { RT, RB, RA,0 }},
{ "subfc.",   XO(31,8,0,1),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "subc.",    XO(31,8,0,1),	XO_MASK,	   OPTYPE_MISC, { RT, RB, RA,0 }},
{ "subfco",   XO(31,8,1,0),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "subco",    XO(31,8,1,0),	XO_MASK,	   OPTYPE_MISC, { RT, RB, RA,0 }},
{ "subfco.",  XO(31,8,1,1),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "subco.",   XO(31,8,1,1),	XO_MASK,	   OPTYPE_MISC, { RT, RB, RA,0 }},
{ "mulhdu",   XO(31,9,0,0),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "mulhdu.",  XO(31,9,0,1),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "addc",     XO(31,10,0,0),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "addc.",    XO(31,10,0,1),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "addco",    XO(31,10,1,0),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "addco.",   XO(31,10,1,1),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "mulhwu",   XO(31,11,0,0),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "mulhwu.",  XO(31,11,0,1),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "mfcr",     X(31,19), 	XRARB_MASK,	   OPTYPE_MISC, { RT,0 }},
{ "lwarx",    X(31,20),	        X_MASK, 	   OPTYPE_LOAD, { RT, RA, RB,0 }},
{ "ldx",      X(31,21),	        X_MASK, 	   OPTYPE_LOAD, { RT, RA, RB,0 }},
{ "lwzx",     X(31,23),	        X_MASK, 	   OPTYPE_LOAD, { RT, RA, RB,0 }},
{ "slw",      XRC(31,24,0),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "slw.",     XRC(31,24,1),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "cntlzw",   XRC(31,26,0),	XRB_MASK,	   OPTYPE_MISC, { RA, RS,0 }},
{ "cntlzw.",  XRC(31,26,1),	XRB_MASK,	   OPTYPE_MISC, { RA, RS,0 }},
{ "sld",      XRC(31,27,0),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "sld.",     XRC(31,27,1),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "and",      XRC(31,28,0),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "and.",     XRC(31,28,1),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "cmplw",    XCMPL(31,32,0),	XCMPL_MASK,	   OPTYPE_MISC, { OBF, RA, RB,0 }},
{ "cmpld",    XCMPL(31,32,1),	XCMPL_MASK,	   OPTYPE_MISC, { OBF, RA, RB,0 }},
{ "cmpl",     X(31,32), 	XCMP_MASK,	   OPTYPE_MISC, { BF, L, RA, RB,0 }},
{ "subf",     XO(31,40,0,0),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "sub",      XO(31,40,0,0),	XO_MASK,	   OPTYPE_MISC, { RT, RB, RA,0 }},
{ "subf.",    XO(31,40,0,1),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "sub.",     XO(31,40,0,1),	XO_MASK,	   OPTYPE_MISC, { RT, RB, RA,0 }},
{ "subfo",    XO(31,40,1,0),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "subo",     XO(31,40,1,0),	XO_MASK,	   OPTYPE_MISC, { RT, RB, RA,0 }},
{ "subfo.",   XO(31,40,1,1),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "subo.",    XO(31,40,1,1),	XO_MASK,	   OPTYPE_MISC, { RT, RB, RA,0 }},
{ "ldux",     X(31,53), 	X_MASK, 	   OPTYPE_LOAD, { RT, RAL, RB,0 }},
{ "dcbst",    X(31,54), 	XRT_MASK,	   OPTYPE_MISC, { RAO, RB,0 }},
{ "lwzux",    X(31,55), 	X_MASK, 	   OPTYPE_LOAD, { RT, RAL, RB,0 }},
{ "cntlzd",   XRC(31,58,0),	XRB_MASK,	   OPTYPE_MISC, { RA, RS,0 }},
{ "cntlzd.",  XRC(31,58,1),	XRB_MASK,	   OPTYPE_MISC, { RA, RS,0 }},
{ "andc",     XRC(31,60,0),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "andc.",    XRC(31,60,1),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "tdlgt",    XTO(31,68,TOLGT), XTO_MASK,	   OPTYPE_MISC, { RA, RB,0 }},
{ "tdllt",    XTO(31,68,TOLLT), XTO_MASK,	   OPTYPE_MISC, { RA, RB,0 }},
{ "tdeq",     XTO(31,68,TOEQ),	XTO_MASK,	   OPTYPE_MISC, { RA, RB,0 }},
{ "tdlge",    XTO(31,68,TOLGE), XTO_MASK,	   OPTYPE_MISC, { RA, RB,0 }},
{ "tdlnl",    XTO(31,68,TOLNL), XTO_MASK,	   OPTYPE_MISC, { RA, RB,0 }},
{ "tdlle",    XTO(31,68,TOLLE), XTO_MASK,	   OPTYPE_MISC, { RA, RB,0 }},
{ "tdlng",    XTO(31,68,TOLNG), XTO_MASK,	   OPTYPE_MISC, { RA, RB,0 }},
{ "tdgt",     XTO(31,68,TOGT),	XTO_MASK,	   OPTYPE_MISC, { RA, RB,0 }},
{ "tdge",     XTO(31,68,TOGE),	XTO_MASK,	   OPTYPE_MISC, { RA, RB,0 }},
{ "tdnl",     XTO(31,68,TONL),	XTO_MASK,	   OPTYPE_MISC, { RA, RB,0 }},
{ "tdlt",     XTO(31,68,TOLT),	XTO_MASK,	   OPTYPE_MISC, { RA, RB,0 }},
{ "tdle",     XTO(31,68,TOLE),	XTO_MASK,	   OPTYPE_MISC, { RA, RB,0 }},
{ "tdng",     XTO(31,68,TONG),	XTO_MASK,	   OPTYPE_MISC, { RA, RB,0 }},
{ "tdne",     XTO(31,68,TONE),	XTO_MASK,	   OPTYPE_MISC, { RA, RB,0 }},
{ "td",       X(31,68), 	X_MASK, 	   OPTYPE_MISC, { TO, RA, RB,0 }},
{ "mulhd",    XO(31,73,0,0),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "mulhd.",   XO(31,73,0,1),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "mulhw",    XO(31,75,0,0),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "mulhw.",   XO(31,75,0,1),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "mfmsr",    X(31,83), 	XRARB_MASK,	   OPTYPE_MISC, { RT,0 }},
{ "ldarx",    X(31,84), 	X_MASK, 	   OPTYPE_LOAD, { RT, RA, RB,0 }},
{ "dcbf",     X(31,86), 	XRT_MASK,	   OPTYPE_MISC, { RAO, RB,0 }},
{ "lbzx",     X(31,87), 	X_MASK, 	   OPTYPE_LOAD, { RT, RA, RB,0 }},
{ "neg",      XO(31,104,0,0),	XORB_MASK,	   OPTYPE_MISC, { RT, RA,0 }},
{ "neg.",     XO(31,104,0,1),	XORB_MASK,	   OPTYPE_MISC, { RT, RA,0 }},
{ "nego",     XO(31,104,1,0),	XORB_MASK,	   OPTYPE_MISC, { RT, RA,0 }},
{ "nego.",    XO(31,104,1,1),	XORB_MASK,	   OPTYPE_MISC, { RT, RA,0 }},
{ "lbzux",    X(31,119),	X_MASK, 	   OPTYPE_LOAD, { RT, RAL, RB,0 }},
{ "not",      XRC(31,124,0),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RBS,0 }},
{ "nor",      XRC(31,124,0),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "not.",     XRC(31,124,1),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RBS,0 }},
{ "nor.",     XRC(31,124,1),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "subfe",    XO(31,136,0,0),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "subfe.",   XO(31,136,0,1),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "subfeo",   XO(31,136,1,0),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "subfeo.",  XO(31,136,1,1),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "adde",     XO(31,138,0,0),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "adde.",    XO(31,138,0,1),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "addeo",    XO(31,138,1,0),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "addeo.",   XO(31,138,1,1),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "mtcrf",    X(31,144),	X_MASK|(1<<20)|(1<<11), OPTYPE_MISC, { FXM, RS,0 }},
{ "mtmsr",    X(31,146),	XRARB_MASK,	   OPTYPE_MISC, { RS,0 }},
{ "stdx",     X(31,149),	X_MASK, 	   OPTYPE_STORE,{ RS, RA, RB,0 }},
{ "stwcx.",   XRC(31,150,1),	X_MASK, 	   OPTYPE_STORE,{ RS, RA, RB,0 }},
{ "stwx",     X(31,151),	X_MASK, 	   OPTYPE_STORE,{ RS, RA, RB,0 }},
{ "stdux",    X(31,181),	X_MASK, 	   OPTYPE_STORE,{ RS, RAS, RB,0 }},
{ "stwux",    X(31,183),	X_MASK, 	   OPTYPE_STORE,{ RS, RAS, RB,0 }},
{ "subfze",   XO(31,200,0,0),	XORB_MASK,	   OPTYPE_MISC, { RT, RA,0 }},
{ "subfze.",  XO(31,200,0,1),	XORB_MASK,	   OPTYPE_MISC, { RT, RA,0 }},
{ "subfzeo",  XO(31,200,1,0),	XORB_MASK,	   OPTYPE_MISC, { RT, RA,0 }},
{ "subfzeo.", XO(31,200,1,1),	XORB_MASK,	   OPTYPE_MISC, { RT, RA,0 }},
{ "addze",    XO(31,202,0,0),	XORB_MASK,	   OPTYPE_MISC, { RT, RA,0 }},
{ "addze.",   XO(31,202,0,1),	XORB_MASK,	   OPTYPE_MISC, { RT, RA,0 }},
{ "addzeo",   XO(31,202,1,0),	XORB_MASK,	   OPTYPE_MISC, { RT, RA,0 }},
{ "addzeo.",  XO(31,202,1,1),	XORB_MASK,	   OPTYPE_MISC, { RT, RA,0 }},
{ "mtsr",     X(31,210),	XRB_MASK|(1<<20),  OPTYPE_MISC, { SR, RS,0 }},
{ "stdcx.",   XRC(31,214,1),	X_MASK, 	   OPTYPE_STORE,{ RS, RA, RB,0 }},
{ "stbx",     X(31,215),	X_MASK, 	   OPTYPE_STORE,{ RS, RA, RB,0 }},
{ "subfme",   XO(31,232,0,0),	XORB_MASK,	   OPTYPE_MISC, { RT, RA,0 }},
{ "subfme.",  XO(31,232,0,1),	XORB_MASK,	   OPTYPE_MISC, { RT, RA,0 }},
{ "subfmeo",  XO(31,232,1,0),	XORB_MASK,	   OPTYPE_MISC, { RT, RA,0 }},
{ "subfmeo.", XO(31,232,1,1),	XORB_MASK,	   OPTYPE_MISC, { RT, RA,0 }},
{ "mulld",    XO(31,233,0,0),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "mulld.",   XO(31,233,0,1),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "mulldo",   XO(31,233,1,0),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "mulldo.",  XO(31,233,1,1),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "addme",    XO(31,234,0,0),	XORB_MASK,	   OPTYPE_MISC, { RT, RA,0 }},
{ "addme.",   XO(31,234,0,1),	XORB_MASK,	   OPTYPE_MISC, { RT, RA,0 }},
{ "addmeo",   XO(31,234,1,0),	XORB_MASK,	   OPTYPE_MISC, { RT, RA,0 }},
{ "addmeo.",  XO(31,234,1,1),	XORB_MASK,	   OPTYPE_MISC, { RT, RA,0 }},
{ "mullw",    XO(31,235,0,0),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "mullw.",   XO(31,235,0,1),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "mullwo",   XO(31,235,1,0),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "mullwo.",  XO(31,235,1,1),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "mtsrin",   X(31,242),	XRA_MASK,	   OPTYPE_MISC, { RS, RB,0 }},
{ "dcbtst",   X(31,246),	XRT_MASK,	   OPTYPE_MISC, { RAO, RB,0 }},
{ "stbux",    X(31,247),	X_MASK, 	   OPTYPE_STORE,{ RS, RAS, RB,0 }},
{ "mfrom",    X(31,265),	XRB_MASK,	   OPTYPE_MISC, { RT, RA,0 }},
{ "add",      XO(31,266,0,0),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "add.",     XO(31,266,0,1),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "addo",     XO(31,266,1,0),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "addo.",    XO(31,266,1,1),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "dcbt",     X(31,278),	XRT_MASK,	   OPTYPE_MISC, { RAO, RB,0 }},
{ "lhzx",     X(31,279),	X_MASK, 	   OPTYPE_LOAD, { RT, RA, RB,0 }},
{ "eqv",      XRC(31,284,0),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "eqv.",     XRC(31,284,1),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "tlbie",    X(31,306),	XRTRA_MASK,	   OPTYPE_MISC, { RB,0 }},
{ "eciwx",    X(31,310),	X_MASK, 	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "lhzux",    X(31,311),	X_MASK, 	   OPTYPE_LOAD, { RT, RAL, RB,0 }},
{ "xor",      XRC(31,316,0),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "xor.",     XRC(31,316,1),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "mfxer",    XSPR(31,339,1),	XSPR_MASK,	   OPTYPE_MISC, { RT,0 }},
{ "mflr",     XSPR(31,339,8),	XSPR_MASK,	   OPTYPE_MISC, { RT,0 }},
{ "mfctr",    XSPR(31,339,9),	XSPR_MASK,	   OPTYPE_MISC, { RT,0 }},
{ "mfdsisr",  XSPR(31,339,18),	XSPR_MASK,	   OPTYPE_MISC, { RT,0 }},
{ "mfdar",    XSPR(31,339,19),	XSPR_MASK,	   OPTYPE_MISC, { RT,0 }},
{ "mfdec",    XSPR(31,339,22),	XSPR_MASK,	   OPTYPE_MISC, { RT,0 }},
{ "mfsdr1",   XSPR(31,339,25),	XSPR_MASK,	   OPTYPE_MISC, { RT,0 }},
{ "mfsrr0",   XSPR(31,339,26),	XSPR_MASK,	   OPTYPE_MISC, { RT,0 }},
{ "mfsrr1",   XSPR(31,339,27),	XSPR_MASK,	   OPTYPE_MISC, { RT,0 }},
{ "mfsprg0",  XSPR(31,339,272),	XSPR_MASK,	   OPTYPE_MISC, { RT,0 }},
{ "mfsprg1",  XSPR(31,339,273),	XSPR_MASK,	   OPTYPE_MISC, { RT,0 }},
{ "mfsprg2",  XSPR(31,339,274),	XSPR_MASK,	   OPTYPE_MISC, { RT,0 }},
{ "mfsprg3",  XSPR(31,339,275),	XSPR_MASK,	   OPTYPE_MISC, { RT,0 }},
{ "mfear",    XSPR(31,339,282),	XSPR_MASK,	   OPTYPE_MISC, { RT,0 }},
{ "mfpvr",    XSPR(31,339,287),	XSPR_MASK,	   OPTYPE_MISC, { RT,0 }},
{ "mfspr",    X(31,339),	X_MASK, 	   OPTYPE_MISC, { RT, SPR,0 }},
{ "mftbl",    XTBR(31,371,268),	XTBR_MASK,	   OPTYPE_MISC, { RT,0 }},
{ "mftbu",    XTBR(31,371,269),	XTBR_MASK,	   OPTYPE_MISC, { RT,0 }},
{ "mftb",     X(31,371),	X_MASK,	           OPTYPE_MISC, { RT, TBR,0 }},
{ "lwax",     X(31,341),	X_MASK, 	   OPTYPE_LOAD, { RT, RA, RB,0 }},
{ "lhax",     X(31,343),	X_MASK, 	   OPTYPE_LOAD, { RT, RA, RB,0 }},
{ "tlbia",    X(31,370),	0xffffffff,	   OPTYPE_MISC, { 0,0 }},
{ "mftb",     X(31,371),	X_MASK, 	   OPTYPE_MISC, { RT, TBR,0 }},
{ "lwaux",    X(31,373),	X_MASK, 	   OPTYPE_LOAD, { RT, RAL, RB,0 }},
{ "lhaux",    X(31,375),	X_MASK, 	   OPTYPE_LOAD, { RT, RAL, RB,0 }},
{ "sthx",     X(31,407),	X_MASK, 	   OPTYPE_STORE, { RS, RA, RB,0 }},
{ "orc",      XRC(31,412,0),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "orc.",     XRC(31,412,1),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "sradi",    XS(31,413,0),	XS_MASK,	   OPTYPE_MISC, { RA, RS, SH6,0 }},
{ "sradi.",   XS(31,413,1),	XS_MASK,	   OPTYPE_MISC, { RA, RS, SH6,0 }},
{ "slbie",    X(31,434),	XRTRA_MASK,	   OPTYPE_MISC, { RB,0 }},
{ "ecowx",    X(31,438),	X_MASK, 	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "sthux",    X(31,439),	X_MASK, 	   OPTYPE_STORE,{ RS, RAS, RB,0 }},
{ "mr",       XRC(31,444,0),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RBS,0 }},
{ "or",       XRC(31,444,0),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "mr.",      XRC(31,444,1),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RBS,0 }},
{ "or.",      XRC(31,444,1),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "divdu",    XO(31,457,0,0),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "divdu.",   XO(31,457,0,1),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "divduo",   XO(31,457,1,0),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "divduo.",  XO(31,457,1,1),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "divwu",    XO(31,459,0,0),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "divwu.",   XO(31,459,0,1),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "divwuo",   XO(31,459,1,0),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "divwuo.",  XO(31,459,1,1),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "mtxer",    XSPR(31,467,1),	XSPR_MASK,	   OPTYPE_MISC, { RS,0 }},
{ "mtlr",     XSPR(31,467,8),	XSPR_MASK,	   OPTYPE_MISC, { RS,0 }},
{ "mtctr",    XSPR(31,467,9),	XSPR_MASK,	   OPTYPE_MISC, { RS,0 }},
{ "mtdsisr",  XSPR(31,467,18),	XSPR_MASK,	   OPTYPE_MISC, { RS,0 }},
{ "mtdar",    XSPR(31,467,19),	XSPR_MASK,	   OPTYPE_MISC, { RS,0 }},
{ "mtdec",    XSPR(31,467,22),	XSPR_MASK,	   OPTYPE_MISC, { RS,0 }},
{ "mtsdr1",   XSPR(31,467,25),	XSPR_MASK,	   OPTYPE_MISC, { RS,0 }},
{ "mtsrr0",   XSPR(31,467,26),	XSPR_MASK,	   OPTYPE_MISC, { RS,0 }},
{ "mtsrr1",   XSPR(31,467,27),	XSPR_MASK,	   OPTYPE_MISC, { RS,0 }},
{ "mtsprg0",  XSPR(31,467,272),	XSPR_MASK,	   OPTYPE_MISC, { RS,0 }},
{ "mtsprg1",  XSPR(31,467,273),	XSPR_MASK,	   OPTYPE_MISC, { RS,0 }},
{ "mtsprg2",  XSPR(31,467,274),	XSPR_MASK,	   OPTYPE_MISC, { RS,0 }},
{ "mtsprg3",  XSPR(31,467,275),	XSPR_MASK,	   OPTYPE_MISC, { RS,0 }},
{ "mtear",    XSPR(31,467,282),	XSPR_MASK,	   OPTYPE_MISC, { RS,0 }},
{ "mttbl",    XSPR(31,467,284),	XSPR_MASK,	   OPTYPE_MISC, { RS,0 }},
{ "mttbu",    XSPR(31,467,285),	XSPR_MASK,	   OPTYPE_MISC, { RS,0 }},
{ "mtspr",    X(31,467),	X_MASK, 	   OPTYPE_MISC, { SPR, RS,0 }},
{ "dcbi",     X(31,470),	XRT_MASK,	   OPTYPE_MISC, { RAO, RB,0 }},
{ "nand",     XRC(31,476,0),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "nand.",    XRC(31,476,1),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "divd",     XO(31,489,0,0),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "divd.",    XO(31,489,0,1),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "divdo",    XO(31,489,1,0),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "divdo.",   XO(31,489,1,1),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "divw",     XO(31,491,0,0),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "divw.",    XO(31,491,0,1),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "divwo",    XO(31,491,1,0),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "divwo.",   XO(31,491,1,1),	XO_MASK,	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "slbia",    X(31,498),	0xffffffff,	   OPTYPE_MISC, { 0,0 }},
{ "mcrxr",    X(31,512),	XRARB_MASK|(3<<21),OPTYPE_MISC, { BF,0 }},
{ "lswx",     X(31,533),	X_MASK, 	   OPTYPE_MISC, { RT, RA, RB,0 }},
{ "lwbrx",    X(31,534),	X_MASK, 	   OPTYPE_LOAD, { RT, RA, RB,0 }},
{ "lfsx",     X(31,535),	X_MASK, 	   OPTYPE_LOAD, { FRT, RA, RB,0 }},
{ "srw",      XRC(31,536,0),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "srw.",     XRC(31,536,1),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "srd",      XRC(31,539,0),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "srd.",     XRC(31,539,1),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "tlbsync",  X(31,566),	0xffffffff,	   OPTYPE_MISC, { 0,0 }},
{ "lfsux",    X(31,567),	X_MASK, 	   OPTYPE_LOAD, { FRT, RAS, RB,0 }},
{ "mfsr",     X(31,595),	XRB_MASK|(1<<20),  OPTYPE_MISC, { RT, SR,0 }},
{ "esa",      X(31,596),	0xffffffff, 	   OPTYPE_MISC, { 0,0 }},
{ "lswi",     X(31,597),	X_MASK, 	   OPTYPE_MISC, { RT, RA, NB,0 }},
{ "sync",     X(31,598),	0xffffffff,	   OPTYPE_MISC, { 0,0 }},
{ "lfdx",     X(31,599),	X_MASK, 	   OPTYPE_LOAD, { FRT, RA, RB,0 }},
{ "dsa",      X(31,628),	0xffffffff, 	   OPTYPE_MISC, { 0,0 }},
{ "lfdux",    X(31,631),	X_MASK, 	   OPTYPE_LOAD, { FRT, RAS, RB,0 }},
{ "mfsrin",   X(31,659),	XRA_MASK,	   OPTYPE_MISC, { RT, RB,0 }},
{ "stswx",    X(31,661),	X_MASK, 	   OPTYPE_MISC, { RS, RA, RB,0 }},
{ "stwbrx",   X(31,662),	X_MASK, 	   OPTYPE_STORE,{ RS, RA, RB,0 }},
{ "stfsx",    X(31,663),	X_MASK, 	   OPTYPE_STORE,{ FRS, RA, RB,0 }},
{ "stfsux",   X(31,695),	X_MASK, 	   OPTYPE_STORE,{ FRS, RAS, RB,0 }},
{ "stswi",    X(31,725),	X_MASK, 	   OPTYPE_MISC, { RS, RA, NB,0 }},
{ "stfdx",    X(31,727),	X_MASK, 	   OPTYPE_STORE,{ FRS, RA, RB,0 }},
{ "stfdux",   X(31,759),	X_MASK, 	   OPTYPE_STORE,{ FRS, RAS, RB,0 }},
{ "lhbrx",    X(31,790),	X_MASK, 	   OPTYPE_LOAD, { RT, RA, RB,0 }},
{ "sraw",     XRC(31,792,0),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "sraw.",    XRC(31,792,1),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "srad",     XRC(31,794,0),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "srad.",    XRC(31,794,1),	X_MASK, 	   OPTYPE_MISC, { RA, RS, RB,0 }},
{ "srawi",    XRC(31,824,0),	X_MASK, 	   OPTYPE_MISC, { RA, RS, SH,0 }},
{ "srawi.",   XRC(31,824,1),	X_MASK, 	   OPTYPE_MISC, { RA, RS, SH,0 }},
{ "eieio",    X(31,854),	0xffffffff,	   OPTYPE_MISC, { 0,0 }},
{ "sthbrx",   X(31,918),	X_MASK, 	   OPTYPE_STORE,{ RS, RA, RB,0 }},
{ "extsh",    XRC(31,922,0),	XRB_MASK,	   OPTYPE_MISC, { RA, RS,0 }},
{ "extsh.",   XRC(31,922,1),	XRB_MASK,	   OPTYPE_MISC, { RA, RS,0 }},
{ "extsb",    XRC(31,954,0),	XRB_MASK,	   OPTYPE_MISC, { RA, RS} },
{ "extsb.",   XRC(31,954,1),	XRB_MASK,	   OPTYPE_MISC, { RA, RS} },
{ "tlbld",    X(31,978),	XRTRA_MASK,	   OPTYPE_MISC, { RB,0 }},
{ "icbi",     X(31,982),	XRT_MASK,	   OPTYPE_MISC, { RA, RB,0 }},
{ "stfiwx",   X(31,983),	X_MASK, 	   OPTYPE_STORE,{ FRS, RA, RB,0 }},
{ "extsw",    XRC(31,986,0),	XRB_MASK,	   OPTYPE_MISC, { RA, RS,0 }},
{ "extsw.",   XRC(31,986,1),	XRB_MASK,	   OPTYPE_MISC, { RA, RS,0 }},
{ "tlbli",    X(31,1010),	XRTRA_MASK,	   OPTYPE_MISC, { RB,0 }},
{ "dcbz",     X(31,1014),	XRT_MASK,	   OPTYPE_MISC, { RAO, RB,0 }},
{ "dclz",     X(31,1014),	XRT_MASK,	   OPTYPE_MISC, { RAO, RB,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes32[] =
{
{ "lwz",      OP(32),		 OP_MASK,	   OPTYPE_LOAD, { RT, D, RA,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes33[] =
{
{ "lwzu",     OP(33),		 OP_MASK,	   OPTYPE_LOAD, { RT, D, RAL,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes34[] =
{
{ "lbz",      OP(34),		 OP_MASK,	   OPTYPE_LOAD, { RT, D, RA,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes35[] =
{
{ "lbzu",     OP(35),		 OP_MASK,	   OPTYPE_LOAD, { RT, D, RAL,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes36[] =
{
{ "stw",      OP(36),		 OP_MASK,	   OPTYPE_STORE, { RS, D, RA,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes37[] =
{
{ "stwu",     OP(37),		 OP_MASK,	   OPTYPE_STORE, { RS, D, RAS,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes38[] =
{
{ "stb",      OP(38),		 OP_MASK,	   OPTYPE_STORE, { RS, D, RA,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes39[] =
{
{ "stbu",     OP(39),		 OP_MASK,	   OPTYPE_STORE, { RS, D, RAS,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes40[] =
{
{ "lhz",      OP(40),		 OP_MASK,	   OPTYPE_LOAD, { RT, D, RA,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes41[] =
{
{ "lhzu",     OP(41),		 OP_MASK,	   OPTYPE_LOAD, { RT, D, RAL,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes42[] =
{
{ "lha",      OP(42),		 OP_MASK,	   OPTYPE_LOAD, { RT, D, RA,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes43[] =
{
{ "lhau",     OP(43),		 OP_MASK,	   OPTYPE_LOAD, { RT, D, RAL,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes44[] =
{
{ "sth",      OP(44),		 OP_MASK,	   OPTYPE_STORE, { RS, D, RA,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes45[] =
{
{ "sthu",     OP(45),		 OP_MASK,	   OPTYPE_STORE, { RS, D, RAS,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes46[] =
{
{ "lmw",      OP(46),		 OP_MASK,	   OPTYPE_LOAD, { RT, D, RAM,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes47[] =
{
{ "stmw",     OP(47),		 OP_MASK,	   OPTYPE_STORE, { RS, D, RA,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes48[] =
{
{ "lfs",      OP(48),		 OP_MASK,	   OPTYPE_LOAD, { FRT, D, RA,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes49[] =
{
{ "lfsu",     OP(49),		 OP_MASK,	   OPTYPE_LOAD, { FRT, D, RAS,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes50[] =
{
{ "lfd",      OP(50),		 OP_MASK,	   OPTYPE_LOAD, { FRT, D, RA,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes51[] =
{
{ "lfdu",     OP(51),		 OP_MASK,	   OPTYPE_LOAD, { FRT, D, RAS,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes52[] =
{
{ "stfs",     OP(52),		 OP_MASK,	   OPTYPE_STORE, { FRS, D, RA,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes53[] =
{
{ "stfsu",    OP(53),		 OP_MASK,	   OPTYPE_STORE, { FRS, D, RAS,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes54[] =
{
{ "stfd",     OP(54),		 OP_MASK,	   OPTYPE_STORE, { FRS, D, RA,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes55[] =
{
{ "stfdu",    OP(55),		 OP_MASK,	   OPTYPE_STORE, { FRS, D, RAS,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes58[] =
{
{ "ld",       DSO(58,0),	 DS_MASK,	   OPTYPE_LOAD, { RT, DS, RA,0 }},
{ "ldu",      DSO(58,1),	 DS_MASK,	   OPTYPE_LOAD, { RT, DS, RAL,0 }},
{ "lwa",      DSO(58,2),	 DS_MASK,	   OPTYPE_LOAD, { RT, DS, RA,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes59[] =
{
{ "fdivs",    A(59,18,0),	 AFRC_MASK,	   OPTYPE_MISC, { FRT, FRA, FRB,0 }},
{ "fdivs.",   A(59,18,1),	 AFRC_MASK,	   OPTYPE_MISC, { FRT, FRA, FRB,0 }},
{ "fsubs",    A(59,20,0),	 AFRC_MASK,	   OPTYPE_MISC, { FRT, FRA, FRB,0 }},
{ "fsubs.",   A(59,20,1),	 AFRC_MASK,	   OPTYPE_MISC, { FRT, FRA, FRB,0 }},
{ "fadds",    A(59,21,0),	 AFRC_MASK,	   OPTYPE_MISC, { FRT, FRA, FRB,0 }},
{ "fadds.",   A(59,21,1),	 AFRC_MASK,	   OPTYPE_MISC, { FRT, FRA, FRB,0 }},
{ "fsqrts",   A(59,22,0),	 AFRAFRC_MASK ,    OPTYPE_MISC, { FRT, FRB,0 }},
{ "fsqrts.",  A(59,22,1),	 AFRAFRC_MASK ,    OPTYPE_MISC, { FRT, FRB,0 }},
{ "fres",     A(59,24,0),	 AFRAFRC_MASK ,    OPTYPE_MISC, { FRT, FRB,0 }},
{ "fres.",    A(59,24,1),	 AFRAFRC_MASK ,    OPTYPE_MISC, { FRT, FRB,0 }},
{ "fmuls",    A(59,25,0),	 AFRB_MASK,	   OPTYPE_MISC, { FRT, FRA, FRC,0 }},
{ "fmuls.",   A(59,25,1),	 AFRB_MASK,	   OPTYPE_MISC, { FRT, FRA, FRC,0 }},
{ "fmsubs",   A(59,28,0),	 A_MASK,	   OPTYPE_MISC, { FRT,FRA,FRC,FRB,0 }},
{ "fmsubs.",  A(59,28,1),	 A_MASK,	   OPTYPE_MISC, { FRT,FRA,FRC,FRB,0 }},
{ "fmadds",   A(59,29,0),	 A_MASK,	   OPTYPE_MISC, { FRT,FRA,FRC,FRB,0 }},
{ "fmadds.",  A(59,29,1),	 A_MASK,	   OPTYPE_MISC, { FRT,FRA,FRC,FRB,0 }},
{ "fnmsubs",  A(59,30,0),	 A_MASK,	   OPTYPE_MISC, { FRT,FRA,FRC,FRB,0 }},
{ "fnmsubs.", A(59,30,1),	 A_MASK,	   OPTYPE_MISC, { FRT,FRA,FRC,FRB,0 }},
{ "fnmadds",  A(59,31,0),	 A_MASK,	   OPTYPE_MISC, { FRT,FRA,FRC,FRB,0 }},
{ "fnmadds.", A(59,31,1),	 A_MASK,	   OPTYPE_MISC, { FRT,FRA,FRC,FRB,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes62[] =
{
{ "std",      DSO(62,0),	 DS_MASK,	   OPTYPE_STORE, { RS, DS, RA,0 }},
{ "stdu",     DSO(62,1),	 DS_MASK,	   OPTYPE_STORE, { RS, DS, RAS,0 }},
{ NULL }
};

const PPCOpcode ppcOpcodes63[] =
{
{ "fcmpu",    X(63,0),		 X_MASK|(3<<21),	    OPTYPE_MISC, { BF, FRA, FRB,0 }},
{ "frsp",     XRC(63,12,0),	 XRA_MASK,		    OPTYPE_MISC, { FRT, FRB,0 }},
{ "frsp.",    XRC(63,12,1),	 XRA_MASK,		    OPTYPE_MISC, { FRT, FRB,0 }},
{ "fctiw",    XRC(63,14,0),	 XRA_MASK,		    OPTYPE_MISC, { FRT, FRB,0 }},
{ "fctiw.",   XRC(63,14,1),	 XRA_MASK,		    OPTYPE_MISC, { FRT, FRB,0 }},
{ "fctiwz",   XRC(63,15,0),	 XRA_MASK,		    OPTYPE_MISC, { FRT, FRB,0 }},
{ "fctiwz.",  XRC(63,15,1),	 XRA_MASK,		    OPTYPE_MISC, { FRT, FRB,0 }},
{ "fdiv",     A(63,18,0),	 AFRC_MASK,		    OPTYPE_MISC, { FRT, FRA, FRB,0 }},
{ "fdiv.",    A(63,18,1),	 AFRC_MASK,		    OPTYPE_MISC, { FRT, FRA, FRB,0 }},
{ "fsub",     A(63,20,0),	 AFRC_MASK,		    OPTYPE_MISC, { FRT, FRA, FRB,0 }},
{ "fsub.",    A(63,20,1),	 AFRC_MASK,		    OPTYPE_MISC, { FRT, FRA, FRB,0 }},
{ "fadd",     A(63,21,0),	 AFRC_MASK,		    OPTYPE_MISC, { FRT, FRA, FRB,0 }},
{ "fadd.",    A(63,21,1),	 AFRC_MASK,		    OPTYPE_MISC, { FRT, FRA, FRB,0 }},
{ "fsqrt",    A(63,22,0),	 AFRAFRC_MASK,		    OPTYPE_MISC, { FRT, FRB,0 }},
{ "fsqrt.",   A(63,22,1),	 AFRAFRC_MASK,		    OPTYPE_MISC, { FRT, FRB,0 }},
{ "fsel",     A(63,23,0),	 A_MASK,		    OPTYPE_MISC, { FRT,FRA,FRC,FRB,0 }},
{ "fsel.",    A(63,23,1),	 A_MASK,		    OPTYPE_MISC, { FRT,FRA,FRC,FRB,0 }},
{ "fmul",     A(63,25,0),	 AFRB_MASK,		    OPTYPE_MISC, { FRT, FRA, FRC,0 }},
{ "fmul.",    A(63,25,1),	 AFRB_MASK,		    OPTYPE_MISC, { FRT, FRA, FRC,0 }},
{ "frsqrte",  A(63,26,0),	 AFRAFRC_MASK,		    OPTYPE_MISC, { FRT, FRB,0 }},
{ "frsqrte.", A(63,26,1),	 AFRAFRC_MASK,		    OPTYPE_MISC, { FRT, FRB,0 }},
{ "fmsub",    A(63,28,0),	 A_MASK,		    OPTYPE_MISC, { FRT,FRA,FRC,FRB,0 }},
{ "fmsub.",   A(63,28,1),	 A_MASK,		    OPTYPE_MISC, { FRT,FRA,FRC,FRB,0 }},
{ "fmadd",    A(63,29,0),	 A_MASK,		    OPTYPE_MISC, { FRT,FRA,FRC,FRB,0 }},
{ "fmadd.",   A(63,29,1),	 A_MASK,		    OPTYPE_MISC, { FRT,FRA,FRC,FRB,0 }},
{ "fnmsub",   A(63,30,0),	 A_MASK,		    OPTYPE_MISC, { FRT,FRA,FRC,FRB,0 }},
{ "fnmsub.",  A(63,30,1),	 A_MASK,		    OPTYPE_MISC, { FRT,FRA,FRC,FRB,0 }},
{ "fnmadd",   A(63,31,0),	 A_MASK,		    OPTYPE_MISC, { FRT,FRA,FRC,FRB,0 }},
{ "fnmadd.",  A(63,31,1),	 A_MASK,		    OPTYPE_MISC, { FRT,FRA,FRC,FRB,0 }},
{ "fcmpo",    X(63,30), 	 X_MASK|(3<<21),	    OPTYPE_MISC, { BF, FRA, FRB,0 }},
{ "mtfsb1",   XRC(63,38,0),	 XRARB_MASK,		    OPTYPE_MISC, { BT,0 }},
{ "mtfsb1.",  XRC(63,38,1),	 XRARB_MASK,		    OPTYPE_MISC, { BT,0 }},
{ "fneg",     XRC(63,40,0),	 XRA_MASK,		    OPTYPE_MISC, { FRT, FRB,0 }},
{ "fneg.",    XRC(63,40,1),	 XRA_MASK,		    OPTYPE_MISC, { FRT, FRB,0 }},
{ "mcrfs",    X(63,64), 	 XRB_MASK|(3<<21)|(3<<16),  OPTYPE_MISC, { BF, BFA,0 }},
{ "mtfsb0",   XRC(63,70,0),	 XRARB_MASK,		    OPTYPE_MISC, { BT,0 }},
{ "mtfsb0.",  XRC(63,70,1),	 XRARB_MASK,		    OPTYPE_MISC, { BT,0 }},
{ "fmr",      XRC(63,72,0),	 XRA_MASK,		    OPTYPE_MISC, { FRT, FRB,0 }},
{ "fmr.",     XRC(63,72,1),	 XRA_MASK,		    OPTYPE_MISC, { FRT, FRB,0 }},
{ "mtfsfi",   XRC(63,134,0),	 XRA_MASK|(3<<21)|(1<<11),  OPTYPE_MISC, { BF, U,0 }},
{ "mtfsfi.",  XRC(63,134,1),	 XRA_MASK|(3<<21)|(1<<11),  OPTYPE_MISC, { BF, U,0 }},
{ "fnabs",    XRC(63,136,0),	 XRA_MASK,		    OPTYPE_MISC, { FRT, FRB,0 }},
{ "fnabs.",   XRC(63,136,1),	 XRA_MASK,		    OPTYPE_MISC, { FRT, FRB,0 }},
{ "fabs",     XRC(63,264,0),	 XRA_MASK,		    OPTYPE_MISC, { FRT, FRB,0 }},
{ "fabs.",    XRC(63,264,1),	 XRA_MASK,		    OPTYPE_MISC, { FRT, FRB,0 }},
{ "mffs",     XRC(63,583,0),	 XRARB_MASK,		    OPTYPE_MISC, { FRT,0 }},
{ "mffs.",    XRC(63,583,1),	 XRARB_MASK,		    OPTYPE_MISC, { FRT,0 }},
{ "mtfsf",    XFL(63,711,0),	 XFL_MASK,		    OPTYPE_MISC, { FLM, FRB,0 }},
{ "mtfsf.",   XFL(63,711,1),	 XFL_MASK,		    OPTYPE_MISC, { FLM, FRB,0 }},
{ "fctid",    XRC(63,814,0),	 XRA_MASK,		    OPTYPE_MISC, { FRT, FRB,0 }},
{ "fctid.",   XRC(63,814,1),	 XRA_MASK,		    OPTYPE_MISC, { FRT, FRB,0 }},
{ "fctidz",   XRC(63,815,0),	 XRA_MASK,		    OPTYPE_MISC, { FRT, FRB,0 }},
{ "fctidz.",  XRC(63,815,1),	 XRA_MASK,		    OPTYPE_MISC, { FRT, FRB,0 }},
{ "fcfid",    XRC(63,846,0),	 XRA_MASK,		    OPTYPE_MISC, { FRT, FRB,0 }},
{ "fcfid.",   XRC(63,846,1),	 XRA_MASK,		    OPTYPE_MISC, { FRT, FRB,0 }},
{ NULL }
};


static const PPCOpcode *ppcOpcodeTables[] =
{
    NULL,
    NULL,
    ppcOpcodes2,
    ppcOpcodes3,
    NULL,
    NULL,
    NULL,
    ppcOpcodes7,
    ppcOpcodes8,
    NULL,
    ppcOpcodes10,
    ppcOpcodes11,
    ppcOpcodes12,
    ppcOpcodes13,
    ppcOpcodes14,
    ppcOpcodes15,
    ppcOpcodes16,
    ppcOpcodes17,
    ppcOpcodes18,
    ppcOpcodes19,
    ppcOpcodes20,
    ppcOpcodes21,
    NULL,
    ppcOpcodes23,
    ppcOpcodes24,
    ppcOpcodes25,
    ppcOpcodes26,
    ppcOpcodes27,
    ppcOpcodes28,
    ppcOpcodes29,
    ppcOpcodes30,
    ppcOpcodes31,
    ppcOpcodes32,
    ppcOpcodes33,
    ppcOpcodes34,
    ppcOpcodes35,
    ppcOpcodes36,
    ppcOpcodes37,
    ppcOpcodes38,
    ppcOpcodes39,
    ppcOpcodes40,
    ppcOpcodes41,
    ppcOpcodes42,
    ppcOpcodes43,
    ppcOpcodes44,
    ppcOpcodes45,
    ppcOpcodes46,
    ppcOpcodes47,
    ppcOpcodes48,
    ppcOpcodes49,
    ppcOpcodes50,
    ppcOpcodes51,
    ppcOpcodes52,
    ppcOpcodes53,
    ppcOpcodes54,
    ppcOpcodes55,
    NULL,
    NULL,
    ppcOpcodes58,
    ppcOpcodes59,
    NULL,
    NULL,
    ppcOpcodes62,
    ppcOpcodes63
};


/*****************************************************************************/


#define AppendStr(f,s)     {sprintf(result,f,s); result = &result[strlen(result)];}
#define AppendComment(f,s) {sprintf(comment,f,s); comment = &comment[strlen(comment)];}


/*****************************************************************************/


static void MakeBranchComment(uint32              instruction,
			      uint32              instrAddr,
                              uint32              value,
                              const PPCRegisters *registers,
                              MakeAddrFunc        addrFunc,
                              char               *comment)
{
uint32 bo;
uint32 bi;
uint32 condition;
uint32 taken;
uint32 quiet;
uint32 ctr;

    if (PPC_OP(instruction) == 19)
    {
        if (!registers)
            return;

        /* branch to lr or ctr */
        if (((instruction >> 1) & 0x3ff) == 16)
        {
            AppendComment("--> 0x%X",registers->ppcr_LR);
        }
        else
        {
            AppendComment("--> 0x%X",registers->ppcr_CTR);
        }
    }
    else if (instruction & 2)
    {
        /* absolute addressing */
        AppendComment("--> 0x%X",value);
    }
    else
    {
        /* relative addressing */
        if (instrAddr == 0)
        {
            AppendComment("%s","*");
        }

        AppendComment("--> 0x%X",instrAddr + value);
    }

    if ((PPC_OP(instruction) == 16) || (PPC_OP(instruction) == 19))
    {
        if (!registers)
            return;

        /* conditional branch */

        /* evaluate the condition */
        bo        = (instruction >> 21) & 0x1e;  /* ignore branch prediction bit */
        bi        = (instruction >> 16) & 0x1f;
        condition = (((1 << (31 - bi)) & registers->ppcr_CR) ? 1 : 0);
        ctr       = registers->ppcr_CTR;

        taken = FALSE;
        quiet = FALSE;
        switch (bo)
        {
            case 0 : taken = ((ctr != 1) && !condition); break;
            case 2 : taken = ((ctr == 1) && !condition); break;
            case 4 : taken = !condition; 		 break;
            case 8 : taken = ((ctr != 1) && condition);	 break;
            case 10: taken = ((ctr == 1) && condition);	 break;
            case 12: taken = condition;			 break;
            case 16: taken = (ctr != 1);		 break;
            case 18: taken = (ctr == 1);		 break;
            case 20: quiet = TRUE;			 break;
        }

        if (!quiet)
        {
            if (taken)
            {
                AppendComment(" %s","(will be taken)");
            }
            else
            {
                AppendComment(" %s","(won't be taken)");
            }
        }
    }
}


/*****************************************************************************/


static void DisasmInstr(uint32              instruction,
                        uint32              instrAddr,
                        const PPCRegisters *registers,
                        MakeAddrFunc        addrFunc,
                        const PPCOpcode    *opcode,
                        char               *result,
                        char               *comment)
{
bool              dummy;
bool              need_comma;
bool              need_paren;
bool              need_comment;
bool              got_displacement;
int32             value;
const PPCOperand *operand;
uint32            i;
int32             displacement;
uint32            lastReg;
uint32            nextToLastReg;

    if (opcode->opc_Operands[0] != LAST)
    {
        AppendStr("%s\t",opcode->opc_Name);
    }
    else
    {
        AppendStr("%s",opcode->opc_Name);
    }

    displacement     = 0;
    got_displacement = FALSE;
    need_comma       = FALSE;
    need_paren       = FALSE;
    need_comment     = FALSE;
    nextToLastReg    = 0;
    lastReg          = 0;

    i = 0;
    while (opcode->opc_Operands[i] != LAST)
    {
        operand = &ppcOperands[opcode->opc_Operands[i]];

        /* Operands that are marked FAKE are simply ignored.  We
         * already made sure that the extract function considered
         * the instruction to be valid.
         */
        if (operand->op_Flags & PPC_OPERAND_FAKE)
        {
            i++;
            continue;
        }

        /* Extract the value from the instruction */
        if (operand->op_Extract)
        {
            value = (*operand->op_Extract)(instruction, &dummy);
        }
        else
        {
            value = (instruction >> operand->op_Shift) & ((1 << operand->op_Bits) - 1);
            if ((operand->op_Flags & PPC_OPERAND_SIGNED)
            && (value & (1 << (operand->op_Bits - 1))) != 0)
                value -= 1 << operand->op_Bits;
        }

        /* If the operand is optional, and the value is zero, don't
         * print anything.
         */
	if ((operand->op_Flags & PPC_OPERAND_OPTIONAL)
         && (operand->op_Flags & PPC_OPERAND_NEXT) == 0
         && value == 0)
        {
            i++;
            continue;
        }

        if (need_paren)
        {
            if (value == 0)
            {
                nextToLastReg = lastReg;
                lastReg       = value;

                /* don't show (r0) */
                i++;
                continue;
            }
            AppendStr("%s","(");
        }
        else if (need_comma)
        {
            AppendStr("%s",",");
            need_comma = FALSE;
        }

        /* Print the operand as directed by the flags.  */
        if (operand->op_Flags & PPC_OPERAND_GPR)
        {
            nextToLastReg = lastReg;
            lastReg       = value;

            if (value == 1)
            {
                AppendStr("sp", value);
            }
            else
            {
                AppendStr("r%ld", value);
            }
        }
        else if (operand->op_Flags & PPC_OPERAND_FPR)
        {
            AppendStr("f%ld", value);
        }
        else if (operand->op_Flags & PPC_OPERAND_SPR)
        {
        const char *sprName;

            switch (value)
            {
                case 18  : sprName = "DSISR";   break;
                case 19  : sprName = "DAR";     break;
                case 25  : sprName = "SDR1";    break;
                case 284 : sprName = "TBL";     break;
                case 285 : sprName = "TBU";     break;
                case 528 : sprName = "IBAT0U";  break;
                case 529 : sprName = "IBAT0L";  break;
                case 530 : sprName = "IBAT1U";  break;
                case 531 : sprName = "IBAT1L";  break;
                case 532 : sprName = "IBAT2U";  break;
                case 533 : sprName = "IBAT2L";  break;
                case 534 : sprName = "IBAT3U";  break;
                case 535 : sprName = "IBAT3L";  break;
                case 536 : sprName = "DBAT0U";  break;
                case 537 : sprName = "DBAT0L";  break;
                case 538 : sprName = "DBAT1U";  break;
                case 539 : sprName = "DBAT1L";  break;
                case 540 : sprName = "DBAT2U";  break;
                case 541 : sprName = "DBAT2L";  break;
                case 542 : sprName = "DBAT3U";  break;
                case 543 : sprName = "DBAT3L";  break;
                case 976 : sprName = "DMISS";   break;
                case 977 : sprName = "DCMP";    break;
                case 978 : sprName = "HASH1";   break;
                case 979 : sprName = "HASH2";   break;
                case 980 : sprName = "IMISS";   break;
                case 981 : sprName = "ICMP";    break;
                case 982 : sprName = "RPA";     break;
                case 984 : sprName = "TCR";     break;
                case 986 : sprName = "IBR";     break;
                case 987 : sprName = "ESASRR";  break;
                case 1008: sprName = "HID0";    break;
                case 1009: sprName = "HID1";    break;
                case 1010: sprName = "IABR";    break;
                case 1021: sprName = "SP";      break;
                case 1022: sprName = "LT";      break;
                default  : sprName = NULL;      break;
            }

            if (sprName)
            {
                AppendStr("%s", sprName);
            }
            else
            {
                AppendStr("%ld", value);
            }
        }
        else if (operand->op_Flags & PPC_OPERAND_RELATIVE)
        {
            if (addrFunc)
            {
                (*addrFunc)(instrAddr + value,result);
                result += strlen(result);
            }
            else
            {
                if (instrAddr == 0)
                {
                    AppendStr("%s","*");
                }
                AppendStr("0x%X",instrAddr + value);
            }
        }
        else if (operand->op_Flags & PPC_OPERAND_ABSOLUTE)
        {
            if (addrFunc)
            {
                (*addrFunc)(value & 0xffffffff,result);
                result += strlen(result);
            }
            else
            {
                AppendStr("0x%X",value & 0xffffffff);
            }
        }
        else if (operand->op_Flags & PPC_OPERAND_CR)
        {
            if (operand->op_Bits == 3)
            {
                AppendStr("cr%d", value);
            }
            else
            {
            static const char *cbnames[4] = { "lt", "gt", "eq", "so" };
            int32 cr;
            int32 cc;

                cr = value >> 2;
                if (cr)
                {
                    AppendStr("4*cr%d", cr);
                }

                cc = value & 3;
                if (cc)
                {
                    if (cr)
                    {
                        AppendStr("%s","+");
                    }

                    AppendStr("%s", cbnames[cc]);
                }
            }
        }
        else if (operand->op_Flags & PPC_OPERAND_HEX)
        {
            AppendStr("0x%lx", value);
        }
        else
        {
            if ((opcode->opc_Type == OPTYPE_LOAD) || (opcode->opc_Type == OPTYPE_STORE))
            {
                displacement     = value;
                got_displacement = TRUE;
            }

            if (value < -15 || value > 15)
            {
                AppendStr("0x%lx", value);
            }
            else
            {
                AppendStr("%ld", value);
            }
        }

        if (need_paren)
        {
            AppendStr("%s",")");
            need_paren = FALSE;
        }

        if (operand->op_Flags & PPC_OPERAND_PARENS)
        {
            need_paren = TRUE;
        }
        else
        {
	    need_comma = TRUE;
        }

        i++;
    }

    /* now generate a comment if we're asked to... */

    if (comment)
    {
        switch (opcode->opc_Type)
        {
            case OPTYPE_LOAD  : if (registers)
                                {
                                    if (got_displacement)
                                    {
                                        AppendComment("<-- 0x%X",(int32)registers->ppcr_GPRs[lastReg] + displacement);
                                    }
                                    else
                                    {
                                        AppendComment("<-- 0x%X",registers->ppcr_GPRs[lastReg] + registers->ppcr_GPRs[nextToLastReg]);
                                    }
                                }
                                break;

            case OPTYPE_STORE : if (registers)
                                {
                                    if (got_displacement)
                                    {
                                        AppendComment("-->0x%X",(int32)registers->ppcr_GPRs[lastReg] + displacement);
                                    }
                                    else
                                    {
                                        AppendComment("-->0x%X",registers->ppcr_GPRs[lastReg] + registers->ppcr_GPRs[nextToLastReg]);
                                    }
                                }
                                break;

            case OPTYPE_BRANCH: MakeBranchComment(instruction,instrAddr,value,registers,addrFunc,comment);
                                break;

            case OPTYPE_RFI   : if (registers)
                                {
                                    AppendComment("-->0x%X",registers->ppcr_SRR0);
                                }
                                break;

            default           : break;
        }
    }
}


/*****************************************************************************/


Boolean DisasmPPC(uint32              instruction,
                  uint32              instrAddr,
                  const PPCRegisters *registers,
                  MakeAddrFunc	      addrFunc,
                  char               *result,
                  char               *comment)
{
uint32           i, j, k;
const PPCOpcode *opcodes;
bool             invalid;

    if (comment)
        comment[0] = 0;

    opcodes = ppcOpcodeTables[PPC_OP(instruction)];
    if (opcodes)
    {
        i = 0;
        while (opcodes[i].opc_Name)
        {
            if ((instruction & opcodes[i].opc_Mask) == opcodes[i].opc_Opcode)
            {
                /* Make two passes over the operands.  First see if any of them
                 * have extraction functions, and, if they do, make sure the
	         	 * instruction is valid.
                 */
                invalid = FALSE;
                j = 0;
                while (k = opcodes[i].opc_Operands[j], k)
                {
					if (ppcOperands[k].op_Extract)
					{
						if (k >= (sizeof(ppcOperands) / sizeof(PPCOperand)))
						{
							fprintf(stderr, "Index OUT OF RANGE: i=%d,j=%d,k=%d,op_Extract=%X\n", i, j, k, ppcOperands[k].op_Extract);
							fflush(stderr);
							return FALSE;
						}
						else
	                        (*ppcOperands[k].op_Extract)(instruction, &invalid);
					}
                    j++;
                }

                if (!invalid)
                {
                    DisasmInstr(instruction, instrAddr,
                                registers, addrFunc,
                                &opcodes[i],result,comment);
                    return TRUE;
                }
            }
            i++;
        }
    }

    /* not found */

    if (instruction == 0x00000001)
    {
        sprintf(result,"DebugBreakpoint() (0x00000001)");
		return TRUE;
    }
    else
    {
    char c0, c1, c2, c3;

        c0 = (instruction >> 24) & 0xff;
        c1 = (instruction >> 16) & 0xff;
        c2 = (instruction >> 8) & 0xff;
        c3 = (instruction) & 0xff;

        sprintf(result,"???? (0x%08x, '%c%c%c%c')",instruction,
                                                   isprint(c0) ? c0 : '.',
                                                   isprint(c1) ? c1 : '.',
                                                   isprint(c2) ? c2 : '.',
                                                   isprint(c3) ? c3 : '.');
    }

    return FALSE;
}


/*****************************************************************************/


/* for 3DODebug */
void dis602(uint32** instructions, char* buffer)
{
    char comment[300];
	DisasmPPC(**instructions, 0,0,0, buffer, comment);
	strcat(buffer, comment);
    (*instructions)++;
}
