/* @(#) writemedia.c 96/04/29 1.3 */

/**
|||	AUTODOC -class Shell_Commands -name WriteMedia
|||	Writes new raw data to media.
|||
|||	  Format
|||
|||	    WriteMedia [-verify]
|||	               [-pattern <byte>]
|||	               [-filename <media image file name>]
|||	               <device name>
|||
|||	  Description
|||
|||	    This command writes raw data to a specified device. Either a
|||	    constant value is written, or a file is used and its contents are
|||	    written out directly to the media. Such a file would typically
|||	    contain a file system image.
|||
|||	  Arguments
|||
|||	    -verify
|||	        Specifies that a verification pass is to be performed. This
|||	        reads every byte of the card and makes sure it is set to the
|||	        correct byte previously written to that location.
|||
|||	    -pattern <byte>
|||	        Lets you specify the byte to use to fill the card. If this
|||	        option is not supplied, 0 is used. This option is ignored
|||	        if the -fileName option is used.
|||
|||	    -fileName <media image file name>
|||	        Lets you specify the filename of an media image file. This file
|||	        is written out directly to the target media. This option
|||	        overrides the -pattern option. If the given file is not large
|||	        enough to fill the whole media, the fill pattern will be used
|||	        to fill out the remainder of the media.
|||
|||	    <device name>
|||	        Lets you specify the name of the device that contains the card
|||	        to erase. This is typically "storagecard".
|||
|||	  Location
|||
|||	    System.m2/Programs/WriteMedia
|||
**/

#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/device.h>
#include <kernel/time.h>
#include <kernel/operror.h>
#include <file/fileio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*****************************************************************************/


static void PrintUsage(void)
{
    printf("writemedia - writes new raw data to media\n");
    printf("  <device Name>        - name of device containing card to erase\n");
    printf("  -verify              - requests that a verification pass be done\n");
    printf("  -pattern <byte>      - lets you specify the byte to use as fill pattern\n");
    printf("  -fileName <fileName> - lets you specify the file to use as source data\n");
}


/*****************************************************************************/


int main(int argc, char *argv[])
{
DeviceStatus ds;
Item         dev;
Item         ior;
IOInfo       ioInfo;
IOInfo       vioInfo;
Err          result;
uint32       i;
int32        parm;
char        *devName;
char        *fileName;
uint8        pattern;
bool         verify;
TimeVal      startTime;
TimeVal      endTime;
TimeVal      tv;
RawFile     *file;
uint8       *buffer;
uint8       *vbuffer;
uint32       bufferSize;
int32        numBlocks;
int32        offset;
int32        numBytes;

    devName  = NULL;
    fileName = NULL;
    verify   = FALSE;
    pattern  = 0;

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

        if (strcasecmp(argv[parm],"-verify") == 0)
        {
            verify = TRUE;
        }
        else if (strcasecmp(argv[parm],"-pattern") == 0)
        {
            parm++;
            if (parm == argc)
            {
                printf("No pattern byte given for '-pattern' option\n");
                return 1;
            }
            pattern = strtoul(argv[parm], NULL, 0);
        }
        else if (strcasecmp(argv[parm],"-fileName") == 0)
        {
            parm++;
            if (parm == argc)
            {
                printf("No filename given for '-fileName' option\n");
                return 1;
            }
            fileName = argv[parm];
        }
        else
        {
            if (devName)
            {
                printf("Only one device name allowed\n");
                return 1;
            }
            devName = argv[parm];
        }
    }

    if (devName == NULL)
    {
        printf("No device name given\n");
        return 1;
    }

    file   = NULL;
    result = 0;
    if (fileName)
        result = OpenRawFile(&file, fileName, FILEOPEN_READ);

    if (result >= 0)
    {
        dev = result = OpenNamedDeviceStack(devName);
        if (dev >= 0)
        {
            ior = result = CreateIOReq(NULL, 0, dev, 0);
            if (ior >= 0)
            {
                memset(&ioInfo, 0, sizeof(ioInfo));
                ioInfo.ioi_Command         = CMD_STATUS;
                ioInfo.ioi_Recv.iob_Buffer = &ds;
                ioInfo.ioi_Recv.iob_Len    = sizeof(ds);

                result = DoIO(ior, &ioInfo);
                if (result >= 0)
                {
                    numBlocks  = ds.ds_DeviceBlockCount;
                    bufferSize = numBlocks * ds.ds_DeviceBlockSize;
                    buffer     = AllocMem(bufferSize*2, MEMTYPE_FILL | pattern);
                    while (!buffer && numBlocks)
                    {
                        numBlocks--;
                        bufferSize = numBlocks * ds.ds_DeviceBlockSize;
                        buffer     = AllocMem(bufferSize*2, MEMTYPE_FILL | pattern);
                    }

                    if (buffer)
                    {
                        vbuffer = &buffer[bufferSize];

                        memset(&ioInfo, 0, sizeof(ioInfo));
                        ioInfo.ioi_Command         = CMD_BLOCKWRITE;
                        ioInfo.ioi_Send.iob_Buffer = buffer;
                        ioInfo.ioi_Send.iob_Len    = bufferSize;

                        memset(&vioInfo, 0, sizeof(vioInfo));
                        vioInfo.ioi_Command         = CMD_BLOCKREAD;
                        vioInfo.ioi_Recv.iob_Buffer = vbuffer;
                        vioInfo.ioi_Recv.iob_Len    = bufferSize;

                        if (file)
                            printf("Writing file '%s' to media in device %s ", fileName, devName);
                        else
                            printf("Setting all bytes to 0x%02x on media in device %s ", pattern, devName);
                        printf("(%d blocks of %d bytes each)\n", ds.ds_DeviceBlockCount, ds.ds_DeviceBlockSize);

                        SampleSystemTimeTV(&startTime);

                        offset = ds.ds_DeviceBlockStart;
                        while (offset < ds.ds_DeviceBlockStart + ds.ds_DeviceBlockCount)
                        {
                            if (file)
                            {
                                numBytes = result = ReadRawFile(file, buffer, bufferSize);
                                if (numBytes < 0)
                                {
                                    printf("Error reading from file %s: ", fileName);
                                    PrintfSysErr(result);
                                    break;
                                }

                                if (numBytes < bufferSize)
                                    memset(&buffer[numBytes], pattern, bufferSize - numBytes);
                            }

                            if (offset + numBlocks > ds.ds_DeviceBlockCount)
                            {
                                ioInfo.ioi_Send.iob_Len  = (ds.ds_DeviceBlockCount - offset) * ds.ds_DeviceBlockSize;
                                vioInfo.ioi_Recv.iob_Len = (ds.ds_DeviceBlockCount - offset) * ds.ds_DeviceBlockSize;
                            }

                            ioInfo.ioi_Offset  = offset;
                            vioInfo.ioi_Offset = offset;
                            offset            += numBlocks;

                            result = DoIO(ior, &ioInfo);
                            if (result < 0)
                            {
                                printf("CMD_BLOCKWRITE of %d bytes at offset %d failed: ", ioInfo.ioi_Send.iob_Len, ioInfo.ioi_Offset);
                                PrintfSysErr(result);
                                break;
                            }

                            if (verify)
                            {
                                result = DoIO(ior, &vioInfo);
                                if (result < 0)
                                {
                                    printf("CMD_BLOCKREAD of %d bytes at offset %d failed: ", vioInfo.ioi_Recv.iob_Len, vioInfo.ioi_Offset);
                                    PrintfSysErr(result);
                                    break;
                                }

                                for (i = 0; i < bufferSize; i++)
                                {
                                    if (buffer[i] != vbuffer[i])
                                    {
                                        printf("Verify pass failed at offset %d\n", ioInfo.ioi_Offset + i);
                                        printf("Expecting 0x%02x, got 0x%02x\n", buffer[i], vbuffer[i]);
                                        result = -1;
                                        break;
                                    }
                                }

                                if (result < 0)
                                    break;
                            }
                        }

                        if (result >= 0)
                        {
                            SampleSystemTimeTV(&endTime);
                            SubTimes(&startTime, &endTime, &tv);
                            printf("Operation successfully completed in %d.%06d seconds\n", tv.tv_Seconds, tv.tv_Microseconds);

                            result = 0;
                        }
                        FreeMem(buffer, bufferSize*2);
                    }
                    else
                    {
                        printf("Unable to allocate %d bytes for I/O buffer\n", bufferSize);
                        result = NOMEM;
                    }
                }
                else
                {
                    printf("CMD_STATUS failed: ");
                    PrintfSysErr(result);
                }
                DeleteIOReq(ior);
            }
            else
            {
                printf("CreateIOReq() failed: ");
                PrintfSysErr(result);
            }
            CloseDeviceStack(dev);
        }
        else
        {
            printf("Unable to open device %s: ", devName);
            PrintfSysErr(result);
        }
        CloseRawFile(file);
    }
    else
    {
        printf("Unable to open file %s: ", fileName);
        PrintfSysErr(result);
    }

    return result;
}
