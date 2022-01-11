/* @(#) restable.c 96/11/19 1.1 */

#include <kernel/task.h>
#include <kernel/operror.h>
#include <stdio.h>


/*****************************************************************************/


void main(void)
{
Err result;

    result = IncreaseResourceTable(200);
    if (result >= 0)
    {
        printf("IncreaseResourceTable() worked: ");
        printf("now has %d resource slots\n", CURRENTTASK->t_ResourceCnt);
    }
    else
    {
        printf("IncreaseResourceTable() failed: ");
        PrintfSysErr(result);
    }

    WaitSignal(0);
}
