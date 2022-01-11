/*
 *    @(#) MPM_LinkSortSplit.c 96/07/18 1.1
 *  Copyright 1996, The 3DO Company
 */
#include "mercury.h"
#include "mpm.h"

/*
 * Utility that links all PODS for a frame into a list,
 * sorts it and then splits the list
 *
 */

void MPM_LinkSortSplit(MpmContext* mpmc)
{
    
    Pod		*liststart;
    Pod		*ppod;
    Pod		*firstpod;
    uint32	i;
    uint32	podcount;

    if (MPM_Getneedsort(mpmc)) {
    
	liststart = MPM_Getpodbase(mpmc);
	ppod = liststart;
	podcount = MPM_Getpodcount(mpmc);

	for(i = 0; i < podcount; i++) {
	    ppod->pnext = ppod+1;
	    ppod++;
	}

	firstpod = M_Sort(podcount,liststart,0);
	MPM_Setfirstpod(mpmc,0,firstpod);
	MPM_Split(mpmc);
	MPM_Setneedsort(mpmc,0);
    }
}
