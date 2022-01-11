/* @(#) EventPort.c 96/07/17 1.11 */

/* The driver for the Event Port pseudo-device. */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/list.h>
#include <kernel/listmacros.h>
#include <kernel/device.h>
#include <kernel/driver.h>
#include <kernel/io.h>
#include <kernel/operror.h>
#include <kernel/super.h>
#include <loader/loader3do.h>
#include <misc/event.h>
#include <stdio.h>
#include <string.h>


/*****************************************************************************/


#define DBUG(x) /* printf x */


/*****************************************************************************/


static List readerList = PREPLIST(readerList);


/*****************************************************************************/


static void EventPortAbortIO(IOReq *ior)
{
    REMOVENODE((Node *)ior);
    ior->io_Error = ABORTED;
    SuperCompleteIO(ior);
}


/*****************************************************************************/


static int32 EventPortEventForwarder(void *foo)
{
int32       len;
uint32      oldints;
IOReq      *reader;
TimeValVBL  tvbl;

    oldints = Disable();
    reader = (IOReq *) RemHead(&readerList);
    Enable(oldints);

    if (reader == NULL)
        return MAKEERR(ER_DEVC,ER_CPORT,ER_SEVER,ER_E_SSTM,ER_C_STND,ER_EndOfMedium);

    len = ((EventFrame *) foo)->ef_ByteCount;
    len = (len + 3) & ~ 3;
    if (len > reader->io_Info.ioi_Recv.iob_Len)
    {
        len = reader->io_Info.ioi_Recv.iob_Len;
        reader->io_Error = MAKEERR(ER_DEVC,ER_CPORT,ER_SEVER,ER_E_SSTM,ER_C_STND,ER_IOIncomplete);
    }
    memcpy(reader->io_Info.ioi_Recv.iob_Buffer, foo, len);
    reader->io_Actual = len;
    SampleSystemTimeVBL(&tvbl);
    reader->io_Info.ioi_Offset = (uint32)tvbl;
    DBUG(("Sending event\n"));
    SuperCompleteIO(reader);

    return len;
}


/*****************************************************************************/


static Item EventPortInit(Driver *drv)
{
    RegisterReportEvent(EventPortEventForwarder);
    return drv->drv.n_Item;
}


/*****************************************************************************/


static int32 CmdReadEvent(IOReq *ior)
{
uint32 oldints;

    ior->io_Flags &= ~IO_QUICK;
    oldints = Disable();
    ADDTAIL(&readerList, (Node *) ior);
    Enable(oldints);
    DBUG(("Event reader queued\n"));
    return 0;
}


/*****************************************************************************/


static int32 CmdWriteEvent(IOReq *ior)
{
uint32  oldints;
uint32  len;
IOReq  *reader;

    oldints = Disable();
    reader = (IOReq *) RemHead(&readerList);
    Enable(oldints);

    if (reader == NULL)
    {
        ior->io_Error = MAKEERR(ER_DEVC,ER_CPORT,ER_SEVER,ER_E_SSTM,ER_C_STND,ER_EndOfMedium);
        return 1;
    }

    len = ior->io_Info.ioi_Send.iob_Len;
    if (len > reader->io_Info.ioi_Recv.iob_Len)
    {
        len = reader->io_Info.ioi_Recv.iob_Len;
        ior->io_Error = reader->io_Error = MAKEERR(ER_DEVC,ER_CPORT,ER_SEVER,ER_E_SSTM,ER_C_STND,ER_IOIncomplete);
    }

    memcpy(reader->io_Info.ioi_Recv.iob_Buffer, ior->io_Info.ioi_Recv.iob_Buffer, len);
    ior->io_Actual = reader->io_Actual = len;
    SuperCompleteIO(reader);

    return 1;
}


/*****************************************************************************/


static const DriverCmdTable cmdTable[] =
{
    {CPORT_CMD_WRITEEVENT, CmdWriteEvent},
    {CPORT_CMD_READEVENT,  CmdReadEvent},
};

int32 main(void)
{
    return CreateItemVA(MKNODEID(KERNELNODE,DRIVERNODE),
                        TAG_ITEM_NAME,                  "eventport",
                        CREATEDRIVER_TAG_CMDTABLE,      cmdTable,
                        CREATEDRIVER_TAG_NUMCMDS,       sizeof(cmdTable) / sizeof(cmdTable[0]),
                        CREATEDRIVER_TAG_CREATEDRV,     EventPortInit,
                        CREATEDRIVER_TAG_ABORTIO,       EventPortAbortIO,
                        CREATEDRIVER_TAG_MODULE,        FindCurrentModule(),
                        TAG_END);
}
