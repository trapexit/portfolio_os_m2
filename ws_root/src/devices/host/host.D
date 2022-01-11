// @(#) host.D 95/12/11 1.5
//
// Device Description File for host driver
//

#include "kernel/ddffile.h"

version 1.0	// Note:  this is a per-file version.

driver host
	needs
		HW = HOST
	end needs
	provides
		protocol : host
		cmds : CMD_STATUS, HOST_CMD_SEND, HOST_CMD_RECV
	end provides
end driver
