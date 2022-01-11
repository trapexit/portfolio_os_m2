/*******************************************************
**
** @(#) beep_init.c 96/07/03 1.8
**
** Beep folio machine init and term.
**
** Author: Phil Burk
** Copyright 1995 3DO
** All Rights Reserved
*******************************************************/

#include <beep/beep.h>
#include <kernel/types.h>
#include <kernel/debug.h>       /* print_vinfo() */
#include <kernel/tags.h>        /* tag iteration */
#include <kernel/kernel.h>
#include <stdlib.h>
#include <dspptouch/dspp_touch.h>
#include "beep_internal.h"

#define DBUG(x)     /* PRT(x) */

Err beepInitDSP( void )
{
	int32 i;
	Err result;

	result =  dsphInitDSPP();
	if( result < 0 ) return result;
	
	for( i=0; i<DSPI_NUM_DMA_CHANNELS; i++ ) dsphClearInputFIFO(i);

	DBUG(("beepInitDSP: beepInitInterrupt\n"));
	return beepInitInterrupt();
}

Err beepTermDSP( void )
{
	
	beepTermInterrupt();
	dsphTermDSPP();
	return 0;
}
