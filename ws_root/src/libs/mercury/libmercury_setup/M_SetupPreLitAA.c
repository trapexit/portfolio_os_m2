#include "mercury.h"

void M_SetupPreLitAA(CloseData *pc)
{
    pc->pdrawroutine = &M_DrawPreLitAA;
    if (!(pc->aa & AA2NDPASS)) {
	pc->plightreturn = &M_LightReturnPreLitAA;
	M_Reserve(M_TBNoTex_Size+M_DBNoBlend_Size);
	CLT_Sync(&pc->pVIwrite);
	M_TBNoTex(&pc->pVIwrite);
	M_DBNoBlend(&pc->pVIwrite);
    } else if (!(pc->aa & AALINEDRAW)) {
	M_Reserve(M_DBTrans_Size);
	CLT_Sync(&pc->pVIwrite);
	M_DBTrans(&pc->pVIwrite,pc->srcaddr, pc->fscreenwidth, pc->depth==32);
    }
}

