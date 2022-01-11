// @(#) ram.D 96/02/28 1.5
// Device Description File for the RAM device driver.
//

#include "kernel/ddffile.h"

version 1.0	// Note:  this is a per-file version.

driver ram
	needs
		HW = RAM
	or
		HW = RAM.r
	end needs

	provides
		cmds: CMD_STATUS, CMD_BLOCKREAD, CMD_BLOCKWRITE,
			CMD_GETMAPINFO, CMD_MAPRANGE, CMD_UNMAPRANGE,
			CMD_PREFER_FSTYPE
		FS: 5 // internal, read/write, defer mount
	end provides
end driver

