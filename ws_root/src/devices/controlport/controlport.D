// @(#) controlport.D 96/08/01 1.5
//
// Device Description File for controlport
//

#include "kernel/ddffile.h"

version 1.0	// Note:  this is a per-file version.

driver controlport
	needs
                HW = CONTROLPORT
	end needs
	provides
		cmds : CPORT_CMD_CONFIGURE, CPORT_CMD_READ, CPORT_CMD_WRITE,
                       CMD_STATUS
	end provides
end driver
