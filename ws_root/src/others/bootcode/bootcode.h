/* @(#) bootcode.h 96/09/25 1.32 */

/********************************************************************************
*
*	bootcode.h
*
*	This file contains various defines and declarations needed by the
*	C based bootcode routines.
*
********************************************************************************/

#include <dipir/rom.h>

#define	PRINTVALUE(x,y)	PrintString(x);PrintHexNum(y);PrintString("\n")

#ifdef	DEBUG
#define DBUG(x)		PrintString(x)
#define DBUGPV(x,y)	PRINTVALUE(x,y)
#else
#define DBUG(x)		/* */
#define DBUGPV(x,y)	/* */
#endif

typedef int32 DipirFunction(uint32 (*QuerySysInfo)());


/********************************************************************************
*
*	Prototypes
*
********************************************************************************/

/* bootvectors.s */
extern	void	FlushDCacheAll(void);
extern	void	FlushDCache(uint32 address, uint32 length);
extern	void	InvalidateICache(void);
extern	uint32	DisableInterrupts(void);
extern	void	RestoreInterrupts(uint32 mask);
extern	uint32	PerformSoftReset(void);
extern	void	MemorySet(ubyte value, uint32 target, uint32 length);
extern	void	MemoryMove(uint32 source, uint32 target, uint32 length);
extern	uint32	MemorySize(void);
extern	uint32	ExceptionPreamble;
extern	uint32	ExceptionStart;
extern	uint32	ExceptionEnd;
extern	uint32	FieldCount(uint32 fields);
extern	uint32	CPUCount(uint32);
extern	uint32	SystemROMSize;

/* bootstrap.c */
extern	void	InitBootGlobals(void);
extern	void	SetupExceptionTable(void);

/* bootvideo.c */
extern	void	SetupBootVideo(void);

/* launchdipir.c */
extern	RomTag	*FindROMTag(RomTag *pROMTag, uint32 NumTags, uint8 SubSysType, uint8 Type);
extern	int32	LaunchDipir(void);
extern	void	CleanupForDipir(void);
extern	void	CleanupForNewOS(void);

/* lowlevelio.c */
extern	void	InitIO(void);
extern	void	PutC(char);
extern	int	MayGetChar(void);
extern	void	PrintString(const char *string);
extern	void	PrintHexNum(ulong number);

/* romsysinfo.c */
extern	bool	GetBDAGPIO(uint32 bit, bool direction);
extern	void	SetBDAGPIO(uint32 bit, uint32 value);
extern	uint32	QueryROMSysInfo(uint32 tag, void *info, uint32 size);
extern	uint32	SetROMSysInfo(uint32 tag, void *info, uint32 size);

/* bootalloc.c */
extern	void	AddBootAlloc(void *startp, uint32 size, uint32 flags);
extern	void	DeleteBootAlloc(void *startp, uint32 size, uint32 flags);
extern	uint32	VerifyBootAlloc(void *startp, uint32 size, uint32 flags);
extern	void *	BootAllocate(uint32 size, uint32 flags);
extern	void	DeleteAllBootAllocs(uint32 mask, uint32 flags);
extern	void	InitBootAllocs(void);
extern	void	SanityCheckBootAllocs(void);

/* testdcache.c */
#ifdef TESTDCBI
extern void	TestDCache(void);
#endif


/********************************************************************************
*
*	CDE SYSCONF related defines
*
*	ALL OS code should use SysInfo and its associated defines to access
*	and decode any SYSCONF fields!
*
*	Note that most SYSCONF fields are split out into individual fields
*	within BootGlobals; and that some of the fields are translated along
*	the way.  As a result the decodings of the raw SYSCONF fields do not
*	necessarily match those of the corresponding BootGlobals fields.
*
*	These defines should ONLY be used in the InitBootGlobals routine.
*	ALL other accesses to SYSCONF derived information should get that
*	information from the appropriate BootGlobals field, and should use
*	the defines provided in bootglobals/bootglobals.h or kernal/sysinfo.h
*	to decode that field.
*
********************************************************************************/

#define SC_AUDIO_CONFIG_BITS		0x60000000	/* Audio configuration field mask */
#define SC_AUDIO_CONFIG_SHIFT		29		/* Audio configuration field shift */
#define SC_SYSTEM_TYPE_BITS		0x00078000	/* System type field mask */
#define SC_SYSTEM_TYPE_SHIFT		15		/* System type field shift */
#define SC_DEFAULT_LANGUAGE_BITS	0x00001800	/* Default language field mask */
#define SC_DEFAULT_LANGUAGE_SHIFT	11		/* Default language field shift */
#define SC_VIDEO_ENCODER_BITS		0x0000000C	/* Video encoder field mask */
#define SC_VIDEO_ENCODER_SHIFT		2		/* Video encoder field shift */
#define SC_VIDEO_MODE_BITS		0x00000001	/* Video mode field mask */
#define SC_VIDEO_MODE_SHIFT		0		/* Video mode field shift */

#define SC_DEFAULT_LANGUAGE_US_ENG	0		/* Default language is US/English */
#define SC_DEFAULT_LANGUAGE_JAP		1		/* Default language is Japan/Japanese */
#define SC_DEFAULT_LANGUAGE_UK_ENG	2		/* Default language is UK/English */

#define	SC_VIDEO_MODE_3DO_PAL		1		/* Video mode for PAL on 3DO DENC */
#define	SC_VIDEO_MODE_3DO_NTSC		0		/* Video mode for NTSC on 3DO DENC */

#define	SC_VIDEO_MODE_BT9103_PAL	1		/* Video mode for PAL on BT9103 (or BT851) */
#define	SC_VIDEO_MODE_BT9103_NTSC	0		/* Video mode for NTSC on BT9103 (or BT851) */

#define	SC_VIDEO_MODE_VP536_PAL		0		/* Video mode for PAL on VP536 */
#define	SC_VIDEO_MODE_VP536_NTSC	1		/* Video mode for NTSC on VP536 */


/********************************************************************************
*
*	boot video related defines
*
********************************************************************************/

#define	BV_PAL_WIDTH			768		/* Width of screen in pixels for PAL */
#define	BV_PAL_HEIGHT			576		/* Height of screen in lines for PAL */
#define	BV_PAL_BLANK			16		/* Upper blank region in lines for PAL */

#define	BV_NTSC_WIDTH			640		/* Width of screen in pixels for NTSC */
#define	BV_NTSC_HEIGHT			480		/* Height of screen in lines for NTSC */
#define	BV_NTSC_BLANK			16		/* Upper blank region in lines for NTSC */

#define	BV_PIXEL_BYTES			2		/* Number of bytes per pixel */

#define	BV_HSTART_3DO_PAL		112		/* Horizontal delay for PAL on 3DO DENC */
#define	BV_HSTART_3DO_NTSC		112		/* Horizontal delay for NTSC on 3DO DENC */

#define	BV_HSTART_BT9103_PAL		112		/* Horizontal delay for PAL on BT9103 (or BT851) */
#define	BV_HSTART_BT9103_NTSC		112		/* Horizontal delay for NTSC on BT9103 (or BT851) */

#define	BV_HSTART_VP536_PAL		0x79		/* Horizontal delay for PAL on VP536 */
#define	BV_HSTART_VP536_NTSC		0x65		/* Horizontal delay for NTSC on VP536 */

#define BANDS				4		/* Number of horizontal color bands */
#define COL_BLOCKS			8		/* Number of color blocks per band */
#define BW_BLOCKS			32		/* Number of blocks in black/white band */

#define	CHAR_HEIGHT			16		/* Height of ASCII characters in pixels */
#define CHAR_WIDTH			12		/* Width of ASCII characters in pixels */

#define LOGO_HEIGHT			7		/* Height of 3DO logo in characters */
#define LOGO_WIDTH			6		/* Width of 3DO logo in characters */

#define LOGO_STRING	"\x80\x81\x81\x81\x81\x82\r\x83\x7F\x84\x85\x7F\x86\r"\
			"\x83\x7F\x87\x88\x7F\x86\r\x83\x7F\x89\x8A\x7F\x86\r"\
			"\x83\x7F\x8B\x8C\x7F\x86\r\x8D\x8E\x8F\x90\x91\x86\r"\
			"\x92\x93\x94\x95\x96\x97\r"

#define RNDUP_TOPAGE(x)		((uint32)((x) + XMODE_PAGEMASK) & ~((uint32)XMODE_PAGEMASK))
#define RNDDOWN_TOPAGE(x)	((uint32)(x) & ~((uint32)XMODE_PAGEMASK))

typedef struct {
	ubyte	hcoord;
	ubyte	vcoord;
	ubyte	stringCR;
} charCoords;


