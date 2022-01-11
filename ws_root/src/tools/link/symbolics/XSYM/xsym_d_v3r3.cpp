

//NOTE: since XsymDumper_v3r2 and XsymDumper are practically the same,
//we include this file for XsymDumper and put #ifdefs around code
//specific to XsymDumper

#include "xsym_verdefs.h"	//sets default to __XSYM_V33__ 
							//if not yet included by another module


#if __XSYM_VER__ == __XSYM_V32__
	#include "xsym_d_v3r2.h"	//header undefs __XSYM_VER__ so that xsym_d_v3r3.h can define itself
	#define __XSYM_VER__ __XSYM_V32__
#elif __XSYM_VER__ == __XSYM_V34__
	#include "xsym_d_v3r4.h"
	#define __XSYM_VER__ __XSYM_V34__
#else 
	#undef __XSYM_VER__ 
	#define __XSYM_VER__ __XSYM_V33__
	#include "xsym_d_v3r3.h"	
	#define __XSYM_VER__ __XSYM_V33__
#endif

#pragma segment xsymsym

//==============================================================
// dumps


#if defined(__NEW_ARMSYM__)
XsymDumper::XsymDumper(char *name, FILE* dump_fp, GetOpts* opts, int dump) 
		: XsymReader(1), Outstrstuffs(dump_fp)
#else
XsymDumper::XsymDumper(char *name, FILE* dump_fp, GetOpts* opts, int dump) 
		: XsymReader(name,1), Outstrstuffs(dump_fp)
#endif
	{
#if defined(__NEW_ARMSYM__)
    char fn[255];
    pname((StringPtr)fn,name);
    ISymbolics((StringPtr)fn);
#endif
    if (!Valid()) {
    	DBG_ERR(("invalid symbolics object!\n"));
    	return;
    	}
    _dump_opts = opts;
    }


void XsymDumper::DumpFile() {
    if (!_fp->open()) return;
	outstr("DISK_SYMBOL_HEADER_BLOCK\n\n");
	outstr("dshb_id='%s'\n", _dshb.dshb_id+1);
	static char version[] = _SYM_FILE_VERSION;
	p2cstr((StringPtr)_dshb.dshb_id);
	p2cstr((StringPtr)version);
	if (strcmp(version, _dshb.dshb_id)) {
		outstr("### Warning: expected version was '%s'; found '%s' ###\n", version, _dshb.dshb_id);
		outstr("### The dump may fail miserably...\n");
		}
	outstr("dshb_page_size=x%x\n", _dshb.dshb_page_size);
	outstr("dshb_hash_page=x%x\n", _dshb.dshb_hash_page);
	outstr("dshb_root_mte=x%x\n", _dshb.dshb_root_mte);
	outstr("dshb_mod_date=$%X\n", _dshb.dshb_mod_date);
	outstr("\n");
	dump_DTIs();	//not needed for symnet
	dump_RTEs();	//read RTEs to get sections
	dump_MTEs();	//read MTEs to get modules & globals
	dump_CSNTEs();	//read CSNTEs to get source offsets
	dump_CVTEs();	//read CVTEs to get variables
	dump_FRTEs();	//not needed for symnet
	dump_CTTEs();	//read CTTEs for type info
    _fp->close();
	}

void XsymDumper::dump_DTIs() {
	outstr("Table    first page      page count    object count\n");
	outstr("-------------------------------------------------------\n");
	dump_DTI("FRTE",&_dshb.dshb_frte);
	dump_DTI("RTE",&_dshb.dshb_rte);
	dump_DTI("MTE",&_dshb.dshb_mte);
	dump_DTI("CMTE",&_dshb.dshb_cmte);
	dump_DTI("CVTE",&_dshb.dshb_cvte);
	dump_DTI("CSNTE",&_dshb.dshb_csnte);
	dump_DTI("CLTE",&_dshb.dshb_clte);
	dump_DTI("CTTE",&_dshb.dshb_ctte);
	dump_DTI("TTE",&_dshb.dshb_tte);
	dump_DTI("NTE",&_dshb.dshb_nte);
	dump_DTI("TINFO",&_dshb.dshb_tinfo);
	dump_DTI("FITE",&_dshb.dshb_fite);
	dump_DTI("CONST",&_dshb.dshb_const);
	outstr("\n");
	}

void XsymDumper::dump_dealy(_DTI* dti, uint32 obj_size, dump_func_t func) {
	read_dealy(dti,obj_size,(read_func_t)func);
	}

void XsymDumper::dump_DTI(char* s,_DTI* dti) {
	DBG_ASSERT(s && dti);
	outstr(" %-6s    %-15d %-15d %-13d\n",
		s,dti->dti_first_page,dti->dti_page_count,dti->dti_object_count);
	}

void XsymDumper::dump_RTEs() {
	outstr("\n\n RESOURCE TABLE ENTRIES (x%x objects)\n\n",_dshb.dshb_rte.dti_object_count);
	outstr("RTE  type    num   size    name           1st MTE   last MTE\n");
	dump_dealy(&_dshb.dshb_rte, _SIZEOF_RTE, &XsymDumper::dump_RTE);
	}

void XsymDumper::dump_RTE(BufEaters* buf,long i) {
	DBG_ASSERT(buf && buf->ptr());
	//_RTE* p=0;
	char* name;
	uint32 res_num,nte_ind,mte_first,mte_last,res_size;
	uint32 res_type; //this is ResType=FourCharCode which is ulong... should we byte swap??
	uint8 res_chars[4];
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
	memcpy(&res_chars,&res_type,4);
	name = get_nte_name(nte_ind);
	outstr("  %-3d %c%c%c%c    %-05d x%-6x %-16s",
			i,
			res_chars[0],res_chars[1],res_chars[2],res_chars[3],
			res_num,res_size,
			name?name:"-none-");
	if (name && strlen(name)>16) 
		outstr("\n                                     ");
	outstr(" %-7d%   -7d\n",
			mte_first,
			mte_last);	
	}


static _FRTEIndex savedIndex;

void XsymDumper::dump_FRTEs() {
	outstr("FILE REFERENCE TABLE ENTRIES (FRTE) (x%x objects)\n\n",_dshb.dshb_frte.dti_object_count);
	outstr("FRTE entry   file     mte_name   mod_date    frte_off frte_mte_ind\n");
	dump_dealy(&_dshb.dshb_frte, _SIZEOF_FRTE, &XsymDumper::dump_FRTE);
	}

void XsymDumper::dump_FRTE(BufEaters* buf, long i) {
	DBG_ASSERT(buf && buf->ptr());
	//_FRTE* frt=0;
	static char file_name[255]="";
	char* name=0;
	char* mte_name=0;
	char entry[4]="";
	uint32 mod_date=0,frte_off=0,frte_mte_ind=0,frte_name_ent=0,frte_nte_ind=0;
	
	if (i==0) {
		DBG_WARN(("skipping first FRTE record\n"));
		DBG_DUMP_(XSYM,_SIZEOF_FRTE,buf->ptr());
		return;
		}
	frte_name_ent = buf->pick(_TYPE_IndexSize,_FRTE_NAME_ENT_OFF);
	if (frte_name_ent == _FILE_NAME_INDEX) {
		strcpy(entry,"FNI");
		frte_nte_ind = buf->pick(_TYPE_NTEIndex,_FRTE_NTE_IND_OFF);
		mod_date  = buf->pick(_TYPE_long,_FRTE_MOD_DATE_OFF);
		name = get_nte_name(frte_nte_ind);
		if (name) strcpy(file_name,name);
		else file_name[0]=0;
		} 
	else if (frte_name_ent == _END_OF_LIST) {
		strcpy(entry,"EOL");
		file_name[0]=0;
		} 
	else {
		frte_off = buf->pick(_TYPE_long,_FRTE_FILE_OFF_OFF);
		frte_mte_ind = buf->pick(_TYPE_MTEIndex,_FRTE_MTE_IND_OFF);
		mte_name = get_mte_name(frte_mte_ind);
		}
	outstr("  %-3d %-4s %-10s", i,entry,name?name:"-none-");
	if (name && strlen(name)>10)
		outstr("\n                     ");
	outstr("  %-10s", mte_name?mte_name:"-none-");
	if (mte_name && strlen(mte_name)>10)
		outstr("\n                                 ");
	outstr(" %08lx    x%-8x    x%-5x\n",mod_date,frte_off,frte_mte_ind);
	}
	
void XsymDumper::dump_MTEs() {
	outstr("\n\nMODULES TABLE ENTRY (x%x objects)\n\n",_dshb.dshb_mte.dti_object_count);	
	outstr(" MTE   name      mname     RTE  resoff  size    kind      scope  parent frte_ind file_off imp_end    CMTE   CVTE  CLTE  CTTE  CSNTE1  CSNT2\n");
	dump_dealy(&_dshb.dshb_mte, _SIZEOF_MTE, &XsymDumper::dump_MTE);
	}

void XsymDumper::dump_MTE(BufEaters* buf, long i) {
	DBG_ASSERT(buf && buf->ptr());
	//_MTE_P mte = (_MTE_P) buf->ptr();
	if (i==0) {
		DBG_WARN(("skipping first MTE record\n"));
		DBG_DUMP_(XSYM,_SIZEOF_MTE,buf->ptr());
		return;
		}
	DBG_FIXME(("change to use buf->pick??\n"));
	uint32 nte_ind = buf->pick(_TYPE_NTEIndex,_MTE_NTE_IND_OFF);
	char*  name = get_nte_name(nte_ind);
	uint32 rte_ind = buf->pick(_TYPE_RTEIndex,_MTE_RTE_IND_OFF);
	uint32 res_off = buf->pick(_TYPE_long,_MTE_RES_OFF_OFF);
	uint32 size = buf->pick(_TYPE_long,_MTE_SIZE_OFF);
	uint32 kind = buf->pick(_TYPE_char,_MTE_KIND_OFF);
	uint32 scope = buf->pick(_TYPE_char,_MTE_SCOPE_OFF);
	uint32 cmte = buf->pick(_TYPE_CMTEIndex,_MTE_CMTE_IND_OFF);
	uint32 cvte = buf->pick(_TYPE_CVTEIndex,_MTE_CVTE_IND_OFF);
	uint32 clte = buf->pick(_TYPE_CLTEIndex,_MTE_CLTE_IND_OFF);
	uint32 ctte = buf->pick(_TYPE_CTTEIndex,_MTE_CTTE_IND_OFF);
	uint32 csnte1 = buf->pick(_TYPE_CSNTEIndex,_MTE_CSNTE_IND1_OFF);
	uint32 csnte2 = buf->pick(_TYPE_CSNTEIndex,_MTE_CSNTE_IND2_OFF);
	uint32 parent = buf->pick(_TYPE_MTEIndex,_MTE_PARENT_OFF);
	//_FREF* imp_fref = &mte->mte_imp_fref;
	uint32 frte_ind = buf->pick(_TYPE_FRTEIndex,_MTE_NTE_IND_OFF);
	uint32 file_off = buf->pick(_TYPE_long,_MTE_NTE_IND_OFF);
	uint32 imp_end =  buf->pick(_TYPE_long,_MTE_NTE_IND_OFF);
	char* mname = 0; //get_frte_mte_name(frte_ind);
	mname = get_mte_name(parent); //get modname from parent

	outstr("  %-3d %-10s",
			  i, name?name:"-none-");
	if (name && strlen(name)>10) 
		outstr("\n                ");
	outstr(" %-10s",
			  mname?mname:"");
	if (mname && strlen(mname)>10) 
		outstr("\n                           ");
	outstr(" %-4d x%-6x x%-5x %-8s  %-7s  %-3d   %-3d     x%-3x     x%-3x    %-3d  %-3d  %-3d  %-3d   %-3d   %-3d\n",
			  rte_ind, res_off, size, 
			  					parse_mte_kind_str(kind), 
			  						  parse_scope_str(scope), 
			  								parent, frte_ind, file_off, imp_end, cmte, cvte, clte, ctte, csnte1, csnte2);
	}
	
void XsymDumper::dump_CSNTEs() {
	outstr("\n\nCSNTEs: Contained Statement Table Entries (%d objects)\n\n",_dshb.dshb_csnte.dti_object_count);
	outstr("CSNTE entry mname     fdelta   mte   mte_offset frte_ind file_off\n");
	dump_dealy(&_dshb.dshb_csnte, _SIZEOF_CSNTE, &XsymDumper::dump_CSNTE);
	}

void XsymDumper::dump_CSNTE(BufEaters* buf, long i) {
	DBG_ASSERT(buf && buf->ptr());
	//_CSNTE* t=0;
	char entry[4]="";
	char* mname=0;
	uint32 frte_ind=0, file_off=0, fdelta=0, mte_ind=0, mte_off=0, entry_type=0;
	
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
			strcpy(entry,"SFC");
			frte_ind = buf->pick(_TYPE_FRTEIndex,_CSNTE_FREF_OFF+_FREF_FRTE_OFF);
			file_off = buf->pick(_TYPE_long,_CSNTE_FREF_OFF+_FREF_FOFF_OFF);
			src_off = file_off;
			mname = get_frte_mte_name(frte_ind);
			break;
		case _END_OF_LIST:
			strcpy(entry,"EOL");
			src_off = 0;
			break;
		default:
			fdelta = buf->pick(_TYPE_short,_CSNTE_FDELTA_OFF); //delta from previous src location
			mte_ind = buf->pick(_TYPE_MTEIndex,_CSNTE_MTE_IND_OFF); // mte index
			mte_off = buf->pick(_TYPE_MTEOffset,_CSNTE_MTE_OFF_OFF); //code offset into mte	
			code_off = mte_off + get_mte_start(mte_ind);
			src_off += fdelta;
		}
		
	outstr("   %-3d %-4s   %-10s",
			  i, entry, mname?mname:"-none-");
	if (mname && strlen(mname)>10) 
		outstr("\n                        ");
	outstr("  x%-5x   %-3d   x%-8x     %-4d    x%-5x\n", fdelta,mte_ind,mte_off,frte_ind,file_off);
	}

void XsymDumper::dump_TTEs() {	//variable length
	outstr("\n\nTTEs: Types Table Entries (%d objects)\n\n",_dshb.dshb_tte.dti_object_count);
	outstr("TTE tte_ind entry_size name       type_size nbytes type_bytes\n");
	//dump_dealy(&_dshb.dshb_tte, 0, &XsymDumper::dump_TTE);
	//dump_dealy(&_dshb.dshb_tte, _SIZEOF_DTTE, &XsymDumper::dump_TTE);
	uint32 nobjs = get_nobjs(&_dshb.dshb_tte);
	if (nobjs==0) {
		outstr("   ---no objects in table---\n");
		}
	else {
		uint32 len;
		BufEaters* buf;	//read whole table and parse thru it
		uint32 size = get_size(&_dshb.dshb_tte);
		if (buf=read_buf(get_off(&_dshb.dshb_tte),size),buf) {	//read whole thing
			for (long i=0; i<=nobjs; i++) {	//loop over objects in this page
				len = 0;	//don't know what length is
				//if (buf=read_obj(&_dshb.dshb_tte,_SIZEOF_DTTE,i),buf) 
				//_DTTE* tte=0;
				len = get_tte_size(buf,buf->pick(_TYPE_ushort,_DTTE_PSIZE_OFF));
				dump_TTE(buf,i);
				buf->eat_nbites(len);
				}
			SYM_DELETE(buf->buf());
			SYM_DELETE(buf);
			}
		}
	}
	
void XsymDumper::dump_TTE(BufEaters* buf, long i) {
	DBG_ASSERT(buf && buf->ptr());
	uint32 nte_ind;
	char entry[4]="";
	uint32 len=0,size=0,psize=0;
	unsigned char *type;
	char* name=0;
	char type_bytes[124]="0x";
	if (i==0) {
		DBG_WARN(("skipping first TTE record\n"));
		DBG_DUMP_(XSYM,buf->bites_left(),buf->ptr());
		return;
		}
	DBG_ASSERT(buf);
	//_DTTE* tte = (_DTTE*) buf->ptr();
	nte_ind = buf->pick(_TYPE_NTEIndex,_DTTE_NTE_IND_OFF);
	name = get_nte_name(nte_ind);
	psize = buf->pick(_TYPE_ushort,_DTTE_PSIZE_OFF); 
	size = min(get_tte_size(buf,psize),124);
	type = get_tte_bytes(buf,psize);
	psize &= 0x1ff;	//type size
	DBG_(XSYM,("type name = %s\n",name));
	DBG_(XSYM,("size=x%x, psize=x%x\n", 
		size,psize));
	DBG_DUMP_(XSYM,size,type);
	for (i=0; i<size; i++) {
		sprintf(type_bytes,"%s%02x",type_bytes,type[i]);
		}
	outstr("  %-3d   %-3d   %-10s",i,len,name?name:"-none-");
	if (name && strlen(name)>10) 
		outstr("\n                                   ");
	outstr(" x%-4x   %3d %s\n",psize,size,type_bytes);
	}

void XsymDumper::dump_CTTEs() {
	outstr("\n\nCTTEs: Contained Types Table Entries (%d objects)\n\n",_dshb.dshb_ctte.dti_object_count);
	outstr("CTTE entry ctte_ind fdelta name       mname     frte_ind file_off\n");
	dump_dealy(&_dshb.dshb_ctte, _SIZEOF_CTTE, &XsymDumper::dump_CTTE);
	}

void XsymDumper::dump_CTTE(BufEaters* buf, long i) {
	DBG_ASSERT(buf && buf->ptr());
	char entry[4]="";
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
		strcpy(entry,"EOL");
		}
	else if (ctte_type == _SOURCE_FILE_CHANGE) {
		strcpy(entry,"SFC");
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
	outstr("  %-3d   %-4s x%-5x x%-4x %-10s",i,entry,index, fdelta, name?name:"-none-");
	if (name && strlen(name)>10) 
		outstr("\n                                   ");
	outstr(" %-10s",
			  i, mname?mname:"-none-");
	if (mname && strlen(mname)>10) 
		outstr("\n                                             ");
	outstr(" x%-4x x%-8x\n",frte_ind,file_off);
	}


void XsymDumper::dump_CVTEs() {
	//CVTE: first element is source_file_change, 
	//		followed by list of variable CVTEs 
	//		then an end_of _list
	//		Keep reading until end of table
	outstr("\n\nCVTEs: Contained Variables Table Entries (%d objects)\n\n",_dshb.dshb_cvte.dti_object_count);
	outstr("CVTE entry   name     mname     TTE    scope  fdelta      offset    kind     class      big_la la_size la_addr    frte_ind file_off\n");
	dump_dealy(&_dshb.dshb_cvte, _SIZEOF_CVTE, &XsymDumper::dump_CVTE);
	}


void XsymDumper::dump_CVTE(BufEaters* buf, long i) {	
	DBG_ASSERT(buf && buf->ptr());
	//_CVTE_P t = (_CVTE_P)buf->ptr();
	char* name=0;
	char* mname=0;
	uint32 val=0;
	unsigned char la_addr[kCVTE_LA_MAX_SIZE]="";
	char entry[4]="";
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
			strcpy(entry,"SFC");
			DBG_(XSYM,("_SOURCE_FILE_CHANGE\n"));
			frte_ind = buf->pick(_TYPE_FRTEIndex,_CVTE_FILE_FR_OFF+_FREF_FRTE_OFF);
			file_off = buf->pick(_TYPE_long,_CVTE_FILE_FR_OFF+_FREF_FOFF_OFF);
			mname = get_frte_mte_name(frte_ind);
			break;
			}
		case _END_OF_LIST: {
			strcpy(entry,"EOL");
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
			//handle union entries separately
			//get location (char char long)
			if (la_size == 0) {
				strcpy(entry,"loc");
				kind = buf->pick(_TYPE_char,_CVTE_LOC_OFF+_SCA_KIND_OFF);
				sclass = buf->pick(_TYPE_char,_CVTE_LOC_OFF+_SCA_CLASS_OFF);
				offset = buf->pick(_TYPE_long,_CVTE_LOC_OFF+_SCA_OFFSET_OFF);
				val = offset;
				} 
			//get big address
			else if (la_size == kCVTE_BIG_LA) {
				strcpy(entry,"big");
				big_la = buf->pick(_TYPE_long,_CVTE_BLAS_LA_OFF);
				kind = buf->pick(_TYPE_char,_CVTE_BLAS_LAK_OFF);
				val = big_la;	//FIXME!!
				} 
			//get address
			else {
				strcpy(entry,"la");
				int i = 0;
				memcpy(&la_addr,buf->ptr()+_CVTE_LAS_LA_OFF,kCVTE_LA_MAX_SIZE);
				kind = buf->pick(_TYPE_char,_CVTE_LAS_LAK_OFF+kCVTE_LA_MAX_SIZE);
				val = get_symaddr(la_addr,la_size);
				}
			}
		}
	outstr("  %-3d %-4s %-10s", 
		i,entry,name?name:"-none-");
	if (name && strlen(name)>8) 
		outstr("\n                     ");
	outstr(" %-10s", 
		mname?mname:"-none-");
	if (mname && strlen(mname)>8) 
		outstr("\n                                ");
	outstr(" %-5d %-8s x%-8x x%-8x  %-8s  %-11s x%-3x  %-2d  %-8s x%-3x  x%-3x\n", 
		type,parse_scope_str(scope),fdelta,offset,parse_kind_str(kind),
		parse_class_str(sclass),big_la,la_size,parse_la_addr_str(la_size,
		la_addr),frte_ind,file_off);
	}
	

//===================================================================
// utilities

char* XsymDumper::parse_scope_str(int scope) { 	//scope is really a char
	switch (scope) {
		case SYMBOL_SCOPE_LOCAL:	return "local  ";
		case SYMBOL_SCOPE_GLOBAL:	return "global ";
		default:
			SET_ERR(se_unknown_type,("unknown scope x%x\n",scope));
			return "unknown";
		}
	}

char* XsymDumper::parse_class_str(int sclass) {	//class is really a char
			switch (sclass) {
			case STORAGE_CLASS_REGISTER:	return "register";
			case STORAGE_CLASS_A5:			return "A5relative";
			case STORAGE_CLASS_A6:			return "A6relative";
			case STORAGE_CLASS_A7:			return "A7relative";
			case STORAGE_CLASS_ABSOLUTE:	return "absolute";
			case STORAGE_CLASS_CONSTANT:	return "constant";
			case STORAGE_CLASS_RESOURCE:	return "resource";
			case STORAGE_CLASS_BIGCONSTANT:	return "bigconst";
			default:
				SET_ERR(se_unknown_type,("unknown class x%x\n",sclass));
				return "unknown";
			}
	}
		
char* XsymDumper::parse_kind_str(int kind) {	//kind is really a char
		switch (kind) {
			case STORAGE_KIND_LOCAL:		return "kind_local ";
			case STORAGE_KIND_VALUE:		return "kind_value ";
			case STORAGE_KIND_REFERENCE:	return "kind_ref   ";
			case STORAGE_KIND_WITH:			return "kind_with  ";
			default:
				SET_ERR(se_unknown_type,("unknown kind x%x\n",kind));
				return "unknown";
			}
	}
	
char* XsymDumper::parse_mte_kind_str(char mte_kind) {
	switch (mte_kind) {
		case MODULE_KIND_PROGRAM:	return "program";
		case MODULE_KIND_UNIT:		return "unit";
		case MODULE_KIND_PROCEDURE:	return "procedure";
		case MODULE_KIND_FUNCTION:	return "function";
		case MODULE_KIND_DATA:		return "data";
		case MODULE_KIND_BLOCK:		return "block";
		case MODULE_KIND_NONE:		return "none";
		default:
			SET_ERR(se_unknown_type,("unknown mte_kind\n"));
			return "unknown";
		}
	}
	
char* XsymDumper::parse_la_addr_str(uint32 la_size,unsigned char* la_addr) {
	static char la_addr_str[kCVTE_LA_MAX_SIZE*2]="";
	DBG_ASSERT(la_size<=kCVTE_LA_MAX_SIZE);
	if (la_size>kCVTE_LA_MAX_SIZE) la_size=kCVTE_LA_MAX_SIZE;
	if (la_size==0) la_addr_str[0]=0;
	else {
		strcpy(la_addr_str,"[");	//logical address bytes
		for (int i = 0; i < la_size; ++i) {
			sprintf(la_addr_str,"%s%02x ", la_addr_str, ((unsigned char)la_addr[i]));
			}
		strcat(la_addr_str,"]");
		}
	return la_addr_str;
	}
	
	
