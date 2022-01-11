#ifndef __DSPPTOUCH_INTERNAL_H
#define __DSPPTOUCH_INTERNAL_H


/******************************************************************************
**
**  @(#) dspptouch_internal.h 96/06/19 1.8
**
**  Internal include file for dspptouch library.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  Initials:
**
**  PLB: Phil Burk (phil)
**  WJB: Bill Barton (peabody)
**
******************************************************************************/

#include <kernel/types.h>
#include <stdio.h>


/* -------------------- Macros */

#define	PRT(x)	{ printf x; }
#ifdef BUILD_STRINGS
	#define ERR(x)  PRT(x)
#else
	#define ERR(x)  /* PRT(x) */
#endif

/*****************************************************************************/

#endif /* __DSPPTOUCH_INTERNAL_H */
