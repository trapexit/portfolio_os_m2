
/******************************************************************************
**
**  @(#) type.c 96/02/26 1.13
**
******************************************************************************/

/**
|||	AUTODOC -class Shell_Commands -name Type
|||	Types a file's content to the output terminal.
|||
|||	  Synopsis
|||
|||	    Type -hex {file}
|||
|||	  Description
|||
|||	    Reads a file and send its contents to the Debugger Terminal window.
|||
|||	  Arguments
|||
|||	    -hex
|||	        Display the contents of the file in hexadecimal form.
|||
|||	    {file}
|||	        Names of the files to type. You can specify an arbitrary number
|||	        of file names, and they will all get typed out.
|||
|||	  Location
|||
|||	    System.m2/Programs/Type
|||
**/

/**
|||	AUTODOC -class Examples -name Type
|||	Types a file's content to the output terminal.
|||
|||	  Synopsis
|||
|||	    Type -hex {file}
|||
|||	  Description
|||
|||	    Demonstrates how to read a file and send its contents to the Debugger
|||	    Terminal window using the byte stream routines.
|||
|||	  Arguments
|||
|||	    -hex
|||	        Display the contents of the file in hexadecimal.
|||
|||	    {file}
|||	        Names of the files to type. You can specify an arbitrary number of
|||	        file names, and they will all get typed out.
|||
|||	  Associated Files
|||
|||	    type.c
|||
|||	  Location
|||
|||	    Examples/FileSystem
|||
**/

#include <kernel/types.h>
#include <kernel/operror.h>
#include <file/fileio.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>


/*****************************************************************************/


#define BUFFER_SIZE 1024

static void Type(const char *path, bool hex)
{
RawFile *file;
char     buffer[BUFFER_SIZE];
int32    numBytes;
uint32   numLines;
Err      err;
uint32   i;
uint32   index;
uint32   mod;
char     hexLine[100];
char     temp[10];
uint32   start;

    /* Open the file as a byte-stream.  A buffer size of zero is
     * specified, which permits the file folio to choose an appropriate
     * amount of buffer space based on the file's actual block size.
     */

    err = OpenRawFile(&file, path, FILEOPEN_READ);
    if (err < 0)
    {
        printf("File '%s' could not be opened: ", path);
        PrintfSysErr(err);
        return;
    }

    hexLine[0] = 0;
    numLines   = 0;
    index      = 0;
    start      = 0;
    while (TRUE)
    {
        numBytes = ReadRawFile(file, buffer, BUFFER_SIZE);
        if (numBytes < 0)
        {
            printf("Error reading '%s': ",path);
            PrintfSysErr(numBytes);
            break;
        }
        else if (numBytes == 0)
        {
            break;
        }

        if (hex)
        {
            for (i = 0; i < numBytes; i++)
            {
                mod = index % 16;
                if (mod == 0)
                {
                    if (index)
                        printf("%s",hexLine);

                    sprintf(hexLine,"%04x: ", index);
                    start = strlen(hexLine);
                    strcat(hexLine,"                                                       \n");
                }

                sprintf(temp,"%02x",buffer[i]);
                hexLine[start + mod*2 + (mod / 4)] = temp[0];
                hexLine[start + mod*2 + (mod / 4) + 1] = temp[1];

                if (isgraph(buffer[i]))
                    hexLine[start + mod + 39] = buffer[i];
                else
                    hexLine[start + mod + 39] = '.';

                index++;
            }
        }
        else
        {
            for (i = 0; i < numBytes; i++)
            {
                if (buffer[i] == '\r')
                    buffer[i] = '\n';

                if (buffer[i] == '\n')
                    numLines++;
            }

            printf("%*.*s",numBytes,numBytes,buffer);
        }
    }

    if (hex)
    {
        if (numBytes >= 0)
            printf("%s",hexLine);
    }
    else
    {
        if (numBytes >= 0)
            printf("\n%u lines processed\n\n", numLines);
    }

    /* close the file */
    CloseRawFile(file);
}


/*****************************************************************************/


int main(int32 argc, char **argv)
{
int32 i;
bool  hex;

    if (argc < 2)
    {
        printf("Usage: type -hex <file1> [file2] [file3] [...]\n");
    }
    else
    {
        hex = FALSE;
        for (i = 1; i < argc; i++)
        {
            if (strcasecmp(argv[i], "-hex") == 0)
            {
                hex = TRUE;
            }
            else if (argv[i][0] == '-')
            {
                printf("Ignoring unknown option '%s'\n",argv[i]);
            }
        }

        for (i = 1; i < argc; i++)
        {
            if (argv[i][0] != '-')
                Type(argv[i],hex);
        }
    }

    return 0;
}
