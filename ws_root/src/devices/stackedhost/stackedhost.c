/* @(#) stackedhost.c 96/11/21 1.59 */

/* This pile of code supports three distinct devices which operate in a very
 * similar way.
 *
 *  1 - The hostfs device provides an interface to map a Portfolio file
 *      system onto a foreign remote file system.
 *
 *  2 - The hostcd device provides an interface to map a complete Portfolio
 *      file system onto a single remote file.
 *
 *  3 - The hostconsole device provides an interface to obtain command-line
 *      input from a host debugging environment.
 *
 * These devices need to communicate with the underlying host device in the
 * same way, which is why they were written to share most of their source
 * code.
 */

#include <kernel/io.h>
#include <kernel/driver.h>
#include <kernel/device.h>
#include <kernel/task.h>
#include <kernel/mem.h>
#include <kernel/kernel.h>
#include <kernel/operror.h>
#include <kernel/super.h>
#include <kernel/cache.h>
#include <kernel/sysinfo.h>
#include <hardware/debugger.h>
#include <hardware/PPCasm.h>
#include <dipir/hw.host.h>
#include <loader/loader3do.h>
#include <file/directory.h>
#include <file/filesystem.h>
#include <string.h>
#include <stdio.h>


#define DBUG(x)	     /* printf x */


/*****************************************************************************/


#define IMAGENAME      "cdrom.image"
#define BUFFER_SIZE    16384
#define NUM_DMABUFFERS 2


/*****************************************************************************/


/* This maps a Portfolio command to a host command, and defines
 * attributes for this command.
 *
 * When an IOReq is dispatched, a pointer to its associated
 * CommandMap structure is put in the io_Extension[1] field.
 */
typedef struct CommandMap
{
    uint32 cm_PortfolioCmd;      /* incoming command            */
    uint8  cm_HostCmd;           /* what it maps to on the host */
    uint8  cm_Flags;             /* how to handle this command  */
    uint16 cm_Pad;
} CommandMap;

/* for cm_Flags */
#define CM_NEWTOKEN          0x01  /* this command returns a new token     */
#define CM_WRITETOMEMORY     0x02  /* DMA will write to memory             */
#define CM_PRIVATEDMABUFFER  0x04  /* don't use a DMA buffer from the pool */

#define CMDMAP(ior) ((CommandMap *)ior->io_Extension[1])


/*****************************************************************************/


/* A DMA buffer used to receive data from the host, before it gets CPU copied
 * into the user's buffers. These only get used if the receive buffer the user
 * supplied isn't cache-aligned.
 */
typedef struct DMABuffer
{
    MinNode  db_Link;
    void    *db_Buffer;
    uint32   db_BufferSize;
} DMABuffer;


/*****************************************************************************/


/* DMA related things that are associated with an IOReq. An instance of
 * this structure is created for every IOReq, and a pointer to it is put
 * in the io_Extension[0] field of the IOReq.
 */
typedef struct DMAStuff
{
    DMABuffer *ds_DMABuffer;    /* buffer for unaligned CM_WRITETOMEMORY     */
    DMABuffer  ds_PrivateDMABuffer;
    IOReq     *ds_HostIO;       /* currently bound underlying host ioreq     */
    void      *ds_RealBuffer;   /* real receive buffer where stuff should go */
    IOBuf      ds_UserRecv;     /* original ioi_Recv buffer                  */
    int32      ds_UserOffset;   /* original ioi_Offset value                 */
    int32      ds_TotalActual;  /* io_Actual value for whole IO              */
    uint32     ds_BlockSize;    /* block size for I/O continuation           */

    /* unit-specific output data */
    union
    {
        HostFSReq      ds_fsSendData;
        HostCDReq      ds_cdSendData;
        HostConsoleReq ds_conSendData;
    } ds_sendinfo;

    /* unit-specific input data */
    union
    {
        HostFSReply      ds_fsRecvData;
        HostCDReply      ds_cdRecvData;
        HostConsoleReply ds_conRecvData;
    } ds_recvinfo;

    const CommandMap *ds_CommandMap;
    uint8             ds_HostUnit;
    bool              ds_HostIOActive;
} DMAStuff;

/* I hate the syntax of C unions, so hide 'em */
#define ds_FSSendData  ds_sendinfo.ds_fsSendData
#define ds_CDSendData  ds_sendinfo.ds_cdSendData
#define ds_ConSendData ds_sendinfo.ds_conSendData

/* Well, I still hate that syntax, so keep hiding 'em */
#define ds_FSRecvData  ds_recvinfo.ds_fsRecvData
#define ds_CDRecvData  ds_recvinfo.ds_cdRecvData
#define ds_ConRecvData ds_recvinfo.ds_conRecvData

#define DMASTUFF(ior) ((DMAStuff *)ior->io_Extension[0])


/*****************************************************************************/


/* ioreqs waiting to be dispatched */
static List iorqs = PREPLIST(iorqs);

/* available DMA buffers when user buffers aren't aligned right */
static List      buffers = PREPLIST(buffers);
static DMABuffer dmaBuffers[NUM_DMABUFFERS];

/* cache info gotten on startup */
static CacheInfo cacheInfo;

/* hostcd info gotten on startup */
static uint32    cdBlockSize;
static uint32    cdNumBlocks;
static void     *cdReferenceToken;


/*****************************************************************************/


static void AbortStackedHostIO(IOReq *ior)
{
    if (DMASTUFF(ior)->ds_HostIOActive)
    {
        /* try to abort the thing at the low-level */
        SuperInternalAbortIO(DMASTUFF(ior)->ds_HostIO);
    }
    else if (ior->io.n_Next)
    {
        /* if it's sitting in one of our queues, nuke it */
        RemNode((Node *)ior);
        ior->io.n_Next = NULL;
        ior->io_Error  = ABORTED;
        SuperCompleteIO(ior);
    }
}


/*****************************************************************************/


static IOReq *IOCallBack(IOReq *hostior);


/* Get another host IO to send out */
static IOReq *GetNextIO(void)
{
IOReq *hostior;
IOReq *ior;

    ior = (IOReq *)RemHead(&iorqs);
    if (ior)
    {
        ior->io.n_Next = NULL;  /* For AbortStackedHostIO()'s benefit */

        /* get the ioreq for the host device */
        hostior = DMASTUFF(ior)->ds_HostIO;
	DMASTUFF(ior)->ds_HostIOActive = TRUE;

        /* flush data being sent out */
        if (ior->io_Info.ioi_Send.iob_Len)
        {
            FlushDCache(0, ior->io_Info.ioi_Send.iob_Buffer,
                           ior->io_Info.ioi_Send.iob_Len);
        }

        if (CMDMAP(ior)->cm_Flags & CM_WRITETOMEMORY)
        {
            /* if recv buffer isn't cache-aligned, we must borrow a
             * temporary DMA buffer. Either we pull one out of the buffer
             * pool, or we use the private one associated with this IOReq.
             */
            if (((uint32)ior->io_Info.ioi_Recv.iob_Buffer & (cacheInfo.cinfo_DCacheLineSize-1))
             || ((uint32)ior->io_Info.ioi_Recv.iob_Len & (cacheInfo.cinfo_DCacheLineSize-1)))
            {
                if (CMDMAP(ior)->cm_Flags & CM_PRIVATEDMABUFFER)
                    DMASTUFF(ior)->ds_DMABuffer = &DMASTUFF(ior)->ds_PrivateDMABuffer;
                else
                    DMASTUFF(ior)->ds_DMABuffer = (DMABuffer *)RemHead(&buffers);

                if (!DMASTUFF(ior)->ds_DMABuffer)
                {
                    /* no buffers left, can't do the I/O right now */
                    AddHead(&iorqs,(Node *)ior);
                    return NULL;
                }

                /* We need to redirect this I/O to a temporary
                 * DMA buffer.
                 */
                DMASTUFF(ior)->ds_RealBuffer = ior->io_Info.ioi_Recv.iob_Buffer;

                ior->io_Info.ioi_Recv.iob_Buffer = DMASTUFF(ior)->ds_DMABuffer->db_Buffer;
                if (ior->io_Info.ioi_Recv.iob_Len > DMASTUFF(ior)->ds_DMABuffer->db_BufferSize)
                    ior->io_Info.ioi_Recv.iob_Len = DMASTUFF(ior)->ds_DMABuffer->db_BufferSize;

                if ((ior->io_Info.ioi_Command == HOSTFS_CMD_READENTRY)
                 || (ior->io_Info.ioi_Command == HOSTFS_CMD_READDIR))
                {
                DirectoryEntry *dirEntry;

                    /* compensate for lack of debugger support */
                    if (ior->io_Info.ioi_Recv.iob_Len >= sizeof(DirectoryEntry))
                    {
                        dirEntry = ior->io_Info.ioi_Recv.iob_Buffer;
                        dirEntry->de_Date.tv_Seconds      = 0;
                        dirEntry->de_Date.tv_Microseconds = 0;
                        FlushDCache(0, dirEntry, sizeof(DirectoryEntry));
                    }
                }
                else if (ior->io_Info.ioi_Command == HOSTFS_CMD_STATUS)
                {
                FileStatus *fileStatus;

                    /* compensate for lack of debugger support */
                    if (ior->io_Info.ioi_Recv.iob_Len >= sizeof(FileStatus))
                    {
                        fileStatus = ior->io_Info.ioi_Recv.iob_Buffer;
                        fileStatus->fs_Date.tv_Seconds      = 0;
                        fileStatus->fs_Date.tv_Microseconds = 0;
                        FlushDCache(0, fileStatus, sizeof(FileStatus));
                    }
                }
            }
        }

        if (ior->io_Info.ioi_Recv.iob_Len)
        {
            SuperInvalidateDCache(ior->io_Info.ioi_Recv.iob_Buffer,
                                  ior->io_Info.ioi_Recv.iob_Len);
        }

        DBUG(("GetNextIO: returning hostior $%x, ior $%x, command $%08x\n",hostior,ior,ior->io_Info.ioi_Command));
        DBUG(("           ioi_Send.iob_Buffer $%08x, ioi_Send.iob_Len %d\n",ior->io_Info.ioi_Send.iob_Buffer, ior->io_Info.ioi_Send.iob_Len));
        DBUG(("           ioi_Recv.iob_Buffer $%08x, ioi_Recv.iob_Len %d\n",ior->io_Info.ioi_Recv.iob_Buffer, ior->io_Info.ioi_Recv.iob_Len));

        hostior->io_Info.ioi_Command  = HOST_CMD_SEND;
        hostior->io_Info.ioi_CmdOptions = DMASTUFF(ior)->ds_HostUnit;
        hostior->io_Info.ioi_UserData = ior;
        hostior->io_CallBack          = IOCallBack;

        if (DMASTUFF(ior)->ds_HostUnit == HOST_FS_UNIT)
        {
            DMASTUFF(ior)->ds_FSSendData.hfs_Command        = CMDMAP(ior)->cm_HostCmd;
            DMASTUFF(ior)->ds_FSSendData.hfs_Send           = ior->io_Info.ioi_Send;
            DMASTUFF(ior)->ds_FSSendData.hfs_Recv           = ior->io_Info.ioi_Recv;
            DMASTUFF(ior)->ds_FSSendData.hfs_Offset         = ior->io_Info.ioi_Offset;
            DMASTUFF(ior)->ds_FSSendData.hfs_ReferenceToken = (void *)ior->io_Info.ioi_CmdOptions;

            hostior->io_Info.ioi_Send.iob_Buffer = &DMASTUFF(ior)->ds_FSSendData.hfs_Command;
            hostior->io_Info.ioi_Send.iob_Len    = DATASIZE;
            hostior->io_Info.ioi_Recv.iob_Buffer = &DMASTUFF(ior)->ds_FSRecvData.hfsr_Command;
            hostior->io_Info.ioi_Recv.iob_Len    = DATASIZE;
        }
        else if (DMASTUFF(ior)->ds_HostUnit == HOST_CD_UNIT)
        {
            DMASTUFF(ior)->ds_CDSendData.hcd_Command        = CMDMAP(ior)->cm_HostCmd;
            DMASTUFF(ior)->ds_CDSendData.hcd_Send           = ior->io_Info.ioi_Send;
            DMASTUFF(ior)->ds_CDSendData.hcd_Recv           = ior->io_Info.ioi_Recv;
            DMASTUFF(ior)->ds_CDSendData.hcd_Offset         = ior->io_Info.ioi_Offset;
            DMASTUFF(ior)->ds_CDSendData.hcd_ReferenceToken = cdReferenceToken;

            hostior->io_Info.ioi_Send.iob_Buffer = &DMASTUFF(ior)->ds_CDSendData.hcd_Command;
            hostior->io_Info.ioi_Send.iob_Len    = DATASIZE;
            hostior->io_Info.ioi_Recv.iob_Buffer = &DMASTUFF(ior)->ds_CDRecvData.hcdr_Command;
            hostior->io_Info.ioi_Recv.iob_Len    = DATASIZE;
        }
        else
        {
            DMASTUFF(ior)->ds_ConSendData.hcon_Command  = CMDMAP(ior)->cm_HostCmd;
            DMASTUFF(ior)->ds_ConSendData.hcon_Send     = ior->io_Info.ioi_Send;
            DMASTUFF(ior)->ds_ConSendData.hcon_Recv     = ior->io_Info.ioi_Recv;

            hostior->io_Info.ioi_Send.iob_Buffer = &DMASTUFF(ior)->ds_ConSendData.hcon_Command;
            hostior->io_Info.ioi_Send.iob_Len    = DATASIZE;
            hostior->io_Info.ioi_Recv.iob_Buffer = &DMASTUFF(ior)->ds_ConRecvData.hconr_Command;
            hostior->io_Info.ioi_Recv.iob_Len    = DATASIZE;
        }

        DBUG(("GetNextIO: returning $%x\n",hostior));

        return hostior;
    }

    return NULL;
}


/*****************************************************************************/


static void FinishIO(IOReq *ior)
{
    ior->io_Info.ioi_Recv   = DMASTUFF(ior)->ds_UserRecv;
    ior->io_Info.ioi_Offset = DMASTUFF(ior)->ds_UserOffset;
    ior->io_Actual          = DMASTUFF(ior)->ds_TotalActual;

    DBUG(("FinishIO: Completing IO $%x, io_Error %d, io_Actual %d\n",ior,ior->io_Error,ior->io_Actual));
    DBUG(("          ioi_CmdOptions $%x\n",ior->io_Info.ioi_CmdOptions));

    SuperCompleteIO(ior);
}


/*****************************************************************************/


static IOReq *IOCallBack(IOReq *hostior)
{
IOReq     *ior;
DMABuffer *db;

    ior = (IOReq *)hostior->io_Info.ioi_UserData;

    DMASTUFF(ior)->ds_HostIOActive = FALSE;

    if (DMASTUFF(ior)->ds_HostUnit == HOST_FS_UNIT)
    {
        ior->io_Error  = DMASTUFF(ior)->ds_FSRecvData.hfsr_Error;
        ior->io_Actual = DMASTUFF(ior)->ds_FSRecvData.hfsr_Actual;
#if 1
	/* Debugger returns INVALIDMETA instead of DIRNOTEMPTY
	 * when you try to delete a non-empty directory. */
	if (ior->io_Info.ioi_Command == HOSTFS_CMD_DELETEENTRY &&
	    ior->io_Error == FILE_ERR_INVALIDMETA)
		ior->io_Error = FILE_ERR_DIRNOTEMPTY;
#endif
    }
    else if (DMASTUFF(ior)->ds_HostUnit == HOST_CD_UNIT)
    {
        ior->io_Error  = DMASTUFF(ior)->ds_CDRecvData.hcdr_Error;
        ior->io_Actual = DMASTUFF(ior)->ds_CDRecvData.hcdr_Actual;
    }
    else
    {
        ior->io_Error  = DMASTUFF(ior)->ds_ConRecvData.hconr_Error;
        ior->io_Actual = DMASTUFF(ior)->ds_ConRecvData.hconr_Actual;
    }

    DBUG(("IOCallBack: called with hostior $%x, ior $%x\n",hostior,ior));

    if (CMDMAP(ior)->cm_Flags & CM_NEWTOKEN)
    {
        /* This command returned a new token, pass it on */
        ior->io_Info.ioi_CmdOptions = (uint32)DMASTUFF(ior)->ds_FSRecvData.hfsr_ReferenceToken;
    }

    DMASTUFF(ior)->ds_TotalActual += ior->io_Actual;

    if (CMDMAP(ior)->cm_Flags & CM_WRITETOMEMORY)
    {
        db = DMASTUFF(ior)->ds_DMABuffer;
        if (db)
        {
            /* This I/O was using a temporary buffer for its incoming
             * data. Copy from the temporary buffer to the real target.
             */

            DBUG(("IOCallBack: copying %d bytes from local buffer at\n",ior->io_Actual));
            DBUG(("            $%08x to $%08x\n",db->db_Buffer,DMASTUFF(ior)->ds_RealBuffer));

            /* make sure the data comes from memory */
            SuperInvalidateDCache(db->db_Buffer, ior->io_Actual);

            memcpy(DMASTUFF(ior)->ds_RealBuffer, db->db_Buffer, ior->io_Actual);

            if ((ior->io_Info.ioi_Command == HOSTFS_CMD_READENTRY)
             || (ior->io_Info.ioi_Command == HOSTFS_CMD_READDIR))
            {
            DirectoryEntry *dirEntry;

                /* compensate for lack of debugger support */
                if (ior->io_Info.ioi_Recv.iob_Len >= sizeof(DirectoryEntry))
                {
                    dirEntry = DMASTUFF(ior)->ds_RealBuffer;
                    dirEntry->de_Date.tv_Seconds      = 0;
                    dirEntry->de_Date.tv_Microseconds = 0;
                }
            }
            else if (ior->io_Info.ioi_Command == HOSTFS_CMD_STATUS)
            {
            FileStatus *fileStatus;

                /* compensate for lack of debugger support */
                if (ior->io_Info.ioi_Recv.iob_Len >= sizeof(FileStatus))
                {
                    fileStatus = DMASTUFF(ior)->ds_RealBuffer;
                    fileStatus->fs_Date.tv_Seconds      = 0;
                    fileStatus->fs_Date.tv_Microseconds = 0;
                }
            }

            if (db != &DMASTUFF(ior)->ds_PrivateDMABuffer)
                AddTail(&buffers,(Node *)db);

            DMASTUFF(ior)->ds_DMABuffer = NULL;
        }

        if ((ior->io_Actual != ior->io_Info.ioi_Recv.iob_Len)
         || (DMASTUFF(ior)->ds_TotalActual == DMASTUFF(ior)->ds_UserRecv.iob_Len))
        {
            /* If we didn't get all the data we asked for in the last
             * read, or if we've read as much as we asked for in the
             * first place, then we're done.
             */
            FinishIO(ior);
            hostior = GetNextIO();
        }
        else
        {
            /* This I/O was broken up into pieces and is not done yet.
             * Adjust things so the I/O will continue from where it left
             * off, and go reschedule the packet.
             */
            ior->io_Info.ioi_Offset         += (ior->io_Info.ioi_Recv.iob_Len / DMASTUFF(ior)->ds_BlockSize);
            ior->io_Info.ioi_Recv.iob_Buffer = (void *)((uint32)DMASTUFF(ior)->ds_UserRecv.iob_Buffer + DMASTUFF(ior)->ds_TotalActual);
            ior->io_Info.ioi_Recv.iob_Len    = DMASTUFF(ior)->ds_UserRecv.iob_Len - DMASTUFF(ior)->ds_TotalActual;
            AddHead(&iorqs,(Node *)ior);
            hostior = GetNextIO();
        }
    }
    else
    {
        /* DMA read operation is done, just return to caller */
        FinishIO(ior);
        hostior = GetNextIO();
    }

    DBUG(("IOCallBack: returning with hostior $%x, ior $%x\n",
	hostior, hostior ? hostior->io_Info.ioi_UserData : 0));

    return hostior;
}


/*****************************************************************************/


/* A new IO needs to be scheduled */
static void ScheduleIO(IOReq *ior)
{
uint32 oldints;
IOReq *hostior;

    oldints = Disable();
    InsertNodeFromTail(&iorqs,(Node *)ior);

    hostior = GetNextIO();
    if (hostior)
    {
        DBUG(("ScheduleIO: sending hostior $%x\n",hostior));
        SuperInternalSendIO(hostior);
    }

    Enable(oldints);
}


/*****************************************************************************/


static void CompleteSyncIO(IOReq *ior)
{
    SuperCompleteIO(ior);
}


/*****************************************************************************/


static int32 CmdStatus(IOReq *ior)
{
DeviceStatus stat;
int32        len;

    DBUG(("CmdStatus: current task %s\n",CURRENTTASK->t.n_Name));

    memset(&stat,0,sizeof(stat));
    stat.ds_DriverIdentity    = DI_OTHER;
    stat.ds_MaximumStatusSize = sizeof(DeviceStatus);

    if (DMASTUFF(ior)->ds_HostUnit == HOST_FS_UNIT)
    {
        stat.ds_FamilyCode        = DS_DEVTYPE_OTHER;
        stat.ds_DeviceUsageFlags  = DS_USAGE_HOSTFS | DS_USAGE_FILESYSTEM | DS_USAGE_EXTERNAL;
    }
    else if (DMASTUFF(ior)->ds_HostUnit == HOST_CD_UNIT)
    {
        stat.ds_FamilyCode        = DS_DEVTYPE_CDROM;
        stat.ds_DeviceUsageFlags  = DS_USAGE_FILESYSTEM | DS_USAGE_EXTERNAL;
	stat.ds_DeviceBlockSize   = cdBlockSize;
	stat.ds_DeviceBlockStart  = 0;
	stat.ds_DeviceBlockCount  = cdNumBlocks;
    }
    else
    {
        stat.ds_FamilyCode        = DS_DEVTYPE_OTHER;
        stat.ds_DeviceUsageFlags  = DS_USAGE_EXTERNAL;
    }

    len = ior->io_Info.ioi_Recv.iob_Len;
    if (len > sizeof(stat))
        len = sizeof(stat);

    memcpy(ior->io_Info.ioi_Recv.iob_Buffer,&stat,len);

    ior->io_Actual = len;

    CompleteSyncIO(ior);

    return 1;
}


/*****************************************************************************/


static int32 CmdSetBlockSize(IOReq *ior)
{
    if (DMASTUFF(ior)->ds_HostUnit != HOST_FS_UNIT)
    {
        ior->io_Error = BADCOMMAND;
        CompleteSyncIO(ior);
        return 1;
    }

    DMASTUFF(ior)->ds_BlockSize = ior->io_Info.ioi_Offset;
    CompleteSyncIO(ior);
    return 1;
}


/*****************************************************************************/


static const CommandMap hostFSCmdTable[] =
{
    {HOSTFS_CMD_OPENENTRY,   HOSTFS_REMOTECMD_OPENENTRY,   CM_NEWTOKEN},
    {HOSTFS_CMD_READENTRY,   HOSTFS_REMOTECMD_READENTRY,   CM_WRITETOMEMORY | CM_PRIVATEDMABUFFER},
    {HOSTFS_CMD_CLOSEENTRY,  HOSTFS_REMOTECMD_CLOSEENTRY,  0},
    {HOSTFS_CMD_BLOCKREAD,   HOSTFS_REMOTECMD_BLOCKREAD,   CM_WRITETOMEMORY},
    {HOSTFS_CMD_READDIR,     HOSTFS_REMOTECMD_READDIR,     CM_WRITETOMEMORY | CM_PRIVATEDMABUFFER},
    {HOSTFS_CMD_BLOCKWRITE,  HOSTFS_REMOTECMD_BLOCKWRITE,  0},
    {HOSTFS_CMD_ALLOCBLOCKS, HOSTFS_REMOTECMD_ALLOCBLOCKS, 0},
    {HOSTFS_CMD_CREATEFILE,  HOSTFS_REMOTECMD_CREATEFILE,  CM_NEWTOKEN},
    {HOSTFS_CMD_SETEOF,      HOSTFS_REMOTECMD_SETEOF,      0},
    {HOSTFS_CMD_SETTYPE,     HOSTFS_REMOTECMD_SETTYPE,     0},
    {HOSTFS_CMD_SETVERSION,  HOSTFS_REMOTECMD_SETVERSION,  0},
    {HOSTFS_CMD_SETDATE,     HOSTFS_REMOTECMD_SETDATE,     0},
    {HOSTFS_CMD_DELETEENTRY, HOSTFS_REMOTECMD_DELETEENTRY, 0},
    {HOSTFS_CMD_RENAMEENTRY, HOSTFS_REMOTECMD_RENAMEENTRY, 0},
    {HOSTFS_CMD_CREATEDIR,   HOSTFS_REMOTECMD_CREATEDIR,   CM_NEWTOKEN},
    {HOSTFS_CMD_STATUS,      HOSTFS_REMOTECMD_STATUS,      CM_WRITETOMEMORY | CM_PRIVATEDMABUFFER},
    {HOSTFS_CMD_FSSTAT,      HOSTFS_REMOTECMD_FSSTAT,      CM_WRITETOMEMORY | CM_PRIVATEDMABUFFER},
    {HOSTFS_CMD_MOUNTFS,     HOSTFS_REMOTECMD_MOUNTFS,     CM_NEWTOKEN | CM_WRITETOMEMORY | CM_PRIVATEDMABUFFER},
    {HOSTFS_CMD_DISMOUNTFS,  HOSTFS_REMOTECMD_DISMOUNTFS,  0},
    {0}
};

static const CommandMap hostCDCmdTable[] =
{
    {CMD_BLOCKREAD,  HOSTCD_REMOTECMD_BLOCKREAD, CM_WRITETOMEMORY},
    {0}
};

static const CommandMap hostConsoleCmdTable[] =
{
    {HOSTCONSOLE_CMD_GETCMDLINE,  HOSTCONSOLE_REMOTECMD_GETCMDLINE, CM_WRITETOMEMORY | CM_PRIVATEDMABUFFER},
    {0}
};


static int32 ProcessStackedHostIO(IOReq *ior)
{
uint32            i;
Err               err;
uint32            size;
const CommandMap *map;

    DBUG(("ProcessStackedHostIO: Entering with ior $%08x, command $%x\n",ior,ior->io_Info.ioi_Command));
    DBUG(("                      ioi_Send.iob_Buffer $%08x, ioi_Send.iob_Len %d\n",ior->io_Info.ioi_Send.iob_Buffer, ior->io_Info.ioi_Send.iob_Len));
    DBUG(("                      ioi_Recv.iob_Buffer $%08x, ioi_Recv.iob_Len %d\n",ior->io_Info.ioi_Recv.iob_Buffer, ior->io_Info.ioi_Recv.iob_Len));

    DMASTUFF(ior)->ds_UserRecv    = ior->io_Info.ioi_Recv;
    DMASTUFF(ior)->ds_UserOffset  = ior->io_Info.ioi_Offset;
    DMASTUFF(ior)->ds_TotalActual = 0;

    map = DMASTUFF(ior)->ds_CommandMap;

    err = BADCOMMAND;
    i   = 0;

    /* You know, a smarter programmer would do a binary search instead of a
     * linear search...
     */
    while (map[i].cm_PortfolioCmd)
    {
        if (map[i].cm_PortfolioCmd == ior->io_Info.ioi_Command)
        {
            ior->io_Flags        &= ~IO_QUICK;
            ior->io_Extension[1]  = (uint32)&map[i];

            if (map[i].cm_Flags & CM_PRIVATEDMABUFFER)
            {
                if (ior->io_Info.ioi_Recv.iob_Len > DMASTUFF(ior)->ds_PrivateDMABuffer.db_BufferSize)
                {
                    SuperFreeMem(DMASTUFF(ior)->ds_PrivateDMABuffer.db_Buffer,
                                 DMASTUFF(ior)->ds_PrivateDMABuffer.db_BufferSize);

                    size = ALLOC_ROUND(ior->io_Info.ioi_Recv.iob_Len, cacheInfo.cinfo_DCacheLineSize);
                    DMASTUFF(ior)->ds_PrivateDMABuffer.db_Buffer = SuperAllocMemAligned(size, MEMTYPE_NORMAL, cacheInfo.cinfo_DCacheLineSize);
                    if (!DMASTUFF(ior)->ds_PrivateDMABuffer.db_Buffer)
                    {
                        err = NOMEM;
                        break;
                    }

                    DMASTUFF(ior)->ds_PrivateDMABuffer.db_BufferSize = size;
                }
            }

            DBUG(("ProcessStackedHostIO: Scheduling command $%x\n",ior->io_Info.ioi_Command));
            ScheduleIO(ior);

            DBUG(("ProcessStackedHostIO: returning 0\n"));
            return 0;
        }

        i++;
    }

    switch (ior->io_Info.ioi_Command)
    {
        case HOSTFS_CMD_SETBLOCKSIZE: return CmdSetBlockSize(ior);
        case CMD_STATUS             : return CmdStatus(ior);
    }

    DBUG(("ProcessStackedHostIO: Received unknown command $%x\n",ior->io_Info.ioi_Command));

    ior->io_Error = err;
    CompleteSyncIO(ior);

    return 1;
}


/*****************************************************************************/


static int32 CreateStackedHostIO(IOReq *ior)
{
Item hostioreq;

    /* Allocate DMAStuff for this IOReq */
    ior->io_Extension[0] = (uint32)SuperAllocMem(sizeof(DMAStuff), MEMTYPE_FILL);
    if (!DMASTUFF(ior))
        return NOMEM;

    /* Allocate a host IOReq for this IOReq */
    hostioreq = CreateIOReq(0, 0, ior->io_Dev->dev_LowerDevice, 0);
    if (hostioreq < 0)
    {
	SuperFreeMem(DMASTUFF(ior),sizeof(DMAStuff));
	return hostioreq;
    }
    DMASTUFF(ior)->ds_HostIO = (IOReq *) LookupItem(hostioreq);

    if (strcasecmp(ior->io_Dev->dev.n_Name, "hostfs") == 0)
    {
        DMASTUFF(ior)->ds_HostUnit   = HOST_FS_UNIT;
        DMASTUFF(ior)->ds_BlockSize  = 8192;
        DMASTUFF(ior)->ds_CommandMap = hostFSCmdTable;
    }
    else if (strcasecmp(ior->io_Dev->dev.n_Name, "hostcd") == 0)
    {
        DMASTUFF(ior)->ds_HostUnit   = HOST_CD_UNIT;
        DMASTUFF(ior)->ds_BlockSize  = 2048;
        DMASTUFF(ior)->ds_CommandMap = hostCDCmdTable;
    }
    else
    {
        DMASTUFF(ior)->ds_HostUnit   = HOST_CONSOLE_UNIT;
        DMASTUFF(ior)->ds_CommandMap = hostConsoleCmdTable;
    }

    return 0;
}


/*****************************************************************************/


static Err DeleteStackedHostIO(IOReq *ior)
{
    SuperInternalDeleteItem(DMASTUFF(ior)->ds_HostIO->io.n_Item);
    SuperFreeMem(DMASTUFF(ior)->ds_PrivateDMABuffer.db_Buffer,
                 DMASTUFF(ior)->ds_PrivateDMABuffer.db_BufferSize);
    SuperFreeMem(DMASTUFF(ior),sizeof(DMAStuff));

    return 0;
}



/*****************************************************************************/


static Err SetOwnerStackedHostIO(IOReq *ior, Item newOwner)
{
IOReq *hostior;

    hostior = DMASTUFF(ior)->ds_HostIO;
    return SuperSetItemOwner(hostior->io.n_Item, newOwner);
}


/*****************************************************************************/


static Item OpenStackedHost(Device *dev, Task *t)
{
Item err;

    err = SuperInternalOpenItem(dev->dev_LowerDevice, 0, t);
    if (err < 0)
        return err;

    return dev->dev.n_Item;
}


/*****************************************************************************/


static Err CloseStackedHost(Device *dev, Task *t)
{
    return SuperInternalCloseItem(dev->dev_LowerDevice, t);
}


/*****************************************************************************/


static Item CreateHostCDDevice(Device *dev)
{
HWResource       res;
HWResource_Host *hres;
Err              result;

    /* Ick! We must search explicitly for the HWResource "HOSTCD".
     * The stackedhost/host architecture should be redesigned so
     * this isn't necessary.
     */

    res.hwr_InsertID = 0;
    while ((result = NextHWResource(&res)) >= 0)
	if (strcmp(res.hwr_Name, "HOSTCD") == 0)
	    break;
    if (result < 0)
	return result;

    hres             = (HWResource_Host *)res.hwr_DeviceSpecific;
    cdReferenceToken = hres->host_ReferenceToken;
    cdBlockSize      = hres->host_BlockSize;
    cdNumBlocks      = hres->host_NumBlocks;
    result           = dev->dev.n_Item;

    return result;
}


/*****************************************************************************/


static void FreeDMABuffers(void)
{
DMABuffer *db;

    while (db = (DMABuffer *)RemHead(&buffers))
        SuperFreeMem(db->db_Buffer, db->db_BufferSize);
}


/*****************************************************************************/


static Item CreateDMABuffers(void)
{
uint32     i;
DMABuffer *db;

    for (i = 0; i < NUM_DMABUFFERS; i++)
    {
        db = &dmaBuffers[i];
        AddTail(&buffers,(Node *)db);

        db->db_BufferSize = ALLOC_ROUND(BUFFER_SIZE, cacheInfo.cinfo_DCacheLineSize);
        db->db_Buffer = SuperAllocMemAligned(db->db_BufferSize, MEMTYPE_NORMAL, cacheInfo.cinfo_DCacheLineSize);
        if (!db->db_Buffer)
	{
	    FreeDMABuffers();
            return NOMEM;
	}
    }
    return 0;
}


/*****************************************************************************/


static Item CreateHostFSDriver(Driver *drv)
{
Err result;

    result = CreateDMABuffers();
    if (result < 0)
	return result;

    return drv->drv.n_Item;
}


/*****************************************************************************/


static Item DeleteHostFSDriver(Driver *drv)
{
    TOUCH(drv);

    FreeDMABuffers();
    return 0;
}


/*****************************************************************************/


int32 main(void)
{
Item hostFS;
Item hostConsole;
Item hostCD;

    GetCacheInfo(&cacheInfo, sizeof(cacheInfo));

    hostFS = CreateItemVA(MKNODEID(KERNELNODE,DRIVERNODE),
		TAG_ITEM_NAME,                "hostfs",
		CREATEDRIVER_TAG_CREATEDRV,   CreateHostFSDriver,
		CREATEDRIVER_TAG_DELETEDRV,   DeleteHostFSDriver,
		CREATEDRIVER_TAG_DISPATCH,    ProcessStackedHostIO,
		CREATEDRIVER_TAG_OPENDEV,     OpenStackedHost,
		CREATEDRIVER_TAG_CLOSEDEV,    CloseStackedHost,
		CREATEDRIVER_TAG_CHANGEOWNER, SetOwnerStackedHostIO,
		CREATEDRIVER_TAG_CRIO,        CreateStackedHostIO,
		CREATEDRIVER_TAG_DLIO,        DeleteStackedHostIO,
		CREATEDRIVER_TAG_ABORTIO,     AbortStackedHostIO,
		CREATEDRIVER_TAG_MODULE,      FindCurrentModule(),
		TAG_END);
    if (hostFS < 0)
        return hostFS;

    hostConsole = CreateItemVA(MKNODEID(KERNELNODE,DRIVERNODE),
		TAG_ITEM_NAME,                "hostconsole",
		CREATEDRIVER_TAG_DISPATCH,    ProcessStackedHostIO,
		CREATEDRIVER_TAG_OPENDEV,     OpenStackedHost,
		CREATEDRIVER_TAG_CLOSEDEV,    CloseStackedHost,
		CREATEDRIVER_TAG_CHANGEOWNER, SetOwnerStackedHostIO,
		CREATEDRIVER_TAG_CRIO,        CreateStackedHostIO,
		CREATEDRIVER_TAG_DLIO,        DeleteStackedHostIO,
		CREATEDRIVER_TAG_ABORTIO,     AbortStackedHostIO,
		CREATEDRIVER_TAG_MODULE,      FindCurrentModule(),
		TAG_END);
    if (hostConsole < 0)
    {
        DeleteItem(hostFS);
        return hostConsole;
    }

    hostCD = CreateItemVA(MKNODEID(KERNELNODE,DRIVERNODE),
		TAG_ITEM_NAME,                "hostcd",
		CREATEDRIVER_TAG_DISPATCH,    ProcessStackedHostIO,
		CREATEDRIVER_TAG_CREATEDEV,   CreateHostCDDevice,
		CREATEDRIVER_TAG_OPENDEV,     OpenStackedHost,
		CREATEDRIVER_TAG_CLOSEDEV,    CloseStackedHost,
		CREATEDRIVER_TAG_CHANGEOWNER, SetOwnerStackedHostIO,
		CREATEDRIVER_TAG_CRIO,        CreateStackedHostIO,
		CREATEDRIVER_TAG_DLIO,        DeleteStackedHostIO,
		CREATEDRIVER_TAG_ABORTIO,     AbortStackedHostIO,
		CREATEDRIVER_TAG_MODULE,      FindCurrentModule(),
		TAG_END);

    /* we don't care if the hostcd driver couldn't be created */

    /* prevent the silly things from going away, since we don't handle unloading
     * of this driver correctly, due to its funky 3-drivers-in-1-module
     * architecture.
     */
    OpenItemAsTask(hostFS,      NULL, KernelBase->kb_OperatorTask);
    OpenItemAsTask(hostConsole, NULL, KernelBase->kb_OperatorTask);
    OpenItemAsTask(hostCD,      NULL, KernelBase->kb_OperatorTask);

    return 0;
}
