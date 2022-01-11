/* @(#) cde_serial.h 95/06/08 1.1 */
#ifndef _cde_serial_h
#define _cde_serial_h

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

void cdeInitSerial( void ); /* set for 4800 baud */
int32 cdeWriteSerial( int32 theChar );
int32 cdeReadSerial( void  );
int32 cdeCheckSerial( void );

#endif /* _cde_serial_h */

