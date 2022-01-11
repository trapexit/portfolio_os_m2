/* @(#) printconfig.c 96/08/23 1.8 */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/kernelnodes.h>
#include <kernel/item.h>
#include <kernel/folio.h>
#include <kernel/list.h>
#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <kernel/debug.h>
#include <kernel/operror.h>
#include <kernel/sysinfo.h>
#include <kernel/super.h>
#include <kernel/interrupts.h>
#include <kernel/panic.h>
#include <kernel/cache.h>
#include <kernel/time.h>
#include <kernel/internalf.h>
#include <kernel/memlock.h>
#include <loader/loader3do.h>
#include <file/discdata.h>
#include <string.h>
#include <stdio.h>
#include <hardware/PPCasm.h>


/*****************************************************************************/


#ifdef BUILD_STRINGS
void DumpCPU(uint32 cpuRev)
{
    switch (cpuRev)
    {
        case 0x50200: printf("PPC 602 2.0"); break;
        case 0x50201: printf("PPC 602 2.1"); break;
        case 0x50202: printf("PPC 602 2.2"); break;
        default     : printf("PPC<%x>", cpuRev); break;
    }
}
#endif


/*****************************************************************************/


#ifdef BUILD_STRINGS
void PrintSysConfig(void)
{
uint32 busclk;
uint32 cpuclk;

    printf("\nBUILD : %s\n",BUILD_PATH);
    printf("SYSTEM: ");

    {
    SystemInfo si;

        SuperQuerySysInfo(SYSINFO_TAG_SYSTEM, &si, sizeof(si));
        switch (si.si_Mfgr)
        {
            case SYSINFO_MFGR_TOSHIBA     : printf("TOSHIBA");      break;
            case SYSINFO_MFGR_MEC         : printf("MEI");          break;
            case SYSINFO_MFGR_SAMSUNG     : printf("SAMSUNG");      break;
            case SYSINFO_MFGR_FUJITSU     : printf("FUJITSU");      break;
            case SYSINFO_MFGR_TI          : printf("TEXAS INSTR");  break;
            case SYSINFO_MFGR_ROHM        : printf("ROHM");         break;
            case SYSINFO_MFGR_CHIPEX      : printf("CHIP EXP");     break;
            case SYSINFO_MFGR_YAMAHA      : printf("YAMAHA");       break;
            case SYSINFO_MFGR_SANYO       : printf("SANYO");        break;
            case SYSINFO_MFGR_IBM         : printf("IBM");          break;
            case SYSINFO_MFGR_GOLDSTAR    : printf("GOLDSTAR");     break;
            case SYSINFO_MFGR_NEC         : printf("NEC");          break;
            case SYSINFO_MFGR_MOTOROLA    : printf("MOTOROLA");     break;
            case SYSINFO_MFGR_ATT         : printf("AT&T");         break;
            case SYSINFO_MFGR_VLSI        : printf("VLSI");         break;
            default                       : printf("MFGR<%x>", si.si_Mfgr);
        }

        printf(" ");
        switch (si.si_SysType)
        {
            case 0 : printf("M2DC"); break;
            case 1 : printf("M2TB"); break;
            case 2 : printf("M2BK"); break;
            case 3 : printf("M2");   break;
            default: printf("SYSTYPE<%x>", si.si_SysType);
        }

        busclk = si.si_BusClkSpeed;
        cpuclk = si.si_CPUClkSpeed;
    }

    {
    DispModeInfo di;

        if (SuperQuerySysInfo(SYSINFO_TAG_GRAFDISP, &di, sizeof(di)) == SYSINFO_SUCCESS)
        {
            printf(", ");
            if (di & SYSINFO_NTSC_CURDISP)
                printf("NTSC");
            else if (di & SYSINFO_PAL_CURDISP)
                printf("PAL");
            else
                printf("<unknown display type>");
        }
    }

    if (KB_FIELD(kb_NumCPUs) > 1)
        printf(", Dual-CPU");
    else
        printf(", Single CPU");

    printf("\nCHIPS : ");
    {
    BDAInfo bi;
    uint32  rev;

        DumpCPU(_mfpvr());
        if (KB_FIELD(kb_NumCPUs) > 1)
        {
            printf(" (master), ");
            DumpCPU(KB_FIELD(kb_SlaveState->ss_SlaveVersion));
            printf(" (slave)");

            if ((KB_FIELD(kb_SlaveState->ss_SlaveVersion) == 0x50200)
             || (_mfpvr() == 0x50200))
            {
                printf("\n\nERROR: MP board with 602 2.0 chips should be destroyed!!\n\n\n");
            }
        }

        if (SuperQuerySysInfo(SYSINFO_TAG_BDA, &bi, sizeof(bi)) == SYSINFO_SUCCESS)
        {
            rev = bi.bda_ID & 0xff;

            printf(", ");
            switch (rev)
            {
                case 0 : printf("BDA 1.0");  break;
                case 1 : printf("BDA 2.0/2.1");  break;
                default: printf("BDA<%x>", bi.bda_ID);
            }
        }
    }

    {
    CDEInfo ci;

        if (SuperQuerySysInfo(SYSINFO_TAG_CDE, &ci, sizeof(ci)) == SYSINFO_SUCCESS)
        {
            printf(", ");
            switch (ci.cde_ID & 0xff)
            {
                case 0 : printf("CDE 1.0");  break;
                case 1 : printf("CDE 2.0");  break;
                default: printf("CDE<%x>", ci.cde_ID);
            }
        }
    }

    printf("\nSPEED : CPU %u.%06u MHz, Bus %u.%06u MHz\n",cpuclk/1000000, cpuclk % 1000000, busclk/1000000, busclk % 1000000);
    printf("MEMORY: 0x%08x..0x%08x\n", KernelBase->kb_MemRegion->mr_MemBase, KernelBase->kb_MemRegion->mr_MemTop);

    if (KB_FIELD(kb_Flags) & KB_UNIQUEID)
        printf("ID    : 0x%08x%08x\n\n",KernelBase->kb_UniqueID[0], KernelBase->kb_UniqueID[1]);
    else
        printf("ID    : <not available>\n\n");
}
#endif
