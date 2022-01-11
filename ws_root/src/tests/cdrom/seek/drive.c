/* @(#) drive.c 96/05/01 1.5 */

/*
  Copyright 1995, The 3DO Compant.
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  drive.c - test hook

  I would not mess with this file. You can write good seek tests by calling
  this file using the #defines in drive.h. If you change this file, then I
  cannot help you afterwards because I honestly do not fully understand its
  operation. That is why all of the code within is isolated in this place,
  away from other code I might play with.  - Hedley
	
  See drive.h
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
#include <kernel/time.h>
#include <device/cdrom.h>
#include <file/filesystem.h>
#include <file/filesystemdefs.h>
#include <file/filefunctions.h>
#include <file/discdata.h>
#include <misc/event.h>
#include <stdlib.h>

#define ARMC

#ifndef ARMC
#include <stdlib.h>
#endif

#include <strings.h>
#include <stdio.h>

#include "drive.h"

#define DBG 	if( 1==1 ) printf

#define CHK4ERR(err, str)       \
                if ((err) < 0) { printf (str); PrintfSysErr(err); exit(9); }

int errlim = 0;
/* extern int exit( int code) ; */
extern Item SuperCreateSizedItem(int32 itemType, void *whatever, int32 size);


int CDKludge(int operation, unsigned char *buf, int sector, int length)
{
	static Item ioReqItem = 0 ;
	static IOReq *ioReq = 0 ;
	static Item cdRom ;
	static int32 err;
	static int blocksize = CDROM_AUDIO_SUBCODE;
	static int density = CDROM_DIGITAL_AUDIO;
	static DeviceStatus status;
	static IOInfo ioInfo;
	static union CDROMCommandOptions options;
  
	static char *DeviceName = "cdrom";
  
	static TagArg ioReqTags[2] =
	{
		CREATEIOREQ_TAG_DEVICE,		0,
		TAG_END,					0
	};
  
	switch (operation)
	{
		case 0:
			switch(length)		/* type */
			{
				case MyCDG:		
					blocksize 	= CDROM_AUDIO_SUBCODE;
					density 	= CDROM_DIGITAL_AUDIO ; 
					break;
				case MyCDDA:	
					blocksize 	= CDROM_AUDIO;
					density 	= CDROM_DIGITAL_AUDIO ;
					break;
				case MyCDROM:
					blocksize 	= CDROM_MODE1;
					density 	= CDROM_DEFAULT_DENSITY ;
					break;
				default:
					printf("\nDeath and Doom. Init of CDROM driver confused.\n");
					exit(99);
				break;
			}
			
			if (buf)
				DeviceName = (char *)buf;
	
	  		memset(&ioInfo, 0, sizeof ioInfo);
	
	  		options.asLongword = 0;
	
			DBG("CD-ROM Open Device Name is >%s<\n", DeviceName);  
	  		options.asFields.speed = sector ? CDROM_DOUBLE_SPEED : CDROM_SINGLE_SPEED;
	  		options.asFields.blockLength = blocksize;
	  		options.asFields.densityCode = density;
	  		cdRom = OpenNamedDeviceStack(DeviceName);
	
			DBG("CD-ROM device is item 0x%x\n", cdRom);
	
	  		if (cdRom < 0)
				exit(0);
		
	  		ioReqTags[0].ta_Arg = (void *)cdRom;
	  		ioReqItem = CreateItem(MKNODEID(KERNELNODE,IOREQNODE), ioReqTags);
			CHK4ERR(ioReqItem, "Can't allocate an IOReq for CD-ROM device\n");
	 	 
			ioReq = (IOReq *)LookupItem(ioReqItem);
		
			ioInfo.ioi_Send.iob_Buffer = NULL;
			ioInfo.ioi_Send.iob_Len = 0;
			ioInfo.ioi_Recv.iob_Buffer = &status;
			ioInfo.ioi_Recv.iob_Len = sizeof(status);
			ioInfo.ioi_Command = CMD_STATUS;
			ioInfo.ioi_CmdOptions = options.asLongword;
			err = DoIO(ioReqItem, &ioInfo);
			if (err < 0)
				printf("Device flags word contains 0x%x\n", status.ds_DeviceFlagWord);
			CHK4ERR(err, "Error in CMD_STATUS\n");
	
			DBG("CDROM disc block count is %d\n", status.ds_DeviceBlockCount);
			DBG("Device flags word contains 0x%x\n", status.ds_DeviceFlagWord);
	
			return (status.ds_DeviceBlockCount);
		case 1:
			if (!blocksize)
			{
				printf("\nDeath and Doom. Read before init of CDROM driver.\n");
				exit(99) ;
			}

			ioInfo.ioi_Recv.iob_Buffer = (void *) (buf) ;
	    	ioInfo.ioi_Recv.iob_Len = (uint32)(blocksize*length);
	    	ioInfo.ioi_Offset =  sector ;
	    	ioInfo.ioi_Command = CMD_READ;
	    	err = DoIO(ioReqItem, &ioInfo);
			if (err < 0)
	    		printf("Block %d: ", sector);
			CHK4ERR(err, "Error in DoIO()\n");

			if (ioReq->io_Actual != blocksize*length)
			{
	    		printf("Block %d: only got %d bytes\n", sector, ioReq->io_Actual);
	    		exit(9);
			}
			break;
		default:
			printf("Operation (%d) undefined!\n", operation);
			exit(0);
	}

	return 0;
}
