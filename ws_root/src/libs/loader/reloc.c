/* @(#) reloc.c 96/10/08 1.3 */

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
#include "daemon.h"
#include "reloc.h"


/*****************************************************************************/


#ifdef BUILD_STRINGS
#define INFO(x) printf x
#else
#define INFO(x)
#endif


#define ALIGN4(x)	(((x) + 3) & ~0x03)


/*****************************************************************************/


static Err RelocateSection(LoaderInfo *li,
                           void *sectBase,
                           uint32 sectLen,
                           RelocSet *relocSet)
{
Elf32_Rela *relocs;
ELF_Imp3DO *imports;
uint32      i;
uint8       relocType;
uint32      newValue;
void       *pokeAddr;

    imports = li->imports;
    relocs  = relocSet->relocs;

    for (i = 0; i < relocSet->numRelocs; i++)
    {
        relocType = ELF32_R_TYPE(relocs[i].r_info);
        pokeAddr  = (char *)sectBase + relocs[i].r_offset;

        if (relocs[i].r_offset >= sectLen)
        {
            INFO(("LOADER: relocation offset out of range (offset %u, sect len %d)\n", relocs[i].r_offset, sectLen));
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
        uint16      importIndex = ELF3DO_EXPORT_INDEX(relocs[i].r_info);
        uint8       modIndex    = ELF3DO_MODULE_INDEX(relocs[i].r_info);
        ELF_Exp3DO *exports;

            if ((imports == NULL) || (modIndex >= imports->numImports))
                return LOADER_ERR_NOIMPORT;

            if (li->importedModules[modIndex] < 0)
            {
                /* skip this one for now */
                continue;
            }

            exports = MODULE(li->importedModules[modIndex])->li->exports;
            if (importIndex >= exports->numExports)
                return LOADER_ERR_NOIMPORT;

            newValue = exports->exportWords[importIndex] + relocs[i].r_addend;
        }
        else
        {
            INFO(("LOADER: reloc %d in module %s is bad (type %d)\n", i, li->header3DO->_3DO_Name, relocType));
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


/* Squeeze all useful reloc entries together */
static void PackRelocs(LoaderInfo *li,
                       RelocSet *relocSet, Elf32_Rela *dstRelocs)
{
uint32      result;
Elf32_Rela *relocs;
ELF_Imp3DO *imports;
uint32      relocType;
uint32      moduleIndex;
uint32      i;

    imports = li->imports;
    result  = 0;
    relocs  = relocSet->relocs;

    /* We walk the reloc table and only keep relocs which are unresolved or
     * the ones that must be preserved because the associated module may be
     * unloaded and later reloaded.
     */
    for (i = 0; i < relocSet->numRelocs; i++)
    {
	relocType = ELF32_R_TYPE(relocs[i].r_info);

	if (IS_IMPREL_RELOC(relocType))
        {
	    moduleIndex = ELF3DO_MODULE_INDEX(relocs[i].r_info);

	    if ((li->importedModules[moduleIndex] < 0) ||
	       (imports->imports[moduleIndex].flags & REIMPORT_ALLOWED))
            {
		/* If we haven't loaded an imported module or the
		 * module may be unloaded - we need to keep the relocs around
		 */
		dstRelocs[result++] = relocs[i];
            }
	}
    }

    relocSet->numRelocs = result;
}


/*****************************************************************************/


/* This function attempts to reduce the size of the text and data relocation
 * tables for the current module. This is done by removing any relocation
 * entry that has been applied, and by compacting the unused relocs together.
 */
static void MinimizeRelocs(LoaderInfo *li)
{
uint32  newSize;
void   *buf;

    PackRelocs(li, &li->textRelocs, li->relocBuffer);
    PackRelocs(li, &li->dataRelocs, (void *)((uint32)li->relocBuffer + li->textRelocs.numRelocs * sizeof(Elf32_Rela)));

    newSize = (li->textRelocs.numRelocs + li->dataRelocs.numRelocs)
            * sizeof(Elf32_Rela);

    if (newSize != li->relocBufferSize)
    {
        if (newSize == 0)
        {
            /* no relocs left at all! */

            FreeMem(li->relocBuffer, li->relocBufferSize);
            buf = NULL;
        }
        else
        {
            if (CURRENTTASK)
            {
                buf = ReallocMem(li->relocBuffer, li->relocBufferSize,
                                 newSize, MEMTYPE_NORMAL);
            }
            else
            {
                /* don't do this for boot modules */
                buf = NULL;
            }

            if (!buf)
            {
                /* couldn't reallocate, just keep using the same buffer */
                buf     = li->relocBuffer;
                newSize = li->relocBufferSize;
            }
        }

        li->relocBuffer       = buf;
        li->relocBufferSize   = newSize;
        li->textRelocs.relocs = buf;
        li->dataRelocs.relocs = (void *)((uint32)li->relocBuffer + li->textRelocs.numRelocs * sizeof(Elf32_Rela));
    }
}


/*****************************************************************************/


Err RelocateModule(LoaderInfo *li, bool minimizeRelocs)
{
Err result;

#if 0
    {
    Module *m;
    uint32  i;

        m = MODULE(li->li_Item);
        if (m)
            printf("Relocating module %s\n", m->n.n_Name);

        if (li->imports)
        {
            for (i = 0; i < li->imports->numImports; i++)
            {
                if (li->importedModules[i] >= 0)
                {
                    printf("  index %d is set to ", i);
                    printf("module 0x%x (", li->importedModules[i]);

                    m = MODULE(li->importedModules[i]);
                    if (m)
                        printf("%s)\n", m->n.n_Name);
                    else
                        printf("bad item number)\n");
                }
                else
                {
                    printf("  index %d is NULL\n", i);
                }
            }
        }
    }
#endif

    result = RelocateSection(li, li->codeBase, li->codeLength, &li->textRelocs);
    if (result >= 0)
    {
        result = RelocateSection(li, li->dataBase, li->dataLength, &li->dataRelocs);
        if (result >= 0)
        {
            FlushDCacheAll(0);
            InvalidateICache();

            if (minimizeRelocs)
                MinimizeRelocs(li);
        }
    }

    return result;
}
