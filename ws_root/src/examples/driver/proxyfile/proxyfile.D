// @(#) proxyfile.D 95/12/11 1.3
// Device Description File for a sample Proxy device
//

#include "kernel/ddffile.h"

version 1.0	// Note:  this is a per-file version.

driver proxyfile  // This gives the name of the device as seen by the client
uses proxy

	needs
		nothing = 0
	end needs

	provides
		cmds: CMD_STATUS, CMD_BLOCKREAD, CMD_BLOCKWRITE,
			CMD_GETMAPINFO, CMD_MAPRANGE, CMD_UNMAPRANGE,
			CMD_PREFER_FSTYPE
		SERVERPATH: "System.M2/Programs/proxyfile"
		SERVERARG:  "/remote/mount.this.image"
		FS: 10
	end provides
end driver

