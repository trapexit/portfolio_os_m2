/* @(#) rename.c 96/02/26 1.3 */

/**
|||	AUTODOC -class Shell_Commands -name Rename
|||	Renames a file or directory.
|||
|||	  Format
|||
|||	    Rename <oldName> <newName>
|||
|||	  Description
|||
|||	    This command lets you change the name of a file or directory.
|||
|||	  Argument
|||
|||	    oldName
|||	        The current name of the file or directory to rename.
|||
|||	    newName
|||	        The new name for the file or directory.
|||
|||	  Implementation
|||
|||	    Command implemented in V30.
|||
|||	  Location
|||
|||	    System.m2/Programs/Rename
|||
**/

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/mem.h>
#include <kernel/nodes.h>
#include <kernel/debug.h>
#include <kernel/list.h>
#include <kernel/device.h>
#include <kernel/driver.h>
#include <kernel/msgport.h>
#include <kernel/kernel.h>
#include <kernel/kernelnodes.h>
#include <kernel/io.h>
#include <kernel/operror.h>
#include <kernel/time.h>
#include <file/discdata.h>
#include <file/filesystem.h>
#include <file/filesystemdefs.h>
#include <file/filefunctions.h>
#include <file/directory.h>
#include <file/directoryfunctions.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define	USAGE	printf("USAGE: %s pathname filename\n", pname);


char	*pname;

int
main(int ac, char *av[])
{
	Err	err;
	char	*path, *fname;

	pname = *av;
	if (ac != 3) {
		printf("%s: Wrong number of arguments\n", pname);
		USAGE;
		return 1;
	}
	path = av[1];
	fname = av[2];

	if ((err = Rename(path, fname)) < 0) {
		printf("Rename(\"%s\", \"%s\") failed: ", path, fname);
		PrintfSysErr(err);
		return err;
	}

	return 0;
}
