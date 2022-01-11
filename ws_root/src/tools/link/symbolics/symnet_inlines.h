/*  @(#) symnet_inlines.h 96/07/25 1.17 */

#include "debug.h"

//these can't be in precompiled header
//sym_net inlines
//search scope for symbol
INLINE SymEntry* SymNet::search_scope(const char* name) { 
	DBG_ENT("search_scope(name)");
	DBG_(SYMNET,("calling search_scope: name=%s\n",name?name:"(null)"));
	return search_scope(&SymNet::cmp_name,(void*)name); 
	}
INLINE SymEntry* SymNet::search_scope(uint32 addr) { 
	DBG_ENT("search_scope(addr)");
	DBG_(SYMNET,("calling search_syms: addr=x%04x\n",addr));
	return search_scope(&SymNet::cmp_addr,(void*)&addr); 
	}
INLINE SymEntry* SymNet::search_data(uint32 addr) {
       return (search_data(&SymNet::cmp_addr,(void*)&addr));
       }
INLINE SymEntry* SymNet::search_data(const char* name) {
       return (search_data(&SymNet::cmp_name,(void*)name));
       }
INLINE SymEntry* SymNet::search_scope_data(const char* name) { //search global&local 
	return search_scope_data(&SymNet::cmp_name,(void*)name); 
	}
INLINE SymEntry* SymNet::search_scope_data(uint32 addr) { 
	return search_scope_data(&SymNet::cmp_addr,(void*)&addr); 
	}
	//search all funcs in scope for name/addr 
INLINE SymEntry* SymNet::search_scope_code(const char* name) { //search global&local
	return search_scope_code(&SymNet::cmp_func_name,(void*)name); 
	}
INLINE SymEntry* SymNet::search_scope_code(uint32 addr) { 
	return search_scope_code(&SymNet::cmp_func_addr,(void*)&addr); 
	}
	//search all funcs for name/addr
INLINE SymFuncEntry* SymNet::search_allfuncs(const char* name) { 
	return search_allfuncs(&SymNet::cmp_func_name,(void*)name); 
	}
INLINE SymFuncEntry* SymNet::search_allfuncs(uint32 addr) { 
	return search_allfuncs(&SymNet::cmp_func_addr,(void*)&addr); 
	}
INLINE SymFuncEntry* SymNet::search_inallfuncs(uint32 addr) { 
	return SymNet::search_allfuncs(&SymNet::cmp_func_addrin,(void*)&addr); 
	}
	//search all funcs in module for func containing addr 
INLINE SymFuncEntry* SymNet::search_inmodfuncs(uint32 addr,SymModEntry*m) { 
	return search_inmodfuncs(&SymNet::cmp_func_addrin,m,(void*)&addr); 
	}
	//search all modules for module containing addr 
INLINE SymModEntry* SymNet::search_inmods(uint32 addr) { 
	return search_mods(&SymNet::cmp_mod_addrin,(void*)&addr); 
	}
	//search all modules for module name 
INLINE SymModEntry* SymNet::search_mods(const char* name) { 
	return search_mods(&SymNet::cmp_mod_name,(void*)name); 
	}

//sym_api inlines
    INLINE SymFileFormat SymNet::SymFF() { DBG_ASSERT(this); return _symff; }
	INLINE unsigned long SymNet::ModDate()	const 	{ DBG_ASSERT(this); /*UNIMPLEMENTED.*/ return 0;/*(_dshb.dshb_mod_date);*/}
    INLINE int SymNet::GetNumFields(SymStructType* s) { DBG_ASSERT(this); return s->nfields(); }
	INLINE SymErr SymNet::GetArrayBounds(SymArrayType* at,uint32& upperbound,uint32& lowerbound) 
		{ DBG_ASSERT(this); upperbound = at->upperbound(); lowerbound = at->lowerbound(); return state(); } 
	//INLINE SymErr SymNet::GetStructInfo(SymStructType* st,int& nfields,uint32& size) 
	//	{ DBG_ASSERT(this); nfields = st->nfields(); size = st->size(); return state(); } 
	INLINE uint32 SymNet::GetSectionSize(SymSec sec) {
		return _sections->size(_sections->secnum(sec)); 
		}
	INLINE SymSec SymNet::GetSectionType(uint32 addr) {
		return (_sections->sectype(GetSection(addr)));
		}
	//type APIs
	INLINE Boolean SymNet::IsaBasicType(SymType* t) { DBG_ASSERT(this); return (Boolean)((t->cat() & TC_REFMASK)==0); }
	INLINE Boolean SymNet::IsaPointer(SymType* t) { DBG_ASSERT(this); return (Boolean)(t->cat() == TC_POINTER); }
	INLINE Boolean SymNet::IsaStructure(SymType* t) { DBG_ASSERT(this); return (Boolean)(t->cat() == TC_STRUCT); }
	INLINE Boolean SymNet::IsanArray(SymType* t) { DBG_ASSERT(this); return (Boolean)(t->cat() == TC_ARRAY); }
	INLINE Boolean SymNet::IsaEnum(SymType* t) { DBG_ASSERT(this); return (Boolean)(t->cat() == TC_ENUM); }
	INLINE Boolean SymNet::IsaFunction(SymType* t) { DBG_ASSERT(this); return (Boolean)(t->cat() == TC_FUNCTION); }
	INLINE SymCat SymNet::GetCat(SymType* t) { DBG_ASSERT(this); return t->cat(); }
	INLINE void SymNet::SetCat(SymType* t,SymCat c) { DBG_ASSERT(this); t->set_cat(c); }
	INLINE void SymNet::SetCat(SymType* t,SymType* t2) { DBG_ASSERT(this); t->set_cat((t2 ? t2->cat() : (SymCat)0)); }
	INLINE uint32 SymNet::GetSize(SymType* t) { DBG_ASSERT(this); return t->size(); }
	INLINE char* SymNet::GetName(SymType* t) { DBG_ASSERT(this); return t->name(); }
	INLINE SymType* SymNet::GetMembType(SymArrayType* t) { DBG_ASSERT(this); return t->membtype(); }
	INLINE SymType* SymNet::GetMembType(SymEnumType* t) { DBG_ASSERT(this); return t->membtype(); }
	//sym APIs
	INLINE char* SymNet::GetModName(uint32 addr) { 
		DBG_ASSERT(this);
		SymModEntry* m; 
		return (m=search_inmods(addr),m)?m->name():0; 
		}
	INLINE SymEntry* SymNet::GetFuncSym(uint32 addr) { 
		DBG_ASSERT(this); 
		SymFuncEntry* f; 
		return (f=search_inallfuncs(addr),f)?f->sym():0; 
		}
	INLINE SymEntry* SymNet::GetSym(char* name) { DBG_ASSERT(this); return search_scope(name); }
	INLINE SymEntry* SymNet::GetSym(uint32 addr) { DBG_ASSERT(this); return search_scope(addr); }
	INLINE SymEntry* SymNet::GetCodeSym(uint32 addr) { DBG_ASSERT(this); return search_code(addr); }
	INLINE SymEntry* SymNet::GetDataSym(uint32 addr) { DBG_ASSERT(this); return search_data(addr); }
	INLINE char* SymNet::GetSymName(SymEntry* s) { DBG_ASSERT(this); return s->name(); }
	INLINE SymType* SymNet::GetSymType(SymEntry* s) { DBG_ASSERT(this); return s->type(); }
	INLINE SymSec SymNet::GetSymSec(SymEntry* s) { DBG_ASSERT(this); return s->sectype(); }	//returns section type of symbol SEC_CODE, etc
	INLINE SymbolClass SymNet::GetSymClass(SymEntry* s) { DBG_ASSERT(this); return s->stgclass(); }	//returns storage class of symbol SC_CODE, etc
	INLINE uint32 SymNet::GetSymAddr(SymEntry* s) { DBG_ASSERT(this); return rel2abs(s); }
	INLINE SymErr SymNet::GetErr(char*& err, char*& fname, uint32& line) { 
		DBG_ASSERT(this);
		err = _state.get_err_name();
		return _state.state(fname,line); 
		}
	INLINE char* SymNet::GetErrStr() { 
		DBG_ASSERT(this);
		return _state.get_err_msg(); 
		}
		
//sym_net inlines
INLINE uint32 SymNet::beg_addr(SymFuncEntry* f) { return rel2abs(sec_code,f->baddr()); }
INLINE uint32 SymNet::end_addr(SymFuncEntry* f) { return rel2abs(sec_code,f->eaddr()); }
INLINE uint32 SymNet::beg_addr(SymModEntry* m) { return rel2abs(sec_code,m->baddr()); }
INLINE uint32 SymNet::end_addr(SymModEntry* m) { return rel2abs(sec_code,m->eaddr()); }
INLINE uint32 SymNet::addr(SymLineEntry* l) { return rel2abs(sec_code,l->addr()); }
INLINE SymFuncEntry* SymNet::cur_func() { return cur_scope.func(); }
INLINE SymFuncEntry* SymNet::cur_block() { return cur_scope.block(); }
INLINE SymModEntry* SymNet::cur_mod() { return cur_scope.mod(); }
INLINE void SymNet::set_cur_func(SymFuncEntry* func) { cur_scope.set_func(func); }
INLINE void SymNet::set_cur_block(SymFuncEntry* block) { cur_scope.set_block(block); }
INLINE void SymNet::set_cur_mod(SymModEntry* mod) { cur_scope.set_mod(mod); }
INLINE void SymNet::set_state(SymErr err, const char* file, uint32 line) 
		{ _state.set_state(err,file,line); }
INLINE SymErr SymNet::state() { return _state.state(); }
INLINE SymStructType* SymNet::add_union(const char*n,uint32 size,uint32 nfields) { 
    	SymType* t = add_type(n,size,tc_union,0); 
    	return t->add_struct(nfields); 
    	}
INLINE SymStructType* SymNet::add_struct(const char*n,uint32 size,uint32 nfields) { 
    	SymType* t = add_type(n,size,tc_struct,0); 
    	return t->add_struct(nfields); 
    	}
INLINE SymStructType::SymFieldType* SymNet::add_field(SymStructType* st, uint32 offset, SymType* ftype) { 
        return st->add_field(offset,ftype);
    	}
INLINE SymEnumType* SymNet::add_enum(const char*n,SymType* base,uint32 count) { 
    	SymType* t = add_type(n,base->size(),tc_enum,0); 
    	return t->add_enum(base,count); 
    	}
INLINE SymEnumType::SymMemberType* SymNet::add_member(SymEnumType* etype,const char*n,uint32 val) { 
		SymType* t = add_type(n,etype->size(),tc_member,0);	//add type for enum pointing back to this type
        return etype->add_member(val,t);
    	}
INLINE SymArrayType* SymNet::add_array(const char*n,uint32 size,SymType* base) {
    	SymType* t = add_type(n,size,tc_array,0); 
    	return t->add_array(base);
    	}
    	
INLINE SymEntry* SymNet::add_sym(const char* n, SymType* t, uint32 v, SymSec sec,SymScope sp,SymbolClass sc) { 
    	return _symroot->add_sym(n,t,abs2rel(sec,v),sec,sp,sc); //make address relative
    	}
INLINE SymFuncEntry* SymNet::add_func(SymEntry* s,SymModEntry* m) { 
	return _symroot->add_func(s,m); 
	}
INLINE SymModEntry* SymNet::add_mod(const char* n, uint32 ba, uint32 ea) { 
    	return _symroot->add_mod(n,ba?abs2rel(sec_code,ba):0,ea?abs2rel(sec_code,ea):0); 
    	}
INLINE SymFuncLink* SymNet::add_lsym(SymEntry* s, SymFuncEntry* f) { 
	return f->add_lsym(s); 
	}
INLINE SymRootLink* SymNet::add_gsym(SymEntry* s, SymModEntry* m) { 
	return _symroot->add_gsym(s,m); 
	}
INLINE SymRootLink* SymNet::add_gfunc(SymFuncEntry* f,SymModEntry* m) { 
	return _symroot->add_gfunc(f,m); 
	}
INLINE SymModLink* SymNet::add_msym(SymEntry* s, SymModEntry* m) { 
	return m->add_msym(s); 
	}
INLINE SymModLink* SymNet::add_mfunc(SymFuncEntry* f,SymModEntry* m) { 
	return m->add_mfunc(f); 
	}
INLINE SymFuncEntry* SymNet::add_block(SymEntry* s,SymFuncEntry* f) { 
	return f->add_block(s); 
	}
INLINE SymLineEntry* SymNet::add_line(SymModEntry* m, uint32 l, uint32 a) {
    	DBG_ASSERT(m);
    	DBG_(PICKY,("l=x%X, a=x%X, abs2rel(sec_code,a)=x%X, m->baddr=x%X, _sections->begaddr(sec_code)=x%X\n",
    		l,a,abs2rel(sec_code,a),m->baddr(),_sections->baseaddr(_sections->secnum(sec_code))));
    	return m->add_line(l,abs2rel(sec_code,a)); 
    	}
INLINE SymLineEntry* SymNet::add_charoff(SymModEntry* m, uint32 c, uint32 a) { 
	DBG_ASSERT(m); return m->add_charoff(c,abs2rel(sec_code,a)); 
    }
INLINE SymFuncEntry* SymNet::search_inmodfuncs(cmp_func_t cmp_func,SymModEntry*m,void*thing) { 
	SymFuncEntry* f;
	return ((f=search_mfuncs(cmp_func,m,thing),f)
			? f : search_mgfuncs(cmp_func,m,thing)); 
	}


//sym_utils inlines
INLINE SymRefType::SymRefType(Fwdref file_offset,SymType* st) { 
		_typeref=file_offset; 
		_symtype=st; 
		}
INLINE SymFwdrefType::SymFwdrefType(Fwdrefid id,Fwdref t) {
   		_id = id;
   		_typeref = t;
   		_symtype = 0;
      }

#ifndef macintosh
// Macintosh version is not inline: moved to sym_utils.cpp
//-------------------------------------------
INLINE Boolean SymFile::seek(int32 off,uint32 base_kind)
//-------------------------------------------
{
    return (Boolean)(test() && (!fseek(_fp,off+_base,base_kind)));
}
#endif

//-------------------------------------------
INLINE Boolean SymFile::read(void* buf,uint32 size)
//-------------------------------------------
{ 
#ifdef macintosh
	return (Boolean)(test()
	    	 && (FSRead(fRefNum, (long*)&size, buf) == noErr));
#else
	return (Boolean)(test() && (fread(buf,1,size,_fp)==size));
#endif
}
