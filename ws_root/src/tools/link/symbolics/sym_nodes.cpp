/*  @(#) sym_nodes.cpp 96/07/25 1.28 */

//====================================================================
// sym_nodes.cpp  -  contains definitions for the nodes of the
//				  	 symbol network
//
// may want to inline these for distribution version  ???

#ifndef USE_DUMP_FILE
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "predefines.h"

#include "symapi.h"
#include "symnet.h"
#include "loaderty.h"
#include "utils.h"

//#include "DebugDataTypes.h"

#ifdef __3DO_DEBUGGER__	
#include "CExpr_Common.h"
#endif /* __3DO_DEBUGGER__ */
 #endif /* USE_DUMP_FILE */
#include "debug.h"

#ifdef __3DO_DEBUGGER__
#define kProgressString1 "\pDelete Forward References, Delete Nodes"
#define kProgressString2 "\pDeleting Nodes"
#elif defined(macintosh)
#define kProgressString1 "\p"
#define kProgressString2 "\p"
#else
#define kProgressString1 ""
#define kProgressString2 ""
#endif

#pragma segment symnodes

//====================================================================
// structs/classes for symbols and types in symbol network

//====================================================================
// types

SymType::SymType(SymType*& chain,const char* n,uint32 s,SymCat c,void* t) {
	if (n) {
		_name = (char*)MALLOC(strlen(n) + 1);
		DBG_ASSERT(_name);
		if (_name) strcpy(_name,n);
		}
	else _name = 0;
	_size = s;
	_type = t; //type is either 0 or a pointer to another type
	_cat = c;
	_chain=chain;
	chain=this;	 //add myself to the chain
	}
      
SymType::~SymType() {
	if (_name) FREE(_name);
	switch(_cat) {
	case tc_union:
	case tc_struct: {	//delete struct/union
		SymStructType* s = (SymStructType*) _type;
		DBG_ASSERT(s);
		SYMNODE_DELETE(s);
		}
		break;
	case tc_array: {	//delete array
		SymArrayType* s = (SymArrayType*) _type;
		DBG_ASSERT(s);
		SYMNODE_DELETE(s);
		}
		break;
	case tc_enum: {		//delete enum
		SymEnumType* s = (SymEnumType*) _type;
		DBG_ASSERT(s);
		SYMNODE_DELETE(s);
		}
		break;
	}
//	6/19/95 dkk - This used to recursively delete the SymType objects that were chained together.
//	Because of the potentially large number of objects of this type, this caused a stack overflow
//	problem.
//      if (_chain) SYMNODE_DELETE(_chain);
      }
      
void SymType::set_name(const char* n) { 
	if (_name) FREE(_name); 
	if (n) { 
		_name=(char*)MALLOC(strlen(n)+1); 
		if (_name) strcpy(_name,n); 
		}
	else _name=0;
	}

SymArrayType* SymType::add_array(SymType* t) {
	SymArrayType* a;
	DBG_(SYMNET,("add_array(elemtype=x%X)\n",t));
	if (_cat!=tc_array || _type!=0) {
		DBG_ERR(("type not an array or referenced type != 0\n"));
		return 0;
		}
	a = (SymArrayType*) SYMNODE_NEW(SymArrayType(t));
	if (a) a->set_type(this);
	_type = (void*)a;
	return a;
	}

SymStructType* SymType::add_struct(int nfields) {
	SymStructType* s;
	DBG_(SYMNET,("add_struct(nfields=x%X)\n",nfields));
	if (_cat!=tc_struct && _cat!=tc_union || _type!=0) {
		DBG_ERR(("type not an struct/union or referenced type != 0\n"));
		return 0;
		}
	s = (SymStructType*) SYMNODE_NEW(SymStructType(nfields));
	if (s) s->set_type(this);
	_type = (void*)s;
	return s;
	}
      
SymEnumType* SymType::add_enum(SymType* t,int nmembers) {
	SymEnumType* e;
	DBG_(SYMNET,("add_enum(nmembers=x%X,elemtype=x%X)\n",nmembers,t));
	if (_cat!=tc_enum || _type!=0) {
		DBG_ERR(("type not an enum or referenced type != 0\n"));
		return 0;
		}
	e = (SymEnumType*) SYMNODE_NEW(SymEnumType(t,nmembers));
	if (e) e->set_type(this);
	_type = (void*)e;
	return e;
	}
      
SymEnumType::SymEnumType(SymType* t,int nmembers) {
	_nmembers = 0; //for now it's 0 - we increase as members are added
	if (nmembers) {
		_memberlist = (SymMemberType*) SYMNODE_NEW(SymMemberType[nmembers]); //array of members
		DBG_ASSERT(_memberlist);
		}
	else
		_memberlist = 0; //we'll add later
	_membtype = t;
	_type = 0;
	}

void SymEnumType::set_nmembers(int nmembers) {
    DBG_ASSERT(nmembers);
	_memberlist = (SymMemberType*) SYMNODE_NEW(SymMemberType[nmembers]); //array of members
	DBG_ASSERT(_memberlist);
	}

SymEnumType::SymMemberType* SymEnumType::add_member(uint32 val,SymType* t) {
    DBG_ASSERT(_memberlist);
	DBG_(SYMNET,("add_member(val=x%X,type=x%X)\n",val,t));
    //type added for member - point back to this type
    //so can search for enum by name (arrays and structs require referencing so don't need separate types for them)
	DBG_ASSERT(t);
	DBG_ASSERT(_memberlist);
    _memberlist[_nmembers]._val = val;
    _memberlist[_nmembers]._mtype = t;	//so enum can reference member's type
    _memberlist[_nmembers]._etype = this;	//so member can reference enum's type
    t->set_type(&_memberlist[_nmembers-1]);	//so member can get to it's SymMemberType
    _nmembers++;
    return &_memberlist[_nmembers-1];
   	}

SymEnumType::~SymEnumType() { 
	if (_memberlist) SYMNODE_DELETE_ARRAY(_memberlist); 
	}
      
uint32 SymArrayType::size() { 
	DBG_ASSERT(_type && membtype());
	if (!_type->size() && membtype()) 
		_type->_size = membtype()->size()*(_upperbound-_lowerbound+1);
	return _type->size();
	}

uint32 SymArrayType::nmembers() {
	uint32 n;
	DBG_ASSERT(membtype());
   	if (!_upperbound && !_lowerbound && membtype() && membtype()->size()) {
		n = size()/membtype()->size();
		}
	else n = _upperbound - _lowerbound + 1;
	return n; 
	}

int32 SymArrayType::upperbound() { 
   	if (!_upperbound && !_lowerbound) {
   		//if the bounds weren't set, try calculating from size
		_upperbound = nmembers()-1;
   		}
   	return _upperbound;
   	}
   	
SymStructType::SymStructType(uint32 nfields) {
	_nfields = 0; //for now it's 0 - we increase as fields are added
	if (nfields) {
		_typelist = (SymFieldType*) SYMNODE_NEW(SymFieldType[nfields]); //array of fields
		DBG_ASSERT(_typelist);
		}
	else
		_typelist = 0; //we'll add them later with set_nfields...
	_type = 0;
	}

SymStructType::~SymStructType() { 
	if (_typelist) SYMNODE_DELETE_ARRAY(_typelist); 
	}

void SymStructType::set_nfields(uint32 nfields) {
	DBG_ASSERT(nfields);
	DBG_ASSERT(!_typelist);
	_typelist = (SymFieldType*) SYMNODE_NEW(SymFieldType[nfields]); //array of fields
	DBG_ASSERT(_typelist);
	}
      
SymStructType::SymFieldType* SymStructType::add_field(uint32 offset,SymType* type) {
    DBG_ASSERT(_typelist);
	DBG_(SYMNET,("add_field(offset=x%X,type=x%X)\n",offset,type));
    _typelist[_nfields]._offset = offset;
    _typelist[_nfields]._type = type;
    _nfields++;
    return &_typelist[_nfields-1];
   	}
      
//====================================================================
//add line to module
	//line is line number in module; from 1 to m->nlines
	//addr is offset from code_base

SymLineEntry::SymLineEntry(SymLineEntry*& chain, uint32 l, uint32 a) {
	_addr=a;
	_line=l;
	//chain will be where we insert ourselves
	if (!chain) {
		_chain=chain;
		chain=this;	 //add myself to the top of the chain
		}
	else {
		_chain = chain->_chain;
		chain->_chain = this;
		}
    } //add new line to lines chain
SymLineEntry::~SymLineEntry()
{ 
//	dkk - Removed recursive object deletion. Can cause stack overflow with large programs.
//	if (_chain) SYMNODE_DELETE(_chain); 
}	//delete this chain

//====================================================================
//structure for symbol node in network
SymEntry::SymEntry(SymEntry*& chain, const char* n, SymType* t, uint32 v, SymSec sec, SymScope scope, SymbolClass sc) {
	if (n) {
		_name = (char*) MALLOC(strlen(n)+1);
		if (_name) strcpy(_name,n);
		}
	else _name=0;
	_parent=0;
	_sec=0; 	//storage class & section type (code/data/stack)
	//set_stgclass(sc); 	//storage class & section type (code/data/stack)
	_type=t;
	_sec=((uint32)sc | (uint32)scope | (uint32)sec);
	//_sec=sc;
	_val = v;		//address already relative
	_chain=chain;
	chain=this;	 //add myself to the chain
	DBG_(SYMNET,("SymEntry: name=%s, val=x%X, sec=x%X; sectype=x%X, stgclass=x%X\n",_name?_name:"(null)",_val,_sec,sectype(),stgclass()));
	}

SymEntry::~SymEntry()
{
	if (_name) FREE(_name);

//	dkk - Removed recursive object deletion. Can cause stack overflow with large programs.
//	if (_chain) SYMNODE_DELETE(_chain);	//delete this chain
}

SymFuncEntry* SymEntry::func() { 
	if (sectype()==sec_code) 
		return (SymFuncEntry*)_parent; 
	else {
		DBG_ERR(("sym not a code symbol!\n"));
		return 0; 
		}	//may be either function or data link
	}

SymModLink* SymEntry::modlink() { 
	if ((sectype()==sec_data || sectype()==sec_bss) && scope()==scope_module) 
		return (SymModLink*)_parent;
	else {
		DBG_ERR(("sym not a data symbol or local to module!\n"));
		return 0; 
		}	//may be either function or data link
	}

SymRootLink* SymEntry::rootlink() { 
	//if ((sectype()==sec_data || sectype()==sec_bss) && scope()==scope_global) 
	if (scope()==scope_global) 
		return (SymRootLink*)_parent;
	else {
		DBG_ERR(("sym not a global data symbol!\n"));
		return 0; 
		}	//may be either function or data link
	}
	
//====================================================================
//link sym/func to module
SymModLink::SymModLink(SymModLink*& chain, SymEntry* s, SymModEntry* m) {
	_thing=(void*)s;
	_mod=m;  //defined in module modp
	DBG_ASSERT(s);
	DBG_ASSERT(m);
	s->set_parent((void*)this);	//but I could be in both globals and modules...
	_chain=chain;
	chain=this;	 //add myself to the chain
	DBG_(SYMNET,("SymModLink: adding sym->val=x%X,sym->name=%s to module=%s\n",
			s->val(),s->name()?s->name():"(null)",m->name()?m->name():"(null)"));
	}

SymModLink::SymModLink(SymModLink*& chain, SymFuncEntry* f, SymModEntry* m) {
	_thing=(void*)f;
	_mod=m;  //defined in module modp
	DBG_ASSERT(f);
	DBG_ASSERT(m);
	f->set_parent((void*)this);	//but I could be in both globals and modules...
	_chain=chain;
	chain=this;	 //add myself to the chain
	DBG_(SYMNET,("SymModLink: adding f->val=x%X,f->name=%s to module=%s\n",
			f->val(),f->name()?f->name():"(null)",m->name()?m->name():"(null)"));
	}

//	7/6/95 dkk - Changed recursive deletion of objects to delete in for loop
//	to avoid possible stack overflow.
//
SymModLink::~SymModLink()
{ 
//	dkk - Removed recursive object deletion. Can cause stack overflow with large programs.
//	if (_chain) SYMNODE_DELETE(_chain); 
}	//delete this chain
    
//====================================================================
//link sym to func
SymFuncLink::SymFuncLink(SymFuncLink*& chain, SymEntry* s, SymFuncEntry* f) {
	_sym=s;
	_func=f;
	DBG_ASSERT(s);
	DBG_ASSERT(f);
	_sym->set_parent((void*)this);
	_sym->set_scope(scope_local);
	_chain=chain;
	chain=this;	 //add myself to the chain
	}

SymFuncLink::~SymFuncLink() { 
	if (_chain) SYMNODE_DELETE(_chain); 
	}	//delete this chain
    
//====================================================================
//link sym/func to root (_mod defaults to 0)
SymRootLink::SymRootLink(SymRootLink*& chain, SymEntry* s, SymModEntry* m) {
	_thing=(void*)s;
	_mod=m;  //defaults to 0
	DBG_ASSERT(s);
	s->set_parent((void*)this);
	s->set_scope(scope_global);
	_chain=chain;
	chain=this;	 //add myself to the chain
	DBG_(SYMNET,("SymRootLink: s->val=x%X,s->name=%s\n",
			s->val(),s->name()?s->name():"(null)"));
	}

SymRootLink::SymRootLink(SymRootLink*& chain, SymFuncEntry* f, SymModEntry* m) {
	_thing=(void*)f;
	_mod=m;  //defaults to 0
	DBG_ASSERT(f);
	f->set_parent((void*)this);
	_chain=chain;
	chain=this;	 //add myself to the chain
	DBG_(SYMNET,("SymRootLink: f->val=x%X,f->name=%s\n",
			f->val(),f->name()?f->name():"(null)"));
	}

SymRootLink::~SymRootLink()
{ 
//	dkk - Removed recursive object deletion. Can cause stack overflow with large programs.
//	if (_chain) SYMNODE_DELETE(_chain);	
}	//delete this chain
    
    
//==========================================================================
// add func to root/mod
SymFuncEntry::SymFuncEntry(SymFuncEntry*& chain, SymEntry* s,SymModEntry* m, SymFuncEntry* f) { //might not be inside a function/module
	DBG_ASSERT(s);
	_mod=m;		//my parents
	_func=f;		
	_sym=s;		//pointer into my sym in symbol network with my name, type, etc
	_lines=0;	//pointer into module's lines
	_lsyms=0;
	_blocks=0;
	_size=0;
	_baddr=0;
	_eaddr=0;
	_bline=0;
	_eline=0;
	_parent=0;
	_chain=chain;
	chain=this;
	DBG_(SYMNET,("SymFuncEntry: sym->val=x%X,sym->name=%s\n",
			_sym->val(),_sym->name()?_sym->name():"(null)"));
	}
		
SymFuncEntry::~SymFuncEntry() { 
	if (_lsyms) SYMNODE_DELETE(_lsyms);
	if (_blocks) SYMNODE_DELETE(_blocks);
//	dkk - Removed recursive object deletion. Can cause stack overflow with large programs.
//	if (_chain) SYMNODE_DELETE(_chain);	//delete this chain
	}
		
SymFuncLink* SymFuncEntry::add_lsym(SymEntry* s) { 
	DBG_(SYMNET,("add_lsym(sym=x%X)\n",s));
	return (SymFuncLink*) SYMNODE_NEW(SymFuncLink(_lsyms, s, this));
	}
    	
SymFuncEntry* SymFuncEntry::add_block(SymEntry* s) { 
	DBG_ASSERT(s);
	DBG_(SYMNET,("add_block(sym=x%X)\n",s));
	s->set_scope(scope_local);
	return (SymFuncEntry*) SYMNODE_NEW(SymFuncEntry(_blocks, s, _mod, this));
	}
    
SymModLink* SymFuncEntry::modlink() { 
   	DBG_ASSERT(_sym);
	if (_sym->scope()==scope_module) 
		return (SymModLink*)_parent;
	else {
		DBG_ERR(("function not local to module!\n"));
		return 0; 
		}	//may be either function or data link
	}

SymRootLink* SymFuncEntry::rootlink() { 
   	DBG_ASSERT(_sym);
	if (_sym->scope()==scope_global) 
		return (SymRootLink*)_parent;
	else {
		DBG_ERR(("function not global!\n"));
		return 0; 
		}
	}
	
//==========================================================================
// SymRoot
//Heap* SymRoot::_heap = SymNet::_heap;
SymRoot::SymRoot() { 
		//_heap = SymNet::_heap;
		//DBG_ASSERT(_heap);
	_gsyms=0;		
	_gfuncs=0;		
	_types=0;
	_syms=0;	
	_mods=0;
	_funcs=0;
	fAllocations = 0;
	}
		
SymRoot::~SymRoot()
{
	uint32 deleteCount = 0;
	TProgressBar progressBar;

	progressBar.InitProgressDialog(kProgressString1, kProgressString2); 
//	dkk - Removed recursive object deletion. Can cause stack overflow with large programs.
	{
		SymType *temp = _types, *next;
		while (temp)
		{
			if ((deleteCount % 2048) == 0)
				progressBar.UpdateProgress(deleteCount, fAllocations);
			++deleteCount;
			next = temp->_chain;
			SYMNODE_DELETE(temp);
			temp = next;
		}
	}
	{
		SymRootLink *temp = _gsyms, *next;
		while (temp)
		{
			if ((deleteCount % 2048) == 0)
				progressBar.UpdateProgress(deleteCount, fAllocations);
			++deleteCount;
			next = temp->_chain;
			SYMNODE_DELETE(temp);
			temp = next;
		}
	}
	{
		SymRootLink *temp = _gfuncs, *next;
		while (temp)
		{
			if ((deleteCount % 2048) == 0)
				progressBar.UpdateProgress(deleteCount, fAllocations);
			++deleteCount;
			next = temp->_chain;
			SYMNODE_DELETE(temp);
			temp = next;
		}
	}
	{
		SymFuncEntry *temp = _funcs, *next;
		while (temp)
		{
			if ((deleteCount % 2048) == 0)
				progressBar.UpdateProgress(deleteCount, fAllocations);
			++deleteCount;
			next = temp->_chain;
			SYMNODE_DELETE(temp);
			temp = next;
		}
	}
	{
		SymModEntry *temp = _mods, *next;
		while (temp)
		{
			if ((deleteCount % 2048) == 0)
				progressBar.UpdateProgress(deleteCount, fAllocations);
			++deleteCount;
			next = temp->_chain;
			SYMNODE_DELETE(temp);
			temp = next;
		}
	}
	{
		SymEntry *temp = _syms, *next;
		while (temp)
		{
			if ((deleteCount % 2048) == 0)
				progressBar.UpdateProgress(deleteCount, fAllocations);
			++deleteCount;
			next = temp->_chain;
			SYMNODE_DELETE(temp);
			temp = next;
		}
	}
	progressBar.UpdateProgress(deleteCount, fAllocations);
	progressBar.EndProgressDialog();
}
		
SymModEntry* SymRoot::add_mod(const char* name, uint32 baddr, uint32 eaddr) {
	fAllocations++;
	DBG_(SYMNET,("add_mod(name=%s,baddr=x%X,eaddr=x%X)\n",name?name:"(null)",baddr,eaddr));
	return (SymModEntry*) SYMNODE_NEW(SymModEntry(_mods,name,baddr,eaddr));
	}
    	
SymFuncEntry* SymRoot::add_func(SymEntry* sym,SymModEntry* mod) {	//global funcs may or maynot be in a module
	fAllocations++;
	DBG_(SYMNET,("add_func(sym=x%X,mod=x%X)\n",sym,mod));
	DBG_ASSERT(sym);
	return (SymFuncEntry*) SYMNODE_NEW(SymFuncEntry(_funcs,sym,mod));
	}
   		
SymEntry* SymRoot::add_sym(const char* name,SymType* type, uint32 val,SymSec sec,SymScope scope,SymbolClass sclass) {
	fAllocations++;
	DBG_(SYMNET,("add_sym(name=%s,type=x%X,val=x%X,sec=x%X,scope=x%X,sclass=x%X)\n",name?name:"(null)",type,val,sec,scope,sclass));
	return (SymEntry*) SYMNODE_NEW(SymEntry(_syms,name,type,val,sec,scope,sclass));
	}
    	
//for symbols that were previously added to the symbol network 
//when reading symbol table, may need to set the module later when
//reading debug section
SymRootLink* SymRoot::add_gsym(SymEntry* sym,SymModEntry* mod) {	//globals may or maynot be in a module
	DBG_(SYMNET,("add_gsym(sym=x%X,mod=x%X)\n",sym,mod));
	SymRootLink* rl;
	DBG_ASSERT(sym);
	sym->set_scope(scope_global);
	if (rl=sym->rootlink(),rl) {	//added already?
		if (!rl->mod()) rl->set_mod(mod);
		}
	else 
	{
		fAllocations++;
		rl = (SymRootLink*) SYMNODE_NEW(SymRootLink(_gsyms,sym,mod));
	}
	if (mod) {
		DBG_(SYMNET,("adding sym=%s to mod->name()=%s\n",
			sym->name()?sym->name():"(null)",mod->name()?mod->name():"(null)"));
		SymModLink* ml = mod->add_mgsym(sym);	//add link to sym in module as well
		DBG_ASSERT(ml);
		DBG_(SYMNET,("mod->mgsyms()=x%X,ml->sym()=x%X,ml->sym()->name()=%s\n",
			mod->mgsyms(),ml->sym(),ml->sym()->name()?ml->sym()->name():"(null)"));
		}
	return rl;
	}
   		
SymRootLink* SymRoot::add_gfunc(SymFuncEntry* f,SymModEntry* mod) {	//globals may or maynot be in a module
	DBG_(SYMNET,("add_gfunc(f=x%X,mod=x%X)\n",f,mod));
	SymRootLink* rl;
	DBG_ASSERT(f);
	DBG_ASSERT(f->sym());
	f->sym()->set_scope(scope_global);
	if (rl=f->rootlink(),rl) {	//added already?
		if (!rl->mod()) rl->set_mod(mod);
		}
	else 
	{
		fAllocations++;
		rl = (SymRootLink*) SYMNODE_NEW(SymRootLink(_gfuncs,f,mod));
	}

	if (mod) {
		DBG_(SYMNET,("adding func=%s to mod->name()=%s\n",
			f->name()?f->name():"(null)",mod->name()?mod->name():"(null)"));
		SymModLink* ml = mod->add_mgfunc(f);	//add link to function in module as well
		DBG_ASSERT(ml);
		DBG_(SYMNET,("mod->mgfuncs()=x%X,ml->func()=x%X,ml->func()->name()=%s\n",
			mod->mgfuncs(),ml->func(),ml->func()->name()?ml->func()->name():"(null)"));
		}
	return rl;
	}
   		
SymType* SymRoot::add_type(const char* name,uint32 size,SymCat cat,void* symt) {
	fAllocations++;
	DBG_(SYMNET,("add_type(name=%s,size=x%X,cat=x%X,symt=x%X)\n",name?name:"(null)",size,cat,symt));
	return (SymType*) SYMNODE_NEW(SymType(_types,name,size,cat,symt));
	}
   		
//==========================================================================
// add module to root
SymModEntry::SymModEntry(SymModEntry*& chain, const char* name,uint32 baddr,uint32 eaddr) {
	_cos=0; //delete CharOffs instance for converting chars<->lines
	if (name) {
		_name = (char*) MALLOC(strlen(name)+1);
		if (_name) strcpy(_name,name);
		}
	else _name=0;
	_path=0;
	_mgsyms=0;
	_mgfuncs=0;
	_msyms=0;
	_mfuncs=0;
	_lines=0;
	_lines_sorted_by_addr=0;
	_nlines=0;
	_baddr=baddr;
	_eaddr=eaddr;
	_cos = 0;  // for line<->charoff mapping
	_chain=chain;
	chain=this;
	DBG_(SYMNET,("SymModEntry: baddr=x%X,eaddr=x%X,name=%s\n",
			_baddr,_eaddr,_name?_name:"(null)"));
	}
		
//	7/6/95 dkk - Changed recursive deletion of SymModLink objects to delete in for loop
//	to avoid possible stack overflow.
//
SymModEntry::~SymModEntry() {
	SymModLink* thisSymModLink;
	SymModLink* nextSymModLink;
	
	if (_cos) SYMNODE_DELETE(_cos); //delete CharOffs instance for converting chars<->lines
	if (_name) FREE(_name);
	if (_path) FREE(_path);

	thisSymModLink = _msyms;
	while (thisSymModLink) {
		nextSymModLink = thisSymModLink->_chain;
		SYMNODE_DELETE(thisSymModLink);
		thisSymModLink = nextSymModLink;
		}

	thisSymModLink = _mfuncs;
	while (thisSymModLink) {
		nextSymModLink = thisSymModLink->_chain;
		SYMNODE_DELETE(thisSymModLink);
		thisSymModLink = nextSymModLink;
		}

	thisSymModLink = _mgsyms;
	while (thisSymModLink) {
		nextSymModLink = thisSymModLink->_chain;
		SYMNODE_DELETE(thisSymModLink);
		thisSymModLink = nextSymModLink;
		}

	thisSymModLink = _mgfuncs;
	while (thisSymModLink) {
		nextSymModLink = thisSymModLink->_chain;
		SYMNODE_DELETE(thisSymModLink);
		thisSymModLink = nextSymModLink;
		}

	SymLineEntry *thisSymLineEntry;
	SymLineEntry *nextSymLineEntry;

	thisSymLineEntry = _lines;
	while (thisSymLineEntry) {
		nextSymLineEntry = thisSymLineEntry->_chain;
		SYMNODE_DELETE(thisSymLineEntry);
		thisSymLineEntry = nextSymLineEntry;
		}

	thisSymLineEntry = _lines_sorted_by_addr;
	while (thisSymLineEntry) {
		nextSymLineEntry = thisSymLineEntry->_chain;
		SYMNODE_DELETE(thisSymLineEntry);
		thisSymLineEntry = nextSymLineEntry;
		}

//	dkk - Removed recursive object deletion. Can cause stack overflow with large programs.
//	if (_chain) SYMNODE_DELETE(_chain);	//delete this chain of modules
	}
		
void SymModEntry::set_name(const char* mname, char* mpath) {
	if (_name) FREE(_name);
	if (_path) FREE(_path);
	if (mpath) { 
		_path=(char*) MALLOC(strlen(mpath)+1); 
		if (_path) strcpy(_path,mpath); 
		}
	else _path=0;
	if (mname) { 
		_name=(char*) MALLOC(strlen(mname)+1); 
		if (_name) strcpy(_name,mname); 
		}
	else _name=0;
	}
    	
SymModLink* SymModEntry::add_msym(SymEntry* s) { 
	DBG_(SYMNET,("add_msym(s=x%X)\n",s));
	DBG_ASSERT(s);
	s->set_scope(scope_module);
	return (SymModLink*) SYMNODE_NEW(SymModLink(_msyms,s,this)); 
	}
    	
SymModLink* SymModEntry::add_mfunc(SymFuncEntry* f) { 
	DBG_(SYMNET,("add_mfunc(f=x%X)\n",f));
	DBG_ASSERT(f);
	DBG_ASSERT(f->sym());
	f->sym()->set_scope(scope_module);
	return (SymModLink*) SYMNODE_NEW(SymModLink(_mfuncs,f,this)); 
	}
    	
SymModLink* SymModEntry::add_mgsym(SymEntry* s) { 
	DBG_ASSERT(s);
	s->set_scope(scope_global);
	return (SymModLink*) SYMNODE_NEW(SymModLink(_mgsyms,s,this)); 
	}
    	
SymModLink* SymModEntry::add_mgfunc(SymFuncEntry* f) { 
	DBG_(SYMNET,("add_mgfunc(f=x%X)\n",f));
	DBG_ASSERT(f);
	DBG_ASSERT(f->sym());
	f->sym()->set_scope(scope_global);
	return (SymModLink*) SYMNODE_NEW(SymModLink(_mgfuncs,f,this)); 
	}
    	
SymLineEntry* SymModEntry::add_line(uint32 l, uint32 a) { 
	DBG_ASSERT((int32)l>=0 && (int32)a>=0);
	SymLineEntry* this_line;
	if (_cos==0)
		_cos = (CharOffs*) SYMNODE_NEW(CharOffs(_name,_path));
	if (!_cos) {
		DBG_ERR(("charoff: unable to create line<->ch mapping for file %s\n",_name?_name:"(null)"));
		return 0;
		}
	DBG_(SYMNET,("add_line(line=x%X,addr=x%lx)\n",l,a));
	_nlines = max(l,_nlines);

	SymLineEntry* last_line;
	SymLineEntry* line;
#if 0
	//add line sorted by addr
	//Diab Data sorts by addr
	//YUK.  messy code - clean it up later... SOMEDAY!!
	//was and should be: SYMNODE_NEW(SymLineEntry(_lines_sorted_by_addr,l,a)); 
	if (_lines_sorted_by_addr) {
		SymLineEntry* save_lines = _lines_sorted_by_addr;
		_lines_sorted_by_addr = 0;
		this_line = (SymLineEntry*) SYMNODE_NEW(SymLineEntry(_lines_sorted_by_addr,l,a));
		this_line->_chain = save_lines; 
		}
	else
		this_line = (SymLineEntry*) SYMNODE_NEW(SymLineEntry(_lines_sorted_by_addr,l,a)); 
#else
	//add line sorted by line
	//quick-n-dirty insertion sort for line numbers
	//Diab Data doesn't order by line number!!
	last_line = 0;
	for (line = _lines_sorted_by_addr; line && (a > line->addr()); line=line->next()) {
		//stop when we get to a line bigger than us; insert before that line
		last_line = line;
		}
	//if we're the smallest, add to the top of the chain
	if (!last_line)  {
		if (_lines_sorted_by_addr) {
			SymLineEntry* save_lines = _lines_sorted_by_addr;
			_lines_sorted_by_addr = 0;
			this_line = (SymLineEntry*) SYMNODE_NEW(SymLineEntry(_lines_sorted_by_addr,l,a));
			this_line->_chain = save_lines; 
			}
		else
			this_line = (SymLineEntry*) SYMNODE_NEW(SymLineEntry(_lines_sorted_by_addr,l,a)); 
		}
	else
		this_line = (SymLineEntry*) SYMNODE_NEW(SymLineEntry(last_line,l,a)); 
#endif

	//add line sorted by line
	//quick-n-dirty insertion sort for line numbers
	//Diab Data doesn't order by line number!!
	last_line = 0;
	for (line = _lines; line && (l > line->line()); line=line->next()) {
		//stop when we get to a line bigger than us; insert before that line
		last_line = line;
		}
	//if we're the smallest, add to the top of the chain
	if (!last_line)  {
		if (_lines) {
			SymLineEntry* save_lines = _lines;
			_lines = 0;
			this_line = (SymLineEntry*) SYMNODE_NEW(SymLineEntry(_lines,l,a));
			this_line->_chain = save_lines; 
			}
		else
			this_line = (SymLineEntry*) SYMNODE_NEW(SymLineEntry(_lines,l,a)); 
		}
	else
		this_line = (SymLineEntry*) SYMNODE_NEW(SymLineEntry(last_line,l,a)); 
	return this_line;
	}
    	
SymLineEntry* SymModEntry::add_charoff(uint32 c, uint32 a) { 
	DBG_ASSERT((int32)c>=0 && (int32)a>=0);
	if (_cos==0) _cos = (CharOffs*) SYMNODE_NEW(CharOffs(_name,_path));
	if (!_cos) {
		DBG_ERR(("charoff: unable to create line<->ch mapping for file %s\n",_name?_name:"(null)"));
		return 0;
		}
	uint32 l = _cos->ch2ln(c)+1;
	DBG_(SYMNET,("add_charoff(charoff=x%X,addr=x%lx); line=%d\n",c,a,l));
	_nlines = max(l,_nlines);
	return (SymLineEntry*) SYMNODE_NEW(SymLineEntry(_lines,l,a)); 
	} //add new line to lines chain
    	
// CHAROFFS_ERR means error
uint32 SymModEntry::charoff(uint32 ln) { 
	uint32 l2c;
	//try loading file now in case user has updated source directories
	if (_cos==0) _cos = (CharOffs*) SYMNODE_NEW(CharOffs(_name,_path));
	if (!_cos) {
		DBG_ERR(("charoff: unable to create line<->ch mapping for file %s\n",_name?_name:"(null)"));
		return CHAROFFS_ERR;
		}
	//char offsets start at 0; we start at 1
	if ((l2c=_cos->ln2ch(ln))==0) {
		DBG_ERR(("charoff: call to ln2ch(%d) for file %s\n",ln,_name?_name:"(null)"));
		return CHAROFFS_ERR;
		}
	return max(l2c-1,0);
	}
    	
// CHAROFFS_ERR means error
uint32 SymModEntry::linenum(uint32 ch) {
	uint32 c2l;
	//try loading file now in case user has updated source directories
	if (_cos==0) _cos = (CharOffs*) SYMNODE_NEW(CharOffs(_name,_path));
	if (!_cos) {
		DBG_ERR(("linenum: unable to create line<->ch mapping for file %s\n",_name?_name:"(null)"));
		return CHAROFFS_ERR;
		}
	//char offsets start at 0; we start at 1
	if ((c2l=_cos->ch2ln(ch+1))==0) {
		DBG_ERR(("linenum: call to ch2ln(%d) for file %s\n",ch+1,_name?_name:"(null)"));
		return CHAROFFS_ERR;
		}
	return _cos->ch2ln(ch+1);
	}

