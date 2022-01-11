/*  @(#) elf_l.cpp 96/09/23 1.75 */

//====================================================================
// elf_l.cpp  -  ElfLinker class defs for linking ELF objs & libs

#define __ELF_L_CP__

#ifndef USE_DUMP_FILE
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "utils.h"
#include "elf_3do.h"
#endif /* USE_DUMP_FILE */

#include "debug.h"
#include "elf_l.h"
#include "linkopts.h"
#include "linkver.h"
#include "elf_d.h"

#if 1
#include "compress_noportfolio.h"
#else
#include <misc/compression.h>
#endif

#pragma segment elflink
//==========================================================================
// defs


//Heap* ElfLinker::_heap = 0;	//bug in MW - won't let me reintroduce
int ElfLinker::_nsecs=0;
int ElfLinker::_nsyms=0;
int ElfLinker::_nstrs=0;
ElfLinker::SymRelocs** ElfLinker::_relocs=0;
int ElfLinker::_nrelocs=0;

//==========================================================================
// main funcs for linking, loading, adding objs files

GetOpts* link_opts=0;
//-------------------------------------------
ElfLinker::ElfLinker(FILE* link_fp, FILE* user_fp, GetOpts* opts)
//-------------------------------------------
:	ElfReader(DUMP_FILE_ONLY)
,	_link_fp(link_fp)
,	_user_fp(user_fp)
,	_link_opts(opts)
{
	DBG_ENT("ElfLinker");
    _state.force_validate();
    //if (Validate()!=se_success) {
    //	DBG_ERR(("invalid symbolics object!\n"));
    //	return;
    // 	}
	//_heap = SymNet::_heap;	//bug in MW - won't let me reintroduced 
	link_opts=_link_opts;
	DBG(("_link_opts=x%X\n",_link_opts));
	DBG(("_link_opts->isset(linkopt_magic)=x%X\n",_link_opts->isset(linkopt_magic)));
    _hashtab = 0;
    _imports = 0;
    _exports = 0;
    _relocs = 0;
    _nrelocs = 0;
    _prog_hdr = 0;
	_unds = (Syms*) NEW(Syms);
	_coms = (Syms*) NEW(Syms);
	_specs = (Syms*) NEW(Syms);
	_imps = (Syms*) NEW(Syms);
	_exps = (Syms*) NEW(Syms);
} // ElfLinker::ElfLinker

//-------------------------------------------
ElfLinker::~ElfLinker()
//-------------------------------------------
{
	DBG_ENT("~ElfLinker");
    if (LibInfo::_libs) { LibInfo::_libs->delete_libs(); SYM_DELETE(LibInfo::_libs); }
    if (DllInfo::_dlls) { DllInfo::_dlls->delete_dlls(); SYM_DELETE(DllInfo::_dlls); }
    if (ObjInfo::_objs) { ObjInfo::_objs->delete_objs(); SYM_DELETE(ObjInfo::_objs); }
	{
		FileInfo* current = FileInfo::_files;
		while (current)
		{
			FileInfo* next = current->_chain;
			LINK_DELETE(current);
			current = next;
		}
	}
	if (_unds) DELETE(_unds);
	if (_coms) DELETE(_coms);
	if (_specs) DELETE(_specs);
	if (_imps) DELETE(_imps);
	if (_exps) DELETE(_exps);
    if (_hashtab) SYM_DELETE(_hashtab);
    if (_prog_hdr) FREE(_prog_hdr);
    if (_relocs)
    {
    	for (int i=0; i<_nrelocs; i++) 
    		if (_relocs[i]) SYM_DELETE(_relocs[i]);
    	//FREE(_relocs);
    	SYM_DELETE(_relocs);
    	}
} // ElfLinker::~ElfLinker()

//-------------------------------------------
SymErr ElfLinker::LinkFiles(const char* fname)
//-------------------------------------------
{
	DBG_ENT("LinkFiles");
    if (Validate()!=se_success) {
    	DBG_ERR(("invalid symbolics object!\n"));
    	return state();
     	}
	_link_fname=fname;
	if (nfiles()==0) {
		fprintf(_user_fp,"# Error: No object files found\n");
		SET_ERR(se_fail,("no object files found\n"));
		return state();
		}
	
	//keep the symbol table if we're generating standard elf
	//NOTE! we don't need this since now we don't emit
	//new relocation types for standard elf...
	if (_link_opts->isset(linkopt_standard)) 
		_link_opts->set(linkopt_keep);
	//if we are creating a relocatable object, allow undefs
	if (_link_opts->isset(linkopt_incremental))
	{
		_link_opts->set(linkopt_allow_unds);
		_link_opts->set(linkopt_relative);
	}
	//entry point
	if (!_link_opts->isset(linkopt_entry))
	{ 
		_link_opts->set(linkopt_entry,"__start");	//default entry is __start
	}
	// special case -e0 indicates no entry point.
	if (strcmp(_link_opts->arg(linkopt_entry), "0") != 0)
		_unds->add(_link_opts->arg(linkopt_entry),0,0);	//treat entry as undefined

	//SOMEDAY. want to add defaults to options
	//and have variable location for which to put value of option
	if (!_link_opts->isset(linkopt_secalign))
		_link_opts->set(linkopt_secalign,STRINGZ(SEC_ALIGNMENT));

	//------------------------------------
	//gather data and figure out what sections & objects we'll need
	DBG(("_link_opts=x%X\n",_link_opts));
	DBG(("_link_opts->isset(linkopt_magic)=x%X\n",_link_opts->isset(linkopt_magic)));
	create_fileinfo_n_sections();
	DBG(("_link_opts=x%X\n",_link_opts));
	DBG(("_link_opts->isset(linkopt_magic)=x%X\n",_link_opts->isset(linkopt_magic)));
	resolve_syms();	//add remaining commons and undefs and resolve imports
    	
	DBG(("_link_opts=x%X\n",_link_opts));
	DBG(("_link_opts->isset(linkopt_magic)=x%X\n",_link_opts->isset(linkopt_magic)));
	if (!_link_opts->isset(linkopt_standard))
	{
		update_3dohdr();	//fill in 3dohdr
		update_imports();	//fill in imports
		update_exports();	//fill in exports
	}    

    //------------------------------------

    //link (order is important in the following calls!)
	DBG(("_link_opts=x%X\n",_link_opts));
	DBG(("_link_opts->isset(linkopt_magic)=x%X\n",_link_opts->isset(linkopt_magic)));
    link_elf_hdr();
    link_section_hdrs();	//must follow link_elf_hdr()
    	// jrm note: the strtab and symtab section sizes emerge
    	// from link_section_hdrs() as zero.  These are patchedin
    	// by merge_symtabs(), below.

    link_program_hdrs();	//must follow link_section_hdrs()
    
    //merge & relocate symbols & strtab
    reloc_symtabs();	//more efficient to do this in merg_symtabs???
    //hash_symtabs();
    //search_libs();	//search libraries for undefineds
    merge_symtabs();	//must follow link_sec_hdr() & symtab & strtab must be last sections.

	//at this point we should have all our info
	//if error occurred, quit
	if (!_state.valid())
	{
		DBG_ERR(("fatal error occurred; quitting\n"));
		return state();
	}
	//------------------------------------
	//update
#define OLDRELS	//the new rels (for imports) don't quite work yet :-(
#ifndef OLDRELS
	//figure out which relocations we'll need and add them
	//we'll need to adjust the size afterwards once we know how
	//many relocations we'll need to generate, which means 
	//we'll have to update the section headers
	add_relocations();
#endif
    update_relocations();	//now that we have a master symtab, update the relocs
	//update starting point now that we have relocated & fixed up 
    _elf_hdr->e_entry=get_entry();	//update entry point

	//------------------------------------
    
    // Write out the headers.  Note that, if compression is on,
    // we will be re-writing the program and section headers, because
    // we don't know how big the filesz is until we write the section
    // data to disk. - jrm
    write_elf_hdr();
    write_program_hdrs();
    write_section_hdrs();
    write_hdr3do();
    
    // apply relocations & write out section contents.  Unfortunately,
    // our compression algorithm has to run before we can tell
    // how big the compressed data is.  write_sections() patches the
    // correct sizes into the sh_size fields as it writes the data
    // Then (see below) we seek back and write out the headers again.
    //   - jrm
	write_sections();
    write_relocations();
    write_dynamic_sections();

	// Before writing out the final sections and patching the
	// section/program headers, do the map.  I moved this here 96/06/12
	// because linkopt_stripmore produces an undumpable file.  This
	// change works around this by doing the dump while the info
	// is still in memory. - jrm
	if (_link_opts->isset(linkopt_map))
	{
		long savedOffset = ftell(_link_fp);
		fflush(_link_fp); // Dumper will try to read from disk.
		generate_mapfile(_link_opts->arg(linkopt_out_file),_link_opts->arg(linkopt_map));
		fseek(_link_fp, savedOffset, SEEK_SET);
	}
    write_shstrtab();
    write_symtab();
    write_strtab();

	// Now write out the patched-in section sizes.  - jrm
	if (_link_opts->isset(linkopt_compress) || _link_opts->isset(linkopt_stripmore))
	{
		long savedOffset = ftell(_link_fp);

	    // since write_sections has changed sh_size in the section
	    // headers, we must recompute the program headers.
		if (_link_opts->isset(linkopt_compress))
		{
		    link_program_hdrs();
		    //rewrite headers
		    fseek(_link_fp, _elf_hdr->e_phoff, SEEK_SET);
	        write_program_hdrs();
	    }
		fseek(_link_fp, _elf_hdr->e_shoff, SEEK_SET);
		write_section_hdrs();
		fseek(_link_fp, savedOffset, SEEK_SET);
	}
   	return state();
} // ElfLinker::LinkFiles(const char*)
    
//-------------------------------------------
void ElfLinker::create_fileinfo_n_sections()
//-------------------------------------------
{
	DBG_ENT("create_fileinfo_n_sections");
	_sections = (Sections*) SYM_NEW(Sections(SEC_NUM));
	//set up elf sections we will be creating (sh_strtab, etc...)
	_nsecs=1;	//section 0 is null entry
	
	if (!_link_opts->isset(linkopt_standard))
		create_3dohdr();
	
	//read object file info & add Elf sections
	read_fileinfo();	//read file sections, adjust baseaddrs, & hash symbols
	DBG(("_link_opts=x%X\n",_link_opts));
	DBG(("_link_opts->isset(linkopt_magic)=x%X\n",_link_opts->isset(linkopt_magic)));
	
	//add special Elf sections (need to adjust base & size later...)
    create_relocations();	//figure out what relocations we'll need to generate
	DBG(("_link_opts=x%X\n",_link_opts));
	DBG(("_link_opts->isset(linkopt_magic)=x%X\n",_link_opts->isset(linkopt_magic)));

	//add imports/exports in case we need them
	if (!_link_opts->isset(linkopt_standard))
	{
		create_imports();
		create_exports();
	}
	DBG(("_link_opts=x%X\n",_link_opts));
	DBG(("_link_opts->isset(linkopt_magic)=x%X\n",_link_opts->isset(linkopt_magic)));
	
	add_master_sec(sec_shstrtab,0);
	add_master_sec(sec_symtab,0);
	add_master_sec(sec_strtab,0);
	DBG(("_link_opts=x%X\n",_link_opts));
	DBG(("_link_opts->isset(linkopt_magic)=x%X\n",_link_opts->isset(linkopt_magic)));
} // ElfLinker::create_fileinfo_n_sections()
	
//-------------------------------------------
int ElfLinker::add_master_sec(SymSec sectype,uint32 base)
//-------------------------------------------
{
	busy_cursor();
	_sections->set_baseaddr(sectype,_nsecs++,base);
	return _nsecs-1;
}

//-------------------------------------------
void ElfLinker::read_fileinfo()
//-------------------------------------------
{
	DBG_ENT("read_fileinfo");
	FileInfo* file;
	LibInfo* l;
	ObjInfo* o;
	init_hash();	//init master hash table
	for (file=firstfile(); file; file=file->next()) {
		switch (file->_type)
		{
			case F_LIB:
			{
				l = file->_info.l;
				if (!l) continue;
				hash_lib(l);
				get_libobjs(l);	//add objs which we'll need to link with
				prune_lib(l);
			}
			break;
	#if 0	//ndef OLD_DLL
			case F_DLL: {
				d = file->_info.d;
				DBG_ASSERT(d);
				if (!d) continue;
				if (_link_opts->isset(linkopt_verbose)) {
					fprintf(_user_fp,"Adding Dll %s\n",d->_fp->filename());
					}
				//get_dllimps(d);
				//is this a DLL?
				//if so, add its export info to the list of _imps and delete the obj
				DBG_ASSERT(d->_o);
				add_imps(d->_o);    //add to imports list
				DBG_WARN(("deleting file %s from link\n",d->_o->_fp->filename()));
				LINK_DELETE(d->_o);
				d->_o=0;
			}
			break;
	#endif
			case F_OBJ:
			{
				o = file->_info.o;
				if (!o) continue;
				if (_link_opts->isset(linkopt_verbose))
				{
					fprintf(_user_fp,"Adding object file %s\n",o->_fp->filename());
				}
				add_obj(o);
			}
			break;
		}
	}
}
		
//==========================================================================
// linkers - funcs to link files together

//-------------------------------------------
void ElfLinker::link_elf_hdr()
//-------------------------------------------
{
	DBG_ENT("link_elf_hdr");
	DBG(("_link_opts=x%X\n",_link_opts));
	DBG(("_link_opts->isset(linkopt_magic)=x%X\n",_link_opts->isset(linkopt_magic)));
	_elf_hdr = (Elf32_Ehdr*) MALLOC(sizeof(Elf32_Ehdr));
	memset(_elf_hdr,0,sizeof(Elf32_Ehdr));
	Elf32_Ehdr* e = _elf_hdr;
	DBG(("_link_opts=x%X\n",_link_opts));
	if (_link_opts->isset(linkopt_magic)) 
    	memcpy(e->e_ident,_link_opts->arg(linkopt_magic),ELFMAGLEN); 
	else
    	memcpy(e->e_ident,ELFMAG,ELFMAGLEN); 
	e->e_ident[EI_CLASS]=ELFCLASS32;
	e->e_ident[EI_DATA]=ELFDATA2MSB;
    e->e_ident[EI_VERSION]=1;	//Elf header version
	if (_sections->secnum(sec_exp3do) && _exports)
    	e->e_type=ET_DYN;
    else if (_link_opts->isset(linkopt_relative))
    	e->e_type=ET_REL;	//ET_NONE ET_REL ET_EXEC ET_DYN ET_CORE
    else
    	e->e_type=ET_EXEC;
	e->e_machine=EM_PPC;
    e->e_version=ELF3DO_VER;	
    e->e_entry=0;	//for now - we'll fix up after we merge & reloc
    e->e_flags=0;	//???

    e->e_ehsize = sizeof(Elf32_Ehdr);
	e->e_shstrndx = _sections->secnum(sec_shstrtab);
	e->e_phnum = get_phnum();
	//e->e_phoff = (e->e_phnum)?sizeof(Elf32_Ehdr):0;	//after elf header
	e->e_phoff = sizeof(Elf32_Ehdr);	//after elf header
	e->e_phentsize = 8*sizeof(Elf32_Word);
	//e->e_shoff = ((e->e_phoff)?e->e_phoff:sizeof(Elf32_Ehdr))
	//		+ e->e_phentsize*e->e_phnum;
	e->e_shoff = e->e_phoff + e->e_phentsize*e->e_phnum;
	e->e_shnum = _nsecs;	//first section is null
	e->e_shentsize = 40;
} // ElfLinker::link_elf_hdr()

//-------------------------------------------
uint32 ElfLinker::get_entry()
//-------------------------------------------
{
	Elf32_Ehdr* e = _elf_hdr;
	DBG_ASSERT(_elf_hdr);
	DBG_ASSERT(_sec_hdr);
    uint32 entry=0;
	char* entry_name;
#ifdef DYNAMICSDONTHAVEMAIN
	/* kevinh fix */
    if (isa_dll())
#else
    if(0)
#endif
	{
    	entry=0;
	}
    else
    {
		_linkhash* h;
    	uint32 entry_idx;
    	if (_link_opts->isset(linkopt_entry))
    		entry_name = _link_opts->arg(linkopt_entry);
		else 
			return 0;
    	h = get_hash(entry_name,hoff_global);
    	if (!h)
    	{
			fprintf(_user_fp,"Warning: executable contains no entry point\n");
			return 0;
    	}
    	else
    	{
			entry_idx = h->_symidx;
			DBG_ASSERT(entry_idx);
			ObjInfo* defo = obj(h->_objind);
			DBG_ASSERT(defo);
			//NOTE: this must follow reloc_symtabs in order to adjust st_value
    		Elf32_Sym* sym = &(defo->_symtab[entry_idx]);
			SymSec sectype = defo->_sections->sectype(sym->st_shndx);
			if (sectype!=sec_code)
			{
				fprintf(_user_fp,"# Error: entry %s not a text symbol\n",entry_name);
				SET_ERR(se_fail,("entry %s not a text symbol\n",entry_name));
				return 0;
			}
    		entry = sym->st_value;
#if 0	//already relocated?
			entry -= defo->_sections->baseaddr(sym->st_shndx);	//make relative to start of object's text section
			entry += _sections->baseaddr(_sections->secnum(sec_code)); 
#endif
			DBG_(ELF,("entry_idx=%d, entry=x%X\n",entry_idx,entry));
    	}
    }
	return entry;
}

//-------------------------------------------
void ElfLinker::link_program_hdrs()
// set up program headers. Updates the program headers, based on
// the section headers.  If the section headers change, this should
// be called again to update the program headers.
//-------------------------------------------
{
	DBG_ENT("link_program_hdrs");
    
    if (!_elf_hdr->e_phnum) return;
    if (!_prog_hdr) // first pass (before compression, if any)
    {
	    size_t headerBytes = _elf_hdr->e_phentsize*_elf_hdr->e_phnum;
	    _prog_hdr = (Elf32_Phdr*)MALLOC(headerBytes);
	    memset(_prog_hdr, 0, headerBytes);
    }    
    int nphs=0;
    //3do specific
    if (_sections->secnum(sec_hdr3do)) {
		DBG_(ELF,("creating hdr3do program header; hdr3do secnum=%d\n",_sections->secnum(sec_hdr3do)));
		get_prog_hdr(PT_NOTE,&_prog_hdr[nphs],1,sec_hdr3do);
    	nphs++;
    	}
    if (_sections->secnum(sec_code) || _sections->secnum(sec_rodata)) {
		DBG_(ELF,("creating code/rodata program header; code secnum=%d, rodata secnum=%d\n",_sections->secnum(sec_code),_sections->secnum(sec_rodata)));
		get_prog_hdr(PT_LOAD,&_prog_hdr[nphs],2,sec_code,sec_rodata);
    	nphs++;
    	}
    if (_sections->secnum(sec_data) || _sections->secnum(sec_bss)) {
		DBG_(ELF,("creating data/bss program header; data secnum=%d, bss secnum=%d\n",_sections->secnum(sec_data),_sections->secnum(sec_bss)));
		get_prog_hdr(PT_LOAD,&_prog_hdr[nphs],2,sec_data,sec_bss);
    	nphs++;
    	}
    if (_sections->secnum(sec_relatext) || _sections->secnum(sec_relarodata) || _sections->secnum(sec_reladata)) {
		DBG_(ELF,("creating relatext/relarodata program header; relatext secnum=%d, relarodata secnum=%d\n",_sections->secnum(sec_relatext),_sections->secnum(sec_relarodata)));
		get_prog_hdr(PT_NOTE,&_prog_hdr[nphs],3,sec_relatext,sec_relarodata,sec_reladata);
    	nphs++;
    	}
	if (_sections->secnum(sec_imp3do)) {
		DBG_(ELF,("creating imports program header; imp3do secnum=%d\n",_sections->secnum(sec_imp3do)));
		get_prog_hdr(PT_NOTE,&_prog_hdr[nphs],1,sec_imp3do);
		nphs++;
		}
	if (_sections->secnum(sec_exp3do)) {
		DBG_(ELF,("creating exports program header; exp3do secnum=%d\n",_sections->secnum(sec_exp3do)));
		get_prog_hdr(PT_NOTE,&_prog_hdr[nphs],1,sec_exp3do);
		nphs++;
		}
	DBG_ASSERT(nphs==_elf_hdr->e_phnum);
} // ElfLinker::link_program_hdrs()

//-------------------------------------------
void ElfLinker::link_section_hdrs()
//-------------------------------------------
{
	DBG_ENT("link_section_hdrs");
	int i, secnum;
	uint32 size=0, off, align, base;
    if (!_nsecs) return;
    
	adjust_baseaddrs();
		
	//set up sh_strtab for sections
    for (i=1; i<_nsecs; i++)
    {
    	//calc size of sh_strtab
    	SymSec sec = _sections->sectype(i);
		size += strlen(get_shname(sec))+1;
    }
    _sh_strtab = (char*) MALLOC(size);
	if (!_sh_strtab)
	{ 
		DBG_ERR(("outamem! can't create _opts!\n")); 
		return; 
	}
    _sections->set_size(_elf_hdr->e_shstrndx, size);
    
    //set up shs
    _sec_hdr = (Elf32_Shdr*)
        MALLOC(_elf_hdr->e_shentsize * _elf_hdr->e_shnum);
	if (!_sec_hdr) { 
		DBG_ERR(("outamem! can't create _opts!\n")); 
		return; 
		}
	memset(_sec_hdr,0,_elf_hdr->e_shentsize);	//first sec_hdr is null
    size=0;
	int sec_align = _link_opts->argval(linkopt_secalign);
	//offset of first section starts after section headers
    off=_elf_hdr->e_shoff+_elf_hdr->e_shentsize*_elf_hdr->e_shnum;

    //was:for (i=1; i<_nsecs; i++) {
	//order by section types
    for (i=1; i<SEC_NUM; i++)
    {
		busy_cursor();
    	SymSec sec = (SymSec)i;
    	secnum = _sections->secnum(sec);	
		if (!secnum) continue;
		off = ALIGN(off,sec_align);	//sec_align better be >= p->sh_addralign!!
		base = get_shbase(sec);
		align = get_shalign(sec);
		//warn if overall section alignment is less than the alignment required within this section
    	if (sec_align<align)
    	{
    		DBG_WARN(("section alignment is too small for section!\n"));
			off = ALIGN(off,align);	//this alignment is for alignment within section
    	}
		strcpy(_sh_strtab+size, get_shname(sec));	//set string
		Elf32_Shdr* p = _sec_hdr + secnum; //my sections don't count the null section
		p->sh_type = get_shtype(sec); 
		p->sh_flags = get_shflags(sec); 
		p->sh_addralign = align; 
		p->sh_addr = base;
		p->sh_size = _sections->size(secnum);	
		p->sh_offset = off;	//file offset
		p->sh_name = size;	//index into sh_strtab
		p->sh_link = get_shlink(sec);
		p->sh_info = get_shinfo(sec);
		p->sh_entsize = get_shentsize(sec);
		//p->sh_entrycount =  ElfReader::calc_symtab_nsyms(p);
		
		DBG_(ELF,("section %d %s to be written at offset=x%X size=x%X\n",\
			secnum,_sh_strtab+size,p->sh_offset,p->sh_size));
		//get ready for next section
    	if (p->sh_type!=SHT_NOBITS)
			off += p->sh_size;
		size += strlen(get_shname(sec))+1;
    }
} // ElfLinker::link_section_hdrs()
        
//-------------------------------------------
void ElfLinker::link_hash_table(Elf32_Shdr *)
//-------------------------------------------
{
	DBG_ENT("link_hash_table");
	DBG_FIXME(("hash table not implemented\n"));
}

//-------------------------------------------
void ElfLinker::link_dynamic_data(Elf32_Shdr *s)
//-------------------------------------------
{
	DBG_ENT("link_dynamic_data");
	DBG_FIXME(("dynamic data not implemented; 3do has their own format\n"));
}

//==========================================================================
// funcs for symtab

//-------------------------------------------
_linkhash* ElfLinker::get_hash(ObjInfo* o,int symidx)
//get hashed symbol from hash table
//-------------------------------------------
{
	DBG_ENT("get_hash");
	_linkhash* h;
	Elf32_Sym* sym=get_sym(o,symidx);
	DBG_ASSERT(sym);
    const char* name = get_symname(o,sym);
    if (!name)
    {
		SET_ERR(se_fail,("#Unable to get name!\n"));
		return 0;
	}
	//hash with objind if local to prevent hash collision
	HashOff scope = scopeid(sym,o->_objind);
	if (h=_hashtab->get_hash(name,scope),h)
	{	//name already in table??
		if (h->_symidx!=symidx)
		{
			SET_ERR(se_fail,("different symbol found for same hash! "
				"symidx=x%X; h->_symidx=x%X\n",symidx,h->_symidx));
		}
	}
	return h;
}

//-------------------------------------------
void ElfLinker::init_hash()
//must read obj headers & lib headers first!
//-------------------------------------------
{
	DBG_ENT("init_hash");
	ObjInfo* o;
	LibInfo* l;
	int minsz;
	uint32 hsize = 0;
	//calculate size for hash table
	for (o=firstobj(); o; o=o->next())
		hsize += get_symtab_nsyms(o);
	//calculate the total number of objects possible
	minsz = nobjs()+1;
	for (l=firstlib(); l; l=l->next())
	{
	#ifdef OLDLIB
		minsz += l->_nobjs;
	#else		//in the new library code, we use external symbols instead of objs
		minsz += l->_ar->_ar_syms._nsyms;
	#endif
	}
	//better if size is odd (even better if prime...)
	hsize = max(hsize/2,minsz);	//so as not to collide with local symbols since obj index is used in has function
	if (hsize/2 == (hsize+1)/2)	hsize++; //even???	make odd
	_hashtab = (LinkHash*) SYM_NEW(LinkHash(hsize));
}

//-------------------------------------------
void ElfLinker::hash_symtabs()
//hash object's symbols so we know what's undefined and can resolve quickly
//-------------------------------------------
{
	DBG_ENT("hash_symtabs");
	ObjInfo* o;
	//hash symbols
	for (o=firstobj(); o; o=o->next())
		hash_symtab(o);
}

//-------------------------------------------
void ElfLinker::reloc_symtabs()
//once we figure out base addresses of sections, relocate the symbol tables
//-------------------------------------------
{
	DBG_ENT("reloc_symtabs");
	ObjInfo* o;
	//hash symbols
	for (o=firstobj(); o; o=o->next())
	{
		busy_cursor();
		reloc_symtab(o);
	}
}

static size_t gMaxSyms = 0;
static size_t gMaxStrs = 0;

//-------------------------------------------
int ElfLinker::add_sym(Elf32_Sym* sym, const char* name, SymSec sectype, HashOff scopeid)
//add symbol to master _symtab and set hash ptr to new index
//add symbol's name to master _strtab and update indexes
//scopeid will be 0 for global symbols
//returns old symbol index
//can have an undefined symbol that needs to be added
//-------------------------------------------
{
	if (_nsyms >= gMaxSyms)
	{
		// jrm 96/05/06
		printf("# Symbol table overflow\n");
		printf("# Link terminated.\n");
		exit(1);
	}
	_linkhash* h=0;
	if (name && *name)
	{
		h = get_hash(name,scopeid);	//update symbol index in hash table
		if (!h)
		{
			//sometimes we want to add symbols which don't come from objects
			//NOTE: we'll need to add objind & symidx to hash if we want to reference them
			DBG_WARN(("symbol %s not found in hashtab! adding anyway...\n",name));
        	h = add_hash(name,scopeid,0,0);
		}
		if (h->_newsymidx)
		{
			//symbol was already added; ignore
			DBG_WARN(("symbol %s was already added at index %d\n",name,h->_newsymidx));
			return h->_symidx;
		}
		else
        	h->_newsymidx = _nsyms;
		_symtab[_nsyms].st_name = _nstrs;	//set & update name
		int len = strlen(name) + 1;
		if (_nstrs + len >= gMaxStrs)
		{
			// jrm 96/05/06
			printf("# String table overflow adding %s.\n", name);
			printf("# Link terminated.\n");
			exit(1);
		}
		strcpy(&_strtab[_nstrs], name);
		_nstrs += len;
	}
	//add symbol to symtab
	_symtab[_nsyms].st_value = sym->st_value;	//copy other fields
	_symtab[_nsyms].st_info = sym->st_info;
	_symtab[_nsyms].st_size = sym->st_size;
	_symtab[_nsyms].st_other = sym->st_other;
	//set & update section
	if (sym->st_shndx==SHN_ABS)
		_symtab[_nsyms].st_shndx = SHN_ABS;	
	else if (sym->st_shndx==SHN_COMMON)
	{
		int secnum = _sections->secnum(sec_data);	
		int osecnum;
		_symtab[_nsyms].st_shndx = secnum;	
		uint32 size = _coms->_tag;	//add to tail of data (tag is last defined data size)
		size = ALIGN(size,get_shalign(sec_data));	//align
		_symtab[_nsyms].st_value = size;	//give symbol a home.	
		size += sym->st_size;	//increase tail of data by new symbol
		_coms->_tag = size; //adjust tail of data for next com symbol
		DBG_ASSERT(h);	//update secnum in original symtab to data
		ObjInfo* o = obj(h->_objind);	//so we'll add the correct offset in relocations
		DBG_ASSERT(o);
		//but obj may not have a data section!  then what?
		//will just have to leave it as common
		osecnum = o->_sections->secnum(sec_data);
		if (!osecnum) osecnum = SHN_COMMON;
		sym->st_shndx = osecnum;	//change obj's symbol to now point to data
		sectype = sec_data;
	}
	else if (sym->st_shndx==SHN_UNDEF)
		_symtab[_nsyms].st_shndx = SHN_UNDEF;	
	else
		_symtab[_nsyms].st_shndx = _sections->secnum(sectype);	
	_nsyms++;
	return h?h->_symidx:0;
} // ElfLinker::add_sym(Elf32_Sym*, const char*, SymSec)

//-------------------------------------------
void ElfLinker::rmv_sym(char* name, HashOff scopeid)
//	Called during merge_symtab when we don't need symbol anymore
//needed in order to remove symbol's tag (objind)... 
//This way when we're processing relocations and 
//search for this symbol, we'll get a 0 hashmark meaning the symbol
//won't get added in the final _symtab, so we shouldn't reference it.
//	SOMEDAY. would be more efficient to add a 3rd entry in the hash
//table for the final symidx instead of using tag for both.
//That way we don't need this routine, and searching for the
//"real" symbol in the relocations will be much easier
//-------------------------------------------
{
	if (name && *name)
	{
		// remove so we don't pick it up as a valid symbol
		// when processing relocations 
		_hashtab->rmv_hash(name,scopeid);
	}
} // ElfLinker::rmv_sym()
	
//-------------------------------------------
void ElfLinker::add_sym(ObjInfo* o,int symidx)
//objind will be 0 for global symbols
//can have an undefined symbol (so may not be in hashtab)
//-------------------------------------------
{
	DBG_ENT("add_sym");
	DBG_ASSERT(o);
	Elf32_Sym* sym = get_sym(o,symidx);
	//if (_link_opts->isset(linkopt_strip))
		//only add symbols that are referenced by relocation entries
	//if (_link_opts->isset(linkopt_relative))	//emit fixups that allow obj to be relative
		//pass relocation entries thru as PPC_REL/PPC_ADDR16
	int hsymidx;
	SymSec sec;
	DBG_ASSERT(sym);
	if (sym->st_shndx==SHN_ABS)
		sec = sec_abs;	
	else if (sym->st_shndx==SHN_COMMON)
		sec = sec_com;	
	else
		sec = o->_sections->sectype(sym->st_shndx);
	const char* name = get_symname(o,sym);
    if (!name || !*name)
    {
		// This test solves a particularly nasty crash.
		static int onlyOnce = 0;
		if (!onlyOnce)
		{
			onlyOnce = 1;
			fprintf(_user_fp, "## Symbol with no name encountered.\n");
		}		
		return;
	}
	DBG_(ELF,("adding symbol %d %s from obj %d %s\n",symidx,name,o->_objind,o->_fp->filename()));
	hsymidx=add_sym(sym,name,sec,scopeid(sym,o->_objind));	//add symbol & string to master symtab & update hash table
	if (!hsymidx)
		SET_INFO(se_not_found,("symbol not found in hash table! (undefined?)\n"))
	else if (symidx!=hsymidx)
	{
		if (_link_opts->isset(linkopt_allow_dups))
			SET_INFO(se_dup_symbols,("different symbol found for same hash (duplicate def?)! symidx=x%X,hsymidx=x%X; %s obj %d %s base=x%X\n",symidx,hsymidx,name,o->_objind,o->_fp->filename(),o->_base))
		else
			SET_ERR(se_fail,("different symbol found for same hash (duplicate def?)! symidx=x%X,hsymidx=x%X; %s obj %d %s base=x%X\n",symidx,hsymidx,name,o->_objind,o->_fp->filename(),o->_base))
	}
} // ElfLinker::add_sym(ObjInfo*,int)

//-------------------------------------------
void ElfLinker::rmv_sym(ObjInfo* o,int symidx)
//-------------------------------------------
{
	DBG_ENT("rmv_sym");
	DBG_ASSERT(o);
	Elf32_Sym* sym = get_sym(o,symidx);
	DBG_ASSERT(sym);
	const char* name = get_symname(o,sym);
	if (name && *name)
	{
		HashOff scope = scopeid(sym, o->_objind);
		_hashtab->rmv_hash(name,scope);
	}
}

//-------------------------------------------
SymErr ElfLinker::init_symtab()
//when merging the symtabs, 
//if (linkopt_debug || linkopt_keep), add all symbols to master symtab,
//else we want to add to the master symtab
//only those symbols which are undefined/in a shared library for 
//use by PPC_ADDR* relocation types
//-------------------------------------------
{
	DBG_ENT("init_symtab");
	int i;
	ObjInfo* o;
	//initial values
	int init_nsyms=1+_specs->_nsyms+(_nsecs-1);	//null entry + special symbols + (nsections - null section)
	int init_nstrs=1;//null entry + special symbols
	Syms::Sym* spec;
	for (spec=_specs->first(); spec; spec=spec->next()) {
		init_nstrs+=strlen(spec->_name)+1;
		}
	
    //estimate size to make merged symtab & strtab
	_nsyms=init_nsyms;	//for file & sections
	_nstrs=init_nstrs;
	//merge obj symtabs
	for (o=firstobj(i); o; o=o->next(i)) {
		int nsyms = get_symtab_nsyms(o);
		int nstrs = get_sechdr(o,sec_strtab)->sh_size;
		DBG_(ELF,("objind=%d; nsyms=x%X, nstrs=x%X\n",o->_objind,nsyms,nstrs));
		_nstrs += nstrs;
		_nstrs += strlen(o->_name) + 1;
		_nsyms += nsyms;
		}
	if (!_nsyms) {	//no symtab/strtab needed!
		_symtab=0;
		_strtab=0;
		return state();
		}
	_symtab = (Elf32_Sym*) MALLOC(_nsyms*sizeof(Elf32_Sym));	//more than enuf
	_strtab = (char*) MALLOC(_nstrs);	//more than enuf
	gMaxSyms = _nsyms;
	gMaxStrs = _nstrs
	DBG_ASSERT(_symtab);
	DBG_ASSERT(_strtab);
	if (!_strtab || ! _strtab) {
		SET_ERR(se_malloc_err,("#Unable to create _symtab/_strtab\n"));
		return state();
		}
	//init to 0
	memset(_symtab, 0, _nsyms * sizeof(Elf32_Sym));
    //go thru each of the objects, merging symtab & strtab
	_nsyms=0;	
	_nstrs=1;
	_strtab[0]='\0';	//null name
	return state();
}
	
//-------------------------------------------
void ElfLinker::resolve_syms()
//after all the objs have been added, add remaining unresolved symbols
//-------------------------------------------
{
	DBG_ENT("resolve_syms");
	Syms::Sym *x, *u, *xnext;
	Elf32_Sym* hsym;
	_linkhash* h;
	//add special symbols if final link
	if (!_link_opts->isset(linkopt_incremental)) {
		_specs->add("_etext",0,0,sec_code);
		_specs->add("_edata",0,0,sec_data);	//use symidx as the sectype
		_specs->add(_link_fname,0,0,sec_none);
		}
	//resolve symbols
	_coms->_tag = _sections->size(_sections->secnum(sec_data));	//store start of common symbols in the _tag
//go ahead and leave the entry as an undefined if not a Dll...
	//if we have't found the entry point, just warn but 
	//remove from _unds so we don't add it to symbol table
	if (_link_opts->isset(linkopt_entry)
			&& (x=_unds->find(_link_opts->arg(linkopt_entry)),x)) {
		if (!get_hash(x->_name,hoff_global))
			_unds->rmv(x);
		}
	//resolve_syms(_specs);	//resolve special symbols
	for (x=_specs->first(); x; x=xnext) {
		ObjInfo* o;
		xnext=x->next();
		if (h=get_hash(x->_name,hoff_global),h) {
			o = obj(h->_objind);
			DBG_ASSERT(o);
			fprintf(_user_fp,"## Warning: symbol %s defined in file %s overrides linker generated symbol of same name\n",x->_name,o->_fp->filename());
			SET_INFO(se_dup_symbols,("linker generated symbol %s defined in file %s\n",x->_name,o->_fp->filename()));
			_specs->rmv(x);
			continue;
			}
		if (u=_unds->find(x->_name),!u) {
			//may want to generate anyway for debug/keep, but would have to change merge_symtab to not reference obj's symtab
			//if (!_link_opts->isset(linkopt_keep) && !_link_opts->isset(linkopt_debug))
			_specs->rmv(x);	//remove uneeded special symbols
			}
		else {
			x->_objind = u->_objind;	//update our specs
			x->_symidx = u->_symidx;
			add_hash(u->_name, u->_symidx, u->_objind);
			//_ordinal is section type - find its relative section number
			//so that if/when we apply relocations, we'll adjust correctly
			//can't use secnum since object may not have a corresponding section 
			ObjInfo* o = obj(u->_objind);
			hsym = get_sym(o,u->_symidx);
			hsym->st_shndx=SHN_ABS;	
			hsym->st_size=0;	
			//we'll update the value later after we've resolved all the symbols
			//hsym->st_value=_sections->size(osecnum); 
			_unds->rmv(u);
			DBG_(ELF,("symbol %s resolved; x->_ordinal=x%X, secnum=x%X, val=x%X (sidx=%d; oind=%d)\n",
				x->_name,x->_ordinal,hsym->st_shndx,hsym->st_value,x->_symidx,x->_objind));
			}
		}
	if (_coms->_nsyms) {
		//it's possible to have common data with no data, 
		//so let's male sure we have a data section
		int secnum = _sections->secnum(sec_data);
		if (!secnum) 
			add_master_sec(sec_data,0);
		resolve_syms(_coms);	//define the remaining commons as data
		}
	resolve_syms(_unds);	//import unds or add as undef 
#if 1	//def OLD_DLL
	resolve_imps();	//get rid of unreferenced imports
#endif

	//go thru remaining undefs and warn/report error
	if (!_link_opts->isset(linkopt_incremental))
	{
		Syms::Sym* sym;
		ObjInfo* o;
		const char* n;
		for (sym=_unds->first(); sym; )
		{
			busy_cursor();
			Syms::Sym* tsym=sym->next();    //save in case we delete sym
			o = obj(sym->_objind);
			DBG_ASSERT(o);
			n = sym->_name;
			if (_link_opts->isset(linkopt_allow_unds))
			{
				SET_INFO(se_und_symbols,("undefined symbol!  symidx %d (%s) objidx %d (%s)\n",sym->_symidx,n,o->_objind,o->_fp->filename()));
				fprintf(_user_fp,"Warning: ");
			}
			else
			{
				SET_ERR(se_fail,("undefined symbol!  symidx %d (%s) objidx %d (%s)\n",sym->_symidx,n,o->_objind,o->_fp->filename()));
				fprintf(_user_fp,"# Error: ");
			}
			if (n)
				fprintf(_user_fp,"symbol %s ",n);
			else
				fprintf(_user_fp,"symbol at index %d ",sym->_symidx);
			if (o->_libind)
				fprintf(_user_fp,"referenced from library %s ",o->_fp->filename());
			else
				fprintf(_user_fp,"referenced from obj %s ",o->_fp->filename());
			fprintf(_user_fp,"is undefined\n");
			//remove it from the unresolved list
			_unds->rmv(sym);
			sym = tsym;
		}
	}
}

//-------------------------------------------
void ElfLinker::resolve_syms(Syms* syms)
//-------------------------------------------
{
	DBG_ENT("resolve_syms");
	Syms::Sym* sym;
	Boolean resolved=false;
	_linkhash* h=0;
	for (sym=syms->first(); sym; ) {
		Syms::Sym* tsym=sym->next();	//save in case we delete sym
		h = get_hash(sym->_name,hoff_global);	//is symbol defined??
		if (!h) {
			//add it now and warn
        	add_hash(sym->_name,sym->_symidx,sym->_objind);
			resolved = resolve_sym(sym);
			DBG_WARN(("symbol %s not resolved; adding it anyway (sidx=%d; oind=%d)\n",sym->_name,sym->_symidx,sym->_objind));
			}
		//remove it from the unresolved list
		if (h || resolved)
			syms->rmv(sym);
		sym = tsym;
		}
	}

//-------------------------------------------
Boolean ElfLinker::resolve_sym(Syms::Sym* sym)
//perform special functions if any in order to resolve sym
//-------------------------------------------
{
	int sidx = sym->_symidx;
	int oind = sym->_objind;
	DBG_ASSERT(obj(oind));
	Elf32_Sym* s = get_sym(obj(oind),sidx);
	DBG_ASSERT(s);
	switch (s->st_shndx)
	{
		case SHN_COMMON:
		{ 	
			//add symbol's size to total data size
			int secnum = _sections->secnum(sec_data);
			uint32 size = _sections->size(secnum);
			size = ALIGN(size, get_shalign(sec_data));	//align
			size += s->st_size;	//increase size of data by new symbol
			_sections->set_size(secnum,size);
			return true;
		}
		case SHN_UNDEF: 	//check imports for undefineds
			//Can't do cleanup of imports without seeing if it resolved someone;
			//how should we handle imports that are also defined in an object?
			//makes sense to treat as duplicate since it's as if we would 
			//have added  the object if it were in a real library
			//	No, want to keep as undefined because will want to emit 
			//	relocations on it and will need the symbol
			//if (_imps->find(sym->_name))
			// {	
			// 	return true;
			// }
			break;
	}
	return false;
} // ElfLinker::resolve_sym(Syms::Sym*)

//-------------------------------------------
SymErr ElfLinker::merge_symtabs()
//-------------------------------------------
{
	DBG_ENT("merge_symtabs");
	Elf32_Shdr *sh, *st;
	ObjInfo* o;
	if (Failed(init_symtab()) || !_symtab)
	{
		_nstrs=0;
		return state();
	}
	Elf32_Sym sym;

	//create null entry
	memset(&sym,0,sizeof(Elf32_Sym));	//null entry
	add_sym(&sym,0,sec_none,hoff_global);	//add null entry

	//create file & section symtabs if linkopt_strip not set
	if (_link_opts->isset(linkopt_debug) || _link_opts->isset(linkopt_keep))
	{
		sym.st_info = ELF32_ST_INFO(STB_LOCAL,STT_FILE);
		sym.st_shndx = SHN_ABS;
		add_sym(&sym,_link_fname,sec_abs,hoff_global);	//add file entry
		for (int i=1; i<_nsecs; i++)
		{	//section entries 
			busy_cursor();
			if ((get_shflags(_sections->sectype(i))&SHF_ALLOC)==SHF_ALLOC)
			{
				sym.st_info = ELF32_ST_INFO(STB_LOCAL,STT_SECTION);
				sym.st_value = _sections->baseaddr(i);
				sym.st_shndx = i;
				add_sym(&sym,get_shname(_sections->sectype(i)),_sections->sectype(i),hoff_global);	//add section entry
			}
		}
	}
	//define special symbols in obj if referenced (_etext, etc) now that we know their values
	Syms::Sym* spec;
	for (spec=_specs->first(); spec; spec=spec->next())
	{
		int secnum;
		Elf32_Sym* s;
		s = get_sym(obj(spec->_objind),spec->_symidx);
		DBG_ASSERT(s);


		s->st_info = ELF32_ST_INFO(STB_GLOBAL,STT_OBJECT);
		secnum = _sections->secnum((SymSec) spec->_ordinal);
		if (secnum)
		{
			// s->st_value = _sections->baseaddr(secnum) + _sections->size(secnum);
			// s->st_value = _sec_hdr[secnum].sh_size;
			s->st_value = _sections->baseaddr(secnum) + _sec_hdr[secnum].sh_size; // jrm 96/04/29
		}
	}
	//merge obj symtabs
	for (o=firstobj(); o; o=o->next())
	{
		_linkhash* h;
		int nsyms = get_symtab_nsyms(o);
		//need to fix up relocations if index changes!!
		//handled by set_hash during merge_symtab
		for (int j=1; j<nsyms; j++)
		{
			busy_cursor();
			Elf32_Sym* sym = &(o->_symtab[j]);
			//if (FILTER_OUT(sym))
			//	continue;
			//if (ELF32_ST_TYPE(sym->st_info)==STT_SECTION)
			//	continue;
			//for undef & common symbols, we'll wait to see if we find the symbol's def
			if (sym->st_shndx!=SHN_UNDEF 
					&& sym->st_shndx!=SHN_COMMON)
			{
				//add to master _symtab and set hash ptr to new index
				if (_link_opts->isset(linkopt_debug) 
						|| _link_opts->isset(linkopt_keep)
#if 0 //def PRE_DOS
//don't need to keep absolute symbols
						|| sym->st_shndx==SHN_ABS
#endif
						|| _exps->find(get_symname(o,j)))
				{
					//need symbol info for exported info
					add_sym(o,j);	//add symbol
				}
			}
			else
			{
				//undef/common; see if defined elsewhere
				//we should be adding iff it's in the hash_table
				//because we don't hash undef/common symbols unless
				//we resolve_syms and make them the token undef/common 
    			const char* name = get_symname(o,j);
				if (!name || !*name)
				{
					//add_sym(i,sym);
					DBG_WARN(("undefined/common symbol has no name!!  symidx=%d objind %d (%s)\n",j,o->_objind,o->_fp->filename()));
					continue;
				}
				//defined elsewhere??  or is THIS the added definition?
				//ie - if it's really defined, otherwise add it anyway
				DBG_(ELF,("und/com symbol %s; sidx=%d, oind=%d\name",name,j,o->_objind));
				if ((h = get_hash(name,scopeid(sym,o->_objind)),h)	//defined elsewhere?
						&& !(h->_symidx==j && h->_objind==o->_objind))	//is this the token undef/common?
					//ignore - we'll deal with this when we get to it...
					;
				else
				{	//add undefined/common symbol 
					add_sym(o,j);
				}
			}
		}
	}
	//update special symbols in _symtab (_etext, etc) now that we know their values
	for (spec=_specs->first(); spec; spec=spec->next())
	{
		int secnum;
		Elf32_Sym* s;
		_linkhash* h;
		h = get_hash(spec->_name,hoff_global);
		DBG_ASSERT(h && h->_newsymidx);
		s = &_symtab[h->_newsymidx];
		DBG_ASSERT(s);
		s->st_info = ELF32_ST_INFO(STB_GLOBAL,STT_OBJECT);
		secnum = _sections->secnum((SymSec) spec->_ordinal);
		if (secnum)
		{
			// s->st_value = _sections->baseaddr(secnum) + _sections->size(secnum);
			// s->st_value = _sec_hdr[secnum].sh_size;
			s->st_value = _sections->baseaddr(secnum) + _sec_hdr[secnum].sh_size; // jrm 96/04/29
			s->st_shndx = secnum;
		}
		else
			s->st_shndx = SHN_ABS;
	}
		
	//update sec_hdrs for symab & strtab (offset is end of last section; size is 0)
	//NOTE!  must follow link_sec_hdr() & symtab & strtab must be last sections.
	sh = SH(sec_symtab);
	st = SH(sec_strtab);
	DBG_ASSERT(sh);
	DBG_ASSERT(st);
	sh->sh_size = _nsyms * sh->sh_entsize;	//update
	//sh->sh_entrycount = _nsyms;
	st->sh_offset = sh->sh_offset+ALIGN(sh->sh_size,_link_opts->argval(linkopt_secalign));
		//symtab->sh_size was originally 0
	st->sh_size = _nstrs; //update
	//st->sh_entrycount =  ElfReader::calc_symtab_nsyms(st);
	if (!sh->sh_size && _link_opts->isset(linkopt_strip))	//no symtab & needed
		_nsecs-=2;	//symtab & strtab are the next to last sections
	//if there were undefineds, validate now
    if (_state.validate()!=se_success)
    	DBG_ERR(("#Unable to validate object!\n"));
	return state();
} // ElfLinker::merge_symtabs()

//==========================================================================
// funcs for relocating

	//---------------------------------------------------------------
	// defs for relocating
	#define SYMIDX(r)	ELF32_R_SYM(r->r_info)
	#define RTYPE(r)	ELF32_R_TYPE(r->r_info)
	#define SYM(r)		(o->_symtab+SYMIDX(r))
	#define SYMSEC(r)		(get_symsectype(o,SYMIDX(r)))
	#define SYMSECNUM(r)	(SYM(r)->st_shndx)
	#define SYMSECBASE(r)	(o->_sections->baseaddr(SYMSECNUM(r)))
	#define SYMSECOBJBASE(r)	((o->_sec_hdr[SYMSECNUM(r))].sh_addr)
	#define SYMTABSEC()	(o->_sections->secnum(sec_symtab))

	//relocation definitions:
	#define LO(x) (x & 0xffff)
	#define HI(x) ((x>>16) & 0xffff)
	#define HA(x) (((x>>16) + ((x&0x8000)?1:0)) & 0xffff)
	#define A(r)		r->r_addend
	#define B(r)		SYMSECBASE(r)
	#define G(r)		(SET_ERR(se_fail,("GOT relocations not allowed!\n")),0)
	#define L(r)		(SET_ERR(se_fail,("PLT relocations not allowed!\n")),0)
	#define P(r)		r->r_offset	//???
	#define R(r)		SYM(r)->st_value - SYMSECOBJBASE(r)	//offset of symbol from section
	#define S(r)		SYM(r)->st_value

	//3do extras:
	//Note, the original definitions of the new relocation 
	//types require a symbol table to
	//contain the symbols which represent have the sections...
	//but since the new relocations don't need a symbol, it
	//is sufficient to put the section number for the symidx!  
	//This breaks EABI conformance.
	//Also would like to remove section names from sh_strtab and
	//use codes instead...  same story there
	#define IMPLIB(r)	(SYMIDX(r)&0xff000000))
	#define IMPSYM(r)	(SYMIDX(r)&0x00ffffff))
	#define BREL(r)		(o->_sections->baseaddr(SYMIDX(r)))
	//we won't be applying any relocations on imports
	//we'll leave that to the loader
	#define BIMP(r)		0	//get_import(IMPLIB(r),IMPSYM(r))

	//relocation calculations:
	#define LOW24		0x03fffffc
	#define LOW14		0x0000fffc
	#define LOW14_BRN		0x00300000 //set bits if BRNTAKEN
	#define GET_LOW24(x,y)	(((x)&(~LOW24)) | ((y)&LOW24))
	#define GET_LOW14(x,y)	((x&~LOW14) | (y&LOW14))
	//#define SET_WORD32(off,val) ((uint32)(*off)=swapfix((uint32)val))
	//#define SET_LOW14(off,val) ((uint32)(*off)=swapfix(GET_LOW14(swap((uint32)(*off)),(uint32)val)))
	//#define SET_LOW24(off,val) ((uint32)(*off)=swapfix(GET_LOW24(swap((uint32)(*off)),(uint32)val)))
	//#define SET_HALF16(off,val) ((uint16)(*off)=swapfix((uint16)val))

//functions for getting/putting values at offsets based on alignment and endianness
//used for applying a relocated value to a section offset
//-------------------------------------------
#ifdef macintosh
inline
#endif
uint16 ElfLinker::get_off16(uint32 salign, uint8* off)
//-------------------------------------------
{
#ifdef macintosh
	return swapfix(*((uint16*)off));
#else
	uint16 val;
	if (!(salign&1)) 
		val = *((uint16*)off);
	else 	//must handle alignment
		memcpy(&val,off,2);
	return swapfix(val);
#endif
}

//-------------------------------------------
#ifdef macintosh
inline
#endif
void ElfLinker::put_off16(uint16 val,uint32 salign,uint8* off)
//-------------------------------------------
{
#ifdef macintosh
	*((uint16*)off) = swapfix(val);
#else
	val = swapfix(val);
	if (!(salign&1)) 
		*((uint16*)off) = val;
	else 	//must handle alignment
		memcpy(off,&val,2);
#endif
}

//-------------------------------------------
#ifdef macintosh
inline
#endif
uint32 ElfLinker::get_off32(uint32 salign, uint8* off)
//-------------------------------------------
{
#ifdef macintosh
	return swapfix(*((uint32*)off));
#else
	uint32 val;
	if (!(salign&3)) 
		val = *((uint32*)off);
	else 	//must handle alignment
		memcpy(&val,off,4);
	return swapfix(val);
#endif
}

//-------------------------------------------
#ifdef macintosh
inline
#endif
void ElfLinker::put_off32(uint32 val, uint32 salign, uint8* off)
//-------------------------------------------
{
#ifdef macintosh
	*((uint32*)off) = swapfix(val);
#else
	val = swapfix(val);
	if (!(salign&3)) 
		*((uint32*)off) = val;
	else 	//must handle alignment
		memcpy(off, &val, 4);
#endif
}

//functions for applying a relocated value to a section offset
//-------------------------------------------
inline void ElfLinker::set_half16(uint8* off, uint32 salign, uint16 val)
//-------------------------------------------
{
	put_off16(val, salign, off);
}

//-------------------------------------------
inline void ElfLinker::set_word32(uint8* off, uint32 salign, uint32 val)
//-------------------------------------------
{
	put_off32(val,salign,off);
} // ElfLinker::set_word32

//-------------------------------------------
inline uint32 ElfLinker::MergeLow14(uint32 hi, uint32 lo)
// This inline function replaces GET_LOW14.
//-------------------------------------------
{
	// JRM 96/04/16 check for branch out of range
	int32 truncatedLo = (lo & LOW14); // Clobbers sign bit
	if ((int16)truncatedLo != (int32)lo) // int16 cast to sign extend
		return 0;
#if 0
	// JRM
	uint32 result = (hi & ~LOW14) | truncatedLo;
	uint32 oldresult = (hi & ~LOW14) | (((lo>>2)<<2) & LOW14);
	if (result != oldresult)
		exit(1);
	return result;
#else
	return (hi & ~LOW14) | truncatedLo;
#endif
} // ElfLinker::MergeLow14

#include "ppc_disasm.h"
//-------------------------------------------
void ElfLinker::set_low14(uint8* off, uint32 salign, uint32 val)
// JRM 96/04/16: I changed this so that val is no longer assumed to
// be shifted right before entry, and then shifted back.  Thus,
// val is currently assumed to be 16 bit integer.
//-------------------------------------------
{
	//JRM 96/04/16 NO: val <<= 2;// shift back by 2 in order to place in correct spot
	// Get the 32-bit quantity currently at that offset.
	uint32 aoff = get_off32(salign, off);
	DBG_(PICKY,("after asn aoff=x%X,val=x%X\n",aoff,val));
	uint32 newoff = MergeLow14(aoff, val);
	if (!newoff)
	{
		printf(
			"# Addr14, Rel14, or BaseRel14 branch out of 16-bit range.\n"
			"# \tObject file           : \"%s\"\n"
			"# \tOffset                : %d(0x%X) (bytes)\n"
			"# \tInstruction           : ",
			_current_objfilename,
			_current_offset_in_section,
			_current_offset_in_section);
		char buffer[100]; char comment[100];
		DisasmPPC(aoff, 0, 0, 0, buffer, comment);
		printf(
			"%s ; %s\n"
			"# \tBranch distance       : 0x%08X\n"
			"# Aborting link. Use \"dump3do -u %s\" for instruction context.\n",
			buffer, comment, val, _current_objfilename);
		exit(1);
	}
	DBG_(PICKY,("after GET14 newoff=x%X,val=x%X\n",newoff,val));
	put_off32(newoff, salign, off);
} // ElfLinker::set_low14

//-------------------------------------------
void ElfLinker::set_low24(uint8* off, uint32 salign, uint32 val)
//-------------------------------------------
{
	val <<= 2;	//shift back by 2 in order to place in correct spot
	uint32 aoff = get_off32(salign, off);
	DBG_(PICKY,("after asn aoff=x%X,val=x%X\n",aoff,val));
	aoff = GET_LOW24(aoff, val); 
	DBG_(PICKY,("after GET24 aoff=x%X,val=x%X\n",aoff,val));
	put_off32(aoff, salign, off);
}

//-------------------------------------------
ElfLinker::SymRelocs::SymRelocs(SymSec sec, int nobjs, uint32 size)
//-------------------------------------------
{
	_sectype = sec;
	_objs = (int*) MALLOC(nobjs*sizeof(int));	//map of relocations to obj where originated
	_buf = (uint8*) MALLOC(size); //new relocs will be no bigger than original
	_nents = 0;	//no relocs added yet		
	_entsize = sizeof(Elf32_Rela);	//only relas supported now
	memset(_objs,0,sizeof(int)*nobjs);
}

//-------------------------------------------
ElfLinker::SymRelocs::~SymRelocs()
//-------------------------------------------
{
	if (_objs) FREE(_objs);
	if (_buf) FREE(_buf);
}

//-------------------------------------------
void ElfLinker::create_relocations()
//create relocation sections that we will need to emit
//-------------------------------------------
{
	DBG_ENT("create_relocations");

    //if relative, emit fixups that allow sec to be relative
	//and pass relocation entries thru as PPC_REL/PPC_ADDR16
	
	SymSec sectype;
	ObjInfo* o;
	int secnum, i, j;
	int rsize[SEC_NUM];
	int nrelocs;
	memset(rsize,0,sizeof(int)*SEC_NUM);
	//count nreloc sections & max size for each
	for (o=firstobj(); o; o=o->next())
	{
		busy_cursor();
		for (j=0; j<o->_sections->nsecs(); j++)
		{
			Elf32_Shdr* s = o->_sec_hdr+j;
			sectype = o->_sections->sectype(j);
			if (RELA(s) && (sectype != sec_reladebug && sectype != sec_relaline))
				rsize[sectype] += s->sh_size;
		}
	}
	nrelocs=0;
	for (i=0; i<SEC_NUM; i++)
	{
		if (rsize[i]) nrelocs++;
	}
	//create _relocs
	//_relocs = (SymRelocs**) MALLOC(sizeof(SymRelocs*)*nrelocs);
	_relocs = (SymRelocs**) SYM_NEW(SymRelocs*[nrelocs]);
	nrelocs=0;
	for (i=0; i<SEC_NUM; i++)
	{
		busy_cursor();
		if (rsize[i])
		{
			_relocs[nrelocs] = (SymRelocs*) SYM_NEW(SymRelocs((SymSec)i,nobjs(),rsize[i]));
			nrelocs++;
		}
	}
	_nrelocs = nrelocs;

#ifdef OLDRELS
	//figure out which relocations we'll need and add them
	add_relocations();
#endif
	//set up sections
	for (i=0; i<_nrelocs; i++)
	{
		secnum = add_master_sec(_relocs[i]->_sectype,0);
		_sections->set_size(secnum,_relocs[i]->_nents*_relocs[i]->_entsize);
	}
}

//-------------------------------------------
void ElfLinker::add_relocations()
//figure out which relocations we'll need and add them
//-------------------------------------------
{
	int i;
	ObjInfo* o;
	SymSec sectype;
	int secnum;
	//fill in _relocs
	for (i=0; i<_nrelocs; i++)
	{
		sectype = _relocs[i]->_sectype;
		for (o=firstobj(); o; o=o->next())
		{
			busy_cursor();
			_relocs[i]->_objs[o->_objind]=_relocs[i]->_nents;	//this index starts the relocations for obj objind
			DBG_(ELF,("set _relocs[%d]->_objs[%d]=%d obj %s base=x%X\n",i,o->_objind,_relocs[i]->_nents,o->_fp->filename(),o->_base));
			//secnum should be .rela.* sections
			if (secnum=o->_sections->secnum(sectype),secnum)
			{
				uint8* relsecbuf;
				if (relsecbuf=read_obj_section(o,secnum),relsecbuf)
				{
					add_relocs(_relocs[i],o,secnum,relsecbuf);
					//FREE(relsecbuf);
					LINK_DELETE_ARRAY(relsecbuf);
					}
				}
			}
		}
#ifdef OLDRELS
	//remove empty _relocs and adjust array
	i=0;
	while (i<_nrelocs) {
		if (_relocs[i]->_nents==0) {
			SYM_DELETE(_relocs[i]);
			_nrelocs--;
			_relocs[i] = _relocs[_nrelocs];
			}
		else i++;
		}
#else
	//set up my sizes
	for (i=0; i<_nrelocs; i++) {
		secnum = _sections->secnum(_relocs[i]->_sectype);
		DBG_ASSERT(secnum);
		_sections->set_size(secnum,_relocs[i]->_nents*_relocs[i]->_entsize);
		}
    link_section_hdrs();	//have to link section headers again because
							//the sections which follow relocations will
							//need to be adjusted
#endif
} // ElfLinker::add_relocations

//-------------------------------------------
void ElfLinker::add_relocs(SymRelocs* relocs,ObjInfo* o,int secnum, unsigned char* relsecbuf)
//called from create_relocs to add relocations from obj's relocations 
//to relocs which apply to absolute addresses
//Throw away relocations on absolute symbols since we can fix them up
//-------------------------------------------
{
	DBG_ENT("add_relocs");
	Elf32_Shdr* rsh=o->_sec_hdr+secnum;
    	unsigned char *rptr, *newrptr;	//ptr to relocation
    	Elf32_Rela *r, *newr;	//relocation entry

    	Boolean only_imports;

    	// If the executable is supposed to be absolute, the only thing we will
	// output is the import relocations.  This is needed for OS components
    	// which are mainly absolute, but they import symbols

    	only_imports = (Boolean)!_link_opts->isset(linkopt_relative);

	newrptr=relocs->_buf + (relocs->_nents*rsh->sh_entsize);	//relocs so far
	
	//relocs->_objs[o->_objind]=relocs->_nents;	//this index starts the relocations for obj objind
	newr = (Elf32_Rela*)newrptr;
	for (rptr=relsecbuf;rptr<relsecbuf+rsh->sh_size;rptr+=rsh->sh_entsize)
	{
		r = (Elf32_Rela*)rptr;
		swap_reloc(r);
		//all absolute relocations sould have a relative counterpart
		//don't generate relocs for absolute symbols; they will be fixed up at link time
		//SOMEDAY!! need to handle relocs on undef symbols here 
		//(they won't have a secnum!)  
		//tho, if defined, we'll need to remove reloc later
	//was:
		//if (SYMSECNUM(r)==SHN_UNDEF
		//	|| isabs(RTYPE(r)) && SYMSECNUM(r)!=SHN_ABS && SYMSECNUM(r)!=SHN_COMMON)
//=======
//SOMEDAY
//make this into function emit_reloc to return true/false
//this code is same as that in update_reloc
		Boolean emit_reloc = false;
		uint32 rtype = ELF32_R_TYPE(r->r_info);	
		uint32 symidx = ELF32_R_SYM(r->r_info);	//index from obj
		if (symidx >= get_symtab_nsyms(o))
		{
			SET_ERR(se_fail,("symidx=%d of obj=%s out of range! (nsyms=%d)\n",
				symidx,o->_fp->filename(),get_symtab_nsyms(o)));
			return;
		}
		uint32 secnum = o->_symtab[symidx].st_shndx;
		const char* name = get_symname(o,symidx);
		if (!name || !*name)
		{
			SET_ERR(se_fail,("no name for symbol %d(x%X) referenced from obj %s\n",symidx,symidx,o->_fp->filename()));
			return;
		}
		uint32 defsymidx=symidx;
		ObjInfo* defo=o;
		//if sectype was und/common in old obj, 
		//try to find where sym was defined and use its secnum
		if (symidx && (secnum==SHN_UNDEF || secnum==SHN_COMMON))
		{
			//emit relocations on und symbols (imps will be unds)
#ifndef OLD_DLL
//for imports, must add token undef when the imp is resolved
//so that find_sym will be able to test sym->st_shndx
#endif
			if (!find_sym(defsymidx,defo,name))
				emit_reloc = true;
		}
//========
		if (isabs(RTYPE(r)) && SYMSECNUM(r)!=SHN_ABS && !only_imports)
			emit_reloc = true;
		if (emit_reloc)
		{
			newr->r_offset = r->r_offset;
			newr->r_info = r->r_info;
			newr->r_addend = r->r_addend;
			newrptr += relocs->_entsize;
			newr = (Elf32_Rela*)newrptr;
			relocs->_nents++;
		}
	}
} // ElfLinker::add_relocs

//-------------------------------------------
void ElfLinker::update_relocations()
//update reloc with PPC_REL* or new symidx if PPC_ADDR* generated 
//-------------------------------------------
{
	DBG_ENT("update_relocations");
    unsigned char* rptr;	//ptr to relocation
    Elf32_Rela* r;	//relocation entry
	SymRelocs* p;
	ObjInfo* o;
	int i,ridx,oind;
	for (i=0; i<_nrelocs; i++) {
		p=_relocs[i];
		ridx=0; rptr=p->_buf; 
		for (o=firstobj(oind); o; o=o->next(oind)) {
			DBG_(ELF,("_relocs[%d]->_objs[%d]=%d; obj=%s, baseoff=x%X\n",i,oind,p->_objs[oind],o->_fp->filename(),o->_base));
			int osecnum = o->_sections->secnum(_relocs[i]->_sectype);
			if (!osecnum)
				continue;
			Elf32_Shdr *s = o->_sec_hdr + osecnum;
			if (!RELA(s)) { 
				SET_ERR(se_fail,("invalid relocation types!\n")); 
				return;
				}
			//update the relocations for this obj
			for (; (oind+1>=nobjs() || ridx<p->_objs[oind+1]) 
					&& ridx<p->_nents; ridx++, rptr+=p->_entsize) {
				busy_cursor();
				DBG_ASSERT(rptr<p->_buf+p->_nents*p->_entsize);
				r = (Elf32_Rela*)rptr;
				update_reloc(r,o,s->sh_info);
			}
		}
	}
} // ElfLinker::update_relocations()

//-------------------------------------------
Boolean ElfLinker::find_sym(uint32& defsymidx,ObjInfo*& defo, const char* name)
//attempt to find symbol def if it's undefined
//if not found, do not modify defsymidx & defo
//-------------------------------------------
{
	_linkhash* h = get_hash(name,hoff_global);
	if (!h) {
		DBG_(PICKY,("symbol %s not found in hash table!\n",name))
		return false;
		}
	defo = obj(h->_objind);
	DBG_ASSERT(defo);
	defsymidx = h->_symidx;
	DBG_ASSERT(defsymidx);
	Elf32_Sym* defsym = &defo->_symtab[defsymidx];
	if (defsym->st_shndx==SHN_UNDEF)
	{
		DBG_WARN(("symbol %s is undefined! sidx=%d, oind=%d\n",name,h->_symidx,h->_objind))
		return false;
		}
	return true;
} // ElfLinker::find_sym(uint32&,ObjInfo*&,const char*)
			
//-------------------------------------------
void ElfLinker::update_reloc(Elf32_Rela* r, ObjInfo* o,int osecnum)
//update reloc with PPC_REL* or new symidx if PPC_ADDR* generated 
//called after symtabs merged
//update_reloc:
//	for relocations which are external/undefined/don't have a new reloc,
//		emit old relocation:
//			info'->type = info->type
//			info'->symidx = newsymidx(sym) (symidx of symbol in created symtab)
//			offset' = offset + new_position_adjustment
//			addend' = addend
//	for new relocations:
//			info'->type = newreloc(info->type)
//			info'->symidx = newsec(sym->st_shndx) (secnum of base to get relocated)
//			offset' = offset + new_position_adjustment
//			addend' = sym->value + addend
//-------------------------------------------
{		
	DBG_ASSERT(r);
	DBG_ASSERT(o);
	DBG_ASSERT(osecnum);
	DBG_ASSERT(_link_opts->isset(linkopt_relative)); 
	uint32 salign = get_shalign(o->_sec_hdr+osecnum,o->_sections->sectype(osecnum));
	DBG_(ELF,(" r->r_info=%#8lx, rtype=%#3lx, symidx=%s(%d), addend=0x%lx\n",
		r->r_info, RTYPE(r), get_symname(o,SYMIDX(r)), SYMIDX(r), r->r_addend));
		
		
	//-----------------------------------------------
	//get info & find new symbol
	//old values
	uint32 rtype = ELF32_R_TYPE(r->r_info);	
	uint32 symidx = ELF32_R_SYM(r->r_info);	//index from obj
	if (symidx >= get_symtab_nsyms(o))
	{
		SET_ERR(se_fail,("symidx=%d of obj=%s out of range! (nsyms=%d)\n",
			symidx,o->_fp->filename(),get_symtab_nsyms(o)));
		return;
	}
	uint32 secnum = o->_symtab[symidx].st_shndx;
	const char* name = get_symname(o,symidx);
	if (!name || !*name)
	{
		SET_ERR(se_fail,("no name for symbol %d(x%X) referenced from obj %s\n",symidx,symidx,o->_fp->filename()));
		return;
	}
	uint32 defsymidx=symidx;
	ObjInfo* defo=o;
	//if sectype was und/common in old obj, 
	//try to find where sym was defined and use it's secnum
	Syms::Sym* import=0;	//in case this thing is an import
	if (symidx && (secnum==SHN_UNDEF || secnum==SHN_COMMON))
	{
		//allow relocations on und/imp symbols
		//find_sym allows common symbols if they are the token symbol
		if (!find_sym(defsymidx,defo,name))
		{
			DBG_WARN(("relocation on unresolved undef/common symbol %s!\n",name));
			//imports will be undefineds, but with a different relocation type
			if (import = _imps->find(name),import)
			{	
				DBG_WARN(("relocation on imported undef/common symbol %s!\n",name));
			}
			else
			{
				if (_link_opts->isset(linkopt_incremental))
				{
					DBG_WARN(("relocation for unresolved symbol %s!!  symidx=%d objind %d (%s)\n",name,symidx,o->_objind,o->_fp->filename()));
				}
				else if (_link_opts->isset(linkopt_allow_unds)) { 
					SET_INFO(se_und_symbols,("relocation for unresolved symbol %s!!  symidx=%d objind %d (%s)\n",name,symidx,o->_objind,o->_fp->filename()));
				}
				else
				{
					fprintf(_user_fp,"# Error: relocation on undefined symbol %s referenced from obj %s\n",name,o->_fp->filename());
					SET_ERR(se_fail,("relocation for unresolved symbol %s!!  symidx=%d objind %d (%s)\n",name,symidx,o->_objind,o->_fp->filename()));
					return;
				}
			}
		}
	}
	Elf32_Sym* defsym = &defo->_symtab[defsymidx];
	DBG_(ELF,(" r->r_info=%#8lx, rtype=%#3lx, symidx=%s(%d), offset=x%X, addend=0x%lx\n",
		r->r_info, rtype, name, symidx, r->r_offset, r->r_addend));

	//new values
	uint32 newsymidx = get_newsymidx(name,scopeid(defsym, defo->_objind));
	uint32 newrtype;
	if (import)
	{
		newrtype = get_newimpreltype(rtype);
		DBG_ASSERT(ISA_IMPREL(newrtype));
	}
	else
	{
		newrtype = get_reltype(rtype);
		DBG_ASSERT(!newrtype || ISA_BASEREL(newrtype));
	}
	DBG_ASSERT(!newrtype || newrtype<256);

	//SOMEDAY.  this code is similar to that of apply_reloc -
	//should combine them!

	//-----------------------------------------------
	//update relocation
	//adjust section offset to account for where 
	//object got relocated in section
	//result will be offset within new section
	r->r_offset += o->_sections->baseaddr(osecnum);
	DBG_(ELF,("symbol=%s obj=%s newrtype=x%X: adjusted r->r_offset=x%X, osecnum=%d\n",
		name,o->_fp->filename(),newrtype,r->r_offset,osecnum));
	//new relocation type?
	if (newrtype)
	{
		//emit new type
		DBG_ASSERT(defsym);
		SymSec sectype;
		uint32 newsecnum;
		switch (defsym->st_shndx)
		{
			case SHN_COMMON: 	
					sectype = sec_data; 
					newsecnum = _sections->secnum(sectype);
					break;
			case SHN_ABS: //error - should have applied already
					Syms::Sym* spec;
					if (!(spec=_specs->find(name),spec)) {	//special symbols may be abs
						SET_ERR(se_fail,("have reloc on abs symbol %s!!  symidx=%d objind %d (%s)\n",name,symidx,o->_objind,o->_fp->filename()));
						return;
						}
					sectype = (SymSec)spec->_ordinal; 
					newsecnum = _sections->secnum(sectype);
					break;
			case SHN_UNDEF: 	//Can't emit new reloc on undefined symbol.	
					if (!import) {
						SET_ERR(se_fail,("new reloc on undef symbol!\n")); 
						return;
						}
					sectype = sec_und; 
					newsecnum = SHN_UNDEF;
					break;
			default: 		
					sectype = defo->_sections->sectype(defsym->st_shndx);
					newsecnum = _sections->secnum(sectype);
		}
		if (import)
		{
			//R_SYM becomes 
				//bits 0-7 = mod_ord (index into .imp3do section)
				//bits 8-23 = sym_ord (ordinal of symbol from .exp3do of imported module)
			r->r_info=ELF32_R_IMPINFO(import->_ordinal,import->_symidx,newrtype);	
			//r->r_addend stays the same 
			DBG_(ELF,("emitting IMPREL relocation - mod_ord=%d, imp_ind=x%X, sym_ord=%d, r->r_info=x%X\n",import->_objind,import->_ordinal,import->_symidx, r->r_info));
		}
		else
		{
			//new relocations get secnum in their info field 
			//info->symidx becomes secnum instead of symidx
			r->r_info=ELF32_R_INFO(newsecnum,newrtype);	//use new relocation type
			//r->r_addend becomes the old value + addend
			r->r_addend += defsym->st_value;
		}
	}
	else
	{	//emit old reloc type
		if (!newsymidx)
		{
			//must have either a newsymidx or a newrtype!
			SET_ERR(se_fail,("can't emit reloc for symbol x%X %s referenced from obj %s (no new relocation or symbol)\n",symidx,name,o->_fp->filename()));
			return;
		}
		Elf32_Sym* newsym = &_symtab[newsymidx];
		DBG_ASSERT(newsym);		//must have a new sym!
		//make sure secnum is valid
		uint32 newsecnum = newsym->st_shndx;
		switch (newsecnum)
		{
			case SHN_UNDEF: //emit old type
				DBG_WARN(("relocation for undefined symbol %s!!  symidx=%d objind %d (%s)\n",name,symidx,o->_objind,o->_fp->filename()));
				break;
			case SHN_COMMON: //this must be the token common symbol
				DBG_WARN(("relocation for common symbol %s!!  symidx=%d objind %d (%s)\n",name,symidx,o->_objind,o->_fp->filename()));
				break;
			case SHN_ABS: //error - should have applied already
				Syms::Sym* spec;
				if (!(spec=_specs->find(name),spec)) {	//special symbols may be abs
					SET_ERR(se_fail,("have reloc on abs symbol %s!!  symidx=%d objind %d (%s)\n",name,symidx,o->_objind,o->_fp->filename()));
					return;
					}
				break;
			default: ;
		}
		r->r_info=ELF32_R_INFO(newsymidx,rtype);	//keep old relocation type
		//r->r_addend stays the same 
	}
} // ElfLinker::update_reloc(Elf32_Rela*,ObjInfo*,int)
	
//-------------------------------------------
SymErr ElfLinker::apply_relocations(ObjInfo* o, int i, unsigned char* secbuf, unsigned char* relsecbuf)
//apply reloc with PPC_REL* or new symidx if PPC_ADDR* generated 
//called after symtabs merged
//apply_reloc:
//	for relocations which can be applied (symbol is defined),
//		apply relocation:
//			info'->type = info->type
//			info'->symidx = newsymidx(sym) (symidx of symbol in created symtab)
//			offset' = offset 
//			addend' = addend
//			apply relocationj calculation
//-------------------------------------------
{
    DBG_ENT("apply_relocations");
    unsigned char* rptr;	//ptr to relocation
    Elf32_Rela* r;	//relocation entry
	DBG_ASSERT(o);
	Elf32_Shdr *s = o->_sec_hdr + i;

	if (!RELA(s))
	{
		RETURN_ERR(se_fail,("invalid relocation types!\n")); 
	}
	unsigned char* lastptr = relsecbuf + s->sh_size;
	for (rptr = relsecbuf; rptr < lastptr; rptr += s->sh_entsize)
	{
		r = (Elf32_Rela*)rptr;
		swap_reloc(r);
		apply_reloc(r, o, secbuf, s->sh_info);
	}
	return state();
} // ElfLinker::apply_relocations

//-------------------------------------------
void ElfLinker::apply_reloc(Elf32_Rela* r, ObjInfo* o, unsigned char* secbuf, uint32 osecnum)
//-------------------------------------------
{
	//old values
	uint32 symidx = ELF32_R_SYM(r->r_info);	//index from obj
	if (symidx >= get_symtab_nsyms(o))
	{
		SET_ERR(se_fail,("symidx=%d of obj=%s out of range! (nsyms=%d)\n",
			symidx,o->_fp->filename(),get_symtab_nsyms(o)));
		return;
		}
	const char* name = get_symname(o,symidx);
	if (!name || !*name)
	{
		SET_ERR(se_fail,("no name for symbol %d(x%X) referenced from obj %s\n",symidx,symidx,o->_fp->filename()));
		return;
		}
	uint32 rtype = ELF32_R_TYPE(r->r_info);	
	uint32 offset = r->r_offset;
	uint32 addend = r->r_addend;
	DBG_(ELF,(" r->r_info=%#8lx, rtype=%#3lx, symidx=%s(%d), addend=0x%lx",
		r->r_info, RTYPE(r), get_symname(o,SYMIDX(r)), SYMIDX(r), r->r_addend));

	//Elf32_Sym* sym = &o->_symtab[symidx];
	//SymSec sectype = get_symsectype(o,symidx);
	uint32 defsymidx = symidx;
	ObjInfo* defo = o;
	//if sectype was und/common in old obj, 
	//try to find where sym was defined and use its secnum
	uint32 secnum = o->_symtab[symidx].st_shndx;
	if (symidx && (secnum==SHN_UNDEF || secnum==SHN_COMMON))
	{
		if (!find_sym(defsymidx, defo, name) && !_imps->find(name))
		{
			if (!_link_opts->isset(linkopt_allow_unds)
					&& !_link_opts->isset(linkopt_relative))
			{
				fprintf(_user_fp,"# Error: relocation found on undefined symbol %s!\n",name);
				SET_ERR(se_fail,("relocation on unresolved undef/common symbol %s!\n",name));
			}
			else
			{
				//no biggie - we'll just emit a relocation type for this guy in update_relocs...
				DBG_WARN(("# ignoring relocation on unresolved undef/common symbol!\n"));
			}
		return;
		}
	}
	Elf32_Sym* defsym = &defo->_symtab[defsymidx];
	DBG_(ELF,(" r->r_info=%#8lx, rtype=%#3lx, symidx=%s(%d), offset=x%X, addend=0x%lx\n",
		r->r_info, rtype, name, symidx, offset, addend));

	//new values
	uint32 newsymidx = get_newsymidx(name, scopeid(defsym, defo->_objind));

	//no need to adjust section offset to account for where 
	//object got relocated in section
	// - this section is the local section piece which starts
	//at offset 0!
	//if have a new symbol, make sure secnum is valid
	//new symbol is undefined/absolute/common?  
	if (newsymidx)
	{
		Elf32_Sym* newsym = newsymidx ? &_symtab[newsymidx] : 0;
		switch (newsym->st_shndx)
		{
			case SHN_COMMON:
				DBG_WARN(("reloc on common symbol\n"));
			case SHN_UNDEF:
				//OK to be undefined if this is an incremental link or an import
				if (_link_opts->isset(linkopt_relative) || _imps->find(name))
				{
					//if undef, ignore - should emit a relocation to handle it
					DBG_WARN(("undef symbol - can't apply relocation for symbol x%X referenced from obj %s\n",symidx,o->_fp->filename()))
				}
				else
				{
					SET_ERR(se_fail,("undefined symbol not resolved! symbol x%X referenced from obj %s\n",symidx,o->_fp->filename()))
					return;
					}
				break;
			case SHN_ABS:
				//if abs, don't care...??
				break;
			default:
				DBG_(ELF,("r->r_offset=x%X, newsecnum=%d, defsecnum=%d\n",
					r->r_offset,newsym->st_shndx, defsym->st_shndx));
		} // switch
	}

	//old symbol's value will have been fixed up in reloc_symtab
	//BUT, symbol's secnum will still be old secnum.
	//B(r)		SYMSECBASE(r)
	//G(r)		(SET_ERR(se_fail,("GOT relocations not allowed!\n")),0)
	//L(r)		(SET_ERR(se_fail,("PLT relocations not allowed!\n")),0)
	//P(r)		r->r_offset	//???
	//R(r)		SYM(r)->st_value - SYMSECOBJBASE(r)	//offset of symbol from section
	//S(r)		SYM(r)->st_value
	//note, if undef, will have a new o & secnum & sym

#undef A
#undef B
#undef G
#undef L
#undef P
#undef R
#undef S
//Kludge -
//what I'm trying to do is to make it such that if the symbol were
//undefined, then these would pick up the correct values 
//A(r) addend (doesn't change)
	#define A(r) (r->r_addend)
//B(r) base offset of section = the new base position of the object's section
//kevinh - We add in the base address of the output executable's section
	#define B(r) (o->_sections->baseaddr(osecnum) + get_shbase(o->_sections->sectype(osecnum)))
//errors (for GOT and PLT relocs)
	#define G(r) 0
	#define L(r) 0
//P(r) should be the the position of the symbol relative to the offset
//which is being fixed up; this becomes offset + new_section_position
	//offset within section (since secbuf is obj's section)
	#define P(r) (r->r_offset + B(r))
	//#define P(r) (r->r_offset)
//S(r) new relocated symbol value
	#define S(r) (defsym->st_value)
//R(r) relative offset of symbol from section it's defined in
	//#define R(r) (S(r) - (defo->_sec_hdr[defsecnum].sh_addr))
	#define R(r) (S(r) - (defo->_sections->baseaddr(defsym->st_shndx)))


	uint8* saddr = secbuf + r->r_offset;
	uint32 salign = get_shalign(o->_sec_hdr+osecnum,o->_sections->sectype(osecnum));
	DBG_(ELF,("relocation saddr=x%X(soffset=x%X), salign=x%X, S(r)=x%X, A(r)=x%X, P(r)=x%X, B(r)=x%X\n",
		saddr, r->r_offset, salign, S(r), A(r), P(r), (SYMSECNUM(r)==SHN_ABS||SYMSECNUM(r)==SHN_COMMON)?0:B(r)));

    switch(RTYPE(r))
    {
	//standard SVR4 relocations:
        case R_PPC_ADDR32:
        	set_word32(saddr, salign, S(r) + A(r));
        	break;
        case R_PPC_ADDR24:
        	set_low24(saddr, salign, (S(r) + A(r))>>2);	
        	break;
        case R_PPC_ADDR16:
        	set_half16(saddr, salign, S(r) + A(r));
        	break;
        case R_PPC_ADDR16_LO:
			DBG_(ELF,("S(r)=x%X\n",S(r)));
			DBG_(ELF,("A(r)=x%X\n",A(r)));
			DBG_(ELF,("(S(r) + A(r))=x%X\n",(S(r) + A(r))));
			DBG_(ELF,("LO(S(r) + A(r))=x%X\n",LO(S(r) + A(r))));
        	set_half16(saddr,salign, LO(S(r) + A(r)));
#ifdef DDI_BUG_TEST
			if (r->r_offset==0x22)
			{
        		set_half16(saddr, salign, LO(3));
			}
#endif
        	break;
        case R_PPC_ADDR16_HI:
        	set_half16(saddr, salign, HI(S(r) + A(r)));
        	break;
        case R_PPC_ADDR16_HA:
        	set_half16(saddr, salign, HA(S(r) + A(r)));
        	break;
        case R_PPC_ADDR14:
        case R_PPC_ADDR14_BRTAKEN:
        case R_PPC_ADDR14_BRNTAKEN:
			//SOMEDAY: may want/need to set LOW14_BRN if BRNTAKEN
			this->_current_objfilename = o->_name;
			this->_current_offset_in_section = r->r_offset;
        	set_low14(saddr, salign, (S(r) + A(r)));
        	break;
        case R_PPC_REL24:
			DBG_(ELF,("S(r)=x%X\n",S(r)));
			DBG_(ELF,("A(r)=x%X\n",A(r)));
			DBG_(ELF,("P(r)=x%X\n",P(r)));
			DBG_(ELF,("(S(r) + A(r) - P(r))=x%X\n",(S(r) + A(r) - P(r))));
			DBG_(ELF,("(S(r) + A(r) - P(r))>>2=x%X\n",(S(r) + A(r) - P(r))>>2));
			DBG_(ELF,("((S(r) + A(r) - P(r))>>2)<<2=x%X\n",((S(r) + A(r) - P(r))>>2)<<2));

        	set_low24(saddr,salign, (S(r) + A(r) - P(r))>>2);	
        	break;
        case R_PPC_REL14:
        case R_PPC_REL14_BRTAKEN:
        case R_PPC_REL14_BRNTAKEN:
			//SOMEDAY: may want/need to set LOW14_BRN if BRNTAKEN
			// Stuff info into our object fields to allow error reporting
			// from called routines
			this->_current_objfilename = o->_name;
			this->_current_offset_in_section = r->r_offset;
        	set_low14(saddr, salign, (S(r) + A(r) - P(r)));	
        	break;
        case R_PPC_UADDR32:	//datum to be relocated can be unaligned
        	set_word32(saddr,1, S(r) + A(r));
        	break;
        case R_PPC_UADDR16:	//datum to be relocated can be unaligned
        	set_half16(saddr,1, S(r) + A(r));
        	break;
        case R_PPC_RELATIVE:
        	set_word32(saddr,salign, B(r) + A(r));
        	break;
        case R_PPC_REL32:
        	set_word32(saddr,salign, S(r) + A(r) - P(r));
        	break;
        case R_PPC_SECTOFF:
        	set_half16(saddr,salign, R(r) + A(r));
			break;
        case R_PPC_SECTOFF_LO:
        	set_half16(saddr,salign, LO(R(r) + A(r)));
			break;
        case R_PPC_SECTOFF_HI:
        	set_half16(saddr,salign, HI(R(r) + A(r)));
			break;
        case R_PPC_SECTOFF_HA:
        	set_half16(saddr,salign, HA(R(r) + A(r)));
			break;

	//new relocation types for elf3do:
	//these use the symbol field as a section number
        case R_PPC_BASEREL32:
        	set_word32(saddr,salign, BREL(r) + A(r));
        	break;
        case R_PPC_BASEREL24:
        	set_low24(saddr,salign, (BREL(r) + A(r))>>2);	
        	break;
        case R_PPC_BASEREL16:
        	set_half16(saddr,salign, BREL(r) + A(r));
        	break;
        case R_PPC_BASEREL16_LO:
        	set_half16(saddr,salign, LO(BREL(r) + A(r)));
        	break;
        case R_PPC_BASEREL16_HI:
        	set_half16(saddr,salign, HI(BREL(r) + A(r)));
        	break;
        case R_PPC_BASEREL16_HA:
        	set_half16(saddr,salign, HA(BREL(r) + A(r)));
        	break;
        case R_PPC_BASEREL14:
        case R_PPC_BASEREL14_BRTAKEN:
        case R_PPC_BASEREL14_BRNTAKEN:
			//SOMEDAY may need to set BRNTAKEN
			// Stuff info into our object fields to allow error reporting
			// from called routines
			this->_current_objfilename = o->_name;
			this->_current_offset_in_section = r->r_offset;
        	set_low14(saddr, salign, (BREL(r) + A(r)));	
        	break;

	//new relocation types for elf3do:
	//these use the symbol field as library_ord (1st byte) & symbol_ord (last 3 bytes)
        case R_PPC_IMPREL32:
        	set_word32(saddr,salign, BIMP(r) + A(r));
        	break;
        case R_PPC_IMPREL24:
        	set_low24(saddr,salign, (BIMP(r) + A(r))>>2);	
        	break;
        case R_PPC_IMPREL14:
        case R_PPC_IMPREL14_BRTAKEN:
        case R_PPC_IMPREL14_BRNTAKEN:
			//SOMEDAY may need to set BRNTAKEN
			// Stuff info into our object fields to allow error reporting
			// from called routines
			this->_current_objfilename = o->_name;
			this->_current_offset_in_section = r->r_offset;
        	set_low14(saddr,salign, (BIMP(r) + A(r)));	
        	break;
        case R_PPC_IMPADDR32:
        	set_word32(saddr, salign, BIMP(r) + A(r));
        	break;
        case R_PPC_IMPADDR24:
        	set_low24(saddr, salign, (BIMP(r) + A(r))>>2);	
        	break;
        case R_PPC_IMPADDR16:
        	set_half16(saddr, salign, BIMP(r) + A(r));
        	break;
        case R_PPC_IMPADDR16_LO:
        	set_half16(saddr, salign, LO(BIMP(r) + A(r)));
        	break;
        case R_PPC_IMPADDR16_HI:
        	set_half16(saddr, salign, HI(BIMP(r) + A(r)));
        	break;
        case R_PPC_IMPADDR16_HA:
        	set_half16(saddr, salign, HA(BIMP(r) + A(r)));
        	break;
        case R_PPC_IMPADDR14:
        case R_PPC_IMPADDR14_BRTAKEN:
        case R_PPC_IMPADDR14_BRNTAKEN:
			//SOMEDAY may need to set BRNTAKEN
			this->_current_objfilename = o->_name;
			this->_current_offset_in_section = r->r_offset;
        	set_low14(saddr,salign, (BIMP(r) + A(r)));	
        	break;
        case R_PPC_UIMPADDR16:
        	set_half16(saddr,1, BIMP(r) + A(r));
        	break;
        case R_PPC_UIMPADDR32:
        	set_word32(saddr,1, BIMP(r) + A(r));
        	break;

	//unsupported relocations:
		//not supported or should not occur in objects
        case R_PPC_NONE: 
        case R_PPC_COPY:	//for dynamic linking
			fprintf(_user_fp,"# Error: NONE/COPY relocations not supported!\n");
			SET_ERR(se_fail,("NONE/COPY relocations found in object!\n"));
        	break;
        case R_PPC_GOT16:
        case R_PPC_GOT16_LO:
        case R_PPC_GOT16_HI:
        case R_PPC_GOT16_HA:
        case R_PPC_GLOB_DAT:	//sets GOT entry to addr of symbol
        case R_PPC_LOCAL24PC:	//symbol referenced is normally GOT
			fprintf(_user_fp,"# Error: GOT relocations not supported!\n");
			SET_ERR(se_fail,("GOT relocations not supported!\n"));
			break;
        case R_PPC_PLTREL24:
        case R_PPC_PLTREL32:
        case R_PPC_PLT16_LO:
        case R_PPC_PLT16_HI:
        case R_PPC_PLT16_HA:
        case R_PPC_JMP_SLOT:	//for dynamic linking - location of PLT entry
			fprintf(_user_fp,"# Error: PLT relocations not supported!\n");
			SET_ERR(se_fail,("PLT relocations not supported!\n"));
			break;
        case R_PPC_SDAREL16:
			fprintf(_user_fp,"# Error: PPC_SDAREL16 relocations not supported!\n");
			SET_ERR(se_fail,("SDAREL16 relocations not supported!\n"));
        	//set_half16(saddr,salign, S(r) + A(r) - _SDA_BASE_);
			break;
		//embedded relocs: //101-105 same ppc_emb_addr32==ppc_addr32
        case 101:	
        case 102:	
        case 103:	
        case 104:	
        case 105:	
			fprintf(_user_fp,"# Error: embedded relocations not yet supported!\n");
			SET_ERR(se_fail,("embedded relocations not yet supported!\n"));
			break;
        case 106:	//small data
        case 107:	//small data
        case 108:	//small data
        case 109:	//small data
        case 116:	//small data
			fprintf(_user_fp,"# Error: relocations on small data section not supported!\n");
			SET_ERR(se_fail,("relocations on small data section not supported!\n"));
			break;
        default:
			fprintf(_user_fp,"# Error: unknown relocation type %d found in file %s!\n",RTYPE(r),o->_fp->filename());
			SET_ERR(se_fail,("unknown relocation type=x%X in objind=%d base=x%X  file %s!\n",RTYPE(r),o->_objind,o->_base,o->_fp->filename()));
	} // switch
} // ElfLinker::apply_reloc()

//==========================================================================
// writers - funcs to write elf file

#ifdef DEBUG	//to make sure sizes & offsets match what was calculated
	uint32 foff;
	uint32 fsize;
#endif

//======================================
//	Compression bottlenecks
// Mimics fwrite(), but does compression.
//======================================

#define COMP_BUF_SIZE 1024

//======================================
struct LinkerCompInfo
//======================================
{
	LinkerCompInfo(FILE* file);
	~LinkerCompInfo();
	
// data
	Compressor* fCompressor;
	Err fErr;
	FILE* fFile;
    uint8 fAccumBuf[COMP_BUF_SIZE];
    uint32 fBytesInBuf;
    uint32 fBytesOutput;
}; // class LinkerCompInfo

extern "C" {
	void LinkerCompFunc(void *userData, uint32 word);
}
size_t cwrite(
	LinkerCompInfo* compInfo,
	const void * buf,
	size_t count,
	size_t objlen,
	FILE* file);

//-------------------------------------------
void LinkerCompFunc(void *userData, uint32 word)
//-------------------------------------------
{
	fwrite(&word, 1, sizeof(uint32), (FILE*)userData);
}

//-------------------------------------------
LinkerCompInfo::LinkerCompInfo(FILE* file)
//-------------------------------------------
:	fCompressor(NULL)
,	fErr(0)
,   fBytesInBuf(0)
,   fBytesOutput(0)
{
	CreateCompressor(&fCompressor, LinkerCompFunc, file);
}

//-------------------------------------------
LinkerCompInfo::~LinkerCompInfo()
//-------------------------------------------
{
    if (fBytesInBuf != 0)
    {
        while (fBytesInBuf % 4)
            fAccumBuf[fBytesInBuf++] = 0;
        FeedCompressor(fCompressor, fAccumBuf, fBytesInBuf / 4);
    }
	DeleteCompressor(fCompressor);
}

//-------------------------------------------
size_t cwrite(
	LinkerCompInfo* info,
	const void * buf,
	size_t count,
	size_t objlen,
	FILE* file)
// Mimics fwrite(), but does compression.
//-------------------------------------------
{
	if (info && info->fCompressor)
	{
        size_t reqBytes = count * objlen;
        size_t numBytes = reqBytes;
        Err err = 0;
        size_t room;
        uint8 *source;

        info->fBytesOutput += reqBytes;

        source = (uint8 *)buf;
        room = COMP_BUF_SIZE - info->fBytesInBuf;
        while (err >= 0 && numBytes >= room)
        {
            memcpy(&info->fAccumBuf[info->fBytesInBuf], source, room);
            err = FeedCompressor(info->fCompressor, info->fAccumBuf, COMP_BUF_SIZE / 4);
            numBytes          -= room;
            source             = &source[room];
            room               = COMP_BUF_SIZE;
            info->fBytesInBuf  = 0;
        }
		if (err < 0)
			return 0;
        memcpy(&info->fAccumBuf[info->fBytesInBuf], source, numBytes);
        info->fBytesInBuf += numBytes;

        return reqBytes;
    }

	return fwrite(buf, count, objlen, file);
}

//-------------------------------------------
void ElfLinker::RecordSectionOffset(Elf32_Shdr* sh, const char* name)
//-------------------------------------------
{
	sh->sh_offset = ftell(_link_fp);
} // ElfLinker::RecordSectionOffset

//-------------------------------------------
void ElfLinker::RecordSectionSize(Elf32_Shdr* sh, const char* name)
//-------------------------------------------
{
	sh->sh_size = ftell(_link_fp) - sh->sh_offset;
} // ElfLinker::RecordSectionSize

//-------------------------------------------
void ElfLinker::write_elf_hdr()
//-------------------------------------------
{
	DBG_(ELF,("elf hdr written at offset=x%X, (should be x%X)\n",foff=ftell(_link_fp),0));
	DBG_ASSERT(foff==0);
	busy_cursor();
	swap_elf_hdr(_elf_hdr);
    fwrite(_elf_hdr,sizeof(Elf32_Ehdr),1,_link_fp);
	swap_elf_hdr(_elf_hdr);
	DBG_(ELF,("elf hdr size=x%X, (should be x%X)\n",fsize=(ftell(_link_fp)-foff),_elf_hdr->e_ehsize));
	DBG_ASSERT(fsize==_elf_hdr->e_ehsize);
}

//-------------------------------------------
void ElfLinker::write_program_hdrs()
//-------------------------------------------
{
	DBG_(ELF,("program hdrs written at offset=x%X, (should be x%X)\n",foff=ftell(_link_fp),_elf_hdr->e_phoff));
	DBG_ASSERT(foff==_elf_hdr->e_phoff);
    Elf32_Phdr *p = _prog_hdr;
    for (int i=0; i<_elf_hdr->e_phnum; i++,p++)
    {
		busy_cursor();
		swap_prog_hdr(p);
    	fwrite(p, _elf_hdr->e_phentsize, 1, _link_fp);
		swap_prog_hdr(p);
    }
	DBG_(ELF,("prog hdrs size=x%X, (should be x%X)\n",fsize=(ftell(_link_fp)-foff),(_elf_hdr->e_phentsize*_elf_hdr->e_phnum)));
	DBG_ASSERT(fsize==(_elf_hdr->e_phentsize*_elf_hdr->e_phnum));
}

//-------------------------------------------
void ElfLinker::write_section_hdrs()
//-------------------------------------------
{
	DBG_ENT("write_section_hdrs");
	DBG_(ELF,("section hdrs written at offset=x%X, (should be x%X)\n",foff=ftell(_link_fp),_elf_hdr->e_shoff));
	DBG_ASSERT(foff==_elf_hdr->e_shoff);
    Elf32_Shdr *p = _sec_hdr;
	for (int i=0; i<_elf_hdr->e_shnum; i++,p++)
    {
		busy_cursor();
		swap_sec_hdr(p);
    	fwrite(p, _elf_hdr->e_shentsize, 1, _link_fp);
		swap_sec_hdr(p);
    }
	DBG_(ELF,("sec hdrs size=x%X, (should be x%X)\n",fsize=(ftell(_link_fp)-foff),(_elf_hdr->e_shentsize*_elf_hdr->e_shnum)));
	DBG_ASSERT(fsize==(_elf_hdr->e_shentsize*_elf_hdr->e_shnum));
} // ElfLinker::write_section_hdrs()

//-------------------------------------------
void ElfLinker::write_shstrtab()
//-------------------------------------------
{
	busy_cursor();
	Elf32_Shdr* sh = _sec_hdr + _elf_hdr->e_shstrndx;
    write_align(_link_opts->argval(linkopt_secalign));
    RecordSectionOffset(sh, "sh_strtab");
    uint32 sh_size = _sections->size(_elf_hdr->e_shstrndx);
    fwrite(_sh_strtab, sh_size, 1, _link_fp);
	RecordSectionSize(sh, "sh_strtab");
} // void ElfLinker::write_shstrtab()

//-------------------------------------------
void ElfLinker::write_strtab()
//-------------------------------------------
{
	DBG_ENT("write_strtab");
	busy_cursor();
	uint32 count = _nstrs;
	// If stripping is on, there's no need for either of these
	// sections.  They're taking up ROM space and the loader
	// never reads 'em anyway.  Set the lengths to zero so they're not written
	Elf32_Shdr* sh = _sec_hdr + _sections->secnum(sec_strtab);
    write_align(_link_opts->argval(linkopt_secalign));
    RecordSectionOffset(sh, "strtab");
    if (!_link_opts->isset(linkopt_stripmore))
	    fwrite(_strtab, count, 1, _link_fp);
	RecordSectionSize(sh, "strtab");
} // ElfLinker::write_strtab()
    
//-------------------------------------------
void ElfLinker::write_symtab()
//-------------------------------------------
{
	DBG_ENT("write_symtab");
	busy_cursor();
	uint32 count = _nsyms;
	// If stripping is on, there's no need for either of these
	// sections.  They're taking up ROM space and the loader
	// never reads 'em anyway.  Set the lengths to zero so they're not written
    write_align(_link_opts->argval(linkopt_secalign));
	Elf32_Shdr* sh = _sec_hdr + _sections->secnum(sec_symtab);
    RecordSectionOffset(sh, "symtab");
    if (!_link_opts->isset(linkopt_stripmore))
    {
		for (int i=0; i<_nsyms; i++)
			swap_sym(&_symtab[i]);
	    fwrite(_symtab, count, sh->sh_entsize, _link_fp);
	}
	RecordSectionSize(sh, "symtab");
} // ElfLinker::write_symtab()
    
//-------------------------------------------
void ElfLinker::write_relocations()
//-------------------------------------------
{
    Elf32_Shdr *sh;
    Elf32_Rela* r;
    unsigned char *buf, *rptr;
    SymSec sectype; 
//JRM There may be import relocations,    if (!_link_opts->isset(linkopt_relative))
//JRM which we write even without "-r"    	return;

    for (int i=0; i<_nrelocs; i++)
    {
		busy_cursor();
    	sectype = _relocs[i]->_sectype;
    	sh = _sec_hdr+_sections->secnum(sectype);
		DBG_ASSERT(RELA(sh));
		buf = _relocs[i]->_buf;
		for (rptr=buf; rptr<buf+sh->sh_size; rptr+=sh->sh_entsize)
		{
			r = (Elf32_Rela*)rptr;
			swap_reloc(r);
		}
    	//write section
		write_align(_link_opts->argval(linkopt_secalign));
	    RecordSectionOffset(sh, "reloc");
    	if (sh->sh_size)
    	{
	    	LinkerCompInfo* compInfo = NULL;
	    	if ((sh->sh_flags & SHF_COMPRESS) != 0)
	    	{
				compInfo = new LinkerCompInfo(_link_fp);
				if (!compInfo || !compInfo->fCompressor)
				{
					SET_ERR(se_malloc_err,("no more memory!\n"));
					return;
				}
	  		}
    		cwrite(compInfo, buf, sh->sh_size, 1, _link_fp);
			delete compInfo; // for C++ novices: this is ok, even if null.
    	}
		RecordSectionSize(sh, "reloc");
	}
} // ElfLinker::write_relocations()
    
//-------------------------------------------
void ElfLinker::write_hdr3do()
//-------------------------------------------
{
	DBG_ENT("write_hdr3do");
	int secnum = _sections->secnum(sec_hdr3do);
	if (_3dohdr && secnum)
	{
		write_section(secnum,(uint8*)_3dohdr);
    	FREE(_3dohdr);
    }
}

//-------------------------------------------
void ElfLinker::write_dynamic_sections()
//-------------------------------------------
{
	DBG_ENT("write_dynamic_sections");
	int secnum;
    if (_imports && (secnum = _sections->secnum(sec_imp3do),secnum))
    {
		write_section(secnum,(uint8*)_imports);
    	FREE(_imports);
    }
	if (_exports && (secnum = _sections->secnum(sec_exp3do),secnum))
	{
		write_section(secnum,(uint8*)_exports);
    	FREE(_exports);
    }
}

//-------------------------------------------
void ElfLinker::write_section(int secnum, uint8* secbuf)
//-------------------------------------------
{
	DBG_ENT("write_section");
	busy_cursor();
    Elf32_Shdr *sh;
    SymSec sectype = _sections->sectype(secnum);
	if (sectype==sec_none) return;	//nothing to do
    sh = _sec_hdr+secnum;
	DBG_(ELF,("section %s of size=x%X written at offset=x%X\n",\
		get_shname(sectype),sh->sh_size,ftell(_link_fp)));
	//skip no bits
	if (!sh->sh_size || get_shtype(sectype)==SHT_NOBITS)
    	return;
	DBG_ASSERT(_link_opts->argval(linkopt_secalign) >= sh->sh_addralign);
	DBG_ASSERT(secbuf);
	if (!secbuf)
	{
		SET_ERR(se_fail,("no buffer for section!\n"));
		return;
	}
	write_align(_link_opts->argval(linkopt_secalign));
    RecordSectionOffset(sh, "Section");
	fwrite(secbuf,sh->sh_size,1,_link_fp);
	RecordSectionSize(sh, "section");
} // ElfLinker::write_section(int,uint8*)
 
//-------------------------------------------
void ElfLinker::write_sections()
//go thru each of the objects and pull out the sections
//which need to be fixed up and written as part of this section
//-------------------------------------------
{
	DBG_ENT("write_sections");
    //was:for (int i=1; i<_nsecs; i++) {
	//order by section types
    for (SymSec sectype = (SymSec)1; sectype < SEC_NUM; sectype = (SymSec)(sectype+1))
    {
		busy_cursor();
    	int secnum = _sections->secnum(sectype);
		if (!secnum) continue;
    	Elf32_Shdr* sh = _sec_hdr + secnum;
    	//special sections will be handled separately
    	if (sectype==sec_shstrtab || sectype==sec_strtab 
    		|| sectype==sec_symtab)
    		continue;
    	//relocations written later
		if (REL(sh) || RELA(sh))
    		continue;
		//3do sections written separately
    	if (sectype==sec_hdr3do || sectype==sec_imp3do
    		|| sectype==sec_exp3do)
    		continue;
    	// ---- write section ---
    	// Check/set the offset.  Must do this even for
    	// zero-length sections such as NOBITS.
		write_align(_link_opts->argval(linkopt_secalign));
	    RecordSectionOffset(sh, "Section");
		//That's all for "nobits", but at least we have set the offset.
		if (!sh->sh_size || get_shtype(sectype)==SHT_NOBITS)
    		continue;
    	LinkerCompInfo* compInfo = NULL;
    	if ((sh->sh_flags & SHF_COMPRESS) != 0)
    	{
			compInfo = new LinkerCompInfo(_link_fp);
			if (!compInfo || !compInfo->fCompressor)
			{
				SET_ERR(se_malloc_err,("no more memory!\n"));
				return;
			}
  		}
    	// go thru each of the object files,
    	// writing out the relocated section contents of this type
		for (ObjInfo* o = firstobj(); o; o = o->next())
		{
			busy_cursor();
    		int osecnum = get_secnum(o,sectype);
			if (!osecnum) continue;
    		Elf32_Shdr *osh = o->_sec_hdr + osecnum;
    		//read section out of obj
    		if (!osh->sh_size || osh->sh_type==SHT_NOBITS)
    		{
				//kludge - shouldn't need this, but adjust_sections does an
				//alignment even when section has no size :-(.
    			cwrite_align(compInfo, sh->sh_addralign);
				DBG_(PICKY,("after align size=%d\n",ftell(_link_fp)-foff));
    			continue;
			}
    		unsigned char* secbuf = read_obj_section(o,osecnum);
    		if (!secbuf)
    		{
    			SET_ERR(se_fail,("#Unable to read section %d of obj %s!\n",\
    				osecnum,o->_fp->filename()));
    			continue;
    		}
			//fix up relocations if any
		    int relsecnum = get_relocs(o,osecnum);
    		if (relsecnum)
    		{ 
    				//&& (!_link_opts->isset(linkopt_relative))
			    unsigned char *relsecbuf = read_obj_section(o,relsecnum);
    			if (!relsecbuf)
    			{
    				SET_ERR(se_fail,("#Unable to load relocation section!\n"));
				}
				else
				{
					if (Failed(apply_relocations(o,relsecnum,secbuf,relsecbuf)))
					{
    					SET_ERR(se_fail,("#Unable to apply relocations!\n"));
					}
    				LINK_DELETE(relsecbuf);
				}
			}  		
			//write section
    		cwrite_align(compInfo, sh->sh_addralign);
			DBG_(PICKY,("after align size=%d\n",ftell(_link_fp)-foff));
			DBG_(ELF,("obj %d=%s section %s written at offset=x%X\n",\
				o->_objind,o->_fp->filename(),get_shname(sectype),ftell(_link_fp)));
    		cwrite(compInfo, secbuf, osh->sh_size, 1, _link_fp);
			DBG_(PICKY,("after write size=%d\n",ftell(_link_fp)-foff));
    		if (secbuf) LINK_DELETE_ARRAY(secbuf);
    	} // section loop
		if (sectype == sec_data)
		{
			//write space for common symbols and init to 0
			//uint32 size = _sections->size(sectype) - _coms->_tag;
			uint32 uncompressedBytesWritten
				= compInfo ?
					compInfo->fBytesOutput
					: ftell(_link_fp) - sh->sh_offset;
			int32 commonDataSize = sh->sh_size - uncompressedBytesWritten;	
			DBG_ASSERT(commonDataSize>=0);
			if (commonDataSize > 0)
			{	//more to write???
				//commonDataSize = ALIGN(commonDataSize,get_shalign(sec_data));
				//commonDataSize -= sh->sh_offset;	//what we wrote so far
				//commonDataSize = _sections->size(sectype) - commonDataSize;	//what we still have to write
    			//write_align(sec_data);
				DBG_(ELF,("writing x%X 0s for common data\n",commonDataSize));
				uint8* secbuf = (uint8*) NEW(uint8[commonDataSize]);
				if (!secbuf)
				{
					SET_ERR(se_malloc_err,("no more memory!\n"));
					return;
				}
				memset(secbuf, 0, commonDataSize);
    			cwrite(compInfo, secbuf, commonDataSize, 1, _link_fp);
				DELETE_ARRAY(secbuf);
			}
		}
		delete compInfo; // for C++ novices: this is ok, even if null.
		RecordSectionSize(sh, "section");
	} // type loop
} // ElfLinker::write_sections()

//-------------------------------------------
void ElfLinker::write_align(int alignment)
//-------------------------------------------
{
	busy_cursor();
		//for padding
    int pad = ALIGN(ftell(_link_fp), alignment) - ftell(_link_fp);	
    if (pad>0)
    {
    	//pad
		const char* nulls="\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
    	fwrite(nulls, pad, 1, _link_fp);
    }
} // ElfLinker::write_align(int)
    
//-------------------------------------------
void ElfLinker::cwrite_align(LinkerCompInfo* compInfo, int alignment)
//-------------------------------------------
{
	busy_cursor();
		//for padding
    int pad;

    if (compInfo)
        pad = ALIGN(compInfo->fBytesOutput, alignment) - compInfo->fBytesOutput;
    else
        pad = ALIGN(ftell(_link_fp), alignment) - ftell(_link_fp);

    if (pad>0)
    {
    	//pad
		const char* nulls="\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
    	cwrite(compInfo, nulls, pad, 1, _link_fp);
    }
} // ElfLinker::cwrite_align(LinkerCompinfo *, int)
    
//==========================================================================
// helpers

//-------------------------------------------
void ElfLinker::adjust_baseaddrs()
//adjust bss sections (and rodata too when we get one??)
//since relocations are for data/text; bss is in data and rodata is in text
//-------------------------------------------
{
	uint32 base;
	int secnum = 1;

	//order by section types
    for (secnum=1; secnum<_nsecs; secnum++) {
		SymSec sec = _sections->sectype(secnum);
		base = get_shbase(sec);

		//adjust sec 
		_sections->set_baseaddr(sec,secnum,base);
#ifdef ADJUST_BASEADDRS
		//adjust bss & rodata
		if (sec==sec_bss || sec==sec_rodata)
		{
			adjust_baseaddrs(sec,base);
		}
#endif
	}
} // ElfLinker::adjust_baseaddrs()

//-------------------------------------------
void ElfLinker::adjust_baseaddrs(SymSec, uint32 base)
//-------------------------------------------
{
	uint32 obase;
	int osecnum=0;
	//for each object, do the same
	for (ObjInfo* o = firstobj(); o; o=o->next())
	{
		//get bss section
    	for (int i=1; i<o->_sections->nsecs(); i++)
    	{	//calc size of sh_strtab
			if (osecnum = o->_sections->secnum(sec_bss),osecnum)
				break;
		}
		if (!osecnum) continue;
		//add bss base address to the base address of bss for each object
		obase = base + o->_sections->size(osecnum);
		o->_sections->set_baseaddr(sec_bss,osecnum,obase);
	}
}

//-------------------------------------------
uint32 ElfLinker::get_shbase(SymSec sec)
//for section base address (sh_addr) value
//bss and rodata are special - they must follow data and text respectively
//-------------------------------------------
{
   	int i = _sections->secnum(sec);	
	//if relative, start everything at 0 so as not to confuse anyone
	uint32 base;
	uint32 imagebase = get_optbase(sec_all); //base offset of first loadable section
#ifdef ADJUST_BASEADDRS
	//bss & rodata are special - they must follow data & text 
	//(done in adjust_baseaddrs)
	if (sec==sec_bss || sec==sec_rodata) {
		SymSec basesec = (sec==sec_bss)?sec_data:sec_code;
		int basesecnum = _sections->secnum(basesec);
		//base will be preceding section's base + size
		base = get_shbase(basesec);
		base += ALIGN(_sections->size(basesecnum),get_shalign(sec));
		return base;
		}
#endif
#ifdef RELBASE_0
//Had forced relative objects to start at 0 
//cuz when adding symtab values, didn't want to
//have to subtract base the base addr
	//if relative, start at 0 so can be absolute too
	if (_link_opts->isset(linkopt_relative)) {
		base = 0;
		}
	else {
#endif
		//relocate loadable sections if absolute (???)
		if (!(base = get_optbase(sec),base)) {
			base = _sections->baseaddr(i);
			if ((get_shflags(sec)&SHF_ALLOC)==SHF_ALLOC) {
				base += imagebase;
				imagebase += ALIGN(_sections->size(i),_link_opts->argval(linkopt_secalign));
				}
			}
#ifdef RELBASE_0
		}
#endif
	return base;
	}

//-------------------------------------------
uint32 ElfLinker::get_optbase(SymSec sectype)
//	return address if specified by user option
//-------------------------------------------
{
	DBG_ENT("get_optbase");
	uint32 imagebase=0;
	if (_link_opts->isset(linkopt_base))
	{
		for (int i=0; i<_link_opts->nargs(linkopt_base); i++)
		{
			char *type = _link_opts->arg(linkopt_base,i);
			if (type)
			{
				switch(*type)
				{
					case 't':	//text
						if (sectype==sec_code) 
							return _link_opts->asnval(linkopt_base,i);
						break;
					case 'd':	//data & bss
						if (sectype==sec_data) 
							return _link_opts->asnval(linkopt_base,i);
						if (sectype==sec_bss)
						{
							int basesec = _sections->secnum(sec_data);
							return get_shbase(sec_data) +
								ALIGN(_sections->size(basesec), get_shalign(sectype));
						}
						break;
					case 'i':	//image
						if (sectype==sec_all) 
							return imagebase = _link_opts->asnval(linkopt_base,i);
						break;
					default:
						fprintf(_user_fp,"Invalid arguments to base option\n");
						return 0;
				}
			}
		}
	}
	if (_link_opts->isset(linkopt_secbase))
	{
		for (int i=0; i<_link_opts->nargs(linkopt_secbase); i++)
		{
			char *n = _link_opts->arg(linkopt_secbase,i);
			if (n && !mystrcmp(n,get_shname(sectype)))
			{
				return _link_opts->asnval(linkopt_secbase,i);
			}
		}
	}
    return 0;
}
	
//-------------------------------------------
int ElfLinker::get_phnum()
//-------------------------------------------
{
	DBG_ENT("get_phnum");
    int nphs=0;
    //contain some 3do specific sections
	if (_link_opts->isset(linkopt_standard) 
		&& _link_opts->isset(linkopt_relative))
		return 0;
	if (!_link_opts->isset(linkopt_standard) 
    	&& _sections->secnum(sec_hdr3do)) nphs++;
    if (_sections->secnum(sec_code) || _sections->secnum(sec_rodata)) nphs++;
    if (_sections->secnum(sec_data) || _sections->secnum(sec_bss)) nphs++;
    if (_sections->secnum(sec_relatext) || _sections->secnum(sec_relarodata) || _sections->secnum(sec_reladata)) nphs++;
	if (!_link_opts->isset(linkopt_standard))
	{
		if (_sections->secnum(sec_imp3do)) nphs++;
		if (_sections->secnum(sec_exp3do)) nphs++;
	}
	return nphs;
}

//-------------------------------------------
void ElfLinker::get_prog_hdr(uint32 type,Elf32_Phdr* p,int n,...)
// This adds up all the sections passed in in the varargs list,
// and fills in the fields of p appropriately.
//-------------------------------------------
{
#define OLD_MEMSIZE_CALC 0
	// This flags the old way of calculating p->p_memsz, which
	// KevinH says is wrong (it was way too big).
// --- ^^^^^^     jrm 96/04/22
	Boolean doCompress = false;
    p->p_type = type;
	p->p_filesz = 0; // DO initialize this on each path.
	// p->p_flags=0;
	// p->p_memsz = 0;
		// NO! the struct is initally cleared with memset,
		// and we rely on this to detect first/second pass.
	va_list ap;
	va_start(ap, n);
	for (int i = 0; i < n; i++)
	{
		SymSec sec = (SymSec)va_arg(ap,int);
		Elf32_Shdr* sec_hdr = _sec_hdr + _sections->secnum(sec);
		if
		(
			_link_opts->isset(linkopt_compress)
			&&
			(
					sec==sec_data
				||	sec==sec_code || sec==sec_rodata
				||	sec==sec_relatext || sec==sec_relarodata
				||	sec==sec_reladata
			)
		)
		{
			doCompress = true;
			sec_hdr->sh_flags |= SHF_COMPRESS;
		}
    	if (i == 0) // set the offset from the first section
    		p->p_offset = sec_hdr->sh_offset;
    	if (!p->p_align)
    		p->p_align = sec_hdr->sh_addralign;
    	// jrm 96/04/05. sh_size for a NOBITS section does not
    	// actually contribute to the file size!  So I think this
    	// was a bug. I added this test:
    	if (sec_hdr->sh_type != SHT_NOBITS)
	    	p->p_filesz += sec_hdr->sh_size;
		if (type==PT_LOAD)
		{
    		if (!p->p_vaddr)
    			p->p_vaddr = _sections->baseaddr(sec);
    		//if (!p->p_paddr) p->p_paddr = 0;	//do we want to set this??
#if OLD_MEMSIZE_CALC
			if ((p->p_flags & PF_C) == 0) //only on first pass
				p->p_memsz += _sections->size(sec);
#endif
    	}
	} // for
	va_end(ap);
#if !OLD_MEMSIZE_CALC
	// new way.  Loadable sections, first pass.
	if (type == PT_LOAD && (p->p_flags & PF_C) == 0)
		p->p_memsz = p->p_filesz; // the INITIAL file size, before compression.
#endif

	// Ugly hack: on the first pass (before the compressed
	// sizes have been computed) the rela sections have
	// size zero.  This is the only way to tell the loader
	// the size in memory:
	if ((p->p_flags & PF_C) == 0 && !p->p_memsz)
		p->p_memsz = p->p_filesz; // the INITIAL file size
    if (doCompress)
        p->p_flags |= PF_C; // so it will be set on 2nd pass.
} //  ElfLinker::get_prog_hdr

//-------------------------------------------
char* ElfLinker::get_shname(SymSec sec)
//-------------------------------------------
{
	return get_secname(sec);
}
	
//-------------------------------------------
int ElfLinker::get_shalign(SymSec sec)
//for section header sh_addralign value
//-------------------------------------------
{
	switch (sec)
	{
		case sec_data: 
		case sec_bss: 
		case sec_rodata: 
			return 8;
		case sec_code: 
		case sec_init: 
		case sec_fini:
			if (_link_opts->isset(linkopt_standard))
				return 8;  //in order to be consistant with Diab Data...
			else
				return 4;
		case sec_symtab: 
		case sec_relatext:
		case sec_reladata:
		case sec_reladebug:
		case sec_relaline:
		case sec_relarodata: 
			return 4;
		case sec_line: 
		case sec_shstrtab:
		case sec_strtab:
		case sec_debug: 
			return 1;	//this could be 4, but Diab doesn't use UADDR relocs so must assume the sections is unaligned
		case sec_abs:
		case sec_com: 
		case sec_und: 
			SET_ERR(se_fail,("section header has section type abs/com/und!\n"));
			return 0;
		//3do additions
		//Provisional. What should these be?
		case sec_hdr3do: return 4;
		case sec_imp3do: return 4;
		case sec_exp3do: return 4;
		default:
			SET_ERR(se_unknown_type,("unknown sectype=x%X\n",sec));
			return 0;
	}
} // ElfLinker::get_shalign(SymSec sec)
	
//-------------------------------------------
int ElfLinker::get_shalign(Elf32_Shdr* sh, SymSec sec)
//-------------------------------------------
{
	switch (sec)
	{
		case sec_data: 
		case sec_bss: 
		case sec_code: 
		case sec_rodata: 
		case sec_init: 
		case sec_fini:
			return sh->sh_addralign;
		default:
			return get_shalign(sec);
	}
} // ElfLinker::get_shalign(Elf32_Shdr* sh, SymSec sec)
	
//-------------------------------------------
uint32 ElfLinker::get_shtype(SymSec sec)
//-------------------------------------------
{
	//others may want to add later
	//SHT_NOTE, SHT_REL, SHT_SHLIB, SHT_DYNSYM, SHT_HASH
	switch (sec)
	{
		case sec_code: 
		case sec_data: 
		case sec_debug: 
		case sec_line: 
			return SHT_PROGBITS;
		case sec_bss: 
			return SHT_NOBITS;
		case sec_init: 
		case sec_fini:
			return SHT_PROGBITS;
		case sec_symtab: 
			return SHT_SYMTAB;
		case sec_shstrtab:
		case sec_strtab:
			return SHT_STRTAB;
		case sec_relatext:
		case sec_reladata:
		case sec_reladebug:
		case sec_relaline:
			return SHT_RELA;
		case sec_abs:
		case sec_com: 
		case sec_und: 
			SET_ERR(se_fail,("section header has section type abs/com/und!\n"));
			return 0;
		//3do additions
		//Provisional. what should these be?
		case sec_hdr3do: return SHT_NOTE;
		case sec_rodata: return SHT_PROGBITS;
		case sec_relarodata: return SHT_RELA;
		case sec_imp3do: return SHT_NOTE;	//SHT_DYNSYM;
		case sec_exp3do: return SHT_NOTE;	//SHT_DYNSYM;
		default:
			SET_ERR(se_unknown_type,("unknown sectype=x%X\n",sec));
			return 0;
	}
}
	
//-------------------------------------------
uint32 ElfLinker::get_shflags(SymSec sec)
//-------------------------------------------
{
	switch (sec)
	{
		case sec_code: 
			return SHF_EXECINSTR | SHF_ALLOC;
		case sec_data: 
		case sec_bss: 
			return SHF_WRITE | SHF_ALLOC;
		case sec_debug: 
		case sec_line: 
		case sec_init: 
		case sec_fini:
		case sec_symtab: 
		case sec_shstrtab:
		case sec_strtab:
		case sec_relatext:
		case sec_reladata:
		case sec_reladebug:
		case sec_relaline:
			return 0;
		case sec_abs: 
		case sec_com: 
		case sec_und: 
			SET_ERR(se_fail,("section header has section type abs/com/und!\n"));
			return 0;
		//3do additions
		//Provisional. what should these be?
		case sec_hdr3do: return 0;
		case sec_rodata: return SHF_ALLOC;
		case sec_relarodata: return 0;
		case sec_imp3do: return 0;
		case sec_exp3do: return 0;
		default:
			SET_ERR(se_unknown_type,("unknown sectype=x%X\n",sec));
			return 0;
	}
}
	
//-------------------------------------------
uint32 ElfLinker::get_shentsize(SymSec sec)
//-------------------------------------------
{
	switch (sec)
	{
		case sec_code: 
		case sec_data: 
		case sec_bss: 
		case sec_debug: 
		case sec_line: 
		case sec_init: 
		case sec_fini:
		case sec_shstrtab:
		case sec_strtab:
			return 0;
		case sec_symtab: 
			return 0x10;
		case sec_relatext:
		case sec_reladata:
		case sec_reladebug:
		case sec_relaline:
			return 0xc;
		case sec_abs: 
		case sec_com: 
		case sec_und: 
			SET_ERR(se_fail,("section header has section type abs/com/und!\n"));
			return 0;
		//3do additions
		//Provisional. what should these be?
		case sec_hdr3do: return 0;
		case sec_rodata: return 0;
		case sec_relarodata: return 0;
		case sec_imp3do: return 0;
		case sec_exp3do: return 0;
		default:
			SET_ERR(se_unknown_type,("unknown sectype=x%X\n",sec));
			return 0;
	}
}
	
//-------------------------------------------
uint32 ElfLinker::get_shlink(SymSec sec)
//-------------------------------------------
{
	switch (sec)
	{
		case sec_code: 
		case sec_data: 
		case sec_bss: 
		case sec_debug: 
		case sec_line: 
		case sec_init: 
		case sec_fini:
		case sec_shstrtab:
		case sec_strtab:
			return 0;
		case sec_symtab: 
			return _sections->secnum(sec_strtab);
		case sec_relatext:
		case sec_reladata:
		case sec_reladebug:
		case sec_relaline:
			return _sections->secnum(sec_symtab);
		case sec_abs: 
		case sec_com: 
		case sec_und: 
			SET_ERR(se_fail,("section header has section type abs/com/und!\n"));
			return 0;
		//3do additions
		//Provisional. what should these be?
		case sec_hdr3do: return 0;
		case sec_rodata: return 0;
		case sec_relarodata: return _sections->secnum(sec_symtab);
		case sec_imp3do: return 0;
		case sec_exp3do: return 0;
		default:
			SET_ERR(se_unknown_type,("unknown sectype=x%X\n",sec));
			return 0;
	}
}
	
//-------------------------------------------
uint32 ElfLinker::get_shinfo(SymSec sec)
//-------------------------------------------
{
	switch (sec)
	{
		case sec_code: 
		case sec_data: 
		case sec_bss: 
		case sec_debug: 
		case sec_line: 
		case sec_init: 
		case sec_fini:
		case sec_shstrtab:
		case sec_strtab:
			return 0;
		case sec_symtab: 
			return _sections->secnum(sec_reladebug);
		case sec_relatext:
			return _sections->secnum(sec_code);
		case sec_reladata:
			return _sections->secnum(sec_data);
		case sec_reladebug:
			return _sections->secnum(sec_debug);
		case sec_relaline:
			return _sections->secnum(sec_line);
		case sec_abs: 
		case sec_com: 
		case sec_und: 
			SET_ERR(se_fail,("section header has section type abs/com/und!\n"));
			return 0;
		//3do additions
		//Provisional. what should these be?
		case sec_hdr3do: return 0;
		case sec_rodata: return 0;
		case sec_relarodata: return _sections->secnum(sec_rodata);
		case sec_imp3do: return 0;
		case sec_exp3do: return 0;
		default:
			SET_ERR(se_unknown_type,("unknown sectype=x%X\n",sec));
			return 0;
	}
}
	
//-------------------------------------------
uint32 ElfLinker::get_reltype(uint32 rtype)
//-------------------------------------------
{
	//don't generate new relocations for standard Elf
	if (_link_opts->isset(linkopt_standard))
		return 0;
	else
		return get_newreltype(rtype);
}

//-------------------------------------------
uint32 ElfLinker::get_newreltype(uint32 rtype)
//-------------------------------------------
{
    switch(rtype)
    {
        case R_PPC_ADDR32:
        	return R_PPC_BASEREL32;
        case R_PPC_ADDR24:
        	return R_PPC_BASEREL24;
        case R_PPC_ADDR16:
        	return R_PPC_BASEREL16;
        case R_PPC_ADDR16_LO:
        	return R_PPC_BASEREL16_LO;
        case R_PPC_ADDR16_HI:
        	return R_PPC_BASEREL16_HI;
        case R_PPC_ADDR16_HA:
        	return R_PPC_BASEREL16_HA;
        case R_PPC_ADDR14:
        	return R_PPC_BASEREL14;
        case R_PPC_ADDR14_BRTAKEN:
        	return R_PPC_BASEREL14_BRTAKEN;
        case R_PPC_ADDR14_BRNTAKEN:
        	return R_PPC_BASEREL14_BRNTAKEN;
        case R_PPC_UADDR32:	//datum to be relocated can be unaligned
        	return R_PPC_UBASEREL32;
        case R_PPC_UADDR16:	//datum to be relocated can be unaligned
        	return R_PPC_UBASEREL16;
        default:
			return 0;
    }
}

//-------------------------------------------
uint32 ElfLinker::get_newimpreltype(uint32 rtype)
//-------------------------------------------
{
    switch(rtype)
    {
        case R_PPC_ADDR32:
        	return R_PPC_IMPADDR32;	
        case R_PPC_ADDR24:
        	return R_PPC_IMPADDR24;
        case R_PPC_ADDR16:
        	return R_PPC_IMPADDR16;
        case R_PPC_ADDR16_LO:
        	return R_PPC_IMPADDR16_LO;
        case R_PPC_ADDR16_HI:
        	return R_PPC_IMPADDR16_HI;
        case R_PPC_ADDR16_HA:
        	return R_PPC_IMPADDR16_HA;
        case R_PPC_ADDR14:
        	return R_PPC_IMPADDR14;
        case R_PPC_ADDR14_BRTAKEN:
        	return R_PPC_IMPADDR14_BRTAKEN;
        case R_PPC_ADDR14_BRNTAKEN:
        	return R_PPC_IMPADDR14_BRNTAKEN;
        case R_PPC_UADDR32:	//datum to be relocated can be unaligned
        	return R_PPC_UIMPADDR32;
        case R_PPC_UADDR16:	//datum to be relocated can be unaligned
        	return R_PPC_UIMPADDR16;

		//relative relocs:
        case R_PPC_REL32:
        	return R_PPC_IMPREL32;	
        case R_PPC_REL24:
        	return R_PPC_IMPREL24;
        case R_PPC_REL14:
        	return R_PPC_IMPREL14;
        case R_PPC_REL14_BRTAKEN:
        	return R_PPC_IMPREL14_BRTAKEN;
        case R_PPC_REL14_BRNTAKEN:
        	return R_PPC_IMPREL14_BRNTAKEN;
        case R_PPC_RELATIVE:
        	return R_PPC_IMPRELATIVE;	

        default:
			return 0;
    }
}

//-------------------------------------------
SymSec ElfLinker::get_symsectype(ObjInfo* o,uint32 idx)
//-------------------------------------------
{
	uint32 secnum = o->_symtab[idx].st_shndx;
	switch (secnum)
	{
		case SHN_ABS: return sec_abs;
		case SHN_COMMON: return sec_com;
		case SHN_UNDEF: return sec_und;
	}
	if (idx>=get_symtab_nsyms(o))
	{
		DBG_ERR(("symtab idx=x%X of obj=%s is out of range; nsyms=x%X\n",
			idx,o->_fp->filename(),get_symtab_nsyms(o)));
		DBG_ASSERT(idx<get_symtab_nsyms(o));
		return sec_none;
	}
	return o->_sections->sectype(secnum);
}

//-------------------------------------------
int ElfLinker::get_relocs(ObjInfo* o,int s)
//if section s has relocations, return secnum of relocations to apply
//-------------------------------------------
{
    Elf32_Shdr *r;
	for (int i=1; i<o->_elf_hdr->e_shnum; i++) {
		r=o->_sec_hdr+i;
    	//in EABI, all relocations are rela!
		if (r->sh_info==s) {
			if (RELA(r) || REL(r))
				return i;
			}
		}
	return 0;
}
//alternatively could check name...
/*
int ElfLinker::get_relocs(Sections* sections,int i) {
    char name[30];
    sectype = sections->sectype(i);
    strcpy(name,".rel");	//and rela too...
    strcat(name,get_shname(sectype));
    relsectype = get_shtype(name);
    relsec = sections->secnum(relsectype);
    return relsec;
    }
*/

//-------------------------------------------
Boolean ElfLinker::isa_dll(ObjInfo* o)
//-------------------------------------------
{	
	int secnum = o->_sections->secnum(sec_exp3do);
	return (Boolean) 
		(secnum 
			&& (o->_sections->size(secnum)>0
				|| o->_elf_hdr && o->_elf_hdr->e_type==ET_DYN
				|| o->_sec_hdr && o->_sec_hdr[secnum].sh_size>0));
}

//-------------------------------------------
Boolean ElfLinker::isa_dll()
//-------------------------------------------
{	
	int secnum = _sections->secnum(sec_exp3do);
	return (Boolean) 
		(secnum 
			&& (_exports
				|| _exps->_nsyms>0
				|| _sections->size(secnum)>0
				|| _elf_hdr && _elf_hdr->e_type==ET_DYN
				|| _sec_hdr && _sec_hdr[secnum].sh_size>0));
}

//-------------------------------------------
Boolean ElfLinker::isabs(uint32 rtype)
//-------------------------------------------
{	//all absolute relocations sould have a relative counterpart
	//absolute relocations will be those that could emit a base reloative relocation
	return (Boolean)(get_newreltype(rtype)!=0);
}

//-------------------------------------------
void ElfLinker::swap_elf_hdr(Elf32_Ehdr* elf_hdr)
//-------------------------------------------
{
	if (swap_needed())
	{
		#define N(x) elf_hdr->e_##x = swapfix(elf_hdr->e_##x)
			N(type); N(machine); N(version); N(entry);
			N(phoff); N(shoff); N(flags); N(ehsize);
			N(phentsize); N(phnum); N(shentsize); N(shnum);
			N(shstrndx);
		#undef N
	}
}

//-------------------------------------------
void ElfLinker::swap_prog_hdr(Elf32_Phdr* prog_hdr)
//-------------------------------------------
{
	if (swap_needed())
	{
		#define N(f) prog_hdr->p_##f = swapfix(prog_hdr->p_##f)
			N(type); N(offset); N(vaddr); N(paddr);
			N(filesz); N(memsz); N(flags); N(align);
		#undef N
	}
}

//-------------------------------------------
void ElfLinker::swap_sec_hdr(Elf32_Shdr* sec_hdr)
//-------------------------------------------
{
	if (swap_needed())
	{
        #define N(x) sec_hdr->sh_##x = swapfix(sec_hdr->sh_##x)
            N(name); N(type); N(flags); N(addr); N(offset);
            N(size); N(link); N(info); N(addralign); N(entsize);
        #undef N
	}
}

//-------------------------------------------
void ElfLinker::swap_sym(Elf32_Sym* s)
//-------------------------------------------
{
	if (swap_needed())
	{
    	swapit(s->st_name);
    	swapit(s->st_value);
    	swapit(s->st_size);
    	//unsigned char st_info;  /* bind, type, st_info */
    	//unsigned char st_other;
    	swapit(s->st_shndx);   /* SHN_* */
	}	
}
	
//-------------------------------------------
void ElfLinker::swap_reloc(Elf32_Rela* r)
//-------------------------------------------
{
	if (swap_needed())
	{
		swapit(r->r_offset);
		swapit(r->r_info);
		swapit(r->r_addend);
	}
}
	
//-------------------------------------------
const char* ElfLinker::find_file(const char* name)
//-------------------------------------------
{
	DBG_ENT("find_file");
	//mmh we can guess our path separator, with test().
	const char* fname = 0;
	const char* filepath;
	if (exists(name))
		return name;
	for (int i=0; i<_link_opts->nargs(linkopt_path); i++)
	{
	    filepath = fmt_str("%s",_link_opts->arg(linkopt_path,i));
	    PathString our_path(filepath);
	    our_path.test();
		fname = fmt_str("%s%c%s",_link_opts->arg(linkopt_path,i),our_path.get_separator(),name);
		if (exists(fname)) return fname;
		fname = fmt_str("%s%s",_link_opts->arg(linkopt_path,i),name);
		if (exists(fname)) return fname;
	}
	return 0;
}
//-------------------------------------------
void ElfLinker::generate_mapfile(char* elf_fname,char* map_fname)
//-------------------------------------------
{
	DBG_ENT("generate_mapfile");
	FILE* map_fp=0;
	GetOpts* o = new GetOpts(0,0,dumpopts);
    o->set(dumpopt_header);
    o->set(dumpopt_program_headers);
    o->set(dumpopt_section_headers);
    o->set(dumpopt_symtab);
    o->set(dumpopt_relocations);
    o->set(dumpopt_dynamic_data);
    //o->set(dumpopt_hash);
    //o->set(dumpopt_debug_info);
    //o->set(dumpopt_lines);
    //o->set(dumpopt_content);	//for now...
	//Diab Data uses m2 to be a more detailed m; we'll treat it as an m for now...
#if defined(_MW_APP_) && defined (__TEST__)
    if ((map_fp=fopen("link.mapfile", "w+"))==0)
	{
		fprintf(_user_fp,"# Unable to create \"link.mapfile\"\n");
		return;
	}
#else
	if (!map_fname || !mystrcmp(map_fname,"2"))
	{
		map_fp=stdout;
	}
	else
	{
    	if ((map_fp=fopen(map_fname,"w+"))==0)
    	{
    		fprintf(_user_fp,"# Unable to create map file %s\n",map_fname);
    		return;
		}
	}
#endif
	ElfDumper map(this, elf_fname,map_fp,o);
	if (!map.Valid())
	{
		fprintf(_user_fp,"Fatal error - unable to proceed with map file\n");
		fprintf(_user_fp,"%s",map.GetErrStr());
	}
	else
	{
		map.DumpFile();	// dump specific stuff to file
	}
	if (map_fname && mystrcmp(map_fname,"2")) 
		fclose(map_fp);
	delete o;
}
		
