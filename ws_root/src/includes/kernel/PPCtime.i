#ifndef __KERNEL_PPCTIME_I
#define __KERNEL_PPCTIME_I


/******************************************************************************
**
**  @(#) PPCtime.i 96/04/24 1.7
**
******************************************************************************/


    .struct	timeval
tv_sec	.long	1	// seconds
tv_usec	.long	1	// and microseconds
    .ends


#endif /* __KERNEL_PPCTIME_I */
