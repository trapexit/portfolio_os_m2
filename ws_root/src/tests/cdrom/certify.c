/* @(#) certify.c 96/02/18 1.9 */

/*
  certify.c - simple CD-ROM certification program
*/

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
#include <device/cdrom.h>
#include <file/filesystem.h>
#include <file/filesystemdefs.h>
#include <file/discdata.h>
#include <file/filefunctions.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


extern TagArg fileFolioTags[];
extern TagArg fileDriverTags[];

extern Item SuperCreateSizedItem(int32 itemType, void *whatever, int32 size);

/* #define DEBUG */

#ifdef DEBUG
#define DBUG(x) printf x
#else
#define DBUG(x) /* x */
#endif

#define BUFSIZE 512*1024
#define READSIZE 2048
#define READINCR 1
#define READMINBLOCK 150

uint32 beq(void *s, void *d, int32 n)
{
  char *s1 = (char *) s;
  char *d1 = (char *) d;
  while (--n >= 0) {
    if (*s1++ != *d1++) return 0;
  }
  return 1;
}

int main(int argc, char **argv)
{
  Item ioReqItem;
  IOReq *ioReq;
  Item cdRom;
  int32 err;
  uint32 i;
  int passnum;
  int readincr, readsize;
  DeviceStatus status;
  char *buf1, *buf2;
  IOInfo ioInfo;
  union CDROMCommandOptions options;
  int32 argnum;
  static TagArg ioReqTags[2] =
    {
      CREATEIOREQ_TAG_DEVICE,       0,
      TAG_END,			0
      };
  options.asLongword = 0;
  for (argnum = 1; argnum < argc; argnum++) {
    if (strcmp(argv[argnum], "-ds") == 0) {
      options.asFields.speed = CDROM_DOUBLE_SPEED;
      printf("Double-speed operation requested\n");
    } else if (strcmp(argv[argnum], "-ss") == 0) {
      options.asFields.speed = CDROM_SINGLE_SPEED;
      printf("Normal-speed operation requested\n");
    } else if (strcmp(argv[argnum], "-circ") == 0) {
      options.asFields.errorRecovery = CDROM_CIRC_RETRIES_ONLY;
      printf("CIRC error recovery only, no LERC\n");
    } else if (strncmp(argv[argnum], "-r", 2) == 0) {
      options.asFields.retryShift = 0;
      printf("Retry limit set to 0\n");
      if (options.asFields.errorRecovery == CDROM_Default_Option) {
	options.asFields.errorRecovery = CDROM_DEFAULT_RECOVERY;
      }
    }
  }
  cdRom = FindNamedItem(MKNODEID(KERNELNODE,DEVICENODE), "cdrom");
  printf("cdrom device is item 0x%x\n", cdRom);
  if (cdRom < 0) return 0;
  ioReqTags[0].ta_Arg = (void *) cdRom;
  ioReqItem = CreateItem(MKNODEID(KERNELNODE,IOREQNODE), ioReqTags);
  if (ioReqItem < 0) {
    printf("Can't allocate an IOReq for cdrom device");
    return 0;
  }
  ioReq = (IOReq *) LookupItem(ioReqItem);
  buf1 = (char *) AllocMem(BUFSIZE, MEMTYPE_DMA);
  buf2 = (char *) AllocMem(BUFSIZE, MEMTYPE_DMA);
  if (!buf1 || !buf2) {
    printf("Can't allocate buffers\n");
    return 0;
  }
  ioInfo.ioi_Send.iob_Buffer = NULL;
  ioInfo.ioi_Send.iob_Len = 0;
  ioInfo.ioi_Recv.iob_Buffer = &status;
  ioInfo.ioi_Recv.iob_Len = sizeof status;
  ioInfo.ioi_Command = CMD_STATUS;
  ioInfo.ioi_CmdOptions = options.asLongword;
  err = DoIO(ioReqItem, &ioInfo);
  if (err < 0 || (err = ioReq->io_Error) < 0) {
    printf("Error getting status: ");
    PrintfSysErr(err);
    return 0;
  }
  for (readincr = BUFSIZE / READSIZE, readsize = BUFSIZE, passnum = 1;
       readincr >= 1;
       readincr /= 2, readsize /= 2, passnum += 1) {
    printf("Pass %d - read and compare %d block(s) at a time\n",
	    passnum, readincr);
    for (i = READMINBLOCK;
	 i < status.ds_DeviceBlockCount - readincr - 3;
	 i += readincr) {
      ioInfo.ioi_Recv.iob_Buffer = (void *) buf1;
      ioInfo.ioi_Recv.iob_Len = readsize;
      ioInfo.ioi_Offset = i;
      ioInfo.ioi_Command = CMD_READ;
      err = DoIO(ioReqItem, &ioInfo);
      if (err < 0 || (err = ioReq->io_Error) < 0) {
	printf("Error reading block %d first time: ", i);
	PrintfSysErr(err);
      } else if (ioReq->io_Actual != readsize) {
	printf("Only got %d bytes for block %d\n", ioReq->io_Actual, i);
      } else {
	ioInfo.ioi_Recv.iob_Buffer = (void *) buf2;
	ioInfo.ioi_Recv.iob_Len = readsize;
	ioInfo.ioi_Offset = i;
	ioInfo.ioi_Command = CMD_READ;
	err = DoIO(ioReqItem, &ioInfo);
	if (err < 0 || (err = ioReq->io_Error) < 0) {
	  printf("Error reading block %d second time: ", i);
	  PrintfSysErr(err);
	} else if (ioReq->io_Actual != readsize) {
	  printf("Only got %d bytes for block %d\n", ioReq->io_Actual, i);
	} else {
	  if (!beq(buf1, buf2, readsize)) {
	    printf("Compare mismatch on block %d!\n");
	  }
	}
      }
    }
  }
  printf("Done\n");
  return 0;
}

