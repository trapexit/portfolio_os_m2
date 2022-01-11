/*
 *	@(#) diplib.verify.c 96/07/02 1.1
 *	Copyright 1996, The 3DO Company
 *
 * Check validity of a signed file.
 * This is a diplib function, callable from device-dipirs.
 */

#include "kernel/types.h"
#include "dipir.h"
#include "notsysrom.h"
#include "diplib.h"

extern const DipirRoutines *dipr;

/*****************************************************************************
*/
DIPIR_RETURN
VerifySigned(DDDFile *fd, uint32 subsys, uint32 type)
{
	RomTag rt;

	if (FindRomTag(fd, subsys, type, 0, &rt) <= 0)
		return DIPIR_RETURN_TROJAN;
	if (ReadSigned(fd, rt.rt_Offset + fd->fd_RomTagBlock, rt.rt_Size, 
			NULL, KEY_128) < 0)
		return DIPIR_RETURN_TROJAN;
	return DIPIR_RETURN_OK;
}
