/*  @(#) elf_objs.cpp 96/07/25 1.37 */

//====================================================================
// elf_objs.cpp  -  ElfLinker funcs for linking ELF objs 

#ifndef USE_DUMP_FILE
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "utils.h"
#include "elf_l.h"
#include "elf_3do.h"
#endif /* USE_DUMP_FILE */

#include "debug.h"

#pragma segment elflink
//==========================================================================
// objects & elf files

void ElfLinker::AddObj(const char* name)
{
	DBG_ENT("AddObj");
    if (Validate()!=se_success)
    {
    	DBG_ERR(("invalid symbolics object!\n"));
    	return;
    }
    const char* fname = find_file(name);
    if (!fname)
    {
		DBG_ERR(("#Unable to locate object file %s!!\n",name));
		fprintf(_user_fp,"#Unable to locate object file %s\n",name);
	}
    else
    {
		FileInfo* file = (FileInfo*) SYM_NEW(FileInfo(fname,F_OBJ));
		file->_info.o = read_objinfo(file->_fp,0,0);
		char*& dst = file->_info.o->_name;
		const char* src = file->_fp->filename();
		dst = new char[1 + strlen(src)];
		strcpy(dst, src);
#if 0	//ndef OLD_DLL
		if (isa_dll(file->_info.o)) 
			file->_type = F_DLL;
#endif
	}
}
    
void ElfLinker::add_obj(ObjInfo* o) {
	DBG_ENT("add_obj");
	DBG_ASSERT(o && o->_fp);
	DBG_(ELF,("adding object %d from file %s, baseoff=x%X\n",o->_objind,o->_fp->filename(),o->_base));
#ifndef OLD_DLL
	//if DLL, we add Dll's exports to list of imports
	if (isa_dll(o)) {
		DllInfo* d = (DllInfo*)SYM_NEW(DllInfo(o->_fp,o));
		DBG_ASSERT(d);
		//get_dllimps(d);
		//add its export info to the list of _imps and delete the obj
		DBG_ASSERT(d->_o);
		add_imps(d->_o);    //add to imports list
		//resolve_imps();	//see if the addition of this obj has resolved any imps
		DBG_WARN(("deleting file %s from link\n",d->_o->_fp->filename()));
		LINK_DELETE(d->_o);
		d->_o=0;
		}
	//else, we add object to elf file
	else {
#endif
		//for each allocable section, figure out what next base 
		//will be and align according to section alignment
		for (int j=1; j<o->_elf_hdr->e_shnum; j++) {
			busy_cursor();
			adjust_section(o,j);	//adjust baseaddrs
			}
		hash_symtab(o);	//hash these obj's syms
#ifndef OLD_DLL
		}
#endif
#ifndef OLDLIB
		resolve_unds();	//see if the addition of this obj has resolved any unds
#endif
		}

//-------------------------------------------
int ElfLinker::resolve_unds()
//resolve undefineds
//-------------------------------------------
{
	DBG_ENT("resolve_unds");
#if 0
	char* n;
	ObjInfo* o;
#endif
	uint32 defsymidx; ObjInfo* defo;
	if (_unds->_nsyms==0) return 0;
	for (Syms::Sym* usym =_unds->first(); usym; )
	{
		Syms::Sym* tusym=usym->next();	//save in case we delete usym
		register const char* symbolName = usym->_name; // optimize
#if 0
		o = obj(usym->_objind);
		DBG_ASSERT(o);
		n = get_symname(o,usym->_symidx);
#endif
		//Dlls' objects are deleted after they're read so we don't 
		//need to concern ourselves about them here
		if (find_sym(defsymidx, defo, symbolName) 
				|| _coms->find(symbolName))
		{
			_unds->rmv(usym);
		}
#ifndef OLD_DLL
		else
		{
			Syms::Sym* imp = _imps->find(symbolName);
			if (imp)
			{
				resolve_imp(imp,usym);
				_unds->rmv(usym);
			}
		}
#endif
		usym = tusym;
	}
    return _unds->_nsyms;
}
    		
void ElfLinker::resolve_imp(Syms::Sym* imp,Syms::Sym* usym) {
	DllInfo* d = dll(imp->_objind);
	//make this the token symbol
	add_hash(usym->_name,usym->_symidx,usym->_objind);
	//??? resolved = resolve_sym(usym);
	resolve_sym(usym);
	d->_used = true;
	}
			
void ElfLinker::adjust_section(ObjInfo* o,int j) {
	DBG_ENT("adjust_section");
	DBG_ASSERT(o);
			uint32 base=0;
			uint32 size=0;
			int secnum;
			Elf32_Shdr *p = o->_sec_hdr+j;
			SymSec sectype = o->_sections->sectype(j);
			DBG_(ELF,("section %d(%s)  type=x%X  addr=0x%lx  off=0x%lx"
				"  tsize=0x%lx(%ld)  link=0x%lx  info=0x%lx  align=0x%lx  entsize=0x%lx\n",
            	j, (o->_sh_strtab+p->sh_name), p->sh_type,p->sh_addr,p->sh_offset,
            	p->sh_size, p->sh_size, p->sh_link, p->sh_info,p->sh_addralign, p->sh_entsize));

			//special sections sold separately
			if (sectype==sec_shstrtab || sectype==sec_strtab 
					|| sectype==sec_symtab) return;	//ignore
			if (REL(p) || RELA(p)) 
				return;	//ignore
			if (sectype==sec_debug || sectype==sec_line) {
				if (_link_opts->isset(linkopt_strip))
					return;	//ignore
				_link_opts->set(linkopt_debug);	//turn on debug opt - found a debug section!
				}
			//add this section to our overall sections
			if (!(secnum=_sections->secnum(sectype),secnum)) {	//do we have one of these already??
				secnum=add_master_sec(sectype,base);	//one more section to add...
				size = 0;
				o->_sections->set_baseaddr(sectype,j,0);	//make relative to 0
				}
			else {
				base = _sections->baseaddr(secnum);	//base for this section
				size = _sections->size(secnum);		//get last size
				size = ALIGN(size,get_shalign(sectype));	//and align 
				o->_sections->set_baseaddr(sectype,j,base+size);	//relocate to new base
				}
			DBG_ASSERT(base==ALIGN(base,_link_opts->argval(linkopt_secalign)));	//make sure aligned!
			if (p->sh_addralign > _link_opts->argval(linkopt_secalign)) {
				fprintf(_user_fp,"# Error: object %s has section alignment %d which exceeds that of the generated file\n",o->_fp->filename(),p->sh_addralign);
				SET_ERR(se_fail,("object %s has section alignment %d which exceeds that of the generated file %d\n",o->_fp->filename(),p->sh_addralign,_link_opts->argval(linkopt_secalign)));
				}
			size += p->sh_size;
			_sections->set_size(secnum,size);	//adjust total size for this section
			}

void ElfLinker::get_objinfo(ObjInfo* o) {
    	//stuff for each file from SymNet:
		o->_fp = _fp;
		o->_sections = _sections;
    	//stuff for each file from ElfReader:
    	o->_sh_strtab = _sh_strtab;
    	o->_strtab = _strtab; 
    	o->_elf_hdr = _elf_hdr;
    	o->_sec_hdr = _sec_hdr;
    	o->_symtab = _symtab; 
    	//reinit for next loop (so ISymbolics doesn't delete)
		_fp=0;
		_sections=0;
    	_sh_strtab=0;
    	_strtab=0; 
    	_elf_hdr=0;
    	_sec_hdr=0;
    	_symtab=0; 
    	}
//stuff for each file from SymNet:
static SymFile* save_fp=0;
static Sections* save_sections=0;
//stuff for each file from ElfReader:
static char *save_sh_strtab=0;
static char *save_strtab=0; 
static Elf32_Ehdr *save_elf_hdr=0;
static Elf32_Shdr *save_sec_hdr=0;
static Elf32_Sym *save_symtab=0; 
void ElfLinker::save_objinfo() {
    	//stuff for each file from SymNet:
		save_fp=_fp;
		save_sections = _sections;
    	//stuff for each file from ElfReader:
    	save_sh_strtab = _sh_strtab;
    	save_strtab = _strtab; 
    	save_elf_hdr = _elf_hdr;
    	save_sec_hdr = _sec_hdr;
    	save_symtab = _symtab; 
    	}
void ElfLinker::restore_objinfo() {
		_fp=save_fp;
		_sections=save_sections;
    	_sh_strtab=save_sh_strtab;
    	_strtab=save_strtab; 
    	_elf_hdr=save_elf_hdr;
    	_sec_hdr=save_sec_hdr;
    	_symtab=save_symtab; 
    	}

//-------------------------------------------
ObjInfo* ElfLinker::read_objinfo(SymFile* fp, uint32 off, int libid)
//-------------------------------------------
{
    DBG_ENT("read_objinfo");
	ObjInfo* o = (ObjInfo*) LINK_NEW(ObjInfo(fp,off,libid));
	Boolean delete_obj=false;
	busy_cursor();
    if (read_obj_hdr(o)!=se_success)
    {
    	DBG_WARN(("%s occurred while reading file %s\n",_state.get_err_msg(),fp->filename()));
    	if (Validate()!=se_success) {
    		fprintf(_user_fp,"# Error: %s occurred while reading file %s\n",_state.get_err_msg(),fp->filename());
			delete_obj=true;
			}
    	}
	//does the obj contain any unsupported sections?
	if (o->_sections->secnum(sec_sdata) || o->_sections->secnum(sec_sbss)) {	
		fprintf(_user_fp,"# Error: object in file %s contains unsupported section\n",fp->filename());
		delete_obj=true;
		}
	if (delete_obj) {
		SET_ERR(se_fail,("invalid object in file %s\n",fp->filename()));
		LINK_DELETE(o);
		return 0;
		}
#ifdef OLD_DLL
	//is this a DLL?
	//if so, add its export info to the list of _imps and delete the obj
	if (o->_sections->secnum(sec_exp3do)) {	
		add_imps(o);	//add to imports list
		DBG_WARN(("deleting file %s from link\n",fp->filename()));
    	_state.force_validate();	//validate for rest of link
    	LINK_DELETE(o);
		return 0;
		}
#endif
    return o;
    }
    
//-------------------------------------------
SymErr ElfLinker::read_obj_hdr(ObjInfo* o)
//-------------------------------------------
{
    DBG_ENT("read_obj_hdr");
	save_objinfo();
    _fp=o->_fp;
    if (o->_base) _fp->set_base(o->_base);
    ISymbolics();
	get_objinfo(o);
    restore_objinfo();
    return state();
    }
    
//-------------------------------------------
unsigned char* ElfLinker::read_obj_section(ObjInfo* o, int s)
//-------------------------------------------
{
	DBG_ENT("read_obj_section");
    Elf32_Shdr *sh;
    unsigned char* secbuf=0;
    uint32 secoff, secsize;
    o->_fp->open();
	o->_fp->set_base(o->_base); 	//set base file offset of this object
	sh = o->_sec_hdr+s;
    secsize = sh->sh_size;
    if (secsize==0)
    	return 0;
    secoff = sh->sh_offset;
	DBG_(ELF,("reading obj %d %s base=x%X section %d (%s)offset x%X size x%X\n",o->_objind,o->_fp->filename(),o->_base,s,get_shname(o->_sections->sectype(s)),secoff,secsize));
    if (!o->_fp->seek(secoff,SEEK_SET))
    {
       	DBG_ERR(("#Unable to seek to section %s of obj %s at offset x%X\n",get_shname(o->_sections->sectype(s)),o->_fp->filename(),secoff))
       	SET_ERR(se_seek_err,("#Unable to seek to section %s of obj %s at offset x%X\n",get_shname(o->_sections->sectype(s)),o->_fp->filename(),secoff))
	}
	else
	{
    	DBG_(ELF,("section size=x%X, alignment=x%X\n",secsize,sh->sh_addralign));
    	//if (secbuf = (uint8*) MALLOC(secsize),secbuf)
    	if (secbuf = (uint8*) LINK_NEW(uint8[secsize]), secbuf)
    	{
    		if (!o->_fp->read(secbuf, secsize))
    		{
       			SET_ERR(se_read_err,("#Unable to read section %s of file %s at offset x%X\n",get_shname(o->_sections->sectype(s)),o->_fp->filename(),secoff))
       			//FREE(secbuf);
       			LINK_DELETE_ARRAY(secbuf);
       			secbuf=0;
   			}
   		}
   	}
	return secbuf;
} // ElfLinker::read_obj_section
        		
_linkhash* ElfLinker::add_hash(ObjInfo* o,int symidx) {
	//DBG_ENT("add_hash");
	char* name;
	Elf32_Sym* sym = get_sym(o,symidx);
    name = get_symname(o,sym);
    if (!name || *name=='\0') {
		SET_INFO(se_not_found,("symbol has no name! - ignoring..."));
		return 0;
		}
	//hash with objind if local to prevent hash collision
	_linkhash* h = _hashtab->get_hash(name,scopeid(sym,o->_objind));	//name already in table??
	//we don't want to add undef/common symbols until we're done
	//adding all the objects -
	//we'll add them in add_sym when we merge_symtabs 
	if (sym->st_shndx==SHN_UNDEF) {
		//if h, symbol is defined elsewhere - ignore this reference
		if (!h) {	//symbol is undefined (may be defined in obj that hasn't been added yet)
			_unds->add(name,symidx,o->_objind);
			}
		}
	else if (sym->st_shndx==SHN_COMMON) {
		//if h, symbol is defined elsewhere - ignore this reference
		if (!h) {
			_coms->add(name,symidx,o->_objind);
			}
		}
	else if (h) {
		if (symidx==h->_symidx && o->_objind==h->_objind)
			return 0;	//symbol already added
		if (_link_opts->isset(linkopt_allow_dups)) {
			fprintf(_user_fp,"Warning: duplicate definition of symbol %s in obj %s\n",name,o->_fp->filename());
			SET_INFO(se_dup_symbols,("duplicate definition of symbol %d %s (hsymidx=%d) in obj %d %s base=x%X\n",symidx,name,h->_symidx,o->_objind,o->_fp->filename(),o->_base));
			}
		else {
			fprintf(_user_fp,"# Error: duplicate definition of symbol %s in obj %s\n",name,o->_fp->filename());
			SET_ERR(se_fail,("duplicate definition of symbol %d %s (hsymidx=%d) in obj %d %s base=x%X\n",symidx,name,h->_symidx,o->_objind,o->_fp->filename(),o->_base));
			}
		}
	else {
		DBG_(ELF,("adding symbol %d %s in obj %d %s\n",symidx,name,o->_objind,o->_fp->filename()));
		h = _hashtab->add_hash(name,scopeid(sym,o->_objind),symidx,o->_objind);
		}
	return h;
	}
_linkhash* ElfLinker::add_hash(const char* name,int symidx,int objind) {
	//add_hash uses pointers to name, so must make sure 
	//we use name from object's _symtab
	DBG_ASSERT(obj(objind));
	const char* n = get_symname(obj(objind),symidx);
	DBG_ASSERT(n && !mystrcmp(n,name));
	//add to hash table and remove from undefineds list
   	return add_hash(n,hoff_global,symidx,objind);
	}
void ElfLinker::hash_symtab(ObjInfo* o) {
	DBG_ENT("hash_symtab");
	DBG_ASSERT(_hashtab);
	if (!_hashtab) return;
	int j;	
	Elf32_Sym* symtab = o->_symtab;
	int nsyms = get_symtab_nsyms(o);
	for (j=1; j<nsyms; j++) {
#if 0
		swap_sym(&symtab[j]);
#endif
		//if (FILTER_OUT(&symtab[j]))
		//	continue;
		add_hash(o,j);
		}
	}
	
void ElfLinker::reloc_symtab(ObjInfo* o) {
	DBG_ENT("reloc_symtab");
	int j;	
	Elf32_Sym* symtab = o->_symtab;
	int nsyms = get_symtab_nsyms(o);
	DBG_(ELF,("objind=%d; obj=%s; nsyms=x%X\n",o->_objind,o->_fp->filename(),nsyms));
	for (j=1; j<nsyms; j++) {
		//if (FILTER_OUT(&symtab[j]))
		//	continue;
		//sec = get_symsectype(o,j);
		//secnum = o->_sections->secnum(sec);
		//symtab[j].st_value += o->_sections->baseaddr(secnum);
		switch (symtab[j].st_shndx) {
        	case SHN_ABS: 
        		DBG_WARN(("ABS section found for index %d\n",j));
        		break;
        	case SHN_COMMON:
        		DBG_WARN(("COMMON section found for index %d\n",j));
        		break;
        	case SHN_UNDEF: 
        		DBG_WARN(("UNDEF section found for index %d\n",j));
        		break;
        	default:
    			if (symtab[j].st_shndx < o->_elf_hdr->e_shnum) {
					DBG_ASSERT(symtab[j].st_shndx==o->_sections->secnum(get_symsectype(o,j)));
#ifndef RELBASE_0
//to account for elf files which have already been relocated (base addr may not be 0)
DBG(("symtab[%d].value=x%X\n",j,symtab[j].st_value));
					symtab[j].st_value -= o->_sec_hdr[symtab[j].st_shndx].sh_addr;
DBG(("-o->sh_addr: symtab[%d].value=x%X\n",j,symtab[j].st_value));
					SymSec sec = o->_sections->sectype(symtab[j].st_shndx);
					int secnum = _sections->secnum(sec);
					symtab[j].st_value += _sec_hdr[secnum].sh_addr;
DBG(("+sh_addr: symtab[%d].value=x%X\n",j,symtab[j].st_value));
#endif
					symtab[j].st_value += o->_sections->baseaddr(symtab[j].st_shndx);
DBG(("+o->baseaddr: symtab[%d].value=x%X\n",j,symtab[j].st_value));
					//if we're generating absolute code, we need to add the section base
					//if (!_link_opts->isset(linkopt_relative))
					//symtab[j].st_value += _sections->baseaddr(_sections->secnum(o->_sections->sectype(symtab[j].st_shndx)));
					}
				else
					SET_ERR(se_fail,("section symtab[%d].st_shndx=%d is out of range!!\n",j,symtab[j].st_shndx));
			}
		}
	}
	
