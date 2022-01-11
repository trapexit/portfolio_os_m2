// @(#) eventport.D 96/07/17 1.3
//
// Device Description File for eventport.
//

#include "kernel/ddffile.h"

version 1.0	// Note:  this is a per-file version.

driver eventport
	needs
		nothing = 0
	end needs
	provides
		cmds : CPORT_CMD_WRITEEVENT, CPORT_CMD_READEVENT
	end provides
end driver
