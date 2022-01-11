/*  @(#) elf_3do.h 96/09/09 1.17 */


#ifndef __ELF_3DO_H__
#define __ELF_3DO_H__

#include "loaderty.h"
#ifdef macintosh
//it's nodes.h in the OS...
#include "operatypes.h"
#endif
#include "header3do.h"

//--------------------------------------------------
//3do sections
#define TYP_3DO_BINHEADER 	((uint32) 0x3d0)
#define TYP_3DO_IMPORTS 	((uint32) 'i')
#define TYP_3DO_EXPORTS 	((uint32) 'e')
#define SEC_3DO_NAME 		((uint32) 0x534b4800)	/* SKH\0 */

struct sec3do {
	uint32 namesz;
	uint32 descsz;
	uint32 type;
	uint32 name;	/* Always SKH\0 */
        // desc follows
	};
#define NOTE_SIZE	0x10	//4 long words
	
//3do header section
struct _3DOBinHeaderSection
        {
        sec3do	      note;
        _3DOBinHeader hdr3do;
        };

#define SEC_HDR_3DO_SIZE 0x90	//sizeof(_3DOBinHeaderSection)
#define HDR_3DO_SIZE 0x80	//sizeof(_3DOBinHeader)


//--------------------------------------------------
//DLLs
	//macros to insert and extract export words
	#define ELF3DO_EXPSECNUM(xword) (((xword)>>24)&0xff)    
	#define ELF3DO_EXPOFFSET(xword) ((xword)&0xffffff)    
	#define ELF3DO_EXPWORD(secnum,offset) ((secnum)<<24 | (offset))    

//--------------------------------------------------
//imports
struct importRecord {
	uint32 nameOffset;
	uint32 libraryCode;
	uint8  libraryVersion;
	uint8  libraryRevision;
	uint8  flags;
	uint8  pad;
	};

struct importTemplate {
        sec3do note;		// section header

	uint32 numImports;
	importRecord records[1];
	//char* strings; strings follow and are pointed to by namOffset
	};
//import flags
#define IMP3DO_IMPORT_NOW       0x0001
#define IMP3DO_REIMPORT_ALLOWED 0x0002
#define IMP3DO_IMPORT_REQUIRED  0x0004
//get position of strings within buffer in RAM
//get position of strings within buffer from Big Endian disk
#define IMP_RECORD_START(i) ((char*)i+0xa)	//after the note header
#define IMP_RECORD_SIZE 0xc	//sizeof(importRecord)
#define IMP_TEMPLATE_SIZE 0x20	//sizeof(importTemplate)
#define IMP_STRINGS_RAM(i)	\
	((char*)(i)	\
	 	+ IMP_RECORD_SIZE*(i)->numImports \
		+ IMP_TEMPLATE_SIZE - 4
		//4 = sizeof(char*))	

//we subtract 1 from numImports because def of importTemplate
//already stores room for 1 impRecord
//this gives us the start of the strings
#define IMP_STRINGS_OFF(nimps)	\
	 	(IMP_RECORD_SIZE*(nimps-1) \
		+ IMP_TEMPLATE_SIZE)	
#define IMP_STRINGS(i)	\
	((char*)(i)	\
	 	+ IMP_STRINGS_OFF(i->numImports))
#define IMP_SIZE(nimps,str_size)	\
	 	(IMP_RECORD_SIZE*(nimps-1) \
		+ IMP_TEMPLATE_SIZE	 \
		+ str_size)	


//--------------------------------------------------
//export 
#define EXP_DATAMASK 0x80000000 
struct exportTemplate {
        sec3do note;		// section header

	uint32 libraryID;
	uint32 numExports;
	uint32 exportWords[1];
	};
#define EXP_TEMPLATE_SIZE (NOTE_SIZE+0x0c)	//sizeof(exportTemplate)
	
//--------------------------------------------------
//Other 3do defines

//3do symbols
#define SYM_SEGMASK 0x80000000 
#define SYM_MODMASK 0x7F000000 
#define SYM_INDMASK 0x00FFFFFF 
//bits:	0 		seg;	//segment of imported module symbol lies within
//		1:7 	mod;	//which module contains symbol
//		8:23 	ind;	//symbol index within imported module
	
//module creation
#define TAG_ITEM_NAME 		1
#define TAG_ITEM_VERSION 	2
#define TAG_ITEM_REVISION	3
#define CREATEMODULE_TAG_MASK		0x00010000
#define CREATEMODULE_TAG_MEMBASE	1 | CREATEMODULE_TAG_MASK
#define CREATEMODULE_TAG_XIP		2 | CREATEMODULE_TAG_MASK
#define CREATEMODULE_TAG_FNAME		3 | CREATEMODULE_TAG_MASK
#define CREATEMODULE_TAG_ABS		4 | CREATEMODULE_TAG_MASK
#define CREATEMODULE_TAG_ALLOCPROC	5 | CREATEMODULE_TAG_MASK
#define CREATEMODULE_TAG_SYSBIND	6 | CREATEMODULE_TAG_MASK
#define CREATEMODULE_TAG_NOBIND		7 | CREATEMODULE_TAG_MASK

#define CODEMODULE	1	//type

//task creation
#define TAG_MODULE				1
#define CREATETASK_TAG_MASK		0x00020000
#define CREATETASK_TAG_MODULE	1 | CREATETASK_TAG_MASK
#define CREATETASK_TAG_AIF		1 | CREATETASK_TAG_MASK

//symbol numbers
#define SYMNUM_EXPSYM_REMOVED	-1
#define SYMNUM_EXPDATA_REMOVED	-2
#define SYMNUM_EXPTEXT_REMOVED	-3

#endif /* __ELF_3DO_H__ */
