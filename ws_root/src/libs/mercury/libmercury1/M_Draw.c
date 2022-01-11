#ifdef MACINTOSH
#include <kernel:io.h>
#include <device:te.h>
#include <graphics:clt:gstate.h>
#include <device:mp.h>
#include <kernel:cache.h>
#include <kernel:spinlock.h>
#else
#include <kernel/io.h>
#include <device/te.h>
#include <graphics/clt/gstate.h>
#include <device/mp.h>
#include <kernel/cache.h>
#include <kernel/spinlock.h>
#endif
#include "mercury.h"

#define MP 1
#define TE 1

#define DCACHESIZE	4096
#define DCACHELINESIZE	32

#define JUMP_WORDS	3
#define JUMP_BYTES	(4*JUMP_WORDS)

#define OUTFLAG_SENT	1
#define OUTFLAG_STARTED	2

extern M_DrawA(Pod *pfirstpod, CloseData *pc);

uint32 waitcount=10;

static void GetLock(SpinLock *lock)
{
#ifdef BUILD_STRINGS
    uint32 count=0;
#endif
    uint32 wait;

    /* Lock queue */
    while (!ObtainSpinLock(lock)) {
#ifdef BUILD_STRINGS
	count++;
	if (count==1000) {
	    printf("<GetLock> CPU %d Can't get lock\n", IsSlaveCPU());
	}
#endif
	for (wait = waitcount; wait>0; wait--);
    }
}

static Pod *LockGetPod(CloseData *pc, PodList **l)
{
    /* Take the next pod off the queue and add it to the pod list */
    PodQueue *q = pc->mp;
    PodList *list = pc->lastl;
    Pod *retpod;
    SpinLock *inlock=q->inlock;
    uint32 numverts;
    uint32 numpods=1;
    Pod *ppod;

    GetLock(inlock);
    
    /* Refresh cache line w/ in stuff */
    FlushDCache(0, q, DCACHELINESIZE);

    retpod = q->curpod;

    if (retpod) {
	list->podid = q->inid++;
	pc->lastl++;

	ppod = retpod;

	numverts = ppod->pgeometry->vertexcount;
	ppod = ppod->pnext;

	while ((numverts < pc->vertsplit) && ppod) {
	    numverts += ppod->pgeometry->vertexcount;
	    numpods++;
	    ppod = ppod->pnext;
	}

	q->curpod = ppod;
    }
	
    /* unlock queue */

    /* Refresh cache line w/ in stuff */
    FlushDCache(0, q, DCACHELINESIZE);
    ReleaseSpinLock(inlock);
    
    list->flags = 0;
    pc->numpods = numpods;
    pc->totalpods += numpods;
    
#ifdef BUILD_STRINGS
    if ((pc->lastl-pc->firstl)>pc->maxpods){
	printf("<LockGetPod> PodList overflowed\n");
	exit(-1);
    }
#endif /*BUILD_STRINGS*/    

    *l = list;
    return retpod;
}

static uint32 DequeueDraw(CloseData *pc)
{
    GState	*gs = (GState*)pc->gstate;
    PodQueue 	*q = pc->mp;
    Pod 	*ppod;
    PodList 	*list;
    uint32 	tricount=0;
    int32 	numbytes;
    int32 	bytecount;
    PodList 	*pl;
    uint32 	*ltf;
    SpinLock	*outlock = q->outlock;
    bool	started = 0;
    bool	lowlatency = GS_IsLowLatency(gs);

    ppod = LockGetPod(pc, &list);
    numbytes = 0;

    while(ppod) {

	if (ppod->pcase != pc->lastcase) {
	    ppod->flags &= ~samecaseFLAG;
	}
	if (!(ppod->flags & samecaseFLAG)){
	    pc->lastcase = ppod->pcase;
	}
	list->startlist = pc->pVIwrite;
	while(pc->numpods-- > 0) {
	    tricount += M_DrawPod(ppod,pc);
	    if (!pc->mp) return tricount;
	    ppod = ppod->pnext;
	} 
	list->endlist = pc->pVIwrite;
	pc->pVIwrite += JUMP_WORDS; /* Skip the jump that will go here */
	    
	numbytes += ((uint32)list->endlist) - ((uint32)list->startlist) + JUMP_BYTES ; /* Don't forget the jump */

	if (numbytes > DCACHESIZE) {
	    /* Send off as many pods as we can */

	    for (pl=pc->firstl; pl != pc->lastl; pl = ++pc->firstl) {
		bytecount=((uint32)pl->endlist)-((uint32)pl->startlist)+JUMP_BYTES;
		if ((numbytes-bytecount) >= (DCACHESIZE+DCACHELINESIZE)) {
		    /* Lock gs */
		    GetLock(outlock);

		    /* Refresh cache line w/ out info */
		    FlushDCache(0,&q->outlock, DCACHELINESIZE);

		    if (q->outid != (pl->podid-1)) {
			/* Release gs */
			ReleaseSpinLock(outlock);
			break;
		    }


#if TE
		    ltf = gs->gs_ListPtr;
		    CLT_JumpAbsolute(GS_Ptr(gs), pl->startlist);
#endif
		    q->outid++;

		    if (lowlatency) {
			if (q->outflag & OUTFLAG_SENT) {
			    /* See if te has started by now */
			    uint32 	teaddr = (uint32)GetTEReadPointer();
			    uint32	endlist = ((uint32)ltf)&0x3fffffff;

			    if (teaddr >= q->startaddr &&
				teaddr <= endlist) {
				q->outflag &= ~ OUTFLAG_SENT;
				q->outflag |= OUTFLAG_STARTED;
			    }
			}
			started = q->outflag & OUTFLAG_STARTED;
		    } 
		    /* Release gs */	
		    /* Refresh cache line w/ out info */
		    FlushDCache(0,&q->outlock, DCACHELINESIZE);

		    FlushDCache(0, ltf, ((char*)gs->gs_ListPtr) - ((char*)ltf));
		    FlushDCache(0, &gs->gs_ListPtr, DCACHELINESIZE);

		    ltf += JUMP_WORDS;  /* Now point to the return address */
		    CLT_JumpAbsolute(&pl->endlist, ltf);

#if TE
		    if (started) {
			/* For the TE's benefit flush the 12 bytes for the jump */
			FlushDCache(0, pl->endlist-JUMP_WORDS, JUMP_BYTES); 

			SetTEWritePointer(ltf); /* Execute the Pod */
		    }
#endif 		    
		    ReleaseSpinLock(outlock);

		    numbytes -= bytecount;
		} else {
		    break;
		}
	    }
	}
	ppod = LockGetPod(pc, &list);
    }
    pc->numbytes = numbytes;
    return tricount;
}

#define STACK_SIZE 4096

static Err SlaveFunction(void *vdata) 
{
    CloseData 	*pc = (CloseData*)vdata;
    uint32	pp;

    /* Results must be cached away where they won't be used
       by CPU0 */

    pp = DequeueDraw(pc);

    /* Flush cache */
    FlushDCacheAll(0); 

    return (Err)pp;
}

uint32
M_Draw(Pod *pfirstpod, CloseData *pc)
{
    GState	*gs = (GState*)pc->gstate;
    uint32	pret;
    uint32 	tricount;
    int32 	numbytes;
    int32 	bytecount;
    PodList 	*pl;
    uint32 	*ltf;
    Err		result;
    Err		slavestatus;
    CloseData	*slave=pc->slaveclosedata;
    PodQueue	*q=pc->mp;
    bool	lowlatency;
#ifdef BUILD_STRINGS
    PodList *savepl;
    PodList *saveslavepl;
    uint32  stuckcount=0;
#endif /*BUILD_STRINGS*/

    pc->aa = 0;
    if (q) {
	/* MP Case */
	if (pc->whichlist != slave->whichlist) {
	    /* We must have just done M_DrawEnd() so initialize pointers */
	    slave->whichlist = pc->whichlist;
	    slave->pVIwrite = slave->clist[slave->whichlist];
	    slave->pVIwritemax = slave->pVIwrite+slave->clistsize;
	    slave->totalpods = 0;
	    pc->pVIwrite = pc->clist[pc->whichlist];
	    pc->pVIwritemax = pc->pVIwrite + pc->clistsize;
	    pc->totalpods = 0;
	}

	/* Sync close data */

	slave->fcamx = pc->fcamx;
	slave->fcamy = pc->fcamy;
	slave->fcamz = pc->fcamz;
	slave->fwclose = pc->fwclose;
	slave->fwfar = pc->fwfar;
	slave->fogcolor =  pc->fogcolor;
	slave->srcaddr =  pc->srcaddr;
	slave->depth =  pc->depth;
	slave->fscreenwidth = pc->fscreenwidth;
	slave->fscreenheight = pc->fscreenheight;
	slave->fcamskewmatrix = pc->fcamskewmatrix;;
	slave->vertsplit = pc->vertsplit;
#ifdef STATISTICS
	slave->numpods_fast = 0;
	slave->numpods_slow = 0;
	slave->numtris_fast = 0;
	slave->numtris_slow = 0;
	slave->numtexloads = 0;
	slave->numtexbytes = 0;
#endif

	/* Master stuff */
	pc->numbytes = 0;
	pc->firstl = pc->podlist; /* start off the pod list */
	pc->lastl = pc->firstl; /* end list = start list means list is empty */
	pc->lastcase = 0;
	pc->numpods = 0;

	/* Slave stuff */
	slave->numbytes = 0;
	slave->firstl = slave->podlist;
	slave->lastl = slave->firstl;
	slave->lastcase = 0;
	pc->numpods = 0;

	/* both master and slave */
	q->inid=0;
	q->outid=-1; /* Not first */
	q->outflag = 0;
	q->curpod = pfirstpod;

	q->outflag |= OUTFLAG_SENT;
	q->startaddr = (uint32)GS_GetCurListStart(gs);

	FlushDCacheAll(0); /* Need to flush because slave will access some of our data */

#if TE
	lowlatency = GS_IsLowLatency(gs);
	if (lowlatency) {
	    GS_SendIO(gs,0,((uint32)gs->gs_ListPtr) - q->startaddr); /* Start off the TE */
#endif
	    q->startaddr &= 0x3fffffff;
	}

#if MP
	result = WaitIO(pc->mpioreq);
	if (result < 0) {
	    printf("<M_Draw> MPWaitIO() failed: ");
	    PrintfSysErr(result);
	    exit(-1);
	}

	result = DispatchMPFunc(pc->mpioreq, SlaveFunction, (void*)slave,
				pc->slavestack+(M_STACK_SIZE/4)-2, STACK_SIZE, &slavestatus);
	if (result < 0){
	    printf("<M_Draw> DispatchMPFunc failed \n");
	    PrintfSysErr(result);
	    exit(-1);
	}
#endif

	tricount = DequeueDraw(pc);
	if (!pc->mp) return tricount;
#if MP
	result = WaitIO(pc->mpioreq);
	if (result < 0) {
	    printf("<M_Draw> MPWaitIO() failed: ");
	    PrintfSysErr(result);
	    exit(-1);
	}
#endif
	tricount += slavestatus;
#ifdef STATISTICS
	pc->numpods_fast += slave->numpods_fast;
	pc->numpods_slow += slave->numpods_slow;
	pc->numtris_fast += slave->numtris_fast;
	pc->numtris_slow += slave->numtris_slow;
	pc->numtexloads += slave->numtexloads;
	pc->numtexbytes += slave->numtexbytes;
#endif

	numbytes = pc->numbytes+slave->numbytes;

	/* Refresh cache line w/ out info */
	FlushDCache(0,&q->outlock, DCACHELINESIZE);

	/* Send all remaining pods off */
	while (numbytes > 0) {
#ifdef BUILD_STRINGS
	    savepl=pc->firstl;
	    saveslavepl=slave->firstl;
#endif /*BUILD_STRINGS*/

	    /* Send off as many pods as we can */
	    
	    for (pl=pc->firstl; pl != pc->lastl; pl = ++pc->firstl) {

		if (q->outid != (pl->podid-1)) {
		    break;
		}
		q->outid++;

		bytecount=((uint32)pl->endlist)-((uint32)pl->startlist)+JUMP_BYTES;
#if TE
		ltf = gs->gs_ListPtr;
		CLT_JumpAbsolute(GS_Ptr(gs), pl->startlist);
#endif
		ltf += JUMP_WORDS;  /* Now point to the return address */
		CLT_JumpAbsolute(&pl->endlist, ltf);
		numbytes -= bytecount;
	    }

	    for (pl=slave->firstl; pl != slave->lastl; pl = ++slave->firstl) {

		if (q->outid != (pl->podid-1)) {
		    break;
		}
		q->outid++;

		bytecount=((uint32)pl->endlist)-((uint32)pl->startlist)+JUMP_BYTES;
#if TE
		ltf = gs->gs_ListPtr;
		CLT_JumpAbsolute(GS_Ptr(gs), pl->startlist);
#endif
		ltf += JUMP_WORDS;  /* Now point to the return address */
		CLT_JumpAbsolute(&pl->endlist, ltf);
		numbytes -= bytecount;
	    }

#ifdef BUILD_STRINGS
	    /* Check to see if we're stuck */
	    if ((savepl == pc->firstl) &&
		(saveslavepl == slave->firstl)) {
		stuckcount++;
	    }
#endif /*BUILD_STRINGS*/
	}

#ifdef BUILD_STRINGS
	if ((stuckcount > 0)) {
	    printf("<M_Draw> was stuck %d times\n",stuckcount);
	}
#endif /*BUILD_STRINGS*/

	if (lowlatency && (q->outflag & OUTFLAG_SENT)) {
	    /* we must wait until the list is started */
	    uint32 	teaddr = (uint32)GetTEReadPointer();
	    uint32	endlist = ((uint32)gs->gs_ListPtr)&0x3fffffff;

	    while(teaddr < q->startaddr ||
		  teaddr > endlist) {
		teaddr = (uint32)GetTEReadPointer();
	    }
	}

	pret = tricount;

    } else {
	/* UP case */
    
	pc->pVIwrite = (uint32 *)gs->gs_ListPtr;
	pc->pVIwritemax = ((uint32 *)gs->gs_EndList)-pc->watermark;
	pret = M_DrawA(pfirstpod, pc);
	gs->gs_ListPtr = pc->pVIwrite;
    }
    return pret;
}

