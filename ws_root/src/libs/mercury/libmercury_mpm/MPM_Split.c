/*
 *    @(#) MPM_Split.c 96/08/26 1.2
 *  Copyright 1996, The 3DO Company
 */
#include "mercury.h"
#include "mpm.h"


/*
 * Splits the list for this frame according to the podcount0
 * value.
 *
 *  Input:
 *	firstpod[0]	CPU0's first pod must be set by a previous sort
 *	podcount0	Desired number of Pod's on CPU0 determines
 *			How many Pods end up on CPU1
 */ 

void MPM_Split(MpmContext* mpmc)
{
    Pod 	*ppod = MPM_Getfirstpod(mpmc,0); /* Start with CPU0's first */
    Pod		*lastppod = 0;

    uint32	i;
    uint32	podcount0=MPM_Getpodcount0(mpmc);

    /* Count up until the first pod that will be used by CPU1*/
    for (i=0; i<podcount0; i++){
	lastppod = ppod;
	ppod = ppod->pnext;
	/* break out if we reach end of list */
	if (!ppod) break;
    }
    MPM_Setfirstpod(mpmc,1,ppod);
    if (ppod) {
	ppod->flags &= ~(samecaseFLAG|sametextureFLAG);
    }
    if (podcount0 == 0) {
	MPM_Setfirstpod(mpmc,0,0);
    } else {
	/* make the last element in the first list an end of list pod */
	lastppod->pnext = 0;
    }

}

