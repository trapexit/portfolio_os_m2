/*  @(#) sym_utils.cpp 96/07/25 1.31 */

//==========================================================================
// sym_utils.cpp	- definitions for SymNet class
//				  and other helper classes
//
// the idea here is to keep one network for searching etc,
// and just have specialized classes for reading the symbols
// into the symbol network.

#ifndef USE_DUMP_FILE
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "predefines.h"
//#include "SymbolsUtil.h"
#include "utils.h"
#include "symnet.h"
#endif /* USE_DUMP_FILE */

#ifdef macintosh
#include <Memory.h>
#include <TextUtils.h>
#endif

#include "debug.h"

#ifdef __3DO_DEBUGGER__
#define kProgressString1 "\pResolving Types"
#define kProgressString2 "\pDelete Forward References, Delete Nodes"
#define kProgressString3 "\pDeleting Forward References"
#elif defined(macintosh)
#define kProgressString1 "\p"
#define kProgressString2 "\p"
#define kProgressString3 "\p"
#else
#define kProgressString1 ""
#define kProgressString2 ""
#define kProgressString3 ""
#endif


#pragma segment symnet

//====================================================================
// class for errors

void SymError::set_state(SymErr err, const char* file, uint32 line) {
	if (valid()) {	//don't change if object is already invalid
		DBG_ASSERT(file);
		_s.set_state(err,file,line);
		}
    }
    
char* SymError::get_err_name() {
	switch (state()) {
		case se_success: return "success";
	//informational return codes
		case se_not_found: return "NOT_FOUND";
		case se_exact_match: return "EXACT_MATCH";
		case se_approx: return "APPROX";
		case se_no_source: return "NO_SOURCE";
		case se_no_debug_info: return "NO_DEBUG_INFO";
		case se_no_line_info: return "NO_LINE_INFO";
		case se_no_symbols: return "NO_SYMBOLS";
		case se_und_symbols: return "UND_SYMBOLS";
		case se_dup_symbols: return "DUP_SYMBOLS";
	//non-fatal errors
		case se_open_fail: return "OPEN_FAIL";
		case se_unknown_format: return "UNKNOWN_FORMAT";
		case se_unknown_type: return "UNKNOWN_TYPE";
		case se_bad_call: return "BAD_CALL";
		case se_parm_err: return "PARM_ERR";
		case se_scope_err: return "SCOPE_ERR";
	//fatal errors
		case se_fail: return "FAIL";
		case se_inv_symbols: return "INV_SYMBOLS";
		case se_malloc_err: return "MALLOC_ERR";
		case se_seek_err: return "SEEK_ERR";
		case se_read_err: return "READ_ERR";
		case se_invalid_obj: return "INVALID_OBJ";
		case se_wrong_version: return "WRONG_VERSION";
		default:
			return "UNKNOWN ERROR";
		}
	}
	
//strings for public consumption
void SymError::set_err_msg(const char* fmt, ...) {
		va_list ap;
		va_start(ap, fmt);
		vsprintf(_msg, fmt, ap);
		va_end(ap);
	}

//strings for public consumption
void SymError::set_err_msg(const char* fmt, va_list ap) {
	vsprintf(_msg, fmt, ap);
	}

//strings for public consumption
char* SymError::get_err_msg() {
	char *s;
	#ifdef DEBUG
	char* file;
	uint32 line;
	_s.state(file,line);
	#endif
	switch (state()) {
		case se_not_found:
		case se_exact_match:
		case se_approx:
			s = 0;
			break;
		case se_no_source:
			s = "no source found for file";
			break;
		case se_no_debug_info:
			s = "no debug info found for file";
			break;
		case se_no_line_info:
			s = "no line info found for file";
			break;
		case se_no_symbols:
			s = "no symbols found for file";
			break;
	//non-fatal errors
		case se_open_fail:
			s = "# Unable to open symbolics file";
			break;
		case se_unknown_type:
			s = "unknown type";
			break;
		case se_bad_call:
			s = "bad call to function detected";
			break;
		case se_parm_err:
			s = "parameter error";
			break;
		case se_scope_err:
			s = "scope error";
			break;
	//fatal errors
		case se_fail:
			s = "failure";
			break;
		case se_inv_symbols:
			s = "symbol table not valid";
			break;
		case se_malloc_err:
			s = "out of memory or heap allocation error";
			break;
		case se_seek_err:
			s = "seek error while reading file";
			break;
		case se_read_err:
			s = "read error while reading file";
			break;
		case se_invalid_obj:
			s = "invalid symbolics object";
			break;
		case se_wrong_version:
			s = "file has unknown version";
			break;
		case se_unknown_format:
			s = "file is of unknown format";
			break;
		default:
			s = "error";
		}
	#ifdef DEBUG
		_s.state(file,line);
		return s ? Strstuffs::fmt_str(
			"%s: \t-detected by module %s at line %d\n",s,_msg,file,line)
			: 0;
	#else
		return s?s:"";
	#endif
	}

//====================================================================
// class for sections and relocation

Sections::Sections(int nsecs) { 
			int i;
			_nsecs = nsecs; 
			_baseaddr = (uint32*)MALLOC(_nsecs*sizeof(uint32));
			if (!_baseaddr) {
				DBG_ERR(("# Unable to create _baseaddr\n"));
				return;
				}
			_size = (uint32*)MALLOC(_nsecs*sizeof(uint32));
			if (!_size) {
				DBG_ERR(("# Unable to create _size\n"));
				return;
				}
			_type = (int*)MALLOC(_nsecs*sizeof(int));
			if (!_type) {
				DBG_ERR(("# Unable to create _type\n"));
				return;
				}
			_type2sec = (int*)MALLOC(SEC_NUM*sizeof(int));//SOMEDAY!! may not be a 1-1 mapping.
			if (!_type2sec) {
				DBG_ERR(("# Unable to create _type2sec\n"));
				return;
				}
			for (i=0; i<nsecs; i++) {
				_type[i]=0;
				_baseaddr[i]=0;
				_size[i]=0;
				}
			for (i=0; i<SEC_NUM; i++) {
				_type2sec[i]=0;
				}
			}
Sections::~Sections() {
			FREE(_baseaddr);
			FREE(_size);
			FREE(_type);
			FREE(_type2sec);
			}
void Sections::set_baseaddr(SymSec sectype, int secnum, unsigned long addr) {
			DBG_ASSERT(_baseaddr);
			DBG_ASSERT(_type);
			DBG_ASSERT(_type2sec);
			DBG_ASSERT((secnum>=0 && secnum<_nsecs));
			DBG_ASSERT((sectype>=sec_none && sectype<sec_num));
			if (_type2sec[sectype]) {
				DBG_ASSERT(_type2sec[sectype]==secnum);	//shouldn't change type
				}
			else
				_type2sec[sectype] = secnum;
			_baseaddr[secnum] = addr;
			_type[secnum] = sectype;
			DBG_(SYMNET,("Sections(sectype=x%X, secnum=%d, addr=x%X): set _baseaddr=x%X, _type=x%X\n",sectype,secnum,addr,_baseaddr[secnum],_type[secnum]));
			}
unsigned long Sections::baseaddr(int secnum) {
			DBG_ASSERT(_baseaddr);
			if (!(secnum>=0 && secnum<_nsecs)) {
				DBG_ERR(("Section number %d is out of range!\n",secnum));
				DBG_ASSERT((secnum>=0 && secnum<_nsecs));
				return 0;
				}
			return _baseaddr[secnum];
			}
void Sections::set_size(int secnum, unsigned long size) {
			DBG_ASSERT(_size);
			DBG_ASSERT((secnum>=0 && secnum<_nsecs));
			_size[secnum] = size;
			}
unsigned long Sections::size(int secnum) {
			DBG_ASSERT(_size);
			DBG_ASSERT((secnum>=0 && secnum<_nsecs));
			return _size[secnum];
			}
SymSec Sections::sectype(int secnum) {
			DBG_ASSERT(_type);
			DBG_ASSERT((secnum>=0 && secnum<_nsecs));
			return (SymSec)_type[secnum];
			}
		//SOMEDAY!! may not be a 1-1 mapping.
int Sections::secnum(SymSec sectype) {
			DBG_ASSERT(_type2sec);
			DBG_ASSERT((sectype>=sec_none && sectype<sec_num));
			return _type2sec[sectype];
			}
			
//====================================================================
// files
#ifndef FOPEN_MAX
#define FOPEN_MAX 60
#endif
int SymFile::_nfilesopen=0; 
SymFile* SymFile::_files=0; 
//-------------------------------------------
SymFile::SymFile(const char* fname)
#ifdef macintosh
:	fRefNum(0)
//:	_buffer(NULL)
#else
:	_fp(NULL)
#endif
//-------------------------------------------
{
	if (fname)
	{
		_name = (char*)MALLOC(strlen(fname)+1);
		if (!_name)
		{
			DBG_ERR(("outamem!  can't create _name!\n"));
			return;
		}
		strcpy(_name, fname);
	}
	else
		_name = 0;
	_base = 0;	
	_offset = 0;
	_chain = _files;
	_files = this;
}

//-------------------------------------------
SymFile::~SymFile()
//-------------------------------------------
{ 
   if (_name) FREE(_name);
	   //if (_chain) SYM_DELETE(_chain); 
} // SymFile::~SymFile()

//-------------------------------------------
Boolean SymFile::open()
//-------------------------------------------
{
#ifdef macintosh
    if (!fRefNum)
    {
		if (_nfilesopen >= FOPEN_MAX-6)
		{
			for (SymFile* f=_files; f; f=f->next())
			{
				if (f->fRefNum)
				{
					f->close();
					break;
				}
			}	//need to close some files to make more room!
		}
		c2pstr(_name);
		OSErr err = HOpen(0, 0, (ConstStr255Param)_name, fsRdPerm, &fRefNum);
		p2cstr((unsigned char*)_name);
		if (err)
    	{
        	DBG_ERR(("Can't open '%s' for input\n", _name));
			return false;
        }
		_nfilesopen++; 
	}
	_offset = 0;
	if (_base) 
		return SetFPos(fRefNum, fsFromStart, _base) == noErr;
#else
    if (!_fp)
    {
		if (_nfilesopen >= FOPEN_MAX-6)
		{
			for (SymFile* f=_files; f; f=f->next())
			{
				if (f->_fp)
				{
					f->close();
					break;
				}
			}	//need to close some files to make more room!
		}
    	if ((_fp = fopen(_name,"rb")) == 0)
    	{
        	DBG_ERR(("Can't open '%s' for input\n", _name));
			return false;
        }
#ifdef macintosh
		_buffer = NewPtr(SYM_BUFFER_SIZE);
		if (_buffer)
			setvbuf(_fp, _buffer, _IOFBF, SYM_BUFFER_SIZE);
#endif
		_nfilesopen++; 
	}
	_offset = 0;
	if (_base && tell()!=_base) 
		return (Boolean)!fseek(_fp,_base,SEEK_SET);
#endif // macintosh
	return true;
}

//-------------------------------------------
void SymFile::close()
//-------------------------------------------
{ 
#ifdef macintosh
	if (fRefNum)
	 { 
        _offset = tell() - _base;
		FSClose(fRefNum); 
		_nfilesopen--; 
		fRefNum=0; 
	} 
#else
	if (_fp)
	 { 
        _offset = tell() - _base;
		fclose(_fp); 
#ifdef macintosh
		if (_buffer)
		{
			DisposePtr (_buffer);
			_buffer = 0;
		}
#endif
		_nfilesopen--; 
		_fp=0; 
	} 
#endif
}

//-------------------------------------------
Boolean SymFile::test()
//-------------------------------------------
{
#ifdef macintosh
	if (!fRefNum)
	{ 	//if file was closed, reset it
		if (!open())
			return false;
		uint32 p = _offset + _base;
        if (p)
        	return (Boolean)(fRefNum
        		&& SetFPos(fRefNum, fsFromStart, p) == noErr);
        return (Boolean)(fRefNum!=0);
	}
#else
	if (!_fp)
	{ 	//if file was closed, reset it
		uint32 p = _offset + _base;
		if (!open())
			return false;
        if (p)
        	return (Boolean)(_fp && !fseek(_fp,p,SEEK_SET));
        return (Boolean)(_fp!=0);
    }
#endif
    return true;
}
		
#ifdef macintosh
// Non-macintosh versions are in symnet_inlines.h
//-------------------------------------------
Boolean SymFile::seek(int32 off,uint32 base_kind)
//-------------------------------------------
{
static char gModeTrans[] = { fsFromStart, fsFromMark, fsFromLEOF };
//	short posmode;
//	switch (base_kind)
//	{
//		case SEEK_SET: posmode = fsFromStart; break;
//		case SEEK_END: posmode = fsFromLEOF; break;
//		case SEEK_CUR: posmode = fsFromMark; break;
//	}
    return (Boolean)(test()
    	 && (SetFPos(fRefNum, gModeTrans[base_kind], off + _base) == noErr));
}
#endif

//==========================================================================
//  forward references

SymFwdrefTypes::~SymFwdrefTypes()
{	//stack safe deletion...
	SymFwdrefType* frt;
	SymFwdrefType* temp_frt;
	SymRefType* rt;
	SymRefType* temp_rt;
	uint32 deleteCount = 0;
	uint32 totalCount = _ref_type_count + _fwdref_type_count;
	TProgressBar progressBar;

	progressBar.InitProgressDialog(kProgressString2, kProgressString3); 
	for (frt=_fwdref_head; frt;)
	{
		if ((deleteCount % 2048) == 0)
			progressBar.UpdateProgress(deleteCount, totalCount);
		++deleteCount;
		temp_frt = frt;
		frt = frt->next();
		SYM_DELETE(temp_frt);
	}

	for (rt=_ref_head; rt;)
	{
		if ((deleteCount % 2048) == 0)
			progressBar.UpdateProgress(deleteCount, totalCount);
		++deleteCount;
		temp_rt = rt;
		rt = rt->next();
		SYM_DELETE(temp_rt);
	}
	progressBar.UpdateProgress(deleteCount, totalCount);
	progressBar.EndProgressDialog();
}

SymType* SymFwdrefTypes::add_ref_type(Fwdref file_offset,SymType* type) {
	DBG_ASSERT(type);
	DBG_(SYMNET,("added type reference file_offset=x%X, type=x%X, type->cat=x%X, type->name=%s\n",
		file_offset,type,type->cat(),type->name()?type->name():"(null)"));
	SymFwdrefType* rt = add_fwdref_type((Fwdrefid)0,file_offset);
	rt->set_symtype(type);
	return type;
	}
	
SymType* SymFwdrefTypes::get_ref_type(Fwdref file_offset,SymRoot* symroot) {
	//SOMEDAY!! could check if type at id already added and return it!
	//that would be more efficient space wise, but this is more efficient time wise... 
	//which is better for us???
	SymType* symtype = symroot->add_type(0,0,tc_ref,(void*)file_offset); //set bogus type to filetype for now...
	SymRefType* rt = (SymRefType*)SYM_NEW(SymRefType(file_offset,symtype));	//need to be able to save offset with each type!
	if (!rt) {
		DBG_ERR(("# Unable to create SymRefType!\n"));
		return 0;
		}
	rt->set_next(_ref_head);//add to top of chain
	_ref_head = rt;
	++_ref_type_count;
	return symtype;
	}
	
//time to resolve the list of fwdrefs with refs...
Boolean SymFwdrefTypes::resolve_ref_types() {
	DBG_ENT("resolve_ref_types");
	SymFwdrefType* rt;
	SymFwdrefType** table;
	SymRefType* r;
	Boolean rc=true;
	uint32 count = 0;
	SymNet::_progress_bar->ClearProgress();
	SymNet::_progress_bar->ChangeStatus((StringPtr) kProgressString1);

	if ((table = build_ref_types_table()) == 0)
		return false;

	for (r=_ref_head; r; r=r->next()) {	//for each bogus type
		++count;
		if ((count % 16) == 0) {
			if (!SymNet::_progress_bar->UpdateProgress(count, _ref_type_count)) {
				DBG_ERR(("User Abort"));
				return false;
				}
			}
		rt=search_fwdref_types(r->typeref(), table);
		if (!rt) {
			DBG_ERR(("no reference found for typeref=x%X\n",r->typeref()));
			rc=false;
			}
		else
			r->resolve(rt->symtype());
		}

	cleanup_ref_types(table);
	return rc;
	}

SymFwdrefType* SymFwdrefTypes::search_fwdref_ids(Fwdrefid id) {
   SymFwdrefType* rt;
   for (rt=_fwdref_head; rt; rt=rt->next()) {
       if (rt->id() == id) return rt;
       }
   return 0;
   }

//	(9/12/95 dkk)
//	Take the linked list of SymFwdrefType's, and store them in an array so that we can
//	binary search, rather than walking the linked list.
SymFwdrefType** SymFwdrefTypes::build_ref_types_table() {
	SymFwdrefType* rt;
	SymFwdrefType** table;
	uint32 index = _fwdref_type_count - 1;
	Fwdref temp;
	Boolean notSorted = false;

	table = (SymFwdrefType **) MALLOC(_fwdref_type_count*sizeof(SymFwdrefType*));
	if (table == 0)
		return 0;

	temp = 0xFFFFFFFF;
	for (rt=_fwdref_head; rt; rt=rt->next()) {
		table[index] = rt;

		//	Check to see if we need to sort the list.
		if (temp < rt->typeref())
			notSorted = true;
   		temp =  rt->typeref();
		--index;
		}

	//	(9/21/95 dkk)
	//	If the list isn't sorted, sort it. We'll just do a bubble sort since the majority of the
	//	items in the list should be in the right order.
	if (notSorted) {
		uint32 i, j;
		SymFwdrefType* hold;
		Boolean noSwap;

		for (i = 0; i < _fwdref_type_count - 1; ++i) {
			noSwap = true;
			for (j = 0; j < _fwdref_type_count - 1 - i; ++j)
				if (table[j]->typeref() > table[j + 1]->typeref())
				{
					hold = table[j];
					table[j] = table[j + 1];
					table[j + 1] = hold;
					noSwap = false;
				}
			if (noSwap)
				break;
			}
		}
	return table;
	}


void SymFwdrefTypes::cleanup_ref_types(SymFwdrefType** ref_table) {
	FREE(ref_table);
	}


SymFwdrefType* SymFwdrefTypes::search_fwdref_types(Fwdref file_offset, SymFwdrefType** ref_table) {
	int32 middle;
	int32 first = 0;
	int32 last = _fwdref_type_count - 1;

	//	Binary search the list
	while (first <= last)
	{
		middle = ((last - first) / 2) + first;
		if (ref_table[middle]->typeref() == file_offset)
			return ref_table[middle];
		
		if (ref_table[middle]->typeref() < file_offset)
			first = middle + 1;
		else
			last = middle - 1;
       }
   return 0;
   }
   
//id - used to identify which type
//t - used as a type reference (pointer/idx to file format's type representation)
SymFwdrefType* SymFwdrefTypes::add_fwdref_type(Fwdrefid id,Fwdref t) {
   DBG_(PICKY,("add_fwdref_type: adding type at id=x%X, t=x%X\n",id,t));
   SymFwdrefType* rt = (SymFwdrefType*)SYM_NEW(SymFwdrefType(id,t));
	if (!rt) {
		DBG_ERR(("# Unable to create SymRefType!\n"));
		return 0;
		}
   DBG_ASSERT(rt);
   rt->set_next(_fwdref_head);//add to top of chain
   _fwdref_head = rt;
   ++_fwdref_type_count;
   return rt;
   }

//==========================================================================
//  line number <==> address mapping
//		used by SymModEntry for getting character offsets from line numbers

CharOffs::CharOffs(char *fname, char *fpath) {
			DBG_ENT("CharOffs");
			char fullname[1024];
			_ln2c = 0;
            _maxlns = 0;
            _maxchs = 0;
			if (!fname) {
                DBG_ERR(("CharOffs: no file name!!\n"))
				DBG_ASSERT(fname);
				return;
				}
			DBG_(SYMNET,("fname=%s; fpath=%s\n",fname,fpath?fpath:"(null)"));
		#ifdef __3DO_DEBUGGER__
			char full3dosrcname[1024];
			//@@@@@ this is a temp hack to make this code the same as the SourceWindow code
			extern void GetSourceFile(char* sourceFilePath, const char* fname);
			GetSourceFile(full3dosrcname, fname);
			DBG_(SYMNET,("full3dosrcname=%s\n",full3dosrcname));
		#endif
			
			// cat the name that was in the sym file (option 3)
			if (fpath) strcpy(fullname, fpath); 
			else *fullname=0;
			strcat(fullname, fname);
			DBG_(SYMNET,("fname=fullname=%s\n",fullname));

			//@@@@@ test file names for source.  This will change to call an external function
            if ((_sfp = fopen(fullname,"r")) == 0
		#ifdef __3DO_DEBUGGER__
                	&& (_sfp = fopen(full3dosrcname,"r")) == 0
		#endif
                	&& (_sfp = fopen(fname,"r")) == 0) {
                DBG_ERR(("CharOffs: unable to open file %s\n",fname))
                return;
                }
			// count total number of lines
			int cc, c, nlines=0;
			//note: assumes lines end in \n...  OK for Mac(right?), but not others
			for (cc=1; (c=getc(_sfp))!=EOF; cc++)
				if (c==NEW_LINE) nlines++;
            _maxlns = nlines+1;	//want initial line of 1,1
            _maxchs = cc;
            if (nlines>0) {
            	DBG_(SYMNET,("maxlns = %d; maxchs = %d\n",_maxlns, _maxchs));
            	rewind(_sfp);
            	_ln2c = (uint32*) MALLOC(_maxlns*sizeof(uint32));
      			DBG_ASSERT(_ln2c);
            	_ln2c[0] = 1;	//initial line
            	DBG_(PICKY,("line 1: _ln2c[0]=1\n"));
            	nlines=1;
            	//char counter starts from 1
            	for (cc=1; (c=getc(_sfp))!=EOF; cc++) {
                	if (c==NEW_LINE) {
                		DBG_(PICKY,("line %d: _ln2c[%d]=x%X\n",nlines+1,nlines,cc+1));
                	    _ln2c[nlines] = cc+1;	//line begins after \n
                	    nlines++;
             	       }
            		}
            	}
            fclose(_sfp);
            _sfp = 0;
            }
				
//ln ranges from 1 to maxlns
unsigned long CharOffs::ln2ch(unsigned long ln) {
			if (!_ln2c) {
                    DBG_ERR(("CharOffs: line<->charoff table not yet created\n"))
                    return 0;
                    }
            if (ln == 0 || ln > _maxlns) {
                    DBG_ERR(("CharOffs: line %d out of range\n",ln))
                    return 0;
                    }
            DBG_(SYMNET,("ln2ch(%d)=x%X\n",ln,_ln2c[ln-1]));
            return _ln2c[ln-1];
            }
				
//ch ranges from 1 to maxchs
unsigned long CharOffs::ch2ln(unsigned long ch) {
            int i;
			if (!_ln2c) {
                DBG_ERR(("CharOffs: line<->charoff table not yet created\n"))
            	return 0;
                }
            if (ch == 0 || ch > _maxchs) {
                DBG_ERR(("CharOffs: char offset %d out of range\n",ch))
                return 0;
                }
            if (ch < _ln2c[0]) return 1;
            if (ch >= _ln2c[_maxlns-1]) {	//last line?
            	DBG_(SYMNET,("ch2ln(x%X)=%d\n",ch,_maxlns));
                return _maxlns;
                }
            for (i=0; i<_maxlns-1; i++) {
                if (ch >= _ln2c[i] && ch < _ln2c[i+1]) {
              		DBG_(SYMNET,("ch2ln(x%X)=%d\n",ch,i+1));
                    return i+1;
                    }
                }
            return 0;
            }
				
CharOffs::~CharOffs() {
            if (_ln2c) FREE(_ln2c);
            }
