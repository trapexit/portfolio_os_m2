/*
 *	@(#) hostcd.h 96/08/22 1.3
 *	Copyright 1995, The 3DO Company
 *
 * Definitions for callers of functions in hostcdcmd.c.
 */

#include "host.h"

extern void	DismountHostCD(DipirHWResource *dev, void *refToken);
extern int32	MountHostCD(DipirHWResource *dev, uint32 unit, 
			void **refToken, uint32 *numBlocks, uint32 *blockSize);
extern void *	ReadHostCDAsync(DipirHWResource *dev, void *refToken, 
			uint32 block, uint32 len, void *buffer);
extern int32	WaitReadHostCD(DipirHWResource *dev, void *id);
