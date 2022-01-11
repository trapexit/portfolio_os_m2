
/******************************************************************************
**
**  @(#) walker.c 96/02/26 1.12
**
******************************************************************************/

/**
|||	AUTODOC -class Shell_Commands -name Walker
|||	Recursively displays the contents of a directory, and all nested
|||	directories.
|||
|||	  Format
|||
|||	    Walker {directory}
|||
|||	  Description
|||
|||	    This program demonstrates how to scan a directory, and recursively scan
|||	    any directories it contains.
|||
|||	  Arguments
|||
|||	    {directory}
|||	        Names of the directories to list. You can specify an arbitrary
|||	        number of directory names, and they will all get listed. If no
|||	        name is specified, then the current directory gets displayed.
|||
|||	  Location
|||
|||	    System.m2/Programs/Walker
|||
**/

/**
|||	AUTODOC -class Examples -name Walker
|||	Recursively displays the contents of a directory, and all nested
|||	directories.
|||
|||	  Synopsis
|||
|||	    Walker {directory}
|||
|||	  Description
|||
|||	    This program demonstrates how to scan a directory, and recursively scan
|||	    any directories it contains.
|||
|||	  Arguments
|||
|||	    {directory}
|||	        Names of the directories to list. You can specify an arbitrary
|||	        number of directory names, and they will all get listed. If no
|||	        name is specified, then the current directory in which the ls
|||	        program is located will get displayed.
|||
|||	  Associated Files
|||
|||	    walker.c
|||
|||	  Location
|||
|||	    Examples/FileSystem
|||
**/

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/operror.h>
#include <file/filesystem.h>
#include <file/directory.h>
#include <file/directoryfunctions.h>
#include <file/filefunctions.h>
#include <stdio.h>


/*****************************************************************************/


static void TraverseDirectory(Item dirItem)
{
Directory      *dir;
DirectoryEntry  de;
Item            subDirItem;
int32           entry;

    dir = OpenDirectoryItem(dirItem);
    if (dir)
    {
        entry = 1;
        while (ReadDirectory(dir, &de) >= 0)
        {
            printf("Entry #%d:\n", entry);
            printf("  file '%s', type 0x%lx, ID 0x%lx",
                    de.de_FileName, de.de_Type,
                    de.de_UniqueIdentifier);
            printf(", flags 0x%lx\n", de.de_Flags);
            printf("  %d bytes, %d block(s) of %d byte(s) each\n",
                    de.de_ByteCount,
                    de.de_BlockCount, de.de_BlockSize);
            printf("  %d avatar(s)", de.de_AvatarCount);
            printf("\n\n");

            if (de.de_Flags & FILE_IS_DIRECTORY)
            {
                subDirItem = OpenFileInDir(dirItem, de.de_FileName);
                if (subDirItem >= 0)
                {
                    printf("********** RECURSE **********\n\n");
                    TraverseDirectory(subDirItem);
                    printf("******* END RECURSION *******\n\n");
                }
                else
                {
                    printf("***  RECURSION FAILED  ***\n\n");
                }
            }
            entry++;
        }
        CloseDirectory(dir);
    }
    else
    {
        printf("OpenDirectoryItem() failed\n");
        CloseFile(dirItem);
    }
}


/*****************************************************************************/


static Err Walk(const char *path)
{
Item startItem;

    startItem = OpenFile((char *)path);
    if (startItem >= 0)
    {
        printf("\nRecursive directory scan from %s\n\n", path);
        TraverseDirectory(startItem);
        printf("End of %s has been reached\n\n", path);
        return 0;
    }
    else
    {
        printf("OpenFile(\"%s\") failed: ",path);
        PrintfSysErr(startItem);
        return startItem;
    }
}


/*****************************************************************************/


int main(int32 argc, char **argv)
{
int32 i;

    if (argc <= 1)
    {
	/* if no directory name was given, scan the current directory */
	Walk(".");
    }
    else
    {
	for (i = 1; i < argc; i++)
	    Walk(argv[i]);
    }

    return 0;
}
