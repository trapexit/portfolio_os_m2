/*  @(#) sym_dump.cpp 96/07/25 1.17 */


#ifndef USE_DUMP_FILE
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "predefines.h"
//#include "SymbolsUtil.h"
#include "symnet.h"
#endif /* USE_DUMP_FILE */
#include "dumpopts.h"
#include "symdump.h"

#include "dumputils.h"
#include "debug.h"

#pragma segment symnet

#ifdef macintosh
	// Macintosh uses tabs of 4 normally...
	#define _TAB 4
#else
	#define _TAB 8
#endif

#define type_name(t) ((t&&((long)t>0)&&t->name())?t->name():"(null)")
static char* sec_type(SymSec t);
static char* sec_scope(SymScope s);
static char* sec_class(SymbolClass c);

inline char* get_typename(SymType* t) { return ((t&&((long)t>0)&&t->name())?t->name():"(null)"); }
inline uint32 get_typesize(SymType* t) { return (t&&((long)t>0)?t->size():0); }
inline uint32 get_typeref(SymType* t) { return (t&&((long)t>0)?(uint32)t->type():0); }
static char* get_catname(SymType* t);

SymDump::SymDump(SymNet* symnet,FILE* dump_fp,GetOpts* opts) : Outstrstuffs(dump_fp) {
	DBG_ENT("SymDump");
	_dump_fp = dump_fp;
	_symnet = symnet;
	_dump_opts = opts;
	}

SymDump::~SymDump() {
	DBG_ENT("~SymDump");
	}

void SymDump::dump_entry(Outy* out) {
	uint32 entry;
	uint32 soff;
    StringPtr fname;
	if (_symnet->AddressOf((StringPtr)"\7__start",
				entry)==se_success) {
		out->print("entry point: 0x%X\n",entry);
        if (_symnet->Address2Source(entry,(StringPtr*)&fname,&soff)==se_success)
        	out->print("in module %s at source offset=x%X\n",fname+1,soff);
		}
	else {
		//_symnet->_state._s.force_validate();
		_symnet->Validate();
		}
	}

void SymDump::dump_sec_hdr(Outy* out) {
	out->print("secnum   secname      baseaddr\n");
	out->print("------------------------------\n");
	}
void SymDump::dump_sec(Outy* out,int n) {
	uint32 baddr = _symnet->_sections->baseaddr(n);
	SymSec sec = _symnet->_sections->sectype(n);
		out->print("  %-4d   %-12s x%-8x\n",
			n,sec_type(sec),baddr);
	}
void SymDump::dump_sections(Outy* out) {
	out->print("dumping sections\n");
	dump_sec_hdr(out);
	if (_symnet->_sections->nsecs()>0) {
		for (int i=0; i<_symnet->_sections->nsecs(); i++) {
			dump_sec(out,i);
			}
		out->print("\n");
		}
	else
		out->print("           no sections found for this file\n");
	out->print("\n\n");
	}
void SymDump::dump_line_hdr(Outy* out) {
	out->print("line: line   srcoffset  src2line   codeaddr\n");
	out->print("      -------------------------------------\n");
	}
void SymDump::dump_line(Outy* out,SymLineEntry* l,SymModEntry* m) {
	uint32 soff = m->charoff(l->line());
	out->print("       %-8d x%-8x %-8d x%-8x\n",
		l->line(),soff,m->linenum(soff),_symnet->rel2abs(sec_code,l->addr()));
	}
void SymDump::dump_type_hdr(Outy* out) {
	out->print("        -------------------------------------------------------\n");
	out->print("        typename     typeaddr    typecat     typesize   typeref\n");
	out->print("        =======================================================\n");
	}
void SymDump::dump_lines(Outy* out) {
	out->print("dumping lines\n");
	if (_symnet->_symroot->mods()) {
		for (SymModEntry* m=_symnet->_symroot->mods(); m; m=m->next()) {
			out->print("module %s:\n",m->name()?m->name():0);
			dump_line_hdr(out);
			if (m->lines()) {
				for (SymLineEntry* l=m->lines(); l; l=l->next())
					dump_line(out,l,m);
				out->print("\n");
				}
			else
				out->print("           no line information found for module %s\n",m->name()?m->name():"(null)");
#ifdef DEBUG
			out->print("\nmodule %s (sorted by addr):\n",m->name()?m->name():0);
			dump_line_hdr(out);
			if (m->lines_sorted_by_addr()) {
				for (SymLineEntry* l=m->lines_sorted_by_addr(); l; l=l->next())
					dump_line(out,l,m);
				out->print("\n");
				}
			else
				out->print("           no line information found for module %s\n",m->name()?m->name():"(null)");
#endif
			}
		out->print("\n");
		}
	else
		out->print("           no modules found for this file\n");
	out->print("\n\n");
	}
void SymDump::dump_types(Outy* out) {
	//types 
	out->print("dumping types\n");
	if (_symnet->_symroot->types()) {
		SymType* t;
		dump_type_hdr(out);
		for (t = _symnet->_symroot->types(); t; t=t->next()) {
			dump_type(out,t);
			}
		}
	else
		out->print("           no types found for this file \n");
	out->print("\n\n");
	}
void SymDump::dump_type(Outy* out,SymType* t) {
	if (!t || (long)t<0) return;
	if (t->cat()==tc_member) {
		return;	//we'll print this as part of what it belongs to
		}
	out->print("        %-12s ",
		get_typename(t));
	if (get_typename(t) && strlen(get_typename(t))>12)
		out->print("\n                     ");
	out->print_cont("x%-8x   %-11s x%-8x  x%-8x\n",
		t,get_catname(t),t->size(),t->type());
	if (t->cat()==tc_struct || t->cat()==tc_union) {
		out->indent();	//{
		SymStructType* s = (SymStructType*)t->type();
		if (t->cat()==tc_struct) 
			dump_struct_hdr(out,s);
		else
			dump_union_hdr(out,s);
		dump_fields(out,s);
		out->unindent();	//}
		}
	if (t->cat()==tc_array) {
		out->indent();	//{
		SymArrayType* a = (SymArrayType*)t->type();
		dump_array_hdr(out,a);
		out->unindent();	//}
		}
	if (t->cat()==tc_enum) {
		out->indent();	//{
		SymEnumType* e = (SymEnumType*)t->type();
		dump_enum_hdr(out,e);
		dump_enum(out,e);
		out->unindent();	//}
		}
	if (t->next() && 
		(t->cat()==tc_struct || t->cat()==tc_union 
		|| t->cat()==tc_array || t->cat()==tc_enum)) {
		dump_type_hdr(out);
		}
	}
void SymDump::dump_struct_hdr(Outy* out,SymStructType* t) {
	out->print("        --------------------------------------------------------------\n");
	out->print("struct: name=%s, nfields=%d, size=x%X\n",t->name()?t->name():"(null)",t->nfields(),t->size());
	out->print("fields: name        offset    typeaddr   typecat     typesize  typeref\n");
	out->print("        ==============================================================\n");
	}
void SymDump::dump_union_hdr(Outy* out,SymStructType* t) {
	out->print("        --------------------------------------------------------------\n");
	out->print("struct: name=%s, nfields=%d, size=x%X\n",t->name()?t->name():"(null)",t->nfields(),t->size());
	out->print("fields: name        offset    typeaddr   typecat     typesize  typeref\n");
	out->print("        ==============================================================\n");
	}
void SymDump::dump_fields(Outy* out,SymStructType* s) {
	int i;
	SymType* t;
	char* name;
	uint32 offset;
	for (i=0; i<s->nfields(); i++) {
		name = s->fieldname(i)?s->fieldname(i):"(null)";
		offset = s->fieldoffset(i);
		t = s->fieldtype(i);
		DBG_ASSERT(t);
		out->print("        %-12s ",
			name);
		if (name && strlen(name)>12)
			out->print("\n                     ");
		out->print_cont("x%-8x x%-8x %-11s x%-8x x%-8x\n",
			offset,t,get_catname(t),t?t->size():0,t?t->type():0);
		}
	}
void SymDump::dump_array_hdr(Outy* out,SymArrayType* a) {
	char* name = a->name()?a->name():"(null)";
	SymType* mt = a->membtype();
	char* mname = mt->name()?mt->name():"(null)";
	out->print("           --------------------------------------------------------------------------\n");
	out->print("array:     name=%s",name);
	out->print_cont("nmembers=%d, size=x%X, lowerbound=x%X, upperbound=x%X, basetype=x%-8x(%s)\n",
		a->nmembers(),a->size(),a->lowerbound(),a->upperbound(),mt,mname);
	}
void SymDump::dump_enum_hdr(Outy* out,SymEnumType* e) {
	char* name = e->name()?e->name():"(null)";
	SymType* mt = e->membtype();
	char* mname = mt->name()?mt->name():"(null)";
	out->print("           -------------------------------------------------------------------------\n");
	out->print("enum:      name=%s, nmembers=%d, size=x%X, basetype=x%-8x(%s)\n",
		name,e->nmembers(),e->size(),mt,mname);
	out->print("members:   name     val typeaddr\n");
	out->print("           =========================================================================\n");
	}
void SymDump::dump_enum(Outy* out,SymEnumType* e) {
	int i;
	char* name;
	uint32 val;
	SymType* t;
	for (i=0; i<e->nmembers(); i++) {
		name = e->membername(i)?e->membername(i):"(null)";
		val = e->memberval(i);
		t = e->membtype();
		out->print("        %-12s ",name);
		if (name && strlen(name)>12)
			out->print("\n                      ");
		out->print_cont("x%-8x x%-8x\n",val,t);
		}
	}

void SymDump::dump_sym_hdr(Outy* out) {
	out->print("           -------------------------------------------------------------------------------------------------------\n");
	out->print("symbols:   name       value    typecat     typename   typesize typeaddr typeref  section      scope        class\n");
	out->print("           =======================================================================================================\n");
	}
void SymDump::dump_sym(Outy* out,SymEntry* s) {
	out->print("           %-10s ",s->name()?s->name():"(null)");
	if (s->name() && strlen(s->name())>10)
		out->print("\n                      ");
	out->print_cont("x%-8x %-11s %-10s x%-8x x%-8x x%-8x %-12s %-12s %-8s\n",
		_symnet->rel2abs(s),
		get_catname(s->type()),get_typename(s->type()),get_typesize(s->type()),s->type(),get_typeref(s->type()),
		sec_type(s->sectype()),sec_scope(s->scope()),sec_class(s->stgclass()));
	}
void SymDump::dump_symbols(Outy* out) {
	SymRootLink* rl;
	out->print("dumping external symbols\n");
	dump_sym_hdr(out);
	if (_symnet->_symroot->gsyms()) {
		for (rl = _symnet->_symroot->gsyms(); rl; rl=rl->next()) {
			dump_sym(out,rl->sym());
			}
		out->print("\n");
		}
	else
		out->print("           no external symbols found for this file \n");
	out->print("\n\n");

	//external funcs
	out->print("dumping external functions\n");
	dump_func_hdr(out);
	if (_symnet->_symroot->gfuncs()) {
		for (rl = _symnet->_symroot->gfuncs(); rl; rl=rl->next()) {
			dump_func(out,rl->func());
			}
		out->print("\n");
		}
	else
		out->print("           no external functions found for this file \n");
	out->print("\n\n");
	}
void SymDump::dump_debug(Outy* out) {
	out->print("dumping debug info\n");
	dump_modules(out);
	dump_types(out);
	}
void SymDump::dump_mod_hdr(Outy* out) {
	out->print("        -----------------------------------\n");
	out->print("module: modname              baddr    eaddr\n");
	out->print("        ===================================\n");
	}
void SymDump::dump_mod(Outy* out,SymModEntry* m) {
	out->print("        %-20s ",m->name()?m->name():"(null)");
	if (m->name() && strlen(m->name())>20)
		out->print("\n                             ");
	out->print_cont("x%-8x x%-8x\n",
		_symnet->rel2abs(sec_code,m->baddr()),_symnet->rel2abs(sec_code,m->eaddr()));
	}
void SymDump::dump_modules(Outy* out) {
	if (_symnet->_symroot->mods()) {
		for (SymModEntry* m = _symnet->_symroot->mods(); m; m=m->next()) {
			dump_mod_hdr(out);
			dump_mod(out,m);
			out->indent();	//{
			dump_mod_syms(out,m);
			dump_mod_funcs(out,m);
			out->unindent();	//}
			}
		out->print("\n");
		}
	else
		out->print("           no modules found for this file \n");
	out->print("\n\n");
	}
	
void SymDump::dump_func_hdr(Outy* out) {
	out->print("           -------------------------------------------------------------------------------------------------------------------------------\n");
	out->print("function:  name         baddr    eaddr    value     typecat     typename     typesize typeaddr   typeref  section      scope        class\n");
	out->print("           ===============================================================================================================================\n");
	}
void SymDump::dump_block_hdr(Outy* out) {
	out->print("           ---------------------------------------------------------------------------------------------------------------\n");
	out->print("blocks:    baddr    eaddr    value    typecat       typename   typesize typeaddr typeref  section      scope        class\n");
	out->print("           ===============================================================================================================\n");
	}
void SymDump::dump_func(Outy* out,SymFuncEntry* f) {
	SymEntry* s;			
	s=f->sym();
	out->print("           %-12s ",s->name()?s->name():" ");
	if (s->name() && strlen(s->name())>10)
		out->print("\n                        ");
	out->print_cont("x%-8x ",_symnet->rel2abs(sec_code,f->baddr()));
	out->print_cont("x%-8x  ",_symnet->rel2abs(sec_code,f->eaddr()));
	out->print_cont("x%-8x ",_symnet->rel2abs(s));
	out->print_cont("%-11s   ",get_catname(s->type()));
	out->print_cont("%-10s ",get_typename(s->type()));
	out->print_cont("x%-8x ",get_typesize(s->type()));
	out->print_cont("x%-8x   ",s->type());
	out->print_cont("x%-8x ",get_typeref(s->type()));
	out->print_cont("%-12s ",sec_type(s->sectype()));
	out->print_cont("%-12s ",sec_scope(s->scope()));
	out->print_cont("%-8s\n",sec_class(s->stgclass()));
	}
void SymDump::dump_block(Outy* out,SymFuncEntry* f) {
	SymEntry* s;			
	s=f->sym();
	out->print("       x%-8x ",_symnet->rel2abs(sec_code,f->baddr()));
	out->print_cont("x%-8x ",_symnet->rel2abs(sec_code,f->eaddr()));
	out->print_cont("x%-8x ",_symnet->rel2abs(s));
	out->print_cont("%-11s ",get_catname(s->type()));
	out->print_cont("%-10s ",get_typename(s->type()));
	out->print_cont("x%-8x ",get_typesize(s->type()));
	out->print_cont("x%-8x ",s->type());
	out->print_cont("x%-8x ",get_typeref(s->type()));
	out->print_cont("%-12s ",sec_type(s->sectype()));
	out->print_cont("%-12s ",sec_scope(s->scope()));
	out->print_cont("%-8s\n",sec_class(s->stgclass()));
	}
void SymDump::dump_func_syms_n_blocks(Outy* out,SymFuncEntry* f) {
	SymFuncEntry* b;
	SymFuncLink* fl;
	SymEntry* s;
		if (f->name())
			dump_func(out,f);
		else
			dump_block(out,f);
		out->indent();	//{
		if (f->lsyms()) {
			dump_sym_hdr(out);
			for (fl = f->lsyms(); fl; fl=fl->next()) {
				s=fl->sym();
				dump_sym(out,s);
				}
			out->print("\n");
			}
		if (f->blocks()) {
			dump_block_hdr(out);
			out->indent();	//{	//we do this since there is no name
			for (b = f->blocks(); b; b=b->next()) {
				dump_func_syms_n_blocks(out,b);
				}
			out->unindent();	//}
			out->print("\n");
			}
		out->print("\n");
		out->unindent();	//}
		}
		
void SymDump::dump_mod_funcs(Outy* out,SymModEntry* m) {
	SymModLink* ml;
		if (m->mgfuncs() || m->mfuncs()) {
			//global funcs within modules
			if (m->mgfuncs()) {
				out->print("\n");
				out->print("global functions:\n");
				for (ml = m->mgfuncs(); ml; ml=ml->next()) {
					dump_func_hdr(out);
					dump_func_syms_n_blocks(out,ml->func());
					}
				}
				//static funcs within modules
			if (m->mfuncs()) {
				out->print("\n");
				out->print("static functions:\n");
				for (ml = m->mfuncs(); ml; ml=ml->next()) {
					dump_func_hdr(out);
					dump_func_syms_n_blocks(out,ml->func());
					}
				}
			}
		else
			out->print("    no global functions found for this module \n");
		out->print("\n");
		}

void SymDump::dump_mod_syms(Outy* out,SymModEntry* m) {
	SymModLink* ml;
		if (m->mgsyms() || m->msyms()) {
			//global syms within modules
			if (m->mgsyms()) {
				out->print("\n");
				out->print("global data:\n");
				dump_sym_hdr(out);
				for (ml = m->mgsyms(); ml; ml=ml->next()) {
					dump_sym(out,ml->sym());
					}
				}
			if (m->msyms()) {
				//static syms within modules
				out->print("\n");
				out->print("static data:\n");
				dump_sym_hdr(out);
				for (ml = m->msyms(); ml; ml=ml->next()) {
					dump_sym(out,ml->sym());
					}
				}
			out->print("\n");
			}
		else
			out->print("    no global symbols found for this module \n");
		}

void SymDump::Dump(FILE* dump_fp) {
	if (!_dump_opts->isset(dumpopt_symnet))
		return;
    
	Outy* out = (Outy*) SYM_NEW(Outy(dump_fp));
	if (!out) return;

	out->print("\n\n");
	out->print("Dump of debug info for file %s\n\n",_symnet->_fp->filename());

    dump_entry(out); //entry point
    dump_sections(out); //sections
    dump_lines(out); //lines
    dump_symbols(out); //external syms
    dump_debug(out); //modules & locals & types
	
	SYM_DELETE(out);
	}
	
static char* sec_type(SymSec t) {
	switch(t) {
		case sec_none: 		return "none";
		case sec_code: 		return "code";
		case sec_data: 		return "data";
		case sec_bss: 		return "bss";
		case sec_debug: 	return "debug";
		case sec_symtab: 	return "symtab";
		case sec_strtab: 	return "strtab";
		case sec_shstrtab: 	return "shstrtab";
		case sec_init: 		return "init";
		case sec_fini: 		return "fini";
		case sec_line: 		return "line";
		case sec_relatext: 	return "relatext";
		case sec_reladata: 	return "reladata";
		case sec_reladebug: return "reladebug";
		case sec_relaline: 	return "relaline";
		case sec_abs: 		return "abs";
		case sec_com: 		return "com";
		case sec_und: 		return "und";
		default: 			return "unknown";
		}
	}
static char* sec_scope(SymScope s) {
	switch(s) {
		case scope_none: 	return "scope_none";
		case scope_global: 	return "scope_global";
		case scope_module: 	return "scope_module";
		case scope_local: 	return "scope_local";
		default: 			return "unknown";
		}
	}
static char* sec_class(SymbolClass c) {
	switch(c) {
		case sc_none: 	return 	"sc_none";
		case sc_code: 	return 	"sc_code";
		case sc_data: 	return 	"sc_data";
		case sc_bss: 	return 	"sc_bss";
		case sc_stack: 	return 	"sc_stack";
		case sc_reg: 	return 	"sc_reg";
		case sc_abs: 	return 	"sc_abs";
		case sc_const: 	return 	"sc_const";
		default: return 		"unknown";
		}
	}
static char* get_catname(SymType* t) {
	if (!t||((long)t<0)) return "unknown";
	switch (t->cat()) {
    	case tc_void:			return "tc_void";
    	case tc_pointer:		return "tc_pointer";
    	case tc_char:			return "tc_char";
    	case tc_uchar:			return "tc_uchar";
    	case tc_short:			return "tc_short";
    	case tc_ushort:			return "tc_ushort";
    	case tc_int:			return "tc_int";
    	case tc_uint:			return "tc_uint";
    	case tc_long:			return "tc_long";
    	case tc_ulong:			return "tc_ulong";
    	case tc_float:			return "tc_float";
    	case tc_double:			return "tc_double";
    	case tc_ldouble:		return "tc_ldouble";
    	case tc_complex:		return "tc_complex";
    	case tc_dcomplex:		return "tc_dcomplex";
    	case tc_llong:			return "tc_llong";
    	case tc_ullong:			return "tc_ullong";
    	case tc_array:			return "tc_array";	
    	case tc_struct:			return "tc_struct";
    	case tc_union:			return "tc_union";
    	case tc_enum:			return "tc_enum";
    	case tc_member:			return "tc_member";
    	case tc_function:		return "tc_function";
    	case tc_arg:			return "tc_arg";
    	case tc_ref:			return "tc_ref";
#ifdef __3DO_DEBUGGER__	
    	case tc_reg:			return "tc_reg";
#endif /* __3DO_DEBUGGER__ */
    	case tc_unknown:		return "tc_unknown";
		default:
				DBG_ERR(("unknown cat type x%X\n",t));
				return "unknown    ";
		}
	}

SymError* SymDump::GetState() {
	DBG_(SYMS,("_state._s._state=x%X\n",_symnet->_state._s.state()));
        return &_symnet->_state;
        }

char* SymDump::GetSymFFStr(SymFileFormat ftype) {
	switch (ftype) {
		case eAR: return "Ar";
		case eELF: return "Elf";
		case eARM: return "Arm sym";
		case eXCOFF: return "xcoff";
		case eXSYM: return "xsym";
		default: return "-unknown-";
		}
	}

//Address to string conversion for disassembler 
//returns "function_name+offset" for addr
void SymDump::AddrToStr(uint32 addr, char* result) {
	DBG(("looking for symbol for addr x%X\n",addr));
	DBG_ASSERT(result);
	SymEntry* s;
	if (result) {
		if (s=_symnet->GetFuncSym(addr),!s) {	//find function which contains addr
			sprintf(result,"%08x",addr);
			}
		else {
			char* symname;
			uint32 symaddr;
			symname = _symnet->GetSymName(s);
			symaddr = _symnet->GetSymAddr(s);
			DBG(("name-%s;addr-%08x\n",symname,symaddr));

			uint32 offset = addr-symaddr;
			if (offset>0)
				sprintf(result,"%s+%X",symname,offset);
			else
				sprintf(result,"%s",symname);
			}
		}
	}

#define BUF_SIZE 1024
#include "ppc_disasm.h"
//given a buffer of text, disassemble and print each instruction
void SymDump::DumpDisasm(uint32 sect_base, uint8* text,uint32 text_size) {
	if (_symnet->SymFF() != eELF)
	{
		fprintf(_dump_fp,"only PPC 602 disassembly is supported\n");
		return;
	}
	uint32* inst;
	char buf[BUF_SIZE];
	char *bufp=buf, *p, offstr[BUF_SIZE];
	uint32 off, addr;
	memset(buf,0,BUF_SIZE);
	for (off=0; off<text_size; off+=4) {
		inst=(uint32*)(&text[off]);
		*bufp=0;	//so dis602 won't append
		busy_cursor();
		dis602(&inst,bufp);
		//resolve *s
		//if string has a *, it's relative to current offset
		if (p=strchr(bufp,'*'),p) {	//bl	*0x0
			//* means current offset
			DBG(("\np=%s\n",p));
			DBG(("ch2val(p)=%X\n",ch2val(p)));
			addr = sect_base + off + ch2val(p);
			DBG(("adr=%X\n",addr));
			sprintf(p,"%08x",addr);
			DBG(("new p=%s\n\n",p));
			}
		//if statement is a branch, resolve branch address
		if (!strncmp(bufp,"bl",2)
			|| !strncmp(bufp,"bn",2)) {
			if (p=strchr(bufp,'\t'),p) {	//bl	*0x0
				uint32 addr;
				char addrstr[BUF_SIZE];
				p++;	//get past tab to address
				DBG(("\np=%s\n",p));
				DBG(("ch2val(p)=%X\n",strtoul(p,0,16)));
				addr = strtoul(p,0,16);
				DBG(("adr=%X\n",addr));
				AddrToStr(addr, addrstr);	//address = symbol + off
				p+=8;	//get past branch address
				sprintf(p,"\t<%s>",addrstr);
				}
			}
		AddrToStr(off + sect_base, offstr);	//text_offset = symbol + off
		fprintf(_dump_fp,"%08x <%s> %s\n",off + sect_base,offstr,bufp);
		}
	}
