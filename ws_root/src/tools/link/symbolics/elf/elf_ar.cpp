/*  @(#) elf_ar.cpp 96/07/25 1.21 */

//====================================================================
// ar.cpp  -  ArReader funcs for reading Archive libraries

#ifndef USE_DUMP_FILE
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "utils.h"
#include "elf_r.h"
#endif /* USE_DUMP_FILE */
#include "elf_ar.h"
#ifdef macintosh
	#include "cursorCtl.h"
#endif


#include "debug.h"

#pragma segment arsym
//==========================================================================
// class ArReader - read archive file

//-------------------------------------------
ArReader::ArReader(int dump)
//-------------------------------------------
{
	//_fp = 0;
	//_fp = SymNet::_fp;
	//_state = (SymError*) NEW(SymError);
	//_state = &(SymNet::_state);
    _symff = eAR;
    _dumping = dump;
    //_heap = SymNet::_heap;
}

//-------------------------------------------
ArReader::ArReader(SymFile* fp,SymError& state)
//-------------------------------------------
{
	//what about old _fp and _state?
	//we create _fp ourselves, so ok, but _state???
	//SOMEDAY.
	_fp = fp;
	_state = state;
    _symff = eAR;
    _dumping = 0;
	ISymbolics();
}

//-------------------------------------------
SymErr ArReader::ISymbolics(const StringPtr file_name)
//-------------------------------------------
{
    static char fn[255];
    cname((char*)fn,file_name);
	_fp = (SymFile*) SYM_NEW(SymFile(fn));
	_state.force_validate();
    ISymbolics();
	return state();
}

//-------------------------------------------
SymErr ArReader::ISymbolics()
//-------------------------------------------
{
		_nobjs = 0;
		_firstobj = 0;
		_foff = 0;
		_fsize = 0;
		if (!this) {
			DBG_ERR(("Invalid state\n"))
			return se_invalid_obj;
			}
		if (!_fp) {
			SET_ERR(se_parm_err,("Invalid file\n"));
			return _state.state();
			}
		busy_cursor();
		_fp->open();
    	char magic[SARMAG];
		if (!_fp->read(magic,SARMAG)) {
			SET_ERR(se_read_err,("# Unable to read lib header of file %s\n",_fp->filename()))
			return _state.state();
			}
		if (strncmp(magic,ARMAG,SARMAG)) {
			SET_ERR(se_unknown_type,("# Library %s is of unknown format\n",_fp->filename()))
			return _state.state();
			}
		_firstobj = _fp->tell();	//init to current fpos
		if (!_fp->seek(0,SEEK_END)) {
			SET_ERR(se_seek_err,("#Unable to seek to end of lib %s\n",_fp->filename()))
			return _state.state();
			}
		_fsize = _fp->tell();
		DBG_(ELF,("reading lib %s\n",_fp->filename()));
		//count archive members
		int i;
		ar_hdr* m;
		for (i=0, m=first(); i<2 && m; i++, m=next()) {
			//symtab has name '/'
			if (!strncmp(m->ar_name,"/               ",sizeof(m->ar_name))) {	
				_ar_syms._off = _foff+sizeof(_ar_hdr);	//past hdr & trail
				_ar_syms._size = get_size();
				_firstobj = _ar_syms._off+_ar_syms._size;
				continue;
				}
			//strtab has name '//'
			if (!strncmp(m->ar_name,"//              ",sizeof(m->ar_name))) {	
				_ar_strs._off = _foff+sizeof(_ar_hdr);	//past hdr & trail
				_ar_strs._size = get_size();
				_firstobj = _ar_strs._off+_ar_strs._size;
				continue;
				}
			}
		if (_ar_syms._off) read_symtab();
		else {
			SET_ERR(se_no_symbols,("No external symbols in archive file %s\n",_fp->filename()))
			}
		if (_ar_strs._off) read_strtab();
		return _state.state();
		}
		
ArReader::~ArReader() {
		if (_ar_syms._syms) DELETE_ARRAY(_ar_syms._syms);
		if (_ar_syms._strs) FREE(_ar_syms._strs);
		if (_ar_strs._buf) FREE(_ar_strs._buf);	
		if (_fp) _fp->close();
		_fp = 0;	//in case we were called from linker, don't let SymNet destroy!
		}
		
ar_hdr* ArReader::first() {
	_foff = _firstobj;	//init hdr pointer to first obj's ar hdr
	return read_arhdr(_foff);
	}
ar_hdr* ArReader::next() {
	_foff += get_size()+sizeof(_ar_hdr);	//advance past ar hdr & object
	return read_arhdr(_foff);
	}
ar_hdr* ArReader::read_arhdr(uint32 off) {
	busy_cursor();
	_foff = off;	//set current ar hdr pointer
	if (!_fp->seek(off,SEEK_SET)) {
		SET_ERR(se_seek_err,("#Unable to seek to 0x%X of file %s\n",_foff,_fp->filename()))
		return 0;
		}
	if (off+sizeof(ar_hdr) > _fsize) return 0;	//done!
	if (!_fp->read(&_ar_hdr,sizeof(ar_hdr))) {
		SET_ERR(se_read_err,("#Unable to read lib member header of file %s\n",_fp->filename()))
		return 0;
		}
	//DBG_(ELF,("read _ar_hdr=%s\n",&_ar_hdr));
	return &_ar_hdr;
	}
char* ArReader::get_name() {
	char* lname;
	uint32 moff;
	ar_hdr* a = &_ar_hdr;
	DBG_ASSERT(a->ar_name);
	if (!a->ar_name) {
			SET_ERR(se_fail,("no name for archive member!\n"));
			return 0;
			}
	if (a->ar_name[0]!='/') {	
		lname = a->ar_name;
		}
	else {			//get name from string table
		if (!_ar_strs._buf) {
			SET_ERR(se_fail,("no string table for archive file!\n"));
			return 0;
			}
		moff = strtoul(a->ar_name+1,0,10);
		lname = _ar_strs._buf+moff;
		DBG_ASSERT(lname && *lname);
		if (!(lname && *lname)) {
			SET_ERR(se_fail,("no name found in archive string table for index 0x%X!\n",moff));
			return 0;
			}
		}
	DBG_(ELF,("member name=%s\n",lname));
	char* p = strchr(lname,'/');	//names end with '/'
	if (p) *p=0;	
	return lname;
	}
uint32 ArReader::get_size() {
	uint32 msize;
	ar_hdr* a = &_ar_hdr;
	DBG_ASSERT(a->ar_size);
	if (!a->ar_size) {
		SET_ERR(se_fail,("no size for archive member!\n"));
		return 0;
		}
	msize = strtoul(a->ar_size,0,10);
	DBG_ASSERT(msize);
	return msize;
	}
//first member is archive symbol & string table
uint8* ArReader::read_obj(uint32 o_off, uint32 o_size) {
	busy_cursor();
	uint8* o=0;
	if (!o_size) {
		DBG_ERR(("object size of file %s is 0!!\n",_fp->filename()))
		return 0;
		}
	o = (uint8*) MALLOC(o_size+1);
	if (!o) {
		SET_ERR(se_malloc_err,("#Unable to create archive string table of file %s\n",_fp->filename()))
		return 0;
		}
	DBG_(ELF,("o_size=0x%X, o_off=0x%X\n",o_size,o_off));
	if (!_fp->seek(o_off,SEEK_SET)) {
		FREE(o);
		SET_ERR(se_seek_err,("#Unable to seek to archive string table of file %s\n",_fp->filename()))
		return 0;
		}
	if (!_fp->read(o,o_size)) {
		FREE(o);
		SET_ERR(se_read_err,("#Unable to read archive string table of file %s\n",_fp->filename()))
		return 0;
		}
	return o;
	}

//given library offset (from symbol table) get object offset, size, and name
Boolean ArReader::get_objinfo(uint32 symoff,char*& mname,uint32& moff,uint32& msize) {
	DBG_ENT("get_objinfo");
	_fp->set_base(0);	//make sure file base is set to 0
	ar_hdr* m=read_arhdr(symoff);
	DBG_ASSERT(m);
	if (!m) {
		SET_ERR(se_parm_err,("#Unable to read ar_hdr at offset 0x%X\n",symoff));
		return false;
		}
	DBG_(ELF,("lib member %s of size %s at offset 0x%X (ftell=x%X)\n"
		,m->ar_name,m->ar_size,_foff+sizeof(_ar_hdr),_fp->tell()));
	moff = obj_offset();	//file offset 
	msize = get_size();
	mname = get_name();
    return true;
	}

//external symbols are put in archive's symbol table
//all words are 4 bytes and are big endian
//contents:
//	nsums
//	array of offsets into archive file (4*nsyms)
//	name string table (ar_size-4*nsyms+1)
void ArReader::read_symtab() {
	DBG_ENT("read_symtab");
	int i;
	Endian e(BIG_ENDIAN);	//bytes are stored in big endian
	uint32 nsyms;	//number of external symbols in library
	uint32* symtab;	//symbol table of external symbols
	uint32 symtab_size;	//size of symbol table
	uint32 strtab_size;	//size of string table
	char* strtab;	//string table for symbol names
	DBG_ASSERT(_ar_syms._off);
	DBG_ASSERT(_ar_syms._size);
	if (!_fp->seek(_ar_syms._off,SEEK_SET)) {
		SET_ERR(se_seek_err,("#Unable to seek to symtab of file %s\n",_fp->filename()));
       	return;
       	}
	if (!_fp->read(&nsyms,4)) {
		SET_ERR(se_read_err,("#Unable to read nsyms of file %s\n",_fp->filename()));
       	return;
       	}
	nsyms = e.swapfix(nsyms);
	
	symtab_size = 4*(nsyms+1);
	symtab=(uint32*)MALLOC(4*nsyms);
	if (!symtab) {
       	SET_ERR(se_malloc_err,("#Unable to create symtab of file %s\n",_fp->filename()));
       	return;
       	}
	if (!_fp->read(symtab,4*nsyms)) {
		FREE(symtab);
		SET_ERR(se_read_err,("#Unable to read symtab of file %s\n",_fp->filename()));
       	return;
       	}
	for (i=0; i<nsyms; i++) {
		e.swapit(symtab[i]);
		}
	
	strtab_size = _ar_syms._size-symtab_size;
	strtab=(char*)MALLOC(strtab_size);
	if (!strtab) {
		FREE(symtab);
		SET_ERR(se_malloc_err,("#Unable to create strtab of file %s\n",_fp->filename()));
       	return;
       	}
	if (!_fp->read(strtab,strtab_size)) {
		FREE(strtab);
		FREE(symtab);
		SET_ERR(se_read_err,("#Unable to read strtab of file %s\n",_fp->filename()));
       	return;
       	}
	
	//setup symbol table
	Ar_sym* syms = (Ar_sym*) NEW(Ar_sym[nsyms]);
	char* p = strtab;
	DBG_(ELF,("reading symtab of library %s (%d symbols)\n",_fp->filename(),nsyms));
	DBG_ASSERT(syms);
	DBG_ASSERT(p);
	if (!syms || !p) {
		SET_ERR(se_malloc_err,("#Unable to create syms or invalid strtab\n"));
		return;
		}

	busy_cursor();
	for (i=0; i<nsyms; i++) {
		syms[i]._name = p;	//name of symbol
		syms[i]._off = symtab[i];	//offset of symbol within library
		DBG_(ELF,("lib symbol %d %s at offset 0x%X\n",i,p,symtab[i]));
		p += strlen(p)+1;
		}

	//setup structure values
	_ar_syms._syms = syms;	//must delete later
	_ar_syms._nsyms = nsyms;
	_ar_syms._strs = strtab;	//must delete later
	_ar_syms._nstrs = strtab_size;
	FREE(symtab);	//don't need anymore
	}
//find which object in the library off falls within
int ArReader::objind(uint32 off) {
	int i; ar_hdr* m;
	uint32 moff, msize;
	//first member too high - start from first obj?
	//fix to be more efficient - don't read whole file everytime
	//fix symbol's strings 
	//start at 1 so 0 can be null
	for (i=1, m=first(); m; m=next(), i++) {
		DBG_(ELF,("lib member %s of size 0x%X at offset 0x%X\n",m->ar_name,m->ar_size,_fp->tell()));
		//moff = _fp->tell()-_firstobj;	//offset from first object
		moff = offset();	//file offset of member (including header)
		msize = get_size();
		if (off >= moff && off <= moff+msize) {
			return i;
			}
		}
	return 0;
	}
//read archive string table
void ArReader::read_strtab() {
	uint8* buf=read_obj(_ar_strs._off,_ar_strs._size);
	if (!buf) {
		DBG_WARN(("file %s has no archive string table or unable to read\n",_fp->filename()))
		return;
		}
	_ar_strs._buf=(char*)buf;	//must delete later
	}

