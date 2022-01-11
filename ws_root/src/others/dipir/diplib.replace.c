/*
 *	@(#) diplib.replace.c 96/07/02 1.1
 *	Copyright 1996, The 3DO Company
 *
 * OS replacement logic.
 * This is a diplib function, callable from device-dipirs.
 */

#include "kernel/types.h"
#include "dipir.h"
#include "notsysrom.h"
#include "diplib.h"

extern const DipirRoutines *dipr;

/*****************************************************************************
*/
Boolean
ShouldReplaceOS(DDDFile *fd, uint32 myAppPrio)
{
	Boolean bootMyApp;
	Boolean replaceOS;
	uint32 osVersion;
	RomTag rt;

	if (FindRomTag(fd, RSANODE, RSA_M2_OS, 0, &rt) <= 0)
		return FALSE;
	osVersion = MakeInt16(rt.rt_Version, rt.rt_Revision);

	bootMyApp = FALSE;
	if (myAppPrio > BootAppPrio())
	{
		SetBootApp(fd->fd_VolumeLabel, myAppPrio);
		bootMyApp = TRUE;
	}

	if (bootMyApp)
	{
		/*
		 * Use this disc's OS, 
		 * unless there's a newer ANYTITLE OS elsewhere.
		 */
		replaceOS = 
			!(BootOSFlags(0) & OS_ANYTITLE) ||
			osVersion > BootOSVersion(0);
	} else
	{
		/*
		 * A disc never has an ANYTITLE OS.
		 * So since we're not booting app from this disc, 
		 * don't use the OS from this disc.
		 */
		replaceOS = FALSE;
	}
	return replaceOS;
}
