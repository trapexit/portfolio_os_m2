//  @(#) symnet.h 96/07/25 1.14

//====================================================================
// symnet.h  -  SymNet class and structs for building and searching 
//				symbol network
//
//		Symbolics class hierarchy:
//
//			Symbolics	- contains main functions visable to world
//				SymNet	- builds internal network of symbols and queries them
//					SymReader - reads ARM sym files with sym debug info and
//								calls SymNet methods to add symbols to network
//					XcoffReader - reads XCOFF files with dbx stabs debug info and
//								calls SymNet methods to add symbols to network
//					ElfReader - reads ELF files with dwarf v1.1 debug info and
//								calls SymNet methods to add symbols to network


#ifndef __SYMNET_H__
#define __SYMNET_H__


//==========================================================================
//  defines

#define SYM_NEW(x) HEAP_NEW(global_heap,x) 
#define SYM_DELETE(x) HEAP_DELETE(global_heap,x) 
#define SYM_TMP_NEW(x) HEAP_NEW(global_tmp_heap,x) 
#define SYM_TMP_DELETE(x) HEAP_DELETE(global_tmp_heap,x) 
#define SYM_FREAD(off,size,n,fp) sym_fread(off,size,n,fp,__FILE__,__LINE__)
#define SYM_FSEEK(off,pos,fp) sym_fseek(off,pos,fp,__FILE__,__LINE__)

//==========================================================================
//  includes

#include <stdlib.h>

#include "predefines.h"
#include "symapi.h"
#include "symutils.h"
#include "symnodes.h"


#ifdef __3DO_DEBUGGER__	
#include "CExpr_Common.h"
#endif /* __3DO_DEBUGGER__ */

//====================================================================
// defines
#define NO_DUMP 0
#define DUMP_ALL 1
#define DUMP_FILE_ONLY 2
#define DUMP_HEADERS_ONLY 3

#ifdef __3DO_DEBUGGER__	
//HW reg names
#define REG_NAME_PC	"PC"
//HW regs
#define REG_PC	1
#define REG_2	2
#define REG_3	3
#define REG_4	4
#define REG_5	5
#define REG_6	6
#define REG_7	7
#define REG_8	8
#define REG_9	9
#define REG_14	14
#endif 

//==========================================================================
//  error handling macros

#define STR(x) #x
        
#define SET_STATE(err) { \
            set_state(err,__FILE__,__LINE__); \
            }
#define SET_ERR(err,str) { \
            set_state(err,__FILE__,__LINE__); \
            set_err_msg str; \
            DBG_ERR(("%s occurred. ", \
                 _state.get_err_name())); \
            DBG_OUT_(ERR,("\n")); \
            DBG_OUT_(ERR,str); \
            }
#define RETURN_ERR(err,str) { \
            set_state(err,__FILE__,__LINE__); \
            set_err_msg str; \
            DBG_ERR(("%s occurred. ", \
                 _state.get_err_name())); \
            DBG_OUT_(ERR,str); \
            DBG_OUT_(ERR,("\n")); \
            return state(); \
            }
//for errors which are informational (not fatal)
#define SET_INFO(err,str) { \
            set_state(err,__FILE__,__LINE__); \
            set_err_msg str; \
            DBG_WARN(("%s set... ", \
                 _state.get_err_name())); \
            DBG_OUT_(WARN,str); \
            DBG_OUT_(WARN,("\n")); \
            }
#define RETURN_INFO(err,str) { \
            set_state(err,__FILE__,__LINE__); \
            set_err_msg str; \
            DBG_WARN(("%s set... ", \
                 _state.get_err_name())); \
            DBG_OUT_(WARN,str); \
            DBG_OUT_(WARN,("\n")); \
            return state(); \
            }
//for indirect accesses
#define SET_ERRI(obj,err,str) { \
            obj->set_state(err,__FILE__,__LINE__); \
            obj->set_err_msg str; \
            DBG_ERR(("%s occurred. ", \
                 obj->get_err_name())); \
            DBG_OUT_(ERR,str); \
            DBG_OUT_(ERR,("\n")); \
            }
#define RETURN_ERRI(obj,err,str) { \
            obj->set_state(err,__FILE__,__LINE__); \
            obj->set_err_msg str; \
            DBG_ERR(("%s occurred. ", \
                 obj->get_err_name())); \
            DBG_OUT_(ERR,str); \
            DBG_OUT_(ERR,("\n")); \
            return obj->error; \
            }

#endif /* __SYMNET_H__ */


