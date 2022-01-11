// @(#) ar_d.h 96/09/10 1.3

//====================================================================
// ar_d.h  -  ArDumper class defs for dumping AR files 
//
//		Symbolics class hierarchy:
//
//			Symbolics	- contains main functions visable to world
//				SymNet	- builds internal network of symbols and queries them
//					SymReader - reads ARM sym files with sym debug info and
//								calls SymNet methods to add symbols to network
//						SymDumper - dumps ARM sym files 
//					XcoffReader - reads XCOFF files with dbx stabs debug info and
//								calls SymNet methods to add symbols to network
//						XcoffDumper - dumps XCOFF files 
//					ElfReader - reads ELF files with dwarf v1.1 debug info and
//								calls SymNet methods to add symbols to network
//						ElfDumper - dumps ELF files 

#ifndef __AR_D_H__
#define __AR_D_H__

#include <time.h>
#include "symdump.h"
#include "elf_ar.h"
#include "dumputils.h"
#include "dumpopts.h"

class ArDumper : public SymDump, public ArReader {
public:
    ArDumper(char *name, FILE* dump_fp,GetOpts*,int dump=DUMP_ALL);
    virtual ~ArDumper() {};
	virtual void DumpFile();	//file-specific dump
    void dump_hdr();
    void dump_members();
    void dump_symtab();
    void dump_strtab();
private:
    GetOpts* _dump_opts;
    };

#endif /* __AR_D_H__ */

