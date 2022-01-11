/*  @(#) header3do.h 96/09/09 1.9 */

#ifndef __HEADER3DO_H
#define __HEADER3DO_H

#ifndef __KERNEL_TYPES_H
#if !defined(__3DO_DEBUGGER__) && !defined(LINKER) && !defined(DUMPER)
#ifndef LOAD_FOR_MAC
#include <kernel/types.h>
#else
#include "types.h"
#endif
#endif
#endif /* !__3DO_DEBUGGER__ && !LINKER && !DUMPER */

#ifndef __KERNEL_NODES_H
#if defined(__3DO_DEBUGGER__) || defined(LINKER) || defined(DUMPER)
	#ifdef macintosh
	#include "operatypes.h"
	#else
	#include "nodes.h"
	#endif
#elif !defined(LOAD_FOR_MAC)
#include <kernel/nodes.h>
#else
#include <:kernel:nodes.h>
#endif
#endif

/* If the AIF_3DOHEADER is defined then the system will take all */
/* needed task parameters from the following structure */
/* above stack WS/aif encoded stack is ignored */

typedef struct _3DOBinHeader
{
	ItemNode _3DO_Item;
	uint8	_3DO_Flags;
	uint8	_3DO_OS_Version;	/* compiled for this OS release */
	uint8	_3DO_OS_Revision;
	uint8	_3DO_Reserved;
	uint32	_3DO_Stack;		/* stack requirements */
	uint32	_3DO_FreeSpace;		/* preallocate bytes for FreeList */
	uint32	_3DO_Signature;		/* if privileged, offset to beginning of sig */
	uint32	_3DO_SignatureLen;	/* length of signature */
	uint32	_3DO_MaxUSecs;		/* max usecs before task switch */
	uint32	_3DO_Reserved0;		/* must be zero */
	char	_3DO_Name[32];		/* optional task name on startup */
	uint32  _3DO_Time;              /* seconds since 1/1/93 00:00:00 GMT */
	uint32	_3DO_Reserved1[7];	/* must be zero */
} _3DOBinHeader;

/* _3DO_Flags */
#define _3DO_NO_CHDIR         0x01        /* Don't change task's dir to program dir */
#define _3DO_PRIVILEGE        0x02        /* Module is privileged */
#define _3DO_PCMCIA_OK        0x04        /* PCMCIA access is legal for this task */
#define _3DO_SHOWINFO         0x08        /* Show version info when module gets created */
#define _3DO_MODULE_CALLBACKS 0x20        /* Callback into module for create/delete/open/close */
#define _3DO_SIGNED           0x40        /* Module has signature */


#endif /* __LOADER_HEADER3DO_H */
