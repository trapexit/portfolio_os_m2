/*  @(#) elf_r.h 96/07/25 1.27 */

//====================================================================
// elf_r.h  -  ElfReader class defs for reading ELF files with dwarf v1.1 debug info
//
//		Symbolics class hierarchy:
//
//			Symbolics	- contains main functions visible to world
//				SymNet	- builds internal network of symbols and queries them
//					SymReader - reads ARM sym files with sym debug info and
//								calls SymNet methods to add symbols to network
//						SymDumper - dumps ARM sym files 
//					XcoffReader - reads XCOFF files with dbx stabs debug info and
//								calls SymNet methods to add symbols to network
//						XcoffDumper - dumps XCOFF files 
//					ElfReader - reads ELF files with dwarf v1.1 debug info and
//								calls SymNet methods to add symbols to network
//						ElfDumper - dumps ELF files 
//						ElfLinker - links ELF files 

#ifndef __ELF_R_H__
#define __ELF_R_H__

#ifndef USE_DUMP_FILE
#include "predefines.h"
#include "symnet.h"
#include "utils.h"
#endif
#include "elf.h" //elf file format

#define ELF_NEW(x) HEAP_NEW(global_heap,x) 
#define ELF_DELETE(x) HEAP_DELETE(global_heap,x) 
class TypesStack;


class ElfReader : public SymNet, protected Endian, public SymFwdrefTypes {
    friend class ArReader;
    friend class EachSection;
    friend class ElfDumper;
public:
#if defined(__NEW_ARMSYM__) || defined(SYMBOLICS_H)
	ElfReader(int dump=NO_DUMP);
    virtual SymErr ISymbolics(const StringPtr file_name); //must pass fname to constructor
    virtual SymErr ISymbolics();
#else
    ElfReader(const char *name, int dump=0);
#endif
    virtual ~ElfReader();
    
// other public funcs
    SymErr read_elf_hdr();
    SymErr read_program_hdrs();
    SymErr read_section_hdrs();
	uint8* read_section(unsigned long sidx);
    SymErr read_string_table(unsigned long sidx, char *&strtab /*, unsigned long &len*/);
    SymErr read_symbol_table(unsigned long sidx, Elf32_Sym *&symtab, char *&strtab);
    SymErr read_main_symbol_table();
	SymErr read_debug();
	SymErr build_syms(BufEaters* buf);
    SymErr read_line_table();
    SymErr read_dynamic_data(unsigned long sidx,
            Elf32_Dyn *&dyn, unsigned long &ndyn, char *&strtab,
            unsigned long &nstr);
    char *symname(Elf32_Sym *s, char *strtab, unsigned long len);
    char* section_name(unsigned long i);
    Boolean isa_dll();
protected:
    char *_sh_strtab;
    char *_strtab; 
    Elf32_Ehdr *_elf_hdr;
    Elf32_Shdr *_sec_hdr;
    Elf32_Sym *_symtab; 
    Elf32_Sym **_symtab_sorted_by_section;
    Elf32_Sym **_symtab_sorted_by_address;
	//static Heap* _heap;
	//used by dumper
    unsigned elf_hash_size;
	uint32 elf_offset; 	//file offset used for type definition
    unsigned long elf_hash(const char *name);
    char* symstr(unsigned long i);      // return string from sh_strtab
    char* get_symname(uint32 symidx);
    // find the number of symbols in symtab
	INLINE uint32 get_symtab_nsyms();
    INLINE static unsigned long get_symtab_nsyms(Elf32_Shdr* s);
    // get strtab for symtab
    INLINE Elf32_Shdr* get_symtab_strtab(Elf32_Shdr* s);
	uint32 find_sym_by_val(int secnum,uint32 val);
    Elf32_Sym* lookup_symbol(unsigned long sidx, unsigned long address,
        unsigned long &off);
    SymErr read_symbol_table_sorted(unsigned long sidx,
        Elf32_Sym **&by_section, Elf32_Sym **&by_address,
        Elf32_Sym *&symtab, unsigned long &nsym,
        char *&strtab, unsigned long &nstr);
    INLINE Elf32_Shdr* sec_shdr(SymSec sec);
	SymSec get_sectype(char* name);
	char* get_secname(SymSec);
private:
    SymModEntry* _cur_mod;	//for keeping scope while parsing dwarf
    SymFuncEntry* _cur_func;
    SymFuncEntry* _cur_block;
	TypesStack* _cur_type;	//stack for enums/structs/unions and their members

    INLINE uint32 sec_num(SymSec sec);
    INLINE unsigned long sec_size(SymSec sec);
    Elf32_Sym* lookup_in(long sidx, unsigned long address,
        long lo, long hi);
    Elf32_Sym* lookup_adr_in(unsigned long address,
        long lo, long hi);
    Elf32_Sym* lookup_address(unsigned long address,
        unsigned long &off);
    SymModEntry* find_mod(uint32);
	SymCat get_type_cat(uint32 elftype);
	INLINE virtual SymType* get_type(uint32 elftype);
	SymType* get_user_type(uint32 elftype);
	SymType* get_mod_type(uint16 len,unsigned char* buf);
	SymType* get_mod_user_type(uint32 len,unsigned char* buf);
	SymType* add_modtypes(uint32 len,unsigned char* buf, SymType* type);
	SymbolClass get_symclass(SymSec sec);
	SymSec get_symsectype(uint32 idx);	//from symidx
	int get_relocs(int s);
	SymScope st_bind(int i);
	void parse_loc(uint32 len,unsigned char* buf,
				uint32& val,SymSec& sec,SymbolClass& sclass);
	void parse_enum_list(uint32 len,unsigned char* buf,Stack* estack);
	void parse_subscr(uint32 len,unsigned char* buf,
				SymType*& type,int& low_bound, int& high_bound);
	SymErr build_array_type(BufEaters* buf);
	SymErr build_class_type(BufEaters* buf);
	SymErr build_entry_point(BufEaters* buf);
	SymErr build_enumeration_type(BufEaters* buf);
	SymErr build_formal_parameter(BufEaters* buf);
	SymErr build_global_subroutine(BufEaters* buf);
	SymErr build_global_variable(BufEaters* buf);
	SymErr build_label(BufEaters* buf);
	SymErr build_lexical_block(BufEaters* buf);
	SymErr build_local_variable(BufEaters* buf);
	SymErr build_member(BufEaters* buf);
	SymErr build_pointer_type(BufEaters* buf);
	SymErr build_reference_type(BufEaters* buf);
	SymErr build_compile_unit(BufEaters* buf);
	SymErr build_common_block(BufEaters* buf);
	SymErr build_common_inclusion(BufEaters* buf);
	SymErr build_inheritance(BufEaters* buf);
	SymErr build_inlined_subroutine(BufEaters* buf);
	SymErr build_module(BufEaters* buf);
	SymErr build_ptr_to_member_type(BufEaters* buf);
	SymErr build_set_type(BufEaters* buf);
	SymErr build_structure_type(BufEaters* buf);
	SymErr build_subroutine(BufEaters* buf);
	SymErr build_subroutine_type(BufEaters* buf);
	SymErr build_typedef(BufEaters* buf);
	SymErr build_union_type(BufEaters* buf);
	SymErr build_unspecified_parameters(BufEaters* buf);
	SymErr build_subrange_type(BufEaters* buf);
	SymErr add_members();
    };

//==========================================================================
//defines for class TypesStack

class TypesStack : Stack {
	class _type_ptr {
	friend class TypesStack;
		SymCat _kind;
		Queue* _queue_o_members;
		uint32 _file_offset;	//position in file where added to stack
		uint32 _ref;		//sibling for this dwarf entry
		union {
			SymType* _type;
			SymStructType* _struct_or_union;
			SymEnumType* _enum;
			SymFuncEntry* _func;
			SymModEntry* _mod;
			} _type;
		_type_ptr(SymType* type,uint32 off,uint32 ref) {
			_type._type = type;
			_kind = type->cat();
			_queue_o_members = 0;
			_file_offset = off;
			_ref = ref;
			}
		_type_ptr(SymFuncEntry* ftype,uint32 off,uint32 ref) {
			_type._func = ftype;
			_kind = tc_function;
			_queue_o_members = 0;
			_file_offset = off;
			_ref = ref;
			}
		_type_ptr(SymModEntry* mtype,uint32 off,uint32 ref) {
			_type._mod = mtype;
			_kind = tc_void;
			_queue_o_members = 0;
			_file_offset = off;
			_ref = ref;
			}
		_type_ptr(SymStructType* stype,uint32 off,uint32 ref) {
			_type._struct_or_union = stype;
			_kind = tc_struct;
			_queue_o_members = (Queue*) NEW(Queue);
			_file_offset = off;
			_ref = ref;
			}
		_type_ptr(SymEnumType* etype,uint32 off,uint32 ref) {
			_type._enum = etype;
			_kind = tc_enum;
			_queue_o_members = (Queue*) NEW(Queue);
			_file_offset = off;
			_ref = ref;
			}
		~_type_ptr() {
			DBG_ASSERT(!_queue_o_members || _queue_o_members->empty());
			if (_queue_o_members) DELETE(_queue_o_members);
			}
		};
	_type_ptr* _cur_type;
	void add_members();
public:
	TypesStack() { _cur_type = 0; }
	~TypesStack() { DBG_ASSERT(Stack::empty()); }
	Boolean empty() { return Stack::empty(); }
	void pop();
	void push(_type_ptr* type);
	void push(SymType* type,uint32 off,uint32 ref);
	void push(SymStructType* stype,uint32 off,uint32 ref);
	void push(SymEnumType* etype,uint32 off,uint32 ref);
	void push(SymFuncEntry* ftype,uint32 off,uint32 ref);
	void push(SymModEntry* mtype,uint32 off,uint32 ref);
	void add_member(uint32 val, SymType* type);
	Boolean isa(SymCat cat) { return (Boolean) (cat==_cur_type->_kind); }
	SymCat cat() { return _cur_type->_kind; }
	uint32 file_offset() { return _cur_type->_file_offset; }
	uint32 sibling() { return _cur_type->_ref; }
	};

//==========================================================================
//defines for class attr_val
//		reads attribute's value based on form
//		advances buf, modifies byte_size, and returns requested type
	
union attr_val {
	uint32 addr;	//address
	uint32 ref;	//byte offset into .debug section
	union {
		uint16 s16;
		uint32 s32;
		struct { uint32 hi; uint32 lo; } s64;
		} con;
	struct {
		union {
			uint16 s16;
			uint32 s32;
			} len;
		unsigned char* ptr;
		} block;
	char* str;
	attr_val(uint16 form,BufEaters* buf);
	};

//======================================
class EachSection
//		iterator for sections
//======================================
{
protected:
    Elf32_Shdr *_sec_hdr;
    long i, n;
public:
    EachSection(ElfReader *er)
    :	_sec_hdr(er->_sec_hdr)
    ,	i(1)
    ,	n(er->_elf_hdr->e_shnum)
    {}
    operator int() { return i < n; }
    virtual Elf32_Shdr* next();
    virtual Elf32_Shdr* next(long &x);
}; // class EachSection

//======================================
class EachSectionByOffset : public EachSection
// iterator for sections, ordered by offset
// WARNING: (int)next() will return zero as soon as the
// index becomes n.  In theory, this could happen before
// all sections are enumerated.  In practice, this works
// because the section with the last index happens to be
// the one with the largest offset.
//======================================
{
    long fCurrentOffset;
    long fCurrentIndex;
public:
    EachSectionByOffset(ElfReader *er)
	    : EachSection(er), fCurrentOffset(0), fCurrentIndex(0) {}
    virtual Elf32_Shdr* next();
    virtual Elf32_Shdr* next(long &x);
}; // class EachSectionByOffset

#define REL(s) ((s)->sh_type==SHT_REL)
#define RELA(s) ((s)->sh_type==SHT_RELA)
#endif /* __ELF_R_H__ */

#ifndef __ELF_R_INLINES_H__
#if !defined(USE_DUMP_FILE) || defined(__ELF_R_CP__)
#define __ELF_R_INLINES_H__
INLINE SymType* ElfReader::get_type(uint32 elftype) {
		return get_fun_type(get_type_cat(elftype));
		};
INLINE unsigned long ElfReader::get_symtab_nsyms(Elf32_Shdr* s) {
    	return s->sh_size / (s->sh_entsize?s->sh_entsize:sizeof(Elf32_Sym));
    	}
INLINE uint32 ElfReader::get_symtab_nsyms() { 
		uint32 symtab_sidx = _sections->secnum(sec_symtab);
		if (!symtab_sidx)
			RETURN_INFO(se_no_symbols,("No main symbol table sections!\n"));
		Elf32_Shdr* symtab_shdr = _sec_hdr + symtab_sidx;
		return get_symtab_nsyms(symtab_shdr);
		}
    // get strtab for symtab
INLINE Elf32_Shdr* ElfReader::get_symtab_strtab(Elf32_Shdr* s) {
    	return &_sec_hdr[s->sh_link];
    	}
INLINE uint32 ElfReader::sec_num(SymSec sec) { 
    	return (_sections) ? _sections->secnum(sec) : 0;
    	}
INLINE unsigned long ElfReader::sec_size(SymSec sec) { 
    	return (_sec_hdr) ? _sec_hdr[sec_num(sec)].sh_size : 0;
    	}
INLINE Elf32_Shdr* ElfReader::sec_shdr(SymSec sec) { 
    	return (_sec_hdr) ? &_sec_hdr[sec_num(sec)] : 0;
    	}
#endif /* !defined(USE_DUMP_FILE) || defined(__ELF_R_CP__) */
#endif /* __ELF_R_INLINES_H__ */

