/* @(#) loader.c 96/10/08 1.42
 *
 * Routines to manage executable binary images.
 */

#include "kernel/types.h"
#include "loader/elf.h"
#include "loader/elf_3do.h"
#include "loader/loader3do.h"
#include "misc/compression.h"
#include "dipir.h"
#include "insysrom.h"

#define ALIGN4(x)	(((x) + 3) & ~0x03)


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

typedef struct
{
    void   *lf_Buffer;
    uint32  lf_NumBytes;
    uint32  lf_Position;
} LoaderFile;

#define INFO(x)  /* printf x */



/*****************************************************************************/


static void *(*dynAlloc)(void *allocArg, uint32 size);
static int32 (*dynFree)(void *allocArg, void *start, uint32 size);
static void   *dynArg;

/* this is needed by the decompression engine */
void *malloc(uint32 numBytes)
{
uint32 *result;

    result = (* dynAlloc)(dynArg, numBytes + sizeof(uint32));
    if (result)
    {
        memset(result, 0, numBytes);
        result[0] = numBytes;
        result++;
    }

    return result;
}

/* this is needed by the decompression engine */
void free(uint32 *ptr)
{
    (* dynFree)(dynArg, &ptr[-1], ptr[-1]);
}


/*****************************************************************************/


typedef struct Context
{
    uint32 *ctx_Dest;
    uint32 *ctx_Max;
    bool    ctx_Overflow;
} Context;


/*****************************************************************************/


static void PutWord(Context *ctx, uint32 word)
{
    if (ctx->ctx_Dest >= ctx->ctx_Max)
        ctx->ctx_Overflow = TRUE;
    else
        *ctx->ctx_Dest++ = word;
}


/*****************************************************************************/


Err SimpleDecompress(const void *source, uint32 sourceWords,
                     void *result, uint32 resultWords)
{
Decompressor *decomp;
Context       ctx;
Err           err;

    ctx.ctx_Dest     = (uint32 *)result;
    ctx.ctx_Max      = (uint32 *)((uint32)result + resultWords * sizeof(uint32));
    ctx.ctx_Overflow = FALSE;

    err = CreateDecompressor(&decomp,(CompFunc)PutWord,(void *)&ctx);
    if (err >= 0)
    {
        FeedDecompressor(decomp,source,sourceWords);
        err = DeleteDecompressor(decomp);

        if (err == 0)
        {
            if (ctx.ctx_Overflow)
                err = COMP_ERR_OVERFLOW;
            else
                err = ((uint32)ctx.ctx_Dest - (uint32)result) / sizeof(uint32);
        }
    }

    return err;
}


/*****************************************************************************/

void
InitBuffers(void)
{
	if ((*theBootGlobals->bg_VerifyBootAlloc)
			((void *)DIPIRBUFSTART, DIPIRBUFSIZE, BA_DIPIR_SHARED))
		return;

	EPRINTF(("No boot alloc for shared buf (%x,%x)\n",
		DIPIRBUFSTART, DIPIRBUFSIZE));
	HardBoot();
}

/*****************************************************************************/


static void PrepLoaderFile(LoaderFile *file, void *buffer, uint32 numBytes)
{
    file->lf_Buffer   = buffer;
    file->lf_NumBytes = numBytes;
    file->lf_Position = 0;
}


/*****************************************************************************/


static Err ReadLoaderFile(LoaderFile *file, void *buffer, uint32 numBytes)
{
Err result;

    if (numBytes > file->lf_NumBytes - file->lf_Position)
        result = file->lf_NumBytes - file->lf_Position;
    else
        result = numBytes;

    memcpy(buffer, &((uint8 *)file->lf_Buffer)[file->lf_Position], result);

    if (result != numBytes)
        return LOADER_ERR_BADFILE;

    file->lf_Position += numBytes;

    return numBytes;
}


/*****************************************************************************/


Err ReadLoaderFileCompressed(LoaderFile *file, void *buffer,
                             uint32 numCompBytes, uint32 numDecompBytes)
{
void *compBuffer;
Err   result;

    compBuffer = malloc(numCompBytes);
    if (compBuffer == NULL)
        return LOADER_ERR_NOMEM;

    result = ReadLoaderFile(file, compBuffer, numCompBytes);
    if (result >= 0)
    {
        result = SimpleDecompress(compBuffer, numCompBytes / 4,
                                  buffer, numDecompBytes / 4);
        if (result >= 0)
            result *= 4;
    }
    free(compBuffer);

    return result;
}


/*****************************************************************************/


static Err SeekLoaderFile(LoaderFile *file, uint32 newPos)
{
    file->lf_Position = newPos;
    return 0;
}


/*****************************************************************************/


static void FreeElfHeaders(ELFHeaders *hdrs)
{
    DipirFree(hdrs->eh_ProgramHeaders);
    DipirFree(hdrs->eh_SectionHeaders);
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
            hdrs->eh_ProgramHeaders = DipirAlloc(sizeof(Elf32_Phdr) * elfHdr->e_phnum, 0);
            hdrs->eh_SectionHeaders = DipirAlloc(sizeof(Elf32_Shdr) * elfHdr->e_shnum, 0);
            if (hdrs->eh_ProgramHeaders && hdrs->eh_SectionHeaders)
            {
                memset(hdrs->eh_ProgramHeaders, 0, sizeof(Elf32_Phdr) * elfHdr->e_phnum);
                memset(hdrs->eh_SectionHeaders, 0, sizeof(Elf32_Shdr) * elfHdr->e_shnum);

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
            hdrs->eh_BSSSection = NULL;

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
            if  (((sectHdr->sh_type == SHT_NOTE) && (prgHdr->p_type == PT_NOTE))
             ||  ((sectHdr->sh_type == SHT_RELA) && (prgHdr->p_type == PT_NOTE))
             ||  ((sectHdr->sh_type == SHT_PROGBITS) && (prgHdr->p_type == PT_LOAD)))
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
                           LoaderInfo *li,
                           void *(*alloc)(void *allocArg, uint32 size), void *allocArg)
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
    /* FIXME: code should be aligned on a cache line boundary */
    li->codeBase   = (* alloc)(allocArg, li->codeLength);
    PRINTF(("  Alloc code %x,%x\n", li->codeBase, li->codeLength));

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
                           LoaderInfo *li,
                           void *(*alloc)(void *allocArg, uint32 size), void *allocArg)
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

    /* FIXME: should be aligned on a cache line boundary */
    li->dataBase = (* alloc)(allocArg, li->dataLength + li->bssLength);
    PRINTF(("  Alloc data %x,%x\n", li->dataBase, li->dataLength + li->bssLength));

    if (!li->dataBase)
	return LOADER_ERR_NOMEM;

    memset(li->dataBase, 0, li->dataLength + li->bssLength);

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
                          void **sect,
                          void *(*alloc)(void *allocArg, uint32 size), void *allocArg)
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
                            buffer = (* alloc)(allocArg, sectHdr->sh_size - sizeof(ELF_Note3DO));
			    PRINTF(("  Alloc 3DO %x,%x; read %x\n", buffer, sectHdr->sh_size - sizeof(ELF_Note3DO), note.descsz));
                            if (buffer)
                            {
                                result = ReadLoaderFile(file, buffer, note.descsz);
                                if (result >= 0)
                                {
                                    *sect = buffer;
                                }
                                else
                                {
                                    /* FIXME: free buffer */
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
                             LoaderInfo *li,
                             void *(*alloc)(void *allocArg, uint32 size), void *allocArg)
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
        li->relocBuffer     = (* alloc)(allocArg, li->relocBufferSize);
	PRINTF(("  Alloc reloc %x,%x\n", li->relocBuffer, li->relocBufferSize));

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
                                    TYP_3DO_IMPORTS,
                                    &li->imports, alloc, allocArg);

            li->importsSize = hdrs->eh_3DOImportSection->sh_size - sizeof(ELF_Note3DO);

            if (result >= 0)
            {
                result = PrepImportTable(li);
                if (result >= 0)
                {
                    if (li->imports)
                    {
                        li->importedModules = (*alloc)(allocArg, sizeof(Item) * li->imports->numImports);
                        if (!li->importedModules)
                            result = LOADER_ERR_NOMEM;
                        memset(li->importedModules, 0xff, sizeof(Item) * li->imports->numImports);
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
                                    TYP_3DO_EXPORTS,
                                    &li->exports, alloc, allocArg);

            li->exportsSize = hdrs->eh_3DOExportSection->sh_size - sizeof(ELF_Note3DO);

            if (result >= 0)
                result = PrepExportTable(li);
        }
    }

    return result;
}


/*****************************************************************************/


static Err RelocateSection(LoaderInfo *li,
                           void *sectBase,
                           uint32 sectLen,
                           RelocSet *relocSet)
{
Elf32_Rela *relocs;
uint32      i;
uint8       relocType;
uint32      newValue;
void       *pokeAddr;

    relocs = relocSet->relocs;
    for (i = 0; i < relocSet->numRelocs; i++)
    {
        relocType = ELF32_R_TYPE(relocs[i].r_info);
        pokeAddr  = (char *)sectBase + relocs[i].r_offset;

        if (relocs[i].r_offset >= sectLen)
        {
            INFO(("Offset out of range: offset %u, sect len %d\n", relocs[i].r_offset, sectLen));
            return LOADER_ERR_RELOC;
        }

        /* Calculate the section base */
        if (IS_BASEREL_RELOC(relocType))
        {
        uint32 relocsym = ELF32_R_SYM(relocs[i].r_info);
        uint32 pointed_to_sect_base;

            if (relocsym == li->codeSectNum)
                pointed_to_sect_base = (uint32)li->codeBase;
            else if (relocsym == li->dataSectNum)
                pointed_to_sect_base = (uint32)li->dataBase;
            else if (relocsym == li->bssSectNum)
                pointed_to_sect_base = (uint32)li->dataBase + ALIGN4(li->dataLength);

            newValue = pointed_to_sect_base + relocs[i].r_addend;
        }
        else if (IS_IMPREL_RELOC(relocType))
        {
            /* skip imports */
            continue;
        }
        else
        {
            INFO(("Error: reloc %d in module %s is bad (type %d)\n", i, li->header3DO->_3DO_Name, relocType));
            return LOADER_ERR_RELOC;
        }

        /* Our reloc_addr is the sum of:
         *
         *    1.  the value stored in the symbol table (this is an
         *        offset for relocatable objects).
         *    2.  the r_addend field,
         *    3.  the base addr of the section pointed to by the
         *        symbol table entry.
         */

        switch (relocType)
        {
            case R_PPC_IMPADDR32        :
            case R_PPC_BASEREL32        : *(uint32 *)pokeAddr = newValue;
                                          break;

            case R_PPC_IMPADDR16_LO     :
            case R_PPC_BASEREL16_LO     : *(uint16 *)pokeAddr = PPC_LO(newValue);
                                          break;

            case R_PPC_IMPADDR16_HI     :
            case R_PPC_BASEREL16_HI     : *(uint16 *)pokeAddr = PPC_HI(newValue);
                                          break;

            case R_PPC_IMPADDR16_HA     :
            case R_PPC_BASEREL16_HA     : *(uint16 *)pokeAddr = PPC_HA(newValue);
                                          break;

            case R_PPC_IMPREL24         : newValue -= (uint32) pokeAddr;  /* Make relative */
                                          *(uint32 *)pokeAddr = ((*(uint32 *) pokeAddr) & 0xfc000003) | (newValue & ~0xfc000003);

                                          /* Upper 7 bits must be the same (range check) */
                                          newValue &= 0xfe000000;
                                          if ((newValue != 0) && (newValue != 0xfe000000))
                                              return LOADER_ERR_IMPRANGE;

                                          break;

            case R_PPC_IMPREL14_BRTAKEN : /* FIXME: prediction */
            case R_PPC_IMPREL14_BRNTAKEN:

            case R_PPC_IMPREL14         : newValue -= (uint32) pokeAddr;  /* Make relative */
                                          *(uint32 *)pokeAddr = ((*(uint32 *) pokeAddr) & 0xffff0003) | (newValue & ~0xffff0003);

                                          /* Upper 17 bits must be the same (range check) */
                                          newValue &= 0xffff8000;
                                          if ((newValue != 0) && (newValue != 0xffff8000))
                                              return LOADER_ERR_IMPRANGE;

                                          break;

            default                     : INFO(("LOADER: unknown relocation type %u\n", relocType));
                                          return LOADER_ERR_RELOC;
        }
    }

    return 0;
}


/*****************************************************************************/


static struct Module *RelocateDynamic(const Elf32_Ehdr *buffer,
	                              void *(*alloc)(void *allocArg, uint32 size),
                                      int32 (*free)(void *allocArg, void *start, uint32 size),
                                      void *allocArg)
{
LoaderFile      file;
ELFHeaders      hdrs;
Err             result;
LoaderInfo     *li;
Module         *m;

    dynAlloc = alloc;
    dynFree  = free;
    dynArg   = allocArg;

    m = (* alloc)(allocArg, sizeof(Module));
    PRINTF(("RelocateDynamic(%x) module %x\n", buffer, m));

    if (!m)
        return NULL;

    memset(m, 0, sizeof(Module));

    m->li = (* alloc)(allocArg, sizeof(LoaderInfo));
    if (!m->li)
        return NULL;

    memset(m->li, 0, sizeof(LoaderInfo));
    li = m->li;

    /* FIXME: supply correct file size */
    PrepLoaderFile(&file, buffer, 500000);

    result = ReadElfHeaders(&hdrs, &file);
    if (result >= 0)
    {
        result = Read3DOSection(&file, hdrs.eh_3DOBinHeaderSection,
                                TYP_3DO_BINHEADER,
                                &li->header3DO, alloc, allocArg);

        if ((result >= 0) && (li->header3DO == NULL))
            result = LOADER_ERR_BADFILE;
    }

    if (result >= 0)
        result = ReadTextSection(&file, &hdrs, li, alloc, allocArg);

    if (result >= 0)
	result = ReadDataSection(&file, &hdrs, li, alloc, allocArg);

    if (result >= 0)
    {
	li->codeSectNum = hdrs.eh_TextSectionNum;
	li->dataSectNum = hdrs.eh_DataSectionNum;
	li->bssSectNum  = hdrs.eh_BSSSectionNum;
	li->entryPoint  = (uint32 *) ((uint32) li->codeBase + hdrs.eh_ElfHeader.e_entry);
        li->directory   = -1;
    }

    if (result >= 0)
	result = ReadRelocSections(&file, &hdrs, li, alloc, allocArg);

    if (result >= 0)
    {
        result = RelocateSection(li, li->codeBase, li->codeLength,
                                 &li->textRelocs);

        if (result >= 0)
        {
            result = RelocateSection(li, li->dataBase, li->dataLength,
                                     &li->dataRelocs);
        }
    }

    FreeElfHeaders(&hdrs);

    if (result < 0)
    {
        /* FIXME: free memory allocated? */
        return NULL;
    }

    FlushDCacheAll();
    InvalidateICache();

    return m;
}


/*****************************************************************************/


static Elf32_Shdr *
FindSection(const Elf32_Ehdr *buf, uint32 sh_type,
			uint32 sh_flags, uint32 note_type)
{
	Elf32_Shdr *shdr = (Elf32_Shdr *) (((uint8 *) buf) + buf->e_shoff);
	uint32 numsects = buf->e_shnum;

	PRINTF(("FindSection: looking at %d sections\n", numsects));

	while (numsects--)
	{
		/* The diab linker emits bogus sections -
		 * look for real ones by checking this field
		 */
		if (shdr->sh_addralign &&
		    shdr->sh_type == sh_type &&
		    (!sh_flags || shdr->sh_flags == sh_flags))
		{
			if (sh_type != SHT_NOTE)
				return shdr;
			{
			/* Note sections need to check the note type also - ugh! */
			ELF_Note3DO *note = (ELF_Note3DO *)
					(((uint8 *) buf) + shdr->sh_offset);
			if (note->type == note_type)
				return shdr;
			}
		}
		shdr++;
	}

	/* PRINTF(("FindSection: No section found\n")); */
	return NULL;
}


/*****************************************************************************
*/
struct Module *
RelocateBinary(const Elf32_Ehdr *buffer,
	void *(*alloc)(void *allocArg, uint32 size),
	int32 (*free)(void *allocArg, void *start, uint32 size),
	void *allocArg)
{
	Elf32_Ehdr *hdr = buffer;
	Elf32_Shdr *bsssect, *datasect, *textsect,
	           *thdosect, *impsect, *expsect, *reltextsect;
	Elf32_Shdr *sects = (Elf32_Shdr *) (((uint8 *) buffer)
					    + buffer->e_shoff);
	Module *module;
	LoaderInfo *li;
	_3DOBinHeader *newhdr;

	if (alloc)
	    return RelocateDynamic(buffer, alloc, free, allocArg);

	PRINTF(("RelocateBinary: buffer %x\n", buffer));

	if (hdr->e_ident[EI_CLASS] != ELFCLASS32)
	{
		PRINTF(("RelocateBinary: bad magic %x\n",
			hdr->e_ident[EI_CLASS]));
		return NULL;
	}

	datasect = FindSection(hdr, SHT_PROGBITS, SHF_WRITE+SHF_ALLOC, 0);
	bsssect  = FindSection(hdr, SHT_NOBITS, SHF_ALLOC+SHF_WRITE, 0);
	textsect = FindSection(hdr, SHT_PROGBITS, SHF_ALLOC+SHF_EXECINSTR, 0);
	thdosect = FindSection(hdr, SHT_NOTE, 0, TYP_3DO_BINHEADER);
	impsect = FindSection(hdr, SHT_NOTE, 0, TYP_3DO_IMPORTS);
	expsect = FindSection(hdr, SHT_NOTE, 0, TYP_3DO_EXPORTS);
	/*
	 * The text relocs may contain imports which need to be resolved by
	 * other modules.  We can assume the first RELA is the text RELA
	 * because the loader spec requires it.
	 */
	reltextsect = FindSection(hdr, SHT_RELA, 0, 0);

	/*
   	 * The module header - placed in front of the 3DO header/data.
	 * If we don't have a data section, the Bss can be used as the
	 * base of where to copy the 3DO header to
	 */
	module = (Module *) ((datasect ? datasect->sh_addr
			               : bsssect->sh_addr) -
			(sizeof(Module) + sizeof(LoaderInfo) + sizeof(_3DOBinHeader)));
	li   = (LoaderInfo *) (((char *) module) + sizeof(Module));
	newhdr = (_3DOBinHeader *) (((char *) li) + sizeof(LoaderInfo));
	memset(module, 0, sizeof(Module) + sizeof(LoaderInfo) + sizeof(_3DOBinHeader));
	module->li = li;

	/*
	 * CODE
	 */
	PRINTF(("RelocateBinary: move code %x,%x,%x\n",
		textsect->sh_addr,
		((uint8*)buffer) + textsect->sh_offset,
		textsect->sh_size));
	memcpy((void*)textsect->sh_addr,
		((uint8*)buffer) + textsect->sh_offset,
		textsect->sh_size);
	module->li->codeBase = (void *) textsect->sh_addr;
	module->li->codeLength = textsect->sh_size;
	module->li->codeSectNum = textsect - sects;

	/*
	 * DATA
	 */
	if (datasect)
	{
		PRINTF(("                move data %x,%x,%x\n",
			datasect->sh_addr,
			((uint8*)buffer) + datasect->sh_offset,
			datasect->sh_size));
		memcpy((void*)datasect->sh_addr,
			((uint8*)buffer) + datasect->sh_offset,
			datasect->sh_size);
		module->li->dataBase = (void *) datasect->sh_addr;
		module->li->dataLength = datasect->sh_size;
		module->li->dataSectNum = datasect - sects;
	}

	/*
	 * BSS
	 */
	if (bsssect)
	{
		PRINTF(("                clear bss %x,%x\n",
			bsssect->sh_addr, bsssect->sh_size));
		/* bss is always immediately after data */
		memset((uint8 *) bsssect->sh_addr, 0, bsssect->sh_size);

		module->li->bssLength = bsssect->sh_size;
		module->li->bssSectNum = bsssect - sects;
	}

	/*
	 * 3DO header - The 3DO header is placed immediately in front of data.
	 */
	if (thdosect)
	{
		PRINTF(("              move header %x,%x,%x\n",
			newhdr,
			((uint8 *) buffer) + thdosect->sh_offset +
				sizeof(ELF_Note3DO),
			sizeof(_3DOBinHeader)));
		memcpy(newhdr,
			((uint8 *) buffer) + thdosect->sh_offset +
				sizeof(ELF_Note3DO),
		       sizeof(_3DOBinHeader));
		module->n.n_Name = newhdr->_3DO_Name;
		module->n.n_Version = newhdr->_3DO_Item.n_Version;
		module->n.n_Revision = newhdr->_3DO_Item.n_Revision;
		module->li->header3DO = newhdr;
	}

	/*
	 * Exports - we place exports just after the code
	 */
	if (expsect)
	{
		ELF_Exp3DO *newexp = (ELF_Exp3DO *)
			(((uint8 *) module->li->codeBase) +
				module->li->codeLength);
		uint32	numexp = ((ELF_Exp3DO *) (((uint8 *) buffer) +
			expsect->sh_offset))->numExports;

		PRINTF(("              exporting to %x, from %x (%x bytes)\n",
			newexp,
			((uint8 *) buffer) + expsect->sh_offset +
				sizeof(ELF_Note3DO),
			(numexp - 1) * sizeof(uint32) + sizeof(ELF_Exp3DO)));
		memcpy(newexp,
			((uint8 *) buffer) + expsect->sh_offset +
				sizeof(ELF_Note3DO),
			(numexp - 1) * sizeof(uint32) + sizeof(ELF_Exp3DO));
		module->li->exports = newexp;
		/* The exports need to be adjusted to reflect the in memory
		locations
		*/
		PrepExportTable(module->li);
	}

	/*
	 * Text relocations - we place these after the bss section.
	 */
	if (reltextsect)
	{
		Elf32_Rela *newrel = (Elf32_Rela *)
			(((uint8 *) module->li->dataBase) + module->li->dataLength + module->li->bssLength);
		uint32 numrel = reltextsect->sh_size / sizeof(Elf32_Rela);

		PRINTF(("              text rels to %x, from %x (%x bytes)\n",
			newrel, ((uint8 *) buffer) + reltextsect->sh_offset,
			numrel * sizeof(Elf32_Rela)));
		memcpy(newrel, ((uint8 *) buffer) + reltextsect->sh_offset,
			numrel * sizeof(Elf32_Rela));
		module->li->textRelocs.numRelocs = numrel;
		module->li->textRelocs.relocs = newrel;
	}

	/*
	 * Imports - we place these just after the text relocations.
	 */
	if (impsect)
	{
		ELF_Imp3DO *newimp = (ELF_Imp3DO *)
			(((uint8 *) module->li->textRelocs.relocs +
			module->li->textRelocs.numRelocs * sizeof(Elf32_Rela)));
		uint32 impsize = impsect->sh_size - sizeof(ELF_Note3DO);

		PRINTF(("              importing to %x, from %x (%x bytes)\n",
			newimp,
			((uint8 *) buffer) + impsect->sh_offset +
				sizeof(ELF_Note3DO),
			impsize));
		memcpy(newimp,
			((uint8 *) buffer) + impsect->sh_offset +
				sizeof(ELF_Note3DO),
			impsize);
		module->li->imports = newimp;
	}

	/*
	 * Entry Point
	 */
	module->li->entryPoint = (void *) hdr->e_entry;
	PRINTF(("RelocateBinary: entry %x\n", hdr->e_entry));

	FlushDCacheAll();
	InvalidateICache();

	return module;
}

/*****************************************************************************
  Restart to the bootROM or OS or...
*/
void
RestartToXXX(void *entrypoint, void *arg1, void *arg2, void *arg3)
{
	DisableInterrupts();

	/* Set SPRG0 to &bootGlobals since that is what kernel expects */
	SetSPRG0((uint32)theBootGlobals);

	/* Go thru some "glue" code and pass these args to entrypoint */
	PRINTF(("RestartToXXX: branching to %x (%x,%x,%x)\n",
		entrypoint, arg1, arg2, arg3));
	BranchTo(entrypoint, arg1, arg2, arg3);

	/* If we came back here, then reset address was in fact a stub */
	/* provided in a non-ROM debugger based development system.    */
	PRINTF(("RestartToXXX: returned!?\n"));
}

/*****************************************************************************
*/
_3DOBinHeader *
Get3DOBinHeader(const struct Module *buffer)
{
	PRINTF(("Welcome to Get3DOBinHeader\n"));

	return buffer->li->header3DO;
}

/*****************************************************************************
*/
int32
CallBinary(void *entry, void *arg1, void *arg2, void *arg3, void *arg4, void *arg5)
{
	int32 (*func)(void*, void*, void*, void*, void*);

	PRINTF(("CallBinary: %x (%x,%x,%x,%x,%x)\n", entry, arg1, arg2, arg3, arg4, arg5));
	func = entry;
	return (*func)(arg1, arg2, arg3, arg4, arg5);
}

/*****************************************************************************
*/
uint32
GetCurrentOSVersion(void)
{
	return theBootGlobals->bg_KernelVersion;
}
