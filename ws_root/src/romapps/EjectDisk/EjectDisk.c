/* @(#) EjectDisk.c 96/06/24 1.3 */

/*
    File:       EjectDisk.c

    Contains:   Used to "eject" a CD from the M2 cd-rom driver/device.

    Copyright:  ©1996 by The 3DO Company, all rights reserved.
*/


#include <kernel/driver.h>
#include <kernel/mem.h>
#include <kernel/task.h>
#include <kernel/debug.h>
#include <kernel/random.h>
#include <kernel/operror.h>
#include <device/m2cd.h>
#include <misc/event.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int32 argc,char *argv[])
{
    Item    devItem, iorItem;
    IOInfo  ioInfo;

    TOUCH(argc);
    TOUCH(argv);
    
    memset(&ioInfo,0,sizeof(ioInfo));

    devItem = OpenRomAppMedia();
    iorItem = CreateIOReq(0, 0, devItem, 0);
    ioInfo.ioi_Command = CDROMCMD_OPEN_DRAWER;
    DoIO(iorItem, &ioInfo);

    DeleteItem(iorItem);
    CloseDeviceStack(devItem);
        
    return (0);
}

