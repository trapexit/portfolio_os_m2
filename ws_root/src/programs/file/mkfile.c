/* @(#) mkfile.c 95/08/29 1.5 */
/* $Id: mkfile.c,v 1.3 1994/02/18 17:48:18 limes Exp $ */

/*
  mkfile.c - make a file (usually on the NVRAM filesystem)
*/

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/mem.h>
#include <kernel/nodes.h>
#include <kernel/debug.h>
#include <kernel/list.h>
#include <kernel/device.h>
#include <kernel/driver.h>
#include <kernel/kernel.h>
#include <kernel/kernelnodes.h>
#include <kernel/io.h>
#include <kernel/operror.h>
#include <file/filesystem.h>
#include <file/filefunctions.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main (int32 argc, char **argv)
{
  int i;
  Item newFileItem;
  if (argc < 2) {
    printf("Usage: mkfile file [file2 ...]\n");
  } else {
    for (i = 1; i < argc; i ++) {
      printf("Creating %s\n", argv[i]);
      newFileItem = CreateFile(argv[i]);
      if (newFileItem < 0) {
	PrintError(0,"create",argv[i],newFileItem);
      } else {
	printf("Item 0x%x created\n", newFileItem);
      }
    }
  }
  return 0;
}
