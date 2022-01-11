/* @(#) minimizefs.c 96/02/26 1.3 */

/**
|||	AUTODOC -class Shell_Commands -name MinimizeFS
|||	Minimize filesystem/folio memory footprint.
|||
|||	  Synopsis
|||
|||	    MinimizeFS
|||
|||	  Description
|||
|||	    MinimizeFS uses the MinimizeFileSystem() call to ask the
|||	    Portfolio filesystem manager to minimize its memory footprint.
|||
|||	  Implementation
|||
|||	    Command implemented in V29.
|||
|||	  Location
|||
|||	    System.m2/Programs/MinimizeFS
|||
|||	  See Also
|||
|||	    Mount, Dismount
|||
**/

#include 	<kernel/types.h>
#include 	<kernel/item.h>
#include 	<kernel/mem.h>
#include 	<kernel/nodes.h>
#include 	<kernel/debug.h>
#include 	<kernel/list.h>
#include 	<kernel/device.h>
#include 	<kernel/driver.h>
#include 	<kernel/msgport.h>
#include 	<kernel/kernel.h>
#include 	<kernel/kernelnodes.h>
#include 	<kernel/io.h>
#include 	<kernel/time.h>
#include 	<kernel/operror.h>
#include 	<file/discdata.h>
#include 	<file/filesystem.h>
#include 	<file/filesystemdefs.h>
#include 	<file/filefunctions.h>
#include 	<stdio.h>
#include 	<string.h>

int
main(int ac, char *av[])
{

  TOUCH(ac);
  TOUCH(av);

  MinimizeFileSystem(FSMINIMIZE_RECOMMENDED);

  printf("Minimize request has been submitted.\n");

  return 0;
}


