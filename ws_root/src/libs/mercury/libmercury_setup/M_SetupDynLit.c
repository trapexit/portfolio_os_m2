#include "mercury.h"

void M_SetupDynLit(CloseData *pc)
{
    pc->pdrawroutine = &M_DrawDynLit;
    pc->plightreturn = &M_LightReturnDynLit;

	M_Reserve(M_TBNoTex_Size+M_DBNoBlend_Size);

    CLT_Sync(&pc->pVIwrite);
    M_TBNoTex(&pc->pVIwrite);
    M_DBNoBlend(&pc->pVIwrite);
}
