
//====================================================================
// xsym_r.h  -  XsymReader class defs for reading XSYM files v3.1 - v3.4
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
//					XsymReader - reads XSYM files and calls SymNet methods to add 
//								symbols to network
//						XsymDumper - dumps XSYM files 

//NOTE: since XsymReader_v32_ and XsymReader_ are practically the same,
//we include this file for XsymReader_ and put #ifdefs around code
//specific to XsymReader_

#include "xsym_verdefs.h"
#include "debug.h"

//figure out if this version of this file has already been included or not
#undef __ALREADY_INCLUDED__
#if __XSYM_VER__ == __XSYM_V31__
	#ifndef __XSYM_R_v31_H__
		#define __XSYM_R_v31_H__
		#include "xsym_defs_v31.h"
	#else
		#define __ALREADY_INCLUDED__
	#endif
#elif __XSYM_VER__ == __XSYM_V32__
	#ifndef __XSYM_R_v32_H__
		#define __XSYM_R_v32_H__
		#include "xsym_defs_v32.h"
	#else
		#define __ALREADY_INCLUDED__
	#endif
#elif __XSYM_VER__ == __XSYM_V34__
	#ifndef __XSYM_R_v34_H__
		#define __XSYM_R_v34_H__
		#include "xsym_defs_v34.h"
	#else
		#define __ALREADY_INCLUDED__
	#endif
#else 	//v33 default!!!
	#undef __XSYM_VER__ 
	#define __XSYM_VER__ __XSYM_V33__
	#ifndef __XSYM_R_v33_H__
		#define __XSYM_R_v33_H__
		#include "xsym_defs_v33.h"
	#else
		#define __ALREADY_INCLUDED__
	#endif
#endif 


#ifndef __ALREADY_INCLUDED__	

#ifndef USE_DUMP_FILE
#include "predefines.h"
#include "symnet.h"
#include "utils.h"
#endif

#define MAX_TYPE_SIZE 124	//max tte size

class XsymReader : public SymNet, protected Endian, public SymFwdrefTypes {

public: // contract member functions for ABC
	XsymReader(int dump=0);  
	virtual ~XsymReader();
		
	SymErr ISymbolics(const StringPtr symbolicsFile);
	#ifdef __3DO_DEBUGGER__	
		//SymErr XSYM_to_YACC(VariableInfo *v,YYSTYPE *result);
	#endif /* __3DO_DEBUGGER__	*/
	
protected: 
	_DISK_SYMBOL_HEADER_BLOCK _dshb;			// First page is read into this.
	
	typedef void (XsymReader::*read_func_t)(BufEaters*,long);	//typedef for symbol compares
	uint32 get_off(_DTI* p) { return p->dti_first_page * _dshb.dshb_page_size; }
	uint32 get_size(_DTI* p) { return p->dti_page_count * _dshb.dshb_page_size; }
	uint32 get_nobjs(_DTI* p) { return p->dti_object_count; }
	char* get_nte_name(long nte_index);
	uint32 get_mte_srcoff(uint32 mte_index);
	uint32 get_mte_start(uint32 mte_index);
	char* get_mte_name(uint32 mte_index);
	char* get_frte_mte_name(uint32 frte_index);
	SymClass get_symclass(SymSec sectype);
	SymSec get_symsec(SymClass sclass);
	uint32 get_symaddr(unsigned char* addr,uint32 size);
	uint32 get_tte_size(BufEaters* buf, uint32 psize);
	unsigned char* get_tte_bytes(BufEaters* buf, uint32 psize);

	// Reader functions 
	void read_xsym();
	void read_name_MTE(_MTEIndex);
	void read_FRTEs();
	void read_FRTE(BufEaters*,long);
	void read_name_FRTE(_FRTEIndex, unsigned long);
	void read_RTEs();
	void read_RTE(BufEaters*,long);
	void read_MTEs();
	void read_MTE(BufEaters*,long);
	void read_CSNTEs();
	void read_CSNTE(BufEaters*,long);
	void read_CVTEs();
	void read_CVTE(BufEaters* buf,long);
	void read_MTE_CVTEs(uint32 cvte_ind);
	void read_CTTEs();
	void read_CTTE(BufEaters*,long);
	SymType* read_TTE(BufEaters*,long);
	void read_dealy(_DTI* dti, uint32 obj_size, read_func_t func);
	BufEaters* read_obj(_DTI* dti, uint32& obj_size, uint32 index);
	BufEaters* read_buf(uint32 offset,uint32& size);
	SymErr read_name_table();
	SymErr read_dshb();

	char* _name_table;	//string table - keep loaded while reading
	const StringPtr	Id()				const 	{return ((StringPtr)_dshb.dshb_id);}
	short page_size()					const 	{return (_dshb.dshb_page_size);}
	short root_mte()					const 	{return (_dshb.dshb_root_mte);}
	virtual unsigned long mod_date()	const 	{return (_dshb.dshb_mod_date);}
	OSType file_creator()				const 	{return (_dshb.dshb_file_creator);}
	ResType file_type()					const 	{return (_dshb.dshb_file_type);}

private:
	SymScope parse_scope(int scope);
	SymClass parse_class(int sclass);
	SymScope parse_kind(int kind);
	SymSec parse_mte_kind(char mte_kind);
	SymCat get_cat(long t);
	SymType* get_type(uint32 tte_ind);
	SymModEntry* cur_mod;
	SymFuncEntry* cur_func;
	SymFuncEntry* cur_block;
	uint32 cvte_globals;	//cvte index of globals
	};

#undef __XSYM_VER__
#endif /* __ALREADY_INCLUDED__ */


