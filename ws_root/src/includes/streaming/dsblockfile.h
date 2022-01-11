/******************************************************************************
**
**  @(#) dsblockfile.h 96/03/01 1.7
**
**	Definitions for BlockFile.c, block oriented file access.
**	This version is a subset of the lib3DO blockfile.h.
**
******************************************************************************/
#ifndef __STREAMING_DSBLOCKFILE_H
#define __STREAMING_DSBLOCKFILE_H

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __FILE_FILEFUNCTIONS_H
#include <file/filefunctions.h>
#endif

#ifndef __FILE_FILESYSTEM_H
#include <file/filesystem.h>
#endif


/**************************************************
 * Data structure for use with BlockFile routines
 **************************************************/

typedef struct BlockFile {
	Item		fDevice;		/* file device Item */
	FileStatus	fStatus;		/* status record */
} BlockFile, *BlockFilePtr;


/**********************
 * Routine prototypes
 **********************/

#ifdef __cplusplus 
extern "C" {
#endif

Err		OpenBlockFile(char *name, BlockFilePtr bf);

void	CloseBlockFile(BlockFilePtr bf);

Item	CreateBlockFileIOReq(Item deviceItem, Item iodoneReplyPort);

uint32	GetBlockFileSize(BlockFilePtr bf);

uint32	GetBlockFileBlockSize(BlockFilePtr bf);

Err		AsynchReadBlockFile(BlockFilePtr bf, Item ioreqItem, void *buffer,
			uint32 count, uint32 offset);

Err		WaitReadDoneBlockFile(Item ioreqItem);

#ifdef __cplusplus
}
#endif

#endif	/* __STREAMING_DSBLOCKFILE_H */

