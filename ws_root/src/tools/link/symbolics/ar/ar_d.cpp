/*  @(#) ar_d.cpp 96/09/09 1.4 */
//====================================================================
// ar_d.cpp  -  ArDumper class defs for dumping archive files


#ifndef USE_DUMP_FILE
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "utils.h"
#endif /* USE_DUMP_FILE */
#include "elf_ar.h"	//FIXME!! want to generalize this to ar_r.h
#include "ar_d.h"	

#include "debug.h"

#pragma segment ardump

//==========================================================================
// main funcs for dumper

#if defined(__NEW_ARMSYM__)
ArDumper::ArDumper(char *name, FILE* dump_fp, GetOpts* opts, int dump) 
		: ArReader(dump), SymDump((SymNet*)this,dump_fp,opts)
#else
ArDumper::ArDumper(char *name, FILE* dump_fp, GetOpts* opts, int dump) 
		: ArReader(name,dump), Outstrstuffs(dump_fp)
#endif
	{
#if defined(__NEW_ARMSYM__)
    char fn[255];
    pname((StringPtr)fn,(char*)name);
    ISymbolics((StringPtr)fn);
#endif
    if (_state.validate()!=se_success) {
    	DBG_ERR(("invalid symbolics object!\n"));
    	return;
     	}
    _dump_opts = opts;
    }

void ArDumper::DumpFile() {
    if (_state.validate()!=se_success) {
    	DBG_ERR(("invalid symbolics object!\n"));
    	return;
     	}
    if (_dump_opts->isset(dumpopt_none)) 
    	_dump_opts->set(dumpopt_generic);
    if (!_fp->open()) return;
    //outstr("Ar dump of file \"%s\":\n", _fp->filename());
    if (_dump_opts->isset(dumpopt_members)) { 
        dump_members();
		}
    if (_state.validate()!=se_success) {
    	DBG_ERR(("invalid symbolics object!\n"));
    	return;
     	}
    if (_ar_syms._syms && _dump_opts->isset(dumpopt_symtab)) {
        dump_symtab();
        }
    if (_fp) _fp->close();
    }
    
//==========================================================================
// dumpers

void ArDumper::dump_members() {
    outstr("\nAr Members\n");
    pad2(4);
    outstr("member ");
    pad2(4);
    outstr("name");
    pad2(28);
    outstr("hdr_off");
    pad2(36);
    outstr("size");
    pad2(44);
    outstr("obj_off\n");
    outstr("--------------------------------------------------\n");
	ar_hdr* m; int i;
	for (i=0, m=first(); m; i++, m=next()) {
    	pad2(4);
    	outstr("%d",i);
    	pad2(8);
    	outstr("%s",get_name());
		if (strlen(get_name())+1>(28-8)) 
			outstr("\n");
    	pad2(28);
    	outstr("x%x",_foff);
    	pad2(36);
    	outstr("x%x",get_size());
    	pad2(44);
    	outstr("x%x",_foff+sizeof(_ar_hdr));
		outstr("\n");
		}
	outstr("\n");
    }

void ArDumper::dump_symtab() {
	if (!_ar_syms._off) {
		outstr("\nNo symbol table found in archive file\n");
		return;
		}
    outstr("\nAr symbol table (at offset x%x):\n\n",_ar_syms._off);
    pad2(4);
    outstr("symbol name");
    pad2(32);
    outstr("file offset\n");
    pad2(4);
    outstr("---------------------------------------\n");
    Ar_sym* syms = _ar_syms._syms;
    int nsyms = _ar_syms._nsyms;

    for (int i=0; i<nsyms; i++) {
    	pad2(4);
        outstr("%d",i);
    	pad2(8);
        outstr("%s",syms[i]._name);
		if (strlen(syms[i]._name)+1>(35-8)) 
			outstr("\n");
    	pad2(35);
        outstr("x%x",syms[i]._off);
#ifdef OLDLIB
    	pad2(40);
        outstr("%d",syms[i]._objind);
#endif
		outstr("\n");
        }
	outstr("\n");
    }

