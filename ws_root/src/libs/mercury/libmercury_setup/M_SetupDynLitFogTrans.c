#include "mercury.h"

void M_SetupDynLitFogTrans(CloseData *pc)
{
    pc->pdrawroutine = &M_DrawDynLitTrans;
    pc->plightreturn = &M_LightReturnDynLitTrans;

    M_Reserve(M_TBFog_Size+M_DBTrans_Size);

    CLT_Sync(&pc->pVIwrite);
    M_TBFog(&pc->pVIwrite, pc->fogcolor);
    M_DBTrans(&pc->pVIwrite, pc->srcaddr, pc->fscreenwidth, pc->depth==32);
}

