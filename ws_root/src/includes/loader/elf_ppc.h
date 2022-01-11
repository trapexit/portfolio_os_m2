#ifndef __LOADER_ELF_PPC_H
#define __LOADER_ELF_PPC_H


/******************************************************************************
**
**  @(#) elf_ppc.h 96/02/20 1.9
**
**  Defines flags used by PowerPC EABI.
**
******************************************************************************/


/**************	Exported data, types and macros	*****************/

/*
 * e_machine
 */
#define EM_PPC		17

/*
 * e_flags
 */
#define EF_PPC_EMB	0x80000000

/*
 * Relocation types
 */
#define R_PPC_NONE	0
#define R_PPC_ADDR32	1
#define R_PPC_ADDR24	2
#define R_PPC_ADDR16	3
#define R_PPC_ADDR16_LO	4
#define R_PPC_ADDR16_HI	5
#define R_PPC_ADDR16_HA	6
#define R_PPC_ADDR14	7
#define R_PPC_ADDR14_BRTAKEN	8
#define R_PPC_ADDR14_BRNTAKEN	9
#define R_PPC_REL24	10
#define R_PPC_REL14	11
#define R_PPC_REL14_BRTAKEN	12
#define R_PPC_REL14_BRNTAKEN	13
#define R_PPC_GOT16	14
#define R_PPC_GOT16_LO	15
#define R_PPC_GOT16_HI	16
#define R_PPC_GOT16_HA	17
#define R_PPC_PLT24	18
#define R_PPC_COPY	19
#define R_PPC_JMP_SLOT	21
#define R_PPC_RELATIVE	22
#define R_PPC_LOCAL24PC	23
#define R_PPC_UADDR32	24
#define R_PPC_UADDR16	25
#define R_PPC_REL32	26
#define R_PPC_PLT32	27
#define R_PPC_PLTREL32	28
#define R_PPC_PLT16_LO	29
#define R_PPC_PLT16_HI	30
#define R_PPC_PLT16_HA	31
#define R_PPC_SDAREL	32

#define R_PPC_EMB_NADDR32	101
#define R_PPC_EMB_NADDR16	102
#define R_PPC_EMB_NADDR16_LO	103
#define R_PPC_EMB_NADDR16_HI	104
#define R_PPC_EMB_NADDR16_HA	105
#define R_PPC_EMB_SDAl16	106
#define R_PPC_EMB_SDA2l16	107
#define R_PPC_EMB_SDA2REL	108
#define R_PPC_EMB_SDA21		109
#define R_PPC_EMB_MRKREF	110
#define R_PPC_EMB_RELSEC16	111
#define R_PPC_EMB_RELST_LO	112
#define R_PPC_EMB_RELST_HI	113
#define R_PPC_EMB_RELST_HA	114
#define R_PPC_EMB_BIT_FLD	115
#define R_PPC_EMB_RELSDA	116

/* Dawn's extensions */

#define R_PPC_BASEREL		0xa0
#define R_PPC_LAST_BASEREL	0xbf

#define R_PPC_BASEREL16 		(R_PPC_BASEREL | R_PPC_ADDR16)
#define R_PPC_BASEREL16_LO 		(R_PPC_BASEREL | R_PPC_ADDR16_LO)
#define R_PPC_BASEREL16_HI 		(R_PPC_BASEREL | R_PPC_ADDR16_HI)
#define R_PPC_BASEREL16_HA 		(R_PPC_BASEREL | R_PPC_ADDR16_HA)
#define R_PPC_BASEREL24			(R_PPC_BASEREL | R_PPC_REL24)
#define R_PPC_BASEREL14 		(R_PPC_BASEREL | R_PPC_REL14)
#define R_PPC_BASEREL14_BRTAKEN 	(R_PPC_BASEREL | R_PPC_REL14_BRTAKEN)
#define R_PPC_BASEREL14_BRNTAKEN 	(R_PPC_BASEREL | R_PPC_REL14_BRNTAKEN)
#define R_PPC_BASEREL32 		(R_PPC_BASEREL | R_PPC_REL32)
#define R_PPC_BASERELATIVE 		(R_PPC_BASEREL | R_PPC_RELATIVE)
#define R_PPC_UBASEREL32 		(R_PPC_BASEREL | R_PPC_UADDR32)
#define R_PPC_UBASEREL16 		(R_PPC_BASEREL | R_PPC_UADDR16)

/* The import's flavors	*/
#define R_PPC_IMPREL			0xc0
#define	R_PPC_LAST_IMPREL		0xdf

#define R_PPC_IMPADDR32 	        (R_PPC_IMPREL | R_PPC_ADDR32)
#define R_PPC_IMPADDR24 		(R_PPC_IMPREL | R_PPC_ADDR24)
#define R_PPC_IMPADDR16 		(R_PPC_IMPREL | R_PPC_ADDR16)
#define R_PPC_IMPADDR16_LO 		(R_PPC_IMPREL | R_PPC_ADDR16_LO)
#define R_PPC_IMPADDR16_HI 		(R_PPC_IMPREL | R_PPC_ADDR16_HI)
#define R_PPC_IMPADDR16_HA 		(R_PPC_IMPREL | R_PPC_ADDR16_HA)
#define R_PPC_IMPADDR14 		(R_PPC_IMPREL | R_PPC_ADDR14)
#define R_PPC_IMPADDR14_BRTAKEN 	(R_PPC_IMPREL | R_PPC_ADDR14_BRTAKEN)
#define R_PPC_IMPADDR14_BRNTAKEN 	(R_PPC_IMPREL | R_PPC_ADDR14_BRNTAKEN)
#define R_PPC_UIMPADDR32 		(R_PPC_IMPREL | R_PPC_UADDR32)
#define R_PPC_UIMPADDR16 		(R_PPC_IMPREL | R_PPC_UADDR16)
#define R_PPC_IMPREL24 			(R_PPC_IMPREL | R_PPC_REL24)
#define R_PPC_IMPREL14 			(R_PPC_IMPREL | R_PPC_REL14)
#define R_PPC_IMPREL14_BRTAKEN 	        (R_PPC_IMPREL | R_PPC_REL14_BRTAKEN)
#define R_PPC_IMPREL14_BRNTAKEN 	(R_PPC_IMPREL | R_PPC_REL14_BRNTAKEN)
#define R_PPC_IMPREL32 	        	(R_PPC_IMPREL | R_PPC_REL32)
#define R_PPC_IMPRELATIVE         	(R_PPC_IMPREL | R_PPC_RELATIVE)

#define IS_BASEREL_RELOC(x)		(((x) >= R_PPC_BASEREL) && ((x) <= R_PPC_LAST_BASEREL))
#define IS_IMPREL_RELOC(x)		(((x) >= R_PPC_IMPREL) && ((x) <= R_PPC_LAST_IMPREL))

#define PPC_HA(x)   ((((x)>>16) + (((x) & 0x8000) ? 1 : 0)) & 0xFFFF)
#define PPC_HI(x)   (((x)>>16) & 0xFFFF)
#define PPC_LO(x)   ((x) & 0xFFFF)


#endif /* __LOADER_ELF_PPC_H */
