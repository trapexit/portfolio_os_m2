/*
 *    @(#) MPM_DrawSendSwapLL.c 96/09/11 1.4
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

static Err SlaveFunctionLowLatency(void *vdata)
{
    Err 	ret;
    MpmSlaveBlock *data = (MpmSlaveBlock*)vdata;
    uint32	pp;

    FlushDCache(0, data->pclosedata->gstate, sizeof(GState));

    /* Results must be cached away where they won't be used
       by CPU0 */

    pp = M_Draw(data->ppod,data->pclosedata);

    data->returned_count = (pp & 0xffff)+(pp >> 16);

    if (data->returned_count==0) {
	ret = -1;
    } else {
	ret = 0;
    }

    /* Flush cache happens in GS_SendLastList */
    GS_SendLastList(data->pclosedata->gstate);

    return ret;
}

/*
 * Low latency Utility that :
 *	dispatchs a new command for CPU1 for current frame
 *	draws CPU0's portion of current frame
 *	sends CPU0's results to TE for current frame
 *	sends CPU1's results to TE for current frame
 *	tells TE to swap buffers after this command
 *
 *	Inputs:
 *		mpc		MpmContext with all parameters for current frame set
 *		newbitmap	bitmap of the next frame that we will be rendering
 *				if == 0 then don't swap buffers
 *	Returns:
 *		triangle count from previous frame
 */

uint32 MPM_DrawSendSwapLL(MpmContext* mpmc, Item newbitmap)
{
    Err 		result;
    uint32		podpiperet,podpiperet1;
    MpmSlaveBlock	*slavedata=&mpmc->slavedata;
    Pod			*firstpod1=MPM_Getfirstpod(mpmc,1);
    Pod			*firstpod0=MPM_Getfirstpod(mpmc,0);
    uint32		whichframe=mpmc->whichframe;


    result = WaitIO(mpmc->mpioreq);
    if (result < 0) {
	printf("WaitIO() for MP IOreq failed: ");
	PrintfSysErr(result);
    }
    podpiperet1=slavedata->returned_count; /* It's safe to copy the return value
					      before starting up 2nd processor again*/
    MPM_SyncCloseData(mpmc);

    if (firstpod1) {

	slavedata->ppod = firstpod1;
	slavedata->pclosedata = mpmc->pclosedata[1+whichframe];

	GS_LowLatency(mpmc->gs[1],0,slavedata->latency);
	FlushDCacheAll(0);

	result = DispatchMPFunc(mpmc->mpioreq, SlaveFunctionLowLatency, (void*)slavedata,
				mpmc->stack+(STACK_SIZE/4)-2, STACK_SIZE, &mpmc->status);
	if (result < 0){
	    printf("DispatchMPFunc failed \n");
	    PrintfSysErr(result);
	    exit(-1);
	}
    }

    podpiperet = mpmc->podpiperet0+podpiperet1;

    if (firstpod0) {
	uint32 pp;

	pp = M_Draw(firstpod0,mpmc->pclosedata[0]);
	mpmc->podpiperet0 = (pp & 0xffff)+(pp >> 16);
    }

    GS_SendLastList(mpmc->gs[0]);

    if (firstpod1)GS_SendIO(mpmc->gs[1],1,0);

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

    return podpiperet;
}
