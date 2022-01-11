#include "mercury.h"

void M_SetupPreLitTrans(CloseData *pc)
{
    pc->pdrawroutine = &M_DrawPreLitTrans;
    pc->plightreturn = &M_LightReturnPreLitTrans;

    M_Reserve(M_TBNoTex_Size+M_DBTrans_Size);

    CLT_Sync(&pc->pVIwrite);
    M_TBNoTex(&pc->pVIwrite);
    M_DBTrans(&pc->pVIwrite, pc->srcaddr, pc->fscreenwidth, pc->depth==32);
}

