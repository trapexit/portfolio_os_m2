/*
 * @(#) m2sysdep.h 96/05/10 1.48
 * Copyright 1994,1995, The 3DO Company
 *
 * Defines, types, function decls for system dependent stuff
 */

#ifndef __HARDWARE_CDE_H
#include <hardware/cde.h>
#endif


/*****************************************************************************
  Defines
*/

#define	TICKS_PER_SEC(freq)	((freq)/CLKS_PER_TICK)
#define TICKS_PER_MILLISEC(freq) (TICKS_PER_SEC(freq)/1000)
#define TICKS_PER_MICROSEC(freq) (TICKS_PER_SEC(freq)/1000000)

/* General */
#define	MAX_OS_SIZE		0x100000
#define	OS_SCRATCH_BUFFER	((void*)DIPIRSCRATCH)
#define	OS_SCRATCH_SIZE		MAX_OS_SIZE

/*
 * These defines match the type subfield of the device id register
 * present at offset 0 from the beginning of each device slot on
 * the powerbus.  See the CDE, Bridgit, etc specs for more details.
 */
#define M2_DEVICE_ID_BDA	BDA_DEVICE_ID_TYPE	/* 0x00 */
#define	M2_DEVICE_ID_CDE	CDE_DEVICE_ID_TYPE	/* 0x01 */
#define	M2_DEVICE_ID_BRIDGIT	BR_DEVICE_ID_TYPE	/* 0x02 */
#define M2_DEVICE_ID_NOT_THERE	UNKNOWN_DEVICE_ID_TYPE	/* 0xFF */


/* Powerbus devices are located at addr 0i000000 */
#define pbDevsBaseAddr		0x00000000
#define pbDevsSlotShift		24

/* Powerbus device flags */
#define pbDevFlagsDDP		1	/* Downloadable driver present flag */

#define pbDevsSlot0		0	/* First PowerBus slot */
#define pbDevsSlotN		7	/* Last PowerBus slot */

/*****************************************************************************
  Macros
*/

/* Build a powerbus base address, given a slot */
#define PBDevsBaseAddr(slot)	((pbDevsBaseAddr) | ((slot) << (pbDevsSlotShift)))

#define M2_CLEAR_OFFSET		0x400

#define PBUSDEV_READ(base,addr)		\
	(*(vuint32 *)((uint32)(base)|(addr)))
#define PBUSDEV_WRITE(base,addr,value)	\
	{ (*(vuint32 *)((uint32)(base)|(addr))) = (uint32)(value); _sync(); }

#define PBUSDEV_SET(base,addr,bits)	\
	PBUSDEV_WRITE(base, addr, bits)
#define PBUSDEV_CLR(base,addr,bits)	\
	PBUSDEV_WRITE(base, (addr) | M2_CLEAR_OFFSET, (bits))

/*****************************************************************************
  Types
*/

/* Beginning of all powerbus device slots have common identification format */
typedef struct PBDevHeader 
{
	uint8	devMfgId;
	uint8	devType;
	uint8	devFlags;
	uint8	devRev;
	uint8	reserved4;
	uint8	reserved5;
	uint8	reserved6;
	uint8	swRev;
	uint32	drvStartAddr; /* Only low 24 bits are valid */
} PBDevHeader;


/*****************************************************************************
  Functions - sysdep.c
*/


/* Interrupt stuff */
extern void EnableInterrupts(void);
extern void DisableInterrupts(void);

#ifdef PREFILLSTACK
/* Stack usage check */
extern void DisplayStackUsage(void);
#endif

/* Various routines related to exiting dipir */
extern void RestartToXXX(void *entryPoint, void *arg1, void *arg2, void *arg3);


/*****************************************************************************
  Functions - m2dipir.s, ppc_clib.s
*/

extern void BranchTo(uint32 *pEntrypoint, void *arg1, void *arg2, void *arg3);
extern uint32 GetMSR(void);
extern void SetMSR(uint32 newMSR);
extern uint32 GetSPRG0(void);
extern void SetSPRG0(uint32 newSPRG0);
extern int32 SoftReset(void);
extern void AsmVisaConfigDownload(uint32 cdeBase, 
			uint32 configReg, uint32 visa_download_bit,
			uint32 statusReg, uint32 visa_DIP_bit);

