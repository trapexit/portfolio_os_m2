/* BFD back-end data structures for ELF files.
   Copyright (C) 1992, 1993 Free Software Foundation, Inc.
   Written by Cygnus Support.

This file is part of BFD, the Binary File Descriptor library.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef _LIBELF_H_
#define _LIBELF_H_ 1

#include "elf/common.h"
#include "elf/internal.h"
#include "elf/external.h"
#include "bfdlink.h"

/* If size isn't specified as 64 or 32, NAME macro should fail.  */
#ifndef NAME
#if ARCH_SIZE==64
#define NAME(x,y) CAT4(x,64,_,y)
#endif
#if ARCH_SIZE==32
#define NAME(x,y) CAT4(x,32,_,y)
#endif
#endif

#ifndef NAME
#define NAME(x,y) CAT4(x,NOSIZE,_,y)
#endif

#define ElfNAME(X)	NAME(Elf,X)
#define elfNAME(X)	NAME(elf,X)

/* Information held for an ELF symbol.  The first field is the
   corresponding asymbol.  Every symbol is an ELF file is actually a
   pointer to this structure, although it is often handled as a
   pointer to an asymbol.  */

typedef struct
{
  /* The BFD symbol.  */
  asymbol symbol;
  /* ELF symbol information.  */
  Elf_Internal_Sym internal_elf_sym;
  /* Backend specific information.  */
  union
    {
      unsigned int hppa_arg_reloc;
      PTR mips_extr;
      PTR any;
    }
  tc_data;
} elf_symbol_type;

/* ELF linker hash table entries.  */

struct elf_link_hash_entry
{
  struct bfd_link_hash_entry root;

  /* Symbol index in output file.  This is initialized to -1.  It is
     set to -2 if the symbol is used by a reloc.  */
  long indx;

  /* Symbol size.  */
  bfd_size_type size;

  /* Symbol index as a dynamic symbol.  Initialized to -1, and remains
     -1 if this is not a dynamic symbol.  */
  long dynindx;

  /* String table index in .dynstr if this is a dynamic symbol.  */
  unsigned long dynstr_index;

  /* If this is a weak defined symbol from a dynamic object, this
     field points to a defined symbol with the same value, if there is
     one.  Otherwise it is NULL.  */
  struct elf_link_hash_entry *weakdef;

  /* If this symbol requires an entry in the global offset table, the
     processor specific backend uses this field to hold the offset
     into the .got section.  If this field is -1, then the symbol does
     not require a global offset table entry.  */
  bfd_vma got_offset;

  /* If this symbol requires an entry in the procedure linkage table,
     the processor specific backend uses these two fields to hold the
     offset into the procedure linkage section and the offset into the
     .got section.  If plt_offset is -1, then the symbol does not
     require an entry in the procedure linkage table.  */
  bfd_vma plt_offset;

  /* Symbol type (STT_NOTYPE, STT_OBJECT, etc.).  */
  char type;

  /* Some flags; legal values follow.  */
  unsigned char elf_link_hash_flags;
  /* Symbol is referenced by a non-shared object.  */
#define ELF_LINK_HASH_REF_REGULAR 01
  /* Symbol is defined by a non-shared object.  */
#define ELF_LINK_HASH_DEF_REGULAR 02
  /* Symbol is referenced by a shared object.  */
#define ELF_LINK_HASH_REF_DYNAMIC 04
  /* Symbol is defined by a shared object.  */
#define ELF_LINK_HASH_DEF_DYNAMIC 010
  /* Dynamic symbol has been adjustd.  */
#define ELF_LINK_HASH_DYNAMIC_ADJUSTED 020
  /* Symbol needs a copy reloc.  */
#define ELF_LINK_HASH_NEEDS_COPY 040
  /* Symbol needs a procedure linkage table entry.  */
#define ELF_LINK_HASH_NEEDS_PLT 0100
};

/* ELF linker hash table.  */

struct elf_link_hash_table
{
  struct bfd_link_hash_table root;
  /* Whether we have created the special dynamic sections required
     when linking against or generating a shared object.  */
  boolean dynamic_sections_created;
  /* The BFD used to hold special sections created by the linker.
     This will be the first BFD found which requires these sections to
     be created.  */
  bfd *dynobj;
  /* The number of symbols found in the link which must be put into
     the .dynsym section.  */
  size_t dynsymcount;
  /* The string table of dynamic symbols, which becomes the .dynstr
     section.  */
  struct bfd_strtab_hash *dynstr;
  /* The number of buckets in the hash table in the .hash section.
     This is based on the number of dynamic symbols.  */
  size_t bucketcount;
  /* A linked list of DT_NEEDED names found in dynamic objects
     included in the link.  */
  struct bfd_link_needed_list *needed;
};

/* Look up an entry in an ELF linker hash table.  */

#define elf_link_hash_lookup(table, string, create, copy, follow)	\
  ((struct elf_link_hash_entry *)					\
   bfd_link_hash_lookup (&(table)->root, (string), (create),		\
			 (copy), (follow)))

/* Traverse an ELF linker hash table.  */

#define elf_link_hash_traverse(table, func, info)			\
  (bfd_link_hash_traverse						\
   (&(table)->root,							\
    (boolean (*) PARAMS ((struct bfd_link_hash_entry *, PTR))) (func),	\
    (info)))

/* Get the ELF linker hash table from a link_info structure.  */

#define elf_hash_table(p) ((struct elf_link_hash_table *) ((p)->hash))

/* Constant information held for an ELF backend.  */

struct elf_size_info {
  unsigned char sizeof_ehdr, sizeof_phdr, sizeof_shdr;
  unsigned char sizeof_rel, sizeof_rela, sizeof_sym, sizeof_dyn, sizeof_note;

  unsigned char arch_size, file_align;
  unsigned char elfclass, ev_current;
  int (*write_out_phdrs) PARAMS ((bfd *, Elf_Internal_Phdr *, int));
  boolean (*write_shdrs_and_ehdr) PARAMS ((bfd *));
  void (*write_relocs) PARAMS ((bfd *, asection *, PTR));
  void (*swap_symbol_out) PARAMS ((bfd *, Elf_Internal_Sym *, PTR));
  boolean (*slurp_reloc_table) PARAMS ((bfd *, asection *, asymbol **));
  long (*slurp_symbol_table) PARAMS ((bfd *, asymbol **, boolean));
};

#define elf_symbol_from(ABFD,S) \
	(((S)->the_bfd->xvec->flavour == bfd_target_elf_flavour \
	  && (S)->the_bfd->tdata.elf_obj_data != 0) \
	 ? (elf_symbol_type *) (S) \
	 : 0)

struct elf_backend_data
{
  /* Whether the backend uses REL or RELA relocations.  FIXME: some
     ELF backends use both.  When we need to support one, this whole
     approach will need to be changed.  */
  int use_rela_p;

  /* The architecture for this backend.  */
  enum bfd_architecture arch;

  /* The ELF machine code (EM_xxxx) for this backend.  */
  int elf_machine_code;

  /* The maximum page size for this backend.  */
  bfd_vma maxpagesize;

  /* This is true if the linker should act like collect and gather
     global constructors and destructors by name.  This is true for
     MIPS ELF because the Irix 5 tools can not handle the .init
     section.  */
  boolean collect;

  /* A function to translate an ELF RELA relocation to a BFD arelent
     structure.  */
  void (*elf_info_to_howto) PARAMS ((bfd *, arelent *,
				     Elf_Internal_Rela *));

  /* A function to translate an ELF REL relocation to a BFD arelent
     structure.  */
  void (*elf_info_to_howto_rel) PARAMS ((bfd *, arelent *,
					 Elf_Internal_Rel *));

  /* A function to determine whether a symbol is global when
     partitioning the symbol table into local and global symbols.
     This should be NULL for most targets, in which case the correct
     thing will be done.  MIPS ELF, at least on the Irix 5, has
     special requirements.  */
  boolean (*elf_backend_sym_is_global) PARAMS ((bfd *, asymbol *));

  /* The remaining functions are hooks which are called only if they
     are not NULL.  */

  /* A function to permit a backend specific check on whether a
     particular BFD format is relevant for an object file, and to
     permit the backend to set any global information it wishes.  When
     this is called elf_elfheader is set, but anything else should be
     used with caution.  If this returns false, the check_format
     routine will return a bfd_error_wrong_format error.  */
  boolean (*elf_backend_object_p) PARAMS ((bfd *));

  /* A function to do additional symbol processing when reading the
     ELF symbol table.  This is where any processor-specific special
     section indices are handled.  */
  void (*elf_backend_symbol_processing) PARAMS ((bfd *, asymbol *));

  /* A function to do additional symbol processing after reading the
     entire ELF symbol table.  */
  boolean (*elf_backend_symbol_table_processing) PARAMS ((bfd *,
							  elf_symbol_type *,
							  unsigned int));

  /* A function to do additional processing on the ELF section header
     just before writing it out.  This is used to set the flags and
     type fields for some sections, or to actually write out data for
     unusual sections.  */
  boolean (*elf_backend_section_processing) PARAMS ((bfd *,
						     Elf32_Internal_Shdr *));

  /* A function to handle unusual section types when creating BFD
     sections from ELF sections.  */
  boolean (*elf_backend_section_from_shdr) PARAMS ((bfd *,
						    Elf32_Internal_Shdr *,
						    char *));

  /* A function to set up the ELF section header for a BFD section in
     preparation for writing it out.  This is where the flags and type
     fields are set for unusual sections.  */
  boolean (*elf_backend_fake_sections) PARAMS ((bfd *, Elf32_Internal_Shdr *,
						asection *));

  /* A function to get the ELF section index for a BFD section.  If
     this returns true, the section was found.  If it is a normal ELF
     section, *RETVAL should be left unchanged.  If it is not a normal
     ELF section *RETVAL should be set to the SHN_xxxx index.  */
  boolean (*elf_backend_section_from_bfd_section)
    PARAMS ((bfd *, Elf32_Internal_Shdr *, asection *, int *retval));

  /* If this field is not NULL, it is called by the add_symbols phase
     of a link just before adding a symbol to the global linker hash
     table.  It may modify any of the fields as it wishes.  If *NAME
     is set to NULL, the symbol will be skipped rather than being
     added to the hash table.  This function is responsible for
     handling all processor dependent symbol bindings and section
     indices, and must set at least *FLAGS and *SEC for each processor
     dependent case; failure to do so will cause a link error.  */
  boolean (*elf_add_symbol_hook)
    PARAMS ((bfd *abfd, struct bfd_link_info *info,
	     const Elf_Internal_Sym *, const char **name,
	     flagword *flags, asection **sec, bfd_vma *value));

  /* If this field is not NULL, it is called by the elf_link_output_sym
     phase of a link for each symbol which will appear in the object file.  */
  boolean (*elf_backend_link_output_symbol_hook)
    PARAMS ((bfd *, struct bfd_link_info *info, const char *,
	     Elf_Internal_Sym *, asection *));

  /* The CREATE_DYNAMIC_SECTIONS function is called by the ELF backend
     linker the first time it encounters a dynamic object in the link.
     This function must create any sections required for dynamic
     linking.  The ABFD argument is a dynamic object.  The .interp,
     .dynamic, .dynsym, .dynstr, and .hash functions have already been
     created, and this function may modify the section flags if
     desired.  This function will normally create the .got and .plt
     sections, but different backends have different requirements.  */
  boolean (*elf_backend_create_dynamic_sections)
    PARAMS ((bfd *abfd, struct bfd_link_info *info));

  /* The CHECK_RELOCS function is called by the add_symbols phase of
     the ELF backend linker.  It is called once for each section with
     relocs of an object file, just after the symbols for the object
     file have been added to the global linker hash table.  The
     function must look through the relocs and do any special handling
     required.  This generally means allocating space in the global
     offset table, and perhaps allocating space for a reloc.  The
     relocs are always passed as Rela structures; if the section
     actually uses Rel structures, the r_addend field will always be
     zero.  */
  boolean (*check_relocs)
    PARAMS ((bfd *abfd, struct bfd_link_info *info, asection *o,
	     const Elf_Internal_Rela *relocs));

  /* The ADJUST_DYNAMIC_SYMBOL function is called by the ELF backend
     linker for every symbol which is defined by a dynamic object and
     referenced by a regular object.  This is called after all the
     input files have been seen, but before the SIZE_DYNAMIC_SECTIONS
     function has been called.  The hash table entry should be
     bfd_link_hash_defined ore bfd_link_hash_defweak, and it should be
     defined in a section from a dynamic object.  Dynamic object
     sections are not included in the final link, and this function is
     responsible for changing the value to something which the rest of
     the link can deal with.  This will normally involve adding an
     entry to the .plt or .got or some such section, and setting the
     symbol to point to that.  */
  boolean (*elf_backend_adjust_dynamic_symbol)
    PARAMS ((struct bfd_link_info *info, struct elf_link_hash_entry *h));

  /* The SIZE_DYNAMIC_SECTIONS function is called by the ELF backend
     linker after all the linker input files have been seen but before
     the sections sizes have been set.  This is called after
     ADJUST_DYNAMIC_SYMBOL has been called on all appropriate symbols.
     It is only called when linking against a dynamic object.  It must
     set the sizes of the dynamic sections, and may fill in their
     contents as well.  The generic ELF linker can handle the .dynsym,
     .dynstr and .hash sections.  This function must handle the
     .interp section and any sections created by the
     CREATE_DYNAMIC_SECTIONS entry point.  */
  boolean (*elf_backend_size_dynamic_sections)
    PARAMS ((bfd *output_bfd, struct bfd_link_info *info));

  /* The RELOCATE_SECTION function is called by the ELF backend linker
     to handle the relocations for a section.

     The relocs are always passed as Rela structures; if the section
     actually uses Rel structures, the r_addend field will always be
     zero.

     This function is responsible for adjust the section contents as
     necessary, and (if using Rela relocs and generating a
     relocateable output file) adjusting the reloc addend as
     necessary.

     This function does not have to worry about setting the reloc
     address or the reloc symbol index.

     LOCAL_SYMS is a pointer to the swapped in local symbols.

     LOCAL_SECTIONS is an array giving the section in the input file
     corresponding to the st_shndx field of each local symbol.

     The global hash table entry for the global symbols can be found
     via elf_sym_hashes (input_bfd).

     When generating relocateable output, this function must handle
     STB_LOCAL/STT_SECTION symbols specially.  The output symbol is
     going to be the section symbol corresponding to the output
     section, which means that the addend must be adjusted
     accordingly.  */
  boolean (*elf_backend_relocate_section)
    PARAMS ((bfd *output_bfd, struct bfd_link_info *info,
	     bfd *input_bfd, asection *input_section, bfd_byte *contents,
	     Elf_Internal_Rela *relocs, Elf_Internal_Sym *local_syms,
	     asection **local_sections));

  /* The FINISH_DYNAMIC_SYMBOL function is called by the ELF backend
     linker just before it writes a symbol out to the .dynsym section.
     The processor backend may make any required adjustment to the
     symbol.  It may also take the opportunity to set contents of the
     dynamic sections.  Note that FINISH_DYNAMIC_SYMBOL is called on
     all .dynsym symbols, while ADJUST_DYNAMIC_SYMBOL is only called
     on those symbols which are defined by a dynamic object.  */
  boolean (*elf_backend_finish_dynamic_symbol)
    PARAMS ((bfd *output_bfd, struct bfd_link_info *info,
	     struct elf_link_hash_entry *h, Elf_Internal_Sym *sym));

  /* The FINISH_DYNAMIC_SECTIONS function is called by the ELF backend
     linker just before it writes all the dynamic sections out to the
     output file.  The FINISH_DYNAMIC_SYMBOL will have been called on
     all dynamic symbols.  */
  boolean (*elf_backend_finish_dynamic_sections)
    PARAMS ((bfd *output_bfd, struct bfd_link_info *info));

  /* A function to do any beginning processing needed for the ELF file
     before building the ELF headers and computing file positions.  */
  void (*elf_backend_begin_write_processing)
    PARAMS ((bfd *, struct bfd_link_info *));

  /* A function to do any final processing needed for the ELF file
     before writing it out.  The LINKER argument is true if this BFD
     was created by the ELF backend linker.  */
  void (*elf_backend_final_write_processing)
    PARAMS ((bfd *, boolean linker));

  /* A function to create any special program headers required by the
     backend.  PHDRS are the program headers, and PHDR_COUNT is the
     number of them.  If PHDRS is NULL, this just counts headers
     without creating them.  This returns an updated value for
     PHDR_COUNT.  */
  int (*elf_backend_create_program_headers)
    PARAMS ((bfd *, Elf_Internal_Phdr *phdrs, int phdr_count));

  /* The swapping table to use when dealing with ECOFF information.
     Used for the MIPS ELF .mdebug section.  */
  const struct ecoff_debug_swap *elf_backend_ecoff_debug_swap;

  /* Alternate EM_xxxx machine codes for this backend.  */
  int elf_machine_alt1;
  int elf_machine_alt2;

  const struct elf_size_info *s;

  unsigned want_got_plt : 1;
  unsigned plt_readonly : 1;
  unsigned want_plt_sym : 1;

  /* Put ELF and program headers in the first loadable segment.  */
  unsigned want_hdr_in_seg : 1;
};

/* Information stored for each BFD section in an ELF file.  This
   structure is allocated by elf_new_section_hook.  */

struct bfd_elf_section_data {
  /* The ELF header for this section.  */
  Elf_Internal_Shdr this_hdr;
  /* The ELF header for the reloc section associated with this
     section, if any.  */
  Elf_Internal_Shdr rel_hdr;
  /* The ELF section number of this section.  Only used for an output
     file.  */
  int this_idx;
  /* The ELF section number of the reloc section associated with this
     section, if any.  Only used for an output file.  */
  int rel_idx;
  /* Used by the backend linker to store the symbol hash table entries
     associated with relocs against global symbols.  */
  struct elf_link_hash_entry **rel_hashes;
  /* A pointer to the swapped relocs.  If the section uses REL relocs,
     rather than RELA, all the r_addend fields will be zero.  This
     pointer may be NULL.  It is used by the backend linker.  */
  Elf_Internal_Rela *relocs;
  /* Used by the backend linker when generating a shared library to
     record the dynamic symbol index for a section symbol
     corresponding to this section.  */
  long dynindx;
};

#define elf_section_data(sec)  ((struct bfd_elf_section_data*)sec->used_by_bfd)

#define get_elf_backend_data(abfd) \
  ((struct elf_backend_data *) (abfd)->xvec->backend_data)

/* Some private data is stashed away for future use using the tdata pointer
   in the bfd structure.  */

struct elf_obj_tdata
{
  Elf_Internal_Ehdr elf_header[1];	/* Actual data, but ref like ptr */
  Elf_Internal_Shdr **elf_sect_ptr;
  Elf_Internal_Phdr *phdr;
  struct bfd_strtab_hash *strtab_ptr;
  int num_locals;
  int num_globals;
  asymbol **section_syms;	/* STT_SECTION symbols for each section */
  Elf_Internal_Shdr symtab_hdr;
  Elf_Internal_Shdr shstrtab_hdr;
  Elf_Internal_Shdr strtab_hdr;
  Elf_Internal_Shdr dynsymtab_hdr;
  Elf_Internal_Shdr dynstrtab_hdr;
  unsigned int symtab_section, shstrtab_section;
  unsigned int strtab_section, dynsymtab_section;
  file_ptr next_file_pos;
  void *prstatus;		/* The raw /proc prstatus structure */
  void *prpsinfo;		/* The raw /proc prpsinfo structure */
  bfd_vma gp;			/* The gp value (MIPS only, for now) */
  unsigned int gp_size;		/* The gp size (MIPS only, for now) */

  /* This is set to true if the object was created by the backend
     linker.  */
  boolean linker;

  /* A mapping from external symbols to entries in the linker hash
     table, used when linking.  This is indexed by the symbol index
     minus the sh_info field of the symbol table header.  */
  struct elf_link_hash_entry **sym_hashes;

  /* A mapping from local symbols to offsets into the global offset
     table, used when linking.  This is indexed by the symbol index.  */
  bfd_vma *local_got_offsets;

  /* The linker ELF emulation code needs to let the backend ELF linker
     know what filename should be used for a dynamic object if the
     dynamic object is found using a search.  This field is used to
     hold that information.  */
  const char *dt_needed_name;

  /* Irix 5 often screws up the symbol table, sorting local symbols
     after global symbols.  This flag is set if the symbol table in
     this BFD appears to be screwed up.  If it is, we ignore the
     sh_info field in the symbol table header, and always read all the
     symbols.  */
  boolean bad_symtab;

  /* Records the result of `get_program_header_size'.  */
  bfd_size_type program_header_size;

  /* Used by MIPS ELF find_nearest_line entry point.  The structure
     could be included directly in this one, but there's no point to
     wasting the memory just for the infrequently called
     find_nearest_line.  */
  struct mips_elf_find_line *find_line_info;

  /* Used by PowerPC to determine if the e_flags field has been intiialized */
  boolean ppc_flags_init;
};

#define elf_tdata(bfd)		((bfd) -> tdata.elf_obj_data)
#define elf_elfheader(bfd)	(elf_tdata(bfd) -> elf_header)
#define elf_elfsections(bfd)	(elf_tdata(bfd) -> elf_sect_ptr)
#define elf_shstrtab(bfd)	(elf_tdata(bfd) -> strtab_ptr)
#define elf_onesymtab(bfd)	(elf_tdata(bfd) -> symtab_section)
#define elf_dynsymtab(bfd)	(elf_tdata(bfd) -> dynsymtab_section)
#define elf_num_locals(bfd)	(elf_tdata(bfd) -> num_locals)
#define elf_num_globals(bfd)	(elf_tdata(bfd) -> num_globals)
#define elf_section_syms(bfd)	(elf_tdata(bfd) -> section_syms)
#define core_prpsinfo(bfd)	(elf_tdata(bfd) -> prpsinfo)
#define core_prstatus(bfd)	(elf_tdata(bfd) -> prstatus)
#define elf_gp(bfd)		(elf_tdata(bfd) -> gp)
#define elf_gp_size(bfd)	(elf_tdata(bfd) -> gp_size)
#define elf_sym_hashes(bfd)	(elf_tdata(bfd) -> sym_hashes)
#define elf_local_got_offsets(bfd) (elf_tdata(bfd) -> local_got_offsets)
#define elf_dt_needed_name(bfd)	(elf_tdata(bfd) -> dt_needed_name)
#define elf_bad_symtab(bfd)	(elf_tdata(bfd) -> bad_symtab)
#define elf_ppc_flags_init(bfd)	(elf_tdata(bfd) -> ppc_flags_init)

extern char * bfd_elf_string_from_elf_section PARAMS ((bfd *, unsigned, unsigned));
extern char * bfd_elf_get_str_section PARAMS ((bfd *, unsigned));

extern void bfd_elf_print_symbol PARAMS ((bfd *, PTR, asymbol *,
					  bfd_print_symbol_type));
#define elf_string_from_elf_strtab(abfd,strindex) \
     bfd_elf_string_from_elf_section(abfd,elf_elfheader(abfd)->e_shstrndx,strindex)

#define bfd_elf32_print_symbol	bfd_elf_print_symbol
#define bfd_elf64_print_symbol	bfd_elf_print_symbol
#define bfd_elf32_mkobject	bfd_elf_mkobject
#define bfd_elf64_mkobject	bfd_elf_mkobject
#define elf_mkobject		bfd_elf_mkobject

extern unsigned long bfd_elf_hash PARAMS ((CONST unsigned char *));

extern bfd_reloc_status_type bfd_elf_generic_reloc PARAMS ((bfd *,
							    arelent *,
							    asymbol *,
							    PTR,
							    asection *,
							    bfd *,
							    char **));
extern boolean bfd_elf_mkobject PARAMS ((bfd *));
extern Elf_Internal_Shdr *bfd_elf_find_section PARAMS ((bfd *, char *));
extern boolean _bfd_elf_make_section_from_shdr
  PARAMS ((bfd *abfd, Elf_Internal_Shdr *hdr, const char *name));
extern struct bfd_hash_entry *_bfd_elf_link_hash_newfunc
  PARAMS ((struct bfd_hash_entry *, struct bfd_hash_table *, const char *));
extern struct bfd_link_hash_table *_bfd_elf_link_hash_table_create
  PARAMS ((bfd *));
extern boolean _bfd_elf_link_hash_table_init
  PARAMS ((struct elf_link_hash_table *, bfd *,
	   struct bfd_hash_entry *(*) (struct bfd_hash_entry *,
				       struct bfd_hash_table *,
				       const char *)));

extern boolean _bfd_elf_copy_private_symbol_data
  PARAMS ((bfd *, asymbol *, bfd *, asymbol *));
extern boolean _bfd_elf_copy_private_section_data
  PARAMS ((bfd *, asection *, bfd *, asection *));
extern boolean _bfd_elf_write_object_contents PARAMS ((bfd *));
extern boolean _bfd_elf_set_section_contents PARAMS ((bfd *, sec_ptr, PTR,
						       file_ptr,
						       bfd_size_type));
extern long _bfd_elf_get_symtab_upper_bound PARAMS ((bfd *));
extern long _bfd_elf_get_symtab PARAMS ((bfd *, asymbol **));
extern long _bfd_elf_get_dynamic_symtab_upper_bound PARAMS ((bfd *));
extern long _bfd_elf_canonicalize_dynamic_symtab PARAMS ((bfd *, asymbol **));
extern long _bfd_elf_get_reloc_upper_bound PARAMS ((bfd *, sec_ptr));
extern long _bfd_elf_canonicalize_reloc PARAMS ((bfd *, sec_ptr,
						  arelent **, asymbol **));
extern asymbol *_bfd_elf_make_empty_symbol PARAMS ((bfd *));
extern void _bfd_elf_get_symbol_info PARAMS ((bfd *, asymbol *,
					       symbol_info *));
extern alent *_bfd_elf_get_lineno PARAMS ((bfd *, asymbol *));
extern boolean _bfd_elf_set_arch_mach PARAMS ((bfd *, enum bfd_architecture,
						unsigned long));
extern boolean _bfd_elf_find_nearest_line PARAMS ((bfd *, asection *,
						    asymbol **,
						    bfd_vma, CONST char **,
						    CONST char **,
						    unsigned int *));
#define _bfd_elf_read_minisymbols _bfd_generic_read_minisymbols
#define _bfd_elf_minisymbol_to_symbol _bfd_generic_minisymbol_to_symbol
extern int _bfd_elf_sizeof_headers PARAMS ((bfd *, boolean));
extern boolean _bfd_elf_new_section_hook PARAMS ((bfd *, asection *));

/* If the target doesn't have reloc handling written yet:  */
extern void _bfd_elf_no_info_to_howto PARAMS ((bfd *, arelent *,
					       Elf_Internal_Rela *));

asection *bfd_section_from_elf_index PARAMS ((bfd *, unsigned int));
boolean _bfd_elf_create_dynamic_sections PARAMS ((bfd *,
						  struct bfd_link_info *));
struct bfd_strtab_hash *_bfd_elf_stringtab_init PARAMS ((void));
boolean
_bfd_elf_link_record_dynamic_symbol PARAMS ((struct bfd_link_info *,
					     struct elf_link_hash_entry *));
boolean
_bfd_elf_compute_section_file_positions PARAMS ((bfd *,
						 struct bfd_link_info *));
void _bfd_elf_assign_file_positions_for_relocs PARAMS ((bfd *));
file_ptr _bfd_elf_assign_file_position_for_section PARAMS ((Elf_Internal_Shdr *,
							    file_ptr,
							    boolean));

boolean _bfd_elf_create_dynamic_sections PARAMS ((bfd *,
						  struct bfd_link_info *));
boolean _bfd_elf_create_got_section PARAMS ((bfd *,
					     struct bfd_link_info *));

extern const bfd_target *bfd_elf32_object_p PARAMS ((bfd *));
extern const bfd_target *bfd_elf32_core_file_p PARAMS ((bfd *));
extern char *bfd_elf32_core_file_failing_command PARAMS ((bfd *));
extern int bfd_elf32_core_file_failing_signal PARAMS ((bfd *));
extern boolean bfd_elf32_core_file_matches_executable_p PARAMS ((bfd *,
								 bfd *));

extern boolean bfd_elf32_bfd_link_add_symbols
  PARAMS ((bfd *, struct bfd_link_info *));
extern boolean bfd_elf32_bfd_final_link
  PARAMS ((bfd *, struct bfd_link_info *));

extern void bfd_elf32_swap_symbol_in
  PARAMS ((bfd *, Elf32_External_Sym *, Elf_Internal_Sym *));
extern void bfd_elf32_swap_symbol_out
  PARAMS ((bfd *, Elf_Internal_Sym *, PTR));
extern void bfd_elf32_swap_reloc_in
  PARAMS ((bfd *, Elf32_External_Rel *, Elf_Internal_Rel *));
extern void bfd_elf32_swap_reloc_out
  PARAMS ((bfd *, Elf_Internal_Rel *, Elf32_External_Rel *));
extern void bfd_elf32_swap_reloca_in
  PARAMS ((bfd *, Elf32_External_Rela *, Elf_Internal_Rela *));
extern void bfd_elf32_swap_reloca_out
  PARAMS ((bfd *, Elf_Internal_Rela *, Elf32_External_Rela *));
extern void bfd_elf32_swap_phdr_in
  PARAMS ((bfd *, Elf32_External_Phdr *, Elf_Internal_Phdr *));
extern void bfd_elf32_swap_phdr_out
  PARAMS ((bfd *, Elf_Internal_Phdr *, Elf32_External_Phdr *));
extern void bfd_elf32_swap_dyn_in
  PARAMS ((bfd *, const Elf32_External_Dyn *, Elf_Internal_Dyn *));
extern void bfd_elf32_swap_dyn_out
  PARAMS ((bfd *, const Elf_Internal_Dyn *, Elf32_External_Dyn *));
extern boolean bfd_elf32_add_dynamic_entry
  PARAMS ((struct bfd_link_info *, bfd_vma, bfd_vma));
extern boolean bfd_elf32_link_create_dynamic_sections
  PARAMS ((bfd *, struct bfd_link_info *));

extern const bfd_target *bfd_elf64_object_p PARAMS ((bfd *));
extern const bfd_target *bfd_elf64_core_file_p PARAMS ((bfd *));
extern char *bfd_elf64_core_file_failing_command PARAMS ((bfd *));
extern int bfd_elf64_core_file_failing_signal PARAMS ((bfd *));
extern boolean bfd_elf64_core_file_matches_executable_p PARAMS ((bfd *,
								 bfd *));
extern boolean bfd_elf64_bfd_link_add_symbols
  PARAMS ((bfd *, struct bfd_link_info *));
extern boolean bfd_elf64_bfd_final_link
  PARAMS ((bfd *, struct bfd_link_info *));

extern void bfd_elf64_swap_symbol_in
  PARAMS ((bfd *, Elf64_External_Sym *, Elf_Internal_Sym *));
extern void bfd_elf64_swap_symbol_out
  PARAMS ((bfd *, Elf_Internal_Sym *, PTR));
extern void bfd_elf64_swap_reloc_in
  PARAMS ((bfd *, Elf64_External_Rel *, Elf_Internal_Rel *));
extern void bfd_elf64_swap_reloc_out
  PARAMS ((bfd *, Elf_Internal_Rel *, Elf64_External_Rel *));
extern void bfd_elf64_swap_reloca_in
  PARAMS ((bfd *, Elf64_External_Rela *, Elf_Internal_Rela *));
extern void bfd_elf64_swap_reloca_out
  PARAMS ((bfd *, Elf_Internal_Rela *, Elf64_External_Rela *));
extern void bfd_elf64_swap_phdr_in
  PARAMS ((bfd *, Elf64_External_Phdr *, Elf_Internal_Phdr *));
extern void bfd_elf64_swap_phdr_out
  PARAMS ((bfd *, Elf_Internal_Phdr *, Elf64_External_Phdr *));
extern void bfd_elf64_swap_dyn_in
  PARAMS ((bfd *, const Elf64_External_Dyn *, Elf_Internal_Dyn *));
extern void bfd_elf64_swap_dyn_out
  PARAMS ((bfd *, const Elf_Internal_Dyn *, Elf64_External_Dyn *));
extern boolean bfd_elf64_add_dynamic_entry
  PARAMS ((struct bfd_link_info *, bfd_vma, bfd_vma));
extern boolean bfd_elf64_link_create_dynamic_sections
  PARAMS ((bfd *, struct bfd_link_info *));

#define bfd_elf32_link_record_dynamic_symbol _bfd_elf_link_record_dynamic_symbol
#define bfd_elf64_link_record_dynamic_symbol _bfd_elf_link_record_dynamic_symbol

#endif /* _LIBELF_H_ */
