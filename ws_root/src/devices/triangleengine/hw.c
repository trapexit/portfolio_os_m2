/* @(#) hw.c 96/05/10 1.2 */

#include <kernel/types.h>
#include <graphics/bitmap.h>
#include <hardware/te.h>
#include <stdio.h>
#include "hw.h"


/*****************************************************************************/


#define TRACE(x) /* printf x */


/*****************************************************************************/


void ClearTEInterrupts(uint32 ints)
{
uint32 disable;

    disable = 0;

    if (ints & TE_INTRSTAT_FBCLIP)
        disable |= TE_DBLNDRSTATUS_FBCLIP;

    if (ints & TE_INTRSTAT_ALUSTAT)
        disable |= TE_DBLNDRSTATUS_ALUSTAT;

    if (ints & TE_INTRSTAT_ZFUNC)
        disable |= TE_DBLNDRSTATUS_ZFUNC;

    if (ints & TE_INTRSTAT_ANYRENDER)
        disable |= TE_DBLNDRSTATUS_ANYREND;

    if (ints & TE_INTRSTAT_WINCLIP)
        disable |= TE_DBLNDRSTATUS_WINCLIP;

    ClearRegister(TE_DBLNDRSTATUS_ADDR, disable);
    ClearRegister(TE_INTERRUPT_STATUS_ADDR, ints);
}


/*****************************************************************************/


void ClearTEStatus(void)
{
    ClearRegister(TE_DBLNDRSTATUS_ADDR, TE_DBLNDRSTATUS_FBCLIP |
                                        TE_DBLNDRSTATUS_ALUSTAT |
                                        TE_DBLNDRSTATUS_ZFUNC |
                                        TE_DBLNDRSTATUS_ANYREND |
                                        TE_DBLNDRSTATUS_WINCLIP);

    ClearRegister(TE_INTERRUPT_STATUS_ADDR, TE_INTRSTAT_IMMEDIATE |
                                            TE_INTRSTAT_FBCLIP |
                                            TE_INTRSTAT_ALUSTAT |
                                            TE_INTRSTAT_ZFUNC |
                                            TE_INTRSTAT_ANYRENDER |
                                            TE_INTRSTAT_SUPERVISOR |
                                            TE_INTRSTAT_ILLINSTR |
                                            TE_INTRSTAT_SPLINSTR |
                                            TE_INTRSTAT_WINCLIP |
                                            TE_INTRSTAT_LISTEND |
                                            TE_INTRSTAT_IMMINSTR |
                                            TE_INTRSTAT_DEFINSTR |
                                            TE_INTRSTAT_DEFINSTR_VECTOR);
}


/*****************************************************************************/


void InitTE(void)
{
    TRACE(("InitTE: entering\n"));

    /* Reset the triangle engine twice (in case we're recovering from
     * a train wreck). Do it twice cause it seems to work better.
     */
    WriteRegister(TE_MASTER_MODE_ADDR, TE_MM_RESET_TE);
    WriteRegister(TE_MASTER_MODE_ADDR, 0);
    WriteRegister(TE_MASTER_MODE_ADDR, TE_MM_RESET_TE);
    WriteRegister(TE_MASTER_MODE_ADDR, 0);

    /* Point the instruction write pointer to 0 */
    WriteRegister(TE_INSTR_WRITE_PTR_ADDR, 0);

    /* Initialize the controls for various units */
    SetRegister(TE_TXT_MASTER_CONTROL_ADDR, TE_TXT_MC_ENABLE);

    /* Disable frame and Z buffering */
    ClearRegister(TE_DBLNDR_SCNTRL_ADDR, TE_DBLNDRSCNTRL_FBEN);
    WriteRegister(TE_MASTER_MODE_ADDR,   TE_MM_DISABLE_ZBUFFER);

    /* Enable 16 byte writes */
    SetRegister(TE_DBLNDR_SCNTRL_ADDR, TE_DBLNDRSCNTRL_WRZ16EN | TE_DBLNDRSCNTRL_WRFB16EN);

    /* Clear the status registers */
    ClearTEStatus();

    /* Enable interrupts we care about */
    SetRegister(TE_DBLNDR_INTERRUPT_ENABLE_ADDR, 0);
    SetRegister(TE_INTERRUPT_ENABLE_ADDR, TE_INTREN_IMMEDIATE |
                                          TE_INTREN_SUPERVISOR |
                                          TE_INTREN_ILLINSTR |
                                          TE_INTREN_SPLINSTR);
}


/*****************************************************************************/


void SetTEFrameBuffer(const Bitmap *bitmap)
{
uint32 data;

    TRACE(("SetTEFrameBuffer: bitmap %x\n", bitmap));

    if (bitmap == NULL)
    {
        /* turn off frame buffering */
        ClearRegister(TE_DBLNDR_SCNTRL_ADDR, TE_DBLNDRSCNTRL_FBEN);
    }
    else
    {
        /* turn on frame buffering */
        data = (bitmap->bm_ClipWidth << TE_FBFR_XLIP_SHIFT) | bitmap->bm_ClipHeight;
        WriteRegister(TE_FBUFFER_CLIP_ADDR, data);

        if ((bitmap->bm_Type == BMTYPE_16)
         || (bitmap->bm_Type == BMTYPE_16_ZBUFFER))
        {
            data = BITSPERPIXEL_16;
        }
        else
        {
            data = BITSPERPIXEL_32;
        }

        WriteRegister(TE_FBUFFER_CNTRL_ADDR, data);
        WriteRegister(TE_FBUFFER_BASE_ADDR,  bitmap->bm_Buffer);
        WriteRegister(TE_FBUFFER_WIDTH_ADDR, bitmap->bm_Width);
        SetRegister(TE_DBLNDR_SCNTRL_ADDR,   TE_DBLNDRSCNTRL_FBEN);
    }
}


/*****************************************************************************/


void SetTEZBuffer(const Bitmap *bitmap)
{
uint32 data;

    TRACE(("SetTEZBuffer: bitmap %x\n", bitmap));

    if (bitmap == NULL)
    {
        /* turn off Z buffering */
        SetRegister(TE_MASTER_MODE_ADDR, TE_MM_DISABLE_ZBUFFER);
    }
    else
    {
        /* turn on Z buffering */
        data = (bitmap->bm_Width << TE_ZBFR_XLIP_SHIFT) | bitmap->bm_Height;
        WriteRegister(TE_ZBUFFER_CLIP_ADDR, data);
        WriteRegister(TE_ZBUFFER_BASE_ADDR, bitmap->bm_Buffer);
        ClearRegister(TE_MASTER_MODE_ADDR,  TE_MM_DISABLE_ZBUFFER);
    }
}
