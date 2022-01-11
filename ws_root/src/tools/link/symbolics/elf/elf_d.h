/*  @(#) elf_d.h 96/09/10 1.12 */

//====================================================================
// elf_d.h  -  ElfDumper class defs for dumping ELF files with dwarf v1.1 debug info
//
//		Symbolics class hierarchy:
//
//			Symbolics	- contains main functions visible to world
//				SymNet	- builds internal network of symbols and queries them
//					SymReader - reads ARM sym files with sym debug info and
//								calls SymNet methods to add symbols to network
//						SymDumper - dumps ARM sym files 
//					XcoffReader - reads XCOFF files with dbx stabs debug info and
//								calls SymNet methods to add symbols to network
//						XcoffDumper - dumps XCOFF files 
//					ElfReader - reads ELF files with dwarf v1.1 debug info and
//								calls SymNet methods to add symbols to network
//						ElfDumper - dumps ELF files 

#ifndef __ELF_D_H__
#define __ELF_D_H__

#include <time.h>
#include "elf_r.h"
#include "symdump.h"
#include "dumputils.h"
#include "dumpopts.h"

class ElfDumper : public SymDump {
public:
    ElfDumper(char *name, FILE* dump_fp,GetOpts*,int dump=DUMP_ALL);
    ElfDumper(ElfReader* r, char *name, FILE* dump_fp,GetOpts*,int dump=DUMP_ALL);
    virtual ~ElfDumper();
	virtual void DumpFile();	//file-specific dump
    void dump_elf_hdr();
    void dump_program_hdrs();
    void dump_section_hdrs();
    void dump_symbol_tables();
    void dump_symbol_table(unsigned long sidx);
    void dump_relocations();
    void dump_relocation_sections(unsigned long sidx);
	void dump_3dohdr();
	void dump_3doimports();
	void dump_3doexports();
    void dump_dynamic_data();
    void dump_dynamic_data_sec(unsigned long sidx);
    void dump_hash_table();
    void dump_hash_table_sec(unsigned long sidx);
    void dump_sections();
    void dump_section(Elf32_Shdr *s, unsigned long i);
	SymErr dump_line_table();
	SymErr dump_debug_info();
	SymErr dump_debug_entry(BufEaters* buf);
	void dump_tag(BufEaters* buf);
	void dump_at(uint16 attr,attr_val* aval);
	void dump_at_cont(uint16 attr,attr_val* aval);
	void dump_loc(uint32 len,unsigned char* buf);
	void dump_mod_type(uint16 len,unsigned char* buf);
	void dump_mod_user_type(uint32 len,unsigned char* buf);
	void dump_mod_types(uint32 len,unsigned char* buf);
	char* dump_tag_name(uint16 tag);
	char* dump_at_name(uint16 attr);
	char* dump_type_name(uint32 elftype);
	char* dump_3doflags(uint8 flags);
	char* dump_3dotime(time_t secs);
	void dump_disasm(void);
// Access to new fReader object jrm 96/06/12
    SymErr Validate() { return fReader->Validate(); }
    Boolean Valid() const { return fReader->Valid(); }
    char* GetErrStr() { return fReader->GetErrStr(); }
	void set_state(SymErr err, const char* file, uint32 line)
		{ fReader->set_state(err, file, line); }
// Access to the Compression flag mmh 96/08/01
   void setCompFlag() {isCompressed = true;}
   int CompFlag() {return isCompressed;}
// other public funcs
// fReaderIsMine true means we have to re-read the data.  If it is false,
// assume the data was already read into the client's reader objects.
#define DELEGATE(retVal, func) \
	{\
		if (fReaderIsMine) \
			return fReader->func(); \
		return retVal; \
	}
    SymErr read_elf_hdr()
    	DELEGATE(se_success, read_elf_hdr)
    SymErr read_section_hdrs()
    	DELEGATE(se_success, read_section_hdrs)
    SymErr read_main_symbol_table()
    	DELEGATE(se_success, read_main_symbol_table)
	SymErr read_debug()
    	DELEGATE(se_success, read_debug)
    SymErr read_line_table()
    	DELEGATE(se_success, read_line_table)
    SymErr read_string_table(unsigned long sidx, char *&strtab)
    {
    	return fReader->read_string_table(sidx, strtab);
    		// doesn't actually read, just gets it from the memory data.
    }
    SymErr read_symbol_table(unsigned long sidx, Elf32_Sym *&symtab, char *&strtab)
    {
    	return fReader->read_symbol_table(sidx, symtab, strtab);
    }
    
private:
    //Keeping track of whether the file has been compressed mmh
    Boolean isCompressed;
    ElfReader* fReader;
    Boolean fReaderIsMine;
    GetOpts* _dump_opts;
    void dump_data(Elf32_Shdr *s);
    void dump_dyn_entry(Elf32_Dyn *dyn, char *strtab, unsigned long nstr);
	char* e_type(Elf32_Half i);
	char* e_machine(Elf32_Half i);
	char* sh_type(Elf32_Word t);
	char* p_type(Elf32_Word l);
	char* r_type(uint32 r);
	char* st_bind(int i);
	char* st_type(int i);
	char* ei_class(int i);
	char* ei_data(int i);
	char* d_tag(unsigned i);
    };

#endif /* __ELF_D_H__ */

