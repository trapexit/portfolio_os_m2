#include "mercury.h"

void M_SetupPreLitTransTex(CloseData *pc)
{
    pc->pdrawroutine = &M_DrawPreLitTex;
    pc->plightreturn = &M_LightReturnPreLitTex;

    M_Reserve(M_TBLitTex_Size+M_DBTrans_Size);

    CLT_Sync(&pc->pVIwrite);
    M_TBLitTex(&pc->pVIwrite);
    M_DBTrans(&pc->pVIwrite, pc->srcaddr, pc->fscreenwidth, pc->depth==32);
}

