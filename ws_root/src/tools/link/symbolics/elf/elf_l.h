/*  @(#) elf_l.h 96/07/25 1.36 */

//====================================================================
// elf_l.h  -  ElfReader class defs for reading ELF files with dwarf v1.1 debug info
//
//		Symbolics class hierarchy:
//
//			Symbolics	- contains main functions visable to world
//				SymNet	- builds internal network of symbols and queries them
//					ElfReader - reads ELF files with dwarf v1.1 debug info and
//								calls SymNet methods to add symbols to network
//						ElfLinker - links ELF files 

#ifndef __ELF_L_H__
#define __ELF_L_H__

#ifndef USE_DUMP_FILE
#include <stdio.h>
#include "option.h"
#include "linkopts.h"
#include "parse.h"
#include "elf_r.h"
#include "elf_ar.h"
#endif

#include "elf_utils.h"
#include "elf_3do.h"

#define SEC_ALIGNMENT 16       //standard is 64, but we want small

#define LINK_NEW(x) HEAP_NEW(global_heap,x) 
#define LINK_DELETE(x) HEAP_DELETE(global_heap,x) 
#define LINK_DELETE_ARRAY(x) HEAP_DELETE_ARRAY(global_heap,x) 
#define LINK_TMP_NEW(x) HEAP_NEW(global_tmp_heap,x) 
#define LINK_TMP_DELETE(x) HEAP_DELETE(global_tmp_heap,x) 

struct LinkerCompInfo;
//======================================
class ElfLinker : public ElfReader
//======================================
{
    class SymRelocs {
    public:
    	SymSec _sectype;
    	uint8* _buf;
    	int*   _objs;
    	uint32 _nents;
    	uint32 _entsize;
		SymRelocs(SymSec sec, int nfiles, uint32 size);
		~SymRelocs();
    	};
friend struct ArReader;
friend struct FileInfo;
friend struct ObjInfo;
friend struct LibInfo;
friend class ElfLinker::SymRelocs;
friend class Syms;
public:
    ElfLinker(FILE* link_fp,FILE* user_fp,GetOpts*);
    virtual ~ElfLinker();
	virtual SymErr LinkFiles(const char* name);	//file-specific link
	void AddObj(const char* name);
	void AddLib(const char* name);

	
private:
    static int _nsecs;
	static int _nsyms;
	static int _nstrs;
    static SymRelocs** _relocs;
    static int _nrelocs;
    Syms *_unds;	//undefined symbols
    Syms *_coms;	//common symbols
    Syms *_specs;	//special symbols (like _etext, etc)
    Syms *_exps;	//symbols to be exported
    Syms *_imps;	//symbols to be imported
    FILE* _user_fp;
    FILE* _link_fp;
    const char* _link_fname;
    GetOpts* _link_opts;
    LinkHash* _hashtab;
    Elf32_Phdr* _prog_hdr;
    _3DOBinHeaderSection* _3dohdr;	//_3DOBinHeader section contents
    importTemplate* _imports;	//import section contents
    exportTemplate* _exports;	//export section contents

   	// Kluge field to allow error reporting
   	const char* _current_objfilename;
   	Elf32_Addr _current_offset_in_section;
    
	int add_master_sec(SymSec sectype,uint32 base);
	void create_fileinfo_n_sections();
	void create_relocations();
	void create_3dohdr();
	void create_imports();
	void create_exports();
	void parse_def(const char* deffile, const char* buf,int32 sz);
	void parse_imp(DefParse& d);
	void parse_exp(DefParse& d);
	void parse_import_flags(DefParse& d,uint32 flag);
	void add_imps(ObjInfo* o);
	uint32 find_sym_by_val(ObjInfo* o,int secnum,uint32 val);
	void read_fileinfo();
	const char* read_deffile(const char* deffile,int32& sz);
	unsigned char* read_obj_section(ObjInfo* o,int s);
	void add_obj(ObjInfo* o);
	ObjInfo* read_objinfo(SymFile* fp, uint32 off, int libid=0);
	SymErr read_obj_hdr(ObjInfo* o);
	void save_objinfo();
	void get_objinfo(ObjInfo* o);
	void restore_objinfo();
	LibInfo* read_libinfo(SymFile* fp);
	SymErr read_lib_hdr(LibInfo* lib);
	SymErr init_lib(LibInfo* lib);
	void init_hash();
	void init_hash(LibInfo* lib);
	//void init_hashobjs(LibInfo* lib);
	SymErr get_libobjs(LibInfo* lib);
	SymErr get_dllimps(DllInfo* dll);
	int resolve_unds();
	int resolve_unds(LibInfo* l);
	int resolve_imps();
	void resolve_imp(Syms::Sym* imp,Syms::Sym* usym);
	Boolean find_sym(uint32& defsymidx,ObjInfo*& defo, const char* name);
	void add_libobjs(LibInfo* l);
	//void search_libs();	//search libraries for undefineds
	void prune_lib(LibInfo* lib);
	void adjust_baseaddrs();
	void adjust_baseaddrs(SymSec sec,uint32 base);
	void adjust_section(ObjInfo* o,int j);
	int add_sym(Elf32_Sym* sym, const char* name, SymSec sectype, HashOff scopeid=hoff_global);
	void add_sym(ObjInfo* o,int symidx);
	void rmv_sym(char* name, HashOff scopeid=hoff_global);
	void rmv_sym(ObjInfo* o,int symidx);
	void resolve_syms();
	void resolve_syms(Syms* syms);
	Boolean resolve_sym(Syms::Sym* sym);
	SymErr init_symtab();
	SymErr merge_symtabs();
	SymErr apply_relocations(ObjInfo* o, int i, unsigned char* secbuf, unsigned char* relsecbuf);
	void apply_reloc(Elf32_Rela* r, ObjInfo* o, unsigned char* secbuf, uint32 salign);
	void add_relocs(SymRelocs* relocs,ObjInfo* o,int secnum, unsigned char* relsecbuf);
	void add_relocations();
        void init_3donote(sec3do *note, uint32 type, uint32 descsize);
	void update_3dohdr();
	void update_imports();
	void update_exports();
	void update_relocations();
	void update_reloc(Elf32_Rela* r, ObjInfo* o,int osecnum);
	void hash_lib(LibInfo* l);
	//void hash_libobjs(LibInfo* l);
	void hash_symtabs();
	void hash_symtab(ObjInfo* o);
	void reloc_symtabs();
	void reloc_symtab(ObjInfo* o);
	void add_hash(LibInfo* l,char* name,int objoffset);
	int get_objind(LibInfo* l,char* name);
	_linkhash* add_hash(const char* name, int symidx,int objind);
	_linkhash* add_hash(const char* name, HashOff scopeid,int symidx,int objind) {
		_linkhash* h = _hashtab->add_hash(name,scopeid,symidx,objind);
		return h;
		}
	_linkhash* add_hash(ObjInfo* o,int symidx);
	_linkhash* get_hash(ObjInfo* o,int symidx);
	_linkhash* get_hash(const char* name,HashOff scopeid) {
		_linkhash* h = (_linkhash*) _hashtab->get_hash(name,scopeid);
		return h;
		}
	int get_newsymidx(const char* name,HashOff scopeid) {
		_linkhash* h = get_hash(name,scopeid);
		return h?h->_newsymidx:0;
		}
	int get_newsymidx(ObjInfo* o,int oldidx) {
		_linkhash* h = get_hash(o,oldidx);
		return h?h->_newsymidx:0;
		}
    void link_elf_hdr();
    void link_program_hdrs();
    void link_section_hdrs();
    void link_relocation_sections(Elf32_Shdr *);
    void link_dynamic_data(Elf32_Shdr *);
    void link_hash_table(Elf32_Shdr *);
    void link_content(Elf32_Shdr *s, unsigned long i);
    void link_data(Elf32_Shdr *s);
    void link_dyn_entry(Elf32_Dyn *dyn, char *strtab, unsigned long nstr);
	
	void RecordSectionOffset(Elf32_Shdr* sh, const char* name);
	void RecordSectionSize(Elf32_Shdr* sh, const char* name);
	void write_elf_hdr();
	void write_program_hdrs();
	void write_section_hdrs();
	void write_hdr3do();
	void write_section(int secnum, uint8* secbuf);
	void write_sections();
	void write_relocations();
	void write_dynamic_sections();
	void write_shstrtab();
	void write_strtab();
	void write_symtab();
	void write_align(int alignment);
	void cwrite_align(LinkerCompInfo* compInfo, int alignment); // uses the compression bottlenecks
	void generate_mapfile(char* elf_fname,char* mapfile);
	
	int get_phnum();
	void get_prog_hdr(uint32 type,Elf32_Phdr* p,int n,...);
	char* get_shname(SymSec sec);
    uint32 get_shbase(SymSec sec);
	int get_shalign(Elf32_Shdr* s,SymSec sec);
	int get_shalign(SymSec sec);
	uint32 get_shtype(SymSec sec);
	uint32 get_shflags(SymSec sec);
	uint32 get_shentsize(SymSec sec);
	uint32 get_shinfo(SymSec sec);
	uint32 get_shlink(SymSec sec);
	uint32 get_entry();
	int get_hdr3do_field(char* fname);
	uint32 get_reltype(uint32 rtype);
	uint32 get_newreltype(uint32 rtype);
	uint32 get_newimpreltype(uint32 rtype);
	int get_relocs(ObjInfo* o,int s);
	int get_h3dofield(char* fname);
	INLINE int get_secnum(ObjInfo* o,SymSec sec);
	INLINE int get_symtab_nsyms(ObjInfo* o);
    INLINE Elf32_Shdr* get_sechdr(ObjInfo* o,SymSec sec);
	INLINE Elf32_Sym* get_sym(ObjInfo* o,uint32 idx);
	SymSec get_symsectype(ObjInfo* o,uint32 idx);
	INLINE char* get_symname(ObjInfo* o,uint32 idx);
	INLINE char* get_symname(ObjInfo* o,Elf32_Sym *sym);
	uint32 get_optbase(SymSec sectype);
	uint32 get_lib_msize(char* lsize);
	char* get_lib_mname(char* lname, char* lstrtab);
	Boolean isabs(uint32 rtype);
	Boolean isa_dll();
	Boolean isa_dll(ObjInfo*);
	void swap_elf_hdr(Elf32_Ehdr* e);
	void swap_sec_hdr(Elf32_Shdr* s);
	void swap_prog_hdr(Elf32_Phdr* p);
	void swap_sym(Elf32_Sym* s);
	void swap_reloc(Elf32_Rela* r);
	void set_word32(uint8* off, uint32 salign, uint32 val);
	uint32 MergeLow14(uint32 hi, uint32 lo);
	void set_low14(uint8* off, uint32 salign, uint32 val);
	void set_low24(uint8* off, uint32 salign, uint32 val);
	void set_half16(uint8* off, uint32 salign, uint16 val);
	uint16 get_off16(uint32 salign, uint8* off);
	uint32 get_off32(uint32 salign, uint8* off);
	void put_off16(uint16 val, uint32 salign, uint8* off);
	void put_off32(uint32 val, uint32 salign, uint8* off);
	const char* find_file(const char* name);
	INLINE FileInfo* firstfile();
	INLINE int nfiles();
	INLINE ObjInfo* firstobj();
	INLINE ObjInfo* firstobj(int& i);
	INLINE int nobjs();
	INLINE ObjInfo* obj(int i);
	INLINE LibInfo* firstlib();
	INLINE int nlibs();
	INLINE LibInfo* lib(int i);
	INLINE DllInfo* firstdll();
	INLINE int ndlls();
	INLINE DllInfo* dll(int i);
    };


#define SH(sec) (_sec_hdr + _sections->secnum(sec))
//#define FILTER_OUT(sym) (ELF32_ST_TYPE((sym)->st_info)==STT_SECTION)

#endif /* __ELF_L_H__ */


#ifndef __ELF_L_INLINES_H__
#if !defined(USE_DUMP_FILE) || defined(__ELF_L_CP__)
#define __ELF_L_INLINES_H__
INLINE int ElfLinker::get_secnum(ObjInfo* o,SymSec sec) {
    	return (o->_sections)->secnum(sec);
    	}
INLINE int ElfLinker::get_symtab_nsyms(ObjInfo* o) {
		return ElfReader::get_symtab_nsyms(get_sechdr(o,sec_symtab));
    	}
INLINE Elf32_Shdr* ElfLinker::get_sechdr(ObjInfo* o,SymSec sec) { 
    	return (o->_sec_hdr + ((o->_sections)->secnum(sec)));
    	}
INLINE Elf32_Sym* ElfLinker::get_sym(ObjInfo* o,uint32 idx) {
		if (idx>=get_symtab_nsyms(o)) {
			DBG_ERR(("symtab idx=x%X of obj=%s is out of range; nsyms=x%X\n",
				idx,o->_fp->filename(),get_symtab_nsyms(o)));
			DBG_ASSERT(idx<get_symtab_nsyms(o));
			return 0;
			}
		return &o->_symtab[idx];
		}

INLINE char* ElfLinker::get_symname(ObjInfo* o,uint32 idx)
{
		if (idx>=get_symtab_nsyms(o)) {
			DBG_ERR(("symtab idx=x%X of obj=%s is out of range; nsyms=x%X\n",
				idx,o->_fp->filename(),get_symtab_nsyms(o)));
			DBG_ASSERT(idx<get_symtab_nsyms(o));
			return 0;
			}
		else if (!o->_strtab)
		{
			return 0;
		}
		DBG_ASSERT(o->_symtab[idx].st_name < get_sechdr(o,sec_strtab)->sh_size);
		return o->_strtab+o->_symtab[idx].st_name;
}

INLINE char* ElfLinker::get_symname(ObjInfo* o,Elf32_Sym *sym) {
		DBG_ASSERT(sym->st_name < get_sechdr(o,sec_strtab)->sh_size);
		if (!o->_strtab)
		{
			return 0;
		}
		return o->_strtab+sym->st_name;
		}
INLINE FileInfo* ElfLinker::firstfile() { return FileInfo::first(); }
INLINE int ElfLinker::nfiles() { return FileInfo::_nfiles; }
INLINE ObjInfo* ElfLinker::firstobj() { return ObjInfo::first(); }
INLINE ObjInfo* ElfLinker::firstobj(int& i) { return ObjInfo::first(i); }
INLINE int ElfLinker::nobjs() { return ObjInfo::_nobjs; }
INLINE ObjInfo* ElfLinker::obj(int i) { return ObjInfo::find(i); }
INLINE LibInfo* ElfLinker::firstlib() { return LibInfo::first(); }
INLINE int ElfLinker::nlibs() { return LibInfo::_nlibs; }
INLINE LibInfo* ElfLinker::lib(int i) { return LibInfo::find(i); }
INLINE DllInfo* ElfLinker::firstdll() { return DllInfo::first(); }
INLINE int ElfLinker::ndlls() { return DllInfo::_ndlls; }
INLINE DllInfo* ElfLinker::dll(int i) { return DllInfo::find(i); }
#endif
#endif /* __ELF_L_INLINES_H__ */


