/* @(#) recheckfs.c 96/02/26 1.6 */

/**
|||	AUTODOC -class Shell_Commands -name RecheckFS
|||	Recheck all filesystems to see if they are still online.
|||
|||	  Format
|||
|||	    RecheckFS
|||
|||	  Description
|||
|||	    RecheckFS uses the RecheckAllFileSystems() call to ask the
|||	    Portfolio filesystem manager to verify that all mounted filesystems
|||	    are still on-line, and to start dismounting any which are not.
|||
|||	  Implementation
|||
|||	    Command implemented in V29.
|||
|||	  Location
|||
|||	    System.m2/Programs/RecheckFS
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

  Err err;

  TOUCH(ac);
  TOUCH(av);

  err = RecheckAllFileSystems();

  if (err < 0) {
    printf("Couldn't request filesystem recheck!\n");
    PrintfSysErr(err);
  } else {
    printf("Recheck request has been submitted.\n");
  }

  return err;
}


