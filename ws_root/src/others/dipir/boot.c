/*
 *	@(#) boot.c 96/10/08 1.44
 *	Copyright 1995,1996, The 3DO Company
 *
 * Code called by device dipirs which want to boot a new OS.
 */
#include "kernel/types.h"
#include "loader/loader3do.h"
#include "dipir.h"
#include "insysrom.h"
#include "hardware/cde.h"
#include "hardware/bda.h"

extern DIPIR_RETURN ReCallDeviceDipir(DipirHWResource *dev, uint32 dddID, 
		uint32 dipirID, uint32 cmd, void *arg);
extern DipirHWResource *DeviceFromID(HardwareID hwID);
extern void CatchExceptions(void);

/*
 * Data structure which represents a bootable OS.
 */
typedef struct OSInfo
{
	ExtVolumeLabel	os_Label;	/* Volume label from OS media */
	uint32		os_Version;	/* Version of the OS */
	List *		os_Addr;	/* The OS component list */
	void *		os_EntryPoint;	/* The initial PC	*/
	uint32		os_Flags;	/* OS flags (see rom.h) */
	uint32		os_MemReserve;	/* Reserved memory for OS */
	uint32		os_DevicePerms;	/* Device permissions */
	uint32		os_DipirID;	/* DevDipir that read this OS */
	uint32		os_DDDID;	/* DDD that read dipir (os_DipirID) */
	DipirHWResource *os_DipirDev;	/* Device that dipir came from */
} OSInfo;

/*
 * Database of info about what we're going to boot.
 */
static struct BootData
{
	/* The best OS to boot, if we want to launch a normal app. */
	OSInfo		boot_OS;
	/* The best OS to boot, if we want to launch a RomApp. */
	OSInfo		boot_RomAppOS;
	/* Volume label of media that the app came from. */
	ExtVolumeLabel	boot_AppLabel;
	/* The "boot priority" of the app we expect to launch. */
	uint32		boot_AppPrio;
	void *		boot_Unused;
	/* Generic boot variables for future use. */
	void *		boot_Vars[8];
} BootData;


/*****************************************************************************
 Set up a default RomApp boot, if we don't find anything else to boot.
*/
	int32
InitRomAppOS(void)
{
	int32 pos;
	RomTag rt;

	pos = 0;
	pos = FindRomTag(dtmp->dt_SysRomFd, 
			RT_SUBSYS_ROM, ROM_KERNEL_ROM, pos, &rt);
	if (pos <= 0) 
	{
#ifdef BUILD_DEBUGGER
		/* There is no kernel in the debugger pseudo-ROM,
		 * but that's ok.  We shouldn't need one. */
		return 0;
#else
		return -1;
#endif
	}
	SetBootOS(dtmp->dt_SysRomFd->fd_VolumeLabel, 
		MakeInt16(rt.rt_Version, rt.rt_Revision),
		(void*)0,
		(void*)0, rt.rt_OSFlags, rt.rt_OSReservedMem, 0xFFFFFFFF,
		dtmp->dt_SysRomFd->fd_HWResource, 0, DIPIRID_SYSROMAPP);
	return 0;
}

/*****************************************************************************
 Initialize the boot database.
*/
	int32
InitBoot(void)
{
	BootData.boot_AppPrio = 0;
	BootData.boot_OS.os_Version = 0;
	BootData.boot_RomAppOS.os_Version = 0;
	dtmp->dt_BootData = &BootData;
	return InitRomAppOS();
}

/*****************************************************************************
 Return the boot priority of the current boot app candidate.
*/
	uint32
BootAppPrio(void)
{
	return BootData.boot_AppPrio;
}

/*****************************************************************************
 Select a new boot app candidate.
*/
	void
SetBootApp(ExtVolumeLabel *label, uint32 prio)
{
	BootData.boot_AppLabel = *label;
	BootData.boot_AppPrio = prio;
}

/*****************************************************************************
 Select the media for a RomApp boot candidate.
*/
	void
SetBootRomAppMedia(DipirHWResource *dev)
{
	if (dev == NULL)
	{
		theBootGlobals->bg_RomAppDevice = 0;
	} else
	{
		theBootGlobals->bg_RomAppDevice = dev->dev.hwr_InsertID;
		dtmp->dt_Flags |= DT_FOUND_ROMAPP_MEDIA;
	}
}

/*****************************************************************************
 Return the media for the current RomApp boot candidate.
*/
	DipirHWResource *
BootRomAppMedia(void)
{
	if (theBootGlobals->bg_RomAppDevice == 0)
		return NULL;
	return DeviceFromID(theBootGlobals->bg_RomAppDevice);
}

/*****************************************************************************
 Return the version number of the current boot OS candidate.
*/
	uint32
BootOSVersion(uint32 osflags)
{
	OSInfo *os;

	os = (osflags & OS_ROMAPP) ? 
		&BootData.boot_RomAppOS : &BootData.boot_OS;
	return os->os_Version;
}
/*****************************************************************************
 Return the load address of the current boot OS candidate.
*/
	struct List *
BootOSAddr(uint32 osflags)
{
	OSInfo *os;

	os = (osflags & OS_ROMAPP) ? 
		&BootData.boot_RomAppOS : &BootData.boot_OS;
	return os->os_Addr;
}

/*****************************************************************************
 Return the OS flags of the current boot OS candidate.
*/
	uint32
BootOSFlags(uint32 osflags)
{
	OSInfo *os;

	os = (osflags & OS_ROMAPP) ?
		&BootData.boot_RomAppOS : &BootData.boot_OS;
	return os->os_Flags;
}

/*****************************************************************************
*/
	void
WillBoot(void)
{
	if (theBootGlobals->bg_DipirFlags & DF_VIDEO_INUSE)
	{
		(*theBootGlobals->bg_CleanupForNewOS)();
		CatchExceptions();
	}
}

/*****************************************************************************
 Select a new boot OS candidate.
*/
	static void
SetBootOSPtr(OSInfo *os,
	ExtVolumeLabel *label, uint32 version, void *addr, void *entry,
	uint32 osflags, uint32 kreserve, uint32 devicePerms, 
	DipirHWResource *dipirDev, uint32 dddID, uint32 dipirID)
{
	os->os_Label = *label;
	os->os_Version = version;
	os->os_Addr = addr;
	os->os_EntryPoint = entry;
	os->os_Flags = osflags;
	os->os_MemReserve = kreserve;
	os->os_DevicePerms = devicePerms;
	os->os_DDDID = dddID;
	os->os_DipirID = dipirID;
	os->os_DipirDev = dipirDev;
	if (addr != 0)
	{
		/* Set all other os->os_Addr = 0. */
		OSInfo *other = (os == &BootData.boot_OS) ? 
			&BootData.boot_RomAppOS : &BootData.boot_OS;
		if (other->os_DDDID != os->os_DDDID ||
		    other->os_DipirID != os->os_DipirID ||
		    other->os_DipirDev != os->os_DipirDev)
			other->os_Addr = 0;
	}
}

/*****************************************************************************
 Select a new boot OS candidate.
*/
	void
SetBootOS(ExtVolumeLabel *label, uint32 version, void *addr, void *entry,
	uint32 osflags, uint32 kreserve, uint32 devicePerms, 
	DipirHWResource *dipirDev, uint32 dddID, uint32 dipirID)
{
	PRINTF(("SetBootOS(%x,%x,%x,%x,%x,%x,%x,%x,%x,%x)\n",
		label, version, addr, entry, osflags, kreserve, devicePerms, 
		dipirDev, dddID, dipirID));
	if (osflags & OS_ROMAPP)
		SetBootOSPtr(&BootData.boot_RomAppOS, 
			label, version, addr, entry, osflags, kreserve,
			devicePerms, dipirDev, dddID, dipirID);
	if (osflags & OS_NORMAL)
		SetBootOSPtr(&BootData.boot_OS, 
			label, version, addr, entry, osflags, kreserve,
			devicePerms, dipirDev, dddID, dipirID);
}

/*****************************************************************************
 Set a generic boot variable.
*/
	int32
SetBootVar(uint32 var, void *value)
{
	if (var >= sizeof(BootData.boot_Vars)/sizeof(void*))
		return -1;
	BootData.boot_Vars[var] = value;
	return 0;
}

/*****************************************************************************
 Get value of a generic boot variable.
*/
	void *
GetBootVar(uint32 var)
{
	return BootData.boot_Vars[var];
}

/*****************************************************************************
 Boot a new OS (normal or RomApp).
*/
	static void
DoBoot(OSInfo *os)
{
	bootGlobals *bg = theBootGlobals;

	bg->bg_OSVolumeLabel = &os->os_Label;
	EPRINTF(("Booting "));
	if (os == &BootData.boot_OS)
	{
		/* Normal boot */
		bg->bg_AppVolumeLabel = &BootData.boot_AppLabel;
		EPRINTF(("app from %s", 
		((ExtVolumeLabel*)bg->bg_AppVolumeLabel)->dl_VolumeIdentifier));
	} else
	{
		/* RomApp boot */
		bg->bg_AppVolumeLabel = &os->os_Label;
		EPRINTF(("no app"));
	}
	EPRINTF((", OS from %s\n", 
		((ExtVolumeLabel*)bg->bg_OSVolumeLabel)->dl_VolumeIdentifier));
	bg->bg_DevicePerms = os->os_DevicePerms;
	bg->bg_KernelVersion = os->os_Version;
	bg->bg_KernelAddr = os->os_Addr;

	Reboot(os->os_EntryPoint,  dtmp->dt_QueryROMSysInfo, 0, 0);
	/*NOTREACHED*/
}

/*****************************************************************************
 Boot a new OS (normal app launch).
*/
	void
BootOS(void)
{
	PRINTF(("BootOS\n"));
	if (BootData.boot_AppPrio == 0)
	{
		/* No app to boot. */
		PRINTF(("No app to boot\n"));
		return;
	}
	if (BootData.boot_OS.os_Version == 0 ||
	    BootData.boot_OS.os_Addr == 0)
	{
		/* App but no OS? */
		EPRINTF(("ERROR: No OS to boot!\n"));
		return;
	}

	/* Clear ROMAPP_BOOT to remember that we're not running a RomApp. */
	if (BootData.boot_OS.os_Flags & OS_ROMAPP)
		SetPowerBusBits(M2_DEVICE_ID_CDE, 
				CDE_VISA_DIS, CDE_ROMAPP_BOOT);
	else
		ClearPowerBusBits(M2_DEVICE_ID_CDE, 
				CDE_VISA_DIS, CDE_ROMAPP_BOOT);
	DoBoot(&BootData.boot_OS);
}

/*****************************************************************************
 Boot a new OS (RomApp boot).
*/
	void
BootRomApp(void)
{
	DIPIR_RETURN ret;

	PRINTF(("BootRomApp\n"));
	if (BootData.boot_RomAppOS.os_Version == 0)
	{
		/* App but no OS? */
		EPRINTF(("ERROR: No RomApp OS to boot!\n"));
		HardBoot();
	}

	if (BootData.boot_RomAppOS.os_Addr == 0)
	{
		ret = ReCallDeviceDipir(BootData.boot_RomAppOS.os_DipirDev,
			BootData.boot_RomAppOS.os_DDDID, 
			BootData.boot_RomAppOS.os_DipirID, 
			DIPIR_LOADROMAPP, 0);
		if (ret < 0)
		{
			EPRINTF(("ERROR: Can't reload RomApp OS\n"));
			HardBoot();
		}
	}
	if (BootData.boot_RomAppOS.os_Addr == 0)
	{
		EPRINTF(("Recalled dipir didn't reload RomApp OS\n"));
		HardBoot();
	}

	PRINTF(("BootRomApp: RomAppDevice %x\n", 
		theBootGlobals->bg_RomAppDevice));
	/* Set ROMAPP_BOOT to remember that we're running a RomApp. */
	SetPowerBusBits(M2_DEVICE_ID_CDE, CDE_VISA_DIS, CDE_ROMAPP_BOOT);
	DoBoot(&BootData.boot_RomAppOS);
	EPRINTF(("DoBoot returned!\n"));
	HardBoot();
}
