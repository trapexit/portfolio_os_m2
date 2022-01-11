/*
 *	@(#) diplib.banner.c 96/07/02 1.1
 *	Copyright 1996, The 3DO Company
 *
 * Display banner screen.
 * This is a diplib function, callable from device-dipirs.
 */

#include "kernel/types.h"
#include "dipir.h"
#include "notsysrom.h"
#include "diplib.h"

extern const DipirRoutines *dipr;

/*****************************************************************************
 Read the banner screen, check its signature, and display it.
*/
	DIPIR_RETURN
ReadAndDisplayBanner(DDDFile *fd, void *buffer, uint32 bufSize)
{
	RomTag rt;

	if (FindRomTag(fd, RSANODE, RSA_M2_APPBANNER, 0, &rt) <= 0)
	{
		PRINTF(("No app banner screen on disc\n"));
		return DIPIR_RETURN_TROJAN;
	}
	if (rt.rt_Size > bufSize)
	{
		PRINTF(("banner too big\n"));
		return DIPIR_RETURN_TROJAN;
	}
	if (!CanDisplay())
		buffer = NULL;
	if (ReadSigned(fd, rt.rt_Offset + fd->fd_RomTagBlock, rt.rt_Size, 
		buffer, KEY_128) < 0)
	{
		PRINTF(("Cannot read banner screen\n"));
		return DIPIR_RETURN_TROJAN;
	}
	/* Display only if we can use the video hardware right now. */
	if (buffer != NULL)
	{
		if (DisplayBanner(buffer, 0, 0) < 0)
		{
			PRINTF(("DisplayBanner failed\n"));
			return DIPIR_RETURN_TROJAN;
		}
	}
	return DIPIR_RETURN_OK;
}
