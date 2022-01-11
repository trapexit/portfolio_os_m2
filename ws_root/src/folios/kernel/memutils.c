/* @(#) memutils.c 96/08/27 1.5 */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/kernelnodes.h>
#include <kernel/item.h>
#include <kernel/folio.h>
#include <kernel/list.h>
#include <kernel/listmacros.h>
#include <kernel/mem.h>
#include <kernel/kernel.h>
#include <kernel/internalf.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>


/*****************************************************************************/


void *AllocateSizedNode(const Folio *f, uint8 ntype, int32 size)
{
Node       *ret;
NodeData   *nd;
int32       nsize;
uint8       flags;
ItemNode   *in;

    if ((ntype == 0) || (ntype > f->f_MaxNodeType) || (size < 0))
        return NULL;

    nd    = f->f_NodeDB;
    nsize = nd[ntype].size;

    if (size != 0)
    {
        /* Only allow alternate size if it's big enough and
         * the size isn't locked down.
         */
        if ((size < nsize) || (nsize && (nd[ntype].flags & NODE_SIZELOCKED)))
            return NULL;

        nsize = size;
    }

    /* nsizes of 0 mean variable length so we return -1 to signal that */
    if (nsize == 0)
        return (void *) -1;

    /* last sanity check */
    if (nsize < sizeof(List))
        return NULL;

    ret = SuperAllocMem(nsize, MEMTYPE_ANY | MEMTYPE_FILL);
    if (ret == NULL)
        return NULL;

    flags = nd[ntype].flags;

    ret->n_SubsysType = (uint8)(f->fn.n_Item);
    ret->n_Type       = ntype;
    ret->n_Size       = nsize;
    ret->n_Flags      = (flags & 0xf0);

    if ((flags & NODE_ITEMVALID) == 0)
        return (void *)ret;

    in = (ItemNode *) ret;
    in->n_Item = GetItem(in);
    if ((int32) (in->n_Item) >= 0)
        return (void *)ret;

    SuperFreeMem(ret, nsize);

    return NULL;
}


/*****************************************************************************/


void *AllocateNode(const Folio *f, uint8 ntype)
{
    return AllocateSizedNode(f, ntype, 0);
}


/*****************************************************************************/


void FreeNode(const Folio *f, void *n)
{
ItemNode *in;

    TOUCH(f);

    if (n)
    {
        in = (ItemNode *)n;
        if (in->n_Flags & NODE_ITEMVALID)
            FreeItem(in->n_Item);

        if (in->n_Flags & (NODE_NAMEVALID | NODE_ITEMVALID))
            FreeString(in->n_Name);

        SuperFreeMem(in, in->n_Size);
    }
}


/*****************************************************************************/


/* handles strings of up to 255 chars in length */
char *AllocateString(const char *str)
{
int32  len;
char  *new;

    len = strlen(str) + 2;
    if (len > 255)
        return NULL;

    new = (char *) SuperAllocMem(len, MEMTYPE_ANY);
    if (new)
    {
        *new++ = (char)len;
        strcpy(new, str);
    }

    return new;
}


/*****************************************************************************/


void FreeString(char *n)
{
    if (n)
    {
        n--;
        SuperFreeMem(n, (int32)*n);
    }
}


/*****************************************************************************/
/*
    Validates that name is legal as an item name.

    Arguments
        name
            Unvalidated pointer

    Results
        TRUE if name is non-NULL, points to readable memory, and contains
        only printable characters.
*/
bool IsLegalName(const char *name)
{
        /* trap bad pointer (including NULL) */
    if (!IsMemReadable (name,1))
        return FALSE;

        /* trap illegal characters */
    while (*name)
    {
        if (!isprint(*name++))
            return FALSE;
    }

    return TRUE;
}
