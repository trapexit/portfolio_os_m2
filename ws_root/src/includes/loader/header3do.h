#ifndef __LOADER_HEADER3DO_H
#define __LOADER_HEADER3DO_H


/******************************************************************************
**
**  @(#) header3do.h 96/05/17 1.17
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#ifndef LOAD_FOR_MAC
#include <kernel/types.h>
#else
#include "types.h"
#endif
#endif

#ifndef __KERNEL_NODES_H
#ifndef LOAD_FOR_MAC
#include <kernel/nodes.h>
#else
#include <:kernel:nodes.h>
#endif
#endif


typedef struct _3DOBinHeader
{
	ItemNode _3DO_Item;
	uint8	_3DO_Flags;
	uint8	_3DO_OS_Version;	/* compiled for this OS release */
	uint8	_3DO_OS_Revision;
	uint8	_3DO_Reserved0;		/* must be zero */
	uint32	_3DO_Stack;		/* stack requirements */
	uint32	_3DO_Reserved1[3];	/* must be zero */
	uint32	_3DO_MaxUSecs;		/* max usecs before task switch */
	uint32	_3DO_Reserved2;		/* must be zero */
	char	_3DO_Name[32];		/* optional task name on startup */
	uint32  _3DO_Time;              /* seconds since 1/1/93 00:00:00 GMT */
	uint32	_3DO_Reserved3[7];	/* must be zero */
} _3DOBinHeader;

/* _3DO_Flags */
#define _3DO_NO_CHDIR         0x01        /* Don't change task's dir to program dir */
#define _3DO_PRIVILEGE        0x02        /* Module is privileged */
#define _3DO_PCMCIA_OK        0x04        /* PCMCIA access is legal for this task */
#define _3DO_SHOWINFO         0x08        /* Show version info when module gets created */
#define _3DO_MODULE_CALLBACKS 0x20        /* Callback into module for create/delete/open/close */
#define _3DO_SIGNED           0x40        /* Module has signature */


#endif /* __LOADER_HEADER3DO_H */
