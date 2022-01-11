/* @(#) hierarchy.c 96/10/30 1.7 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/list.h>
#include <graphics/font.h>
#include <ui/requester.h>
#include <stdio.h>
#include <string.h>
#include "utils.h"
#include "msgstrings.h"
#include "req.h"
#include "hierarchy.h"


/*****************************************************************************/


void PrepHierarchy(Hierarchy *hier)
{
    PrepList(&hier->h_Entries);
    hier->h_Root.he_Label  = NULL;
    hier->h_NumEntries     = 0;
}


/*****************************************************************************/


void UnprepHierarchy(Hierarchy *hier)
{
HierarchyEntry *he;

    while (he = (HierarchyEntry *)RemHead(&hier->h_Entries))
    {
        DeleteTextState(he->he_Label);
        he->he_Label = NULL;

        if (he != &hier->h_Root)
            FreeMem(he, sizeof(HierarchyEntry) + strlen(he->he.n_Name) + 1);
    }
    hier->h_NumEntries = 0;
}


/*****************************************************************************/


Err GetHierarchy(StorageReq *req, Hierarchy *hier, const char *path)
{
Err             result;
HierarchyEntry *he;
uint32          i;
uint32          start;
FontTextArray   fta;

    UnprepHierarchy(hier);

	hier->h_Root.he.n_Name = "/";
    hier->h_NumEntries     = 1;
    AddTail(&hier->h_Entries, (Node *)&hier->h_Root);

    result = 0;
    i      = 0;
    while (path[i])
    {
        /* skip over slash */
        i++;

        start = i;
        while (path[i] && (path[i] != '/'))
            i++;

        if (i == start)
            break;

        he = AllocMem(sizeof(HierarchyEntry) + i - start + 1, MEMTYPE_NORMAL);
        if (he)
        {
            he->he.n_Name = (char *)&he[1];
            stccpy(he->he.n_Name, &path[start], i - start + 1);
            AddTail(&hier->h_Entries, (Node *)he);
            hier->h_NumEntries++;

            fta.fta_StructSize          = sizeof(fta);
            fta.fta_Pen.pen_X           = 0;
            fta.fta_Pen.pen_Y           = req->sr_SampleChar.cd_Ascent;
            fta.fta_Pen.pen_FgColor     = TEXT_COLOR_HIERARCHY_NORMAL;
            fta.fta_Pen.pen_BgColor     = 0;
            fta.fta_Pen.pen_XScale      = 1.0;
            fta.fta_Pen.pen_YScale      = 1.0;
            fta.fta_Pen.pen_Flags       = 0;
            fta.fta_Pen.pen_reserved    = 0;
            fta.fta_Clip.min.x          = 0.0;
            fta.fta_Clip.min.y          = 0.0;
            fta.fta_Clip.max.x          = BG_WIDTH;
            fta.fta_Clip.max.y          = BG_HEIGHT;
            fta.fta_String              = he->he.n_Name;
            fta.fta_NumChars            = strlen(he->he.n_Name);

            result = CreateTextState(&he->he_Label, req->sr_Font, &fta, 1);
            if (result < 0)
                break;

            GetTextExtent(he->he_Label, &he->he_Extent);
        }
        else
        {
            result = REQ_ERR_NOMEM;
            break;
        }
    }

    return result;
}
