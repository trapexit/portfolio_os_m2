// @(#) ramdisk.D 96/02/29 1.4
// Device Description File for ramdisk.
//

#include "kernel/ddffile.h"

version 1.0	// Note:  this is a per-file version.

driver ramdisk
	needs
		nothing = 0
	end needs

	provides
		cmds: CMD_STATUS, CMD_BLOCKREAD, CMD_BLOCKWRITE
		FS: 10
		flags: DDFF_REOPEN_ME
	end provides
end driver

