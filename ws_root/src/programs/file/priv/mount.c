/* @(#) mount.c 96/02/26 1.11 */

/**
|||	AUTODOC -class Shell_Commands -name Mount
|||	Mounts a file system.
|||
|||	  Format
|||
|||	    Mount <device name> [offset]
|||
|||	  Description
|||
|||	    This command lets you mount file systems that aren't automatically
|||	    mounted by the system. Mounting them prepares them for use and
|||	    makes them available for regular file I/O operations.
|||
|||	  Arguments
|||
|||	    <device name>
|||	        This specifies the name of the device the file folio should
|||	        look on to find a file system.
|||
|||	    [offset]
|||	        The block offset within the device where the file system
|||	        label information can be found. If this argument is not
|||	        specified, offset 0 is assumed.
|||
|||	  Implementation
|||
|||	    Command implemented in V20.
|||
|||	  Location
|||
|||	    System.m2/Programs/Mount
|||
|||	  See Also
|||
|||	    Dismount(@)
|||
**/

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/operror.h>
#include <file/filesystem.h>
#include <file/filefunctions.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


int main(int argc, char **argv)
{
    uint32 offset;
    Item   dev;
    Err    result;
    char   devnm[40];

    if ((argc < 2) || (argc > 4)) {
        printf("Usage: mount <device name> [<device id> [offset]]\n");
        return -1;
    }

    strncpy(devnm, argv[1], 40);
    if (argc > 2) {
        offset = strtoul(argv[2], NULL, 0);
    } else {
        offset = 0;
    }

    dev = OpenNamedDeviceStack(devnm);
    if (dev < 0) {
	printf("Unable to open device '%s': ", devnm);
	PrintfSysErr(dev);
	return dev;
    }

    result = MountFileSystem(dev, offset);
    if (result < 0) {
	printf("Unable to mount file system for device %s, offset %u\n",
		devnm, offset);
	PrintfSysErr(result);
	CloseDeviceStack(dev);
    }

    return 0;
}
