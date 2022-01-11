#ifdef MACINTOSH
#include <kernel:types.h>
#include <kernel:mem.h>
#else
#include <kernel/types.h>
#include <kernel/mem.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "matrix.h"
#include "mercury.h"

float *pvtxbuf;
float *pedgevtxbuf;
uint32 *pedgebuf;
uint16 *pedgeidbuf;

uint32 edgevtxcnt;
uint32 edgecnt;

#define MAX_EDGE 16384


static bool
add_edge_vertex(uint32 idx, uint32 *vidx)
{
    uint32 i;
    float *pv;
    float x, y, z;
    float vx, vy, vz;
    float *pvtx;

    pv = pvtxbuf + idx*3;
    x = *pv;
    y = *(pv+1);
    z = *(pv+2);

    pvtx = pedgevtxbuf;
    for (i=0; i<edgevtxcnt; i++) {
	vx = *pvtx++;
	vy = *pvtx++;
	vz = *pvtx++;
	if ((vx==x)&&(vy==y)&&(vz==z)) {
	    *vidx = i;
	    return 0;	/* not new */
	}
    }
    *pvtx++ = x;
    *pvtx++ = y;
    *pvtx = z;
    *vidx = edgevtxcnt;
    edgevtxcnt++;
    return 1;	/* new */
}

static uint32
add_silhouette_edge(uint32 idx0, uint32 idx1)
{
    uint32 i;
    uint32 *pedge;
    uint32 vidx0, vidx1;
    uint32 eidx0, eidx1, eidx;
    bool new0, new1;

    new0 = add_edge_vertex(idx0, &vidx0);
    new1 = add_edge_vertex(idx1, &vidx1);

    eidx0 = (vidx0 << 16) | vidx1;
    if (new0 || new1) {
	pedge = pedgebuf + edgecnt;
    } else {
	eidx1 = (vidx1 << 16) | vidx0;

	pedge = pedgebuf;
	for (i=0; i<edgecnt; i++) {
	    eidx = *pedge++;
	    if ((eidx == eidx0) || (eidx == eidx1)) {
		return i;
	    }
	}
    }
    *pedge = eidx0;
    edgecnt++;
    if (edgecnt > MAX_EDGE) {
	printf("edge cnt exceeds limit \n"); 
    }
    return edgecnt-1;
}

static uint32 walk_index_array(Pod *pod)
{	
    uint16 *pedgeid;
    int16 *pidx;
    int32 idx_nxt, idx0, idx1, idx2, idx_cur;
    uint32 id_nxt, id0, id1, id2, id_cur;
    uint32 tid_nxt, tid0, tid1, tid2;
    uint32 tri, tri1;
    bool pclk, pfan, cfan, news, stxt, alterprim;
    idx2=0;
    id2=0;
    /*
     * walk through the index array, look for silhouette edges.
     */
    pedgeid = pedgeidbuf;
    pidx = (int16 *)pod->pgeometry->pindex;
	
    id_nxt = 0;
    idx_nxt = *pidx++;
    stxt = (idx_nxt & STXT) ? 1 : 0;
    while(1) {
	if (stxt) {
	    if (*pidx++ < 0) {
		break;
	    }
	}
	idx0 = idx_nxt & 0x7ff;
	idx1 = *pidx++ & 0x7ff;
	idx_nxt = *pidx++ & 0x7ff;

	id0 = id_nxt++;
	id1 = id_nxt++;

	tid0 = 0;
	tid1 = 1;
	tid2 = 2;

	/* start edge, strip, ccw */
	*(pedgeid+id0) = add_silhouette_edge(idx0,idx1);

	while (1) {
	    idx_cur = idx_nxt;
	    id_cur = id_nxt++;

	    idx_nxt = *pidx++;
	    pclk = (idx_nxt & PCLK) ? 1 : 0;
	    pfan = (idx_nxt & PFAN) ? 1 : 0;
	    cfan = (idx_nxt & CFAN) ? 1 : 0;
	    news = (idx_nxt & NEWS) ? 1 : 0;
	    stxt = (idx_nxt & STXT) ? 1 : 0;
	    alterprim = pfan ^ cfan;
	    idx_nxt = idx_nxt & 0x7ff;

	    /*
	     * there are 48 cases, combine tid0,tid1,tid2 and
	     * pfan,pclk,alterprim.
	     */
	    tri = (tid0<<8)|(tid1<<4)|tid2;
	    tri1 = (pfan<<8)|(pclk<<4)|alterprim;
	    switch (tri) {
		/***********
		 * tri 012 *
		 ***********/
	    case 0x012:
		idx2 = idx_cur;
		id2 = id_cur;
		switch (tri1) {
		case 0x000: 	/* strip, ccw, no alterprim */
		    /* b: 20, e: 12 */
		    *(pedgeid+id2) = add_silhouette_edge(idx2,idx0);
		    if (news) {
			*(pedgeid+id1) = add_silhouette_edge(idx1,idx2);
		    }
		    break;

		case 0x001: 	/* strip, ccw,    alterprim */
		    /* b: 20, e: 12 */
		    if (news) {
			*(pedgeid+id2) = add_silhouette_edge(idx2,idx0);
		    }
		    *(pedgeid+id1) = add_silhouette_edge(idx1,idx2);
		    break;

		case 0x010: 	/* strip,  cw, no alterprim */
		    /* b: 02, e: 21 */
		    *(pedgeid+id0) = add_silhouette_edge(idx0,idx2);
		    if (news) {
			*(pedgeid+id2) = add_silhouette_edge(idx2,idx1);
		    }
		    break;

		case 0x011: 	/* strip,  cw,    alterprim */
		    /* b: 02, e: 21 */
		    if (news) {
			*(pedgeid+id0) = add_silhouette_edge(idx0,idx2);
		    }
		    *(pedgeid+id2) = add_silhouette_edge(idx2,idx1);
		    break;

		case 0x100: 	/*   fan, ccw, no alterprim */
		    /* b: 12, e: 20 */
		    *(pedgeid+id1) = add_silhouette_edge(idx1,idx2);
		    if (news) {
			*(pedgeid+id2) = add_silhouette_edge(idx2,idx0);
		    }
		    break;

		case 0x101: 	/*   fan, ccw,    alterprim */
		    /* b: 12, e: 20 */
		    if (news) {
			*(pedgeid+id1) = add_silhouette_edge(idx1,idx2);
		    }
		    *(pedgeid+id2) = add_silhouette_edge(idx2,idx0);
		    break;

		case 0x110: 	/*   fan,  cw, no alterprim */
		    /* b: 21, e: 02 */
		    *(pedgeid+id2) = add_silhouette_edge(idx2,idx1);
		    if (news) {
			*(pedgeid+id0) = add_silhouette_edge(idx0,idx2);
		    }
		    break;

		case 0x111: 	/*   fan,  cw,    alterprim */
		    /* b: 21, e: 02 */
		    if (news) {
			*(pedgeid+id2) = add_silhouette_edge(idx2,idx1);
		    }
		    *(pedgeid+id0) = add_silhouette_edge(idx0,idx2);
		    break;
		default:
		    break;

		}
		break;

		/***********
		 * tri 021 *
		 ***********/
	    case 0x021:
		idx1 = idx_cur;
		id1 = id_cur;
		switch (tri1) {
		case 0x000: 	/* strip, ccw, no alterprim */
		    /* b: 01, e: 12 */
		    *(pedgeid+id0) = add_silhouette_edge(idx0,idx1);
		    if (news) {
			*(pedgeid+id1) = add_silhouette_edge(idx1,idx2);
		    }
		    break;

		case 0x001: 	/* strip, ccw,    alterprim */
		    /* b: 01, e: 12 */
		    if (news) {
			*(pedgeid+id0) = add_silhouette_edge(idx0,idx1);
		    }
		    *(pedgeid+id1) = add_silhouette_edge(idx1,idx2);
		    break;

		case 0x010: 	/* strip,  cw, no alterprim */
		    /* b: 10, e: 21 */
		    *(pedgeid+id1) = add_silhouette_edge(idx1,idx0);
		    if (news) {
			*(pedgeid+id2) = add_silhouette_edge(idx2,idx1);
		    }
		    break;

		case 0x011: 	/* strip,  cw,    alterprim */
		    /* b: 10, e: 21 */
		    if (news) {
			*(pedgeid+id1) = add_silhouette_edge(idx1,idx0);
		    }
		    *(pedgeid+id2) = add_silhouette_edge(idx2,idx1);
		    break;

		case 0x100: 	/*   fan, ccw, no alterprim */
		    /* b: 12, e: 01 */
		    *(pedgeid+id1) = add_silhouette_edge(idx1,idx2);
		    if (news) {
			*(pedgeid+id0) = add_silhouette_edge(idx0,idx1);
		    }
		    break;

		case 0x101: 	/*   fan, ccw,    alterprim */
		    /* b: 12, e: 01 */
		    if (news) {
			*(pedgeid+id1) = add_silhouette_edge(idx1,idx2);
		    }
		    *(pedgeid+id0) = add_silhouette_edge(idx0,idx1);
		    break;

		case 0x110: 	/*   fan,  cw, no alterprim */
		    /* b: 21, e: 10 */
		    *(pedgeid+id2) = add_silhouette_edge(idx2,idx1);
		    if (news) {
			*(pedgeid+id1) = add_silhouette_edge(idx1,idx0);
		    }
		    break;

		case 0x111: 	/*   fan,  cw,    alterprim */
		    /* b: 21, e: 10 */
		    if (news) {
			*(pedgeid+id2) = add_silhouette_edge(idx2,idx1);
		    }
		    *(pedgeid+id1) = add_silhouette_edge(idx1,idx0);
		    break;
		default:
		    break;
		}
		break;

		/***********
		 * tri 102 *
		 ***********/
	    case 0x102:
		idx2 = idx_cur;
		id2 = id_cur;
		switch (tri1) {
		case 0x000: 	/* strip, ccw, no alterprim */
		    /* b: 12, e: 20 */
		    *(pedgeid+id1) = add_silhouette_edge(idx1,idx2);
		    if (news) {
			*(pedgeid+id2) = add_silhouette_edge(idx2,idx0);
		    }
		    break;

		case 0x001: 	/* strip, ccw,    alterprim */
		    /* b: 12, e: 20 */
		    if (news) {
			*(pedgeid+id1) = add_silhouette_edge(idx1,idx2);
		    }
		    *(pedgeid+id2) = add_silhouette_edge(idx2,idx0);
		    break;

		case 0x010: 	/* strip,  cw, no alterprim */
		    /* b: 21, e: 02 */
		    *(pedgeid+id2) = add_silhouette_edge(idx2,idx1);
		    if (news) {
			*(pedgeid+id0) = add_silhouette_edge(idx0,idx2);
		    }
		    break;

		case 0x011: 	/* strip,  cw,    alterprim */
		    /* b: 21, e: 02 */
		    if (news) {
			*(pedgeid+id2) = add_silhouette_edge(idx2,idx1);
		    }
		    *(pedgeid+id0) = add_silhouette_edge(idx0,idx2);
		    break;

		case 0x100: 	/*   fan, ccw, no alterprim */
		    /* b: 20, e: 12 */
		    *(pedgeid+id2) = add_silhouette_edge(idx2,idx0);
		    if (news) {
			*(pedgeid+id1) = add_silhouette_edge(idx1,idx2);
		    }
		    break;

		case 0x101: 	/*   fan, ccw,    alterprim */
		    /* b: 20, e: 12 */
		    if (news) {
			*(pedgeid+id2) = add_silhouette_edge(idx2,idx0);
		    }
		    *(pedgeid+id1) = add_silhouette_edge(idx1,idx2);
		    break;

		case 0x110: 	/*   fan,  cw, no alterprim */
		    /* b: 02, e: 21 */
		    *(pedgeid+id0) = add_silhouette_edge(idx0,idx2);
		    if (news) {
			*(pedgeid+id2) = add_silhouette_edge(idx2,idx1);
		    }
		    break;

		case 0x111: 	/*   fan,  cw,    alterprim */
		    /* b: 02, e: 21 */
		    if (news) {
			*(pedgeid+id0) = add_silhouette_edge(idx0,idx2);
		    }
		    *(pedgeid+id2) = add_silhouette_edge(idx2,idx1);
		    break;
		default:
		    break;
		}
		break;

		/***********
		 * tri 120 *
		 ***********/
	    case 0x120:
		idx0 = idx_cur;
		id0 = id_cur;
		switch (tri1) {
		case 0x000: 	/* strip, ccw, no alterprim */
		    /* b: 01, e: 20 */
		    *(pedgeid+id0) = add_silhouette_edge(idx0,idx1);
		    if (news) {
			*(pedgeid+id2) = add_silhouette_edge(idx2,idx0);
		    }
		    break;

		case 0x001: 	/* strip, ccw,    alterprim */
		    /* b: 01, e: 20 */
		    if (news) {
			*(pedgeid+id0) = add_silhouette_edge(idx0,idx1);
		    }
		    *(pedgeid+id2) = add_silhouette_edge(idx2,idx0);
		    break;

		case 0x010: 	/* strip,  cw, no alterprim */
		    /* b: 10, e: 02 */
		    *(pedgeid+id1) = add_silhouette_edge(idx1,idx0);
		    if (news) {
			*(pedgeid+id0) = add_silhouette_edge(idx0,idx2);
		    }
		    break;

		case 0x011: 	/* strip,  cw,    alterprim */
		    /* b: 10, e: 02 */
		    if (news) {
			*(pedgeid+id1) = add_silhouette_edge(idx1,idx0);
		    }
		    *(pedgeid+id0) = add_silhouette_edge(idx0,idx2);
		    break;

		case 0x100: 	/*   fan, ccw, no alterprim */
		    /* b: 20, e: 01 */
		    *(pedgeid+id2) = add_silhouette_edge(idx2,idx0);
		    if (news) {
			*(pedgeid+id0) = add_silhouette_edge(idx0,idx1);
		    }
		    break;

		case 0x101: 	/*   fan, ccw,    alterprim */
		    /* b: 20, e: 01 */
		    if (news) {
			*(pedgeid+id2) = add_silhouette_edge(idx2,idx0);
		    }
		    *(pedgeid+id0) = add_silhouette_edge(idx0,idx1);
		    break;

		case 0x110: 	/*   fan,  cw, no alterprim */
		    /* b: 02, e: 10 */
		    *(pedgeid+id0) = add_silhouette_edge(idx0,idx2);
		    if (news) {
			*(pedgeid+id1) = add_silhouette_edge(idx1,idx0);
		    }
		    break;

		case 0x111: 	/*   fan,  cw,    alterprim */
		    /* b: 02, e: 10 */
		    if (news) {
			*(pedgeid+id0) = add_silhouette_edge(idx0,idx2);
		    }
		    *(pedgeid+id1) = add_silhouette_edge(idx1,idx0);
		    break;
		default:
		    break;
		}
		break;

		/***********
		 * tri 201 *
		 ***********/
	    case 0x201:
		idx1 = idx_cur;
		id1 = id_cur;
		switch (tri1) {
		case 0x000: 	/* strip, ccw, no alterprim */
		    /* b: 12, e: 01 */
		    *(pedgeid+id1) = add_silhouette_edge(idx1,idx2);
		    if (news) {
			*(pedgeid+id0) = add_silhouette_edge(idx0,idx1);
		    }
		    break;

		case 0x001: 	/* strip, ccw,    alterprim */
		    /* b: 12, e: 01 */
		    if (news) {
			*(pedgeid+id1) = add_silhouette_edge(idx1,idx2);
		    }
		    *(pedgeid+id0) = add_silhouette_edge(idx0,idx1);
		    break;

		case 0x010: 	/* strip,  cw, no alterprim */
		    /* b: 21, e: 10 */
		    *(pedgeid+id2) = add_silhouette_edge(idx2,idx1);
		    if (news) {
			*(pedgeid+id1) = add_silhouette_edge(idx1,idx0);
		    }
		    break;

		case 0x011: 	/* strip,  cw,    alterprim */
		    /* b: 21, e: 10 */
		    if (news) {
			*(pedgeid+id2) = add_silhouette_edge(idx2,idx1);
		    }
		    *(pedgeid+id1) = add_silhouette_edge(idx1,idx0);
		    break;

		case 0x100: 	/*   fan, ccw, no alterprim */
		    /* b: 01, e: 12 */
		    *(pedgeid+id0) = add_silhouette_edge(idx0,idx1);
		    if (news) {
			*(pedgeid+id1) = add_silhouette_edge(idx1,idx2);
		    }
		    break;

		case 0x101: 	/*   fan, ccw,    alterprim */
		    /* b: 01, e: 12 */
		    if (news) {
			*(pedgeid+id0) = add_silhouette_edge(idx0,idx1);
		    }
		    *(pedgeid+id1) = add_silhouette_edge(idx1,idx2);
		    break;

		case 0x110: 	/*   fan,  cw, no alterprim */
		    /* b: 10, e: 21 */
		    *(pedgeid+id1) = add_silhouette_edge(idx1,idx0);
		    if (news) {
			*(pedgeid+id2) = add_silhouette_edge(idx2,idx1);
		    }
		    break;

		case 0x111: 	/*   fan,  cw,    alterprim */
		    /* b: 10, e: 21 */
		    if (news) {
			*(pedgeid+id1) = add_silhouette_edge(idx1,idx0);
		    }
		    *(pedgeid+id2) = add_silhouette_edge(idx2,idx1);
		    break;
		default:
		    break;
		}
		break;

		/***********
		 * tri 210 *
		 ***********/
	    case 0x210:
		idx0 = idx_cur;
		id0 = id_cur;
		switch (tri1) {
		case 0x000: 	/* strip, ccw, no alterprim */
		    /* b: 20, e: 01 */
		    *(pedgeid+id2) = add_silhouette_edge(idx2,idx0);
		    if (news) {
			*(pedgeid+id0) = add_silhouette_edge(idx0,idx1);
		    }
		    break;

		case 0x001: 	/* strip, ccw,    alterprim */
		    /* b: 20, e: 01 */
		    if (news) {
			*(pedgeid+id2) = add_silhouette_edge(idx2,idx0);
		    }
		    *(pedgeid+id0) = add_silhouette_edge(idx0,idx1);
		    break;

		case 0x010: 	/* strip,  cw, no alterprim */
		    /* b: 02, e: 10 */
		    *(pedgeid+id0) = add_silhouette_edge(idx0,idx2);
		    if (news) {
			*(pedgeid+id1) = add_silhouette_edge(idx1,idx0);
		    }
		    break;

		case 0x011: 	/* strip,  cw,    alterprim */
		    /* b: 02, e: 10 */
		    if (news) {
			*(pedgeid+id0) = add_silhouette_edge(idx0,idx2);
		    }
		    *(pedgeid+id1) = add_silhouette_edge(idx1,idx0);
		    break;

		case 0x100: 	/*   fan, ccw, no alterprim */
		    /* b: 01, e: 20 */
		    *(pedgeid+id0) = add_silhouette_edge(idx0,idx1);
		    if (news) {
			*(pedgeid+id2) = add_silhouette_edge(idx2,idx0);
		    }
		    break;

		case 0x101: 	/*   fan, ccw,    alterprim */
		    /* b: 01, e: 20 */
		    if (news) {
			*(pedgeid+id0) = add_silhouette_edge(idx0,idx1);
		    }
		    *(pedgeid+id2) = add_silhouette_edge(idx2,idx0);
		    break;

		case 0x110: 	/*   fan,  cw, no alterprim */
		    /* b: 10, e: 02 */
		    *(pedgeid+id1) = add_silhouette_edge(idx1,idx0);
		    if (news) {
			*(pedgeid+id0) = add_silhouette_edge(idx0,idx2);
		    }
		    break;

		case 0x111: 	/*   fan,  cw,    alterprim */
		    /* b: 10, e: 02 */
		    if (news) {
			*(pedgeid+id1) = add_silhouette_edge(idx1,idx0);
		    }
		    *(pedgeid+id0) = add_silhouette_edge(idx0,idx2);
		    break;
		default:
		    break;
		}
		break;

	    default:
		break;
	    }
	    if (news) {
		break;
	    }
	    if (cfan) {
		tid_nxt = tid1;
		tid1 = tid2;
		tid2 = tid_nxt;
	    } else {
		tid_nxt = tid0;
		tid0 = tid1;
		tid1 = tid2;
		tid2 = tid_nxt;
	    }
	}
    }
    return(id_nxt);
}

static uint32 CountIndices (Pod *ppod)
{
    short *pidx;
    uint16 idx;
    uint32 idxcnt = 0;

    pidx = ppod->pgeometry->pindex;

    while(1) {
	idx = (uint16)*pidx++;
	idxcnt++;
	if (idx & STXT) {
	    if (*pidx++ < 0) {
		break;
	    } 
	}
    }
    return idxcnt;
}


/*
 * for each pod, transform the vertices, walk through the index list,
 * alloc a global silhouette edge table.
 */
void M_BuildAAEdgeTable(Pod *ppod)
{
    uint32 idxcnt;
    AAData *aadat;
    PodGeometry *geo;
    uint32 vtxcnt;

    aadat = (AAData*)AllocMem(sizeof(AAData),MEMTYPE_NORMAL);

    idxcnt = CountIndices(ppod);
    geo = ppod->pgeometry;
    geo->paaedge = (uint16*)AllocMemAligned(idxcnt*sizeof(uint16),MEMTYPE_TRACKSIZE,32);
    ppod->paadata = aadat;
    vtxcnt = geo->vertexcount+geo->sharedcount;

    pvtxbuf = geo->pvertex;
    pedgevtxbuf = (float *)AllocMem(vtxcnt*3*sizeof(float),MEMTYPE_NORMAL);
    pedgebuf = (uint32 *)AllocMem(MAX_EDGE*sizeof(uint32),MEMTYPE_NORMAL);

    edgevtxcnt = 0;
    edgecnt = 0;

    pedgeidbuf = ppod->pgeometry->paaedge;
    walk_index_array(ppod);

    FreeMem(pedgevtxbuf,vtxcnt*3*sizeof(float));
    FreeMem(pedgebuf,MAX_EDGE*sizeof(uint32));

    aadat->edgecount = edgecnt;
    aadat->flags = 0;
    aadat->paaedgebuffer = AllocMemAligned(edgecnt,MEMTYPE_NORMAL,32);
}

void M_FreeAAEdgeTable(Pod *ppod)
{
    AAData *aadat = ppod->paadata;

    FreeMem(ppod->pgeometry->paaedge,TRACKED_SIZE);
    FreeMem(aadat->paaedgebuffer,aadat->edgecount);
    FreeMem(aadat,sizeof(AAData));
    ppod->paadata = 0;
    ppod->pgeometry->paaedge = 0;
}
