/*
 *	@(#) chan.bridgit.c 96/01/12 1.14
 *	Copyright 1995, The 3DO Company
 *
 * Channel driver for Bridgit.
 */

#ifdef BRIDGIT

#include "kernel/types.h"
#include "dipir.h"
#include "insysrom.h"

extern const ChannelDriver BridgitChannelDriver;

	static void
InitBridgit(void)
{
	return;
}

	static void
ProbeBridgit(void)
{
}

	static int32
ReadBridgit(DipirHWResource *dev, uint32 offset, uint32 len, void *buf)
{
	return 0;
}

	static int32
MapBridgit(DipirHWResource *dev, uint32 offset, uint32 len, void **paddr)
{
	return -1;
}

	static int32
UnmapBridgit(DipirHWResource *dev, uint32 offset, uint32 len)
{
	return -1;
}

	static int32
DeviceControlBridgit(DipirHWResource *dev, uint32 cmd)
{
	switch (cmd)
	{
	case CHAN_BLOCK:
	case CHAN_UNBLOCK:
		break;
	}
	return -1;
}

	static int32
ChannelControlBridgit(uint32 cmd)
{
	switch (cmd)
	{
	case CHAN_DISALLOW_UNBLOCK:
		break;
	}
	return -1;
}

const ChannelDriver BridgitChannelDriver = 
{
	InitBridgit,
	ProbeBridgit,
	ReadBridgit,
	MapBridgit,
	UnmapBridgit,
	DeviceControlBridgit,
	ChannelControlBridgit
};

#else /* BRIDGIT */
extern int foo;
#endif /* BRIDGIT */
