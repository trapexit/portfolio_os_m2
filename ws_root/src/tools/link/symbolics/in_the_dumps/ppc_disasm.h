/*  @(#) ppc_disasm.h 96/07/25 1.9 */

/* Copyright 1994 Free Software Foundation, Inc.
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

 *  PROMINENT NOTICE: This file was substantially modified by
 *  The 3DO Company, in 1995.  For more information, contact
 *  your 3DO DTS representative at 415 261 3400, or send email to
 *  'support@3do.com'.
 */

#ifndef __DISPPC_H
#define __DISPPC_H

#ifdef _SUN4
#include "loaderty.h"	//dawn
#elif defined(macintosh)
#include <types.h>		//dawn
#include "compiler.h"	//dawn
#else
typedef unsigned long uint32;
typedef long int32;
typedef enum { false, true } bool;
typedef bool Boolean;
#endif


#ifndef TRUE
#define TRUE (Boolean)1
#endif

#ifndef FALSE
#define FALSE (Boolean)0
#endif


/* Current value of CPU registers. This used by the disassembler to
 * pre-calculate register contents, effective addresses, etc.
 *
 * (9/8/95 dkk) Rearranged this structure and added some unused fields so
 * that it matches the format we already used to keep the registers.
 */
typedef struct PPCRegisters
{
	uint32 ppcr_GPRs[32];
	uint32 ppcr_CR;
	uint32 ppcr_XER;	//	unused by disassembler
	uint32 ppcr_LR;
	uint32 ppcr_CTR;
	uint32 ppcr_PC;		//	unused by disassembler
	uint32 ppcr_SRR0;
} PPCRegisters;

typedef void (*MakeAddrFunc)(uint32 addr, char *result);


Boolean DisasmPPC(
	uint32 instruction,				/* instruction to disassemble     */
	uint32 instrAddr,				/* address of instruction   */
	const PPCRegisters *registers,  /* current reg values, or NULL    */
	MakeAddrFunc addrFunc,			/* convert an address to a string */
	char *result,					/* where result is deposited */
	char *comment);					/* for a helpful comment    */

/* simplified interface for 3DODebug */
void dis602(uint32** instructions, char* buffer);

// default Address to string conversion (should do lookups, etc. in =future)
extern void AddrToStr(uint32 addr, char* result);



#endif /* __DISPPC_H */

