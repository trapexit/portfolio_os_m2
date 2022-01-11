// @(#) cdrom.D 96/01/07 1.5
// Device Description File for the Low-Cost CD-ROM.
//

#include "kernel/ddffile.h"

version 1.0	// Note:  this is a per-file version.

driver cdrom
	needs
		HW = LCCD
	end needs

	provides
		cmds: CMD_STATUS, CMD_BLOCKREAD,
			CDROMCMD_CLOSE_DRAWER, CDROMCMD_DIAG_INFO,
			CDROMCMD_DISCDATA, CDROMCMD_OPEN_DRAWER,
			CDROMCMD_READ, CDROMCMD_READ_SUBQ,
			CDROMCMD_RESIZE_BUFFERS, CDROMCMD_SCAN_READ,
			CDROMCMD_SETDEFAULTS, CDROMCMD_WOBBLE_INFO
		FS: 1
		romappmedia: 1
	end provides
end driver

