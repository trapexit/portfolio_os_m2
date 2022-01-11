/* @(#) pchost.c 96/10/18 1.1 */

#ifdef BUILD_DEBUGGER

#ifdef BUILD_PCDEBUGGER
/************IBM PC STUFF BELOW***************************/

#include <kernel/io.h>
#include <kernel/driver.h>
#include <kernel/device.h>
#include <kernel/task.h>
#include <kernel/mem.h>
#include <kernel/debug.h>
#include <kernel/interrupts.h>
#include <kernel/operror.h>
#include <kernel/super.h>
#include <kernel/cache.h>
#include <kernel/kernel.h>
#include <kernel/sysinfo.h>
#include <kernel/timer.h>
#include <hardware/bridgit.h>
#include <hardware/bda.h>
#include <hardware/debugger.h>
#include <hardware/PPCasm.h>
#include <loader/loader3do.h>
#include <hardware/cde.h>
#include <string.h>
#include <stdio.h>

#include <device/dma.h>
#include <hardware/cde.h>
#include <file/directory.h>
#include <kernel/monitor.h>

#include <hardware/pcdebugger.h>

#if 0
/* hard coding hardware address, ugly....*/
#define PCMCIA_BASE 0x34000000
#define PARALLEL_WINDOW_OFFSET 0x1000000
#define CARDINTMASK INT_CDE_DEV5
#define CARDINT CDE_3DO_CARD_INT
#define CARDCONF CDE_DEV5_CONF
#define PCDEVDMACHANNEL 0
#define DMACNTL CDE_DMA1_CNTL
#define DMAINT INT_CDE_DMAC1

#define NULLTOKEN (0xfe000000) /*permission token*/
#define PUTCTOKEN (0x30000000) /*PutC send 64 byte along*/
#define CMDLTOKEN (0x31000000) /*GetCmdLine*/
#define MONTOKEN  (0x32000000) /*used for read/write memory*/
#define FSTOKEN   (0x33000000) /*All HOSTFS command*/
#define HCDTOKEN  (0x34000000  /*All HOSTCD command*/

/* MONITOR DEBUGGER communication token */
#define MONNULLTOKEN (0xfd000000) /*permission token for monitor*/
#define DBGR_TOKEN 	 (0x35000000) /*Debugger Packets*/
#define MON_TOKEN    (0x36000000) /*Monitor Packets*/
#define STOP_TOKEN   (0x37000000) /*Monitor Packets*/

typedef struct PchostToken
{
	uint32 opcode;
	uint8 *address;
	uint32 len;
	void  (*pchost_CallBack)(void *userarg, Err err); /* call on completion */
	uint32 userdata[4];
} PchostToken; /*sizeof(PchostToken) is same as cache line size*/

#define PROMPTSIZE 64
#define	MAX_PUTC_CHARS (64)
#define STRBUFSIZE (MAX_PUTC_CHARS+sizeof(PchostToken))

typedef struct PCDirectoryEntry
{
    uint32 de_Flags;
    uint32 de_UniqueIdentifier;
    uint32 de_Type;
    uint32 de_BlockSize;
    uint32 de_ByteCount;
    uint32 de_BlockCount;
    uint8  de_Version;
    uint8  de_Revision;
    uint16 de_rfu;
    uint32 de_rfu2;
    char   de_FileName[FILESYSTEM_MAX_NAME_LEN];
 } PCDirectoryEntry;
#endif

extern void MonitorFirq(void);

static void direntryreloc(void *arg, Err err)
{
	/*ugly data relocation for packing directory entry in 64(32*2) bytes*/
	HostFSReply *orgpacket = arg;
	void  (*pchost_CallBack)(void *userarg, Err err) = (void*)orgpacket->hfsr_Pad[0];
	IOReq *ior = (IOReq *)orgpacket->hfsr_Pad[1];
	PCDirectoryEntry *src = (PCDirectoryEntry *)orgpacket->hfsr_Pad[2];
	DirectoryEntry *dst = (DirectoryEntry *)orgpacket->hfsr_Pad[2];
	memcpy((void*)&(dst->de_FileName[0]),(void*)&(src->de_FileName[0]),FILESYSTEM_MAX_NAME_LEN);
	orgpacket->hfsr_Actual = sizeof(DirectoryEntry);
	dst->de_AvatarCount = 0;
	dst->de_Location = 0;
	FlushDCache(0, dst,sizeof(PCDirectoryEntry)+sizeof(PchostToken));
	pchost_CallBack(ior,err);
}

/*****************************************************************************/
/* Globals */
static Item CardFirq=-1;
static 	void *inttmpmem;			/*permission packet pointer*/			
static 	PchostToken *intpcpacket;	/*32 byte aligned permission packet pointer*/
static char *storageBuffer,*putcbuf;/*pointer to PutC buffer*/
static void (*putcfunc)(char a);    /*save original PutC just in case*/
static uint32 dmawindow;

/*****************************************************************************/
#undef DEBUG
/*#define DEBUG*/
#ifdef DEBUG
#define DBUG(x)     printf x
#define XBUG(x)     printf x
#else
#define DBUG(x)		
#define XBUG(x) 
#endif

/*****************************************************************************/
/* used to indicate the IOReq can currently be aborted */
#define IO_ABORTABLE 0x80000000

static void checkecpturnaround(void)
{
	if (dmawindow==PCMCIA_ECPDMAREG_OFFSET)
	{
		
		do
		{
			SuperInvalidateDCache((void*)((PCMCIA_BASE)|(PCMCIA_ECPCNTLREG_OFFSET)), 4);
		}
 		while (((*(volatile unsigned long *)((uint32)(PCMCIA_BASE)|(PCMCIA_ECPCNTLREG_OFFSET)))&0x80808080)==0);
	}
}

#ifdef DEBUG
static void printtoken(PchostToken *pcpacket)
{
	printf("opcode=%x\naddress=%x\nlen=%x\npchost_CallBack=%x\nuserdata[0]=%x\nuserdata[1]=%x\nuserdata[2]=%x\nuserdata[3]=%x\n",
	pcpacket->opcode , /*Unit,Command,Flags*/
	pcpacket->address  ,
	pcpacket->len  ,
	pcpacket->pchost_CallBack,
	pcpacket->userdata[0] ,
	pcpacket->userdata[1] ,
	pcpacket->userdata[2] ,
	pcpacket->userdata[3] );

}
#else
#define	printtoken(x)
static void sprinttoken(PchostToken *pcpacket,char *tmp)
{
	sprintf(tmp,"\nopcode=%x\naddress=%x\nlen=%x\npchost_CallBack=%x\nuserdata[0]=%x\nuserdata[1]=%x\nuserdata[2]=%x\nuserdata[3]=%x\n",
	pcpacket->opcode , /*Unit,Command,Flags*/
	pcpacket->address  ,
	pcpacket->len  ,
	pcpacket->pchost_CallBack,
	pcpacket->userdata[0] ,
	pcpacket->userdata[1] ,
	pcpacket->userdata[2] ,
	pcpacket->userdata[3] );

}
#endif


static bool
DMAActive(void)
{
	if (CDE_READ(KernelBase->kb_CDEBase, DMACNTL) &
			CDE_DMA_CURR_VALID)
		return TRUE;
	return FALSE;
}

static void convertHSTrequesttoken(PchostToken *pcpacket,HostFSReq *orgpacket,uint32 command)
{
	DBUG(("convertHSTrequesttoken:PchostToken=%x,HostFSReq=%x\n",pcpacket,orgpacket));
	pcpacket->opcode = command | (*(uint32*)(orgpacket) & 0xffffff); /*Unit,Command,Flags*/
	pcpacket->address = orgpacket->hfs_Recv.iob_Buffer;
	pcpacket->pchost_CallBack = NULL;
	pcpacket->userdata[1] = (uint32)orgpacket->hfs_Recv.iob_Len;
	pcpacket->userdata[2] = (uint32)orgpacket->hfs_Offset;
	pcpacket->userdata[3] = (uint32)orgpacket->hfs_ReferenceToken;
}

static void convertHFSreplytoken(HostFSReply *orgpacket,PchostToken *pcpacket)
{
	DBUG(("convertHFSreplytoken:PchostToken=%x,HostFSReply=%x\n",pcpacket,orgpacket));
	*(uint32*)(orgpacket) = pcpacket->opcode & 0xffffff;
	orgpacket->hfsr_Error = (int32)pcpacket->userdata[1];
	orgpacket->hfsr_Actual = pcpacket->userdata[3];
	orgpacket->hfsr_ReferenceToken = (void*)pcpacket->userdata[2];
}


/*
	send request
	make token
	userdata[0]=ior
	disable CARDINT
	if (hfs_Recv.iob_Len>0) completionflag=0 reply expected
	else if (hfs_Send.iob_Len>0) completionflag=1 send only
	else completionflag=2 send token only
	send token startdma if completionflag==2 set callback+arg to Acallback+ior
	       				else send data startdma no-sync 
						     if completionflag==1 set callback+arg to Acallback+ior
							 else no callback
	enable CARDINT
	return
			
	got interrupt
	completion 1st stage
	send NULL packet startdma sync. dchan=AllocDMAChannel(DMA_SYNC); StartDMA(DMA_READ|DMA_SYNC,dchan)
	read token startdma sync. to receive buffer StartDMA(DMA_WRITE|DMA_SYNC,dchan)
	if any read data startdma no-sync set callback+arg to the Acallback+userdata[0] from token 
	else call Acallback with userdata[0]
	return;
	
	Acallback
	SuperCompleteIO(arg);
*/

static void ReplyComplete(void *arg, Err err)
{
	IOReq *ior = arg;

    DBUG(("ReplyComplete: ior $%x, command $%x\n",ior, ior->io_Info.ioi_Command));
	ior->io_Error = err;
	ior->io_Actual = ior->io_Info.ioi_Send.iob_Len;
	SuperCompleteIO(ior);
}


static int32 CardFirqhandler (void)
{
	IOReq *ior;
	uint32 tmp;
	
	XBUG(("CardFirqhandler: got int\n"));
	DisableInterrupt(INT_CDE_DMAC1);
	ClearInterrupt(CARDINTMASK);
	if (DMAActive()) return 0;
	intpcpacket->opcode = NULLTOKEN;
  	FlushDCache(0, intpcpacket,sizeof(PchostToken));
	StartDMA((uint32)(PCMCIA_BASE+dmawindow), 
		(void*)intpcpacket, sizeof(PchostToken), 
		DMA_WRITE|DMA_SYNC, 
		PCDEVDMACHANNEL, 
		NULL, 
		(void*)checkecpturnaround);
	StartDMA( (uint32)(PCMCIA_BASE+dmawindow),
		(void*)intpcpacket, sizeof(PchostToken), 
		DMA_READ|DMA_SYNC, 
		PCDEVDMACHANNEL, 
		NULL, 
		(void*)checkecpturnaround);
	XBUG(("CardFirqhandler: got intpcpacket=%x\n",intpcpacket));
	printtoken(intpcpacket);
	/*see if monitor request*/
	if ((intpcpacket->opcode&0xff000000)==MONTOKEN)
	{
	/*if it is go do it and return*/
		switch ((intpcpacket->opcode&0xff00)>>8)
		{
		  case 'W':
				StartDMA( PCMCIA_BASE+dmawindow,
				(void*)intpcpacket->address, intpcpacket->len, 
				DMA_READ|DMA_FORCE, 
				PCDEVDMACHANNEL, 
				NULL, 
				NULL);
			break;
		  case 'R':
                                if (intpcpacket->address==(ubyte*)(0x00040008))
                                {
                                uint32 a;
                                a=*((volatile uint32 *)intpcpacket->address);
                                *((uint32*)intpcpacket)=a;
                                FlushDCache(0, intpcpacket,sizeof(PchostToken));
                                StartDMA( PCMCIA_BASE+dmawindow,
                                (void*)intpcpacket, intpcpacket->len,
                                DMA_WRITE|DMA_FORCE,
                                PCDEVDMACHANNEL,
                                NULL,
                                (void*)checkecpturnaround);
                                }
                                else
				StartDMA( PCMCIA_BASE+dmawindow,
				(void*)intpcpacket->address, intpcpacket->len, 
				DMA_WRITE|DMA_FORCE, 
				PCDEVDMACHANNEL, 
				NULL, 
				(void*)checkecpturnaround);
			break;
		  default:
			break;
		}
		goto intdone;
	}
	else if ((intpcpacket->opcode&0xff000000)==STOP_TOKEN)
	{
		ClearInterrupt(CARDINTMASK);
		MonitorFirq(); /*get into monitor*/
		goto intdone1;
	}
	
	switch((intpcpacket->opcode&0xff000000))
	{
		case FSTOKEN:
		case CMDLTOKEN:
		 break;
		default:
		{
		 char temp[512];
		 uint32 i;
		 putcfunc('X');
		 sprinttoken(intpcpacket,temp);
		 for (i=0;i<strlen(temp);i++) putcfunc(temp[i]);
		 while (1) ;
		}
		 break;
	}
	
	ior = (IOReq*)intpcpacket->userdata[0];
    XBUG(("CardFirqhandler: ior $%x, command $%x\n",ior, ior->io_Info.ioi_Command));
	tmp = (uint32)(ior->io_Info.ioi_Recv.iob_Buffer)-2;
	convertHFSreplytoken((HostFSReply*)tmp,intpcpacket);
	if (intpcpacket->len>0)
	{
		if ((intpcpacket->opcode&0xff000000)==FSTOKEN)
		switch ((intpcpacket->opcode&0xff00)>>8)
		{
		  case HOSTFS_REMOTECMD_READENTRY:
		  case HOSTFS_REMOTECMD_READDIR:
			{
				HostFSReply *orgpacket=(HostFSReply*)tmp;
				/*ugly data relocation for packing directory entry in 64 bytes
				after transfer is complete*/
				orgpacket->hfsr_Pad[0]=(uint32)intpcpacket->pchost_CallBack;
				orgpacket->hfsr_Pad[1]=(uint32)ior;
				orgpacket->hfsr_Pad[2]=(uint32)intpcpacket->address;				
				StartDMA( PCMCIA_BASE+dmawindow,
				(void*)intpcpacket->address, intpcpacket->len, 
				DMA_READ|DMA_FORCE, 
				PCDEVDMACHANNEL, 
				direntryreloc, 
				orgpacket);
				goto intdone;
			}			
		  default:
			break;
		}
		
		StartDMA( PCMCIA_BASE+dmawindow,
			(void*)intpcpacket->address, intpcpacket->len, 
			DMA_READ|DMA_FORCE, 
			PCDEVDMACHANNEL, 
			intpcpacket->pchost_CallBack, 
			ior);
	}
	else if (intpcpacket->pchost_CallBack!=NULL) intpcpacket->pchost_CallBack((void*)ior,0);
intdone:
	ClearInterrupt(CARDINTMASK);
intdone1:
	EnableInterrupt(INT_CDE_DMAC1);
	return 0;
}

static int32 CreateHostIO(IOReq *ior)
{
	/* Allocate tokenbuf for this IOReq */
	ior->io_Extension[0] = (uint32)SuperAllocMem(sizeof(PchostToken)*3, MEMTYPE_NORMAL);
	if (ior->io_Extension[0]==0)
		return NOMEM;
	 return 0;
}

static int32 DeleteHostIO(IOReq *ior)
{
    /* Free tokenbuf for this IOReq */
	SuperFreeMem((void*)ior->io_Extension[0],sizeof(PchostToken)*3);
	return 0;
}

static int32 HostHFSCmdSend(IOReq *ior)
{
	PchostToken *pcpacket;
	uint32 i;
	uint32 tmp = (uint32)(ior->io_Info.ioi_Send.iob_Buffer) - 2;
	HostFSReq *orgpacket=(HostFSReq*)tmp;
	ubyte *sendbuf;
	uint32 sendlen;
	
	DBUG(("HostHFSCmdSend: ior $%x, command $%x, orgpacket$%x\n",ior, ior->io_Info.ioi_Command,orgpacket));
	pcpacket = (PchostToken*)ior->io_Extension[0];
	i = ((uint32)pcpacket & (0xffffffff^(sizeof(PchostToken)-1))) + sizeof(PchostToken);
	pcpacket = (PchostToken *)i;
	orgpacket->hfs_Unit = (uint8)ior->io_Info.ioi_CmdOptions;
	convertHSTrequesttoken(pcpacket,orgpacket,FSTOKEN);
	
	ior->io_Flags &= (~IO_QUICK);
	/*see if send buffer > sizeof(PchostToken) (i.e 32 bytes)
	(ior has 32 byte aligned 2*sizeof(PchostToken) bytes buffer*/
	if ((sendlen=orgpacket->hfs_Send.iob_Len)>sizeof(PchostToken))
	{
		i=2; /*send big data*/
        sendbuf=(ubyte*)(orgpacket->hfs_Send.iob_Buffer);
	}
	else if (sendlen>0)
	{
		i =1; /*send data*/
        sendlen=sizeof(PchostToken);
		sendbuf=(ubyte*)pcpacket + sizeof(PchostToken);
		memcpy(sendbuf,(void*)(orgpacket->hfs_Send.iob_Buffer),sendlen);
	}
	else 
	{
		i=0; /*send token*/
		sendbuf=NULL;
	}
    pcpacket->len = sendlen;
    pcpacket->userdata[0] = (uint32)ior;
	pcpacket->pchost_CallBack = ReplyComplete;

    /* this one's commited, no turning back */
    ior->io_Flags &= (~IO_ABORTABLE);
    FlushDCache(0, pcpacket, sizeof(PchostToken)*2);
 	switch(i)
	{
		case 0:
			StartDMA(PCMCIA_BASE+dmawindow, 
				(void*)pcpacket, sizeof(PchostToken), 
				DMA_WRITE, 
				PCDEVDMACHANNEL, 
				NULL, 
				(void*)checkecpturnaround);
			break;
		case 1:
			StartDMA(PCMCIA_BASE+dmawindow, 
				(void*)pcpacket, sizeof(PchostToken)+sendlen, 
				DMA_WRITE, 
				PCDEVDMACHANNEL, 
				NULL, 
				(void*)checkecpturnaround);
			break;
		case 2: /*big buffer send*/
			{
			uint32 oldints;
			oldints = Disable();
			while (DMAActive()) /*wait till current dma done*/
			{
				Enable(oldints);
				oldints = Disable();	
			}			
			StartDMA(PCMCIA_BASE+dmawindow,
				(void*)pcpacket, sizeof(PchostToken),
  				DMA_WRITE|DMA_SYNC,
				PCDEVDMACHANNEL,
				NULL,
				(void*)checkecpturnaround);
            StartDMA(PCMCIA_BASE+dmawindow,
                                (void*)sendbuf, sendlen,
                                DMA_WRITE|DMA_FORCE,
                                PCDEVDMACHANNEL,
                                NULL,
                                (void*)checkecpturnaround);
			Enable(oldints);
			}
			break;
		default:
			DBUG(("HostHFSCmdSend:WHAT IS HAPPENING\n"));
	}
	DBUG(("HostHFSCmdSend: mode=%d\n",i));
	printtoken(pcpacket);
	if (sendlen>0) DBUG(("sendbuf=%s\n",(char*)sendbuf));
	return 0;
}

static void syncputC(char character)
{
	static uint32 bytecount[2];
	static uint32 currentbuf;
	static uint32 state = 0;
	static bool printcomcame=false;
	uint32 oldints;
	oldints = Disable();
	switch (state)
	{
		case 0: 
				bytecount[0]=0;
				bytecount[1]=0;
				currentbuf=0;
				state = 1;
		case 1:	
				if ((bytecount[currentbuf] >= MAX_PUTC_CHARS) || (character==0))
				{
					PchostToken *tmpputctoken;
					if (bytecount[currentbuf]==0) break;
					state = 2;
					currentbuf = 1 - currentbuf;
					printcomcame = TRUE;
					if (character)
						storageBuffer[sizeof(PchostToken)+currentbuf*STRBUFSIZE+(bytecount[currentbuf]++)] = character;
					DisableInterrupt(INT_CDE_DMAC1);
					DisableInterrupt(CARDINTMASK);
					
					while (printcomcame)
					{
					printcomcame=FALSE;	
					while(DMAActive())
					{
						Enable(oldints); /*enable for long spin-lock*/
						oldints = Disable();
					}
					/*sendit*/
					if (bytecount[1-currentbuf]<MAX_PUTC_CHARS) 
						storageBuffer[sizeof(PchostToken)+(1-currentbuf)*STRBUFSIZE+(bytecount[1-currentbuf]++)]=0;
					if (bytecount[1-currentbuf]<(MAX_PUTC_CHARS/2)) bytecount[1-currentbuf]=(MAX_PUTC_CHARS/2);
					else bytecount[1-currentbuf]=MAX_PUTC_CHARS;
					tmpputctoken = (PchostToken*)(&storageBuffer[(1-currentbuf)*STRBUFSIZE]);
					tmpputctoken->len = bytecount[(1-currentbuf)];
					FlushDCache(0, tmpputctoken, sizeof(PchostToken)+MAX_PUTC_CHARS);

					StartDMA(PCMCIA_BASE+dmawindow, 
						(void*)tmpputctoken, sizeof(PchostToken)+bytecount[(1-currentbuf)], 
						DMA_WRITE|DMA_FORCE, 
						PCDEVDMACHANNEL, 
						NULL, 
						(void*)checkecpturnaround);
					bytecount[(1-currentbuf)]=0;
					if (printcomcame) currentbuf=1-currentbuf;
					if (bytecount[1-currentbuf]==0) break;
					}

					
					EnableInterrupt(INT_CDE_DMAC1);
					EnableInterrupt(CARDINTMASK);
					state = 1;
					break;
				}
				else
				{
					storageBuffer[sizeof(PchostToken)+currentbuf*STRBUFSIZE+(bytecount[currentbuf]++)] = character;
					break;
				}
		case 2:
				if ((bytecount[currentbuf] >= MAX_PUTC_CHARS))
				{
					state = 3;
				}
				else
				{
					if (character) storageBuffer[sizeof(PchostToken)+currentbuf*STRBUFSIZE+(bytecount[currentbuf]++)] = character;
					else 
					{
						printcomcame=true;
						state=3;
					}
					break;
				}
		case 3: break; /*both buffer full so lose characters(s)*/
	}
	Enable(oldints);
}

static PchostToken *hostcmdlbuf;
static int32 HostCMDLCmdSend(IOReq *ior)
{
	PchostToken *pcpacket;
	uint32 i;
	uint32 tmp = (uint32)(ior->io_Info.ioi_Send.iob_Buffer) - 2;
	HostFSReq *orgpacket=(HostFSReq*)tmp;
	ubyte *sendbuf;
	uint32 sendlen;
	
    DBUG(("HostCMDLCmdSend: ior $%x, command $%x, orgpacket$%x\n",ior, ior->io_Info.ioi_Command,orgpacket));
	pcpacket = (PchostToken*)hostcmdlbuf;
	i = ((uint32)pcpacket & (0xffffffff^(sizeof(PchostToken)-1))) + sizeof(PchostToken);
	pcpacket = (PchostToken *)i;
	
	orgpacket->hfs_Unit = (uint8)ior->io_Info.ioi_CmdOptions;
	if ((sendlen=orgpacket->hfs_Send.iob_Len)>0)
	{
		sendlen++; /*add 1 for NULL char*/
		if (sendlen>PROMPTSIZE) sendlen=PROMPTSIZE;
		sendbuf=(ubyte*)pcpacket + sizeof(PchostToken);
		memcpy(sendbuf,(void*)(orgpacket->hfs_Send.iob_Buffer),sendlen);
	}
	convertHSTrequesttoken(pcpacket,orgpacket,CMDLTOKEN);
	if (sendlen<(PROMPTSIZE/2)) sendlen = (PROMPTSIZE/2);
	else sendlen = PROMPTSIZE;
	ior->io_Flags &= (~IO_QUICK);
    /* this one's commited, no turning back */
    ior->io_Flags &= (~IO_ABORTABLE);
    pcpacket->len = sendlen;
    pcpacket->userdata[0] = (uint32)ior;
	pcpacket->pchost_CallBack = ReplyComplete;
    FlushDCache(0, pcpacket, sizeof(PchostToken)+sendlen);
	StartDMA(PCMCIA_BASE+dmawindow, 
		(void*)pcpacket, sizeof(PchostToken)+sendlen, 
		DMA_WRITE, 
		PCDEVDMACHANNEL, 
		NULL, 
		(void*)checkecpturnaround);
	DBUG(("HostHFSCmdSend: mode=%d\n",i));
	printtoken(pcpacket);
	if (sendlen>0) DBUG(("sendbuf=%s\n",(char*)sendbuf));
	return 0;
}

/************IBM PC STUFF ABOVE***************************/

static void AbortHostIO(IOReq *ior)
{
	TOUCH(ior);
}



/*****************************************************************************/


static int32 CmdStatus(IOReq *ior)
{
DeviceStatus stat;
int32        len;

    memset(&stat,0,sizeof(stat));
    stat.ds_DriverIdentity    = DI_OTHER;
    stat.ds_MaximumStatusSize = sizeof(DeviceStatus);
    stat.ds_FamilyCode        = DS_DEVTYPE_OTHER;

    len = ior->io_Info.ioi_Recv.iob_Len;
    if (len > sizeof(stat))
        len = sizeof(stat);

    memcpy(ior->io_Info.ioi_Recv.iob_Buffer,&stat,len);

    ior->io_Actual = len;

    return 1;
}


/*****************************************************************************/


static int32 HostCmdSend(IOReq *ior)
{
    if ((ior->io_Info.ioi_Send.iob_Len > DATASIZE)
     || (ior->io_Info.ioi_Recv.iob_Len > DATASIZE))
    {
        ior->io_Error = BADIOARG;
        return 1;
    }
	if (ior->io_Info.ioi_CmdOptions==HOST_FS_UNIT)
	{
		return HostHFSCmdSend(ior);
	}
	else if (ior->io_Info.ioi_CmdOptions==HOST_CONSOLE_UNIT)
	{
		return HostCMDLCmdSend(ior);
	}
	DBUG(("HostCmdSend:  ior $%x, command $%x is not suported\n",ior, ior->io_Info.ioi_Command));
	ior->io_Error = BADIOARG;
    return 1;
}


/*****************************************************************************/


static Item CreateHostDriver(Driver *drv)
{
uint32  i;
uint32 intdchan;
uint32 oldints;
	/*once allocated dma channel, never gives it back*/
	do
	{
		intdchan = AllocDMAChannel(DMA_SYNC);
		DBUG(("CreateHostDriver: dmachan=%x\n",intdchan));
	}while(intdchan!=PCDEVDMACHANNEL);
    DBUG(("CreateHostDriver: dmachan%x\n",intdchan));
	oldints = Disable();
	while(DMAActive()) continue;
    dmawindow = PARALLEL_WINDOW_OFFSET;
	printf("dmawindow=%x\n",dmawindow);
	Enable(oldints);
	printf("ReplyComplete=%x\n",ReplyComplete);
	inttmpmem=(void*)SuperAllocMem(sizeof(PchostToken)*2, MEMTYPE_NORMAL);
	if (inttmpmem==NULL) return NOMEM;
	i = ((uint32)inttmpmem & (0xffffffff^(sizeof(PchostToken)-1))) + sizeof(PchostToken);
	intpcpacket = (PchostToken *)i;
	CDE_SET(KernelBase->kb_CDEBase,CDE_DEV5_CONF, CDE_BIOBUS_SAFE|CDE_IOCONF);
	CardFirq = SuperCreateFIRQ("CardFirq", 1, CardFirqhandler, CARDINTMASK);
	if (CardFirq<0)  
	{
		DBUG(("CreateHostDriver: SuperCreateFIRQ failed=%x\n",CardFirq));
		return CardFirq;
	}
	if (putcbuf==NULL)
	{
		PchostToken *tmpputctoken;
		putcbuf = SuperAllocMem(sizeof(PchostToken)+STRBUFSIZE*2,MEMTYPE_NORMAL);
		if (putcbuf==NULL) return NOMEM;
		i = ((uint32)putcbuf & (0xffffffff^(sizeof(PchostToken)-1))) + sizeof(PchostToken);
		storageBuffer = (char*)i;
		tmpputctoken = (PchostToken*)i;
		tmpputctoken->opcode = (uint32)PUTCTOKEN;
		tmpputctoken->address = NULL;
		tmpputctoken->pchost_CallBack = NULL;
		i += STRBUFSIZE;
		tmpputctoken = (PchostToken*)i;
		tmpputctoken->opcode = (uint32)PUTCTOKEN;
		tmpputctoken->address = NULL;
		tmpputctoken->pchost_CallBack = NULL;
		putcfunc = KB_FIELD(kb_PutC);
#if 1
		KB_FIELD(kb_PutC)=syncputC;
#endif
		DBUG(("host: Hello PC=%x\n",storageBuffer));
	}
 	hostcmdlbuf = (PchostToken *)SuperAllocMem(sizeof(PchostToken)*2+PROMPTSIZE, MEMTYPE_NORMAL);
 	ClearInterrupt(CARDINTMASK);
	EnableInterrupt(CARDINTMASK);
 	drv->drv.n_OpenCount++;
	return drv->drv.n_Item;
}

/*****************************************************************************/
#if 0
static Err DeleteHostDriver(Device* dev)
{
	TOUCH(dev);
    return NOSUPPORT;
}
#endif
/*****************************************************************************/


static Item ChangeHostDriverOwner(Driver *drv, Item newOwner)
{
	TOUCH(drv);
	SetItemOwner(CardFirq, newOwner);
	return 0;
}

/*****************************************************************************/


static DriverCmdTable cmdTable[] =
{
    {HOST_CMD_SEND, HostCmdSend},
    {CMD_STATUS,    CmdStatus},
};

int32 main(void)
{
    return CreateItemVA(MKNODEID(KERNELNODE,DRIVERNODE),
	    TAG_ITEM_NAME,              "pchost",
	    TAG_ITEM_PRI,               1,
	    CREATEDRIVER_TAG_CMDTABLE,  cmdTable,
	    CREATEDRIVER_TAG_NUMCMDS,   sizeof(cmdTable) / sizeof(cmdTable[0]),
	    CREATEDRIVER_TAG_CREATEDRV, CreateHostDriver,
#if 0
		CREATEDRIVER_TAG_DELETEDEV,	DeleteHostDriver,
#endif
		CREATEDRIVER_TAG_CRIO,        CreateHostIO,
		CREATEDRIVER_TAG_DLIO,        DeleteHostIO,
	    CREATEDRIVER_TAG_ABORTIO,   AbortHostIO,
	    CREATEDRIVER_TAG_CHOWN_DRV,	ChangeHostDriverOwner,
	    CREATEDRIVER_TAG_MODULE,    FindCurrentModule(),
	    TAG_END);
}

/************IBM PC STUFF ABOVE***************************/
#else
int main(void)
{
	return 0;
}
#endif

#else /* BUILD_DEBUGGER */

int main(void)
{
	return 0;
}

#endif
