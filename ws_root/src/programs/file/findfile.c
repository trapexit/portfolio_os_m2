/* @(#) findfile.c 96/02/26 1.6 */

/**
|||	AUTODOC -class Shell_Commands -name FindFile
|||	Searches for a file, and prints its pathname.
|||
|||	  Synopsis
|||
|||	    FindFile <partialPath>
|||
|||	  Description
|||
|||	    Scan for a file and print its pathname.
|||
|||	  Arguments
|||
|||	    partialPath
|||	        Specifies a relative pathname. FindFile will search for the
|||	        file identified by this path in the current directory, and in
|||	        the root directory of each mounted filesystem. If more than one
|||	        such file exists, the file with the highest version/revision
|||	        number will be chosen.
|||
|||	  Location
|||
|||	    System.m2/Programs/FindFile
|||
**/

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/operror.h>
#include <file/filesystem.h>
#include <file/filefunctions.h>
#include <file/directory.h>
#include <file/directoryfunctions.h>
#include <stdio.h>
#include <string.h>

int main(int32 argc, char **argv)
{
  Err   err;
  char *pathBuf[512];
  static TagArg searchTags[] = {
#ifdef FILESEARCH_TAG_TRACE_SEARCH
    {FILESEARCH_TAG_TRACE_SEARCH, (void *) 1},
#endif
    {FILESEARCH_TAG_SEARCH_PATH, "."},
    {FILESEARCH_TAG_SEARCH_FILESYSTEMS, 0},
    {TAG_END, 0}};

  if (argc != 2)
    {
      printf("Usage: findfile RELATIVEPATH\n");
      return 0;
    }

  err = FindFileAndIdentify((const char *) pathBuf, sizeof pathBuf,
			      (const char *) argv[1], searchTags);
  if (err < 0) {
    printf("Search failed: ");
    PrintfSysErr(err);
  } else {
    printf("Search found '%s'\n", pathBuf);
  }

  return 0;

}
