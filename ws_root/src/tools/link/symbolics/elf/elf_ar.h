// @(#) elf_ar.h 96/07/25 1.5


//====================================================================
// elf_ar.h  -  ArReader class defs for reading unix archive files 
//

#ifndef __ELF_AR_H__
#define __ELF_AR_H__

#ifndef USE_DUMP_FILE
#include "predefines.h"
#include "symnet.h"
#include "symapi.h"
#include "utils.h"
#endif
#include "elf.h" //elf file format
#include "ar.h"	 //archive file format

//==========================================================================
//defines for class ArReader
//		archive reader
class ArReader : public SymNet {
public:
#if defined(__NEW_ARMSYM__) 
    ArReader(int dump=NO_DUMP);
    virtual SymErr ISymbolics(const StringPtr fname);
    virtual SymErr ISymbolics();
#else
    ArReader(const char *name, int dump=0);
#endif
	ArReader(SymFile* fp,SymError& state);
	virtual ~ArReader();

	//archive symbol & string table
	struct Ar_sym {
		uint32 _off;
//DELETEME!!!  getting rid of _objind
#ifdef OLDLIB
		int _objind;
#endif
		char* _name;
		};
	struct Ar_syms { 
		uint32 _off;	
		uint32 _size;	
		Ar_sym* _syms;	//library's master symbol table
		int _nsyms;
		char* _strs;
		uint32 _nstrs;
		Ar_syms() {
			_off=0;	
			_size=0;	
			_syms=0;
			_nsyms=0;
			_strs=0;
			_nstrs=0;
			}
		};
protected:
	struct Ar_strs { 
		char* _buf;			//library's string table
		uint32 _off;	
		uint32 _size;	
		Ar_strs() {
			_buf=0;	
			_off=0;	
			_size=0;	
			}
		};
	ar_hdr _ar_hdr;
	uint32 _foff;
	uint32 _fsize;
	int _nobjs;
	//overrides of SymNet's _fp and _state
	//SymFile* _fp;
	//SymError* _state;
private:
	uint32 _firstobj;
	ar_hdr* read_arhdr(uint32 off);
	void read_symtab();
	void read_strtab();
	uint8* read_obj(uint32 o_off, uint32 o_size);
	int objind(uint32 off);
public:
	Ar_syms _ar_syms;
	Ar_strs _ar_strs;
	uint32 obj_offset() { return _foff+sizeof(ar_hdr); }	//offset of object
	uint32 offset() { return _foff; }	//offset of header 
	int nobjs() { return _nobjs; }
	char* get_name();
	uint32 get_size();
#ifndef OLDLIB
	Boolean get_objinfo(uint32 symoff, char*& mname,uint32& moff,uint32& msize);
#endif
	ar_hdr* first();
	ar_hdr* next();
	};
	
#endif /* __ELF_AR_H__ */

