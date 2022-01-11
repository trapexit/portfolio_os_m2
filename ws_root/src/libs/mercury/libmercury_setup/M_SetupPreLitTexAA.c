#include "mercury.h"

void M_SetupPreLitTexAA(CloseData *pc)
{
    pc->pdrawroutine = &M_DrawPreLitTexAA;
    if (!(pc->aa & AA2NDPASS)) {
	pc->plightreturn = &M_LightReturnPreLitTexAA;
	M_Reserve(M_TBLitTex_Size+M_DBNoBlend_Size);
    	CLT_Sync(&pc->pVIwrite);
	M_TBLitTex(&pc->pVIwrite);
    	M_DBNoBlend(&pc->pVIwrite);
    } else if (pc->aa & AALINEDRAW) {
	M_Reserve(M_TBNoTex_Size);
	CLT_Sync(&pc->pVIwrite);
	M_TBNoTex(&pc->pVIwrite);
    } else {
	M_Reserve(M_DBTrans_Size);
	CLT_Sync(&pc->pVIwrite);
	M_DBTrans(&pc->pVIwrite,pc->srcaddr, pc->fscreenwidth, pc->depth==32);
    }
}
