/* Opcode table for the Alpha.

   Copyright 1993, 1995 Free Software Foundation, Inc.
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Contributed by sac@cygnus.com.  */


#define OSF_ASMCODE 1

/* Alpha opcode format */
#define RA(x) (((x)>>21)& 0x1f)
#define RB(x) (((x)>>16)& 0x1f)
#define RC(x) (((x)>>0) & 0x1f)
#define DISP(x) ((((x) & 0xffff) ^ 0x8000)-0x8000)
#define BDISP(x) ((((x) & 0x1fffff) ^ 0x100000)-0x100000)
#define OPCODE(x) (((x) >>26)&0x3f)
#define JUMP_OPTYPE(x) (((x)>>14) & 0xf)
#define JUMP_HINT(x) ((x) & 0x3fff)
#define JDISP(x) ((((x) & 0x3fff) ^ 0x2000)-0x2000)
#define OP_OPTYPE(x) (((x)>>5)&0x7f)
#define OP_IS_CONSTANT(x) ((x) & (1<<12))
#define LITERAL(x) (((x)>>13) & 0xff)


/* Shapes

   Memory instruction format    oooo ooaa aaab bbbb dddd dddd dddd dddd
   Memory with function         oooo ooaa aaab bbbb ffff ffff ffff ffff
   Memory branch                oooo ooaa aaab bbbb BBff ffff ffff ffff
   Branch                       oooo ooaa aaad dddd dddd dddd dddd dddd
   Operate reg                  oooo ooaa aaab bbbb ***F ffff fffc cccc
   Operate cont                 oooo ooaa aaal llll lll1 ffff fffc cccc
   FP reg                       oooo ooaa aaab bbbb 000f ffff fffc cccc
   Pal                          oooo oodd dddd dddd dddd dddd dddd dddd

   The following masks just give opcode & function
*/

#define MEMORY_FORMAT_MASK 		0xfc000000
#define MEMORY_FUNCTION_FORMAT_MASK	0xfc00ffff
#define MEMORY_BRANCH_FORMAT_MASK 	0xfc00c000
#define BRANCH_FORMAT_MASK    		0xfc000000
#define OPERATE_FORMAT_MASK		0xfc000fe0
#define FLOAT_FORMAT_MASK		0xfc000fe0

typedef struct 
{
  unsigned i;
  char *name;
  int type;
} alpha_insn;

#ifdef DEFINE_TABLE

char *alpha_regs[32] =
{
   "v0",  "t0",  "t1",  "t2",  "t3",  "t4",  "t5",  "t6",
   "t7",  "s0",  "s1",  "s2",  "s3",  "s4",  "s5",  "fp",
   "a0",  "a1",  "a2",  "a3",  "a4",  "a5",  "t8",  "t9",
  "t10", "t11",  "ra", "t12",  "at",  "gp",  "sp", "zero"
};

#define MEMORY_FORMAT_CODE 1
#define MEMORY_FORMAT(op, name) \
 { op << 26, name, MEMORY_FORMAT_CODE }

#define MEMORY_BRANCH_FORMAT_CODE 2
#define MEMORY_BRANCH_FORMAT(op, func, name) \
{ (op<<26)+(func<<14),name, MEMORY_BRANCH_FORMAT_CODE }

#define MEMORY_FUNCTION_FORMAT_CODE 3
#define MEMORY_FORMAT_FUNCTION(op, func, name) \
 { (op<<26)+(func), name, MEMORY_FUNCTION_FORMAT_CODE }

#define BRANCH_FORMAT_CODE  4
#define BRANCH_FORMAT(op, name) \
 { (op<<26), name , BRANCH_FORMAT_CODE }

#define OPERATE_FORMAT_CODE 5
#define OPERATE_FORMAT(op, extra,name)  \
 {(op<<26)+(extra<<5),name , OPERATE_FORMAT_CODE}

#define FLOAT_FORMAT_CODE 6
#define FLOAT_FORMAT(op, extra,name) \
{(op<<26)+(extra<<5),name , FLOAT_FORMAT_CODE }

#define PAL_FORMAT_CODE 7
#define PAL_FORMAT(op, extra, name) \
{(op<<26)+(extra),name, PAL_FORMAT_CODE}  

#define FLOAT_MEMORY_FORMAT_CODE 8
#define FLOAT_MEMORY_FORMAT(op, name) \
 { op << 26, name, FLOAT_MEMORY_FORMAT_CODE }

#define FLOAT_BRANCH_FORMAT_CODE  9
#define FLOAT_BRANCH_FORMAT(op, name) \
 { (op<<26), name , FLOAT_BRANCH_FORMAT_CODE }

alpha_insn alpha_insn_set[] =
{
  
/* Memory format instruction opcodes */
MEMORY_FORMAT(		0x08, "lda"),
FLOAT_MEMORY_FORMAT(	0x21, "ldg"),
MEMORY_FORMAT(		0x29, "ldq"),
FLOAT_MEMORY_FORMAT(	0x22, "lds"),
FLOAT_MEMORY_FORMAT(	0x25, "stg"),
MEMORY_FORMAT(		0x2d, "stq"),
FLOAT_MEMORY_FORMAT(	0x26, "sts"),
MEMORY_FORMAT(		0x09, "ldah"),
MEMORY_FORMAT(		0x28, "ldl"),
MEMORY_FORMAT(		0x2b, "ldq_l"),
FLOAT_MEMORY_FORMAT(	0x23, "ldt"),
MEMORY_FORMAT(		0x2c, "stl"),
MEMORY_FORMAT(		0x2f, "stq_c"),
FLOAT_MEMORY_FORMAT(	0x27, "stt"),
FLOAT_MEMORY_FORMAT(	0x20, "ldf"),
MEMORY_FORMAT(		0x2a, "ldl_l"),
MEMORY_FORMAT(		0x0b, "ldq_u"),
FLOAT_MEMORY_FORMAT(	0x24, "stf"),
MEMORY_FORMAT(		0x2e, "stl_c"),
MEMORY_FORMAT(		0x0f, "stq_u"),

/* Memory format instructions with a function code */
MEMORY_FORMAT_FUNCTION(	0x18, 0x8000, "fetch"),
MEMORY_FORMAT_FUNCTION(	0x18, 0xe000, "rc"),
MEMORY_FORMAT_FUNCTION(	0x18, 0x0000, "trapb"),
MEMORY_FORMAT_FUNCTION(	0x18, 0xa000, "fetch_m"),
MEMORY_FORMAT_FUNCTION(	0x18, 0xc000, "rpcc"),
MEMORY_FORMAT_FUNCTION(	0x18, 0x4000, "mb"),
MEMORY_FORMAT_FUNCTION(	0x18, 0xf000, "rs"),

MEMORY_BRANCH_FORMAT(	0x1a, 0x0, "jmp"),
MEMORY_BRANCH_FORMAT(	0x1a, 0x2, "ret"),
MEMORY_BRANCH_FORMAT(	0x1a, 0x1, "jsr"),
MEMORY_BRANCH_FORMAT(	0x1a, 0x3, "jsr_coroutine"),


BRANCH_FORMAT(		0x30, "br"),
FLOAT_BRANCH_FORMAT(	0x33, "fble"),
FLOAT_BRANCH_FORMAT(	0x36, "fbge"),
BRANCH_FORMAT(		0x39, "beq"),
BRANCH_FORMAT(		0x3c, "blbs"),
BRANCH_FORMAT(		0x3f, "bgt"),
FLOAT_BRANCH_FORMAT(	0x31, "fbeq"),
BRANCH_FORMAT(		0x34, "bsr"),
FLOAT_BRANCH_FORMAT(	0x37, "fbgt"),
BRANCH_FORMAT(		0x3a, "blt"),
BRANCH_FORMAT(		0x3d, "bne"),
FLOAT_BRANCH_FORMAT(	0x32, "fblt"),
FLOAT_BRANCH_FORMAT(	0x35, "fbne"),
BRANCH_FORMAT(		0x38, "blbc"),
BRANCH_FORMAT(		0x3b, "ble"),
BRANCH_FORMAT(		0x3e, "bge"),

OPERATE_FORMAT(0x10, 0x00, "addl"),
OPERATE_FORMAT(0x10, 0x02, "s4addl"),
OPERATE_FORMAT(0x10, 0x09, "subl"),
OPERATE_FORMAT(0x10, 0x0b, "s4subl"),
OPERATE_FORMAT(0x10, 0x0f, "cmpbge"),
OPERATE_FORMAT(0x10, 0x12, "s8addl"),
OPERATE_FORMAT(0x10, 0x1b, "s8subl"),
OPERATE_FORMAT(0x10, 0x1d, "cmpult"),
OPERATE_FORMAT(0x10, 0x20, "addq"),
OPERATE_FORMAT(0x10, 0x22, "s4addq"),
OPERATE_FORMAT(0x10, 0x29, "subq"),
OPERATE_FORMAT(0x10, 0x2b, "s4subq"),
OPERATE_FORMAT(0x10, 0x2d, "cmpeq"),
OPERATE_FORMAT(0x10, 0x32, "s8addq"),
OPERATE_FORMAT(0x10, 0x3b, "s8subq"),
OPERATE_FORMAT(0x10, 0x3d, "cmpule"),
OPERATE_FORMAT(0x10, 0x40, "addlv"),
OPERATE_FORMAT(0x10, 0x49, "sublv"),
OPERATE_FORMAT(0x10, 0x4d, "cmplt"),
OPERATE_FORMAT(0x10, 0x60, "addqv"),
OPERATE_FORMAT(0x10, 0x69, "subqv"),
OPERATE_FORMAT(0x10, 0x6d, "cmple"),
OPERATE_FORMAT(0x11, 0x00, "and"),
OPERATE_FORMAT(0x11, 0x08, "bic"),
OPERATE_FORMAT(0x11, 0x14, "cmovlbs"),
OPERATE_FORMAT(0x11, 0x16, "cmovlbc"),
OPERATE_FORMAT(0x11, 0x20, "bis"),
OPERATE_FORMAT(0x11, 0x24, "cmoveq"),
OPERATE_FORMAT(0x11, 0x26, "cmovne"),
OPERATE_FORMAT(0x11, 0x28, "ornot"),
OPERATE_FORMAT(0x11, 0x40, "xor"),
OPERATE_FORMAT(0x11, 0x44, "cmovlt"),
OPERATE_FORMAT(0x11, 0x46, "cmovge"),
OPERATE_FORMAT(0x11, 0x48, "eqv"),
OPERATE_FORMAT(0x11, 0x64, "cmovle"),
OPERATE_FORMAT(0x11, 0x66, "cmovgt"),
OPERATE_FORMAT(0x12, 0x02, "mskbl"),
OPERATE_FORMAT(0x12, 0x06, "extbl"),
OPERATE_FORMAT(0x12, 0x0b, "insbl"),
OPERATE_FORMAT(0x12, 0x12, "mskwl"),
OPERATE_FORMAT(0x12, 0x16, "extwl"),
OPERATE_FORMAT(0x12, 0x1b, "inswl"),
OPERATE_FORMAT(0x12, 0x22, "mskll"),
OPERATE_FORMAT(0x12, 0x26, "extll"),
OPERATE_FORMAT(0x12, 0x2b, "insll"),
OPERATE_FORMAT(0x12, 0x30, "zap"),
OPERATE_FORMAT(0x12, 0x31, "zapnot"),
OPERATE_FORMAT(0x12, 0x32, "mskql"),
OPERATE_FORMAT(0x12, 0x34, "srl"),
OPERATE_FORMAT(0x12, 0x36, "extql"),
OPERATE_FORMAT(0x12, 0x39, "sll"),
OPERATE_FORMAT(0x12, 0x3b, "insql"),
OPERATE_FORMAT(0x12, 0x3c, "sra"),
OPERATE_FORMAT(0x12, 0x52, "mskwh"),
OPERATE_FORMAT(0x12, 0x57, "inswh"),
OPERATE_FORMAT(0x12, 0x5a, "extwh"),
OPERATE_FORMAT(0x12, 0x62, "msklh"),
OPERATE_FORMAT(0x12, 0x67, "inslh"),
OPERATE_FORMAT(0x12, 0x6a, "extlh"),
OPERATE_FORMAT(0x12, 0x72, "mskqh"),
OPERATE_FORMAT(0x12, 0x77, "insqh"),
OPERATE_FORMAT(0x12, 0x7a, "extqh"),
OPERATE_FORMAT(0x13, 0x00, "mull"),
OPERATE_FORMAT(0x13, 0x20, "mulq"),
OPERATE_FORMAT(0x13, 0x30, "umulh"),
OPERATE_FORMAT(0x13, 0x40, "mullv"),
OPERATE_FORMAT(0x13, 0x60, "mulqv"),

FLOAT_FORMAT(0x17, 0x20, "cpys"),
FLOAT_FORMAT(0x17, 0x21, "cpysn"),
FLOAT_FORMAT(0x17, 0x22, "cpyse"),
FLOAT_FORMAT(0x17, 0x24, "mt_fpcr"),
FLOAT_FORMAT(0x17, 0x25, "mf_fpcr"),
FLOAT_FORMAT(0x17, 0x2a, "fcmoveq"),
FLOAT_FORMAT(0x17, 0x2b, "fcmovne"),
FLOAT_FORMAT(0x17, 0x2c, "fcmovlt"),
FLOAT_FORMAT(0x17, 0x2d, "fcmovge"),
FLOAT_FORMAT(0x17, 0x2e, "fcmovle"),
FLOAT_FORMAT(0x17, 0x2f, "fcmovgt"),
FLOAT_FORMAT(0x17, 0x10, "cvtlq"),
FLOAT_FORMAT(0x17, 0x30, "cvtql"),
FLOAT_FORMAT(0x17, 0x130, "cvtql/v"),
FLOAT_FORMAT(0x17, 0x530, "cvtql/sv"),

/* IEEE floating point operations: */

FLOAT_FORMAT(0x16, 0x080, "adds"),
FLOAT_FORMAT(0x16, 0x000, "adds/c"),
FLOAT_FORMAT(0x16, 0x040, "adds/m"),
FLOAT_FORMAT(0x16, 0x0c0, "adds/d"),
FLOAT_FORMAT(0x16, 0x180, "adds/u"),
FLOAT_FORMAT(0x16, 0x100, "adds/uc"),
FLOAT_FORMAT(0x16, 0x140, "adds/um"),
FLOAT_FORMAT(0x16, 0x1c0, "adds/ud"),
FLOAT_FORMAT(0x16, 0x580, "adds/su"),
FLOAT_FORMAT(0x16, 0x500, "adds/suc"),
FLOAT_FORMAT(0x16, 0x540, "adds/sum"),
FLOAT_FORMAT(0x16, 0x5c0, "adds/sud"),
FLOAT_FORMAT(0x16, 0x780, "adds/sui"),
FLOAT_FORMAT(0x16, 0x700, "adds/suic"),
FLOAT_FORMAT(0x16, 0x740, "adds/suim"),
FLOAT_FORMAT(0x16, 0x7c0, "adds/suid"),
FLOAT_FORMAT(0x16, 0x0a0, "addt"),
FLOAT_FORMAT(0x16, 0x020, "addt/c"),
FLOAT_FORMAT(0x16, 0x060, "addt/m"),
FLOAT_FORMAT(0x16, 0x0e0, "addt/d"),
FLOAT_FORMAT(0x16, 0x1a0, "addt/u"),
FLOAT_FORMAT(0x16, 0x120, "addt/uc"),
FLOAT_FORMAT(0x16, 0x160, "addt/um"),
FLOAT_FORMAT(0x16, 0x1e0, "addt/ud"),
FLOAT_FORMAT(0x16, 0x5a0, "addt/su"),
FLOAT_FORMAT(0x16, 0x520, "addt/suc"),
FLOAT_FORMAT(0x16, 0x560, "addt/sum"),
FLOAT_FORMAT(0x16, 0x5e0, "addt/sud"),
FLOAT_FORMAT(0x16, 0x7a0, "addt/sui"),
FLOAT_FORMAT(0x16, 0x720, "addt/suic"),
FLOAT_FORMAT(0x16, 0x760, "addt/suim"),
FLOAT_FORMAT(0x16, 0x7e0, "addt/suid"),
FLOAT_FORMAT(0x16, 0x0a5, "cmpteq"),
FLOAT_FORMAT(0x16, 0x025, "cmpteq/c"),
FLOAT_FORMAT(0x16, 0x065, "cmpteq/m"),
FLOAT_FORMAT(0x16, 0x0e5, "cmpteq/d"),
FLOAT_FORMAT(0x16, 0x1a5, "cmpteq/u"),
FLOAT_FORMAT(0x16, 0x125, "cmpteq/uc"),
FLOAT_FORMAT(0x16, 0x165, "cmpteq/um"),
FLOAT_FORMAT(0x16, 0x1e5, "cmpteq/ud"),
FLOAT_FORMAT(0x16, 0x5a5, "cmpteq/su"),
FLOAT_FORMAT(0x16, 0x525, "cmpteq/suc"),
FLOAT_FORMAT(0x16, 0x565, "cmpteq/sum"),
FLOAT_FORMAT(0x16, 0x5e5, "cmpteq/sud"),
FLOAT_FORMAT(0x16, 0x7a5, "cmpteq/sui"),
FLOAT_FORMAT(0x16, 0x725, "cmpteq/suic"),
FLOAT_FORMAT(0x16, 0x765, "cmpteq/suim"),
FLOAT_FORMAT(0x16, 0x7e5, "cmpteq/suid"),
FLOAT_FORMAT(0x16, 0x0a6, "cmptlt"),
FLOAT_FORMAT(0x16, 0x026, "cmptlt/c"),
FLOAT_FORMAT(0x16, 0x066, "cmptlt/m"),
FLOAT_FORMAT(0x16, 0x0e6, "cmptlt/d"),
FLOAT_FORMAT(0x16, 0x1a6, "cmptlt/u"),
FLOAT_FORMAT(0x16, 0x126, "cmptlt/uc"),
FLOAT_FORMAT(0x16, 0x166, "cmptlt/um"),
FLOAT_FORMAT(0x16, 0x1e6, "cmptlt/ud"),
FLOAT_FORMAT(0x16, 0x5a6, "cmptlt/su"),
FLOAT_FORMAT(0x16, 0x526, "cmptlt/suc"),
FLOAT_FORMAT(0x16, 0x566, "cmptlt/sum"),
FLOAT_FORMAT(0x16, 0x5e6, "cmptlt/sud"),
FLOAT_FORMAT(0x16, 0x7a6, "cmptlt/sui"),
FLOAT_FORMAT(0x16, 0x726, "cmptlt/suic"),
FLOAT_FORMAT(0x16, 0x766, "cmptlt/suim"),
FLOAT_FORMAT(0x16, 0x7e6, "cmptlt/suid"),
FLOAT_FORMAT(0x16, 0x0a7, "cmptle"),
FLOAT_FORMAT(0x16, 0x027, "cmptle/c"),
FLOAT_FORMAT(0x16, 0x067, "cmptle/m"),
FLOAT_FORMAT(0x16, 0x0e7, "cmptle/d"),
FLOAT_FORMAT(0x16, 0x1a7, "cmptle/u"),
FLOAT_FORMAT(0x16, 0x127, "cmptle/uc"),
FLOAT_FORMAT(0x16, 0x167, "cmptle/um"),
FLOAT_FORMAT(0x16, 0x1e7, "cmptle/ud"),
FLOAT_FORMAT(0x16, 0x5a7, "cmptle/su"),
FLOAT_FORMAT(0x16, 0x527, "cmptle/suc"),
FLOAT_FORMAT(0x16, 0x567, "cmptle/sum"),
FLOAT_FORMAT(0x16, 0x5e7, "cmptle/sud"),
FLOAT_FORMAT(0x16, 0x7a7, "cmptle/sui"),
FLOAT_FORMAT(0x16, 0x727, "cmptle/suic"),
FLOAT_FORMAT(0x16, 0x767, "cmptle/suim"),
FLOAT_FORMAT(0x16, 0x7e7, "cmptle/suid"),
FLOAT_FORMAT(0x16, 0x0a4, "cmptun"),
FLOAT_FORMAT(0x16, 0x024, "cmptun/c"),
FLOAT_FORMAT(0x16, 0x064, "cmptun/m"),
FLOAT_FORMAT(0x16, 0x0e4, "cmptun/d"),
FLOAT_FORMAT(0x16, 0x1a4, "cmptun/u"),
FLOAT_FORMAT(0x16, 0x124, "cmptun/uc"),
FLOAT_FORMAT(0x16, 0x164, "cmptun/um"),
FLOAT_FORMAT(0x16, 0x1e4, "cmptun/ud"),
FLOAT_FORMAT(0x16, 0x5a4, "cmptun/su"),
FLOAT_FORMAT(0x16, 0x524, "cmptun/suc"),
FLOAT_FORMAT(0x16, 0x564, "cmptun/sum"),
FLOAT_FORMAT(0x16, 0x5e4, "cmptun/sud"),
FLOAT_FORMAT(0x16, 0x7a4, "cmptun/sui"),
FLOAT_FORMAT(0x16, 0x724, "cmptun/suic"),
FLOAT_FORMAT(0x16, 0x764, "cmptun/suim"),
FLOAT_FORMAT(0x16, 0x7e4, "cmptun/suid"),
FLOAT_FORMAT(0x16, 0x0bc, "cvtqs"),
FLOAT_FORMAT(0x16, 0x03c, "cvtqs/c"),
FLOAT_FORMAT(0x16, 0x07c, "cvtqs/m"),
FLOAT_FORMAT(0x16, 0x0fc, "cvtqs/d"),
FLOAT_FORMAT(0x16, 0x1bc, "cvtqs/u"),
FLOAT_FORMAT(0x16, 0x13c, "cvtqs/uc"),
FLOAT_FORMAT(0x16, 0x17c, "cvtqs/um"),
FLOAT_FORMAT(0x16, 0x1fc, "cvtqs/ud"),
FLOAT_FORMAT(0x16, 0x5bc, "cvtqs/su"),
FLOAT_FORMAT(0x16, 0x53c, "cvtqs/suc"),
FLOAT_FORMAT(0x16, 0x57c, "cvtqs/sum"),
FLOAT_FORMAT(0x16, 0x5fc, "cvtqs/sud"),
FLOAT_FORMAT(0x16, 0x7bc, "cvtqs/sui"),
FLOAT_FORMAT(0x16, 0x73c, "cvtqs/suic"),
FLOAT_FORMAT(0x16, 0x77c, "cvtqs/suim"),
FLOAT_FORMAT(0x16, 0x7fc, "cvtqs/suid"),
FLOAT_FORMAT(0x16, 0x0be, "cvtqt"),
FLOAT_FORMAT(0x16, 0x03e, "cvtqt/c"),
FLOAT_FORMAT(0x16, 0x07e, "cvtqt/m"),
FLOAT_FORMAT(0x16, 0x0fe, "cvtqt/d"),
FLOAT_FORMAT(0x16, 0x1be, "cvtqt/u"),
FLOAT_FORMAT(0x16, 0x13e, "cvtqt/uc"),
FLOAT_FORMAT(0x16, 0x17e, "cvtqt/um"),
FLOAT_FORMAT(0x16, 0x1fe, "cvtqt/ud"),
FLOAT_FORMAT(0x16, 0x5be, "cvtqt/su"),
FLOAT_FORMAT(0x16, 0x53e, "cvtqt/suc"),
FLOAT_FORMAT(0x16, 0x57e, "cvtqt/sum"),
FLOAT_FORMAT(0x16, 0x5fe, "cvtqt/sud"),
FLOAT_FORMAT(0x16, 0x7be, "cvtqt/sui"),
FLOAT_FORMAT(0x16, 0x73e, "cvtqt/suic"),
FLOAT_FORMAT(0x16, 0x77e, "cvtqt/suim"),
FLOAT_FORMAT(0x16, 0x7fe, "cvtqt/suid"),
FLOAT_FORMAT(0x16, 0x0ac, "cvtts"),
FLOAT_FORMAT(0x16, 0x02c, "cvtts/c"),
FLOAT_FORMAT(0x16, 0x06c, "cvtts/m"),
FLOAT_FORMAT(0x16, 0x0ec, "cvtts/d"),
FLOAT_FORMAT(0x16, 0x1ac, "cvtts/u"),
FLOAT_FORMAT(0x16, 0x12c, "cvtts/uc"),
FLOAT_FORMAT(0x16, 0x16c, "cvtts/um"),
FLOAT_FORMAT(0x16, 0x1ec, "cvtts/ud"),
FLOAT_FORMAT(0x16, 0x5ac, "cvtts/su"),
FLOAT_FORMAT(0x16, 0x52c, "cvtts/suc"),
FLOAT_FORMAT(0x16, 0x56c, "cvtts/sum"),
FLOAT_FORMAT(0x16, 0x5ec, "cvtts/sud"),
FLOAT_FORMAT(0x16, 0x7ac, "cvtts/sui"),
FLOAT_FORMAT(0x16, 0x72c, "cvtts/suic"),
FLOAT_FORMAT(0x16, 0x76c, "cvtts/suim"),
FLOAT_FORMAT(0x16, 0x7ec, "cvtts/suid"),
FLOAT_FORMAT(0x16, 0x083, "divs"),
FLOAT_FORMAT(0x16, 0x003, "divs/c"),
FLOAT_FORMAT(0x16, 0x043, "divs/m"),
FLOAT_FORMAT(0x16, 0x0c3, "divs/d"),
FLOAT_FORMAT(0x16, 0x183, "divs/u"),
FLOAT_FORMAT(0x16, 0x103, "divs/uc"),
FLOAT_FORMAT(0x16, 0x143, "divs/um"),
FLOAT_FORMAT(0x16, 0x1c3, "divs/ud"),
FLOAT_FORMAT(0x16, 0x583, "divs/su"),
FLOAT_FORMAT(0x16, 0x503, "divs/suc"),
FLOAT_FORMAT(0x16, 0x543, "divs/sum"),
FLOAT_FORMAT(0x16, 0x5c3, "divs/sud"),
FLOAT_FORMAT(0x16, 0x783, "divs/sui"),
FLOAT_FORMAT(0x16, 0x703, "divs/suic"),
FLOAT_FORMAT(0x16, 0x743, "divs/suim"),
FLOAT_FORMAT(0x16, 0x7c3, "divs/suid"),
FLOAT_FORMAT(0x16, 0x0a3, "divt"),
FLOAT_FORMAT(0x16, 0x023, "divt/c"),
FLOAT_FORMAT(0x16, 0x063, "divt/m"),
FLOAT_FORMAT(0x16, 0x0e3, "divt/d"),
FLOAT_FORMAT(0x16, 0x1a3, "divt/u"),
FLOAT_FORMAT(0x16, 0x123, "divt/uc"),
FLOAT_FORMAT(0x16, 0x163, "divt/um"),
FLOAT_FORMAT(0x16, 0x1e3, "divt/ud"),
FLOAT_FORMAT(0x16, 0x5a3, "divt/su"),
FLOAT_FORMAT(0x16, 0x523, "divt/suc"),
FLOAT_FORMAT(0x16, 0x563, "divt/sum"),
FLOAT_FORMAT(0x16, 0x5e3, "divt/sud"),
FLOAT_FORMAT(0x16, 0x7a3, "divt/sui"),
FLOAT_FORMAT(0x16, 0x723, "divt/suic"),
FLOAT_FORMAT(0x16, 0x763, "divt/suim"),
FLOAT_FORMAT(0x16, 0x7e3, "divt/suid"),
FLOAT_FORMAT(0x16, 0x082, "muls"),
FLOAT_FORMAT(0x16, 0x002, "muls/c"),
FLOAT_FORMAT(0x16, 0x042, "muls/m"),
FLOAT_FORMAT(0x16, 0x0c2, "muls/d"),
FLOAT_FORMAT(0x16, 0x182, "muls/u"),
FLOAT_FORMAT(0x16, 0x102, "muls/uc"),
FLOAT_FORMAT(0x16, 0x142, "muls/um"),
FLOAT_FORMAT(0x16, 0x1c2, "muls/ud"),
FLOAT_FORMAT(0x16, 0x582, "muls/su"),
FLOAT_FORMAT(0x16, 0x502, "muls/suc"),
FLOAT_FORMAT(0x16, 0x542, "muls/sum"),
FLOAT_FORMAT(0x16, 0x5c2, "muls/sud"),
FLOAT_FORMAT(0x16, 0x782, "muls/sui"),
FLOAT_FORMAT(0x16, 0x702, "muls/suic"),
FLOAT_FORMAT(0x16, 0x742, "muls/suim"),
FLOAT_FORMAT(0x16, 0x7c2, "muls/suid"),
FLOAT_FORMAT(0x16, 0x0a2, "mult"),
FLOAT_FORMAT(0x16, 0x022, "mult/c"),
FLOAT_FORMAT(0x16, 0x062, "mult/m"),
FLOAT_FORMAT(0x16, 0x0e2, "mult/d"),
FLOAT_FORMAT(0x16, 0x1a2, "mult/u"),
FLOAT_FORMAT(0x16, 0x122, "mult/uc"),
FLOAT_FORMAT(0x16, 0x162, "mult/um"),
FLOAT_FORMAT(0x16, 0x1e2, "mult/ud"),
FLOAT_FORMAT(0x16, 0x5a2, "mult/su"),
FLOAT_FORMAT(0x16, 0x522, "mult/suc"),
FLOAT_FORMAT(0x16, 0x562, "mult/sum"),
FLOAT_FORMAT(0x16, 0x5e2, "mult/sud"),
FLOAT_FORMAT(0x16, 0x7a2, "mult/sui"),
FLOAT_FORMAT(0x16, 0x722, "mult/suic"),
FLOAT_FORMAT(0x16, 0x762, "mult/suim"),
FLOAT_FORMAT(0x16, 0x7e2, "mult/suid"),
FLOAT_FORMAT(0x16, 0x081, "subs"),
FLOAT_FORMAT(0x16, 0x001, "subs/c"),
FLOAT_FORMAT(0x16, 0x041, "subs/m"),
FLOAT_FORMAT(0x16, 0x0c1, "subs/d"),
FLOAT_FORMAT(0x16, 0x181, "subs/u"),
FLOAT_FORMAT(0x16, 0x101, "subs/uc"),
FLOAT_FORMAT(0x16, 0x141, "subs/um"),
FLOAT_FORMAT(0x16, 0x1c1, "subs/ud"),
FLOAT_FORMAT(0x16, 0x581, "subs/su"),
FLOAT_FORMAT(0x16, 0x501, "subs/suc"),
FLOAT_FORMAT(0x16, 0x541, "subs/sum"),
FLOAT_FORMAT(0x16, 0x5c1, "subs/sud"),
FLOAT_FORMAT(0x16, 0x781, "subs/sui"),
FLOAT_FORMAT(0x16, 0x701, "subs/suic"),
FLOAT_FORMAT(0x16, 0x741, "subs/suim"),
FLOAT_FORMAT(0x16, 0x7c1, "subs/suid"),
FLOAT_FORMAT(0x16, 0x0a1, "subt"),
FLOAT_FORMAT(0x16, 0x021, "subt/c"),
FLOAT_FORMAT(0x16, 0x061, "subt/m"),
FLOAT_FORMAT(0x16, 0x0e1, "subt/d"),
FLOAT_FORMAT(0x16, 0x1a1, "subt/u"),
FLOAT_FORMAT(0x16, 0x121, "subt/uc"),
FLOAT_FORMAT(0x16, 0x161, "subt/um"),
FLOAT_FORMAT(0x16, 0x1e1, "subt/ud"),
FLOAT_FORMAT(0x16, 0x5a1, "subt/su"),
FLOAT_FORMAT(0x16, 0x521, "subt/suc"),
FLOAT_FORMAT(0x16, 0x561, "subt/sum"),
FLOAT_FORMAT(0x16, 0x5e1, "subt/sud"),
FLOAT_FORMAT(0x16, 0x7a1, "subt/sui"),
FLOAT_FORMAT(0x16, 0x721, "subt/suic"),
FLOAT_FORMAT(0x16, 0x761, "subt/suim"),
FLOAT_FORMAT(0x16, 0x7e1, "subt/suid"),

/* VAX floating point operations: */

FLOAT_FORMAT(0x16, 0x080, "addf"),
FLOAT_FORMAT(0x16, 0x000, "addf/c"),
FLOAT_FORMAT(0x16, 0x180, "addf/u"),
FLOAT_FORMAT(0x16, 0x100, "addf/uc"),
FLOAT_FORMAT(0x16, 0x480, "addf/s"),
FLOAT_FORMAT(0x16, 0x400, "addf/sc"),
FLOAT_FORMAT(0x16, 0x580, "addf/su"),
FLOAT_FORMAT(0x16, 0x500, "addf/suc"),
FLOAT_FORMAT(0x16, 0x09e, "cvtdg"),
FLOAT_FORMAT(0x16, 0x01e, "cvtdg/c"),
FLOAT_FORMAT(0x16, 0x19e, "cvtdg/u"),
FLOAT_FORMAT(0x16, 0x11e, "cvtdg/uc"),
FLOAT_FORMAT(0x16, 0x49e, "cvtdg/s"),
FLOAT_FORMAT(0x16, 0x41e, "cvtdg/sc"),
FLOAT_FORMAT(0x16, 0x59e, "cvtdg/su"),
FLOAT_FORMAT(0x16, 0x51e, "cvtdg/suc"),
FLOAT_FORMAT(0x16, 0x0a0, "addg"),
FLOAT_FORMAT(0x16, 0x020, "addg/c"),
FLOAT_FORMAT(0x16, 0x1a0, "addg/u"),
FLOAT_FORMAT(0x16, 0x120, "addg/uc"),
FLOAT_FORMAT(0x16, 0x4a0, "addg/s"),
FLOAT_FORMAT(0x16, 0x420, "addg/sc"),
FLOAT_FORMAT(0x16, 0x5a0, "addg/su"),
FLOAT_FORMAT(0x16, 0x520, "addg/suc"),
FLOAT_FORMAT(0x16, 0x0a5, "cmpgeq"),
FLOAT_FORMAT(0x16, 0x4a5, "cmpgeq/s"),
FLOAT_FORMAT(0x16, 0x0a6, "cmpglt"),
FLOAT_FORMAT(0x16, 0x4a6, "cmpglt/s"),
FLOAT_FORMAT(0x16, 0x0a7, "cmpgle"),
FLOAT_FORMAT(0x16, 0x4a7, "cmpgle/s"),
FLOAT_FORMAT(0x16, 0x0ac, "cvtgf"),
FLOAT_FORMAT(0x16, 0x02c, "cvtgf/c"),
FLOAT_FORMAT(0x16, 0x1ac, "cvtgf/u"),
FLOAT_FORMAT(0x16, 0x12c, "cvtgf/uc"),
FLOAT_FORMAT(0x16, 0x4ac, "cvtgf/s"),
FLOAT_FORMAT(0x16, 0x42c, "cvtgf/sc"),
FLOAT_FORMAT(0x16, 0x5ac, "cvtgf/su"),
FLOAT_FORMAT(0x16, 0x52c, "cvtgf/suc"),
FLOAT_FORMAT(0x16, 0x0ad, "cvtgd"),
FLOAT_FORMAT(0x16, 0x02d, "cvtgd/c"),
FLOAT_FORMAT(0x16, 0x1ad, "cvtgd/u"),
FLOAT_FORMAT(0x16, 0x12d, "cvtgd/uc"),
FLOAT_FORMAT(0x16, 0x4ad, "cvtgd/s"),
FLOAT_FORMAT(0x16, 0x42d, "cvtgd/sc"),
FLOAT_FORMAT(0x16, 0x5ad, "cvtgd/su"),
FLOAT_FORMAT(0x16, 0x52d, "cvtgd/suc"),
FLOAT_FORMAT(0x16, 0x0bc, "cvtqf"),
FLOAT_FORMAT(0x16, 0x03c, "cvtqf/c"),
FLOAT_FORMAT(0x16, 0x0be, "cvtqg"),
FLOAT_FORMAT(0x16, 0x03e, "cvtqg/c"),
FLOAT_FORMAT(0x16, 0x083, "divf"),
FLOAT_FORMAT(0x16, 0x003, "divf/c"),
FLOAT_FORMAT(0x16, 0x183, "divf/u"),
FLOAT_FORMAT(0x16, 0x103, "divf/uc"),
FLOAT_FORMAT(0x16, 0x483, "divf/s"),
FLOAT_FORMAT(0x16, 0x403, "divf/sc"),
FLOAT_FORMAT(0x16, 0x583, "divf/su"),
FLOAT_FORMAT(0x16, 0x503, "divf/suc"),
FLOAT_FORMAT(0x16, 0x0a3, "divg"),
FLOAT_FORMAT(0x16, 0x023, "divg/c"),
FLOAT_FORMAT(0x16, 0x1a3, "divg/u"),
FLOAT_FORMAT(0x16, 0x123, "divg/uc"),
FLOAT_FORMAT(0x16, 0x4a3, "divg/s"),
FLOAT_FORMAT(0x16, 0x423, "divg/sc"),
FLOAT_FORMAT(0x16, 0x5a3, "divg/su"),
FLOAT_FORMAT(0x16, 0x523, "divg/suc"),
FLOAT_FORMAT(0x16, 0x082, "mulf"),
FLOAT_FORMAT(0x16, 0x002, "mulf/c"),
FLOAT_FORMAT(0x16, 0x182, "mulf/u"),
FLOAT_FORMAT(0x16, 0x102, "mulf/uc"),
FLOAT_FORMAT(0x16, 0x482, "mulf/s"),
FLOAT_FORMAT(0x16, 0x402, "mulf/sc"),
FLOAT_FORMAT(0x16, 0x582, "mulf/su"),
FLOAT_FORMAT(0x16, 0x502, "mulf/suc"),
FLOAT_FORMAT(0x16, 0x0a2, "mulg"),
FLOAT_FORMAT(0x16, 0x022, "mulg/c"),
FLOAT_FORMAT(0x16, 0x1a2, "mulg/u"),
FLOAT_FORMAT(0x16, 0x122, "mulg/uc"),
FLOAT_FORMAT(0x16, 0x4a2, "mulg/s"),
FLOAT_FORMAT(0x16, 0x422, "mulg/sc"),
FLOAT_FORMAT(0x16, 0x5a2, "mulg/su"),
FLOAT_FORMAT(0x16, 0x522, "mulg/suc"),
FLOAT_FORMAT(0x16, 0x081, "subf"),
FLOAT_FORMAT(0x16, 0x001, "subf/c"),
FLOAT_FORMAT(0x16, 0x181, "subf/u"),
FLOAT_FORMAT(0x16, 0x101, "subf/uc"),
FLOAT_FORMAT(0x16, 0x481, "subf/s"),
FLOAT_FORMAT(0x16, 0x401, "subf/sc"),
FLOAT_FORMAT(0x16, 0x581, "subf/su"),
FLOAT_FORMAT(0x16, 0x501, "subf/suc"),
FLOAT_FORMAT(0x16, 0x0a1, "subg"),
FLOAT_FORMAT(0x16, 0x021, "subg/c"),
FLOAT_FORMAT(0x16, 0x1a1, "subg/u"),
FLOAT_FORMAT(0x16, 0x121, "subg/uc"),
FLOAT_FORMAT(0x16, 0x4a1, "subg/s"),
FLOAT_FORMAT(0x16, 0x421, "subg/sc"),
FLOAT_FORMAT(0x16, 0x5a1, "subg/su"),
FLOAT_FORMAT(0x16, 0x521, "subg/suc"),
FLOAT_FORMAT(0x16, 0x0af, "cvtgq"),
FLOAT_FORMAT(0x16, 0x02f, "cvtgq/c"),
FLOAT_FORMAT(0x16, 0x1af, "cvtgq/v"),
FLOAT_FORMAT(0x16, 0x12f, "cvtgq/vc"),
FLOAT_FORMAT(0x16, 0x4af, "cvtgq/s"),
FLOAT_FORMAT(0x16, 0x42f, "cvtgq/sc"),
FLOAT_FORMAT(0x16, 0x5af, "cvtgq/sv"),
FLOAT_FORMAT(0x16, 0x52f, "cvtgq/svc"),

#if (VMS_ASMCODE)
				/* unprivileged codes */
PAL_FORMAT(0x00, 0x0080, "bpt"),
PAL_FORMAT(0x00, 0x0081, "bugchk"),
PAL_FORMAT(0x00, 0x0082, "chme"),
PAL_FORMAT(0x00, 0x0083, "chmk"),
PAL_FORMAT(0x00, 0x0084, "chms"),
PAL_FORMAT(0x00, 0x0085, "chmu"),
PAL_FORMAT(0x00, 0x0086, "imb"),
PAL_FORMAT(0x00, 0x0087, "insqhil"),
PAL_FORMAT(0x00, 0x0088, "insqtil"),
PAL_FORMAT(0x00, 0x0089, "insqhiq"),
PAL_FORMAT(0x00, 0x008a, "insqtiq"),
PAL_FORMAT(0x00, 0x008b, "insquel"),
PAL_FORMAT(0x00, 0x008c, "insqueq"),
PAL_FORMAT(0x00, 0x008d, "insquel/d"),
PAL_FORMAT(0x00, 0x008e, "insqueq/d"),
PAL_FORMAT(0x00, 0x008f, "prober"),
PAL_FORMAT(0x00, 0x0090, "probew"),
PAL_FORMAT(0x00, 0x0091, "rd_ps"),
PAL_FORMAT(0x00, 0x0092, "rei"),
PAL_FORMAT(0x00, 0x0093, "remqhil"),
PAL_FORMAT(0x00, 0x0095, "remqhiq"),
PAL_FORMAT(0x00, 0x009e, "read_unq"),
PAL_FORMAT(0x00, 0x0094, "remqtil"),
PAL_FORMAT(0x00, 0x0096, "remqtiq"),
PAL_FORMAT(0x00, 0x0097, "remquel"),
PAL_FORMAT(0x00, 0x0098, "remqueq"),
PAL_FORMAT(0x00, 0x0099, "remquel/d"),
PAL_FORMAT(0x00, 0x009a, "remqueq/d"),
PAL_FORMAT(0x00, 0x009b, "swasten"),
PAL_FORMAT(0x00, 0x009c, "wr_ps_sw"),
PAL_FORMAT(0x00, 0x009d, "rscc"),
PAL_FORMAT(0x00, 0x009f, "write_unq"),
PAL_FORMAT(0x00, 0x00a0, "amovrr"),
PAL_FORMAT(0x00, 0x00a1, "amovrm"),
PAL_FORMAT(0x00, 0x00a2, "insqhilr"),
PAL_FORMAT(0x00, 0x00a3, "insqtilr"),
PAL_FORMAT(0x00, 0x00a4, "insqhiqr"),
PAL_FORMAT(0x00, 0x00a5, "insqtiqr"),
PAL_FORMAT(0x00, 0x00a6, "remqhilr"),
PAL_FORMAT(0x00, 0x00a7, "remqtilr"),
PAL_FORMAT(0x00, 0x00a8, "remqhiqr"),
PAL_FORMAT(0x00, 0x00a9, "remqtiqr"),
PAL_FORMAT(0x00, 0x00aa, "gentrap"),
				/* privileged codes */
PAL_FORMAT(0x00, 0x0000, "halt"),
PAL_FORMAT(0x00, 0x0001, "cflush"),
PAL_FORMAT(0x00, 0x0002, "draina"),
PAL_FORMAT(0x00, 0x0003, "ldqp"),
PAL_FORMAT(0x00, 0x0004, "stqp"),
PAL_FORMAT(0x00, 0x0005, "swpctx"),
PAL_FORMAT(0x00, 0x0006, "mfpr_asn"),
PAL_FORMAT(0x00, 0x0007, "mtpr_asten"),
PAL_FORMAT(0x00, 0x0008, "mtpr_astsr"),
PAL_FORMAT(0x00, 0x000b, "mfpr_fen"),
PAL_FORMAT(0x00, 0x000c, "mtpr_fen"),
PAL_FORMAT(0x00, 0x000d, "mtpr_ipir"),
PAL_FORMAT(0x00, 0x000e, "mfpr_ipl"),
PAL_FORMAT(0x00, 0x000f, "mtpr_ipl"),
PAL_FORMAT(0x00, 0x0010, "mfpr_mces"),
PAL_FORMAT(0x00, 0x0011, "mtpr_mces"),
PAL_FORMAT(0x00, 0x0012, "mfpr_pcbb"),
PAL_FORMAT(0x00, 0x0013, "mfpr_prbr"),
PAL_FORMAT(0x00, 0x0014, "mtpr_prbr"),
PAL_FORMAT(0x00, 0x0015, "mfpr_ptbr"),
PAL_FORMAT(0x00, 0x0016, "mfpr_scbb"),
PAL_FORMAT(0x00, 0x0017, "mtpr_scbb"),
PAL_FORMAT(0x00, 0x0018, "mtpr_sirr"),
PAL_FORMAT(0x00, 0x0019, "mfpr_sisr"),
PAL_FORMAT(0x00, 0x001a, "mfpr_tbchk"),
PAL_FORMAT(0x00, 0x001b, "mtpr_tbia"),
PAL_FORMAT(0x00, 0x001c, "mtpr_tbiap"),
PAL_FORMAT(0x00, 0x001d, "mtpr_tbis"),
PAL_FORMAT(0x00, 0x001e, "mfpr_esp"),
PAL_FORMAT(0x00, 0x001f, "mtpr_esp"),
PAL_FORMAT(0x00, 0x0020, "mfpr_ssp"),
PAL_FORMAT(0x00, 0x0021, "mtpr_ssp"),
PAL_FORMAT(0x00, 0x0022, "mfpr_usp"),
PAL_FORMAT(0x00, 0x0023, "mtpr_usp"),
PAL_FORMAT(0x00, 0x0024, "mtpr_tbisd"),
PAL_FORMAT(0x00, 0x0025, "mtpr_tbisi"),
PAL_FORMAT(0x00, 0x0026, "mfpr_asten"),
PAL_FORMAT(0x00, 0x0027, "mfpr_astsr"),
PAL_FORMAT(0x00, 0x0029, "mfpr_vptb"),
PAL_FORMAT(0x00, 0x002a, "mtpr_vptb"),
PAL_FORMAT(0x00, 0x002b, "mtpr_perfmon"),
PAL_FORMAT(0x00, 0x002e, "mtpr_datfx"),
PAL_FORMAT(0x00, 0x003f, "mfpr_whami"),

#elif OSF_ASMCODE
				/* unprivileged codes */
PAL_FORMAT(0x00, 0x0080, "bpt"),
PAL_FORMAT(0x00, 0x0081, "bugchk"),
PAL_FORMAT(0x00, 0x0083, "callsys"),
PAL_FORMAT(0x00, 0x0086, "imb"),
PAL_FORMAT(0x00, 0x009f, "wrunique"),
PAL_FORMAT(0x00, 0x009e, "rdunique"),
PAL_FORMAT(0x00, 0x00aa, "gentrap"),
				/* privileged codes */
PAL_FORMAT(0x00, 0x0000, "halt"),
PAL_FORMAT(0x00, 0x0032, "rdval"),
PAL_FORMAT(0x00, 0x0030, "swpctx"),
PAL_FORMAT(0x00, 0x003c, "whami"),
PAL_FORMAT(0x00, 0x0037, "wrkgp"),
PAL_FORMAT(0x00, 0x002d, "wrvptptr"),
PAL_FORMAT(0x00, 0x0036, "rdps"),
PAL_FORMAT(0x00, 0x003d, "retsys"),
PAL_FORMAT(0x00, 0x0035, "swpipl"),
PAL_FORMAT(0x00, 0x0034, "wrent"),
PAL_FORMAT(0x00, 0x0038, "wrusp"),
PAL_FORMAT(0x00, 0x003a, "rdusp"),
PAL_FORMAT(0x00, 0x003f, "rti"),
PAL_FORMAT(0x00, 0x0033, "tbi"),
PAL_FORMAT(0x00, 0x002b, "wrfen"),
PAL_FORMAT(0x00, 0x0031, "wrval"),

#endif /* OSF_ASMCODE */

/* This is the old set we had before:

	PAL_FORMAT(0x00, 0x0000, "halt"),
	PAL_FORMAT(0x00, 0x0080, "bpt"),
	PAL_FORMAT(0x00, 0x00aa, "gentrap"),
	PAL_FORMAT(0x00, 0x009f, "wrunique"),
	PAL_FORMAT(0x00, 0x0081, "bugchk"),
	PAL_FORMAT(0x00, 0x0086, "imb"),
	PAL_FORMAT(0x00, 0x0083, "callsys"),
	PAL_FORMAT(0x00, 0x009e, "rdunique"),
*/


	0
};
#endif

