/*  @(#) symapi.h 96/07/25 1.29 */

/*
	File:		symapi.h

	Written by:	Dawn Perchik

 symapi.h  -  Symbolics class and structs visable to world 

		Symbolics class hierarchy:

			Symbolics	- contains main functions visible to world
				SymNet	- builds internal network of symbols and queries them
					SymReader - reads ARM sym files with sym debug info and
								calls SymNet methods to add symbols to network
					XcoffReader - reads XCOFF files with dbx stabs debug info and
								calls SymNet methods to add symbols to network
					ElfReader - reads ELF files with dwarf v1.1 debug info and
								calls SymNet methods to add symbols to network
	Copyright:	© 1996 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by The 3DO Company.

	Change History (most recent first):

		<11>	 7/25/96	PUT YOUR INITIALS HERE		Auto Update - 96/05/08 3:11:47 PM
				96/03/15	JRM		Header first added

	To Do:
*/

#ifndef SYMAPI__H
#define SYMAPI__H

#include "predefines.h"
#include "loaderty.h"
#include "utils.h"

//==========================================================================
//  forward declarations

class TProgressBar;
struct SymEntry; //handle to symbol
struct SymType; //handle to type
struct SymStructType; //handle to struct/union type
struct SymArrayType; //handle to array type
struct SymEnumType; //handle to enum type
struct SymFieldType; //handle to struct field type

#ifdef __3DO_DEBUGGER__	
struct YYSTYPE;
#include "CExpr_Types.h"
#endif /* __3DO_DEBUGGER__	*/

//==========================================================================
//  return codes for Symbolics apis:

#define SE_ADDR_ERR 		0xffffffff	//error for apis that return an address 
#define SE_SUCCESS 			0
//non-fatal errors
#define SE_NOT_FOUND 		1 //symbol/address not found
#define SE_EXACT_MATCH   	2
#define SE_APPROX  			3
#define SE_UNKNOWN_TYPE 	6 //general unknown
#define SE_BAD_CALL 		7 //bad call to symbolics library
#define SE_PARM_ERR 		8 //should this be fatal??? invalid parameter to api
#define SE_SCOPE_ERR  		9 //unable to set scope 
#define SE_NO_SOURCE  		10 //no source for module
#define SE_NO_LINE_INFO 	11 //no line information
#define SE_NO_DEBUG_INFO 	12 //no debug information
#define SE_NO_SYMBOLS 		13 //no symbols found
#define SE_UND_SYMBOLS 		14 //undefined symbols found
#define SE_DUP_SYMBOLS 		15 //duplicate defs of symbols found
//fatal errors
#define SE_FATAL_ERR 		0x1000
#define SE_FAIL 			(1 | SE_FATAL_ERR)	//general failure
#define SE_OPEN_FAIL 		(3 | SE_FATAL_ERR)	//unable to open symbol file
#define SE_INV_SYMBOLS 		(2 | SE_FATAL_ERR)	//symbol table not valid
#define SE_MALLOC_ERR 		(4 | SE_FATAL_ERR)	//call to malloc failed
#define SE_SEEK_ERR 		(5 | SE_FATAL_ERR)	//file seek error while reading symbol file
#define SE_READ_ERR 		(6 | SE_FATAL_ERR)	//read error while reading symbol file
#define SE_INVALID_OBJ 		(7 | SE_FATAL_ERR)	//invalid symbolics object
#define SE_WRONG_VERSION 	(8 | SE_FATAL_ERR)	//wrong version
#define SE_UNKNOWN_FORMAT 	(9 | SE_FATAL_ERR)	//unknown file format

enum SymErr {
		se_success = SE_SUCCESS,
	//informational return codes
		se_not_found = SE_NOT_FOUND,
		se_exact_match = SE_EXACT_MATCH,
		se_approx = SE_APPROX,
		se_no_source = SE_NO_SOURCE,
		se_no_debug_info = SE_NO_DEBUG_INFO,
		se_no_line_info = SE_NO_LINE_INFO,
		se_no_symbols = SE_NO_SYMBOLS,
		se_und_symbols = SE_UND_SYMBOLS,
		se_dup_symbols = SE_DUP_SYMBOLS,
	//non-fatal errors
		se_unknown_type = SE_UNKNOWN_TYPE,
		se_bad_call = SE_BAD_CALL,
		se_parm_err = SE_PARM_ERR,
		se_scope_err = SE_SCOPE_ERR,
	//fatal errors
		se_fail = SE_FAIL,
		se_open_fail = SE_OPEN_FAIL,	//moved to fatal error
		se_unknown_format = SE_UNKNOWN_FORMAT,
		se_inv_symbols = SE_INV_SYMBOLS,
		se_malloc_err = SE_MALLOC_ERR,
		se_seek_err = SE_SEEK_ERR,
		se_read_err = SE_READ_ERR,
		se_invalid_obj = SE_INVALID_OBJ,
		se_wrong_version = SE_WRONG_VERSION
	};

//==========================================================================
//  defines for Symbolics apis:

//type categories
#ifndef __3DO_DEBUGGER__	
/********************************/
//from Cexpr_common.h
//(to make conversion fast & easy)
#define BTYPE_Char 		  0x100L
#define BTYPE_Double 	  0x200L
#define BTYPE_Float 	  0x400L
#define BTYPE_Int 		  0x800L
#define BTYPE_Long 		 0x1000L
#define BTYPE_Short 	 0x2000L
#define BTYPE_Signed 	 0x4000L
#define BTYPE_Unsigned 	 0x8000L
#define BTYPE_Void 		0x10000L
#define BTYPE_String 	0x20000L
#define BTYPE_LLong 	0x40000L	//FIXME! not yet supported
/********************************/
#endif /* !__3DO_DEBUGGER__	*/

#define TC_UNKNOWN   0
#define TC_VOID      BTYPE_Void
#define TC_CHAR      (BTYPE_Char  | BTYPE_Signed)       /* character */
#define TC_UCHAR     (BTYPE_Char  | BTYPE_Unsigned)     /* unsigned character */
#define TC_SHORT     (BTYPE_Short | BTYPE_Signed)       /* short integer */
#define TC_USHORT    (BTYPE_Short | BTYPE_Unsigned)     /* unsigned short */
#define TC_INT       (BTYPE_Int   | BTYPE_Signed)       /* integer */
#define TC_UINT      (BTYPE_Int   | BTYPE_Unsigned)     /* unsigned integer */
#define TC_LONG      (BTYPE_Long  | BTYPE_Signed)       /* long integer */
#define TC_ULONG     (BTYPE_Long  | BTYPE_Unsigned)     /* unsigned long */
#define TC_LLONG	 (BTYPE_LLong | BTYPE_Signed)
#define TC_ULLONG    (BTYPE_LLong | BTYPE_Unsigned)
#define TC_FLOAT     BTYPE_Float      				  /* floating point */
#define TC_DOUBLE    BTYPE_Double       			  /* double word */
#define TC_STRING    BTYPE_String       			  /* string */
#if 0 	//def __3DO_DEBUGGER__	
	#define TC_REG       0x40000000L       /* special HW register */
#endif /* __3DO_DEBUGGER__ */

#define TC_LDOUBLE 		0x80000L
#define TC_COMPLEX 		0x100000L
#define TC_DCOMPLEX 	0x200000L 		//what do I do with a function type? xcoff doesn't have these...

//used to determine if type is simple or references another type
#define TC_REFMASK   	0x80000000L	//want it to be negative for quick testing

#define TC_POINTER  	(TC_REFMASK | 0x0000001L)		// so debugger can test for 0xFF
#define TC_ARRAY    	(TC_REFMASK | 0x400000L)    	//xcoff doesn't have arrays??
#define TC_STRUCT   	(TC_REFMASK | 0x800000L)   		// structure 
#define TC_UNION    	(TC_REFMASK | 0x1000000L)    	// union 
#define TC_ENUM     	(TC_REFMASK | 0x2000000L)    	// enumeration 
#define TC_MEMBER      	(TC_REFMASK | 0x4000000L)    	// member of struct/union/array/enum
#define TC_FUNCTION 	(TC_REFMASK | 0x8000000L) 		//what do I do with a function type? xcoff doesn't have these...
#define TC_ARG      	(TC_REFMASK | 0x10000000L)     	// function argument (only used by compiler) 
#define TC_REF      	(TC_REFMASK | 0x20000000L)
#define TC_REFSTMASK 	((~TC_REFMASK) & (TC_REF | TC_POINTER | TC_FUNCTION | TC_ARG | TC_MEMBER))		//used to determine if referenced type is a SymType

enum SymCat {
		tc_unknown = (long)TC_UNKNOWN,
		tc_void = (long)TC_VOID,
		tc_char = (long)TC_CHAR,
		tc_uchar = (long)TC_UCHAR,
		tc_short = (long)TC_SHORT,
		tc_ushort = (long)TC_USHORT,
		tc_int = (long)TC_INT,
		tc_uint = (long)TC_UINT,
		tc_long = (long)TC_LONG,
		tc_ulong = (long)TC_ULONG,
		tc_llong = (long)TC_LLONG,
		tc_ullong = (long)TC_ULLONG,
		tc_float = (long)TC_FLOAT,
		tc_double = (long)TC_DOUBLE,
	#if 0 	//def __3DO_DEBUGGER__	
		tc_reg = (long)TC_REG,
	#endif /* __3DO_DEBUGGER__ */
		tc_ldouble = (long)TC_LDOUBLE,
		tc_complex = (long)TC_COMPLEX,
		tc_string = (long)TC_STRING,
		tc_dcomplex = (long)TC_DCOMPLEX,
		tc_pointer = (long)TC_POINTER,
		tc_array = (long)TC_ARRAY,	
		tc_struct = (long)TC_STRUCT, 
		tc_union = (long)TC_UNION,	 
		tc_enum = (long)TC_ENUM,   
		tc_member = (long)TC_MEMBER,   
		tc_function = (long)TC_FUNCTION,
		tc_arg = (long)TC_ARG,  
		tc_ref = (long)TC_REF
	};
	
//section types by name 
//(linker will use this ordering for its own sections)
enum SymSec {
	sec_none, 
	sec_hdr3do, 	//3do header
	sec_code,		//text
	sec_init, 		//init section (contains text)
	sec_fini, 		//fini section (contains text)
	sec_rodata, 	//read only data
	sec_data,		//data
	sec_bss, 		//uninitialized data
	sec_sdata,		//small area data
	sec_sbss,		//small area bss
	sec_debug, 		//debug section
	sec_line, 		//line section
	sec_relatext, 	//rela relocs for text section
	sec_relainit, 	//rela relocs for init section
	sec_relafini, 	//rela relocs for fini section
	sec_relarodata, //rela relocs for read only data
	sec_reladata, 	//rela relocs for data section
	sec_relasdata, 	//rela relocs for small data section
	sec_reladebug, 	//rela relocs for debug section
	sec_relaline, 	//rela relocs for line section
	sec_imp3do, 	//imports
	sec_exp3do, 	//exports
	sec_shstrtab, 	//header strtab section
	sec_symtab, 	//symtab section
	sec_strtab, 	//strtab section
	sec_abs, 		//absolute (no section)
	sec_com, 		//common (no section)
	sec_und, 		//undefined (no section)
	sec_num, 		//number of sections including SEC_NONE	
	sec_all=sec_num //represents any or all sections
	};
#define SEC_MASK 	0x000f
//#define SEC_NUM 	((int)sec_num)
const int SEC_NUM = (int)sec_num;

//storage class types - used primarily for relocation
#define SC_MASK 		(SEC_MASK | 0x0ff0)
#define SC_NONE			((int)sec_none | 0x0000)	//no relocation
#define SC_CODE			((int)sec_code | 0x0010)	//use SEC_CODE
#define SC_DATA			((int)sec_data | 0x0020)	//use SEC_DATA
#define SC_BSS			((int)sec_bss  | 0x0040)	//use SEC_BSS 
#define SC_STACK		((int)sec_none | 0x0080)	//return offset (relocate to stack frame)
#define SC_REG			((int)sec_none | 0x0100)	//no relocation
#define SC_CONST		((int)sec_none | 0x0200)	//no relocation
#define SC_ABS			((int)sec_none | 0x0400)	//no relocation
//composites used for searching criteria
#define SC_NONAUTO_DATA (SC_DATA | SC_BSS)
#define SC_AUTO_DATA 	(SC_STACK | SC_REG)
#define SC_ALL_DATA 	(SC_DATA | SC_BSS | SC_AUTO_DATA)
#define SC_ALL 			(SC_ALL_DATA | SC_CODE) //all sections

enum SymbolClass {
		sc_none = SC_NONE,
		sc_code = SC_CODE,
		sc_data = SC_DATA,
		sc_bss = SC_BSS,
		sc_stack = SC_STACK,
		sc_reg = SC_REG,
		sc_const = SC_CONST,
		sc_abs = SC_ABS,
		sc_nonauto_data = SC_NONAUTO_DATA,
		sc_auto_data = SC_AUTO_DATA,
		sc_all_data = SC_ALL_DATA,
		sc_all = SC_ALL
	};

//scopes
#define SCOPE_MASK 		0xf000
#define SCOPE_NONE		0x0000
#define SCOPE_GLOBAL	0x1000
#define SCOPE_MODULE	0x2000
#define SCOPE_LOCAL		0x4000
#define SCOPE_ALL		(SCOPE_GLOBAL | SCOPE_MODULE | SCOPE_LOCAL)

enum SymScope {
		scope_none = SCOPE_NONE,
		scope_global = SCOPE_GLOBAL,
		scope_module = SCOPE_MODULE,
		scope_local = SCOPE_LOCAL,
		scope_all = SCOPE_ALL
	};


//==========================================================================
//  Symbolics object types
enum SymFileFormat
{
	eUNKNOWN = 0,	//not created yet
	eAR,	//unix library archive format
	eARM,
	eXCOFF,
	eELF,
	eXSYM = 30,
	eXSYMv32 = 32,
	eXSYMv33 = 33,
	eXSYMv34 = 34
	};

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
class SymFile;		// file class

#include "utils.h"
#include "symutils.h"

// include the new base class for the global declaration
//====================================================================
// SymNet class

#ifdef _CFRONT_BUG
class SymNet : public Strstuffs {
#else
class SymNet : protected Strstuffs {
#endif
friend class Sections;
friend class SymFile;
friend class SymFwdrefTypes;
friend class CharOffs;
friend class SymTypes;
friend class SymRoot;
friend class SymDump;
public: 
    SymNet();
    virtual ~SymNet();
	virtual SymErr ISymbolics(const StringPtr symbolicsFile) = 0;

// contract member functions for ABC
	//error handling
    SymErr Error();      // returns last error or 0 if was successfully called
    INLINE SymFileFormat SymFF();
    static SymFileFormat GetSymFF(char* fname);	//figure out what type of file fname is
	INLINE SymErr GetErr(char*& err, char*& fname, uint32& line);
    INLINE char* GetErrStr();
    SymErr Validate();   // reset error to se_success
    Boolean Valid(); 	// returns true if ISymbolics was successfully called
    static Boolean Failed(SymErr e) { return SymError::failed(e); } 	// returns true if e is a fatal error
	//sections and base addresses
	//outside world only knows sections by type...
	void SetBaseAddress(SymSec sectype, unsigned long addr);
	unsigned long GetBaseAddress(SymSec sectype);
	int GetSection(unsigned long addr);	//get section number containing address
	INLINE uint32 GetSectionSize(SymSec sec);
	INLINE SymSec GetSectionType(uint32 addr);
	//source mappings
	SymErr Address2Source(unsigned long addr, StringPtr *filename, uint32 *offset, uint32 *line=0);
	SymErr Source2Address(const StringPtr fnm, uint32 offset, uint32& addr);
	//symbol <-> address mappings
	SymErr AddressOf(StringPtr,unsigned long&); //SOMEDAY. use GetSym!!
	//symbols
	SymEntry* GetSymAtInd(unsigned long sym_idx);
	unsigned long SymbolCount(); //Mac returns mtre_last-mte_first+1 of RTE with type/num CODE/0001... huh??
//who knows... had to add for debugger 
	INLINE unsigned long	ModDate()	const;

// other public funcs
    //types
    SymType* GetNthField(SymStructType* s,int i,uint32& off);
    INLINE int GetNumFields(SymStructType* s);
	INLINE SymErr GetArrayBounds(SymArrayType* at,uint32& upperbound,uint32& lowerbound);
	//type APIs
	INLINE Boolean IsaBasicType(SymType* t);
	INLINE Boolean IsaPointer(SymType* t);
	INLINE Boolean IsaStructure(SymType* t);
	INLINE Boolean IsanArray(SymType* t);
	INLINE Boolean IsaEnum(SymType* t);
	INLINE Boolean IsaFunction(SymType* t);
	INLINE SymCat GetCat(SymType* t);
	INLINE void SetCat(SymType* t,SymCat c);
	INLINE void SetCat(SymType* t,SymType* t2);
	INLINE uint32 GetSize(SymType* t);
	INLINE char* GetName(SymType* t);
	//perhaps I want to hide types like array, etc from outside world?
	SymType* GetRefType(SymType* t);
	SymStructType* GetStructType(SymType* t);
	SymArrayType* GetArrayType(SymType* t);
	SymEnumType* GetEnumType(SymType* t);
	INLINE SymType* GetMembType(SymArrayType* t);
	INLINE SymType* GetMembType(SymEnumType* t);
	int FollowPtrs(SymType*& rt);	//follow pointers to actual type - return pointer count and new type
	int FollowRefs(SymType*& rt);	//follow references to actual type - return pointer count and new type
	SymType* GetType(char* typeName);

	//symbol APIs
	INLINE SymEntry* GetFuncSym(uint32 addr);
	INLINE char* GetModName(uint32 addr);
	INLINE SymEntry* GetSym(char* name);
	INLINE SymEntry* GetSym(uint32 addr);
	INLINE SymEntry* GetCodeSym(uint32 addr);
	INLINE SymEntry* GetDataSym(uint32 addr);
	INLINE char* GetSymName(SymEntry* s);
	INLINE SymType* GetSymType(SymEntry* s);
	SymScope GetSymScope(SymEntry* s,uint32& start,uint32& end);	//returns scope SCOPE_GLOBAL, etc
	SymScope GetSymLife(SymEntry* sym,uint32& start,uint32& end);
	INLINE SymSec GetSymSec(SymEntry* s);		//returns section type of symbol SEC_CODE, etc
	INLINE SymbolClass GetSymClass(SymEntry* s);	//returns storage class of symbol SC_CODE, etc
	INLINE uint32 GetSymAddr(SymEntry* s);
	
    //scopes
    SymErr SetAddrScope(uint32 addr);
    SymErr SetModScope(char* mname);
    SymErr SetFuncScope(char* fname);
    SymErr SetLineScope(uint32 line);

protected:
	//static Heap* _heap;
	//static Heap* _tmp_heap;
    SymFileFormat _symff;
	Sections* _sections;	//sections for holding base addresses
    SymFile* _fp;
    SymError _state; //symbol apis will need to set
    SymRoot* _symroot; //root of symbol table
    int _dumping;
	int _sym_idx;	//for __NEW_ARMSYM__ indexes
	#if 0 	//def __3DO_DEBUGGER__	
		SymType *_hwreg_type;
	#endif /* __3DO_DEBUGGER__ */
    //fundamental types
	SymType* st_void;
	SymType* st_char;
	SymType* st_uchar;
	SymType* st_short;
	SymType* st_ushort;
	SymType* st_int;
	SymType* st_uint;
	SymType* st_long;
	SymType* st_ulong;
	SymType* st_float;
	SymType* st_double;
	SymType* st_ldouble;
	SymType* st_complex;
	SymType* st_dcomplex;
	SymType* st_string;
	SymType* st_function;
	SymType* st_ref;
	SymType* st_pointer;
	static TProgressBar* _progress_bar;


	typedef Boolean (SymNet::*cmp_sym_t)(SymEntry*,void*);	//typedef for symbol compares
	typedef Boolean (SymNet::*cmp_func_t)(SymFuncEntry*,void*);	//typedef for symbol compares
	typedef Boolean (SymNet::*cmp_mod_t)(SymModEntry*,void*);	//typedef for symbol compares
	typedef Boolean (SymNet::*cmp_line_t)(SymLineEntry*,void*);	//typedef for symbol compares
	Boolean cmp_name(SymEntry*s,void*n);
	Boolean cmp_addr(SymEntry*s,void*a);	//addr==symbol address
	Boolean cmp_line_addr(SymLineEntry*l,void*a);	//addr==line address
	//Boolean cmp_line_addrin(SymLineEntry*l,void*a);	//addr follows this line
	Boolean cmp_line_line(SymLineEntry*l,void*a);
	Boolean cmp_func_name(SymFuncEntry*f,void*n);
	Boolean cmp_func_addr(SymFuncEntry*f,void*a);	//addr==start of function
	Boolean cmp_func_addrin(SymFuncEntry*f,void*a);	//addr inside function
	Boolean cmp_func_linein(SymFuncEntry*f,void*a);
	Boolean cmp_mod_name(SymModEntry*m,void*n);
	Boolean cmp_mod_addrin(SymModEntry*m,void*a);
	Boolean cmp_mod_linein(SymModEntry*m,void*a);
	Boolean cmp_idx(SymEntry*s,void*i);
	//SymEntry * search_syms(cmp_sym_t,void*);
	char* rmv_path(const char sep, char* fullname, char*& path);

    //these are called by my kids to read and build me a symbol network
    //add stuff to symbol network
	SymType* get_fun_type(SymCat t);
	uint32 get_type_size(SymCat cat);
    SymType* add_type(const char* name,uint32 size,SymCat cat,void* symt);
    INLINE SymStructType* add_union(const char*n,uint32 size,uint32 nfields=0);
    INLINE SymStructType* add_struct(const char*n,uint32 size,uint32 nfields=0);
    INLINE SymStructType::SymFieldType* add_field(SymStructType* st, uint32 offset, SymType* ftype);
    INLINE SymEnumType* add_enum(const char*n,SymType* base,uint32 count=0);
    INLINE SymEnumType::SymMemberType* add_member(SymEnumType* etype,const char*n,uint32 val);
    INLINE SymArrayType* add_array(const char*n,uint32 size,SymType* base);
    INLINE SymEntry* add_sym(const char* n, SymType* t, uint32 v, SymSec sec,SymScope sp,SymbolClass sc);
    INLINE SymFuncEntry* add_func(SymEntry* s,SymModEntry* m=0);
    INLINE SymModEntry* add_mod(const char* n, uint32 ba, uint32 ea);
    INLINE SymFuncLink* add_lsym(SymEntry* s, SymFuncEntry* f);
	INLINE SymRootLink* add_gsym(SymEntry* s, SymModEntry* m=0);
    INLINE SymRootLink* add_gfunc(SymFuncEntry* f,SymModEntry* m=0);
    INLINE SymModLink* add_msym(SymEntry* s, SymModEntry* m);
    INLINE SymModLink* add_mfunc(SymFuncEntry* f,SymModEntry* m=0);
    INLINE SymFuncEntry* add_block(SymEntry* s,SymFuncEntry* f);
    INLINE SymLineEntry* add_line(SymModEntry* m, uint32 l, uint32 a);
    INLINE SymLineEntry* add_charoff(SymModEntry* m, uint32 c, uint32 a);

    //search stuff in symbol network passing search function
	//search all funcs for name/addr 
    SymEntry* search_code(const char* name);	//search all code 
    SymEntry* search_code(uint32 addr);
    SymEntry* search_data(const char* name);	//search all non-local data
    SymEntry* search_data(uint32 addr);

    SymEntry* search_lsyms(cmp_sym_t,SymFuncEntry *b, void*thing);
    SymEntry* search_gsyms(cmp_sym_t,void*thing);
    SymEntry* search_mgsyms(cmp_sym_t,SymModEntry* m,void*thing);
    SymEntry* search_msyms(cmp_sym_t,SymModEntry* m, void*thing);
    SymFuncEntry* search_blocks(cmp_func_t,SymFuncEntry *f, void*thing);
    SymFuncEntry* search_allblocks(cmp_func_t,void*thing);
    SymFuncEntry* search_allblocks(cmp_func_t,SymModEntry *m, void*thing);
    SymLineEntry* search_lines(cmp_line_t,SymModEntry *m,void*thing);
    SymFuncEntry* search_allfuncs(cmp_func_t,void*thing);
    INLINE SymFuncEntry* search_inmodfuncs(cmp_func_t cmp_func,SymModEntry*m,void*thing);
    SymFuncEntry* search_mfuncs(cmp_func_t,SymModEntry *m, void*thing);
    SymFuncEntry* search_mgfuncs(cmp_func_t,SymModEntry *m, void*thing);
    SymFuncEntry* search_gfuncs(cmp_func_t,void*thing);
    SymModEntry* search_mods(cmp_mod_t,void*thing);
    SymEntry* search_code(cmp_func_t,void*thing);
    SymEntry* search_data(cmp_sym_t,void*thing);
    SymEntry* search_scope(cmp_sym_t,void*thing);
    SymEntry* search_scope_data(cmp_sym_t,void*thing);
    SymEntry* search_scope_code(cmp_func_t,void*thing);
    //search stuff in symbol network
    SymType* search_types(const char*);
	//search in scope for any symbol with name/addr 
    SymEntry* search_scope(const char* name);
    SymEntry* search_scope(uint32 addr);
    INLINE SymEntry* search_scope_data(const char* name); //search global&local
    INLINE SymEntry* search_scope_data(uint32 addr);
    INLINE SymEntry* search_scope_code(const char* name); //search global&local
    INLINE SymEntry* search_scope_code(uint32 addr);
    INLINE SymFuncEntry* search_allfuncs(const char* name);
    INLINE SymFuncEntry* search_allfuncs(uint32 addr);
    INLINE SymFuncEntry* search_inallfuncs(uint32 addr);
    INLINE SymFuncEntry* search_inmodfuncs(uint32 addr,SymModEntry*m);
    INLINE SymModEntry* search_inmods(uint32 addr);
    INLINE SymModEntry* search_mods(const char* name);
	#if 0 //def __3DO_DEBUGGER__ - debugger does this now	
		//only need to add for special regs names - what are they?
		uint32 hwreg(uint32 r);
	#endif /* __3DO_DEBUGGER__ */
    unsigned long rel2abs(SymEntry*);
    unsigned long abs2rel(SymEntry*);
	uint32 rel2abs(SymSec sectype,uint32 addr);
	uint32 abs2rel(SymSec sectype,uint32 addr);
    INLINE uint32 beg_addr(SymFuncEntry* f);
    INLINE uint32 end_addr(SymFuncEntry* f);
    INLINE uint32 beg_addr(SymModEntry* m);
    INLINE uint32 end_addr(SymModEntry* m);
    INLINE uint32 addr(SymLineEntry* l);
    Boolean inside_rel(uint32 addr,SymModEntry* m);
    Boolean inside_rel(uint32 addr,SymFuncEntry* f);
    Boolean inside(uint32 addr,SymModEntry* m);
    Boolean inside(uint32 addr,SymFuncEntry* f);

	//void* sym_new(void* x,char* file,uint32 line);
	//void sym_delete(unsigned char* p,char* file,uint32 line);
	INLINE void set_state(SymErr err, const char* file, uint32 line);
	void set_err_msg(const char* fmt, ...);
	INLINE SymErr state();
	//unsigned char* sym_malloc(uint32 size,int align,char* file,uint32 line);
	uint32 sym_fread(unsigned char* buf,uint32 size,uint32 n,FILE* fp, const char* file, uint32 line);
	uint32 sym_fseek(uint32 off,uint32 pos,FILE* fp, const char* file,uint32 line);
    
private:
	SymNetScope cur_scope;
    //scopes
	INLINE SymFuncEntry* cur_func();
	INLINE SymFuncEntry* cur_block();
	INLINE SymModEntry* cur_mod();
	INLINE void set_cur_func(SymFuncEntry* func);
	INLINE void set_cur_block(SymFuncEntry* block);
	INLINE void set_cur_mod(SymModEntry* mod);
	//unsigned long convertTypeCode(SymType* sym_type);
	SymEntry * symcounter(unsigned long& sym_idx);
    };
    
#if !defined(USE_DUMP_FILE)
#include "symnet_inlines.h"
#endif
    	
#endif /* SYMAPI__H */

