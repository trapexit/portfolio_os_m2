/* @(#) api.c 96/10/15 1.33 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/nodes.h>
#include <loader/loader3do.h>
#include <loader/elf_3do.h>
#include <kernel/internalf.h>
#include <kernel/listmacros.h>
#include <file/filefunctions.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "daemon.h"


/*****************************************************************************/


Item externalImportByName(Item module, const char *name)
{
    return AskDaemon(DAEMON_IMPORTBYNAME, module, name);
}


/*****************************************************************************/


Err externalUnimportByName(Item module, const char *name)
{
    return AskDaemon(DAEMON_UNIMPORTBYNAME, module, name);
}


/*****************************************************************************/


Item externalOpenModule(const char *path, OpenModuleTypes type, const TagArg *tags)
{
    Item fileItem;
    Item result;

    TOUCH(type);
    fileItem = OpenFile(path);
    if (fileItem < 0)
	return fileItem;
    result = AskDaemon(DAEMON_OPENMODULE, (uint32)path, tags);
    CloseFile(fileItem);
    return result;
}


/*****************************************************************************/


Item externalOpenModuleTree(Item module, Item clientTask)
{
    return AskDaemon(DAEMON_OPENMODULETREE, (uint32)module, (void *)clientTask);
}


/*****************************************************************************/


Err externalCloseModule(Item module)
{
    return AskDaemon(DAEMON_CLOSEMODULE, module, NULL);
}


/*****************************************************************************/


typedef int32 (* EntryFunc)(int32 argc, char **argv, Item module);

int32 ExecuteModule(Item module, uint32 argc, char **argv)
{
EntryFunc entry;
Module   *mod;

    mod = (Module *)CheckItem(module, KERNELNODE, MODULENODE);
    if (!mod || !mod->li->entryPoint)
        return LOADER_ERR_BADITEM;

    if (CURRENTTASK->t.n_ItemFlags & ITEMNODE_PRIVILEGED)
    {
        /* If the current task is privileged, the code being run must be
         * signed
         */
        if ((mod->li->header3DO->_3DO_Flags & _3DO_SIGNED) == 0)
            return LOADER_ERR_BADPRIV;
    }

    entry = (EntryFunc)mod->li->entryPoint;

    return (* entry)(argc, argv, module);
}


/*****************************************************************************/


Item FindModuleByAddress(const void *addr)
{
Module *m;

    ScanList(&KB_FIELD(kb_Modules), m, Module)
    {
	if ((addr >= m->li->codeBase)
         && (addr < (uint8 *) m->li->codeBase + m->li->codeLength))
	{
            return m->n.n_Item;
        }

	if ((addr >= m->li->dataBase)
	 && (addr < (uint8 *) m->li->dataBase + m->li->dataLength + m->li->bssLength))
	{
            return m->n.n_Item;
	}
    }

    return LOADER_ERR_NOTFOUND;
}
