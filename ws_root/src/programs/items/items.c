/* @(#) items.c 96/09/05 1.62 */

/**
|||	AUTODOC -class Shell_Commands -name Items
|||	Displays lists of active items
|||
|||	  Format
|||
|||	    Items [item number]
|||	          [-name <item name>]
|||	          [-type <item type>]
|||	          [-owner <task name>]
|||	          [-full]
|||
|||	  Description
|||
|||	    This command displays information about the items that
|||	    currently exist in the system. The information is sent
|||	    out to the debugging terminal.
|||
|||	  Arguments
|||
|||	    [item number]
|||	        Requests that only information on the item number supplied be
|||	        displayed. The item number can be in decimal, or in
|||	        hexadecimal starting with 0x or $.
|||
|||	    -name <item name>
|||	        Requests that only items with the supplied name be listed.
|||
|||	    -type <item type>
|||	        Requests that only items with the supplied type be listed. The
|||	        currently supported types are:
|||
|||	        Folio
|||	        Task
|||	        FIRQ
|||	        Semaphore
|||	        Message
|||	        MsgPort
|||	        Driver
|||	        IOReq
|||	        Device
|||	        Timer
|||	        ErrorText
|||	        FileSystem
|||	        File
|||	        Alias
|||	        Locale
|||	        Template
|||	        Instrument
|||	        Knob
|||	        Sample
|||	        Cue
|||	        Envelope
|||	        Attachment
|||	        Tuning
|||	        Probe
|||	        AudioClock
|||	        Bitmap
|||	        View
|||	        ViewList
|||	        Projector
|||	        Font
|||
|||	    -owner <task name>
|||	        Requests that only items owned by the supplied task be
|||	        displayed.
|||
|||	    -full
|||	        Requests that any extra information available on the items
|||	        being listed be displayed.
|||
|||	  Implementation
|||
|||	    Command implemented in V20.
|||
|||	  Location
|||
|||	    System.m2/Programs/items
**/

/* To add new item types, simply find the pertinent string table, and add
 * the new item name string at the end.
 *
 * To add new subsys types, define a new name table for the items in the
 * subsystem (that is, a table similar to kernelNodes or audioNodes), and
 * add an entry for this table in the nameTables array.
 */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/list.h>
#include <kernel/kernel.h>
#include <kernel/task.h>
#include <kernel/kernelnodes.h>
#include <kernel/device.h>
#include <kernel/io.h>
#include <kernel/semaphore.h>
#include <kernel/debug.h>
#include <kernel/operror.h>
#include <graphics/font.h>
#include <loader/loader3do.h>
#include <loader/elf.h>
#include <loader/elf_3do.h>
#include <file/filesystem.h>
#include <audio/audio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


/*****************************************************************************/


static void DumpExtraFolio(Folio *f);
static void DumpExtraIOReq(IOReq *ioReq);
static void DumpExtraSemaphore(Semaphore *sem);
static void DumpExtraMessage(Message *msg);
static void DumpExtraMsgPort(MsgPort *port);
static void DumpExtraErrorText(ErrorText *errText);
static void DumpExtraAlias(Alias *alias);
static void DumpExtraModule(Module *m);
static void DumpExtraSample(ItemNode *sample);
static void DumpExtraDevice(Device *dev);


/*****************************************************************************/


typedef void (* DumpExtraFunc)(void *data);

typedef struct ItemMap
{
    char          *im_Name;
    DumpExtraFunc  im_DumpExtra;
} ItemMap;

static const ItemMap kernelNodes[] =
{
    {"<unknown>",	NULL},
    {NULL,		NULL},
    {NULL,		NULL},
    {NULL,		NULL},
    {"Folio",		(DumpExtraFunc)DumpExtraFolio},
    {"Task",		NULL},
    {"FIRQ",		NULL},
    {"Semaphore",	(DumpExtraFunc)DumpExtraSemaphore},
    {NULL,		NULL},
    {"Message",		(DumpExtraFunc)DumpExtraMessage},
    {"MsgPort",		(DumpExtraFunc)DumpExtraMsgPort},
    {NULL,		NULL},
    {NULL,		NULL},
    {"Driver",		NULL},
    {"IOReq",		(DumpExtraFunc)DumpExtraIOReq},
    {"Device",		(DumpExtraFunc)DumpExtraDevice},
    {"Timer",		NULL},
    {"ErrorText",	(DumpExtraFunc)DumpExtraErrorText},
    {"MemLock",		NULL},
    {"Module",		(DumpExtraFunc)DumpExtraModule}
};

static const ItemMap fileNodes[] =
{
    {"<unknown>",	NULL},
    {"FileSystem",	NULL},
    {"File",		NULL},
    {"Alias",		(DumpExtraFunc)DumpExtraAlias},
};

static const ItemMap intlNodes[] =
{
    {"<unknown>",	NULL},
    {"Locale",		NULL},
};

static const ItemMap audioNodes[] =
{
    {"<unknown>",	NULL},
    {"Template",	NULL},
    {"Instrument",	NULL},
    {"Knob",		NULL},
    {"Sample",		(DumpExtraFunc)DumpExtraSample},
    {"Cue",		NULL},
    {"Envelope",	NULL},
    {"Attachment",	NULL},
    {"Tuning",		NULL},
    {"Probe",		NULL},
    {"AudioClock",	NULL},
};

static const ItemMap graphicsNodes[] =
{
    {"<unknown>",       NULL},
    {"Bitmap",          NULL},
    {"View",            NULL},
    {"ViewList",        NULL},
    {"Projector",       NULL},
};

static const ItemMap fontNodes[] =
{
    {"<unknown>",       NULL},
    {"Font",            NULL},
};


/*****************************************************************************/


typedef struct Subsystem
{
    char    *ss_Name;
    uint32   ss_SubsysType;
    ItemMap *ss_ItemMap;
    uint32   ss_NumItems;
} Subsystem;

static const Subsystem subsystems[] =
{
    {"Kernel",        NST_KERNEL,      kernelNodes,   sizeof(kernelNodes) / sizeof(ItemMap)},
    {"FileSystem",    NST_FILESYS,     fileNodes,     sizeof(fileNodes) / sizeof(ItemMap)},
    {"Audio",         NST_AUDIO,       audioNodes,    sizeof(audioNodes) / sizeof(ItemMap)},
    {"International", NST_INTL,        intlNodes,     sizeof(intlNodes) / sizeof(ItemMap)},
    {"Graphics",      NST_GRAPHICS,    graphicsNodes, sizeof(graphicsNodes) / sizeof(ItemMap)},
    {"Font",          NST_FONT,        fontNodes,     sizeof(fontNodes) / sizeof(ItemMap)},
};

#define NUM_SUBSYSTEMS (sizeof(subsystems) / sizeof(Subsystem))


/*****************************************************************************/


typedef struct CommandMap
{
    uint32  cm_Command;
    char   *cm_Name;
} CommandMap;

static const CommandMap commandMap[] =
{
    {CMD_WRITE                  , "CMD_WRITE"},
    {CMD_READ                   , "CMD_READ"},
    {CMD_STATUS                 , "CMD_STATUS"},
    {CMD_GETMAPINFO             , "CMD_GETMAPINFO"},
    {CMD_MAPRANGE               , "CMD_MAPRANGE"},
    {CMD_UNMAPRANGE             , "CMD_UNMAPRANGE"},
    {CMD_BLOCKREAD              , "CMD_BLOCKREAD"},
    {CMD_BLOCKWRITE             , "CMD_BLOCKWRITE"},
    {CMD_STREAMREAD             , "CMD_STREAMREAD"},
    {CMD_STREAMWRITE            , "CMD_STREAMWRITE"},
    {CMD_GETICON                , "CMD_GETICON"},
    {CMD_PREFER_FSTYPE          , "CMD_PREFER_FSTYPE"},
    {CMD_STREAMFLUSH            , "CMD_STREAMFLUSH"},
    {TE_CMD_EXECUTELIST         , "TE_CMD_EXECUTELIST"},
    {TE_CMD_SETFRAMEBUFFER      , "TE_CMD_SETFRAMEBUFFER"},
    {TE_CMD_SETZBUFFER          , "TE_CMD_SETZBUFFER"},
    {TE_CMD_STEP                , "TE_CMD_STEP"},
    {TE_CMD_SPEEDCONTROL        , "TE_CMD_SPEEDCONTROL"},
    {TE_CMD_DISPLAYFRAMEBUFFER  , "TE_CMD_DISPLAYFRAMEBUFFER"},
    {TE_CMD_GETIDLETIME         , "TE_CMD_GETIDLETIME"},
    {GFXCMD_PROJECTORMODULE     , "GFXCMD_PROJECTORMODULE"},
    {FILECMD_READDIR            , "FILECMD_READDIR"},
    {FILECMD_GETPATH            , "FILECMD_GETPATH"},
    {FILECMD_READENTRY          , "FILECMD_READENTRY"},
    {FILECMD_ALLOCBLOCKS        , "FILECMD_ALLOCBLOCKS"},
    {FILECMD_SETEOF             , "FILECMD_SETEOF"},
    {FILECMD_ADDENTRY           , "FILECMD_ADDENTRY"},
    {FILECMD_DELETEENTRY        , "FILECMD_DELETEENTRY"},
    {FILECMD_SETTYPE            , "FILECMD_SETTYPE"},
    {FILECMD_OPENENTRY          , "FILECMD_OPENENTRY"},
    {FILECMD_FSSTAT             , "FILECMD_FSSTAT"},
    {FILECMD_ADDDIR             , "FILECMD_ADDDIR"},
    {FILECMD_DELETEDIR          , "FILECMD_DELETEDIR"},
    {FILECMD_SETVERSION         , "FILECMD_SETVERSION"},
    {FILECMD_SETBLOCKSIZE       , "FILECMD_SETBLOCKSIZE"},
    {FILECMD_SETFLAGS           , "FILECMD_SETFLAGS"},
    {FILECMD_RENAME             , "FILECMD_RENAME"},
    {FILECMD_SETDATE            , "FILECMD_SETDATE"},
    {CDROMCMD_READ              , "CDROMCMD_READ"},
    {CDROMCMD_SCAN_READ         , "CDROMCMD_SCAN_READ"},
    {CDROMCMD_SETDEFAULTS       , "CDROMCMD_SETDEFAULTS"},
    {CDROMCMD_DISCDATA          , "CDROMCMD_DISCDATA"},
    {CDROMCMD_READ_SUBQ         , "CDROMCMD_READ_SUBQ"},
    {CDROMCMD_OPEN_DRAWER       , "CDROMCMD_OPEN_DRAWER"},
    {CDROMCMD_CLOSE_DRAWER      , "CDROMCMD_CLOSE_DRAWER"},
    {CDROMCMD_WOBBLE_INFO       , "CDROMCMD_WOBBLE_INFO"},
    {CDROMCMD_RESIZE_BUFFERS    , "CDROMCMD_RESIZE_BUFFERS"},
    {CDROMCMD_DIAG_INFO         , "CDROMCMD_DIAG_INFO"},
    {TIMERCMD_GETTIME_VBL       , "TIMERCMD_GETTIME_VBL"},
    {TIMERCMD_SETTIME_VBL       , "TIMERCMD_SETTIME_VBL"},
    {TIMERCMD_DELAY_VBL         , "TIMERCMD_DELAY_VBL"},
    {TIMERCMD_DELAYUNTIL_VBL    , "TIMERCMD_DELAYUNTIL_VBL"},
    {TIMERCMD_METRONOME_VBL     , "TIMERCMD_METRONOME_VBL"},
    {TIMERCMD_GETTIME_USEC      , "TIMERCMD_GETTIME_USEC"},
    {TIMERCMD_SETTIME_USEC      , "TIMERCMD_SETTIME_USEC"},
    {TIMERCMD_DELAY_USEC        , "TIMERCMD_DELAY_USEC"},
    {TIMERCMD_DELAYUNTIL_USEC   , "TIMERCMD_DELAYUNTIL_USEC"},
    {TIMERCMD_METRONOME_USEC    , "TIMERCMD_METRONOME_USEC"},
    {SLOTCMD_SETTIMING          , "SLOTCMD_SETTIMING"},
    {CPORT_CMD_READ             , "CPORT_CMD_READ"},
    {CPORT_CMD_WRITE            , "CPORT_CMD_WRITE"},
    {CPORT_CMD_READEVENT        , "CPORT_CMD_READEVENT"},
    {CPORT_CMD_WRITEEVENT       , "CPORT_CMD_WRITEEVENT"},
    {CPORT_CMD_CONFIGURE        , "CPORT_CMD_CONFIGURE"},
    {MPEGVIDEOCMD_CONTROL       , "MPEGVIDEOCMD_CONTROL"},
    {MPEGVIDEOCMD_WRITE         , "MPEGVIDEOCMD_WRITE"},
    {MPEGVIDEOCMD_READ          , "MPEGVIDEOCMD_READ"},
    {PROXY_CMD_CREATE_DEVICE    , "PROXY_CMD_CREATE_DEVICE"},
    {PROXY_CMD_DWIM             , "PROXY_CMD_DWIM"},
    {PROXY_CMD_CREATE_SOFTFIRQ  , "PROXY_CMD_CREATE_SOFTFIRQ"},
    {HOST_CMD_SEND              , "HOST_CMD_SEND"},
    {HOST_CMD_RECV              , "HOST_CMD_RECV"},
    {HOSTFS_CMD_MOUNTFS         , "HOSTFS_CMD_MOUNTFS"},
    {HOSTFS_CMD_OPENENTRY       , "HOSTFS_CMD_OPENENTRY"},
    {HOSTFS_CMD_CLOSEENTRY      , "HOSTFS_CMD_CLOSEENTRY"},
    {HOSTFS_CMD_CREATEFILE      , "HOSTFS_CMD_CREATEFILE"},
    {HOSTFS_CMD_CREATEDIR       , "HOSTFS_CMD_CREATEDIR"},
    {HOSTFS_CMD_DELETEENTRY     , "HOSTFS_CMD_DELETEENTRY"},
    {HOSTFS_CMD_READENTRY       , "HOSTFS_CMD_READENTRY"},
    {HOSTFS_CMD_READDIR         , "HOSTFS_CMD_READDIR"},
    {HOSTFS_CMD_ALLOCBLOCKS     , "HOSTFS_CMD_ALLOCBLOCKS"},
    {HOSTFS_CMD_BLOCKREAD       , "HOSTFS_CMD_BLOCKREAD"},
    {HOSTFS_CMD_STATUS          , "HOSTFS_CMD_STATUS"},
    {HOSTFS_CMD_FSSTAT          , "HOSTFS_CMD_FSSTAT"},
    {HOSTFS_CMD_BLOCKWRITE      , "HOSTFS_CMD_BLOCKWRITE"},
    {HOSTFS_CMD_SETEOF          , "HOSTFS_CMD_SETEOF"},
    {HOSTFS_CMD_SETTYPE         , "HOSTFS_CMD_SETTYPE"},
    {HOSTFS_CMD_SETVERSION      , "HOSTFS_CMD_SETVERSION"},
    {HOSTFS_CMD_DISMOUNTFS      , "HOSTFS_CMD_DISMOUNTFS"},
    {HOSTFS_CMD_SETBLOCKSIZE    , "HOSTFS_CMD_SETBLOCKSIZE"},
    {HOSTCONSOLE_CMD_GETCMDLINE , "HOSTCONSOLE_CMD_GETCMDLINE"},
    {SER_CMD_STATUS             , "SER_CMD_STATUS"},
    {SER_CMD_SETCONFIG          , "SER_CMD_SETCONFIG"},
    {SER_CMD_GETCONFIG          , "SER_CMD_GETCONFIG"},
    {SER_CMD_WAITEVENT          , "SER_CMD_WAITEVENT"},
    {SER_CMD_SETRTS             , "SER_CMD_SETRTS"},
    {SER_CMD_SETDTR             , "SER_CMD_SETDTR"},
    {SER_CMD_SETLOOPBACK        , "SER_CMD_SETLOOPBACK"},
    {SER_CMD_BREAK              , "SER_CMD_BREAK"},
    {USLOTCMD_RESET             , "USLOTCMD_RESET"},
    {USLOTCMD_SETCLOCK          , "USLOTCMD_SETCLOCK"},
    {USLOTCMD_SEQ            	, "USLOTCMD_SEQ"},
};

#define NUM_COMMANDMAPS (sizeof(commandMap) / sizeof(CommandMap))


/*****************************************************************************/


typedef struct FlagMap
{
    uint32  fm_Flag;
    char   *fm_Name;
} FlagMap;


static const FlagMap nodeFlags[] =
{
    {NODE_NAMEVALID,    "NODE_NAMEVALID"},
    {NODE_SIZELOCKED,   "NODE_SIZELOCKED"},
    {NODE_ITEMVALID,    "NODE_ITEMVALID"},
    {NODE_OPENVALID,    "NODE_OPENVALID"},
    {0,                 NULL}
};

static const FlagMap messageFlags[] =
{
    {NODE_NAMEVALID,        "NODE_NAMEVALID"},
    {NODE_SIZELOCKED,       "NODE_SIZELOCKED"},
    {NODE_ITEMVALID,        "NODE_ITEMVALID"},
    {NODE_OPENVALID,        "NODE_OPENVALID"},
    {MESSAGE_SENT,          "MESSAGE_SENT"},
    {MESSAGE_REPLIED,       "MESSAGE_REPLIED"},
    {MESSAGE_SMALL,         "MESSAGE_SMALL"},
    {MESSAGE_PASS_BY_VALUE, "MESSAGE_PASS_BY_VALUE"},
    {0,                     NULL}
};

static const FlagMap itemFlags[] =
{
    {ITEMNODE_NOTREADY,         "ITEMNODE_NOTREADY"},
    {ITEMNODE_PRIVILEGED,       "ITEMNODE_PRIVILEGED"},
    {ITEMNODE_UNIQUE_NAME,      "ITEMNODE_UNIQUE_NAME"},
    {ITEMNODE_DELETED,          "ITEMNODE_DELETED"},
    {0,                         NULL}
};

static const FlagMap ioFlags[] =
{
    {IO_DONE,         "IO_DONE"},
    {IO_QUICK,        "IO_QUICK"},
    {IO_INTERNAL,     "IO_INTERNAL"},
    {IO_WAITING,      "IO_WAITING"},
    {IO_MESSAGEBASED, "IO_MESSAGEBASED"},
    {0, NULL}
};

static const FlagMap taskFlags[] =
{
    {TASK_READY,           "TASK_READY"},
    {TASK_WAITING,         "TASK_WAITING"},
    {TASK_RUNNING,         "TASK_RUNNING"},
    {TASK_QUANTUM_EXPIRED, "TASK_QUANTUM_EXPIRED"},
    {0, NULL}
};


/*****************************************************************************/


static void DumpFlags(const FlagMap *fm, uint32 flags)
{
uint32 i,j;
bool   first;

    printf("0x%08x",flags);
    if (flags)
    {
        first = TRUE;
        for (i = 0; i < 31; i++)
        {
            if ((1 << i) & flags)
            {
                j = 0;
                while (fm[j].fm_Name)
                {
                    if (fm[j].fm_Flag & (1 << i))
                    {
                        if (first)
                        {
                            printf(" (");
                            first = FALSE;
                        }
                        else
                        {
                            printf(" | ");
                        }

                        printf("%s",fm[j].fm_Name);
                        break;
                    }

                    j++;
                }
            }
        }

        if (!first)
            printf(")");
    }
}


/*****************************************************************************/


static void DumpExtraFolio(Folio *f)
{
    printf("  f_TaskDataIndex    %u\n",f->f_TaskDataIndex);
    printf("  f_MaxSwiFunctions  %u\n",f->f_MaxSwiFunctions);
    printf("  f_MaxNodeType      %u\n",f->f_MaxNodeType);
}


/*****************************************************************************/


static void DumpExtraRelocs(RelocSet *relocs)
{
    int i;
    struct Elf32_Rela *reloc = relocs->relocs;

    printf("%d Relocs\n", relocs->numRelocs);
    for(i = 0; i < relocs->numRelocs; i++)
	{
	if(IS_IMPREL_RELOC(ELF32_R_TYPE(reloc->r_info)))
	    printf("    Import: module %d, index %d, ",
		   ELF3DO_MODULE_INDEX(reloc->r_info),
		   ELF3DO_EXPORT_INDEX(reloc->r_info));
	else
	    printf("    Other: sym 0x%x, ", ELF32_R_SYM(reloc->r_info));

	printf("addend 0x%x, type 0x%x\n", reloc->r_addend,
	       ELF32_R_TYPE(reloc->r_info));
	}
}


static void DumpExtraModule(Module *m)
{
    printf("  code 		  @ 0x%08x (0x%x bytes)\n",
	    m->li->codeBase, m->li->codeLength);
    printf("  data/bss		  @ 0x%08x (0x%x bytes)\n",
	    m->li->dataBase, m->li->dataLength + m->li->bssLength);

    printf("  3DO_Header	  @ 0x%08x\n", m->li->header3DO);

    if(m->li->imports)
	{
	int i;
	ELF_Imp3DO *imports = m->li->imports;
	ELF_ImportRec *rec = m->li->imports->imports;

	printf("  %d imports:\n", imports->numImports);
	for(i = 0; i < imports->numImports; i++, rec++)
	    printf("    name %s, code 0x%x, version %d, revision %d, flags 0x%x\n",
		   NULL, rec->libraryCode,
		   rec->libraryVersion, rec->libraryRevision,
		   rec->flags);
	}

    if(m->li->exports)
	{
	int i;
	ELF_Exp3DO *exports = m->li->exports;

	printf("  %d exports:\n", exports->numExports);
	for(i = 0; i < exports->numExports; i++)
	    printf("    code 0x%x\n", exports->exportWords[i]);
	}

    printf("  Text: ");
    DumpExtraRelocs(&m->li->textRelocs);

    printf("  Data: ");
    DumpExtraRelocs(&m->li->dataRelocs);
}


/*****************************************************************************/


static void DumpExtraIOReq(IOReq *ior)
{
uint32 i;
char   errorBuffer[100];

    printf("  io_Dev         0x%08x ('%s', item 0x%08x)\n",ior->io_Dev,ior->io_Dev->dev.n_Name,ior->io_Dev->dev.n_Item);
    printf("  io_CallBack    0x%08x\n",ior->io_CallBack);

    printf("  ioi_Command    %d",ior->io_Info.ioi_Command);
    for (i = 0; i < NUM_COMMANDMAPS; i++)
    {
        if (ior->io_Info.ioi_Command == commandMap[i].cm_Command)
        {
            printf(" (%s)",commandMap[i].cm_Name);
            break;
        }
    }
    printf("\n");

    printf("  ioi_Flags      "); DumpFlags(ioFlags,ior->io_Info.ioi_Flags); printf("\n");

    printf("\n");

    printf("  ioi_CmdOptions 0x%08x\n",ior->io_Info.ioi_CmdOptions);
    printf("  ioi_UserData   0x%08x\n",ior->io_Info.ioi_UserData);
    printf("  ioi_Offset     %d\n",ior->io_Info.ioi_Offset);
    printf("  ioi_Send       buf 0x%08x, len %d\n",ior->io_Info.ioi_Send.iob_Buffer,ior->io_Info.ioi_Send.iob_Len);
    printf("  ioi_Recv       buf 0x%08x, len %d\n",ior->io_Info.ioi_Recv.iob_Buffer,ior->io_Info.ioi_Recv.iob_Len);
    printf("  io_Actual      %d\n",ior->io_Actual);
    printf("  io_Flags       "); DumpFlags(ioFlags,ior->io_Flags); printf("\n");
    printf("  io_Error       0x%08x",ior->io_Error);

    if (ior->io_Error < 0)
    {
        GetSysErr(errorBuffer,sizeof(errorBuffer),ior->io_Error);
        printf(" ('%s')",errorBuffer);
    }
    printf("\n");

    printf("  io_MsgItem     0x%08x\n",ior->io_MsgItem);
    printf("\n");
}


/*****************************************************************************/


static void DumpExtraDevice(Device *dev)
{
IOReq *ior;

    printf("  dev_HWResource      %x\n", dev->dev_HWResource);
    printf("  dev_LowerDevice     %x\n", dev->dev_LowerDevice);
    printf("  dev_DriverData      %x\n", dev->dev_DriverData);
    printf("  dev_Driver          %x\n", dev->dev_Driver);
    printf("  dev_DDFNode         %x\n", dev->dev_DDFNode);
    printf("  dev_IOReqs          ");
    ScanList(&dev->dev_IOReqs, ior, IOReq)
    {
	printf("%x ", ior);
    }
    printf("\n");
}


/*****************************************************************************/


static void DumpExtraSemaphore(Semaphore *sem)
{
SemaphoreWaitNode *swn;
SharedLocker      *sl;

    printf("  sem_SharedLockers   ");
    if (!IsListEmpty(&sem->sem_SharedLockers))
    {
        ScanList(&sem->sem_SharedLockers,sl,SharedLocker)
        {
            printf("0x%08x ",sl->sl_Task->t.n_Item);
        }
        printf("\n");
    }
    else
    {
        printf("<list empty>\n");
    }

    printf("  sem_Locker          0x%08x\n",sem->sem_Locker);
    printf("  sem_NestCnt         %d\n",sem->sem_NestCnt);

    printf("  sem_TaskWaitingList ");
    if (!IsListEmpty(&sem->sem_TaskWaitingList))
    {
        ScanList(&sem->sem_TaskWaitingList,swn,SemaphoreWaitNode)
        {
            printf("0x%08x ",swn->swn_Task->t.n_Item);
        }
        printf("\n");
    }
    else
    {
        printf("<list empty>\n");
    }

    printf("  sem_UserData        0x%08x\n",sem->sem_UserData);
}


/*****************************************************************************/


static void DumpExtraMessage(Message *msg)
{
    printf("  msg_ReplyPort   0x%08x\n",msg->msg_ReplyPort);
    printf("  msg_Result      %d\n",msg->msg_Result);
    printf("  msg_DataPtr     0x%08x\n",msg->msg_DataPtr);
    printf("  msg_DataSize    0x%d\n",msg->msg_DataSize);
    printf("  msg_Holder      0x%08x\n",msg->msg_Holder);
    printf("  msg_DataPtrSize %d\n",msg->msg_DataPtrSize);
    printf("  msg_UserData    0x%08x\n",msg->msg_UserData);
    printf("\n");
}


/*****************************************************************************/


static void DumpExtraMsgPort(MsgPort *port)
{
    printf("  mp_Signal   0x%08x\n",port->mp_Signal);
    printf("  mp_UserData 0x%08x\n",port->mp_UserData);
}


/*****************************************************************************/


static void DumpExtraErrorText(ErrorText *errText)
{
    printf("  et_ObjID         0x%08x\n",errText->et_ObjID);
    printf("  et_ErrorTable    0x%08x\n",errText->et_ErrorTable);
    printf("  et_MaxErr        %d\n",errText->et_MaxErr);
}


/*****************************************************************************/


static void DumpExtraAlias(Alias *alias)
{
    printf("  a_Value      '%s'\n",alias->a_Value);
}


/*****************************************************************************/


static void DumpExtraSample(ItemNode *sample)
{
    if (OpenAudioFolio() >= 0)
    {
        DebugSample(sample->n_Item);
        CloseAudioFolio();
    }
}


/*****************************************************************************/


static bool DumpItem(Item it, bool full)
{
ItemNode     *n;
bool          found;
uint32        table;
Item          owner;
ItemNode     *t;
DumpExtraFunc dumpExtra;
char          typeStr[80];
char          ownerStr[80];
char          nameStr[80];

    n = (ItemNode *)LookupItem(it);
    if (n)
    {
        found     = FALSE;
        dumpExtra = NULL;

        for (table = 0; table < NUM_SUBSYSTEMS; table++)
        {
            if (subsystems[table].ss_SubsysType == n->n_SubsysType)
            {
                found = TRUE;
                break;
            }
        }

        if (found)
        {
            if (n->n_Type < subsystems[table].ss_NumItems)
            {
                strcpy(typeStr,subsystems[table].ss_ItemMap[n->n_Type].im_Name);
                dumpExtra = subsystems[table].ss_ItemMap[n->n_Type].im_DumpExtra;
            }
            else
            {
                sprintf(typeStr,"<type %d from %s>",n->n_Type,subsystems[table].ss_Name);
            }
        }
        else
        {
            /* unknown subsystem, try to find the responsible party and print its name  */
            Item folioI = n->n_SubsysType;
            Node *n = (Node *)LookupItem(folioI);
            sprintf(typeStr,"(%s's node) ",n->n_Name);
        }

        owner = n->n_Owner;
        t = (ItemNode *)LookupItem(owner);
        if (owner == 0)
        {
            strcpy(ownerStr,"'kernel'");
        }
        else if (t == 0)
        {
            strcpy(ownerStr,"<unknown>");
        }
        else
        {
            sprintf(ownerStr,"'%s'",t->n_Name);
        }

        if (n->n_Flags & NODE_NAMEVALID)
        {
            if (n->n_Name)
            {
                sprintf(nameStr,"'%s'",n->n_Name);
            }
            else
            {
                strcpy(nameStr,"<null>");
            }
        }
        else
        {
            strcpy(nameStr,"<unnamed>");
        }

        if (full)
        {
            printf("-----------\n");
            printf("  Item address 0x%08x\n",n);
            printf("  n_Type       %s\n",typeStr);
            printf("  n_Priority   %u\n",n->n_Priority);
            printf("  n_Flags      ");

            if ((n->n_SubsysType == KERNELNODE) && (n->n_Type == MESSAGENODE))
                DumpFlags(messageFlags,n->n_Flags);
            else if ((n->n_SubsysType == KERNELNODE) && (n->n_Type == TASKNODE))
                DumpFlags(taskFlags,n->n_Flags);
            else
                DumpFlags(nodeFlags,n->n_Flags);

            printf("\n");
            printf("  n_Size       %d\n",n->n_Size);
            printf("  n_Name       %s\n",nameStr);
            printf("  n_Version    %u\n",n->n_Version);
            printf("  n_Revision   %u\n",n->n_Revision);
            printf("  n_ItemFlags  "); DumpFlags(itemFlags,n->n_ItemFlags); printf("\n");
            printf("  n_Item       0x%08x\n",n->n_Item);
            printf("  n_Owner      0x%08x (%s)\n",n->n_Owner,ownerStr);

            if (n->n_Flags & NODE_OPENVALID)
                printf("  n_OpenCount  %u\n",((OpeningItemNode *)n)->n_OpenCount);

            if (dumpExtra)
                (* dumpExtra)(n);
        }
        else
        {
	    printf("%-12s 0x%08x @ 0x%08x %3d ",typeStr, it, n, n->n_Priority);

            if (n->n_Flags & NODE_OPENVALID)
            {
                printf ("%4u ", ((OpeningItemNode *)n)->n_OpenCount);
            }
            else
            {
                printf ("     ");
            }

            if (n->n_Flags & NODE_NAMEVALID)
            {
                printf("%-14s ",ownerStr);
                printf(" %s",nameStr);
            }
            else
            {
                printf("%-12s    namenotvalid",ownerStr);
            }
	    /* room to print some more stuff out now */
            if ((n->n_SubsysType == KERNELNODE) && (n->n_Type == MODULENODE))
	    {
		Module *m = (Module *)n;
		printf("	c[%lx,%lx] d[%lx,%lx]",m->li->codeBase,m->li->codeLength,m->li->dataBase,m->li->dataLength + m->li->bssLength);
	    }
	    printf("\n");
        }

        return (TRUE);
    }

    return (FALSE);
}


/*****************************************************************************/


#define ITEMS_PER_BLOCK	512

static ItemEntry *GetItemEntryPtr(uint32 i)
{
ItemEntry  *p;
ItemEntry **ipt;
uint32      j;

    ipt = KernelBase->kb_ItemTable;
    if (KernelBase->kb_MaxItem <= i)	return 0;
    j = i/ITEMS_PER_BLOCK;	/* which block */
    p = ipt[j];
    i -= j*ITEMS_PER_BLOCK;	/* which one in this block? */

    return p + i;
}


/*****************************************************************************/


static bool GetType(const char *name, int32 *subsystype, int32 *type)
{
uint32  i;
uint32  table;
char   *subname;

    /* try to match string to our tables */
    for (table = 0; table < NUM_SUBSYSTEMS; table++)
    {
        for (i = 0; i < subsystems[table].ss_NumItems; i++)
        {
            subname = subsystems[table].ss_ItemMap[i].im_Name;
            if (subname)
            {
                if (strcasecmp(name,subname) == 0)
                {
                    *type       = i;
                    *subsystype = subsystems[table].ss_SubsysType;

                    return (TRUE);
                }
            }
        }
    }

    return (FALSE);
}


/*****************************************************************************/


static void PrintUsage(void)
{
    printf("items - prints out list of all active items\n");
    printf("  <item number>      - display info on the item with this number\n");
    printf("  -name <item name>  - display info on items with this name\n");
    printf("  -type <item type>  - display info on items with this type\n");
    printf("  -owner <task name> - display info on items belonging to this task\n");
    printf("  -full              - display more details if available\n");
}


/*****************************************************************************/


static bool MatchItem(ItemNode   *ip,
                      int32       itemType,
                      int32       subsysType,
                      Item        itemOwner,
                      const char *itemName)
{
    /* Does this item match criteria for dumping contents? */

    if (itemType >= 0)
    {
        if ((ip->n_SubsysType != subsysType) || (ip->n_Type != itemType))
            return (FALSE);
    }

    if (itemOwner >= 0)
    {
        if (ip->n_Owner != itemOwner)
            return FALSE;
    }

    if (itemName)
    {
        if (!ip->n_Name)
            return (FALSE);

        if (!(ip->n_Flags & NODE_NAMEVALID))
            return (FALSE);

        if (strcasecmp(itemName,ip->n_Name))
            return (FALSE);
    }

    return (TRUE);
}


/*****************************************************************************/


static uint32 ConvertNum(char *str)
{
    if (*str == '$')
    {
        str++;
        return strtoul(str,0,16);
    }

    return strtoul(str,0,0);
}


/*****************************************************************************/


int main(int argc, char **argv)
{
int32      parm;
bool       full;
ItemEntry *ie;
Item       gen;
uint32     i;
ItemNode  *ip;
Item       itm;
int32      itemType;
int32      subsysType;
Item       itemOwner;
char      *itemName;

    itemType   = -1;
    subsysType = -1;
    itemOwner  = -1;
    itemName   = NULL;
    full       = FALSE;

    for (parm = 1; parm < argc; parm++)
    {
        if ((strcasecmp("-help",argv[parm]) == 0)
         || (strcasecmp("-?",argv[parm]) == 0)
         || (strcasecmp("help",argv[parm]) == 0)
         || (strcasecmp("?",argv[parm]) == 0))
        {
            PrintUsage();
            return (0);
        }

        if (strcasecmp("-owner",argv[parm]) == 0)
        {
            /* do owner match also */
            parm++;
            if (parm == argc)
            {
                printf("No task name specified for '-owner' option\n");
                return (1);
            }

            itemOwner = FindTask(argv[parm]);
            if (itemOwner < 0)
            {
                printf("Could not find owner task '%s'\n",argv[parm]);
                return (1);
            }
        }
        else if (strcasecmp("-name",argv[parm]) == 0)
        {
            parm++;
            if (parm == argc)
            {
                printf("No item name specified for '-name' option\n");
                return (1);
            }

            itemName = argv[parm];
        }
        else if (strcasecmp("-full",argv[parm]) == 0)
        {
            full = TRUE;
        }
        else if (strcasecmp("-type",argv[parm]) == 0)
        {
            parm++;
            if (parm == argc)
            {
                printf("No type name specified for '-type' option\n");
                return (1);
            }

            if (!GetType(argv[parm],&subsysType,&itemType))
            {
                printf("Could not find item type '%s'\n",argv[parm]);
                return (1);
            }
        }
        else
        {
            /* interpret parameter as item number */
            Item t = (Item)(ConvertNum(argv[parm]));

            if (LookupItem(t))
            {
                DumpItem(t,TRUE);
                return (0);
            }

            if (!GetType(argv[parm],&subsysType,&itemType))
                itemName = argv[parm];
        }
    }

    printf("type         item #       address    pri open owner           name\n");
    for (i = 0; i < KernelBase->kb_MaxItem; i++)
    {
        ie  = GetItemEntryPtr(i);
        gen = (int)(ie->ie_ItemInfo & ITEM_GEN_MASK);
        itm = (Item)(gen+i);

        ip = (ItemNode *)LookupItem(itm);
        if (ip)
        {
            if (MatchItem(ip, itemType, subsysType, itemOwner, itemName))
            {
                DumpItem(itm,full);
            }
        }
    }

    return 0;
}
