/*  @(#) elf_tags.cpp 96/07/25 1.25 */

/*

	File:		elf_tags.cpp

	Written by:	John R. McMullen

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by The 3DO Company.

	Change History (most recent first):

		 <4>	96/01/17	JRM		Add header
		<20>	96/01/07	JRM		"Seven" bug.  A legitimate situation was being treated as an
									error.

	To Do:

*/

//====================================================================
// elf_tags.cpp  -  tag funcs for ElfReader class
//		for reading & parsing debug tag entries based on dwarf 
//		attributes, advancing buffer, and building symbol nodes 
//		as we go...

#ifndef USE_DUMP_FILE
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "utils.h"
#include "elf_r.h"
#endif /* USE_DUMP_FILE */

#include "dwarf.h"
#include "debug.h"

#pragma segment elfsym

//==========================================================================
// sub-funcions for building symbols 
//		pases dwarf info based on attribute

SymErr ElfReader::build_array_type(BufEaters* buf) {
	uint16 attr;
	uint16 order;
	char* name=0;
	SymType* type=0;
	uint32 size=0, bit_offset=0, bit_size=0, val=0;
	SymSec sec=sec_none; SymbolClass sclass=sc_none;
	uint32 len;
	uint8* subscr;
	int low_bound=0, high_bound=0;
	while (buf->bites_left())
	{
		attr = buf->eat_uint16();
		attr_val aval(attr,buf);	//gets the attr's value based on the form of the attr
		switch (attr)
		{
		case AT_sibling:
			DBG_(ELF,("AT_sibling\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_name:
			DBG_(ELF,("AT_name\n"));
			name=aval.str;
			break;
		case AT_ordering:
			DBG_(ELF,("AT_ordering\n"));
			order = aval.con.s16;	//either ORD_col_major or ORD_row_major
			DBG_(ELF,("order = x%X\n",order));
			break;
		case AT_subscr_data:		//contains ranges & types
			DBG_(ELF,("AT_subscr_data\n"));
			len = aval.block.len.s16;
			subscr = aval.block.ptr;
			DBG_(ELF,("subscr_data:(len=x%X, block=",len));
			DBG_DUMP_(ELF,len,subscr);
			parse_subscr(len,subscr,type,low_bound,high_bound);
			break;
		case AT_byte_size:
			DBG_(ELF,("AT_byte_size\n"));
			size = aval.con.s32;	//currently getting from type, so ignored
			break;
		case AT_private:
			DBG_(ELF,("AT_private\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_protected:
			DBG_(ELF,("AT_protected\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_public:
			DBG_(ELF,("AT_public\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_start_scope:
			DBG_(ELF,("AT_start_scope\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_stride_size:
			DBG_(ELF,("AT_stride_size\n"));
			DBG_WARN(("ignored\n"));
			break;
		default:
			SET_ERR(se_unknown_type,("unknown or invalid attribute x%X\n",attr));
		}
	} // while
	//DBG_ASSERT(type);
	//if (!type) 
	//	RETURN_ERR(se_fail,("build_array_type: parse_subscr failed!\n"));
	
	//given subscr, name, size, order...
	SymArrayType* at = add_array(name,size,type);
	at->set_bounds(low_bound,high_bound);
	//for resolving ref types later
	add_ref_type(elf_offset,at->type());
	return state();
	}

SymErr ElfReader::build_class_type(BufEaters* buf) {
	uint16 attr;
	DBG_WARN(("not implemented\n"));
	while (buf->bites_left()) {
		attr = buf->eat_uint16();
		attr_val aval(attr,buf);	//gets the attr's value based on the form of the attr
		switch (attr) {
		case AT_sibling:
			DBG_(ELF,("AT_sibling\n"));
			break;
		case AT_name:
			DBG_(ELF,("AT_name\n"));
			break;
		case AT_byte_size:
			DBG_(ELF,("AT_byte_size\n"));
			break;
		case AT_friends:
			DBG_(ELF,("AT_friends\n"));
			break;
		case AT_private:
			DBG_(ELF,("AT_private\n"));
			break;
		case AT_protected:
			DBG_(ELF,("AT_protected\n"));
			break;
		case AT_public:
			DBG_(ELF,("AT_public\n"));
			break;
		case AT_start_scope:
			DBG_(ELF,("AT_start_scope\n"));
			break;
		default:
			SET_ERR(se_unknown_type,("unknown or invalid attribute x%X\n",attr));
		}
		}
	return state();
	}
SymErr ElfReader::build_entry_point(BufEaters* buf) {
	uint16 attr;
	DBG_WARN(("not implemented\n"));
	while (buf->bites_left()) {
		attr = buf->eat_uint16();
		attr_val aval(attr,buf);	//gets the attr's value based on the form of the attr
		switch (attr) {
		case AT_sibling:
			DBG_(ELF,("AT_sibling\n"));
			break;
		case AT_name:
			DBG_(ELF,("AT_name\n"));
			break;
		case AT_low_pc:
			DBG_(ELF,("AT_low_pc\n"));
			break;
		case AT_return_addr:
			DBG_(ELF,("AT_return_addr\n"));
			break;
		case AT_fund_type:
			DBG_(ELF,("AT_fund_type\n"));
			break;
		case AT_mod_fund_type:
			DBG_(ELF,("AT_mod_fund_type\n"));
			break;
		case AT_user_def_type:
			DBG_(ELF,("AT_user_def_type\n"));
			break;
		case AT_mod_u_d_type:
			DBG_(ELF,("AT_mod_u_d_type\n"));
			break;
		default:
			SET_ERR(se_unknown_type,("unknown or invalid attribute x%X\n",attr));
		}
		}
	return state();
	}
SymErr ElfReader::build_enumeration_type(BufEaters* buf) {
	uint16 attr;
	uint32 len=0;
	uint32 size=0;
	uint32 ref=0;
	uint8* list=0;
	Stack estack;
	char* name=0;
	SymType* type=0;
	while (buf->bites_left()) {
		attr = buf->eat_uint16();
		attr_val aval(attr,buf);	//gets the attr's value based on the form of the attr
		switch (attr) {
		case AT_sibling:
			DBG_(ELF,("AT_sibling\n"));
			ref = aval.con.s32;	
			break;
		case AT_name:
			DBG_(ELF,("AT_name\n"));
			name = aval.str;
			break;
		case AT_byte_size: {
			DBG_(ELF,("AT_byte_size\n"));
			size = aval.con.s32;	//currently getting from type, so ignored
			switch (size) {
				case 4: type = get_fun_type(tc_ulong); break;
				case 2: type = get_fun_type(tc_ushort); break;
				case 1: type = get_fun_type(tc_uchar); break;
				default: 
					SET_ERR(se_fail,("invalid size for enum; size=x%X\n",size));
				}
			}
			break;
		case AT_element_list:
			DBG_(ELF,("AT_element_list\n"));	
					//list of data elements stored in a block of continuous bytes
					//each data item - const, block of bytes, null
					//const contains internal of the enum elem
					//null terminated block holds the enum literal string
					//(generated in reverse order)
			len = aval.block.len.s32;
			list = aval.block.ptr;
			DBG_(ELF,("element_list: len=x%X, block=",len));
			DBG_DUMP_(ELF,len,list);
			parse_enum_list(len,list,&estack);
			break;
		case AT_private:
			DBG_(ELF,("AT_private\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_protected:
			DBG_(ELF,("AT_protected\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_public:
			DBG_(ELF,("AT_public\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_start_scope:
			DBG_(ELF,("AT_start_scope\n"));
			DBG_WARN(("ignored\n"));
			break;
		default:
			SET_ERR(se_unknown_type,("unknown or invalid attribute x%X\n",attr));
		}
		}
	DBG_ASSERT(type);
	if (!type) {
		RETURN_ERR(se_fail,("# Unable to get type for enum\n"));
		}
	char* ename;
	int count = estack.num();
	SymEnumType* etype = add_enum(name,type,count);
	if (!etype) {
		SET_ERR(se_bad_call,("# Unable to create add_enum\n"));
		return state();
		}
	_cur_type->push(etype,elf_offset,ref);
	//for resolving ref types later
	add_ref_type(elf_offset,etype->type());
	//inefficient....
	//_cur_type now handles stack of members, so we should delete them from here
	SymType* mtype;
	for (int i=0; i<count && !estack.empty(); i++) {
		uint32 n,val;
		estack.pop(n,val);
		ename = (char*) n;
		//was: etype->add_member(e,ename,val);
		mtype = add_type(ename,size,tc_member,type);
		_cur_type->add_member(val,mtype);
		}
	_cur_type->pop();	//enums don't have to wait for a null entry for end of list
	return state();
	}
SymErr ElfReader::build_formal_parameter(BufEaters* buf) {
	uint16 attr;
	char* name=0;
	uint32 val=0;
	SymType* type=0;
	SymSec sec=sec_none;
	SymbolClass sclass=sc_none;;	//sc_reg/sc_stack
	while (buf->bites_left()) {
		attr = buf->eat_uint16();
		attr_val aval(attr,buf);	//gets the attr's value based on the form of the attr
		switch (attr) {
		case AT_sibling:
			DBG_(ELF,("AT_sibling\n"));
			break;
		case AT_location:
			DBG_(ELF,("AT_location\n"));
			parse_loc(aval.block.len.s16,aval.block.ptr,val,sec,sclass);
			break;
		case AT_name:
			DBG_(ELF,("AT_name\n"));
			name = aval.str;
			break;
		case AT_default_value_addr:
			DBG_(ELF,("AT_default_value_addr\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_default_value_data2:
			DBG_(ELF,("AT_default_value_data2\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_default_value_data4:
			DBG_(ELF,("AT_default_value_data4\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_default_value_data8:
			DBG_(ELF,("AT_default_value_data8\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_default_value_string:
			DBG_(ELF,("AT_default_value_string\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_is_optional:
			DBG_(ELF,("AT_is_optional\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_fund_type:
			DBG_(ELF,("AT_fund_type\n"));
			type = get_type(aval.con.s16);
			break;
		case AT_mod_fund_type:
			DBG_(ELF,("AT_mod_fund_type\n"));
			type = get_mod_type(aval.block.len.s16,aval.block.ptr);
			break;
		case AT_user_def_type:
			DBG_(ELF,("AT_user_def_type\n"));
			type = get_user_type(aval.ref);
			break;
		case AT_mod_u_d_type:
			DBG_(ELF,("AT_mod_u_d_type\n"));
			type = get_mod_user_type(aval.block.len.s16,aval.block.ptr);
			break;
		default:
			SET_ERR(se_unknown_type,("unknown or invalid attribute x%X\n",attr));
		}
		}

	//we can get these things on either func defs or func typedefs,
	//so we need to check the context here... 
	//kludge - need a better way to find context
	if (!_cur_type->empty() && !_cur_type->isa(tc_function)) {
		DBG_WARN(("can't support arguments in types - not implemented.\n"));
		return state();
		}
	if (!_cur_func) {
		//SET_ERR(se_bad_call,("no cur_func to add symbol to!\n"));
		DBG_FIXME(("Ignoring parm - must be a typedef??? no cur_func to add symbol to!\n"));
		return state();
		}
	SymEntry* sym;
	sym=add_sym(name,type,val,sec,scope_local,sclass);
	add_lsym(sym,_cur_func);
	return state();
}

//-------------------------------------------
SymErr ElfReader::build_global_subroutine(BufEaters* buf)
//-------------------------------------------
{
	char* name=0;
	uint32 baddr=0, eaddr=0;
	SymType* type=0;	//SOMEDAY: need return type here...
	uint16 attr;
	uint32 ref=0;
	while (buf->bites_left()) {
		attr = buf->eat_uint16();
		attr_val aval(attr,buf);	//gets the attr's value based on the form of the attr
		switch (attr) {
		case AT_sibling:
			DBG_(ELF,("AT_sibling\n"));
			ref = aval.con.s32;	
			break;
		case AT_location:
			DBG_(ELF,("AT_location\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_name:
			DBG_(ELF,("AT_name\n"));
			name=aval.str;
			break;
		case AT_low_pc:
			DBG_(ELF,("AT_low_pc\n"));
			baddr=aval.addr;
			break;
		case AT_high_pc:
			DBG_(ELF,("AT_high_pc\n"));
			eaddr=aval.addr;
			break;
		case AT_member:
			DBG_(ELF,("AT_member\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_inline:
			DBG_(ELF,("AT_inline\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_private:
			DBG_(ELF,("AT_private\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_protected:
			DBG_(ELF,("AT_protected\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_prototyped:
			DBG_(ELF,("AT_prototyped\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_public:
			DBG_(ELF,("AT_public\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_pure_virtual:
			DBG_(ELF,("AT_pure_virtual\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_return_addr:
			DBG_(ELF,("AT_return_addr\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_start_scope:
			DBG_(ELF,("AT_start_scope\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_virtual:
			DBG_(ELF,("AT_virtual\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_fund_type:
			DBG_(ELF,("AT_fund_type\n"));
			type = get_type(aval.con.s16);
			break;
		case AT_mod_fund_type:
			DBG_(ELF,("AT_mod_fund_type\n"));
			type = get_mod_type(aval.block.len.s16,aval.block.ptr);
			break;
		case AT_user_def_type:
			DBG_(ELF,("AT_user_def_type\n"));
			type = get_user_type(aval.ref);
			break;
		case AT_mod_u_d_type:
			DBG_(ELF,("AT_mod_u_d_type\n"));
			type = get_mod_user_type(aval.block.len.s16,aval.block.ptr);
			break;
		//Diab C++ extentions
		case AT_mangled_name:
			DBG_(ELF,("AT_mangled_name\n"));
			name=aval.str;	//mangled name will override name
			break;
		case AT_glob_cpp:
			DBG_(ELF,("AT_glob_app\n"));
			DBG_FIXME(("what's this?\n"));
			break;
		default:
			SET_ERR(se_unknown_type,("unknown or invalid attribute x%X\n",attr));
		}
		}
	SymEntry* sym;
	SymFuncEntry* f;
	uint32 val=baddr;
    baddr=abs2rel(sec_code,baddr); //relative offset
    eaddr=abs2rel(sec_code,eaddr); //relative offset
    DBG_ASSERT(_cur_mod);
	if (!_cur_mod) {
		SET_ERR(se_bad_call,("no cur_mod to add func to!\n"));
		return state();
		}
	DBG_ASSERT(baddr>=_cur_mod->baddr() && eaddr<=_cur_mod->eaddr());
	//global symbols added when reading symbol table - get it 
	if (!name || !(name && (f=search_allfuncs(name),f))) {
		sym=add_sym(name,type,val,sec_code,scope_global,sc_code);
		f = add_func(sym,_cur_mod);
		}
	else {
		DBG_WARN(("func was already added\n"));
		f->sym()->set_type(type);
		}
	add_gfunc(f,_cur_mod);
    f->set_baddr(baddr); //relative offset
	f->set_eaddr(eaddr);
	_cur_func=f;
	_cur_block=0;
	_cur_type->push(_cur_func,elf_offset,ref);
	return state();
}

SymErr ElfReader::build_subroutine(BufEaters* buf)
{
	char* name=0;
	uint32 baddr=0, eaddr=0;
	SymType* type=0;	//SOMEDAY: need return type here...
	uint16 attr;
	while (buf->bites_left()) {
		attr = buf->eat_uint16();
		attr_val aval(attr,buf);	//gets the attr's value based on the form of the attr
		switch (attr) {
		case AT_sibling:
			DBG_(ELF,("AT_sibling\n"));
			break;
		case AT_name:
			DBG_(ELF,("AT_name\n"));
			name=aval.str;
			break;
		case AT_low_pc:
			DBG_(ELF,("AT_low_pc\n"));
			baddr=aval.addr;
			break;
		case AT_high_pc:
			DBG_(ELF,("AT_high_pc\n"));
			eaddr=aval.addr;
			break;
		case AT_member:
			DBG_(ELF,("AT_member\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_inline:
			DBG_(ELF,("AT_inline\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_private:
			DBG_(ELF,("AT_private\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_protected:
			DBG_(ELF,("AT_protected\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_prototyped:
			DBG_(ELF,("AT_prototyped\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_public:
			DBG_(ELF,("AT_public\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_return_addr:
			DBG_(ELF,("AT_return_addr\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_start_scope:
			DBG_(ELF,("AT_start_scope\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_fund_type:
			DBG_(ELF,("AT_fund_type\n"));
			type = get_type(aval.con.s16);
			break;
		case AT_mod_fund_type:
			DBG_(ELF,("AT_mod_fund_type\n"));
			type = get_mod_type(aval.block.len.s16,aval.block.ptr);
			break;
		case AT_user_def_type:
			DBG_(ELF,("AT_user_def_type\n"));
			type = get_user_type(aval.ref);
			break;
		case AT_mod_u_d_type:
			DBG_(ELF,("AT_mod_u_d_type\n"));
			type = get_mod_user_type(aval.block.len.s16,aval.block.ptr);
			break;
		//Diab C++ extentions
		case AT_mangled_name:
			DBG_(ELF,("AT_mangled_name\n"));
			name=aval.str;	//mangled name will override name
			break;
		default:
			SET_ERR(se_unknown_type,("unknown or invalid attribute x%X\n",attr));
		}
		}
	SymFuncEntry* f;
	SymEntry* sym;
	DBG_ASSERT(_cur_mod);
	if (!_cur_mod) {
		SET_ERR(se_bad_call,("no cur_mod to add func to!\n"));
		return state();
		}
	uint32 val=baddr;
    baddr=abs2rel(sec_code,baddr); //relative offset
    eaddr=abs2rel(sec_code,eaddr); //relative offset
	DBG_ASSERT(baddr>=_cur_mod->baddr() && eaddr<=_cur_mod->eaddr());
	//global symbols added when reading symbol table - get it 
	if (!name || !(name && (f=search_allfuncs(name),f))) {
		sym=add_sym(name,type,val,sec_code,scope_module,sc_code);
		f = add_func(sym,_cur_mod);
		}
	else {
		DBG_WARN(("func was already added\n"));
		f->sym()->set_type(type);
		f->sym()->set_scope(scope_module);
		}
	add_mfunc(f,_cur_mod);
    f->set_baddr(baddr); //relative offset
    f->set_eaddr(eaddr); //relative offset
	_cur_func=f;
	_cur_block=0;
	return state();
	}
SymErr ElfReader::build_subroutine_type(BufEaters* buf) {
	uint16 attr;
	char* name=0;
	SymType* type=0;
	uint32 size=0, bit_offset=0, bit_size=0, val=0;
	uint32 ref=0;
	int low_bound=0, high_bound=0;
	DBG_WARN(("not implemented\n"));
	while (buf->bites_left()) {
		attr = buf->eat_uint16();
		attr_val aval(attr,buf);	//gets the attr's value based on the form of the attr
		switch (attr) {
		case AT_sibling:
			DBG_(ELF,("AT_sibling\n"));
			ref = aval.con.s32;	
			break;
		case AT_name:
			DBG_(ELF,("AT_name\n"));
			name=aval.str;
			break;
		case AT_private:
			DBG_(ELF,("AT_private\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_protected:
			DBG_(ELF,("AT_protected\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_prototyped:
			DBG_(ELF,("AT_prototyped\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_public:
			DBG_(ELF,("AT_public\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_start_scope:
			DBG_(ELF,("AT_start_scope\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_fund_type:
			DBG_(ELF,("AT_fund_type\n"));
			type = get_type(aval.con.s16);
			break;
		case AT_mod_fund_type:
			DBG_(ELF,("AT_mod_fund_type\n"));
			type = get_mod_type(aval.block.len.s16,aval.block.ptr);
			break;
		case AT_user_def_type:
			DBG_(ELF,("AT_user_def_type\n"));
			type = get_user_type(aval.ref);
			break;
		case AT_mod_u_d_type:
			DBG_(ELF,("AT_mod_u_d_type\n"));
			type = get_mod_user_type(aval.block.len.s16,aval.block.ptr);
			break;
		default:
			SET_ERR(se_unknown_type,("unknown or invalid attribute x%X\n",attr));
		}
		}
	DBG_ASSERT(type);
	type = add_type(name,size,tc_function,type);	
	//push down a level since we'll get a null entry when we're 
	//done with the complete definition
	_cur_type->push(type,elf_offset,ref);
	//for resolving ref types later
	add_ref_type(elf_offset,type);
	return state();
	}
SymErr ElfReader::build_global_variable(BufEaters* buf) {
	char* name=0;
	uint16 attr;
	uint32 val=0;
	SymSec sec=sec_data;
	SymbolClass sclass=sc_data;	//sc_bss/sc_data
	SymType* type=0;
	while (buf->bites_left()) {
		attr = buf->eat_uint16();
		attr_val aval(attr,buf);	//gets the attr's value based on the form of the attr
		switch (attr) {
		case AT_sibling:
			DBG_(ELF,("AT_sibling\n"));
			break;
		case AT_location:
			DBG_(ELF,("AT_location\n"));
			parse_loc(aval.block.len.s16,aval.block.ptr,val,sec,sclass);
			break;
		case AT_name:
			DBG_(ELF,("AT_name\n"));
			name=aval.str;
			break;
		case AT_member:
			DBG_(ELF,("AT_member\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_const_value_string:
			DBG_(ELF,("AT_const_value_string\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_const_value_data2:
			DBG_(ELF,("AT_const_value_data2\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_const_value_data4:
			DBG_(ELF,("AT_const_value_data4\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_const_value_data8:
			DBG_(ELF,("AT_const_value_data8\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_const_value_block2:
			DBG_(ELF,("AT_const_value_block2\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_const_value_block4:
			DBG_(ELF,("AT_const_value_block4\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_private:
			DBG_(ELF,("AT_private\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_protected:
			DBG_(ELF,("AT_protected\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_public:
			DBG_(ELF,("AT_public\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_start_scope:
			DBG_(ELF,("AT_start_scope\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_fund_type:
			DBG_(ELF,("AT_fund_type\n"));
			type = get_type(aval.con.s16);
			break;
		case AT_mod_fund_type:
			DBG_(ELF,("AT_mod_fund_type\n"));
			type = get_mod_type(aval.block.len.s16,aval.block.ptr);
			break;
		case AT_user_def_type:
			DBG_(ELF,("AT_user_def_type\n"));
			type = get_user_type(aval.ref);
			break;
		case AT_mod_u_d_type:
			DBG_(ELF,("AT_mod_u_d_type\n"));
			type = get_mod_user_type(aval.block.len.s16,aval.block.ptr);
			break;
		default:
			SET_ERR(se_unknown_type,("unknown or invalid attribute x%X\n",attr));
		}
		}
	SymEntry* sym;
	//global variables added when reading symbol table - get it 
	if (!name || !(name && (sym=search_data(name),sym))) {
		sym=add_sym(name,type,val,sec,scope_global,sclass);
		}
	else {
		DBG_WARN(("symbol already added\n"));
		sym->set_type(type);
		DBG_WARN(("new val=x%X, new rel_val=x%X, old val=x%X\n",val,abs2rel(sec_data,val),sym->val()));
		//why do we need to set val?  should have done that when read symtab
		//can't assume it's data!! could be bss!!
		//sym->set_val(abs2rel(sec_data,val));
		//sym->set_sec(sec_data);
		//sym->set_stgclass(sc_data);
		}
	add_gsym(sym,_cur_mod);
	return state();
	}
SymErr ElfReader::build_label(BufEaters* buf) {
	uint16 attr;
	DBG_WARN(("not implemented\n"));
	while (buf->bites_left()) {
		attr = buf->eat_uint16();
		attr_val aval(attr,buf);	//gets the attr's value based on the form of the attr
		switch (attr) {
		case AT_sibling:
			DBG_(ELF,("AT_sibling\n"));
			break;
		case AT_name:
			DBG_(ELF,("AT_name\n"));
			break;
		case AT_low_pc:
			DBG_(ELF,("AT_low_pc\n"));
			break;
		case AT_start_scope:
			DBG_(ELF,("AT_start_scope\n"));
			break;
		default:
			SET_ERR(se_unknown_type,("unknown or invalid attribute x%X\n",attr));
		}
		}
	return state();
	}
SymErr ElfReader::build_lexical_block(BufEaters* buf) {
	uint32 baddr=0, eaddr=0;
	uint16 attr;
	while (buf->bites_left()) {
		attr = buf->eat_uint16();
		attr_val aval(attr,buf);	//gets the attr's value based on the form of the attr
		switch (attr) {
		case AT_sibling:
			DBG_(ELF,("AT_sibling\n"));
			break;
		case AT_name:
			DBG_(ELF,("AT_name\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_low_pc:
			DBG_(ELF,("AT_low_pc\n"));
			baddr=aval.addr;
			break;
		case AT_high_pc:
			DBG_(ELF,("AT_high_pc\n"));
			eaddr=aval.addr;
			break;
		default:
			SET_ERR(se_unknown_type,("unknown or invalid attribute x%X\n",attr));
		}
		}
	SymFuncEntry* f=0, *b;
	SymEntry* sym;
	DBG_ASSERT(_cur_func);
	if (!_cur_func) {
		printf("no cur_func to add block to!\n");
		// Do NOT set the error, as there can be code (as in the Seven "d7" bug)
		//  in which this occurs legitimately.
		// SET_ERR(se_bad_call,("no cur_func to add block to!\n"));
		return state();
		}
	uint32 val=baddr;
    baddr=abs2rel(sec_code,baddr); //relative offset
    eaddr=abs2rel(sec_code,eaddr); //relative offset
	DBG_ASSERT(baddr>=_cur_func->baddr() && eaddr<=_cur_func->eaddr());
	sym=add_sym(0,0,val,sec_code,scope_local,sc_code);
	if (_cur_block) 
		for (b=_cur_block; b; b=b->func()) {	//search for parent block/function
			if (baddr>=b->baddr() && eaddr<=b->eaddr()) 
				f = add_block(sym,b);
			}
	else
		f = add_block(sym,_cur_func);
	DBG_ASSERT(f);
	if (!f) {
		SET_ERR(se_bad_call,("no f to for cur_func!\n"));
		return state();
		}
    f->set_baddr(baddr); //relative offset
    f->set_eaddr(eaddr); //relative offset
	_cur_block=f;
	return state();
	}
SymErr ElfReader::build_local_variable(BufEaters* buf) {
	uint16 attr;
	char* name=0;
	uint32 val=0;
	SymType* type=0;
	SymSec sec=sec_none;
	SymbolClass sclass=sc_none;;	//sc_reg/sc_stack
	while (buf->bites_left()) {
		attr = buf->eat_uint16();
		attr_val aval(attr,buf);	//gets the attr's value based on the form of the attr
		switch (attr) {
		case AT_sibling:
			DBG_(ELF,("AT_sibling\n"));
			break;
		case AT_location:
			DBG_(ELF,("AT_location\n"));	//get section and val 
			parse_loc(aval.block.len.s16,aval.block.ptr,val,sec,sclass);
			break;
		case AT_name:
			DBG_(ELF,("AT_name\n"));
			name=aval.str;
			break;
		case AT_const_value_string:
			DBG_(ELF,("AT_const_value_string\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_const_value_data2:
			DBG_(ELF,("AT_const_value_data2\n"));
			DBG_WARN(("ignored\n"));
			//val = aval.con.s16;
			break;
		case AT_const_value_data4:
			DBG_(ELF,("AT_const_value_data4\n"));
			DBG_WARN(("ignored\n"));
			//val = aval.con.s32;
			break;
		case AT_const_value_data8:
			DBG_(ELF,("AT_const_value_data8\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_const_value_block2:
			DBG_(ELF,("AT_const_value_block2\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_const_value_block4:
			DBG_(ELF,("AT_const_value_block4\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_private:
			DBG_(ELF,("AT_private\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_protected:
			DBG_(ELF,("AT_protected\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_public:
			DBG_(ELF,("AT_public\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_start_scope:
			DBG_(ELF,("AT_start_scope\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_fund_type:
			DBG_(ELF,("AT_fund_type\n"));
			type = get_type(aval.con.s16);
			break;
		case AT_mod_fund_type:
			DBG_(ELF,("AT_mod_fund_type\n"));
			type = get_mod_type(aval.block.len.s16,aval.block.ptr);
			break;
		case AT_user_def_type:
			DBG_(ELF,("AT_user_def_type\n"));
			type = get_user_type(aval.ref);
			break;
		case AT_mod_u_d_type:
			DBG_(ELF,("AT_mod_u_d_type\n"));
			type = get_mod_user_type(aval.block.len.s16,aval.block.ptr);
			break;
		default:
			SET_ERR(se_unknown_type,("unknown or invalid attribute x%X\n",attr));
		}
		}
	SymEntry* sym;
	DBG_ASSERT(_cur_func || _cur_mod || _cur_block);

	if (_cur_block) {
		sym=add_sym(name,type,val,sec,scope_local,sclass);
		add_lsym(sym,_cur_block);
		}
	else if (_cur_func) {
		sym=add_sym(name,type,val,sec,scope_local,sclass);
		add_lsym(sym,_cur_func);
		}
	else if (_cur_mod) {
		sym=add_sym(name,type,val,sec,scope_module,sclass);
		add_msym(sym,_cur_mod);
		}
	else {
		SET_ERR(se_bad_call,("no cur_func to add local sym to!\n"));
		}
	return state();
	}
	
static Stack* stack_o_members=0;
SymErr ElfReader::build_member(BufEaters* buf) {
	uint16 attr;
	char* name=0;
	SymType* type=0;
	uint32 size=0, bit_offset=0, bit_size=0, val=0;
	SymSec sec=sec_none; SymbolClass sclass=sc_none;
	while (buf->bites_left()) {
		attr = buf->eat_uint16();
		attr_val aval(attr,buf);	//gets the attr's value based on the form of the attr
		switch (attr) {
		case AT_sibling:
			DBG_(ELF,("AT_sibling\n"));
			break;
		case AT_location:
			DBG_(ELF,("AT_location\n"));
			parse_loc(aval.block.len.s16,aval.block.ptr,val,sec,sclass);
			break;
		case AT_name:
			DBG_(ELF,("AT_name\n"));
			name=aval.str;
			break;
		case AT_byte_size:
			DBG_(ELF,("AT_byte_size\n"));
			size = aval.con.s32;	//currently getting from type, so ignored
			break;
		case AT_bit_offset:
			DBG_(ELF,("AT_bit_offset\n"));
			DBG_FIXME(("should implement bitfields!\n"));
			bit_offset = aval.con.s32;
			break;
		case AT_bit_size:
			DBG_(ELF,("AT_bit_size\n"));
			DBG_FIXME(("should implement bitfields!\n"));
			bit_size = aval.con.s32;
			break;
		case AT_private:
			DBG_(ELF,("AT_private\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_protected:
			DBG_(ELF,("AT_protected\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_public:
			DBG_(ELF,("AT_public\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_fund_type:
			DBG_(ELF,("AT_fund_type\n"));
			type = get_type(aval.con.s16);
			break;
		case AT_mod_fund_type:
			DBG_(ELF,("AT_mod_fund_type\n"));
			type = get_mod_type(aval.block.len.s16,aval.block.ptr);
			break;
		case AT_user_def_type:
			DBG_(ELF,("AT_user_def_type\n"));
			type = get_user_type(aval.ref);
			break;
		case AT_mod_u_d_type:
			DBG_(ELF,("AT_mod_u_d_type\n"));
			type = get_mod_user_type(aval.block.len.s16,aval.block.ptr);
			break;
		//Diab C++ extentions
		case AT_mangled_name:
			DBG_(ELF,("AT_mangled_name\n"));
			name=aval.str;	//mangled name will override name
			break;
		default:
			SET_ERR(se_unknown_type,("unknown or invalid attribute x%X\n",attr));
		}
		}
	if (_cur_type->empty()) {
		SET_ERR(se_fail,("member found but no active type\n"));
		return state();
		}
	//eat this member
	//was: _cur_type->add_a_member(name,val,size,type);
	type = add_type(name,size,tc_member,type);
	add_ref_type(elf_offset,type);
	_cur_type->add_member(val,type);
		
	return state();
	}
	
SymErr ElfReader::build_pointer_type(BufEaters* buf) {
	uint16 attr;
	DBG_WARN(("not implemented\n"));
	while (buf->bites_left()) {
		attr = buf->eat_uint16();
		attr_val aval(attr,buf);	//gets the attr's value based on the form of the attr
		switch (attr) {
		case AT_sibling:
			DBG_(ELF,("AT_sibling\n"));
			break;
		case AT_name:
			DBG_(ELF,("AT_name\n"));
			break;
		case AT_private:
			DBG_(ELF,("AT_private\n"));
			break;
		case AT_protected:
			DBG_(ELF,("AT_protected\n"));
			break;
		case AT_public:
			DBG_(ELF,("AT_public\n"));
			break;
		case AT_start_scope:
			DBG_(ELF,("AT_start_scope\n"));
			break;
		case AT_fund_type:
			DBG_(ELF,("AT_fund_type\n"));
			break;
		case AT_mod_fund_type:
			DBG_(ELF,("AT_mod_fund_type\n"));
			break;
		case AT_user_def_type:
			DBG_(ELF,("AT_user_def_type\n"));
			break;
		case AT_mod_u_d_type:
			DBG_(ELF,("AT_mod_u_d_type\n"));
			break;
		default:
			SET_ERR(se_unknown_type,("unknown or invalid attribute x%X\n",attr));
		}
		}
	return state();
	}
SymErr ElfReader::build_reference_type(BufEaters* buf) {
	uint16 attr;
	DBG_WARN(("not implemented\n"));
	while (buf->bites_left()) {
		attr = buf->eat_uint16();
		attr_val aval(attr,buf);	//gets the attr's value based on the form of the attr
		switch (attr) {
		case AT_sibling:
			DBG_(ELF,("AT_sibling\n"));
			break;
		case AT_name:
			DBG_(ELF,("AT_name\n"));
			break;
		case AT_private:
			DBG_(ELF,("AT_private\n"));
			break;
		case AT_protected:
			DBG_(ELF,("AT_protected\n"));
			break;
		case AT_public:
			DBG_(ELF,("AT_public\n"));
			break;
		case AT_start_scope:
			DBG_(ELF,("AT_start_scope\n"));
			break;
		case AT_fund_type:
			DBG_(ELF,("AT_fund_type\n"));
			break;
		case AT_mod_fund_type:
			DBG_(ELF,("AT_mod_fund_type\n"));
			break;
		case AT_user_def_type:
			DBG_(ELF,("AT_user_def_type\n"));
			break;
		case AT_mod_u_d_type:
			DBG_(ELF,("AT_mod_u_d_type\n"));
			break;
		default:
			SET_ERR(se_unknown_type,("unknown or invalid attribute x%X\n",attr));
		}
		}
	return state();
	}
SymErr ElfReader::build_compile_unit(BufEaters* buf) {
	char* name=0;
	uint32 baddr=0, eaddr=0;
	uint16 attr;
	uint32 ref=0;
	while (buf->bites_left()) {
		attr = buf->eat_uint16();
		attr_val aval(attr,buf);	//gets the attr's value based on the form of the attr
		switch (attr) {
			case AT_sibling:
				DBG_(ELF,("AT_sibling\n"));
				ref = aval.con.s32;	
				break;
			case AT_name:
				DBG_(ELF,("AT_name\n"));
				name = aval.str;
				break;
			case AT_low_pc:
				DBG_(ELF,("AT_low_pc\n"));
				baddr = aval.addr;
				break;
			case AT_high_pc:
				DBG_(ELF,("AT_high_pc\n"));
				eaddr = aval.addr;
				break;
			case AT_language:
				DBG_(ELF,("AT_language\n"));
				DBG_WARN(("ignored\n"));
				break;
			case AT_comp_dir:
				DBG_(ELF,("AT_comp_dir\n"));
				DBG_WARN(("ignored\n"));
				break;
			case AT_producer:
				DBG_(ELF,("AT_producer\n"));
				DBG_WARN(("ignored\n"));
				break;
			case AT_stmt_list:
				DBG_(ELF,("AT_stmt_list\n"));
				DBG_WARN(("ignored\n"));
				break;
			default:
				SET_ERR(se_unknown_type,("unknown or invalid attribute x%X\n",attr));
			}
		}
	_cur_mod = add_mod(name,baddr,eaddr);
	_cur_func=0;	//just in case
	_cur_block=0;
	_cur_type->push(_cur_mod,elf_offset,ref);
	return state();
	}
SymErr ElfReader::build_common_block(BufEaters* buf) {
	uint16 attr;
	DBG_WARN(("not implemented\n"));
	while (buf->bites_left()) {
		attr = buf->eat_uint16();
		attr_val aval(attr,buf);	//gets the attr's value based on the form of the attr
		switch (attr) {
		case AT_sibling:
			DBG_(ELF,("AT_sibling\n"));
			break;
		case AT_location:
			DBG_(ELF,("AT_location\n"));
			break;
		case AT_name:
			DBG_(ELF,("AT_name\n"));
			break;

		default:
			SET_ERR(se_unknown_type,("unknown or invalid attribute x%X\n",attr));
		}
		}
	return state();
	}
SymErr ElfReader::build_common_inclusion(BufEaters* buf) {
	uint16 attr;
	DBG_WARN(("not implemented\n"));
	while (buf->bites_left()) {
		attr = buf->eat_uint16();
		attr_val aval(attr,buf);	//gets the attr's value based on the form of the attr
		switch (attr) {
		case AT_sibling:
			DBG_(ELF,("AT_sibling\n"));
			break;
		case AT_common_reference:
			DBG_(ELF,("AT_common_reference\n"));
			break;
		default:
			SET_ERR(se_unknown_type,("unknown or invalid attribute x%X\n",attr));
		}
		}
	return state();
	}
SymErr ElfReader::build_inheritance(BufEaters* buf) {
	char* name=0;
	uint16 attr;
	DBG_WARN(("not implemented\n"));
	while (buf->bites_left()) {
		attr = buf->eat_uint16();
		attr_val aval(attr,buf);	//gets the attr's value based on the form of the attr
		switch (attr) {
		case AT_sibling:
			DBG_(ELF,("AT_sibling\n"));
			break;
		case AT_location:
			DBG_(ELF,("AT_location\n"));
			break;
		case AT_user_def_type:
			DBG_(ELF,("AT_user_def_type\n"));
			break;
		case AT_private:
			DBG_(ELF,("AT_private\n"));
			break;
		case AT_protected:
			DBG_(ELF,("AT_protected\n"));
			break;
		case AT_public:
			DBG_(ELF,("AT_public\n"));
			break;
		case AT_virtual:
			DBG_(ELF,("AT_virtual\n"));
			break;
		//Diab C++ extentions
		case AT_name:
			DBG_(ELF,("AT_name\n"));
			name=aval.str;	
			break;
		default:
			SET_ERR(se_unknown_type,("unknown or invalid attribute x%X\n",attr));
		}
		}
	return state();
	}
SymErr ElfReader::build_inlined_subroutine(BufEaters* buf) {
	uint16 attr;
	DBG_WARN(("not implemented\n"));
	while (buf->bites_left()) {
		attr = buf->eat_uint16();
		attr_val aval(attr,buf);	//gets the attr's value based on the form of the attr
		switch (attr) {
		case AT_sibling:
			DBG_(ELF,("AT_sibling\n"));
			break;
		case AT_low_pc:
			DBG_(ELF,("AT_low_pc\n"));
			break;
		case AT_high_pc:
			DBG_(ELF,("AT_high_pc\n"));
			break;
		case AT_specification:	//SOMEDAY:  not in dwarf headers...
			DBG_(ELF,("AT_specification\n"));
			break;
		default:
			SET_ERR(se_unknown_type,("unknown or invalid attribute x%X\n",attr));
		}
		}
	return state();
	}
SymErr ElfReader::build_module(BufEaters* buf) {
	uint16 attr;
	DBG_WARN(("not implemented\n"));
	while (buf->bites_left()) {
		attr = buf->eat_uint16();
		attr_val aval(attr,buf);	//gets the attr's value based on the form of the attr
		switch (attr) {
		case AT_sibling:
			DBG_(ELF,("AT_sibling\n"));
			break;
		case AT_name:
			DBG_(ELF,("AT_name\n"));
			break;
		case AT_low_pc:
			DBG_(ELF,("AT_low_pc\n"));
			break;
		case AT_high_pc:
			DBG_(ELF,("AT_high_pc\n"));
			break;
		case AT_private:
			DBG_(ELF,("AT_private\n"));
			break;
		case AT_protected:
			DBG_(ELF,("AT_protected\n"));
			break;
		case AT_public:
			DBG_(ELF,("AT_public\n"));
			break;
		default:
			SET_ERR(se_unknown_type,("unknown or invalid attribute x%X\n",attr));
		}
		}
	return state();
	}
SymErr ElfReader::build_ptr_to_member_type(BufEaters* buf) {
	uint16 attr;
	DBG_WARN(("not implemented\n"));
	while (buf->bites_left()) {
		attr = buf->eat_uint16();
		attr_val aval(attr,buf);	//gets the attr's value based on the form of the attr
		switch (attr) {
		case AT_sibling:
			DBG_(ELF,("AT_sibling\n"));
			break;
		case AT_name:
			DBG_(ELF,("AT_name\n"));
			break;
		case AT_containing_type:
			DBG_(ELF,("AT_containing_type\n"));
			break;
		case AT_fund_type:
			DBG_(ELF,("AT_fund_type\n"));
			break;
		case AT_mod_fund_type:
			DBG_(ELF,("AT_mod_fund_type\n"));
			break;
		case AT_user_def_type:
			DBG_(ELF,("AT_user_def_type\n"));
			break;
		case AT_mod_u_d_type:
			DBG_(ELF,("AT_mod_u_d_type\n"));
			break;
		default:
			SET_ERR(se_unknown_type,("unknown or invalid attribute x%X\n",attr));
		}
		}
	return state();
	}
SymErr ElfReader::build_set_type(BufEaters* buf) {
	uint16 attr;
	DBG_WARN(("not implemented\n"));
	while (buf->bites_left()) {
		attr = buf->eat_uint16();
		attr_val aval(attr,buf);	//gets the attr's value based on the form of the attr
		switch (attr) {
		case AT_sibling:
			DBG_(ELF,("AT_sibling\n"));
			break;
		case AT_name:
			DBG_(ELF,("AT_name\n"));
			break;
		case AT_private:
			DBG_(ELF,("AT_private\n"));
			break;
		case AT_protected:
			DBG_(ELF,("AT_protected\n"));
			break;
		case AT_prototyped:
			DBG_(ELF,("AT_prototyped\n"));
			break;
		case AT_public:
			DBG_(ELF,("AT_public\n"));
			break;
		case AT_start_scope:
			DBG_(ELF,("AT_start_scope\n"));
			break;
		case AT_fund_type:
			DBG_(ELF,("AT_fund_type\n"));
			break;
		case AT_mod_fund_type:
			DBG_(ELF,("AT_mod_fund_type\n"));
			break;
		case AT_user_def_type:
			DBG_(ELF,("AT_user_def_type\n"));
			break;
		case AT_mod_u_d_type:
			DBG_(ELF,("AT_mod_u_d_type\n"));
			break;
		default:
			SET_ERR(se_unknown_type,("unknown or invalid attribute x%X\n",attr));
		}
		}
	return state();
	}
SymErr ElfReader::build_structure_type(BufEaters* buf) {
	uint16 attr;
	uint32 size=0;
	uint32 ref=0;
	char* name=0;
	while (buf->bites_left()) {
		attr = buf->eat_uint16();
		attr_val aval(attr,buf);	//gets the attr's value based on the form of the attr
		switch (attr) {
		case AT_sibling:
			DBG_(ELF,("AT_sibling\n"));
			ref = aval.con.s32;	
			break;
		case AT_byte_size:
			DBG_(ELF,("AT_byte_size\n"));
			size = aval.con.s32;
			break;
		case AT_name:
			DBG_(ELF,("AT_name\n"));
			name = aval.str;
			break;
		case AT_private:
			DBG_(ELF,("AT_private\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_protected:
			DBG_(ELF,("AT_protected\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_public:
			DBG_(ELF,("AT_public\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_start_scope:
			DBG_(ELF,("AT_start_scope\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_string_length:
			DBG_(ELF,("AT_string_length\n"));
			DBG_WARN(("ignored\n"));
			break;
		default:
			SET_ERR(se_unknown_type,("unknown or invalid attribute x%X\n",attr));
		}
		}
	SymStructType* stype = add_struct(name,size,0);	//need to fill in ref type later
	if (!stype) {
		SET_ERR(se_bad_call,("can't create add_struct!\n"));
		return state();
		}
	_cur_type->push(stype,elf_offset,ref);
	add_ref_type(elf_offset,stype->type());
	return state();
	}
SymErr ElfReader::build_typedef(BufEaters* buf) {
	uint16 attr;
	uint32 size=0;
	char* name=0;
	SymType* type=0;
	while (buf->bites_left()) {
		attr = buf->eat_uint16();
		attr_val aval(attr,buf);	//gets the attr's value based on the form of the attr
		switch (attr) {
		case AT_sibling:
			DBG_(ELF,("AT_sibling\n"));
			break;
		case AT_name:
			DBG_(ELF,("AT_name\n"));
			name=aval.str;
			break;
		case AT_private:
			DBG_(ELF,("AT_private\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_protected:
			DBG_(ELF,("AT_protected\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_public:
			DBG_(ELF,("AT_public\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_start_scope:
			DBG_(ELF,("AT_start_scope\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_fund_type:
			DBG_(ELF,("AT_fund_type\n"));
			type = get_type(aval.con.s16);
			break;
		case AT_mod_fund_type:
			DBG_(ELF,("AT_mod_fund_type\n"));
			type = get_mod_type(aval.block.len.s16,aval.block.ptr);
			break;
		case AT_user_def_type:
			DBG_(ELF,("AT_user_def_type\n"));
			type = get_user_type(aval.ref);
			break;
		case AT_mod_u_d_type:
			DBG_(ELF,("AT_mod_u_d_type\n"));
			type = get_mod_user_type(aval.block.len.s16,aval.block.ptr);
			break;
		default:
			SET_ERR(se_unknown_type,("unknown or invalid attribute x%X\n",attr));
		}
		}
	DBG_ASSERT(type);
	type = add_type(name,size,tc_ref,type);
	//for resolving ref types later
	add_ref_type(elf_offset,type);
	return state();
	}

SymErr ElfReader::build_union_type(BufEaters* buf) {
	uint16 attr;
	uint32 size=0;
	char* name=0;
	uint32 ref=0;
	while (buf->bites_left()) {
		attr = buf->eat_uint16();
		attr_val aval(attr,buf);	//gets the attr's value based on the form of the attr
		switch (attr) {
		case AT_sibling:
			DBG_(ELF,("AT_sibling\n"));
			ref = aval.con.s32;	
			break;
		case AT_byte_size:
			DBG_(ELF,("AT_byte_size\n"));
			size = aval.con.s32;
			break;
		case AT_name:
			DBG_(ELF,("AT_name\n"));
			name=aval.str;
			break;
		case AT_friends:
			DBG_(ELF,("AT_friends\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_private:
			DBG_(ELF,("AT_private\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_protected:
			DBG_(ELF,("AT_protected\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_public:
			DBG_(ELF,("AT_public\n"));
			DBG_WARN(("ignored\n"));
			break;
		case AT_start_scope:
			DBG_(ELF,("AT_start_scope\n"));
			DBG_WARN(("ignored\n"));
			break;
		default:
			SET_ERR(se_unknown_type,("unknown or invalid attribute x%X\n",attr));
		}
		}
	SymStructType* utype = add_union(name,size,0);	//need to fill in ref type later
	if (!utype) {
		SET_ERR(se_bad_call,("can't create add_union!\n"));
		return state();
		}
	_cur_type->push(utype,elf_offset,ref);
	add_ref_type(elf_offset,utype->type());
	return state();
	}
SymErr ElfReader::build_subrange_type(BufEaters* buf) {
	uint16 attr;
	DBG_WARN(("not implemented\n"));
	while (buf->bites_left()) {
		attr = buf->eat_uint16();
		attr_val aval(attr,buf);	//gets the attr's value based on the form of the attr
		switch (attr) {
		case AT_sibling:
			DBG_(ELF,("AT_sibling\n"));
			break;
		case AT_byte_size:
			DBG_(ELF,("AT_byte_size\n"));
			break;
		case AT_name:
			DBG_(ELF,("AT_name\n"));
			break;
		case AT_lower_bound_ref:
			DBG_(ELF,("AT_lower_bound_ref\n"));
			break;
		case AT_lower_bound_data2:
			DBG_(ELF,("AT_lower_bound_data2\n"));
			break;
		case AT_lower_bound_data4:
			DBG_(ELF,("AT_lower_bound_data4\n"));
			break;
		case AT_lower_bound_data8:
			DBG_(ELF,("AT_lower_bound_data8\n"));
			break;
		case AT_upper_bound_ref:
			DBG_(ELF,("AT_upper_bound_ref\n"));
			break;
		case AT_upper_bound_data2:
			DBG_(ELF,("AT_upper_bound_data2\n"));
			break;
		case AT_upper_bound_data4:
			DBG_(ELF,("AT_upper_bound_data4\n"));
			break;
		case AT_upper_bound_data8:
			DBG_(ELF,("AT_upper_bound_data8\n"));
			break;
		case AT_private:
			DBG_(ELF,("AT_private\n"));
			break;
		case AT_protected:
			DBG_(ELF,("AT_protected\n"));
			break;
		case AT_public:
			DBG_(ELF,("AT_public\n"));
			break;
		case AT_fund_type:
			DBG_(ELF,("AT_fund_type\n"));
			break;
		case AT_mod_fund_type:
			DBG_(ELF,("AT_mod_fund_type\n"));
			break;
		case AT_user_def_type:
			DBG_(ELF,("AT_user_def_type\n"));
			break;
		case AT_mod_u_d_type:
			DBG_(ELF,("AT_mod_u_d_type\n"));
			break;
		default:
			SET_ERR(se_unknown_type,("unknown or invalid attribute x%X\n",attr));
		}
		}
	return state();
	}
SymErr ElfReader::build_unspecified_parameters(BufEaters* buf) {
	uint16 attr;
	DBG_WARN(("not implemented\n"));
	while (buf->bites_left()) {
		attr = buf->eat_uint16();
		attr_val aval(attr,buf);	//gets the attr's value based on the form of the attr
		switch (attr) {
		case AT_sibling:
			DBG_(ELF,("AT_sibling\n"));
			break;
		default:
			SET_ERR(se_unknown_type,("unknown or invalid attribute x%X\n",attr));
		}
		}
	return state();
	}
