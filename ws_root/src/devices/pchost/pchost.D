// @(#) pchost.D 96/10/18 1.1
//
// Device Description File for host driver
//

#include "kernel/ddffile.h"

version 1.0	// Note:  this is a per-file version.

driver pchost
	needs
		nothing = 0
	end needs
	provides
		protocol : host
		cmds : CMD_STATUS, HOST_CMD_SEND
	end provides
end driver
