/*  @(#) symnodes.h 96/07/25 1.21 */

//====================================================================
// symnodes.h  -  contains definitions for the nodes of the
//				  symbol network
//
// Symbol Network organization/hierarchy (organized by scope):
//
//		symroot	- top symbol node
//			gsyms	- link of data symbols which are global/external
//			gfuncs	- link of functions which are global/external
//			types	- all types 
//				SOMEDAY: types have scopes too, but we're not taking that 
//						 into account here!!
//			syms	- all symbols
//			funcs	- all functions
//				lsyms		- link of all symbols which are local to this function
//				blocks		- all blocks within this function
//					lsyms		- link of all symbols which are local to this block
//					blocks		- all blocks within this block
//						lsyms		- link of all symbols which are local to this block
//						blocks		- all blocks within this block
//							(etc...	- recursively defined)
//			mods	- all modules
//				lines		- all lines for this module
//				mgsyms	- link of data symbols defined in this module which are global/external (subset of gsyms)
//				mgfuncs	- link of functions defined in this module which are global/external (subset of gfuncs)
//				msyms		- link of data symbols which are local to this module
//				mfuncs		- all functions which are local to this module
//					lsyms		- link of all symbols which are local to this function
//					blocks		- all blocks within this function
//						(etc...	- recursively defined)
//
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

#ifndef __SYMNODES_H__
#define __SYMNODES_H__
#define SYMNODE_NEW(x) HEAP_NEW(global_heap,x) 
#define SYMNODE_DELETE(x) HEAP_DELETE(global_heap,x) 
#define SYMNODE_DELETE_ARRAY(x) HEAP_DELETE_ARRAY(global_heap,x) 

//====================================================================
// structs/classes for modules, lines, etc in symbol network

class SymRoot;		// symbol root
class SymEntry;		// symbol
class SymModEntry;	// module
class SymFuncEntry;	// function
class SymModLink;	// links symbol/func to module
class SymFuncLink;	// links symbol to function
class SymRootLink;	// links symbol/func to root symbol node
class SymLineEntry;	// line
class SymType;		// type for a symbol
class SymStructType;// type reference for a struct/union
class SymArrayType;	// type reference for an array
class SymEnumType;	// type reference for an enum
class SymError;		// error & state class

//====================================================================
// root class for all nodes

class SymRoot {
friend class SymNet;	
friend class SymDump;	
friend class SymFwdrefTypes;	
friend class SymEntry;		// symbol
friend class SymModEntry;	// module
friend class SymFuncEntry;	// function
friend class SymModLink;	// links symbol/func to module
friend class SymFuncLink;	// links symbol to function
friend class SymRootLink;	// links symbol/func to root symbol node
friend class SymLineEntry;	// line
friend class SymType;		// type for a symbol
friend class SymStructType;	// type reference for a struct/union
friend class SymArrayType;	// type reference for an array
friend class SymEnumType;	// type reference for an enum
friend class CharOffs;		// character <-> line mapping
	//static SymError* _state;	// caller's error state structure
	//static Heap* _heap;	
    SymType* _types;			// all types
    SymRootLink* _gsyms;		// links together all data symbols which are global or external
    SymRootLink* _gfuncs;		// links together all functions which are global or external
    SymFuncEntry* _funcs;		// all functions everywhere
    SymModEntry* _mods;			// all modules everywhere
    SymEntry* _syms;			// all syms everywhere
    SymEntry* add_sym(const char* name,SymType* type, uint32 val,SymSec sec,SymScope scope,SymbolClass sclass);
    SymModEntry* add_mod(const char* name, uint32 baddr, uint32 eaddr);
    SymFuncEntry* add_func(SymEntry* sym,SymModEntry* mod=0);
    SymRootLink* add_gsym(SymEntry* sym,SymModEntry* mod=0);
    SymRootLink* add_gfunc(SymFuncEntry* f,SymModEntry* mod=0);
	SymType* add_type(const char* name,uint32 size,SymCat c,void* ref=0);
	uint32 fAllocations;		//	Used to display progress bar when deleting objects.
public:
    SymRoot();
    ~SymRoot();
    SymRootLink* gsyms() { return _gsyms; }
    SymRootLink* gfuncs() { return _gfuncs; }
    SymFuncEntry* funcs() { return _funcs; }
    SymModEntry* mods() { return _mods; }
    SymEntry* syms() { return _syms; }
    SymType* types() { return _types; }
    };

//====================================================================
// structs/classes for types
//
//		SymType:
//			_cat	- category of type (tc_pointer, tc_char, etc)
//			_type	- if compound type, _type hold pointer to other types
//			_size	- size in bytes of type (bits for bitfield)
//			_name	- name of type
//			_chain	- pointer to rest of types in network
//		 
// if cat&TC_REFMASK==0,type=0 				- type is simple and is in cat
// if cat==TC_FUNCTION, type=>SymType 		- type is return type
// if cat==TC_ARRAY, 	type=>SymArrayType	- type is array
// if cat==TC_STRUCT, 	type=>SymStructType	- type is struct
// if cat==TC_UNION, 	type=>SymStructType	- type is struct
// if cat==TC_ENUM, 	type=>SymEnumType	- type is enum
// if cat==TC_MEMBER, 		type=>SymType		- type points to enum type
// if cat==TC_POINTER, 	type=>SymType 		- type is what type points to
// if cat==TC_REF, 		type=>SymType 		- type is what type refers to
// if cat==TC_ARG, 		type=>SymType		- type is argument type
//

class SymType {
friend class SymNet;
friend class SymDump;
friend class SymRoot;
friend class SymStructType;
friend class SymEnumType;
friend class SymArrayType;
friend class SymRefType;
friend class ElfReader;
friend class XsymReader_v32;
friend class XsymReader_v33;
friend class XsymReader_v34;
friend class TypesStack;	// stack of types for parsing
   void* _type; //can be whatever - another type, struct, array...
   uint32 _size;
   SymCat _cat;	
   char* _name;
   SymType* _chain;
   SymArrayType* add_array(SymType* t);
   SymStructType* add_struct(int nfields=0);
   SymEnumType* add_enum(SymType* t,int nmembers=0);
   void set_name(const char* n);
   void set_size(uint32 size) { _size=size; }
   void set_type(void* t) { _type=t; }
   SymType* next() { return _chain; }
public:
   SymType(SymType*& chain, const char* n,uint32 s,SymCat c,void* t);
   ~SymType();
   char* name() { return _name; }
   uint32 size() { return _size; }
   void* type() { return _type; }
   SymCat cat() { return _cat; }
   void set_cat(SymCat c) { _cat=c; }
   };
   
//structs are created by SymType when cat is a struct
   
class SymStructType {
friend class SymNet;	
friend class SymDump;	
friend class SymType;	
friend class ElfReader;	
friend class TypesStack;	// stack of types for parsing
	struct SymFieldType {
  	 	SymType* _type;
   		uint32 _offset;
   		};
   SymType* _type; //pointer to parent SymType type for this struct
   SymFieldType* _typelist; //array of fields
   uint32 _nfields;
   //uint32 _size;	//isn't size in the parent type?
   SymFieldType* add_field(uint32 offset,SymType* type);
   void set_type(SymType* t) { _type=t; }; //set type after type has been added to network
	void set_nfields(uint32 nfields);
public:
   SymStructType(uint32 _nfields=0);
   ~SymStructType();
   uint32 nfields() { return _nfields; }
   //??? uint32 size() { return _size; }	//in parent right??
   SymType* type() { return _type; }
   char* name() { return _type ? _type->name() : 0; }
   uint32 size() { return _type ? _type->size() : 0; }
   char* fieldname(int i) { 
		return ((_typelist && i>=0 && i<_nfields) 
			? (_typelist[i]._type ? _typelist[i]._type->name() : 0) 
			: 0); 
			}
   SymType* fieldtype(int i) 
   		{ return ((_typelist && i>=0 && i<_nfields) ? _typelist[i]._type : 0); }
   uint32 fieldoffset(int i) 
   		{ return ((_typelist && i>=0 && i<_nfields) ? _typelist[i]._offset : 0); }
   };
   
class SymArrayType {
friend class SymNet;	
friend class SymDump;	
friend class SymType;	
friend class SymReader;
friend class ElfReader;
friend class TypesStack;	// stack of types for parsing
   SymType* _type; //pointer to parent SymType type for this array
   SymType* _elemtype; //pointer to base type for the elements of this array
   int32 _lowerbound;
   int32 _upperbound;
   void set_type(SymType* t) { _type=t; }; //set type after type has been added to network
   void set_bounds(int32 l, int32 u) { 
   		_lowerbound=l; 
   		_upperbound=u; 
   		} //set type after type has been added to network
public:
   SymArrayType(SymType* elemtype) { 
   		_type = 0;
   		_lowerbound = 0;
   		_upperbound = 0;
   		_elemtype = elemtype; 
   		}
   ~SymArrayType() {};
   SymType* type() { return _type; }
   char* name() { return _type->name(); }
   uint32 size();
   SymType* membtype() { return _elemtype; }
   int32 upperbound();
   int32 lowerbound() { return _lowerbound; }
   uint32 nmembers();
   };
   
class SymEnumType {
friend class SymNet;	
friend class SymDump;	
friend class SymType;	
friend class ElfReader;	
friend class TypesStack;	// stack of types for parsing
	struct SymMemberType {	//member's referenced type points to this
		SymType* _mtype;	//member's type 
		SymEnumType* _etype;	//enum's type
		uint32 _val;	//member's value
		SymEnumType* etype() { return _etype; }	//allow member to reference enum type
		uint32 val() { return _val; }	//allow member to reference val
   		};
   SymType* _type; //pointer to parent SymType type for this enum
   SymType* _membtype; //pointer to base type for the elements of this enum
   SymMemberType* _memberlist; //array of members
   uint32 _nmembers;
   void set_type(SymType* t) { _type=t; }; //set type after type has been added to network
   SymMemberType* add_member(uint32 val,SymType* type);
	void set_nmembers(int nmembers);
public:
   SymEnumType(SymType* membtype,int nmembers=0);
   ~SymEnumType();
   SymType* membtype() { return _membtype; }
   uint32 nmembers() { return _nmembers; }
   SymType* type() { return _type; }
   char* name() { return _type->name(); }
   uint32 size() { return _type->size(); }
   char* membername(int i) 
   		{ return ((_memberlist && i>=0 && i<_nmembers) ? _memberlist[i]._mtype->name() : 0); }
   SymType* membertype(int i) 
   		{ return ((_memberlist && i>=0 && i<_nmembers) ? _memberlist[i]._mtype : 0); }
   uint32 memberval(int i) 
   		{ return ((_memberlist && i>=0 && i<_nmembers) ? _memberlist[i]._val : 0); }
   };
   
   
//====================================================================
// structs/classes for modules, lines, etc in symbol network

//add line to module (sorted in decending. order since last entry goes on top of list)
class SymLineEntry {
friend class SymNet;	
friend class SymDump;	
friend class SymModEntry;	
    uint32 _addr;
    uint32 _line;
    SymLineEntry* _chain;
    SymLineEntry* next() { return _chain; }
public:
    SymLineEntry(SymLineEntry*& chain, uint32 l, uint32 a);
    ~SymLineEntry();
    uint32 addr() { return _addr; }
    uint32 line() { return _line; }
	//Boolean cmp_addr(uint32 addr) { return (addr>_addr && _chain && addr<=_chain->_addr); }	//note that _chain is really the line we want!!
    };

//structure for symbol node in network
class SymEntry {
friend class SymNet;	
friend class SymDump;	
friend class SymRoot;	
friend class SymFuncEntry;
friend class SymModEntry;
friend class SymModLink;
friend class SymFuncLink;
friend class SymRootLink;
friend class ElfReader;
friend class SymReader;
friend class XcoffReader;
friend class XsymReader_v32;
friend class XsymReader_v33;
friend class XsymReader_v34;
    //int32 symidx;       // index for symbol
    uint32 _sec;  //section type (code/data/...)/storage class (auto/reg/...)/scope (local/...)
    SymType* _type;
    void* _parent; //ptr to my creator
    uint32 _val; //starting addr; saved val from symtab (replaced with ptr to this struct)
    char* _name;
    SymEntry* _chain; //for all syms - should be binary tree (later)
    SymEntry* next() { return _chain; }
	void set_sectype(SymSec sectype) { _sec &= ~SEC_MASK; _sec |= (((uint32)sectype)&SEC_MASK); }
	// set storage class of symbol
	void set_stgclass(SymbolClass stgclass) { _sec &= ~SC_MASK; _sec |= (((uint32)stgclass)&SC_MASK); }
	// set scope of symbol 
	void set_scope(SymScope scope) { _sec &= ~SCOPE_MASK; _sec |= (((uint32)scope)&SCOPE_MASK); }
    void set_type(SymType* symt) { _type = symt; }
    void set_val(uint32 v) { _val = v; }
    void set_parent(void* p) { _parent = p; }
	void* parent() { return _parent; }	//may be either function or data link
public:
	SymEntry(SymEntry*& chain, const char* n, SymType* t, uint32 v, SymSec sec, SymScope scope, SymbolClass sc);
    ~SymEntry();
	// currently symbols are stored by section type, but that could change...
	// get section type symbol belongs to
	SymSec sectype() {	return (SymSec)(_sec&SEC_MASK); }
	// get storage class of symbol
	SymbolClass stgclass() { return (SymbolClass)(_sec&SC_MASK); }
	// get scope of symbol 
	SymScope scope() { return (SymScope)(_sec&SCOPE_MASK); }
	SymType* type() { return _type; }
	uint32 val() { return _val; }
	char* name() { return _name; }
	SymFuncEntry* func();
	SymModLink* modlink();
	SymRootLink* rootlink();
    //Boolean cmp_name(const char* n) { return (n && _name && !mystrcmp(_name, n)); }
    //Boolean cmp_addr(uint32 a) { return (a == _val); }
    };
    
//link sym to module (eg. static functions/data)
class SymModLink {
friend class SymNet;	
friend class SymDump;	
friend class SymRoot;	
friend class SymModEntry;	
    SymModEntry* _mod;	//module defined in
    void* _thing;		//symbol/function node in network for this link
    SymModLink* _chain;	//next symbol link
    SymModLink* next() { return _chain; }
    void set_mod(SymModEntry* mod) { _mod = mod; }
public:
    SymModLink(SymModLink*& chain, SymEntry* s, SymModEntry* m);
    SymModLink(SymModLink*& chain, SymFuncEntry* f, SymModEntry* m);
    ~SymModLink();
    SymEntry* sym() { return (SymEntry*)_thing; }
    SymFuncEntry* func() { return ((SymFuncEntry*)_thing); }	//if this links a function to a module, return function
    SymModEntry* mod() { return _mod; }
    };
    
//link sym to func (eg. local symbols/register variables)
class SymFuncLink {
friend class SymNet;	
friend class SymDump;	
    SymFuncEntry* _func;	//parent function/block
    SymEntry* _sym;			//symbol node in network for this sym
    SymFuncLink* _chain;	//next symbol link
    SymFuncLink* next() { return _chain; }
public:
    SymFuncLink(SymFuncLink*& chain, SymEntry* s, SymFuncEntry* f);
    ~SymFuncLink();
    SymFuncEntry* func() { return _func; }
    SymEntry* sym() { return _sym; }
    };
    
//link sym to root (eg. hwregs/global/externs)
class SymRootLink {
friend class SymNet;	
friend class SymDump;	
friend class SymRoot;	
    SymModEntry* _mod;		//module defined in if any (default is 0)
    void* _thing;			//symbol/function node in network for this link
    SymRootLink* _chain;	//next symbol link
    SymRootLink* next() { return _chain; }
    void set_mod(SymModEntry* mod) { _mod = mod; }
public:
    SymRootLink(SymRootLink*& chain, SymEntry* s, SymModEntry* m=0);
    SymRootLink(SymRootLink*& chain, SymFuncEntry* s, SymModEntry* m=0);
    ~SymRootLink();
    SymEntry* sym() { return (SymEntry*)_thing; }
    SymModEntry* mod() { return _mod; }	//0 if not defined in any module
    SymFuncEntry* func() { return ((SymFuncEntry*)_thing); }	//if this links a function to a module, return function
    };
	#define CHAROFFS_ERR	0xffffffff	//returned by charoff and linenum if error
	class CharOffs;

//add module to root
class SymModEntry {
friend class SymNet;	
friend class SymDump;	
friend class SymRoot;	
friend class SymReader;
friend class ElfReader;
friend class XcoffReader;
friend class XsymReader_v32;
friend class XsymReader_v33;
friend class XsymReader_v34;
    SymModLink* _msyms;		// data syms local to this module
    SymModLink* _mfuncs;	// funcs local to this module
    SymModLink* _mgsyms;	// global syms defined in this module
    SymModLink* _mgfuncs;	// global funcs defined in this module
    SymLineEntry* _lines;	// lines for this module sorted by line
    SymLineEntry* _lines_sorted_by_addr;	// lines for this module sorted by addr
    int32 _nlines;			// number of lines in module
    uint32 _baddr;
    uint32 _eaddr;
    char* _name;
    char* _path;
    SymModEntry* _chain;
	CharOffs* _cos;
    void set_name(const char* mname, char* mpath);
    void set_baddr(uint32 addr) { _baddr=addr; }
    void set_eaddr(uint32 addr) { _eaddr=addr; }
    void set_nlines(int32 n) { _nlines=n; }
    SymModLink* add_msym(SymEntry* s);
    SymModLink* add_mfunc(SymFuncEntry* f);
    SymModLink* add_mgsym(SymEntry* s);
    SymModLink* add_mgfunc(SymFuncEntry* f);
    SymLineEntry* add_line(uint32 l, uint32 a);
    SymLineEntry* add_charoff(uint32 c, uint32 a);
    SymModEntry* next() { return _chain; }
public:
    SymModEntry(SymModEntry*& chain, const char* name,uint32 baddr,uint32 eaddr);
    ~SymModEntry();
    char* name() { return _name; }
    char* path() { return _path; }
    uint32 baddr() { return _baddr; }
    uint32 eaddr() { return _eaddr; }
    int32 nlines() { return _nlines; }
    SymModLink* msyms() { return _msyms; }		//data symbols local to module
    SymModLink* mfuncs() { return _mfuncs; }	//functions local to module
    SymModLink* mgsyms() { return _mgsyms; }		//global data symbols defined in module (subset of gsyms)
    SymModLink* mgfuncs() { return _mgfuncs; }	//global functions defined in module (subset of gfuncs)
    SymLineEntry* lines() { return _lines; }
    SymLineEntry* lines_sorted_by_addr() { return _lines_sorted_by_addr; }
    uint32 charoff(uint32 ln);
    uint32 linenum(uint32 ch);
    //Boolean cmp_name(const char* n) { return (n && _name && !mystrcmp(_name, n)); }
    //Boolean inside(uint32 a) { return (a >= _baddr && a <= _eaddr); }
    };
    
//add func to root/module or block to block/func
//note: SymFuncEntry used for both functions and blocks
class SymFuncEntry {
friend class SymNet;	
friend class SymDump;	
friend class SymRoot;	
friend class SymReader;
friend class SymModLink;
friend class SymRootLink;
friend class ElfReader;
friend class XcoffReader;
friend class XsymReader_v32;
friend class XsymReader_v33;
friend class XsymReader_v34;
	//_mod isn't needed if I use Links instead
    SymModEntry* _mod;		// ptr to mod entry
    SymFuncEntry* _func;	// ptr to parent func/block entry (null if top level)
    SymEntry* _sym;			// symbol node in network for this func
    SymLineEntry* _lines;	// lines for func/block
    SymFuncLink* _lsyms;	// links together all symbols which are local to this function
    SymFuncEntry* _blocks;	// all blocks for this func/block
    int32 _bline;     //line for start of function code
    int32 _eline;     //line for end of function code
    uint32 _baddr;	//beginning addr offset from text base
    uint32 _eaddr;	//ending addr offset from text base
    uint32 _size;	//size of code for function
	void* _parent;
    SymFuncEntry* _chain;
    void set_bline(int32 line) { _bline=line; }
    void set_eline(int32 line) { _eline=line; }
    void set_baddr(uint32 addr) { _baddr=addr; }
    void set_eaddr(uint32 addr) { _eaddr=addr; }
    void set_size(uint32 n) { _size=n; }
    void set_lines(SymLineEntry* linesptr) { _lines=linesptr; }
    SymFuncLink* add_lsym(SymEntry* s);
    SymFuncEntry* add_block(SymEntry* s);
    SymFuncEntry* next() { return _chain; }
    void set_mod(SymModEntry* mod) { _mod = mod; }
    void set_parent(void* p) { _parent = p; }
	void* parent() { return _parent; }	//may be either function or data link
public:
    SymFuncEntry(SymFuncEntry*& chain, SymEntry* s,SymModEntry* m=0,SymFuncEntry* f=0);
    ~SymFuncEntry();
    uint32 baddr() { return _baddr; }
    uint32 eaddr() { return _eaddr; }
    uint32 bline() { return _bline; }
    uint32 eline() { return _eline; }
    uint32 size() { return _size; }
    SymFuncEntry* func() { return _func; }
    SymModEntry* mod() { return _mod; }
    SymEntry* sym() { return _sym; }
    SymLineEntry* lines() { return _lines; }	//not used??
    SymFuncLink* lsyms() { return _lsyms; }
    SymFuncEntry* blocks() { return _blocks; }
    char* name() { return _sym->name(); }
    uint32 val() { return _sym->val(); }
    SymScope scope() { return _sym->scope(); }
	SymModLink* modlink();
	SymRootLink* rootlink();
    //Boolean cmp_name(const char* n) { return (_sym && _sym->cmp_name(n)); }
    //Boolean cmp_addr(uint32 a) { return (a == _sym->val()); }
    //Boolean inside(uint32 a) { return (a >= _sym->val() && a <= _eaddr); }
    };
    
//SymAnyEntry
union SymAnyEntry {
    SymFuncLink* fl;	//local sym
    SymRootLink* rl;	//root link for sym/func
    SymModLink* ml;		//module link for sym/func
    SymFuncEntry* f;	//func/block
    SymEntry* s;		//sym
    SymModEntry* m;		//module
    };
                
#endif /* __SYMNODES_H__ */


