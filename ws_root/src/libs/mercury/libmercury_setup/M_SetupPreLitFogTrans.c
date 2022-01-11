#include "mercury.h"

void M_SetupPreLitFogTrans(CloseData *pc)
{
    pc->pdrawroutine = &M_DrawPreLitTrans;
    pc->plightreturn = &M_LightReturnPreLitTrans;

    M_Reserve(M_TBFog_Size+M_DBTrans_Size);

    CLT_Sync(&pc->pVIwrite);
    M_TBFog(&pc->pVIwrite, pc->fogcolor);
    M_DBTrans(&pc->pVIwrite, pc->srcaddr, pc->fscreenwidth, pc->depth==32);
}

