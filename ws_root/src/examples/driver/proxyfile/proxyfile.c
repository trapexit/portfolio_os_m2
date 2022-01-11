/* @(#) proxyfile.c 96/07/19 1.9 */

/*
 *
 *	Proxy a file as if it were a block device
 *
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
#include <kernel/kernelnodes.h>
#include <kernel/io.h>
#include <kernel/operror.h>
#include <kernel/time.h>
#include <kernel/task.h>
#include <file/filesystem.h>
#include <file/filefunctions.h>
#include <device/proxy.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#undef	DEBUG

#ifdef	DEBUG
#define	DBUG(x)	printf x
#else	/* DEBUG */
#define	DBUG(x)	/* x */
#endif	/* DEBUG */

#define DBUG0(x) printf x


#define MAXBUF 32768

char	*pname;

#define MakePDErr(svr,class,err)    MakeErr(ER_DEVC,((Make6Bit('P')<<6)|Make6Bit('D')),svr,ER_E_SSTM,class,err)

#define kPDErrNotSupported          MakePDErr(ER_SEVERE, ER_C_STND, ER_NotSupported);
#define kPDErrAborted		    MakePDErr(ER_SEVERE, ER_C_STND, ER_Aborted)
#define kPDErrBadArg		    MakePDErr(ER_SEVERE, ER_C_STND, ER_BadIOArg)
#define kPDErrBadCommand	    MakePDErr(ER_SEVERE, ER_C_STND, ER_BadCommand)
#define kPDErrBadUnit		    MakePDErr(ER_SEVERE, ER_C_STND, ER_BadUnit)
#define kPDErrNotPrivileged	    MakePDErr(ER_SEVERE, ER_C_STND, ER_NotPrivileged);
#define kPDErrSoftErr		    MakePDErr(ER_SEVERE, ER_C_STND, ER_SoftErr)
#define kPDErrNoMemAvail	    MakePDErr(ER_SEVERE, ER_C_STND, ER_NoMem)
#define kPDErrIOInProgress	    MakePDErr(ER_SEVERE, ER_C_STND, ER_IONotDone)
#define kPDErrIOIncomplete	    MakePDErr(ER_SEVERE, ER_C_STND, ER_IOIncomplete)
#define kPDErrDeviceOffline	    MakePDErr(ER_SEVERE, ER_C_STND, ER_DeviceOffline)
#define kPDErrDeviceError	    MakePDErr(ER_SEVERE, ER_C_STND, ER_DeviceEr

/**
|||	AUTODOC -class Shell_Commands -name ProxyFile
|||	Proxy a file so that it becomes a block-oriented device
|||
|||	  Format
|||
|||	    proxyfile deviceitem filepath
|||
|||	  Description
|||
|||	    This command is used to provide "proxy" device driver services
|||	    which allow the contents of a file to be accessed as if the
|||	    file were a block-structured disk device capable of supporting
|||	    a filesystem.
|||
|||	    This command is NOT intended to be invoked directly from the
|||	    shell.  Rather, it's intended to be invoked as a server process
|||	    by the proxy driver, when a proxied (virtual) device is opened.
|||	    The proxy driver locates this program, and identifies the path
|||	    to the file to be proxied, by examining entries in the DDF
|||	    (Device Description File) for the proxied device.
|||
|||	    A sample DDF which uses this server program might be:
|||
|||	  -preformatted
|||
|||	        version 1.0     // Note:  this is a per-file version.
|||
|||	        driver proxyfile // this names the device visible to client
|||	        uses proxy
|||
|||	            needs
|||	                nothing = 0
|||	            end needs
|||
|||	            provides
|||	                cmds: CMD_STATUS, CMD_BLOCKREAD,
|||	                      CMD_BLOCKWRITE, CMD_GETMAPINFO,
|||	                      CMD_MAPRANGE, CMD_UNMAPRANGE,
|||	                      CMD_PREFER_FSTYPE
|||	                SERVERPATH: "System.M2/Programs/ProxyFile"
|||	                SERVERARG:  "/remote/mount.this.image"
|||	                FS: 10
|||	            end provides
|||	        end driver
|||
|||	  -normal_format
|||
|||	    If this DDF file is present in the System.M2/Devices directory of
|||	    an "blessed" M2 filesystem, the DDF will be loaded into memory
|||	    when the filesystem is mounted.  Subsequently, any attempt to
|||	    open the device named in this DDF will result in the proxy driver
|||	    being loaded.  The proxy driver will create the named device,
|||	    and will create a task which runs the proxyfile program.  The
|||	    proxyfile program will then serve as a user-mode "device driver",
|||	    making the contents of the "mount.this.image" file appear as
|||	    the contents of the device.
|||
|||	    One would normally use this device driver by issuing a shell
|||	    command such as "mount proxyfile" or "mount foobar" or
|||	    whatever.  Note that the name of the proxied device (given in
|||	    the "driver" line of the DDF) does not need to be the same as
|||	    the name of the proxyfile program itself (given in the SERVERPATH
|||	    line).  A single copy of the server program "proxyfile" can be
|||	    used to support any number of different proxied devices (each of
|||	    which must of course have a unique name).
|||
|||	  Implementation
|||
|||	    Command implemented in V30.
|||
|||	  Location
|||
|||	    System.m2/Programs/ProxyFile
|||
|||	  See Also
|||
|||	    Mount(@)
|||
**/

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

static const char *funcs[] =
{
  "open device",
  "sendio",
  "abortio",
  "close device",
  "delete device"
};

int
main(int ac, char *av[])
{
	Err	err;
	Item    fileItem, fileioItem = 0, proxyItem, proxyioItem = 0;
	Item    clientItem;
	IOReq   *fileIOReq = NULL, *clientIOReq;
	Item    portItem, msgItem;
	FileStatus fStatus;
	DeviceStatus dStatus;
	IOInfo  ioInfo;
	Message *msg;
	int32 stop = 0, dwimi, bytes, devOffset, devBlocks, burstBlocks;
	int32 tagsChewed, actual, maxFit = 0, blocksTransferred;
	int32 isOpen = FALSE;
	char *pathname, *devName;
	void *buffer = NULL;
	int32 alive = FALSE, openers = 0;

	TagArg dwimTags[20];

	pname = basenm(av[0]);

	if (ac != 3) {
		printf("%s: Wrong number of arguments\n", pname);
		return 1;
	}

	proxyItem = atoi(av[1]);
	pathname = av[2];

	portItem = CURRENTTASK->t_DefaultMsgPort;

	devName = ((Device *) LookupItem(proxyItem))->dev.n_Name;

	DBUG0(("%s proxying device 0x%X '%s', msg port 0x%X, path '%s'\n",
	       pname, proxyItem, devName, portItem, pathname));

	DBUG(("Open the file\n"));

	fileItem = OpenFile(pathname);
	if (fileItem < 0) {
	  printf("%s: can't open %: ", pname, pathname);
	  PrintfSysErr(fileItem);
	  goto serve;
	}

	DBUG(("Create fileio\n"));

	fileioItem = CreateIOReq(NULL, 0, fileItem, 0);
	if (fileioItem < 0) {
	  printf("%s: can't create IOReq for file: ", pname);
	  PrintfSysErr(fileioItem);
	  goto serve;
	}

	fileIOReq = IOREQ(fileioItem);

	memset(&ioInfo, 0, sizeof ioInfo);
	memset(&fStatus, 0, sizeof fStatus);

	ioInfo.ioi_Command = CMD_STATUS;
	ioInfo.ioi_Recv.iob_Buffer = &fStatus;
	ioInfo.ioi_Recv.iob_Len = sizeof fStatus;

	DBUG(("Get file status\n"));

	err = DoIO(fileioItem, &ioInfo);

	if (err < 0) {
	  printf("%s: error getting file status: ", pname);
	  PrintfSysErr(err);
	  goto serve;
	}

	if ((fStatus.fs_Flags & FILE_IS_DIRECTORY) ||
	    fStatus.fs.ds_DeviceBlockCount == 0 ||
	    fStatus.fs.ds_DeviceBlockSize == 0 ||
	    fStatus.fs.ds_DeviceBlockSize > MAXBUF) {
	  printf("%s: %s is not suitable for proxy operation\n", pname,
		 pathname);
	  goto serve;
	}

	buffer = AllocMem(MAXBUF, MEMTYPE_FILL);

	if (!buffer) {
	  DBUG(("Could not allocate %d-byte buffer\n", MAXBUF));
	  goto serve;
	}

	DBUG(("Buffer is at 0x%X\n", buffer));

	DBUG(("Device is ready\n"));

	maxFit = MAXBUF / fStatus.fs.ds_DeviceBlockSize;
	alive = TRUE;

      serve:

	while ((msgItem = WaitPort(portItem, 0)) && !stop) {

	  if (msgItem < 0) {
	    DBUG(("Error getting message\n"));
/*	    PrintfSysErr(msgItem); */
	    break;
	  }


	  msg = MESSAGE(msgItem);

	  DBUG(("Got message item 0x%X at 0x%X\n", msgItem, msg));

	  if (msg->msg_Val1 > PROXY_FUNC_DELETE_DEVICE) {
	    DBUG0(("Illegal message code 0x%X\n", msg->msg_Val1));
	    break;
	  }

	  TOUCH(funcs);
	  DBUG(("Got '%s' message\n", funcs[msg->msg_Val1]));

	  err = 0;

	  switch (msg->msg_Val1) {

	  case PROXY_FUNC_OPEN_DEVICE:

	    DBUG(("Task 0x%X has opened '%s'\n", msg->msg_Val2, devName));
	    openers ++;
	    break;

	  case PROXY_FUNC_CLOSE_DEVICE:

	    DBUG(("Task 0x%X has closed '%s'\n", msg->msg_Val2, devName));
	    openers --;
	    break;

	  case PROXY_FUNC_SENDIO:

	    if (!isOpen) {

	      DBUG(("Open proxied device\n"));

	      err = OpenItem(proxyItem, NULL);

	      if (err < 0) {
		printf("Could not open the device I'm supposed to proxy!\n");
		PrintfSysErr(err);
		return 0;
	      }

	      DBUG(("Create proxy I/O\n"));

	      proxyioItem = CreateIOReq(NULL, 0, proxyItem, 0);
	      if (proxyioItem < 0) {
		printf("%s: can't create IOReq to talk with proxy\n", pname);
		PrintfSysErr(proxyioItem);
		return 0;
	      }

	      isOpen = TRUE;
	    }

	    clientItem = msg->msg_Val2;
	    clientIOReq = IOREQ(clientItem);
	    if (!clientIOReq ||
		clientIOReq->io_Extension[0] != msgItem ||
		(clientIOReq->io_Flags & IO_DONE)) {
	      err = kPDErrBadArg; /* something seriously bogus */
	      break;
	    }

	    memset(dwimTags, 0, sizeof dwimTags);
	    dwimi = 0;

	    dwimTags[dwimi].ta_Tag = PROXY_TAG_IOREQ;
	    dwimTags[dwimi].ta_Arg = (void *) clientItem;
	    dwimi++;

	    if (!alive) {
	      err = kPDErrDeviceOffline;
	      goto finishCommand;
	    }

	    switch (clientIOReq->io_Info.ioi_Command) {

	    case CMD_STATUS:
	      memcpy(&dStatus, &fStatus, sizeof dStatus);
	      dStatus.ds_DeviceUsageFlags = DS_USAGE_FILESYSTEM;
	      bytes = clientIOReq->io_Info.ioi_Recv.iob_Len;
	      if (bytes > sizeof dStatus) {
		bytes = sizeof dStatus;
	      }
	      dwimTags[dwimi].ta_Tag = PROXY_TAG_SET_SRC_PTR;
	      dwimTags[dwimi].ta_Arg = (void *) &dStatus;
	      dwimi++;
	      dwimTags[dwimi].ta_Tag = PROXY_TAG_SET_DEST_OFFSET;
	      dwimTags[dwimi].ta_Arg = (void *) 0;
	      dwimi++;
	      dwimTags[dwimi].ta_Tag = PROXY_TAG_SET_COUNT;
	      dwimTags[dwimi].ta_Arg = (void *) bytes;
	      dwimi++;
	      dwimTags[dwimi].ta_Tag = PROXY_TAG_MEMCPY;
	      dwimi++;
	      dwimTags[dwimi].ta_Tag = PROXY_TAG_SET_IO_ACTUAL;
	      dwimTags[dwimi].ta_Arg = (void *) bytes;
	      dwimi++;
	      break;

	    case CMD_READ:
	    case CMD_BLOCKREAD:
	      bytes = clientIOReq->io_Info.ioi_Recv.iob_Len;
	      devOffset = clientIOReq->io_Info.ioi_Offset;
	      devBlocks = bytes / fStatus.fs.ds_DeviceBlockSize;
	      DBUG(("Request to read %d block[s] from offset %d\n",
		    devBlocks, devOffset));
	      if (bytes % fStatus.fs.ds_DeviceBlockSize != 0 ||
		  devOffset < 0 ||
		  devOffset + devBlocks > fStatus.fs.ds_DeviceBlockCount) {
		err = kPDErrBadArg;
		break;
	      }
	      err = 0;
	      actual = 0;
	      tagsChewed = 0;
	      while (devBlocks > 0 && !err) {
		burstBlocks = (devBlocks > maxFit) ? maxFit : devBlocks;
		memset(&ioInfo, 0, sizeof ioInfo);
		ioInfo.ioi_Command = CMD_BLOCKREAD;
		ioInfo.ioi_Recv.iob_Buffer = buffer;
		ioInfo.ioi_Recv.iob_Len =
		  burstBlocks * fStatus.fs.ds_DeviceBlockSize;
		ioInfo.ioi_Offset = devOffset;
		err = DoIO(fileioItem, &ioInfo);
		tagsChewed = 0;
		if (err >= 0) {
		  dwimTags[dwimi].ta_Tag = PROXY_TAG_SET_SRC_PTR;
		  dwimTags[dwimi].ta_Arg = buffer;
		  dwimTags[dwimi+1].ta_Tag = PROXY_TAG_SET_DEST_OFFSET;
		  dwimTags[dwimi+1].ta_Arg = (void *) actual;
		  dwimTags[dwimi+2].ta_Tag = PROXY_TAG_SET_COUNT;
		  dwimTags[dwimi+2].ta_Arg = (void *) fileIOReq->io_Actual;
		  dwimTags[dwimi+3].ta_Tag = PROXY_TAG_MEMCPY;
		  actual += fileIOReq->io_Actual;
		  blocksTransferred =
		    fileIOReq->io_Actual / fStatus.fs.ds_DeviceBlockSize;
		  devBlocks -= blocksTransferred;
		  devOffset += blocksTransferred;
		  if (devBlocks == 0) {
		    tagsChewed = 4;
		    DBUG(("Transferring last or only segment\n"));
		  } else {
		    dwimTags[dwimi+4].ta_Tag = TAG_END;
		    DBUG(("Transferring partial segment\n"));
		    memset(&ioInfo, 0, sizeof ioInfo);
		    ioInfo.ioi_Command = PROXY_CMD_DWIM;
		    ioInfo.ioi_Send.iob_Buffer = dwimTags;
		    ioInfo.ioi_Send.iob_Len = sizeof dwimTags;
		    err = DoIO(proxyioItem, &ioInfo);
		    if (err < 0) {
		      DBUG0(("Proxy DWIM error\n"));
		      PrintfSysErr(err);
		    }
		  }
		}
	      }
	      dwimi += tagsChewed;
	      dwimTags[dwimi].ta_Tag = PROXY_TAG_SET_IO_ACTUAL;
	      dwimTags[dwimi].ta_Arg = (void *) actual;
	      dwimi++;
	      break;

	    default:
	      err = kPDErrBadCommand;
	      break;
	    }

	  finishCommand:

	    dwimTags[dwimi].ta_Tag = PROXY_TAG_SET_IO_ERROR;
	    dwimTags[dwimi].ta_Arg = (void *) err;
	    dwimi++;
	    dwimTags[dwimi].ta_Tag = PROXY_TAG_COMPLETEIO;
	    dwimi++;
	    dwimTags[dwimi].ta_Tag = TAG_END;

	    if (err == 0) {
	      DBUG(("Completing request, no error\n"));
	    } else {
	      DBUG(("Completing request with an error\n"));
/*	      PrintfSysErr(err); */
	    }

	    memset(&ioInfo, 0, sizeof ioInfo);

	    ioInfo.ioi_Command = PROXY_CMD_DWIM;
	    ioInfo.ioi_Send.iob_Buffer = dwimTags;
	    ioInfo.ioi_Send.iob_Len = sizeof dwimTags;

	    err = DoIO(proxyioItem, &ioInfo);

	    if (err < 0) {
	      DBUG0(("Proxy DWIM error: "));
	      PrintfSysErr(err);
	    }

	    break;

	  case PROXY_FUNC_ABORTIO:
	    break;

	  case PROXY_FUNC_DELETE_DEVICE:
	    stop = 1;
	    break;
	  }

	  if (CheckItem(msg->msg_ReplyPort,KERNELNODE,MSGPORTNODE)) {
	    DBUG(("Replying with a result of 0x%X\n", err));
	    err = ReplyMsg(msgItem, err, 0, 0);
	    if (err < 0) {
	      DBUG0(("Error sending reply\n"));
	      PrintfSysErr(err);
	    }
	  }

	  if (isOpen && openers == 0) { /* could check isAlive also */
	    DBUG(("Closing device to allow for possible deletion\n"));
	    DeleteItem(proxyioItem);
	    DBUG(("IOReq has been deleted\n"));
	    CloseItem(proxyItem);
	    DBUG(("Proxy device has been closed\n"));
	    isOpen = FALSE;
	  }

	}

	DBUG0(("Exiting\n"));

	return 0;
}



