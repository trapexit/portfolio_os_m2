/* @(#) DefaultDriver.c 96/07/08 1.20 */

/* Control Port driverlet code for devices which
 * don't have a built-in driverlet.  Includes code to load real
 * driverlet from file system.
 */

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <kernel/nodes.h>
#include <kernel/list.h>
#include <kernel/kernelnodes.h>
#include <kernel/io.h>
#include <kernel/operror.h>
#include <loader/loader3do.h>
#include <misc/event.h>
#include <misc/poddriver.h>
#include <file/filefunctions.h>
#include <stdio.h>
#include <string.h>


extern List podDrivers;


#ifdef DEBUG
# define DBUG(x)  printf x
#else
# define DBUG(x) /* x */
#endif


static const TagArg searchTags[] =
{
    FILESEARCH_TAG_SEARCH_FILESYSTEMS,  (TagData) DONT_SEARCH_UNBLESSED,
    TAG_END
};

Err DefaultDriver(PodInterface *interfaceStruct)
{
Pod          *pod;
PodInterface  pi;
PodDriver    *newDriver;
char          driverPath[64];
char          foundPath[96];
Item          mod;
Err           result;

    result = 0;
    pod    = interfaceStruct->pi_Pod;

    switch (interfaceStruct->pi_Command)
    {
        case PD_InitDriver:
        {
            DBUG(("Default driverlet init\n"));
            break;
        }

        case PD_InitPod:
        {
            DBUG(("Default driver pod init\n"));
            pod->pod_Flags    = 0;
            pod->pod_Blipvert = TRUE;

            sprintf(driverPath, "System.m2/EventBroker/driverlet_%x.eb", pod->pod_Type);

            result = FindFileAndIdentify(foundPath, sizeof(foundPath), driverPath, searchTags);
            if (result < 0)
            {
                DBUG(("Error finding driverlet '%s': ", driverPath));
                return result;
            }

            newDriver = AllocMem(sizeof(PodDriver), MEMTYPE_FILL);
            if (newDriver == NULL)
            {
                DBUG(("Unable to allocate PodDriver structure\n"));
                return MAKEEB(ER_SEVERE,ER_C_STND,ER_NoMem);
            }

            DBUG(("Trying to load driverlet '%s'\n", foundPath));

            mod = OpenModule(foundPath, OPENMODULE_FOR_THREAD, NULL);
            if (mod >= 0)
            {
                newDriver->pd_DeviceType = pod->pod_Type;
                newDriver->pd_UseCount   = 1;
                newDriver->pd_Flags      = PD_LOADED_INTO_RAM | PD_LOADED_FROM_FILE;
                newDriver->pd_Module     = mod;

                result = ExecuteModule(mod, (uint32) newDriver, NULL);
                if (result >= 0)
                {
                    pi            = *interfaceStruct;
                    pi.pi_Command = PD_InitDriver;
                    result = (*newDriver->pd_DriverEntry)(&pi);
                    if (result >= 0)
                    {
                        AddHead(&podDrivers, (Node *) newDriver);
                        pod->pod_Driver = newDriver;
                        return (*newDriver->pd_DriverEntry)(interfaceStruct);
                    }
                    DBUG(("Error initializing driverlet '%s': ", driverPath));
                }
                else
                {
                    DBUG(("Error booting driverlet '%s': ", driverPath));
                }
                CloseModule(mod);
            }
            else
            {
                DBUG(("Error loading driverlet '%s': ", driverPath));
            }
            FreeMem(newDriver, sizeof(PodDriver));
            break;
        }

        case PD_ProcessCommand:
        {
            interfaceStruct->pi_CommandOutLen = 0;
            break;
        }

        case PD_ReconnectPod:
        case PD_ParsePodInput:
        case PD_ConstructPodOutput:
        case PD_TeardownPod:
        default:
        {
            break;
        }
    }

    return result;
}
