/*
 *    @(#) MPM_LastFrame.c 96/08/26 1.2
 *  Copyright 1996, The 3DO Company
 */
#include "mercury.h"
#include "mpm.h"
#ifdef MACINTOSH
#include <kernel:io.h>
#else
#include <kernel/io.h>
#endif
#include <stdio.h>

/*
 *	Completes sending of the last frame
 *
 *  Inputs:
 *		mpc		MpmContext to complete
 *		newbitmap	bitmap of the next frame that we will be rendering
 *				if == 0 then don't swap buffers
 *  Returns:
 *		triangle count assembled from all triangles drawn by both CPUs for
 *		last frame
 */

uint32	MPM_LastFrame(MpmContext* mpmc,Item newbitmap)
{
    Err 		result;
    uint32		podpiperet,podpiperet1;
    MpmSlaveBlock	*slavedata=&mpmc->slavedata;
    
    result = WaitIO(mpmc->mpioreq);
    if (result < 0) {
	printf("MPWaitIO() failed: ");
	PrintfSysErr(result);
    }
    podpiperet1=slavedata->returned_count; /* It's safe to copy the return value
					      before starting up 2nd processor again*/

    GS_SendList(mpmc->gs[1]);
    if (newbitmap) {
	/* inform the TE driver to swap buffers when it completes rendering */
	GS_EndFrame(mpmc->gs[0]);

	GS_SetDestBuffer(mpmc->gs[0],newbitmap);

	{
	    /* If we are doing transparent objects we must place the address of the new FB
	       as the src address to blend with */

	    Bitmap * newDestBM = (Bitmap*)LookupItem(newbitmap);
	    mpmc->pclosedata[0]->srcaddr = (uint32)newDestBM->bm_Buffer;
	    mpmc->pclosedata[1+mpmc->whichframe]->srcaddr = (uint32)newDestBM->bm_Buffer;
	}
    }

    podpiperet = mpmc->podpiperet0+podpiperet1;

    return podpiperet;
}

