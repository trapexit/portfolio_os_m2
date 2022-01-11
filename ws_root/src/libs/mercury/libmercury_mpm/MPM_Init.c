/*
 *    @(#) MPM_Init.c 96/08/06 1.7
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

/*
 * Initializes context
 *
 */
void MPM_Init(MpmContext *mpmc)
{
    CacheInfo ci;

    GetCacheInfo(&ci, sizeof(ci));

    mpmc->stack = (uint32*) AllocMemAligned(ALLOC_ROUND(STACK_SIZE, ci.cinfo_DCacheLineSize),
					    MEMTYPE_FILL,
					    ci.cinfo_DCacheLineSize);
    mpmc->mpioreq = 0;
    mpmc->whichframe = 0;
    mpmc->podpiperet0 = 0;
    mpmc->podcount = 0;
    
    mpmc->pm[0].needsort = 1;
    mpmc->pm[0].podcount0 = 0;
    mpmc->pm[0].podbase = 0;
    mpmc->pm[0].matrixbase = 0;
    mpmc->pm[0].firstpod[0] = 0;
    mpmc->pm[0].firstpod[1] = 0;

    mpmc->pm[1].needsort = 1;
    mpmc->pm[1].podcount0 = 0;
    mpmc->pm[1].podbase = 0;
    mpmc->pm[1].matrixbase = 0;
    mpmc->pm[1].firstpod[0] = 0;
    mpmc->pm[1].firstpod[1] = 0;

    mpmc->pclosedata[0] = 0;
    mpmc->pclosedata[1] = 0;
    mpmc->pclosedata[2] = 0;

    mpmc->gs[0] = 0;
    mpmc->gs[1] = 0;

    mpmc->slavedata.ppod = 0;
    mpmc->slavedata.pclosedata = 0;
    mpmc->slavedata.returned_count = 0;
}

