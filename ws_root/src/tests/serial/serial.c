/* @(#) serial.c 95/12/22 1.5 */

/* generic serial driver test */

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/devicecmd.h>
#include <device/serial.h>
#include <string.h>
#include <stdio.h>


/*****************************************************************************/


static Item CreateSerialIOReq(void)
{
Item  device;
Item  ioreq;
List *list;
Err   result;

    result = CreateDeviceStackListVA(&list,
                "cmds", DDF_EQ, DDF_INT, 10,
		        CMD_STREAMREAD,
                        CMD_STREAMWRITE,
                        SER_CMD_STATUS,
                        SER_CMD_SETCONFIG,
                        SER_CMD_GETCONFIG,
                        SER_CMD_WAITEVENT,
                        SER_CMD_SETRTS,
                        SER_CMD_SETDTR,
                        SER_CMD_SETLOOPBACK,
                        SER_CMD_BREAK,
		NULL);
    if (result >= 0)
    {
        if (!IsEmptyList(list))
        {
            device = result = OpenDeviceStack((DeviceStack *) FirstNode(list));
            if (device >= 0)
            {
                ioreq = result = CreateIOReq(NULL,0,device,0);
                if (ioreq < 0)
                    CloseDeviceStack(device);
            }
        }
        else
        {
            result = MAKEKERR(ER_SEVER,ER_C_STND,ER_NoHardware);
        }
        DeleteDeviceStackList(list);
    }

    return result;
}


/*****************************************************************************/


static Err DeleteSerialIOReq(Item ioreq)
{
IOReq  *io;
Device *dev;
Err     result;

    io = (IOReq *)CheckItem(ioreq,KERNELNODE,IOREQNODE);
    if (!io)
        return BADITEM;

    dev    = io->io_Dev;
    result = DeleteIOReq(ioreq);
    CloseDeviceStack(dev->dev.n_Item);

    return result;
}


/*****************************************************************************/


char inputBuffer[8192];

int main(void)
{
Item       ioreq;
IOInfo     ioInfo;
Err        result;
SerConfig  sc;
uint32     i;

    ioreq = CreateSerialIOReq();
    if (ioreq >= 0)
    {
        printf("Serial device active!\n");

        sc.sc_BaudRate           = 57600;
        sc.sc_Handshake          = SER_HANDSHAKE_NONE;
        sc.sc_WordLength         = SER_WORDLENGTH_8;
        sc.sc_Parity             = SER_PARITY_NONE;
        sc.sc_StopBits           = SER_STOPBITS_1;
        sc.sc_OverflowBufferSize = 0;

        memset(&ioInfo, 0, sizeof(ioInfo));
        ioInfo.ioi_Command         = SER_CMD_SETCONFIG;
        ioInfo.ioi_Send.iob_Buffer = &sc;
        ioInfo.ioi_Send.iob_Len    = sizeof(sc);
        result = DoIO(ioreq, &ioInfo);
        if (result >= 0)
        {
            memset(&ioInfo,0,sizeof(ioInfo));
            ioInfo.ioi_Command         = CMD_STREAMWRITE;
            ioInfo.ioi_Send.iob_Buffer = "Hello!\n";
            ioInfo.ioi_Send.iob_Len    = 7;
            result = DoIO(ioreq, &ioInfo);
            if (result < 0)
            {
                printf("Writing failed: ");
                PrintfSysErr(result);
            }

            printf("Attempting to read serial data\n");
            memset(&ioInfo,0,sizeof(ioInfo));
            ioInfo.ioi_Command         = CMD_STREAMREAD;
            ioInfo.ioi_Recv.iob_Buffer = inputBuffer;
            ioInfo.ioi_Recv.iob_Len    = sizeof(inputBuffer);
            result = DoIO(ioreq, &ioInfo);
            if (result >= 0)
            {
                printf("Read returned success\n");
                printf("io_Actual %d\n",IOREQ(ioreq)->io_Actual);

                for (i = 0; i < IOREQ(ioreq)->io_Actual; i++)
                    printf("%c",inputBuffer[i]);
            }
            else
            {
                printf("Reading failed: ");
                PrintfSysErr(result);
            }
        }
        else
        {
            printf("Setting config failed: ");
            PrintfSysErr(result);
        }
        DeleteSerialIOReq(ioreq);
    }
    else
    {
        printf("Couldn't open serial device: ");
        PrintfSysErr(ioreq);
    }

    return 0;
}
