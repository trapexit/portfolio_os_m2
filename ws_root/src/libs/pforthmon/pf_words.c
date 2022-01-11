/* @(#) pf_words.c 96/08/05 1.8*/
/***************************************************************
** Forth words for PForth based on 'C'
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
**
**	941031	rdg		fix ffScan() to look for CRs and LFs
**
***************************************************************/

#include "pf_all.h"


/***************************************************************
** Print number in current base to output stream.
** This version does not handle double precision.
*/
void ffDot( int32 n )
{
	MSG( ConvertNumberToText( n, gVarBase, TRUE, 1 ) );
	EMIT(' ');
}

/***************************************************************
** Print number in current base to output stream.
** This version does not handle double precision.
*/
void ffDotHex( int32 n )
{
	MSG( ConvertNumberToText( n, 16, TRUE, 1 ) );
	EMIT(' ');
}

/* ( ... --- ... , print stack ) */
void ffDotS( void )
{
	cell *sp;
	int32 i, Depth;

	MSG("Stack<");
	MSG( ConvertNumberToText( gVarBase, 10, TRUE, 1 ) ); /* Print base in decimal. */
	MSG("> ");
	
	Depth = gCurrentTask->td_StackBase - gCurrentTask->td_StackPtr;
	sp = gCurrentTask->td_StackBase;
	
	if( Depth < 0 )
	{
		MSG("UNDERFLOW!");
	}
	else
	{
		for( i=0; i<Depth; i++ )
		{
			ffDot( *(--sp) );
		}
	}
	MSG("\n");
}

/* ( addr cnt char -- addr' cnt' , skip leading characters ) */
cell ffSkip( char *AddrIn, cell Cnt, char c, char **AddrOut )
{
	char *s;
	
	s = AddrIn;

	if( c == BLANK )
	{
		while( ( Cnt > 0 ) &&
			(( *s == BLANK) || ( *s == '\t')) )
		{
DBUGX(("ffSkip BLANK: %c, %d\n", *s, Cnt ));
			s++;
			Cnt--;
		}
	}
	else
	{
		while(( Cnt > 0 ) && ( *s == c ))
		{
DBUGX(("ffSkip: %c=0x%x, %d\n", *s, Cnt ));
		s++;
		Cnt--;
		}
	}
	*AddrOut = s;
	return Cnt;
}

/* ( addr cnt char -- addr' cnt' , scan for char ) */
cell ffScan( char *AddrIn, cell Cnt, char c, char **AddrOut )
{
	char *s;
	
	s = AddrIn;

	if( c == BLANK )
	{
		while(( Cnt > 0 ) &&
			( *s != BLANK) &&
			( *s != '\r') &&
			( *s != '\n') &&
			( *s != '\t'))
		{
DBUGX(("ffScan BLANK: %c, %d\n", *s, Cnt ));
			s++;
			Cnt--;
		}
	}
	else
	{
		while(( Cnt > 0 ) && ( *s != c ))
		{
DBUGX(("ffScan: %c, %d\n", *s, Cnt ));
			s++;
			Cnt--;
		}
	}
	*AddrOut = s;
	return Cnt;
}

/***************************************************************
** Forth equivalent 'C' functions.
***************************************************************/
/* Convert a string to the corresponding hex number. */
cell ffNumberQ( char *FWord, cell *Num )
{
	int32 Len, i, Accum=0, n, Sign=1;
	char *s;
	
/* get count */
	Len = *FWord++;
	s = FWord;
DBUGX(("ffNumberQ: Len=%d, s[0]=%c\n", Len, *s));

/* process initial minus sign */
	if( *s == '-' )
	{
		Sign = -1;
		s++;
		Len--;
	}

	for( i=0; i<Len; i++)
	{
		n = HexDigitToNumber( *s++ );
		if( n < 0 )
		{
			return NUM_TYPE_BAD;
		}
		
		Accum = (Accum << 4) + n;
	}
	*Num = Accum * Sign;
	return NUM_TYPE_SINGLE;
}

/***************************************************************
** Compiler Support
***************************************************************/

/* ( char -- c-addr , parse word ) */
char * ffWord( char c )
{
	char *s1,*s2,*s3;
	int32 n1, n2, n3;
	int32 i, nc;

	s1 = gCurrentTask->td_SourcePtr + gCurrentTask->td_IN;
	n1 = gCurrentTask->td_SourceNum - gCurrentTask->td_IN;
	n2 = ffSkip( s1, n1, c, &s2 );
DBUGX(("ffWord: s2=%c, %d\n", *s2, n2 ));
	n3 = ffScan( s2, n2, c, &s3 );
DBUGX(("ffWord: s3=%c, %d\n", *s3, n3 ));
	nc = n2-n3;
	if (nc > 0)
	{
		gScratch[0] = nc;
		for( i=0; i<nc; i++ )
		{
			gScratch[i+1] = pfCharToUpper( s2[i] );
		}
	}
	else
	{
	
		gScratch[0] = 0;
	}
	gCurrentTask->td_IN += (n1-n3) + 1;
	return &gScratch[0];
}
