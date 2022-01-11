// @(#) mpegvideo.D 96/12/11 1.4
// Device Description File for the MPEG video device.
//

#include "kernel/ddffile.h"

version 1.0	// Note:  this is a per-file version.

driver mpegvideo
	needs
		HW = MPEG1
	end needs

	provides
		cmds: CMD_STATUS, MPEGVIDEOCMD_CONTROL, 
			MPEGVIDEOCMD_WRITE, MPEGVIDEOCMD_READ,
			MPEGVIDEOCMD_GETBUFFERINFO,
			MPEGVIDEOCMD_ALLOCBUFFERS,
			MPEGVIDEOCMD_SETSTRIPBUFFER,
			MPEGVIDEOCMD_SETREFERENCEBUFFER
	end provides
end driver

