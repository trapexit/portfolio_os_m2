/*
 * @(#) dma.c 96/10/18 1.13
 *
 * Routines to do DMA.
 */

#include <kernel/types.h>
#include <kernel/kernelnodes.h>
#include <kernel/debug.h>
#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <kernel/super.h>
#include <kernel/interrupts.h>
#include <kernel/cache.h>
#include <kernel/internalf.h>
#include <hardware/PPCasm.h>
#include <hardware/cde.h>
#include <device/dma.h>
#include <string.h>

#define	Max(a,b)	(((a) > (b)) ? (a) : (b))

#ifdef BUILD_PCDEBUGGER
#undef BUILD_DEBUGGER 
#endif

/* #define DEBUG */
#ifdef DEBUG
#define	DBUG(x) printf x
#else
#define	DBUG(x)
#endif

#define	NUM_DMA_CHANNELS	2	/* Number of DMA channels */
#define	MIN_DMA_SIZE		32	/* Don't use DMA if xfer < this size */

/* Structure representing a DMA request */
typedef struct DMAReq
{
	Node	n;			/* Node for queuing */
	uint32	dma_DMAAddr;		/* DMA address */
	void *	dma_MemAddr;		/* Memory address */
	uint32	dma_Len;		/* Length of transfer */
	uint32	dma_Flags;		/* Flags */
	uint32	dma_Channel;		/* DMA channel */
	void	(*dma_Callback)(void *arg, Err err); /* Callback when done */
	void *	dma_CallbackArg;	/* Argument to callback function */
} DMAReq;

/* Structure telling how to report DMA completion */
typedef struct DMAComp
{
	uint32	dmc_Flags;		/* Flags; see below */
	void	(*dmc_Callback)(void *arg, Err err); /* Callback when done */
	void *	dmc_CallbackArg;	/* Argument to callback function */
	void *	dmc_TailMem;		/* Memory addr for tail memcpy */
	uint32	dmc_TailDMA;		/* DMA addr for tail memcpy */
	uint32	dmc_TailLen;		/* Length for tail memcpy */
	uint32	dmc_DMAFlags;		/* Flags from dma_Flags */
} DMAComp;

/* Bits in dmc_Flags */
#define	DMA_BUSY		0x00000001
#define	DMA_RESERVED		0x00000002

/* DMA registers */

static const uint32 _DMAControlReg[] = {
	CDE_DMA1_CNTL, CDE_DMA2_CNTL
};
static const uint32 _DMACDEAddr[] = {
	CDE_DMA1_CBAD, CDE_DMA2_CBAD
};
static const uint32 _DMAPowerBusAddr[] = {
	CDE_DMA1_CPAD, CDE_DMA2_CPAD
};
static const uint32 _DMACountReg[] = {
	CDE_DMA1_CCNT, CDE_DMA2_CCNT
};
static const uint32 _DMAInterruptNumber[] = {
	INT_CDE_DMAC1, INT_CDE_DMAC2
};

#define	DMAControlReg(dchan)	_DMAControlReg		[dchan]
#define	DMACDEAddr(dchan)	_DMACDEAddr		[dchan]
#define	DMAPowerBusAddr(dchan)	_DMAPowerBusAddr	[dchan]
#define	DMACountReg(dchan)	_DMACountReg		[dchan]
#define	DMAInterruptNumber(dchan) _DMAInterruptNumber	[dchan]


/* Queue of pending DMA requests */
static List dmaQueue = PREPLIST(dmaQueue);
/* DMA operations currently in progress (per DMA channel) */
static DMAComp dmaWorking[NUM_DMA_CHANNELS];
/* Interrupt handlers for DMA completion interrupts */
static Item DMAfirq[NUM_DMA_CHANNELS];

static uint32 cacheLineSize;

#ifdef BUILD_DEBUGGER
static uint32 sysRamStart;
static uint32 sysRamEnd;
#endif

/*****************************************************************************
  Is DMA hardware currently active?
*/
static bool
DMAActive(uint32 dchan)
{
	if (CDE_READ(KernelBase->kb_CDEBase, DMAControlReg(dchan)) &
			CDE_DMA_CURR_VALID)
		return TRUE;
	return FALSE;
}

/*****************************************************************************
  Find a free DMA channel.
*/
static uint32
FindFreeDMAChannel(void)
{
	uint32 dchan;

	for (dchan = 0;  dchan < NUM_DMA_CHANNELS;  dchan++)
	{
		if ((dmaWorking[dchan].dmc_Flags &
			(DMA_BUSY|DMA_RESERVED)) == 0)
		{
#ifdef BUILD_PARANOIA
			if (DMAActive(dchan))
			{
				printf("*** DMA channel %d free but active!\n",
					dchan);
				continue;
			}
#endif
			return dchan;
		}
	}
	/* No channels available. */
	return DMA_CHANNEL_ANY;
}

/**
|||	AUTODOC -private -class Kernel -group Device-drivers -name AllocDMAChannel
|||	Allocate a DMA channel.
|||
|||	  Synopsis
|||
|||	    uint32 AllocDMAChannel(uint32 flags)
|||
|||	  Description
|||
|||	    Allocates an available DMA channel.  The channel becomes
|||	    unavailable to the system until FreeDMAChannel() is called.
|||
|||	  Arguments
|||
|||	    flags
|||	        If DMA_SYNC is set, and there are no DMA channels
|||	        available, the function waits until a channel
|||	        becomes available.
|||
|||	  Return Value
|||
|||	    Returns the channel number of the allocated DMA
|||	    channel, or DMA_CHANNEL_ANY if no channel was available.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V28.
|||
|||	  Notes
|||
|||	    Normally it is not necessary to call AllocDMAChannel().
|||	    A client can simply call StartDMA() with the dchan
|||	    argument set to DMA_CHANNEL_ANY, and StartDMA() will
|||	    allocate a channel itself.
|||
|||	  Associated Files
|||
|||	    <device/dma.h>
|||
|||	  See Also
|||
|||	    FreeDMAChannel(), StartDMA()
|||
**/

/*****************************************************************************
  Allocate a free DMA channel.
*/
uint32
AllocDMAChannel(uint32 flags)
{
	uint32 dchan;
	uint32 oldints;

	InitDMA();
Again:
	oldints = Disable();
	dchan = FindFreeDMAChannel();
	if (dchan != DMA_CHANNEL_ANY)
	{
		/* Found a free channel. */
		dmaWorking[dchan].dmc_Flags |= DMA_RESERVED;
		Enable(oldints);
		return dchan;
	}
	Enable(oldints);

	/* If the DMA_SYNC flag is set, wait for a channel to become free. */
	if (flags & DMA_SYNC)
	{
		goto Again;
	}
	/* No channels available. */
	return DMA_CHANNEL_ANY;
}

/**
|||	AUTODOC -private -class Kernel -group Device-drivers -name FreeDMAChannel
|||	Free a DMA channel.
|||
|||	  Synopsis
|||
|||	    Err FreeDMAChannel(uint32 dchan)
|||
|||	  Description
|||
|||	    Frees a DMA channel previously allocated by AllocDMAChannel().
|||
|||	  Arguments
|||
|||	    dchan
|||	        The channel number of the DMA channel to be freed.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code on
|||	    failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V28.
|||
|||	  Associated Files
|||
|||	    <device/dma.h>
|||
|||	  See Also
|||
|||	    AllocDMAChannel(), StartDMA()
|||
**/

/*****************************************************************************
 Free an allocated DMA channel.
*/
Err
FreeDMAChannel(uint32 dchan)
{
	uint32 oldints;
	Err err = 0;

	InitDMA();
	oldints = Disable();
	if (dmaWorking[dchan].dmc_Flags & DMA_RESERVED)
		dmaWorking[dchan].dmc_Flags &= ~DMA_RESERVED;
	else
		err = MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_ItemNotOpen);
	Enable(oldints);
	return err;
}

/*****************************************************************************
  Queue a DMA request for later.
*/
static void
QueueDMA(uint32 dmaAddr, void *memAddr, uint32 len, uint32 flags, uint32 dchan,
	void (*Callback)(void *arg, Err err), void *callbackArg)
{
	DMAReq *dma;

	DBUG(("QueueDMA(%x,%x,%x,%x,%x)\n", dmaAddr, memAddr, len, flags, dchan));
	dma = (DMAReq *)
		SuperAllocMem(sizeof(DMAReq), MEMTYPE_ANY|MEMTYPE_FILL);
	dma->dma_DMAAddr = dmaAddr;
	dma->dma_MemAddr = memAddr;
	dma->dma_Len = len;
	dma->dma_Flags = flags;
	dma->dma_Channel = dchan;
	dma->dma_Callback = Callback;
	dma->dma_CallbackArg = callbackArg;
	InsertNodeFromTail(&dmaQueue, (Node *)dma);
}

#ifdef BUILD_PCDEBUGGER
static DMAComp saveddmaWorking[NUM_DMA_CHANNELS];
static void InitDMAFirq(void);
#endif

/*****************************************************************************
*/
static void (*putcfunc)(char a);    /*save original PutC just in case*/
static void
CpuCopy(void *memAddr, uint32 dmaAddr, uint32 len, uint32 flags)
{
	void *dst;
	void *src;

	if (flags & DMA_WRITE)
	{
		src = memAddr;
		dst = (void *) dmaAddr;
	} else
	{
		src = (void *) dmaAddr;
		dst = memAddr;
	}
	
	if (flags & DMA_32BIT)
	{
		uint32 *src32 = src;
		uint32 *dst32 = dst;
		while (len > 0)
		{
			*dst32++ = *src32++;
			len -= sizeof(uint32);
		}
	} else if (flags & DMA_16BIT)
	{
		uint16 *src16 = src;
		uint16 *dst16 = dst;
		while (len > 0)
		{
			*dst16++ = *src16++;
			len -= sizeof(uint16);
		}
	} else /* DMA_8BIT */
	{
		uint8 *src8 = src;
		uint8 *dst8 = dst;
		while (len > 0)
		{
			*dst8++ = *src8++;
			len -= sizeof(uint8);
		}
	}
}

/**
|||	AUTODOC -private -class Kernel -group Device-drivers -name StartDMA
|||	Begin a DMA transfer.
|||
|||	  Synopsis
|||
|||	    Err StartDMA(uint32 dmaAddr, void *memAddr, uint32 len, uint32 flags, uint32 dchan, void (*Callback)(void *arg, Err err), void *callbackArg)
|||
|||	  Description
|||
|||	    Begins a DMA transfer.  If flags has the DMA_READ bit set,
|||	    the transfer is from the DMA address dmaAddr to the memory
|||	    address memAddr.  If flags has the DMA_WRITE bit set,
|||	    the transfer is from the memory address memAddr to the DMA
|||	    address dmaAddr.  len is the length of the transfer in bytes.
|||	    If dchan is DMA_CHANNEL_ANY, StartDMA() will allocate a
|||	    DMA channel for the transfer.  Otherwise, dchan must be a
|||	    channel number previously returned from AllocDMAChannel().
|||
|||	    When the transfer is complete, the callback function
|||	    Callback is called, and passed the parameter callbackArg.
|||	    The second parameter to the callback function is an error
|||	    code indicating whether the transfer completed successfully.
|||	    However, if Callback is NULL, no callback function is called.
|||	    If flags has the DMA_SYNC bit set, StartDMA() does not
|||	    return until the transfer is complete.
|||
|||	  Arguments
|||
|||	    dmaAddr
|||	        DMA address (source address if DMA_READ; destination
|||	        address if DMA_WRITE).
|||	    memAddr
|||	        Memory address (destination address if DMA_READ;
|||	        source address if DMA_WRITE).
|||	    len
|||	        Length of the transfer, in bytes.
|||	    flags
|||	        Various bits: DMA_READ, DMA_WRITE, DMA_SYNC.
|||	    dchan
|||	        The channel number of the DMA channel to be used
|||	        for the transfer, or DMA_CHANNEL_ANY to request
|||	        StartDMA() to allocate a channel.
|||	    Callback
|||	        Pointer to the callback function.
|||	    callbackArg
|||	        Argument to be passed to the callback function.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code on
|||	    failure.  Note that on return, the transfer may not
|||	    yet be complete (unless DMA_SYNC was specified).
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V28.
|||
|||	  Notes
|||
|||	    DMA_SYNC should be specified only if a DMA channel is
|||	    also specified (that is, if dchan is not DMA_CHANNEL_ANY).
|||
|||	    The callback function may not call StartDMA().
|||
|||	  Associated Files
|||
|||	    <device/dma.h>
|||
|||	  See Also
|||
|||	    AllocDMAChannel(), FreeDMAChannel(), AbortDMA()
|||
**/

/*****************************************************************************
  Begin a DMA operation.
  Callback may NOT call StartDMA.
*/
Err
StartDMA(uint32 dmaAddr, void *vMemAddr, uint32 len, uint32 flags, uint32 dchan,
	void (*Callback)(void *arg, Err err), void *callbackArg)
{
	uint32 n;
	uint32 oldints;
	uint8 *memAddr = vMemAddr;

	static void DMACompletion(uint32 dchan);

	DBUG(("StartDMA(%x,%x,%x,%x)\n", dmaAddr, memAddr, len, flags));

	if ((flags & DMA_16BIT) && 
	    ((dmaAddr % sizeof(uint16)) || (len % sizeof(uint16))))
	{
		return PARAMERROR;
	}

	if ((flags & DMA_32BIT) && 
	    ((dmaAddr % sizeof(uint32)) || (len % sizeof(uint32))))
	{
		return PARAMERROR;
	}

	InitDMA();
#ifdef BUILD_PCDEBUGGER
	if ((flags & DMA_SYNC)==0) InitDMAFirq();
#endif
	oldints = Disable();

	/* Find a DMA channel. */
	if (dchan == DMA_CHANNEL_ANY)
		dchan = FindFreeDMAChannel();
	if (dchan == DMA_CHANNEL_ANY)
	{
		/* No free channels; queue it for later. */
		if (flags & DMA_SYNC)
		{
			Enable(oldints);
			return MAKEKERR(ER_SEVER,ER_C_STND,ER_NoHardware);
		}
		QueueDMA(dmaAddr, memAddr, len, flags, DMA_CHANNEL_ANY,
			Callback, callbackArg);
		Enable(oldints);
		return 0;
	}
#ifdef BUILD_PCDEBUGGER
	else if ((flags & (DMA_SYNC|DMA_FORCE))==0)
	{
		if (DMAActive(dchan)||(dmaWorking[dchan].dmc_Flags&DMA_BUSY))
		{
		DBUG(("dma%x flags%x active in Q\n",dchan,flags));
			QueueDMA(dmaAddr, memAddr, len, flags, dchan,
				Callback, callbackArg);
			Enable(oldints);
			return 0;
		}
		DBUG(("dma%x flags%x is not active start it\n",dchan,flags));
	}
	if ((flags & (DMA_SYNC|DMA_FORCE))&&(dmaWorking[dchan].dmc_Flags & DMA_BUSY))
	{
		DBUG(("dma%x flags%x dma was active before sync\n",dchan,flags));
		if (saveddmaWorking[dchan].dmc_Callback==NULL)
		{
			memcpy(&saveddmaWorking[dchan],&dmaWorking[dchan],sizeof(DMAComp));
		}
	}
#endif
	dmaWorking[dchan].dmc_Flags |= DMA_BUSY;

	/*
	 * Need to get the transfer aligned to start and end
	 * on a cache line boundary (not cross a cache line).
	 */

	/* Do the head. */
	n = (uint32)memAddr & (cacheLineSize - 1);
	if (n > 0)
		n = cacheLineSize - n;
	if (n > len) n = len;
	DBUG((" DMA: head memcpy mem %x, dma %x, len %x\n",
		memAddr, dmaAddr, n));
	CpuCopy(memAddr, dmaAddr, n, flags);
	dmaAddr += n;
	memAddr += n;
	len -= n;

	/* Exclude the tail. */
	n = len - (len & (cacheLineSize - 1));

	/*
	 * In BUILD_DEBUGGER, a part of system RAM is used as the ROM.
	 * This can cause attempts to DMA from that part of RAM.
	 * This won't work; the DMA hardware won't do a RAM-to-RAM copy.
	 * Make sure we don't try; do a CPU copy in that case.
	 */
	if (n < MIN_DMA_SIZE
#ifdef BUILD_DEBUGGER
	   || dmaAddr >= sysRamStart && dmaAddr + len < sysRamEnd
#endif
	   )
	{
		/* Don't use DMA at all; finish the transfer with the CPU. */
		DBUG((" DMA: done memcpy mem %x, dma %x, len %x\n",
			memAddr, dmaAddr, len));
		dmaWorking[dchan].dmc_Flags &= ~DMA_BUSY;
		CpuCopy(memAddr, dmaAddr, len, flags);
		if (Callback != NULL)
			(*Callback)(callbackArg, 0);
		Enable(oldints);
		return 0;
	}

	/* Start the DMA. */
	DBUG((" DMA: real dmacpy mem %x, dma %x, len %x\n",
		memAddr, dmaAddr, n));
	dmaWorking[dchan].dmc_Callback = Callback;
	dmaWorking[dchan].dmc_CallbackArg = callbackArg;
	dmaWorking[dchan].dmc_TailMem = memAddr + n;
	dmaWorking[dchan].dmc_TailDMA = dmaAddr + n;
	dmaWorking[dchan].dmc_TailLen = len - n;
	dmaWorking[dchan].dmc_DMAFlags = flags;
	externalInvalidateDCache(memAddr, n);
	/* Clear all control bits. */
	CDE_CLR(KernelBase->kb_CDEBase, DMAControlReg(dchan), ~0);
	/* Set up addresses and length. */
	CDE_SET(KernelBase->kb_CDEBase, DMACDEAddr(dchan), (uint32)dmaAddr);
	CDE_SET(KernelBase->kb_CDEBase, DMAPowerBusAddr(dchan), (uint32)memAddr);
	CDE_SET(KernelBase->kb_CDEBase, DMACountReg(dchan), n);
#ifdef BUILD_PCDEBUGGER
	if ((Callback==NULL)&&(callbackArg!=NULL))
	{
		void (*PreCallback)(void) = callbackArg;
		PreCallback();
	}
#endif
	/* Begin! */
	n = CDE_DMA_CURR_VALID;
	if (flags & DMA_WRITE)
		n |= CDE_DMA_DIRECTION;
	CDE_SET(KernelBase->kb_CDEBase, DMAControlReg(dchan), n);

	if (flags & DMA_SYNC)
	{
#if 1 /*#ifdef BUILD_PCDEBUGGER*/
		while (DMAActive(dchan))
			continue;
#else
		while (DMAActive(dchan))
			continue;
		DMACompletion(dchan);
#endif
	}
	Enable(oldints);
	return 0;
}

/*****************************************************************************
  Handle a DMA completion event.
*/
static void
DMACompletion(uint32 dchan)
{
	DMAReq *dma;

	for (;;)
	{
#ifdef BUILD_PCDEBUGGER
		 if (saveddmaWorking[dchan].dmc_Callback!=NULL)
		 {
				DBUG(("DMA done: callback %x(%x)\n",
				saveddmaWorking[dchan].dmc_Callback,
				saveddmaWorking[dchan].dmc_CallbackArg));
				(*saveddmaWorking[dchan].dmc_Callback)(
				saveddmaWorking[dchan].dmc_CallbackArg, 0);
				saveddmaWorking[dchan].dmc_Callback=NULL;
		 }
#endif
		/* Is it really done? */
		if (DMAActive(dchan))
		{
			/* DMA is still active. */
			return;
		}

		if ((dmaWorking[dchan].dmc_Flags & DMA_BUSY) == 0)
		{
			/*
			 * No DMA in progress.  Ignore the interrupt.
			 * This can happen if we already did the
			 * DMACompletion synchronously (from StartDMA),
			 * and then get the completion interrupt.
			 */
			return;
		}

		dmaWorking[dchan].dmc_Flags &= ~DMA_BUSY;

		/* Finish the tail. */
		DBUG((" DMA: tail memcpy src %x, dst %x, len %x\n",
			dmaWorking[dchan].dmc_TailSrc,
			dmaWorking[dchan].dmc_TailDst,
			dmaWorking[dchan].dmc_TailLen));
		CpuCopy(dmaWorking[dchan].dmc_TailMem,
			dmaWorking[dchan].dmc_TailDMA,
			dmaWorking[dchan].dmc_TailLen,
			dmaWorking[dchan].dmc_DMAFlags);

		/* Do the callback to notify the client that its done. */
		if (dmaWorking[dchan].dmc_Callback != NULL)
		{
			DBUG(("DMA done: callback %x(%x)\n",
				dmaWorking[dchan].dmc_Callback,
				dmaWorking[dchan].dmc_CallbackArg));
			(*dmaWorking[dchan].dmc_Callback)(
				dmaWorking[dchan].dmc_CallbackArg, 0);
#ifdef BUILD_PCDEBUGGER
#else
			dmaWorking[dchan].dmc_Callback = NULL;
#endif
		}

		/* See if there is a new request to start up. */
		dma = (DMAReq *) RemHead(&dmaQueue);
		if (dma == NULL)
			return;
		StartDMA(dma->dma_DMAAddr, dma->dma_MemAddr, dma->dma_Len,
			dma->dma_Flags, dma->dma_Channel,
			dma->dma_Callback, dma->dma_CallbackArg);
	}
}

/*****************************************************************************
  Interrupt handlers for DMA completion interrupts.
*/
static int32
DMAInterrupt1(void)
{
	ClearInterrupt(DMAInterruptNumber(0));
	DMACompletion(0);
	return 0;
}

static int32
DMAInterrupt2(void)
{
	ClearInterrupt(DMAInterruptNumber(1));
	DMACompletion(1);
	return 0;
}

typedef int32 InterruptHandler(void);
static InterruptHandler *const DMAInterruptHandler[NUM_DMA_CHANNELS] = {
	DMAInterrupt1, DMAInterrupt2
};


/**
|||	AUTODOC -private -class Kernel -group Device-drivers -name AbortDMA
|||	Abort a DMA operation.
|||
|||	  Synopsis
|||
|||	    Err AbortDMA(void (*Callback)(void *arg, Err err), void *callbackArg)
|||
|||	  Description
|||
|||	    Aborts a DMA operation previously started via StartDMA().
|||
|||	  Arguments
|||
|||	    Callback
|||	    callbackArg
|||	        The arguments previously passed to StartDMA().
|||	        These arguments are used to identify which DMA
|||	        operation is to be aborted.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code on
|||	    failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V28.
|||
|||	  Associated Files
|||
|||	    <device/dma.h>
|||
|||	  See Also
|||
|||	    StartDMA()
|||
**/

/*****************************************************************************
  Abort a DMA request.
*/
Err
AbortDMA(void (*Callback)(void *arg, Err err), void *callbackArg)
{
	DMAReq *dma;
	uint32 oldints;
	uint32 dchan;

	InitDMA();
	oldints = Disable();

	/* Wait for current DMA to finish. */
	for (dchan = 0;  dchan < NUM_DMA_CHANNELS;  dchan++)
	{
		if (dmaWorking[dchan].dmc_Callback == Callback &&
		    dmaWorking[dchan].dmc_CallbackArg == callbackArg)
		{
			while (dmaWorking[dchan].dmc_Callback == Callback &&
			       dmaWorking[dchan].dmc_CallbackArg == callbackArg)
			{
				Enable(oldints);
				oldints = Disable();
			}
			return 0;
		}
	}

	/* Find the DMA request and remove it from the list of requests. */
	ScanList(&dmaQueue, dma, DMAReq)
	{
		if (dma->dma_Callback == Callback &&
		    dma->dma_CallbackArg == callbackArg)
		{
			RemNode((Node *)dma);
			Enable(oldints);
			return 0;
		}
	}

	Enable(oldints);
	return MAKEKERR(ER_SEVER,ER_C_STND,ER_NotFound);
}

/*****************************************************************************
  Initialize the DMA system.
  Needs to be called once before any DMA requests are done.
*/
void
InitDMA(void)
{
	uint32 dchan;
	CacheInfo ci;
	static bool initdone = FALSE;

	if (initdone) return;
	initdone = TRUE;

	putcfunc = KB_FIELD(kb_PutC);
	GetCacheInfo(&ci, sizeof(ci));
	cacheLineSize = Max(ci.cinfo_DCacheLineSize, ci.cinfo_ICacheLineSize);
#ifdef BUILD_DEBUGGER
	sysRamStart = KB_FIELD(kb_RAMBaseAddress);
	sysRamEnd = sysRamStart + (64*1024*1024);
#endif
	/* Initialize per-channel data structures. */
	for (dchan = 0;  dchan < NUM_DMA_CHANNELS;  dchan++)
	{
		dmaWorking[dchan].dmc_Flags = 0;
#ifndef BUILD_PCDEBUGGER
		DMAfirq[dchan] = SuperCreateFIRQ("DMA Firq", 1, 
				DMAInterruptHandler[dchan], 
				DMAInterruptNumber(dchan));
		if (DMAfirq[dchan] < 0)
		{
#ifdef BUILD_STRINGS
			printf("Cannot create DMA%d firq! error %x\n", 
				dchan, DMAfirq[dchan]);
#endif
			return;
		}
		ClearInterrupt(DMAInterruptNumber(dchan));
		EnableInterrupt(DMAInterruptNumber(dchan));
#endif
	}
}

#ifdef BUILD_PCDEBUGGER
static void InitDMAFirq(void)
{
	uint32 dchan;
	static bool initdone = FALSE;

	if (initdone) return;
	initdone = TRUE;
	/* Initialize per-channel data structures. */
	for (dchan = 0;  dchan < NUM_DMA_CHANNELS;  dchan++)
	{
		dmaWorking[dchan].dmc_Flags = 0;
		DMAfirq[dchan] = SuperCreateFIRQ("DMA Firq", 1, 
				DMAInterruptHandler[dchan], 
				DMAInterruptNumber(dchan));
		if (DMAfirq[dchan] < 0)
		{
#ifdef BUILD_STRINGS
			printf("Cannot create DMA%d firq! error %x\n", 
				dchan, DMAfirq[dchan]);
#endif
			return;
		}
		ClearInterrupt(DMAInterruptNumber(dchan));
		EnableInterrupt(DMAInterruptNumber(dchan));
	}
}
#endif
