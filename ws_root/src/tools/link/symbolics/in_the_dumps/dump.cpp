/*  @(#) dump.cpp 96/09/09 1.27 */

/*
	File:		dump.cpp

	Written by:	Dawn Perchik

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by The 3DO Company.

	Change History (most recent first):

		<8+>	96/04/10	JRM		More cleaning up
		 <3>	96/01/17	JRM		Merge
		 <2>	96/01/08	JRM		Merged from dawn's multiple copies.
				96/01/08	JRM		Header first added

	To Do:
*/

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "option.h"
#include "utils.h"
#define DEFINE_OPTS
#include "dumpopts.h"
#include "symdump.h"
#include "elf_d.h"
#include "ar_d.h"
#ifndef ELF_ONLY
	//#include "sym_d.h"
	//#include "xcoff_d.h"
	//#include "xsym_d_v3r3.h"	//assuming __XSYM_VER__ = __XSYM_V33__
#endif
#if defined(macintosh)
	#if defined(_MW_APP_)
		#include <console.h>	//for int ccommand(char ***);
	#else
		#include <signal.h>
	#endif
#endif
#include "debug.h"

#pragma segment dump

//#define _MW_APP_  //for running dumper as an app
//==========================================================================
// defines & prototypes

#include "dumpver.h"
//apps have some trouble with stderr???
#if defined(macintosh) && defined(_MW_APP_)
	#define DUMP_FILE "dump.out"
	#define USERMSG_FILE "dump.err"
#else
	#define DUMP_FILE "stdout"
	#define USERMSG_FILE "stderr"
#endif

#define __STR(x) #x
#define _STR(x) __STR(x)	//force evaluation

static FILE* dump_fp=stdout;	//in case we want to redirect user messages
static FILE* usermsg_fp=stdout;	//in case we want to redirect user messages
static Boolean parse_opts(GetOpts* o);
static Boolean dump_ok(SymDump* dumper);
static void dump(GetOpts* o, SymDump* dumper);
static char *progname;
static void init();
static int term(int rc);
#if defined(macintosh)
	extern "C" void signal_handler(int);
#else
	void signal_handler(int sig, int code, sigcontext *scp, char* addr);
#endif
#ifdef __TEST__
	static void set_test_args(int& argc,char**& argv);
#endif
#if defined(macintosh)
extern "C" void signal_handler(int) {
	DBG_ERR(("### Signal Handler: an exception occurred!\n"));
#else
void signal_handler(int sig, int code, sigcontext *scp, char* addr)
{
	DBG_ERR(("### An exception occurred!\n"));
	printf("sig=%d\n, code=%d\n, addr = %s\n", sig, code, addr);
#endif
	if (dump_fp) fclose(dump_fp);
	fprintf(usermsg_fp,"### Link aborted; no file generated\n");
	}

//==========================================================================
// dumper - main program

#ifdef _MW_APP_
int main() {
    char **argv;
    int argc=ccommand(&argv);	//get Mac command args
#else
int main(int argc, char **argv) {
#endif
	//usermsg & dump file may both go to a file, 
	//to stdout, or usermsg may go to stdout while dump goes to file...
	#if defined(_SUN4) || defined(_MSDOS) //what about Mac???
		register_xcpt_handler(signal_handler);	//catch signals
	#elif defined(macintosh) && !defined(_MW_APP_)
		signal(SIGINT, signal_handler);
	#endif
	#ifdef __TEST__
		set_test_args(argc,argv);
	#endif
	expand_at_files(&argc,&argv);
	
    int dumpkind;

    init();   
    progname = (argc ? argv[0] : "Dump3DO");

    if (argc <= 1) { 
		fprintf(usermsg_fp,"%s version %s\n", progname, DUMP3DO_VER);
		GetOpts::usage(dumpopts,progname,usermsg_fp);
    	return term(0); 
    	}

    // iterate thru options to turn on what to dump
    GetOpts* o = new GetOpts(argc,argv,dumpopts);
	if (o) {
		if (parse_opts(o)) {
			int nerrors = 0;
			dump_fp=open_(o->arg(dumpopt_out_file));	// open output file (if not stdout)
			for (int j=0; j<o->nargs(USER_OPT); j++) {	//loop thru each file to dump
				char* dfile = o->arg(USER_OPT,j);
				SymFileFormat type = SymNet::GetSymFF(dfile);
    			fprintf(dump_fp,"\n\nDump of %s file %s:\n", SymDump::GetSymFFStr(type), dfile);
    
   				if (o->isset(dumpopt_symnet))
    				dumpkind = DUMP_ALL;
    			else if (!o->isset(dumpopt_symtab) && !o->isset(dumpopt_debug_info))
    				dumpkind = DUMP_HEADERS_ONLY;
   				else 
    				dumpkind = DUMP_FILE_ONLY;

				SymDump* dumper = NULL;
				switch (type) {
					case eAR:
						dumper = new ArDumper(dfile,dump_fp,o,dumpkind);
						break;
					case eELF:
						dumper = new ElfDumper(dfile,dump_fp,o,dumpkind);
						break;
					case eARM:
						//dumper = new SymDumper(dfile,dump_fp,o,dumpkind);
						fprintf(usermsg_fp,"Support for Arm has been dropped for this release\n");
    					return term(1);
					case eXCOFF:
						//dumper = new XcoffDumper(dfile,dump_fp,o,dumpkind);
						fprintf(usermsg_fp,"Support for xcoff has been dropped for this release\n");
    					return term(1);
					case eXSYM:
						//dumper = new XsymDumper(dfile,dump_fp,o,dumpkind);
						fprintf(usermsg_fp,"Support for XSym has been dropped for this release\n");
    					return term(1);
					default:
						fprintf(usermsg_fp,"file is of an unknown format\n");
    					return term(1);
					}
				if (!dumper || !dumper->GetState()->valid()) {
					fprintf(usermsg_fp,"dump failed - unable to proceed\n");
					nerrors++;
					}
				else
					dump(o,dumper);
				delete dumper; 	
				}
			if (nerrors) {
				fprintf(usermsg_fp,"%d Errors found\n",nerrors);
				return term(1);
				}
			if (o->isset(dumpopt_out_file))
				close_(dump_fp);
			}
		delete o; 
		}
    return term(0);
    }

//==========================================================================
// parse user options & set up defaults

Boolean parse_opts(GetOpts* o) {
	DBG_ENT("parse_opts");
    //int opt;
    unsigned char opt;
	if (o->isset(dumpopt_version)) 
		fprintf(usermsg_fp,"%s version %s\n", progname, DUMP3DO_VER);
	if (o->isset(dumpopt_help)) {
		GetOpts::usage(dumpopts,progname,usermsg_fp);
        return false;
        }
    for (opt=o->firstopt(); opt; opt = o->nextopt()) {
        switch (opt) {
            case UNKNOWN_OPT:
                fprintf(usermsg_fp,"Unknown option: -%c\n", o->cmdid()); 
				GetOpts::usage(dumpopts,progname,usermsg_fp);
                return(false);
            }
        }
        
	if (o->isset(dumpopt_out_file)) {
		if (!o->arg(dumpopt_out_file)) {
			fprintf(usermsg_fp,"error - no output file specified on \"o\" option\n");
			return(false);
			}
		}
	else
		o->set(dumpopt_out_file,DUMP_FILE);

	if (!o->isset(USER_OPT)) {
		fprintf(usermsg_fp,"error - no input file specified\n");
		return(false);
		}
	DBG_ASSERT(o->arg(USER_OPT));

	//set defaults
	//Added the outfile with defaults option,which had been missing mmh 96/08/16
	//was: if ((o->ncmdopts() <= 2) || (o->isset(dumpopt_generic))) {
	Boolean defaultOutfile = false;
	if ( (o->totalArgs() == 4) && (o->isset(dumpopt_out_file)))
				defaultOutfile = true;
	if ( (o->ncmdopts() - o->nargs(USER_OPT) <= 0) || (o->isset(dumpopt_generic)) || (defaultOutfile))
	   	{
	   	o->set(dumpopt_header);
        o->set(dumpopt_program_headers);
        o->set(dumpopt_section_headers);
        o->set(dumpopt_symtab);
        o->set(dumpopt_relocations);
        o->set(dumpopt_dynamic_data);
        //o->set(dumpopt_hash);
        //o->set(dumpopt_debug_info);
        //o->set(dumpopt_lines);
        //o->set(dumpopt_content);	
        //o->set(dumpopt_symnet);
        }
    return true;
    }
    
//==========================================================================
// call dumper and check results

Boolean dump_ok(SymDump* dumper) {
	if (dumper->GetState()->state()!=se_success)
		fprintf(usermsg_fp,"# %s\n",dumper->GetState()->get_err_msg());
	if (!dumper->GetState()->valid()) {
		fprintf(usermsg_fp,"# Fatal error - unable to proceed with dump\n");
		return false;
		}
	return true;
	}
	
void dump(GetOpts* o, SymDump* dumper) {
	if (!dump_ok(dumper)) return;
	dumper->DumpFile();	// dump specific stuff to file
	if (!dump_ok(dumper)) return;
    if (o->isset(dumpopt_symnet))
		dumper->Dump(dump_fp); 		// dump symbols in network
	if (!dump_ok(dumper)) return;
	}
	
//==========================================================================
// misc utilities

static void init() {
    DBG_SET_LEVEL(ALL);
    DBG_INIT("debug.out");
    usermsg_fp=open_(USERMSG_FILE);
    }
static int term(int rc) {
	CHECK_HEAP();
    close_(usermsg_fp);
    DBG_TERM();
    return (rc);
	}
	
#ifdef __TEST__
static void set_test_args(int& argc,char**& argv) {
		const int test_argc  = 2;
		static char* test_argv[test_argc] = {
			"Dump3DO",
			"@dargs",
			};
		argc = test_argc;
		argv = test_argv;
		}
#endif

/**
|||	AUTODOC -public -class Dev_Commands -name dump3do
|||	This command lets you examine the contents of 3DO executables and object
|||	modules
|||
|||	  Synopsis
|||
|||	    dump3do
|||	          [-ofname]
|||	          [-?] [-V] [-h] [-p] [-s] [-t] [-r]
|||	          [-d] [-a] [-g] [-c] [-u]
|||	          [-l] [-i] [-b] [-m] objfile...
|||
|||	  Description
|||
|||	    dump3do prints information about one or more object files.
|||	    The options control what particular information to display.
|||
|||	    objfile... are the object files to be examined. When you
|||	    specify archives, objdump shows information on each of the
|||	    member object files.
|||
|||	  Arguments
|||
|||	    -m
|||	        If any files from objfile are archives, display the archive
|||	        header information.
|||
|||	    -u
|||	        Disassemble text section.  Display the assembler mnemonics
|||	        for the machine instructions from objfile. This option only
|||	        disassembles those sections (.text sections)
|||	        which are expected to contain instructions.
|||
|||	    -h
|||	        File header.
|||	        Display summary information from the overall header of
|||	        each file in objfile.
|||
|||	    -s
|||	        Section headers.
|||	        Display summary information from the section headers of
|||	        the object file.
|||
|||	    -p
|||	        Program headers.
|||	        Display summary information from the program headers of
|||	        the object file.
|||
|||	    -?  Help
|||	        Print a summary of the options to this program and exit.
|||
|||	    -l
|||	        Label the display (using debugging information) with
|||	        the filename and source line numbers corresponding to
|||	        the object code shown. Only useful with -u.
|||
|||	    -r
|||	        Relocation Entries.  Print the relocation entries of the file.
|||
|||	    -d
|||	        Dynamic table.  Print the dynamic relocation entries of the file. This
|||	        is only meaningful for dynamic objects, such as certain
|||	        types of shared libraries.
|||
|||	    -c
|||	        Section contents.  Display the full contents of any sections.
|||
|||	    -i
|||	        Interpret symbolic info
|||
|||	    -ofname
|||	        Redirects output to output file
|||
|||	    -t
|||	        Symbol Table. Print the symbol table entries of the
|||	        file.
|||
|||	    -V
|||	        Print the version number of this program and exit.
|||
|||	    -a
|||	        Hash table contents.
|||
|||	    -g
|||	        Debug info
|||
|||	    -b
|||	        Binary dump
|||
|||	  Copying
|||
|||	    The 602 disassembly portion of Dump3DO is based on code which is
|||	    Copyright (c) 1991 Free Software Foundation, Inc.
|||
|||	    Permission is granted to make and distribute verbatim copies
|||	    of this manual provided the copyright notice and this permission
|||	    notice are preserved on all copies.
|||
|||	    Permission is granted to copy and distribute modified version
|||	    of this manual under the conditions for verbatim copying,
|||	    provided that the entire resulting derived work is distributed
|||	    under the terms of a permission notice identical to this one.
|||
|||	    Permission is granted to copy and distribute translations of
|||	    this manual into another language, under the above conditions
|||	    for modified versions, except that this permission notice may be
|||	    included in translations approved by the Free Software Foundation
|||	    instead of in the original English.
|||
**/
