// @(#) ram.D 95/12/11 1.3
// Device Description File for the Proxy device


#include "kernel/ddffile.h"

version 1.0	// Note:  this is a per-file version.

driver proxy

	needs
		nothing = 0
	end needs

	provides
		cmds: CMD_STATUS
	end provides
end driver

