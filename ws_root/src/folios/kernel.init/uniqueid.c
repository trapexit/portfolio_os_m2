/* @(#) uniqueid.c 96/06/26 1.3 */

/* Dallas ("LoneStar") unique id chip support.
 * This code checks for the presence of the unique id chip, and if present,
 * snarfs the 64 bit data from the chip and stores it in KernelBase->kb_UniqueID.
 *
 * unique id is of the form :
 *   8 bits CRC code
 *  48 bits serial number
 *   8 bits family code (0x1 for dallas parts)
 */

#include <kernel/types.h>
#include <kernel/kernel.h>
#include <kernel/uniqueid.h>
#include <hardware/cde.h>
#include <stdio.h>


/*****************************************************************************/


#define LS_MASK    0x100000
#define LS_WAIT()  {while(!(CDE_READ(KB_FIELD(kb_CDEBase),CDE_INT_STS) & LS_MASK));}
#define LS_CLEAR() (CDE_CLR(KB_FIELD(kb_CDEBase),CDE_INT_STS,LS_MASK))

/* The following calculations are based on BDA clock speed (in MHz), which is
 * 2x bus clock. The many magic numbers are courtesy of Edouard Landau
 */
#define CLK_SPD   (2*KB_FIELD(kb_BusClk)/1000000)
#define LS_RESET  (0x10000 | ((uint32)(2.340000*CLK_SPD) << 8) | (uint32)(0.46*CLK_SPD))
#define LS_WRITE0 (0x40000 | ((uint32)(0.937500*CLK_SPD) << 8) | (uint32)(1))
#define LS_WRITE1 (0x40000 | ((uint32)(0.093750*CLK_SPD) << 8) | (uint32)(1))
#define LS_READ   (0x20000 | ((uint32)(0.031250*CLK_SPD) << 8) | (uint32)(0.1718*CLK_SPD))


/*****************************************************************************/


/* write a command to the dallas part via CDE */
static void LoneStarCmd(uint32 cmd)
{
    CDE_WRITE(KB_FIELD(kb_CDEBase),CDE_UNIQ_ID_CMD,cmd);
    LS_CLEAR();             /* workaround for CDE bug */
    LS_WAIT();
}


/*****************************************************************************/


void GetUniqueID(void)
{
uint32 bit;
uint32 i;

    /* attemp to reset the part via CDE */
    LoneStarCmd(LS_RESET);

    if (CDE_READ(KB_FIELD(kb_CDEBase),CDE_UNIQ_ID_RD) == 0)
    {
        /* chip is present */

        /* send dallas write command 0x33 */
        LoneStarCmd(LS_WRITE1);
        LoneStarCmd(LS_WRITE1);
        LoneStarCmd(LS_WRITE0);
        LoneStarCmd(LS_WRITE0);
        LoneStarCmd(LS_WRITE1);
        LoneStarCmd(LS_WRITE1);
        LoneStarCmd(LS_WRITE0);
        LoneStarCmd(LS_WRITE0);

        /* read 64 bits from the part */
        for (i = 0; i < 64; i++)
        {
            LoneStarCmd(LS_READ);
	    bit = CDE_READ(KB_FIELD(kb_CDEBase), CDE_UNIQ_ID_RD) & 1;
            if (i < 32)
                KB_FIELD(kb_UniqueID)[1] |= (bit << i);
            else
                KB_FIELD(kb_UniqueID)[0] |= (bit << (i-32));
        }
        KB_FIELD(kb_Flags) |= KB_UNIQUEID;
    }
}
