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

Err M_InitMP(CloseData *pclosedata, uint32 maxpods, uint32 clistwords, uint32 numverts)
{
    CloseData *slave;
    Err	e;
    CacheInfo ci;
    uint32 xformbuffersize = pclosedata->nverts*13 * sizeof(gfloat);
    GetCacheInfo(&ci, sizeof(ci));
    
    if (pclosedata->mp) {
#ifdef BUILD_STRINGS
	printf("<M_InitMP> MP already enabled\n");
#endif
	return -1;
    }

    pclosedata->mpioreq = CreateMPIOReq(0);
    if (pclosedata->mpioreq<0) {
#ifdef BUILD_STRINGS
	printf("<M_InitMP> Can't create MP item, perhaps this system is single processor: \n");
	PrintfSysErr(pclosedata->mpioreq);
#endif
	return -4;
    }

    pclosedata->mp = (PodQueue*)AllocMemAligned(sizeof(PodQueue), MEMTYPE_NORMAL, ci.cinfo_DCacheLineSize);

    pclosedata->maxpods = maxpods;
    
    if (!pclosedata->mp) goto mem0;

    e = CreateSpinLock((SpinLock**)&pclosedata->mp->inlock);
    if (e) {
    ret3:
#ifdef BUILD_STRINGS
	printf("<M_InitMP> can't create spinlock err = 0x%x\n",e);
	PrintfSysErr(e);
#endif
	goto mem1;
    }
    e = CreateSpinLock((SpinLock**)&pclosedata->mp->outlock);
    if (e) goto ret3;
    
    pclosedata->slavestack = (uint32*) AllocMemAligned(ALLOC_ROUND(M_STACK_SIZE, ci.cinfo_DCacheLineSize),
					       MEMTYPE_FILL,ci.cinfo_DCacheLineSize);
    if (!pclosedata->slavestack) goto mem2;

    slave = (CloseData *)AllocMemAligned(sizeof(CloseData), MEMTYPE_NORMAL, ci.cinfo_DCacheLineSize);

    if (!slave) goto mem3;

    pclosedata->slaveclosedata = slave;

    /* Copy all the data */
    memcpy(slave, pclosedata, sizeof(CloseData));
    
    slave->pxformbuffer = (gfloat *)AllocMemAligned(xformbuffersize, MEMTYPE_NORMAL, ci.cinfo_DCacheLineSize);

    if (!slave->pxformbuffer) goto mem4;

    pclosedata->clist[0] = (uint32*)AllocMemAligned(2*clistwords * sizeof(uint32),
						    MEMTYPE_NORMAL, ci.cinfo_DCacheLineSize);

    if (!pclosedata->clist[0]) goto mem5;

    slave->clist[0] = (uint32*)AllocMemAligned(2*clistwords * sizeof(uint32),
					       MEMTYPE_NORMAL, ci.cinfo_DCacheLineSize);

    if (!slave->clist[0]) goto mem6;

    pclosedata->clistsize = clistwords;
    pclosedata->clist[1] = pclosedata->clist[0]+clistwords;

    slave->clistsize = clistwords;
    slave->clist[1] = slave->clist[0]+clistwords;

    pclosedata->podlist = (PodList*)AllocMemAligned(maxpods * sizeof(PodList),
						    MEMTYPE_NORMAL, ci.cinfo_DCacheLineSize);
    if (!pclosedata->podlist) goto mem7;

    slave->podlist = (PodList*)AllocMemAligned(maxpods * sizeof(PodList),
					       MEMTYPE_NORMAL, ci.cinfo_DCacheLineSize);

    if (!slave->podlist) goto mem8;

    pclosedata->whichlist = 0;
    slave->whichlist = -1;

    pclosedata->totalpods = slave->totalpods = 0;

    pclosedata->vertsplit = (numverts==0) ? 500 : numverts;

    return 0;

    mem8:
	FreeMem(pclosedata->podlist, pclosedata->maxpods*sizeof(PodList));
    mem7:
	FreeMem(slave->clist[0], clistwords*2*sizeof(uint32));
    mem6:
	FreeMem(pclosedata->clist[0], clistwords*2*sizeof(uint32));
    mem5:
	FreeMem(slave->pxformbuffer, xformbuffersize);
    mem4:
	FreeMem(slave, sizeof(CloseData));
    mem3:
	FreeMem(pclosedata->slavestack, M_STACK_SIZE);
    mem2:	
	DeleteSpinLock((SpinLock*)pclosedata->mp->inlock);
	DeleteSpinLock((SpinLock*)pclosedata->mp->outlock);
    mem1:
	FreeMem(pclosedata->mp, sizeof(PodQueue));
    mem0:
	DeleteMPIOReq(pclosedata->mpioreq);

	/* Turn off MP on this CloseData */
	pclosedata->mp = 0;
#ifdef BUILD_STRINGS
	printf("<M_InitMP> not enough memory to initialize MP mode\n");
#endif
	return -2;
}

void M_EndMP(CloseData *pclosedata)
{
    uint32 xformbuffersize=pclosedata->nverts*13 * sizeof(gfloat);
    PodQueue *mp = pclosedata->mp;
    if (mp) {
	CloseData *slave = pclosedata->slaveclosedata;
	DeleteMPIOReq(pclosedata->mpioreq);
	FreeMem(slave->pxformbuffer, xformbuffersize);
	FreeMem(pclosedata->clist[0], pclosedata->clistsize*2*sizeof(uint32));
	FreeMem(slave->clist[0], slave->clistsize*2*sizeof(uint32));
	FreeMem(pclosedata->podlist, pclosedata->maxpods*sizeof(PodList));
	FreeMem(slave->podlist, slave->maxpods*sizeof(PodList));
	FreeMem(slave, sizeof(CloseData));
	FreeMem(pclosedata->slavestack, M_STACK_SIZE);
	DeleteSpinLock((SpinLock*)mp->inlock);
	DeleteSpinLock((SpinLock*)mp->outlock);
	FreeMem(mp, sizeof(PodQueue));
    }
    /* Turn off MP on this CloseData */
    pclosedata->mp = 0;
}
