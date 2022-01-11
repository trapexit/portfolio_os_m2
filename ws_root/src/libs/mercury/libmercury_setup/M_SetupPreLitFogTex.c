#include "mercury.h"

void M_SetupPreLitFogTex(CloseData *pc)
{
    pc->pdrawroutine = &M_DrawPreLitTex;
    pc->plightreturn = &M_LightReturnPreLitTex;

    M_Reserve(M_TBLitTex_Size+M_DBFog_Size);

    CLT_Sync(&pc->pVIwrite);
    M_TBLitTex(&pc->pVIwrite);
    M_DBFog(&pc->pVIwrite, pc->fogcolor);
}

