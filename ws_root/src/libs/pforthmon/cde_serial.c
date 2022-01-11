/* @(#) cde_serial.c 95/11/15 1.3 */
/***************************************************************
** CDE serial driver in 'C" based on Martin Hunt's assembly code.
**
** Author: Phil Burk
**
** Copyright 3DO 1994
** All rights reserved.
**
****************************************************************
***************************************************************/

#include "kernel/types.h"
#include "hardware/cde.h"
#include "bootcode/boothw.h"
#include "cde_serial.h"

/* Use serial debug port in CDE */
static  int32  gIfInitSerial = 0;
static  int32  gIfCDE2 = 0;
/********************************************************/
void cdeInitSerial( void )
{
	gIfCDE2 = CDE_READ( CDE_BASE, CDE_DEVICE_ID ) & 1;

	if( gIfCDE2 )
	{
 /* set for 9600 baud assuming 25 MHz CDE clock */
		CDE_WRITE( CDE_BASE, CDE_SDBG_CNTL, 0x000028b0 );
	}
	else
	{
 /* set for 4800 baud */
		CDE_WRITE( CDE_BASE, CDE_SDBG_CNTL, 0 );
	}

/* Clear interrupt enables for serial. */
	CDE_WRITE( CDE_BASE, (CDE_INT_ENABLE + CDE_CLEAR_OFFSET), 
		(CDE_SDBG_RD_DONE | CDE_SDBG_WRT_DONE) );

/* Clear completion bits for serial IO. */
	CDE_WRITE( CDE_BASE, (CDE_INT_STS + CDE_CLEAR_OFFSET), 
		(CDE_SDBG_RD_DONE | CDE_SDBG_WRT_DONE) );
	gIfInitSerial = 1;
}

/********************************************************/
int32 cdeWriteSerial( int32 theChar )
{
	if( !gIfInitSerial ) cdeInitSerial();
	
/* Convert Line Feeds to Carriage Returns for dumb Mac terminal programs. */
	if( theChar == 0x0A ) theChar = 0x0D;
	
/* Send character. */
	CDE_WRITE( CDE_BASE, CDE_SDBG_WRT, theChar );

/* Poll until Write done and ready for next char. */
	while( (CDE_READ(CDE_BASE,CDE_INT_STS) & CDE_SDBG_WRT_DONE) == 0 );

/* Clear completion bits for serial write. */
	CDE_WRITE( CDE_BASE, (CDE_INT_STS + CDE_CLEAR_OFFSET), 
		CDE_SDBG_WRT_DONE );
	return 0;
}

/********************************************************
** Returns character from input.  Waits until one received.
*/
int32 cdeReadSerial( void  )
{
	int32 theChar;

	if( !gIfInitSerial ) cdeInitSerial();

/* Poll for Read done, is character ready?. */
	while( (CDE_READ(CDE_BASE,CDE_INT_STS) & CDE_SDBG_RD_DONE) == 0 );

/* Get character. */
	theChar = CDE_READ( CDE_BASE, CDE_SDBG_RD );

/* Clear completion bits for serial read. */
	CDE_WRITE( CDE_BASE, (CDE_INT_STS + CDE_CLEAR_OFFSET), 
		CDE_SDBG_RD_DONE );
		
	return theChar;
}

/********************************************************
** Returns TRUE if a character is ready for input.
*/
int32 cdeCheckSerial( void )
{
	if( !gIfInitSerial ) cdeInitSerial();
/* Check for Read done. Is character ready?. */
	return ( CDE_READ(CDE_BASE,CDE_INT_STS) & CDE_SDBG_RD_DONE ) ? TRUE : FALSE ;
}
