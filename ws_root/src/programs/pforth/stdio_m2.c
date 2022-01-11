/* @(#) stdio_m2.c 96/04/01 1.5 */
/*************************************************************
** Implement pseudo-standard I/O calls using M2 Portfolio.
**
** FIXME - This is not a perfect implementation of stdio but it
** works better than nothing at all.
**
** Author: Phil Burk
** Copyright 1996 Phil Burk
*************************************************************/

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/time.h>
#include <kernel/kernel.h>
#include <kernel/task.h>
#include <kernel/debug.h>
#include <kernel/mem.h>
#include <kernel/devicecmd.h>
#include <file/filefunctions.h>
#include <file/filesystem.h>
#include <file/fileio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/********************************************************/
int fputc( int theChar, FILE * Stream )
{
	char c;

	if( Stream == stdout )
	{
		return printf("%c", theChar);
	}
	else
	{
		c = theChar;
		return fwrite( &c, 1, 1, Stream );
	}
}

/********************************************************/
#ifdef BUILD_DEBUGGER
/* Ripped from tasks/shell.c and modified. */
#define DCON_BUF_SIZE   (256)
static Item           gConsoleDev = 0;
static Item           gConsoleIO = 0;
static char           gInputBuffer[DCON_BUF_SIZE];
static int32          gInputIndex;  /* Index into buffer. */
static int32          gInputLen;    /* Number of chars in buffer. */
static IOInfo         gIoInfo;

void TermDebuggerConsole( void )
{
	if( gConsoleIO )
	{
		DeleteIOReq(gConsoleIO);
		gConsoleIO = 0;
	}
	if( gConsoleDev )
	{
		CloseDeviceStack(gConsoleDev);
		gConsoleDev = 0;
	}
}

/********************************************************/
static Err InitDebuggerConsole(void)
{
Err            err;
List          *list;

	gInputIndex = 0;

	err = CreateDeviceStackListVA(&list,
		"cmds", DDF_EQ, DDF_INT, 1, HOSTCONSOLE_CMD_GETCMDLINE,
		NULL);
	if (err < 0)
	{
		printf("FATAL: cannot get device list for host console\n");
		goto clean;
	}
	if (IsEmptyList(list))
	{
		printf("FATAL: cannot find host console\n");
		err = -2;
		goto clean;
	}
	gConsoleDev = OpenDeviceStack((DeviceStack *) FirstNode(list));
	DeleteDeviceStackList(list);
	if (gConsoleDev < 0)
	{
		printf("FATAL: cannot open host console\n");
		err = -3;
		goto clean;
	}
	if (gConsoleDev >= 0)
	{
		gConsoleIO = CreateIOReq(0,0,gConsoleDev,0);
		if (gConsoleIO >= 0)
		{
			memset(&gIoInfo,0,sizeof(gIoInfo));
			gIoInfo.ioi_Command         = HOSTCONSOLE_CMD_GETCMDLINE;
			gIoInfo.ioi_Recv.iob_Buffer = gInputBuffer;
			gIoInfo.ioi_Recv.iob_Len    = sizeof(gInputBuffer) - 1;
			gIoInfo.ioi_Send.iob_Buffer = NULL;
                	gIoInfo.ioi_Send.iob_Len = 0;
			return 0;
		}
	}

clean:
	TermDebuggerConsole();
	return err;
}

/********************************************************/
Err ReadDebuggerConsole( void )
{
	Err   err = 0;

	if( gConsoleDev == 0 )
	{
		err = InitDebuggerConsole();
		if( err < 0 ) return err;
	}
	if( (KernelBase->kb_CPUFlags & KB_NODBGR) == 0)
	{
/* wait for something to happen */
		err = DoIO(gConsoleIO, &gIoInfo);
		if (err >= 0)
		{
			gInputLen = IOREQ(gConsoleIO)->io_Actual;
			gInputIndex = 0;
		}
	}
	return err;
}

int GetDebuggerChar( void )
{
	char c;
	Err err;
	int32 ret;
	if( gInputIndex >= gInputLen )
	{
		if( (err = ReadDebuggerConsole()) < 0 ) return err;
	}
	c = gInputBuffer[gInputIndex++];
	ret = (c == 0) ? '\n' : c;
	return ret;
}
#endif  /* BUILD_DEBUGGER --------------------------------------------- */

static Item  gSerialTimer = 0;
int GetSerialChar( void )
{
	int32 ch;
	if( gSerialTimer == 0 )
	{
		gSerialTimer = CreateTimerIOReq();
		if (gSerialTimer < 0) return gSerialTimer;
	}

	while( TRUE )
	{
		ch = MayGetChar();
		if( ch > 0 ) break;
		WaitTimeVBL(gSerialTimer, 1);
	}
/* Echo character, full duplex. */
	fputc( ch, stdout );
	return ch;
}

/********************************************************
** Returns character from input.  Waits until one received.
*/
int getc( FILE * Stream  )
{
	Err err;
	char c;
	int32 ret;

	if( Stream == stdin )
	{
        	if (KernelBase->kb_CPUFlags & KB_SERIALPORT)
		{
			ret = GetSerialChar();
		}
#ifdef BUILD_DEBUGGER
		else
		{
			ret = GetDebuggerChar();
		}
#else
		else
		{
			ret = -1;
		}
#endif
	}
	else
	{
		err = fread( &c, 1, 1, Stream );
		if( err < 1 )
		{
			/* printf("getc: err = 0x%x, return EOF\n", err); */
			ret = EOF;
		}
		else
		{
			ret = c;
		}
	}
/* printf("getc: ret = 0x%x = %c, index = %d, len = %d\n", ret, ret, gInputIndex, gInputLen  ); */
	return ret;
}

/***********************************************************************************/
/* Declare stubs for standard I/O */
/*
** RawFile *stdin;
** RawFile *stdout;
*/

FILE *fopen( const char *FileName, const char *Mode )
{
	Err result;
	RawFile *Stream;
	FileOpenModes  mode;

	if(Mode[0] == 'r')
	{
		mode = (Mode[1] == '+') ?
			FILEOPEN_READWRITE_EXISTING :
			FILEOPEN_READ;
	}
	else if(Mode[0] == 'w')
	{
		mode = (Mode[1] == '+') ?
			FILEOPEN_READWRITE_NEW :
			FILEOPEN_READWRITE;
	}
	else return NULL;

	result = OpenRawFile( &Stream, FileName, mode);
/* printf("fopen: file = %s, mode = %s, Stream = 0x%x, result = 0x%x\n", FileName, Mode, Stream, result ); */
	return (result < 0) ? NULL : Stream ;
}
/***********************************************************************************/
int fflush( FILE * Stream  )
{
	TOUCH(Stream);
	return 0;
}
/***********************************************************************************/
size_t fread( void *ptr, size_t Size, size_t nItems, FILE * Stream  )
{
	size_t result;
	result = ReadRawFile( (RawFile *) Stream, ptr, Size*nItems);
/* printf("m2ReadFile: ptr = 0x%x, size = %d, nitems = %d, result = 0x%x\n", ptr, Size, nItems, result ); */
	if( result > 0 ) result = result / Size;
	return result;
}
/***********************************************************************************/
size_t fwrite( void *ptr, size_t Size, size_t nItems, FILE * Stream  )
{
	size_t result;
	result = WriteRawFile( (RawFile *) Stream, ptr, Size*nItems);
	if( result > 0 ) result = result / Size;
	return result;
}
/***********************************************************************************/
int fseek( FILE * Stream, long int offset, int Mode )
{
#if 0
	FileSeekModes rawMode = 0;
	switch (Mode)
	{
	case PF_SEEK_SET: rawMode = FILESEEK_START;   break; /* FIXME */
	case PF_SEEK_CUR: rawMode = FILESEEK_CURRENT; break;
	case PF_SEEK_END: rawMode = FILESEEK_END;     break;
	}
#endif
	return SeekRawFile( (RawFile *) Stream, offset, Mode);
}
/***********************************************************************************/
long int ftell( FILE * Stream )
{
	return SeekRawFile( (RawFile *) Stream, 0, FILESEEK_CURRENT);
}
/***********************************************************************************/
int fclose( FILE * Stream )
{
/* printf("fclose: Stream = 0x%x\n", Stream ); */
	CloseRawFile( (RawFile *) Stream );
	return 0;
}

