

//====================================================================
// xsym_r.cpp  -  XsymReader class defs for reading Apple's Sym - Xsym 
//				  files and adding symbols to the network


//NOTE: since XsymReader_v32 and XsymReader are practically the same,
//we include this file for XsymReader and put #ifdefs around code
//specific to XsymReader

#include "xsym_verdefs.h"	//sets default to __XSYM_V33__ 
							//if not yet included by another module

#ifndef USE_DUMP_FILE
#include <files.h>
#include <memory.h>
#include <string.h>
#include <TextUtils.h>
#include <PLStringFuncs.h>
#include <strings.h>
#include <Types.h>
#include "utils.h"
#include "DebugDataTypes.h"
#include "symapi.h"
#include "symnet.h" //contains SymNet class
#include "ULogWindow.h"
#endif /* USE_DUMP_FILE */

#include "xsym.h"
#include "debug.h"

#define __XSYM_R_CP__
#if __XSYM_VER__ == __XSYM_V33__
	#include "xsym_r_v3r3.h"	//header undefs __XSYM_VER__ so that xsym_r_v3r3.h can define itself
	#define __XSYM_VER__ __XSYM_V33__
#elif __XSYM_VER__ == __XSYM_V32__
	#include "xsym_r_v3r2.h"
	#define __XSYM_VER__ __XSYM_V32__
#elif __XSYM_VER__ == __XSYM_V34__
	#include "xsym_r_v3r4.h"
	#define __XSYM_VER__ __XSYM_V34__
#else
	#error "No XSYM version defined!"
#endif



#pragma segment xsymsym



XsymReader::XsymReader(int dump) {
	DBG_ENT("XsymReader");
	_symff = (SymFileFormat)__XSYM_VER__;
	_name_table=0;
    _dumping=dump;  //default is 0
	}

SymErr XsymReader::ISymbolics(const StringPtr symbolicsFile) {
	DBG_ENT("ISymbolics");
    char* file_name = (char*) SYM_NEW(char[symbolicsFile[0]+1]);
    if (!file_name) return state();
    cname(file_name,symbolicsFile);
    //gen_offsets();	... if needed to regenerate ...
	_state.force_validate();
    _fp = (SymFile*) SYM_NEW(SymFile(file_name));
    if (!_fp->open())
        SET_ERR(se_open_fail,("Can't open '%s' for input\n", _fp->filename()))
	else {
    	_sections = (Sections*) SYM_NEW(Sections(2));
    	if (!_sections) return state();
    	_sections->set_baseaddr(sec_code,0,0);
    	_sections->set_baseaddr(sec_data,1,0);
		short retval=0; 
		if (read_dshb()!=se_success)
        	SET_ERR(se_fail,("unable to read DSHB header\n"))
		else
			if (memcmp(_dshb.dshb_id,_SYM_FILE_VERSION,_dshb.dshb_id[0]+1)!= 0)
       			SET_ERR(se_wrong_version,("wrong version!\n"))
			else {
				read_name_table();
				}
		cur_mod = 0;
		cur_func = 0;
		cur_block = 0;
		//_hash_table = 0;								
		if (_state.valid()/* && !_dumping*/)	//don't bother to read if we're dumping
			read_xsym();	//try to read in and add to network as we go
		}
	//#ifdef __3DO_DEBUGGER__	
    //    //add special register symbols to symbol network
	//	SymType* _hwreg_type;
	//	hwreg_type = add_type("hwreg",4,tc_reg,0);	//first time
    //  	SymEntry* s = add_sym(REG_NAME_PC,hwreg_type,REG_PC,sec_none,scope_global,sc_reg);
    //   	add_gsym(s);
	//#endif /* __3DO_DEBUGGER__ */
    _fp->close();
	return (state());
	}
	
XsymReader::~XsymReader() {
	DBG_ENT("~XsymReader");
	if (_name_table) SYM_DELETE(_name_table);
    _fp->close();
	}


//===================================================================
// readers

SymErr XsymReader::read_dshb() {
	unsigned char dshb_buf[_SIZEOF_DSHB];
	if (!_fp->read(dshb_buf,_SIZEOF_DSHB))	
        RETURN_ERR(se_read_err,("unable to read DSHB header\n"));
    BufEaters buf((Endian*)this,_SIZEOF_DSHB,dshb_buf);
	#define read_memb(memb,off)	\
		_dshb.dshb##memb = buf.pick(_dshb.dshb##memb,off);
	#define read_dti(tab,off)	\
		_dshb.dshb##tab##.dti_first_page = buf.pick(_dshb.dshb##tab##.dti_first_page,off+_DTI_FIRST_PG_OFF);	\
		_dshb.dshb##tab##.dti_page_count = buf.pick(_dshb.dshb##tab##.dti_page_count,off+_DTI_PAGE_CNT_OFF);	\
		_dshb.dshb##tab##.dti_object_count = buf.pick(_dshb.dshb##tab##.dti_object_count,off+_DTI_OBJ_CNT_OFF);	
	memcpy(_dshb.dshb_id,dshb_buf,32);
	read_memb(_page_size,_DSHB_PG_SZ_OFF);
	read_memb(_hash_page,_DSHB_HASHPG_SZ_OFF);
	read_memb(_root_mte,_DSHB_ROOT_MTE_OFF);
	read_memb(_mod_date,_DSHB_MOD_DATE_OFF);
	read_dti(_frte,_DSHB_FRTE_OFF);
	read_dti(_rte,_DSHB_RTE_OFF);
	read_dti(_mte,_DSHB_MTE_OFF);
	read_dti(_cmte,_DSHB_CMTE_OFF);
	read_dti(_cvte,_DSHB_CVTE_OFF);
	read_dti(_csnte,_DSHB_CSNTE_OFF);
	read_dti(_clte,_DSHB_CLTE_OFF);
	read_dti(_ctte,_DSHB_CTTE_OFF);
	read_dti(_tte,_DSHB_TTE_OFF);
	read_dti(_nte,_DSHB_NTE_OFF);
	read_dti(_tinfo,_DSHB_TINFO_OFF);
	read_dti(_fite,_DSHB_FITE_OFF);
	read_dti(_const,_DSHB_CONST_OFF);
	read_memb(_file_creator,_DSHB_CREATOR_OFF);
	read_memb(_file_type,_DSHB_TYPE_OFF);
	#undef read_memb	
	#undef read_dti
	return state();
	}	
	
void XsymReader::read_xsym() {
	read_RTEs();	//read RTEs to get sections
	read_MTEs();	//read MTEs to get modules & globals
	read_CSNTEs();	//read CSNTEs to get source offsets
	//read_CVTEs();	//read CVTEs to get variables
	read_FRTEs();	//not needed for symnet
	read_CTTEs();	//read CTTEs for type info
	}

void XsymReader::read_RTEs() {
	read_dealy(&_dshb.dshb_rte, _SIZEOF_RTE, &XsymReader::read_RTE);
	}

void XsymReader::read_RTE(BufEaters* buf,long i) {
	DBG_ASSERT(buf && buf->ptr());
	//_RTE* p=0;
	char* name;
	uint32 res_num,nte_ind,mte_first,mte_last,res_size;
	uint32 res_type; //this is ResType=FourCharCode which is ulong... should we byte swap??
	//uint8 res_chars[4];
	if (i==0) {
		DBG_WARN(("skipping first RTE record\n"));
		DBG_DUMP_(XSYM,_SIZEOF_RTE,buf->ptr());
		return;
		}
	res_num = buf->pick(_TYPE_short,_RTE_RES_NUM_OFF);
	res_size = buf->pick(_TYPE_ulong,_RTE_RES_SIZE_OFF);
	nte_ind = buf->pick(_TYPE_NTEIndex,_RTE_NTE_IND_OFF);
	mte_first = buf->pick(_TYPE_MTEIndex,_RTE_MTE_FIRST_OFF);
	mte_last = buf->pick(_TYPE_MTEIndex,_RTE_MTE_LAST_OFF);
	res_type = buf->pick(_TYPE_ResType,_RTE_RES_TYPE_OFF);
	//memcpy(&res_chars,&res_type,4);
	name = get_nte_name(nte_ind);
	}

void XsymReader::read_FRTEs() {
	read_dealy(&_dshb.dshb_frte, _SIZEOF_FRTE, &XsymReader::read_FRTE);
	}

void XsymReader::read_FRTE(BufEaters* buf, long i) {
	DBG_ASSERT(buf && buf->ptr());
	//_FRTE* frte=0;
	static char file_name[255]="";
	char* name=0;
	char* mte_name=0;
	uint32 mod_date=0,frte_off=0,frte_mte_ind=0,frte_name_ent=0,frte_nte_ind=0;
	
	if (i==0) {
		DBG_WARN(("skipping first FRTE record\n"));
		DBG_DUMP_(XSYM,_SIZEOF_FRTE,buf->ptr());
		return;
		}
	frte_name_ent = buf->pick(_TYPE_IndexSize,_FRTE_NAME_ENT_OFF);
	if (frte_name_ent == _FILE_NAME_INDEX) {
		frte_nte_ind = buf->pick(_TYPE_NTEIndex,_FRTE_NTE_IND_OFF);
		mod_date  = buf->pick(_TYPE_long,_FRTE_MOD_DATE_OFF);
		name = get_nte_name(frte_nte_ind);
		if (name) strcpy(file_name,name);
		else file_name[0]=0;
		} 
	else if (frte_name_ent == _END_OF_LIST) {
		file_name[0]=0;
		} 
	else {
		frte_off = buf->pick(_TYPE_long,_FRTE_FILE_OFF_OFF);
		frte_mte_ind = buf->pick(_TYPE_MTEIndex,_FRTE_MTE_IND_OFF);
		mte_name = get_mte_name(frte_mte_ind);
		}
	}
	
void XsymReader::read_MTEs() {
	read_dealy(&_dshb.dshb_mte, _SIZEOF_MTE, &XsymReader::read_MTE);
	}

//read cvtes belonging to mte - stop at end of list
void XsymReader::read_MTE_CVTEs(uint32 cvte_ind) {
	//_CVTE_P cvte;
	uint32 cvte_type;
	if (cvte_ind == 0) {
		//probably not a bug, but just wierdness...
		//MW seems to use 1st record for something else - what???
		DBG_WARN(("skipping 0'th entry\n"));
		return;
		}
	uint32 max_objs = get_nobjs(&_dshb.dshb_cvte);
	//add variables for this mte
	for (int i=cvte_ind; (i<max_objs); i++) {
		BufEaters* cvte_buf;
		uint32 size = _SIZEOF_CVTE;
		cvte_buf = read_obj(&_dshb.dshb_cvte,size,i);
		if (!cvte_buf) {
			SET_ERR(se_fail,("unable to read cvte object at index=%d\n",i))
			continue;
			}
		//cvte = (_CVTE_P)cvte_buf->ptr();
		cvte_type = cvte_buf->pick(_TYPE_IndexSize,_CVTE_FILE_CH_OFF);	//gets uint16/uint32 based on type
 		if (cvte_type==_END_OF_LIST) {
			FREE(cvte_buf->buf());
			SYM_DELETE(cvte_buf);
			break;	//quit - done with this list
			}
 		read_CVTE(cvte_buf,i);
		FREE(cvte_buf->buf());
		SYM_DELETE(cvte_buf);
 	 	}
	}
	
void XsymReader::read_MTE(BufEaters* buf, long i) {
	//static uint32 eaddr=0;
	DBG_ASSERT(buf && buf->ptr());
	//_MTE_P mte = (_MTE_P) buf->ptr();
	if (i==0) {
		DBG_WARN(("skipping first MTE record\n"));
		DBG_DUMP_(XSYM,_SIZEOF_MTE,buf->ptr());
		return;
		}
	
	char*  name;
	/* 
	uint32 nte_ind,rte_ind,res_off,size,kind,scope,cmte,
		cvte,clte,ctte,csnte1,csnte2,parent,frte_ind,file_off,imp_end;
	*/
	uint32 nte_ind,res_off,size,cvte,parent;
	char kind,scope;
	nte_ind = buf->pick(_TYPE_NTEIndex,_MTE_NTE_IND_OFF);
	name = get_nte_name(nte_ind);
	//rte_ind = buf->pick(_TYPE_RTEIndex,_MTE_RTE_IND_OFF);
	res_off = buf->pick(_TYPE_long,_MTE_RES_OFF_OFF);
	size = buf->pick(_TYPE_long,_MTE_SIZE_OFF);
	kind = buf->pick(_TYPE_char,_MTE_KIND_OFF);
	scope = buf->pick(_TYPE_char,_MTE_SCOPE_OFF);
	//cmte = buf->pick(_TYPE_CMTEIndex,_MTE_CMTE_IND_OFF);
	cvte = buf->pick(_TYPE_CVTEMTEIndex,_MTE_CVTE_IND_OFF);
	//clte = buf->pick(_TYPE_CLTEIndex,_MTE_CLTE_IND_OFF);
	//ctte = buf->pick(_TYPE_CTTEIndex,_MTE_CTTE_IND_OFF);
	//csnte1 = buf->pick(_TYPE_CSNTEIndex,_MTE_CSNTE_IND1_OFF);
	//csnte2 = buf->pick(_TYPE_CSNTEIndex,_MTE_CSNTE_IND2_OFF);
	parent = buf->pick(_TYPE_MTEIndex,_MTE_PARENT_OFF);
	//_FREF* imp_fref = &mte->mte_imp_fref;
	//frte_ind = buf->pick(_TYPE_FRTEIndex,_MTE_IMP_FREF_OFF+_FREF_FRTE_OFF);
	//file_off = buf->pick(_TYPE_long,_MTE_IMP_FREF_OFF+_FREF_FOFF_OFF);
	//imp_end =  buf->pick(_TYPE_long,_MTE_IMP_END_OFF);
	
	
	char* mname = 0; //get_frte_mte_name(frte_ind);
	SymModEntry* m;
	SymFuncEntry* f;
	SymSec sym_sec;
	uint32 val;
	SymEntry* sym;
	SymType* type;
	switch (kind) {
		case MODULE_KIND_UNIT:	//we get CMTE, CVTE
			DBG_FIXME(("need to get code offsets\n"));
			if (!strcmp(name,"%?Anon")) {	//this is the root
                cur_mod = 0;
                DBG_ASSERT(cur_func==0);
                DBG_ASSERT(cur_block==0);
                cur_func=0;
                cur_block=0;
            	read_MTE_CVTEs(cvte_globals);	//get globals now
            	}
            else {
				char* mod_name; char* mod_path;
				//if (cur_mod) cur_mod->set_eaddr(eaddr);	//set eaddr to last addr
				//eaddr=res_off>0?res_off-1:0;
                mod_name = rmv_path(':',name,mod_path);
				m = add_mod(0,res_off,res_off+size);	//should change this to pass path...
                m->set_name(mod_name,mod_path);
                cur_mod = m;
                cur_func = 0;
                cur_block = 0;
                read_MTE_CVTEs(cvte);	//get stuff local to module (statics)
            	cur_mod = 0;
                DBG_ASSERT(cur_func==0);
                DBG_ASSERT(cur_block==0);
                cur_func = 0;
				}
			break;
		case MODULE_KIND_PROGRAM:
			cvte_globals = cvte;	//save to get globals after modules
			cur_mod = 0;
            DBG_ASSERT(cur_func==0);
            DBG_ASSERT(cur_block==0);
            cur_func = 0;
            cur_block = 0;
			break;
		case MODULE_KIND_NONE:	//globals with no module, we get resoff, size, scope
			DBG_FIXME(("need to get code offsets\n"));
			val = res_off; // + _sections->baseaddr(resnum);
			sym_sec = sec_code; //_sections->sectype(resnum);
			type = get_fun_type(tc_function);
			if (name && name[0]=='.') name++;	//power open funcs begin with '.'s
			sym = add_sym(name,type,val,sec_code,scope_global,sc_code);
			f = add_func(sym,0);
			add_gfunc(f,0);
			f->set_baddr(res_off);
			f->set_eaddr(res_off+size);
			break;
		case MODULE_KIND_PROCEDURE:	
			DBG_WARN(("MODULE_KIND_PROCEDURE treated as func\n"));
		case MODULE_KIND_FUNCTION:	//global funcs within module, 
			//we get resoff, size, scope,frte_ind file_off imp_end CVTE CSNTE1 CSNT2
			DBG_FIXME(("need to get code offsets??\n"));
			val = res_off; // + _sections->baseaddr(resnum);
			SymScope sym_scope = parse_scope(scope);
			if (sym_scope==scope_local) sym_scope = scope_module;	//local really means module scope
			type = get_fun_type(tc_function);
			if (name && name[0]=='.') name++;	//power open funcs begin with '.'s
			sym = add_sym(name,type,val,sec_code,sym_scope,sc_code);
			mname = get_mte_name(parent); //get modname from parent
			char *mod_name, *mod_path; 
            mod_name = rmv_path(':',mname,mod_path);
			if (!(m = search_mods(mod_name),m)) 
				SET_ERR(se_fail,("no module found for parent=x%x, mname=%s\n",parent,mname?mname:"-none-"));
			cur_mod=m;
            DBG_ASSERT(cur_mod);
			f = add_func(sym,m);
			add_gfunc(f,m);
			f->set_baddr(res_off);
			f->set_eaddr(res_off+size);
			m->set_baddr((m->baddr()==0)?f->baddr():min(f->baddr(),m->baddr()));
			m->set_eaddr(max(f->eaddr(),m->eaddr()));
			cur_func = f;
			cur_block = 0;
            read_MTE_CVTEs(cvte);	//get function locals
            cur_mod = 0;
            DBG_ASSERT(cur_block==0);
            cur_func = 0;
			break;
		case MODULE_KIND_DATA:		
			SET_ERR(se_fail,("MW doesn't generate data mtes - ignoring\n"));
			break;
		case MODULE_KIND_BLOCK:		
			SET_ERR(se_fail,("don't support global blocks - ignoring\n"));
			break;
		default:
			SET_ERR(se_unknown_type,("unknown mte_kind\n"));
		}
	}
	
void XsymReader::read_CSNTEs() {
	read_dealy(&_dshb.dshb_csnte, _SIZEOF_CSNTE, &XsymReader::read_CSNTE);
	}

void XsymReader::read_CSNTE(BufEaters* buf, long i) {
	DBG_ASSERT(buf && buf->ptr());
	//_CSNTE* t=0;
	char* mname=0;
	uint32 frte_ind=0, file_off=0, fdelta=0, mte_ind=0, mte_off=0, entry_type=0;
	static uint32 last_mte_ind=0, mte_start=0;	//starting addr
	
	if (i==0) {
		DBG_WARN(("skipping first CSNTE record\n"));
		DBG_DUMP_(XSYM,_SIZEOF_CSNTE,buf->ptr());
		return;
		}
	static uint32 src_off=0;
	uint32 code_off=0;
	entry_type = buf->pick(_TYPE_IndexSize,_CSNTE_FILE_CH_OFF);
	switch (entry_type) {
		case _SOURCE_FILE_CHANGE:
			cur_mod = 0;
			frte_ind = buf->pick(_TYPE_FRTEIndex,_CSNTE_FREF_OFF+_FREF_FRTE_OFF);
			file_off = buf->pick(_TYPE_long,_CSNTE_FREF_OFF+_FREF_FOFF_OFF);
			//read_name_FRTE(frte_ind, file_off);
			src_off = file_off;
			mname = get_frte_mte_name(frte_ind);
			char *mod_name, *mod_path; 
            mod_name = rmv_path(':',mname,mod_path);
			if (!(cur_mod = search_mods(mod_name),cur_mod)) {
				SET_ERR(se_fail,("no module found for frte_ind=x%x, mname=%s\n",frte_ind,mname?mname:"-none-"));
				}
			break;
		case _END_OF_LIST:
			if (cur_mod->lines==0) {
				DBG_WARN(("no source found for module %s\n",
					cur_mod->name()?cur_mod->name():"-none-"));
				#ifdef __3DO_DEBUGGER__	
					char* wstr = str(" Warning! No source found for module %s\n",
						(cur_mod&&cur_mod->name())?cur_mod->name():"-none-");
					wstr[0]=strlen(wstr+1);
					WriteDebuggerMsg(wstr);
				#endif /* __3DO_DEBUGGER__	*/
				}
			cur_mod = 0;
			src_off = 0;
			break;
		default:
			fdelta = buf->pick(_TYPE_short,_CSNTE_FDELTA_OFF); //delta from previous src location
			mte_ind = buf->pick(_TYPE_MTEIndex,_CSNTE_MTE_IND_OFF); // mte index
			mte_off = buf->pick(_TYPE_MTEOffset,_CSNTE_MTE_OFF_OFF); //code offset into mte	
			//code_off = mte_off + get_mte_start(mte_ind);
			if (mte_ind!=last_mte_ind) {
				mte_start = get_mte_start(mte_ind);	//get new starting addr
				last_mte_ind=mte_ind;
				}
			code_off = mte_off + mte_start;
			src_off += fdelta;
			DBG_ASSERT(cur_mod);
			add_charoff(cur_mod,src_off,code_off);
		}
	}

void XsymReader::read_CTTEs() {
	read_dealy(&_dshb.dshb_ctte, _SIZEOF_CTTE, &XsymReader::read_CTTE);
	}

void XsymReader::read_CTTE(BufEaters* buf, long i) {
	DBG_ASSERT(buf && buf->ptr());
	uint32 index=0,fdelta=0,frte_ind=0,file_off=0,nte_ind;
	char* name=0;
	char* mname=0;
	if (i==0) {
		DBG_WARN(("skipping first CTTE record\n"));
		DBG_DUMP_(XSYM,_SIZEOF_CTTE,buf->ptr());
		return;
		}
	//_CTTE_P t = (_CTTE_P)buf->ptr();
	uint32 ctte_type = buf->pick(_TYPE_IndexSize,_CTTE_FILE_CH_OFF);
	if (ctte_type == _END_OF_LIST) {
		}
	else if (ctte_type == _SOURCE_FILE_CHANGE) {
		frte_ind = buf->pick(_TYPE_FRTEIndex,_CTTE_FREF_OFF+_FREF_FRTE_OFF);
		file_off = buf->pick(_TYPE_long,_CTTE_FREF_OFF+_FREF_FOFF_OFF);
		mname = get_frte_mte_name(frte_ind);
		} 
	else {
		index = buf->pick(_TYPE_TTEIndex,_CTTE_TTE_IND_OFF);
		fdelta = buf->pick(_TYPE_short,_CTTE_FDELTA_OFF);
		nte_ind = buf->pick(_TYPE_NTEIndex,_CTTE_NTE_IND_OFF);
		name = get_nte_name(nte_ind);
		}
	}

SymType* XsymReader::read_TTE(BufEaters* buf, long i) {
	char* name; uint32 nte_ind, psize, size;
	unsigned char *type;
	if (i==0) {
		DBG_WARN(("skipping first TTE record\n"));
		DBG_DUMP_(XSYM,_SIZEOF_DTTE,buf->ptr());
		return 0;
		}
	DBG_FIXME(("don't know what type structure is!!! doesn't match DebugDataTypes!!\n"));
	return 0;
	//_DTTE* tte = (_DTTE*) buf->ptr();
	nte_ind = buf->pick(_TYPE_NTEIndex,_DTTE_NTE_IND_OFF); //delta from previous src location
	name = get_nte_name(nte_ind);
	psize = buf->pick(_TYPE_ushort,_DTTE_PSIZE_OFF); //delta from previous src location
	size = min(get_tte_size(buf,psize),124);
	type = get_tte_bytes(buf,psize);
	psize &= 0x1ff;	//type size
	DBG_(XSYM,("type name = %s\n",name));
	DBG_(XSYM,("size=x%x, psize=x%x\n", 
		size,psize));
	DBG_DUMP_(XSYM,size,type);
	//tref = parse(len,type);
	DBG_FIXME(("need to parse type!\n"));
	return add_type(name,psize,tc_ref,0);
	}

void XsymReader::read_CVTEs() {
	//CVTE: first element is source_file_change, 
	//		followed by list of variable CVTEs 
	//		then an end_of _list
	//		Keep reading until end of table
	read_dealy(&_dshb.dshb_cvte, _SIZEOF_CVTE, &XsymReader::read_CVTE);
	}


void XsymReader::read_CVTE(BufEaters* buf, long i) {	
	DBG_ASSERT(buf && buf->ptr());
	//_CVTE_P t = (_CVTE_P)buf->ptr();
	char* name=0;
	char* mname=0;
	uint32 val=0;
	SymType* sym_type=0;
	SymEntry* sym;
	SymScope sym_scope;
	SymSec sym_sec;
	SymClass sym_class;
	static SymModEntry* m=0;	//module defined in
	unsigned char la_addr[kCVTE_LA_MAX_SIZE]="";
	uint32 scope=0,kind=0,sclass=0;
	uint32 cvte_type=0,type=0,fdelta=0,offset=0,
		big_la=0,la_size=0,frte_ind=0,file_off=0,nte_ind=0;
	if (i==0) {
		DBG_WARN(("skipping first CVTE record\n"));
		DBG_DUMP_(XSYM,_SIZEOF_CVTE,buf->ptr());
		return;
		}
	cvte_type = buf->pick(_TYPE_IndexSize,_CVTE_FILE_CH_OFF);	//gets uint16/uint32 based on type
	switch (cvte_type) {
		case _SOURCE_FILE_CHANGE: {
			DBG_(XSYM,("_SOURCE_FILE_CHANGE\n"));
			frte_ind = buf->pick(_TYPE_FRTEIndex,_CVTE_FILE_FR_OFF+_FREF_FRTE_OFF);
			file_off = buf->pick(_TYPE_long,_CVTE_FILE_FR_OFF+_FREF_FOFF_OFF);
			mname = get_frte_mte_name(frte_ind);
			char *mod_name, *mod_path; 
            mod_name = rmv_path(':',mname,mod_path);
			if (!(m = search_mods(mod_name),m)) {
				SET_ERR(se_fail,("no module found for frte_ind=x%x, mname=%s\n",frte_ind,mname?mname:"-none-"));
				}
			break;
			}
		case _END_OF_LIST: {
			DBG_(XSYM,("_END_OF_LIST\n"));
			break;
			}
		default: {
			DBG_(XSYM,("variable\n"));
			//buf->puke(cvte_type); //put it back - we need it for reading variable
			type = buf->pick(_TYPE_TTEIndex,_CVTE_TTE_OFF);
			nte_ind = buf->pick(_TYPE_NTEIndex,_CVTE_NTE_OFF);
			fdelta = buf->pick(_TYPE_short,_CVTE_FDELTA_OFF);
			scope = buf->pick(_TYPE_char,_CVTE_SCOPE_OFF);
			la_size = buf->pick(_TYPE_char,_CVTE_LASIZE_OFF);
			//get stuff for adding symbol
			name = get_nte_name(nte_ind);
			val = 0;
			sym_scope = parse_scope(scope);
			sym_sec = sec_none;
			sym_class = sc_none;
			sym_type = get_type(type);
			//if (kind!=STORAGE_KIND_LOCAL)
			//	cat=tc_arg;
			
			//handle union entries separately
			//get location (char char long)
			if (la_size == 0) {
				kind = buf->pick(_TYPE_char,_CVTE_LOC_OFF+_SCA_KIND_OFF);
				sclass = buf->pick(_TYPE_char,_CVTE_LOC_OFF+_SCA_CLASS_OFF);
				offset = buf->pick(_TYPE_long,_CVTE_LOC_OFF+_SCA_OFFSET_OFF);
				val = offset;
				sym_class = parse_class(sclass);
				sym_sec = get_symsec(sym_class);
				//parse_kind(kind);	//get kind
				//parse_class(sclass);	//get class
				} 
			//get big address
			else if (la_size == kCVTE_BIG_LA) {
				big_la = buf->pick(_TYPE_long,_CVTE_BLAS_LA_OFF);
				kind = buf->pick(_TYPE_char,_CVTE_BLAS_LAK_OFF);
				val = big_la;	//FIXME!!
				sym_sec = sec_none;
				sym_class = sc_const;
				//parse_kind(kind);	//get kind
				} 
			//get address
			else {
				int i = 0;
				memcpy(&la_addr,buf->ptr()+_CVTE_LAS_LA_OFF,kCVTE_LA_MAX_SIZE);
				kind = buf->pick(_TYPE_char,_CVTE_LAS_LAK_OFF+kCVTE_LA_MAX_SIZE);
				val = get_symaddr(la_addr,la_size);
				sym_sec = sec_data;
				sym_class = sc_data;
				//parse_kind(kind);	//get kind
				}
			/*
			case STORAGE_CLASS_REGISTER:	return sc_reg;
			case STORAGE_CLASS_A5:			return sc_data;
			case STORAGE_CLASS_A6:			return sc_stack;
			case STORAGE_CLASS_A7:			return sc_data;	//relative to rsrc
			case STORAGE_CLASS_ABSOLUTE:	return sc_abs;
			case STORAGE_CLASS_CONSTANT:	return sc_const;
			case STORAGE_CLASS_RESOURCE:	return sc_none;	
			case STORAGE_CLASS_BIGCONSTANT:	return sc_const;
			*/
			sym = add_sym(name,sym_type,val,sym_sec,sym_scope,sym_class);
			if (cur_block) {	//local to block
				if (sym_class==sc_reg || sym_class==sc_stack) {
					add_lsym(sym,cur_block);
					DBG_ASSERT(m && m==cur_block->mod());
					}
				}	//non-auto data will be added when parsing root
			else {
				if (cur_func) {	//local to function
					if (sym_class==sc_reg || sym_class==sc_stack) {
						add_lsym(sym,cur_func);
						DBG_ASSERT(m && m==cur_func->mod());
						}	//non-auto data will be added when parsing root
					}
				else {
					if (cur_mod) {	//local to module
						if (scope==SYMBOL_SCOPE_LOCAL) {
							add_msym(sym,m);	//(m better = cur_mod!!!)
							DBG_ASSERT(m==cur_mod);
							sym->set_scope(scope_module);
							}	//globals will be added when parsing root
						}
					else {
						if (scope==SYMBOL_SCOPE_GLOBAL) {
							add_gsym(sym,m);	//global
							}
						}
					}
				}
			}
		}				
	}
	
void XsymReader::read_dealy(_DTI* dti, uint32 obj_size, read_func_t func) {
	DBG_ASSERT(dti);
	uint32 nobjs = get_nobjs(dti);
	if (nobjs==0) {
		DBG_WARN(("   ---no objects in table---\n"))
		}
	else {
		DBG_ASSERT(obj_size);
		DBG_ASSERT(_dshb.dshb_page_size);
		uint32 off, npages; 
		uint32 nobjs_per_pg, size_per_pg, nobjs_this_pg, size_this_page;
		uint32 obj_count = 0;
		BufEaters* buf;
		// initialize
		size_per_pg = _dshb.dshb_page_size;
		off = get_off(dti);	//offset of first page
		nobjs_per_pg = size_per_pg/obj_size;
		npages = dti->dti_page_count;
		DBG_(XSYM,("npages=x%x, nobjs=x%x, nobjs_per_pg=x%x, obj_size=x%x\n",npages,nobjs,nobjs_per_pg,obj_size));
		// loop thru objs
		for (long i=0; i<npages; i++) {	//loop over pages
			nobjs_this_pg = min(nobjs_per_pg,nobjs);
			DBG_(XSYM,("page=x%x, nobjs_this_pg=x%x\n",i,nobjs_this_pg));
			size_this_page = nobjs_this_pg*obj_size;
			buf = read_buf(off,size_this_page);
			for (long i=0; i<nobjs_this_pg; i++) {	//loop over objects in this page
				(this->*func)(buf,obj_count++);
				buf->eat_nbites(obj_size);
				}
			// get ready for next loop
			nobjs -= nobjs_this_pg;
			off += size_per_pg;	//next page
			FREE(buf->buf());
			SYM_DELETE(buf);
			}
		}
	}
	
BufEaters* XsymReader::read_obj(_DTI* dti, uint32& obj_size, uint32 index) {
#ifdef __MWERKS__
#pragma ARM_conform on
#endif
	uint32 nobjs = get_nobjs(dti);
	if (nobjs==0) {
		DBG_WARN(("no objects in table!!\n"));
		return 0;
		}
	else {
		DBG_ASSERT(_dshb.dshb_page_size);
		uint32 off, npages; 
		uint32 nobjs_per_pg, size_per_pg, size;
		BufEaters* buf;
		// initialize
		npages = dti->dti_page_count;
		size_per_pg = _dshb.dshb_page_size;
		if (obj_size==0) { //unknown size - like nte, tte, etc..
			nobjs_per_pg = size_per_pg;
			obj_size = 1;
			size = 0;
			}
		else {
			nobjs_per_pg = size_per_pg/obj_size;
			size = obj_size;
			}
		if (index==0) {	//read whole deal
			off = get_off(dti);
			size = size_per_pg*npages;
			}
		else {
			//figure out which page index is in
			//if index=nobjs_per_pg, it will be on next page
			for (long page=0; page<npages && index>=nobjs_per_pg; page++) {
				index -= nobjs_per_pg;
				} //now page is page, index is index into page
			off = index*obj_size + get_off(dti) + page*size_per_pg;
			DBG_(PICKY,("off=x%x, index=x%x, page=%d, npages=x%x, nobjs=x%x, nobjs_per_pg=x%x, obj_size=x%x\n",off, index, page, npages,nobjs,nobjs_per_pg,obj_size));
			}
		buf = read_buf(off,size);
		obj_size = size; 	//how many bytes we actually read
		DBG_ASSERT(buf);
		return buf;
		}
	}
	
BufEaters* XsymReader::read_buf(uint32 off,uint32& size) {
	DBG_(PICKY,("off=x%x, size=x%x\n",off,size));
	DBG_(PICKY,("before seek - fp=x%x\n",_fp->tell()));
	if (!_fp->seek(off,SEEK_SET)) {
        SET_ERR(se_seek_err,("read_DTI: seek error to 0x%x\n",off));
        return 0;
        }
	DBG_(PICKY,("after seek - fp=x%x\n",_fp->tell()));
    if (size==0) {	//means we'll find size at the first byte
		if (!_fp->read(&size,1)) {
       	 	SET_ERR(se_read_err,("read_DTI: read failed at x%x\n",_fp->tell()));
        	return 0;
        	}
        }
	unsigned char* buf = (unsigned char*) MALLOC(size);
	if (!buf) {
        SET_ERR(se_malloc_err,("read_DTI: unable to create buffer; size=x%x\n",size));
        return 0;
        }
	if (!_fp->read(buf,size)) {
        SET_ERR(se_read_err,("read_DTI: read failed at x%x\n",_fp->tell()));
		FREE(buf);
        return 0;
        }
	DBG_(XSYM,("after read - fp=x%x\n",_fp->tell()));
	BufEaters* buf_eatr = (BufEaters*) SYM_NEW(BufEaters((Endian*)this,size,buf));
	if (!buf_eatr) {
        SET_ERR(se_malloc_err,("read_DTI: unable to create buf_eatr object\n"));
		FREE(buf);
        return 0;
        }
	return buf_eatr;
	}
	
SymErr XsymReader::read_name_table() {
	uint32 size = get_size(&_dshb.dshb_nte);
	uint32 fpos = get_off(&_dshb.dshb_nte);
	if (!size) { 
		SET_ERR(se_invalid_obj,("no name table!\n"));
		}
	else {
		_name_table = (char*) SYM_NEW(char[size]);
		if (!_name_table) 
        	RETURN_ERR(se_malloc_err,("unable to seek to name table\n"))
		DBG_(XSYM,("_name_table=x%x, size=x%x\n",_name_table, size));
		if (!_fp->seek(fpos,SEEK_SET)) {
        	SET_ERR(se_seek_err,("unable to seek to name table\n"))
        	SYM_DELETE(_name_table); 
        	_name_table = 0;
        	}
		else {	//may need to break it up into chuncks... 
			if (!_fp->read(_name_table,size)) {
        		SET_ERR(se_read_err,("unable to read name table\n"))
        		SYM_DELETE(_name_table); 
        		_name_table = 0;
        		}
			}
		}
	return state();
	}

//==========================================================================
// types	

uint32 XsymReader::get_tte_size(BufEaters* buf, uint32 psize) {
		return ((psize&0x1000)
				?	buf->pick(_TYPE_ulong,_DTTE_LONG_SZ_OFF)
				:	buf->pick(_TYPE_ushort,_DTTE_WORD_SZ_OFF)); 
				}
unsigned char* XsymReader::get_tte_bytes(BufEaters* buf, uint32 psize) {
		return ((psize&0x1000)
				?	buf->ptr()+_DTTE_LONG_BZ_OFF	
				:	buf->ptr()+_DTTE_WORD_BZ_OFF);
				}

//copied here for reference...
/*	DISK_TYPE_TABLE_ENTRY
** The structure of the TYPE TABLE is currently unknown.  All that is
** known is that it will contain base type information, composite
** type information (indices back into itself), and name table information
** (indices into the name table for the names of record fields and
** enumerated types).
**
** The structure will be:
**		ARRAY [0 .. 32767] OF UNSIGNED
*/
//#define dtte_word_size	tte_.word_.size
//#define dtte_word_bytes	tte_.word_.bytes
//#define dtte_long_size	tte_.long_.size
//#define dtte_long_bytes	tte_.long_.bytes
//struct DISK_TYPE_TABLE_ENTRY {
//	NTEIndex		dtte_nte;					/* The name of the type					*/
//	unsigned short	dtte_psize;					/* Hi-bit: Big.  Lower 9: physical size */
//	union {
//		struct {
//			unsigned short	size;				/* two bytes of size	*/
//			unsigned char	bytes[1];			/* type codes			*/
//		} word_;
//		struct {
//			unsigned long	size;				/* four bytes of size	*/
//			unsigned char	bytes[1];			/* type codes			*/
//		} long_;
//	} tte_;
//};

SymType* XsymReader::get_type(uint32 tte_ind) {
	SymType* t=0;
	SymCat cat;
	uint32 len=0,size=0,psize=0;
	//unsigned char *type;
	char* name=0;
	if (tte_ind < 100) {
		cat = get_cat(tte_ind);
		t=get_fun_type(cat);
		} 
	else {	//let's hope _SIZEOF_TTE+MAX_TYPE_SIZE is big enough...
		DBG_FIXME(("found user type index=%d=x%x\n",tte_ind,tte_ind));
		/***
		ignore types for now...
		uint32 size = _SIZEOF_DTTE+MAX_TYPE_SIZE;
		BufEaters* buf;
		if (!(buf = read_obj(&_dshb.dshb_tte,size,tte_ind),buf)) {
			SET_ERR(se_fail,("unable to read type! at index %d\n",tte_ind))
			}
		else {	
			t=read_TTE(buf,tte_ind);
			SYM_DELETE(buf->buf());
			SYM_DELETE(buf);
			}
		***/
		}
	return t;
	}
			
SymCat XsymReader::get_cat(long t) {
	long cat = 0;
	switch (t) {
		case XT_void:	cat = tc_void;	break;
		case XT_ulong:	cat = tc_ulong;	break;
		case XT_long:	cat = tc_long;	break;
		case XT_double:	cat = tc_double;	break; //Reggie had double as 13!! MW gives 4
		case XT_uchar:	cat = tc_uchar;	break;
		case XT_char:	cat = tc_char;	break;
		case XT_schar:	cat = tc_char;	break;
		case XT_byte:	cat = tc_char;	break;
		case XT_ushort:	cat = tc_ushort;	break;
		case XT_short:	cat = tc_short;	break;
		case XT_float:	cat = tc_float;	break;
		case XT_string:	cat = 0x20000; /*BTYPE_String*/	break;	//BTYPE_String
		case XT_13:		cat = tc_double;	break;	//Reggie had double as 13!! MW gives 4
		case XT_1:	
		case XT_5:	
		case XT_14:	
		case XT_15:
		case XT_17:
			SET_ERR(se_unknown_type,("what are these???\n"));
			cat = tc_unknown;
			break;
		default:	
			SET_ERR(se_unknown_type,("unknown XSYM fun type!\n"));
			cat = tc_unknown;
			break;
		}
	return (SymCat) cat;
	}
	
	
//===================================================================
// utilities

char* XsymReader::get_nte_name(long nte_index) {
	DBG_ASSERT(nte_index >=0 && nte_index*2<get_size(&_dshb.dshb_nte));
    //DBG_(XSYM,("index=x%x, _name_table=x%x, size=x%x, offset=x%x\n",nte_index,_name_table,get_size(&_dshb.dshb_nte),_name_table+nte_index*2));
	StringPtr pname;
	char* cn=0;
	if (nte_index*2>=get_size(&_dshb.dshb_nte)) {
        SET_ERR(se_bad_call,("name table index out of range\n"));
        return 0;
        }
	pname = ((StringPtr)(_name_table+nte_index*2));
	if (pname && pname[0]) {
		//memcpy(cn,pname+1,pname[0]);
		//cn[pname[0]]=0;
		cn = str(pname[0],"%s",pname+1);
		DBG_ASSERT(cn);
		cname(cn,pname);
		}
	return cn;
	}
	
uint32 XsymReader::get_mte_start(uint32 mte_index) {
	uint32 addr =0;
	DBG_ASSERT(mte_index>0 && mte_index<=get_nobjs(&_dshb.dshb_mte));
	if (mte_index==0) return 0;
	if (mte_index<0 || mte_index>get_nobjs(&_dshb.dshb_mte)) {
		DBG_(XSYM,("index out of range: mte_index=x%x\n",mte_index));
		return 0;
		}
	uint32 size = _SIZEOF_MTE;
	BufEaters* buf = read_obj(&_dshb.dshb_mte,size,mte_index);
	if (!buf) return 0;
	_MTE_P mte = (_MTE_P) buf->ptr();
	addr = buf->pick(_TYPE_long,_MTE_RES_OFF_OFF);
	FREE(buf->buf());
	SYM_DELETE(buf);
	return addr;
	}
	
/****
uint32 XsymReader::get_mte_srcoff(uint32 mte_index) {
	uint32 off =0;
	BufEaters* buf = read_obj(&_dshb.dshb_mte,_SIZEOF_MTE,mte_index);
	_MTE_P mte = (_MTE_P) buf->ptr();
	frte_index = buf->pick(_TYPE_long,_MTE_FRTE_IND_OFF);
	BufEaters* buf2 = read_obj(&_dshb.dshb_frte,_SIZEOF_FRTE,frte_index);
	_FRTE_P frte = (_FRTE_P) buf->ptr();
	off = buf->pick(_TYPE_long,_FRTE_FILE_OFF_OFF);
	SYM_DELETE(buf->buf());
	SYM_DELETE(buf);
	buf2->digest();
	SYM_DELETE(buf2);
	return off;
	}
******/

char* XsymReader::get_mte_name(uint32 mte_index) {
	char* name=0;
	uint32 nte_ind;
	DBG_ASSERT(mte_index>0 && mte_index<=get_nobjs(&_dshb.dshb_mte));
	if (mte_index==0) return 0;
	if (mte_index<0 || mte_index>get_nobjs(&_dshb.dshb_mte)) {
		DBG_(XSYM,("index out of range: mte_index=x%x\n",mte_index));
		return 0;
		}
	uint32 size = _SIZEOF_MTE;
	BufEaters* buf = read_obj(&_dshb.dshb_mte,size,mte_index);
	if (!buf) return 0;
	nte_ind = buf->pick(_TYPE_NTEIndex,_MTE_NTE_IND_OFF);
	name = get_nte_name(nte_ind);
	FREE(buf->buf());
	SYM_DELETE(buf);
	return name;
	}
char* XsymReader::get_frte_mte_name(uint32 frte_index) {
	char* name=0;
	uint32 size = _SIZEOF_FRTE, frte_type, name_ind;
	DBG_ASSERT(frte_index>0 && frte_index<=get_nobjs(&_dshb.dshb_frte));
	if (frte_index==0) return 0;
	if (frte_index<0 || frte_index>get_nobjs(&_dshb.dshb_frte)) {
		DBG_(XSYM,("index out of range: frte_index=x%x\n",frte_index));
		return 0;
		}
	BufEaters* buf = read_obj(&_dshb.dshb_frte,size,frte_index);
	if (!buf) return 0;
	frte_type = buf->pick(_TYPE_IndexSize,_FRTE_NAME_ENT_OFF);
	if (frte_type == _FILE_NAME_INDEX) {
		name_ind = buf->pick(_TYPE_NTEIndex,_FRTE_NTE_IND_OFF);
		name = get_nte_name(name_ind);
		}
	else {
		name_ind = buf->pick(_TYPE_MTEIndex,_FRTE_MTE_IND_OFF);
		name = get_mte_name(name_ind);
		}
	FREE(buf->buf());
	SYM_DELETE(buf);
	return name;
	}
	
SymClass XsymReader::get_symclass(SymSec sectype) {
   //switch(_sections->sectype(secnum)) {
   switch(sectype) {
   		case sec_code: return sc_code;
   		case sec_data: return sc_data;
   		case sec_bss:  return sc_bss;
   		case sec_none: return sc_none;
		default: 
			SET_ERR(se_unknown_type,("unknown sectype=x%x\n",sectype));
			return sc_none;
   	   }
	}
	
SymSec XsymReader::get_symsec(SymClass sclass) {
   switch(sclass) {
   		case sc_code: return sec_code;
   		case sc_data: return sec_data;
   		case sc_bss:  return sec_bss;
   		case sc_reg:
   		case sc_const:
   		case sc_abs:
   		case sc_stack:
   		case sc_none: return sec_none;
		default: 
			SET_ERR(se_unknown_type,("unknown sclass=x%x\n",sclass));
			return sec_none;
   	   }
	}

uint32 XsymReader::get_symaddr(unsigned char* la_addr,uint32 la_size) { 	//scope is really a char
	DBG_FIXME(("parse addr!!\n"));
	uint32 la_addr_val=0;	//logical address bytes
	DBG_ASSERT(la_size<=kCVTE_LA_MAX_SIZE);
	if (la_size>kCVTE_LA_MAX_SIZE) la_size=kCVTE_LA_MAX_SIZE;
	if (la_size==0) la_addr_val=0;
	else {
		for (int i = 0; i < la_size; ++i) {
			la_addr_val = la_addr_val*16+((unsigned char)la_addr[i]);
			}
		}
	return la_addr_val;
	}

SymScope XsymReader::parse_scope(int scope) { 	//scope is really a char
	switch (scope) {
		case SYMBOL_SCOPE_LOCAL:	return scope_local;
		case SYMBOL_SCOPE_GLOBAL:	return scope_global;
		default:
			SET_ERR(se_unknown_type,("unknown scope x%x\n",scope));
			return scope_none;
		}
	}
	
SymClass XsymReader::parse_class(int sclass) {	//class is really a char
			switch (sclass) {
			case STORAGE_CLASS_REGISTER:	return sc_reg;
			case STORAGE_CLASS_A5:			return sc_data;
			case STORAGE_CLASS_A6:			return sc_stack;
			case STORAGE_CLASS_A7:			
				DBG_FIXME(("what to do with STORAGE_CLASS_A7???\n"));
				return sc_data;
			case STORAGE_CLASS_ABSOLUTE:	
				DBG_FIXME(("STORAGE_CLASS_ABSOLUTE: could be code?? how is different from STORAGE_CLASS_A5???\n"));
				return sc_abs;
			case STORAGE_CLASS_CONSTANT:	return sc_const;
			case STORAGE_CLASS_RESOURCE:	
				DBG_FIXME(("what to do with STORAGE_CLASS_RESOURCE???\n"));
				return sc_none;
			case STORAGE_CLASS_BIGCONSTANT:	
				DBG_FIXME(("need to lookup BIGCONSTANT!\n"));
				return sc_const;
			default:
				SET_ERR(se_unknown_type,("unknown class x%x\n",sclass));
				return sc_none;
			}
	}
	
//only useful for parameters
/***
? XsymReader::parse_kind(int kind) {	//kind is really a char
		DBG_FIXME(("this gives us multiple information! need to handle...\n"));
		switch (kind) {
			case STORAGE_KIND_LOCAL:		return ?; //scope_global/scope_module/scope_local
			case STORAGE_KIND_VALUE:		return ?; //scope_local & tc_arg & tc_ref
			case STORAGE_KIND_REFERENCE:	return ?; //scope_local & tc_arg & tc_pointer
			case STORAGE_KIND_WITH:			
				SET_ERR(se_fail,("can't deal with WITH blocks\n"));
				return ?;
			default:
				SET_ERR(se_unknown_type,("unknown kind x%x\n",kind));
				return ?;
			}
	}
**/
		
SymSec XsymReader::parse_mte_kind(char mte_kind) {
	switch (mte_kind) {
		case MODULE_KIND_PROGRAM:	return sec_none;
		case MODULE_KIND_UNIT:		return sec_code;
		case MODULE_KIND_PROCEDURE:	return sec_code;
		case MODULE_KIND_FUNCTION:	return sec_code;
		case MODULE_KIND_DATA:		return sec_data;
		case MODULE_KIND_BLOCK:		return sec_code;
		default:
			SET_ERR(se_unknown_type,("unknown mte_kind\n"));
			return sec_none;
		}
	}
	
