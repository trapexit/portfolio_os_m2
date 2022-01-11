#include "mercury.h"

void M_SetupDynLitTex(CloseData *pc)
{
    pc->pdrawroutine = &M_DrawDynLitTex;
    pc->plightreturn = &M_LightReturnDynLitTex;

    M_Reserve(M_TBLitTex_Size+M_DBNoBlend_Size);

    CLT_Sync(&pc->pVIwrite);
    M_TBLitTex(&pc->pVIwrite);
    M_DBNoBlend(&pc->pVIwrite);
}

