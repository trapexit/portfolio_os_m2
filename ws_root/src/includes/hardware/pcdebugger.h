#ifndef __HARDWARE_PCDEBUGGER_H
#define __HARDWARE_PCDEBUGGER_H


/******************************************************************************
**
**  @(#) pcdebugger.h 96/10/15 1.00
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __HARDWARE_CDE_H
#include <hardware/cde.h>
#endif

/* hard coding hardware address, ugly....*/

/*#define PCDEVUSESLOT5
*/
#ifdef PCDEVUSESLOT5
#define PCMCIA_BASE 0x34000000
#define CARDINTMASK INT_CDE_DEV5
#define CARDINT CDE_3DO_CARD_INT
#define CARDCONF CDE_DEV5_CONF
#define CARDSETUP CDE_DEV5_SETUP
#else
#define PCMCIA_BASE 0x38000000
#define CARDINTMASK INT_CDE_DEV6
#define CARDINT 0x00040000
#define CARDCONF CDE_DEV6_CONF
#define CARDSETUP CDE_DEV6_SETUP
#endif

#define PCMCIA_ECPCNTLREG_OFFSET (0x800000)
#define PCMCIA_ECPDMAREG_OFFSET (0xc00000)
#define PARALLEL_WINDOW_OFFSET (((*(volatile unsigned long *)((uint32)(PCMCIA_BASE)|(PCMCIA_ECPCNTLREG_OFFSET)))&0xf)==5 ? PCMCIA_ECPDMAREG_OFFSET:0x1000000)
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
#define DBGR_TOKEN   (0x35000000) /*Debugger Packets*/
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


 
#endif /* __HARDWARE_PCDEBUGGER_H */
