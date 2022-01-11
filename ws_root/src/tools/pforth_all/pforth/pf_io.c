/* @(#) pf_io.c 96/04/23 1.8 */
/***************************************************************
** I/O subsystem for PForth based on 'C'
**
** Author: Phil Burk
** Copyright 1994 3DO, Phil Burk, Larry Polansky, Devid Rosenboom
**
** The pForth software code is dedicated to the public domain,
** and any third party may reproduce, distribute and modify
** the pForth software code or any derivative works thereof
** without any compensation or license.  The pForth software
** code is provided on an "as is" basis without any warranty
** of any kind, including, without limitation, the implied
** warranties of merchantability and fitness for a particular
** purpose and their equivalents under the laws of any jurisdiction.
**
****************************************************************
** 941004 PLB Extracted IO calls from pforth_main.c
***************************************************************/

#include "pf_all.h"

/***************************************************************
** Flush any buffered characters in stream.
*/
void ioFlush( FileStream *Stream )
{
	sdFlushFile(Stream);
}

/***************************************************************
** Send single character to output stream.
*/
void ioEmit( char c, FileStream *Stream )
{
	int32 Result;
	Result = sdOutputChar( c, Stream );
	if( Result < 0 ) ABORT;
	if(c == '\n')
	{
		gCurrentTask->td_OUT = 0;
		sdFlushFile( Stream );
	}
	else
	{
		gCurrentTask->td_OUT++;
	}
}

void ioType( const char *s, int32 n, FileStream *Stream)
{
	int32 i;

	for( i=0; i<n; i++)
	{
		ioEmit ( *s++, Stream);
	}
}

void ioTypeCString( const char *s, FileStream *Stream )
{
	ioType( s, pfCStringLength(s), Stream );
}

/***************************************************************
** Return single character from input device, always keyboard.
*/
cell ioKey( void )
{
	return sdInputChar( (FileStream *)stdin );
}

/**************************************************************
** Receive line from input stream.
** Return length, or -1 for EOF.
*/
#define BACKSPACE  (8)
cell ioAccept( char *Target, cell MaxLen, FileStream *Stream )
{
	int32 c;
	int32 Len;
	char *p;

DBUGX(("ioAccept(0x%x, 0x%x, 0x%x)\n", Target, Len, Stream ));
	p = Target;
	Len = MaxLen;
	while(Len > 0)
	{
		c = sdInputChar(Stream);
		switch(c)
		{
			case EOF:
				DBUG(("EOF\n"));
				return -1;
				break;
				
			case '\r':
			case '\n':
				*p++ = c;
				DBUGX(("EOL\n"));
				goto gotline;
				break;
				
			case BACKSPACE:
				if( Len < MaxLen )  /* Don't go beyond beginning of line. */
				{
					EMIT(' ');
					EMIT(BACKSPACE);
					p--;
					Len++;
				}
				break;
				
			default:
				*p++ = c;
				Len--;
				break;
		}
		
	}
gotline:
	*p = '\0';
		
	return pfCStringLength( Target );
}
