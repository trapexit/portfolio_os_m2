// @(#) bdavideo.D 96/03/28 1.2
// Device Description File for the BDA video "driver" module
//

#include "kernel/ddffile.h"

version 1.0	// Note:  this is a per-file version.

driver bdavideo
	needs
		HW = BDA_VDU
	end needs

	provides
		cmds: GFXCMD_PROJECTORMODULE
	end provides
end driver
