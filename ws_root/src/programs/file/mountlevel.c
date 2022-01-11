/* @(#) mountlevel.c 96/03/11 1.4 */

/**
|||	AUTODOC -class Shell_Commands -name MountLevel
|||	Displays or changes the current filesystem automounter level
|||
|||	  Synopsis
|||
|||	    mountlevel [newlevel]
|||
|||	  Description
|||
|||	    Displays the current filesystem mount level, and optionally
|||	    changes it to a different level.
|||
|||	  Arguments
|||
|||	    [newlevel]
|||	        New mount level (an integer between 0 and 255)
|||
|||	  Location
|||
|||	    System.m2/Programs/mountlevel
|||
**/


#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/operror.h>
#include <file/filesystem.h>
#include <file/filefunctions.h>
#include <file/directory.h>
#include <file/directoryfunctions.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


int main(int32 argc, char **argv)
{
  int32 oldlevel, newlevel;

  oldlevel = GetMountLevel();

    if (argc <= 1)
    {
      printf("Mount level is %d\n", oldlevel);
    }
    else
    {
      newlevel = atoi(argv[1]);
      if (newlevel < 0 || newlevel > 255) {
	printf("Illegal mount level - must be between 0 and 255\n");
      } else {
	(void) SetMountLevel(newlevel);
	printf("Mount level was %d, is now %d\n", oldlevel, newlevel);
      }
    }

    return 0;
}
