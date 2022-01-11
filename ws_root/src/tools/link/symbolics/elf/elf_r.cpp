/*  @(#) elf_r.cpp 96/07/25 1.42 */

//====================================================================
// elf_r.cpp  -  ElfReader class defs for reading ELF files and 
//				 adding symbols to the network

#define __ELF_R_CP__

#ifndef USE_DUMP_FILE
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "utils.h"
#endif /* USE_DUMP_FILE */

#include "debug.h"
#include "dwarf.h"
#include "elf.h"
#include "elf_r.h"

#ifdef __3DO_DEBUGGER__
#define kProgressString1 "\pRead Debug, Resolve Types, Read Line Table"
#define kProgressString2 "\pReading Line Table"
#define kProgressString3 "\pReading Debug"
#else
#define kProgressString1 ""
#define kProgressString2 ""
#define kProgressString3 ""
#endif

//routines called by quick sort
extern "C" { 
static int cmpsym_adr(const void *A, const void *B);
static int cmpsym_sec(const void *A, const void *B);
}

#pragma segment elfsym

//==========================================================================
// constructors, etc
// constructor - takes filename as input abd builds symbol network
//Heap* ElfReader::_heap=0;

ElfReader::ElfReader(int dump)
{
    DBG_ENT("ElfReader");
	_symff = eELF;
    _dumping=dump;  //default is 0
	//_heap = SymNet::_heap;
    _elf_hdr = 0;
    _sh_strtab = 0;
    _strtab = 0;
    _symtab = 0;
    _symtab_sorted_by_section = 0;
    _symtab_sorted_by_address = 0;
    _sec_hdr = 0;
	// NO jrm: it doesn't belong to us! _progress_bar = 0;
	}
	
//-------------------------------------------
SymErr ElfReader::ISymbolics(const StringPtr fnm)
//-------------------------------------------
{
	DBG_ENT("ISymbolics");
    char* file_name = (char*) MALLOC((unsigned char)fnm[0]+1);
    if (!file_name) return state();
    cname(file_name,fnm);
    _fp = (SymFile*) SYM_NEW(SymFile(file_name));
    if (!_fp)
	    SET_ERR(se_fail,("No memory to allocate SymFile object '%s'\n", file_name))
    ISymbolics();
    FREE(file_name);
    return state();
} // ElfReader::ISymbolics(const StringPtr)
	
//-------------------------------------------
SymErr ElfReader::ISymbolics()
//-------------------------------------------
{
	if (!_progress_bar) {
		_progress_bar = (TProgressBar*) NEW(TProgressBar);
		if (!_progress_bar) {
			RETURN_ERR(se_malloc_err,("failed to create Progress Bar\n"));
			} 
	}
	_progress_bar->InitProgressDialog((StringPtr)kProgressString1,(StringPtr)"\0"); 
    _state.force_validate();
    if (!_fp->open()) 
	    SET_ERR(se_open_fail,("Can't open '%s' for input\n", _fp->filename()))
	else {
	    elf_hash_size = ELF_HASH_SIZE; //for dynamic loading, but I could use it too...
	    if (read_elf_hdr()!=se_success) {
		    DBG_ERR(("failed to read elf header!\n"));
			}
	    else {
			_sections = (Sections*) SYM_NEW(Sections(_elf_hdr->e_shnum));
			if (!_sections) {
				DBG_ERR(("failed to create sections!\n"));
				}
			else if (read_section_hdrs()!=se_success) {
					DBG_ERR(("failed to read section headers!\n"));
					}
				else if (_dumping!=DUMP_HEADERS_ONLY) {
					if (read_main_symbol_table()!=se_success) {
						DBG_WARN(("#Unable to read main symbol table!\n"));
						}
					else if (_dumping!=DUMP_FILE_ONLY) {
						if (read_debug()!=se_success) {
							DBG_WARN(("#Unable to read debug info!\n"));
							}
						else if (read_line_table()!=se_success) {
							DBG_WARN(("#Unable to read line table!\n"));
							}
						}
					}
			}
		}
	//#ifdef __3DO_DEBUGGER__	
	//	//add special register symbols to symbol network
	//	SymType* _hwreg_type;
	//	_hwreg_type = add_type("hwreg",4,tc_reg,0);	//first time
	//   	SymEntry* s = add_sym(REG_NAME_PC,_hwreg_type,REG_PC,sec_none,scope_global,sc_reg);
	//#endif /* __3DO_DEBUGGER__ */
	_progress_bar->EndProgressDialog();

    _fp->close();
    return state();
} // ISymbolics()
	
//-------------------------------------------
ElfReader::~ElfReader()
//-------------------------------------------
{
    DBG_ENT("~ElfReader");
    if (_elf_hdr) FREE(_elf_hdr);
    if (_sh_strtab && _sh_strtab!=_strtab) FREE(_sh_strtab);
    if (_strtab) FREE(_strtab);
    if (_sec_hdr) FREE(_sec_hdr);
    if (_symtab) FREE(_symtab);
    if (_symtab_sorted_by_section) FREE(_symtab_sorted_by_section);
    if (_symtab_sorted_by_address) FREE(_symtab_sorted_by_address);
	// These members are in the base class, and should not be deleted
	// here (to do so, causes a crash) jrm 95/12/19.
	//if (_progress_bar) DELETE(_progress_bar);
	//_progress_bar = 0;
	// if (_fp) _fp->close();
	}

//==========================================================================
// readers

// reads elf header and figures out endianness.
//-------------------------------------------
SymErr ElfReader::read_elf_hdr()
//-------------------------------------------
{
    DBG_ENT("read_elf_hdr");
    Boolean target_is_little_endian;
	// elf_hdr is aligned so we can read the whole thing
    if (!_fp->seek(0,SEEK_SET))
	    RETURN_ERR(se_seek_err,("#Unable to seek to elf header\n"));
	_elf_hdr = (Elf32_Ehdr*) MALLOC(sizeof(Elf32_Ehdr));
	if (!_elf_hdr) return state();
	DBG_(ELF,("reading elf header at offset x%X of file %s\n",_fp->tell(),_fp->filename()));
    if (!_fp->read(_elf_hdr,sizeof(Elf32_Ehdr)))
	    RETURN_ERR(se_read_err,("#Unable to read elf header\n"));
	// check magic
    unsigned char *p = _elf_hdr->e_ident;
    if (memcmp(p,ELFMAG,ELFMAGLEN)!=0)
	    RETURN_ERR(se_fail,("\"%s\" is not an ELF file\n",_fp->filename()));

    target_is_little_endian = (Boolean)(_elf_hdr->e_ident[EI_DATA]==ELFDATA2LSB);
    set_endianness(target_is_little_endian);

	#define N(x) _elf_hdr->e_##x = swap(_elf_hdr->e_##x)
	    N(type); N(machine); N(version); N(entry);
	    N(phoff); N(shoff); N(flags); N(ehsize);
	    N(phentsize); N(phnum); N(shentsize); N(shnum);
	    N(shstrndx);
	#undef N
    return state();
	}

//-------------------------------------------
SymErr ElfReader::read_section_hdrs()
// Read all the section headers.  We read the sh_strtab here
// to so that we can look up the section header names.
// Once we've read the section headers, we look for the sections
// we're interested in so we can use them later.
//-------------------------------------------
{
    long i;
    DBG_ENT("read_section_hdrs");
    _sec_hdr = (Elf32_Shdr*)
	    MALLOC(_elf_hdr->e_shentsize*_elf_hdr->e_shnum);
    if (!_sec_hdr) return state();
    if (!_fp->seek(_elf_hdr->e_shoff,SEEK_SET))
	    RETURN_ERR(se_seek_err,("#Unable to seek to section headers at offset=x%X\n",_elf_hdr->e_shoff));
	// sec_hdr is aligned so we can read the whole thing
    if (!_fp->read(_sec_hdr,_elf_hdr->e_shentsize*_elf_hdr->e_shnum))
	    RETURN_ERR(se_read_err,("Error reading section header in '%s'.\n",_fp->filename()));
    Elf32_Shdr *p = _sec_hdr;
    for (i=0; i<_elf_hdr->e_shnum; i++, p++)
	{
		#define N(x) p->sh_##x = swap(p->sh_##x)
		    N(name); N(type); N(flags); N(addr); N(offset);
		    N(size); N(link); N(info); N(addralign); N(entsize);
		#undef N
	}
    if (read_string_table(_elf_hdr->e_shstrndx, _sh_strtab)!=se_success)
	{
		FREE(_sec_hdr);
	    _sec_hdr = 0;
	    RETURN_ERR(se_fail,("Error reading string table\n"));
	}
	// set up sections
    _sections->set_baseaddr(sec_shstrtab,_elf_hdr->e_shstrndx,_sec_hdr[_elf_hdr->e_shstrndx].sh_addr);
    EachSection es(this);
    while (es)
	{
		SymSec sec;
	    Elf32_Shdr *p = es.next(i);
	    char *s = _sh_strtab + p->sh_name;
	    DBG_(ELF,("Elf32_Shdr[%d]: name=%s, type=x%X, addr=x%X, offset=x%X, size=x%X\n",
				i,s,p->sh_type,p->sh_addr,p->sh_offset,p->sh_size));
	    if (s) {
			if ((sec=get_sectype(s))!=sec_none) {
			#ifdef _DDI_BUG	//this is non-svr4
			//DDI relocates .debug & .line
				if (p->sh_type==SHT_PROGBITS || !get_relocs(i))
			#else
				if (p->sh_type==SHT_PROGBITS)
			#endif
					_sections->set_baseaddr(sec,i,p->sh_addr);
				else
					_sections->set_baseaddr(sec,i,0);
				_sections->set_size(i,p->sh_size);
				}
			else
				SET_ERR(se_unknown_type,("unknown section found! name=%s)\n",s?s:"-none-"));
		}
	}
    return state();
} // ElfReader::read_section_hdrs()

//-------------------------------------------
uint8* ElfReader::read_section(unsigned long sidx)
//-------------------------------------------
{
    DBG_ENT("read_section");
    uint8* buf = 0;
    if (!sidx)
	{
	    SET_ERR(se_parm_err,("No section index for section index %d\n",sidx));
		return 0;
	}
    Elf32_Shdr *s = _sec_hdr + sidx;
    if (!s->sh_size)
	{
	    SET_INFO(se_not_found,("No contents for section index %d\n",sidx));
		return 0;
	}
    if (s->sh_flags & SHF_COMPRESS)
    {
	    SET_ERR(se_fail,("Section %d is compressed - unable to read\n",sidx));
		return 0;
    }
    if (!_fp->seek(s->sh_offset,SEEK_SET))
	{
	    SET_ERR(se_seek_err,("#Unable to seek to section at offset=x%X\n",s->sh_offset));
		return 0;
	}
    buf = (uint8*) MALLOC(s->sh_size);
    if (buf)
	{
		if (!_fp->read(buf,s->sh_size))
		{
		    FREE(buf);
		    SET_ERR(se_read_err,("Error reading section\n"));
			return 0;
		}
	}
    return buf;
} // ElfReader::read_section(unsigned long)
	
//-------------------------------------------
SymErr ElfReader::read_main_symbol_table()
//-------------------------------------------
{
    DBG_ENT("read_main_symbol_table");
	// Read main symbol table info
    uint32 nsyms = get_symtab_nsyms();
    uint32 strtab_size = sec_size(sec_strtab);
    if (!nsyms)
	    RETURN_INFO(se_no_symbols,("No main symbol table section entries!\n"));
    DBG_ASSERT(sec_shdr(sec_strtab));
    DBG_ASSERT(sec_size(sec_symtab));
    DBG_ASSERT(strtab_size);
    if (read_symbol_table(_sections->secnum(sec_symtab),_symtab,_strtab)!=se_success)
	    RETURN_ERR(se_fail,("failed to read main symbol table!\n"));
	if (_dumping==DUMP_HEADERS_ONLY || _dumping==DUMP_FILE_ONLY)
		return state();
	//read Value, Size, Bind, Type, Sect, Name
    SymModEntry* mod=0, *last_mod=0;
    SymFuncEntry* func=0;
    SymEntry* sym=0;
    SymbolClass sclass;
    SymScope scope;
    SymSec sec;
    uint32 value, size, secnum, st_type;
    const char* name;
    for (unsigned long i=1; i<nsyms;i++)
	{
		DBG_(ELF,("looping: i=x%X, nsyms=x%X\n",i,nsyms));
	    Elf32_Sym *p = _symtab+i;
		st_type = ELF32_ST_TYPE(p->st_info);
	    if (st_type==STT_OBJECT || st_type==STT_FUNC || st_type==STT_SECTION)
		{
			value = p->st_value;
			size = p->st_size;
			scope = st_bind(ELF32_ST_BIND(p->st_info));
			secnum = p->st_shndx;	//STT_FILEs & STT_NOTYPEs get bogus secnums of xfff1!
			DBG_ASSERT((unsigned)p->st_name < strtab_size);
			name = ((unsigned)p->st_name < strtab_size)?_strtab+p->st_name:0;
			sec = get_symsectype(i);
			sclass = get_symclass(sec);
			DBG_(ELF,("name=%s,value=x%X,size=x%X,scope=x%X,sclass=x%X,secnum=x%X,type=x%X\n",
				    name?name:"(null)",value,size,scope,sclass,secnum,st_type));
		}
	    else
		{
			DBG_WARN(("skipping name=%s,value=x%X,size=x%X,scope=x%X,secnum=x%X,type=x%X\n",
					((unsigned)p->st_name < strtab_size)?_strtab+p->st_name:"(null)",
				    p->st_value,p->st_size,st_bind(ELF32_ST_BIND(p->st_info)),p->st_shndx,st_type));
			continue;
		}
		// check type
		//ERROR. sec = get_symsectype(i) != get_sectype(section_name(secnum))
	    switch(st_type)
		{
		    case STT_SECTION: // secs (.text, etc...) these come first
					//set the base address for this section
				    DBG_(ELF,("STT_SECTION: name=%s, sectype=x%X, secnum=x%X, value=x%X\n",
							//section_name(secnum),get_sectype(section_name(secnum)),secnum,value));
							section_name(secnum),_sections->sectype(secnum),secnum,value));
					//already set in section headers...
					//_sections->set_baseaddr(get_sectype(section_name(secnum)),secnum,value);
				    break;
		    case STT_OBJECT: // globls
				    DBG_(ELF,("STT_OBJECT\n"));
					if (ELF32_ST_BIND(p->st_info)==STB_LOCAL) {
						sym = add_sym(name, 0, value, sec, scope_module, sclass);
						if (mod) add_msym(sym, mod);
						}
					else {
						sym = add_sym(name, 0, value, sec, scope_global, sclass);
						add_gsym(sym, mod);
						}
				    break;
		    case STT_FUNC: { // funcs
				    DBG_(ELF,("STT_FUNC\n"));
					uint32 n = _sections->secnum(sec); 
					uint32 b = _sections->baseaddr(_sections->secnum(sec)); 
					uint32 v = abs2rel(sec,value); 
				    sym = add_sym(name, 0, value, sec, scope, sclass);
				    sym->set_scope(scope);
				    func = add_func(sym, mod);
				    add_gfunc(func, mod);
				    func->set_baddr(abs2rel(sec_code,value));
				    func->set_eaddr(abs2rel(sec_code,value+size));
				    break;
					}
		    case STT_FILE:
				    DBG_(ELF,("STT_FILE\n"));
					//Definition varies from compiler to compiler...
					//This file is the whole sym file - not the modules within
					//we should be getting the section starting addrs next...
					//0x0    	   file abs      t1.c
					//0x20102e8     null .data    .L00DATA //data
					//0x2000098     null .text    bt..00 //text
					//if (mod) last_mod = mod; // save so we can close off eaddr later
					//mod = add_mod(name, 0, 0); //we'll fill in later
				    break;
		    case STT_NOTYPE:
				    DBG_(ELF,("STT_NOTYPE\n"));
					//if (mod && name && !mystrcmp(name,"bt..00")) {
					//    mod->set_baddr(value); //start addr for module
					//    last_mod->set_eaddr(value); //close off last module
					//	}
					//if (mod && name && !mystrcmp(name,"_etext"))
					//    mod->set_eaddr(value); //close off last module
					//if (mod && name && !mystrcmp(name,".L00DATA")) {
					//    mod->set_bdaddr(value); //don't care about data yet...
					//    last_mod->set_edaddr(value);
					//	}
					//if (mod && name && !mystrcmp(name,"_data"))
					//    mod->set_edaddr(value); //don't care about data yet...
				    break;
		    default:
			    SET_ERR(se_unknown_type,("unknown type x%lx\n",st_type));
			}
		}
	//FREE(_symtab); //??
	//FREE(_strtab);
    return state();
	}

//-------------------------------------------
SymErr ElfReader::read_string_table(unsigned long sidx, char *&strtab)
// Given the header index of a string table, read the string
// table into memory for quick processing later.
//
// Handle specially if this string table
// is the main symbol table's string table (_strtab),
// or the one referenced by the elf header (sh_strtab).
// SOMEDAY!  May just want to ignore whether this is a
// special case and let caller delete their own copy.
//-------------------------------------------
{
    DBG_ENT("read_string_table");
    strtab = 0;
    if (sidx == _elf_hdr->e_shstrndx && _sh_strtab != 0) {
	    strtab = _sh_strtab;
		return state();
		}
    if (_sections->secnum(sec_strtab) == sidx && _strtab) {
	    strtab = _strtab;
		return state();
		}
    if (sidx >= _elf_hdr->e_shnum) {
	    strtab = 0;
	    RETURN_ERR(se_fail,("Bad string_table section index\n"));
		}
	strtab = (char *) read_section(sidx);
    if (strtab) {
		if (sidx == _elf_hdr->e_shstrndx) {
			_sh_strtab = strtab;
			}
		}
    return state();
	}

//-------------------------------------------
SymErr ElfReader::read_symbol_table(unsigned long sidx,
	    Elf32_Sym *&symtab,  char *&strtab)
// Given the header index of a symbol table, read the symbol
// table into memory for quick processing later.
// We also read the string table belonging to the symbol
// table...
//
// Handle specially if this string table
// is the main symbol table's string table (main_strtab).
// or the one referenced by the elf header (sh_strtab).
// SOMEDAY!  May just want to ignore whether this is a
// special case and let caller delete their own copy.
//-------------------------------------------
{
    DBG_ENT("read_symbol_table");
// SOMEDAY.  should set this up for all symbol tables!
	Elf32_Shdr *s = &_sec_hdr[sidx];
    if (s == sec_shdr(sec_symtab) && _symtab != 0) {
	    symtab = _symtab;
	    strtab = _strtab;
	    return state();
		}
// SOMEDAY.
	unsigned long nsym;
    symtab = 0; strtab = 0;
	// read symbol table
    symtab = (Elf32_Sym *) read_section(sidx);
    if (!symtab) return state();
    nsym = get_symtab_nsyms(s);	// get the number of symbols in symtab
	// read symbol table's string table
    if (read_string_table(s->sh_link,strtab)!=se_success) {
	    if (strtab) FREE(strtab);
	    strtab=0;
	    DBG_ERR(("Error reading string table\n"));
	    return state();
		}
    if (swap_needed())
	    for (unsigned long i=0;i<nsym;i++) {
		    Elf32_Sym *p = symtab+i;
			#define N(x) p->st_##x = swap(p->st_##x)
			    N(name); N(value); N(size); N(shndx);
			#undef N
			}
    return state();
	}

// Reads dynamic data's string table, then the dynamic data
SymErr ElfReader::read_dynamic_data(unsigned long sidx,
		    Elf32_Dyn *&dyn, unsigned long &ndyn,
		    char *&strtab, unsigned long &nstr) {
    DBG_ENT("read_dynamic_data");
    dyn = 0; ndyn = 0; strtab = 0; nstr = 0;
	Elf32_Shdr *s = &_sec_hdr[sidx];
    if (read_string_table(s->sh_link,strtab)!=se_success)
	    RETURN_ERR(se_fail,("Error reading string table\n"));
    ndyn = get_symtab_nsyms(s);
    dyn = (Elf32_Dyn *) read_section(sidx);
    if (!dyn) return state();
	//size = s->sh_entsize*ndyn;
    for (unsigned long i=0;i<ndyn;i++){
	    Elf32_Dyn *p = dyn+i;
	    p->d_tag = swap(p->d_tag);
	    p->d_un.d_val = swap(p->d_un.d_val);
		}
    return state();
	}

// reads and sorts the symbol table
typedef int (*VCMP)(const void *, const void *);
SymErr ElfReader::read_symbol_table_sorted(unsigned long sidx,
	    Elf32_Sym **&by_section, Elf32_Sym **&by_address,
	    Elf32_Sym *&symtab, unsigned long &nsym,
	    char *&strtab, unsigned long &nstr) {
    DBG_ENT("read_symbol_table_sorted");
    by_section = by_address = 0;
	Elf32_Shdr *s = &_sec_hdr[sidx];
    if ((s == sec_shdr(sec_symtab)) && _symtab_sorted_by_address) {
	    by_address = _symtab_sorted_by_address;
	    by_section = _symtab_sorted_by_section;
		}
    if (read_symbol_table(sidx,symtab,strtab)!=se_success)
	    RETURN_ERR(se_fail,("Error reading symbol table\n"));
    nsym = get_symtab_nsyms(s);
    nstr = get_symtab_strtab(s)->sh_size;
    by_section = (Elf32_Sym**) MALLOC(sizeof(Elf32_Sym)*nsym);
    if (!by_section) return state();
    by_address = (Elf32_Sym**) MALLOC(sizeof(Elf32_Sym)*nsym);
    if (!by_address) return state();
    for (unsigned long i=0; i<nsym; i++)
		(by_section)[i] = (by_address)[i] = symtab+i;
    qsort(by_section,nsym,sizeof(Elf32_Sym*),cmpsym_sec);
    qsort(by_address,nsym,sizeof(Elf32_Sym*),cmpsym_adr);
    if (s == sec_shdr(sec_symtab)) {
	    _symtab_sorted_by_address = by_address;
	    _symtab_sorted_by_section = by_section;
		}
    return state();
	}

// Read line table info
SymErr ElfReader::read_line_table() {
    DBG_ENT("read_line_table");
    Elf32_Shdr* s = sec_shdr(sec_line);
    if (!s)
	   RETURN_INFO(se_no_line_info,("No line sections!\n"));

    SymModEntry* cur_mod=0;
    Elf32_Lhdr line_hdr;
    uint32 bytes_read = 0;
    DBG_ASSERT(s);
    if (!_fp->seek(s->sh_offset,SEEK_SET))
	    RETURN_ERR(se_seek_err,("#Unable to seek to line table at offset=x%X\n",s->sh_offset));
	// read line table
	_progress_bar->ClearProgress();
	_progress_bar->ChangeStatus((StringPtr) kProgressString2);
    while (bytes_read < s->sh_size) {
	// line_hdr is aligned so we can read the whole thing
		if (!_progress_bar->UpdateProgress(bytes_read, s->sh_size))
			RETURN_ERR(se_fail,("User Abort\n"));
	    if (!_fp->read(&line_hdr,8))
		    RETURN_ERR(se_read_err,("Error reading line header\n"));
	    bytes_read += 8;
	    if (swap_needed()) {
		 	line_hdr.lh_size=swap(line_hdr.lh_size);
			line_hdr.lh_svaddr=swap(line_hdr.lh_svaddr);
			}
	    int32 lines_size = line_hdr.lh_size-8; //size includes header
	    if (lines_size>0) {
			DBG_(ELF,("line_hdr.lh_size=x%X, line_hdr.lh_svaddr=x%X\n",line_hdr.lh_size, line_hdr.lh_svaddr));
			cur_mod = find_mod(line_hdr.lh_svaddr);
			DBG_ASSERT(cur_mod);
			if (!cur_mod)
				RETURN_ERR(se_fail,("no module found for lines at svaddr=x%X\n",line_hdr.lh_svaddr));
			char *mod_lines = (char*) MALLOC(lines_size);
			if (!mod_lines) return state();
			DBG_ASSERT(mod_lines);
			if (!_fp->read(mod_lines,lines_size)) {
			    FREE(mod_lines);
			    RETURN_ERR(se_read_err,("Error reading lines entries for lines at svaddr=x%X\n",line_hdr.lh_svaddr));
				}
			bytes_read += lines_size;
			char* lines_ptr;
			unsigned long ln_num;
			unsigned long ln_addr;
			// the middle short word is for a character offset, but tools vendors ignore it
			for (lines_ptr = mod_lines; lines_ptr < (mod_lines+lines_size);
					lines_ptr += SIZEOF_LINE) {
					// alignment is 16 bits so must handle specially
 					// more efficient to move bytes... ?
				memcpy(&ln_num,lines_ptr,4);
				memcpy(&ln_addr,lines_ptr+6,4);
				if (swap_needed()) {
					ln_num=swap(ln_num);	  // line number of module
					ln_addr=swap(ln_addr);	 // code offset from start of module
					}
				//hack. out of order, and Diab puts 0 as the end of the
				//last function, so must save last line number
				static int last_line = 0;
				if (ln_num==0) ln_num = last_line+1;
				last_line = ln_num;

			    add_line(cur_mod,ln_num,ln_addr+line_hdr.lh_svaddr);
				DBG_(ELF,("ln_num=x%X, ln_addr=x%X\n",ln_num,ln_addr));
				}
			if (mod_lines) FREE(mod_lines);
			}
		}
    return state();
	}

SymErr ElfReader::read_debug() {
    DBG_ENT("read_debug");
	// Read dwarf debug info
    Elf32_Shdr* s = sec_shdr(sec_debug);
    DBG_ASSERT(s);
    if (!s)
 		RETURN_INFO(se_no_debug_info,("No debug section!\n"));

    _cur_mod=0;	//init scope for parsing debug data
    _cur_func=0;
    _cur_block=0;
	_cur_type = (TypesStack*) NEW(TypesStack);
	if (!_cur_type) RETURN_ERR(se_malloc_err,("#Unable to create type stack\n"));
    unsigned char* buf;
    uint32 bytes_read = 0;
    uint32 buf_size;
	uint32 size_allocated = 0;
	uint32 n = 0;
    uint32 debug_base = sec_shdr(sec_debug)->sh_offset;
	unsigned char *p;
	Boolean needSwap;

	#ifdef _DDI_BUG	//this is a temp hack (I hope) 
		extern void set_debug_base(uint32 base);
		//don't do if have .rela.debug (in which case offsets will be relative)
		if (_sections->secnum(sec_reladebug))
			set_debug_base(0);
		else
			set_debug_base(sec_shdr(sec_debug)->sh_addr);
			//set_debug_base(_sections->baseaddr(_sections->secnum(sec_debug)));
		DBG_(ELF,("BUG!!  Diab Data's ref should be offset from debug section!\n"));
	#endif
    if (!_fp->seek(s->sh_offset,SEEK_SET))
	    RETURN_ERR(se_seek_err,("Unable to seek to debug section at offset=x%X\n",s->sh_offset));
	_progress_bar->ChangeStatus((StringPtr) kProgressString3);

	// ------------------------
	// create buffer for symbol table
	buf = (unsigned char*) MALLOC(s->sh_size);
	if (!buf)
		RETURN_ERR(se_malloc_err,("buf allocation failed\n"));
		
	elf_offset = _fp->tell() - debug_base;	//save to use for type referencing
	
	// read entire table into buffer
	if (!_fp->read(buf,s->sh_size)) {
		FREE(buf);
		RETURN_ERR(se_read_err,("Error reading debug table\n"));
	}
	n = 0;
	p = buf;
	needSwap = swap_needed();
	while (bytes_read < s->sh_size) {
		elf_offset += n;
		// get size
		buf_size = *(uint32*)p;
		if(needSwap)
			swapit(buf_size);
		bytes_read += 4;
		n = 4;
		p += n;
	    buf_size -= 4;
		// get entry
	    if(buf_size > 0) {
		    BufEaters buf_obj((Endian*)this,buf_size,p);	//stream handler for buf
		   	build_syms(&buf_obj);
		    bytes_read += buf_size;
		    n += buf_size;
			p += buf_size;
		}
	    else {
			build_syms(0);
			if (!_progress_bar->UpdateProgress(bytes_read, s->sh_size)) {
				RETURN_ERR(se_fail,("User Abort\n"));
			}
		}
	}
    FREE(buf);
	// ------------------------

	if (!resolve_ref_types())
		SET_ERR(se_not_found,("resolve_ref_types failed!  Some references could not be resolved\n"));
	if (_cur_type) {
		if (!_cur_type->empty()) {
			SET_ERR(se_unknown_format,("one or more debug sections were not terminated\n"));
			while (!_cur_type->empty()) {
				DBG_ERR(("type cat x%X at offset x%X not terminated\n",_cur_type->cat(),_cur_type->file_offset()));
				_cur_type->pop();
				}
			}
		DBG_ASSERT(_cur_type->empty());
		DELETE(_cur_type);
		}
    return state();
}
	
//-------------------------------------------
SymErr ElfReader::build_syms(BufEaters* buf)
// main function for building symbols 
//		reads debug tag and entry and calls sub-functions
//		based on TAGs to parse debug entry
//-------------------------------------------
{
    DBG_ENT("build_syms");
	static uint16 last_tag = 0;
	if (!buf || buf->bites_left() < 4)
	{
		//already read 4
		if (buf) buf->eat_nbites(buf->bites_left());
		//null entry - means we're at the end of a list of things
		//if we had any open structs/unions/enums, close them now...
		//if (cur_type struct_or_union || cur_enum) {
		DBG_ASSERT(_cur_type);
		DBG_(SYMS,("null entry!\n"));
		if (!_cur_type->empty())
		{
			//kludge to avoid having to make a big change -
			//will SOMEDAY get rid o this and use types stack only
			//was: if (_cur_func && (!_cur_block || _cur_block==_cur_func)) { //pop the function
			if (_cur_func && _cur_type->isa(tc_function) 
				&& (!_cur_block || _cur_block==_cur_func))
			{
				//pop the function
				DBG_FIXME(("kludge - popping function %s\n",_cur_func->name()));
				_cur_func = 0;
				_cur_block = 0;
			}
			DBG_WARN(("popping _cur_type\n"));
			_cur_type->pop();
		}
		return state() ; 
	}
    DBG_DUMP(buf->bites_left(),buf->ptr());
    uint16 tag = buf->eat_uint16();	//buf must be aligned
	//for TAGs which would normally have a list but don't,
	//we need to pop them since we're not going to be getting
	//a null entry to end the list
	//if (last_tag==TAG_structure_type && tag!=TAG_member
	//	|| last_tag==TAG_subroutine_type && tag!=TAG_formal_parameter) {
	//that didn't work... let's try this
	if (!_cur_type->empty() && elf_offset==_cur_type->sibling())
	{
		_cur_type->pop();
	}
	last_tag = tag;

	//===================================
    switch (tag) {
		case  TAG_padding:
			DBG_(ELF,("TAG_padding\n"));
			break;
		case  TAG_array_type:
			DBG_(ELF,("TAG_array_type\n"));
			build_array_type(buf);
			break;
		case  TAG_class_type:
			DBG_(ELF,("TAG_class_type\n"));
			build_class_type(buf);
			break;
 		case  TAG_entry_point:
			DBG_(ELF,("TAG_entry_point\n"));
			build_entry_point(buf);
			break;
		case  TAG_enumeration_type:
			DBG_(ELF,("TAG_enumeration_type\n"));
			build_enumeration_type(buf);
			break;
 		case  TAG_formal_parameter:
			DBG_(ELF,("TAG_formal_parameter\n"));
			build_formal_parameter(buf);
			break;
		case  TAG_global_subroutine:
			DBG_(ELF,("TAG_global_subroutine\n"));
			build_global_subroutine(buf);
			break;
		case  TAG_global_variable:
			DBG_(ELF,("TAG_global_variable\n"));
			build_global_variable(buf);
			break;
		case  TAG_label:
			DBG_(ELF,("TAG_label\n"));
			build_label(buf);
			break;
		case  TAG_lexical_block:
			DBG_(ELF,("TAG_lexical_block\n"));
			build_lexical_block(buf);
			break;
		case  TAG_local_variable:
			DBG_(ELF,("TAG_local_variable\n"));
			build_local_variable(buf);
			break;
		case  TAG_member:
			DBG_(ELF,("TAG_member\n"));
			build_member(buf);
			break;
		case  TAG_pointer_type:
			DBG_(ELF,("TAG_pointer_type\n"));
			build_pointer_type(buf);
			break;
		case  TAG_reference_type:
			DBG_(ELF,("TAG_reference_type\n"));
			build_reference_type(buf);
			break;
		case  TAG_compile_unit:	//module!!
			DBG_(ELF,("TAG_compile_unit\n"));
			build_compile_unit(buf);
			break;
		case  TAG_string_type:
			DBG_(ELF,("TAG_string_type\n"));
			SET_ERR(se_fail,("string_types not supported.\n"));
			break;
		case  TAG_structure_type:
			DBG_(ELF,("TAG_structure_type\n"));
			build_structure_type(buf);
			break;
		case  TAG_subroutine:
			DBG_(ELF,("TAG_subroutine\n"));
			build_subroutine(buf);
			break;
		case  TAG_subroutine_type:
			DBG_(ELF,("TAG_subroutine_type\n"));
			build_subroutine_type(buf);
			break;
		case  TAG_typedef:
			DBG_(ELF,("TAG_typedef\n"));
			build_typedef(buf);
			break;
		case  TAG_union_type:
			DBG_(ELF,("TAG_union_type\n"));
			build_union_type(buf);
			break;
		case  TAG_unspecified_parameters:
			DBG_(ELF,("TAG_unspecified_parameters\n"));
			build_unspecified_parameters(buf);
			break;
		case  TAG_variant:
			DBG_(ELF,("TAG_variant\n"));
			SET_ERR(se_fail,("variants not supported.\n"));
			break;
		case  TAG_common_block:
			DBG_(ELF,("TAG_common_block\n"));
			build_common_block(buf);
			break;
		case  TAG_common_inclusion:
			DBG_(ELF,("TAG_common_inclusion\n"));
			build_common_inclusion(buf);
			break;
		case  TAG_inheritance:
			DBG_(ELF,("TAG_inheritance\n"));
			build_inheritance(buf);
			break;
		case  TAG_inlined_subroutine:
			DBG_(ELF,("TAG_inlined_subroutine\n"));
			build_inlined_subroutine(buf);
			break;
		case  TAG_module:
			DBG_(ELF,("TAG_module\n"));
			build_module(buf);
			break;
		case  TAG_ptr_to_member_type:
			DBG_(ELF,("TAG_ptr_to_member_type\n"));
			build_ptr_to_member_type(buf);
			break;
		case  TAG_set_type:
			DBG_(ELF,("TAG_set_type\n"));
			build_set_type(buf);
			break;
 		case  TAG_subrange_type:
			DBG_(ELF,("TAG_subrange_type\n"));
			build_subrange_type(buf);
			break;
		case  TAG_with_stmt:
			DBG_(ELF,("TAG_with_stmt\n"));
			SET_ERR(se_fail,("with statements not supported.\n"));
			break;
		default:
	   		SET_ERR(se_unknown_type,("unknown TAG=x%X\n",tag));
		}
	return state();
	}


//==========================================================================
// utilities

// This hash function was taken from the AT&T UNIX SVR4 Programmer's Guide.
// Intended for quick access to symbols if dynamically loaded...  we might
// want to use this later...
unsigned long ElfReader::elf_hash(const char *name) {
    unsigned long h = 0, g;
    const unsigned char *cp = (const unsigned char *)name;
    while (*cp){
	    h = (h<<4) + *cp++;
	    g = h & 0xF0000000;
	    if (g) h ^= g>>24;
	    h &= ~g;
	}
    return h % elf_hash_size;
}

//-------------------------------------------
char* ElfReader::symstr(unsigned long i)
// given sh_strtab offset, returns string
//-------------------------------------------
{
	#ifdef DEBUG
	int sec = sec_shstrtab;
	int sn = _sections->secnum(sec_shstrtab);
	int s = sec_size(sec_shstrtab);
	#endif
    if (i < sec_size(sec_shstrtab)) return _sh_strtab+i;
    SET_ERR(se_fail,("index out of range x%lx\n",i));
    return "???";
}

SymScope ElfReader::st_bind(int i)
{
    switch (i)
    {
	    case STB_LOCAL: return scope_local;
	    case STB_GLOBAL: return scope_global;
	    case STB_WEAK: return scope_global;
	    default:
		    SET_ERR(se_unknown_type,("unknown binding x%lx\n",i));
		    return scope_none;
	}
}

SymbolClass ElfReader::get_symclass(SymSec sec)
{
   switch (sec)
   {
   		case sec_code: return sc_code;
   		case sec_init: return sc_code;
   		case sec_fini: return sc_code;
		case sec_rodata: return sc_data;
   		case sec_data: return sc_data;
   		case sec_bss: return sc_bss;
   		case sec_sdata: 
   		case sec_sbss: 
   		case sec_line: 
   		case sec_debug: 
   		case sec_symtab: 
   		case sec_strtab: 
   		case sec_shstrtab: 
   		case sec_relatext: 
   		case sec_relainit: 
   		case sec_relafini: 
   		case sec_relarodata: 
   		case sec_reladata:
   		case sec_relasdata:
   		case sec_reladebug:
   		case sec_relaline: 
   		case sec_abs: 
   		case sec_com: 
   		case sec_und: 
   		case sec_none: 
		//3do additions
		//Provisional. what should these be?
		case sec_hdr3do:
		case sec_imp3do:
		case sec_exp3do:
   			return sc_none;
		default: 
			SET_ERR(se_unknown_type,("unknown sectype=x%X\n",sec));
			return sc_none;
	}
}

//-------------------------------------------
SymSec ElfReader::get_symsectype(uint32 idx)
//-------------------------------------------
{
	uint32 secnum = _symtab[idx].st_shndx;
    switch(secnum)
    {
	    case SHN_ABS: return sec_abs;
	    case SHN_COMMON: return sec_com;
	    case SHN_UNDEF: return sec_und;
	}
	DBG_ASSERT(idx<get_symtab_nsyms());
	return (_sections->sectype(secnum));
}
	
SymSec ElfReader::get_sectype(char* name) {
	int sectype;
	for (sectype = 1; sectype<SEC_NUM; sectype++) {
		if (!mystrcmp(name,get_secname((SymSec)sectype))) 
			return (SymSec)sectype;
		}
	SET_ERR(se_unknown_type,("unknown secname=%s\n",name));
	return sec_none;
	}
	
char* ElfReader::get_secname(SymSec sectype) {
	switch (sectype) {
		case sec_code: return ".text";
		case sec_rodata: return ".rodata";
		case sec_data: return ".data";
		case sec_bss: return ".bss";
		case sec_sdata: return ".sdata";
		case sec_sbss: return ".sbss";
		case sec_line: return ".line";
		case sec_debug: return ".debug";
		case sec_init: return ".init";
		case sec_fini: return ".fini";
		case sec_symtab: return ".symtab";
		case sec_strtab: return ".strtab";
		case sec_shstrtab: return ".shstrtab";
		case sec_abs: return "abs";
		case sec_com: return "com";
		case sec_und: 
			DBG_WARN(("file contains undefined symbols!\n"));
			return "und";
		case sec_relatext: return ".rela.text";
		case sec_relainit: return ".rela.init";
		case sec_relafini: return ".rela.fini";
		case sec_relarodata: return ".rela.rodata";
		case sec_reladata: return ".rela.data";
		case sec_relasdata: return ".rela.sdata";
		case sec_reladebug: return ".rela.debug";
		case sec_relaline: return ".rela.line";
		//3do additions
		case sec_hdr3do: return ".hdr3do";
		case sec_imp3do: return ".imp3do";
		case sec_exp3do: return ".exp3do";
		case sec_none: 
			DBG_WARN(("null section type\n"));
			return "???";
		default:
			SET_ERR(se_unknown_type,("unknown sectype=x%X\n",sectype));
			return "???";
		}
	}

//-------------------------------------------
uint32 ElfReader::find_sym_by_val(int secnum,uint32 val)
//given value, find symbol index for matching symbol
//SOMEDAY - would be better to have class functions for ObjInfo
//so wouldn't have to repeat code for linker
//-------------------------------------------
{
	uint32 symval;
	int symsecnum;
	uint8 symbind;
	int nsyms = get_symtab_nsyms();
	DBG_ASSERT(_symtab);
	for (int i=0; i<nsyms; i++)
	{
		symval = _symtab[i].st_value;
		symsecnum = _symtab[i].st_shndx;
		symbind = ELF32_ST_BIND(_symtab[i].st_info);
		if (val == symval && secnum == symsecnum
				&& symbind == STB_GLOBAL)
		{
			return i;
		}
	}
	return 0;
}

//-------------------------------------------
int ElfReader::get_relocs(int s)
//if section s has relocations, return secnum of relocations to apply
//-------------------------------------------
{
    Elf32_Shdr *r;
	for (int i=1; i<_elf_hdr->e_shnum; i++) {
		r=_sec_hdr+i;
		//in EABI, all relocations are rela!
		if (r->sh_info==s) {
			if (RELA(r) || REL(r))
				return i;
			}
		}
	return 0;
	}

// Given the starting virtual address of the module, find the
// module
SymModEntry* ElfReader::find_mod(uint32 svaddr) {
    SymModEntry* m;
    m = search_inmods(svaddr);
    if (!m) {
	    SET_ERR(se_fail,("couldn't find module for svaddr=x%X!\n",svaddr));
	    m = add_mod(0,svaddr,svaddr+0x1000); //we'll fix these up later
		}
    return m;
	}

//
char* ElfReader::get_symname(uint32 symidx) {
	int nsyms = get_symtab_nsyms();
	if (symidx >= nsyms) {
		SET_ERR(se_fail,("symidx x%X is out of range\n",symidx));
		return "";
		}
	Elf32_Sym *s = &_symtab[symidx]; 
	uint32 len = sec_size(sec_strtab);
	return symname(s, _strtab, len);
	}
char* ElfReader::symname(Elf32_Sym *s, char *strtab, unsigned long len) {
    char *p = s->st_name<len?strtab+s->st_name:"???";
    if (*p) return p;
    if (ELF32_ST_TYPE(s->st_info) == STT_SECTION &&
	   (unsigned)s->st_shndx < _elf_hdr->e_shnum)
	    return fmt_str("[%.80s]",symstr(_sec_hdr[s->st_shndx].sh_name));
    return "";
	}

// Given section index returns the section name
char* ElfReader::section_name(unsigned long i) {
    switch(i) {
	    case SHN_ABS: return "abs";
	    case SHN_COMMON: return "com";
	    case SHN_UNDEF: return "und";
		}
    if (i<_elf_hdr->e_shnum) {
		DBG_(PICKY,("_sec_hdr[i=%d].sh_name=%d, _elf_hdr->e_shnum=%d\n",i,_sec_hdr[i].sh_name,_elf_hdr->e_shnum));
		return symstr(_sec_hdr[i].sh_name);
		}
	SET_ERR(se_unknown_type,("unknown section %d\n",i));
    return "-none-";
	}


// Binary search on sorted main symbol table by address
Elf32_Sym* ElfReader::lookup_in(long sidx, unsigned long address,
	    long lo, long hi) {
    if (_symtab_sorted_by_section == 0) return NULL;
    Elf32_Sym *s;
    if (lo > hi) return NULL;
    if (lo==hi) {
	    s = _symtab_sorted_by_section[lo];
	    if ( s->st_shndx == sidx && s->st_value <= address)
		    return s;
	    return NULL;
		}
    long mid = (hi+lo)/2;
    s = _symtab_sorted_by_section[mid];
    if ( sidx < s->st_shndx  ||
		 sidx == s->st_shndx &&  s->st_value > address)
		{
	    return lookup_in(sidx,address,lo,mid-1);
		}
    else
    if ( s->st_shndx == sidx){
	    if (s->st_value == address){
			/* Don't return section ref */
		    if (s->st_name==0)
			    if (mid<hi){
				    Elf32_Sym *gs = _symtab_sorted_by_section[mid+1];
				    if (gs->st_shndx == sidx && gs->st_value == address)
					    return gs;
					}
			    else if (mid > lo){
				    Elf32_Sym *gs = _symtab_sorted_by_section[mid-1];
				    if (gs->st_shndx == sidx && gs->st_value == address)
					    return gs;
					}
		    return s;
			}
	    if (mid<hi &&
			(_symtab_sorted_by_section[mid+1]->st_shndx != sidx ||
		    _symtab_sorted_by_section[mid+1]->st_value > address))
		    return s;
	    else
		    return lookup_in(sidx,address,mid+1,hi);
		}
    else
	    return lookup_in(sidx,address,mid+1,hi);
	}

// Calls lookup_in to do a binary search for a symbol by address.
// If approximation is necessary, return difference in off.
Elf32_Sym* ElfReader::lookup_symbol(unsigned long sidx, unsigned long address,
	    unsigned long &off) {
    if (_symtab==0) return NULL;

    address += _sec_hdr[sidx].sh_addr;  // Convert to virtual address
    Elf32_Sym *s = lookup_in(sidx,address,0,get_symtab_nsyms()-1);
    if (s) off = address-s->st_value;
    return s;
	}

// Binary search for symbol by address
Elf32_Sym* ElfReader::lookup_adr_in(unsigned long address, long lo, long hi) {
    if (_symtab_sorted_by_address == 0) return NULL;
    Elf32_Sym *s;
    if (lo > hi) return NULL;
    if (lo==hi) {
	    s = _symtab_sorted_by_address[lo];
	    if (s->st_value <= address)
		    return s;
	    return NULL;
		}
    long mid = (hi+lo)/2;
    s = _symtab_sorted_by_address[mid];
    if ( address <= s->st_value) {
	    if ( s->st_value == address){
			/* Don't return section ref */
		    if (s->st_name==0)
			    if (mid<hi){
				    Elf32_Sym *gs = _symtab_sorted_by_address[mid+1];
				    if (gs->st_value == address)
					    return gs;
					}
			    else if (mid > lo){
				    Elf32_Sym *gs = _symtab_sorted_by_address[mid-1];
				    if ( gs->st_value == address)
					    return gs;
					}
		    return s;
			}
	    return lookup_adr_in(address,lo,mid-1);
		}
    else
    if (mid == hi || _symtab_sorted_by_address[mid+1]->st_value > address)
	    return s;
    else
	    return lookup_adr_in(address,mid+1,hi);
	}

// Calls lookup_adr_in to do a binary search for a symbol by address.
// If approximation is necessary, return difference in off.
Elf32_Sym* ElfReader::lookup_address(unsigned long address, unsigned long &off){
    Elf32_Sym *s = lookup_adr_in(address,0,get_symtab_nsyms()-1);
    if (s) off = address-s->st_value;
    return s;
	}
Boolean ElfReader::isa_dll() {
	int secnum = _sections->secnum(sec_exp3do);
	return (Boolean)(secnum && (_sec_hdr[secnum].sh_size>0 || _elf_hdr->e_type==ET_DYN));
	}

extern "C" { 
// compares symbols by address within section header
static int cmpsym_sec(const void *A, const void *B){
    const Elf32_Sym *a = *(const Elf32_Sym **) A;
    const Elf32_Sym *b = *(const Elf32_Sym **) B;
    if (a->st_shndx < b->st_shndx) return -1;
    if (a->st_shndx == b->st_shndx){
	    if (a->st_value < b->st_value) return -1;
	    if (a->st_value == b->st_value){
		    if (a->st_name == 0){
			    if (b->st_name != 0) return -1;  // Put nameless first
				}
		    else if (b->st_name == 0) return 1;
		    return 0;
			}
		}
    return 1;
	}

// compares symbols by address
static int cmpsym_adr(const void *A, const void *B){
    const Elf32_Sym *a = *(const Elf32_Sym **) A;
    const Elf32_Sym *b = *(const Elf32_Sym **) B;
    if (a->st_value < b->st_value) return -1;
    if (a->st_value == b->st_value) {
	    if (a->st_name == 0){
		    if (b->st_name != 0) return -1;  // Put nameless first
			}
	    else if (b->st_name == 0) return 1;
	    return 0;
		}
    return 1;
	}
}

//==========================================================================
// defs for class TypesStack

void TypesStack::push(SymType* type,uint32 off,uint32 ref) {
	DBG_ENT("push");
	_type_ptr* type_ptr = (_type_ptr*) NEW(_type_ptr(type,off,ref));
	push(type_ptr);
	}
void TypesStack::push(SymFuncEntry* ftype,uint32 off,uint32 ref) {
	DBG_ENT("push");
	_type_ptr* type_ptr = (_type_ptr*) NEW(_type_ptr(ftype,off,ref));
	push(type_ptr);
	}
void TypesStack::push(SymModEntry* mtype,uint32 off,uint32 ref) {
	DBG_ENT("push");
	_type_ptr* type_ptr = (_type_ptr*) NEW(_type_ptr(mtype,off,ref));
	push(type_ptr);
	}
void TypesStack::push(SymStructType* stype,uint32 off,uint32 ref) {
	DBG_ENT("push");
	_type_ptr* stype_ptr = (_type_ptr*) NEW(_type_ptr(stype,off,ref));
	push(stype_ptr);
	}
void TypesStack::push(SymEnumType* etype,uint32 off,uint32 ref) {
	DBG_ENT("push");
	_type_ptr* etype_ptr = (_type_ptr*) NEW(_type_ptr(etype,off,ref));
	push(etype_ptr);
	}
void TypesStack::push(_type_ptr* type) {
	Stack::push((uint32)_cur_type);	//push the old cur_type down on the stack
	_cur_type = type;
}

//-------------------------------------------
void TypesStack::pop()
//-------------------------------------------
{
	DBG_ENT("pop");
	DBG_ASSERT(_cur_type);
	add_members();
	DBG_ASSERT(!_cur_type->_queue_o_members || _cur_type->_queue_o_members->empty());
	DELETE(_cur_type);
	uint32 t;
	Stack::pop(t);
	_cur_type = (_type_ptr*) t;
}

//-------------------------------------------
void TypesStack::add_member(uint32 val, SymType* type)
//-------------------------------------------
{
	//Diab Data doesn't emit special members for enums -
	//only the enum name is emitted as a typedef of int/whatever;
	//we don't get any members with names...  no biggie... 
	switch (_cur_type->_kind)
	{
		case tc_struct:
		case tc_union:
		case tc_enum:
			DBG_ASSERT(_cur_type->_queue_o_members);
			_cur_type->_queue_o_members->add(val,type);
			break;
		default:
			DBG_ERR(("can't add members to non-struct/enum types\n"));
	}
}

//-------------------------------------------
void TypesStack::add_members()
//-------------------------------------------
{
	//if we're done eating members, time to add them to the type
	//SOMEDAY.  should get rid of _cur_func, etc and access everything
	//through _cur_type!
	uint32 offset;
	char* name=0;
	SymType* type=0;
	switch (_cur_type->_kind) {
		case tc_struct:
		case tc_union: {
			DBG_ASSERT(_cur_type->_queue_o_members);
			if (_cur_type->_queue_o_members->empty()) {
				DBG_WARN(("no stack_o_members to add! (could be a forward declaration for an external structure)\n")); 
				}
			else {
				SymStructType* stype = _cur_type->_type._struct_or_union;
				DBG_ASSERT(stype && stype->type());
				DBG_(ELF,("adding members to stype %s\n",stype->type()->name()?stype->type()->name():"(null)"));
				stype->set_nfields(_cur_type->_queue_o_members->num()); //if struct
				while (!_cur_type->_queue_o_members->empty()) {
					uint32 t;
					_cur_type->_queue_o_members->rmv(offset,t);
					type = (SymType*) t;
					stype->add_field(offset,type); //if struct
					}
				}
			DBG_ASSERT(_cur_type->_queue_o_members->empty());
			}
			break;
		case tc_enum: {
			DBG_ASSERT(_cur_type->_queue_o_members);
			if (_cur_type->_queue_o_members->empty()) {
				DBG_ERR(("no stack_o_members to add members to!\n"));
				return;
				}
			SymEnumType* etype = _cur_type->_type._enum;
			DBG_ASSERT(etype && etype->type());
			DBG_(ELF,("adding members to etype %s\n",etype->type()->name()?etype->type()->name():"(null)"));
			etype->set_nmembers(_cur_type->_queue_o_members->num()); //if enum
			while (!_cur_type->_queue_o_members->empty()) {
				uint32 t;
				_cur_type->_queue_o_members->rmv(offset,t);
				type = (SymType*) t;
				etype->add_member(offset,type); //if enum
				}
			DBG_ASSERT(_cur_type->_queue_o_members->empty());
			}
			break;
		case tc_function: {
			SymFuncEntry* ftype = _cur_type->_type._func;
			DBG_ASSERT(ftype && ftype->sym());
			}
		case tc_void: {
			SymModEntry* mtype = _cur_type->_type._mod;
			DBG_ASSERT(mtype);
			}
			break;
		default:
			DBG_(SYMS,("Nothing to do for this type\n"));
		}
	}

//-------------------------------------------
Elf32_Shdr* EachSection::next()
//-------------------------------------------
{
    Elf32_Shdr *p = _sec_hdr+i;
    i++;
    return p;
} // Elf32_Shdr* EachSection::next()

//-------------------------------------------
Elf32_Shdr* EachSection::next(long &x)
//-------------------------------------------
{
    x = i;
    return next();
} // EachSection::next(long &x)

//-------------------------------------------
Elf32_Shdr* EachSectionByOffset::next()
//-------------------------------------------
{
	unsigned long nextOffset = 0xffffffff;
	long nextIndex = 0;
		// convert it to the actual previous value.
		// (it's stored as one more, because of the way
		// dawn tests using operator int())
	// Find the smallest offset larger than previous one.
    for (Elf32_Shdr* p = _sec_hdr + 1; p - _sec_hdr < n; p++)
	{
		if (p->sh_offset < fCurrentOffset)
			continue; // we already did that one
		// Deal with ties in order of their index
		if (p->sh_offset == fCurrentOffset
			&& p - _sec_hdr <= fCurrentIndex)
			continue; // we did that one, too.
		if (p->sh_offset < nextOffset)
		{
   			nextIndex = (p - _sec_hdr);
			nextOffset = p->sh_offset;
		}
	}
    fCurrentIndex = nextIndex;
    fCurrentOffset = nextOffset;
	i++; // maintain the count so that operator int() works. (yuk)
    return _sec_hdr + nextIndex;
} // Elf32_Shdr* EachSectionByOffset::next()

//-------------------------------------------
Elf32_Shdr* EachSectionByOffset::next(long &x)
//-------------------------------------------
{
   Elf32_Shdr* result = next();
   x = fCurrentIndex; // the last index we actually found
   return result;
} // EachSectionByOffset::next(long &x)
