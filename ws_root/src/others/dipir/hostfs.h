/*
 *	@(#) hostfs.h 96/08/22 1.2
 *	Copyright 1995, The 3DO Company
 *
 * Definitions for callers of functions in hostfscmd.c.
 */

#include "host.h"

extern void	InitHostFS(void);
extern int32	MountHostFS(DipirHWResource *dev, char *fsName, 
			RefToken *pRefToken);
extern void	DismountHostFS(DipirHWResource *dev, RefToken refToken);
extern int32	OpenHostFSFile(DipirHWResource *dev, RefToken dir, 
			char *filename, RefToken *pRefToken, 
			uint32 *pSize, uint32 *pBlockSize);
extern void	CloseHostFSFile(DipirHWResource *dev, RefToken fileToken);
extern void *	ReadHostFSFileAsync(DipirHWResource *dev, RefToken file, 
			uint32 block, uint32 len, void *buffer);
extern int32	WaitReadHostFSFile(DipirHWResource *dev, void *id);
