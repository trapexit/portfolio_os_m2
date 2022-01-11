#include "mercury.h"

void M_SetupDynLitEnv(CloseData *pc)
{
    pc->pdrawroutine = &M_DrawDynLitEnv;
    pc->plightreturn = &M_LightReturnDynLitEnv;

    M_Reserve(M_TBLitTex_Size+M_DBNoBlend_Size);

    CLT_Sync(&pc->pVIwrite);
    M_TBLitTex(&pc->pVIwrite);
    M_DBNoBlend(&pc->pVIwrite);
}

