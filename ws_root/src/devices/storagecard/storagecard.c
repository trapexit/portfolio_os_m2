/* @(#) storagecard.c 96/08/20 1.57 */

/* #define DEBUG */

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/device.h>
#include <kernel/dipir.h>
#include <kernel/driver.h>
#include <kernel/io.h>
#include <kernel/mem.h>
#include <kernel/operror.h>
#include <kernel/debug.h>
#include <kernel/kernel.h>
#include <kernel/super.h>
#include <dipir/hw.storagecard.h>
#include <device/microslot.h>
#include <device/storagecard.h>
#include <loader/loader3do.h>
#include <string.h>

#define MakeStorCardErr(svr,class,err) \
	MakeErr(ER_DEVC, MakeErrId('S','c'), (svr), ER_E_SSTM, (class), (err))

/* StorageCard commands */
#define	SCC_READ	0x00		/* Read data */
#define	SCC_WRITE	0x80		/* Write data */
#define	SCC_LEN		6		/* Num bytes in read or write cmd */
#define	MAX_SC_XFER	(64*1024)	/* Max transfer size in one cmd */

#ifndef BUILD_STRINGS
#undef DEBUG
#endif

#ifdef DEBUG
#define DBUG(x) printf x
#else
#define DBUG(x)
#endif

typedef struct CardState
{
    uint32		cs_Slot;	/* Slot number */
    Item		cs_SlotItem;	/* Slot device item */
    uint32		cs_UsageFlags;	/* Usage flags */
    uint32		cs_BlockSize;	/* Size of each block */
    uint32		cs_NumBlocks;	/* Number of blocks in card */
    uint8		cs_MemType;	/* Type of memory in card */
    uint8		cs_ChipRev;	/* Interface chip revision */
    uint8		cs_ChipMfg;	/* Interface chip manufacturer */
    uint8		cs_Pad;
} CardState;

#define	CS_MEMSIZE(cs)	((cs)->cs_BlockSize * (cs)->cs_NumBlocks)

typedef struct Seq
{
    MicroSlotSeq *	seq_Ptr;	/* Ptr to current MicroSlotSeq cmd */
    MicroSlotSeq	seq_Array[5];	/* Array of MicroSlotSeq cmds */
} Seq;

typedef struct IOReqAux
{
    uint32		iora_UssCmd;	/* Current cmd */
    uint32		iora_CurrAddr;	/* Current card address */
    uint8 *		iora_CurrBuf;	/* Current memory buffer */
    uint32		iora_CurrSize;	/* Size of current transfer */
    uint32		iora_VerifyAddr;  /* Card addr to verify when done */
    uint8 *		iora_VerifyBuf;   /* Buffer to verify when done */
    uint32		iora_VerifySize;  /* Size to verify when done */
    uint32		iora_Overhead;	/* Transfer overhead (cmd bytes) */
    Seq			iora_Seq;	/* Array of MicroSlotSeq cmds */
    uchar		iora_CmdBytes[SCC_LEN]; /* Cmd buffer */
} IOReqAux;



static Item StCardDeviceInit(Device* dev)
{
    CardState *cs = dev->dev_DriverData;
    HWResource hwr;
    HWResource_StorCard *hsc;
    Err err;

    DBUG(("StCardDeviceInit\n"));


    err = FindHWResource(dev->dev_HWResource, &hwr);
    if (err < 0)
	return err;
    hsc = (HWResource_StorCard *) &hwr.hwr_DeviceSpecific;

    cs->cs_Slot = hwr.hwr_Slot;
    cs->cs_BlockSize = (hsc->sc_MemType == SCMT_AT29C) ? hsc->sc_SectorSize : 1;
    cs->cs_NumBlocks = hsc->sc_MemSize / cs->cs_BlockSize;
    cs->cs_MemType = hsc->sc_MemType;
    cs->cs_ChipRev = hsc->sc_ChipRev;
    cs->cs_ChipMfg = hsc->sc_ChipMfg;
    cs->cs_UsageFlags = 0;
    if (cs->cs_MemType == SCMT_ROM)
	cs->cs_UsageFlags |= DS_USAGE_TRUEROM;
    if (hwr.hwr_Perms & HWR_WRITE_PROTECT)
	cs->cs_UsageFlags |= DS_USAGE_READONLY;
    if (hwr.hwr_Perms & HWR_UNSIGNED_OK)
	cs->cs_UsageFlags |= DS_USAGE_UNSIGNED_OK;

    /* Open the MicroSlot device. */
    cs->cs_SlotItem = OpenSlotDevice(dev->dev_HWResource);
    if (cs->cs_SlotItem < 0)
    {
	DBUG(("Cannot open microslot device:  %x\n", cs->cs_SlotItem));
	return cs->cs_SlotItem;
    }
    DBUG(("microslot device is item $%x\n", cs->cs_SlotItem));

    DBUG(("storagecard %d: blksize %d, blkcount: %d, flags %x, type %x %x.%x\n",
	cs->cs_Slot, cs->cs_BlockSize, cs->cs_NumBlocks, cs->cs_UsageFlags,
	cs->cs_MemType, cs->cs_ChipRev, cs->cs_ChipMfg));
    return dev->dev.n_Item;
}

static Err StCardDeviceDelete(Device* dev)
{
    CardState* cs;
    uint8 oldPriv;

    cs = dev->dev_DriverData;
    oldPriv = PromotePriv(CURRENTTASK);
    CloseItemAsTask(cs->cs_SlotItem, dev->dev.n_Owner);
    DemotePriv(CURRENTTASK, oldPriv);
    return 0;
}

static Err StCardChangeDeviceOwner(Device *dev, Item newOwner)
{
    CardState* cs;
    Err err;

    /* Transfer the "openness" of the slot device to the new owner. */
    cs = dev->dev_DriverData;
    err = OpenItemAsTask(cs->cs_SlotItem, NULL, newOwner);
    if (err < 0)
	return err;
    CloseItem(cs->cs_SlotItem);
    return 0;
}

static Item StCardCreateIO(IOReq* ior)
{
    CardState* cs;
    IOReq* slotior;
    Item iorItem;
    IOReqAux *aux;

    aux = SuperAllocMem(sizeof(IOReqAux), MEMTYPE_NORMAL);
    if (aux == NULL)
	return NOMEM;

    cs = ior->io_Dev->dev_DriverData;
    iorItem = CreateIOReq(0, 0, cs->cs_SlotItem, 0);
    if (iorItem < 0)
    {
	DBUG(("Cannot create microslot IOReq: %x\n", iorItem));
	SuperFreeMem(aux, sizeof(IOReqAux));
	return iorItem;
    }
    slotior = (IOReq*) LookupItem(iorItem);
    if (slotior == NULL)
    {
	DBUG(("Cannot lookup microslot IOReq\n"));
	SuperFreeMem(aux, sizeof(IOReqAux));
	DeleteItem(iorItem);
	return MakeKErr(ER_SEVERE,ER_C_STND,ER_SoftErr);
    }

    DBUG(("StCardCreateIO(%x): aux %x, lior %x\n", ior, aux, slotior));
    ior->io_Extension[0]= (int32) slotior;
    ior->io_Extension[1]= (int32) aux;
    return 0;
}

static Err StCardChangeIOReqOwner(IOReq* ior, Item newOwner)
{
    Item iorItem;

    DBUG(("StCardChangeIOReqOwner(%x): Changing owner to %x\n", ior, newOwner));
    if (ior->io_Extension[0] == 0)
	return BADITEM;
    iorItem= ((IOReq*)ior->io_Extension[0])->io.n_Item;
    if (iorItem < 0)
	return BADITEM;
    return SuperSetItemOwner(iorItem, newOwner);
}

static void StCardDeleteIO(IOReq* ior)
{
    IOReq* usior= (IOReq*) ior->io_Extension[0];
    IOReqAux* aux = (IOReqAux*) ior->io_Extension[1];

    DBUG(("StCardDeleteIO(%x) aux %x, lior %x\n", ior, aux, usior));
    if (usior) {
	DeleteItem(usior->io.n_Item);
        ior->io_Extension[0] = 0;
    }
    if (aux) {
	SuperFreeMem(aux, sizeof(IOReqAux));
        ior->io_Extension[1] = 0;
    }
}

static Item StCardDeviceOpen(Device* dev, Task* t)
{
    CardState* cs = dev->dev_DriverData;
    Err err;

    DBUG(("StCardDeviceOpen(%x) c=%d\n", dev, dev->dev.n_OpenCount));
    err = SuperInternalOpenItem(cs->cs_SlotItem, 0, t);
    if (err < 0)
	return err;
    return dev->dev.n_Item;
}

static Err StCardDeviceClose(Device* dev, Task* t)
{
    CardState* cs = dev->dev_DriverData;

    DBUG(("StCardDeviceClose(%x), c=%d\n", dev, dev->dev.n_OpenCount));
    SuperInternalCloseItem(cs->cs_SlotItem, t);
    return 0;
}

/*
 * Format a "standard" storagecard command:
 * 1-byte command, 3-byte address, 2-byte length.
 */
static void FormatSCCmd(uint8 *cmdBytes, uint32 sccCmd, uint32 addr, uint32 len)
{
    cmdBytes[0] = sccCmd;
    cmdBytes[1]= addr >> 16;
    cmdBytes[2]= addr >> 8;
    cmdBytes[3]= addr;
    cmdBytes[4]= (len - 1) >> 8;
    cmdBytes[5]= (len - 1);
}

static void InitSeq(Seq *seq)
{
    seq->seq_Ptr = seq->seq_Array;
}

static void AddSeqCmd(Seq *seq, uint32 ussCmd, void *buf, uint32 len)
{
   seq->seq_Ptr->uss_Cmd    = ussCmd;
   seq->seq_Ptr->uss_Buffer = buf;
   seq->seq_Ptr->uss_Len    = len;
   seq->seq_Ptr++;
}

static uint32 NumSeq(Seq *seq)
{
    return seq->seq_Ptr - seq->seq_Array;
}

static IOReq *IOCallback(IOReq *lior)
{
    IOReq *ior;
    IOReqAux *aux;
    uint32 len;
    CardState* cs;

    ior = (IOReq *) lior->io_Info.ioi_UserData;
    aux = (IOReqAux *) ior->io_Extension[1];
    cs = ior->io_Dev->dev_DriverData;
    DBUG(("IOCallback(%x) ior %x, aux %x\n", lior, ior, aux));

    if (lior->io_Error < 0)
    {
	DBUG(("Storagecard: microslot error %x\n", lior->io_Error));
	ior->io_Error = lior->io_Error;
	SuperCompleteIO(ior);
    	return NULL;
    }
    if (lior->io_Actual < aux->iora_Overhead)
    {
	DBUG(("Storagecard: microslot failed to send cmd\n"));
	ior->io_Error = MakeStorCardErr(ER_SEVERE,ER_C_STND,ER_DeviceError);
	SuperCompleteIO(ior);
	return NULL;
    }

    len = lior->io_Actual - aux->iora_Overhead;
    if (len > aux->iora_CurrSize)
    {
	/* It transferred more than we asked it to! */
	DBUG(("Storagecard: overrrun! %x > %x\n", len, aux->iora_CurrSize));
	len = aux->iora_CurrSize;
    }
    aux->iora_CurrAddr += len;
    aux->iora_CurrBuf  += len;
    aux->iora_CurrSize -= len;
    if (aux->iora_UssCmd != USSCMD_VERIFY)
        ior->io_Actual += len;

    DBUG(("IOCallback(%x): addr %x, buf %x, size %x, actual %x\n",
	    lior, aux->iora_CurrAddr, aux->iora_CurrBuf, aux->iora_CurrSize,
	    ior->io_Actual));

    /* Did we transfer everything? */
    if (aux->iora_CurrSize == 0)
    {
	if (aux->iora_VerifySize == 0)
	{
	    /* All done. */
	    SuperCompleteIO(ior);
	    return NULL;
	}
	/* Finished the transfer; need to verify it now. */
	aux->iora_UssCmd = USSCMD_VERIFY;
	aux->iora_CurrAddr = aux->iora_VerifyAddr;
	aux->iora_CurrBuf  = aux->iora_VerifyBuf;
	aux->iora_CurrSize = aux->iora_VerifySize;
	aux->iora_VerifySize = 0;
    }

    /*
     * Still have more to transfer.  Set up the next microslot I/O.
     * Format a 6-byte storagecard cmd.
     * Pass two USS cmds to the microslot driver:
     *	1. Write the 6-byte cmd.
     *	2. Read/write/verify the data for the cmd.
     */
    len = aux->iora_CurrSize;
    if (len > MAX_SC_XFER)
	len = MAX_SC_XFER;
    if (len % cs->cs_BlockSize)
    {
	DBUG(("IOCallback(%x): not aligned %x,%x\n", len, cs->cs_BlockSize));
	ior->io_Error = MakeStorCardErr(ER_SEVERE,ER_C_STND,ER_IOIncomplete);
	SuperCompleteIO(ior);
	return NULL;
    }
    FormatSCCmd(aux->iora_CmdBytes,
	(aux->iora_UssCmd == USSCMD_WRITE) ? SCC_WRITE : SCC_READ,
	aux->iora_CurrAddr, len);
    InitSeq(&aux->iora_Seq);
    AddSeqCmd(&aux->iora_Seq, USSCMD_WRITE,     aux->iora_CmdBytes, SCC_LEN);
    AddSeqCmd(&aux->iora_Seq, aux->iora_UssCmd, aux->iora_CurrBuf,  len);
    AddSeqCmd(&aux->iora_Seq, USSCMD_END,       0, 0);
    aux->iora_Overhead = SCC_LEN;
    DBUG(("IOCallback(%x): send %x,%x,%x,%x,%x,%x; then %x (%x,%x)\n", lior,
	aux->iora_CmdBytes[0], aux->iora_CmdBytes[1], aux->iora_CmdBytes[2],
	aux->iora_CmdBytes[3], aux->iora_CmdBytes[4], aux->iora_CmdBytes[5],
	aux->iora_UssCmd, aux->iora_CurrBuf, len));

    lior->io_Info.ioi_Command = USLOTCMD_SEQ;
    lior->io_Info.ioi_Send.iob_Buffer = aux->iora_Seq.seq_Array;
    lior->io_Info.ioi_Send.iob_Len = NumSeq(&aux->iora_Seq) * sizeof(MicroSlotSeq);
    lior->io_Info.ioi_Recv.iob_Len = 0;
    lior->io_Info.ioi_UserData = ior;
    lior->io_CallBack = IOCallback;
    return lior;
}

static Err CmdWrite(IOReq* ior)
{
    IOReq* lior;
    uint32 addr;
    Err err;
    IOReqAux *aux = (IOReqAux *) ior->io_Extension[1];
    CardState* cs = ior->io_Dev->dev_DriverData;

    DBUG(("Storagecard.Write: offset %x, buf %x, len %x\n",
	ior->io_Info.ioi_Offset,
	ior->io_Info.ioi_Send.iob_Buffer,
	ior->io_Info.ioi_Send.iob_Len));

    /* Is this card read-only? */
    if (cs->cs_UsageFlags & DS_USAGE_READONLY)
    {
	DBUG(("StorageCard is write-protected\n"));
	ior->io_Error = MakeStorCardErr(ER_SEVERE,ER_C_STND,ER_DeviceReadonly);
	return 1;
    }

    /* Attempt to write beyond card size? */
    addr = ior->io_Info.ioi_Offset * cs->cs_BlockSize;
    if (addr > CS_MEMSIZE(cs) ||
	addr + ior->io_Info.ioi_Send.iob_Len > CS_MEMSIZE(cs))
    {
	DBUG(("StorageCard.Write: addr %x + %x > %x * %x\n",
		addr, ior->io_Info.ioi_Send.iob_Len,
		cs->cs_NumBlocks, cs->cs_BlockSize));
	ior->io_Error = BADIOARG;
	return 1;
    }

    /* Transfer not block size multiple? */
    if (ior->io_Info.ioi_Send.iob_Len % cs->cs_BlockSize)
    {
	DBUG(("StorageCard.Write: len %x not aligned to %x\n",
		ior->io_Info.ioi_Send.iob_Len, cs->cs_BlockSize));
	ior->io_Error = BADIOARG;
	return 1;
    }

    if (ior->io_Info.ioi_Send.iob_Len == 0)
    {
	return 1;
    }

    aux->iora_UssCmd = USSCMD_WRITE;
    aux->iora_CurrAddr = addr;
    aux->iora_CurrBuf  = ior->io_Info.ioi_Send.iob_Buffer;
    aux->iora_CurrSize = ior->io_Info.ioi_Send.iob_Len;
    aux->iora_Overhead = 0;

    if (cs->cs_MemType == SCMT_AT29C)
    {
	/* Flash memory must be verified after being modified. */
	aux->iora_VerifyAddr = aux->iora_CurrAddr;
	aux->iora_VerifyBuf  = aux->iora_CurrBuf;
	aux->iora_VerifySize = aux->iora_CurrSize;
    } else
	aux->iora_VerifySize = 0;

    lior= (IOReq*) ior->io_Extension[0];
    lior->io_Error = 0;
    lior->io_Actual = 0;
    lior->io_Info.ioi_UserData = ior;
    ior->io_Flags &= ~IO_QUICK;
    /* Start the I/O by calling IOCallback */
    lior = IOCallback(lior);
    if (lior == NULL)
    {
	DBUG(("IOCallback returned NULL\n"));
	/* ior->io_Error was set by IOCallback. */
	return 1;
    }
    err = SuperInternalSendIO(lior);
    if (err < 0)
    {
	ior->io_Error = err;
	return 1;
    }
    return 0;
}

static Err CmdRead(IOReq* ior)
{
    IOReq* lior;
    uint32 addr;
    Err err;
    IOReqAux *aux = (IOReqAux *) ior->io_Extension[1];
    CardState* cs = ior->io_Dev->dev_DriverData;

    DBUG(("Storagecard.Read: offset %x, buf %x, len %x\n",
	ior->io_Info.ioi_Offset,
	ior->io_Info.ioi_Recv.iob_Buffer,
	ior->io_Info.ioi_Recv.iob_Len));

    /* Attempt to read beyond card size? */
    addr = ior->io_Info.ioi_Offset * cs->cs_BlockSize;
    if (addr > CS_MEMSIZE(cs) ||
	addr + ior->io_Info.ioi_Send.iob_Len > CS_MEMSIZE(cs))
    {
	DBUG(("StorageCard.Read: addr %x + %x > %x * %x\n",
		addr, ior->io_Info.ioi_Recv.iob_Len,
		cs->cs_NumBlocks, cs->cs_BlockSize));
	ior->io_Error = BADIOARG;
	return 1;
    }

    /* Transfer not block size multiple? */
    if (ior->io_Info.ioi_Recv.iob_Len % cs->cs_BlockSize)
    {
	DBUG(("StorageCard.Read: len %x not aligned to %x\n",
		ior->io_Info.ioi_Recv.iob_Len, cs->cs_BlockSize));
	ior->io_Error = BADIOARG;
	return 1;
    }

    if (ior->io_Info.ioi_Recv.iob_Len == 0)
    {
	return 1;
    }

    aux->iora_UssCmd = USSCMD_READ;
    aux->iora_CurrAddr = addr;
    aux->iora_CurrBuf  = ior->io_Info.ioi_Recv.iob_Buffer;
    aux->iora_CurrSize = ior->io_Info.ioi_Recv.iob_Len;
    aux->iora_Overhead = 0;
    aux->iora_VerifySize = 0;

    lior= (IOReq*) ior->io_Extension[0];
    lior->io_Error = 0;
    lior->io_Actual = 0;
    lior->io_Info.ioi_UserData = ior;
    ior->io_Flags &= ~IO_QUICK;
    /* Start the I/O by calling IOCallback */
    lior = IOCallback(lior);
    if (lior == NULL)
    {
	DBUG(("IOCallback returned NULL\n"));
	/* ior->io_Error was set by IOCallback. */
	return 1;
    }
    err = SuperInternalSendIO(lior);
    if (err < 0)
    {
	ior->io_Error = err;
	return 1;
    }
    return 0;
}

static Err CmdStatus(IOReq* ior)
{
    int32 len = ior->io_Info.ioi_Recv.iob_Len;
    CardState* cs = ior->io_Dev->dev_DriverData;
    StorageCardStatus stat;

    DBUG(("storagecard: CmdStatus\n"));
    if(len <= 0)
    {
	ior->io_Error = BADIOARG;
	return 1;
    }

    memset(&stat, 0, sizeof(stat));
    stat.sc_MemType = cs->cs_MemType;
    stat.sc_ChipRev = cs->cs_ChipRev;
    stat.sc_ChipMfg = cs->cs_ChipMfg;
    stat.sc_ds.ds_DriverIdentity = DI_OTHER;
    stat.sc_ds.ds_FamilyCode = DS_DEVTYPE_OTHER;
    stat.sc_ds.ds_MaximumStatusSize = sizeof(StorageCardStatus);
    stat.sc_ds.ds_DeviceBlockSize = cs->cs_BlockSize;
    stat.sc_ds.ds_DeviceBlockCount = cs->cs_NumBlocks;
    stat.sc_ds.ds_DeviceBlockStart = 0;
    stat.sc_ds.ds_DeviceUsageFlags =
		DS_USAGE_FILESYSTEM | DS_USAGE_PRIVACCESS |
		DS_USAGE_EXTERNAL | DS_USAGE_REMOVABLE |
		cs->cs_UsageFlags;

    if(len > sizeof(StorageCardStatus)) len= sizeof(StorageCardStatus);
    memcpy(ior->io_Info.ioi_Recv.iob_Buffer, &stat, len);
    ior->io_Actual= len;
    return 1;
}

static int32 CmdPreferFSType(IOReq *ior)
{
    uint32 fstype;

    if (ior->io_Info.ioi_Recv.iob_Len < sizeof(fstype))
    {
        ior->io_Error = MakeKErr(ER_SEVER,ER_C_STND,ER_IOIncomplete);
        return 1;
    }
    fstype = VOLUME_STRUCTURE_ACROBAT;
    memcpy(ior->io_Info.ioi_Recv.iob_Buffer, &fstype, sizeof(fstype));
    ior->io_Actual = sizeof(fstype);
    return 1;
}

static void StCardAbortIO(IOReq* ior)
{
    DBUG(("StCardAbortIO\n"));
    /* Tell the slot driver to abort it. */
    SuperInternalAbortIO((IOReq*)(ior->io_Extension[0]));
}


static DriverCmdTable CmdTable[]=
{
    { CMD_STATUS,	CmdStatus },
    { CMD_BLOCKREAD,	CmdRead },
    { CMD_BLOCKWRITE,	CmdWrite },
    { CMD_PREFER_FSTYPE,CmdPreferFSType }
};
#define NUM_CMDS (sizeof(CmdTable) / sizeof(CmdTable[0]))

int32 main(void)
{
    Item drvrItem;

    DBUG(("%s:main at $%x\n", STCRD_DEVICE_NAME, &main));

    DBUG(("Creating %s driver...\n", STCRD_DEVICE_NAME));
    drvrItem= CreateItemVA(MKNODEID(KERNELNODE, DRIVERNODE),
	TAG_ITEM_NAME,			STCRD_DEVICE_NAME,
	TAG_ITEM_PRI,			1,
	CREATEDRIVER_TAG_CMDTABLE,	CmdTable,
	CREATEDRIVER_TAG_NUMCMDS,	NUM_CMDS,
	CREATEDRIVER_TAG_CREATEDEV,	StCardDeviceInit,
	CREATEDRIVER_TAG_DELETEDEV,	StCardDeviceDelete,
	CREATEDRIVER_TAG_CHOWN_DEV,	StCardChangeDeviceOwner,
	CREATEDRIVER_TAG_ABORTIO,	StCardAbortIO,
	CREATEDRIVER_TAG_OPENDEV,	StCardDeviceOpen,
	CREATEDRIVER_TAG_CLOSEDEV,	StCardDeviceClose,
	CREATEDRIVER_TAG_CRIO,		StCardCreateIO,
	CREATEDRIVER_TAG_CHOWN_IO,	StCardChangeIOReqOwner,
	CREATEDRIVER_TAG_DLIO,		StCardDeleteIO,
	CREATEDRIVER_TAG_DEVICEDATASIZE,sizeof(CardState),
	CREATEDRIVER_TAG_MODULE,	FindCurrentModule(),
	TAG_END);
    if(drvrItem < 0)
    {
#ifdef DEBUG
	DBUG(("CreateItem failed:  "));
	PrintfSysErr(drvrItem);
#endif
    }
    return drvrItem;
}
