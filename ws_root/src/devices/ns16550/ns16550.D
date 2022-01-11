// @(#) ns16550.D 96/01/11 1.6
//
// Device Description File for NS16550 driver
//

#include "kernel/ddffile.h"

version 1.0	// Note:  this is a per-file version.

driver ns16550
	needs
		HW = PC16550
	end needs
	provides
		protocol : serial
		cmds : CMD_STREAMREAD,
			CMD_STREAMWRITE,
			SER_CMD_SETCONFIG,
			SER_CMD_GETCONFIG,
			SER_CMD_WAITEVENT,
			SER_CMD_SETRTS,
			SER_CMD_SETDTR,
			SER_CMD_SETLOOPBACK,
			SER_CMD_BREAK,
			SER_CMD_STATUS
		flags: DDFF_REOPEN_ME
	end provides
end driver
