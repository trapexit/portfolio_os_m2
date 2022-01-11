/*
 *	@(#) insysrom.h 96/07/22 1.39
 *	Copyright 1994,1995,1996 The 3DO Company
 *
 * Main "C" entrypoint and basic dipir logic
 * Definitions of functions resident in the system ROM.
 */

#ifdef DEBUG
#define	PRINTF(args)	printf args
#else
#define	PRINTF(args)
#endif

#ifdef BUILD_STRINGS
#define	EPRINTF(args)	printf args
#else
#define	EPRINTF(args)
#endif

#ifdef DO_TIME
#define	TIMESTAMP(s,v)	TimeStamp(s,(void*)(v))
#else
#define	TIMESTAMP(s,v)
#endif

extern void	printf(const char *fmt, ...);
extern void	ResetTimer(TimerState *tm);
extern uint32	ReadMilliTimer(TimerState *tm);
extern uint32	ReadMicroTimer(TimerState *tm);
extern void	TimeStamp(char *s, void *v);
extern void	ResetDevAndExit(DDDFile *fd);
extern void	Reboot(void *addr, void *arg1, void *arg2, void *arg3);
extern void	HardBoot(void);
extern int32	DisplayImage(VideoImage *image, VideoPos pos, uint32 expand, uint32 attr, uint32 attr2);
extern Boolean	AppExpectingDataDisc(void);

/* In romtag.c */
extern uint32	FindRomTag(DDDFile *fd, uint32 subsys, uint32 type, 
			uint32 pos, struct RomTag *rt);
extern uint32	NextRomTag(DDDFile *fd, uint32 pos, struct RomTag *rt);

/* In alloc.c */
extern void *	DipirAlloc(uint32 size, uint32 allocFlags);
extern void	DipirFree(void *ptr);

/* In dbuffer.c */
extern int32	ReadDoubleBuffer(DDDFile *fd, uint32 startBlock, 
			uint32 readSize, uint32 trailerSize, 
			void *buf1, void *buf2, uint32 bufSize,
			int32 (*Digest)(DDDFile *fd, void *arg, void *buf, uint32 size), 
			void *digestArg,
			int32 (*Final)(DDDFile *fd, void *arg, void *buf, uint32 size),
			void *finalArg);
extern int32	ReadSigned(DDDFile *fd, uint32 offset, uint32 size, 
			void *buffer, KeyID key);

/* In ntgdigest */
extern void	DipirInitDigest(void);
extern void	DipirUpdateDigest(void *buffer, uint32 len);
extern void	DipirFinalDigest(void);

/* In rsadipir.c */
extern int	RSAInit(Boolean indipir);
extern int	RSAFinalWithKey(A_RSA_KEY *key, uchar *sig, int sigLen);
extern int	RSAFinal(uchar *sig, KeyID key);
extern int32	GenRSADigestInit(DipirDigestContext *info);
extern int32	GenRSADigestUpdate(DipirDigestContext *info, 
			uchar *buf, uint32 len);
extern int32	GenRSADigestFinal(DipirDigestContext *info, 
			uchar *sig, uint32 sigLen);

/* In swecc.c */
extern int32	SectorECC(uint8 *buf);

/* In display.c */
extern Boolean	CanDisplay(void);
extern int32	DisplayIcon(VideoImage *icon, uint32 attr, uint32 attr2);
extern int32	DisplayBanner(VideoImage *image, uint32 attr, uint32 attr2);
extern int32	FillRect(VideoRect rect, uint32 color);
extern void	SetPixel(VideoPos pos, uint32 color);
extern int32	ReplaceIcon(VideoImage *icon, DipirHWResource *dev, VideoImage *newIcon, uint32 newSize);
extern int32	InternalIcon(uint32 iconID, VideoImage *newIcon, uint32 newSize);

/* In boot.c */
extern int32	InitBoot(void);
extern uint32	BootAppPrio(void);
extern void	SetBootApp(ExtVolumeLabel *label, uint32 prio);
extern void	SetBootRomAppMedia(DipirHWResource *dev);
extern DipirHWResource * BootRomAppMedia(void);
extern uint32	BootOSVersion(uint32 osflags);
extern struct List *	BootOSAddr(uint32 osflags);
extern uint32	BootOSFlags(uint32 osflags);
extern void	WillBoot(void);
extern void	SetBootOS(ExtVolumeLabel *label, uint32 version, 
			void *addr, void *entryPoint,
			uint32 osflags, uint32 kreserve, uint32 devicePerms,
			DipirHWResource *dipirDev, uint32 dddID, uint32 dipirID);
extern int32	SetBootVar(uint32 var, void *value);
extern void *	GetBootVar(uint32 var);


/* In rommisc.c */
extern void	strncpyz(char *to, char *from, uint32 tolen, uint32 fromlen);
extern int	strcmp(char *s1, char *s2);
extern int	strncmp(char *s1, char *s2, int n);
extern char *	strcat(char *a, const char *b);
extern char *	strcpy(char *a, const char *b);
extern int	strlen(const char *s);
extern int	memcmp(const void *s1, const void *s2, int n);
extern void *	memcpy(void *a, const void *b, uint32 n);
extern void *	memset(void *a, uint8 c, uint32 n);
extern int	strcmp(char *s1, char *s2);

/* In loader.c */
struct Elf32_Ehdr;	/* elf.h */
struct _3DOBinHeader;	/* header3do.h */
struct Module;		/* loader3do.h */
struct List;

extern struct Module *	RelocateBinary(const struct Elf32_Ehdr *buffer, 
			void *(*alloc)(void *allocArg, uint32 size), 
			int32 (*free)(void *allocArg, void *start, uint32 size),
			void *allocArg);
extern int32	CallBinary(void *entry, void *arg1, void *arg2, void *arg3, void *arg4, void *arg5);
extern struct _3DOBinHeader *Get3DOBinHeader(const struct Module *buffer);
extern uint32	GetCurrentOSVersion(void);

/* In chan.powerbus.c */
extern int32	PowerSlot(uint32 devType);
extern uint32	ReadPowerBusRegister(uint32 devType, uint32 regaddr);
extern void	WritePowerBusRegister(uint32 devType, uint32 regaddr, 
			uint32 value);
extern int32	SetPowerBusBits(uint32 devType, uint32 regaddr, uint32 bits);
extern int32	ClearPowerBusBits(uint32 devType, uint32 regaddr, uint32 bits);

/* In ddd.c */
extern DDDFunction * FindDDDFunction(DDD *ddd, uint32 cmd);
extern int32	ReadBytes(DDDFile *fd, uint32 offset, uint32 size, void *buffer);
extern void *	ReadAsync(DDDFile *fd, uint32 blk, uint32 nblks, void *buffer);
extern int32	WaitRead(DDDFile *fd, void *id);
extern int32	ReadSync(DDDFile *fd, uint32 blk, uint32 nblks, void *buffer);

/* In tiny.c */
extern uint32	FindTinyRomTag(DDDFile *fd, uint8 type, uint32 pos, 
				TinyRomTag **pResult);
extern DIPIR_RETURN DipirValidateTinyDevice(DDDFile *fd, 
				DefaultNameFunction *DefaultName);

/* Misc */
extern void	FlushDCacheAll(void);
extern void	FlushDCache(void *addr, uint32 len);
extern void	InvalidateDCache(void *addr, uint32 len);
extern void	InvalidateICache(void);
extern uint32	ScaledRand(uint32 scale);
extern void	srand(uint32 seed);
extern void *	ReadRomTagFile(DDDFile *fd, RomTag *rt, uint32 allocFlags);
extern void	Delay(uint32 ms);
extern void	SPutHex(uint32 value, char *buf, uint32 len);
