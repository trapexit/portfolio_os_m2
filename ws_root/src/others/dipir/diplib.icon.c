/*
 *	@(#) diplib.icon.c 96/07/02 1.1
 *	Copyright 1996, The 3DO Company
 *
 * Display device icon.
 * This is a diplib function, callable from device-dipirs.
 */

#include "kernel/types.h"
#include "dipir.h"
#include "notsysrom.h"
#include "diplib.h"

extern const DipirRoutines *dipr;

/*****************************************************************************
 Read an icon, check its signature, and display it.
*/
	DIPIR_RETURN
ReadAndDisplayIcon(DDDFile *fd)
{
	RomTag rt;
	void *buffer;

	if (FindRomTag(fd, RSANODE, RSA_M2_ICON, 0, &rt) <= 0)
	{
		PRINTF(("Cannot find icon\n"));
		return DIPIR_RETURN_TROJAN;
	}
	if (CanDisplay())
	{
		buffer = DipirAlloc(rt.rt_Size, 0);
		if (buffer == NULL)
		{
			PRINTF(("Cannot alloc %x for icon\n", rt.rt_Size));
			return DIPIR_RETURN_TROJAN;
		}
	} else
	{
		buffer = NULL;
	}
	if (ReadSigned(fd, rt.rt_Offset + fd->fd_RomTagBlock, rt.rt_Size, 
		buffer, KEY_128) < 0)
	{
		PRINTF(("Cannot read icon\n"));
		return DIPIR_RETURN_TROJAN;
	}
	/* Display only if we can use the video hardware right now. */
	if (buffer != NULL)
	{
		if (DisplayIcon(buffer, 0, 0) < 0)
		{
			PRINTF(("Cannot display icon\n"));
			return DIPIR_RETURN_TROJAN;
		}
		DipirFree(buffer);
	}
	/* Save icon location & size for later use. */
	fd->fd_HWResource->dev_IconOffset = 
		(rt.rt_Offset + fd->fd_RomTagBlock) * fd->fd_BlockSize;
	fd->fd_HWResource->dev_IconSize = rt.rt_Size;
	return DIPIR_RETURN_OK;
}
