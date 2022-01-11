/*
 *	@(#) hostutil.c 96/08/22 1.12
 *	Copyright 1995, The 3DO Company
 *
 * Utility functions used by code which communicates with the debugger host.
 */

#include "kernel/types.h"
#include "hardware/PPCasm.h"
#include "hardware/debugger.h"
#include "dipir.h"
#include "notsysrom.h"
#include "host.h"


extern const DipirRoutines *dipr;

static volatile HostFSReq   * CmdChannel;
static volatile HostFSReply * ReplyChannel;

/*****************************************************************************
*/
	void
InitHost(DipirTemp *dt)
{
	DebuggerInfo *dbg;

	(*dt->dt_QueryROMSysInfo)(SYSINFO_TAG_DEBUGGERREGION, &dbg, sizeof(dbg));
	CmdChannel = dbg->dbg_CommOutPtr;
	ReplyChannel = dbg->dbg_CommInPtr;

	/* I'm not sure why the debugger can't do this initialization. */
	CmdChannel->hfs_Busy = 0;
	FlushDCache(CmdChannel, sizeof(HostFSReq));
	ReplyChannel->hfsr_Busy = 0;
	FlushDCache(ReplyChannel, sizeof(HostFSReply));
	_sync();
}

/*****************************************************************************
 Send a command packet to the host.
*/
	void
SendHostCmd(DipirHWResource *dev, HostFSReq *cmd)
{
	cmd->hfs_Busy = 0;

	TOUCH(dev);
	while (!DEBUGGER_ACK())
		;

	while (CmdChannel->hfs_Busy);
		InvalidateDCache(CmdChannel, sizeof(HostFSReq));

	if (cmd->hfs_Send.iob_Buffer != NULL)
		InvalidateDCache(cmd->hfs_Send.iob_Buffer, cmd->hfs_Send.iob_Len);
	else
		cmd->hfs_Send.iob_Len = 0;

	if (cmd->hfs_Recv.iob_Buffer != NULL)
		InvalidateDCache(cmd->hfs_Recv.iob_Buffer, cmd->hfs_Recv.iob_Len);
	else
		cmd->hfs_Recv.iob_Len = 0;

	*CmdChannel = *cmd;
	_dcbst(CmdChannel);
	_sync();
	CmdChannel->hfs_Busy = 1;
	FlushDCache(CmdChannel, sizeof(HostFSReq));
	_sync();
	DEBUGGER_CHANNEL_FULL();

	while (!DEBUGGER_ACK())
		;

	while (CmdChannel->hfs_Busy);
		InvalidateDCache(CmdChannel, sizeof(HostFSReq));
}

/*****************************************************************************
 Get a reply packet from the host.
*/
	void
GetHostReply(DipirHWResource *dev, HostFSReply *reply)
{
	TOUCH(dev);
	do {
		InvalidateDCache(ReplyChannel, sizeof(HostFSReply));
	} while (ReplyChannel->hfsr_Busy == 0);
	*reply = *ReplyChannel;
	ReplyChannel->hfsr_Busy = 0;
	FlushDCache(ReplyChannel, sizeof(HostFSReply));
	_sync();
	DEBUGGER_CHANNEL_EMPTY();
}
