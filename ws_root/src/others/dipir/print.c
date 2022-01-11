/*
 *	@(#) print.c 95/08/30 1.18
 *	Copyright 1995, The 3DO Company
 *
 * Routines for printing debugging messages.
 */

#include "kernel/types.h"
#include "varargs.h"
#include "hardware/debugger.h"
#include "dipir.h"
#include "insysrom.h"

#ifdef BUILD_STRINGS
#define	BOOTSERIAL 1
/* #define MEMORY_PUTS 1 */
/* #define LOGIC_ANALYSER 1 */
#endif

#ifdef MEMORY_PUTS
#define	CHARBUFFSIZE 0x800
static char _charbuff[CHARBUFFSIZE];
static int charcount = 0;
static char *charbuff = _charbuff;
#endif /* MEMORY_PUTS */

/*****************************************************************************
*/
void
SPutHex(uint32 value, char *buf, uint32 len)
{
	char *p;
	uint32 v;

	for (p = buf + len - 1;  p >= buf;  p--)
	{
		v = value % 16;
		value /= 16;
		*p = v < 10 ? v + '0' : v - 10 + 'A';
	}
}

#ifdef BUILD_STRINGS
/*****************************************************************************
  InitPutChar

  Do any initialization necessary to use the memory buffer, serial ports, etc.
*/
void InitPutChar(void)
{
#ifdef MEMORY_PUTS
	{
		uint32 i;

		for (i = 0; i < CHARBUFFSIZE; i++)
			charbuff[i] = ' ';
		charcount = 0;
	}
#endif /* MEMORY_PUTS */
}


/*****************************************************************************
  putc
  Display a character on the serial port, memory buffer, etc
*/
void
putc(int c)
{
#ifdef MEMORY_PUTS
	/* Put the character in a memory buffer */
	{
		charbuff[charcount++] = c;
		if (charcount >= CHARBUFFSIZE) charcount = 0;
		charbuff[charcount] = '>';	/* Current end marker */
	}
#endif /* MEMORY_PUTS */
#ifdef LOGIC_ANALYSER
	/* Put the char at a magic address, for Logic Analyser debugging */
	{
		volatile int32 *pointer = (int32 *)(LOGIC_ANALYSER_ADDR);
		if (c != '\n')
			*pointer = c;
	}
#endif
#ifdef BOOTSERIAL
	(*theBootGlobals->bg_PutC)(c);
	if (c == '\n' || c == '\r')
		(*theBootGlobals->bg_PutC)(0);
		
#endif
}

/*****************************************************************************
  puts
  Display a string
*/
void
puts(char *s)
{
#ifdef MEMORY_PUTS
	/* This single quote is to help identify strings in the memory buf */
	/* There is not a matching close quote because the memory buffer   */
	/* got too busy and it used up space unnecessarily.                */
	putc('`');
#endif
	while (*s)
            (*theBootGlobals->bg_PutC)(*s++);
}

/*****************************************************************************
  DisplayDec

  Display a dec value
*/
static void DisplayDec(uint32 iv, uint32 width, uint32 zfill)
{
	Boolean printing = FALSE;
	uint32 quotient;
	uint32 divisor;
	uint32 w;

	for (divisor = 1000000000, w = 9; divisor; divisor /= 10, w--) {
		quotient = iv / divisor;
		if (quotient) {
			printing = TRUE;
			iv -= quotient * divisor;
			putc('0'+quotient);
		} else if (printing) {
			putc('0');
		} else if (width > w) {
			printing = TRUE;
			putc(zfill ? '0' : ' ');
		}
	}
	if (!printing) {
		putc('0');
	}
	TOUCH(width);
}

/*****************************************************************************
  DisplayHex

  Display a hex value
  Width is ignored if greater than 8, always pads with zeros.
*/
static void DisplayHex(uint32 iv, uint32 width, Boolean upper)
{
	char chA;
	uint32 v = iv;
	int i;
	Boolean emit = FALSE;
	uint32 hexval;

	chA = upper ? 'A' : 'a';
	for (i = 0;  i < 8;  i++, v <<= 4)
	{
		if (!emit)
		{
			if ((v & 0xf0000000) == 0) {
				/* Don't display leading zeros */
				/* Unless width is larger than number */
				if ((8 - width) != i) {
					continue;
				}
			}
			emit = TRUE;
		}
		hexval = v >> 28;
		if (hexval < 10)
			putc((char)(hexval+'0'));
		else
			putc((char)(hexval+chA-10));
	}
	if (!emit)
		putc('0');
}

/*****************************************************************************
  printf

  Simple version of printf.  Only supports %c, %d, %s, %u, %x, %X.
  Absolutely no error checking of formats is done.
  Width stuff is only relevent for numbers for now.
*/
void
printf(const char *pFmt, ...)
{
	char ch;
	char aCh;
	char *pStr;
	int32 width = 0;
	int32 anInt32;
	uint32 aUInt32;
	Boolean formatting = FALSE;
	Boolean bigHex = FALSE;
	Boolean zfill = FALSE;
	va_list pVarArgs;


	va_start(pVarArgs, pFmt);
	while (TRUE) {
		ch = *pFmt++;
		if (ch == 0) break;

		if (formatting) {
			if (ch == 'X') {
				bigHex = TRUE;
				ch = 'x';
			}
			if ((ch >= 'c') && (ch <= 'x')) {
				if (!width) width = 1;
				if (ch == 'c') {
					aCh = (char)
						va_arg(pVarArgs, char);
					putc(aCh);
				} else if (ch == 'd') {
					anInt32 = (int32)
						va_arg(pVarArgs, int32);
					if (anInt32 < 0) {
						putc('-');
						anInt32 = -anInt32;
					}
					DisplayDec((uint32)anInt32, width, zfill);
				} else if (ch == 's') {
					pStr = (char *)
						va_arg(pVarArgs, char *);
					puts(pStr);
				} else if (ch == 'u') {
					aUInt32 = (uint32)
						va_arg(pVarArgs, uint32);
					DisplayDec((uint32)aUInt32, width, zfill);
				} else if (ch == 'x') {
					aUInt32 = (uint32)
						va_arg(pVarArgs, uint32);
					DisplayHex(aUInt32, width, bigHex);
					bigHex = FALSE;
				}
				formatting = FALSE;
				width = 0;
			} else if ((ch >= '0') && (ch <= '9')) {
				if (width == 0 && ch == '0')
					zfill = TRUE;
				width = width * 10 + (ch - '0');
			}
		} else {
			if (ch == '%') {
				formatting = TRUE;
				zfill = FALSE;
				continue;
			} else {
				putc(ch);
			}
		}
	}
	va_end(pVarArgs);
}
#else /* BUILD_STRINGS */
/* No-debug code goes here */
void InitPutChar(void) {}
void putc(int c) { TOUCH(c); }
void puts(char *s) { TOUCH(s); }
void printf(const char *pFmt, ...) { TOUCH(pFmt); }
#endif /* BUILD_STRINGS */

