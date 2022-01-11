/* @(#) dismount.c 96/02/26 1.6 */

/**
|||	AUTODOC -class Shell_Commands -name Dismount
|||	Dismounts a file system.
|||
|||	  Format
|||
|||	    Dismount <mountName>
|||
|||	  Description
|||
|||	    This command lets you dismount file systems, which produces similar
|||	    results as physically removing the media containing the file
|||	    system.
|||
|||	  Arguments
|||
|||	    <mountName name>
|||	        The name of the file system to dismount.
|||
|||	  Implementation
|||
|||	    Command implemented in V20.
|||
|||	  Location
|||
|||	    System.m2/Programs/Dismount
|||
|||	  See Also
|||
|||	    Mount(@)
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
#include <kernel/time.h>
#include <kernel/operror.h>
#include <file/discdata.h>
#include <file/filesystem.h>
#include <file/filesystemdefs.h>
#include <file/filefunctions.h>
#include <stdio.h>
#include <string.h>

#define fail(err,verb,noun) if (err < 0) { PrintError(0,verb,noun,err); return (int) err; }

int main(int argc, char **argv)
{
  int32 err;
  if (argc != 2) {
    printf("Usage: dismount <mountName>\n");
    return 0;
  }
  err = DismountFileSystem(argv[1]);
  fail(err,"dismount",argv[1]);
  return (int) err;
}
