#include "mercury.h"

void M_SetupDynLitSpecTex(CloseData *pc)
{
    pc->pdrawroutine = &M_DrawDynLitTex;
    pc->plightreturn = &M_LightReturnDynLitTex;

    M_Reserve(M_TBLitTex_Size+M_DBSpec_Size);

    CLT_Sync(&pc->pVIwrite);
    M_TBLitTex(&pc->pVIwrite);
    M_DBSpec(&pc->pVIwrite);
}

