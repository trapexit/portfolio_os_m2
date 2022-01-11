/* @(#) opentedevice.c 96/10/03 1.5 */

#include <kernel/types.h>
#include <kernel/device.h>
#include <kernel/devicecmd.h>
#include <device/te.h>


/*****************************************************************************/


Item OpenTEDevice(void)
{
List *list;
Err   result;

    /* Find devices that support TE commands */
    result = CreateDeviceStackListVA(&list,"cmds", DDF_EQ, DDF_INT, 8,
                                     TE_CMD_EXECUTELIST,
                                     TE_CMD_SETVIEW,
                                     TE_CMD_SETZBUFFER,
                                     TE_CMD_SETFRAMEBUFFER,
                                     TE_CMD_DISPLAYFRAMEBUFFER,
                                     TE_CMD_SPEEDCONTROL,
                                     TE_CMD_STEP,
                                     TE_CMD_SETVBLABORTCOUNT,
                                     NULL);
    if (result >= 0)
    {
        if (!IsEmptyList(list))
        {
            /* Just take the first one. */
            result = OpenDeviceStack((DeviceStack *)FirstNode(list));
        }
        else
        {
            result = NOSUPPORT;
        }
        DeleteDeviceStackList(list);
    }

    return result;
}
