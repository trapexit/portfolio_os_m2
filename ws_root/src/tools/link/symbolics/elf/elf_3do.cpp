/*  @(#) elf_3do.cpp 96/07/25 1.40 */

//====================================================================
// elf_3do.cpp  -  ElfLinker class funcs for 3do specifics


#ifndef USE_DUMP_FILE
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "utils.h"
#include "elf_l.h"
#endif /* USE_DUMP_FILE */

#include "linkopts.h"
#include "elf_3do.h"
#include "debug.h"

#pragma segment elflink
//==========================================================================
// 3do specific

void ElfLinker::create_3dohdr() {
	DBG_ENT("create_3dohdr");
	//create 3dobinheader
	add_master_sec(sec_hdr3do,0);	//first section is 3dobinheader
	_sections->set_size(_sections->secnum(sec_hdr3do),SEC_HDR_3DO_SIZE);
	}

enum hdr3do_field_types {
	t_none,
	t_uint32,
	t_uint8,
	t_char32,
	t_time
	};
struct hdr3do_field {
	char* _fname;
	hdr3do_field_types _ftype;
	uint32 _foffset;
	};
hdr3do_field hdr3do_fields[]= {
#ifndef USER_PRIVS
	{ "pri", t_uint8, offsetof(_3DOBinHeader,_3DO_Item.n_Priority) },
	{ "priority", t_uint8, offsetof(_3DOBinHeader,_3DO_Item.n_Priority) },
#endif
	{ "ver", t_uint8, offsetof(_3DOBinHeader,_3DO_Item.n_Version) },
	{ "version", t_uint8, offsetof(_3DOBinHeader,_3DO_Item.n_Version) },
	{ "rev", t_uint8, offsetof(_3DOBinHeader,_3DO_Item.n_Revision) },
	{ "revision", t_uint8, offsetof(_3DOBinHeader,_3DO_Item.n_Revision) },
	{ "type", t_uint8, offsetof(_3DOBinHeader,_3DO_Item.n_Type) },
	{ "subsys", t_uint8, offsetof(_3DOBinHeader,_3DO_Item.n_SubsysType) },
	{ "flags", t_uint8, offsetof(_3DOBinHeader,_3DO_Flags) },
	{ "osver", t_uint8, offsetof(_3DOBinHeader,_3DO_OS_Version) },
	{ "osversion", t_uint8, offsetof(_3DOBinHeader,_3DO_OS_Version) },
	{ "osrev", t_uint8, offsetof(_3DOBinHeader,_3DO_OS_Revision) },
	{ "osrevision", t_uint8, offsetof(_3DOBinHeader,_3DO_OS_Revision) },
	{ "stack", t_uint32, offsetof(_3DOBinHeader,_3DO_Stack) },
	{ "maxusecs", t_uint32, offsetof(_3DOBinHeader,_3DO_MaxUSecs) },
	{ "name", t_char32, offsetof(_3DOBinHeader,_3DO_Name) },
	{ "time", t_time, offsetof(_3DOBinHeader,_3DO_Time) },
	{ 0, t_none, 0 }
	};


void ElfLinker::init_3donote(sec3do *note, uint32 type, uint32 descsize) {
	note->namesz = 4;
	note->descsz = descsize;
	note->type = type;
	note->name = SEC_3DO_NAME;
		//For PC Compatibility, MH 7/24/96
		if( swap_needed() ) {
		swapit( note->namesz );
		swapit( note->descsz );
		swapit( note->type );
		swapit( note->name );
	   }
	}

void ElfLinker::update_3dohdr() {
	DBG_ENT("update_3dohdr");
	_3dohdr = (_3DOBinHeaderSection*) MALLOC(SEC_HDR_3DO_SIZE);
	if (!_3dohdr) return;

	init_3donote(&_3dohdr->note, TYP_3DO_BINHEADER, HDR_3DO_SIZE);

	_3DOBinHeader *raw3do = &_3dohdr->hdr3do;

	DBG_FIXME(("add code to set _3DOBinHeader!!\n"));
	//FLAGS:
	//_3DO_DATADISCOK	32	/* App willing to accept data discs */
	//_3DO_NORESETOK	16	/* App willing to keep running on empty drawer */
	//_3DO_LUNCH		8	/* OS has been launched, chips may be lunched */
	//_3DO_USERAPP		4	/* User app requiring authentication */
	//_3DO_PRIVILEGE	2
	//_3DO_PERMSTACK	1
	//byte values

	//set defaults
	memset(raw3do,0,HDR_3DO_SIZE);
	raw3do->_3DO_Item.n_Version=1;	//???
	raw3do->_3DO_Flags=0;	//???

	//set user values
	if (_link_opts->isset(linkopt_header3do)) {
		for (int i=0; i<_link_opts->nargs(linkopt_header3do); i++) {
			busy_cursor();
			char* fname = _link_opts->arg(linkopt_header3do,i);
			if (!fname) {
    			fprintf(_user_fp,"No field specified for option -H\n");
				SET_ERR(se_fail,("no field for -H\n"));
				return;
				}
			char* fval = _link_opts->asn(linkopt_header3do,i);
			if (!fval) {
    			fprintf(_user_fp,"No value specified for option -H%s\n",fname);
				SET_ERR(se_fail,("no value for -H%s\n",fname));
				return;
				}
			for (int j=0; hdr3do_fields[j]._fname; j++) {
				if (!mystrcmp(hdr3do_fields[j]._fname,fname)) {
					uint32 foff = hdr3do_fields[j]._foffset;
					uint8* p = (uint8*)raw3do+foff;
					switch(hdr3do_fields[j]._ftype) {
						case t_uint8: {
							uint8 v = ch2val(fval);
							*p = v;
							}
							break;
						case t_uint32: {
							uint32 v = ch2val(fval);
							//For PC Compatibility, MH 7/24/96
							if( swap_needed() )
								swapit( v );
							*((uint32*)p) = v;
							}
							break;
						case t_char32: {
							int len = strlen(fval);
							memcpy(p,fval,min(len,32));
							}
							break;
						case t_time: {
							uint32 v = str2time(fval);
							//For PC Compatibility, MH 7/24/96
							if( swap_needed() )
								swapit( v );

							if (!v) {
								fprintf(_user_fp,"Error in Date/Time input %s\n",fval);
    							fprintf(_user_fp,"The Time parameter should be in 'MM/DD/YY,HH:MM:SS' format\n");
    							fprintf(_user_fp,"The earliest time supported is 01/01/93 00:00:00 GMT\n");
								SET_ERR(se_fail,("bad time input %s\n",fval));
								return;
								}
							*((uint32*)p) = v;
							}
							break;
						default:
							DBG_ERR(("invalid header type\n"));
						}
					}
				}
			}
		}
	}

//-------------------------------------------
void ElfLinker::create_imports()
//-------------------------------------------
{
	DBG_ENT("create_imports");
	//at this point we don't know how many relocations we'll need to generate
	//or which symbols are undefined
	//as we find undef symbols which are imported, we'll
	//add them as undefs but add them here too
	//SOMEDAY!!
	//if no IMPORTS keyword is given, we should
	//read exports of each object and add imports that way
	//(so user doesn't have to specify at the cost of a longer link)
	ObjInfo* o;
	for (o=firstobj(); o; o=o->next())
	{
		//actually, we delete the objs if they're DLLs;
		//instead we read their exports so we can see what we can imports
		//so, won't be needing this!  SOMEDAY
    	if (o->_sections->secnum(sec_exp3do)) 	//see if anyone has any exports that we might import from
    		_imps->_tag++;
    }
	if (_link_opts->isset(linkopt_def) 	//def file could have IMPORTS
			|| _imps->_tag)				//otherwise, any import syms from any objects?
		add_master_sec(sec_imp3do,0);
}

//-------------------------------------------
int ElfLinker::resolve_imps()
//all imps should have name, symord & liborg at this point...
//some imps added after reading def file (we'll need to verify these)
//in resolve_unds, we either add an obj from a libry or add imp...
//when we found an imp while going thru resolve_unds, we add it as an import
//at that time (instead of adding the object)
//other imps added when we read thru def file
//here we go thru and verify them by making sure they're really referenced...

//remove _unds which are resolved by imports
//remove _imps which aren't referenced
//-------------------------------------------
{
	DBG_ENT("resolve_imps");
	//SOMEDAY! combine this code with resolve_syms
	//resolve which syms need to be imported
	//we added the library ords & sym ords when we parsed the deffile;
	//now add the imports that we really need

	//go thru undefineds and see if any of them are resolved
	//with the addition of these imports
    Syms::Sym *usym, *isym;
    //int symidx;
    char *name;
    for (isym=_imps->first(); isym; ) {
		busy_cursor();
        Syms::Sym* tisym=isym->next();  //save in case we delete usym
		name = isym->_name;
#ifdef OLD_DLL //we now just set the Dll's _used field if the Dll is imported
		Boolean keepimp=false;
#endif
#if 0	//replaced by while loop
    	for (usym=_unds->first(); usym; ) {
        	Syms::Sym* tusym=usym->next();  //save in case we delete usym
        	ObjInfo* o = obj(usym->_objind);
        	DBG_ASSERT(o);
        	//n = get_symname(o,usym->_symidx);
        	char* n = usym->_name;
			DBG_ASSERT(n);
			if (!mystrcmp(n,name)) {	//found one.
#endif
		while (usym=_unds->find(name),usym)
		{
				//NOTE: these symbols will still be undefined here!
#ifdef OLD_DLL //we now just set the Dll's _used field if the Dll is imported
				keepimp=true;
#else
				resolve_imp(isym,usym);
#endif
            	_unds->rmv(usym);
#if 0
				//break;	//don't want to break cuz could have several defines that reference the same symbol
            	}
        	usym = tusym;
#endif
        }
#ifdef OLD_DLL //we now just set the Dll's _used field if the Dll is imported
		if (keepimp==false) 	//remove unreferenced imports
			_imps->rmv(isym);
#endif
        isym = tisym;
	}
	return 1;
}

//-------------------------------------------
void ElfLinker::update_imports()
//all imps should have name, symord & liborg
//called after we've hashed & relocated all symbols
//and before we merge symtab
//exportWords - top bit for data; rest = offset within segment
//-------------------------------------------
{
	DBG_ENT("update_imports");
	importTemplate* i;
	int j; char* n;
	//count up size of strings
	int istrsize=1, libord=0, nimplibs=0;
#define MAX_IMPLIBS 20

	if (!ndlls()) return;
	DllInfo *d, *next_d;
	for (d=firstdll(); d; d=next_d) {
		next_d = d->next();
#ifndef OLD_DLL
		if (!d->_used) {
			SYM_DELETE(d);
			}
		else {
#endif
    	n = d->_name;
		if (n) {
			istrsize+=strlen(n)+1;
			}
#ifndef OLD_DLL
			}
#endif
		}
	//create imports sections contents buffer
	nimplibs = ndlls();
	if (!nimplibs) {
		DBG_WARN(("tried to update imports, but no Dlls referenced!\n"));
		return;
		}
	uint32 isize = IMP_SIZE(nimplibs,istrsize);
	i = (importTemplate*) MALLOC(isize);
	if (!i) return;

	//init the note header
	init_3donote(&i->note, TYP_3DO_IMPORTS, isize - NOTE_SIZE);

	//fill in section contents
	i->numImports=nimplibs;
	char* imp_records = IMP_RECORD_START(i);		//get address of import records
	char* imp_strings = IMP_STRINGS(i);		//get start of strings
	uint32 noff = 1;		//init name offset
	DBG_(ELF,("IMP_RECORD_SIZE=%d\n",IMP_RECORD_SIZE));
	DBG_(ELF,("IMP_TEMPLATE_SIZE=%d\n",IMP_TEMPLATE_SIZE));
	DBG_(ELF,("i=x%X, i->numImports=%d, i->strings=x%X, \
		IMP_STRINGS_OFF=%d\n",
		i,i->numImports,IMP_STRINGS(i), \
		(IMP_STRINGS_OFF(i->numImports))));
		//For PC Compatibility, MH 7/24/96
			if( swap_needed() )
				swapit( i->numImports );

	for (d=firstdll(), j=0; d; d=d->next(), j++) {
		busy_cursor();
    	n = d->_name;
		libord = d->_ord;
		d->_impind = j;	//update the Dll's import index into .imp3do
		if (n) {
			//name offset is from start of import record
			i->records[j].nameOffset = noff;
			//For PC Compatibility, MH 7/24/96
			if( swap_needed() )
				swapit( i->records[j].nameOffset );
			strcpy((char*)imp_strings+noff,n);
			noff+=strlen(n)+1;
			}
		else
			i->records[j].nameOffset=0;
		//SOMEDAY. why are we using the name here when we can just as easily
		//put the ordinal number here?  For readability...
		i->records[j].libraryCode = libord;
		//For PC Compatibility, MH 7/24/96
		if( swap_needed() )
			swapit( i->records[j].libraryCode );
		//these come from the .hdr3do section
		i->records[j].libraryVersion = d->_ver;
		i->records[j].libraryRevision = d->_rev;
		if (d->_flags)
			i->records[j].flags = d->_flags;
		else
	                {
			i->records[j].flags =
	                    _link_opts->isset(linkopt_noload)
 				? 0 : (IMP3DO_IMPORT_NOW | IMP3DO_IMPORT_REQUIRED);
 						//default
			if(_link_opts->isset(linkopt_allow_reimport))
				i->records[j].flags |= IMP3DO_REIMPORT_ALLOWED;
			}

		i->records[j].pad = 0;
		}
	_imports = i;
	_sections->set_size(_sections->secnum(sec_imp3do),ALIGN(isize,4));
	DBG_(ELF,("isize=%d; _sections->size(_sections->secnum(sec_imp3do))=%d\n",
		isize, _sections->size(_sections->secnum(sec_imp3do))));
	//now that we know the lib's index within .imp3do, update the imports
	Syms::Sym *x, *xnext;
	for (x=_imps->first(); x; x=xnext) {
		xnext = x->next();
		d = DllInfo::find(x->_objind);	//finds the dll given the ordinal number
		if (!d) {
			//we must have deleted this dll so should get rid of all it's
			//imports as well
			_imps->rmv(x);
			}
		else {
			x->_ordinal = d->_impind;
			DBG_(ELF,("import x: name=%s, ordinal=x%X, lib=x%X, symidx=x%X\n",
				x->_name,x->_ordinal,x->_objind,x->_symidx));
			}
		}
	}

//file format:
//IMPORTS <name>=<lib>.<ord> <name2>=<lib2>.<ord> (<lib3>.<name3>???) ...
//MAGIC   <modnum>
//MODULE  <modnum>
//EXPORTS <ord>=<name> <ord2>=<name2> (<name3>???) ...
        //void update_exp(int symidx, int objind) { }
        //void add_exp() {
        //          int data = 0, off = 0;  //update later - after
        //          _exp_syms[_nexps]=(data?0x10000000:0) | off;
        //          _nexps++;
        //          }
void ElfLinker::create_exports() {
	DBG_ENT("create_exports");
	int secnum;
	//for exports, we'll just copy what the user gave in their .def file
	int line=0;
	int32 sz=0;
	//char* buf=0;
	if (!_link_opts->isset(linkopt_def))
		return;
	secnum = add_master_sec(sec_exp3do,0);
	const char* deffile = _link_opts->arg(linkopt_def);
	const char* buf = read_deffile(deffile, sz);
	if (!buf) return;
	//SOMEDAY!! move this to before call since imports also uses
	parse_def(deffile, buf, sz);
	FREE((char*)buf);
	}

void ElfLinker::update_exports() {
	DBG_ENT("update_exports");
	exportTemplate* e; int i;
	Syms::Sym* x; Elf32_Sym* s; uint32 xoffset;
	if (!_exps->_nsyms) return;

	//create exports sections contents buffer
	//kevinh - The exportTemplate includes an extra longword for the
	//    first export - we need to compensate for that.
	int maxord = 0;
	for (x=_exps->first(); x; x=x->next()) {
		maxord = max(x->_ordinal,maxord);
		}
	//we use the ordinal number as the index, so the maximum index is the
	//maximum ordinal number
	//was: uint32 esize = EXP_TEMPLATE_SIZE+_exps->_nsyms*4-sizeof(uint32);
	uint32 esize = EXP_TEMPLATE_SIZE+(maxord+1)*4-sizeof(uint32);

	e = (exportTemplate*) MALLOC(esize);
	if (!e) return;
	memset(e,0,esize);

	for(i = 0; i <= maxord; i++)
		e->exportWords[i] = ELF3DO_EXPWORD(0xff, 0);

	//init the note header
	init_3donote(&e->note, TYP_3DO_EXPORTS, esize - NOTE_SIZE);

	//fill in section contents
	e->libraryID=_exps->_tag;
	//e->numExports=_exps->_nsyms;
	e->numExports=maxord+1;
	//For PC Compatibility, MH 7/24/96
	if( swap_needed() ) {
		swapit( e->libraryID );
		swapit( e->numExports );
	}
	DBG_(ELF,("tag=x%X, nsyms=x%X, maxord=x%X\n",_exps->_tag,_exps->_nsyms,maxord));
	for (x=_exps->first(); x; x=x->next()) {
		busy_cursor();
		int ord = x->_ordinal;
		ObjInfo* o = obj(x->_objind);
		SymSec sec = get_symsectype(o,x->_symidx);
		int osecnum = o->_sections->secnum(sec);
		uint32 base = o->_sections->baseaddr(osecnum);
		int secnum = _sections->secnum(sec);
		s = get_sym(o,x->_symidx);
		xoffset = s->st_value+base;	//??? offset within section - must be relocated already!
		DBG_(ELF,("xoffset=x%X, base=x%X\n",xoffset,base));
		//the upper byte is the section number
		//the lower 3 bytes is the offset within that section
		e->exportWords[ord] = ELF3DO_EXPWORD(secnum,xoffset);
		//For PC Compatibility, MH 7/24/96
		if( swap_needed() ) {
			swapit(e->exportWords[ord] );
		}

		DBG_(ELF,("sec=x%X, sym=%s, xoffset=x%X, stv=x%X, b=x%X\n",sec,get_symname(o,x->_symidx),xoffset, s->st_value, base));
		}
	_exports = e;
	_sections->set_size(_sections->secnum(sec_exp3do),esize);
	}

//-------------------------------------------
const char* ElfLinker::read_deffile(
	const char* deffile,
	int32& sz)
//-------------------------------------------
{
	char* buf=0;
	FILE* dfp=0;
	sz=0;
	if (!deffile)
	{
		fprintf(_user_fp,"# Error: No definitions file specified for -%c option\n",_link_opts->id(linkopt_def));
		SET_ERR(se_fail,("No definitions file specified for -%c option\n",_link_opts->id(linkopt_def)));
		return 0;
	}
	if ((dfp=fopen(deffile,"r"))==NULL)
	{
		fprintf(_user_fp,"# Error: unable to open definitions file %s\n",deffile);
		SET_ERR(se_fail,("#Unable to open definitions file %s\n",deffile));
		return 0;
	}
	fseek(dfp,0,SEEK_END);
	sz = ftell(dfp);
    DBG_(ELF,("sizeof deffile from ftell = %d\n", sz));
	if (!sz)
	{
		fprintf(_user_fp,"Warning: definitions file %s is empty\n",deffile);
		return 0;
	}
	buf = (char*) MALLOC(sz);
	if (!buf) return 0;
	if (fseek(dfp,0,SEEK_SET))
	{
		fprintf(_user_fp,"# Error: Unable to seek to end of definitions file %s\n",deffile);
		SET_ERR(se_seek_err,("#Unable to seek to end of definitions file %s\n",deffile));
		return 0;
	}
    sz = fread(buf,1,sz,dfp);
    DBG_(ELF,("actually read = %d\n", sz));
	if (!sz)
	{
		fprintf(_user_fp,"# Error: Unable to read definitions file %s\n",deffile);
		SET_ERR(se_read_err,("#Unable to read definitions file %s\n",deffile));
		return 0;
	}
	fclose(dfp); dfp=0;
	return buf;
} // ElfLinker::read_deffile

#define DEF_ERR(d,x)	\
			fprintf(_user_fp,"Error in definitions file ");	\
			if (d.file()) fprintf(_user_fp,"%s ",d.file());	\
			if (d.line()) fprintf(_user_fp,"at line %d",d.line());	\
			if (d.col()) fprintf(_user_fp," column %d",d.col());	\
			char* _buf=fmt_str x;	\
			fprintf(_user_fp,": %s",_buf);	\
			fprintf(_user_fp,"\n");	\
			SET_ERR(se_fail,("error in definitions file\n"));

#define DEF_ERR_RET(d,x) {	\
			DEF_ERR(d,x)	\
			return;		\
			}

void ElfLinker::parse_def(const char* deffile, const char* buf, int32 sz) {
	int line=0;
	DefParse d(buf, sz, deffile);
	uint8 import_flag=0;
	#define PARSING_DEFFILE 0
	#define PARSING_IMPORTS 1
	#define PARSING_EXPORTS 2
	#define PARSING_IMPORT_FLAG 3	//import flag
	int parse_state = PARSING_DEFFILE;
	while (d.more()) {
		busy_cursor();
		switch (d.get_token()) {
			case DefParse::def_modnum:
				if (_exps->_tag)
					DEF_ERR_RET(d,("multiple MAGIC tokens found\n"));
				if (d.get_token()!=DefParse::def_number)
					DEF_ERR_RET(d,("Invalid MAGIC token %s\n",d._token_buf));
				_exps->_tag=d._num;
				break;
			case DefParse::def_imports:
				parse_state = PARSING_IMPORTS;
				break;
			case DefParse::def_import_on_demand:
				parse_state = PARSING_IMPORT_FLAG;
				import_flag = IMP3DO_IMPORT_REQUIRED;	//parse import list for IMPORT_NOW
				break;
			case DefParse::def_reimport_allowed:
				parse_state = PARSING_IMPORT_FLAG;
				import_flag = IMP3DO_REIMPORT_ALLOWED;	//parse import list for IMPORT_NOW
				break;
			case DefParse::def_import_now:
				parse_state = PARSING_IMPORT_FLAG;
				import_flag = IMP3DO_IMPORT_NOW;	//parse import list for IMPORT_NOW
				break;
			case DefParse::def_import_flag:
				parse_state = PARSING_IMPORT_FLAG;
				if (d.get_token()==DefParse::def_number)
					import_flag = d._num;	//parse import list for flag
				else
					DEF_ERR_RET(d,("Invalid token %s\n",d._token_buf));
				break;
			case DefParse::def_exports:
				parse_state = PARSING_EXPORTS;
				break;
			case DefParse::def_number:
				switch (parse_state) {
					case PARSING_EXPORTS:
						//exports syntax: number=symbol
						parse_exp(d);	//parse export definition
						break;
					case PARSING_IMPORT_FLAG:
						parse_import_flags(d,import_flag);	//parse import list for flag
						break;
					default:
						DEF_ERR_RET(d,("Invalid token %s\n",d._token_buf));
					}
				break;
			case DefParse::def_symbol:
				switch (parse_state)
				{
					case PARSING_IMPORTS:
						//imports syntax: symbol=number.number
						parse_imp(d);	//parse import definition
						break;
					case PARSING_IMPORT_FLAG:
						parse_import_flags(d,import_flag);	//parse import list for IMPORT_NOW
						break;
					default:
						DEF_ERR_RET(d,("Invalid token %s\n",d._token_buf));
				}
				break;
			case DefParse::def_comment:	//ignore...
				d.skip_line();
				break;
			case DefParse::def_eof:	//done!
				break;
			default:
				if (parse_state == PARSING_IMPORTS)
				{
					fprintf(_user_fp, "Unexpected token: %s\n", d._token_buf);
					d.skip_line();
				}
				else
					DEF_ERR_RET(d,("Invalid token %s\n",d._token_buf));
		}
	}
}

//add possible imports from exp3do of object
void ElfLinker::add_imps(ObjInfo* o) {
	DBG_ENT("add_imps");
	int symord, libord; char* name;
	uint32 libver, librev;

	//get .exp3do section from object
	exportTemplate* e;
	DBG_ASSERT(o->_sections);
	uint32 sidx = o->_sections->secnum(sec_exp3do);
	if (!sidx || o->_sec_hdr[sidx].sh_size==0)
		return;
	e = (exportTemplate*)read_obj_section(o,sidx);
	//For PC Compatibility, MH 7/24/96
	if( swap_needed() ) {
		swapit( e->libraryID );
		swapit( e->numExports );
	}
	//what about note header??
	if (!e || !e->numExports) {
		DBG_WARN(("DLL contains no exports\n"));
		return;
		}
	libord=e->libraryID;

	//get .hdr3do section from object
	_3DOBinHeaderSection* h;
	uint32 hidx = o->_sections->secnum(sec_hdr3do);
	if (!hidx || o->_sec_hdr[hidx].sh_size==0) {
		DBG_WARN(("DLL does not contain a .hdr3do section\n"));
		return;
		}
	h = (_3DOBinHeaderSection*)read_obj_section(o,hidx);
	if (!h) {
		DBG_WARN(("#Unable to read .hdr3do section\n"));
		return;
		}
	libver=h->hdr3do._3DO_Item.n_Version;
	librev=h->hdr3do._3DO_Item.n_Revision;

	_imps->_tag++;
	DBG_(ELF,("Exports libID %d; %d entries\n", e->libraryID, e->numExports));
	//check to see if we already have this dll from reading a definitions file
	DllInfo* d;
	if (!(d=DllInfo::find(o->_fp->filename()),d) && !(d=DllInfo::find(libord),d)) {
#ifndef OLD_DLL
		d = (DllInfo*) SYM_NEW(DllInfo(o->_fp,o));
#else
		d = (DllInfo*) SYM_NEW(DllInfo(o->_fp));
#endif
		}
	d->_ord = libord;
	d->_ver = libver;
	d->_rev = librev;
	for (int i=0; i<e->numExports; i++)
	{
		busy_cursor();
		//For PC Compatibility, MH 7/24/96
			if( swap_needed() ) {
			swapit( e->exportWords[i] );
		}
		int osecnum = ELF3DO_EXPSECNUM(e->exportWords[i]);
		uint32 xoffset = ELF3DO_EXPOFFSET(e->exportWords[i]);
		if (osecnum != 0xff)
		{
			uint32 osecbase = o->_sections->baseaddr(osecnum);
			if (symord=find_sym_by_val(o,osecnum, osecbase + xoffset),symord)
			{
				name=get_symname(o,symord);
				//we'll get the real info later when we update
				//Kevin wants .exp3do index; not symbol index!
					//_imps->add(dup_str(name),symord,libord);
				_imps->add(dup_str(name),i,libord);
			}
			else
			{
				DBG_WARN(("#Unable to find symbol for export value x%X\n",e->exportWords[i]));
			}
		}
	}
	if (e) LINK_DELETE_ARRAY(e);	//delete section - don't need it anymore
	if (h) LINK_DELETE(h);	//delete section - don't need it anymore
}

//given value, find symbol index for matching symbol 
uint32 ElfLinker::find_sym_by_val(ObjInfo* o,int secnum,uint32 val) {
	uint32 symval;
	int symsecnum;
	uint8 symbind;
	int nsyms = get_symtab_nsyms(o);
	DBG_ASSERT(o->_symtab);
	for (int i=0; i<nsyms; i++) {
		symval = o->_symtab[i].st_value;
		symsecnum = o->_symtab[i].st_shndx;
		symbind = ELF32_ST_BIND(o->_symtab[i].st_info);
		//should we only allow types STT_FUNC, STT_OBJECT??
		if (val == symval && secnum == symsecnum
				&& symbind == STB_GLOBAL) {
			return i;
			}
		}
	return 0;
	}
//-------------------------------------------
void ElfLinker::parse_imp(DefParse& p)
//parse import definition and add to list of imports
//	imports syntax: symbol=number.number
//	since we don't know yet what we'll really need
//	to import, we must assume everything in this list
//	is going to be imported...
//-------------------------------------------
{
	DBG_ENT("parse_imp");
	const char* deffile = p.file();
	uint32 libord, symord; char* name;
	name=p._str;
	if (p.get_token()!=DefParse::def_equals)
		DEF_ERR_RET(p,("\"=\" expected, found %s\n",p._token_buf));
	if (p.get_token()!=DefParse::def_number)
		DEF_ERR_RET(p,("library ordinal number expected, found %s\n",p._token_buf));
	libord=p._num;
	DllInfo* d = dll(libord);
	if (!d) {	//if we didn't have a file for this, add it now
#ifndef OLD_DLL
		d = (DllInfo*) SYM_NEW(DllInfo(0,0));
#else
		d = (DllInfo*) SYM_NEW(DllInfo(0));
#endif
		d->_ord = libord;
		}
	if (p.get_token()!=DefParse::def_dot)
		DEF_ERR_RET(p,("\".\" expected, found %s\n",p._token_buf));
	if (p.get_token()!=DefParse::def_number)
		DEF_ERR_RET(p,("symbol ordinal number expected, found %s\n",p._token_buf));
	symord=p._num;

	//we'll get the real info later when we update
	_imps->add(dup_str(name),symord,libord);
	}

//-------------------------------------------
void ElfLinker::parse_exp(DefParse& d)
//parse export definition and add to list of exports
//	exports syntax: number=symbol
//-------------------------------------------
{
	DBG_ENT("parse_exp");
	const char* deffile = d.file();
	uint32 ord; char* name;
	ord=d._num;
	if (d.get_token()!=DefParse::def_equals)
		DEF_ERR_RET(d,("\"=\" expected, found %s\n",d._token_buf));
	if (d.get_token()!=DefParse::def_symbol)
		DEF_ERR_RET(d,("symbol name expected, found %s\n",d._token_buf));
	name=d._str;
	_linkhash* h = _hashtab->get_hash(name,hoff_global);
	if (!h || !h->_symidx)
		DEF_ERR_RET(d,("exported symbol name %s is undefined\n",name));
	Elf32_Sym* sym = get_sym(obj(h->_objind),h->_symidx);
	if (!sym || sym->st_shndx==SHN_UNDEF)
		DEF_ERR_RET(d,("exported symbol name %s is undefined\n",name));
	_exps->add(dup_str(name),h->_symidx,h->_objind,ord);
	}

//-------------------------------------------
void ElfLinker::parse_import_flags(DefParse& p, uint32 flag)
//-------------------------------------------
{
	DBG_ENT("parse_import_flags");
	const char* deffile = p.file();
	DllInfo* d=0;
	if (p._token==DefParse::def_number) {
		int libord;
		libord = p._num;
		d = DllInfo::find(libord);
		if (!d) {	//if we didn't have a file for this, add it now
			DBG_WARN(("no lib found for libord = %d\n",libord));
#ifndef OLD_DLL
			d = (DllInfo*) SYM_NEW(DllInfo(0,0));
#else
			d = (DllInfo*) SYM_NEW(DllInfo(0));
#endif
			if (d) d->_ord = libord;
			}
		}
	else {
		if (p._token==DefParse::def_symbol) {
			char fname[1024];
			strcpy(fname, p._str);
			DefParse::DefToken c;
			//make sure we parse the whole name
			while (c = p.next_token(),
					c==DefParse::def_dot||c==DefParse::def_fwdslash
					||c==DefParse::def_colon||c==DefParse::def_symbol) {
				p.get_token();
				strcat(fname, p._token_buf);
				}
			const char* n = ::rmv_path(fname);
			d = DllInfo::find(n);
			if (!d) {	//if we didn't have a file for this, add it now
				DBG_WARN(("no lib found for fname %s\n",n));
#ifndef OLD_DLL
				d = (DllInfo*) SYM_NEW(DllInfo(0,0));
#else
				d = (DllInfo*) SYM_NEW(DllInfo(0));
#endif
				if (d) d->_name = dup_str(n);
				}
			}
		else {
			DEF_ERR_RET(p,("expected module number or file name but found %s\n",p._token_buf));
			}
		}
	if (!d) {	//if we didn't have a file for this, add it now
		DEF_ERR_RET(p,("failed to find or create Dll for flags\n"));
		}
	d->_flags |= flag;
	}
//called after we've hashed & relocated all symbols
//and before we merge symtab
//exportWords - top bit for data; rest = offset within segment
