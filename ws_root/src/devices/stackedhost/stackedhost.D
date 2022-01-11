// @(#) stackedhost.D 96/09/10 1.3
//
// Device Description File for hostfs, hostcd and hostconsole devices.
//

#include "kernel/ddffile.h"

version 1.0	// Note:  this is a per-file version.

driver hostfs
module stackedhost
	needs
		protocol = host
	end needs
	provides
		cmds : CMD_STATUS, HOSTFS_CMD_SETBLOCKSIZE,
			HOSTFS_CMD_OPENENTRY, HOSTFS_CMD_READENTRY,
			HOSTFS_CMD_CLOSEENTRY, HOSTFS_CMD_BLOCKREAD,
			HOSTFS_CMD_READDIR, HOSTFS_CMD_BLOCKWRITE,
			HOSTFS_CMD_ALLOCBLOCKS, HOSTFS_CMD_CREATEFILE,
			HOSTFS_CMD_SETEOF, HOSTFS_CMD_SETTYPE,
			HOSTFS_CMD_SETVERSION, HOSTFS_CMD_DELETEENTRY,
			HOSTFS_CMD_CREATEDIR, HOSTFS_CMD_STATUS,
			HOSTFS_CMD_FSSTAT, HOSTFS_CMD_MOUNTFS,
			HOSTFS_CMD_DISMOUNTFS, HOSTFS_CMD_RENAMEENTRY,
			HOSTFS_CMD_SETDATE
		FS : 1
	end provides
end driver

driver hostcd
module stackedhost
	needs
		protocol = host
	end needs
	provides
		cmds : CMD_STATUS, CMD_BLOCKREAD
		FS : 1
	end provides
end driver

driver hostconsole
module stackedhost
	needs
		protocol = host
	end needs
	provides
		cmds : CMD_STATUS, HOSTCONSOLE_CMD_GETCMDLINE
	end provides
end driver
