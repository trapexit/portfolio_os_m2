#ifdef MACINTOSH
#include <kernel:types.h>
#include <kernel:mem.h>
#include <kernel:spinlock.h>
#include <kernel:cache.h>
#include <device:mp.h>
#else
#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/spinlock.h>
#include <kernel/cache.h>
#include <device/mp.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mercury.h"

/* Assembly needs two areas it can muck with 
   These can be global because they are only used in the sort
   code which isn't multi-threaded */
Pod M_anchorpod;
/* 16 pointers, so up to 2048 objects in sort */
Pod *M_userpodptrs[16];

static short touchloadtable[32] = 
{
	 32, 32, 32, 32, 32, 32, 32, 32,
	 32, 32, 32, 40, 48, 56, 64, 72,
	 80, 88, 96,104,112,120,128,136,
	 144,152,160,168,168,168,168,168
};

CloseData  *M_Init(uint32 nverts, uint32 clistwords, GState *gs)
{
    CloseData *pclosedata;
    gfloat *pxformbuffer;
    CacheInfo ci;

    GetCacheInfo(&ci, sizeof(ci));
    /*	The beginning of the sorted list is anchorpod.  Make sure that
     *	its case, etc are different from the user's first pod.
     */
    M_anchorpod.pcase = 0;
    M_anchorpod.ptexture = 0;
    M_anchorpod.pgeometry = 0;
    M_anchorpod.pmatrix = 0;
    M_anchorpod.plights = 0;

    /* Longest temp vert is 13 floats so allocate accordingly */
    pxformbuffer = (gfloat *)AllocMemAligned(13 * sizeof(gfloat) * nverts,
					     MEMTYPE_NORMAL, ci.cinfo_DCacheLineSize);
    pclosedata = (CloseData *)AllocMemAligned(sizeof(CloseData), MEMTYPE_NORMAL, ci.cinfo_DCacheLineSize);
    
    if(!pxformbuffer || !pclosedata)
	{
	    printf("Out of memory in M_Init\n");
	    exit(1);
	}

    {
	long i;
	for(i = 0; i < 32; i++)
	    pclosedata->tltable[i] = touchloadtable[i];
    }
    pclosedata->fconstTINY = 0.000001;
    pclosedata->fconst0pt0 = 0.0;
    pclosedata->fconst0pt25 = 0.25;
    pclosedata->fconst0pt5 = 0.5;
    pclosedata->fconst0pt75 = 0.75;
    pclosedata->fconst1pt0 = 1.0;
    pclosedata->fconst2pt0 = 2.0;
    pclosedata->fconst3pt0 = 3.0;
    pclosedata->fconst8pt0 = 8.0;
    pclosedata->fconst16pt0 = 16.0;
    pclosedata->fconst255pt0 = 255.0;
    pclosedata->fconst12million = 12.0 * 1024.0 * 1024.0;
    pclosedata->fconstasin0 = 1.0/(2*3.141592653589793);
    pclosedata->fconstasin1 = 1.0/(6.0*2*3.141592653589793);
    pclosedata->fconstasin2 = 3.0/(40.0*2*3.141592653589793);
    pclosedata->fconstasin3 = 15.0/(336.0*2*3.141592653589793);

    pclosedata->pxformbuffer = pxformbuffer;
    pclosedata->gstate = gs;
    pclosedata->watermark = clistwords<200 ? 200 : clistwords;

    /* Initialize MP data to be UP */
    pclosedata->mp = 0;
    pclosedata->nverts = nverts;
    return pclosedata;
}

void M_End(CloseData *pclosedata)
{
    uint32 xformbuffersize=pclosedata->nverts*13 * sizeof(gfloat);
    PodQueue *mp = pclosedata->mp;

    FreeMem(pclosedata->pxformbuffer,xformbuffersize );

    if (mp) {
	M_EndMP(pclosedata);
    }
	
    FreeMem(pclosedata,sizeof(CloseData));
}


