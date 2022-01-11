/*
 *	@(#) dipir.h 96/10/08 1.94
 *	Copyright 1994,1995,1996 The 3DO Company
 *
 * System independent defines, types, function decls
 */

/* Public stuff */
#include "kernel/types.h"
#include "bootcode/bootglobals.h"
#include "hardware/PPC.h"
#include "file/discdata.h"
#include "dipir/dipirpub.h"
#include "dipir/hwresource.h"
#include "dipir/rom.h"
#include "dipir/rsa.h"
/* Dipir private stuff */
#include "m2sysdep.h"
#include "channel.h"
#include "ddd.h"
#include "tiny.h"
/* RSA stuff */
#include "global.h"
#include "bsafe2.h"
#include "md5.h"

/*****************************************************************************
  Defines
*/

/* Dipir jmp buf size is smaller than std to save space, see ppc_clib.s */
#define jmp_buf_size		24

/* Values for longjmp() */
#define	JMP_ABORT		1	/* Trap occurred */
#define	JMP_OFFLINE		2	/* Media went offline */

#define	NUM_DIPIR_EXCEPTIONS	1
#define	EXCEPTION_SIZE		(10*4)	/* See CatchException() */
#ifdef DIPIR_INTERRUPTS
#define	INTERRUPT_SIZE		(11*4)	/* See CatchInterrupt() */
#endif /* DIPIR_INTERRUPTS */

#define	MIN_PRIVATE_BUF_SIZE	(32*1024) 

/*
 * Return codes visible to operator.
 */
typedef int32 DIPIR_RETURN;

#define	DIPIR_RETURN_TROJAN	-1	/* We ejected 'em */
#define	DIPIR_RETURN_TRYAGAIN	 0	/* Dipir failed.  Try again. */
#define	DIPIR_RETURN_OK		 1	/* Good 3DO disc */
#define	DIPIR_RETURN_DATADISC	 2	/* Data disc.  Initiate discchange */
#define	DIPIR_RETURN_NODISC	 3	/* Empty drive */
#define	DIPIR_RETURN_ROMAPPDISC	 4	/* Photo, Red Book Audio, etc. */
#define	DIPIR_RETURN_SPECIAL	 7	/* Special 3DO disc */

/* Commands to a device dipir */
#define	DIPIR_VALIDATE		1	/* Validate the device */
#define	DIPIR_LOADROMAPP	2	/* Load ROMAPP OS from device */

/* Dipir environment structure versions */
#define DIPIR_VERSION		1
#define DIPIR_REVISION		1

/* Dipir routine table version mechanism bases */
#define	DIPIR_ENV_VERSION_0	0
#define	DIPIR_ROUTINES_VERSION_0 0
#define	DEVICE_ROUTINES_VERSION_0 0

/* Flags for DipirAlloc */
#define	ALLOC_DMA		0x00000001 /* Mem will be accessed via DMA */
#define	ALLOC_EXEC		0x00000002 /* Mem will hold executable code */

/* RSA keys */
typedef uint32 KeyID;
#define	KEYBIT_THDO		0x01
#define	KEYBIT_APP		0x02
#define	KEYBIT_64		0x10
#define	KEYBIT_128		0x20

#define	KEY_THDO_64		(KEYBIT_THDO|KEYBIT_64)
#define	KEY_APP_64		(KEYBIT_APP|KEYBIT_64)
#define	KEY_128			(KEYBIT_128)

#define	KeyLen(key)		(((key) & KEYBIT_128) ? 128 : 64)

#define	SIG_LEN			64	/* Length of RSA signature */
#define SIG_128_LEN		128	/* Length of big RSA signature */

#define	MAX_ROMTAG_BLOCK_SIZE	(8*1024)

/*
 * Sync bytes for special dipir-only filesystems
 * (not recognized by filesystem in the OS).
 */
#define	VOLUME_SYNC_BYTE_DIPIR	0x59

/*****************************************************************************
  Macros
*/

#define	LoByte(x)		((uint8)x)
#define	HiByte(x)		((uint8)(((uint16)(x)) >> 8))
#define	MakeInt16(H,L)		((((uint32)(H))<<8) | ((uint32)(L)))
#define	min(x,y)		(((x) < (y)) ? (x) : (y))
#define	max(x,y)		(((x) > (y)) ? (x) : (y))
#define	ROUNDUP(v,sz)		(((v)+(sz)-1) & ~((sz)-1))


/*****************************************************************************
  Types
*/

/*
 * Representation of a timestamp.
 */
typedef struct TimerState
{
	uint32	ts_TimerHigh;
	uint32	ts_TimerLow;
} TimerState;

/*
 * Saved exception vector code.
 */
typedef struct ExceptionCode
{
	uint32	exc_Code[EXCEPTION_SIZE/4];	/* Space to save old vector */
} ExceptionCode;

/*
 * MD5 digest.
 */
typedef struct Digest
{
	uchar	digest[16];
} Digest;

/* WARNING: layout of these structures must match definitions in dipir.i */

#ifdef DIPIR_INTERRUPTS
/*
 * Space to save registers on interrupt.
 */
typedef struct RegBlock
{
	uint32	rb_GPRs[32];
	uint32	rb_CR;
	uint32	rb_XER;
	uint32	rb_LR;
	uint32	rb_CTR;
	uint32	rb_PC;
	uint32	rb_MSR;
} RegBlock;
#endif /* DIPIR_INTERRUPTS */

/*
 * Structure of an OS component file.
 * An OS component file starts with a CompFileHeader, followed by
 * a sequence of components, each of which is prefixed with a CompHeader.
 */
typedef struct CompFileHeader
{
	uint32	comp_pad1;	/* not used */
	uint32	comp_pad2;	/* not used */
} CompFileHeader;

typedef struct CompHeader {
	uint32	comp_Addr;	/* Address of component (not used) */
	uint32	comp_Size;	/* Size of component (bytes) */
	uint8	comp_Data[1];	/* The component itself */
} CompHeader;

#define	SizeofCompHeader (2*sizeof(uint32))

/*
 * The master structure that holds all of the dipir "temporary" data.
 * Temporary data is stuff that doesn't need to be saved across dipir events.
 */
typedef struct DipirTemp
{
	uint32		dt_Version;
	const struct DipirRoutines *dt_DipirRoutines;
	bootGlobals *	dt_BootGlobals;
	int		dt_JmpBuf[jmp_buf_size];
#ifdef DIPIR_INTERRUPTS
	RegBlock	dt_InterruptSaveArea;
	uint32		dt_SaveVint;
	uint32		dt_SaveInterruptMask;
	uint32		dt_SavedIntrCode[INTERRUPT_SIZE];
#endif /* DIPIR_INTERRUPTS */
	DDDFile *	dt_SysRomFd;
	uint32		dt_BusClock;
	uint32		(*dt_QueryROMSysInfo)(uint32 tag, void *info, uint32 size);
	void *		dt_BootData;
	uint32		dt_CacheLineSize;
	uint32		dt_PrivateBufSize;
	ExceptionCode	dt_SavedExceptions[NUM_DIPIR_EXCEPTIONS];
	Digest		dt_DipirDigest;
	MD5_CTX		dt_DipirDigestContext;
	uint32		dt_Flags;
} DipirTemp;

/* Bits in dt_Flags */
#define	DT_FOUND_ROMAPP_MEDIA	0x00000001

extern DipirTemp *dtmp;
extern bootGlobals *theBootGlobals;
struct Elf32_Ehdr;
struct _3DOBinHeader;
struct Module;
struct List;

/* Routines provided by rom dipir code for drivers and device dipirs */
typedef struct DipirRoutines
{
	uint32	DipirRoutinesVersion;
	void 	(*dr_printf)(const char *fmt, ...);
	void	(*dr_ResetTimer)(TimerState *tm);
	uint32	(*dr_ReadMilliTimer)(TimerState *tm);
	uint32	(*dr_ReadMicroTimer)(TimerState *tm);
	void	(*dr_Reboot)(void *addr, void *arg1, void *arg2, void *arg3);
	void	(*dr_HardBoot)(void);
	int32	(*dr_DisplayImage)(VideoImage *image, VideoPos pos, 
			uint32 expand, uint32 attr, uint32 attr2);
	Boolean	(*dr_AppExpectingDataDisc)(void);

	uint32	(*dr_NextRomTag)(DDDFile *fd, uint32 pos, struct RomTag *rt);
	uint32	(*dr_FindRomTag)(DDDFile *fd, uint32 subsys, uint32 type, 
			uint32 pos, struct RomTag *rt);
	void *	(*dr_DipirAlloc)(uint32 size, uint32 allocFlags);
	void	(*dr_DipirFree)(void *ptr);
	int32	(*dr_ReadDoubleBuffer)(DDDFile *fd, uint32 startBlock, 
			uint32 readSize, uint32 trailerSize, 
			void *buf1, void *buf2, uint32 bufSize,
			int32 (*Digest)(DDDFile *fd, void *arg, void *buf, uint32 size), 
			void *digestArg,
			int32 (*Final)(DDDFile *fd, void *arg, void *buf, uint32 size),
			void *finalArg);
	int32	(*dr_ReadSigned)(DDDFile *fd, uint32 offset, 
			uint32 size, void *buffer, KeyID key);
	void	(*dr_DipirInitDigest)(void);
	void	(*dr_DipirUpdateDigest)(void *buffer, uint32 len);
	void	(*dr_DipirFinalDigest)(void);
	int	(*dr_RSAInit)(Boolean indipir);
	int	(*dr_RSAFinalWithKey)(A_RSA_KEY *key, uchar *sig, int sigLen);
	int	(*dr_RSAFinal)(uchar *sig, KeyID key);
	int32	(*dr_GenRSADigestInit)(DipirDigestContext *info);
	int32	(*dr_GenRSADigestUpdate)(DipirDigestContext *info, 
			uchar *buf, uint32 len);
	int32	(*dr_GenRSADigestFinal)(DipirDigestContext *info, 
			uchar *sig, uint32 sigLen);
	int32	(*dr_SectorECC)(uint8 *buf);
	Boolean	(*dr_CanDisplay)(void);
	int32	(*dr_DisplayIcon)(VideoImage *icon, uint32 attr, uint32 attr2);
	int32	(*dr_DisplayBanner)(VideoImage *image, uint32 attr, uint32 attr2);
	int32	(*dr_InitBoot)(void);
	uint32	(*dr_BootAppPrio)(void);
	void	(*dr_SetBootApp)(ExtVolumeLabel *label, uint32 prio);
	void	(*dr_SetBootRomAppMedia)(DipirHWResource *dev);
	DipirHWResource *(*dr_BootRomAppMedia)(void);
	uint32	(*dr_BootOSVersion)(uint32 osflags);
	struct List *	(*dr_BootOSAddr)(uint32 osflags);
	uint32	(*dr_BootOSFlags)(uint32 osflags);
	void	(*dr_SetBootOS)(ExtVolumeLabel *label, uint32 version, 
			void *addr, void *entry, uint32 osflags, 
			uint32 kreserve, uint32 devicePerms,
			DipirHWResource *dipirDev, uint32 dddID, uint32 dipirID);
	void	(*dr_strncpyz)(char *to, char *from, uint32 tolen, uint32 fromlen);
	int	(*dr_strcmp)(char *s1, char *s2);
	int	(*dr_strncmp)(char *s1, char *s2, int n);
	char *	(*dr_strcat)(char *a, const char *b);
	char *	(*dr_strcpy)(char *a, const char *b);
	int	(*dr_strlen)(const char *s);
	int	(*dr_memcmp)(const void *s1, const void *s2, int n);
	void *	(*dr_memcpy)(void *a, const void *b, uint32 n);
	void *	(*dr_memset)(void *a, uint8 c, uint32 n);
	int32	(*dr_SetBootVar)(uint32 var, void *value);
	void *	(*dr_GetBootVar)(uint32 var);
	void	(*dr_FlushDCacheAll)(void);
	void	(*dr_FlushDCache)(void *addr, uint32 len);
	struct Module *	(*dr_RelocateBinary)(const struct Elf32_Ehdr *buffer, 
			void *(*alloc)(void *allocArg, uint32 size), 
			int32 (*free)(void *allocArg, void *start, uint32 size),
			void *allocArg);
	int32	(*dr_CallBinary)(void *entry, void *arg1, void *arg2, void *arg3, void *arg4, void *arg5);
	struct _3DOBinHeader *	(*dr_Get3DOBinHeader)(const struct Module *);
	int32	(*dr_FillRect)(VideoRect rect, uint32 color);
	uint32	(*dr_GetCurrentOSVersion)(void);
	int32	(*dr_PowerSlot)(uint32 devType);
	uint32	(*dr_ReadPowerBusRegister)(uint32 devType, uint32 regaddr);
	void	(*dr_WritePowerBusRegister)(uint32 devType, uint32 regaddr, uint32 value);
	int32	(*dr_SetPowerBusBits)(uint32 devType, uint32 regaddr, uint32 bits);
	int32	(*dr_ClearPowerBusBits)(uint32 devType, uint32 regaddr, uint32 bits);
	void	(*dr_SetPixel)(VideoPos pos, uint32 color);
	DDDFunction * (*dr_FindDDDFunction)(DDD *ddd, uint32 cmd);
	void	(*dr_InvalidateICache)(void);
	uint32	(*dr_ScaledRand)(uint32 scale);
	int32	(*dr_ReadBytes)(DDDFile *fd, uint32 offset, uint32 size, void *buffer);
	void *	(*dr_ReadRomTagFile)(DDDFile *fd, RomTag *rt, uint32 allocFlags);
	void	(*dr_Delay)(uint32 ms);
	void *	(*dr_ReadAsync)(DDDFile *fd, uint32 blk, uint32 nblks, void *buffer);
	int32	(*dr_WaitRead)(DDDFile *fd, void *id);
	int32	(*dr_ReadSync)(DDDFile *fd, uint32 blk, uint32 nblks, void *buffer);
	int32	(*dr_ReplaceIcon)(VideoImage *icon, DipirHWResource *dev, VideoImage *newIcon, uint32 newSize);
	uint32	(*dr_FindTinyRomTag)(DDDFile *fd, uint8 type, uint32 pos, 
				TinyRomTag **pResult);
	DIPIR_RETURN (*dr_DipirValidateTinyDevice)(DDDFile *fd, 
				DefaultNameFunction *DefaultName);
	void	(*dr_InvalidateDCache)(void *addr, uint32 len);
	void	(*dr_SPutHex)(uint32 value, char *buf, uint32 len);
	void	(*dr_srand)(uint32 seed);
	int32	(*dr_InternalIcon)(uint32 iconID, VideoImage *newIcon, uint32 newSize);
	void	(*dr_WillBoot)(void);
	void	(*dr_TimeStamp)(char *s, void *v);
} DipirRoutines;
