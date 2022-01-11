/*
 *	@(#) diplib.loados.c 96/07/03 1.2
 *	Copyright 1996, The 3DO Company
 *
 * Load an OS into memory and create the list of OS components.
 * This is a diplib function, callable from device-dipirs.
 */

#include "kernel/types.h"
#include "kernel/list.h"
#include "kernel/listmacros.h"
#include "dipir.h"
#include "notsysrom.h"
#include "diplib.h"

extern const DipirRoutines *dipr;

/*****************************************************************************
 Load all components of an OS into memory and link them into a list.
 Caller supplies two functions:
   NextFile() reads the next OS component into memory.
   Alloc() allocates memory.
*/
	BootInfo *
LoadOS(int32 (*NextFile)(void *fileCtx, void **pAddr),
	void *fileCtx,
	void *(*Alloc)(void *allocCtx, uint32 size),
	void *allocCtx)
{
	BootInfo *bootInfo;
	OSComponentNode *node;
	void *addr;
	int32 size;

	/* Allocate a list header. */
	bootInfo = (*Alloc)(allocCtx, sizeof(BootInfo));
	if (bootInfo == NULL)
		return NULL;
	memset(bootInfo, 0, sizeof(BootInfo));
	PrepList(&bootInfo->bi_ComponentList);

	/* Read each file and link it into the list. */
	while ((size = (*NextFile)(fileCtx, &addr)) > 0)
	{
		node = (*Alloc)(allocCtx, sizeof(OSComponentNode));
		memset(node, 0, sizeof(OSComponentNode));
		node->n.n_SubsysType = 0;
		node->n.n_Type = 0;
		node->n.n_Size = sizeof(OSComponentNode);
		node->cn_Addr = addr;
		node->cn_Size = size;
		ADDTAIL(&bootInfo->bi_ComponentList, node);
	}
	if (size < 0)
	{
		PRINTF(("Cannot read OS component, size %x\n", size));
		return NULL;
	}
	return bootInfo;
}
