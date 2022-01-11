/*  @(#) symutils.h 96/07/25 1.24 */

//====================================================================
// symutils.h  -  Utility classes used by SymNet
//

#ifndef __SYMUTILS_H__
#define __SYMUTILS_H__

#include <stdlib.h>
#include "predefines.h"
#include "symapi.h"
#include "symnodes.h"
#include "loaderty.h"
#include "utils.h"

#ifdef macintosh
#include <Files.h>
#endif

//====================================================================
// class for sections and relocation

class Sections {
		int _nsecs;
		unsigned long *_baseaddr;
		unsigned long *_size;
		int *_type;	//type is SEC_DATA, SEC_CODE, etc...
		int *_type2sec;
	public:
		Sections(int nsecs);
		~Sections();
		void set_baseaddr(SymSec sectype, int secnum, unsigned long addr);
		void set_size(int secnum, unsigned long size);
		unsigned long baseaddr(int secnum);
		unsigned long size(int secnum);
		SymSec sectype(int secnum);
		int secnum(SymSec sectype);
		int nsecs() { return _nsecs; }
		};
		
//====================================================================
// structs/classes for resolving forward references
//		these link the types read from the file image
//		to their symbol types built in the symbol network
//		for resolving forward references
//when a user type referenced, return type as elf_offset -
//we'll resolve them later when we're done

typedef uint32 Fwdref; //so can cast anything to here and compiler is happy
typedef uint32 Fwdrefid; //so can cast anything to here and compiler is happy
//SOMEDAY: want to change derived classes to new this so it doesn't have to live forever!!
class SymRefType {
	friend class SymFwdrefTypes;
	SymRefType* _chain; //next ref in list
	Fwdref _typeref;	//id for matching with file_offset
	SymType* _symtype;	//ptr to symtype which needs to have it's ref fixed later
	void set_next(SymRefType* rt) { _chain=rt; }
public:
	INLINE SymRefType(Fwdref file_offset,SymType* st);
	SymRefType* next() { return _chain; }
	void resolve(void* rt) { _symtype->set_type(rt); }
	Fwdref typeref() { return _typeref; }
	};

class SymFwdrefType {
	friend class SymFwdrefTypes;
private:
	//some file formats which don't use an id
	//SOMEDAY!! can delete _id and just use _id in SymRefType instead...
	Fwdrefid _id;  //id used by file format for referencing types
	Fwdref _typeref; //reference for type (ptr/idx/type/whatever)
	SymType* _symtype; //symnet type
	SymFwdrefType* _chain;
	void set_id(Fwdrefid id) { _id = id; }
	Fwdrefid id() { return _id; }
	void set_typeref(Fwdref t) { _typeref = t; }
	SymFwdrefType* next() { return _chain; }
	void set_next(SymFwdrefType* rt) { _chain=rt; }
public:     
	INLINE SymFwdrefType(Fwdrefid id,Fwdref t);
	Fwdref typeref() { return _typeref; }
	SymType* symtype() { return _symtype; }
	void set_symtype(SymType* t) { _symtype=t; }
	};
     
class SymFwdrefTypes {
private:
	SymFwdrefType* _fwdref_head;
	SymRefType* _ref_head;
	uint32 _ref_type_count;
	uint32 _fwdref_type_count;
public:
   SymFwdrefTypes() {
      _fwdref_head = 0;
      _ref_head = 0;
	  _ref_type_count = 0;
	  _fwdref_type_count = 0;
      }
   ~SymFwdrefTypes();
	SymFwdrefType* search_fwdref_ids(Fwdrefid id);
	SymFwdrefType* search_fwdref_types(Fwdref file_offset, SymFwdrefType** ref_table);
	SymFwdrefType* add_fwdref_type(Fwdrefid id,Fwdref t);
	SymType* add_ref_type(Fwdref file_offset,SymType* type);
	SymType* get_ref_type(Fwdref file_offset,SymRoot* symroot);
	SymFwdrefType** build_ref_types_table();
	void cleanup_ref_types(SymFwdrefType** ref_table);
	//time to resolve the list of fwdrefs with refs...
	Boolean resolve_ref_types();
   };
   
//====================================================================
// SymNetScope
//		these hold pointers to the current nodes in the symbol network
//		for module, function and block

class SymNetScope {
	SymFuncEntry* _func;
	SymFuncEntry* _block;
	SymModEntry* _mod;
public:
	SymNetScope(SymModEntry* m=0, SymFuncEntry* f=0, SymFuncEntry* b=0) { _mod=m, _func=f, _block=b; }
	void init_scope() { _mod=0, _func=0, _block=0; }
	void set_mod(SymModEntry* m=0) { _mod=m; }
	void set_func(SymFuncEntry* f=0) { _func=f; }
	void set_block(SymFuncEntry* b=0) { _block=b; }
	SymModEntry* mod() { return _mod; }
	SymFuncEntry* func() { return _func; }
	SymFuncEntry* block() { return _block; }
	SymNetScope& operator = (SymNetScope& s) { _func=s._func; _block=s._block; _mod=s._mod; return *this; }
	};

#define MAX_MSG 1024
//======================================
class SymError
//======================================
{
friend class SymNet;
friend class SymDump;
friend class SymRoot;
friend class CharOffs;
friend class ArReader;
friend class ElfReader;
friend class ElfLinker;
friend class XcoffReader;
friend class XsymReader_v32;
friend class XsymReader_v33;
friend class XsymReader_v34;
friend class SymReader;
	State _s;
	char _msg[MAX_MSG];
	void set_state(SymErr err, const char* file=0, uint32 line=0);
public:
	SymError() {
		_s.set_state(se_invalid_obj);
		}
	SymErr state(char*& fname, uint32& line) { return (SymErr)_s.state(fname,line); }
	SymErr state() { return (SymErr)_s.state(); }
	static Boolean failed(SymErr e) { 
    	return (Boolean)((((uint32)e)&SE_FATAL_ERR)!=0); // returns false if e is a fatal error
		}
	Boolean valid() { 
    	return (Boolean)(!failed((SymErr)_s.state())); // returns true if state is valid
		}
	SymErr validate() { if (valid()) _s.set_state(se_success); return (SymErr)_s.state(); }
	void force_validate() { _s.set_state(se_success); }
	char* get_err_name();
	char* get_err_msg();
	void set_err_msg(const char* fmt, ...);
	void set_err_msg(const char* fmt, va_list ap);
}; // class SymError


	
#ifdef macintosh
#define SYM_BUFFER_SIZE 16384
#endif

//======================================
class SymFile
//======================================
{
	static int _nfilesopen;
	static SymFile* _files;
#ifdef macintosh
	short fRefNum;
//	Ptr _buffer;
#else
	FILE* _fp;	//file ptr
#endif
	uint32 _base;	//file base
	uint32 _offset;	//file offset from base
	char* _name;
	SymFile* _chain;
public:
	SymFile(const char* fname);
	~SymFile();
	const char* filename() { return _name; }
	SymFile* next() { return _chain; }
	Boolean open();
	void close();
#ifdef macintosh
	uint32 tell()
		{ int32 result;  GetFPos(fRefNum, &result); return result; }
#else
	uint32 tell() { return ftell(_fp); }
	FILE* fp() { return _fp; }
#endif
	Boolean test();
	void set_base(uint32 base) { _base = base; }
#ifndef macintosh
	INLINE
#endif
	Boolean seek(int32 off, uint32 base_kind);
	INLINE Boolean read(void* buf, uint32 size);
}; // class SymFile
		
//==========================================================================
//  line number <==> address mapping


class CharOffs {
private:
	FILE *_sfp;
	uint32 *_ln2c;
	unsigned long _maxlns, _maxchs;
public:
	CharOffs(char *fname, char *fpath);
	~CharOffs();
	unsigned long ln2ch(unsigned long ln);
	unsigned long ch2ln(unsigned long ch);
	};


#endif /* __SYMUTILS_H__ */
