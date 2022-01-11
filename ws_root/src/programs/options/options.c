/* @(#) options.c 96/08/23 1.27 */

/**
|||	AUTODOC -class Shell_Commands -name Options
|||	Controls various system run-time options.
|||
|||	  Format
|||
|||	    Options [-icache <on|off>]
|||	            [-dcache <on|off>]
|||	            [-writethru <on|off>]
|||	            [-printf <on|off>]
|||	            [-prefetch <on|off>]
|||	            [-ldebug <on|off>]
|||	            [-memdebug <on|off>]
|||	            [-iodebug <on|off>]
|||
|||	  Desription
|||
|||	    This command lets you adjust system options. When run with no
|||	    argument, it reports the current state of some of the options
|||	    it controls.
|||
|||	  Arguments
|||
|||	    -icache <on|off>
|||	        Turn the CPU instruction cache on or off.
|||
|||	    -dcache <on|off>
|||	        Turn the CPU data cache on or off
|||
|||	    -writethru <on|off>
|||	        Turn the CPU data cache write-through mode on or off.
|||
|||	    -printf <on|off>
|||	        Turn debugging output on or off.
|||
|||	    -prefetch <on|off>
|||	        Turn CPU prefetch on or off.
|||
|||	    -ldebug <on|off>
|||	        Turn launch debugging on or off.
|||
|||	    -memdebug <on|off>
|||	        Provides an command line interface to CreateMemDebug
|||
|||	    -iodebug <on|off>
|||	        Turn I/O debugging on or off.
|||
|||	  Implementation
|||
|||	    Command implemented in V27.
|||
|||	  Location
|||
|||	    System.m2/Programs/options
|||
**/

#include <kernel/types.h>
#include <kernel/cache.h>
#include <kernel/kernel.h>
#include <kernel/super.h>
#include <kernel/io.h>
#include <kernel/debug.h>
#include <kernel/mem.h>
#include <hardware/bda.h>
#include <hardware/PPCasm.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


/*****************************************************************************/


static void PrintState(CacheInfo *ci)
{
    printf("Instruction cache: %s\n", (ci->cinfo_Flags & CACHE_INSTR_ENABLED) ? "ON" : "OFF");
    printf("Data cache       : %s\n", (ci->cinfo_Flags & CACHE_DATA_ENABLED) ? "ON" : "OFF");
    printf("Write Through    : %s\n", (ci->cinfo_Flags & CACHE_DATA_WRITETHROUGH) ? "ON" : "OFF");
    printf("BDA Prefetching  : %s\n", (BDA_READ(BDAPCTL_PBCONTROL) & BDAPCTL_PREF_MASK) ? "ON" : "OFF");
    printf("Launch Debugging : %s\n", (KernelBase->kb_ShowTaskMessages ? "ON" : "OFF"));
    printf("Memory Debugging : %s\n", (KernelBase->kb_Flags & KB_MEMDEBUG ? "ON" : "OFF"));
    printf("IO Debugging     : %s\n", (KernelBase->kb_Flags & KB_IODEBUG ? "ON" : "OFF"));
}


/*****************************************************************************/


static void PrintControl(bool state)
{
    if (state)
        printf("Printing is now off%c\n", KPRINTF_STOP);
    else
        printf("%c\nPrinting is now on\n", KPRINTF_START);
}


/*****************************************************************************/


static void ICacheControl(bool state)
{
    if (state)
    {
        ControlCaches(CACHEC_INSTR_ENABLE);
        printf("Instruction cache is now on\n");
    }
    else
    {
        ControlCaches(CACHEC_INSTR_DISABLE);
        printf("Instruction cache is now off\n");
    }
}


/*****************************************************************************/


static void DCacheControl(bool state)
{
    if (state)
    {
        ControlCaches(CACHEC_DATA_ENABLE);
        printf("Data cache is now on\n");
    }
    else
    {
        ControlCaches(CACHEC_DATA_DISABLE);
        printf("Data cache is now off\n");
    }
}


/*****************************************************************************/


static void WriteThroughControl(bool state)
{
    if (state)
    {
        ControlCaches(CACHEC_DATA_WRITETHROUGH);
        printf("Write through is now on\n");
    }
    else
    {
        ControlCaches(CACHEC_DATA_COPYBACK);
        printf("Write through is now off\n");
    }
}


/*****************************************************************************/


static Err SuperPrefetchControl(bool state)
{
uint32 pbctl;

    pbctl = BDA_READ(BDAPCTL_PBCONTROL);

    if (state)
    {
        pbctl |= BDAPCTL_PREF_MASK; /* set the prefetch bit */
        printf("Prefetch is now on\n");
    }
    else
    {
        pbctl &= ~BDAPCTL_PREF_MASK; /* turn off the prefetch bit */
        printf("Prefetch is now off\n");
    }

    BDA_WRITE(BDAPCTL_PBCONTROL,pbctl);

    return 0;
}


/*****************************************************************************/


static void PrefetchControl(bool state)
{
    CallBackSuper(SuperPrefetchControl, state, 0, 0);
}


/*****************************************************************************/


static void MemDebugControl(bool state)
{
Err result;

    if (!state)
    {
	result = DeleteMemDebug();
	if (result < 0)
	{
	    printf("Couldn't disable memory debugging: ");
	    PrintfSysErr(result);
	}
	else
	{
	    printf("Memory debugging is now OFF.\n");
	}
    }
    else
    {
	result = CreateMemDebug(NULL);
	if (result >= 0)
	{
	    result = ControlMemDebug(MEMDEBUGF_ALLOC_PATTERNS |
                                     MEMDEBUGF_FREE_PATTERNS |
                                     MEMDEBUGF_PAD_COOKIES |
                                     MEMDEBUGF_CHECK_ALLOC_FAILURES);

            if (result < 0)
            {
                printf("Couldn't enable memory debugging: ");
                PrintfSysErr(result);
            }
            else
            {
                printf("Memory debugging is now ON.\n");
            }
	}
    }
}


/*****************************************************************************/


static void IODebugControl(bool state)
{
    if (!state)
    {
        ControlIODebug(0);
        printf("I/O debugging is now OFF.\n");
    }
    else
    {
        ControlIODebug(IODEBUGF_PREVENT_PREREAD | IODEBUGF_PREVENT_POSTWRITE);
        printf("I/O debugging is now ON.\n");
    }
}


/*****************************************************************************/


static Err SuperLDebugControl(bool state)
{
    KernelBase->kb_ShowTaskMessages = (state ? 1 : 0);
    printf("Task launch debugging is now %s.\n", KernelBase->kb_ShowTaskMessages ? "ON" : "OFF");
    return 0;
}


/*****************************************************************************/


static void LDebugControl(bool state)
{
    CallBackSuper(SuperLDebugControl, state, 0, 0);
}


/*****************************************************************************/


static void PrintUsage(void)
{
    printf("options - controls various system run-time options\n"
           "  -icache <on|off>    - turn the instruction cache on/off\n"
           "  -dcache <on|off>    - turn the data cache on/off\n"
           "  -writethru <on|off> - turn the data cache write-through mode on/off\n"
           "  -printf <on|off>    - turn printing on/off\n"
           "  -prefetch <on|off>  - turn prefetch on/off\n"
           "  -ldebug <on|off>    - turn launch debugging on/off\n"
           "  -memdebug <on|off>  - turn memory debugging on/off\n"
           "  -iodebug <on|off>   - turn I/O debugging on/off\n");
}


/*****************************************************************************/



typedef void (* BoolFunc)(bool state);

typedef struct BoolOpt
{
    char     *bo_Name;
    BoolFunc  bo_Func;
} BoolOpt;

static const BoolOpt boolOpts[] =
{
    {"-icache",       ICacheControl},
    {"-dcache",       DCacheControl},
    {"-writethru",    WriteThroughControl},
    {"-writethrough", WriteThroughControl},  /* to be nice */
    {"-prefetch",     PrefetchControl},
    {"-print",        PrintControl},
    {"-ldebug",	      LDebugControl},
    {"-memdebug",     MemDebugControl},
    {"-iodebug",     IODebugControl},
    {NULL,           NULL}
};


/*****************************************************************************/


int main(int32 argc, char **argv)
{
int32     parm;
uint32    i;
CacheInfo ci;

    if (argc == 1)
    {
        /* run GetCacheInfo() in user-mode so it won't fail */
        GetCacheInfo(&ci, sizeof(ci));
        CallBackSuper((CallBackProcPtr)PrintState, (uint32)&ci, 0, 0);
    }
    else
    {
        for (parm = 1; parm < argc; parm++)
        {
            if ((strcasecmp("-help",argv[parm]) == 0)
             || (strcasecmp("-?",argv[parm]) == 0)
             || (strcasecmp("help",argv[parm]) == 0)
             || (strcasecmp("?",argv[parm]) == 0))
            {
                PrintUsage();
                return (0);
            }

            i = 0;
            while (boolOpts[i].bo_Name)
            {
                if (strcasecmp(boolOpts[i].bo_Name, argv[parm]) == 0)
                {
                    parm++;
                    if (parm < argc)
                    {
                        if (strcasecmp(argv[parm],"on") == 0)
                        {
                            (* boolOpts[i].bo_Func)(TRUE);
                            break;
                        }

                        if (strcasecmp(argv[parm],"off") == 0)
                        {
                            (* boolOpts[i].bo_Func)(FALSE);
                            break;
                        }
                    }

                    printf("No ON or OFF switch for the '%s' option\n",argv[parm-1]);
                    return 1;
                }
                i++;
            }

            if (boolOpts[i].bo_Name == NULL)
            {
                printf("'%s' is not a valid option\n",argv[parm]);
                return 1;
            }
        }
    }

    return 0;
}
