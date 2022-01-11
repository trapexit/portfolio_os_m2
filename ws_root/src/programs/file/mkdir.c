/* @(#) mkdir.c 96/02/26 1.2 */

/**
|||	AUTODOC -class Shell_Commands -name MkDir
|||	Creates new directories.
|||
|||	  Format
|||
|||	    MkDir {directory}
|||
|||	  Description
|||
|||	    This command creates directories. It can only create directories
|||	    in the filesystems that are mounted read-write and support
|||	    directories.
|||
|||	  Arguments
|||
|||	    {directory}
|||	        Names of the directories to create.
|||
|||	  Implementation
|||
|||	    Command implemented in V29.
|||
|||	  Location
|||
|||	    System.m2/Programs/MkDir
|||
|||	  See Also
|||
|||	    Delete
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
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/*
 *	debug stuff
 */
/* #define	DEBUG */
#ifdef	DEBUG
#define	DBG(x)	printf x
#else	/* DEBUG */
#define	DBG(x)	/* x */
#endif	/* DEBUG */

#define	USAGE	printf("USAGE: %s directory ...\n", pname)

char	*pname;

char	*basenm(char *dir);

int
main(int ac, char *av[])
{
	Err	err;

	pname = basenm(*av);
	ac--, av++;

	if (ac < 1) {
		printf("%s: Wrong number of arguments\n", pname);
		USAGE;
		return 1;
	}

	for (; ac; ac--, av++) {
		DBG(("%s on %s\n", pname, *av));
		if ((err = CreateDirectory(*av)) < 0)
			PrintError(0, "create", *av, err);
	}
	return 0;
}


char	*
basenm(char *dir)
{
	char *cp = dir;

	while (*cp++)
		;
	cp--;
	while ((*cp != '/') && (cp != dir))
		cp--;
	if (cp == dir)
		return dir;
	else
		return (cp + 1);
}


