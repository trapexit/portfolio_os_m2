#ifndef __KERNEL_DDFNODE_H
#define __KERNEL_DDFNODE_H


/******************************************************************************
**
**  @(#) ddfnode.h 96/04/29 1.17
**
**  Definition of kernel Device Description File structures.
**
******************************************************************************/


#ifdef EXTERNAL_RELEASE
#error "This file may not be used in externally released source code or link lib"
#endif

#ifndef __KERNEL_DDF_H
#include <kernel/ddf.h>
#endif

/*
 * Representation of a DDF in memory.
 */
typedef struct DDFNode
{
	Node	ddf_n;
	List	ddf_Children;
	List	ddf_Parents;
	void *	ddf_Needs;
	void *	ddf_Provides;
	uint8	ddf_Version;
	uint8   ddf_Revision;
	uint16	ddf_Flags;
	char *	ddf_Module;
	char *	ddf_Driver;
} DDFNode;

/* ddf_Flags are in ddfflags.h */
#ifndef __KERNEL_DDFFLAGS_H
#include <kernel/ddfflags.h>
#endif

/* DDF macros */
#define set_ddf_flags(d,f)	((d)->ddf_Flags |= (f))
#define ddf_is_llvl(d)		((d)->ddf_Flags & DDFF_LOWLEVEL)
#define ddf_supports_fs(d)	((d)->ddf_Flags & DDFF_FS)
#define ddf_is_mounted(d)	((d)->ddf_Flags & DDFF_MOUNTED)
#define ddf_is_enabled(d)	((d)->ddf_Flags & DDFF_ENABLED)
#define ddf_is_disabled(d)	(!ddf_is_enabled(d))

/*
 * Link from one DDFNode to another, (e.g., a parent DDF to a child DDF).
 */
typedef struct DDFLink
{
	MinNode	ddfl_n;
	DDFNode *ddfl_Link;
} DDFLink;

#endif /* __KERNEL_DDFNODE_H */
