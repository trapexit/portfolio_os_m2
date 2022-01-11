/* @(#) hierarchy.h 96/09/07 1.2 */

#ifndef __HIERARCHY_H
#define __HIERARCHY_H


/*****************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_NODES_H
#include <kernel/nodes.h>
#endif

#ifndef __KERNEL_LIST_H
#include <kernel/list.h>
#endif

#ifndef __GRAPHICS_FONT_H
#include <graphics/font.h>
#endif


/*****************************************************************************/


typedef struct
{
    Node       he;
    TextState *he_Label;
    TextExtent he_Extent;
} HierarchyEntry;

typedef struct
{
    List           h_Entries;      /* list of HierarchyEntry */
    uint32         h_NumEntries;   /* # elements in list     */
    HierarchyEntry h_Root;
} Hierarchy;


/*****************************************************************************/


void PrepHierarchy(Hierarchy *hier);
void UnprepHierarchy(Hierarchy *hier);
Err GetHierarchy(struct StorageReq *req, Hierarchy *hier, const char *path);


/*****************************************************************************/


#endif /* __HIERARCHY_H */
