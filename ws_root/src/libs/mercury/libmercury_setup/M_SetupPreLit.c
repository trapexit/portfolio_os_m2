#include "mercury.h"

void M_SetupPreLit(CloseData *pc)
{
    pc->pdrawroutine = &M_DrawPreLit;
    pc->plightreturn = &M_LightReturnPreLit;

    M_Reserve(M_TBNoTex_Size+M_DBNoBlend_Size);

    CLT_Sync(&pc->pVIwrite);
    M_TBNoTex(&pc->pVIwrite);
    M_DBNoBlend(&pc->pVIwrite);
}
