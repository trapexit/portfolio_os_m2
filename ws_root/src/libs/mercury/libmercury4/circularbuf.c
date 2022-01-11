#include "mercury.h"
#include <device/mp.h>

/* The current fake for a circular buffer manager is to call GS_SendList
   which does a SendIO and lets us use another list if one is available */

void M_ClistManagerC(CloseData *pc)
{
    GState	*gs = (GState*)pc->gstate;

    if (pc->mp) {
	/* For MP the buffer should never overflow until
	   circular management is in place */
#ifdef BUILD_STRINGS
	printf("<M_ClistManagerC> CPU <%d> buffer filled up.\n",IsSlaveCPU());
#endif
	if (IsSlaveCPU()) {
	    exit(-1);
	} else {
#ifdef BUILD_STRINGS
	    printf("Reverting to Single Processor Mode\n");
#endif
	    M_EndMP(pc);
	    pc->pVIwrite = (uint32 *)gs->gs_ListPtr;
	    pc->pVIwritemax = ((uint32 *)gs->gs_EndList)-pc->watermark;
	}
    } else {
	gs->gs_ListPtr = (CmdListP)pc->pVIwrite;
	gs->gs_SendList(gs);
	pc->pVIwrite = (uint32 *)gs->gs_ListPtr;
	pc->pVIwritemax = ((uint32 *)gs->gs_EndList)-pc->watermark;
    }
}

