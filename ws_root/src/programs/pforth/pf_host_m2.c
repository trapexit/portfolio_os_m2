/* @(#) pf_system.c 95/04/25 1.3 */
/***************************************************************
** System Dependant code for PForth based on 'C'
**
** Author: Phil Burk
**
** Copyright Phil Burk 1994
** All rights reserved.
**
****************************************************************
***************************************************************/

#include "pf_all.h"

#ifdef PF_CDE_SERIAL

#include "hardware/cde.h"
#include "cde_serial.h"
FILE *stdin;
FILE *stdout;

#ifdef SUPPORT_602_CACHE
static int32 gIfCacheOn = 0;

void TurnOnCaches( void )
{
	Err    result = 0;
	uint32 oldhid;
	uint32 oldints;

	oldhid = _mfhid0();

/* Turn on data cache. */
	if (!(oldhid & HID_DCE))
	{
		/* Invalidate and turn on the cache */
		_mthid0(oldhid | HID_DCI);
		_mthid0(oldhid & ~HID_DCI);
		_mthid0((oldhid | HID_DCE) & ~HID_DCI);
	}
/* Turn on instruction cache. */
	if (!(oldhid & HID_ICE))
	{
		/* Invalidate and turn on the cache */
		_mthid0(oldhid | HID_ICFI);
		_mthid0(oldhid & ~HID_ICFI);
		_mthid0(oldhid | HID_ICE);
	}
	cdeWriteSerial('C');
	cdeWriteSerial('a');
	cdeWriteSerial('c');
	cdeWriteSerial('h');
	cdeWriteSerial('e');
	cdeWriteSerial('s');
	cdeWriteSerial(' ');
	cdeWriteSerial('o');
	cdeWriteSerial('n');
	cdeWriteSerial('\n');
}
#endif

/********************************************************/
int32 m2PutChar( int32 theChar, FileStream * Stream )
{
	TOUCH(Stream);
	return cdeWriteSerial( theChar );
}

/********************************************************
** Returns character from input.  Waits until one received.
*/
int32 m2GetChar( FileStream * Stream  )
{
	int32 theChar = cdeReadSerial();

	TOUCH(Stream);
/* Echo character received. */
	cdeWriteSerial( theChar );
	return theChar;
}
#endif /* PF_CDE_SERIAL */

/***********************************************************************************/
#ifdef PF_NO_FILEIO
/* Declare stubs for standard I/O */
/*
** FileStream *stdin;
** FileStream *stdout;
*/

FileStream *m2OpenFile( char *FileName, char *Mode ) { TOUCH(FileName); TOUCH(Mode); return NULL; }
int32 m2FlushFile( FileStream * Stream  ) { TOUCH(Stream); return 0; }
int32 m2ReadFile( void *ptr, int32 Size, int32 nItems, FileStream * Stream  ) 
{ 
	TOUCH(ptr);
	TOUCH(Size);
	TOUCH(nItems);
	TOUCH(Stream);
	return 0; 
}
int32 m2WriteFile( void *ptr, int32 Size, int32 nItems, FileStream * Stream  )
{ 
	TOUCH(ptr);
	TOUCH(Size);
	TOUCH(nItems);
	TOUCH(Stream);
	return 0; 
}
int32 m2SeekFile( FileStream * Stream, int32 Position, int32 Mode ) 
{ 
	TOUCH(Stream);
	TOUCH(Position);
	TOUCH(Mode);
	return 0; 
}
int32 m2TellFile( FileStream * Stream ) 
{ 
	TOUCH(Stream);
	return 0; 
}
int32 m2CloseFile( FileStream * Stream ) 
{ 
	TOUCH(Stream);
	return 0; 
}
#endif

