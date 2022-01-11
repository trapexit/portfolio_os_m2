/* @(#) iodebug.c 96/02/18 1.1 */

#ifdef BUILD_IODEBUG

/**
|||	AUTODOC -class Kernel -group Debugging -name ControlIODebug
|||	Controls what IODebug does and doesn't do.
|||
|||	  Synopsis
|||
|||	    Err ControlIODebug(bool active);
|||
|||	  Description
|||
|||	    This function lets you control various options that determine
|||	    what IODebug does.
|||
|||	    IODebug can do the following things:
|||
|||	      - When writing data to an I/O device, checks are done to ensure
|||	        that the buffer used for the I/O remains consistent and does
|||	        not get altered while the write operation occurs. This
|||	        guarantees that a client is not fiddling with data that is in
|||	        the process of being fetched by an I/O module.
|||
|||	      - When reading data from an I/O device, prevents any data
|||	        associated with the I/O request from being read until the I/O
|||	        request has completed. This guarantees that a client is not
|||	        reading data before it has been completely transferred.
|||
|||	    IODebug does its work by allocating temporary buffers and using
|||	    these for the I/O transactions. Doing so requires some extra memory,
|||	    and requires some extra CPU time to copy data between the buffers.
|||	    This can affect the performance
|||	    of a program, possibly causing some jerky animations, but should
|||	    never cause a crash, or trashed sound or graphics. If these
|||	    things happen, then the program has a bug.
|||
|||	  Arguments
|||
|||	    controlFlags
|||	        A set of bit flags controlling various IODebug options. See
|||	        below.
|||
|||	  Flags
|||
|||	    The control flags can be any of:
|||
|||	    IODEBUGF_PREVENT_PREREAD
|||	        Makes sure that client tasks don't attempt to read data from
|||	        an I/O buffer supplied using the IOInfo.ioi_Recv field while the
|||	        I/O operation is still in progress. This catches bugs where
|||	        data is fetched prematurely.
|||
|||	    IODEBUGF_PREVENT_POSTWRITE
|||	        Makes sure that client tasks don't attempt to write data to
|||	        an I/O buffer supplied using the IOInfo.ioi_Send field while the
|||	        I/O operation is still in progress. This catches bugs where
|||	        data is modified before it has been completely processed by an
|||	        I/O device.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V30.
|||
|||	  Associated Files
|||
|||	    <kernel/io.h>
|||
**/

#include <kernel/types.h>
#include <kernel/folio.h>
#include <kernel/io.h>
#include <kernel/device.h>
#include <kernel/kernel.h>
#include <kernel/list.h>
#include <kernel/mem.h>
#include <kernel/operror.h>
#include <kernel/super.h>
#include <kernel/internalf.h>
#include <string.h>
#include <stdio.h>


/*****************************************************************************/


typedef struct
{
    MinNode  tr_Link;

    void    *tr_TempBuf;
    void    *tr_RecvBuf;
    uint32   tr_RecvLen;

    uint32   tr_SendBufferCRC;
    bool     tr_CRCValid;
} Transaction;


/*****************************************************************************/


/* cache for unused transaction structures */
static List   freeTransactions = PREPLIST(freeTransactions);
static uint32 freeTransactionCount; /* count the number of cached structures   */

/* when a transaction is done, awaiting to be freed, it is put on this list */
static List   doneTransactions = PREPLIST(doneTransactions);

/* controls what work this code does */
static uint32 options;


/*****************************************************************************/


static uint32 CalcCRC(char *data, uint32 numBytes)
{
uint32 i, w, crc, index;
static uint32 crc32table[256] =
{
    0x00000000,0x04c11db7,0x09823b6e,0x0d4326d9,0x130476dc,0x17c56b6b,0x1a864db2,0x1e475005,
    0x2608edb8,0x22c9f00f,0x2f8ad6d6,0x2b4bcb61,0x350c9b64,0x31cd86d3,0x3c8ea00a,0x384fbdbd,
    0x4c11db70,0x48d0c6c7,0x4593e01e,0x4152fda9,0x5f15adac,0x5bd4b01b,0x569796c2,0x52568b75,
    0x6a1936c8,0x6ed82b7f,0x639b0da6,0x675a1011,0x791d4014,0x7ddc5da3,0x709f7b7a,0x745e66cd,
    0x9823b6e0,0x9ce2ab57,0x91a18d8e,0x95609039,0x8b27c03c,0x8fe6dd8b,0x82a5fb52,0x8664e6e5,
    0xbe2b5b58,0xbaea46ef,0xb7a96036,0xb3687d81,0xad2f2d84,0xa9ee3033,0xa4ad16ea,0xa06c0b5d,
    0xd4326d90,0xd0f37027,0xddb056fe,0xd9714b49,0xc7361b4c,0xc3f706fb,0xceb42022,0xca753d95,
    0xf23a8028,0xf6fb9d9f,0xfbb8bb46,0xff79a6f1,0xe13ef6f4,0xe5ffeb43,0xe8bccd9a,0xec7dd02d,
    0x34867077,0x30476dc0,0x3d044b19,0x39c556ae,0x278206ab,0x23431b1c,0x2e003dc5,0x2ac12072,
    0x128e9dcf,0x164f8078,0x1b0ca6a1,0x1fcdbb16,0x018aeb13,0x054bf6a4,0x0808d07d,0x0cc9cdca,
    0x7897ab07,0x7c56b6b0,0x71159069,0x75d48dde,0x6b93dddb,0x6f52c06c,0x6211e6b5,0x66d0fb02,
    0x5e9f46bf,0x5a5e5b08,0x571d7dd1,0x53dc6066,0x4d9b3063,0x495a2dd4,0x44190b0d,0x40d816ba,
    0xaca5c697,0xa864db20,0xa527fdf9,0xa1e6e04e,0xbfa1b04b,0xbb60adfc,0xb6238b25,0xb2e29692,
    0x8aad2b2f,0x8e6c3698,0x832f1041,0x87ee0df6,0x99a95df3,0x9d684044,0x902b669d,0x94ea7b2a,
    0xe0b41de7,0xe4750050,0xe9362689,0xedf73b3e,0xf3b06b3b,0xf771768c,0xfa325055,0xfef34de2,
    0xc6bcf05f,0xc27dede8,0xcf3ecb31,0xcbffd686,0xd5b88683,0xd1799b34,0xdc3abded,0xd8fba05a,
    0x690ce0ee,0x6dcdfd59,0x608edb80,0x644fc637,0x7a089632,0x7ec98b85,0x738aad5c,0x774bb0eb,
    0x4f040d56,0x4bc510e1,0x46863638,0x42472b8f,0x5c007b8a,0x58c1663d,0x558240e4,0x51435d53,
    0x251d3b9e,0x21dc2629,0x2c9f00f0,0x285e1d47,0x36194d42,0x32d850f5,0x3f9b762c,0x3b5a6b9b,
    0x0315d626,0x07d4cb91,0x0a97ed48,0x0e56f0ff,0x1011a0fa,0x14d0bd4d,0x19939b94,0x1d528623,
    0xf12f560e,0xf5ee4bb9,0xf8ad6d60,0xfc6c70d7,0xe22b20d2,0xe6ea3d65,0xeba91bbc,0xef68060b,
    0xd727bbb6,0xd3e6a601,0xdea580d8,0xda649d6f,0xc423cd6a,0xc0e2d0dd,0xcda1f604,0xc960ebb3,
    0xbd3e8d7e,0xb9ff90c9,0xb4bcb610,0xb07daba7,0xae3afba2,0xaafbe615,0xa7b8c0cc,0xa379dd7b,
    0x9b3660c6,0x9ff77d71,0x92b45ba8,0x9675461f,0x8832161a,0x8cf30bad,0x81b02d74,0x857130c3,
    0x5d8a9099,0x594b8d2e,0x5408abf7,0x50c9b640,0x4e8ee645,0x4a4ffbf2,0x470cdd2b,0x43cdc09c,
    0x7b827d21,0x7f436096,0x7200464f,0x76c15bf8,0x68860bfd,0x6c47164a,0x61043093,0x65c52d24,
    0x119b4be9,0x155a565e,0x18197087,0x1cd86d30,0x029f3d35,0x065e2082,0x0b1d065b,0x0fdc1bec,
    0x3793a651,0x3352bbe6,0x3e119d3f,0x3ad08088,0x2497d08d,0x2056cd3a,0x2d15ebe3,0x29d4f654,
    0xc5a92679,0xc1683bce,0xcc2b1d17,0xc8ea00a0,0xd6ad50a5,0xd26c4d12,0xdf2f6bcb,0xdbee767c,
    0xe3a1cbc1,0xe760d676,0xea23f0af,0xeee2ed18,0xf0a5bd1d,0xf464a0aa,0xf9278673,0xfde69bc4,
    0x89b8fd09,0x8d79e0be,0x803ac667,0x84fbdbd0,0x9abc8bd5,0x9e7d9662,0x933eb0bb,0x97ffad0c,
    0xafb010b1,0xab710d06,0xa6322bdf,0xa2f33668,0xbcb4666d,0xb8757bda,0xb5365d03,0xb1f740b4
};

    crc = 0xffffffff;
    for (i = 0; i < numBytes; i++)
    {
	w = (uint32)*data++;
	index = (crc >> 24) ^ w;
	crc = (crc << 8) ^ crc32table[index & 0x000000ff];
    }

    return (crc);
}


/*****************************************************************************/


static void FreeTransaction(Transaction *tr)
{
    SuperFreeMem(tr->tr_TempBuf, tr->tr_RecvLen);

    if (freeTransactionCount > 32)
    {
        /* don't cache more than 32 transactions */
        SuperFreeMem(tr, sizeof(Transaction));
    }
    else
    {
        freeTransactionCount++;
        AddHead(&freeTransactions, (Node *)tr);
    }
}


/*****************************************************************************/


static Transaction *AllocTransaction(IOReq *ior)
{
Transaction *tr;

    if (freeTransactionCount)
    {
        freeTransactionCount--;
        tr = (Transaction *)RemHead(&freeTransactions);
    }
    else
    {
        tr = (Transaction *)SuperAllocMem(sizeof(Transaction), MEMTYPE_NORMAL);
    }

    if (tr)
    {
        tr->tr_CRCValid = FALSE;
        tr->tr_RecvBuf  = ior->io_Info.ioi_Recv.iob_Buffer;
        tr->tr_RecvLen  = ior->io_Info.ioi_Recv.iob_Len;
        tr->tr_TempBuf  = NULL;

        if (options & IODEBUGF_PREVENT_PREREAD)
        {
            if (!tr->tr_RecvLen)
                return tr;

            tr->tr_TempBuf = SuperAllocMem(tr->tr_RecvLen, MEMTYPE_NORMAL);
            if (!tr->tr_TempBuf)
            {
                 FreeTransaction(tr);
                 return NULL;
            }
            ior->io_Info.ioi_Recv.iob_Buffer = tr->tr_TempBuf;
            ior->io_Transaction = tr;
        }
    }

    return tr;
}


/*****************************************************************************/


static void DisconnectTransaction(IOReq *ior, Transaction *tr)
{
    if (tr->tr_TempBuf)
    {
        /* copy from the temporary buffer to the real target buffer */
        memcpy(tr->tr_RecvBuf, tr->tr_TempBuf, tr->tr_RecvLen);
        ior->io_Info.ioi_Recv.iob_Buffer = tr->tr_RecvBuf;
    }

    if (tr->tr_CRCValid)
    {
        /* make sure the buffer being written out hasn't changed illegally */
        if (CalcCRC((char *)ior->io_Info.ioi_Send.iob_Buffer, ior->io_Info.ioi_Send.iob_Len) != tr->tr_SendBufferCRC)
        {
            printf("WARNING: Send buffer contents were altered before IO completed!\n");
            printf("         IOReq Item $%05x, buffer $%x\n",ior->io.n_Item,ior->io_Info.ioi_Send.iob_Buffer);
        }
    }
    ior->io_Transaction = NULL;
}


/*****************************************************************************/


/* cleanup any loose ends */
void CleanupDebuggedIOs(void)
{
Transaction *tr;
uint32       oldints;

    if (!IsListEmpty(&doneTransactions))
    {
        /* finish the cleanup of any complete transactions */
        while (TRUE)
        {
            oldints = Disable();
            tr = (Transaction *)RemHead(&doneTransactions);
            Enable(oldints);

            if (!tr)
                break;

            FreeTransaction(tr);
        }
        ScavengeMem();
    }
}


/*****************************************************************************/


/* do the work needed before dispatching a debugged IOReq */
Err internalSendDebuggedIO(IOReq *ior)
{
Transaction *tr;
Err          result;

    /* get a transaction structure for the IO request */
    tr = AllocTransaction(ior);
    if (!tr)
        return NOMEM;

    if (ior->io_Info.ioi_Send.iob_Len)
    {
        if (options & IODEBUGF_PREVENT_POSTWRITE)
        {
            /* calculate a CRC on the output buffer, so we can verify it
             * doesn't change during the write operation.
             */
            tr->tr_SendBufferCRC = CalcCRC((char *)ior->io_Info.ioi_Send.iob_Buffer,ior->io_Info.ioi_Send.iob_Len);
            tr->tr_CRCValid = TRUE;
        }
    }

    /* set the real buffer to garbage to avoid all confusion */
    if (tr->tr_RecvBuf)
        memset(tr->tr_RecvBuf, 0xf7, tr->tr_RecvLen);

    /* perform the IO for real */
    result = internalSendIO(ior);

    if (result != 0)
    {
        /* If the IO is not deferred, we must clean up now.
         * If the IO is deferred, clean up will happen when
         * CompleteIO() is called by the driver.
         */
        DisconnectTransaction(ior, tr);
        FreeTransaction(tr);
    }

    return result;
}


/*****************************************************************************/


/* finish the work needed to complete a debugged IOReq */
void CompleteDebuggedIO(IOReq *ior)
{
Transaction *tr;
uint32       oldints;

    tr = (Transaction *)ior->io_Transaction;
    if (tr)
    {
        /* We sync up the io request, and put the transaction on a list so
         * it will get freed the next time an IO operation is started. We can't
         * free it from here directly, since we might be called from interrupt
         * code.
         */
        DisconnectTransaction(ior, tr);
        oldints = Disable();
        AddHead(&doneTransactions, (Node *)tr);
        Enable(oldints);
    }
}


/*****************************************************************************/


Err ControlIODebug(uint32 controlFlags)
{
    if (controlFlags == 0)
    {
        KB_FIELD(kb_Flags) &= (~KB_IODEBUG);
        return 0;
    }

    KB_FIELD(kb_Flags) |= KB_IODEBUG;
    options = controlFlags;
    return 0;
}


/*****************************************************************************/


#else /* BUILD_IODEBUG */

#include <kernel/types.h>

Err ControlIODebug(uint32 controlFlags)
{
    TOUCH(controlFlags);
    return 0;
}

#endif
