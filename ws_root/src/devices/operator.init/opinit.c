/* @(#) opinit.c 96/08/06 1.20 */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/kernelnodes.h>
#include <kernel/task.h>
#include <kernel/kernel.h>
#include <kernel/operror.h>
#include <kernel/dipir.h>
#include <kernel/super.h>
#include <kernel/sysinfo.h>
#include <kernel/mem.h>
#include <file/filesystem.h>
#include <file/filefunctions.h>
#include <loader/loader3do.h>
#include <stdio.h>
#include <string.h>


/*****************************************************************************/


extern void ProvideItemServices(Item serverPort);


/*****************************************************************************/


static Item CreateFileSystem(void)
{
ItemNode *n;

    n = (ItemNode *)FindNamedNode(&KB_FIELD(kb_Modules), "FileSystem");
    if (n == NULL)
        return LOADER_ERR_NOTFOUND;

    return CreateItemVA(MKNODEID(KERNELNODE,TASKNODE),
                        CREATETASK_TAG_PRIVILEGED,   TRUE,
                        CREATETASK_TAG_MODULE,       n->n_Item,
                        CREATETASK_TAG_THREAD,       TRUE,
                        CREATETASK_TAG_SINGLE_STACK, TRUE,
                        TAG_END);
}


/*****************************************************************************/


static const TagArg moduleTags[] =
{
    {MODULE_TAG_MUST_BE_SIGNED,  (TagData)TRUE},
    {TAG_END, }
};

static const TagArg shellSearch[] =
{
    {FILESEARCH_TAG_SEARCH_FILESYSTEMS,  (TagData) DONT_SEARCH_UNBLESSED},
    {TAG_END, 0}
};


static Item CreateShell(void)
{
Item module;
Err  result;
char path[64];

    result = FindFileAndIdentify(path, sizeof(path), "System.m2/Tasks/shell", shellSearch);
    if (result >= 0)
    {
        module = result = OpenModule(path, OPENMODULE_FOR_TASK, moduleTags);
        if (module >= 0)
        {
            result = CreateTask(module, "Shell", NULL);
            CloseModule(module);
        }
    }

    return result;
}


/*****************************************************************************/


extern Err CreateBuiltinDDFs(void);
extern Err TimerDriverMain(void);
extern Err ChanDriverMain(void);
extern Err PCMCIADriverMain(void);
extern Err MicroslotDriverMain(void);
extern Err PChanDriverMain(void);
extern Err MChanDriverMain(void);

typedef Err (* DriverInitFunc)(void);

static const DriverInitFunc initFuncs[] =
{
    TimerDriverMain,
    ChanDriverMain,
    PCMCIADriverMain,
    MicroslotDriverMain,
    PChanDriverMain,
    MChanDriverMain
};


#ifdef BUILD_STRINGS
#define DO(x,name) {err = x; if (err < 0) { printf("OPERATOR: %s failed: ", name); PrintfSysErr(err); }}
#define INFO(x)    printf x
#else
#define DO(x,name) x
#define INFO(x)
#endif


void OperatorInit(void)
{
uint32 i;
Err    err;

    AllocSignal(RESCAN_SIGNAL);
    AllocSignal(WAKEE_WAKE_SIGNAL);

    DO(CallBackSuper(CreateBuiltinDDFs, 0, 0, 0), "CreateBuiltinDDFs");

    for (i = 0; i < sizeof(initFuncs) / sizeof(DriverInitFunc); i++)
    {
        err = initFuncs[i]();
        if (err >= 0)
        {
            err = OpenItem(err, NULL);
            if (err < 0)
            {
                INFO(("OPERATOR: failed to open device #%u\n", i));
            }
        }
        else
        {
            INFO(("OPERATOR: failed to create device #%u\n", i));
        }
    }

    DO(CreateFileSystem(),    "CreateFileSystem");

    WaitSignal(RESCAN_SIGNAL);

    PlugNPlay();
    WaitSignal(WAKEE_WAKE_SIGNAL);

    DO(CreateShell(), "CreateShell");
}


/****************************************************************************/


int main(void)
{
    return 0;
}
