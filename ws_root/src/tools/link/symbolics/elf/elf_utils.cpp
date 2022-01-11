/*  @(#) elf_utils.cpp 96/07/25 1.29 */

//====================================================================
// elf_utils.cpp  -  utility classes for ElfLinker 


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
// other classes

int ObjInfo::_nobjs=0;
ObjInfo* ObjInfo::_objs=0;
ObjInfo* ObjInfo::_lastobj=0;
int LibInfo::_nlibs=0;
LibInfo* LibInfo::_libs=0;
int FileInfo::_nfiles=0;
FileInfo* FileInfo::_files=0;
FileInfo* FileInfo::_lastfile=0;
int DllInfo::_ndlls=0;
DllInfo* DllInfo::_dlls=0;

//==========================================================================
// other classes

//-------------------------------------------
Syms::Sym::Sym(Sym*& treetop, const char* name, uint32 i, int o, int ord)
//-------------------------------------------
:	_symidx(i)
,	_objind(o)
,	_ordinal(ord)
,	_mom(NULL)
,	_ltree(NULL)
,	_rtree(NULL)
,	_name(NULL)
{
	if (name)
	{
		_name = (char*)MALLOC(strlen(name)+1);
		strcpy(_name, name);
	}
	if (!treetop)
	{
		treetop = this;
		return;
	}
	Sym *next = treetop, *current = NULL;
	while (next)
	{
		current = next;
		int compareResult = mystrcmp(name, current->_name);
		if (compareResult <= 0)
		{
			next = current->_ltree;
			if (!next)
				current->_ltree = this;
		}
		else
		{
			next = current->_rtree;
			if (!next)
				current->_rtree = this;
		}
	}
	_mom = current;
} // Syms::Sym::Sym

//-------------------------------------------
Syms::Sym::~Sym()
//-------------------------------------------
{
	if (_name)
		FREE(_name);
	
//	if (_mom)
//		_mom->_chain = _chain;
//		//if no _mom, we're top and Syms::rmv should handle
//	if (_chain)
//		_chain->_mom=_mom; 
} // Syms::Sym::~Sym()

//-------------------------------------------
Syms::Sym* Syms::Sym::next() const
//-------------------------------------------
{
	// If no right subtree, go back up the tree to the first
	// ancestor of which we are on the left
	if (!_rtree)
	{
		Sym* parent = _mom;
		const Sym* child = this;
		while (parent && parent->_rtree == child)
		{
			child = parent;
			parent = child->_mom;
		}
		// here parent is null, or parent->_ltree == child
		return parent;
	}
	// So there's a right subtree.  Head down there, then look left.
	Sym* result = _rtree;
	Sym* closer = result->_ltree;
	while (closer)
	{
		result = closer;
		closer = result->_ltree;
	}
	return result;
} // Syms::Sym::next

//-------------------------------------------
void Syms::Sym::unlink(Sym*& treetop) const
//-------------------------------------------
{
	// Unlink ourselves from the containing tree.
	// See Knuth Vol 3, 6.2.2
	
	Sym* myReplacement; 

	if (!_ltree || !_rtree)
	{
		// Simple case.  One of the subtrees is null.
		// Set myReplacement to the non-null child, if any.
		if (!_ltree)
			myReplacement = _rtree;
		else
			myReplacement = _ltree;
	}
	else
	{
		// Find next(), remove it from the tree, and reinsert it
		// here, where we currently are.
		myReplacement = this->next();
		Sym* dummy = NULL;
		myReplacement->unlink(dummy); // dummy null argument.
			// always has a null ltree, so simple case
		// Put myReplacement in the tree where this currently is
		myReplacement->_ltree = _ltree;
		if (_ltree) _ltree->_mom = myReplacement;
		myReplacement->_rtree = _rtree;
		if (_rtree) _rtree->_mom = myReplacement;
	}
	// myReplacement may be null
	if (myReplacement)
		myReplacement->_mom = _mom;
	if (_mom)
	{
		if (_mom->_ltree == this)
			_mom->_ltree = myReplacement;
		else
			_mom->_rtree = myReplacement;
	}
	else
		treetop = myReplacement;
} // Syms::Sym::unlink

//-------------------------------------------
void Syms::add(const char* name, uint32 i, int o, int ord)
//-------------------------------------------
{ 
	LINK_NEW(Sym(_syms, name, i, o, ord)); 
	_nsyms++;
}

//-------------------------------------------
Syms::Sym* Syms::find(const char* name) const
//-------------------------------------------
{
	Sym* current = _syms;
	while (current)
	{
		int compareResult = mystrcmp(name, current->_name);
		if (compareResult < 0)
			current = current->_ltree;
		else if (compareResult > 0)
			current = current->_rtree;
		else
			return current;
	}
	return NULL;
//	for (Sym* s->first(); s; s = s->next())
//	{
//		if (!mystrcmp(s->_name, name))
//			return s;
//	}
//	return NULL;
}

//-------------------------------------------
Syms::~Syms()
//-------------------------------------------
{ 
	// Keep removing first element.
	for (Syms::Sym *current = first(); current; current = first())
		rmv(current);
}

//-------------------------------------------
void Syms::rmv(Syms::Sym* s)
//-------------------------------------------
{ 
	if (s == _syms)
		s->unlink(_syms);
	else
	{
		Sym* dummytop = NULL;
		s->unlink(dummytop);	
	}
//	if (s == _syms)
//		_syms=_syms->next();
	LINK_DELETE(s); 
	_nsyms--;
}

//-------------------------------------------
Syms::Sym* Syms::first() const
//-------------------------------------------
{ 
	Sym* next = _syms, *result = NULL;
	while (next)
	{
		result = next;
		next = result->_ltree;
	}
	return result;
}

//-------------------------------------------
FileInfo::FileInfo(const char* name, int type)
//-------------------------------------------
{
	_fp = (SymFile*) LINK_NEW(SymFile(name));
	_type = type;
	_info.o = 0;
//add to tail of chain
	_chain = 0;
	if (_lastfile) _lastfile->_chain = this;
	_lastfile = this;
	if (!_files) _files=this;
	_nfiles++;
}

//-------------------------------------------
FileInfo::~FileInfo()
//-------------------------------------------
{ 
	//BEWARE: of SymNet trying to delete _fp!
	// SOMEDAY: make this foolproof.
	if (_fp) LINK_DELETE(_fp);
#if 0
	{
		SymFile* current = _fp;
		while (current)
		{
			SymFile* next = current->next();
			LINK_DELETE(current);
			current = next;
		}
	}
#endif
	//if (_chain) LINK_DELETE(_chain);
	// JRM moved into a loop in ElfLinker::~ElfLinker()
} // FileInfo::~FileInfo()

ObjInfo::ObjInfo(SymFile* fp, uint32 base, int l) {
    	_objind = _nobjs++;
    	_libind = l;	//do we need libind??
    	_fp = fp;
		_name = 0;
		_base = base;
		_sections = 0;
    	_sh_strtab = 0;
    	_strtab = 0;
    	_elf_hdr = 0;
    	_sec_hdr = 0;
    	_symtab = 0;
	//add to tail of chain
    	_chain = 0;
    	_mom = _lastobj;
    	if (_mom) _mom->_chain = this;	
    	if (!_objs) _objs = this;	//fix first obj
		_lastobj = this;	//fix last obj
    	}
ObjInfo::~ObjInfo() { 
    	_nobjs--;
		delete [] _name;// Note: we now keep a clone of this. jrm 96/06/07
		if (_sections) LINK_DELETE(_sections);
		//_strtab may be used for _sh_strtab
    	if (_sh_strtab && _sh_strtab!=_strtab) FREE(_sh_strtab);
    	if (_strtab) FREE(_strtab);
    	if (_elf_hdr) FREE(_elf_hdr);
    	if (_sec_hdr) FREE(_sec_hdr);
    	if (_symtab) FREE(_symtab);
    	if (_chain)
    		_chain->_mom = _mom;
		else
			_lastobj = _mom;		//fix last obj
		if (_mom)
			_mom->_chain = _chain;
		else
			_objs = _chain;		//fix first obj
		}

//-------------------------------------------
void ObjInfo::delete_objs()
//-------------------------------------------
{
//	if (_chain)
//	{
//		_chain->delete_objs();
//		LINK_DELETE(_chain);
//	}
	ObjInfo* current = _chain;
	while (current)
	{
		ObjInfo* next = current->_chain;
		LINK_DELETE(current);
		current = next;
	}
} 

ObjInfo* ObjInfo::find(int i) {
	ObjInfo* o;
	for (o=_objs; o; o=o->next()) {
		if (o->_objind==i) return o;
		}
	return 0;
	}
ObjInfo* ObjInfo::find(const char* n) {
	ObjInfo* o;
	const char* name = rmv_path(n);	//make sure path is stripped
	for (o=_objs; o; o=o->next()) {
		DBG_ASSERT(rmv_path(o->_name)==o->_name);
		if (o->_name && !mystrcmp(o->_name, name)) return o;
		}
	return 0;
	}

LibInfo::LibInfo(SymFile* fp) {
    	_libind = _nlibs++;
    	_fp = fp;
		_ar = 0;
		_objs = 0;
		_nobjs = 0;
		_hashtab = 0;
    	_mom = 0;
    	_chain = _libs;
    	if (_libs) _libs->_mom = this;
    	_libs = this;
    	}
LibInfo::~LibInfo() { 
	if (_ar) LINK_DELETE(_ar); 
	if (_hashtab) LINK_DELETE(_hashtab); 
	if (_objs) LINK_DELETE_ARRAY(_objs); 
    if (_chain) _chain->_mom = _mom;
	//else _lastlib = _mom;		//fix last lib - if decide to do this
	if (_mom) _mom->_chain = _chain;
	else _libs = _chain;
	}
void LibInfo::delete_libs() {
	if (_chain) {
		_chain->delete_libs();
		LINK_DELETE(_chain);
		}
	} 
LibInfo* LibInfo::find(int i) {
	LibInfo* l;
	for (l=_libs; l; l=l->next()) {
		if (l->_libind==i) return l;
		}
	return 0;
	}

#ifndef OLD_DLL
DllInfo::DllInfo(SymFile* fp, ObjInfo* o) {
#else
DllInfo::DllInfo(SymFile* fp) {
#endif
    	_dllind = _ndlls++;
    	_impind = 0;	//index into .imp3do section
    	_ord = 0;	//library's magic number
		_ver = 0;
		_rev = 0;
		_flags = 0;
    	_fp = fp;
#ifndef OLD_DLL
		_used = false;
		_o = o;
#endif
		_name = 0;
		const char* name=0;
		if (o) 
			name = o->_name;
		else if (fp && fp->filename()) 
			name = fp->filename();
		if (name)
		{
			name = rmv_path(name);
			_name = (char*)MALLOC(strlen(name) + 1);
			if (_name) strcpy(_name,name);
		}
    	_mom = 0;
    	_chain = _dlls;
    	if (_dlls) _dlls->_mom = this;
    	_dlls = this;
    	}

DllInfo::~DllInfo() { 
    if (_chain) _chain->_mom = _mom;
	//else _lastdll = _mom;		//fix last dll - if decide to use
	if (_mom) _mom->_chain = _chain;
	else _dlls = _chain;		//fix first dll
    _ndlls--;
	//if we have a name and an _fp, the name points to the _fp->filename() 
	//so don't delete it
	if (_name) FREE(_name);
	}

//-------------------------------------------
void DllInfo::delete_dlls()
//-------------------------------------------
{
//	if (_chain)
//	{
//		_chain->delete_dlls();
//		LINK_DELETE(_chain);
//	}
	DllInfo* current = _chain;
	while (current)
	{
		DllInfo* next = current->_chain;
		LINK_DELETE(current);
		current = next;
	}
}
 
//given this dll's ordinal number, find the Dll
DllInfo* DllInfo::find(int i) {
	DllInfo* d;
	for (d=_dlls; d; d=d->next()) {
		if (d->_ord==i) return d;
		}
	return 0;
	}
DllInfo* DllInfo::find(const char* n) {
	DllInfo* d;
	const char* name = rmv_path(n);	//make sure path is stripped
	for (d=_dlls; d; d=d->next()) {
		//some Dlls won't have a name, eg. if imported from def file
		DBG_ASSERT(!d->_name || rmv_path(d->_name)==d->_name);
		if (d->_name && !mystrcmp(d->_name,name)) return d;
		}
	return 0;
	}

DefParse::DefToken DefParse::get_token() {
    DefToken tkn=next_token();
    advance_buf(_next_token_idx);
    return tkn;
    }

DefParse::DefToken DefParse::next_token() {
	Parse::Token tkn;
	DefParse::DefToken dtkn;
	tkn = Parse::next_token();
	switch (tkn) {
		case tkn_symbol:
			if (!mystrcmp(_str,"MAGIC") || !mystrcmp(_str,"MODULE")) 
				dtkn = def_modnum;
			else if (!mystrcmp(_str,"IMPORTS")) 
				dtkn = def_imports;
			else if (!mystrcmp(_str,"IMPORT_NOW")) 
				dtkn = def_import_now;
			else if (!mystrcmp(_str,"IMPORT_ON_DEMAND")) 
				dtkn = def_import_on_demand;
			else if (!mystrcmp(_str,"REIMPORT_ALLOWED")) 
				dtkn = def_reimport_allowed;
			else if (!mystrcmp(_str,"IMPORT_FLAG")) 
				dtkn = def_import_flag;
			else if (!mystrcmp(_str,"EXPORTS")) 
				dtkn = def_exports;
			else dtkn = def_symbol;
			break;
		case tkn_number:
			dtkn = def_number;
			break;
		case tkn_equals:
			dtkn = def_equals;
			break;
		case tkn_period:
			dtkn = def_dot;
			break;
		case tkn_pound:
		case tkn_exclaimation:
			dtkn = def_comment;
			break;
		case tkn_fwdslash:
			dtkn = def_fwdslash;
			break;
		case tkn_colon:
			dtkn = def_colon;
			break;
		case tkn_eof:
			dtkn = def_eof;
			break;
		default:
			dtkn = def_unknown;
			break;
		}
	_token = dtkn;
	return dtkn;
	}
