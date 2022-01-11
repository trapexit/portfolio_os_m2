/*  @(#) elf_attrs.cpp 96/07/25 1.18 */

//====================================================================
// elf_attrs.cpp  -  attr funcs for ElfReader class
//		for reading & parsing debug attributes and 
//		advancing buffer


#ifndef USE_DUMP_FILE
#include "elf_r.h"
#include "strings.h"
#endif /* USE_DUMP_FILE */
#include "debug.h"
#include "dwarf.h"

#pragma segment elfsym

#ifdef _DDI_BUG	//this is non-svr4
					//non-allocable/non-loadable sections like debug should 
					//have the base of the section starting from 0
					//so we have to make relative to debug section...
static uint32 debug_section_base=0;
void set_debug_base(uint32 base);
void set_debug_base(uint32 base) {
	DBG_(ELF,("HACK!!  Diab Data's ref should be offset from debug section!\n"));
	DBG_(ELF,("debug base =0x%X\n",base));
	//don't do if have .rela.debug (in which case offsets will be relative)
	debug_section_base=base;
	}
uint32 get_debug_base();
uint32 get_debug_base() {
	return debug_section_base;
	}
#endif
//==========================================================================
//defines for class attr_val
//		reads attribute's value based on form of attr
//		advances buf, modifies byte_size, and returns requested type

attr_val::attr_val(uint16 attr,BufEaters* buf) {
	switch (attr&FORM_MASK) {
		case FORM_ADDR:
			addr = buf->eat_uint32();
			DBG_(ELF,("FORM_ADDR: x%lx\n",addr));
			break;
		case FORM_REF:
			ref = buf->eat_uint32();
			DBG_(ELF,("FORM_REF: x%lx\n",ref));
	#ifdef _DDI_BUG	//this is non-svr4
					//non-allocable/non-loadable sections like debug should 
					//have the base of the section starting from 0
					//so we have to make relative to debug section...
		DBG_(ELF,("BUG!!  Diab Data's ref should be offset from debug section!\n"));
		//don't do if have rela.debug
		DBG_(ELF,("ref=0x%X, ref-debug_base=0x%X, debug_base =0x%X\n", \
			ref,ref-debug_section_base,debug_section_base));
		ref -= debug_section_base;
	#endif
			break;
		case FORM_BLOCK2:
			block.len.s16 = buf->eat_uint16();
			block.ptr = buf->eat_nbites(block.len.s16);
			DBG_(ELF,("FORM_BLOCK2: len=0x%X, block:\n",block.len.s16));
			DBG_DUMP_(ELF,block.len.s16,block.ptr);
			break;
		case FORM_BLOCK4:
			block.len.s32 = buf->eat_uint32();
			block.ptr = buf->eat_nbites(block.len.s32);
			DBG_(ELF,("FORM_BLOCK4: len=0x%X, block=",block.len.s32));
			DBG_DUMP_(ELF,block.len.s32,block.ptr);
			break;
		case FORM_DATA2:
			con.s16 = buf->eat_uint16();
			DBG_(ELF,("FORM_DATA2: 0x%X\n",con.s16));
			break;
		case FORM_DATA4:
			con.s32 = buf->eat_uint32();
			DBG_(ELF,("FORM_DATA4: x%lx\n",con.s32));
			break;
		case FORM_DATA8:
			con.s64.hi = buf->eat_uint32();
			con.s64.lo = buf->eat_uint32();
			DBG_(ELF,("FORM_DATA8: x%lx x%lx\n",con.s64.hi,con.s64.lo));
			break;
		case FORM_STRING:
			str = buf->eat_str();
			DBG_(ELF,("FORM_STRING: %s\n",str));
			break;
		default: 
			DBG_ERR(("unknown form 0x%X\n",attr&FORM_MASK));
			attr = 0;
		}
	}
	
		
//==========================================================================
// parse location atoms
//		locations are stack based with postfix ops -  
//		given buf=(id1,thing1), (id2,thing2), (id3,op)...:
//			(id1,thing1)==>push(id1,thing1); 
//			(id2,thing2)==>push(id2,thing2);
//			(id3,op)==>pop(id2,thing2); pop(id1,thing1); 
//					thing3=op(thing1,thing2);
//					push(idresult,thing3);
//			continue until end of buffer
//			result will be pop(id,thing);

#define STACK_REG 1
#define LOP_RESULT 10

//	dkk - This used to be variable s, and was allocated on the stack. By making this static, we save
//	constructing a new object everytime this routine is called. Assuming the code is correct, when
//	this routine exits, the stack should be left in the same state as a newly constructed object.
static Stack gParseStack;

void ElfReader::parse_loc(uint32 len,unsigned char* buf,
				uint32& val,SymSec& sec,SymbolClass& sclass) {
	DBG_ENT("parse_loc");
	val=0;
	uint32 op,v;
	sec=sec_none;
	sclass=sc_none;
//	dkk - Use static object instead.
//	Stack s;

	DBG_ASSERT(len>0 && buf);
	int i = 0;
	while (i<len) {
		switch (buf[i++]) {	//first byte identifies atom
			case LOP_REG:
				sec=sec_none;
				sclass=sc_reg;
				GET_UINT32(v,buf,i);	
				gParseStack.push(LOP_REG,v);
				DBG_(ELF,("LOP_REG: v=0x%X\n",v));
				break;
			case LOP_BASEREG:
				sec=sec_none;
				sclass=sc_stack;
				GET_UINT32(v,buf,i);
				gParseStack.push(LOP_BASEREG,v);
				DBG_(ELF,("LOP_BASEREG: v=0x%X\n",v));
				break;
			case LOP_CONST:
				GET_UINT32(v,buf,i);	
				gParseStack.push(LOP_CONST,v);
				DBG_(ELF,("LOP_CONST: v=0x%X\n",v));
				break;
			case LOP_ADDR: 
				sec=sec_data;
				sclass=sc_data;
				GET_UINT32(v,buf,i);	
				gParseStack.push(LOP_ADDR,v);
				DBG_(ELF,("LOP_ADDR: v=0x%X\n",v));
				break;
			case LOP_DEREF2: {
				DBG_(ELF,("LOP_DEREF2\n"));
				DBG_ERR(("what to do?\n"));
				}
				break;
			case LOP_DEREF4: {
				DBG_(ELF,("LOP_DEREF4\n"));
				DBG_ERR(("what to do?\n"));
				}
				break;
			case LOP_ADD: {
				DBG_(ELF,("LOP_ADD\n"));
				DBG_ASSERT(!gParseStack.empty());
				uint32 o,o2,v,v2;
				gParseStack.pop(o,v);
				if (gParseStack.empty()) gParseStack.push(LOP_RESULT,v);	// ??? add
				else {
					gParseStack.pop(o2,v2);
					//LOP_CONST
					if (o==LOP_CONST) {
						if (o2==LOP_BASEREG) {	// basereg const add
							if (v2!=STACK_REG)
								DBG_ERR(("non stack register=x%X used as base!\n",v2));
							DBG_(ELF,("pushing (%X,x%X)\n",LOP_RESULT,v));
							gParseStack.push(LOP_RESULT,v); //use const as stack offset for stacks
							}
						else {	// ??? const add
							DBG_ERR(("got a const=x%X but no base reg!\n",v));
							gParseStack.push(LOP_RESULT,v);	//set to constant
							}
						}
					//LOP_ADDR
					else if (o==LOP_ADDR) {	// ??? addr add
						DBG_ERR(("what to do with v2?\n"));
						gParseStack.push(LOP_RESULT,v);
						}
					//LOP_REG
					else if (o==LOP_REG) {	// ??? reg add
						DBG_ERR(("what to do with v2?\n"));
						gParseStack.push(LOP_RESULT,v);
						}
					//LOP_BASEREG
					else if (o==LOP_BASEREG) {	// ??? basereg add
						DBG_ERR(("what to do with v2?\n"));
						gParseStack.push(LOP_RESULT,v);
					}
				}
				break;
			}
			case LOP_lo_user:
			{
				DBG_(ELF,("LOP_lo_user\n"));
				DBG_ERR(("what to do?\n"));
				break;
			}
			case LOP_hi_user:
			{
				DBG_(ELF,("LOP_hi_user\n"));
				DBG_ERR(("what to do?\n"));
				break;
			}
			default:
				SET_ERR(se_unknown_type,("unknown location atom 0x%X\n",buf[i-1]));
			}
	}
	gParseStack.pop(op,val);
	DBG_ASSERT(gParseStack.empty());
	DBG_(ELF,("parse_loc: returning val=0x%X, sec=0x%X, sclass=0x%X\n",val,sec,sclass));
}
	
//-------------------------------------------
static Boolean SanityCheck(BufEaters& b, int bytesNeeded);
static Boolean SanityCheck(BufEaters& b, int bytesNeeded)
//-------------------------------------------
{
	if (b.bites_left() < bytesNeeded)
	{
		DBG_FIXME(("not enuf bytes.\n"));
		b.eat_nbites(b.bites_left());
		return false;
	}
	return true;
} // SanityCheck

//-------------------------------------------
void ElfReader::parse_subscr(
	uint32 len,
	unsigned char* buf,
	SymType*& type,
	int& low_bound,
	int& high_bound)
// parse array subscript data
//AT_subscr_data      
//								 a ft  c1       c2       et at  t
//char[7] block2(len=x10, block= 00000800 00000000 00000608 00550003)uchar 
//int[8]  block2(len=x10, block= 00000800 00000000 00000708 00550007)int 
//st[15]  block2(len=x12, block= 00000800 00000000 00000e08 00720000 004a)ref 
//-------------------------------------------
{
	DBG_ENT("parse_subscr");

	type = 0; low_bound = 0; high_bound = 0;
	uint32 ftype=0, rtype=0;	//always 8?
	
	DBG_ASSERT(len>0 && buf);
	BufEaters b((Endian*)this,len,buf);
	while (b.bites_left())
	{
		switch (b.eat_bite())
		{
			//first byte identifies atom
			case FMT_FT_C_C:	//fund type, constant, constant
				DBG_(ELF,("FMT_FT_C_C\n"));
				if (!SanityCheck(b, 10)) break;
				ftype = b.eat_uint16();
				low_bound = b.eat_uint32();
				high_bound = b.eat_uint32();
				break;
			case FMT_FT_C_X:	//fund type, constant, location
				DBG_(ELF,("FMT_FT_C_X\n"));
				if (!SanityCheck(b, 6)) break;
				DBG_FIXME(("not supported\n"));
				ftype = b.eat_uint16();
				low_bound = b.eat_uint32();
				break;
			case FMT_FT_X_C:	//fund type, location, constant
				DBG_(ELF,("FMT_FT_X_C\n"));
				if (!SanityCheck(b, 6)) break;
				DBG_FIXME(("not supported\n"));
				ftype = b.eat_uint16();
				high_bound = b.eat_uint32();
				break;
			case FMT_FT_X_X:	//fund type, location, location
				DBG_(ELF,("FMT_FT_X_X\n"));
				if (!SanityCheck(b, 2)) break;
				DBG_FIXME(("not supported\n"));
				ftype = b.eat_uint16();
				break;
			case FMT_UT_C_C:	//user type, constant, constant
				DBG_(ELF,("FMT_UT_C_C\n"));
				if (!SanityCheck(b, 12)) break;
				DBG_FIXME(("not supported\n"));
				rtype = b.eat_uint32();
				low_bound = b.eat_uint32();
				high_bound = b.eat_uint32();
				break;
			case FMT_UT_C_X:	//user type, constant, location
				DBG_(ELF,("FMT_UT_C_X\n"));
				if (!SanityCheck(b, 8)) break;
				DBG_FIXME(("not supported\n"));
				rtype = b.eat_uint32();
				low_bound = b.eat_uint32();
				break;
			case FMT_UT_X_C:	//user type, location, constant
				DBG_(ELF,("FMT_UT_X_C\n"));
				if (!SanityCheck(b, 8)) break;
				DBG_FIXME(("not supported\n"));
				rtype = b.eat_uint32();
				high_bound = b.eat_uint32();
				break;
			case FMT_UT_X_X:	//user type, location, location
				DBG_(ELF,("FMT_UT_X_X\n"));
				if (!SanityCheck(b, 4)) break;
				DBG_FIXME(("not supported\n"));
				rtype = b.eat_uint32();
				break;
			case FMT_ET:
			{
				DBG_(ELF,("FMT_ET\n"));
				if (!SanityCheck(b, 2)) break;
				DBG_FIXME(("huh??\n"));
				uint16 attr;
				attr = b.eat_uint16();
				attr_val aval(attr,&b);    //gets the attr's value based on the form of the attr   
				switch (attr)
				{
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
						SET_ERR(se_unknown_type, \
							("unknown or invalid attribute " \
							"for array element type 0x%X\n",attr));
				} // switch
				break;
			} // FMT_ET
			default:
				SET_ERR(se_unknown_type,("unknown subscript type\n"));
		} // switch
		DBG_(ELF,("ftype=0x%X, rtype=0x%X, type=0x%X, low=0x%X, high=0x%X\n", \
			ftype,rtype,type,low_bound,high_bound));
	} // while
	if (b.bites_left())
	{
		DBG_FIXME(("bytes left.\n"));
		b.eat_nbites(b.bites_left());
	}
	DBG_(ELF,("parse_subscr: returning type=0x%X, low_bound=0x%X, \
		high_bound=0x%X\n",type,low_bound,high_bound));
} // ElfReader::parse_subscr(...)
	
//-------------------------------------------
void ElfReader::parse_enum_list(
	uint32 len,
	unsigned char* buf,
	Stack* estack)
//list of data elements stored in a block of continuous bytes
//each data item - const, block of bytes, null
//const contains internal of the enum elem
//null terminated block holds the enum literal string
//(generated in reverse order)
//-------------------------------------------
{
	DBG_ENT("parse_enum_list");

	DBG_WARN(("not implemented\n"));
	return;
	
	uint32 val; char* ename;
	BufEaters b((Endian*)this,len,buf);
	DBG_ASSERT((long)len>0 && buf);
	int i = 0; 
	while (i<b.bites_left())
	{
		val = b.eat_uint32();
		ename = b.eat_str();
		estack->push(val,ename);
		DBG_(ELF,("parse_enum_list: count=%d, val=0x%X, \
			ename=%s\n",estack->num(),val,ename));
	}
	DBG_(ELF,("parse_enum_list: returning count=0x%X\n",estack->num()));
}
	
//==========================================================================
// dwarf types	

//add fundamental symbol type - keeps track of whether type has
//been already added.  If so, return that type.
SymCat ElfReader::get_type_cat(uint32 elftype) {
	switch (elftype) {
    	case FT_void:				return (tc_void);
    	case FT_pointer:			return (tc_pointer);
		case FT_char:				return (tc_char);
    	case FT_signed_char:		return (tc_char);
    	case FT_unsigned_char:		return (tc_uchar);
    	case FT_short:				return (tc_short);
    	case FT_signed_short:		return (tc_short);
    	case FT_unsigned_short:		return (tc_ushort);
    	case FT_integer:			return (tc_int);
    	case FT_signed_integer:		return (tc_int);
    	case FT_unsigned_integer:	return (tc_uint);
    	case FT_long:				return (tc_long);
    	case FT_signed_long:		return (tc_long);
    	case FT_unsigned_long:		return (tc_ulong);
    	case FT_float:				return (tc_float);
    	case FT_dbl_prec_float:		return (tc_double);
    	case FT_ext_prec_float:		return (tc_ldouble);
    	case FT_complex:			return (tc_complex);
    	case FT_dbl_prec_complex:	return (tc_dcomplex);
    	case FT_ext_prec_complex:	
    			DBG_WARN(("FT_ext_prec_complex not supported\n"));
    			return (tc_dcomplex);
    	case FT_boolean:			
    			DBG_WARN(("FT_boolean not supported\n"));
    			return (tc_long);
    	case FT_label:				
    			DBG_WARN(("FT_label not supported\n"));
    			return (tc_long);
		case FT_lo_user:	
    			DBG_WARN(("FT_lo_user not supported\n"));
				return (tc_ulong);
		case FT_hi_user:
    			DBG_WARN(("FT_hi_user not supported\n"));
				return (tc_ulong);
		default:
				SET_ERR(se_unknown_type,("unknown fundamental type 0x%X\n",elftype));
				return tc_unknown;
		}
	}
	
//when get a user defined type, add it to list with elf_offset for 
//		resolving later 
//when a user type is referenced, return type as elf_offset -
//		we'll resolve them later when we're done
SymType* ElfReader::get_user_type(uint32 elf_offset) {
	return get_ref_type(elf_offset,_symroot);
	}

SymType* ElfReader::get_mod_type(uint16 len,unsigned char* buf) {
	DBG_ASSERT(len>0 && buf);
	SymType* type;
	uint16 v;
	len-=2;
	GET_UINT16(v,buf,len);	//last 2 bytes are fund_type (len will advance by 2)
	type = get_type(v);	//get base type
	type = add_modtypes(len-2,buf,type);	//add modifiers to types
	return type;
	}
	
SymType* ElfReader::get_mod_user_type(uint32 len,unsigned char* buf) {
	DBG_ASSERT(len>0 && buf);
	SymType* type;
	uint32 v;
	len-=4;
	GET_UINT32(v,buf,len);	//last 4 bytes are user_type  (len will advance by 4)
	#ifdef _DDI_BUG	//this is non-svr4
					//non-allocable/non-loadable sections like debug should 
					//have the base of the section starting from 0
					//so we have to make relative to debug section...
		DBG_(ELF,("BUG!!  Diab Data's ref should be offset from debug section!\n"));
		v -= debug_section_base;
		DBG_(ELF,("ref=0x%X, debug base =0x%X\n",v,debug_section_base));
	#endif
	type = get_user_type(v);	//get base type
	type = add_modtypes(len-4,buf,type);	//add modifiers to types
	return type;
	}
	
SymType* ElfReader::add_modtypes(uint32 len,unsigned char* buf, SymType* type) {
	int i=0;
	while (i<len) {
		switch (buf[i++]) {	//first byte identifies mod
			case MOD_pointer_to:
			case MOD_reference_to:
				type = add_type(0,4,tc_pointer,(void*)type);	//keep adding pointers...
				break;
			case MOD_const:
			case MOD_volatile:
			case MOD_lo_user:
			case MOD_hi_user:
				DBG_WARN(("# ignoring MOD_const/MOD_volatile/MOD_lo_user/MOD_hi_user\n"));
				//ignore - just return base type
				break;
			default:
				SET_ERR(se_unknown_type,("# unknown mod fund type 0x%X\n",buf[i-1]));
			}
		}
	return type;
	};

