/***********************************************************
**
** @(#) test_cde_serial.c 95/06/22 1.1
**
** Test CDE serial interface.
**
** Author: Phil Burk
** Copyright (C) 3DO 1995
************************************************************/

#include "kernel/types.h"
#include "pforth/cde_serial.h"
#include "pforth/cde_serial.c"

static char Greetings[] = "0123456789 Greetings. Hit q to quit.\n";

int main( int32 argc, char **argv )
{
	int32 i;
	char *s, c;

/* Output message. */
	s = &Greetings[0];
	while(*s != 0)
	{
		cdeWriteSerial( *s++ );
	}

/* Wait for 'q' */
	do
	{
		c = cdeReadSerial();
/* Echo twice */
		cdeWriteSerial( c );
		cdeWriteSerial( c );
	} while(c != 'q');
	return 0;
}
