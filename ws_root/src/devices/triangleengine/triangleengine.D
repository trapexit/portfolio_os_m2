// @(#) triangleengine.D 96/10/03 1.8
// Device Description File for the Triangle Engine.
//

#include "kernel/ddffile.h"

version 1.0	// Note:  this is a per-file version.

driver triangleengine
	needs
		HW = BDA_VDU
	end needs

	provides
		cmds:	TE_CMD_EXECUTELIST,
			TE_CMD_SETVIEW,
			TE_CMD_SETZBUFFER,
			TE_CMD_SETFRAMEBUFFER,
			TE_CMD_DISPLAYFRAMEBUFFER,
			TE_CMD_STEP,
			TE_CMD_SPEEDCONTROL,
			TE_CMD_SETVBLABORTCOUNT,
			TE_CMD_GETIDLETIME
	end provides
end driver

