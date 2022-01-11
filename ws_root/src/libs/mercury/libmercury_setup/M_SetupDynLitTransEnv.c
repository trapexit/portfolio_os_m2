#include "mercury.h"

void M_SetupDynLitTransEnv(CloseData *pc)
{
    pc->pdrawroutine = &M_DrawDynLitEnv;
    pc->plightreturn = &M_LightReturnDynLitEnv;

    M_Reserve(M_TBLitTex_Size+M_DBTrans_Size);

    CLT_Sync(&pc->pVIwrite);
    M_TBLitTex(&pc->pVIwrite);
    M_DBTrans(&pc->pVIwrite, pc->srcaddr, pc->fscreenwidth, pc->depth==32);
}

