/* @(#) startup.c 96/10/08 1.14 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/sysinfo.h>
#include <loader/loader3do.h>
#include <loader/elf.h>
#include <loader/elf_3do.h>
#include <kernel/internalf.h>
#include <kernel/super.h>
#include <kernel/kernel.h>
#include <kernel/monitor.h>
#include <dipir/dipirpub.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "reloc.h"
#include "load.h"
#include "daemon.h"


/*****************************************************************************/


#define TRACE(x) /*printf x*/


/*****************************************************************************/


#if 0
void ShowFreeBlocks(const char *banner, Task *t)
{
FreeBlock *fb;
PagePool  *pp;

    printf("%s\n", banner);

    if (t)
        pp = t->t_PagePool;
    else
        pp = KernelBase->kb_PagePool;

    fb = pp->pp_FreeBlocks->fb_Next;
    while (fb)
    {
        printf("  Free memory, range $%08x-$%08x, size %u\n",fb,(uint32)fb + (uint32)fb->fb_Size - 1,fb->fb_Size);
        fb = fb->fb_Next;
    }
}
#endif


/*****************************************************************************/


/* delete all *.init modules */
void internalFreeInitModules(void)
{
LoaderInfo      *li;
Module          *m;
Item             mitem;
OSComponentNode *cn;
uint32           len;
List            *list;
BootAlloc       *ba;
void		(*DeleteBootAlloc)(void *startp, uint32 size, uint32 flags);

    TRACE(("FreeInitModules\n"));

    ScanList(KB_FIELD(kb_OSComponents), cn, OSComponentNode)
    {
        len = strlen(cn->n.n_Name);
        if ((len >= 5) && (strcasecmp(&cn->n.n_Name[len-5], ".init") == 0))
        {
            m = (Module *)FindNamedNode(&KB_FIELD(kb_Modules), cn->n.n_Name);
            if (m)
            {
                mitem = m->n.n_Item;
                li    = m->li;

                externalSetItemOwner(mitem, KB_FIELD(kb_OperatorTask));
                internalDeleteItem(mitem);
                UnloadModule(li);
            }
        }
    }

    /* stop looking in the component list when opening files */
    KB_FIELD(kb_OSComponents) = NULL;

    TRACE(("FreeInitModules: freeing premultitask memory\n"));

    /* free the memory used to hold the component list */
    QUERY_SYS_INFO(SYSINFO_TAG_BOOTALLOCLIST, list);
    QUERY_SYS_INFO(SYSINFO_TAG_DELETEBOOTALLOC, DeleteBootAlloc);
Rescan:
    ScanList(list, ba, BootAlloc)
    {
	if (ba->ba_Flags & BA_PREMULTITASK)
	{
	    (*DeleteBootAlloc)(ba->ba_Start, ba->ba_Size, ba->ba_Flags);
	    SuperFreeRawMem(ba->ba_Start, ba->ba_Size);
	    goto Rescan;
	}
    }

    TRACE(("FreeInitModules: returning\n"));
}


/*****************************************************************************/


Err internalRegisterOperator(void)
{
Module *m;

    TRACE(("RegisterOperator\n"));

    if (!IsPriv(CURRENTTASK))
        return LOADER_ERR_BADPRIV;

    /* let the operator write to the boot module memory */
    ScanList(&KB_FIELD(kb_Modules), m, Module)
    {
	ControlPagePool(KB_FIELD(kb_PagePool),
                        m->li->dataBase, m->li->dataLength + m->li->bssLength,
                        MEMC_OKWRITE, KB_FIELD(kb_OperatorTask));
    }

    StartLoaderDaemon();

    return 0;
}


/*****************************************************************************/


/* Relocate the base OS components. */
static void RelocateOSComponents(void)
{
Module  *m;
void   (*DeleteBootAlloc)(void *startp, uint32 size, uint32 flags);

    ScanList(&KB_FIELD(kb_Modules), m, Module)
    {
        TRACE(("RelocateOSComponents: relocating %s\n", m->n.n_Name));
        InitImportedModules(m->li, -1);
        RelocateModule(m->li, strcasecmp(m->n.n_Name, "kernel") != 0);
    }

    /* the kernel's all relocated now, nuke its relocation table */
    m = (Module *)FindNamedNode(&KB_FIELD(kb_Modules), "kernel");
    QUERY_SYS_INFO(SYSINFO_TAG_DELETEBOOTALLOC, DeleteBootAlloc);
    (*DeleteBootAlloc)(m->li->relocBuffer, (m->li->relocBufferSize + 7) & ~7, BA_OSDATA);

    SuperFreeRawMem(m->li->relocBuffer, m->li->relocBufferSize);
    m->li->relocBuffer     = NULL;
    m->li->relocBufferSize = 0;
}


/*****************************************************************************/


/* Given the list of memory-based OS components supplied by dipir, scan
 * the list and peek into the executable images to find the names of
 * all the components.
 */
static void NameOSComponents(void)
{
Elf32_Ehdr      *elfHdr;
Elf32_Shdr      *sectHdr;
uint32           i;
OSComponentNode *cn;
_3DOBinHeader   *binHdr;

    ScanList(KB_FIELD(kb_OSComponents), cn, OSComponentNode)
    {
        elfHdr  = cn->cn_Addr;
        sectHdr = (void *)((uint32)cn->cn_Addr + elfHdr->e_shoff);
        for (i = 0; i < elfHdr->e_shnum; i++)
        {
            if (sectHdr->sh_type == SHT_NOTE)
            {
                binHdr = (void *)((uint32)cn->cn_Addr + sectHdr->sh_offset + sizeof(ELF_Note3DO));
                cn->n.n_Name = binHdr->_3DO_Name;
                break;
            }
            sectHdr = (Elf32_Shdr *)((uint32)sectHdr + elfHdr->e_shentsize);
        }
    }
}


/*****************************************************************************/


/* Create the kernel's Module item. The kernelAddr parameter points to
 * a template Module item allocated by dipir
 */
static Err CreateKernelModule(Module *m, void *kernelAddr)
{
Module *template;

    template                      = kernelAddr;
    m->li                         = template->li;
    m->li->importedModules        = AllocMem(sizeof(Item) * m->li->imports->numImports, MEMTYPE_FILL | 0xff);
    m->li->header3DO->_3DO_Flags |= _3DO_SIGNED;
    m->n.n_Name                   = m->li->header3DO->_3DO_Name;
    m->n.n_ItemFlags             |= ITEMNODE_PRIVILEGED;
    m->n.n_Version                = m->li->header3DO->_3DO_Item.n_Version;
    m->n.n_Revision               = m->li->header3DO->_3DO_Item.n_Revision;

    AddTail(&KB_FIELD(kb_Modules), (Node *) m);
    internalPrint3DOHeader(m->n.n_Item, NULL, NULL);

#ifdef BUILD_DEBUGGER
    Dbgr_ModuleCreated(m);
#endif

    return 0;
}


/*****************************************************************************/


/* This function moves the boot modules out of the dipir list and
 * into the main module list.
 */
void CreateBootModules(void)
{
Module          *module;
TagArg           tags[2];
OSComponentNode *cn;
Err              result;

    TRACE(("CreateBootModules: calling NameOSComponents()\n"));
    NameOSComponents();

    ScanList(KB_FIELD(kb_OSComponents), cn, OSComponentNode)
    {
        TRACE(("CreateBootModules: creating module item for %s\n", cn->n.n_Name));

        module = (Module *) AllocateNode((Folio *) &KB, MODULENODE);
        if (module)
	{
            if (cn == (OSComponentNode *)FirstNode(KB_FIELD(kb_OSComponents)))
            {
                result = CreateKernelModule(module, KB_FIELD(kb_KernelModule));
            }
            else
            {
                result = LoadModule(&module->li, cn->n.n_Name, TRUE, FALSE);
                if (result >= 0)
                {
                    module->li->header3DO->_3DO_Flags |= _3DO_SIGNED;
                    module->li->flags                 |= LF_PRIVILEGED;

                    tags[0].ta_Tag = TAG_ITEM_NAME;
                    tags[0].ta_Arg = cn->n.n_Name;
                    tags[1].ta_Tag = TAG_END;
                    result = internalCreateModule(module, tags);

                    module->n.n_OpenCount = 1;
                }
            }
        }
        else
        {
            result = NOMEM;
        }

        if (result < 0)
        {
#ifdef BUILD_STRINGS
            printf("Could not create module %s (err %x)\n", cn->n.n_Name, result);
#endif
        }
    }

    TRACE(("CreateBootModules: calling RelocateOSComponents()\n"));
    RelocateOSComponents();

    TRACE(("CreateBootModules: returning\n"));
}
