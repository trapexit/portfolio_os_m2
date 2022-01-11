/*  @(#) sym_api.cpp 96/07/25 1.28 */

/*
	File:		sym_api.cpp

	Written by:	Dawn Perchik

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by The 3DO Company.

	Change History (most recent first):

		 <5>	96/02/14	JRM		Merge
		 <3>	96/01/17	JRM		Merge.

	To Do:
*/

//==========================================================================
// sym_api.cpp - contains definitions for methods in SymNet class which
// 				 are defined pure in Symbolics class and visable to the universe

#ifndef USE_DUMP_FILE
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "predefines.h"

#ifdef __3DO_DEBUGGER__
#include "Processor.h"
#include "ARMHardware.h"
#endif /* __3DO_DEBUGGER__ */

#include "symnet.h"
#include "utils.h"
#endif /* USE_DUMP_FILE */

#include "debug.h"

#pragma segment symapi
//the idea here is to keep one network for searching etc,
//and just have specialized classes for reading the symbols
//into the symbol network.
TProgressBar* SymNet::_progress_bar=0;

//==========================================================================
// initialization, termination & error routines
// 		returns last error or se_success if was se_successfully called

SymNet::SymNet() {
    DBG_ENT("SymNet");
    DBG_ASSERT(this);
	SET_STATE(se_invalid_obj);	//we'll validate symbolics class on call to ISymbolics
	DBG_(SYMNET,("creating heaps\n"));
	//_heap = (Heap*) NEW(Heap(&_state._s));
	//_tmp_heap = (Heap*) NEW(Heap(&_state._s));
	_sections = 0;
    //filename = 0;
    cur_scope.init_scope();
	DBG_(SYMNET,("creating _symroot\n"));
    _symroot = (SymRoot*) SYM_NEW(SymRoot);	if (!_symroot) return;
    _fp = 0;
    _dumping=0;
    //init "static" fundamental type pointers
	st_void=0;
	st_char=0;
	st_uchar=0;
	st_short=0;
	st_ushort=0;
	st_int=0;
	st_uint=0;
	st_long=0;
	st_ulong=0;
	st_float=0;
	st_double=0;
	st_ldouble=0;
	st_complex=0;
	st_dcomplex=0;
	st_string=0;
	st_function=0;
	st_pointer=0;
	st_ref=0;
	// NO! It's static _progress_bar=0;
    }

SymNet::~SymNet() {
    DBG_ENT("~SymNet");
	DBG_ASSERT(this);
    if (_symroot) SYM_DELETE(_symroot); _symroot=0; //delete all symbols and types
    if (_sections) SYM_DELETE(_sections); _sections=0;
    if (_fp) SYM_DELETE(_fp); _fp=0;
    //if (_heap) DELETE(_heap); _heap=0;
	//if (_tmp_heap) DELETE(_tmp_heap); _tmp_heap=0;
	//if (_progress_bar) DELETE(_progress_bar); _progress_bar=0;
    }

SymErr SymNet::Error() {
    DBG_ENT("Error");
	DBG_ASSERT(this);
    if (!this) {
       	DBG_(SYMAPI,("FATAL ERROR: symbolics object not created yet.\n"));
		return se_invalid_obj;
		}
    if (_symroot==0) {
		SET_ERR(se_invalid_obj,("invalid symbolics object!! _symroot==0.\n"));
		}
	else
    	if (!_state.valid()) {
       		DBG_(SYMAPI,("FATAL ERROR: %s occurred.\n",_state.get_err_name()));
       		}
    	else 
    		if (state()!=se_success) {
       			DBG_(SYMAPI,("WARNING: %s occurred.\n",_state.get_err_name()));
				}
    return state();
    }
    
Boolean SymNet::Valid() {
    if (!this) {
       	DBG_(SYMAPI,("FATAL ERROR: symbolics object not created yet.\n"));
		return false;
		}
    if (_symroot==0) {
		SET_ERR(se_invalid_obj,("invalid symbolics object!! _symroot==0.\n"));
		return false;
		}
	if (!_state.valid()) {
		DBG_ERR(("invalid symbolics object!! %s occurred.\n", _state.get_err_name()));
		return false;
		}
    return true;
    }

SymErr SymNet::Validate() {
	DBG_ASSERT(this);
    if (!this) {
       	DBG_(SYMAPI,("FATAL ERROR: symbolics object not created yet.\n"));
		return se_invalid_obj;
		}
    if (_symroot==0) {
		SET_ERR(se_invalid_obj,("invalid symbolics object!! _symroot==0.\n"));
		}
	if (_state.validate()!=se_success) {
		DBG_ERR(("# Unable to validate symbol network: %s occurred.\n", _state.get_err_name()));
		}
    return state();
    }

SymFileFormat SymNet::GetSymFF(char* fname) {
	DBG_ENT("GetSymFF");
	if (!fname) return eUNKNOWN;
	//for now we'll just hack it
	char* p;
	if (p=strrchr(fname,'.'),p) {
		if (!mystrcmp(p,".a")) return eAR;
		if (!mystrcmp(p,".elf")) return eELF;
		if (!mystrcmp(p,".sym")) return eARM;
		if (!mystrcmp(p,".xsym")) return eXSYM;
		if (!mystrcmp(p,".xcoff")) return eXCOFF;
		}
	return eELF;	//default
	}

//==========================================================================
// sections & addressing apis
// 		outside world only knows sections by type...

void SymNet::SetBaseAddress(SymSec sectype, unsigned long addr) {
	//what about multiple data/code/etc ?  current debugger doesn't support!
	DBG_ENT("SetBaseAddress");
	DBG_ASSERT(this);
	if (Validate()!=se_success) return;
	DBG_ASSERT(_sections);
	DBG_(SYMNET,("setting baseaddr=x%X; sectype=%X, _sections->secnum(sectype)=x%X\n",addr,sectype,_sections->secnum(sectype)));
	_sections->set_baseaddr(sectype,_sections->secnum(sectype),addr);
	if (sectype==sec_data && _sections->secnum(sec_bss)) {	//set bss also
		int data_secnum = _sections->secnum(sec_data);
		//guick hack until OS tells us the real story; may get into trouble with alignment
		uint32 bss_start = _sections->baseaddr(data_secnum) + _sections->size(data_secnum);
		DBG_(SYMNET,("setting baseaddr=x%X; sectype=sec_bss, _sections->secnum(sectype)=x%X\n",bss_start,_sections->secnum(sec_bss)));
		_sections->set_baseaddr(sec_bss,_sections->secnum(sec_bss),bss_start);
		}
	}

unsigned long SymNet::GetBaseAddress(SymSec sectype) { 
	//what about multiple data/code/etc ?  current debugger doesn't support!
	DBG_ENT("GetBaseAddress");
	DBG_ASSERT(this);
	if (Validate()!=se_success) return 0;
	DBG_ASSERT(_sections);
	return _sections->baseaddr(_sections->secnum(sectype));
	}
		
//given address, figure out what section it's in
//note, we don't know section sizes so this is a kludge -
//we find maximum section number which is below addr
//added to mimic GetSectionVarInfo in debugger
int SymNet::GetSection(unsigned long addr) { 
	DBG_ENT("GetSection");
	DBG_ASSERT(this);
	int secnum=0;
	if (Validate()!=se_success) return 0;
	DBG_ASSERT(_sections);
	uint32 sec_addr,min_addr=0;
	int min_sec=0,i;
	for (i=0; i<_sections->nsecs(); i++) {
		sec_addr = _sections->baseaddr(i);
		if (sec_addr < addr & sec_addr > min_addr) {
			min_addr = sec_addr; min_sec = i;
			}
		}
	secnum = _sections->sectype(min_sec);	//do I want to return sectype or number here?
	return secnum;
	}

//==========================================================================
// type apis

SymType* SymNet::GetRefType(SymType* t) { 
	DBG_ASSERT(this);
	if (((uint32)t->cat())&TC_REFSTMASK)
		return ((SymType*)t->type()); 
	else
		return 0;
	}

SymStructType* SymNet::GetStructType(SymType* t) { 
	DBG_ASSERT(this);
	if (t->cat()==tc_struct || t->cat()==tc_union)
		return ((SymStructType*) t->type()); 
	else
		return 0;
	}

SymArrayType* SymNet::GetArrayType(SymType* t) { 
	DBG_ASSERT(this);
	if (t->cat()==tc_array)
		return ((SymArrayType*) t->type()); 
	else
		return 0;
	}

SymEnumType* SymNet::GetEnumType(SymType* t) { 
	DBG_ASSERT(this);
	if (t->cat()==tc_enum)
		return ((SymEnumType*) t->type()); 
	else
		return 0;
	}
            
//SymErr SymNet::GetTypeInfo(char* typeName,SymType*& ttype) {
SymType* SymNet::GetType(char* typeName) {
	DBG_ENT("GetType");
	DBG_ASSERT(this);
	SymType* ttype=0;
	if (Validate()!=se_success) return 0;
	DBG_ASSERT(typeName);
	for (ttype=_symroot->types(); ttype; ttype=ttype->next()) {
		if (!mystrcmp(typeName,ttype->name()))
			return ttype;
		}
	SET_INFO(se_not_found,("type %s not found\n",typeName));
	return 0;
	}

int SymNet::FollowPtrs(SymType*& rt) { 
	DBG_ASSERT(this);
	int ptr_count;
	for (ptr_count = 0;
		rt && rt->cat()==tc_pointer; 
		ptr_count++, rt = (SymType*)rt->type())
			DBG_ASSERT(rt);
	return ptr_count;
	}
		
int SymNet::FollowRefs(SymType*& rt) { 
	DBG_ASSERT(this);
	int ref_count;
	for (ref_count = 0;
		rt && rt->cat()==tc_ref; 
		ref_count++, rt = (SymType*)rt->type())
			DBG_ASSERT(rt);
	return ref_count;
	}

SymType* SymNet::GetNthField(SymStructType* st,int field,uint32& offset) {
	offset=0; SymType* ftype=0;
	DBG_ENT("GetNthField");
	DBG_ASSERT(this);
	if (Validate()!=se_success) return 0;
	DBG_ASSERT(st);
	if (!st) { SET_ERR(se_parm_err,("GetNthField: null structure type!\n")); return 0; }
	DBG_ASSERT(field<st->nfields() && field>=0);
	if (field>=st->nfields() || field<0) { SET_ERR(se_bad_call,("GetNthFieldType: field %d out of range for struct!\n",field)); return 0; }
	offset = st->fieldoffset(field);
	ftype = st->fieldtype(field);
	DBG_ASSERT(ftype);
	if (!ftype) { SET_ERR(se_bad_call,("GetNthField: no refereced type for field %d!\n",field)); return 0; }
	return ftype;
	}
      

//==========================================================================
// symbol apis

SymScope SymNet::GetSymScope(SymEntry* sym,uint32& start,uint32& end) {
	DBG_ASSERT(sym);
	DBG_ASSERT(this);
	switch(sym->scope()) {
		case scope_global:
			start = 0;
			end = 0xFFFFFFFF;
			break;
		case scope_module:	//when funcs create syms, they set parent to the func
							//when modules link syms, they set parent to SymModLink...
			DBG_ASSERT(sym->parent());
			SymModEntry* m;
			switch(sym->stgclass()) {
				case sc_code:	//func
					m = ((SymFuncEntry*)sym->parent())->mod();	//parent point to func entry
					DBG_ASSERT(m);
					start = beg_addr(m);
					end = end_addr(m);
					break;
				case sc_data:	//data (as in static but local)
				case sc_bss:
					m = ((SymModLink*)sym->parent())->mod();
					DBG_ASSERT(m);
					start = beg_addr(m);
					end = end_addr(m);
					break;
				case sc_stack:	//lsym
				case sc_reg:	//reg
				default:
					DBG_ERR(("unexpected storage class x%X\n",sym->stgclass()));
					start = 0;
					end = 0;
					return scope_none;
				}
		case scope_local:	//block/lsym/reg..??
			DBG_ASSERT(sym->parent());
			SymFuncEntry* f;
			switch(sym->stgclass()) {
				case sc_code:	//block
					DBG_ASSERT((SymFuncEntry*)sym->parent());
					f = (SymFuncEntry*)sym->parent();
					start = beg_addr(f);
					end = end_addr(f);
					break;
				case sc_data:	//data (as in static but local)
				case sc_bss:
				case sc_stack:	//lsym
				case sc_reg:	//reg
					DBG_ASSERT(((SymFuncLink*)sym->parent())->func());
					f = ((SymFuncLink*)sym->parent())->func();
					start = beg_addr(f);
					end = end_addr(f);
					break;
				default:
					DBG_ERR(("unexpected storage class x%X\n",sym->stgclass()));
					start = 0;
					end = 0;
					return scope_none;
				}
			break;
		default:
			DBG_ERR(("unknown scope!\n"));
			start = 0;
			end = 0;
			return scope_none;
		}
	return(sym->scope());
	}
		
//lifetime of symbol
SymScope SymNet::GetSymLife(SymEntry* sym,uint32& start,uint32& end) {
	DBG_ASSERT(sym);
	DBG_ASSERT(this);
	switch(sym->scope()) {
		case scope_global:
		case scope_module:	//when funcs create syms, they set parent to the func
							//when modules link syms, they set parent to SymModLink...
			start = 0;
			end = 0xFFFFFFFF;
			break;
		case scope_local:	//block/lsym/reg..??
			DBG_ASSERT(sym->parent());
			SymFuncEntry* f;
			switch(sym->stgclass()) {
				case sc_code:	//block
					f = (SymFuncEntry*)sym->parent();
					start = beg_addr(f);
					end = end_addr(f);
					break;
				case sc_data:	//data (as in static but local)
				case sc_bss:
					start = 0;	//don't know who I belong too!
					end = 0xFFFFFFFF;
					break;
				case sc_stack:	//lsym
				case sc_reg:	//reg
					DBG_ASSERT(((SymFuncLink*)sym->parent())->func());
					f = ((SymFuncLink*)sym->parent())->func();
					start = beg_addr(f);
					end = end_addr(f);
					break;
				default:
					DBG_ERR(("unexpected storage class x%X\n",sym->stgclass()));
					start = 0;	//don't know who I belong too!
					end = 0xFFFFFFFF;	//but let them see it anyway
					return scope_none;
				}
			break;
		default:
			DBG_ERR(("unknown scope!\n"));
			start = 0;
			end = 0;
			return scope_none;
		}
	return(sym->scope());
	}
				
//==========================================================================
// functions for setting scope
//set module scope by name
SymErr SymNet::SetModScope(char* mname) {
	DBG_ENT("SetModScope");
	DBG_ASSERT(this);
    if (Validate()!=se_success) return state();
    DBG_ASSERT(mname);
    if (cur_mod() && cmp_mod_name(cur_mod(),(void*)mname))
       return state();	//nothing to do - already there
    SymModEntry* m;
    if (m=search_mods(mname),m) {
       cur_scope.set_mod(m);
       return state();
       }
    RETURN_ERR(se_not_found,("SetModScope: module %s not found\n", mname));
    }
    
//set func scope by name
SymErr SymNet::SetFuncScope(char* fname) {
    SymFuncEntry* f;
    DBG_ENT("SetFuncScope");
	DBG_ASSERT(this);
    if (Validate()!=se_success) return state();
    DBG_ASSERT(fname);
    //are we there already?
    if (cur_func() && cmp_func_name(cur_func(),(void*)fname)) {
       cur_scope.set_block(0); //init block to signify beginning of func
       return state();
       }
    //find the func
    if (f=search_allfuncs(fname),f) {
       cur_scope.set_func(f);
       cur_scope.set_mod(cur_func()->mod());
       cur_scope.set_block(0);
       return state();
       }
    RETURN_ERR(se_not_found,("SetFuncScope: function %s not found\n", fname));
    }
    
// set func & block scope by line number
// 		assumes must be in current module (if not, use SetModScope to change)
SymErr SymNet::SetLineScope(uint32 line) {
    SymLineEntry* l;
	DBG_ENT("SetLineScope");
	DBG_ASSERT(this);
    if (Validate()!=se_success) return state();
   	DBG_ASSERT(cur_mod() && (long)line>=0 && cur_mod()->nlines()>=line);
    if (!cur_mod())
        RETURN_ERR(se_bad_call,("SetLineScope: no current module in scope for line %d\n", line));
   	if (!((long)line>=0 && cur_mod()->nlines()>=line))
        RETURN_ERR(se_not_found,("SetLineScope: line %d out of range for module %s\n", line,(cur_mod()->name()) ? cur_mod()->name() : "(null)"));
    //when line in mod but not in any funcs, what should we do???
    if (l=search_lines(&SymNet::cmp_line_line,cur_mod(),(void*)&line),!l)
        RETURN_ERR(se_not_found,("SetLineScope: line %d not found in any function for module %s\n", line,cur_mod()->name()));
	uint32 addr = rel2abs(sec_code,l->addr());
	return SetAddrScope(addr);
    }
    
// set func & block scope by address
SymErr SymNet::SetAddrScope(uint32 addr) {
    SymFuncEntry* f;
    SymModEntry* m;
	DBG_ENT("SetAddrScope");
	DBG_ASSERT(this);
    if (Validate()!=se_success) return state();
    //in current block??
    if (cur_block() && inside(addr,cur_block()))
       return state();
    cur_scope.set_block(0);
    //in current func??
    if (cur_func() && inside(addr,cur_func())) {
       if (f=search_blocks(&SymNet::cmp_func_addrin,cur_func(),(void*)&addr),f) cur_scope.set_block(f);
       return state();
       }
    cur_scope.set_func(0);
    //in current mod??
    if (cur_mod() && inside(addr,cur_mod())) {
       if (f=search_inmodfuncs(&SymNet::cmp_func_addrin,cur_mod(),(void*)&addr),!f)
           RETURN_ERR(se_not_found,("SetAddrScope: addr x%04x not found in any function for module %s\n", addr,cur_mod()->name()));
       cur_scope.set_func(f);
       if (f=search_blocks(&SymNet::cmp_func_addrin,cur_func(),(void*)&addr),f) cur_scope.set_block(f);
       return state();
       }
    cur_scope.set_mod(0);
    //find out which block it's in & set scope there
    if (m=search_mods(&SymNet::cmp_mod_addrin,(void*)&addr),m) {
       cur_scope.set_mod(m);
       if (f=search_inmodfuncs(&SymNet::cmp_func_addrin,cur_mod(),(void*)&addr),!f) return se_not_found;
       cur_scope.set_func(f);
       if (f=search_blocks(&SymNet::cmp_func_addrin,cur_func(),(void*)&addr),f) cur_scope.set_block(f);
       return state();
       }
    //this code may not have an associated module name??
    if (f=search_allfuncs(&SymNet::cmp_func_addrin,(void*)&addr),f) {
       cur_scope.set_func(f);
       if (f=search_blocks(&SymNet::cmp_func_addrin,cur_func(),(void*)&addr),f) cur_scope.set_block(f);
       return state();
       }
    RETURN_ERR(se_not_found,("SetAddrScope: addr x%04x not found in any function/block for module %s\n", addr,cur_mod()->name()?cur_mod()->name():"(null)"));
    }

//==========================================================================
// old debugger symbol apis

#ifdef __3DO_DEBUGGER__	
#include "CExpr_Common.h"
#endif /* __3DO_DEBUGGER__ */

// used in symbol window along with SymbolCount
//- note that only globals are searched so we turn off scoping for now
SymEntry* SymNet::GetSymAtInd(unsigned long sym_idx) {
	DBG_ENT("GetSymAtInd");
	DBG_ASSERT(this);
	SymEntry* sym=0;
	if (Validate()!=se_success) return 0;
	sym_idx++; // symcounter starts counting at 1 
	DBG_(SYMAPI,("GetSymAtInd: parms sym_idx=x%X\n",sym_idx));
	uint32 codesyms=0;
	SymNetScope save_scope;
	save_scope=cur_scope;
	cur_scope.init_scope();	//temporarily turn off scoping
	cur_scope.set_mod(save_scope.mod());	//except for module
	if (sym=symcounter(sym_idx),sym) {
	DBG_(SYMAPI,("GetSymAtInd: returning name=%s, val=x%04x; rel2abs(sym)=x%X\n",sym->name()?sym->name():"(NULL)",sym->val(),rel2abs(sym)));
	}
	else
	DBG_WARN(("GetSymAtInd: NOTFOUND! returning 0\n"));
	cur_scope=save_scope;	//reset scope
	return sym;
	};  // input is really MTE Index.

//used in symbol window along with GetSymbolInfo
//- note that only globals are searched so we turn off scoping for now
unsigned long SymNet::SymbolCount() { //Mac returns mtre_last-mte_first+1 of RTE with type/num CODE/0001... huh??
	DBG_ENT("SymbolCount");		//does this mean I should return the count of symbols in the code segment for current object?
	DBG_ASSERT(this);
	if (Validate()!=se_success) return 0;
	SymNetScope save_scope;
	save_scope=cur_scope;
	cur_scope.init_scope();	//temporarily turn off scoping
	cur_scope.set_mod(save_scope.mod());	//except for module
	uint32 syms=0;
	symcounter(syms);	//passing 0 will cause symcounter to count all symbols
	cur_scope=save_scope;	//reset scope
	DBG_(SYMAPI,("SymbolCount: returning %d\n",syms));
	return syms;
	};

//==========================================================================
// generic symbol apis
// 		check globs, statics, mods, funcs, blocks, lsyms...

// AddressOf
//		register valiables will return their register number as an address
SymErr SymNet::AddressOf(StringPtr nm,unsigned long& addr)
{
    DBG_ENT("AddressOf");
	DBG_ASSERT(this);

	SymSec sec=sec_none; // what to do here?  Reggie's stuff returns code only
	DBG_ASSERT(nm);
	DBG_ASSERT(nm[0]);
	char* name = cnamestr(nm);

    DBG_ASSERT(name);
	addr=0;
    if (Validate()!=se_success) return state();
    SymEntry* sym;
    if (sym=search_scope(name),sym) {
       addr = rel2abs(sym);
       sec = sym->sectype();
       }
    else {
      addr = 0;
      sec = sec_none;
      SET_INFO(se_not_found,("AddressOf: symbol %s\n",name));
      }
    //SYM_DELETE(name);
	DBG_(SYMNET,("returning addr=x%04x\n",addr));
    return state();
    }

//==========================================================================
// Address2Source
SymErr SymNet::Address2Source(unsigned long addr, StringPtr *filename, uint32 *offset, uint32 *sline) 
{
    DBG_ENT("Address2Source");
	DBG_ASSERT(this);

	uint32 line=0;
	if (!offset) {
		DBG_ERR(("Address2Source: nul offset!\n"));
		return se_parm_err; 
		}
	if (!filename) {
		DBG_ERR(("Address2Source: nul filename!\n"));
		return se_parm_err; 
		}
	if (sline) *sline=0;
	*offset=0; 
	static const char* fnull="\4NULL";
	*filename = (StringPtr) fnull;

    if (Validate()!=se_success) return state();
    char *fn=0, *fname=0;
    SymModEntry* m=0;
    if (m=search_mods(&SymNet::cmp_mod_addrin,(void*)&addr),m) {
          SymLineEntry* l;
          if (l=search_lines(&SymNet::cmp_line_addr,m,(void*)&addr),l) {
                 if (rel2abs(sec_code,l->addr()) != addr)
                     SET_INFO(se_approx,("Address2Source: looking for x%X, found x%X\n",addr,rel2abs(sec_code,l->addr())));
                 line = l->line();
                 fn = m->name();
                 }
       }
    if (line==0) {	//filename & line/offset initialized already
        SET_INFO(se_not_found,("Address2Source: addr x%X\n",addr));
        }
    else {
		if (sline) *sline=line;
        *offset = m?m->charoff(line):0; //find char offset within file
        if (*offset==CHAROFFS_ERR) {
        	SET_ERR(se_no_line_info,("Address2Source: no line info available for filename %s\n",filename));
        	*offset=0; 
        	}
    	DBG_(SYMAPI,("Address2Source(x%X): returning fname=%s, choff=x%X, line=x%X\n",addr,fn,*offset,line));
        *filename = pnamestr(fn);  //return pointer
    	//if (state()==se_approx) SET_STATE(se_success); 	//assume approximate is OK
        DBG_(SYMS,("Address2Source: for addr=x%X, returning file=%s at line=%d/charoff=x%X\n",addr,fn,line,*offset));
		}
    return state(); //return whether we found an exact match or approximate
 }
 
//-------------------------------------------
SymErr SymNet::Source2Address(const StringPtr fnm, uint32 offset, uint32& addr) 
//-------------------------------------------
{
    DBG_ENT("Source2Address");
	DBG_ASSERT(this);

	uint32 line=0;
	addr=0;
	//char* filename = SYM_NEW(char[*((char*)fnm)+1]);
	//if (!filename) return 0;
	//memcpy(filename,fnm,*((char*)fnm)+1);
	//p2cstr((StringPtr)filename);
	DBG_ASSERT(fnm);
	DBG_ASSERT(fnm[0]);
	char* filename = cnamestr(fnm);

    if (Validate()!=se_success) 
    	return state();

    SymModEntry* m;
    DBG_ASSERT(filename);
    if (m=search_mods(&SymNet::cmp_mod_name,(void*)filename),!m)
    {
        SET_INFO(se_not_found,("Source2Address: filename %s not found\n",filename));
    }
    else
    {
        line = m->linenum(offset); // old debugger uses file offsets
        if (line==CHAROFFS_ERR)
        { 
        	SET_ERR(se_no_line_info,("Source2Address: unable to get line info for filename %s\n",filename));
        	line=0; 
        }
        else
        {
        	DBG_(SYMAPI,("line=%d; offset=x%04x\n",line,offset));
		    SymLineEntry* l = search_lines(&SymNet::cmp_line_line, m, (void*)&line);
	        if (!l)
	        {
	        	SET_INFO(se_not_found,("Source2Address: line %d not found in filename %s\n",line,filename));
			}
			else
			{
				if (l->line() != line)
				{
					SET_INFO(se_approx,("Source2Address: looking for %d, found %d\n",line,l->line()));
				}
				//else	//WARNING. this is a new change - the above used to return an approximate value
				addr = rel2abs(sec_code, l->addr());
			}
    	}
	} //close off else for call to linenum
        //SYM_DELETE(filename);
    DBG_(SYMAPI,("Source2Address(%s,x%X): returning addr=x%X\n",filename,offset,addr));
    return state();
}

//==========================================================================
//  source line apis

