// @(#) mp.D 96/04/25 1.2
// Device Description File for the Multi-Processor device.
//

#include "kernel/ddffile.h"

version 1.0	// Note:  this is a per-file version.

driver mp
	needs
		nothing = 0
	end needs

	provides
		cmds: MP_CMD_DISPATCH
	end provides
end driver
