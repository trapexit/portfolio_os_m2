/*
 *    @(#) MPM_DuplicatePods.c 96/07/18 1.1
 *  Copyright 1996, The 3DO Company
 */
#include "mercury.h"
#include "mpm.h"
#include <string.h>

/*
 * Utility that duplicates Pods and matrices for a Context
 *
 * Inputs:
 *	mpmc	MpmContext that has valid data in frame 0
 *
 * Outputs:
 *	mpmc	Data in frame 1 is updated
 *
 */
void MPM_DuplicatePods(MpmContext *mpmc)
{
    Pod 	*podbuffer = mpmc->pm[0].podbase;
    Pod		*ppod;
    Matrix	*pmatrixbuffer = mpmc->pm[0].matrixbase;
    Matrix	*pmatrix;
    uint32	count = mpmc->podcount;
    uint32	i;
    
    memcpy((char*)(podbuffer+count),(char*)podbuffer,count*sizeof(Pod));
    pmatrix=pmatrixbuffer+count;
    ppod = podbuffer+count;
    for (i=0; i<count; i++) {
	ppod->pmatrix = pmatrix;
	memcpy((char*)pmatrix,(char*)&pmatrixbuffer[i],sizeof(Matrix));
	ppod++;
	pmatrix++;
    }
}

