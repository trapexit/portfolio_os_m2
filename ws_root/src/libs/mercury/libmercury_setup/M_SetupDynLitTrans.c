#include "mercury.h"

void M_SetupDynLitTrans(CloseData *pc)
{
    pc->pdrawroutine = &M_DrawDynLit;
    pc->plightreturn = &M_LightReturnDynLit;

    M_Reserve(M_TBNoTex_Size+M_DBTrans_Size);

    CLT_Sync(&pc->pVIwrite);
    M_TBNoTex(&pc->pVIwrite);
    M_DBTrans(&pc->pVIwrite, pc->srcaddr, pc->fscreenwidth, pc->depth==32);
}

