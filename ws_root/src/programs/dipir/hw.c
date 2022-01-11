/* @(#) hw.c 96/02/28 1.5 */

/**
|||	AUTODOC -class Shell_Commands -name HW
|||	Prints a list of all hardware resources currently in the system.
|||
|||	  Format
|||
|||	    HW
|||
|||	  Description
|||
|||	    This command prints out a list of all hardware resources currently
|||	    in the system. Hardware resources are a data structure built on
|||	    bootup and rebuilt when new devices go offline or come online.
|||	    These structures define which hardware is currently attached.
|||
|||	  Implementation
|||
|||	    Command implemented in V29.
|||
|||	  Location
|||
|||	    System.m2/Programs/HW
|||
**/

#include <kernel/types.h>
#include <kernel/device.h>
#include <dipir/hwresource.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
	HWResource hwr;
	int32 i;
	int32 last;

	printf("%5s %-32s %-8s %-4s %-8s %-8s   %s\n",
		"ID", "Name", "Channel", "Slot", "ROM-size", "ROM-offt", "Flags");

	hwr.hwr_InsertID = 0;
	while (NextHWResource(&hwr) >= 0)
	{
		printf("%5d %-32s ",
			hwr.hwr_InsertID,
			hwr.hwr_Name);
		switch (hwr.hwr_Channel)
		{
		case CHANNEL_SYSMEM:	printf("%-8s ", "SysMem");	break;
		case CHANNEL_ONBOARD:	printf("%-8s ", "OnBoard");	break;
		case CHANNEL_PCMCIA:	printf("%-8s ", "PCMCIA");	break;
		case CHANNEL_MICROCARD:	printf("%-8s ", "Micro");	break;
		case CHANNEL_LCCD:	printf("%-8s ", "LCCD");	break;
		case CHANNEL_BRIDGIT:	printf("%-8s ", "Bridgit");	break;
		case CHANNEL_CTLPORT:	printf("%-8s ", "CntlPort");	break;
		case CHANNEL_SERIAL:	printf("%-8s ", "Serial");	break;
		case CHANNEL_RTC:	printf("%-8s ", "RTC");		break;
		case CHANNEL_HOST:	printf("%-8s ", "Host");		break;
		default:		printf("%8x ", hwr.hwr_Channel); break;
		}
		printf("%4x %8x %8x   ",
			hwr.hwr_Slot,
			hwr.hwr_ROMSize,
			hwr.hwr_ROMUserStart);
		if (hwr.hwr_Perms & HWR_WRITE_PROTECT)
			printf("WRITEPROT ");
		if (hwr.hwr_Perms & HWR_XIP_OK)
			printf("XIP ");
		if (hwr.hwr_Perms & HWR_UNSIGNED_OK)
			printf("UNSIGN ");

		/* Print DeviceSpecific, but suppress trailing zero fields. */
		for (last = sizeof(hwr.hwr_DeviceSpecific)/sizeof(uint32) - 1;
		     last >= 0;
		     last--)
			if (hwr.hwr_DeviceSpecific[last] != 0)
				break;
		if (last >= 0)
		{
			printf("DevInfo: ");
			for (i = 0;  i <= last; i++)
				printf("%x ", hwr.hwr_DeviceSpecific[i]);
		}
		printf("\n");
	}

	return 0;
}
