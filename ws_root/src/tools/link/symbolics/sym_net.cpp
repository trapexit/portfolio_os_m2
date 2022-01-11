/*  @(#) sym_net.cpp 96/07/25 1.27 */


//==========================================================================
// sym_net.cpp	- definitions for SymNet class
//				  and other helper classes
//
// the idea here is to keep one network for searching etc,
// and just have specialized classes for reading the symbols
// into the symbol network.

#ifndef USE_DUMP_FILE
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "predefines.h"
//#include "SymbolsUtil.h"
#include "utils.h"
#include "symnet.h"
#endif /* USE_DUMP_FILE */

#include "debug.h"
#if defined(USE_DUMP_FILE)
	#include "symnet_inlines.h"
#endif


#pragma segment symnet

//Heap* SymNet::_heap=0;
//Heap* SymNet::_tmp_heap=0;

//==========================================================================
//error handling

uint32 SymNet::sym_fread(unsigned char* buf,uint32 size,uint32 n,FILE* fp, const char* file,uint32 line) {
	uint32 p=fread(buf,size,n,fp);
	if (p!=n) {
		set_state(se_read_err,file,line); 
		DBG_ERR(("se_read_err occurred.\n")); 
		DBG_OUT_(ERR,("# Unable to read x%X bytes!\n",size*n)); 
		return 0;
		}
	return p;
	}
	
uint32 SymNet::sym_fseek(uint32 off,uint32 pos,FILE* fp, const char* file,uint32 line) {
	uint32 p=fseek(fp,off,pos);
	if (!p) {
		set_state(se_seek_err,file,line); 
		DBG_ERR(("se_seek_err occurred.\n")); 
		DBG_OUT_(ERR,("# Unable to seek tp offset x%X!\n",off)); 
		return 0;
		}
	return p;
	}
	
//==========================================================================
// types

SymType* SymNet::add_type(const char* name,uint32 size,SymCat cat,void* symt) {
	DBG_(SYMNET,("add_type: added type name=%s, size=%d, cat=x%X, symt=x%X\n",name?name:"(null)",size,cat,symt));
	SymType* t = _symroot->add_type(name,size,cat,symt);
	DBG_ASSERT(t);
	return t;
	}
      
SymType* SymNet::search_types(const char* n) {
	SymType* t;
	DBG_ASSERT(n);
	for (t=_symroot->types(); t; t=t->next())
	  if (t->name() && !mystrcmp(t->name(),n)) return t;
	return 0;
	}
      
//kludge for now...
//add fundamental symbol type - keeps track of whether type has
//been already added.  If so, return that type.
SymType* SymNet::get_fun_type(SymCat t) {
	#define STR(x) #x
	#define return_sym_type(stc) { \
		if (!st_##stc) \
			st_##stc = add_type(STR(stc),get_type_size(tc_##stc),tc_##stc,0);	\
		return st_##stc;	\
		}
	switch (t) {
    	case tc_void:				return_sym_type(void);
		case tc_char:				return_sym_type(char);
		case tc_uchar:				return_sym_type(uchar);
    	case tc_short:				return_sym_type(short);
    	case tc_ushort:				return_sym_type(ushort);
    	case tc_int:				return_sym_type(int);
    	case tc_uint:				return_sym_type(uint);
    	case tc_long:				return_sym_type(long);
    	case tc_ulong:				return_sym_type(ulong);
    	case tc_float:				return_sym_type(float);
    	case tc_double:				return_sym_type(double);
    	case tc_string:				return_sym_type(string);
    	//when there is nothing to reference, we can use
    	//funcs, ptrs, refs, etc as fun types
    	case tc_function:			return_sym_type(function);
    	case tc_pointer:			return_sym_type(pointer);
    	case tc_ref:				return_sym_type(ref);
		default:
				SET_ERR(se_unknown_type,("unknown fundamental type x%X\n",t));
				return 0;
		}
	}
	
//given type returns sym type size (override if processor differs)
//return size of types on target for each type category
uint32 SymNet::get_type_size(SymCat cat) {
    switch (cat) {
        case tc_void    : return 0;
        case tc_char    : return 1;
        case tc_uchar   : return 1;
        case tc_short   : return 2;
        case tc_ushort  : return 2;
    	case tc_int		: return 4;
    	case tc_uint	: return 4;
        case tc_long    : return 4;
        case tc_ulong   : return 4;
        case tc_float   : return 4;
        case tc_double  : return 8;
        case tc_ldouble : return 16;
        case tc_complex : return 8;
        case tc_dcomplex: return 16;
        case tc_string	: return 1;	//??? 1st char is length!
        case tc_function: return 4; //???
        case tc_pointer : return 4;
        default: 
        	SET_ERR(se_parm_err,("unknown cat type x%X\n",cat)); 
        	return 0;
        }
   }
   
      
#pragma segment newsymnet
//==========================================================================
//  addressing routines
	
Boolean SymNet::inside(uint32 addr,SymModEntry* m) { 
	DBG_(SYMNET,("checking inside: addr=x%04x, m->baddr=x%04x,m->eaddr=x%04x\n",addr,m->baddr(),m->eaddr()));
	addr = abs2rel(sec_code,addr);
	return (inside_rel(addr,m)); 
	}
   		
Boolean SymNet::inside(uint32 addr,SymFuncEntry* f) { 
	addr = abs2rel(sec_code,addr);
	return (inside_rel(addr,f)); 
	}
   		
Boolean SymNet::inside_rel(uint32 addr,SymModEntry* m) { 
	DBG_(SYMNET,("checking inside_rel: addr=x%04x, m->baddr=x%04x,m->eaddr=x%04x\n",addr,m->baddr(),m->eaddr()));
   		return (Boolean)(m->baddr()<=addr && m->eaddr()>=addr); 
   		}
Boolean SymNet::inside_rel(uint32 addr,SymFuncEntry* f) { 
   		return (Boolean)(f->sym()->val()<=addr && f->eaddr()>=addr); 
   		//return (f->baddr<=addr && f->eaddr>=addr); 
   		}
// relocate symbol address to its physical address
uint32 SymNet::abs2rel(SymSec sectype,uint32 addr) { 
	return addr - _sections->baseaddr(_sections->secnum(sectype)); 
	}

uint32 SymNet::rel2abs(SymSec sectype,uint32 addr) { 
	return addr + _sections->baseaddr(_sections->secnum(sectype)); 
	}
		
unsigned long SymNet::rel2abs(SymEntry* s) { //relative offset to absolute address
   	DBG_ASSERT(s);
   	DBG_(SYMNET,("sym==> name=%s, val=x%X, cat=x%X, sectype=x%X, scope=x%X, stgclass=x%X, baseaddr=x%04x\n",
   				s->name()?s->name():"(null)",s->val(),s->type()?s->type()->cat():0,s->sectype(),s->scope(),s->stgclass(),_sections->baseaddr(_sections->secnum(s->sectype()))));
    switch (s->stgclass()) {
#if 0	//defined(__3DO_DEBUGGER__)
		//debugger does register evaluation...
       case sc_reg: 
       		return hwreg(s->val()); //value is reg in this case
#else
       case sc_reg: 
       		return (s->val()); //value is reg in this case
#endif
       case sc_data: 
       case sc_bss: 
       case sc_code:
       		return (rel2abs(s->sectype(),s->val()));
       case sc_stack:
       case sc_none:
       case sc_const:
       case sc_abs:
       		return (s->val());
       default:	
       		DBG_ERR(("unknown storage class x%X\n",s->stgclass()));
       		return (s->val());
       }
    return (0);
    }
    
// relocate symbol address to its physical address
unsigned long SymNet::abs2rel(SymEntry* s) { //relative offset to absolute address
   	DBG_ASSERT(s);
    switch (s->stgclass()) {
       case sc_reg: 
       		return (s->val()); //value is reg in this case
       case sc_data: 
       case sc_bss: 
       case sc_code:
   			DBG_(SYMNET,("s->scope=x%X; sectype=x%X, stgclass=x%X\n",s->scope(),s->sectype(),s->stgclass()));
       		return (abs2rel(s->sectype(),s->val()));
       case sc_stack:
       case sc_none:
       		return (s->val());
       default:
       		DBG_ERR(("unknown storage class x%X\n",s->stgclass()));
       		return (s->val());
       }
    return (0);
    }
    
#if 0	//def __3DO_DEBUGGER__
		//debugger does register evaluation...
extern uint32 GetExecuteAddr();
	
uint32 SymNet::hwreg(uint32 r) {
	DBG_ENT("hwregs");
	uint32 reg_val;
	switch (r)	{
		case REG_PC:
			reg_val = GetExecuteAddr();
			break;
		case REG_2:
		case REG_3:
		case REG_4:
		case REG_5:
		case REG_6:
		case REG_7:
		case REG_8:
		case REG_9:
		case REG_14:
			reg_val = r;	//until I figure out a call for these...
			break;
		default:
			reg_val = 0;
			SET_ERR(se_unknown_type,("unknown reg %d\n",r));
		}
	return reg_val;
	}
#endif /* 0 */
    
//==========================================================================
//  searching symbol network

		#define CMP_FUNC(s,t) (this->*cmp_func)(s,t)
		#define CMP_SYM(s,t) (this->*cmp_sym)(s,t)
		#define CMP_MOD(s,t) (this->*cmp_mod)(s,t)
		#define CMP_LINE(s,t) (this->*cmp_line)(s,t)
		
//comparison functions to be passed to search functions
//symbol comparisons
Boolean SymNet::cmp_name(SymEntry*s,void*n) { 
    DBG_ASSERT(n);
    DBG_ASSERT(s);
    DBG_ASSERT(s->name());
	return (Boolean)(s && s->name() && n && !mystrcmp(s->name(),(char*)n)); 
	}
Boolean SymNet::cmp_addr(SymEntry*s,void*a) { 
    DBG_ASSERT(s);
	return (Boolean)(rel2abs(s)== *((uint32*)a)); 
	}
Boolean SymNet::cmp_idx(SymEntry*s,void*i) { 
    DBG_ASSERT(s);
	return (Boolean)(++_sym_idx== *((int*)i)); 
	}

//function comparisons
Boolean SymNet::cmp_func_name(SymFuncEntry*f,void*n) { 
    DBG_ASSERT(f);
	return (Boolean)(f && f->name() && n && !mystrcmp(f->name(),(char*)n)); 
	}
Boolean SymNet::cmp_func_addr(SymFuncEntry*f,void*a) { 
    DBG_ASSERT(f);
	return (Boolean)(rel2abs(f->sym())== *((uint32*)a)); 
	}
Boolean SymNet::cmp_func_addrin(SymFuncEntry*f,void*a) { 
    DBG_ASSERT(f);
    return (Boolean)(rel2abs(sec_code,f->baddr()) <= *((uint32*)a) && 
    		rel2abs(sec_code,f->eaddr()) >= *((uint32*)a));
	}
Boolean SymNet::cmp_func_linein(SymFuncEntry*f,void*l) { 
    DBG_ASSERT(f);
    return (Boolean)(f->bline() <= *((uint32*)l) && 
    		f->eline() >= *((uint32*)l));
	}
	
//module comparisons
Boolean SymNet::cmp_mod_addrin(SymModEntry*m,void*a) { 
    DBG_ASSERT(m);
    return (Boolean)(rel2abs(sec_code,m->baddr()) <= *((uint32*)a) && 
    		rel2abs(sec_code,m->eaddr()) >= *((uint32*)a));
	}
Boolean SymNet::cmp_mod_linein(SymModEntry*m,void*l) { 
    DBG_ASSERT(m);
    return (Boolean)(0 <= *((int32*)l) && 
    		m->nlines() >= *((int32*)l));
	}
Boolean SymNet::cmp_mod_name(SymModEntry*m,void*n) { 
    DBG_ASSERT(m);
	return (Boolean)(m && m->name() && n && !mystrcmp(m->name(),(char*)n)); 
	}
	
//line comparisons
// lines must be sorted by addr in ascending order
Boolean SymNet::cmp_line_addr(SymLineEntry*l,void*a) { 
    DBG_ASSERT(l);
    DBG_(SYMNET,("l->line()=x%X; l->addr()=x%X; *((uint32*)a)=x%X\n",l->line(),l->addr(),*((uint32*)a)));
    uint32 addr = *((uint32*)a);
    return (Boolean)(rel2abs(sec_code,l->addr()) <= addr
		&& (l->next() ? rel2abs(sec_code,l->next()->addr()) > addr : true));
	}
// lines must be sorted by line in ascending order
// 3, 5, 9, ...	if want 7 return line at 5; stop when line>=a
Boolean SymNet::cmp_line_line(SymLineEntry*l,void*a) { 
    DBG_ASSERT(l);
    DBG_(SYMNET,("l->line()=x%X; l->addr()=x%X; *((uint32*)a)=x%X\n",l->line(),l->addr(),*((uint32*)a)));
    return (Boolean)(l->line() >= *((uint32*)a));
	}
	
//-------------------------------------------------------------------------
//search lines in a module 
//search lines in a module by lines
//		returns closest line entry for passed thing
SymLineEntry* SymNet::search_lines(cmp_line_t cmp_line,SymModEntry *m,void*thing) {
    SymLineEntry* l;
    DBG_ASSERT(m);
    DBG_ASSERT(thing);
	if (cmp_line==&SymNet::cmp_line_addr) 
		l = m->lines_sorted_by_addr();
	else
		l=m->lines();
    for (; l; l=l->next()) {
    	if (CMP_LINE(l,thing)) {
    		DBG_(SYMNET,("returning l->line()=x%X; l->addr()=x%X\n",l->line(),l->addr()));
          	return l;
          	}
    	}
    return 0;
    }
    
//-------------------------------------------------------------------------
// searching symbols

//search global data symbols
SymEntry* SymNet::search_gsyms(cmp_sym_t cmp_sym, void* thing) {
    SymRootLink* g;
	DBG_ASSERT(thing);
	//search externals
	for (g=_symroot->gsyms(); g; g=g->next()) {
	   DBG_ASSERT(g->sym());
	   if (CMP_SYM(g->sym(), thing))
		  return g->sym();  //return symbol
	   }
	//search symbols defined in modules
	//for (m=_symroot->mods(); m; m=m->next()) {
	//	if (s=search_mgsyms(cmp_sym,m,thing),s) 
	//			return s;
	//     }
	return 0;
       }
       
//search for global data defined in module
SymEntry* SymNet::search_mgsyms(cmp_sym_t cmp_sym, SymModEntry *m, void* thing) {
	SymModLink* s;
	DBG_ASSERT(thing);
	DBG_ASSERT(m);
	for (s=m->mgsyms(); s; s=s->next()) {
	   DBG_ASSERT(s->sym());
	   if (CMP_SYM(s->sym(),thing))
		  return s->sym();  //return data address of symbol
	   }
	return 0;
	}

//search for data sym local to module
SymEntry* SymNet::search_msyms(cmp_sym_t cmp_sym, SymModEntry *m, void* thing) {
	SymModLink* s;
	DBG_ASSERT(thing);
	DBG_ASSERT(m);
	if (!m->msyms()) return 0;
	for (s=m->msyms(); s; s=s->next()) {
	   DBG_ASSERT(s->sym());
	   if (CMP_SYM(s->sym(),thing))
		  return s->sym();  //return data address of symbol
	   }
	return 0;
	}

//search non-local data symbols for symbol
SymEntry* SymNet::search_data(cmp_sym_t cmp_sym,void*thing) {
	SymEntry* s;
	SymModEntry* m;
	//SOMEDAY. C++ member searches not implemented yet!
	//for (c=cur_class; c; c=c->base) { //walk up the scope looking for symbol
	//   if (sym=search_classdata(cmp_sym,c,thing),sym) return sym;
	//   }
	//funcp will be 0 when we reach the top func level (then modp is mod)
	if (s=search_gsyms(cmp_sym,thing),s) return s;
	for (m=_symroot->mods(); m; m=m->next())
		if (s=search_msyms(cmp_sym,m,thing),s) return s;
	return 0;
	}
       
//-------------------------------------------------------------------------
// search functions 

// search global funcs
SymFuncEntry* SymNet::search_gfuncs(cmp_func_t cmp_func, void* thing) {
	SymRootLink* rl=0;
	SymFuncEntry* f=0;
	DBG_ASSERT(thing);
	//search externals
	for (rl=_symroot->gfuncs(); rl; rl=rl->next()) {
	   DBG_ASSERT(rl->func());
	   if (CMP_FUNC(rl->func(),thing)) {
		  return rl->func();  //return func sym
		  }
	   }
	//search functions defined in modules
	//for (m=_symroot->mods(); m; m=m->next()) {
	//		if (f=search_mgfuncs(cmp_func,m,thing),f) 
	//			return f;
	//     }
	return 0;
	}
       
//search funcs local to module
SymFuncEntry* SymNet::search_mfuncs(cmp_func_t cmp_func, SymModEntry *m, void* thing) {
	SymModLink* f=0;
	DBG_ASSERT(thing);
	DBG_ASSERT(m);
	for (f=m->mfuncs(); f; f=f->next()) {
	   DBG_ASSERT(f->func());
	   if (CMP_FUNC(f->func(),thing)) {
		  return f->func();  //return func sym
		  }
	   }
	return 0;
	}
       
//search global funcs defined in module
//		this function was added to make scope searching easier 
SymFuncEntry* SymNet::search_mgfuncs(cmp_func_t cmp_func, SymModEntry *m, void* thing) {
	SymModLink* f=0;
	DBG_ASSERT(thing);
	DBG_ASSERT(m);
	DBG_(SYMNET,("m->name()=%s, thing=x%X\n",
		m->name()?m->name():"(null)",*((uint32*)thing)));
	DBG_(SYMNET,("m->mgfuncs()=x%X, m->mgfuncs()->func()=x%X, m->mgfuncs()->func()->name()=%s\n",
		m->mgfuncs(),m->mgfuncs()?m->mgfuncs()->func():0,(m->mgfuncs()&&m->mgfuncs()->func())? (m->mgfuncs()->func()->name()?m->mgfuncs()->func()->name():"(null)"):"notta"));
	for (f=m->mgfuncs(); f; f=f->next()) {
	   DBG_ASSERT(f->func());
		DBG_(SYMNET,("checking f->func()==> name=%s, baddr=x%X, eaddr=x%X, bline=x%X, eline=x%X\n",
				f->func()->name()?f->func()->name():"(null)",f->func()->baddr(),f->func()->eaddr(),f->func()->bline(),f->func()->eline()));
	   if (CMP_FUNC(f->func(),thing)) {
		  return f->func();  //return func sym
		  }
	   }
	return 0;
	}
       
//search all funcs
SymFuncEntry* SymNet::search_allfuncs(cmp_func_t cmp_func, void* thing) {
	SymFuncEntry* f=0;
	DBG_ASSERT(thing);
	uint32 i = 0; // 95/12/21 jrm prevent endless loops
	static unsigned long total = this->SymbolCount();  // 95/12/21 jrm prevent endless loops
	for (f=_symroot->funcs(); f && (i < total); f=f->next(), i++) {
	   if (CMP_FUNC(f,thing)) {
		  return f;  //return func sym
		  }
	   }
	return 0;
	}
              
//-------------------------------------------------------------------------
//search blocks

//search all blocks for a given func
SymFuncEntry* SymNet::search_blocks(cmp_func_t cmp_func, SymFuncEntry* f,void* thing) {
	SymFuncEntry *b_decendant, *b;
	DBG_ASSERT(f);
	DBG_ASSERT(thing);
	for (b=f->blocks(); b; b=b->next()) {
		if (CMP_FUNC(b,thing)) 	//me??
			return b;
		if (b_decendant=search_blocks(cmp_func,b,thing),b_decendant)
			return b_decendant;	//one of my kids??
		}
	return 0;
	}
              
       
//-------------------------------------------------------------------------
// search lsyms in functions/block

//search for local func/block sym
SymEntry* SymNet::search_lsyms(cmp_sym_t cmp_sym, SymFuncEntry *f, void* thing) {
	SymFuncLink* ls;
	DBG_ASSERT(f);
	DBG_ASSERT(thing);
	for (ls=f->lsyms(); ls; ls=ls->next()) {
	   DBG_ASSERT(ls->sym());
	   if (CMP_SYM(ls->sym(),thing))
		  return ls->sym();  //return stack symbol
	   }
	return 0;
	}
       
//-------------------------------------------------------------------------
// search mods
SymModEntry* SymNet::search_mods(cmp_mod_t cmp_mod, void* thing) {
	SymModEntry* m;
	DBG_ASSERT(thing);
	if (!_symroot->mods()) return 0;
	for (m=_symroot->mods(); m; m=m->next()) {
	   if (CMP_MOD(m,thing))
		  return m;
	   }
	return 0;
	}

//-------------------------------------------------------------------------
// scope sensative searches

//search code symbols for symbol - scope sensitive
SymEntry* SymNet::search_scope_code(cmp_func_t cmp_func,void*thing) {
	SymFuncEntry* f=0;
	//SOMEDAY! 
	//	C++ member function search not yet implemented
	//for (c=cur_class(); c; c=c->base()) { //walk up the scope looking for symbol
	//   if (f=search_classfuncs(&SymNet::cmp_func_addr,c,&addr),f) return sym->sym();
	//   }
	if (cur_mod() &&		//search funcs local to module
			(f=search_mfuncs(cmp_func,cur_mod(),thing),f))
		return f->sym();
	if (f=search_allfuncs(cmp_func,thing),f) 
		return f->sym();
	return 0;
	}
       
//search data symbols for symbol - scope sensitive
SymEntry* SymNet::search_scope_data(cmp_sym_t cmp_sym,void*thing) {
	SymEntry* s;
	SymFuncEntry* f;
	//SOMEDAY. C++ member searches not implemented yet!
	//for (c=cur_class; c; c=c->base) { //walk up the scope looking for symbol
	//   if (sym=search_classdata(cmp_sym,c,thing),sym) return sym;
	//   }
	//funcp will be 0 when we reach the top func level (then modp is mod)
	if (cur_block()) { //last sym.f will be cur_func()
	  //search current block and parent's block and their parents block etc...
	  for (f=cur_block(); f; f=f->func()) //walk up the scope looking for symbol
		 if (s=search_lsyms(cmp_sym,f,thing),s) return s;
	  }
	else {
	  if (cur_func()) {
		 if (s=search_lsyms(cmp_sym,cur_func(),thing),s) return s;
		 }
	  }
	if (cur_mod())
		if (s=search_msyms(cmp_sym,cur_mod(),thing),s) return s;
	if (s=search_gsyms(cmp_sym,thing),s) return s;
	return 0;
	}
       
//search thru symbols in scope for thing
SymEntry * SymNet::search_scope(cmp_sym_t cmp_sym,void*thing) {
	DBG_ENT("search_scope");
	#define RETURN(sym_) { 	\
		DBG_ASSERT(sym_);	\
		DBG_(SYMNET,("returning sym->name()=%s, sym->val()=x%04x, sym->scope()=x%04x, sym->sectype()=x%04x, sym->stgclass()=x%04x\n",sym_->name()?sym_->name():"(null)",sym_->val(),sym_->scope(), sym_->sectype(), sym_->stgclass()));	\
		return(sym_);	\
		}
	SymFuncEntry* f;
	//SymModEntry* m;
	SymModLink* mf;
	SymRootLink* gf;
	SymEntry* s;
	uint32 syms=0;
	
	DBG_ASSERT(cmp_sym);
	DBG_(PICKY,("parms: thing=x%X\n",thing));
	//search local symbols
	//SOMEDAY!! C++ support??
	if (cur_block()) {
	  DBG_(PICKY,("cur_block()\n"));
	  for (f=cur_block(); f; f=f->func()) //walk up the scope looking for symbol
		 if (s=search_lsyms(cmp_sym,f,thing),s) RETURN(s);
		}
	if (cur_func()) {
		if (s=search_lsyms(cmp_sym,cur_func(),thing),s) RETURN(s);
		}
	//search static symbols
	if (cur_mod()) {
		if (s=search_msyms(cmp_sym,cur_mod(),thing),s) RETURN(s); 
		for (mf=cur_mod()->mfuncs(); mf; mf=mf->next())
			if (CMP_SYM(mf->func()->sym(),thing)) RETURN(mf->func()->sym());
		}
	//search global symbols
	//if (s=search_gsyms(cmp_sym,thing),s) RETURN(s);
	//for (gf=_symroot->gfuncs(); gf; gf=gf->next())
	//	if (CMP_SYM(gf->func()->sym(),thing)) RETURN(gf->func()->sym());
	if (s=search_data(cmp_sym,thing),s) RETURN(s);
	for (gf=_symroot->gfuncs(); gf; gf=gf->next())
		if (CMP_SYM(gf->func()->sym(),thing)) RETURN(gf->func()->sym());
	//above should have checked globals; no need to test inside modules too
	//for (m=_symroot->mods(); m; m=m->next()) {
	//	for (mf=m->mgfuncs(); mf; mf=mf->next())
	//		if (CMP_SYM(mf->func()->sym(),thing)) RETURN(mf->func()->sym());
	//	}
	
	//search HW register symbols
	//	HW syms are just globals now...
	DBG_(SYMNET,("search_scope: symbol NOTFOUND! returning 0\n"));
	return 0;
	};
	
	
//-------------------------------------------------------------------------
// functions for searching for symbols anywhere in code/data/stack/all

// search global functions and functions local to modules
SymEntry* SymNet::search_code(uint32 addr) {
	SymFuncEntry* f=0;
	return (f=search_allfuncs(&SymNet::cmp_func_addr,(void*)&addr),f) 
		? f->sym() : 0;
	}

SymEntry* SymNet::search_code(const char* name) {
	SymFuncEntry* f=0;
	return (f=search_allfuncs(&SymNet::cmp_func_name,(void*)name),f) 
		? f->sym() : 0;
	}
       

//uses index to search thru scope for index counting symbols as we go
//if index=0, then will keep counting and count number of symbols
SymEntry * SymNet::symcounter(unsigned long& search_sym_idx) {
	DBG_ENT("symcounter");
	SymEntry* s;
	_sym_idx=0;	//initialize symbol index
	//funcp will be 0 when we reach the top func level (then modp is mod)
	DBG_(SYMNET,("searching for symbol at index=x%04x\n",search_sym_idx));
	if (s=search_scope(&SymNet::cmp_idx,(void*)&search_sym_idx),s) {
		DBG_(SYMNET,("symcounter: returning name=%s, address=x%04x\n",s->name()?s->name():"(NULL)",s->val()));
		return s;
		}
	if (search_sym_idx == 0) { 		// if 0, return number of symbols
		search_sym_idx = _sym_idx;
		DBG_(SYMNET,("symcounter: returning %d\n",search_sym_idx));
		}
	else 
		DBG_WARN(("symcounter: symbol NOTFOUND! returning 0\n"));
	return 0;
	};  // input is really MTE Index.

char* SymNet::rmv_path(const char sep, char* fullname, char*& path) {
	char* name;
	DBG_ASSERT(fullname);
	char *p = fmt_str("%s",fullname); //create temp space for name
	char *n = strrchr(p,sep);
	if (n) { n++; name = str(n); *n=0; path = p; }
	else { name = p; path = 0; }
	return name;
	}

void SymNet::set_err_msg(const char* fmt, ...)
{
		va_list ap;
		va_start(ap, fmt);
		_state.set_err_msg(fmt, ap);
		//vsprintf(msg, fmt, ap);
		va_end(ap);
}

