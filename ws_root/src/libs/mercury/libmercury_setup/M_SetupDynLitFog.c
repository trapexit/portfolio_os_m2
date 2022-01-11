#include "mercury.h"

void M_SetupDynLitFog(CloseData *pc)
{
    pc->pdrawroutine = &M_DrawDynLit;
    pc->plightreturn = &M_LightReturnDynLit;

    M_Reserve(M_TBNoTex_Size+M_DBFog_Size);

    CLT_Sync(&pc->pVIwrite);
    M_TBNoTex(&pc->pVIwrite);
    M_DBFog(&pc->pVIwrite, pc->fogcolor);
}
