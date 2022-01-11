/*
 *	@(#) m2dipir.c 96/10/08 1.138
 *	Copyright 1994,1995,1996 The 3DO Company
 *
 * Main "C" entrypoint and basic dipir logic
 *
 * Dipir runs in its own little environment and cannot depend on anything
 * else in the system (in the kernel or elsewhere).
 */

#include "kernel/types.h"
#include "kernel/sysinfo.h"
#include "loader/loader3do.h"
#include "hardware/PPCasm.h"
#include "setjmp.h"
#include "dipir.h"
#include "insysrom.h"
#include "hardware/bda.h"
#include "hardware/cde.h"
#include "bootcode/boothw.h"
#ifdef BUILD_DEBUGGER
#include "hardware/debugger.h"
#endif

#ifdef DO_TIME
#define	MAX_TIMESTAMP	64
static struct TimeStamp { uint32 time; char str[4]; void *val; } *TimeStampBuf;
static uint32 TimeStampIndex;
static TimerState tss;

static void InitTimeStamp(void)
{
	TimeStampBuf = DipirAlloc(MAX_TIMESTAMP*sizeof(struct TimeStamp),0);
	EPRINTF(("TimeStamp %x\n", TimeStampBuf));
	TimeStampIndex = 0;
	ResetTimer(&tss);
}

static void TimeStamp(char *s, void *v)
{
	TimeStampBuf[TimeStampIndex].time = ReadMicroTimer(&tss);
	memcpy(TimeStampBuf[TimeStampIndex].str, s, 4);
	TimeStampBuf[TimeStampIndex].val = v;
	if (++TimeStampIndex >= MAX_TIMESTAMP) TimeStampIndex = 0;
}
static void DumpTimeStamp(void)
{
	uint32 i;
	uint32 t;

	for (i = 0;  i < TimeStampIndex;  i++)
	{
		t = (TimeStampBuf[i+1].time - TimeStampBuf[i].time + 500) / 1000;
		EPRINTF(("%8d  (%d.%03d)  %c%c%c%c %x \n",
			TimeStampBuf[i].time, t/1000, t%1000,
			TimeStampBuf[i].str[0], TimeStampBuf[i].str[1],
			TimeStampBuf[i].str[2], TimeStampBuf[i].str[3],
			TimeStampBuf[i].val));
	}
}
#else
static void TimeStamp(char *s, void *v)
{
	TOUCH(s); TOUCH(v);
}
#define	InitTimeStamp()
#define	DumpTimeStamp()
#endif /* DO_TIME */

/*****************************************************************************
  Defines
*/
#define	MAX_DRAMSIZE	(16*1024*1024)

/*****************************************************************************
  Types
*/


/*****************************************************************************
  External variables
*/

extern DipirHWResource *HWResources;

/* List of all Channel Drivers */
extern const ChannelDriver PowerBusChannelDriver;
extern const ChannelDriver SysMemChannelDriver;
extern const ChannelDriver LCCDChannelDriver;
extern const ChannelDriver PCMCIAChannelDriver;
extern const ChannelDriver MicrocardChannelDriver;
extern const ChannelDriver MiscChannelDriver;
#ifdef BRIDGIT
extern const ChannelDriver BridgitChannelDriver;
#endif
#ifdef BUILD_DEBUGGER
extern const ChannelDriver HostChannelDriver;
extern const ChannelDriver HostCDChannelDriver;
#endif

extern DDD ChannelDDD;

/*****************************************************************************
  External functions
*/

extern void PreProbe(void);
extern void PostProbe(void);
extern void InitECC(void);
extern void InitPutChar(void);
extern void InitBuffers(void);
extern void InitDipirAlloc(void);
extern void InitIcons(void);
extern void BootOS(void);
extern void BootRomApp(void);
extern void InitForBoot(void);
extern uint32 GetIBR(void);
extern void InitTimer(void);
extern void RestoreTimer(void);
extern Boolean DeviceBlockedError(void);
extern void DeleteHWResource(DipirHWResource *dev);

/*****************************************************************************
  Local functions - forward declarations
*/


/*****************************************************************************
  Local Data
*/

DipirTemp *dtmp;

/* Note: we use theBootGlobals instead of dtmp->dt_BootGlobals in any code
 * that can be called directly from the OS (not via SoftReset), such as
 * ChannelRead.  When called dipir code is called directly, dtmp is invalid. */
bootGlobals *theBootGlobals;



static const DipirRoutines dr =
{
	DIPIR_ROUTINES_VERSION_0+0,
	printf,
	ResetTimer,
	ReadMilliTimer,
	ReadMicroTimer,
	Reboot,
	HardBoot,
	DisplayImage,
	AppExpectingDataDisc,
	NextRomTag,
	FindRomTag,
	DipirAlloc,
	DipirFree,
	ReadDoubleBuffer,
	ReadSigned,
	DipirInitDigest,
	DipirUpdateDigest,
	DipirFinalDigest,
	RSAInit,
	RSAFinalWithKey,
	RSAFinal,
	GenRSADigestInit,
	GenRSADigestUpdate,
	GenRSADigestFinal,
	SectorECC,
	CanDisplay,
	DisplayIcon,
	DisplayBanner,
	InitBoot,
	BootAppPrio,
	SetBootApp,
	SetBootRomAppMedia,
	BootRomAppMedia,
	BootOSVersion,
	BootOSAddr,
	BootOSFlags,
	SetBootOS,
	strncpyz,
	strcmp,
	strncmp,
	strcat,
	strcpy,
	strlen,
	memcmp,
	memcpy,
	memset,
	SetBootVar,
	GetBootVar,
	FlushDCacheAll,
	FlushDCache,
	RelocateBinary,
	CallBinary,
	Get3DOBinHeader,
	FillRect,
	GetCurrentOSVersion,
	PowerSlot,
	ReadPowerBusRegister,
	WritePowerBusRegister,
	SetPowerBusBits,
	ClearPowerBusBits,
	SetPixel,
	FindDDDFunction,
	InvalidateICache,
	ScaledRand,
	ReadBytes,
	ReadRomTagFile,
	Delay,
	ReadAsync,
	WaitRead,
	ReadSync,
	ReplaceIcon,
	FindTinyRomTag,
	DipirValidateTinyDevice,
	InvalidateDCache,
	SPutHex,
	srand,
	InternalIcon,
	WillBoot,
	TimeStamp,
};

static const PublicDipirRoutines pdr =
{
	(sizeof(pdr) / sizeof(void *)),
	SectorECC,
	GenRSADigestInit,
	GenRSADigestUpdate,
	GenRSADigestFinal,
	GetHWResource,
	ChannelRead,
	ChannelMap,
	ChannelUnmap,
	GetHWIcon,
	SoftReset,
	memcpy,
	memset,
};

static const ChannelDriver *const ChannelDrivers[] =
{
	&PowerBusChannelDriver,
	&SysMemChannelDriver,
	&MiscChannelDriver,
	&MicrocardChannelDriver,
	&PCMCIAChannelDriver,
#ifdef BRIDGIT
	&BridgitChannelDriver,
#endif
	&LCCDChannelDriver,
#ifdef BUILD_DEBUGGER
	&HostChannelDriver,
	&HostCDChannelDriver,
#endif
};
#define	NUM_CHANNEL_DRIVERS	(sizeof(ChannelDrivers)/sizeof(*ChannelDrivers))

/*****************************************************************************
 Table of exceptions.
 We want to trap certain exceptions if they occur while in dipir.
*/

const uint8 DipirExceptions[NUM_DIPIR_EXCEPTIONS] =
{
	MACHINE_CHK,	/* machine check exception */
};


/*****************************************************************************
  Code follows...
*****************************************************************************/


/*****************************************************************************
  Delay for a specified number of milliseconds in a busy loop.
*/
void
Delay(uint32 ms)
{
	TimerState tm;

	ResetTimer(&tm);
	while (ReadMilliTimer(&tm) < ms)
		continue;
}

/*****************************************************************************
 DisallowUnblocks
 Don't allow anyone to unblock devices (until the next SoftReset).
*/
	static void
DisallowUnblocks(void)
{
	uint32 i;
	const ChannelDriver *cd;

	for (i = 0;  i < NUM_CHANNEL_DRIVERS;  i++)
	{
		cd = ChannelDrivers[i];
		(*cd->cd_ChannelControl)(CHAN_DISALLOW_UNBLOCK);
	}
}

/*****************************************************************************
 ProbeChannelDrivers
 Probe all channels for devices present.
*/
	static void
ProbeChannelDrivers(void)
{
	uint32 i;
	const ChannelDriver *cd;

	PreProbe();
	for (i = 0;  i < NUM_CHANNEL_DRIVERS;  i++)
	{
		cd = ChannelDrivers[i];
		TIMESTAMP("Prbc",cd);
		(*cd->cd_Probe)();
	}
	TIMESTAMP("PstP",0);
	PostProbe();
}


/*****************************************************************************
 AppExpectingDataDisc
*/
	Boolean
AppExpectingDataDisc(void)
{
	return (theBootGlobals->bg_DipirFlags & DF_EXPECT_DATADISC) != 0;
}

#ifdef DEBUG
/*****************************************************************************
*/
void
DumpBytes(char *str, uchar *buffer, uint32 size)
{
	int i;

	printf("=== %s ===\n", str);
	for (i = 0;  i < size;  i++)
	{
		printf("%02x ", buffer[i]);
		if ((i % 16) == 15)  printf("\n");
	}
	if ((size % 16) != 0)
		printf("\n");
}
#endif /* DEBUG */

/*****************************************************************************
*/
	Boolean
SysROMNewer(RomTag *rt, RomTag *sysrt)
{
	uint32 pos;

	for (pos = 0;;)
	{
		pos = FindRomTag(dtmp->dt_SysRomFd, rt->rt_SubSysType, rt->rt_Type,
			pos, sysrt);
		if (pos <= 0)
			break;
		if (sysrt->rt_ComponentID != rt->rt_ComponentID)
			continue;
		if (MakeInt16(sysrt->rt_Version, sysrt->rt_Revision) >=
		    MakeInt16(rt->rt_Version, rt->rt_Revision))
			return TRUE;
	}
	return FALSE;

}

/*****************************************************************************
*/
	void *
ReadRomTagFile(DDDFile *fd, RomTag *rt, uint32 allocFlags)
{
	void *buffer;
	int32 n;

	if (rt->rt_Offset == 0)
		/* The file is not in this device. */
		return NULL;
	buffer = DipirAlloc(rt->rt_Size, fd->fd_AllocFlags | allocFlags);
	if (buffer == NULL)
		return NULL;
	n = ReadBytes(fd,
		(fd->fd_RomTagBlock + rt->rt_Offset) * fd->fd_BlockSize,
		rt->rt_Size, buffer);
	if (n != rt->rt_Size)
	{
		EPRINTF(("ReadRTF: bad read %x != %x\n", n, rt->rt_Size));
		DipirFree(buffer);
		return NULL;
	}
	return buffer;
}

/*****************************************************************************
*/
	void
FreeDDD(DDD *ddd)
{
	TOUCH(ddd);
	PRINTF(("FreeDDD(%x)\n", ddd));
}

/*****************************************************************************
*/
	DDD *
LoadDDD(DDDFile *fd, RomTag *rt)
{
	void *buffer;
	DDD *ddd;
	struct Module *module;
	int ok;

	/* Read the DDD into a buffer. */
	PRINTF(("LoadDDD dev %s\n", fd->fd_HWResource->dev.hwr_Name));
	buffer = ReadRomTagFile(fd, rt, ALLOC_EXEC);
	if (buffer == NULL)
	{
		PRINTF(("LoadDDD: No RomTag\n"));
		return NULL;
	}

	if ((fd->fd_Flags & DDD_SECURE) == 0)
	{
		DipirInitDigest();
		DipirUpdateDigest(buffer, rt->rt_Size - KeyLen(KEY_128));
		DipirFinalDigest();
		/* Signature is at the end of the device dipir. */
		ok = RSAFinal(((uint8*)buffer) + rt->rt_Size - KeyLen(KEY_128),
			KEY_128);
		PRINTF(("LoadDDD: ok %x\n", ok));
		if (!ok)
		{
			DipirFree(buffer);
			return NULL;
		}
	}
	module = RelocateBinary(buffer, NULL, NULL, NULL);
	DipirFree(buffer);
	if (module == NULL)
		return NULL;
	ddd = (DDD *) CallBinary(module->li->entryPoint,
			(void*)dtmp, (void*)0, (void*)0, (void*)0, (void*)0);
	PRINTF(("LoadDDD(%x): ret %x\n", fd, ddd));
	return ddd;
}

/*****************************************************************************
*/
	DIPIR_RETURN
CallDeviceDipir(DDDFile *codefd, DDDFile *devfd, RomTag *rt,
		uint32 cmd, void *arg, uint32 dddID)
{
	DIPIR_RETURN ret;
	void *buffer;
	struct Module *module;
	int ok;

	PRINTF(("CallDeviceDipir(%x,%x,%x,%x,%x,%x)\n",
		codefd, devfd, rt, cmd, arg, dddID));
	buffer = ReadRomTagFile(codefd, rt, ALLOC_EXEC);
	PRINTF((" code at %x\n", buffer));
	if (buffer == NULL)
		return DIPIR_RETURN_TROJAN;

	if ((codefd->fd_Flags & DDD_SECURE) == 0)
	{
		/*
		 * Check the signature.
		 * Signature covers label + RTT + device dipir.
		 */
		PRINTF(("Check sig\n"));
		DipirInitDigest();
		DipirUpdateDigest((void*)devfd->fd_VolumeLabel,
			sizeof(ExtVolumeLabel));
		DipirUpdateDigest((void*)devfd->fd_RomTagTable,
			devfd->fd_VolumeLabel->dl_NumRomTags * sizeof(RomTag));
		DipirUpdateDigest(buffer, rt->rt_Size - KeyLen(KEY_128));
		DipirFinalDigest();
		/* Signature is at the end of the device dipir. */
		ok = RSAFinal(((uint8*)buffer) + rt->rt_Size - KeyLen(KEY_128),
			KEY_128);
		if (!ok)
		{
			DipirFree(buffer);
			return DIPIR_RETURN_TROJAN;
		}
	}
	module = RelocateBinary(buffer, NULL, NULL, NULL);
	DipirFree(buffer);
	if (module == NULL)
		return DIPIR_RETURN_TROJAN;
	ret = (DIPIR_RETURN)
		CallBinary(module->li->entryPoint, (void*)devfd, (void*)cmd,
			   (void*)arg,
			   (void*)rt->rt_ComponentID, (void*)dddID);
	return ret;
}

/*****************************************************************************
*/
	DIPIR_RETURN
CallAllDeviceDipirs(DDDFile *devfd, uint32 cmd, void *arg, uint32 dddID)
{
	DIPIR_RETURN ret;
	uint32 ndipirs;
	uint32 pos;
	RomTag devrt;
	RomTag sysrt;

	if (devfd->fd_Flags & DDD_SECURE)
		return DIPIR_RETURN_OK;

	/* Assert (devfd->fd_RomTagTable != NULL); */
	/* For each dipir in the device, call it. */
	ndipirs = 0;
	for (pos = 0;;)
	{
		/* Find next Device Dipir in the device */
		pos = FindRomTag(devfd, RSANODE, RSA_M2_DEVDIPIR, pos, &devrt);
		if (pos == 0)
			break;
		/* See if system ROM has a newer copy of this device dipir */
		if (SysROMNewer(&devrt, &sysrt))
		{
			PRINTF(("Calling dipir %x from system ROM\n",
				devrt.rt_ComponentID));
			ret = CallDeviceDipir(dtmp->dt_SysRomFd, devfd, &sysrt,
						cmd, arg, dddID);
		} else
		{
			PRINTF(("Calling dipir %x from device\n",
				devrt.rt_ComponentID));
			ret = CallDeviceDipir(devfd, devfd, &devrt,
						cmd, arg, dddID);
		}
		if (ret < 0)
			return ret;
		ndipirs++;
	}
	return DIPIR_RETURN_OK;
}

/*****************************************************************************
*/
	DIPIR_RETURN
ValidateLabel(DDDFile *fd)
{
	int ok;
	void *sig;

	PRINTF(("ValidateLabel\n"));
	if (fd->fd_VolumeLabel == NULL ||
	    fd->fd_RomTagTable == NULL)
	{
		/*
		 * Device has no valid label.
		 * It's RomApp media if device supports RomApp;
		 * otherwise reject it.
		 */
		if (fd->fd_DDD != &ChannelDDD &&
		    (fd->fd_HWResource->dev_Flags & HWR_ROMAPP_OK))
			return DIPIR_RETURN_ROMAPPDISC;
		EPRINTF(("No label on non-ROMAPP device %s\n",
			fd->fd_HWResource->dev.hwr_Name));
		return DIPIR_RETURN_TROJAN;
	}

	if (fd->fd_Flags & DDD_SECURE)
		return DIPIR_RETURN_OK;

	/*
	 * Sign-check the VolumeLabel and RomTag table
	 * using the larger (128 byte) 3DO key.
	 */
	DipirInitDigest();
	DipirUpdateDigest((void*)fd->fd_VolumeLabel,
		sizeof(ExtVolumeLabel));
	DipirUpdateDigest((void*)fd->fd_RomTagTable,
		fd->fd_VolumeLabel->dl_NumRomTags * sizeof(RomTag));
	DipirFinalDigest();
	/*
	 * Signature is at the end of the RomTag table.
	 * If this is an Opera-compatible device, there is an Opera signature
	 * immediately after the RomTags; the M2 signature follows it.
	 * Otherwise, the M2 signature is immediately after the RomTags.
	 */
	sig = fd->fd_RomTagTable + fd->fd_VolumeLabel->dl_NumRomTags;
	if ((fd->fd_VolumeLabel->dl_VolumeFlags & VF_M2ONLY) == 0)
	{
		/* Skip over the terminating RomTag and the Opera signature */
		sig = ((uint8*)sig) + sizeof(RomTag) + KeyLen(KEY_THDO_64);
	}
	PRINTF(("Check sig at %x, rtt %x\n", sig, fd->fd_RomTagTable));
	ok = RSAFinal(sig, KEY_128);
	PRINTF(("ValidateLabel ok=%x\n", ok));
	if (!ok)
		return DIPIR_RETURN_TROJAN;

	return DIPIR_RETURN_OK;
}

/*****************************************************************************
*/
	DIPIR_RETURN
ValidateTinyDevice(DDDFile *devfd, uint32 cmd, void *arg)
{
	RomTag rt;
	RomTag sysrt;

	rt.rt_SubSysType = RSANODE;
	rt.rt_Type = RSA_M2_DEVDIPIR;
	rt.rt_Version = rt.rt_Revision = 0;
	rt.rt_ComponentID = devfd->fd_TinyVolumeLabel->tl_DipirID;
	if (!SysROMNewer(&rt, &sysrt))
	{
		/* Must find the device dipir in the system ROM. */
		EPRINTF(("Tiny dipir %x not in sys ROM\n", rt.rt_ComponentID));
		return DIPIR_RETURN_TROJAN;
	}
	PRINTF(("Calling TINY dipir %x from system ROM\n",
		rt.rt_ComponentID));
	return CallDeviceDipir(dtmp->dt_SysRomFd, devfd, &sysrt, cmd, arg, 0);
}

/*****************************************************************************
*/
	DIPIR_RETURN
ValidateDevice(DipirHWResource *dev, uint32 cmd, void *arg)
{
	DIPIR_RETURN ret;
	DDDFile *romfd;
	DDDFile *devfd = NULL;
	uint32 pos;
	DDD *ddd;
	RomTag sysrt;
	RomTag devrt;

	/* Open the device ROM. */
	romfd = OpenDipirDevice(dev, &ChannelDDD);
	if (romfd == NULL || romfd == IGNORE_DEVICE)
	{
		EPRINTF(("ValidateDevice: cannot open %s\n",
			dev->dev.hwr_Name));
		return DIPIR_RETURN_TROJAN;
	}
	if (romfd->fd_Flags & DDD_TINY)
	{
		/* Special case for "tiny" device. */
		PRINTF(("ValidateDevice: TINY ROM\n"));
		ret = ValidateTinyDevice(romfd, cmd, arg);
		CloseDipirDevice(romfd);
		return ret;
	}
	ret = ValidateLabel(romfd);
	if (ret < 0)
	{
		EPRINTF(("Invalid label (%x) on %s\n",
			ret, dev->dev.hwr_Name));
		return ret;
	}
	/* Call dipir(s) in the device ROM to validate device ROM. */
	ret = CallAllDeviceDipirs(romfd, DIPIR_VALIDATE, 0, 0);
	if (ret < 0)
	{
		EPRINTF(("ROM failed (%x) on %s\n",
			ret, dev->dev.hwr_Name));
		return ret;
	}

	/* For each DDD in the device ROM, call all device dipirs. */
	for (pos = 0;;)
	{
		/* Find next DDD in the device ROM. */
		pos = FindRomTag(romfd, RSANODE, RSA_M2_DRIVER, pos, &devrt);
		PRINTF(("Driver RT @ %x, id %x\n", pos, devrt.rt_ComponentID));
		if (pos == 0)
		{
			CloseDipirDevice(romfd);
			return DIPIR_RETURN_OK;
		}
		/* Load the DDD. */
		/* See if system ROM has a newer copy of this DDD. */
		if (SysROMNewer(&devrt, &sysrt))
		{
			PRINTF(("Loading driver %x from system ROM\n",
				devrt.rt_ComponentID));
			ddd = LoadDDD(dtmp->dt_SysRomFd, &sysrt);
		} else
		{
			PRINTF(("Loading driver %x from device\n",
				devrt.rt_ComponentID));
			ddd = LoadDDD(romfd, &devrt);
		}
		if (ddd == NULL)
		{
			EPRINTF(("Cannot load DDD\n"));
			ret = DIPIR_RETURN_TROJAN;
			break;
		}
		/* Call all device dipirs on the device. */
		devfd = OpenDipirDevice(dev, ddd);
		if (devfd == NULL)
		{
			/*
			 * Cannot open the device.
			 */
			EPRINTF(("Cannot open device %s\n", dev->dev.hwr_Name));
			ret = DIPIR_RETURN_TROJAN;
			break;
		}
		if (devfd == IGNORE_DEVICE)
		{
			/*
			 * May be something like a CD with no disc.
			 * Block the device just to be sure, and continue.
			 */
			PRINTF(("Ignore device\n"));
			if ((*dev->dev_ChannelDriver->cd_DeviceControl)
				(dev, CHAN_BLOCK) < 0)
			{
				EPRINTF(("Cannot BLOCK device; reboot!\n"));
				HardBoot();
			}
			ret = DIPIR_RETURN_NODISC;
			break;
		}
		ret = ValidateLabel(devfd);
		if (ret < 0)
		{
			EPRINTF(("Device label failed (%x) on %s\n",
				ret, dev->dev.hwr_Name));
			break;
		}
		if (ret == DIPIR_RETURN_ROMAPPDISC)
		{
			PRINTF(("ROMAPP media\n"));
#ifdef DEBUG
			/* Try special MEDIA_DEBUG dipir. */
			devrt.rt_SubSysType = RSANODE;
			devrt.rt_Type = RSA_M2_DEVDIPIR;
			devrt.rt_ComponentID = DIPIRID_MEDIA_DEBUG;
			devrt.rt_Version = devrt.rt_Revision = 0;
			if (SysROMNewer(&devrt, &sysrt))
			{
				(void) CallDeviceDipir(dtmp->dt_SysRomFd,
						devfd, &sysrt,
						DIPIR_VALIDATE, 0,
						devrt.rt_ComponentID);
			} else
			{
				PRINTF(("MEDIA_DEBUG dipir not found\n"));
			}
#endif /* DEBUG */
			SetBootRomAppMedia(dev);
		} else
		{
			ret = CallAllDeviceDipirs(devfd, cmd, arg,
						devrt.rt_ComponentID);
			if (ret < 0)
				break;
		}
		CloseDipirDevice(devfd);
		FreeDDD(ddd);
	}

	if (ret < 0)
	{
		EPRINTF(("Device %s failed\n", dev->dev.hwr_Name));
		if (devfd != NULL)
		{
			/* EjectDevice(devfd); */
			CloseDipirDevice(devfd);
		}
	}
	if (ddd != NULL)
	{
		FreeDDD(ddd);
	}
	CloseDipirDevice(romfd);
	return ret;
}

/*****************************************************************************
*/
	DIPIR_RETURN
ReCallDeviceDipir(DipirHWResource *dev, uint32 dddID, uint32 dipirID,
		uint32 cmd, void *arg)
{
	DIPIR_RETURN ret;
	DDDFile *romfd;
	DDDFile *devfd = NULL;
	DDD *ddd = NULL;
	uint32 pos;
	RomTag sysrt;
	RomTag devrt;

	PRINTF(("ReCallDeviceDipir(%x,%x,%x,%x,%x) [%s]\n",
		dev, dddID, dipirID, cmd, arg, dev->dev.hwr_Name));
	/* Open the device ROM. */
	romfd = OpenDipirDevice(dev, &ChannelDDD);
	if (romfd == NULL ||
	    romfd == IGNORE_DEVICE ||
	    romfd->fd_VolumeLabel == NULL ||
	    romfd->fd_RomTagTable == NULL)
	{
		PRINTF((" cannot open ROM\n"));
		goto Trojan;
	}
	if (romfd->fd_Flags & DDD_TINY)
	{
		PRINTF((" ReCall TINY ROM\n"));
		ret = ValidateTinyDevice(romfd, cmd, arg);
		CloseDipirDevice(romfd);
		return ret;
	}

	if (dddID == 0)
	{
		ddd = &ChannelDDD;
	} else
	{
		/* Find the right DDD in the device ROM. */
		for (pos = 0;;)
		{
			/* Find next DDD in the device ROM. */
			pos = FindRomTag(romfd, RSANODE, RSA_M2_DRIVER, pos, &devrt);
			if (pos == 0)
				goto Trojan;
			if (devrt.rt_ComponentID == dddID || dddID == 0)
				break;
		}
		/* Found the DDD.  Load it. */
		/* See if system ROM has a newer copy of this DDD. */
		if (SysROMNewer(&devrt, &sysrt))
			ddd = LoadDDD(dtmp->dt_SysRomFd, &sysrt);
		else
			ddd = LoadDDD(romfd, &devrt);
		if (ddd == NULL)
			goto Trojan;
	}
	/* Find the bootable device dipir on the device. */
	devfd = OpenDipirDevice(dev, ddd);
	if (devfd == NULL || devfd == IGNORE_DEVICE)
		goto Trojan;
	ret = ValidateLabel(devfd);
	if (ret < 0)
		goto Exit;
	/* Find Device Dipir in the device */
	for (pos = 0;;)
	{
		pos = FindRomTag(devfd, RSANODE, RSA_M2_DEVDIPIR, pos, &devrt);
		if (pos == 0)
			goto Trojan;
		if (devrt.rt_ComponentID == dipirID || dipirID == 0)
			break;
	}
	/* Found the Dipir.  Call it. */
	/* See if system ROM has a newer copy of this dipir */
	if (SysROMNewer(&devrt, &sysrt))
		ret = CallDeviceDipir(dtmp->dt_SysRomFd, devfd, &sysrt, cmd, arg, dddID);
	else
		ret = CallDeviceDipir(devfd, devfd, &devrt, cmd, arg, dddID);
Exit:
	if (devfd != NULL)
		CloseDipirDevice(devfd);
	if (ddd != NULL && ddd != &ChannelDDD)
		FreeDDD(ddd);
	if (romfd != NULL)
		CloseDipirDevice(romfd);
	PRINTF(("ReCall: ret %x\n", ret));
	return ret;

Trojan:
	PRINTF(("ReCall: trojan\n"));
	ret =  DIPIR_RETURN_TROJAN;
	goto Exit;
}

/*****************************************************************************
 Initialize some stuff we need to get from SysInfo.
*/
static int32
InitSysInfo(void)
{
	uint32 ret;
	SystemInfo si;
	SysCacheInfo ci;

	ret = (*dtmp->dt_QueryROMSysInfo)(SYSINFO_TAG_SYSTEM, &si, sizeof(si));
	if (ret != SYSINFO_SUCCESS)
		return -1;
	dtmp->dt_BusClock = si.si_BusClkSpeed;

	ret = (*dtmp->dt_QueryROMSysInfo)(SYSINFO_TAG_CACHE, &ci, sizeof(ci));
	if (ret != SYSINFO_SUCCESS)
		return -1;
	dtmp->dt_CacheLineSize =
		max(ci.sci_PICacheLineSize, ci.sci_PDCacheLineSize);
	PRINTF(("BusClock %d, cache %d\n",
		dtmp->dt_BusClock, dtmp->dt_CacheLineSize));
	return 0;
}

/*****************************************************************************
 Initialize dipir flags.
*/
static void
InitDipirControl(void)
{
	/* If the DIPIR_SPECIAL bit is set, set the DC_NOKEY bit. */
	if (CDE_READ(CDE_BASE, CDE_VISA_DIS) & CDE_DIPIR_SPECIAL)
	{
		theBootGlobals->bg_DipirControl |= DC_NOKEY;
	}
}

/*****************************************************************************
 Initialize some miscellaneous stuff in the dipir environment.
*/
int32
InitDipirEnv(void)
{
	DipirHWResource *dev;

	for (dev = HWResources;  dev != NULL;  dev = dev->dev_Next)
	{
		if (strcmp(dev->dev.hwr_Name, "SYSROM") == 0)
			break;
	}
	if (dev == NULL)
	{
		/* No system ROM! */
		EPRINTF(("No system ROM!\n"));
		return -1;
	}
	dtmp->dt_SysRomFd = OpenDipirDevice(dev, &ChannelDDD);
	if (dtmp->dt_SysRomFd == NULL ||
	    dtmp->dt_SysRomFd == IGNORE_DEVICE ||
	    dtmp->dt_SysRomFd->fd_VolumeLabel == NULL ||
	    dtmp->dt_SysRomFd->fd_RomTagTable == NULL)
	{
		/* Can't open system ROM! */
		EPRINTF(("Cannot open system ROM!\n"));
		return -1;
	}

	if (InitBoot() < 0)
	{
		EPRINTF(("Cannot init ROMApp OS\n"));
		return -1;
	}

	InitDipirControl();
	return 0;
}

/*****************************************************************************
*/
void
DeinitDipirEnv(void)
{
	if (dtmp->dt_SysRomFd != NULL)
		CloseDipirDevice(dtmp->dt_SysRomFd);
	DisallowUnblocks();
	dtmp = 0; /* Fault if any accesses to dtmp after this. */
}

/*****************************************************************************
  Reboot
  Restart to the bootROM or OS or...
*/
void
Reboot(void *addr, void *arg1, void *arg2, void *arg3)
{
	PRINTF(("Reboot: addr %x, args (%x,%x,%x)\n", addr, arg1, arg2, arg3));
	/* Do hardware cleanup (reinitialization). */
	InitForBoot();
	/* Exit dipir. */
	DeinitDipirEnv();
	/* Jump to the new OS. */
	RestartToXXX(addr, arg1, arg2, arg3);
	/*NOTREACHED*/
}

/*****************************************************************************
 Setup certain exception vectors to trap into dipir.
*/
void
CatchExceptions(void)
{
	uint32 exc;
	uint32 ibr = GetIBR();

	extern char CatchException[];
	extern char CatchExceptionEnd[];

#ifdef DEBUG
	if (CatchExceptionEnd - CatchException != EXCEPTION_SIZE)
	{
		EPRINTF(("DIPIR: size error CatchException %x %x\n",
			CatchExceptionEnd - CatchException, EXCEPTION_SIZE));
		HardBoot();
	}
#ifdef DIPIR_INTERRUPTS
	if (CatchInterruptEnd - CatchInterrupt != INTERRUPT_SIZE)
	{
		EPRINTF(("DIPIR: size error CatchInterrupt %x %x\n",
			CatchInterruptEnd - CatchInterrupt, INTERRUPT_SIZE));
		HardBoot();
	}
#endif /* DIPIR_INTERRUPTS */
#endif /* DEBUG */

	/* Save old exception vector, and install our own handler. */
	for (exc = 0;  exc < NUM_DIPIR_EXCEPTIONS;  exc++)
	{
		memcpy(&dtmp->dt_SavedExceptions[exc].exc_Code,
			(void*)(ibr + DipirExceptions[exc]*0x100),
			EXCEPTION_SIZE);
		memcpy((void*)(ibr + DipirExceptions[exc]*0x100),
			CatchException,
			EXCEPTION_SIZE);
	}

#ifdef DIPIR_INTERRUPTS
	/* Same thing for interrupts, but point to a different handler. */
	{
		extern char CatchInterrupt[];
		extern char CatchInterruptEnd[];

		memcpy(dtmp->dt_SavedIntrCode,
			(void*)(ibr + EXTERNAL_INT*0x100),
			INTERRUPT_SIZE);
		memcpy((void*)(ibr + EXTERNAL_INT*0x100),
			CatchInterrupt,
			INTERRUPT_SIZE);
	}
#endif /* DIPIR_INTERRUPTS */

	FlushDCacheAll();
	InvalidateICache();
}

/*****************************************************************************
 Restore old exception vectors.
*/
void
RestoreExceptions(void)
{
	uint32 exc;
	uint32 ibr = GetIBR();

	for (exc = 0;  exc < NUM_DIPIR_EXCEPTIONS;  exc++)
	{
		memcpy((void*)(ibr + DipirExceptions[exc]*0x100),
			&dtmp->dt_SavedExceptions[exc].exc_Code,
			EXCEPTION_SIZE);
	}

#ifdef DIPIR_INTERRUPTS
	{
		memcpy((void*)(ibr + EXTERNAL_INT*0x100),
			dtmp->dt_SavedIntrCode,
			INTERRUPT_SIZE);
	}
#endif /* DIPIR_INTERRUPTS */

	FlushDCacheAll();
	InvalidateICache();
}

#ifdef DIPIR_INTERRUPTS
/*****************************************************************************
 Enable certain interrupts.
*/
void
SetupInterrupts(void)
{
	/* Enable the VBLANK interrupt (interrupt at line 1). */
	dtmp->dt_SaveVint = BDA_READ(BDAVDU_VINT);
	BDA_WRITE(BDAVDU_VINT, (dtmp->dt_SaveVint & ~BDAVLINE0_MASK) | 0x00100000);
	dtmp->dt_SaveInterruptMask = BDA_READ(BDAPCTL_PBINTENSET);
	BDA_WRITE(BDAPCTL_PBINTENSET, BDAINT_VINT0_MASK);
	EnableInterrupts();
}

/*****************************************************************************
 Restore state of interrupts previously enabled by SetupInterrupts.
*/
void
RestoreInterrupts(void)
{
	/* Restore the state of the VBLANK interrupt. */
	DisableInterrupts();
	BDA_WRITE(BDAVDU_VINT, dtmp->dt_SaveVint);
	BDA_CLR(BDAPCTL_PBINTENSET, ~0);
	BDA_WRITE(BDAPCTL_PBINTENSET, dtmp->dt_SaveInterruptMask);
}
#endif /* DIPIR_INTERRUPTS */

/*****************************************************************************
*/
static void
InitDtmp(uint32 (*QueryROMSysInfo)(uint32 tag, void *info, uint32 size))
{
	bootGlobals	*bg;
	BootAlloc	*ba;
	uint32		result;

	result = (*QueryROMSysInfo)(SYSINFO_TAG_BOOTGLOBALS,
			(void *)&bg, sizeof(bg));
	if (result != SYSINFO_SUCCESS)
	{
		/* EPRINTF(("Error getting bootglobals pointer!\n")); */
		HardBoot();
	}

	dtmp = NULL;
	ScanList(&bg->bg_BootAllocs, ba, BootAlloc)
	{
		if (ba->ba_Flags & BA_DIPIR_PRIVATE)
		{
			if (ba->ba_Size < MIN_PRIVATE_BUF_SIZE)
			{
				/* EPRINTF(("Dtmp too small (%x)\n", ba->ba_Size)); */
				HardBoot();
			}
			dtmp = ba->ba_Start;
			dtmp->dt_PrivateBufSize = ba->ba_Size;
			break;
		}
	}
	/* Make sure dtmp is a sane pointer. */
	if (dtmp == NULL)
	{
		/* EPRINTF(("No dtmp!\n")); */
		HardBoot();
	}
	if (((uint32)dtmp & 0x3) ||
	    (uint32)dtmp < DRAMSTART ||
	    (uint32)dtmp + sizeof(DipirTemp) >= DRAMSTART + MAX_DRAMSIZE)
	{
		/* EPRINTF(("Bad dtmp!\n")); */
		HardBoot();
	}

	dtmp->dt_Version = 1;
	dtmp->dt_DipirRoutines = &dr;
	dtmp->dt_QueryROMSysInfo = QueryROMSysInfo;
	dtmp->dt_BootGlobals = theBootGlobals = bg;
	dtmp->dt_Flags = 0;
}

/*****************************************************************************
*/
static void
InitBootGlobals(void)
{
	theBootGlobals->bg_DipirRtns = (void *) &pdr;
}

/*****************************************************************************
 Dipir
 On entry, dtmp already points to the DipirTemp area.
*/
DIPIR_RETURN
Dipir(uint32 (*QueryROMSysInfo)(uint32 tag, void *info, uint32 size))
{
	DIPIR_RETURN ret;
	DipirHWResource *dev;
	Boolean doubleChecked = FALSE;
	int jmp;

	InitDtmp(QueryROMSysInfo);
	InitBootGlobals();
	InitPutChar();
	PRINTF(("Welcome to Dipir (control=%x)\n",
		theBootGlobals->bg_DipirControl));
	CatchExceptions();
#ifdef DIPIR_INTERRUPTS
	SetupInterrupts();
#endif /* DIPIR_INTERRUPTS */
	InitDipirAlloc();
	InitTimeStamp();
	TIMESTAMP("Entr",0);
	/* hard reset? */
	if (CDE_READ(CDE_BASE, CDE_VISA_DIS) & CDE_NOT_1ST_DIPIR)
	{
		theBootGlobals->bg_DipirFlags &= ~DF_HARD_RESET;
	} else
	{
		theBootGlobals->bg_DipirFlags |= DF_HARD_RESET;
		/* Boot-time initializations. */
		InitHWResources();
		InitIcons();
		CDE_WRITE(CDE_BASE, CDE_VISA_DIS, CDE_NOT_1ST_DIPIR);
	}

	InitECC();
	InitTimer();
	InitBuffers();
	srand(0); /* Uses HardwareRandomNumber */
	if (InitSysInfo() < 0)
	{
		EPRINTF(("InitSysInfo failed\n"));
		HardBoot();
	}

	/* Call WillBoot() as early as possible. */
	if (theBootGlobals->bg_KernelAddr == 0)
		WillBoot();

	jmp = setjmp(dtmp->dt_JmpBuf);
	if (jmp)
	{
		EPRINTF(("DIPIR: Got longjmp during probe\n"));
		if (jmp == JMP_ABORT && !DeviceBlockedError())
			HardBoot();
		return DIPIR_RETURN_TRYAGAIN;
	}
	TIMESTAMP("Prob",0);
	ProbeChannelDrivers();

	TIMESTAMP("Init",0);
	if (InitDipirEnv() < 0)
		HardBoot();

	RSAInit(TRUE);

	for (dev = HWResources;  dev != NULL;  dev = dev->dev_Next)
		dev->dev_Flags &= ~HWR_DIPIRED;

	TIMESTAMP("Devs",0);
CheckDevices:
	for (dev = HWResources;  dev != NULL;  dev = dev->dev_Next)
	{
		if ((dev->dev_Flags & HWR_MEDIA_ACCESS) == 0)
			continue;
		if (dev->dev_Flags & HWR_DIPIRED)
			continue;
		TIMESTAMP("chek",dev->dev.hwr_Name);
		PRINTF(("============ %s ============\n", dev->dev.hwr_Name));
		jmp = setjmp(dtmp->dt_JmpBuf);
		if (jmp)
		{
			EPRINTF(("Got longjmp; reblock dev & exit dipir\n"));
			/* If we got an abort, make sure it is due to
			 * a device being blocked. */
			if (jmp == JMP_ABORT && !DeviceBlockedError())
			{
				EPRINTF(("Unexpected CDE error\n"));
				HardBoot();
			}
			if ((*dev->dev_ChannelDriver->cd_DeviceControl)
				(dev, CHAN_BLOCK) < 0)
			{
				EPRINTF(("Cannot reblock device!\n"));
				HardBoot();
			}
			ret = DIPIR_RETURN_TRYAGAIN;
			goto Exit;
		}
		if ((*dev->dev_ChannelDriver->cd_DeviceControl)
			(dev, CHAN_UNBLOCK) < 0)
		{
			/*
			 * Cannot unblock the device.
			 * Skip it; maybe we'll be able to deal with it later.
			 */
			PRINTF(("cannot unblock\n"));
			continue;
		}
		ret = ValidateDevice(dev, DIPIR_VALIDATE, 0);
		TIMESTAMP("done",dev->dev.hwr_Name);
		PRINTF(("DIPIR: %s ret %x\n", dev->dev.hwr_Name, ret));
		if (dev->dev_Flags & HWR_DELETE)
		{
			EPRINTF(("DIPIR: Deleting %s\n", dev->dev.hwr_Name));
			DeleteHWResource(dev);
		}
		if (ret < 0 &&
		   (theBootGlobals->bg_DipirControl & DC_IGNORE_FAIL) == 0)
		{
			ret = DIPIR_RETURN_TRYAGAIN;
			goto Exit;
		}
		dev->dev_Flags |= HWR_DIPIRED;
		dev->dev_Flags &= ~HWR_MEDIA_ACCESS;
		/*
		 * Check the entire list again, in case it got
		 * changed while we were in a device dipir.
		 */
		goto CheckDevices;
	}
	PRINTF(("==============================\n"));

	ret = DIPIR_RETURN_OK;
Exit:
	TIMESTAMP("Exit",ret);
	PRINTF(("DIPIR: ret %x\n", ret));
	RestoreTimer();
#ifdef PREFILLSTACK
	DisplayStackUsage();
#endif
	/*
	 * Restore exception table.
	 * But not if we've called CleanupForNewOS(), since that 
	 * reinitializes the exception table itself.
	 */
#ifdef DIPIR_INTERRUPTS
	RestoreInterrupts();
#endif /* DIPIR_INTERRUPTS */
	RestoreExceptions();

	if (ret == DIPIR_RETURN_TRYAGAIN)
		goto Deinit;

	/*
	 * Decide if we need to reboot.
	 */

	if (BootRomAppMedia())
	{
		/* Found RomApp media somewhere.  Do a RomApp boot. */
		uint32 reg =
			ReadPowerBusRegister(M2_DEVICE_ID_CDE, CDE_VISA_DIS);
		PRINTF(("Found RomApp media: reg %x\n", reg));
		if ((reg & CDE_ROMAPP_BOOT) &&
		    theBootGlobals->bg_KernelAddr != 0 &&
		    ((dtmp->dt_Flags & DT_FOUND_ROMAPP_MEDIA) == 0 || 
		     (theBootGlobals->bg_DipirFlags & DF_NO_REBOOT)))
		{
			/* We're already running a RomApp. Just return to it. */
			goto Deinit;
		}
		if (!doubleChecked) goto DoubleCheck;
		TIMESTAMP("RomA",0);
		DumpTimeStamp();
		BootRomApp();
		/* BootRomApp never returns */
	}

	/* Data disc app: don't reboot, even if a device wants to. */
	if (AppExpectingDataDisc() &&
	    theBootGlobals->bg_KernelAddr != 0)
		goto Deinit;

	/* Boot the highest priority device that wants to boot (if any). */
	if (BootAppPrio() != 0)
	{
		PRINTF(("Booting new OS\n"));
		if (!doubleChecked) goto DoubleCheck;
		TIMESTAMP("Boot",0);
		DumpTimeStamp();
		BootOS();
	}

	/* Nobody wanted to boot. */
	if (theBootGlobals->bg_KernelAddr == 0)
	{
		/*
		 * There's no OS currently running.
		 * Boot a ROMApp OS to run the "NoCD" app.
		 */
		PRINTF(("NoCD: boot RomApp\n"));
		if (!doubleChecked) goto DoubleCheck;
		TIMESTAMP("RomN",0);
		DumpTimeStamp();
		BootRomApp();
		/* BootRomApp never returns */
	}

Deinit:
	/* Return to currently running OS. */
	DeinitDipirEnv();
	TIMESTAMP("retn",ret);
	DumpTimeStamp();
	PRINTF(("Returning to currently running OS\n"));
	return ret;

DoubleCheck:
	/*
	 * Check any devices that haven't been dipired during this
	 * dipir event.  We must do this before booting to determine
	 * whether some other device has the OS we should use.
	 */
	PRINTF(("============ Double Check ============\n"));
	for (dev = HWResources;  dev != NULL;  dev = dev->dev_Next)
	{
		if ((dev->dev_Flags & (HWR_DIPIRED | HWR_NODIPIR)) == 0)
			dev->dev_Flags |= HWR_MEDIA_ACCESS;
	}
	doubleChecked = TRUE;
	TIMESTAMP("dubl",0);
	goto CheckDevices;
}
