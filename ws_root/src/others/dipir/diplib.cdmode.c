/*
 *	@(#) diplib.cdmode.c 96/07/02 1.1
 *	Copyright 1996, The 3DO Company
 *
 * Check CD-ROM modes.
 * This is a diplib function, callable from device-dipirs.
 */

#include "kernel/types.h"
#include "dipir.h"
#include "notsysrom.h"
#include "ddd.cd.h"
#include "diplib.h"

extern const DipirRoutines *dipr;

/*****************************************************************************
 Is the disc mode ok?
 Check for audio tracks, inconsistent block count.
*/
DIPIR_RETURN 
CheckDiscMode(DDDFile *fd, DiscInfo *cdinfo)
{
	TOCInfo ti;
    	uint32 lastTrack;
	uint32 totalFrames;
    	uint32 track;
	uint32 flags;
	int32 ret;

	ret = DeviceGetCDFlags(fd, &flags);
	if (ret < 0)
		return DIPIR_RETURN_TROJAN;
	if (flags & (CD_MULTISESSION | CD_WRITABLE | CD_NOT_3DO))
		return DIPIR_RETURN_TROJAN;

	if (cdinfo->di_DiscId != 0)
		return DIPIR_RETURN_TROJAN;

        lastTrack = cdinfo->di_LastTrackNumber;
	totalFrames = cdinfo->di_MSFEndAddr_Frm +
		cdinfo->di_MSFEndAddr_Sec * FRAMEperSEC +
		cdinfo->di_MSFEndAddr_Min * FRAMEperSEC * SECperMIN -
		2 * FRAMEperSEC;
	PRINTF(("lastTrack %x, blockCount %x, totalFrames %x\n", 
		lastTrack, fd->fd_VolumeLabel->dl_VolumeBlockCount, 
		totalFrames));
	if (lastTrack > 1)
		return DIPIR_RETURN_TROJAN;

	/* allow 1M of slop */
	if (totalFrames * fd->fd_BlockSize >
	    fd->fd_VolumeLabel->dl_VolumeBlockCount * fd->fd_BlockSize + 1024*1024)
	{
		/* We are unable to verify enough data, kick it out */
		PRINTF(("Too many extra frames on this disc!\n"));
		return DIPIR_RETURN_TROJAN;
	}

	/*
	 * This doesn't really need to be a loop, since we only 
	 * get this far if there is exactly one track on the disc.
	 */
    	for (track = 1;  track <= lastTrack;  track++)
	{
		if (DeviceGetCDTOC(fd, track, &ti) < 0)
			return DIPIR_RETURN_TROJAN;
		if (ti.toc_AddrCntrl & (ACB_AUDIO_PREEMPH|ACB_FOUR_CHANNEL))
			return DIPIR_RETURN_TROJAN;
		if ((ti.toc_AddrCntrl & ACB_DIGITAL_TRACK) == 0)
			return DIPIR_RETURN_TROJAN;
        }

	return DIPIR_RETURN_OK;
}

