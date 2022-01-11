/* @(#) m2cd.c 96/04/03 1.16 */

/*
	File:		m2cd.c

	Contains:	Used to test the M2 cd-rom driver/device.

	Copyright:	©1995 by The 3DO Company, all rights reserved.
*/


#include <kernel/driver.h>
#include <kernel/mem.h>
#include <kernel/task.h>
#include <kernel/debug.h>
#include <kernel/random.h>
#include <kernel/operror.h>
#include <device/m2cd.h>
#include <misc/event.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define	DEBUG	1

#if DEBUG
	#define DBUG(x) kprintf x
#else
	#define DBUG(x)
#endif

#define	CHK4ERR(x,str)	if ((x)<0) { DBUG(str); PrintfSysErr(x); return (x); }

#define BCD2BIN(bcd)	((((bcd) >> 4) * 10) + ((bcd) & 0x0F))
#define BIN2BCD(bin)	((((bin) / 10) << 4) | ((bin) % 10))

extern uint32 SectorECC(uint8 *);

void printBin(uint32 bin)
{
	int8 x;

	for (x = 1; x <= 32; x++, bin <<= 1)
	{
		printf("%c", (bin & 0x80000000) ? '1' : '.');
		if (!(x % 4))
			printf(" ");
	}
}

uint32 BCDMSF2Offset(uint32 minutes, uint32 seconds, uint32 frames)
{
	minutes = BCD2BIN(minutes);
	seconds = BCD2BIN(seconds);
	frames = BCD2BIN(frames);
	
	return ((((minutes * 60L) + seconds) * 75L) + frames);
}

uint32 ScaledRand(uint32 scale) 
{
	uint32 r;
	r = ReadHardwareRandomNumber() % scale;
	return (r);
}

Err WaitForExit(void)
{
	ControlPadEventData cped;
	Err             err;
					 
	err = InitEventUtility (1, 0, LC_ISFOCUSED);
	CHK4ERR(err, ("Error in InitEventUtility\n"));
							  
	for (;;)
	{
		/* if any button event, then break */
		GetControlPad (1, FALSE, &cped);
		if (cped.cped_ButtonBits) break;
	}

	KillEventUtility();

	return (0);
}

int main(int32 argc,char *argv[])
{
	Err					err;
	uint8				*buf = NULL, *buf2 = NULL, *buf3 = NULL, sendBuf[12], cmd;
	uint32				len = 0, x, min, sec, frm, addLen, diff, cnt;
	int32				ecc;
	Item				daemon, devItem, iorItem;
	IOReq*				ior;
	IOInfo				ioInfo;
	cdrom				cdInfo;
	SubQInfo			qCode;
	DeviceStatus		devStat;
	CDWobbleInfo		wobbleInfo;
	CDROM_Disc_Data		discInfo;
	CDROMCommandOptions	cmdOpts;
	ControlPadEventData	cped;
	
	if (argc >= 2)
	{
		memset(&ioInfo,0,sizeof(ioInfo));

		cmdOpts.asLongword = 0;

		devItem = OpenNamedDeviceStack("cdrom");
		CHK4ERR(devItem, ("M2CD:  >ERROR<  Couldn't open CD-ROM device\n"));
	
		iorItem = CreateIOReq(0,0,devItem,0);
		CHK4ERR(iorItem, ("M2CD:  >ERROR<  Couldn't create ioReqItem\n"));
	
		ior = (IOReq*)LookupItem(iorItem);

		cmd = argv[1][0];
		switch (cmd)
		{
			case 'k':	/* kill daemon */
				DBUG(("M2CD:  Attemping to kill the daemon...\n"));
				daemon = FindTask("M2CD Daemon");
				if (daemon >= 0)								
				{
					SendSignal(daemon, SIGF_ABORT);
					while(FindTask("M2CD Daemon") == daemon);
					kprintf("M2CD Daemon is dead!\n\n");
				}

				return (0);
			case 'e':	/* test ecc code */
			case 'f':	/* test ecc code */
				/* initialize event broker, in order to use cpad */
				err = InitEventUtility (1, 0, LC_ISFOCUSED);
				CHK4ERR(err, ("Error in InitEventUtility\n"));
							  
				ioInfo.ioi_Command = CDROMCMD_DISCDATA;
				ioInfo.ioi_Recv.iob_Buffer = &discInfo;
				ioInfo.ioi_Recv.iob_Len = sizeof(discInfo);
				DoIO(iorItem, &ioInfo);

				ioInfo.ioi_Command = CDROMCMD_READ;
				cmdOpts.asFields.densityCode = CDROM_DEFAULT_DENSITY;
				cmdOpts.asFields.blockLength = 2340;
				cmdOpts.asFields.speed = CDROM_DOUBLE_SPEED;
				cmdOpts.asFields.errorRecovery = CDROM_CIRC_RETRIES_ONLY;
				cmdOpts.asFields.retryShift = 0;
				cmdOpts.asFields.addressFormat = CDROM_Address_Abs_MSF;

				if (argc == 5)
				{
					min = strtol(argv[2],0,10);
					sec = strtol(argv[3],0,10);
					frm = strtol(argv[4],0,10);
				}
				else
				{
					min = ScaledRand(discInfo.info.minutes-1);
					sec = ScaledRand(59);
					frm = ScaledRand(74);
				}
				ioInfo.ioi_Offset = (min << 16) | (sec << 8) | (frm);

				ioInfo.ioi_CmdOptions = cmdOpts.asLongword;
				
				len = 2340;
				buf = (uint8 *)AllocMem(len, MEMTYPE_NORMAL);
				buf2 = (uint8 *)AllocMem(len, MEMTYPE_NORMAL);
				buf3 = (uint8 *)AllocMem(len, MEMTYPE_NORMAL);
				if (!buf || !buf2 || !buf3)
				{
					kprintf("M2CD: >ERROR<  Could not allocate buffer memory\n");
					return (0);
				}
				ioInfo.ioi_Recv.iob_Buffer = buf;
				ioInfo.ioi_Recv.iob_Len = len;

				DBUG(("Buffer @ %08x\n", buf));
				DBUG(("M2CD:  Testing ECC code...\n"));
				DBUG(("M2CD:  Reading from %02d:%02d:%02d...  ", min, sec, frm));
				break;
			case 'c':	/* close drawer */
				ioInfo.ioi_Command = CDROMCMD_CLOSE_DRAWER;

				DBUG(("M2CD:  Closing the drawer...\n"));
				break;
			case 'o':	/* open drawer */
				ioInfo.ioi_Command = CDROMCMD_OPEN_DRAWER;

				DBUG(("M2CD:  Opening the drawer..."));
				break;
			case 'q':	/* read subq */
				ioInfo.ioi_Command = CDROMCMD_READ_SUBQ;
				ioInfo.ioi_Recv.iob_Buffer = &qCode;
				ioInfo.ioi_Recv.iob_Len = sizeof(qCode);
	
				DBUG(("M2CD:  Getting a QCode packet..."));
				break;
			case 's':	/* CDROMCMD_SCAN_READ */
			case 'r':	/* CDROMCMD_READ */
			case 'i':	/* CMD_READ */
			case 'j':	/* CMD_BLOCKREAD */
			case 'x':	/* CMD_STATUS */
				ioInfo.ioi_Command = CMD_STATUS;
				ioInfo.ioi_Recv.iob_Buffer =  &devStat;
				ioInfo.ioi_Recv.iob_Len = sizeof(devStat);
				if (cmd == 'x')
					break;
				DoIO(iorItem, &ioInfo);
				if (devStat.ds_DeviceBlockSize == CDROM_MODE1)
				{
					cmdOpts.asFields.densityCode = CDROM_DEFAULT_DENSITY;
					addLen = (argc == 7) ? strtol(argv[6],0,10) : 0;
					cmdOpts.asFields.blockLength = CDROM_MODE1 + addLen;
				}
				else
				{
					cmdOpts.asFields.densityCode = CDROM_DIGITAL_AUDIO;
					cmdOpts.asFields.blockLength = (argc == 7) ? CDROM_AUDIO_SUBCODE : CDROM_AUDIO;
					cmdOpts.asFields.speed = CDROM_SINGLE_SPEED;
				}

				ioInfo.ioi_Command = (cmd == 'r') ? CDROMCMD_READ : CDROMCMD_SCAN_READ;
				if (cmd == 'i')
					ioInfo.ioi_Command = CMD_READ;
				if (cmd == 'j')
					ioInfo.ioi_Command = CMD_BLOCKREAD;

				min = strtol(argv[2],0,10);
				sec = strtol(argv[3],0,10);
				frm = strtol(argv[4],0,10);
				ioInfo.ioi_Offset = (min << 16) | (sec << 8) | (frm);

				cmdOpts.asFields.addressFormat = CDROM_Address_Abs_MSF;
				cmdOpts.asFields.pitch = CDROM_PITCH_NORMAL;
				ioInfo.ioi_CmdOptions = cmdOpts.asLongword;
				
				len = strtol(argv[5],0,10) * cmdOpts.asFields.blockLength;
				buf = (uint8 *)AllocMem(len, MEMTYPE_NORMAL);
				if (!buf)
				{
					kprintf("M2CD: >ERROR<  Could not allocate buffer for %s sectors (%d bytes)\n", argv[5], len);
					return (0);
				}
				
				ioInfo.ioi_Recv.iob_Buffer = buf;
				ioInfo.ioi_Recv.iob_Len = len;

				DBUG(("M2CD:  Reading %s sectors (total of %d bytes) from ", argv[5], len));
				DBUG(("%02d:%02d:%02d\n", min, sec, frm));
				DBUG(("Buffer @ %08x\n", buf));
				break;
			case 'd':	/* set/get defaults */
				ioInfo.ioi_Command = CDROMCMD_SETDEFAULTS;
				ioInfo.ioi_CmdOptions = strtol(argv[2],0,16);	

				ioInfo.ioi_Recv.iob_Buffer = &cmdOpts;
				ioInfo.ioi_Recv.iob_Len = sizeof(cmdOpts);
				
				DBUG(("M2CD:  S/Getting defaults..."));
				break;
			case 'b':	/* resize buffers */
				ioInfo.ioi_Command = CDROMCMD_RESIZE_BUFFERS;
				ioInfo.ioi_CmdOptions = strtol(argv[2],0,16);	
				
				DBUG(("M2CD:  Resizing buffers...\n"));
				break;
			case 't':	/* get discdata */
				ioInfo.ioi_Command = CDROMCMD_DISCDATA;
				ioInfo.ioi_Recv.iob_Buffer = &discInfo;
				ioInfo.ioi_Recv.iob_Len = sizeof(discInfo);
				
				DBUG(("M2CD:  Getting TOC info..."));
				break;
			case 'w':	/* get wobble info */
				ioInfo.ioi_Command = CDROMCMD_WOBBLE_INFO;
				ioInfo.ioi_Recv.iob_Buffer = &wobbleInfo;
				ioInfo.ioi_Recv.iob_Len = sizeof(wobbleInfo);

				min = strtol(argv[2],0,10);		
				sec = strtol(argv[3],0,10);
				frm = strtol(argv[4],0,10);
				ioInfo.ioi_Offset = (min << 16) | (sec << 8) | (frm);

				if (argc == 6)
				{
					cmdOpts.asFields.speed = strtol(argv[5],0,10);
					ioInfo.ioi_CmdOptions = cmdOpts.asLongword;
				}

				DBUG(("M2CD:  Getting wobble info from %02d:%02d:%02d ", min, sec, frm));
				DBUG(("(spd=%d)...\n", cmdOpts.asFields.speed));
				break;
			case 'p':	/* send commands to drive using passthru mode */
				ioInfo.ioi_Command = CDROMCMD_PASSTHRU;
				for (x = 2; x < argc; x++)
					sendBuf[x-2] = (uint8)strtol(argv[x], 0, 16);
				ioInfo.ioi_Send.iob_Buffer = sendBuf;
				ioInfo.ioi_Send.iob_Len = (argc - 2);

				DBUG(("M2CD:  Sending cmd to drive(r)..."));
				break;
			case 'z':	/* set/get diag info */
				ioInfo.ioi_Command = CDROMCMD_DIAG_INFO;
				switch (argv[1][1])
				{
					case 'p':
						ioInfo.ioi_CmdOptions = strtol(argv[2],0,16);	
						ioInfo.ioi_Send.iob_Buffer = &argv[1][1];
						ioInfo.ioi_Send.iob_Len = 1;
						break;
					case 'h':
						ioInfo.ioi_CmdOptions = strtol(argv[2],0,10);	
						ioInfo.ioi_Send.iob_Buffer = &argv[1][1];
						ioInfo.ioi_Send.iob_Len = 1;
						break;
					default:
						ioInfo.ioi_Recv.iob_Buffer = &cdInfo;
						ioInfo.ioi_Recv.iob_Len = sizeof(cdInfo);
						break;
				}
				DBUG(("M2CD:  S/Getting diag info..."));
				break;
			default:
				goto help;
				break;
		}
	}
	else
	{
help:
		kprintf("\nM2CD: usage M2CD [cefoqtxz]\n");
		kprintf("                 [b <bufMode>]\n");
		kprintf("                 [d <CmdOpts>]\n");
		kprintf("                 [rsij <MSF in BCD> <num sectors> <extra data>]\n");
		kprintf("                 [w <MSF in BCD> <speed>]\n");
		kprintf("M2CD:    b - resize the prefetch buffer space to <bufMode>\n");
		kprintf("M2CD:        where <bufMode> is one of:\n");
		kprintf("M2CD:          CDROM_DEFAULT_BUFFERS (0)\n");
		kprintf("M2CD:          CDROM_MINIMUM_BUFFERS (1)\n");
		kprintf("M2CD:          CDROM_STANDARD_BUFFERS (2)\n");
		kprintf("M2CD:    c - closes the drawer\n");
		kprintf("M2CD:    d <CmdOpts> - set the driver defaults to <CmdOpts>\n");
		kprintf("M2CD:        where <CmdOpts> is a 32 bit field split as follows:\n");
		kprintf("M2CD:          reserved      : 5\n");
		kprintf("M2CD:          densityCode   : 3\n");
		kprintf("M2CD:          errorRecovery : 2\n");
		kprintf("M2CD:          addressFormat : 2\n");
		kprintf("M2CD:          retryShift    : 3\n");
		kprintf("M2CD:          speed         : 3\n");
		kprintf("M2CD:          pitch         : 2\n");
		kprintf("M2CD:          blockLength   : 12\n");
 
		kprintf("M2CD:    e - test ECC code, call DebugBreakpoint() prior to correcting data\n");
		kprintf("M2CD:    f - test ECC code, don't call DebugBreakpoint()\n");
		kprintf("M2CD:    r MM SS FF n x - read n sectors starting at MM:SS:FF\n");
		kprintf("                          where 'x' is the magic cookie which gives\n");
		kprintf("                          you subcode on DA discs and varying extra\n");
		kprintf("                          data on Mode1 or XA discs.\n");
		kprintf("                          Example:  r 0 2 0 2 4 will read two sectors\n");
		kprintf("                                    starting at 0:2:0 and give the\n");
		kprintf("                                    header and data (4+2048) for each sector.\n");
		kprintf("                          Example:  r 12 34 56 1 292 will read one sector\n");
		kprintf("                                    at 12:34:56 and give the complete\n");
		kprintf("                                    sector data (2340 bytes).\n");
		kprintf("M2CD:    s - same as 'r'; but uses CDROMCMD_SCAN_READ\n");
		kprintf("M2CD:    i - same as 'r'; but uses CMD_READ\n");
		kprintf("M2CD:    j - same as 'r'; but uses CMD_BLOCKREAD\n");
		kprintf("M2CD:    o - open the drawer\n");
		kprintf("M2CD:    p - send cmds to the drive using passthru mode\n");
		kprintf("M2CD:    q - ReadSubQ\n");
		kprintf("M2CD:    t - get TOC info\n");
		kprintf("M2CD:    w MM SS FF s - get wobble info from location MSF using speed s\n");
		kprintf("M2CD:    x - get Status info\n");
		kprintf("M2CD:    z - get/set (zp,zh <opt>) diag info\n");
		return (0);
	}
	
RetryIOReq:
	err = SendIO(iorItem, &ioInfo);
	CHK4ERR(err, ("\nM2CD:  >ERROR<  Problem in SendIO(ior)\n"));
	
	err = WaitIO(iorItem);

	DBUG(("bytes returned...%d\n", ior->io_Actual));

	/* keep trying until we detect a bad sector */
	if ((cmd == 'e') || (cmd == 'f'))
	{
		if (err != MakeCDErr(ER_SEVERE, ER_C_STND, ER_MediaError))
		{
KeepOnTrying:
			/* if any button event, then exit */
			GetControlPad (1, FALSE, &cped);
			if (cped.cped_ButtonBits)
			{
				KillEventUtility();
				return (0);
			}

			if (argc == 5)
			{
				min = strtol(argv[2],0,10);
				sec = strtol(argv[3],0,10);
				frm = strtol(argv[4],0,10);
			}
			else
			{
				min = ScaledRand(discInfo.info.minutes-1);
				sec = ScaledRand(59);
				frm = ScaledRand(74);
			}
			ioInfo.ioi_Offset = (min << 16) | (sec << 8) | (frm);

			ioInfo.ioi_Recv.iob_Buffer = buf;
			ioInfo.ioi_Recv.iob_Len = len;

			kprintf("M2CD:  Reading from %02d:%02d:%02d...  ", min, sec, frm);
			goto RetryIOReq;
		}
	}
	else
		CHK4ERR(err, ("\nM2CD:  >ERROR<  Problem in WaitIO(ior)\n"));

	switch (cmd)
	{
		case 'd':
			kprintf("Current default command options...\n");
			kprintf("   .reserved      : %3x\n",cmdOpts.asFields.reserved);
			kprintf("   .densityCode   : %3x\n",cmdOpts.asFields.densityCode);
			kprintf("   .errorRecovery : %3x\n",cmdOpts.asFields.errorRecovery);
			kprintf("   .addressFormat : %3x\n",cmdOpts.asFields.addressFormat);
			kprintf("   .retryshift    : %3x\n",cmdOpts.asFields.retryShift);
			kprintf("   .speed         : %3x\n",cmdOpts.asFields.speed);
			kprintf("   .pitch         : %3x\n",cmdOpts.asFields.pitch);
			kprintf("   .blockLength   : %3x\n",cmdOpts.asFields.blockLength);
			break;
		case 'e':
		case 'f':
#if 0
			{
			RawFile	*outfile;
			err = OpenRawFile(&outfile, "sectordata", FILEOPEN_WRITE_NEW);
			CHK4ERR(err, ("M2CD:  Error opening /remote/sectordata\n"));

			WriteRawFile(outfile, buf, 2340);

			err = CloseRawFile(outfile);
			CHK4ERR(err, ("M2CD:  Error closing /remote/sectordata\n"));
			}
#else
			/* perform ECC on a copy of the sector */
			memcpy(buf2, buf, 2340);
			ecc = SectorECC(buf2);
			kprintf("SectorECC(buf2) returns 0x%08x\n", ecc);

			/* keep on looking if the ecc failed */
			if ((ecc < 0) || (ecc == 0x00010000))
				goto KeepOnTrying;

			/* keep reading the same sector until we read it cleanly */
			ioInfo.ioi_Recv.iob_Buffer = buf3;
			cnt = 0;
			do {
				err = DoIO(iorItem, &ioInfo);
				PrintfSysErr(err);
				if (err < 0)
					cnt++;					/* update failure count */
				if (cnt > 10)				/* if exceed 10 tries, give up on */
					goto KeepOnTrying;		/* this sector, and try again     */
			} while (err < 0);

			/* keep trying until we find a difference in the first 2056 bytes */
			diff = memcmp(buf2, buf3, 2056);
			kprintf("M2CD:  diff = %d\n", diff);
			if (!diff)
				goto KeepOnTrying;

			if (cmd == 'e')
				DebugBreakpoint();
			ecc = SectorECC(buf);
			kprintf("SectorECC(buf) returns 0x%08x\n", ecc);
#endif
			break;
		case 'r':
			if (argv[1][1] == 'd')
			{
				uint32 x, y;

				for (x = 0x0000; x < 0x0930; x += 0x10)
				{
					printf("%04X: ", x);
					for (y = 0; y < 16; y += 4)
						printf("%02X %02X %02X %02X ", buf[x+y], buf[x+y+1], buf[x+y+2], buf[x+y+3]);
					printf("\n");
				}
			}
			break;
		case 'q':
			kprintf("M2CD:  ");
			for (x = 0; x < sizeof(qCode); x++)
				kprintf("%02x ", ((uint8 *)&qCode)[x]);
			kprintf("\n");
			break;
		case 't':
			kprintf("     DiscID:  %02x\n", discInfo.info.discID);
			kprintf("First track:  %02d\n", discInfo.info.firstTrackNumber);
			kprintf(" Last track:  %02d\n", discInfo.info.lastTrackNumber);
			kprintf("End of Disc:  %02d:%02d:%02d\n", discInfo.info.minutes,
				discInfo.info.seconds, discInfo.info.frames);
			kprintf("SessionInfo:  %02x\n", discInfo.session.valid);
			kprintf(" SessionMSF:  %02d:%02d:%02d\n", discInfo.session.minutes,
				discInfo.session.seconds, discInfo.session.frames);
			kprintf("1st Session:  %02d:%02d:%02d\n", discInfo.firstsession.minutes,
				discInfo.firstsession.seconds, discInfo.firstsession.frames);
			kprintf("Track  AdrCtl  MM:SS:FF\n");
			kprintf("-----------------------\n");
			for (x = discInfo.info.firstTrackNumber;
				x <= discInfo.info.lastTrackNumber;
				x++)
			{
				kprintf("  %2d     %02x    ", x, discInfo.TOC[x].addressAndControl);
				kprintf("%02d:%02d:%02d\n", discInfo.TOC[x].minutes,
					discInfo.TOC[x].seconds, discInfo.TOC[x].frames);
			}
			break;
		case 'w':
			kprintf(" LowKHzEnergy:  %06x\n", wobbleInfo.LowKHzEnergy);
			kprintf("HighKHzEnergy:  %06x\n", wobbleInfo.HighKHzEnergy);
			kprintf("        Ratio:  %02x.%02x\n", wobbleInfo.RatioWhole, 
				wobbleInfo.RatioFraction);
			break;
		case 'x':
			kprintf("StatusWord:\n");
			kprintf("                            Door\n");
			kprintf("                            | DiscIn\n");
			kprintf("                            | |SpinUp\n");
			kprintf("                            | ||8xSpeed\n");
			kprintf("                            | |||6xSpeed\n");
			kprintf("                            | |||| 4xSpeed\n");
			kprintf("                            | |||| |2xSpeed\n");
			kprintf("                            | |||| ||Error\n");
			kprintf("                            | |||| |||Ready\n");
			kprintf("/-------- Reserved --------\\| |||| ||||\n");
			printBin(devStat.ds_DeviceFlagWord);
			kprintf("\n");
			break;
	}
	
	DeleteItem(iorItem);
	CloseDeviceStack(devItem);
		
	return (0);
}
