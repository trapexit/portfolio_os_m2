/*
 *    @(#) MPM_SyncCloseData.c 96/07/18 1.1
 *  Copyright 1996, The 3DO Company
 */
#include "mercury.h"
#include "mpm.h"

/*
 * Copies all data that is set by the calling app from the closedata for
 * CPU 0 to the closedata for CPU 1
 *
 * Inputs closedata parameters for CPU0 that are copied
 *
 */

void  MPM_SyncCloseData(MpmContext *mpmc)
{
    mpmc->pclosedata[1+mpmc->whichframe]->fcamx = mpmc->pclosedata[0]->fcamx;
    mpmc->pclosedata[1+mpmc->whichframe]->fcamy = mpmc->pclosedata[0]->fcamy;
    mpmc->pclosedata[1+mpmc->whichframe]->fcamz = mpmc->pclosedata[0]->fcamz;
    mpmc->pclosedata[1+mpmc->whichframe]->fwclose = mpmc->pclosedata[0]->fwclose;
    mpmc->pclosedata[1+mpmc->whichframe]->fwfar = mpmc->pclosedata[0]->fwfar;
    mpmc->pclosedata[1+mpmc->whichframe]->fogcolor =  mpmc->pclosedata[0]->fogcolor;
    mpmc->pclosedata[1+mpmc->whichframe]->srcaddr =  mpmc->pclosedata[0]->srcaddr;
    mpmc->pclosedata[1+mpmc->whichframe]->depth =  mpmc->pclosedata[0]->depth;
    mpmc->pclosedata[1+mpmc->whichframe]->fscreenwidth = mpmc->pclosedata[0]->fscreenwidth;
    mpmc->pclosedata[1+mpmc->whichframe]->fscreenheight = mpmc->pclosedata[0]->fscreenheight;
    mpmc->pclosedata[1+mpmc->whichframe]->fcamskewmatrix = mpmc->pclosedata[0]->fcamskewmatrix;;

}
