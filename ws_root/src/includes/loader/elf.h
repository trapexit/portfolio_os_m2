#ifndef __LOADER_ELF_H
#define __LOADER_ELF_H


/******************************************************************************
**
**  @(#) elf.h 96/05/31 1.8
**
******************************************************************************/


#ifndef __LOADER_ELFTYPES_H
#include <loader/elftypes.h>
#endif

/*
 * Elf header
 */
#define EI_NIDENT	16

typedef struct Elf32_Ehdr {
	unsigned char	e_ident[EI_NIDENT];
	Elf32_Half	e_type;			/* ET_XXX */
	Elf32_Half	e_machine;		/* EM_XXX */
	Elf32_Word	e_version;		/* EV_XXX */
	Elf32_Addr	e_entry;
	Elf32_Off	e_phoff;
	Elf32_Off	e_shoff;
	Elf32_Word	e_flags;		/* EF_XXX */
	Elf32_Half	e_ehsize;
	Elf32_Half	e_phentsize;
	Elf32_Half	e_phnum;
	Elf32_Half	e_shentsize;
	Elf32_Half	e_shnum;
	Elf32_Half	e_shstrndx;
} Elf32_Ehdr;

#define EHDRSZ sizeof(Elf32_Ehdr)

/*
 * e_ident[] values
 */
#define EI_MAG0		0
#define EI_MAG1		1
#define EI_MAG2		2
#define EI_MAG3		3
#define EI_CLASS	4
#define EI_DATA		5
#define EI_VERSION	6
#define EI_PAD		7

#define ELFMAG0		0x7f
#define ELFMAG1		'E'
#define ELFMAG2		'L'
#define ELFMAG3		'F'
#define ELFMAG		"\177ELF"
#define SELFMAG		4

/*
 * EI_CLASS
 */
#define ELFCLASSNONE	0
#define ELFCLASS32	1
#define ELFCLASS64	2

/*
 * EI_DATA
 */
#define ELFDATANONE	0
#define ELFDATA2LSB	1
#define ELFDATA2MSB	2

/*
 * e_type
 */
#define ET_NONE		0
#define ET_REL		1
#define ET_EXEC		2
#define ET_DYN		3
#define ET_CORE		4
#define ET_LOPROC	0xff00
#define ET_HIPROC	0xffff

/*
 * e_machine
 */
#define EM_NONE		0		/* No machine */
#define EM_M32		1		/* AT&T WE 32100 */
#define EM_SPARC	2		/* SPARC */
#define EM_386		3		/* Intel 80386 */
#define EM_68K		4		/* Motorola 68000 */
#define EM_88K		5		/* Motorola 88000 */
#define EM_860		7		/* Intel 80860 */
#define EM_MIPS		8		/* MIPS RS3000 Big-Endian */
#define EM_MIPS_RS4_BE 10		/* MIPS RS4000 Big-Endian */
#define EM_PPC		17		/* PowerPC */
#define EM_PPC_EABI	20		/* PowerPC EABI */

/*
 * e_version and EI_VERSION
 */
#define EV_NONE		0
#define EV_CURRENT	1

/*
 * Special section indexes
 */
#define SHN_UNDEF	0
#define SHN_LORESERVE	0xff00
#define SHN_LOPROC	0xff00
#define SHN_HIPROC	0xff1f
#define SHN_ABS		0xfff1
#define SHN_COMMON	0xfff2
#define SHN_HIRESERVE	0xffff

/*
 * Section header
 */
typedef struct Elf32_Shdr {
	Elf32_Word	sh_name;
	Elf32_Word	sh_type;	/* SHT_... */
	Elf32_Word	sh_flags;	/* SHF_... */
	Elf32_Addr	sh_addr;
	Elf32_Off	sh_offset;
	Elf32_Word	sh_size;
	Elf32_Word	sh_link;
	Elf32_Word	sh_info;
	Elf32_Word	sh_addralign;
	Elf32_Word	sh_entsize;
} Elf32_Shdr;

#define SHDRSZ sizeof(Elf32_Shdr)

/*
 * sh_type
 */
#define SHT_NULL	0
#define SHT_PROGBITS	1
#define SHT_SYMTAB	2
#define SHT_STRTAB	3
#define SHT_RELA	4
#define SHT_HASH	5
#define SHT_DYNAMIC	6
#define SHT_NOTE	7
#define SHT_NOBITS	8
#define SHT_REL		9
#define SHT_SHLIB	10
#define SHT_DYNSYM	11
#define SHT_LOPROC	0x70000000
#define SHT_HIPROC	0x7fffffff
#define SHT_LOUSER	0x80000000
#define SHT_HIUSER	0xffffffff

/*
 * sh_flags
 */
#define SHF_WRITE	0x1
#define SHF_ALLOC	0x2
#define SHF_EXECINSTR	0x4
#define SHF_COMPRESS	0x8
#define SHF_MASKPROC	0xf0000000

/*
 * Symbol table
 */
typedef struct Elf32_Sym {
	Elf32_Word	st_name;
	Elf32_Addr	st_value;
	Elf32_Word	st_size;
	unsigned char	st_info;	/* ELF_32_ST_XXX */
	unsigned char	st_other;
	Elf32_Half	st_shndx;	/* SHN_XXX */
} Elf32_Sym;

#define STN_UNDEF	0

#define STB_LOCAL	0
#define STB_GLOBAL	1
#define STB_WEAK	2
#define STB_LOPROC	13
#define STB_HIPROC	15

#define STT_NOTYPE	0
#define STT_OBJECT	1
#define STT_FUNC	2
#define STT_SECTION	3
#define STT_FILE	4
#define STT_LOPROC	13
#define STT_HIPROC	15

#define ELF32_ST_BIND(info)		((info) >> 4)
#define ELF32_ST_TYPE(info)		((info) & 0xf)
#define ELF32_ST_INFO(bind,type)	(((bind)<<4)+((type)&0xf))

/*
 * Relocation
 */
typedef struct Elf32_Rel {
	Elf32_Addr	r_offset;
	Elf32_Word	r_info;		/* ELF32_R_XXX */
} Elf32_Rel;

typedef struct Elf32_Rela {
	Elf32_Addr	r_offset;	/* r_offset better be unsigned -
					   otherwise we might break priv */
	Elf32_Word	r_info;		/* ELF32_R_XXX */
	Elf32_Sword	r_addend;
} Elf32_Rela;

#define ELF32_R_SYM(info)	((info)>>8)
#define ELF32_R_TYPE(info)	((unsigned char)(info))
#define ELF32_R_INFO(sym,type)	(((sym)<<8)+(unsigned char)(type))

/*
 * Program header
 */
typedef struct Elf32_Phdr {
	Elf32_Word	p_type;		/* PT_XXX */
	Elf32_Off	p_offset;
	Elf32_Addr	p_vaddr;
	Elf32_Addr	p_paddr;
	Elf32_Word	p_filesz;
	Elf32_Word	p_memsz;
	Elf32_Word	p_flags;	/* PF_XXX */
	Elf32_Word	p_align;
} Elf32_Phdr;

#define PHDRSZ sizeof(Elf32_Phdr)

/*
 * p_type
 */
#define PT_NULL		0
#define PT_LOAD		1
#define PT_DYNAMIC	2
#define PT_INTERP	3
#define PT_NOTE		4
#define PT_SHLIB	5
#define PT_PHDR		6
#define PT_LOPROC	0x70000000
#define PT_HIPROC	0x7fffffff

/*
 * p_flags
 */
#define PF_X		0x1
#define PF_W		0x2
#define PF_R		0x4
#define PF_C		0x10       /* compressed */
#define PF_MASKPROC	0xf0000000

#ifndef __LOADER_ELF_PPC_H
#include <loader/elf_ppc.h>
#endif


#endif /* __LOADER_ELF_H */
