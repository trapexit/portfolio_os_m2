/*  @(#) symdump.h 96/07/25 1.6 */


//====================================================================
// symdump.h  -  SymDump class and structs for dumping
//
//		Symbolics class hierarchy:
//
//			SymNet	- builds internal network of symbols and queries them
//				SymDump - main dumping routines
//					ArDumper -  dumps unix archive files 
//					ElfDumper - dumps ELF files with dwarf v1.1 debug info and
//					SymDumper - dumps ARM sym files with sym debug info and
//					XcoffDumper - dumps XCOFF files with dbx stabs debug info and
//					XsymDumper - dumps XSYM files 


#ifndef __SYMDUMP_H__
#define __SYMDUMP_H__


//==========================================================================
//  includes

#include <stdlib.h>
#include <stdio.h>
#include "symnet.h"
#include "symapi.h"
#include "utils.h"
#include "symutils.h"
#include "dumputils.h"
#include "dumpopts.h"

//====================================================================
// SymDump class

#ifdef _CFRONT_BUG
class SymDump : public Outstrstuffs {
#else
class SymDump : protected Outstrstuffs {
#endif
public: 
    SymDump(SymNet*,FILE* dump_fp,GetOpts* opts);
    virtual ~SymDump();
	virtual void DumpFile()=0;
	void Dump(FILE* dump_fp=0);	//symbol network dump
    static char* GetSymFFStr(SymFileFormat ftype);	
    SymError* GetState();
	void DumpDisasm(uint32 text_base, uint8* text,uint32 text_size);
	void AddrToStr(uint32 addr, char* result);

protected:
	void dump_entry(class Outy*);
	void dump_sec_hdr(class Outy*);
	void dump_sec(class Outy*,int n);
	void dump_sections(class Outy*);
	void dump_type_hdr(class Outy* out);
	void dump_type(class Outy* out,SymType* t);
	void dump_types(class Outy*);
	void dump_struct_hdr(class Outy*,SymStructType*);
	void dump_union_hdr(class Outy*,SymStructType*);
	void dump_fields(class Outy*,SymStructType*);
	void dump_array_hdr(class Outy*,SymArrayType*);
	void dump_enum_hdr(class Outy*,SymEnumType*);
	void dump_enum(class Outy*,SymEnumType*);
	void dump_func_hdr(class Outy*);
	void dump_func(class Outy*,SymFuncEntry* f);
	void dump_block_hdr(class Outy*);
	void dump_block(class Outy*,SymFuncEntry* f);
	void dump_line_hdr(class Outy*);
	void dump_line(class Outy*,SymLineEntry* l,SymModEntry* m);
	void dump_lines(class Outy*);
	void dump_sym_hdr(class Outy*);
	void dump_sym(class Outy*,SymEntry* s);
	void dump_symbols(class Outy*);
	void dump_mod_hdr(class Outy*);
	void dump_mod(class Outy*,SymModEntry* m);
	void dump_modules(class Outy*);
	void dump_mod_syms(Outy* out,SymModEntry* m);
	void dump_mod_funcs(Outy* out,SymModEntry* m);
	void dump_func_syms_n_blocks(class Outy*,SymFuncEntry* f);
	void dump_debug(class Outy*);
	GetOpts* _dump_opts;
protected:
	SymNet* _symnet;
	FILE* _dump_fp;
    };
    

#endif /* __SYMDUMP_H__ */


