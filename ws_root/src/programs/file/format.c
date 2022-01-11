/* @(#) format.c 96/09/04 1.1 */

/**
|||	AUTODOC -class Shell_Commands -name Format
|||	Formats media as a file system.
|||
|||	  Format
|||
|||	    Format [-fsType <filesystem type>]
|||	           <device Name>
|||	           <volume Name>
|||
|||	  Description
|||
|||	    This command does the work needed to prepare new media for use
|||	    as a filesystem. Any data existing on the media is erased and
|||	    replaced by the new empty filesystem structure.
|||
|||	  Arguments
|||
|||	    -fsType <filesystem type>
|||	        Lets you specify the filesystem to use on the media.
|||	        This can be "opera.fs", "acrobat.fs", or "host.fs". If
|||	        This argument is not supplied, the preferred file system
|||	        for the given media will be used.
|||
|||	    <device Name>
|||	        The name of the device to format as a filesystem.
|||
|||	    <volume Name>
|||	        The name to give the new filesystem.
|||
|||	  Implementation
|||
|||	    Command implemented in V33.
|||
|||	  Location
|||
|||	    System.m2/Programs/Format
|||
**/

#include <kernel/types.h>
#include <kernel/operror.h>
#include <file/filefunctions.h>
#include <file/filesystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/****************************************************************************/


#define Error(x,err) {printf x; PrintfSysErr(err);}


int32 main(int32 argc, char *argv[])
{
Err         result;
int         parm;
char       *devName;
char       *volName;
char       *fsType;
Item        dev;
FileSystem *fs;

    fsType  = NULL;
    devName = NULL;
    volName = NULL;

    for (parm = 1; parm < argc; parm++)
    {
        if ((strcasecmp("-help",argv[parm]) == 0)
         || (strcasecmp("-?",argv[parm]) == 0))
        {
            printf("format - format media as a file system\n");
            printf("  -fsType <filesystem type> - type of filesystem to use\n");
            printf("  <device Name>             - name of device containing media to format\n");
            printf("  <volume Name>             - name of the new filesystem\n");
            return (0);
        }

        if (strcasecmp("-fsType",argv[parm]) == 0)
        {
            parm++;
            if (parm == argc)
            {
                printf("No type specified for '-fsType' option\n");
                return 1;
            }
            fsType = argv[parm];
        }
        else if (volName)
        {
            printf("Too many arguments\n");
            return 1;
        }
        else if (devName)
        {
            volName = argv[parm];
        }
        else
        {
            devName = argv[parm];
        }
    }

    if (!devName)
    {
        printf("No device name given\n");
        return 1;
    }

    if (!volName)
    {
        printf("No volume name given\n");
        return 1;
    }

    result = NamedDeviceStackMounted(devName);
    if (result != 0)
    {
        if (result < 0)
        {
            Error(("Unable to access device %s: ", devName), result);
            return 1;
        }

        fs = (FileSystem *) LookupItem(result);
        printf("Filesystem /%s is already mounted on '%s'\n", fs->fs_FileSystemName, devName);

        return 1;
    }

    dev = result = OpenNamedDeviceStack(devName);
    if (dev >= 0)
    {
        result = FormatFileSystemVA(dev, volName, FORMATFS_TAG_FSNAME, fsType, TAG_END);
        if (result < 0)
        {
            Error(("Unable to format %s: ", devName), result);
        }
        CloseDeviceStack(dev);
    }
    else
    {
        Error(("Unable to access %s: ", devName), result);
    }

    return result;
}
