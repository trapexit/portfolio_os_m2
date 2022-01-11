/*  @(#) linkopts.h 96/07/25 1.16 */
#ifndef __LINKOPTS_H__
#define __LINKOPTS_H__



#include "option.h"

//==========================================================================
//options for linking
enum LinkOpts {
        linkopt_none,
        linkopt_help,		//?
        linkopt_map,		//m[2|fname]
        linkopt_base,		//B[d=n|t=n|i=n] 
        linkopt_secbase,	//bsname=val
        linkopt_secalign,	//Aval
        linkopt_def,		//xdefname
        linkopt_entry,		//e sym
        linkopt_library,	//lx
        linkopt_path,		//Lpath
        linkopt_strip,		//s[s]
        linkopt_stripmore,	//d
        linkopt_verbose,	//v
        linkopt_debug,		//g
        linkopt_debug_file,	//G
        linkopt_keep,		//k
        linkopt_relative,	//r
        linkopt_incremental,//i
        linkopt_standard,	//n
        linkopt_allow_dups,	//D
        linkopt_allow_unds,	//U
        linkopt_compress,	//C jrm 96/04/03
		linkopt_noload,		//N
        linkopt_out_file,	//o fname
    	linkopt_allow_reimport, //a             
	//not supported - supplied for compatibility
    	linkopt_fill,  		//f filler       
    	linkopt_magic, 		//Mmagicno      
    	linkopt_version, 	//V[S value]    
    	linkopt_skipsyms,	//X
    	linkopt_common,		//t
    	linkopt_dirlist,	//Y[L,dir|P,dir|U,dir]
	//fixup3do opts
		//fixup3do [-debug] [-v] [-h] [-i codeAddr dataAddr] [-o outname] file
		//support already provided in linker:
			//-debug: generate external .sym files\n"
			//-v: verbose
			//-h: help
			//-i: generate image file at the specified code and data addresses
			//-o: specifies the name of the output file, defaults to <file>.koff\n");
		//3DO Header options:
		linkopt_header3do,       //Hfield val
	//for defaults
        linkopt_generic,
		linkopt_max_args 
        };
        
#ifdef DEFINE_OPTS
Opts linkopts[linkopt_max_args+1]= {
    	{ linkopt_none, 	 ' ',0,0 },
    	{ linkopt_help, 	 '?',0,"           This help\n"
		"@argfile           Read arguments from \"argfile\"\n"
		},
    	{ linkopt_base,      'B',1,"[d=data_base,t=text_base,i=image_base]  Set [data&bss|text|image] base\n"},
    	{ linkopt_entry,	 'e',1,"symbol     Use \"symbol\" as entry point\n"},
    	{ linkopt_library, 	 'l',1,"lname      Use library lib\"lname\".a\n"},	
    	{ linkopt_map, 		 'm',0,"[2|fname]  Generate map file to stdout [fname]\n"},	
    	{ linkopt_out_file,  'o',1,"fname      Output file (default is a.out)\n"},
    	{ linkopt_relative,  'r',0,"           Generate relocations in file to keep file relative\n"},	
    	{ linkopt_incremental,'i',0,"           Incremental link\n"},	
    	{ linkopt_strip, 	 's',0,"[s]        Strip unnecessary stuff from file \n"\
    	                     "                 [.comment too]\n"},
    	{ linkopt_stripmore, 'd',0,"           Strip even more stuff from file (implies -s)\n"\
    	                     "                 (strips .symtab and .strtab)\n"\
    	                     "                 Warning: -d output cannot be used as linker input\n"},
    	{ linkopt_path, 	 'L',1,"path       Add library search path\n"},	
    	{ linkopt_secbase, 	 'b',1,"[secname]=base   Set section base\n"},
    	{ linkopt_secalign,  'A',1,"alignment Set section alignment\n"},
    	{ linkopt_def,		 'x',1,"def_file   Use definitions file for resolving imports/exports\n"},
    	{ linkopt_verbose, 	 'v',0,"           Be verbose\n"},
    	{ linkopt_debug, 	 'g',0,"           Generate debug info in file\n"},
    	{ linkopt_debug_file,'G',0,"           Generate debug info to external .sym file\n"},
    	{ linkopt_keep, 	 'k',0,"           Keep everything in file\n"},
    	{ linkopt_standard,  'n',0,"           Generate standard Elf file\n"},	
    	{ linkopt_allow_dups,'D',0,"           Allow duplicate symbols\n"},	
    	{ linkopt_allow_unds,'U',0,"           Allow undefined symbols\n"},	
        { linkopt_compress,  'C',0,"           Compress loadable sections\n"}, // jrm 96/04/03
    	{ linkopt_noload    ,'N',0,"           Do not autoload DLLs\n"},
    	{ linkopt_allow_reimport, 'R', 0,"           Allow all DLLs to be reloaded\n"},	
		//3DO Header options :
    	{ linkopt_header3do, 'H',1,"field=val  Set the field in the 3do header\n"
		"                field names:\n"
		"                	-Hname=<n>      : sets the app's name\n"
#ifndef USER_PRIVS
		"	                -Hpri=<n>        : sets the app's priority\n"
#endif
		"	                -Hversion=<n>    : sets the app's version\n"
		"                	-Hrevision=<n>   : sets the app's revision\n"
	    "                	-Htype=<n>       : sets the app's type\n"
	    "                	-Hsubsys=<n>     : sets the app's subsystype\n"                
        "                	-Htime=<date|now>: sets the app's time ('MM/DD/YY HH:MM:SS' or 'now'\n"
		"                	-Hosversion=<n>  : sets the app's osversion\n"
		"                	-Hosrevision=<n> : sets the app's osrevision\n"
		"                	-Hfreespace=<n>  : sets the app's freespace\n"
		"                	-Hmaxusecs=<n>   : sets the app's maxusecs\n"
		"                	-Hflags=<n>      : sets the app's flags\n"
		},
	//new additions
    	{ linkopt_version, 	 'V',0,"           Prints the version of the linker.\n"},	
    	{ linkopt_magic, 	 'M',1,"magicno    Use \"magicno\" as the magic number.\n"},	

//removed from autodoc output to prevent confusion 
//(but parse them anyway for Diab compatibility)
#if 1
    	{ linkopt_fill,  	 'f',1,0},
    	//was:{ linkopt_version, 	 'V',0,0},	//modified and moved above	
    	{ linkopt_skipsyms,  'X',0,0},	
    	{ linkopt_common, 	 't',0,0},
    	{ linkopt_dirlist, 	 'Y',1,0},
	//for defaults
    	{ linkopt_generic, 	 ' ',0,0}, 	
#else
	//not supported - supplied for compatibility
    	{ linkopt_fill,  	 'f',1,"filler     *Fill holes in output section with \"filler\".\n"},
    	//was:{ linkopt_version, 	 'V',0,"[S value]  *Prints the version of the linker (default) [stores value as the version stamp in the optionnal header].\n"},	
    	{ linkopt_skipsyms,  'X',0,"           *Skips symbols starting with @L and .L in symbol table.\n"},	
    	{ linkopt_common, 	 't',0,"           *Don't warn about common blocks with different sizes.\n"},	
    	{ linkopt_dirlist, 	 'Y',1,"[L,dir|P,dir|U,dir] *Set [first|list|second] directories for searching for libraries.\n"},	
	//for defaults
    	{ linkopt_generic, 	 ' ',0,"* - option not supported but supplied for compatibility.\n"}, 	
#endif

    	{ END_OPTS, 		 ' ',0,0}
    	};
#endif /* DEFINE_OPTS */

#endif /* __LINKOPTS_H__ */
