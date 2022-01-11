/*  @(#) link.cpp 96/07/25 1.39 */

#ifndef USE_DUMP_FILE
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "option.h"
#include "utils.h"
#endif /* USE_DUMP_FILE */

#include <signal.h>

#if defined(macintosh)
#if !defined(_MW_APP_)
#endif
#if defined(_MW_APP_) && defined(__TEST__)
#include <profiler.h>
#endif
#endif
#define DEFINE_OPTS
#include "linkopts.h"
#include "linkver.h"
#include "debug.h"

#pragma segment link

//#define _MW_APP_  //for running linker as an app
#define __ELF__
//==========================================================================
// defines & prototypes

#define LINK_FILE "a.out"
//apps have some trouble with stderr???
#if defined(macintosh) && defined(_MW_APP_)
	#define USERMSG_FILE "link.out"
#else
	#define USERMSG_FILE "stderr"
#endif

#define __STR(x) #x
#define _STR(x) __STR(x)	//force evaluation

#ifdef macintosh
	#ifdef _MW_APP_
		#include <console.h>	//for int ccommand(char ***);
	#endif
#endif
#if defined(__ELF__)
	#include "elf_l.h"
	#include "elf_d.h"
	#define CLASS ElfLinker
	#define MAPCLASS ElfDumper
	#define TYPE "Elf"
#elif defined(__SYM__)
	#include "sym_l.h"
	#include "sym_d.h"
	#define CLASS SymLinker
	#define MAPCLASS SymDumper
	#define TYPE "Sym"
#elif defined(__XCOFF__)
	#include "xcoff_l.h"
	#include "xcoff_d.h"
	#define CLASS XcoffLinker
	#define MAPCLASS XcoffDumper
	#define TYPE "Xcoff"
#elif defined(__XSYM__)
	#include "xsym_l_v3r3.h"	//assuming __XSYM_VER__ = __XSYM_V33__
	#include "xsym_d_v3r3.h"
	#define CLASS XsymLinker_v33
	#define MAPCLASS XsymDumper_v33
	#define TYPE "Xsym"
#endif

static char* link_file=0;
static FILE* usermsg_fp=stdout;	//in case we want to redirect user messages
static FILE* link_fp=0;
static Boolean parse_opts(GetOpts* o);
static Boolean link_ok(CLASS* dumper);
static Boolean link(GetOpts* o,CLASS* linker);
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
extern "C"
#endif

//-------------------------------------------
#if defined(macintosh)
void signal_handler(int)
#else
void signal_handler(int sig, int code, sigcontext *scp, char* addr)
#endif
//-------------------------------------------
{
	DBG_ERR(("### Signal Handler: an exception occurred!\n"));
#if defined(_SUN4)
	DBG_ERR(("### sig=%d\n### code=%d\n", sig, code));
#endif
	if (link_fp) fclose(link_fp);
	if (link_file && exists(link_file)) remove(link_file);
	DBG_ERR(("### Link aborted; no file generated\n"));
}

//==========================================================================
// linker - main program

//-------------------------------------------
int main(int argc, char **argv)
//-------------------------------------------
{
	//usermsg & link file may both go to a file, 
	//or usermsg may go to stdout while link goes to file...
#if defined(_SUN4) || defined(_MSDOS) //what about Mac???
	register_xcpt_handler(signal_handler);	//catch signals
#elif defined(macintosh) && !defined(_MW_APP_)
	signal(SIGINT, signal_handler);
#endif
#if defined(_MW_APP_) && defined (__PROFILE__)
	if (ProfilerInit(collectDetailed, bestTimeBase, 600, 200) != NULL)
		return 0;
#endif
#ifdef __TEST__
	set_test_args(argc,argv);
#elif defined(_MW_APP_)
	argc=ccommand(&argv);	//get Mac command args
#endif
	expand_at_files(&argc,&argv);

	FILE* try_fp=0;
    char *input=0;
	
    init();   
    progname = (argv ? argv[0] : "Link3DO");

    if (argc <= 1)
    { 
    	fprintf(usermsg_fp,"%s version %s\n", progname, LINK3DO_VER);
		GetOpts::usage(linkopts,progname,usermsg_fp);
    	return term(0); 
    }
    	
    // iterate thru options 
    GetOpts* o = new GetOpts(argc,argv,linkopts);
    if (o)
    {
    	if (parse_opts(o))
    	{
    		Boolean linked_ok=false;
			if (argc <= 2) o->set(linkopt_generic);
			link_file=o->arg(linkopt_out_file);			// open output file (if not stdout)
			if (!link_file)
			{
				fprintf(usermsg_fp,"no filename specified for elf file\n");
				return term(1);
			}
			link_fp=openb_(link_file);			// open output file (if not stdout)
			if (!link_fp)
			{
				fprintf(usermsg_fp,"# Unable to open elf file %s\n",link_file);
				return term(1);
			}
			CLASS* linker = new CLASS(link_fp, usermsg_fp,o);
    		if (linker)
    		{
				linked_ok = link(o,linker);
				delete linker;	
			}
			close_(link_fp);
    		if (!linked_ok) 
    			return term(1);
		}
		delete o; 
	}
#if defined(_MW_APP_) && defined(__PROFILE__)
	ProfilerDump("\pLink3do.profile");
#endif
	return term(0);
} // main()

//==========================================================================
// parse user options & set up defaults

//-------------------------------------------
Boolean parse_opts(GetOpts* o)
//-------------------------------------------
{
	DBG_ENT("parse_opts");
    int opt;
	char* mapfile=0;
    char *output=0;
	
	if (o->isset(linkopt_verbose) || o->isset(linkopt_version)) 
    	fprintf(usermsg_fp,"%s version %s\n", progname, LINK3DO_VER);
	if (o->isset(linkopt_help))
	{
		GetOpts::usage(linkopts,progname,usermsg_fp);
        return false;
    }
	if (o->isset(linkopt_out_file) && o->arg(linkopt_out_file)==0)
	{ 
		fprintf(usermsg_fp,"error - no output file specified on \"o\" option\n");
		return false;
	}
    // iterate thru options to turn on what to link
    for (opt=o->firstopt(); opt; opt = o->nextopt())
    {
        switch (opt)
        {
			case UNKNOWN_OPT:
                fprintf(usermsg_fp,"### Unknown option: -%c\n", o->cmdid()); 
				GetOpts::usage(linkopts,progname,usermsg_fp);
                return false;
        }
    }
	if (!o->isset(USER_OPT))
	{
		fprintf(usermsg_fp,"### No object files specified\n");
		return false;
	}
	if (!o->isset(linkopt_out_file)) 
    	o->set(linkopt_out_file,LINK_FILE);	//use default output file
	if (o->isset(linkopt_debug) && o->isset(linkopt_stripmore))
	{
        fprintf(usermsg_fp,"### Warning: -g and -d options are incompatible.  Ignoring -d option.\n"); 
    	o->clear(linkopt_stripmore);
    }
	if (o->isset(linkopt_stripmore) && !o->isset(linkopt_strip)) 
    	o->set(linkopt_strip);	//-d implies -s
	//set defaults
	if (o->isset(linkopt_generic)) {
        //if (!o->isset(linkopt_map)) o->set(linkopt_map);
        if (!o->isset(linkopt_base)) o->set(linkopt_base,"i=0x1000");
        //if (!o->isset(linkopt_sec_base)) o->set(linkopt_sec_base,".debug=0x1000");
        //if (!o->isset(linkopt_base)) o->set(linkopt_base,"t=0x1000");
        //if (!o->isset(linkopt_base)) o->set(linkopt_base,"d=512");
        //if (!o->isset(linkopt_library)) o->set(linkopt_library);
        //if (!o->isset(linkopt_path)) o->set(linkopt_path);
        //if (!o->isset(linkopt_verbose)) o->set(linkopt_verbose);
        //if (!o->isset(linkopt_debug)) o->set(linkopt_debug);
        if (!o->isset(linkopt_strip)) o->set(linkopt_strip);
        //if (!o->isset(linkopt_keep)) o->set(linkopt_keep);
        if (!o->isset(linkopt_relative)) o->set(linkopt_relative);
        }
	return true;
} // parse_opts()
	
//==========================================================================
// call linker and check results

Boolean link_ok(CLASS* linker)
{
	if (linker->Error()!=se_success) {
		if (!linker->Valid()) {
			fprintf(usermsg_fp,"# Error: %s",linker->GetErrStr());
			fprintf(usermsg_fp,"\t- unable to proceed with link\n");
			return false;
			}
		else {
			//fprintf(usermsg_fp,"Warning: %s",linker->GetErrStr());
			fprintf(usermsg_fp,"Warnings occurred\n");
			}
		}
	return true;
} // link_ok
	
//-------------------------------------------
Boolean link(GetOpts* o,CLASS* linker)
//-------------------------------------------
{
	DBG_ENT("link");
	if (!link_ok(linker)) return false;
    int opt;
    for (opt=o->firstopt(); opt; opt = o->nextopt())
    {
        switch (opt)
        {
            case 'l':
            {   	//add library
    				char* lib = o->cmdarg();
					if (!lib) {
						o->nextopt();	//try next arg
    					lib = o->cmdarg();
						}
					if (!lib) 
						fprintf(usermsg_fp,"# Error: no library specified for option l\n");
					else
					{
    					linker->AddLib(lib);
    				}
			}
            break;
           	case USER_OPT:
           	{
    				char* obj = o->cmdarg();
					if (!obj) 
						fprintf(usermsg_fp,"# Error: missign object name in command line\n");
					else
					{
    					linker->AddObj(obj);
    				}
		    }
    		break;
        } // switch
	} // for	
	if (!link_ok(linker)) return false;
	linker->LinkFiles(o->arg(linkopt_out_file));
	if (!link_ok(linker)) return false;
	return true;
} // link()
	    					
//==========================================================================
// generate map file

//==========================================================================
// misc utilities

//-------------------------------------------
static void init()
//-------------------------------------------
{
    //DBG_SET_LEVEL(ALLSYMS);
    DBG_SET_LEVEL(ALL);
    DBG_INIT("debug.out");
    usermsg_fp=open_(USERMSG_FILE);
}

//-------------------------------------------
static int term(int rc)
//-------------------------------------------
{
	CHECK_HEAP();
    close_(usermsg_fp);
    DBG_TERM();
	if (rc!=0)
	{
		if (link_file) remove(link_file);
	}
    return (rc);
}
	
#ifdef __TEST__
//-------------------------------------------
static void set_test_args(int& argc,char**& argv)
//-------------------------------------------
{
	const int test_argc  = 2;
	static char* test_argv[test_argc] = {
		"Link3DO",
		"@args"
		};
	argc = test_argc;
	argv = test_argv;
}
#endif

/**
|||	AUTODOC -public -class Dev_Commands -name link3do
|||	This command lets you build an M2 executable or DLL from object files,
|||	DLL modules, and libraries
|||
|||	  Synopsis
|||
|||	    link3do
|||	          [-B[d=data_base,t=text_base,i=image_base]]
|||	          [-e symbol] [-llname]
|||	          [-m[2|fname]] [-ofname] [-r] [-i] [-s[s]]
|||	          [-Lpath...] [-b[secname]=base] [-Aalignment]
|||	          [-xdef_file] [-v] [-g] [-G] [-k] [-n] [-D] [-U]
|||	          [-C] [-N] [-R] [-Hfield=val...] [-V] [-Mmagicno]
|||	          objfile... [modulefile...] [-llname...]
|||
|||	  Description
|||
|||	    Link3DO reads various input files (libraries, dllmodules,
|||	    and object files produced by the compiler or assembler),
|||	    resolves and relocates references between and within them,
|||	    and creates an m2 executable file or DLL module in 3DO ELF
|||	    format.
|||
|||	    objfile... are the object files. modulefule... are the DLL
|||	    module files to be linked.
|||
|||	  Arguments -preformatted
|||
|||
|||	    -B[d=data_base,t=text_base,i=image_base]
|||	        Set [data&bss|text|image] base
|||
|||	    -esymbol
|||	        Use "symbol" as entry point.  The special form '-e0'
|||	        allows a file to have no entry point.
|||
|||	    -llname
|||	        Use library lib"lname".a  The form -lfullpath is also
|||	        supported.  The latter form does not prefix "lib" or
|||	        append ".a"
|||
|||	    -m[2|fname]
|||	        Generate map file to stdout [fname]
|||
|||	    -ofname
|||	        Output file (default is a.out)
|||
|||	    -r
|||	        Generate relocations in file to keep file relative
|||
|||	    -i
|||	        Incremental link
|||
|||	    -s[s]
|||	        Strip unnecessary stuff from file
|||	        [ including .comment]
|||
|||	    -d
|||	        Strip even more stuff from file, specifically the symbol name strings.
|||	        i.e. .strtab, .symtab
|||	        WARNING: -d option produces files that cannot be used as linker input.
|||	        WARNING: This option is ignored if the -g flag is also set.
|||
|||	    -Lpath
|||	        Add this directory path to the list of paths searched for libraries.
|||	        This affects libraries used with the -llname option.
|||
|||	    -b[secname]=base
|||	        Set section base
|||
|||	    -Aalignment
|||	        Set section alignment
|||
|||	    -xdef_file
|||	        Use definitions file for resolving imports/exports
|||
|||	    -v
|||	        Be verbose
|||
|||	    -g
|||	        Generate debug info in file
|||
|||	    -G
|||	        Generate debug info to external .sym file
|||
|||	    -k
|||	        Keep everything in file
|||
|||	    -n
|||	        Generate standard Elf file
|||
|||	    -D
|||	        Allow duplicate symbols
|||
|||	    -U
|||	        Allow undefined symbols
|||
|||	    -Hfield=val
|||	        Set the field in the 3do header
|||	        Field names:
|||	            -Hname=<string>  : sets the app name.
|||	            -Hpri=<n>        : sets the app priority
|||	            -Hversion=<n>    : sets the app version
|||	            -Hrevision=<n>   : sets the app revision
|||	            -Htype=<n>       : sets the app type
|||	            -Hsubsys=<n>     : sets the app subsystype
|||	            -Htime=<date|now>: sets the app time ('MM/DD/YY HH:MM:SS' or 'now')
|||	            -Hosversion=<n>  : sets the app osversion
|||	            -Hosrevision=<n> : sets the app osrevision
|||	            -Hfreespace=<n>  : sets the app freespace
|||	            -Hmaxusecs=<n>   : sets the app maxusecs
|||	            -Hflags=<n>      : sets the app flags
|||
|||	    -C
|||	        Compress .text, .data, .rela.text, and .rela.data sections
|||	        WARNING: As of this writing, the OS loader cannot load such sections.
|||
|||	    -N
|||	        Do not autoload DLLs
|||
|||	    -R
|||	        Allow all DLLs to be reloaded
|||
|||	    -V
|||	        Prints the version of the linker.
|||
|||	    -Mmagicno
|||	        Use "magicno" as the magic number.
|||
|||
**/
/**
|||	AUTODOC -internal -class Dev_Commands -name ELF_compression
|||	This note describes the changes to the ELF file format that were
|||	introduced to support compression in Link3DO.
|||
|||	  Synopsis
|||	    A new option -C has been added to Link3DO.  With this option,
|||	    sections that the loader loads (.text and .data), as well as the
|||	    sections that it reads (relocations) are compressed using the
|||	    compression folio compression algorithm.
|||
|||	  Description
|||
|||	    In general, a single program header describes either a single
|||	    .text section, or a consecutive run of two sections, a .data
|||	    and a .bss section.  (The .bss section is of type SHT_NOBITS, which
|||	    means that uninitialized memory is to be allocated.)  The -C
|||	    option will cause all sections that are destined to be
|||	    loaded to be compressed.  Usually, this means .text
|||	    and .data sections.
|||
|||	    If a loadable (or relocation) section has been compressed, the
|||	    following will be true of the ELF file:
|||
|||	    (1) a newly allocated bit in the Elf32_Phdr.p_flags field
|||	    of its program header will be set:
|||
|||	        #define PF_C                        0x10  
|||
|||	    (2) the Elf32_Phdr.p_filesz field of its program header will
|||	    continue to refer to the size on disk, i.e., the compressed size.
|||
|||	    (3) the Elf32_Phdr.p_memsz field of its program header will be
|||	    guaranteed to be at least as great as the expanded size of the
|||	    section.  The loader therefore may safely allocate p_memsz bytes
|||	    before decompressing the data.  In accordance with the ELF
|||	    specification, any excess bytes must be null-filled.
|||
|||	    (4) a newly allocated bit in the Elf32_Shdr.sh_flags field
|||	    of its corresponding section header will be set:
|||
|||	        #define SHF_COMPRESS        0x8
|||
|||	    (5) again, the Elf32_Shdr.sh_size field of its section header will
|||	    refer to the size on disk, i.e., the compressed size.  In the case of
|||	    .text and .data, the memory size is encoded in the corresponding
|||	    program header entry.
|||
|||
**/
