// @(#) null.D 95/12/11 1.2
// Device Description File for the null driver.
//

#include "kernel/ddffile.h"

version 1.0	// Note:  this is a per-file version.

driver null
	needs
		nothing = 0
	end needs

	provides
		cmds: CMD_STATUS
	end provides
end driver

