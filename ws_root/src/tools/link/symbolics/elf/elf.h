/*  @(#) elf.h 96/07/25 1.16 */

#ifndef __ELF_H__
#define __ELF_H__


typedef unsigned short  Elf32_Half;
typedef unsigned long   Elf32_Off;
typedef long            Elf32_Sword;
typedef unsigned long   Elf32_Word;
typedef unsigned long   Elf32_Addr;

/* ELF header */
#define EI_NIDENT       16
typedef struct Elf32_Ehdr {
    unsigned char e_ident[EI_NIDENT];   /* ident bytes */
    Elf32_Half  e_type;                 /* file type */
    Elf32_Half  e_machine;              /* target machine */
    Elf32_Word  e_version;              /* file version */
    Elf32_Addr  e_entry;                /* start address */
    Elf32_Off   e_phoff;                /* phdr file offset */
    Elf32_Off   e_shoff;                /* shdr file offset */
    Elf32_Word  e_flags;                /* file flags */
    Elf32_Half  e_ehsize;               /* sizeof ehdr */
    Elf32_Half  e_phentsize;            /* sizeof phdr */
    Elf32_Half  e_phnum;                /* number phdrs */
    Elf32_Half  e_shentsize;            /* sizeof shdr */
    Elf32_Half  e_shnum;                /* number shdrs */
    Elf32_Half  e_shstrndx;             /* shdr string index */
    } Elf32_Ehdr;

    /* ---- index to e_ident[] ---- */
    #define EI_MAG0     0
    #define EI_MAG1     1
    #define EI_MAG2     2
    #define EI_MAG3     3
    #define EI_CLASS    4
    #define EI_DATA     5
    #define EI_VERSION  6
    #define EI_PAD      7

    /* ---- magic ---- */
    #define ELFMAG0             0x7f
    #define ELFMAG1             'E'
    #define ELFMAG2             'L'
    #define ELFMAG3             'F'
    #define ELFMAG              "\177ELF"
    #define ELFMAGLEN           4

    /* ---- e_type ---- */
    #define ET_NONE     0
    #define ET_REL      1
    #define ET_EXEC     2
    #define ET_DYN      3
    #define ET_CORE     4
    #define ET_LOPROC   0xff00
    #define ET_HIPROC   0xffff

    /* ---- e_machine ---- */
    #define EM_NONE     0
    #define EM_M32      1               /* AT&T WE 32100 */
    #define EM_SPARC    2               /* Sun SPARC */
    #define EM_386      3               /* Intel 80386 */
    #define EM_68K      4               /* Motorola 68000 */
    #define EM_88K      5               /* Motorola 88000 */
    #define EM_486      6               /* Intel 80486 */
    #define EM_860      7               /* Intel i860 */
    #define EM_PPC      0x11            /* EABI */

    /* ---- EI_CLASS ---- */
    #define ELFCLASSNONE        0
    #define ELFCLASS32          1
    #define ELFCLASS64          2

    /* ---- EI_DATA ---- */
    #define ELFDATANONE 0
    #define ELFDATA2LSB 1
    #define ELFDATA2MSB 2

    /* ---- EI_VERSION ---- */
    #define EV_NONE     0
    #define EV_CURRENT  1
    #define EV_NUM      2


/* ======== Program header ======== */
typedef struct Elf32_Phdr {
    Elf32_Word  p_type;         /* entry type */
    Elf32_Off   p_offset;       /* file offset */
    Elf32_Addr  p_vaddr;        /* virtual address */
    Elf32_Addr  p_paddr;        /* physical address */
    Elf32_Word  p_filesz;       /* file size */
    Elf32_Word  p_memsz;        /* memory size */
    Elf32_Word  p_flags;        /* entry flags */
    Elf32_Word  p_align;        /* memory/file alignment */
    } Elf32_Phdr;

    /* ---- p_type ---- */
    #define PT_NULL     0
    #define PT_LOAD     1
    #define PT_DYNAMIC  2
    #define PT_INTERP   3
    #define PT_NOTE     4
    #define PT_SHLIB    5
    #define PT_PHDR     6
    #define PT_LOPROC   0x70000000
    #define PT_HIPROC   0x7fffffff

    /* ---- Segment Flag p_flags ---- */
    #define PF_X                        0x1     /* execute */
    #define PF_W                        0x2     /* write */
    #define PF_R                        0x4     /* read */
    #define PF_C                        0x8     /* compressed */
    #define PF_MASKPROC                 0xf0000000
    #define PF_MASK                     0x0000007f


/* ======== Section header ========*/
typedef struct Elf32_Shdr {
    Elf32_Word  sh_name;        /* section name */
    Elf32_Word  sh_type;        /* SHT_* */
    Elf32_Word  sh_flags;       /* SHF_* */
    Elf32_Addr  sh_addr;        /* virtual address */
    Elf32_Off   sh_offset;      /* file offset */
    Elf32_Word  sh_size;        /* section size */
    Elf32_Word  sh_link;        /* misc info */
    Elf32_Word  sh_info;        /* misc info */
    Elf32_Word  sh_addralign;   /* memory alignment */
    Elf32_Word  sh_entsize;     /* entry size if table */
    } Elf32_Shdr;

    /* ---- sh_type ---- */
    #define SHT_NULL            0
    #define SHT_PROGBITS        1
    #define SHT_SYMTAB          2
    #define SHT_STRTAB          3
    #define SHT_RELA            4
    #define SHT_HASH            5
    #define SHT_DYNAMIC         6
    #define SHT_NOTE            7
    #define SHT_NOBITS          8
    #define SHT_REL             9
    #define SHT_SHLIB           10
    #define SHT_DYNSYM          11
    #define SHT_LOUSER          0x80000000
    #define SHT_HIUSER          0xffffffff
    #define SHT_LOPROC          0x70000000
    #define SHT_HIPROC          0x7fffffff

    /* ---- sh_flags ---- */
    #define SHF_WRITE           0x1
    #define SHF_ALLOC           0x2
    #define SHF_EXECINSTR       0x4
    #define SHF_COMPRESS        0x8
    #define SHF_MASKPROC        0xf0000000      /* processor specific values */

    /* ---- special section number ---- */
    #define SHN_UNDEF           0
    #define SHN_LORESERVE       0xff00
    #define SHN_ABS             0xfff1
    #define SHN_COMMON          0xfff2
    #define SHN_HIRESERVE       0xffff
    #define SHN_LOPROC          0xff00
    #define SHN_HIPROC          0xff1f


/* ======== Symbol table Entry ======== */
typedef struct Elf32_Sym {
    Elf32_Word st_name;
    Elf32_Addr st_value;
    Elf32_Word st_size;
    unsigned char st_info;  /* bind, type, st_info */
    unsigned char st_other;
    Elf32_Half  st_shndx;   /* SHN_* */
    } Elf32_Sym;

    /* ---- st_name value ---- */
    #define STN_UNDEF   0

    /* ---- The macros to insert and extract st_info. ---- */
    #define ELF32_ST_INFO(bind,type)    (((bind)<<4)+((type)&0xf))
    #define ELF32_ST_BIND(info)         ((info) >> 4)
    #define ELF32_ST_TYPE(info)         ((info) & 0xf)

    /* ---- ELF32_ST_BIND(info) ---- */
    #define STB_LOCAL   0
    #define STB_GLOBAL  1
    #define STB_WEAK    2
    #define STB_LOPROC  13
    #define STB_HIPROC  15

    /* ---- ELF32_ST_TYPE(info) ---- */
    #define STT_NOTYPE  0
    #define STT_OBJECT  1
    #define STT_FUNC    2
    #define STT_SECTION 3
    #define STT_FILE    4
    #define STT_LOPROC  13
    #define STT_HIPROC  15

/* ======== Line Entry ======= */
typedef struct Elf32_Lhdr {
    Elf32_Word lh_size; //size of line number entries for this module
    Elf32_Word lh_svaddr; // starting virtual address of module
    } Elf32_Lhdr;
#define SIZEOF_LHDR 8
typedef struct Elf32_line {
    Elf32_Word ln_num; //line number
    Elf32_Half ln_bogus; // who knows
    Elf32_Word ln_addr; // address
    } Elf32_Line;
#define SIZEOF_LINE 10
#define LN_NUM_OFF 0
#define LN_ADDR_OFF 6

/* ======== Relocation Entry ======= */
typedef struct Elf32_Rel {
    Elf32_Addr  r_offset;
    Elf32_Word  r_info;         /* symbol index or relocation type */
    } Elf32_Rel;

typedef struct Elf32_Rela {
    Elf32_Addr  r_offset;
    Elf32_Word  r_info;         /* symbol index or relocation type */
    Elf32_Sword r_addend;
    } Elf32_Rela;

    /* ---- macros to insert and extract r_info field ---- */
    #define ELF32_R_INFO(sym,type)      (((sym)<<8)+(unsigned char)(type))
    #define ELF32_R_SYM(info)   ((info)>>8)
    #define ELF32_R_IMPINFO(lib,sym,type) ELF32_R_INFO(((lib)<<16)+(unsigned short)(sym),type)
    #define ELF32_R_IMPSYM(info)   ((unsigned short)(ELF32_R_SYM(info)))
    #define ELF32_R_IMPLIB(info)   (ELF32_R_SYM(info)>>16)
    #define ELF32_R_TYPE(info)  ((unsigned char)(info))

    /* ---- ELF32_R_TYPE(r_info) values for PPC: R_PPC_* ---- */
    #define R_PPC_NONE 				0
    #define R_PPC_ADDR32			1
    #define R_PPC_ADDR24 			2
    #define R_PPC_ADDR16 			3
    #define R_PPC_ADDR16_LO 		4
    #define R_PPC_ADDR16_HI 		5
    #define R_PPC_ADDR16_HA 		6
    #define R_PPC_ADDR14 			7
    #define R_PPC_ADDR14_BRTAKEN 	8
    #define R_PPC_ADDR14_BRNTAKEN 	9
    #define R_PPC_REL24				10
    #define R_PPC_REL14 			11
    #define R_PPC_REL14_BRTAKEN 	12
    #define R_PPC_REL14_BRNTAKEN 	13
    #define R_PPC_REL32 			26
    #define R_PPC_RELATIVE 			22
    #define R_PPC_UADDR32 			24
    #define R_PPC_UADDR16 			25
	//3doelf doesn't support
    #define R_PPC_GOT16 			14
    #define R_PPC_GOT16_LO 			15
    #define R_PPC_GOT16_HI 			16
    #define R_PPC_GOT16_HA 			17
    #define R_PPC_PLTREL24 			18
    #define R_PPC_COPY 				19
    #define R_PPC_GLOB_DAT 			20
    #define R_PPC_JMP_SLOT 			21
    #define R_PPC_LOCAL24PC 		23
    #define R_PPC_PLT32 			27
    #define R_PPC_PLTREL32 			28
    #define R_PPC_PLT16_LO 			29
    #define R_PPC_PLT16_HI 			30
    #define R_PPC_PLT16_HA 			31
    #define R_PPC_SDAREL16 			32
    #define R_PPC_SECTOFF 			33
    #define R_PPC_SECTOFF_LO 		34
    #define R_PPC_SECTOFF_HI 		35
    #define R_PPC_SECTOFF_HA 		36

    //3do extras
	//base relative relocs - use section number in SYMIDX(r) 

	#define R_PPC_BASEREL 0xA0
	#define R_PPC_LAST_BASEREL 0xBF
	#define ISA_BASEREL(x) (x >= R_PPC_BASEREL && x <= R_PPC_LAST_BASEREL)
    #define R_PPC_BASEREL16 			R_PPC_BASEREL | R_PPC_ADDR16 	
    #define R_PPC_BASEREL16_LO 			R_PPC_BASEREL | R_PPC_ADDR16_LO 		
    #define R_PPC_BASEREL16_HI 			R_PPC_BASEREL | R_PPC_ADDR16_HI 		
    #define R_PPC_BASEREL16_HA 			R_PPC_BASEREL | R_PPC_ADDR16_HA 	
    #define R_PPC_BASEREL24				R_PPC_BASEREL | R_PPC_REL24			
    #define R_PPC_BASEREL14 			R_PPC_BASEREL | R_PPC_REL14 			
    #define R_PPC_BASEREL14_BRTAKEN 	R_PPC_BASEREL | R_PPC_REL14_BRTAKEN 
    #define R_PPC_BASEREL14_BRNTAKEN 	R_PPC_BASEREL | R_PPC_REL14_BRNTAKEN 
    #define R_PPC_BASEREL32 			R_PPC_BASEREL | R_PPC_REL32 		
    #define R_PPC_BASERELATIVE 			R_PPC_BASEREL | R_PPC_RELATIVE 	
    #define R_PPC_UBASEREL32 			R_PPC_BASEREL | R_PPC_UADDR32 		
    #define R_PPC_UBASEREL16 			R_PPC_BASEREL | R_PPC_UADDR16 	

	//import relative relocs - use lib_ord & sym_ord in SYMIDX(r) 

	#define R_PPC_IMPREL 0xC0
	#define R_PPC_LAST_IMPREL 0xDF
	#define ISA_IMPREL(x) (x >= R_PPC_IMPREL && x <= R_PPC_LAST_IMPREL)
    #define R_PPC_IMPADDR32				R_PPC_IMPREL | R_PPC_ADDR32			
    #define R_PPC_IMPADDR24 			R_PPC_IMPREL | R_PPC_ADDR24 		
    #define R_PPC_IMPADDR16 			R_PPC_IMPREL | R_PPC_ADDR16 	
    #define R_PPC_IMPADDR16_LO 			R_PPC_IMPREL | R_PPC_ADDR16_LO 		
    #define R_PPC_IMPADDR16_HI 			R_PPC_IMPREL | R_PPC_ADDR16_HI 		
    #define R_PPC_IMPADDR16_HA 			R_PPC_IMPREL | R_PPC_ADDR16_HA 	
    #define R_PPC_IMPADDR14 			R_PPC_IMPREL | R_PPC_ADDR14 			
    #define R_PPC_IMPADDR14_BRTAKEN 	R_PPC_IMPREL | R_PPC_ADDR14_BRTAKEN 	
    #define R_PPC_IMPADDR14_BRNTAKEN 	R_PPC_IMPREL | R_PPC_ADDR14_BRNTAKEN 
    #define R_PPC_UIMPADDR32 			R_PPC_IMPREL | R_PPC_UADDR32 		
    #define R_PPC_UIMPADDR16 			R_PPC_IMPREL | R_PPC_UADDR16 	
    #define R_PPC_IMPREL24				R_PPC_IMPREL | R_PPC_REL24			
    #define R_PPC_IMPREL14 				R_PPC_IMPREL | R_PPC_REL14 			
    #define R_PPC_IMPREL14_BRTAKEN 		R_PPC_IMPREL | R_PPC_REL14_BRTAKEN 
    #define R_PPC_IMPREL14_BRNTAKEN 	R_PPC_IMPREL | R_PPC_REL14_BRNTAKEN 
    #define R_PPC_IMPREL32 				R_PPC_IMPREL | R_PPC_REL32 		
    #define R_PPC_IMPRELATIVE 			R_PPC_IMPREL | R_PPC_RELATIVE 	

/* ======== DT entry ======= */
typedef struct Elf32_Dyn {
    Elf32_Sword d_tag;  /* DT_* tag */
    union {
        Elf32_Word      d_val;
        Elf32_Addr      d_ptr;
        Elf32_Off       d_off;
        } d_un;
    } Elf32_Dyn;

    /* ---- d_tag values : DT_* tag ---- */
    #define DT_NULL     0       /* last entry in list */
    #define DT_NEEDED   1       /* list of needed objects */
    #define DT_PLTRELSZ 2       /* size of relocations for the PLT */
    #define DT_PLTGOT   3       /* addr for PLT */
    #define DT_HASH     4       /* hash table */
    #define DT_STRTAB   5       /* string table */
    #define DT_SYMTAB   6       /* symbol table */
    #define DT_RELA     7       /* addr of rel entries */
    #define DT_RELASZ   8       /* size of rel table */
    #define DT_RELAENT  9       /* base size of rel entry */
    #define DT_STRSZ    10      /* size of string table */
    #define DT_SYMENT   11      /* size of symbol table entry */
    #define DT_INIT     12      /* _init addr */
    #define DT_FINI     13      /* _fini addr */
    #define DT_SONAME   14      /* name of this shared object */
    #define DT_RPATH    15      /* run-time search path */
    #define DT_SYMBOLIC 16      /* shared object linked -Bsymbolic */
    #define DT_REL      17      /* addr of rel entries */
    #define DT_RELSZ    18      /* size of rel table */
    #define DT_RELENT   19      /* base size of relocation entry */
    #define DT_PLTREL   20      /* rel type for PLT entry */
    #define DT_DEBUG    21      /* pointer to r_debug structure */
    #define DT_TEXTREL  22      /* text rel for this object */
    #define DT_JMPREL   23      /* pointer to the PLT rel entries */
    #define DT_LOPROC   0x70000000
    #define DT_HIPROC   0x7fffffff

#define ELF_HASH_SIZE 67        //default.
extern void elf_set_hash_size(unsigned long size);
extern unsigned long elf_hash(const char *name);

#endif  /* __ELF_H__ */

