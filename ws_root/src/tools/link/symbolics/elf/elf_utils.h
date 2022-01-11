/*  @(#) elf_utils.h 96/09/23 1.27 */

//====================================================================
// elf_utils.h  -  utility classes for ElfReader 
//

#ifndef __ELF_UTILS_H__
#define __ELF_UTILS_H__

#include <stdio.h>
#include "parse.h"
#include "elf_r.h"
#include "elf_3do.h"

#define F_OBJ 1
#define F_LIB 0
#define F_DLL 2 

inline Boolean isglob(Elf32_Sym* sym) { 
	return (Boolean)(ELF32_ST_BIND((sym)->st_info)==STB_GLOBAL);
	}
inline HashOff scopeid(Elf32_Sym* sym,int objind) { 
	return (isglob(sym))
		? hoff_global
		: (HashOff) (objind+1);
	}


struct _linkhash {
	const char* _name;
	int _symidx;
	int _objind;
	int _newsymidx;
	};
struct LinkHash : Hash {
	LinkHash(int size) : Hash(size) {}
	~LinkHash() {
		for (void* p=first_hash(); p; p=next_hash()) {
			_linkhash* l = (_linkhash*) p;
			Hash::rmv_hash();
			if (l) DELETE(l);
			}
		}
	//returns struct of conflicting hash if any
	//add_hash uses pointers to name, so must make sure
	//we use name from object's _symtab
	_linkhash* add_hash(const char* name, HashOff scopeid, int symidx, int objind) {
		_linkhash* l;
		l = (_linkhash*) NEW(_linkhash);
		l->_name = name;
		l->_symidx = symidx;
		l->_objind = objind;
		l->_newsymidx = 0;
		Hash::add_hash(name,(void*)l,scopeid);
		return l;
		}
	_linkhash* get_hash(const char* name,HashOff scopeid) {
		return (_linkhash*) Hash::get_hash(name,scopeid);
		}
	int rmv_hash(const char* name,HashOff scopeid) {
		_linkhash* l;
		l = (_linkhash*) Hash::set_hash(name,0,scopeid);	//delete reference from hash table
		if (l) DELETE(l);
		return l?l->_newsymidx:0;
		}
	_linkhash* update_hash(const char* name, HashOff scopeid, int newsymidx) {
		_linkhash* l;
		l = (_linkhash*) Hash::get_hash(name,scopeid);
		l->_newsymidx = newsymidx;
		return l;
		}
	};


class DefParse : public Parse {
public:
	enum DefToken {
		def_unknown,
		def_equals,		// =
		def_dot,		// .
		def_fwdslash,		// /
		def_colon,		// :
		def_comment,	// # or !
		def_number,
		def_symbol,
		def_imports,	// IMPORTS
		def_import_now,	// IMPORT_NOW
		def_import_on_demand,	// IMPORT_ON_DEMAND
		def_reimport_allowed,	// REIMPORT_ALLOWED
		def_import_flag,	// IMPORT_FLAG
		def_exports,	// EXPORTS
		def_modnum,		// MAGIC or MODULE
		def_file,		// path/file.ext
		def_eof			// EOF
		};
	DefToken _token;
	DefParse(const char* buf,int32 sz, const char* file) : Parse(buf, sz, file) { _token = def_unknown; }
	//DefToken get_token();	//inherit Parse's
	DefToken get_token();	
	DefToken next_token();	
	};

struct ObjInfo;
struct DllInfo;
struct LibInfo;
//======================================
class Syms
//======================================
{
	friend class ElfLinker;
	class Sym
	{
	public:
		Sym(Sym*& treetop, const char* _name, uint32 i, int o, int ord);
		~Sym();
		Sym* next() const;
	private:
		void unlink(Sym*& treetop) const;
	private:
		friend class Syms;
		Sym *_ltree, *_rtree;
		Sym* _mom;
	public:
		char* _name;
		uint32 _symidx;
		int _objind;	//index of obj where undefined
		int _ordinal;	//ordinal (if import/export)
	}; // class Syms::Sym
public:
	Syms() : _tag(0), _syms(0), _nsyms(0) {}
	~Syms();
	void add(const char* name, uint32 i, int o, int ord = 0);
	void rmv(Syms::Sym* s);
	Sym* find(const char* name) const;
	Sym* first() const;

	Sym* _syms;
	int _nsyms;
	uint32 _tag;
		// _tag can be any additional information that
		// we want to keep about this list of symbols
}; // class Syms

//======================================
class FileInfo
//======================================
{
	friend class ElfLinker;
	static FileInfo* _files;
	static FileInfo* _lastfile;
	static int _nfiles;
	int _type;
	FileInfo* _chain;
	SymFile* _fp;
public:
	union { 
		ObjInfo* o;
//#define OLD_DLL
#ifndef OLD_DLL
		DllInfo* d;
#endif
		LibInfo* l;
		} _info;
	FileInfo(const char* fname, int type=F_OBJ);
	~FileInfo();
	static FileInfo* first() { return _files; }
	FileInfo* next() { return _chain; }
	};
class ObjInfo {
	friend class ElfLinker;
	static ObjInfo* _objs;
	static ObjInfo* _lastobj;
	static int _nobjs;
	ObjInfo* _mom;
	ObjInfo* _chain;
public:
	int _libind;
	int _objind;
	char* _name;
	//stuff for each file from SymNet:
	SymFile* _fp;
	uint32 _base;	//file offset base (in case in a library)
	Sections* _sections;
	//stuff for each file from ElfReader:
	char *_sh_strtab;
	char *_strtab; 
	Elf32_Ehdr *_elf_hdr;
	Elf32_Shdr *_sec_hdr;
	Elf32_Sym *_symtab; 
	ObjInfo(SymFile* f, uint32 base, int l=0);
	~ObjInfo();
	void delete_objs();
	static ObjInfo* first(int& i) { i=(_objs?_objs->_objind:-1); return _objs; }
	static ObjInfo* first() { return _objs; }
	ObjInfo* next(int& i) { i=(_chain?_chain->_objind:-1); return _chain; }
	ObjInfo* next() { return _chain; }
	static ObjInfo* find(int i);
	static ObjInfo* find(const char* n);
}; // class FileInfo

class LibInfo {
	friend class ElfLinker;
	static LibInfo* _libs;
	static int _nlibs;
	LibInfo* _mom;
	LibInfo* _chain;
public:
	enum Used {
		not_used=0,
		need_to_add,
		used
		};
	struct Objs {
		Used _used;	//did we use this obj?
		uint32 _offset;	//offset within lib
		uint32 _size;	//size of member
		char* _name;	//object name
		ObjInfo* _obj;
		};
	int _libind;
	SymFile* _fp;
	Objs* _objs;
	int _nobjs;			//nobjs in library
	LinkHash* _hashtab;		//library's hash table
	//archive symbol & string table
	ArReader* _ar;
	//methods
	LibInfo(SymFile* f);
	~LibInfo();
	void delete_libs();
	static LibInfo* first(int& i) { i=(_libs?_libs->_libind:-1); return _libs; }
	static LibInfo* first() { return _libs; }
	LibInfo* next(int& i) { i=(_chain?_chain->_libind:-1); return _chain; }
	LibInfo* next() { return _chain; }
	static LibInfo* find(int i);
	};
class DllInfo {
	friend class ElfLinker;
	static DllInfo* _dlls;
	static int _ndlls;
	DllInfo* _mom;
	DllInfo* _chain;
public:
	int _dllind;
	int _impind;	//index into .imp3do for this entry
	char* _name;
	int _ord;		//ordinal/magic number for module
	uint32 _ver;	//version
	uint32 _rev;	//revision
	uint32 _flags;
	SymFile* _fp;
#ifndef OLD_DLL
	ObjInfo* _o;
#endif
	//methods
#ifndef OLD_DLL
	DllInfo(SymFile* f,ObjInfo* o);
	Boolean _used;
#else
	DllInfo(SymFile* f);
#endif
	~DllInfo();
	void delete_dlls();
	static DllInfo* first() { return _dlls; }
	DllInfo* next() { return _chain; }
	static DllInfo* find(int i);
	static DllInfo* find(const char* n);
	};
    	
#endif /* __ELF_UTILS_H__ */
