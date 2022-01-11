/* @(#) delete.c 96/11/26 1.10 */

/**
|||	AUTODOC -class Shell_Commands -name Delete
|||	Deletes files and directories.
|||
|||	  Format
|||
|||	    Delete [-all] {file | directory}
|||
|||	  Description
|||
|||	    This command lets you delete files or directories.
|||	    Without the -all option, this command will delete only
|||	    files and empty directories. When the -all option is given,
|||	    the command will recursively delete directories and their
|||	    contents.
|||
|||	  Arguments
|||
|||	    -all
|||	        Specifies that if a non-empty directory is provided, all of
|||	        its contents may be deleted. If this option is not provided,
|||	        then attempting to delete a directory that is not empty will
|||	        fail.
|||
|||	    {file | directory}
|||	        Specifies the name of the file or directory to delete. Any
|||	        number of files or directories can be specified.
|||
|||	  Implementation
|||
|||	    Command implemented in V20.
|||
|||	  Location
|||
|||	    System.m2/Programs/Delete
|||
**/

#include <kernel/types.h>
#include <file/filefunctions.h>
#include <file/filesystem.h>
#include <file/directory.h>
#include <file/directoryfunctions.h>
#include <file/fsutils.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/****************************************************************************/

static void usage(void)
{
    printf("usage: delete [-all] file...\n");
    printf("  -all  - deletes directories recursively\n");
    printf("  file  - specifies files or directories to delete\n");
    exit(0);
}

/****************************************************************************/


int32 main(int32 argc, char *argv[])
{
int32 parm;
bool  all;
Err   err;

    all = FALSE;

    if (argc <= 1)
	usage();

    for (parm = 1; parm < argc; parm++)
    {
        if ((strcasecmp("-help",argv[parm]) == 0)
         || (strcasecmp("-?",argv[parm]) == 0))
	    usage();

        if (strcasecmp("-all",argv[parm]) == 0)
        {
            all = TRUE;
	    continue;
        }

        if (all)
        {
	    err = DeleteTree(argv[parm], NULL);
        }
	else
        {
            err = DeleteFile(argv[parm]);
            if (err == FILE_ERR_NOTAFILE || err == FILE_ERR_DIRNOTEMPTY)
                err = DeleteDirectory(argv[parm]);
        }
        if (err < 0)
	{
	    PrintError(NULL, "delete", argv[parm], err);
            break;
	}
    }

    return 0;
}
