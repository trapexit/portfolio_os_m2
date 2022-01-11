
//NOTE: since XsymDumper_v3r2_ and XsymDumper_ are practically the same,
//we include this file for XsymDumper_ and put #ifdefs around code
//specific to XsymDumper_

#include "xsym_verdefs.h"


//figure out if this version of this file has already been included or not
#undef __ALREADY_INCLUDED__
#if __XSYM_VER__ == __XSYM_V31__
	#ifndef __XSYM_D_v31_H__
		#define __XSYM_D_v31_H__
		#include "xsym_r_v3r1.h"
		#undef __ALREADY_INCLUDED__
		#define __XSYM_VER__ __XSYM_V31__
	#else
		#define __ALREADY_INCLUDED__
	#endif
#elif __XSYM_VER__ == __XSYM_V32__
	#ifndef __XSYM_D_v32_H__
		#define __XSYM_D_v32_H__
		#include "xsym_r_v3r2.h"
		#undef __ALREADY_INCLUDED__
		#define __XSYM_VER__ __XSYM_V32__
	#else
		#define __ALREADY_INCLUDED__
	#endif
#elif __XSYM_VER__ == __XSYM_V34__
	#ifndef __XSYM_D_v34_H__
		#define __XSYM_D_v34_H__
		#include "xsym_r_v3r4.h"
		#undef __ALREADY_INCLUDED__
		#define __XSYM_VER__ __XSYM_V34__
	#else
		#define __ALREADY_INCLUDED__
	#endif
#else 	//v33 default!!!
	#ifndef __XSYM_D_v33_H__
		#define __XSYM_D_v33_H__
		#undef __XSYM_VER__ 
		#define __XSYM_VER__ __XSYM_V33__
		#include "xsym_r_v3r3.h"
		#undef __ALREADY_INCLUDED__
		#define __XSYM_VER__ __XSYM_V33__
	#else
		#define __ALREADY_INCLUDED__
	#endif
#endif 


#ifndef __ALREADY_INCLUDED__	

#include DEF_FILE
#include "dumputils.h"
#include "dumpopts.h"

class XsymDumper : public XsymReader, Outstrstuffs  {

public: // contract member functions for ABC
	XsymDumper(char *name, FILE* dump_fp, GetOpts* opts,int dump=DUMP_ALL);  
	virtual ~XsymDumper() {};
	virtual void DumpFile();
		
	typedef void (XsymDumper::*dump_func_t)(BufEaters*,long);	//typedef for symbol compares
	void dump_name_MTE(_MTEIndex);
	void dump_FRTEs();
	void dump_FRTE(BufEaters*,long);
	void dump_RTEs();
	void dump_RTE(BufEaters*,long);
	void dump_MTEs();
	void dump_MTE(BufEaters*,long);
	void dump_name_FRTE(_FRTEIndex, unsigned long);
	void dump_CSNTEs();
	void dump_CSNTE(BufEaters*,long);
	void dump_CVTEs();
	void dump_CVTE(BufEaters* buf,long);
	void dump_CTTEs();
	void dump_CTTE(BufEaters*,long);
	void dump_TTEs();
	void dump_TTE(BufEaters*,long);
	void dump_DTIs();
	void dump_DTI(char* s,_DTI* dti);
	void dump_dealy(_DTI* dti, uint32 obj_size, dump_func_t func);
	
private:
	GetOpts* _dump_opts;
	char* parse_scope_str(int scope);
	char* parse_class_str(int sclass);
	char* parse_kind_str(int kind);
	char* parse_mte_kind_str(char mte_kind);
	static char* parse_la_addr_str(uint32 la_size,unsigned char* la_addr);
};

#undef __XSYM_VER__
#endif /* __ALREADY_INCLUDED__ */


