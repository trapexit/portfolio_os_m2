/* @(#) snippets.c 96/07/09 1.3 */
/* This code runs through a seres of tests of snippet handling. */

#include <kernel/types.h>
#include <graphics/blitter.h>
#include <stdio.h>
#include "testblit.h"

void doSnippetTests(void)
{
    Err err;
    BlitObject *bo[2] = {NULL, NULL};
    DBlendSnippet *dbOld;
    TxLoadSnippet *tl, *tlOld;

    printf("Run through a series of snippet tests\n");
    
    err = Blt_CreateBlitObject(&bo[0], NULL);
    printf("BlitObject[0] = 0x%lx\n", bo[0]);
    PrintfSysErr(err);
    err = Blt_CreateBlitObject(&bo[1], NULL);
    printf("BlitObject[1] = 0x%lx\n", bo[1]);
    PrintfSysErr(err);

    printf("Try to delete DBlend snippet of BlitObject[1] 0x%lx\n", bo[1]->bo_dbl);
    err = Blt_DeleteSnippet(bo[1]->bo_dbl);
    PrintfSysErr(err);
    
    printf("CopySnippet. Before, dblend[0] = 0x%lx, dblend[1] = 0x%lx\n",
           bo[0]->bo_dbl, bo[1]->bo_dbl);
    err = Blt_CopySnippet(bo[0], bo[1], (void *)&dbOld, BLIT_TAG_DBLEND);
    printf("After, dblend[0] = 0x%lx, dblend[1] = 0x%lx, replaced 0x%lx\n",
           bo[0]->bo_dbl, bo[1]->bo_dbl, dbOld);
    PrintfSysErr(err);
    
    printf("Now try to delete snippet 0x%lx\n", dbOld);
    err = Blt_DeleteSnippet(dbOld);
    PrintfSysErr(err);

    printf("Before swapping PIPLoad snippets, PIPLoad[0] = 0x%lx, PIPLoad[1] = 0x%lx\n",
           bo[0]->bo_pip, bo[1]->bo_pip);
    Blt_SwapSnippet(bo[0], bo[1], BLIT_TAG_PIP);
    printf("After swapping PIPLoad snippets, PIPLoad[0] = 0x%lx, PIPLoad[1] = 0x%lx\n",
           bo[0]->bo_pip, bo[1]->bo_pip);

    printf("Remove the TxLoad[0] 0x%lx\n", bo[0]->bo_txl);
    tl = (TxLoadSnippet *)Blt_RemoveSnippet(bo[0], BLIT_TAG_TXLOAD);
    printf("TxLoad[0] now = 0x%lx. Should delete it.\n", bo[0]->bo_txl);
    err = Blt_DeleteSnippet(tl);
    PrintfSysErr(err);

    tl = (TxLoadSnippet *)Blt_CreateSnippet(BLIT_TAG_TXLOAD);
    printf("Created a new TXLOAD snippet 0x%lx\n", tl);
    tlOld = (TxLoadSnippet *)Blt_SetSnippet(bo[1], (void *)tl);
    printf("Old TXLOAD snippet was 0x%lx\n", tlOld);
    err = Blt_DeleteSnippet(tlOld);
    PrintfSysErr(err);

    Blt_DeleteBlitObject(bo[0]);
    Blt_DeleteBlitObject(bo[1]);
    printf("Blt_DeleteBlitObject()\n");

        /* Lets test Blt_SetVertices() */
    {
        /* Create vertices of type (x, y, r, g, b, a, u, v) */
        gfloat vertices[] =
        {
            10.0, 10.0, 1.0, 0.0, 0.0, 1.0, 0, 0,
            80.0, 10.0, 0.0, 1.0, 0.0, 1.0, 70, 0,
            80.0, 70.0, 0.0, 0.0, 1.0, 1.0, 70, 70,
            10.0, 70.0, 1.0, 1.0, 0.0, 1.0, 0, 70,
        };
        VerticesSnippet *vts;
        TagArg ta[] =
        {
            BLIT_TAG_VERTICES, 0,
            TAG_END, 0,
        };
        
        err = Blt_CreateVertices(&vts, CLA_TRIANGLE(1, 1, 0, 1, 1, 4));
        printf("vertices = 0x%lx\n", vts);
        PrintfSysErr(err);
        Blt_SetVertices(vts, vertices);
        
        ta[0].ta_Arg = (void *)vts;
        err = Blt_CreateBlitObject(&bo[0], ta);
        printf("bo[0] = 0x%lx\n", bo[0]);
        PrintfSysErr(err);
        printf("Usage count on vertices = 0x%lx\n", vts->vtx_header.bsh_usageCount);
        
        Blt_DeleteBlitObject(bo[0]);
        printf("Usage count on vertices = 0x%lx\n", vts->vtx_header.bsh_usageCount);
        err = Blt_DeleteVertices(vts);
        PrintfSysErr(err);
    }
    
    printf("Snippet tests done\n\n");
    return;
}

