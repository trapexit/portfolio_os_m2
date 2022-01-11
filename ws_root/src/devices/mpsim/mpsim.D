// @(#) mpsim.D 96/07/19 1.2
// Device Description File for the simulated Multi-Processor device.
//

#include "kernel/ddffile.h"

version 1.0	// Note:  this is a per-file version.

driver mpsim
	needs
		nothing = 0
	end needs

	provides
		simulation: TRUE
		cmds: MP_CMD_DISPATCH
	end provides
end driver
