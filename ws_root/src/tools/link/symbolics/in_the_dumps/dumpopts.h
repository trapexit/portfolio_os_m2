/*  @(#) dumpopts.h 96/07/25 1.7 */
#ifndef __DUMPOPTS_H__
#define __DUMPOPTS_H__



#include "option.h"

//==========================================================================
//options for dumping
enum DumpOpts {
        dumpopt_none,
        dumpopt_out_file,
        dumpopt_help,
        dumpopt_version,
        dumpopt_header,
        dumpopt_program_headers,
        dumpopt_section_headers,
        dumpopt_symtab,
        dumpopt_relocations,
        dumpopt_dynamic_data,
        dumpopt_hash,
        dumpopt_debug_info,
        dumpopt_content,
		dumpopt_disasm,
        dumpopt_lines,
        dumpopt_symnet,
        dumpopt_binary,
        dumpopt_members,
        dumpopt_generic,
		dumpopt_max_args 
        };
      
#ifdef DEFINE_OPTS
Opts dumpopts[dumpopt_max_args+1]= {
    	{ dumpopt_none,				' ',0,0}, 	
    	{ dumpopt_out_file,			'o',1,"fname  Output file\n"},	
    	{ dumpopt_help,				'?',0,"  This help\n"},	
    	{ dumpopt_version,			'V',0,"  Version of this tool\n"},	
    	{ dumpopt_header,			'h',0,"  File header\n"},	
    	{ dumpopt_program_headers,	'p',0,"  Program headers\n"},	
    	{ dumpopt_section_headers,	's',0,"  Section headers\n"},
    	{ dumpopt_symtab,			't',0,"  Symbol table\n"},
    	{ dumpopt_relocations,		'r',0,"  Relocation Entries\n"},
    	{ dumpopt_dynamic_data,		'd',0,"  Dynamic table\n"},
    	{ dumpopt_hash,				'a',0,"  Hash table contents\n"},
    	{ dumpopt_debug_info,		'g',0,"  Debug info\n"},	
    	{ dumpopt_content,			'c',0,"  Section contents\n"},
    	{ dumpopt_disasm,			'u',0,"  Disassemble text section\n"},
    	{ dumpopt_lines,			'l',0,"  Line info\n"},
    	{ dumpopt_symnet,			'i',0,"  Interpret symbolic info\n"},
    	{ dumpopt_binary,			'b',0,"  Binary\n"},
    	{ dumpopt_members,			'm',0,"  If library, list members\n"},
    	{ dumpopt_generic,			' ',0,0}, 
    	{ END_OPTS, 				' ',0,0}
    	};
#else
extern Opts dumpopts[];
#endif /* DEFINE_OPTS */
        
#endif /* __DUMPOPTS_H__ */
