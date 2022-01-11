#ifndef __BOOTCODE_BOOTHW_H
#define __BOOTCODE_BOOTHW_H


/******************************************************************************
**
**  @(#) boothw.h 96/06/11 1.5
**
**  Hardware definitions which should only be used by bootcode.
**  Other parts of the system should use SysInfo to get this information.
**
******************************************************************************/


#define	CDE_BASE	0x04000000	/* Base address of CDE */
#define	VID_THRESHOLD	0xD		/* Upper bound of "safe" video region */


#endif /* __BOOTCODE_BOOTHW_H */
