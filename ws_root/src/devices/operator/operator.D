// @(#) operator.D 96/08/06 1.4
//
// Device Description File for builtin devices
//

#include "kernel/ddffile.h"

version 1.0

driver chan
	needs
		HW = *
	end needs
	provides
		cmds : CMD_BLOCKREAD, CMD_STATUS, CMD_GETMAPINFO,
		       CMD_MAPRANGE, CMD_UNMAPRANGE
		FS : 1
		flags : DDFF_REOPEN_ME
	end provides
end driver


/*****************************************************************************/


driver pchan
	needs
		HW = *
	end needs
	provides
		cmds : CMD_BLOCKREAD, CMD_STATUS
		FS : 1
		flags : DDFF_REOPEN_ME
	end provides
end driver


/*****************************************************************************/


driver mchan
	needs
		HW = *
	end needs
	provides
		cmds: CMD_STATUS, CMD_BLOCKREAD
		FS: 3 // external, read-only, mount at boot
		flags: DDFF_REOPEN_ME
	end provides
end driver


/*****************************************************************************/


driver microslot
	needs
		HW = *
	end needs
	provides
		cmds : CMD_STATUS, CMD_STREAMREAD, CMD_STREAMWRITE,
			USLOTCMD_RESET, USLOTCMD_SETCLOCK, USLOTCMD_SEQ
		slot : microslot
	end provides
end driver


/*****************************************************************************/


driver pcmciaslot
	needs
		HW = *
	end needs
	provides
		cmds : CMD_STATUS, CMD_BLOCKREAD, CMD_BLOCKWRITE,
			CMD_GETMAPINFO, CMD_MAPRANGE, CMD_UNMAPRANGE,
			SLOTCMD_SETTIMING, CMD_GETICON
		slot : pcmcia
	end provides
end driver


/*****************************************************************************/


driver timer
	needs
		nothing = 0
	end needs
	provides
		cmds : TIMERCMD_GETTIME_VBL, TIMERCMD_SETTIME_VBL,
			TIMERCMD_DELAY_VBL, TIMERCMD_DELAYUNTIL_VBL,
			TIMERCMD_METRONOME_VBL,
			TIMERCMD_GETTIME_USEC, TIMERCMD_SETTIME_USEC,
			TIMERCMD_DELAY_USEC, TIMERCMD_DELAYUNTIL_USEC,
			TIMERCMD_METRONOME_USEC
		flags : DDFF_REOPEN_ME
	end provides
end driver
