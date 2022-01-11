#include "mercury.h"

void M_SetupPreLitFog(CloseData *pc)
{
    pc->pdrawroutine = &M_DrawPreLit;
    pc->plightreturn = &M_LightReturnPreLit;

    M_Reserve(M_TBNoTex_Size+M_DBFog_Size);

    CLT_Sync(&pc->pVIwrite);
    M_TBNoTex(&pc->pVIwrite);
    M_DBFog(&pc->pVIwrite, pc->fogcolor);
}
