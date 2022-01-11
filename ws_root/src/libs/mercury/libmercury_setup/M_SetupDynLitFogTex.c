#include "mercury.h"

void M_SetupDynLitFogTex(CloseData *pc)
{
    pc->pdrawroutine = &M_DrawDynLitTex;
    pc->plightreturn = &M_LightReturnDynLitTex;

    M_Reserve(M_TBLitTex_Size+M_DBFog_Size);

    CLT_Sync(&pc->pVIwrite);
    M_TBLitTex(&pc->pVIwrite);
    M_DBFog(&pc->pVIwrite, pc->fogcolor);
}

