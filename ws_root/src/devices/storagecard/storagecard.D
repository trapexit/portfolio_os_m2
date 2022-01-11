// @(#) storagecard.D 96/03/13 1.10

#include "kernel/ddffile.h"

version 1.0	// Note:  this is a per-file version.

driver storagecard
	needs
		HW = Microcard00
	end needs

	provides
		cmds: CMD_STATUS, CMD_BLOCKWRITE, CMD_BLOCKREAD,
			CMD_PREFER_FSTYPE
		FS: 6   // defer automount until after the Shell is up
	end provides
end driver
