#include "mercury.h"

void M_SetupDynLitTransTex(CloseData *pc)
{
    pc->pdrawroutine = &M_DrawDynLitTex;
    pc->plightreturn = &M_LightReturnDynLitTex;

    M_Reserve(M_TBLitTex_Size+M_DBTrans_Size);

    CLT_Sync(&pc->pVIwrite);
    M_TBLitTex(&pc->pVIwrite);
    M_DBTrans(&pc->pVIwrite, pc->srcaddr, pc->fscreenwidth, pc->depth==32);
}

