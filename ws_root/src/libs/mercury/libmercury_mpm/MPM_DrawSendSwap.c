/*
 *    @(#) MPM_DrawSendSwap.c 96/09/11 1.6
 *  Copyright 1996, The 3DO Company
 */
#include "mercury.h"
#include "mpm.h"
#ifdef MACINTOSH
#include <device:mp.h>
#include <kernel:cache.h>
#include <kernel:mem.h>
#include <kernel:io.h>
#else
#include <device/mp.h>
#include <kernel/cache.h>
#include <kernel/mem.h>
#include <kernel/io.h>
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define STACK_SIZE 4096

static Err SlaveFunction(void *vdata)
{
    Err 	ret;
    MpmSlaveBlock *data = (MpmSlaveBlock*)vdata;
    uint32	pp;

    /* Results must be cached away where they won't be used
       by CPU0 */


    pp = M_Draw(data->ppod,data->pclosedata);

    data->returned_count = (pp & 0xffff)+(pp >> 16);

    if (data->returned_count==0) {
	ret = -1;
    } else {
	ret = 0;
    }

    /* Flush cache */
    FlushDCacheAll(GetDCacheFlushCount());

    return ret;
}

/*
 * Utility that :
 *	waits for completion of results previous ioreq to CPU1
 *	sends CPU1's results to TE for previous frame
 *	dispatchs a new command for CPU1 for current frame
 *	tells TE to swap buffers after this command
 *	draws CPU0's portion of current frame
 *	sends CPU0's results to TE for current frame
 *
 *	Inputs:
 *		mpc		MpmContext with all parameters for current frame set
 *		newbitmap	bitmap of the next frame that we will be rendering
 *				if == 0 then don't swap buffers
 *	Returns:
 *		triangle count from previous frame
 */

uint32 MPM_DrawSendSwap(MpmContext* mpmc, Item newbitmap)
{
    Err 		result;
    uint32		podpiperet,podpiperet1;
    MpmSlaveBlock	*slavedata=&mpmc->slavedata;
    Pod			*firstpod=MPM_Getfirstpod(mpmc,1);
    uint32		whichframe=mpmc->whichframe;


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
	GS_SetDestBuffer(mpmc->gs[0], newbitmap);

	{
	    /* If we are doing transparent objects we must place the address of the new FB
	       as the src address to blend with */

	    Bitmap * newDestBM = (Bitmap*)LookupItem(newbitmap);
	    mpmc->pclosedata[0]->srcaddr = (uint32)newDestBM->bm_Buffer;
	}
    }
    MPM_SyncCloseData(mpmc);

    if (firstpod) {
	/* this cache flush not necessary because we just sent to the TE */
	slavedata->ppod = firstpod;
	slavedata->pclosedata = mpmc->pclosedata[1+whichframe];

	FlushDCacheAll(GetDCacheFlushCount());

	result = DispatchMPFunc(mpmc->mpioreq, SlaveFunction, (void*)slavedata,
				mpmc->stack+(STACK_SIZE/4)-2, STACK_SIZE, &mpmc->status);
	if (result < 0){
	    printf("DispatchMPFunc failed \n");
	    PrintfSysErr(result);
	    exit(-1);
	}
    }

    podpiperet = mpmc->podpiperet0+podpiperet1;

    firstpod = MPM_Getfirstpod(mpmc,0);
    if (firstpod) {
	uint32 pp = M_Draw(firstpod,mpmc->pclosedata[0]);
	mpmc->podpiperet0 = (pp & 0xffff)+(pp >> 16);
    }

    /* Always send this list because it has the clear screen */
    GS_SendList(mpmc->gs[0]);

    return podpiperet;
}
