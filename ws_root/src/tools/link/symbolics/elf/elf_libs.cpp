/*  @(#) elf_libs.cpp 96/07/25 1.48 */

//====================================================================
// elf_libs.cpp  -  ElfLinker funcs for linking ELF libs

#ifndef USE_DUMP_FILE
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "utils.h"
#include "elf_l.h"
#endif /* USE_DUMP_FILE */

#include "linkopts.h"
#include "elf_3do.h"
#include "debug.h"

#pragma segment elflink
//==========================================================================
// Archive libraries

void ElfLinker::AddLib(const char* name) {
	DBG_ENT("AddLib");
	DBG_ASSERT(name)
    if (Validate()!=se_success) {
    	DBG_ERR(("invalid symbolics object!\n"));
    	return;
     	}
    const char* fname;
	//search for lib??.a first
	if (!(fname = find_file(fmt_str("lib%s.a",name)),fname)
		&& !(fname = find_file(fmt_str("%s.a",name)),fname)
    	&& !(fname = find_file(name),fname)) {
		DBG_WARN(("#Unable to locate library %s\n",name));
		fprintf(_user_fp,"#Unable to locate library %s\n",name);
		}
    else {
    	FileInfo* file = (FileInfo*) SYM_NEW(FileInfo(fname,F_LIB));
    	DBG_ASSERT(file);
    	file->_info.l = read_libinfo(file->_fp);
    	}
    }
    
//hash symtabs library l
void ElfLinker::hash_lib(LibInfo* l) {
    DBG_ASSERT(l);
	init_hash(l);	//init lib's hash table
	ArReader::Ar_syms ar_syms = l->_ar->_ar_syms;
	ArReader::Ar_sym* s = ar_syms._syms;
	for (int i=0; i<ar_syms._nsyms; i++) {
//DELETEME. use symtab offsets instead
#ifdef OLDLIB
		DBG_(ELF,("added sym %d (%s) in objind %d (offset x%X) lib %s\n",i,s[i]._name,s[i]._objind,s[i]._off,l->_fp->filename()));
		add_hash(l,s[i]._name,s[i]._objind);
#else
		DBG_(ELF,("added sym %d (%s) at offset x%X in lib %s\n",i,s[i]._name,s[i]._off,l->_fp->filename()));
		add_hash(l,s[i]._name,s[i]._off);
#endif
		}
	}
	
LibInfo* ElfLinker::read_libinfo(SymFile* fp) {
    DBG_ENT("read_libinfo");
    DBG_ASSERT(fp);
	LibInfo* l = (LibInfo*) SYM_NEW(LibInfo(fp));
    if (read_lib_hdr(l)!=se_success) {
    	_state.force_validate();	//validate for rest of link
    	LINK_DELETE(l);
    	l=0;
    	}
    return l;
	}
	
//read library header info
SymErr ElfLinker::read_lib_hdr(LibInfo* lib) {
    DBG_ENT("read_lib_hdr");
    DBG_ASSERT(lib);
	int i=0;
	if (_link_opts->isset(linkopt_verbose))
		fprintf(_user_fp,"Reading library %s\n",lib->_fp->filename());
	//read info from file
	ArReader* ar = (ArReader*) LINK_NEW(ArReader(lib->_fp,_state));
	lib->_ar = ar;
	if (state()!=se_success) {
    	DBG_ERR(("error %s occurred while reading file %s\n",_state.get_err_msg(),lib->_fp->filename()));
		if (!_state.valid()) {
			fprintf(_user_fp,"# Error: unable to read library %s.\n",lib->_fp->filename());
			fprintf(_user_fp,"%s\n",_state.get_err_msg());
			return state();
			}
		switch (state()) {
			case se_no_symbols:
				fprintf(_user_fp,"Warning: library %s contains no external symbols.\n",lib->_fp->filename());
				return state();
			default:
				fprintf(_user_fp,"Warning: error occurred while reading library %s.\n",lib->_fp->filename());
				fprintf(_user_fp,"%s\n",_state.get_err_msg());
			}
    	_state.validate();	//validate for rest of link
		} 
//DELETEME. use symtab instead
#ifdef OLDLIB
	//create objects structure
	lib->_nobjs = ar->nobjs();	//with first member null
	DBG_ASSERT(!lib->_objs);
	lib->_objs = (LibInfo::Objs*) LINK_NEW(LibInfo::Objs[lib->_nobjs+1]);
	//read archive members info
	char* mname;
	uint32 moff, msize=0;;
	i=1;
	DBG_(ELF,("reading member info for lib %d %s\n",lib->_libind,lib->_fp->filename()));
	memset(&lib->_objs[0],0,sizeof(LibInfo::Objs));
	for (m=ar->first(); m; m=ar->next()) {
		mname = ar->get_name();	//if <16 is here, else in string table
		msize = ar->get_size();
		moff = ar->obj_offset();
		DBG_(ELF,("lib member %d %s of size x%X at offset x%X\n",i,mname,msize,moff));
		lib->_objs[i]._name = mname;
		lib->_objs[i]._size = msize;
		lib->_objs[i]._offset = moff;
		i++;
		}
#endif
    return state();
	}
	
//init lib
SymErr ElfLinker::init_lib(LibInfo* lib) {
    DBG_ENT("init_lib");
    DBG_ASSERT(lib);
	DBG_ASSERT(!lib->_objs);
#ifdef OLDLIB
	int j;
	lib->_objs = (LibInfo::Objs*) SYM_NEW(LibInfo::Objs[lib->_nobjs+1]);
	for (j=1; j<=lib->_nobjs; j++) {
		lib->_objs[j]._used=LibInfo::not_used;
		lib->_objs[j]._obj=0;
		lib->_objs[j]._offset=0;	//???
		}
#endif
    return state();
	}
    
//before linking in objs from a library, need to attempt to resolve undefineds
//so as to not pull in uneeded objs
SymErr ElfLinker::get_libobjs(LibInfo* lib) {
	DBG_ENT("get_libobjs");
    DBG_ASSERT(lib);
	int newobjs=0;
	newobjs = resolve_unds(lib);	//try to resolve any undefineds
	while (newobjs!=0) {
#ifdef OLDLIB
		add_libobjs(lib);	//add any new objects (and new undefineds)
#endif
		newobjs = resolve_unds(lib);	//try to resolve any undefineds
		}
    return state();
	}
	
//add library objects which are needed to resolve undefineds
int ElfLinker::resolve_unds(LibInfo* l) {
	DBG_ENT("resolve_unds");
	//what's lib???  MW doesn't complain about undefined identifier.
    //DBG_ASSERT(lib);
    DBG_ASSERT(l);
	Syms::Sym* usym;
	int offset;
#ifndef OLDLIB
	#define MAX_NEWOBJS 20		//don't add more than 20 objs at a time
	LList newobjs(MAX_NEWOBJS);
#endif
	if (_unds->_nsyms==0) return 0;
	for (usym=_unds->first(); usym; )
	{
		Syms::Sym* tusym=usym->next();	//save in case we delete usym
		DBG_(ELF,("resolveing: usym _name=%s, _symidx=%d, _objind=%d\n",
			usym->_name,usym->_symidx,usym->_objind));
		//here offset is really object member file offset
		char* mname; uint32 moff, msize;
		if (offset=get_objind(l, usym->_name), offset)
		{
			//if (!newobjs.find(s->_objind) 
			if (!newobjs.find(offset))
			{
				l->_ar->get_objinfo(offset,mname,moff,msize);
				//ar->get_objinfo(s->_objind,mname,moff,msize);
				//ar->get_obj(moff,msize);
				DBG_(ELF,("Adding object file %s from library %s\n",mname,l->_fp->filename()));
				if (_link_opts->isset(linkopt_verbose)) {
					fprintf(_user_fp,"Adding object file %s from library %s\n",mname,l->_fp->filename());
					}
				ObjInfo* o = read_objinfo(l->_fp,moff,msize);
				if (o)
				{	
					delete [] o->_name;
					o->_name = new char[1+strlen(mname)];
					strcpy(o->_name, mname);
    				add_obj(o);	//hash symbols & adjust sections
#if 1
					return 1;	//quick fix for Mac
#else
					//newobj[newobjs]=s->_objind;	//add this offset to list
					newobjs.add(offset); //add this offset to list
#endif
				}
			}
			//add_obj will remove resolved unds for us :-)
			//_unds->rmv(usym);
			}
		if (newobjs.full())
			return newobjs.num();		//enough for now... add more next time
		usym = tusym;
	}
#ifdef OLDLIB
    return newobjs;
#else
	return newobjs.num();		
#endif
	}
    		
void ElfLinker::prune_lib(LibInfo* l) {
	DBG_ENT("prune_lib");
    //delete what we don't need anymore
    DBG_ASSERT(l);
    SYM_DELETE(l->_hashtab);
    l->_hashtab=0;
    int nobjs=0;
#ifdef OLDLIB
	for (int i=1; i<=l->_nobjs; i++) {
		if (l->_objs[i]._used!=LibInfo::not_used) {
    		nobjs++;
    		}
    	}
    if (nobjs==0) {
    	SYM_DELETE(l);
    	}
#endif
	}
	
void ElfLinker::add_libobjs(LibInfo* l) {
	DBG_ENT("add_libobjs");
    DBG_ASSERT(l);
#ifdef OLDLIB
	for (int i=1; i<=l->_nobjs; i++) {
		if (l->_objs[i]._used==LibInfo::need_to_add) {
			DBG_(ELF,("Adding object file %d %s from library %s\n",i,l->_objs[i]._name,l->_fp->filename()));
			ObjInfo* o = read_objinfo(l->_fp,l->_objs[i]._offset,l->_libind);
    		l->_objs[i]._obj = o;
    		add_obj(o);	//hash symbols & adjust sections
			l->_objs[i]._used=LibInfo::used;
    		}
    	}
#endif
	}
	
//returns object member file offset within the library
int ElfLinker::get_objind(LibInfo* l,char* name) {
	//DBG_ENT("get_objind");
    DBG_ASSERT(l);
	_linkhash* h=0; 
    if (!name || *name=='\0') {
		DBG_WARN(("symbol has no name! - ignoring...\n"));
		return 0;
		}
	if (h=l->_hashtab->get_hash(name,hoff_global),h) {	//name already in table??
		return h->_objind;
		}
	return 0;
	}
//add symbol names and offsets from libraries symbol table
void ElfLinker::add_hash(LibInfo* l,char* name,int objoffset) {
	//DBG_ENT("add_hash");
    DBG_ASSERT(l);
	uint32 h=0; int i;
    if (!name || *name=='\0') {
		DBG_WARN(("symbol has no name! - ignoring...\n"));
		return;
		}
	//hash with objind if local to prevent hash collision
	if (i=get_objind(l,name),i) { 	//name already in table??
		if (i==objoffset) return;	//already added
		if (_link_opts->isset(linkopt_allow_dups)) {
			fprintf(_user_fp,"Warning: duplicate definition of symbol %s in library %s\n",name,l->_fp->filename());
			SET_INFO(se_dup_symbols,("duplicate definition of symbol %s in library %s\n",name,l->_fp->filename()));
			}
		else {
			fprintf(_user_fp,"# Error: duplicate definition of symbol %s in library %s\n",name,l->_fp->filename());
			SET_ERR(se_fail,("duplicate definition of symbol %s in library %s\n",name,l->_fp->filename()));
			}
		}
	else
		//only care about globals and which obj they're in...
		l->_hashtab->add_hash(name,hoff_global,0,objoffset);
	}
	
//hash library's symtab so can look up symbol references quickly
void ElfLinker::init_hash(LibInfo* lib) {
	DBG_ENT("init_hash");
    DBG_ASSERT(lib);
	int i=0, minsz;
	uint32 hsize = 0;
	minsz = lib->_nobjs;
	hsize = lib->_ar->_ar_syms._nsyms;
	//better if size is odd (even better if prime...)
	hsize = max(hsize/2,minsz);	//so as not to collide with local symbols since obj index is used in has function
	if (hsize/2 == (hsize+1)/2)	hsize++; //even???	make odd
	lib->_hashtab = (LinkHash*) SYM_NEW(LinkHash(hsize));
	}
	
