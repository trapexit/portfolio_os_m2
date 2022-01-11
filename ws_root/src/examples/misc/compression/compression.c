
/******************************************************************************
**
**  @(#) compression.c 95/09/30 1.5
**
******************************************************************************/

/**
|||	AUTODOC -public -class examples -name compression
|||	Demonstrates use of the compression folio.
|||
|||	  Synopsis
|||
|||	    compression
|||
|||	  Description
|||
|||	    Simple program demonstrating how to use the compression routines supplied
|||	    by compression folio. The program loads itself into a memory buffer,
|||	    compresses the data, decompresses it, and compares the original data with
|||	    the decompressed data to make sure the compression and decompression
|||	    processes worked successfully.
|||
|||	  Associated Files
|||
|||	    compression.c
|||
|||	  Location
|||
|||	    examples/Miscellaneous/Compression
|||
**/

#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/time.h>
#include <file/fileio.h>
#include <misc/compression.h>
#include <stdio.h>
#include <stdlib.h>


/*****************************************************************************/


int main(int32 argc, char **argv)
{
RawFile   *file;
FileInfo   info;
Err        err;
bool       same;
uint32     i;
int32      fileSize;
uint32    *originalData;
uint32    *compressedData;
uint32    *finalData;
int32      numFinalWords;
int32      numCompWords;
TimerTicks compStartTime, compEndTime, compTotalTime;
TimerTicks decompStartTime, decompEndTime, decompTotalTime;
TimeVal    compTV, decompTV;

    TOUCH(argc);

    err = OpenCompressionFolio();
    if (err >= 0)
    {
        err = OpenRawFile(&file,argv[0],FILEOPEN_READ);
        if (err >= 0)
        {
            GetRawFileInfo(file,&info,sizeof(info));
            fileSize       = info.fi_ByteCount & 0xfffffffc;
            originalData   = (uint32 *)malloc(fileSize);
            compressedData = (uint32 *)malloc(fileSize);
            finalData      = (uint32 *)malloc(fileSize);

            if (originalData && compressedData && finalData)
            {
                if (ReadRawFile(file, originalData, fileSize) == fileSize)
                {
                    SampleSystemTimeTT(&compStartTime);

                    err = SimpleCompress(originalData, fileSize / sizeof(uint32),
                                         compressedData, fileSize / sizeof(uint32));

                    SampleSystemTimeTT(&compEndTime);

                    if (err >= 0)
                    {
                        numCompWords = err;

                        SampleSystemTimeTT(&decompStartTime);

                        err = SimpleDecompress(compressedData, numCompWords,
                                               finalData, fileSize / sizeof(uint32));

                        SampleSystemTimeTT(&decompEndTime);

                        if (err >= 0)
                        {
                            SubTimerTicks(&compStartTime,&compEndTime,&compTotalTime);
                            SubTimerTicks(&decompStartTime,&decompEndTime,&decompTotalTime);
                            ConvertTimerTicksToTimeVal(&compTotalTime,&compTV);
                            ConvertTimerTicksToTimeVal(&decompTotalTime,&decompTV);
                            numFinalWords = err;

                            printf("Original data size    : %d words\n",fileSize / sizeof(uint32));
                            printf("Compressed data size  : %d words\n",numCompWords);
                            printf("Uncompressed data size: %d words\n",numFinalWords);
                            printf("Compression Time      : %d.%06d\n",compTV.tv_Seconds,compTV.tv_Microseconds);
                            printf("Decompression Time    : %d.%06d\n",decompTV.tv_Seconds,decompTV.tv_Microseconds);

                            same = TRUE;
                            for (i = 0; i < fileSize / sizeof(uint32); i++)
                            {
                                if (originalData[i] != finalData[i])
                                {
                                    same = FALSE;
                                    break;
                                }
                            }

                            if (same)
                            {
                                printf("Uncompressed data matched original\n");
                            }
                            else
                            {
                                printf("Uncompressed data differed with original!\n");
                                for (i = 0; i < 10; i++)
                                {
                                    printf("orig $%08x, final $%08x, comp $%08x\n",
                                           originalData[i],
                                           finalData[i],
                                           compressedData[i]);
                                }
                            }
                        }
                        else
                        {
                            printf("SimpleDecompress() failed: ");
                            PrintfSysErr(err);
                        }
                    }
                    else
                    {
                        printf("SimpleCompress() failed: ");
                        PrintfSysErr(err);
                    }
                }
                else
                {
                    printf("Could not read whole file\n");
                }
            }
            else
            {
                printf("Could not allocate memory buffers\n");
            }

            free(originalData);
            free(compressedData);
            free(finalData);

            CloseRawFile(file);
        }
        else
        {
            printf("Could not open '%s' as an input file: ",argv[0]);
            PrintfSysErr(err);
        }
        CloseCompressionFolio();
    }
    else
    {
        printf("OpenCompressionFolio() failed: ");
        PrintfSysErr(err);
    }

    return (0);
}
