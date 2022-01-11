/*  @(#) elf_d.cpp 96/09/23 1.45 */

//====================================================================
// elf_d.cpp  -  ElfDumper class defs for dumping ELF binaries


#ifndef USE_DUMP_FILE
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "utils.h"
#include "elf_r.h"
#endif /* USE_DUMP_FILE */

#include "elf_3do.h"
#include "elf_d.h"
#include "dwarf.h"
#include "debug.h"

#pragma segment elfdump

#define TEST_SECT(f) \
    if ((unsigned)s->sh_##f >= fReader->_elf_hdr->e_shnum) {\
        DBG_ERR(("0x%lx: Bad \"" #f "\" field for reloc section %s\n",\
            s->sh_##f,symstr(s->sh_name))); return;      \
        }


//==========================================================================
// main funcs for dumper

#if !defined(__NEW_ARMSYM__)
//-------------------------------------------
ElfDumper::ElfDumper(char *name, FILE* dump_fp, GetOpts* opts, int dump)
//-------------------------------------------
:	Outstrstuffs(dump_fp)
{
    isCompressed = false;
	fReader = new ElfReader(name, dump);
    if (!fReader)
    {
    	fprintf(stderr, "# No memory for dumper object\n");
    	exit(1);
    }
    if (Validate()!=se_success) {
    	DBG_ERR(("# invalid symbolics object!\n"));
    	return;
     	}
    _dump_opts = opts;
}
#endif

#if defined(__NEW_ARMSYM__)
//-------------------------------------------
ElfDumper::ElfDumper(char *name, FILE* dump_fp, GetOpts* opts, int dump)
//-------------------------------------------
:	SymDump(NULL, dump_fp,opts)
,	fReader(NULL)
,	fReaderIsMine(true)
{
	isCompressed = false;
	fReader = new ElfReader(dump);
    if (!fReader)
    {
    	fprintf(stderr, "# No memory for dumper object.\n");
    	exit(1);
    }
    _symnet = fReader;
    char fn[255];
    pname((StringPtr)fn,(char*)name);
    fReader->ISymbolics((StringPtr)fn);
    if (Validate()!=se_success) {
    	DBG_ERR(("# invalid symbolics object.\n"));
    	return;
     	}
    _dump_opts = opts;
}
#endif

#if defined(__NEW_ARMSYM__)
//-------------------------------------------
ElfDumper::ElfDumper(ElfReader* r, char *name, FILE* dump_fp, GetOpts* opts, int dump)
//-------------------------------------------
:	SymDump((SymNet*)this,dump_fp,opts)
,	fReader(r)
,	fReaderIsMine(false)
{
    isCompressed = false;
    if (!fReader)
    {
    	fprintf(stderr, "# No dumper object.\n");
    	exit(1);
    }
    fReader->_fp = new SymFile(name);
    if (!fReader->_fp)
    {
    	fprintf(stderr, "# No dumper Symfile object.\n");
    	exit(1);
    }
    if (Validate()!=se_success)
    {
    	DBG_ERR(("# invalid symbolics object!\n"));
    	return;
     }
    _dump_opts = opts;
}
#endif

//-------------------------------------------
ElfDumper::~ElfDumper()
//-------------------------------------------
{
	if (fReaderIsMine)
		delete fReader;
	//To accommodate the new generate_mapfile location MH 7/24/96
//otherwise it gets deleted again with "linker"object and crashes
//	else
	//	delete fReader->_fp;
}

#define set_err_msg fReader->set_err_msg
#define _state fReader->_state
#define _fp fReader->_fp
#define _symtab fReader->_symtab
#define _strtab fReader->_strtab
#define _sh_strtab fReader->_sh_strtab
#define _dumping fReader->_dumping
#define elf_hash_size fReader->elf_hash_size
#define elf_offset fReader->elf_offset
#define fmt_str fReader->fmt_str

#define elf_hash fReader->elf_hash
#define swap fReader->swap
#define symstr fReader->symstr
#define read_section fReader->read_section
#define find_sym_by_val fReader->find_sym_by_val
#define get_symname fReader->get_symname
#define get_symtab_nsyms fReader->get_symtab_nsyms
#define get_symtab_strtab fReader->get_symtab_strtab
#define get_secname fReader->get_secname
#define section_name fReader->section_name
#define swap_needed fReader->swap_needed
#define swapit fReader->swapit
#define symname fReader->symname
#define read_dynamic_data fReader->read_dynamic_data
#define lookup_symbol fReader->lookup_symbol
#define read_symbol_table_sorted fReader->read_symbol_table_sorted
#define sec_shdr fReader->sec_shdr
#define state fReader->state

//-------------------------------------------
void ElfDumper::DumpFile()
//-------------------------------------------
{
    if (Validate()!=se_success)
    {
    	DBG_ERR(("invalid symbolics object!\n"));
    	return;
    }
    if (_dump_opts->isset(dumpopt_none))
    	_dump_opts->set(dumpopt_generic);
    if (!_fp->open()) return;
    if (fReader->_elf_hdr || read_elf_hdr()==se_success)
    {
        //outstr("Elf dump of file \"%s\":\n", _fp->filename());
        if (_dump_opts->isset(dumpopt_header))
        {
            dump_elf_hdr();
            if (fReader->_sections->secnum(sec_hdr3do))
            	dump_3dohdr();
        }
        if (_dump_opts->isset(dumpopt_program_headers))
            dump_program_hdrs();
        if (fReader->_sec_hdr || read_section_hdrs()==se_success)
        {
            if (_dump_opts->isset(dumpopt_section_headers))
                dump_section_hdrs();
            //If the file we are reading has not been compressed mmh 96/08/01
            if(!CompFlag()) {
            	if (_dump_opts->isset(dumpopt_relocations))
                	dump_relocations();
                	}
                else { //Avoid crash in reading compressed relocations
                	outstr("\n\n------------------------------------------------------------------\n");
                	outstr("File %s has been compressed: No relocation entries available\n",_fp->filename());
                	outstr("------------------------------------------------------------------\n");
                	}
            if (_dump_opts->isset(dumpopt_symtab))
                dump_symbol_tables();
#ifdef DUMPER
			if (_dump_opts->isset(dumpopt_disasm))
                dump_disasm();
#endif
            if (_dump_opts->isset(dumpopt_dynamic_data))
            {
            	if (fReader->_sections->secnum(sec_imp3do))
            		dump_3doimports();
            	if (fReader->_sections->secnum(sec_exp3do))
            		dump_3doexports();
                dump_dynamic_data();
            }
            if (_dump_opts->isset(dumpopt_hash))
                dump_hash_table();
            if (_dump_opts->isset(dumpopt_content))
                dump_sections();
            if (_dump_opts->isset(dumpopt_debug_info))
            	dump_debug_info();
            if (_dump_opts->isset(dumpopt_lines))
            	dump_line_table();
        }
    }
    _fp->close();
    }

//==========================================================================
// dumpers

void ElfDumper::dump_elf_hdr() {
    outstr("\nElf Header\n");
    pad2(4);
    outstr("ident=<%s,%s,EI_VERSION=%d>",
            ei_class(fReader->_elf_hdr->e_ident[EI_CLASS]),
            ei_data(fReader->_elf_hdr->e_ident[EI_DATA]),
            fReader->_elf_hdr->e_ident[EI_VERSION]);
    pad2(49);
    outstr("type=%s\n", e_type(fReader->_elf_hdr->e_type));
    pad2(4);
    outstr("machine=%s", e_machine(fReader->_elf_hdr->e_machine));
    pad2(18);
    outstr("version=%ld", fReader->_elf_hdr->e_version);
    pad2(32);
    outstr("entry=0x%06lX", fReader->_elf_hdr->e_entry);
    pad2(49);
    outstr("flags=0x%lx\n", fReader->_elf_hdr->e_flags);

    pad2(4);
    outstr("ehsize=%u", fReader->_elf_hdr->e_ehsize);
    pad2(18);
    outstr("phoff=0x%lX", fReader->_elf_hdr->e_phoff);
    pad2(32);
    outstr("phentsize=%u", fReader->_elf_hdr->e_phentsize);
    pad2(49);
    outstr("phnum=%u\n", fReader->_elf_hdr->e_phnum);

    pad2(4);
    outstr("shoff=0x%lX", fReader->_elf_hdr->e_shoff);
    pad2(18);
    outstr("shentsize=%u", fReader->_elf_hdr->e_shentsize);
    pad2(32);
    outstr("shnum=%u", fReader->_elf_hdr->e_shnum);
    pad2(49);
    outstr("shstrndx=%d\n", fReader->_elf_hdr->e_shstrndx);
    }

void ElfDumper::dump_program_hdrs() {
    if (fReader->_elf_hdr->e_phoff && fReader->_elf_hdr->e_phnum) {
        uint32 num = fReader->_elf_hdr->e_phnum;
        uint32 entsize = fReader->_elf_hdr->e_phentsize;
        Elf32_Phdr *ph = (Elf32_Phdr*)MALLOC(num*entsize);
        if (!ph) return;
        if (!_fp->seek(fReader->_elf_hdr->e_phoff,SEEK_SET)) {
            SET_ERR(se_seek_err,("#Unable to seek to program header\n"));
            return;
            }
        if (!_fp->read(ph, entsize*num))
        {
            SET_ERR(se_read_err,("Error reading program header\n"));
            return;
        }
        outstr("\nProgram Headers (%d entries)\n",num);
    	pad2(4);
        outstr("index   type    vaddr     off       paddr     filesz    memsz  align  flags\n");
    	pad2(4);
    	outstr("---------------------------------------------------------------------------\n");
        for (uint32 i=0;i<num;i++) {
            Elf32_Phdr *p = ph+i;
#               define N(f) p->p_##f = swap(p->p_##f)
                N(type); N(offset); N(vaddr); N(paddr);
                N(filesz); N(memsz); N(flags); N(align);
#               undef N
    		pad2(4);
            outstr("  %-2d    %-7s 0x%06lX  0x%06lX",
                i, p_type(p->p_type), p->p_vaddr,p->p_offset);
            if (p->p_type == PT_INTERP && p->p_filesz < 256) {
                char *path = (char*) MALLOC(p->p_filesz+1);
                if (!path) return;
                if (!_fp->seek(p->p_offset,SEEK_SET)) {
                	SET_ERR(se_seek_err,("#Unable to seek to path offset\n"));
					return;
					}
                if (!_fp->read(path,p->p_filesz)) {
                	SET_ERR(se_read_err,("#Unable to read path\n"));
					return;
					}
                outstr("(\"%s\")",path);
                FREE(path);
                }
            outstr("  0x%06lX  0x%06lX  0x%06lX %02d  ",
                p->p_paddr,p->p_filesz,p->p_memsz,p->p_align);
            if (PF_R & p->p_flags) outstr("R");
            if (PF_W & p->p_flags) outstr("W");
            if (PF_X & p->p_flags) outstr("X");
            if (PF_C & p->p_flags) outstr("C");
            if (p->p_flags & ~PF_MASK)
                outstr(" 0x%lx",p->p_flags & ~PF_MASK);
            outstr("\n");
            }
        FREE(ph);
        }
    }

void ElfDumper::dump_section_hdrs() {
    outstr("\nSection Headers sorted by section offset (%d entries)\n",fReader->_elf_hdr->e_shnum - 1);
    pad2(4);
    outstr("index  section    type       addr     off      size     link  info  align  entsize flags\n");
    pad2(4);
    outstr("----------------------------------------------------------------------------------------\n");
    EachSectionByOffset es(fReader);
    while (es)
    {
        long i;
        Elf32_Shdr *p = es.next(i);
    	pad2(4);
        outstr("  %2d  %-11s %-10s 0x%06lX 0x%06lX 0x%06lX 0x%03lX 0x%03lX 0x%03lX  0x%03lX   ",
            i, symstr(p->sh_name), sh_type(p->sh_type),
            p->sh_addr, p->sh_offset,
            p->sh_size, p->sh_link, p->sh_info,
            p->sh_addralign, p->sh_entsize
            );
        int space_it = 0;
            if (SHF_EXECINSTR & p->sh_flags) {
                outstr("EXEC"); space_it = 1;
                };
            if (SHF_WRITE & p->sh_flags) {
                if (space_it) outstr("|");
                outstr("WRITE");
                space_it = 1;
                }
            if (SHF_ALLOC & p->sh_flags) {
                if (space_it) outstr("|");
                outstr("ALLOC");
                }
            if (SHF_COMPRESS & p->sh_flags) {
                setCompFlag();
                if (space_it) outstr("|");
                outstr("COMPRESS");
                }
            if (p->sh_flags &
                 ~(SHF_WRITE|SHF_EXECINSTR|SHF_ALLOC|SHF_COMPRESS))
            outstr(" 0x%-4lx",
                p->sh_flags&~(SHF_WRITE|SHF_EXECINSTR|SHF_ALLOC));
        outstr("\n");
    }
}

void ElfDumper::dump_3dohdr()
{
	uint32 sidx = fReader->_sections->secnum(sec_hdr3do);
    if (!sidx) return;

    outstr("\n3DO Header\n");

    void* sectionBuf = (_3DOBinHeader*)read_section(sidx);
	if (!sectionBuf) return;
    _3DOBinHeader* _3do = (_3DOBinHeader*) ((uint32)sectionBuf + 16);

    outstr("    n_SubsysType  = %u\n", (unsigned char) _3do->_3DO_Item.n_SubsysType);
    outstr("    n_Type        = %u\n", (unsigned char) _3do->_3DO_Item.n_Type);
    outstr("    n_Priority    = %u\n", (unsigned char) _3do->_3DO_Item.n_Priority);
    outstr("    n_Version     = %u\n", (unsigned char) _3do->_3DO_Item.n_Version);
    outstr("    n_Revision    = %u\n", (unsigned char) _3do->_3DO_Item.n_Revision);
    outstr("    Flags         = 0x%02x (%s)\n", _3do->_3DO_Flags, dump_3doflags(_3do->_3DO_Flags));
    outstr("    OS_Version    = %u\n", (unsigned char) _3do->_3DO_OS_Version);
    outstr("    OS_Revision   = %u\n", (unsigned char) _3do->_3DO_OS_Revision);
    outstr("    Stack         = %u\n", _3do->_3DO_Stack);
    outstr("    Signature     = %u\n", _3do->_3DO_Signature);
    outstr("    SignatureLen  = %u\n", _3do->_3DO_SignatureLen);
    outstr("    MaxUSecs      = %u\n", _3do->_3DO_MaxUSecs);
    outstr("    Name          = %s\n", _3do->_3DO_Name);
    outstr("    Date/Time     = %s\n", dump_3dotime((time_t)_3do->_3DO_Time));
    FREE(sectionBuf);
}

//-------------------------------------------
void ElfDumper::dump_3doimports()
//-------------------------------------------
{
	importTemplate* i;
    if (!_symtab)
    {
		if (read_main_symbol_table() != se_success)
		{
			//exports/imports should have a symbol table with at least the
			//externals/undefineds that are to be exported/imported so we
			//can search by name
			//We might require references by ordinal number
			//for release distributions to reduce size.
			// jrm 96/04/29.  I think that is in fact now done.
			outstr("# Dumping imports, but unable to get names from symbol table.\n");
		}
	}
	uint32 sidx = fReader->_sections->secnum(sec_imp3do);
	if (!sidx || fReader->_sec_hdr[sidx].sh_size==0) return;
    i = (importTemplate*)read_section(sidx);
	if (!i || !i->numImports) return;
    outstr("\n3DO Imports (%d entries)\n", i->numImports);
	char* imp_strings = IMP_STRINGS(i);	//get address of strings
    pad2(4);	outstr("index");
    pad2(10);	outstr("name(index)");
    pad2(26);	outstr("lib_code");
    pad2(35);	outstr("lib_ver");
    pad2(43);	outstr("lib_rev");
    pad2(54);	outstr("flags");
    outstr("\n");
    pad2(4);
	outstr("-------------------------------------------------------\n");
	int noff;
	for (int j=0; j<i->numImports; j++)
	{
    	pad2(6);	outstr("%d", j);
		noff = i->records[j].nameOffset;
    	pad2(10);	outstr("%s(0x%X)", noff?(imp_strings+noff):"---",noff);
    	pad2(26);	outstr("x%X", i->records[j].libraryCode);
    	pad2(35);	outstr("x%X", i->records[j].libraryVersion);
    	pad2(43);	outstr("x%X", i->records[j].libraryRevision);
    	pad2(54);	outstr("x%X", i->records[j].flags);
    	outstr("\n");
	}
}

//-------------------------------------------
void ElfDumper::dump_3doexports()
//-------------------------------------------
{
	exportTemplate* e;
	uint32 sidx = fReader->_sections->secnum(sec_exp3do);
	if (!sidx || fReader->_sec_hdr[sidx].sh_size==0) return;
    if (!_symtab)
    {
		if (read_main_symbol_table() != se_success)
		{
			//exports/imports should have a symbol table with at least the
			//externals/undefineds that are to be exported/imported so we
			//can search by name
			//We might require references by ordinal number
			//for release distributions to reduce size.
			// jrm 96/04/29.  In fact that is now the case.
			outstr("# Dumping exports, but unable to get names from symbol table.\n");
		}
	}
    e = (exportTemplate*)read_section(sidx);
	if (!e || !e->numExports) return;
    outstr("\n3DO Exports (libID %d; %d entries)\n", e->libraryID, e->numExports);
    pad2(4);	outstr("index");
    pad2(10);	outstr("secnum");
    pad2(20);	outstr("offset");
    pad2(30);	outstr("symidx");
    pad2(40);	outstr("name");
    outstr("\n");
	pad2(4);
	outstr("-------------------------------------------------\n");
	for (int j=0; j<e->numExports; j++)
	{
    	int secnum = ELF3DO_EXPSECNUM(e->exportWords[j]);
    	uint32 xoffset = ELF3DO_EXPOFFSET(e->exportWords[j]);
    	pad2(6);	outstr("%3d", j);
    	pad2(10);	outstr("%4d", secnum);
    	pad2(20);	outstr("0x%06X", xoffset);
		if (_symtab)
		{
			int symidx = find_sym_by_val(secnum,xoffset);
	    	pad2(30);	outstr("0x%04X", symidx);
			char* name = get_symname(symidx);
	    	pad2(40);	outstr("%s", name);
		}
		else { //curly braces added, bug fix mmh 09/11/96
			pad2(30);	
			outstr("Syms unavailable");
		}
    	outstr("\n");
	}
}

//-------------------------------------------
void ElfDumper::dump_symbol_tables()
//-------------------------------------------
{
	EachSection es(fReader);
	long i;
	while (es)
	{
		Elf32_Shdr *p = es.next(i);
		if (p->sh_type==SHT_SYMTAB||p->sh_type==SHT_DYNSYM)
			dump_symbol_table(i);
	}
}

//-------------------------------------------
void ElfDumper::dump_symbol_table(unsigned long sidx)
//-------------------------------------------
{
    Elf32_Sym *symtab;
    uint32 nsym;
    char *strtab;
    uint32 nstr;
	Elf32_Shdr *s = &fReader->_sec_hdr[sidx];
    // safe for main_symbol_table since that's the only one we
    // read in ElfReader and we make sure we don't delete it
    if (read_symbol_table(sidx,symtab,strtab)==se_success)
    {
    	outstr("\nSymbol table \"%s\"\n",symstr(s->sh_name));
    	nsym = get_symtab_nsyms(s);
    	nstr = get_symtab_strtab(s)->sh_size;
    	pad2(4);
        outstr("index value    size   bind type section     name\n");
    	pad2(4);
        outstr("-------------------------------------------------\n");
        for (uint32 i=0; i<nsym;i++)
        {
            Elf32_Sym *p = symtab+i;
    		pad2(4);
            outstr("%3d   0x%-8lx %-4d %-4s %-4s %-11.11s %s\n",
                i,p->st_value,p->st_size,
                st_bind(ELF32_ST_BIND(p->st_info)),
                st_type(ELF32_ST_TYPE(p->st_info)),
                section_name(p->st_shndx),
                ((unsigned)p->st_name < nstr)?strtab+p->st_name:"???");
        }
        // free symbol table & string table
        // but keep _symtab and main_strtab because ElfReader needs these
        if (symtab != _symtab) FREE(symtab);
        if (strtab != _sh_strtab && strtab != _strtab) FREE(strtab);
    }
}

void ElfDumper::dump_hash_table() {
	EachSection es(fReader);
	long i;
	while (es) {
		Elf32_Shdr *p = es.next(i);
		if (p->sh_type == SHT_HASH)
			dump_hash_table_sec(i);
			}
		}

void ElfDumper::dump_hash_table_sec(unsigned long sidx) {
    Elf32_Sym *symtab;
    uint32 nsym;
    char *strtab;
    uint32 nstr;
    uint32 nbucket;
    uint32 nchain;
	Elf32_Shdr *s = fReader->_sec_hdr + sidx;
    TEST_SECT(link);
    Elf32_Shdr *s_link = fReader->_sec_hdr+s->sh_link;
    // safe for main_symbol_table since that's the only one we
    // read in ElfReader and we make sure we don't delete it here
    if (read_symbol_table(s->sh_link, symtab, strtab)==se_success) {
    	nsym = get_symtab_nsyms(s_link);
    	nstr = get_symtab_strtab(s_link)->sh_size;
        if (!_fp->seek(s->sh_offset,SEEK_SET))
        	SET_ERR(se_seek_err,("seek to hash table error\n"));
        if (!_fp->read(&nbucket,4))
        	SET_ERR(se_read_err,("#Unable to read nbuckets\n"));
        if (!_fp->read(&nchain,4))
        	SET_ERR(se_read_err,("#Unable to read nchains\n"));
        if (swap_needed()) {
            swap(nbucket);
            swap(nchain);
            }
        outstr("\nHash table for symbol table \"%s\"\n",
              symstr(fReader->_sec_hdr[s->sh_link].sh_name));
        outstr("nbucket=%ld,  nchain=%ld\n",nbucket,nchain);
        if (nchain != nsym)
            outstr("nchain not equal to symbol count (%ld)!!\n",nsym);
        long *bucket = (long*) MALLOC(nbucket*sizeof(long));
        if (!bucket) return;
        long *chain = (long*) MALLOC(nchain*sizeof(long));
        if (!chain) return;
        if (!_fp->read(bucket,sizeof(long)*nbucket) ||
                !_fp->read(chain,sizeof(long)*nchain)) {
            SET_ERR(se_read_err,("Error reading hash table\n"));
            FREE(bucket); FREE(chain);
            return;
            }
        uint32 i;
        if (swap_needed()) {
            for(i=0;i<nbucket;i++) bucket[i] = swap(bucket[i]);
            for(i=0;i<nchain;i++) chain[i] = swap(chain[i]);
            }

        // Check for validity of chains
        for(i=0;i<nchain;i++) if (chain[i] >= nchain) {
            outstr("chain %ld (\"%s\") is invalid (=%ld)\n",i,
                    strtab+symtab[i].st_name,chain[i]);
            chain[i] = 0;
            }
        for(i=0;i<nbucket;i++) if (bucket[i] >= nchain) {
            outstr("bucket[%ld] is invalid (=%ld)\n",i,bucket[i]);
            bucket[i] = 0;
            }
        elf_hash_size = nbucket;
        for (i=0; i<nbucket; i++) {
            uint32 b = bucket[i];
            if (b >= nsym){
                outstr("Bucket %ld has invalid entry: %ld\n", i,b);
                continue;
                }
            outstr("bucket %ld:",i);
            while(b) {
                char *name = strtab+symtab[b].st_name;
                outstr(" %s",name);
                uint32 h = elf_hash(name);
                if (h != i)
                    outstr("hash of %s should be %lu!\n",name,h);
                #define SET_SEEN(b) (chain[b] |= 0x80000000)
                #define IS_SEEN(b) (chain[b] & 0x80000000)
                if (IS_SEEN(b)) {
                    outstr("\nchain entry %ld (%s) in more than one bucket!\n",
                              chain[b]&0x7FFFFFFF,strtab+symtab[chain[b]&0x7FFFFFFF].st_name);
                    break;
                    }
                uint32 bnext = chain[b];
                if (bnext)
                    SET_SEEN(b);        //Detect duplicate
                b = bnext;
                }
            outstr("\n");
            }
        // Look for unused chains
        for(i=0;i<nchain;i++) if (chain[i] && !IS_SEEN(i)) {
            outstr("Symbol %s (index %d) not in hash chain!\n",
                    strtab+symtab[i].st_name,i);
            }
        // free symbol table & string table
        // but keep main_symtab and main_strtab because ElfReader needs these
        if (strtab != _sh_strtab && strtab != _strtab) FREE(strtab);
        if (symtab != _symtab) FREE(symtab);	//FREE(symtab);
        }
    }

void ElfDumper::dump_relocations() {
	EachSection es(fReader);
	long i;
	while (es) {
		Elf32_Shdr *p = es.next(i);
		if (p->sh_type==SHT_REL||p->sh_type==SHT_RELA)
			dump_relocation_sections(i);
			}
		}

void ElfDumper::dump_relocation_sections(unsigned long sidx) {

    Elf32_Sym *symtab;
    uint32 nsym=0, nstr=0;
    char *strtab;
    Elf32_Shdr *s = fReader->_sec_hdr + sidx;
    TEST_SECT(info);
    TEST_SECT(link);
    outstr("\nRelocation entries for section \"%s\" (index %d)\n",
                symstr(fReader->_sec_hdr[s->sh_info].sh_name),s->sh_info);
    Elf32_Shdr *s_link = fReader->_sec_hdr+s->sh_link;

    if (read_symbol_table(s->sh_link, symtab, strtab)==se_success) {
    	nsym = get_symtab_nsyms(s_link);
    	nstr = get_symtab_strtab(s_link)->sh_size;
    	}
	int rela = s->sh_type==SHT_RELA;
	uint32 entsize = s->sh_entsize;
	unsigned cnt = s->sh_size/entsize;
	Elf32_Word rbuf[8];     //Big enough for biggest relocation entry
	if (sizeof rbuf < entsize){
		DBG_ERR(("entsize field of relocation segment %s is invalid(=0x%lx)",
			symstr(fReader->_sec_hdr[s->sh_info].sh_name),entsize));
		return;
		}
	if (!_fp->seek(s->sh_offset,SEEK_SET)) {
		SET_ERR(se_seek_err,("seek to symbol table error\n"));
		return;
		}
	outstr("    offset        relInfo         symbol");
	if (rela) outstr("                addend");
	outstr("\n    -----------------------------------");
	if (rela) outstr("------------------------");
	outstr("\n");
	unsigned i;
	for (i=0;i<cnt;i++){
		if (!_fp->read(&rbuf,entsize)) {
			SET_ERR(se_read_err,( "Error reading relocation entries.\n"));
			break;
			}
		uint32 j, rtype;
		for (j=0; j*sizeof(Elf32_Word) < entsize; j++)
			rbuf[j] = swap(rbuf[j]);
#       define FREL(p,field) ((Elf32_Rel*)p)->field
#       define FRELA(p,field) ((Elf32_Rela*)p)->field
		rtype = ELF32_R_TYPE(FREL(rbuf,r_info));
		uint32 symidx = ELF32_R_SYM(FREL(rbuf,r_info));
		outstr("     0x%-8lx %3d:%-11s",
					rbuf[0], rtype,r_type(rtype));
		if (!symtab)
			outstr("Syms unavailable"); // don't try to mess with symbols!
		else if (ISA_BASEREL(rtype))
		{
			//symidx is secnum
			SymSec sectype = fReader->_sections->sectype(symidx);
			outstr(" %3d:%-12s", symidx,
				(unsigned)symidx <fReader->_sections->nsecs()?get_secname(sectype):"???");
		}
		else if (ISA_IMPREL(rtype))
		{
			//symidx is import ordinal & symbol ordinal
			uint32 impsym = ELF32_R_IMPSYM(FREL(rbuf,r_info));
			uint32 impord = ELF32_R_IMPLIB(FREL(rbuf,r_info));
			//outstr(" x%08x:(imp %d, sym %d)", symidx,
			//	(symidx&0xff0000)>>23, symidx&0x00ffffff);
			outstr(" x%08x:(impord %d, symord %d)", symidx, impord, impsym);
		}
		else
		{
			outstr(" %3d:%-12s", symidx,
				(unsigned)symidx <nsym?symname(&symtab[symidx],strtab,nstr):"???");
		}
		if (rela)
			outstr("        0x%lx", FRELA(rbuf,r_addend));
		outstr("\n");
		}
	// free symbol table & string table
	// but keep main_symtab and main_strtab because ElfReader needs these
	if (strtab && strtab != _sh_strtab && strtab != _strtab) FREE(strtab);
	if (symtab && symtab != _symtab) FREE(symtab);
    }

void ElfDumper::dump_dyn_entry(Elf32_Dyn *dyn, char *strtab, uint32 nstr) {
    outstr("    %s",d_tag(dyn->d_tag));
#   define V dyn->d_un.d_val
#   define P dyn->d_un.d_ptr
    switch(dyn->d_tag){
        case DT_NULL:
        		outstr("DT_NULL");
        		break;
        case DT_RPATH:
        		outstr("DT_RPATH");
                outstr(",\"%s\"",V<nstr?strtab+V:"???"); break;
        case DT_SONAME:
        		outstr("DT_SONAME");
                outstr(",\"%s\"",V<nstr?strtab+V:"???"); break;
        case DT_NEEDED:
        		outstr("DT_NEEDED");
                outstr(",\"%s\"",V<nstr?strtab+V:"???"); break;
        case DT_PLTREL:
        		outstr("DT_PLTREL");
                outstr(", %s",d_tag(V)); break;
        case DT_RELASZ:
        		outstr("DT_RELASZ");
                outstr(", size=%ld(0x%lx)",V,V); break;
        case DT_RELAENT:
        		outstr("DT_RELAENT");
                outstr(", size=%ld(0x%lx)",V,V); break;
        case DT_STRSZ:
        		outstr("DT_STRSZ");
                outstr(", size=%ld(0x%lx)",V,V); break;
        case DT_RELSZ:
        		outstr("DT_RELSZ");
                outstr(", size=%ld(0x%lx)",V,V); break;
        case DT_RELENT:
        		outstr("DT_RELENT");
                outstr(", size=%ld(0x%lx)",V,V); break;
        case DT_SYMENT:
        		outstr("DT_SYMENT");
                outstr(", size=%ld(0x%lx)",V,V); break;
        case DT_PLTRELSZ:
        		outstr("DT_PLTRELSZ");
                outstr(", size=%ld(0x%lx)",V,V); break;
        case DT_HASH:
        		outstr("DT_HASH");
                outstr(", off=0x%lx",V); break;
        case DT_REL:
        		outstr("DT_REL");
                outstr(", off=0x%lx",V); break;
        case DT_RELA:
        		outstr("DT_RELA");
                outstr(", off=0x%lx",V); break;
        case DT_SYMTAB:
        		outstr("DT_SYMTAB");
                outstr(", off=0x%lx",V); break;
        case DT_PLTGOT:
        		outstr("DT_PLTGOT");
                outstr(", off=0x%lx",V); break;
        case DT_FINI:
        		outstr("DT_FINI");
                outstr(", adr=0x%lx",V); break;
        case DT_STRTAB:
        		outstr("DT_STRTAB");
                outstr(", adr=0x%lx",V); break;
        case DT_DEBUG:
        		outstr("DT_DEBUG");
                outstr(", adr=0x%lx",V); break;
        case DT_JMPREL:
        		outstr("DT_JMPREL");
                outstr(", adr=0x%lx",V); break;
        case DT_INIT:
        		outstr("DT_INIT");
                outstr(", adr=0x%lx",V); break;
        default:
        		outstr("unknown");
                outstr(", value=0x%lx",V); break;
        }
    outstr("\n");
#   undef V
#   undef P
    }


void ElfDumper::dump_dynamic_data() {
	EachSection es(fReader);
	long i;
	while (es) {
		Elf32_Shdr *p = es.next(i);
		if (p->sh_type == SHT_DYNAMIC)
			dump_dynamic_data_sec(i);
			}
		}

void ElfDumper::dump_dynamic_data_sec(unsigned long sidx)
{
    Elf32_Dyn *dyn; uint32 ndyn;
    char *strtab; uint32 nstr;
    Elf32_Shdr *s = fReader->_sec_hdr + sidx;
    outstr("Dynamic section \"%s\"\n", symstr(s->sh_name));
    if (read_dynamic_data(sidx,dyn,ndyn,strtab,nstr)==se_success) {
        for (uint32 i=0;i<ndyn;i++) {
            Elf32_Dyn *p = dyn+i;
            dump_dyn_entry(p,strtab,nstr);
            }
        FREE(dyn);
        if (strtab != _sh_strtab && strtab != _strtab) FREE(strtab);
        }
}


#define _min(x,y) ((x) < (y)?(x):(y))
#define Oneof2(x,a,b) (((x) == (a)) || ((x) == (b)))

void ElfDumper::dump_data(Elf32_Shdr *s) {
#define BSIZE   1024
    long ptr,size;
    int k,len;
    unsigned char buf[BSIZE];
    if (!_fp->seek( s->sh_offset, 0)) {
        SET_ERR(se_seek_err,("#Unable to seek to section\n"));
        return;
        }
    size = s->sh_size;
    uint32 adr = s->sh_addr;
    uint32 sidx = s - fReader->_sec_hdr;
    for (ptr = 0; ptr < size; )
        {
        len = size - ptr;

        if (len > BSIZE) len = BSIZE;
        if ((ptr+adr) % 16) len = _min(len,16-(ptr+adr)%16);

        if (!_fp->read(buf,len))
        	SET_ERR(se_read_err,("#Unable to read nbuckets\n"));
        for (k = 0; k < len; )
            {
            uint32 l;
            l = len - k;
            if (l > 16) l = 16;
            uint32 off;
            Elf32_Sym *where = lookup_symbol(sidx,ptr+k+l,off);
            if (where && off <= l){  // symbol refernces this text?
                long i;
                long last_off = 0;
                for(i=0;i<l;i++){
                    Elf32_Sym *s = lookup_symbol(sidx,ptr+k+i,off);
                    if (s && off==0){
                        if (i > last_off)
                            dumphex(buf+k+last_off,i-last_off,
                                        ptr+k+adr+last_off);
                        last_off = i;
                        outstr("%s:\n",_strtab+s->st_name);
                        }
                    }
                if (last_off < l)
                    dumphex(buf+k+last_off,l-last_off,ptr+k+adr+last_off);
                }
            else
                dumphex(buf+k, l, ptr+k+adr);
            k += l;
            }
        ptr += len;
        }
    }

void ElfDumper::dump_sections() {
	outstr("\nSections Contents Dump\n");
	Elf32_Sym **by_section; Elf32_Sym **by_address;
	Elf32_Sym *symtab; uint32 nsym;
	char *strtab; uint32 nstr;
	// read in the main symtab and sort it
	uint32 sidx = fReader->_sections->secnum(sec_symtab);
	if (!sidx) {
		outstr("\n\tNo symbol table found for file\n");
		SET_INFO(se_not_found,("No symbol table found for file\n"));
		return;
		}
	read_symbol_table_sorted(sidx,
			by_section, by_address, symtab, nsym, strtab, nstr);
	EachSection es(fReader);
	while (es) {
		long i;
		Elf32_Shdr *p = es.next(i);
		char *s = _sh_strtab + p->sh_name;
		if (_dump_opts->isset(dumpopt_content))
			if (p->sh_type != SHT_NOBITS)
				dump_section(p,i);
		}
	}

void ElfDumper::dump_section(Elf32_Shdr *s, uint32 i) {
    outstr("Dump of section %s (index %d) offset=0x%lx size=0x%lx\n",
                _sh_strtab+s->sh_name, i, s->sh_offset, s->sh_size);
    dump_data(s);
    }

// dump line table info
SymErr ElfDumper::dump_line_table() {
    DBG_ENT("dump_line_table");
    outstr("\n\nDump of line info\n");
    Elf32_Shdr* s = sec_shdr(sec_line);
    if (!s)
       RETURN_INFO(se_no_line_info,("No line sections!\n"));

    Elf32_Lhdr line_hdr;
    uint32 bytes_read = 0;
    DBG_ASSERT(s);
    if (!_fp->seek(s->sh_offset,SEEK_SET))
        RETURN_ERR(se_seek_err,("#Unable to seek to line table at offset=0x%X\n",s->sh_offset));
    // read line table
    while (bytes_read < s->sh_size) {
    // line_hdr is aligned so we can read the whole thing
        if (!_fp->read(&line_hdr,8))
            RETURN_ERR(se_read_err,("Error reading line header\n"));
        bytes_read += 8;
        if (swap_needed()) {
         	line_hdr.lh_size=swap(line_hdr.lh_size);
        	line_hdr.lh_svaddr=swap(line_hdr.lh_svaddr);
        	}
        int32 lines_size = line_hdr.lh_size-8; //size includes header
        if (lines_size>0) {
        	DBG_(ELF,("line_hdr.lh_size=0x%X, line_hdr.lh_svaddr=0x%X\n",line_hdr.lh_size, line_hdr.lh_svaddr));
			pad2(4);
			outstr("lines for module at vaddr 0x%X:\n",line_hdr.lh_svaddr);
			pad2(8);
            outstr("line      code offset  code addr\n");
			pad2(8);
            outstr("--------------------------------\n");
			pad2(8);
        	char *mod_lines = (char*) MALLOC(lines_size);
        	if (!mod_lines) return state();
        	DBG_ASSERT(mod_lines);
        	if (!_fp->read(mod_lines,lines_size)) {
        	    FREE(mod_lines);
        	    RETURN_ERR(se_read_err,("Error reading lines entries for lines at svaddr=0x%X\n",line_hdr.lh_svaddr));
        		}
        	bytes_read += lines_size;
        	char* lines_ptr;
        	unsigned long ln_num;
        	unsigned long ln_addr;
        	// the middle short word is for a character offset, but tools vendors ignore it
        	for (lines_ptr = mod_lines; lines_ptr < (mod_lines+lines_size);
					lines_ptr += SIZEOF_LINE) {
					// alignment is 16 bits so must handle specially
 					// could also do a memcpy and align...

            	memcpy(&ln_num,lines_ptr,4);
            	memcpy(&ln_addr,lines_ptr+6,4);
            	if (swap_needed()) {
                	ln_num=swap(ln_num);      // line number of module
                	ln_addr=swap(ln_addr);     // code offset from start of module
                	}
				pad2(8);
            	outstr("%-3d       0x%-8x   0x%-8x\n",ln_num,ln_addr,ln_addr+line_hdr.lh_svaddr);
            	}
        	if (mod_lines) FREE(mod_lines);
            }
        }
    return state();
    }
SymErr ElfDumper::dump_debug_info() {
    DBG_ENT("dump_debug_info");
    // Read dwarf debug info
    Elf32_Shdr* s = sec_shdr(sec_debug);
    DBG_ASSERT(s);
    if (!s || !s->sh_size)
 		RETURN_INFO(se_no_debug_info,("No debug section in file.\n"));
    outstr("\nDebug info");
	indent();
    outnl();
    unsigned char* buf;
    uint32 bytes_read = 0;
    uint32 buf_size;
    uint32 debug_base = s->sh_offset;
	#ifdef _DDI_BUG	//this is a temp hack (I hope)
		extern void set_debug_base(uint32 base);
		//don't do if have .rela.debug (in which case offsets will be relative)
		if (fReader->_sections->secnum(sec_reladebug))
			set_debug_base(0);
		else
			set_debug_base(sec_shdr(sec_debug)->sh_addr);
		DBG_(ELF,("BUG!!  Diab Data's ref should be offset from debug section!\n"));
	#endif
    if (!_fp->seek(s->sh_offset,SEEK_SET))
        RETURN_ERR(se_seek_err,("#Unable to seek to debug section at offset=0x%X\n",s->sh_offset));
    while (bytes_read < s->sh_size) {
    // read size of first entry
    	elf_offset = _fp->tell() - debug_base;	//save to use for type referencing
    	outnl();
    	outstr("<0x%8X>::",elf_offset);
    	DBG_(ELF,("elf_offset=0x%X\n",elf_offset));
        if (!_fp->read(&buf_size,4))
            RETURN_ERR(se_read_err,("Error reading debug entry size\n"));
        bytes_read += 4;
        swapit(buf_size);
        DBG_(ELF,("buf_size=x%X\n",buf_size));
        buf_size-=4;
        if (buf_size>0) {
        	buf = (unsigned char*) MALLOC(buf_size);
        	if (!buf) return state();
        	if (!_fp->read(buf,buf_size)) {
        		FREE(buf);
            	RETURN_ERR(se_read_err,("Error reading debug entry\n"));
            	}
            BufEaters buf_obj((Endian*)this,buf_size,buf);	//stream handler for buf
        	dump_debug_entry(&buf_obj);
        	bytes_read += buf_size;
        	FREE(buf);
        	}
        else
        	dump_debug_entry(0);
        }
    return state();
    }

SymErr ElfDumper::dump_debug_entry(BufEaters* buf) {
    DBG_ENT("dump_debug_entry");
	if (!buf || buf->bites_left()<4) {	//already read 4
		if (buf) buf->eat_nbites(buf->bites_left());
		//null entry - means we're at the end of a list of things
		//if we had any open structs/unions/enums, close them now...
    	outstr("(null)");
		return state();
		}
    DBG_DUMP(buf->bites_left(),buf->ptr());
    uint16 tag = buf->eat_uint16();	//buf must be aligned
    //===================================
    outstr("%s",dump_tag_name(tag));
	indent();
	dump_tag(buf);
	unindent();
	return state();
	}

void ElfDumper::dump_tag(BufEaters* buf) {
	uint16 attr;
	char* atname=0;
	while (buf->bites_left()) {
		attr = buf->eat_uint16();
		attr_val aval(attr,buf);	//gets the attr's value based on the form of the attr
    	atname = dump_at_name(attr);
    	outnl();
    	outstr("%-20s",atname);
    	if (strlen(atname)>20) {
    		outnl();
    		outstr("                    ");
    		}
		dump_at(attr,&aval);
		indent();
		dump_at_cont(attr,&aval);
		unindent();
		}
	}

void ElfDumper::dump_at(uint16 attr,attr_val* aval) {
	switch (attr&FORM_MASK) {
		case FORM_ADDR:
			outstr("addr(x%lx) ",aval->addr);
			break;
		case FORM_REF: {
			outstr("ref(");
			#ifdef _DDI_BUG	//this is non-svr4
				extern uint32 get_debug_base();
				uint32 ref = aval->ref + get_debug_base();
				outstr("x%lx/",ref);
			#endif
			outstr("0x%X) ",aval->ref);
		    }
			break;
		case FORM_BLOCK2:
			outstr("block2(len=0x%X, block=",aval->block.len.s16);
			dumpbytes(aval->block.len.s16,aval->block.ptr);
			outstr(") ");
			break;
		case FORM_BLOCK4:
			outstr("block4(len=0x%X, block=",aval->block.len.s32);
			dumpbytes(aval->block.len.s32,aval->block.ptr);
			outstr(") ");
			break;
		case FORM_DATA2:
			outstr("data2(0x%2X) ",aval->con.s16);
			break;
		case FORM_DATA4:
			outstr("data4(0x%4lx) ",aval->con.s32);
			break;
		case FORM_DATA8:
			outstr("data8(0x%8lx 0x%8lx) ",aval->con.s64.hi,aval->con.s64.lo);
			break;
		case FORM_STRING:
			outstr("string(%s) ",aval->str);
			break;
		default:
			outstr("unknown(0x%8X) ",attr&FORM_MASK);
			attr = 0;
		}
	}

void ElfDumper::dump_at_cont(uint16 attr,attr_val* aval) {
	switch (attr) {
		case AT_location:
			dump_loc(aval->block.len.s16,aval->block.ptr);
			break;
		case AT_fund_type:
			outstr("type(%s) ",dump_type_name(aval->con.s16));
			break;
		case AT_mod_fund_type:
			dump_mod_type(aval->block.len.s16,aval->block.ptr);
			break;
		case AT_user_def_type:
			outstr("(ref=0x%8X) ",aval->ref);
			break;
		case AT_mod_u_d_type:
			dump_mod_user_type(aval->block.len.s16,aval->block.ptr);
			break;
		}
	}

void ElfDumper::dump_loc(uint32 len,unsigned char* buf) {
	DBG_ENT("dump_loc");
	uint32 v;

	outnl();
	pad2(36);
	DBG_ASSERT(len>0 && buf);
	int i = 0;
	while (i<len) {
		switch (buf[i++]) {	//first byte identifies atom
			case LOP_REG:
				GET_UINT32(v,buf,i);
				outstr("LOP_REG(0x%X) ",v);
				break;
			case LOP_BASEREG:
				GET_UINT32(v,buf,i);
				outstr("LOP_BASEREG(0x%X) ",v);
				break;
			case LOP_CONST:
				GET_UINT32(v,buf,i);
				outstr("LOP_CONST(0x%X) ",v);
				break;
			case LOP_ADDR:
				GET_UINT32(v,buf,i);
				outstr("LOP_ADDR(0x%X) ",v);
				break;
			case LOP_DEREF2: {
				outstr("LOP_DEREF2 ");
				DBG_ERR(("what to do?\n"));
				}
				break;
			case LOP_DEREF4: {
				outstr("LOP_DEREF4 ");
				DBG_ERR(("what to do?\n"));
				}
				break;
			case LOP_ADD: {
				outstr("LOP_ADD ");
				}
				break;
			case LOP_lo_user: {
				outstr("LOP_lo_user ");
				DBG_ERR(("what to do?\n"));
				}
				break;
			case LOP_hi_user: {
				outstr("LOP_hi_user ");
				DBG_ERR(("what to do?\n"));
				}
				break;
			default:
				outstr("unknown(0x%X) ",buf[i-1]);
				SET_ERR(se_unknown_type,("unknown location atom 0x%X\n",buf[i-1]));
			}
		}
	}

void ElfDumper::dump_mod_type(uint16 len,unsigned char* buf) {
	outnl();
	pad2(36);
	DBG_ASSERT(len>0 && buf);
	uint16 v;
	len-=2;
	GET_UINT16(v,buf,len);	//last 2 bytes are fund_type (len will advance by 2)
	dump_mod_types(len-2,buf);	//add modifiers to types
	outstr("type(%s) ",dump_type_name(v));	//get base type
	}

void ElfDumper::dump_mod_user_type(uint32 len,unsigned char* buf) {
	DBG_ASSERT(len>0 && buf);
	uint32 v;
	len-=4;
	GET_UINT32(v,buf,len);	//last 4 bytes are user_type  (len will advance by 4)
	dump_mod_types(len-4,buf);	//add modifiers to types
	outstr("type(0x%X) ",v);	//get base type
	}

void ElfDumper::dump_mod_types(uint32 len,unsigned char* buf) {
	int i=0;
	while (i<len) {
		switch (buf[i++]) {	//first byte identifies mod
			case MOD_pointer_to:
				outstr("MOD_pointer_to ");
				break;
			case MOD_reference_to:
				outstr("MOD_reference_to ");
				break;
			case MOD_const:
				outstr("MOD_const ");
				break;
			case MOD_volatile:
				outstr("MOD_volatile ");
				break;
			case MOD_lo_user:
				outstr("MOD_lo_user ");
				break;
			case MOD_hi_user:
				outstr("MOD_hi_user ");
				break;
			default:
				outstr("unknown ");
				SET_ERR(se_unknown_type,("unknown mod fund type 0x%X\n",buf[i-1]));
			}
		}
	};

//==========================================================================
// helpers

char* ElfDumper::ei_class(int i) {
    switch(i) {
        case ELFCLASSNONE: return "CLASSNONE";
        case ELFCLASS32: return "CLASS32";
        case ELFCLASS64: return "CLASS64";
        default: return "UNKNOW ELFCLASS";
        }
    }

char* ElfDumper::ei_data(int i) {
    switch(i) {
        case ELFDATANONE: return "DATANONE";
        case ELFDATA2LSB: return "DATA2LSB";
        case ELFDATA2MSB: return "DATA2MSB";
        default: return "UNKNOW ELFDATA";
        }
    }

char* ElfDumper::e_type(Elf32_Half i) {
    switch(i) {
        case ET_NONE: return "none";
        case ET_REL:  return "rel";
        case ET_EXEC: return "exec";
        case ET_DYN:  return "dyn";
        case ET_CORE: return "core";
        default:
            return fmt_str("0x%X", i);
        }
    }

char* ElfDumper::e_machine(Elf32_Half i) {
    switch(i){
        case EM_NONE: return "NONE";
        case EM_M32:  return "AT&T WE 32100";
        case EM_SPARC:  return "SPARC";
        case EM_386:  return "I386";
        case EM_68K:  return "MC68K";
        case EM_88K:  return "MC88K";
        case EM_860:  return "I860";
        case EM_PPC:  return "PPC";
        default:
            return fmt_str("0x%X", i);
        }
    }

char* ElfDumper::sh_type(Elf32_Word t) {
    switch(t) {
        case SHT_NULL: return "null";
        case SHT_PROGBITS: return "progbits";
        case SHT_SYMTAB: return "symtab";
        case SHT_STRTAB: return "strtab";
        case SHT_RELA: return "rela";
        case SHT_HASH: return "hash";
        case SHT_DYNAMIC: return "dynamic";
        case SHT_NOTE: return "note";
        case SHT_NOBITS: return "nobits";
        case SHT_REL: return "rel";
        case SHT_SHLIB: return "shlib";
        case SHT_DYNSYM: return "dynsym";
        default:
            return fmt_str("0x%lx", t);
        }
    }

char* ElfDumper::p_type(Elf32_Word l) {
    switch(l){
        case PT_NULL: return "null";
        case PT_LOAD: return "load";
        case PT_DYNAMIC: return "dynamic";
        case PT_INTERP: return "interp";
        case PT_NOTE: return "note";
        case PT_SHLIB: return "shlib";
        case PT_PHDR: return "phdr";
        default:
            return fmt_str("0x%lx",l);
        }
    }

char* ElfDumper::r_type(uint32 r) {
    switch(r) {
        case R_PPC_NONE				: return "none";
        case R_PPC_ADDR32			: return "addr32";
        case R_PPC_ADDR24			: return "addr24";
        case R_PPC_ADDR16			: return "addr16";
        case R_PPC_ADDR16_LO		: return "addr16lo";
        case R_PPC_ADDR16_HI		: return "addr16hi";
        case R_PPC_ADDR16_HA		: return "addr16ha";
        case R_PPC_ADDR14			: return "addr14";
        case R_PPC_ADDR14_BRTAKEN	: return "addr14brt";
        case R_PPC_ADDR14_BRNTAKEN	: return "addr14brn";
        case R_PPC_REL24			: return "rel24";
        case R_PPC_REL14			: return "rel14";
        case R_PPC_REL14_BRTAKEN	: return "rel14brt";
        case R_PPC_REL14_BRNTAKEN	: return "rel14brn";
        case R_PPC_RELATIVE			: return "relative";
        case R_PPC_GOT16			: return "got16";
        case R_PPC_GOT16_LO			: return "got16lo";
        case R_PPC_GOT16_HI			: return "got16hi";
        case R_PPC_GOT16_HA			: return "got16ha";
        case R_PPC_PLTREL24			: return "pltrel24";
        case R_PPC_COPY				: return "copy";
        case R_PPC_GLOB_DAT			: return "globdat";
        case R_PPC_JMP_SLOT			: return "jmpslot";
        case R_PPC_LOCAL24PC		: return "local24pc";
        case R_PPC_UADDR32			: return "uaddr32";
        case R_PPC_UADDR16			: return "uaddr16";
        case R_PPC_REL32			: return "rel32";
        case R_PPC_PLT32			: return "plt32";
        case R_PPC_PLTREL32			: return "pltrel32";
        case R_PPC_PLT16_LO			: return "plt16lo";
        case R_PPC_PLT16_HI			: return "plt16hi";
        case R_PPC_PLT16_HA			: return "plt16ha";
        case R_PPC_SDAREL16			: return "sdarel16";
        case R_PPC_SECTOFF			: return "sectoff";
        case R_PPC_SECTOFF_LO		: return "sectofflo";
        case R_PPC_SECTOFF_HI		: return "sectoffhi";
        case R_PPC_SECTOFF_HA		: return "sectoffha";
        //baserels
        case R_PPC_BASEREL32		: return "baserel32";
        case R_PPC_BASEREL24		: return "baserel24";
        case R_PPC_BASEREL16		: return "baserel16";
        case R_PPC_BASEREL16_LO		: return "baserel16lo";
        case R_PPC_BASEREL16_HI		: return "baserel16hi";
        case R_PPC_BASEREL16_HA		: return "baserel16ha";
        case R_PPC_BASEREL14		: return "baserel14";
        case R_PPC_BASEREL14_BRTAKEN	: return "baserel14br";
        case R_PPC_BASEREL14_BRNTAKEN	: return "baserel14brn";
        case R_PPC_UBASEREL16		: return "ubaserel16";
        case R_PPC_UBASEREL32		: return "ubaserel32";
		//imports
        case R_PPC_IMPADDR32		: return "impaddr32";
        case R_PPC_IMPADDR24		: return "impaddr24";
        case R_PPC_IMPADDR16		: return "impaddr16";
        case R_PPC_IMPADDR16_LO		: return "impaddr16lo";
        case R_PPC_IMPADDR16_HI		: return "impaddr16hi";
        case R_PPC_IMPADDR16_HA		: return "impaddr16ha";
        case R_PPC_IMPADDR14		: return "impaddr14";
        case R_PPC_IMPADDR14_BRTAKEN	: return "impaddr14br";
        case R_PPC_IMPADDR14_BRNTAKEN	: return "impaddr14brn";
        case R_PPC_UIMPADDR16		: return "uimpaddr16";
        case R_PPC_UIMPADDR32		: return "uimpaddr32";
        case R_PPC_IMPREL24			: return "imprel24";
        case R_PPC_IMPREL14			: return "imprel14";
        case R_PPC_IMPREL14_BRTAKEN	: return "imprel14br";
        case R_PPC_IMPREL14_BRNTAKEN	: return "imprel14brn";
        case R_PPC_IMPREL32			: return "imprel32";
        case R_PPC_IMPRELATIVE		: return "imprelative";
        default:
            return fmt_str("0x%lx",r);
        }
    }

char* ElfDumper::st_bind(int i) {
    switch(i) {
        case STB_LOCAL: return "loc";
        case STB_GLOBAL: return "glob";
        case STB_WEAK: return "weak";
        default:
            return fmt_str("0x%lx",i);
        }
    }

char* ElfDumper::st_type(int i) {
    switch(i) {
        case STT_NOTYPE: return "null";
        case STT_OBJECT: return "obj";
        case STT_FUNC: return "func";
        case STT_SECTION: return "sect";
        case STT_FILE: return "file";
        default:
            return fmt_str("0x%lx",i);
        }
    }

char* ElfDumper::d_tag(unsigned i) {
#   define C(x) case DT_##x: return #x
    switch(i){
        C(NULL); C(NEEDED); C(PLTRELSZ); C(PLTGOT);
        C(HASH); C(STRTAB); C(SYMTAB); C(RELA);
        C(RELASZ); C(RELAENT); C(STRSZ); C(SYMENT);
        C(INIT); C(FINI); C(SONAME); C(RPATH);
        C(SYMBOLIC); C(REL); C(RELSZ); C(RELENT);
        C(PLTREL); C(DEBUG); C(TEXTREL); C(JMPREL);
        default:
            return fmt_str("0x%X",i);
        }
#   undef C
    }

char* ElfDumper::dump_tag_name(uint16 tag) {
#   define C(x) case x: return #x
    switch (tag) {
		C(TAG_padding);
		C(TAG_array_type);
		C(TAG_class_type);
 		C(TAG_entry_point);
		C(TAG_enumeration_type);
 		C(TAG_formal_parameter);
		C(TAG_global_subroutine);
		C(TAG_global_variable);
		C(TAG_label);
		C(TAG_lexical_block);
		C(TAG_local_variable);
		C(TAG_member);
		C(TAG_pointer_type);
		C(TAG_reference_type);
		C(TAG_compile_unit);	//module!!
		C(TAG_string_type);
		C(TAG_structure_type);
		C(TAG_subroutine);
		C(TAG_subroutine_type);
		C(TAG_typedef);
		C(TAG_union_type);
		C(TAG_unspecified_parameters);
		C(TAG_variant);
		C(TAG_common_block);
		C(TAG_common_inclusion);
		C(TAG_inheritance);
		C(TAG_inlined_subroutine);
		C(TAG_module);
		C(TAG_ptr_to_member_type);
		C(TAG_set_type);
 		C(TAG_subrange_type);
		C(TAG_with_stmt);
		default:
       		SET_ERR(se_unknown_type,("unknown TAG=0x%X\n",tag));
			return fmt_str("0x%X",tag);
        }
#   undef C
	}

//taken from fixup3do
char *ElfDumper::dump_3doflags(uint8 flags) {
	char* s=0;

	if (flags&_3DO_NO_CHDIR) 	s=s?fmt_str("%s,NO_CHDIR",s):"NO_CHDIR";
	if (flags&_3DO_PRIVILEGE) 	s=s?fmt_str("%s,PRIVILEGE",s):"PRIVILEGE";
	if (flags&_3DO_PCMCIA_OK) 	s=s?fmt_str("%s,PCMCIA_OK",s):"PCMCIA_OK";
	if (flags&_3DO_SHOWINFO) 	s=s?fmt_str("%s,SHOWINFO",s):"SHOWINFO";
	if (flags&_3DO_MODULE_CALLBACKS)s=s?fmt_str("%s,MODULE_CALLBACKS",s):"MODULE_CALLBACKS";
	if (flags&_3DO_SIGNED) 		s=s?fmt_str("%s,SIGNED",s):"SIGNED";
	return s?s:"-none-";
	}

char *ElfDumper::dump_3dotime(time_t secs) {
    /* secs is the number of seconds since 01/01/93 00:00:00 GMT */
    static char	  prbuf[18] = { '\0', };
    struct tm *lt;

    /* convert to the number of seconds since 1970 */
    secs += ((23*365+6)*24*60*60);
    lt = localtime(&secs);

    sprintf(prbuf,"%02d/%02d/%02d %02d:%02d:%02d", lt->tm_mon+1, lt->tm_mday,
			lt->tm_year, lt->tm_hour, lt->tm_min, lt->tm_sec);
    return prbuf;
	}

char* ElfDumper::dump_at_name(uint16 attr) {
#   define C(x) case x: return #x
	switch (attr) {
 		C(AT_specification);	//same as AT_abstract_origin;
 		C(AT_sibling);
 		C(AT_location);
 		C(AT_name);
 		C(AT_fund_type);
 		C(AT_mod_fund_type);
 		C(AT_user_def_type);
 		C(AT_mod_u_d_type);
 		C(AT_ordering);
 		C(AT_subscr_data);
 		C(AT_byte_size);
 		C(AT_bit_offset);
 		C(AT_bit_size);
 		C(AT_element_list);
 		C(AT_stmt_list);
 		C(AT_low_pc);
 		C(AT_high_pc);
 		C(AT_language);
 		C(AT_member);
 		C(AT_discr);
 		C(AT_discr_value);
 		C(AT_string_length);
 		C(AT_common_reference);
 		C(AT_comp_dir);
     	C(AT_const_value_string);
     	C(AT_const_value_data2);
     	C(AT_const_value_data4);
     	C(AT_const_value_data8);
     	C(AT_const_value_block2);
     	C(AT_const_value_block4);
 		C(AT_containing_type);
     	C(AT_default_value_addr);
     	C(AT_default_value_data2);
     	C(AT_default_value_data4);
     	C(AT_default_value_data8);
     	C(AT_default_value_string);
 		C(AT_friends);
 		C(AT_inline);
 		C(AT_is_optional);
     	C(AT_lower_bound_ref);
     	C(AT_lower_bound_data2);
     	C(AT_lower_bound_data4);
     	C(AT_lower_bound_data8);
 		C(AT_private);
 		C(AT_producer);
 		C(AT_program);
 		C(AT_protected);
 		C(AT_prototyped);
 		C(AT_public);
 		C(AT_pure_virtual);
 		C(AT_return_addr);
 		C(AT_start_scope);
 		C(AT_stride_size);
     	C(AT_upper_bound_ref);
     	C(AT_upper_bound_data2);
     	C(AT_upper_bound_data4);
     	C(AT_upper_bound_data8);
 		C(AT_virtual);
    	/* GNU extensions.  */
 		C(AT_sf_names);
 		C(AT_src_info);
 		C(AT_mac_info);
 		C(AT_src_coords);
 		C(AT_body_begin);
 		C(AT_body_end);
 		C(AT_lo_user);
 		C(AT_hi_user);
    	/* Diab C++ extensions.  */
 		C(AT_mangled_name);
 		C(AT_glob_cpp);
		default:
			SET_ERR(se_unknown_type,("unknown or invalid attribute 0x%X\n",attr));
			return fmt_str("0x%X",attr);
		}
#   undef C
	}

char* ElfDumper::dump_type_name(uint32 elftype) {
	switch (elftype) {
    	case FT_void:				return "void";
    	case FT_pointer:			return "pointer";
		case FT_char:				return "char";
    	case FT_signed_char:		return "schar";
    	case FT_unsigned_char:		return "uchar";
    	case FT_short:				return "short";
    	case FT_signed_short:		return "sshort";
    	case FT_unsigned_short:		return "ushort";
    	case FT_integer:			return "int";
    	case FT_signed_integer:		return "sint";
    	case FT_unsigned_integer:	return "uint";
    	case FT_long:				return "long";
    	case FT_signed_long:		return "slong";
    	case FT_unsigned_long:		return "ulong";
    	case FT_float:				return "float";
    	case FT_dbl_prec_float:		return "dblfloat";
    	case FT_ext_prec_float:		return "extfloat";
    	case FT_complex:			return "complex";
    	case FT_dbl_prec_complex:	return "dblcomplex";
    	case FT_ext_prec_complex:	return "extcomplex";
    	case FT_boolean:			return "boolean";
    	case FT_label:				return "label";
		case FT_lo_user:			return "louser";
		case FT_hi_user:			return "hiuser";
		default:
				SET_ERR(se_unknown_type,("unknown fundamental type 0x%X\n",elftype));
				return "unknown";
		}
	}

//disassemble the text section
void ElfDumper::dump_disasm() {
	uint8* text;
	uint32 text_size;
	int sidx = fReader->_sections->secnum(sec_code);
	if (!sidx)
		return;
	text = read_section(sidx);
	if (!text)
		return;
	text_size = fReader->_sec_hdr[sidx].sh_size;
	_dumping&=~DUMP_FILE_ONLY;	//turn off DUMP_FILE_ONLY so will read symbols
	read_main_symbol_table();
	DumpDisasm(fReader->_sections->baseaddr(sidx), text, text_size);
	FREE(text);
	}

