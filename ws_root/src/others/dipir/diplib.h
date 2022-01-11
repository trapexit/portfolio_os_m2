/*
 *	@(#) diplib.h 96/07/16 1.5
 *	Copyright 1996, The 3DO Company
 */

/* BANNER_SIZE is bitmap (300K) + VideoImage header (24) + signature (128). */
#define	BANNER_SIZE	(300*1024 + 24 + 128)

DIPIR_RETURN ReadAndDisplayBanner(DDDFile *fd, void *buffer, uint32 bufSize);

DIPIR_RETURN ReadAndDisplayIcon(DDDFile *fd);

BootInfo *LoadOS(
	int32 (*NextFile)(void *fileCtx, void **pAddr),
	void *fileCtx,
	void *(*Alloc)(void *allocCtx, uint32 size),
	void *allocCtx);

DIPIR_RETURN CheckDiscMode(DDDFile *fd, struct DiscInfo *cdinfo);

DIPIR_RETURN Check3DODisc(DDDFile *fd, struct DiscInfo *cdinfo,
	uint32 num4xTries, uint32 num2xTries);

DIPIR_RETURN RelocateKernel(DDDFile *fd, BootInfo *bootInfo);

BootInfo *LinkCompFile(DDDFile *fd, CompFileHeader *fh, uint32 fileSize);

Boolean ShouldReplaceOS(DDDFile *fd, uint32 myAppPrio);

uint32 DevicePerms(DDDFile *fd);

DIPIR_RETURN VerifySigned(DDDFile *fd, uint32 subsys, uint32 type);

void *AllocScratch(DipirTemp *dt, uint32 size, void *minAddr);

void DipirWillBoot(DipirTemp *dt);
