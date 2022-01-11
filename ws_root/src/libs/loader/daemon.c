/* @(#) daemon.c 96/11/20 1.11 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/msgport.h>
#include <kernel/kernel.h>
#include <loader/loader3do.h>
#include <loader/elf_3do.h>
#include <file/filefunctions.h>
#include <string.h>
#include <stdio.h>
#include "load.h"
#include "reloc.h"
#include "modules.h"
#include "daemon.h"


/*****************************************************************************/


#define TRACE(x) /*printf x*/


/*****************************************************************************/


typedef struct
{
    DaemonOp dp_Op;
    Item     dp_Client;
    uint32   dp_Arg1;
    void    *dp_Arg2;
} DaemonPacket;

typedef struct
{
    MinNode ai;
    Item    ai_Module;
    bool    ai_Scanned;
} AutoImport;

typedef int32 (* CallBackFunc)(uint32 op, Item task, Item mod);


/*****************************************************************************/


static Item  daemon;
static Item  daemonPort;
static int32 daemonSig;


/*****************************************************************************/


static char *GetImportName(LoaderInfo *li, uint32 impIndex)
{
ELF_Imp3DO *imports;

    imports = li->imports;
    return (char *)&imports->imports[imports->numImports] + imports->imports[impIndex].nameOffset;
}

/*****************************************************************************/


/* Look in the module's import table for an import of a given name and return
 * its index.
 */
static Err FindImportIndex(LoaderInfo *li, const char *impName)
{
uint32 impIndex;

    if (li->imports)
    {
        for (impIndex = 0; impIndex < li->imports->numImports; impIndex++)
        {
            if (strcasecmp(GetImportName(li, impIndex), impName) == 0)
                return impIndex;
        }
    }

    return LOADER_ERR_NOTFOUND;
}


/*****************************************************************************/


static bool GoodFit(LoaderInfo *importer, LoaderInfo *exporter, uint32 impIndex,
                    bool checkSig)
{
char       *name;
uint32      id;
uint8       version;
uint8       revision;
bool        rsa;

    name     = GetImportName(importer, impIndex);
    id       = importer->imports->imports[impIndex].libraryCode;
    version  = importer->imports->imports[impIndex].libraryVersion;
    revision = importer->imports->imports[impIndex].libraryRevision;

    if (checkSig && (importer->header3DO->_3DO_Flags & _3DO_SIGNED))
        rsa = TRUE;
    else
        rsa = FALSE;

    if ((exporter != importer)
     && ((exporter->flags & LF_CANTEXPORT) == 0)
     && (exporter->exports)
     && (strcasecmp(exporter->header3DO->_3DO_Name, name) == 0)
     && (exporter->exports->libraryID == id)
     && (!rsa || (exporter->header3DO->_3DO_Flags & _3DO_SIGNED))
     && (exporter->header3DO->_3DO_Item.n_Version >= version)
     && ((exporter->header3DO->_3DO_Item.n_Version > version) || (exporter->header3DO->_3DO_Item.n_Revision >= revision)))
    {
        return TRUE;
    }

    return FALSE;
}


/*****************************************************************************/


static Item FindImport(LoaderInfo *importer, uint32 impIndex, bool checkSig)
{
Module *m;

    ScanList(&KB_FIELD(kb_Modules), m, Module)
    {
        if (GoodFit(importer, m->li, impIndex, checkSig))
            return m->n.n_Item;
    }

    return LOADER_ERR_NOTFOUND;
}


/*****************************************************************************/


void InitImportedModules(LoaderInfo *li, Item clientTask)
{
ELF_Imp3DO *imports;
Item       *importedModules;
uint32      impIndex;
Item        module;
char       *name;

    imports         = li->imports;
    importedModules = li->importedModules;
    if (imports)
    {
        for (impIndex = 0; impIndex < imports->numImports; impIndex++)
        {
            importedModules[impIndex] = -1;

            module = FindImport(li, impIndex, TRUE);
            if (module >= 0)
            {
                name = GetImportName(li, impIndex);
                if ((imports->imports[impIndex].flags & IMPORT_NOW)
                 || (strcasecmp(name, "kernel") == 0)
                 || (strcasecmp(name, "filesystem") == 0)
                 || (IsItemOpened(clientTask, module) >= 0))
                {
                    importedModules[impIndex] = module;
                }
            }
        }
    }
}


/*****************************************************************************/


static void DeleteAutoImportList(List *l)
{
AutoImport *ai;

    while (ai = (AutoImport *)RemHead(l))
        FreeMem(ai, sizeof(AutoImport));
}


/*****************************************************************************/


static Err CreateAutoImportList(Item root, List *l)
{
AutoImport *ai;
AutoImport *ai2;
uint32      impIndex;
bool        found;
bool        new;
Err         result;
LoaderInfo *li;
Item       *importedModules;

    PrepList(l);

    ai = AllocMem(sizeof(AutoImport), MEMTYPE_NORMAL);
    if (!ai)
        return LOADER_ERR_NOMEM;

    ai->ai_Module  = root;
    ai->ai_Scanned = FALSE;
    AddTail(l, (Node *)ai);

    new    = TRUE;
    result = 0;
    while (new && (result >= 0))
    {
        new = FALSE;

        /* for all modules in the list, add their imports to the list and
         * recurse
         */
        ScanList(l, ai, AutoImport)
        {
            if (ai->ai_Scanned == FALSE)
            {
                li              = MODULE(ai->ai_Module)->li;
                importedModules = li->importedModules;
                if (li->imports)
                {
                    for (impIndex = 0; impIndex < li->imports->numImports; impIndex++)
                    {
                        if (li->imports->imports[impIndex].flags & IMPORT_NOW)
                        {
                            /* This check is needed in order to deal with
                             * the only cases of auto import modules that
                             * get deleted when clients are still around:
                             * the .init modules.
                             */
                            if (MODULE(importedModules[impIndex]) == NULL)
                            {
                            }
                            else
                            {
                                /* See if this import is already in the list
                                 * we are scanning. If not, add it.
                                 */

                                found = FALSE;
                                ScanList(l, ai2, AutoImport)
                                {
                                    if (ai2->ai_Module == importedModules[impIndex])
                                    {
                                        found = TRUE;
                                        break;
                                    }
                                }

                                if (!found)
                                {
                                    ai2 = AllocMem(sizeof(AutoImport), MEMTYPE_NORMAL);
                                    if (ai2)
                                    {
                                        ai2->ai_Module  = importedModules[impIndex];
                                        ai2->ai_Scanned = FALSE;
                                        AddTail(l, (Node *)ai2);
                                        new = TRUE;
                                        continue;
                                    }

                                    DeleteAutoImportList(l);
                                    return LOADER_ERR_NOMEM;
                                }
                            }
                        }
                    }
                }
                ai->ai_Scanned = TRUE;
            }
        }
    }

    return 0;
}


/*****************************************************************************/


/* Open all automatic imports of the supplied module, and recursively open all
 * automatic imports they have.
 */
static Item OpenModuleTree(Item root, Item clientTask)
{
List        l;
AutoImport *ai;
AutoImport *ai2;
Err         result;

    TRACE(("OpenModuleTree: entering with clientTask %s, root %s\n", TASK(clientTask)->t.n_Name,
           MODULE(root)->n.n_Name));

    result = CreateAutoImportList(root, &l);
    if (result >= 0)
    {
        ScanList(&l, ai, AutoImport)
        {
            result = OpenItemAsTask(ai->ai_Module, NULL, clientTask);
            if (result < 0)
            {
                ScanList(&l, ai2, AutoImport)
                {
                    if (ai2 == ai)
                        break;

                    CloseItemAsTask(ai2->ai_Module, clientTask);
                }
                break;
            }
        }

        if (result >= 0)
            result = root;

        DeleteAutoImportList(&l);
    }

    return result;
}


/*****************************************************************************/


/* Close all automatic imports of the supplied modules, and recursively close
 * all automatic imports they have.
 */
static Err CloseModuleTree(Item clientTask, Item root)
{
List        l;
AutoImport *ai;
Err         result;

    result = CreateAutoImportList(root, &l);
    if (result >= 0)
    {
        ScanList(&l, ai, AutoImport)
        {
            result = CloseItemAsTask(ai->ai_Module, clientTask);
            if (result < 0)
                break;
        }

        DeleteAutoImportList(&l);
    }

    return result;
}


/*****************************************************************************/


/* Loads a named module into memory and brings in all modules it depends on.
 * This returns the item number of the main module of interest. Items are
 * created for all new modules, all relocations are resolved, but no opens are
 * done on anything.
 */
static Item DaemonLoad(const char *name, bool systemDir)
{
Item        result;
ELF_Imp3DO *imports;
uint32      impIndex;
LoaderInfo *li;
LoaderInfo *li2;
LoaderInfo *main;
bool        new;
bool        currentDir;
List        loadedModules;

    TRACE(("DaemonLoad: entering with %s\n", name));

    result = LoadModule(&main, name, TRUE, systemDir);
    if (result < 0)
        return result;

    PrepList(&loadedModules);
    AddTail(&loadedModules, (Node *)main);

    new    = TRUE;
    result = 0;
    while (new && (result >= 0))
    {
        new = FALSE;

        /* for all loaded modules, load needed imports */
        ScanList(&loadedModules, li, LoaderInfo)
        {
            imports = li->imports;
            if (imports)
            {
                for (impIndex = 0; impIndex < imports->numImports; impIndex++)
                {
                    if (imports->imports[impIndex].flags & IMPORT_NOW)
                    {
                        result = FindImport(li, impIndex, TRUE);
                        if (result < 0)
                        {
                            /* don't load it if it's already in the
                             * loadedModules list and it matches the binding
                             * requirements.
                             */
                            ScanList(&loadedModules, li2, LoaderInfo)
                            {
                                if (GoodFit(li, li2, impIndex, TRUE))
                                {
                                    result = 0;
                                    break;
                                }
                            }

                            if (result < 0)
                            {
                                currentDir = TRUE;
                                if (strcasecmp(GetImportName(li, impIndex), li->header3DO->_3DO_Name) == 0)
                                {
                                    /* if the import has the same name as
                                     * the importer, don't look in the current
                                     * directory for the file, otherwise the
                                     * importer would end up getting reloaded.
                                     */
                                    currentDir = FALSE;
                                }

                                result = LoadModule(&li2, GetImportName(li, impIndex), currentDir, TRUE);
                                if (result >= 0)
                                {
                                    AddTail(&loadedModules, (Node *)li2);
                                    if (!GoodFit(li, li2, impIndex, TRUE))
                                    {
                                        result = LOADER_ERR_BADFILE;
                                    }
                                    else
                                    {
                                        new = TRUE;
                                    }
                                }
                            }
                        }
                    }
                }

                if (new || (result < 0))
                    break;
            }
        }
    }

    if (result >= 0)
    {
        /* create items for all the loaded modules */
        ScanList(&loadedModules, li, LoaderInfo)
        {
            li->li_Item = result = CreateItemVA(MKNODEID(KERNELNODE, MODULENODE),
                                         CREATEMODULE_TAG_LOADERINFO, li,
                                         TAG_ITEM_NAME,               li->header3DO->_3DO_Name,
                                         TAG_END);
            if (li->li_Item < 0)
                break;
        }
    }

#ifdef BUILD_DEBUGGER
    if (result >= 0)
    {
        /* a module's path string is only needed until the module is created and
         * the path has been sent to the debugger...
         */
        ScanList(&loadedModules, li, LoaderInfo)
        {
            if (li->path)
            {
                FreeMem(li->path, strlen(li->path) + 1);
                li->path = NULL;
            }
        }
    }
#endif

    if (result >= 0)
    {
        /* all required imports are now in memory and have items */

        /* apply relocations for all the loaded modules */
        ScanList(&loadedModules, li, LoaderInfo)
        {
            InitImportedModules(li, -1);

            result = RelocateModule(li, TRUE);
            if (result < 0)
               break;
        }
    }

    if (result >= 0)
    {
        /* now give a chance to the modules themselves to init */
        ScanList(&loadedModules, li, LoaderInfo)
        {
            if ((li->header3DO->_3DO_Flags & _3DO_MODULE_CALLBACKS)
             && (li->header3DO->_3DO_Flags & _3DO_PRIVILEGE)
             && (li->entryPoint))
            {
                TRACE(("Calling %s with DEMANDLOAD_MAIN_CREATE\n", li->header3DO->_3DO_Name));

                result = ((CallBackFunc)li->entryPoint)(DEMANDLOAD_MAIN_CREATE, daemon, li->li_Item);

                TRACE(("        returned with %x\n", result));

                if (result < 0)
                {
                    /* for all modules we've already told
                     * DEMANDLOAD_MAIN_CREATE, now tell them
                     * DEMANDLOAD_MAIN_DELETE
                     */
                    ScanList(&loadedModules, li2, LoaderInfo)
                    {
                        if (li == li2)
                            break;

                        if ((li2->header3DO->_3DO_Flags & _3DO_MODULE_CALLBACKS)
                         && (li2->header3DO->_3DO_Flags & _3DO_PRIVILEGE)
                         && (li2->entryPoint))
                        {
                            ((CallBackFunc)li2->entryPoint)(DEMANDLOAD_MAIN_DELETE, daemon, li2->li_Item);
                        }
                    }

                    break;
                }
            }
        }
    }

    if (result >= 0)
    {
        /* everything worked, return the item number of the primary module... */
        return main->li_Item;
    }

    /* didn't go as we planned, cleanup and fail */
    while (li = (LoaderInfo *)RemHead(&loadedModules))
    {
        DeleteItem(li->li_Item);
        UnloadModule(li);
    }
    return result;
}


/*****************************************************************************/


static Err DaemonUnload(Item module)
{
Err         result;
LoaderInfo *li;

    li = MODULE(module)->li;

    /* give a chance to the module to cleanup */
    if (li->header3DO->_3DO_Flags & _3DO_MODULE_CALLBACKS)
    {
        if (li->header3DO->_3DO_Flags & _3DO_PRIVILEGE)
        {
            if (li->entryPoint)
            {
                TRACE(("Calling %s with DEMANDLOAD_MAIN_DELETE\n", li->header3DO->_3DO_Name));

                result = ((CallBackFunc)li->entryPoint)(DEMANDLOAD_MAIN_DELETE, daemon, li->li_Item);

                TRACE(("        returned with %x\n", result));

                if (result < 0)
                    return result;
            }
        }
    }

    result = DeleteItem(module);
    if (result >= 0)
        UnloadModule(li);

    return result;
}


/*****************************************************************************/


static Err DaemonImportByName(Item clientTask, Item importer,
                              const char *importName)
{
uint32          impIndex;
Err             result;
Item            module;
Module         *m;
LoaderInfo     *li;

    TRACE(("DaemonImportByName: entering with clientTask %s, importer %s, importName %s\n",
          TASK(clientTask)->t.n_Name,
          MODULE(importer)->n.n_Name,
          importName));

#if 0
    result = IsItemOpened(clientTask, importer);
    if (result < 0)
        return result;
#endif

    if (!IsMemReadable(importName, 1))
        return LOADER_ERR_BADPTR;

    m  = (Module *)LookupItem(importer);
    li = m->li;

    impIndex = result = FindImportIndex(li, importName);
    if (result >= 0)
    {
        module = FindImport(li, impIndex, TRUE);
        if (module < 0)
            module = result = DaemonLoad(importName, TRUE);

        if (module >= 0)
        {
            result = OpenModuleTree(module, clientTask);
            if (result >= 0)
            {
                InitImportedModules(li, clientTask);

                result = RelocateModule(li, TRUE);
                if (result >= 0)
                    return result;

                CloseModuleTree(module, clientTask);
            }

            DaemonUnload(module);
        }
    }
#ifdef BUILD_STRINGS
    else
    {
        printf("ERROR: module %s could not import module %s\n", m->n.n_Name, importName);
        printf("       because %s doesn't appear in the module's import table.\n", importName);
        printf("       You likely need to link %s against %s, or the linker over-optimized things\n", m->n.n_Name, importName);
        printf("       by stripping away modules that it thinks aren't being used\n");
    }
#endif

    return result;
}


/*****************************************************************************/


static Err DaemonUnimportByName(Item clientTask, Item importer,
                                const char *importName)
{
uint32          impIndex;
Err             result;
Module         *m;
LoaderInfo     *li;

    TRACE(("DaemonUnimportByName: entering with clientTask %s, importer %s, importName %s\n",
          TASK(clientTask)->t.n_Name,
          MODULE(importer)->n.n_Name,
          importName));

#if 0
    result = IsItemOpened(clientTask, importer);
    if (result < 0)
        return result;
#endif

    if (!IsMemReadable(importName, 1))
        return LOADER_ERR_BADPTR;

    m  = (Module *)LookupItem(importer);
    li = m->li;

    impIndex = result = FindImportIndex(li, importName);
    if (result >= 0)
    {
        result = FindImport(li, impIndex, FALSE);
        if (result >= 0)
        {
            if ((li->imports->imports[impIndex].flags & IMPORT_NOW) == 0)
            {
                result = CloseModuleTree(clientTask, result);
            }
        }
    }

    return result;
}


/*****************************************************************************/


static Item DaemonOpenModule(Item clientTask, const char *modName,
                             const TagArg *tags)
{
Item    module;
Err     result;
bool    needPriv;
bool    needSig;
Module *mod;
TagArg *tag;

    TRACE(("DaemonOpenModule: entering with clientTask %s, modName %s\n", TASK(clientTask)->t.n_Name,
           modName));

    if (!IsMemReadable(modName, 1))
        return LOADER_ERR_BADPTR;

    module = result = DaemonLoad(modName, FALSE);
    if (module >= 0)
    {
        mod = MODULE(module);

        needPriv = FALSE;
        needSig  = FALSE;

        while (tag = NextTagArg(&tags))
        {
            switch (tag->ta_Tag)
            {
                case MODULE_TAG_MUST_BE_SIGNED:
                {
                    needSig = (tag->ta_Arg ? TRUE : FALSE);
                    break;
                }

                case MODULE_TAG_MUST_BE_PRIVILEGED:
                {
                    needPriv = (tag->ta_Arg ? TRUE : FALSE);
                    break;
                }

                default:
                {
                    result = LOADER_ERR_BADTAG;
                    break;
                }
            }
        }

        if (needPriv)
        {
            if ((mod->li->header3DO->_3DO_Flags & _3DO_PRIVILEGE) == 0)
                result = LOADER_ERR_BADPRIV;
        }

        if (needSig)
        {
            if ((mod->li->header3DO->_3DO_Flags & _3DO_SIGNED) == 0)
                result = LOADER_ERR_BADPRIV;
        }

        if (result >= 0)
        {
            /* Something that was loaded with OpenModule() shouldn't be
             * exporting symbols.
             */
            mod->li->flags |= LF_CANTEXPORT;

            result = OpenModuleTree(result, clientTask);
            if (result >= 0)
                return module;
        }

        DaemonUnload(module);
    }

    return result;
}


/*****************************************************************************/


static Err DaemonCloseModule(Item clientTask, Item module)
{
    TRACE(("DaemonCloseModule: entering with clientTask %s, module %s\n", TASK(clientTask)->t.n_Name,
           MODULE(module)->n.n_Name));

    return CloseModuleTree(clientTask, module);
}


/*****************************************************************************/


static void PurgeModules(void)
{
Module *m;
Err     result;

again:

    ScanList(&KB_FIELD(kb_Modules), m, Module)
    {
        if (m->n.n_OpenCount == 0)
        {
            result = DaemonUnload(m->n.n_Item);
            if (result >= 0)
                goto again;
        }
    }
}


/*****************************************************************************/


/* sit around, waiting for folks to ask for things to be loaded... */
static void LoaderDaemon(void)
{
Item          port;
Item          msgItem;
Message      *msg;
Item          result;
DaemonPacket *pkt;

    TRACE(("LoaderDaemon: entering\n"));

    IncreaseResourceTable(256);

    port = CURRENTTASK->t_DefaultMsgPort;

    while (TRUE)
    {
        ScavengeMem();

        /* We wait for a request to come in. We get three types of events
         * here. Either we get an incoming message, or such a ping signal
         * that wakes us up so we scavenge memory and purge unused modules,
         * or a dead task signal which has the same effect as a ping signal.
         *
         * We handle the dead task case mainly as a convenience to modules
         * that launch threads, like the graphics folio. When these threads
         * die, we'd like to scavenge their resources if possible.
         */
        WaitSignal(MSGPORT(port)->mp_Signal | SIGF_DEADTASK);
        while (TRUE)
        {
            msgItem = GetMsg(port);
            if (msgItem <= 0)
                break;

            msg = MESSAGE(msgItem);
            if ((msg->msg.n_ItemFlags & ITEMNODE_PRIVILEGED) == 0)
            {
                /* hey, who's the wise guy? */
                ReplyMsg(msgItem, BADPRIV, NULL, 0);
                continue;
            }

            TRACE(("LoaderDaemon: processing message\n"));

            pkt    = msg->msg_DataPtr;
            result = 0;

            /* don't call the FS before it has been loaded */
            if (pkt->dp_Client != KB_FIELD(kb_OperatorTask))
                result = ChangeDirectoryInDir(pkt->dp_Client, "");

            if (result >= 0)
            {
                switch (pkt->dp_Op)
                {
                    case DAEMON_IMPORTBYNAME    : result = DaemonImportByName(pkt->dp_Client, (Item)pkt->dp_Arg1, (char *)pkt->dp_Arg2);
                                                  break;

                    case DAEMON_UNIMPORTBYNAME  : result = DaemonUnimportByName(pkt->dp_Client, (Item)pkt->dp_Arg1, (char *)pkt->dp_Arg2);
                                                  break;

                    case DAEMON_OPENMODULE      : result = DaemonOpenModule(pkt->dp_Client, (char *)pkt->dp_Arg1, (TagArg *)pkt->dp_Arg2);
                                                  break;

                    case DAEMON_OPENMODULETREE  : result = OpenModuleTree((Item)pkt->dp_Arg1, (Item)pkt->dp_Arg2);
                                                  break;

                    case DAEMON_CLOSEMODULE     : result = DaemonCloseModule(pkt->dp_Client, (Item)pkt->dp_Arg1);
                                                  break;

                    default                     : result = -1;
                                                  break;
                }

                if (pkt->dp_Client != KB_FIELD(kb_OperatorTask))
                    ChangeDirectory("/");
            }
            ReplyMsg(msgItem, result, 0, 0);
        }
        PurgeModules();
    }
}


/*****************************************************************************/


Err AskDaemon(DaemonOp op, uint32 arg1, void *arg2)
{
Item         port;
Item         msg;
Err          result;
DaemonPacket pkt;

    TRACE(("AskDaemon: entering with op %d, arg 1 0x%x, arg 2 0x%x\n", op, arg1, arg2));

    port = result = CreateMsgPort(NULL, 0, 0);
    if (port >= 0)
    {
        msg = result = CreateMsg(NULL, 0, port);
        if (msg >= 0)
        {
            pkt.dp_Op     = op;
            pkt.dp_Client = CURRENTTASKITEM;
            pkt.dp_Arg1   = arg1;
            pkt.dp_Arg2   = arg2;

            /* make the daemon trust this message */
            MESSAGE(msg)->msg.n_ItemFlags |= ITEMNODE_PRIVILEGED;

            result = SendMsg(daemonPort, msg, &pkt, sizeof(pkt));
            if (result >= 0)
            {
                result = WaitPort(port, msg);
                if (result >= 0)
                    result = MESSAGE(msg)->msg_Result;
            }
            DeleteMsg(msg);
        }
        DeleteMsgPort(port);
    }

    TRACE(("AskDaemon: exiting with 0x%x\n", result));

    return result;
}


/*****************************************************************************/


Err PingDaemon(void)
{
    return SendSignal(daemon, daemonSig);
}


/*****************************************************************************/


Err StartLoaderDaemon(void)
{
Err result;

    daemon = result = CreateItemVA(MKNODEID(KERNELNODE,TASKNODE),
                          TAG_ITEM_NAME,                  "Loader Daemon",
                          CREATETASK_TAG_PC,              LoaderDaemon,
                          CREATETASK_TAG_STACKSIZE,       3072,
                          CREATETASK_TAG_THREAD,          0,
                          CREATETASK_TAG_PRIVILEGED,      TRUE,
                          CREATETASK_TAG_SINGLE_STACK,    TRUE,
                          CREATETASK_TAG_DEFAULTMSGPORT,  TRUE,
                          TAG_END);
    if (result >= 0)
    {
        daemonPort = TASK(result)->t_DefaultMsgPort;
        daemonSig  = MSGPORT(daemonPort)->mp_Signal;
    }

    TRACE(("StartLoaderDaemon: exiting with 0x%x\n",result));

    return result;
}
