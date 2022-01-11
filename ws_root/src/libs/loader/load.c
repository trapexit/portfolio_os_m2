/* @(#) load.c 96/11/06 1.8 */

#include <kernel/types.h>
#include <kernel/cache.h>
#include <kernel/super.h>
#include <kernel/task.h>
#include <kernel/internalf.h>
#include <kernel/operror.h>
#include <file/filefunctions.h>
#include <loader/elftypes.h>
#include <loader/elf.h>
#include <loader/elf_3do.h>
#include <loader/loader3do.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "reloc.h"
#include "fileio.h"
#include "load.h"


#define ALIGN4(x)	(((x) + 3) & ~0x03)


/*****************************************************************************/


#ifdef BUILD_STRINGS
#define INFO(x) printf x
#else
#define INFO(x)
#endif


/*****************************************************************************/


typedef struct
{
    Elf32_Ehdr  eh_ElfHeader;
    Elf32_Phdr *eh_ProgramHeaders;
    Elf32_Shdr *eh_SectionHeaders;

    Elf32_Shdr *eh_TextSection;
    Elf32_Shdr *eh_DataSection;
    Elf32_Shdr *eh_BSSSection;
    Elf32_Shdr *eh_TextSectionRela;
    Elf32_Shdr *eh_DataSectionRela;
    Elf32_Shdr *eh_3DOBinHeaderSection;
    Elf32_Shdr *eh_3DOImportSection;
    Elf32_Shdr *eh_3DOExportSection;

    uint32      eh_TextSectionNum;
    uint32      eh_DataSectionNum;
    uint32      eh_BSSSectionNum;
} ELFHeaders;


/*****************************************************************************/


static void FreeElfHeaders(ELFHeaders *hdrs)
{
    FreeMem(hdrs->eh_ProgramHeaders, sizeof(Elf32_Phdr) * hdrs->eh_ElfHeader.e_phnum);
    FreeMem(hdrs->eh_SectionHeaders, sizeof(Elf32_Shdr) * hdrs->eh_ElfHeader.e_shnum);
}


/*****************************************************************************/


static Err ReadElfHeaders(ELFHeaders *hdrs, LoaderFile *file)
{
Elf32_Ehdr *elfHdr;
Elf32_Shdr *sectHdr;
Elf32_Phdr *prgHdr;
Err         result;
Elf32_Word  flags;
uint32      i;

    memset(hdrs, 0, sizeof(ELFHeaders));
    elfHdr = &hdrs->eh_ElfHeader;

    result = ReadLoaderFile(file, &hdrs->eh_ElfHeader, sizeof(Elf32_Ehdr));
    if (result >= 0)
    {
        result = LOADER_ERR_BADFILE;
        if ((elfHdr->e_ident[EI_MAG0] == ELFMAG0)
         && (elfHdr->e_ident[EI_MAG1] == ELFMAG1)
         && (elfHdr->e_ident[EI_MAG2] == ELFMAG2)
         && (elfHdr->e_ident[EI_MAG3] == ELFMAG3)
         && (elfHdr->e_ident[EI_CLASS] == ELFCLASS32)
         && (elfHdr->e_ident[EI_DATA] == ELFDATA2MSB)
         && ((elfHdr->e_type == ET_REL) || (elfHdr->e_type == ET_DYN))
         && ((elfHdr->e_machine == EM_PPC) || (elfHdr->e_machine == EM_PPC_EABI))
         && (elfHdr->e_version >= EV_CURRENT))
        {
            result = LOADER_ERR_NOMEM;
            hdrs->eh_ProgramHeaders = AllocMem(sizeof(Elf32_Phdr) * elfHdr->e_phnum, MEMTYPE_FILL);
            hdrs->eh_SectionHeaders = AllocMem(sizeof(Elf32_Shdr) * elfHdr->e_shnum, MEMTYPE_FILL);
            if (hdrs->eh_ProgramHeaders && hdrs->eh_SectionHeaders)
            {
                result = SeekLoaderFile(file, elfHdr->e_phoff);
                if (result >= 0)
                {
                    result = ReadLoaderFile(file, hdrs->eh_ProgramHeaders, sizeof(Elf32_Phdr) * elfHdr->e_phnum);
                    if (result >= 0)
                    {
                        result = SeekLoaderFile(file, elfHdr->e_shoff);
                        if (result >= 0)
                        {
                            result = ReadLoaderFile(file, hdrs->eh_SectionHeaders, sizeof(Elf32_Shdr) * elfHdr->e_shnum);
                        }
                    }
                }
            }
        }
    }

    if (result >= 0)
    {
        sectHdr = hdrs->eh_SectionHeaders;
        for (i = 0; i < elfHdr->e_shnum; i++)
        {
            flags = sectHdr->sh_flags & (~SHF_COMPRESS);

            switch (sectHdr->sh_type)
            {
                case SHT_PROGBITS: if (flags == (SHF_WRITE | SHF_ALLOC))
                                   {
                                       hdrs->eh_DataSection    = sectHdr;
                                       hdrs->eh_DataSectionNum = i;
                                   }
                                   else if (flags == (SHF_ALLOC | SHF_EXECINSTR))
                                   {
                                       hdrs->eh_TextSection    = sectHdr;
                                       hdrs->eh_TextSectionNum = i;
                                   }
                                   else if (flags)
                                   {
                                       result = LOADER_ERR_BADFILE;
                                       INFO(("LOADER: unknown flag bits (0x%x) in section of type SHT_PROGBITS\n", flags));
                                   }
                                   break;

                case SHT_NOBITS  : if (flags == (SHF_ALLOC | SHF_WRITE))
                                   {
                                       hdrs->eh_BSSSection    = sectHdr;
                                       hdrs->eh_BSSSectionNum = i;
                                   }
                                   else
                                   {
                                       result = LOADER_ERR_BADFILE;
                                       INFO(("LOADER: unknown flag bits (0x%x) in section of type SHT_NOBITS\n", flags));
                                   }
                                   break;

                case SHT_RELA    : if (sectHdr->sh_info < elfHdr->e_shnum)
                                   {
                                       if (sectHdr->sh_info == hdrs->eh_TextSectionNum)
                                       {
                                           hdrs->eh_TextSectionRela = sectHdr;
                                           break;
                                       }
                                       else if (sectHdr->sh_info == hdrs->eh_DataSectionNum)
                                       {
                                           hdrs->eh_DataSectionRela = sectHdr;
                                           break;
                                       }
                                   }
                                   result = LOADER_ERR_BADFILE;
                                   INFO(("LOADER: relocations for unknown section %d\n", sectHdr->sh_info));
                                   break;

                case SHT_NOTE    : /* Note sections come in the following order:
                                    *
                                    * hdr3do
                                    * imp3do
                                    * exp3do
                                    */
                                   if (!hdrs->eh_3DOBinHeaderSection)
                                   {
                                       hdrs->eh_3DOBinHeaderSection = sectHdr;
                                   }
                                   else if (!hdrs->eh_3DOImportSection)
                                   {
                                       hdrs->eh_3DOImportSection = sectHdr;
                                   }
                                   else if (!hdrs->eh_3DOExportSection)
                                   {
                                       hdrs->eh_3DOExportSection = sectHdr;
                                   }
                                   else
                                   {
                                       result = LOADER_ERR_BADFILE;
                                       INFO(("LOADER: unknown note section\n"));
                                   }
                                   break;

                case SHT_NULL    :
                case SHT_SYMTAB  :
                case SHT_STRTAB  : /* ignore these... */
                                   break;

                case SHT_HASH    :
                case SHT_DYNAMIC :
                case SHT_REL     :
                case SHT_SHLIB   :
                case SHT_DYNSYM  :
                case SHT_LOPROC  :
                case SHT_HIPROC  :
                case SHT_LOUSER  :
                case SHT_HIUSER  : result = LOADER_ERR_BADSECTION;
                                   INFO(("LOADER: section type 0x%x is not supported\n", sectHdr->sh_type));
                                   break;

                default          : result = LOADER_ERR_BADFILE;
                                   INFO(("LOADER: unknown section type 0x%x\n", sectHdr->sh_type));
                                   break;
            }

            if (result < 0)
                break;

            sectHdr = (Elf32_Shdr *)((uint32)sectHdr + elfHdr->e_shentsize);
        }
    }

    if (result >= 0)
    {
        prgHdr = hdrs->eh_ProgramHeaders;
        for (i = 0; i < elfHdr->e_phnum; i++)
        {
            if ((prgHdr->p_type != PT_LOAD)
             && (prgHdr->p_type != PT_NOTE)
             && (prgHdr->p_type != PT_NULL))
            {
                result = LOADER_ERR_BADFILE;
                INFO(("LOADER: unknown program header type %x\n", prgHdr->p_type));
                break;
            }

            prgHdr = (Elf32_Phdr *)((uint32)prgHdr + elfHdr->e_phentsize);
        }
    }

#if 0
    if (result >= 0)
    {
        prgHdr = hdrs->eh_ProgramHeaders;
        printf("\nProgram Headers: offset   filesz   memsz\n");
        for (i = 0; i < elfHdr->e_phnum; i++)
        {
            printf("                 %6u %8u %7u\n", prgHdr->p_offset,
                                                     prgHdr->p_filesz,
                                                     prgHdr->p_memsz);

            prgHdr = (Elf32_Phdr *)((uint32)prgHdr + elfHdr->e_phentsize);
        }

        sectHdr = hdrs->eh_SectionHeaders;
        printf("\nSection Headers: offset   size type\n");
        for (i = 0; i < elfHdr->e_shnum; i++)
        {
            printf("                 %6u %6u ", sectHdr->sh_offset,
                                                sectHdr->sh_size);

            if (sectHdr == hdrs->eh_TextSection)
            {
                printf("text");
            }
            else if (sectHdr == hdrs->eh_DataSection)
            {
                printf("data");
            }
            else if (sectHdr == hdrs->eh_BSSSection)
            {
                printf("bss");
            }
            else if (sectHdr == hdrs->eh_TextSectionRela)
            {
                printf("rela.text");
            }
            else if (sectHdr == hdrs->eh_DataSectionRela)
            {
                printf("rela.data");
            }
            else if (sectHdr == hdrs->eh_3DOBinHeaderSection)
            {
                printf("hdr3do");
            }
            else if (sectHdr == hdrs->eh_3DOImportSection)
            {
                printf("imp3do");
            }
            else if (sectHdr == hdrs->eh_3DOExportSection)
            {
                printf("exp3do");
            }
            else
            {
                printf("unknown");
            }
            printf("\n");

            sectHdr = (Elf32_Shdr *)((uint32)sectHdr + elfHdr->e_shentsize);
        }
    }
#endif

    if (result >= 0)
    {
        if (hdrs->eh_TextSection && (hdrs->eh_TextSection->sh_size == 0))
        {
            hdrs->eh_TextSection    = NULL;
            hdrs->eh_TextSectionNum = 0;
        }

        if (hdrs->eh_DataSection && (hdrs->eh_DataSection->sh_size == 0))
        {
            hdrs->eh_DataSection    = NULL;
            hdrs->eh_DataSectionNum = 0;
        }

        if (hdrs->eh_BSSSection && (hdrs->eh_BSSSection->sh_size == 0))
        {
            hdrs->eh_BSSSection    = NULL;
            hdrs->eh_BSSSectionNum = 0;
        }

        if (hdrs->eh_TextSectionRela && (hdrs->eh_TextSectionRela->sh_size == 0))
            hdrs->eh_TextSectionRela = NULL;

        if (hdrs->eh_DataSectionRela && (hdrs->eh_DataSectionRela->sh_size == 0))
            hdrs->eh_DataSectionRela = NULL;

        if (hdrs->eh_3DOBinHeaderSection && (hdrs->eh_3DOBinHeaderSection->sh_size != sizeof(_3DOBinHeader) + sizeof(ELF_Note3DO)))
            hdrs->eh_3DOBinHeaderSection = NULL;

        if (hdrs->eh_3DOExportSection && (hdrs->eh_3DOExportSection->sh_size == 0))
            hdrs->eh_3DOExportSection = NULL;

        if (hdrs->eh_3DOImportSection && (hdrs->eh_3DOImportSection->sh_size == 0))
            hdrs->eh_3DOImportSection = NULL;
    }

    return result;
}


/*****************************************************************************/


/* find the program header associated with a given section header */
static Elf32_Phdr *FindProgramHeader(const ELFHeaders *hdrs,
                                     const Elf32_Shdr *sectHdr)
{
uint32      i;
Elf32_Phdr *prgHdr;

    prgHdr = hdrs->eh_ProgramHeaders;
    for (i = 0; i < hdrs->eh_ElfHeader.e_phnum; i++)
    {
        if (prgHdr->p_filesz
         && (sectHdr->sh_offset >= prgHdr->p_offset)
         && (sectHdr->sh_offset < prgHdr->p_offset + prgHdr->p_filesz - 1))
        {
            if (((sectHdr->sh_type == SHT_NOTE) && (prgHdr->p_type == PT_NOTE))
             || ((sectHdr->sh_type == SHT_RELA) && (prgHdr->p_type == PT_NOTE))
             || ((sectHdr->sh_type == SHT_PROGBITS) && (prgHdr->p_type == PT_LOAD)))
            {
                return prgHdr;
            }
        }

        prgHdr = (Elf32_Phdr *)((uint32)prgHdr + hdrs->eh_ElfHeader.e_phentsize);
    }

    return NULL;
}


/*****************************************************************************/


static Err ReadTextSection(LoaderFile *file,
                           ELFHeaders *hdrs,
                           LoaderInfo *li)
{
Elf32_Shdr *sectHdr;
Elf32_Phdr *prgHdr;
Err         result;

    sectHdr = hdrs->eh_TextSection;
    if (!sectHdr)
	return 0;

    prgHdr = FindProgramHeader(hdrs, sectHdr);
    if (!prgHdr)
        return LOADER_ERR_BADFILE;

    li->codeLength = ALIGN4(prgHdr->p_memsz);
    li->codeBase   = AllocMemAligned(li->codeLength, MEMTYPE_NORMAL, KB_FIELD(kb_DCacheBlockSize));

    if (!li->codeBase)
        return LOADER_ERR_NOMEM;

    result = SeekLoaderFile(file, sectHdr->sh_offset);
    if (result >= 0)
    {
        if (sectHdr->sh_flags & SHF_COMPRESS)
            result = ReadLoaderFileCompressed(file, li->codeBase, sectHdr->sh_size, li->codeLength);
        else
            result = ReadLoaderFile(file, li->codeBase, sectHdr->sh_size);
    }

    return result;
}


/*****************************************************************************/


static Err ReadDataSection(LoaderFile *file,
                           ELFHeaders *hdrs,
                           LoaderInfo *li)
{
Elf32_Phdr *prgHdr;
Elf32_Shdr *dataSectHdr;
Elf32_Shdr *bssSectHdr;
Err         result;

    dataSectHdr = hdrs->eh_DataSection;
    bssSectHdr  = hdrs->eh_BSSSection;

    if (dataSectHdr == NULL)
    {
        if (bssSectHdr == NULL)
            return 0;

        li->dataLength = 0;
        li->bssLength  = bssSectHdr->sh_size;
    }
    else
    {
        prgHdr = FindProgramHeader(hdrs, dataSectHdr);
        if (!prgHdr)
            return LOADER_ERR_BADFILE;

        li->dataLength = ALIGN4(prgHdr->p_memsz);
        li->bssLength  = 0;

        if (bssSectHdr)
            li->bssLength = bssSectHdr->sh_size;
    }

    if (li->dataLength + li->bssLength == 0)
    {
        /* nothing to load */
        return 0;
    }

    if (li->flags & LF_PRIVILEGED)
    {
        /* if privileged, use the operator's pages */
        li->dataBase = AllocMem(li->dataLength + li->bssLength, MEMTYPE_NORMAL | MEMTYPE_FILL);
    }
    else
    {
        /* allocate totally separate pages for unprivileged module data */
        li->dataBase = AllocMemPages(li->dataLength + li->bssLength, MEMTYPE_NORMAL | MEMTYPE_FILL);
    }

    if (!li->dataBase)
	return LOADER_ERR_NOMEM;

    if (!dataSectHdr)
    {
        /* no data section, just BSS, so we're done */
	return 0;
    }

    result = SeekLoaderFile(file, dataSectHdr->sh_offset);
    if (result >= 0)
    {
        if (dataSectHdr->sh_flags & SHF_COMPRESS)
            result = ReadLoaderFileCompressed(file, li->dataBase, dataSectHdr->sh_size, li->dataLength);
        else
            result = ReadLoaderFile(file, li->dataBase, dataSectHdr->sh_size);
    }

    return result;
}


/*****************************************************************************/


static Err Read3DOSection(LoaderFile *file,
                          const Elf32_Shdr *sectHdr,
			  uint32 expectedType,
                          void **sect)
{
void        *buffer;
Err          result;
ELF_Note3DO  note;

    *sect = NULL;

    result = LOADER_ERR_BADFILE;
    if (sectHdr->sh_size >= sizeof(ELF_Note3DO))
    {
        result = SeekLoaderFile(file, sectHdr->sh_offset);
        if (result >= 0)
        {
            result = ReadLoaderFile(file, &note, sizeof(note));
            if (result >= 0)
            {
                result = LOADER_ERR_BADFILE;
                if (note.name == SEC_3DO_NAME)
                {
                    if (note.type == expectedType)
                    {
                        if (note.descsz <= sectHdr->sh_size - sizeof(ELF_Note3DO))
                        {
                            buffer = AllocMem(sectHdr->sh_size - sizeof(ELF_Note3DO), MEMTYPE_NORMAL);
                            if (buffer)
                            {
                                result = ReadLoaderFile(file, buffer, note.descsz);
                                if (result >= 0)
                                {
                                    *sect = buffer;
                                }
                                else
                                {
                                    FreeMem(buffer, note.descsz);
                                }
                            }
                            else
                            {
                                result = LOADER_ERR_NOMEM;
                            }
                        }
                        else
                        {
                            INFO(("LOADER: note size (%u) is larger than section size (%u)\n", note.descsz, sectHdr->sh_size - sizeof(ELF_Note3DO)));
                        }
                    }
                    else
                    {
                        INFO(("LOADER: expected note section type (got 0x%x, wanted 0x%x)\n", note.type, expectedType));
                    }
                }
                else
                {
                    INFO(("LOADER: 0x%x is not a valid note section name\n", note.name));
                }
            }
        }
    }
    else if (sectHdr->sh_size == 0)
    {
        /* 'cause the linker spits these out once in awhile */
        result = 0;
    }
    else
    {
        INFO(("LOADER: note section too small (%u)\n", sectHdr->sh_size));
    }

    return result;
}


/*****************************************************************************/


/* converts section offsets into pointers */
static Err PrepExportTable(LoaderInfo *li)
{
uint32 *exports;
uint32  i;
uint32  offset;
uint32  sectLen;
uint32  section;
uint32  numExports;

    if (li->exports)
    {
        numExports = (li->exportsSize - sizeof(ELF_Exp3DO) + sizeof(uint32)) / sizeof(uint32);
        if (li->exports->numExports > numExports)
        {
            INFO(("LOADER: export count exceeds number allowed\n"));
            return LOADER_ERR_BADFILE;
        }

        exports = li->exports->exportWords;
        for (i = 0; i < li->exports->numExports; i++)
        {
            offset  = ELF3DO_EXPORT_OFFSET(exports[i]);
            section = ELF3DO_EXPORT_SECTION(exports[i]);

            if (section == li->codeSectNum)
            {
                exports[i] = (uint32)li->codeBase + offset;
                sectLen    = li->codeLength;
            }
            else if (section == li->dataSectNum)
            {
                exports[i] = (uint32)li->dataBase + offset;
                sectLen    = li->dataLength;
            }
            else if (section == li->bssSectNum)
            {
                exports[i] = (uint32)li->dataBase + ALIGN4(li->dataLength) + offset;
                sectLen    = li->bssLength;
            }
            else if (section == ELF3DO_EXPORT_UNDEFINED_SECT)
            {
                exports[i] = 0;    /* UNDEFINED */
                continue;
            }
            else
            {
                INFO(("LOADER: unknown export section %u\n", section));
                return LOADER_ERR_BADFILE;
            }

            if (offset >= sectLen)
            {
                INFO(("LOADER: export offset (%u) out of range\n", offset));
                return LOADER_ERR_BADFILE;
            }
        }
    }

    return 0;
}


/*****************************************************************************/


static Err PrepImportTable(LoaderInfo *li)
{
uint32 numImports;
uint32 i;
uint32 maxOffset;

    numImports = (li->importsSize - sizeof(ELF_Imp3DO) + sizeof(ELF_ImportRec)) / sizeof(ELF_ImportRec);
    if (li->imports->numImports > numImports)
    {
        INFO(("LOADER: import count exceeds number allowed\n"));
        return LOADER_ERR_BADFILE;
    }

    /* make sure the name offsets fall within the section */
    maxOffset = li->importsSize -
                (sizeof(ELF_Imp3DO) - sizeof(ELF_ImportRec)) -
                (li->imports->numImports * sizeof(ELF_ImportRec)) - 1;

    for (i = 0; i < li->imports->numImports; i++)
    {
        if (li->imports->imports[i].nameOffset > maxOffset)
        {
            INFO(("LOADER: import name offset (%u) out of range [0..%u]\n",
                  li->imports->imports[i].nameOffset, maxOffset));
            return LOADER_ERR_BADFILE;
        }
    }

    return 0;
}


/*****************************************************************************/


static Err ReadRelocSections(LoaderFile *file,
                             ELFHeaders *hdrs,
                             LoaderInfo *li)
{
Err         result;
Elf32_Shdr *sectHdr;
Elf32_Phdr *prgHdr;

    result = 0;

    sectHdr = hdrs->eh_TextSectionRela;
    if (!sectHdr)
        sectHdr = hdrs->eh_DataSectionRela;

    if (sectHdr)
    {
        prgHdr = FindProgramHeader(hdrs, sectHdr);
        if (!prgHdr)
            return LOADER_ERR_BADFILE;

        li->relocBufferSize = prgHdr->p_memsz;
        li->relocBuffer     = AllocMem(li->relocBufferSize, MEMTYPE_NORMAL);

        if (!li->relocBuffer)
            return LOADER_ERR_NOMEM;

        sectHdr = hdrs->eh_TextSectionRela;
        if (sectHdr)
        {
            result = SeekLoaderFile(file, sectHdr->sh_offset);
            if (result >= 0)
            {
                if (sectHdr->sh_flags & SHF_COMPRESS)
                    result = ReadLoaderFileCompressed(file, li->relocBuffer, sectHdr->sh_size, li->relocBufferSize);
                else
                    result = ReadLoaderFile(file, li->relocBuffer, sectHdr->sh_size);

                if (result >= 0)
                {
                    li->textRelocs.relocs    = li->relocBuffer;
                    li->textRelocs.numRelocs = result / sizeof(Elf32_Rela);
                    li->dataRelocs.relocs    = (void *)((uint32)li->relocBuffer + result);
                    li->dataRelocs.numRelocs = (li->relocBufferSize - result) / sizeof(Elf32_Rela);
                }
            }
        }
        else
        {
            li->dataRelocs.relocs    = li->relocBuffer;
            li->dataRelocs.numRelocs = li->relocBufferSize / sizeof(Elf32_Rela);
        }

        if (result >= 0)
        {
            sectHdr = hdrs->eh_DataSectionRela;
            if (sectHdr)
            {
                result = SeekLoaderFile(file, sectHdr->sh_offset);
                if (result >= 0)
                {
                    if (sectHdr->sh_flags & SHF_COMPRESS)
                        result = ReadLoaderFileCompressed(file, li->dataRelocs.relocs, sectHdr->sh_size, li->dataRelocs.numRelocs * sizeof(Elf32_Rela));
                    else
                        result = ReadLoaderFile(file, li->dataRelocs.relocs, sectHdr->sh_size);
                }
            }
        }
    }

    if (result >= 0)
    {
        if (hdrs->eh_3DOImportSection)
        {
            result = Read3DOSection(file, hdrs->eh_3DOImportSection,
                                    TYP_3DO_IMPORTS, &li->imports);

            li->importsSize = hdrs->eh_3DOImportSection->sh_size - sizeof(ELF_Note3DO);

            if (result >= 0)
            {
                result = PrepImportTable(li);
                if (result >= 0)
                {
                    if (li->imports)
                    {
                        li->importedModules = AllocMem(sizeof(Item) * li->imports->numImports, MEMTYPE_FILL|0xff);
                        if (!li->importedModules)
                            result = LOADER_ERR_NOMEM;
                    }
                }
            }
        }
    }

    if (result >= 0)
    {
        if (hdrs->eh_3DOExportSection)
        {
            result = Read3DOSection(file, hdrs->eh_3DOExportSection,
                                    TYP_3DO_EXPORTS, &li->exports);

            li->exportsSize = hdrs->eh_3DOExportSection->sh_size - sizeof(ELF_Note3DO);

            if (result >= 0)
                result = PrepExportTable(li);
        }
    }

    return result;
}


/*****************************************************************************/


void UnloadModule(LoaderInfo *li)
{
    if (!li)
        return;

    if (IsSuper())
    {
        SuperFreeMem(li->header3DO,   sizeof(_3DOBinHeader));
        SuperFreeMem(li->codeBase,    li->codeLength);
        SuperFreeMem(li->relocBuffer, li->relocBufferSize);

        if (li->imports)
        {
            SuperFreeMem(li->importedModules, li->imports->numImports * sizeof(Item));
            SuperFreeMem(li->imports,         li->importsSize);
        }

        SuperFreeMem(li->dataBase, li->dataLength + li->bssLength);
        SuperFreeMem(li->exports,  li->exportsSize);

        if (li->path)
            SuperFreeMem(li->path, strlen(li->path) + 1);

        SuperFreeMem(li,           sizeof(LoaderInfo));
    }
    else
    {
        FreeMem(li->header3DO,   sizeof(_3DOBinHeader));
        FreeMem(li->codeBase,    li->codeLength);
        FreeMem(li->relocBuffer, li->relocBufferSize);

        if (li->imports)
        {
            FreeMem(li->importedModules, li->imports->numImports * sizeof(Item));
            FreeMem(li->imports,         li->importsSize);
        }

        if (li->flags & LF_PRIVILEGED)
            FreeMem(li->dataBase, li->dataLength + li->bssLength);
        else
            FreeMemPages(li->dataBase, li->dataLength + li->bssLength);

        FreeMem(li->exports, li->exportsSize);

        if (li->openedfile >= 0)
            CloseFile(li->openedfile);

        if (li->path)
            FreeMem(li->path, strlen(li->path) + 1);

        FreeMem(li,          sizeof(LoaderInfo));
    }
}


/*****************************************************************************/


Err LoadModule(LoaderInfo **resultLI, const char *name,
               bool currentDir, bool systemDir)
{
LoaderInfo *li;
LoaderFile  file;
ELFHeaders  hdrs;
Err         result;
bool        privModule;
bool        signedModule;
uint32      i;

    privModule   = FALSE;
    signedModule = FALSE;
    li           = NULL;

    result = FILE_ERR_NOFILE;

    if (currentDir)
        result = OpenLoaderFile(&file, name);

    if (result == FILE_ERR_NOFILE)
    {
        if (systemDir)
        {
        char path[64 + FILESYSTEM_MAX_NAME_LEN + 1];
        char temp[64];

            sprintf(temp, "System.m2/Modules/%.32s", name);
            result = FindFileAndIdentifyVA(path, sizeof(path), temp,
                                           FILESEARCH_TAG_SEARCH_FILESYSTEMS, DONT_SEARCH_UNBLESSED,
                                           TAG_END);
            if (result >= 0)
                result = OpenLoaderFile(&file, path);
        }
    }

    if (result < 0)
        return result;

    result = ReadElfHeaders(&hdrs, &file);

    if (result >= 0)
    {
        li = *resultLI = AllocMem(sizeof(LoaderInfo), MEMTYPE_FILL);
        if (li == NULL)
            result = LOADER_ERR_NOMEM;
    }

    if (result >= 0)
    {
        if (hdrs.eh_3DOBinHeaderSection == NULL)
            result = LOADER_ERR_BADFILE;
    }

    if (result >= 0)
    {
        result = Read3DOSection(&file, hdrs.eh_3DOBinHeaderSection,
                                TYP_3DO_BINHEADER, &li->header3DO);

        if ((result >= 0) && (li->header3DO == NULL))
            result = LOADER_ERR_BADFILE;
    }

    if (result >= 0)
    {
        privModule   = (li->header3DO->_3DO_Flags & _3DO_PRIVILEGE);
        signedModule = (li->header3DO->_3DO_Flags & _3DO_SIGNED);

        if (signedModule)
        {
            i = strlen(name);
            while (i && (name[i] != '/'))
                i--;

            if (name[i] == '/')
                i++;

            if (strcasecmp(li->header3DO->_3DO_Name, &name[i]))
            {
                /* Someone must be trying to hack a signed module - they
                 * have copied a system executable to a differently named file...
                 */

                result = LOADER_ERR_BADPRIV;
            }
        }
        else if (privModule && CURRENTTASK)
        {
            /* can't be privileged and not signed, unless it's a boot module */
            result = LOADER_ERR_BADPRIV;
        }
    }

    if (result >= 0)
    {
        if (privModule)
            li->flags |= LF_PRIVILEGED;

        if (signedModule)
        {
            if (!IsLoaderFileOnBlessedFS(&file))
            {
                /* only unsigned executables can be run from unblessed FS */
                result = LOADER_ERR_BADPRIV;
            }
        }
        else
        {
            /* Stop dipiring the blocks - not needed for unsigned modules */
            StopRSALoaderFile(&file);
        }
    }

    if (result >= 0)
        result = ReadTextSection(&file, &hdrs, li);

    if (result >= 0)
	result = ReadDataSection(&file, &hdrs, li);

    if (result >= 0)
    {
	li->codeSectNum = hdrs.eh_TextSectionNum;
	li->dataSectNum = hdrs.eh_DataSectionNum;
	li->bssSectNum  = hdrs.eh_BSSSectionNum;

	if (li->codeBase)
	{
	    if (hdrs.eh_ElfHeader.e_entry < li->codeLength)
	    {
                li->entryPoint = (uint32 *) ((uint32) li->codeBase + hdrs.eh_ElfHeader.e_entry);
            }
            else
            {
                INFO(("LOADER: entry point out of range\n"));
            }
        }

        li->directory = -1;
        li->openedfile = -1;

        if ((li->header3DO->_3DO_Flags & _3DO_NO_CHDIR) == 0)
        {
            li->openedfile = result = OpenLoaderFileParent(&file);
            if (li->openedfile >= 0)
            {
                li->directory = ((OFile *)LookupItem(li->openedfile))->ofi_File->fi.n_Item;
            }
        }
    }

#ifdef BUILD_DEBUGGER
    if (result >= 0)
    {
    char path[FILESYSTEM_MAX_PATH_LEN];

        result = GetLoaderFilePath(&file, path, sizeof(path));
        if (path[0])
        {
            li->path = AllocMem(strlen(path) + 1, MEMTYPE_NORMAL);
            if (li->path)
            {
                strcpy(li->path, path);
            }
            else
            {
                result = LOADER_ERR_NOMEM;
            }
        }
    }
#endif

    if (result >= 0)
	result = ReadRelocSections(&file, &hdrs, li);

    if ((result >= 0) && signedModule)
        result = CheckRSALoaderFile(&file);

    CloseLoaderFile(&file);
    FreeElfHeaders(&hdrs);

    if (result < 0)
        UnloadModule(li);

    return result;
}
