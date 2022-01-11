#ifndef __LOADER_ELF_3DO_H
#define __LOADER_ELF_3DO_H


/******************************************************************************
**
**  @(#) elf_3do.h 96/07/31 1.13
**
******************************************************************************/


/* 3do sections */
#define TYP_3DO_BINHEADER 	((uint32) 0x3d0)
#define TYP_3DO_IMPORTS 	((uint32) 'i')
#define TYP_3DO_EXPORTS 	((uint32) 'e')
#define SEC_3DO_NAME 		((uint32) 0x534b4800)	/* SKH\0 */

/* import flags */
#define IMPORT_NOW       0x01
#define REIMPORT_ALLOWED 0x02
#define IMPORT_REQUIRED  0x04

/* 3do import relocations - these live in r_info */
#define ELF3DO_IMP_MODMASK 0xff000000
#define ELF3DO_IMP_INDMASK 0x00FFFF00
#define	ELF3DO_IMP_RSHIFT  8
#define ELF3DO_MOD_RSHIFT  24

/*
bits:
		0:7 	mod;	//which module contains symbol
		8:23 	ind;	//symbol index within imported module
*/

/* ELF3DO_EXPORT_INDEX returns the index within the exporting module's sect */
#define ELF3DO_EXPORT_INDEX(info) (((info) & ELF3DO_IMP_INDMASK) \
				   >> ELF3DO_IMP_RSHIFT)

/* ELF3DO_MODULE_INDEX returns the index in the importer's imp3do section */
#define ELF3DO_MODULE_INDEX(info) (((info) & ELF3DO_IMP_MODMASK) \
				   >> ELF3DO_MOD_RSHIFT)

typedef struct ELF_Note3DO
{
    uint32	namesz;	/* Always 4    			*/
    uint32	descsz;	/* # of bytes of desc data	*/
    uint32	type;	/* TYP_3DO_xxx			*/
    uint32	name;   /* Always SKH\0			*/

    /* desc follows - contains the section contents	*/
} ELF_Note3DO;

typedef struct ELF_ImportRec
{
    uint32	nameOffset;
    uint32	libraryCode;
    uint8	libraryVersion;
    uint8	libraryRevision;
    uint8	flags;
    uint8	pad;
} ELF_ImportRec;

typedef struct ELF_Imp3DO
{
    uint32		numImports;
    ELF_ImportRec	imports[1];	/* template	*/
} ELF_Imp3DO;

typedef struct ELF_Exp3DO
{
    uint32		libraryID;
    uint32		numExports;
    uint32		exportWords[1];	/* template	*/
} ELF_Exp3DO;

/* The following macros are used to extract the subfields of the exportWords
   array...

   ELF3DO_EXPORT_SECTION(word) - returns the section number which is exported
   ELF3DO_EXPORT_OFFSET(word)  - returns the offset within that section

   If the special section 0xff is used, it means the export is undefined.
*/

#define	ELF3DO_EXPORT_SECTION(word)	((word) >> 24)
#define	ELF3DO_EXPORT_OFFSET(word)	((word) & 0x00ffffff)
#define	ELF3DO_EXPORT_UNDEFINED_SECT	0xff


/*****************************************************************************/


#endif /* __LOADER_ELF_3DO_H */
