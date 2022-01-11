#ifndef __LOADER_LOADER3DO_H
#define __LOADER_LOADER3DO_H


/******************************************************************************
**
**  @(#) loader3do.h 96/11/06 1.59
**
******************************************************************************/


#ifndef __KERNEL_TASK_H
#include <kernel/task.h>
#endif

#ifndef __KERNEL_ITEM_H
#include <kernel/item.h>
#endif

#ifndef __LOADER_LOADERERR_H
#include <loader/loadererr.h>
#endif

#ifndef EXTERNAL_RELEASE
#ifndef __LOADER_HEADER3DO_H
#include <loader/header3do.h>
#endif

#ifndef __LOADER_ELF_H
#include <loader/elf.h>
#endif

#ifndef __LOADER_ELF_3DO_H
#include <loader/elf_3do.h>
#endif
#endif

/*****************************************************************************/

#ifndef EXTERNAL_RELEASE
typedef struct
{
    uint32        numRelocs;
    Elf32_Rela   *relocs;
} RelocSet;

typedef struct LoaderInfo
{
    MinNode        li;
    Item           li_Item;
    uint32         flags;
    uint32        *entryPoint;
    _3DOBinHeader *header3DO;
    Item          *importedModules;

    ELF_Imp3DO    *imports;
    uint32         importsSize;

    ELF_Exp3DO    *exports;
    uint32         exportsSize;

    RelocSet       textRelocs;
    RelocSet       dataRelocs;

    /* this buffer holds both text and data relocs. The above textRelocs and
     * dataRelocs fields hold pointers into this buffer.
     */
    void          *relocBuffer;
    uint32         relocBufferSize;

    void          *codeBase;    /* address of code              */
    void          *dataBase;    /* address of data, bss follows */
    uint32         codeLength;  /* # bytes of code              */
    uint32         dataLength;  /* # bytes of data              */
    uint32         bssLength;   /* # bytes of bss               */

    Item           directory;
    Item           openedfile;

    /* ELF section numbers, needed when applying IMPREL relocs */
    uint32         codeSectNum;
    uint32         dataSectNum;
    uint32         bssSectNum;

    char          *path;
} LoaderInfo;

/* for LoaderInfo.flags */
#define LF_PRIVILEGED   0x00000001      /* This module is privileged        */
#define LF_CANTEXPORT   0x00000002      /* This module can't export symbols */


typedef struct Module
{
    OpeningItemNode  n;
    LoaderInfo      *li;
} Module;

#define MODULE(m) ((Module *)LookupItem(m))

typedef enum ModuleTags
{
    MODULE_TAG_MUST_BE_SIGNED,
    /* If set to one, module must be signed */

    MODULE_TAG_MUST_BE_PRIVILEGED
    /* If set to one, the module must be privleged */
} ModuleTags;

#endif /* EXTERNAL_RELEASE */

typedef enum OpenModuleTypes
{
    OPENMODULE_FOR_TASK,    /* use new pages for data+bss            */
    OPENMODULE_FOR_THREAD   /* use current task's pages for data+bss */
} OpenModuleTypes;


/*****************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif /* cplusplus 	*/


Item   OpenModule(const char *path, OpenModuleTypes type, const TagArg *tags);
Err    CloseModule(Item module);
int32  ExecuteModule(Item module, uint32 argc, char **argv);
Item   FindCurrentModule(void);
Item   ImportByName(Item module, const char *name);
Err    UnimportByName(Item module, const char *name);

#ifndef EXTERNAL_RELEASE
Item  internalCreateModule(Module *m, TagArg *args);
Item  internalCreateModuleVA(Module *m, uint32 args, ...);
int32 internalDeleteModule(Module *m, Task *t);
Item  internalOpenModule(struct Module *m, void *a, Task *t);
Err   internalCloseModule(struct Module *m, Task *t);
Item  externalImportByName(Item module, const char *name);
Err   externalUnimportByName(Item module, const char *name);
Item  externalOpenModuleTree(Item module, Item task);
Item  externalOpenModule(const char *path, OpenModuleTypes type, const TagArg *tags);
Err   externalCloseModule(Item module);
Err   RegisterOperator(void);
void  FreeInitModules(void);
void  CreateBootModules(void);
#endif	/* EXTERNAL_RELEASE */

#ifdef __cplusplus
}
#endif  /* __cplusplus */


/*****************************************************************************/


#endif /* __LOADER_LOADER3DO_H */
